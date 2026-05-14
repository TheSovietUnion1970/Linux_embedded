// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include <linux/gfp.h>
#include <linux/sched.h>

#include "wlcore.h"
#include "debug.h"
#include "acx.h"
#include "rx.h"
#include "tx.h"
#include "io.h"
#include "wl18xx.h"
#include "ops.h"


#include "common.h"
#include "main.h"

/*
 * TODO: this is here just for now, it must be removed when the data
 * operations are in place.
 */
// #include "../wl12xx/reg.h"
#include "reg.h"

static u32 wificore_rx_get_buf_size(u32 rx_pkt_desc)
{
	// if (wifi_data->quirks & WLCORE_QUIRK_RX_BLOCKSIZE_ALIGN)
	// 	return (rx_pkt_desc & ALIGNED_RX_BUF_SIZE_MASK) >>
	// 	       ALIGNED_RX_BUF_SIZE_SHIFT;

	// return (rx_pkt_desc & RX_BUF_SIZE_MASK) >> RX_BUF_SIZE_SHIFT_DIV;

	return (rx_pkt_desc & ALIGNED_RX_BUF_SIZE_MASK) >>
			ALIGNED_RX_BUF_SIZE_SHIFT;
}

static u32 wificore_rx_get_align_buf_size(u32 pkt_len)
{
	// if (wifi_data->quirks & WLCORE_QUIRK_RX_BLOCKSIZE_ALIGN)
	// 	return ALIGN(pkt_len, WL12XX_BUS_BLOCK_SIZE);

	// return pkt_len;

	return ALIGN(pkt_len, WL12XX_BUS_BLOCK_SIZE);
}

static void wifi_rx_status(
			     struct wifi_rx_descriptor *desc,
			     struct ieee80211_rx_status *status,
			     u8 beacon, u8 probe_rsp)
{
	memset(status, 0, sizeof(struct ieee80211_rx_status));

	if ((desc->flags & WL1271_RX_DESC_BAND_MASK) == WL1271_RX_DESC_BAND_BG) // 2.4 GHz
		status->band = NL80211_BAND_2GHZ;
	else
		status->band = NL80211_BAND_5GHZ; // WL1271_RX_DESC_BAND_A -> 5 GHz Band (A = 802.11a)

	status->rate_idx = wificore_rate_to_idx(desc->rate, status->band);
	// -> rate_idx is used for interal idx of mac80211

	/* 11n support */
	if (desc->rate <= 15) // WL18XX_CONF_HW_RXTX_RATE_MCS0
		status->encoding = RX_ENC_HT;

#if (PRINT_DEBUG_RATE)
	printk("[RX_RATE] - RX rate = %s\n", wifi_rx_rate_to_string(desc->rate));
#endif

	/*
	* Read the signal level and antenna diversity indication.
	* The msb in the signal level is always set as it is a
	* negative number.
	* The antenna indication is the msb of the rssi.
	*/
	//status->signal = ((desc->rssi & RSSI_LEVEL_BITMASK) | BIT(7));
	status->signal = -66;
	status->antenna = ((desc->rssi & ANT_DIVERSITY_BITMASK) >> 7);

	/*
	 * FIXME: In wl1251, the SNR should be divided by two.  In wl1271 we
	 * need to divide by two for now, but TI has been discussing about
	 * changing it.  This needs to be rechecked.
	 */
	//wifi_data->noise = desc->rssi - (desc->snr >> 1);

	// status->freq is usually 0x994 = 2452 -> 2.4 GHz at ch 2, 5, 7, 9
	status->freq = ieee80211_channel_to_frequency(desc->channel,
						      status->band);
	//printk("desc->channel = 0x%x, freq = %d\n", desc->channel, status->freq);

	if (desc->flags & WL1271_RX_DESC_ENCRYPT_MASK) {
		u8 desc_err_code = desc->status & WL1271_RX_DESC_STATUS_MASK;

		status->flag |= RX_FLAG_IV_STRIPPED | RX_FLAG_MMIC_STRIPPED |
				RX_FLAG_DECRYPTED;

		if (unlikely(desc_err_code & WL1271_RX_DESC_MIC_FAIL)) {
			status->flag |= RX_FLAG_MMIC_ERROR;
			wifi_warning("Michael MIC error. Desc: 0x%x",
				       desc_err_code);
		}
	}

	if (beacon || probe_rsp)
		status->boottime_ns = ktime_get_boottime_ns();
		

	if (beacon)
		wificore_set_pending_regdomain_ch((u16)desc->channel,
						status->band);
}

static u32 wifi_get_rx_packet_len(void *rx_data,
				    u32 data_len)
{
	struct wifi_rx_descriptor *desc = rx_data;

	/* invalid packet */
	if (data_len < sizeof(*desc))
		return 0;

	return data_len - sizeof(*desc);
}

static int wifi_rx_handle_data(u8 *data, u32 length,
				 enum wl_rx_buf_align rx_align, u8 *hlid)
{
	struct wifi_rx_descriptor *desc;
	struct sk_buff *skb;
	struct ieee80211_hdr *hdr;
	u8 beacon = 0;
	u8 is_data = 0;
	u8 reserved = 0, offset_to_data = 0;
	u16 seq_num;
	u32 pkt_data_len;

	pkt_data_len = wifi_get_rx_packet_len(data, length); // length - sizeof(*desc);
	if (!pkt_data_len) {
		wifi_error("Invalid packet arrived from HW. length %d",
			     length);
		return -EINVAL;
	}

	if (rx_align == WLCORE_RX_BUF_UNALIGNED)
		reserved = RX_BUF_ALIGN;
	else if (rx_align == WLCORE_RX_BUF_PADDED)
		offset_to_data = RX_BUF_ALIGN;

	/* the data read starts with the descriptor */
	desc = (struct wifi_rx_descriptor *) data;

	/* discard corrupted packets */
	if (desc->status & WL1271_RX_DESC_DECRYPT_FAIL) {
		hdr = (void *)(data + sizeof(*desc) + offset_to_data);
		wifi_warning("corrupted packet in RX: status: 0x%x len: %d",
			       desc->status & WL1271_RX_DESC_STATUS_MASK,
			       pkt_data_len);
		wifi_dump((DEBUG_RX|DEBUG_CMD), "PKT: ", data + sizeof(*desc),
			    min(pkt_data_len,
				ieee80211_hdrlen(hdr->frame_control)));
		return -EINVAL;
	}

	/* skb length not including rx descriptor */
	skb = __dev_alloc_skb(pkt_data_len + reserved, GFP_KERNEL);
	if (!skb) {
		wifi_error("Couldn't allocate RX frame");
		return -ENOMEM;
	}

	/* reserve the unaligned payload(if any) */
	skb_reserve(skb, reserved);

	/*
	 * Copy packets from aggregation buffer to the skbs without rx
	 * descriptor and with packet payload aligned care. In case of unaligned
	 * packets copy the packets in offset of 2 bytes guarantee IP header
	 * payload aligned to 4 bytes.
	 */
	skb_put_data(skb, data + sizeof(*desc), pkt_data_len);
	if (rx_align == WLCORE_RX_BUF_PADDED)
		skb_pull(skb, RX_BUF_ALIGN);

#if (PRINT_DEBUG_DATA_FRAME)
	WIFI_Print_Hex(skb->data, (RX_LIMIT < skb->len) ? RX_LIMIT : skb->len, "RX frame");
#endif

	*hlid = desc->hlid;

	hdr = (struct ieee80211_hdr *)skb->data;
	if (ieee80211_is_beacon(hdr->frame_control))
		beacon = 1;
	if (ieee80211_is_data_present(hdr->frame_control))
		is_data = 1;

	// display info of wifi on userspace
	// status->signal = ((desc->rssi & RSSI_LEVEL_BITMASK) | BIT(7));
	wifi_rx_status(desc, IEEE80211_SKB_RXCB(skb), beacon,
			 ieee80211_is_probe_resp(hdr->frame_control));

	seq_num = (le16_to_cpu(hdr->seq_ctrl) & IEEE80211_SCTL_SEQ) >> 4;
	wifi_debug(DEBUG_RX, "rx skb 0x%p: %d B %s seq %d hlid %d", skb,
		     skb->len - desc->pad_len,
		     beacon ? "beacon" : "",
		     seq_num, *hlid);

#if (PRINT_DEBUG)
	printk("rx skb: beacon: %d, is_data: %d\n", beacon, is_data);
#endif

	skb_queue_tail(&wifi_deferred_rx_queue, skb);
	queue_work(wifi_work.freezable_wq, &wifi_work.netstack_work);

	return is_data;
}

static enum wl_rx_buf_align
wifi_get_rx_buf_align(u32 rx_desc)
{
	if (rx_desc & RX_BUF_PADDED_PAYLOAD) // bit 30
		return WLCORE_RX_BUF_PADDED;

	return WLCORE_RX_BUF_ALIGNED; // this is the return
}

#include "wl18xx.h"
int wificore_rx(void)
{
	unsigned long active_hlids[BITS_TO_LONGS(WLCORE_MAX_LINKS)] = {0};
	u32 buf_size;
	u32 fw_rx_counter = wifi_status_reg->fw_rx_counter % WL18XX_NUM_RX_DESCRIPTORS;
	u32 drv_rx_counter = wifi_rx_counter % WL18XX_NUM_RX_DESCRIPTORS;
	u32 rx_counter;
	u32 pkt_len, align_pkt_len;
	u32 pkt_offset, des;
	u8 hlid;
	enum wl_rx_buf_align rx_align;
	int ret = 0;

	/* update rates per link */
	hlid = wifi_status_reg->hlid;

	if (hlid < WLCORE_MAX_LINKS)
		wifi_links[hlid].fw_rate_mbps =
				wifi_status_reg->tx_last_rate_mbps;

	while (drv_rx_counter != fw_rx_counter) {
		buf_size = 0;
		rx_counter = drv_rx_counter;
		while (rx_counter != fw_rx_counter) {
			des = le32_to_cpu(wifi_status_reg->rx_pkt_descs[rx_counter]);
			pkt_len = wificore_rx_get_buf_size(des);
			align_pkt_len = wificore_rx_get_align_buf_size(pkt_len);
			if (buf_size + align_pkt_len > WL18XX_AGGR_BUFFER_SIZE)
				break;
			buf_size += align_pkt_len;
			rx_counter++;
			rx_counter %= WL18XX_NUM_RX_DESCRIPTORS;
		}

		if (buf_size == 0) {
			wifi_warning("received empty data");
			break;
		}

		// buf_size is don here to know the limit for breaking the wifi_aggr_buf below

		/* Read all available packets at once */
		des = le32_to_cpu(wifi_status_reg->rx_pkt_descs[drv_rx_counter]);
		
		ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_SLV_MEM_DATA]), (u32*)wifi_aggr_buf, buf_size, true);
		if (ret < 0)
			goto out;

		/* Split data into separate packets */
		pkt_offset = 0;
		while (pkt_offset < buf_size) {
			des = le32_to_cpu(wifi_status_reg->rx_pkt_descs[drv_rx_counter]);

			// des => [30] - rx_align, [23, 8] - pkt_len
			pkt_len = wificore_rx_get_buf_size(des);
			rx_align = wifi_get_rx_buf_align(des);

			/*
			 * the handle data call can only fail in memory-outage
			 * conditions, in that case the received frame will just
			 * be dropped.
			 */
			if (wifi_rx_handle_data(
						  wifi_aggr_buf + pkt_offset,
						  pkt_len, rx_align,
						  &hlid) == 1) {
				if (hlid < WL18XX_MAX_LINKS)
					__set_bit(hlid, active_hlids);
				else
					WARN(1,
					     "hlid (%d) exceeded MAX_LINKS\n",
					     hlid);
			}

			wifi_rx_counter++;
			drv_rx_counter++;
			drv_rx_counter %= WL18XX_NUM_RX_DESCRIPTORS;
			pkt_offset += wificore_rx_get_align_buf_size(pkt_len);
		}
	}

	/*
	 * Write the driver's packet counter to the FW. This is only required
	 * for older hardware revisions
	 */

out:
	return ret;
}

