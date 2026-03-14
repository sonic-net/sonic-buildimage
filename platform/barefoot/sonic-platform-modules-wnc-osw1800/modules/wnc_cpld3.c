/*
 * wnc_cpld.c - A driver for control wnc_cpld3 base on lm75.c
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
static const unsigned short normal_i2c[] = { 0x33, I2C_CLIENT_END };

/* Size of EEPROM in bytes */
#define CPLD_SIZE 3

/* Each client has this additional data */
struct cpld_data {
	struct i2c_client 	*client;
	struct mutex		update_lock;
	char			valid;		 /* !=0 if registers are valid */
	unsigned long		last_updated;    /* In jiffies */
	u8			data[CPLD_SIZE]; /* Register value */
};

static ssize_t show_hwmon_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct cpld_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int index = to_sensor_dev_attr_2(da)->index;

	mutex_lock(&data->update_lock);
	data->data[0] = i2c_smbus_read_byte_data(client, index);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", data->data[0]);
}

static ssize_t show_sysfs_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);
	data->data[0] = i2c_smbus_read_byte_data(client, attr->index);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%02x\n", data->data[0]);
}

static ssize_t set_hwmon_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	struct cpld_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int index = to_sensor_dev_attr_2(da)->index;
	u8 temp;
	int error;

	error = kstrtou8(buf, 10, &temp);
	if (error)
		return error;

	mutex_lock(&data->update_lock);
	i2c_smbus_write_byte_data(client, index, temp);
	mutex_unlock(&data->update_lock);

	return count;
}

static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_hwmon_value, NULL, 6);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, show_hwmon_value, NULL, 7);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO, show_hwmon_value, NULL, 8);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO, show_hwmon_value, NULL, 9);
static SENSOR_DEVICE_ATTR(fan5_input, S_IRUGO, show_hwmon_value, NULL, 10);
static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR | S_IRUGO, show_hwmon_value, set_hwmon_value, 6);
static SENSOR_DEVICE_ATTR(pwm2, S_IWUSR | S_IRUGO, show_hwmon_value, set_hwmon_value, 7);
static SENSOR_DEVICE_ATTR(pwm3, S_IWUSR | S_IRUGO, show_hwmon_value, set_hwmon_value, 8);
static SENSOR_DEVICE_ATTR(pwm4, S_IWUSR | S_IRUGO, show_hwmon_value, set_hwmon_value, 9);
static SENSOR_DEVICE_ATTR(pwm5, S_IWUSR | S_IRUGO, show_hwmon_value, set_hwmon_value, 10);

static struct attribute *cpld3_hwmon_attrs[] = {
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	&sensor_dev_attr_fan3_input.dev_attr.attr,
	&sensor_dev_attr_fan4_input.dev_attr.attr,
	&sensor_dev_attr_fan5_input.dev_attr.attr,
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_pwm2.dev_attr.attr,
	&sensor_dev_attr_pwm3.dev_attr.attr,
	&sensor_dev_attr_pwm4.dev_attr.attr,
	&sensor_dev_attr_pwm5.dev_attr.attr,
	NULL
};

ATTRIBUTE_GROUPS(cpld3_hwmon);

static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, show_sysfs_value, NULL, 1);
static SENSOR_DEVICE_ATTR(power_good, S_IRUGO, show_sysfs_value, NULL, 4);

static struct attribute *cpld3_sysfs_attrs[] = {
	&sensor_dev_attr_cpld_version.dev_attr.attr,
	&sensor_dev_attr_power_good.dev_attr.attr,
	NULL
};

static const struct attribute_group cpld3_sysfs_group = {
	.attrs = cpld3_sysfs_attrs,
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
	struct device *hwmon_dev;
	struct cpld_data *data;
	int status;

	data = devm_kzalloc(&client->dev, sizeof(struct cpld_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	status = sysfs_create_group(&client->dev.kobj, &cpld3_sysfs_group);
	hwmon_dev = devm_hwmon_device_register_with_groups(&client->dev, client->name, data, cpld3_hwmon_groups);

	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static int cpld_remove(struct i2c_client *client)
{
	devm_hwmon_device_unregister(&client->dev);
	sysfs_remove_group(&client->dev.kobj, &cpld3_sysfs_group);
	return 0;
}

static const struct i2c_device_id cpld_id[] = {
	{ "wnc_cpld3", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "wnc_cpld3",
	},
	.probe		= cpld_probe,
	.remove		= cpld_remove,
	.id_table	= cpld_id,
	.detect		= cpld_detect,
	.address_list	= normal_i2c,
};

module_i2c_driver(cpld_driver);

MODULE_AUTHOR("WNC <wnc@wnc.com.tw>");
MODULE_DESCRIPTION("WNC CPLD3 driver");
MODULE_LICENSE("GPL");
