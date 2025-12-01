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
};

struct net_device ndev_ins;

u8 macaddr[6] = {0x24, 0x76, 0x25, 0xe7, 0x29, 0xf0};

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

    p_create_ports(data);
    ret = p_register_ports(data);



    return ret;
}

static int ether_remove(struct platform_device *pdev)
{
    struct ether_device_data *data = platform_get_drvdata(pdev);
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