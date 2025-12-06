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

    /* Data received at the start */
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
    iowrite32(0x05, data->base_wr + WR_CONTROL); /* [TODO] */

    iowrite32(0xff, data->base_wr + WR_C0_RX_EN);
    iowrite32(0xff, data->base_wr + WR_C0_TX_EN);

    // Int channel 7 TX <-> DMA channel 7 TX
    iowrite32(BIT(chan_linear(data->tx_dma_channel)), data->base_cpdma + CPDMA_TXINTMASKSET);

    // Int channel 0 RX <-> DMA channel 32 RX
    iowrite32(BIT(chan_linear(data->rx_dma_channel)), data->base_cpdma + CPDMA_RXINTMASKSET);
}

void cpdma_intr_disable(struct ether_device_data* data){
    iowrite32(0, data->base_wr + WR_C0_RX_EN);
    iowrite32(0, data->base_wr + WR_C0_TX_EN);

    // Int channel 7 TX <-> DMA channel 7 TX
    iowrite32(BIT(chan_linear(data->tx_dma_channel)), data->base_cpdma + CPDMA_TXINTMASKCLEAR);

    // Int channel 0 RX <-> DMA channel 32 RX
    iowrite32(BIT(chan_linear(data->rx_dma_channel)), data->base_cpdma + CPDMA_RXINTMASKCLEAR);
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
    iowrite32(CPDMA_EOI_RX, data->base_cpdma + CPDMA_MACEOIVECTOR); 

    return IRQ_HANDLED; 
}

irqreturn_t tx_handler(int irq, void *dev_id){
    struct ether_device_data *data = dev_id;

    //printk("tx_handler\n");

    //cpdma_intr_disable(data);

    iowrite32(0, data->base_wr + WR_C0_TX_EN);
    iowrite32(CPDMA_EOI_TX, data->base_cpdma + CPDMA_MACEOIVECTOR); 

    napi_schedule(&data->napi_tx);

    return IRQ_HANDLED; 
}

irqreturn_t misc_handler(int irq, void *dev_id){
    struct ether_device_data *data = dev_id;

     printk("misc_handler\n");

    iowrite32(0, data->base_wr + WR_C0_MISC_EN);
    return IRQ_HANDLED; 
}

// =================== [cpdma_desc]
dma_addr_t desc_phys(struct cpdma_desc_pool *pool, struct cpdma_desc __iomem *desc)
{
	if (!desc){
        //printk("Fail: desc_phys\n");
        return 0;
    }
    //printk("pool->hw_addr = 0x%x\n", pool->hw_addr);
	return pool->hw_addr + (__force long)desc - (__force long)pool->iomap;
}

struct cpdma_desc __iomem *
desc_from_phys(struct cpdma_desc_pool *pool, dma_addr_t dma)
{
	return dma ? pool->iomap + dma - pool->hw_addr : NULL;
}

int cpdma_desc_pool_create(struct ether_device_data *data, phys_addr_t desc_mem_phys, u32 bd_ram_size, u32 descs_pool_size){
    struct cpdma_desc_pool *desc_pool;
    int ret;

    desc_pool = devm_kzalloc(data->dev, sizeof(*desc_pool), GFP_KERNEL);
    if (!desc_pool) {
        printk("Fail: desc_pool\n");
        data->desc_pool = NULL;
        return -1;
    }
    data->desc_pool = desc_pool;

    desc_pool->hw_addr = desc_mem_phys;

    desc_pool->desc_size = ALIGN(sizeof(struct cpdma_desc), 16); // 16 bytes
    desc_pool->gen_pool = devm_gen_pool_create(data->dev, ilog2(desc_pool->desc_size),
					      -1, "cpdma");

    desc_pool->mem_size = desc_pool->desc_size * descs_pool_size;  
    // desc_pool->iomap = ioremap(desc_mem_phys,
    //                 desc_pool->mem_size);              
    desc_pool->iomap = devm_ioremap(data->dev, desc_mem_phys,
                    desc_pool->mem_size);
    if (!desc_pool->iomap){
        printk("Fail: desc_pool->iomap\n");
        data->desc_pool = NULL;
        return -1;
    }

	ret = gen_pool_add_virt(desc_pool->gen_pool, (unsigned long)desc_pool->iomap,
				desc_mem_phys, desc_pool->mem_size, -1);

    if (ret < 0){
        printk("Fail: gen_pool_add_virt\n");
        return -1;
    }

    //printk("desc_pool->iomap = 0x%x\n", desc_pool->iomap);

    return 0;
}

struct cpdma_desc __iomem *
cpdma_desc_alloc(struct cpdma_desc_pool *pool)
{
	return (struct cpdma_desc __iomem *)
		gen_pool_alloc(pool->gen_pool, pool->desc_size);
}

void cpdma_desc_free(struct cpdma_desc_pool *pool, struct cpdma_desc __iomem *desc)
{
	gen_pool_free(pool->gen_pool, (unsigned long)desc, pool->desc_size);
}

/* DMA submit */
#define CPDMA_DMA_EXT_MAP BIT(16)
int cpdma_rx_fill(struct ether_device_data* data){
    u8 desc_num = 128;
    u8 i = 0;
    struct page *page;

    for (i = 0; i < desc_num; i++){
        page = page_pool_dev_alloc_pages(data->pool[data->rx_dma_channel]);
        if (!page) {
            printk("Error: allocate rx page\n");
            return -1;
        }
    }
    return 0;
}

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
    dma_addr_t buffer, desc_dma_phys;
    u32 mode;
    int ret;
    unsigned long flags;

    //printk("data->dev = 0x%x\n", data->dev);
    if (!data->dev){
        printk("ERROR\n");
        return;
    }

    spin_lock_irqsave(&data->lock, flags);

    buffer = dma_map_single(data->dev, buf, len, dir);
    ret = dma_mapping_error(data->dev, buffer);
    if (ret) {
        printk("Fail to dma_map_single\n");
        return ;
    }

    mode = CPDMA_DESC_OWNER | CPDMA_DESC_SOP | CPDMA_DESC_EOP;

    // must be tx
    if ((dir == 1) ||(dir == 2)) mode |= CPDMA_DESC_TO_PORT_EN | (dir << 16);

    //printk("hw_mode begin = 0x%x\n", mode);


    /* dma desc */
    // ret = cpdma_desc_pool_create(data, CPPIRAM_BASE,
    //          CPSW_BD_RAM_SIZE, CPSW_CPDMA_DESCS_POOL_SIZE_DEFAULT);
    /* Allocate desc_dma at phys addr */
    if (!data->desc_dma){
        data->desc_dma = cpdma_desc_alloc(data->desc_pool);
        desc_dma_phys = desc_phys(data->desc_pool, data->desc_dma);

        // desc_dma_phys = CPPIRAM_BASE;


        // fulfill desc
        iowrite32(0, &data->desc_dma->hw_next);
        iowrite32(buffer, &data->desc_dma->hw_buffer);
        iowrite32(len, &data->desc_dma->hw_len);
        iowrite32(mode | len, &data->desc_dma->hw_mode);

        iowrite32((u32)buf, &data->desc_dma->sw_token);
        iowrite32((u32)buf, &data->desc_dma->sw_buffer);
        iowrite32(len, &data->desc_dma->sw_len);

        //printk("hw_mode = 0x%x\n", ioread32(&data->desc_dma->hw_mode));

        // phys_addr_t phys = virt_to_phys(data->desc_dma);
        //printk("desc_dma = 0x%x, phys = 0x%x, ch = %d\n", data->desc_dma, desc_dma_phys, ch);

        // //iowrite32(0, data->base_txhdp + 4*ch);
        //printk("Before: 0x%x, current desc = 0x%x\n", ioread32(data->base_txhdp + 4*ch), data->desc_dma);
        // // store desc into hdp
        printk(">>> [TX] Begin transmit the packet\n");

        ETHER1_Print_Hex(buf, len, "txch");

        iowrite32(desc_dma_phys, data->base_txhdp + 4*ch); // at channel 7

        //printk("After: 0x%x, current desc = 0x%x\n", ioread32(data->base_txhdp + 4*ch), data->desc_dma);

    }
    spin_unlock_irqrestore(&data->lock, flags);
    // /* Free desc_dma */
    // cpdma_desc_free(data->desc_pool, data->desc_dma);
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

    // channel 0
    p_create_rx_pool(data, ch);

    ret = p_ndev_create_xdp_rxq(data, ch);
    if (ret){
        p_destroy_xdp_rxqs(data, ch);
        return ret;
    }

    return 0;
}

int tx_mq_poll(struct napi_struct *napi_tx, int budget){
    struct ether_device_data *data = container_of(napi_tx, struct ether_device_data, napi_tx);
    u8 ch = 0;
    dma_addr_t desc_dma;
    unsigned long flags;
    u32 token, len, hw_mode;

	spin_lock_irqsave(&data->lock, flags);

    ch = ioread32(data->base_cpdma + CPDMA_TXINTSTATMASKED);

    /*cpdma_desc_free(pool, desc, 1);*/
    if (data->desc_dma){
        /* = __cpdma_chan_process =*/
        /* Get dma addr of desc */
        desc_dma = desc_phys(data->desc_pool, data->desc_dma);
        /* Store desc to complete pointer */
        iowrite32(desc_dma, data->base_txcp + 4*data->tx_dma_channel);
        
        /* __cpdma_chan_free */
        token = ioread32(&data->desc_dma->sw_token);
        len = ioread32(&data->desc_dma->sw_len);
        hw_mode = ioread32(&data->desc_dma->hw_mode);

        dma_unmap_single(data->dev, desc_dma, len, 1);
        cpdma_desc_free(data->desc_pool, data->desc_dma);
        data->desc_dma = NULL;

        //printk("hw_mode end = 0x%x\n", hw_mode);
        /* cpsw_tx_handler */
    }

    /* End of queue and owner bit is clear */
    if ((hw_mode&CPDMA_DESC_EOQ) && (!(hw_mode&CPDMA_DESC_OWNER))){
        printk("<<< [TX] Done transmitted the last packet\n");

        /* End of tx_mq_poll */
        napi_complete(napi_tx);
        iowrite32(0xff, data->base_wr + WR_C0_TX_EN);
    }


    spin_unlock_irqrestore(&data->lock, flags);
    return 0;
}

static int cpsw_ndo_open(struct net_device *ndev){

    struct device *dev = ndev->dev.parent;
    struct ether_device_data* data = dev_get_drvdata(dev);
    int ret;

    printk("cpsw_ndo_open\n");
    ret = netif_set_real_num_tx_queues(ndev, data->tx_dma_channel);
    if (ret < 0) {
        printk("Fail netif_set_real_num_tx_queues\n");
        return -1;
    }

    if (ret == 0){
        ret = cpsw_open(data);
    }

    return ret;
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
    //printk("dummy_xmit\n");
    struct netdev_queue *txq;
    int q_idx;
    struct device *dev = ndev->dev.parent;
    struct ether_device_data* data = dev_get_drvdata(dev);
    if (!data) {
        printk("error getting data\n");
        return -1;
    }

    /* ndev for queues */
    q_idx = skb_get_queue_mapping(skb);
    if (q_idx >= data->tx_dma_channel){
        q_idx = q_idx % data->tx_dma_channel;
    }
    txq = netdev_get_tx_queue(ndev, q_idx);
    skb_tx_timestamp(skb);

    cpdma_submit_tx(data, skb->data, skb->len, 1, data->tx_dma_channel);

    // /* dma desc */
    // cpdma_desc_pool_create(data, CPPIRAM_BASE,
    //          CPSW_BD_RAM_SIZE, CPSW_CPDMA_DESCS_POOL_SIZE_DEFAULT);

    // data->desc_dma = ioremap(CPPIRAM_BASE, CPSW_BD_RAM_SIZE);

    // ETHER1_Print_Hex(skb->data, skb->len, "txch");
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

    //printk("0x%x - 0x%x\n", data, test);

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