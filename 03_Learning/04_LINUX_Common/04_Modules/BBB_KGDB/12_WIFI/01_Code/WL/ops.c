#include "ops.h"
#include "main.h"

static int
wifi_scan_get_channels(
			 struct ieee80211_channel *req_channels[],
			 u32 n_channels,
			 u32 n_ssids,
			 struct wifi_scan_ch_params *channels,
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

	// [TODO]
	min_dwell_time_active = 25;
	max_dwell_time_active = 50;
	dwell_time_passive = 100;
	dwell_time_dfs = 150;

	// always: n_channel <= max_channel
	// j is used to prevent overflow of stuct buffer, but overflow 
	// rarely happens as n_channel <= max_channel

	// req_channels - i - n_channel   : from ptr
	// channels     - j - max_channels: from macro

	// from multiple req_channels[i] -> only conditional channels[j]
	// multiple req_channels[i] is scaned for passive0, active0, ... active1
#if (PRINT_DEBUG_SCAN)
	printk("START LOOP - %s\n", (band == NL80211_BAND_2GHZ) ? "2.4GHz" : "5GHz");
#endif
	for (i = 0, j = start;
	     i < n_channels && j < max_channels;
	     i++) {
		flags = req_channels[i]->flags;

		if (force_passive)
			flags |= IEEE80211_CHAN_NO_IR;

		//printk("band = %d\n", req_channels[i]->band);
		if ((req_channels[i]->band == band) &&
		    !(flags & IEEE80211_CHAN_DISABLED) &&
		    (!!(flags & IEEE80211_CHAN_RADAR) == radar) &&
		    /* if radar is set, we ignore the passive flag */
		    (radar ||
		     !!(flags & IEEE80211_CHAN_NO_IR) == passive)) 
		{
#if (PRINT_DEBUG_SCAN)
			printk("%d is selected, channel = %d, freq = %d\n", i, req_channels[i]->hw_value, 
							ieee80211_channel_to_frequency(req_channels[i]->hw_value, band));
#endif
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


			// channel 12-14 as passive = listen only (DFS(special passive-like scan))
			// In many countries/regions (especially Europe, Japan, etc.), 
			// channels 12 and 13 (and sometimes 14) have strict regulatory restrictions
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
			}
			j++;
		}
	}
	// update channels		: flags, passive_duration, min_duration, max_duration, tx_power_att, channel
	// from   req_channels  : flags, band, max_power, hw_value

	return j - start;
}

static bool
wifi_set_scan_chan_params(
			    struct wifi_scan_channels *cfg,
			    struct ieee80211_channel *channels[],
			    u32 n_channels,
			    u32 n_ssids,
			    int scan_type)
{
	u8 n_pactive_ch = 0;
#if (PRINT_DEBUG)
	printk("[SCAN] - %d, %d, %d\n", n_channels, n_ssids, scan_type);
#endif

	cfg->passive[0] =
		wifi_scan_get_channels(
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
		wifi_scan_get_channels(
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
		wifi_scan_get_channels(
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_5,
					 NL80211_BAND_5GHZ,
					 false, true, 0,
					 WL18XX_MAX_CHANNELS_5GHZ,
					 &n_pactive_ch,
					 scan_type);
	cfg->dfs =
		wifi_scan_get_channels(
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_5,
					 NL80211_BAND_5GHZ,
					 true, true,
					 cfg->passive[1],
					 WL18XX_MAX_CHANNELS_5GHZ,
					 &n_pactive_ch,
					 scan_type);
	cfg->active[1] =
		wifi_scan_get_channels(
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_5,
					 NL80211_BAND_5GHZ,
					 false, false,
					 cfg->passive[1] + cfg->dfs,
					 WL18XX_MAX_CHANNELS_5GHZ,
					 &n_pactive_ch,
					 scan_type);

	/* 802.11j channels are not supported yet */
	cfg->passive[2] = 0;
	cfg->active[2] = 0;

	cfg->passive_active = n_pactive_ch;

	// wifi_debug(DEBUG_SCAN, "    2.4GHz: active %d passive %d",
	// 	     cfg->active[0], cfg->passive[0]);
	// wifi_debug(DEBUG_SCAN, "    5GHz: active %d passive %d",
	// 	     cfg->active[1], cfg->passive[1]);
	// wifi_debug(DEBUG_SCAN, "    DFS: %d", cfg->dfs);

	return  cfg->passive[0] || cfg->active[0] ||
		cfg->passive[1] || cfg->active[1] || cfg->dfs ||
		cfg->passive[2] || cfg->active[2];
}

static void wifi_adjust_channels(struct wifi_cmd_scan_params *cmd,
				   struct wifi_scan_channels *cmd_channels)
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

int wifi_scan_send(struct wifi_vif *wifi_vif,
			    struct cfg80211_scan_request *req)
{
	struct wifi_cmd_scan_params *cmd;
	struct wifi_scan_channels *cmd_channels = NULL;
	int ret;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	/* scan on the dev role if the regular one is not started */
	if (wificore_is_p2p_mgmt(wifi_vif))
		cmd->role_id = wifi_vif->dev_role_id;
	else
		cmd->role_id = wifi_vif->role_id;

	if (cmd->role_id == WL12XX_INVALID_ROLE_ID) {
		printk("INVALID - role_id\n");
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

	// .num_probe_reqs			= 2,
	cmd->n_probe_reqs = 2;
	cmd->terminate_after = 0;

	/* configure channels */
	WARN_ON(req->n_ssids > 1);

	cmd_channels = kzalloc(sizeof(*cmd_channels), GFP_KERNEL);
	if (!cmd_channels) {
		ret = -ENOMEM;
		goto out;
	}

	wifi_set_scan_chan_params(cmd_channels, req->channels,
				    req->n_channels, req->n_ssids,
				    SCAN_TYPE_SEARCH);
	// returning cmd_channels->passive[0], active[0]      -> 2.4 GHz
						//	 ->passive[1], dfs, active[1] -> 5 GHz
	// FROM un-ordered req->channels
	wifi_adjust_channels(cmd, cmd_channels); // cpy cmd_channels->active|passive|dtfs -> cmd

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
		ret = wifi_cmd_build_probe_req(wifi_vif,
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
		ret = wifi_cmd_build_probe_req(wifi_vif,
				 cmd->role_id, band,
				 req->ssids ? req->ssids[0].ssid : NULL,
				 req->ssids ? req->ssids[0].ssid_len : 0,
				 req->ie,
				 req->ie_len,
				 NULL,
				 0,
				 false);
		if (ret < 0) {
			printk("5GHz PROBE request template failed");
			goto out;
		}
	}

	//wifi_dump(DEBUG_SCAN, "SCAN: ", cmd, sizeof(*cmd));

	ret = wifi_cmd_send(CMD_SCAN, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		printk("SCAN failed");
		goto out;
	}

out:
	kfree(cmd_channels);
	kfree(cmd);
	return ret;
}

int wifi_get_mac(void)
{
	u32 mac1, mac2;
	int ret;

	ret = wifi_set_partition_core(&wifi_data->ptable[PART_TOP_PRCM_ELP_SOC]);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_read(wificore_translate_addr(WL18XX_REG_FUSE_BD_ADDR_1), &mac1, 4, false);
	if (ret < 0)
		goto out;

	ret = wifi_sdio_raw_read(wificore_translate_addr(WL18XX_REG_FUSE_BD_ADDR_2), &mac2, 4, false);
	if (ret < 0)
		goto out;

	/* these are the two parts of the BD_ADDR */
	wifi_data->fuse_oui_addr = ((mac2 & 0xffff) << 8) +
		((mac1 & 0xff000000) >> 24);
	wifi_data->fuse_nic_addr = (mac1 & 0xffffff);

	if (!wifi_data->fuse_oui_addr && !wifi_data->fuse_nic_addr) {
		u8 mac[ETH_ALEN];

		eth_random_addr(mac);

		wifi_data->fuse_oui_addr = (mac[0] << 16) + (mac[1] << 8) + mac[2];
		wifi_data->fuse_nic_addr = (mac[3] << 16) + (mac[4] << 8) + mac[5];
		//printk("MAC address from fuse not available, using random locally administered addresses.");
	}

	ret = wifi_set_partition_core(&wifi_data->ptable[PART_DOWN]);

out:
	return ret;
}

int wifi_wait_for_event(enum wificore_wait_event event, bool *timeout)
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
	return wificore_cmd_wait_for_event_or_timeout(local_event, timeout);
}

static inline void
wificore_set_min_fw_ver(unsigned int chip,
		      unsigned int iftype_sr, unsigned int major_sr,
		      unsigned int subtype_sr, unsigned int minor_sr,
		      unsigned int iftype_mr, unsigned int major_mr,
		      unsigned int subtype_mr, unsigned int minor_mr)
{
	wifi_data->min_sr_fw_ver[FW_VER_CHIP] = chip;
	wifi_data->min_sr_fw_ver[FW_VER_IF_TYPE] = iftype_sr;
	wifi_data->min_sr_fw_ver[FW_VER_MAJOR] = major_sr;
	wifi_data->min_sr_fw_ver[FW_VER_SUBTYPE] = subtype_sr;
	wifi_data->min_sr_fw_ver[FW_VER_MINOR] = minor_sr;

	wifi_data->min_mr_fw_ver[FW_VER_CHIP] = chip;
	wifi_data->min_mr_fw_ver[FW_VER_IF_TYPE] = iftype_mr;
	wifi_data->min_mr_fw_ver[FW_VER_MAJOR] = major_mr;
	wifi_data->min_mr_fw_ver[FW_VER_SUBTYPE] = subtype_mr;
	wifi_data->min_mr_fw_ver[FW_VER_MINOR] = minor_mr;
}

int wifi_identify_chip(void)
{
	int ret = 0;

	switch (wifi_chip->id) {
	case CHIP_ID_185x_PG20:
		// wifi_debug(DEBUG_BOOT, "chip id 0x%x (185x PG20)",
		// 		 wifi_chip->id);
		wifi_data->sr_fw_name = WL18XX_FW_NAME;
		/* wl18xx uses the same firmware for PLT */
		//wifi_data->plt_fw_name = WL18XX_FW_NAME;
		wifi_data->quirks |= WLCORE_QUIRK_RX_BLOCKSIZE_ALIGN |
			      WLCORE_QUIRK_TX_BLOCKSIZE_ALIGN |
			      WLCORE_QUIRK_NO_SCHED_SCAN_WHILE_CONN |
			      WLCORE_QUIRK_TX_PAD_LAST_FRAME |
			      WLCORE_QUIRK_REGDOMAIN_CONF |
			      WLCORE_QUIRK_DUAL_PROBE_TMPL;

		wificore_set_min_fw_ver(WL18XX_CHIP_VER,
				      WL18XX_IFTYPE_VER,  WL18XX_MAJOR_VER,
				      WL18XX_SUBTYPE_VER, WL18XX_MINOR_VER,
				      /* there's no separate multi-role FW */
				      0, 0, 0, 0);
		break;
	case CHIP_ID_185x_PG10:
		printk("chip id 0x%x (185x PG10) is deprecated",
			       wifi_chip->id);
		ret = -ENODEV;
		goto out;

	default:
		printk("unsupported chip id: 0x%x", wifi_chip->id);
		ret = -ENODEV;
		goto out;
	}
out:
	return ret;
}

#if (PRINT_DEBUG_DATA_FRAME)
void WIFI_Print_Hex(u8 *data, u16 len, u8 *name){
    char line[3 * 8 + 1]; // "XX " * 8 bytes + null terminator = 25 chars
    u16 i;

    if (!data || len == 0)
        return;

    printk("# %s (len=%u bytes):\n", name, len);

    for (i = 0; i < len; i++) {
        int pos = (i % 8) * 3;
        snprintf(&line[pos], sizeof(line) - pos, "%02X ", data[i]);

        // Print every 8 bytes, or at the end of data
        if ((i % 8) == 7 || i == len - 1) {
            printk("  %s\n", line);
            memset(line, 0, sizeof(line));
        }
    }
}
#endif

