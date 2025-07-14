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
#include "pegatron_pub.h"

#undef pega_DEBUG
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
#define FAN_LED_GREENOFF_COMMAND        0x40
#define FAN_LED_GREENON_COMMAND         0x41
#define FAN_LED_AMBEROFF_COMMAND        0x50
#define FAN_LED_AMBERON_COMMAND         0x51
#define SMART_FAN_ENABLE_BIT            0
#define SMART_FAN_SETTING_ENABLE_BIT    0
#define SA56004X_REMOTE_TEMP_ALERT_BIT  4
#define I2C_FANBOARD_TIMEOUT_BIT        0
#define ALERT_MODE_BIT                  0
#define GET_BIT(data, bit, value)       value = (data >> bit) & 0x1
#define SET_BIT(data, bit)              data |= (1 << bit)
#define CLEAR_BIT(data, bit)            data &= ~(1 << bit)

enum fan_alert
{
    FAN_WRONG_AIRFLOW = 0,
    FAN_OUTER_RPM_OVER_ALERT_BIT,
    FAN_OUTER_RPM_UNDER_ALERT_BIT,
    FAN_OUTER_RPM_ZERO_ALERT_BIT,
    FAN_INNER_RPM_OVER_ALERT_BIT,
    FAN_INNER_RPM_UNDER_ALERT_BIT,
    FAN_INNER_RPM_ZERO_ALERT_BIT,
    FAN_NOTCONNECT_ALERT_BIT,
};

enum fan_status
{
    SYSTEM_AIRFLOW_DIR,
    FAN_STATUS_AIRFLOW,
    FAN_ALERT_BIT,
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
    FAN6_INNER_RPM_REG,

    FAN1_OUTER_RPM_REG = 48,
    FAN2_OUTER_RPM_REG,
    FAN3_OUTER_RPM_REG,
    FAN4_OUTER_RPM_REG,
    FAN5_OUTER_RPM_REG,
    FAN6_OUTER_RPM_REG,

    FAN1_STATUS_REG = 64,
    FAN2_STATUS_REG,
    FAN3_STATUS_REG,
    FAN4_STATUS_REG,
    FAN5_STATUS_REG,
    FAN6_STATUS_REG,

    ADC_UNDER_VOL_ALERT_REG = 80,
    ADC_OVER_VOL_ALERT_REG,
    TS_OVER_TEMP_ALERT_REG,

    FAN1_ALERT_REG,
    FAN2_ALERT_REG,
    FAN3_ALERT_REG,
    FAN4_ALERT_REG,
    FAN5_ALERT_REG,
    FAN6_ALERT_REG,

    I2C_BUS_ALERT_REG,
    ALERT_MODE_REG,

    MONITOR_ADC_VOLTAGE_REG = 96,

    LM_0X48_TEMP_REG = 112,
    LM_0X49_TEMP_REG,
    LM_0X4A_TEMP_REG,
    SA56004X_LOCAL_TEMP_REG,
    SA56004X_REMOTE_TEMP_REG,

};

#define HWMON_MCU_TEMP_NUM 5
struct hwmon_mcu_data {
	struct i2c_client	*client;
    int temp_max[HWMON_MCU_TEMP_NUM];
    int temp_crit[HWMON_MCU_TEMP_NUM];
    int temp_min[HWMON_MCU_TEMP_NUM];
};
int hwmon_temp_threshold_max[HWMON_MCU_TEMP_NUM] = {56, 65, 60, 65, 65};
int hwmon_temp_threshold_crit[HWMON_MCU_TEMP_NUM] = {66, 75, 70, 75, 75};
int hwmon_temp_threshold_min[HWMON_MCU_TEMP_NUM] = {4, 10, 1, 5, 4};


static struct mutex pega_hwmon_mcu_lock;

static uint loglevel = LOG_INFO | LOG_WARNING | LOG_ERR;
static char debug[MAX_DEBUG_INFO_LEN] = "debug info:                     \n\
we should set smartFan_enable to 1 instead of setting the fan ratio.   \n";

#define MAX_FAN_NUM 6
#define MOTOR_NUM_PER_FAN   1

static LIST_HEAD(mcu_client_list);
static struct mutex  list_lock;

struct mcu_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

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
int pega_mcu_read(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct mcu_client_node *mcu_mode = NULL;
    int data = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &mcu_client_list)
    {
        mcu_mode = list_entry(list_node, struct mcu_client_node, list);

        if (mcu_mode->client->addr == addr) {
            //data = i2c_smbus_read_byte_data(mcu_mode->client, reg);
            data = pega_hwmon_mcu_read(mcu_mode->client, reg);
            pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", addr, reg, data);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return data;
}
EXPORT_SYMBOL(pega_mcu_read);


int pega_mcu_write(unsigned short addr, u8 reg, u8 val)
{
    struct list_head   *list_node = NULL;
    struct mcu_client_node *mcu_mode = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &mcu_client_list)
    {
        mcu_mode = list_entry(list_node, struct mcu_client_node, list);

        if (mcu_mode->client->addr == addr) {
            //ret = i2c_smbus_write_byte_data(mcu_mode->client, reg, val);
            ret = pega_hwmon_mcu_write(mcu_mode->client, reg, val);
            pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", addr, reg, val);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(pega_mcu_write);

static ssize_t mainBoardUpgrade(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
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

    pega_print(DEBUG,"addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, FW_UPGRADE_COMMAND);

    return count;
}

static ssize_t fanBoardUpgrade(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
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

    pega_print(DEBUG,"addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, FW_UPGRADE_COMMAND);

    return count;
}

static ssize_t get_MB_HW_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = MB_HW_VER_REG;

    data = pega_hwmon_mcu_read(client, reg);

    pega_print(DEBUG,"addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, FW_UPGRADE_COMMAND);

    data &= 0x1f;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t get_FB_HW_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = FB_HW_SKUVER_REG;

    data = pega_hwmon_mcu_read(client, reg);

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    data = (data >> 5) & 0x7;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t get_FB_boardId(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = FB_HW_SKUVER_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    data &= 0x1f;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t get_MB_FW_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, major_ver = 0, minor_ver = 0;
    u8 reg = MB_FW_VER_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    major_ver = (data >> 4) & 0xf;
    minor_ver = data & 0xf;

    return sprintf(buf, "%d.%d\n", major_ver, minor_ver);
}

static ssize_t get_FB_FW_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, major_ver = 0, minor_ver = 0;
    u8 reg = FB_FW_VER_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    major_ver = (data >> 4) & 0xf;
    minor_ver = data & 0xf;

    return sprintf(buf, "%d.%d\n", major_ver, minor_ver);
}

static ssize_t get_pwr_down(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = 0xb2;
    u8 data = 0;
    u8 pwr_down = 0;

    pega_hwmon_mcu_write(client, 0xB0, 0x0);
    pega_hwmon_mcu_write(client, 0xB1, 0xa0);
    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    pwr_down = ((data == 0xff)?0:1);

    return sprintf(buf, "0x%02x\n", pwr_down);
}

static ssize_t get_fan_PWM(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = FAN_PWM_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_fan_pwm(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = FAN_PWM_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);
    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_enable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = SF_ENABLE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, SMART_FAN_ENABLE_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t set_smartFan_enable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_ENABLE_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

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
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = SF_SETTING_ENABLE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, SMART_FAN_SETTING_ENABLE_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t set_smartFan_setting_enable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_SETTING_ENABLE_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

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
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_DEVICE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%x\n", data);
}

static ssize_t set_smartFan_device(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = SF_DEVICE_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_update(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_UPDATE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_update(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = SF_UPDATE_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_max_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_TEMP_MAX_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_max_temp(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = SF_TEMP_MAX_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_mid_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_TEMP_MID_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_mid_temp(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = SF_TEMP_MID_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_min_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_TEMP_MIN_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_min_temp(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = SF_TEMP_MIN_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_max_pwm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_PWM_MAX_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_max_pwm(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = SF_PWM_MAX_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_mid_pwm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_PWM_MID_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_mid_pwm(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = SF_PWM_MID_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_smartFan_min_pwm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = SF_PWM_MIN_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_smartFan_min_pwm(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = SF_PWM_MIN_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

    pega_hwmon_mcu_write(client, reg, val);

    return count;
}

static ssize_t get_fan_inner_rpm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u16 data = 0;
    u8 reg = FAN1_INNER_RPM_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t get_fan_outer_rpm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u16 data = 0;
    u8 reg = FAN1_OUTER_RPM_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data);
}

static ssize_t get_fan_airflow_dir(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, SYSTEM_AIRFLOW_DIR, val);

    return sprintf(buf, "%d\n", val);
}


#define FAN_ABSENT 1
#define FAN_POWER_OK 0

#define KWAI_FAN_ABSENT 0
#define KWAI_FAN_PRESENT_NORMAL 1
#define KWAI_FAN_PRESENT_ABNORMAL 2

static ssize_t get_fan_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0,present = 0,alarm = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_PRESENT_BIT, present);
    GET_BIT(data, FAN_ALERT_BIT, alarm);


    if(present == FAN_ABSENT)
    {
        val = KWAI_FAN_ABSENT;
    }
    else
    {
        if(alarm == FAN_POWER_OK)
        {
            val = KWAI_FAN_PRESENT_NORMAL;
        }
        else
        {
            val = KWAI_FAN_PRESENT_ABNORMAL;
        }
    }

    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t get_fan_enable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_ENABLE_BIT, val);

    return sprintf(buf, "%d\n", val);
}


static ssize_t set_fan_enable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = FAN1_STATUS_REG + attr->index;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

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
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_LED_AUTO_BIT, val);

    return sprintf(buf, "%d\n", val);
}


static ssize_t set_fan_led_auto(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = FAN1_STATUS_REG + attr->index;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

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
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_LED_GREEN_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_led_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    //Kwai's requirement
    if(data & (1 << FAN_LED_GREEN_BIT))
    {
        val = 1;
    }
    else if(data & (1 << FAN_LED_AMBER_BIT))
    {
        val = 3;
    }

    return sprintf(buf, "%d\n", val);
}

static ssize_t set_fan_led_green(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = FAN1_STATUS_REG + attr->index;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

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
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_LED_AMBER_BIT, val);

    return sprintf(buf, "%d\n", val);
}


static ssize_t set_fan_led_amber(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 reg = FAN1_STATUS_REG + attr->index;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

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
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_STATUS_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_adc_under_vol_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = ADC_UNDER_VOL_ALERT_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, attr->index, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_adc_over_vol_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = ADC_OVER_VOL_ALERT_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, attr->index, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_temp_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = TS_OVER_TEMP_ALERT_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, attr->index, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_wrong_airflow_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_WRONG_AIRFLOW, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_outerRPMOver_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_OUTER_RPM_OVER_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_outerRPMUnder_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_OUTER_RPM_UNDER_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_outerRPMZero_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_OUTER_RPM_ZERO_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_innerRPMOver_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_INNER_RPM_OVER_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_innerRPMUnder_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_INNER_RPM_UNDER_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_innerRPMZero_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_INNER_RPM_ZERO_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_fan_notconnect_alert(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = FAN1_ALERT_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, FAN_NOTCONNECT_ALERT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_i2c_timeout(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = I2C_BUS_ALERT_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, (I2C_FANBOARD_TIMEOUT_BIT + attr->index), val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_alert_mode(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0, val = 0;
    u8 reg = ALERT_MODE_REG;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, ALERT_MODE_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t set_alert_mode(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = ALERT_MODE_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %lx\r\n", client->addr, reg, val);

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
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u16 data = 0, reg = MONITOR_ADC_VOLTAGE_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d.%02d\n", data/1000, data%1000);
}

static ssize_t get_hwmon_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data = 0;
    u8 reg = LM_0X48_TEMP_REG + attr->index;

    data = pega_hwmon_mcu_read(client, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%d\n", data*1000);
}
static ssize_t get_label(struct device *dev,
				struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	return sprintf(buf, "temp%d\n", attr->index);
}

static ssize_t get_hwmon_temp_min(struct device *dev,
				struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", drv_data->temp_min[attr->index]);
}

static ssize_t get_hwmon_temp_max(struct device *dev,
				struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", drv_data->temp_max[attr->index]);
}
static ssize_t set_hwmon_temp_max(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

	drv_data->temp_max[attr->index] = val;
    return count;
}
static ssize_t get_hwmon_temp_crit(struct device *dev,
				struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", drv_data->temp_crit[attr->index]);
}
static ssize_t set_hwmon_temp_crit(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct hwmon_mcu_data *drv_data = dev_get_drvdata(dev);
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    drv_data->temp_crit[attr->index] = val;
    return count;
}

static ssize_t get_total_fan_num(struct device *dev,
            struct device_attribute *da, char *buf)
{
    return sprintf(buf, "%d\n", MAX_FAN_NUM);
}
static ssize_t get_motor_num(struct device *dev,
            struct device_attribute *da, char *buf)
{
    return sprintf(buf, "%d\n", MOTOR_NUM_PER_FAN);
}
static ssize_t get_fan_model_name(struct device *dev,
            struct device_attribute *da, char *buf)
{
    return sprintf(buf, "XG40561BX-1Q313-S9H");
}
static ssize_t get_fan_serial_num(struct device *dev,
            struct device_attribute *da, char *buf)
{
    return sprintf(buf, "N/A");
}
static ssize_t get_fan_vendor(struct device *dev,
            struct device_attribute *da, char *buf)
{
    return sprintf(buf, "SUNON");
}
static ssize_t get_fan_part_num(struct device *dev,
            struct device_attribute *da, char *buf)
{
    return sprintf(buf, "N/A");
}
static ssize_t get_fan_hard_version(struct device *dev,
            struct device_attribute *da, char *buf)
{
    return sprintf(buf, "1.0");
}
static ssize_t get_fan_motor_speed_target(struct device *dev,
            struct device_attribute *da, char *buf)
{
    return sprintf(buf, "27500");
}
static ssize_t get_fan_motor_speed_tolerance(struct device *dev,
            struct device_attribute *da, char *buf)
{
    //10 percentage
    return sprintf(buf, "2750");
}
#define SET_FAN_ATTR(_num) \
    static SENSOR_DEVICE_ATTR(fan##_num##_inner_rpm,  S_IRUGO, get_fan_inner_rpm, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_outer_rpm,  S_IRUGO, get_fan_outer_rpm, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_present,  S_IRUGO, get_fan_present, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_enable,  S_IRUGO | S_IWUSR, get_fan_enable, set_fan_enable, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_led_auto,  S_IRUGO | S_IWUSR, get_fan_led_auto, set_fan_led_auto, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_led_status,  S_IRUGO, get_fan_led_status, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_led_green,  S_IRUGO | S_IWUSR, get_fan_led_green, set_fan_led_green, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_led_amber,  S_IRUGO | S_IWUSR, get_fan_led_amber, set_fan_led_amber, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_status_alert,  S_IRUGO, get_fan_status_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_wrongAirflow_alert,  S_IRUGO, get_fan_wrong_airflow_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_outerRPMOver_alert,  S_IRUGO, get_fan_outerRPMOver_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_outerRPMUnder_alert,  S_IRUGO, get_fan_outerRPMUnder_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_outerRPMZero_alert,  S_IRUGO, get_fan_outerRPMZero_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_innerRPMOver_alert,  S_IRUGO, get_fan_innerRPMOver_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_innerRPMUnder_alert,  S_IRUGO, get_fan_innerRPMUnder_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_innerRPMZero_alert,  S_IRUGO, get_fan_innerRPMZero_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_notconnect_alert,  S_IRUGO, get_fan_notconnect_alert, NULL, _num-1);  \
    static SENSOR_DEVICE_ATTR(fan##_num##_airflow_dir,  S_IRUGO, get_fan_airflow_dir, NULL, _num-1);\
    static SENSOR_DEVICE_ATTR(fan##_num##_motor_num,  S_IRUGO, get_motor_num, NULL, _num-1);\
    static SENSOR_DEVICE_ATTR(fan##_num##_model_name,  S_IRUGO, get_fan_model_name, NULL, _num-1);\
    static SENSOR_DEVICE_ATTR(fan##_num##_serial_num,  S_IRUGO, get_fan_serial_num, NULL, _num-1);\
    static SENSOR_DEVICE_ATTR(fan##_num##_vendor,  S_IRUGO, get_fan_vendor, NULL, _num-1);\
    static SENSOR_DEVICE_ATTR(fan##_num##_part_num,  S_IRUGO, get_fan_part_num, NULL, _num-1);\
    static SENSOR_DEVICE_ATTR(fan##_num##_hard_version,  S_IRUGO, get_fan_hard_version, NULL, _num-1);\
    static SENSOR_DEVICE_ATTR(fan##_num##_speed_target,  S_IRUGO, get_fan_motor_speed_target, NULL, _num-1);\
    static SENSOR_DEVICE_ATTR(fan##_num##_speed_tolerance,  S_IRUGO, get_fan_motor_speed_tolerance, NULL, _num-1);



SET_FAN_ATTR(1);SET_FAN_ATTR(2);SET_FAN_ATTR(3);SET_FAN_ATTR(4);SET_FAN_ATTR(5);SET_FAN_ATTR(6);

#define SET_ADC_ATTR(_num) \
    static SENSOR_DEVICE_ATTR(ADC##_num##_under_alert,  S_IRUGO, get_adc_under_vol_alert, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(ADC##_num##_over_alert,  S_IRUGO, get_adc_over_vol_alert, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(ADC##_num##_vol,  S_IRUGO, get_adc_vol, NULL, _num-1)

SET_ADC_ATTR(1);SET_ADC_ATTR(2);SET_ADC_ATTR(3);SET_ADC_ATTR(4);SET_ADC_ATTR(5);SET_ADC_ATTR(6);SET_ADC_ATTR(7);SET_ADC_ATTR(8);

static SENSOR_DEVICE_ATTR(mb_fw_upgrade,  S_IWUSR, NULL, mainBoardUpgrade, 0);
static SENSOR_DEVICE_ATTR(fb_fw_upgrade,  S_IWUSR, NULL, fanBoardUpgrade, 0);
static SENSOR_DEVICE_ATTR(mb_hw_version,  S_IRUGO, get_MB_HW_version, NULL, 0);
static SENSOR_DEVICE_ATTR(fb_hw_version,  S_IRUGO, get_FB_HW_version, NULL, 0);
static SENSOR_DEVICE_ATTR(fb_board_id,  S_IRUGO, get_FB_boardId, NULL, 0);
static SENSOR_DEVICE_ATTR(mb_fw_version,  S_IRUGO, get_MB_FW_version, NULL, 0);
static SENSOR_DEVICE_ATTR(fb_fw_version,  S_IRUGO, get_FB_FW_version, NULL, 0);
static SENSOR_DEVICE_ATTR(fan_pwm,  S_IRUGO | S_IWUSR, get_fan_PWM, set_fan_pwm, 0);
static SENSOR_DEVICE_ATTR(pwr_down,  S_IRUGO, get_pwr_down, NULL, 0);

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

static SENSOR_DEVICE_ATTR(lm75_48_temp_alert,  S_IRUGO, get_temp_alert, NULL, 5);
static SENSOR_DEVICE_ATTR(lm75_49_temp_alert,  S_IRUGO, get_temp_alert, NULL, 4);
static SENSOR_DEVICE_ATTR(lm75_4a_temp_alert,  S_IRUGO, get_temp_alert, NULL, 3);
static SENSOR_DEVICE_ATTR(sa56004x_Ltemp_alert,  S_IRUGO, get_temp_alert, NULL, 2);
static SENSOR_DEVICE_ATTR(sa56004x_Rtemp_alert,  S_IRUGO, get_temp_alert, NULL, 1);
static SENSOR_DEVICE_ATTR(fanBoard_alert,  S_IRUGO, get_temp_alert, NULL, 0);

static SENSOR_DEVICE_ATTR(i2c_fb_timeout,  S_IRUGO, get_i2c_timeout, NULL, 0);
static SENSOR_DEVICE_ATTR(i2c_remote_timeout,  S_IRUGO, get_i2c_timeout, NULL, 1);
static SENSOR_DEVICE_ATTR(i2c_local_timeout,  S_IRUGO, get_i2c_timeout, NULL, 2);
static SENSOR_DEVICE_ATTR(i2c_lm75_48_timeout,  S_IRUGO, get_i2c_timeout, NULL, 3);
static SENSOR_DEVICE_ATTR(i2c_lm75_49_timeout,  S_IRUGO, get_i2c_timeout, NULL, 4);
static SENSOR_DEVICE_ATTR(alert_mode,  S_IRUGO | S_IWUSR, get_alert_mode, set_alert_mode, 0);

static SENSOR_DEVICE_ATTR(fan_num,  S_IRUGO, get_total_fan_num, NULL, 0);

#define SET_TEMP_ATTR(_num) \
    static SENSOR_DEVICE_ATTR(temp##_num##_input,  S_IRUGO, get_hwmon_temp, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(temp##_num##_label,  S_IRUGO, get_label, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(temp##_num##_min,  S_IRUGO, get_hwmon_temp_min, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(temp##_num##_max,  S_IRUGO, get_hwmon_temp_max, set_hwmon_temp_max, _num-1); \
    static SENSOR_DEVICE_ATTR(temp##_num##_crit,  S_IRUGO, get_hwmon_temp_crit, set_hwmon_temp_crit, _num-1);

SET_TEMP_ATTR(1);SET_TEMP_ATTR(2);SET_TEMP_ATTR(3);SET_TEMP_ATTR(4);SET_TEMP_ATTR(5);

static struct attribute *pega_hwmon_mcu_attrs[] = {
    &sensor_dev_attr_mb_fw_upgrade.dev_attr.attr,
    &sensor_dev_attr_fb_fw_upgrade.dev_attr.attr,
    &sensor_dev_attr_mb_hw_version.dev_attr.attr,
    &sensor_dev_attr_fb_hw_version.dev_attr.attr,
    &sensor_dev_attr_fb_board_id.dev_attr.attr,
    &sensor_dev_attr_mb_fw_version.dev_attr.attr,
    &sensor_dev_attr_fb_fw_version.dev_attr.attr,
    &sensor_dev_attr_fan_pwm.dev_attr.attr,
    &sensor_dev_attr_pwr_down.dev_attr.attr,

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
    &sensor_dev_attr_fan6_inner_rpm.dev_attr.attr,

    &sensor_dev_attr_fan1_outer_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_outer_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_outer_rpm.dev_attr.attr,
    &sensor_dev_attr_fan4_outer_rpm.dev_attr.attr,
    &sensor_dev_attr_fan5_outer_rpm.dev_attr.attr,
    &sensor_dev_attr_fan6_outer_rpm.dev_attr.attr,

    &sensor_dev_attr_fan1_present.dev_attr.attr,
    &sensor_dev_attr_fan2_present.dev_attr.attr,
    &sensor_dev_attr_fan3_present.dev_attr.attr,
    &sensor_dev_attr_fan4_present.dev_attr.attr,
    &sensor_dev_attr_fan5_present.dev_attr.attr,
    &sensor_dev_attr_fan6_present.dev_attr.attr,

    &sensor_dev_attr_fan1_enable.dev_attr.attr,
    &sensor_dev_attr_fan2_enable.dev_attr.attr,
    &sensor_dev_attr_fan3_enable.dev_attr.attr,
    &sensor_dev_attr_fan4_enable.dev_attr.attr,
    &sensor_dev_attr_fan5_enable.dev_attr.attr,
    &sensor_dev_attr_fan6_enable.dev_attr.attr,

    &sensor_dev_attr_fan1_led_auto.dev_attr.attr,
    &sensor_dev_attr_fan2_led_auto.dev_attr.attr,
    &sensor_dev_attr_fan3_led_auto.dev_attr.attr,
    &sensor_dev_attr_fan4_led_auto.dev_attr.attr,
    &sensor_dev_attr_fan5_led_auto.dev_attr.attr,
    &sensor_dev_attr_fan6_led_auto.dev_attr.attr,

    &sensor_dev_attr_fan1_led_status.dev_attr.attr,
    &sensor_dev_attr_fan2_led_status.dev_attr.attr,
    &sensor_dev_attr_fan3_led_status.dev_attr.attr,
    &sensor_dev_attr_fan4_led_status.dev_attr.attr,
    &sensor_dev_attr_fan5_led_status.dev_attr.attr,
    &sensor_dev_attr_fan6_led_status.dev_attr.attr,

    &sensor_dev_attr_fan1_led_green.dev_attr.attr,
    &sensor_dev_attr_fan2_led_green.dev_attr.attr,
    &sensor_dev_attr_fan3_led_green.dev_attr.attr,
    &sensor_dev_attr_fan4_led_green.dev_attr.attr,
    &sensor_dev_attr_fan5_led_green.dev_attr.attr,
    &sensor_dev_attr_fan6_led_green.dev_attr.attr,

    &sensor_dev_attr_fan1_led_amber.dev_attr.attr,
    &sensor_dev_attr_fan2_led_amber.dev_attr.attr,
    &sensor_dev_attr_fan3_led_amber.dev_attr.attr,
    &sensor_dev_attr_fan4_led_amber.dev_attr.attr,
    &sensor_dev_attr_fan5_led_amber.dev_attr.attr,
    &sensor_dev_attr_fan6_led_amber.dev_attr.attr,

    &sensor_dev_attr_fan1_status_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_status_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_status_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_status_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_status_alert.dev_attr.attr,
    &sensor_dev_attr_fan6_status_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_airflow_dir.dev_attr.attr,
    &sensor_dev_attr_fan2_airflow_dir.dev_attr.attr,
    &sensor_dev_attr_fan3_airflow_dir.dev_attr.attr,
    &sensor_dev_attr_fan4_airflow_dir.dev_attr.attr,
    &sensor_dev_attr_fan5_airflow_dir.dev_attr.attr,
    &sensor_dev_attr_fan6_airflow_dir.dev_attr.attr,

    &sensor_dev_attr_fan1_motor_num.dev_attr.attr,
    &sensor_dev_attr_fan2_motor_num.dev_attr.attr,
    &sensor_dev_attr_fan3_motor_num.dev_attr.attr,
    &sensor_dev_attr_fan4_motor_num.dev_attr.attr,
    &sensor_dev_attr_fan5_motor_num.dev_attr.attr,
    &sensor_dev_attr_fan6_motor_num.dev_attr.attr,

    &sensor_dev_attr_fan1_model_name.dev_attr.attr,
    &sensor_dev_attr_fan2_model_name.dev_attr.attr,
    &sensor_dev_attr_fan3_model_name.dev_attr.attr,
    &sensor_dev_attr_fan4_model_name.dev_attr.attr,
    &sensor_dev_attr_fan5_model_name.dev_attr.attr,
    &sensor_dev_attr_fan6_model_name.dev_attr.attr,

    &sensor_dev_attr_fan1_serial_num.dev_attr.attr,
    &sensor_dev_attr_fan2_serial_num.dev_attr.attr,
    &sensor_dev_attr_fan3_serial_num.dev_attr.attr,
    &sensor_dev_attr_fan4_serial_num.dev_attr.attr,
    &sensor_dev_attr_fan5_serial_num.dev_attr.attr,
    &sensor_dev_attr_fan6_serial_num.dev_attr.attr,

    &sensor_dev_attr_fan1_vendor.dev_attr.attr,
    &sensor_dev_attr_fan2_vendor.dev_attr.attr,
    &sensor_dev_attr_fan3_vendor.dev_attr.attr,
    &sensor_dev_attr_fan4_vendor.dev_attr.attr,
    &sensor_dev_attr_fan5_vendor.dev_attr.attr,
    &sensor_dev_attr_fan6_vendor.dev_attr.attr,

    &sensor_dev_attr_fan1_part_num.dev_attr.attr,
    &sensor_dev_attr_fan2_part_num.dev_attr.attr,
    &sensor_dev_attr_fan3_part_num.dev_attr.attr,
    &sensor_dev_attr_fan4_part_num.dev_attr.attr,
    &sensor_dev_attr_fan5_part_num.dev_attr.attr,
    &sensor_dev_attr_fan6_part_num.dev_attr.attr,

    &sensor_dev_attr_fan1_hard_version.dev_attr.attr,
    &sensor_dev_attr_fan2_hard_version.dev_attr.attr,
    &sensor_dev_attr_fan3_hard_version.dev_attr.attr,
    &sensor_dev_attr_fan4_hard_version.dev_attr.attr,
    &sensor_dev_attr_fan5_hard_version.dev_attr.attr,
    &sensor_dev_attr_fan6_hard_version.dev_attr.attr,

    &sensor_dev_attr_fan1_speed_target.dev_attr.attr,
    &sensor_dev_attr_fan2_speed_target.dev_attr.attr,
    &sensor_dev_attr_fan3_speed_target.dev_attr.attr,
    &sensor_dev_attr_fan4_speed_target.dev_attr.attr,
    &sensor_dev_attr_fan5_speed_target.dev_attr.attr,
    &sensor_dev_attr_fan6_speed_target.dev_attr.attr,

    &sensor_dev_attr_fan1_speed_tolerance.dev_attr.attr,
    &sensor_dev_attr_fan2_speed_tolerance.dev_attr.attr,
    &sensor_dev_attr_fan3_speed_tolerance.dev_attr.attr,
    &sensor_dev_attr_fan4_speed_tolerance.dev_attr.attr,
    &sensor_dev_attr_fan5_speed_tolerance.dev_attr.attr,
    &sensor_dev_attr_fan6_speed_tolerance.dev_attr.attr,

    &sensor_dev_attr_fan_num.dev_attr.attr,
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
    &sensor_dev_attr_lm75_4a_temp_alert.dev_attr.attr,
    &sensor_dev_attr_sa56004x_Ltemp_alert.dev_attr.attr,
    &sensor_dev_attr_sa56004x_Rtemp_alert.dev_attr.attr,
    &sensor_dev_attr_fanBoard_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_wrongAirflow_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_wrongAirflow_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_wrongAirflow_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_wrongAirflow_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_wrongAirflow_alert.dev_attr.attr,
    &sensor_dev_attr_fan6_wrongAirflow_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_outerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_outerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_outerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_outerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_outerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan6_outerRPMOver_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_outerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_outerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_outerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_outerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_outerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan6_outerRPMUnder_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_outerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_outerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_outerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_outerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_outerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan6_outerRPMZero_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_innerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_innerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_innerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_innerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_innerRPMOver_alert.dev_attr.attr,
    &sensor_dev_attr_fan6_innerRPMOver_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_innerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_innerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_innerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_innerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_innerRPMUnder_alert.dev_attr.attr,
    &sensor_dev_attr_fan6_innerRPMUnder_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_innerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_innerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_innerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_innerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_innerRPMZero_alert.dev_attr.attr,
    &sensor_dev_attr_fan6_innerRPMZero_alert.dev_attr.attr,

    &sensor_dev_attr_fan1_notconnect_alert.dev_attr.attr,
    &sensor_dev_attr_fan2_notconnect_alert.dev_attr.attr,
    &sensor_dev_attr_fan3_notconnect_alert.dev_attr.attr,
    &sensor_dev_attr_fan4_notconnect_alert.dev_attr.attr,
    &sensor_dev_attr_fan5_notconnect_alert.dev_attr.attr,
    &sensor_dev_attr_fan6_notconnect_alert.dev_attr.attr,

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

    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp3_input.dev_attr.attr,
    &sensor_dev_attr_temp4_input.dev_attr.attr,
    &sensor_dev_attr_temp5_input.dev_attr.attr,

    &sensor_dev_attr_temp1_label.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp3_label.dev_attr.attr,
    &sensor_dev_attr_temp4_label.dev_attr.attr,
    &sensor_dev_attr_temp5_label.dev_attr.attr,

    &sensor_dev_attr_temp1_max.dev_attr.attr,
    &sensor_dev_attr_temp2_max.dev_attr.attr,
    &sensor_dev_attr_temp3_max.dev_attr.attr,
    &sensor_dev_attr_temp4_max.dev_attr.attr,
    &sensor_dev_attr_temp5_max.dev_attr.attr,

    &sensor_dev_attr_temp1_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp3_crit.dev_attr.attr,
    &sensor_dev_attr_temp4_crit.dev_attr.attr,
    &sensor_dev_attr_temp5_crit.dev_attr.attr,

    NULL
};

//static const struct attribute_group pega_hwmon_mcu_group = { .attrs = pega_hwmon_mcu_attributes};
ATTRIBUTE_GROUPS(pega_hwmon_mcu);

static void pega_hwmon_mcu_add_client(struct i2c_client *client)
{
    struct mcu_client_node *node = kzalloc(sizeof(struct mcu_client_node), GFP_KERNEL);

    if (!node) {
        pega_print(ERR, "Can't allocate mcu_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &mcu_client_list);
    mutex_unlock(&list_lock);
    pega_print(INFO, "mcu client list added.\n");
}

static void pega_hwmon_mcu_remove_client(struct i2c_client *client)
{
    struct list_head        *list_node = NULL;
    struct mcu_client_node *mcu_mode = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &mcu_client_list)
    {
        mcu_mode = list_entry(list_node, struct mcu_client_node, list);

        if (mcu_mode->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(mcu_mode);
    }

    mutex_unlock(&list_lock);
    pega_print(INFO, "mcu client list removed.\n");
}



static int pega_hwmon_mcu_probe(struct i2c_client *client,
                                const struct i2c_device_id *dev_id)
{
    int i;
    struct device *dev = &client->dev;
    struct hwmon_mcu_data *data;
    struct device *hwmon_dev;

    data = devm_kzalloc(dev, sizeof(struct hwmon_mcu_data), GFP_KERNEL);
    if (data == NULL)
        return -ENOMEM;
    data->client = client;

    for (i = 0; i < HWMON_MCU_TEMP_NUM; i++) {
        data->temp_max[i] = hwmon_temp_threshold_max[i] * 1000;
        data->temp_crit[i] = hwmon_temp_threshold_crit[i] * 1000;
        data->temp_min[i] = hwmon_temp_threshold_min[i] * 1000;
    }

    hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
                                                       data, pega_hwmon_mcu_groups);
    if (IS_ERR(hwmon_dev))
        return PTR_ERR(hwmon_dev);

    //pega_hwmon_mcu_add_client(client);
    pega_print(INFO, "chip found\n");

    return 0;
}

static int pega_hwmon_mcu_remove(struct i2c_client *client)
{
    //pega_hwmon_mcu_remove_client(client);
    return 0;
}

static const struct i2c_device_id pega_hwmon_mcu_id[] = {
    { "pega_hwmon_mcu", 0 },
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
    mutex_init(&list_lock);

    return i2c_add_driver(&pega_hwmon_mcu_driver);
}

static void __exit pega_hwmon_mcu_exit(void)
{
    i2c_del_driver(&pega_hwmon_mcu_driver);
}

module_param(loglevel, uint, 0644);
module_param_string(debug, debug, MAX_DEBUG_INFO_LEN, 0644);
MODULE_PARM_DESC(loglevel,"0x01-LOG_ERR,0x02-LOG_WARNING,0x04-LOG_INFO,0x08-LOG_DEBUG");
MODULE_PARM_DESC(debug,"help info");

MODULE_AUTHOR("Peter5 Lin <Peter5_Lin@pegatroncorp.com.tw>");
MODULE_DESCRIPTION("pega_hwmon_mcu driver");
MODULE_LICENSE("GPL");

module_init(pega_hwmon_mcu_init);
module_exit(pega_hwmon_mcu_exit);
