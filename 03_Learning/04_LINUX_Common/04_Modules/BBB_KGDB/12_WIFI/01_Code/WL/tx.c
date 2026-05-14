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

#include "common.h"
#include "main.h"

#define WL18XX_NUM_TX_DESCRIPTORS 32

/*
 * TODO: this is here just for now, it must be removed when the data
 * operations are in place.
 */
#include "reg.h"

static int wifi_alloc_tx_id(struct sk_buff *skb)
{
	int id;

	id = find_first_zero_bit(wifi_map.tx_frames_map, WL18XX_NUM_TX_DESCRIPTORS);
	if (id >= WL18XX_NUM_TX_DESCRIPTORS)
		return -EBUSY;

	__set_bit(id, wifi_map.tx_frames_map);
	wifi_skb_tx_frames[id] = skb;
	wifi_skb_tx_frames_cnt++;
	return id;
}

void wifi_free_tx_id(int id)
{
	// if (__test_and_clear_bit(id, wifi_data->tx_frames_map)) {
	if (__test_and_clear_bit(id, wifi_map.tx_frames_map)) {
		wifi_skb_tx_frames[id] = NULL;
		wifi_skb_tx_frames_cnt--;
	}
}
EXPORT_SYMBOL(wifi_free_tx_id);

bool wifi_is_dummy_packet(struct sk_buff *skb)
{
	return wifi_dummy_packet == skb;
}
EXPORT_SYMBOL(wifi_is_dummy_packet);

u8 wifi_tx_get_hlid(struct wifi_vif *wifi_vif,
		      struct sk_buff *skb, struct ieee80211_sta *sta)
{
	struct ieee80211_tx_info *control;

	control = IEEE80211_SKB_CB(skb);
	if (control->flags & IEEE80211_TX_CTL_TX_OFFCHAN) {
		wifi_debug(DEBUG_TX, "tx offchannel");
		return wifi_vif->dev_hlid;
	}

	return wifi_vif->sta.hlid;
}

#define WL18XX_TX_HW_BLOCK_SPARE        1
/* for special cases - namely, TKIP and GEM */
#define WL18XX_TX_HW_EXTRA_BLOCK_SPARE  2
#define WL18XX_TX_HW_BLOCK_SIZE         268
#include "wl18xx.h"
static int wifi_tx_allocate(struct sk_buff *skb, u32 buf_offset, u8 hlid)
{
	struct wifi_tx_hw_descr *desc;
	u32 total_len = skb->len + sizeof(struct wifi_tx_hw_descr);
	u32 total_blocks;
	int id, ret = -EBUSY, ac;
	u32 spare_blocks;
	u32 blk_size;

	if (buf_offset + total_len > WL18XX_AGGR_BUFFER_SIZE)
		return -EAGAIN;

	/* If we have keys requiring extra spare, indulge them */
	spare_blocks = WL18XX_TX_HW_BLOCK_SPARE;

	/* allocate free identifier for the packet */
	id = wifi_alloc_tx_id(skb); // put skb into wifi_skb_tx_frames[32], wifi_skb_tx_frames_cnt++
	if (id < 0)
		return id;

	blk_size = WL18XX_TX_HW_BLOCK_SIZE;
	total_blocks = (total_len + blk_size - 1) / blk_size + spare_blocks;

	if (total_blocks <= wifi_tx_blocks_available) {
		// Adds the TX descriptor at the front of the skb
		desc = skb_push(skb, total_len - skb->len); // len = sizeof(struct wifi_tx_hw_descr)
		desc->wl18xx_mem.total_mem_blocks = total_blocks;

		desc->id = id;

		wifi_tx_blocks_available -= total_blocks;

		wifi_tx_allocated_blocks += total_blocks;
#if (PRINT_DEBUG_TX_WATCHDOG)
		printk("Allocated blks for transmit: %d\n", total_blocks);
#endif

		/*
		 * If the FW was empty before, arm the Tx watchdog. Also do
		 * this on the first Tx after resume, as we always cancel the
		 * watchdog on suspend.
		 */
		// if wifi_tx_allocated_blocks is zero BEFORE
		if (wifi_tx_allocated_blocks == total_blocks ||
		    test_and_clear_bit(WL1271_FLAG_REINIT_TX_WDOG, &wifi_data->flags))
			wifi_rearm_tx_watchdog_locked();

		ac = skb_get_queue_mapping(skb);
		wifi_tx_allocated_pkts[ac]++;

		if (test_bit(hlid, wifi_map.links_map))
			wifi_allocated_pkts[hlid]++;

		ret = 0;

		wifi_debug(DEBUG_TX,
			     "tx_allocate: size: %d, blocks: %d, id: %d",
			     total_len, total_blocks, id);
	} else {
		wifi_free_tx_id(id);
	}

	return ret;
}

/* Indicates this TX HW frame is not padded to SDIO block size */
#define WL18XX_TX_CTRL_NOT_PADDED	BIT(7)
static void wifi_tx_fill_hdr(struct sk_buff *skb,
			       struct ieee80211_tx_info *control, u8 hlid)
{
	struct wifi_tx_hw_descr *desc;
	int ac, rate_idx;
	s64 hosttime;
	u16 tx_attr = 0;
	__le16 frame_control;
	struct ieee80211_hdr *hdr;
	u8 *frame_start;
	bool is_dummy;
	u8 session_id;

	desc = (struct wifi_tx_hw_descr *) skb->data;
	frame_start = (u8 *)(desc + 1); // frame_start points to the actual 802.11 frame.
	hdr = (struct ieee80211_hdr *)(frame_start);
	frame_control = hdr->frame_control;

	/* configure packet life time */
	hosttime = (ktime_get_boottime_ns() >> 10);
	desc->start_time = cpu_to_le32(hosttime - wifi_time_offset);

	
	is_dummy = wifi_is_dummy_packet(skb);
	desc->life_time = cpu_to_le16(TX_HW_MGMT_PKT_LIFETIME_TU);

	/* queue */
	ac = skb_get_queue_mapping(skb);
	desc->tid = skb->priority;

	session_id = wifi_session_ids[hlid];


	/* configure the tx attributes */
	tx_attr = session_id << TX_HW_ATTR_OFST_SESSION_COUNTER;

	desc->hlid = hlid;
	/*
		* if the packets are data packets
		* send them with AP rate policies (EAPOLs are an exception),
		* otherwise use default basic rates
		*/
	if (skb->protocol == cpu_to_be16(ETH_P_PAE))
		rate_idx = STA_BASIC_RATE_IDX;
	else if (control->flags & IEEE80211_TX_CTL_NO_CCK_RATE)
		rate_idx = STA_P2P_RATE_IDX;
	else if (ieee80211_is_data(frame_control))
		rate_idx = STA_AP_RATE_IDX;
	else
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

	desc->wl18xx_checksum_data = 0;

	desc->length = cpu_to_le16(skb->len);

	desc->wl18xx_mem.ctrl = WL18XX_TX_CTRL_NOT_PADDED;
}

/* caller must hold wifi_data->mutex */
static int wifi_prepare_tx_frame(struct sk_buff *skb, u32 buf_offset, u8 hlid)
{
	struct ieee80211_tx_info *info;
	int ret = 0;
	u32 total_len;

	// skb is taken from wifi_skb_dequeue

	if (!skb) {
		wifi_error("discarding null skb");
		return -EINVAL;
	}

	if (hlid == WL12XX_INVALID_LINK_ID) {
		wifi_error("invalid hlid. dropping skb 0x%p", skb);
		return -EINVAL;
	}

	info = IEEE80211_SKB_CB(skb);

	ret = wifi_tx_allocate(skb, buf_offset, hlid);
	if (ret < 0)
		return ret;

	wifi_tx_fill_hdr(skb, info, hlid);

	/*
	 * The length of each packet is stored in terms of
	 * words. Thus, we must pad the skb data to make sure its
	 * length is aligned.  The number of padding bytes is computed
	 * and set in wifi_tx_fill_hdr.
	 * In special cases, we want to align to a specific block size
	 * (eg. for wl128x with SDIO we align to 256).
	 */
	total_len = ALIGN(skb->len, WL1271_TX_ALIGN_TO);

	memcpy(wifi_aggr_buf + buf_offset, skb->data, skb->len);
	memset(wifi_aggr_buf + buf_offset + skb->len, 0, total_len - skb->len);

	// /* Revert side effects in the dummy packet skb, so it can be reused */
	// if (is_dummy)
	// 	skb_pull(skb, sizeof(struct wifi_tx_hw_descr));

	return total_len;
}

u32 wifi_tx_enabled_rates_get(u32 rate_set,
				enum nl80211_band rate_band)
{
	struct ieee80211_supported_band *band;
	u32 enabled_rates = 0;
	int bit;

	// rate_set = [MCS_rate][unsued][n_bitrates_id]
	//			   31 - 16             12 - 0
	//             HW bit             band->bitrates[id].hw_value

	// enabled_rates = 
	//    28 bit - aligned with HW_bit enum in conf.h


	band = wifi_data->hw->wiphy->bands[rate_band];

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

	return enabled_rates;
}

static int wificore_select_ac(void)
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
		ac = i;
		//if (wifi_data->tx_queue_count[ac] &&
		if (wifi_tx_queue_count[ac] &&
			wifi_tx_allocated_pkts[ac] < min_pkts) {
			q = ac;
			// min_pkts = wifi_data->tx_allocated_pkts[q];
			min_pkts = wifi_tx_allocated_pkts[q];
		}
	}

	return q;
}

static struct sk_buff *wificore_lnk_dequeue(u8 hlid, u8 q)
{
	struct sk_buff *skb;
	unsigned long flags;

	skb = skb_dequeue(&wifi_tx_queue[hlid][q]);
#if (PRINT_DEBUG)
	printk("[3] - skb = 0x%x\n", skb);
#endif
	if (skb) {
		spin_lock_irqsave(&wifi_data->lock, flags);
		wifi_tx_queue_count[q]--;
		spin_unlock_irqrestore(&wifi_data->lock, flags);
	}

	return skb;
}

static bool wifi_lnk_high_prio(u8 hlid)
{
	u8 thold;
	unsigned long suspend_bitmap = 0;

	if (test_bit(hlid, &suspend_bitmap))
		return false; // false if using default hlink 0

	/* the priority thresholds are taken from FW */
	// if (test_bit(hlid, &wifi_data->fw_fast_lnk_map) &&
	//     !test_bit(hlid, &wifi_data->ap_fw_ps_map))
	if (test_bit(hlid, (unsigned long*)&wifi_status_reg->link_fast_bitmap))
		thold = wifi_status_reg->tx_fast_link_prio_threshold;
	else
		thold = wifi_status_reg->tx_slow_link_prio_threshold;
	return wifi_allocated_pkts[hlid] < thold;
}

static bool wifi_lnk_low_prio(u8 hlid)
{
	u8 thold;
	unsigned long suspend_bitmap;

	suspend_bitmap = le32_to_cpu(wifi_status_reg->link_suspend_bitmap);

	if (test_bit(hlid, &suspend_bitmap))
		thold = wifi_status_reg->tx_suspend_threshold;
	else if (test_bit(hlid, (unsigned long*)&wifi_status_reg->link_fast_bitmap))
		thold = wifi_status_reg->tx_fast_stop_threshold;
	else
		thold = wifi_status_reg->tx_slow_stop_threshold;


	return wifi_allocated_pkts[hlid] < thold;
}

int test = 0;
static struct sk_buff *wifi_skb_dequeue(void)
{
	struct sk_buff *skb = NULL;
	int ac;
	u8 low_prio_hlid = WL12XX_INVALID_LINK_ID;

	// Find ac has data (the least allocated blks and V0>VI>...)
	ac = wificore_select_ac();
	if (ac < 0){
#if (PRINT_DEBUG)
			printk("FAILED - ac\n");
#endif
		return NULL;
	}

	if (!wifi_lnk_high_prio(HW_LINK_ID)) {
		if (low_prio_hlid == WL12XX_INVALID_LINK_ID &&
			!skb_queue_empty(&wifi_tx_queue[HW_LINK_ID][ac]) &&
			wifi_lnk_low_prio(HW_LINK_ID)) // wl18xx_lnk_low_prio
			/* we found the first non-empty low priority queue */
			low_prio_hlid = HW_LINK_ID;

		skb = NULL;
	}
	// this case for high priority
	else skb = wificore_lnk_dequeue(HW_LINK_ID, ac);
#if (PRINT_DEBUG)
	printk("[2] - skb = 0x%x, low_prio_hlid = %x\n", skb, low_prio_hlid);
#endif
	return skb;
}

static bool wifi_tx_is_data_present(struct sk_buff *skb)
{
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)(skb->data);

	return ieee80211_is_data_present(hdr->frame_control);
}

/*
 * Returns failure values only in case of failed bus ops within this function.
 * wifi_prepare_tx_frame retvals won't be returned in order to avoid
 * triggering recovery by higher layers when not necessary.
 * In case a FW command fails within wifi_prepare_tx_frame fails a recovery
 * will be queued in wifi_cmd_send. -EAGAIN/-EBUSY from prepare_tx_frame
 * can occur and are legitimate so don't propagate. -EINVAL will emit a WARNING
 * within prepare_tx_frame code but there's nothing we should do about those
 * as well.
 */
/* Indicates this TX HW frame is not padded to SDIO block size */
#define WL18XX_TX_CTRL_NOT_PADDED	BIT(7)
int wificore_tx_work_locked(void)
{
	struct wifi_vif *wifi_vif;
	struct sk_buff *skb;
	struct wifi_tx_hw_descr *desc;
	u32 buf_offset = 0, last_len = 0;
	bool sent_packets = false;
	unsigned long active_hlids[BITS_TO_LONGS(WLCORE_MAX_LINKS)] = {0};
	//int ret = 0;
	int bus_ret = 0;
	u8 hlid = HW_LINK_ID;

	while ((skb = wifi_skb_dequeue())) {
		struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
		bool has_data = false;

		wifi_vif = wifi_vif_to_data(info->control.vif);

		has_data = wifi_tx_is_data_present(skb);
		last_len = wifi_prepare_tx_frame(skb, buf_offset,
					      hlid);
#if (PRINT_DEBUG)
		printk("wifi_prepare_tx_frame->wifi_vif = 0x%x\n", wifi_vif);
#endif
		buf_offset += last_len;
		wifi_tx_packets_count++;
		if (has_data) {
			desc = (struct wifi_tx_hw_descr *) skb->data;
			__set_bit(desc->hlid, active_hlids);
		}
	}

	if (buf_offset) {
		struct wifi_tx_hw_descr *last_desc;

		/* get the last TX HW descriptor written to the aggr buf */
		last_desc = (struct wifi_tx_hw_descr *)(wifi_aggr_buf +
							buf_offset - last_len);

		/* the last frame is padded up to an SDIO block */
		last_desc->wl18xx_mem.ctrl &= ~WL18XX_TX_CTRL_NOT_PADDED;
		buf_offset = ALIGN(buf_offset, WL12XX_BUS_BLOCK_SIZE);


		// REG_SLV_MEM_DATA → the address in the firmware’s memory where TX data should be written.
		bus_ret = wifi_sdio_raw_write1(wificore_translate_addr(wifi_data->rtable[REG_SLV_MEM_DATA]), wifi_aggr_buf, buf_offset, true);
		if (bus_ret < 0)
			goto out;

		sent_packets = true;
	}
out:
	return bus_ret;
}

void wifi_tx_work(struct work_struct *work)
{
	int ret;

	mutex_lock(&wifi_data->mutex);
	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out;
	}

	ret = wificore_tx_work_locked();
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
#if (PRINT_DEBUG)
		printk("wifi_queue_recovery_work -> SHOULD RESTART\n");
#endif
		goto out;
	}

	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);
out:
	mutex_unlock(&wifi_data->mutex);
}

void wifi_tx_reset_link_queues(u8 hlid)
{
	struct sk_buff *skb;
	int i;
	unsigned long flags;
	struct ieee80211_tx_info *info;
	int total[NUM_TX_QUEUES];

	for (i = 0; i < NUM_TX_QUEUES; i++) {
		total[i] = 0;
		while ((skb = skb_dequeue(&wifi_tx_queue[hlid][i]))) {
#if (PRINT_DEBUG)
			printk("TX_QUEUE - wifi_tx_reset_link_queues\n");
#endif
			if (!wifi_is_dummy_packet(skb)) {
				info = IEEE80211_SKB_CB(skb);
				info->status.rates[0].idx = -1;
				info->status.rates[0].count = 0;
				ieee80211_tx_status_ni(wifi_data->hw, skb);
			}

			total[i]++;
		}
	}

	spin_lock_irqsave(&wifi_data->lock, flags);
	for (i = 0; i < NUM_TX_QUEUES; i++) {
		wifi_tx_queue_count[i] -= total[i];
	}
	spin_unlock_irqrestore(&wifi_data->lock, flags);

}

/* caller must hold wifi_data->mutex and TX must be stopped */
void wifi_tx_reset_wifi_vif(struct wifi_vif *wifi_vif)
{
	int i;

	/* TX failure */
	for_each_set_bit(i, wifi_vif->links_map, WL18XX_MAX_LINKS) {
		u8 hlid = i;
		wifi_free_link(wifi_vif, &hlid);
	}

	//wifi_vif->last_tx_hlid = 0;
}
/* caller must hold wifi_data->mutex and TX must be stopped */
void wifi_tx_reset(void)
{
	int i;
	struct sk_buff *skb;
	struct ieee80211_tx_info *info;

	/* only reset the queues if something bad happened */
	if (wifi_tx_total_queue_count() != 0) {
		for (i = 0; i < WL18XX_MAX_LINKS; i++)
			wifi_tx_reset_link_queues(i);

		for (i = 0; i < NUM_TX_QUEUES; i++)
			//wifi_data->tx_queue_count[i] = 0;
			wifi_tx_queue_count[i] = 0;
	}

	/*
	 * Make sure the driver is at a consistent state, in case this
	 * function is called from a context other than interface removal.
	 * This call will always wake the TX queues.
	 */

	for (i = 0; i < WL18XX_NUM_TX_DESCRIPTORS; i++) {
		if (wifi_skb_tx_frames[i] == NULL)
			continue;

		skb = wifi_skb_tx_frames[i];
		wifi_free_tx_id(i);
		wifi_debug(DEBUG_TX, "freeing skb 0x%p", skb);

		if (!wifi_is_dummy_packet(skb)) {
			/*
			 * Remove private headers before passing the skb to
			 * mac80211
			 */
			info = IEEE80211_SKB_CB(skb);
			skb_pull(skb, sizeof(struct wifi_tx_hw_descr));
			if ((wifi_data->quirks & WLCORE_QUIRK_TKIP_HEADER_SPACE) &&
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

			ieee80211_tx_status_ni(wifi_data->hw, skb);
		}
	}
}

#define WL1271_TX_FLUSH_TIMEOUT 500000

/* caller must *NOT* hold wifi_data->mutex */
void wifi_tx_flush(void)
{
	unsigned long timeout, start_time;
	int i;
	start_time = jiffies;
	timeout = start_time + usecs_to_jiffies(WL1271_TX_FLUSH_TIMEOUT);

	/* only one flush should be in progress, for consistent queue state */
	mutex_lock(&wifi_data->flush_mutex);

	mutex_lock(&wifi_data->mutex);
	if (wifi_skb_tx_frames_cnt == 0 && wifi_tx_total_queue_count() == 0) {
		mutex_unlock(&wifi_data->mutex);
		goto out;
	}

	wificore_stop_queues(WLCORE_QUEUE_STOP_REASON_FLUSH);

	while (!time_after(jiffies, timeout)) {
		wifi_debug(DEBUG_MAC80211, "flushing tx buffer: %d %d",
			     wifi_skb_tx_frames_cnt,
			     wifi_tx_total_queue_count());

		/* force Tx and give the driver some time to flush data */
		mutex_unlock(&wifi_data->mutex);
		if (wifi_tx_total_queue_count())
			wifi_tx_work(&wifi_work.tx_work);
		msleep(20);
		mutex_lock(&wifi_data->mutex);

		if ((wifi_skb_tx_frames_cnt == 0) &&
		    (wifi_tx_total_queue_count() == 0)) {
			wifi_debug(DEBUG_MAC80211, "tx flush took %d ms",
				     jiffies_to_msecs(jiffies - start_time));
			goto out_wake;
		}
	}

	wifi_warning("Unable to flush all TX buffers, "
		       "timed out (timeout %d ms",
		       WL1271_TX_FLUSH_TIMEOUT / 1000);

	/* forcibly flush all Tx buffers on our queues */
	for (i = 0; i < WL18XX_MAX_LINKS; i++)
		wifi_tx_reset_link_queues(i);

out_wake:
	wificore_wake_queues(WLCORE_QUEUE_STOP_REASON_FLUSH);
	mutex_unlock(&wifi_data->mutex);
out:
	mutex_unlock(&wifi_data->flush_mutex);
}
EXPORT_SYMBOL_GPL(wifi_tx_flush);

u32 wifi_tx_min_rate_get(u32 rate_set)
{
	if (WARN_ON(!rate_set))
		return 0;

	return BIT(__ffs(rate_set));
}
EXPORT_SYMBOL_GPL(wifi_tx_min_rate_get);

void wificore_stop_queues(
			enum wificore_queue_stop_reason reason)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&wifi_data->lock, flags);

	/* mark all possible queues as stopped */
        for (i = 0; i < WLCORE_NUM_MAC_ADDRESSES * NUM_TX_QUEUES; i++)
                WARN_ON_ONCE(test_and_set_bit(reason,
					      &wifi_data->queue_stop_reasons[i]));

	/* use the global version to make sure all vifs in mac80211 we don't
	 * know are stopped.
	 */
	ieee80211_stop_queues(wifi_data->hw);

	spin_unlock_irqrestore(&wifi_data->lock, flags);
}

void wificore_wake_queues(
			enum wificore_queue_stop_reason reason)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&wifi_data->lock, flags);

	/* mark all possible queues as awake */
        for (i = 0; i < WLCORE_NUM_MAC_ADDRESSES * NUM_TX_QUEUES; i++)
		WARN_ON_ONCE(!test_and_clear_bit(reason,
						 &wifi_data->queue_stop_reasons[i]));

	/* use the global version to make sure all vifs in mac80211 we don't
	 * know are woken up.
	 */
	ieee80211_wake_queues(wifi_data->hw);

	spin_unlock_irqrestore(&wifi_data->lock, flags);
}

bool wificore_is_queue_stopped_by_reason_locked(
				       struct wifi_vif *wifi_vif, u8 queue,
				       enum wificore_queue_stop_reason reason)
{
	int hwq = wificore_tx_get_mac80211_queue(wifi_vif, queue);

	assert_spin_locked(&wifi_data->lock);
	return test_bit(reason, &wifi_data->queue_stop_reasons[hwq]);
}

