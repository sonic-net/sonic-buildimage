// SPDX-License-Identifier: GPL-2.0+
/*
 * Hardware monitoring driver for CAD2512.
 *
 * Copyright (c) 2020 Facebook Inc.
 */

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>

/*
 * CAD2512 Control Byte. Refer to CAD2512 datasheet
 */
#define CAD2512_CHIP1_BUS		4
#define CAD2512_CHIP2_ADDR		0x1D
#define CAD2512_CHIP3_ADDR		0x1F

#define CAD2512_NUM_CHANNELS			8

#define CAD2512_CFG_REG					0x0
#define CAD2512_ADV_CFG_REG				0xB
#define CAD2512_BASE_REG_IN				0x20
#define CAD2512_BASE_REG_LIMIT_HI		0x2A
#define CAD2512_BASE_REG_LIMIT_LOW		0x2B

#define CAD2512_SET_CHANNEL_IN(ch)		((ch) & 7)
#define CAD2512_SET_CHANNEL_LIMIT(ch)	(((ch) & 7) << 1)

#define CAD2512_INPUT_LEN		2
#define CAD2512_LIMIT_LEN		1
#define CAD2512_INPUT_SHIFT		4

/*
 * voltage(mV)=ch_val*2.56/(2^12)*factor*1000
 * calculate by long
 * ->voltage(mV)=ch_val*256*(factor*100)/(40960)
 *
 * limit(mV)=limit_val*2.56/(2^8)*factor*1000
 * calculate by long
 * ->limit(mV)=limit_val*256*(factor*100)/2560
 */
#define CAD2512_FIXED_FACTOR1		(256)
#define CAD2512_FIXED_FACTOR2		(40960)
#define CAD2512_FIXED_FACTOR3		(2560)
const long cad2512_chip1_factor100[CAD2512_NUM_CHANNELS]={300,570,110,0,0,0,0,0};
const long cad2512_chip2_factor100[CAD2512_NUM_CHANNELS]={321,182,502,100,100,120,0,0};
const long cad2512_chip3_factor100[CAD2512_NUM_CHANNELS]={100,120,321,321,321,120,182,100};

struct cad2512_data {
	struct i2c_client *client;
	u8 in_reg[CAD2512_NUM_CHANNELS];
	u8 limit_low_reg[CAD2512_NUM_CHANNELS];
	u8 limit_hi_reg[CAD2512_NUM_CHANNELS];
};

static long cad2512_get_factor(struct i2c_client *client , int channel)
{
	if (client->adapter->nr == CAD2512_CHIP1_BUS) {
		return cad2512_chip1_factor100[channel];
	} else {
		if (client->addr == CAD2512_CHIP2_ADDR)
			return cad2512_chip2_factor100[channel];
		else if (client->addr == CAD2512_CHIP3_ADDR)
			return cad2512_chip3_factor100[channel];
	}

	return 0;
}

static int cad2512_read_ch(struct i2c_client *client, u8 ctrl_byte, long *val)
{
	int status,hi_b,low_b;

	status = i2c_smbus_read_word_data(client, ctrl_byte);
	if (status < 0)
		return status;
	hi_b = ((status >> 8) & 0xff);
	low_b = (status & 0xff);

	*val = (hi_b >> CAD2512_INPUT_SHIFT) |
		((u16)low_b << CAD2512_INPUT_SHIFT);

	//printk("read reg val:%x %x val=%ld\n", i2c_data[0], i2c_data[1],*val);
	return 0;
}

static int cad2512_read_input(struct cad2512_data *data, int channel, long *val)
{
	long raw;
	long factor;
	int status;
	struct i2c_client *client = data->client;
	u8 ctrl_byte = data->in_reg[channel];

	status = cad2512_read_ch(client, ctrl_byte, &raw);
	if (status)
		goto exit;

	factor = cad2512_get_factor(client, channel);
	*val = (raw * CAD2512_FIXED_FACTOR1 * factor / CAD2512_FIXED_FACTOR2);

exit:
	return status;
}

static int cad2512_read_min(struct cad2512_data *data, int channel, long *val)
{
	u8 raw;
	int status;
	long factor;
	struct i2c_client *client = data->client;
	u8 ctrl_byte = data->limit_low_reg[channel];

	status = i2c_smbus_read_byte_data(client, ctrl_byte);
	if (status < 0)
		goto exit;
	raw = (u8)status;
	factor = cad2512_get_factor(client, channel);
	*val = ((long)raw * CAD2512_FIXED_FACTOR1 * factor / CAD2512_FIXED_FACTOR3);
exit:
	return status;
}

static int cad2512_read_max(struct cad2512_data *data, int channel, long *val)
{
	u8 raw;
	int status;
	long factor;
	struct i2c_client *client = data->client;
	u8 ctrl_byte = data->limit_hi_reg[channel];

	status = i2c_smbus_read_byte_data(client, ctrl_byte);

	if (status < 0)
		goto exit;
	raw = (u8)status;

	factor = cad2512_get_factor(client, channel);
	*val = ((long)raw * CAD2512_FIXED_FACTOR1 * factor / CAD2512_FIXED_FACTOR3);

exit:
	return status;
}

static int cad2512_write_min(struct cad2512_data *data, int channel, long val)
{
	int status;
	long factor;
	u8 byte;
	struct i2c_client *client = data->client;
	u8 ctrl_byte = data->limit_low_reg[channel];

	factor = cad2512_get_factor(client, channel);
	byte = (u8)((val * CAD2512_FIXED_FACTOR3) / CAD2512_FIXED_FACTOR1 / factor);

	status = i2c_smbus_write_byte_data(client, ctrl_byte, byte);

	return status;
}

static int cad2512_write_max(struct cad2512_data *data, int channel, long val)
{
	int status;
	long factor;
	u8 byte;
	struct i2c_client *client = data->client;
	u8 ctrl_byte = data->limit_hi_reg[channel];

	factor = cad2512_get_factor(client, channel);
	byte = (u8)((val * CAD2512_FIXED_FACTOR3) / CAD2512_FIXED_FACTOR1 / factor);

	status = i2c_smbus_write_byte_data(client, ctrl_byte, byte);

	return status;
}

static umode_t cad2512_is_visible(const void *_data,
				 enum hwmon_sensor_types type,
				 u32 attr, int channel)
{
	if (type == hwmon_in) {
		switch (attr) {
		case hwmon_in_input:
			return 0444;

		case hwmon_in_min:
		case hwmon_in_max:
			return 0644;

		default:
			break;
		}
	}

	return 0;
}

static int cad2512_read(struct device *dev, enum hwmon_sensor_types type,
			u32 attr, int channel, long *val)
{
	int status;
	struct cad2512_data *data = dev_get_drvdata(dev);

	if (type != hwmon_in)
		return -EOPNOTSUPP;

	switch (attr) {
	case hwmon_in_input:
		status = cad2512_read_input(data, channel, val);
		break;

	case hwmon_in_min:
		status = cad2512_read_min(data, channel, val);
		break;

	case hwmon_in_max:
		status = cad2512_read_max(data, channel, val);
		break;

	default:
		status = -EOPNOTSUPP;
		break;
	}

	return status;
}

static int cad2512_write(struct device *dev, enum hwmon_sensor_types type,
			u32 attr, int channel, long val)
{
	int status;
	struct cad2512_data *data = dev_get_drvdata(dev);

	if (type != hwmon_in)
		return -EOPNOTSUPP;

	switch (attr) {
	case hwmon_in_min:
		status = cad2512_write_min(data, channel, val);
		break;

	case hwmon_in_max:
		status = cad2512_write_max(data, channel, val);
		break;

	default:
		status = -EOPNOTSUPP;
		break;
	}

	return status;
}

static const struct hwmon_ops cad2512_hwmon_ops = {
	.is_visible = cad2512_is_visible,
	.read = cad2512_read,
	.write = cad2512_write,
};

static const struct hwmon_channel_info *cad2512_info[] = {
	HWMON_CHANNEL_INFO(in,
			   HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX,
			   HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX,
			   HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX,
			   HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX,
			   HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX,
			   HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX,
			   HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX,
			   HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX),
	NULL,
};

static const struct hwmon_chip_info cad2512_chip_info = {
	.ops = &cad2512_hwmon_ops,
	.info = cad2512_info,
};

static int cad2512_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int i;
	struct device *hwmon_dev;
	struct cad2512_data *data;
	struct device *dev = &client->dev;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;

	for (i = 0; i < ARRAY_SIZE(data->in_reg); i++)
		data->in_reg[i] = (CAD2512_BASE_REG_IN + CAD2512_SET_CHANNEL_IN(i));

	for (i = 0; i < ARRAY_SIZE(data->limit_low_reg); i++)
		data->limit_low_reg[i] = (CAD2512_BASE_REG_LIMIT_LOW + CAD2512_SET_CHANNEL_LIMIT(i));

	for (i = 0; i < ARRAY_SIZE(data->limit_hi_reg); i++)
		data->limit_hi_reg[i] = (CAD2512_BASE_REG_LIMIT_HI + CAD2512_SET_CHANNEL_LIMIT(i));

	hwmon_dev = devm_hwmon_device_register_with_info(dev, client->name,
							 data,
							 &cad2512_chip_info,
							 NULL);

	/* init cad2512 */
	i2c_smbus_write_byte_data(client, CAD2512_ADV_CFG_REG, 0x2);
	i2c_smbus_write_byte_data(client, CAD2512_CFG_REG, 0x1);

	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct i2c_device_id cad2512_id[] = {
	{ "cad2512", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cad2512_id);

static struct i2c_driver cad2512_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "cad2512",
	},
	.probe		= cad2512_probe,
	.id_table	= cad2512_id,
};

module_i2c_driver(cad2512_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhuang.yong@embedway.com");
MODULE_DESCRIPTION("CAD2512 Hardware Monitoring driver");
