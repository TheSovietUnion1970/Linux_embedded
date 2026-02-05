/**
 * tasklet_example.c - Simple Linux kernel module demonstrating Tasklet
 *
 * Compile:
 *   make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
 *
 * Load:   sudo insmod tasklet_example.ko
 * Unload: sudo rmmod tasklet_example
 */

#include <linux/module.h>      // module_init, module_exit
#include <linux/kernel.h>      // printk
#include <linux/interrupt.h>   // tasklet
#include <linux/jiffies.h>     // jiffies
#include <linux/delay.h>       // msleep
#include <linux/spinlock.h>
#include <linux/mutex.h>

// Our tasklet structure
static struct tasklet_struct my_tasklet;

// lock
spinlock_t lock;
unsigned long flags;

// mutex
struct mutex mutex_lock_t;

// Tasklet function (runs in softirq context - cannot sleep)
static void my_tasklet_func(unsigned long data)
{
    // spin_lock_irqsave(&lock, flags);
    mutex_lock(&mutex_lock_t);

    printk(KERN_INFO "Tasklet executed! data = %lu, jiffies = %lu\n", 
           data, jiffies);
    // msleep(100);

    // spin_unlock_irqrestore(&lock, flags);
    mutex_unlock(&mutex_lock_t);

    // You CANNOT use msleep(), mutex, or GFP_KERNEL allocation here
    // Only atomic operations, spinlocks, etc.

}

// Module initialization
static int __init tasklet_example_init(void)
{
    printk(KERN_INFO "Tasklet module loaded\n");

    // Initialize tasklet
    // Arguments: tasklet struct, function, data passed to function
    tasklet_init(&my_tasklet, my_tasklet_func, 2025);
    // spin_lock_init(&lock);
    mutex_init(&mutex_lock_t);

    // Schedule the tasklet (can be called from interrupt, timer, etc.)
    tasklet_schedule(&my_tasklet);

    msleep(1000);

    // You can also call it multiple times (it queues)
    tasklet_schedule(&my_tasklet);

    return 0;  // success
}

// Module cleanup
static void __exit tasklet_example_exit(void)
{
    // Make sure tasklet is not running / scheduled
    tasklet_kill(&my_tasklet);

    printk(KERN_INFO "Tasklet module unloaded\n");
}

module_init(tasklet_example_init);
module_exit(tasklet_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Tasklet demonstration");