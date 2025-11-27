// m1.c — Platform driver + thread started after 5-second delay using delayed_work

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/of_device.h>
#include <linux/delay.h>

#define DRIVER_NAME "ether0_driver"


#define START_DELAY_SECONDS 2
#define LOOP 2


u8 thread_created = 0;
u8 loop = 0;
struct platform_device *dbg_pdev;
struct delayed_work start_work;

static int ether_probe(struct platform_device *pdev);
static int ether_remove(struct platform_device *pdev);

/* This function runs ~5 seconds after probe */
static void start_thread_work(struct work_struct *work)
{
    printk(KERN_INFO "start_thread_work 1\n");
    while (!kthread_should_stop()) {
        printk(KERN_INFO "ether0_driver: tick - %lu\n", jiffies);

        // /* Your periodic work here */
        // ether_remove(dbg_pdev);
        // ether_probe(dbg_pdev);
        device_release_driver(&dbg_pdev->dev);
        device_attach(&dbg_pdev->dev);  // or driver_probe_device()
        msleep(200);

        if (loop == LOOP) break;

        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(HZ);   /* Sleep 1 second */

        loop++;
    }
    printk(KERN_INFO "start_thread_work 2\n");
}

static int ether_probe(struct platform_device *pdev)
{
    printk(KERN_INFO "ether_probe 1\n");

    if ((thread_created == 0) && (LOOP)){

        dbg_pdev = pdev;

        /* Schedule the thread to start after 5 seconds — NO sleeping here! */
        INIT_DELAYED_WORK(&start_work, start_thread_work);
        schedule_delayed_work(&start_work, START_DELAY_SECONDS * HZ);
        thread_created = 1;
    }

    printk(KERN_INFO "ether_probe 2\n");

    return 0;
}

static int ether_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "ether_remove 1\n");

    if ((loop > LOOP) && (LOOP)){
        /* Cancel pending delayed work if still queued */
        printk("cancel_delayed_work_sync is called\n");
        cancel_delayed_work_sync(&start_work);
    }

    printk(KERN_INFO "ether_remove 2\n");

    return 0;
}

/* Device tree matching */
static const struct of_device_id ether_of_match[] = {
    { .compatible = "ether-based" },
    { }
};
MODULE_DEVICE_TABLE(of, ether_of_match);

static struct platform_driver ether_driver = {
    .probe  = ether_probe,
    .remove = ether_remove,
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = ether_of_match,
    },
};

module_platform_driver(ether_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Platform driver with 1Hz thread started after 5-second delay using delayed_work");