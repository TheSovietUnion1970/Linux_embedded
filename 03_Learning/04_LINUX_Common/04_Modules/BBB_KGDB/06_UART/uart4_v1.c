#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/ctype.h> /* Add this for isprint */

/* AM335x UART register offsets */
#define UART_THR 0x00 /* Transmit Holding Register */
#define UART_RHR 0x00 /* Receive Holding Register */
#define UART_LSR 0x14 /* Line Status Register */
#define UART_IER 0x04 /* Interrupt Enable Register */
#define UART_FCR 0x08 /* FIFO Control Register */
#define UART_LCR 0x0C /* Line Control Register */
#define UART_MCR 0x10 /* Modem Control Register */
#define UART_DLL 0x00 /* Divisor Latch Low */
#define UART_DLH 0x04 /* Divisor Latch High */
#define UART_SYSC 0x54 /* System Configuration Register */
#define UART_SYSS 0x58 /* System Status Register */
#define UART_MDR1 0x20 /* Mode Definition Register 1 */

/* Line Status Register bits */
#define UART_LSR_TXFIFOE (1 << 5) /* Transmit FIFO empty */
#define UART_LSR_DR      (1 << 0) /* Data Ready (Receive FIFO has data) */

/* Device structure */
struct bbb_uart {
    void __iomem *base;     /* UART registers */
    void __iomem *sysc;     /* System control register */
    void __iomem *syss;     /* System status register */
    dev_t devno;
    struct cdev cdev;
    struct device *dev;
    struct clk *clk;
};

/* Device tree match table */
static const struct of_device_id bbb_uart_of_match[] = {
    { .compatible = "uart4-based" },
    { }
};
MODULE_DEVICE_TABLE(of, bbb_uart_of_match);

/* File operations */
static int bbb_uart_open(struct inode *inode, struct file *filp)
{
    struct bbb_uart *uart = container_of(inode->i_cdev, struct bbb_uart, cdev);
    filp->private_data = uart;
    return 0;
}

static int bbb_uart_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t bbb_uart_write(struct file *filp, const char __user *buf,
                              size_t count, loff_t *ppos)
{
    struct bbb_uart *uart = filp->private_data;
    char *kbuf;
    int i;

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, buf, count)) {
        kfree(kbuf);
        return -EFAULT;
    }
    printk("TX - count = %d\n", count);

    for (i = 0; i < count; i++) {
        unsigned long timeout = jiffies + msecs_to_jiffies(1000);
        u32 lsr;
        while (1) {
            lsr = ioread32(uart->base + UART_LSR);
            dev_info(uart->dev, "Write: LSR=0x%x\n", lsr);  // Log LSR value
            if (lsr & UART_LSR_TXFIFOE)
                break;
            if (time_after(jiffies, timeout)) {
                dev_err(uart->dev, "TX timeout, LSR=0x%x\n", lsr);
                kfree(kbuf);
                return -ETIMEDOUT;
            }
            cpu_relax();
        }
        iowrite32(kbuf[i], uart->base + UART_THR);
    }

    kfree(kbuf);
    return count;
}

static ssize_t bbb_uart_read(struct file *filp, char __user *buf,
                             size_t count, loff_t *ppos)
{
    struct bbb_uart *uart = filp->private_data;
    char *kbuf;
    int i;
    unsigned long timeout;

    dev_info(uart->dev, "RX - count = %zu\n", count);

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    for (i = 0; i < count; i++) {
        timeout = jiffies + msecs_to_jiffies(100);
        while (!(ioread32(uart->base + UART_LSR) & UART_LSR_DR)) {
            u32 lsr = ioread32(uart->base + UART_LSR);
            dev_info(uart->dev, "Read: LSR=0x%x\n", lsr);
            if (lsr & UART_LSR_DR) {
                /* Data became available just after checking */
                break;
            }
            if (time_after(jiffies, timeout)) {
                dev_info(uart->dev, "RX timeout, LSR=0x%x, read %d bytes\n",
                         lsr, i);
                if (i > 0) {
                    /* Return partial read if some data was read */
                    if (copy_to_user(buf, kbuf, i)) {
                        kfree(kbuf);
                        return -EFAULT;
                    }
                    kfree(kbuf);
                    return i;
                }
                kfree(kbuf);
                return 0; /* No data available */
            }
            cpu_relax();
        }
        kbuf[i] = ioread32(uart->base + UART_RHR) & 0xFF;
        dev_info(uart->dev, "Read: LSR=0x%x, byte=%c\n",
                 ioread32(uart->base + UART_LSR),
                 isprint(kbuf[i]) ? kbuf[i] : '.');
    }

    if (copy_to_user(buf, kbuf, count)) {
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    return count;
}

static const struct file_operations bbb_uart_fops = {
    .owner = THIS_MODULE,
    .open = bbb_uart_open,
    .release = bbb_uart_release,
    .write = bbb_uart_write,
    .read = bbb_uart_read,
};

static void bbb_uart_init_hw(struct bbb_uart *uart)
{
    u32 val;

    /* Reset the UART via SYSC register */
    iowrite32(0x2, uart->sysc);  // Example: soft reset
    printk("Before while, val = %du\n", ioread32(uart->syss));
    do {
        val = ioread32(uart->syss);
    } while (!(val & 0x1));  // Wait for reset completion
    printk("After while\n");
    dev_info(uart->dev, "Soft reset completed\n");

    /* 2. Disable UART */
    iowrite32(0x7, uart->base + UART_MDR1);  /* Disable UART */

    /* 3. Configure baud rate (e.g., 115200 with 48MHz clock) */
    iowrite32(0xBF, uart->base + UART_LCR);  /* Access DLL/DLH */
    iowrite32(26 & 0xFF, uart->base + UART_DLL);  /* 115200 baud */
    iowrite32(0, uart->base + UART_DLH);
    iowrite32(0x03, uart->base + UART_LCR);  /* 8N1 */

    /* 4. Enable and configure FIFOs */
    iowrite32(0x07, uart->base + UART_FCR);  /* Enable FIFO, clear TX/RX */

    /* 5. Enable UART */
    iowrite32(0x0, uart->base + UART_MDR1);  /* UART 16x mode */

    usleep_range(1000, 2000);  /* 1ms */

    /* 6. Enable loopback */
    iowrite32(0x10, uart->base + UART_MCR);  /* Set loopback */
}

static int bbb_uart_probe(struct platform_device *pdev)
{
    struct bbb_uart *uart;
    struct resource *res;
    int ret;

    dev_info(&pdev->dev, "Probing device: %s, Node: %s\n", 
             pdev->name, pdev->dev.of_node->full_name);

    uart = devm_kzalloc(&pdev->dev, sizeof(*uart), GFP_KERNEL);
    if (!uart)
        return -ENOMEM;

    /* Get the single memory resource */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "No memory resource\n");
        return -ENODEV;
    }
    dev_info(&pdev->dev, "Memory resource start: 0x%x, end: 0x%x, flags: 0x%lx\n",
             res->start, res->end, res->flags);

    /* Map the entire region */
    uart->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(uart->base)) {
        dev_err(&pdev->dev, "Failed to map memory resource: %ld\n", 
                PTR_ERR(uart->base));
        return PTR_ERR(uart->base);
    }

    /* Set offsets for control registers */
    uart->sysc = uart->base + 0x54;  // SYSC register at offset 0x54
    uart->syss = uart->base + 0x58;  // SYSS register at offset 0x58

    /* Clock setup (assuming this part is unchanged) */
    uart->clk = devm_clk_get(&pdev->dev, "fck");
    if (IS_ERR(uart->clk)) {
        dev_err(&pdev->dev, "Failed to get clock: %ld\n", PTR_ERR(uart->clk));
        return PTR_ERR(uart->clk);
    }
    ret = clk_prepare_enable(uart->clk);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable clock: %d\n", ret);
        return ret;
    }

    /* Remaining initialization (e.g., PM, chrdev) */
    pm_runtime_enable(&pdev->dev);
    ret = pm_runtime_get_sync(&pdev->dev);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to enable PM: %d\n", ret);
        clk_disable_unprepare(uart->clk);
        return ret;
    }

    ret = alloc_chrdev_region(&uart->devno, 0, 1, "bbb_uart4");
    if (ret) {
        dev_err(&pdev->dev, "Failed to allocate chrdev: %d\n", ret);
        pm_runtime_put_sync(&pdev->dev);
        clk_disable_unprepare(uart->clk);
        return ret;
    }
    dev_info(&pdev->dev, "Allocated chrdev major: %d\n", MAJOR(uart->devno));

    cdev_init(&uart->cdev, &bbb_uart_fops);
    uart->cdev.owner = THIS_MODULE;
    ret = cdev_add(&uart->cdev, uart->devno, 1);
    if (ret) {
        dev_err(&pdev->dev, "Failed to add cdev: %d\n", ret);
        unregister_chrdev_region(uart->devno, 1);
        pm_runtime_put_sync(&pdev->dev);
        clk_disable_unprepare(uart->clk);
        return ret;
    }
    dev_info(&pdev->dev, "Added cdev successfully\n");

    bbb_uart_init_hw(uart);  // Ensure this uses uart->sysc and uart->syss correctly
    dev_info(&pdev->dev, "Done - bbb_uart_init_hw\n");

    uart->dev = &pdev->dev;
    platform_set_drvdata(pdev, uart);

    dev_info(&pdev->dev, "UART4 initialized\n");

    dev_info(uart->dev, "LCR=0x%x, FCR=0x%x, MDR1=0x%x, MCR=0x%x\n",
            ioread32(uart->base + UART_LCR),
            ioread32(uart->base + UART_FCR),
            ioread32(uart->base + UART_MDR1),
            ioread32(uart->base + UART_MCR));
    dev_info(uart->dev, "Clock rate: %lu Hz\n", clk_get_rate(uart->clk));

    return 0;
}
/* Remove function */
static int bbb_uart_remove(struct platform_device *pdev)
{
    struct bbb_uart *uart = platform_get_drvdata(pdev);

    cdev_del(&uart->cdev);
    unregister_chrdev_region(uart->devno, 1);
    pm_runtime_put_sync(&pdev->dev);
    pm_runtime_disable(&pdev->dev);
    clk_disable_unprepare(uart->clk);
    dev_info(&pdev->dev, "UART4 removed\n");
    return 0;
}

/* Platform driver */
static struct platform_driver bbb_uart_driver = {
    .probe = bbb_uart_probe,
    .remove = bbb_uart_remove,
    .driver = {
        .name = "bbb-uart4",
        .of_match_table = bbb_uart_of_match,
    },
};

/* Module init and exit */
static int __init bbb_uart_init(void)
{
    return platform_driver_register(&bbb_uart_driver);
}

static void __exit bbb_uart_exit(void)
{
    platform_driver_unregister(&bbb_uart_driver);
}

module_init(bbb_uart_init);
module_exit(bbb_uart_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Grok");
MODULE_DESCRIPTION("BeagleBone Black UART4 Driver");
