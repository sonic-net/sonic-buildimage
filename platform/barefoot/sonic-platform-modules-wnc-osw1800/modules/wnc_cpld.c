/*
 * wnc_cpld.c - A driver for control wnc_cpld base on lm75.c
 *
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
 * Copyright (c) 2017 WNC <wnc@wnc.com.tw>
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
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x31, 0x32, I2C_CLIENT_END };

/* Size of EEPROM in bytes */
#define CPLD_SIZE 3

/* Each client has this additional data */
struct cpld_data {
	struct mutex		update_lock;
	char			valid;		 /* !=0 if registers are valid */
	unsigned long		last_updated;    /* In jiffies */
	u8			data[CPLD_SIZE]; /* Register value */
};

static ssize_t show_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);
	data->data[0] = i2c_smbus_read_byte_data(client, attr->index);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%02x\n", data->data[0]);
}

static ssize_t set_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 temp;
	int error;

	error = kstrtou8(buf, 10, &temp);
	if (error)
		return error;

	mutex_lock(&data->update_lock);
	i2c_smbus_write_byte_data(client, attr->index, temp);
	mutex_unlock(&data->update_lock);

	return count;
}

static SENSOR_DEVICE_ATTR(reset_control, S_IWUSR | S_IRUGO, show_value, set_value, 2);
static SENSOR_DEVICE_ATTR(sfp_mod_abs1, S_IRUGO, show_value, NULL, 3);
static SENSOR_DEVICE_ATTR(sfp_mod_abs2, S_IRUGO, show_value, NULL, 4);
static SENSOR_DEVICE_ATTR(sfp_mod_abs3, S_IRUGO, show_value, NULL, 5);
static SENSOR_DEVICE_ATTR(sfp_mod_abs4, S_IRUGO, show_value, NULL, 6);
static SENSOR_DEVICE_ATTR(qsfp_modprs, S_IRUGO, show_value, NULL, 22);
static SENSOR_DEVICE_ATTR(qsfp_lpmode, S_IWUSR | S_IRUGO, show_value, set_value, 24);

static struct attribute *cpld2_attributes[] = {
	&sensor_dev_attr_reset_control.dev_attr.attr,
	&sensor_dev_attr_sfp_mod_abs1.dev_attr.attr,
	&sensor_dev_attr_sfp_mod_abs2.dev_attr.attr,
	&sensor_dev_attr_sfp_mod_abs3.dev_attr.attr,
	&sensor_dev_attr_qsfp_modprs.dev_attr.attr,
	&sensor_dev_attr_qsfp_lpmode.dev_attr.attr,
	NULL
};

static struct attribute *cpld1_attributes[] = {
	&sensor_dev_attr_sfp_mod_abs1.dev_attr.attr,
	&sensor_dev_attr_sfp_mod_abs2.dev_attr.attr,
	&sensor_dev_attr_sfp_mod_abs3.dev_attr.attr,
	&sensor_dev_attr_sfp_mod_abs4.dev_attr.attr,
	NULL
};

static const struct attribute_group cpld2_group = {
	.attrs = cpld2_attributes,
};

static const struct attribute_group cpld1_group = {
	.attrs = cpld1_attributes,
};

/* Return 0 if detection is successful, -ENODEV otherwise */
static int cpld_detect(struct i2c_client *new_client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = new_client->adapter;
	int conf;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA |
				     I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;

	/* Unused bits */
	conf = i2c_smbus_read_byte_data(new_client, 0);
	if (!conf)
		return -ENODEV;

	return 0;
}


static int cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cpld_data *data;
	int status;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;
#if 1
	data = devm_kzalloc(&client->dev, sizeof(struct cpld_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);
#endif

	if (client->addr == 49) /* 0x31 */
		/* Register sysfs hooks */
		status = sysfs_create_group(&client->dev.kobj, &cpld1_group);
	else if (client->addr == 50) /* 0x32 */
		/* Register sysfs hooks */
		status = sysfs_create_group(&client->dev.kobj, &cpld2_group);
	else
		status = 1;

	if (status)
		return status;

	dev_info(&client->dev, "cpld 0x%x found\n", client->addr);

	return 0;
}

static int cpld_remove(struct i2c_client *client)
{
	if (client->addr == 49) /* 0x31 */
		sysfs_remove_group(&client->dev.kobj, &cpld1_group);
	else if (client->addr == 50) /* 0x32 */
		sysfs_remove_group(&client->dev.kobj, &cpld2_group);
	else
		return 1;

	return 0;
}

static const struct i2c_device_id cpld_id[] = {
	{ "wnc_cpld", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "wnc_cpld",
	},
	.probe		= cpld_probe,
	.remove		= cpld_remove,
	.id_table	= cpld_id,
	.detect		= cpld_detect,
	.address_list	= normal_i2c,
};

module_i2c_driver(cpld_driver);

MODULE_AUTHOR("WNC <wnc@wnc.com.tw>");
MODULE_DESCRIPTION("WNC CPLD driver");
MODULE_LICENSE("GPL");
