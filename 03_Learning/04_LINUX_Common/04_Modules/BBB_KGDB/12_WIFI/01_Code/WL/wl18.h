#ifndef WL18_H
#define WL18_H

#include "cmd.h"
#include "debug.h"
#include "acx.h"

#include "common.h"

enum {
	ACX_NS_IPV6_FILTER		 = 0x0050,
	ACX_PEER_HT_OPERATION_MODE_CFG	 = 0x0051,
	ACX_CSUM_CONFIG			 = 0x0052,
	ACX_SIM_CONFIG			 = 0x0053,
	ACX_CLEAR_STATISTICS		 = 0x0054,
	ACX_AUTO_RX_STREAMING		 = 0x0055,
	ACX_PEER_CAP			 = 0x0056,
	ACX_INTERRUPT_NOTIFY		 = 0x0057,
	ACX_RX_BA_FILTER		 = 0x0058,
	ACX_AP_SLEEP_CFG                 = 0x0059,
	ACX_DYNAMIC_TRACES_CFG		 = 0x005A,
	ACX_TIME_SYNC_CFG		 = 0x005B,
};

/* cac_start and cac_stop share the same params */
struct wifi_cmd_cac_start {
	struct wifi_cmd_header header;

	u8 role_id;
	u8 channel;
	u8 band;
	u8 bandwidth;
} __packed;

struct wl18xx_cmd_smart_config_set_group_key {
	struct wifi_cmd_header header;

	__le32 group_id;

	u8 key[16];
} __packed;


/* Target's information element */
struct acx1_header {
	struct wifi_cmd_header cmd;

	/* acx (or information element) header */
	__le16 id;

	/* payload length (not including headers */
	__le16 len;
} __packed;
/*
 * ACX_PEER_CAP
 * this struct is very similar to wifi_acx_ht_capabilities, with the
 * addition of supported rates
 */
struct wifi_acx_peer_cap {
	struct acx1_header header;

	/* bitmask of capability bits supported by the peer */
	__le32 ht_capabilites;

	/* rates supported by the remote peer */
	__le32 supported_rates;

	/* Indicates to which link these capabilities apply. */
	u8 hlid;

	/*
	 * This the maximum A-MPDU length supported by the AP. The FW may not
	 * exceed this length when sending A-MPDUs
	 */
	u8 ampdu_max_length;

	/* This is the minimal spacing required when sending A-MPDUs to the AP*/
	u8 ampdu_min_spacing;

	u8 padding;
} __packed;
int wifi_acx_set_peer_cap(
			    struct ieee80211_sta_ht_cap *ht_cap,
			    bool allow_ht_operation,
			    u32 rate_set, u8 hlid);

#define WL18XX_PHY_VERSION_MAX_LEN 20
struct wifi_static_data_priv {
	char phy_version[WL18XX_PHY_VERSION_MAX_LEN];
};
int wifi_handle_static_data(struct wifi_static_data *static_data);

#endif