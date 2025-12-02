// test_ioremap_phys.c — copy-paste ready
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <net/page_pool.h>
#include <net/xdp.h>
#include <linux/fs.h> // alloc_chrdev_region
#include <linux/pci.h> // ioremap
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <linux/ethtool.h>

#define CPSW_BASE       0x4A100000
#define CPDMA_BASE      0x4A100800
#define CPDMA_DESC_RAM  0x4A102000

static int test_probe(struct platform_device *pdev)
{
    void __iomem *cpsw_vaddr;
    void __iomem *cpdma_vaddr;
    void __iomem *desc_vaddr;
    phys_addr_t cpsw_phys, cpdma_phys, desc_phys;

    printk(KERN_INFO "=== ioremap + virt_to_phys test ===\n");

    // 1. Map the three main regions
    cpsw_vaddr  = ioremap(CPSW_BASE,      0x1000);
    cpdma_vaddr = ioremap(CPDMA_BASE,     0x800);
    desc_vaddr  = ioremap(CPDMA_DESC_RAM, 8192);

    if (!cpsw_vaddr || !cpdma_vaddr || !desc_vaddr) {
        printk(KERN_ERR "ioremap failed!\n");
        return -ENOMEM;
    }

    // 2. Convert back to physical
    cpsw_phys  = virt_to_phys(cpsw_vaddr);
    cpdma_phys = virt_to_phys(cpdma_vaddr);
    desc_phys  = virt_to_phys(desc_vaddr);

    // 3. Print results
    printk(KERN_INFO "CPSW  virtual = %px  → physical = 0x%llx\n",
           cpsw_vaddr,  (u64)cpsw_phys);
    printk(KERN_INFO "CPDMA virtual = %px  → physical = 0x%llx\n",
           cpdma_vaddr, (u64)cpdma_phys);
    printk(KERN_INFO "DESC  virtual = %px  → physical = 0x%llx\n",
           desc_vaddr,  (u64)desc_phys);

    // 4. Bonus: test first descriptor address (what you write to HDP)
    void __iomem *first_desc = desc_vaddr;  // desc 0
    phys_addr_t first_desc_phys = virt_to_phys(first_desc);
    printk(KERN_INFO "First descriptor phys addr (for TX7_HDP) = 0x%llx\n",
           (u64)first_desc_phys);

    // 5. Optional: read a real register to prove mapping works
    u32 idver = ioread32(cpdma_vaddr + 0x00);  // CPDMA_TXIDVER
    printk(KERN_INFO "CPDMA IDVER register = 0x%08x\n", idver);

    // Clean up
    iounmap(cpsw_vaddr);
    iounmap(cpdma_vaddr);
    iounmap(desc_vaddr);

    return 0;
}

static int test_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "test_ioremap_phys removed\n");
    return 0;
}

static const struct of_device_id test_dt_ids[] = {
    { .compatible = "ether-based" },  // matches real CPSW node
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, test_dt_ids);

static struct platform_driver test_driver = {
    .probe  = test_probe,
    .remove = test_remove,
    .driver = {
        .name = "test-ioremap-phys",
        .of_match_table = test_dt_ids,
    },
};

module_platform_driver(test_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Test ioremap() → virt_to_phys() on BeagleBone Black CPSW");