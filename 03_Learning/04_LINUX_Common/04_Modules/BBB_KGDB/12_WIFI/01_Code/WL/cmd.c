// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009-2010 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/spi/spi.h>
#include <linux/etherdevice.h>
#include <linux/ieee80211.h>
#include <linux/slab.h>

#include "wlcore.h"
#include "debug.h"
#include "io.h"
#include "acx.h"
#include "wifi_80211.h"
#include "cmd.h"
#include "event.h"
#include "tx.h"

#include "common.h"
#include "ops.h"
#include "main.h"

#define WL1271_CMD_FAST_POLL_COUNT       50
#define WL1271_WAIT_EVENT_FAST_POLL_COUNT 20

#define WL18XX_CMD_MAX_SIZE          740
int wifi_cmd_send(u16 id, void *buf, size_t len, size_t res_len)
{
	struct wifi_cmd_header *cmd;
	unsigned long timeout;
	u32 intr;
	int ret;
	u16 status;
	u8 cmd_max[WL18XX_CMD_MAX_SIZE];

	cmd = buf;
	cmd->id = cpu_to_le16(id);
	cmd->status = 0;

	ret = wifi_sdio_raw_write1(wificore_translate_addr(*wifi_data->cmd_box_addr), buf, len, false);
	if (ret < 0)
		return ret;

	memcpy(cmd_max, buf, len);
	memset(cmd_max + len, 0, WL18XX_CMD_MAX_SIZE - len);

	ret = wifi_sdio_raw_write1(wificore_translate_addr(*wifi_data->cmd_box_addr), cmd_max, WL18XX_CMD_MAX_SIZE, false);


	timeout = jiffies + msecs_to_jiffies(WL1271_COMMAND_TIMEOUT);
	ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_INTERRUPT_NO_CLEAR]), &intr, sizeof(intr), false);
	if (ret < 0)
		return ret;

	while (!(intr & WL1271_ACX_INTR_CMD_COMPLETE)) {
		if (time_after(jiffies, timeout)) {
			wifi_error("command complete timeout");
			return -ETIMEDOUT;
		}
		ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_INTERRUPT_NO_CLEAR]), &intr, sizeof(intr), false);
		if (ret < 0)
			return ret;
	}

	/* read back the status code of the command */
	if (res_len == 0)
		res_len = sizeof(struct wifi_cmd_header);

	ret = wifi_sdio_raw_read(wificore_translate_addr(*wifi_data->cmd_box_addr), (u32*)cmd, sizeof(*cmd), false);
	if (ret < 0)
		return ret;
	status = le16_to_cpu(cmd->status);

	ret = wifi_sdio_raw_write(wificore_translate_addr(wifi_data->rtable[REG_INTERRUPT_ACK]), 
				WL1271_ACX_INTR_CMD_COMPLETE, sizeof(WL1271_ACX_INTR_CMD_COMPLETE), false);
	if (ret < 0){
		printk("[FAILED] - 1.wifi_cmd_send\n");
		return ret;
	}

	if (status == 1) return 0;
	else {
		printk("[FAILED] - 2.wifi_cmd_send\n");
		return -1;
	}
}
EXPORT_SYMBOL_GPL(wifi_cmd_send);

int wifi_cmd_configure(u16 id, void *buf,
				  size_t len)
{
	struct acx_header *acx = buf;
	int ret;

	acx->id = cpu_to_le16(id);

	/* payload length, does not include any headers */
	acx->len = cpu_to_le16(len - sizeof(*acx));

	ret = wifi_cmd_send(CMD_CONFIGURE, acx, len, 0);
	if (ret < 0) {
		wifi_warning("CONFIGURE command NOK");
		return ret;
	}

	return ret;
}

/*
 * Poll the mailbox event field until any of the bits in the mask is set or a
 * timeout occurs (WL1271_EVENT_TIMEOUT in msecs)
 */
int wificore_cmd_wait_for_event_or_timeout(u32 mask, bool *timeout)
{
	u32 *events_vector;
	u32 event;
	unsigned long timeout_time;
	u16 poll_count = 0;
	int ret = 0;

	*timeout = false;

	events_vector = kmalloc(sizeof(*events_vector), GFP_KERNEL | GFP_DMA);
	if (!events_vector)
		return -ENOMEM;

	timeout_time = jiffies + msecs_to_jiffies(WL1271_EVENT_TIMEOUT);

	ret = pm_runtime_get_sync(wifi_data->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(wifi_data->dev);
		goto free_vector;
	}

	do {
		if (time_after(jiffies, timeout_time)) {
			wifi_debug(DEBUG_CMD, "timeout waiting for event %d",
				     (int)mask);
			*timeout = true;
			goto out;
		}

		poll_count++;
		if (poll_count < WL1271_WAIT_EVENT_FAST_POLL_COUNT)
			usleep_range(50, 51);
		else
			usleep_range(1000, 5000);

		/* read from both event fields */
		ret = wifi_sdio_raw_read(wificore_translate_addr(*wifi_data->mbox_ptr[0]), (u32*)events_vector, sizeof(*events_vector), false);
		if (ret < 0)
			goto out;

		event = *events_vector & mask;
		ret = wifi_sdio_raw_read(wificore_translate_addr(*wifi_data->mbox_ptr[1]), (u32*)events_vector, sizeof(*events_vector), false);
		if (ret < 0)
			goto out;

		event |= *events_vector & mask;
	} while (!event);

out:
	pm_runtime_mark_last_busy(wifi_data->dev);
	pm_runtime_put_autosuspend(wifi_data->dev);
free_vector:
	kfree(events_vector);
	return ret;
}
EXPORT_SYMBOL_GPL(wificore_cmd_wait_for_event_or_timeout);

int wifi_cmd_role_enable(u8 *addr, u8 role_type,
			   u8 *role_id)
{
	struct wifi_cmd_role_enable *cmd;
	int ret;

	wifi_debug(DEBUG_CMD, "cmd role enable");

	if (WARN_ON(*role_id != WL12XX_INVALID_ROLE_ID))
		return -EBUSY;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	// /* get role id */
	// cmd->role_id = find_first_zero_bit(wifi_data->roles_map, WL12XX_MAX_ROLES);
	// if (cmd->role_id >= WL12XX_MAX_ROLES) {
	// 	ret = -EBUSY;
	// 	goto out_free;
	// }

	if (role_type == WL1271_ROLE_STA){
		cmd->role_id = STA_ROLE_ID;
	}
	else {
		cmd->role_id = P2P_ROLE_ID;
	}

	memcpy(cmd->mac_address, addr, ETH_ALEN);
	cmd->role_type = role_type;

	ret = wifi_cmd_send(CMD_ROLE_ENABLE, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("failed to initiate cmd role enable");
		goto out_free;
	}

	// __set_bit(cmd->role_id, wifi_data->roles_map);
	*role_id = cmd->role_id;


out_free:
	kfree(cmd);

out:
	return ret;
}

int wifi_cmd_role_disable(u8 *role_id)
{
	struct wifi_cmd_role_disable *cmd;
	int ret;

	wifi_debug(DEBUG_CMD, "cmd role disable");

	if (WARN_ON(*role_id == WL12XX_INVALID_ROLE_ID))
		return -ENOENT;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}
	cmd->role_id = *role_id;

	ret = wifi_cmd_send(CMD_ROLE_DISABLE, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("failed to initiate cmd role disable");
		goto out_free;
	}

	//__clear_bit(*role_id, wifi_data->roles_map);
	*role_id = WL12XX_INVALID_ROLE_ID;

out_free:
	kfree(cmd);

out:
	return ret;
}

static int wificore_get_new_session_id(u8 hlid)
{
	if (wifi_session_ids[hlid] >= SESSION_COUNTER_MAX)
		wifi_session_ids[hlid] = 0;

	wifi_session_ids[hlid]++;

	return wifi_session_ids[hlid];
}

#define WL18XX_MAX_LINKS 16
int wifi_allocate_link(struct wifi_vif *wifi_vif, u8 *hlid)
{
	unsigned long flags;

	u8 link = HW_LINK_ID;
	if (link >= WL18XX_MAX_LINKS)
		return -EBUSY;

	wifi_session_ids[link] = wificore_get_new_session_id(link);

	/* these bits are used by op_tx */
	spin_lock_irqsave(&wifi_data->lock, flags);
	__set_bit(link, wifi_map.links_map);
	__set_bit(link, wifi_vif->links_map);
	spin_unlock_irqrestore(&wifi_data->lock, flags);

	/*
	 * take the last "freed packets" value from the current FW status.
	 * on recovery, we might not have fw_status yet, and
	 * tx_lnk_free_pkts will be NULL. check for it.
	 */
	if (wifi_status_reg->tx_lnk_free_pkts)
		wifi_links[link].prev_freed_pkts =
			wifi_status_reg->tx_lnk_free_pkts[link];


	wifi_links[link].wifi_vif = wifi_vif;

	/*
	 * Take saved value for total freed packets from wifi_vif, in case this is
	 * recovery/resume
	 */
	if (wifi_vif->bss_type != BSS_TYPE_AP_BSS)
		wifi_links[link].total_freed_pkts = wifi_vif->total_freed_pkts;

	*hlid = link;

	//wifi_data->active_link_count++;
	return 0;
}

void wifi_free_link(struct wifi_vif *wifi_vif, u8 *hlid)
{
	unsigned long flags;

	if (*hlid == WL12XX_INVALID_LINK_ID)
		return;

	/* these bits are used by op_tx */
	spin_lock_irqsave(&wifi_data->lock, flags);
	__clear_bit(*hlid, wifi_map.links_map);
	__clear_bit(*hlid, wifi_vif->links_map);
	spin_unlock_irqrestore(&wifi_data->lock, flags);

	// wifi_links[*hlid].allocated_pkts = 0;
	wifi_allocated_pkts[*hlid] = 0;
	wifi_links[*hlid].prev_freed_pkts = 0;
	wifi_links[*hlid].ba_bitmap = 0;
	eth_zero_addr(wifi_links[*hlid].addr);

	/*
	 * At this point op_tx() will not add more packets to the queues. We
	 * can purge them.
	 */
	wifi_tx_reset_link_queues(*hlid);
	wifi_links[*hlid].wifi_vif = NULL;

	wifi_links[*hlid].total_freed_pkts = 0;

	*hlid = WL12XX_INVALID_LINK_ID;
	//wifi_data->active_link_count--;
	//WARN_ON_ONCE(wifi_data->active_link_count < 0);
}

u8 wificore_get_native_channel_type(u8 nl_channel_type)
{
	switch (nl_channel_type) {
	case NL80211_CHAN_NO_HT:
		return WLCORE_CHAN_NO_HT;
	case NL80211_CHAN_HT20:
		return WLCORE_CHAN_HT20;
	case NL80211_CHAN_HT40MINUS:
		return WLCORE_CHAN_HT40MINUS;
	case NL80211_CHAN_HT40PLUS:
		return WLCORE_CHAN_HT40PLUS;
	default:
		WARN_ON(1);
		return WLCORE_CHAN_NO_HT;
	}
}
EXPORT_SYMBOL_GPL(wificore_get_native_channel_type);

int wifi_cmd_role_start_sta(struct wifi_vif *wifi_vif)
{
	struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);
	struct wifi_cmd_role_start *cmd;
	u32 supported_rates;
	int ret;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	wifi_debug(DEBUG_CMD, "cmd role start sta %d", wifi_vif->role_id);

	cmd->role_id = wifi_vif->role_id;
	if (wifi_vif->band == NL80211_BAND_5GHZ)
		cmd->band = WLCORE_BAND_5GHZ;
	cmd->channel = wifi_vif->channel;
	cmd->sta.basic_rate_set = cpu_to_le32(wifi_vif->basic_rate_set);
	cmd->sta.beacon_interval = cpu_to_le16(wifi_vif->beacon_int);
	cmd->sta.ssid_type = WL12XX_SSID_TYPE_ANY;
	cmd->sta.ssid_len = wifi_vif->ssid_len;
	memcpy(cmd->sta.ssid, wifi_vif->ssid, wifi_vif->ssid_len);
	memcpy(cmd->sta.bssid, vif->bss_conf.bssid, ETH_ALEN);

	supported_rates = CONF_TX_ENABLED_RATES | CONF_TX_MCS_RATES | wifi_vif->rate_set;
	if (wifi_vif->p2p)
		supported_rates &= ~CONF_TX_CCK_RATES;

	cmd->sta.local_rates = cpu_to_le32(supported_rates);

	cmd->channel_type = wificore_get_native_channel_type(wifi_vif->channel_type);

	if (wifi_vif->sta.hlid == WL12XX_INVALID_LINK_ID) {
		ret = wifi_allocate_link(wifi_vif, &wifi_vif->sta.hlid);
		if (ret)
			goto out_free;
	}
	cmd->sta.hlid = wifi_vif->sta.hlid;
	cmd->sta.session = wifi_session_ids[wifi_vif->sta.hlid];
	/*
	 * We don't have the correct remote rates in this stage.  The
	 * rates will be reconfigured later, after association, if the
	 * firmware supports ACX_PEER_CAP.  Otherwise, there's nothing
	 * we can do, so use all supported_rates here.
	 */
	cmd->sta.remote_rates = cpu_to_le32(supported_rates);

	ret = wifi_cmd_send(CMD_ROLE_START, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("failed to initiate cmd role start sta");
		goto err_hlid;
	}

	goto out_free;

err_hlid:
	/* clear links on error. */
	wifi_free_link(wifi_vif, &wifi_vif->sta.hlid);

out_free:
	kfree(cmd);

out:
	return ret;
}

/* use this function to stop ibss as well */
int wifi_cmd_role_stop_sta(struct wifi_vif *wifi_vif)
{
	struct wifi_cmd_role_stop *cmd;
	int ret;

	if (WARN_ON(wifi_vif->sta.hlid == WL12XX_INVALID_LINK_ID))
		return -EINVAL;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	wifi_debug(DEBUG_CMD, "cmd role stop sta %d", wifi_vif->role_id);

	cmd->role_id = wifi_vif->role_id;
	cmd->disc_type = DISCONNECT_IMMEDIATE;
	cmd->reason = cpu_to_le16(WLAN_REASON_UNSPECIFIED);

	ret = wifi_cmd_send(CMD_ROLE_STOP, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("failed to initiate cmd role stop sta");
		goto out_free;
	}

	wifi_free_link(wifi_vif, &wifi_vif->sta.hlid);

out_free:
	kfree(cmd);

out:
	return ret;
}

/**
 * wifi_cmd_interrogate - read acx from firmware
 *
 * @wl: wl struct
 * @id: acx id
 * @buf: buffer for the response, including all headers, must work with dma
 * @cmd_len: length of command
 * @res_len: length of payload
 */
int wifi_cmd_interrogate(u16 id, void *buf,
			   size_t cmd_len, size_t res_len)
{
	struct acx_header *acx = buf;
	int ret;

	wifi_debug(DEBUG_CMD, "cmd interrogate");

	acx->id = cpu_to_le16(id);

	/* response payload length, does not include any headers */
	acx->len = cpu_to_le16(res_len - sizeof(*acx));

	ret = wifi_cmd_send(CMD_INTERROGATE, acx, cmd_len, res_len);
	if (ret < 0)
		wifi_error("INTERROGATE command failed");

	return ret;
}

int wifi_cmd_data_path(bool enable)
{
	struct cmd_enabledisable_path *cmd;
	int ret;
	u16 cmd_rx, cmd_tx;

	wifi_debug(DEBUG_CMD, "cmd data path");

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	/* the channel here is only used for calibration, so hardcoded to 1 */
	cmd->channel = 1;

	if (enable) {
		cmd_rx = CMD_ENABLE_RX;
		cmd_tx = CMD_ENABLE_TX;
	} else {
		cmd_rx = CMD_DISABLE_RX;
		cmd_tx = CMD_DISABLE_TX;
	}

	ret = wifi_cmd_send(cmd_rx, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("rx %s cmd for channel %d failed",
			     enable ? "start" : "stop", cmd->channel);
		goto out;
	}

	wifi_debug(DEBUG_BOOT, "rx %s cmd channel %d",
		     enable ? "start" : "stop", cmd->channel);

	ret = wifi_cmd_send(cmd_tx, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("tx %s cmd for channel %d failed",
			     enable ? "start" : "stop", cmd->channel);
		goto out;
	}

	wifi_debug(DEBUG_BOOT, "tx %s cmd channel %d",
		     enable ? "start" : "stop", cmd->channel);

out:
	kfree(cmd);
	return ret;
}
EXPORT_SYMBOL_GPL(wifi_cmd_data_path);

int wifi_cmd_ps_mode(struct wifi_vif *wifi_vif,
		       u8 ps_mode, u16 auto_ps_timeout)
{
	struct wifi_cmd_ps_params *ps_params = NULL;
	int ret = 0;

	wifi_debug(DEBUG_CMD, "cmd set ps mode");

	ps_params = kzalloc(sizeof(*ps_params), GFP_KERNEL);
	if (!ps_params) {
		ret = -ENOMEM;
		goto out;
	}

	ps_params->role_id = wifi_vif->role_id;
	ps_params->ps_mode = ps_mode;
	ps_params->auto_ps_timeout = auto_ps_timeout;

	ret = wifi_cmd_send(CMD_SET_PS_MODE, ps_params,
			      sizeof(*ps_params), 0);
	if (ret < 0) {
		wifi_error("cmd set_ps_mode failed");
		goto out;
	}

out:
	kfree(ps_params);
	return ret;
}

int wifi_cmd_template_set(u8 role_id,
			    u16 template_id, void *buf, size_t buf_len,
			    int index, u32 rates)
{
	struct wifi_cmd_template_set *cmd;
	int ret = 0;

	wifi_debug(DEBUG_CMD, "cmd template_set %d (role %d)",
		     template_id, role_id);

	WARN_ON(buf_len > WL1271_CMD_TEMPL_MAX_SIZE);
	buf_len = min_t(size_t, buf_len, WL1271_CMD_TEMPL_MAX_SIZE);

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	/* during initialization wifi_vif is NULL */
	cmd->role_id = role_id; // role_id here is link id
	cmd->len = cpu_to_le16(buf_len);
	cmd->template_type = template_id;
	cmd->enabled_rates = cpu_to_le32(rates);
		// .tmpl_short_retry_limit      = 10,
		// .tmpl_long_retry_limit       = 10,
	cmd->short_retry_limit = 10;
	cmd->long_retry_limit = 10;
	cmd->index = index;

	if (buf)
		memcpy(cmd->template_data, buf, buf_len);

	ret = wifi_cmd_send(CMD_SET_TEMPLATE, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_warning("cmd set_template failed: %d", ret);
		goto out_free;
	}

out_free:
	kfree(cmd);

out:
	return ret;
}

int wifi_cmd_build_null_data(struct wifi_vif *wifi_vif)
{
	struct sk_buff *skb = NULL;
	int size;
	void *ptr;
	int ret = -ENOMEM;


	if (wifi_vif->bss_type == BSS_TYPE_IBSS) {
		size = sizeof(struct wifi_null_data_template);
		ptr = NULL;
	} else {
		skb = ieee80211_nullfunc_get(wifi_data->hw,
					     wifi_wifi_vif_to_vif(wifi_vif),
					     false);
		if (!skb)
			goto out;
		size = skb->len;
		ptr = skb->data;
	}

	ret = wifi_cmd_template_set(wifi_vif->role_id,
				      CMD_TEMPL_NULL_DATA, ptr, size, 0,
				      wifi_vif->basic_rate);

out:
	dev_kfree_skb(skb);
	if (ret)
		wifi_warning("cmd build null data failed %d", ret);

	return ret;

}

int wifi_cmd_build_klv_null_data(
				   struct wifi_vif *wifi_vif)
{
	struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);
	struct sk_buff *skb = NULL;
	int ret = -ENOMEM;

	skb = ieee80211_nullfunc_get(wifi_data->hw, vif, false);
	if (!skb)
		goto out;

	ret = wifi_cmd_template_set(wifi_vif->role_id, CMD_TEMPL_KLV,
				      skb->data, skb->len,
					  STA_KLV_TEMPLATE_IDX,
				      wifi_vif->basic_rate);

out:
	dev_kfree_skb(skb);
	if (ret)
		wifi_warning("cmd build klv null data failed %d", ret);

	return ret;

}

int wifi_cmd_build_ps_poll(struct wifi_vif *wifi_vif,
			     u16 aid)
{
	struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);
	struct sk_buff *skb;
	int ret = 0;

	skb = ieee80211_pspoll_get(wifi_data->hw, vif);
	if (!skb)
		goto out;

	//wifi_vif->basic_rate_set here is 1
	ret = wifi_cmd_template_set(wifi_vif->role_id,
				      CMD_TEMPL_PS_POLL, skb->data,
				      skb->len, 0, wifi_vif->basic_rate_set);

out:
	dev_kfree_skb(skb);
	return ret;
}

int wifi_cmd_build_probe_req(struct wifi_vif *wifi_vif,
			       u8 role_id, u8 band,
			       const u8 *ssid, size_t ssid_len,
			       const u8 *ie0, size_t ie0_len, const u8 *ie1,
			       size_t ie1_len, bool sched_scan)
{
	struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);
	struct sk_buff *skb;
	int ret;
	u32 rate;
	u16 template_id_2_4 = CMD_TEMPL_CFG_PROBE_REQ_2_4;
	u16 template_id_5 = CMD_TEMPL_CFG_PROBE_REQ_5;

	wifi_debug(DEBUG_SCAN, "build probe request band %d", band);

	skb = ieee80211_probereq_get(wifi_data->hw, vif->addr, ssid, ssid_len,
				     ie0_len + ie1_len);
	if (!skb) {
		ret = -ENOMEM;
		goto out;
	}
	if (ie0_len)
		skb_put_data(skb, ie0, ie0_len);
	if (ie1_len)
		skb_put_data(skb, ie1, ie1_len);


	rate = wifi_tx_min_rate_get(wifi_vif->bitrate_masks[band]);
#if (PRINT_DEBUG)
	printk("rate = %d from %d\n", rate, wifi_vif->bitrate_masks[band]);
#endif
	if (band == NL80211_BAND_2GHZ)
		ret = wifi_cmd_template_set(role_id,
					      template_id_2_4,
					      skb->data, skb->len, 0, rate);
	else
		ret = wifi_cmd_template_set(role_id,
					      template_id_5,
					      skb->data, skb->len, 0, rate);

out:
	dev_kfree_skb(skb);
	return ret;
}
EXPORT_SYMBOL_GPL(wifi_cmd_build_probe_req);

int wifi_cmd_build_arp_rsp(struct wifi_vif *wifi_vif)
{
	int ret, extra = 0;
	u16 fc;
	struct ieee80211_vif *vif = wifi_wifi_vif_to_vif(wifi_vif);
	struct sk_buff *skb;
	struct wifi_arp_rsp_template *tmpl;
	struct ieee80211_hdr_3addr *hdr;
	struct arphdr *arp_hdr;

	skb = dev_alloc_skb(sizeof(*hdr) + sizeof(__le16) + sizeof(*tmpl) +
			    WL1271_EXTRA_SPACE_MAX);
	if (!skb) {
		wifi_error("failed to allocate buffer for arp rsp template");
		return -ENOMEM;
	}

	skb_reserve(skb, sizeof(*hdr) + WL1271_EXTRA_SPACE_MAX);

	tmpl = skb_put_zero(skb, sizeof(*tmpl));

	/* llc layer */
	memcpy(tmpl->llc_hdr, rfc1042_header, sizeof(rfc1042_header));
	tmpl->llc_type = cpu_to_be16(ETH_P_ARP);

	/* arp header */
	arp_hdr = &tmpl->arp_hdr;
	arp_hdr->ar_hrd = cpu_to_be16(ARPHRD_ETHER);
	arp_hdr->ar_pro = cpu_to_be16(ETH_P_IP);
	arp_hdr->ar_hln = ETH_ALEN;
	arp_hdr->ar_pln = 4;
	arp_hdr->ar_op = cpu_to_be16(ARPOP_REPLY);

	/* arp payload */
	memcpy(tmpl->sender_hw, vif->addr, ETH_ALEN);
	tmpl->sender_ip = wifi_vif->ip_addr;

	/* encryption space */
	switch (wifi_vif->encryption_type) {
	case KEY_TKIP:
		if (wifi_data->quirks & WLCORE_QUIRK_TKIP_HEADER_SPACE)
			extra = WL1271_EXTRA_SPACE_TKIP;
		break;
	case KEY_AES:
		extra = WL1271_EXTRA_SPACE_AES;
		break;
	case KEY_NONE:
	case KEY_WEP:
	case KEY_GEM:
		extra = 0;
		break;
	default:
		wifi_warning("Unknown encryption type: %d",
			       wifi_vif->encryption_type);
		ret = -EINVAL;
		goto out;
	}

	if (extra) {
		u8 *space = skb_push(skb, extra);
		memset(space, 0, extra);
	}

	/* QoS header - BE */
	if (wifi_vif->sta.qos)
		memset(skb_push(skb, sizeof(__le16)), 0, sizeof(__le16));

	/* mac80211 header */
	hdr = skb_push(skb, sizeof(*hdr));
	memset(hdr, 0, sizeof(*hdr));
	fc = IEEE80211_FTYPE_DATA | IEEE80211_FCTL_TODS;
	if (wifi_vif->sta.qos)
		fc |= IEEE80211_STYPE_QOS_DATA;
	else
		fc |= IEEE80211_STYPE_DATA;
	if (wifi_vif->encryption_type != KEY_NONE)
		fc |= IEEE80211_FCTL_PROTECTED;

	hdr->frame_control = cpu_to_le16(fc);
	memcpy(hdr->addr1, vif->bss_conf.bssid, ETH_ALEN);
	memcpy(hdr->addr2, vif->addr, ETH_ALEN);
	eth_broadcast_addr(hdr->addr3);

	ret = wifi_cmd_template_set(wifi_vif->role_id, CMD_TEMPL_ARP_RSP,
				      skb->data, skb->len, 0,
				      wifi_vif->basic_rate);
out:
	dev_kfree_skb(skb);
	return ret;
}

int wifi_build_qos_null_data(struct ieee80211_vif *vif)
{
	struct wifi_vif *wifi_vif = wifi_vif_to_data(vif);
	struct ieee80211_qos_hdr template;

	memset(&template, 0, sizeof(template));

	memcpy(template.addr1, vif->bss_conf.bssid, ETH_ALEN);
	memcpy(template.addr2, vif->addr, ETH_ALEN);
	memcpy(template.addr3, vif->bss_conf.bssid, ETH_ALEN);

	template.frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA |
					     IEEE80211_STYPE_QOS_NULLFUNC |
					     IEEE80211_FCTL_TODS);

	/* FIXME: not sure what priority to use here */
	template.qos_ctrl = cpu_to_le16(0);

	return wifi_cmd_template_set(wifi_vif->role_id,
				       CMD_TEMPL_QOS_NULL_DATA, &template,
				       sizeof(template), 0,
				       wifi_vif->basic_rate);
}

int wifi_cmd_set_sta_key(struct wifi_vif *wifi_vif,
		       u16 action, u8 id, u8 key_type,
		       u8 key_size, const u8 *key, const u8 *addr,
		       u32 tx_seq_32, u16 tx_seq_16)
{
	struct wifi_cmd_set_keys *cmd;
	int ret = 0;

	/* hlid might have already been deleted */
	if (wifi_vif->sta.hlid == WL12XX_INVALID_LINK_ID)
		return 0;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->hlid = wifi_vif->sta.hlid;

	if (key_type == KEY_WEP)
		cmd->lid_key_type = WEP_DEFAULT_LID_TYPE;
	else if (is_broadcast_ether_addr(addr))
		cmd->lid_key_type = BROADCAST_LID_TYPE;
	else
		cmd->lid_key_type = UNICAST_LID_TYPE;

	cmd->key_action = cpu_to_le16(action);
	cmd->key_size = key_size;
	cmd->key_type = key_type;

	cmd->ac_seq_num16[0] = cpu_to_le16(tx_seq_16);
	cmd->ac_seq_num32[0] = cpu_to_le32(tx_seq_32);

	cmd->key_id = id;

	if (key_type == KEY_TKIP) {
		/*
		 * We get the key in the following form:
		 * TKIP (16 bytes) - TX MIC (8 bytes) - RX MIC (8 bytes)
		 * but the target is expecting:
		 * TKIP - RX MIC - TX MIC
		 */
		memcpy(cmd->key, key, 16);
		memcpy(cmd->key + 16, key + 24, 8);
		memcpy(cmd->key + 24, key + 16, 8);

	} else {
		memcpy(cmd->key, key, key_size);
	}

	wifi_dump(DEBUG_CRYPT, "TARGET KEY: ", cmd, sizeof(*cmd));

	ret = wifi_cmd_send(CMD_SET_KEYS, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_warning("could not set keys");
		goto out;
	}

out:
	kfree(cmd);

	return ret;
}

int wifi_cmd_set_peer_state(struct wifi_vif *wifi_vif,
			      u8 hlid)
{
	struct wifi_cmd_set_peer_state *cmd;
	int ret = 0;

	wifi_debug(DEBUG_CMD, "cmd set peer state (hlid=%d)", hlid);

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->hlid = hlid;
	cmd->state = WL1271_CMD_STA_STATE_CONNECTED;

	/* wmm param is valid only for station role */
	if (wifi_vif->bss_type == BSS_TYPE_STA_BSS)
		cmd->wmm = wifi_vif->wmm_enabled;

	ret = wifi_cmd_send(CMD_SET_PEER_STATE, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("failed to send set peer state command");
		goto out_free;
	}

out_free:
	kfree(cmd);

out:
	return ret;
}

static int wificore_get_reg_conf_ch_idx(enum nl80211_band band, u16 ch)
{
	/*
	 * map the given band/channel to the respective predefined
	 * bit expected by the fw
	 */
	switch (band) {
	case NL80211_BAND_2GHZ:
		/* channels 1..14 are mapped to 0..13 */
		if (ch >= 1 && ch <= 14)
			return ch - 1;
		break;
	case NL80211_BAND_5GHZ:
		switch (ch) {
		case 8 ... 16:
			/* channels 8,12,16 are mapped to 18,19,20 */
			return 18 + (ch-8)/4;
		case 34 ... 48:
			/* channels 34,36..48 are mapped to 21..28 */
			return 21 + (ch-34)/2;
		case 52 ... 64:
			/* channels 52,56..64 are mapped to 29..32 */
			return 29 + (ch-52)/4;
		case 100 ... 140:
			/* channels 100,104..140 are mapped to 33..43 */
			return 33 + (ch-100)/4;
		case 149 ... 165:
			/* channels 149,153..165 are mapped to 44..48 */
			return 44 + (ch-149)/4;
		default:
			break;
		}
		break;
	default:
		break;
	}

	wifi_error("%s: unknown band/channel: %d/%d", __func__, band, ch);
	return -1;
}

void wificore_set_pending_regdomain_ch(u16 channel,
				     enum nl80211_band band)
{
	int ch_bit_idx = 0;

	// if (!(wifi_data->quirks & WLCORE_QUIRK_REGDOMAIN_CONF))
	// 	return;

	ch_bit_idx = wificore_get_reg_conf_ch_idx(band, channel);

	if (ch_bit_idx >= 0 && ch_bit_idx <= WL1271_MAX_CHANNELS)
		__set_bit_le(ch_bit_idx, (long *)wifi_data->reg_ch_conf_pending);
}

int wificore_cmd_regdomain_config_locked(void)
{
	struct wifi_cmd_regdomain_dfs_config *cmd = NULL;
	int ret = 0, i, b, ch_bit_idx;
	__le32 tmp_ch_bitmap[2] __aligned(sizeof(unsigned long));
	struct wiphy *wiphy = wifi_data->hw->wiphy;
	struct ieee80211_supported_band *band;
	bool timeout = false;

	// if (!(wifi_data->quirks & WLCORE_QUIRK_REGDOMAIN_CONF))
	// 	return 0;

	wifi_debug(DEBUG_CMD, "cmd reg domain config");

	memcpy(tmp_ch_bitmap, wifi_data->reg_ch_conf_pending, sizeof(tmp_ch_bitmap));

	for (b = NL80211_BAND_2GHZ; b <= NL80211_BAND_5GHZ; b++) {
		band = wiphy->bands[b];
		for (i = 0; i < band->n_channels; i++) {
			struct ieee80211_channel *channel = &band->channels[i];
			u16 ch = channel->hw_value;
			u32 flags = channel->flags;

			if (flags & (IEEE80211_CHAN_DISABLED |
				     IEEE80211_CHAN_NO_IR))
				continue;

			if ((flags & IEEE80211_CHAN_RADAR) &&
			    channel->dfs_state != NL80211_DFS_AVAILABLE)
				continue;

			ch_bit_idx = wificore_get_reg_conf_ch_idx(b, ch);
			if (ch_bit_idx < 0)
				continue;

			__set_bit_le(ch_bit_idx, (long *)tmp_ch_bitmap);
		}
	}

	if (!memcmp(tmp_ch_bitmap, wifi_data->reg_ch_conf_last, sizeof(tmp_ch_bitmap)))
		goto out;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->ch_bit_map1 = tmp_ch_bitmap[0];
	cmd->ch_bit_map2 = tmp_ch_bitmap[1];
	cmd->dfs_region = wifi_data->dfs_region;

	wifi_debug(DEBUG_CMD,
		     "cmd reg domain bitmap1: 0x%08x, bitmap2: 0x%08x",
		     cmd->ch_bit_map1, cmd->ch_bit_map2);

	ret = wifi_cmd_send(CMD_DFS_CHANNEL_CONFIG, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("failed to send reg domain dfs config");
		goto out;
	}

	ret = wifi_wait_for_event(
				      WLCORE_EVENT_DFS_CONFIG_COMPLETE,
				      &timeout);
	if (ret < 0 || timeout) {
		wifi_error("reg domain conf %serror",
			     timeout ? "completion " : "");
		ret = timeout ? -ETIMEDOUT : ret;
		goto out;
	}

	memcpy(wifi_data->reg_ch_conf_last, tmp_ch_bitmap, sizeof(tmp_ch_bitmap));
	memset(wifi_data->reg_ch_conf_pending, 0, sizeof(wifi_data->reg_ch_conf_pending));

out:
	kfree(cmd);
	return ret;
}

int wifi_cmd_config_fwlog(void)
{
	struct wifi_cmd_config_fwlog *cmd;
	int ret = 0;

	wifi_debug(DEBUG_CMD, "cmd config firmware logger");

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->logger_mode = wifi_data->conf.fwlog.mode;
	cmd->log_severity = wifi_data->conf.fwlog.severity;
	cmd->timestamp = wifi_data->conf.fwlog.timestamp;
	cmd->output = wifi_data->conf.fwlog.output;
	cmd->threshold = wifi_data->conf.fwlog.threshold;

	ret = wifi_cmd_send(CMD_CONFIG_FWLOGGER, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("failed to send config firmware logger command");
		goto out_free;
	}

out_free:
	kfree(cmd);

out:
	return ret;
}

int wifi_cmd_stop_channel_switch(struct wifi_vif *wifi_vif)
{
	struct wifi_cmd_stop_channel_switch *cmd;
	int ret;

	wifi_debug(DEBUG_ACX, "cmd stop channel switch");

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->role_id = wifi_vif->role_id;

	ret = wifi_cmd_send(CMD_STOP_CHANNEL_SWICTH, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wifi_error("failed to stop channel switch command");
		goto out_free;
	}

out_free:
	kfree(cmd);

out:
	return ret;
}

/* Vinh custom */
int wifi_roc(struct wifi_vif *wifi_vif, u8 role_id,
	       enum nl80211_band band, u8 channel)
{
	int ret = 0;
	struct wifi_cmd_roc cmd;

	if (test_bit(role_id, wifi_data->roc_map)){
		return 0;
	}

	cmd.role_id = role_id;
	cmd.channel = channel;
	switch (band) {
		case NL80211_BAND_2GHZ:
			cmd.band = WLCORE_BAND_2_4GHZ;
			break;
		case NL80211_BAND_5GHZ:
			cmd.band = WLCORE_BAND_5GHZ;
			break;
		default:
			break;
    }
    ret = wifi_cmd_send(CMD_REMAIN_ON_CHANNEL, &cmd, sizeof(cmd), 0);
	if (ret < 0)
		goto out;

	__set_bit(role_id, wifi_data->roc_map);
out:
	return ret;
}

int wifi_crocV(u8 role_id)
{
	int ret = 0;
	struct wifi_cmd_croc cmd;

	if ((!test_bit(role_id, wifi_data->roc_map))){
        printk("ALREADY - wifi_crocV\n");
        return 0;
    }

    cmd.role_id = role_id;
	ret = wifi_cmd_send(CMD_CANCEL_REMAIN_ON_CHANNEL, &cmd, sizeof(cmd), 0);
	if (ret < 0)
		goto out;

	__clear_bit(role_id, wifi_data->roc_map);

	/*
	 * Rearm the tx watchdog when removing the last ROC. This prevents
	 * recoveries due to just finished ROCs - when Tx hasn't yet had
	 * a chance to get out.
	 */
	if (find_first_bit(wifi_data->roc_map, WL12XX_MAX_ROLES) >= WL12XX_MAX_ROLES)
		wifi_rearm_tx_watchdog_locked();
out:
	return ret;
}

int wifi_set_authorized(struct wifi_vif *wifi_vif)
{
	int ret;

	if (WARN_ON(wifi_vif->bss_type != BSS_TYPE_STA_BSS))
		return -EINVAL;

	// Make sure that before authorized, association is set
	if (!test_bit(wifi_vif_FLAG_STA_ASSOCIATED, &wifi_vif->flags)){
		return 0;
	}

	if (test_and_set_bit(wifi_vif_FLAG_STA_STATE_SENT, &wifi_vif->flags))
		return 0;

	ret = wifi_cmd_set_peer_state(wifi_vif, wifi_vif->sta.hlid);
	if (ret < 0)
		return ret;

	wifi_info("Association completed.");
	return 0;
}