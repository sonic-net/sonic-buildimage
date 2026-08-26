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
#include "../../../../../pddf/i2c/modules/include/pddf_psu_defs.h"
#include "pddf_psu_driver.h"


#define PSU_REG_VOUT_MODE 0x20
#define PSU_REG_READ_VOUT 0x8b

/*#define PSU_DEBUG*/
#ifdef PSU_DEBUG
#define psu_dbg(...) printk(__VA_ARGS__)
#else
#define psu_dbg(...)
#endif


void get_psu_duplicate_sysfs(int idx, char *str)
{
    switch (idx)
    {
        case PSU_V_OUT:
            strscpy(str, "in3_input", ATTR_NAME_LEN);
            break;
        case PSU_I_OUT:
            strscpy(str, "curr2_input", ATTR_NAME_LEN);
            break;
        case PSU_P_OUT:
            strscpy(str, "power2_input", ATTR_NAME_LEN);
            break;
        case PSU_FAN1_SPEED:
            strscpy(str, "fan1_input", ATTR_NAME_LEN);
            break;
        case PSU_TEMP1_INPUT:
            strscpy(str, "temp1_input", ATTR_NAME_LEN);
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
    {
        /*printk(KERN_ERR "%s: vout_mode reg value 0x%x\n", __func__, status);*/
        return status;
    }
}

static u16 psu_get_v_out(struct i2c_client *client)
{
    u16 status = 0, retry = 10;
    uint8_t offset = PSU_REG_READ_VOUT;

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
        printk(KERN_ERR "%s: Get PSU Vout failed\n", __func__);
        return 0;
    }
    else
    {
        /*printk(KERN_ERR "%s: vout reg value 0x%x\n", __func__, status);*/
        return status;
    }
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
    int i, status=0;
    u16 value = 0;
    u8 vout_mode = 0;
    int exponent, mantissa;
    int multiplier = 1000;
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
        goto exit;
    }

    psu_update_attr(dev, sysfs_attr_info, usr_data);

    switch(attr->index)
    {
        case PSU_PRESENT:
        case PSU_POWER_GOOD:
            status = sysfs_attr_info->val.intval;
            return sprintf(buf, "%d\n", status);
            break;
        case PSU_MODEL_NAME:
        case PSU_MFR_ID:
        case PSU_SERIAL_NUM:
        case PSU_FAN_DIR:
            return sprintf(buf, "%s\n", sysfs_attr_info->val.strval);
            break;
        case PSU_V_OUT:
            value = psu_get_v_out(client);
            vout_mode = psu_get_vout_mode(client);
            if ((vout_mode >> 5) == 0)
                exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
            else
                exponent = 0;
    
            mantissa = value;
            if (exponent >= 0)
                return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
			
            else
                return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
            break;

        case PSU_V_OUT_MIN:
        case PSU_V_OUT_MAX:
            multiplier = 1000;
            value = sysfs_attr_info->val.shortval;
            vout_mode = psu_get_vout_mode(client);
            if ((vout_mode >> 5) == 0)
                exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
            else
                exponent = 0;
            mantissa = two_complement_to_int(value & 0xffff, 16, 0xffff);
			
            if (exponent >= 0)
                return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
            else
                return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
            break;       
        case PSU_I_OUT:
        case PSU_V_IN:
        case PSU_I_IN:
        case PSU_P_OUT_MAX:
            multiplier = 1000;
            value = sysfs_attr_info->val.shortval;
            exponent = two_complement_to_int(value >> 11, 5, 0x1f);
            mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);
            if (exponent >= 0)
                return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
            else
                return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
            break;
        case PSU_P_IN:
        case PSU_P_OUT:
            multiplier = 1000000;
            value = sysfs_attr_info->val.shortval;
            exponent = two_complement_to_int(value >> 11, 5, 0x1f);
            mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);
            if (exponent >= 0)
                return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
            else
                return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));

            break;
        case PSU_FAN1_SPEED:
            value = sysfs_attr_info->val.shortval;
            exponent = two_complement_to_int(value >> 11, 5, 0x1f);
            mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);
            if (exponent >= 0)
                return sprintf(buf, "%d\n", (mantissa << exponent));
            else
                return sprintf(buf, "%d\n", (mantissa) / (1 << -exponent));

            break;
        case PSU_TEMP1_INPUT:
        case PSU_TEMP1_HIGH_THRESHOLD:
            multiplier = 1000;
            value = sysfs_attr_info->val.shortval;
            exponent = two_complement_to_int(value >> 11, 5, 0x1f);
            mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);
            if (exponent >= 0)
                return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
            else
                return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));

            break;
        default:
            printk(KERN_ERR "%s: Unable to find attribute index for %s\n", __FUNCTION__, usr_data->aname);
            goto exit;
    }

exit:
    return sprintf(buf, "%d\n", status);
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


    if (strncmp(adata->devtype, "cpld", strlen("cpld")) == 0)
    {
        val = board_i2c_cpld_read_new(adata->devaddr, adata->devname, adata->offset);
        if (val < 0){
            return val;
		}
        padata->val.intval =  ((val & adata->mask) == adata->cmpval);
        psu_dbg(KERN_ERR "%s: byte_value = 0x%x\n", __FUNCTION__, padata->val.intval);
    }

    return status;
}

int sonic_i2c_get_psu_block_default(void *client, PSU_DATA_ATTR *adata, void *data)
{
    int status = 0, retry = 10;
    struct psu_attr_info *padata = (struct psu_attr_info *)data;
    char buf[32]="";  //temporary placeholder for block data
    uint8_t offset = (uint8_t)adata->offset;
    int data_len = adata->len;

    while (retry)
    {
        status = i2c_smbus_read_i2c_block_data((struct i2c_client *)client, offset, data_len-1, buf);
        if (unlikely(status<0))
        {
            msleep(60);
            retry--;
            continue;
        }
        break;
    }

    if (status < 0)
    {
        buf[0] = '\0';
        dev_dbg(&((struct i2c_client *)client)->dev, "unable to read block of data from (0x%x)\n", ((struct i2c_client *)client)->addr);
    }
    else
    {
        buf[data_len-1] = '\0';
    }

    if (strncmp(adata->devtype, "pmbus", strlen("pmbus")) == 0)
        strscpy(padata->val.strval, buf+1, data_len-1);
    else
        strscpy(padata->val.strval, buf, data_len);

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
