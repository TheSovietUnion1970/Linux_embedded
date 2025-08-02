#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/clk.h>

#define SLAVE_ADDRESS 0x41

// Register base addresses
#define I2C2_BASE       0x4819c000
#define GPIO_BASE       0x44e10000

// i2c2 registers (offsets)
#define I2C_SYSC        0x10  // System configuration
#define I2C_SYSS        0x90  // System status
#define I2C_CON         0xA4  // Control
#define I2C_PSC         0xB0  // Prescaler
#define I2C_SCLL        0xB4  // SCL low time
#define I2C_SCLH        0xB8  // SCL high time
#define I2C_SA          0xAC  // Slave address
#define I2C_OA          0xA8  // Own address
#define I2C_CNT         0x98  // Data count
#define I2C_DATA        0x9C  // Data
#define I2C_IRQENABLE_SET 0x2C  // Enable interrupts
#define I2C_IRQENABLE_CLR 0x30  // Enable interrupts
#define I2C_IRQSTATUS_RAW 0x24  // Interrupt raw status
#define I2C_IRQSTATUS   0x28  // Interrupt status
#define I2C_BUF         0x94  // Buffer

#define I2C_IRQSTATUS_RAW_XRDY BIT(4)
#define I2C_IRQSTATUS_RAW_BB BIT(12)
#define I2C_IRQSTATUS_RAW_RRDY BIT(3)
#define I2C_IRQSTATUS_RAW_ARDY BIT(2)
#define I2C_IRQSTATUS_RAW_NACK BIT(1)

#define I2C_BUF_RXTRSH 8 // [13:8]
#define I2C_BUF_RXFIFO_CLR BIT(14)

#define XRDY_IE BIT(4)
#define RRDY_IE BIT(3)
#define AAS_IE BIT(9)

#define RX_TRIGGER 0

#define DRIVER_NAME "i2c2_device_driver"
#define DEVICE_NAME "i2c2"

struct i2c_device_data {
    struct i2c_adapter *adap;

    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;

    void __iomem *base;  // Mapped base address of i2c2 registers
    void __iomem *base_gpio;
    struct clk *clk;

    int irq;
    int count;
    int data_count;

    u8 rx[256];
};

static irqreturn_t irqHandler(int irq, void *d)
{
    struct i2c_device_data *data = d;
    u32 irqsts;
    u8 i;

    irqsts = ioread32(data->base + I2C_IRQSTATUS);

    printk("irqsts = 0x%x\n", ioread32(data->base + I2C_IRQSTATUS));

    if (data->data_count == 0){
        memset(data->rx, 0, 256);
    }

    if ((irqsts & RRDY_IE) == RRDY_IE) {

        for (i = 0; i < RX_TRIGGER + 1; i++){
            data->rx[data->data_count++] = ioread32(data->base + I2C_DATA); // Read data
        }

        // Clear the XRDY interrupt
        iowrite32(RRDY_IE, data->base + I2C_IRQSTATUS);
    }

    if ((irqsts & AAS_IE) == AAS_IE) {

        // used when an address slave is choosen by master
        // Clear the AAS interrupt
        iowrite32(AAS_IE, data->base + I2C_IRQSTATUS);
    }

    if (data->rx[data->data_count - 1] == '\n'){
        data->rx[data->data_count - 1] = '\0';
        data->data_count = 0;
        data->count = 0; // avoid too many interrupts if data is read

        dev_info(data->dev, "Rx: %s\n", data->rx);
    }

    data->count++;
    if (data->count > 100){
        printk("Too many interrupts, 0x%x\n", ioread32(data->base + I2C_IRQSTATUS));
        iowrite32(0, data->base + I2C_IRQENABLE_SET); 
        iowrite32(0xFF, data->base + I2C_IRQENABLE_CLR); 
        data->count = 0;
    }

    return IRQ_HANDLED;
}

void init_clk2(struct i2c_device_data *data, u32 fclk_rate, u32 internal_speed, u32 speed)
{
    u32 scll = 0, sclh = 0;
    u8 psc = 0;

    /* Compute prescaler divisor */
    psc = fclk_rate / internal_speed;
    psc = psc - 1;
    iowrite32(psc, data->base + I2C_PSC); // Prescaler: 48 MHz / (4+1) = 9.6 MHz

    scll = internal_speed / (speed*2) - 7;
    sclh = internal_speed / (speed*2) - 5;
    iowrite32(scll, data->base + I2C_SCLL); // SCL low time
    iowrite32(sclh, data->base + I2C_SCLH); // SCL high time
 
    dev_info(data->dev, "pdc = %d, scll = %d. sclh = %d\n", psc, scll, sclh);
}


void GPIO_init(struct i2c_device_data *data)
{
    iowrite32(0x33, data->base_gpio + 0x978); // i2c2_scl
    iowrite32(0x33, data->base_gpio + 0x97c); // i2c2_sda
}

// Initialize i2c2 as slave
void i2c2_slave_init(struct i2c_device_data *data) {
    // enable_i2c2_clock(data);
    u32 i2c_con = 0;

    i2c_con = ioread32(data->base + I2C_CON);
    i2c_con |= (0u << 15); // [15] disable i2c module before reset
    iowrite32(i2c_con, data->base + I2C_CON);

    dev_info(data->dev, "Begin reset\n");
    iowrite32(0x2, data->base + I2C_SYSC); // Set soft reset

    i2c_con = ioread32(data->base + I2C_CON);
    i2c_con |= (1u << 15); // [15] enable i2c module before reset
    iowrite32(i2c_con, data->base + I2C_CON);

    dev_info(data->dev, "Wait to reset\n");
    while (!(ioread32(data->base + I2C_SYSS)&(1u)));  // Wait for reset complete

    dev_info(data->dev, "Done reset\n");
    iowrite32(0x0, data->base + I2C_SYSC); // Clear reset - normal mode

    i2c_con = ioread32(data->base + I2C_CON);
    i2c_con &= ~(1u << 15); // disable i2c module
    iowrite32(i2c_con, data->base + I2C_CON);

    iowrite32(SLAVE_ADDRESS, data->base + I2C_OA); // Set own address of slave (0x40)
    iowrite32(SLAVE_ADDRESS, data->base + I2C_SA); 

    init_clk2(data, 48000000, 12000000, 100000);

    i2c_con = ioread32(data->base + I2C_CON);
    i2c_con |= (1u << 15)|(0u << 10)|(0u << 9); // [15] enable i2c module, [MST:10]: slave mode, [TRX:9]:  MST = 0, TRX = x, Operating Mode = Slave receiver
    iowrite32(i2c_con, data->base + I2C_CON);

    iowrite32((RX_TRIGGER << I2C_BUF_RXTRSH) | I2C_BUF_RXFIFO_CLR, data->base + I2C_BUF); // clear RX FIFO, RX threshold is 1 byte
    iowrite32(RRDY_IE | AAS_IE, data->base + I2C_IRQENABLE_SET); // Receive data ready interrupt enabled
}

static int i2c2_slave_open(struct inode *inode, struct file *file)
{
    struct i2c_device_data *data = container_of(inode->i_cdev, struct i2c_device_data, cdev);
    file->private_data = data;
    return 0;
}

static ssize_t i2c2_slave_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct i2c_device_data *data = filp->private_data;

    dev_info(data->dev, "Reading %zu bytes: %*ph\n", data->data_count, (int)data->data_count, data->rx);
    dev_info(data->dev, "Rx: %s\n", data->rx);

    count = data->data_count;

    if (copy_to_user(buf, data->rx, count)) {
        return -EFAULT;
    }

    // data->data_count = 0;
    // data->count = 0;
    return 0;
}

static const struct file_operations i2c_device_fops = {
    .owner = THIS_MODULE,
    .open = i2c2_slave_open,
    .read = i2c2_slave_read,
};

static int i2c2_probe(struct platform_device *pdev)
{
    struct i2c_device_data *data;
    int ret;

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    platform_set_drvdata(pdev, data);

    data->base_gpio = ioremap(GPIO_BASE, 0x1000);
    data->base = ioremap(I2C2_BASE, 0x1000);
    data->dev = &pdev->dev;

    /* Clock setup (assuming this part is unchanged) */
    data->clk = devm_clk_get(&pdev->dev, "fck-i2c2");
    if (IS_ERR(data->clk)) {
        dev_err(&pdev->dev, "Failed to get clock: %ld\n", PTR_ERR(data->clk));
        return PTR_ERR(data->clk);
    }
    ret = clk_prepare_enable(data->clk);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable clock: %d\n", ret);
        return ret;
    }
    dev_info(&pdev->dev, "i2c2 clock rate: %lu Hz\n", clk_get_rate(data->clk));

    GPIO_init(data);
    // Initialize hardware
    i2c2_slave_init(data);


    // ========== Request IRQ (hwirq 30 maps to swirq x on AM33xx) ==========
    data->irq = platform_get_irq(pdev, 0);
    if (data->irq < 0) {
        dev_err(&pdev->dev, "Failed Formatted: Unable to get IRQ: %d\n", data->irq);
        return data->irq;
    }
    ret = devm_request_irq(&pdev->dev, data->irq, irqHandler, 0, "i2c2", data);
    if (ret < 0) {
        dev_err(&pdev->dev, "Unable to request IRQ %d: %d\n", data->irq, ret);
        return ret;
    }

    dev_info(&pdev->dev, "IRQ num = %d\n", data->irq);

    // ============ Create character device
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

    data->class = class_create(THIS_MODULE, "i2c2_class");
    if (IS_ERR(data->class)) {
        dev_err(&pdev->dev, "Failed to create class: %ld\n", PTR_ERR(data->class));
        cdev_del(&data->cdev);
        //unregister_chrdev_region(&data->dev_num, 1);
        iounmap(data->base);
        return PTR_ERR(data->class);
    }

    data->dev = device_create(data->class, &pdev->dev, data->dev_num, NULL, "i2c2");
    if (IS_ERR(data->dev)) {
        dev_err(&pdev->dev, "Failed to create device: %ld\n", PTR_ERR(data->dev));
        class_destroy(data->class);
        cdev_del(&data->cdev);
        //unregister_chrdev_region(&data->dev_num, 1);
        iounmap(data->base);
        return PTR_ERR(data->dev);
    }

    dev_info(&pdev->dev, "Created /dev/%s\n", DEVICE_NAME);

    GPIO_init(data);

    return 0;
}

static int i2c2_remove(struct platform_device *pdev)
{
    struct i2c_device_data *data = platform_get_drvdata(pdev);
    
    if (data->dev)
        device_destroy(data->class, data->dev_num);
    if (data->class)
        class_destroy(data->class);
    cdev_del(&data->cdev);

    return 0;
}

static const struct of_device_id i2c_device_of_match[] = {
    { .compatible = "i2c2-based" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, i2c_device_of_match);

static struct platform_driver i2c_device_driver = {
    .probe = i2c2_probe,
    .remove = i2c2_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = i2c_device_of_match,
    },
};

module_platform_driver(i2c_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Custom I2C Device Driver for BeagleBone Black I2C2");
