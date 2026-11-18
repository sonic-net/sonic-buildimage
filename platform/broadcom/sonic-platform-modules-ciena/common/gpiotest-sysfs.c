/*
 * Copyright 2020 Ciena Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

#include "gpiotest-sysfs.h"

/* ----------------------------------------------------------------------- */
static ssize_t trigger_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf,
			     size_t count)
{
	struct ciena_sysfs_gpiotest_pdata *pdata = dev->platform_data;
	int                                rc;

	if (pdata) {
		dev_dbg(dev,
			"trigger: offset = 0x%x, mask = 0x%x\n",
			pdata->reg,
			pdata->mask);

		rc = regmap_write_bits(pdata->regmap,
				       pdata->reg,
				       pdata->mask,
				       pdata->mask);
		if (rc)
			return rc;
	}
	else
		return -ENODEV;

	return count;
}
static DEVICE_ATTR_WO(trigger);

/* ----------------------------------------------------------------------- */
static int ciena_sysfs_gpiotest_probe(struct platform_device *pdev)
{
	struct ciena_sysfs_gpiotest_pdata *pdata = pdev->dev.platform_data;

	dev_dbg(&pdev->dev, "%s entry\n", GPIOTEST_SYSFS_DRIVER_NAME);
	if (NULL == pdata)
		return -ENODEV;

	if (device_create_file(&pdev->dev, &dev_attr_trigger))
		dev_warn(&pdev->dev, "trigger file creation failed\n");

	/* nudge a uevent from the device, because adding files under
	 * the platform device will not */
	kobject_uevent(&pdev->dev.kobj, KOBJ_ADD);

	dev_dbg(&pdev->dev, "%s exit\n", GPIOTEST_SYSFS_DRIVER_NAME);
	return 0;
}

/* ----------------------------------------------------------------------- */
static void ciena_sysfs_gpiotest_remove(struct platform_device *pdev)
{
	kobject_uevent(&pdev->dev.kobj, KOBJ_REMOVE);

	device_remove_file(&pdev->dev, &dev_attr_trigger);
}

/* ----------------------------------------------------------------------- */
static struct platform_driver ciena_sysfs_gpiotest_driver = {
	.probe	  = ciena_sysfs_gpiotest_probe,
	.remove	  = ciena_sysfs_gpiotest_remove,
	.driver   = {
		.name		= GPIOTEST_SYSFS_DRIVER_NAME,
		.owner		= THIS_MODULE,
	},
};
module_platform_driver(ciena_sysfs_gpiotest_driver);

MODULE_AUTHOR("Ron Belaire");
MODULE_DESCRIPTION("Ciena gpiotest sysfs interface");
MODULE_LICENSE("GPL");
// vim: sw=8 noet
