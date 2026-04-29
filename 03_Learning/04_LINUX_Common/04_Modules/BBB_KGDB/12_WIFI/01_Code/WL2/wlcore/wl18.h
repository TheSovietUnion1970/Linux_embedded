#ifndef WL18_H
#define WL18_H

#include "cmd.h"
#include "debug.h"
#include "acx.h"

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
struct VV_cmd_cac_start {
	struct wl1271_cmd_header header;

	u8 role_id;
	u8 channel;
	u8 band;
	u8 bandwidth;
} __packed;
int VV_cmd_set_cac(struct wl1271 *wl, struct wl12xx_vif *wlvif, bool start);


struct wl18xx_cmd_smart_config_set_group_key {
	struct wl1271_cmd_header header;

	__le32 group_id;

	u8 key[16];
} __packed;
int VV_cmd_smart_config_set_group_key(struct wl1271 *wl, u16 group_id,
					  u8 key_len, u8 *key);
int VV_cmd_smart_config_stop(struct wl1271 *wl);



struct VV_cmd_smart_config_start {
	struct wl1271_cmd_header header;

	__le32 group_id_bitmask;
} __packed;
int VV_cmd_smart_config_start(struct wl1271 *wl, u32 group_bitmap);


/* Target's information element */
struct acx1_header {
	struct wl1271_cmd_header cmd;

	/* acx (or information element) header */
	__le16 id;

	/* payload length (not including headers */
	__le16 len;
} __packed;
/*
 * ACX_PEER_CAP
 * this struct is very similar to wl1271_acx_ht_capabilities, with the
 * addition of supported rates
 */
struct VV_acx_peer_cap {
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
int VV_acx_set_peer_cap(struct wl1271 *wl,
			    struct ieee80211_sta_ht_cap *ht_cap,
			    bool allow_ht_operation,
			    u32 rate_set, u8 hlid);

#define WL18XX_PHY_VERSION_MAX_LEN 20
struct VV_static_data_priv {
	char phy_version[WL18XX_PHY_VERSION_MAX_LEN];
};
int VV_handle_static_data(struct wl1271 *wl,
				     struct wl1271_static_data *static_data);

#endif