#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include "u1.h"

#define DRIVER_NAME "usb1_driver"
#define DEVICE_NAME "usb1"

struct usb_device_data {

    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;

    void __iomem *base_usbss;  
    void __iomem *base_usb1ctl;
    void __iomem *base_usb1phy;
    void __iomem *base_usb1core;
    struct clk *clk;
};

static int wait_register_update(struct usb_device_data *data, u16 offset, u16 bit_offset, u8 bit_val, u16 delay_ms, u8* name_register){
    unsigned long timeout;
    timeout = jiffies + msecs_to_jiffies(delay_ms);

    while ((ioread32(data->base + offset)&(1u << bit_offset)) != (bit_val << bit_offset))  // Wait register updated
    {
        if (time_after(jiffies, timeout)) {
            dev_err(data->dev, "Timeout %s\n", name_register);
            return -ETIMEDOUT;
        }
        cpu_relax();
    } 

    return 0;
}

static int usb1_probe(struct platform_device *pdev)
{
    struct usb_device_data *data;
    int ret;

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    platform_set_drvdata(pdev, data);

    data->dev = &pdev->dev;
    data->base_usbss = ioremap(BASE_USBSS, 0x1000);
    data->base_usb1ctl = ioremap(BASE_USB1CTL, 0x200);
    data->base_usb1phy = ioremap(BASE_USB1PHY, 0x100);
    data->base_usb1core = ioremap(BASE_USB1CORE, 0x400);

    /* ===== Clock setup (assuming this part is unchanged) */
    data->clk = devm_clk_get(&pdev->dev, "fck-usb1");
    if (IS_ERR(data->clk)) {
        dev_err(&pdev->dev, "Failed to get clock: %ld\n", PTR_ERR(data->clk));
        return PTR_ERR(data->clk);
    }
    ret = clk_prepare_enable(data->clk);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable clock: %d\n", ret);
        return ret;
    }
    dev_info(&pdev->dev, "can0 clock rate: %lu Hz\n", clk_get_rate(data->clk));

    // ===== Create character device
    ret = alloc_chrdev_region(&data->dev_num, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to allocate chrdev region: %d\n", ret);
        return ret;
    }

    cdev_init(&data->cdev, &can_device_fops);
    data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&data->cdev, data->dev_num, 1);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to add cdev: %d\n", ret);
        //unregister_chrdev_region(&data->dev_num, 1);
        iounmap(data->base);
        return ret;
    }

    data->class = class_create(THIS_MODULE, "can0_class");
    if (IS_ERR(data->class)) {
        dev_err(&pdev->dev, "Failed to create class: %ld\n", PTR_ERR(data->class));
        cdev_del(&data->cdev);
        //unregister_chrdev_region(&data->dev_num, 1);
        iounmap(data->base);
        return PTR_ERR(data->class);
    }

    data->dev = device_create(data->class, &pdev->dev, data->dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(data->dev)) {
        dev_err(&pdev->dev, "Failed to create device: %ld\n", PTR_ERR(data->dev));
        class_destroy(data->class);
        cdev_del(&data->cdev);
        //unregister_chrdev_region(&data->dev_num, 1);
        iounmap(data->base);
        return PTR_ERR(data->dev);
    }

    dev_info(&pdev->dev, "Created /dev/%s\n", DEVICE_NAME);

    return 0;
}

static int usb1_remove(struct platform_device *pdev)
{
    struct usb_device_data *data = platform_get_drvdata(pdev);

    if (data->dev)
        device_destroy(data->class, data->dev_num);
    if (data->class)
        class_destroy(data->class);
    cdev_del(&data->cdev);

    return 0;
}

static const struct of_device_id usb_device_of_match[] = {
    { .compatible = "usb1-based" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, usb_device_of_match);

static struct platform_driver usb_device_driver = {
    .probe = usb1_probe,
    .remove = usb1_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = usb_device_of_match,
    },
};

module_platform_driver(usb_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Soviet");
MODULE_DESCRIPTION("Custom USB1 Device Driver for BeagleBone Black USB1");
