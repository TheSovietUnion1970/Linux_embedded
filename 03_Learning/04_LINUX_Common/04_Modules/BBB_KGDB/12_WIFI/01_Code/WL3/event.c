// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include "wlcore.h"
#include "debug.h"
#include "io.h"
#include "event.h"
#include "ps.h"
#include "scan.h"
#include "wl12xx_80211.h"
//#include "hw_ops.h"

#include "common.h"
#include "wl18xx.h"
#include "ops.h"

#define WL18XX_LOGGER_SDIO_BUFF_MAX	(0x1020)
#define WL18XX_DATA_RAM_BASE_ADDRESS	(0x20000000)
#define WL18XX_LOGGER_SDIO_BUFF_ADDR	(0x40159c)
#define WL18XX_LOGGER_BUFF_OFFSET	(sizeof(struct fw_logger_information))
#define WL18XX_LOGGER_READ_POINT_OFFSET		(12)

void wlcore_event_rssi_trigger(struct wl1271 *wl, s8 *metric_arr)
{
	struct wl12xx_vif *wlvif;
	struct ieee80211_vif *vif;
	enum nl80211_cqm_rssi_threshold_event event;
	s8 metric = metric_arr[0];

	wl1271_debug(DEBUG_EVENT, "RSSI trigger metric: %d", metric);

	/* TODO: check actual multi-role support */
	wl12xx_for_each_wlvif_sta(wl, wlvif) {
		if (metric <= wlvif->rssi_thold)
			event = NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW;
		else
			event = NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH;

		vif = wl12xx_wlvif_to_vif(wlvif);
		if (event != wlvif->last_rssi_event)
			ieee80211_cqm_rssi_notify(vif, event, metric,
						  GFP_KERNEL);
		wlvif->last_rssi_event = event;
	}
}
EXPORT_SYMBOL_GPL(wlcore_event_rssi_trigger);

static void wl1271_stop_ba_event(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	struct ieee80211_vif *vif = wl12xx_wlvif_to_vif(wlvif);

	if (wlvif->bss_type != BSS_TYPE_AP_BSS) {
		u8 hlid = wlvif->sta.hlid;
		if (!VV_links[hlid].ba_bitmap)
			return;
		ieee80211_stop_rx_ba_session(vif, VV_links[hlid].ba_bitmap,
					     vif->bss_conf.bssid);
	} else {
		u8 hlid;
		for_each_set_bit(hlid, wlvif->ap.sta_hlid_map,
				 WL18XX_MAX_LINKS) {
			if (!VV_links[hlid].ba_bitmap)
				continue;

			ieee80211_stop_rx_ba_session(vif,
						     VV_links[hlid].ba_bitmap,
						     VV_links[hlid].addr);
		}
	}
}

void wlcore_event_soft_gemini_sense(struct wl1271 *wl, u8 enable)
{
	struct wl12xx_vif *wlvif;

	if (enable) {
		set_bit(WL1271_FLAG_SOFT_GEMINI, &wifi_data.flags);
	} else {
		clear_bit(WL1271_FLAG_SOFT_GEMINI, &wifi_data.flags);
		wl12xx_for_each_wlvif_sta(wl, wlvif) {
			wl1271_recalc_rx_streaming(wl, wlvif);
		}
	}
}
EXPORT_SYMBOL_GPL(wlcore_event_soft_gemini_sense);

void wlcore_event_sched_scan_completed(struct wl1271 *wl,
				       u8 status)
{
	wl1271_debug(DEBUG_EVENT, "PERIODIC_SCAN_COMPLETE_EVENT (status 0x%0x)",
		     status);

	printk("[EVENT] - wl->sched_vif: 0x%x\n", wl->sched_vif);
	if (wl->sched_vif) {
		ieee80211_sched_scan_stopped(wl->hw);
		wl->sched_vif = NULL;
	}
}
EXPORT_SYMBOL_GPL(wlcore_event_sched_scan_completed);

void wlcore_event_ba_rx_constraint(struct wl1271 *wl,
				   unsigned long roles_bitmap,
				   unsigned long allowed_bitmap)
{
	struct wl12xx_vif *wlvif;

	wl1271_debug(DEBUG_EVENT, "%s: roles=0x%lx allowed=0x%lx",
		     __func__, roles_bitmap, allowed_bitmap);

	wl12xx_for_each_wlvif(wl, wlvif) {
		if (wlvif->role_id == WL12XX_INVALID_ROLE_ID ||
		    !test_bit(wlvif->role_id , &roles_bitmap))
			continue;

		wlvif->ba_allowed = !!test_bit(wlvif->role_id,
					       &allowed_bitmap);
		if (!wlvif->ba_allowed)
			wl1271_stop_ba_event(wl, wlvif);
	}
}
EXPORT_SYMBOL_GPL(wlcore_event_ba_rx_constraint);

void wlcore_event_channel_switch(struct wl1271 *wl,
				 unsigned long roles_bitmap,
				 bool success)
{
	struct wl12xx_vif *wlvif;
	struct ieee80211_vif *vif;

	wl1271_debug(DEBUG_EVENT, "%s: roles=0x%lx success=%d",
		     __func__, roles_bitmap, success);

	wl12xx_for_each_wlvif(wl, wlvif) {
		if (wlvif->role_id == WL12XX_INVALID_ROLE_ID ||
		    !test_bit(wlvif->role_id , &roles_bitmap))
			continue;

		if (!test_and_clear_bit(WLVIF_FLAG_CS_PROGRESS,
					&wlvif->flags))
			continue;

		vif = wl12xx_wlvif_to_vif(wlvif);

		if (wlvif->bss_type == BSS_TYPE_STA_BSS) {
			ieee80211_chswitch_done(vif, success);
			// cancel_delayed_work(&wlvif->channel_switch_work);
		} else {
			set_bit(WLVIF_FLAG_BEACON_DISABLED, &wlvif->flags);
			ieee80211_csa_finish(vif);
		}
	}
}
EXPORT_SYMBOL_GPL(wlcore_event_channel_switch);

void wlcore_event_dummy_packet(struct wl1271 *wl)
{
	if (wl->plt) {
		wl1271_info("Got DUMMY_PACKET event in PLT mode.  FW bug, ignoring.");
		return;
	}

	wl1271_debug(DEBUG_EVENT, "DUMMY_PACKET_ID_EVENT_ID");
	wl1271_tx_dummy_packet(wl);
}
EXPORT_SYMBOL_GPL(wlcore_event_dummy_packet);

static void wlcore_disconnect_sta(struct wl1271 *wl, unsigned long sta_bitmap)
{
	u32 num_packets = wl->conf.tx.max_tx_retries;
	struct wl12xx_vif *wlvif;
	struct ieee80211_vif *vif;
	struct ieee80211_sta *sta;
	const u8 *addr;
	int h;

	for_each_set_bit(h, &sta_bitmap, WL18XX_MAX_LINKS) {
		bool found = false;
		/* find the ap vif connected to this sta */
		wl12xx_for_each_wlvif_ap(wl, wlvif) {
			if (!test_bit(h, wlvif->ap.sta_hlid_map))
				continue;
			found = true;
			break;
		}
		if (!found)
			continue;

		vif = wl12xx_wlvif_to_vif(wlvif);
		addr = VV_links[h].addr;

		rcu_read_lock();
		sta = ieee80211_find_sta(vif, addr);
		if (sta) {
			wl1271_debug(DEBUG_EVENT, "remove sta %d", h);
			ieee80211_report_low_ack(sta, num_packets);
		}
		rcu_read_unlock();
	}
}

void wlcore_event_max_tx_failure(struct wl1271 *wl, unsigned long sta_bitmap)
{
	wl1271_debug(DEBUG_EVENT, "MAX_TX_FAILURE_EVENT_ID");
	wlcore_disconnect_sta(wl, sta_bitmap);
}
EXPORT_SYMBOL_GPL(wlcore_event_max_tx_failure);

void wlcore_event_inactive_sta(struct wl1271 *wl, unsigned long sta_bitmap)
{
	wl1271_debug(DEBUG_EVENT, "INACTIVE_STA_EVENT_ID");
	wlcore_disconnect_sta(wl, sta_bitmap);
}
EXPORT_SYMBOL_GPL(wlcore_event_inactive_sta);

void wlcore_event_roc_complete(struct wl1271 *wl)
{
	wl1271_debug(DEBUG_EVENT, "REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID");
	printk("wl->roc_vif = 0x%x\n", wl->roc_vif);
	if (wl->roc_vif)
		ieee80211_ready_on_channel(wl->hw);
}
EXPORT_SYMBOL_GPL(wlcore_event_roc_complete);

void wlcore_event_beacon_loss(struct wl1271 *wl, unsigned long roles_bitmap)
{
	/*
	 * We are HW_MONITOR device. On beacon loss - queue
	 * connection loss work. Cancel it on REGAINED event.
	 */
	struct wl12xx_vif *wlvif;
	struct ieee80211_vif *vif;
	int delay = wl->conf.conn.synch_fail_thold *
				wl->conf.conn.bss_lose_timeout;

	wl1271_info("Beacon loss detected. roles:0x%lx", roles_bitmap);

	wl12xx_for_each_wlvif_sta(wl, wlvif) {
		if (wlvif->role_id == WL12XX_INVALID_ROLE_ID ||
		    !test_bit(wlvif->role_id , &roles_bitmap))
			continue;

		vif = wl12xx_wlvif_to_vif(wlvif);

		/* don't attempt roaming in case of p2p */
		if (wlvif->p2p) {
			ieee80211_connection_loss(vif);
			continue;
		}

		/*
		 * if the work is already queued, it should take place.
		 * We don't want to delay the connection loss
		 * indication any more.
		 */
		// ieee80211_queue_delayed_work(wl->hw,
		// 			     &wlvif->connection_loss_work,
		// 			     msecs_to_jiffies(delay));

		ieee80211_cqm_beacon_loss_notify(vif, GFP_KERNEL);
	}
}
EXPORT_SYMBOL_GPL(wlcore_event_beacon_loss);

int wl1271_event_unmask(struct wl1271 *wl)
{
	int ret;

	wl1271_debug(DEBUG_EVENT, "unmasking event_mask 0x%x", wl->event_mask);
	ret = wl1271_acx_event_mbox_mask(wl, ~(wl->event_mask));
	if (ret < 0)
		return ret;

	return 0;
}

// #include "../wl18xx/event.h"

enum wlcore_vendor_events {
	WLCORE_VENDOR_EVENT_SC_SYNC,
	WLCORE_VENDOR_EVENT_SC_DECODE,
};

enum wlcore_vendor_attributes {
	WLCORE_VENDOR_ATTR_FREQ,
	WLCORE_VENDOR_ATTR_PSK,
	WLCORE_VENDOR_ATTR_SSID,
	WLCORE_VENDOR_ATTR_GROUP_ID,
	WLCORE_VENDOR_ATTR_GROUP_KEY,

	NUM_WLCORE_VENDOR_ATTR,
	MAX_WLCORE_VENDOR_ATTR = NUM_WLCORE_VENDOR_ATTR - 1
};

static void VV_scan_completed(void)
{
	//wl->scan.failed = false;
	printk("VV_scan_completed\n");
	VV_scan_failed = false;
	cancel_delayed_work(&VV_work.scan_complete_work);
	ieee80211_queue_delayed_work(VV_work.hw, &VV_work.scan_complete_work,
				     msecs_to_jiffies(0));
}

#include <linux/bitops.h>
static int VV_process_mailbox_events(void)
{
	struct wl18xx_event_mailbox *mbox = wifi_data.mbox;
	u32 vector;

	vector = le32_to_cpu(mbox->events_vector);
	printk("[EVENTS] - MBOX vector: 0x%x, bit: %d", vector, fls(vector) - 1);

	// 0x100
	if (vector & SCAN_COMPLETE_EVENT_ID) {
		wl1271_debug(DEBUG_EVENT, "scan results: %d",
			     mbox->number_of_scan_results);

		// if (wl->scan_wlvif)
		// 	VV_scan_completed(wl, wl->scan_wlvif);
		if (VV_vif_ptr[0])
			VV_scan_completed();
	}

	// 0x40000
	if (vector & REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID)
		//wlcore_event_roc_complete(wl);
		printk("REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID\n");
	return 0;
}

#define WL18XX_INTR_TRIG_EVENT_ACK BIT(29)
static int VV_ack_event(void)
{
	// return wlcore_write_reg(wl, REG_INTERRUPT_TRIG,
	// 			WL18XX_INTR_TRIG_EVENT_ACK);
	return VV_sdio_raw_write(wlcore_translate_addr(wifi_data.rtable[REG_INTERRUPT_TRIG]), WL18XX_INTR_TRIG_EVENT_ACK, 4, false);
}

int wl1271_event_handle(u8 mbox_num)
{
	int ret;

	wl1271_debug(DEBUG_EVENT, "EVENT on mbox %d", mbox_num);

	if (mbox_num > 1)
		return -EINVAL;

	/* first we read the mbox descriptor */
	ret = VV_sdio_raw_read(wlcore_translate_addr(*wifi_data.mbox_ptr[mbox_num]), (u32*)wifi_data.mbox, sizeof(struct wl18xx_event_mailbox), false);
	if (ret < 0)
		return ret;

	/* process the descriptor */
	//ret = wl->ops->process_mailbox_events(wl);
	ret = VV_process_mailbox_events();
	if (ret < 0)
		return ret;

	/*
	 * TODO: we just need this because one bit is in a different
	 * place.  Is there any better way?
	 */
	//ret = wl->ops->ack_event(wl);
	ret = VV_ack_event();

	return ret;
}
