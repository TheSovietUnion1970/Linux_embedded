#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/pinctrl/consumer.h>

struct irq_data_t {
    struct pinctrl *pinctrl;
    struct pinctrl_state *irq_state;
    int irq;
    struct work_struct re_request_work;
    struct device *dev;
};

static irqreturn_t irq_handler(int irq, void *dev_id);

static void re_request_irq_work(struct work_struct *work)
{
    struct irq_data_t *data = container_of(work, struct irq_data_t, re_request_work);
    struct device *dev = data->dev;
    int ret;

    if (data->irq >= 0){
        devm_free_irq(dev, data->irq, data);
        dev_info(dev, "Workqueue: free IRQ %d\n", data->irq);
    }

    dev_info(dev, "Workqueue: Attempting to re-request IRQ %d\n", data->irq);
    ret = devm_request_irq(dev, data->irq, irq_handler, IRQF_TRIGGER_RISING, "irq_driver", data);
    if (ret) {
        dev_err(dev, "Workqueue: Failed to re-request IRQ %d: %d\n", data->irq, ret);
    } else {
        dev_info(dev, "Workqueue: Successfully re-requested IRQ %d\n", data->irq);
    }
}

static irqreturn_t irq_handler(int irq, void *dev_id)
{
    struct irq_data_t *  = dev_id;

    dev_info(data->dev, "IRQ triggered on P9_15 (gpio1_16)\n");

    // Schedule work to re-request IRQ
    schedule_work(&data->re_request_work);

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

    // Get the IRQ number from the device tree
    data->irq = platform_get_irq(pdev, 0);
    if (data->irq < 0) {
        dev_err(dev, "Failed to get IRQ: %d\n", data->irq);
        return data->irq;
    }

    // Request the IRQ
    ret = devm_request_irq(dev, data->irq, irq_handler, IRQF_TRIGGER_RISING, "irq_driver", data);
    if (ret) {
        dev_err(dev, "Failed to request IRQ: %d\n", ret);
        return ret;
    }

    // Configure pinctrl for IRQ state
    data->pinctrl = devm_pinctrl_get(dev);
    if (IS_ERR(data->pinctrl)) {
        dev_err(dev, "Failed to get pinctrl: %ld\n", PTR_ERR(data->pinctrl));
        return PTR_ERR(data->pinctrl);
    }

    data->irq_state = pinctrl_lookup_state(data->pinctrl, "irq");
    if (IS_ERR(data->irq_state)) {
        dev_err(dev, "Failed to lookup irq state: %ld\n", PTR_ERR(data->irq_state));
        return PTR_ERR(data->irq_state);
    }

    ret = pinctrl_select_state(data->pinctrl, data->irq_state);
    if (ret) {
        dev_err(dev, "Failed to select irq state: %d\n", ret);
        return ret;
    }

    dev_info(dev, "IRQ driver initialized for P9_15 (gpio1_16)\n");
    return 0;
}

static int irq_remove(struct platform_device *pdev)
{
    struct irq_data_t *data = dev_get_drvdata(&pdev->dev);

    if (data->irq >= 0)
        devm_free_irq(&pdev->dev, data->irq, data);

    return 0;
}

static const struct of_device_id irq_of_match[] = {
    { .compatible = "gpio-descriptor-based" },
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