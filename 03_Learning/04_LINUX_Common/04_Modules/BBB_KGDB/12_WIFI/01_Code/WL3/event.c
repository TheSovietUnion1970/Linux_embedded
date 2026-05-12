// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include "wlcore.h"
#include "debug.h"
#include "io.h"
#include "event.h"
#include "ps.h"
#include "scan.h"
#include "wl12xx_80211.h"
//#include "hw_ops.h"

#include "common.h"
#include "wl18xx.h"
#include "ops.h"

#define WL18XX_LOGGER_SDIO_BUFF_MAX	(0x1020)
#define WL18XX_DATA_RAM_BASE_ADDRESS	(0x20000000)
#define WL18XX_LOGGER_SDIO_BUFF_ADDR	(0x40159c)
#define WL18XX_LOGGER_BUFF_OFFSET	(sizeof(struct fw_logger_information))
#define WL18XX_LOGGER_READ_POINT_OFFSET		(12)

int wl1271_event_unmask(void)
{
	int ret;

	wl1271_debug(DEBUG_EVENT, "unmasking event_mask 0x%x", wifi_data->event_mask);
	ret = wl1271_acx_event_mbox_mask(~(wifi_data->event_mask));
	if (ret < 0)
		return ret;

	return 0;
}

// #include "../wl18xx/event.h"

enum wlcore_vendor_events {
	WLCORE_VENDOR_EVENT_SC_SYNC,
	WLCORE_VENDOR_EVENT_SC_DECODE,
};

enum wlcore_vendor_attributes {
	WLCORE_VENDOR_ATTR_FREQ,
	WLCORE_VENDOR_ATTR_PSK,
	WLCORE_VENDOR_ATTR_SSID,
	WLCORE_VENDOR_ATTR_GROUP_ID,
	WLCORE_VENDOR_ATTR_GROUP_KEY,

	NUM_WLCORE_VENDOR_ATTR,
	MAX_WLCORE_VENDOR_ATTR = NUM_WLCORE_VENDOR_ATTR - 1
};

static void VV_scan_completed(void)
{
	//wifi_data->scan.failed = false;
	printk("VV_scan_completed\n");
	VV_scan_failed = false;
	cancel_delayed_work(&VV_work.scan_complete_work);
	ieee80211_queue_delayed_work(wifi_data->hw, &VV_work.scan_complete_work,
				     msecs_to_jiffies(0));
}

#include <linux/bitops.h>
static int VV_process_mailbox_events(void)
{
	struct wl18xx_event_mailbox *mbox = wifi_data->mbox;
	u32 vector;
	int i = 0;

	vector = le32_to_cpu(mbox->events_vector);
	printk("[EVENTS] - MBOX vector: 0x%x, bit: %d", vector, fls(vector) - 1);

	// 0x100
	if (vector & SCAN_COMPLETE_EVENT_ID) {
		wl1271_debug(DEBUG_EVENT, "scan results: %d",
			     mbox->number_of_scan_results);

		for (i = 0; i < VV_vif_ptr_id; i++){
			if (!wlcore_is_p2p_mgmt(VV_vif_ptr[i])){
				printk("[EVENTS] [%d] - bss = %d", i, VV_vif_ptr[i]->bss_type);
				if (VV_vif_ptr[i]->bss_type == BSS_TYPE_STA_BSS)
					VV_scan_completed();
			}
		}
	}

	// 0x40000
	else if (vector & REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID)
		printk("REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID\n");

	else printk("OTHER EVENTS, vector = 0x%x\n", vector);
	return 0;
}

#define WL18XX_INTR_TRIG_EVENT_ACK BIT(29)
static int VV_ack_event(void)
{
	return VV_sdio_raw_write(wlcore_translate_addr(wifi_data->rtable[REG_INTERRUPT_TRIG]), WL18XX_INTR_TRIG_EVENT_ACK, 4, false);
}

int wl1271_event_handle(u8 mbox_num)
{
	int ret;

	wl1271_debug(DEBUG_EVENT, "EVENT on mbox %d", mbox_num);

	if (mbox_num > 1)
		return -EINVAL;

	/* first we read the mbox descriptor */
	ret = VV_sdio_raw_read(wlcore_translate_addr(*wifi_data->mbox_ptr[mbox_num]), (u32*)wifi_data->mbox, sizeof(struct wl18xx_event_mailbox), false);
	if (ret < 0)
		return ret;

	/* process the descriptor */
	ret = VV_process_mailbox_events();
	if (ret < 0)
		return ret;

	/*
	 * TODO: we just need this because one bit is in a different
	 * place.  Is there any better way?
	 */
	ret = VV_ack_event();

	return ret;
}
