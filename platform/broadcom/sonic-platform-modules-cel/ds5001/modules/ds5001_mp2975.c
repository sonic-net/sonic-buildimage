// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hardware monitoring driver for MPS Multi-phase Digital VR Controllers
 *
 * Copyright (C) 2020 Nvidia Technologies Ltd.
 *
 * Note:
 * 1. Implement full P0/P1 support for existing sensors.
 * 2. STANDARD_UNITS: Convert all sysfs output to standard
 *    hwmon units (mV, mA, uW, mC) based on data format assumptions.
 * The driver converts these to standard hwmon units.
 *
 * --- Page 0 Sensors (6 files) ---
 *
 * sysfs Node   | PMBus Cmd | Meaning                | Output Unit
 * --------------------------------------------------------------------------
 * in1_input    | 0x8b      | P0 Output Volts        | Millivolts (mV)
 * curr1_input  | 0x8f      | P0 Input Current       | Milliamps (mA)
 * curr2_input  | 0x8c      | P0 Output Current      | Milliamps (mA)
 * power1_input | 0x97      | P0 Input Power (PIN)   | Microwatts (uW)
 * power2_input | 0x96      | P0 Output Power (POUT) | Microwatts (uW)
 * temp1_input  | 0x8d      | P0 Temperature         | Millicelsius (mC)
 *
 *
 * --- Page 1 Sensors (3 files) ---
 * (PIN, IIN, and TEMP do not exist on Page 1)
 *
 * sysfs Node   | PMBus Cmd | Meaning                | Output Unit
 * --------------------------------------------------------------------------
 * in2_input    | 0x8b      | P1 Output Volts |        Millivolts (mV)
 * curr3_input  | 0x8c      | P1 Output Current      | Milliamps (mA)
 * power3_input | 0x96      | P1 Output Power (POUT) | Microwatts (uW)
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/gfp.h>
#include <linux/pmbus.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>

#define MP2975_I2C_TIMEOUT_MS    50
#define MP2975_MAX_RETRIES       2
#define MP2975_SHORT_DELAY_US    100
#define MP2975_LONG_DELAY_MS     5
#define MP2975_POWER_UP_DELAY_MS 200

#define PMBUS_PAGE               0x00
#define PMBUS_VOUT_MODE          0x20
#define PMBUS_VOUT_MAX           0x24
#define PMBUS_READ_VOUT          0x8b
#define PMBUS_READ_IOUT          0x8c
#define PMBUS_READ_TEMPERATURE   0x8d
#define PMBUS_READ_VIN           0x8e
#define PMBUS_READ_IIN           0x8f
#define PMBUS_READ_POUT          0x96
#define PMBUS_READ_PIN           0x97

/* Conversion factors for hwmon standard units */
#define CONVERT_TEMP_TO_MILLI      1000 /* C -> mC */
#define CONVERT_AMPS_TO_MILLIAMPS  1000 /* A -> mA */
#define CONVERT_WATTS_TO_MICROWATTS 1000000 /* W -> uW */

enum mp2975_sensors {
	/* Page 0 Sensors (All 6) */
	in_vout_p0,
	curr_iin_p0,
	curr_iout_p0,
	power_pin_p0,
	power_pout_p0,
	temp_p0,

	/* Page 1 Sensors (Only 3) */
	in_vout_p1,
	curr_iout_p1,
	power_pout_p1,

	MAX_SENSORS   /* limit on the number of sensors */
};

struct mp2975_data {
	struct mutex lock;
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	int current_page;
	bool is_initialized;
};

/* MODIFIED: Return type changed to long long to prevent overflow */
static long long mp2975_read_vout(struct mp2975_data *data);
static long long mp2975_read_iin(struct mp2975_data *data);
static long long mp2975_read_iout(struct mp2975_data *data);
static long long mp2975_read_temp(struct mp2975_data *data);
static long long mp2975_read_pin(struct mp2975_data *data);
static long long mp2975_read_pout(struct mp2975_data *data);

/* MODIFIED: Return type changed to long long */
typedef long long (*mp2975_sensor_reader_t)(struct mp2975_data *data);

static const mp2975_sensor_reader_t mp2975_sensor_readers[MAX_SENSORS] = {
	[in_vout_p0] = mp2975_read_vout,
	[curr_iin_p0] = mp2975_read_iin,
	[curr_iout_p0] = mp2975_read_iout,
	[power_pin_p0] = mp2975_read_pin,
	[power_pout_p0] = mp2975_read_pout,
	[temp_p0] = mp2975_read_temp,

	[in_vout_p1] = mp2975_read_vout,
	[curr_iout_p1] = mp2975_read_iout,
	[power_pout_p1] = mp2975_read_pout,
};

static const int mp2975_sensor_pages[MAX_SENSORS] = {
	[in_vout_p0] = 0,
	[curr_iin_p0] = 0,
	[curr_iout_p0] = 0,
	[power_pin_p0] = 0,
	[power_pout_p0] = 0,
	[temp_p0] = 0,

	[in_vout_p1] = 1,
	[curr_iout_p1] = 1,
	[power_pout_p1] = 1,
};


static int mp2975_i2c_write_byte(struct mp2975_data *data, u8 cmd, u8 value)
{
	int ret;
	unsigned long timeout = jiffies + msecs_to_jiffies(MP2975_I2C_TIMEOUT_MS);

	data->adapter->timeout = MP2975_I2C_TIMEOUT_MS;

	do {
		ret = i2c_smbus_write_byte_data(data->client, cmd, value);
		if (ret >= 0)
			return ret;

		udelay(MP2975_SHORT_DELAY_US);
	} while (time_before(jiffies, timeout));

	dev_err(&data->client->dev, "I2C write timeout: addr=0x%02x, cmd=0x%02x\n",
			data->client->addr, cmd);
	return -ETIMEDOUT;
}

static int mp2975_i2c_read_word(struct mp2975_data *data, u8 cmd)
{
	int ret;
	unsigned long timeout = jiffies + msecs_to_jiffies(MP2975_I2C_TIMEOUT_MS);

	data->adapter->timeout = MP2975_I2C_TIMEOUT_MS;

	do {
		/* * We use read_word_data, which handles the 16-bit value.
		 * Based on user i2cget testing, byte swapping is NOT needed.
		 * 0x002d is read as 45.
		 */
		ret = i2c_smbus_read_word_data(data->client, cmd);
		if (ret >= 0)
			return ret;

		udelay(MP2975_SHORT_DELAY_US);
	} while (time_before(jiffies, timeout));

	dev_err(&data->client->dev, "I2C read timeout: addr=0x%02x, cmd=0x%02x\n",
			data->client->addr, cmd);
	return -ETIMEDOUT;
}

static int mp2975_set_page(struct mp2975_data *data, int page, bool is_init)
{
	int ret;
	int retry = MP2975_MAX_RETRIES;

	if (page < 0 || page > 3) {
		return -EINVAL;
	}

	if (data->current_page == page)
		return 0;

	while (retry-- > 0) {
		ret = mp2975_i2c_write_byte(data, PMBUS_PAGE, page);
		if (ret == 0) {
			data->current_page = page;
			return 0;
		}

		msleep(MP2975_LONG_DELAY_MS);
		dev_dbg(&data->client->dev, "Retry set page to %d (remaining: %d)\n", page, retry);
	}

	if (is_init) {
		dev_warn(&data->client->dev, "Failed to set page %d, continue with defaults\n", page);
		return -EAGAIN;
	}

	dev_err(&data->client->dev, "Failed to set page %d after retries\n", page);
	return -ETIMEDOUT;
}

/* Assumes return value is already in millivolts (mV) */
static long long mp2975_read_vout(struct mp2975_data *data)
{
	return mp2975_i2c_read_word(data, PMBUS_READ_VOUT);
}

/* Assumes return value is in mA, returns it for hwmon (mA) */
static long long mp2975_read_iin(struct mp2975_data *data)
{
	/* TODO: Confirm if 0x8f (IIN) is in Amps or Milliamps. */
	/* Assuming Milliamps for now, matching VOUT's milli- prefix. */
	return mp2975_i2c_read_word(data, PMBUS_READ_IIN);
}

/* MODIFIED: Assumes return value is in Amps (A), converts to Milliamps (mA) */
static long long mp2975_read_iout(struct mp2975_data *data)
{
	long long ret = mp2975_i2c_read_word(data, PMBUS_READ_IOUT);
	return ret < 0 ? ret : ret * CONVERT_AMPS_TO_MILLIAMPS;
}

/* Assumes return value is in C, converts to mC for hwmon */
static long long mp2975_read_temp(struct mp2975_data *data)
{
	long long ret = mp2975_i2c_read_word(data, PMBUS_READ_TEMPERATURE);

	return ret < 0 ? ret : ret * CONVERT_TEMP_TO_MILLI;
}

/* MODIFIED: Assumes return value is in Watts (W), converts to Microwatts (uW) */
static long long mp2975_read_pin(struct mp2975_data *data)
{
	long long ret = mp2975_i2c_read_word(data, PMBUS_READ_PIN);

	return ret < 0 ? ret : ret * CONVERT_WATTS_TO_MICROWATTS;
}

/* MODIFIED: Assumes return value is in Watts (W), converts to Microwatts (uW) */
static long long mp2975_read_pout(struct mp2975_data *data)
{
	long long ret = mp2975_i2c_read_word(data, PMBUS_READ_POUT);

	return ret < 0 ? ret : ret * CONVERT_WATTS_TO_MICROWATTS;
}

static ssize_t mp2975_show_sensor(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mp2975_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
	int index = sattr->index;
	long long value = -EIO; /* MODIFIED: Changed to long long */
	int ret;
	ktime_t start;
	int original_page;

	if (index < 0 || index >= MAX_SENSORS)
		return -EINVAL;

	if (!mp2975_sensor_readers[index])
		return -ENOSYS;

	if (!data->is_initialized)
		return -ENODEV;

	mutex_lock(&data->lock);
	start = ktime_get();
	original_page = data->current_page;

	ret = mp2975_set_page(data, mp2975_sensor_pages[index], false);
	if (ret < 0 && ret != -EAGAIN) {
		dev_err(dev, "Failed to set page for sensor %d\n", index);
		value = ret;
		goto unlock;
	}

	value = mp2975_sensor_readers[index](data);

	if (ktime_to_ms(ktime_sub(ktime_get(), start)) > MP2975_I2C_TIMEOUT_MS * 2) {
		dev_err(dev, "Sensor read timeout (index=%d)\n", index);
		value = -ETIMEDOUT;
	}

unlock:
	if (original_page != data->current_page) {
		mp2975_set_page(data, original_page, false);
	}
	mutex_unlock(&data->lock);

	if (value < 0)
		return value;

	/* MODIFIED: Print as %lld (long long) */
	return sprintf(buf, "%lld\n", value);
}

/* Page 0 Attributes */
static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, mp2975_show_sensor, NULL, in_vout_p0);
static SENSOR_DEVICE_ATTR(curr1_input, S_IRUGO, mp2975_show_sensor, NULL, curr_iin_p0);
static SENSOR_DEVICE_ATTR(curr2_input, S_IRUGO, mp2975_show_sensor, NULL, curr_iout_p0);
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, mp2975_show_sensor, NULL, power_pin_p0);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, mp2975_show_sensor, NULL, power_pout_p0);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, mp2975_show_sensor, NULL, temp_p0);

/* Page 1 Attributes */
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, mp2975_show_sensor, NULL, in_vout_p1);
static SENSOR_DEVICE_ATTR(curr3_input, S_IRUGO, mp2975_show_sensor, NULL, curr_iout_p1);
static SENSOR_DEVICE_ATTR(power3_input, S_IRUGO, mp2975_show_sensor, NULL, power_pout_p1);


/* Attribute list of all existing sensors */
static struct attribute *mp2975_attrs[] = {
	/* Page 0 */
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_curr1_input.dev_attr.attr,
	&sensor_dev_attr_curr2_input.dev_attr.attr,
	&sensor_dev_attr_power1_input.dev_attr.attr,
	&sensor_dev_attr_power2_input.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	/* Page 1 */
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_curr3_input.dev_attr.attr,
	&sensor_dev_attr_power3_input.dev_attr.attr,
	NULL
};

static const struct attribute_group mp2975_attr_group = {
	.attrs = mp2975_attrs,
};

static int mp2975_init_device(struct mp2975_data *data)
{
	int ret;
	int page;

	/* This is the simple probe that succeeds on this hardware */
	ret = mp2975_i2c_read_word(data, PMBUS_VOUT_MODE);
	if (ret < 0) {
		dev_err(&data->client->dev, "Device not responding to init check\n");
		return ret;
	}

	/* Check that both pages are accessible */
	for (page = 0; page < 2; page++) {
		ret = mp2975_set_page(data, page, true);
		if (ret < 0 && ret != -EAGAIN) {
			dev_warn(&data->client->dev, "Page %d initialization skipped\n", page);
		} else {
			dev_dbg(&data->client->dev, "Page %d initialized\n", page);
		}
	}

	mp2975_set_page(data, 0, true);

	return 0;
}

static int mp2975_probe(struct i2c_client *client)
{
	struct mp2975_data *data;
	int ret;

	msleep(MP2975_POWER_UP_DELAY_MS);

	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	data->adapter = client->adapter;
	data->current_page = -1;
	data->is_initialized = false;
	mutex_init(&data->lock);

	ret = mp2975_init_device(data);
	if (ret < 0) {
		dev_err(&client->dev, "Device initialization failed\n");
		return ret;
	}

	ret = sysfs_create_group(&client->dev.kobj, &mp2975_attr_group);
	if (ret) {
		dev_err(&client->dev, "Failed to create sysfs attributes\n");
		return ret;
	}

	data->is_initialized = true;
	i2c_set_clientdata(client, data);

	dev_info(&client->dev, "Initialized at 0x%02x\n", client->addr);

	return 0;
}

static void mp2975_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &mp2975_attr_group);
	dev_info(&client->dev, "Removed from 0x%02x\n", client->addr);
}

static const struct i2c_device_id mp2975_id[] = {
	{"mp2975gu-z", 0},
	{"mp2975", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, mp2975_id);

static const struct of_device_id mp2975_of_match[] = {
	{ .compatible = "mps,mp2975gu-z" },
	{ .compatible = "mps,mp2975" },
	{}
};
MODULE_DEVICE_TABLE(of, mp2975_of_match);

static struct i2c_driver mp2975_driver = {
	.driver = {
		.name = "mp2975gu-z",
		.of_match_table = mp2975_of_match,
	},
	.probe = mp2975_probe,
	.remove = mp2975_remove,
	.id_table = mp2975_id,
};
module_i2c_driver(mp2975_driver);
MODULE_AUTHOR("Yagami Jiang <yajiang@celestica.com>");
MODULE_DESCRIPTION("PMBus driver for DS5001 MP2975 device");
MODULE_LICENSE("GPL");