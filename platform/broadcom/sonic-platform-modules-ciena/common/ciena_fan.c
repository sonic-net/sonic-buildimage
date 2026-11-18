/*
    ciena_fan.c - Driver for raw access to a regmap client

    Copyright (C) 2025 Ciena Corporation

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.
*/

#include <linux/device.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kobject.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#include "ciena_fan.h"

// Number of possible groups + NULL
#define GROUPS 6

struct ciena_fan_data {
	struct ciena_fan_pdata       *pdata;
	struct regmap                *regmap;
	const struct attribute_group *attr_groups[GROUPS];
};

static ssize_t fan_input_show(struct device *dev,
			      struct device_attribute *devattr,
			      char *buf)
{
	struct ciena_fan_data *fan_data = dev_get_drvdata(dev);
	unsigned int           value    = 0;
	int                    rc;

	rc = regmap_read(fan_data->regmap, fan_data->pdata->tach_reg, &value);
	if (rc < 0) {
		dev_err(dev,
			"error getting tach_reg\n");
		return -EINVAL;
	}
	dev_dbg(dev,
		"reg=%08x value=%08x mask=%08x shift=%u\n",
		fan_data->pdata->tach_reg,
		value,
		fan_data->pdata->tach_mask,
		fan_data->pdata->tach_shift);

	value = (value & fan_data->pdata->tach_mask) >> fan_data->pdata->tach_shift;

	dev_dbg(dev, "value=%u\n", value);
	return sprintf(buf, "%u\n", value);
}

static ssize_t fan_fault_show(struct device *dev,
			      struct device_attribute *devattr,
			      char *buf)
{
	struct ciena_fan_data *fan_data = dev_get_drvdata(dev);
	unsigned int           value    = 0;
	int                    rc;

	rc = regmap_read(fan_data->regmap, fan_data->pdata->stat_reg, &value);
	if (rc < 0) {
		dev_err(dev,
			"error getting stat_reg\n");
		return -EINVAL;
	}
	dev_dbg(dev,
		"reg=%08x value=%08x mask=%08x invert=%08x\n",
		fan_data->pdata->stat_reg,
		value,
		fan_data->pdata->fault_mask,
		fan_data->pdata->fault_invert);

	value = !!(value & fan_data->pdata->fault_mask);
	value ^= fan_data->pdata->fault_invert;

	dev_dbg(dev, "value=%u\n", value);
	return sprintf(buf, "%u\n", value);
}

static ssize_t fan_present_show(struct device *dev,
				struct device_attribute *devattr,
				char *buf)
{
	struct ciena_fan_data *fan_data = dev_get_drvdata(dev);
	unsigned int           value    = 0;
	int                    rc;

	if (fan_data->pdata->pres_mask) {
		rc = regmap_read(fan_data->regmap, fan_data->pdata->stat_reg, &value);
		if (rc < 0) {
			dev_err(dev,
				"error getting stat_reg\n");
			return -EINVAL;
		}
		dev_dbg(dev,
			"reg=%08x value=%08x mask=%08x invert=%08x\n",
			fan_data->pdata->stat_reg,
			value,
			fan_data->pdata->pres_mask,
			fan_data->pdata->pres_invert);

		value = !!(value & fan_data->pdata->pres_mask);
		value ^= fan_data->pdata->pres_invert;
	}
	else
		value = 1;	// Assume present if no h/w detection

	dev_dbg(dev, "value=%u\n", value);
	return sprintf(buf, "%u\n", value);
}

static ssize_t thres_low_crit_show(struct device *dev,
				   struct device_attribute *devattr,
				   char *buf)
{
	struct ciena_fan_data *fan_data = dev_get_drvdata(dev);
	unsigned int           value    = fan_data->pdata->thres_low_crit;
	int                    rc;

	if (fan_data->pdata->thres_reg) {
		rc = regmap_read(fan_data->regmap, fan_data->pdata->thres_reg, &value);
		if (rc < 0) {
			dev_err(dev,
				"error getting thres_reg\n");
			return -EINVAL;
		}
		dev_dbg(dev,
			 "reg=%08x value=%08x mask=%08x shift=%u\n",
			 fan_data->pdata->thres_reg,
			 value,
			 fan_data->pdata->thres_min_mask,
			 fan_data->pdata->thres_min_shift);

		value = (value & fan_data->pdata->thres_min_mask) >>
			fan_data->pdata->thres_min_shift;
	}

	dev_dbg(dev, "value=%u\n", value);
	return sprintf(buf, "%u\n", value);
}

static ssize_t thres_max_norm_show(struct device *dev,
				   struct device_attribute *devattr,
				   char *buf)
{
	struct ciena_fan_data *fan_data = dev_get_drvdata(dev);
	unsigned int           value    = fan_data->pdata->thres_max_norm;
	int                    rc;

	if (fan_data->pdata->thres_reg) {
		rc = regmap_read(fan_data->regmap, fan_data->pdata->thres_reg, &value);
		if (rc < 0) {
			dev_err(dev,
				"error getting thres_reg\n");
			return -EINVAL;
		}
		dev_dbg(dev,
			 "reg=%08x value=%08x mask=%08x shift=%u\n",
			 fan_data->pdata->thres_reg,
			 value,
			 fan_data->pdata->thres_max_mask,
			 fan_data->pdata->thres_max_shift);

		value = (value & fan_data->pdata->thres_max_mask) >>
			fan_data->pdata->thres_max_shift;
	}

	dev_dbg(dev, "value=%u\n", value);
	return sprintf(buf, "%u\n", value);
}

static SENSOR_DEVICE_ATTR_RO(fan_input,      fan_input,      0);
static SENSOR_DEVICE_ATTR_RO(fan_fault,      fan_fault,      0);
static SENSOR_DEVICE_ATTR_RO(fan_present,    fan_present,    0);
static SENSOR_DEVICE_ATTR_RO(thres_low_crit, thres_low_crit, 0);
static SENSOR_DEVICE_ATTR_RO(thres_max_norm, thres_max_norm, 0);

// Attributes
static struct attribute *ciena_fan_input_attrs[] = {
	&sensor_dev_attr_fan_input.dev_attr.attr,
	NULL
};

static struct attribute *ciena_fan_fault_attrs[] = {
	&sensor_dev_attr_fan_fault.dev_attr.attr,
	NULL
};

static struct attribute *ciena_fan_present_attrs[] = {
	&sensor_dev_attr_fan_present.dev_attr.attr,
	NULL
};

static struct attribute *ciena_thres_low_crit_attrs[] = {
	&sensor_dev_attr_thres_low_crit.dev_attr.attr,
	NULL
};

// Groups
static struct attribute *ciena_thres_max_norm_attrs[] = {
	&sensor_dev_attr_thres_max_norm.dev_attr.attr,
	NULL
};

static const struct attribute_group ciena_fan_input_group = {
	.attrs = ciena_fan_input_attrs,
};

static const struct attribute_group ciena_fan_fault_group = {
	.attrs = ciena_fan_fault_attrs,
};

static const struct attribute_group ciena_fan_present_group = {
	.attrs = ciena_fan_present_attrs,
};

static const struct attribute_group ciena_thres_low_crit_group = {
	.attrs = ciena_thres_low_crit_attrs,
};

static const struct attribute_group ciena_thres_max_norm_group = {
	.attrs = ciena_thres_max_norm_attrs,
};

static int ciena_fan_parse_pdata(struct platform_device *pdev,
				 struct ciena_fan_data  *fan_data)
{
	struct ciena_fan_pdata *pdata      = pdev->dev.platform_data;
	struct device          *dev        = &pdev->dev;
	struct regmap          *regmap;
	unsigned int            group_tail = 0;

	if (pdata) {
		/* Grab the pointer to the platform data */
		fan_data->pdata = pdata;

		/* use the platform data if it is provided */
		if (pdata->regmap) {
			dev_dbg(dev, "using pdata->regmap %p\n", pdata->regmap);
			fan_data->regmap = pdata->regmap;
		}
		else if ((regmap = dev_get_regmap(dev->parent, NULL))) {
			dev_dbg(dev, "using dev_get_regmap() %p\n", regmap);
			fan_data->regmap = regmap;
		}
		else {
			dev_err(dev, "cannot get regmap\n");
			return -ENODEV;
		}

		fan_data->attr_groups[group_tail++] = &ciena_fan_input_group;
		fan_data->attr_groups[group_tail++] = &ciena_fan_present_group;
		if (fan_data->pdata->fault_mask)
			fan_data->attr_groups[group_tail++] = &ciena_fan_fault_group;
		if (fan_data->pdata->thres_reg       |
		    fan_data->pdata->thres_low_crit  |
		    fan_data->pdata->thres_max_norm) {
			fan_data->attr_groups[group_tail++] = &ciena_thres_low_crit_group;
			fan_data->attr_groups[group_tail++] = &ciena_thres_max_norm_group;
		}

		return 0;
	}

	dev_err(dev, "missing platform data\n");
	return -EINVAL;
}

/* ------------------------------------------------------------------------- */
static int ciena_fan_probe(struct platform_device *pdev)
{
	struct ciena_fan_data *fan_data;
	struct device         *dev        = &pdev->dev;
	int                    rc;
	struct device         *classdev;

	dev_dbg(&pdev->dev, "%s entry\n", CIENA_FAN_NAME);

	fan_data = devm_kzalloc(dev, sizeof(*fan_data), GFP_KERNEL);
	if (!fan_data) {
		dev_err(dev, "no memory for private data\n");
		return -ENOMEM;
	}

	rc = ciena_fan_parse_pdata(pdev, fan_data);
	if (rc) return rc;

	dev_info(&pdev->dev, "Adding fan sensor: %s\n", fan_data->pdata->name);
	classdev = devm_hwmon_device_register_with_groups(dev,
							  fan_data->pdata->name,
							  fan_data,
							  fan_data->attr_groups);

	platform_set_drvdata(pdev, fan_data);

	dev_dbg(&pdev->dev, "%s exit\n", CIENA_FAN_NAME);

	return PTR_ERR_OR_ZERO(classdev);
}

/* ------------------------------------------------------------------------- */
static const struct platform_device_id ciena_fan_platform_ids[] = {
	{ .name = CIENA_FAN_NAME },
	{ },
};
MODULE_DEVICE_TABLE(platform, ciena_fan_platform_ids);

static struct platform_driver ciena_fan_driver = {
	.probe    = ciena_fan_probe,
	.id_table = ciena_fan_platform_ids,
	.driver   = {
		.name  = CIENA_FAN_NAME,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(ciena_fan_driver);

MODULE_AUTHOR("Ron Belaire <rbelaire@ciena.com>");
MODULE_DESCRIPTION("generic fan sysfs provides a set of interfaces for fan devices");
MODULE_LICENSE("GPL");
// vim: sw=8 noet
