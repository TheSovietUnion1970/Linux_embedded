/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wl1271
 *
 * Copyright (C) 1998-2009 Texas Instruments. All rights reserved.
 * Copyright (C) 2008-2010 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#ifndef __IO_H__
#define __IO_H__

#include <linux/irqreturn.h>
#include "common.h"

#define HW_ACCESS_MEMORY_MAX_RANGE	0x1FFC0

#define HW_PARTITION_REGISTERS_ADDR     0x1FFC0
#define HW_PART0_SIZE_ADDR              (HW_PARTITION_REGISTERS_ADDR)
#define HW_PART0_START_ADDR             (HW_PARTITION_REGISTERS_ADDR + 4)
#define HW_PART1_SIZE_ADDR              (HW_PARTITION_REGISTERS_ADDR + 8)
#define HW_PART1_START_ADDR             (HW_PARTITION_REGISTERS_ADDR + 12)
#define HW_PART2_SIZE_ADDR              (HW_PARTITION_REGISTERS_ADDR + 16)
#define HW_PART2_START_ADDR             (HW_PARTITION_REGISTERS_ADDR + 20)
#define HW_PART3_SIZE_ADDR              (HW_PARTITION_REGISTERS_ADDR + 24)
#define HW_PART3_START_ADDR             (HW_PARTITION_REGISTERS_ADDR + 28)

#define HW_ACCESS_REGISTER_SIZE         4

#define HW_ACCESS_PRAM_MAX_RANGE	0x3c000

struct wl1271;

void wlcore_disable_interrupts(void);
void wlcore_disable_interrupts_nosync(void);
void wlcore_enable_interrupts(void);
void wlcore_synchronize_interrupts(void);

int wlcore_translate_addr(int addr);

static inline void wl1271_power_off(void)
{
	int ret = 0;

	if (!test_bit(WL1271_FLAG_GPIO_POWER, &wifi_data.flags))
		return;

	if (wifi_data.if_ops->power)
		ret = wifi_data.if_ops->power(wifi_data.dev, false);
	if (!ret)
		clear_bit(WL1271_FLAG_GPIO_POWER, &wifi_data.flags);
}

static inline int wl1271_power_on(struct wl1271 *wl)
{
	int ret = 0;

	if (wifi_data.if_ops->power)
		ret = wifi_data.if_ops->power(wifi_data.dev, true);
	if (ret == 0)
		set_bit(WL1271_FLAG_GPIO_POWER, &wifi_data.flags);

	return ret;
}

/* Functions from wl1271_main.c */

int wl1271_tx_dummy_packet(struct wl1271 *wl);

/* Vinh custom */
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include "common.h"
int VV_sdio_raw_write(int addr, u32 var, size_t len, bool fixed);
int VV_sdio_raw_write1(int addr, void* var, size_t len, bool fixed);
int VV_sdio_raw_read(int addr, u32* var, size_t len, bool fixed);
int wlcore_translate_addr(int addr);
int VV_set_partition_core(struct VV_partition_set *p);
void VV_sdio_set_block_size(unsigned int blksz);

#endif
