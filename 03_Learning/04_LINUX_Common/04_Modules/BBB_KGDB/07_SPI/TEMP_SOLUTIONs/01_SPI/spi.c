#include <linux/module.h>
#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of_address.h>

#define DRIVER_NAME "spi0_device_driver"
#define DEVICE_NAME "spi0"
#define SLK 500000  // 500 kHz

// Register offsets for OMAP2 McSPI (from spi-omap2-mcspi.c)
#define OMAP2_MCSPI_CHCONF0   0x2c
#define OMAP2_MCSPI_CHSTAT0   0x30
#define OMAP2_MCSPI_CHCTRL0   0x34
#define OMAP2_MCSPI_TX0       0x38
#define OMAP2_MCSPI_RX0       0x3c
#define OMAP2_MCSPI_MODULCTRL 0x28

// Bitmasks for CHCONF0
#define OMAP2_MCSPI_CHCONF_POL    BIT(1)  // Clock polarity
#define OMAP2_MCSPI_CHCONF_PHA    BIT(0)  // Clock phase
#define OMAP2_MCSPI_CHCONF_WL_MASK (0x1f << 7)  // Word length mask
#define OMAP2_MCSPI_CHCONF_DPE0   BIT(16) // Data pin 0 enable (MOSI)
#define OMAP2_MCSPI_CHCONF_IS     BIT(18) // Input select (MISO)
#define OMAP2_MCSPI_CHCONF_FORCE  BIT(20) // Force SPI_EN (CS)

// Bitmasks for CHSTAT0
#define OMAP2_MCSPI_CHSTAT_TXS    BIT(1)  // TX register empty
#define OMAP2_MCSPI_CHSTAT_RXS    BIT(0)  // RX register full

// Bitmasks for CHCTRL0
#define OMAP2_MCSPI_CHCTRL_EN     BIT(0)  // Channel enable

// Bitmasks for MODULCTRL
#define OMAP2_MCSPI_MODULCTRL_SINGLE BIT(0) // Single-channel mode

struct spi_device_data {
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;
    void __iomem *base;  // Mapped base address of SPI0 registers
};

static int spi_device_open(struct inode *inode, struct file *file)
{
    struct spi_device_data *data = container_of(inode->i_cdev, struct spi_device_data, cdev);
    file->private_data = data;
    return 0;
}

static ssize_t spi_device_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct spi_device_data *data = filp->private_data;
    void __iomem *base = data->base;
    u8 *tx_buf;
    int i;

    // Allocate buffer for TX data
    tx_buf = kmalloc(count, GFP_KERNEL);
    if (!tx_buf)
        return -ENOMEM;

    if (copy_from_user(tx_buf, buf, count)) {
        kfree(tx_buf);
        return -EFAULT;
    }

    dev_info(data->dev, "Writing %zu bytes: %*ph\n", count, (int)count, tx_buf);

    // Enable the SPI channel
    writel_relaxed(OMAP2_MCSPI_CHCTRL_EN, base + OMAP2_MCSPI_CHCTRL0);

    // Perform the transfer
    for (i = 0; i < count; i++) {
        // Wait for TX register to be empty
        while (!(readl_relaxed(base + OMAP2_MCSPI_CHSTAT0) & OMAP2_MCSPI_CHSTAT_TXS))
            cpu_relax();

        // Write data to TX register
        writel_relaxed(tx_buf[i], base + OMAP2_MCSPI_TX0);

        // Wait for RX register to be full (for full-duplex, even if not reading)
        while (!(readl_relaxed(base + OMAP2_MCSPI_CHSTAT0) & OMAP2_MCSPI_CHSTAT_RXS))
            cpu_relax();

        // Optionally read RX data (discard here since it's a write-only operation)
        readl_relaxed(base + OMAP2_MCSPI_RX0);
    }

    // Disable the channel
    writel_relaxed(0, base + OMAP2_MCSPI_CHCTRL0);

    kfree(tx_buf);
    return count;
}

static const struct file_operations spi_device_fops = {
    .owner = THIS_MODULE,
    .open = spi_device_open,
    .write = spi_device_write,
};

static int spi_device_probe(struct platform_device *pdev)
{
    struct spi_device_data *data;
    struct resource res;
    int ret;
    u32 chconf = 0;

    dev_info(&pdev->dev, "SPI device probed successfully\n");

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    platform_set_drvdata(pdev, data);

    // Map the SPI controller registers from device tree
    if (of_address_to_resource(pdev->dev.of_node, 0, &res)) {
        dev_err(&pdev->dev, "Failed to get resource\n");
        return -EINVAL;
    }
    data->base = devm_ioremap_resource(&pdev->dev, &res);
    if (IS_ERR(data->base)) {
        dev_err(&pdev->dev, "Failed to map resource\n");
        return PTR_ERR(data->base);
    }
    data->dev = &pdev->dev;

    // Configure SPI controller (single-channel mode)
    writel_relaxed(OMAP2_MCSPI_MODULCTRL_SINGLE, data->base + OMAP2_MCSPI_MODULCTRL);

    // Configure CHCONF0 for SPI0
    // Word length: 8 bits
    chconf |= (8 - 1) << 7;
    // SPI Mode 0 (CPOL=0, CPHA=0)
    chconf &= ~OMAP2_MCSPI_CHCONF_POL;
    chconf &= ~OMAP2_MCSPI_CHCONF_PHA;
    // Pin direction: D0 in (MISO), D1 out (MOSI) as per device tree
    chconf |= OMAP2_MCSPI_CHCONF_IS;    // D0 as input
    chconf &= ~OMAP2_MCSPI_CHCONF_DPE0; // D1 as output
    // CS active low (default)
    chconf |= OMAP2_MCSPI_CHCONF_FORCE; // Manual CS control
    writel_relaxed(chconf, data->base + OMAP2_MCSPI_CHCONF0);

    // Create character device
    ret = alloc_chrdev_region(&data->dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to allocate chrdev region: %d\n", ret);
        return ret;
    }

    cdev_init(&data->cdev, &spi_device_fops);
    data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&data->cdev, data->dev_num, 1);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to add cdev: %d\n", ret);
        unregister_chrdev_region(data->dev_num, 1);
        return ret;
    }

    data->class = class_create(THIS_MODULE, "spi0_class");
    if (IS_ERR(data->class)) {
        dev_err(&pdev->dev, "Failed to create class: %ld\n", PTR_ERR(data->class));
        cdev_del(&data->cdev);
        unregister_chrdev_region(data->dev_num, 1);
        return PTR_ERR(data->class);
    }

    data->dev = device_create(data->class, &pdev->dev, data->dev_num, NULL, "spi0");
    if (IS_ERR(data->dev)) {
        dev_err(&pdev->dev, "Failed to create device: %ld\n", PTR_ERR(data->dev));
        class_destroy(data->class);
        cdev_del(&data->cdev);
        unregister_chrdev_region(data->dev_num, 1);
        return PTR_ERR(data->dev);
    }

    dev_info(&pdev->dev, "Created /dev/%s\n", DEVICE_NAME);
    return 0;
}

static int spi_device_remove(struct platform_device *pdev)
{
    struct spi_device_data *data = platform_get_drvdata(pdev);
    if (data->dev)
        device_destroy(data->class, data->dev_num);
    if (data->class)
        class_destroy(data->class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->dev_num, 1);
    dev_info(&pdev->dev, "SPI device removed\n");
    return 0;
}

static const struct of_device_id spi_device_of_match[] = {
    { .compatible = "spi0-based" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, spi_device_of_match);

static struct platform_driver spi_device_driver = {
    .probe = spi_device_probe,
    .remove = spi_device_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = spi_device_of_match,
    },
};

module_platform_driver(spi_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Custom SPI Device Driver for BeagleBone Black SPI0");