#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include "pddf_psu_defs.h"
#include "pddf_client_defs.h"

static int *log_level = &psu_log_level;

#define PSU_REG_VOUT_MODE 0x20
#define PSU_REG_READ_VOUT 0x8b
#define PSU_REG_READ_VOUT_MIN 0xa4
#define PSU_REG_READ_VOUT_MAX 0xa5
#define PSU_REG_STATUS_WORD 0x79

ssize_t pddf_show_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf);
ssize_t pddf_show_custom_psu_v_out_min(struct device *dev, struct device_attribute *da, char *buf);
ssize_t pddf_show_custom_psu_v_out_max(struct device *dev, struct device_attribute *da, char *buf);
ssize_t pddf_show_custom_psu_alarm(struct device *dev, struct device_attribute *da, char *buf);

extern PSU_SYSFS_ATTR_DATA access_psu_v_out;
extern PSU_SYSFS_ATTR_DATA access_psu_v_out_min;
extern PSU_SYSFS_ATTR_DATA access_psu_v_out_max;
extern PSU_SYSFS_ATTR_DATA access_psu_alarm;

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static u8 psu_get_vout_mode(struct i2c_client *client)
{
    int status = 0, retry = 10;
    uint8_t offset = PSU_REG_VOUT_MODE;

    while (retry) {
        status = i2c_smbus_read_byte_data((struct i2c_client *)client, offset);
        if (unlikely(status < 0)) {
            msleep(60);
            retry--;
            continue;
        }
        break;
    }

    if (status < 0)
    {
        pddf_dbg(PSU, "%s: Get PSU Vout mode failed\n", __func__);
        return 0;
    }
    else
    {
        pddf_dbg(PSU, "%s: vout_mode reg value 0x%x\n", __func__, status);
        return status;
    }
}

static u16 psu_get_v_value(struct i2c_client *client, uint8_t offset)
{
    int status = 0, retry = 10;

    while (retry) {
        status = i2c_smbus_read_word_data((struct i2c_client *)client, offset);
        if (unlikely(status < 0)) {
            msleep(60);
            retry--;
            continue;
        }
        break;
    }

    if (status < 0)
    {
        pddf_dbg(PSU, "%s: Get PSU Vout failed\n", __func__);
        return 0;
    }
    else
    {
        pddf_dbg(PSU, "%s: vout reg value 0x%x\n", __func__, status);
        return status;
    }
}

ssize_t pddf_show_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 value;
    u8 vout_mode;

    value = psu_get_v_value(client, PSU_REG_READ_VOUT);
    vout_mode = psu_get_vout_mode(client);

    if ((vout_mode >> 5) == 0)
        exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
    else
    {
        pddf_err(PSU, "%s: Only support linear mode for vout mode\n", __func__);
        exponent = 0;
    }
    mantissa = value;
    if (exponent >= 0)
        return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
    else
        return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

ssize_t pddf_show_custom_psu_v_out_min(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int exponent, mantissa;
    int multiplier = 1000;
    u16 value;
    u8 vout_mode;

    value = psu_get_v_value(client, PSU_REG_READ_VOUT_MIN);
    vout_mode = psu_get_vout_mode(client);

    if ((vout_mode >> 5) == 0)
        exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
    else
    {
        pddf_err(PSU, "%s: Only support linear mode for vout mode\n", __func__);
        exponent = 0;
    }
    mantissa = value;
    if (exponent >= 0)
        return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
    else
        return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}
ssize_t pddf_show_custom_psu_v_out_max(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int exponent, mantissa;
    int multiplier = 1000;
    u16 value;
    u8 vout_mode;
    
    value = psu_get_v_value(client, PSU_REG_READ_VOUT_MAX);
    vout_mode = psu_get_vout_mode(client);

    if ((vout_mode >> 5) == 0)
        exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
    else
    {
        /*printk(KERN_ERR "%s: Only support linear mode for vout mode\n", __func__);*/
        exponent = 0;
    }
    mantissa = value;
    if (exponent >= 0)
        return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
    else
        return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

#define STATUS_BITVAL_TEMP (1 << 2)//temp
#define STATUS_BITVAL_VIN_UV (1 << 3)//voltage
#define STATUS_BITVAL_IOUT_OC (1 << 4)//current
#define STATUS_BITVAL_VOUT_OV (1 << 5)//voltage
#define STATUS_BITVAL_FAN (1 << (2+8))//fan
#define STATUS_BITVAL_INPUT (1 << (5+8))//voltage&current
#define STATUS_BITVAL_IOUT_POUT (1 << (6+8))//current
#define STATUS_BITVAL_VOUT (1 << (7+8))//voltage
#define CLX_ALARM_NORMAL    0x0
#define CLX_ALARM_TEMP      0x1
#define CLX_ALARM_FAN       0x2
#define CLX_ALARM_VOLT      0x4
ssize_t pddf_show_custom_psu_alarm(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    u16 value;
    u8 alarm = 0;
    
    value = psu_get_v_value(client, PSU_REG_STATUS_WORD);
    pddf_dbg(PSU, "PSU_REG_STATUS_WORD val:0x%x\n",value);
    if (value & STATUS_BITVAL_TEMP) {
        alarm |= CLX_ALARM_TEMP;
    }
    if ((value & STATUS_BITVAL_VIN_UV) || (value & STATUS_BITVAL_VOUT_OV) 
        || (value & STATUS_BITVAL_INPUT) || (value & STATUS_BITVAL_VOUT)) {
        alarm |= CLX_ALARM_VOLT;
    }
    if (value & STATUS_BITVAL_FAN) {
        alarm |= CLX_ALARM_FAN;
    }
    pddf_dbg(PSU, "alarm val:0x%x\n",alarm);
    return sprintf(buf, "0x%02x\n", alarm);
}

static int __init pddf_custom_psu_init(void)
{
    access_psu_v_out.show = pddf_show_custom_psu_v_out;
    access_psu_v_out_min.show = pddf_show_custom_psu_v_out_min;
    access_psu_v_out_max.show = pddf_show_custom_psu_v_out_max;
    access_psu_alarm.show = pddf_show_custom_psu_alarm;
    access_psu_v_out.do_get = NULL;
    return 0;
}

static void __exit pddf_custom_psu_exit(void)
{
    return;
}

MODULE_AUTHOR("Embedway");
MODULE_DESCRIPTION("pddf custom psu api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_psu_init);
module_exit(pddf_custom_psu_exit);
