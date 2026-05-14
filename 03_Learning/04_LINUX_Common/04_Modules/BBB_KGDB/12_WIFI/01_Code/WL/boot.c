// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2010 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include <linux/slab.h>
#include <linux/export.h>

#include "debug.h"
#include "acx.h"
#include "boot.h"
#include "io.h"
#include "event.h"
#include "rx.h"

#include "common.h"
#include "main.h"
#include "wl18.h"

static int wifi_boot_set_ecpu_ctrl(u32 flag)
{
	u32 cpu_ctrl;
	int ret;

	/* 10.5.0 run the firmware (I) */
	ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_ECPU_CONTROL]), &cpu_ctrl, 4, false);
	if (ret < 0)
		goto out;

	/* 10.5.1 run the firmware (II) */
	cpu_ctrl |= flag;
	ret = wifi_sdio_raw_write(wificore_translate_addr(wifi_data->rtable[REG_ECPU_CONTROL]), cpu_ctrl, 4, false);

out:
	return ret;
}

static int wificore_boot_parse_fw_ver(
				    struct wifi_static_data *static_data)
{
	int ret;

	strncpy(wifi_chip->fw_ver_str, static_data->fw_version,
		sizeof(wifi_chip->fw_ver_str));

	/* make sure the string is NULL-terminated */
	wifi_chip->fw_ver_str[sizeof(wifi_chip->fw_ver_str) - 1] = '\0';

	ret = sscanf(wifi_chip->fw_ver_str + 4, "%u.%u.%u.%u.%u",
		     &wifi_chip->fw_ver[0], &wifi_chip->fw_ver[1],
		     &wifi_chip->fw_ver[2], &wifi_chip->fw_ver[3],
		     &wifi_chip->fw_ver[4]);

	if (ret != 5) {
		wifi_warning("fw version incorrect value");
		memset(wifi_chip->fw_ver, 0, sizeof(wifi_chip->fw_ver));
		ret = -EINVAL;
		goto out;
	}

out:
	return ret;
}

static int wificore_validate_fw_ver(void)
{
	unsigned int *fw_ver = wifi_chip->fw_ver;
	unsigned int *min_ver = (wifi_data->fw_type == WL12XX_FW_TYPE_MULTI) ?
		wifi_data->min_mr_fw_ver : wifi_data->min_sr_fw_ver;
	char min_fw_str[32] = "";
	int off = 0;
	int i;

	/* the chip must be exactly equal */
	if ((min_ver[FW_VER_CHIP] != WLCORE_FW_VER_IGNORE) &&
	    (min_ver[FW_VER_CHIP] != fw_ver[FW_VER_CHIP]))
		goto fail;

	/* the firmware type must be equal */
	if ((min_ver[FW_VER_IF_TYPE] != WLCORE_FW_VER_IGNORE) &&
	    (min_ver[FW_VER_IF_TYPE] != fw_ver[FW_VER_IF_TYPE]))
		goto fail;

	/* the project number must be equal */
	if ((min_ver[FW_VER_SUBTYPE] != WLCORE_FW_VER_IGNORE) &&
	    (min_ver[FW_VER_SUBTYPE] != fw_ver[FW_VER_SUBTYPE]))
		goto fail;

	/* the API version must be greater or equal */
	if ((min_ver[FW_VER_MAJOR] != WLCORE_FW_VER_IGNORE) &&
		 (min_ver[FW_VER_MAJOR] > fw_ver[FW_VER_MAJOR]))
		goto fail;

	/* if the API version is equal... */
	if (((min_ver[FW_VER_MAJOR] == WLCORE_FW_VER_IGNORE) ||
	     (min_ver[FW_VER_MAJOR] == fw_ver[FW_VER_MAJOR])) &&
	    /* ...the minor must be greater or equal */
	    ((min_ver[FW_VER_MINOR] != WLCORE_FW_VER_IGNORE) &&
	     (min_ver[FW_VER_MINOR] > fw_ver[FW_VER_MINOR])))
		goto fail;

	return 0;

fail:
	for (i = 0; i < NUM_FW_VER && off < sizeof(min_fw_str); i++)
		if (min_ver[i] == WLCORE_FW_VER_IGNORE)
			off += snprintf(min_fw_str + off,
					sizeof(min_fw_str) - off,
					"*.");
		else
			off += snprintf(min_fw_str + off,
					sizeof(min_fw_str) - off,
					"%u.", min_ver[i]);

	wifi_error("Your WiFi FW version (%u.%u.%u.%u.%u) is invalid.\n"
		     "Please use at least FW %s\n"
		     "You can get the latest firmwares at:\n"
		     "git://git.ti.com/wilink8-wlan/wl18xx_fw.git",
		     fw_ver[FW_VER_CHIP], fw_ver[FW_VER_IF_TYPE],
		     fw_ver[FW_VER_MAJOR], fw_ver[FW_VER_SUBTYPE],
		     fw_ver[FW_VER_MINOR], min_fw_str);
	return -EINVAL;
}

static int wifi_boot_static_data(void)
{
	struct wifi_static_data *static_data;
	size_t len = sizeof(*static_data) + wifi_data->static_data_priv_len;
	int ret;

	static_data = kmalloc(len, GFP_KERNEL);
	if (!static_data) {
		ret = -ENOMEM;
		goto out;
	}

	ret = wifi_sdio_raw_read(wificore_translate_addr(*wifi_data->cmd_box_addr), (u32*)static_data, len, false);
	if (ret < 0)
		goto out_free;

	ret = wificore_boot_parse_fw_ver(static_data);
	if (ret < 0)
		goto out_free;

	ret = wificore_validate_fw_ver();
	if (ret < 0)
		goto out_free;

	ret = wifi_handle_static_data(static_data);
	if (ret < 0)
		goto out_free;

out_free:
	kfree(static_data);
out:
	return ret;
}

static int wifi_boot_upload_firmware_chunk(void *buf,
					     size_t fw_data_len, u32 dest)
{
	// struct wificore_partition_set partition;
	struct wifi_partition_set partition;
	int addr, chunk_num, partition_limit;
	u8 *p, *chunk;
	int ret;

	/* whal_FwCtrl_LoadFwImageSm() */

	wifi_debug(DEBUG_BOOT, "starting firmware upload");

	wifi_debug(DEBUG_BOOT, "fw_data_len %zd chunk_size %d",
		     fw_data_len, CHUNK_SIZE);

	if ((fw_data_len % 4) != 0) {
		wifi_error("firmware length not multiple of four");
		return -EIO;
	}

	chunk = kmalloc(CHUNK_SIZE, GFP_KERNEL);
	if (!chunk) {
		wifi_error("allocation for firmware upload chunk failed");
		return -ENOMEM;
	}

	// memcpy(&partition, &wifi_data->ptable[PART_DOWN], sizeof(partition));
	memcpy(&partition, &wifi_data->ptable[PART_DOWN], sizeof(partition));
	partition.mem.start = dest;
	ret = wifi_set_partition_core(&partition);
	if (ret < 0)
		goto out;

	/* 10.1 set partition limit and chunk num */
	chunk_num = 0;
	partition_limit = wifi_data->ptable[PART_DOWN].mem.size;

	while (chunk_num < fw_data_len / CHUNK_SIZE) {
		/* 10.2 update partition, if needed */
		addr = dest + (chunk_num + 2) * CHUNK_SIZE;
		if (addr > partition_limit) {
			addr = dest + chunk_num * CHUNK_SIZE;
			partition_limit = chunk_num * CHUNK_SIZE +
				wifi_data->ptable[PART_DOWN].mem.size;
			partition.mem.start = addr;
			ret = wifi_set_partition_core(&partition);
			if (ret < 0)
				goto out;
		}

		/* 10.3 upload the chunk */
		addr = dest + chunk_num * CHUNK_SIZE;
		p = buf + chunk_num * CHUNK_SIZE;
		memcpy(chunk, p, CHUNK_SIZE);
		wifi_debug(DEBUG_BOOT, "uploading fw chunk 0x%p to 0x%x",
			     p, addr);
		ret = wifi_sdio_raw_write1(wificore_translate_addr(addr), chunk, CHUNK_SIZE, false);
		if (ret < 0)
			goto out;

		chunk_num++;
	}

	/* 10.4 upload the last chunk */
	addr = dest + chunk_num * CHUNK_SIZE;
	p = buf + chunk_num * CHUNK_SIZE;
	memcpy(chunk, p, fw_data_len % CHUNK_SIZE);
	wifi_debug(DEBUG_BOOT, "uploading fw last chunk (%zd B) 0x%p to 0x%x",
		     fw_data_len % CHUNK_SIZE, p, addr);
	ret = wifi_sdio_raw_write1(wificore_translate_addr(addr), chunk, fw_data_len % CHUNK_SIZE, false);

out:
	kfree(chunk);
	return ret;
}

int wificore_boot_upload_firmware(void)
{
	u32 chunks, addr, len;
	int ret = 0;
	u8 *fw;

	fw = wifi_data->fw;
	chunks = be32_to_cpup((__be32 *) fw);
	fw += sizeof(u32);

	wifi_debug(DEBUG_BOOT, "firmware chunks to be uploaded: %u", chunks);

	while (chunks--) {
		addr = be32_to_cpup((__be32 *) fw);
		fw += sizeof(u32);
		len = be32_to_cpup((__be32 *) fw);
		fw += sizeof(u32);

		if (len > 300000) {
			printk("firmware chunk too long: %u", len);
			return -EINVAL;
		}
		wifi_debug(DEBUG_BOOT, "chunk %d addr 0x%x len %u",
			     chunks, addr, len);
		ret = wifi_boot_upload_firmware_chunk(fw, len, addr);
		if (ret != 0)
			break;
		fw += len;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(wificore_boot_upload_firmware);

#include "wl18xx.h"
int wificore_boot_run_firmware(void)
{
	int loop, ret;
	u32 chip_id, intr;

	/* Make sure we have the boot partition */
	ret = wifi_set_partition_core(&wifi_data->ptable[PART_BOOT]);
	if (ret < 0)
		return ret;

	ret = wifi_boot_set_ecpu_ctrl(ECPU_CONTROL_HALT);
	if (ret < 0)
		return ret;

	ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_CHIP_ID_B]), &chip_id, 4, false);
	if (ret < 0)
		return ret;

	wifi_debug(DEBUG_BOOT, "chip id after firmware boot: 0x%x", chip_id);

	if (chip_id != wifi_chip->id) {
		wifi_error("chip id doesn't match after firmware boot");
		return -EIO;
	}

	/* wait for init to complete */
	loop = 0;
	while (loop++ < INIT_LOOP) {
		udelay(INIT_LOOP_DELAY);
		ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_INTERRUPT_NO_CLEAR]), &intr, 4, false);
		if (ret < 0)
			return ret;

		if (intr == 0xffffffff) {
			wifi_error("error reading hardware complete "
				     "init indication");
			return -EIO;
		}
		/* check that ACX_INTR_INIT_COMPLETE is enabled */
		else if (intr & WL1271_ACX_INTR_INIT_COMPLETE) {
			ret = wifi_sdio_raw_write(wificore_translate_addr(wifi_data->rtable[REG_INTERRUPT_ACK]), WL1271_ACX_INTR_INIT_COMPLETE, 4, false);
			if (ret < 0)
				return ret;
			break;
		}
	}

	if (loop > INIT_LOOP) {
		wifi_error("timeout waiting for the hardware to "
			     "complete initialization");
		return -EIO;
	}

	/* get hardware config command mail box */
	ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_COMMAND_MAILBOX_PTR]), wifi_data->cmd_box_addr, 4, false);
	if (ret < 0)
		return ret;
#if (PRINT_DEBUG)
	printk("cmd_box_addr 0x%x", wifi_data->cmd_box_addr);
#endif
	ret = wifi_sdio_raw_read(wificore_translate_addr(wifi_data->rtable[REG_EVENT_MAILBOX_PTR]), (u32*)wifi_data->mbox_ptr[0], 4, false);
	if (ret < 0)
		return ret;
	*(wifi_data->mbox_ptr[1]) = *(wifi_data->mbox_ptr[0]) + sizeof(struct wl18xx_event_mailbox);

	ret = wifi_boot_static_data();
	if (ret < 0) {
		wifi_error("error getting static data");
		return ret;
	}

	/*
	 * in case of full asynchronous mode the firmware event must be
	 * ready to receive event from the command mailbox
	 */

	/* unmask required mbox events  */
	ret = wifi_event_unmask();
	if (ret < 0) {
		wifi_error("EVENT mask setting failed");
		return ret;
	}

	/* set the working partition to its "running" mode offset */
	ret = wifi_set_partition_core(&wifi_data->ptable[PART_WORK]);

	/* firmware startup completed */
	return ret;
}
EXPORT_SYMBOL_GPL(wificore_boot_run_firmware);
