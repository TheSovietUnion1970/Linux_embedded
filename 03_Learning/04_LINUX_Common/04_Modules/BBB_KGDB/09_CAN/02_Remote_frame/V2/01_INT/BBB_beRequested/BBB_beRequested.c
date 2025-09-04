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
#include <linux/delay.h>

#define DRIVER_NAME "can0_driver"
#define DEVICE_NAME "can0"

#define MS_DELAY 2000

#define CAN0_BASE       0x481cc000
#define GPIO_BASE       0x44e10000

#define CAN_CTL 0x00
#define CAN_ES 0x04
#define CAN_BTR 0x0C
#define CAN_INT 0x10
#define CAN_IF1CMD 0x100
#define CAN_IF1MSK 0x104
#define CAN_IF1ARB 0x108
#define CAN_IF1MCTL 0x10C
#define CAN_IF1DATA 0x110
#define CAN_INTPND_X 0xAC
#define CAN_INTPND12 0xB0
#define CAN_MSGVAL12 0xC4
#define CAN_MSGVAL34 0xC8
#define CAN_MSGVAL56 0xCC
#define CAN_MSGVAL78 0xD0
#define CAN_NWDAT_X 0x98
#define CAN_NWDAT12 0x9C
#define CAN_TXRQ_X 0x84
#define CAN_TXRQ12 0x88
#define CAN_INTMUX12 0xD8

#define CAN_CTL_INIT BIT(0)
#define CAN_CTL_INIT_OFFSET 0
#define CAN_CTL_CCE BIT(6)
#define CAN_CTL_CCE_OFFSET 6
#define CAN_CTL_DAR BIT(5)
#define CAN_CTL_IE1 BIT(17)
#define CAN_CTL_IE0 BIT(1)
#define CAN_CTL_SIE BIT(2)
#define CAN_CTL_EIE BIT(3)

#define CAN_IFxCMD_msgnum 0xff // [7:0]
#define CAN_IFxCMD_Busy BIT(15)
#define CAN_IFxCMD_Busy_OFFSET 15
#define CAN_IFxCMD_WR_RD BIT(23)

// #define CAN_IFxMSK_Msk 0x1fffffff // [28:0]
// #define CAN_IFxMSK_MDir BIT(30)
// #define CAN_IFxMSK_MXtd BIT(31)
#define CAN_IFxCMD_Arb BIT(21)

#define CAN_IFxARB_MsgVal BIT(31)
#define CAN_IFxARB_Dir BIT(29)
#define CAN_IFxARB_Xtd BIT(30)
#define CAN_IFxARB_ID 0x1fffffff // [28:0]

#define CAN_IFxMCTL_UMask BIT(12)
#define CAN_IFxMCTL_EoB BIT(7)
#define CAN_IFxMCTL_NewDat BIT(15)
#define CAN_IFxMCTL_TxRqst BIT(8)
#define CAN_IFxMCTL_DLC 0xf // [3:0]
#define CAN_IFxMCTL_IntPnd BIT(13)
#define CAN_CTL_SWR BIT(15)
#define CAN_CTL_SWR_OFFSET 15

#define CAN_IFxDATA_0 0xff // [7:0]
#define CAN_IFxDATA_1 0xff00 // [15:8]
#define CAN_IFxDATA_2 0xff0000 // [23:16]
#define CAN_IFxDATA_3 0xff000000 // [31:24]

#define RD 0
#define WR 1

#define ID_CAN0 0x112
#define DMA_OBJ_MSG_NUM 12
u8 TX[4] = {0x00, 0x01, 0x02, 0x03};
u8 size = sizeof(TX);

struct can_device_data {

    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;

    void __iomem *base;  // Mapped base address of can0 registers
    void __iomem *base_gpio;
    struct clk *clk;

    u8 increment;
    struct work_struct re_request_work;
    bool is_scheduled;
    atomic_t should_stop; // Use atomic_t instead of bool

    int irq;
    u8 count_many;
};

static int wait_register_update(struct can_device_data *data, u16 offset, u16 bit_offset, u8 bit_val, u16 delay_ms, u8* name_register){
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

void can0_bit_timing(struct can_device_data *data, u32 can_clk, u32 bitrate){
    u32 val = 0;

    u8 SJW = 1;
    u8 TSeg1 = 4;
    u8 TSeg2 = 3;

    /* CAN_CLK is 24MHz */
    u16 BRP = can_clk/(bitrate*(SJW + TSeg1 + TSeg2)); // 5
    printk("BRP = 0x%x\n", BRP);

    BRP = BRP - 1;
    SJW = SJW - 1;
    TSeg1 = TSeg1 - 1;
    TSeg2 = TSeg2 - 1;

    val = ((BRP)&0x3F) | ((SJW << 6)&0xC0) | ((TSeg1 << 8)&0xF00) | ((TSeg2 << 12)&0x7000);

    iowrite32(val, data->base + CAN_BTR);

}

void can0_receive_obj_Remote_Frames_read(struct can_device_data *data, u8 msg_num){
    u32 if1cmd = 0;

    if1cmd = ioread32(data->base + CAN_IF1CMD);
    if1cmd &=~ (1u << 23); // CAN_IFxCMD_WR_RD: Read
    if1cmd |= msg_num&(0xFF);
    iowrite32(if1cmd, data->base + CAN_IF1CMD);

    wait_register_update(data, CAN_IF1CMD, CAN_IFxCMD_Busy_OFFSET, 0, MS_DELAY, "Busy bit");
}

void can0_receive_obj_Remote_Frames_write(struct can_device_data *data, u8 msg_num){
    u32 if1cmd = 0, if1data = 0, if1mctl = 0;

    // Transfer the data bytes of a message into a message object 
    if1data = TX[0]&(CAN_IFxDATA_0);
    if1data |= (TX[1] << 8)&(CAN_IFxDATA_1);
    if1data |= (TX[2] << 16)&(CAN_IFxDATA_2);
    if1data |= (TX[3] << 24)&(CAN_IFxDATA_3);
    iowrite32(if1data, data->base + CAN_IF1DATA); 

    // config msg control
    if1mctl = ioread32(data->base + CAN_IF1MCTL);
    if1mctl &=~ (1u << 8); // :CAN_IFxMCTL_TxRqst message object is set by rmtEn at reception /* Remote frame only */
    iowrite32(if1mctl, data->base + CAN_IF1MCTL); 

    if1cmd = ioread32(data->base + CAN_IF1CMD);
    if1cmd |= (1u << 23); // CAN_IFxCMD_WR_RD: Write
    if1cmd |= msg_num&(0xFF);
    iowrite32(if1cmd, data->base + CAN_IF1CMD);

    wait_register_update(data, CAN_IF1CMD, CAN_IFxCMD_Busy_OFFSET, 0, MS_DELAY, "Busy bit");
}

void can0_receive_obj_Remote_Frames_init(struct can_device_data *data, u16 can_id, u8 msg_num, u8 msg_handler){
    u32 if1cmd = 0, if1mctl = 0, if1arb = 0, if1data = 0, if1msk = 0;

    // ID: accept any ID comming from any node
    if1msk &=~ (1u << 31); // Mask Extended Identifier or acceptance filtering.
    if1msk &=~ (1u << 30); // the message direction bit (Dir) is used for acceptance filtering
    if1msk &=~ (0x7FF << 18); // Match all ID bits, no wildcards
    iowrite32(if1msk, data->base + CAN_IF1MSK); 

    // Transfer a complete message structure into a message object.
    iowrite32(0, data->base + CAN_IF1ARB); // reset arb registers
    if (msg_handler == 1) if1arb = (1u << 31)&(CAN_IFxARB_MsgVal); // The message object is to be used by the message handler.
    else if1arb &=~ (1u << 31)&(CAN_IFxARB_MsgVal);
    if1arb &=~ (1U << 30); // CAN_IFxARB_Xtd: no Extended Identifier
    if1arb |= (1U << 29); // CAN_IFxARB_Dir: transmit
    if1arb |= (can_id << 18)&(CAN_IFxARB_ID); // 11-bit ID for [28:18]
    iowrite32(if1arb, data->base + CAN_IF1ARB); 

    // Transfer the data bytes of a message into a message object 
    if1data = TX[0]&(CAN_IFxDATA_0);
    if1data |= (TX[1] << 8)&(CAN_IFxDATA_1);
    if1data |= (TX[2] << 16)&(CAN_IFxDATA_2);
    if1data |= (TX[3] << 24)&(CAN_IFxDATA_3);
    iowrite32(if1data, data->base + CAN_IF1DATA); 

    // config msg control,  set TxRqst 
    if1mctl = (1u << 12); // CAN_IFxMCTL_UMask: mask used (Msk[28:0], MXtd, and MDir)
    if1mctl |= (1u << 7); // CAN_IFxMCTL_EoB: a single msg obj
    if1mctl &=~ (1u << 15); // CAN_IFxMCTL_NewDat: 

    if1mctl |= (1u << 9); //  RmtEn: At the reception of a remote frame, TxRqst is set. /* Remote frame only */
    if1mctl &=~ (1u << 8); // :CAN_IFxMCTL_TxRqst message object is set by rmtEn at reception /* Remote frame only */

    if1mctl |= (size); // DLC = size byte
    if1mctl |= (1u << 10); // RxIE /* Remote frame only */
    iowrite32(if1mctl, data->base + CAN_IF1MCTL); 

 
    // ===== config cmd as the last config
    if1cmd = ioread32(data->base + CAN_IF1CMD);

    if1cmd = (1u << 21)&(CAN_IFxCMD_Arb); // ****** Access arbitration bits -> to update the next time
    if1cmd |= (1u << 22); // use mask

    if1cmd |= (1u << 23); // CAN_IFxCMD_WR_RD: Write
    if1cmd &=~ (1u << 18); // TxRqst_NewDat will by handled by CAN_IFxMCTL_TxRqst or CAN_IFxMCTL_NewDat in CAN_IFxMCTL

    if1cmd |= (1u << 20); // access control bits: msg control bits is transfered FROM IF1 register set TO message object by message number (Bits [7:0]).
    if1cmd |= (1u << 17); // use DATA_A: The data bytes 0-3 will be   transfered FROM IF1 register set TO message object by message number (Bits [7:0]).

    if1cmd |= msg_num << 0; // CAN_IFxCMD_msgnum = 1, trigger transfer

    iowrite32(if1cmd, data->base + CAN_IF1CMD);

    wait_register_update(data, CAN_IF1CMD, CAN_IFxCMD_Busy_OFFSET, 0, MS_DELAY, "Busy bit");
    //data->count_many = 0;

}

void Reset_msg_obj(struct can_device_data *data, u8 rd_wr, u8 msg_num, u8 msg_handler){
    u32 if1cmd = 0, if1arb = 0;

    iowrite32(0x00, data->base + CAN_IF1MCTL); // Reset mctl
    iowrite32(0x00, data->base + CAN_IF1MSK); // Reset msk

    iowrite32(0, data->base + CAN_IF1ARB); // reset arb registers
    if (msg_handler == 1) if1arb = (1u << 31)&(CAN_IFxARB_MsgVal); // The message object is to be used by the message handler.
    else if1arb &=~ (1u << 31);
    iowrite32(if1arb, data->base + CAN_IF1ARB); 

    // cmd: w/r, control, msgnum, arb
    if1cmd |= (1u << 21); // Access arbitration bits
    if1cmd |= (1u << 22); // Access mask bits
    if1cmd |= (1u << 20); // access control bits
    if (rd_wr == 1) if1cmd |= 1u << 23; // W
    else if1cmd &=~ (1u << 23); // R
    if1cmd |= msg_num&(0xFF);
    iowrite32(if1cmd, data->base + CAN_IF1CMD);

    // wait the hanler send msg object from IF1 registers to msg RAM
    wait_register_update(data, CAN_IF1CMD, CAN_IFxCMD_Busy_OFFSET, 0, MS_DELAY, "Busy bit");
}

static irqreturn_t irqHandler(int irq, void *d){
    struct can_device_data *data = d;
    u32 canctl, Int0ID, can_mctl, can_cmd, can_arb;
    u32 es = 0, intpnd_x = 0, intpnd12 = 0, msgval12 = 0, msgval34 = 0, msgval56 = 0, msgval78 = 0;
    u32 datx, dat12;
    u32 txx, tx12, mux12;
    u16 id = 0;

    if (!data) {
        pr_err("NULL data in irqHandler\n");
        return IRQ_NONE;
    }

    // reset
    iowrite32(0x0, data->base + CAN_IF1ARB);
    iowrite32(0x0, data->base + CAN_IF1MCTL);
    // read data at DMA_OBJ_MSG_NUM
    can0_receive_obj_Remote_Frames_read(data, DMA_OBJ_MSG_NUM);

    intpnd12 = ioread32(data->base + CAN_INTPND12);
    intpnd_x = ioread32(data->base + CAN_INTPND_X);
    Int0ID = ioread32(data->base + CAN_INT);
    can_mctl = ioread32(data->base + CAN_IF1MCTL);
    can_cmd = ioread32(data->base + CAN_IF1CMD);
    can_arb = ioread32(data->base + CAN_IF1ARB);

    msgval12 = ioread32(data->base + CAN_MSGVAL12);
    msgval34 = ioread32(data->base + CAN_MSGVAL34);
    msgval56 = ioread32(data->base + CAN_MSGVAL56);
    msgval78 = ioread32(data->base + CAN_MSGVAL78);

    datx = ioread32(data->base + CAN_NWDAT_X);
    dat12 = ioread32(data->base + CAN_NWDAT12);
    txx = ioread32(data->base + CAN_TXRQ_X);
    tx12 = ioread32(data->base + CAN_TXRQ12);
    mux12 = ioread32(data->base + CAN_INTMUX12);

    id = (can_arb >> 18)&0x7FF;

    if (Int0ID != 0x8000){ 
        /* Not applicable for Remote frame */
    } 
    else { /* Int0ID = 0x8000 */
        es = ioread32(data->base + CAN_ES);
        printk("es = 0x%x, mctl = 0x%x, intpnd_x = 0x%x, id = 0x%x, pd12 = 0x%x\n", es, can_mctl, intpnd_x, id, intpnd12);

        if ((es&(1u << 4u)) == 1u << 4u){ // RxOk -> update Tx value
            if (TX[0] == 0x1F) TX[0] = 0;
            if (TX[1] == 0x1F) TX[1] = 0;
            if (TX[2] == 0x1F) TX[2] = 0;
            if (TX[3] == 0x1F) TX[3] = 0;

            TX[0]++;
            TX[1]++;
            TX[2]++;
            TX[3]++;
            //can0_receive_obj_Remote_Frames_init(data, ID_CAN0, DMA_OBJ_MSG_NUM, 1);
            can0_receive_obj_Remote_Frames_write(data, DMA_OBJ_MSG_NUM);
        }
        else {
            // raised after responding to send data frame back
        }
    }


    data->count_many++;
    if (data->count_many > 100){
        printk("Too many interrupts\n");
        data->count_many = 0;

        canctl = ioread32(data->base + CAN_CTL);
        canctl &=~ CAN_CTL_IE0; // disable DCAN0INT
        iowrite32(canctl, data->base + CAN_CTL);
    }


    return IRQ_HANDLED;
}

void GPIO_init(struct can_device_data *data){
    /* P9_19: can0_rx - AM335X_PIN_UART1_RTSN 0x97c */
    iowrite32(0x32, data->base_gpio + 0x97c);

    /* P9_20: can0_tx - AM335X_PIN_UART1_CTSN 0x978 */
    iowrite32(0x12, data->base_gpio + 0x978);
}

void can0_INT_init(struct can_device_data *data){
    u32 canctl = 0;

    canctl = ioread32(data->base + CAN_CTL);
    canctl |= CAN_CTL_IE0; // DCan0INT
    canctl |= CAN_CTL_IE1; // DCAN1INT
    canctl |= CAN_CTL_SIE; // used for see status if no INT for msg object is raised
    canctl |= CAN_CTL_EIE; 
    iowrite32(canctl, data->base + CAN_CTL);

}

void can0_init(struct can_device_data *data) {
    u32 can_ctl = 0;
    int i = 0;

    can_ctl = CAN_CTL_INIT | CAN_CTL_CCE ; // CAN_CTL_DAR is used to disable auto retransmission
    iowrite32(can_ctl, data->base + CAN_CTL); // enter init mode, access to registers
    //while((ioread32(data->base + CAN_CTL)&CAN_CTL_INIT) == CAN_CTL_INIT); // wait init = 1;
    wait_register_update(data, CAN_CTL, CAN_CTL_INIT_OFFSET, 1, MS_DELAY, "INIT mode");
    //wait_register_update(data, CAN_CTL, 15, 0, MS_DELAY, "Reset state");

    // Bit timing values into BTR, output bitrate is 500KHz
    can0_bit_timing(data, clk_get_rate(data->clk), 500000);

    // In init mode, configure received obj for all slots (0-128)
    for (i = 0; i < 64; i ++){
        Reset_msg_obj(data, RD, i+1, 0); // reset msg obj including msgVal
    }
    // init transmit obj with remote frame configuration
    can0_receive_obj_Remote_Frames_init(data, ID_CAN0, DMA_OBJ_MSG_NUM, 1);
    // re-configure tramsmit obj into receive obj
    //can0_receive_obj_Remote_Frames_read(data, DMA_OBJ_MSG_NUM);

    // clear init, CCE
    can_ctl &=~ (CAN_CTL_INIT | CAN_CTL_CCE);
    iowrite32(can_ctl, data->base + CAN_CTL); // enter init mode, access to registers
    //while((ioread32(data->base + CAN_CTL)&CAN_CTL_INIT) != CAN_CTL_INIT); // wait init = 0;
    wait_register_update(data, CAN_CTL, CAN_CTL_INIT_OFFSET, 0, MS_DELAY, "Normal mode");
    printk("ctl[0] = 0x%x\n", ioread32(data->base + CAN_CTL));

    // if1mctl = ioread32(data->base + CAN_IF1MCTL);
    // if1mctl &=~ (1u << 8); // :CAN_IFxMCTL_TxRqst message object is not waiting for a transmission
    // iowrite32(if1mctl, data->base + CAN_IF1MCTL); 
    // iowrite32(if1cmd, data->base + CAN_IF1CMD); // Apply the update
}

static int can0_open(struct inode *inode, struct file *file){
    struct can_device_data *data = container_of(inode->i_cdev, struct can_device_data, cdev);
    file->private_data = data;
    return 0;
}

static void re_request_irq_work(struct work_struct *work)
{
    struct can_device_data *data = container_of(work, struct can_device_data, re_request_work);
    u32 txx, tx12, mux12;
    
    while (!atomic_read(&data->should_stop)){
        // Config message object
        can0_receive_obj_Remote_Frames_init(data, ID_CAN0, 5, 1);

        txx = ioread32(data->base + CAN_TXRQ_X);
        tx12 = ioread32(data->base + CAN_TXRQ12);
        mux12 = ioread32(data->base + CAN_INTMUX12);
    
        printk("WORK -> txrq = 0x%x, can_mctl = 0x%x\n", ioread32(data->base + CAN_TXRQ12), ioread32(data->base + CAN_IF1MCTL));

        TX[0]++;
        TX[1]++;
        TX[2]++;
        TX[3]++;

        if (TX[0] == 0xFF) TX[0] = 0;
        if (TX[1] == 0xFF) TX[1] = 0;
        if (TX[2] == 0xFF) TX[2] = 0;
        if (TX[3] == 0xFF) TX[3] = 0;
 

        msleep(1000);
    }
}

static ssize_t can0_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct can_device_data *data = filp->private_data;

    schedule_work(&data->re_request_work);

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
    data->dev = &pdev->dev;
    data->increment = 0;
    data->is_scheduled = 0;
    atomic_set(&data->should_stop, 0); // Initialize to 0 (false)

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

    // ====== Can init
    GPIO_init(data);
    can0_init(data);
    //INIT_WORK(&data->re_request_work, re_request_irq_work);
    can0_INT_init(data);

    // ========== Request IRQ (hwirq 30 maps to swirq x on AM33xx) ==========
    data->irq = platform_get_irq(pdev, 0);
    if (data->irq < 0) {
        dev_err(&pdev->dev, "Failed Formatted: Unable to get IRQ: %d\n", data->irq);
        return data->irq;
    }
    ret = devm_request_irq(&pdev->dev, data->irq, irqHandler, 0, "can0", data);
    if (ret < 0) {
        dev_err(&pdev->dev, "Unable to request IRQ %d: %d\n", data->irq, ret);
        return ret;
    }
    dev_info(&pdev->dev, "IRQ num = %d\n", data->irq);

    // ====== Create device character
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

    //iowrite32(0x0, data->base + CAN_CTL); // enter init mode, access to registers
    //while((ioread32(data->base + CAN_CTL)&CAN_CTL_INIT) == CAN_CTL_INIT); // wait init = 0;
    wait_register_update(data, CAN_CTL, CAN_CTL_INIT_OFFSET, 0, MS_DELAY, "Normal mode 1");
    printk("ctl1[0] = 0x%x\n", ioread32(data->base + CAN_CTL));

    return 0;
}

static int can0_remove(struct platform_device *pdev)
{
    struct can_device_data *data = platform_get_drvdata(pdev);
    u32 can_ctl;

    dev_info(data->dev, "Removed\n");

    atomic_set(&data->should_stop, 1); // Set to 1 (true)
    smp_mb(); // Memory barrier to ensure should_stop is visible

    //cancel_work_sync(&data->re_request_work);

    can_ctl = CAN_CTL_INIT | CAN_CTL_CCE | CAN_CTL_SWR ; // CAN_CTL_DAR is used to disable auto retransmission
    iowrite32(can_ctl, data->base + CAN_CTL); // enter init mode, access to registers
    wait_register_update(data, CAN_CTL, CAN_CTL_INIT_OFFSET, 1, MS_DELAY, "INIT mode");
    wait_register_update(data, CAN_CTL, CAN_CTL_SWR_OFFSET, 0, MS_DELAY, "Reset mode");

    clk_disable_unprepare(data->clk);
    
    if (data->dev)
        device_destroy(data->class, data->dev_num);
    if (data->class)
        class_destroy(data->class);
    cdev_del(&data->cdev);

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

