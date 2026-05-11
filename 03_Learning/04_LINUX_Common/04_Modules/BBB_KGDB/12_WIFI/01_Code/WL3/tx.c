// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/etherdevice.h>
#include <linux/pm_runtime.h>
#include <linux/spinlock.h>

#include "wlcore.h"
#include "debug.h"
#include "io.h"
#include "ps.h"
#include "tx.h"
#include "event.h"
//#include "hw_ops.h"

#include "common.h"

#define WL18XX_NUM_TX_DESCRIPTORS 32

/*
 * TODO: this is here just for now, it must be removed when the data
 * operations are in place.
 */
#include "reg.h"

static int wl1271_alloc_tx_id(struct sk_buff *skb)
{
	int id;

	// id = find_first_zero_bit(wl->tx_frames_map, WL18XX_NUM_TX_DESCRIPTORS);
	id = find_first_zero_bit(VV_map.tx_frames_map, WL18XX_NUM_TX_DESCRIPTORS);
	if (id >= WL18XX_NUM_TX_DESCRIPTORS)
		return -EBUSY;

	// __set_bit(id, wl->tx_frames_map);
	__set_bit(id, VV_map.tx_frames_map);
	VV_skb_tx_frames[id] = skb;
	VV_skb_tx_frames_cnt++;
	return id;
}

void wl1271_free_tx_id(int id)
{
	// if (__test_and_clear_bit(id, wl->tx_frames_map)) {
	if (__test_and_clear_bit(id, VV_map.tx_frames_map)) {
		VV_skb_tx_frames[id] = NULL;
		VV_skb_tx_frames_cnt--;
	}
}
EXPORT_SYMBOL(wl1271_free_tx_id);

bool wl12xx_is_dummy_packet(struct sk_buff *skb)
{
	return VV_dummy_packet == skb;
}
EXPORT_SYMBOL(wl12xx_is_dummy_packet);

u8 wl12xx_tx_get_hlid(struct wl1271 *wl, struct wl12xx_vif *wlvif,
		      struct sk_buff *skb, struct ieee80211_sta *sta)
{
	struct ieee80211_tx_info *control;

	// if (wlvif->bss_type == BSS_TYPE_AP_BSS)
	// 	return wl12xx_tx_get_hlid_ap(wl, wlvif, skb, sta);

	control = IEEE80211_SKB_CB(skb);
	if (control->flags & IEEE80211_TX_CTL_TX_OFFCHAN) {
		wl1271_debug(DEBUG_TX, "tx offchannel");
		return wlvif->dev_hlid;
	}

	return wlvif->sta.hlid;
}

unsigned int wlcore_calc_packet_alignment(struct wl1271 *wl,
					  unsigned int packet_length)
{
	if ((wl->quirks & WLCORE_QUIRK_TX_PAD_LAST_FRAME) ||
	    !(wl->quirks & WLCORE_QUIRK_TX_BLOCKSIZE_ALIGN))
		{
			//printk("wlcore_calc_packet_alignment - IF\n");
			return ALIGN(packet_length, WL1271_TX_ALIGN_TO);
		}
	else{
		//printk("wlcore_calc_packet_alignment - ELSE\n");
			return ALIGN(packet_length, WL12XX_BUS_BLOCK_SIZE);
	}
}
EXPORT_SYMBOL(wlcore_calc_packet_alignment);

#define WL18XX_TX_HW_BLOCK_SPARE        1
/* for special cases - namely, TKIP and GEM */
#define WL18XX_TX_HW_EXTRA_BLOCK_SPARE  2
#define WL18XX_TX_HW_BLOCK_SIZE         268
#include "wl18xx.h"
static int wl1271_tx_allocate(struct sk_buff *skb, u32 buf_offset, u8 hlid)
{
	struct wl1271_tx_hw_descr *desc;
	u32 total_len = skb->len + sizeof(struct wl1271_tx_hw_descr);
	u32 total_blocks;
	int id, ret = -EBUSY, ac;
	u32 spare_blocks;

	if (buf_offset + total_len > WL18XX_AGGR_BUFFER_SIZE)
		return -EAGAIN;

	// spare_blocks = wlcore_hw_get_spare_blocks(wl, is_gem);
	//struct wl18xx_priv *priv = wl->priv;
	/* If we have keys requiring extra spare, indulge them */
	spare_blocks = WL18XX_TX_HW_BLOCK_SPARE;


	/* allocate free identifier for the packet */
	id = wl1271_alloc_tx_id(skb); // put skb into VV_skb_tx_frames[32], VV_skb_tx_frames_cnt++
	if (id < 0)
		return id;

	// total_blocks = wlcore_hw_calc_tx_blocks(wl, total_len, spare_blocks);
	u32 blk_size = WL18XX_TX_HW_BLOCK_SIZE;
	total_blocks = (total_len + blk_size - 1) / blk_size + spare_blocks;

	if (total_blocks <= VV_tx_blocks_available) {
		// Adds the TX descriptor at the front of the skb
		desc = skb_push(skb, total_len - skb->len); // len = sizeof(struct wl1271_tx_hw_descr) + extra

		// wlcore_hw_set_tx_desc_blocks(wl, desc, total_blocks,
		// 			     spare_blocks);
		desc->wl18xx_mem.total_mem_blocks = total_blocks;

		desc->id = id;

		VV_tx_blocks_available -= total_blocks;

		// VV_tx_allocated_blocks += total_blocks;
		VV_tx_allocated_blocks = VV_tx_allocated_blocks + total_blocks;

		/*
		 * If the FW was empty before, arm the Tx watchdog. Also do
		 * this on the first Tx after resume, as we always cancel the
		 * watchdog on suspend.
		 */
		// if VV_tx_allocated_blocks is zero BEFORE
		if (VV_tx_allocated_blocks == total_blocks ||
		    test_and_clear_bit(WL1271_FLAG_REINIT_TX_WDOG, &wifi_data.flags))
			wl12xx_rearm_tx_watchdog_locked();

		ac = wl1271_tx_get_queue(skb_get_queue_mapping(skb));
		VV_tx_allocated_pkts[ac]++;

		if (test_bit(hlid, VV_map.links_map))
			VV_allocated_pkts[hlid]++;

		ret = 0;

		wl1271_debug(DEBUG_TX,
			     "tx_allocate: size: %d, blocks: %d, id: %d",
			     total_len, total_blocks, id);
	} else {
		wl1271_free_tx_id(id);
	}

	return ret;
}

/* Indicates this TX HW frame is not padded to SDIO block size */
#define WL18XX_TX_CTRL_NOT_PADDED	BIT(7)
static void wl1271_tx_fill_hdr(struct sk_buff *skb,
			       struct ieee80211_tx_info *control, u8 hlid)
{
	struct wl1271_tx_hw_descr *desc;
	int ac, rate_idx;
	s64 hosttime;
	u16 tx_attr = 0;
	__le16 frame_control;
	struct ieee80211_hdr *hdr;
	u8 *frame_start;
	bool is_dummy;

	desc = (struct wl1271_tx_hw_descr *) skb->data;
	frame_start = (u8 *)(desc + 1); // frame_start points to the actual 802.11 frame.
	hdr = (struct ieee80211_hdr *)(frame_start);
	frame_control = hdr->frame_control;

	// printk("[CHECK] - extra = %d\n", extra);
	// /* relocate space for security header */
	// if (extra) {
	// 	int hdrlen = ieee80211_hdrlen(frame_control);
	// 	memmove(frame_start, hdr, hdrlen);
	// 	skb_set_network_header(skb, skb_network_offset(skb) + extra);
	// }

	/* configure packet life time */
	hosttime = (ktime_get_boottime_ns() >> 10);
	desc->start_time = cpu_to_le32(hosttime - VV_time_offset);

	
	is_dummy = wl12xx_is_dummy_packet(skb);
	//is_dummy = (wifi_data.VV_dummy_packet == skb);
	// if (is_dummy || !wlvif || wlvif->bss_type != BSS_TYPE_AP_BSS)
	// 	desc->life_time = cpu_to_le16(TX_HW_MGMT_PKT_LIFETIME_TU);
	// [TODO-wlvif]
	desc->life_time = cpu_to_le16(TX_HW_MGMT_PKT_LIFETIME_TU);

	/* queue */
	ac = wl1271_tx_get_queue(skb_get_queue_mapping(skb));
	desc->tid = skb->priority;

	u8 session_id = VV_session_ids[hlid];

	// if ((wl->quirks & WLCORE_QUIRK_AP_ZERO_SESSION_ID) &&
	// 	(wlvif->bss_type == BSS_TYPE_AP_BSS))
	// 	session_id = 0;

	/* configure the tx attributes */
	tx_attr = session_id << TX_HW_ATTR_OFST_SESSION_COUNTER;

	desc->hlid = hlid;
	/*
		* if the packets are data packets
		* send them with AP rate policies (EAPOLs are an exception),
		* otherwise use default basic rates
		*/
	if (skb->protocol == cpu_to_be16(ETH_P_PAE))
		// rate_idx = wlvif->sta.basic_rate_idx;
		rate_idx = STA_BASIC_RATE_IDX;
	else if (control->flags & IEEE80211_TX_CTL_NO_CCK_RATE)
		// rate_idx = wlvif->sta.p2p_rate_idx;
		rate_idx = STA_P2P_RATE_IDX;
	else if (ieee80211_is_data(frame_control))
		// rate_idx = wlvif->sta.ap_rate_idx;
		rate_idx = STA_AP_RATE_IDX;
	else
		// rate_idx = wlvif->sta.basic_rate_idx;
		rate_idx = STA_BASIC_RATE_IDX;

	tx_attr |= rate_idx << TX_HW_ATTR_OFST_RATE_POLICY;

	/* for WEP shared auth - no fw encryption is needed */
	if (ieee80211_is_auth(frame_control) &&
	    ieee80211_has_protected(frame_control))
		tx_attr |= TX_HW_ATTR_HOST_ENCRYPT;

	/* send EAPOL frames as voice */
	if (control->control.flags & IEEE80211_TX_CTRL_PORT_CTRL_PROTO)
		tx_attr |= TX_HW_ATTR_EAPOL_FRAME;

	desc->tx_attr = cpu_to_le16(tx_attr);

	//wlcore_hw_set_tx_desc_csum(wl, desc, skb);
	desc->wl18xx_checksum_data = 0;

	//wlcore_hw_set_tx_desc_data_len(wl, desc, skb);
	desc->length = cpu_to_le16(skb->len);

	/* if only the last frame is to be padded, we unset this bit on Tx */
	// if (wl->quirks & WLCORE_QUIRK_TX_PAD_LAST_FRAME){
	// 	printk("TX PADD - IF\n");
	// 	desc->wl18xx_mem.ctrl = WL18XX_TX_CTRL_NOT_PADDED;
	// }
	// else{
	// 	printk("TX PADD - ELSE\n");
	// 	desc->wl18xx_mem.ctrl = 0;	
	// }

	// if (wl->quirks & WLCORE_QUIRK_TX_PAD_LAST_FRAME)
	desc->wl18xx_mem.ctrl = WL18XX_TX_CTRL_NOT_PADDED;
}

/* caller must hold wifi_data.mutex */
static int wl1271_prepare_tx_frame(struct sk_buff *skb, u32 buf_offset, u8 hlid)
{
	struct ieee80211_tx_info *info;
	int ret = 0;
	u32 total_len;
	// bool is_dummy;
	//bool is_gem = false;

	// skb is taken from VV_skb_dequeue1

	if (!skb) {
		wl1271_error("discarding null skb");
		return -EINVAL;
	}

	if (hlid == WL12XX_INVALID_LINK_ID) {
		wl1271_error("invalid hlid. dropping skb 0x%p", skb);
		return -EINVAL;
	}

	info = IEEE80211_SKB_CB(skb);

	ret = wl1271_tx_allocate(skb, buf_offset, hlid);
	if (ret < 0)
		return ret;

	wl1271_tx_fill_hdr(skb, info, hlid);

	/*
	 * The length of each packet is stored in terms of
	 * words. Thus, we must pad the skb data to make sure its
	 * length is aligned.  The number of padding bytes is computed
	 * and set in wl1271_tx_fill_hdr.
	 * In special cases, we want to align to a specific block size
	 * (eg. for wl128x with SDIO we align to 256).
	 */
	//total_len = wlcore_calc_packet_alignment(wl, skb->len);
	total_len = ALIGN(skb->len, WL1271_TX_ALIGN_TO);

	memcpy(VV_aggr_buf + buf_offset, skb->data, skb->len);
	memset(VV_aggr_buf + buf_offset + skb->len, 0, total_len - skb->len);

	// /* Revert side effects in the dummy packet skb, so it can be reused */
	// if (is_dummy)
	// 	skb_pull(skb, sizeof(struct wl1271_tx_hw_descr));

	return total_len;
}

u32 wl1271_tx_enabled_rates_get(struct wl1271 *wl, u32 rate_set,
				enum nl80211_band rate_band)
{
	struct ieee80211_supported_band *band;
	u32 enabled_rates = 0;
	int bit;

	band = wl->hw->wiphy->bands[rate_band];
	for (bit = 0; bit < band->n_bitrates; bit++) {
		if (rate_set & 0x1)
			enabled_rates |= band->bitrates[bit].hw_value;
		rate_set >>= 1;
	}

	/* MCS rates indication are on bits 16 - 31 */
	rate_set >>= HW_HT_RATES_OFFSET - band->n_bitrates;

	for (bit = 0; bit < 16; bit++) {
		if (rate_set & 0x1)
			enabled_rates |= (CONF_HW_BIT_RATE_MCS_0 << bit);
		rate_set >>= 1;
	}

	//printk("rate_set = 0x%x, enabled_rates = 0x%x, band->n_bitrates = 0x%x\n", rate_set, enabled_rates, band->n_bitrates);

	return enabled_rates;
}

static int wlcore_select_ac(void)
{
	int i, q = -1, ac;
	u32 min_pkts = 0xffffffff;

	/*
	 * Find a non-empty ac where:
	 * 1. There are packets to transmit
	 * 2. The FW has the least allocated blocks
	 *
	 * We prioritize the ACs according to VO>VI>BE>BK
	 */
	for (i = 0; i < NUM_TX_QUEUES; i++) {
		ac = wl1271_tx_get_queue(i);
		//if (wl->tx_queue_count[ac] &&
		if (VV_tx_queue_count[ac] &&
		    // wl->tx_allocated_pkts[ac] < min_pkts) {
			VV_tx_allocated_pkts[ac] < min_pkts) {
			q = ac;
			// min_pkts = wl->tx_allocated_pkts[q];
			min_pkts = VV_tx_allocated_pkts[q];
		}
	}

	return q;
}

static struct sk_buff *wlcore_lnk_dequeue(u8 hlid, u8 q)
{
	struct sk_buff *skb;
	unsigned long flags;

	skb = skb_dequeue(&VV_tx_queue[hlid][q]);
	printk("[3] - skb = 0x%x\n", skb);
	if (skb) {
		spin_lock_irqsave(&wifi_data.lock, flags);
		VV_tx_queue_count[q]--;
		spin_unlock_irqrestore(&wifi_data.lock, flags);
	}

	return skb;
}

static bool VV_lnk_high_prio(u8 hlid)
{
	u8 thold;
	unsigned long suspend_bitmap = 0;

	if (test_bit(hlid, &suspend_bitmap))
		return false; // false if using default hlink 0

	/* the priority thresholds are taken from FW */
	// if (test_bit(hlid, &wl->fw_fast_lnk_map) &&
	//     !test_bit(hlid, &wl->ap_fw_ps_map))
	if (test_bit(hlid, (unsigned long*)&VV_status_reg->link_fast_bitmap))
		thold = VV_status_reg->tx_fast_link_prio_threshold;
	else
		thold = VV_status_reg->tx_slow_link_prio_threshold;
	return VV_allocated_pkts[hlid] < thold;
}

static bool VV_lnk_low_prio(u8 hlid)
{
	u8 thold;
	unsigned long suspend_bitmap;

	suspend_bitmap = le32_to_cpu(VV_status_reg->link_suspend_bitmap);
	//printk("L - suspend_bitmap = 0x%x\n", suspend_bitmap);

	if (test_bit(hlid, &suspend_bitmap))
		thold = VV_status_reg->tx_suspend_threshold;
	else if (test_bit(hlid, (unsigned long*)&VV_status_reg->link_fast_bitmap))
		thold = VV_status_reg->tx_fast_stop_threshold;
	else
		thold = VV_status_reg->tx_slow_stop_threshold;


	return VV_allocated_pkts[hlid] < thold;
}

int test = 0;
static struct sk_buff *VV_skb_dequeue1(void)
{
	unsigned long flags;
	struct sk_buff *skb = NULL;
	int ac;
	int i;
	u8 low_prio_hlid = WL12XX_INVALID_LINK_ID;

	// Find ac has data (the least allocated blks and V0>VI>...)
	ac = wlcore_select_ac();
	if (ac < 0){
		//printk("FAILED - ac\n");
		return skb;
	}

	/* Do a new pass over the wlvif list. But no need to continue
	 * after last_wlvif. The previous pass should have found it. */
	if (!skb) {
		//printk("[0] - wlvif = 0x%x\n", wlvif);
		//wl12xx_for_each_wlvif(wl, wlvif) {
		for (i = 0; i < VV_vif_ptr_id; i++){
			//printk("[1] - START LOOP\n");
			if (!VV_lnk_high_prio(HW_LINK_ID)) {
				if (low_prio_hlid == WL12XX_INVALID_LINK_ID &&
					!skb_queue_empty(&VV_tx_queue[HW_LINK_ID][ac]) &&
					VV_lnk_low_prio(HW_LINK_ID)) // wl18xx_lnk_low_prio
					/* we found the first non-empty low priority queue */
					low_prio_hlid = HW_LINK_ID;

				skb = NULL;
			}
			// this case for high priority
			else skb = wlcore_lnk_dequeue(HW_LINK_ID, ac);

			VV_vif_ptr[i]->last_tx_hlid = HW_LINK_ID;

	// // test
	// if (!test) skb = NULL;
	// test = 1;

			if (skb) {
				break;
			}
		}
	}

	printk("[2] - skb = 0x%x, VV_vif_ptr[%d] = 0x%x, low_prio_hlid = %x\n", skb, i, VV_vif_ptr[i], low_prio_hlid);
	return skb;
}

static bool wl1271_tx_is_data_present(struct sk_buff *skb)
{
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)(skb->data);

	return ieee80211_is_data_present(hdr->frame_control);
}

/*
 * Returns failure values only in case of failed bus ops within this function.
 * wl1271_prepare_tx_frame retvals won't be returned in order to avoid
 * triggering recovery by higher layers when not necessary.
 * In case a FW command fails within wl1271_prepare_tx_frame fails a recovery
 * will be queued in wl1271_cmd_send. -EAGAIN/-EBUSY from prepare_tx_frame
 * can occur and are legitimate so don't propagate. -EINVAL will emit a WARNING
 * within prepare_tx_frame code but there's nothing we should do about those
 * as well.
 */
/* Indicates this TX HW frame is not padded to SDIO block size */
#define WL18XX_TX_CTRL_NOT_PADDED	BIT(7)
int wlcore_tx_work_locked(void)
{
	struct wl12xx_vif *wlvif;
	struct sk_buff *skb;
	struct wl1271_tx_hw_descr *desc;
	u32 buf_offset = 0, last_len = 0;
	bool sent_packets = false;
	unsigned long active_hlids[BITS_TO_LONGS(WLCORE_MAX_LINKS)] = {0};
	//int ret = 0;
	int bus_ret = 0;
	u8 hlid = HW_LINK_ID;

	// while ((skb = wl1271_skb_dequeue(wl, &hlid))) {
	while ((skb = VV_skb_dequeue1())) {
		struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
		bool has_data = false;

		// wlvif = NULL;
		// if (!wl12xx_is_dummy_packet(wl, skb))
		// 	wlvif = wl12xx_vif_to_data(info->control.vif);
		// else
		// 	hlid = wl->system_hlid;
		wlvif = wl12xx_vif_to_data(info->control.vif);
		// [TOTO-wlvif]
		if (wlvif != VV_vif_ptr[0]){
			printk("WARNING DIFF in wlvif\n");
		}

		has_data = wl1271_tx_is_data_present(skb);
		last_len = wl1271_prepare_tx_frame(skb, buf_offset,
					      hlid);

		printk("wl1271_prepare_tx_frame->wlvif = 0x%x\n", wlvif);

		buf_offset += last_len;
		VV_tx_packets_count++;
		if (has_data) {
			desc = (struct wl1271_tx_hw_descr *) skb->data;
			__set_bit(desc->hlid, active_hlids);
		}
	}

	if (buf_offset) {
		// buf_offset = wlcore_hw_pre_pkt_send(wl, buf_offset, last_len);
		//if (wl->quirks & WLCORE_QUIRK_TX_PAD_LAST_FRAME) 
		struct wl1271_tx_hw_descr *last_desc;

		/* get the last TX HW descriptor written to the aggr buf */
		last_desc = (struct wl1271_tx_hw_descr *)(VV_aggr_buf +
							buf_offset - last_len);

		/* the last frame is padded up to an SDIO block */
		last_desc->wl18xx_mem.ctrl &= ~WL18XX_TX_CTRL_NOT_PADDED;
		buf_offset = ALIGN(buf_offset, WL12XX_BUS_BLOCK_SIZE);


		// REG_SLV_MEM_DATA → the address in the firmware’s memory where TX data should be written.
		// bus_ret = wlcore_write_data(wl, REG_SLV_MEM_DATA, VV_aggr_buf,
		// 			     buf_offset, true);
		bus_ret = VV_sdio_raw_write1(wlcore_translate_addr(wifi_data.rtable[REG_SLV_MEM_DATA]), VV_aggr_buf, buf_offset, true);
		if (bus_ret < 0)
			goto out;

		sent_packets = true;
	}
	if (sent_packets) {
		/*
		 * Interrupt the firmware with the new packets. This is only
		 * required for older hardware revisions
		 */
		// if (wl->quirks & WLCORE_QUIRK_END_OF_TRANSACTION) {
		// 	printk("WLCORE_QUIRK_END_OF_TRANSACTION\n");
		// 	// bus_ret = wlcore_write32(wl, WL12XX_HOST_WR_ACCESS,
		// 	// 		     VV_tx_packets_count);
		// 	bus_ret = VV_sdio_raw_write(wlcore_translate_addr(WL12XX_HOST_WR_ACCESS), VV_tx_packets_count, 4, false);
		// 	if (bus_ret < 0)
		// 		goto out;
		// }

		//wl1271_handle_tx_low_watermark(wl);
	}
	// Feature that improves bidirectional throughput
	//wl12xx_rearm_rx_streaming(wl, active_hlids);

out:
	return bus_ret;
}

void wl1271_tx_work(struct work_struct *work)
{
	//printk("[WORK] - wl1271_tx_work\n");
	// struct wl1271 *wl = container_of(work, struct wl1271, tx_work);
	int ret;

	mutex_lock(&wifi_data.mutex);
	ret = pm_runtime_get_sync(wifi_data.wl->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data.wl->dev);
		goto out;
	}

	ret = wlcore_tx_work_locked();
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data.wl->dev);
		wl12xx_queue_recovery_work(wifi_data.wl);
		goto out;
	}

	pm_runtime_mark_last_busy(wifi_data.wl->dev);
	pm_runtime_put_autosuspend(wifi_data.wl->dev);
out:
	mutex_unlock(&wifi_data.mutex);
}

void wl1271_tx_reset_link_queues(struct wl1271 *wl, u8 hlid)
{
	struct sk_buff *skb;
	int i;
	unsigned long flags;
	struct ieee80211_tx_info *info;
	int total[NUM_TX_QUEUES];

	for (i = 0; i < NUM_TX_QUEUES; i++) {
		total[i] = 0;
		while ((skb = skb_dequeue(&VV_tx_queue[hlid][i]))) {
			printk("TX_QUEUE - wl1271_tx_reset_link_queues\n");

			if (!wl12xx_is_dummy_packet(skb)) {
				info = IEEE80211_SKB_CB(skb);
				info->status.rates[0].idx = -1;
				info->status.rates[0].count = 0;
				ieee80211_tx_status_ni(wl->hw, skb);
			}

			total[i]++;
		}
	}

	spin_lock_irqsave(&wifi_data.lock, flags);
	for (i = 0; i < NUM_TX_QUEUES; i++) {
		//wl->tx_queue_count[i] -= total[i];
		VV_tx_queue_count[i] -= total[i];
	}
	spin_unlock_irqrestore(&wifi_data.lock, flags);

	//wl1271_handle_tx_low_watermark(wl);
}

/* caller must hold wifi_data.mutex and TX must be stopped */
void wl12xx_tx_reset_wlvif(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	int i;

	/* TX failure */
	for_each_set_bit(i, wlvif->links_map, WL18XX_MAX_LINKS) {
		if (wlvif->bss_type == BSS_TYPE_AP_BSS &&
		    i != wlvif->ap.bcast_hlid && i != wlvif->ap.global_hlid) {
			/* this calls wl12xx_free_link */
			wl1271_free_sta(wl, wlvif, i);
		} else {
			u8 hlid = i;
			wl12xx_free_link(wl, wlvif, &hlid);
		}
	}

	wlvif->last_tx_hlid = 0;

	// for (i = 0; i < NUM_TX_QUEUES; i++)
	// 	wlvif->tx_queue_count[i] = 0;
}
/* caller must hold wifi_data.mutex and TX must be stopped */
void wl12xx_tx_reset(struct wl1271 *wl)
{
	int i;
	struct sk_buff *skb;
	struct ieee80211_tx_info *info;

	/* only reset the queues if something bad happened */
	if (wl1271_tx_total_queue_count() != 0) {
		for (i = 0; i < WL18XX_MAX_LINKS; i++)
			wl1271_tx_reset_link_queues(wl, i);

		for (i = 0; i < NUM_TX_QUEUES; i++)
			//wl->tx_queue_count[i] = 0;
			VV_tx_queue_count[i] = 0;
	}

	/*
	 * Make sure the driver is at a consistent state, in case this
	 * function is called from a context other than interface removal.
	 * This call will always wake the TX queues.
	 */
	//wl1271_handle_tx_low_watermark(wl);

	for (i = 0; i < WL18XX_NUM_TX_DESCRIPTORS; i++) {
		if (VV_skb_tx_frames[i] == NULL)
			continue;

		skb = VV_skb_tx_frames[i];
		wl1271_free_tx_id(i);
		wl1271_debug(DEBUG_TX, "freeing skb 0x%p", skb);

		if (!wl12xx_is_dummy_packet(skb)) {
			/*
			 * Remove private headers before passing the skb to
			 * mac80211
			 */
			info = IEEE80211_SKB_CB(skb);
			skb_pull(skb, sizeof(struct wl1271_tx_hw_descr));
			if ((wl->quirks & WLCORE_QUIRK_TKIP_HEADER_SPACE) &&
			    info->control.hw_key &&
			    info->control.hw_key->cipher ==
			    WLAN_CIPHER_SUITE_TKIP) {
				int hdrlen = ieee80211_get_hdrlen_from_skb(skb);
				memmove(skb->data + WL1271_EXTRA_SPACE_TKIP,
					skb->data, hdrlen);
				skb_pull(skb, WL1271_EXTRA_SPACE_TKIP);
			}

			info->status.rates[0].idx = -1;
			info->status.rates[0].count = 0;

			ieee80211_tx_status_ni(wl->hw, skb);
		}
	}
}

#define WL1271_TX_FLUSH_TIMEOUT 500000

/* caller must *NOT* hold wifi_data.mutex */
void wl1271_tx_flush(struct wl1271 *wl)
{
	unsigned long timeout, start_time;
	int i;
	start_time = jiffies;
	timeout = start_time + usecs_to_jiffies(WL1271_TX_FLUSH_TIMEOUT);

	/* only one flush should be in progress, for consistent queue state */
	mutex_lock(&wl->flush_mutex);

	mutex_lock(&wifi_data.mutex);
	if (VV_skb_tx_frames_cnt == 0 && wl1271_tx_total_queue_count() == 0) {
		mutex_unlock(&wifi_data.mutex);
		goto out;
	}

	wlcore_stop_queues(wl, WLCORE_QUEUE_STOP_REASON_FLUSH);

	while (!time_after(jiffies, timeout)) {
		wl1271_debug(DEBUG_MAC80211, "flushing tx buffer: %d %d",
			     VV_skb_tx_frames_cnt,
			     wl1271_tx_total_queue_count());

		/* force Tx and give the driver some time to flush data */
		mutex_unlock(&wifi_data.mutex);
		if (wl1271_tx_total_queue_count())
			wl1271_tx_work(&VV_work.tx_work);
		msleep(20);
		mutex_lock(&wifi_data.mutex);

		if ((VV_skb_tx_frames_cnt == 0) &&
		    (wl1271_tx_total_queue_count() == 0)) {
			wl1271_debug(DEBUG_MAC80211, "tx flush took %d ms",
				     jiffies_to_msecs(jiffies - start_time));
			goto out_wake;
		}
	}

	wl1271_warning("Unable to flush all TX buffers, "
		       "timed out (timeout %d ms",
		       WL1271_TX_FLUSH_TIMEOUT / 1000);

	/* forcibly flush all Tx buffers on our queues */
	for (i = 0; i < WL18XX_MAX_LINKS; i++)
		wl1271_tx_reset_link_queues(wl, i);

out_wake:
	wlcore_wake_queues(wl, WLCORE_QUEUE_STOP_REASON_FLUSH);
	mutex_unlock(&wifi_data.mutex);
out:
	mutex_unlock(&wl->flush_mutex);
}
EXPORT_SYMBOL_GPL(wl1271_tx_flush);

u32 wl1271_tx_min_rate_get(u32 rate_set)
{
	if (WARN_ON(!rate_set))
		return 0;

	return BIT(__ffs(rate_set));
}
EXPORT_SYMBOL_GPL(wl1271_tx_min_rate_get);

void wlcore_stop_queue_locked(struct wl1271 *wl, struct wl12xx_vif *wlvif,
			      u8 queue, enum wlcore_queue_stop_reason reason)
{
	int hwq = wlcore_tx_get_mac80211_queue(wlvif, queue);
	bool stopped = !!wl->queue_stop_reasons[hwq]; // equal to 0 -> true

	/* queue should not be stopped for this reason */
	WARN_ON_ONCE(test_and_set_bit(reason, &wl->queue_stop_reasons[hwq]));

	//printk("stopped = %d - reason = %d\n", stopped, reason);
	if (stopped)
		return;

	ieee80211_stop_queue(wl->hw, hwq);
}

void wlcore_stop_queue(struct wl1271 *wl, struct wl12xx_vif *wlvif, u8 queue,
		       enum wlcore_queue_stop_reason reason)
{
	unsigned long flags;

	spin_lock_irqsave(&wifi_data.lock, flags);
	wlcore_stop_queue_locked(wl, wlvif, queue, reason);
	spin_unlock_irqrestore(&wifi_data.lock, flags);
}

void wlcore_wake_queue(struct wl1271 *wl, struct wl12xx_vif *wlvif, u8 queue,
		       enum wlcore_queue_stop_reason reason)
{
	unsigned long flags;
	int hwq = wlcore_tx_get_mac80211_queue(wlvif, queue);

	spin_lock_irqsave(&wifi_data.lock, flags);

	/* queue should not be clear for this reason */
	WARN_ON_ONCE(!test_and_clear_bit(reason, &wl->queue_stop_reasons[hwq]));

	if (wl->queue_stop_reasons[hwq])
		goto out;

	ieee80211_wake_queue(wl->hw, hwq);

out:
	spin_unlock_irqrestore(&wifi_data.lock, flags);
}

void wlcore_stop_queues(struct wl1271 *wl,
			enum wlcore_queue_stop_reason reason)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&wifi_data.lock, flags);

	/* mark all possible queues as stopped */
        for (i = 0; i < WLCORE_NUM_MAC_ADDRESSES * NUM_TX_QUEUES; i++)
                WARN_ON_ONCE(test_and_set_bit(reason,
					      &wl->queue_stop_reasons[i]));

	/* use the global version to make sure all vifs in mac80211 we don't
	 * know are stopped.
	 */
	ieee80211_stop_queues(wl->hw);

	spin_unlock_irqrestore(&wifi_data.lock, flags);
}

void wlcore_wake_queues(struct wl1271 *wl,
			enum wlcore_queue_stop_reason reason)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&wifi_data.lock, flags);

	/* mark all possible queues as awake */
        for (i = 0; i < WLCORE_NUM_MAC_ADDRESSES * NUM_TX_QUEUES; i++)
		WARN_ON_ONCE(!test_and_clear_bit(reason,
						 &wl->queue_stop_reasons[i]));

	/* use the global version to make sure all vifs in mac80211 we don't
	 * know are woken up.
	 */
	ieee80211_wake_queues(wl->hw);

	spin_unlock_irqrestore(&wifi_data.lock, flags);
}

/* Vinh_custom */
void VV_wake_queue(struct wl1271 *wl, struct wl12xx_vif *wlvif, u8 queue,
		       enum wlcore_queue_stop_reason reason)
{
	unsigned long flags;
	int hwq;
	if (queue == CONF_TX_AC_VO){
		hwq = HW_QUEUE_BASE + 0;
	}
	else if (queue == CONF_TX_AC_VI){
		hwq = HW_QUEUE_BASE + 1;
	}
	else if (queue == CONF_TX_AC_BE){
		hwq = HW_QUEUE_BASE + 2;
	}
	else if (queue == CONF_TX_AC_BK){
		hwq = HW_QUEUE_BASE + 3;
	}
	else {
		hwq = HW_QUEUE_BASE + 3;
	}

	spin_lock_irqsave(&wifi_data.lock, flags);

	clear_bit(reason, &wl->queue_stop_reasons[hwq]);

	// if 4 reason bits are all clear -> wake up
	if (!wl->queue_stop_reasons[hwq])
		ieee80211_wake_queue(wl->hw, hwq);

	spin_unlock_irqrestore(&wifi_data.lock, flags);
}

void VV_stop_queue(struct wl1271 *wl, struct wl12xx_vif *wlvif, u8 queue,
		       enum wlcore_queue_stop_reason reason)
{
	unsigned long flags;
	int hwq;
	if (queue == CONF_TX_AC_VO){
		hwq = HW_QUEUE_BASE + 0;
	}
	else if (queue == CONF_TX_AC_VI){
		hwq = HW_QUEUE_BASE + 1;
	}
	else if (queue == CONF_TX_AC_BE){
		hwq = HW_QUEUE_BASE + 2;
	}
	else if (queue == CONF_TX_AC_BK){
		hwq = HW_QUEUE_BASE + 3;
	}
	else {
		hwq = HW_QUEUE_BASE + 3;
	}

	spin_lock_irqsave(&wifi_data.lock, flags);

	// if all 4 bits are clear -> no need to stop
	if (!wl->queue_stop_reasons[hwq]){

	}
	// if there is one or more set -> call stop
	else {
		ieee80211_stop_queue(wl->hw, hwq);
	}

	set_bit(reason, &wl->queue_stop_reasons[hwq]);

	spin_unlock_irqrestore(&wifi_data.lock, flags);
}

void VV_wake_all_queues(struct wl1271 *wl,
			enum wlcore_queue_stop_reason reason)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&wifi_data.lock, flags);

	/* mark all possible queues as awake */
	for (i = 0; i < WLCORE_NUM_MAC_ADDRESSES * NUM_TX_QUEUES; i++)
		clear_bit(reason, &wl->queue_stop_reasons[i]);

	/* use the global version to make sure all vifs in mac80211 we don't
	 * know are woken up.
	 */
	ieee80211_wake_queues(wl->hw);

	spin_unlock_irqrestore(&wifi_data.lock, flags);
}

void VV_stop_all_queues(struct wl1271 *wl,
			enum wlcore_queue_stop_reason reason)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&wifi_data.lock, flags);

	/* mark all possible queues as stopped */
    for (i = 0; i < WLCORE_NUM_MAC_ADDRESSES * NUM_TX_QUEUES; i++)
        set_bit(reason, &wl->queue_stop_reasons[i]);

	/* use the global version to make sure all vifs in mac80211 we don't
	 * know are stopped.
	 */
	ieee80211_stop_queues(wl->hw);

	spin_unlock_irqrestore(&wifi_data.lock, flags);
}

bool VV_stopped_by_reason(struct wl1271 *wl, u8 queue,
			enum wlcore_queue_stop_reason reason){
	unsigned long flags;
	int hwq;

	spin_lock_irqsave(&wifi_data.lock, flags);

	if (queue == CONF_TX_AC_VO){
		hwq = HW_QUEUE_BASE + 0;
	}
	else if (queue == CONF_TX_AC_VI){
		hwq = HW_QUEUE_BASE + 1;
	}
	else if (queue == CONF_TX_AC_BE){
		hwq = HW_QUEUE_BASE + 2;
	}
	else if (queue == CONF_TX_AC_BK){
		hwq = HW_QUEUE_BASE + 3;
	}
	else {
		hwq = HW_QUEUE_BASE + 3;
	}

	if (test_bit(reason, &wl->queue_stop_reasons[hwq])){
		spin_unlock_irqrestore(&wifi_data.lock, flags);
		return true;
	}
	else {
		spin_unlock_irqrestore(&wifi_data.lock, flags);
		return false;
	}
}
bool wlcore_is_queue_stopped_by_reason(struct wl1271 *wl,
				       struct wl12xx_vif *wlvif, u8 queue,
				       enum wlcore_queue_stop_reason reason)
{
	unsigned long flags;
	bool stopped;

	spin_lock_irqsave(&wifi_data.lock, flags);
	stopped = wlcore_is_queue_stopped_by_reason_locked(wl, wlvif, queue,
							   reason);
	spin_unlock_irqrestore(&wifi_data.lock, flags);

	return stopped;
}

bool wlcore_is_queue_stopped_by_reason_locked(struct wl1271 *wl,
				       struct wl12xx_vif *wlvif, u8 queue,
				       enum wlcore_queue_stop_reason reason)
{
	int hwq = wlcore_tx_get_mac80211_queue(wlvif, queue);

	assert_spin_locked(&wifi_data.lock);
	return test_bit(reason, &wl->queue_stop_reasons[hwq]);
}

bool wlcore_is_queue_stopped_locked(struct wl1271 *wl, struct wl12xx_vif *wlvif,
				    u8 queue)
{
	int hwq = wlcore_tx_get_mac80211_queue(wlvif, queue);

	assert_spin_locked(&wifi_data.lock);
	return !!wl->queue_stop_reasons[hwq];
}
