/*
 * A hwmon driver for the Accton as4625 fan
 *
 * Copyright (C) 2016 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
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
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/platform_device.h>

#define DRVNAME "as4625_fan"

#define FAN_STATUS_I2C_ADDR	0x64

static struct as4625_fan_data *as4625_fan_update_device(struct device *dev);
static ssize_t fan_show_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
extern int as4625_cpld_read(unsigned short cpld_addr, u8 reg);
extern int as4625_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

/* fan related data, the index should match sysfs_fan_attributes
 */
static const u8 fan_reg[] = {
	0x40,	   /* fan-1 PWM */
	0x41,	   /* fan-2 PWM */
	0x42,	   /* fan-3 PWM */
	0x46,	   /* fan-1 speed(rpm) */
	0x47,	   /* fan-2 speed(rpm) */
	0x48,	   /* fan-3 speed(rpm) */
	0x00	   /* fan direction, 001: AS4625-54T (F2B), 010: AS4625-54T (B2F) */
};

static const int fan_target_speed_f2b[] = {
	    0,  1350,  2550,  3900,  6450,  7800,  9150, 10500,
	11850, 13050, 14400, 15600, 16950, 18150, 19650, 23000
};

static const int fan_target_speed_b2f[] = {
	    0,  3150,  5850,  8100, 12150, 13650, 15000, 16350,
	17850, 19050, 19950, 20700, 21450, 22200, 22800, 23400
};

/* fan data */
struct as4625_fan_data {
    struct platform_device *pdev;
    struct device   *hwmon_dev;
    struct mutex     update_lock;
    char             valid;           /* != 0 if registers are valid */
    unsigned long    last_updated;    /* In jiffies */
    u8               reg_val[ARRAY_SIZE(fan_reg)]; /* Register value */
};

struct as4625_fan_data *data = NULL;

enum fan_id {
    FAN1_ID,
    FAN2_ID,
    FAN3_ID
};

enum sysfs_fan_attributes {
    FAN1_PWM,
    FAN2_PWM,
    FAN3_PWM,
    FAN1_INPUT,
    FAN2_INPUT,
    FAN3_INPUT,
    FAN1_FAULT,
    FAN2_FAULT,
    FAN3_FAULT,
    FAN1_DIR,
    FAN2_DIR,
    FAN3_DIR,
    FAN1_TARGET_RPM,
    FAN2_TARGET_RPM,
    FAN3_TARGET_RPM
};

/* Define attributes
 */
#define DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_fault, S_IRUGO, fan_show_value, NULL, FAN##index##_FAULT)
#define DECLARE_FAN_FAULT_ATTR(index)      &sensor_dev_attr_fan##index##_fault.dev_attr.attr

#define DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_pwm, S_IWUSR | S_IRUGO, fan_show_value, set_duty_cycle, FAN##index##_PWM)
#define DECLARE_FAN_DUTY_CYCLE_ATTR(index) &sensor_dev_attr_fan##index##_pwm.dev_attr.attr

#define DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_input, S_IRUGO, fan_show_value, NULL, FAN##index##_INPUT)
#define DECLARE_FAN_SPEED_RPM_ATTR(index)  &sensor_dev_attr_fan##index##_input.dev_attr.attr

#define DECLARE_FAN_DIR_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_dir, S_IRUGO, fan_show_value, NULL, FAN##index##_DIR)
#define DECLARE_FAN_DIR_ATTR(index)  &sensor_dev_attr_fan##index##_dir.dev_attr.attr

#define DECLARE_FAN_TARGET_RPM_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_target_rpm, S_IRUGO, fan_show_value, NULL, FAN##index##_TARGET_RPM)
#define DECLARE_FAN_TARGET_RPM_ATTR(index)  &sensor_dev_attr_fan##index##_target_rpm.dev_attr.attr

/* 3 fan fault attributes in this platform */
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(1);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(2);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(3);

/* 3 fan speed(rpm) attributes in this platform */
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(1);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(2);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(3);

/* 3 fan duty cycle attribute in this platform */
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(1);
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(2);
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(3);

/* 3 fan direction attribute in this platform */
DECLARE_FAN_DIR_SENSOR_DEV_ATTR(1);
DECLARE_FAN_DIR_SENSOR_DEV_ATTR(2);
DECLARE_FAN_DIR_SENSOR_DEV_ATTR(3);

/* 3 fan target speed attribute in this platform */
DECLARE_FAN_TARGET_RPM_SENSOR_DEV_ATTR(1);
DECLARE_FAN_TARGET_RPM_SENSOR_DEV_ATTR(2);
DECLARE_FAN_TARGET_RPM_SENSOR_DEV_ATTR(3);

static struct attribute *as4625_fan_attributes[] = {
    /* fan related attributes */
    DECLARE_FAN_FAULT_ATTR(1),
    DECLARE_FAN_FAULT_ATTR(2),
    DECLARE_FAN_FAULT_ATTR(3),
    DECLARE_FAN_DUTY_CYCLE_ATTR(1),
    DECLARE_FAN_DUTY_CYCLE_ATTR(2),
    DECLARE_FAN_DUTY_CYCLE_ATTR(3),
    DECLARE_FAN_SPEED_RPM_ATTR(1),
    DECLARE_FAN_SPEED_RPM_ATTR(2),
    DECLARE_FAN_SPEED_RPM_ATTR(3),
    DECLARE_FAN_DIR_ATTR(1),
    DECLARE_FAN_DIR_ATTR(2),
    DECLARE_FAN_DIR_ATTR(3),
    DECLARE_FAN_TARGET_RPM_ATTR(1),
    DECLARE_FAN_TARGET_RPM_ATTR(2),
    DECLARE_FAN_TARGET_RPM_ATTR(3),
    NULL
};

#define FAN_DUTY_CYCLE_REG_MASK         0x0F
#define FAN_MAX_DUTY_CYCLE              100
#define FAN_REG_VAL_TO_SPEED_RPM_STEP   150

static int as4625_fan_read_value(u8 reg)
{
    return as4625_cpld_read(FAN_STATUS_I2C_ADDR, reg);
}

static int as4625_fan_write_value(u8 reg, u8 value)
{
    return as4625_cpld_write(FAN_STATUS_I2C_ADDR, reg, value);
}

/* fan utility functions
 */
static u32 reg_val_to_duty_cycle(u8 reg_val)
{
	reg_val &= FAN_DUTY_CYCLE_REG_MASK;
	return (u32)(reg_val+1) * 625 / 100;
}

static u8 duty_cycle_to_reg_val(u8 duty_cycle)
{
    if (duty_cycle == 0) {
        return 0;
    }
	else if (duty_cycle > FAN_MAX_DUTY_CYCLE) {
		duty_cycle = FAN_MAX_DUTY_CYCLE;
	}

    return ((u32)duty_cycle * 100 / 625) - 1;
}

static u32 reg_val_to_speed_rpm(u8 reg_val)
{
    return (u32)reg_val * FAN_REG_VAL_TO_SPEED_RPM_STEP;
}

static u8 is_fan_fault(struct as4625_fan_data *data, enum fan_id id)
{
	return !reg_val_to_speed_rpm(data->reg_val[FAN1_INPUT + id]);
}

static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    int error, value, reg;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;

    if (value < 0 || value > FAN_MAX_DUTY_CYCLE)
        return -EINVAL;

    mutex_lock(&data->update_lock);

    reg = fan_reg[attr->index - FAN1_PWM];
    as4625_fan_write_value(reg, duty_cycle_to_reg_val(value));
	data->valid = 0;

    mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t fan_show_value(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    ssize_t ret = 0;

    mutex_lock(&data->update_lock);

    data = as4625_fan_update_device(dev);
    if (data->valid) {
        switch (attr->index) {
            case FAN1_PWM:
            case FAN2_PWM:
            case FAN3_PWM:
            {
                u32 duty_cycle;
                duty_cycle = reg_val_to_duty_cycle(data->reg_val[attr->index]);
                ret = sprintf(buf, "%u\n", duty_cycle);
                break;
            }
            case FAN1_INPUT:
            case FAN2_INPUT:
            case FAN3_INPUT:
                ret = sprintf(buf, "%u\n", reg_val_to_speed_rpm(data->reg_val[attr->index]));
                break;
            case FAN1_FAULT:
            case FAN2_FAULT:
            case FAN3_FAULT:
                ret = sprintf(buf, "%d\n", is_fan_fault(data, attr->index - FAN1_FAULT));
                break;
            case FAN1_DIR:
            case FAN2_DIR:
            case FAN3_DIR:
            {
                u8 board_id = (data->reg_val[6] >> 3) & 0x7;
                ret = sprintf(buf, "%s\n", (board_id == 2) ? "B2F" : "F2B");
                break;
            }
            case FAN1_TARGET_RPM:
            case FAN2_TARGET_RPM:
            case FAN3_TARGET_RPM:
            {
                u8 board_id = (data->reg_val[6] >> 3) & 0x7;
                const int *target = NULL;
                int pwm_index = FAN1_PWM + (attr->index - FAN1_TARGET_RPM);
                target = (board_id == 2) ? fan_target_speed_b2f : fan_target_speed_f2b;
                ret = sprintf(buf, "%d\n", target[data->reg_val[pwm_index] & 0xF]);
                break;
            }
            default:
                break;
        }
    }

    mutex_unlock(&data->update_lock);

    return ret;
}

static const struct attribute_group as4625_fan_group = {
    .attrs = as4625_fan_attributes,
};

static struct as4625_fan_data *as4625_fan_update_device(struct device *dev)
{
    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) ||
        !data->valid) {
        int i;

        dev_dbg(&data->pdev->dev, "Starting as4625_fan update\n");
        data->valid = 0;

        /* Update fan data
         */
        for (i = 0; i < ARRAY_SIZE(data->reg_val); i++) {
            int status = as4625_fan_read_value(fan_reg[i]);

            if (status < 0) {
                data->valid = 0;
                mutex_unlock(&data->update_lock);
                dev_dbg(&data->pdev->dev, "reg %d, err %d\n", fan_reg[i], status);
                return data;
            }
            else {
                data->reg_val[i] = status;
            }
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    return data;
}

static int as4625_fan_probe(struct platform_device *pdev)
{
	int status;

	data->hwmon_dev = hwmon_device_register_with_info(&pdev->dev,
											DRVNAME, NULL, NULL, NULL);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		return status;
	}

	/* Register sysfs hooks */
	status = sysfs_create_group(&data->hwmon_dev->kobj, &as4625_fan_group);
	if (status)
		goto exit_remove;

	dev_info(&pdev->dev, "device created\n");
    return 0;

exit_remove:
	hwmon_device_unregister(data->hwmon_dev);
	return status;
}

static int as4625_fan_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&data->hwmon_dev->kobj, &as4625_fan_group);
	hwmon_device_unregister(data->hwmon_dev);
    return 0;
}

static struct platform_driver as4625_fan_driver = {
	.probe = as4625_fan_probe,
	.remove = as4625_fan_remove,
	.driver = {
		.name = DRVNAME,
		.owner = THIS_MODULE,
	},
};

static int __init as4625_fan_init(void)
{
	int ret;

	data = kzalloc(sizeof(struct as4625_fan_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	mutex_init(&data->update_lock);
	data->valid = 0;

	ret = platform_driver_register(&as4625_fan_driver);
	if (ret < 0)
		goto dri_reg_err;

	data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(data->pdev)) {
		ret = PTR_ERR(data->pdev);
		goto dev_reg_err;
	}

	return 0;

dev_reg_err:
	platform_driver_unregister(&as4625_fan_driver);
dri_reg_err:
	kfree(data);
alloc_err:
	return ret;
}

static void __exit as4625_fan_exit(void)
{
	platform_device_unregister(data->pdev);
	platform_driver_unregister(&as4625_fan_driver);
	kfree(data);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as4625_fan driver");
MODULE_LICENSE("GPL");

module_init(as4625_fan_init);
module_exit(as4625_fan_exit);
