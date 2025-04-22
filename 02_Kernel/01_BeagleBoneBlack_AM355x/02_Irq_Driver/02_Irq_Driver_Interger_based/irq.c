#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#define DRIVER_AUTHOR "Vinh vdang3900@gmail.com"
#define DRIVER_DESC   "GPIO Interrupt Handler for LED using GPIO API"

#define BUTTON_GPIO  16    // GPIO1_16
#define LED_GPIO     19    // GPIO1_19

static int button_irq_number;
static bool led_on = false;
uint8_t count = 0;

/* Custom interrupt handler */
static irqreturn_t my_custom_irq_handler(int irq, void *dev_id)
{
    static unsigned long last_interrupt_time = 0;
    unsigned long current_time = jiffies;

    /* Debouncing: ignore interrupts within 50ms */
    if (time_before(current_time, last_interrupt_time + msecs_to_jiffies(100)))
        return IRQ_HANDLED;

    last_interrupt_time = current_time;

    pr_info("GPIO %d interrupt triggered, count = %u\n", BUTTON_GPIO, count);
    count++;

    /* Toggle LED */
    led_on = !led_on;
    gpio_set_value(LED_GPIO, led_on);

    return IRQ_HANDLED;
}

/* Constructor */
static int __init gpio_init(void)
{
    int ret;

    /* Request the LED GPIO */
    ret = gpio_request(LED_GPIO, "LED_GPIO");
    if (ret) {
        pr_err("Failed to request LED GPIO %d\n", LED_GPIO);
        return ret;
    }

    gpio_direction_output(LED_GPIO, 0);

    /* Request the BUTTON GPIO */
    ret = gpio_request(BUTTON_GPIO, "BUTTON_GPIO");
    if (ret) {
        pr_err("Failed to request BUTTON GPIO %d\n", BUTTON_GPIO);
        gpio_free(LED_GPIO);
        return ret;
    }

    gpio_direction_input(BUTTON_GPIO);

    /* Get IRQ number for BUTTON GPIO */
    button_irq_number = gpio_to_irq(BUTTON_GPIO);
    if (button_irq_number < 0) {
        pr_err("Failed to get IRQ number for GPIO %d\n", BUTTON_GPIO);
        gpio_free(LED_GPIO);
        gpio_free(BUTTON_GPIO);
        return button_irq_number;
    }

    /* Request IRQ */
    ret = request_irq(button_irq_number, my_custom_irq_handler,
                      IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, "button_gpio_irq", NULL);
    if (ret) {
        pr_err("Failed to request IRQ %d for GPIO %d\n", button_irq_number, BUTTON_GPIO);
        gpio_free(LED_GPIO);
        gpio_free(BUTTON_GPIO);
        return ret;
    }

    pr_info("GPIO interrupt module loaded, IRQ %d\n", button_irq_number);
    return 0;
}

/* Destructor */
static void __exit gpio_exit(void)
{
    free_irq(button_irq_number, NULL);
    gpio_set_value(LED_GPIO, 0);  // Turn off LED
    gpio_free(LED_GPIO);
    gpio_free(BUTTON_GPIO);
    pr_info("GPIO interrupt module unloaded\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION("1.0");
