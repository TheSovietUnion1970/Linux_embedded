// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl18xx
 *
 * Copyright (C) 2011 Texas Instruments
 */

#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/ip.h>
#include <linux/firmware.h>
#include <linux/etherdevice.h>
#include <linux/irq.h>

#include "wlcore.h"
#include "debug.h"
#include "io.h"
#include "acx.h"
#include "tx.h"
#include "rx.h"
#include "boot.h"
#include "ops.h"
#include "wl18.h"

#include "wl18xx.h"
#include "common.h"
#include "main.h"


/* Vinh custom */
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
int wl18xx_top_reg_write(int addr, u16 val)
{
	u32 tmp;
	int ret;

	if (WARN_ON(addr % 2))
		return -EINVAL;

	if ((addr % 4) == 0) {
		ret = wifi_sdio_raw_read(wificore_translate_addr(addr), &tmp, 4, false);
		if (ret < 0)
			goto out;

		tmp = (tmp & 0xffff0000) | val;
		ret = wifi_sdio_raw_write(wificore_translate_addr(addr), tmp, 4, false);
	} else {
		ret = wifi_sdio_raw_read(wificore_translate_addr(addr - 2), &tmp, 4, false);
		if (ret < 0)
			goto out;

		tmp = (tmp & 0xffff) | (val << 16);
		ret = wifi_sdio_raw_write(wificore_translate_addr(addr - 2), tmp, 4, false);
	}

out:
	return ret;
}

int wl18xx_top_reg_read(int addr, u16 *out)
{
	u32 val = 0;
	int ret;

	if (WARN_ON(addr % 2))
		return -EINVAL;

	if ((addr % 4) == 0) {
		/* address is 4-bytes aligned */
		ret = wifi_sdio_raw_read(wificore_translate_addr(addr), &val, 4, false);
		if (ret >= 0 && out)
			*out = val & 0xffff;
	} else {
		ret = wifi_sdio_raw_read(wificore_translate_addr(addr - 2), &val, 4, false);
		if (ret >= 0 && out)
			*out = (val & 0xffff0000) >> 16;
	}

	return ret;
}


#define WL18XX_TX_HW_BLOCK_SPARE        1
/* for special cases - namely, TKIP and GEM */
#define WL18XX_TX_HW_EXTRA_BLOCK_SPARE  2
#define WL18XX_TX_HW_BLOCK_SIZE         268

#define WL18XX_TX_STATUS_DESC_ID_MASK    0x7F
#define WL18XX_TX_STATUS_STAT_BIT_IDX    7

/* Indicates this TX HW frame is not padded to SDIO block size */
#define WL18XX_TX_CTRL_NOT_PADDED	BIT(7)

/*
 * The FW uses a special bit to indicate a wide channel should be used in
 * the rate policy.
 */
#define CONF_TX_RATE_USE_WIDE_CHAN BIT(31)


#define WL18XX_RX_CHECKSUM_MASK      0x40

static char *ht_mode_param = NULL;
static char *board_type_param = NULL;
static bool checksum_param = false;
static int num_rx_desc_param = -1;

/* phy paramters */
static int dc2dc_param = -1;
static int n_antennas_2_param = -1;
static int n_antennas_5_param = -1;
static int low_band_component_param = -1;
static int low_band_component_type_param = -1;
static int high_band_component_param = -1;
static int high_band_component_type_param = -1;
static int pwr_limit_reference_11_abg_param = -1;

static const u8 wl18xx_rate_to_idx_2ghz[] = {
	/* MCS rates are used only with 11n */
	15,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS15 */
	14,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS14 */
	13,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS13 */
	12,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS12 */
	11,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS11 */
	10,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS10 */
	9,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS9 */
	8,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS8 */
	7,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS7 */
	6,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS6 */
	5,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS5 */
	4,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS4 */
	3,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS3 */
	2,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS2 */
	1,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS1 */
	0,                             /* WL18XX_CONF_HW_RXTX_RATE_MCS0 */

	11,                            /* WL18XX_CONF_HW_RXTX_RATE_54   */
	10,                            /* WL18XX_CONF_HW_RXTX_RATE_48   */
	9,                             /* WL18XX_CONF_HW_RXTX_RATE_36   */
	8,                             /* WL18XX_CONF_HW_RXTX_RATE_24   */

	/* TI-specific rate */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* WL18XX_CONF_HW_RXTX_RATE_22   */

	7,                             /* WL18XX_CONF_HW_RXTX_RATE_18   */
	6,                             /* WL18XX_CONF_HW_RXTX_RATE_12   */
	3,                             /* WL18XX_CONF_HW_RXTX_RATE_11   */
	5,                             /* WL18XX_CONF_HW_RXTX_RATE_9    */
	4,                             /* WL18XX_CONF_HW_RXTX_RATE_6    */
	2,                             /* WL18XX_CONF_HW_RXTX_RATE_5_5  */
	1,                             /* WL18XX_CONF_HW_RXTX_RATE_2    */
	0                              /* WL18XX_CONF_HW_RXTX_RATE_1    */
};

static const u8 wl18xx_rate_to_idx_5ghz[] = {
	/* MCS rates are used only with 11n */
	15,                           /* WL18XX_CONF_HW_RXTX_RATE_MCS15 */
	14,                           /* WL18XX_CONF_HW_RXTX_RATE_MCS14 */
	13,                           /* WL18XX_CONF_HW_RXTX_RATE_MCS13 */
	12,                           /* WL18XX_CONF_HW_RXTX_RATE_MCS12 */
	11,                           /* WL18XX_CONF_HW_RXTX_RATE_MCS11 */
	10,                           /* WL18XX_CONF_HW_RXTX_RATE_MCS10 */
	9,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS9 */
	8,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS8 */
	7,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS7 */
	6,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS6 */
	5,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS5 */
	4,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS4 */
	3,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS3 */
	2,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS2 */
	1,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS1 */
	0,                            /* WL18XX_CONF_HW_RXTX_RATE_MCS0 */

	7,                             /* WL18XX_CONF_HW_RXTX_RATE_54   */
	6,                             /* WL18XX_CONF_HW_RXTX_RATE_48   */
	5,                             /* WL18XX_CONF_HW_RXTX_RATE_36   */
	4,                             /* WL18XX_CONF_HW_RXTX_RATE_24   */

	/* TI-specific rate */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* WL18XX_CONF_HW_RXTX_RATE_22   */

	3,                             /* WL18XX_CONF_HW_RXTX_RATE_18   */
	2,                             /* WL18XX_CONF_HW_RXTX_RATE_12   */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* WL18XX_CONF_HW_RXTX_RATE_11   */
	1,                             /* WL18XX_CONF_HW_RXTX_RATE_9    */
	0,                             /* WL18XX_CONF_HW_RXTX_RATE_6    */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* WL18XX_CONF_HW_RXTX_RATE_5_5  */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* WL18XX_CONF_HW_RXTX_RATE_2    */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* WL18XX_CONF_HW_RXTX_RATE_1    */
};

static const u8 *wl18xx_band_rate_to_idx[] = {
	[NL80211_BAND_2GHZ] = wl18xx_rate_to_idx_2ghz,
	[NL80211_BAND_5GHZ] = wl18xx_rate_to_idx_5ghz
};

// display here to map it to wl18xx_rate_to_idx_2ghz/ wl18xx_rate_to_idx_5ghz
enum wl18xx_hw_rates {
	WL18XX_CONF_HW_RXTX_RATE_MCS15 = 0,
	WL18XX_CONF_HW_RXTX_RATE_MCS14,
	WL18XX_CONF_HW_RXTX_RATE_MCS13,
	WL18XX_CONF_HW_RXTX_RATE_MCS12,
	WL18XX_CONF_HW_RXTX_RATE_MCS11,
	WL18XX_CONF_HW_RXTX_RATE_MCS10,
	WL18XX_CONF_HW_RXTX_RATE_MCS9,
	WL18XX_CONF_HW_RXTX_RATE_MCS8,
	WL18XX_CONF_HW_RXTX_RATE_MCS7,
	WL18XX_CONF_HW_RXTX_RATE_MCS6,
	WL18XX_CONF_HW_RXTX_RATE_MCS5,
	WL18XX_CONF_HW_RXTX_RATE_MCS4,
	WL18XX_CONF_HW_RXTX_RATE_MCS3,
	WL18XX_CONF_HW_RXTX_RATE_MCS2,
	WL18XX_CONF_HW_RXTX_RATE_MCS1,
	WL18XX_CONF_HW_RXTX_RATE_MCS0,
	WL18XX_CONF_HW_RXTX_RATE_54,
	WL18XX_CONF_HW_RXTX_RATE_48,
	WL18XX_CONF_HW_RXTX_RATE_36,
	WL18XX_CONF_HW_RXTX_RATE_24,
	WL18XX_CONF_HW_RXTX_RATE_22,
	WL18XX_CONF_HW_RXTX_RATE_18,
	WL18XX_CONF_HW_RXTX_RATE_12,
	WL18XX_CONF_HW_RXTX_RATE_11,
	WL18XX_CONF_HW_RXTX_RATE_9,
	WL18XX_CONF_HW_RXTX_RATE_6,
	WL18XX_CONF_HW_RXTX_RATE_5_5,
	WL18XX_CONF_HW_RXTX_RATE_2,
	WL18XX_CONF_HW_RXTX_RATE_1,
	WL18XX_CONF_HW_RXTX_RATE_MAX,
};

const char *wifi_rx_rate_to_string(u8 rate)
{
    switch (rate) {
    case WL18XX_CONF_HW_RXTX_RATE_MCS15:    return "MCS15 (130 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS14:    return "MCS14 (117 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS13:    return "MCS13 (104 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS12:    return "MCS12 (78 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS11:    return "MCS11 (65 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS10:    return "MCS10 (58.5 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS9:     return "MCS9 (52 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS8:     return "MCS8 (39 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS7:     return "MCS7 (65 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS6:     return "MCS6 (58.5 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS5:     return "MCS5 (52 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS4:     return "MCS4 (39 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS3:     return "MCS3 (26 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS2:     return "MCS2 (19.5 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS1:     return "MCS1 (13 Mbps)";
    case WL18XX_CONF_HW_RXTX_RATE_MCS0:     return "MCS0 (6.5 Mbps)";

    /* Legacy OFDM rates */
    case WL18XX_CONF_HW_RXTX_RATE_54:       return "54 Mbps (802.11g)";
    case WL18XX_CONF_HW_RXTX_RATE_48:       return "48 Mbps";
    case WL18XX_CONF_HW_RXTX_RATE_36:       return "36 Mbps";
    case WL18XX_CONF_HW_RXTX_RATE_24:       return "24 Mbps";
    case WL18XX_CONF_HW_RXTX_RATE_18:       return "18 Mbps";
    case WL18XX_CONF_HW_RXTX_RATE_12:       return "12 Mbps";
    case WL18XX_CONF_HW_RXTX_RATE_9:        return "9 Mbps";
    case WL18XX_CONF_HW_RXTX_RATE_6:        return "6 Mbps";

    /* Legacy CCK rates */
    case WL18XX_CONF_HW_RXTX_RATE_11:       return "11 Mbps (802.11b)";
    case WL18XX_CONF_HW_RXTX_RATE_5_5:      return "5.5 Mbps";
    case WL18XX_CONF_HW_RXTX_RATE_2:        return "2 Mbps";
    case WL18XX_CONF_HW_RXTX_RATE_1:        return "1 Mbps";

    /* Special cases */
    case WL18XX_CONF_HW_RXTX_RATE_22:       return "22 Mbps (TI special)";

    default:
        return "Unknown Rate";
    }
}

#define SCAN_MAX_CYCLE_INTERVALS 16

/* The FW intervals can take up to 16 entries.
 * The 1st entry isn't used (scan is immediate). The last
 * entry should be used for the long_interval
 */
#define SCAN_MAX_SHORT_INTERVALS (SCAN_MAX_CYCLE_INTERVALS - 2)

static struct wificore_conf wl18xx_conf = {
	.sg = {
		.params = {
			[WL18XX_CONF_SG_PARAM_0] = 0,
			/* Configuration Parameters */
			[WL18XX_CONF_SG_ANTENNA_CONFIGURATION] = 0,
			[WL18XX_CONF_SG_ZIGBEE_COEX] = 0,
			[WL18XX_CONF_SG_TIME_SYNC] = 0,
			[WL18XX_CONF_SG_PARAM_4] = 0,
			[WL18XX_CONF_SG_PARAM_5] = 0,
			[WL18XX_CONF_SG_PARAM_6] = 0,
			[WL18XX_CONF_SG_PARAM_7] = 0,
			[WL18XX_CONF_SG_PARAM_8] = 0,
			[WL18XX_CONF_SG_PARAM_9] = 0,
			[WL18XX_CONF_SG_PARAM_10] = 0,
			[WL18XX_CONF_SG_PARAM_11] = 0,
			[WL18XX_CONF_SG_PARAM_12] = 0,
			[WL18XX_CONF_SG_PARAM_13] = 0,
			[WL18XX_CONF_SG_PARAM_14] = 0,
			[WL18XX_CONF_SG_PARAM_15] = 0,
			[WL18XX_CONF_SG_PARAM_16] = 0,
			[WL18XX_CONF_SG_PARAM_17] = 0,
			[WL18XX_CONF_SG_PARAM_18] = 0,
			[WL18XX_CONF_SG_PARAM_19] = 0,
			[WL18XX_CONF_SG_PARAM_20] = 0,
			[WL18XX_CONF_SG_PARAM_21] = 0,
			[WL18XX_CONF_SG_PARAM_22] = 0,
			[WL18XX_CONF_SG_PARAM_23] = 0,
			[WL18XX_CONF_SG_PARAM_24] = 0,
			[WL18XX_CONF_SG_PARAM_25] = 0,
			/* Active Scan Parameters */
			[WL18XX_CONF_SG_AUTO_SCAN_PROBE_REQ] = 170,
			[WL18XX_CONF_SG_ACTIVE_SCAN_DURATION_FACTOR_HV3] = 50,
			[WL18XX_CONF_SG_PARAM_28] = 0,
			/* Passive Scan Parameters */
			[WL18XX_CONF_SG_PARAM_29] = 0,
			[WL18XX_CONF_SG_PARAM_30] = 0,
			[WL18XX_CONF_SG_PASSIVE_SCAN_DURATION_FACTOR_HV3] = 200,
			/* Passive Scan in Dual Antenna Parameters */
			[WL18XX_CONF_SG_CONSECUTIVE_HV3_IN_PASSIVE_SCAN] = 0,
			[WL18XX_CONF_SG_BEACON_HV3_COLL_TH_IN_PASSIVE_SCAN] = 0,
			[WL18XX_CONF_SG_TX_RX_PROTECT_BW_IN_PASSIVE_SCAN] = 0,
			/* General Parameters */
			[WL18XX_CONF_SG_STA_FORCE_PS_IN_BT_SCO] = 1,
			[WL18XX_CONF_SG_PARAM_36] = 0,
			[WL18XX_CONF_SG_BEACON_MISS_PERCENT] = 60,
			[WL18XX_CONF_SG_PARAM_38] = 0,
			[WL18XX_CONF_SG_RXT] = 1200,
			[WL18XX_CONF_SG_UNUSED] = 0,
			[WL18XX_CONF_SG_ADAPTIVE_RXT_TXT] = 1,
			[WL18XX_CONF_SG_GENERAL_USAGE_BIT_MAP] = 3,
			[WL18XX_CONF_SG_HV3_MAX_SERVED] = 6,
			[WL18XX_CONF_SG_PARAM_44] = 0,
			[WL18XX_CONF_SG_PARAM_45] = 0,
			[WL18XX_CONF_SG_CONSECUTIVE_CTS_THRESHOLD] = 2,
			[WL18XX_CONF_SG_GEMINI_PARAM_47] = 0,
			[WL18XX_CONF_SG_STA_CONNECTION_PROTECTION_TIME] = 0,
			/* AP Parameters */
			[WL18XX_CONF_SG_AP_BEACON_MISS_TX] = 3,
			[WL18XX_CONF_SG_PARAM_50] = 0,
			[WL18XX_CONF_SG_AP_BEACON_WINDOW_INTERVAL] = 2,
			[WL18XX_CONF_SG_AP_CONNECTION_PROTECTION_TIME] = 30,
			[WL18XX_CONF_SG_PARAM_53] = 0,
			[WL18XX_CONF_SG_PARAM_54] = 0,
			/* CTS Diluting Parameters */
			[WL18XX_CONF_SG_CTS_DILUTED_BAD_RX_PACKETS_TH] = 0,
			[WL18XX_CONF_SG_CTS_CHOP_IN_DUAL_ANT_SCO_MASTER] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_1] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_2] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_3] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_4] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_5] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_6] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_7] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_8] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_9] = 0,
			[WL18XX_CONF_SG_TEMP_PARAM_10] = 0,
		},
		.state = CONF_SG_PROTECTIVE,
	},
	.rx = {
		.rx_msdu_life_time           = 512000,
		.packet_detection_threshold  = 0,
		.ps_poll_timeout             = 15,
		.upsd_timeout                = 15,
		.rts_threshold               = IEEE80211_MAX_RTS_THRESHOLD,
		.rx_cca_threshold            = 0,
		.irq_blk_threshold           = 0xFFFF,
		.irq_pkt_threshold           = 0,
		.irq_timeout                 = 600,
		.queue_type                  = CONF_RX_QUEUE_TYPE_LOW_PRIORITY,
	},
	.tx = {
		.tx_energy_detection         = 0,
		.sta_rc_conf                 = {
			.enabled_rates       = 0,
			.short_retry_limit   = 10,
			.long_retry_limit    = 10,
			.aflags              = 0,
		},
		.ac_conf_count               = 4,
		.ac_conf                     = {
			[CONF_TX_AC_BE] = {
				.ac          = CONF_TX_AC_BE,
				.cw_min      = 15,
				.cw_max      = 63,
				.aifsn       = 3,
				.tx_op_limit = 0,
			},
			[CONF_TX_AC_BK] = {
				.ac          = CONF_TX_AC_BK,
				.cw_min      = 15,
				.cw_max      = 63,
				.aifsn       = 7,
				.tx_op_limit = 0,
			},
			[CONF_TX_AC_VI] = {
				.ac          = CONF_TX_AC_VI,
				.cw_min      = 15,
				.cw_max      = 63,
				.aifsn       = CONF_TX_AIFS_PIFS,
				.tx_op_limit = 3008,
			},
			[CONF_TX_AC_VO] = {
				.ac          = CONF_TX_AC_VO,
				.cw_min      = 15,
				.cw_max      = 63,
				.aifsn       = CONF_TX_AIFS_PIFS,
				.tx_op_limit = 1504,
			},
		},
		.max_tx_retries = 100,
		.ap_aging_period = 300,
		.tid_conf_count = 4,
		.tid_conf = {
			[CONF_TX_AC_BE] = {
				.queue_id    = CONF_TX_AC_BE,
				.channel_type = CONF_CHANNEL_TYPE_EDCF,
				.tsid        = CONF_TX_AC_BE,
				.ps_scheme   = CONF_PS_SCHEME_LEGACY,
				.ack_policy  = CONF_ACK_POLICY_LEGACY,
				.apsd_conf   = {0, 0},
			},
			[CONF_TX_AC_BK] = {
				.queue_id    = CONF_TX_AC_BK,
				.channel_type = CONF_CHANNEL_TYPE_EDCF,
				.tsid        = CONF_TX_AC_BK,
				.ps_scheme   = CONF_PS_SCHEME_LEGACY,
				.ack_policy  = CONF_ACK_POLICY_LEGACY,
				.apsd_conf   = {0, 0},
			},
			[CONF_TX_AC_VI] = {
				.queue_id    = CONF_TX_AC_VI,
				.channel_type = CONF_CHANNEL_TYPE_EDCF,
				.tsid        = CONF_TX_AC_VI,
				.ps_scheme   = CONF_PS_SCHEME_LEGACY,
				.ack_policy  = CONF_ACK_POLICY_LEGACY,
				.apsd_conf   = {0, 0},
			},
			[CONF_TX_AC_VO] = {
				.queue_id    = CONF_TX_AC_VO,
				.channel_type = CONF_CHANNEL_TYPE_EDCF,
				.tsid        = CONF_TX_AC_VO,
				.ps_scheme   = CONF_PS_SCHEME_LEGACY,
				.ack_policy  = CONF_ACK_POLICY_LEGACY,
				.apsd_conf   = {0, 0},
			},
		},
		.frag_threshold              = IEEE80211_MAX_FRAG_THRESHOLD,
		.tx_compl_timeout            = 350,
		.tx_compl_threshold          = 10,
		.basic_rate                  = CONF_HW_BIT_RATE_1MBPS,
		.basic_rate_5                = CONF_HW_BIT_RATE_6MBPS,
		.tmpl_short_retry_limit      = 10,
		.tmpl_long_retry_limit       = 10,
		.tx_watchdog_timeout         = 5000,
		.slow_link_thold             = 3,
		.fast_link_thold             = 30,
	},
	.conn = {
		.wake_up_event               = CONF_WAKE_UP_EVENT_DTIM,
		.listen_interval             = 1,
		.suspend_wake_up_event       = CONF_WAKE_UP_EVENT_N_DTIM,
		.suspend_listen_interval     = 3,
		.bcn_filt_mode               = CONF_BCN_FILT_MODE_ENABLED,
		.bcn_filt_ie_count           = 3,
		.bcn_filt_ie = {
			[0] = {
				.ie          = WLAN_EID_CHANNEL_SWITCH,
				.rule        = CONF_BCN_RULE_PASS_ON_APPEARANCE,
			},
			[1] = {
				.ie          = WLAN_EID_HT_OPERATION,
				.rule        = CONF_BCN_RULE_PASS_ON_CHANGE,
			},
			[2] = {
				.ie	     = WLAN_EID_ERP_INFO,
				.rule	     = CONF_BCN_RULE_PASS_ON_CHANGE,
			},
		},
		.synch_fail_thold            = 12,
		.bss_lose_timeout            = 400,
		.beacon_rx_timeout           = 10000,
		.broadcast_timeout           = 20000,
		.rx_broadcast_in_ps          = 1,
		.ps_poll_threshold           = 10,
		.bet_enable                  = CONF_BET_MODE_ENABLE,
		.bet_max_consecutive         = 50,
		.psm_entry_retries           = 8,
		.psm_exit_retries            = 16,
		.psm_entry_nullfunc_retries  = 3,
		.dynamic_ps_timeout          = 1500,
		.forced_ps                   = false,
		.keep_alive_interval         = 55000,
		.max_listen_interval         = 20,
		.sta_sleep_auth              = WL1271_PSM_ILLEGAL,
		.suspend_rx_ba_activity      = 0,
	},
	.itrim = {
		.enable = false,
		.timeout = 50000,
	},
	.pm_config = {
		.host_clk_settling_time = 5000,
		.host_fast_wakeup_support = CONF_FAST_WAKEUP_DISABLE,
	},
	.roam_trigger = {
		.trigger_pacing               = 1,
		.avg_weight_rssi_beacon       = 20,
		.avg_weight_rssi_data         = 10,
		.avg_weight_snr_beacon        = 20,
		.avg_weight_snr_data          = 10,
	},
	.scan = {
		.min_dwell_time_active        = 7500,
		.max_dwell_time_active        = 30000,
		.min_dwell_time_active_long   = 25000,
		.max_dwell_time_active_long   = 50000,
		.dwell_time_passive           = 100000,
		.dwell_time_dfs               = 150000,
		.num_probe_reqs               = 2,
		.split_scan_timeout           = 50000,
	},
	.sched_scan = {
		/*
		 * Values are in TU/1000 but since sched scan FW command
		 * params are in TUs rounding up may occur.
		 */
		.base_dwell_time		= 7500,
		.max_dwell_time_delta		= 22500,
		/* based on 250bits per probe @1Mbps */
		.dwell_time_delta_per_probe	= 2000,
		/* based on 250bits per probe @6Mbps (plus a bit more) */
		.dwell_time_delta_per_probe_5	= 350,
		.dwell_time_passive		= 100000,
		.dwell_time_dfs			= 150000,
		.num_probe_reqs			= 2,
		.rssi_threshold			= -90,
		.snr_threshold			= 0,
		.num_short_intervals		= SCAN_MAX_SHORT_INTERVALS,
		.long_interval			= 30000,
	},
	.ht = {
		.rx_ba_win_size = 32,
		.tx_ba_win_size = 64,
		.inactivity_timeout = 10000,
		.tx_ba_tid_bitmap = CONF_TX_BA_ENABLED_TID_BITMAP,
	},
	.mem = {
		.num_stations                 = 1,
		.ssid_profiles                = 1,
		.rx_block_num                 = 40,
		.tx_min_block_num             = 40,
		.dynamic_memory               = 1,
		.min_req_tx_blocks            = 45,
		.min_req_rx_blocks            = 22,
		.tx_min                       = 27,
	},
	.fm_coex = {
		.enable                       = true,
		.swallow_period               = 5,
		.n_divider_fref_set_1         = 0xff,       /* default */
		.n_divider_fref_set_2         = 12,
		.m_divider_fref_set_1         = 0xffff,
		.m_divider_fref_set_2         = 148,        /* default */
		.coex_pll_stabilization_time  = 0xffffffff, /* default */
		.ldo_stabilization_time       = 0xffff,     /* default */
		.fm_disturbed_band_margin     = 0xff,       /* default */
		.swallow_clk_diff             = 0xff,       /* default */
	},
	.rx_streaming = {
		.duration                      = 150,
		.queues                        = 0x1,
		.interval                      = 20,
		.always                        = 0,
	},
	.fwlog = {
		.mode                         = WL12XX_FWLOG_CONTINUOUS,
		.mem_blocks                   = 0,
		.severity                     = 0,
		.timestamp                    = WL12XX_FWLOG_TIMESTAMP_DISABLED,
		.output                       = WL12XX_FWLOG_OUTPUT_DBG_PINS,
		.threshold                    = 0,
	},
	.rate = {
		.rate_retry_score = 32000,
		.per_add = 8192,
		.per_th1 = 2048,
		.per_th2 = 4096,
		.max_per = 8100,
		.inverse_curiosity_factor = 5,
		.tx_fail_low_th = 4,
		.tx_fail_high_th = 10,
		.per_alpha_shift = 4,
		.per_add_shift = 13,
		.per_beta1_shift = 10,
		.per_beta2_shift = 8,
		.rate_check_up = 2,
		.rate_check_down = 12,
		.rate_retry_policy = {
			0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00,
		},
	},
	.hangover = {
		.recover_time               = 0,
		.hangover_period            = 20,
		.dynamic_mode               = 1,
		.early_termination_mode     = 1,
		.max_period                 = 20,
		.min_period                 = 1,
		.increase_delta             = 1,
		.decrease_delta             = 2,
		.quiet_time                 = 4,
		.increase_time              = 1,
		.window_size                = 16,
	},
	.recovery = {
		.bug_on_recovery	    = 0,
		.no_recovery		    = 0,
	},
};

static struct wifi_priv_conf wl18xx_default_priv_conf = {
	.ht = {
		.mode				= HT_MODE_WIDE,
	},
	.phy = {
		.phy_standalone			= 0x00,
		.primary_clock_setting_time	= 0x05,
		.clock_valid_on_wake_up		= 0x00,
		.secondary_clock_setting_time	= 0x05,
		.board_type 			= BOARD_TYPE_HDK_18XX,
		.auto_detect			= 0x00,
		.dedicated_fem			= FEM_NONE,
		.low_band_component		= COMPONENT_3_WAY_SWITCH,
		.low_band_component_type	= 0x05,
		.high_band_component		= COMPONENT_2_WAY_SWITCH,
		.high_band_component_type	= 0x09,
		.tcxo_ldo_voltage		= 0x00,
		.xtal_itrim_val			= 0x04,
		.srf_state			= 0x00,
		.io_configuration		= 0x01,
		.sdio_configuration		= 0x00,
		.settings			= 0x00,
		.enable_clpc			= 0x00,
		.enable_tx_low_pwr_on_siso_rdl	= 0x00,
		.rx_profile			= 0x00,
		.pwr_limit_reference_11_abg	= 0x64,
		.per_chan_pwr_limit_arr_11abg	= {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
		.pwr_limit_reference_11p	= 0x64,
		.per_chan_bo_mode_11_abg	= { 0x00, 0x00, 0x00, 0x00,
						    0x00, 0x00, 0x00, 0x00,
						    0x00, 0x00, 0x00, 0x00,
						    0x00 },
		.per_chan_bo_mode_11_p		= { 0x00, 0x00, 0x00, 0x00 },
		.per_chan_pwr_limit_arr_11p	= { 0xff, 0xff, 0xff, 0xff,
						    0xff, 0xff, 0xff },
		.psat				= 0,
		.external_pa_dc2dc		= 0,
		.number_of_assembled_ant2_4	= 2,
		.number_of_assembled_ant5	= 1,
		.low_power_val			= 0xff,
		.med_power_val			= 0xff,
		.high_power_val			= 0xff,
		.low_power_val_2nd		= 0xff,
		.med_power_val_2nd		= 0xff,
		.high_power_val_2nd		= 0xff,
		.tx_rf_margin			= 1,
	},
	.ap_sleep = {               /* disabled by default */
		.idle_duty_cycle        = 0,
		.connected_duty_cycle   = 0,
		.max_stations_thresh    = 0,
		.idle_conn_thresh       = 0,
	},
};

static const struct wifi_partition_set wl18xx_ptable[PART_TABLE_LEN] = {
	[PART_TOP_PRCM_ELP_SOC] = {
		.mem  = { .start = 0x00A00000, .size  = 0x00012000 },
		.reg  = { .start = 0x00807000, .size  = 0x00005000 },
		.mem2 = { .start = 0x00800000, .size  = 0x0000B000 },
		.mem3 = { .start = 0x00401594, .size  = 0x00001020 },
	},
	[PART_DOWN] = {
		.mem  = { .start = 0x00000000, .size  = 0x00014000 },
		.reg  = { .start = 0x00810000, .size  = 0x0000BFFF },
		.mem2 = { .start = 0x00000000, .size  = 0x00000000 },
		.mem3 = { .start = 0x00000000, .size  = 0x00000000 },
	},
	[PART_BOOT] = {
		.mem  = { .start = 0x00700000, .size = 0x0000030c },
		.reg  = { .start = 0x00802000, .size = 0x00014578 },
		.mem2 = { .start = 0x00B00404, .size = 0x00001000 },
		.mem3 = { .start = 0x00C00000, .size = 0x00000400 },
	},
	[PART_WORK] = {
		.mem  = { .start = 0x00800000, .size  = 0x000050FC },
		.reg  = { .start = 0x00B00404, .size  = 0x00001000 },
		.mem2 = { .start = 0x00C00000, .size  = 0x00000400 },
		.mem3 = { .start = 0x00401594, .size  = 0x00001020 },
	},
	[PART_PHY_INIT] = {
		.mem  = { .start = WL18XX_PHY_INIT_MEM_ADDR,
			  .size  = WL18XX_PHY_INIT_MEM_SIZE },
		.reg  = { .start = 0x00000000, .size = 0x00000000 },
		.mem2 = { .start = 0x00000000, .size = 0x00000000 },
		.mem3 = { .start = 0x00000000, .size = 0x00000000 },
	},
};

static const int wl18xx_rtable[REG_TABLE_LEN] = {
	[REG_ECPU_CONTROL]		= WL18XX_REG_ECPU_CONTROL,
	[REG_INTERRUPT_NO_CLEAR]	= WL18XX_REG_INTERRUPT_NO_CLEAR,
	[REG_INTERRUPT_ACK]		= WL18XX_REG_INTERRUPT_ACK,
	[REG_COMMAND_MAILBOX_PTR]	= WL18XX_REG_COMMAND_MAILBOX_PTR,
	[REG_EVENT_MAILBOX_PTR]		= WL18XX_REG_EVENT_MAILBOX_PTR,
	[REG_INTERRUPT_TRIG]		= WL18XX_REG_INTERRUPT_TRIG_H,
	[REG_INTERRUPT_MASK]		= WL18XX_REG_INTERRUPT_MASK,
	[REG_PC_ON_RECOVERY]		= WL18XX_SCR_PAD4,
	[REG_CHIP_ID_B]			= WL18XX_REG_CHIP_ID_B,
	[REG_CMD_MBOX_ADDRESS]		= WL18XX_CMD_MBOX_ADDRESS,

	/* data access memory addresses, used with partition translation */
	[REG_SLV_MEM_DATA]		= WL18XX_SLV_MEM_DATA,
	[REG_SLV_REG_DATA]		= WL18XX_SLV_REG_DATA,

	/* raw data access memory addresses */
	[REG_RAW_FW_STATUS_ADDR]	= WL18XX_FW_STATUS_ADDR,
};

static const struct wl18xx_clk_cfg wl18xx_clk_table_coex[NUM_CLOCK_CONFIGS] = {
	[CLOCK_CONFIG_16_2_M]	= { 8,  121, 0, 0, false },
	[CLOCK_CONFIG_16_368_M]	= { 8,  120, 0, 0, false },
	[CLOCK_CONFIG_16_8_M]	= { 8,  117, 0, 0, false },
	[CLOCK_CONFIG_19_2_M]	= { 10, 128, 0, 0, false },
	[CLOCK_CONFIG_26_M]	= { 11, 104, 0, 0, false },
	[CLOCK_CONFIG_32_736_M]	= { 8,  120, 0, 0, false },
	[CLOCK_CONFIG_33_6_M]	= { 8,  117, 0, 0, false },
	[CLOCK_CONFIG_38_468_M]	= { 10, 128, 0, 0, false },
	[CLOCK_CONFIG_52_M]	= { 11, 104, 0, 0, false },
};

static const struct wl18xx_clk_cfg wl18xx_clk_table[NUM_CLOCK_CONFIGS] = {
	[CLOCK_CONFIG_16_2_M]	= { 7,  104,  801, 4,  true },
	[CLOCK_CONFIG_16_368_M]	= { 9,  132, 3751, 4,  true },
	[CLOCK_CONFIG_16_8_M]	= { 7,  100,    0, 0, false },
	[CLOCK_CONFIG_19_2_M]	= { 8,  100,    0, 0, false },
	[CLOCK_CONFIG_26_M]	= { 13, 120,    0, 0, false },
	[CLOCK_CONFIG_32_736_M]	= { 9,  132, 3751, 4,  true },
	[CLOCK_CONFIG_33_6_M]	= { 7,  100,    0, 0, false },
	[CLOCK_CONFIG_38_468_M]	= { 8,  100,    0, 0, false },
	[CLOCK_CONFIG_52_M]	= { 13, 120,    0, 0, false },
};

static int wl18xx_set_clk(void)
{
	u16 clk_freq;
	int ret;

	ret = wifi_set_partition_core(&wifi_data->ptable[PART_TOP_PRCM_ELP_SOC]);
	if (ret < 0)
		goto out;

	/* TODO: PG2: apparently we need to read the clk type */

	ret = wl18xx_top_reg_read(PRIMARY_CLK_DETECT, &clk_freq);
	if (ret < 0)
		goto out;

	wifi_debug(DEBUG_BOOT, "clock freq %d (%d, %d, %d, %d, %s)", clk_freq,
		     wl18xx_clk_table[clk_freq].n, wl18xx_clk_table[clk_freq].m,
		     wl18xx_clk_table[clk_freq].p, wl18xx_clk_table[clk_freq].q,
		     wl18xx_clk_table[clk_freq].swallow ? "swallow" : "spit");

	/* coex PLL configuration */
	ret = wl18xx_top_reg_write(PLLSH_COEX_PLL_N,
				   wl18xx_clk_table_coex[clk_freq].n);
	if (ret < 0)
		goto out;

	ret = wl18xx_top_reg_write(PLLSH_COEX_PLL_M,
				   wl18xx_clk_table_coex[clk_freq].m);
	if (ret < 0)
		goto out;

	/* bypass the swallowing logic */
	ret = wl18xx_top_reg_write(PLLSH_COEX_PLL_SWALLOW_EN,
				   PLLSH_COEX_PLL_SWALLOW_EN_VAL1);
	if (ret < 0)
		goto out;

	ret = wl18xx_top_reg_write(PLLSH_WCS_PLL_N,
				   wl18xx_clk_table[clk_freq].n);
	if (ret < 0)
		goto out;

	ret = wl18xx_top_reg_write(PLLSH_WCS_PLL_M,
				   wl18xx_clk_table[clk_freq].m);
	if (ret < 0)
		goto out;

	if (wl18xx_clk_table[clk_freq].swallow) {
		/* first the 16 lower bits */
		ret = wl18xx_top_reg_write(PLLSH_WCS_PLL_Q_FACTOR_CFG_1,
					   wl18xx_clk_table[clk_freq].q &
					   PLLSH_WCS_PLL_Q_FACTOR_CFG_1_MASK);
		if (ret < 0)
			goto out;

		/* then the 16 higher bits, masked out */
		ret = wl18xx_top_reg_write(PLLSH_WCS_PLL_Q_FACTOR_CFG_2,
					(wl18xx_clk_table[clk_freq].q >> 16) &
					PLLSH_WCS_PLL_Q_FACTOR_CFG_2_MASK);
		if (ret < 0)
			goto out;

		/* first the 16 lower bits */
		ret = wl18xx_top_reg_write(PLLSH_WCS_PLL_P_FACTOR_CFG_1,
					   wl18xx_clk_table[clk_freq].p &
					   PLLSH_WCS_PLL_P_FACTOR_CFG_1_MASK);
		if (ret < 0)
			goto out;

		/* then the 16 higher bits, masked out */
		ret = wl18xx_top_reg_write(PLLSH_WCS_PLL_P_FACTOR_CFG_2,
					(wl18xx_clk_table[clk_freq].p >> 16) &
					PLLSH_WCS_PLL_P_FACTOR_CFG_2_MASK);
		if (ret < 0)
			goto out;
	} else {
		ret = wl18xx_top_reg_write(PLLSH_WCS_PLL_SWALLOW_EN,
					   PLLSH_WCS_PLL_SWALLOW_EN_VAL2);
		if (ret < 0)
			goto out;
	}

	/* choose WCS PLL */
	ret = wl18xx_top_reg_write(PLLSH_WL_PLL_SEL,
				   PLLSH_WL_PLL_SEL_WCS_PLL);
	if (ret < 0)
		goto out;

	/* enable both PLLs */
	ret = wl18xx_top_reg_write(PLLSH_WL_PLL_EN, PLLSH_WL_PLL_EN_VAL1);
	if (ret < 0)
		goto out;

	udelay(1000);

	/* disable coex PLL */
	ret = wl18xx_top_reg_write(PLLSH_WL_PLL_EN, PLLSH_WL_PLL_EN_VAL2);
	if (ret < 0)
		goto out;

	/* reset the swallowing logic */
	ret = wl18xx_top_reg_write(PLLSH_COEX_PLL_SWALLOW_EN,
				   PLLSH_COEX_PLL_SWALLOW_EN_VAL2);

out:
	return ret;
}

static int wl18xx_pre_boot(void)
{
	int ret;

	ret = wl18xx_set_clk();
	if (ret < 0)
		goto out;

	/* Continue the ELP wake up sequence */
	ret = wifi_sdio_raw_write(wificore_translate_addr(WL18XX_WELP_ARM_COMMAND), WELP_ARM_COMMAND_VAL, 4, false);
	if (ret < 0)
		goto out;

	udelay(500);

	ret = wifi_set_partition_core(&wifi_data->ptable[PART_BOOT]);
	if (ret < 0)
		goto out;

	/* Disable interrupts */
	ret = wifi_sdio_raw_write(wificore_translate_addr(wifi_data->rtable[REG_INTERRUPT_MASK]), WL1271_ACX_INTR_ALL, 4, false);
	if (ret < 0)
		goto out;

	/* disable Rx/Tx */
	ret = wifi_sdio_raw_write(wificore_translate_addr(WL18XX_ENABLE), 0x0, 4, false);
	if (ret < 0)
		goto out;

	/* disable auto calibration on start*/
	ret = wifi_sdio_raw_write(wificore_translate_addr(WL18XX_SPARE_A2), 0xffff, 4, false);

out:
	return ret;
}

static int wl18xx_pre_upload(void)
{
	u32 tmp;
	int ret;
	u16 irq_invert;

	BUILD_BUG_ON(sizeof(struct wl18xx_mac_and_phy_params) >
		WL18XX_PHY_INIT_MEM_SIZE);

	ret = wifi_set_partition_core(&wifi_data->ptable[PART_BOOT]);
	if (ret < 0)
		goto out;

	/* TODO: check if this is all needed */
	ret = wifi_sdio_raw_write(wificore_translate_addr(WL18XX_EEPROMLESS_IND), WL18XX_EEPROMLESS_IND, 4, false);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_CHIP_ID_B]), &tmp, 4, false);
	if (ret < 0)
		goto out;

	wifi_debug(DEBUG_BOOT, "chip id 0x%x", tmp);

	ret = wifi_sdio_raw_read(wificore_translate_addr(WL18XX_SCR_PAD2), &tmp, 4, false);
	if (ret < 0)
		goto out;

	/*
	 * Workaround for FDSP code RAM corruption (needed for PG2.1
	 * and newer; for older chips it's a NOP).  Change FDSP clock
	 * settings so that it's muxed to the ATGP clock instead of
	 * its own clock.
	 */

	ret = wifi_set_partition_core(&wifi_data->ptable[PART_PHY_INIT]);
	if (ret < 0)
		goto out;

	/* disable FDSP clock */
	ret = wifi_sdio_raw_write(wificore_translate_addr(WL18XX_PHY_FPGA_SPARE_1), MEM_FDSP_CLK_120_DISABLE, 4, false);
	if (ret < 0)
		goto out;

	/* set ATPG clock toward FDSP Code RAM rather than its own clock */
	ret = wifi_sdio_raw_write(wificore_translate_addr(WL18XX_PHY_FPGA_SPARE_1), MEM_FDSP_CODERAM_FUNC_CLK_SEL, 4, false);
	if (ret < 0)
		goto out;

	/* re-enable FDSP clock */
	ret = wifi_sdio_raw_write(wificore_translate_addr(WL18XX_PHY_FPGA_SPARE_1), MEM_FDSP_CLK_120_ENABLE, 4, false);
	if (ret < 0)
		goto out;

	ret = irq_get_trigger_type(wifi_data->irq);
	if ((ret == IRQ_TYPE_LEVEL_LOW) || (ret == IRQ_TYPE_EDGE_FALLING)) {
		printk("using inverted interrupt logic: %d", ret);
		ret = wifi_set_partition_core(&wifi_data->ptable[PART_TOP_PRCM_ELP_SOC]);
		if (ret < 0)
			goto out;

		ret = wl18xx_top_reg_read(TOP_FN0_CCCR_REG_32, &irq_invert);
		if (ret < 0)
			goto out;

		irq_invert |= BIT(1);
		ret = wl18xx_top_reg_write(TOP_FN0_CCCR_REG_32, irq_invert);
		if (ret < 0)
			goto out;

		ret = wifi_set_partition_core(&wifi_data->ptable[PART_PHY_INIT]);
	}

out:
	return ret;
}

static int wl18xx_set_mac_and_phy(void)
{
	struct wifi_priv *priv = wifi_data->priv;
	struct wl18xx_mac_and_phy_params *params;
	int ret;

	params = kmemdup(&priv->conf.phy, sizeof(*params), GFP_KERNEL);
	if (!params) {
		ret = -ENOMEM;
		goto out;
	}

	ret = wifi_set_partition_core(&wifi_data->ptable[PART_PHY_INIT]);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_write1(wificore_translate_addr(WL18XX_PHY_INIT_MEM_ADDR), params, sizeof(*params), false);

out:
	kfree(params);
	return ret;
}

static int wl18xx_enable_interrupts(void)
{
	u32 event_mask, intr_mask;
	int ret;

	event_mask = WL18XX_ACX_EVENTS_VECTOR;
	intr_mask = WL18XX_INTR_MASK;

	ret = wifi_sdio_raw_write(wificore_translate_addr(wifi_data->rtable[REG_INTERRUPT_MASK]), event_mask, 4, false);
	if (ret < 0)
		goto out;

	wificore_enable_interrupts();

	ret = wifi_sdio_raw_write(wificore_translate_addr(wifi_data->rtable[REG_INTERRUPT_MASK]), WL1271_ACX_INTR_ALL & ~intr_mask, 4, false);
	if (ret < 0)
		goto disable_interrupts;

	return ret;

disable_interrupts:
	wificore_disable_interrupts();

out:
	return ret;
}

static int wl18xx_boot(void)
{
	int ret;

	ret = wl18xx_pre_boot();
	if (ret < 0)
		goto out;

	ret = wl18xx_pre_upload();
	if (ret < 0)
		goto out;

	ret = wificore_boot_upload_firmware();
	if (ret < 0)
		goto out;

	ret = wl18xx_set_mac_and_phy();
	if (ret < 0)
		goto out;

	wifi_data->event_mask = BSS_LOSS_EVENT_ID |
		SCAN_COMPLETE_EVENT_ID |
		RADAR_DETECTED_EVENT_ID |
		RSSI_SNR_TRIGGER_0_EVENT_ID |
		PERIODIC_SCAN_COMPLETE_EVENT_ID |
		PERIODIC_SCAN_REPORT_EVENT_ID |
		DUMMY_PACKET_EVENT_ID |
		PEER_REMOVE_COMPLETE_EVENT_ID |
		BA_SESSION_RX_CONSTRAINT_EVENT_ID |
		REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID |
		INACTIVE_STA_EVENT_ID |
		CHANNEL_SWITCH_COMPLETE_EVENT_ID |
		DFS_CHANNELS_CONFIG_COMPLETE_EVENT |
		SMART_CONFIG_SYNC_EVENT_ID |
		SMART_CONFIG_DECODE_EVENT_ID |
		TIME_SYNC_EVENT_ID |
		FW_LOGGER_INDICATION |
		RX_BA_WIN_SIZE_CHANGE_EVENT_ID;

	//wifi_data->ap_event_mask = MAX_TX_FAILURE_EVENT_ID;

	ret = wificore_boot_run_firmware();
	if (ret < 0)
		goto out;

	ret = wl18xx_enable_interrupts();

out:
	return ret;
}

static int wifi_acx_host_if_cfg_bitmap(u32 host_cfg_bitmap,
				  u32 sdio_blk_size, u32 extra_mem_blks,
				  u32 len_field_size)
{
	struct wl18xx_acx_host_config_bitmap *bitmap_conf;
	int ret;

	wifi_debug(DEBUG_ACX, "acx cfg bitmap %d blk %d spare %d field %d",
		     host_cfg_bitmap, sdio_blk_size, extra_mem_blks,
		     len_field_size);

	bitmap_conf = kzalloc(sizeof(*bitmap_conf), GFP_KERNEL);
	if (!bitmap_conf) {
		ret = -ENOMEM;
		goto out;
	}

	bitmap_conf->host_cfg_bitmap = cpu_to_le32(host_cfg_bitmap);
	bitmap_conf->host_sdio_block_size = cpu_to_le32(sdio_blk_size);
	bitmap_conf->extra_mem_blocks = cpu_to_le32(extra_mem_blks);
	bitmap_conf->length_field_size = cpu_to_le32(len_field_size);

	ret = wifi_cmd_configure(ACX_HOST_IF_CFG_BITMAP,
				   bitmap_conf, sizeof(*bitmap_conf));
	if (ret < 0) {
		wifi_warning("wl1271 bitmap config opt failed: %d", ret);
		goto out;
	}

out:
	kfree(bitmap_conf);

	return ret;
}

static int wl18xx_set_host_cfg_bitmap(u32 extra_mem_blk)
{
	int ret;
	u32 sdio_align_size = 0;
	u32 host_cfg_bitmap = HOST_IF_CFG_RX_FIFO_ENABLE |
			      HOST_IF_CFG_ADD_RX_ALIGNMENT;

	/* Enable Tx SDIO padding */
	if (wifi_data->quirks & WLCORE_QUIRK_TX_BLOCKSIZE_ALIGN) {
		host_cfg_bitmap |= HOST_IF_CFG_TX_PAD_TO_SDIO_BLK;
		sdio_align_size = WL12XX_BUS_BLOCK_SIZE;
	}

	/* Enable Rx SDIO padding */
	if (wifi_data->quirks & WLCORE_QUIRK_RX_BLOCKSIZE_ALIGN) {
		host_cfg_bitmap |= HOST_IF_CFG_RX_PAD_TO_SDIO_BLK;
		sdio_align_size = WL12XX_BUS_BLOCK_SIZE;
	}

	ret = wifi_acx_host_if_cfg_bitmap(host_cfg_bitmap,
					    sdio_align_size, extra_mem_blk,
					    WL18XX_HOST_IF_LEN_SIZE_FIELD);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl18xx_hw_init(void)
{
	int ret;

	/* set the default amount of spare blocks in the bitmap */
	ret = wl18xx_set_host_cfg_bitmap(WL18XX_TX_HW_BLOCK_SPARE);
	if (ret < 0)
		return ret;

	return ret;
}

static bool wl18xx_is_mimo_supported(void)
{
	struct wifi_priv *priv = wifi_data->priv;

	/* only support MIMO with multiple antennas, and when SISO
	 * is not forced through config
	 */
	return (priv->conf.phy.number_of_assembled_ant2_4 >= 2) &&
	       (priv->conf.ht.mode != HT_MODE_WIDE) &&
	       (priv->conf.ht.mode != HT_MODE_SISO20);
}
#if (APPLY_EXTERNAL_CONFIG)
static int wl18xx_load_conf_file(struct device *dev, struct wificore_conf *conf,
				 struct wifi_priv_conf *priv_conf,
				 const char *file)
{
	struct wificore_conf_file *conf_file;
	const struct firmware *fw;
	int ret;

	ret = request_firmware(&fw, file, dev);
	if (ret < 0) {
		wifi_error("could not get configuration binary %s: %d",
			     file, ret);
		return ret;
	}

	if (fw->size != WL18XX_CONF_SIZE) {
		wifi_error("%s configuration binary size is wrong, expected %zu got %zu",
			     file, WL18XX_CONF_SIZE, fw->size);
		ret = -EINVAL;
		goto out_release;
	}

	conf_file = (struct wificore_conf_file *) fw->data;

	if (conf_file->header.magic != cpu_to_le32(WL18XX_CONF_MAGIC)) {
		wifi_error("configuration binary file magic number mismatch, "
			     "expected 0x%0x got 0x%0x", WL18XX_CONF_MAGIC,
			     conf_file->header.magic);
		ret = -EINVAL;
		goto out_release;
	}

	if (conf_file->header.version != cpu_to_le32(WL18XX_CONF_VERSION)) {
		wifi_error("configuration binary file version not supported, "
			     "expected 0x%08x got 0x%08x",
			     WL18XX_CONF_VERSION, conf_file->header.version);
		ret = -EINVAL;
		goto out_release;
	}

	memcpy(conf, &conf_file->core, sizeof(*conf));
	memcpy(priv_conf, &conf_file->priv, sizeof(*priv_conf));

out_release:
	release_firmware(fw);
	return ret;
}
#endif
static int wl18xx_conf_init(struct device *dev)
{
#if (APPLY_EXTERNAL_CONFIG)
	struct platform_device *pdev = wifi_data->pdev;
	struct wificore_platdev_data *pdata = dev_get_platdata(&pdev->dev);
#endif
	struct wifi_priv *priv = wifi_data->priv;

#if (APPLY_EXTERNAL_CONFIG)
	if (wl18xx_load_conf_file(dev, &wifi_data->conf, &priv->conf,
				  pdata->family->cfg_name) < 0) {
		wifi_warning("falling back to default config");

		/* apply driver default configuration */
		memcpy(&wifi_data->conf, &wl18xx_conf, sizeof(wifi_data->conf));
		/* apply default private configuration */
		memcpy(&priv->conf, &wl18xx_default_priv_conf,
		       sizeof(priv->conf));
	}
#else
	wifi_info("Apply default config\n");
	/* apply driver default configuration */
	memcpy(&wifi_data->conf, &wl18xx_conf, sizeof(wifi_data->conf));
	/* apply default private configuration */
	memcpy(&priv->conf, &wl18xx_default_priv_conf,
			sizeof(priv->conf));
#endif

	return 0;
}

static int wl18xx_setup(void);

static struct wificore_ops wl18xx_ops = {
	.setup		= wl18xx_setup,
	.boot		= wl18xx_boot,
	.hw_init	= wl18xx_hw_init,
};

/* HT cap appropriate for wide channels in 2Ghz */
static struct ieee80211_sta_ht_cap wl18xx_siso40_ht_cap_2ghz = {
	.cap = IEEE80211_HT_CAP_SGI_20 | IEEE80211_HT_CAP_SGI_40 |
	       IEEE80211_HT_CAP_SUP_WIDTH_20_40 | IEEE80211_HT_CAP_DSSSCCK40 |
	       IEEE80211_HT_CAP_GRN_FLD,
	.ht_supported = true,
	.ampdu_factor = IEEE80211_HT_MAX_AMPDU_16K,
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16,
	.mcs = {
		.rx_mask = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		.rx_highest = cpu_to_le16(150),
		.tx_params = IEEE80211_HT_MCS_TX_DEFINED,
		},
};

/* HT cap appropriate for wide channels in 5Ghz */
static struct ieee80211_sta_ht_cap wl18xx_siso40_ht_cap_5ghz = {
	.cap = IEEE80211_HT_CAP_SGI_20 | IEEE80211_HT_CAP_SGI_40 |
	       IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
	       IEEE80211_HT_CAP_GRN_FLD,
	.ht_supported = true,
	.ampdu_factor = IEEE80211_HT_MAX_AMPDU_16K,
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16,
	.mcs = {
		.rx_mask = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		.rx_highest = cpu_to_le16(150),
		.tx_params = IEEE80211_HT_MCS_TX_DEFINED,
		},
};

/* HT cap appropriate for SISO 20 */
static struct ieee80211_sta_ht_cap wl18xx_siso20_ht_cap = {
	.cap = IEEE80211_HT_CAP_SGI_20 |
	       IEEE80211_HT_CAP_GRN_FLD,
	.ht_supported = true,
	.ampdu_factor = IEEE80211_HT_MAX_AMPDU_16K,
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16,
	.mcs = {
		.rx_mask = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		.rx_highest = cpu_to_le16(72),
		.tx_params = IEEE80211_HT_MCS_TX_DEFINED,
		},
};

/* HT cap appropriate for MIMO rates in 20mhz channel */
static struct ieee80211_sta_ht_cap wl18xx_mimo_ht_cap_2ghz = {
	.cap = IEEE80211_HT_CAP_SGI_20 |
	       IEEE80211_HT_CAP_GRN_FLD,
	.ht_supported = true,
	.ampdu_factor = IEEE80211_HT_MAX_AMPDU_16K,
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16,
	.mcs = {
		.rx_mask = { 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, },
		.rx_highest = cpu_to_le16(144),
		.tx_params = IEEE80211_HT_MCS_TX_DEFINED,
		},
};

static const struct ieee80211_iface_limit wl18xx_iface_limits[] = {
	{
		.max = 2,
		.types = BIT(NL80211_IFTYPE_STATION),
	},
	{
		.max = 1,
		.types =   BIT(NL80211_IFTYPE_AP)
			 | BIT(NL80211_IFTYPE_P2P_GO)
			 | BIT(NL80211_IFTYPE_P2P_CLIENT)
#ifdef CONFIG_MAC80211_MESH
			 | BIT(NL80211_IFTYPE_MESH_POINT)
#endif
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_DEVICE),
	},
};

static const struct ieee80211_iface_limit wl18xx_iface_ap_limits[] = {
	{
		.max = 2,
		.types = BIT(NL80211_IFTYPE_AP),
	},
#ifdef CONFIG_MAC80211_MESH
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_MESH_POINT),
	},
#endif
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_DEVICE),
	},
};

static const struct ieee80211_iface_combination
wl18xx_iface_combinations[] = {
	{
		.max_interfaces = 3,
		.limits = wl18xx_iface_limits,
		.n_limits = ARRAY_SIZE(wl18xx_iface_limits),
		.num_different_channels = 2,
	},
	{
		.max_interfaces = 2,
		.limits = wl18xx_iface_ap_limits,
		.n_limits = ARRAY_SIZE(wl18xx_iface_ap_limits),
		.num_different_channels = 1,
		.radar_detect_widths =	BIT(NL80211_CHAN_NO_HT) |
					BIT(NL80211_CHAN_HT20) |
					BIT(NL80211_CHAN_HT40MINUS) |
					BIT(NL80211_CHAN_HT40PLUS),
	}
};

static inline void
wificore_set_ht_cap(enum nl80211_band band,
		  struct ieee80211_sta_ht_cap *ht_cap)
{
	memcpy(&wifi_data->ht_cap[band], ht_cap, sizeof(*ht_cap));
}

static int wl18xx_setup(void)
{
	struct wifi_priv *priv = wifi_data->priv;
	int ret;

	BUILD_BUG_ON(WL18XX_MAX_LINKS > WLCORE_MAX_LINKS);
	BUILD_BUG_ON(WL18XX_MAX_AP_STATIONS > WL18XX_MAX_LINKS);
	BUILD_BUG_ON(WL18XX_CONF_SG_PARAMS_MAX > WLCORE_CONF_SG_PARAMS_MAX);

	wifi_data->rtable = wl18xx_rtable;

	wifi_data->iface_combinations = wl18xx_iface_combinations;
	wifi_data->n_iface_combinations = ARRAY_SIZE(wl18xx_iface_combinations);
	wifi_data->num_mac_addr = WL18XX_NUM_MAC_ADDRESSES;
	wifi_data->static_data_priv_len = sizeof(struct wl18xx_static_data_priv);

	wifi_data->band_rate_to_idx = wl18xx_band_rate_to_idx;


	ret = wl18xx_conf_init(wifi_data->dev);
	if (ret < 0)
		return ret;

	/* If the module param is set, update it in conf */
	if (board_type_param) {
		if (!strcmp(board_type_param, "fpga")) {
			priv->conf.phy.board_type = BOARD_TYPE_FPGA_18XX;
		} else if (!strcmp(board_type_param, "hdk")) {
			priv->conf.phy.board_type = BOARD_TYPE_HDK_18XX;
		} else if (!strcmp(board_type_param, "dvp")) {
			priv->conf.phy.board_type = BOARD_TYPE_DVP_18XX;
		} else if (!strcmp(board_type_param, "evb")) {
			priv->conf.phy.board_type = BOARD_TYPE_EVB_18XX;
		} else if (!strcmp(board_type_param, "com8")) {
			priv->conf.phy.board_type = BOARD_TYPE_COM8_18XX;
		} else {
			wifi_error("invalid board type '%s'",
				board_type_param);
			return -EINVAL;
		}
	}

	if (priv->conf.phy.board_type >= NUM_BOARD_TYPES) {
		wifi_error("invalid board type '%d'",
			priv->conf.phy.board_type);
		return -EINVAL;
	}

	if (low_band_component_param != -1)
		priv->conf.phy.low_band_component = low_band_component_param;
	if (low_band_component_type_param != -1)
		priv->conf.phy.low_band_component_type =
			low_band_component_type_param;
	if (high_band_component_param != -1)
		priv->conf.phy.high_band_component = high_band_component_param;
	if (high_band_component_type_param != -1)
		priv->conf.phy.high_band_component_type =
			high_band_component_type_param;
	if (pwr_limit_reference_11_abg_param != -1)
		priv->conf.phy.pwr_limit_reference_11_abg =
			pwr_limit_reference_11_abg_param;
	if (n_antennas_2_param != -1)
		priv->conf.phy.number_of_assembled_ant2_4 = n_antennas_2_param;
	if (n_antennas_5_param != -1)
		priv->conf.phy.number_of_assembled_ant5 = n_antennas_5_param;
	if (dc2dc_param != -1)
		priv->conf.phy.external_pa_dc2dc = dc2dc_param;

	if (ht_mode_param) {
		if (!strcmp(ht_mode_param, "default"))
			priv->conf.ht.mode = HT_MODE_DEFAULT;
		else if (!strcmp(ht_mode_param, "wide"))
			priv->conf.ht.mode = HT_MODE_WIDE;
		else if (!strcmp(ht_mode_param, "siso20"))
			priv->conf.ht.mode = HT_MODE_SISO20;
		else {
			wifi_error("invalid ht_mode '%s'", ht_mode_param);
			return -EINVAL;
		}
	}

	if (priv->conf.ht.mode == HT_MODE_DEFAULT) {
		/*
		 * Only support mimo with multiple antennas. Fall back to
		 * siso40.
		 */
		if (wl18xx_is_mimo_supported())
			wificore_set_ht_cap(NL80211_BAND_2GHZ,
					  &wl18xx_mimo_ht_cap_2ghz);
		else
			wificore_set_ht_cap(NL80211_BAND_2GHZ,
					  &wl18xx_siso40_ht_cap_2ghz);

		/* 5Ghz is always wide */
		wificore_set_ht_cap(NL80211_BAND_5GHZ,
				  &wl18xx_siso40_ht_cap_5ghz);
	} else if (priv->conf.ht.mode == HT_MODE_WIDE) {
		wificore_set_ht_cap(NL80211_BAND_2GHZ,
				  &wl18xx_siso40_ht_cap_2ghz);
		wificore_set_ht_cap(NL80211_BAND_5GHZ,
				  &wl18xx_siso40_ht_cap_5ghz);
	} else if (priv->conf.ht.mode == HT_MODE_SISO20) {
		wificore_set_ht_cap(NL80211_BAND_2GHZ,
				  &wl18xx_siso20_ht_cap);
		wificore_set_ht_cap(NL80211_BAND_5GHZ,
				  &wl18xx_siso20_ht_cap);
	}

	/* Enable 11a Band only if we have 5G antennas */
	wifi_data->enable_11a = (priv->conf.phy.number_of_assembled_ant5 != 0);

	return 0;
}

static int wl18xx_probe(struct platform_device *pdev)
{
	//struct wl1271 *wl;
	struct ieee80211_hw *hw;
	int ret;

	hw = wificore_alloc_hw(sizeof(struct wifi_priv),
			     WL18XX_AGGR_BUFFER_SIZE,
			     sizeof(struct wl18xx_event_mailbox));
	if (IS_ERR(hw)) {
		wifi_error("can't allocate hw");
		ret = PTR_ERR(hw);
		goto out;
	}

	//wl = hw->priv;
	wifi_data->ops = &wl18xx_ops;
	wifi_data->ptable = wl18xx_ptable;

	ret = wificore_probe(pdev);
#if (PRINT_DEBUG)
	printk("[MERGE] - wifi_data->dev: 0x%x, parent = 0x%x\n", wifi_data->dev, wifi_data->dev->parent);
#endif
	if (ret)
		goto out_free;

	wifi_data->cmd_box_addr = devm_kzalloc(wifi_data->dev, 4, GFP_KERNEL);
	wifi_data->mbox_ptr[0] = (u32*)devm_kzalloc(wifi_data->dev, 4, GFP_KERNEL);
	wifi_data->mbox_ptr[1] = (u32*)devm_kzalloc(wifi_data->dev, 4, GFP_KERNEL);
	wifi_data->mbox = devm_kzalloc(wifi_data->dev, sizeof(struct wl18xx_event_mailbox), GFP_KERNEL | GFP_DMA);

	return ret;

out_free:
	wificore_free_hw();
out:
	return ret;
}

static const struct platform_device_id wl18xx_id_table[] = {
	{ "wl18xx", 0 },
	{  } /* Terminating Entry */
};
MODULE_DEVICE_TABLE(platform, wl18xx_id_table);

static struct platform_driver wl18xx_driver = {
	.probe		= wl18xx_probe,
	.remove		= wificore_remove,
	.id_table	= wl18xx_id_table,
	.driver = {
		.name	= "wl18xx_driver",
	}
};

module_platform_driver(wl18xx_driver);
module_param_named(ht_mode, ht_mode_param, charp, 0400);
MODULE_PARM_DESC(ht_mode, "Force HT mode: wide or siso20");

module_param_named(board_type, board_type_param, charp, 0400);
MODULE_PARM_DESC(board_type, "Board type: fpga, hdk (default), evb, com8 or "
		 "dvp");

module_param_named(checksum, checksum_param, bool, 0400);
MODULE_PARM_DESC(checksum, "Enable TCP checksum: boolean (defaults to false)");

module_param_named(dc2dc, dc2dc_param, int, 0400);
MODULE_PARM_DESC(dc2dc, "External DC2DC: u8 (defaults to 0)");

module_param_named(n_antennas_2, n_antennas_2_param, int, 0400);
MODULE_PARM_DESC(n_antennas_2,
		 "Number of installed 2.4GHz antennas: 1 (default) or 2");

module_param_named(n_antennas_5, n_antennas_5_param, int, 0400);
MODULE_PARM_DESC(n_antennas_5,
		 "Number of installed 5GHz antennas: 1 (default) or 2");

module_param_named(low_band_component, low_band_component_param, int, 0400);
MODULE_PARM_DESC(low_band_component, "Low band component: u8 "
		 "(default is 0x01)");

module_param_named(low_band_component_type, low_band_component_type_param,
		   int, 0400);
MODULE_PARM_DESC(low_band_component_type, "Low band component type: u8 "
		 "(default is 0x05 or 0x06 depending on the board_type)");

module_param_named(high_band_component, high_band_component_param, int, 0400);
MODULE_PARM_DESC(high_band_component, "High band component: u8, "
		 "(default is 0x01)");

module_param_named(high_band_component_type, high_band_component_type_param,
		   int, 0400);
MODULE_PARM_DESC(high_band_component_type, "High band component type: u8 "
		 "(default is 0x09)");

module_param_named(pwr_limit_reference_11_abg,
		   pwr_limit_reference_11_abg_param, int, 0400);
MODULE_PARM_DESC(pwr_limit_reference_11_abg, "Power limit reference: u8 "
		 "(default is 0xc8)");

module_param_named(num_rx_desc, num_rx_desc_param, int, 0400);
MODULE_PARM_DESC(num_rx_desc_param,
		 "Number of Rx descriptors: u8 (default is 32)");

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_FIRMWARE(WL18XX_FW_NAME);
