/*
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

#undef PEGA_DEBUG
/*#define pega_DEBUG*/
#ifdef PEGA_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif /* DEBUG */

#define PSU_EEPROM_FRU_50_ADDRESS 0x50
#define PSU_EEPROM_FRU_51_ADDRESS 0x51
#define PSU_58_ADDRESS 0x58
#define PSU_59_ADDRESS 0x59
#define PSU_VOUT_MODE_REG 0x20
#define PSU_VOUT_STATUS_REG 0x7A
#define PSU_IOUT_STATUS_REG 0x7B
#define PSU_INPUT_STATUS_REG 0x7C
#define PSU_TEMP_STATUS_REG 0x7D
#define PSU_FANS_1_2_STATUS 0x81


#define PSU_VOUT_OVER_VOLTAGE_BIT 7
#define PSU_IOUT_OVER_CURRENT_FAULT_BIT 7
#define PSU_IOUT_OVER_CURRENT_WARNING_BIT 5
#define PSU_IPUT_OVER_CURRENT_WARNING_BIT 1
#define PSU_IPUT_INSUFFICIENT_BIT 3
#define PSU_TEMP_OVER_TEMP_FAULT_BIT 7
#define PSU_TEMP_OVER_TEMP_WARNING_BIT 6


#define PM_BUS_BLOCK_MAX_LEN 32

enum _sysfs_attributes {
    PSU_V_IN = 0,
    PSU_I_IN,
    PSU_V_OUT,
    PSU_I_OUT,
    PSU_TEMP1_INPUT,
    PSU_TEMP2_INPUT,
    PSU_TEMP3_INPUT,
    PSU_FAN1_SPEED,

    PSU_P_OUT,
    PSU_P_IN,
    PSU_PMBUS_REVISION,
    PSU_MFR_ID,
    PSU_MFR_MODEL,
    PSU_MFR_REVISION,
    PSU_MFR_LOCATION,
    PSU_MFR_DATE,
    PSU_MFR_SERIAL,

    PSU_VIN_MIN,
    PSU_VIN_MAX,
    PSU_IIN_MAX,
    PSU_PIN_MAX,
    PSU_VOUT_MIN,
    PSU_VOUT_MAX,
    PSU_IOUT_MAX,
    PSU_POUT_MAX,

    //PSU_FAN1_FAULT,
    //PSU_FAN1_DUTY_CYCLE,
    PSU_MFR_TEMP1_MAX,
    PSU_MFR_TEMP2_MAX,
    PSU_MFR_TEMP3_MAX,

    PSU_ATTR_END
};
enum _psu_pmbus_reg{
    PSU_VIN_REG = 0x88,
    PSU_IIN_REG,

    PSU_VOUT_REG = 0x8B,
    PSU_IOUT_REG,
    PSU_TEMP1_REG,
    PSU_TEMP2_REG,
    PSU_TEMP3_REG,
    PSU_FAN_SPEED_1_REG,

    PSU_POUT_REG = 0x96,
    PSU_PIN_REG,
    PSU_PMBUS_REVISION_REG,
    PSU_MFR_ID_REG,
    PSU_MFR_MODEL_REG,
    PSU_MFR_REVISION_REG,
    PSU_MFR_LOCATION_REG,
    PSU_MFR_DATE_REG,
    PSU_MFR_SERIAL_REG,

    PSU_VIN_MIN_REG = 0xA0,
    PSU_VIN_MAX_REG,
    PSU_IIN_MAX_REG,
    PSU_PIN_MAX_REG,
    PSU_VOUT_MIN_REG,
    PSU_VOUT_MAX_REG,
    PSU_IOUT_MAX_REG,
    PSU_POUT_MAX_REG,

    PSU_MFR_TEMP1_MAX_REG = 0xC0,
    PSU_MFR_TEMP2_MAX_REG,
    PSU_MFR_TEMP3_MAX_REG,

};
u8 psu_pmbus_reg_map[PSU_ATTR_END] = {
    PSU_VIN_REG,PSU_IIN_REG,PSU_VOUT_REG,PSU_IOUT_REG,PSU_TEMP1_REG,PSU_TEMP2_REG,PSU_TEMP3_REG,PSU_FAN_SPEED_1_REG,
    PSU_POUT_REG,PSU_PIN_REG,PSU_PMBUS_REVISION_REG,PSU_MFR_ID_REG, PSU_MFR_MODEL_REG,PSU_MFR_REVISION_REG , PSU_MFR_LOCATION_REG,PSU_MFR_DATE_REG ,PSU_MFR_SERIAL_REG,
    PSU_VIN_MIN_REG,PSU_VIN_MAX_REG,PSU_IIN_MAX_REG,PSU_PIN_MAX_REG,PSU_VOUT_MIN_REG,PSU_VOUT_MAX_REG,PSU_IOUT_MAX_REG,PSU_POUT_MAX_REG,
    PSU_MFR_TEMP1_MAX_REG,PSU_MFR_TEMP2_MAX_REG,PSU_MFR_TEMP3_MAX_REG
};

enum power_type {
    POWER_AC = 0,
    POWER_DC = 1
};

#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

struct psu_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

#define MAX_PSU_NUMS    2
#define MAX_PSU_TEMP_SENSORS    3
struct fn8656_bnf_psu_data {
	struct i2c_client	*client;
    uint total_psu_num;
    uint temp_sensor_num;
    enum power_type type;

    int temp_max[MAX_PSU_TEMP_SENSORS];
    int temp_min[MAX_PSU_TEMP_SENSORS];
    int temp_crit[MAX_PSU_TEMP_SENSORS];
};
int psu_temp_max[MAX_PSU_TEMP_SENSORS] = {80, 90, 105};
int psu_temp_min[MAX_PSU_TEMP_SENSORS] = {-2, 7, 14};
int psu_temp_crit[MAX_PSU_TEMP_SENSORS] = {85, 95, 110};
static const unsigned short normal_i2c[] = { PSU_EEPROM_FRU_50_ADDRESS,PSU_EEPROM_FRU_51_ADDRESS,PSU_58_ADDRESS, PSU_59_ADDRESS, I2C_CLIENT_END };
static LIST_HEAD(psu_client_list);
static struct mutex  list_lock;

static uint loglevel = LOG_INFO | LOG_WARNING | LOG_ERR;
static char debug[MAX_DEBUG_INFO_LEN] = "debug info:                     \n\
echo 8 > */loglevel --- log driver's debug details    \n";

static int pega_fn8656_bnf_psu_read(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct psu_client_node *psu_node = NULL;
    int data = -EPERM;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &psu_client_list)
    {
        psu_node = list_entry(list_node, struct psu_client_node, list);
        
        if (psu_node->client->addr == addr) {
            data = i2c_smbus_read_byte_data(psu_node->client, reg);
            pega_print(DEBUG, "addr: 0x%x, reg: 0x%x, data: 0x%x\r\n", addr, reg, data);
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return data;
}

static int pega_fn8656_bnf_psu_read_word(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct psu_client_node *psu_node = NULL;
    int data = -EPERM;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &psu_client_list)
    {
        psu_node = list_entry(list_node, struct psu_client_node, list);
        
        if (psu_node->client->addr == addr) {
            data = i2c_smbus_read_word_data(psu_node->client, reg);
            pega_print(DEBUG, "addr: 0x%x, reg: 0x%x, data: 0x%x\r\n", addr, reg, data);
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return data;
}

static int pega_fn8656_bnf_psu_read_block(unsigned short addr, u8 reg,u8* buf)
{
    struct list_head   *list_node = NULL;
    struct psu_client_node *psu_node = NULL;
    int ret_val = -EPERM,i = 0;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &psu_client_list)
    {
        psu_node = list_entry(list_node, struct psu_client_node, list);
        
        if (psu_node->client->addr == addr) {
            ret_val = i2c_smbus_read_block_data(psu_node->client, reg, buf);
            pega_print(DEBUG, "addr: 0x%x, reg: 0x%x, length:%d buf: ", addr, reg,ret_val);
            for(i = 0;i < ret_val; i++){
                pega_print(DEBUG,"0x%x ",buf[i]);
            }
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return ret_val;
}

static ssize_t read_psu_alarm(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u8 vout_data = 0, temp_data = 0,fan_data = 0,val = 0;

    vout_data = pega_fn8656_bnf_psu_read(client->addr, PSU_VOUT_STATUS_REG);
    temp_data = pega_fn8656_bnf_psu_read(client->addr, PSU_TEMP_STATUS_REG);
    fan_data = pega_fn8656_bnf_psu_read(client->addr, PSU_FANS_1_2_STATUS);
    pega_print(DEBUG, "vout_data:0x%x,temp_data:0x%x,fan_data:0x%x\r\n", vout_data,temp_data,fan_data);
    
    //BIT0: Normal, Bit0: temprature status, BIT1:fan status, BIT2:vol status
    if(temp_data & 0xff)
        SET_BIT(val, 0);
    if(fan_data & 0xff)
        SET_BIT(val, 1);
    if(vout_data & 0xff)
        SET_BIT(val, 2);

    return sprintf(buf, "0x%02x\n", val);
}
static ssize_t get_psu_led_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u8 vout_data = 0, temp_data = 0,fan_data = 0,led_status = 0;

    vout_data = pega_fn8656_bnf_psu_read(client->addr, PSU_VOUT_STATUS_REG);
    temp_data = pega_fn8656_bnf_psu_read(client->addr, PSU_TEMP_STATUS_REG);
    fan_data = pega_fn8656_bnf_psu_read(client->addr, PSU_FANS_1_2_STATUS);
    pega_print(DEBUG, "vout_data:0x%x,temp_data:0x%x,fan_data:0x%x\r\n", vout_data,temp_data,fan_data);
    
    //Kwai's led color definition:1 green,3 red
    if(temp_data & 0xff || fan_data & 0xff || vout_data & 0xff)
    {
        led_status = 3;
    }
    else
    {
        led_status = 1;
    }

    return sprintf(buf, "0x%02x\n", led_status);
}


static ssize_t read_psu_vout_over_voltage(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u8 data = 0, reg = PSU_VOUT_STATUS_REG, val = 0;

    data = pega_fn8656_bnf_psu_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, PSU_VOUT_OVER_VOLTAGE_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_iout_over_current_fault(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u8 data = 0, reg = PSU_IOUT_STATUS_REG, val = 0;

    data = pega_fn8656_bnf_psu_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, PSU_IOUT_OVER_CURRENT_FAULT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_iout_over_current_warning(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u8 data = 0, reg = PSU_IOUT_STATUS_REG, val = 0;

    data = pega_fn8656_bnf_psu_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, PSU_IOUT_OVER_CURRENT_WARNING_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_iput_over_current_warning(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u8 data = 0, reg = PSU_INPUT_STATUS_REG, val = 0;

    data = pega_fn8656_bnf_psu_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, PSU_IPUT_OVER_CURRENT_WARNING_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_iput_insufficient(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u8 data = 0, reg = PSU_INPUT_STATUS_REG, val = 0;

    data = pega_fn8656_bnf_psu_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, PSU_IPUT_INSUFFICIENT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_temp_over_temp_fault(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u8 data = 0, reg = PSU_TEMP_STATUS_REG, val = 0;

    data = pega_fn8656_bnf_psu_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, PSU_TEMP_OVER_TEMP_FAULT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_temp_over_temp_warning(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u8 data = 0, reg = PSU_TEMP_STATUS_REG, val = 0;

    data = pega_fn8656_bnf_psu_read(client->addr, reg);
    pega_print(DEBUG, "addr: 0x%x, reg: %x, data: %x\r\n", client->addr, reg, data);
    GET_BIT(data, PSU_TEMP_OVER_TEMP_WARNING_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static ssize_t show_linear(struct device *dev, struct device_attribute *da,
                           char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data =  dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;    
    u16 read_value = 0;
    int exponent, mantissa;
    int multiplier = 1000;
    int ret_val;

    read_value = pega_fn8656_bnf_psu_read_word(client->addr, psu_pmbus_reg_map[attr->index]);
    exponent = two_complement_to_int(read_value >> 11, 5, 0x1f);
    mantissa = two_complement_to_int(read_value & 0x7ff, 11, 0x7ff);

    pega_print(DEBUG,"addr:0x%x, reg:0x%x, read_value:0x%x,exponent:%d,mantissa:%d\n",client->addr,psu_pmbus_reg_map[attr->index],read_value,exponent,mantissa);
    if(exponent >= 0)
        ret_val = (mantissa << exponent) * multiplier;
    else
        ret_val = (mantissa * multiplier) / (1 << -exponent);
    
    if(PSU_FAN1_SPEED == attr->index)
        return sprintf(buf, "%d\n", ret_val/1000);
    else if(PSU_POUT_MAX == attr->index)
        ret_val = ret_val * 105 / 100; //the power_out_max can have a 5%tolenrence
    return sprintf(buf, "%d.%d\n", ret_val/1000,ret_val%1000);
}

static ssize_t read_psu_vol_out(struct device *dev, struct device_attribute *da,
                                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    int data = 0, ret_val = 0;
    int exponent, mantissa;
    int multiplier = 1000;
    u16 vout_mode;

    vout_mode = pega_fn8656_bnf_psu_read(client->addr, PSU_VOUT_MODE_REG);
    data = pega_fn8656_bnf_psu_read_word(client->addr, psu_pmbus_reg_map[attr->index]);

    exponent = two_complement_to_int(vout_mode, 5, 0x1f);
    mantissa = data;

    pega_print(DEBUG, "addr:0x%x, reg:0x%x, read_value:0x%x,exponent:%d,mantissa:%d\n", client->addr, psu_pmbus_reg_map[attr->index], data, exponent, mantissa);
    if (exponent >= 0)
        ret_val = (mantissa << exponent) * multiplier;
    else
        ret_val = (mantissa * multiplier) / (1 << -exponent);

    /*the vol_out_max can have a 3% tolenrence.*/
    if(attr->index == PSU_VOUT_MAX)
        ret_val = ret_val * 103 / 100;
    else if(attr->index == PSU_VOUT_MIN)
        ret_val = ret_val * 97 / 100;

    return sprintf(buf, "%d.%d\n", ret_val / 1000, ret_val % 1000);
}

static ssize_t pega_fn8656_bnf_psu_read_data_direct(struct device *dev, struct device_attribute *da,
                                                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u16 read_value = 0;
    int ret_val;
    /* X = 1/m * (Y * 10^-R - b) */
    int m = 1, b = 0, r = -3;

    pega_print(DEBUG, "m:%d,b:%d,r:%d\n", m, b, r);
    read_value = pega_fn8656_bnf_psu_read_word(client->addr, psu_pmbus_reg_map[attr->index]);
    ret_val = 1 / m * (read_value * (10 << (-r)) - b);

    return sprintf(buf, "%d.%d\n", ret_val / 1000, ret_val % 1000);
}

static ssize_t show_manufacture_info(struct device *dev, struct device_attribute *da,
                                     char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u8 data[PM_BUS_BLOCK_MAX_LEN] = {0}, read_count;

    read_count = pega_fn8656_bnf_psu_read_block(client->addr, psu_pmbus_reg_map[attr->index], data);
    data[read_count] = '\0';
    return sprintf(buf, "%s\n", data);
}

static ssize_t get_label(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    ssize_t size = 0;

    switch (attr->index) {
    case 0:
        size = sprintf(buf, "PSU Ambient\n");
        break;
    case 1:
        size = sprintf(buf, "PSU SR Hotspot\n");
        break;
    case 2:
        size = sprintf(buf, "PSU PFC Hotspot\n");
        break;
    default:
        pega_print(WARNING, "only 3 PSU temp sensors\n");
        break;
    }
    return size;
}
static ssize_t get_psu_sensor_type(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    return sprintf(buf, "N/A\n");
}

static ssize_t get_psu_sensor_temp_min(struct device *dev,
				struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);

    return sprintf(buf, "%d\n", drv_data->temp_min[attr->index]);
}

static ssize_t get_psu_sensor_temp_max(struct device *dev,
				struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);

    return sprintf(buf, "%d\n", drv_data->temp_max[attr->index]);
}

static ssize_t set_psu_sensor_temp_max(struct device *dev, struct device_attribute *da,
                                       const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    long val = 0;

    if (kstrtol(buf, 10, &val)) {
        return -EINVAL;
    }

    drv_data->temp_max[attr->index] = val;
    return count;
}

static ssize_t get_psu_sensor_temperature(struct device *dev,
                                          struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u16 read_value = 0;
    int exponent, mantissa;
    int multiplier = 1000;
    int ret_val;
    u8 reg = attr->index + PSU_TEMP1_REG;

    if (attr->index >= MAX_PSU_TEMP_SENSORS) {
        pega_print(WARNING, "only 3 PSU temp sensors\n");
        return -ENXIO;
    }
    read_value = pega_fn8656_bnf_psu_read_word(client->addr, reg);
    exponent = two_complement_to_int(read_value >> 11, 5, 0x1f);
    mantissa = two_complement_to_int(read_value & 0x7ff, 11, 0x7ff);

    pega_print(DEBUG, "addr:0x%x, reg:0x%x, read_value:0x%x,exponent:%d,mantissa:%d\n", client->addr, reg, read_value, exponent, mantissa);
    if (exponent >= 0)
        ret_val = (mantissa << exponent) * multiplier;
    else
        ret_val = (mantissa * multiplier) / (1 << -exponent);

    return sprintf(buf, "%d\n", ret_val);
}

static ssize_t get_psu_sensor_temp_crit(struct device *dev,
                                        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    struct i2c_client *client = drv_data->client;
    u16 read_value = 0;
    int exponent, mantissa;
    int multiplier = 1000;
    int ret_val;
    u8 reg = attr->index + PSU_MFR_TEMP1_MAX_REG;
    int temp_sensor = attr->index;

    if (attr->index >= MAX_PSU_TEMP_SENSORS) {
        pega_print(WARNING, "only 3 PSU temp sensors\n");
        return -ENXIO;
    }
    read_value = pega_fn8656_bnf_psu_read_word(client->addr, reg);
    exponent = two_complement_to_int(read_value >> 11, 5, 0x1f);
    mantissa = two_complement_to_int(read_value & 0x7ff, 11, 0x7ff);

    pega_print(DEBUG, "addr:0x%x, reg:0x%x, read_value:0x%x,exponent:%d,mantissa:%d\n", client->addr, reg, read_value, exponent, mantissa);
    if (exponent >= 0)
        ret_val = (mantissa << exponent) * multiplier;
    else
        ret_val = (mantissa * multiplier) / (1 << -exponent);

    /* PSU vendor didnot set the max temperature*/
    if(ret_val == 0)
    {
        ret_val = drv_data->temp_crit[temp_sensor];
    }

    return sprintf(buf, "%d\n", ret_val);
}

static ssize_t set_psu_sensor_temp_crit(struct device *dev, struct device_attribute *da,
                                        const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    long val = 0;
 //   int temp_sensor = attr->index - PSU_MFR_TEMP1_MAX_REG;
    int temp_sensor = attr->index;

    if (kstrtol(buf, 10, &val)) {
        return -EINVAL;
    }

    drv_data->temp_crit[temp_sensor] = val;
    return count;
}

static ssize_t get_total_psu_num(struct device *dev, struct device_attribute *da,
                                 char *buf)
{
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    if (!drv_data) {
        pega_print(ERR, "fn8656_bnf_psu_data is NULL!\n");
        return -ENOENT;
    }
    return sprintf(buf, "%d\n", drv_data->total_psu_num);
}
static ssize_t get_temp_sensor_num(struct device *dev, struct device_attribute *da,
                                   char *buf)
{
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    if (!drv_data) {
        pega_print(ERR, "fn8656_bnf_psu_data is NULL!\n");
        return -ENOENT;
    }
    return sprintf(buf, "%d\n", drv_data->temp_sensor_num);
}
static ssize_t get_psu_type(struct device *dev, struct device_attribute *da,
                            char *buf)
{
    struct fn8656_bnf_psu_data *drv_data = dev_get_drvdata(dev);
    if (!drv_data) {
        pega_print(ERR, "fn8656_bnf_psu_data is NULL!\n");
        return -ENOENT;
    }
    return sprintf(buf, "%d\n", drv_data->type);
}

static SENSOR_DEVICE_ATTR(psu_led_status,  S_IRUGO, get_psu_led_status, NULL, 0);
static SENSOR_DEVICE_ATTR(psu_alarm,  S_IRUGO, read_psu_alarm, NULL, 0);
static SENSOR_DEVICE_ATTR(vout_over_voltage,  S_IRUGO, read_psu_vout_over_voltage, NULL, 0);
static SENSOR_DEVICE_ATTR(iout_over_current_fault,  S_IRUGO, read_psu_iout_over_current_fault, NULL, 0);
static SENSOR_DEVICE_ATTR(iout_over_current_warning,  S_IRUGO, read_psu_iout_over_current_warning, NULL, 0);
static SENSOR_DEVICE_ATTR(iput_over_current_warning,  S_IRUGO, read_psu_iput_over_current_warning, NULL, 0);
static SENSOR_DEVICE_ATTR(iput_insufficient,  S_IRUGO, read_psu_iput_insufficient, NULL, 0);
static SENSOR_DEVICE_ATTR(temp_over_temp_fault,  S_IRUGO, read_psu_temp_over_temp_fault, NULL, 0);
static SENSOR_DEVICE_ATTR(temp_over_temp_warning,  S_IRUGO, read_psu_temp_over_temp_warning, NULL, 0);
static SENSOR_DEVICE_ATTR(vol_out,  S_IRUGO, read_psu_vol_out, NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(vol_in,  S_IRUGO, show_linear, NULL, PSU_V_IN);
static SENSOR_DEVICE_ATTR(curr_out,  S_IRUGO, show_linear, NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(curr_in,  S_IRUGO, show_linear, NULL, PSU_I_IN);
static SENSOR_DEVICE_ATTR(power_out,  S_IRUGO, show_linear, NULL, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(power_in,  S_IRUGO, show_linear, NULL, PSU_P_IN);
static SENSOR_DEVICE_ATTR(fan1_speed,  S_IRUGO, show_linear, NULL, PSU_FAN1_SPEED);

static SENSOR_DEVICE_ATTR(manuafacture_id,S_IRUGO, show_manufacture_info, NULL, PSU_MFR_ID);
static SENSOR_DEVICE_ATTR(manuafacture_model,S_IRUGO, show_manufacture_info, NULL, PSU_MFR_MODEL);
static SENSOR_DEVICE_ATTR(manuafacture_revision,S_IRUGO, show_manufacture_info, NULL, PSU_MFR_REVISION);
static SENSOR_DEVICE_ATTR(manuafacture_serial,S_IRUGO, show_manufacture_info, NULL, PSU_MFR_SERIAL);
static SENSOR_DEVICE_ATTR(manuafacture_date,S_IRUGO, show_manufacture_info, NULL, PSU_MFR_DATE);
static SENSOR_DEVICE_ATTR(manuafacture_location,S_IRUGO, show_manufacture_info, NULL, PSU_MFR_LOCATION);
static SENSOR_DEVICE_ATTR(manuafacture_part_num,S_IRUGO, show_manufacture_info, NULL, PSU_MFR_MODEL);

static SENSOR_DEVICE_ATTR(vol_in_min,S_IRUGO, show_linear, NULL, PSU_VIN_MIN);
static SENSOR_DEVICE_ATTR(vol_in_max,S_IRUGO, show_linear, NULL, PSU_VIN_MAX);
static SENSOR_DEVICE_ATTR(curr_in_max,S_IRUGO, show_linear, NULL, PSU_IIN_MAX);
static SENSOR_DEVICE_ATTR(power_in_max,S_IRUGO, pega_fn8656_bnf_psu_read_data_direct, NULL, PSU_PIN_MAX);
static SENSOR_DEVICE_ATTR(vol_out_min,S_IRUGO, read_psu_vol_out, NULL, PSU_VOUT_MIN);
static SENSOR_DEVICE_ATTR(vol_out_max,S_IRUGO, read_psu_vol_out, NULL, PSU_VOUT_MAX);
static SENSOR_DEVICE_ATTR(curr_out_max,S_IRUGO, show_linear, NULL, PSU_IOUT_MAX);
static SENSOR_DEVICE_ATTR(power_out_max,S_IRUGO, pega_fn8656_bnf_psu_read_data_direct, NULL, PSU_POUT_MAX);

static SENSOR_DEVICE_ATTR(psu_num,  S_IRUGO, get_total_psu_num, NULL, 0);
static SENSOR_DEVICE_ATTR(sensor_num,  S_IRUGO, get_temp_sensor_num, NULL, 0);
static SENSOR_DEVICE_ATTR(psu_type,  S_IRUGO, get_psu_type, NULL, 0);
static SENSOR_DEVICE_ATTR(psu_sensor_type,  S_IRUGO, get_psu_sensor_type, NULL, 0);

#define SET_TEMP_ATTR(_num) \
    static SENSOR_DEVICE_ATTR(temp##_num##_input,  S_IRUGO, get_psu_sensor_temperature, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(temp##_num##_label,  S_IRUGO, get_label, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(temp##_num##_max,  S_IRUGO, get_psu_sensor_temp_max, set_psu_sensor_temp_max, _num-1); \
    static SENSOR_DEVICE_ATTR(temp##_num##_min,  S_IRUGO, get_psu_sensor_temp_min, NULL, _num-1); \
    static SENSOR_DEVICE_ATTR(temp##_num##_crit,  S_IRUGO, get_psu_sensor_temp_crit, set_psu_sensor_temp_crit, _num-1);

SET_TEMP_ATTR(1);SET_TEMP_ATTR(2);SET_TEMP_ATTR(3);

static struct attribute *pega_fn8656_bnf_psu_attrs[] = {
    &sensor_dev_attr_psu_led_status.dev_attr.attr,
    &sensor_dev_attr_psu_alarm.dev_attr.attr,
    &sensor_dev_attr_vout_over_voltage.dev_attr.attr,
    &sensor_dev_attr_iout_over_current_fault.dev_attr.attr,
    &sensor_dev_attr_iout_over_current_warning.dev_attr.attr,
    &sensor_dev_attr_iput_over_current_warning.dev_attr.attr,
    &sensor_dev_attr_iput_insufficient.dev_attr.attr,
    &sensor_dev_attr_temp_over_temp_fault.dev_attr.attr,
    &sensor_dev_attr_temp_over_temp_warning.dev_attr.attr,

    &sensor_dev_attr_vol_out.dev_attr.attr,
    &sensor_dev_attr_vol_in.dev_attr.attr,
    &sensor_dev_attr_curr_out.dev_attr.attr,
    &sensor_dev_attr_curr_in.dev_attr.attr,
    &sensor_dev_attr_power_out.dev_attr.attr,
    &sensor_dev_attr_power_in.dev_attr.attr,
    &sensor_dev_attr_fan1_speed.dev_attr.attr,

    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp3_input.dev_attr.attr,
    &sensor_dev_attr_temp1_label.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp3_label.dev_attr.attr,
    &sensor_dev_attr_temp1_max.dev_attr.attr,
    &sensor_dev_attr_temp2_max.dev_attr.attr,
    &sensor_dev_attr_temp3_max.dev_attr.attr,
    &sensor_dev_attr_temp1_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp3_crit.dev_attr.attr,

    &sensor_dev_attr_manuafacture_id.dev_attr.attr,
    &sensor_dev_attr_manuafacture_model.dev_attr.attr,
    &sensor_dev_attr_manuafacture_revision.dev_attr.attr,
    &sensor_dev_attr_manuafacture_serial.dev_attr.attr,
    &sensor_dev_attr_manuafacture_date.dev_attr.attr,
    &sensor_dev_attr_manuafacture_location.dev_attr.attr,
    &sensor_dev_attr_manuafacture_part_num.dev_attr.attr,
    &sensor_dev_attr_vol_in_min.dev_attr.attr,
    &sensor_dev_attr_vol_in_max.dev_attr.attr,
    &sensor_dev_attr_curr_in_max.dev_attr.attr,
    &sensor_dev_attr_power_in_max.dev_attr.attr,
    &sensor_dev_attr_vol_out_min.dev_attr.attr,
    &sensor_dev_attr_vol_out_max.dev_attr.attr,
    &sensor_dev_attr_curr_out_max.dev_attr.attr,
    &sensor_dev_attr_power_out_max.dev_attr.attr,

    &sensor_dev_attr_psu_num.dev_attr.attr,
    &sensor_dev_attr_sensor_num.dev_attr.attr,
    &sensor_dev_attr_psu_type.dev_attr.attr,
    &sensor_dev_attr_psu_sensor_type.dev_attr.attr,
    NULL
};

//static const struct attribute_group pega_fn8656_bnf_psu_group = { .attrs = pega_fn8656_bnf_psu_attributes};
ATTRIBUTE_GROUPS(pega_fn8656_bnf_psu);
static void pega_fn8656_bnf_psu_add_client(struct i2c_client *client)
{
    struct psu_client_node *node = kzalloc(sizeof(struct psu_client_node), GFP_KERNEL);
    
    if (!node) {
        pega_print(ERR, "Can't allocate psu_client_node (0x%x)\n", client->addr);
        return;
    }
    
    node->client = client;
    
    mutex_lock(&list_lock);
    list_add(&node->list, &psu_client_list);
    mutex_unlock(&list_lock);
}

static void pega_fn8656_bnf_psu_remove_client(struct i2c_client *client)
{
    struct list_head        *list_node = NULL;
    struct psu_client_node *psu_node = NULL;
    int found = 0;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &psu_client_list)
    {
        psu_node = list_entry(list_node, struct psu_client_node, list);
        
        if (psu_node->client == client) {
            found = 1;
            break;
        }
    }
    
    if (found) {
        list_del(list_node);
        kfree(psu_node);
    }
    
    mutex_unlock(&list_lock);
}

static int pega_fn8656_bnf_psu_probe(struct i2c_client *client,
                                     const struct i2c_device_id *dev_id)
{
    int status = 0 , i = 0;
    struct device *dev = &client->dev;
    struct fn8656_bnf_psu_data *data;
    struct device *hwmon_dev;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        pega_print(ERR, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    data = devm_kzalloc(dev, sizeof(struct fn8656_bnf_psu_data), GFP_KERNEL);
    if (data == NULL)
        return -ENOMEM;
    data->client = client;
    data->total_psu_num = MAX_PSU_NUMS;
    data->temp_sensor_num = MAX_PSU_TEMP_SENSORS;
    data->type = POWER_AC;
    for (i = 0; i < MAX_PSU_TEMP_SENSORS; i++) {
        data->temp_max[i] = psu_temp_max[i] * 1000;
        data->temp_min[i] = psu_temp_min[i]  * 1000;
        data->temp_crit[i] = psu_temp_crit[i] *1000;
    }

    /* Register sysfs hooks */
    switch (client->addr) {
    case PSU_58_ADDRESS:
    case PSU_59_ADDRESS:
        pega_print(ERR,"hwmon_dev register\n");
        hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
                                                           data, pega_fn8656_bnf_psu_groups);
        if (IS_ERR(hwmon_dev))
            return PTR_ERR(hwmon_dev);

        break;
    default:
        pega_print(WARNING, "i2c_check_psu failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
        break;
    }

    if (status) {
        goto exit;
    }

    pega_print(INFO, "chip found\n");
    pega_fn8656_bnf_psu_add_client(client);

    return 0;

exit:
    return status;
}

static int pega_fn8656_bnf_psu_remove(struct i2c_client *client)
{
    pega_fn8656_bnf_psu_remove_client(client);  
    return 0;
}

static const struct i2c_device_id pega_fn8656_bnf_psu_id[] = {
    { "fn8656_bnf_psu", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, pega_fn8656_bnf_psu_id);

static struct i2c_driver pega_fn8656_bnf_psu_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "pegatron_fn8656_bnf_psu",
    },
    .probe      = pega_fn8656_bnf_psu_probe,
    .remove     = pega_fn8656_bnf_psu_remove,
    .id_table   = pega_fn8656_bnf_psu_id,
    .address_list = normal_i2c,
};

static int __init pega_fn8656_bnf_psu_init(void)
{
    mutex_init(&list_lock);

    return i2c_add_driver(&pega_fn8656_bnf_psu_driver);
}

static void __exit pega_fn8656_bnf_psu_exit(void)
{
    i2c_del_driver(&pega_fn8656_bnf_psu_driver);
}

module_param(loglevel, uint, 0644);
module_param_string(debug, debug, MAX_DEBUG_INFO_LEN, 0644);
MODULE_PARM_DESC(loglevel, "0x01-LOG_ERR,0x02-LOG_WARNING,0x04-LOG_INFO,0x08-LOG_DEBUG");
MODULE_PARM_DESC(debug, "help info");

MODULE_AUTHOR("Peter5 Lin <Peter5_Lin@pegatroncorp.com.tw>");
MODULE_DESCRIPTION("pega_fn8656_bnf_psu driver");
MODULE_LICENSE("GPL");

module_init(pega_fn8656_bnf_psu_init);
module_exit(pega_fn8656_bnf_psu_exit);
