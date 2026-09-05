/*
 * Juniper Networks TMC fpga PSU driver
 *
 * This driver is for detecting if the PSU is present or not
 *
 * Copyright (C) 2020 Juniper Networks
 * Author: Ciju Rajan K <crajank@juniper.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/hwmon.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/hwmon-sysfs.h>
#include <linux/errno.h>
#include <linux/string.h>

/* Max PSUs supported by this driver */
#define NUM_PSU		2

struct tmc_psu_data {
	int num_psu;
	void __iomem *tmc_membase;
};

enum sysfs_psu_attributes {
	PSU0_PRESENT,
	PSU1_PRESENT,
};

static bool get_psu_presense(void *addr, u8 idx)
{
	bool ret = 0;
	u32 value = ~(-1);

	value = ioread32(addr);
	/*
	 * BIT(6) is for PSU 0
	 * BIT(7) is for PSU 1
	 * idx will be either 0 (PSU0) or 1 (PSU1)
	 */
	value &= BIT(idx+6);

	if (value)
		ret = 1;

	return ret;
}

/*
 * Sysfs files are present in this path
 * /sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/psu-tmc.15/psu*_present
 */

#define DECLARE_PSU_PRESENT_SENSOR_DEV_ATTR(index) \
	    static SENSOR_DEVICE_ATTR(psu##index##_present, S_IRUGO, tmc_psu_presense_show, NULL, PSU##index##_PRESENT)
#define DECLARE_PSU_PRESENT_ATTR(index)      &sensor_dev_attr_psu##index##_present.dev_attr.attr

static ssize_t tmc_psu_presense_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
	struct platform_device *pdev = to_platform_device(dev);
	struct tmc_psu_data *psu = platform_get_drvdata(pdev);	
	
	return sprintf(buf, "%d\n", get_psu_presense(psu->tmc_membase, s_attr->index));

}

DECLARE_PSU_PRESENT_SENSOR_DEV_ATTR(0);
DECLARE_PSU_PRESENT_SENSOR_DEV_ATTR(1);

static struct attribute *tmc_psu_attrs[] = {
	DECLARE_PSU_PRESENT_ATTR(0),
	DECLARE_PSU_PRESENT_ATTR(1),
	NULL
};

static struct attribute_group tmc_psu_attr_group = {
	.attrs = tmc_psu_attrs,
};

static int tmc_psu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct tmc_psu_data *psu;
	int ret;
	struct resource *res;
	void __iomem *addr;

	psu = devm_kzalloc(dev, sizeof(*psu), GFP_KERNEL);
	if (!psu) {
		dev_err(dev, "psu structure allocation failed\n");
		return -ENOMEM;
	}
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "res allocation failed\n");
		return -ENODEV;
	}
	
	addr = devm_ioremap(dev, res->start, resource_size(res));
	if (!addr) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}

	psu->tmc_membase = addr;
	psu->num_psu = NUM_PSU;

	platform_set_drvdata(pdev, psu);

	ret = sysfs_create_group(&dev->kobj, &tmc_psu_attr_group);
	if (ret != 0) {
		dev_err(dev, "jnx-tmc-psu: sysfs_create_group failed: %d\n", ret);
		return ret;
	} 
	
	return 0;
}

static int tmc_psu_remove(struct platform_device *pdev)
{
	struct tmc_psu_data *psu = platform_get_drvdata(pdev);

	if (psu) {
		devm_kfree(&pdev->dev, psu);
	}
	sysfs_remove_group(&pdev->dev.kobj, &tmc_psu_attr_group);

	return 0;
}

static struct platform_driver jnx_tmc_psu_driver = {
	.driver = {
		.name  = "psu-tmc",
		.owner  = THIS_MODULE,
	},
	.probe = tmc_psu_probe,
	.remove = tmc_psu_remove,
};

static int __init jnx_tmc_psu_driver_init(void)
{
	int ret = -1;

	ret = platform_driver_register(&jnx_tmc_psu_driver);
	
	return ret;

}

static void __exit jnx_tmc_psu_driver_exit(void)
{
	platform_driver_unregister(&jnx_tmc_psu_driver);
}

module_init(jnx_tmc_psu_driver_init);
module_exit(jnx_tmc_psu_driver_exit);

MODULE_DESCRIPTION("Juniper Networks TMC PSU driver");
MODULE_AUTHOR("Ciju Rajan K <crajank@juniper.net>");
MODULE_LICENSE("GPL");
