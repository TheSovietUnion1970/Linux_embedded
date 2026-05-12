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
//#include "hw_ops.h"

static int wl1271_init_templates_config(void)
{
	int ret, i;
	size_t max_size;

	/* send empty templates for fw memory reservation */
	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_CFG_PROBE_REQ_2_4, NULL,
				      WL1271_CMD_TEMPL_MAX_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_CFG_PROBE_REQ_5,
				      NULL, WL1271_CMD_TEMPL_MAX_SIZE, 0,
				      WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	if (wifi_data->quirks & WLCORE_QUIRK_DUAL_PROBE_TMPL) {
		ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
					    //   wifi_data->sched_scan_templ_id_2_4,
						  CMD_TEMPL_PROBE_REQ_2_4_PERIODIC,
					      NULL,
					      WL1271_CMD_TEMPL_MAX_SIZE,
					      0, WL1271_RATE_AUTOMATIC);
		if (ret < 0)
			return ret;

		ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
					      // wifi_data->sched_scan_templ_id_5,
						  CMD_TEMPL_PROBE_REQ_5_PERIODIC,
					      NULL,
					      WL1271_CMD_TEMPL_MAX_SIZE,
					      0, WL1271_RATE_AUTOMATIC);
		if (ret < 0)
			return ret;
	}

	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_NULL_DATA, NULL,
				      sizeof(struct wl12xx_null_data_template),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_PS_POLL, NULL,
				      sizeof(struct wl12xx_ps_poll_template),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_QOS_NULL_DATA, NULL,
				      sizeof
				      (struct ieee80211_qos_hdr),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_PROBE_RESPONSE, NULL,
				      WL1271_CMD_TEMPL_DFLT_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_BEACON, NULL,
				      WL1271_CMD_TEMPL_DFLT_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	max_size = sizeof(struct wl12xx_arp_rsp_template) +
		   WL1271_EXTRA_SPACE_MAX;
	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_ARP_RSP, NULL,
				      max_size,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	/*
	 * Put very large empty placeholders for all templates. These
	 * reserve memory for later.
	 */
	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_AP_PROBE_RESPONSE, NULL,
				      WL1271_CMD_TEMPL_MAX_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_AP_BEACON, NULL,
				      WL1271_CMD_TEMPL_MAX_SIZE,
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
				      CMD_TEMPL_DEAUTH_AP, NULL,
				      sizeof
				      (struct wl12xx_disconn_template),
				      0, WL1271_RATE_AUTOMATIC);
	if (ret < 0)
		return ret;

	for (i = 0; i < WLCORE_MAX_KLV_TEMPLATES; i++) {
		ret = wl1271_cmd_template_set(WL12XX_INVALID_ROLE_ID,
					      CMD_TEMPL_KLV, NULL,
					      sizeof(struct ieee80211_qos_hdr),
					      i, WL1271_RATE_AUTOMATIC);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int wl12xx_init_rx_config(void)
{
	int ret;

	ret = wl1271_acx_rx_msdu_life_time();
	if (ret < 0)
		return ret;

	return 0;
}

static int wl12xx_init_phy_vif_config(
					    struct wl12xx_vif *wlvif)
{
	int ret;

	ret = wl1271_acx_slot(wlvif, DEFAULT_SLOT_TIME);
	if (ret < 0)
		return ret;

	ret = wl1271_acx_service_period_timeout(wlvif);
	if (ret < 0)
		return ret;

	ret = wl1271_acx_rts_threshold(wlvif, wifi_data->hw->wiphy->rts_threshold);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_init_sta_beacon_filter(
					 struct wl12xx_vif *wlvif)
{
	int ret;

	ret = wl1271_acx_beacon_filter_table(wlvif);
	if (ret < 0)
		return ret;

	/* disable beacon filtering until we get the first beacon */
	ret = wl1271_acx_beacon_filter_opt(wlvif, false);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_init_pta(void)
{
	int ret;

	ret = wl12xx_acx_sg_cfg();
	if (ret < 0)
		return ret;

	ret = wl1271_acx_sg_enable(true); // wifi_data->sg_enabled = true
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_init_energy_detection(void)
{
	int ret;

	ret = wl1271_acx_cca_threshold();
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_init_beacon_broadcast(
					struct wl12xx_vif *wlvif)
{
	int ret;

	ret = wl1271_acx_bcn_dtim_options(wlvif);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl12xx_init_fwlog(void)
{
	int ret;

	if (wifi_data->quirks & WLCORE_QUIRK_FWLOG_NOT_IMPLEMENTED)
		return 0;

	ret = wl12xx_cmd_config_fwlog();
	if (ret < 0)
		return ret;

	return 0;
}

/* generic sta initialization (non vif-specific) */
int wl1271_sta_hw_init(struct wl12xx_vif *wlvif)
{
	int ret;

	/* FM WLAN coexistence */
	ret = wl1271_acx_fm_coex();
	if (ret < 0)
		return ret;

	ret = wl1271_acx_sta_rate_policies(wlvif);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_sta_hw_init_post_mem(
				       struct ieee80211_vif *vif)
{
	struct wl12xx_vif *wlvif = wl12xx_vif_to_data(vif);
	int ret;

	/* disable the keep-alive feature */
	ret = wl1271_acx_keep_alive_mode(wlvif, false);
	if (ret < 0)
		return ret;

	return 0;
}

static int wl1271_set_ba_policies(struct wl12xx_vif *wlvif)
{
	/* Reset the BA RX indicators */
	//wlvif->ba_allowed = true;
	//wifi_data->ba_rx_session_count = 0;

	/* BA is supported in STA/AP modes */
	if (wlvif->bss_type != BSS_TYPE_AP_BSS &&
	    wlvif->bss_type != BSS_TYPE_STA_BSS) {
		//wlvif->ba_support = false;
		return 0;
	}

	//wlvif->ba_support = true;

	/* 802.11n initiator BA session setting */
	return wl12xx_acx_set_ba_initiator_policy(wlvif);
}

/* vif-specifc initialization */
static int wl12xx_init_sta_role(struct wl12xx_vif *wlvif)
{
	int ret;

	ret = wl1271_acx_group_address_tbl(wlvif, true, NULL, 0);
	if (ret < 0)
		return ret;

	/* Initialize connection monitoring thresholds */
	ret = wl1271_acx_conn_monit_params(wlvif, false);
	if (ret < 0)
		return ret;

	/* Beacon filtering */
	ret = wl1271_init_sta_beacon_filter(wlvif);
	if (ret < 0)
		return ret;

	/* Beacons and broadcast settings */
	ret = wl1271_init_beacon_broadcast(wlvif);
	if (ret < 0)
		return ret;

	/* Configure rssi/snr averaging weights */
	ret = wl1271_acx_rssi_snr_avg_weights(wlvif);
	if (ret < 0)
		return ret;

	return 0;
}

/* vif-specific initialization */
int wl1271_init_vif_specific(struct ieee80211_vif *vif)
{
	struct wl12xx_vif *wlvif = wl12xx_vif_to_data(vif);
	struct conf_tx_ac_category *conf_ac;
	struct conf_tx_tid *conf_tid;
	bool is_ap = (wlvif->bss_type == BSS_TYPE_AP_BSS);
	int ret, i;

	/* consider all existing roles before configuring psm. */
	u8 sta_auth = wifi_data->conf.conn.sta_sleep_auth;
	/* Configure for power according to debugfs */
	if (sta_auth != WL1271_PSM_ILLEGAL)
		ret = wl1271_acx_sleep_auth(sta_auth);
	/* Configure for ELP power saving */
	else
		ret = wl1271_acx_sleep_auth(WL1271_PSM_ELP);

	if (ret < 0)
		return ret;

	/* Mode specific init */
	ret = wl1271_sta_hw_init(wlvif);
	if (ret < 0)
		return ret;

	ret = wl12xx_init_sta_role(wlvif);
	if (ret < 0)
		return ret;

	wl12xx_init_phy_vif_config(wlvif);

	/* Default TID/AC configuration */
	BUG_ON(wifi_data->conf.tx.tid_conf_count != wifi_data->conf.tx.ac_conf_count);
	for (i = 0; i < wifi_data->conf.tx.tid_conf_count; i++) {
		conf_ac = &wifi_data->conf.tx.ac_conf[i];
		ret = wl1271_acx_ac_cfg(wlvif, conf_ac->ac,
					conf_ac->cw_min, conf_ac->cw_max,
					conf_ac->aifsn, conf_ac->tx_op_limit);
		if (ret < 0)
			return ret;

		conf_tid = &wifi_data->conf.tx.tid_conf[i];
		ret = wl1271_acx_tid_cfg(wlvif,
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
	ret = wl1271_acx_feature_cfg(wlvif);
	if (ret < 0)
		return ret;

	/* Mode specific init - post mem init */
	ret = wl1271_sta_hw_init_post_mem(vif);

	if (ret < 0)
		return ret;

	/* Configure initiator BA sessions policies */
	ret = wl1271_set_ba_policies(wlvif);
	if (ret < 0)
		return ret;

	return 0;
}

int wl1271_hw_init(void)
{
	int ret;

	/* Chip-specific hw init */
	ret = wifi_data->ops->hw_init();
	if (ret < 0)
		return ret;

	/* Init templates */
	ret = wl1271_init_templates_config();
	if (ret < 0)
		return ret;

	ret = wl12xx_acx_mem_cfg();
	if (ret < 0)
		return ret;

	/* Configure the FW logger */
	ret = wl12xx_init_fwlog();
	if (ret < 0)
		return ret;

	ret = wlcore_cmd_regdomain_config_locked();
	if (ret < 0)
		return ret;

	/* Bluetooth WLAN coexistence */
	ret = wl1271_init_pta();
	if (ret < 0)
		return ret;

	/* Default memory configuration */
	ret = wl1271_acx_init_mem_config();
	if (ret < 0)
		return ret;

	/* RX config */
	ret = wl12xx_init_rx_config();
	if (ret < 0)
		goto out_free_memmap;

	ret = wl1271_acx_dco_itrim_params();
	if (ret < 0)
		goto out_free_memmap;

	/* Configure TX patch complete interrupt behavior */
	ret = wl1271_acx_tx_config_options();
	if (ret < 0)
		goto out_free_memmap;

	/* RX complete interrupt pacing */
	ret = wl1271_acx_init_rx_interrupt();
	if (ret < 0)
		goto out_free_memmap;

	/* Energy detection */
	ret = wl1271_init_energy_detection();
	if (ret < 0)
		goto out_free_memmap;

	/* Default fragmentation threshold */
	ret = wl1271_acx_frag_threshold(wifi_data->hw->wiphy->frag_threshold);
	if (ret < 0)
		goto out_free_memmap;

	/* Enable data path */
	ret = wl1271_cmd_data_path(1);
	if (ret < 0)
		goto out_free_memmap;

	/* configure PM */
	ret = wl1271_acx_pm_config();
	if (ret < 0)
		goto out_free_memmap;

	ret = wl12xx_acx_set_rate_mgmt_params();
	if (ret < 0)
		goto out_free_memmap;

	/* configure hangover */
	ret = wl12xx_acx_config_hangover();
	if (ret < 0)
		goto out_free_memmap;

	return 0;

 out_free_memmap:
	kfree(wifi_data->target_mem_map);
	wifi_data->target_mem_map = NULL;

	return ret;
}
