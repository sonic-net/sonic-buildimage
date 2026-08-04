/*
 * A MCU driver connect to hwmon
 *
 * Copyright (C) 2018 Pegatron Corporation.
 * Peter5_Lin <Peter5_Lin@pegatroncorp.com.tw>
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
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>

#undef pega_DEBUG
/*#define pega_DEBUG*/
#ifdef pega_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif /* DEBUG */

#define FW_UPGRADE_COMMAND              0xA5
#define FAN_DISABLE_COMMAND             0x20
#define FAN_ENABLE_COMMAND              0x21
#define FAN_LED_SETTO_MANUAL_COMMAND    0x30
#define FAN_LED_SETTO_AUTO_COMMAND      0x31
#define FAN_LED_GREENON_COMMAND         0x40
#define FAN_LED_GREENOFF_COMMAND        0x41
#define FAN_LED_AMBERON_COMMAND         0x50
#define FAN_LED_AMBEROFF_COMMAND        0x51
#define SMART_FAN_ENABLE_BIT            0
#define SMART_FAN_SETTING_ENABLE_BIT    0
#define SA56004X_REMOTE_TEMP_ALERT_BIT  4
#define I2C_FANBOARD_TIMEOUT_BIT        0
#define ALERT_MODE_BIT                  0
#define GET_BIT(data, bit, value)       value = (data >> bit) & 0x1
#define SET_BIT(data, bit)              data |= (1 << bit)
#define CLEAR_BIT(data, bit)            data &= ~(1 << bit)

enum chips
{   
    mercedes3 = 0,
    cadillac,
    porsche,
};

enum fan_alert
{
    FAN_OUTER_RPM_OVER_ALERT_BIT = 0,
    FAN_OUTER_RPM_UNDER_ALERT_BIT, 
    FAN_INNER_RPM_OVER_ALERT_BIT,
    FAN_INNER_RPM_UNDER_ALERT_BIT,
    FAN_CONNECT_ALERT_BIT,
    FAN_DISCONNECT_ALERT_BIT,
};

enum fan_status
{
    FAN_ALERT_BIT = 2,
    FAN_LED_AMBER_BIT, 
    FAN_LED_GREEN_BIT,
    FAN_LED_AUTO_BIT,
    FAN_ENABLE_BIT,
    FAN_PRESENT_BIT,
};

enum hwmon_mcu_register
{ 
    MB_FW_UG_REG = 0,
    FB_FW_UG_REG,
    MB_HW_VER_REG,
    FB_HW_SKUVER_REG,
    MB_FW_VER_REG,
    FB_FW_VER_REG,

    FAN_PWM_REG = 16,

    SF_ENABLE_REG,
    SF_SETTING_ENABLE_REG,
    SF_DEVICE_REG,
    SF_UPDATE_REG,
    SF_TEMP_MAX_REG,
    SF_TEMP_MID_REG,
    SF_TEMP_MIN_REG,
    SF_PWM_MAX_REG,
    SF_PWM_MID_REG,
    SF_PWM_MIN_REG,

    FAN1_INNER_RPM_REG = 32,
    FAN2_INNER_RPM_REG,
    FAN3_INNER_RPM_REG,
    FAN4_INNER_RPM_REG,
    FAN5_INNER_RPM_REG,

    FAN1_OUTER_RPM_REG = 48,
    FAN2_OUTER_RPM_REG,
    FAN3_OUTER_RPM_REG,
    FAN4_OUTER_RPM_REG,
    FAN5_OUTER_RPM_REG,

    FAN1_STATUS_REG = 64,
    FAN2_STATUS_REG,
    FAN3_STATUS_REG,
    FAN4_STATUS_REG,
    FAN5_STATUS_REG,

    ADC_UNDER_VOL_ALERT_REG = 80,
    ADC_OVER_VOL_ALERT_REG,
    TS_OVER_TEMP_ALERT_REG,

    FAN1_ALERT_REG,
    FAN2_ALERT_REG,
    FAN3_ALERT_REG,
    FAN4_ALERT_REG,
    FAN5_ALERT_REG,

    I2C_BUS_ALERT_REG,
    ALERT_MODE_REG,

    MONITOR_ADC_VOLTAGE_REG = 96,

    LM_0X49_TEMP_REG = 112,
    LM_0X48_TEMP_REG,
    SA56004X_LOCAL_TEMP_REG,
    SA56004X_REMOTE_TEMP_REG,
    
};

static struct mutex pega_hwmon_mcu_lock;

static int pega_hwmon_mcu_read(struct i2c_client *client, u8 reg)
{
    int data = -EPERM;
    
    mutex_lock(&pega_hwmon_mcu_lock);

    data = i2c_smbus_read_word_data(client, reg);
    
    mutex_unlock(&pega_hwmon_mcu_lock);

    return data;
}

static int pega_hwmon_mcu_write(struct i2c_client *client, u8 reg, u8 val)
{
    int ret = -EIO;
    
    mutex_lock(&pega_hwmon_mcu_lock);

    ret = i2c_smbus_write_byte_data(client, reg, val);
    
    mutex_unlock(&pega_hwmon_mcu_lock);

    return ret;
}

static ssize_t mainBoardUpgrade(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = MB_FW_UG_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    if(val)
        pega_hwmon_mcu_write(client, reg, FW_UPGRADE_COMMAND);
    else
        pega_hwmon_mcu_write(client, reg, 0xff);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, FW_UPGRADE_COMMAND));

    return count;
}

static ssize_t fanBoardUpgrade(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = FB_FW_UG_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    if(val)
        pega_hwmon_mcu_write(client, reg, FW_UPGRADE_COMMAND);
    else
        pega_hwmon_mcu_write(client, reg, 0xff);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, FW_UPGRADE_COMMAND));

    return count;
}

static ssize_t get_MB_HW_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = MB_HW_VER_REG;

    data = pega_hwmon_mcu_read(client, reg);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    data &= 0x1f;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t get_FB_HW_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = FB_HW_SKUVER_REG;

    data = pega_hwmon_mcu_read(client, reg);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    data = (data >> 5) & 0x7;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t get_FB_boardId(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = FB_HW_SKUVER_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x1f;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t get_MB_FW_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, major_ver = 0, minor_ver = 0;
    u8 reg = MB_FW_VER_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    major_ver = (data >> 4) & 0xf;
    minor_ver = data & 0xf;

    return sprintf(buf, "%d.%d\n", major_ver, minor_ver);
}

static ssize_t get_FB_FW_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, major_ver = 0, minor_ver = 0;
    u8 reg = FB_FW_VER_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    major_ver = (data >> 4) & 0xf;
    minor_ver = data & 0xf;

    return sprintf(buf, "%d.%d\n", major_ver, minor_ver);
}

static ssize_t get_fan_PWM(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = FAN_PWM_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_fan_pwm(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = FAN_PWM_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, val: %x\r\n", __func__, client->addr, reg, val));
    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_enable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = SF_ENABLE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, SMART_FAN_ENABLE_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t set_smartFan_enable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_ENABLE_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    if(val)
        SET_BIT(data, SMART_FAN_ENABLE_BIT);
    else
        CLEAR_BIT(data, SMART_FAN_ENABLE_BIT);
    pega_hwmon_mcu_write(client, reg, data);

    return count;
}

static ssize_t get_smartFan_setting_enable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = SF_SETTING_ENABLE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, SMART_FAN_SETTING_ENABLE_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t set_smartFan_setting_enable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_SETTING_ENABLE_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    if(val)
        SET_BIT(data, SMART_FAN_SETTING_ENABLE_BIT);
    else
        CLEAR_BIT(data, SMART_FAN_SETTING_ENABLE_BIT);
    pega_hwmon_mcu_write(client, reg, data);

    return count;
}

static ssize_t get_smartFan_device(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_DEVICE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%x\n", data);
}

static ssize_t set_smartFan_device(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_DEVICE_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_update(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_UPDATE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_update(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_UPDATE_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_max_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_TEMP_MAX_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_max_temp(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_TEMP_MAX_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_mid_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_TEMP_MID_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_mid_temp(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_TEMP_MID_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_min_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_TEMP_MID_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_min_temp(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_TEMP_MID_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_max_pwm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MAX_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_max_pwm(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MAX_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_mid_pwm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MID_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_mid_pwm(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MID_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_min_pwm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MIN_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_min_pwm(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MIN_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_fan_inner_rpm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = FAN1_INNER_RPM_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t get_fan_outer_rpm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = FAN1_OUTER_RPM_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}

static ssize_t get_fan_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_PRESENT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_enable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_ENABLE_BIT, val);

    return sprintf(buf, "%d\n", val);
}


static ssize_t set_fan_enable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MID_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, val: %x\r\n", __func__, client->addr, reg, val));

    if(val)
        pega_hwmon_mcu_write(client, reg, FAN_ENABLE_COMMAND);
    else
        pega_hwmon_mcu_write(client, reg, FAN_DISABLE_COMMAND);

    return count;
}

static ssize_t get_fan_led_auto(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_LED_AUTO_BIT, val);

    return sprintf(buf, "%d\n", val);
}


static ssize_t set_fan_led_auto(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MID_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, val: %x\r\n", __func__, client->addr, reg, val));

    if(val)
        pega_hwmon_mcu_write(client, reg, FAN_LED_SETTO_AUTO_COMMAND);
    else
        pega_hwmon_mcu_write(client, reg, FAN_LED_SETTO_MANUAL_COMMAND);

    return count;
}

static ssize_t get_fan_led_green(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_LED_GREEN_BIT, val);

    return sprintf(buf, "%d\n", val);
}


static ssize_t set_fan_led_green(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MID_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, val: %x\r\n", __func__, client->addr, reg, val));

    if(val)
        pega_hwmon_mcu_write(client, reg, FAN_LED_GREENON_COMMAND);
    else
        pega_hwmon_mcu_write(client, reg, FAN_LED_GREENOFF_COMMAND);

    return count;
}

static ssize_t get_fan_led_amber(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_LED_AMBER_BIT, val);

    return sprintf(buf, "%d\n", val);
}


static ssize_t set_fan_led_amber(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = SF_PWM_MID_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, val: %x\r\n", __func__, client->addr, reg, val));

    if(val)
        pega_hwmon_mcu_write(client, reg, FAN_LED_AMBERON_COMMAND);
    else
        pega_hwmon_mcu_write(client, reg, FAN_LED_AMBEROFF_COMMAND);

    return count;
}

static ssize_t get_fan_status_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_adc_under_vol_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = ADC_UNDER_VOL_ALERT_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, attr->index, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_adc_over_vol_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = ADC_OVER_VOL_ALERT_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, attr->index, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_temp_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = TS_OVER_TEMP_ALERT_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, SA56004X_REMOTE_TEMP_ALERT_BIT + attr->index, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_outerRPMOver_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_OUTER_RPM_OVER_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_outerRPMUnder_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_OUTER_RPM_UNDER_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_innerRPMOver_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_INNER_RPM_OVER_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_innerRPMUnder_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_INNER_RPM_UNDER_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_connect_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_CONNECT_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_disconnect_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, FAN_DISCONNECT_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_i2c_timeout(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = I2C_BUS_ALERT_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, I2C_FANBOARD_TIMEOUT_BIT + attr->index, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_alert_mode(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0;
    u8 reg = ALERT_MODE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, ALERT_MODE_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t set_alert_mode(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0;
    u8 reg = ALERT_MODE_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, val: %x\r\n", __func__, client->addr, reg, val));

    if(val)
        SET_BIT(data, ALERT_MODE_BIT);
    else
        CLEAR_BIT(data, ALERT_MODE_BIT);
    pega_hwmon_mcu_write(client, reg, data);

    return count;
}

static ssize_t get_adc_vol(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0, reg = MONITOR_ADC_VOLTAGE_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d.%02d\n", data/1000, (data/10)%12);
}

static ssize_t get_hwmon_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev); 
    u8 data = 0;  
    u8 reg = LM_0X49_TEMP_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%d\n", data);
}
#define SET_FAN_ATTR(_num) \
    static SENSOR_DEVICE_ATTR(fan##_num##_inner_rpm,  S_IRUGO, get_fan_inner_rpm, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_outer_rpm,  S_IRUGO, get_fan_outer_rpm, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_present,  S_IRUGO, get_fan_present, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_enable,  S_IRUGO | S_IWUSR, get_fan_enable, set_fan_enable, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_led_auto,  S_IRUGO | S_IWUSR, get_fan_led_auto, set_fan_led_auto, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_led_green,  S_IRUGO | S_IWUSR, get_fan_led_green, set_fan_led_green, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_led_amber,  S_IRUGO | S_IWUSR, get_fan_led_amber, set_fan_led_amber, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_status_alert,  S_IRUGO, get_fan_status_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_outerRPMOver_alert,  S_IRUGO, get_fan_outerRPMOver_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_outerRPMUnder_alert,  S_IRUGO, get_fan_outerRPMUnder_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_innerRPMOver_alert,  S_IRUGO, get_fan_innerRPMOver_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_innerRPMUnder_alert,  S_IRUGO, get_fan_innerRPMUnder_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_connect_alert,  S_IRUGO, get_fan_connect_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_disconnect_alert,  S_IRUGO, get_fan_disconnect_alert, NULL, _num-1)

SET_FAN_ATTR(1);SET_FAN_ATTR(2);SET_FAN_ATTR(3);SET_FAN_ATTR(4);SET_FAN_ATTR(5);

#define SET_ADC_ATTR(_num) \
    static SENSOR_DEVICE_ATTR(ADC##_num##_under_alert,  S_IRUGO, get_adc_under_vol_alert, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(ADC##_num##_over_alert,  S_IRUGO, get_adc_over_vol_alert, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(ADC##_num##_vol,  S_IRUGO, get_adc_vol, NULL, 8-_num)

SET_ADC_ATTR(1);SET_ADC_ATTR(2);SET_ADC_ATTR(3);SET_ADC_ATTR(4);SET_ADC_ATTR(5);SET_ADC_ATTR(6);SET_ADC_ATTR(7);SET_ADC_ATTR(8);

static SENSOR_DEVICE_ATTR(mb_fw_upgrade,  S_IWUSR, NULL, mainBoardUpgrade, 0);
static SENSOR_DEVICE_ATTR(fb_fw_upgrade,  S_IWUSR, NULL, fanBoardUpgrade, 0);
static SENSOR_DEVICE_ATTR(mb_hw_version,  S_IRUGO, get_MB_HW_version, NULL, 0);
static SENSOR_DEVICE_ATTR(fb_hw_version,  S_IRUGO, get_FB_HW_version, NULL, 0);
static SENSOR_DEVICE_ATTR(fb_board_id,  S_IRUGO, get_FB_boardId, NULL, 0);
static SENSOR_DEVICE_ATTR(mb_fw_version,  S_IRUGO, get_MB_FW_version, NULL, 0);
static SENSOR_DEVICE_ATTR(fb_fw_version,  S_IRUGO, get_FB_FW_version, NULL, 0);
static SENSOR_DEVICE_ATTR(fan_pwm,  S_IRUGO | S_IWUSR, get_fan_PWM, set_fan_pwm, 0);

static SENSOR_DEVICE_ATTR(smartFan_enable,  S_IRUGO | S_IWUSR, get_smartFan_enable, set_smartFan_enable, 0);
static SENSOR_DEVICE_ATTR(smartFan_setting_enable,  S_IRUGO | S_IWUSR, get_smartFan_setting_enable, set_smartFan_setting_enable, 0);
static SENSOR_DEVICE_ATTR(smartFan_device,  S_IRUGO | S_IWUSR, get_smartFan_device, set_smartFan_device, 0);
static SENSOR_DEVICE_ATTR(smartFan_update,  S_IRUGO | S_IWUSR, get_smartFan_update, set_smartFan_update, 0);
static SENSOR_DEVICE_ATTR(smartFan_max_temp,  S_IRUGO | S_IWUSR, get_smartFan_max_temp, set_smartFan_max_temp, 0);
static SENSOR_DEVICE_ATTR(smartFan_mid_temp,  S_IRUGO | S_IWUSR, get_smartFan_mid_temp, set_smartFan_mid_temp, 0);
static SENSOR_DEVICE_ATTR(smartFan_min_temp,  S_IRUGO | S_IWUSR, get_smartFan_min_temp, set_smartFan_min_temp, 0);
static SENSOR_DEVICE_ATTR(smartFan_max_pwm,  S_IRUGO | S_IWUSR, get_smartFan_max_pwm, set_smartFan_max_pwm, 0);
static SENSOR_DEVICE_ATTR(smartFan_mid_pwm,  S_IRUGO | S_IWUSR, get_smartFan_mid_pwm, set_smartFan_mid_pwm, 0);
static SENSOR_DEVICE_ATTR(smartFan_min_pwm,  S_IRUGO | S_IWUSR, get_smartFan_min_pwm, set_smartFan_min_pwm, 0);

static SENSOR_DEVICE_ATTR(lm75_49_temp_alert,  S_IRUGO, get_temp_alert, NULL, 3);
static SENSOR_DEVICE_ATTR(lm75_48_temp_alert,  S_IRUGO, get_temp_alert, NULL, 2);
static SENSOR_DEVICE_ATTR(SA56004X_Ltemp_alert,  S_IRUGO, get_temp_alert, NULL, 1);
static SENSOR_DEVICE_ATTR(SA56004X_Rtemp_alert,  S_IRUGO, get_temp_alert, NULL, 0);

static SENSOR_DEVICE_ATTR(i2c_fb_timeout,  S_IRUGO, get_i2c_timeout, NULL, 0);
static SENSOR_DEVICE_ATTR(i2c_remote_timeout,  S_IRUGO, get_i2c_timeout, NULL, 1);
static SENSOR_DEVICE_ATTR(i2c_local_timeout,  S_IRUGO, get_i2c_timeout, NULL, 2);
static SENSOR_DEVICE_ATTR(i2c_lm75_48_timeout,  S_IRUGO, get_i2c_timeout, NULL, 3);
static SENSOR_DEVICE_ATTR(i2c_lm75_49_timeout,  S_IRUGO, get_i2c_timeout, NULL, 4);
static SENSOR_DEVICE_ATTR(alert_mode,  S_IRUGO | S_IWUSR, get_alert_mode, set_alert_mode, 0);

static SENSOR_DEVICE_ATTR(lm75_49_temp,  S_IRUGO, get_hwmon_temp, NULL, 0);
static SENSOR_DEVICE_ATTR(lm75_48_temp,  S_IRUGO, get_hwmon_temp, NULL, 1);
static SENSOR_DEVICE_ATTR(SA56004_local_temp,  S_IRUGO, get_hwmon_temp, NULL, 2);
static SENSOR_DEVICE_ATTR(SA56004_remote_temp,  S_IRUGO, get_hwmon_temp, NULL, 3);

static struct attribute *pega_hwmon_mcu_attributes[] = {
    &sensor_dev_attr_mb_fw_upgrade.dev_attr.attr,
    &sensor_dev_attr_fb_fw_upgrade.dev_attr.attr,
    &sensor_dev_attr_mb_hw_version.dev_attr.attr,
    &sensor_dev_attr_fb_hw_version.dev_attr.attr,
    &sensor_dev_attr_fb_board_id.dev_attr.attr,
    &sensor_dev_attr_mb_fw_version.dev_attr.attr,
    &sensor_dev_attr_fb_fw_version.dev_attr.attr,
    &sensor_dev_attr_fan_pwm.dev_attr.attr,

    &sensor_dev_attr_smartFan_enable.dev_attr.attr,
    &sensor_dev_attr_smartFan_setting_enable.dev_attr.attr,
    &sensor_dev_attr_smartFan_device.dev_attr.attr,
    &sensor_dev_attr_smartFan_update.dev_attr.attr,
    &sensor_dev_attr_smartFan_max_temp.dev_attr.attr,
    &sensor_dev_attr_smartFan_mid_temp.dev_attr.attr,
    &sensor_dev_attr_smartFan_min_temp.dev_attr.attr,
    &sensor_dev_attr_smartFan_max_pwm.dev_attr.attr,
    &sensor_dev_attr_smartFan_mid_pwm.dev_attr.attr,
    &sensor_dev_attr_smartFan_min_pwm.dev_attr.attr,

    &sensor_dev_attr_fan1_inner_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_inner_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_inner_rpm.dev_attr.attr,
    &sensor_dev_attr_fan4_inner_rpm.dev_attr.attr,
    &sensor_dev_attr_fan5_inner_rpm.dev_attr.attr,

    &sensor_dev_attr_fan1_outer_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_outer_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_outer_rpm.dev_attr.attr,
    &sensor_dev_attr_fan4_outer_rpm.dev_attr.attr,
    &sensor_dev_attr_fan5_outer_rpm.dev_attr.attr,

    &sensor_dev_attr_fan1_present.dev_attr.attr,
    &sensor_dev_attr_fan2_present.dev_attr.attr,
    &sensor_dev_attr_fan3_present.dev_attr.attr,
    &sensor_dev_attr_fan4_present.dev_attr.attr,
    &sensor_dev_attr_fan5_present.dev_attr.attr,

    &sensor_dev_attr_fan1_enable.dev_attr.attr,
    &sensor_dev_attr_fan2_enable.dev_attr.attr,
    &sensor_dev_attr_fan3_enable.dev_attr.attr,
    &sensor_dev_attr_fan4_enable.dev_attr.attr,
    &sensor_dev_attr_fan5_enable.dev_attr.attr,

    &sensor_dev_attr_fan1_led_auto.dev_attr.attr,
    &sensor_dev_attr_fan2_led_auto.dev_attr.attr,
    &sensor_dev_attr_fan3_led_auto.dev_attr.attr,
    &sensor_dev_attr_fan4_led_auto.dev_attr.attr,
    &sensor_dev_attr_fan5_led_auto.dev_attr.attr,

    &sensor_dev_attr_fan1_led_green.dev_attr.attr,
    &sensor_dev_attr_fan2_led_green.dev_attr.attr,
    &sensor_dev_attr_fan3_led_green.dev_attr.attr,
    &sensor_dev_attr_fan4_led_green.dev_attr.attr,
    &sensor_dev_attr_fan5_led_green.dev_attr.attr,

    &sensor_dev_attr_fan1_led_amber.dev_attr.attr,
    &sensor_dev_attr_fan2_led_amber.dev_attr.attr,
    &sensor_dev_attr_fan3_led_amber.dev_attr.attr,
    &sensor_dev_attr_fan4_led_amber.dev_attr.attr,
    &sensor_dev_attr_fan5_led_amber.dev_attr.attr,

    &sensor_dev_attr_fan1_status_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_status_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_status_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_status_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_status_alert.dev_attr.attr,

    &sensor_dev_attr_ADC1_under_alert.dev_attr.attr,
    &sensor_dev_attr_ADC2_under_alert.dev_attr.attr,
    &sensor_dev_attr_ADC3_under_alert.dev_attr.attr,
    &sensor_dev_attr_ADC4_under_alert.dev_attr.attr,
    &sensor_dev_attr_ADC5_under_alert.dev_attr.attr,
    &sensor_dev_attr_ADC6_under_alert.dev_attr.attr,
    &sensor_dev_attr_ADC7_under_alert.dev_attr.attr,
    &sensor_dev_attr_ADC8_under_alert.dev_attr.attr,

    &sensor_dev_attr_ADC1_over_alert.dev_attr.attr,
    &sensor_dev_attr_ADC2_over_alert.dev_attr.attr,
    &sensor_dev_attr_ADC3_over_alert.dev_attr.attr,
    &sensor_dev_attr_ADC4_over_alert.dev_attr.attr,
    &sensor_dev_attr_ADC5_over_alert.dev_attr.attr,
    &sensor_dev_attr_ADC6_over_alert.dev_attr.attr,
    &sensor_dev_attr_ADC7_over_alert.dev_attr.attr,
    &sensor_dev_attr_ADC8_over_alert.dev_attr.attr,

    &sensor_dev_attr_lm75_48_temp_alert.dev_attr.attr,
    &sensor_dev_attr_lm75_49_temp_alert.dev_attr.attr,
    &sensor_dev_attr_SA56004X_Ltemp_alert.dev_attr.attr,
    &sensor_dev_attr_SA56004X_Rtemp_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_outerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_outerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_outerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_outerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_outerRPMOver_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_outerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_outerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_outerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_outerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_outerRPMUnder_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_innerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_innerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_innerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_innerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_innerRPMOver_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_innerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_innerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_innerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_innerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_innerRPMUnder_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_connect_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_connect_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_connect_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_connect_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_connect_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_disconnect_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_disconnect_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_disconnect_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_disconnect_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_disconnect_alert.dev_attr.attr,

    &sensor_dev_attr_i2c_fb_timeout.dev_attr.attr,
    &sensor_dev_attr_i2c_remote_timeout.dev_attr.attr,
    &sensor_dev_attr_i2c_local_timeout.dev_attr.attr,
    &sensor_dev_attr_i2c_lm75_48_timeout.dev_attr.attr,
    &sensor_dev_attr_i2c_lm75_49_timeout.dev_attr.attr,
    &sensor_dev_attr_alert_mode.dev_attr.attr,

    &sensor_dev_attr_ADC1_vol.dev_attr.attr,
    &sensor_dev_attr_ADC2_vol.dev_attr.attr,
    &sensor_dev_attr_ADC3_vol.dev_attr.attr,
    &sensor_dev_attr_ADC4_vol.dev_attr.attr,
    &sensor_dev_attr_ADC5_vol.dev_attr.attr,
    &sensor_dev_attr_ADC6_vol.dev_attr.attr,
    &sensor_dev_attr_ADC7_vol.dev_attr.attr,
    &sensor_dev_attr_ADC8_vol.dev_attr.attr,

    &sensor_dev_attr_lm75_49_temp.dev_attr.attr,
    &sensor_dev_attr_lm75_48_temp.dev_attr.attr,
    &sensor_dev_attr_SA56004_local_temp.dev_attr.attr,
    &sensor_dev_attr_SA56004_remote_temp.dev_attr.attr,

    NULL
};

static const struct attribute_group pega_hwmon_mcu_group = { .attrs = pega_hwmon_mcu_attributes};

static int pega_hwmon_mcu_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{  
    int status;

    status = sysfs_create_group(&client->dev.kobj, &pega_hwmon_mcu_group);

    if (status) {
        goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    
    return 0; 

exit:
    return status;
}

static int pega_hwmon_mcu_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &pega_hwmon_mcu_group);
    return 0;
}

static const struct i2c_device_id pega_hwmon_mcu_id[] = {
    { "porsche_hwmon_mcu", porsche },
    {}
};
MODULE_DEVICE_TABLE(i2c, pega_hwmon_mcu_id);

static struct i2c_driver pega_hwmon_mcu_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "pegatron_hwmon_mcu",
    },
    .probe      = pega_hwmon_mcu_probe,
    .remove     = pega_hwmon_mcu_remove,
    .id_table   = pega_hwmon_mcu_id,
};

static int __init pega_hwmon_mcu_init(void)
{
    mutex_init(&pega_hwmon_mcu_lock);

    return i2c_add_driver(&pega_hwmon_mcu_driver);
}

static void __exit pega_hwmon_mcu_exit(void)
{
    i2c_del_driver(&pega_hwmon_mcu_driver);
}

MODULE_AUTHOR("Peter5 Lin <Peter5_Lin@pegatroncorp.com.tw>");
MODULE_DESCRIPTION("pega_hwmon_mcu driver");
MODULE_LICENSE("GPL");

module_init(pega_hwmon_mcu_init);
module_exit(pega_hwmon_mcu_exit);