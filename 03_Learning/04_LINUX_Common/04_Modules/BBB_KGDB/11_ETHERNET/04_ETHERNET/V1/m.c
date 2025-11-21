#include <linux/module.h>
#include <linux/fs.h> // alloc_chrdev_region
#include <linux/pci.h> // ioremap
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>
#include <linux/pm_runtime.h>
#include "mdio.h"

#define DRIVER_NAME "ether0_driver"
#define DEVICE_NAME "ether0"
#define DEVICE_CLASS "ether0_class"

static void re_request_irq_work(struct work_struct *work){
    struct ether_device_data *data = container_of(work, struct ether_device_data, re_request_work);

}

static int ether_open(struct inode *inode, struct file *file){
    struct ether_device_data *data = container_of(inode->i_cdev, struct ether_device_data, cdev);
    file->private_data = data;
    return 0;
}
static ssize_t ether_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct ether_device_data *data = filp->private_data;

    //ret = ether_GetDesc_Transfer(data);
    schedule_work(&data->re_request_work);

    return count;
}
static const struct file_operations ether_device_fops = {
    .owner = THIS_MODULE,
    .open = ether_open,
    .write = ether_write,
};

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
    data->base_mdio = ioremap(MDIO_BASE, 0x1000);
    data->base_ctrmod = ioremap(CTRMOD_BASE, 0x1000);
    data->base_clk = ioremap(CLK_BASE, 0x1000);
    if (!data->base_mdio) {
        dev_err(&pdev->dev, "Failed to ioremap MDIO\n");
        return -ENOMEM;
    }



    /* ===== Clock setup (assuming this part is unchanged) */
    // printk("clk = 0x%x\n", ioread32(data->base_clk + 0x14));
    // data->clk = devm_clk_get(&pdev->dev, "fck-ether");
    // if (IS_ERR(data->clk)) {
    //     dev_err(&pdev->dev, "Failed to get ether clock: %ld\n", PTR_ERR(data->clk));
    //     return PTR_ERR(data->clk);
    // }
    // ret = clk_prepare_enable(data->clk);
    // if (ret) {
    //     dev_err(&pdev->dev, "Failed to enable clock: %d\n", ret);
    //     return ret;
    // }
    // printk("clk = 0x%x\n", ioread32(data->base_clk + 0x14));

    // data->clk2 = devm_clk_get(&pdev->dev, "fck-mdio");
    // if (IS_ERR(data->clk2)) {
    //     dev_err(&pdev->dev, "Failed to get mdio clock: %ld\n", PTR_ERR(data->clk2));
    //     return PTR_ERR(data->clk2);
    // }
    // ret = clk_prepare_enable(data->clk2);
    // if (ret) {
    //     dev_err(&pdev->dev, "Failed to enable clock: %d\n", ret);
    //     return ret;
    // }

    // data->clk3 = devm_clk_get(&pdev->dev, "fck-cpts");
    // if (IS_ERR(data->clk3)) {
    //     dev_err(&pdev->dev, "Failed to get mdio clock: %ld\n", PTR_ERR(data->clk3));
    //     return PTR_ERR(data->clk3);
    // }
    // ret = clk_prepare_enable(data->clk3);
    // if (ret) {
    //     dev_err(&pdev->dev, "Failed to enable clock: %d\n", ret);
    //     return ret;
    // }

    // dev_info(&pdev->dev, "ether clock rate: %lu Hz\n", clk_get_rate(data->clk));
    // dev_info(&pdev->dev, "mdio clock rate: %lu Hz\n", clk_get_rate(data->clk2));
    // dev_info(&pdev->dev, "mdio clock rate: %lu Hz\n", clk_get_rate(data->clk3));
    // data->clk_freq = clk_get_rate(data->clk);

    // // Enable runtime PM for this device
    // pm_runtime_enable(&pdev->dev);    
    // // Power on the CPSW module NOW                 
    // ret = pm_runtime_get_sync(&pdev->dev);             
    // if (ret < 0) {
    //     dev_err(&pdev->dev, "pm_runtime_get_sync failed: %d\n", ret);
    //     pm_runtime_put_noidle(&pdev->dev);
    //     pm_runtime_disable(&pdev->dev);
    //     return ret;
    // }

    // iowrite32(0x2, data->base_clk + 0x14);
    // wait_register_update();
    // data->clk_freq = 125000000;
    // printk("clk = 0x%x\n", ioread32(data->base_clk + 0x14));

    // ========== Request IRQ ==========
    data->rx_thresh_irq = platform_get_irq(pdev, 0);
    if (data->rx_thresh_irq < 0) {
        dev_err(&pdev->dev, "Failed Formatted: Unable to get IRQ: %d\n", data->rx_thresh_irq);
        return data->rx_thresh_irq;
    }
    dev_info(&pdev->dev, "rx_thresh_irq num = %d\n", data->rx_thresh_irq);

    data->rx_irq = platform_get_irq(pdev, 1);
    if (data->rx_irq < 0) {
        dev_err(&pdev->dev, "Failed Formatted: Unable to get IRQ: %d\n", data->rx_irq);
        return data->rx_irq;
    }
    dev_info(&pdev->dev, "rx_irq num = %d\n", data->rx_irq);

    data->tx_irq = platform_get_irq(pdev, 2);
    if (data->tx_irq < 0) {
        dev_err(&pdev->dev, "Failed Formatted: Unable to get IRQ: %d\n", data->tx_irq);
        return data->tx_irq;
    }
    dev_info(&pdev->dev, "tx_irq num = %d\n", data->tx_irq);

    data->misc_irq = platform_get_irq(pdev, 3);
    if (data->misc_irq < 0) {
        dev_err(&pdev->dev, "Failed Formatted: Unable to get IRQ: %d\n", data->misc_irq);
        return data->misc_irq;
    }
    dev_info(&pdev->dev, "misc_irq num = %d\n", data->misc_irq);

    // ===== Create character device
    ret = alloc_chrdev_region(&data->dev_num, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to allocate chrdev region: %d\n", ret);
        return ret;
    }

    cdev_init(&data->cdev, &ether_device_fops);
    data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&data->cdev, data->dev_num, 1);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to add cdev: %d\n", ret);
        // iounmap(data->base_etherss);
        // iounmap(data->base_etherctl);
        // iounmap(data->base_etherphy);
        // iounmap(data->base_ethercore);
        return ret;
    }

    data->class = class_create(THIS_MODULE, DEVICE_CLASS);
    if (IS_ERR(data->class)) {
        dev_err(&pdev->dev, "Failed to create class: %ld\n", PTR_ERR(data->class));
        cdev_del(&data->cdev);
        // iounmap(data->base_etherss);
        // iounmap(data->base_etherctl);
        // iounmap(data->base_etherphy);
        // iounmap(data->base_ethercore);
        return PTR_ERR(data->class);
    }

    data->dev = device_create(data->class, &pdev->dev, data->dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(data->dev)) {
        dev_err(&pdev->dev, "Failed to create device: %ld\n", PTR_ERR(data->dev));
        class_destroy(data->class);
        cdev_del(&data->cdev);
        // iounmap(data->base_etherss);
        // iounmap(data->base_etherctl);
        // iounmap(data->base_etherphy);
        // iounmap(data->base_ethercore);
        return PTR_ERR(data->dev);
    }

    dev_info(&pdev->dev, "Created /dev/%s\n", DEVICE_NAME);

    gmii_sel_init(data);
    ret = clock_init(data);
    if (ret == 0){
        ret = ether_mdio_init(data);
        if (ret < 0) printk("Fail ether_mdio_init\n");
    }

    return 0;
}

static int ether_remove(struct platform_device *pdev)
{
    struct ether_device_data *data = platform_get_drvdata(pdev);

    printk(KERN_INFO "ether_remove called\n");

    if (data->dev) {
        device_destroy(data->class, data->dev_num);
        data->dev = NULL;
    }

    if (data->class) {
        class_destroy(data->class);
        data->class = NULL;
    }

    cdev_del(&data->cdev);
    unregister_chrdev_region(data->dev_num, 1);

    //clock_deinit(data);

    // pm_runtime_put_sync(&pdev->dev);
    // pm_runtime_disable(&pdev->dev);


    iounmap(data->base_mdio);
    iounmap(data->base_ctrmod);
    iounmap(data->base_clk);

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
