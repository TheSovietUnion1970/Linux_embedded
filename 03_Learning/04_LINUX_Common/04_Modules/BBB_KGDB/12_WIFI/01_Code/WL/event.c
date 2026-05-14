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
#include "wifi_80211.h"
//#include "hw_ops.h"

#include "common.h"
#include "wl18xx.h"
#include "ops.h"
#include "main.h"

#define WL18XX_LOGGER_SDIO_BUFF_MAX	(0x1020)
#define WL18XX_DATA_RAM_BASE_ADDRESS	(0x20000000)
#define WL18XX_LOGGER_SDIO_BUFF_ADDR	(0x40159c)
#define WL18XX_LOGGER_BUFF_OFFSET	(sizeof(struct fw_logger_information))
#define WL18XX_LOGGER_READ_POINT_OFFSET		(12)

int wifi_event_unmask(void)
{
	int ret;

	wifi_debug(DEBUG_EVENT, "unmasking event_mask 0x%x", wifi_data->event_mask);
	ret = wifi_acx_event_mbox_mask(~(wifi_data->event_mask));
	if (ret < 0)
		return ret;

	return 0;
}

// #include "../wl18xx/event.h"

enum wificore_vendor_events {
	WLCORE_VENDOR_EVENT_SC_SYNC,
	WLCORE_VENDOR_EVENT_SC_DECODE,
};

enum wificore_vendor_attributes {
	WLCORE_VENDOR_ATTR_FREQ,
	WLCORE_VENDOR_ATTR_PSK,
	WLCORE_VENDOR_ATTR_SSID,
	WLCORE_VENDOR_ATTR_GROUP_ID,
	WLCORE_VENDOR_ATTR_GROUP_KEY,

	NUM_WLCORE_VENDOR_ATTR,
	MAX_WLCORE_VENDOR_ATTR = NUM_WLCORE_VENDOR_ATTR - 1
};

static void wifi_scan_completed(void)
{
#if (PRINT_DEBUG)
	printk("wifi_scan_completed\n");
#endif
	wifi_scan_failed = false;
	cancel_delayed_work(&wifi_work.scan_complete_work);
	ieee80211_queue_delayed_work(wifi_data->hw, &wifi_work.scan_complete_work,
				     msecs_to_jiffies(0));
}

#include <linux/bitops.h>
static int wifi_process_mailbox_events(void)
{
	struct wl18xx_event_mailbox *mbox = wifi_data->mbox;
	u32 vector;
	int i = 0;

	vector = le32_to_cpu(mbox->events_vector);
#if (PRINT_DEBUG)
	printk("[EVENTS] - MBOX vector: 0x%x, bit: %d", vector, fls(vector) - 1);
#endif
	// 0x100
	if (vector & SCAN_COMPLETE_EVENT_ID) {
		wifi_debug(DEBUG_EVENT, "scan results: %d",
			     mbox->number_of_scan_results);

		for (i = 0; i < wifi_vif_ptr_id; i++){
			if (!wificore_is_p2p_mgmt(wifi_vif_ptr[i])){
#if (PRINT_DEBUG)
				printk("[EVENTS] [%d] - bss = %d", i, wifi_vif_ptr[i]->bss_type);
#endif
				if (wifi_vif_ptr[i]->bss_type == BSS_TYPE_STA_BSS)
					wifi_scan_completed();
			}
		}
	}

	// 0x40000
#if (PRINT_DEBUG)
	else if (vector & REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID)
		printk("REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID\n");

	else 
		printk("OTHER EVENTS, vector = 0x%x\n", vector);
#endif
	return 0;
}

#define WL18XX_INTR_TRIG_EVENT_ACK BIT(29)
static int wifi_ack_event(void)
{
	return wifi_sdio_raw_write(wificore_translate_addr(wifi_data->rtable[REG_INTERRUPT_TRIG]), WL18XX_INTR_TRIG_EVENT_ACK, 4, false);
}

int wifi_event_handle(u8 mbox_num)
{
	int ret;

	wifi_debug(DEBUG_EVENT, "EVENT on mbox %d", mbox_num);

	if (mbox_num > 1)
		return -EINVAL;

	/* first we read the mbox descriptor */
	ret = wifi_sdio_raw_read(wificore_translate_addr(*wifi_data->mbox_ptr[mbox_num]), (u32*)wifi_data->mbox, sizeof(struct wl18xx_event_mailbox), false);
	if (ret < 0)
		return ret;

	/* process the descriptor */
	ret = wifi_process_mailbox_events();
	if (ret < 0)
		return ret;

	/*
	 * TODO: we just need this because one bit is in a different
	 * place.  Is there any better way?
	 */
	ret = wifi_ack_event();

	return ret;
}
