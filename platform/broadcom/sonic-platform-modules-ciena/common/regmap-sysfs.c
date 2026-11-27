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

#include "regmap-sysfs.h"

struct regmap_sysfs_priv {
	struct bin_attribute read_attr;
};

static ssize_t offset_and_or_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct ciena_sysfs_regmap_pdata *pdata    = dev->platform_data;
	unsigned int                     offset;
	unsigned int                     mask_and;
	unsigned int                     mask_or;
	int                              rc;

	if (pdata) {
		rc = sscanf(buf, "%x,%x,%x", &offset, &mask_and, &mask_or);
		if (rc != 3)
			return -EINVAL;

		dev_dbg(dev,
			"offset = 0x%x, and = 0x%x, or = 0x%x for %s\n",
			offset, mask_and, mask_or, pdata->name);

		rc = regmap_write_bits(pdata->regmap,
				       offset, mask_and, mask_or);
		if (rc)
			return rc;
	}
	else
		return -ENODEV;

	return count;
}
static DEVICE_ATTR_WO(offset_and_or);

static ssize_t ciena_sysfs_regmap_read(struct file *file, struct kobject *kobj,
				       struct bin_attribute *bin_attr, char *to,
				       loff_t pos, size_t count)
{
	struct device                   *dev    = kobj_to_dev(kobj);
	struct ciena_sysfs_regmap_pdata *pdata  = dev->platform_data;
	unsigned int                     offset = pos;
	unsigned int                     val;
	void                            *ret = &val;
	u16                              r16;
	u8                               r8;
	int                              rc;

	if (pdata) {
		if (count > sizeof(val)) count = sizeof(val);

		if (count != regmap_get_reg_stride(pdata->regmap))
			return -EINVAL;

		rc = regmap_read(pdata->regmap, offset, &val);
		if (rc) return rc;
	}
	else return -ENODEV;

	switch (count) {
	case (sizeof(r8)):
		r8  = val;
		ret = &r8;
		break;
	case (sizeof(r16)):
		r16 = val;
		ret = &r16;
		break;
	case (sizeof(val)):
		break;
	default:
		dev_err(dev, "unsupported register size: %zu\n", count);
		return -EINVAL;
	}

	memcpy(to, ret, count);

	return count;
}

static ssize_t reg_size_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct ciena_sysfs_regmap_pdata *pdata = dev->platform_data;

	return sprintf(buf, "%d\n", regmap_get_reg_stride(pdata->regmap));
}
static DEVICE_ATTR_RO(reg_size);

static int ciena_sysfs_regmap_probe(struct platform_device *pdev)
{
	struct ciena_sysfs_regmap_pdata *pdata = pdev->dev.platform_data;
	struct regmap_sysfs_priv        *priv;

	dev_info(&pdev->dev, "%s entry\n", REGMAP_SYSFS_DRIVER_NAME);
	if (NULL == pdata)
		return -ENODEV;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "no memory for private data\n");
		return -ENOMEM;
	}

	if (device_create_file(&pdev->dev, &dev_attr_offset_and_or))
		dev_warn(&pdev->dev, "offset_and_or file creation failed\n");

	if (device_create_file(&pdev->dev, &dev_attr_reg_size))
		dev_warn(&pdev->dev, "reg_size file creation failed\n");

	/* bin files need a size, only known at device creation time */
	sysfs_bin_attr_init(&priv->read_attr);

	priv->read_attr.attr.name = "read";
	priv->read_attr.attr.mode = S_IRUGO;
	priv->read_attr.read      = ciena_sysfs_regmap_read;
	priv->read_attr.size      = (regmap_get_max_register(pdata->regmap) *
				     regmap_get_reg_stride(pdata->regmap));

	if (device_create_bin_file(&pdev->dev, &priv->read_attr))
		dev_warn(&pdev->dev, "read file creation failed\n");

	dev_set_drvdata(&pdev->dev, priv);

	/* nudge a uevent from the device, because adding files under
	 * the platform device will not */
	kobject_uevent(&pdev->dev.kobj, KOBJ_ADD);

	dev_info(&pdev->dev, "%s exit\n", REGMAP_SYSFS_DRIVER_NAME);
	return 0;
}

static void ciena_sysfs_regmap_remove(struct platform_device *pdev)
{
	struct regmap_sysfs_priv *priv = dev_get_drvdata(&pdev->dev);

	kobject_uevent(&pdev->dev.kobj, KOBJ_REMOVE);

	device_remove_bin_file(&pdev->dev, &priv->read_attr);
	device_remove_file(&pdev->dev, &dev_attr_reg_size);
	device_remove_file(&pdev->dev, &dev_attr_offset_and_or);
}


static struct platform_driver ciena_sysfs_regmap_driver = {
	.probe	  = ciena_sysfs_regmap_probe,
	.remove	  = ciena_sysfs_regmap_remove,
	.driver   = {
		.name		= REGMAP_SYSFS_DRIVER_NAME,
		.owner		= THIS_MODULE,
	},
};
module_platform_driver(ciena_sysfs_regmap_driver);

MODULE_AUTHOR("Ron Belaire");
MODULE_DESCRIPTION("Ciena sysfs regmap interface");
MODULE_LICENSE("GPL");
