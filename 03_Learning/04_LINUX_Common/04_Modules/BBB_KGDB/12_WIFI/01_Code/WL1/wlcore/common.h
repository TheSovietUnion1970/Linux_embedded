#define STA_BASIC_RATE_IDX          0
#define STA_AP_RATE_IDX             1
#define STA_P2P_RATE_IDX            2

#define STA_KLV_TEMPLATE_IDX        0

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
    // wl->tx_blocks_available -= total_blocks;
    // wl->tx_allocated_blocks += total_blocks;
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