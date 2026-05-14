/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wl1271
 *
 * Copyright (C) 1998-2009 Texas Instruments. All rights reserved.
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#ifndef __EVENT_H__
#define __EVENT_H__

/*
 * Mbox events
 *
 * The event mechanism is based on a pair of event buffers (buffers A and
 * B) at fixed locations in the target's memory. The host processes one
 * buffer while the other buffer continues to collect events. If the host
 * is not processing events, an interrupt is issued to signal that a buffer
 * is ready. Once the host is done with processing events from one buffer,
 * it signals the target (with an ACK interrupt) that the event buffer is
 * free.
 */

enum {
	RSSI_SNR_TRIGGER_0_EVENT_ID              = BIT(0),
	RSSI_SNR_TRIGGER_1_EVENT_ID              = BIT(1),
	RSSI_SNR_TRIGGER_2_EVENT_ID              = BIT(2),
	RSSI_SNR_TRIGGER_3_EVENT_ID              = BIT(3),
	RSSI_SNR_TRIGGER_4_EVENT_ID              = BIT(4),
	RSSI_SNR_TRIGGER_5_EVENT_ID              = BIT(5),
	RSSI_SNR_TRIGGER_6_EVENT_ID              = BIT(6),
	RSSI_SNR_TRIGGER_7_EVENT_ID              = BIT(7),

	EVENT_MBOX_ALL_EVENT_ID			 = 0x7fffffff,
};

/* events the driver might want to wait for */
enum wificore_wait_event {
	WLCORE_EVENT_ROLE_STOP_COMPLETE,
	WLCORE_EVENT_PEER_REMOVE_COMPLETE,
	WLCORE_EVENT_DFS_CONFIG_COMPLETE
};

enum {
	EVENT_ENTER_POWER_SAVE_FAIL = 0,
	EVENT_ENTER_POWER_SAVE_SUCCESS,
};

#define NUM_OF_RSSI_SNR_TRIGGERS 8

struct fw_logger_information {
	__le32 max_buff_size;
	__le32 actual_buff_size;
	__le32 num_trace_drop;
	__le32 buff_read_ptr;
	__le32 buff_write_ptr;
} __packed;

struct wl1271;

int wifi_event_unmask(void);
int wifi_event_handle(u8 mbox);

#endif
