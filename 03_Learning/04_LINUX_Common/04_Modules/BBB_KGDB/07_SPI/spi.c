#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define DRIVER_NAME "spi0_device_driver"
#define DEVICE_NAME "spi0"

#define SLK 500000

struct spi_device_data {
    struct spi_device *spi;
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;
};

static int spi_device_open(struct inode *inode, struct file *file)
{
    struct spi_device_data *data = container_of(inode->i_cdev, struct spi_device_data, cdev);
    file->private_data = data;
    return 0;
}

static ssize_t spi_device_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    struct spi_device_data *data = filp->private_data;
    struct spi_message msg;
    struct spi_transfer transfer = {0};
    u8 *tx_buf;
    int ret;

    tx_buf = kmalloc(count, GFP_KERNEL);
    if (!tx_buf)
        return -ENOMEM;

    if (copy_from_user(tx_buf, buf, count)) {
        kfree(tx_buf);
        return -EFAULT;
    }

    dev_info(&data->spi->dev, "Writing %zu bytes: %*ph\n", count, (int)count, tx_buf);

    transfer.tx_buf = tx_buf;
    transfer.len = count;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);

    ret = spi_sync(data->spi, &msg);
    dev_info(&data->spi->dev, "spi_sync returned %d, transferred %d bytes\n", ret, msg.actual_length);

    kfree(tx_buf);
    return (ret == 0) ? count : ret;
}

static const struct file_operations spi_device_fops = {
    .owner = THIS_MODULE,
    .open = spi_device_open,
    .write = spi_device_write,
};

static int spi_device_probe(struct spi_device *spi)
{
    struct spi_device_data *data;
    int ret;

    dev_info(&spi->dev, "SPI device probed successfully with compatible 'spi0-based'\n");

    data = devm_kzalloc(&spi->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->spi = spi;
    spi_set_drvdata(spi, data);

    // Configure SPI device
    spi->mode = SPI_MODE_0;
    spi->bits_per_word = 8;
    spi->max_speed_hz = SLK;
    ret = spi_setup(spi);
    if (ret) {
        dev_err(&spi->dev, "Failed to setup SPI device: %d\n", ret);
        return ret;
    }

    // Create character device
    ret = alloc_chrdev_region(&data->dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        dev_err(&spi->dev, "Failed to allocate chrdev region: %d\n", ret);
        return ret;
    }

    cdev_init(&data->cdev, &spi_device_fops);
    data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&data->cdev, data->dev_num, 1);
    if (ret < 0) {
        dev_err(&spi->dev, "Failed to add cdev: %d\n", ret);
        unregister_chrdev_region(data->dev_num, 1);
        return ret;
    }

    // Create device class
    data->class = class_create(THIS_MODULE, "spi0_class");
    if (IS_ERR(data->class)) {
        dev_err(&spi->dev, "Failed to create class: %ld\n", PTR_ERR(data->class));
        cdev_del(&data->cdev);
        unregister_chrdev_region(data->dev_num, 1);
        return PTR_ERR(data->class);
    }

    // Create device node /dev/spi0
    data->dev = device_create(data->class, &spi->dev, data->dev_num, NULL, "spi0");
    if (IS_ERR(data->dev)) {
        dev_err(&spi->dev, "Failed to create device: %ld\n", PTR_ERR(data->dev));
        class_destroy(data->class);
        cdev_del(&data->cdev);
        unregister_chrdev_region(data->dev_num, 1);
        return PTR_ERR(data->dev);
    }

    dev_info(&spi->dev, "Created /dev/%s\n", DEVICE_NAME);
    return 0;
}

static int spi_device_remove(struct spi_device *spi)
{
    struct spi_device_data *data = spi_get_drvdata(spi);
    if (data->dev)
        device_destroy(data->class, data->dev_num);
    if (data->class)
        class_destroy(data->class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->dev_num, 1);
    dev_info(&spi->dev, "SPI device removed\n");
    return 0;
}

static const struct of_device_id spi_device_of_match[] = {
    { .compatible = "spi0-based" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, spi_device_of_match);

static struct spi_driver spi_device_driver = {
    .probe = spi_device_probe,
    .remove = spi_device_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = spi_device_of_match,
    },
};

module_spi_driver(spi_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Custom SPI Device Driver for BeagleBone Black SPI0");
