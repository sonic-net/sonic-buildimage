/*
 * A hwmon driver for the Accton as7716 32x fan
 *
 * Copyright (C) 2014 Accton Technology Corporation.
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
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define DRVNAME "as7716_32xb_fan"

#define NUM_THERMAL_SENSORS     (3)     /* Get sum of this number of sensors.*/
#define THERMAL_SENSORS_DRIVER     "lm75"
#define STRING_TO_DEC_VALUE		10

#define		IN
#define		OUT

static struct as7716_32xb_fan_data *as7716_32xb_fan_update_device(struct device *dev);
static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count);
static ssize_t get_enable(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_enable(struct device *dev, struct device_attribute *da,
                          const char *buf, size_t count);

static ssize_t fan_value_show(struct device *dev, struct device_attribute *da,
                              char *buf);
static ssize_t fan_value_store(struct device *dev, struct device_attribute *da,
                               const char *buf, size_t size);

/* fan related data, the index should match sysfs_fan_attributes
 */
static const u8 fan_reg[] = {
    0x0F,       /* fan 1-6 present status */
    0x10,        /* fan 1-6 direction(0:B2F 1:F2B) */
    0x11,       /* fan PWM(for all fan) */
    0x12,       /* front fan 1 speed(rpm) */
    0x13,       /* front fan 2 speed(rpm) */
    0x14,       /* front fan 3 speed(rpm) */
    0x15,       /* front fan 4 speed(rpm) */
    0x16,       /* front fan 5 speed(rpm) */
    0x17,       /* front fan 6 speed(rpm) */
    0x22,       /* rear fan 1 speed(rpm) */
    0x23,       /* rear fan 2 speed(rpm) */
    0x24,       /* rear fan 3 speed(rpm) */
    0x25,       /* rear fan 4 speed(rpm) */
    0x26,       /* rear fan 5 speed(rpm) */
    0x27,       /* rear fan 6 speed(rpm) */
};

#define FAN_NUM_MAX 6
/* Each client has this additional data */
struct as7716_32xb_fan_data {
    struct device   *hwmon_dev;
    struct mutex     update_lock;
    char             valid;           /* != 0 if registers are valid */
    unsigned long    last_updated;    /* In jiffies */
    u8               reg_val[ARRAY_SIZE(fan_reg)]; /* Register value */
    u8               enable;
    u8               duty_cycle;
    unsigned int     present[FAN_NUM_MAX];
    unsigned int     front_speed_rpm[FAN_NUM_MAX];
    unsigned int     rear_speed_rpm[FAN_NUM_MAX];
    unsigned int     direction[FAN_NUM_MAX];
    unsigned int     fault[FAN_NUM_MAX];
    unsigned int     input[FAN_NUM_MAX];
};

enum FAN_ID {
    FAN1_ID,
    FAN2_ID,
    FAN3_ID,
    FAN4_ID,
    FAN5_ID,
    FAN6_ID
};

enum sysfs_fan_attributes {
    FAN_PRESENT_REG,
    FAN_DIRECTION_REG,
    FAN_DUTY_CYCLE_PERCENTAGE, /* Only one CPLD register to control duty cycle for all fans */
    FAN1_FRONT_SPEED_RPM,
    FAN2_FRONT_SPEED_RPM,
    FAN3_FRONT_SPEED_RPM,
    FAN4_FRONT_SPEED_RPM,
    FAN5_FRONT_SPEED_RPM,
    FAN6_FRONT_SPEED_RPM,
    FAN1_REAR_SPEED_RPM,
    FAN2_REAR_SPEED_RPM,
    FAN3_REAR_SPEED_RPM,
    FAN4_REAR_SPEED_RPM,
    FAN5_REAR_SPEED_RPM,
    FAN6_REAR_SPEED_RPM,
    FAN1_DIRECTION,
    FAN2_DIRECTION,
    FAN3_DIRECTION,
    FAN4_DIRECTION,
    FAN5_DIRECTION,
    FAN6_DIRECTION,
    FAN1_PRESENT,
    FAN2_PRESENT,
    FAN3_PRESENT,
    FAN4_PRESENT,
    FAN5_PRESENT,
    FAN6_PRESENT,
    FAN1_FAULT,
    FAN2_FAULT,
    FAN3_FAULT,
    FAN4_FAULT,
    FAN5_FAULT,
    FAN6_FAULT
};

/* Define attributes
 */
#define DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(index, index2) \
    static SENSOR_DEVICE_ATTR(fan##index##_fault, S_IRUGO|S_IWUSR, fan_value_show, fan_value_store, FAN##index##_FAULT);\
    static SENSOR_DEVICE_ATTR(fan##index2##_fault, S_IRUGO|S_IWUSR, fan_value_show, fan_value_store, FAN##index##_FAULT)
#define DECLARE_FAN_FAULT_ATTR(index, index2)      &sensor_dev_attr_fan##index##_fault.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index2##_fault.dev_attr.attr

#define DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_direction, S_IRUGO|S_IWUSR, fan_value_show, fan_value_store, FAN##index##_DIRECTION)
#define DECLARE_FAN_DIRECTION_ATTR(index)  &sensor_dev_attr_fan##index##_direction.dev_attr.attr


#define DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan_duty_cycle_percentage, S_IWUSR | S_IRUGO, fan_value_show, fan_value_store, FAN_DUTY_CYCLE_PERCENTAGE);\
    static SENSOR_DEVICE_ATTR(pwm##index, S_IWUSR | S_IRUGO, fan_value_show, set_duty_cycle, FAN_DUTY_CYCLE_PERCENTAGE);\
    static SENSOR_DEVICE_ATTR(pwm##index##_enable, S_IWUSR | S_IRUGO, get_enable, set_enable, FAN_DUTY_CYCLE_PERCENTAGE)

#define DECLARE_FAN_DUTY_CYCLE_ATTR(index) &sensor_dev_attr_fan_duty_cycle_percentage.dev_attr.attr, \
                                           &sensor_dev_attr_pwm##index.dev_attr.attr, \
                                           &sensor_dev_attr_pwm##index##_enable.dev_attr.attr

#define DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_present, S_IWUSR|S_IRUGO, fan_value_show, fan_value_store, FAN##index##_PRESENT)
#define DECLARE_FAN_PRESENT_ATTR(index)      &sensor_dev_attr_fan##index##_present.dev_attr.attr


#define DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(index, index2) \
    static SENSOR_DEVICE_ATTR(fan##index##_front_speed_rpm, S_IWUSR|S_IRUGO, fan_value_show, fan_value_store, FAN##index##_FRONT_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index##_rear_speed_rpm, S_IWUSR|S_IRUGO, fan_value_show, fan_value_store, FAN##index##_REAR_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index##_input, S_IWUSR|S_IRUGO, fan_value_show, fan_value_store, FAN##index##_FRONT_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index2##_input, S_IWUSR|S_IRUGO, fan_value_show, fan_value_store, FAN##index##_REAR_SPEED_RPM)
#define DECLARE_FAN_SPEED_RPM_ATTR(index, index2)  &sensor_dev_attr_fan##index##_front_speed_rpm.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index##_rear_speed_rpm.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index##_input.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index2##_input.dev_attr.attr

/* 6 fan fault attributes in this platform */
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(1,11);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(2,12);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(3,13);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(4,14);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(5,15);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(6,16);
/* 6 fan speed(rpm) attributes in this platform */
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(1,11);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(2,12);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(3,13);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(4,14);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(5,15);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(6,16);
/* 6 fan present attributes in this platform */
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(1);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(2);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(3);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(4);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(5);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(6);

DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(1);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(2);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(3);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(4);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(5);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(6);

/* 1 fan duty cycle attribute in this platform */
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(1);

static struct attribute *as7716_32xb_fan_attributes[] = {
    /* fan related attributes */
    DECLARE_FAN_FAULT_ATTR(1,11),
    DECLARE_FAN_FAULT_ATTR(2,12),
    DECLARE_FAN_FAULT_ATTR(3,13),
    DECLARE_FAN_FAULT_ATTR(4,14),
    DECLARE_FAN_FAULT_ATTR(5,15),
    DECLARE_FAN_FAULT_ATTR(6,16),
    DECLARE_FAN_SPEED_RPM_ATTR(1,11),
    DECLARE_FAN_SPEED_RPM_ATTR(2,12),
    DECLARE_FAN_SPEED_RPM_ATTR(3,13),
    DECLARE_FAN_SPEED_RPM_ATTR(4,14),
    DECLARE_FAN_SPEED_RPM_ATTR(5,15),
    DECLARE_FAN_SPEED_RPM_ATTR(6,16),
    DECLARE_FAN_PRESENT_ATTR(1),
    DECLARE_FAN_PRESENT_ATTR(2),
    DECLARE_FAN_PRESENT_ATTR(3),
    DECLARE_FAN_PRESENT_ATTR(4),
    DECLARE_FAN_PRESENT_ATTR(5),
    DECLARE_FAN_PRESENT_ATTR(6),
    DECLARE_FAN_DIRECTION_ATTR(1),
    DECLARE_FAN_DIRECTION_ATTR(2),
    DECLARE_FAN_DIRECTION_ATTR(3),
    DECLARE_FAN_DIRECTION_ATTR(4),
    DECLARE_FAN_DIRECTION_ATTR(5),
    DECLARE_FAN_DIRECTION_ATTR(6),
    DECLARE_FAN_DUTY_CYCLE_ATTR(1),
    NULL
};

#define FAN_DUTY_CYCLE_REG_MASK         0xF
#define FAN_MAX_DUTY_CYCLE              100
#define FAN_REG_VAL_TO_SPEED_RPM_STEP   100

static int as7716_32xb_fan_read_value(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}

static int as7716_32xb_fan_write_value(struct i2c_client *client, u8 reg, u8 value)
{
    return i2c_smbus_write_byte_data(client, reg, value);
}

/* fan utility functions
 */
static u32 reg_val_to_duty_cycle(u8 reg_val)
{
    reg_val &= FAN_DUTY_CYCLE_REG_MASK;
    return ((u32)(reg_val+1) * 625 + 75)/ 100;
}

static u8 duty_cycle_to_reg_val(u8 duty_cycle)
{
    return ((u32)duty_cycle * 100 / 625) - 1;
}

static u32 reg_val_to_speed_rpm(u8 reg_val)
{
    return (u32)reg_val * FAN_REG_VAL_TO_SPEED_RPM_STEP;
}

static u8 reg_val_to_direction(u8 reg_val, enum FAN_ID id)
{
    u8 mask = (1 << id);

    reg_val &= mask;

    return reg_val ? 1 : 0;
}
static u8 reg_val_to_is_present(u8 reg_val, enum FAN_ID id)
{
    u8 mask = (1 << id);

    reg_val &= mask;

    return reg_val ? 0 : 1;
}

static u8 is_fan_fault(struct as7716_32xb_fan_data *data, enum FAN_ID id)
{
    u8 ret = 0;

    /* Check if the speed of front or rear fan is ZERO,
     */
    if (!data->front_speed_rpm[id] &&
            !data->rear_speed_rpm[id] )
    {
        ret = 1;
    }

    return ret;
}


static ssize_t set_enable(struct device *dev, struct device_attribute *da,
                          const char *buf, size_t count)
{
    struct as7716_32xb_fan_data *data = as7716_32xb_fan_update_device(dev);
    int error, value;

    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;

    if (value < 0 || value > 1)
        return -EINVAL;

    data->enable = value;
    if (value == 0)
    {
        return set_duty_cycle(dev, da, buf, FAN_MAX_DUTY_CYCLE);
    }
    return count;
}



static ssize_t get_enable(struct device *dev, struct device_attribute *da,
                          char *buf)
{
    struct as7716_32xb_fan_data *data = as7716_32xb_fan_update_device(dev);

    return sprintf(buf, "%u\n", data->enable);
}

static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    int error, value;
    struct i2c_client *client = to_i2c_client(dev);

    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;

    if (value < 0 || value > FAN_MAX_DUTY_CYCLE)
        return -EINVAL;

    as7716_32xb_fan_write_value(client, 0x33, 0); /* Disable fan speed watch dog */
    as7716_32xb_fan_write_value(client, fan_reg[FAN_DUTY_CYCLE_PERCENTAGE], duty_cycle_to_reg_val(value));
    return count;
}

static int fan_array_index_get(int attr_idx)
{
    switch(attr_idx)
    {
    case FAN1_PRESENT:
    case FAN1_FRONT_SPEED_RPM:
    case FAN1_REAR_SPEED_RPM:
    case FAN1_FAULT:
    case FAN1_DIRECTION:
        return FAN1_ID;
    case FAN2_PRESENT:
    case FAN2_FRONT_SPEED_RPM:
    case FAN2_REAR_SPEED_RPM:
    case FAN2_FAULT:
    case FAN2_DIRECTION:
        return FAN2_ID;
    case FAN3_PRESENT:
    case FAN3_FRONT_SPEED_RPM:
    case FAN3_REAR_SPEED_RPM:
    case FAN3_FAULT:
    case FAN3_DIRECTION:
        return FAN3_ID;
    case FAN4_PRESENT:
    case FAN4_FRONT_SPEED_RPM:
    case FAN4_REAR_SPEED_RPM:
    case FAN4_FAULT:
    case FAN4_DIRECTION:
        return FAN4_ID;
    case FAN5_PRESENT:
    case FAN5_FRONT_SPEED_RPM:
    case FAN5_REAR_SPEED_RPM:
    case FAN5_FAULT:
    case FAN5_DIRECTION:
        return FAN5_ID;
    case FAN6_PRESENT:
    case FAN6_FRONT_SPEED_RPM:
    case FAN6_REAR_SPEED_RPM:
    case FAN6_FAULT:
    case FAN6_DIRECTION:
        return FAN6_ID;
    default :
        return -1;
    }
}

static ssize_t fan_value_store(struct device *dev, struct device_attribute *da,
                               const char *buf, size_t size)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_fan_data *data = i2c_get_clientdata(client);
    int status = -EINVAL;
    int index;
    long keyin = 0;
    mutex_lock(&data->update_lock);
    switch (attr->index)
    {
    case FAN_DUTY_CYCLE_PERCENTAGE:
        status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
        if (status)
            break;
        if (keyin > 1 || keyin < 0)
            break;
        data->duty_cycle=keyin;
        break;
    case FAN1_FRONT_SPEED_RPM:
    case FAN2_FRONT_SPEED_RPM:
    case FAN3_FRONT_SPEED_RPM:
    case FAN4_FRONT_SPEED_RPM:
    case FAN5_FRONT_SPEED_RPM:
    case FAN6_FRONT_SPEED_RPM:
        status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
        if (status)
            break;
        index=fan_array_index_get(attr->index);
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        data->front_speed_rpm[index]=keyin;
        break;
    case FAN1_REAR_SPEED_RPM:
    case FAN2_REAR_SPEED_RPM:
    case FAN3_REAR_SPEED_RPM:
    case FAN4_REAR_SPEED_RPM:
    case FAN5_REAR_SPEED_RPM:
    case FAN6_REAR_SPEED_RPM:
        status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
        if (status)
            break;
        index=fan_array_index_get(attr->index);
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        data->rear_speed_rpm[index]=keyin;
    case FAN1_PRESENT:
    case FAN2_PRESENT:
    case FAN3_PRESENT:
    case FAN4_PRESENT:
    case FAN5_PRESENT:
    case FAN6_PRESENT:
        status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
        if (status)
            break;
        index=fan_array_index_get(attr->index);
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        if(keyin < 0 || keyin > 1)
            break;
        data->present[index]=keyin;
        break;
    case FAN1_FAULT:
    case FAN2_FAULT:
    case FAN3_FAULT:
    case FAN4_FAULT:
    case FAN5_FAULT:
    case FAN6_FAULT:
        status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
        if (status)
            break;
        index=fan_array_index_get(attr->index);
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        if(keyin < 0 || keyin > 1)
            break;
        data->fault[index]=keyin;
        break;
    case FAN1_DIRECTION:
    case FAN2_DIRECTION:
    case FAN3_DIRECTION:
    case FAN4_DIRECTION:
    case FAN5_DIRECTION:
    case FAN6_DIRECTION:
        status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
        if (status)
            break;
        index=fan_array_index_get(attr->index);
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        if(keyin < 0 || keyin > 1)
            break;
        data->direction[index]=keyin;
        break;
    default:
        break;
    }
    mutex_unlock(&data->update_lock);
    return size;
}
static ssize_t fan_value_show(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_fan_data *data = i2c_get_clientdata(client);
    int status = -EINVAL;
    int index=0;
    mutex_lock(&data->update_lock);
    switch (attr->index)
    {
    case FAN_DUTY_CYCLE_PERCENTAGE:
        status = sprintf(buf, "%u\n", data->duty_cycle);
        break;
    case FAN1_FRONT_SPEED_RPM:
    case FAN2_FRONT_SPEED_RPM:
    case FAN3_FRONT_SPEED_RPM:
    case FAN4_FRONT_SPEED_RPM:
    case FAN5_FRONT_SPEED_RPM:
    case FAN6_FRONT_SPEED_RPM:
        index=fan_array_index_get(attr->index);
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        status = sprintf(buf, "%u\n", data->front_speed_rpm[index]);
        break;
    case FAN1_REAR_SPEED_RPM:
    case FAN2_REAR_SPEED_RPM:
    case FAN3_REAR_SPEED_RPM:
    case FAN4_REAR_SPEED_RPM:
    case FAN5_REAR_SPEED_RPM:
    case FAN6_REAR_SPEED_RPM:
        index=fan_array_index_get(attr->index);
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        status = sprintf(buf, "%u\n", data->rear_speed_rpm[index]);
        break;
    case FAN1_PRESENT:
    case FAN2_PRESENT:
    case FAN3_PRESENT:
    case FAN4_PRESENT:
    case FAN5_PRESENT:
    case FAN6_PRESENT:
        index=fan_array_index_get(attr->index);
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        status = sprintf(buf, "%d\n", data->present[index]);
        break;
    case FAN1_FAULT:
    case FAN2_FAULT:
    case FAN3_FAULT:
    case FAN4_FAULT:
    case FAN5_FAULT:
    case FAN6_FAULT:
        index=fan_array_index_get(attr->index);
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        status = sprintf(buf, "%d\n", is_fan_fault(data, index));
        break;
    case FAN1_DIRECTION:
    case FAN2_DIRECTION:
    case FAN3_DIRECTION:
    case FAN4_DIRECTION:
    case FAN5_DIRECTION:
    case FAN6_DIRECTION:
        index=fan_array_index_get(attr->index);;
        if(index < 0 || index > (FAN_NUM_MAX-1))
            break;
        status = sprintf(buf, "%d\n", data->direction[index]);
        break;
    default:
        break;
    }
    mutex_unlock(&data->update_lock);
    return status;
}

static const struct attribute_group as7716_32xb_fan_group = {
    .attrs = as7716_32xb_fan_attributes,
};

static struct as7716_32xb_fan_data *as7716_32xb_fan_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_fan_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) ||
            !data->valid) {
        int i;

        dev_dbg(&client->dev, "Starting as7716_32xb_fan update\n");
        data->valid = 0;

        /* Update fan data
         */
        for (i = 0; i < ARRAY_SIZE(data->reg_val); i++) {
            int status = as7716_32xb_fan_read_value(client, fan_reg[i]);

            if (status < 0) {
                data->valid = 0;
                mutex_unlock(&data->update_lock);
                dev_dbg(&client->dev, "reg %d, err %d\n", fan_reg[i], status);
                return data;
            }
            else {
                data->reg_val[i] = status;
            }
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return data;
}

static int as7716_32xb_fan_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    struct as7716_32xb_fan_data *data;
    int status;

    data = kzalloc(sizeof(struct as7716_32xb_fan_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as7716_32xb_fan_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: fan '%s'\n",
             dev_name(data->hwmon_dev), client->name);

    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_fan_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void as7716_32xb_fan_remove(struct i2c_client *client)
{
    struct as7716_32xb_fan_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_fan_group);

}

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x66, I2C_CLIENT_END };

static const struct i2c_device_id as7716_32xb_fan_id[] = {
    { "as7716_32xb_fan", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, as7716_32xb_fan_id);

static struct i2c_driver as7716_32xb_fan_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = DRVNAME,
    },
    .probe        = as7716_32xb_fan_probe,
    .remove       = as7716_32xb_fan_remove,
    .id_table     = as7716_32xb_fan_id,
    .address_list = normal_i2c,
};

static int __init as7716_32xb_fan_init(void)
{
    return i2c_add_driver(&as7716_32xb_fan_driver);
}

static void __exit as7716_32xb_fan_exit(void)
{
    i2c_del_driver(&as7716_32xb_fan_driver);
}

module_init(as7716_32xb_fan_init);
module_exit(as7716_32xb_fan_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as7716_32xb_fan driver");
MODULE_LICENSE("GPL");

