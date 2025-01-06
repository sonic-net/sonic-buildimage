/*
 * g3748cpld.c - A driver for control of SMC SSE-G3748 cpld based on lm75.c
 *
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
 * Copyright (c) 2023 Supermicro <support@supermicro.com>
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

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x66, I2C_CLIENT_END };

/* CPLD WDT global timeout */
static u32 gTimeout = 0;

/* Size of EEPROM in bytes */
#define CPLD_SIZE 12

#define FAN_NUM   2
#define PSU_NUM   2
#define DEFAULT_VOLTAGE_MIN   11000
#define DEFAULT_VOLTAGE_MAX   13000
#define DEFAULT_VOLTAGE_LCRIT 11500
#define DEFAULT_VOLTAGE_CRIT  12500

#define G3748_CPLD_VER_REG 0x09
//#define G3748_POWER_GOOD_REG 0x0A
//#define G3748_PSU_PRESENT_REG 0x0A
#define G3748_PSU_STATUS_REG 0x0A
#define G3748_FAN_STATUS_REG 0x3B
#define G3748_FAN_CONTROL_REG 0x3C
#define G3748_FAN_SPEED_REG 0x3E
#define G3748_PSU_FRONTLED_REG 0x12
#define G3748_FAN_SYS_FRONTLED_REG 0x11
#define G3748_WDT_TIMER_REG 0x1B
#define G3748_WDT_CTRL_REG 0x1C
#define G3748_WDT_INTR_CNT_MASK_REG 0x1D
#define G3748_WDT_INTR_FLAG_REG 0x1E

/* Each client has this additional data */
struct cpld_data {
	struct device           *hwmon_dev;
	struct i2c_client 	*client;
	struct mutex		update_lock;
	char			valid;		    /* !=0 if registers are valid */
	unsigned long		last_updated;       /* In jiffies */
	u8			data[CPLD_SIZE];    /* Register value */

        u32                     in[PSU_NUM];        /* Register value for PSU Voltage input value */
	u32                     in_min[PSU_NUM];    /* Register value for PSU Voltage min value */
	u32                     in_max[PSU_NUM];    /* Register value for PSU Voltage max value */
	u32                     in_crit[PSU_NUM];   /* Register value for PSU Voltage critical max value */
	u32                     in_lcrit[PSU_NUM];  /* Register value for PSU Voltage critical min value */

	u32                     fan[FAN_NUM];       /* Register value for FAN speed(RPM) input value */
        u32                     fan_min[FAN_NUM];   /* Register value for FAN speed(RPM) min value */	
        u32                     fan_max[FAN_NUM];   /* Register value for FAN speed(RPM) max value */
        u32                     fan_target[FAN_NUM];   /* Register value for FAN desired speed(RPM) value */
	u8                      pwm[FAN_NUM];       /* Register value for Pulse Width Modulation Ctrl 0-255 255:max(100%) */
};

u8 gCurrentPwm = 100;   /* Powe on default CPLD FAN Ctrl Reg Bit[4..0] = 0x1f (pwm: 100%) */

static int g3748cpld_read (struct cpld_data *data, u8 reg)
{
    int ret = -EPERM;
    //u8  high_reg = 0x00;

    mutex_lock(&data->update_lock);
    //ret = i2c_smbus_write_byte_data (data->client, high_reg, reg);
    ret = i2c_smbus_read_byte_data (data->client, reg);
    mutex_unlock(&data->update_lock);

    return ret;
}

static int g3748cpld_write (struct cpld_data *data, u8 reg, u8 val)
{
    int ret = 0;

    mutex_lock(&data->update_lock);
    ret = i2c_smbus_write_byte_data (data->client, reg, val);
    mutex_unlock(&data->update_lock);

    return ret;
}

#if 0
static ssize_t show_hwmon_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct cpld_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int index = to_sensor_dev_attr_2(da)->index;

	mutex_lock(&data->update_lock);
	data->data[0] = i2c_smbus_read_byte_data(client, index);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", data->data[0]);
}

static ssize_t show_sysfs_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);
	data->data[0] = i2c_smbus_read_byte_data(client, attr->index);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%02x\n", data->data[0]);
}

static ssize_t set_hwmon_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	struct cpld_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int index = to_sensor_dev_attr_2(da)->index;
	u8 temp;
	int error;

	error = kstrtou8(buf, 10, &temp);
	if (error)
		return error;

	mutex_lock(&data->update_lock);
	i2c_smbus_write_byte_data(client, index, temp);
	mutex_unlock(&data->update_lock);

	return count;
}
#endif

static ssize_t get_cpld_ver(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    u8  devdata = 0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = g3748cpld_read (data, G3748_CPLD_VER_REG);
    if (ret < 0)
	return sprintf(buf, "cpld read error");

    devdata = (u8)ret & 0xff;
    return sprintf(buf, "0x%02X\n", devdata);
}

static ssize_t get_power_good(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    u8  devdata = 0;
    struct cpld_data *data = dev_get_drvdata(dev);
    int index = to_sensor_dev_attr_2(da)->index;

    //ret = g3748cpld_read (data, G3748_POWER_GOOD_REG);
    ret = g3748cpld_read (data, G3748_PSU_STATUS_REG);
    if (ret < 0)
        return sprintf(buf, "cpld read error");

    if (index == 1) {
        devdata = (u8)ret & 0x04;
    } else if (index == 2) {
	devdata = (u8)ret & 0x08;
    }

    return sprintf(buf, "%s\n", (devdata == 0)? "12000":"0");
}

static ssize_t get_psu_status(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    //u8  devdata = 0;
    struct cpld_data *data = dev_get_drvdata(dev);
    //int index = to_sensor_dev_attr_2(da)->index;

    ret = g3748cpld_read (data, G3748_PSU_STATUS_REG);
    if (ret < 0)
        return sprintf(buf, "cpld psu status read error");

    //if (index == 1) {
    //    devdata = (u8)ret & 0x01;
    //} else if (index == 2) {
    //    devdata = (u8)ret & 0x02;
    //}

    return sprintf(buf, "%d\n", (u8)ret);
}

static void initCpldDataReg (struct cpld_data *data)
{
    if (data != NULL) {
	data->in_min[0] = data->in_min[1] = DEFAULT_VOLTAGE_MIN;
	data->in_max[0] = data->in_max[1] = DEFAULT_VOLTAGE_MAX;
	data->in_lcrit[0] = data->in_lcrit[1] = DEFAULT_VOLTAGE_LCRIT;
	data->in_crit[0] = data->in_crit[1] = DEFAULT_VOLTAGE_CRIT;
    }
}

static ssize_t get_voltage_limit(struct device *dev, struct device_attribute *da, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    int index = to_sensor_dev_attr_2(da)->index;
    u32 inData = 0;
    //ret = g3748cpld_read (data, G3748_PSU_PRESENT_REG);
    //if (ret < 0)
    //    return sprintf(buf, "cpld read error");

    /* Register data read from our cpld_data structure storage area */
    if ((index == 11) || (index == 12)) {
        /* Read for in1_min or in2_min */
	inData = data->in_min[index - 11];
    } else if ((index == 21) || (index == 22)) {
        /* Read for in1_max or in2_max */
        inData = data->in_max[index - 21];
    } else if ((index == 31) || (index == 32)) {
        /* Read for in1_lcrit or in2_lcrit */
        inData = data->in_lcrit[index - 31];
    } else if ((index == 41) || (index == 42)) {
        /* Read for in1_crit or in2_crit */
        inData = data->in_crit[index - 41];
    }

    return sprintf(buf, "%d\n", (int)inData);
}

static ssize_t set_voltage_limit(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    int index = to_sensor_dev_attr_2(da)->index;
    u32 inData = 0;
    int error;

    error = kstrtou32(buf, 10, &inData);
    if (error)
        return error;

    /* Register data read from our cpld_data structure storage area */
    if ((index == 11) || (index == 12)) {
        /* write in1_min or in2_min */
        data->in_min[index - 11] = inData;
    } else if ((index == 21) || (index == 22)) {
        /* write in1_max or in2_max */
        data->in_max[index - 21] = inData;
    } else if ((index == 31) || (index == 32)) {
        /* write in1_lcrit or in2_lcrit */
        data->in_lcrit[index - 31] = inData;
    } else if ((index == 41) || (index == 42)) {
        /* Read for in1_crit or in2_crit */
        data->in_crit[index - 41] = inData;
    }

    return count;
}

static ssize_t get_fan_status(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret = 0;
    //u8  devdata = 0;
    struct cpld_data *data = dev_get_drvdata(dev);
    //int index = to_sensor_dev_attr_2(da)->index;

    ret = g3748cpld_read (data, G3748_FAN_STATUS_REG);
    if (ret < 0)
        return sprintf(buf, "cpld fan status read error");

    //if (index == 1) {
    //    devdata = (u8)ret & 0x04;
    //} else if (index == 2) {
    //    devdata = (u8)ret & 0x08;
    //}

    return sprintf(buf, "%d\n", (u8)ret);
}

static ssize_t get_fan_fault(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret = 0;
    u8  fault = 0;
    struct cpld_data *data = dev_get_drvdata(dev);
    int index = to_sensor_dev_attr_2(da)->index;

    ret = g3748cpld_read (data, G3748_FAN_STATUS_REG);
    if (ret < 0)
        return sprintf(buf, "ERR: fan status read");

    if (index == 1) {
        if (((u8)ret & 0x04) == 0) {
	    fault = 1;   /* not present */
	}
    } else if (index == 2) {
        if (((u8)ret & 0x084) == 0) {
	    fault = 1;   /* not present */
	}
    }

    return sprintf(buf, "%d\n", fault);
}

static ssize_t get_fan_speed(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret, rpm;
    //u8  devdata = 0;
    struct cpld_data *data = dev_get_drvdata(dev);
    int index = to_sensor_dev_attr_2(da)->index;

    ret = g3748cpld_read (data, (G3748_FAN_SPEED_REG + index - 1));
    if (ret < 0)
        return sprintf(buf, "ERR");

    rpm = (5000 * 60) / ret;

    return sprintf(buf, "%d\n", rpm);
}

/* G3748 FAN Control Register:
 *    Register value -- total 32 steps (0-31(0x1f)) to control system FAN PWM value (0% to 100%)
 *                      RPM -- 0x00:0rpm, 0x0F:8800rpm, 0x1F:17600rpm.
 *                      PWM -- 0x00:0%,   0x0F:50%,     0x1F:100%
 */
static ssize_t get_fan_pwm(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    //u8  devdata = 0;
    struct cpld_data *data = dev_get_drvdata(dev);
    int index = to_sensor_dev_attr_2(da)->index;
    u8  pwm;

    ret = g3748cpld_read (data, (G3748_FAN_CONTROL_REG + index - 1));
    if (ret < 0)
        return sprintf(buf, "cpld fan ctrl read error");

    ret = ret & 0x1F;
    if (ret == 0x1F) {
	pwm = 100;
    } else if (ret == 0) {
	pwm = 0;
    } else {
        //pwm = (u8)((ret * 100) / 31);
	pwm = gCurrentPwm;  /* Return what we set in previos PWM set value */
    }

    return sprintf(buf, "%d\n", pwm);
}

static ssize_t set_fan_pwm(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    //struct i2c_client *client = data->client;
    int index = to_sensor_dev_attr_2(da)->index;
    u8 temp;
    int error;
    u8 regVal = 0;

    error = kstrtou8(buf, 10, &temp);
    if (error)
        return error;
    if (temp > 100) {
	temp = 100;
    } else if (temp < 0) {
	temp = 0;
    }

    gCurrentPwm = temp;
    regVal = ((int)temp * 31) / 100;
    
    g3748cpld_write (data, (G3748_FAN_CONTROL_REG + index - 1), regVal);

    return count;
}

static ssize_t get_fan_target(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    //u8  devdata = 0;
    struct cpld_data *data = dev_get_drvdata(dev);
    int index = to_sensor_dev_attr_2(da)->index;
    int  rpm;

    ret = g3748cpld_read (data, (G3748_FAN_CONTROL_REG + index - 1));
    if (ret < 0)
        return sprintf(buf, "cpld fan ctrl read error");

    ret = ret & 0x1F;
    if (ret == 0x1F) {
        rpm = 17600;
    } else if (ret == 0) {
        rpm = 0;
    } else {
        //rpm = (ret * 17600) / 31;
	rpm = gCurrentPwm * 176;
    }

    return sprintf(buf, "%d\n", rpm);
}

static ssize_t set_fan_target(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    //struct i2c_client *client = data->client;
    int index = to_sensor_dev_attr_2(da)->index;
    int temp;
    int error;
    u8 regVal = 0;

    error = kstrtou32(buf, 10, (u32 *)&temp);
    if (error)
        return error;
    if (temp > 17600) {
        temp = 17600;
    } else if (temp < 0) {
        temp = 0;
    }
    gCurrentPwm= temp / 176;    /* % of PWM duty */
    regVal = (u8)((gCurrentPwm * 31) / 100);

    g3748cpld_write (data, (G3748_FAN_CONTROL_REG + index - 1), regVal);

    return count;
}

static ssize_t get_psu_frontled_reg(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = g3748cpld_read (data, G3748_PSU_FRONTLED_REG);
    if (ret < 0) {
        return sprintf(buf, "ERR: psu reg read");
    }

    return sprintf(buf, "%d\n", ret);
}

static ssize_t set_psu_frontled_reg(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 temp;
    int error;

    error = kstrtou8(buf, 10, &temp);
    if (error) {
        return error;
    }

    g3748cpld_write (data, G3748_PSU_FRONTLED_REG, temp);

    return count;
}

static ssize_t get_fan_sys_frontled_reg(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = g3748cpld_read (data, G3748_FAN_SYS_FRONTLED_REG);
    if (ret < 0) {
        return sprintf(buf, "ERR: fan_sys reg read");
    }

    return sprintf(buf, "%d\n", ret);
}

static ssize_t set_fan_sys_frontled_reg(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 temp;
    int error;

    error = kstrtou8(buf, 10, &temp);
    if (error) {
        return error;
    }

    g3748cpld_write (data, G3748_FAN_SYS_FRONTLED_REG, temp);

    return count;
}

static ssize_t get_wdt_timer_reg(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = g3748cpld_read (data, G3748_WDT_TIMER_REG);
    if (ret < 0) {
        return sprintf(buf, "ERR: WDT Timer reg read");
    }

    return sprintf(buf, "%d\n", ret);
}

static ssize_t set_wdt_timer_reg(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 temp;
    int error;

    error = kstrtou8(buf, 10, &temp);
    if (error) {
        return error;
    }

    g3748cpld_write (data, G3748_WDT_TIMER_REG, temp);

    return count;
}

static ssize_t get_wdt_ctrl_reg(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = g3748cpld_read (data, G3748_WDT_CTRL_REG);
    if (ret < 0) {
        return sprintf(buf, "ERR: WDT Ctrl reg read");
    }

    return sprintf(buf, "%d\n", ret);
}

static ssize_t set_wdt_ctrl_reg(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 temp;
    int error;

    error = kstrtou8(buf, 10, &temp);
    if (error) {
        return error;
    }

    g3748cpld_write (data, G3748_WDT_CTRL_REG, temp);

    return count;
}

static ssize_t get_wdt_intr_cnt_mask_reg(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = g3748cpld_read (data, G3748_WDT_INTR_CNT_MASK_REG);
    if (ret < 0) {
        return sprintf(buf, "ERR: WDT Intr Count Mask reg read");
    }

    return sprintf(buf, "%d\n", ret);
}

static ssize_t set_wdt_intr_cnt_mask_reg(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 temp;
    int error;

    error = kstrtou8(buf, 10, &temp);
    if (error) {
        return error;
    }

    g3748cpld_write (data, G3748_WDT_INTR_CNT_MASK_REG, temp);

    return count;
}

static ssize_t get_wdt_intr_flag_reg(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = g3748cpld_read (data, G3748_WDT_INTR_FLAG_REG);
    if (ret < 0) {
        return sprintf(buf, "ERR: WDT Intr Flag reg read");
    }

    return sprintf(buf, "%d\n", ret);
}

static ssize_t set_wdt_intr_flag_reg(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 temp;
    int error;

    error = kstrtou8(buf, 10, &temp);
    if (error) {
        return error;
    }

    g3748cpld_write (data, G3748_WDT_INTR_FLAG_REG, temp);

    return count;
}

static ssize_t get_wdt_timeout(struct device *dev, struct device_attribute *da, char *buf)
{
    return sprintf(buf, "%u\n", gTimeout);
}

static ssize_t set_wdt_timeout(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    /* struct cpld_data *data = dev_get_drvdata(dev); */
    u32 temp;
    int error;

    error = kstrtou32(buf, 10, &temp);
    if (error) {
        return error;
    }

    gTimeout = temp;

    return count;
}

static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, get_power_good, NULL, 1);        /* PSU1 voltage (power good) */  
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, get_power_good, NULL, 2);        /* PSU2 voltage (power good) */
static SENSOR_DEVICE_ATTR(in1_min, S_IRUGO | S_IWUSR, get_voltage_limit, set_voltage_limit, 11);  /* PSU1 voltage min */  
static SENSOR_DEVICE_ATTR(in2_min, S_IRUGO | S_IWUSR, get_voltage_limit, set_voltage_limit, 12);  /* PSU2 voltage min */
static SENSOR_DEVICE_ATTR(in1_max, S_IRUGO | S_IWUSR, get_voltage_limit, set_voltage_limit, 21);  /* PSU1 voltage max */  
static SENSOR_DEVICE_ATTR(in2_max, S_IRUGO | S_IWUSR, get_voltage_limit, set_voltage_limit, 22);  /* PSU2 voltage max */
static SENSOR_DEVICE_ATTR(in1_lcrit, S_IRUGO | S_IWUSR, get_voltage_limit, set_voltage_limit, 31); /* PSU1 voltage lcrit */  
static SENSOR_DEVICE_ATTR(in2_lcrit, S_IRUGO | S_IWUSR, get_voltage_limit, set_voltage_limit, 32); /* PSU2 voltage lcrit */
static SENSOR_DEVICE_ATTR(in1_crit, S_IRUGO | S_IWUSR, get_voltage_limit, set_voltage_limit, 41);  /* PSU1 voltage crit */  
static SENSOR_DEVICE_ATTR(in2_crit, S_IRUGO | S_IWUSR, get_voltage_limit, set_voltage_limit, 42);  /* PSU2 voltage crit */

static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR | S_IRUGO, get_fan_pwm, set_fan_pwm, 1);
static SENSOR_DEVICE_ATTR(pwm2, S_IWUSR | S_IRUGO, get_fan_pwm, set_fan_pwm, 2);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, get_fan_speed, NULL, 1);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, get_fan_speed, NULL, 2);
static SENSOR_DEVICE_ATTR(fan1_target, S_IRUGO | S_IWUSR, get_fan_target, set_fan_target, 1);
static SENSOR_DEVICE_ATTR(fan2_target, S_IRUGO | S_IWUSR, get_fan_target, set_fan_target, 2);
static SENSOR_DEVICE_ATTR(fan1_fault, S_IRUGO, get_fan_fault, NULL, 1);
static SENSOR_DEVICE_ATTR(fan2_fault, S_IRUGO, get_fan_fault, NULL, 2);

static struct attribute *cpld_hwmon_attrs[] = {
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in1_min.dev_attr.attr,
	&sensor_dev_attr_in2_min.dev_attr.attr,
	&sensor_dev_attr_in1_max.dev_attr.attr,
	&sensor_dev_attr_in2_max.dev_attr.attr,
	&sensor_dev_attr_in1_lcrit.dev_attr.attr,
	&sensor_dev_attr_in2_lcrit.dev_attr.attr,
	&sensor_dev_attr_in1_crit.dev_attr.attr,
	&sensor_dev_attr_in2_crit.dev_attr.attr,

	&sensor_dev_attr_pwm1.dev_attr.attr,
        &sensor_dev_attr_pwm2.dev_attr.attr,
        &sensor_dev_attr_fan1_input.dev_attr.attr,
        &sensor_dev_attr_fan2_input.dev_attr.attr,
        &sensor_dev_attr_fan1_target.dev_attr.attr,
        &sensor_dev_attr_fan2_target.dev_attr.attr,
        &sensor_dev_attr_fan1_fault.dev_attr.attr,
        &sensor_dev_attr_fan2_fault.dev_attr.attr,
        NULL
};

ATTRIBUTE_GROUPS(cpld_hwmon);
#if 0
static const struct attribute_group cpld_hwmon_group = {
	.attrs = cpld_hwmon_attrs,
};
#endif

static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, get_cpld_ver, NULL, 0x00);
static SENSOR_DEVICE_ATTR(psu_status, S_IRUGO, get_psu_status, NULL, 0);     /* PSU Status Register */
static SENSOR_DEVICE_ATTR(fan_status, S_IRUGO, get_fan_status, NULL, 0);     /* FAN Status Register */
static SENSOR_DEVICE_ATTR(fan_sys_frontled, S_IRUGO | S_IWUSR, get_fan_sys_frontled_reg, set_fan_sys_frontled_reg, 0);  /* FAN front panel LED */
static SENSOR_DEVICE_ATTR(psu_frontled, S_IRUGO | S_IWUSR, get_psu_frontled_reg, set_psu_frontled_reg, 0);          /* PSU front panel LED */

static SENSOR_DEVICE_ATTR(wdt_timer, S_IRUGO | S_IWUSR, get_wdt_timer_reg, set_wdt_timer_reg, 0);
static SENSOR_DEVICE_ATTR(wdt_ctrl, S_IRUGO | S_IWUSR, get_wdt_ctrl_reg, set_wdt_ctrl_reg, 0);
static SENSOR_DEVICE_ATTR(wdt_intr_cnt_mask, S_IRUGO | S_IWUSR, get_wdt_intr_cnt_mask_reg, set_wdt_intr_cnt_mask_reg, 0);
static SENSOR_DEVICE_ATTR(wdt_intr_flag, S_IRUGO | S_IWUSR, get_wdt_intr_flag_reg, set_wdt_intr_flag_reg, 0);
static SENSOR_DEVICE_ATTR(wdt_timeout, S_IRUGO | S_IWUSR, get_wdt_timeout, set_wdt_timeout, 0);

static struct attribute *cpld_sysfs_attrs[] = {
	&sensor_dev_attr_cpld_version.dev_attr.attr,
	&sensor_dev_attr_psu_status.dev_attr.attr,
	&sensor_dev_attr_fan_status.dev_attr.attr,
	&sensor_dev_attr_fan_sys_frontled.dev_attr.attr,
	&sensor_dev_attr_psu_frontled.dev_attr.attr,

	&sensor_dev_attr_wdt_timer.dev_attr.attr,
	&sensor_dev_attr_wdt_ctrl.dev_attr.attr,
	&sensor_dev_attr_wdt_intr_cnt_mask.dev_attr.attr,
	&sensor_dev_attr_wdt_intr_flag.dev_attr.attr,
	&sensor_dev_attr_wdt_timeout.dev_attr.attr,

	NULL
};

static const struct attribute_group cpld_sysfs_group = {
	.attrs = cpld_sysfs_attrs,
};

/* Return 0 if detection is successful, -ENODEV otherwise */
static int g3748cpld_detect(struct i2c_client *new_client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = new_client->adapter;
	int conf;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	/* Unused bits */
	conf = i2c_smbus_read_byte_data(new_client, 0);
	if (!conf)
		return -ENODEV;

	return 0;
}

static void g3748cpld_add_client (struct i2c_client *client)
{
        struct cpld_data *data;

	data = devm_kzalloc(&client->dev, sizeof(struct cpld_data), GFP_KERNEL);
        if (!data) {
		dev_info(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
                return;
	}

        data->client = client;
        i2c_set_clientdata(client, data);
        mutex_init(&data->update_lock);
}

static int g3748cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int status;
    struct cpld_data *data;
    int err;
   
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_info(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        return status;
    }
    
    dev_info(&client->dev, "chip found - CPLD\n");
    g3748cpld_add_client(client);
    
    /* Register sysfs hooks */
    //printk(KERN_INFO "g3748cpld_probe: Register sysfs hooks\n");
    status = sysfs_create_group(&client->dev.kobj, &cpld_sysfs_group);
    if (status) {
        printk(KERN_INFO "g3748cpld cannot create sysfs\n");
    }

    data = i2c_get_clientdata(client);
    //data->hwmon_dev = devm_hwmon_device_register_with_groups(&client->dev, client->name, data, cpld_hwmon_groups);
    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, data, cpld_hwmon_groups);

    if (IS_ERR(data->hwmon_dev)) {
        err = PTR_ERR(data->hwmon_dev);
	goto exit_remove_files;
    }

    initCpldDataReg(data);
    dev_info(&client->dev, "%s: sensor '%s'\n", dev_name(data->hwmon_dev), client->name);
    return 0;

exit_remove_files:
    sysfs_remove_group(&client->dev.kobj, &cpld_sysfs_group);
    return err;
}

static void g3748cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    //devm_hwmon_device_unregister(&client->dev);
    sysfs_remove_group(&client->dev.kobj, &cpld_sysfs_group);
}

static const struct i2c_device_id g3748cpld_id[] = {
	{ "g3748cpld", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, g3748cpld_id);

static struct i2c_driver g3748cpld_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "g3748cpld",
	},
	.probe		= g3748cpld_probe,
	.remove		= g3748cpld_remove,
	.id_table	= g3748cpld_id,
	.detect		= g3748cpld_detect,
	.address_list	= normal_i2c,
};

module_i2c_driver(g3748cpld_driver);

MODULE_AUTHOR("SMC <support@supermicro.com>");
MODULE_DESCRIPTION("SMC SSE-G3748 CPLD driver");
MODULE_LICENSE("GPL");
