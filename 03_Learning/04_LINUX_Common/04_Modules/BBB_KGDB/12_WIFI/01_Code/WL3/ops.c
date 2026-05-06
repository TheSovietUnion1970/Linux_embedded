#include "ops.h"

int VV_scan_stop(struct wl1271 *wl, struct wl12xx_vif *wlvif, u8 scan_type)
{
	struct VV_cmd_scan_stop *stop;
	int ret;

	//wl1271_debug(DEBUG_CMD, "cmd periodic scan stop");

	stop = kzalloc(sizeof(*stop), GFP_KERNEL);
	if (!stop) {
		printk("failed to alloc memory to send sched scan stop");
		return -ENOMEM;
	}

	stop->role_id = wlvif->role_id;
	stop->scan_type = scan_type;

	ret = VV_cmd_send(CMD_STOP_SCAN, stop, sizeof(*stop), 0);
	if (ret < 0) {
		printk("failed to send sched scan stop command");
		goto out_free;
	}

out_free:
	kfree(stop);
	return ret;
}


static int
VV_scan_get_channels(struct wl1271 *wl,
			 struct ieee80211_channel *req_channels[],
			 u32 n_channels,
			 u32 n_ssids,
			 struct VV_scan_ch_params *channels,
			 u32 band, bool radar, bool passive,
			 int start, int max_channels,
			 u8 *n_pactive_ch,
			 int scan_type)
{
	int i, j;
	u32 flags;
	bool force_passive = !n_ssids;
	u32 min_dwell_time_active, max_dwell_time_active;
	u32 dwell_time_passive, dwell_time_dfs;

	// /* configure dwell times according to scan type */
	// if (scan_type == SCAN_TYPE_SEARCH) {
	// 	struct conf_scan_settings *c = &wl->conf.scan;
	// 	bool active_vif_exists = !!wlcore_count_started_vifs(wl);

	// 	min_dwell_time_active = active_vif_exists ?
	// 		c->min_dwell_time_active :
	// 		c->min_dwell_time_active_long;
	// 	max_dwell_time_active = active_vif_exists ?
	// 		c->max_dwell_time_active :
	// 		c->max_dwell_time_active_long;
	// 	dwell_time_passive = c->dwell_time_passive;
	// 	dwell_time_dfs = c->dwell_time_dfs;
	// } else {
	// 	struct conf_sched_scan_settings *c = &wl->conf.sched_scan;
	// 	u32 delta_per_probe;

	// 	if (band == NL80211_BAND_5GHZ)
	// 		delta_per_probe = c->dwell_time_delta_per_probe_5;
	// 	else
	// 		delta_per_probe = c->dwell_time_delta_per_probe;

	// 	min_dwell_time_active = c->base_dwell_time +
	// 		 n_ssids * c->num_probe_reqs * delta_per_probe;

	// 	max_dwell_time_active = min_dwell_time_active +
	// 				c->max_dwell_time_delta;
	// 	dwell_time_passive = c->dwell_time_passive;
	// 	dwell_time_dfs = c->dwell_time_dfs;
	// }
	// min_dwell_time_active = DIV_ROUND_UP(min_dwell_time_active, 1000);
	// max_dwell_time_active = DIV_ROUND_UP(max_dwell_time_active, 1000);
	// dwell_time_passive = DIV_ROUND_UP(dwell_time_passive, 1000);
	// dwell_time_dfs = DIV_ROUND_UP(dwell_time_dfs, 1000);

	// [TODO]
	min_dwell_time_active = 25;
	max_dwell_time_active = 50;
	dwell_time_passive = 100;
	dwell_time_dfs = 150;

	//printk("i start = %d, j start = %d\n", i, start);

	for (i = 0, j = start;
	     i < n_channels && j < max_channels;
	     i++) {
		flags = req_channels[i]->flags;

		if (force_passive)
			flags |= IEEE80211_CHAN_NO_IR;

		if ((req_channels[i]->band == band) &&
		    !(flags & IEEE80211_CHAN_DISABLED) &&
		    (!!(flags & IEEE80211_CHAN_RADAR) == radar) &&
		    /* if radar is set, we ignore the passive flag */
		    (radar ||
		     !!(flags & IEEE80211_CHAN_NO_IR) == passive)) {
			if (flags & IEEE80211_CHAN_RADAR) {
				channels[j].flags |= SCAN_CHANNEL_FLAGS_DFS;

				channels[j].passive_duration =
					cpu_to_le16(dwell_time_dfs);
			} else {
				channels[j].passive_duration =
					cpu_to_le16(dwell_time_passive);
			}

			channels[j].min_duration =
				cpu_to_le16(min_dwell_time_active);
			channels[j].max_duration =
				cpu_to_le16(max_dwell_time_active);

			channels[j].tx_power_att = req_channels[i]->max_power;
			channels[j].channel = req_channels[i]->hw_value;

			if (n_pactive_ch &&
			    (band == NL80211_BAND_2GHZ) &&
			    (channels[j].channel >= 12) &&
			    (channels[j].channel <= 14) &&
			    (flags & IEEE80211_CHAN_NO_IR) &&
			    !force_passive) {
				/* pactive channels treated as DFS */
				channels[j].flags = SCAN_CHANNEL_FLAGS_DFS;

				/*
				 * n_pactive_ch is counted down from the end of
				 * the passive channel list
				 */
				(*n_pactive_ch)++;
				// wl1271_debug(DEBUG_SCAN, "n_pactive_ch = %d",
				// 	     *n_pactive_ch);
			}

			// channels.flags
			// channels.passive_duration
			// channels.min_duration
			// channels.max_duration
			// channels.tx_power_att
			// channels.channel

			// -> 3 types: active, passive, DFS

			// wl1271_debug(DEBUG_SCAN, "freq %d, ch. %d, flags 0x%x, power %d, min/max_dwell %d/%d%s%s",
			// 	     req_channels[i]->center_freq,
			// 	     req_channels[i]->hw_value,
			// 	     req_channels[i]->flags,
			// 	     req_channels[i]->max_power,
			// 	     min_dwell_time_active,
			// 	     max_dwell_time_active,
			// 	     flags & IEEE80211_CHAN_RADAR ?
			// 		", DFS" : "",
			// 	     flags & IEEE80211_CHAN_NO_IR ?
			// 		", NO-IR" : "");
			j++;
		}
	}
	//printk("i end = %d, j end = %d\n", i, j);

	return j - start;
}

bool
VV_set_scan_chan_params(struct wl1271 *wl,
			    struct VV_scan_channels *cfg,
			    struct ieee80211_channel *channels[],
			    u32 n_channels,
			    u32 n_ssids,
			    int scan_type)
{
	u8 n_pactive_ch = 0;

	cfg->passive[0] =
		VV_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_2,
					 NL80211_BAND_2GHZ,
					 false, true, 0,
					 MAX_CHANNELS_2GHZ,
					 &n_pactive_ch,
					 scan_type);
	cfg->active[0] =
		VV_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_2,
					 NL80211_BAND_2GHZ,
					 false, false,
					 cfg->passive[0],
					 MAX_CHANNELS_2GHZ,
					 &n_pactive_ch,
					 scan_type);
	cfg->passive[1] =
		VV_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_5,
					 NL80211_BAND_5GHZ,
					 false, true, 0,
					 wl->max_channels_5,
					 &n_pactive_ch,
					 scan_type);
	cfg->dfs =
		VV_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_5,
					 NL80211_BAND_5GHZ,
					 true, true,
					 cfg->passive[1],
					 wl->max_channels_5,
					 &n_pactive_ch,
					 scan_type);
	cfg->active[1] =
		VV_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_5,
					 NL80211_BAND_5GHZ,
					 false, false,
					 cfg->passive[1] + cfg->dfs,
					 wl->max_channels_5,
					 &n_pactive_ch,
					 scan_type);

	/* 802.11j channels are not supported yet */
	cfg->passive[2] = 0;
	cfg->active[2] = 0;

	cfg->passive_active = n_pactive_ch;

	// wl1271_debug(DEBUG_SCAN, "    2.4GHz: active %d passive %d",
	// 	     cfg->active[0], cfg->passive[0]);
	// wl1271_debug(DEBUG_SCAN, "    5GHz: active %d passive %d",
	// 	     cfg->active[1], cfg->passive[1]);
	// wl1271_debug(DEBUG_SCAN, "    DFS: %d", cfg->dfs);

	return  cfg->passive[0] || cfg->active[0] ||
		cfg->passive[1] || cfg->active[1] || cfg->dfs ||
		cfg->passive[2] || cfg->active[2];
}

static void VV_adjust_channels(struct VV_cmd_scan_params *cmd,
				   struct VV_scan_channels *cmd_channels)
{
	memcpy(cmd->passive, cmd_channels->passive, sizeof(cmd->passive));
	memcpy(cmd->active, cmd_channels->active, sizeof(cmd->active));
	cmd->dfs = cmd_channels->dfs;
	cmd->passive_active = cmd_channels->passive_active;

	memcpy(cmd->channels_2, cmd_channels->channels_2,
	       sizeof(cmd->channels_2));
	memcpy(cmd->channels_5, cmd_channels->channels_5,
	       sizeof(cmd->channels_5));
	/* channels_4 are not supported, so no need to copy them */
}

int VV_scan_send(struct wl1271 *wl, struct wl12xx_vif *wlvif,
			    struct cfg80211_scan_request *req)
{
	struct VV_cmd_scan_params *cmd;
	struct VV_scan_channels *cmd_channels = NULL;
	int ret;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	/* scan on the dev role if the regular one is not started */
	if (wlcore_is_p2p_mgmt(wlvif))
		cmd->role_id = wlvif->dev_role_id;
	else
		cmd->role_id = wlvif->role_id;

	if (WARN_ON(cmd->role_id == WL12XX_INVALID_ROLE_ID)) {
		ret = -EINVAL;
		goto out;
	}

	cmd->scan_type = SCAN_TYPE_SEARCH;
	cmd->rssi_threshold = -127;
	cmd->snr_threshold = 0;

	cmd->bss_type = SCAN_BSS_TYPE_ANY;

	cmd->ssid_from_list = 0;
	cmd->filter = 0;
	cmd->add_broadcast = 0;

	cmd->urgency = 0;
	cmd->protect = 0;

	cmd->n_probe_reqs = wl->conf.scan.num_probe_reqs;
	cmd->terminate_after = 0;

	/* configure channels */
	WARN_ON(req->n_ssids > 1);

	cmd_channels = kzalloc(sizeof(*cmd_channels), GFP_KERNEL);
	if (!cmd_channels) {
		ret = -ENOMEM;
		goto out;
	}

	//printk("[SCAN] - n_channels = %d, n_ssids = %d\n",  req->n_channels, req->n_ssids);
	VV_set_scan_chan_params(wl, cmd_channels, req->channels,
				    req->n_channels, req->n_ssids,
				    SCAN_TYPE_SEARCH);
	VV_adjust_channels(cmd, cmd_channels); // cpy cmd_channels->active|passive|dtfs -> cmd

	/*
	 * all the cycles params (except total cycles) should
	 * remain 0 for normal scan
	 */
	cmd->total_cycles = 1;

	if (req->no_cck)
		cmd->rate = WL18XX_SCAN_RATE_6;

	cmd->tag = WL1271_SCAN_DEFAULT_TAG;

	if (req->n_ssids) {
		cmd->ssid_len = req->ssids[0].ssid_len;
		memcpy(cmd->ssid, req->ssids[0].ssid, cmd->ssid_len);
	}

	/* TODO: per-band ies? */
	if (cmd->active[0]) {
		u8 band = NL80211_BAND_2GHZ;
		ret = wl12xx_cmd_build_probe_req(wl, wlvif,
				 cmd->role_id, band,
				 req->ssids ? req->ssids[0].ssid : NULL,
				 req->ssids ? req->ssids[0].ssid_len : 0,
				 req->ie,
				 req->ie_len,
				 NULL,
				 0,
				 false);
		if (ret < 0) {
			printk("2.4GHz PROBE request template failed");
			goto out;
		}
	}

	if (cmd->active[1] || cmd->dfs) {
		u8 band = NL80211_BAND_5GHZ;
		ret = wl12xx_cmd_build_probe_req(wl, wlvif,
				 cmd->role_id, band,
				 req->ssids ? req->ssids[0].ssid : NULL,
				 req->ssids ? req->ssids[0].ssid_len : 0,
				 req->ie,
				 req->ie_len,
				 NULL,
				 0,
				 false);
		if (ret < 0) {
			//("5GHz PROBE request template failed");
			goto out;
		}
	}

	//wl1271_dump(DEBUG_SCAN, "SCAN: ", cmd, sizeof(*cmd));

	ret = VV_cmd_send(CMD_SCAN, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		printk("SCAN failed");
		goto out;
	}

out:
	kfree(cmd_channels);
	kfree(cmd);
	return ret;
}

int VV_get_mac(struct wl1271 *wl)
{
	u32 mac1, mac2;
	int ret;

	ret = VV_set_partition_core(wl, &wifi_data.ptable[PART_TOP_PRCM_ELP_SOC]);
	if (ret < 0)
		goto out;

	//ret = wlcore_read32(wl, WL18XX_REG_FUSE_BD_ADDR_1, &mac1);
	ret = VV_sdio_raw_read(wlcore_translate_addr(WL18XX_REG_FUSE_BD_ADDR_1), &mac1, 4, false);
	if (ret < 0)
		goto out;

	//ret = wlcore_read32(wl, WL18XX_REG_FUSE_BD_ADDR_2, &mac2);
	ret = VV_sdio_raw_read(wlcore_translate_addr(WL18XX_REG_FUSE_BD_ADDR_2), &mac2, 4, false);
	if (ret < 0)
		goto out;

	/* these are the two parts of the BD_ADDR */
	wl->fuse_oui_addr = ((mac2 & 0xffff) << 8) +
		((mac1 & 0xff000000) >> 24);
	wl->fuse_nic_addr = (mac1 & 0xffffff);

	if (!wl->fuse_oui_addr && !wl->fuse_nic_addr) {
		u8 mac[ETH_ALEN];

		eth_random_addr(mac);

		wl->fuse_oui_addr = (mac[0] << 16) + (mac[1] << 8) + mac[2];
		wl->fuse_nic_addr = (mac[3] << 16) + (mac[4] << 8) + mac[5];
		//printk("MAC address from fuse not available, using random locally administered addresses.");
	}

	ret = VV_set_partition_core(wl, &wifi_data.ptable[PART_DOWN]);

out:
	return ret;
}

static const char *VV_rdl_name(enum wl18xx_rdl_num rdl_num)
{
	switch (rdl_num) {
	case RDL_1_HP:
		return "183xH";
	case RDL_2_SP:
		return "183x or 180x";
	case RDL_3_HP:
		return "187xH";
	case RDL_4_SP:
		return "187x";
	case RDL_5_SP:
		return "RDL11 - Not Supported";
	case RDL_6_SP:
		return "180xD";
	case RDL_7_SP:
		return "RDL13 - Not Supported (1893Q)";
	case RDL_8_SP:
		return "18xxQ";
	case RDL_NONE:
		return "UNTRIMMED";
	default:
		return "UNKNOWN";
	}
}

int VV1_get_pg_ver(struct wl1271 *wl, s8 *ver)
{
	u32 fuse;
	s8 rom = 0, metal = 0, pg_ver = 0, rdl_ver = 0, package_type = 0;
	int ret;

	ret = VV_set_partition_core(wl, &wifi_data.ptable[PART_TOP_PRCM_ELP_SOC]);
	if (ret < 0)
		goto out;

	//ret = wlcore_read32(wl, WL18XX_REG_FUSE_DATA_2_3, &fuse);
	ret = VV_sdio_raw_read(wlcore_translate_addr(WL18XX_REG_FUSE_DATA_2_3), &fuse, 4, false);
	if (ret < 0)
		goto out;

	package_type = (fuse >> WL18XX_PACKAGE_TYPE_OFFSET) & 1;

	//ret = wlcore_read32(wl, WL18XX_REG_FUSE_DATA_1_3, &fuse);
	ret = VV_sdio_raw_read(wlcore_translate_addr(WL18XX_REG_FUSE_DATA_1_3), &fuse, 4, false);
	if (ret < 0)
		goto out;

	pg_ver = (fuse & WL18XX_PG_VER_MASK) >> WL18XX_PG_VER_OFFSET;
	rom = (fuse & WL18XX_ROM_VER_MASK) >> WL18XX_ROM_VER_OFFSET;

	if ((rom <= 0xE) && (package_type == WL18XX_PACKAGE_TYPE_WSP))
		metal = (fuse & WL18XX_METAL_VER_MASK) >>
			WL18XX_METAL_VER_OFFSET;
	else
		metal = (fuse & WL18XX_NEW_METAL_VER_MASK) >>
			WL18XX_NEW_METAL_VER_OFFSET;

	//ret = wlcore_read32(wl, WL18XX_REG_FUSE_DATA_2_3, &fuse);
	ret = VV_sdio_raw_read(wlcore_translate_addr(WL18XX_REG_FUSE_DATA_2_3), &fuse, 4, false);
	if (ret < 0)
		goto out;

	rdl_ver = (fuse & WL18XX_RDL_VER_MASK) >> WL18XX_RDL_VER_OFFSET;

	printk("wl18xx HW: %s, PG %d.%d (ROM 0x%x)",
		    VV_rdl_name(rdl_ver), pg_ver, metal, rom);

	if (ver)
		*ver = pg_ver;

	ret = VV_set_partition_core(wl, &wifi_data.ptable[PART_BOOT]);

out:
	return ret;
}

int VV_wait_for_event(enum wlcore_wait_event event, bool *timeout)
{
	u32 local_event;

	switch (event) {
	case WLCORE_EVENT_PEER_REMOVE_COMPLETE:
		local_event = PEER_REMOVE_COMPLETE_EVENT_ID;
		break;

	case WLCORE_EVENT_DFS_CONFIG_COMPLETE:
		local_event = DFS_CHANNELS_CONFIG_COMPLETE_EVENT;
		break;

	default:
		/* event not implemented */
		return 0;
	}
	return wlcore_cmd_wait_for_event_or_timeout(local_event, timeout);
}

int VV_identify_chip(struct wl1271 *wl)
{
	int ret = 0;

	switch (wl->chip.id) {
	case CHIP_ID_185x_PG20:
		// wl1271_debug(DEBUG_BOOT, "chip id 0x%x (185x PG20)",
		// 		 wl->chip.id);
		wl->sr_fw_name = WL18XX_FW_NAME;
		/* wl18xx uses the same firmware for PLT */
		wl->plt_fw_name = WL18XX_FW_NAME;
		wl->quirks |= WLCORE_QUIRK_RX_BLOCKSIZE_ALIGN |
			      WLCORE_QUIRK_TX_BLOCKSIZE_ALIGN |
			      WLCORE_QUIRK_NO_SCHED_SCAN_WHILE_CONN |
			      WLCORE_QUIRK_TX_PAD_LAST_FRAME |
			      WLCORE_QUIRK_REGDOMAIN_CONF |
			      WLCORE_QUIRK_DUAL_PROBE_TMPL;

		wlcore_set_min_fw_ver(wl, WL18XX_CHIP_VER,
				      WL18XX_IFTYPE_VER,  WL18XX_MAJOR_VER,
				      WL18XX_SUBTYPE_VER, WL18XX_MINOR_VER,
				      /* there's no separate multi-role FW */
				      0, 0, 0, 0);
		break;
	case CHIP_ID_185x_PG10:
		printk("chip id 0x%x (185x PG10) is deprecated",
			       wl->chip.id);
		ret = -ENODEV;
		goto out;

	default:
		printk("unsupported chip id: 0x%x", wl->chip.id);
		ret = -ENODEV;
		goto out;
	}

	wl->fw_mem_block_size = 272;
	wl->fwlog_end = 0x40000000;

	wl->scan_templ_id_2_4 = CMD_TEMPL_CFG_PROBE_REQ_2_4;
	wl->scan_templ_id_5 = CMD_TEMPL_CFG_PROBE_REQ_5;
	wl->sched_scan_templ_id_2_4 = CMD_TEMPL_PROBE_REQ_2_4_PERIODIC;
	wl->sched_scan_templ_id_5 = CMD_TEMPL_PROBE_REQ_5_PERIODIC;
	wl->max_channels_5 = WL18XX_MAX_CHANNELS_5GHZ;
	wl->ba_rx_session_count_max = WL18XX_RX_BA_MAX_SESSIONS;
out:
	return ret;
}


