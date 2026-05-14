/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wl1271
 *
 * Copyright (C) 1998-2009 Texas Instruments. All rights reserved.
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#ifndef __WLCORE_I_H__
#define __WLCORE_I_H__

#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/bitops.h>
#include <net/mac80211.h>

#include "conf.h"
#include "ini.h"


struct wilink_family_data {
	const char *name;
	const char *nvs_name;	/* wl12xx nvs file */
	const char *cfg_name;	/* wl18xx cfg file */
};

#define WL1271_TX_SECURITY_LO16(s) ((u16)((s) & 0xffff))
#define WL1271_TX_SECURITY_HI32(s) ((u32)(((s) >> 16) & 0xffffffff))
#define WL1271_TX_SQN_POST_RECOVERY_PADDING 0xff
/* Use smaller padding for GEM, as some  APs have issues when it's too big */
#define WL1271_TX_SQN_POST_RECOVERY_PADDING_GEM 0x20


#define WL1271_CIPHER_SUITE_GEM 0x00147201

#define WL1271_BUSY_WORD_CNT 1
#define WL1271_BUSY_WORD_LEN (WL1271_BUSY_WORD_CNT * sizeof(u32))

#define WL1271_ELP_HW_STATE_ASLEEP 0
#define WL1271_ELP_HW_STATE_IRQ    1

#define WL1271_DEFAULT_BEACON_INT  100
#define WL1271_DEFAULT_DTIM_PERIOD 1

#define WL12XX_MAX_ROLES           4
#define WL12XX_INVALID_ROLE_ID     0xff
#define WL12XX_INVALID_LINK_ID     0xff

/*
 * max number of links allowed by all HWs.
 * this is NOT the actual max links supported by the current hw.
 */
#define WLCORE_MAX_LINKS 16

/* the driver supports the 2.4Ghz and 5Ghz bands */
#define WLCORE_NUM_BANDS           2

#define WL12XX_MAX_RATE_POLICIES 16
#define WLCORE_MAX_KLV_TEMPLATES 4

/* Defined by FW as 0. Will not be freed or allocated. */
#define WL12XX_SYSTEM_HLID         0

/*
 * When in AP-mode, we allow (at least) this number of packets
 * to be transmitted to FW for a STA in PS-mode. Only when packets are
 * present in the FW buffers it will wake the sleeping STA. We want to put
 * enough packets for the driver to transmit all of its buffered data before
 * the STA goes to sleep again. But we don't want to take too much memory
 * as it might hurt the throughput of active STAs.
 */
#define WL1271_PS_STA_MAX_PACKETS  2

#define WL1271_AP_BSS_INDEX        0
#define WL1271_AP_DEF_BEACON_EXP   20

enum wificore_state {
	WLCORE_STATE_OFF,
	WLCORE_STATE_RESTARTING,
	WLCORE_STATE_ON,
};

enum wifi_fw_type {
	WL12XX_FW_TYPE_NONE,
	WL12XX_FW_TYPE_NORMAL,
	WL12XX_FW_TYPE_MULTI,
	WL12XX_FW_TYPE_PLT,
};


enum {
	FW_VER_CHIP,
	FW_VER_IF_TYPE,
	FW_VER_MAJOR,
	FW_VER_SUBTYPE,
	FW_VER_MINOR,

	NUM_FW_VER
};

// struct wifi_chip {
// 	u32 id;
// 	char fw_ver_str[ETHTOOL_FWVERS_LEN];
// 	unsigned int fw_ver[NUM_FW_VER];
// 	char phy_fw_ver_str[ETHTOOL_FWVERS_LEN];
// };

#define NUM_TX_QUEUES              4

struct wl_fw_status {
	u32 intr;
	u8  fw_rx_counter;
	u8  drv_rx_counter;
	u8  tx_results_counter;
	__le32 *rx_pkt_descs;

	u32 fw_localtime;

	/*
	 * A bitmap (where each bit represents a single HLID)
	 * to indicate if the station is in PS mode.
	 */
	u32 link_ps_bitmap;

	/*
	 * A bitmap (where each bit represents a single HLID) to indicate
	 * if the station is in Fast mode
	 */
	u32 link_fast_bitmap;

	/* Cumulative counter of total released mem blocks since FW-reset */
	u32 total_released_blks;

	/* Size (in Memory Blocks) of TX pool */
	u32 tx_total;

	struct {
		/*
		 * Cumulative counter of released packets per AC
		 * (length of the array is NUM_TX_QUEUES)
		 */
		u8 *tx_released_pkts;

		/*
		 * Cumulative counter of freed packets per HLID
		 * (length of the array is wifi_data->num_links)
		 */
		u8 *tx_lnk_free_pkts;

		/* Cumulative counter of released Voice memory blocks */
		u8 tx_voice_released_blks;

		/* Tx rate of the last transmitted packet */
		u8 tx_last_rate;

		/* Tx rate or Tx rate estimate pre calculated by fw in mbps */
		u8 tx_last_rate_mbps;

		/* hlid for which the rates were reported */
		u8 hlid;
	} counters;

	u32 log_start_addr;

	/* Private status to be used by the lower drivers */
	void *priv;
};

#define WL1271_MAX_CHANNELS 64

struct wifi_if_operations {
	int __must_check (*read)(struct device *child, int addr, void *buf,
				 size_t len, bool fixed);
	int __must_check (*write)(struct device *child, int addr, void *buf,
				  size_t len, bool fixed);
	void (*reset)(struct device *child);
	void (*init)(struct device *child);
	int (*power)(struct device *child, bool enable);
	void (*set_block_size) (struct device *child, unsigned int blksz);
};

struct wificore_platdev_data {
	struct wifi_if_operations *if_ops;
	const struct wilink_family_data *family;

	bool ref_clock_xtal;	/* specify whether the clock is XTAL or not */
	u32 ref_clock_freq;	/* in Hertz */
	u32 tcxo_clock_freq;	/* in Hertz, tcxo is always XTAL */
	bool pwr_in_suspend;
};

#define MAX_NUM_KEYS 14
#define MAX_KEY_SIZE 32

struct wifi_ap_key {
	u8 id;
	u8 key_type;
	u8 key_size;
	u8 key[MAX_KEY_SIZE];
	u8 hlid;
	u32 tx_seq_32;
	u16 tx_seq_16;
	bool is_pairwise;
};

enum wifi_flags {
	WL1271_FLAG_GPIO_POWER,
	WL1271_FLAG_TX_QUEUE_STOPPED,
	WL1271_FLAG_TX_PENDING,
	WL1271_FLAG_IN_ELP,
	WL1271_FLAG_IRQ_RUNNING,
	WL1271_FLAG_FW_TX_BUSY,
	WL1271_FLAG_DUMMY_PACKET_PENDING,
	WL1271_FLAG_SUSPENDED,
	WL1271_FLAG_PENDING_WORK,
	WL1271_FLAG_SOFT_GEMINI,
	WL1271_FLAG_RECOVERY_IN_PROGRESS,
	WL1271_FLAG_VIF_CHANGE_IN_PROGRESS,
	WL1271_FLAG_INTENDED_FW_RECOVERY,
	WL1271_FLAG_IO_FAILED,
	WL1271_FLAG_REINIT_TX_WDOG,
};

enum wifi_vif_flags {
	wifi_vif_FLAG_INITIALIZED,
	wifi_vif_FLAG_STA_ASSOCIATED,
	wifi_vif_FLAG_STA_AUTHORIZED,
	wifi_vif_FLAG_IBSS_JOINED,
	wifi_vif_FLAG_AP_STARTED,
	wifi_vif_FLAG_IN_PS,
	wifi_vif_FLAG_STA_STATE_SENT,
	wifi_vif_FLAG_RX_STREAMING_STARTED,
	wifi_vif_FLAG_PSPOLL_FAILURE,
	wifi_vif_FLAG_CS_PROGRESS,
	wifi_vif_FLAG_AP_PROBE_RESP_SET,
	wifi_vif_FLAG_IN_USE,
	wifi_vif_FLAG_ACTIVE,
	wifi_vif_FLAG_BEACON_DISABLED,
};

#define WL1271_MAX_RX_FILTERS 5
#define WL1271_RX_FILTER_MAX_FIELDS 8

#define WL1271_RX_FILTER_ETH_HEADER_SIZE 14
#define WL1271_RX_FILTER_MAX_FIELDS_SIZE 95
#define RX_FILTER_FIELD_OVERHEAD				\
	(sizeof(struct wifi_rx_filter_field) - sizeof(u8 *))
#define WL1271_RX_FILTER_MAX_PATTERN_SIZE			\
	(WL1271_RX_FILTER_MAX_FIELDS_SIZE - RX_FILTER_FIELD_OVERHEAD)

#define WL1271_RX_FILTER_FLAG_MASK                BIT(0)
#define WL1271_RX_FILTER_FLAG_IP_HEADER           0
#define WL1271_RX_FILTER_FLAG_ETHERNET_HEADER     BIT(1)

enum rx_filter_action {
	FILTER_DROP = 0,
	FILTER_SIGNAL = 1,
	FILTER_FW_HANDLE = 2
};

enum plt_mode {
	PLT_OFF = 0,
	PLT_ON = 1,
	PLT_FEM_DETECT = 2,
	PLT_CHIP_AWAKE = 3
};

struct wifi_rx_filter_field {
	__le16 offset;
	u8 len;
	u8 flags;
	u8 *pattern;
} __packed;

struct wifi_rx_filter {
	u8 action;
	int num_fields;
	struct wifi_rx_filter_field fields[WL1271_RX_FILTER_MAX_FIELDS];
};

struct wifi_station {
	u8 hlid;
	bool in_connection;

	/*
	 * total freed FW packets on the link to the STA - used for tracking the
	 * AES/TKIP PN across recoveries. Re-initialized each time from the
	 * wifi_station structure.
	 * Used in both AP and STA mode.
	 */
	u64 total_freed_pkts;
};


static inline struct wifi_vif *wifi_vif_to_data(struct ieee80211_vif *vif)
{
	WARN_ON(!vif);
	return (struct wifi_vif *)vif->drv_priv;
}

static inline
struct ieee80211_vif *wifi_wifi_vif_to_vif(struct wifi_vif *wifi_vif)
{
	return container_of((void *)wifi_vif, struct ieee80211_vif, drv_priv);
}

static inline bool wificore_is_p2p_mgmt(struct wifi_vif *wifi_vif)
{
	return wifi_wifi_vif_to_vif(wifi_vif)->type == NL80211_IFTYPE_P2P_DEVICE;
}

#define wifi_for_each_wifi_vif(wifi_vif) \
		list_for_each_entry(wifi_vif, &wifi_data->wifi_vif_list, list)

void wifi_queue_recovery_work(void);

#define JOIN_TIMEOUT 5000 /* 5000 milliseconds to join */

#define SESSION_COUNTER_MAX 6 /* maximum value for the session counter */
#define SESSION_COUNTER_INVALID 7 /* used with dummy_packet */

#define WL1271_DEFAULT_POWER_LEVEL 0

#define WL1271_TX_QUEUE_LOW_WATERMARK  32
#define WL1271_TX_QUEUE_HIGH_WATERMARK 256

#define WL1271_DEFERRED_QUEUE_LIMIT    64

/* WL1271 needs a 200ms sleep after power on, and a 20ms sleep before power
   on in case is has been shut down shortly before */
#define WL1271_PRE_POWER_ON_SLEEP 20 /* in milliseconds */
#define WL1271_POWER_ON_SLEEP 200 /* in milliseconds */

/* Macros to handle wl1271.sta_rate_set */
#define HW_BG_RATES_MASK	0xffff
#define HW_HT_RATES_OFFSET	16
#define HW_MIMO_RATES_OFFSET	24

#endif /* __WLCORE_I_H__ */
