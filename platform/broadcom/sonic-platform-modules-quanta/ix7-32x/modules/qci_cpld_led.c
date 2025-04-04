/*
 * A LED CPLD driver for Quanta Switch Platform
 *
 * The CPLD is customize by Quanta for decode led bit stream,
 * This driver modify from Quanta CPLD I/O driver.
 *
 * Copyright (C) 2015 Quanta Inc.
 *
 * Author: Luffy Cheng <luffy.cheng@quantatw.com>
 * Author: Roger Chang <Roger.Chang@quantatw.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/idr.h>

static DEFINE_IDA(cpld_led_ida);

enum platform_type {
	IX7 = 0,
	IX8,
	NONE
};

static struct class *cpld_class = NULL;

struct cpld_data {
	struct i2c_client *cpld_client;
	char name[16];
	u8 cpld_id;
};

struct cpld_led_data {
	struct mutex lock;
	struct device *port_dev;
	struct cpld_data *cpld_data;
};

static int cpld_led_probe(struct i2c_client *client,
			 const struct i2c_device_id *id);
static int cpld_led_remove(struct i2c_client *client);

static const struct i2c_device_id cpld_led_id[] = {
	{ "CPLDLED_IX7", IX7 },
	{ "CPLDLED_IX8", IX8 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cpld_led_id);

static struct i2c_driver cpld_led_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "qci_cpld_led",
	},
	.probe		= cpld_led_probe,
	.remove		= cpld_led_remove,
	.id_table	= cpld_led_id,
//	.address_list	= normal_i2c,
};

#define CPLD_LED_ID_PREFIX "CPLDLED-"
#define CPLD_LED_ID_FORMAT CPLD_LED_ID_PREFIX "%d"

#define	CPLD_DECODER_OFFSET	0x4
#define	CPLD_DECODER_MASK	0x1
#define CPLD_USERCODE_START_OFFSET	0x0

static ssize_t get_led_decode(struct device *dev,
			     struct device_attribute *devattr,
			     char *buf)
{
	struct cpld_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->cpld_client;
	u8 offset = (u8)(CPLD_DECODER_OFFSET);
	s32 value;

	value = i2c_smbus_read_byte_data(client, offset);
	if (value < 0)
		return -ENODEV;

	dev_dbg(&client->dev, "read led decode value= %x\n", value);

	value &= CPLD_DECODER_MASK;

	return sprintf(buf, "%d\n", (value == 0) ? 1 : 0);
}

static ssize_t get_usercode(struct device *dev,
			     struct device_attribute *devattr,
			     char *buf)
{
	struct cpld_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->cpld_client;
	u8 i = 0;
	s32 value = 0, reading = 0;

	for (i = 0; i < 4; i++)
	{
		reading = i2c_smbus_read_byte_data(client, CPLD_USERCODE_START_OFFSET + i);
		if (reading < 0)
			return -ENODEV;

		dev_dbg(&client->dev, "read led usercode reg %d value= %x\n", i, reading);

		value |= reading << (24 - 8 * i);
	}

	return sprintf(buf, "%X\n", value);
}

static ssize_t set_led_decode(struct device *dev,
			    struct device_attribute *devattr,
			    const char *buf,
			    size_t count)
{
	struct cpld_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->cpld_client;
	s32 value;
	long enable;

	if (kstrtol(buf, 0, &enable))
		return -EINVAL;

	if ((enable != 1) && (enable != 0))
		return -EINVAL;

//	mutex_lock(&data->lock);
	value = i2c_smbus_read_byte_data(client, CPLD_DECODER_OFFSET);
	if (value < 0)
		return -ENODEV;

	dev_dbg(&client->dev, "read led decode value= %x\n", value);

	value |= CPLD_DECODER_MASK;
	if (enable)
		value &= ~CPLD_DECODER_MASK;

	dev_dbg(&client->dev, "write led decode value= %x\n", value);

	i2c_smbus_write_byte_data(client, CPLD_DECODER_OFFSET, (u8)value);
//	mutex_unlock(&data->lock);

	return count;
}

static DEVICE_ATTR(led_decode, S_IWUSR | S_IRUGO, get_led_decode, set_led_decode);
static DEVICE_ATTR(usercode, S_IRUGO, get_usercode, NULL);

static const struct attribute *led_attrs[] = {
	&dev_attr_usercode.attr,
	&dev_attr_led_decode.attr,
	NULL,
};

static const struct attribute_group led_attr_group = {
	.attrs = (struct attribute **) led_attrs,
};

static int cpld_led_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct cpld_led_data *data;
	struct cpld_data *led_data;
	struct device *port_dev;
	int nr, err;

	if (!cpld_class)
	{
		cpld_class = class_create(THIS_MODULE, "cpld-led");
		if (IS_ERR(cpld_class)) {
			pr_err("couldn't create sysfs class\n");
			return PTR_ERR(cpld_class);
		}
	}

	data = devm_kzalloc(&client->dev, sizeof(struct cpld_led_data),
			    GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* Test */
	nr = ida_simple_get(&cpld_led_ida, 1, 99, GFP_KERNEL);
	if (nr < 0)
		goto err_out;

	led_data = kzalloc(sizeof(struct cpld_led_data), GFP_KERNEL);

	port_dev = device_create(cpld_class, &client->dev, MKDEV(0,0), led_data, CPLD_LED_ID_FORMAT, nr);
	if (IS_ERR(port_dev)) {
		err = PTR_ERR(port_dev);
		// printk("err_status\n");
	}

	data->port_dev = port_dev;
	data->cpld_data = led_data;

	dev_info(&client->dev, "Register CPLDLED %d\n", nr);

	sprintf(led_data->name, "LED%d-data", nr);
	led_data->cpld_id = nr;
	dev_set_drvdata(port_dev, led_data);
	port_dev->init_name = led_data->name;
	led_data->cpld_client = client;

	err = sysfs_create_group(&port_dev->kobj, &led_attr_group);
	// if (status)	printk("err status\n");
	/* end */

	i2c_set_clientdata(client, data);
	mutex_init(&data->lock);

	dev_info(&client->dev, "%s device found\n", client->name);


	return 0;

err_out:
	return nr;
}

#if 0
/* FIXME: for older kernel doesn't with idr_is_empty function, implement here */
static int idr_has_entry(int id, void *p, void *data)
{
	return 1;
}

static bool cpld_idr_is_empty(struct idr *idp)
{
	return !idr_for_each(idp, idr_has_entry, NULL);
}
#endif

static int cpld_led_remove(struct i2c_client *client)
{
	struct cpld_led_data *data = i2c_get_clientdata(client);

	dev_info(data->port_dev, "Remove CPLDLED-%d\n", data->cpld_data->cpld_id);
	device_unregister(data->port_dev);
	ida_simple_remove(&cpld_led_ida, data->cpld_data->cpld_id);
	kfree(data->cpld_data);

	if (ida_is_empty(&cpld_led_ida))
	{
		class_destroy(cpld_class);
		cpld_class = NULL;
	}

	return 0;
}

module_i2c_driver(cpld_led_driver);

MODULE_AUTHOR("Luffy Cheng <luffy.cheng@quantatw.com>");
MODULE_AUTHOR("Roger Chang <Roger.Chang@quantatw.com>");
MODULE_DESCRIPTION("Quanta Switch LED CPLD driver");
MODULE_LICENSE("GPL");
