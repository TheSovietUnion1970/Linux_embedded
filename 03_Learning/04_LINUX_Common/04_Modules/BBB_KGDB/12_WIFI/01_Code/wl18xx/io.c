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
		ret = wlcore_read32(wl, addr, &tmp);
		if (ret < 0)
			goto out;

		tmp = (tmp & 0xffff0000) | val;
		ret = wlcore_write32(wl, addr, tmp);
	} else {
		ret = wlcore_read32(wl, addr - 2, &tmp);
		if (ret < 0)
			goto out;

		tmp = (tmp & 0xffff) | (val << 16);
		ret = wlcore_write32(wl, addr - 2, tmp);
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
		ret = wlcore_read32(wl, addr, &val);
		if (ret >= 0 && out)
			*out = val & 0xffff;
	} else {
		ret = wlcore_read32(wl, addr - 2, &val);
		if (ret >= 0 && out)
			*out = (val & 0xffff0000) >> 16;
	}

	return ret;
}

int VV_top_write(struct wl1271 *wl, int addr, u16 val){
	u32 tmp;
	int ret;
	if ((addr % 4) == 0){
		ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, addr), &tmp, sizeof(tmp), false);
		if (ret < 0){
			return ret;
		}
		tmp = (tmp & 0xffff0000) | val;
		ret = VV_sdio_raw_write(wl, wlcore_translate_addr(wl, addr), tmp, sizeof(tmp), false);
	}
	else{
		ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, addr - 2), &tmp, sizeof(tmp), false);
		if (ret < 0){
			return ret;
		}
		tmp = (tmp & 0xffff) | (val << 16);
		ret = VV_sdio_raw_write(wl, wlcore_translate_addr(wl, addr - 2), tmp, sizeof(tmp), false);
	}
	return ret;
}

int VV_top_read(struct wl1271 *wl, int addr, u16* out){
	u32 val = 0;
	int ret;

	if ((addr % 4) == 0) {
		/* address is 4-bytes aligned */
		//ret = wlcore_read32(wl, addr, &val);
		ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, addr), &val, sizeof(val), false);
		if (ret >= 0 && out)
			*out = val & 0xffff;
	} else {
		ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, addr - 2), &val, sizeof(val), false);
		if (ret >= 0 && out)
			*out = (val & 0xffff0000) >> 16;
	}

	return ret;
}