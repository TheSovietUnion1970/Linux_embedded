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
#include <linux/interrupt.h>
#include <linux/of_irq.h>

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
#define UART_IIR 0x08 /* Interrupt Identification Register */

/* Line Status Register bits */
#define UART_LSR_TXFIFOE (1 << 5) /* Transmit FIFO empty */
#define UART_LSR_DR      (1 << 0) /* Data Ready (Receive FIFO has data) */

/* Interrupt Enable Register bits */
#define UART_IER_RHR_IT  (1 << 0) /* Received Data Available */

/* Interrupt Enable Register bits */
#define UART_IIR_IT_PENDING (1 << 0) /* Interrupt pending (0 = pending) */
#define UART_IIR_CTO_IT  (0x6 << 1) /* Character Timeout (priority 2) */
#define UART_IIR_RLS_IT  (0x3 << 1) /* Receiver Line Status (priority 1) */
#define UART_IIR_RHR_IT  (0x2 << 1) /* Received Data Available (priority 2) */
#define UART_IIR_THR_IT  (0x1 << 1) /* Transmitter Holding Register Empty (priority 3) */
#define UART_IIR_MSI     (0x0 << 1) /* Modem Status (priority 4) */

#define MAX_NUM_INTERUPTS 200
#define BUFFER_SIZE 256

// ===============================================================================================================

struct irq_data_t {

    void __iomem *base;     /* UART registers */
    void __iomem *sysc;     /* System control register */
    void __iomem *syss;     /* System status register */
    dev_t devno;
    struct cdev cdev;
    struct clk *clk;

    int uart4_irq;
    unsigned int count;
    unsigned long irqFlags;

    unsigned int Tx_Rx_count;
    char RX_buffer[256];

    struct class *class;    /* Class for device */

    int gpio_irq;
    struct work_struct re_request_work;
    
    struct device *dev;
};

static irqreturn_t gpio_handler(int irq, void *dev_id);

// ===================================== uart4 functions =====================
/* File operations */
static int bbb_uart_open(struct inode *inode, struct file *filp)
{
    struct irq_data_t *data;

    data = container_of(inode->i_cdev, struct irq_data_t, cdev);
    filp->private_data = data;
    return 0;
}

static int bbb_uart_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t bbb_uart_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    struct irq_data_t *data;
    char *kbuf;
    unsigned long timeout;
    u32 lsr;
    size_t i;
    int ret;

    data = filp->private_data;
    kbuf = NULL;

    /* Re-enable RHR interrupts */
    iowrite32(UART_IER_RHR_IT, data->base + UART_IER);

    data->count = 0;
    data->Tx_Rx_count = 0;

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    ret = copy_from_user(kbuf, buf, count);
    if (ret) {
        kfree(kbuf);
        return -EFAULT;
    }

    for (i = 0; i < count; i++) {
        timeout = jiffies + msecs_to_jiffies(1000);
        while (1) {
            lsr = ioread32(data->base + UART_LSR);
            if (lsr & UART_LSR_TXFIFOE)
                break;
            if (time_after(jiffies, timeout)) {
                dev_err(data->dev, "TX timeout, LSR=0x%x\n", lsr);
                kfree(kbuf);
                return -ETIMEDOUT;
            }
            cpu_relax();
        }
        iowrite32(kbuf[i], data->base + UART_THR);
    }

    kfree(kbuf);
    return count;
}

static ssize_t bbb_uart_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    struct irq_data_t *data;
    ssize_t bytes_read;
    int ret;

    data = filp->private_data;
    bytes_read = 0;

    if (data->Tx_Rx_count == 0)
        return 0;

    if (count > data->Tx_Rx_count)
        count = data->Tx_Rx_count;

    ret = copy_to_user(buf, data->RX_buffer, count);
    if (ret)
        return -EFAULT;

    bytes_read = count;
    data->Tx_Rx_count = 0;
    memset(data->RX_buffer, 0, BUFFER_SIZE);

    return bytes_read;
}

static const struct file_operations bbb_uart_fops = {
    .owner = THIS_MODULE,
    .open = bbb_uart_open,
    .release = bbb_uart_release,
    .write = bbb_uart_write,
    .read = bbb_uart_read,
};

static void bbb_uart_init_hw(struct irq_data_t *uart)
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
    iowrite32(0x00, uart->base + UART_MCR);  /* Not Set loopback */

    /* Enable RHR interrupts */
    iowrite32(UART_IER_RHR_IT, uart->base + UART_IER);
}

static int bbb_uart_probe(struct platform_device *pdev)
{
    struct irq_data_t *data;
    struct resource *res;
    int ret;

    dev_info(&pdev->dev, "Probing device: %s, Node: %s\n", 
             pdev->name, pdev->dev.of_node->full_name);

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->count = 0;

    /* Get the single memory resource */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "No memory resource\n");
        return -ENODEV;
    }
    dev_info(&pdev->dev, "Memory resource start: 0x%x, end: 0x%x, flags: 0x%lx\n",
             res->start, res->end, res->flags);

    /* Map the entire region */
    data->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(data->base)) {
        dev_err(&pdev->dev, "Failed to map memory resource: %ld\n", 
                PTR_ERR(data->base));
        return PTR_ERR(data->base);
    }


    // data->uart4_irq = platform_get_irq(pdev, 0);
    // // data->uart4_irq = of_irq_get(pdev->dev.of_node, 0); // Gets <&intc 45 0>
	// if (data->uart4_irq < 0) {
	// 	dev_err(&pdev->dev, "%s: unable to get IRQ\n", __func__);
	// 	return data->uart4_irq;
	// }
    // dev_info(&pdev->dev, "Get IRQ: %d\n", data->uart4_irq);

    // ret = devm_request_irq(&pdev->dev, data->uart4_irq, irqHandler, 0, "bbb-uart4", uart);
    // if (ret < 0) 
    // {
    //     dev_err(&pdev->dev, "%s: unable to request IRQ %d (%d)\n", __func__, data->uart4_irq, ret);
    //     return ret;
    // }



    /* Set offsets for control registers */
    data->sysc = data->base + 0x54;  // SYSC register at offset 0x54
    data->syss = data->base + 0x58;  // SYSS register at offset 0x58

    /* Clock setup (assuming this part is unchanged) */
    data->clk = devm_clk_get(&pdev->dev, "fck");
    if (IS_ERR(data->clk)) {
        dev_err(&pdev->dev, "Failed to get clock: %ld\n", PTR_ERR(data->clk));
        return PTR_ERR(data->clk);
    }
    ret = clk_prepare_enable(data->clk);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable clock: %d\n", ret);
        return ret;
    }

    /* Remaining initialization (e.g., PM, chrdev) */
    pm_runtime_enable(&pdev->dev);
    ret = pm_runtime_get_sync(&pdev->dev);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to enable PM: %d\n", ret);
        clk_disable_unprepare(data->clk);
        return ret;
    }

    ret = alloc_chrdev_region(&data->devno, 0, 1, "bbb_uart4");
    if (ret) {
        dev_err(&pdev->dev, "Failed to allocate chrdev: %d\n", ret);
        pm_runtime_put_sync(&pdev->dev);
        clk_disable_unprepare(data->clk);
        return ret;
    }
    dev_info(&pdev->dev, "Allocated chrdev major: %d\n", MAJOR(data->devno));

    cdev_init(&data->cdev, &bbb_uart_fops);
    data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&data->cdev, data->devno, 1);
    if (ret) {
        dev_err(&pdev->dev, "Failed to add cdev: %d\n", ret);
        unregister_chrdev_region(data->devno, 1);
        pm_runtime_put_sync(&pdev->dev);
        clk_disable_unprepare(data->clk);
        return ret;
    }
    dev_info(&pdev->dev, "Added cdev successfully\n");

    /* ===================== Create a charecter device ======================== */
    /* Create device class */
    data->class = class_create(THIS_MODULE, "bbb_uart4_class");
    if (IS_ERR(data->class)) {
        dev_err(&pdev->dev, "Failed to create class: %ld\n", PTR_ERR(data->class));
        cdev_del(&data->cdev);
        unregister_chrdev_region(data->devno, 1);
        pm_runtime_put_sync(&pdev->dev);
        clk_disable_unprepare(data->clk);
        return PTR_ERR(data->class);
    }

    /* Create device node /dev/uart4 */
    data->dev = device_create(data->class, &pdev->dev, data->devno, NULL, "uart4");
    if (IS_ERR(data->dev)) {
        dev_err(&pdev->dev, "Failed to create device: %ld\n", PTR_ERR(data->dev));
        class_destroy(data->class);
        cdev_del(&data->cdev);
        unregister_chrdev_region(data->devno, 1);
        pm_runtime_put_sync(&pdev->dev);
        clk_disable_unprepare(data->clk);
        return PTR_ERR(data->dev);
    }
    dev_info(&pdev->dev, "Created device /dev/uart4\n");


    bbb_uart_init_hw(data);  // Ensure this uses data->sysc and data->syss correctly
    dev_info(&pdev->dev, "Done - bbb_uart_init_hw\n");

    data->dev = &pdev->dev;
    platform_set_drvdata(pdev, data);

    dev_info(&pdev->dev, "UART4 initialized\n");

    dev_info(data->dev, "LCR=0x%x, FCR=0x%x, MDR1=0x%x, MCR=0x%x\n",
            ioread32(data->base + UART_LCR),
            ioread32(data->base + UART_FCR),
            ioread32(data->base + UART_MDR1),
            ioread32(data->base + UART_MCR));
    dev_info(data->dev, "Clock rate: %lu Hz\n", clk_get_rate(data->clk));

    return 0;
}

static int bbb_uart_remove(struct platform_device *pdev)
{
    struct irq_data_t *data = platform_get_drvdata(pdev);

    device_destroy(data->class, data->devno);
    class_destroy(data->class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->devno, 1);
    pm_runtime_put_sync(&pdev->dev);
    pm_runtime_disable(&pdev->dev);
    clk_disable_unprepare(data->clk);
    dev_info(&pdev->dev, "UART4 removed\n");
    return 0;
}

static void re_request_irq_work(struct work_struct *work)
{
    struct irq_data_t *data = container_of(work, struct irq_data_t, re_request_work);
    struct device *dev = data->dev;
    int ret;

    dev_info(dev, "Workqueue: Attempting to re-request UART4 IRQ %d\n", data->uart4_irq);
}

static irqreturn_t gpio_handler(int irq, void *dev_id)
{
    struct irq_data_t * data = dev_id;
    struct platform_device *pdev = container_of(data->dev, struct platform_device, dev);
    
    dev_info(data->dev, "IRQ triggered on P9_15 (gpio1_16)\n");
    printk("num_reources = %d\n", pdev->num_resources);
    printk("reource[0]->start = %u\n", pdev->resource[0].start);
    printk("reource[0]->flags = %lu\n", pdev->resource[0].flags);
    // Schedule work to re-request IRQ
    schedule_work(&data->re_request_work);

    return IRQ_HANDLED;
}

static irqreturn_t uart4_handler(int irq, void *dev_id)
{
    struct irq_data_t *dev = dev_id;
    dev->count++;

    // Read val in UART_RHR to clear IIR from 0xcc to 0xc1
    printk("%x\n", ioread32(dev->base + UART_IIR));
    dev->RX_buffer[dev->Tx_Rx_count++] = ioread32(dev->base + UART_RHR);
    printk("Character: '%c'\n", dev->RX_buffer[dev->Tx_Rx_count-1]);

    if (dev->count > MAX_NUM_INTERUPTS){
        printk("Too many interrupts - IIR=0x%x, LSR=0x%x\n", ioread32(dev->base + UART_IIR), ioread32(dev->base + UART_LSR));
        iowrite32(0x0, dev->base + UART_IER);
        dev->count = 0;
    }

    return IRQ_HANDLED;
}

static int irq_probe(struct platform_device *pdev)
{
    struct irq_data_t *data;
    struct device *dev = &pdev->dev;
    int ret;

    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->dev = dev;
    dev_set_drvdata(dev, data);

    // Initialize workqueue
    INIT_WORK(&data->re_request_work, re_request_irq_work);

    // =====================================

    // Get the IRQ number from the device tree
    data->gpio_irq = platform_get_irq(pdev, 1);
    // data->gpio_irq = of_irq_get(pdev->dev.of_node, 1); // Gets <&gpio1 16 GPIO_ACTIVE_HIGH>
    if (data->gpio_irq < 0) {
        dev_err(dev, "Failed to get GPIO IRQ: %d\n", data->gpio_irq);
        return data->gpio_irq;
    }
    dev_info(dev, "Get GPIO IRQ: %d\n", data->gpio_irq);

    // Request the IRQ
    ret = devm_request_irq(dev, data->gpio_irq, gpio_handler, IRQF_TRIGGER_RISING, "irq_driver", data);
    if (ret) {
        dev_err(dev, "Failed to request GPIO IRQ: %d\n", ret);
        return ret;
    }

    // =====================================

    data->uart4_irq = platform_get_irq(pdev, 0);
    // uart->irq = of_irq_get(pdev->dev.of_node, 0); // Gets <&intc 45 0>
	if (data->uart4_irq < 0) {
		dev_err(dev, "unable to get UART4 IRQ: %d\n", data->uart4_irq);
		return data->uart4_irq;
	}
    dev_info(dev, "Get UART4 IRQ: %d\n", data->uart4_irq);

    ret = devm_request_irq(dev, data->uart4_irq, uart4_handler, 0, "irq_driver", data);
    if (ret) {
        dev_err(dev, "Failed to request UART4 IRQ: %d\n", ret);
        return ret;
    }

    // =====================================

    dev_info(dev, "IRQ driver initialized for P9_15 (gpio1_16)\n");
    printk("=================\n");

    bbb_uart_probe(pdev);
    dev_info(dev, "UART4 IRQ driver initialized f\n");
    printk("=================\n");

    return 0;
}

static int irq_remove(struct platform_device *pdev)
{
    struct irq_data_t *data = dev_get_drvdata(&pdev->dev);
    struct device *dev = &pdev->dev;

    if (data->gpio_irq >= 0)
        devm_free_irq(&pdev->dev, data->gpio_irq, data);

    if (data->uart4_irq >= 0)
        devm_free_irq(&pdev->dev, data->uart4_irq, data);

    dev_info(dev, "Both IRQ drivers are destroyed\n");

    bbb_uart_remove(pdev);
    dev_info(dev, "UART4 drivers are destroyed\n");

    return 0;
}

static const struct of_device_id irq_of_match[] = {
    { .compatible = "uart4-based" },
    { }
};
MODULE_DEVICE_TABLE(of, irq_of_match);

static struct platform_driver irq_driver = {
    .probe  = irq_probe,
    .remove = irq_remove,
    .driver = {
        .name = "irq_driver",
        .of_match_table = irq_of_match,
    },
};
module_platform_driver(irq_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OpenAI + You");
MODULE_DESCRIPTION("IRQ-Only Driver for P9_15 (gpio1_16)");
