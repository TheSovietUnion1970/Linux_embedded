/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wl1271
 *
 * Copyright (C) 1998-2009 Texas Instruments. All rights reserved.
 * Copyright (C) 2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#ifndef __TX_H__
#define __TX_H__

#define TX_HW_MGMT_PKT_LIFETIME_TU       2000
#define TX_HW_AP_MODE_PKT_LIFETIME_TU    8000

#define TX_HW_ATTR_SAVE_RETRIES          BIT(0)
#define TX_HW_ATTR_HEADER_PAD            BIT(1)
#define TX_HW_ATTR_SESSION_COUNTER       (BIT(2) | BIT(3) | BIT(4))
#define TX_HW_ATTR_RATE_POLICY           (BIT(5) | BIT(6) | BIT(7) | \
					  BIT(8) | BIT(9))
#define TX_HW_ATTR_LAST_WORD_PAD         (BIT(10) | BIT(11))
#define TX_HW_ATTR_TX_CMPLT_REQ          BIT(12)
#define TX_HW_ATTR_TX_DUMMY_REQ          BIT(13)
#define TX_HW_ATTR_HOST_ENCRYPT          BIT(14)
#define TX_HW_ATTR_EAPOL_FRAME           BIT(15)

#define TX_HW_ATTR_OFST_SAVE_RETRIES     0
#define TX_HW_ATTR_OFST_HEADER_PAD       1
#define TX_HW_ATTR_OFST_SESSION_COUNTER  2
#define TX_HW_ATTR_OFST_RATE_POLICY      5
#define TX_HW_ATTR_OFST_LAST_WORD_PAD    10
#define TX_HW_ATTR_OFST_TX_CMPLT_REQ     12

#define TX_HW_RESULT_QUEUE_LEN           16
#define TX_HW_RESULT_QUEUE_LEN_MASK      0xf

#define WL1271_TX_ALIGN_TO 4
#define WL1271_EXTRA_SPACE_TKIP 4
#define WL1271_EXTRA_SPACE_AES  8
#define WL1271_EXTRA_SPACE_MAX  8

#include "common.h"

/* Used for management frames and dummy packets */
#define WL1271_TID_MGMT 7

/* stop a ROC for pending authentication reply after this time (ms) */
#define WLCORE_PEND_AUTH_ROC_TIMEOUT     1000

struct wl127x_tx_mem {
	/*
	 * Number of extra memory blocks to allocate for this packet
	 * in addition to the number of blocks derived from the packet
	 * length.
	 */
	u8 extra_blocks;
	/*
	 * Total number of memory blocks allocated by the host for
	 * this packet. Must be equal or greater than the actual
	 * blocks number allocated by HW.
	 */
	u8 total_mem_blocks;
} __packed;

struct wl128x_tx_mem {
	/*
	 * Total number of memory blocks allocated by the host for
	 * this packet.
	 */
	u8 total_mem_blocks;
	/*
	 * Number of extra bytes, at the end of the frame. the host
	 * uses this padding to complete each frame to integer number
	 * of SDIO blocks.
	 */
	u8 extra_bytes;
} __packed;

struct wl18xx_tx_mem {
	/*
	 * Total number of memory blocks allocated by the host for
	 * this packet.
	 */
	u8 total_mem_blocks;

	/*
	 * control bits
	 */
	u8 ctrl;
} __packed;

/*
 * On wl128x based devices, when TX packets are aggregated, each packet
 * size must be aligned to the SDIO block size. The maximum block size
 * is bounded by the type of the padded bytes field that is sent to the
 * FW. Currently the type is u8, so the maximum block size is 256 bytes.
 */
#define WL12XX_BUS_BLOCK_SIZE min(512u,	\
	    (1u << (8 * sizeof(((struct wl128x_tx_mem *) 0)->extra_bytes))))

struct wifi_tx_hw_descr {
	/* Length of packet in words, including descriptor+header+data */
	__le16 length;
	union {
		struct wl127x_tx_mem wl127x_mem;
		struct wl128x_tx_mem wl128x_mem;
		struct wl18xx_tx_mem wl18xx_mem;
	} __packed;
	/* Device time (in us) when the packet arrived to the driver */
	__le32 start_time;
	/*
	 * Max delay in TUs until transmission. The last device time the
	 * packet can be transmitted is: start_time + (1024 * life_time)
	 */
	__le16 life_time;
	/* Bitwise fields - see TX_ATTR... definitions above. */
	__le16 tx_attr;
	/* Packet identifier used also in the Tx-Result. */
	u8 id;
	/* The packet TID value (as User-Priority) */
	u8 tid;
	/* host link ID (HLID) */
	u8 hlid;

	union {
		u8 wifi_reserved;

		/*
		 * bit 0   -> 0 = udp, 1 = tcp
		 * bit 1:7 -> IP header offset
		 */
		u8 wl18xx_checksum_data;
	} __packed;
} __packed;

enum wifi_tx_hw_res_status {
	TX_SUCCESS          = 0,
	TX_HW_ERROR         = 1,
	TX_DISABLED         = 2,
	TX_RETRY_EXCEEDED   = 3,
	TX_TIMEOUT          = 4,
	TX_KEY_NOT_FOUND    = 5,
	TX_PEER_NOT_FOUND   = 6,
	TX_SESSION_MISMATCH = 7,
	TX_LINK_NOT_VALID   = 8,
};

struct wifi_tx_hw_res_descr {
	/* Packet Identifier - same value used in the Tx descriptor.*/
	u8 id;
	/* The status of the transmission, indicating success or one of
	   several possible reasons for failure. */
	u8 status;
	/* Total air access duration including all retrys and overheads.*/
	__le16 medium_usage;
	/* The time passed from host xfer to Tx-complete.*/
	__le32 fw_handling_time;
	/* Total media delay
	   (from 1st EDCA AIFS counter until TX Complete). */
	__le32 medium_delay;
	/* LS-byte of last TKIP seq-num (saved per AC for recovery). */
	u8 tx_security_sequence_number_lsb;
	/* Retry count - number of transmissions without successful ACK.*/
	u8 ack_failures;
	/* The rate that succeeded getting ACK
	   (Valid only if status=SUCCESS). */
	u8 rate_class_index;
	/* for 4-byte alignment. */
	u8 spare;
} __packed;

struct wifi_tx_hw_res_if {
	__le32 tx_result_fw_counter;
	__le32 tx_result_host_counter;
	struct wifi_tx_hw_res_descr tx_results_queue[TX_HW_RESULT_QUEUE_LEN];
} __packed;

enum wificore_queue_stop_reason {
	WLCORE_QUEUE_STOP_REASON_WATERMARK,
	WLCORE_QUEUE_STOP_REASON_FW_RESTART,
	WLCORE_QUEUE_STOP_REASON_FLUSH,
	WLCORE_QUEUE_STOP_REASON_SPARE_BLK, /* 18xx specific */
};

static inline
int wificore_tx_get_mac80211_queue(struct wifi_vif *wifi_vif, int queue)
{
	//int mac_queue = wifi_vif->hw_queue_base;
	int mac_queue = HW_QUEUE_BASE;

	switch (queue) {
	case CONF_TX_AC_VO:
		return mac_queue + 0;
	case CONF_TX_AC_VI:
		return mac_queue + 1;
	case CONF_TX_AC_BE:
		return mac_queue + 2;
	case CONF_TX_AC_BK:
		return mac_queue + 3;
	default:
		return mac_queue + 2;
	}
}

static inline int wifi_tx_total_queue_count(void)
{
	int i, count = 0;

	for (i = 0; i < NUM_TX_QUEUES; i++)
		//count += wifi_data->tx_queue_count[i];
		count += wifi_tx_queue_count[i];

	return count;
}

void wifi_tx_work(struct work_struct *work);
int wificore_tx_work_locked(void);
void wifi_tx_reset_wifi_vif(struct wifi_vif *wifi_vif);
void wifi_tx_reset(void);
void wifi_tx_flush(void);
u8 wificore_rate_to_idx(u8 rate, enum nl80211_band band);
u32 wifi_tx_enabled_rates_get(u32 rate_set,
				enum nl80211_band rate_band);
u32 wifi_tx_min_rate_get(u32 rate_set);
u8 wifi_tx_get_hlid(struct wifi_vif *wifi_vif,
		      struct sk_buff *skb, struct ieee80211_sta *sta);
void wifi_tx_reset_link_queues(u8 hlid);
bool wifi_is_dummy_packet(struct sk_buff *skb);
void wifi_free_tx_id(int id);
void wificore_stop_queues(
			enum wificore_queue_stop_reason reason);
void wificore_wake_queues(
			enum wificore_queue_stop_reason reason);
bool
wificore_is_queue_stopped_by_reason_locked(
					 struct wifi_vif *wifi_vif,
					 u8 queue,
					 enum wificore_queue_stop_reason reason);

/* from main.c */
void wifi_free_sta(struct wifi_vif *wifi_vif, u8 hlid);
void wifi_rearm_tx_watchdog_locked(void);

#endif
