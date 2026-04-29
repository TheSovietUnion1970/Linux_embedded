// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl18xx
 *
 * Copyright (C) 2012 Texas Instruments. All rights reserved.
 */

#include <linux/ieee80211.h>
#include "scan.h"
#include "../wlcore/debug.h"

static int __wl18xx_scan_stop(struct wl1271 *wl, struct wl12xx_vif *wlvif,
			       u8 scan_type)
{
	struct wl18xx_cmd_scan_stop *stop;
	int ret;

	wl1271_debug(DEBUG_CMD, "cmd periodic scan stop");

	stop = kzalloc(sizeof(*stop), GFP_KERNEL);
	if (!stop) {
		wl1271_error("failed to alloc memory to send sched scan stop");
		return -ENOMEM;
	}

	stop->role_id = wlvif->role_id;
	stop->scan_type = scan_type;

	ret = wl1271_cmd_send(wl, CMD_STOP_SCAN, stop, sizeof(*stop), 0);
	if (ret < 0) {
		wl1271_error("failed to send sched scan stop command");
		goto out_free;
	}

out_free:
	kfree(stop);
	return ret;
}

void wl18xx_scan_sched_scan_stop(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	__wl18xx_scan_stop(wl, wlvif, SCAN_TYPE_PERIODIC);
}

int wl18xx_scan_stop(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	return __wl18xx_scan_stop(wl, wlvif, SCAN_TYPE_SEARCH);
}
