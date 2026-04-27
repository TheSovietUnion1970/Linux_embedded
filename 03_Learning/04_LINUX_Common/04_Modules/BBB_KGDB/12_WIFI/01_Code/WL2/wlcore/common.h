#ifndef COMMON_H
#define COMMON_H

#define STA_BASIC_RATE_IDX          0
#define STA_AP_RATE_IDX             1
#define STA_P2P_RATE_IDX            2

#define STA_KLV_TEMPLATE_IDX        0

#define STA_ROLE_ID 1
#define P2P_ROLE_ID 2

// Each HW Link ID is assigned to each connection (STA or AP)
#define HW_LINK_ID 1 // (< WL18XX_MAX_LINKS 16)
        // must be non-zero as WL12XX_SYSTEM_HLID = 0 is existed

// Per-if has 4 queues (VO, VI, BE and BK)
#define Q_BASE 0
#define HW_QUEUE_BASE  Q_BASE*4 // (0*NUM_TX_QUEUES)

extern struct sk_buff_head VV_tx_queue[WLCORE_MAX_LINKS][NUM_TX_QUEUES];
extern int VV_tx_queue_count[NUM_TX_QUEUES]; /* Frames scheduled for transmission, not handled yet */ 

extern u8 VV_allocated_pkts[WLCORE_MAX_LINKS];

/* Accounting for allocated / available Tx packets in HW */
extern u32 VV_tx_pkts_freed[NUM_TX_QUEUES];
extern u32 VV_tx_allocated_pkts[NUM_TX_QUEUES];
extern u32 VV_tx_allocated_blocks; // new - last (released blks)
        // incremented - allocate hw
        // decremented - TX interupt
extern u32 VV_tx_blocks_available; // get from old val or updated (tx total - VV_tx_allocated_blocks)
                                   // = the available slot where blcks can be allocated
        // incremented - max (old, tx total - allocated)
        // decremented - allocate hw  
extern u32 VV_tx_packets_count; 
extern u8 VV_last_fw_rls_idx; // it's incremented every TX interrupt
        
extern struct sk_buff *VV_skb_tx_frames[WLCORE_MAX_TX_DESCRIPTORS];
        // ptr to skb per tx desc
extern int VV_skb_tx_frames_cnt;
extern u32 VV_last_updated_tmp_tx_blocks_freed;

#define WL18XX_NUM_RX_DESCRIPTORS 32
#define WL18XX_MAX_LINKS 16
#define WL18XX_FW_MAX_TX_STATUS_DESC 33
/* FW status registers */
struct VV_wl18xx_fw_status {
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

	//struct wl18xx_fw_packet_counters counters;
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
    // ====

	__le32 log_start_addr;

	/* Private status to be used by the lower drivers */
	//struct wl18xx_fw_status_priv priv;
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
} __packed;
extern struct VV_wl18xx_fw_status* VV_status_reg;

#define WLCORE_MAX_LINKS 16

struct wl12xx_vif;
struct VV_link {
	/* AP-mode - TX queue per AC in link */
	//struct sk_buff_head tx_queue[NUM_TX_QUEUES];

	/* accounting for allocated / freed packets in FW */
	//u8 allocated_pkts;
	u8 prev_freed_pkts;

	u8 addr[6];

	/* bitmap of TIDs where RX BA sessions are active for this link */
	u8 ba_bitmap;

	/* the last fw rate index we used for this link */
	u8 fw_rate_idx;

	/* the last fw rate [Mbps] we used for this link */
	u8 fw_rate_mbps;

	/* The wlvif this link belongs to. Might be null for global links */
	struct wl12xx_vif *wlvif;
	/*
	 * total freed FW packets on the link - used for tracking the
	 * AES/TKIP PN across recoveries. Re-initialized each time
	 * from the wl1271_station structure.
	 */
	u64 total_freed_pkts;
};
extern struct VV_link VV_links[WLCORE_MAX_LINKS];
#endif
//extern struct VV_wl18xx_fw_status VV_status_reg;

//              [----------DEFAULT-------]      [-------USER----]
// QUEUEs       0                               1                      2  ... 15         
// TYPEs        [VO     VI     BE     BK]       [VO,VI,BE,BK] ...
//               |       |      |     |
// REASONs     4-bit   4-bit   4-bit  4-bit

// queue_stop_reasons -> idx = TYPEs num
// tx_queue_count     -> usually for <USER>, [0],[1],[2],[3]

/* ========================================================================================= */
// Funcs

// wlcore_set_bssid:
    // wl1271_acx_sta_rate_policies(wl, wlvif) 
        // -> Set rate policies -> STA_BASIC_RATE_IDX, STA_AP_RATE_IDX, STA_P2P_RATE_IDX

    // wl12xx_cmd_build_null_data(wl, wlvif);
        //  CMD_TEMPL_NULL_DATA <- CMD_SET_TEMPLATE
        // -> generate a Null Data frame using ieee80211_nullfunc_get()
        // used when QoS is not enabled. 
        // -> Good power saving behavior -> not critical

    // wl1271_build_qos_null_data(wl, wl12xx_wlvif_to_vif(wlvif));
        // CMD_TEMPL_QOS_NULL_DATA <- CMD_SET_TEMPLATE
        // // -> Good power saving behavior -> not critical

    // wl1271_tx_enabled_rates_get(wl,
    //             sta_rate_set,
    //             wlvif->band);
        // raw:      sta_rate_set    -> 0xFF0FFF
        // firmware: wlvif->rate_set -> 0x1FFEFF
        // -> support speed: legacy + HT/MCS rates

	// // get wlvif->ssid_len and ssid string
	// wlcore_set_ssid(wl, wlvif); 

// wlcore_clear_bssid:
    // wl1271_acx_sta_rate_policies(wl, wlvif) 

    // wl12xx_cmd_role_stop_sta(wl, wlvif)
    // CMD_ROLE_STOP

// wl1271_acx_beacon_filter_opt(wl, wlvif, true);
// Most beacons → silently dropped by firmware.
// Only beacons that say “I have data for you” → passed to driver.
// Traffic Indication Map (TIM) = “I (the Access Point) have buffered data waiting for you.”

// wl1271_bss_erp_info_changed: 
// ERP (Extended Rate PHY) handling.
    // wl1271_acx_slot(wl, wlvif, SLOT_TIME_SHORT / SLOT_TIME_LONG);
        // ACX_SLOT <- CMD_CONFIGURE
        // very basic timing parameter in 802.11

    // wl1271_acx_set_preamble(wl, wlvif, ACX_PREAMBLE_SHORT / ACX_PREAMBLE_LONG);
        // ACX_PREAMBLE_TYPE <- CMD_CONFIGURE
        // the beginning part of every WiFi frame.

    // wl1271_acx_cts_protect(wl, wlvif, CTSPROTECT_ENABLE / CTSPROTECT_DISABLE);
        // ACX_CTS_PROTECTION <- CMD_CONFIGURE
        // before sending a data frame at high speed (802.11g), CLI -> CTS frame at low speed (802.11b rate)
          // -> all nearby 802.11b devices to stay quiet

// wlcore_join(wl, wlvif):
    // wl12xx_cmd_role_start_sta
    // CMD_ROLE_START -> enable STA role (beacon, ssid, bssid)

// wlcore_set_assoc:
    // wl1271_cmd_build_ps_poll(wl, wlvif, wlvif->aid):
    //-> PS-poll frame by CLI in pow-save mode -> AP: 'I'm awake'
        // ieee80211_pspoll_get(wl->hw, vif) -> standard PS-Poll frame

        // ret = wl1271_cmd_template_set(wl, wlvif->role_id,
        // 			      CMD_TEMPL_PS_POLL, skb->data,
        // 			      skb->len, 0, wlvif->basic_rate_set);
        // -> PS-Poll frame to the firmware - template with ID CMD_TEMPL_PS_POLL

    // Re-play the step of getting ssid plus wl1271_cmd_template_set - CMD_TEMPL_CFG_PROBE_REQ_2_4

    // wl1271_acx_conn_monit_params(wl, wlvif, true);
        // -> synch_fail_thold + bss_lose_timeout
    
    // wl1271_acx_keep_alive_mode(wl, wlvif, true);
        // In pwr save mode, MAKE fw send Null Data or QoS Null Data frames periodically

    // wl1271_acx_aid(wl, wlvif, wlvif->aid);
        // Send AID to fw

    // wl12xx_cmd_build_klv_null_data(wl, wlvif);
        // Building a special Null Data template dedicated to Keep-Alive.

    // wl1271_acx_keep_alive_config(wl, wlvif, STA_KLV_TEMPLATE_IDX, ACX_KEEP_ALIVE_TPL_VALID);
        // main configuration command that tells the firmware how to use Keep-Alive.

    // wl1271_ps_set_mode(wl, wlvif, STATION_ACTIVE_MODE);
        // STA into active mode (X - pwr-saving mode)

    // REPLAY - Set rate policies

// wlcore_unset_assoc:
    // wl1271_acx_conn_monit_params(wl, wlvif, false);
    // wl1271_acx_keep_alive_mode(wl, wlvif, false);
    // wl1271_acx_beacon_filter_opt(wl, wlvif, false);
	// wl1271_acx_keep_alive_config(wl, wlvif, STA_KLV_TEMPLATE_IDX, ACX_KEEP_ALIVE_TPL_INVALID);

// wl12xx_set_authorized:
    // wl12xx_cmd_set_peer_state(wl, wlvif, wlvif->sta.hlid);
        // -> This function tells the firmware that the peer (the Access Point) has moved to the CONNECTED state.
        // cmd->hlid = hlid: Hardware Link ID (usually 0 for STA)

// wlcore_hw_set_peer_cap(wl,&sta_ht_cap,enabled,wlvif->rate_set,wlvif->sta.hlid);
    // Send the AP’s HT capabilities to the firmware

// wl1271_acx_set_ht_information(wl, wlvif, bss_conf->ht_operation_mode);
    // If HT is enabled, send additional HT Operation Mode information.

// wl1271_cmd_build_arp_rsp(wl, wlvif);
// wl1271_acx_arp_ip_filter(wl, wlvif,(ACX_ARP_FILTER_ARP_FILTERING |ACX_ARP_FILTER_AUTO_ARP),addr);
    // -> Its main job is to configure the firmware to automatically reply to ARP requests from the Access Point (or network) without waking up the host CPU every time.
    // major power-saving feature.

// wlcore_hw_set_cac(wl, wlvif, true); // wl18xx_cmd_set_cac -> Channel Availability Check
    // When connecting to a 5 GHz AP

// wl1271_acx_group_address_tbl(wl, wlvif, fp->enabled,fp->mc_list,fp->mc_list_length);
    // If FIF_ALLMULTI is set → Disable filtering (accept all multicast).
    // Otherwise → Send the list of allowed multicast addresses (mc_list) to the firmware using wl1271_acx_group_address_tbl().

// wlcore_set_scan_chan_params(wl, cmd_channels, req->channels,
// 			    req->n_channels, req->n_ssids,
// 			    SCAN_TYPE_SEARCH);
	// return  cfg->passive[0] || cfg->active[0] ||         -> 2GHz
	// 	cfg->passive[1] || cfg->active[1] || cfg->dfs ||    -> 5GHz
	// 	cfg->passive[2] || cfg->active[2];                  -> not supported

// ret = wl12xx_cmd_build_probe_req(wl, wlvif,
//             cmd->role_id, band,
//             req->ssids ? req->ssids[0].ssid : NULL,
//             req->ssids ? req->ssids[0].ssid_len : 0,
//             req->ie,
//             req->ie_len,
//             NULL,
//             0,
//             false);
    // This function creates a Probe Request frame (a packet that says “Is anyone there?”) and uploads it to the firmware as a template.

// wlcore_scan:
    // delayed work: scan_complete_work
    
    // wl->ops->scan_start(wl, wlvif, req) = wl18xx_scan_send
        // -> cpy cmd_channels->active|passive|dtfs -> cmd
        // -> wl12xx_cmd_build_probe_req - 2 + 5 GHz
        // <- CMD_SCAN

// wl1271_set_key:
    // ret = wl1271_cmd_set_sta_key(wl, wlvif, action,
    // 			     id, key_type, key_size,
    // 			     key, addr, tx_seq_32,
    // 			     tx_seq_16);
        // CMD_SET_KEYS <-

// 	ret = wl1271_tx_allocate(wl, wlvif, skb, extra, buf_offset, hlid,is_gem);
    // VV_tx_blocks_available -= total_blocks;
    // VV_tx_allocated_blocks += total_blocks;
    // Adds the TX descriptor at the front of the skb (sizeof(struct wl1271_tx_hw_descr) + extra;)

// wl1271_tx_fill_hdr(wl, wlvif, skb, extra, info, hlid);
    // Fill in wl1271_tx_hw_descr *desc:

// ret = wl1271_prepare_tx_frame(wl, wlvif, skb, buf_offset, hlid);
    // wl1271_tx_allocate + wl1271_tx_fill_hdr
    // update wl->aggr_buf

// wlcore_tx_work_locked:
    // = wl1271_prepare_tx_frame
    // write wl->aggr_buf -> REG_SLV_MEM_DATA = the address in the firmware’s memory where TX data should be written.

// wl->tx_work = wl1271_tx_work = wlcore_tx_work_locked

// wl1271_op_tx -> called whenver there is a packet about to transmit.
    // Decide drop or transmit

// wl12xx_tx_reset -> ieee80211_tx_status_ni = pass TX status to stack

// wlcore_op_stop = wlcore_op_stop_locked:
    // disable_irq_nosync + synchronize_irq
    // wl1271_flush_deferred_work -> flush RX + TX status to stack
    // wl12xx_tx_reset + wl1271_power_off

// wl->recovery_work = wl1271_recovery_work
    // disable_irq_nosync + ieee80211_stop_queues
    // __wl1271_op_remove_interface
    // wlcore_op_stop_locked = wlcore_op_stop
    // ieee80211_restart_hw + ieee80211_wake_queues

// wl1271_op_remove_interface = __wl1271_op_remove_interface:
    // wl12xx_cmd_role_disable

// wl12xx_chip_wakeup:
    // wl12xx_set_power_on
    // wl1271_sdio_set_block_size
    // wl1271_setup
        // alloc(wl->fw_status);
        // alloc(wl->raw_fw_status);
        // alloc(wl->tx_res_if);
    // wl12xx_fetch_firmware:
        // wl->fw_type
        // wl->fw_len 
        // wl->fw 

// wl18xx_set_clk:
    // configures the internal clock/PLL system of the wl18xx WiFi chip.

// wl18xx_pre_boot:
    // wl18xx_set_clk
    // ELP wake up sequence
    // => [PART_BOOT]
    // Disable interrupts
    // disable Rx/Tx + auto calibration on start

// wl18xx_pre_upload: it prepares the hardware before the actual firmware binary is loaded into the chip.
    // => [PART_BOOT]
    // => [PART_PHY_INIT]

// ->boot = wl18xx_boot:
    // wl18xx_pre_boot
    // wl18xx_pre_upload
    // wlcore_boot_upload_firmware
    // wl18xx_set_mac_and_phy
    // wlcore_boot_run_firmware
    // wl18xx_enable_interrupts

// wl1271_hw_init:
    // ->hw_init = wl18xx_hw_init
        // wl18xx_set_host_cfg_bitmap -> set the default amount of spare blocks in the bitmap
        // wl18xx_acx_dynamic_fw_traces -> set the dynamic fw traces bitmap

    // wl1271_init_templates_config:
        // Init templates with role_id = WL12XX_INVALID_ROLE_ID

    // wl12xx_acx_mem_cfg:
        // internal memory allocation of the firmware.
        // too few RX blocks → packet loss under high load.
        // too few TX blocks → TX stalls, high latency, or watchdog triggers.

    // wl12xx_init_fwlog:
        // Initializes Firmware Logging / Tracing.

    // wlcore_cmd_regdomain_config_locked
        // Regulatory Domain (country code, allowed channels, max TX power, etc.).

    // wl1271_init_pta:
        // Initializes PTA (Packet Traffic Arbitration) — the coexistence mechanism between WiFi and Bluetooth.

    // wl1271_acx_init_mem_config:
        // Configures internal memory allocation (how many blocks for RX, TX, stations, etc.).

    // wl12xx_init_rx_config:
        // Configures general RX (receive) parameters (filters, thresholds, etc.).

    // wl1271_acx_dco_itrim_params:
        // Configures DCO ITrim (Digitally Controlled Oscillator trimming).
        // Fine-tunes the internal clock for better stability and performance.

    // wl1271_acx_tx_config_options:
        // Configures how the firmware signals TX completion back to the driver (interrupt behavior).

    // wl1271_acx_init_rx_interrupt:
        // Configures RX interrupt pacing — how often the firmware should interrupt the host when receiving packets.

    // wl1271_init_energy_detection:
        // Configures Energy Detection (Clear Channel Assessment).
        // Helps the chip detect when the channel is busy before transmitting.

    // wl1271_acx_frag_threshold:
        // Sets the Fragmentation Threshold.
        // Packets larger than this size will be fragmented. Usually 2346 bytes (almost never fragmented in modern networks).

    // wl1271_cmd_data_path:
        // Enables the data path in the firmware.
        // Before this command, the firmware only handles management frames. After this, it can send/receive real data traffic.

    // ret = wl1271_acx_pm_config(wl);:
        // Configures Power Management settings (beacon filtering, power save parameters, etc.).:

    // wl12xx_acx_set_rate_mgmt_params:
        // Configures Rate Management parameters (how the firmware chooses TX rates, retry limits, etc.).

    // wl12xx_acx_config_hangover:
        // Configures the Hangover mechanism — a firmware feature that helps recover from TX stalls or bad channel conditions.

// wl1271_sta_hw_init:
    // wl12xx_acx_config_ps
        // Configures Power Save (PS) parameters for this station: Beacon filtering + PS-Poll behavior
        // -> battery life and power efficiency.

    // wl1271_acx_fm_coex:
        // Configures coexistence with FM radio (if the board has an FM receiver).
        // This is part of the broader coexistence management (WiFi + Bluetooth + FM).

    // wl1271_acx_sta_rate_policies:
        // Configures the Rate Policies for this STA connection.

// wl12xx_init_fw:
    // wl12xx_chip_wakeup
    // ->boot = wl18xx_boot
    // wl1271_hw_init

// wl1271_init_vif_specific:
    // Power Save configuration
    // STA role initialization
    // QoS / TID / AC configuration
    // Hardware encryption setup
    // Post-memory mode-specific init
    // BA session policies
    // Final HW vif setup

// wl12xx_get_vif_count:
    // Iterate over all if (ieee80211_vif *vif)]
    // triggered by only active if


// wl1271_op_add_interface:
    // wl12xx_init_fw
    // wl12xx_cmd_role_enable

    // wl1271_init_vif_specific                 -> p2p
    // wl1271_sta_hw_init -> specific to STA    -> sta



// wl18xx_lnk_high_prio:
    // wl->fw_status->priv is read from wl18xx_convert_fw_status (Interrupt)
    // if (suspend_bitmap = hlid = bit 1) -> choose wl18xx_lnk_low_prio 
    // else thold = tx_fast_link_prio_threshold or tx_slow_link_prio_threshold
    // CHECK lnk->allocated_pkts (usually 0) < thold

// wl18xx_lnk_low_prio:
    // wl->fw_status->priv is read from wl18xx_convert_fw_status (Interrupt)
    // if (suspend_bitmap = hlid = bit 1) -> thold = tx_suspend_threshold 
    // else thold = tx_fast_stop_threshold or tx_slow_stop_threshold
    // CHECK lnk->allocated_pkts (usually 0) < thold

// wlcore_lnk_dequeue(wl, lnk, ac);:
    // skb_dequeue(&lnk->tx_queue[ac])
    // ->tx_queue_count[q]--



/* ========================================================= */
// enum wlcore_queue_stop_reason {
// 	WLCORE_QUEUE_STOP_REASON_WATERMARK,
// 	WLCORE_QUEUE_STOP_REASON_FW_RESTART,
// 	WLCORE_QUEUE_STOP_REASON_FLUSH,
// 	WLCORE_QUEUE_STOP_REASON_SPARE_BLK, /* 18xx specific */
// };
    // -> WLCORE_QUEUE_STOP_REASON_WATERMARK = 0 is used for not stopping immediatedly 
    // -> X - ieee80211_stop_queue, X - ieee80211_wake_queue