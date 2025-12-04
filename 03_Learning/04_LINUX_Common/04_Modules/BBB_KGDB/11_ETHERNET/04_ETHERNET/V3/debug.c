#include "debug.h"

void Print_register_val_cpsw(struct ether_device_data *data,
                u8 ss_regs,
                u8 host_port_regs,
                u8 wr_regs,
                u8 slaves,
                u8 ale_regs,
                u8 cpdma_regs){
	u16 i = 0;

    if (ss_regs){
        printk("--- [ss regs] ---\n");
        for (i = 0; i < 13; i++){
            printk("# [%xh] = 0x%x\n", i*4, readl_relaxed((u8*)data->base_cpsw + i*4));
        }
        printk("--- [>>>>><<<<<] ---\n");
    }

    if (host_port_regs){
        printk("--- [host_port_regs] ---\n");
        for (i = 0; i < 16; i++){
            printk("# [%xh] = 0x%x\n", i*4, readl_relaxed((u8*)data->base_port0 + i*4));
        }
        for (i = 16; i < 34; i++){
            printk("# [%xh] = 0x%x\n", i*4, readl_relaxed((u8*)data->base_port1 + i*4));
        }
        printk("--- [>>>>><<<<<] ---\n");
    }

    if (wr_regs){
        printk("--- [wr_regs] ---\n");
        for (i = 0; i < 8; i++){
            printk("# [%xh] = 0x%x\n", i*4, readl_relaxed((u8*)data->base_wr + i*4));
        }
        printk("--- [>>>>><<<<<] ---\n");
    }

    if (slaves){
        printk("--- [slaves] ---\n");
        for (i = 0; i < 11; i++){
            printk("# [%xh] = 0x%x\n", i*4, readl_relaxed((u8*)data->base_cpsw_sl + i*4));
        }
        printk("--- [>>>>><<<<<] ---\n");
    }

    if (ale_regs){
        printk("--- [ale_regs] ---\n");
        for (i = 0; i < 19; i++){
            printk("# [%xh] = 0x%x\n", i*4, readl_relaxed((u8*)data->base_ale + i*4));
        }
        printk("--- [>>>>><<<<<] ---\n");
    }

    if (cpdma_regs){
        printk("--- [cpdma_regs] ---\n");
        for (i = 0; i < 49; i++){
            printk("# [%xh] = 0x%x\n", i*4, readl_relaxed((u8*)data->base_cpdma + i*4));
        }
        printk("--- [>>>>><<<<<] ---\n");
    }

}