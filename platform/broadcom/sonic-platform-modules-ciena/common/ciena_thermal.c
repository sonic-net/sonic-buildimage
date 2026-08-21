/*
    ciena_thermal.c - Driver for raw access to a regmap client

    Copyright (C) 2024 Ciena Corporation

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
#include <linux/thermal.h>

#include "kcompat.h"
#include "ciena_thermal.h"

/* Have one threhold/trip per name so it can be used as a key */
#define CIENA_THERMAL_NUM_TRIPS 1

struct ciena_thermal_data {
	struct thermal_zone_device *tzd;
	const char                 *name;
	struct regmap              *map;
	unsigned int                reg;
	unsigned int                valid_mask;
	unsigned int                temp_mask;
	unsigned int                temp_shift;
	unsigned int                temp_qnbits;
	unsigned int                temp_unsigned;
	unsigned int                threshold;
	struct thermal_trip         trips[CIENA_THERMAL_NUM_TRIPS];
};

static int ciena_thermal_get_temp(struct thermal_zone_device *tz, int *temp)
{
	struct ciena_thermal_data *thermal_data = thermal_zone_device_priv(tz);
	struct device             *dev          = thermal_zone_device(tz);
	unsigned int               value        = 0;
	unsigned int               rtemp        = 0;
	unsigned int               valid        = 0;
	int                        rc;
	int                        sample;
	int                        milliC;

	dev_dbg(dev, "Entry get_temp\n");
	if (thermal_data->threshold) {
		*temp = 0;
		return 0;
	}

	rc = regmap_read(thermal_data->map, thermal_data->reg, &value);
	if (rc < 0) {
		dev_err(dev,
			"error getting thermal_data status\n");
		return -EINVAL;
	}

	dev_dbg(dev,
		"reg=%08x value=%08x mask=%08x shift=%d\n",
		thermal_data->reg,
		value,
		thermal_data->temp_mask,
		thermal_data->temp_shift);
	rtemp = (value & thermal_data->temp_mask) >> thermal_data->temp_shift;
	valid = !!(value & thermal_data->valid_mask);
	dev_dbg(dev, "rtemp=%d valid=%d\n", rtemp, valid);

	if (thermal_data->valid_mask && valid==0) {
		dev_dbg(dev, "read temp failed, not valid\n");
		return -EIO;
	}

	/*
	 * The Q number format is Qm.n, where:
	 *    * m is the integer part (m bits wide)
	 *    * n is the binary fraction part (n bits wide)
	 *
	 * For instance, 0x1f means:
	 *    * 31     in Q8.0
	 *    * -1     in Q5.0
	 *    * -0.25  in Q3.2
	 *    *  3.875 in Q3.3
	 *    * 15.5   in Q7.1
	 */

	/* milliCelsuis value (zero if temp_qnbits is zero) */
	milliC = rtemp & ((1 << thermal_data->temp_qnbits) - 1);
	milliC = (1000 * milliC) / (1 << thermal_data->temp_qnbits);

	/* Celsius value field (unchanged if temp_qnbits is zero) */
	rtemp >>= thermal_data->temp_qnbits;

	if (thermal_data->temp_unsigned)
		sample = rtemp;
	else
		/* The most significant bit is the sign bit */
		sample = sign_extend32(rtemp,
				       fls(thermal_data->temp_mask) -
				       (1 + thermal_data->temp_shift) -
				       thermal_data->temp_qnbits);

	*temp = (sample * 1000) + milliC;

	return 0;
}

static int ciena_thermal_get_trip(struct platform_device *pdev,
				  struct ciena_thermal_data *thermal_data,
				  int *p_temp)
{
	struct device *dev   = &pdev->dev;
	unsigned int   value = 0;
	unsigned int   rtemp = 0;
	int            rc;
	int            sample;

	dev_dbg(dev, "Entry get_trip\n");

	rc = regmap_read(thermal_data->map, thermal_data->reg, &value);
	if (rc < 0) {
		dev_err(dev,
			"error getting thermal_data status\n");
		return -EINVAL;
	}

	dev_dbg(dev,
		"reg=%08x value=%08x mask=%08x shift=%d\n",
		thermal_data->reg,
		value,
		thermal_data->temp_mask,
		thermal_data->temp_shift);
	rtemp = (value & thermal_data->temp_mask) >> thermal_data->temp_shift;
	dev_dbg(dev, "rtemp=%d\n", rtemp);

	if (thermal_data->temp_unsigned)
		sample = rtemp;
	else
		/* The most significant bit is the sign bit */
		sample = sign_extend32(rtemp,
				       fls(thermal_data->temp_mask) -
				       (1+ thermal_data->temp_shift));

	*p_temp = sample * 1000;

	return 0;
}

static int ciena_thermal_set_trip_temp(struct thermal_zone_device *tz,
				       const struct thermal_trip *trip,
				       int temp)
{
	struct ciena_thermal_data *thermal_data = thermal_zone_device_priv(tz);
	struct device             *dev          = thermal_zone_device(tz);
	int                        temp_max;
	int                        temp_min;
	int                        rc;
	unsigned int               trip_index;

	dev_dbg(dev, "Entry set_trip\n");

	trip_index = THERMAL_TRIP_PRIV_TO_INT(trip->priv);

	if (trip_index >= CIENA_THERMAL_NUM_TRIPS)
		return -EINVAL;

	temp = temp / 1000;

	temp_max =   thermal_data->temp_mask;
	temp_max >>= (0 == thermal_data->temp_unsigned);
	temp_min =   0 - (thermal_data->temp_mask & ~temp_max);

	if (temp_max < temp) {
		dev_dbg(dev, "%d < %d\n", temp_max, temp);
		return -EINVAL;
	}

	if (temp_min > temp) {
		dev_dbg(dev, "%d > %d\n", temp_min, temp);
		return -EINVAL;
	}

	rc = regmap_write_bits(thermal_data->map,
			       thermal_data->reg,
			       thermal_data->temp_mask,
			       temp << thermal_data->temp_shift);
	if (rc < 0) {
		dev_err(dev,
			"error setting thermal_data status\n");
		return -EINVAL;
	}

	dev_dbg(dev,
		"reg=%08x value=%08x mask=%08x shift=%d\n",
		thermal_data->reg,
		temp,
		thermal_data->temp_mask,
		thermal_data->temp_shift);

	return 0;
}

static struct thermal_zone_device_ops ops = {
	.get_temp      = ciena_thermal_get_temp,
	.set_trip_temp = ciena_thermal_set_trip_temp,
};

static int ciena_thermal_parse_pdata(struct platform_device    *pdev,
				     struct ciena_thermal_data *data)
{
	struct ciena_thermal_pdata   *pdata = pdev->dev.platform_data;
	struct device                *dev   = &pdev->dev;
	struct regmap                *map;

	if (pdata) {
		/* use the platform data if it is provided */
		if (pdata->regmap) {
			dev_dbg(dev, "using pdata->regmap %p\n", pdata->regmap);
			data->map = pdata->regmap;
		}
		else if ((map = dev_get_regmap(dev->parent, NULL))) {
			dev_dbg(dev, "using dev_get_regmap() %p\n", map);
			data->map = map;
		}
		else {
			dev_err(dev, "cannot get regmap\n");
			return -ENODEV;
		}
		data->reg           = pdata->reg;
		data->name          = pdata->name;
		data->valid_mask    = pdata->valid_mask;
		data->temp_mask     = pdata->temp_mask;
		data->temp_shift    = pdata->temp_shift;
		data->temp_qnbits   = pdata->temp_qnbits;
		data->temp_unsigned = pdata->temp_unsigned;
		data->threshold     = pdata->threshold;
		return 0;
	}

	dev_err(dev, "missing platform data\n");
	return -EINVAL;
}

/* ------------------------------------------------------------------------- */
static int ciena_thermal_probe(struct platform_device *pdev)
{
	struct ciena_thermal_data *thermal_data;
	struct device             *dev          = &pdev->dev;
	int                        rc;
	int                        num_trips    = CIENA_THERMAL_NUM_TRIPS;

	dev_dbg(&pdev->dev, "%s entry\n", CIENA_THERMAL_NAME);

	thermal_data = devm_kzalloc(dev, sizeof(*thermal_data), GFP_KERNEL);
	if (!thermal_data) {
		dev_err(dev, "no memory for private data\n");
		return -ENOMEM;
	}

	rc = ciena_thermal_parse_pdata(pdev, thermal_data);
	if (rc) return rc;

	if (thermal_data->threshold) {
		dev_info(&pdev->dev, "Adding thermal threshold: %s\n", thermal_data->name);
		rc = ciena_thermal_get_trip(pdev, thermal_data,
					    &thermal_data->trips[0].temperature);
		if (rc) return rc;
		thermal_data->trips[0].type = THERMAL_TRIP_PASSIVE;
		thermal_data->trips[0].priv = THERMAL_INT_TO_TRIP_PRIV(0);
	} else {
		num_trips = 0;
		dev_info(&pdev->dev, "Adding thermal sensor:    %s\n", thermal_data->name);
	}
	thermal_data->tzd = thermal_zone_device_register_with_trips(
		thermal_data->name, thermal_data->trips, num_trips,
		thermal_data, &ops, NULL, 0, 0);

	if (IS_ERR(thermal_data->tzd)) {
		rc = PTR_ERR(thermal_data->tzd);
		dev_err(&pdev->dev, "failed to register thermal zone\n");
		return rc;
	}

	platform_set_drvdata(pdev, thermal_data);

	dev_dbg(&pdev->dev, "%s exit\n", CIENA_THERMAL_NAME);

	return 0;
}

static void ciena_thermal_remove(struct platform_device *pdev)
{
	struct ciena_thermal_data  *thermal_data = platform_get_drvdata(pdev);

	thermal_zone_device_unregister(thermal_data->tzd);
}

/* ------------------------------------------------------------------------- */
static const struct platform_device_id ciena_thermal_platform_ids[] = {
	{ .name = CIENA_THERMAL_NAME },
	{ },
};
MODULE_DEVICE_TABLE(platform, ciena_thermal_platform_ids);

static struct platform_driver ciena_thermal_driver = {
	.probe    = ciena_thermal_probe,
	.remove   = ciena_thermal_remove,
	.id_table = ciena_thermal_platform_ids,
	.driver   = {
		.name  = CIENA_THERMAL_NAME,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(ciena_thermal_driver);

MODULE_AUTHOR("Ron Belaire <rbelaire@ciena.com>");
MODULE_DESCRIPTION("generic thermal sysfs provides a set of interfaces for thermal zone devices (sensors)");
MODULE_LICENSE("GPL");
// vim: sw=8 noet
