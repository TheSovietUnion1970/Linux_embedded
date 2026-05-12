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

int wl1271_ps_set_mode(struct VV_vif *VV_vif,
		       enum wl1271_cmd_ps_mode mode)
{
	int ret;
	u16 timeout = wifi_data->conf.conn.dynamic_ps_timeout;

	switch (mode) {
	case STATION_AUTO_PS_MODE:
	case STATION_POWER_SAVE_MODE:
		wl1271_debug(DEBUG_PSM, "entering psm (mode=%d,timeout=%u)",
			     mode, timeout);

		ret = wl1271_acx_wake_up_conditions(VV_vif,
					    wifi_data->conf.conn.wake_up_event,
					    wifi_data->conf.conn.listen_interval);
		if (ret < 0) {
			wl1271_error("couldn't set wake up conditions");
			return ret;
		}

		ret = wl1271_cmd_ps_mode(VV_vif, mode, timeout);
		if (ret < 0)
			return ret;

		set_bit(VV_vif_FLAG_IN_PS, &VV_vif->flags);

		/*
		 * enable beacon early termination.
		 * Not relevant for 5GHz and for high rates.
		 */
		if ((VV_vif->band == NL80211_BAND_2GHZ) &&
		    (VV_vif->basic_rate < CONF_HW_BIT_RATE_9MBPS)) {
			ret = wl1271_acx_bet_enable(VV_vif, true);
			if (ret < 0)
				return ret;
		}
		break;
	case STATION_ACTIVE_MODE:
		wl1271_debug(DEBUG_PSM, "leaving psm");

		/* disable beacon early termination */
		if ((VV_vif->band == NL80211_BAND_2GHZ) &&
		    (VV_vif->basic_rate < CONF_HW_BIT_RATE_9MBPS)) {
			ret = wl1271_acx_bet_enable(VV_vif, false);
			if (ret < 0)
				return ret;
		}

		ret = wl1271_cmd_ps_mode(VV_vif, mode, 0);
		if (ret < 0)
			return ret;

		clear_bit(VV_vif_FLAG_IN_PS, &VV_vif->flags);
		break;
	default:
		wl1271_warning("trying to set ps to unsupported mode %d", mode);
		ret = -EINVAL;
	}

	return ret;
}
