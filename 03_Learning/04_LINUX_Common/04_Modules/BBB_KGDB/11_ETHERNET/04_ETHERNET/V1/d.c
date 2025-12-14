#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/delay.h>

struct dma_data {
    struct device *dev;
    struct dma_chan *dma_chan;
    struct completion transfer_done;
    struct dentry *debugfs_root;
    u8 *src_buffer;
    u8 *dst_buffer;
    dma_addr_t src_dma_addr;
    dma_addr_t dst_dma_addr;
    size_t buffer_size;
};

#define BUFFER_SIZE (32)  // 4 KB buffer for transfer

/* Debugfs read callback for source buffer */
static ssize_t src_buffer_read(struct file *file, char __user *user_buf,
                               size_t count, loff_t *ppos)
{
    struct dma_data *data = file->private_data;
    return simple_read_from_buffer(user_buf, count, ppos, data->src_buffer, data->buffer_size);
}

/* Debugfs read callback for destination buffer */
static ssize_t dst_buffer_read(struct file *file, char __user *user_buf,
                               size_t count, loff_t *ppos)
{
    struct dma_data *data = file->private_data;
    return simple_read_from_buffer(user_buf, count, ppos, data->dst_buffer, data->buffer_size);
}

/* Debugfs write callback to trigger DMA transfer */
static ssize_t trigger_transfer_write(struct file *file, const char __user *user_buf,
                                     size_t count, loff_t *ppos)
{
    struct dma_data *data = file->private_data;
    struct device *dev = data->dev;
    struct dma_async_tx_descriptor *tx;
    dma_cookie_t cookie;
    enum dma_status status;
    int ret;

    // Prepare buffers with test data
    memset(data->src_buffer, 0xAA, data->buffer_size);  // Fill source with 0xAA
    memset(data->dst_buffer, 0x00, data->buffer_size);  // Clear destination

    // Prepare DMA transfer
    tx = dmaengine_prep_dma_memcpy(data->dma_chan, data->dst_dma_addr,
                                   data->src_dma_addr, data->buffer_size,
                                   DMA_CTRL_ACK | DMA_PREP_INTERRUPT);
    if (!tx) {
        dev_err(dev, "Failed to prepare DMA memcpy\n");
        return -EIO;
    }

    // Set up completion
    init_completion(&data->transfer_done);
    tx->callback = (void (*)(void *))complete;
    tx->callback_param = &data->transfer_done;

    // Submit the transfer
    cookie = dmaengine_submit(tx);
    if (dma_submit_error(cookie)) {
        dev_err(dev, "Failed to submit DMA transfer\n");
        return -EIO;
    }

    // Start the transfer
    dma_async_issue_pending(data->dma_chan);

    // Wait for completion
    ret = wait_for_completion_timeout(&data->transfer_done, msecs_to_jiffies(1000));
    if (ret == 0) {
        dev_err(dev, "DMA transfer timed out\n");
        dmaengine_terminate_sync(data->dma_chan);
        return -ETIMEDOUT;
    }

    // Check transfer status
    status = dma_async_is_tx_complete(data->dma_chan, cookie, NULL, NULL);
    if (status != DMA_COMPLETE) {
        dev_err(dev, "DMA transfer failed: status %d\n", status);
        return -EIO;
    }

    // Verify the data
    if (memcmp(data->src_buffer, data->dst_buffer, data->buffer_size) != 0) {
        dev_err(dev, "DMA transfer verification failed\n");
        return -EIO;
    }

    dev_info(dev, "DMA transfer completed successfully\n");
    return count;
}

static const struct file_operations src_buffer_fops = {
    .read = src_buffer_read,
    .open = simple_open,
};

static const struct file_operations dst_buffer_fops = {
    .read = dst_buffer_read,
    .open = simple_open,
};

static const struct file_operations trigger_fops = {
    .write = trigger_transfer_write,
    .open = simple_open,
};

static int dma_probe(struct platform_device *pdev)
{
    struct dma_data *data;
    struct device *dev = &pdev->dev;
    dma_cap_mask_t mask;
    int ret;

    // Allocate driver data
    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data) {
        dev_err(dev, "Failed to allocate driver data\n");
        return -ENOMEM;
    }

    data->dev = dev;
    data->buffer_size = BUFFER_SIZE;
    dev_set_drvdata(dev, data);

    // Allocate source and destination buffers
    data->src_buffer = devm_kmalloc(dev, data->buffer_size, GFP_KERNEL | GFP_DMA);
    if (!data->src_buffer) {
        dev_err(dev, "Failed to allocate source buffer\n");
        return -ENOMEM;
    }

    data->dst_buffer = devm_kmalloc(dev, data->buffer_size, GFP_KERNEL | GFP_DMA);
    if (!data->dst_buffer) {
        dev_err(dev, "Failed to allocate destination buffer\n");
        return -ENOMEM;
    }

    // Map buffers for DMA
    data->src_dma_addr = dma_map_single(dev, data->src_buffer, data->buffer_size, DMA_TO_DEVICE);
    if (dma_mapping_error(dev, data->src_dma_addr)) {
        dev_err(dev, "Failed to map source buffer for DMA\n");
        return -ENOMEM;
    }

    //msleep(6000);

    data->dst_dma_addr = dma_map_single(dev, data->dst_buffer, data->buffer_size, DMA_FROM_DEVICE);
    if (dma_mapping_error(dev, data->dst_dma_addr)) {
        dev_err(dev, "Failed to map destination buffer for DMA\n");
        dma_unmap_single(dev, data->src_dma_addr, data->buffer_size, DMA_TO_DEVICE);
        return -ENOMEM;
    }

    // Request DMA channel
    dma_cap_zero(mask);
    dma_cap_set(DMA_MEMCPY, mask);
    data->dma_chan = dma_request_channel(mask, NULL, "memcpy");
    if (!data->dma_chan) {
        dev_err(dev, "Failed to request DMA channel\n");
        ret = -ENODEV;
        goto unmap_buffers;
    }

    // Create debugfs entries
    data->debugfs_root = debugfs_create_dir("dma_driver", NULL);
    if (IS_ERR(data->debugfs_root)) {
        ret = PTR_ERR(data->debugfs_root);
        dev_err(dev, "Failed to create debugfs directory: %d\n", ret);
        goto release_channel;
    }

    debugfs_create_file("src_buffer", 0400, data->debugfs_root, data, &src_buffer_fops);
    debugfs_create_file("dst_buffer", 0400, data->debugfs_root, data, &dst_buffer_fops);
    debugfs_create_file("trigger_transfer", 0200, data->debugfs_root, data, &trigger_fops);

    dev_info(dev, "DMA driver initialized\n");
    return 0;

release_channel:
    dma_release_channel(data->dma_chan);
unmap_buffers:
    dma_unmap_single(dev, data->src_dma_addr, data->buffer_size, DMA_TO_DEVICE);
    dma_unmap_single(dev, data->dst_dma_addr, data->buffer_size, DMA_FROM_DEVICE);
    return ret;
}

static int dma_remove(struct platform_device *pdev)
{
    struct dma_data *data = dev_get_drvdata(&pdev->dev);

    // Clean up debugfs
    debugfs_remove_recursive(data->debugfs_root);

    // Release DMA channel
    if (data->dma_chan)
        dma_release_channel(data->dma_chan);

    // Unmap DMA buffers
    if (data->src_dma_addr)
        dma_unmap_single(data->dev, data->src_dma_addr, data->buffer_size, DMA_TO_DEVICE);
    if (data->dst_dma_addr)
        dma_unmap_single(data->dev, data->dst_dma_addr, data->buffer_size, DMA_FROM_DEVICE);

    dev_info(data->dev, "DMA driver removed\n");
    return 0;
}

static const struct of_device_id dma_of_match[] = {
    { .compatible = "memcpy-dma-transfer" },
    { }
};
MODULE_DEVICE_TABLE(of, dma_of_match);

static struct platform_driver dma_driver = {
    .probe  = dma_probe,
    .remove = dma_remove,
    .driver = {
        .name = "dma_driver",
        .of_match_table = dma_of_match,
    },
};
module_platform_driver(dma_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SovietUnion");
MODULE_DESCRIPTION("DMA Driver for AM335x EDMA Memory-to-Memory Transfers");