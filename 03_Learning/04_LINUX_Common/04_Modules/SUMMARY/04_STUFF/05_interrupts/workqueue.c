/**
 * workqueue_example.c - Simple Linux kernel module demonstrating Workqueue
 *
 * Compile & load same way as above
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>   // workqueue
#include <linux/jiffies.h>
#include <linux/delay.h>       // msleep (allowed here)

static struct workqueue_struct *my_wq;
static struct work_struct my_work;
int a = 0;

// Work function (runs in process context - can sleep)
static void my_work_func(struct work_struct *work)
{
    printk(KERN_INFO "Workqueue function started! jiffies = %lu, a = %d\n", jiffies, ++a);

    // You CAN sleep, take mutexes, allocate memory with GFP_KERNEL, etc.
    msleep(20000);   // sleep 1.5 seconds - allowed here!

    printk(KERN_INFO "Workqueue function finished! jiffies = %lu\n", jiffies);
}

// Module initialization
static int __init workqueue_example_init(void)
{
    printk(KERN_INFO "Workqueue module loaded, a = %d\n", ++a);

    // Create a dedicated workqueue (you can also use system_wq)
    my_wq = alloc_workqueue("my-example-wq", WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
    if (!my_wq) {
        printk(KERN_ERR "Failed to create workqueue\n");
        return -ENOMEM;
    }

    // Initialize work structure
    INIT_WORK(&my_work, my_work_func);

    // Queue the work
    queue_work(my_wq, &my_work);

    // You can queue it again later, multiple times, etc.
    // queue_work(my_wq, &my_work);

    return 0;
}

// Module cleanup
static void __exit workqueue_example_exit(void)
{
    // Wait for all pending works to complete
    destroy_workqueue(my_wq);

    printk(KERN_INFO "Workqueue module unloaded\n");
}

module_init(workqueue_example_init);
module_exit(workqueue_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Workqueue demonstration");