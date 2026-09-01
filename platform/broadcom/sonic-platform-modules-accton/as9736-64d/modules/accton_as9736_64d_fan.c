/*
 * A hwmon driver for the Accton as9736-64d fan
 *
 * Copyright (C) 2014 Accton Technology Corporation.
 * Michael Shih <michael_shih@edge-core.com>
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

#define DRVNAME "as9736_64d_fan"

#define FAN_DUTY_CYCLE_REG_MASK         0xFF
#define FAN_MAX_DUTY_CYCLE              100

int fan_tach_clock[4] = {1048,   /*10.48 * 100*/
                         2097,   /*20.97 * 100*/
                         4194,   /*41.94 * 100*/
                         8389};  /*83.89 * 100*/

static struct as9736_64d_fan_data *as9736_64d_fan_update_device(struct device *dev);
static ssize_t fan_show_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count);
static ssize_t set_eeprom_protect(struct device *dev, struct device_attribute *da,
                                  const char *buf, size_t count);
static ssize_t set_power_enable(struct device *dev, struct device_attribute *da,
                                const char *buf, size_t count);
static ssize_t set_watch_dog(struct device *dev, struct device_attribute *da,
                             const char *buf, size_t count);

/* fan related data, the index should match sysfs_fan_attributes
 */
static const u8 fan_reg[] = {
    0x00,       /* board info */
    0x01,       /* cpld major revision */
    0x02,       /* hw pcb version: R0A, R0B... */
    0x10,       /* fan 1-4 present status, bit3: port1  ~ bit0: port4*/
    0x18,       /* fan 1-4 eeprom un-protect/protect */
    0x1a,       /* fan 1-4 power enable/disable */
    0x20,       /* watchdog enable/disable */
    0x30,       /* fan 1 PWM */
    0x31,       /* fan 2 PWM */
    0x32,       /* fan 3 PWM */
    0x33,       /* fan 4 PWM */
    0x40,       /* fan 1 front tach speed(rpm) */
    0x41,       /* fan 2 front tach speed(rpm) */
    0x42,       /* fan 3 front tach speed(rpm) */
    0x43,       /* fan 4 front tach speed(rpm) */
    0x50,       /* fan 1 rear tach speed(rpm) */
    0x51,       /* fan 2 rear tach speed(rpm) */
    0x52,       /* fan 3 rear tach speed(rpm) */
    0x53,       /* fan 4 rear tach speed(rpm) */
    0x90,       /* fan tach speed setting */
};

/* Each client has this additional data */
struct as9736_64d_fan_data {
    struct device   *hwmon_dev;
    struct mutex     update_lock;
    char             valid;           /* != 0 if registers are valid */
    unsigned long    last_updated;    /* In jiffies */
    u8               reg_val[ARRAY_SIZE(fan_reg)]; /* Register value */
    int              system_temp;    /*In unit of mini-Celsius*/
    int              sensors_found;
};

enum fan_id {
    FAN1_ID,
    FAN2_ID,
    FAN3_ID,
    FAN4_ID,
};

enum sysfs_fan_attributes {
    FAN_BOARD_INFO_REG,
    FAN_CPLD_REVISION_REG,
    FAN_HW_REVISION_REG,
    FAN_PRESENT_REG,
    FAN_EEPROM_PROTECT_REG,
    FAN_POWER_ENABLE_REG,
    FAN_WATCHDOG_REG,
    FAN1_DUTY_CYCLE_PERCENTAGE,
    FAN2_DUTY_CYCLE_PERCENTAGE,
    FAN3_DUTY_CYCLE_PERCENTAGE,
    FAN4_DUTY_CYCLE_PERCENTAGE,
    FAN1_FRONT_SPEED_RPM,
    FAN2_FRONT_SPEED_RPM,
    FAN3_FRONT_SPEED_RPM,
    FAN4_FRONT_SPEED_RPM,
    FAN1_REAR_SPEED_RPM,
    FAN2_REAR_SPEED_RPM,
    FAN3_REAR_SPEED_RPM,
    FAN4_REAR_SPEED_RPM,
    FAN_TACH_SPEED_REG,
    FAN1_PRESENT,
    FAN2_PRESENT,
    FAN3_PRESENT,
    FAN4_PRESENT,
    FAN1_EEPROM_PROTECT,
    FAN2_EEPROM_PROTECT,
    FAN3_EEPROM_PROTECT,
    FAN4_EEPROM_PROTECT,
    FAN1_POWER_ENABLE,
    FAN2_POWER_ENABLE,
    FAN3_POWER_ENABLE,
    FAN4_POWER_ENABLE,
    FAN_TACH_SPEED_CLOCK,
    FAN_TACH_SPEED_COUNTER,
    FAN1_FAULT,
    FAN2_FAULT,
    FAN3_FAULT,
    FAN4_FAULT,
    FAN_VERSION
};

/***********************************************************************
 *                  Define attributes
 ***********************************************************************/
/*fan present*/
#define DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_present, S_IRUGO, fan_show_value, NULL, FAN##index##_PRESENT)
#define DECLARE_FAN_PRESENT_ATTR(index)      &sensor_dev_attr_fan##index##_present.dev_attr.attr

/*eeprom protect*/
#define DECLARE_FAN_EEPROM_PROTECT_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_eeprom, S_IWUSR | S_IRUGO, fan_show_value, set_eeprom_protect, FAN##index##_EEPROM_PROTECT)
#define DECLARE_FAN_EEPROM_PROTECT_ATTR(index)      &sensor_dev_attr_fan##index##_eeprom.dev_attr.attr

/*power enable*/
#define DECLARE_FAN_POWER_ENABLE_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_power, S_IWUSR | S_IRUGO, fan_show_value, set_power_enable, FAN##index##_POWER_ENABLE)
#define DECLARE_FAN_POWER_ENABLE_ATTR(index)      &sensor_dev_attr_fan##index##_power.dev_attr.attr

/*fan duty cycle percentage*/
#define DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_duty_cycle_percentage, S_IWUSR | S_IRUGO, fan_show_value, set_duty_cycle, FAN##index##_DUTY_CYCLE_PERCENTAGE)
#define DECLARE_FAN_DUTY_CYCLE_ATTR(index) &sensor_dev_attr_fan##index##_duty_cycle_percentage.dev_attr.attr

/*fan speed(RPM)*/
#define DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(index, index2) \
    static SENSOR_DEVICE_ATTR(fan##index##_front_speed_rpm, S_IRUGO, fan_show_value, NULL, FAN##index##_FRONT_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index##_rear_speed_rpm, S_IRUGO, fan_show_value, NULL, FAN##index##_REAR_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index##_input, S_IRUGO, fan_show_value, NULL, FAN##index##_FRONT_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index2##_input, S_IRUGO, fan_show_value, NULL, FAN##index##_REAR_SPEED_RPM)
#define DECLARE_FAN_SPEED_RPM_ATTR(index, index2)  &sensor_dev_attr_fan##index##_front_speed_rpm.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index##_rear_speed_rpm.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index##_input.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index2##_input.dev_attr.attr

/*fan fault*/
#define DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_fault, S_IRUGO, fan_show_value, NULL, FAN##index##_FAULT)
#define DECLARE_FAN_FAULT_ATTR(index)  &sensor_dev_attr_fan##index##_fault.dev_attr.attr

/***********************************************************************
 *                  Extend attributes
 ***********************************************************************/
static SENSOR_DEVICE_ATTR(version, S_IRUGO, fan_show_value, NULL, FAN_VERSION);
static SENSOR_DEVICE_ATTR(board_info, S_IRUGO, fan_show_value, NULL, FAN_BOARD_INFO_REG);
static SENSOR_DEVICE_ATTR(cpld_ver, S_IRUGO, fan_show_value, NULL, FAN_CPLD_REVISION_REG);
static SENSOR_DEVICE_ATTR(hw_ver, S_IRUGO, fan_show_value, NULL, FAN_HW_REVISION_REG);
static SENSOR_DEVICE_ATTR(watch_dog, S_IWUSR | S_IRUGO, fan_show_value, set_watch_dog, FAN_WATCHDOG_REG);
static SENSOR_DEVICE_ATTR(fan_tach_speed_clk, S_IRUGO, fan_show_value, NULL, FAN_TACH_SPEED_CLOCK);
static SENSOR_DEVICE_ATTR(fan_tach_speed_cnt, S_IRUGO, fan_show_value, NULL, FAN_TACH_SPEED_COUNTER);
/* 4 fan present attributes in this platform */
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(1);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(2);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(3);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(4);
/* 4 fan eeprom protect attributes in this platform */
DECLARE_FAN_EEPROM_PROTECT_SENSOR_DEV_ATTR(1);
DECLARE_FAN_EEPROM_PROTECT_SENSOR_DEV_ATTR(2);
DECLARE_FAN_EEPROM_PROTECT_SENSOR_DEV_ATTR(3);
DECLARE_FAN_EEPROM_PROTECT_SENSOR_DEV_ATTR(4);
/* 4 fan power enable attributes in this platform */
DECLARE_FAN_POWER_ENABLE_SENSOR_DEV_ATTR(1);
DECLARE_FAN_POWER_ENABLE_SENSOR_DEV_ATTR(2);
DECLARE_FAN_POWER_ENABLE_SENSOR_DEV_ATTR(3);
DECLARE_FAN_POWER_ENABLE_SENSOR_DEV_ATTR(4);
/* 4 fan duty cycle percentage attribute in this platform */
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(1);
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(2);
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(3);
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(4);
/* 4 fan speed(rpm) attributes in this platform */
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(1,11);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(2,12);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(3,13);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(4,14);
/* 4 fan fault attributes in this platform */
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(1);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(2);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(3);
DECLARE_FAN_FAULT_SENSOR_DEV_ATTR(4);

/***********************************************************************
 *                  Define sysfs
 ***********************************************************************/
static struct attribute *as9736_64d_fan_attributes[] = {
    /* fan related attributes */
    &sensor_dev_attr_board_info.dev_attr.attr,
    &sensor_dev_attr_cpld_ver.dev_attr.attr,
    &sensor_dev_attr_hw_ver.dev_attr.attr,
    &sensor_dev_attr_watch_dog.dev_attr.attr,
    &sensor_dev_attr_fan_tach_speed_clk.dev_attr.attr,
    &sensor_dev_attr_fan_tach_speed_cnt.dev_attr.attr,
    DECLARE_FAN_PRESENT_ATTR(1),
    DECLARE_FAN_PRESENT_ATTR(2),
    DECLARE_FAN_PRESENT_ATTR(3),
    DECLARE_FAN_PRESENT_ATTR(4),
    DECLARE_FAN_EEPROM_PROTECT_ATTR(1),
    DECLARE_FAN_EEPROM_PROTECT_ATTR(2),
    DECLARE_FAN_EEPROM_PROTECT_ATTR(3),
    DECLARE_FAN_EEPROM_PROTECT_ATTR(4),
    DECLARE_FAN_POWER_ENABLE_ATTR(1),
    DECLARE_FAN_POWER_ENABLE_ATTR(2),
    DECLARE_FAN_POWER_ENABLE_ATTR(3),
    DECLARE_FAN_POWER_ENABLE_ATTR(4),
    DECLARE_FAN_DUTY_CYCLE_ATTR(1),
    DECLARE_FAN_DUTY_CYCLE_ATTR(2),
    DECLARE_FAN_DUTY_CYCLE_ATTR(3),
    DECLARE_FAN_DUTY_CYCLE_ATTR(4),
    DECLARE_FAN_SPEED_RPM_ATTR(1,11),
    DECLARE_FAN_SPEED_RPM_ATTR(2,12),
    DECLARE_FAN_SPEED_RPM_ATTR(3,13),
    DECLARE_FAN_SPEED_RPM_ATTR(4,14),
    DECLARE_FAN_FAULT_ATTR(1),
    DECLARE_FAN_FAULT_ATTR(2),
    DECLARE_FAN_FAULT_ATTR(3),
    DECLARE_FAN_FAULT_ATTR(4),
    &sensor_dev_attr_version.dev_attr.attr,
    NULL
};


static int as9736_64d_fan_read_value(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}

static int as9736_64d_fan_write_value(struct i2c_client *client, u8 reg, u8 value)
{
    return i2c_smbus_write_byte_data(client, reg, value);
}

/* fan utility functions
 */
static u32 reg_val_to_duty_cycle(u8 reg_val)
{
    reg_val &= FAN_DUTY_CYCLE_REG_MASK;
    u32 duty_cycle = 0;

    switch(reg_val)
    {
        case 0:
            duty_cycle = 0;
            break;
        case 25: /*0x19*/
            duty_cycle = 10;
            break;
        case 51: /*0x33*/
            duty_cycle = 20;
            break;
        case 76: /*0x4c*/
            duty_cycle = 30;
            break;
        case 102: /*0x66*/
            duty_cycle = 40;
            break;
        case 127: /*0x7F*/
            duty_cycle = 50;
            break;
        case 153: /*0x99*/
            duty_cycle = 60;
            break;
        case 179: /*0xB3*/
            duty_cycle = 70;
            break;
        case 204: /*0xCC*/
            duty_cycle = 80;
            break;
        case 229: /*0xE5*/
            duty_cycle = 90;
            break;
        default: /*0xFF*/
            duty_cycle = 100;
            break;
    }

    return duty_cycle;
}

static u32 reg_val_to_speed_rpm(u8 reg_val, u8 tach_speed)
{
    int fan_tach_clk_index = (tach_speed >> 6) & 0x3;
    int fan_tach_cnt = tach_speed & 0x3F;
    /*
       Coculation formula  = reg_val/(fan_clk*can_cnt)*1000/2*60 = [(reg_val)*30000] / fan_clk*can_cnt
    */
    return (u32)( (reg_val * 30000 * 100) / (fan_tach_clock[fan_tach_clk_index] * fan_tach_cnt) );
}

static u8 reg_val_to_is_present(u8 reg_val, enum fan_id id)
{
    u8 mask = (1 << (id) );

    reg_val &= mask;  /*reg: 0 is present, 1 is not present*/

    return reg_val ? 0 : 1; /*turn to : 0 is not present, 1 is present*/
}

static u8 is_fan_fault(struct as9736_64d_fan_data *data, enum fan_id id)
{
    u8 ret = 1;
    int front_fan_index = id + 11;       /*fan1=11, fan2=12, fan3=13, fan4=14*/
    int rear_fan_index  = front_fan_index  + 4; /*fan1=15, fan2=16, fan3=17, fan4=18*/

    /* Check if the speed of front or rear fan is ZERO,
     */
    if (reg_val_to_speed_rpm(data->reg_val[front_fan_index], data->reg_val[FAN_TACH_SPEED_REG]) &&
        reg_val_to_speed_rpm(data->reg_val[rear_fan_index], data->reg_val[FAN_TACH_SPEED_REG]) )  {
        ret = 0;
    }

    return ret;
}

static u8 reg_val_to_is_eeprom_protect(u8 reg_val, enum fan_id id)
{
    u8 mask = 0x1;
    reg_val = reg_val >> (id);
    reg_val &= mask;  /*reg: 0 is unprotect, 1 is protect*/

    return reg_val;
}

static u8 reg_val_to_is_power_enable(u8 reg_val, enum fan_id id)
{
    u8 mask = (1 << (3 - id) );

    reg_val &= mask;  /*reg: 0 is enable, 1 is disable*/

   return reg_val ? 0 : 1; /*turn to : 0 is disable, 1 is enable*/
}

static ssize_t set_duty_cycle(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    int error, value;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 duty_cycle = 0;

    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;

    if (value < 0 || value > FAN_MAX_DUTY_CYCLE)
        return -EINVAL;

    switch(value)
    {
        case 0:
            duty_cycle = 0;
            break;
        case 1 ... 10:
            duty_cycle = 25;
            break;
        case 11 ... 20:
            duty_cycle = 51;
            break;
        case 21 ... 30:
            duty_cycle = 76;
            break;
        case 31 ... 40:
            duty_cycle = 102;
            break;
        case 41 ... 50:
            duty_cycle = 127;
            break;
        case 51 ... 60:
            duty_cycle = 153;
            break;
        case 61 ... 70:
            duty_cycle = 179;
            break;
        case 71 ... 80:
            duty_cycle = 204;
            break;
        case 81 ... 90:
            duty_cycle = 229;
            break;
        default:
            duty_cycle = 255;
            break;
    }

    switch (attr->index) {
        case FAN1_DUTY_CYCLE_PERCENTAGE:
            as9736_64d_fan_write_value(client, fan_reg[FAN1_DUTY_CYCLE_PERCENTAGE], duty_cycle);
            break;
        case FAN2_DUTY_CYCLE_PERCENTAGE:
            as9736_64d_fan_write_value(client, fan_reg[FAN2_DUTY_CYCLE_PERCENTAGE], duty_cycle);
            break;
        case FAN3_DUTY_CYCLE_PERCENTAGE:
            as9736_64d_fan_write_value(client, fan_reg[FAN3_DUTY_CYCLE_PERCENTAGE], duty_cycle);
            break;
        default:
            as9736_64d_fan_write_value(client, fan_reg[FAN4_DUTY_CYCLE_PERCENTAGE], duty_cycle);
            break;
    }

    return count;
}

static u8 set_reg_bit(u8 reg_val, u8 number_bit, u8 value)
{
    int chk_bit, i;
    u8 high_bytes, lower_bytes;
    u8 mask = 0x0;
    chk_bit = (reg_val >> number_bit) & 0x1;

    if( chk_bit==value ) {
        return reg_val;
    } else {
        for( i = 1 ; i <= number_bit ; i++ ) {
            mask |= 1 << (i-1);
        }
        lower_bytes = reg_val & mask;
        high_bytes = reg_val & ( (~mask << 1) );

        return (u8)( high_bytes | (value << number_bit) | lower_bytes );
    }
}

static ssize_t set_eeprom_protect(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    int reg_val, port_bit;
    int error, value;
    struct i2c_client *client = to_i2c_client(dev);
    struct as9736_64d_fan_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;

    if (value < 0 || value > 1)
        return -EINVAL;

    reg_val = as9736_64d_fan_read_value(client, fan_reg[FAN_EEPROM_PROTECT_REG]);
    port_bit = (3 - (attr->index - FAN1_EEPROM_PROTECT) );

    mutex_lock(&data->update_lock);
       as9736_64d_fan_write_value(client, fan_reg[FAN_EEPROM_PROTECT_REG], set_reg_bit(reg_val, port_bit, value));
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t set_power_enable(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    int reg_val, port_bit;
    int error, value;
    struct i2c_client *client = to_i2c_client(dev);
    struct as9736_64d_fan_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;

    if (value < 0 || value > 1)
        return -EINVAL;

    reg_val = as9736_64d_fan_read_value(client, fan_reg[FAN_POWER_ENABLE_REG]);
    port_bit = (3 - (attr->index - FAN1_POWER_ENABLE) );

    mutex_lock(&data->update_lock);
       as9736_64d_fan_write_value(client, fan_reg[FAN_POWER_ENABLE_REG], set_reg_bit(reg_val, port_bit, value ? 0 : 1));
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t set_watch_dog(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    int error, value;
    struct i2c_client *client = to_i2c_client(dev);
    struct as9736_64d_fan_data *data = i2c_get_clientdata(client);

    error = kstrtoint(buf, 10, &value);
    if (error)
        return error;

    if (value < 0 || value > 1)
        return -EINVAL;

    mutex_lock(&data->update_lock);
        as9736_64d_fan_write_value(client, fan_reg[FAN_WATCHDOG_REG], value);
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t fan_show_value(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct as9736_64d_fan_data *data = as9736_64d_fan_update_device(dev);
    ssize_t ret = 0;
    u32 duty_cycle = 0;

    if (data->valid) {
        switch (attr->index) {
        case FAN_VERSION:
            ret = sprintf(buf, "%d.%d\n", (data->reg_val[FAN_CPLD_REVISION_REG]) & 0x7F, (data->reg_val[FAN_BOARD_INFO_REG]) & 0xFF);
            break;
        case FAN_BOARD_INFO_REG:
            ret = sprintf(buf, "0x%x\n", (data->reg_val[FAN_BOARD_INFO_REG]) & 0xF);
            break;
        case FAN_CPLD_REVISION_REG:
            ret = sprintf(buf, "0x%x\n", (data->reg_val[FAN_CPLD_REVISION_REG]) & 0xF);
            break;
        case FAN_HW_REVISION_REG:
            ret = sprintf(buf, "0x%x\n", (data->reg_val[FAN_HW_REVISION_REG]) & 0xFF);
            break;
        case FAN1_PRESENT:
        case FAN2_PRESENT:
        case FAN3_PRESENT:
        case FAN4_PRESENT:
            ret = sprintf(buf, "%d\n", reg_val_to_is_present(data->reg_val[FAN_PRESENT_REG],
                                                             attr->index - FAN1_PRESENT) );
            break;
        case FAN1_EEPROM_PROTECT:
        case FAN2_EEPROM_PROTECT:
        case FAN3_EEPROM_PROTECT:
        case FAN4_EEPROM_PROTECT:
            ret = sprintf(buf, "%d\n", reg_val_to_is_eeprom_protect(data->reg_val[FAN_EEPROM_PROTECT_REG],
                                                                    attr->index - FAN1_EEPROM_PROTECT) );
            break;
        case FAN1_POWER_ENABLE:
        case FAN2_POWER_ENABLE:
        case FAN3_POWER_ENABLE:
        case FAN4_POWER_ENABLE:
            ret = sprintf(buf, "%d\n",
                          reg_val_to_is_power_enable(data->reg_val[FAN_POWER_ENABLE_REG],
                                                attr->index - FAN1_POWER_ENABLE) );
            break;
        case FAN_WATCHDOG_REG:
            ret = sprintf(buf, "%x\n", (data->reg_val[FAN_WATCHDOG_REG]) & 0x1);
            break;
        case FAN1_DUTY_CYCLE_PERCENTAGE:
            duty_cycle = reg_val_to_duty_cycle(data->reg_val[FAN1_DUTY_CYCLE_PERCENTAGE]);
            ret = sprintf(buf, "%u\n", duty_cycle);
            break;
        case FAN2_DUTY_CYCLE_PERCENTAGE:
            duty_cycle = reg_val_to_duty_cycle(data->reg_val[FAN2_DUTY_CYCLE_PERCENTAGE]);
            ret = sprintf(buf, "%u\n", duty_cycle);
            break;
        case FAN3_DUTY_CYCLE_PERCENTAGE:
            duty_cycle = reg_val_to_duty_cycle(data->reg_val[FAN3_DUTY_CYCLE_PERCENTAGE]);
            ret = sprintf(buf, "%u\n", duty_cycle);
            break;
        case FAN4_DUTY_CYCLE_PERCENTAGE:
        {
            duty_cycle = reg_val_to_duty_cycle(data->reg_val[FAN4_DUTY_CYCLE_PERCENTAGE]);
            ret = sprintf(buf, "%u\n", duty_cycle);
            break;
        }
        case FAN1_FRONT_SPEED_RPM:
        case FAN2_FRONT_SPEED_RPM:
        case FAN3_FRONT_SPEED_RPM:
        case FAN4_FRONT_SPEED_RPM:
        case FAN1_REAR_SPEED_RPM:
        case FAN2_REAR_SPEED_RPM:
        case FAN3_REAR_SPEED_RPM:
        case FAN4_REAR_SPEED_RPM:
            ret = sprintf(buf, "%u\n", reg_val_to_speed_rpm(data->reg_val[attr->index], data->reg_val[FAN_TACH_SPEED_REG]));
            break;
        case FAN_TACH_SPEED_CLOCK:
            ret = sprintf(buf, "%d\n", ((data->reg_val[FAN_TACH_SPEED_REG]) >> 6) & 0x3); /*reg: 0x90, upper 2 bits*/
            break;
        case FAN_TACH_SPEED_COUNTER:
            ret = sprintf(buf, "%d\n", (data->reg_val[FAN_TACH_SPEED_REG]) & 0x3F); /*reg: 0x90, lower 6 bits*/
            break;
        case FAN1_FAULT:
        case FAN2_FAULT:
        case FAN3_FAULT:
        case FAN4_FAULT:
            ret = sprintf(buf, "%d\n", is_fan_fault(data, attr->index - FAN1_FAULT));
            break;
        default:
            break;
        }
    }

    return ret;
}

static const struct attribute_group as9736_64d_fan_group = {
    .attrs = as9736_64d_fan_attributes,
};

static struct as9736_64d_fan_data *as9736_64d_fan_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as9736_64d_fan_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) ||
            !data->valid) {
        int i;

        dev_dbg(&client->dev, "Starting as9736_64d_fan update\n");
        data->valid = 0;

        /* Update fan data
         */
        for (i = 0; i < ARRAY_SIZE(data->reg_val); i++) {
            int status = as9736_64d_fan_read_value(client, fan_reg[i]);
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

static int as9736_64d_fan_probe(struct i2c_client *client,
                                const struct i2c_device_id *dev_id)
{
    struct as9736_64d_fan_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct as9736_64d_fan_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as9736_64d_fan_group);
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
    sysfs_remove_group(&client->dev.kobj, &as9736_64d_fan_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static int as9736_64d_fan_remove(struct i2c_client *client)
{
    struct as9736_64d_fan_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as9736_64d_fan_group);

    return 0;
}

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x66, I2C_CLIENT_END };

static const struct i2c_device_id as9736_64d_fan_id[] = {
    { "as9736_64d_fan", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, as9736_64d_fan_id);

static struct i2c_driver as9736_64d_fan_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = DRVNAME,
    },
    .probe        = as9736_64d_fan_probe,
    .remove       = as9736_64d_fan_remove,
    .id_table     = as9736_64d_fan_id,
    .address_list = normal_i2c,
};

module_i2c_driver(as9736_64d_fan_driver);

MODULE_AUTHOR("Michael Shih <michael_shih@edge-core.com>");
MODULE_DESCRIPTION("as9736_64d_fan driver");
MODULE_LICENSE("GPL");

