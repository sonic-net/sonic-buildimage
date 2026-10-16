/*
 * Copyright 2019 Broadcom.
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
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
 *
 *  Description of various APIs related to PSU component
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/kobject.h>
#include "pddf_psu_defs.h"
#include "pddf_psu_driver.h"

#define PSU_REG_VOUT_MODE 0x20
#define PSU_REG_READ_VOUT 0x8b
#define PSU_I2C_TMP_BUFSZ  32

/*#define PSU_DEBUG*/
#ifdef PSU_DEBUG
#define psu_dbg(...) printk(__VA_ARGS__)
#else
#define psu_dbg(...)
#endif

static unsigned char psu_ioport_cpld_read(size_t ioport_bar, size_t offset)
{
    unsigned char val = 0;
    val = inb(ioport_bar + offset);
    psu_dbg(KERN_INFO "%s: Reading from LPC CPLD at address 0x%x\n", __func__, ioport_bar + offset);
    return val;
}

void get_psu_duplicate_sysfs(int idx, char *str)
{
    switch (idx)
    {
        case PSU_V_OUT:
            strscpy(str, "in3_input", I2C_NAME_SIZE);
            break;
        case PSU_I_OUT:
            strscpy(str, "curr2_input", I2C_NAME_SIZE);
            break;
        case PSU_P_OUT:
            strscpy(str, "power2_input", I2C_NAME_SIZE);
            break;
        case PSU_FAN1_SPEED:
            strscpy(str, "fan1_input", I2C_NAME_SIZE);
            break;
        case PSU_TEMP1_INPUT:
            strscpy(str, "temp1_input", I2C_NAME_SIZE);
            break;
        case PSU_TEMP2_INPUT:
            strscpy(str, "temp2_input", I2C_NAME_SIZE);
            break;
        default:
            break;
    }

    return;
}

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static int psu_linear11_to_val(u16 value, long long *result)
{
    int exponent, mantissa;

    if (!result)
        return -EINVAL;

    exponent = two_complement_to_int(value >> 11, 5, 0x1f);
    mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);

    mantissa = mantissa * 1000LL;
    if (exponent >= 0)
        *result = (long long)mantissa << exponent;
    else
        *result = (long long)mantissa >> -exponent;
    
    return 0;
}

/**
 * psu_vout_mode_to_val - Convert VOUT register value to actual voltage
 * @value: value read from VOUT register
 * @vout_mode: VOUT mode read from VOUT_MODE register
 * @result: pointer to store the converted value
 * Return: 0 on success, or negative error code
 */
static int psu_vout_mode_to_val(u16 value, u8 vout_mode, long long *result)
{
    int exponent;
    long long mantissa = value;

    if (!result)
        return -EINVAL;

    if ((vout_mode >> 5) != 0) { /* Not linear mode */
        printk(KERN_ERR "%s: Unsupported VOUT mode 0x%x\n", __func__, vout_mode);
        return -ENOTSUPP;
    }

    exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);

    mantissa = mantissa * 1000LL; 
    if (exponent >= 0)
        *result = mantissa << exponent;
    else
        *result = mantissa >> -exponent;

    return 0;
}

static u8 psu_get_vout_mode(struct i2c_client *client)
{
    u8 status = 0, retry = 10;
    uint8_t offset = PSU_REG_VOUT_MODE;

    while (retry)
    {
        status = i2c_smbus_read_byte_data((struct i2c_client *)client, offset);
        if (unlikely(status < 0)) 
        {
            msleep(60);
            retry--;
            continue;
        }
        break;
    }

    if (status < 0)
    {
        printk(KERN_ERR "%s: Get PSU Vout mode failed\n", __func__);
        return 0;
    }
    else
        return status;

}

int psu_update_hw(struct device *dev, struct psu_attr_info *info, PSU_DATA_ATTR *udata)
{
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);
    PSU_SYSFS_ATTR_DATA *sysfs_attr_data = NULL;


    mutex_lock(&info->update_lock);

    sysfs_attr_data = udata->access_data;
    if (sysfs_attr_data->pre_set != NULL)
    {
        status = (sysfs_attr_data->pre_set)(client, udata, info);
        if (status!=0)
            dev_warn(&client->dev, "%s: pre_set function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
    }
    if (sysfs_attr_data->do_set != NULL)
    {
        status = (sysfs_attr_data->do_set)(client, udata, info);
        if (status!=0)
            dev_warn(&client->dev, "%s: do_set function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);

    }
    if (sysfs_attr_data->post_set != NULL)
    {
        status = (sysfs_attr_data->post_set)(client, udata, info);
        if (status!=0)
            dev_warn(&client->dev, "%s: post_set function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
    }

    mutex_unlock(&info->update_lock);

    return 0;
}


int psu_update_attr(struct device *dev, struct psu_attr_info *data, PSU_DATA_ATTR *udata)
{
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);
    PSU_SYSFS_ATTR_DATA *sysfs_attr_data=NULL;

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) || !data->valid)
    {
        dev_dbg(&client->dev, "Starting update for %s\n", data->name);

        sysfs_attr_data = udata->access_data;
        if (sysfs_attr_data->pre_get != NULL)
        {
            status = (sysfs_attr_data->pre_get)(client, udata, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: pre_get function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
        }
        if (sysfs_attr_data->do_get != NULL)
        {
            status = (sysfs_attr_data->do_get)(client, udata, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: do_get function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);

        }
        if (sysfs_attr_data->post_get != NULL)
        {
            status = (sysfs_attr_data->post_get)(client, udata, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: post_get function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);
    return 0;
}

ssize_t psu_show_default(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct psu_data *data = i2c_get_clientdata(client);
    PSU_PDATA *pdata = (PSU_PDATA *)(client->dev.platform_data);
    PSU_DATA_ATTR *usr_data = NULL;
    struct psu_attr_info *sysfs_attr_info = NULL;
    int i;
    char new_str[ATTR_NAME_LEN] = "";
    PSU_SYSFS_ATTR_DATA *ptr = NULL;

    for (i=0;i<data->num_attr;i++)
    {
        ptr = (PSU_SYSFS_ATTR_DATA *)pdata->psu_attrs[i].access_data;
        get_psu_duplicate_sysfs(ptr->index , new_str);
        if ( strcmp(attr->dev_attr.attr.name, pdata->psu_attrs[i].aname) == 0 || strcmp(attr->dev_attr.attr.name, new_str) == 0 )
        {
            sysfs_attr_info = &data->attr_info[i];
            usr_data = &pdata->psu_attrs[i];
            strscpy(new_str, "", ATTR_NAME_LEN);
        }
    }

    if (sysfs_attr_info==NULL || usr_data==NULL)
    {
        printk(KERN_ERR "%s is not supported attribute for this client\n", attr->dev_attr.attr.name);
        return -EINVAL;
    }

    psu_update_attr(dev, sysfs_attr_info, usr_data);

    switch(attr->index)
    {
        case PSU_PRESENT:
        case PSU_POWER_GOOD:
            return sprintf(buf, "%d\n", sysfs_attr_info->val.intval);
        case PSU_MODEL_NAME:
        case PSU_MFR_ID:
        case PSU_SERIAL_NUM:
        case PSU_FAN_DIR:
            /*This is */
            return sprintf(buf, "%s\n", sysfs_attr_info->val.strval);
        case PSU_V_OUT:
        case PSU_V_OUT_MIN:
        case PSU_V_OUT_MAX:
        {
            long long val;
            int err;
            u16 value = sysfs_attr_info->val.shortval;
            u8 vout_mode = psu_get_vout_mode(client);
            err = psu_vout_mode_to_val(value, vout_mode, &val);
            if (err)
                return err;
            return sprintf(buf, "%lld\n", val);
        }
        case PSU_I_OUT:
        case PSU_V_IN:
        case PSU_I_IN:
        case PSU_P_OUT_MAX:
        {
            long long val;
            int err;
            u16 value = sysfs_attr_info->val.shortval;
            err = psu_linear11_to_val(value, &val);
            if (err)
                return err;
            return sprintf(buf, "%lld\n", val);
        }
        case PSU_P_IN:
        case PSU_P_OUT:
        {
            long long val;
            int err;
            u16 value = sysfs_attr_info->val.shortval;
            err = psu_linear11_to_val(value, &val);
            if (err)
                return err;
            return sprintf(buf, "%lld\n", val * 1000);
        }
        case PSU_FAN1_SPEED:
        {
            long long val;
            int err;
            u16 value = sysfs_attr_info->val.shortval;
            err = psu_linear11_to_val(value, &val);
            if (err)
                return err;
            return sprintf(buf, "%lld\n", div_s64(val, 1000));
        }
        case PSU_TEMP1_INPUT:
        case PSU_TEMP2_INPUT:
        case PSU_TEMP1_HIGH_THRESHOLD:
        case PSU_TEMP2_HIGH_THRESHOLD:
        {
            long long val;
            int err;
            u16 value = sysfs_attr_info->val.shortval;
            err = psu_linear11_to_val(value, &val);
            if (err)
                return err;
            return sprintf(buf, "%lld\n", val);
        }
        default:
            printk(KERN_ERR "%s: Unable to find attribute index for %s\n", __FUNCTION__, usr_data->aname);
            return -EINVAL;
    }

    return -EINVAL; /* Should not be reached */
}


ssize_t psu_store_default(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct psu_data *data = i2c_get_clientdata(client);
    PSU_PDATA *pdata = (PSU_PDATA *)(client->dev.platform_data);
    PSU_DATA_ATTR *usr_data = NULL;
    struct psu_attr_info *sysfs_attr_info = NULL;
    int i;

    for (i=0;i<data->num_attr;i++)
    {
        if (strcmp(data->attr_info[i].name, attr->dev_attr.attr.name) == 0 && strcmp(pdata->psu_attrs[i].aname, attr->dev_attr.attr.name) == 0)
        {
            sysfs_attr_info = &data->attr_info[i];
            usr_data = &pdata->psu_attrs[i];
        }
    }

    if (sysfs_attr_info==NULL || usr_data==NULL) {
        printk(KERN_ERR "%s is not supported attribute for this client\n", attr->dev_attr.attr.name);
        goto exit;
    }

    switch(attr->index)
    {
        /*No write attributes for now in PSU*/
        default:
            goto exit;
    }

    psu_update_hw(dev, sysfs_attr_info, usr_data);

exit:
    return count;
}

extern int board_i2c_cpld_read_new(unsigned short cpld_addr, char *name, u8 reg);
int sonic_i2c_get_psu_byte_default(void *client, PSU_DATA_ATTR *adata, void *data)
{
    int status = 0;
    int val = 0;
    struct psu_attr_info *padata = (struct psu_attr_info *)data;


    if (strncmp(adata->devtype, "cpld", strlen("cpld")) == 0) {
        val = board_i2c_cpld_read_new(adata->devaddr, adata->devname, adata->offset);
        if (val < 0){
            return val;
		}
        padata->val.intval =  ((val & adata->mask) == adata->cmpval);
        psu_dbg(KERN_ERR "%s: byte_value = 0x%x\n", __FUNCTION__, padata->val.intval);
    } else if (strncmp(adata->devtype, "ioport", strlen("ioport")) == 0) {
        val = psu_ioport_cpld_read(adata->devaddr, adata->offset);

        padata->val.intval =  ((val & adata->mask) == adata->cmpval);
        psu_dbg(KERN_ERR "%s: byte_value = 0x%x\n", __FUNCTION__, padata->val.intval);
    }

    return status;
}

int sonic_i2c_get_psu_block_default(void *client, PSU_DATA_ATTR *adata, void *data)
{
    int status = 0, retry = 10;
    struct psu_attr_info *padata = (struct psu_attr_info *)data;
    char buf[PSU_I2C_TMP_BUFSZ] = {0};  //temporary placeholder for block data
    uint8_t offset = (uint8_t)adata->offset;
    int data_len = adata->len;

    if (data_len > PSU_I2C_TMP_BUFSZ)
        return 0;

    while (retry)
    {
        status = i2c_smbus_read_i2c_block_data((struct i2c_client *)client, offset, data_len, buf);
        if (unlikely(status<0))
        {
            msleep(60);
            retry--;
            continue;
        }
        break;
    }

    if (status < 0) {
        buf[0] = '\0';
        dev_dbg(&((struct i2c_client *)client)->dev, "unable to read block of data from (0x%x)\n", ((struct i2c_client *)client)->addr);
    } else {
        /* Handle PMBus block data read as raw data bytes */
        if (strncmp(adata->devtype, "pmbus", strlen("pmbus")) == 0) {
            size_t len_from_device = buf[0];
            if (len_from_device < data_len && len_from_device < sizeof(padata->val.strval)) {
                memcpy(padata->val.strval, buf + 1, len_from_device);
                padata->val.strval[len_from_device] = '\0';
            } else {
                strscpy(padata->val.strval, buf + 1, sizeof(padata->val.strval));
            }

        } else {
            strscpy(padata->val.strval, buf, sizeof(padata->val.strval));
        }
    }

    psu_dbg(KERN_ERR "%s: status = %d, buf block: %s\n", __FUNCTION__, status, padata->val.strval);
    return 0;
}

int sonic_i2c_get_psu_word_default(void *client, PSU_DATA_ATTR *adata, void *data)
{

    int status = 0, retry = 10;
    struct psu_attr_info *padata = (struct psu_attr_info *)data;
    uint8_t offset = (uint8_t)adata->offset;

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
        padata->val.shortval = 0;
        dev_dbg(&((struct i2c_client *)client)->dev, "unable to read a word from (0x%x)\n", ((struct i2c_client *)client)->addr);
    }
    else
    {
        padata->val.shortval = status;
    }

    psu_dbg(KERN_ERR "%s: word value : %d\n", __FUNCTION__, padata->val.shortval);
    return 0;
}
