#ifndef E_H
#define E_H

#include <linux/module.h>
#include <linux/fs.h> // alloc_chrdev_region
#include <linux/pci.h> // ioremap
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/workqueue.h>

#define PHY_ID0 0
#define MDIO_TIMEOUT		100 /* msecs */

#define CTRMOD_BASE     0x44e10000   
#define CLK_BASE        0x44e00000

#define CPSW_BASE       0x4a100000   // CPSW subsystem base
#define MDIO_BASE       (CPSW_BASE + 0x1000)   // MDIO block = 0x4a101000
#define CPSW_WR_BASE    (CPSW_BASE + 0x1200)   // WRAPPER (if you need it)
#define PORT0_BASE      (CPSW_BASE + 0x100)    // Host port registers
#define PORT1_BASE      (CPSW_BASE + 0x200)    // Slave port 0 (eth0)
#define PORT2_BASE      (CPSW_BASE + 0x300)    // Slave port 1 (eth1)

/* MDIO command */
#define USERACCESS_GO		BIT(31)
#define USERACCESS_GO_BIT		31
#define USERACCESS_WRITE	BIT(30)
#define USERACCESS_ACK		BIT(29)
#define USERACCESS_ACK_BIT		29
#define USERACCESS_READ		(0)
#define USERACCESS_DATA		(0xffff)

/* MDIO register + offset */
#define MDIO_MDIOVER 0x00
#define MDIO_MDIOCONTROL 0x04
    #define MDIOC_PREAMBLE			1u << 20
    #define MDIOC_CLKDIV(div)		((div) & 0xff)
    #define MDIOC_ENABLE     		1u << 30
    #define CONTROL_IDLE            1u << 31
#define MDIO_MDIOALIVE 0x08
#define MDIO_MDIOUSERACCESS0 0x80
#define MDIO_MDIOUSERPHYSEL0 0x84
#define MDIO_MDIOUSERACCESS1 0x88
#define MDIO_MDIOUSERPHYSEL1 0x8c

struct ether_device_data {
    /* Essential */
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *dev;
    struct clk *clk;
    struct clk *clk2;
    struct clk *clk3;
    u32 clk_freq;

    /* Base address */
    void __iomem *base_mdio; 
    void __iomem *base_ctrmod; 
    void __iomem *base_clk; 
    
    /* scheduled work */
    struct work_struct re_request_work;
    bool is_scheduled;
    atomic_t should_stop; // Use atomic_t instead of bool

    /* "rx_thresh", "rx", "tx", "misc" */
    int rx_thresh_irq;
    int rx_irq;
    int tx_irq;
    int misc_irq;
};

int clock_init(struct ether_device_data *data);
int clock_deinit(struct ether_device_data *data);

void gmii_sel_init(struct ether_device_data *data);
int ether_mdio_init(struct ether_device_data* data);

#endif /* E_H */
