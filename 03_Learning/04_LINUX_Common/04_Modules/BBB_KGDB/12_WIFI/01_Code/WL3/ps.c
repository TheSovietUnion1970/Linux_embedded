// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include "ps.h"
#include "io.h"
#include "tx.h"
#include "debug.h"

int wl1271_ps_set_mode(struct wl1271 *wl, struct wl12xx_vif *wlvif,
		       enum wl1271_cmd_ps_mode mode)
{
	int ret;
	u16 timeout = wl->conf.conn.dynamic_ps_timeout;

	switch (mode) {
	case STATION_AUTO_PS_MODE:
	case STATION_POWER_SAVE_MODE:
		wl1271_debug(DEBUG_PSM, "entering psm (mode=%d,timeout=%u)",
			     mode, timeout);

		ret = wl1271_acx_wake_up_conditions(wl, wlvif,
					    wl->conf.conn.wake_up_event,
					    wl->conf.conn.listen_interval);
		if (ret < 0) {
			wl1271_error("couldn't set wake up conditions");
			return ret;
		}

		ret = wl1271_cmd_ps_mode(wl, wlvif, mode, timeout);
		if (ret < 0)
			return ret;

		set_bit(WLVIF_FLAG_IN_PS, &wlvif->flags);

		/*
		 * enable beacon early termination.
		 * Not relevant for 5GHz and for high rates.
		 */
		if ((wlvif->band == NL80211_BAND_2GHZ) &&
		    (wlvif->basic_rate < CONF_HW_BIT_RATE_9MBPS)) {
			ret = wl1271_acx_bet_enable(wl, wlvif, true);
			if (ret < 0)
				return ret;
		}
		break;
	case STATION_ACTIVE_MODE:
		wl1271_debug(DEBUG_PSM, "leaving psm");

		/* disable beacon early termination */
		if ((wlvif->band == NL80211_BAND_2GHZ) &&
		    (wlvif->basic_rate < CONF_HW_BIT_RATE_9MBPS)) {
			ret = wl1271_acx_bet_enable(wl, wlvif, false);
			if (ret < 0)
				return ret;
		}

		ret = wl1271_cmd_ps_mode(wl, wlvif, mode, 0);
		if (ret < 0)
			return ret;

		clear_bit(WLVIF_FLAG_IN_PS, &wlvif->flags);
		break;
	default:
		wl1271_warning("trying to set ps to unsupported mode %d", mode);
		ret = -EINVAL;
	}

	return ret;
}

static void wl1271_ps_filter_frames(struct wl1271 *wl, u8 hlid)
{
	int i;
	struct sk_buff *skb;
	struct ieee80211_tx_info *info;
	unsigned long flags;
	int filtered[NUM_TX_QUEUES];

	/* filter all frames currently in the low level queues for this hlid */
	for (i = 0; i < NUM_TX_QUEUES; i++) {
		filtered[i] = 0;
		while ((skb = skb_dequeue(&VV_tx_queue[hlid][i]))) {
			printk("TX_QUEUE - wl1271_ps_filter_frames\n");
			filtered[i]++;

			if (WARN_ON(wl12xx_is_dummy_packet(wl, skb)))
				continue;

			info = IEEE80211_SKB_CB(skb);
			info->flags |= IEEE80211_TX_STAT_TX_FILTERED;
			info->status.rates[0].idx = -1;
			ieee80211_tx_status_ni(wl->hw, skb);
		}
	}

	spin_lock_irqsave(&wl->wl_lock, flags);
	for (i = 0; i < NUM_TX_QUEUES; i++) {
		//wl->tx_queue_count[i] -= filtered[i];
		VV_tx_queue_count[i] -= filtered[i];
	}
	spin_unlock_irqrestore(&wl->wl_lock, flags);

	//wl1271_handle_tx_low_watermark(wl);
}
