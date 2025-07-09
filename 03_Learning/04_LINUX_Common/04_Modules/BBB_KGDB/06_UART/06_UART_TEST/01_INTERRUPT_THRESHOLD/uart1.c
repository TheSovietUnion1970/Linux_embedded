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
#define UART_EFR 0x08 /* Interrupt Identification Register */

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

#define UART_TLR 0x1C /* TLR */
#define UART_SCR 0x40 /* SCR */
#define UART_UASR 0x38 /* UASR */
#define UART_EFR 0x08 /* EFR */
#define UART_MCR 0x10 /* MCR */

#define IIR_MASK 0x3E
#define IIR_PENDING 0x01

#define UART_RXFIFO_LVL 0x64
#define UART_TXFIFO_LVL 0x68

#define MAX_NUM_INTERRUPTS 1000
#define UART_BUFFER_SIZE 256

#define TX_THRESHOLD_VAL 35 /*******************************************/

/* Device structure */
struct bbb_uart {
    void __iomem *base;     /* UART registers */
    void __iomem *sysc;     /* System control register */
    void __iomem *syss;     /* System status register */
    dev_t devno;
    struct cdev cdev;
    struct device *dev;
    struct clk *clk;

    int irq;
    unsigned int count;
    unsigned int count_loop;
    unsigned long irqFlags;

    unsigned int Tx_Rx_count;
    char RX_buffer[256];

    struct class *class;    /* Class for device */
    int flag[256];
    char *kbuf;
    int gg;

    u32 tlv;
    u32 rlv;

    u8 Cnt;
};

/* Updated irqHandler */
static irqreturn_t irqHandler(int irq, void *d)
{
    struct bbb_uart *uart = d;
    uart->count++;
    int x;
    u32 iir = ioread32(uart->base + UART_IIR);
    uart->tlv = ioread32(uart->base + UART_TXFIFO_LVL);
    uart->rlv = ioread32(uart->base + UART_RXFIFO_LVL);

    printk("iir = 0x%x\n", iir);

    /* Interrupt pending */
    if ((iir&IIR_PENDING) == 0x0){
        /* RHR timeout interrupt */
        if ((iir&IIR_MASK) == 0xC) uart->RX_buffer[uart->Tx_Rx_count++] = ioread32(uart->base + UART_RHR);
        /* RHR interrupt */
        else if ((iir&IIR_MASK) == 0x4) {
            printk("RX -> iir = 0x%x, rxlv = %d, txlv = %d, rx count = %d\n", iir, uart->rlv, uart->tlv, uart->Tx_Rx_count);
            uart->RX_buffer[uart->Tx_Rx_count++] = ioread32(uart->base + UART_RHR);
        }
        /* THR interrupt */
        else if ((iir&IIR_MASK) == 0x2){
            iowrite32(0x40, uart->base + UART_THR);
            printk("TX -> iir = 0x%x, rxlv = %d, txlv = %d, uart->Cnt = %x\n", iir, uart->rlv, uart->tlv, uart->Cnt);

            // tracking for testing
            uart->Cnt++;
        }
    }

    if (uart->gg == 0){
        if (uart->tlv == 64 - TX_THRESHOLD_VAL) {
            iowrite32(0x0, uart->base + UART_IER); // disable TX RX interrupt
            iowrite32(0x0, uart->base + UART_MDR1); // enable uart
            uart->gg++;
        }
    }



    if (uart->count > MAX_NUM_INTERRUPTS) {
        printk("Too many interrupts - IIR=0x%x, LSR=0x%x\n", 
               ioread32(uart->base + UART_IIR), ioread32(uart->base + UART_LSR));
        iowrite32(0x0, uart->base + UART_IER);
        uart->count = 0;
    }
    uart->count_loop = 0;

    return IRQ_HANDLED;
}
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
    uart->tlv = ioread32(uart->base + UART_TXFIFO_LVL);

    printk("tlv = %d, rlv = %d\n", uart->tlv, ioread32(uart->base + UART_RXFIFO_LVL));

    if (uart->tlv == 0) {
        uart->Cnt = 0;
        iowrite32(0x7, uart->base + UART_MDR1); // disable uart
        iowrite32(0x3, uart->base + UART_IER); // enable TX RX interrupt
    }

    return 1;
}

static ssize_t bbb_uart_read(struct file *filp, char __user *buf,
                             size_t count, loff_t *ppos)
{
    struct bbb_uart *uart = filp->private_data;

    // copy_to_user will print the buf in the terminal
    if (copy_to_user(buf, uart->RX_buffer, uart->Tx_Rx_count)) {
        return -EFAULT;
    }

    printk("RX count = %d, left in RX FIFO = %d\n\n", uart->Tx_Rx_count, ioread32(uart->base + UART_RXFIFO_LVL));

    /* Save how many bytes we are returning */
    ssize_t bytes_read = uart->Tx_Rx_count;

    /* Reset buffer and counters */
    memset(uart->RX_buffer, 0, sizeof(uart->RX_buffer));
    uart->Tx_Rx_count = 0;

    // return correct bytes_read to avoid the next automatic read operation
    return 0;
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
/*
LCR(0xBF) -> DLL/DLH and EFR
EFR(1<<4) -> LCR(0x03 = normal operation) -> FCR[5:4] TX
EFR(0<<4) -> LCR(0x03 = normal operation) -> FCR[7:6] RX

EFR(1<<4) -> MCR[6] -> TCR/TLR



LCR != 0xBF and LCR[7] = 1 => Config mode A
LCR = 0xBF and LCR[7] = 1 => Config mode B
LCR[7] = 0 => operational mode 
*/
    u32 val;

    /* 1. Soft reset UART */
    iowrite32(0x2, uart->sysc);  // SYSC soft reset
    do {
        val = ioread32(uart->syss);
    } while (!(val & 0x1));  // SYSS reset done
    dev_info(uart->dev, "Soft reset completed\n");

    /* 2. Disable UART before config (set MDR1 = 0x7) */
    iowrite32(0x7, uart->base + UART_MDR1);

    /* 3. LCR = 0xBF  and LCR[7] = 1 => Config mode B ========================*/
    iowrite32(0xBF, uart->base + UART_LCR);
	
    iowrite32(0x10, uart->base + UART_EFR);  // EFR[4] = 1 => Enhanced features (access MCR[6])
	
    iowrite32(26, uart->base + UART_DLL); // Set baud rate (115200) — assuming 48MHz clock: divisor = 26
    iowrite32(0, uart->base + UART_DLH);
	
    /* 4. Set 8N1 format (LCR = 0x03) => Operational mode ========================*/
    iowrite32(0x03, uart->base + UART_LCR);
	
	iowrite32(1 << 6, uart->base + UART_MCR); // Set MCR[6] = 1 (TCR/TLR enable)
	
	// always accessable
	iowrite32((1 << 6) | 0x03 | (1 << 7), uart->base + UART_SCR);   // SCR[6] = 1 => granularity of TX = 1, SCR[2:1] = 1 -> DMA mode 1 (UARTnDMAREQ[0] in TX, UARTnDMAREQ[1] in RX)
														 // SCR[0] = 1 -> The DMAMODE is set with SCR[2:1]
                                                         // SCR[7] = 1 => granularity of RX = 1
	
	iowrite32(0x88, uart->base + UART_TLR);  // TLR[3:0] = 0 => TX trigger = 1000xx
                                             // TLR[7:4] = 0 => RX trigger = 1000xx
	iowrite32(0x37, uart->base + UART_FCR);  // FCR[5:4] = 3 => TX trigger = xxxx11, 
                                             // FCR[7:6] = 1 => RX trigger = xxxx00, 
                                             // FCR[2:0] = 7 -> enable FIFO, clear FIFOs
	
	/* TLR[3:0] + FCR[5:4] = 1000 11 -> TX threshold trigger is 35 bytes (<=29) */
    /* TLR[7:4] + FCR[7:6] = 1000 00 -> RX threshold trigger is 32 bytes */

    /* 5. Set MDR1 = 0x00 => 16x UART mode (enable UART) */
    iowrite32(0x07, uart->base + UART_MDR1);
	
    /* 6. Enable RHR interrupt (optional if you're using RX IRQs) */
    iowrite32(UART_IER_RHR_IT | 1 << 1, uart->base + UART_IER);	
	
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

    uart->count = 0;
    uart->Cnt = 0;
    uart->gg = 0;

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


    uart->irq = platform_get_irq(pdev, 0);
	if (uart->irq < 0) {
		dev_err(&pdev->dev, "%s: unable to get IRQ\n", __func__);
		return uart->irq;
	}

    ret = devm_request_irq(&pdev->dev, uart->irq, irqHandler, 0, "bbb-uart1", uart);
    if (ret < 0) 
    {
        dev_err(&pdev->dev, "%s: unable to request IRQ %d (%d)\n", __func__, uart->irq, ret);
        return ret;
    }



    /* Set offsets for control registers */
    uart->sysc = uart->base + 0x54;  // SYSC register at offset 0x54
    uart->syss = uart->base + 0x58;  // SYSS register at offset 0x58

    /* Clock setup (assuming this part is unchanged) */
    uart->clk = devm_clk_get(&pdev->dev, "fck_uart1");
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

    ret = alloc_chrdev_region(&uart->devno, 0, 1, "bbb_uart1");
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

    /* ===================== Create a charecter device ======================== */
    /* Create device class */
    uart->class = class_create(THIS_MODULE, "bbb_uart1_class");
    if (IS_ERR(uart->class)) {
        dev_err(&pdev->dev, "Failed to create class: %ld\n", PTR_ERR(uart->class));
        cdev_del(&uart->cdev);
        unregister_chrdev_region(uart->devno, 1);
        pm_runtime_put_sync(&pdev->dev);
        clk_disable_unprepare(uart->clk);
        return PTR_ERR(uart->class);
    }

    /* Create device node /dev/uart1 */
    uart->dev = device_create(uart->class, &pdev->dev, uart->devno, NULL, "uart1");
    if (IS_ERR(uart->dev)) {
        dev_err(&pdev->dev, "Failed to create device: %ld\n", PTR_ERR(uart->dev));
        class_destroy(uart->class);
        cdev_del(&uart->cdev);
        unregister_chrdev_region(uart->devno, 1);
        pm_runtime_put_sync(&pdev->dev);
        clk_disable_unprepare(uart->clk);
        return PTR_ERR(uart->dev);
    }
    dev_info(&pdev->dev, "Created device /dev/uart1\n");


    bbb_uart_init_hw(uart);  // Ensure this uses uart->sysc and uart->syss correctly
    dev_info(&pdev->dev, "Done - bbb_uart_init_hw\n");

    uart->dev = &pdev->dev;
    platform_set_drvdata(pdev, uart);

    dev_info(&pdev->dev, "Uart1 initialized\n");

    dev_info(uart->dev, "LCR=0x%x, FCR=0x%x, MDR1=0x%x, MCR=0x%x, DLL = 0x%x\n",
            ioread32(uart->base + UART_LCR),
            ioread32(uart->base + UART_FCR),
            ioread32(uart->base + UART_MDR1),
            ioread32(uart->base + UART_MCR),
            ioread32(uart->base + UART_DLL));
    dev_info(uart->dev, "Clock rate: %lu Hz\n", clk_get_rate(uart->clk));

    return 0;
}
/* Remove function */
static int bbb_uart_remove(struct platform_device *pdev)
{
    struct bbb_uart *uart = platform_get_drvdata(pdev);

    device_destroy(uart->class, uart->devno);
    class_destroy(uart->class);

    cdev_del(&uart->cdev);
    unregister_chrdev_region(uart->devno, 1);
    pm_runtime_put_sync(&pdev->dev);
    pm_runtime_disable(&pdev->dev);
    clk_disable_unprepare(uart->clk);
    dev_info(&pdev->dev, "Uart1 removed\n");
    return 0;
}

/* Device tree match table */
static const struct of_device_id bbb_uart_of_match[] = {
    { .compatible = "uart1-based" },
    { }
};
MODULE_DEVICE_TABLE(of, bbb_uart_of_match);

/* Platform driver */
static struct platform_driver bbb_uart1 = {
    .probe = bbb_uart_probe,
    .remove = bbb_uart_remove,
    .driver = {
        .name = "bbb_uart1",
        .of_match_table = bbb_uart_of_match,
    },
};
module_platform_driver(bbb_uart1);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Grok");
MODULE_DESCRIPTION("BeagleBone Black Uart1 Driver");
