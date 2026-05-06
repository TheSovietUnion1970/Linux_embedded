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

int wlcore_event_fw_logger(struct wl1271 *wl)
{
	int ret;
	struct fw_logger_information fw_log;
	u8  *buffer;
	u32 internal_fw_addrbase = WL18XX_DATA_RAM_BASE_ADDRESS;
	u32 addr = WL18XX_LOGGER_SDIO_BUFF_ADDR;
	u32 addr_ptr;
	u32 buff_start_ptr;
	u32 buff_read_ptr;
	u32 buff_end_ptr;
	u32 available_len;
	u32 actual_len;
	u32 clear_ptr;
	size_t len;
	u32 start_loc;

	buffer = kzalloc(WL18XX_LOGGER_SDIO_BUFF_MAX, GFP_KERNEL);
	if (!buffer) {
		wl1271_error("Fail to allocate fw logger memory");
		actual_len = 0;
		goto out;
	}

	// ret = wlcore_read(wl, addr, buffer, WL18XX_LOGGER_SDIO_BUFF_MAX,
	// 		  false);
	ret = VV_sdio_raw_read(wlcore_translate_addr(addr), (u32*)buffer, WL18XX_LOGGER_SDIO_BUFF_MAX, false);
	if (ret < 0) {
		wl1271_error("Fail to read logger buffer, error_id = %d",
			     ret);
		actual_len = 0;
		goto free_out;
	}

	memcpy(&fw_log, buffer, sizeof(fw_log));

	actual_len = le32_to_cpu(fw_log.actual_buff_size);
	if (actual_len == 0)
		goto free_out;

	/* Calculate the internal pointer to the fwlog structure */
	addr_ptr = internal_fw_addrbase + addr;

	/* Calculate the internal pointers to the start and end of log buffer */
	buff_start_ptr = addr_ptr + WL18XX_LOGGER_BUFF_OFFSET;
	buff_end_ptr = buff_start_ptr + le32_to_cpu(fw_log.max_buff_size);

	/* Read the read pointer and validate it */
	buff_read_ptr = le32_to_cpu(fw_log.buff_read_ptr);
	if (buff_read_ptr < buff_start_ptr ||
	    buff_read_ptr >= buff_end_ptr) {
		wl1271_error("buffer read pointer out of bounds: %x not in (%x-%x)\n",
			     buff_read_ptr, buff_start_ptr, buff_end_ptr);
		goto free_out;
	}

	start_loc = buff_read_ptr - addr_ptr;
	available_len = buff_end_ptr - buff_read_ptr;

	/* Copy initial part up to the end of ring buffer */
	len = min(actual_len, available_len);
	wl12xx_copy_fwlog(wl, &buffer[start_loc], len);
	clear_ptr = addr_ptr + start_loc + actual_len;
	if (clear_ptr == buff_end_ptr)
		clear_ptr = buff_start_ptr;

	/* Copy any remaining part from beginning of ring buffer */
	len = actual_len - len;
	if (len) {
		wl12xx_copy_fwlog(wl,
				  &buffer[WL18XX_LOGGER_BUFF_OFFSET],
				  len);
		clear_ptr = addr_ptr + WL18XX_LOGGER_BUFF_OFFSET + len;
	}

	/* Update the read pointer */
	// ret = wlcore_write32(wl, addr + WL18XX_LOGGER_READ_POINT_OFFSET,
	// 		     clear_ptr);
	ret = VV_sdio_raw_write(wlcore_translate_addr(addr + WL18XX_LOGGER_READ_POINT_OFFSET), clear_ptr, 4, false);
free_out:
	kfree(buffer);
out:
	return actual_len;
}
EXPORT_SYMBOL_GPL(wlcore_event_fw_logger);

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
				 wl->num_links) {
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

	for_each_set_bit(h, &sta_bitmap, wl->num_links) {
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

static int VV_smart_config_sync_event(struct wl1271 *wl, u8 sync_channel,
					  u8 sync_band)
{
	struct sk_buff *skb;
	enum nl80211_band band;
	int freq;

	if (sync_band == WLCORE_BAND_5GHZ)
		band = NL80211_BAND_5GHZ;
	else
		band = NL80211_BAND_2GHZ;

	freq = ieee80211_channel_to_frequency(sync_channel, band);

	wl1271_debug(DEBUG_EVENT,
		     "SMART_CONFIG_SYNC_EVENT_ID, freq: %d (chan: %d band %d)",
		     freq, sync_channel, sync_band);
	skb = cfg80211_vendor_event_alloc(wl->hw->wiphy, NULL, 20,
					  WLCORE_VENDOR_EVENT_SC_SYNC,
					  GFP_KERNEL);

	if (nla_put_u32(skb, WLCORE_VENDOR_ATTR_FREQ, freq)) {
		kfree_skb(skb);
		return -EMSGSIZE;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	return 0;
}

static int VV_smart_config_decode_event(struct wl1271 *wl,
					    u8 ssid_len, u8 *ssid,
					    u8 pwd_len, u8 *pwd)
{
	struct sk_buff *skb;

	wl1271_debug(DEBUG_EVENT, "SMART_CONFIG_DECODE_EVENT_ID");
	wl1271_dump_ascii(DEBUG_EVENT, "SSID:", ssid, ssid_len);

	skb = cfg80211_vendor_event_alloc(wl->hw->wiphy, NULL,
					  ssid_len + pwd_len + 20,
					  WLCORE_VENDOR_EVENT_SC_DECODE,
					  GFP_KERNEL);

	if (nla_put(skb, WLCORE_VENDOR_ATTR_SSID, ssid_len, ssid) ||
	    nla_put(skb, WLCORE_VENDOR_ATTR_PSK, pwd_len, pwd)) {
		kfree_skb(skb);
		return -EMSGSIZE;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	return 0;
}

static const char *VV_radar_type_decode(u8 radar_type)
{
	switch (radar_type) {
	case RADAR_TYPE_REGULAR:
		return "REGULAR";
	case RADAR_TYPE_CHIRP:
		return "CHIRP";
	case RADAR_TYPE_NONE:
	default:
		return "N/A";
	}
}

static void VV_scan_completed(void)
{
	//wl->scan.failed = false;
	VV_scan_failed = false;
	cancel_delayed_work(&VV_work.scan_complete_work);
	ieee80211_queue_delayed_work(VV_work.hw, &VV_work.scan_complete_work,
				     msecs_to_jiffies(0));
}

static void VV_event_time_sync(struct wl1271 *wl,
				   u16 tsf_high_msb, u16 tsf_high_lsb,
				   u16 tsf_low_msb, u16 tsf_low_lsb)
{
	u32 clock_low;
	u32 clock_high;

	clock_high = (tsf_high_msb << 16) | tsf_high_lsb;
	clock_low = (tsf_low_msb << 16) | tsf_low_lsb;

	wl1271_info("TIME_SYNC_EVENT_ID: clock_high %u, clock low %u",
		    clock_high, clock_low);
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
