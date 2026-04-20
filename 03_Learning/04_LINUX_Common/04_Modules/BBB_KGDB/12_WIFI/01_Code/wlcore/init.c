// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "debug.h"
#include "init.h"
#include "wl12xx_80211.h"
#include "acx.h"
#include "cmd.h"
#include "tx.h"
#include "io.h"
#include "hw_ops.h"

#define WL18XX_CMD_MAX_SIZE 740
int VV_cmd_send1(struct wl1271 *wl, u16 id, void *buf,
			     size_t len, size_t res_len)
{
	struct wl1271_cmd_header *cmd;
	unsigned long timeout;
	u32 intr;
	int ret;
	u16 status;
	u8 cmd_max[WL18XX_CMD_MAX_SIZE];

	cmd = buf;
	cmd->id = cpu_to_le16(id);
	cmd->status = 0;

	// ret = wlcore_write(wl, wl->cmd_box_addr, buf, len, false);
	ret = VV_sdio_raw_write(wl, wlcore_translate_addr(wl, wl->cmd_box_addr), buf, len, false);
	if (ret < 0)
		return ret;

	memcpy(cmd_max, buf, len);
	memset(cmd_max + len, 0, WL18XX_CMD_MAX_SIZE - len);

	// wlcore_write(wl, wl->cmd_box_addr, priv->cmd_buf,
	// 		    WL18XX_CMD_MAX_SIZE, false);
	ret = VV_sdio_raw_write1(wl, wlcore_translate_addr(wl, wl->cmd_box_addr), cmd_max, WL18XX_CMD_MAX_SIZE, false);


	timeout = jiffies + msecs_to_jiffies(WL1271_COMMAND_TIMEOUT);
	//ret = wlcore_read_reg(wl, REG_INTERRUPT_NO_CLEAR, &intr);
	ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, wl->rtable[REG_INTERRUPT_NO_CLEAR]), &intr, sizeof(intr), false);
	if (ret < 0)
		return ret;

	while (!(intr & WL1271_ACX_INTR_CMD_COMPLETE)) {
		if (time_after(jiffies, timeout)) {
			wl1271_error("command complete timeout");
			return -ETIMEDOUT;
		}
		//ret = wlcore_read_reg(wl, REG_INTERRUPT_NO_CLEAR, &intr);
		ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, wl->rtable[REG_INTERRUPT_NO_CLEAR]), &intr, sizeof(intr), false);
		if (ret < 0)
			return ret;
	}

	/* read back the status code of the command */
	if (res_len == 0)
		res_len = sizeof(struct wl1271_cmd_header);

	//ret = wlcore_read(wl, wl->cmd_box_addr, cmd, res_len, false);
	ret = VV_sdio_raw_read(wl, wlcore_translate_addr(wl, wl->cmd_box_addr), (u32*)cmd, sizeof(*cmd), false);
	if (ret < 0)
		return ret;
	status = le16_to_cpu(cmd->status);
	// ret = wlcore_write_reg(wl, REG_INTERRUPT_ACK,
	// 		       WL1271_ACX_INTR_CMD_COMPLETE);
	ret = VV_sdio_raw_write(wl, wlcore_translate_addr(wl, wl->rtable[REG_INTERRUPT_ACK]), 
				WL1271_ACX_INTR_CMD_COMPLETE, sizeof(WL1271_ACX_INTR_CMD_COMPLETE), false);
	if (ret < 0)
		return ret;

	return status;
}

int VV_cmd_template_set(struct wl1271 *wl, u8 role_id,
			    u16 template_id, void *buf, size_t buf_len,
			    int index, u32 rates)
{
	struct wl1271_cmd_template_set cmd;
	int ret = 0;

	buf_len = min_t(size_t, buf_len, WL1271_CMD_TEMPL_MAX_SIZE);

	/* during initialization wlvif is NULL */
	cmd.role_id = role_id;
	cmd.len = cpu_to_le16(buf_len);
	cmd.template_type = template_id;
	cmd.enabled_rates = cpu_to_le32(rates);
	cmd.short_retry_limit = wl->conf.tx.tmpl_short_retry_limit;
	cmd.long_retry_limit = wl->conf.tx.tmpl_long_retry_limit;
	cmd.index = index;

	if (buf)
		memcpy(cmd.template_data, buf, buf_len);

	//ret = wl1271_cmd_send(wl, CMD_SET_TEMPLATE, &cmd, sizeof(cmd), 0);
	ret = wl1271_cmd_send1(wl, CMD_SET_TEMPLATE, &cmd, sizeof(cmd), 0);
	printk("[TEMPLATE] - ret = %d\n",ret);
	return ret;

// 	struct wl1271_cmd_template_set *cmd;
// 	int ret = 0;

// 	wl1271_debug(DEBUG_CMD, "cmd template_set %d (role %d)",
// 		     template_id, role_id);

// 	WARN_ON(buf_len > WL1271_CMD_TEMPL_MAX_SIZE);
// 	buf_len = min_t(size_t, buf_len, WL1271_CMD_TEMPL_MAX_SIZE);

// 	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
// 	if (!cmd) {
// 		ret = -ENOMEM;
// 		goto out;
// 	}

// 	/* during initialization wlvif is NULL */
// 	cmd->role_id = role_id;
// 	cmd->len = cpu_to_le16(buf_len);
// 	cmd->template_type = template_id;
// 	cmd->enabled_rates = cpu_to_le32(rates);
// 	cmd->short_retry_limit = wl->conf.tx.tmpl_short_retry_limit;
// 	cmd->long_retry_limit = wl->conf.tx.tmpl_long_retry_limit;
// 	cmd->index = index;

// 	if (buf)
// 		memcpy(cmd->template_data, buf, buf_len);

// 	ret = wl1271_cmd_send(wl, CMD_SET_TEMPLATE, cmd, sizeof(*cmd), 0);
// 	if (ret < 0) {
// 		wl1271_warning("cmd set_template failed: %d", ret);
// 		goto out_free;
// 	}

// out_free:
// 	kfree(cmd);

// out:
// 	return ret;
}

int wl1271_init_templates_config(struct wl1271 *wl)
{
	int ret, i;
	size_t max_size;

	/* send empty templates for fw memory reservation */
	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      wl->scan_templ_id_2_4, NULL,
				      WL1271_CMD_TEMPL_MAX_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      wl->scan_templ_id_5,
				      NULL, WL1271_CMD_TEMPL_MAX_SIZE, 0,
				      WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	if (wl->quirks & WLCORE_QUIRK_DUAL_PROBE_TMPL) {
		ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
					      wl->sched_scan_templ_id_2_4,
					      NULL,
					      WL1271_CMD_TEMPL_MAX_SIZE,
					      0, WL1271_RATE_AUTOMATIC);
		if (ret < 0)
			return ret;

		ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
					      wl->sched_scan_templ_id_5,
					      NULL,
					      WL1271_CMD_TEMPL_MAX_SIZE,
					      0, WL1271_RATE_AUTOMATIC);
		if (ret < 0)
			return ret;
	}

	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_NULL_DATA, NULL,
				      sizeof(struct wl12xx_null_data_template),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_PS_POLL, NULL,
				      sizeof(struct wl12xx_ps_poll_template),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_QOS_NULL_DATA, NULL,
				      sizeof
				      (struct ieee80211_qos_hdr),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_PROBE_RESPONSE, NULL,
				      WL1271_CMD_TEMPL_DFLT_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_BEACON, NULL,
				      WL1271_CMD_TEMPL_DFLT_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	max_size = sizeof(struct wl12xx_arp_rsp_template) +
		   WL1271_EXTRA_SPACE_MAX;
	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_ARP_RSP, NULL,
				      max_size,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	/*
	 * Put very large empty placeholders for all templates. These
	 * reserve memory for later.
	 */
	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_AP_PROBE_RESPONSE, NULL,
				      WL1271_CMD_TEMPL_MAX_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_AP_BEACON, NULL,
				      WL1271_CMD_TEMPL_MAX_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_DEAUTH_AP, NULL,
				      sizeof
				      (struct wl12xx_disconn_template),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	for (i = 0; i < WLCORE_MAX_KLV_TEMPLATES; i++) {
		ret = wl1271_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
					      CMD_TEMPL_KLV, NULL,
					      sizeof(struct ieee80211_qos_hdr),
					      i, WL1271_RATE_AUTOMATIC);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int VV_init_templates_config(struct wl1271 *wl)
{
	int ret, i;
	size_t max_size;

	/* send empty templates for fw memory reservation */
	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      wl->scan_templ_id_2_4, NULL,
				      WL1271_CMD_TEMPL_MAX_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      wl->scan_templ_id_5,
				      NULL, WL1271_CMD_TEMPL_MAX_SIZE, 0,
				      WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	if (wl->quirks & WLCORE_QUIRK_DUAL_PROBE_TMPL) {
		ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
					      wl->sched_scan_templ_id_2_4,
					      NULL,
					      WL1271_CMD_TEMPL_MAX_SIZE,
					      0, WL1271_RATE_AUTOMATIC);
		if (ret < 0)
			return ret;

		ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
					      wl->sched_scan_templ_id_5,
					      NULL,
					      WL1271_CMD_TEMPL_MAX_SIZE,
					      0, WL1271_RATE_AUTOMATIC);
		if (ret < 0)
			return ret;
	}

	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_NULL_DATA, NULL,
				      sizeof(struct wl12xx_null_data_template),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_PS_POLL, NULL,
				      sizeof(struct wl12xx_ps_poll_template),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_QOS_NULL_DATA, NULL,
				      sizeof
				      (struct ieee80211_qos_hdr),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_PROBE_RESPONSE, NULL,
				      WL1271_CMD_TEMPL_DFLT_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_BEACON, NULL,
				      WL1271_CMD_TEMPL_DFLT_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	max_size = sizeof(struct wl12xx_arp_rsp_template) +
		   WL1271_EXTRA_SPACE_MAX;
	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_ARP_RSP, NULL,
				      max_size,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	/*
	 * Put very large empty placeholders for all templates. These
	 * reserve memory for later.
	 */
	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_AP_PROBE_RESPONSE, NULL,
				      WL1271_CMD_TEMPL_MAX_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_AP_BEACON, NULL,
				      WL1271_CMD_TEMPL_MAX_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_DEAUTH_AP, NULL,
				      sizeof
				      (struct wl12xx_disconn_template),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	for (i = 0; i < WLCORE_MAX_KLV_TEMPLATES; i++) {
		ret = VV_cmd_template_set(wl, WL12XX_INVALID_ROLE_ID,
					      CMD_TEMPL_KLV, NULL,
					      sizeof(struct ieee80211_qos_hdr),
					      i, WL1271_RATE_AUTOMATIC);
		if (ret < 0)
			return ret;
	}

	return 0;
}


static int wl1271_ap_init_deauth_template(struct wl1271 *wl,
					  struct wl12xx_vif *wlvif)
{
	struct wl12xx_disconn_template *tmpl;
	int ret;
	u32 rate;

	tmpl = kzalloc(sizeof(*tmpl), GFP_KERNEL);
	if (!tmpl) {
		ret = -ENOMEM;
		goto out;
	}

	tmpl->header.frame_ctl = cpu_to_le16(IEEE80211_FTYPE_MGMT |
					     IEEE80211_STYPE_DEAUTH);

	rate = wl1271_tx_min_rate_get(wl, wlvif->basic_rate_set);
	ret = wl1271_cmd_template_set(wl, wlvif->role_id,
				      CMD_TEMPL_DEAUTH_AP,
				      tmpl, sizeof(*tmpl), 0, rate);

out:
	kfree(tmpl);
	return ret;
}

static int wl1271_ap_init_null_template(struct wl1271 *wl,
					struct ieee80211_vif *vif)
{
	struct wl12xx_vif *wlvif = wl12xx_vif_to_data(vif);
	struct ieee80211_hdr_3addr *nullfunc;
	int ret;
	u32 rate;

	nullfunc = kzalloc(sizeof(*nullfunc), GFP_KERNEL);
	if (!nullfunc) {
		ret = -ENOMEM;
		goto out;
	}

	nullfunc->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA |
					      IEEE80211_STYPE_NULLFUNC |
					      IEEE80211_FCTL_FROMDS);

	/* nullfunc->addr1 is filled by FW */

	memcpy(nullfunc->addr2, vif->addr, ETH_ALEN);
	memcpy(nullfunc->addr3, vif->addr, ETH_ALEN);

	rate = wl1271_tx_min_rate_get(wl, wlvif->basic_rate_set);
	ret = wl1271_cmd_template_set(wl, wlvif->role_id,
				      CMD_TEMPL_NULL_DATA, nullfunc,
				      sizeof(*nullfunc), 0, rate);

out:
	kfree(nullfunc);
	return ret;
}

static int wl1271_ap_init_qos_null_template(struct wl1271 *wl,
					    struct ieee80211_vif *vif)
{
	struct wl12xx_vif *wlvif = wl12xx_vif_to_data(vif);
	struct ieee80211_qos_hdr *qosnull;
	int ret;
	u32 rate;

	qosnull = kzalloc(sizeof(*qosnull), GFP_KERNEL);
	if (!qosnull) {
		ret = -ENOMEM;
		goto out;
	}

	qosnull->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA |
					     IEEE80211_STYPE_QOS_NULLFUNC |
					     IEEE80211_FCTL_FROMDS);

	/* qosnull->addr1 is filled by FW */

	memcpy(qosnull->addr2, vif->addr, ETH_ALEN);
	memcpy(qosnull->addr3, vif->addr, ETH_ALEN);

	rate = wl1271_tx_min_rate_get(wl, wlvif->basic_rate_set);
	ret = wl1271_cmd_template_set(wl, wlvif->role_id,
				      CMD_TEMPL_QOS_NULL_DATA, qosnull,
				      sizeof(*qosnull), 0, rate);

out:
	kfree(qosnull);
	return ret;
}

static int wl12xx_init_rx_config(struct wl1271 *wl)
{
	int ret;

	ret = wl1271_acx_rx_msdu_life_time(wl);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl12xx_init_phy_vif_config(struct wl1271 *wl,
					    struct wl12xx_vif *wlvif)
{
	int ret;

	//ret = wl1271_acx_slot(wl, wlvif, DEFAULT_SLOT_TIME);
	struct acx_slot slot;
	slot.role_id = wlvif->role_id;
	slot.wone_index = STATION_WONE_INDEX;
	slot.slot_time = DEFAULT_SLOT_TIME;
	ret = VV_cmd_configure(wl, ACX_SLOT, &slot, sizeof(slot), 0);
	if (ret < 0)
		return ret;

	// ret = wl1271_acx_service_period_timeout(wl, wlvif);
	struct acx_rx_timeout rx_timeout;
	rx_timeout.role_id = wlvif->role_id;
	rx_timeout.ps_poll_timeout = cpu_to_le16(wl->conf.rx.ps_poll_timeout);
	rx_timeout.upsd_timeout = cpu_to_le16(wl->conf.rx.upsd_timeout);
	ret = VV_cmd_configure(wl, ACX_SERVICE_PERIOD_TIMEOUT, &rx_timeout, sizeof(rx_timeout), 0);
	if (ret < 0)
		return ret;

	//ret = wl1271_acx_rts_threshold(wl, wlvif, wl->hw->wiphy->rts_threshold);
	struct acx_rts_threshold rts;
	/*
	 * If the RTS threshold is not configured or out of range, use the
	 * default value.
	 */
	rts.threshold = cpu_to_le16((u16)wl->hw->wiphy->rts_threshold);
	if (rts.threshold > IEEE80211_MAX_RTS_THRESHOLD)
		rts.threshold = wl->conf.rx.rts_threshold;
	rts.role_id = wlvif->role_id;
	ret = VV_cmd_configure(wl, DOT11_RTS_THRESHOLD, &rts, sizeof(rts), 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_init_sta_beacon_filter(struct wl1271 *wl,
					 struct wl12xx_vif *wlvif)
{
	int ret;

	// ret = wl1271_acx_beacon_filter_table(wl, wlvif);
	struct acx_beacon_filter_ie_table ie_table;
	int i, idx = 0;
	bool vendor_spec = false;

	/* configure default beacon pass-through rules */
	ie_table.role_id = wlvif->role_id;
	ie_table.num_ie = 0;
	for (i = 0; i < wl->conf.conn.bcn_filt_ie_count; i++) {
		struct conf_bcn_filt_rule *r = &(wl->conf.conn.bcn_filt_ie[i]);
		ie_table.table[idx++] = r->ie;
		ie_table.table[idx++] = r->rule;

		if (r->ie == WLAN_EID_VENDOR_SPECIFIC) {
			/* only one vendor specific ie allowed */
			if (vendor_spec)
				continue;

			/* for vendor specific rules configure the
			   additional fields */
			memcpy(&(ie_table.table[idx]), r->oui,
			       CONF_BCN_IE_OUI_LEN);
			idx += CONF_BCN_IE_OUI_LEN;
			ie_table.table[idx++] = r->type;
			memcpy(&(ie_table.table[idx]), r->version,
			       CONF_BCN_IE_VER_LEN);
			idx += CONF_BCN_IE_VER_LEN;
			vendor_spec = true;
		}

		ie_table.num_ie++;
	}
	
	ret = VV_cmd_configure(wl, ACX_BEACON_FILTER_TABLE, &ie_table, sizeof(ie_table), 0);
	if (ret < 0)
		return ret;


	/* disable beacon filtering until we get the first beacon */
	// ret = wl1271_acx_beacon_filter_opt(wl, wlvif, false);
	struct acx_beacon_filter_option beacon_filter;
	beacon_filter.role_id = wlvif->role_id;
	beacon_filter.enable = false;
	/*
	 * When set to zero, and the filter is enabled, beacons
	 * without the unicast TIM bit set are dropped.
	 */
	beacon_filter.max_num_beacons = 0;
	
	ret = VV_cmd_configure(wl, ACX_BEACON_FILTER_OPT, &beacon_filter, sizeof(beacon_filter), 0);
	if (ret < 0)
		return ret;

	return 0;
}

int wl1271_init_pta(struct wl1271 *wl)
{
	int ret, i;

	// ret = wl12xx_acx_sg_cfg(wl);
	struct acx_bt_wlan_coex_param param;
	struct conf_sg_settings *c = &wl->conf.sg;
	/* BT-WLAN coext parameters */
	for (i = 0; i < WLCORE_CONF_SG_PARAMS_MAX; i++)
		param.params[i] = cpu_to_le32(c->params[i]);
	param.param_idx = WLCORE_CONF_SG_PARAMS_ALL;
	ret = VV_cmd_configure(wl, ACX_SG_CFG, &param, sizeof(param), 0);
	if (ret < 0)
		return ret;

	// ret = wl1271_acx_sg_enable(wl, wl->sg_enabled);
	struct acx_bt_wlan_coex pta;
	if (wl->sg_enabled)
		pta.enable = wl->conf.sg.state;
	else
		pta.enable = CONF_SG_DISABLE;
	ret = VV_cmd_configure(wl, ACX_SG_ENABLE, &pta, sizeof(pta), 0);
	if (ret < 0)
		return ret;

	return 0;
}

int wl1271_init_energy_detection(struct wl1271 *wl)
{
	int ret;

	ret = wl1271_acx_cca_threshold(wl);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_init_beacon_broadcast(struct wl1271 *wl,
					struct wl12xx_vif *wlvif)
{
	int ret;

	ret = wl1271_acx_bcn_dtim_options(wl, wlvif);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl12xx_init_fwlog(struct wl1271 *wl)
{
	int ret;

	if (wl->quirks & WLCORE_QUIRK_FWLOG_NOT_IMPLEMENTED)
		return 0;

	ret = wl12xx_cmd_config_fwlog(wl);
	if (ret < 0)
		return ret;

	return 0;
}

/* generic sta initialization (non vif-specific) */
int wl1271_sta_hw_init(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	int ret;

	/* PS config */
	//ret = wl12xx_acx_config_ps(wl, wlvif);
	struct wl1271_acx_config_ps config_ps;
	config_ps.exit_retries = wl->conf.conn.psm_exit_retries;
	config_ps.enter_retries = wl->conf.conn.psm_entry_retries;
	config_ps.null_data_rate = cpu_to_le32(wlvif->basic_rate);
	ret = VV_cmd_configure(wl, ACX_CONFIG_PS, &config_ps, sizeof(config_ps), 0);
	if (ret < 0)
		return ret;

	/* FM WLAN coexistence */
	//ret = wl1271_acx_fm_coex(wl);
	struct wl1271_acx_fm_coex acx;
	acx.enable = wl->conf.fm_coex.enable;
	acx.swallow_period = wl->conf.fm_coex.swallow_period;
	acx.n_divider_fref_set_1 = wl->conf.fm_coex.n_divider_fref_set_1;
	acx.n_divider_fref_set_2 = wl->conf.fm_coex.n_divider_fref_set_2;
	acx.m_divider_fref_set_1 =
		cpu_to_le16(wl->conf.fm_coex.m_divider_fref_set_1);
	acx.m_divider_fref_set_2 =
		cpu_to_le16(wl->conf.fm_coex.m_divider_fref_set_2);
	acx.coex_pll_stabilization_time =
		cpu_to_le32(wl->conf.fm_coex.coex_pll_stabilization_time);
	acx.ldo_stabilization_time =
		cpu_to_le16(wl->conf.fm_coex.ldo_stabilization_time);
	acx.fm_disturbed_band_margin =
		wl->conf.fm_coex.fm_disturbed_band_margin;
	acx.swallow_clk_diff = wl->conf.fm_coex.swallow_clk_diff;
	ret = VV_cmd_configure(wl, ACX_FM_COEX_CFG, &acx, sizeof(acx), 0);
	if (ret < 0)
		return ret;

	ret = wl1271_acx_sta_rate_policies(wl, wlvif);
	struct acx_rate_policy acxrp;
	struct conf_tx_rate_class *c = &wl->conf.tx.sta_rc_conf;

	/* configure one basic rate class */
	acxrp.rate_policy_idx = cpu_to_le32(wlvif->sta.basic_rate_idx);
	acxrp.rate_policy.enabled_rates = cpu_to_le32(wlvif->basic_rate);
	acxrp.rate_policy.short_retry_limit = c->short_retry_limit;
	acxrp.rate_policy.long_retry_limit = c->long_retry_limit;
	acxrp.rate_policy.aflags = c->aflags;
	
	ret = VV_cmd_configure(wl, ACX_FM_COEX_CFG, &acxrp, sizeof(acxrp), 0);
	if (ret < 0)
		return ret;

	/* configure one AP supported rate class */
	acxrp.rate_policy_idx = cpu_to_le32(wlvif->sta.ap_rate_idx);

	/* the AP policy is HW specific */
	acxrp.rate_policy.enabled_rates =
		cpu_to_le32(wlcore_hw_sta_get_ap_rate_mask(wl, wlvif));
	acxrp.rate_policy.short_retry_limit = c->short_retry_limit;
	acxrp.rate_policy.long_retry_limit = c->long_retry_limit;
	acxrp.rate_policy.aflags = c->aflags;
	ret = VV_cmd_configure(wl, ACX_FM_COEX_CFG, &acxrp, sizeof(acxrp), 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_sta_hw_init_post_mem(struct wl1271 *wl,
				       struct ieee80211_vif *vif)
{
	struct wl12xx_vif *wlvif = wl12xx_vif_to_data(vif);
	int ret;

	/* disable the keep-alive feature */
	ret = wl1271_acx_keep_alive_mode(wl, wlvif, false);
	if (ret < 0)
		return ret;

	return 0;
}

/* generic ap initialization (non vif-specific) */
static int wl1271_ap_hw_init(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	int ret;

	ret = wl1271_init_ap_rates(wl, wlvif);
	if (ret < 0)
		return ret;

	/* configure AP sleep, if enabled */
	ret = wlcore_hw_ap_sleep(wl);
	if (ret < 0)
		return ret;

	return 0;
}

int wl1271_ap_init_templates(struct wl1271 *wl, struct ieee80211_vif *vif)
{
	struct wl12xx_vif *wlvif = wl12xx_vif_to_data(vif);
	int ret;

	ret = wl1271_ap_init_deauth_template(wl, wlvif);
	if (ret < 0)
		return ret;

	ret = wl1271_ap_init_null_template(wl, vif);
	if (ret < 0)
		return ret;

	ret = wl1271_ap_init_qos_null_template(wl, vif);
	if (ret < 0)
		return ret;

	/*
	 * when operating as AP we want to receive external beacons for
	 * configuring ERP protection.
	 */
	ret = wl1271_acx_beacon_filter_opt(wl, wlvif, false);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_ap_hw_init_post_mem(struct wl1271 *wl,
				      struct ieee80211_vif *vif)
{
	return wl1271_ap_init_templates(wl, vif);
}

int wl1271_init_ap_rates(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	int i, ret;
	struct conf_tx_rate_class rc;
	u32 supported_rates;

	wl1271_debug(DEBUG_AP, "AP basic rate set: 0x%x",
		     wlvif->basic_rate_set);

	if (wlvif->basic_rate_set == 0)
		return -EINVAL;

	rc.enabled_rates = wlvif->basic_rate_set;
	rc.long_retry_limit = 10;
	rc.short_retry_limit = 10;
	rc.aflags = 0;
	ret = wl1271_acx_ap_rate_policy(wl, &rc, wlvif->ap.mgmt_rate_idx);
	if (ret < 0)
		return ret;

	/* use the min basic rate for AP broadcast/multicast */
	rc.enabled_rates = wl1271_tx_min_rate_get(wl, wlvif->basic_rate_set);
	rc.short_retry_limit = 10;
	rc.long_retry_limit = 10;
	rc.aflags = 0;
	ret = wl1271_acx_ap_rate_policy(wl, &rc, wlvif->ap.bcast_rate_idx);
	if (ret < 0)
		return ret;

	/*
	 * If the basic rates contain OFDM rates, use OFDM only
	 * rates for unicast TX as well. Else use all supported rates.
	 */
	if (wl->ofdm_only_ap && (wlvif->basic_rate_set & CONF_TX_OFDM_RATES))
		supported_rates = CONF_TX_OFDM_RATES;
	else
		supported_rates = CONF_TX_ENABLED_RATES;

	/* unconditionally enable HT rates */
	supported_rates |= CONF_TX_MCS_RATES;

	/* get extra MIMO or wide-chan rates where the HW supports it */
	supported_rates |= wlcore_hw_ap_get_mimo_wide_rate_mask(wl, wlvif);

	/* configure unicast TX rate classes */
	for (i = 0; i < wl->conf.tx.ac_conf_count; i++) {
		rc.enabled_rates = supported_rates;
		rc.short_retry_limit = 10;
		rc.long_retry_limit = 10;
		rc.aflags = 0;
		ret = wl1271_acx_ap_rate_policy(wl, &rc,
						wlvif->ap.ucast_rate_idx[i]);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int wl1271_set_ba_policies(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	/* Reset the BA RX indicators */
	wlvif->ba_allowed = true;
	wl->ba_rx_session_count = 0;

	/* BA is supported in STA/AP modes */
	if (wlvif->bss_type != BSS_TYPE_AP_BSS &&
	    wlvif->bss_type != BSS_TYPE_STA_BSS) {
		wlvif->ba_support = false;
		return 0;
	}

	wlvif->ba_support = true;

	/* 802.11n initiator BA session setting */
	return wl12xx_acx_set_ba_initiator_policy(wl, wlvif);
}

/* vif-specifc initialization */
static int wl12xx_init_sta_role(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	int ret;

	/* =============== ***************** ==================*/
	//ret = wl1271_acx_group_address_tbl(wl, wlvif, true, NULL, 0);
	struct acx_dot11_grp_addr_tbl acx;
	/* MAC filtering */
	acx.role_id = wlvif->role_id;
	acx.enabled = true;
	acx.num_groups = 0;
	memcpy(acx.mac_table, NULL, 0 * ETH_ALEN);
	ret = VV_cmd_configure(wl, DOT11_GROUP_ADDRESS_TBL, &acx, sizeof(acx), 0);
	if (ret < 0)
		return ret;

	/* =============== ***************** ==================*/
	/* Initialize connection monitoring thresholds */
	// ret = wl1271_acx_conn_monit_params(wl, wlvif, false);
	struct acx_conn_monit_params acx1;
	u32 threshold = 0xffffffff;
	u32 timeout = 0xffffffff;
	acx1.role_id = wlvif->role_id;
	acx1.synch_fail_thold = cpu_to_le32(threshold);
	acx1.bss_lose_timeout = cpu_to_le32(timeout);
	ret = VV_cmd_configure(wl, ACX_CONN_MONIT_PARAMS, &acx1, sizeof(acx1), 0);
	if (ret < 0)
		return ret;

	/* Beacon filtering */
	// ret = wl1271_init_sta_beacon_filter(wl, wlvif);
	// ret = wl1271_acx_beacon_filter_table(wl, wlvif);
	struct acx_beacon_filter_ie_table ie_table;
	int i, idx = 0;
	bool vendor_spec = false;

	/* configure default beacon pass-through rules */
	ie_table.role_id = wlvif->role_id;
	ie_table.num_ie = 0;
	for (i = 0; i < wl->conf.conn.bcn_filt_ie_count; i++) {
		struct conf_bcn_filt_rule *r = &(wl->conf.conn.bcn_filt_ie[i]);
		ie_table.table[idx++] = r->ie;
		ie_table.table[idx++] = r->rule;

		if (r->ie == WLAN_EID_VENDOR_SPECIFIC) {
			/* only one vendor specific ie allowed */
			if (vendor_spec)
				continue;

			/* for vendor specific rules configure the
			   additional fields */
			memcpy(&(ie_table.table[idx]), r->oui,
			       CONF_BCN_IE_OUI_LEN);
			idx += CONF_BCN_IE_OUI_LEN;
			ie_table.table[idx++] = r->type;
			memcpy(&(ie_table.table[idx]), r->version,
			       CONF_BCN_IE_VER_LEN);
			idx += CONF_BCN_IE_VER_LEN;
			vendor_spec = true;
		}

		ie_table.num_ie++;
	}
	ret = VV_cmd_configure(wl, ACX_BEACON_FILTER_TABLE, &ie_table, sizeof(ie_table), 0);
	if (ret < 0)
		return ret;


	/* disable beacon filtering until we get the first beacon */
	//ret = wl1271_acx_beacon_filter_opt(wl, wlvif, false);
	struct acx_beacon_filter_option beacon_filter;
	beacon_filter.role_id = wlvif->role_id;
	beacon_filter.enable = false;
	/*
	 * When set to zero, and the filter is enabled, beacons
	 * without the unicast TIM bit set are dropped.
	 */
	beacon_filter.max_num_beacons = 0;
	ret = VV_cmd_configure(wl, ACX_BEACON_FILTER_OPT, &beacon_filter, sizeof(beacon_filter), 0);
	if (ret < 0)
		return ret;

	/* =============== ***************** ==================*/
	/* Beacons and broadcast settings */
	//ret = wl1271_init_beacon_broadcast(wl, wlvif);
	struct acx_beacon_broadcast bb;
	bb.role_id = wlvif->role_id;
	bb.beacon_rx_timeout = cpu_to_le16(wl->conf.conn.beacon_rx_timeout);
	bb.broadcast_timeout = cpu_to_le16(wl->conf.conn.broadcast_timeout);
	bb.rx_broadcast_in_ps = wl->conf.conn.rx_broadcast_in_ps;
	bb.ps_poll_threshold = wl->conf.conn.ps_poll_threshold;
	ret = VV_cmd_configure(wl, ACX_BCN_DTIM_OPTIONS, &bb, sizeof(bb), 0);
	if (ret < 0)
		return ret;

	/* =============== ***************** ==================*/
	/* Configure rssi/snr averaging weights */
	//ret = wl1271_acx_rssi_snr_avg_weights(wl, wlvif);
	struct wl1271_acx_rssi_snr_avg_weights acx2;
	struct conf_roam_trigger_settings *c = &wl->conf.roam_trigger;
	acx2.role_id = wlvif->role_id;
	acx2.rssi_beacon = c->avg_weight_rssi_beacon;
	acx2.rssi_data = c->avg_weight_rssi_data;
	acx2.snr_beacon = c->avg_weight_snr_beacon;
	acx2.snr_data = c->avg_weight_snr_data;
	ret = VV_cmd_configure(wl, ACX_RSSI_SNR_WEIGHTS, &acx2, sizeof(acx2), 0);
	if (ret < 0)
		return ret;

	return 0;
}

/* vif-specific initialization */
static int wl12xx_init_ap_role(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	int ret;

	ret = wl1271_acx_ap_max_tx_retry(wl, wlvif);
	if (ret < 0)
		return ret;

	/* initialize Tx power */
	ret = wl1271_acx_tx_power(wl, wlvif, wlvif->power_level);
	if (ret < 0)
		return ret;

	if (wl->radar_debug_mode)
		wlcore_cmd_generic_cfg(wl, wlvif,
				       WLCORE_CFG_FEATURE_RADAR_DEBUG,
				       wl->radar_debug_mode, 0);

	return 0;
}

int wl1271_init_vif_specific(struct wl1271 *wl, struct ieee80211_vif *vif)
{
	struct wl12xx_vif *wlvif = wl12xx_vif_to_data(vif);
	struct conf_tx_ac_category *conf_ac;
	struct conf_tx_tid *conf_tid;
	bool is_ap = (wlvif->bss_type == BSS_TYPE_AP_BSS);
	int ret, i;

	/* consider all existing roles before configuring psm. */
	printk("wl->ap_count = %d, is_ap = %d\n", wl->ap_count, is_ap);

	if (wl->ap_count == 0 && is_ap) { /* first AP */
		ret = wl1271_acx_sleep_auth(wl, WL1271_PSM_ELP);
		if (ret < 0)
			return ret;

		/* unmask ap events */
		wl->event_mask |= wl->ap_event_mask;
		ret = wl1271_event_unmask(wl);
		if (ret < 0)
			return ret;
	/* first STA, no APs */
	} else if (wl->sta_count == 0 && wl->ap_count == 0 && !is_ap) {
		u8 sta_auth = wl->conf.conn.sta_sleep_auth;
		/* Configure for power according to debugfs */
		if (sta_auth != WL1271_PSM_ILLEGAL)
			ret = wl1271_acx_sleep_auth(wl, sta_auth);
		/* Configure for ELP power saving */
		else
			ret = wl1271_acx_sleep_auth(wl, WL1271_PSM_ELP);

		if (ret < 0)
			return ret;
	}

	/* Mode specific init */
	if (is_ap) {
		ret = wl1271_ap_hw_init(wl, wlvif);
		if (ret < 0)
			return ret;

		ret = wl12xx_init_ap_role(wl, wlvif);
		if (ret < 0)
			return ret;
	} else {
		ret = wl1271_sta_hw_init(wl, wlvif);
		if (ret < 0)
			return ret;

		ret = wl12xx_init_sta_role(wl, wlvif);
		if (ret < 0)
			return ret;
	}

	wl12xx_init_phy_vif_config(wl, wlvif);

	/* Default TID/AC configuration */
	BUG_ON(wl->conf.tx.tid_conf_count != wl->conf.tx.ac_conf_count);
	for (i = 0; i < wl->conf.tx.tid_conf_count; i++) {
		conf_ac = &wl->conf.tx.ac_conf[i];
		ret = wl1271_acx_ac_cfg(wl, wlvif, conf_ac->ac,
					conf_ac->cw_min, conf_ac->cw_max,
					conf_ac->aifsn, conf_ac->tx_op_limit);
		if (ret < 0)
			return ret;

		conf_tid = &wl->conf.tx.tid_conf[i];
		ret = wl1271_acx_tid_cfg(wl, wlvif,
					 conf_tid->queue_id,
					 conf_tid->channel_type,
					 conf_tid->tsid,
					 conf_tid->ps_scheme,
					 conf_tid->ack_policy,
					 conf_tid->apsd_conf[0],
					 conf_tid->apsd_conf[1]);
		if (ret < 0)
			return ret;
	}

	/* Configure HW encryption */
	ret = wl1271_acx_feature_cfg(wl, wlvif);
	if (ret < 0)
		return ret;

	/* Mode specific init - post mem init */
	if (is_ap)
		ret = wl1271_ap_hw_init_post_mem(wl, vif);
	else
		ret = wl1271_sta_hw_init_post_mem(wl, vif);

	if (ret < 0)
		return ret;

	/* Configure initiator BA sessions policies */
	ret = wl1271_set_ba_policies(wl, wlvif);
	if (ret < 0)
		return ret;

	ret = wlcore_hw_init_vif(wl, wlvif);
	if (ret < 0)
		return ret;

	return 0;
}

int VV_init_vif_specific(struct wl1271 *wl, struct ieee80211_vif *vif)
{
	struct wl12xx_vif *wlvif = wl12xx_vif_to_data(vif);
	struct conf_tx_ac_category *conf_ac;
	struct conf_tx_tid *conf_tid;
	bool is_ap = (wlvif->bss_type == BSS_TYPE_AP_BSS);
	int ret, i;

	/* consider all existing roles before configuring psm. */

	// if (wl->sta_count == 0 && wl->ap_count == 0 && !is_ap) {
	// 	u8 sta_auth = wl->conf.conn.sta_sleep_auth;
	// 	/* Configure for power according to debugfs */
	// 	if (sta_auth != WL1271_PSM_ILLEGAL)
	// 		ret = wl1271_acx_sleep_auth(wl, sta_auth);
	// 	/* Configure for ELP power saving */
	// 	else
	// 		ret = wl1271_acx_sleep_auth(wl, WL1271_PSM_ELP);

	// 	if (ret < 0)
	// 		return ret;
	// }

	/* VV_ Mode specific init = PS, FM WLAN coexistence */
	ret = wl1271_sta_hw_init(wl, wlvif);
	if (ret < 0)
		return ret;

	// VV_
	ret = wl12xx_init_sta_role(wl, wlvif);
	if (ret < 0)
		return ret;


	// VV_
	wl12xx_init_phy_vif_config(wl, wlvif);

	/* Default TID/AC configuration : conf.tx.tid_conf_count */
	for (i = 0; i < 4; i++) {
		conf_ac = &wl->conf.tx.ac_conf[i];
		// ret = wl1271_acx_ac_cfg(wl, wlvif, conf_ac->ac,
		// 			conf_ac->cw_min, conf_ac->cw_max,
		// 			conf_ac->aifsn, conf_ac->tx_op_limit);
		struct acx_ac_cfg acx4;
		acx4.role_id = wlvif->role_id;
		acx4.ac = conf_ac->ac;
		acx4.cw_min = conf_ac->cw_min;
		acx4.cw_max = cpu_to_le16(conf_ac->cw_max);
		acx4.aifsn = conf_ac->aifsn;
		acx4.tx_op_limit = cpu_to_le16(conf_ac->tx_op_limit);
		ret = VV_cmd_configure(wl, ACX_AC_CFG, &acx4, sizeof(acx4), 0);
		if (ret < 0)
			return ret;

		conf_tid = &wl->conf.tx.tid_conf[i];
		// ret = wl1271_acx_tid_cfg(wl, wlvif,
		// 			 conf_tid->queue_id,
		// 			 conf_tid->channel_type,
		// 			 conf_tid->tsid,
		// 			 conf_tid->ps_scheme,
		// 			 conf_tid->ack_policy,
		// 			 conf_tid->apsd_conf[0],
		// 			 conf_tid->apsd_conf[1]);
		struct acx_tid_config acx5;
		acx5.role_id = wlvif->role_id;
		acx5.queue_id = conf_tid->queue_id;
		acx5.channel_type = conf_tid->channel_type;
		acx5.tsid = conf_tid->tsid;
		acx5.ps_scheme = conf_tid->ps_scheme;
		acx5.ack_policy = conf_tid->ack_policy;
		acx5.apsd_conf[0] = cpu_to_le32(conf_tid->apsd_conf[0]);
		acx5.apsd_conf[1] = cpu_to_le32(conf_tid->apsd_conf[1]);
		ret = VV_cmd_configure(wl, ACX_TID_CFG, &acx5, sizeof(acx5), 0);
		if (ret < 0)
			return ret;
	}

	/* Configure HW encryption */
	// ret = wl1271_acx_feature_cfg(wl, wlvif);
	struct acx_feature_config feature;
	feature.role_id = wlvif->role_id;
	feature.data_flow_options = 0;
	feature.options = 0;
	ret = VV_cmd_configure(wl, ACX_FEATURE_CFG, &feature, sizeof(feature), 0);
	if (ret < 0)
		return ret;

	/* Mode specific init - post mem init */
	// if (is_ap)
	// 	ret = wl1271_ap_hw_init_post_mem(wl, vif);
	// else
		// ret = wl1271_sta_hw_init_post_mem(wl, vif);
	struct wl1271_acx_keep_alive_mode acx6;
	acx6.role_id = wlvif->role_id;
	acx6.enabled = false;
	ret = VV_cmd_configure(wl, ACX_KEEP_ALIVE_MODE, &acx6, sizeof(acx6), 0);

	if (ret < 0)
		return ret;

	/* Configure initiator BA sessions policies */
	// ret = wl1271_set_ba_policies(wl, wlvif);
	/* Reset the BA RX indicators */
	wlvif->ba_allowed = true;
	wl->ba_rx_session_count = 0;
	struct wl1271_acx_ba_initiator_policy acx7;
	acx7.role_id = wlvif->role_id;
	acx7.tid_bitmap = wl->conf.ht.tx_ba_tid_bitmap;
	acx7.win_size = wl->conf.ht.tx_ba_win_size;
	acx7.inactivity_timeout = wl->conf.ht.inactivity_timeout;
	ret = VV_cmd_configure(wl, ACX_BA_SESSION_INIT_POLICY, &acx7, sizeof(acx7), 0);
	if (ret < 0)
		return ret;

	// ret = wlcore_hw_init_vif(wl, wlvif);
	// if (ret < 0)
	// 	return ret;

	return 0;
}

#include "../wl18xx/tx.h"
#include "../wl18xx/acx.h"
#include "../wl18xx/wl18xx.h"
int wl1271_hw_init(struct wl1271 *wl)
{
	int ret;
	struct wl18xx_priv *priv = wl->priv;

	/* Chip-specific hw init */
	// ret = wl->ops->hw_init(wl);
	/* (re)init private structures. Relevant on recovery as well. */
	priv->last_fw_rls_idx = 0;
	priv->extra_spare_key_count = 0;

	/* set the default amount of spare blocks in the bitmap */
	// ret = wl18xx_set_host_cfg_bitmap(wl, WL18XX_TX_HW_BLOCK_SPARE);
	u32 sdio_align_size = 0;
	u32 host_cfg_bitmap = HOST_IF_CFG_RX_FIFO_ENABLE |
			      HOST_IF_CFG_ADD_RX_ALIGNMENT;

	/* Enable Tx SDIO padding */
	if (wl->quirks & WLCORE_QUIRK_TX_BLOCKSIZE_ALIGN) {
		host_cfg_bitmap |= HOST_IF_CFG_TX_PAD_TO_SDIO_BLK;
		sdio_align_size = WL12XX_BUS_BLOCK_SIZE;
	}

	/* Enable Rx SDIO padding */
	if (wl->quirks & WLCORE_QUIRK_RX_BLOCKSIZE_ALIGN) {
		host_cfg_bitmap |= HOST_IF_CFG_RX_PAD_TO_SDIO_BLK;
		sdio_align_size = WL12XX_BUS_BLOCK_SIZE;
	}

	//printk("wl->quirks = %d\n", wl->quirks);
	struct wl18xx_acx_host_config_bitmap bitmap_conf;
	bitmap_conf.host_cfg_bitmap = cpu_to_le32(host_cfg_bitmap);
	bitmap_conf.host_sdio_block_size = cpu_to_le32(sdio_align_size);
	bitmap_conf.extra_mem_blocks = cpu_to_le32(WL18XX_TX_HW_BLOCK_SPARE);
	bitmap_conf.length_field_size = cpu_to_le32(WL18XX_HOST_IF_LEN_SIZE_FIELD);
	ret = VV_cmd_configure(wl, ACX_HOST_IF_CFG_BITMAP, &bitmap_conf, sizeof(bitmap_conf), 0);
	if (ret < 0)
		return ret;

	/* set the dynamic fw traces bitmap */
	// ret = wl18xx_acx_dynamic_fw_traces(wl);
	struct acx_dynamic_fw_traces_cfg acx;
	acx.dynamic_fw_traces = cpu_to_le32(wl->dynamic_fw_traces);
	ret = VV_cmd_configure(wl, ACX_DYNAMIC_TRACES_CFG, &acx, sizeof(acx), 0);
	if (ret < 0)
		return ret;
	// if (checksum_param) {
	// 	ret = wl18xx_acx_set_checksum_state(wl);
	// 	if (ret != 0)
	// 		return ret;
	// }


	/* Init templates */
	// ret = wl1271_init_templates_config(wl);
	ret = VV_init_templates_config(wl);
	if (ret < 0)
		return ret;

	ret = wl12xx_acx_mem_cfg(wl);
	if (ret < 0)
		return ret;

	/* Configure the FW logger */
	ret = wl12xx_init_fwlog(wl);
	if (ret < 0)
		return ret;

	ret = wlcore_cmd_regdomain_config_locked(wl);
	if (ret < 0)
		return ret;

	/* Bluetooth WLAN coexistence */
	ret = wl1271_init_pta(wl);
	if (ret < 0)
		return ret;

	/* Default memory configuration */
	ret = wl1271_acx_init_mem_config(wl);
	if (ret < 0)
		return ret;

	/* RX config */
	ret = wl12xx_init_rx_config(wl);
	if (ret < 0)
		goto out_free_memmap;

	ret = wl1271_acx_dco_itrim_params(wl);
	if (ret < 0)
		goto out_free_memmap;

	/* Configure TX patch complete interrupt behavior */
	ret = wl1271_acx_tx_config_options(wl);
	if (ret < 0)
		goto out_free_memmap;

	/* RX complete interrupt pacing */
	ret = wl1271_acx_init_rx_interrupt(wl);
	if (ret < 0)
		goto out_free_memmap;

	/* Energy detection */
	ret = wl1271_init_energy_detection(wl);
	if (ret < 0)
		goto out_free_memmap;

	/* Default fragmentation threshold */
	ret = wl1271_acx_frag_threshold(wl, wl->hw->wiphy->frag_threshold);
	if (ret < 0)
		goto out_free_memmap;

	/* Enable data path */
	ret = wl1271_cmd_data_path(wl, 1);
	if (ret < 0)
		goto out_free_memmap;

	/* configure PM */
	ret = wl1271_acx_pm_config(wl);
	if (ret < 0)
		goto out_free_memmap;

	ret = wl12xx_acx_set_rate_mgmt_params(wl);
	if (ret < 0)
		goto out_free_memmap;

	/* configure hangover */
	ret = wl12xx_acx_config_hangover(wl);
	if (ret < 0)
		goto out_free_memmap;

	return 0;

 out_free_memmap:
	kfree(wl->target_mem_map);
	wl->target_mem_map = NULL;

	return ret;
}

int VV_hw_init(struct wl1271 *wl)
{
	int ret;
	struct wl18xx_priv *priv = wl->priv;

	/* Chip-specific hw init */
	// ret = wl->ops->hw_init(wl);
	/* (re)init private structures. Relevant on recovery as well. */
	priv->last_fw_rls_idx = 0;
	priv->extra_spare_key_count = 0;

	/* set the default amount of spare blocks in the bitmap */
	// ret = wl18xx_set_host_cfg_bitmap(wl, WL18XX_TX_HW_BLOCK_SPARE);
	u32 sdio_align_size = 0;
	u32 host_cfg_bitmap = HOST_IF_CFG_RX_FIFO_ENABLE |
			      HOST_IF_CFG_ADD_RX_ALIGNMENT;

	/* Enable Tx SDIO padding */
	if (wl->quirks & WLCORE_QUIRK_TX_BLOCKSIZE_ALIGN) {
		host_cfg_bitmap |= HOST_IF_CFG_TX_PAD_TO_SDIO_BLK;
		sdio_align_size = WL12XX_BUS_BLOCK_SIZE;
	}

	/* Enable Rx SDIO padding */
	if (wl->quirks & WLCORE_QUIRK_RX_BLOCKSIZE_ALIGN) {
		host_cfg_bitmap |= HOST_IF_CFG_RX_PAD_TO_SDIO_BLK;
		sdio_align_size = WL12XX_BUS_BLOCK_SIZE;
	}

	//printk("wl->quirks = %d\n", wl->quirks);
	struct wl18xx_acx_host_config_bitmap bitmap_conf;
	bitmap_conf.host_cfg_bitmap = cpu_to_le32(host_cfg_bitmap);
	bitmap_conf.host_sdio_block_size = cpu_to_le32(sdio_align_size);
	bitmap_conf.extra_mem_blocks = cpu_to_le32(WL18XX_TX_HW_BLOCK_SPARE);
	bitmap_conf.length_field_size = cpu_to_le32(WL18XX_HOST_IF_LEN_SIZE_FIELD);
	ret = VV_cmd_configure(wl, ACX_HOST_IF_CFG_BITMAP, &bitmap_conf, sizeof(bitmap_conf), 0);
	if (ret < 0)
		return ret;

	/* set the dynamic fw traces bitmap */
	// ret = wl18xx_acx_dynamic_fw_traces(wl);
	struct acx_dynamic_fw_traces_cfg acx;
	acx.dynamic_fw_traces = cpu_to_le32(wl->dynamic_fw_traces);
	ret = VV_cmd_configure(wl, ACX_DYNAMIC_TRACES_CFG, &acx, sizeof(acx), 0);
	if (ret < 0)
		return ret;
	// if (checksum_param) {
	// 	ret = wl18xx_acx_set_checksum_state(wl);
	// 	if (ret != 0)
	// 		return ret;
	// }


	/* Init templates */
	// ret = wl1271_init_templates_config(wl);
	ret = VV_init_templates_config(wl);
	if (ret < 0)
		return ret;

	// ret = wl12xx_acx_mem_cfg(wl);
	struct wl12xx_acx_config_memory mem_conf;
	struct conf_memory_settings *mem;
	mem = &wl->conf.mem;
	mem_conf.num_stations = mem->num_stations;
	mem_conf.rx_mem_block_num = mem->rx_block_num;
	mem_conf.tx_min_mem_block_num = mem->tx_min_block_num;
	mem_conf.num_ssid_profiles = mem->ssid_profiles;
	mem_conf.total_tx_descriptors = cpu_to_le32(wl->num_tx_desc);
	mem_conf.dyn_mem_enable = mem->dynamic_memory;
	mem_conf.tx_free_req = mem->min_req_tx_blocks;
	mem_conf.rx_free_req = mem->min_req_rx_blocks;
	mem_conf.tx_min = mem->tx_min;
	mem_conf.fwlog_blocks = wl->conf.fwlog.mem_blocks;
	ret = VV_cmd_configure(wl, ACX_MEM_CFG, &mem_conf, sizeof(mem_conf), 0);
	if (ret < 0)
		return ret;

	/* Configure the FW logger */
	// ret = wl12xx_init_fwlog(wl);
	struct wl12xx_cmd_config_fwlog cmd;
	cmd.logger_mode = wl->conf.fwlog.mode;
	cmd.log_severity = wl->conf.fwlog.severity;
	cmd.timestamp = wl->conf.fwlog.timestamp;
	cmd.output = wl->conf.fwlog.output;
	cmd.threshold = wl->conf.fwlog.threshold;
	ret = VV_cmd_configure(wl, CMD_CONFIG_FWLOGGER, &cmd, sizeof(cmd), 0);
	if (ret < 0)
		return ret;

	// VV_
	ret = VV_cmd_regdomain_config_locked(wl);
	if (ret < 0)
		return ret;

	/* Bluetooth WLAN coexistence */
	// ret = wl1271_init_pta(wl);
	int i;
	// ret = wl12xx_acx_sg_cfg(wl);
	struct acx_bt_wlan_coex_param param;
	struct conf_sg_settings *c = &wl->conf.sg;
	/* BT-WLAN coext parameters */
	for (i = 0; i < WLCORE_CONF_SG_PARAMS_MAX; i++)
		param.params[i] = cpu_to_le32(c->params[i]);
	param.param_idx = WLCORE_CONF_SG_PARAMS_ALL;
	ret = VV_cmd_configure(wl, ACX_SG_CFG, &param, sizeof(param), 0);
	if (ret < 0)
		return ret;

	// ret = wl1271_acx_sg_enable(wl, wl->sg_enabled);
	struct acx_bt_wlan_coex pta;
	if (wl->sg_enabled)
		pta.enable = wl->conf.sg.state;
	else
		pta.enable = CONF_SG_DISABLE;
	ret = VV_cmd_configure(wl, ACX_SG_ENABLE, &pta, sizeof(pta), 0);
	if (ret < 0)
		return ret;

	/* Default memory configuration */
	// ret = wl1271_acx_init_mem_config(wl);
	wl->target_mem_map = kzalloc(sizeof(struct wl1271_acx_mem_map),
				     GFP_KERNEL);
	if (!wl->target_mem_map) {
		wl1271_error("couldn't allocate target memory map");
		return -ENOMEM;
	}

	/* we now ask for the firmware built memory map */
	ret = VV_cmd_interrogate(wl, ACX_MEM_MAP, (void *)wl->target_mem_map,
				     sizeof(struct acx_header), sizeof(struct wl1271_acx_mem_map));
	if (ret < 0) {
		wl1271_error("couldn't retrieve firmware memory map");
		kfree(wl->target_mem_map);
		wl->target_mem_map = NULL;
		return -1;
	}

	/* initialize TX block book keeping */
	wl->tx_blocks_available =
		le32_to_cpu(wl->target_mem_map->num_tx_mem_blocks);
	wl1271_debug(DEBUG_TX, "available tx blocks: %d",
		     wl->tx_blocks_available);
	/* ====== [END] === */

	/* RX config */
	// ret = wl12xx_init_rx_config(wl);
	struct acx_rx_msdu_lifetime acx1;
	acx1.lifetime = cpu_to_le32(wl->conf.rx.rx_msdu_life_time);
	ret = VV_cmd_configure(wl, DOT11_RX_MSDU_LIFE_TIME, &acx1, sizeof(acx1), 0);
	if (ret < 0)
		goto out_free_memmap;

	// ret = wl1271_acx_dco_itrim_params(wl);
	struct acx_dco_itrim_params dco;
	struct conf_itrim_settings *c1 = &wl->conf.itrim;
	dco.enable = c1->enable;
	dco.timeout = cpu_to_le32(c1->timeout);
	ret = VV_cmd_configure(wl, ACX_SET_DCO_ITRIM_PARAMS, &dco, sizeof(dco), 0);
	if (ret < 0)
		goto out_free_memmap;

	/* Configure TX patch complete interrupt behavior */
	// ret = wl1271_acx_tx_config_options(wl);
	struct acx_tx_config_options acx2;
	acx2.tx_compl_timeout = cpu_to_le16(wl->conf.tx.tx_compl_timeout);
	acx2.tx_compl_threshold = cpu_to_le16(wl->conf.tx.tx_compl_threshold);
	ret = VV_cmd_configure(wl, ACX_TX_CONFIG_OPT, &acx2, sizeof(acx2), 0);
	if (ret < 0)
		goto out_free_memmap;

	/* RX complete interrupt pacing */
	// ret = wl1271_acx_init_rx_interrupt(wl);
	struct wl1271_acx_rx_config_opt rx_conf;
	rx_conf.threshold = cpu_to_le16(wl->conf.rx.irq_pkt_threshold);
	rx_conf.timeout = cpu_to_le16(wl->conf.rx.irq_timeout);
	rx_conf.mblk_threshold = cpu_to_le16(wl->conf.rx.irq_blk_threshold);
	rx_conf.queue_type = wl->conf.rx.queue_type;
	ret = VV_cmd_configure(wl, ACX_RX_CONFIG_OPT, &rx_conf, sizeof(rx_conf), 0);
	if (ret < 0)
		goto out_free_memmap;

	/* Energy detection */
	// ret = wl1271_init_energy_detection(wl);
	struct acx_energy_detection detection;
	detection.rx_cca_threshold = cpu_to_le16(wl->conf.rx.rx_cca_threshold);
	detection.tx_energy_detection = wl->conf.tx.tx_energy_detection;
	ret = VV_cmd_configure(wl, ACX_CCA_THRESHOLD, &detection, sizeof(detection), 0);
	if (ret < 0)
		goto out_free_memmap;

	/* Default fragmentation threshold */
	ret = wl1271_acx_frag_threshold(wl, wl->hw->wiphy->frag_threshold);
	struct acx_frag_threshold acx3;
	if (wl->hw->wiphy->frag_threshold > IEEE80211_MAX_FRAG_THRESHOLD)
		wl->hw->wiphy->frag_threshold = wl->conf.tx.frag_threshold;
	acx3.frag_threshold = cpu_to_le16((u16)wl->hw->wiphy->frag_threshold);
	ret = VV_cmd_configure(wl, ACX_FRAG_CFG, &acx3, sizeof(acx3), 0);
	if (ret < 0)
		goto out_free_memmap;

	/* Enable data path */
	// ret = wl1271_cmd_data_path(wl, 1);
	struct cmd_enabledisable_path cmd1;
	u16 cmd_rx, cmd_tx;
	/* the channel here is only used for calibration, so hardcoded to 1 */
	cmd1.channel = 1;
	cmd_rx = CMD_ENABLE_RX;
	cmd_tx = CMD_ENABLE_TX;
	ret = wl1271_cmd_send1(wl, cmd_rx, &cmd1, sizeof(cmd1), 0);
	if (ret < 0)
		goto out_free_memmap;
	ret = wl1271_cmd_send1(wl, cmd_tx, &cmd1, sizeof(cmd1), 0);
	if (ret < 0)
		goto out_free_memmap;

	/* configure PM */
	// ret = wl1271_acx_pm_config(wl);
	struct wl1271_acx_pm_config acx5;
	struct conf_pm_config_settings *c2 = &wl->conf.pm_config;
	acx5.host_clk_settling_time = cpu_to_le32(c2->host_clk_settling_time);
	acx5.host_fast_wakeup_support = c2->host_fast_wakeup_support;
	ret = VV_cmd_configure(wl, ACX_PM_CONFIG, &acx5, sizeof(acx5), 0);
	if (ret < 0)
		goto out_free_memmap;

	// ret = wl12xx_acx_set_rate_mgmt_params(wl);
	struct wl12xx_acx_set_rate_mgmt_params acx4;
	struct conf_rate_policy_settings *conf = &wl->conf.rate;
	acx4.index = ACX_RATE_MGMT_ALL_PARAMS;
	acx4.rate_retry_score = cpu_to_le16(conf->rate_retry_score);
	acx4.per_add = cpu_to_le16(conf->per_add);
	acx4.per_th1 = cpu_to_le16(conf->per_th1);
	acx4.per_th2 = cpu_to_le16(conf->per_th2);
	acx4.max_per = cpu_to_le16(conf->max_per);
	acx4.inverse_curiosity_factor = conf->inverse_curiosity_factor;
	acx4.tx_fail_low_th = conf->tx_fail_low_th;
	acx4.tx_fail_high_th = conf->tx_fail_high_th;
	acx4.per_alpha_shift = conf->per_alpha_shift;
	acx4.per_add_shift = conf->per_add_shift;
	acx4.per_beta1_shift = conf->per_beta1_shift;
	acx4.per_beta2_shift = conf->per_beta2_shift;
	acx4.rate_check_up = conf->rate_check_up;
	acx4.rate_check_down = conf->rate_check_down;
	memcpy(acx4.rate_retry_policy, conf->rate_retry_policy,
	       sizeof(acx4.rate_retry_policy));
	ret = VV_cmd_configure(wl, ACX_SET_RATE_MGMT_PARAMS, &acx4, sizeof(acx4), 0);
	if (ret < 0)
		goto out_free_memmap;

	/* configure hangover */
	ret = wl12xx_acx_config_hangover(wl);
	struct wl12xx_acx_config_hangover acx6;
	struct conf_hangover_settings *conf1 = &wl->conf.hangover;
	acx6.recover_time = cpu_to_le32(conf1->recover_time);
	acx6.hangover_period = conf1->hangover_period;
	acx6.dynamic_mode = conf1->dynamic_mode;
	acx6.early_termination_mode = conf1->early_termination_mode;
	acx6.max_period = conf1->max_period;
	acx6.min_period = conf1->min_period;
	acx6.increase_delta = conf1->increase_delta;
	acx6.decrease_delta = conf1->decrease_delta;
	acx6.quiet_time = conf1->quiet_time;
	acx6.increase_time = conf1->increase_time;
	acx6.window_size = conf1->window_size;
	ret = VV_cmd_configure(wl, ACX_CONFIG_HANGOVER, &acx6, sizeof(acx6), 0);
	if (ret < 0)
		goto out_free_memmap;

	return 0;

 out_free_memmap:
	kfree(wl->target_mem_map);
	wl->target_mem_map = NULL;

	return ret;
}
