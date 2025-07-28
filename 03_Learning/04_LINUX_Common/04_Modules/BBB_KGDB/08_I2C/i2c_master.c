#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/clk.h>

#define SLAVE_ADDRESS 0x40

// Register base addresses
#define I2C1_BASE       0x4802A000
#define CM_PER_BASE     0x44E00000

// CM_PER registers
//#define CM_PER_I2C1_CLKCTRL    (*(volatile uint32_t *)(CM_PER_BASE + 0x48))

// I2C1 registers (offsets)
#define I2C_SYSC        0x10  // System configuration
#define I2C_SYSS        0x90  // System status
#define I2C_CON         0xA4  // Control
#define I2C_PSC         0xB0  // Prescaler
#define I2C_SCLL        0xB4  // SCL low time
#define I2C_SCLH        0xB8  // SCL high time
#define I2C_SA          0xAC  // Slave address
#define I2C_CNT         0x98  // Data count
#define I2C_DATA        0x9C  // Data
#define I2C_IRQENABLE_SET 0x2C  // Enable interrupts
#define I2C_IRQSTATUS_RAW 0x24  // Interrupt raw status
#define I2C_IRQSTATUS   0x28  // Interrupt status

#define I2C_IRQSTATUS_RAW_XRDY BIT(4)
#define I2C_IRQSTATUS_RAW_BB BIT(12)
#define I2C_IRQSTATUS_RAW_ARDY BIT(2)
#define I2C_IRQSTATUS_RAW_NACK BIT(1)

#define DRIVER_NAME "i2c1_device_driver"
#define DEVICE_NAME "i2c1"

struct i2c_device_data {
    struct i2c_adapter *adap;

    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;

    void __iomem *base;  // Mapped base address of I2c1 registers
    struct clk *clk;

    int irq;
};

// // Enable module clock for I2C1
// void enable_i2c1_clock(struct i2c_device_data *data) {
//     CM_PER_I2C1_CLKCTRL = 0x2;  // Enable I2C1 clock
//     while (!(CM_PER_I2C1_CLKCTRL & 0x2));  // Wait for enable
// }



// Initialize I2C1 as master
void i2c1_master_init(struct i2c_device_data *data) {
    // enable_i2c1_clock(data);
    u32 i2c_con;

    iowrite32(0x2, data->base + I2C_SYSC); // Set soft reset
    while (!(ioread32(data->base + I2C_SYSS)&(1u)));  // Wait for reset complete
    iowrite32(0x0, data->base + I2C_SYSC); // Clear reset - normal mode

    i2c_con = ioread32(data->base + i2c_con);
    i2c_con &= ~(1u << 15); // disable i2c module
    iowrite32(i2c_con, data->base + I2C_CON);

    iowrite32(5, data->base + I2C_PSC); // Prescaler: 24 MHz / (5+1) = 4 MHz

    iowrite32(8, data->base + I2C_SCLL); // SCL low time
    iowrite32(10, data->base + I2C_SCLH); // SCL high time
    // Total period = (8 + 10) * (1/4 MHz) = 4.5 µs (~400 kHz)

    i2c_con = ioread32(data->base + i2c_con);
    i2c_con |= (1u << 15)|(1u << 10); // [15] enable i2c module, [10]: master mode
    iowrite32(i2c_con, data->base + I2C_CON);

    //i2c1[I2C_IRQENABLE_SET / 4] = 0x64C;  // Enable XRDY, RRDY, BB interrupts
}

static int i2c1_master_open(struct inode *inode, struct file *file)
{
    struct i2c_device_data *data = container_of(inode->i_cdev, struct i2c_device_data, cdev);
    file->private_data = data;
    return 0;
}

// Write data to slave
int i2c1_write(struct i2c_device_data *data, uint16_t slave_addr, uint8_t *tx, uint32_t len) {
    uint32_t i;
    u32 i2c_sts_raw, i2c_con;

    while ((ioread32(data->base + I2C_IRQSTATUS_RAW)&I2C_IRQSTATUS_RAW_BB) == I2C_IRQSTATUS_RAW_BB);  // Wait for bus to be free

    iowrite32(slave_addr, data->base + I2C_SA); // Set slave address
    iowrite32(len, data->base + I2C_CNT); // Number of bytes to write

    i2c_con = ioread32(data->base + i2c_con);
    i2c_con |= 0x1; // Start condition
    iowrite32(i2c_con, data->base + I2C_CON);


    for (i = 0; i < len; i++) {
        while ((ioread32(data->base + I2C_IRQSTATUS_RAW)&(I2C_IRQSTATUS_RAW_XRDY)) != I2C_IRQSTATUS_RAW_XRDY);  // Wait for XRDY (transmit data ready)
        iowrite32(tx[i], data->base + I2C_DATA); // Write data

        i2c_sts_raw = ioread32(data->base + I2C_IRQSTATUS_RAW);
        i2c_sts_raw |= I2C_IRQSTATUS_RAW_XRDY; // Clear XRDY
        iowrite32(i2c_sts_raw, data->base + I2C_IRQSTATUS_RAW);

    }

    while ((ioread32(data->base + I2C_IRQSTATUS_RAW)&(I2C_IRQSTATUS_RAW_ARDY)) != I2C_IRQSTATUS_RAW_ARDY);  // Wait for ARDY (Access ready)

    i2c_con = ioread32(data->base + i2c_con);
    i2c_con |= 0x2; // Stop condition
    iowrite32(i2c_con, data->base + I2C_CON);

    i2c_sts_raw = ioread32(data->base + I2C_IRQSTATUS_RAW);
    i2c_sts_raw |= I2C_IRQSTATUS_RAW_ARDY; // Clear ARDY
    iowrite32(i2c_sts_raw, data->base + I2C_IRQSTATUS_RAW);

    if ((ioread32(data->base + I2C_IRQSTATUS_RAW)&(I2C_IRQSTATUS_RAW_NACK)) == I2C_IRQSTATUS_RAW_NACK){
        return -1;  // Error
    }
    return 0;  // Success
}

static ssize_t i2c1_master_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct i2c_device_data *data = filp->private_data;
    u8 *tx_buf;
    int i;
    u32 ch0cfg;
    unsigned long timeout;

    // Allocate buffer for TX data
    tx_buf = kmalloc(count, GFP_KERNEL);
    if (!tx_buf)
        return -ENOMEM;

    if (copy_from_user(tx_buf, buf, count)) {
        kfree(tx_buf);
        return -EFAULT;
    }

    dev_info(data->dev, "Writing %zu bytes: %*ph\n", count, (int)count, tx_buf);

    i2c1_write(data, SLAVE_ADDRESS, tx_buf, count);

    kfree(tx_buf);
    return count;
}

static const struct file_operations i2c_device_fops = {
    .owner = THIS_MODULE,
    .open = i2c1_master_open,
    .write = i2c1_master_write,
};

static int i2c1_probe(struct platform_device *pdev)
{
    struct i2c_device_data *data;
    int ret;

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->base = ioremap(I2C1_BASE, 0x10000);
    data->dev = &pdev->dev;

    /* Clock setup (assuming this part is unchanged) */
    data->clk = devm_clk_get(&pdev->dev, "fck-i2c1");
    if (IS_ERR(data->clk)) {
        dev_err(&pdev->dev, "Failed to get clock: %ld\n", PTR_ERR(data->clk));
        return PTR_ERR(data->clk);
    }
    ret = clk_prepare_enable(data->clk);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable clock: %d\n", ret);
        return ret;
    }
    dev_info(&pdev->dev, "I2c1 clock rate: %lu Hz\n", clk_get_rate(data->clk));

    // Initialize hardware
    i2c1_master_init(data);

    // Create character device
    ret = alloc_chrdev_region(&data->dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to allocate chrdev region: %d\n", ret);
        return ret;
    }

    cdev_init(&data->cdev, &i2c_device_fops);
    data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&data->cdev, data->dev_num, 1);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to add cdev: %d\n", ret);
        //unregister_chrdev_region(&data->dev_num, 1);
        iounmap(data->base);
        return ret;
    }

    data->class = class_create(THIS_MODULE, "i2c1_class");
    if (IS_ERR(data->class)) {
        dev_err(&pdev->dev, "Failed to create class: %ld\n", PTR_ERR(data->class));
        cdev_del(&data->cdev);
        //unregister_chrdev_region(&data->dev_num, 1);
        iounmap(data->base);
        return PTR_ERR(data->class);
    }

    data->dev = device_create(data->class, &pdev->dev, data->dev_num, NULL, "i2c1");
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

static int i2c1_remove(struct platform_device *pdev)
{
    struct i2c_adapter *adap = platform_get_drvdata(pdev);
    i2c_del_adapter(adap);
    return 0;
}

static const struct of_device_id i2c_device_of_match[] = {
    { .compatible = "i2c1-based" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, i2c_device_of_match);

static struct platform_driver i2c_device_driver = {
    .probe = i2c1_probe,
    .remove = i2c1_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = i2c_device_of_match,
    },
};

module_platform_driver(i2c_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Custom I2C Device Driver for BeagleBone Black SPI0");
