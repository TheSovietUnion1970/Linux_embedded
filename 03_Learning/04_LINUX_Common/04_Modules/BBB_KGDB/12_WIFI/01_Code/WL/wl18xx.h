/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wl18xx
 *
 * Copyright (C) 2011 Texas Instruments Inc.
 */

#ifndef __WL18XX_PRIV_H__
#define __WL18XX_PRIV_H__

// #include "conf.h"

/* minimum FW required for driver */
#define WL18XX_CHIP_VER		8
#define WL18XX_IFTYPE_VER	9
#define WL18XX_MAJOR_VER	WLCORE_FW_VER_IGNORE
#define WL18XX_SUBTYPE_VER	WLCORE_FW_VER_IGNORE
#define WL18XX_MINOR_VER	58

#define WL18XX_CMD_MAX_SIZE          740

#define WL18XX_AGGR_BUFFER_SIZE		(13 * PAGE_SIZE)

#define WL18XX_NUM_TX_DESCRIPTORS 32
#define WL18XX_NUM_RX_DESCRIPTORS 32

#define WL18XX_NUM_MAC_ADDRESSES 2

#define WL18XX_RX_BA_MAX_SESSIONS 13

#define WL18XX_MAX_AP_STATIONS 10
#define WL18XX_MAX_LINKS 16

/* === conf.h === */
#define WL18XX_CONF_MAGIC	0x10e100ca
#define WL18XX_CONF_VERSION	(WLCORE_CONF_VERSION | 0x0007)
#define WL18XX_CONF_MASK	0x0000ffff
#define WL18XX_CONF_SIZE	(WLCORE_CONF_SIZE + \
				 sizeof(struct wifi_priv_conf))

#define NUM_OF_CHANNELS_11_ABG 150
#define NUM_OF_CHANNELS_11_P 7
#define SRF_TABLE_LEN 16
#define PIN_MUXING_SIZE 2
#define WL18XX_TRACE_LOSS_GAPS_TX 10
#define WL18XX_TRACE_LOSS_GAPS_RX 18

struct wl18xx_mac_and_phy_params {
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

enum wl18xx_ht_mode {
	/* Default - use MIMO, fallback to SISO20 */
	HT_MODE_DEFAULT = 0,

	/* Wide - use SISO40 */
	HT_MODE_WIDE = 1,

	/* Use SISO20 */
	HT_MODE_SISO20 = 2,
};

struct wl18xx_ht_settings {
	/* DEFAULT / WIDE / SISO20 */
	u8 mode;
} __packed;

struct conf_ap_sleep_settings {
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


struct wifi_priv_conf {
	/* Module params structures */
	struct wl18xx_ht_settings ht;

	/* this structure is copied wholesale to FW */
	struct wl18xx_mac_and_phy_params phy;

	struct conf_ap_sleep_settings ap_sleep;
} __packed;

enum wl18xx_sg_params {
	WL18XX_CONF_SG_PARAM_0 = 0,

	/* Configuration Parameters */
	WL18XX_CONF_SG_ANTENNA_CONFIGURATION,
	WL18XX_CONF_SG_ZIGBEE_COEX,
	WL18XX_CONF_SG_TIME_SYNC,

	WL18XX_CONF_SG_PARAM_4,
	WL18XX_CONF_SG_PARAM_5,
	WL18XX_CONF_SG_PARAM_6,
	WL18XX_CONF_SG_PARAM_7,
	WL18XX_CONF_SG_PARAM_8,
	WL18XX_CONF_SG_PARAM_9,
	WL18XX_CONF_SG_PARAM_10,
	WL18XX_CONF_SG_PARAM_11,
	WL18XX_CONF_SG_PARAM_12,
	WL18XX_CONF_SG_PARAM_13,
	WL18XX_CONF_SG_PARAM_14,
	WL18XX_CONF_SG_PARAM_15,
	WL18XX_CONF_SG_PARAM_16,
	WL18XX_CONF_SG_PARAM_17,
	WL18XX_CONF_SG_PARAM_18,
	WL18XX_CONF_SG_PARAM_19,
	WL18XX_CONF_SG_PARAM_20,
	WL18XX_CONF_SG_PARAM_21,
	WL18XX_CONF_SG_PARAM_22,
	WL18XX_CONF_SG_PARAM_23,
	WL18XX_CONF_SG_PARAM_24,
	WL18XX_CONF_SG_PARAM_25,

	/* Active Scan Parameters */
	WL18XX_CONF_SG_AUTO_SCAN_PROBE_REQ,
	WL18XX_CONF_SG_ACTIVE_SCAN_DURATION_FACTOR_HV3,

	WL18XX_CONF_SG_PARAM_28,

	/* Passive Scan Parameters */
	WL18XX_CONF_SG_PARAM_29,
	WL18XX_CONF_SG_PARAM_30,
	WL18XX_CONF_SG_PASSIVE_SCAN_DURATION_FACTOR_HV3,

	/* Passive Scan in Dual Antenna Parameters */
	WL18XX_CONF_SG_CONSECUTIVE_HV3_IN_PASSIVE_SCAN,
	WL18XX_CONF_SG_BEACON_HV3_COLL_TH_IN_PASSIVE_SCAN,
	WL18XX_CONF_SG_TX_RX_PROTECT_BW_IN_PASSIVE_SCAN,

	/* General Parameters */
	WL18XX_CONF_SG_STA_FORCE_PS_IN_BT_SCO,
	WL18XX_CONF_SG_PARAM_36,
	WL18XX_CONF_SG_BEACON_MISS_PERCENT,
	WL18XX_CONF_SG_PARAM_38,
	WL18XX_CONF_SG_RXT,
	WL18XX_CONF_SG_UNUSED,
	WL18XX_CONF_SG_ADAPTIVE_RXT_TXT,
	WL18XX_CONF_SG_GENERAL_USAGE_BIT_MAP,
	WL18XX_CONF_SG_HV3_MAX_SERVED,
	WL18XX_CONF_SG_PARAM_44,
	WL18XX_CONF_SG_PARAM_45,
	WL18XX_CONF_SG_CONSECUTIVE_CTS_THRESHOLD,
	WL18XX_CONF_SG_GEMINI_PARAM_47,
	WL18XX_CONF_SG_STA_CONNECTION_PROTECTION_TIME,

	/* AP Parameters */
	WL18XX_CONF_SG_AP_BEACON_MISS_TX,
	WL18XX_CONF_SG_PARAM_50,
	WL18XX_CONF_SG_AP_BEACON_WINDOW_INTERVAL,
	WL18XX_CONF_SG_AP_CONNECTION_PROTECTION_TIME,
	WL18XX_CONF_SG_PARAM_53,
	WL18XX_CONF_SG_PARAM_54,

	/* CTS Diluting Parameters */
	WL18XX_CONF_SG_CTS_DILUTED_BAD_RX_PACKETS_TH,
	WL18XX_CONF_SG_CTS_CHOP_IN_DUAL_ANT_SCO_MASTER,

	WL18XX_CONF_SG_TEMP_PARAM_1,
	WL18XX_CONF_SG_TEMP_PARAM_2,
	WL18XX_CONF_SG_TEMP_PARAM_3,
	WL18XX_CONF_SG_TEMP_PARAM_4,
	WL18XX_CONF_SG_TEMP_PARAM_5,
	WL18XX_CONF_SG_TEMP_PARAM_6,
	WL18XX_CONF_SG_TEMP_PARAM_7,
	WL18XX_CONF_SG_TEMP_PARAM_8,
	WL18XX_CONF_SG_TEMP_PARAM_9,
	WL18XX_CONF_SG_TEMP_PARAM_10,

	WL18XX_CONF_SG_PARAMS_MAX,
	WL18XX_CONF_SG_PARAMS_ALL = 0xff
};


struct wifi_priv {
	struct wifi_priv_conf conf;
};

const char *wifi_rx_rate_to_string(u8 rate);

#define WL18XX_FW_MAX_TX_STATUS_DESC 33

struct wl18xx_fw_status_priv {
	/*
	 * Index in released_tx_desc for first byte that holds
	 * released tx host desc
	 */
	u8 fw_release_idx;

	/*
	 * Array of host Tx descriptors, where fw_release_idx
	 * indicated the first released idx.
	 */
	u8 released_tx_desc[WL18XX_FW_MAX_TX_STATUS_DESC];

	/* A bitmap representing the currently suspended links. The suspend
	 * is short lived, for multi-channel Tx requirements.
	 */
	__le32 link_suspend_bitmap;

	/* packet threshold for an "almost empty" AC,
	 * for Tx schedulng purposes
	 */
	u8 tx_ac_threshold;

	/* number of packets to queue up for a link in PS */
	u8 tx_ps_threshold;

	/* number of packet to queue up for a suspended link */
	u8 tx_suspend_threshold;

	/* Should have less than this number of packets in queue of a slow
	 * link to qualify as high priority link
	 */
	u8 tx_slow_link_prio_threshold;

	/* Should have less than this number of packets in queue of a fast
	 * link to qualify as high priority link
	 */
	u8 tx_fast_link_prio_threshold;

	/* Should have less than this number of packets in queue of a slow
	 * link before we stop queuing up packets for it.
	 */
	u8 tx_slow_stop_threshold;

	/* Should have less than this number of packets in queue of a fast
	 * link before we stop queuing up packets for it.
	 */
	u8 tx_fast_stop_threshold;

	u8 padding[3];
};

struct wl18xx_fw_packet_counters {
	/* Cumulative counter of released packets per AC */
	u8 tx_released_pkts[NUM_TX_QUEUES];

	/* Cumulative counter of freed packets per HLID */
	u8 tx_lnk_free_pkts[WL18XX_MAX_LINKS];

	/* Cumulative counter of released Voice memory blocks */
	u8 tx_voice_released_blks;

	/* Tx rate of the last transmitted packet */
	u8 tx_last_rate;

	/* Tx rate or Tx rate estimate pre-calculated by fw in mbps units */
	u8 tx_last_rate_mbps;

	/* hlid for which the rates were reported */
	u8 hlid;
} __packed;

/* FW status registers */
struct wl18xx_fw_status {
	__le32 intr;
	u8  fw_rx_counter;
	u8  drv_rx_counter;
	u8  reserved;
	u8  tx_results_counter;
	__le32 rx_pkt_descs[WL18XX_NUM_RX_DESCRIPTORS];

	__le32 fw_localtime;

	/*
	 * A bitmap (where each bit represents a single HLID)
	 * to indicate if the station is in PS mode.
	 */
	__le32 link_ps_bitmap;

	/*
	 * A bitmap (where each bit represents a single HLID) to indicate
	 * if the station is in Fast mode
	 */
	__le32 link_fast_bitmap;

	/* Cumulative counter of total released mem blocks since FW-reset */
	__le32 total_released_blks;

	/* Size (in Memory Blocks) of TX pool */
	__le32 tx_total;

	struct wl18xx_fw_packet_counters counters;

	__le32 log_start_addr;

	/* Private status to be used by the lower drivers */
	struct wl18xx_fw_status_priv priv;
} __packed;

#define WL18XX_PHY_VERSION_MAX_LEN 20

struct wl18xx_static_data_priv {
	char phy_version[WL18XX_PHY_VERSION_MAX_LEN];
};

struct wl18xx_clk_cfg {
	u32 n;
	u32 m;
	u32 p;
	u32 q;
	bool swallow;
};

enum {
	CLOCK_CONFIG_16_2_M	= 1,
	CLOCK_CONFIG_16_368_M,
	CLOCK_CONFIG_16_8_M,
	CLOCK_CONFIG_19_2_M,
	CLOCK_CONFIG_26_M,
	CLOCK_CONFIG_32_736_M,
	CLOCK_CONFIG_33_6_M,
	CLOCK_CONFIG_38_468_M,
	CLOCK_CONFIG_52_M,

	NUM_CLOCK_CONFIGS,
};


/* === event.h === */
// #include "../wlcore/wlcore.h"

enum wl18xx_radar_types {
	RADAR_TYPE_NONE,
	RADAR_TYPE_REGULAR,
	RADAR_TYPE_CHIRP
};

struct wl18xx_event_mailbox {
	__le32 events_vector;

	u8 number_of_scan_results;
	u8 number_of_sched_scan_results;

	__le16 channel_switch_role_id_bitmap;

	s8 rssi_snr_trigger_metric[NUM_OF_RSSI_SNR_TRIGGERS];

	/* bitmap of removed links */
	__le32 hlid_removed_bitmap;

	/* rx ba constraint */
	__le16 rx_ba_role_id_bitmap; /* 0xfff means any role. */
	__le16 rx_ba_allowed_bitmap;

	/* bitmap of roc completed (by role id) */
	__le16 roc_completed_bitmap;

	/* bitmap of stations (by role id) with bss loss */
	__le16 bss_loss_bitmap;

	/* bitmap of stations (by HLID) which exceeded max tx retries */
	__le16 tx_retry_exceeded_bitmap;

	/* time sync high msb*/
	__le16 time_sync_tsf_high_msb;

	/* bitmap of inactive stations (by HLID) */
	__le16 inactive_sta_bitmap;

	/* time sync high lsb*/
	__le16 time_sync_tsf_high_lsb;

	/* rx BA win size indicated by RX_BA_WIN_SIZE_CHANGE_EVENT_ID */
	u8 rx_ba_role_id;
	u8 rx_ba_link_id;
	u8 rx_ba_win_size;
	u8 padding;

	/* smart config */
	u8 sc_ssid_len;
	u8 sc_pwd_len;
	u8 sc_token_len;
	u8 padding1;
	u8 sc_ssid[32];
	u8 sc_pwd[64];
	u8 sc_token[32];

	/* smart config sync channel */
	u8 sc_sync_channel;
	u8 sc_sync_band;

	/* time sync low msb*/
	__le16 time_sync_tsf_low_msb;

	/* radar detect */
	u8 radar_channel;
	u8 radar_type;

	/* time sync low lsb*/
	__le16 time_sync_tsf_low_lsb;

} __packed;

/* === acx.h === */
// enum {
// 	ACX_NS_IPV6_FILTER		 = 0x0050,
// 	ACX_PEER_HT_OPERATION_MODE_CFG	 = 0x0051,
// 	ACX_CSUM_CONFIG			 = 0x0052,
// 	ACX_SIM_CONFIG			 = 0x0053,
// 	ACX_CLEAR_STATISTICS		 = 0x0054,
// 	ACX_AUTO_RX_STREAMING		 = 0x0055,
// 	ACX_PEER_CAP			 = 0x0056,
// 	ACX_INTERRUPT_NOTIFY		 = 0x0057,
// 	ACX_RX_BA_FILTER		 = 0x0058,
// 	ACX_AP_SLEEP_CFG                 = 0x0059,
// 	ACX_DYNAMIC_TRACES_CFG		 = 0x005A,
// 	ACX_TIME_SYNC_CFG		 = 0x005B,
// };

/* numbers of bits the length field takes (add 1 for the actual number) */
#define WL18XX_HOST_IF_LEN_SIZE_FIELD 15

#define WL18XX_ACX_EVENTS_VECTOR	(WL1271_ACX_INTR_WATCHDOG	| \
					 WL1271_ACX_INTR_INIT_COMPLETE	| \
					 WL1271_ACX_INTR_EVENT_A	| \
					 WL1271_ACX_INTR_EVENT_B	| \
					 WL1271_ACX_INTR_CMD_COMPLETE	| \
					 WL1271_ACX_INTR_HW_AVAILABLE	| \
					 WL1271_ACX_INTR_DATA		| \
					 WL1271_ACX_SW_INTR_WATCHDOG)

#define WL18XX_INTR_MASK		(WL1271_ACX_INTR_WATCHDOG	| \
					 WL1271_ACX_INTR_EVENT_A	| \
					 WL1271_ACX_INTR_EVENT_B	| \
					 WL1271_ACX_INTR_HW_AVAILABLE	| \
					 WL1271_ACX_INTR_DATA		| \
					 WL1271_ACX_SW_INTR_WATCHDOG)

struct wl18xx_acx_host_config_bitmap {
	struct acx_header header;

	__le32 host_cfg_bitmap;

	__le32 host_sdio_block_size;

	/* extra mem blocks per frame in TX. */
	__le32 extra_mem_blocks;

	/*
	 * number of bits of the length field in the first TX word
	 * (up to 15 - for using the entire 16 bits).
	 */
	__le32 length_field_size;

} __packed;

enum {
	CHECKSUM_OFFLOAD_DISABLED = 0,
	CHECKSUM_OFFLOAD_ENABLED  = 1,
	CHECKSUM_OFFLOAD_FAKE_RX  = 2,
	CHECKSUM_OFFLOAD_INVALID  = 0xFF
};

struct wl18xx_acx_checksum_state {
	struct acx_header header;

	 /* enum acx_checksum_state */
	u8 checksum_state;
	u8 pad[3];
} __packed;


struct wl18xx_acx_error_stats {
	u32 error_frame_non_ctrl;
	u32 error_frame_ctrl;
	u32 error_frame_during_protection;
	u32 null_frame_tx_start;
	u32 null_frame_cts_start;
	u32 bar_retry;
	u32 num_frame_cts_nul_flid;
	u32 tx_abort_failure;
	u32 tx_resume_failure;
	u32 rx_cmplt_db_overflow_cnt;
	u32 elp_while_rx_exch;
	u32 elp_while_tx_exch;
	u32 elp_while_tx;
	u32 elp_while_nvic_pending;
	u32 rx_excessive_frame_len;
	u32 burst_mismatch;
	u32 tbc_exch_mismatch;
} __packed;

#define NUM_OF_RATES_INDEXES 30
struct wl18xx_acx_tx_stats {
	u32 tx_prepared_descs;
	u32 tx_cmplt;
	u32 tx_template_prepared;
	u32 tx_data_prepared;
	u32 tx_template_programmed;
	u32 tx_data_programmed;
	u32 tx_burst_programmed;
	u32 tx_starts;
	u32 tx_stop;
	u32 tx_start_templates;
	u32 tx_start_int_templates;
	u32 tx_start_fw_gen;
	u32 tx_start_data;
	u32 tx_start_null_frame;
	u32 tx_exch;
	u32 tx_retry_template;
	u32 tx_retry_data;
	u32 tx_retry_per_rate[NUM_OF_RATES_INDEXES];
	u32 tx_exch_pending;
	u32 tx_exch_expiry;
	u32 tx_done_template;
	u32 tx_done_data;
	u32 tx_done_int_template;
	u32 tx_cfe1;
	u32 tx_cfe2;
	u32 frag_called;
	u32 frag_mpdu_alloc_failed;
	u32 frag_init_called;
	u32 frag_in_process_called;
	u32 frag_tkip_called;
	u32 frag_key_not_found;
	u32 frag_need_fragmentation;
	u32 frag_bad_mblk_num;
	u32 frag_failed;
	u32 frag_cache_hit;
	u32 frag_cache_miss;
} __packed;

struct wl18xx_acx_rx_stats {
	u32 rx_beacon_early_term;
	u32 rx_out_of_mpdu_nodes;
	u32 rx_hdr_overflow;
	u32 rx_dropped_frame;
	u32 rx_done_stage;
	u32 rx_done;
	u32 rx_defrag;
	u32 rx_defrag_end;
	u32 rx_cmplt;
	u32 rx_pre_complt;
	u32 rx_cmplt_task;
	u32 rx_phy_hdr;
	u32 rx_timeout;
	u32 rx_rts_timeout;
	u32 rx_timeout_wa;
	u32 defrag_called;
	u32 defrag_init_called;
	u32 defrag_in_process_called;
	u32 defrag_tkip_called;
	u32 defrag_need_defrag;
	u32 defrag_decrypt_failed;
	u32 decrypt_key_not_found;
	u32 defrag_need_decrypt;
	u32 rx_tkip_replays;
	u32 rx_xfr;
} __packed;

struct wl18xx_acx_isr_stats {
	u32 irqs;
} __packed;

#define PWR_STAT_MAX_CONT_MISSED_BCNS_SPREAD 10

struct wl18xx_acx_pwr_stats {
	u32 missing_bcns_cnt;
	u32 rcvd_bcns_cnt;
	u32 connection_out_of_sync;
	u32 cont_miss_bcns_spread[PWR_STAT_MAX_CONT_MISSED_BCNS_SPREAD];
	u32 rcvd_awake_bcns_cnt;
	u32 sleep_time_count;
	u32 sleep_time_avg;
	u32 sleep_cycle_avg;
	u32 sleep_percent;
	u32 ap_sleep_active_conf;
	u32 ap_sleep_user_conf;
	u32 ap_sleep_counter;
} __packed;

struct wl18xx_acx_rx_filter_stats {
	u32 beacon_filter;
	u32 arp_filter;
	u32 mc_filter;
	u32 dup_filter;
	u32 data_filter;
	u32 ibss_filter;
	u32 protection_filter;
	u32 accum_arp_pend_requests;
	u32 max_arp_queue_dep;
} __packed;

struct wl18xx_acx_rx_rate_stats {
	u32 rx_frames_per_rates[50];
} __packed;

#define AGGR_STATS_TX_AGG	16
#define AGGR_STATS_RX_SIZE_LEN	16

struct wl18xx_acx_aggr_stats {
	u32 tx_agg_rate[AGGR_STATS_TX_AGG];
	u32 tx_agg_len[AGGR_STATS_TX_AGG];
	u32 rx_size[AGGR_STATS_RX_SIZE_LEN];
} __packed;

#define PIPE_STATS_HW_FIFO	11

struct wl18xx_acx_pipeline_stats {
	u32 hs_tx_stat_fifo_int;
	u32 hs_rx_stat_fifo_int;
	u32 enc_tx_stat_fifo_int;
	u32 enc_rx_stat_fifo_int;
	u32 rx_complete_stat_fifo_int;
	u32 pre_proc_swi;
	u32 post_proc_swi;
	u32 sec_frag_swi;
	u32 pre_to_defrag_swi;
	u32 defrag_to_rx_xfer_swi;
	u32 dec_packet_in;
	u32 dec_packet_in_fifo_full;
	u32 dec_packet_out;
	u16 pipeline_fifo_full[PIPE_STATS_HW_FIFO];
	u16 padding;
} __packed;

#define DIVERSITY_STATS_NUM_OF_ANT	2

struct wl18xx_acx_diversity_stats {
	u32 num_of_packets_per_ant[DIVERSITY_STATS_NUM_OF_ANT];
	u32 total_num_of_toggles;
} __packed;

struct wl18xx_acx_thermal_stats {
	u16 irq_thr_low;
	u16 irq_thr_high;
	u16 tx_stop;
	u16 tx_resume;
	u16 false_irq;
	u16 adc_source_unexpected;
} __packed;

#define WL18XX_NUM_OF_CALIBRATIONS_ERRORS 18
struct wl18xx_acx_calib_failure_stats {
	u16 fail_count[WL18XX_NUM_OF_CALIBRATIONS_ERRORS];
	u32 calib_count;
} __packed;

struct wl18xx_roaming_stats {
	s32 rssi_level;
} __packed;

struct wl18xx_dfs_stats {
	u32 num_of_radar_detections;
} __packed;

struct wl18xx_acx_statistics {
	struct acx_header header;

	struct wl18xx_acx_error_stats		error;
	struct wl18xx_acx_tx_stats		tx;
	struct wl18xx_acx_rx_stats		rx;
	struct wl18xx_acx_isr_stats		isr;
	struct wl18xx_acx_pwr_stats		pwr;
	struct wl18xx_acx_rx_filter_stats	rx_filter;
	struct wl18xx_acx_rx_rate_stats		rx_rate;
	struct wl18xx_acx_aggr_stats		aggr_size;
	struct wl18xx_acx_pipeline_stats	pipeline;
	struct wl18xx_acx_diversity_stats	diversity;
	struct wl18xx_acx_thermal_stats		thermal;
	struct wl18xx_acx_calib_failure_stats	calib;
	struct wl18xx_roaming_stats		roaming;
	struct wl18xx_dfs_stats			dfs;
} __packed;

enum wificore_bandwidth {
	WLCORE_BANDWIDTH_20MHZ,
	WLCORE_BANDWIDTH_40MHZ,
};

struct wificore_peer_ht_operation_mode {
	struct acx_header header;

	u8 hlid;
	u8 bandwidth; /* enum wificore_bandwidth */
	u8 padding[2];
};

/*
 * ACX_PEER_CAP
 * this struct is very similar to wifi_acx_ht_capabilities, with the
 * addition of supported rates
 */
struct wificore_acx_peer_cap {
	struct acx_header header;

	/* bitmask of capability bits supported by the peer */
	__le32 ht_capabilites;

	/* rates supported by the remote peer */
	__le32 supported_rates;

	/* Indicates to which link these capabilities apply. */
	u8 hlid;

	/*
	 * This the maximum A-MPDU length supported by the AP. The FW may not
	 * exceed this length when sending A-MPDUs
	 */
	u8 ampdu_max_length;

	/* This is the minimal spacing required when sending A-MPDUs to the AP*/
	u8 ampdu_min_spacing;

	u8 padding;
} __packed;

/*
 * ACX_INTERRUPT_NOTIFY
 * enable/disable fast-link/PSM notification from FW
 */
struct wl18xx_acx_interrupt_notify {
	struct acx_header header;
	u32 enable;
};


struct acx_ap_sleep_cfg {
	struct acx_header header;
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

/*
 * ACX_DYNAMIC_TRACES_CFG
 * configure the FW dynamic traces
 */
struct acx_dynamic_fw_traces_cfg {
	struct acx_header header;
	__le32 dynamic_fw_traces;
} __packed;


#endif /* __WL18XX_PRIV_H__ */
