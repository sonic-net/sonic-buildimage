/**
 * A simple hwmon driver for the tps546b24.
 * The driver implements custom functions to handle the relative VOUT_MODE feature of the tps546b24.
 *
 * Copyright (c) 2026, Celestica Inc.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/of.h>
#include "pmbus.h"

struct tps546b24_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	s8 vout_exponent;
	bool is_vout_relative;
	u16 vout_command;
	bool vout_command_valid;
	unsigned long vout_command_last_updated;
};

static int tps546b24_read_word(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_word_data(client, reg);
}

static s64 linear11_to_value(u16 linear11)
{
	s16 mantissa;
	s8 exponent;
	s64 val;

	mantissa = ((s16)(linear11 << 5)) >> 5;
	exponent = ((s16)linear11) >> 11;

	val = (s64)mantissa;
	val *= 1000; // Multiply by 1000 to retain precision for fractional parts
	if (exponent >= 0)
		val <<= exponent;
	else
		val >>= -exponent;

	return val;
}

static s64 linear16_to_value(u16 linear16, s8 exponent)
{
	s64 val;
	val = (s64)linear16;
	val *= 1000; // Multiply by 1000 to retain precision for fractional parts
	if (exponent >= 0)
		val <<= exponent;
	else
		val >>= -exponent;
	return val;
}

static int update_vout_command(struct i2c_client *client,
			       struct tps546b24_data *data)
{
	int ret;

	mutex_lock(&data->update_lock);
	/* Update VOUT_COMMAND if the cache is older than 1 second */
	if (time_after(jiffies, data->vout_command_last_updated + HZ) ||
	    !data->vout_command_valid) {
		ret = tps546b24_read_word(client, PMBUS_VOUT_COMMAND);
		if (ret < 0) {
			data->vout_command_valid = false;
			mutex_unlock(&data->update_lock);
			return ret;
		}
		data->vout_command = (u16)ret;
		data->vout_command_last_updated = jiffies;
		data->vout_command_valid = true;
	}

	mutex_unlock(&data->update_lock);
	return 0;
}

static ssize_t tps546b24_value_show(struct device *dev,
				    struct device_attribute *devattr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev->parent);
	struct tps546b24_data *data = i2c_get_clientdata(client);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	int val;
	s64 sval;

	val = tps546b24_read_word(client, attr->index);
	if (val < 0)
		return val;

	switch (attr->index) {
	case PMBUS_READ_VOUT:
		sval = linear16_to_value((u16)val, data->vout_exponent);
		break;
	case PMBUS_VOUT_OV_FAULT_LIMIT:
	case PMBUS_VOUT_UV_FAULT_LIMIT:
	case PMBUS_VOUT_OV_WARN_LIMIT:
	case PMBUS_VOUT_UV_WARN_LIMIT:
		sval = linear16_to_value((u16)val, data->vout_exponent);
		if (data->is_vout_relative) {
			int ret = update_vout_command(client, data);
			if (ret < 0)
				return ret;
			sval *= linear16_to_value(data->vout_command,
						  data->vout_exponent);
			sval /= 1000; // Adjust back after multiplication
		}
		break;
	case PMBUS_READ_VIN:
	case PMBUS_VIN_OV_FAULT_LIMIT:
	case PMBUS_READ_IOUT:
	case PMBUS_IOUT_OC_FAULT_LIMIT:
	case PMBUS_IOUT_OC_WARN_LIMIT:
	case PMBUS_READ_TEMPERATURE_1:
	case PMBUS_OT_FAULT_LIMIT:
	case PMBUS_OT_WARN_LIMIT:
		sval = linear11_to_value(val);
		break;
	default:
		sval = val;
		break;
	}

	return sprintf(buf, "%lld", sval);
}
static ssize_t tps546b24_value_store(struct device *dev,
				     struct device_attribute *devattr,
				     const char *buf, size_t count)
{
	/* write operation is not supported currently */
	return -ENOTSUPP;
}

static SENSOR_DEVICE_ATTR_RO(in1_input, tps546b24_value, PMBUS_READ_VIN);
static SENSOR_DEVICE_ATTR_RW(in1_min, tps546b24_value, PMBUS_VIN_UV_WARN_LIMIT);
static SENSOR_DEVICE_ATTR_RW(in1_crit, tps546b24_value,
			     PMBUS_VIN_OV_FAULT_LIMIT);

static SENSOR_DEVICE_ATTR_RO(in3_input, tps546b24_value, PMBUS_READ_VOUT);
static SENSOR_DEVICE_ATTR_RW(in3_max, tps546b24_value,
			     PMBUS_VOUT_OV_WARN_LIMIT);
static SENSOR_DEVICE_ATTR_RW(in3_min, tps546b24_value,
			     PMBUS_VOUT_UV_WARN_LIMIT);
static SENSOR_DEVICE_ATTR_RW(in3_crit, tps546b24_value,
			     PMBUS_VOUT_OV_FAULT_LIMIT);
static SENSOR_DEVICE_ATTR_RW(in3_lcrit, tps546b24_value,
			     PMBUS_VOUT_UV_FAULT_LIMIT);

static SENSOR_DEVICE_ATTR_RO(curr1_input, tps546b24_value, PMBUS_READ_IOUT);
static SENSOR_DEVICE_ATTR_RW(curr1_max, tps546b24_value,
			     PMBUS_IOUT_OC_FAULT_LIMIT);
static SENSOR_DEVICE_ATTR_RW(curr1_crit, tps546b24_value,
			     PMBUS_IOUT_OC_WARN_LIMIT);

static SENSOR_DEVICE_ATTR_RO(temp1_input, tps546b24_value,
			     PMBUS_READ_TEMPERATURE_1);
static SENSOR_DEVICE_ATTR_RW(temp1_max, tps546b24_value, PMBUS_OT_FAULT_LIMIT);
static SENSOR_DEVICE_ATTR_RW(temp1_crit, tps546b24_value, PMBUS_OT_WARN_LIMIT);

static struct attribute *tps546b24_attrs[] = {
	/* Vin attributes */
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in1_min.dev_attr.attr,
	&sensor_dev_attr_in1_crit.dev_attr.attr,

	/* Vout attributes */
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in3_max.dev_attr.attr,
	&sensor_dev_attr_in3_min.dev_attr.attr,
	&sensor_dev_attr_in3_crit.dev_attr.attr,
	&sensor_dev_attr_in3_lcrit.dev_attr.attr,

	/* Iout attributes */
	&sensor_dev_attr_curr1_input.dev_attr.attr,
	&sensor_dev_attr_curr1_max.dev_attr.attr,
	&sensor_dev_attr_curr1_crit.dev_attr.attr,

	/* Temperature attributes */
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp1_crit.dev_attr.attr, NULL
};

ATTRIBUTE_GROUPS(tps546b24);

static int tps546b24_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct tps546b24_data *data;
	int vout_mode, mode;
	int ret = 0;

	vout_mode = i2c_smbus_read_byte_data(client, PMBUS_VOUT_MODE);
	if (vout_mode < 0) {
		dev_err(dev, "Failed to read VOUT_MODE (%d)\n", vout_mode);
		ret = vout_mode;
		goto exit;
	}

	/* strip the relative bit */
	if (vout_mode & 0x80) {
		dev_info(dev, "VOUT_MODE is in relative format (val=0x%02x).",
			 vout_mode);
	}

	mode = (vout_mode >> 5) & 0x03;

	if (mode != 0) {
		dev_warn(dev, "Only linear VOUT_MODE is supported");
		ret = -ENOTSUPP;
		goto exit;
	}

	data = devm_kzalloc(dev, sizeof(struct tps546b24_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);

	mutex_init(&data->update_lock);
	data->vout_exponent = ((s8)(vout_mode << 3)) >> 3;
	data->is_vout_relative = !!(vout_mode & 0x80);
	data->hwmon_dev = hwmon_device_register_with_groups(
		dev, "tps546b24", NULL, tps546b24_groups);
	if (IS_ERR(data->hwmon_dev))
		return PTR_ERR(data->hwmon_dev);

exit:
	return ret;
}

static void tps546b24_remove(struct i2c_client *client)
{
	struct tps546b24_data *data = i2c_get_clientdata(client);
	hwmon_device_unregister(data->hwmon_dev);
}

static const struct i2c_device_id tps546b24_id[] = { 
	{ "tps546b24", 0 },
	{} 
};
MODULE_DEVICE_TABLE(i2c, tps546b24_id);

static const struct of_device_id tps546b24_of_match[] = {
	{ .compatible = "ti,tps546b24" },
	{}
};
MODULE_DEVICE_TABLE(of, tps546b24_of_match);

static struct i2c_driver tps546b24_driver = {
	.driver = {
		.name	= "tps546b24",
		.of_match_table = of_match_ptr(tps546b24_of_match),
	},
	.probe		= tps546b24_probe,
	.remove		= tps546b24_remove,
	.id_table	= tps546b24_id,
};

module_i2c_driver(tps546b24_driver);

MODULE_AUTHOR("Celstica Inc.");
MODULE_DESCRIPTION("Custom hwmon driver for tps546b24");
MODULE_LICENSE("GPL v2");
