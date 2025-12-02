#include "cpdma.h"
#include "cpsw.h"

#include <linux/delay.h>
#include <net/page_pool.h>
#include <net/xdp.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/dma-mapping.h>

/*
DMA 64 channels:
0-31 Tx
32-64 Rx

tx idx = 7 => Tx ch = 0 + 7 = 7
rx idx = 0 => Rx ch = 32 + 0 = 32

TxDma[n] <-> TxInt[n], 0 =< n <= 7
RxDma[n + 32] <-> RxInt[n], 0 =< n <= 7

#define __chan_linear(chan_num)	((chan_num) & (CPDMA_MAX_CHANNELS - 1))
*/

int cpdma_ctlr_start(struct ether_device_data* data){
    int ret = 0;
    u8 i = 0;
    u32 cpdma_control = 0;

    iowrite32(1, data->base_cpdma + CPDMA_SOFTRESET);
    ret = wait_register_update(data, data->base_cpdma, CPDMA_SOFTRESET, 0, BIT_VAL_0, 2000, "CPDMA_SOFTRESET");
    if (ret < 0) return -1;

    // 8 chan num
    for (i = 0; i < 8; i++){
        iowrite32(0, data->base_txhdp + 4*i);
        iowrite32(0, data->base_rxhdp + 4*i);
        iowrite32(0, data->base_txcp + 4*i);
        iowrite32(0, data->base_rxcp + 4*i);
    }

    iowrite32(0xffffffff, data->base_cpdma + CPDMA_RXINTMASKCLEAR);
    iowrite32(0xffffffff, data->base_cpdma + CPDMA_TXINTMASKCLEAR);

    // enable
    iowrite32(1, data->base_cpdma + CPDMA_TXCONTROL);
    iowrite32(1, data->base_cpdma + CPDMA_RXCONTROL);

    /* ctlr->state = CPDMA_STATE_ACTIVE; */

    /*
    cpdma_chan_set_chan_shaper
    cpdma_chan_on
    -> no need as chan->rate = 0 and CPDMA_STATE_ACTIVE
    */

    cpdma_control = ioread32(data->base_cpdma + CPDMA_CONTROL);
    cpdma_control |= TX_PTYPE; // uses the highest priority 7
    iowrite32(cpdma_control, data->base_cpdma + CPDMA_CONTROL);

    iowrite32(0, data->base_cpdma + CPDMA_RX_BUFF_OFFSET);

    return 0;
}

int cpdma_ctlr_stop(struct ether_device_data* data){
    iowrite32(0xffffffff, data->base_cpdma + CPDMA_RXINTMASKCLEAR);
    iowrite32(0xffffffff, data->base_cpdma + CPDMA_TXINTMASKCLEAR);

    // disable
    iowrite32(0, data->base_cpdma + CPDMA_TXCONTROL);
    iowrite32(0, data->base_cpdma + CPDMA_RXCONTROL);

    /* ctlr->state = CPDMA_STATE_IDLE; */

    return 0;
}

void cpdma_intr_enable(struct ether_device_data* data){
    iowrite32(0xff, data->base_wr + WR_C0_RX_EN);
    iowrite32(0xff, data->base_wr + WR_C0_TX_EN);

    // Int channel 7 TX <-> DMA channel 7 TX
    iowrite32(chan_linear(data->tx_dma_channel), data->base_cpdma + TX_INT_SET);

    // Int channel 0 RX <-> DMA channel 32 RX
    iowrite32(chan_linear(data->rx_dma_channel), data->base_cpdma + RX_INT_SET);
}

void cpdma_intr_disable(struct ether_device_data* data){
    iowrite32(0, data->base_wr + WR_C0_RX_EN);
    iowrite32(0, data->base_wr + WR_C0_TX_EN);

    // Int channel 7 TX <-> DMA channel 7 TX
    iowrite32(chan_linear(data->tx_dma_channel), data->base_cpdma + TX_INT_CLEAR);

    // Int channel 0 RX <-> DMA channel 32 RX
    iowrite32(chan_linear(data->rx_dma_channel), data->base_cpdma + RX_INT_CLEAR);
}

/* IRQ handlers */
irqreturn_t rx_thresh_handler(int irq, void *dev_id){
    struct ether_device_data *data = dev_id;

    printk("rx_thresh_handler\n");

    iowrite32(0, data->base_wr + WR_C0_RX_THRESH_EN);
    return IRQ_HANDLED; 
}

irqreturn_t rx_handler(int irq, void *dev_id){
    struct ether_device_data *data = dev_id;

    printk("rx_handler\n");

    iowrite32(0, data->base_wr + WR_C0_RX_EN);
    return IRQ_HANDLED; 
}

irqreturn_t tx_handler(int irq, void *dev_id){
    struct ether_device_data *data = dev_id;

    printk("tx_handler\n");

    iowrite32(0, data->base_wr + WR_C0_TX_EN);
    return IRQ_HANDLED; 
}

irqreturn_t misc_handler(int irq, void *dev_id){
    struct ether_device_data *data = dev_id;

     printk("misc_handler\n");

    iowrite32(0, data->base_wr + WR_C0_MISC_EN);
    return IRQ_HANDLED; 
}

/* DMA submit */
#define CPDMA_DMA_EXT_MAP BIT(16)
void cpdma_submit_rx(struct ether_device_data* data, u8* buf, u16 len, u8 dir, int ch){
    dma_addr_t buffer;
    u32 mode;
    struct page *page;
    u8* tmp;

    //printk("data->pool = 0x%x\n", data->pool);
    page = page_pool_dev_alloc_pages(data->pool[ch]);
    if (!page){
        printk("allocate rx page err");
        return;
    }
    data->page[ch] = page;
    printk("DONE - page_pool_dev_alloc_pages\n");

    // buffer = page_pool_get_dma_addr(page) + CPSW_HEADROOM_NA;
    buffer = page_pool_get_dma_addr(page);

    tmp = page_address(page);
    memcpy(tmp, buf, len);

    mode = CPDMA_DESC_OWNER | CPDMA_DESC_SOP | CPDMA_DESC_EOP;

    // must be tx
    if ((dir == 1) ||(dir == 2)) mode |= CPDMA_DESC_TO_PORT_EN | (dir << 16);

    dma_sync_single_for_device(data->dev, buffer, len, dir);

    // fulfill desc
    iowrite32(0, &data->desc_dma->hw_next);
    iowrite32(buffer, &data->desc_dma->hw_buffer);
    iowrite32(len, &data->desc_dma->hw_len);
    iowrite32(mode, &data->desc_dma->hw_mode);

    iowrite32((u32)buf, &data->desc_dma->sw_token);
    iowrite32(buffer, &data->desc_dma->sw_buffer);
    iowrite32(len | CPDMA_DMA_EXT_MAP, &data->desc_dma->sw_len);

    // store desc into hdp
    iowrite32((u32)data->desc_dma, data->base_rxhdp + 4*ch); // at channel 7

}

void cpdma_submit_tx(struct ether_device_data* data, u8* buf, u16 len, u8 dir, int ch){
    dma_addr_t buffer;
    u32 mode;
    int ret;

    printk("data->dev = 0x%x\n", data->dev);
    if (!data->dev){
        printk("ERROR\n");
        return;
    }
    buffer = dma_map_single(data->dev, buf, len, dir);
    ret = dma_mapping_error(data->dev, buffer);
    if (ret) {
        printk("Fail to dma_map_single\n");
        return ;
    }

    mode = CPDMA_DESC_OWNER | CPDMA_DESC_SOP | CPDMA_DESC_EOP;

    // must be tx
    if ((dir == 1) ||(dir == 2)) mode |= CPDMA_DESC_TO_PORT_EN | (dir << 16);

    // fulfill desc
    iowrite32(0, &data->desc_dma->hw_next);
    iowrite32(buffer, &data->desc_dma->hw_buffer);
    iowrite32(len, &data->desc_dma->hw_len);
    iowrite32(mode, &data->desc_dma->hw_mode);

    iowrite32((u32)buf, &data->desc_dma->sw_token);
    iowrite32((u32)buf, &data->desc_dma->sw_buffer);
    iowrite32(len, &data->desc_dma->sw_len);

    phys_addr_t phys = virt_to_phys(data->desc_dma);
    printk("data->desc_dma = 0x%x, phys = 0x%x\n", (u32)data->desc_dma, (u32)phys);

    //iowrite32(0, data->base_txhdp + 4*ch);
    printk("Before: 0x%x, current desc = 0x%x\n", ioread32(data->base_txhdp + 4*ch), data->desc_dma);
    // store desc into hdp
    iowrite32((u32)data->desc_dma, data->base_txhdp + 4*ch); // at channel 7

    dma_unmap_single(data->dev, buffer, len, dir);
}


/* Page pool funcs for dma physical addr */
void p_create_rx_pool(struct ether_device_data *data, int ch){
    int pool_size = 128; /* TODO */

    /* cpsw_create_page_pool for rx channel 32 (1) */
    struct page_pool_params pp_params = {};

	pp_params.order = 0;
	pp_params.flags = PP_FLAG_DMA_MAP;
	pp_params.pool_size = pool_size;
	pp_params.nid = NUMA_NO_NODE;
	pp_params.dma_dir = DMA_BIDIRECTIONAL;
	pp_params.dev = data->dev;

    data->pool[ch] = page_pool_create(&pp_params);

    if (IS_ERR(data->pool[ch])){
        printk("cannot create rx page pool\n");
    } 

}

void p_ndev_destroy_xdp_rxq(struct ether_device_data *data, int ch){
	// if (!xdp_rxq_info_is_reg(&data->xdp_rxq[ch]))
	// 	return;

	xdp_rxq_info_unreg(&data->xdp_rxq[ch]);
}
int p_ndev_create_xdp_rxq(struct ether_device_data *data, int ch){
    int ret;
    u32 queue_index = 1;
    struct xdp_rxq_info* rxq;

    rxq = &data->xdp_rxq[ch];

    if (!data->ndev) return -1;
    ret = xdp_rxq_info_reg((rxq), data->ndev, queue_index, 0);
	if (ret)
		return ret;
    
	ret = xdp_rxq_info_reg_mem_model((rxq), MEM_TYPE_PAGE_POOL, data->pool);
	if (ret)
		xdp_rxq_info_unreg((rxq));

	return ret; 
}

void p_destroy_xdp_rxqs(struct ether_device_data *data, int ch){
    //page_pool_recycle_direct(data->pool[ch], data->page[ch]); // without this -> page_pool_release_retry() stalled pool shutdown 1 inflight
    p_ndev_destroy_xdp_rxq(data, ch);
    //page_pool_recycle_direct(data->pool[ch], data->page[ch]); 
    page_pool_destroy(data->pool[ch]);
}
int p_create_xdp_rxqs(struct ether_device_data *data, int ch){
    int ret;

    // channel 32
    p_create_rx_pool(data, ch);

    ret = p_ndev_create_xdp_rxq(data, ch);
    if (ret){
        p_destroy_xdp_rxqs(data, ch);
        return ret;
    }

    return 0;
}

int tx_mq_poll(struct napi_struct *napi_rx, int budget){
    printk("cpsw_rx_mq_poll\n");
    return 0;
}

static int cpsw_ndo_open(struct net_device *ndev){
    return 0;
}

static int cpsw_ndo_stop(struct net_device *ndev){
    return 0;
}

static void cpsw_get_drvinfo(struct net_device *ndev,
			     struct ethtool_drvinfo *info){
    return;
}

static netdev_tx_t dummy_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    printk("dummy_xmit\n");
    struct device *dev = ndev->dev.parent;
    struct ether_device_data* data = dev_get_drvdata(dev);
    if (!data) {
        printk("error getting data\n");
        return -1;
    }

    skb_tx_timestamp(skb);
    cpdma_submit_tx(data, skb->data, skb->len, 1, data->tx_dma_channel);

    ETHER1_Print_Hex(skb->data, skb->len, "txch");
    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

static int cpsw_ndo_vlan_rx_add_vid(struct net_device *ndev,
				    __be16 proto, u16 vid){
    return 0;
}

static int cpsw_ndo_vlan_rx_kill_vid(struct net_device *ndev,
				    __be16 proto, u16 vid){
    return 0;
}

u32 cpsw_get_msglevel(struct net_device *ndev)
{
	return 0;
}

static const struct net_device_ops cpsw_netdev_ops = {
	.ndo_open		= cpsw_ndo_open,
	.ndo_stop		= cpsw_ndo_stop,
    .ndo_start_xmit = dummy_xmit,

	.ndo_vlan_rx_add_vid	= cpsw_ndo_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= cpsw_ndo_vlan_rx_kill_vid,
};

static const struct ethtool_ops cpsw_ethtool_ops = {
	.supported_coalesce_params = ETHTOOL_COALESCE_RX_USECS,
	.get_drvinfo		= cpsw_get_drvinfo,
	.get_msglevel		= cpsw_get_msglevel,
};

u8 macaddr[6] = {0x24, 0x76, 0x25, 0xe7, 0x29, 0xf0};
/*
return alloc_netdev_mqs(sizeof_priv, "eth%d", NET_NAME_UNKNOWN,
				ether_setup, txqs, rxqs);
*/
int p_create_ports(struct ether_device_data *data){
    struct ether_device_data *test;
    data->ndev = devm_alloc_etherdev_mqs(data->dev, sizeof(struct ether_device_data),
                        CPSW_MAX_QUEUES,
                        CPSW_MAX_QUEUES);

    test = netdev_priv(data->ndev);

    printk("0x%x - 0x%x\n", data, test);

    eth_hw_addr_set(data->ndev, macaddr);

    data->ndev->features |= NETIF_F_HW_VLAN_CTAG_FILTER |
                NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_NETNS_LOCAL;

    //data->ndev->features = 0;

    data->ndev->netdev_ops = &cpsw_netdev_ops;
    data->ndev->ethtool_ops = &cpsw_ethtool_ops;

    SET_NETDEV_DEV(data->ndev, data->dev);

    /* #define CPSW_POLL_WEIGHT	64 */
    netif_napi_add(data->ndev, &data->napi_tx,
                tx_mq_poll,
                64);

    return 0;
}

int p_register_ports(struct ether_device_data *data){
    int ret;

    if (!data->ndev) {
        return -1;
    }
    ret = register_netdev(data->ndev);
    if (ret) {
        printk("err registering net device\n");
        return -1;
    }

    return 0;
}


void test_send_packet(struct ether_device_data *data)
{
    /* Ethernet + IP + ICMP Echo Reply (60 bytes total) */
    u8 test_packet[60] = {
        /* Ethernet header (14 bytes) */
        0x00, 0x1b, 0x21, 0xaa, 0xbb, 0xcc,   // dst MAC (your PC)
        0x24, 0x76, 0x25, 0xe7, 0x29, 0xf0,   // src MAC (BBB eth0)
        0x08, 0x00,                           // EtherType: IPv4

        /* IP header (20 bytes) */
        0x45, 0x00, 0x00, 0x3c,               // Version, IHL, DSCP, total length 60
        0x00, 0x00, 0x40, 0x00,               // ID, flags, TTL=64
        0x40, 0x01, 0xab, 0xcd,               // Protocol=ICMP, checksum
        0xc0, 0xa8, 0x8a, 0x02,               // src IP 192.168.138.2
        0xc0, 0xa8, 0x8a, 0x01,               // dst IP 192.168.138.1

        /* ICMP Echo Reply (8 bytes + 18 padding) */
        0x00, 0x00, 0x12, 0x34,               // Type=0 (reply), code=0, checksum
        0x00, 0x01, 0x08, 0x09,               // ID=1, seq=1
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72
    };

    printk(KERN_INFO "Sending test packet (60 bytes)\n");

    /* Your function — transmit on channel 7, directed to Port 1 (RJ45) */
    cpdma_submit_rx(data, test_packet, 60, 1, data->tx_dma_channel);  // dir=1 = to Port 1

    /* Wait a bit so packet goes out */
    mdelay(10);
}

/* Call this from your probe or a sysfs trigger */
void run_test(struct ether_device_data *data)
{
    int i;
    for (i = 0; i < 1; i++) {
        test_send_packet(data);
        msleep(500);   // 0.5 sec between packets
    }
}