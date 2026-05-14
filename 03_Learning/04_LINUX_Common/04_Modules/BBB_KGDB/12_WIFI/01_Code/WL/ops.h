#ifndef OPS_H
#define OPS_H

#include "reg.h"
#include <linux/ieee80211.h>
// #include "io.h"
#include "cmd.h"
#include "scan.h"
#include "common.h"
#include "io.h"
#include "main.h"

struct wifi_cmd_scan_stop {
	struct wifi_cmd_header header;

	u8 role_id;
	u8 scan_type;
	u8 padding[2];
} __packed;

#define SCAN_MAX_BANDS 3
#define MAX_CHANNELS_2GHZ	14
#define MAX_CHANNELS_4GHZ	4
#define MAX_CHANNELS_5GHZ	42
#define WL18XX_MAX_CHANNELS_5GHZ 32
/* probe request rate */
enum
{
	WL18XX_SCAN_RATE_1	= 0,
	WL18XX_SCAN_RATE_5_5	= 1,
	WL18XX_SCAN_RATE_6	= 2,
};
struct wifi_scan_ch_params {
	__le16 min_duration;
	__le16 max_duration;
	__le16 passive_duration;

	u8  channel;
	u8  tx_power_att;

	/* bit 0: DFS channel; bit 1: DFS enabled */
	u8  flags;

	u8  padding[3];
} __packed;

struct wifi_scan_channels {
	u8 passive[SCAN_MAX_BANDS]; /* number of passive scan channels */
	u8 active[SCAN_MAX_BANDS];  /* number of active scan channels */
	u8 dfs;		   /* number of dfs channels in 5ghz */
	u8 passive_active; /* number of passive before active channels 2.4ghz */

	struct wifi_scan_ch_params channels_2[MAX_CHANNELS_2GHZ];
	struct wifi_scan_ch_params channels_5[MAX_CHANNELS_5GHZ];
	struct wifi_scan_ch_params channels_4[MAX_CHANNELS_4GHZ];
};

struct wifi_tracking_ch_params {
	struct wifi_scan_ch_params channel;

	__le32 bssid_lsb;
	__le16 bssid_msb;

	u8 padding[2];
} __packed;

struct wifi_cmd_scan_params {
	struct wifi_cmd_header header;

	u8 role_id;
	u8 scan_type;

	s8 rssi_threshold; /* for filtering (in dBm) */
	s8 snr_threshold;  /* for filtering (in dB) */

	u8 bss_type;	   /* for filtering */
	u8 ssid_from_list; /* use ssid from configured ssid list */
	u8 filter;	   /* forward only results with matching ssids */

	/*
	 * add broadcast ssid in addition to the configured ssids.
	 * the driver should add dummy entry for it (?).
	 */
	u8 add_broadcast;

	u8 urgency;
	u8 protect;	 /* ??? */
	u8 n_probe_reqs;    /* Number of probes requests per channel */
	u8 terminate_after; /* early terminate scan operation */

	u8 passive[SCAN_MAX_BANDS]; /* number of passive scan channels */
	u8 active[SCAN_MAX_BANDS];  /* number of active scan channels */
	u8 dfs;		   /* number of dfs channels in 5ghz */
	u8 passive_active; /* number of passive before active channels 2.4ghz */

	__le16 short_cycles_msec;
	__le16 long_cycles_msec;
	u8 short_cycles_count;
	u8 total_cycles; /* 0 - infinite */
	u8 padding[2];

	union {
		struct {
			struct wifi_scan_ch_params channels_2[MAX_CHANNELS_2GHZ];
			struct wifi_scan_ch_params channels_5[WL18XX_MAX_CHANNELS_5GHZ];
			struct wifi_scan_ch_params channels_4[MAX_CHANNELS_4GHZ];
		};
		struct wifi_tracking_ch_params channels_tracking[WL1271_SCAN_MAX_CHANNELS]; // WL1271_SCAN_MAX_CHANNELS
	} ;

	u8 ssid[IEEE80211_MAX_SSID_LEN];
	u8 ssid_len;	 /* For SCAN_SSID_FILTER_SPECIFIC */
	u8 tag;
	u8 rate;

	/* send SCAN_REPORT_EVENT in periodic scans after each cycle
	* if number of results >= report_threshold. Must be 0 for
	* non periodic scans
	*/
	u8 report_threshold;

	/* Should periodic scan stop after a report event was created.
	* Must be 0 for non periodic scans.
	*/
	u8 terminate_on_report;

	u8 padding1[3];
} __packed;

int wifi_scan_send(struct wifi_vif *wifi_vif,
			    struct cfg80211_scan_request *req);

int wifi_get_mac(void);

#define WL18XX_TRACE_LOSS_GAPS_TX 10
#define WL18XX_TRACE_LOSS_GAPS_RX 18
#define NUM_OF_CHANNELS_11_ABG 150
#define NUM_OF_CHANNELS_11_P 7
#define SRF_TABLE_LEN 16
#define PIN_MUXING_SIZE 2
struct wifi_conf_ap_sleep_settings {
	/* Duty Cycle (20-80% of staying Awake) for IDLE AP
	 * (0: disable)
	 */
	u8 idle_duty_cycle;
	/* Duty Cycle (20-80% of staying Awake) for Connected AP
	 * (0: disable)
	 */
	u8 connected_duty_cycle;
	/* Maximum stations that are allowed to be connected to AP
	 *  (255: no limit)
	 */
	u8 max_stations_thresh;
	/* Timeout till enabling the Sleep Mechanism after data stops
	 * [unit: 100 msec]
	 */
	u8 idle_conn_thresh;
} __packed;

struct wifi_ht_settings {
	/* DEFAULT / WIDE / SISO20 */
	u8 mode;
} __packed;

struct wifi_mac_and_phy_params {
	u8 phy_standalone;
	u8 spare0;
	u8 enable_clpc;
	u8 enable_tx_low_pwr_on_siso_rdl;
	u8 auto_detect;
	u8 dedicated_fem;

	u8 low_band_component;

	/* Bit 0: One Hot, Bit 1: Control Enable, Bit 2: 1.8V, Bit 3: 3V */
	u8 low_band_component_type;

	u8 high_band_component;

	/* Bit 0: One Hot, Bit 1: Control Enable, Bit 2: 1.8V, Bit 3: 3V */
	u8 high_band_component_type;
	u8 number_of_assembled_ant2_4;
	u8 number_of_assembled_ant5;
	u8 pin_muxing_platform_options[PIN_MUXING_SIZE];
	u8 external_pa_dc2dc;
	u8 tcxo_ldo_voltage;
	u8 xtal_itrim_val;
	u8 srf_state;
	u8 srf1[SRF_TABLE_LEN];
	u8 srf2[SRF_TABLE_LEN];
	u8 srf3[SRF_TABLE_LEN];
	u8 io_configuration;
	u8 sdio_configuration;
	u8 settings;
	u8 rx_profile;
	u8 per_chan_pwr_limit_arr_11abg[NUM_OF_CHANNELS_11_ABG];
	u8 pwr_limit_reference_11_abg;
	u8 per_chan_pwr_limit_arr_11p[NUM_OF_CHANNELS_11_P];
	u8 pwr_limit_reference_11p;
	u8 spare1;
	u8 per_chan_bo_mode_11_abg[13];
	u8 per_chan_bo_mode_11_p[4];
	u8 primary_clock_setting_time;
	u8 clock_valid_on_wake_up;
	u8 secondary_clock_setting_time;
	u8 board_type;
	/* enable point saturation */
	u8 psat;
	/* low/medium/high Tx power in dBm for STA-HP BG */
	s8 low_power_val;
	s8 med_power_val;
	s8 high_power_val;
	s8 per_sub_band_tx_trace_loss[WL18XX_TRACE_LOSS_GAPS_TX];
	s8 per_sub_band_rx_trace_loss[WL18XX_TRACE_LOSS_GAPS_RX];
	u8 tx_rf_margin;
	/* low/medium/high Tx power in dBm for other role */
	s8 low_power_val_2nd;
	s8 med_power_val_2nd;
	s8 high_power_val_2nd;

	u8 padding[1];
} __packed;

enum {
	SCAN_COMPLETE_EVENT_ID                   = BIT(8),
	RADAR_DETECTED_EVENT_ID                  = BIT(9),
	CHANNEL_SWITCH_COMPLETE_EVENT_ID         = BIT(10),
	BSS_LOSS_EVENT_ID                        = BIT(11),
	MAX_TX_FAILURE_EVENT_ID                  = BIT(12),
	DUMMY_PACKET_EVENT_ID                    = BIT(13),
	INACTIVE_STA_EVENT_ID                    = BIT(14),
	PEER_REMOVE_COMPLETE_EVENT_ID            = BIT(15),
	PERIODIC_SCAN_COMPLETE_EVENT_ID          = BIT(16),
	BA_SESSION_RX_CONSTRAINT_EVENT_ID        = BIT(17),
	REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID      = BIT(18),
	DFS_CHANNELS_CONFIG_COMPLETE_EVENT       = BIT(19),
	PERIODIC_SCAN_REPORT_EVENT_ID            = BIT(20),
	RX_BA_WIN_SIZE_CHANGE_EVENT_ID           = BIT(21),
	SMART_CONFIG_SYNC_EVENT_ID               = BIT(22),
	SMART_CONFIG_DECODE_EVENT_ID             = BIT(23),
	TIME_SYNC_EVENT_ID                       = BIT(24),
	FW_LOGGER_INDICATION			= BIT(25),
};
int wifi_wait_for_event(enum wificore_wait_event event,
			  bool *timeout);

              #define WL18XX_CHIP_VER		8
#define WL18XX_IFTYPE_VER	9
#define WL18XX_MAJOR_VER	WLCORE_FW_VER_IGNORE
#define WL18XX_SUBTYPE_VER	WLCORE_FW_VER_IGNORE
#define WL18XX_MINOR_VER	58
#define WL18XX_RX_BA_MAX_SESSIONS 13
#define WL18XX_FW_NAME "ti-connectivity/wl18xx-fw-4.bin"
int wifi_identify_chip(void);
#if (PRINT_DEBUG_DATA_FRAME)
void WIFI_Print_Hex(u8 *data, u16 len, u8 *name);
#endif
#endif