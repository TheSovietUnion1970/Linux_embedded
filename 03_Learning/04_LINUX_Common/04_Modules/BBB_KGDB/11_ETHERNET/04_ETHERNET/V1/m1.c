// minimal_eth0_only.c — only creates eth0 (copy-paste ready)

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <net/page_pool.h>
#include <net/xdp.h>
#include <linux/fs.h> // alloc_chrdev_region
#include <linux/pci.h> // ioremap
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <linux/ethtool.h>

#define DRIVER_NAME "minimal_eth0"
#define DEVICE_NAME "eth0"

struct ether_device_data {
    /* Essential */
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;
    struct platform_device *pdev;   // ← ADD THIS

    struct cpdma_desc *desc_dma; 

    struct net_device *ndev;
    struct xdp_rxq_info *rxq;
    struct page_pool *pool;
	struct napi_struct		napi_rx;
	struct napi_struct		napi_tx;

    struct xdp_rxq_info		xdp_rxq[8];
};
struct page *page;
struct net_device ndev_ins;

u8 macaddr[6] = {0x24, 0x76, 0x25, 0xe7, 0x29, 0xf0};
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

static int cpsw_ndo_vlan_rx_add_vid(struct net_device *ndev,
				    __be16 proto, u16 vid){
    return 0;
}

static int cpsw_ndo_vlan_rx_kill_vid(struct net_device *ndev,
				    __be16 proto, u16 vid){
    return 0;
}

static void cpsw_get_drvinfo(struct net_device *ndev,
			     struct ethtool_drvinfo *info){
    return;
}

static netdev_tx_t dummy_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    printk("dummy_xmit\n");
    // struct ether_device_data* data = netdev_priv(ndev);
    // struct ether_device_data *data = container_of(work, struct ether_device_data, re_request_work);
    struct device *dev = ndev->dev.parent;
    struct ether_device_data* data = dev_get_drvdata(dev);

    printk("data = 0x%x, dev = 0x%x\n", data, dev);

    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
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



int p_create_ports(struct ether_device_data *data){
    printk("data = 0x%x, data->dev = 0x%x\n", data, data->dev);
    data->ndev = devm_alloc_etherdev_mqs(data->dev, sizeof(struct ether_device_data),
                        8,
                        8);

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

// ===================
/* Page pool funcs for dma physical addr */
void p_create_rx_pool(struct ether_device_data *data){
    int pool_size = 128;

    /* cpsw_create_page_pool for rx channel 32 (1) */
    struct page_pool_params pp_params = {};

	pp_params.order = 0;
	pp_params.flags = PP_FLAG_DMA_MAP;
	pp_params.pool_size = pool_size;
	pp_params.nid = NUMA_NO_NODE;
	pp_params.dma_dir = DMA_BIDIRECTIONAL;
	pp_params.dev = data->dev;

    data->pool = page_pool_create(&pp_params);

    if (IS_ERR(data->pool)){
        printk("cannot create rx page pool\n");
    } 

}

void p_ndev_destroy_xdp_rxq(struct ether_device_data *data){
	if (!xdp_rxq_info_is_reg(data->rxq))
		return;

	xdp_rxq_info_unreg(data->rxq);
}
int p_ndev_create_xdp_rxq(struct ether_device_data *data){
    int ret;
    u32 queue_index = 1;

    data->rxq = &data->xdp_rxq[0];

    if (!data->ndev) return -1;
    ret = xdp_rxq_info_reg((data->rxq), data->ndev, queue_index, 0);
	if (ret)
		return ret;
    
	ret = xdp_rxq_info_reg_mem_model((data->rxq), MEM_TYPE_PAGE_POOL, data->pool);
	if (ret)
		xdp_rxq_info_unreg((data->rxq));

	return ret; 
}

void p_destroy_xdp_rxqs(struct ether_device_data *data){
    p_ndev_destroy_xdp_rxq(data);
    page_pool_recycle_direct(data->pool, page);
    page_pool_destroy(data->pool);
}
int p_create_xdp_rxqs(struct ether_device_data *data){
    int ret;

    // channel 32
    p_create_rx_pool(data);

    ret = p_ndev_create_xdp_rxq(data);
    if (ret){
        p_destroy_xdp_rxqs(data);
        return ret;
    }

    return 0;
}

// === alocate dma using pool
void create_dma(struct ether_device_data* data, u8* buf, u16 len, u8 dir){
    dma_addr_t buffer;
    u32 mode;
    u8* tmp;
    
    //printk("data->pool = 0x%x\n", data->pool);
    page = page_pool_dev_alloc_pages(data->pool);
    if (!page){
        printk("allocate rx page err");
        return;
    }
    printk("DONE - page_pool_dev_alloc_pages\n");

    buffer = page_pool_get_dma_addr(page);

    tmp = page_address(page);
    memcpy(tmp, buf, len);

    dma_sync_single_for_device(data->dev, buffer, len, dir);

}
// ============================================= [PROBE] =======================
static int ether_probe(struct platform_device *pdev)
{
    struct ether_device_data *data;
    int ret;

    //dev_info(&pdev->dev, "Probed\n");
    printk("probed\n");

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    platform_set_drvdata(pdev, data);
    data->dev = &pdev->dev;
    data->pdev = pdev;
    dev_set_drvdata(data->dev, data);

    p_create_ports(data);
    ret = p_register_ports(data);

    ret = p_create_xdp_rxqs(data);
    if (ret < 0) return -1;

    napi_enable(&data->napi_tx);

    create_dma(data, test_packet, 60, 1);

    return ret;
}

static int ether_remove(struct platform_device *pdev)
{
    struct ether_device_data *data = platform_get_drvdata(pdev);

    napi_disable(&data->napi_tx);
    if (data->rxq) p_destroy_xdp_rxqs(data);

    if (data->ndev) {
        unregister_netdev(data->ndev);
        //free_netdev(data->ndev);
        printk(KERN_INFO "minimal_eth0: eth0 removed\n");
    }
    return 0;
}
static const struct of_device_id ether_device_of_match[] = {
    { .compatible = "ether-based" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ether_device_of_match);

static struct platform_driver ether_device_driver = {
    .probe = ether_probe,
    .remove = ether_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = ether_device_of_match,
    },
};

module_platform_driver(ether_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Soviet");
MODULE_DESCRIPTION("Custom ether Device Driver for BeagleBone Black ether");