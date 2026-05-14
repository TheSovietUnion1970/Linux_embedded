// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009-2010 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include <linux/ieee80211.h>
#include <linux/pm_runtime.h>

#include "wlcore.h"
#include "debug.h"
#include "cmd.h"
#include "scan.h"
#include "acx.h"
#include "tx.h"
#include "ops.h"
#include "common.h"
#include "main.h"

void wifi_scan_complete_work(struct work_struct *work)
{
	struct cfg80211_scan_info info = {
		.aborted = false,
	};
	int ret;
#if (PRINT_DEBUG)
	printk("[WORK] Scan complete state: %d\n", wifi_data->scan_state);
#endif
	mutex_lock(&wifi_data->mutex);

	if (unlikely(wifi_data->state != WLCORE_STATE_ON))
		goto out;

	if (wifi_data->scan_state == WL1271_SCAN_STATE_IDLE)
		goto out;



	/*
	 * Rearm the tx watchdog just before idling scan. This
	 * prevents just-finished scans from triggering the watchdog
	 */
	wifi_rearm_tx_watchdog_locked();

	wifi_data->scan_state = WL1271_SCAN_STATE_IDLE;
	//memset(wifi_data->scan.scanned_ch, 0, sizeof(wifi_data->scan.scanned_ch));
	//wifi_data->scan.req = NULL;
	//wifi_data->scan_wifi_vif = NULL;

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out;
	}

	// if (wifi_data->scan.failed) {
	if (wifi_scan_failed) {
		wifi_error("Scan completed due to error.");
		wifi_error("wifi_queue_recovery_work -> SHOULD RESTART\n");
	}

	wificore_cmd_regdomain_config_locked();

	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);

	/*
	this function needs to be called by the driver to notify
	mac80211 that the scan finished. This function can be called from
	any context, including hardirq context.
	*/
	ieee80211_scan_completed(wifi_data->hw, &info); 

out:
	mutex_unlock(&wifi_data->mutex);

}

int wificore_scan(struct ieee80211_vif *vif,
		const u8 *ssid, size_t ssid_len,
		struct cfg80211_scan_request *req)
{
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);

	/*
	 * cfg80211 should guarantee that we don't get more channels
	 * than what we have registered.
	 */
	BUG_ON(req->n_channels > WL1271_MAX_CHANNELS);

	if (wifi_data->scan_state != WL1271_SCAN_STATE_IDLE)
		return -EBUSY;

	wifi_data->scan_state = WL1271_SCAN_STATE_2GHZ_ACTIVE;

	wifi_scan_failed = true;
	ieee80211_queue_delayed_work(wifi_data->hw, &wifi_work.scan_complete_work,
				     msecs_to_jiffies(WL1271_SCAN_TIMEOUT));
#if (PRINT_DEBUG_SCAN)
	printk("SCHEDULE SCAN - wifi_vif = 0x%x\n", wifi_vif);
#endif
	(void)wifi_scan_send(wifi_vif, req); 
	// Config cmd for CMD_SCAN, template for probe req if active[0], dtf, active[1] > 0
	

	return 0;
}


