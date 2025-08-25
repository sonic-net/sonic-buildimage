// SPDX-License-Identifier: GPL-2.0
/*
 * An hwmon driver for the Delta DPS-850AB-4 Power Module
 *
 * Copyright (C) 2017 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
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
#include <linux/i2c.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/version.h>

#define I2C_RW_RETRY_COUNT		10
#define I2C_RW_RETRY_INTERVAL	60 /* ms */

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

enum chips {
	DPS850
};

/* Each client has this additional data
 */
struct dps850_data {
	struct mutex		update_lock;
	char				valid;		 /* !=0 if registers are valid */
	unsigned long	   last_updated;   /* In jiffies */
	u8	 chip;			/* chip id */
	u8   vout_mode;	 	/* Register value */
	u16  v_in;		  	/* Register value */
	u16  v_out;		 	/* Register value */
	u16  i_in;		  	/* Register value */
	u16  i_out;		 	/* Register value */
	u16  p_in;		  	/* Register value */
	u16  p_out;		 	/* Register value */
	u16  temp_input[3]; /* Register value */
	u16  fan_speed;		/* Register value */
	u8   mfr_model[16]; /* Register value */
	u8   mfr_serial[16]; /* Register value */
};

static ssize_t show_linear(struct device *dev, struct device_attribute *da,
			 char *buf);
static ssize_t show_vout_by_mode(struct device *dev, struct device_attribute *da,
			 char *buf);
static ssize_t show_ascii(struct device *dev, struct device_attribute *da,
			 char *buf);
static struct dps850_data *dps850_update_device(struct device *dev);

enum dps850_sysfs_attributes {
	PSU_V_IN,
	PSU_V_OUT,
	PSU_I_IN,
	PSU_I_OUT,
	PSU_P_IN,
	PSU_P_OUT,
	PSU_TEMP1_INPUT,
	PSU_TEMP2_INPUT,
	PSU_TEMP3_INPUT,
	PSU_FAN1_SPEED,
	PSU_MFR_MODEL,
	PSU_MFR_SERIAL
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(psu_v_in,	S_IRUGO, show_linear,	  NULL, PSU_V_IN);
static SENSOR_DEVICE_ATTR(psu_v_out,S_IRUGO, show_vout_by_mode,NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(psu_i_in,	S_IRUGO, show_linear,	  NULL, PSU_I_IN);
static SENSOR_DEVICE_ATTR(psu_i_out,S_IRUGO, show_linear,	  NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(psu_p_in,	S_IRUGO, show_linear,	  NULL, PSU_P_IN);
static SENSOR_DEVICE_ATTR(psu_p_out,S_IRUGO, show_linear,	  NULL, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(psu_temp1_input, 	S_IRUGO, show_linear,	NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(psu_temp2_input, 	S_IRUGO, show_linear,	NULL, PSU_TEMP2_INPUT);
static SENSOR_DEVICE_ATTR(psu_temp3_input, 	S_IRUGO, show_linear,	NULL, PSU_TEMP3_INPUT);
static SENSOR_DEVICE_ATTR(psu_fan1_speed_rpm, S_IRUGO, show_linear, NULL, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(psu_mfr_model,	S_IRUGO, show_ascii,  NULL, PSU_MFR_MODEL);
static SENSOR_DEVICE_ATTR(psu_mfr_serial,	S_IRUGO, show_ascii, NULL, PSU_MFR_SERIAL);

static struct attribute *dps850_attributes[] = {
	&sensor_dev_attr_psu_v_out.dev_attr.attr,
	&sensor_dev_attr_psu_i_out.dev_attr.attr,
	&sensor_dev_attr_psu_p_out.dev_attr.attr,
	&sensor_dev_attr_psu_v_in.dev_attr.attr,
	&sensor_dev_attr_psu_i_in.dev_attr.attr,
	&sensor_dev_attr_psu_p_in.dev_attr.attr,
	&sensor_dev_attr_psu_temp1_input.dev_attr.attr,
	&sensor_dev_attr_psu_temp2_input.dev_attr.attr,
	&sensor_dev_attr_psu_temp3_input.dev_attr.attr,
	&sensor_dev_attr_psu_fan1_speed_rpm.dev_attr.attr,
	&sensor_dev_attr_psu_mfr_model.dev_attr.attr,
	&sensor_dev_attr_psu_mfr_serial.dev_attr.attr,
	NULL
};

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
	u16  valid_data  = data & mask;
	bool is_negative = valid_data >> (valid_bit - 1);

	return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static ssize_t show_linear(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct dps850_data *data = dps850_update_device(dev);

	u16 value = 0;
	int exponent, mantissa;
	int multiplier = 1000;

	if (!data->valid) {
		return 0;
	}

	switch (attr->index) {
	case PSU_V_IN:
		value = data->v_in;
		break;
	case PSU_I_IN:
		value = data->i_in;
		break;
	case PSU_I_OUT:
		value = data->i_out;
		break;
	case PSU_P_IN:
		value = data->p_in;
		break;
	case PSU_P_OUT:
		value = data->p_out;
		break;
	case PSU_TEMP1_INPUT:
	case PSU_TEMP2_INPUT:
	case PSU_TEMP3_INPUT:
		value = data->temp_input[attr->index-PSU_TEMP1_INPUT];
		break;
	case PSU_FAN1_SPEED:
		value = data->fan_speed;
		multiplier = 1;
		break;
	}

	exponent = two_complement_to_int(value >> 11, 5, 0x1f);
	mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);

	return (exponent >= 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * multiplier) :
							 sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static ssize_t show_ascii(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct dps850_data *data = dps850_update_device(dev);
	u8 *ptr = NULL;

	if (!data->valid) {
		return 0;
	}

	switch (attr->index) {
	case PSU_MFR_MODEL: /* psu_mfr_model */
		ptr = data->mfr_model + 1; /* The first byte is the length of string. */
		break;
	case PSU_MFR_SERIAL: /* psu_mfr_serial */
		ptr = data->mfr_serial + 1; /* The first byte is the length of string. */
		break;
	default:
		return 0;
	}

	return sprintf(buf, "%s\n", ptr);
}

static ssize_t show_vout_by_mode(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct dps850_data *data = dps850_update_device(dev);
	int exponent, mantissa;
	int multiplier = 1000;

	if (!data->valid) {
		return 0;
	}

	exponent = two_complement_to_int(data->vout_mode, 5, 0x1f);
	mantissa = data->v_out;

	return (exponent > 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * multiplier) :
							sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static const struct attribute_group dps850_group = {
	.attrs = dps850_attributes,
};

static int dps850_probe(struct i2c_client *client,
			const struct i2c_device_id *dev_id)
{
	struct dps850_data *data;
	int status;

	if (!i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA |
		I2C_FUNC_SMBUS_I2C_BLOCK)) {
		status = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct dps850_data), GFP_KERNEL);
	if (!data) {
		status = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);
	data->chip = dev_id->driver_data;
	dev_info(&client->dev, "chip found\n");

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &dps850_group);
	if (status) {
		goto exit_free;
	}

	return 0;

exit_free:
	kfree(data);
exit:

	return status;
}

static void dps850_remove(struct i2c_client *client)
{
	struct dps850_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &dps850_group);
	kfree(data);
}

static const struct i2c_device_id dps850_id[] = {
	{ "dps850", DPS850 },
	{}
};
MODULE_DEVICE_TABLE(i2c, dps850_id);

static struct i2c_driver dps850_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "dps850",
	},
	.probe	  = dps850_probe,
	.remove	  = dps850_remove,
	.id_table = dps850_id,
	.address_list = normal_i2c,
};

static int dps850_read_byte(struct i2c_client *client, u8 reg)
{
	int status = 0, retry = I2C_RW_RETRY_COUNT;

	while (retry) {
		status = i2c_smbus_read_byte_data(client, reg);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

	return status;
}

static int dps850_read_word(struct i2c_client *client, u8 reg)
{
	int status = 0, retry = I2C_RW_RETRY_COUNT;

	while (retry) {
		status = i2c_smbus_read_word_data(client, reg);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

	return status;
}

static int dps850_read_block(struct i2c_client *client, u8 command, u8 *data,
			  int data_len)
{
	int status = 0, retry = I2C_RW_RETRY_COUNT;

	while (retry) {
		status = i2c_smbus_read_i2c_block_data(client, command, data_len, data);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

	return status;
}

struct reg_data_byte {
	u8   reg;
	u8  *value;
};

struct reg_data_word {
	u8   reg;
	u16 *value;
};

static struct dps850_data *dps850_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct dps850_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
		|| !data->valid) {
		int i, status, length;
		u8 command, buf;
		struct reg_data_byte regs_byte[] = { {0x20, &data->vout_mode}};
		struct reg_data_word regs_word[] = { {0x88, &data->v_in},
											 {0x8b, &data->v_out},
											 {0x89, &data->i_in},
											 {0x8c, &data->i_out},
											 {0x96, &data->p_out},
											 {0x97, &data->p_in},
											 {0x8d, &(data->temp_input[0])},
											 {0x8e, &(data->temp_input[1])},
											 {0x8f, &(data->temp_input[2])},
											 {0x90, &data->fan_speed}};

		dev_dbg(&client->dev, "Starting dps850 update\n");
		data->valid = 0;

		/* Read byte data */
		for (i = 0; i < ARRAY_SIZE(regs_byte); i++) {
			status = dps850_read_byte(client, regs_byte[i].reg);

			if (status < 0) {
				dev_dbg(&client->dev, "reg %d, err %d\n",
						regs_byte[i].reg, status);
				goto exit;
			}
			else {
				*(regs_byte[i].value) = status;
			}
		}

		/* Read word data */
		for (i = 0; i < ARRAY_SIZE(regs_word); i++) {
			status = dps850_read_word(client, regs_word[i].reg);

			if (status < 0) {
				dev_dbg(&client->dev, "reg %d, err %d\n",
						regs_word[i].reg, status);
				goto exit;
			}
			else {
				*(regs_word[i].value) = status;
			}
		}

		/* Read mfr_model */
		command = 0x9a;
		length  = 1;
		memset(data->mfr_model, 0, sizeof(data->mfr_model));

		/* Read first byte to determine the length of data */
		status = dps850_read_block(client, command, &buf, length);
		if (status < 0) {
			dev_dbg(&client->dev, "reg %d, err %d\n", command, status);
			goto exit;
		}

		status = dps850_read_block(client, command, data->mfr_model, buf+1);
		data->mfr_model[buf+1] = '\0';

		if (status < 0) {
			dev_dbg(&client->dev, "reg %d, err %d\n", command, status);
			goto exit;
		}

		/* Read mfr_serial */
		command = 0x9e;
		length  = 1;
		memset(data->mfr_serial, 0, sizeof(data->mfr_serial));

		/* Read first byte to determine the length of data */
		status = dps850_read_block(client, command, &buf, length);
		if (status < 0) {
			dev_dbg(&client->dev, "reg %d, err %d\n", command, status);
			goto exit;
		}

		status = dps850_read_block(client, command, data->mfr_serial, buf+1);
		data->mfr_serial[buf+1] = '\0';

		if (status < 0) {
			dev_dbg(&client->dev, "reg %d, err %d\n", command, status);
			goto exit;
		}

		data->last_updated = jiffies;
		data->valid = 1;
	}

exit:
	mutex_unlock(&data->update_lock);

	return data;
}

static int __init dps850_init(void)
{
	return i2c_add_driver(&dps850_driver);
}

static void __exit dps850_exit(void)
{
	i2c_del_driver(&dps850_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("DELTA DPS-850AB driver");
MODULE_LICENSE("GPL");

module_init(dps850_init);
module_exit(dps850_exit);

