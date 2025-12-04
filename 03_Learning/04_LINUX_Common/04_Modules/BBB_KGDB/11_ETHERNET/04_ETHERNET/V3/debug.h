#ifndef DEBUG_H
#define DEBUG_H

#include "cpsw.h"

#define SS_EN 1
#define SS_DIS 0

#define HOST_EN 1
#define HOST_DIS 0

#define WR_EN 1
#define WR_DIS 0

#define SL_EN 1
#define SL_DIS 0

#define ALE_EN 1
#define ALE_DIS 0

#define CPDMA_EN 1
#define CPDMA_DIS 0

void Print_register_val_cpsw(struct ether_device_data *data,
                u8 ss_regs,
                u8 host_port_regs,
                u8 wr_regs,
                u8 slaves,
                u8 ale_regs,
                u8 cpdma_regs);

#endif /* DEBUG_H */