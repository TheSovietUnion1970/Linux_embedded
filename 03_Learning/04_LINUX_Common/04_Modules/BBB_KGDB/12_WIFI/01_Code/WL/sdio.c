// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009-2010 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 */

#include <linux/irq.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include <linux/pm_runtime.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#include "wlcore.h"
#include "wifi_80211.h"
#include "io.h"

#include "common.h"
#include "main.h"

static bool dump = false;

struct wifi_sdio_glue {
	struct device *dev;
	struct platform_device *core;
};

static const struct sdio_device_id wifi_devices[] = {
	{ SDIO_DEVICE(SDIO_VENDOR_ID_TI, SDIO_DEVICE_ID_TI_WL1271) },
	{}
};
MODULE_DEVICE_TABLE(sdio, wifi_devices);

static int wifi_sdio_power_on(struct wifi_sdio_glue *glue)
{
	int ret;
	struct sdio_func *func = dev_to_sdio_func(glue->dev);
	struct mmc_card *card = func->card;
#if (PRINT_DEBUG)
	printk("wifi_sdio_power_on -> 0x%x 0x%x %x\n", glue->dev, func, card);
#endif
	ret = pm_runtime_get_sync(&card->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(&card->dev);
		dev_err(glue->dev, "%s: failed to get_sync(%d)\n",
			__func__, ret);

		return ret;
	}

	sdio_claim_host(func);
	/*
	 * To guarantee that the SDIO card is power cycled, as required to make
	 * the FW programming to succeed, let's do a brute force HW reset.
	 */
	mmc_hw_reset(card->host);

	sdio_enable_func(func);
	sdio_release_host(func);

	return 0;
}

static int wifi_sdio_power_off(struct wifi_sdio_glue *glue)
{
	struct sdio_func *func = dev_to_sdio_func(glue->dev);
	struct mmc_card *card = func->card;

	sdio_claim_host(func);
	sdio_disable_func(func);
	sdio_release_host(func);

	/* Let runtime PM know the card is powered off */
	pm_runtime_put(&card->dev);
	return 0;
}

static int wifi_sdio_set_power(struct device *child, bool enable)
{
	struct wifi_sdio_glue *glue = dev_get_drvdata(child->parent);

	if (enable)
		return wifi_sdio_power_on(glue);
	else
		return wifi_sdio_power_off(glue);
}

static struct wifi_if_operations sdio_ops = {
	// .read		= wifi_sdio_raw_read,
	// .write		= wifi_sdio_raw_write,
	.power		= wifi_sdio_set_power,
	// .set_block_size = wifi_sdio_set_block_size,
};

#ifdef CONFIG_OF

static const struct wilink_family_data wl127x_data = {
	.name = "wl127x",
	.nvs_name = "ti-connectivity/wl127x-nvs.bin",
};

static const struct wilink_family_data wl128x_data = {
	.name = "wl128x",
	.nvs_name = "ti-connectivity/wl128x-nvs.bin",
};

static const struct wilink_family_data wl18xx_data = {
	.name = "wl18xx",
	.cfg_name = "ti-connectivity/wl18xx-conf.bin",
	.nvs_name = "ti-connectivity/wl1271-nvs.bin",
};

static const struct of_device_id wificore_sdio_of_match_table[] = {
	{ .compatible = "ti,wl1271", .data = &wl127x_data },
	{ .compatible = "ti,wl1273", .data = &wl127x_data },
	{ .compatible = "ti,wl1281", .data = &wl128x_data },
	{ .compatible = "ti,wl1283", .data = &wl128x_data },
	{ .compatible = "ti,wl1285", .data = &wl128x_data },
	{ .compatible = "ti,wl1801", .data = &wl18xx_data },
	{ .compatible = "ti,wl1805", .data = &wl18xx_data },
	{ .compatible = "ti,wl1807", .data = &wl18xx_data },
	{ .compatible = "ti,wl1831", .data = &wl18xx_data },
	{ .compatible = "ti,wl1835", .data = &wl18xx_data },
	{ .compatible = "ti,wl1837", .data = &wl18xx_data },
	{ }
};

static int wificore_probe_of(struct device *dev, int *irq, int *wakeirq,
			   struct wificore_platdev_data *pdev_data)
{
	struct device_node *np = dev->of_node;
	const struct of_device_id *of_id;

	of_id = of_match_node(wificore_sdio_of_match_table, np);
	if (!of_id)
		return -ENODEV;

	pdev_data->family = of_id->data;

	*irq = irq_of_parse_and_map(np, 0);
	if (!*irq) {
		dev_err(dev, "No irq in platform data\n");
		return -EINVAL;
	}

	*wakeirq = irq_of_parse_and_map(np, 1);

	/* optional clock frequency params */
	of_property_read_u32(np, "ref-clock-frequency",
			     &pdev_data->ref_clock_freq);
	of_property_read_u32(np, "tcxo-clock-frequency",
			     &pdev_data->tcxo_clock_freq);

	return 0;
}
#else
static int wificore_probe_of(struct device *dev, int *irq, int *wakeirq,
			   struct wificore_platdev_data *pdev_data)
{
	return -ENODATA;
}
#endif

static int wifi_probe(struct sdio_func *func,
				  const struct sdio_device_id *id)
{
	struct wificore_platdev_data *pdev_data;
	struct wifi_sdio_glue *glue;
	struct resource res[2];
	mmc_pm_flag_t mmcflags;
	int ret = -ENOMEM;
	int irq, wakeirq, num_irqs;
	const char *chip_family;
#if (PRINT_DEBUG)
	printk("[MERGE] - wifi_probe\n");
#endif
	/* We are only able to handle the wlan function */
	if (func->num != 0x02)
		return -ENODEV;

	pdev_data = devm_kzalloc(&func->dev, sizeof(*pdev_data), GFP_KERNEL);
	if (!pdev_data)
		return -ENOMEM;

	pdev_data->if_ops = &sdio_ops;

	glue = devm_kzalloc(&func->dev, sizeof(*glue), GFP_KERNEL);
	if (!glue)
		return -ENOMEM;

	glue->dev = &func->dev;

	/* Grab access to FN0 for ELP reg. */
	func->card->quirks |= MMC_QUIRK_LENIENT_FN0;

	/* Use block mode for transferring over one block size of data */
	func->card->quirks |= MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;

	ret = wificore_probe_of(&func->dev, &irq, &wakeirq, pdev_data);
	if (ret)
		goto out;

	/* if sdio can keep power while host is suspended, enable wow */
	mmcflags = sdio_get_host_pm_caps(func);
	dev_dbg(glue->dev, "sdio PM caps = 0x%x\n", mmcflags);

	if (mmcflags & MMC_PM_KEEP_POWER)
		pdev_data->pwr_in_suspend = true;

	sdio_set_drvdata(func, glue);

	/* Tell PM core that we don't need the card to be powered now */
	pm_runtime_put_noidle(&func->dev);
#if (PRINT_DEBUG)
	printk("[MERGE] - glue->dev: 0x%x, parent = 0x%x\n", glue->dev, glue->dev->parent);
#endif
	/*
	 * Due to a hardware bug, we can't differentiate wl18xx from
	 * wl12xx, because both report the same device ID.  The only
	 * way to differentiate is by checking the SDIO revision,
	 * which is 3.00 on the wl18xx chips.
	 */
	if (func->card->cccr.sdio_vsn == SDIO_SDIO_REV_3_00)
		chip_family = "wl18xx";
	else
		chip_family = "wl12xx";

	glue->core = platform_device_alloc(chip_family, PLATFORM_DEVID_AUTO);
	if (!glue->core) {
		dev_err(glue->dev, "can't allocate platform_device");
		ret = -ENOMEM;
		goto out;
	}

	glue->core->dev.parent = &func->dev;

	memset(res, 0x00, sizeof(res));

	res[0].start = irq;
	res[0].flags = IORESOURCE_IRQ |
		       irqd_get_trigger_type(irq_get_irq_data(irq));
	res[0].name = "irq";


	if (wakeirq > 0) {
		res[1].start = wakeirq;
		res[1].flags = IORESOURCE_IRQ |
			       irqd_get_trigger_type(irq_get_irq_data(wakeirq));
		res[1].name = "wakeirq";
		num_irqs = 2;
	} else {
		num_irqs = 1;
	}
	ret = platform_device_add_resources(glue->core, res, num_irqs);
	if (ret) {
		dev_err(glue->dev, "can't add resources\n");
		goto out_dev_put;
	}

	ret = platform_device_add_data(glue->core, pdev_data,
				       sizeof(*pdev_data));
	if (ret) {
		dev_err(glue->dev, "can't add platform data\n");
		goto out_dev_put;
	}

	ret = platform_device_add(glue->core);
	if (ret) {
		dev_err(glue->dev, "can't add platform device\n");
		goto out_dev_put;
	}
	return 0;

out_dev_put:
	platform_device_put(glue->core);

out:
	return ret;
}

static void wifi_remove(struct sdio_func *func)
{
	struct wifi_sdio_glue *glue = sdio_get_drvdata(func);
#if (PRINT_DEBUG)
	printk("[MERGE] - wifi_remove\n");
#endif
	/* Undo decrement done above in wifi_probe */
	pm_runtime_get_noresume(&func->dev);

	platform_device_unregister(glue->core);
}

#ifdef CONFIG_PM
static int wifi_suspend(struct device *dev)
{
	dev_info(dev, "wl1271 suspend\n");
	return 0;	
}

static int wifi_resume(struct device *dev)
{
	dev_info(dev, "wl1271 resume\n");
	return 0;
}

static const struct dev_pm_ops wifi_sdio_pm_ops = {
	.suspend	= wifi_suspend,
	.resume		= wifi_resume,
};
#endif

static struct sdio_driver wifi_sdio_driver = {
	.name		= "wifi_sdio",
	.id_table	= wifi_devices,
	.probe		= wifi_probe,
	.remove		= wifi_remove,
#ifdef CONFIG_PM
	.drv = {
		.pm = &wifi_sdio_pm_ops,
	},
#endif
};

static int __init wifi_init(void)
{
	return sdio_register_driver(&wifi_sdio_driver);
}

static void __exit wifi_exit(void)
{
	sdio_unregister_driver(&wifi_sdio_driver);
}

module_init(wifi_init);
module_exit(wifi_exit);

module_param(dump, bool, 0600);
MODULE_PARM_DESC(dump, "Enable sdio read/write dumps.");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
