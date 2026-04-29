// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl18xx
 *
 * Copyright (C) 2011 Texas Instruments Inc.
 */

#include "../wlcore/cmd.h"
#include "../wlcore/debug.h"
#include "../wlcore/hw_ops.h"

#include "cmd.h"

int wl18xx_cmd_smart_config_start(struct wl1271 *wl, u32 group_bitmap)
{
	struct wl18xx_cmd_smart_config_start *cmd;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd smart config start group_bitmap=0x%x",
		     group_bitmap);

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->group_id_bitmask = cpu_to_le32(group_bitmap);

	ret = wl1271_cmd_send(wl, CMD_SMART_CONFIG_START, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("failed to send smart config start command");
		goto out_free;
	}

out_free:
	kfree(cmd);
out:
	return ret;
}

int wl18xx_cmd_smart_config_stop(struct wl1271 *wl)
{
	struct wl1271_cmd_header *cmd;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd smart config stop");

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	ret = wl1271_cmd_send(wl, CMD_SMART_CONFIG_STOP, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("failed to send smart config stop command");
		goto out_free;
	}

out_free:
	kfree(cmd);
out:
	return ret;
}

int wl18xx_cmd_smart_config_set_group_key(struct wl1271 *wl, u16 group_id,
					  u8 key_len, u8 *key)
{
	struct wl18xx_cmd_smart_config_set_group_key *cmd;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd smart config set group key id=0x%x",
		     group_id);

	if (key_len != sizeof(cmd->key)) {
		wl1271_error("invalid group key size: %d", key_len);
		return -E2BIG;
	}

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->group_id = cpu_to_le32(group_id);
	memcpy(cmd->key, key, key_len);

	ret = wl1271_cmd_send(wl, CMD_SMART_CONFIG_SET_GROUP_KEY, cmd,
			      sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("failed to send smart config set group key cmd");
		goto out_free;
	}

out_free:
	kfree(cmd);
out:
	return ret;
}

int wl18xx_cmd_radar_detection_debug(struct wl1271 *wl, u8 channel)
{
	struct wl18xx_cmd_dfs_radar_debug *cmd;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd radar detection debug (chan %d)",
		     channel);

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd->channel = channel;

	ret = wl1271_cmd_send(wl, CMD_DFS_RADAR_DETECTION_DEBUG,
			      cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("failed to send radar detection debug command");
		goto out_free;
	}

out_free:
	kfree(cmd);
	return ret;
}

int wl18xx_cmd_dfs_master_restart(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	struct wl18xx_cmd_dfs_master_restart *cmd;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd dfs master restart (role %d)",
		     wlvif->role_id);

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd->role_id = wlvif->role_id;

	ret = wl1271_cmd_send(wl, CMD_DFS_MASTER_RESTART,
			      cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("failed to send dfs master restart command");
		goto out_free;
	}
out_free:
	kfree(cmd);
	return ret;
}
