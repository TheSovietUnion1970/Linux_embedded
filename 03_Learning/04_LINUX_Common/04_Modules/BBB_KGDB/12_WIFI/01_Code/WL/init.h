/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#ifndef __INIT_H__
#define __INIT_H__

#include "wlcore.h"

int wifi_hw_init(void);
int wifi_init_vif_specific(struct ieee80211_vif *vif);
int wifi_sta_hw_init(struct wifi_vif *wifi_vif);

#endif
