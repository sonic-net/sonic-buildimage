/*
 * rg_cpld.c - A driver for pmbus psu
 *
 * Copyright (c) 2019  <sonic_rd@ruijie.com.cn>
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

#define MAGIC_PSU_RATE        (0xA7)
#define MAGIC_PSU_OUT_CURRENT (0x8C)
#define MAGIC_PSU_OUT_VOLTAGE (0x8B)
#define MAGIC_PSU_IN_VOLTAGE  (0x88)
#define MAGIC_PSU_IN_CURRENT  (0x89)
#define MAGIC_PSU_TEMP        (0x8D)
#define MAGIC_PSU_TYPE        (0x25)
#define MAGIC_PSU_SN          (0x38)
#define MAGIC_PSU_HW          (0x35)
#define PSU_SIZE              (256)

typedef enum {
	DBG_START,
	DBG_VERBOSE,
	DBG_KEY,
	DBG_WARN,
	DBG_ERROR,
	DBG_END,
} dbg_level_t;

static int debuglevel = 0;

#define DBG_DEBUG(fmt, arg...)                                                 \
	do {                                                                   \
		if (debuglevel > DBG_START && debuglevel < DBG_ERROR) {        \
			printk(KERN_INFO "[DEBUG]:<%s, %d>:" fmt,              \
			       __FUNCTION__, __LINE__, ##arg);                 \
		} else if (debuglevel >= DBG_ERROR) {                          \
			printk(KERN_ERR "[DEBUG]:<%s, %d>:" fmt, __FUNCTION__, \
			       __LINE__, ##arg);                               \
		} else {                                                       \
		}                                                              \
	} while (0)

#define DBG_INFO(fmt, arg...)                                                  \
	do {                                                                   \
		if (debuglevel > DBG_KEY) {                                    \
			printk(KERN_INFO "[INFO]:<%s, %d>:" fmt, __FUNCTION__, \
			       __LINE__, ##arg);                               \
		}                                                              \
	} while (0)

#define DBG_ERROR(fmt, arg...)                                                 \
	do {                                                                   \
		if (debuglevel > DBG_START) {                                  \
			printk(KERN_ERR "[ERROR]:<%s, %d>:" fmt, __FUNCTION__, \
			       __LINE__, ##arg);                               \
		}                                                              \
	} while (0)

static const unsigned short rg_i2c_psu[] = { 0x50, 0x53, 0x58, 0x5b, I2C_CLIENT_END };

extern s32 platform_i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command);
extern s32 platform_i2c_smbus_read_i2c_block_data(const struct i2c_client *client,
                u8 command, u8 length, u8 *values);

struct psu_data {
	struct   i2c_client *client;
	struct   device *hwmon_dev;
	struct   mutex update_lock;
	char     valid;             /* !=0 if registers are valid */
	unsigned long last_updated; /* In jiffies */
	u8       data[PSU_SIZE];    /* Register value */
};

static ssize_t show_psu_sysfs_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_sysfs_15_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_psu_value(struct device *dev, struct device_attribute *da, char *buf);

static SENSOR_DEVICE_ATTR(psu_rate, S_IRUGO, show_psu_sysfs_value, NULL, MAGIC_PSU_RATE);
static SENSOR_DEVICE_ATTR(psu_out_current, S_IRUGO, show_psu_sysfs_value, NULL, MAGIC_PSU_OUT_CURRENT);
static SENSOR_DEVICE_ATTR(psu_out_voltage, S_IRUGO, show_psu_sysfs_value, NULL, MAGIC_PSU_OUT_VOLTAGE);
static SENSOR_DEVICE_ATTR(psu_in_voltage, S_IRUGO, show_psu_sysfs_value, NULL, MAGIC_PSU_IN_VOLTAGE);
static SENSOR_DEVICE_ATTR(psu_in_current, S_IRUGO, show_psu_sysfs_value, NULL, MAGIC_PSU_IN_CURRENT);
static SENSOR_DEVICE_ATTR(psu_temp, S_IRUGO, show_psu_sysfs_value, NULL, MAGIC_PSU_TEMP);
static SENSOR_DEVICE_ATTR(psu_type, S_IRUGO, show_sysfs_15_value, NULL, MAGIC_PSU_TYPE);
static SENSOR_DEVICE_ATTR(psu_sn, S_IRUGO, show_sysfs_15_value, NULL, MAGIC_PSU_SN);
static SENSOR_DEVICE_ATTR(psu_hw, S_IRUGO, show_psu_value, NULL, MAGIC_PSU_HW);

static struct attribute *psu_pmbus_sysfs_attrs[] = {
	&sensor_dev_attr_psu_rate.dev_attr.attr,
	&sensor_dev_attr_psu_out_current.dev_attr.attr,
	&sensor_dev_attr_psu_out_voltage.dev_attr.attr,
	&sensor_dev_attr_psu_in_voltage.dev_attr.attr,
	&sensor_dev_attr_psu_in_current.dev_attr.attr,
	&sensor_dev_attr_psu_temp.dev_attr.attr,
	NULL
};

static struct attribute *psu_fru_sysfs_attrs[] = {
	&sensor_dev_attr_psu_type.dev_attr.attr,
	&sensor_dev_attr_psu_sn.dev_attr.attr,
	&sensor_dev_attr_psu_hw.dev_attr.attr,
	NULL
};

static const struct attribute_group psu_pmbus_sysfs_attrs_group = {
	.attrs = psu_pmbus_sysfs_attrs,
};

static const struct attribute_group psu_fru_sysfs_attrs_group = {
	.attrs = psu_fru_sysfs_attrs,
};

static ssize_t show_psu_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	int ret;
	char psu_buf[PSU_SIZE];
	memset(psu_buf, 0, PSU_SIZE);
	mutex_lock(&data->update_lock);
	ret = platform_i2c_smbus_read_i2c_block_data(client, attr->index, 2, psu_buf);
	if (ret < 0) {
		DBG_ERROR("Failed to read psu\n");
	}
	DBG_DEBUG("cpld reg pos:0x%x value:0x%02x\n", attr->index, data->data[0]);
	mutex_unlock(&data->update_lock);
	return snprintf(buf, 3, "%s\n", psu_buf);
}

static int linear_to_value(short reg, bool v_out)
{
	short exponent;
	int mantissa;
	long val;

	if (v_out) {
		exponent = -9;
		mantissa = reg;
	} else {
		exponent = reg >> 11;
		mantissa = (((reg & 0x7ff) << 5)) >> 5;
	}
	val = mantissa;
	val = val * 1000L;
	if (exponent >= 0) {
		val <<= exponent;
	} else {
		val >>= -exponent;
	}

	return val;
}

static ssize_t show_psu_sysfs_value(struct device *dev,
				    struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	int ret;
	u8 smbud_buf[PSU_SIZE];
	uint16_t value;
	int result;

	ret = -1;
	memset(smbud_buf, 0, PSU_SIZE);
	mutex_lock(&data->update_lock);
	DBG_DEBUG("ret:%d", ret);
	ret = platform_i2c_smbus_read_i2c_block_data(client, attr->index, 2, smbud_buf);
	if (ret < 0) {
		DBG_ERROR("Failed to read psu \n");
	}
	value = smbud_buf[1];
	value = value << 8;
	value |= smbud_buf[0];

	if (attr->index == 0x8b) {
		result = linear_to_value(value, true);
	} else {
		result = linear_to_value(value, false);
	}
	mutex_unlock(&data->update_lock);
	return snprintf(buf, PSU_SIZE, "%d\n", result);
}

static ssize_t show_sysfs_15_value(struct device *dev,
				   struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	int ret;
	u8 smbud_buf[PSU_SIZE];

	memset(smbud_buf, 0, PSU_SIZE);
	mutex_lock(&data->update_lock);
	ret = platform_i2c_smbus_read_i2c_block_data(client, attr->index, 15, smbud_buf);
	if (ret < 0) {
		DBG_ERROR("Failed to read psu\n");
	}
	mutex_unlock(&data->update_lock);
	return snprintf(buf, PSU_SIZE, "%s\n", smbud_buf);
}

static ssize_t show_sysfs_13_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct psu_data *data = i2c_get_clientdata(client);
    int ret;
    u8  smbud_buf[PSU_SIZE];

    memset(smbud_buf, 0, PSU_SIZE);
    mutex_lock(&data->update_lock);
    ret = platform_i2c_smbus_read_i2c_block_data(client, attr->index, 13, smbud_buf);
    if (ret < 0) {
        DBG_ERROR("Failed to read psu \n");
    }
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PSU_SIZE, "%s\n", smbud_buf);
}

static int psu_detect(struct i2c_client *new_client,
		      struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = new_client->adapter;
	int conf;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA |
						      I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;
	conf = platform_i2c_smbus_read_byte_data(new_client, 0);
	if (!conf)
		return -ENODEV;

	return 0;
}

static int psu_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct psu_data *data;
	int status;

	status = -1;
	data = devm_kzalloc(&client->dev, sizeof(struct psu_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	switch (client->addr) {
	case 0x50:
	case 0x53:
		status = sysfs_create_group(&client->dev.kobj,
					    &psu_fru_sysfs_attrs_group);
		if (status != 0) {
			DBG_ERROR("%s %d sysfs_create_group err\n", __func__, __LINE__);
		}
		break;
	case 0x58:
	case 0x5b:
		status = sysfs_create_group(&client->dev.kobj,
					    &psu_pmbus_sysfs_attrs_group);
		if (status != 0) {
			DBG_ERROR("%s %d sysfs_create_group err\n", __func__, __LINE__);
			break;
		}
		break;
	default:
		break;
	}

	return status;
}

static int psu_remove(struct i2c_client *client)
{
	switch (client->addr) {
	case 0x50:
	case 0x53:
		sysfs_remove_group(&client->dev.kobj, &psu_fru_sysfs_attrs_group);
		break;
	case 0x58:
	case 0x5b:
		sysfs_remove_group(&client->dev.kobj, &psu_pmbus_sysfs_attrs_group);
		break;
	default:
		break;
	}
	return 0;
}

static const struct i2c_device_id psu_id[] = {
	{ "rg_psu", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, psu_id);

static struct i2c_driver rg_psu_driver = {
	.class        = I2C_CLASS_HWMON,
	.driver       = {
		.name = "rg_psu",
	},
	.probe        = psu_probe,
	.remove       = psu_remove,
	.id_table     = psu_id,
	.detect       = psu_detect,
	.address_list = rg_i2c_psu,
};

module_i2c_driver(rg_psu_driver);

MODULE_AUTHOR("sonic_rd <sonic_rd@ruijie.com.cn>");
MODULE_DESCRIPTION("ruijie pmbus psu driver");
MODULE_LICENSE("GPL");
