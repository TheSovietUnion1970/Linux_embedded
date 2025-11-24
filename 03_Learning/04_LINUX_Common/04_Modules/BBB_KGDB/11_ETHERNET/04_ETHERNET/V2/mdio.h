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

#define BIT_VAL_0 0
#define BIT_VAL_1 1

#define CTRMOD_BASE     0x44e10000   
#define CLK_BASE        0x44e00000

#define CPSW_BASE       0x4a100000   // CPSW subsystem base
#define PORT0_BASE      (CPSW_BASE + 0x108)    // Host port registers
#define PORT1_BASE      (CPSW_BASE + 0x200)    // Slave port 0 (eth0)
#define PORT2_BASE      (CPSW_BASE + 0x300)    // Slave port 1 (eth1)
#define STATS_BASE      (CPSW_BASE + 0x900)    // Statistics Registers
#define CPTS_BASE       (CPSW_BASE + 0xC00)    // Common Platform Time Sync
#define ALE_BASE        (CPSW_BASE + 0xD00)    // ALE
#define MDIO_BASE       (CPSW_BASE + 0x1000)   // MDIO block = 0x4a101000
#define CPSW_WR_BASE    (CPSW_BASE + 0x1200)   // WRAPPER (if you need it)

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

/* Generic MII registers. */
#define MII_BMCR		0x00	/* Basic mode control register */
#define MII_BMSR		0x01	/* Basic mode status register  */
#define MII_PHYSID1		0x02	/* PHYS ID 1                   */
#define MII_PHYSID2		0x03	/* PHYS ID 2                   */
#define MII_ADVERTISE		0x04	/* Advertisement control reg   */
#define MII_LAN83C185_ISF 29 /* Interrupt Source Flags */
#define MII_LAN83C185_IM  30 /* Interrupt Mask */
#define MII_LAN83C185_CTRL_STATUS 17 /* Mode/Status Register */
#define MII_LAN83C185_SPECIAL_MODES 18 /* Special Modes Register */

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
    void __iomem *base_ctrmod; 
    void __iomem *base_clk; 

    void __iomem *base_cpsw; 
    void __iomem *base_ale; 
    void __iomem *base_port0; 
    void __iomem *base_port1; 
    void __iomem *base_port2; 

    void __iomem *base_mdio; 
    
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

int wait_register_update(struct ether_device_data *data, void __iomem *mem, u16 reg_offset, u16 bit_offset, u8 bit_val, u16 delay_ms, u8* name_register);
int wait_val_update(struct ether_device_data *data, u16* var, u16 val, u16 delay_ms, u8* name_val);

int clock_init(struct ether_device_data *data);
int clock_deinit(struct ether_device_data *data);

void gmii_sel_init(struct ether_device_data *data);
int ether_mdio_init(struct ether_device_data* data);

#endif /* E_H */