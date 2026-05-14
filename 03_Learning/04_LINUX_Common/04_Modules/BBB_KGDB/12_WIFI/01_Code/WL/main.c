// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wlcore
 *
 * Copyright (C) 2008-2010 Nokia Corporation
 * Copyright (C) 2011-2013 Texas Instruments Inc.
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/etherdevice.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pm_runtime.h>
#include <linux/pm_wakeirq.h>

#include "wlcore.h"
#include "debug.h"
#include "wifi_80211.h"
#include "io.h"
#include "tx.h"
#include "rx.h"
#include "ps.h"
#include "init.h"
#include "scan.h"
#include "cmd.h"

#include "common.h"
#include "wl18.h"
#include "reg.h"
#include "ops.h"
#include "main.h"
struct sk_buff_head wifi_tx_queue[WLCORE_MAX_LINKS][NUM_TX_QUEUES];
struct sk_buff_head wifi_deferred_rx_queue;
struct sk_buff_head wifi_deferred_tx_queue;
u8 wifi_allocated_pkts[WLCORE_MAX_LINKS];
int wifi_tx_queue_count[NUM_TX_QUEUES];
u32 wifi_tx_pkts_freed[NUM_TX_QUEUES];
u32 wifi_tx_allocated_pkts[NUM_TX_QUEUES];
struct wifi_wl18xx_fw_status *wifi_status_reg;
u32 wifi_tx_allocated_blocks;
u32 wifi_tx_blocks_available;
struct sk_buff *wifi_skb_tx_frames[WLCORE_MAX_TX_DESCRIPTORS];
struct sk_buff *wifi_dummy_packet;
struct wifi_link wifi_links[WLCORE_MAX_LINKS];
int wifi_skb_tx_frames_cnt;
u32 wifi_last_updated_tmp_tx_blocks_freed;
u32 wifi_tx_packets_count; 
u8 wifi_last_fw_rls_idx = 0;
struct wifi_Work wifi_work;
u8 wifi_session_ids[WLCORE_MAX_LINKS];
s64 wifi_time_offset;

/* Main */
struct Wifi_data* wifi_data;
struct wifi_map wifi_map;
struct wifi_vif* wifi_vif_ptr[10];
struct wifi_chip* wifi_chip;
int wifi_vif_ptr_id = 0;
u8 *wifi_aggr_buf;
u32 wifi_rx_counter;
bool wifi_scan_failed;

#define WL1271_BOOT_RETRIES 3
#define WL1271_WAKEUP_TIMEOUT 500

static char *fwlog_param;
static int fwlog_mem_blocks = -1;
static int bug_on_recovery = -1;
static int no_recovery     = -1;

static void __wifi_op_remove_interface(
					 struct ieee80211_vif *vif,
					 bool reset_tx_queues);
static void wificore_op_stop_locked(void);

static void wifi_reg_notify(struct wiphy *wiphy,
			      struct regulatory_request *request)
{
	/* copy the current dfs region */
	if (request)
		wifi_data->dfs_region = request->dfs_region;

	wificore_regdomain_config();
}

/* wifi_data->mutex must be taken */
void wifi_rearm_tx_watchdog_locked(void)
{
	/* if the watchdog is not armed, don't do anything */
	// If there are no blocks currently allocated for TX -> no need to transmit
	// so no need to check TX data is stuck
	if (wifi_tx_allocated_blocks == 0)
		return;

	cancel_delayed_work(&wifi_work.tx_watchdog_work);
	ieee80211_queue_delayed_work(wifi_data->hw, &wifi_work.tx_watchdog_work,
		msecs_to_jiffies(5000));
}

static void wifi_tx_watchdog_work(struct work_struct *work)
{
//#if (PRINT_DEBUG)
	printk("[WORK] - wifi_tx_watchdog_work\n");
//#endif
	struct delayed_work *dwork;

	dwork = to_delayed_work(work);

	mutex_lock(&wifi_data->mutex);

	if (unlikely(wifi_data->state != WLCORE_STATE_ON))
		goto out;

	/* Tx went out in the meantime - everything is ok */
	if (unlikely(wifi_tx_allocated_blocks == 0))
		goto out;

	/*
	 * if a ROC is in progress, we might not have any Tx for a long
	 * time (e.g. pending Tx on the non-ROC channels)
	 */
	if (find_first_bit(wifi_data->roc_map, WL12XX_MAX_ROLES) < WL12XX_MAX_ROLES) {
		wifi_debug(DEBUG_TX, "No Tx (in FW) for %d ms due to ROC",
			     wifi_data->conf.tx.tx_watchdog_timeout);
		wifi_rearm_tx_watchdog_locked();
		goto out;
	}

	/*
	 * if a scan is in progress, we might not have any Tx for a long
	 * time
	 */
	if (wifi_data->scan_state != WL1271_SCAN_STATE_IDLE) {
		wifi_debug(DEBUG_TX, "No Tx (in FW) for %d ms due to scan",
			     wifi_data->conf.tx.tx_watchdog_timeout);
		wifi_rearm_tx_watchdog_locked();
		goto out;
	}

	// /*
	// * AP might cache a frame for a long time for a sleeping station,
	// * so rearm the timer if there's an AP interface with stations. If
	// * Tx is genuinely stuck we will most hopefully discover it when all
	// * stations are removed due to inactivity.
	// */
	// if (wifi_data->active_sta_count) {
	// 	wifi_debug(DEBUG_TX, "No Tx (in FW) for %d ms. AP has "
	// 		     " %d stations",
	// 		      wifi_data->conf.tx.tx_watchdog_timeout,
	// 		      wifi_data->active_sta_count);
	// 	wifi_rearm_tx_watchdog_locked();
	// 	goto out;
	// }

	wifi_error("Tx stuck (in FW) for %d ms. Starting recovery",
		     wifi_data->conf.tx.tx_watchdog_timeout);
	printk("wifi_queue_recovery_work -> SHOULD RESTART\n");

out:
	mutex_unlock(&wifi_data->mutex);
}

static void wificore_adjust_conf(void)
{

	if (fwlog_param) {
		if (!strcmp(fwlog_param, "continuous")) {
			wifi_data->conf.fwlog.mode = WL12XX_FWLOG_CONTINUOUS;
			wifi_data->conf.fwlog.output = WL12XX_FWLOG_OUTPUT_HOST;
		} else if (!strcmp(fwlog_param, "dbgpins")) {
			wifi_data->conf.fwlog.mode = WL12XX_FWLOG_CONTINUOUS;
			wifi_data->conf.fwlog.output = WL12XX_FWLOG_OUTPUT_DBG_PINS;
		} else if (!strcmp(fwlog_param, "disable")) {
			wifi_data->conf.fwlog.mem_blocks = 0;
			wifi_data->conf.fwlog.output = WL12XX_FWLOG_OUTPUT_NONE;
		} else {
			wifi_error("Unknown fwlog parameter %s", fwlog_param);
		}
	}

	if (bug_on_recovery != -1)
		wifi_data->conf.recovery.bug_on_recovery = (u8) bug_on_recovery;

	if (no_recovery != -1)
		wifi_data->conf.recovery.no_recovery = (u8) no_recovery;
}

#include "wl18xx.h"
static int wificore_fw_status(void)
{
	//struct wifi_vif *wifi_vif;
	//u32 old_tx_blk_count = wifi_tx_blocks_available;
	int avail, freed_blocks;
	int i;
	int ret;

	ret = wifi_sdio_raw_read(wifi_data->rtable[REG_RAW_FW_STATUS_ADDR],
				   (void*)wifi_status_reg,
				   sizeof(struct wifi_wl18xx_fw_status), false);
	if (ret < 0)
		return ret;

	for (i = 0; i < NUM_TX_QUEUES; i++) {
		/* prevent wrap-around in freed-packets counter */
		wifi_tx_allocated_pkts[i] -=
				(wifi_status_reg->tx_released_pkts[i] -
				wifi_tx_pkts_freed[i]) & 0xff;

		wifi_tx_pkts_freed[i] = wifi_status_reg->tx_released_pkts[i];

		// counters.tx_released_pkts and counters.tx_released_pkts are read from Interrupt
	}

	//printk("START\n");
	for_each_set_bit(i, wifi_map.links_map, WL18XX_MAX_LINKS) {
		//printk("wificore_fw_status i = %d\n", i);
		u8 diff;

		/* prevent wrap-around in freed-packets counter */
		diff = (wifi_status_reg->tx_lnk_free_pkts[i] -
		       wifi_links[i].prev_freed_pkts) & 0xff;

		if (diff == 0)
			continue;

		wifi_allocated_pkts[i] -= diff;
		wifi_links[i].prev_freed_pkts = wifi_status_reg->tx_lnk_free_pkts[i];

		/* accumulate the prev_freed_pkts counter */
		wifi_links[i].total_freed_pkts += diff;
	}
	//printk("END\n");

	/* prevent wrap-around in total blocks counter */
	if (likely(wifi_last_updated_tmp_tx_blocks_freed <= wifi_status_reg->total_released_blks))
		freed_blocks = wifi_status_reg->total_released_blks -
			       wifi_last_updated_tmp_tx_blocks_freed;
	else
		freed_blocks = 0x100000000LL - wifi_last_updated_tmp_tx_blocks_freed +
			       wifi_status_reg->total_released_blks;

	wifi_last_updated_tmp_tx_blocks_freed = wifi_status_reg->total_released_blks;
	// => freed_blocks = new (total_released_blks) - last (tx_blocks_freed)
	//				   = the ctr number of released blocks

	//freed_blocks-=2;
	
	wifi_tx_allocated_blocks = wifi_tx_allocated_blocks - freed_blocks;

#if (PRINT_DEBUG_TX_WATCHDOG)
		printk("Free blks for transmit: %d\n", freed_blocks);
#endif

	/*
	 * If the FW freed some blocks:
	 * If we still have allocated blocks - re-arm the timer, Tx is
	 * not stuck. Otherwise, cancel the timer (no Tx currently).
	 */
	if (freed_blocks) {
		if (wifi_tx_allocated_blocks) 
			wifi_rearm_tx_watchdog_locked();
		else
			cancel_delayed_work(&wifi_work.tx_watchdog_work);
	}
	// // if tx_allocated_blocks > 0 -> there is blocks in fw -> raise TX stuck when there
	// is no action to send these blks out

	avail = wifi_status_reg->tx_total - wifi_tx_allocated_blocks;

	/*
	 * The FW might change the total number of TX memblocks before
	 * we get a notification about blocks being released. Thus, the
	 * available blocks calculation might yield a temporary result
	 * which is lower than the actual available blocks. Keeping in
	 * mind that only blocks that were allocated can be moved from
	 * TX to RX, tx_blocks_available should never decrease here.
	 */
	wifi_tx_blocks_available = max((int)wifi_tx_blocks_available,
				      avail);
	// printk("[RUN] - 0x%x vs 0x%x - 0x%x vs 0x%x\n", wifi_status_reg->tx_total, wifi_tx_allocated_blocks
	// 				, wifi_tx_blocks_available, old_tx_blk_count);

	/* if more blocks are available now, tx work can be scheduled */
	// if (wifi_tx_blocks_available > old_tx_blk_count)
	// 	clear_bit(WL1271_FLAG_FW_TX_BUSY, &wifi_data->flags);

	/* update the host-chipset time offset */
	wifi_time_offset = (ktime_get_boottime_ns() >> 10) -
		(s64)(wifi_status_reg->fw_localtime);

	//wifi_data->fw_fast_lnk_map = wifi_status_reg->link_fast_bitmap;

	return 0;
}

static void wifi_flush_deferred_work(void)
{
	struct sk_buff *skb;

	/* Pass all received frames to the network stack */
	while ((skb = skb_dequeue(&wifi_deferred_rx_queue)))
		ieee80211_rx_ni(wifi_data->hw, skb);

	/* Return sent skbs to the network stack */
	while ((skb = skb_dequeue(&wifi_deferred_tx_queue)))
		ieee80211_tx_status_ni(wifi_data->hw, skb);
}

static void wifi_netstack_work(struct work_struct *work)
{
	do {
		struct sk_buff *skb;

		/* Pass all received frames to the network stack */
		while ((skb = skb_dequeue(&wifi_deferred_rx_queue)))
			// ieee80211_rx_ni(wifi_data->hw, skb); // take skb ptr -> pass to nwstack
			ieee80211_rx_ni(wifi_data->hw, skb); // take skb ptr -> pass to nwstack

		/* Return sent skbs to the network stack */
		while ((skb = skb_dequeue(&wifi_deferred_tx_queue)))
			// ieee80211_tx_status_ni(wifi_data->hw, skb);
			ieee80211_tx_status_ni(wifi_data->hw, skb);
	} while (skb_queue_len(&wifi_deferred_rx_queue)); // drain until there is no queue left (no list of ptrs)
}

#if (PRINT_DEBUG_RATE)
static const char *wifi_tx_rate_to_string(u8 rate)
{
    switch (rate) {
    case CONF_HW_RATE_INDEX_1MBPS:          return "1 Mbps";
    case CONF_HW_RATE_INDEX_2MBPS:          return "2 Mbps";
    case CONF_HW_RATE_INDEX_5_5MBPS:        return "5.5 Mbps";
    case CONF_HW_RATE_INDEX_11MBPS:         return "11 Mbps";
    case CONF_HW_RATE_INDEX_6MBPS:          return "6 Mbps";
    case CONF_HW_RATE_INDEX_9MBPS:          return "9 Mbps";
    case CONF_HW_RATE_INDEX_12MBPS:         return "12 Mbps";
    case CONF_HW_RATE_INDEX_18MBPS:         return "18 Mbps";
    case CONF_HW_RATE_INDEX_24MBPS:         return "24 Mbps";
    case CONF_HW_RATE_INDEX_36MBPS:         return "36 Mbps";
    case CONF_HW_RATE_INDEX_48MBPS:         return "48 Mbps";
    case CONF_HW_RATE_INDEX_54MBPS:         return "54 Mbps";

    /* 802.11n MCS rates */
    case CONF_HW_RATE_INDEX_MCS0:           return "MCS0 (6.5 Mbps)";
    case CONF_HW_RATE_INDEX_MCS1:           return "MCS1 (13 Mbps)";
    case CONF_HW_RATE_INDEX_MCS2:           return "MCS2 (19.5 Mbps)";
    case CONF_HW_RATE_INDEX_MCS3:           return "MCS3 (26 Mbps)";
    case CONF_HW_RATE_INDEX_MCS4:           return "MCS4 (39 Mbps)";
    case CONF_HW_RATE_INDEX_MCS5:           return "MCS5 (52 Mbps)";
    case CONF_HW_RATE_INDEX_MCS6:           return "MCS6 (58.5 Mbps)";
    case CONF_HW_RATE_INDEX_MCS7:           return "MCS7 (65 Mbps)";
    case CONF_HW_RATE_INDEX_MCS7_SGI:       return "MCS7 SGI (72.2 Mbps)";

    /* 40MHz rates */ /* MIMO rates */
    case CONF_HW_RATE_INDEX_MCS0_40MHZ:     return "MCS0 40MHz (13.5 Mbps) | MCS8 (13 Mbps)";
    case CONF_HW_RATE_INDEX_MCS7_40MHZ:     return "MCS7 40MHz (135 Mbps) | MCS15 (130 Mbps)";
    case CONF_HW_RATE_INDEX_MCS7_40MHZ_SGI: return "MCS7 40MHz SGI (150 Mbps) | MCS15 SGI (144.4 Mbps)";


    default:
        return "Unknown Rate";
    }
}
#endif

static void wifi_get_last_tx_rate(struct ieee80211_vif *vif,
			     u8 band, struct ieee80211_tx_rate *rate, u8 hlid)
{
	u8 fw_rate = wifi_links[hlid].fw_rate_idx; // read from wifi_status_reg->tx_last_rate;

	if (fw_rate > CONF_HW_RATE_INDEX_MAX) {
		wifi_error("last Tx rate invalid: %d", fw_rate);
		rate->idx = 0;
		rate->flags = 0;
		return;
	}

	// fw_rate is usually MCS1, MCS3, MCS5 - normal and reasonable for a real-world connection.
	if (fw_rate <= CONF_HW_RATE_INDEX_54MBPS) {
		rate->idx = fw_rate;
		if (band == NL80211_BAND_5GHZ)
			rate->idx -= CONF_HW_RATE_INDEX_6MBPS;
		rate->flags = 0;
	} else {
		rate->flags = IEEE80211_TX_RC_MCS;
		rate->idx = fw_rate - CONF_HW_RATE_INDEX_MCS0;

		/* SGI modifier is counted as a separate rate */
		if (fw_rate >= CONF_HW_RATE_INDEX_MCS7_SGI)
			(rate->idx)--;
		if (fw_rate == CONF_HW_RATE_INDEX_MCS15_SGI)
			(rate->idx)--;

		/* this also covers the 40Mhz SGI case (= MCS15) */
		if (fw_rate == CONF_HW_RATE_INDEX_MCS7_SGI ||
		    fw_rate == CONF_HW_RATE_INDEX_MCS15_SGI)
			rate->flags |= IEEE80211_TX_RC_SHORT_GI;

		if (fw_rate > CONF_HW_RATE_INDEX_MCS7_SGI && vif) {
			struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
			if (wifi_vif->channel_type == NL80211_CHAN_HT40MINUS ||
			    wifi_vif->channel_type == NL80211_CHAN_HT40PLUS) {
				/* adjustment needed for range 0-7 */
				rate->idx -= 8;
				rate->flags |= IEEE80211_TX_RC_40_MHZ_WIDTH;
			}
		}
	}

	//rate->idx = CONF_HW_RATE_INDEX_6MBPS;

	
#if (PRINT_DEBUG)
	printk("[TX_RATE] - fw_rate = %d, rate->idx = %d\n", fw_rate, rate->idx);
#endif
#if (PRINT_DEBUG_RATE)
	printk("[TX_RATE] - TX rate = %s\n", wifi_tx_rate_to_string(fw_rate));
#endif

	// Final purpose:
	// Update ieee80211_tx_rate *rate
		// idx
		// flags

	// fw_rate -> its name shows the true speed, its integer val -> idx for mac80211 to use
	// Ex: CONF_HW_RATE_INDEX_6MBPS -> speed: 6Mbps, its idx = 4 -> rate->idx = 4
}

#define WL18XX_TX_STATUS_DESC_ID_MASK    0x7F
#define WL18XX_TX_STATUS_STAT_BIT_IDX    7
static void wifi_tx_complete_packet(u8 tx_stat_byte)
{
	struct ieee80211_tx_info *info;
	struct sk_buff *skb;
	int id = tx_stat_byte & WL18XX_TX_STATUS_DESC_ID_MASK;
	bool tx_success;
	struct wifi_tx_hw_descr *tx_desc;

	/* check for id legality */
	if (unlikely(id >= WL18XX_NUM_TX_DESCRIPTORS || wifi_skb_tx_frames[id] == NULL)) {
		wifi_warning("illegal id in tx completion: %d", id);
		return;
	}

	/* a zero bit indicates Tx success */
	tx_success = !(tx_stat_byte & BIT(WL18XX_TX_STATUS_STAT_BIT_IDX));

	skb = wifi_skb_tx_frames[id];
	info = IEEE80211_SKB_CB(skb);
	tx_desc = (struct wifi_tx_hw_descr *)skb->data;

	if (wifi_is_dummy_packet(skb)) {
		wifi_free_tx_id(id);
		return;
	}

	/* update the TX status info */
	if (tx_success && !(info->flags & IEEE80211_TX_CTL_NO_ACK))
		info->flags |= IEEE80211_TX_STAT_ACK;
	/*
	 * first pass info->control.vif while it's valid, and then fill out
	 * the info->status structures
	 */
	wifi_get_last_tx_rate(info->control.vif,
				info->band,
				&info->status.rates[0],
				tx_desc->hlid);

	info->status.rates[0].count = 1; /* no data about retries */
	info->status.ack_signal = -1;

	/*
	 * TODO: update sequence number for encryption? seems to be
	 * unsupported for now. needed for recovery with encryption.
	 */

	/* remove private header from packet */
	skb_pull(skb, sizeof(struct wifi_tx_hw_descr));

#if (PRINT_DEBUG_DATA_FRAME)
	WIFI_Print_Hex(skb->data, (TX_LIMIT < skb->len) ? TX_LIMIT : skb->len, "TX frame:");
#endif

	/* return the packet to the stack */
	skb_queue_tail(&wifi_deferred_tx_queue, skb);
	//queue_work(wifi_work.freezable_wq, &wifi_data->netstack_work);
	queue_work(wifi_work.freezable_wq, &wifi_work.netstack_work);

	wifi_free_tx_id(id);
}

void wifi_tx_immediate_complete(void)
{
	u8 i, hlid;

	/* nothing to do here */
	if (wifi_last_fw_rls_idx == wifi_status_reg->fw_release_idx)
		return;

	/* update rates per link */
	hlid = wifi_status_reg->hlid;

	//printk("fw_release_idx = %d, hlid = %d\n", status_priv->fw_release_idx, hlid);

	if (hlid < WLCORE_MAX_LINKS) {
		wifi_links[hlid].fw_rate_idx =
				wifi_status_reg->tx_last_rate;
		wifi_links[hlid].fw_rate_mbps =
				wifi_status_reg->tx_last_rate_mbps;
	}

	/* freed Tx descriptors */
#if (PRINT_DEBUG)
	printk("last released desc = %d, current idx = %d",
	 	     wifi_last_fw_rls_idx, wifi_status_reg->fw_release_idx);
#endif
	if (wifi_status_reg->fw_release_idx >= WL18XX_FW_MAX_TX_STATUS_DESC) {
		wifi_error("invalid desc release index %d",
			     wifi_status_reg->fw_release_idx);
		WARN_ON(1);
		return;
	}

	for (i = wifi_last_fw_rls_idx;
	    //i != status_priv->fw_release_idx;
		i != wifi_status_reg->fw_release_idx;
	     i = (i + 1) % WL18XX_FW_MAX_TX_STATUS_DESC) {
		wifi_tx_complete_packet(wifi_status_reg->released_tx_desc[i]);

	}

	wifi_last_fw_rls_idx = wifi_status_reg->fw_release_idx;
}

#define WL1271_IRQ_MAX_LOOPS 256

static int wifi_irq_locked(void)
{
	int ret = 0;
	u32 intr;
	int loopcount = WL1271_IRQ_MAX_LOOPS;
	bool run_tx_queue = true;
	bool done = false;
	unsigned int defer_count;
	unsigned long flags;

	// /*
	//  * In case edge triggered interrupt must be used, we cannot iterate
	//  * more than once without introducing race conditions with the hardirq.
	//  */
	// if (wifi_data->irq_flags & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING))
	// 	loopcount = 1;

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out;
	}

	loopcount = 1;
	while (!done && loopcount--) {
		smp_mb__after_atomic();

		ret = wificore_fw_status(); // Read wifi_status_reg
		if (ret < 0)
			goto err_ret;

		wifi_tx_immediate_complete(); // Handle TX frame completed

		intr = wifi_status_reg->intr;
		intr &= WLCORE_ALL_INTR_MASK;
		if (!intr) {
			done = true;
			continue;
		}

		if (likely(intr & WL1271_ACX_INTR_DATA)) {
			wifi_debug(DEBUG_IRQ, "WL1271_ACX_INTR_DATA");

			ret = wificore_rx();
			if (ret < 0)
				goto err_ret;

			/* Check if any tx blocks were freed */
			if (spin_trylock_irqsave(&wifi_data->lock, flags)) {
				if (!wifi_tx_total_queue_count())
					run_tx_queue = false;
				spin_unlock_irqrestore(&wifi_data->lock, flags);
			}

			// Check if tx_queue in the current link is zero or not
			if (run_tx_queue) {
				ret = wificore_tx_work_locked(); // Handle TX frame completed
				if (ret < 0)
					goto err_ret;
			}

			/* Make sure the deferred queues don't get too long */
			defer_count = skb_queue_len(&wifi_deferred_tx_queue) +
				      skb_queue_len(&wifi_deferred_rx_queue);
			if (defer_count > WL1271_DEFERRED_QUEUE_LIMIT){
				struct sk_buff *skb;

				/* Pass all received frames to the network stack */
				while ((skb = skb_dequeue(&wifi_deferred_rx_queue)))
					ieee80211_rx_ni(wifi_data->hw, skb);

				/* Return sent skbs to the network stack */
				while ((skb = skb_dequeue(&wifi_deferred_tx_queue)))
					ieee80211_tx_status_ni(wifi_data->hw, skb);
			}
		}

		if (intr & WL1271_ACX_INTR_EVENT_A) {
			wifi_debug(DEBUG_IRQ, "WL1271_ACX_INTR_EVENT_A");
			ret = wifi_event_handle(0);
			if (ret < 0)
				goto err_ret;
		}

		if (intr & WL1271_ACX_INTR_EVENT_B) {
			wifi_debug(DEBUG_IRQ, "WL1271_ACX_INTR_EVENT_B");
			ret = wifi_event_handle(1);
			if (ret < 0)
				goto err_ret;
		}

		if (intr & WL1271_ACX_INTR_INIT_COMPLETE)
			wifi_debug(DEBUG_IRQ,
				     "WL1271_ACX_INTR_INIT_COMPLETE");

		if (intr & WL1271_ACX_INTR_HW_AVAILABLE)
			wifi_debug(DEBUG_IRQ, "WL1271_ACX_INTR_HW_AVAILABLE");
	}

err_ret:
	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);

out:
	return ret;
}

static irqreturn_t wificore_irq(int irq, void *cookie)
{
	int ret;
	unsigned long flags;
	bool queue_tx_work = true;

	set_bit(WL1271_FLAG_IRQ_RUNNING, &wifi_data->flags);

	/* TX might be handled here, avoid redundant work */
	set_bit(WL1271_FLAG_TX_PENDING, &wifi_data->flags);
	cancel_work_sync(&wifi_work.tx_work);

	mutex_lock(&wifi_data->mutex);

	ret = wifi_irq_locked();

	/* In case TX was not handled in wificore_irq_locked(), queue TX work */
	clear_bit(WL1271_FLAG_TX_PENDING, &wifi_data->flags);
	if (spin_trylock_irqsave(&wifi_data->lock, flags)) {
		if (!wifi_tx_total_queue_count()) // counts of all Frames scheduled for transmission, not handled yet
												// count += wifi_tx_queue_count[i];
			queue_tx_work = false;
		spin_unlock_irqrestore(&wifi_data->lock, flags);
	}

	// wifi_tx_queue_count is non-zero -> there are frames that not handled yet 
	if (queue_tx_work)
		ieee80211_queue_work(wifi_data->hw, &wifi_work.tx_work);
	//}

	mutex_unlock(&wifi_data->mutex);

	clear_bit(WL1271_FLAG_IRQ_RUNNING, &wifi_data->flags);

	return IRQ_HANDLED;
}

static void wifi_vif_count_iter(void *data, u8 *mac,
				  struct ieee80211_vif *vif)
{
#if (PRINT_DEBUG_INIT)
	struct wifi_vif *wifi_vif;
	wifi_vif = wifi_vif_to_data(vif);
	printk("[0] - ACTIVE - wifi_vif = 0x%x\n", wifi_vif);
#endif
}

static int wifi_fetch_firmware(bool plt)
{
	const struct firmware *fw;
	const char *fw_name;
	enum wifi_fw_type fw_type;
	int ret;

	fw_type = WL12XX_FW_TYPE_NORMAL;
	fw_name = wifi_data->sr_fw_name; // name of signle role

	if (wifi_data->fw_type == fw_type){
#if (PRINT_DEBUG)
		printk("SAME fw_type\n");
#endif
		return 0;
	}

	printk("booting firmware '%s', fw_type = %d", fw_name, fw_type);

	ret = request_firmware(&fw, fw_name, wifi_data->dev);

	if (ret < 0) {
		wifi_error("could not get firmware %s: %d", fw_name, ret);
		return ret;
	}

	if (fw->size % 4) {
		wifi_error("firmware size is not multiple of 32 bits: %zu",
			     fw->size);
		ret = -EILSEQ;
		goto out;
	}

	vfree(wifi_data->fw);
	wifi_data->fw_type = WL12XX_FW_TYPE_NONE;
	wifi_data->fw_len = fw->size;
	wifi_data->fw = vmalloc(wifi_data->fw_len);

	if (!wifi_data->fw) {
		wifi_error("could not allocate memory for the firmware");
		ret = -ENOMEM;
		goto out;
	}

	memcpy(wifi_data->fw, fw->data, wifi_data->fw_len);
	ret = 0;
	wifi_data->fw_type = fw_type;
out:
	release_firmware(fw);

	return ret;
}

void wifi_queue_recovery_work(void)
{
	printk("wifi_queue_recovery_work -> SHOULD RESTART\n");
}

static void wificore_save_freed_pkts(struct wifi_vif *wifi_vif,
				   u8 hlid, struct ieee80211_sta *sta)
{
	struct wifi_station *wl_sta;
	u32 sqn_recovery_padding = WL1271_TX_SQN_POST_RECOVERY_PADDING;

	wl_sta = (void *)sta->drv_priv;
	wl_sta->total_freed_pkts = wifi_links[hlid].total_freed_pkts;

	/*
	 * increment the initial seq number on recovery to account for
	 * transmitted packets that we haven't yet got in the FW status
	 */
	if (wifi_vif->encryption_type == KEY_GEM)
		sqn_recovery_padding = WL1271_TX_SQN_POST_RECOVERY_PADDING_GEM;

	if (test_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wifi_data->flags))
		wl_sta->total_freed_pkts += sqn_recovery_padding;
}

static int wifi_setup(void)
{
	/* Vinh custom */
	wifi_status_reg = kzalloc(sizeof(struct wifi_wl18xx_fw_status), GFP_KERNEL);
	if (!wifi_status_reg)
		goto err;

	return 0;
err:
	kfree(wifi_status_reg);
	return -ENOMEM;
}

// #include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
// #include <linux/mmc/sdio_ids.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

static int V_power_on(void)
{
	int ret;
	struct sdio_func *func = dev_to_sdio_func(wifi_data->dev->parent);
	struct mmc_card *card = func->card;
	//printk("V_power_on -> 0x%x 0x%x %x\n", wifi_data->dev->parent, func, card);

	ret = pm_runtime_get_sync(&card->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(&card->dev);
		printk("Failed -> V_power_on\n");

		return ret;
	}

	sdio_claim_host(func);

	mmc_hw_reset(card->host);
	sdio_enable_func(func);

	sdio_release_host(func);

	return 0;
}

int V_power_off(void)
{
	int ret;
	struct sdio_func *func = dev_to_sdio_func(wifi_data->dev->parent);
	struct mmc_card *card = func->card;
	//printk("V_power_on -> 0x%x 0x%x %x\n", wifi_data->dev->parent, func, card);

	ret = pm_runtime_get_sync(&card->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(&card->dev);
		printk("Failed -> V_power_on\n");

		return ret;
	}

	sdio_claim_host(func);
	sdio_disable_func(func);
	sdio_release_host(func);

	/* Let runtime PM know the card is powered off */
	pm_runtime_put(&card->dev);

	return 0;
}

static int wifi_set_power_on(void)
{
	int ret;

	msleep(WL1271_PRE_POWER_ON_SLEEP);
	ret = V_power_on();
	if (ret < 0)
		goto out;
	msleep(WL1271_POWER_ON_SLEEP);

	ret = wifi_set_partition_core(&wifi_data->ptable[PART_BOOT]);
	if (ret < 0)
		goto fail;

	/* ELP module wake up = Enhanced Low Power */
	wifi_sdio_raw_write(HW_ACCESS_ELP_CTRL_REG, ELPCTRL_WAKE_UP, sizeof(ELPCTRL_WAKE_UP), false);
	if (ret < 0)
		goto fail;

out:
	return ret;

fail:
	V_power_off();
	return ret;
}

static int wifi_chip_wakeup(bool plt)
{
	int ret = 0;

	ret = wifi_set_power_on();
	if (ret < 0)
		goto out;

	/*
	 * For wl127x based devices we could use the default block
	 * size (512 bytes), but due to a bug in the sdio driver, we
	 * need to set it explicitly after the chip is powered on.  To
	 * simplify the code and since the performance impact is
	 * negligible, we use the same block size for all different
	 * chip types.
	 *
	 * Check if the bus supports blocksize alignment and, if it
	 * doesn't, make sure we don't have the quirk.
	 */

	wifi_sdio_set_block_size(WL12XX_BUS_BLOCK_SIZE);

	/* TODO: make sure the lower driver has set things up correctly */
	ret = wifi_setup();
	if (ret < 0)
		goto out;

	ret = wifi_fetch_firmware(plt); // wifi_
	if (ret < 0) {
		kfree(wifi_status_reg);
	}

out:
	return ret;
}

static void wifi_op_tx(struct ieee80211_hw *hw,
			 struct ieee80211_tx_control *control,
			 struct sk_buff *skb)
{
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_vif *vif = info->control.vif;
	struct wifi_vif *wifi_vif = NULL;
	unsigned long flags;
	int q, mapping;
	u8 hlid;

	// Drop packet
	if (!vif) {
		wifi_debug(DEBUG_TX, "DROP skb with no vif");
		ieee80211_free_txskb(hw, skb);
		return;
	}

	wifi_vif = wifi_vif_to_data(vif);
	mapping = skb_get_queue_mapping(skb);
	q = mapping;

#if (PRINT_DEBUG)
	printk("q = %d, mapping = %d\n", q, mapping);
#endif

	hlid = wifi_tx_get_hlid(wifi_vif, skb, control->sta); // sta.hlid

	spin_lock_irqsave(&wifi_data->lock, flags);

	// Drop packet
	/*
	 * drop the packet if the link is invalid or the queue is stopped
	 * for any reason but watermark. Watermark is a "soft"-stop so we
	 * allow these packets through.
	 */
	if (hlid == WL12XX_INVALID_LINK_ID ||
	    (!test_bit(hlid, wifi_vif->links_map))
		) {
		wifi_debug(DEBUG_TX, "DROP skb hlid %d q %d", hlid, q);
		ieee80211_free_txskb(hw, skb);
		goto out;
	}


	skb_queue_tail(&wifi_tx_queue[hlid][q], skb);
	wifi_tx_queue_count[q]++;

	/*
	 * The chip specific setup must run before the first TX packet -
	 * before that, the tx_work will not be initialized!
	 */
	// If the TX work is not already busy or pending, schedule wifi_work.tx_work (which eventually calls wificore_tx_work_locked()
	// -> This is what triggers the actual transmission.
	if (!test_bit(WL1271_FLAG_TX_PENDING, &wifi_data->flags))
		ieee80211_queue_work(wifi_data->hw, &wifi_work.tx_work);

out:
	spin_unlock_irqrestore(&wifi_data->lock, flags);
}

/*
 * The size of the dummy packet should be at least 1400 bytes. However, in
 * order to minimize the number of bus transactions, aligning it to 512 bytes
 * boundaries could be beneficial, performance wise
 */
#define TOTAL_TX_DUMMY_PACKET_SIZE (ALIGN(1400, 512))

static struct sk_buff *wifi_alloc_dummy_packet(void)
{
	struct sk_buff *skb;
	struct ieee80211_hdr_3addr *hdr;
	unsigned int dummy_packet_size;

	dummy_packet_size = TOTAL_TX_DUMMY_PACKET_SIZE -
			    sizeof(struct wifi_tx_hw_descr) - sizeof(*hdr);

	skb = dev_alloc_skb(TOTAL_TX_DUMMY_PACKET_SIZE);
	if (!skb) {
		wifi_warning("Failed to allocate a dummy packet skb");
		return NULL;
	}

	skb_reserve(skb, sizeof(struct wifi_tx_hw_descr));

	hdr = skb_put_zero(skb, sizeof(*hdr));
	hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA |
					 IEEE80211_STYPE_NULLFUNC |
					 IEEE80211_FCTL_TODS);

	skb_put_zero(skb, dummy_packet_size);

	/* Dummy packets require the TID to be management */
	skb->priority = WL1271_TID_MGMT;

	/* Initialize all fields that might be used */
	skb_set_queue_mapping(skb, 0);
	memset(IEEE80211_SKB_CB(skb), 0, sizeof(struct ieee80211_tx_info));

	return skb;
}

static void wificore_op_stop_locked(void)
{
	int i;
#if (PRINT_DEBUG)
	printk("wificore_op_stop_locked\n");
#endif
	if (wifi_data->state == WLCORE_STATE_OFF) {
		if (test_and_clear_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS,
					&wifi_data->flags))
			wificore_enable_interrupts();

		return;
	}

	/*
	 * this must be before the cancel_work calls below, so that the work
	 * functions don't perform further work.
	 */
	wifi_data->state = WLCORE_STATE_OFF;

	/*
	 * Use the nosync variant to disable interrupts, so the mutex could be
	 * held while doing so without deadlocking.
	 */
	wificore_disable_interrupts_nosync(); // disable_irq_nosync(wifi_data->irq);

	mutex_unlock(&wifi_data->mutex);

	wificore_synchronize_interrupts(); // synchronize_irq(wifi_data->irq);
	// if (!test_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wifi_data->flags))
	// 	cancel_work_sync(&wifi_data->recovery_work);
	wifi_flush_deferred_work();
	cancel_delayed_work_sync(&wifi_work.scan_complete_work);
	//cancel_work_sync(&wifi_data->netstack_work);
	cancel_work_sync(&wifi_work.netstack_work);
	cancel_work_sync(&wifi_work.tx_work);
	cancel_delayed_work_sync(&wifi_work.tx_watchdog_work);

	/* let's notify MAC80211 about the remaining pending TX frames */
	mutex_lock(&wifi_data->mutex);
	wifi_tx_reset();

	wifi_power_off();
	/*
	 * In case a recovery was scheduled, interrupts were disabled to avoid
	 * an interrupt storm. Now that the power is down, it is safe to
	 * re-enable interrupts to balance the disable depth
	 */
	if (test_and_clear_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wifi_data->flags))
		wificore_enable_interrupts();

	//wifi_data->band = NL80211_BAND_2GHZ;

	wifi_rx_counter = 0;
	//wifi_data->power_level = WL1271_DEFAULT_POWER_LEVEL;
	//wifi_data->channel_type = NL80211_CHAN_NO_HT;
	wifi_tx_blocks_available = 0;
	wifi_tx_allocated_blocks = 0;
	//wifi_data->tx_results_count = 0;
	wifi_tx_packets_count = 0;
	wifi_time_offset = 0;
	//wifi_data->ap_fw_ps_map = 0;
	//wifi_data->ap_ps_map = 0;
	wifi_data->sleep_auth = WL1271_PSM_ILLEGAL;
	//memset(wifi_data->roles_map, 0, sizeof(wifi_data->roles_map));
	memset(wifi_map.links_map, 0, sizeof(wifi_map.links_map));
	memset(wifi_data->roc_map, 0, sizeof(wifi_data->roc_map));
	memset(wifi_session_ids, 0, sizeof(wifi_session_ids));
	memset(wifi_data->rx_filter_enabled, 0, sizeof(wifi_data->rx_filter_enabled));
	//wifi_data->active_sta_count = 0;
	//wifi_data->active_link_count = 0;

	/* The system link is always allocated */
	//wifi_links[WL12XX_SYSTEM_HLID].allocated_pkts = 0;
	wifi_allocated_pkts[WL12XX_SYSTEM_HLID] = 0;
	wifi_links[WL12XX_SYSTEM_HLID].prev_freed_pkts = 0;
	__set_bit(WL12XX_SYSTEM_HLID, wifi_map.links_map);

	/*
	 * this is performed after the cancel_work calls and the associated
	 * mutex_lock, so that wifi_op_add_interface does not accidentally
	 * get executed before all these vars have been reset.
	 */
	wifi_data->flags = 0;

	wifi_last_updated_tmp_tx_blocks_freed = 0;

	for (i = 0; i < NUM_TX_QUEUES; i++) {
		wifi_tx_pkts_freed[i] = 0;
		//wifi_data->tx_allocated_pkts[i] = 0;
		wifi_tx_allocated_pkts[i] = 0;
	}

	kfree(wifi_data->target_mem_map);
	wifi_data->target_mem_map = NULL;

	kfree(wifi_status_reg);
	wifi_status_reg = NULL;

	/*
	 * FW channels must be re-calibrated after recovery,
	 * save current Reg-Domain channel configuration and clear it.
	 */
	memcpy(wifi_data->reg_ch_conf_pending, wifi_data->reg_ch_conf_last,
	       sizeof(wifi_data->reg_ch_conf_pending));
	memset(wifi_data->reg_ch_conf_last, 0, sizeof(wifi_data->reg_ch_conf_last));
}

static void wificore_op_stop(struct ieee80211_hw *hw)
{
	wifi_debug(DEBUG_MAC80211, "mac80211 stop");

	mutex_lock(&wifi_data->mutex);

	wificore_op_stop_locked();

	mutex_unlock(&wifi_data->mutex);
}

static u8 wifi_get_role_type(struct wifi_vif *wifi_vif)
{
	struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);

	switch (wifi_vif->bss_type) {
	case BSS_TYPE_AP_BSS:
		if (wifi_vif->p2p)
			return WL1271_ROLE_P2P_GO;
		else if (ieee80211_vif_is_mesh(vif))
			return WL1271_ROLE_MESH_POINT;
		else
			return WL1271_ROLE_AP;

	case BSS_TYPE_STA_BSS:
		if (wifi_vif->p2p)
			return WL1271_ROLE_P2P_CL;
		else
			return WL1271_ROLE_STA;

	case BSS_TYPE_IBSS:
		return WL1271_ROLE_IBSS;

	default:
		wifi_error("invalid bss_type: %d", wifi_vif->bss_type);
	}
	return WL12XX_INVALID_ROLE_TYPE;
}

static int wifi_init_vif_data(struct ieee80211_vif *vif)
{
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);

	/* clear everything but the persistent data */
	memset(wifi_vif, 0, offsetof(struct wifi_vif, persistent));

	wifi_vif->bss_type = BSS_TYPE_STA_BSS;

	wifi_vif->role_id = WL12XX_INVALID_ROLE_ID;
	wifi_vif->dev_role_id = WL12XX_INVALID_ROLE_ID;
	wifi_vif->dev_hlid = WL12XX_INVALID_LINK_ID;

	/* init sta/ibss data */
	wifi_vif->sta.hlid = WL12XX_INVALID_LINK_ID;

	wifi_vif->basic_rate_set = CONF_TX_RATE_MASK_BASIC;

	wifi_vif->basic_rate = CONF_TX_RATE_MASK_BASIC;
	wifi_vif->rate_set = CONF_TX_RATE_MASK_BASIC;
	

	wifi_vif->bitrate_masks[NL80211_BAND_2GHZ] = CONF_HW_BIT_RATE_1MBPS;
	wifi_vif->bitrate_masks[NL80211_BAND_5GHZ] = CONF_HW_BIT_RATE_6MBPS;
	wifi_vif->beacon_int = WL1271_DEFAULT_BEACON_INT;

	/*
	 * mac80211 configures some values globally, while we treat them
	 * per-interface. thus, on init, we have to copy them from wl
	 */
	wifi_vif->band = NL80211_BAND_2GHZ;
	wifi_vif->channel = 0;
	wifi_vif->power_level = WL1271_DEFAULT_POWER_LEVEL;
	wifi_vif->channel_type = NL80211_CHAN_NO_HT;

	return 0;
}

static int wifi_init_fw(void)
{
	int retries = WL1271_BOOT_RETRIES;
	bool booted = false;
	struct wiphy *wiphy = wifi_data->hw->wiphy;
	int ret;

	while (retries) {
		retries--;
		// wifi_fetch_firmware
		ret = wifi_chip_wakeup(false); // wifi_
		if (ret < 0)
			goto power_off;

		// load fw (binary code) to Wifi chip
		// ret = request_firmware(&fw, fw_name, wifi_data->dev);
		// from /lib/firmware/ti-connectivity/wl18xx-fw-4.bin
		// addr = fw, data = fw->data, len = fw->len
		ret = wifi_data->ops->boot();
		if (ret < 0)
			goto power_off;

		ret = wifi_hw_init();
		if (ret < 0)
			goto irq_disable;

		booted = true;
		break;

irq_disable:
		mutex_unlock(&wifi_data->mutex);
		/* Unlocking the mutex in the middle of handling is
		   inherently unsafe. In this case we deem it safe to do,
		   because we need to let any possibly pending IRQ out of
		   the system (and while we are WLCORE_STATE_OFF the IRQ
		   work function will not do anything.) Also, any other
		   possible concurrent operations will fail due to the
		   current state, hence the wl1271 struct should be safe. */
		wificore_disable_interrupts();
		wifi_flush_deferred_work();
		//cancel_work_sync(&wifi_data->netstack_work);
		cancel_work_sync(&wifi_work.netstack_work);
		mutex_lock(&wifi_data->mutex);
power_off:
		wifi_power_off();
	}

	if (!booted) {
		wifi_error("firmware boot failed despite %d retries",
			     WL1271_BOOT_RETRIES);
		goto out;
	}

	printk("firmware booted (%s)", wifi_chip->fw_ver_str);

	/* update hw/fw version info in wiphy struct */
	wiphy->hw_version = wifi_chip->id;
	strncpy(wiphy->fw_version, wifi_chip->fw_ver_str,
		sizeof(wiphy->fw_version));

	/*
	 * Now we know if 11a is supported (info from the NVS), so disable
	 * 11a channels if not supported
	 */
	if (!wifi_data->enable_11a)
		wiphy->bands[NL80211_BAND_5GHZ]->n_channels = 0;

	wifi_debug(DEBUG_MAC80211, "11a is %ssupported",
		     wifi_data->enable_11a ? "" : "not ");

	wifi_data->state = WLCORE_STATE_ON;
out:
	return ret;
}

static int wificore_allocate_hw_queue_base(struct wifi_vif *wifi_vif)
{
	struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);
	//struct wificore_hw_queue_iter_data iter_data = {};
	int i;

	if (vif->type == NL80211_IFTYPE_P2P_DEVICE) {
		vif->cab_queue = IEEE80211_INVAL_HW_QUEUE;
		return 0;
	}

	for (i = 0; i < NUM_TX_QUEUES; i++) {
		wifi_data->queue_stop_reasons[HW_QUEUE_BASE + i] = 0;
		/* register hw queues in mac80211 */
		vif->hw_queue[i] = HW_QUEUE_BASE + i;
	}

	/* the last places are reserved for cab queues per interface */
	vif->cab_queue = IEEE80211_INVAL_HW_QUEUE;

	return 0;
}

static int wifi_op_add_interface(struct ieee80211_hw *hw,
				   struct ieee80211_vif *vif)
{
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	int ret = 0;
	u8 role_type;
	int i = 0, exist = 0;

	vif->driver_flags |= IEEE80211_VIF_BEACON_FILTER |
			     IEEE80211_VIF_SUPPORTS_UAPSD |
			     IEEE80211_VIF_SUPPORTS_CQM_RSSI;

	wifi_debug(DEBUG_MAC80211, "mac80211 add interface type %d mac %pM",
		     ieee80211_vif_type_p2p(vif), vif->addr);

#if (PRINT_DEBUG_INIT)
	printk("[0] - wifi_op_add_interface - vif->drv_priv = 0x%x\n", vif->drv_priv);
#endif

	// Call this to know this wifi_vif is active or not
	ieee80211_iterate_active_interfaces(hw, IEEE80211_IFACE_ITER_RESUME_ALL,
					    wifi_vif_count_iter, NULL);

	mutex_lock(&wifi_data->mutex);

	/*
	 * in some very corner case HW recovery scenarios its possible to
	 * get here before __wifi_op_remove_interface is complete, so
	 * opt out if that is the case.
	 */
	if (test_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wifi_data->flags) ||
	    test_bit(wifi_vif_FLAG_INITIALIZED, &wifi_vif->flags)) {
		ret = -EBUSY;
		goto out;
	}

	ret = wifi_init_vif_data(vif);
	if (ret < 0)
		goto out;

	role_type = wifi_get_role_type(wifi_vif);
	if (role_type == WL12XX_INVALID_ROLE_TYPE) {
		ret = -EINVAL;
		goto out;
	}

	ret = wificore_allocate_hw_queue_base(wifi_vif);
	if (ret < 0)
		goto out;

	/*
	 * TODO: after the nvs issue will be solved, move this block
	 * to start(), and make sure here the driver is ON.
	 */
	if (wifi_data->state == WLCORE_STATE_OFF) {
		/*
		 * we still need this in order to configure the fw
		 * while uploading the nvs
		 */
		memcpy(wifi_data->addresses[0].addr, vif->addr, ETH_ALEN);
#if (PRINT_DEBUG_INIT)
		printk("[1] - PROGRESS - wifi_init_fw\n");
#endif
		ret = wifi_init_fw();
		if (ret < 0)
			goto out;

#if (PRINT_DEBUG_INIT)
	printk("[1] - DONE - wifi_init_fw\n");
#endif
	}
	/*
	 * Call runtime PM only after possible wifi_init_fw() above
	 * is done. Otherwise we do not have interrupts enabled.
	 */
	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out_unlock;
	}

	// no need fw change as we use single role

	if (!(vif->type == NL80211_IFTYPE_P2P_DEVICE)) {
		ret = wifi_cmd_role_enable(vif->addr,
					     role_type, &wifi_vif->role_id); // WL1271_ROLE_STA
		if (ret < 0)
			goto out;
		//printk("IF - wifi_vif->role_id = %d\n", wifi_vif->role_id);
		ret = wifi_init_vif_specific(vif);
		if (ret < 0)
			goto out;

	} else {
		ret = wifi_cmd_role_enable(vif->addr, WL1271_ROLE_DEVICE,
					     &wifi_vif->dev_role_id);
		if (ret < 0)
			goto out;
		//printk("ELSE - wifi_vif->dev_role_id = %d\n", wifi_vif->dev_role_id);
		/* needed mainly for configuring rate policies */
		ret = wifi_sta_hw_init(wifi_vif);
		if (ret < 0)
			goto out;
	}

	/* wifi_vif_ptr is set global to use for scanning, configure_filter */
	for (i = 0; i < wifi_vif_ptr_id; i++){
		if (wifi_vif_ptr[i] == wifi_vif) exist = 1;
	}

	if (!exist){
		wifi_vif_ptr[wifi_vif_ptr_id++] = (struct wifi_vif *)wifi_vif;
	}
	
	set_bit(wifi_vif_FLAG_INITIALIZED, &wifi_vif->flags);

	wifi_data->sta_count++;

out:
	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);
out_unlock:
	mutex_unlock(&wifi_data->mutex);

	return ret;
}

static void __wifi_op_remove_interface(
					 struct ieee80211_vif *vif,
					 bool reset_tx_queues)
{
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	int ret;
	//bool is_ap = (wifi_vif->bss_type == BSS_TYPE_AP_BSS);

	wifi_debug(DEBUG_MAC80211, "mac80211 remove interface");

	if (!test_and_clear_bit(wifi_vif_FLAG_INITIALIZED, &wifi_vif->flags))
		return;

	/* because of hardware recovery, we may get here twice */
	if (wifi_data->state == WLCORE_STATE_OFF)
		return;

	wifi_info("down");

	if (wifi_data->scan_state != WL1271_SCAN_STATE_IDLE )
	    // && wifi_data->scan_wifi_vif == wifi_vif) 
		{
		struct cfg80211_scan_info info = {
			.aborted = true,
		};

		/*
		 * Rearm the tx watchdog just before idling scan. This
		 * prevents just-finished scans from triggering the watchdog
		 */
		wifi_rearm_tx_watchdog_locked();

		wifi_data->scan_state = WL1271_SCAN_STATE_IDLE;
		ieee80211_scan_completed(wifi_data->hw, &info);
	}
#if (PRINT_DEBUG)
	printk("[MAIN] - 0x%x vs 0x%x\n", wifi_data->sched_vif, wifi_vif);
#endif
	if (wifi_data->sched_vif == wifi_vif)
		wifi_data->sched_vif = NULL;

	if (wifi_data->roc_vif == vif) {
		wifi_data->roc_vif = NULL;
		ieee80211_remain_on_channel_expired(wifi_data->hw);
	}

	if (!test_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wifi_data->flags)) {
		/* disable active roles */
		ret = pm_runtime_get_sync(wifi_data->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(wifi_data->dev);
			goto deinit;
		}

		if (!wificore_is_p2p_mgmt(wifi_vif)) {
			ret = wifi_cmd_role_disable(&wifi_vif->role_id);
			if (ret < 0) {
				pm_runtime_put_noidle(wifi_data->dev);
				goto deinit;
			}
		} else {
			ret = wifi_cmd_role_disable(&wifi_vif->dev_role_id);
			if (ret < 0) {
				pm_runtime_put_noidle(wifi_data->dev);
				goto deinit;
			}
		}

		pm_runtime_mark_last_busy(wifi_data->dev);
		pm_runtime_put_autosuspend(wifi_data->dev);
	}
deinit:
	wifi_tx_reset_wifi_vif(wifi_vif);

	/* clear all hlids (except system_hlid) */
	wifi_vif->dev_hlid = WL12XX_INVALID_LINK_ID;

	wifi_vif->sta.hlid = WL12XX_INVALID_LINK_ID;

	dev_kfree_skb(wifi_vif->probereq);
	wifi_vif->probereq = NULL;
	wifi_vif->role_id = WL12XX_INVALID_ROLE_ID;
	wifi_vif->dev_role_id = WL12XX_INVALID_ROLE_ID;

	wifi_data->sta_count--;

	/*
	 * Last AP, have more stations. Configure sleep auth according to STA.
	 * Don't do thin on unintended recovery.
	 */
	if (test_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wifi_data->flags) &&
	    !test_bit(WL1271_FLAG_INTENDED_FW_RECOVERY, &wifi_data->flags))
		goto unlock;

unlock:
	mutex_unlock(&wifi_data->mutex);
	mutex_lock(&wifi_data->mutex);
}

static void wifi_op_remove_interface(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif)
{
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	//struct wifi_vif *iter;

	// Call this to know this if is active or not
	ieee80211_iterate_active_interfaces(hw, IEEE80211_IFACE_ITER_RESUME_ALL,
					    wifi_vif_count_iter, NULL);

	mutex_lock(&wifi_data->mutex);

	if (wifi_data->state == WLCORE_STATE_OFF ||
	    !test_bit(wifi_vif_FLAG_INITIALIZED, &wifi_vif->flags))
		goto out;

	/*
	 * wifi_data->vif can be null here if someone shuts down the interface
	 * just when hardware recovery has been started.
	 */
	__wifi_op_remove_interface(vif, true);
	// no need fw change as we use single role
out:
	mutex_unlock(&wifi_data->mutex);
}

static int wificore_join(struct wifi_vif *wifi_vif)
{
	int ret;

	/*
	 * One of the side effects of the JOIN command is that is clears
	 * WPA/WPA2 keys from the chipset. Performing a JOIN while associated
	 * to a WPA/WPA2 access point will therefore kill the data-path.
	 * Currently the only valid scenario for JOIN during association
	 * is on roaming, in which case we will also be given new keys.
	 * Keep the below message for now, unless it starts bothering
	 * users who really like to roam a lot :)
	 */
	if (test_bit(wifi_vif_FLAG_STA_ASSOCIATED, &wifi_vif->flags))
		printk("JOIN while associated.");

	/* clear encryption type */
	wifi_vif->encryption_type = KEY_NONE;

	ret = wifi_cmd_role_start_sta(wifi_vif);

	return ret;
}

static int wificore_set_ssid(struct wifi_vif *wifi_vif)
{
	struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);
	struct sk_buff *skb;
	int ieoffset;
	u8 ssid_len;
	const u8 *ptr;

	/* we currently only support setting the ssid from the ap probe req */
	if (wifi_vif->bss_type != BSS_TYPE_STA_BSS)
		return -EINVAL;

	// returns a fake Probe Request packet that contains the SSID
	skb = ieee80211_ap_probereq_get(wifi_data->hw, vif);
	if (!skb)
		return -EINVAL;

	ieoffset = offsetof(struct ieee80211_mgmt,
			    u.probe_req.variable);

	ptr = cfg80211_find_ie(WLAN_EID_SSID, skb->data + ieoffset,
					 skb->len - ieoffset);
	ssid_len = ptr[1];
	wifi_vif->ssid_len = ssid_len;
	memcpy(wifi_vif->ssid, ptr+2, ssid_len);
	// → ptr[0] = Element ID (WLAN_EID_SSID)
	// → ptr[1] = Length of SSID
	// → ptr+2  = Actual SSID string -> "64DVC" or "NGOC VY"

	dev_kfree_skb(skb);

	return 0;
}

static int wificore_set_assoc(struct wifi_vif *wifi_vif,
			    struct ieee80211_bss_conf *bss_conf,
			    u32 sta_rate_set)
{
	int ret;

	wifi_vif->aid = bss_conf->aid;
	wifi_vif->channel_type = cfg80211_get_chandef_type(&bss_conf->chandef);
	wifi_vif->beacon_int = bss_conf->beacon_int;
	wifi_vif->wmm_enabled = bss_conf->qos;

	set_bit(wifi_vif_FLAG_STA_ASSOCIATED, &wifi_vif->flags);

	/*
	 * with wl1271, we don't need to update the
	 * beacon_int and dtim_period, because the firmware
	 * updates it by itself when the first beacon is
	 * received after a join.
	 */
	ret = wifi_cmd_build_ps_poll(wifi_vif, wifi_vif->aid);
	if (ret < 0)
		return ret;

	/*
	 * Get a template for hardware connection maintenance
	 */
	// Re-play the step of getting ssid plus wifi_cmd_template_set - CMD_TEMPL_CFG_PROBE_REQ_2_4
	// The firmware uses this template when it needs to send a Probe Request (for scanning, roaming, reconnecting, etc.).
	// The template is stored in firmware memory instead of from CPU building
	dev_kfree_skb(wifi_vif->probereq);

	// synch_fail_thold + bss_lose_timeout
	// = How many consecutive beacons the firmware can miss before it considers the link "broken".
	// = Maximum time (in milliseconds) the firmware will wait without receiving any beacon from the AP before declaring the connection lost.
	/* enable the connection monitoring feature */
	ret = wifi_acx_conn_monit_params(wifi_vif, true);
	if (ret < 0)
		return ret;

	/*
	 * The join command disable the keep-alive mode, shut down its process,
	 * and also clear the template config, so we need to reset it all after
	 * the join. The acx_aid starts the keep-alive process, and the order
	 * of the commands below is relevant.
	 */
	// Make sure STA sending Null Data (wifi_cmd_build_klv_null_data) frames periodically
	ret = wifi_acx_keep_alive_mode(wifi_vif, true);
	if (ret < 0)
		return ret;

	// Sending AID to fw
	ret = wifi_acx_aid(wifi_vif, wifi_vif->aid);
	if (ret < 0)
		return ret;

	// Building a special Null Data template dedicated to Keep-Alive.
	ret = wifi_cmd_build_klv_null_data(wifi_vif);
	if (ret < 0)
		return ret;

	// main configuration command that tells the firmware how to use Keep-Alive.
	ret = wifi_acx_keep_alive_config(wifi_vif,
					   STA_KLV_TEMPLATE_IDX,
					   ACX_KEEP_ALIVE_TPL_VALID);
	if (ret < 0)
		return ret;

	/*
	 * The default fw psm configuration is AUTO, while mac80211 default
	 * setting is off (ACTIVE), so sync the fw with the correct value.
	 */
	ret = wifi_ps_set_mode(wifi_vif, STATION_ACTIVE_MODE);
	if (ret < 0)
		return ret;

	if (sta_rate_set) {
		wifi_vif->rate_set =
			wifi_tx_enabled_rates_get(
						    sta_rate_set,
						    wifi_vif->band);
		ret = wifi_acx_sta_rate_policies(wifi_vif);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static int wificore_unset_assoc(struct wifi_vif *wifi_vif)
{
	int ret;
	bool sta = wifi_vif->bss_type == BSS_TYPE_STA_BSS;

	/* make sure we are connected (sta) joined */
	if (sta &&
	    !test_and_clear_bit(wifi_vif_FLAG_STA_ASSOCIATED, &wifi_vif->flags))
		return false;

	/* make sure we are joined (ibss) */
	if (!sta &&
	    test_and_clear_bit(wifi_vif_FLAG_IBSS_JOINED, &wifi_vif->flags))
		return false;

	if (sta) {
		/* use defaults when not associated */
		wifi_vif->aid = 0;

		/* free probe-request template */
		dev_kfree_skb(wifi_vif->probereq);
		wifi_vif->probereq = NULL;

		/* disable connection monitor features */
		ret = wifi_acx_conn_monit_params(wifi_vif, false);
		if (ret < 0)
			return ret;

		/* Disable the keep-alive feature */
		ret = wifi_acx_keep_alive_mode(wifi_vif, false);
		if (ret < 0)
			return ret;

		/* disable beacon filtering */
		ret = wifi_acx_beacon_filter_opt(wifi_vif, false);
		if (ret < 0)
			return ret;
	}

	if (test_and_clear_bit(wifi_vif_FLAG_CS_PROGRESS, &wifi_vif->flags)) {
		struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);

		wifi_cmd_stop_channel_switch(wifi_vif);
		ieee80211_chswitch_done(vif, false);
	}

	/* invalidate keep-alive template */
	wifi_acx_keep_alive_config(wifi_vif,
					 STA_KLV_TEMPLATE_IDX,
				     ACX_KEEP_ALIVE_TPL_INVALID);

	return 0;
}

static void wifi_set_band_rate(struct wifi_vif *wifi_vif)
{
	wifi_vif->basic_rate_set = wifi_vif->bitrate_masks[wifi_vif->band];
	//printk("[basic_rate_set] = %d\n", wifi_vif->basic_rate_set);
	wifi_vif->rate_set = wifi_vif->basic_rate_set;
}

struct wifi_filter_params {
	bool enabled;
	int mc_list_length;
	u8 mc_list[ACX_MC_ADDRESS_GROUP_MAX][ETH_ALEN];
};

#define WL1271_SUPPORTED_FILTERS (FIF_ALLMULTI | \
				  FIF_FCSFAIL | \
				  FIF_BCN_PRBRESP_PROMISC | \
				  FIF_CONTROL | \
				  FIF_OTHER_BSS)


#if (PRINT_DEBUG_CONFIG_FILTER)
static void wifi_print_mc_list(struct wifi_filter_params *fp)
{
    int i;

    if (!fp) {
        printk(KERN_INFO "[FILTER] fp is NULL\n");
        return;
    }

    printk(KERN_INFO "[FILTER] Multicast Filter Config:\n");
    printk(KERN_INFO "[FILTER]   enabled       = %d\n", fp->enabled);
    printk(KERN_INFO "[FILTER]   mc_list_length = %d\n", fp->mc_list_length);

    if (!fp->enabled || fp->mc_list_length <= 0) {
        printk(KERN_INFO "[FILTER]   No multicast addresses configured\n");
        return;
    }

    for (i = 0; i < fp->mc_list_length && i < ACX_MC_ADDRESS_GROUP_MAX; i++) {
        printk(KERN_INFO "[FILTER]   MC[%2d] = %pM\n", 
               i, fp->mc_list[i]);
    }
}
#endif

static void wifi_op_configure_filter(struct ieee80211_hw *hw,
				       unsigned int changed,
				       unsigned int *total, u64 multicast)
{
	struct wifi_filter_params *fp = (void *)(unsigned long)multicast;
	int i = 0;

	int ret;

#if (PRINT_DEBUG_CONFIG_FILTER)
		printk("multicast = 0x%x\n", multicast);
		printk("mac80211 configure filter changed 0x%x"
		     " total 0x%x, supported_filters = 0x%x", changed, *total, WL1271_SUPPORTED_FILTERS);
		wifi_print_mc_list(fp);
#endif

	mutex_lock(&wifi_data->mutex);

	*total &= WL1271_SUPPORTED_FILTERS;
	changed &= WL1271_SUPPORTED_FILTERS;

	if (unlikely(wifi_data->state != WLCORE_STATE_ON))
		goto out;

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out;
	}

	for (i = 0; i < wifi_vif_ptr_id; i++){
		if (!wificore_is_p2p_mgmt(wifi_vif_ptr[i])){
#if (PRINT_DEBUG)
			printk("[FILTER] [%d] - bss = %d", i, wifi_vif_ptr[i]->bss_type);
#endif
			if (wifi_vif_ptr[i]->bss_type == BSS_TYPE_STA_BSS){
				if (*total & FIF_ALLMULTI)
					ret = wifi_acx_group_address_tbl(wifi_vif_ptr[i],
										false,
										NULL, 0);
				else if (fp)
					ret = wifi_acx_group_address_tbl(wifi_vif_ptr[i],
								fp->enabled,
								fp->mc_list,
								fp->mc_list_length);
				if (ret < 0)
					goto out_sleep;
			}
		}
	}

	/*
	 * the fw doesn't provide an api to configure the filters. instead,
	 * the filters configuration is based on the active roles / ROC
	 * state.
	 */

out_sleep:
	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);

out:
	mutex_unlock(&wifi_data->mutex);
	kfree(fp);
}

static int wifi_set_key(struct wifi_vif *wifi_vif,
		       u16 action, u8 id, u8 key_type,
		       u8 key_size, const u8 *key, u32 tx_seq_32,
		       u16 tx_seq_16, struct ieee80211_sta *sta,
		       bool is_pairwise)
{
	int ret;
	const u8 *addr;
	static const u8 bcast_addr[ETH_ALEN] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};

	addr = sta ? sta->addr : bcast_addr;

	if (is_zero_ether_addr(addr)) {
		/* We dont support TX only encryption */
		return -EOPNOTSUPP;
	}

	/* The wl1271 does not allow to remove unicast keys - they
		will be cleared automatically on next CMD_JOIN. Ignore the
		request silently, as we dont want the mac80211 to emit
		an error message. */
	if (action == KEY_REMOVE && !is_broadcast_ether_addr(addr))
		return 0;

	/* don't remove key if hlid was already deleted */
	if (action == KEY_REMOVE &&
		wifi_vif->sta.hlid == WL12XX_INVALID_LINK_ID)
		return 0;

	ret = wifi_cmd_set_sta_key(wifi_vif, action,
						id, key_type, key_size,
						key, addr, tx_seq_32,
						tx_seq_16);
	if (ret < 0)
		return ret;

	

	return 0;
}

static int wificore_op_set_key(struct ieee80211_hw *hw, enum set_key_cmd cmd,
			     struct ieee80211_vif *vif,
			     struct ieee80211_sta *sta,
			     struct ieee80211_key_conf *key_conf)
{
	int ret;

	mutex_lock(&wifi_data->mutex);

	if (unlikely(wifi_data->state != WLCORE_STATE_ON)) {
		ret = -EAGAIN;
		goto out_wake_queues;
	}

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out_wake_queues;
	}

	ret = wificore_set_key(cmd, vif, sta, key_conf);

	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);

out_wake_queues:
	mutex_unlock(&wifi_data->mutex);

	return ret;
}

int wificore_set_key(enum set_key_cmd cmd,
		   struct ieee80211_vif *vif,
		   struct ieee80211_sta *sta,
		   struct ieee80211_key_conf *key_conf)
{
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	int ret;
	u32 tx_seq_32 = 0;
	u16 tx_seq_16 = 0;
	u8 key_type;
	u8 hlid;
	bool is_pairwise;

	wifi_debug(DEBUG_MAC80211, "mac80211 set key");

	wifi_debug(DEBUG_CRYPT, "CMD: 0x%x sta: %p", cmd, sta);
	wifi_debug(DEBUG_CRYPT, "Key: algo:0x%x, id:%d, len:%d flags 0x%x",
		     key_conf->cipher, key_conf->keyidx,
		     key_conf->keylen, key_conf->flags);
	wifi_dump(DEBUG_CRYPT, "KEY: ", key_conf->key, key_conf->keylen);

	hlid = wifi_vif->sta.hlid;

	if (hlid != WL12XX_INVALID_LINK_ID) {
		u64 tx_seq = wifi_links[hlid].total_freed_pkts;
		tx_seq_32 = WL1271_TX_SECURITY_HI32(tx_seq);
		tx_seq_16 = WL1271_TX_SECURITY_LO16(tx_seq);
	}

	key_type = KEY_AES;
	key_conf->flags |= IEEE80211_KEY_FLAG_PUT_IV_SPACE;

	is_pairwise = key_conf->flags & IEEE80211_KEY_FLAG_PAIRWISE;

	switch (cmd) {
	case SET_KEY:
		ret = wifi_set_key(wifi_vif, KEY_ADD_OR_REPLACE,
				 key_conf->keyidx, key_type,
				 key_conf->keylen, key_conf->key,
				 tx_seq_32, tx_seq_16, sta, is_pairwise);
		if (ret < 0) {
			wifi_error("Could not add or replace key");
			return ret;
		}

		/*
		 * reconfiguring arp response if the unicast (or common)
		 * encryption key type was changed
		 */
		if (wifi_vif->bss_type == BSS_TYPE_STA_BSS &&
		    (sta || key_type == KEY_WEP) &&
		    wifi_vif->encryption_type != key_type) {
			wifi_vif->encryption_type = key_type;
			ret = wifi_cmd_build_arp_rsp(wifi_vif);
			if (ret < 0) {
				wifi_warning("build arp rsp failed: %d", ret);
				return ret;
			}
		}
		break;

	case DISABLE_KEY:
		ret = wifi_set_key(wifi_vif, KEY_REMOVE,
				     key_conf->keyidx, key_type,
				     key_conf->keylen, key_conf->key,
				     0, 0, sta, is_pairwise);
		if (ret < 0) {
			wifi_error("Could not remove key");
			return ret;
		}
		break;

	default:
		wifi_error("Unsupported key cmd 0x%x", cmd);
		return -EOPNOTSUPP;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(wificore_set_key);

void wificore_regdomain_config(void)
{
	int ret;

	// if (!(wifi_data->quirks & WLCORE_QUIRK_REGDOMAIN_CONF))
	// 	return;

	mutex_lock(&wifi_data->mutex);

	// if (unlikely(wifi_data->state != WLCORE_STATE_ON))
	// 	goto out;

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_autosuspend(wifi_data->dev);
		goto out;
	}

	ret = wificore_cmd_regdomain_config_locked();

	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);
out:
	mutex_unlock(&wifi_data->mutex);
}

static int wifi_op_hw_scan(struct ieee80211_hw *hw,
			     struct ieee80211_vif *vif,
			     struct ieee80211_scan_request *hw_req)
{
	struct cfg80211_scan_request *req = &hw_req->req;
	int ret;
	u8 *ssid = NULL;
	size_t len = 0;

#if (PRINT_DEBUG_SCAN)
	printk("========= wifi_op_hw_scan =========\n");
#endif

	if (req->n_ssids) {
		ssid = req->ssids[0].ssid;
		len = req->ssids[0].ssid_len;
	}

	mutex_lock(&wifi_data->mutex);

	if (unlikely(wifi_data->state != WLCORE_STATE_ON)) {
		/*
		 * We cannot return -EBUSY here because cfg80211 will expect
		 * a call to ieee80211_scan_completed if we do - in this case
		 * there won't be any call.
		 */
		ret = -EAGAIN;
		goto out;
	}

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out;
	}

	/* fail if there is any role in ROC */
	if (find_first_bit(wifi_data->roc_map, WL12XX_MAX_ROLES) < WL12XX_MAX_ROLES) {
		/* don't allow scanning right now */
		ret = -EBUSY;
		goto out_sleep;
	}

	ret = wificore_scan(vif, ssid, len, req);
out_sleep:
	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);
out:
	mutex_unlock(&wifi_data->mutex);

	return ret;
}

static int wifi_bss_erp_info_changed(
				       struct ieee80211_vif *vif,
				       struct ieee80211_bss_conf *bss_conf,
				       u32 changed)
{
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	int ret = 0;

	if (changed & BSS_CHANGED_ERP_SLOT) {
		if (bss_conf->use_short_slot)
			ret = wifi_acx_slot(wifi_vif, SLOT_TIME_SHORT);
		else
			ret = wifi_acx_slot(wifi_vif, SLOT_TIME_LONG);
		if (ret < 0) {
			wifi_warning("Set slot time failed %d", ret);
			goto out;
		}
	}

	if (changed & BSS_CHANGED_ERP_PREAMBLE) {
		if (bss_conf->use_short_preamble)
			wifi_acx_set_preamble(wifi_vif, ACX_PREAMBLE_SHORT);
		else
			wifi_acx_set_preamble(wifi_vif, ACX_PREAMBLE_LONG);
	}

	if (changed & BSS_CHANGED_ERP_CTS_PROT) {
		if (bss_conf->use_cts_prot)
			ret = wifi_acx_cts_protect(wifi_vif,
						     CTSPROTECT_ENABLE);
		else
			ret = wifi_acx_cts_protect(wifi_vif,
						     CTSPROTECT_DISABLE);
		if (ret < 0) {
			wifi_warning("Set ctsprotect failed %d", ret);
			goto out;
		}
	}

out:
	return ret;
}

static int wificore_set_bssid(struct wifi_vif *wifi_vif,
			    struct ieee80211_bss_conf *bss_conf,
			    u32 sta_rate_set)
{
	int ret;

	wifi_debug(DEBUG_MAC80211,
	     "changed_bssid: %pM, aid: %d, bcn_int: %d, brates: 0x%x sta_rate_set: 0x%x",
	     bss_conf->bssid, bss_conf->aid,
	     bss_conf->beacon_int,
	     bss_conf->basic_rates, sta_rate_set);

	// Beacon Interval - AP announce the network’s presence, SSID, supported rates, capabilities, timing information
	wifi_vif->beacon_int = bss_conf->beacon_int;

	if (sta_rate_set)
		wifi_vif->rate_set =
			wifi_tx_enabled_rates_get(
						sta_rate_set,
						wifi_vif->band);
	// raw:      sta_rate_set    -> 0xFF0FFF
	// firmware: wifi_vif->rate_set -> 0x1FFEFF -> map to HW bit enum (28 bit) in conf.h
	// -> maximum supported on AP: CONF_HW_BIT_RATE_MCS_7

	// Set rate policies -> STA_BASIC_RATE_IDX, STA_AP_RATE_IDX, STA_P2P_RATE_IDX
	ret = wifi_acx_sta_rate_policies(wifi_vif);
	if (ret < 0)
		return ret;

	ret = wifi_cmd_build_null_data(wifi_vif);
	if (ret < 0)
		return ret;

	ret = wifi_build_qos_null_data(wifi_wifi_vif_to_vif(wifi_vif));
	if (ret < 0)
		return ret;

	// get wifi_vif->ssid_len and ssid string
	wificore_set_ssid(wifi_vif); 

	set_bit(wifi_vif_FLAG_IN_USE, &wifi_vif->flags);

	return 0;
}

static int wificore_clear_bssid(struct wifi_vif *wifi_vif)
{
	int ret;

	/* revert back to minimum rates for the current band */
	wifi_set_band_rate(wifi_vif);
	wifi_vif->basic_rate = wifi_tx_min_rate_get(wifi_vif->basic_rate_set);

	ret = wifi_acx_sta_rate_policies(wifi_vif);
	if (ret < 0)
		return ret;

	if (wifi_vif->bss_type == BSS_TYPE_STA_BSS &&
	    test_bit(wifi_vif_FLAG_IN_USE, &wifi_vif->flags)) {
		ret = wifi_cmd_role_stop_sta(wifi_vif);
		if (ret < 0)
			return ret;
	}

	clear_bit(wifi_vif_FLAG_IN_USE, &wifi_vif->flags);
	return 0;
}

/*
[ 6413.330376] wlan0: authenticate with f4:27:56:13:90:d8
[ 6413.347031] wifi_bss_info_changed_sta, [0x40000, 0]
[ 6413.352278] wifi_bss_info_changed_sta, [0x4000, 0] 						-> BSS_CHANGED_IDLE
[ 6413.357341] STATE - 1
[ 6413.359660] wifi_bss_info_changed_sta, [0xe0, 0]  						-> BSS_CHANGED_BSSID
[ 6413.364503] STATE - 3, sta = 0x0
[ 6413.366811] STATE - 4 - SET							=> wifi_cmd_role_start_sta



[ 6413.394130] wlan0: send auth to f4:27:56:13:90:d8 (try 1/3)
[ 6413.422809] wlan0: authenticated
[ 6413.434633] wlan0: associate with f4:27:56:13:90:d8 (try 1/3)
[ 6413.455721] wlan0: RX AssocResp from f4:27:56:13:90:d8 (capab=0x
1431 status=0 aid=1)
[ 6413.463696] wifi_bss_info_changed_sta, [0x10200d, 0] 						-> BSS_CHANGED_ASSOC
[ 6413.468932] STATE - 3, sta = 0xca7267a0
[ 6413.471242] STATE - 5
[ 6413.496488] STATE - 6
[ 6413.520038] STATE - 7


[ 6413.523490] wlan0: associated
[ 6413.564523] wifi_bss_info_changed_sta, [0x10, 0] 								-> BSS_CHANGED_HT
[ 6413.569525] STATE - 3, sta = 0xca7267a0
[ 6413.611124] IPv6: ADDRCONF(NETDEV_CHANGE): wlan0: link becomes r
eady
[ 6413.635245] wifi_bss_info_changed_sta, [0x400, 0] 							-> BSS_CHANGED_CQM
[ 6413.640347] STATE - 2


[ 6413.649453] wifi0: Association completed.
[ 6413.656209] wifi_bss_info_changed_sta, [0x20000, 0]
[ 6413.815010] wifi_bss_info_changed_sta, [0x1000, 0] 							-> BSS_CHANGED_ARP_FILTER
[ 6413.820154] STATE - 7





debian@arm:~$ [ 7561.666865] wlan0: deauthenticating from f4:27:56:
13:90:d8 by local choice (Reason: 3=DEAUTH_LEAVING)
[ 7561.676309] wifi_bss_info_changed_sta, [0x20000, 0]
[ 7561.681789] wifi_bss_info_changed_sta, [0x80309f, 0]  					-> BSS_CHANGED_ASSOC |  BSS_CHANGED_BSSID
[ 3282.687947] STATE - 3, sta = 0x0
[ 3282.700392] STATE - 4 - CLEAR
[ 3282.706596] STATE - 6
[ 3282.715132] STATE - 7
[ 7561.785507] wifi_bss_info_changed_sta, [0x4000, 0]
[ 7561.790582] STATE - 1
[ 7561.881982] wifi0: down
[ 7561.953422] wifi_bss_info_changed_sta, [0xe, 0]
[ 7561.962877] wifi_bss_info_changed_sta, [0x2000, 0]
[ 7561.967950] STATE - 7
[ 7562.315485] wifi0: down
[ 7562.377046] wifi_bss_info_changed_sta, [0xe, 0]
[ 7562.386298] wifi_bss_info_changed_sta, [0x2000, 0]
[ 7562.391415] STATE - 7
*/

/*
1 -> 3 -> 4
3 -> 5 -> 6 -> 7
3 -> 2 -> 7 ...

3 -> 4 -> 6 -> 7
*/

/* STA/IBSS mode changes */
static void wifi_bss_info_changed_sta(
					struct ieee80211_vif *vif,
					struct ieee80211_bss_conf *bss_conf,
					u32 changed)
{
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	bool do_join = false;
	bool is_ibss = (wifi_vif->bss_type == BSS_TYPE_IBSS);
	//bool ibss_joined = false;
	u32 sta_rate_set = 0;
	int ret;
	struct ieee80211_sta *sta;
	bool sta_exists = false;
	struct ieee80211_sta_ht_cap sta_ht_cap;

#if (PRINT_DEBUG_ROC)
	printk("info_changed_sta, [0x%x, %d]\n", changed, is_ibss);
#endif

	if (changed & BSS_CHANGED_IDLE && !is_ibss){
#if (PRINT_DEBUG_ROC)
		printk("[BSS_STATE] - 1\n");
#endif
		set_bit(wifi_vif_FLAG_ACTIVE, &wifi_vif->flags);
	}

	// Connection Quality Monitor -> signal strength of the connected Access Point
	if (changed & BSS_CHANGED_CQM) {
#if (PRINT_DEBUG_ROC)
		printk("[BSS_STATE] - 2, %d, %d\n", bss_conf->cqm_rssi_thold, bss_conf->cqm_rssi_hyst);
#endif
		bool enable = false;
		if (bss_conf->cqm_rssi_thold) // RSSI threshold in dBm
			enable = true;
		ret = wifi_acx_rssi_snr_trigger(wifi_vif, enable,
						  bss_conf->cqm_rssi_thold,
						  bss_conf->cqm_rssi_hyst); 
		if (ret < 0)
			goto out;
	}

	// Info of AP  - sta rate cap
	if (changed & (BSS_CHANGED_BSSID | BSS_CHANGED_HT |
		       BSS_CHANGED_ASSOC)) {
		rcu_read_lock();
		sta = ieee80211_find_sta(vif, bss_conf->bssid); // info of AP
#if (PRINT_DEBUG_ROC)
		printk("[BSS_STATE] - 3, sta = 0x%x\n", sta);
#endif
		if (sta) {
			u8 *rx_mask = sta->ht_cap.mcs.rx_mask;

			/* save the supp_rates of the ap */
			sta_rate_set = sta->supp_rates[wifi_vif->band];
			if (sta->ht_cap.ht_supported)
				sta_rate_set |=
					(rx_mask[0] << HW_HT_RATES_OFFSET) |
					(rx_mask[1] << HW_MIMO_RATES_OFFSET);
			sta_ht_cap = sta->ht_cap;
			sta_exists = true;
		}

		rcu_read_unlock();
	}

	if (changed & BSS_CHANGED_BSSID) {
		if (!is_zero_ether_addr(bss_conf->bssid)) {
			// sta_rate_set = enabled_rates | ht_rates | mimo_rates -> vif->rate_set
			//  vif->rate_set -> wifi_acx_sta_rate_policies with rate set
			// Get wifi_vif->ssid, vif->ssid_len
			ret = wificore_set_bssid(wifi_vif, bss_conf,
					       sta_rate_set); 
			if (ret < 0)
				goto out;

			/* Need to update the BSSID (for filtering etc) */
			do_join = true;
#if (PRINT_DEBUG_ROC)
			printk("[BSS_STATE] - 4 - SET\n");
#endif
		} else {
			// wifi_acx_sta_rate_policies with rate set
			// Get wifi_vif->ssid, vif->ssid_len
			// wifi_cmd_role_stop_sta
			ret = wificore_clear_bssid(wifi_vif);
			if (ret < 0)
				goto out;
#if (PRINT_DEBUG_ROC)
			printk("[BSS_STATE] - 4 - CLEAR\n");
#endif
		}
	}

	// Traffic Indication Map.
	if ((changed & BSS_CHANGED_BEACON_INFO) && bss_conf->dtim_period) {
#if (PRINT_DEBUG_ROC)
		printk("[BSS_STATE] - 5\n");
#endif
		/* enable beacon filtering */
		ret = wifi_acx_beacon_filter_opt(wifi_vif, true);
		if (ret < 0)
			goto out;
	}

	// Extended Rate PHY
	ret = wifi_bss_erp_info_changed(vif, bss_conf, changed);
	if (ret < 0)
		goto out;

	if (do_join) {
		ret = wificore_join(wifi_vif);
		if (ret < 0) {
			wifi_warning("cmd join failed %d", ret);
			goto out;
		}
	}

	if (changed & BSS_CHANGED_ASSOC) {
#if (PRINT_DEBUG_ROC)
		printk("[BSS_STATE] - 6\n");
#endif
		if (bss_conf->assoc) {
			ret = wificore_set_assoc(wifi_vif, bss_conf,
					       sta_rate_set);
			if (ret < 0)
				goto out;

			if (test_bit(wifi_vif_FLAG_STA_AUTHORIZED, &wifi_vif->flags))
				wifi_set_authorized(wifi_vif);
		} else {
			wificore_unset_assoc(wifi_vif);
		}
	}

	/* Handle new association with HT. Do this after join. */
	if (sta_exists) {
		bool enabled =
			bss_conf->chandef.width != NL80211_CHAN_WIDTH_20_NOHT;

		ret = wifi_acx_set_peer_cap(
					     &sta_ht_cap,
					     enabled,
					     wifi_vif->rate_set,
					     wifi_vif->sta.hlid);
		if (ret < 0) {
			wifi_warning("Set ht cap failed %d", ret);
			goto out;

		}

		if (enabled) {
			ret = wifi_acx_set_ht_information(wifi_vif,
						bss_conf->ht_operation_mode);
			if (ret < 0) {
				wifi_warning("Set ht information failed %d",
					       ret);
				goto out;
			}
		}
	}

	// Its main job is to configure the firmware to automatically reply to ARP requests from the Access Point (or network) without waking up the host CPU every time.
	/* Handle arp filtering. Done after join. */
	if ((changed & BSS_CHANGED_ARP_FILTER) ||
	    (!is_ibss && (changed & BSS_CHANGED_QOS))) {
		__be32 addr = bss_conf->arp_addr_list[0];
		wifi_vif->sta.qos = bss_conf->qos;
		WARN_ON(wifi_vif->bss_type != BSS_TYPE_STA_BSS);

		if (bss_conf->arp_addr_cnt == 1 && bss_conf->assoc) {
			wifi_vif->ip_addr = addr;
#if (PRINT_DEBUG_ROC)
			printk("[BSS_STATE] - 7 - ENABLE\n");
#endif
			/*
			 * The template should have been configured only upon
			 * association. however, it seems that the correct ip
			 * isn't being set (when sending), so we have to
			 * reconfigure the template upon every ip change.
			 */
			ret = wifi_cmd_build_arp_rsp(wifi_vif);
			if (ret < 0) {
				wifi_warning("build arp rsp failed: %d", ret);
				goto out;
			}

			ret = wifi_acx_arp_ip_filter(wifi_vif,
				(ACX_ARP_FILTER_ARP_FILTERING |
				 ACX_ARP_FILTER_AUTO_ARP),
				addr);
		} else {
			wifi_vif->ip_addr = 0;
#if (PRINT_DEBUG_ROC)
			printk("[BSS_STATE] - 7 - DISABLE\n");
#endif
			ret = wifi_acx_arp_ip_filter(wifi_vif, 0, addr);
		}

		if (ret < 0)
			goto out;
	}

out:
	return;
}

static void wifi_op_bss_info_changed(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif,
				       struct ieee80211_bss_conf *bss_conf,
				       u32 changed)
{
	//struct wl1271 *wl = hw->priv;
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	int ret;

	mutex_lock(&wifi_data->mutex);

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out;
	}

	// change the transmit power of the radio.
	if ((changed & BSS_CHANGED_TXPOWER) &&
	    bss_conf->txpower != wifi_vif->power_level) {

		ret = wifi_acx_tx_power(wifi_vif, bss_conf->txpower);
		if (ret < 0)
			goto out;

		wifi_vif->power_level = bss_conf->txpower;
	}

	wifi_bss_info_changed_sta(vif, bss_conf, changed);

	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);

out:
	mutex_unlock(&wifi_data->mutex);
}

static int wificore_op_assign_vif_chanctx(struct ieee80211_hw *hw,
					struct ieee80211_vif *vif,
					struct ieee80211_chanctx_conf *ctx)
{
	//struct wl1271 *wl = hw->priv;
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	int channel = ieee80211_frequency_to_channel(
		ctx->def.chan->center_freq);
	int ret = -EINVAL;

#if (PRINT_DEBUG_SCAN)
	printk("SCAN DONE -> choose channel: %d, freq: %d\n", channel
						, ctx->def.chan->center_freq);
#endif

	mutex_lock(&wifi_data->mutex);

	if (unlikely(wifi_data->state != WLCORE_STATE_ON))
		goto out;

	if (unlikely(!test_bit(wifi_vif_FLAG_INITIALIZED, &wifi_vif->flags)))
		goto out;

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out;
	}

	wifi_vif->band = ctx->def.chan->band;
	wifi_vif->channel = channel;
	wifi_vif->channel_type = cfg80211_get_chandef_type(&ctx->def);

	/* update default rates according to the band */
	wifi_set_band_rate(wifi_vif);

	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);
out:
	mutex_unlock(&wifi_data->mutex);

	return 0;
}

void wifi_free_sta(struct wifi_vif *wifi_vif, u8 hlid)
{
	/*
	 * save the last used PN in the private part of iee80211_sta,
	 * in case of recovery/suspend
	 */
	struct ieee80211_sta *sta;
	struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);

	if (WARN_ON(hlid == WL12XX_INVALID_LINK_ID ||
		    is_zero_ether_addr(wifi_links[hlid].addr)))
		return;

	rcu_read_lock();
	sta = ieee80211_find_sta(vif, wifi_links[hlid].addr);
	if (sta)
		wificore_save_freed_pkts(wifi_vif, hlid, sta);
	rcu_read_unlock();


	wifi_free_link(wifi_vif, &hlid);
}

static int wifi_update_sta_state(
				   struct wifi_vif *wifi_vif,
				   struct ieee80211_sta *sta,
				   enum ieee80211_sta_state old_state,
				   enum ieee80211_sta_state new_state)
{
	struct wifi_station *wl_sta;
	bool is_sta = wifi_vif->bss_type == BSS_TYPE_STA_BSS;
	int ret;

	wl_sta = (struct wifi_station *)sta->drv_priv;

	// STEP UP	: 6 -> 4 -> 1 -> 5 = NOTEXIST -> NONE -> AUTH -> ASSOC -> AUTHORIZED
	// STEP DOWN: 2 -> 3 -> 5	   = AUTHORIZED -> ASSOC -> AUTH -> NOTEXIST
	/* Authorize station */
	if (is_sta &&
	    new_state == IEEE80211_STA_AUTHORIZED) {
#if (PRINT_DEBUG_ROC)
		printk("	[STA_STATE] - 1\n");
#endif
		set_bit(wifi_vif_FLAG_STA_AUTHORIZED, &wifi_vif->flags);
		ret = wifi_set_authorized(wifi_vif); // wifi_ -> Association completed.
		if (ret)
			return ret;
	}

	// WHEN deauthenticating = down with WIFI
	if (is_sta &&
	    old_state == IEEE80211_STA_AUTHORIZED &&
	    new_state == IEEE80211_STA_ASSOC) {
#if (PRINT_DEBUG_ROC)
		printk("	[STA_STATE] - 2\n");
#endif
		clear_bit(wifi_vif_FLAG_STA_AUTHORIZED, &wifi_vif->flags);
		clear_bit(wifi_vif_FLAG_STA_STATE_SENT, &wifi_vif->flags);
	}

	// the next step with down with WIFI
	/* save seq number on disassoc (suspend) */
	if (is_sta &&
	    old_state == IEEE80211_STA_ASSOC &&
	    new_state == IEEE80211_STA_AUTH) {
#if (PRINT_DEBUG_ROC)
		printk("	[STA_STATE] - 3\n");
#endif
		wificore_save_freed_pkts(wifi_vif, wifi_vif->sta.hlid, sta);
		wifi_vif->total_freed_pkts = 0;
	}

	/* restore seq number on assoc (resume) */
	if (is_sta &&
	    old_state == IEEE80211_STA_AUTH &&
	    new_state == IEEE80211_STA_ASSOC) {
#if (PRINT_DEBUG_ROC)
		printk("	[STA_STATE] - 4\n");
#endif
		wifi_vif->total_freed_pkts = wl_sta->total_freed_pkts;
	}

	/* clear ROCs on failure or authorization */
	// WHEN FULLY connected or disconnected -> ancel any pending "Remain On Channel"
	if (is_sta &&
	    (new_state == IEEE80211_STA_AUTHORIZED ||
	     new_state == IEEE80211_STA_NOTEXIST)) {
#if (PRINT_DEBUG_ROC)
		printk("	[STA_STATE] - 5\n");
#endif
		if (test_bit(wifi_vif->role_id, wifi_data->roc_map)){
			wifi_crocV(wifi_vif->role_id); // wifi_
		}
	}

	// WHEN not fully connected -> REMAIN on CHANNEL
	if (is_sta &&
	    old_state == IEEE80211_STA_NOTEXIST &&
	    new_state == IEEE80211_STA_NONE) {
#if (PRINT_DEBUG_ROC)
		printk("	[STA_STATE] - 6\n");
#endif
		if (find_first_bit(wifi_data->roc_map,
				   WL12XX_MAX_ROLES) >= WL12XX_MAX_ROLES) {
			WARN_ON(wifi_vif->role_id == WL12XX_INVALID_ROLE_ID);

			// printk("wifi_roc, roc_map = 0x%x\n", wifi_data->roc_map);
			wifi_roc(wifi_vif, wifi_vif->role_id,
				   wifi_vif->band, wifi_vif->channel); // wifi_
		}
	}
	return 0;
}

static int wifi_op_sta_state(struct ieee80211_hw *hw,
			       struct ieee80211_vif *vif,
			       struct ieee80211_sta *sta,
			       enum ieee80211_sta_state old_state,
			       enum ieee80211_sta_state new_state)
{
	//struct wl1271 *wl = hw->priv;
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	int ret;

	wifi_debug(DEBUG_MAC80211, "mac80211 sta %d state=%d->%d",
		     sta->aid, old_state, new_state);

	mutex_lock(&wifi_data->mutex);

	if (unlikely(wifi_data->state != WLCORE_STATE_ON)) {
		ret = -EBUSY;
		goto out;
	}

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto out;
	}

	ret = wifi_update_sta_state(wifi_vif, sta, old_state, new_state); // wifi_

	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);
out:
	mutex_unlock(&wifi_data->mutex);
	if (new_state < old_state)
		return 0;
	return ret;
}

/* can't be const, mac80211 writes to this */
static struct ieee80211_rate wifi_rates[] = {
	{ .bitrate = 10,
	  .hw_value = CONF_HW_BIT_RATE_1MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_1MBPS, },
	{ .bitrate = 20,
	  .hw_value = CONF_HW_BIT_RATE_2MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_2MBPS,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 55,
	  .hw_value = CONF_HW_BIT_RATE_5_5MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_5_5MBPS,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 110,
	  .hw_value = CONF_HW_BIT_RATE_11MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_11MBPS,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 60,
	  .hw_value = CONF_HW_BIT_RATE_6MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_6MBPS, },
	{ .bitrate = 90,
	  .hw_value = CONF_HW_BIT_RATE_9MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_9MBPS, },
	{ .bitrate = 120,
	  .hw_value = CONF_HW_BIT_RATE_12MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_12MBPS, },
	{ .bitrate = 180,
	  .hw_value = CONF_HW_BIT_RATE_18MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_18MBPS, },
	{ .bitrate = 240,
	  .hw_value = CONF_HW_BIT_RATE_24MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_24MBPS, },
	{ .bitrate = 360,
	 .hw_value = CONF_HW_BIT_RATE_36MBPS,
	 .hw_value_short = CONF_HW_BIT_RATE_36MBPS, },
	{ .bitrate = 480,
	  .hw_value = CONF_HW_BIT_RATE_48MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_48MBPS, },
	{ .bitrate = 540,
	  .hw_value = CONF_HW_BIT_RATE_54MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_54MBPS, },
};

/* can't be const, mac80211 writes to this */
static struct ieee80211_channel wifi_channels[] = {
	{ .hw_value = 1, .center_freq = 2412, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 2, .center_freq = 2417, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 3, .center_freq = 2422, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 4, .center_freq = 2427, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 5, .center_freq = 2432, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 6, .center_freq = 2437, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 7, .center_freq = 2442, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 8, .center_freq = 2447, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 9, .center_freq = 2452, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 10, .center_freq = 2457, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 11, .center_freq = 2462, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 12, .center_freq = 2467, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 13, .center_freq = 2472, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 14, .center_freq = 2484, .max_power = WLCORE_MAX_TXPWR },
};

/* can't be const, mac80211 writes to this */
static struct ieee80211_supported_band wifi_band_2ghz = {
	.channels = wifi_channels,
	.n_channels = ARRAY_SIZE(wifi_channels),
	.bitrates = wifi_rates,
	.n_bitrates = ARRAY_SIZE(wifi_rates),
};

/* 5 GHz data rates for WL1273 */
static struct ieee80211_rate wifi_rates_5ghz[] = {
	{ .bitrate = 60,
	  .hw_value = CONF_HW_BIT_RATE_6MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_6MBPS, },
	{ .bitrate = 90,
	  .hw_value = CONF_HW_BIT_RATE_9MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_9MBPS, },
	{ .bitrate = 120,
	  .hw_value = CONF_HW_BIT_RATE_12MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_12MBPS, },
	{ .bitrate = 180,
	  .hw_value = CONF_HW_BIT_RATE_18MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_18MBPS, },
	{ .bitrate = 240,
	  .hw_value = CONF_HW_BIT_RATE_24MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_24MBPS, },
	{ .bitrate = 360,
	 .hw_value = CONF_HW_BIT_RATE_36MBPS,
	 .hw_value_short = CONF_HW_BIT_RATE_36MBPS, },
	{ .bitrate = 480,
	  .hw_value = CONF_HW_BIT_RATE_48MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_48MBPS, },
	{ .bitrate = 540,
	  .hw_value = CONF_HW_BIT_RATE_54MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_54MBPS, },
};

/* 5 GHz band channels for WL1273 */
static struct ieee80211_channel wifi_channels_5ghz[] = {
	{ .hw_value = 8, .center_freq = 5040, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 12, .center_freq = 5060, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 16, .center_freq = 5080, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 34, .center_freq = 5170, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 36, .center_freq = 5180, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 38, .center_freq = 5190, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 40, .center_freq = 5200, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 42, .center_freq = 5210, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 44, .center_freq = 5220, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 46, .center_freq = 5230, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 48, .center_freq = 5240, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 52, .center_freq = 5260, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 56, .center_freq = 5280, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 60, .center_freq = 5300, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 64, .center_freq = 5320, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 100, .center_freq = 5500, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 104, .center_freq = 5520, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 108, .center_freq = 5540, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 112, .center_freq = 5560, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 116, .center_freq = 5580, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 120, .center_freq = 5600, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 124, .center_freq = 5620, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 128, .center_freq = 5640, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 132, .center_freq = 5660, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 136, .center_freq = 5680, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 140, .center_freq = 5700, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 149, .center_freq = 5745, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 153, .center_freq = 5765, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 157, .center_freq = 5785, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 161, .center_freq = 5805, .max_power = WLCORE_MAX_TXPWR },
	{ .hw_value = 165, .center_freq = 5825, .max_power = WLCORE_MAX_TXPWR },
};

static struct ieee80211_supported_band wifi_band_5ghz = {
	.channels = wifi_channels_5ghz,
	.n_channels = ARRAY_SIZE(wifi_channels_5ghz),
	.bitrates = wifi_rates_5ghz,
	.n_bitrates = ARRAY_SIZE(wifi_rates_5ghz),
};

/* === CUSTOM ====*/
static void wificore_op_unassign_vif_chanctx(struct ieee80211_hw *hw,
					   struct ieee80211_vif *vif,
					   struct ieee80211_chanctx_conf *ctx)
{
	return;
}


int wifi_wifi_op_change_interface(struct ieee80211_hw *hw,
				      struct ieee80211_vif *vif,
				      enum nl80211_iftype new_type, bool p2p)
{
	//printk("STUBBBBB - wifi_wifi_op_change_interface\n");
    return 0;
}

u64 wifi_wifi_op_prepare_multicast(struct ieee80211_hw *hw,
				       struct netdev_hw_addr_list *mc_list)
{
	//printk("STUBBBBB - wifi_wifi_op_prepare_multicast\n");
	return 0;
}

void wifi_wifi_op_cancel_hw_scan(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
	//printk("STUBBBBB - wifi_wifi_op_cancel_hw_scan\n");
	return;
}

int wifi_wifi_op_sched_scan_start(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct cfg80211_sched_scan_request *req, struct ieee80211_scan_ies *ies)
{
	//printk("STUBBBBB - wifi_wifi_op_sched_scan_start\n");
	return 0;
}

int wifi_wifi_op_sched_scan_stop(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
	//printk("STUBBBBB - wifi_wifi_op_sched_scan_stop\n");
	return 0;
}

/* Stub for .set_frag_threshold */
static int wifi_set_frag_threshold(struct ieee80211_hw *hw, u32 value)
{
	//printk("STUBBBBB - wifi_set_frag_threshold\n");
    return 0;
}

/* Stub for .set_rts_threshold */
static int wifi_set_rts_threshold(struct ieee80211_hw *hw, u32 value)
{
	//printk("STUBBBBB - wifi_set_rts_threshold\n");
    return 0;
}

/* Stub for .conf_tx */
static int wifi_conf_tx(struct ieee80211_hw *hw,
                      struct ieee80211_vif *vif,
                      u16 queue,
                      const struct ieee80211_tx_queue_params *params)
{
	//printk("STUBBBBB - wifi_conf_tx\n");
    return 0;
}

/* Stub for .get_tsf */
static u64 wifi_get_tsf(struct ieee80211_hw *hw,
                      struct ieee80211_vif *vif)
{
	//printk("STUBBBBB - wifi_get_tsf\n");
    return 0;           // or ULLONG_MAX if you prefer
}

/* Stub for .get_survey */
static int wifi_get_survey(struct ieee80211_hw *hw, int idx,
                         struct survey_info *survey)
{
	//printk("STUBBBBB - wifi_get_survey\n");
    return -ENOENT;     // or 0
}

/* Stub for .ampdu_action */
static int wifi_ampdu_action(struct ieee80211_hw *hw,
                           struct ieee80211_vif *vif,
                           struct ieee80211_ampdu_params *params)
{
	//printk("STUBBBBB - wifi_ampdu_action\n");
    return 0;
}

/* Stub for .tx_frames_pending */
static bool wifi_tx_frames_pending(struct ieee80211_hw *hw)
{
	//printk("STUBBBBB - wifi_tx_frames_pending\n");
    return false;
}

/* Stub for .set_bitrate_mask */
static int wifi_set_bitrate_mask(struct ieee80211_hw *hw,
                               struct ieee80211_vif *vif,
                               const struct cfg80211_bitrate_mask *mask)
{
	//printk("STUBBBBB - wifi_set_bitrate_mask\n");
    return 0;
}

/* Stub for .set_default_unicast_key */
static void wifi_set_default_unicast_key(struct ieee80211_hw *hw,
                                      struct ieee80211_vif *vif,
                                      int idx)
{
	//printk("STUBBBBB - wifi_set_default_unicast_key\n");
    //return 0;
}

/* Stub for .channel_switch */
static void wifi_channel_switch(struct ieee80211_hw *hw,
                              struct ieee80211_vif *vif,
                              struct ieee80211_channel_switch *ch_switch)
{
	//printk("STUBBBBB - wifi_channel_switch\n");
    /* No-op for stub */
}

/* Stub for .channel_switch_beacon */
static void wifi_channel_switch_beacon(struct ieee80211_hw *hw,
                                     struct ieee80211_vif *vif,
                                     struct cfg80211_chan_def *chandef)
{
	//printk("STUBBBBB - wifi_channel_switch_beacon\n");
    /* No-op for stub */
}

/* Stub for .flush */
static void wifi_flush(struct ieee80211_hw *hw,
                     struct ieee80211_vif *vif,
                     u32 queues,
                     bool drop)
{
	//printk("STUBBBBB - wifi_flush\n");
    /* No-op for stub */
}

/* Stub for .remain_on_channel */
static int wifi_remain_on_channel(struct ieee80211_hw *hw,
                                struct ieee80211_vif *vif,
                                struct ieee80211_channel *chan,
                                int duration,
                                enum ieee80211_roc_type type)
{
	//printk("STUBBBBB - wifi_remain_on_channel\n");
    return 0;
}

/* Stub for .cancel_remain_on_channel */
static int wifi_cancel_remain_on_channel(struct ieee80211_hw *hw,
                                       struct ieee80211_vif *vif)
{
	//printk("STUBBBBB - wifi_cancel_remain_on_channel\n");
    return 0;
}

/* Stub for .add_chanctx */
static int wifi_add_chanctx(struct ieee80211_hw *hw,
                          struct ieee80211_chanctx_conf *ctx)
{
	//printk("STUBBBBB - wifi_add_chanctx\n");
    return 0;
}

/* Stub for .remove_chanctx */
static void wifi_remove_chanctx(struct ieee80211_hw *hw,
                              struct ieee80211_chanctx_conf *ctx)
{
	//printk("STUBBBBB - wifi_remove_chanctx\n");
    /* No-op */
}

/* Stub for .change_chanctx */
static void wifi_change_chanctx(struct ieee80211_hw *hw,
                              struct ieee80211_chanctx_conf *ctx,
                              u32 changed)
{
	//printk("STUBBBBB - wifi_change_chanctx\n");
    /* No-op */
}

/* Stub for .switch_vif_chanctx */
static int wifi_switch_vif_chanctx(struct ieee80211_hw *hw,
                                 struct ieee80211_vif_chanctx_switch *vifs,
                                 int n_vifs,
                                 enum ieee80211_chanctx_switch_mode mode)
{
	//printk("STUBBBBB - wifi_switch_vif_chanctx\n");
    return 0;
}

/* Stub for .sta_rc_update */
static void wifi_sta_rc_update(struct ieee80211_hw *hw,
                             struct ieee80211_vif *vif,
                             struct ieee80211_sta *sta,
                             u32 changed)
{
	//printk("STUBBBBB - wifi_sta_rc_update\n");
    /* No-op */
}

/* Stub for .sta_statistics */
static void wifi_sta_statistics(struct ieee80211_hw *hw,
                              struct ieee80211_vif *vif,
                              struct ieee80211_sta *sta,
                              struct station_info *sinfo)
{
	//printk("STUBBBBB - wifi_sta_statistics\n");
    /* No-op */
}

/* Stub for .get_expected_throughput */
static u32 wifi_get_expected_throughput(struct ieee80211_hw *hw,
                                      struct ieee80211_sta *sta)
{
	//printk("STUBBBBB - wifi_get_expected_throughput\n");
    return 0;
}

int wifi_wifi_op_start(struct ieee80211_hw *hw)
{
	//printk("STUBBBBB - wifi_wifi_op_start\n");
	return 0;
}

int wifi_op_suspend(struct ieee80211_hw *hw,
					    struct cfg80211_wowlan *wow)
						{
							printk("STUBBBBB - wifi_op_suspend\n");
							return 0;
						}

int wifi_op_resume(struct ieee80211_hw *hw){
	//printk("STUBBBBB - wifi_op_resume\n");
	return 0;
}

int wifi_op_config(struct ieee80211_hw *hw, u32 changed){
	return 0;
}

static const struct ieee80211_ops wifi_ops = {
	.stop = wificore_op_stop, // wifi_

	.add_interface = wifi_op_add_interface, // ~wifi_
	.remove_interface = wifi_op_remove_interface, // ~wifi_

	.configure_filter = wifi_op_configure_filter, // config rx frame filtering
													// Ex: FIF_ALLMULTI -> accept all
	.tx = wifi_op_tx, // when want to transmit data
	.set_key = wificore_op_set_key, // set key for WPAx after association
								  // After assoc + authrized, data transfered is encrypted (WPA2 = AES_CCMP | WPA3 = AES_GCMP)
								  // struct ieee80211_key_conf *key_conf->keyidx | keylen | key
	.hw_scan = wifi_op_hw_scan, // if roc is 0 -> allow to scan, else -> no allow
								  // Get wifi_scan_channels *ch->passive | active | flags => build probe req if active > 0
								  // => CMD_SCAN

								  // if not connected, wifi_op_hw_scan is triggered periodically

								  // wificore_op_assign_vif_chanctx -> CHOOSE channel from freq

	.bss_info_changed = wifi_op_bss_info_changed, // collab with wifi_op_sta_state 
	.sta_state = wifi_op_sta_state, // Remain on chip -> focusing on association / deauthenticating
	.assign_vif_chanctx = wificore_op_assign_vif_chanctx, // assign rate_set = basic_rate
														// also get the same channel between AP and STA

// 	/* =====================*/ /* =====================*/ /* =====================*/

	// .config = wifi_op_config,
	// .prepare_multicast = wifi_op_prepare_multicast,
	// .cancel_hw_scan = wifi_op_cancel_hw_scan,
	// .conf_tx            = wifi_op_conf_tx,
	// .get_survey         = wifi_op_get_survey,
	// .ampdu_action              = wifi_op_ampdu_action,
	// .set_default_unicast_key   = wifi_op_set_default_key_idx,
    // .add_chanctx               = wificore_op_add_chanctx,
    // .remove_chanctx            = wificore_op_remove_chanctx,
    // .change_chanctx            = wificore_op_change_chanctx,
	// .sta_statistics = wificore_op_sta_statistics,
	// .get_expected_throughput = wificore_op_get_expected_throughput,
    // .flush                     = wificore_op_flush,
	// .switch_vif_chanctx = wificore_op_switch_vif_chanctx,
	// .unassign_vif_chanctx = wificore_op_unassign_vif_chanctx,

	.config = wifi_op_config,
	.prepare_multicast = wifi_wifi_op_prepare_multicast,
	.cancel_hw_scan = wifi_wifi_op_cancel_hw_scan,
	.conf_tx            = wifi_conf_tx,
	.get_survey         = wifi_get_survey,
	.ampdu_action              = wifi_ampdu_action,
	.set_default_unicast_key   = wifi_set_default_unicast_key,
    .add_chanctx               = wifi_add_chanctx,
    .remove_chanctx            = wifi_remove_chanctx,
    .change_chanctx            = wifi_change_chanctx,
	.sta_statistics = wifi_sta_statistics,
	.get_expected_throughput = wifi_get_expected_throughput,
    .flush                     = wifi_flush,
	.switch_vif_chanctx = wifi_switch_vif_chanctx,
	.unassign_vif_chanctx = wificore_op_unassign_vif_chanctx,
	/* =====================*/ /* =====================*/ /* =====================*/
#ifdef CONFIG_PM
	.suspend = wifi_op_suspend,
	.resume = wifi_op_resume,
#endif
	.start = wifi_wifi_op_start,
	.change_interface = wifi_wifi_op_change_interface,
	.sched_scan_start = wifi_wifi_op_sched_scan_start,
	.sched_scan_stop = wifi_wifi_op_sched_scan_stop,

	.set_frag_threshold = wifi_set_frag_threshold,
    .set_rts_threshold  = wifi_set_rts_threshold,
    .get_tsf            = wifi_get_tsf,

    .tx_frames_pending         = wifi_tx_frames_pending,
    .set_bitrate_mask          = wifi_set_bitrate_mask,
    .channel_switch            = wifi_channel_switch,
    .channel_switch_beacon     = wifi_channel_switch_beacon,
    .remain_on_channel         = wifi_remain_on_channel,
    .cancel_remain_on_channel  = wifi_cancel_remain_on_channel,
    .switch_vif_chanctx        = wifi_switch_vif_chanctx,
    .sta_rc_update             = wifi_sta_rc_update,
};


u8 wificore_rate_to_idx(u8 rate, enum nl80211_band band)
{
	u8 idx;

	BUG_ON(band >= 2);

	if (unlikely(rate >= 29)) { // WL18XX_CONF_HW_RXTX_RATE_MAX
		wifi_error("Illegal RX rate from HW: %d", rate);
		return 0;
	}

	idx = wifi_data->band_rate_to_idx[band][rate];
	if (unlikely(idx == CONF_HW_RXTX_RATE_UNSUPPORTED)) {
		wifi_error("Unsupported RX rate from HW: %d", rate);
		return 0;
	}

	return idx;
}

static void wifi_derive_mac_addresses(u32 oui, u32 nic)
{
	int i;

	wifi_debug(DEBUG_PROBE, "base address: oui %06x nic %06x",
		     oui, nic);

	if (nic + WLCORE_NUM_MAC_ADDRESSES - wifi_data->num_mac_addr > 0xffffff)
		wifi_warning("NIC part of the MAC address wraps around!");

	for (i = 0; i < wifi_data->num_mac_addr; i++) {
		wifi_data->addresses[i].addr[0] = (u8)(oui >> 16);
		wifi_data->addresses[i].addr[1] = (u8)(oui >> 8);
		wifi_data->addresses[i].addr[2] = (u8) oui;
		wifi_data->addresses[i].addr[3] = (u8)(nic >> 16);
		wifi_data->addresses[i].addr[4] = (u8)(nic >> 8);
		wifi_data->addresses[i].addr[5] = (u8) nic;
		nic++;
	}

	/* we may be one address short at the most */
	WARN_ON(wifi_data->num_mac_addr + 1 < WLCORE_NUM_MAC_ADDRESSES);

	/*
	 * turn on the LAA bit in the first address and use it as
	 * the last address.
	 */
	if (wifi_data->num_mac_addr < WLCORE_NUM_MAC_ADDRESSES) {
		int idx = WLCORE_NUM_MAC_ADDRESSES - 1;
		memcpy(&wifi_data->addresses[idx], &wifi_data->addresses[0],
		       sizeof(wifi_data->addresses[0]));
		/* LAA bit */
		wifi_data->addresses[idx].addr[0] |= BIT(1);
	}

	wifi_data->hw->wiphy->n_addresses = WLCORE_NUM_MAC_ADDRESSES;
	wifi_data->hw->wiphy->addresses = wifi_data->addresses;
}

static int wifi_get_hw_info(void)
{
	int ret;

	ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_CHIP_ID_B]), &wifi_chip->id, 4, false);
	if (ret < 0)
		goto out;

	wifi_data->fuse_oui_addr = 0;
	wifi_data->fuse_nic_addr = 0;

	ret = wifi_get_mac();

out:
	return ret;
}

static int wifi_register_hw(void)
{
	int ret;
	u32 oui_addr = 0, nic_addr = 0;
	struct platform_device *pdev = wifi_data->pdev;
	struct wificore_platdev_data *pdev_data = dev_get_platdata(&pdev->dev);

	if (wifi_data->mac80211_registered)
		return 0;

	if (wifi_data->nvs_len >= 12) {
		/* NOTE: The wifi_data->nvs->nvs element must be first, in
		 * order to simplify the casting, we assume it is at
		 * the beginning of the wifi_data->nvs structure.
		 */
		u8 *nvs_ptr = (u8 *)wifi_data->nvs;

		oui_addr =
			(nvs_ptr[11] << 16) + (nvs_ptr[10] << 8) + nvs_ptr[6];
		nic_addr =
			(nvs_ptr[5] << 16) + (nvs_ptr[4] << 8) + nvs_ptr[3];
	}

	/* if the MAC address is zeroed in the NVS derive from fuse */
	if (oui_addr == 0 && nic_addr == 0) {
		oui_addr = wifi_data->fuse_oui_addr;
		/* fuse has the BD_ADDR, the WLAN addresses are the next two */
		nic_addr = wifi_data->fuse_nic_addr + 1;
	}

	if (oui_addr == 0xdeadbe && nic_addr == 0xef0000) {
		wifi_warning("Detected unconfigured mac address in nvs, derive from fuse instead.");
		if (!strcmp(pdev_data->family->name, "wl18xx")) {
			wifi_warning("This default nvs file can be removed from the file system");
		} else {
			wifi_warning("Your device performance is not optimized.");
			wifi_warning("Please use the calibrator tool to configure your device.");
		}

		if (wifi_data->fuse_oui_addr == 0 && wifi_data->fuse_nic_addr == 0) {
			wifi_warning("Fuse mac address is zero. using random mac");
			/* Use TI oui and a random nic */
			oui_addr = WLCORE_TI_OUI_ADDRESS;
			nic_addr = get_random_int();
		} else {
			oui_addr = wifi_data->fuse_oui_addr;
			/* fuse has the BD_ADDR, the WLAN addresses are the next two */
			nic_addr = wifi_data->fuse_nic_addr + 1;
		}
	}

	wifi_derive_mac_addresses(oui_addr, nic_addr);

	ret = ieee80211_register_hw(wifi_data->hw);
	if (ret < 0) {
		wifi_error("unable to register mac80211 hw: %d", ret);
		goto out;
	}

	wifi_data->mac80211_registered = true;

	printk("loaded");

out:
	return ret;
}

static void wifi_unregister_hw(void)
{
	ieee80211_unregister_hw(wifi_data->hw);
	wifi_data->mac80211_registered = false;

}

static int wifi_init_ieee80211(void)
{
	int i;
	static const u32 cipher_suites[] = {
		WLAN_CIPHER_SUITE_WEP40,
		WLAN_CIPHER_SUITE_WEP104,
		WLAN_CIPHER_SUITE_TKIP,
		WLAN_CIPHER_SUITE_CCMP,
		WL1271_CIPHER_SUITE_GEM,
	};

	/* The tx descriptor buffer */
	wifi_data->hw->extra_tx_headroom = sizeof(struct wifi_tx_hw_descr);

	if (wifi_data->quirks & WLCORE_QUIRK_TKIP_HEADER_SPACE)
		wifi_data->hw->extra_tx_headroom += WL1271_EXTRA_SPACE_TKIP;

	/* unit us */
	/* FIXME: find a proper value */
	wifi_data->hw->max_listen_interval = wifi_data->conf.conn.max_listen_interval;

	ieee80211_hw_set(wifi_data->hw, SUPPORT_FAST_XMIT);
	ieee80211_hw_set(wifi_data->hw, CHANCTX_STA_CSA);
	ieee80211_hw_set(wifi_data->hw, SUPPORTS_PER_STA_GTK);
	ieee80211_hw_set(wifi_data->hw, QUEUE_CONTROL);
	ieee80211_hw_set(wifi_data->hw, TX_AMPDU_SETUP_IN_HW);
	ieee80211_hw_set(wifi_data->hw, AMPDU_AGGREGATION);
	ieee80211_hw_set(wifi_data->hw, AP_LINK_PS);
	ieee80211_hw_set(wifi_data->hw, SPECTRUM_MGMT);
	ieee80211_hw_set(wifi_data->hw, REPORTS_TX_ACK_STATUS);
	ieee80211_hw_set(wifi_data->hw, CONNECTION_MONITOR);
	ieee80211_hw_set(wifi_data->hw, HAS_RATE_CONTROL);
	ieee80211_hw_set(wifi_data->hw, SUPPORTS_DYNAMIC_PS);
	ieee80211_hw_set(wifi_data->hw, SIGNAL_DBM);
	ieee80211_hw_set(wifi_data->hw, SUPPORTS_PS);
	ieee80211_hw_set(wifi_data->hw, SUPPORTS_TX_FRAG);

	wifi_data->hw->wiphy->cipher_suites = cipher_suites;
	wifi_data->hw->wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites);

	wifi_data->hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
					 BIT(NL80211_IFTYPE_AP) |
					 BIT(NL80211_IFTYPE_P2P_DEVICE) |
					 BIT(NL80211_IFTYPE_P2P_CLIENT) |
#ifdef CONFIG_MAC80211_MESH
					 BIT(NL80211_IFTYPE_MESH_POINT) |
#endif
					 BIT(NL80211_IFTYPE_P2P_GO);

	wifi_data->hw->wiphy->max_scan_ssids = 1;
	wifi_data->hw->wiphy->max_sched_scan_ssids = 16;
	wifi_data->hw->wiphy->max_match_sets = 16;
	/*
	 * Maximum length of elements in scanning probe request templates
	 * should be the maximum length possible for a template, without
	 * the IEEE80211 header of the template
	 */
	wifi_data->hw->wiphy->max_scan_ie_len = WL1271_CMD_TEMPL_MAX_SIZE -
			sizeof(struct ieee80211_header);

	wifi_data->hw->wiphy->max_sched_scan_reqs = 1;
	wifi_data->hw->wiphy->max_sched_scan_ie_len = WL1271_CMD_TEMPL_MAX_SIZE -
		sizeof(struct ieee80211_header);

	wifi_data->hw->wiphy->max_remain_on_channel_duration = 30000;

	wifi_data->hw->wiphy->flags |= WIPHY_FLAG_AP_UAPSD |
				WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL |
				WIPHY_FLAG_HAS_CHANNEL_SWITCH |
				WIPHY_FLAG_IBSS_RSN;

	wifi_data->hw->wiphy->features |= NL80211_FEATURE_AP_SCAN;

	/* make sure all our channels fit in the scanned_ch bitmask */
	BUILD_BUG_ON(ARRAY_SIZE(wifi_channels) +
		     ARRAY_SIZE(wifi_channels_5ghz) >
		     WL1271_MAX_CHANNELS);
	/*
	* clear channel flags from the previous usage
	* and restore max_power & max_antenna_gain values.
	*/
	for (i = 0; i < ARRAY_SIZE(wifi_channels); i++) {
		wifi_band_2ghz.channels[i].flags = 0;
		wifi_band_2ghz.channels[i].max_power = WLCORE_MAX_TXPWR;
		wifi_band_2ghz.channels[i].max_antenna_gain = 0;
	}

	for (i = 0; i < ARRAY_SIZE(wifi_channels_5ghz); i++) {
		wifi_band_5ghz.channels[i].flags = 0;
		wifi_band_5ghz.channels[i].max_power = WLCORE_MAX_TXPWR;
		wifi_band_5ghz.channels[i].max_antenna_gain = 0;
	}

	/*
	 * We keep local copies of the band structs because we need to
	 * modify them on a per-device basis.
	 */
	memcpy(&wifi_data->bands[NL80211_BAND_2GHZ], &wifi_band_2ghz,
	       sizeof(wifi_band_2ghz));
	memcpy(&wifi_data->bands[NL80211_BAND_2GHZ].ht_cap,
	       &wifi_data->ht_cap[NL80211_BAND_2GHZ],
	       sizeof(*wifi_data->ht_cap));
	memcpy(&wifi_data->bands[NL80211_BAND_5GHZ], &wifi_band_5ghz,
	       sizeof(wifi_band_5ghz));
	memcpy(&wifi_data->bands[NL80211_BAND_5GHZ].ht_cap,
	       &wifi_data->ht_cap[NL80211_BAND_5GHZ],
	       sizeof(*wifi_data->ht_cap));

	wifi_data->hw->wiphy->bands[NL80211_BAND_2GHZ] =
		&wifi_data->bands[NL80211_BAND_2GHZ];
	wifi_data->hw->wiphy->bands[NL80211_BAND_5GHZ] =
		&wifi_data->bands[NL80211_BAND_5GHZ];

	/*
	 * allow 4 queues per mac address we support +
	 * 1 cab queue per mac + one global offchannel Tx queue
	 */
	wifi_data->hw->queues = (NUM_TX_QUEUES + 1) * WLCORE_NUM_MAC_ADDRESSES + 1;

	/* the last queue is the offchannel queue */
	wifi_data->hw->offchannel_tx_hw_queue = wifi_data->hw->queues - 1;
	wifi_data->hw->max_rates = 1;

	wifi_data->hw->wiphy->reg_notifier = wifi_reg_notify;

	/* the FW answers probe-requests in AP-mode */
	wifi_data->hw->wiphy->flags |= WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD;
	wifi_data->hw->wiphy->probe_resp_offload =
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS |
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS2 |
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_P2P;

	/* allowed interface combinations */
	wifi_data->hw->wiphy->iface_combinations = wifi_data->iface_combinations;
	wifi_data->hw->wiphy->n_iface_combinations = wifi_data->n_iface_combinations;

	SET_IEEE80211_DEV(wifi_data->hw, wifi_data->dev);

	wifi_data->hw->sta_data_size = sizeof(struct wifi_station);
	wifi_data->hw->vif_data_size = sizeof(struct wifi_vif);

	wifi_data->hw->max_rx_aggregation_subframes = wifi_data->conf.ht.rx_ba_win_size;

	return 0;
}

struct ieee80211_hw *wificore_alloc_hw(size_t priv_size, u32 aggr_buf_size,
				     u32 mbox_size)
{
	struct ieee80211_hw *hw;
	int i, j, ret;
	unsigned int order;

	hw = ieee80211_alloc_hw(sizeof(*wifi_data), &wifi_ops);
	if (!hw) {
		wifi_error("could not alloc ieee80211_hw");
		ret = -ENOMEM;
		goto err_hw_alloc;
	}

	wifi_data = hw->priv;
	memset(wifi_data, 0, sizeof(*wifi_data));

	// point to conf
	wifi_data->priv = kzalloc(priv_size, GFP_KERNEL);
	if (!wifi_data->priv) {
		wifi_error("could not alloc wl priv");
		ret = -ENOMEM;
		goto err_priv_alloc;
	}

	INIT_LIST_HEAD(&wifi_data->wifi_vif_list);
	INIT_LIST_HEAD(&wifi_data->wifi_vif_list);

	wifi_data->hw = hw;

	/*
	 * wifi_data->num_links is not configured yet, so just use WLCORE_MAX_LINKS.
	 * we don't allocate any additional resource here, so that's fine.
	 */
	for (i = 0; i < NUM_TX_QUEUES; i++){
		for (j = 0; j < WLCORE_MAX_LINKS; j++){
			skb_queue_head_init(&wifi_tx_queue[j][i]);
		}
	}

	skb_queue_head_init(&wifi_deferred_rx_queue);
	skb_queue_head_init(&wifi_deferred_tx_queue);

	INIT_WORK(&wifi_work.netstack_work, wifi_netstack_work);

	INIT_WORK(&wifi_work.tx_work, wifi_tx_work);
	INIT_DELAYED_WORK(&wifi_work.scan_complete_work, wifi_scan_complete_work);
	INIT_DELAYED_WORK(&wifi_work.tx_watchdog_work, wifi_tx_watchdog_work);

	wifi_work.freezable_wq = create_freezable_workqueue("wifi_wq");
	if (!wifi_work.freezable_wq) {
		ret = -ENOMEM;
		goto err_hw;
	}

	wifi_data->channel = 0;
	wifi_rx_counter = 0;
	wifi_data->flags = 0;
	wifi_data->sleep_auth = WL1271_PSM_ILLEGAL;
	wifi_data->quirks = 0;

	/* The system link is always allocated */
	__set_bit(WL12XX_SYSTEM_HLID, wifi_map.links_map);

	for (i = 0; i < WL18XX_NUM_TX_DESCRIPTORS; i++)
		wifi_skb_tx_frames[i] = NULL;

	spin_lock_init(&wifi_data->lock);

	wifi_data->state = WLCORE_STATE_OFF;
	wifi_data->fw_type = WL12XX_FW_TYPE_NONE;
	mutex_init(&wifi_data->mutex);
	mutex_init(&wifi_data->flush_mutex);
	init_completion(&wifi_data->nvs_loading_complete);

	order = get_order(aggr_buf_size);
	wifi_aggr_buf = (u8 *)__get_free_pages(GFP_KERNEL, order);
	if (!wifi_aggr_buf) {
		ret = -ENOMEM;
		goto err_wq;
	}
	//WL18XX_AGGR_BUFFER_SIZE = aggr_buf_size;
	// 53248 ÷ 4096 = 13.0 -> aggr_buf_size needs 13 pages
	// get_order returns the number has the power of 2
	// -> order = 4 -> 2^4 = 16 > 13

	wifi_dummy_packet = wifi_alloc_dummy_packet();
	if (!wifi_dummy_packet) {
		ret = -ENOMEM;
		goto err_aggr;
	}

	return hw;

err_aggr:
	free_pages((unsigned long)wifi_aggr_buf, order);

err_wq:
	destroy_workqueue(wifi_work.freezable_wq);

err_hw:
	kfree(wifi_data->priv);

err_priv_alloc:
	ieee80211_free_hw(hw);

err_hw_alloc:

	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(wificore_alloc_hw);

int wificore_free_hw(void)
{
	/* Unblock any fwlog readers */
	mutex_lock(&wifi_data->mutex);
	mutex_unlock(&wifi_data->mutex);

	dev_kfree_skb(wifi_dummy_packet);
	free_pages((unsigned long)wifi_aggr_buf, get_order(WL18XX_AGGR_BUFFER_SIZE));

	vfree(wifi_data->fw);
	wifi_data->fw = NULL;
	wifi_data->fw_type = WL12XX_FW_TYPE_NONE;
	kfree(wifi_data->nvs);
	wifi_data->nvs = NULL;

	kfree(wifi_status_reg);
	destroy_workqueue(wifi_work.freezable_wq);

	kfree(wifi_data->priv);
	ieee80211_free_hw(wifi_data->hw);

	return 0;
}
EXPORT_SYMBOL_GPL(wificore_free_hw);

#ifdef CONFIG_PM
static const struct wiphy_wowlan_support wificore_wowlan_support = {
	.flags = WIPHY_WOWLAN_ANY,
	.n_patterns = WL1271_MAX_RX_FILTERS,
	.pattern_min_len = 1,
	.pattern_max_len = WL1271_RX_FILTER_MAX_PATTERN_SIZE,
};
#endif

static irqreturn_t wificore_hardirq(int irq, void *cookie)
{
	return IRQ_WAKE_THREAD;
}
#include "tables.h"
static void wificore_nvs_cb(const struct firmware *fw)
{
	struct platform_device *pdev = wifi_data->pdev;
	struct wificore_platdev_data *pdev_data = dev_get_platdata(&pdev->dev);
	struct resource *res;

	int ret;
	irq_handler_t hardirq_fn = NULL;

	if (fw) {
		wifi_data->nvs = kmemdup(fw->data, fw->size, GFP_KERNEL);
		if (!wifi_data->nvs) {
			wifi_error("Could not allocate nvs data");
			goto out;
		}
		wifi_data->nvs_len = fw->size;
	} else if (pdev_data->family->nvs_name) {
		wifi_debug(DEBUG_BOOT, "Could not get nvs file %s",
			     pdev_data->family->nvs_name);
		wifi_data->nvs = NULL;
		wifi_data->nvs_len = 0;
	} else {
		wifi_data->nvs = NULL;
		wifi_data->nvs_len = 0;
	}

	ret = wifi_data->ops->setup();
	if (ret < 0)
		goto out_free_nvs;

	/* VInh custom */
	wifi_data->rtable = wifi_rtable;


	BUG_ON(WL18XX_NUM_TX_DESCRIPTORS > WLCORE_MAX_TX_DESCRIPTORS);

	/* adjust some runtime configuration parameters */
	wificore_adjust_conf();

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		wifi_error("Could not get IRQ resource");
		goto out_free_nvs;
	}

	wifi_data->irq = res->start;
	wifi_data->irq_flags = res->flags & IRQF_TRIGGER_MASK;
	wifi_data->if_ops = pdev_data->if_ops;

	if (wifi_data->irq_flags & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING))
		hardirq_fn = wificore_hardirq;
	else
		wifi_data->irq_flags |= IRQF_ONESHOT;

	// V_
	ret = wifi_set_power_on();
	if (ret < 0)
		goto out_free_nvs;

	ret = wifi_get_hw_info();
	if (ret < 0) {
		wifi_error("couldn't get hw info");
		wifi_power_off();
		goto out_free_nvs;
	}

	ret = request_threaded_irq(wifi_data->irq, hardirq_fn, wificore_irq,
				   wifi_data->irq_flags, pdev->name, NULL);
	if (ret < 0) {
		wifi_error("interrupt configuration failed");
		wifi_power_off();
		goto out_free_nvs;
	}

#ifdef CONFIG_PM
	device_init_wakeup(wifi_data->dev, true);

	ret = enable_irq_wake(wifi_data->irq);
	if (!ret) {
		wifi_data->irq_wake_enabled = true;
		if (pdev_data->pwr_in_suspend)
			wifi_data->hw->wiphy->wowlan = &wificore_wowlan_support;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (res) {
		wifi_data->wakeirq = res->start;
		//wifi_data->wakeirq_flags = res->flags & IRQF_TRIGGER_MASK;
		ret = dev_pm_set_dedicated_wake_irq(wifi_data->dev, wifi_data->wakeirq);
		if (ret)
			wifi_data->wakeirq = -ENODEV;
	} else {
		wifi_data->wakeirq = -ENODEV;
	}
#endif
	disable_irq(wifi_data->irq);
	wifi_power_off();

	ret = wifi_identify_chip();
	if (ret < 0)
		goto out_irq;

	ret = wifi_init_ieee80211();
	if (ret)
		goto out_irq;

	ret = wifi_register_hw();
	if (ret)
		goto out_irq;


	wifi_data->initialized = true;
	goto out;

out_irq:
	if (wifi_data->wakeirq >= 0)
		dev_pm_clear_wake_irq(wifi_data->dev);
	device_init_wakeup(wifi_data->dev, false);
	free_irq(wifi_data->irq, NULL);

out_free_nvs:
	kfree(wifi_data->nvs);

out:
	release_firmware(fw);
	complete_all(&wifi_data->nvs_loading_complete);
}

int wificore_probe(struct platform_device *pdev)
{
	struct wificore_platdev_data *pdev_data = dev_get_platdata(&pdev->dev);
	//const char *nvs_name;
	int ret = 0;

	if (!wifi_data->ops || !wifi_data->ptable || !pdev_data)
		return -EINVAL;

	wifi_data->dev = &pdev->dev;
	wifi_data->pdev = pdev;

	wifi_chip = (struct wifi_chip*)devm_kzalloc(wifi_data->dev, sizeof(struct wifi_chip), GFP_KERNEL);

	wificore_nvs_cb(NULL);

	//wifi_data->dev->driver->pm = &wificore_pm_ops;
	pm_runtime_set_autosuspend_delay(wifi_data->dev, 50);
	pm_runtime_use_autosuspend(wifi_data->dev);
	pm_runtime_enable(wifi_data->dev);

	return ret;
}
EXPORT_SYMBOL_GPL(wificore_probe);

int wificore_remove(struct platform_device *pdev)
{
	struct wificore_platdev_data *pdev_data = dev_get_platdata(&pdev->dev);
	int error;

	error = pm_runtime_get_sync(wifi_data->dev);
	if (error < 0)
		dev_warn(wifi_data->dev, "PM runtime failed: %i\n", error);

	wifi_data->dev->driver->pm = NULL;

	if (pdev_data->family && pdev_data->family->nvs_name)
		wait_for_completion(&wifi_data->nvs_loading_complete);
	if (!wifi_data->initialized)
		return 0;

	if (wifi_data->wakeirq >= 0) {
		dev_pm_clear_wake_irq(wifi_data->dev);
		wifi_data->wakeirq = -ENODEV;
	}

	device_init_wakeup(wifi_data->dev, false);

	if (wifi_data->irq_wake_enabled)
		disable_irq_wake(wifi_data->irq);

	wifi_unregister_hw();

	pm_runtime_put_sync(wifi_data->dev);
	pm_runtime_dont_use_autosuspend(wifi_data->dev);
	pm_runtime_disable(wifi_data->dev);

	free_irq(wifi_data->irq, NULL);
	wificore_free_hw();

	return 0;
}
EXPORT_SYMBOL_GPL(wificore_remove);

u32 wifi_debug_level = DEBUG_NONE;
EXPORT_SYMBOL_GPL(wifi_debug_level);
module_param_named(debug_level, wifi_debug_level, uint, 0600);
MODULE_PARM_DESC(debug_level, "wl12xx debugging level");

module_param_named(fwlog, fwlog_param, charp, 0);
MODULE_PARM_DESC(fwlog,
		 "FW logger options: continuous, dbgpins or disable");

module_param(fwlog_mem_blocks, int, 0600);
MODULE_PARM_DESC(fwlog_mem_blocks, "fwlog mem_blocks");

module_param(bug_on_recovery, int, 0600);
MODULE_PARM_DESC(bug_on_recovery, "BUG() on fw recovery");

module_param(no_recovery, int, 0600);
MODULE_PARM_DESC(no_recovery, "Prevent HW recovery. FW will remain stuck.");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
