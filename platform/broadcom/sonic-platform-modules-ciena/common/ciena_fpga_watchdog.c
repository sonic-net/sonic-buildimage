/*
 * File: ciena_fpga_watchdog.c
 *
 * Description: Unified FPGA Watchdog driver for ARM and x86 based
 * platforms having watchdog support in their FPGAs.
 *
 * Copyright (C) 2022 Ciena Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/watchdog.h>

#include "ciena_fpga_watchdog.h"

#define SIRIL_CTL_WDT_MASK		BIT(0)

struct ciena_fpga_watchdog_priv {
	struct watchdog_device		 wdt;
	struct regmap			*regmap;
	unsigned			 wdt_ctl_reg;
	unsigned			 wdt_clr_reg;
};

/* Start the watchdog timer by unmasking it */
static int wd_start(struct watchdog_device *w)
{
	struct ciena_fpga_watchdog_priv *priv = (struct ciena_fpga_watchdog_priv *)watchdog_get_drvdata(w);
	int rc;

	rc = regmap_write_bits(priv->regmap, priv->wdt_ctl_reg, SIRIL_CTL_WDT_MASK, 0);
	if (rc) {
		pr_err("Failed to start the watchdog rc=%d", rc);
		return rc;
	}

	return 0;
}

/* Feed the watchdog */
static int wd_ping(struct watchdog_device *w)
{
	struct ciena_fpga_watchdog_priv *priv = (struct ciena_fpga_watchdog_priv *)watchdog_get_drvdata(w);
	int rc;

	rc = regmap_write_bits(priv->regmap, priv->wdt_clr_reg, SIRIL_CTL_WDT_MASK, 1);
	if (rc) {
		pr_err("Failed to feed the watchdog rc=%d", rc);
		return rc;
	}

	return 0;
}

/* Stop watchdog by masking it */
static int wd_stop(struct watchdog_device *w)
{
	struct ciena_fpga_watchdog_priv *priv = (struct ciena_fpga_watchdog_priv *)watchdog_get_drvdata(w);
	int rc;

	rc = regmap_write_bits(priv->regmap, priv->wdt_ctl_reg, SIRIL_CTL_WDT_MASK, 1);
	if (rc) {
		pr_err("Failed to stop the watchdog rc=%d", rc);
		return rc;
	}

	return 0;
}

/* Set up watchdog device */
static struct watchdog_info wd_info = {
        .identity = "Ciena Fpga Watchdog",
        .options  = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
};

static const struct watchdog_ops wd_ops = {
	.owner = THIS_MODULE,
	.start = wd_start,
	.ping  = wd_ping,
	.stop  = wd_stop,
};

static int ciena_fpga_watchdog_parse_pdata(struct platform_device *pdev,
					   struct ciena_fpga_watchdog_priv *priv)
{
	struct ciena_fpga_watchdog_pdata *pdata = pdev->dev.platform_data;

	if (pdata) {
		priv->regmap        = pdata->regmap;
		priv->wdt_ctl_reg   = pdata->wdt_ctl_reg;
		priv->wdt_clr_reg   = pdata->wdt_clr_reg;

		return 0;
	}

	return -EINVAL;
}

static int ciena_fpga_watchdog_probe(struct platform_device *pdev)
{
	struct ciena_fpga_watchdog_priv *priv;
	int rc;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) return -ENOMEM;

	rc = ciena_fpga_watchdog_parse_pdata(pdev, priv);
	if (rc)
		goto out;

	priv->wdt.info = &wd_info;
	priv->wdt.ops  = &wd_ops;

	rc = watchdog_register_device(&priv->wdt);
	if (rc) {
		dev_err(&pdev->dev, "Error in registering watchdog device");
		goto out;
	} else {
		watchdog_set_drvdata(&priv->wdt, priv);
	}

	dev_set_drvdata(&pdev->dev, priv);

	return 0;
out:
	devm_kfree(&pdev->dev, priv);

	return rc;
}

static void ciena_fpga_watchdog_remove(struct platform_device *pdev)
{
	struct ciena_fpga_watchdog_priv *priv = dev_get_drvdata(&pdev->dev);

	watchdog_unregister_device(&priv->wdt);
	dev_set_drvdata(&pdev->dev, NULL);
}

static const struct platform_device_id ciena_fpga_watchdog_platform_ids[] = {
	{ .name = FPGA_WATCHDOG_DRIVER_NAME, },
	{ /* sentinal */ },
};
MODULE_DEVICE_TABLE(platform, ciena_fpga_watchdog_platform_ids);

static struct platform_driver ciena_fpga_watchdog_driver = {
	.probe    = ciena_fpga_watchdog_probe,
	.remove   = ciena_fpga_watchdog_remove,
	.id_table = ciena_fpga_watchdog_platform_ids,
	.driver   = {
		.name  = FPGA_WATCHDOG_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ciena_fpga_watchdog_init(void)
{
	int rc;

	rc = platform_driver_register(&ciena_fpga_watchdog_driver);
	if (rc) {
                pr_err("%s: platform_driver_register failed, rc=%d\n", __func__, rc);
                return rc;
        }

        return rc;
}

static void __exit ciena_fpga_watchdog_exit(void)
{
	platform_driver_unregister(&ciena_fpga_watchdog_driver);
}

module_init(ciena_fpga_watchdog_init);
module_exit(ciena_fpga_watchdog_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Ciena Corporation");
MODULE_AUTHOR("Naveen Burmi <nburmi@ciena.com>");

