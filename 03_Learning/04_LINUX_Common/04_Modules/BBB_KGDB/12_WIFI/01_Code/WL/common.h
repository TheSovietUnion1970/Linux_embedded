#ifndef COMMON_H
#define COMMON_H

#define STA_BASIC_RATE_IDX          0
#define STA_AP_RATE_IDX             1
#define STA_P2P_RATE_IDX            2

#define STA_KLV_TEMPLATE_IDX        0

#define STA_ROLE_ID 1
#define P2P_ROLE_ID 2

// Each HW Link ID is assigned to each connection (STA or AP)
#define HW_LINK_ID 1 // (< WL18XX_MAX_LINKS 16)
        // must be non-zero as WL12XX_SYSTEM_HLID = 0 is existed

// Per-if has 4 queues (VO, VI, BE and BK)
#define Q_BASE 0
#define HW_QUEUE_BASE  Q_BASE*4 // (0*NUM_TX_QUEUES)

extern struct sk_buff_head wifi_tx_queue[WLCORE_MAX_LINKS][NUM_TX_QUEUES];
/* Frames received, not handled yet by mac80211 */
extern struct sk_buff_head wifi_deferred_rx_queue;
/* Frames sent, not returned yet to mac80211 */
extern struct sk_buff_head wifi_deferred_tx_queue;

extern int wifi_tx_queue_count[NUM_TX_QUEUES]; /* Frames scheduled for transmission, not handled yet */ 
        // incremented - wifi_op_tx, wifi_tx_dummy_packet
        // decremented - wificore_lnk_dequeue

extern u8 wifi_allocated_pkts[WLCORE_MAX_LINKS]; // refer to links_map

/* Accounting for allocated / available Tx packets in HW */
extern u32 wifi_tx_pkts_freed[NUM_TX_QUEUES];
extern u32 wifi_tx_allocated_pkts[NUM_TX_QUEUES];
extern u32 wifi_tx_allocated_blocks; // new - last (released blks)
        // incremented - allocate hw
        // decremented - TX interupt
extern u32 wifi_tx_blocks_available; // get from old val or updated (tx total - wifi_tx_allocated_blocks)
                                   // = the available slot where blcks can be allocated
        // incremented - max (old, tx total - allocated)
        // decremented - allocate hw  
extern u32 wifi_tx_packets_count; 
extern u8 wifi_last_fw_rls_idx; // it's incremented every TX interrupt
        
extern struct sk_buff *wifi_skb_tx_frames[WLCORE_MAX_TX_DESCRIPTORS];
        // ptr to skb per tx desc
extern struct sk_buff *wifi_dummy_packet;
extern int wifi_skb_tx_frames_cnt;
extern u32 wifi_last_updated_tmp_tx_blocks_freed;
extern u8 *wifi_aggr_buf;

/* session_id for starting sta role, configure the tx attributes (tx_attr = session_id << TX_HW_ATTR_OFST_SESSION_COUNTER;) */
extern u8 wifi_session_ids[WLCORE_MAX_LINKS];
extern s64 wifi_time_offset; /* Time-offset between host and chipset clocks */

/* FW Rx counter */
extern u32 wifi_rx_counter;
extern bool wifi_scan_failed;

#define WL18XX_NUM_RX_DESCRIPTORS 32
#define WL18XX_MAX_LINKS 16
#define WL18XX_FW_MAX_TX_STATUS_DESC 33
/* FW status registers */
struct wifi_wl18xx_fw_status {
	__le32 intr;
	u8  fw_rx_counter;
	u8  drv_rx_counter;
	u8  reserved;
	u8  tx_results_counter;
	__le32 rx_pkt_descs[WL18XX_NUM_RX_DESCRIPTORS];

	__le32 fw_localtime;

	/*
	 * A bitmap (where each bit represents a single HLID)
	 * to indicate if the station is in PS mode.
	 */
	__le32 link_ps_bitmap;

	/*
	 * A bitmap (where each bit represents a single HLID) to indicate
	 * if the station is in Fast mode
	 */
	__le32 link_fast_bitmap;

	/* Cumulative counter of total released mem blocks since FW-reset */
	__le32 total_released_blks;

	/* Size (in Memory Blocks) of TX pool */
	__le32 tx_total;

	//struct wl18xx_fw_packet_counters counters;
	/* Cumulative counter of released packets per AC */
	u8 tx_released_pkts[NUM_TX_QUEUES];

	/* Cumulative counter of freed packets per HLID */
	u8 tx_lnk_free_pkts[WL18XX_MAX_LINKS];

	/* Cumulative counter of released Voice memory blocks */
	u8 tx_voice_released_blks;

	/* Tx rate of the last transmitted packet */
	u8 tx_last_rate;

	/* Tx rate or Tx rate estimate pre-calculated by fw in mbps units */
	u8 tx_last_rate_mbps;

	/* hlid for which the rates were reported */
	u8 hlid;
    // ====

	__le32 log_start_addr;

	/* Private status to be used by the lower drivers */
	//struct wl18xx_fw_status_priv priv;
	/*
	 * Index in released_tx_desc for first byte that holds
	 * released tx host desc
	 */
	u8 fw_release_idx;

	/*
	 * Array of host Tx descriptors, where fw_release_idx
	 * indicated the first released idx.
	 */
	u8 released_tx_desc[WL18XX_FW_MAX_TX_STATUS_DESC];

	/* A bitmap representing the currently suspended links. The suspend
	 * is short lived, for multi-channel Tx requirements.
	 */
	__le32 link_suspend_bitmap;

	/* packet threshold for an "almost empty" AC,
	 * for Tx schedulng purposes
	 */
	u8 tx_ac_threshold;

	/* number of packets to queue up for a link in PS */
	u8 tx_ps_threshold;

	/* number of packet to queue up for a suspended link */
	u8 tx_suspend_threshold;

	/* Should have less than this number of packets in queue of a slow
	 * link to qualify as high priority link
	 */
	u8 tx_slow_link_prio_threshold;

	/* Should have less than this number of packets in queue of a fast
	 * link to qualify as high priority link
	 */
	u8 tx_fast_link_prio_threshold;

	/* Should have less than this number of packets in queue of a slow
	 * link before we stop queuing up packets for it.
	 */
	u8 tx_slow_stop_threshold;

	/* Should have less than this number of packets in queue of a fast
	 * link before we stop queuing up packets for it.
	 */
	u8 tx_fast_stop_threshold;

	u8 padding[3];
} __packed;
extern struct wifi_wl18xx_fw_status* wifi_status_reg;

#define WLCORE_MAX_LINKS 16

struct wifi_vif;
struct wifi_link {
	/* AP-mode - TX queue per AC in link */
	//struct sk_buff_head tx_queue[NUM_TX_QUEUES];

	/* accounting for allocated / freed packets in FW */
	//u8 allocated_pkts;
	u8 prev_freed_pkts;

	u8 addr[6];

	/* bitmap of TIDs where RX BA sessions are active for this link */
	u8 ba_bitmap;

	/* the last fw rate index we used for this link */
	u8 fw_rate_idx;

	/* the last fw rate [Mbps] we used for this link */
	u8 fw_rate_mbps;

	/* The wifi_vif this link belongs to. Might be null for global links */
	struct wifi_vif *wifi_vif;
	/*
	 * total freed FW packets on the link - used for tracking the
	 * AES/TKIP PN across recoveries. Re-initialized each time
	 * from the wifi_station structure.
	 */
	u64 total_freed_pkts;
};
extern struct wifi_link wifi_links[WLCORE_MAX_LINKS];

struct wifi_Work {
	struct workqueue_struct *freezable_wq;

	/* Network stack work  */
	struct work_struct netstack_work;

    struct work_struct tx_work;

	/* work to fire when Tx is stuck */
	struct delayed_work tx_watchdog_work;

    struct delayed_work scan_complete_work;

    /* Temporary use */
    struct ieee80211_hw *hw;
};
extern struct wifi_Work wifi_work;

struct wifi_vif {
	struct wl1271 *wl;
	unsigned long flags;
	u8 bss_type;
	u8 p2p; /* we are using p2p role */
	u8 role_id;

	/* sta/ibss specific */
	u8 dev_role_id;
	u8 dev_hlid;

	union {
		struct {
			u8 hlid;
			bool qos;
		} sta;
	};

	unsigned long links_map[BITS_TO_LONGS(WLCORE_MAX_LINKS)];

	u8 ssid[IEEE80211_MAX_SSID_LEN + 1];
	u8 ssid_len;

	/* The current band */
	enum nl80211_band band;
	int channel;
	enum nl80211_channel_type channel_type;

	u32 bitrate_masks[WLCORE_NUM_BANDS];
	u32 basic_rate_set;

	/*
	 * currently configured rate set:
	 *	bits  0-15 - 802.11abg rates
	 *	bits 16-23 - 802.11n   MCS index mask
	 * support only 1 stream, thus only 8 bits for the MCS rates (0-7).
	 */
	u32 basic_rate;
	u32 rate_set;

	/* probe-req template for the current AP */
	struct sk_buff *probereq;

	/* Beaconing interval (needed for ad-hoc) */
	u32 beacon_int;

	/* Our association ID */
	u16 aid;

	/* in dBm */
	int power_level;

	/* save the current encryption type for auto-arp config */
	u8 encryption_type;
	__be32 ip_addr;

	bool wmm_enabled;

	/*
	 * total freed FW packets on the link.
	 * For STA this holds the PN of the link to the AP.
	 * For AP this holds the PN of the broadcast link.
	 */
	u64 total_freed_pkts;

	/*
	 * This struct must be last!
	 * data that has to be saved acrossed reconfigs (e.g. recovery)
	 * should be declared in this struct.
	 */
	struct {
		u8 persistent[0];
	};
};
extern struct wifi_vif* wifi_vif_ptr[10]; // buffer up to 19
extern int wifi_vif_ptr_id;

struct wifi_map {
    /* Mapping to skb per tx desc */
	unsigned long tx_frames_map[BITS_TO_LONGS(WLCORE_MAX_TX_DESCRIPTORS)];

    /* Mapping for hw link */
    unsigned long links_map[BITS_TO_LONGS(WLCORE_MAX_LINKS)];
};
extern struct wifi_map wifi_map;

struct wifi_chip {
	u32 id;
	char fw_ver_str[ETHTOOL_FWVERS_LEN];
	unsigned int fw_ver[NUM_FW_VER];
	char phy_fw_ver_str[ETHTOOL_FWVERS_LEN];
};
extern struct wifi_chip* wifi_chip;

struct wifi_partition {
	u32 size;
	u32 start;
};

struct wifi_partition_set {
	struct wifi_partition mem;
	struct wifi_partition reg;
	struct wifi_partition mem2;
	struct wifi_partition mem3;
};

struct Wifi_data {
	struct device *dev;
    struct mutex mutex;
    spinlock_t lock;
	unsigned long flags;

    struct ieee80211_vif *vif;
    struct list_head wifi_vif_list;

	const int *rtable;
	struct wifi_partition_set curr_part;
	const struct wifi_partition_set *ptable;

	const u8 **band_rate_to_idx;
	/* Reg domain last configuration */
	DECLARE_BITMAP(reg_ch_conf_last, 64);
	/* Reg domain pending configuration */
	DECLARE_BITMAP(reg_ch_conf_pending, 64);


	/* the current dfs region */
	enum nl80211_dfs_regions dfs_region;
	int* cmd_box_addr;
	u32* mbox_ptr[2];
	/* Pointer that holds DMA-friendly block for the mailbox */
	void *mbox;

	u8 scan_state;

    /* Temporary use */
    struct wl1271 *wl;



	/* ===== */
	bool initialized;
	struct ieee80211_hw *hw;
	bool mac80211_registered;

	struct platform_device *pdev;

	struct wifi_if_operations *if_ops;

	int irq;
	int wakeirq;

	int irq_flags;

	enum wificore_state state;
	enum wifi_fw_type fw_type;
	//bool plt;

	u8 *fw;
	size_t fw_len;
	void *nvs;
	size_t nvs_len;


	/* address read from the fuse ROM */
	u32 fuse_oui_addr;
	u32 fuse_nic_addr;

	/* we have up to 2 MAC addresses */
	struct mac_address addresses[WLCORE_NUM_MAC_ADDRESSES];
	int channel;

	unsigned long links_map[BITS_TO_LONGS(WLCORE_MAX_LINKS)];
	unsigned long roc_map[BITS_TO_LONGS(WL12XX_MAX_ROLES)];


	u8 sta_count;

	struct wifi_acx_mem_map *target_mem_map;

	unsigned long queue_stop_reasons[
				NUM_TX_QUEUES * WLCORE_NUM_MAC_ADDRESSES];

	/* The mbox event mask */
	u32 event_mask;

	struct delayed_work scan_complete_work;

	struct ieee80211_vif *roc_vif;
	//struct delayed_work roc_complete_work;

	struct wifi_vif *sched_vif;

	/* Current chipset configuration */
	struct wificore_conf conf;

	bool enable_11a;

	/* bands supported by this instance of wl12xx */
	struct ieee80211_supported_band bands[WLCORE_NUM_BANDS];

	bool irq_wake_enabled;

	/* Quirks of specific hardware revisions */
	unsigned int quirks;

	/* last wifi_vif we transmitted from */
	struct wifi_vif *last_wifi_vif;

	/* work to fire when Tx is stuck */
	struct delayed_work tx_watchdog_work;

	struct wificore_ops *ops;

	const char *sr_fw_name;

	/* per-chip-family private structure */
	void *priv; // pointing to conf

	/* HW HT (11n) capabilities */
	struct ieee80211_sta_ht_cap ht_cap[WLCORE_NUM_BANDS];

	/* RX Data filter rule state - enabled/disabled */
	unsigned long rx_filter_enabled[BITS_TO_LONGS(WL1271_MAX_RX_FILTERS)];

	/* size of the private static data */
	size_t static_data_priv_len;

	/* mutex for protecting the tx_flush function */
	struct mutex flush_mutex;

	/* sleep auth value currently configured to FW */
	int sleep_auth;

	/* the number of allocated MAC addresses in this chip */
	int num_mac_addr;

	/* minimum FW version required for the driver to work in single-role */
	unsigned int min_sr_fw_ver[NUM_FW_VER];

	/* minimum FW version required for the driver to work in multi-role */
	unsigned int min_mr_fw_ver[NUM_FW_VER];

	struct completion nvs_loading_complete;

	/* interface combinations supported by the hw */
	const struct ieee80211_iface_combination *iface_combinations;
	u8 n_iface_combinations;

	void *last_valid_wifi_vif;
};
extern struct Wifi_data* wifi_data;

// static inline
// struct ieee80211_vif *wifi_wifi_vif_to_vif(int idx)
// {
// 	return container_of((void *)wifi_vif_ptr[idx], struct ieee80211_vif, drv_priv);
// }

#endif
//extern struct wifi_wl18xx_fw_status wifi_status_reg;

//              [----------DEFAULT-------]      [-------USER----]
// LINKS       0                               1                      2  ... 15         
// QUEUE        [VO     VI     BE     BK]       [VO,VI,BE,BK] ...
//               |       |      |     |
// REASONs     4-bit   4-bit   4-bit  4-bit

// queue_stop_reasons -> idx = TYPEs num
// tx_queue_count     -> usually for <USER>, [0],[1],[2],[3]

