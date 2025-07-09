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
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

#define HW_TRIGGER 0
#define SW_TRIGGER 1

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
#define UART_TLR 0x1C /* TLR */

#define UART_SCR 0x40 /* SCR */
#define UART_UASR 0x38 /* UASR */
#define UART_EFR 0x08 /* EFR */
#define UART_MCR 0x10 /* MCR */

#define UART_RXFIFO_LVL 0x64
#define UART_TXFIFO_LVL 0x68


#define DMA_EER 0x1020
#define DMA_ER 0x1000
#define DMA_ECR 0x1008
#define DMA_EMCR 0x308
#define DMA_SECR 0x1040
#define DMA_EESR 0x1030
#define DMA_IPR 0x1068
#define DMA_ESR 0x1010

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

#define MAX_NUM_INTERRUPTS 50
#define UART_BUFFER_SIZE 256

#define EDMA_BASE 0x49000000
#define EDMA_DCHMAP 0x0100

#define API_USE 0
#define REG_USE 1

/* Device structure */
struct bbb_uart {
    void __iomem *base;     /* UART registers */
    void __iomem *sysc;     /* System control register */
    void __iomem *syss;     /* System status register */
    void __iomem *edma_base;     /* edma registers */

    void __iomem *uart_phy_base;     /* UART registers */

    dev_t devno;
    struct cdev cdev;
    struct device *dev;
    struct clk *clk;
    struct class *class;    /* Class for device */
    int irq;
    unsigned int count;
    unsigned long irqFlags;
    unsigned int Tx_Rx_count;
    unsigned int Count;
    char RX_buffer[UART_BUFFER_SIZE];

    /* DMA-related fields */
    struct dma_chan *tx_chan;
    struct dma_chan *rx_chan;
    struct completion tx_completion;
    struct completion rx_completion;
    bool use_dma; /* Flag to indicate if DMA is used */

    int count_loop;
};

static void bbb_uart_tx_dma_callback(void *data)
{
    struct bbb_uart *uart = data;
    printk("DMA callback => TX FIFO lv = %d\n", ioread32(uart->base + UART_TXFIFO_LVL));
    dev_info(uart->dev, "TX DMA callback called\n");
    complete(&uart->tx_completion);
    
    iowrite32(0x0, uart->base + UART_MDR1); // enable uart
    // iowrite32(0x1, uart->base + UART_IER); // disable TX RX interrupt /////////
}

static void bbb_uart_rx_dma_callback(void *data)
{
    struct bbb_uart *uart = data;
    complete(&uart->rx_completion);
    dev_info(uart->dev, "RX DMA callback called\n");
}

static irqreturn_t irqHandler(int irq, void *d)
{
    
    struct bbb_uart *uart = d;
    printk("IIR = 0x%x\n",ioread32(uart->base + UART_IIR));
    uart->count++;
    
    printk("Start enyrt loop\n");
    // Read all available bytes from the FIFO
    while (ioread32(uart->base + UART_LSR) & UART_LSR_DR) {
        if (uart->Tx_Rx_count < UART_BUFFER_SIZE) {
            uart->RX_buffer[uart->Tx_Rx_count++] = ioread32(uart->base + UART_RHR);
            printk("int IRQ -> '%c'\n", uart->RX_buffer[uart->Tx_Rx_count-1]);
        } else {
            dev_err(uart->dev, "RX buffer overflow\n");
            //break;
        }

        if (uart->count_loop > 1000000) {
            printk("Stuck at loop ISR - IIR=0x%x, LSR=0x%x\n", 
                ioread32(uart->base + UART_IIR), ioread32(uart->base + UART_LSR));
            iowrite32(0x0, uart->base + UART_IER);
            uart->count = 0;
        }
        uart->count_loop++;
    }

    printk("IIR = 0x%x\n",ioread32(uart->base + UART_IIR));

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

static void dump_edma_dchmap(struct bbb_uart *uart)
{
    void __iomem *edma_base = ioremap(EDMA_BASE, 0x10000);
    int ch_num;

    if (!edma_base) {
        dev_err(uart->dev, "Failed to map eDMA registers\n");
        return;
    }
    // if (uart->tx_chan) {
    //     dev_info(uart->dev, "TX DMA DCHMAP[%d]=0x%x\n",
    //              uart->tx_chan->chan_id+0,
    //              ioread32(edma_base + EDMA_DCHMAP + ((uart->tx_chan->chan_id+0) * 4)));
    // }
    // if (uart->rx_chan) {
    //     dev_info(uart->dev, "RX DMA DCHMAP[%d]=0x%x\n",
    //              uart->rx_chan->chan_id+0,
    //              ioread32(edma_base + EDMA_DCHMAP + ((uart->rx_chan->chan_id+0) * 4)));
    // }

    for (ch_num = 0; ch_num < 64; ch_num++){
        dev_info(uart->dev, "RX DMA DCHMAP[%d]=0x%x\n",
                 ch_num,
                 ioread32(edma_base + EDMA_DCHMAP + (ch_num * 4)));
    }

    iounmap(edma_base);
}

static void write_dma(struct bbb_uart *uart, const char *data, size_t count)
{
    void *kbuf;
    dma_addr_t dma_addr;
    void *test_buf;
    dma_addr_t test_addr;
    int ret;
    struct dma_async_tx_descriptor *tx_desc;
    dma_cookie_t cookie;
    uart->Count = count;

    kbuf = dma_alloc_coherent(uart->dev, count, &dma_addr, GFP_KERNEL | GFP_DMA);
    if (!kbuf) {
        dev_err(uart->dev, "Failed to allocate DMA-coherent buffer\n");
        return;
    }
    test_buf = dma_alloc_coherent(uart->dev, count, &test_addr, GFP_KERNEL | GFP_DMA);
    if (!kbuf) {
        dev_err(uart->dev, "Failed to allocate DMA-coherent buffer\n");
        return;
    }

    memcpy(kbuf, data, count);
    memset(test_buf, 0x40, count);
    dev_info(uart->dev, "Write: Preparing %zu bytes\n", count);

    dev_info(uart->dev, "WRITE-DMA - HW_TRIGGER\n");


    iowrite32(0x7, uart->base + UART_MDR1); // disable uart
    printk("TX FIFO lv = %d\n", ioread32(uart->base + UART_TXFIFO_LVL));/////////
    iowrite32(0x0, uart->base + UART_IER); // disable TX RX interrupt /////////

#if (API_USE == 1)
    /* ======================================== APIs ============================================ */
    init_completion(&uart->tx_completion);
    tx_desc = dmaengine_prep_slave_single(uart->tx_chan, (dma_addr_t)dma_addr + 1, count - 1,
                                        DMA_MEM_TO_DEV, DMA_CTRL_ACK | DMA_PREP_INTERRUPT);
    if (!tx_desc) {
        dev_err(uart->dev, "Failed to prepare TX DMA descriptor\n");
        goto free_buf;
    }

    tx_desc->callback = bbb_uart_tx_dma_callback;
    tx_desc->callback_param = uart;

    dev_info(uart->dev, "Submitting TX DMA descriptor\n");
    cookie = dmaengine_submit(tx_desc);

	// dma_sync_single_for_device(uart->dev, (dma_addr_t)dma_addr,
	// 			   count, DMA_TO_DEVICE);

    dev_info(uart->dev, "Issuing TX DMA pending, cookie = %d\n", cookie);
    dma_async_issue_pending(uart->tx_chan);

    // iowrite32(0x40, uart->base + 0x0); 
    iowrite32(((char*)kbuf)[0], uart->base + 0x0); 

    ret = wait_for_completion_timeout(&uart->tx_completion, msecs_to_jiffies(1000));
    if (ret == 0) {
        dev_err(uart->dev, "DMA TX timeout\n");
        dmaengine_terminate_sync(uart->tx_chan);
    } else {
        dev_info(uart->dev, "DMA TX completed\n");
    }

    printk("After DMA completion, TX FIFO lv = %d\n", ioread32(uart->base + UART_TXFIFO_LVL));/////////
    
#elif (REG_USE == 1)
    /* ======================================== Regs ============================================ */
    uart->edma_base = ioremap(0x49000000, 0x10000);
    u32 param_addr = 0, AB_Cnt;
    param_addr = 0x4820; /* ch 28 -> paramset 65 */

    AB_Cnt = (count-1)<<16 | 1;
    //AB_Cnt = 1<<16 | count;

    int xxx = 0;;

    iowrite32(0x91C000, uart->edma_base + param_addr + 0x0);  /* OPT */
    /* Configure PaRAM Set for channel 28 */
    iowrite32(dma_addr + 1, uart->edma_base + param_addr + 0x4);  /* SRC */
    iowrite32(AB_Cnt, uart->edma_base + param_addr + 0x8);  /* ACNT=6, BCNT=1 */
    
    iowrite32(uart->uart_phy_base, uart->edma_base + param_addr + 0xC);  /* DST */
    //iowrite32(test_addr, uart->edma_base + param_addr + 0xC);  /* DST */

    //iowrite32(0x000001, uart->edma_base + param_addr + 0x10);  /* DSTBIDX=0, SRCBIDX=1 */
    iowrite32(0<<16 | 1, uart->edma_base + param_addr + 0x10);  /* DSTBIDX=0, SRCBIDX=1 */
    
    iowrite32(0xFFFF, uart->edma_base + param_addr + 0x14);  /* LINK=0xFFFF */
    iowrite32(1, uart->edma_base + param_addr + 0x18);  /* CIDX=1 */
    iowrite32(1, uart->edma_base + param_addr + 0x1C);  /* CCNT=1 */


    /* Clear any flags */
    iowrite32(1 << 28, uart->edma_base + DMA_ECR);  
    iowrite32(1 << 28, uart->edma_base + DMA_EMCR);  
    iowrite32(1 << 28, uart->edma_base + DMA_SECR);  


    /* Enable channel 28 */
    iowrite32(1 << 28, uart->edma_base + DMA_EESR);  /* EESR , HW-TRIGGER*/

    // iowrite32(0x05, uart->base + UART_FCR);  /* Clear TX FIFO */
    iowrite32(((char*)kbuf)[0], uart->base + 0x0); 
    
    while (!(ioread32(uart->edma_base + DMA_IPR) & (1 << 28))){
        xxx++;
        if (xxx >= 500000){
            dev_info(uart->dev, "***Timeout***");
            break;
        }
    }

    int h;
    for (h = 0; h < count; h++){
        printk("kbuf[%d] = 0x%x, test_buf[%d] = 0x%x\n", h, ((char*)kbuf)[h], h, ((char*)test_buf)[h]);
    }
    printk("*****After DMA completion, TX FIFO lv = %d*****\n", ioread32(uart->base + UART_TXFIFO_LVL));/////////

    iowrite32(0x0, uart->base + UART_MDR1); // enable uart
    // iowrite32(0x1, uart->base + UART_IER); // disable TX RX interrupt /////////
#endif

free_buf:
    dma_free_coherent(uart->dev, count, kbuf, dma_addr);
}

static void read_dma(struct bbb_uart *uart, const char *data, size_t count)
{
    void *kbuf;
    dma_addr_t dma_addr;
    void *test_buf;
    dma_addr_t test_addr;
    struct dma_async_tx_descriptor *rx_desc;
    dma_cookie_t cookie;
    int ret;
    uart->edma_base = ioremap(0x49000000, 0x10000);

    count = uart->Count;

    // u8 arr[] = {1,2,3,4,5};

    kbuf = dma_alloc_coherent(uart->dev, uart->Count, &dma_addr, GFP_KERNEL | GFP_DMA);
    if (!kbuf) {
        dev_err(uart->dev, "Failed to allocate DMA-coherent kbuf buffer, count = %d\n", count);
        return;
    }
    test_buf = dma_alloc_coherent(uart->dev, uart->Count, &test_addr, GFP_KERNEL | GFP_DMA);
    if (!kbuf) {
        dev_err(uart->dev, "Failed to allocate DMA-coherent test_buf buffer\n");
        return;
    }

    // memcpy(test_buf, arr, count);
    memset(kbuf, 0x0, count);
    dev_info(uart->dev, "Read: Preparing %zu bytes\n", count);

    dev_info(uart->dev, "READ-DMA - HW_TRIGGER\n");


    iowrite32(0x7, uart->base + UART_MDR1); // disable uart
    printk("Before DMA, RX FIFO lv = %d\n", ioread32(uart->base + UART_RXFIFO_LVL));/////////
    iowrite32(0x0, uart->base + UART_IER); // disable TX RX interrupt /////////

#if (API_USE == 1)
    /* ======================================== APIs ============================================ */
    init_completion(&uart->rx_completion);
    rx_desc = dmaengine_prep_slave_single(uart->rx_chan, (dma_addr_t)dma_addr + 1, count - 1,
                                        DMA_DEV_TO_MEM, DMA_CTRL_ACK | DMA_PREP_INTERRUPT);
    if (!rx_desc) {
        dev_err(uart->dev, "Failed to prepare RX DMA descriptor\n");
        goto free_buf;
    }

    rx_desc->callback = bbb_uart_rx_dma_callback;
    rx_desc->callback_param = uart;

    dev_info(uart->dev, "Submitting RX DMA descriptor\n");
    cookie = dmaengine_submit(rx_desc);

	// dma_sync_single_for_device(uart->dev, (dma_addr_t)dma_addr,
	// 			   count, DMA_FROM_DEVICE);

    dev_info(uart->dev, "Issuing RX DMA pending, cookie = %d, uart->base = 0x%p\n", cookie, uart->base);
    dma_async_issue_pending(uart->rx_chan);

    ((char*)kbuf)[0] = ioread32(uart->base + 0x0); 

    ret = wait_for_completion_timeout(&uart->rx_completion, msecs_to_jiffies(1000));
    printk("After DMA completion, RX FIFO lv = %d\n", ioread32(uart->base + UART_RXFIFO_LVL));/////////

    if (ret == 0) {
        dev_err(uart->dev, "DMA RX timeout\n");
        dmaengine_terminate_sync(uart->rx_chan);
    } else {
        dev_info(uart->dev, "DMA RX completed\n");
    }

    printk("*****After DMA completion, RX FIFO lv = %d*****\n", ioread32(uart->base + UART_RXFIFO_LVL));/////////

    printk("*****dst = '%s'*****\n", (char*)kbuf);
    
#elif (REG_USE == 1)
    /* ======================================== Regs ============================================ */
    //uart->edma_base = ioremap(0x49000000, 0x10000);
    u32 param_addr = 0, AB_Cnt;

    //param_addr = 0x4820; /* ch 28 -> paramset 65 */
    param_addr = 0x4840; /****************** ch 29 -> paramset 66 */

    int ch = 29; /****************** ch  */

    AB_Cnt = (count-1)<<16 | 1;
    //AB_Cnt = 1<<16 | count;

    int xxx = 0;

    iowrite32( (0x900000) | (ch << 12), uart->edma_base + param_addr + 0x0);  /** OPT */
    // iowrite32(0x91D000, uart->edma_base + param_addr + 0x0);  / OPT */
    /* Configure PaRAM Set for channel 28 */
    iowrite32(uart->uart_phy_base, uart->edma_base + param_addr + 0x4);  /* SRC */
    iowrite32(AB_Cnt, uart->edma_base + param_addr + 0x8);  /* ACNT=6, BCNT=1 */
    
    //iowrite32(test_addr, uart->edma_base + param_addr + 0xC);  /* DST */
    iowrite32(dma_addr + 1, uart->edma_base + param_addr + 0xC);  /* DST */

    //iowrite32(0x000001, uart->edma_base + param_addr + 0x10);  /* DSTBIDX=0, SRCBIDX=1 */
    iowrite32(1<<16 | 0, uart->edma_base + param_addr + 0x10);  /* DSTBIDX=0, SRCBIDX=1 */
    
    iowrite32(0xFFFF, uart->edma_base + param_addr + 0x14);  /* LINK=0xFFFF */
    iowrite32(1, uart->edma_base + param_addr + 0x18);  /* CIDX=1 */
    iowrite32(1, uart->edma_base + param_addr + 0x1C);  /* CCNT=1 */


    /* Clear any flags */
    iowrite32(1 << ch, uart->edma_base + DMA_ECR);  
    iowrite32(1 << ch, uart->edma_base + DMA_EMCR);  
    iowrite32(1 << ch, uart->edma_base + DMA_SECR);  


    /* Enable channel 28 */
    iowrite32(1 << ch, uart->edma_base + DMA_EESR);  /* EESR , HW-TRIGGER*/

    ((char*)kbuf)[0] = ioread32(uart->base + 0x0); 
    
    while (!(ioread32(uart->edma_base + DMA_IPR) & (1 << ch))){
        xxx++;
        if (xxx >= 500000){
            dev_info(uart->dev, "***Timeout***");
            break;
        }
    }

    // int h;
    // for (h = 0; h < count; h++){
    //     printk("kbuf[%d] = 0x%x, test_buf[%d] = 0x%x\n", h, ((char*)kbuf)[h], h, ((char*)test_buf)[h]);
    // }



    printk("After DMA completion, RX FIFO lv = %d\n", ioread32(uart->base + UART_RXFIFO_LVL));/////////

    printk("*****After DMA completion, RX FIFO lv = %d*****\n", ioread32(uart->base + UART_RXFIFO_LVL));/////////

    printk("*****dst = '%s'*****\n", (char*)kbuf);

    // // iowrite32(0x0, uart->base + UART_MDR1); // enable uart
    // // iowrite32(0x1, uart->base + UART_IER); // disable TX RX interrupt /////////
#endif


free_buf:
    dma_free_coherent(uart->dev, count, kbuf, dma_addr);
}

static ssize_t bbb_uart_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)                             
{
    struct bbb_uart *uart = filp->private_data;
    char *kbuf;
    // int x;

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, buf, count)) {
        kfree(kbuf);
        return -EFAULT;
    }
    printk("TX - count = %d\n", count);

    write_dma(uart, kbuf, count);
    dev_info(uart->dev, "Wrote %zu bytes, last char = '%c'\n", count, kbuf[count - 1]);

    kfree(kbuf);
    return count;
}

static ssize_t bbb_uart_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)                        
{
    struct bbb_uart *uart = filp->private_data;
    unsigned int bytes_read;

    read_dma(uart, uart->RX_buffer, uart->Tx_Rx_count);

    // copy_to_user will print the buf in the terminal
    if (copy_to_user(buf, uart->RX_buffer, uart->Tx_Rx_count)) {
        return -EFAULT;
    }

    /* Save how many bytes we are returning */
    bytes_read = uart->Tx_Rx_count;

    /* Reset buffer and counters */
    memset(uart->RX_buffer, 0, sizeof(uart->RX_buffer));
    uart->Tx_Rx_count = 0;

    // return correct bytes_read to avoid the next automatic read operation
    return bytes_read;
}

static const struct file_operations bbb_uart_fops = {
    .owner = THIS_MODULE,
    .open = bbb_uart_open,
    .release = bbb_uart_release,
    .write = bbb_uart_write,
    .read = bbb_uart_read,
};

/*
LCR(0xBF) -> DLL/DLH and EFR
EFR(1<<4) -> LCR(0x03 = normal operation) -> FCR[5:4] TX
EFR(0<<4) -> LCR(0x03 = normal operation) -> FCR[7:6] RX

EFR(1<<4) -> MCR[6] -> TCR/TLR



LCR != 0xBF and LCR[7] = 1 => Config mode A
LCR = 0xBF and LCR[7] = 1 => Config mode B
LCR[7] = 0 => operational mode 
*/
static void bbb_uart_init_hw(struct bbb_uart *uart)
{
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
	



	// iowrite32(0x88, uart->base + UART_TLR);  // TLR[3:0] = 0 => TX trigger = 1000xx
    //                                          // TLR[7:4] = 0 => RX trigger = 1000xx
	// iowrite32(0x37, uart->base + UART_FCR);  // FCR[5:4] = 3 => TX trigger = xxxx11, 
    //                                          // FCR[7:6] = 1 => RX trigger = xxxx00, 
    //                                          // FCR[2:0] = 7 -> enable FIFO, clear FIFOs


	iowrite32(0x00, uart->base + UART_TLR);  // TLR[3:0] = 0 => TX trigger = 0000xx
                                             // TLR[7:4] = 0 => RX trigger = 1000xx
	iowrite32(0x57, uart->base + UART_FCR);  // FCR[5:4] = 1 => TX trigger = xxxx01, 
                                             // FCR[7:6] = 0 => RX trigger = xxxx00, 
                                             // FCR[2:0] = 7 -> enable FIFO, clear FIFOs

	
	/* TLR[3:0] + FCR[5:4] = 0000 01 -> TX threshold trigger is 1 bytes (<=63) */
    /* TLR[7:4] + FCR[7:6] = 1000 00 -> RX threshold trigger is 32 bytes */

    /* 5. Set MDR1 = 0x00 => 16x UART mode (enable UART) */
    iowrite32(0x07, uart->base + UART_MDR1);
	
    /* 6. Enable RHR interrupt (optional if you're using RX IRQs) */
    iowrite32(UART_IER_RHR_IT, uart->base + UART_IER);	
	
}

static int bbb_uart_configure_dma(struct bbb_uart *uart)
{
    struct dma_slave_config tx_conf = {0};
    struct dma_slave_config rx_conf = {0};
    int ret;

    dev_info(uart->dev, ">> bbb_uart_configure_dma <<\n");

    if (uart->tx_chan) {
        tx_conf.direction = DMA_MEM_TO_DEV;
        tx_conf.dst_addr = (dma_addr_t)(uart->uart_phy_base + UART_THR);
        tx_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE; /* UART uses 8-bit transfers */
        tx_conf.dst_maxburst = 1; /* Single byte per transfer */
        ret = dmaengine_slave_config(uart->tx_chan, &tx_conf);
        if (ret) {
            dev_err(uart->dev, "Failed to configure TX DMA channel: %d\n", ret);
            return ret;
        }
        dev_info(uart->dev, "TX DMA channel configured\n");
    }

    if (uart->rx_chan) {
        rx_conf.direction = DMA_DEV_TO_MEM;
        rx_conf.src_addr = (dma_addr_t)(uart->uart_phy_base + UART_RHR);
        rx_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE; /* UART uses 8-bit transfers */
        rx_conf.src_maxburst = 1; /* Single byte per transfer */
        ret = dmaengine_slave_config(uart->rx_chan, &rx_conf);
        if (ret) {
            dev_err(uart->dev, "Failed to configure RX DMA channel: %d\n", ret);
            return ret;
        }
        dev_info(uart->dev, "RX DMA channel configured\n");
    }

    return 0;
}

static int bbb_uart_probe(struct platform_device *pdev)
{
    struct bbb_uart *uart;
    struct resource *res;
    struct clk *edma_clk;
    int ret;

    dev_info(&pdev->dev, "Probing device: %s, Node: %s\n", 
             pdev->name, pdev->dev.of_node->full_name);

    uart = devm_kzalloc(&pdev->dev, sizeof(*uart), GFP_KERNEL);
    if (!uart)
        return -ENOMEM;

    uart->count = 0;
    uart->count_loop = 0;
    uart->dev = &pdev->dev;

    /* Get the single memory resource */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "No memory resource\n");
        return -ENODEV;
    }
    dev_info(&pdev->dev, "Memory resource start: 0x%x, end: 0x%x, flags: 0x%lx\n",
             res->start, res->end, res->flags);
    uart->uart_phy_base = (void*)res->start;
    // uart->uart_phy_base = (void*)res->start;

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
    dev_info(&pdev->dev, "Uart clock rate: %lu Hz\n", clk_get_rate(uart->clk));

    /* Remaining initialization (e.g., PM, chrdev) */
    pm_runtime_enable(&pdev->dev);
    ret = pm_runtime_get_sync(&pdev->dev);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to enable PM: %d\n", ret);
        clk_disable_unprepare(uart->clk);
        return ret;
    }

    /* =============== DMA ================= */
    edma_clk = devm_clk_get(&pdev->dev, "fck_dma");
    if (IS_ERR(edma_clk)) {
        dev_err(&pdev->dev, "Failed to get eDMA clock: %ld\n", PTR_ERR(edma_clk));
        return PTR_ERR(edma_clk);
    }
    ret = clk_prepare_enable(edma_clk);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable eDMA clock: %d\n", ret);
        return ret;
    }
    dev_info(&pdev->dev, "Edma clock rate: %lu Hz\n", clk_get_rate(edma_clk));

    /* Request DMA channels */
    uart->tx_chan = dma_request_chan(&pdev->dev, "tx111");
    if (IS_ERR(uart->tx_chan)) {
        dev_info(&pdev->dev, "No TX DMA channel, falling back to non-DMA mode: %ld\n",
                 PTR_ERR(uart->tx_chan));
        uart->tx_chan = NULL;
        //return PTR_ERR(uart->tx_chan);
    }
    uart->rx_chan = dma_request_chan(&pdev->dev, "rx111");
    if (IS_ERR(uart->rx_chan)) {
        dev_info(&pdev->dev, "No RX DMA channel, falling back to non-DMA mode: %ld\n",
                 PTR_ERR(uart->rx_chan));
        if (uart->tx_chan)
            dma_release_channel(uart->tx_chan);
        uart->tx_chan = NULL;
        uart->rx_chan = NULL;
        //return PTR_ERR(uart->tx_chan);
    }

    /* COnfig parameters for DMA */
    uart->use_dma = (uart->tx_chan && uart->rx_chan);
    if (uart->use_dma){
        dev_info(&pdev->dev, "Using DMA for UART transfers\n");
        ret = bbb_uart_configure_dma(uart);
        ret = 0;
        if (ret) {
            dev_info(&pdev->dev, "Failed to configure DMA, falling back to non-DMA mode\n");
            if (uart->tx_chan)
                dma_release_channel(uart->tx_chan);
            if (uart->rx_chan)
                dma_release_channel(uart->rx_chan);
            uart->tx_chan = NULL;
            uart->rx_chan = NULL;
            uart->use_dma = false;
        }
        else {
            dev_info(&pdev->dev, "Succeed to configure DMA\n");
        }
    }
    else
        dev_info(&pdev->dev, "Using interrupt-driven transfers\n");

    init_completion(&uart->tx_completion);
    init_completion(&uart->rx_completion);  

    dump_edma_dchmap(uart); 

    /* ===================== Create a charecter device ======================== */
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

    dev_info(uart->dev, "LCR=0x%x, FCR=0x%x, MDR1=0x%x, MCR=0x%x, DLL = 0x%x, SCR = 0x%x\n",
            ioread32(uart->base + UART_LCR),
            ioread32(uart->base + UART_FCR),
            ioread32(uart->base + UART_MDR1),
            ioread32(uart->base + UART_MCR),
            ioread32(uart->base + UART_DLL),
            ioread32(uart->base + UART_SCR));

    return 0;
}

/* Remove function */
static int bbb_uart_remove(struct platform_device *pdev)
{
    struct bbb_uart *uart = platform_get_drvdata(pdev);

    if (uart->tx_chan)
        dma_release_channel(uart->tx_chan);
    if (uart->rx_chan)
        dma_release_channel(uart->rx_chan);


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
