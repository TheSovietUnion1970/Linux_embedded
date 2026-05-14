// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include "ps.h"
#include "io.h"
#include "tx.h"
#include "debug.h"

int wifi_ps_set_mode(struct wifi_vif *wifi_vif,
		       enum wifi_cmd_ps_mode mode)
{
	int ret;
	u16 timeout = wifi_data->conf.conn.dynamic_ps_timeout;

	switch (mode) {
	case STATION_AUTO_PS_MODE:
	case STATION_POWER_SAVE_MODE:
		wifi_debug(DEBUG_PSM, "entering psm (mode=%d,timeout=%u)",
			     mode, timeout);

		ret = wifi_acx_wake_up_conditions(wifi_vif,
					    wifi_data->conf.conn.wake_up_event,
					    wifi_data->conf.conn.listen_interval);
		if (ret < 0) {
			wifi_error("couldn't set wake up conditions");
			return ret;
		}

		ret = wifi_cmd_ps_mode(wifi_vif, mode, timeout);
		if (ret < 0)
			return ret;

		set_bit(wifi_vif_FLAG_IN_PS, &wifi_vif->flags);

		/*
		 * enable beacon early termination.
		 * Not relevant for 5GHz and for high rates.
		 */
		if ((wifi_vif->band == NL80211_BAND_2GHZ) &&
		    (wifi_vif->basic_rate < CONF_HW_BIT_RATE_9MBPS)) {
			ret = wifi_acx_bet_enable(wifi_vif, true);
			if (ret < 0)
				return ret;
		}
		break;
	case STATION_ACTIVE_MODE:
		wifi_debug(DEBUG_PSM, "leaving psm");

		/* disable beacon early termination */
		if ((wifi_vif->band == NL80211_BAND_2GHZ) &&
		    (wifi_vif->basic_rate < CONF_HW_BIT_RATE_9MBPS)) {
			ret = wifi_acx_bet_enable(wifi_vif, false);
			if (ret < 0)
				return ret;
		}

		ret = wifi_cmd_ps_mode(wifi_vif, mode, 0);
		if (ret < 0)
			return ret;

		clear_bit(wifi_vif_FLAG_IN_PS, &wifi_vif->flags);
		break;
	default:
		wifi_warning("trying to set ps to unsupported mode %d", mode);
		ret = -EINVAL;
	}

	return ret;
}
