#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/wait.h>

#define DRIVER_NAME "spi1_device_driver"
#define DEVICE_NAME "spi1"
#define SLK 500000

#define SPI1_BASE 0x481a0000

// Register offsets for OMAP2 McSPI (AM33xx SPI1, base 0x481a0000)
#define MCSPI_SYSCONFIG   0x110
#define MCSPI_SYSSTATUS   0x114

// #define MCSPI_CHCONF1     0x140
// #define MCSPI_CHSTAT1     0x144
// #define MCSPI_CHCTRL1     0x148
// #define MCSPI_TX1         0x14c
// #define MCSPI_RX1         0x150

#define MCSPI_CHCONF0     0x12c
#define MCSPI_CHSTAT0     0x130
#define MCSPI_CHCTRL0     0x134
#define MCSPI_TX0         0x138
#define MCSPI_RX0         0x13c

#define MCSPI_MODULCTRL   0x128
#define MCSPI_IRQSTATUS   0x118
#define MCSPI_IRQENABLE   0x11c

#define MCSPI_DAFRX       0x1a0
#define MCSPI_XFERLEVEL   0x17c

// Bitmasks for CHCONF1
#define MCSPI_CHCONF_POL      BIT(1)  // Clock polarity
#define MCSPI_CHCONF_PHA      BIT(0)  // Clock phase
#define MCSPI_CHCONF_WL_MASK  (0x1f << 7)  // Word length mask
#define MCSPI_CHCONF_TRM_RX_ONLY BIT(12)  // Receive-only mode
#define MCSPI_CHCONF_DPE0     BIT(16) // Data pin 0 enable
#define MCSPI_CHCONF_DPE1     BIT(17) // Data pin 1 enable
#define MCSPI_CHCONF_IS       BIT(18) // Input select (MISO)
#define MCSPI_CHCONF_FORCE    BIT(20) // Force SPI_EN (CS)
#define MCSPI_CHCONF_EPOL     BIT(6)  // Chip select polarity

// Bitmasks for CHSTAT1
#define MCSPI_CHSTAT_EOT      BIT(2)  // End of frame
#define MCSPI_CHSTAT_TXS      BIT(1)  // TX register empty
#define MCSPI_CHSTAT_RXS      BIT(0)  // RX register full

// Bitmasks for CHCTRL1
#define MCSPI_CHCTRL_EN       BIT(0)  // Channel enable

// Bitmasks for MODULCTRL
#define MCSPI_MODULCTRL_MS    BIT(2)  // Master/Slave mode

// Bitmasks for IRQSTATUS and IRQENABLE
#define MCSPI_CHSTAT_RX1_FULL BIT(6)  // RX1 full interrupt
#define MCSPI_CHSTAT_RX0_FULL BIT(2)  // RX0 full interrupt
#define MCSPI_CHSTAT_RX0_OVERFLOW BIT(3)  // RX0 overflow interrupt

// Receive interval for logging (2 seconds)
#define SPI_RECV_INTERVAL (2 * HZ)

struct spi_device_data {
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;
    void __iomem *base;  // Mapped base address of SPI1 registers
    struct clk *clk;
    int irq;
    struct task_struct *recv_thread;
    wait_queue_head_t recv_wait;
    u8 rx_buffer[256];  // Buffer for received data
    size_t rx_count;    // Number of bytes received
    bool data_ready;    // Flag for new data

    u8 index;
};

u8 rx_data[256];
static irqreturn_t irqHandler(int irq, void *d)
{
    struct spi_device_data *data = d;
    u32 irqsts;
    u8 i;

    irqsts = ioread32(data->base + MCSPI_IRQSTATUS);
    //printk("irqsts = 0x%x, WCNT = %d\n", irqsts, ioread32(data->base + MCSPI_XFERLEVEL));

    if (irqsts & MCSPI_CHSTAT_RX0_FULL) {
        // Read data from RX1
        rx_data[data->index++] = ioread32(data->base + MCSPI_RX0);

        //dev_info(data->dev, "RX0-f data: '%c'\n", rx_data);
        // Clear RX1_FULL interrupt
        iowrite32(MCSPI_CHSTAT_RX0_FULL, data->base + MCSPI_IRQSTATUS);
    }
    else if (irqsts & MCSPI_CHSTAT_RX0_OVERFLOW) {
        // Read data from RX1
        rx_data[data->index++] = ioread32(data->base + MCSPI_RX0);

        //dev_info(data->dev, "RX0-o data: '%c'\n", rx_data);
        // Clear RX1_FULL interrupt
        iowrite32(MCSPI_CHSTAT_RX0_OVERFLOW, data->base + MCSPI_IRQSTATUS);      
    }

    if (ioread32(data->base + MCSPI_RX0) == '\0'){
        i = data->index;
        data->index = 0;

        printk("rx = '%s', i = %d\n", rx_data, i);


        memset(rx_data, 0x0, 256);
    }

    return IRQ_HANDLED;
}

static int spi_device_open(struct inode *inode, struct file *file)
{
    struct spi_device_data *data = container_of(inode->i_cdev, struct spi_device_data, cdev);
    file->private_data = data;
    return 0;
}

static ssize_t spi_device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct spi_device_data *data = filp->private_data;
    // size_t to_copy;

    // // Wait for data to be available
    // wait_event_interruptible(data->recv_wait, data->data_ready || kthread_should_stop());

    // // Copy received data to user space
    // to_copy = min(count, data->rx_count);
    // if (copy_to_user(buf, data->rx_buffer, to_copy)) {
    //     return -EFAULT;
    // }

    // // Shift remaining data in buffer
    // if (to_copy < data->rx_count) {
    //     memmove(data->rx_buffer, data->rx_buffer + to_copy, data->rx_count - to_copy);
    // }
    // data->rx_count -= to_copy;
    // if (data->rx_count == 0) {
    //     data->data_ready = false;
    // }

    // return to_copy;

    printk("channel status1 = 0x%x, RX1 = 0x%x, irqsts = 0x%x\n", ioread32(data->base + MCSPI_CHSTAT0), ioread32(data->base + MCSPI_RX0), ioread32(data->base + MCSPI_IRQSTATUS));
    printk("rx = '%s', i = %d\n", rx_data, data->index);

    return 0;
}

static const struct file_operations spi_device_fops = {
    .owner = THIS_MODULE,
    .open = spi_device_open,
    .read = spi_device_read,
};

static int spi_device_probe(struct platform_device *pdev)
{
    struct spi_device_data *data;
    int ret;
    u32 chconf = 0;

    dev_info(&pdev->dev, "SPI1 device probed successfully with compatible 'spi1-based'\n");

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    platform_set_drvdata(pdev, data);

    data->index = 0;

    // Map SPI controller registers
    data->base = ioremap(SPI1_BASE, 0x400);
    if (!data->base) {
        dev_err(&pdev->dev, "Failed to map resource\n");
        return -ENOMEM;
    }
    data->dev = &pdev->dev;

    // Clock setup
    data->clk = devm_clk_get(&pdev->dev, "fck-spi1");
    if (IS_ERR(data->clk)) {
        dev_err(&pdev->dev, "Failed to get clock: %ld\n", PTR_ERR(data->clk));
        iounmap(data->base);
        return PTR_ERR(data->clk);
    }
    ret = clk_prepare_enable(data->clk);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable clock: %d\n", ret);
        iounmap(data->base);
        return ret;
    }
    dev_info(&pdev->dev, "SPI1 clock rate: %lu Hz\n", clk_get_rate(data->clk));

    // Software reset
    iowrite32(BIT(1), data->base + MCSPI_SYSCONFIG);
    while (!(ioread32(data->base + MCSPI_SYSSTATUS) & BIT(0)))
        cpu_relax();

    // Configure SPI controller (slave mode)
    iowrite32(MCSPI_MODULCTRL_MS, data->base + MCSPI_MODULCTRL);

    // Configure CHCONF1 for SPI1 (CS1)
    chconf |= (8 - 1) << 7; // 8-bit word length
    chconf &= ~MCSPI_CHCONF_POL; // SPI Mode 0
    chconf &= ~MCSPI_CHCONF_PHA; // SPI Mode 0

    chconf &= ~MCSPI_CHCONF_IS; // D0 as input (MISO)
    chconf |= MCSPI_CHCONF_DPE0; // D0 not driven (MISO input)
    chconf &= ~MCSPI_CHCONF_DPE1; // D1 as output (not use here)

    chconf |= MCSPI_CHCONF_EPOL; // Active-low CS
    chconf |= MCSPI_CHCONF_TRM_RX_ONLY; // Receive-only mode

    chconf |= 1u << 28; // The FIFO buffer is used to receive data.

    iowrite32(chconf, data->base + MCSPI_CHCONF0);

    // Enable channel
    iowrite32(MCSPI_CHCTRL_EN, data->base + MCSPI_CHCTRL0);

    //iowrite32(3u << 8, data->base + MCSPI_XFERLEVEL); // interrupt at least 4 bytes

    // Enable RX0_FULL interrupt
    iowrite32(MCSPI_CHSTAT_RX0_FULL | MCSPI_CHSTAT_RX0_OVERFLOW, data->base + MCSPI_IRQENABLE);

    // Request IRQ (hwirq 125 for SPI1 on AM33xx)
    data->irq = platform_get_irq(pdev, 0);
    if (data->irq < 0) {
        dev_err(&pdev->dev, "Failed to get IRQ: %d\n", data->irq);
        clk_disable_unprepare(data->clk);
        iounmap(data->base);
        return data->irq;
    }
    printk("swirq of SPI1 = %d\n", data->irq);
    ret = devm_request_irq(&pdev->dev, data->irq, irqHandler, 0, "spi1", data);
    if (ret < 0) {
        dev_err(&pdev->dev, "Unable to request IRQ %d: %d\n", data->irq, ret);
        clk_disable_unprepare(data->clk);
        iounmap(data->base);
        return ret;
    }

    // Create character device
    ret = alloc_chrdev_region(&data->dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to allocate chrdev region: %d\n", ret);
        kthread_stop(data->recv_thread);
        clk_disable_unprepare(data->clk);
        iounmap(data->base);
        return ret;
    }

    cdev_init(&data->cdev, &spi_device_fops);
    data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&data->cdev, data->dev_num, 1);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to add cdev: %d\n", ret);
        unregister_chrdev_region(data->dev_num, 1);
        kthread_stop(data->recv_thread);
        clk_disable_unprepare(data->clk);
        iounmap(data->base);
        return ret;
    }

    data->class = class_create(THIS_MODULE, "spi1_class");
    if (IS_ERR(data->class)) {
        dev_err(&pdev->dev, "Failed to create class: %ld\n", PTR_ERR(data->class));
        cdev_del(&data->cdev);
        unregister_chrdev_region(data->dev_num, 1);
        kthread_stop(data->recv_thread);
        clk_disable_unprepare(data->clk);
        iounmap(data->base);
        return PTR_ERR(data->class);
    }

    data->dev = device_create(data->class, &pdev->dev, data->dev_num, NULL, "spi1");
    if (IS_ERR(data->dev)) {
        dev_err(&pdev->dev, "Failed to create device: %ld\n", PTR_ERR(data->dev));
        class_destroy(data->class);
        cdev_del(&data->cdev);
        unregister_chrdev_region(data->dev_num, 1);
        kthread_stop(data->recv_thread);
        clk_disable_unprepare(data->clk);
        iounmap(data->base);
        return PTR_ERR(data->dev);
    }

    dev_info(&pdev->dev, "Created /dev/%s\n", DEVICE_NAME);
    return 0;
}

static int spi_device_remove(struct platform_device *pdev)
{
    struct spi_device_data *data = platform_get_drvdata(pdev);

    // Stop receive thread
    if (data->recv_thread)
        kthread_stop(data->recv_thread);

    // Disable channel
    iowrite32(0, data->base + MCSPI_CHCTRL0);

    // Clean up character device
    if (data->dev)
        device_destroy(data->class, data->dev_num);
    if (data->class)
        class_destroy(data->class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->dev_num, 1);

    // Disable clock and unmap registers
    clk_disable_unprepare(data->clk);
    iounmap(data->base);

    dev_info(&pdev->dev, "SPI1 device removed\n");
    return 0;
}

static const struct of_device_id spi_device_of_match[] = {
    { .compatible = "spi1-based" },
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
MODULE_DESCRIPTION("Custom SPI Slave Device Driver for BeagleBone Black SPI1");
