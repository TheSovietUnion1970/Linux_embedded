/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#ifndef __PS_H__
#define __PS_H__

#include "wlcore.h"
#include "acx.h"

int wifi_ps_set_mode(struct wifi_vif *wifi_vif,
		       enum wifi_cmd_ps_mode mode);

#define WL1271_PS_COMPLETE_TIMEOUT 500

#endif /* __WL1271_PS_H__ */
