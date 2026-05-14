#include "wl18.h"

/*
 * this command is basically the same as wifi_acx_ht_capabilities,
 * with the addition of supported rates. they should be unified in
 * the next fw api change
 */
int wifi_acx_set_peer_cap(
			    struct ieee80211_sta_ht_cap *ht_cap,
			    bool allow_ht_operation,
			    u32 rate_set, u8 hlid)
{
	struct wifi_acx_peer_cap *acx;
	int ret = 0;
	u32 ht_capabilites = 0;

	wifi_debug(DEBUG_ACX,
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

	ret = wifi_cmd_configure(ACX_PEER_CAP, acx, sizeof(*acx));
	if (ret < 0) {
		wifi_warning("acx ht capabilities setting failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wifi_handle_static_data(struct wifi_static_data *static_data)
{
	struct wifi_static_data_priv *static_data_priv =
		(struct wifi_static_data_priv *) static_data->priv;

	strncpy(wifi_chip->phy_fw_ver_str, static_data_priv->phy_version,
		sizeof(wifi_chip->phy_fw_ver_str));

	/* make sure the string is NULL-terminated */
	wifi_chip->phy_fw_ver_str[sizeof(wifi_chip->phy_fw_ver_str) - 1] = '\0';

	printk("**PHY firmware version: %s", static_data_priv->phy_version);

	return 0;
}

