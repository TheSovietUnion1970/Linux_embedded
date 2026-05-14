// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2010 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>

#include "wlcore.h"
#include "debug.h"
#include "wifi_80211.h"
#include "io.h"
#include "tx.h"

void wificore_disable_interrupts(void)
{
	disable_irq(wifi_data->irq);
}
EXPORT_SYMBOL_GPL(wificore_disable_interrupts);

void wificore_disable_interrupts_nosync(void)
{
	disable_irq_nosync(wifi_data->irq);
}
EXPORT_SYMBOL_GPL(wificore_disable_interrupts_nosync);

void wificore_enable_interrupts(void)
{
	enable_irq(wifi_data->irq);
}
EXPORT_SYMBOL_GPL(wificore_enable_interrupts);

void wificore_synchronize_interrupts(void)
{
	synchronize_irq(wifi_data->irq);
}
EXPORT_SYMBOL_GPL(wificore_synchronize_interrupts);

int wificore_translate_addr(int addr)
{
	// struct wificore_partition_set *part = &wifi_data->curr_part;
	struct wifi_partition_set *part = &wifi_data->curr_part;

	/*
	 * To translate, first check to which window of addresses the
	 * particular address belongs. Then subtract the starting address
	 * of that window from the address. Then, add offset of the
	 * translated region.
	 *
	 * The translated regions occur next to each other in physical device
	 * memory, so just add the sizes of the preceding address regions to
	 * get the offset to the new region.
	 */
	if ((addr >= part->mem.start) &&
	    (addr < part->mem.start + part->mem.size))
		return addr - part->mem.start;
	else if ((addr >= part->reg.start) &&
		 (addr < part->reg.start + part->reg.size))
		return addr - part->reg.start + part->mem.size;
	else if ((addr >= part->mem2.start) &&
		 (addr < part->mem2.start + part->mem2.size))
		return addr - part->mem2.start + part->mem.size +
			part->reg.size;
	else if ((addr >= part->mem3.start) &&
		 (addr < part->mem3.start + part->mem3.size))
		return addr - part->mem3.start + part->mem.size +
			part->reg.size + part->mem2.size;

	WARN(1, "HW address 0x%x out of range", addr);
	return 0;
}
EXPORT_SYMBOL_GPL(wificore_translate_addr);

/* Set the partitions to access the chip addresses
 *
 * To simplify driver code, a fixed (virtual) memory map is defined for
 * register and memory addresses. Because in the chipset, in different stages
 * of operation, those addresses will move around, an address translation
 * mechanism is required.
 *
 * There are four partitions (three memory and one register partition),
 * which are mapped to two different areas of the hardware memory.
 *
 *                                Virtual address
 *                                     space
 *
 *                                    |    |
 *                                 ...+----+--> mem.start
 *          Physical address    ...   |    |
 *               space       ...      |    | [PART_0]
 *                        ...         |    |
 *  00000000  <--+----+...         ...+----+--> mem.start + mem.size
 *               |    |         ...   |    |
 *               |MEM |      ...      |    |
 *               |    |   ...         |    |
 *  mem.size  <--+----+...            |    | {unused area)
 *               |    |   ...         |    |
 *               |REG |      ...      |    |
 *  mem.size     |    |         ...   |    |
 *      +     <--+----+...         ...+----+--> reg.start
 *  reg.size     |    |   ...         |    |
 *               |MEM2|      ...      |    | [PART_1]
 *               |    |         ...   |    |
 *                                 ...+----+--> reg.start + reg.size
 *                                    |    |
 *
 */

/* Vinh custom */
int wifi_sdio_raw_write(int addr, u32 var, size_t len, bool fixed)
{
	int ret = 0;
	struct sdio_func *func = dev_to_sdio_func(wifi_data->dev->parent);

	sdio_claim_host(func);

	if (fixed)
		ret = sdio_writesb(func, addr, &var, len);
	else
		ret = sdio_memcpy_toio(func, addr, &var, len);
	

	sdio_release_host(func);

	return ret;
}

int wifi_sdio_raw_write1(int addr, void* var, size_t len, bool fixed)
{
	int ret = 0;
	struct sdio_func *func = dev_to_sdio_func(wifi_data->dev->parent);

	sdio_claim_host(func);

	if (fixed)
		ret = sdio_writesb(func, addr, var, len);
	else
		ret = sdio_memcpy_toio(func, addr, var, len);
	

	sdio_release_host(func);

	return ret;
}

int wifi_sdio_raw_read(int addr, u32* var, size_t len, bool fixed)
{
	int ret = 0;
	struct sdio_func *func = dev_to_sdio_func(wifi_data->dev->parent);

	sdio_claim_host(func);

	if (fixed)
		ret = sdio_readsb(func, var, addr, len);
	else
		ret = sdio_memcpy_fromio(func, var, addr, len);
	

	sdio_release_host(func);

	return ret;
}

int wifi_set_partition_core(const struct wifi_partition_set *p)
{
	int ret;

	/* copy partition info */
	//memcpy(&wifi_data->curr_part, p, sizeof(*p));
	memcpy(&wifi_data->curr_part, p, sizeof(*p));

	ret = wifi_sdio_raw_write(HW_PART0_START_ADDR, p->mem.start, sizeof(p->mem.start), false);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_write(HW_PART0_SIZE_ADDR, p->mem.size, sizeof(p->mem.size), false);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_write(HW_PART1_START_ADDR, p->reg.start, sizeof(p->reg.start), false);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_write(HW_PART1_SIZE_ADDR, p->reg.size, sizeof(p->reg.size), false);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_write(HW_PART2_START_ADDR, p->mem2.start, sizeof(p->mem2.start), false);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_write(HW_PART2_SIZE_ADDR, p->mem2.size, sizeof(p->mem2.size), false);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_write(HW_PART3_START_ADDR, p->mem3.start, sizeof(p->mem3.start), false);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_write(HW_PART3_SIZE_ADDR, p->mem3.size, sizeof(p->mem3.size), false);
	if (ret < 0)
		goto out;

out:
	return ret;
}

void wifi_sdio_set_block_size(unsigned int blksz)
{
	struct sdio_func *func = dev_to_sdio_func(wifi_data->dev->parent);

	sdio_claim_host(func);
	sdio_set_block_size(func, blksz);
	sdio_release_host(func);
}

