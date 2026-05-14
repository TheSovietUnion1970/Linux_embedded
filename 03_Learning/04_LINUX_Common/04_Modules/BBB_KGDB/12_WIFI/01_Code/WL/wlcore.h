/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wlcore
 *
 * Copyright (C) 2011 Texas Instruments Inc.
 */

#ifndef __WLCORE_H__
#define __WLCORE_H__

#include <linux/platform_device.h>

#include "wificore_i.h"
#include "event.h"
#include "boot.h"
// extern struct Wifi_data wifi_data;

/* The maximum number of Tx descriptors in all chip families */
#define WLCORE_MAX_TX_DESCRIPTORS 32

/*
 * We always allocate this number of mac addresses. If we don't
 * have enough allocated addresses, the LAA bit is used
 */
#define WLCORE_NUM_MAC_ADDRESSES 3

/* wl12xx/wl18xx maximum transmission power (in dBm) */
#define WLCORE_MAX_TXPWR        25

/* Texas Instruments pre assigned OUI */
#define WLCORE_TI_OUI_ADDRESS 0x080028

/* forward declaration */
struct wifi_tx_hw_descr;
enum wl_rx_buf_align;
struct wifi_rx_descriptor;

struct wificore_ops {
	int (*setup)(void);
	int (*boot)(void);
	int (*hw_init)(void);
};

enum wificore_partitions {
	PART_DOWN,
	PART_WORK,
	PART_BOOT,
	PART_DRPW,
	PART_TOP_PRCM_ELP_SOC,
	PART_PHY_INIT,

	PART_TABLE_LEN,
};

struct wificore_partition {
	u32 size;
	u32 start;
};

struct wificore_partition_set {
	struct wificore_partition mem;
	struct wificore_partition reg;
	struct wificore_partition mem2;
	struct wificore_partition mem3;
};

enum wificore_registers {
	/* register addresses, used with partition translation */
	REG_ECPU_CONTROL,
	REG_INTERRUPT_NO_CLEAR,
	REG_INTERRUPT_ACK,
	REG_COMMAND_MAILBOX_PTR,
	REG_EVENT_MAILBOX_PTR,
	REG_INTERRUPT_TRIG,
	REG_INTERRUPT_MASK,
	REG_PC_ON_RECOVERY,
	REG_CHIP_ID_B,
	REG_CMD_MBOX_ADDRESS,

	/* data access memory addresses, used with partition translation */
	REG_SLV_MEM_DATA,
	REG_SLV_REG_DATA,

	/* raw data access memory addresses */
	REG_RAW_FW_STATUS_ADDR,

	REG_TABLE_LEN,
};

int wificore_probe(struct platform_device *pdev);
int wificore_remove(struct platform_device *pdev);
struct ieee80211_hw *wificore_alloc_hw(size_t priv_size, u32 aggr_buf_size,
				     u32 mbox_size);
int wificore_free_hw(void);
int wificore_set_key(enum set_key_cmd cmd,
		   struct ieee80211_vif *vif,
		   struct ieee80211_sta *sta,
		   struct ieee80211_key_conf *key_conf);
void wificore_regdomain_config(void);

/* Tell wlcore not to care about this element when checking the version */
#define WLCORE_FW_VER_IGNORE	-1


/* Firmware image load chunk size */
#define CHUNK_SIZE	16384

/* Quirks */

/* Each RX/TX transaction requires an end-of-transaction transfer */
#define WLCORE_QUIRK_END_OF_TRANSACTION		BIT(0)

/* wl127x and SPI don't support SDIO block size alignment */
#define WLCORE_QUIRK_TX_BLOCKSIZE_ALIGN		BIT(2)

/* means aggregated Rx packets are aligned to a SDIO block */
#define WLCORE_QUIRK_RX_BLOCKSIZE_ALIGN		BIT(3)

/* Older firmwares did not implement the FW logger over bus feature */
#define WLCORE_QUIRK_FWLOG_NOT_IMPLEMENTED	BIT(4)

/* Older firmwares use an old NVS format */
#define WLCORE_QUIRK_LEGACY_NVS			BIT(5)

/* pad only the last frame in the aggregate buffer */
#define WLCORE_QUIRK_TX_PAD_LAST_FRAME		BIT(7)

/* extra header space is required for TKIP */
#define WLCORE_QUIRK_TKIP_HEADER_SPACE		BIT(8)

/* Some firmwares not support sched scans while connected */
#define WLCORE_QUIRK_NO_SCHED_SCAN_WHILE_CONN	BIT(9)

/* separate probe response templates for one-shot and sched scans */
#define WLCORE_QUIRK_DUAL_PROBE_TMPL		BIT(10)

/* Firmware requires reg domain configuration for active calibration */
#define WLCORE_QUIRK_REGDOMAIN_CONF		BIT(11)

/* The FW only support a zero session id for AP */
#define WLCORE_QUIRK_AP_ZERO_SESSION_ID		BIT(12)

/* TODO: move all these common registers and values elsewhere */
#define HW_ACCESS_ELP_CTRL_REG		0x1FFFC

/* ELP register commands */
#define ELPCTRL_WAKE_UP             0x1
#define ELPCTRL_WAKE_UP_WLAN_READY  0x5
#define ELPCTRL_SLEEP               0x0
/* ELP WLAN_READY bit */
#define ELPCTRL_WLAN_READY          0x2

/*************************************************************************

    Interrupt Trigger Register (Host -> WiLink)

**************************************************************************/

/* Hardware to Embedded CPU Interrupts - first 32-bit register set */

/*
 * The host sets this bit to inform the Wlan
 * FW that a TX packet is in the XFER
 * Buffer #0.
 */
#define INTR_TRIG_TX_PROC0 BIT(2)

/*
 * The host sets this bit to inform the FW
 * that it read a packet from RX XFER
 * Buffer #0.
 */
#define INTR_TRIG_RX_PROC0 BIT(3)

#define INTR_TRIG_DEBUG_ACK BIT(4)

#define INTR_TRIG_STATE_CHANGED BIT(5)

/* Hardware to Embedded CPU Interrupts - second 32-bit register set */

/*
 * The host sets this bit to inform the FW
 * that it read a packet from RX XFER
 * Buffer #1.
 */
#define INTR_TRIG_RX_PROC1 BIT(17)

/*
 * The host sets this bit to inform the Wlan
 * hardware that a TX packet is in the XFER
 * Buffer #1.
 */
#define INTR_TRIG_TX_PROC1 BIT(18)

#define ACX_SLV_SOFT_RESET_BIT	BIT(1)
#define SOFT_RESET_MAX_TIME	1000000
#define SOFT_RESET_STALL_TIME	1000

#define ECPU_CONTROL_HALT	0x00000101

#define WELP_ARM_COMMAND_VAL	0x4

#endif /* __WLCORE_H__ */
