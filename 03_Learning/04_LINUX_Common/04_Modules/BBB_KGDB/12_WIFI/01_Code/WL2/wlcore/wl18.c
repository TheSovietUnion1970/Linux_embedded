#include "wl18.h"

int VV_cmd_set_cac(struct wl1271 *wl, struct wl12xx_vif *wlvif, bool start)
{
	struct VV_cmd_cac_start *cmd;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd cac (channel %d) %s",
		     wlvif->channel, start ? "start" : "stop");

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd->role_id = wlvif->role_id;
	cmd->channel = wlvif->channel;
	if (wlvif->band == NL80211_BAND_5GHZ)
		cmd->band = WLCORE_BAND_5GHZ;
	cmd->bandwidth = wlcore_get_native_channel_type(wlvif->channel_type);

	ret = wl1271_cmd_send(wl,
			      start ? CMD_CAC_START : CMD_CAC_STOP,
			      cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("failed to send cac command");
		goto out_free;
	}

out_free:
	kfree(cmd);
	return ret;
}

int VV_cmd_smart_config_set_group_key(struct wl1271 *wl, u16 group_id,
					  u8 key_len, u8 *key)
{
	struct wl18xx_cmd_smart_config_set_group_key *cmd;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd smart config set group key id=0x%x",
		     group_id);

	if (key_len != sizeof(cmd->key)) {
		wl1271_error("invalid group key size: %d", key_len);
		return -E2BIG;
	}

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->group_id = cpu_to_le32(group_id);
	memcpy(cmd->key, key, key_len);

	ret = wl1271_cmd_send(wl, CMD_SMART_CONFIG_SET_GROUP_KEY, cmd,
			      sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("failed to send smart config set group key cmd");
		goto out_free;
	}

out_free:
	kfree(cmd);
out:
	return ret;
}

int VV_cmd_smart_config_stop(struct wl1271 *wl)
{
	struct wl1271_cmd_header *cmd;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd smart config stop");

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	ret = wl1271_cmd_send(wl, CMD_SMART_CONFIG_STOP, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("failed to send smart config stop command");
		goto out_free;
	}

out_free:
	kfree(cmd);
out:
	return ret;
}

int VV_cmd_smart_config_start(struct wl1271 *wl, u32 group_bitmap)
{
	struct VV_cmd_smart_config_start *cmd;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd smart config start group_bitmap=0x%x",
		     group_bitmap);

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->group_id_bitmask = cpu_to_le32(group_bitmap);

	ret = wl1271_cmd_send(wl, CMD_SMART_CONFIG_START, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("failed to send smart config start command");
		goto out_free;
	}

out_free:
	kfree(cmd);
out:
	return ret;
}

/*
 * this command is basically the same as wl1271_acx_ht_capabilities,
 * with the addition of supported rates. they should be unified in
 * the next fw api change
 */
int VV_acx_set_peer_cap(struct wl1271 *wl,
			    struct ieee80211_sta_ht_cap *ht_cap,
			    bool allow_ht_operation,
			    u32 rate_set, u8 hlid)
{
	struct VV_acx_peer_cap *acx;
	int ret = 0;
	u32 ht_capabilites = 0;

	wl1271_debug(DEBUG_ACX,
		     "acx set cap ht_supp: %d ht_cap: %d rates: 0x%x",
		     ht_cap->ht_supported, ht_cap->cap, rate_set);

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	if (allow_ht_operation && ht_cap->ht_supported) {
		/* no need to translate capabilities - use the spec values */
		ht_capabilites = ht_cap->cap;

		/*
		 * this bit is not employed by the spec but only by FW to
		 * indicate peer HT support
		 */
		ht_capabilites |= WL12XX_HT_CAP_HT_OPERATION;

		/* get data from A-MPDU parameters field */
		acx->ampdu_max_length = ht_cap->ampdu_factor;
		acx->ampdu_min_spacing = ht_cap->ampdu_density;
	}

	acx->hlid = hlid;
	acx->ht_capabilites = cpu_to_le32(ht_capabilites);
	acx->supported_rates = cpu_to_le32(rate_set);

	ret = wl1271_cmd_configure(wl, ACX_PEER_CAP, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx ht capabilities setting failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int VV_handle_static_data(struct wl1271 *wl,
				     struct wl1271_static_data *static_data)
{
	struct VV_static_data_priv *static_data_priv =
		(struct VV_static_data_priv *) static_data->priv;

	strncpy(wl->chip.phy_fw_ver_str, static_data_priv->phy_version,
		sizeof(wl->chip.phy_fw_ver_str));

	/* make sure the string is NULL-terminated */
	wl->chip.phy_fw_ver_str[sizeof(wl->chip.phy_fw_ver_str) - 1] = '\0';

	wl1271_info("**PHY firmware version: %s", static_data_priv->phy_version);

	return 0;
}

