/*
 * A hwmon driver for the Accton as9926 24d fan
 *
 * Copyright (C) 2016 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/io.h>

#define DRVNAME "as9716_32d_ioport"
#define IOPORT_I2C_MUX_RST 0x50D

static ssize_t show_i2c_mux_rst(struct device *dev, struct device_attribute *da,
		char *buf);
static ssize_t set_i2c_mux_rst(struct device *dev, struct device_attribute *da,
		const char *buf, size_t count);

static struct as9716_ioport_data *data = NULL;

/* ioport data */
struct as9716_ioport_data {
	struct platform_device *pdev;
	struct device *hwmon_dev;
	struct mutex update_lock;
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(i2c_mux_rst, S_IRUGO | S_IWUSR, show_i2c_mux_rst,
			set_i2c_mux_rst, 0);

static struct attribute *sys_attributes[] = {
	&sensor_dev_attr_i2c_mux_rst.dev_attr.attr,
	NULL
};

static struct attribute_group sys_group = {
	.attrs = sys_attributes,
};

static ssize_t show_i2c_mux_rst(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u8 val = 0;
	mutex_lock(&data->update_lock);
	val = inb(IOPORT_I2C_MUX_RST);
	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d\n", !(val & 0xEF));
}

static ssize_t set_i2c_mux_rst(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	u8 val = 0;
	long value = 0;
	int status = 0;

	status = kstrtol(buf, 10, &value);
	if (status)
		return status;

	mutex_lock(&data->update_lock);

	val = inb(IOPORT_I2C_MUX_RST);
	if (value)
		outb(val & 0xEF, IOPORT_I2C_MUX_RST);
	else
		outb(val | 0x10, IOPORT_I2C_MUX_RST);

	mutex_unlock(&data->update_lock);
	return count;
}

static int as9716_32d_ioport_probe(struct platform_device *pdev)
{
	int status = -1;

	request_region(IOPORT_I2C_MUX_RST, 1, "mux_rst");

	/* Register sysfs hooks */
	status = sysfs_create_group(&pdev->dev.kobj, &sys_group);
	if (status) {
		goto exit;
	}

	dev_info(&pdev->dev, "device created\n");
	return 0;

exit:
	sysfs_remove_group(&pdev->dev.kobj, &sys_group);
	return status;
}

static int as9716_32d_ioport_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &sys_group);
	release_region(IOPORT_I2C_MUX_RST, 1);
	return 0;
}

static struct platform_driver as9716_32d_ioport_driver = {
	.probe	  = as9716_32d_ioport_probe,
	.remove	 = as9716_32d_ioport_remove,
	.driver	 = {
		.name   = DRVNAME,
		.owner  = THIS_MODULE,
	},
};

static int __init as9716_32d_ioport_init(void)
{
	int ret;

	data = kzalloc(sizeof(struct as9716_ioport_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	mutex_init(&data->update_lock);

	ret = platform_driver_register(&as9716_32d_ioport_driver);
	if (ret < 0)
		goto dri_reg_err;

	data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(data->pdev)) {
		ret = PTR_ERR(data->pdev);
		goto dev_reg_err;
	}

	return 0;

dev_reg_err:
	platform_driver_unregister(&as9716_32d_ioport_driver);
dri_reg_err:
	kfree(data);
alloc_err:
	return ret;
}

static void __exit as9716_32d_ioport_exit(void)
{
	platform_device_unregister(data->pdev);
	platform_driver_unregister(&as9716_32d_ioport_driver);
	kfree(data);
}

module_init(as9716_32d_ioport_init);
module_exit(as9716_32d_ioport_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as9716_32d_ioport driver");
MODULE_LICENSE("GPL");
