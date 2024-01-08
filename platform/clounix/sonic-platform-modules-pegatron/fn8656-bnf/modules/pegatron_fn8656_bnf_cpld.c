/*
 * A CPLD driver for the fn8656_bnf
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


#define PEGA_RD(x) 1

#define CPLD_MAX_NUM                4
#define CPLD_SFP_MAX_GROUP          3
#define SFP_PORT_MAX_NUM            56
#define SFP_EEPROM_SIZE             256
#define QSFP_FIRST_PORT             48
#define CPLDA_SFP_NUM               0
#define CPLDB_SFP_NUM               28
#define CPLDC_SFP_NUM               28
#define CPLDA_ADDRESS               0x74
#define CPLDB_ADDRESS               0x75
#define CPLDC_ADDRESS               0x76
#define CPLDD_ADDRESS               0x18
#define LM75B_ADDRESS               0x4a
#define LM75B_TEMP_REG 	            0x0


#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

static LIST_HEAD(cpld_client_list);
static struct mutex  list_lock;
/* Addresses scanned for pegatron_fn8656_bnf_cpld
 */
static const unsigned short normal_i2c[] = { CPLDA_ADDRESS, CPLDB_ADDRESS, CPLDC_ADDRESS, CPLDD_ADDRESS, LM75B_ADDRESS, I2C_CLIENT_END };

static uint loglevel = LOG_INFO | LOG_WARNING | LOG_ERR;
static char sfp_debug[MAX_DEBUG_INFO_LEN] = "debug info:                     \n\
init_mode ---  0: High power mode, 1: Low power mode  \n\
power_on  ---  0: Not finished,    1: Finished        \n\
reset     ---  0: Reset Keep low,  1: Reset Keep high \n\
present   ---  0: Module insert,   1: Module absent   \n\
interrupt ---  0: Interrupt active 1: Normal operation\n";

static char cpld_debug[MAX_DEBUG_INFO_LEN] = "cpld debug info:            \n";
static char sysled_debug[MAX_DEBUG_INFO_LEN] = "sysled debug info:            \n";

enum{
    DEFAULT_REQUIREMENT = 0x0,
    KS_REQUIREMENT = 0x01
};

static uint requirement_flag = DEFAULT_REQUIREMENT;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

int pegatron_fn8656_bnf_cpld_read(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int data = -EPERM;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client->addr == addr) {
            data = i2c_smbus_read_byte_data(cpld_node->client, reg);
            pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", addr, reg, data);
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return data;
}
EXPORT_SYMBOL(pegatron_fn8656_bnf_cpld_read);


int pegatron_fn8656_bnf_cpld_write(unsigned short addr, u8 reg, u8 val)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client->addr == addr) {
            ret = i2c_smbus_write_byte_data(cpld_node->client, reg, val);
            pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", addr, reg, val);
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(pegatron_fn8656_bnf_cpld_write);

#if PEGA_RD(CPLD_VERSION)

#define CPLD_VERSION_REG                    0x0
    #define CPLD_HW_VERSION_BIT_OFFSET      5
    #define CPLD_HW_VERSION_BIT_MSK         (0x3 << CPLD_HW_VERSION_BIT_OFFSET)
    #define CPLD_SW_VERSION_BIT_OFFSET      0
    #define CPLD_SW_VERSION_BIT_MSK         (0x1F << CPLD_SW_VERSION_BIT_OFFSET)

static ssize_t read_cpld_HWversion(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_VERSION_REG;

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "0x%02x\n", (data & CPLD_HW_VERSION_BIT_MSK) >> CPLD_HW_VERSION_BIT_OFFSET);
}

static ssize_t read_cpld_SWversion(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_VERSION_REG;

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "%02d\n", (data & CPLD_SW_VERSION_BIT_MSK) >> CPLD_SW_VERSION_BIT_OFFSET);
}
#endif //CPLD_VERSION

#if PEGA_RD(LED)

#define CPLD_A_LED_CONTROL_REG_1            0x5
#define CPLD_A_SYS_LED_CTL_BIT_OFFSET       5
#define CPLD_A_SYS_LED_CTL_BIT_MSK          (0x7 << CPLD_A_SYS_LED_CTL_BIT_OFFSET)
#define CPLD_A_PWR_LED_CTL_BIT_OFFSET       2
#define CPLD_A_PWR_LED_CTL_BIT_MSK          (0x7 << CPLD_A_PWR_LED_CTL_BIT_OFFSET)

#define CPLD_A_LED_CONTROL_REG_2            0x6
#define CPLD_A_FAN_LED_CTL_BIT_OFFSET       0
#define CPLD_A_FAN_LED_CTL_BIT_MSK          (0x7 << CPLD_A_FAN_LED_CTL_BIT_OFFSET)
#define CPLD_A_SERIAL_LED_BIT_OFFSET        3
#define CPLD_A_SERIAL_LED_BIT_MASK          (0x1 << CPLD_A_SERIAL_LED_BIT_OFFSET)
#define CPLD_A_LED_CONTROL_REG_3            0x7
#define CPLD_A_ALL_LED_CTL_BIT_OFFSET       0
#define CPLD_A_ALL_LED_CTL_BIT_MSK          (0xFF << CPLD_A_ALL_LED_CTL_BIT_OFFSET)
#define CPLD_BC_LED_CONTROL_REG_1           0x01
#define CPLD_BC_SERIAL_LED_BIT_OFFSET       2
#define CPLD_BC_SERIAL_LED_BIT_MASK         (0x1 << CPLD_BC_SERIAL_LED_BIT_OFFSET)

#define CPLD_D_LED_CONTROL_REG              0x6
#define CPLD_D_LOC_LED_CTL_BIT_OFFSET       0
#define CPLD_D_LOC_LED_CTL_BIT_MSK          (0x3 << CPLD_D_LOC_LED_CTL_BIT_OFFSET)

enum _sysfs_led_attr {
    ALL_LED_CTRL = 0,
    SERIAL_LED_CTRL,
    SYS_LED_CTRL,
    PWR_LED_CTRL,
    LOC_LED_CTRL,
    FAN_LED_CTRL,
    LED_ATTR_END
};
enum _cpld_led_bit_attr{
    CPLD_LED_STATUS_GREEN_ON = 0,
    CPLD_LED_STATUS_RED_ON,
    CPLD_LED_STATUS_OFF,
    CPLD_LED_STATUS_MIX_ON,
    CPLD_LED_STATUS_GREEN_BLINK,
    CPLD_LED_STATUS_RED_BLINK,
    CPLD_LED_STATUS_MIX_BLINK,
    CPLD_LED_STATUS_NOT_SUPPORT
};
enum _kwai_led_bit_attr{
    KWAI_LED_STATUS_OFF = 0,
    KWAI_LED_STATUS_GREEN_ON,
    KWAI_LED_STATUS_YELLOW_ON,
    KWAI_LED_STATUS_RED_ON,
    KWAI_LED_STATUS_GREEN_BLINK,
    KWAI_LED_STATUS_YELLOW_BLINK,
    KWAI_LED_STATUS_RED_BLINK,
    KWAI_LED_STATUS_BLLUE_ON,
    KWAI_LED_STATUS_BLLUE_BLINK,
    KWAI_LED_STATUS_NOT_EXIST
};


static u8 map_ledstatus_cpldconfig_2_userconfig[] = {KWAI_LED_STATUS_GREEN_ON,KWAI_LED_STATUS_RED_ON,KWAI_LED_STATUS_OFF,KWAI_LED_STATUS_NOT_EXIST,
            KWAI_LED_STATUS_GREEN_BLINK,KWAI_LED_STATUS_RED_BLINK,KWAI_LED_STATUS_NOT_EXIST};


static u8 map_ledstatus_userconfig_2_cpldconfig[] = {CPLD_LED_STATUS_OFF,CPLD_LED_STATUS_GREEN_ON,CPLD_LED_STATUS_NOT_SUPPORT,CPLD_LED_STATUS_RED_ON,CPLD_LED_STATUS_GREEN_BLINK,
            CPLD_LED_STATUS_NOT_SUPPORT,CPLD_LED_STATUS_RED_BLINK,CPLD_LED_STATUS_NOT_SUPPORT,CPLD_LED_STATUS_NOT_SUPPORT};

static ssize_t show_led_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = 0,bit_offset = 0,bit_msk = 0;
    bool attr_cpld_2_user = FALSE;
    
    switch(attr->index) {
        case ALL_LED_CTRL: 
            reg = CPLD_A_LED_CONTROL_REG_3;
            bit_offset = CPLD_A_ALL_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_A_ALL_LED_CTL_BIT_MSK;
            break;
        case SERIAL_LED_CTRL:
            if(client->addr == CPLDA_ADDRESS)
            {
                reg = CPLD_A_LED_CONTROL_REG_2;
                bit_offset = CPLD_A_SERIAL_LED_BIT_OFFSET; 
                bit_msk = CPLD_A_SERIAL_LED_BIT_MASK;
            }
            else if((client->addr == CPLDB_ADDRESS) || (client->addr == CPLDC_ADDRESS))
            {
                reg = CPLD_BC_LED_CONTROL_REG_1; 
                bit_offset = CPLD_BC_SERIAL_LED_BIT_OFFSET; 
                bit_msk = CPLD_BC_SERIAL_LED_BIT_MASK;
            }
            break;
        case SYS_LED_CTRL:
            reg = CPLD_A_LED_CONTROL_REG_1;
            bit_offset = CPLD_A_SYS_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_A_SYS_LED_CTL_BIT_MSK;
            attr_cpld_2_user = TRUE;
            break;
        case PWR_LED_CTRL:
            reg = CPLD_A_LED_CONTROL_REG_1;
            bit_offset = CPLD_A_PWR_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_A_PWR_LED_CTL_BIT_MSK;
            attr_cpld_2_user = TRUE;
            break;
        case FAN_LED_CTRL:
            reg = CPLD_A_LED_CONTROL_REG_2;
            bit_offset = CPLD_A_FAN_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_A_FAN_LED_CTL_BIT_MSK;
            attr_cpld_2_user = TRUE;
            break;
        case LOC_LED_CTRL:
            reg = CPLD_D_LED_CONTROL_REG;
            bit_offset = CPLD_D_LOC_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_D_LOC_LED_CTL_BIT_MSK;
            break;
        default:
            pega_print(ERR,"incorrect led type\n");
            return sprintf(buf, "ERR");
    }

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    data = (data & bit_msk) >> bit_offset;
    
    if(data >= CPLD_LED_STATUS_NOT_SUPPORT)
        return -EINVAL;
    if(attr_cpld_2_user == TRUE)
        data = map_ledstatus_cpldconfig_2_userconfig[data];

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_led_status(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = 0,bit_offset = 0,bit_msk = 0;
    long val = 0;
    bool attr_user_2_cpld = FALSE;
    
    switch(attr->index) {
        case ALL_LED_CTRL: 
            reg = CPLD_A_LED_CONTROL_REG_3;
            bit_offset = CPLD_A_ALL_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_A_ALL_LED_CTL_BIT_MSK;
            break;
        case SERIAL_LED_CTRL:
            if(client->addr == CPLDA_ADDRESS)
            {
                reg = CPLD_A_LED_CONTROL_REG_2;
                bit_offset = CPLD_A_SERIAL_LED_BIT_OFFSET; 
                bit_msk = CPLD_A_SERIAL_LED_BIT_MASK;
            }
            else if((client->addr == CPLDB_ADDRESS) || (client->addr == CPLDC_ADDRESS))
            {
                reg = CPLD_BC_LED_CONTROL_REG_1; 
                bit_offset = CPLD_BC_SERIAL_LED_BIT_OFFSET; 
                bit_msk = CPLD_BC_SERIAL_LED_BIT_MASK;
            }
            break;
        case SYS_LED_CTRL:
            reg = CPLD_A_LED_CONTROL_REG_1;
            bit_offset = CPLD_A_SYS_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_A_SYS_LED_CTL_BIT_MSK;
            attr_user_2_cpld = TRUE;
            break;
        case PWR_LED_CTRL:
            reg = CPLD_A_LED_CONTROL_REG_1;
            bit_offset = CPLD_A_PWR_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_A_PWR_LED_CTL_BIT_MSK;
            attr_user_2_cpld = TRUE;
            break;
        case FAN_LED_CTRL:
            reg = CPLD_A_LED_CONTROL_REG_2;
            bit_offset = CPLD_A_FAN_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_A_FAN_LED_CTL_BIT_MSK;
            attr_user_2_cpld = TRUE;
            break;
        case LOC_LED_CTRL:
            reg = CPLD_D_LED_CONTROL_REG;
            bit_offset = CPLD_D_LOC_LED_CTL_BIT_OFFSET;
            bit_msk = CPLD_D_LOC_LED_CTL_BIT_MSK;
            break;
        default:
            pega_print(ERR,"incorrect led type\n");
            return -EINVAL;
    }
    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    if(val >= KWAI_LED_STATUS_NOT_EXIST)
    {
        return -EINVAL;
    }
    if(attr_user_2_cpld == TRUE)
        val = map_ledstatus_userconfig_2_cpldconfig[val];
    if(val >= CPLD_LED_STATUS_NOT_SUPPORT)
    {
        return -EINVAL;
    }

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    data = (val << bit_offset) | (data & (~bit_msk));

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    pegatron_fn8656_bnf_cpld_write(client->addr, reg, data);

    return count;
}
#endif //LED

#if PEGA_RD(EEPROM_WRITE_ENABLE)
#define CPLD_A_EEPROM_WRITE_REG       0x1
#define CPLD_A_EEPROM_WRITE_BIT       1
static ssize_t show_eeprom_write_enable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = CPLD_A_EEPROM_WRITE_REG;

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, reg, val);

    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t set_eeprom_write_enable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_A_EEPROM_WRITE_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    if(val)
        SET_BIT(data, CPLD_A_EEPROM_WRITE_BIT);
    else
        CLEAR_BIT(data, CPLD_A_EEPROM_WRITE_BIT);
    
    pegatron_fn8656_bnf_cpld_write(client->addr, reg, data);

    return count;
}
#endif //EEPROM_WRITE_ENABLE

#if PEGA_RD(PSU)
#define CPLD_A_PSU_REG                0x2
    #define CPLD_A_PSU_PWOK_BASE          0
    #define CPLD_A_PSU_PRESENT_BASE       2
    #define CPLD_A_PSU_AC_OK_BASE         4

#define PSU_ABSENT 1
#define PSU_POWER_OK 1

#define KWAI_PSU_ABSENT 0
#define KWAI_PSU_PRESENT_POWER_OK 1
#define KWAI_PSU_PRESENT_POWER_NOT_OK 2

static ssize_t read_psu_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val=0, reg = CPLD_A_PSU_REG;
    u8 power_good = 0, ac_good = 0, present = 0;

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    
    GET_BIT(data, (CPLD_A_PSU_PWOK_BASE + attr->index), power_good);
    GET_BIT(data, (CPLD_A_PSU_AC_OK_BASE + attr->index), ac_good);
    GET_BIT(data, (CPLD_A_PSU_PRESENT_BASE + attr->index), present);

    if(present == PSU_ABSENT)
    {
        val = KWAI_PSU_ABSENT;
    }
    else
    {
        if((ac_good == PSU_POWER_OK)&&(power_good == PSU_POWER_OK))
        {
            val = KWAI_PSU_PRESENT_POWER_OK;
        }
        else
        {
            val = KWAI_PSU_PRESENT_POWER_NOT_OK;
        }
    }

    return sprintf(buf, "0x%02x\n", val);
}
#endif //PSU

#define CPLD_A_INT_REG                0x0B
static ssize_t read_int_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    u8 data = 0, val=0, reg = CPLD_A_INT_REG,fan_fault = 0, psu_fault = 0;

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    GET_BIT(data, 5, fan_fault);
    //3V3 system power alert
    GET_BIT(data, 0, psu_fault);

    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x,Fan board MCU:%d,3V3 system power alert:%d\r\n", client->addr, reg, data, fan_fault, psu_fault);
    val = fan_fault | (psu_fault << 1);

    return sprintf(buf, "0x%02x\n", val);
}

#define CPLD_D_RST_REG                0x09
#define CPLD_D_SYS_RST_BIT     3 //systemp RST bit 0: reset low active, write and clear 1:Normal operation
#define CPLD_D_COLD_RST_FLAG_BIT     4
#define CPLD_D_WARM_RST_FLAG_BIT     5
static ssize_t set_sys_rst(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    u8 reg, data = 0;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    reg = CPLD_D_RST_REG;
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    if(val)
        CLEAR_BIT(data, CPLD_D_SYS_RST_BIT);
    else
        SET_BIT(data, CPLD_D_SYS_RST_BIT);

    pegatron_fn8656_bnf_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t show_sys_rst(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = CPLD_D_RST_REG;

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, CPLD_D_SYS_RST_BIT, val);

    return sprintf(buf, "0x%02x\n", val);
}

#define CPLD_D_WDT_REG 0x12
#define CPLD_D_WATCHDOG_TMO_BIT     6
extern int pega_mcu_read(unsigned short addr, u8 reg);
extern int pega_mcu_write(unsigned short addr, u8 reg, u8 val);
#define MCU_MB_ADDRESS               0x70
static ssize_t show_reboot_cause(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, watchdog = 0, reg = CPLD_D_WDT_REG;
    u8 reboot_cause = 0;
    u8 cold_rst = 0, warm_rst = 0, pwr_down = 0;

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, CPLD_D_WATCHDOG_TMO_BIT, watchdog);
    reg = CPLD_D_RST_REG;
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, CPLD_D_COLD_RST_FLAG_BIT, cold_rst);
    GET_BIT(data, CPLD_D_WARM_RST_FLAG_BIT, warm_rst);
#if 0//power down reason is from mcu
    pega_mcu_write(MCU_MB_ADDRESS, 0xB0, 0x0);
    pega_mcu_write(MCU_MB_ADDRESS, 0xB1, 0xa0);
    pwr_down = ((pega_mcu_read(MCU_MB_ADDRESS, 0xB2) == 0xff)?0:1);
#endif

    reboot_cause = pwr_down | (watchdog << 1) | (cold_rst << 2) | (warm_rst << 3); 
    return sprintf(buf, "0x%02x\n", reboot_cause);
}

#if PEGA_RD(DSFP)

#define DSFP_PRESENT_ADDRESS        0x21
#define DSFP_RESET_ADDRESS_BASE     0X25
#define DSFP_IRQ_STATUS_ADDRESS     0x2C
#define DSFP_LOW_POWER_ADDRESS      0x30

#define GET_DSFP_STATUS_ADDRESS(idx, reg) \
    do{\
        if(idx >=0 && idx < 24)\
        {\
            reg = DSFP_PRESENT_ADDRESS + ((idx % 24) / 8);\
        }\
        else if(idx >=24 && idx < 28)\
        {\
            reg = 0x24;\
        }\
        else if(idx >=28 && idx < 36)\
        {\
            reg = 0x21;\
        }\
        else if (idx >= 36 && idx < 44)\
        {\
            reg = 0x22;\
        }\
        else if(idx >=44 && idx <48)\
        {\
            reg = 0x23;\
        }\
    } while(0)

#define GET_DSFP_RST_ADDRESS(idx, reg) \
            reg = (DSFP_RESET_ADDRESS_BASE + ((idx % 28) / 4)); 

#define GET_DSFP_LOWPOWER_ADDRESS(idx, reg) \
            reg = DSFP_LOW_POWER_ADDRESS + ((idx % 28) / 8)

#define GET_DSFP_IRQ_STATUS_ADDRESS(idx, reg) \
            reg = DSFP_IRQ_STATUS_ADDRESS + ((idx % 28) / 8)


static ssize_t get_dsfp_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg;

    GET_DSFP_STATUS_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "attr->index:%d,addr: 0x%x, reg: %x, data: %x\r\n",attr->index, client->addr, reg, data);

    if(requirement_flag & KS_REQUIREMENT)
    {
        data = ~data;
    }
    if(attr->index >= 28)
        GET_BIT(data, (attr->index - 4) % 8, val);
    else
        GET_BIT(data, attr->index % 8, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_dsfp_irq_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg;

    GET_DSFP_IRQ_STATUS_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, attr->index % 8, val);

    if(requirement_flag & KS_REQUIREMENT)
    {
        GET_BIT(~data, attr->index % 8, val);
    }
    return sprintf(buf, "%d\n", val);
}

static ssize_t set_dsfp_power_on(struct device *dev, struct device_attribute *da,
              const char *buf, size_t count)
{
    return count;
}

static ssize_t get_dsfp_power_on(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 val = 1;
    return sprintf(buf, "%d\n", val);
}

/*DSFP CPLD 0:reset  1:not reset. KWAI 0:not reset 1:reset*/
static ssize_t get_dsfp_reset(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg, data =0;

    GET_DSFP_RST_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    data = (data >> ((attr->index % 4)*2)) & 0x3;

    return sprintf(buf, "%d\n", !data);
}

static ssize_t set_dsfp_reset(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg, data = 0;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    GET_DSFP_RST_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    CLEAR_BIT(data, (attr->index % 4)*2);
    CLEAR_BIT(data, ((attr->index % 4)*2+1));
    data |= ((!val) & 0x3) << ((attr->index % 4)*2);

    pegatron_fn8656_bnf_cpld_write(client->addr, reg, data);

    return count;
}

/*DSFP CPLD 0:Low  1:High. KWAI 0:High 1:Low*/
static ssize_t get_dsfp_lowpower(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg;
    u32 port_bit = 0;
    if((attr->index >= 44 && attr->index < 48) || (attr->index >= 24 && attr->index < 28))
        port_bit = attr->index % 4;
    else
        port_bit =  attr->index % 8;

    GET_DSFP_LOWPOWER_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, port_bit, val);
    return sprintf(buf, "0x%02x\n", !val);//convert val for Kwai requirement
}

static ssize_t set_dsfp_lowpower(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg;
    long val = 0;
    u32 port_bit = 0;
    if((attr->index >= 44 && attr->index < 48) || (attr->index >= 24 && attr->index < 28))
        port_bit = attr->index % 4;
    else
        port_bit =  attr->index % 8;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    GET_DSFP_LOWPOWER_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    if(val)//convert val for Kwai requirement
        CLEAR_BIT(data, port_bit);
    else
        SET_BIT(data, port_bit);

    pegatron_fn8656_bnf_cpld_write(client->addr, reg, data);

    return count;
}
#endif //DSFP


#if PEGA_RD(QSFP)

#define QSFP_PRESENT_ADDRESS        0x24
#define QSFP_RESET_ADDRESS_BASE     0X2A
#define QSFP_IRQ_STATUS_ADDRESS     0x2F
#define QSFP_LOW_POWER_ADDRESS      0x33


#define GET_QSFP_STATUS_ADDRESS(idx, reg) \
            reg = QSFP_PRESENT_ADDRESS

#define GET_QSFP_RST_ADDRESS(idx, reg) \
            reg = QSFP_RESET_ADDRESS_BASE + ((idx % QSFP_FIRST_PORT) / 4)

#define GET_QSFP_LOWPOWER_ADDRESS(idx, reg) \
            reg = QSFP_LOW_POWER_ADDRESS

#define GET_QSFP_IRQ_STATUS_ADDRESS(idx, reg) \
            reg = QSFP_IRQ_STATUS_ADDRESS

static ssize_t get_qsfp_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg;

    GET_QSFP_STATUS_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, attr->index % 8, val);

    if(requirement_flag & KS_REQUIREMENT)
    {
        GET_BIT(~data, attr->index % 8, val);
    }
    return sprintf(buf, "%d\n", val);
}
static ssize_t get_qsfp_irq_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg;

    GET_QSFP_IRQ_STATUS_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, attr->index % 8, val);

    if(requirement_flag & KS_REQUIREMENT)
    {
        GET_BIT(~data, attr->index % 8, val);
    }
    return sprintf(buf, "%d\n", val);
}
static ssize_t get_qsfp_power_on(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 val = 1;
    return sprintf(buf, "%d\n", val);
}

/*QSFP CPLD 0:reset  1:not reset. KWAI 0:not reset 1:reset*/
static ssize_t get_qsfp_reset(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg, data =0;

    GET_QSFP_RST_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    data = (data >> ((attr->index % 4)*2)) & 0x3;

    return sprintf(buf, "%d\n", !data);
}

static ssize_t set_qsfp_reset(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg, data = 0;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    GET_QSFP_RST_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    CLEAR_BIT(data, (attr->index % 4)*2);
    CLEAR_BIT(data, ((attr->index % 4)*2+1));
    data |= ((!val) & 0x3) << ((attr->index % 4)*2);

    pegatron_fn8656_bnf_cpld_write(client->addr, reg, data);

    return count;
}

/*QSFP-DD CPLD 0:High  1:Low. KWAI 0:High 1:Low*/
static ssize_t get_qsfp_lowpower(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg;

    GET_QSFP_LOWPOWER_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, (attr->index % 8), val);
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t set_qsfp_lowpower(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    GET_QSFP_LOWPOWER_ADDRESS(attr->index, reg);
    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    if(val)
        SET_BIT(data, (attr->index % 8));
    else
        CLEAR_BIT(data, (attr->index % 8));

    pegatron_fn8656_bnf_cpld_write(client->addr, reg, data);

    return count;
}
#endif //QSFP

#define MAX_SFP_NUM_BIT_MASK  0xffffffffffffffUL   //56 sfp

static ssize_t get_all_port_sfp_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 data1 = 0, data2 = 0,i = 0;
    ssize_t present_bit_map = 0 , tmp = 0;
    for (i = 0 ; i < 4;i++)
    {
        data1 = pegatron_fn8656_bnf_cpld_read(CPLDB_ADDRESS, DSFP_PRESENT_ADDRESS + i);
        data2 = pegatron_fn8656_bnf_cpld_read(CPLDC_ADDRESS, DSFP_PRESENT_ADDRESS + i);
        present_bit_map |= (data1 & 0xffUL) << i*8 | (data2 & 0xffUL) << (32 + i*8);
    }

    /*8 registers,only 56bits in use, bit29-32 & bit53-56 reserved */
    tmp = (present_bit_map & 0xfffffffUL) | ((present_bit_map >> 4) & ~0xfffffffUL);
    present_bit_map = (tmp & 0xffffffffffffUL) | ((tmp >> 4) & ~0xffffffffffffUL);
    if(requirement_flag & KS_REQUIREMENT)
    {
        present_bit_map = (~present_bit_map) & MAX_SFP_NUM_BIT_MASK;
    }

    pega_print(DEBUG, "present_bit_map:0x%lx\r\n", present_bit_map);

    return sprintf(buf, "0x%lx\n", present_bit_map);
}

static ssize_t get_all_port_sfp_power_on(struct device *dev, struct device_attribute *da,
             char *buf)
{
    size_t poweron_bit_map = MAX_SFP_NUM_BIT_MASK;
    pega_print(DEBUG, "poweron_bit_map:%lx\r\n", poweron_bit_map);
    return sprintf(buf, "0x%lx\n", poweron_bit_map);
}

static ssize_t get_cpld_alias(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    ssize_t ret_len = 0;
    
    switch(client->addr)
    {
        case CPLDA_ADDRESS:
            ret_len = sprintf(buf, "CPLD-A");
            break;
        case CPLDB_ADDRESS:
            ret_len = sprintf(buf, "CPLD-B");
            break;
        case CPLDC_ADDRESS:
            ret_len = sprintf(buf, "CPLD-C");
            break;
	case CPLDD_ADDRESS:
            ret_len = sprintf(buf, "CPLD-D");
            break;
        default:
            pega_print(WARNING, "Error address %x\n", client->addr);
            break;
    }

    return ret_len;
}

static ssize_t get_cpld_type(struct device *dev, struct device_attribute *da,
             char *buf)
{
    return sprintf(buf, "LCMXO2-1200HC\n");
}
static ssize_t get_cpld_board_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_VERSION_REG;

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);

    return sprintf(buf, "0x%02x\n", (data & CPLD_HW_VERSION_BIT_MSK) >> CPLD_HW_VERSION_BIT_OFFSET);
}
static ssize_t get_cpld_num(struct device *dev, struct device_attribute *da,
             char *buf)
{
    return sprintf(buf, "%d\n",CPLD_MAX_NUM);
}
static ssize_t get_sfp_num(struct device *dev, struct device_attribute *da,
             char *buf)
{
    return sprintf(buf, "%d\n",SFP_PORT_MAX_NUM);
}
static ssize_t get_cpld_debug(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int count = 0;

    switch (attr->index) {
    case 0:
        count = sprintf(buf, "%s\n", sysled_debug);
        break;
    case 1:
        count = sprintf(buf, "%s\n", sfp_debug);
        break;
    case 2:
        count = sprintf(buf, "%s\n", cpld_debug);
        break;
    default:
        pega_print(WARNING,"no debug info\n");
    }
    return count;
}

static ssize_t set_cpld_debug(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int debug_len = (count < MAX_DEBUG_INFO_LEN) ? count : MAX_DEBUG_INFO_LEN;
    
    switch (attr->index) {
    case 0:
        memset(sysled_debug,0,strlen(sysled_debug));
        strncpy(sysled_debug, buf, debug_len);
        break;
    case 1:
        memset(sfp_debug,0,strlen(sysled_debug));
        strncpy(sfp_debug, buf, debug_len);
        break;
    case 2:
        memset(cpld_debug,0,strlen(sysled_debug));
        strncpy(cpld_debug, buf, debug_len);
        break;
    default:
        pega_print(WARNING,"no debug info\n");
    }
    return debug_len;
}

/*
static ssize_t set_sfp_debug(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    strncpy(sysled_debug,buf, (count < MAX_DEBUG_INFO_LEN) ? count : MAX_DEBUG_INFO_LEN);

    return (count < MAX_DEBUG_INFO_LEN) ? count : MAX_DEBUG_INFO_LEN;
}

static ssize_t set_sysled_debug(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    strncpy(sysled_debug,buf, (count < MAX_DEBUG_INFO_LEN) ? count : MAX_DEBUG_INFO_LEN);

    return (count < MAX_DEBUG_INFO_LEN) ? count : MAX_DEBUG_INFO_LEN;
}
*/

static SENSOR_DEVICE_ATTR(cpld_hw_version,  S_IRUGO, read_cpld_HWversion, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_sw_version,  S_IRUGO, read_cpld_SWversion, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_alias,  S_IRUGO, get_cpld_alias, NULL, 0); 
static SENSOR_DEVICE_ATTR(cpld_type,  S_IRUGO, get_cpld_type, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_board_version,  S_IRUGO, get_cpld_board_version, NULL, 0);  
static SENSOR_DEVICE_ATTR(cpld_num,  S_IRUGO, get_cpld_num, NULL, 0);  
static SENSOR_DEVICE_ATTR(sfp_port_num,  S_IRUGO, get_sfp_num, NULL, 0);  
static SENSOR_DEVICE_ATTR(debug_cpld,  S_IRUGO | S_IWUSR, get_cpld_debug, set_cpld_debug, 2); 
static SENSOR_DEVICE_ATTR(debug_sfp,  S_IRUGO | S_IWUSR, get_cpld_debug, set_cpld_debug, 1); 
static SENSOR_DEVICE_ATTR(debug_sysled,  S_IRUGO | S_IWUSR, get_cpld_debug, set_cpld_debug, 0); 

static SENSOR_DEVICE_ATTR(cpld_allled_ctrl,  S_IRUGO | S_IWUSR, show_led_status, set_led_status, ALL_LED_CTRL);
static SENSOR_DEVICE_ATTR(serial_led_enable,  S_IRUGO | S_IWUSR, show_led_status, set_led_status, SERIAL_LED_CTRL);
static SENSOR_DEVICE_ATTR(sys_led,  S_IRUGO | S_IWUSR, show_led_status, set_led_status, SYS_LED_CTRL);
static SENSOR_DEVICE_ATTR(pwr_led,  S_IRUGO , show_led_status, set_led_status, PWR_LED_CTRL);
static SENSOR_DEVICE_ATTR(fan_led,  S_IRUGO , show_led_status, set_led_status, FAN_LED_CTRL);
static SENSOR_DEVICE_ATTR(loc_led,  S_IRUGO | S_IWUSR, show_led_status, set_led_status, LOC_LED_CTRL);
static SENSOR_DEVICE_ATTR(sys_rst,  S_IRUGO | S_IWUSR, show_sys_rst, set_sys_rst, 0);
static SENSOR_DEVICE_ATTR(reboot_cause,  S_IRUGO , show_reboot_cause, NULL, 0);

static SENSOR_DEVICE_ATTR(eeprom_write_enable,  S_IRUGO | S_IWUSR, show_eeprom_write_enable, set_eeprom_write_enable, 0);
static SENSOR_DEVICE_ATTR(psu_1_status,  S_IRUGO, read_psu_status, NULL, 0);
static SENSOR_DEVICE_ATTR(psu_2_status,  S_IRUGO, read_psu_status, NULL, 1);
static SENSOR_DEVICE_ATTR(int_status,  S_IRUGO, read_int_status, NULL, 0);

static SENSOR_DEVICE_ATTR(sfp_all_present,  S_IRUGO, get_all_port_sfp_present, NULL, 0); 
static SENSOR_DEVICE_ATTR(sfp_all_power_on,  S_IRUGO, get_all_port_sfp_power_on, NULL, 0); 

#define SET_DSFP_ATTR(_num) \
        static SENSOR_DEVICE_ATTR(sfp##_num##_present,  S_IRUGO, get_dsfp_present, NULL, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_reset,  S_IRUGO | S_IWUSR, get_dsfp_reset, set_dsfp_reset, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_lowpower,  S_IRUGO | S_IWUSR, get_dsfp_lowpower, set_dsfp_lowpower, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_irq_status,  S_IRUGO, get_dsfp_irq_status, NULL, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_power_on,  S_IRUGO | S_IWUSR, get_dsfp_power_on, set_dsfp_power_on,_num-1)

#define SET_QSFP_ATTR(_num) \
        static SENSOR_DEVICE_ATTR(sfp##_num##_present,  S_IRUGO, get_qsfp_present, NULL, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_reset,  S_IRUGO | S_IWUSR, get_qsfp_reset, set_qsfp_reset, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_lowpower,  S_IRUGO | S_IWUSR, get_qsfp_lowpower, set_qsfp_lowpower, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_irq_status,  S_IRUGO, get_qsfp_irq_status, NULL, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_power_on,  S_IRUGO | S_IWUSR, get_qsfp_power_on, NULL,_num-1)

SET_DSFP_ATTR(1);SET_DSFP_ATTR(2);SET_DSFP_ATTR(3);SET_DSFP_ATTR(4);SET_DSFP_ATTR(5);SET_DSFP_ATTR(6);SET_DSFP_ATTR(7);SET_DSFP_ATTR(8);SET_DSFP_ATTR(9);
SET_DSFP_ATTR(10);SET_DSFP_ATTR(11);SET_DSFP_ATTR(12);SET_DSFP_ATTR(13);SET_DSFP_ATTR(14);SET_DSFP_ATTR(15);SET_DSFP_ATTR(16);SET_DSFP_ATTR(17);SET_DSFP_ATTR(18);
SET_DSFP_ATTR(19);SET_DSFP_ATTR(20);SET_DSFP_ATTR(21);SET_DSFP_ATTR(22);SET_DSFP_ATTR(23);SET_DSFP_ATTR(24);SET_DSFP_ATTR(25);SET_DSFP_ATTR(26);SET_DSFP_ATTR(27);
SET_DSFP_ATTR(28);SET_DSFP_ATTR(29);SET_DSFP_ATTR(30);SET_DSFP_ATTR(31);SET_DSFP_ATTR(32);SET_DSFP_ATTR(33);SET_DSFP_ATTR(34);SET_DSFP_ATTR(35);SET_DSFP_ATTR(36);
SET_DSFP_ATTR(37);SET_DSFP_ATTR(38);SET_DSFP_ATTR(39);SET_DSFP_ATTR(40);SET_DSFP_ATTR(41);SET_DSFP_ATTR(42);SET_DSFP_ATTR(43);SET_DSFP_ATTR(44);SET_DSFP_ATTR(45);
SET_DSFP_ATTR(46);SET_DSFP_ATTR(47);SET_DSFP_ATTR(48);
SET_QSFP_ATTR(49);SET_QSFP_ATTR(50);SET_QSFP_ATTR(51);SET_QSFP_ATTR(52);SET_QSFP_ATTR(53);SET_QSFP_ATTR(54);SET_QSFP_ATTR(55);SET_QSFP_ATTR(56);

static struct attribute *pegatron_fn8656_bnf_cpldA_attributes[] = {
    &sensor_dev_attr_cpld_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_sw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_alias.dev_attr.attr,
    &sensor_dev_attr_cpld_type.dev_attr.attr,
    &sensor_dev_attr_cpld_board_version.dev_attr.attr,
    &sensor_dev_attr_cpld_num.dev_attr.attr,
    &sensor_dev_attr_debug_cpld.dev_attr.attr,
    &sensor_dev_attr_debug_sfp.dev_attr.attr,
    &sensor_dev_attr_debug_sysled.dev_attr.attr,

    &sensor_dev_attr_cpld_allled_ctrl.dev_attr.attr,
    &sensor_dev_attr_serial_led_enable.dev_attr.attr,
    &sensor_dev_attr_sys_led.dev_attr.attr,
    &sensor_dev_attr_pwr_led.dev_attr.attr,
    &sensor_dev_attr_fan_led.dev_attr.attr,
    &sensor_dev_attr_eeprom_write_enable.dev_attr.attr,

    &sensor_dev_attr_psu_1_status.dev_attr.attr,
    &sensor_dev_attr_psu_2_status.dev_attr.attr,
    &sensor_dev_attr_int_status.dev_attr.attr,
    NULL
};

static struct attribute *pegatron_fn8656_bnf_cpldB_attributes[] = {
    &sensor_dev_attr_cpld_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_sw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_alias.dev_attr.attr,
    &sensor_dev_attr_cpld_type.dev_attr.attr,
    &sensor_dev_attr_cpld_board_version.dev_attr.attr,
    &sensor_dev_attr_sfp_port_num.dev_attr.attr,

    &sensor_dev_attr_serial_led_enable.dev_attr.attr,

    &sensor_dev_attr_sfp_all_present.dev_attr.attr,
    &sensor_dev_attr_sfp_all_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp1_present.dev_attr.attr,
    &sensor_dev_attr_sfp1_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp1_reset.dev_attr.attr,
    &sensor_dev_attr_sfp1_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp1_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp2_present.dev_attr.attr,
    &sensor_dev_attr_sfp2_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp2_reset.dev_attr.attr,
    &sensor_dev_attr_sfp2_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp2_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp3_present.dev_attr.attr,
    &sensor_dev_attr_sfp3_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp3_reset.dev_attr.attr,
    &sensor_dev_attr_sfp3_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp3_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp4_present.dev_attr.attr,
    &sensor_dev_attr_sfp4_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp4_reset.dev_attr.attr,
    &sensor_dev_attr_sfp4_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp4_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp5_present.dev_attr.attr,
    &sensor_dev_attr_sfp5_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp5_reset.dev_attr.attr,
    &sensor_dev_attr_sfp5_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp5_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp6_present.dev_attr.attr,
    &sensor_dev_attr_sfp6_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp6_reset.dev_attr.attr,
    &sensor_dev_attr_sfp6_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp6_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp7_present.dev_attr.attr,
    &sensor_dev_attr_sfp7_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp7_reset.dev_attr.attr,
    &sensor_dev_attr_sfp7_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp7_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp8_present.dev_attr.attr,
    &sensor_dev_attr_sfp8_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp8_reset.dev_attr.attr,
    &sensor_dev_attr_sfp8_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp8_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp9_present.dev_attr.attr,
    &sensor_dev_attr_sfp9_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp9_reset.dev_attr.attr,
    &sensor_dev_attr_sfp9_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp9_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp10_present.dev_attr.attr,
    &sensor_dev_attr_sfp10_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp10_reset.dev_attr.attr,
    &sensor_dev_attr_sfp10_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp10_power_on.dev_attr.attr,
 
    &sensor_dev_attr_sfp11_present.dev_attr.attr,
    &sensor_dev_attr_sfp11_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp11_reset.dev_attr.attr,
    &sensor_dev_attr_sfp11_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp11_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp12_present.dev_attr.attr,
    &sensor_dev_attr_sfp12_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp12_reset.dev_attr.attr,
    &sensor_dev_attr_sfp12_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp12_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp13_present.dev_attr.attr,
    &sensor_dev_attr_sfp13_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp13_reset.dev_attr.attr,
    &sensor_dev_attr_sfp13_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp13_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp14_present.dev_attr.attr,
    &sensor_dev_attr_sfp14_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp14_reset.dev_attr.attr,
    &sensor_dev_attr_sfp14_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp14_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp15_present.dev_attr.attr,
    &sensor_dev_attr_sfp15_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp15_reset.dev_attr.attr,
    &sensor_dev_attr_sfp15_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp15_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp16_present.dev_attr.attr,
    &sensor_dev_attr_sfp16_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp16_reset.dev_attr.attr,
    &sensor_dev_attr_sfp16_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp16_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp17_present.dev_attr.attr,
    &sensor_dev_attr_sfp17_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp17_reset.dev_attr.attr,
    &sensor_dev_attr_sfp17_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp17_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp18_present.dev_attr.attr,
    &sensor_dev_attr_sfp18_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp18_reset.dev_attr.attr,
    &sensor_dev_attr_sfp18_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp18_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp19_present.dev_attr.attr,
    &sensor_dev_attr_sfp19_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp19_reset.dev_attr.attr,
    &sensor_dev_attr_sfp19_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp19_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp20_present.dev_attr.attr,
    &sensor_dev_attr_sfp20_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp20_reset.dev_attr.attr,
    &sensor_dev_attr_sfp20_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp20_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp21_present.dev_attr.attr,
    &sensor_dev_attr_sfp21_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp21_reset.dev_attr.attr,
    &sensor_dev_attr_sfp21_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp21_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp22_present.dev_attr.attr,
    &sensor_dev_attr_sfp22_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp22_reset.dev_attr.attr,
    &sensor_dev_attr_sfp22_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp22_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp23_present.dev_attr.attr,
    &sensor_dev_attr_sfp23_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp23_reset.dev_attr.attr,
    &sensor_dev_attr_sfp23_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp23_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp24_present.dev_attr.attr,
    &sensor_dev_attr_sfp24_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp24_reset.dev_attr.attr,
    &sensor_dev_attr_sfp24_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp24_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp25_present.dev_attr.attr,
    &sensor_dev_attr_sfp25_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp25_reset.dev_attr.attr,
    &sensor_dev_attr_sfp25_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp25_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp26_present.dev_attr.attr,
    &sensor_dev_attr_sfp26_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp26_reset.dev_attr.attr,
    &sensor_dev_attr_sfp26_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp26_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp27_present.dev_attr.attr,
    &sensor_dev_attr_sfp27_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp27_reset.dev_attr.attr,
    &sensor_dev_attr_sfp27_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp27_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp28_present.dev_attr.attr,
    &sensor_dev_attr_sfp28_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp28_reset.dev_attr.attr,
    &sensor_dev_attr_sfp28_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp28_power_on.dev_attr.attr,

    NULL
};

static struct attribute* pegatron_fn8656_bnf_cpldC_attributes[] = {
    &sensor_dev_attr_cpld_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_sw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_alias.dev_attr.attr,
    &sensor_dev_attr_cpld_type.dev_attr.attr,
    &sensor_dev_attr_cpld_board_version.dev_attr.attr,

    &sensor_dev_attr_serial_led_enable.dev_attr.attr,

    &sensor_dev_attr_sfp_all_present.dev_attr.attr,
    &sensor_dev_attr_sfp_all_power_on.dev_attr.attr,

    /*dsfp 29-48*/
    &sensor_dev_attr_sfp29_present.dev_attr.attr,
    &sensor_dev_attr_sfp29_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp29_reset.dev_attr.attr,
    &sensor_dev_attr_sfp29_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp29_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp30_present.dev_attr.attr,
    &sensor_dev_attr_sfp30_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp30_reset.dev_attr.attr,
    &sensor_dev_attr_sfp30_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp30_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp31_present.dev_attr.attr,
    &sensor_dev_attr_sfp31_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp31_reset.dev_attr.attr,
    &sensor_dev_attr_sfp31_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp31_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp32_present.dev_attr.attr,
    &sensor_dev_attr_sfp32_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp32_reset.dev_attr.attr,
    &sensor_dev_attr_sfp32_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp32_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp33_present.dev_attr.attr,
    &sensor_dev_attr_sfp33_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp33_reset.dev_attr.attr,
    &sensor_dev_attr_sfp33_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp33_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp34_present.dev_attr.attr,
    &sensor_dev_attr_sfp34_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp34_reset.dev_attr.attr,
    &sensor_dev_attr_sfp34_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp34_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp35_present.dev_attr.attr,
    &sensor_dev_attr_sfp35_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp35_reset.dev_attr.attr,
    &sensor_dev_attr_sfp35_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp35_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp36_present.dev_attr.attr,
    &sensor_dev_attr_sfp36_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp36_reset.dev_attr.attr,
    &sensor_dev_attr_sfp36_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp36_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp37_present.dev_attr.attr,
    &sensor_dev_attr_sfp37_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp37_reset.dev_attr.attr,
    &sensor_dev_attr_sfp37_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp37_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp38_present.dev_attr.attr,
    &sensor_dev_attr_sfp38_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp38_reset.dev_attr.attr,
    &sensor_dev_attr_sfp38_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp38_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp39_present.dev_attr.attr,
    &sensor_dev_attr_sfp39_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp39_reset.dev_attr.attr,
    &sensor_dev_attr_sfp39_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp39_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp40_present.dev_attr.attr,
    &sensor_dev_attr_sfp40_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp40_reset.dev_attr.attr,
    &sensor_dev_attr_sfp40_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp40_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp41_present.dev_attr.attr,
    &sensor_dev_attr_sfp41_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp41_reset.dev_attr.attr,
    &sensor_dev_attr_sfp41_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp41_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp42_present.dev_attr.attr,
    &sensor_dev_attr_sfp42_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp42_reset.dev_attr.attr,
    &sensor_dev_attr_sfp42_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp42_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp43_present.dev_attr.attr,
    &sensor_dev_attr_sfp43_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp43_reset.dev_attr.attr,
    &sensor_dev_attr_sfp43_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp43_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp44_present.dev_attr.attr,
    &sensor_dev_attr_sfp44_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp44_reset.dev_attr.attr,
    &sensor_dev_attr_sfp44_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp44_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp45_present.dev_attr.attr,
    &sensor_dev_attr_sfp45_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp45_reset.dev_attr.attr,
    &sensor_dev_attr_sfp45_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp45_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp46_present.dev_attr.attr,
    &sensor_dev_attr_sfp46_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp46_reset.dev_attr.attr,
    &sensor_dev_attr_sfp46_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp46_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp47_present.dev_attr.attr,
    &sensor_dev_attr_sfp47_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp47_reset.dev_attr.attr,
    &sensor_dev_attr_sfp47_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp47_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp48_present.dev_attr.attr,
    &sensor_dev_attr_sfp48_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp48_reset.dev_attr.attr,
    &sensor_dev_attr_sfp48_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp48_power_on.dev_attr.attr,

    /*qsfp 49-56*/
    &sensor_dev_attr_sfp49_present.dev_attr.attr,
    &sensor_dev_attr_sfp49_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp49_reset.dev_attr.attr,
    &sensor_dev_attr_sfp49_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp49_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp50_present.dev_attr.attr,
    &sensor_dev_attr_sfp50_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp50_reset.dev_attr.attr,
    &sensor_dev_attr_sfp50_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp50_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp51_present.dev_attr.attr,
    &sensor_dev_attr_sfp51_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp51_reset.dev_attr.attr,
    &sensor_dev_attr_sfp51_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp51_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp52_present.dev_attr.attr,
    &sensor_dev_attr_sfp52_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp52_reset.dev_attr.attr,
    &sensor_dev_attr_sfp52_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp52_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp53_present.dev_attr.attr,
    &sensor_dev_attr_sfp53_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp53_reset.dev_attr.attr,
    &sensor_dev_attr_sfp53_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp53_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp54_present.dev_attr.attr,
    &sensor_dev_attr_sfp54_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp54_reset.dev_attr.attr,
    &sensor_dev_attr_sfp54_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp54_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp55_present.dev_attr.attr,
    &sensor_dev_attr_sfp55_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp55_reset.dev_attr.attr,
    &sensor_dev_attr_sfp55_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp55_power_on.dev_attr.attr,

    &sensor_dev_attr_sfp56_present.dev_attr.attr,
    &sensor_dev_attr_sfp56_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp56_reset.dev_attr.attr,
    &sensor_dev_attr_sfp56_irq_status.dev_attr.attr,
    &sensor_dev_attr_sfp56_power_on.dev_attr.attr,
    NULL
};

static struct attribute *pegatron_fn8656_bnf_cpldD_attributes[] = {
    &sensor_dev_attr_cpld_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_sw_version.dev_attr.attr,
    &sensor_dev_attr_loc_led.dev_attr.attr,
    &sensor_dev_attr_cpld_alias.dev_attr.attr,
    &sensor_dev_attr_cpld_type.dev_attr.attr,
    &sensor_dev_attr_cpld_board_version.dev_attr.attr,
    &sensor_dev_attr_sys_rst.dev_attr.attr,
    &sensor_dev_attr_reboot_cause.dev_attr.attr,

	NULL
};

static ssize_t get_lm75b_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev); 
    u16 data = 0;  
    u8 reg = LM75B_TEMP_REG;

    data = pegatron_fn8656_bnf_cpld_read(client->addr, reg);
    pega_print(DEBUG,"%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data);
	sprintf(buf, "%d\n", data);
	return strlen(buf);

}

static SENSOR_DEVICE_ATTR(lm75b_temp,  S_IRUGO, get_lm75b_temp, NULL, 0);

static struct attribute *pegatron_fn8656_bnf_lm75b_attributes[] = {
	&sensor_dev_attr_lm75b_temp.dev_attr.attr,
	NULL
};

static const struct attribute_group pegatron_fn8656_bnf_cpldA_group = { .attrs = pegatron_fn8656_bnf_cpldA_attributes};
static const struct attribute_group pegatron_fn8656_bnf_cpldB_group = { .attrs = pegatron_fn8656_bnf_cpldB_attributes};
static const struct attribute_group pegatron_fn8656_bnf_cpldC_group = { .attrs = pegatron_fn8656_bnf_cpldC_attributes};
static const struct attribute_group pegatron_fn8656_bnf_cpldD_group = { .attrs = pegatron_fn8656_bnf_cpldD_attributes};
static const struct attribute_group pegatron_fn8656_bnf_lm75b_group = { .attrs = pegatron_fn8656_bnf_lm75b_attributes};

static void pegatron_fn8656_bnf_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);
    
    if (!node) {
        pega_print(ERR, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }
    
    node->client = client;
    
    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void pegatron_fn8656_bnf_cpld_remove_client(struct i2c_client *client)
{
    struct list_head        *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client == client) {
            found = 1;
            break;
        }
    }
    
    if (found) {
        list_del(list_node);
        kfree(cpld_node);
    }
    
    mutex_unlock(&list_lock);
}

static int pegatron_fn8656_bnf_cpld_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{  
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        pega_print(ERR, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    /* Register sysfs hooks */
    switch(client->addr)
    {
        case CPLDA_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8656_bnf_cpldA_group);
            break;
        case CPLDB_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8656_bnf_cpldB_group);
            break;
        case CPLDC_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8656_bnf_cpldC_group);
            break;
	case CPLDD_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8656_bnf_cpldD_group);
            break;
	case LM75B_ADDRESS:
           status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8656_bnf_lm75b_group);
	   break;
        default:
            pega_print(WARNING, "i2c_check_CPLD failed (0x%x)\n", client->addr);
            status = -EIO;
            goto exit;
            break;
    }

    if (status) {
        goto exit;
    }

    pega_print(INFO, "chip found\n");
    pegatron_fn8656_bnf_cpld_add_client(client);
    
    return 0; 

exit:
    return status;
}

static int pegatron_fn8656_bnf_cpld_remove(struct i2c_client *client)
{
    switch(client->addr)
    {
        case CPLDA_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8656_bnf_cpldA_group);
            break;
        case CPLDB_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8656_bnf_cpldB_group);
            break;
        case CPLDC_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8656_bnf_cpldC_group);
            break;
		case CPLDD_ADDRESS:	
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8656_bnf_cpldD_group);
            break;
		case LM75B_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8656_bnf_lm75b_group);
			break;
        default:
            pega_print(WARNING, "i2c_remove_CPLD failed (0x%x)\n", client->addr);
            break;
    }

  
    pegatron_fn8656_bnf_cpld_remove_client(client);  
    return 0;
}

static const struct i2c_device_id pegatron_fn8656_bnf_cpld_id[] = {
    { "fn8656_bnf_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, pegatron_fn8656_bnf_cpld_id);

static struct i2c_driver pegatron_fn8656_bnf_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "pegatron_fn8656_bnf_cpld",
    },
    .probe      = pegatron_fn8656_bnf_cpld_probe,
    .remove     = pegatron_fn8656_bnf_cpld_remove,
    .id_table   = pegatron_fn8656_bnf_cpld_id,
    .address_list = normal_i2c,
};

static int __init pegatron_fn8656_bnf_cpld_init(void)
{
    mutex_init(&list_lock);

    return i2c_add_driver(&pegatron_fn8656_bnf_cpld_driver);
}

static void __exit pegatron_fn8656_bnf_cpld_exit(void)
{
    i2c_del_driver(&pegatron_fn8656_bnf_cpld_driver);
}

module_param(loglevel,uint,0644);
module_param(requirement_flag,uint,0664);

MODULE_PARM_DESC(loglevel,"0x01-LOG_ERR,0x02-LOG_WARNING,0x04-LOG_INFO,0x08-LOG_DEBUG");
MODULE_PARM_DESC(requirement_flag,"custom requirement.eg:requirement_flag=1 for KS");

MODULE_AUTHOR("Peter5 Lin <Peter5_Lin@pegatroncorp.com.tw>");
MODULE_DESCRIPTION("pegatron_fn8656_bnf_cpld driver");
MODULE_LICENSE("GPL");

module_init(pegatron_fn8656_bnf_cpld_init);
module_exit(pegatron_fn8656_bnf_cpld_exit);
