/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wl18xx
 *
 * Copyright (C) 2011 Texas Instruments
 */

#ifndef __WL18XX_IO_H__
#define __WL18XX_IO_H__

int __must_check wl18xx_top_reg_write(struct wl1271 *wl, int addr, u16 val);
int __must_check wl18xx_top_reg_read(struct wl1271 *wl, int addr, u16 *out);

// /* Temporary use */
// struct VV_partition {
// 	u32 size;
// 	u32 start;
// };

// struct VV_partition_set {
// 	struct VV_partition mem;
// 	struct VV_partition reg;
// 	struct VV_partition mem2;
// 	struct VV_partition mem3;
// };


/* Vinh custom */
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include "../wlcore/common.h"

int VV_sdio_raw_write(struct wl1271 *wl, int addr, u32 var, size_t len, bool fixed);
int VV_sdio_raw_write1(struct wl1271 *wl, int addr, void* var, size_t len, bool fixed);
int VV_sdio_raw_read(struct wl1271 *wl, int addr, u32* var, size_t len, bool fixed);
int wlcore_translate_addr(int addr);
int VV_set_partition_18(struct wl1271 *wl, const struct VV_partition_set *p);
void VV_sdio_set_block_size(struct wl1271 *wl, unsigned int blksz);

#endif /* __WL18XX_IO_H__ */
