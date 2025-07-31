#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>

#define DRIVER_NAME "i2c1_device_driver"
#define DEVICE_NAME "i2c1"


struct i2c_device_data {
	struct i2c_client *client;

	u8 line_num;
	u8 cursor_position;
	u8 font_size;

    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;
};

static int i2c1_write(struct i2c_device_data* data, unsigned char * buf, unsigned int len){
	return i2c_master_send(data->client, buf, len);
}

static int i2c1_master_open(struct inode *inode, struct file *file)
{
    struct i2c_device_data *data = container_of(inode->i_cdev, struct i2c_device_data, cdev);
    file->private_data = data;
    return 0;
}

static ssize_t i2c1_master_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct i2c_device_data *data = filp->private_data;
    u8 *tx_buf;

    // Allocate buffer for TX data
    tx_buf = kmalloc(count, GFP_KERNEL);
    if (!tx_buf)
        return -ENOMEM;

    if (copy_from_user(tx_buf, buf, count)) {
        kfree(tx_buf);
        return -EFAULT;
    }

    dev_info(data->dev, "Writing %zu bytes: %*ph\n", count, (int)count, tx_buf);

    i2c1_write(data, tx_buf, count);

    kfree(tx_buf);
    return count;
}

static const struct file_operations i2c_device_fops = {
    .owner = THIS_MODULE,
    .open = i2c1_master_open,
    .write = i2c1_master_write,
};

static int Create_dev(struct i2c_device_data* data){
	int ret = 0;
    // Create character device
    ret = alloc_chrdev_region(&data->dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        dev_err(data->dev, "Failed to allocate chrdev region: %d\n", ret);
        return ret;
    }

    cdev_init(&data->cdev, &i2c_device_fops);
    data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&data->cdev, data->dev_num, 1);
    if (ret < 0) {
        dev_err(data->dev, "Failed to add cdev: %d\n", ret);
        //unregister_chrdev_region(&data->dev_num, 1);
        //iounmap(data->base);
        return ret;
    }

    data->class = class_create(THIS_MODULE, "i2c1_class");
    if (IS_ERR(data->class)) {
        dev_err(data->dev, "Failed to create class: %ld\n", PTR_ERR(data->class));
        cdev_del(&data->cdev);
        //unregister_chrdev_region(&data->dev_num, 1);
        //iounmap(data->base);
        return PTR_ERR(data->class);
    }

    data->dev = device_create(data->class, data->dev, data->dev_num, NULL, "i2c1");
    if (IS_ERR(data->dev)) {
        dev_err(data->dev, "Failed to create device: %ld\n", PTR_ERR(data->dev));
        class_destroy(data->class);
        cdev_del(&data->cdev);
        //unregister_chrdev_region(&data->dev_num, 1);
        //iounmap(data->base);
        return PTR_ERR(data->dev);
    }

    dev_info(data->dev, "Created /dev/%s\n", DEVICE_NAME);

	return 0;
}

static int i2c1_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_device_data* data;
	int ret = 0;

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (!data){
		pr_err("kmalloc failed\n");
	}

	data->dev = &client->dev;

	data->client = client;
	data->line_num = 0;
	data->cursor_position = 0;
	data->font_size = 50;
	i2c_set_clientdata(client, data);

	ret = Create_dev(data);
	if (ret < 0){
		pr_err("dev failed\n");
	}
    return 0;
}

static int i2c1_remove(struct i2c_client *client)
{
	struct i2c_device_data* data = i2c_get_clientdata(client);

	device_destroy(data->class, data->dev_num);
    class_destroy(data->class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->dev_num, 1);

	kfree(data);
    return 0;
}

static const struct i2c_device_id i2c_device_of_id[] = {
    { .name = DRIVER_NAME, },
    { /* sentinel */ }
};

static const struct of_device_id i2c_device_of_match[] = {
    { .compatible = "ssd1306" },
    { /* sentinel */ }
};

static struct i2c_driver i2c_driver = {
    .probe = i2c1_probe,
    .remove = i2c1_remove,
	.id_table = i2c_device_of_id,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = i2c_device_of_match,
    },
};

module_i2c_driver(i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Soviet");
MODULE_DESCRIPTION("Custom I2C Device Driver for BeagleBone Black SPI0");
