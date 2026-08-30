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
 * A pddf kernel module driver for PSU
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
#include "../../../../pddf/i2c/modules/include/pddf_psu_defs.h"
#include "../../../../pddf/i2c/modules/include/pddf_psu_driver.h"
#include "../../../../pddf/i2c/modules/include/pddf_psu_api.h"

#define PSU_REG_VOUT_MODE 0x20
#define PSU_REG_READ_VOUT 0x8b

extern PSU_SYSFS_ATTR_DATA access_psu_v_out;
extern PSU_SYSFS_ATTR_DATA access_psu_v_out_min;
extern PSU_SYSFS_ATTR_DATA access_psu_v_out_max;

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

void get_psu_duplicate_sysfs(int idx, char *str)
{
    switch (idx)
    {
        case PSU_V_OUT:
            strcpy(str, "in3_input");
            break;
        case PSU_I_OUT:
            strcpy(str, "curr2_input");
            break;
        case PSU_P_OUT:
            strcpy(str, "power2_input");
            break;
        case PSU_FAN1_SPEED:
            strcpy(str, "fan1_input");
            break;
        case PSU_TEMP1_INPUT:
            strcpy(str, "temp1_input");
            break;
        default:
            break;
    }

    return;
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

ssize_t psu_show_custom(struct device *dev, struct device_attribute *da, char *buf)
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
            strcpy(new_str, "");
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

static int __init pddf_psu_patch_init(void)
{   
    access_psu_v_out.show = psu_show_custom;
    access_psu_v_out_min.show = psu_show_custom;
    access_psu_v_out_max.show = psu_show_custom;    
    return 0;
}

static void __exit pddf_psu_patch_exit(void)
{
    return;
}

MODULE_AUTHOR("Fan Xinghua");
MODULE_DESCRIPTION("pddf custom psu api");
MODULE_LICENSE("GPL");

module_init(pddf_psu_patch_init);
module_exit(pddf_psu_patch_exit);