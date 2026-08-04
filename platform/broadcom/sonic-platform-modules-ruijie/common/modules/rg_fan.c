/*
 * rg_fan.c - A driver for control rg_fan base on rg_fan.c
 *
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
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

#define FAN_SIZE        (256)
#define SYS_FAN_BUF_LEN (64)

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

#define DBG_ERROR(fmt, arg...)                                                 \
	do {                                                                   \
		if (debuglevel > DBG_START) {                                  \
			printk(KERN_ERR "[ERROR]:<%s, %d>:" fmt, __FUNCTION__, \
			       __LINE__, ##arg);                               \
		}                                                              \
	} while (0)

extern s32 platform_i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command);
extern s32 platform_i2c_smbus_read_i2c_block_data(
		const struct i2c_client *client, u8 command, u8 length, u8 *values);
extern s32 platform_i2c_smbus_read_word_data(const struct i2c_client *client, u8 command);

typedef enum dfd_dev_info_type_e {
	DFD_DEV_INFO_TYPE_MAC      = 1,
	DFD_DEV_INFO_TYPE_NAME     = 2,
	DFD_DEV_INFO_TYPE_SN       = 3,
	DFD_DEV_INFO_TYPE_PWR_CONS = 4,
	DFD_DEV_INFO_TYPE_HW_INFO  = 5,
	DFD_DEV_INFO_TYPE_DEV_TYPE = 6,
} dfd_dev_tlv_type_t;

typedef struct dfd_dev_head_info_s {
	uint8_t ver;     /* define E2PROM version，default is 0x01  */
	uint8_t flag;    /* flag is 0x7E in new version E2PROM */
	uint8_t hw_ver;  /* consists of main version and revise version */
	uint8_t type;    /* HW type */
	int16_t tlv_len; /* 16 bits */
} dfd_dev_head_info_t;

typedef struct dfd_dev_tlv_info_s {
	uint8_t type;
	uint8_t len;
	uint8_t data[0];
} dfd_dev_tlv_info_t;

struct fan_data {
	struct   i2c_client *client;
	struct   mutex update_lock;
	char     valid;                /* !=0 if registers are valid */
	unsigned long last_updated[8]; /* In jiffies */
	u8       data[FAN_SIZE];       /* Register value */
};

static ssize_t show_fan_sysfs_tlv_value(struct device *dev,
					struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct fan_data *data = i2c_get_clientdata(client);

	dfd_dev_head_info_t info;
	uint8_t tmp_tlv_len[sizeof(uint16_t)];
	uint8_t *tlv_data;
	dfd_dev_tlv_info_t *tlv;
	int type;
	int buf_len = SYS_FAN_BUF_LEN - 1;
	u8 sysfs_buf[SYS_FAN_BUF_LEN];
	int i;
	int ret = 0;

	mutex_lock(&data->update_lock);
	memset(sysfs_buf, 0, SYS_FAN_BUF_LEN);
	ret = platform_i2c_smbus_read_i2c_block_data(
		client, 0, sizeof(dfd_dev_head_info_t), (uint8_t *)&info);
	if (ret != sizeof(dfd_dev_head_info_t)) {
		DBG_ERROR("fan maybe not set mac or not present0");
		goto exit;
	}

	/* transform TLV_LEN */
	memcpy(tmp_tlv_len, (uint8_t *)&info.tlv_len, sizeof(int16_t));
	info.tlv_len = (tmp_tlv_len[0] << 8) + tmp_tlv_len[1];

	if ((info.tlv_len <= 0) || (info.tlv_len > 0xFF)) {
		DBG_ERROR("fan maybe not set mac or not present1");
		goto exit;
	}

	type = attr->index;
	tlv_data = (uint8_t *)kmalloc(info.tlv_len, GFP_KERNEL);
	memset(tlv_data, 0, info.tlv_len);

	if (i2c_check_functionality(client->adapter,
				    I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
		for (i = 0; i < info.tlv_len; i += 32)
			if (platform_i2c_smbus_read_i2c_block_data(client,
						sizeof(dfd_dev_head_info_t) + i,
						32, tlv_data + i) != 32)
				break;
	}

	DBG_DEBUG("TLV Len:%d\n", (int)sizeof(dfd_dev_tlv_info_t));
	for (tlv = (dfd_dev_tlv_info_t *)tlv_data;
	     (ulong)tlv < (ulong)tlv_data + info.tlv_len;) {
		DBG_DEBUG(
			"tlv: %p, tlv->type: 0x%x, tlv->len: 0x%x info->tlv_len: 0x%x\n",
			tlv, tlv->type, tlv->len, info.tlv_len);
		if (tlv->type == type && buf_len >= tlv->len) {
			memcpy((uint8_t *)sysfs_buf, (uint8_t *)tlv->data,
			       tlv->len);
			buf_len = (uint32_t)tlv->len;
			break;
		}
		tlv = (dfd_dev_tlv_info_t *)((uint8_t *)tlv +
					     sizeof(dfd_dev_tlv_info_t) +
					     tlv->len);
	}

	kfree(tlv_data);
	DBG_DEBUG("value: %s \n", sysfs_buf);
exit:
	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%s\n", sysfs_buf);
}

static ssize_t show_fan_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct fan_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int i;

	mutex_lock(&data->update_lock);

	if (i2c_check_functionality(client->adapter,
				    I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
		for (i = 0; i < FAN_SIZE; i += 32) {
			if (platform_i2c_smbus_read_i2c_block_data(
				    client, i, 32, data->data + i) != 32)
				goto exit;
		}
	} else {
		for (i = 0; i < FAN_SIZE; i += 2) {
			int word = platform_i2c_smbus_read_word_data(client, i);
			if (word < 0)
				goto exit;
			data->data[i] = word & 0xff;
			data->data[i + 1] = word >> 8;
		}
	}
	memcpy(buf, &data->data[0], FAN_SIZE);
exit:
	mutex_unlock(&data->update_lock);
	return FAN_SIZE;
}

static SENSOR_DEVICE_ATTR(fan_hw_version, S_IRUGO, show_fan_sysfs_tlv_value, NULL, DFD_DEV_INFO_TYPE_HW_INFO);
static SENSOR_DEVICE_ATTR(fan_sn, S_IRUGO, show_fan_sysfs_tlv_value, NULL, DFD_DEV_INFO_TYPE_SN);
static SENSOR_DEVICE_ATTR(fan_type, S_IRUGO, show_fan_sysfs_tlv_value, NULL, DFD_DEV_INFO_TYPE_NAME);
static SENSOR_DEVICE_ATTR(fan, S_IRUGO, show_fan_value, NULL, 0);

static struct attribute *fan_sysfs_attrs[] = {
	&sensor_dev_attr_fan_hw_version.dev_attr.attr,
	&sensor_dev_attr_fan_sn.dev_attr.attr,
	&sensor_dev_attr_fan_type.dev_attr.attr,
	&sensor_dev_attr_fan.dev_attr.attr,
	NULL
};

static const struct attribute_group fan_sysfs_group = {
	.attrs = fan_sysfs_attrs,
};

static int fan_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct fan_data *data;
	int status;

	status = -1;
	DBG_DEBUG("fan_probe(0x%02x)\n", client->addr);
	data = devm_kzalloc(&client->dev, sizeof(struct fan_data), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	data->client = client;
	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	status = sysfs_create_group(&client->dev.kobj, &fan_sysfs_group);
	if (status != 0) {
		DBG_ERROR(" sysfs_create_group err\n");
		return status;
	}
	return 0;
}

static int fan_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &fan_sysfs_group);
	return 0;
}

static const struct i2c_device_id fan_id[] = { { "rg_fan", 0 }, {} };
MODULE_DEVICE_TABLE(i2c, fan_id);

static struct i2c_driver rg_fan_driver = {
	.driver   = {
		.name = "rg_fan",
	},
	.probe    = fan_probe,
	.remove   = fan_remove,
	.id_table = fan_id,
};

module_i2c_driver(rg_fan_driver);
MODULE_AUTHOR("sonic_rd <sonic_rd@ruijie.com.cn>");
MODULE_DESCRIPTION("ruijie fan driver");
MODULE_LICENSE("GPL");
