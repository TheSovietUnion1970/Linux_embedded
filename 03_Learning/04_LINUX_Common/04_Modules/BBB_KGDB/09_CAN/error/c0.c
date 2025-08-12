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
#define CAN_BTR 0x0C
#define CAN_IF1CMD 0x100
#define CAN_IF1MSK 0x104
#define CAN_IF1ARB 0x108
#define CAN_IF1MCTL 0x10C
#define CAN_IF1DATA 0x110

#define CAN_CTL_INIT BIT(0)
#define CAN_CTL_INIT_OFFSET 0
#define CAN_CTL_CCE BIT(6)
#define CAN_CTL_CCE_OFFSET 6

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

#define CAN_IFxDATA_0 0xff // [7:0]
#define CAN_IFxDATA_1 0xff00 // [15:8]

#define ID_CAN0 0x01
u8 TX[2] = {0x01, 0x02};

struct can_device_data {

    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;

    void __iomem *base;  // Mapped base address of can0 registers
    void __iomem *base_gpio;
    struct clk *clk;
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

void can0_bit_timing(struct can_device_data *data){
    u32 fq = 500000; // Frequency Quantum: 500kHz
    u32 val = 0;

    u8 SJW = 1;
    u8 TSeg1 = 4;
    u8 TSeg2 = 3;

    /* CAN_CLK is 24MHz */
    u16 BRP = clk_get_rate(data->clk)/(fq*(SJW + TSeg1 + TSeg2)); // 5
    printk("BRP = 0x%x\n", BRP);

    BRP = BRP - 1;
    SJW = SJW - 1;
    TSeg1 = TSeg1 - 1;
    TSeg2 = TSeg2 - 1;

    val = ((BRP)&0x3F) | ((SJW << 6)&0xC0) | ((TSeg1 << 8)&0xF00) | ((TSeg2 << 12)&0x7000);

    iowrite32(val, data->base + CAN_BTR);

}

void can0_msg_obj_Data_Frames(struct can_device_data *data){
    u32 if1cmd = 0, if1msk = 0, if1mctl = 0, if1arb = 0, if1data = 0;

    if1cmd = (1u << 21)&(CAN_IFxCMD_Arb); // Access arbitration bits
    if1cmd &=~ (1u << 22); // no use mask
    iowrite32(if1cmd, data->base + CAN_IF1CMD);

    //iowrite32(if1msk, data->base + CAN_IF1MSK); // no use of mask bits

    if1mctl &=~ (1u << 12); // CAN_IFxMCTL_UMask: mask ignored
    if1mctl |= (1u << 7); // CAN_IFxMCTL_EoB: a single msg obj
    if1mctl &=~ (1u << 15); // CAN_IFxMCTL_NewDat: msg_hler/CPU write no new data
    if1mctl &=~ (1u << 8); // CAN_IFxMCTL_TxRqst: message object is not waiting for a transmission
    if1mctl |= (0x1)&CAN_IFxMCTL_DLC; // DLC = 1 byte
    iowrite32(if1mctl, data->base + CAN_IF1MCTL); 

    if1arb = (1u << 31)&(CAN_IFxARB_MsgVal); // The message object is to be used by the message handler.
    if1arb &=~ (1U << 30); // CAN_IFxARB_Xtd: no Extended Identifier
    if1arb |= (1U << 29); // CAN_IFxARB_Dir: transmit
    if1arb |= (ID_CAN0 << 18)&(CAN_IFxARB_ID); // 11-bit ID for [28:18]
    iowrite32(if1arb, data->base + CAN_IF1ARB); 

    if1data |= TX[0]&(CAN_IFxDATA_0);
    //if1data |= (TX[1] << 8)&(CAN_IFxDATA_1);
    iowrite32(if1data, data->base + CAN_IF1DATA); 

    // config cmd
    if1cmd = ioread32(data->base + CAN_IF1CMD);
    if1cmd |= (1u << 23); // CAN_IFxCMD_WR_RD: Write
    if1cmd |= (1u << 20); // access control bits
    if1cmd &=~ (1u << 18); // handled by msg number, not by TxRqst_NewDat
    if1cmd |= (1u << 17); // use DATA_A
    iowrite32(if1cmd, data->base + CAN_IF1CMD);

}

void GPIO_init(struct can_device_data *data){
    /* P9_19: can0_rx - AM335X_PIN_UART1_RTSN 0x97c */
    iowrite32(0x32, data->base_gpio + 0x97c);

    /* P9_20: can0_tx - AM335X_PIN_UART1_CTSN 0x978 */
    iowrite32(0x12, data->base_gpio + 0x978);
}

void can0_init(struct can_device_data *data) {
    u32 can_ctl = 0;

    can_ctl = CAN_CTL_INIT | CAN_CTL_CCE;
    iowrite32(can_ctl, data->base + CAN_CTL); // enter init mode, access to registers
    //while((ioread32(data->base + CAN_CTL)&CAN_CTL_INIT) == CAN_CTL_INIT); // wait init = 1;
    wait_register_update(data, CAN_CTL, CAN_CTL_INIT_OFFSET, 1, MS_DELAY, "INIT mode");

    // Bit timing values into BTR
    can0_bit_timing(data);

    // clear init, CCE
    can_ctl &=~ (CAN_CTL_INIT | CAN_CTL_CCE);
    iowrite32(can_ctl, data->base + CAN_CTL); // enter init mode, access to registers
    //while((ioread32(data->base + CAN_CTL)&CAN_CTL_INIT) != CAN_CTL_INIT); // wait init = 0;
    wait_register_update(data, CAN_CTL, CAN_CTL_INIT_OFFSET, 0, MS_DELAY, "Normal mode");
    printk("ctl[0] = 0x%x\n", ioread32(data->base + CAN_CTL));

}

static int can0_open(struct inode *inode, struct file *file){
    struct can_device_data *data = container_of(inode->i_cdev, struct can_device_data, cdev);
    file->private_data = data;
    return 0;
}

static ssize_t can0_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct can_device_data *data = filp->private_data;
    u32 can_if1cmd = 0;

    // Config message object
    can0_msg_obj_Data_Frames(data);

    can_if1cmd = ioread32(data->base + CAN_IF1CMD);
    can_if1cmd |= 1u << 0; // CAN_IFxCMD_msgnum = 1, trigger transfer
    iowrite32(can_if1cmd, data->base + CAN_IF1CMD);

    if ((ioread32(data->base + CAN_IF1CMD)&BIT(15)) == BIT(15)) printk("HEHEHE\n");

    // wait the hanler send msg object from IF1 registers to msg RAM
    wait_register_update(data, CAN_IF1CMD, CAN_IFxCMD_Busy_OFFSET, 0, MS_DELAY, "Busy bit");

    dev_info(data->dev, "Done transmitting data frame \n");

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

    return 0;
}

static int can0_remove(struct platform_device *pdev)
{
    struct can_device_data *data = platform_get_drvdata(pdev);

    dev_info(data->dev, "Removed\n");
    
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

