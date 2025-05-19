#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/pwm.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>

#define PWM_PERIOD_NS 1000000 // 1ms = 1kHz
#define FADE_STEPS    100
#define STEP_DELAY_MS 20

struct led_pwm_data {
    struct pwm_device *pwm;
    struct gpio_desc *gpio_led;
};

static int fade_in_led(struct pwm_device *pwm)
{
    int duty, ret;

    ret = pwm_enable(pwm);
    if (ret) {
        pr_err("Failed to enable PWM: %d\n", ret);
        return ret;
    }

    pwm_config(pwm, 1000000, PWM_PERIOD_NS);
    printk("PWM -> max\n");
    msleep(2000);
    pwm_config(pwm, 0, PWM_PERIOD_NS);
    printk("PWM -> 0\n");
    msleep(2000);

    for (duty = 0; duty <= FADE_STEPS; duty++) {
        unsigned int duty_ns = (PWM_PERIOD_NS * duty) / FADE_STEPS;

        ret = pwm_config(pwm, duty_ns, PWM_PERIOD_NS);
        if (ret) {
            pr_err("Failed to config PWM: %d\n", ret);
            pwm_disable(pwm);
            return ret;
        }
        printk("PWM -> %d\n", duty_ns);
        msleep(STEP_DELAY_MS);
    }

    return 0;
}

static int led_pwm_probe(struct platform_device *pdev)
{
    struct led_pwm_data *data;
    struct device *dev = &pdev->dev;
    int ret;

    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    dev_set_drvdata(dev, data);

    // Request GPIO
    data->gpio_led = devm_gpiod_get(dev, "led30", GPIOD_OUT_HIGH);
    if (IS_ERR(data->gpio_led)) {
        dev_warn(dev, "No GPIO found, proceeding with PWM only\n");
        data->gpio_led = NULL;
    }

    // Request PWM
    data->pwm = devm_pwm_get(dev, "led50");
    if (IS_ERR(data->pwm)) {
        dev_err(dev, "Failed to get PWM: %ld\n", PTR_ERR(data->pwm));
        return PTR_ERR(data->pwm);
    }

    dev_info(dev, "Starting LED fade-in\n");
    ret = fade_in_led(data->pwm);
    if (ret) {
        dev_err(dev, "Failed to fade in LED: %d\n", ret);
        return ret;
    }

    return 0;
}

static int led_pwm_remove(struct platform_device *pdev)
{
    struct led_pwm_data *data = platform_get_drvdata(pdev);
    pwm_disable(data->pwm);
    return 0;
}

static const struct of_device_id led_pwm_of_match[] = {
    { .compatible = "gpio-descriptor-based" },
    { }
};
MODULE_DEVICE_TABLE(of, led_pwm_of_match);

static struct platform_driver led_pwm_driver = {
    .probe  = led_pwm_probe,
    .remove = led_pwm_remove,
    .driver = {
        .name = "led_pwm_driver",
        .of_match_table = led_pwm_of_match,
    },
};
module_platform_driver(led_pwm_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OpenAI + You");
MODULE_DESCRIPTION("PWM LED Brightness Driver");
