// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include "acx.h"

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>

#include "wlcore.h"
#include "debug.h"
#include "wl12xx_80211.h"
//#include "hw_ops.h"

#include "common.h"

int wl1271_acx_wake_up_conditions(struct wl12xx_vif *wlvif,
				  u8 wake_up_event, u8 listen_interval)
{
	struct acx_wake_up_condition *wake_up;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx wake up conditions (wake_up_event %d listen_interval %d)",
		     wake_up_event, listen_interval);

	wake_up = kzalloc(sizeof(*wake_up), GFP_KERNEL);
	if (!wake_up) {
		ret = -ENOMEM;
		goto out;
	}

	wake_up->role_id = wlvif->role_id;
	wake_up->wake_up_event = wake_up_event;
	wake_up->listen_interval = listen_interval;

	ret = VV_cmd_configure(ACX_WAKE_UP_CONDITIONS,
				   wake_up, sizeof(*wake_up));
	if (ret < 0) {
		wl1271_warning("could not set wake up conditions: %d", ret);
		goto out;
	}

out:
	kfree(wake_up);
	return ret;
}

int wl1271_acx_sleep_auth(u8 sleep_auth)
{
	struct acx_sleep_auth *auth;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx sleep auth %d", sleep_auth);

	auth = kzalloc(sizeof(*auth), GFP_KERNEL);
	if (!auth) {
		ret = -ENOMEM;
		goto out;
	}

	auth->sleep_auth = sleep_auth;

	ret = VV_cmd_configure(ACX_SLEEP_AUTH, auth, sizeof(*auth));
	if (ret < 0) {
		wl1271_error("could not configure sleep_auth to %d: %d",
			     sleep_auth, ret);
		goto out;
	}

	wifi_data->sleep_auth = sleep_auth;
out:
	kfree(auth);
	return ret;
}
EXPORT_SYMBOL_GPL(wl1271_acx_sleep_auth);

int wl1271_acx_tx_power(struct wl12xx_vif *wlvif,
			int power)
{
	struct acx_current_tx_power *acx;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx dot11_cur_tx_pwr %d", power);

	if (power < 0 || power > 25)
		return -EINVAL;

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->current_tx_power = power * 10;

	ret = VV_cmd_configure(DOT11_CUR_TX_PWR, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("configure of tx power failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_feature_cfg(struct wl12xx_vif *wlvif)
{
	struct acx_feature_config *feature;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx feature cfg");

	feature = kzalloc(sizeof(*feature), GFP_KERNEL);
	if (!feature) {
		ret = -ENOMEM;
		goto out;
	}

	/* DF_ENCRYPTION_DISABLE and DF_SNIFF_MODE_ENABLE are disabled */
	feature->role_id = wlvif->role_id;
	feature->data_flow_options = 0;
	feature->options = 0;

	ret = VV_cmd_configure(ACX_FEATURE_CFG,
				   feature, sizeof(*feature));
	if (ret < 0) {
		wl1271_error("Couldn't set HW encryption");
		goto out;
	}

out:
	kfree(feature);
	return ret;
}

int wl1271_acx_mem_map(struct acx_header *mem_map,
		       size_t len)
{
	int ret;

	wl1271_debug(DEBUG_ACX, "acx mem map");

	ret = wl1271_cmd_interrogate(ACX_MEM_MAP, mem_map,
				     sizeof(struct acx_header), len);
	if (ret < 0)
		return ret;

	return 0;
}

int wl1271_acx_rx_msdu_life_time(void)
{
	struct acx_rx_msdu_lifetime *acx;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx rx msdu life time");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->lifetime = cpu_to_le32(wifi_data->conf.rx.rx_msdu_life_time);
	ret = VV_cmd_configure(DOT11_RX_MSDU_LIFE_TIME,
				   acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("failed to set rx msdu life time: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_slot(struct wl12xx_vif *wlvif,
		    enum acx_slot_type slot_time)
{
	struct acx_slot *slot;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx slot");

	slot = kzalloc(sizeof(*slot), GFP_KERNEL);
	if (!slot) {
		ret = -ENOMEM;
		goto out;
	}

	slot->role_id = wlvif->role_id;
	slot->wone_index = STATION_WONE_INDEX;
	slot->slot_time = slot_time;

	ret = VV_cmd_configure(ACX_SLOT, slot, sizeof(*slot));
	if (ret < 0) {
		wl1271_warning("failed to set slot time: %d", ret);
		goto out;
	}

out:
	kfree(slot);
	return ret;
}

int wl1271_acx_group_address_tbl(struct wl12xx_vif *wlvif,
				 bool enable, void *mc_list, u32 mc_list_len)
{
	struct acx_dot11_grp_addr_tbl *acx;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx group address tbl");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	/* MAC filtering */
	acx->role_id = wlvif->role_id;
	acx->enabled = enable;
	acx->num_groups = mc_list_len;
	memcpy(acx->mac_table, mc_list, mc_list_len * ETH_ALEN);

	ret = VV_cmd_configure(DOT11_GROUP_ADDRESS_TBL,
				   acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("failed to set group addr table: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_service_period_timeout(
				      struct wl12xx_vif *wlvif)
{
	struct acx_rx_timeout *rx_timeout;
	int ret;

	rx_timeout = kzalloc(sizeof(*rx_timeout), GFP_KERNEL);
	if (!rx_timeout) {
		ret = -ENOMEM;
		goto out;
	}

	wl1271_debug(DEBUG_ACX, "acx service period timeout");

	rx_timeout->role_id = wlvif->role_id;
	rx_timeout->ps_poll_timeout = cpu_to_le16(wifi_data->conf.rx.ps_poll_timeout);
	rx_timeout->upsd_timeout = cpu_to_le16(wifi_data->conf.rx.upsd_timeout);

	ret = VV_cmd_configure(ACX_SERVICE_PERIOD_TIMEOUT,
				   rx_timeout, sizeof(*rx_timeout));
	if (ret < 0) {
		wl1271_warning("failed to set service period timeout: %d",
			       ret);
		goto out;
	}

out:
	kfree(rx_timeout);
	return ret;
}

int wl1271_acx_rts_threshold(struct wl12xx_vif *wlvif,
			     u32 rts_threshold)
{
	struct acx_rts_threshold *rts;
	int ret;

	/*
	 * If the RTS threshold is not configured or out of range, use the
	 * default value.
	 */
	if (rts_threshold > IEEE80211_MAX_RTS_THRESHOLD)
		rts_threshold = wifi_data->conf.rx.rts_threshold;

	wl1271_debug(DEBUG_ACX, "acx rts threshold: %d", rts_threshold);

	rts = kzalloc(sizeof(*rts), GFP_KERNEL);
	if (!rts) {
		ret = -ENOMEM;
		goto out;
	}

	rts->role_id = wlvif->role_id;
	rts->threshold = cpu_to_le16((u16)rts_threshold);

	ret = VV_cmd_configure(DOT11_RTS_THRESHOLD, rts, sizeof(*rts));
	if (ret < 0) {
		wl1271_warning("failed to set rts threshold: %d", ret);
		goto out;
	}

out:
	kfree(rts);
	return ret;
}

int wl1271_acx_dco_itrim_params(void)
{
	struct acx_dco_itrim_params *dco;
	struct conf_itrim_settings *c = &wifi_data->conf.itrim;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx dco itrim parameters");

	dco = kzalloc(sizeof(*dco), GFP_KERNEL);
	if (!dco) {
		ret = -ENOMEM;
		goto out;
	}

	dco->enable = c->enable;
	dco->timeout = cpu_to_le32(c->timeout);

	ret = VV_cmd_configure(ACX_SET_DCO_ITRIM_PARAMS,
				   dco, sizeof(*dco));
	if (ret < 0) {
		wl1271_warning("failed to set dco itrim parameters: %d", ret);
		goto out;
	}

out:
	kfree(dco);
	return ret;
}

int wl1271_acx_beacon_filter_opt(struct wl12xx_vif *wlvif,
				 bool enable_filter)
{
	struct acx_beacon_filter_option *beacon_filter = NULL;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx beacon filter opt enable=%d",
		     enable_filter);

	if (enable_filter &&
	    wifi_data->conf.conn.bcn_filt_mode == CONF_BCN_FILT_MODE_DISABLED)
		goto out;

	beacon_filter = kzalloc(sizeof(*beacon_filter), GFP_KERNEL);
	if (!beacon_filter) {
		ret = -ENOMEM;
		goto out;
	}

	beacon_filter->role_id = wlvif->role_id;
	beacon_filter->enable = enable_filter;

	/*
	 * When set to zero, and the filter is enabled, beacons
	 * without the unicast TIM bit set are dropped.
	 */
	beacon_filter->max_num_beacons = 0;

	ret = VV_cmd_configure(ACX_BEACON_FILTER_OPT,
				   beacon_filter, sizeof(*beacon_filter));
	if (ret < 0) {
		wl1271_warning("failed to set beacon filter opt: %d", ret);
		goto out;
	}

out:
	kfree(beacon_filter);
	return ret;
}

int wl1271_acx_beacon_filter_table(
				   struct wl12xx_vif *wlvif)
{
	struct acx_beacon_filter_ie_table *ie_table;
	int i, idx = 0;
	int ret;
	bool vendor_spec = false;

	wl1271_debug(DEBUG_ACX, "acx beacon filter table");

	ie_table = kzalloc(sizeof(*ie_table), GFP_KERNEL);
	if (!ie_table) {
		ret = -ENOMEM;
		goto out;
	}

	/* configure default beacon pass-through rules */
	ie_table->role_id = wlvif->role_id;
	ie_table->num_ie = 0;
	for (i = 0; i < wifi_data->conf.conn.bcn_filt_ie_count; i++) {
		struct conf_bcn_filt_rule *r = &(wifi_data->conf.conn.bcn_filt_ie[i]);
		ie_table->table[idx++] = r->ie;
		ie_table->table[idx++] = r->rule;

		if (r->ie == WLAN_EID_VENDOR_SPECIFIC) {
			/* only one vendor specific ie allowed */
			if (vendor_spec)
				continue;

			/* for vendor specific rules configure the
			   additional fields */
			memcpy(&(ie_table->table[idx]), r->oui,
			       CONF_BCN_IE_OUI_LEN);
			idx += CONF_BCN_IE_OUI_LEN;
			ie_table->table[idx++] = r->type;
			memcpy(&(ie_table->table[idx]), r->version,
			       CONF_BCN_IE_VER_LEN);
			idx += CONF_BCN_IE_VER_LEN;
			vendor_spec = true;
		}

		ie_table->num_ie++;
	}

	ret = VV_cmd_configure(ACX_BEACON_FILTER_TABLE,
				   ie_table, sizeof(*ie_table));
	if (ret < 0) {
		wl1271_warning("failed to set beacon filter table: %d", ret);
		goto out;
	}

out:
	kfree(ie_table);
	return ret;
}

#define ACX_CONN_MONIT_DISABLE_VALUE  0xffffffff

int wl1271_acx_conn_monit_params(struct wl12xx_vif *wlvif,
				 bool enable)
{
	struct acx_conn_monit_params *acx;
	u32 threshold = ACX_CONN_MONIT_DISABLE_VALUE;
	u32 timeout = ACX_CONN_MONIT_DISABLE_VALUE;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx connection monitor parameters: %s",
		     enable ? "enabled" : "disabled");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	if (enable) {
		threshold = wifi_data->conf.conn.synch_fail_thold;
		timeout = wifi_data->conf.conn.bss_lose_timeout;
	}

	acx->role_id = wlvif->role_id;
	acx->synch_fail_thold = cpu_to_le32(threshold);
	acx->bss_lose_timeout = cpu_to_le32(timeout);

	ret = VV_cmd_configure(ACX_CONN_MONIT_PARAMS,
				   acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("failed to set connection monitor "
			       "parameters: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}


int wl1271_acx_sg_enable(bool enable)
{
	struct acx_bt_wlan_coex *pta;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx sg enable");

	pta = kzalloc(sizeof(*pta), GFP_KERNEL);
	if (!pta) {
		ret = -ENOMEM;
		goto out;
	}

	if (enable)
		pta->enable = wifi_data->conf.sg.state;
	else
		pta->enable = CONF_SG_DISABLE;

	ret = VV_cmd_configure(ACX_SG_ENABLE, pta, sizeof(*pta));
	if (ret < 0) {
		wl1271_warning("failed to set softgemini enable: %d", ret);
		goto out;
	}

out:
	kfree(pta);
	return ret;
}

int wl12xx_acx_sg_cfg(void)
{
	struct acx_bt_wlan_coex_param *param;
	struct conf_sg_settings *c = &wifi_data->conf.sg;
	int i, ret;

	wl1271_debug(DEBUG_ACX, "acx sg cfg");

	param = kzalloc(sizeof(*param), GFP_KERNEL);
	if (!param) {
		ret = -ENOMEM;
		goto out;
	}

	/* BT-WLAN coext parameters */
	for (i = 0; i < WLCORE_CONF_SG_PARAMS_MAX; i++)
		param->params[i] = cpu_to_le32(c->params[i]);
	param->param_idx = WLCORE_CONF_SG_PARAMS_ALL;

	ret = VV_cmd_configure(ACX_SG_CFG, param, sizeof(*param));
	if (ret < 0) {
		wl1271_warning("failed to set sg config: %d", ret);
		goto out;
	}

out:
	kfree(param);
	return ret;
}

int wl1271_acx_cca_threshold(void)
{
	struct acx_energy_detection *detection;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx cca threshold");

	detection = kzalloc(sizeof(*detection), GFP_KERNEL);
	if (!detection) {
		ret = -ENOMEM;
		goto out;
	}

	detection->rx_cca_threshold = cpu_to_le16(wifi_data->conf.rx.rx_cca_threshold);
	detection->tx_energy_detection = wifi_data->conf.tx.tx_energy_detection;

	ret = VV_cmd_configure(ACX_CCA_THRESHOLD,
				   detection, sizeof(*detection));
	if (ret < 0)
		wl1271_warning("failed to set cca threshold: %d", ret);

out:
	kfree(detection);
	return ret;
}

int wl1271_acx_bcn_dtim_options(struct wl12xx_vif *wlvif)
{
	struct acx_beacon_broadcast *bb;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx bcn dtim options");

	bb = kzalloc(sizeof(*bb), GFP_KERNEL);
	if (!bb) {
		ret = -ENOMEM;
		goto out;
	}

	bb->role_id = wlvif->role_id;
	bb->beacon_rx_timeout = cpu_to_le16(wifi_data->conf.conn.beacon_rx_timeout);
	bb->broadcast_timeout = cpu_to_le16(wifi_data->conf.conn.broadcast_timeout);
	bb->rx_broadcast_in_ps = wifi_data->conf.conn.rx_broadcast_in_ps;
	bb->ps_poll_threshold = wifi_data->conf.conn.ps_poll_threshold;

	ret = VV_cmd_configure(ACX_BCN_DTIM_OPTIONS, bb, sizeof(*bb));
	if (ret < 0) {
		wl1271_warning("failed to set rx config: %d", ret);
		goto out;
	}

out:
	kfree(bb);
	return ret;
}

int wl1271_acx_aid(struct wl12xx_vif *wlvif, u16 aid)
{
	struct acx_aid *acx_aid;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx aid");

	acx_aid = kzalloc(sizeof(*acx_aid), GFP_KERNEL);
	if (!acx_aid) {
		ret = -ENOMEM;
		goto out;
	}

	acx_aid->role_id = wlvif->role_id;
	acx_aid->aid = cpu_to_le16(aid);

	ret = VV_cmd_configure(ACX_AID, acx_aid, sizeof(*acx_aid));
	if (ret < 0) {
		wl1271_warning("failed to set aid: %d", ret);
		goto out;
	}

out:
	kfree(acx_aid);
	return ret;
}

int wl1271_acx_event_mbox_mask(u32 event_mask)
{
	struct acx_event_mask *mask;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx event mbox mask");

	mask = kzalloc(sizeof(*mask), GFP_KERNEL);
	if (!mask) {
		ret = -ENOMEM;
		goto out;
	}

	/* high event mask is unused */
	mask->high_event_mask = cpu_to_le32(0xffffffff);
	mask->event_mask = cpu_to_le32(event_mask);

	ret = VV_cmd_configure(ACX_EVENT_MBOX_MASK,
				   mask, sizeof(*mask));
	if (ret < 0) {
		wl1271_warning("failed to set acx_event_mbox_mask: %d", ret);
		goto out;
	}

out:
	kfree(mask);
	return ret;
}

int wl1271_acx_set_preamble(struct wl12xx_vif *wlvif,
			    enum acx_preamble_type preamble)
{
	struct acx_preamble *acx;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx_set_preamble");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->preamble = preamble;

	ret = VV_cmd_configure(ACX_PREAMBLE_TYPE, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("Setting of preamble failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_cts_protect(struct wl12xx_vif *wlvif,
			   enum acx_ctsprotect_type ctsprotect)
{
	struct acx_ctsprotect *acx;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx_set_ctsprotect");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->ctsprotect = ctsprotect;

	ret = VV_cmd_configure(ACX_CTS_PROTECTION, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("Setting of ctsprotect failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_sta_rate_policies(struct wl12xx_vif *wlvif)
{
	struct acx_rate_policy *acx;
	struct conf_tx_rate_class *c = &wifi_data->conf.tx.sta_rc_conf;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx rate policies");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);

	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	wl1271_debug(DEBUG_ACX, "basic_rate: 0x%x, full_rate: 0x%x",
		wlvif->basic_rate, wlvif->rate_set);

	/* configure one basic rate class */
	acx->rate_policy_idx = cpu_to_le32(STA_BASIC_RATE_IDX);
	acx->rate_policy.enabled_rates = cpu_to_le32(wlvif->basic_rate);
	acx->rate_policy.short_retry_limit = c->short_retry_limit;
	acx->rate_policy.long_retry_limit = c->long_retry_limit;
	acx->rate_policy.aflags = c->aflags;

	ret = VV_cmd_configure(ACX_RATE_POLICY, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("Setting of rate policies failed: %d", ret);
		goto out;
	}

	/* configure one AP supported rate class */
	acx->rate_policy_idx = cpu_to_le32(STA_AP_RATE_IDX);

	/* the AP policy is HW specific */
	acx->rate_policy.enabled_rates = wlvif->rate_set;
		
	acx->rate_policy.short_retry_limit = c->short_retry_limit;
	acx->rate_policy.long_retry_limit = c->long_retry_limit;
	acx->rate_policy.aflags = c->aflags;

	ret = VV_cmd_configure(ACX_RATE_POLICY, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("Setting of rate policies failed: %d", ret);
		goto out;
	}

	/*
	 * configure one rate class for basic p2p operations.
	 * (p2p packets should always go out with OFDM rates, even
	 * if we are currently connected to 11b AP)
	 */
	acx->rate_policy_idx = cpu_to_le32(STA_P2P_RATE_IDX);
	acx->rate_policy.enabled_rates =
				cpu_to_le32(CONF_TX_RATE_MASK_BASIC_P2P);
	acx->rate_policy.short_retry_limit = c->short_retry_limit;
	acx->rate_policy.long_retry_limit = c->long_retry_limit;
	acx->rate_policy.aflags = c->aflags;

	ret = VV_cmd_configure(ACX_RATE_POLICY, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("Setting of rate policies failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_ac_cfg(struct wl12xx_vif *wlvif,
		      u8 ac, u8 cw_min, u16 cw_max, u8 aifsn, u16 txop)
{
	struct acx_ac_cfg *acx;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx ac cfg %d cw_ming %d cw_max %d "
		     "aifs %d txop %d", ac, cw_min, cw_max, aifsn, txop);

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);

	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->ac = ac;
	acx->cw_min = cw_min;
	acx->cw_max = cpu_to_le16(cw_max);
	acx->aifsn = aifsn;
	acx->tx_op_limit = cpu_to_le16(txop);

	ret = VV_cmd_configure(ACX_AC_CFG, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx ac cfg failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_tid_cfg(struct wl12xx_vif *wlvif,
		       u8 queue_id, u8 channel_type,
		       u8 tsid, u8 ps_scheme, u8 ack_policy,
		       u32 apsd_conf0, u32 apsd_conf1)
{
	struct acx_tid_config *acx;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx tid config");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);

	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->queue_id = queue_id;
	acx->channel_type = channel_type;
	acx->tsid = tsid;
	acx->ps_scheme = ps_scheme;
	acx->ack_policy = ack_policy;
	acx->apsd_conf[0] = cpu_to_le32(apsd_conf0);
	acx->apsd_conf[1] = cpu_to_le32(apsd_conf1);

	ret = VV_cmd_configure(ACX_TID_CFG, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("Setting of tid config failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_frag_threshold(u32 frag_threshold)
{
	struct acx_frag_threshold *acx;
	int ret = 0;

	/*
	 * If the fragmentation is not configured or out of range, use the
	 * default value.
	 */
	if (frag_threshold > IEEE80211_MAX_FRAG_THRESHOLD)
		frag_threshold = wifi_data->conf.tx.frag_threshold;

	wl1271_debug(DEBUG_ACX, "acx frag threshold: %d", frag_threshold);

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);

	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->frag_threshold = cpu_to_le16((u16)frag_threshold);
	ret = VV_cmd_configure(ACX_FRAG_CFG, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("Setting of frag threshold failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_tx_config_options(void)
{
	struct acx_tx_config_options *acx;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx tx config options");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);

	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->tx_compl_timeout = cpu_to_le16(wifi_data->conf.tx.tx_compl_timeout);
	acx->tx_compl_threshold = cpu_to_le16(wifi_data->conf.tx.tx_compl_threshold);
	ret = VV_cmd_configure(ACX_TX_CONFIG_OPT, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("Setting of tx options failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

#define WL18XX_NUM_TX_DESCRIPTORS 32
int wl12xx_acx_mem_cfg(void)
{
	struct wl12xx_acx_config_memory *mem_conf;
	struct conf_memory_settings *mem;
	int ret;

	wl1271_debug(DEBUG_ACX, "wl1271 mem cfg");

	mem_conf = kzalloc(sizeof(*mem_conf), GFP_KERNEL);
	if (!mem_conf) {
		ret = -ENOMEM;
		goto out;
	}

	mem = &wifi_data->conf.mem;

	/* memory config */
	mem_conf->num_stations = mem->num_stations;
	mem_conf->rx_mem_block_num = mem->rx_block_num;
	mem_conf->tx_min_mem_block_num = mem->tx_min_block_num;
	mem_conf->num_ssid_profiles = mem->ssid_profiles;
	mem_conf->total_tx_descriptors = cpu_to_le32(WL18XX_NUM_TX_DESCRIPTORS);
	mem_conf->dyn_mem_enable = mem->dynamic_memory;
	mem_conf->tx_free_req = mem->min_req_tx_blocks;
	mem_conf->rx_free_req = mem->min_req_rx_blocks;
	mem_conf->tx_min = mem->tx_min;
	mem_conf->fwlog_blocks = wifi_data->conf.fwlog.mem_blocks;

	ret = VV_cmd_configure(ACX_MEM_CFG, mem_conf,
				   sizeof(*mem_conf));
	if (ret < 0) {
		wl1271_warning("wl1271 mem config failed: %d", ret);
		goto out;
	}

out:
	kfree(mem_conf);
	return ret;
}
EXPORT_SYMBOL_GPL(wl12xx_acx_mem_cfg);

int wl1271_acx_init_mem_config(void)
{
	int ret;

	wifi_data->target_mem_map = kzalloc(sizeof(struct wl1271_acx_mem_map),
				     GFP_KERNEL);
	if (!wifi_data->target_mem_map) {
		wl1271_error("couldn't allocate target memory map");
		return -ENOMEM;
	}

	/* we now ask for the firmware built memory map */
	ret = wl1271_acx_mem_map((void *)wifi_data->target_mem_map,
				 sizeof(struct wl1271_acx_mem_map));
	if (ret < 0) {
		wl1271_error("couldn't retrieve firmware memory map");
		kfree(wifi_data->target_mem_map);
		wifi_data->target_mem_map = NULL;
		return ret;
	}

	/* initialize TX block book keeping */
	VV_tx_blocks_available =
		le32_to_cpu(wifi_data->target_mem_map->num_tx_mem_blocks);
	// wl1271_debug(DEBUG_TX, "available tx blocks: %d",
	// 	     VV_tx_blocks_available);
	printk("[INIT] available tx blocks: %d", VV_tx_blocks_available);

	return 0;
}
EXPORT_SYMBOL_GPL(wl1271_acx_init_mem_config);

int wl1271_acx_init_rx_interrupt(void)
{
	struct wl1271_acx_rx_config_opt *rx_conf;
	int ret;

	wl1271_debug(DEBUG_ACX, "wl1271 rx interrupt config");

	rx_conf = kzalloc(sizeof(*rx_conf), GFP_KERNEL);
	if (!rx_conf) {
		ret = -ENOMEM;
		goto out;
	}

	rx_conf->threshold = cpu_to_le16(wifi_data->conf.rx.irq_pkt_threshold);
	rx_conf->timeout = cpu_to_le16(wifi_data->conf.rx.irq_timeout);
	rx_conf->mblk_threshold = cpu_to_le16(wifi_data->conf.rx.irq_blk_threshold);
	rx_conf->queue_type = wifi_data->conf.rx.queue_type;

	ret = VV_cmd_configure(ACX_RX_CONFIG_OPT, rx_conf,
				   sizeof(*rx_conf));
	if (ret < 0) {
		wl1271_warning("wl1271 rx config opt failed: %d", ret);
		goto out;
	}

out:
	kfree(rx_conf);
	return ret;
}

int wl1271_acx_bet_enable(struct wl12xx_vif *wlvif,
			  bool enable)
{
	struct wl1271_acx_bet_enable *acx = NULL;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx bet enable");

	if (enable && wifi_data->conf.conn.bet_enable == CONF_BET_MODE_DISABLE)
		goto out;

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->enable = enable ? CONF_BET_MODE_ENABLE : CONF_BET_MODE_DISABLE;
	acx->max_consecutive = wifi_data->conf.conn.bet_max_consecutive;

	ret = VV_cmd_configure(ACX_BET_ENABLE, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx bet enable failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_arp_ip_filter(struct wl12xx_vif *wlvif,
			     u8 enable, __be32 address)
{
	struct wl1271_acx_arp_filter *acx;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx arp ip filter, enable: %d", enable);

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->version = ACX_IPV4_VERSION;
	acx->enable = enable;

	if (enable)
		memcpy(acx->address, &address, ACX_IPV4_ADDR_SIZE);

	ret = VV_cmd_configure(ACX_ARP_IP_FILTER,
				   acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("failed to set arp ip filter: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_pm_config(void)
{
	struct wl1271_acx_pm_config *acx = NULL;
	struct  conf_pm_config_settings *c = &wifi_data->conf.pm_config;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx pm config");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->host_clk_settling_time = cpu_to_le32(c->host_clk_settling_time);
	acx->host_fast_wakeup_support = c->host_fast_wakeup_support;

	ret = VV_cmd_configure(ACX_PM_CONFIG, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx pm config failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}
EXPORT_SYMBOL_GPL(wl1271_acx_pm_config);

int wl1271_acx_keep_alive_mode(struct wl12xx_vif *wlvif,
			       bool enable)
{
	struct wl1271_acx_keep_alive_mode *acx = NULL;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx keep alive mode: %d", enable);

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->enabled = enable;

	ret = VV_cmd_configure(ACX_KEEP_ALIVE_MODE, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx keep alive mode failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_keep_alive_config(struct wl12xx_vif *wlvif,
				 u8 index, u8 tpl_valid)
{
	struct wl1271_acx_keep_alive_config *acx = NULL;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx keep alive config");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->period = cpu_to_le32(wifi_data->conf.conn.keep_alive_interval);
	acx->index = index;
	acx->tpl_validation = tpl_valid;
	acx->trigger = ACX_KEEP_ALIVE_NO_TX;

	ret = VV_cmd_configure(ACX_SET_KEEP_ALIVE_CONFIG,
				   acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx keep alive config failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_rssi_snr_trigger(struct wl12xx_vif *wlvif,
				bool enable, s16 thold, u8 hyst)
{
	struct wl1271_acx_rssi_snr_trigger *acx = NULL;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx rssi snr trigger");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	//wlvif->last_rssi_event = -1;

	acx->role_id = wlvif->role_id;
	acx->pacing = cpu_to_le16(wifi_data->conf.roam_trigger.trigger_pacing);
	acx->metric = WL1271_ACX_TRIG_METRIC_RSSI_BEACON;
	acx->type = WL1271_ACX_TRIG_TYPE_EDGE;
	if (enable)
		acx->enable = WL1271_ACX_TRIG_ENABLE;
	else
		acx->enable = WL1271_ACX_TRIG_DISABLE;

	acx->index = WL1271_ACX_TRIG_IDX_RSSI;
	acx->dir = WL1271_ACX_TRIG_DIR_BIDIR;
	acx->threshold = cpu_to_le16(thold);
	acx->hysteresis = hyst;

	ret = VV_cmd_configure(ACX_RSSI_SNR_TRIGGER, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx rssi snr trigger setting failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_rssi_snr_avg_weights(
				    struct wl12xx_vif *wlvif)
{
	struct wl1271_acx_rssi_snr_avg_weights *acx = NULL;
	struct conf_roam_trigger_settings *c = &wifi_data->conf.roam_trigger;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx rssi snr avg weights");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->rssi_beacon = c->avg_weight_rssi_beacon;
	acx->rssi_data = c->avg_weight_rssi_data;
	acx->snr_beacon = c->avg_weight_snr_beacon;
	acx->snr_data = c->avg_weight_snr_data;

	ret = VV_cmd_configure(ACX_RSSI_SNR_WEIGHTS, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx rssi snr trigger weights failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_set_ht_information(
				   struct wl12xx_vif *wlvif,
				   u16 ht_operation_mode)
{
	struct wl1271_acx_ht_information *acx;
	int ret = 0;

	wl1271_debug(DEBUG_ACX, "acx ht information setting");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->role_id = wlvif->role_id;
	acx->ht_protection =
		(u8)(ht_operation_mode & IEEE80211_HT_OP_MODE_PROTECTION);
	acx->rifs_mode = 0;
	acx->gf_protection =
		!!(ht_operation_mode & IEEE80211_HT_OP_MODE_NON_GF_STA_PRSNT);
	acx->ht_tx_burst_limit = 0;
	acx->dual_cts_protection = 0;

	ret = VV_cmd_configure(ACX_HT_BSS_OPERATION, acx, sizeof(*acx));

	if (ret < 0) {
		wl1271_warning("acx ht information setting failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

/* Configure BA session initiator/receiver parameters setting in the FW. */
int wl12xx_acx_set_ba_initiator_policy(
				       struct wl12xx_vif *wlvif)
{
	struct wl1271_acx_ba_initiator_policy *acx;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx ba initiator policy");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	/* set for the current role */
	acx->role_id = wlvif->role_id;
	acx->tid_bitmap = wifi_data->conf.ht.tx_ba_tid_bitmap;
	acx->win_size = wifi_data->conf.ht.tx_ba_win_size;
	acx->inactivity_timeout = wifi_data->conf.ht.inactivity_timeout;

	ret = VV_cmd_configure(
				   ACX_BA_SESSION_INIT_POLICY,
				   acx,
				   sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx ba initiator policy failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl1271_acx_ps_rx_streaming(struct wl12xx_vif *wlvif,
			       bool enable)
{
	struct wl1271_acx_ps_rx_streaming *rx_streaming;
	u32 conf_queues, enable_queues;
	int i, ret = 0;

	wl1271_debug(DEBUG_ACX, "acx ps rx streaming");

	rx_streaming = kzalloc(sizeof(*rx_streaming), GFP_KERNEL);
	if (!rx_streaming) {
		ret = -ENOMEM;
		goto out;
	}

	conf_queues = wifi_data->conf.rx_streaming.queues;
	if (enable)
		enable_queues = conf_queues;
	else
		enable_queues = 0;

	for (i = 0; i < 8; i++) {
		/*
		 * Skip non-changed queues, to avoid redundant acxs.
		 * this check assumes conf.rx_streaming.queues can't
		 * be changed while rx_streaming is enabled.
		 */
		if (!(conf_queues & BIT(i)))
			continue;

		rx_streaming->role_id = wlvif->role_id;
		rx_streaming->tid = i;
		rx_streaming->enable = enable_queues & BIT(i);
		rx_streaming->period = wifi_data->conf.rx_streaming.interval;
		rx_streaming->timeout = wifi_data->conf.rx_streaming.interval;

		ret = VV_cmd_configure(ACX_PS_RX_STREAMING,
					   rx_streaming,
					   sizeof(*rx_streaming));
		if (ret < 0) {
			wl1271_warning("acx ps rx streaming failed: %d", ret);
			goto out;
		}
	}
out:
	kfree(rx_streaming);
	return ret;
}

int wl1271_acx_fm_coex(void)
{
	struct wl1271_acx_fm_coex *acx;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx fm coex setting");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->enable = wifi_data->conf.fm_coex.enable;
	acx->swallow_period = wifi_data->conf.fm_coex.swallow_period;
	acx->n_divider_fref_set_1 = wifi_data->conf.fm_coex.n_divider_fref_set_1;
	acx->n_divider_fref_set_2 = wifi_data->conf.fm_coex.n_divider_fref_set_2;
	acx->m_divider_fref_set_1 =
		cpu_to_le16(wifi_data->conf.fm_coex.m_divider_fref_set_1);
	acx->m_divider_fref_set_2 =
		cpu_to_le16(wifi_data->conf.fm_coex.m_divider_fref_set_2);
	acx->coex_pll_stabilization_time =
		cpu_to_le32(wifi_data->conf.fm_coex.coex_pll_stabilization_time);
	acx->ldo_stabilization_time =
		cpu_to_le16(wifi_data->conf.fm_coex.ldo_stabilization_time);
	acx->fm_disturbed_band_margin =
		wifi_data->conf.fm_coex.fm_disturbed_band_margin;
	acx->swallow_clk_diff = wifi_data->conf.fm_coex.swallow_clk_diff;

	ret = VV_cmd_configure(ACX_FM_COEX_CFG, acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx fm coex setting failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl12xx_acx_set_rate_mgmt_params(void)
{
	struct wl12xx_acx_set_rate_mgmt_params *acx = NULL;
	struct conf_rate_policy_settings *conf = &wifi_data->conf.rate;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx set rate mgmt params");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx)
		return -ENOMEM;

	acx->index = ACX_RATE_MGMT_ALL_PARAMS;
	acx->rate_retry_score = cpu_to_le16(conf->rate_retry_score);
	acx->per_add = cpu_to_le16(conf->per_add);
	acx->per_th1 = cpu_to_le16(conf->per_th1);
	acx->per_th2 = cpu_to_le16(conf->per_th2);
	acx->max_per = cpu_to_le16(conf->max_per);
	acx->inverse_curiosity_factor = conf->inverse_curiosity_factor;
	acx->tx_fail_low_th = conf->tx_fail_low_th;
	acx->tx_fail_high_th = conf->tx_fail_high_th;
	acx->per_alpha_shift = conf->per_alpha_shift;
	acx->per_add_shift = conf->per_add_shift;
	acx->per_beta1_shift = conf->per_beta1_shift;
	acx->per_beta2_shift = conf->per_beta2_shift;
	acx->rate_check_up = conf->rate_check_up;
	acx->rate_check_down = conf->rate_check_down;
	memcpy(acx->rate_retry_policy, conf->rate_retry_policy,
	       sizeof(acx->rate_retry_policy));

	ret = VV_cmd_configure(ACX_SET_RATE_MGMT_PARAMS,
				   acx, sizeof(*acx));
	if (ret < 0) {
		wl1271_warning("acx set rate mgmt params failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;
}

int wl12xx_acx_config_hangover(void)
{
	struct wl12xx_acx_config_hangover *acx;
	struct conf_hangover_settings *conf = &wifi_data->conf.hangover;
	int ret;

	wl1271_debug(DEBUG_ACX, "acx config hangover");

	acx = kzalloc(sizeof(*acx), GFP_KERNEL);
	if (!acx) {
		ret = -ENOMEM;
		goto out;
	}

	acx->recover_time = cpu_to_le32(conf->recover_time);
	acx->hangover_period = conf->hangover_period;
	acx->dynamic_mode = conf->dynamic_mode;
	acx->early_termination_mode = conf->early_termination_mode;
	acx->max_period = conf->max_period;
	acx->min_period = conf->min_period;
	acx->increase_delta = conf->increase_delta;
	acx->decrease_delta = conf->decrease_delta;
	acx->quiet_time = conf->quiet_time;
	acx->increase_time = conf->increase_time;
	acx->window_size = conf->window_size;

	ret = VV_cmd_configure(ACX_CONFIG_HANGOVER, acx,
				   sizeof(*acx));

	if (ret < 0) {
		wl1271_warning("acx config hangover failed: %d", ret);
		goto out;
	}

out:
	kfree(acx);
	return ret;

}

