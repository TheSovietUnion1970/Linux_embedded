#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/timer.h>

#define DRIVER_AUTHOR "Vinh vdang3900@gmail.com"
#define DRIVER_DESC   "Blinking LED with timer"

#define GPIO1_ADDR_BASE  0x4804C000
#define GPIO1_ADDR_SIZE  0x1000
#define GPIO_OE_OFFSET           0x134
#define GPIO_SETDATAOUT_OFFSET   0x194
#define GPIO_CLEARDATAOUT_OFFSET 0x190
#define GPIO1_19 (1 << 19)

static uint32_t __iomem *gpio1_base;
static struct timer_list blink_timer;
static bool led_on = false;

/* Timer callback */
static void blink_timer_callback(struct timer_list *timer)
{
    if (led_on) {
        *(gpio1_base + GPIO_CLEARDATAOUT_OFFSET/4) = GPIO1_19; // Turn OFF
    } else {
        *(gpio1_base + GPIO_SETDATAOUT_OFFSET/4) = GPIO1_19;   // Turn ON
    }
    led_on = !led_on;

    /* Re-arm the timer */
    mod_timer(&blink_timer, jiffies + msecs_to_jiffies(1000)); // 1s
}

/* Constructor */
static int __init gpio_init(void)
{
    gpio1_base = ioremap(GPIO1_ADDR_BASE, GPIO1_ADDR_SIZE);
    if (!gpio1_base) {
        pr_err("Failed to ioremap\n");
        return -ENOMEM;
    }

    /* Set GPIO1_19 as output */
    *(gpio1_base + GPIO_OE_OFFSET/4) &= ~GPIO1_19;

    /* Initialize timer */
    timer_setup(&blink_timer, blink_timer_callback, 0);
    mod_timer(&blink_timer, jiffies + msecs_to_jiffies(1000)); // Start after 1s

    pr_info("Blinking LED module loaded\n");
    return 0;
}

/* Destructor */
static void __exit gpio_exit(void)
{
    /* Turn off LED */
    *(gpio1_base + GPIO_CLEARDATAOUT_OFFSET/4) = GPIO1_19;

    /* Remove timer */
    del_timer_sync(&blink_timer);

    iounmap(gpio1_base);
    pr_info("Blinking LED module unloaded\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION("1.0");
