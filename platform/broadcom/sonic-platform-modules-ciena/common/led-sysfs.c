/*
 * Copyright 2022 Ciena Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "led-sysfs.h"

struct ciena_led_data {
	struct led_classdev  cdev;
	struct regmap       *map;
	unsigned int         reg;
	unsigned int         mask;
	unsigned int         val;
	bool                 use_val;
	bool                 invert;
	unsigned int         blnk;
};

static void ciena_led_sysfs_set(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	struct ciena_led_data *led = container_of(led_cdev, struct ciena_led_data, cdev);
	u32                    val;
	int                    rc;

	// Logical XOR
	if (!led->invert != (value == LED_OFF)) {
		if (led->use_val)         // Useful when less bits than
			val = led->val;   // mask are to be set
		else
			val = led->mask;
	}
	else
		val = 0;

	rc = regmap_update_bits(led->map, led->reg, led->mask, val);
	if (rc < 0)
		dev_err(led->cdev.dev, "error updating LED status\n");
}

static enum led_brightness ciena_led_sysfs_get(struct led_classdev *led_cdev)
{
	struct ciena_led_data *led        = container_of(led_cdev, struct ciena_led_data, cdev);
	unsigned int           led_status = 0;
	int                    rc;

	rc = regmap_read(led->map, led->reg, &led_status);
	if (rc < 0) {
		dev_err(led->cdev.dev, "error getting LED status\n");
		return LED_OFF;
	}

	led_status &= led->mask;

	// Logical XOR
	if (!led->invert != !(led_status == (led->use_val ? led->val : led->mask)))
		return LED_ON;
	else
		return LED_OFF;
}

static ssize_t ciena_led_sysfs_blink_show(struct device *dev,
					  struct device_attribute *attr, char *buf)
{
	struct led_classdev   *led_cdev   = dev_get_drvdata(dev);
	struct ciena_led_data *led        = container_of(led_cdev, struct ciena_led_data, cdev);
	int                    blinking   = 0;
	unsigned int           led_status = 0;
	int                    rc;

	rc = regmap_read(led->map, led->reg, &led_status);
	if (rc < 0) return rc;

	if (led_status & led->blnk)
		blinking = 1;
	return sprintf(buf, "%u\n", blinking);
}

static ssize_t ciena_led_sysfs_blink_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	struct led_classdev   *led_cdev    = dev_get_drvdata(dev);
	struct ciena_led_data *led         = container_of(led_cdev, struct ciena_led_data, cdev);
	int                    rc;
	unsigned long          blink_state;

	rc = kstrtoul(buf, 10, &blink_state);
	if (rc) return rc;

	regmap_update_bits(led->map, led->reg, led->blnk, blink_state ? led->blnk : 0);

	return size;
}

static DEVICE_ATTR(blink, 0644, ciena_led_sysfs_blink_show, ciena_led_sysfs_blink_store);

static struct attribute *ciena_led_sysfs_attrs[] = {
	&dev_attr_blink.attr,
	NULL
};
ATTRIBUTE_GROUPS(ciena_led_sysfs);

static int ciena_led_sysfs_parse_pdata(struct platform_device *pdev,
				       struct ciena_led_data *data)
{
	struct ciena_led_sysfs_pdata *pdata = pdev->dev.platform_data;
	struct device                *dev   = &pdev->dev;
	struct regmap                *map   = dev_get_regmap(dev->parent, NULL);

	if (!map)
		return -ENODEV;

	if (pdata) {
		/* use the platform data if it is provided */
		data->cdev.name           = pdata->name;
		data->cdev.brightness_set = ciena_led_sysfs_set;
		data->cdev.brightness_get = ciena_led_sysfs_get;
		if (pdata->blnk)
			data->cdev.groups = ciena_led_sysfs_groups;
		data->map                 = map;
		data->reg                 = pdata->reg;
		data->mask                = pdata->mask;
		data->val                 = pdata->val;
		data->use_val             = pdata->use_val;
		data->invert              = pdata->invert;
		data->blnk                = pdata->blnk;
		return 0;
	}

	return -EINVAL;
}

static int ciena_led_sysfs_probe(struct platform_device *pdev)
{
	struct device         *dev  = &pdev->dev;
	struct ciena_led_data *led_data;
	int                    rc;

	led_data = devm_kzalloc(dev, sizeof(*led_data), GFP_KERNEL);
	if (!led_data)
		return -ENOMEM;

	rc = ciena_led_sysfs_parse_pdata(pdev, led_data);
	if (rc) return rc;

	rc = devm_led_classdev_register(&pdev->dev, &led_data->cdev);
	if (rc) {
		dev_err(&pdev->dev, "%s: registration failed (%d)\n",
			__func__, rc);
		return rc;
	}
	if (rc < 0)
		return rc;

	platform_set_drvdata(pdev, led_data);

	dev_info(&pdev->dev, "registered %s\n", led_data->cdev.name);

	return 0;
}

static void ciena_led_sysfs_remove(struct platform_device *pdev)
{
	struct ciena_led_data *led_data = platform_get_drvdata(pdev);

	if (led_data) {
		dev_info(&pdev->dev, "unregistrating %s\n", led_data->cdev.name);
		devm_led_classdev_unregister(&pdev->dev, &led_data->cdev);
	}
}

static struct platform_driver ciena_led_sysfs_driver = {
	.probe	  = ciena_led_sysfs_probe,
	.remove   = ciena_led_sysfs_remove,
	.driver   = {
		.name		= LED_SYSFS_DRIVER_NAME,
		.owner		= THIS_MODULE,
	},
};
module_platform_driver(ciena_led_sysfs_driver);

MODULE_AUTHOR("Ron Belaire");
MODULE_DESCRIPTION("Ciena LED Sysfs Driver");
MODULE_LICENSE("GPL");

/* set ts=8 noet sw=8 */
