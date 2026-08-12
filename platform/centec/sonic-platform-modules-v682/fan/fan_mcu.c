// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * fan_mcu.c - Part of fan, Linux kernel modules for hardware monitoring
 * Author: qianj <qianj@centec.com>
 * Copyright 2005-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/log2.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/util_macros.h>
#include <linux/sched.h>

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x2C, 0x2E, 0x2F, I2C_CLIENT_END };


#define FAN_MCU_REG_VENDOR			0x3E
#define FAN_MCU_REG_DEVICE			0x3D
#define FAN_MCU_VENDOR				0xcb
#define FAN_MCU_DEVICE				0x10

//#define FAN_MCU_FAN_COUNT	12
#define FAN_MCU_FAN_COUNT	6
#define FAN_MCU_TACH_BASE_REG 0x50


//#define FAN_MCU_PWM_COUNT	12
#define FAN_MCU_PWM_COUNT	6
#define FAN_MCU_PWM_BASE_REG 0x10
#define FAN_MCU_PWM_DUTY_MIN 0
#define FAN_MCU_PWM_DUTY_MAX 100

struct fan_mcu_data {
	struct i2c_client	*client;
	struct mutex		update_lock;
	char   valid;
	unsigned long		last_updated;	/* In jiffies */
	u8	   pwm[FAN_MCU_PWM_COUNT]; 
	u16    fan[FAN_MCU_FAN_COUNT];
};

static struct fan_mcu_data *fan_mcu_update_device(struct device *dev)
{
	struct fan_mcu_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	u16 tach = 0;
	u8 tach_reg = 0;
	int i;

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
	    || !data->valid)
    {
		dev_dbg(&client->dev, "Starting fan mcu update\n");

		for (i = 0; i < FAN_MCU_FAN_COUNT; i++) 
        {
			tach_reg = FAN_MCU_TACH_BASE_REG + 2*2*i;
			tach = i2c_smbus_read_byte_data(client, tach_reg);
			tach_reg = FAN_MCU_TACH_BASE_REG + 2*2*i + 1;
			tach = tach + (i2c_smbus_read_byte_data(client, tach_reg) << 8);
			data->fan[i] = tach;
		}

		for (i = 0; i < FAN_MCU_PWM_COUNT; i++) 
        {
			data->pwm[i] = i2c_smbus_read_byte_data(client, FAN_MCU_PWM_BASE_REG + 2*i);
		}
        
		data->last_updated = jiffies;
		data->valid = 1;
	}

	mutex_unlock(&data->update_lock);

	return data;
}


static ssize_t fan_show(struct device *dev, struct device_attribute *devattr,
			char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct fan_mcu_data *data = fan_mcu_update_device(dev);

	return sprintf(buf, "%d\n",data->fan[attr->index]);

}


static ssize_t pwm_show(struct device *dev, struct device_attribute *devattr,
			char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct fan_mcu_data *data = fan_mcu_update_device(dev);

	return sprintf(buf, "%d\n", data->pwm[attr->index]);
}

static ssize_t pwm_store(struct device *dev, struct device_attribute *devattr,
			 const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct fan_mcu_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int nr = attr->index;
	long val;
	int err;

	err = kstrtol(buf, 10, &val);
	if (err)
		return err;

	val = clamp_val(val, FAN_MCU_PWM_DUTY_MIN, FAN_MCU_PWM_DUTY_MAX);

	mutex_lock(&data->update_lock);
	data->pwm[nr] = (u8)val;
	i2c_smbus_write_byte_data(client, FAN_MCU_PWM_BASE_REG + 2*nr, val);
	i2c_smbus_write_byte_data(client, FAN_MCU_PWM_BASE_REG + 2*nr + 1, val);
	mutex_unlock(&data->update_lock);

	return count;
}

static SENSOR_DEVICE_ATTR_RW(pwm1, pwm, 0);
static SENSOR_DEVICE_ATTR_RW(pwm2, pwm, 1);
static SENSOR_DEVICE_ATTR_RW(pwm3, pwm, 2);
static SENSOR_DEVICE_ATTR_RW(pwm4, pwm, 3);
static SENSOR_DEVICE_ATTR_RW(pwm5, pwm, 4);
static SENSOR_DEVICE_ATTR_RW(pwm6, pwm, 5);
#if 0
static SENSOR_DEVICE_ATTR_RW(pwm7, pwm, 6);
static SENSOR_DEVICE_ATTR_RW(pwm8, pwm, 7);
static SENSOR_DEVICE_ATTR_RW(pwm9, pwm, 8);
static SENSOR_DEVICE_ATTR_RW(pwm10, pwm, 9);
static SENSOR_DEVICE_ATTR_RW(pwm11, pwm, 10);
static SENSOR_DEVICE_ATTR_RW(pwm12, pwm, 11);
#endif

static SENSOR_DEVICE_ATTR_RO(fan1_input, fan, 0);
static SENSOR_DEVICE_ATTR_RO(fan2_input, fan, 1);
static SENSOR_DEVICE_ATTR_RO(fan3_input, fan, 2);
static SENSOR_DEVICE_ATTR_RO(fan4_input, fan, 3);
static SENSOR_DEVICE_ATTR_RO(fan5_input, fan, 4);
static SENSOR_DEVICE_ATTR_RO(fan6_input, fan, 5);
#if 0
static SENSOR_DEVICE_ATTR_RO(fan7_input, fan, 6);
static SENSOR_DEVICE_ATTR_RO(fan8_input, fan, 7);
static SENSOR_DEVICE_ATTR_RO(fan9_input, fan, 8);
static SENSOR_DEVICE_ATTR_RO(fan10_input, fan, 9);
static SENSOR_DEVICE_ATTR_RO(fan11_input, fan, 10);
static SENSOR_DEVICE_ATTR_RO(fan12_input, fan, 11);
#endif
static struct attribute *fan_mcu_attrs[] = {
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_pwm2.dev_attr.attr,
	&sensor_dev_attr_pwm3.dev_attr.attr,
	&sensor_dev_attr_pwm4.dev_attr.attr,
	&sensor_dev_attr_pwm5.dev_attr.attr,
	&sensor_dev_attr_pwm6.dev_attr.attr,
#if 0
	&sensor_dev_attr_pwm7.dev_attr.attr,
	&sensor_dev_attr_pwm8.dev_attr.attr,
	&sensor_dev_attr_pwm9.dev_attr.attr,
	&sensor_dev_attr_pwm10.dev_attr.attr,
	&sensor_dev_attr_pwm11.dev_attr.attr,
	&sensor_dev_attr_pwm12.dev_attr.attr,
#endif
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	&sensor_dev_attr_fan3_input.dev_attr.attr,
	&sensor_dev_attr_fan4_input.dev_attr.attr,
	&sensor_dev_attr_fan5_input.dev_attr.attr,
	&sensor_dev_attr_fan6_input.dev_attr.attr,
#if 0
	&sensor_dev_attr_fan7_input.dev_attr.attr,
	&sensor_dev_attr_fan8_input.dev_attr.attr,
	&sensor_dev_attr_fan9_input.dev_attr.attr,
	&sensor_dev_attr_fan10_input.dev_attr.attr,
	&sensor_dev_attr_fan11_input.dev_attr.attr,
	&sensor_dev_attr_fan12_input.dev_attr.attr,
#endif
	NULL,
};

ATTRIBUTE_GROUPS(fan_mcu);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int fan_mcu_detect(struct i2c_client *client,
			  struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int vendor, device;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	vendor = i2c_smbus_read_byte_data(client, FAN_MCU_REG_VENDOR);
	if (vendor != FAN_MCU_VENDOR)
		return -ENODEV;

	device = i2c_smbus_read_byte_data(client, FAN_MCU_REG_DEVICE);
	if (device != FAN_MCU_DEVICE)
		return -ENODEV;

	strscpy(info->type, "fan_mcu", I2C_NAME_SIZE);

	return 0;
}

static int fan_mcu_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct device *hwmon_dev;
	struct fan_mcu_data *data;

	data = devm_kzalloc(dev, sizeof(struct fan_mcu_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	mutex_init(&data->update_lock);

	dev_info(&client->dev, "%s chip found\n", client->name);

	/* Register sysfs hooks */
	hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
							   data, fan_mcu_groups);

	return PTR_ERR_OR_ZERO(hwmon_dev);
}


static const struct i2c_device_id fan_mcu_id[] = {
	{ "fan_mcu", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, fan_mcu_id);

static struct i2c_driver fan_mcu_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "fan_mcu",
	},
	.probe  	= fan_mcu_probe,
	//.remove		= fan_mcu_remove,
	.id_table	= fan_mcu_id,
	.detect		= fan_mcu_detect,
	.address_list	= normal_i2c,
};

module_i2c_driver(fan_mcu_driver);

MODULE_AUTHOR("Qianj <qianj@centec.com>");
MODULE_DESCRIPTION("FAN MCU driver");
MODULE_LICENSE("GPL");
