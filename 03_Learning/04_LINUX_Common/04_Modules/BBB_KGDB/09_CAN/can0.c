#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>

#define DRIVER_NAME "can0_driver"

#define CAN0_BASE       0x481cc000
#define GPIO_BASE       0x44e10000

#define CAN_CTL 0x00
#define CAN_BTR 0x0C

#define CAN_CTL_INIT BIT(0)
#define CAN_CTL_CCE BIT(6)

struct can_device_data {

    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;

    void __iomem *base;  // Mapped base address of can0 registers
    void __iomem *base_gpio;
    struct clk *clk;
};

void can0_bit_timing(struct can_device_data *data){
    u32 fq = 500000; // Frequency Quantum: 500kHz
    u32 val = 0;

    u16 BRP = clk_get_rate(data->clk)/fq; // 96

    u8 SJW = 1;
    u8 TSeg1 = 4;
    u8 TSeg2 = 2;

    BRP = BRP - 1;
    SJW = SJW - 1;
    TSeg1 = TSeg1 - 1;
    TSeg2 = TSeg2 - 1;

    val = ((BRP)&0x3F) | ((SJW << 6)&0xC0) | ((TSeg1 << 8)&0xF00) | ((TSeg2 << 12)&0x7000);

    iowrite32(val, data->base + CAN_BTR);


}

void can0_init(struct can_device_data *data) {
    u32 can_ctl = 0;

    can_ctl = CAN_CTL_INIT | CAN_CTL_CCE;
    iowrite32(can_ctl, data->base + CAN_CTL); // enter init mode, access to registers
    while((ioread32(data->base + CAN_CTL)&CAN_CTL_INIT) == CAN_CTL_INIT); // wait init = 0;

    // Bit timing values into BTR
    can0_bit_timing(data);

    // clear init, CCE
    can_tcl &=~ (CAN_CTL_INIT | CAN_CTL_CCE);
    iowrite32(can_ctl, data->base + CAN_CTL); // enter init mode, access to registers
    while((ioread32(data->base + CAN_CTL)&CAN_CTL_INIT) != CAN_CTL_INIT); // wait init = 0;


}

static int can0_open(struct inode *inode, struct file *file){
    struct can_device_data *data = container_of(inode->i_cdev, struct can_device_data, cdev);
    file->private_data = data;
    return 0;
}

static ssize_t can0_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct can_device_data *data = filp->private_data;

    return count;
}

static const struct file_operations can_device_fops = {
    .owner = THIS_MODULE,
    .open = can0_open,
    .write = can0_write,
};

static int can0_probe(struct platform_device *pdev)
{
    struct can_device_data *data;
    int ret;

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    platform_set_drvdata(pdev, data);

    data->base = ioremap(CAN0_BASE, 0x1000);
    data->base_gpio = ioremap(GPIO_BASE, 0x1000);

    /* ===== Clock setup (assuming this part is unchanged) */
    data->clk = devm_clk_get(&pdev->dev, "fck-can0");
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

    data->dev = device_create(data->class, &pdev->dev, data->dev_num, NULL, DRIVER_NAME);
    if (IS_ERR(data->dev)) {
        dev_err(&pdev->dev, "Failed to create device: %ld\n", PTR_ERR(data->dev));
        class_destroy(data->class);
        cdev_del(&data->cdev);
        //unregister_chrdev_region(&data->dev_num, 1);
        iounmap(data->base);
        return PTR_ERR(data->dev);
    }

    dev_info(&pdev->dev, "Created /dev/%s\n", DRIVER_NAME);

    return 0;
}

static int can0_remove(struct platform_device *pdev)
{
    struct can_device_data *data = platform_get_drvdata(pdev);
    
    if (data->dev)
        device_destroy(data->class, data->dev_num);
    if (data->class)
        class_destroy(data->class);
    cdev_del(&data->cdev);

    dev_info(data->dev, "Removed\n");
    return 0;
}

static const struct of_device_id can_device_of_match[] = {
    { .compatible = "can0-based" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, can_device_of_match);

static struct platform_driver can_device_driver = {
    .probe = can0_probe,
    .remove = can0_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = can_device_of_match,
    },
};

module_platform_driver(can_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Soviet");
MODULE_DESCRIPTION("Custom CAN Device Driver for BeagleBone Black CAN0");

