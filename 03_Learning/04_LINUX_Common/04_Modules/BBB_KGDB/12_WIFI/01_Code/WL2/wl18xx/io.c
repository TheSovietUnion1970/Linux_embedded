// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl18xx
 *
 * Copyright (C) 2011 Texas Instruments
 */

#include "../wlcore/wlcore.h"
#include "../wlcore/io.h"

#include "io.h"

int wl18xx_top_reg_write(struct wl1271 *wl, int addr, u16 val)
{
	u32 tmp;
	int ret;

	if (WARN_ON(addr % 2))
		return -EINVAL;

	if ((addr % 4) == 0) {
		//ret = wlcore_read32(wl, addr, &tmp);
		ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, addr), &tmp, 4, false);
		if (ret < 0)
			goto out;

		tmp = (tmp & 0xffff0000) | val;
		//ret = wlcore_write32(wl, addr, tmp);
		ret = VV_sdio_raw_write(wl, wlcore_translate_addr(wl, addr), tmp, 4, false);
	} else {
		//ret = wlcore_read32(wl, addr - 2, &tmp);
		ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, addr - 2), &tmp, 4, false);
		if (ret < 0)
			goto out;

		tmp = (tmp & 0xffff) | (val << 16);
		//ret = wlcore_write32(wl, addr - 2, tmp);
		ret = VV_sdio_raw_write(wl, wlcore_translate_addr(wl, addr - 2), tmp, 4, false);
	}

out:
	return ret;
}

int wl18xx_top_reg_read(struct wl1271 *wl, int addr, u16 *out)
{
	u32 val = 0;
	int ret;

	if (WARN_ON(addr % 2))
		return -EINVAL;

	if ((addr % 4) == 0) {
		/* address is 4-bytes aligned */
		//ret = wlcore_read32(wl, addr, &val);
		ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, addr), &val, 4, false);
		if (ret >= 0 && out)
			*out = val & 0xffff;
	} else {
		//ret = wlcore_read32(wl, addr - 2, &val);
		ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, addr - 2), &val, 4, false);
		if (ret >= 0 && out)
			*out = (val & 0xffff0000) >> 16;
	}

	return ret;
}

/* Vinh custom */
int VV_sdio_raw_write(struct wl1271 *wl, int addr, u32 var, size_t len, bool fixed)
{
	int ret = 0;
	struct sdio_func *func = dev_to_sdio_func(wl->dev->parent);

	sdio_claim_host(func);

	// printk("sdio write 53 addr 0x%x, %zu bytes\n",
	// 	addr, len);

	if (fixed)
		ret = sdio_writesb(func, addr, &var, len);
	else
		ret = sdio_memcpy_toio(func, addr, &var, len);
	

	sdio_release_host(func);

	return ret;
}

int VV_sdio_raw_write1(struct wl1271 *wl, int addr, void* var, size_t len, bool fixed)
{
	int ret = 0;
	struct sdio_func *func = dev_to_sdio_func(wl->dev->parent);

	sdio_claim_host(func);

	// printk("sdio write 53 addr 0x%x, %zu bytes\n",
	// 	addr, len);

	if (fixed)
		ret = sdio_writesb(func, addr, var, len);
	else
		ret = sdio_memcpy_toio(func, addr, var, len);
	

	sdio_release_host(func);

	return ret;
}

int VV_sdio_raw_read(struct wl1271 *wl, int addr, u32* var, size_t len, bool fixed)
{
	int ret = 0;
	struct sdio_func *func = dev_to_sdio_func(wl->dev->parent);

	sdio_claim_host(func);

	// printk("sdio write 53 addr 0x%x, %zu bytes\n",
	// 	addr, len);

	if (fixed)
		ret = sdio_readsb(func, var, addr, len);
	else
		ret = sdio_memcpy_fromio(func, var, addr, len);
	

	sdio_release_host(func);

	return ret;
}

int VV_set_partition(struct wl1271 *wl, const struct wlcore_partition_set *p)
{
	int ret;

	/* copy partition info */
	memcpy(&wl->curr_part, p, sizeof(*p));

	ret = VV_sdio_raw_write(wl, HW_PART0_START_ADDR, p->mem.start, sizeof(p->mem.start), false);
	if (ret < 0)
		goto out;

	ret = VV_sdio_raw_write(wl, HW_PART0_SIZE_ADDR, p->mem.size, sizeof(p->mem.size), false);
	if (ret < 0)
		goto out;

	ret = VV_sdio_raw_write(wl, HW_PART1_START_ADDR, p->reg.start, sizeof(p->reg.start), false);
	if (ret < 0)
		goto out;

	ret = VV_sdio_raw_write(wl, HW_PART1_SIZE_ADDR, p->reg.size, sizeof(p->reg.size), false);
	if (ret < 0)
		goto out;

	ret = VV_sdio_raw_write(wl, HW_PART2_START_ADDR, p->mem2.start, sizeof(p->mem2.start), false);
	if (ret < 0)
		goto out;

	ret = VV_sdio_raw_write(wl, HW_PART2_SIZE_ADDR, p->mem2.size, sizeof(p->mem2.size), false);
	if (ret < 0)
		goto out;

	ret = VV_sdio_raw_write(wl, HW_PART3_START_ADDR, p->mem3.start, sizeof(p->mem3.start), false);
	if (ret < 0)
		goto out;

	ret = VV_sdio_raw_write(wl, HW_PART3_SIZE_ADDR, p->mem3.size, sizeof(p->mem3.size), false);
	if (ret < 0)
		goto out;

out:
	return ret;
}

void VV_sdio_set_block_size(struct wl1271 *wl, unsigned int blksz)
{
	struct sdio_func *func = dev_to_sdio_func(wl->dev->parent);

	sdio_claim_host(func);
	sdio_set_block_size(func, blksz);
	sdio_release_host(func);
}