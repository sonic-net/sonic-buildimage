/*
 * CPLD driver for Nokia-7215-IXS-A1 Router
 *
 * Copyright (C) 2023 Nokia Corporation.
 * Natarajan Subbiramani <natarajan.subbiramani.ext@nokia.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * see <http://www.gnu.org/licenses/>
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
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/mutex.h>

#define DRIVER_NAME "nokia_7215_a1_cpld"

// REG ADDRESS-MAP
#define BOARD_ID_REG                  0x00
#define RESET_CAUSE_REG               0x01
#define CPLD_VERSION_REG              0x02
#define SFP_PRESENCE_REG              0x03
#define SFP_LOS_REG                   0x04
#define SFP_TX_DISABLE_REG            0x05
#define MAC_INIT_STATUS_REG           0x06
#define SYSTEM_STATUS_LED_CONTROL_REG 0x07
#define POWER_AND_FAN_LED_CONTROL_REG 0x08
#define SFP_TX_FAULT_STATUS_REG       0x09
#define PSU1_PSU2_DEVICE_STATUS_REG   0x0A
#define FAN_ENABLE_REG                0x0B
#define USB_POWER_ENABLE_REG          0x0C
#define SFP_LED_TEST_REG              0x0F
#define RESET_REG                     0x10
#define PHY_IRQ_LIVE_STATE_REG        0x11
#define MISC_IRQ_LIVE_STATE_REG       0x12
#define INTERRUPT_REG                 0x13
#define INTERRUPT_MASK_REG            0x14
#define PHY_INT_STATUS_REG            0x15
#define MISC_INT_STATUS_REG           0x16
#define PHY_INT_MASK_REG              0x17
#define MISC_INT_MASK_REG             0x18
#define GPIO_DIRECTION_REG            0x19
#define GPIO_DATA_IN_REG              0x1A
#define GPIO_DATA_OUT_REG             0x1B
#define SCRATCH_REG                   0xFE

//REG BIT FIELD POSITIONS
#define RESET_CAUSE_REG_COLD_RESET    0x1
#define RESET_CAUSE_REG_WARM_RESET    0x2
#define RESET_CAUSE_REG_WDOG_RESET    0x4
#define RESET_CAUSE_REG_SYS_RESET     0x8

#define SFP_PRESENCE_REG_SFP49        0x0
#define SFP_PRESENCE_REG_SFP50        0x1
#define SFP_PRESENCE_REG_SFP51        0x2
#define SFP_PRESENCE_REG_SFP52        0x3

#define SFP_LOS_REG_SFP49             0x0
#define SFP_LOS_REG_SFP50             0x1
#define SFP_LOS_REG_SFP51             0x2
#define SFP_LOS_REG_SFP52             0x3

#define SFP_TX_DISABLE_REG_SFP49      0x0
#define SFP_TX_DISABLE_REG_SFP50      0x1
#define SFP_TX_DISABLE_REG_SFP51      0x2
#define SFP_TX_DISABLE_REG_SFP52      0x3
#define SFP_TX_DISABLE_REG_LED_MUX    0x4

#define MAC_INIT_STATUS_REG_INIT_DONE 0x2

enum system_status_led_mode {
    SYSTEM_STATUS_LED_OFF,
    SYSTEM_STATUS_LED_AMBER,
    SYSTEM_STATUS_LED_GREEN,
    SYSTEM_STATUS_LED_AMBERBLINK8,
    SYSTEM_STATUS_LED_GREENBLINK4,
    SYSTEM_STATUS_LED_AMBERBLINK4,
    SYSTEM_STATUS_LED_GREENBLINK1,
    SYSTEM_STATUS_LED_COLORTOGGLE,
    SYSTEM_STATUS_LED_INVALID
};

char *system_status_led_mode_str[]={"off", "amber", "green", "blink8-amber", "blink4-green",
                                    "blink4-amber", "blink1-green", "toggle", "invalid"};

#define POWER_LED_OFFSET  6
#define FAN_LED_OFFSET    4
enum power_fan_led_mode {
    POWER_FAN_LED_OFF,
    POWER_FAN_LED_GREEN,
    POWER_FAN_LED_AMBER,
    POWER_FAN_LED_GREEN_BLINK,
    POWER_FAN_INVALID
};
char *power_fan_led_mode_str[]={"off", "green", "amber", "blink-green", "invalid"};

#define SFP_TX_FAULT_STATUS_SFP49  0x0
#define SFP_TX_FAULT_STATUS_SFP50  0x1
#define SFP_TX_FAULT_STATUS_SFP51  0x2
#define SFP_TX_FAULT_STATUS_SFP52  0x3

#define PSU1_POWERGOOD   2
#define PSU2_POWERGOOD   3

#define FAN1_ENABLE      0
#define FAN2_ENABLE      1

#define USB_POWER_ENABLE 0

#define RESET_REG_WARM_RESET   0x0
#define RESET_REG_COLD_RESET   0x4
#define RESET_REG_I2CMUX_RESET 0x6
#define RESET_REG_ZL_RESET     0x7

static const unsigned short cpld_address_list[] = {0x41, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int mb_hw_version;
    int cpld_version;
};

static int nokia_7215_ixs_a1_cpld_read(struct cpld_data *data, u8 reg)
{
    int val=0;
    struct i2c_client *client=data->client;

    mutex_lock(&data->update_lock);
    val = i2c_smbus_read_byte_data(client, reg);
    if (val < 0) {
         dev_err(&client->dev, "CPLD READ ERROR: reg(0x%02x) err %d\n", reg, val);
    }
    mutex_unlock(&data->update_lock);

    return val;
}

static void nokia_7215_ixs_a1_cpld_write(struct cpld_data *data, u8 reg, u8 value)
{
    int res=0;
    struct i2c_client *client=data->client;

    mutex_lock(&data->update_lock);
    res = i2c_smbus_write_byte_data(client, reg, value);
    if (res < 0) {
        dev_err(&client->dev, "CPLD WRITE ERROR: reg(0x%02x) err %d\n", reg, res);
    }
    mutex_unlock(&data->update_lock);
}

static ssize_t show_mainboard_hwversion(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf,"0x%02x\n",data->mb_hw_version);
}

static ssize_t show_last_reset_cause(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val=0;
    char *reason=NULL;
    val = nokia_7215_ixs_a1_cpld_read(data, RESET_CAUSE_REG);
    switch (val) {
    case RESET_CAUSE_REG_COLD_RESET:
        reason="cold reset";
        break;
    case RESET_CAUSE_REG_WARM_RESET:
        reason="warm reset";
        break;
    case RESET_CAUSE_REG_WDOG_RESET:
        reason="wdog reset";
        break;
    case RESET_CAUSE_REG_SYS_RESET:
        reason="sys reset";
        break;
    default:
        reason="unknown";
        break;
    }
    return sprintf(buf,"0x%02x %s\n",val, reason);
}

static ssize_t show_cpld_version(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf,"0x%02x\n",data->cpld_version);
}

static ssize_t show_sfp_present(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val=0;
    val = nokia_7215_ixs_a1_cpld_read(data, SFP_PRESENCE_REG);
    
    /* If the bit is set, SFP is not present. So, we are toggling intentionally */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 0:1);
}

static ssize_t show_sfp_los(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val=0;
    val = nokia_7215_ixs_a1_cpld_read(data, SFP_LOS_REG);
    
    /* If the bit is set, LOS condition */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_sfp_tx_disable(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val=0;
    val = nokia_7215_ixs_a1_cpld_read(data, SFP_TX_DISABLE_REG);
    
    /* If the bit is set, Tx is disabled */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_sfp_tx_disable(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << sda->index)) & 0xFF;
    reg_val = nokia_7215_ixs_a1_cpld_read(data, SFP_TX_DISABLE_REG);
    reg_val = reg_val & mask;

    usr_val = usr_val << sda->index;

    nokia_7215_ixs_a1_cpld_write(data, SFP_TX_DISABLE_REG, (reg_val|usr_val));

    return count;

}

static ssize_t show_system_led_status(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val=0;
    val = nokia_7215_ixs_a1_cpld_read(data, SYSTEM_STATUS_LED_CONTROL_REG);
    if (val > SYSTEM_STATUS_LED_INVALID) {
        val = SYSTEM_STATUS_LED_INVALID;
    }
    return sprintf(buf,"%s\n",system_status_led_mode_str[val]);
}

static ssize_t set_system_led_status(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    int mode;

    for(mode=SYSTEM_STATUS_LED_OFF; mode<SYSTEM_STATUS_LED_INVALID; mode++) {
        if(strncmp(buf, system_status_led_mode_str[mode],strlen(system_status_led_mode_str[mode]))==0) {
            nokia_7215_ixs_a1_cpld_write(data, SYSTEM_STATUS_LED_CONTROL_REG, mode);
            break;
        }
    }

    return count;
}

static ssize_t show_power_fan_led_status(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = nokia_7215_ixs_a1_cpld_read(data, POWER_AND_FAN_LED_CONTROL_REG);
    val = (val >> sda->index) & 0x3;
    return sprintf(buf,"%s\n",power_fan_led_mode_str[val]);
}

static ssize_t set_power_fan_led_status(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, mask=0, usr_val=0;
    int i;

    mask = (~(0x3 << sda->index)) & 0xFF;
    reg_val = nokia_7215_ixs_a1_cpld_read(data, POWER_AND_FAN_LED_CONTROL_REG);
    reg_val = reg_val & mask;

    for(i=POWER_FAN_LED_OFF; i<POWER_FAN_INVALID; i++) {
        if(strncmp(buf, power_fan_led_mode_str[i],strlen(power_fan_led_mode_str[i]))==0) {
            usr_val = i << sda->index;
            nokia_7215_ixs_a1_cpld_write(data, POWER_AND_FAN_LED_CONTROL_REG, reg_val|usr_val);
            break;
        }
    }

    return count;
}

static ssize_t show_sfp_tx_fault(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val=0;
    val = nokia_7215_ixs_a1_cpld_read(data, SFP_TX_FAULT_STATUS_REG);
    
    /* If the bit is set, TX fault condition */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 1:0);
}


static ssize_t show_psu_pg_status(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val=0;
    val = nokia_7215_ixs_a1_cpld_read(data, PSU1_PSU2_DEVICE_STATUS_REG);
    
    /* If the bit is set, psu power is good */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_fan_enable_status(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val=0;
    val = nokia_7215_ixs_a1_cpld_read(data, FAN_ENABLE_REG);
    
    /* If the bit is set, fan is disabled. So, toggling intentionally */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 0:1);
}

static ssize_t set_fan_enable_status(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << sda->index)) & 0xFF;
    reg_val = nokia_7215_ixs_a1_cpld_read(data, RESET_REG);
    reg_val = reg_val & mask;

    usr_val = !usr_val; // If the bit is set, fan is disabled. So, toggling intentionally
    usr_val = usr_val << sda->index;

    nokia_7215_ixs_a1_cpld_write(data, FAN_ENABLE_REG, (reg_val|usr_val));

    return count;

}

static ssize_t show_usb_enable_status(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val=0;
    val = nokia_7215_ixs_a1_cpld_read(data, USB_POWER_ENABLE_REG);
    
    /* If the bit is set, usb power is disabled. So, toggling intentionally */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 0:1);
}

static ssize_t set_usb_enable_status(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 usr_val=0;
    int ret=kstrtou8(buf,16, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if(usr_val > 1) {
        return -EINVAL;
    }
    /* If the bit is set, usb power is disabled. So, toggling intentionally */
    usr_val = !usr_val;

    nokia_7215_ixs_a1_cpld_write(data, USB_POWER_ENABLE_REG, usr_val);
    return count;
}

static ssize_t show_sfp_ledtest_status(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = nokia_7215_ixs_a1_cpld_read(data, SFP_LED_TEST_REG);
    return sprintf(buf,"0x%02x\n",val);
}

static ssize_t set_sfp_ledtest_status(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 usr_val=0;
    int ret=kstrtou8(buf,16, &usr_val);
    if (ret != 0) {
        return ret; 
    }

    nokia_7215_ixs_a1_cpld_write(data, SFP_LED_TEST_REG, usr_val);
    return count;
}

static ssize_t show_reset_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val=0;
    val = nokia_7215_ixs_a1_cpld_read(data, RESET_REG);

    return sprintf(buf,"0x%02x\n",(val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_reset_reg(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << sda->index)) & 0xFF;
    reg_val = nokia_7215_ixs_a1_cpld_read(data, RESET_REG);
    reg_val = reg_val & mask;

    usr_val = usr_val << sda->index;

    nokia_7215_ixs_a1_cpld_write(data, RESET_REG, (reg_val|usr_val));

    return count;
}


/* sysfs attributes */
static SENSOR_DEVICE_ATTR(mbhwversion, S_IRUGO, show_mainboard_hwversion, NULL, 0);
static SENSOR_DEVICE_ATTR(last_reset_cause, S_IRUGO, show_last_reset_cause, NULL, 0);
static SENSOR_DEVICE_ATTR(cpldversion, S_IRUGO, show_cpld_version, NULL, 0);
static SENSOR_DEVICE_ATTR(sfp49_present, S_IRUGO, show_sfp_present, NULL, SFP_PRESENCE_REG_SFP49);
static SENSOR_DEVICE_ATTR(sfp50_present, S_IRUGO, show_sfp_present, NULL, SFP_PRESENCE_REG_SFP50);
static SENSOR_DEVICE_ATTR(sfp51_present, S_IRUGO, show_sfp_present, NULL, SFP_PRESENCE_REG_SFP51);
static SENSOR_DEVICE_ATTR(sfp52_present, S_IRUGO, show_sfp_present, NULL, SFP_PRESENCE_REG_SFP52);
static SENSOR_DEVICE_ATTR(sfp49_los, S_IRUGO, show_sfp_los, NULL, SFP_LOS_REG_SFP49);
static SENSOR_DEVICE_ATTR(sfp50_los, S_IRUGO, show_sfp_los, NULL, SFP_LOS_REG_SFP50);
static SENSOR_DEVICE_ATTR(sfp51_los, S_IRUGO, show_sfp_los, NULL, SFP_LOS_REG_SFP51);
static SENSOR_DEVICE_ATTR(sfp52_los, S_IRUGO, show_sfp_los, NULL, SFP_LOS_REG_SFP52);
static SENSOR_DEVICE_ATTR(sfp49_tx_disable, S_IRUGO | S_IWUSR, show_sfp_tx_disable, set_sfp_tx_disable, SFP_LOS_REG_SFP49);
static SENSOR_DEVICE_ATTR(sfp50_tx_disable, S_IRUGO | S_IWUSR, show_sfp_tx_disable, set_sfp_tx_disable, SFP_LOS_REG_SFP50);
static SENSOR_DEVICE_ATTR(sfp51_tx_disable, S_IRUGO | S_IWUSR, show_sfp_tx_disable, set_sfp_tx_disable, SFP_LOS_REG_SFP51);
static SENSOR_DEVICE_ATTR(sfp52_tx_disable, S_IRUGO | S_IWUSR, show_sfp_tx_disable, set_sfp_tx_disable, SFP_LOS_REG_SFP52);
static SENSOR_DEVICE_ATTR(system_led, S_IRUGO | S_IWUSR, show_system_led_status, set_system_led_status, 0);
static SENSOR_DEVICE_ATTR(psu_led, S_IRUGO | S_IWUSR, show_power_fan_led_status, set_power_fan_led_status, POWER_LED_OFFSET);
static SENSOR_DEVICE_ATTR(fan_led, S_IRUGO | S_IWUSR, show_power_fan_led_status, set_power_fan_led_status, FAN_LED_OFFSET);
static SENSOR_DEVICE_ATTR(sfp49_tx_fault, S_IRUGO, show_sfp_tx_fault, NULL, SFP_TX_FAULT_STATUS_SFP49);
static SENSOR_DEVICE_ATTR(sfp50_tx_fault, S_IRUGO, show_sfp_tx_fault, NULL, SFP_TX_FAULT_STATUS_SFP50);
static SENSOR_DEVICE_ATTR(sfp51_tx_fault, S_IRUGO, show_sfp_tx_fault, NULL, SFP_TX_FAULT_STATUS_SFP51);
static SENSOR_DEVICE_ATTR(sfp52_tx_fault, S_IRUGO, show_sfp_tx_fault, NULL, SFP_TX_FAULT_STATUS_SFP52);
static SENSOR_DEVICE_ATTR(psu1_power_good, S_IRUGO, show_psu_pg_status, NULL, PSU1_POWERGOOD);
static SENSOR_DEVICE_ATTR(psu2_power_good, S_IRUGO, show_psu_pg_status, NULL, PSU2_POWERGOOD);
static SENSOR_DEVICE_ATTR(fan1_enable, S_IRUGO | S_IWUSR, show_fan_enable_status, set_fan_enable_status, FAN1_ENABLE);
static SENSOR_DEVICE_ATTR(fan2_enable, S_IRUGO | S_IWUSR, show_fan_enable_status, set_fan_enable_status, FAN2_ENABLE);
static SENSOR_DEVICE_ATTR(usb_power_enable, S_IRUGO | S_IWUSR, show_usb_enable_status, set_usb_enable_status, 0);
static SENSOR_DEVICE_ATTR(sfp_led_test, S_IRUGO | S_IWUSR, show_sfp_ledtest_status, set_sfp_ledtest_status, 0);
static SENSOR_DEVICE_ATTR(warm_reset, S_IRUGO | S_IWUSR, show_reset_reg, set_reset_reg, RESET_REG_WARM_RESET);
static SENSOR_DEVICE_ATTR(cold_reset, S_IRUGO | S_IWUSR, show_reset_reg, set_reset_reg, RESET_REG_COLD_RESET);
static SENSOR_DEVICE_ATTR(i2cmux_reset, S_IRUGO | S_IWUSR, show_reset_reg, set_reset_reg, RESET_REG_I2CMUX_RESET);
static SENSOR_DEVICE_ATTR(zarlink_reset, S_IRUGO | S_IWUSR, show_reset_reg, set_reset_reg, RESET_REG_ZL_RESET);

static struct attribute *nokia_7215_ixs_a1_cpld_attributes[] = {
    &sensor_dev_attr_mbhwversion.dev_attr.attr,
    &sensor_dev_attr_last_reset_cause.dev_attr.attr,
    &sensor_dev_attr_cpldversion.dev_attr.attr,
    &sensor_dev_attr_sfp49_present.dev_attr.attr,
    &sensor_dev_attr_sfp50_present.dev_attr.attr,
    &sensor_dev_attr_sfp51_present.dev_attr.attr,
    &sensor_dev_attr_sfp52_present.dev_attr.attr,
    &sensor_dev_attr_sfp49_los.dev_attr.attr,
    &sensor_dev_attr_sfp50_los.dev_attr.attr,
    &sensor_dev_attr_sfp51_los.dev_attr.attr,
    &sensor_dev_attr_sfp52_los.dev_attr.attr,
    &sensor_dev_attr_sfp49_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp50_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp51_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp52_tx_disable.dev_attr.attr,
    &sensor_dev_attr_system_led.dev_attr.attr,
    &sensor_dev_attr_psu_led.dev_attr.attr,
    &sensor_dev_attr_fan_led.dev_attr.attr,
    &sensor_dev_attr_sfp49_tx_fault.dev_attr.attr,
    &sensor_dev_attr_sfp50_tx_fault.dev_attr.attr,
    &sensor_dev_attr_sfp51_tx_fault.dev_attr.attr,
    &sensor_dev_attr_sfp52_tx_fault.dev_attr.attr,
    &sensor_dev_attr_psu1_power_good.dev_attr.attr,
    &sensor_dev_attr_psu2_power_good.dev_attr.attr,
    &sensor_dev_attr_fan1_enable.dev_attr.attr,
    &sensor_dev_attr_fan2_enable.dev_attr.attr,
    &sensor_dev_attr_usb_power_enable.dev_attr.attr,
    &sensor_dev_attr_sfp_led_test.dev_attr.attr,
    &sensor_dev_attr_warm_reset.dev_attr.attr,
    &sensor_dev_attr_cold_reset.dev_attr.attr,
    &sensor_dev_attr_i2cmux_reset.dev_attr.attr,
    &sensor_dev_attr_zarlink_reset.dev_attr.attr,
    NULL
};

static const struct attribute_group nokia_7215_ixs_a1_cpld_group = {
    .attrs = nokia_7215_ixs_a1_cpld_attributes,
};


static int nokia_7215_ixs_a1_cpld_probe(struct i2c_client *client,
        const struct i2c_device_id *dev_id)
{
    int status;
     struct cpld_data *data=NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7215-IXS-A1 CPLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &nokia_7215_ixs_a1_cpld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    data->mb_hw_version = nokia_7215_ixs_a1_cpld_read(data, BOARD_ID_REG);
    data->cpld_version = nokia_7215_ixs_a1_cpld_read(data, CPLD_VERSION_REG);

    return 0;

exit:
    return status;
}

static void nokia_7215_ixs_a1_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &nokia_7215_ixs_a1_cpld_group);
    kfree(data);
    return;
}

static const struct of_device_id nokia_7215_ixs_a1_cpld_of_ids[] = {
    {
        .compatible = "nokia,7215_a1_cpld",
        .data = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, nokia_7215_ixs_a1_cpld_of_ids);

static const struct i2c_device_id nokia_7215_ixs_a1_cpld_ids[] = {
    { DRIVER_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, nokia_7215_ixs_a1_cpld_ids);

static struct i2c_driver nokia_7215_ixs_a1_cpld_driver = {
    .driver = {
        .name     = DRIVER_NAME,
        .of_match_table = of_match_ptr(nokia_7215_ixs_a1_cpld_of_ids),
    },
    .probe        = nokia_7215_ixs_a1_cpld_probe,
    .remove       = nokia_7215_ixs_a1_cpld_remove,
    .id_table     = nokia_7215_ixs_a1_cpld_ids,
    .address_list = cpld_address_list,
};



static int __init nokia_7215_ixs_a1_cpld_init(void)
{
    return i2c_add_driver(&nokia_7215_ixs_a1_cpld_driver);
}

static void __exit nokia_7215_ixs_a1_cpld_exit(void)
{
    i2c_del_driver(&nokia_7215_ixs_a1_cpld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7215-IXS-A1 CPLD driver");
MODULE_LICENSE("GPL");

module_init(nokia_7215_ixs_a1_cpld_init);
module_exit(nokia_7215_ixs_a1_cpld_exit);
