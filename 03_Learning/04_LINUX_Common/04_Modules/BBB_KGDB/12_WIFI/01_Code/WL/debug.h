/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wl12xx
 *
 * Copyright (C) 2011 Texas Instruments. All rights reserved.
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <coelho@ti.com>
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <linux/bitops.h>
#include <linux/printk.h>

#define DRIVER_NAME "wifi0"
#define DEBUG_NAME "wifi0_debug"
#define WARN_NAME "wifi0_warning"
#define DRIVER_PREFIX DRIVER_NAME ": "
#define DRIVER_DEBUG DEBUG_NAME ": "
#define DRIVER_WARN WARN_NAME ": "

enum {
	DEBUG_NONE	= 0,
	DEBUG_IRQ	= BIT(0),
	DEBUG_SPI	= BIT(1),
	DEBUG_BOOT	= BIT(2),
	DEBUG_MAILBOX	= BIT(3),
	DEBUG_TESTMODE	= BIT(4),
	DEBUG_EVENT	= BIT(5),
	DEBUG_TX	= BIT(6),
	DEBUG_RX	= BIT(7),
	DEBUG_SCAN	= BIT(8),
	DEBUG_CRYPT	= BIT(9),
	DEBUG_PSM	= BIT(10),
	DEBUG_MAC80211	= BIT(11),
	DEBUG_CMD	= BIT(12),
	DEBUG_ACX	= BIT(13),
	DEBUG_SDIO	= BIT(14),
	DEBUG_FILTERS   = BIT(15),
	DEBUG_ADHOC     = BIT(16),
	DEBUG_AP	= BIT(17),
	DEBUG_PROBE	= BIT(18),
	DEBUG_IO	= BIT(19),
	DEBUG_MASTER	= (DEBUG_ADHOC | DEBUG_AP),
	DEBUG_ALL	= ~0,
};

extern u32 wifi_debug_level;

#define DEBUG_DUMP_LIMIT 1024

#define wifi_info(fmt, arg...) \
	pr_info(DRIVER_PREFIX fmt "\n", ##arg)

#define wifi_error(fmt, arg...) \
	pr_err(DRIVER_PREFIX "ERROR " fmt "\n", ##arg)

#define wifi_warning(fmt, arg...) \
	pr_warn(DRIVER_PREFIX "WARNING " fmt "\n", ##arg)


/* define the debug macro differently if dynamic debug is supported */
#if defined(CONFIG_DYNAMIC_DEBUG)
#define wifi_debug(level, fmt, arg...) \
	do { \
		if (unlikely(level & wifi_debug_level)) \
			dynamic_pr_debug(DRIVER_PREFIX fmt "\n", ##arg); \
	} while (0)
#else
#define wifi_debug(level, fmt, arg...) \
	do { \
		if (unlikely(level & wifi_debug_level)) \
			printk(KERN_DEBUG pr_fmt(DRIVER_PREFIX fmt "\n"), \
			       ##arg); \
	} while (0)
#endif

#define wifi_dump(level, prefix, buf, len)				      \
	do {								      \
		if (level & wifi_debug_level)				      \
			print_hex_dump_debug(DRIVER_PREFIX prefix,	      \
					DUMP_PREFIX_OFFSET, 16, 1,	      \
					buf,				      \
					min_t(size_t, len, DEBUG_DUMP_LIMIT), \
					0);				      \
	} while (0)

#endif /* __DEBUG_H__ */
