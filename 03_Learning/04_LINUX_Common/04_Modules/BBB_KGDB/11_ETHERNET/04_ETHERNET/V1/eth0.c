// minimal_eth0.c — shows eth0 using only alloc_etherdev + register_netdev

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>

#include <linux/fs.h> // alloc_chrdev_region
#include <linux/pci.h> // ioremap
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define DRIVER_NAME "ether0_driver"
#define DEVICE_NAME "ether0"
#define DEVICE_CLASS "ether0_class"

static struct net_device *my_ndev;

static int my_open(struct net_device *dev)
{
    netif_start_queue(dev);
    printk(KERN_INFO "my_eth0: interface opened\n");
    return 0;
}

static int my_stop(struct net_device *dev)
{
    netif_stop_queue(dev);
    printk(KERN_INFO "my_eth0: interface stopped\n");
    return 0;
}

static const struct net_device_ops my_netdev_ops = {
    .ndo_open       = my_open,
    .ndo_stop       = my_stop,
    .ndo_start_xmit = NULL,  // we don't transmit
};

static int ether_probe(struct platform_device *pdev)
{
    int ret;

    /* 1. Create the net_device — this is the magic */
    my_ndev = alloc_etherdev(0);           // size 0 = no private data
    if (!my_ndev)
        return -ENOMEM;

    /* Optional: give it a name */
    strcpy(my_ndev->name, "eth0");

    /* Set some basic things */
    my_ndev->netdev_ops = &my_netdev_ops;
    eth_hw_addr_random(my_ndev);           // random MAC

    /* 2. Register it — this makes eth0 appear! */
    ret = register_netdev(my_ndev);
    if (ret) {
        printk(KERN_ERR "Failed to register netdev: %d\n", ret);
        free_netdev(my_ndev);
        return ret;
    }

    printk(KERN_INFO "my_eth0: eth0 created with MAC %pM\n", my_ndev->dev_addr);
    return 0;
}

static int ether_remove(struct platform_device *pdev)
{
    if (my_ndev) {
        unregister_netdev(my_ndev);
        free_netdev(my_ndev);
    }
    printk(KERN_INFO "my_eth0: removed\n");
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