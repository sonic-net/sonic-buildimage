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
 * A pddf kernel module to create I2C client for FAN controller 
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
#include "pddf_client_defs.h"
#include "pddf_fan_defs.h"


static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
extern void *get_fan_access_data(char *);
extern void* get_device_table(char *name);
extern void delete_device_table(char *name);

FAN_DATA fan_data = {0};



/* FAN CLIENT DATA */
PDDF_DATA_ATTR(num_fantrays, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&fan_data.num_fantrays, NULL);

PDDF_DATA_ATTR(attr_name, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, (void*)&fan_data.fan_attr.aname, NULL);
PDDF_DATA_ATTR(attr_devtype, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 8, (void*)&fan_data.fan_attr.devtype, NULL);
PDDF_DATA_ATTR(attr_devname, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 8, (void*)&fan_data.fan_attr.devname, NULL);
PDDF_DATA_ATTR(attr_devaddr, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&fan_data.fan_attr.devaddr, NULL);
PDDF_DATA_ATTR(attr_offset, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&fan_data.fan_attr.offset, NULL);
PDDF_DATA_ATTR(attr_mask, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&fan_data.fan_attr.mask, NULL);
PDDF_DATA_ATTR(attr_cmpval, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&fan_data.fan_attr.cmpval, NULL);
PDDF_DATA_ATTR(attr_len, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&fan_data.fan_attr.len, NULL);
PDDF_DATA_ATTR(attr_mult, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&fan_data.fan_attr.mult, NULL);
PDDF_DATA_ATTR(attr_is_divisor, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UCHAR, sizeof(unsigned char), (void*)&fan_data.fan_attr.is_divisor, NULL);
PDDF_DATA_ATTR(attr_ops, S_IWUSR, NULL, do_attr_operation, PDDF_CHAR, 8, (void*)&fan_data, NULL);
PDDF_DATA_ATTR(dev_ops, S_IWUSR, NULL, do_device_operation, PDDF_CHAR, 8, (void*)&fan_data, (void*)&pddf_data);



static struct attribute *fan_attributes[] = {
    &attr_num_fantrays.dev_attr.attr,
    &attr_attr_name.dev_attr.attr,
    &attr_attr_devtype.dev_attr.attr,
    &attr_attr_devname.dev_attr.attr,
    &attr_attr_devaddr.dev_attr.attr,
    &attr_attr_offset.dev_attr.attr,
    &attr_attr_mask.dev_attr.attr,
    &attr_attr_cmpval.dev_attr.attr,
    &attr_attr_len.dev_attr.attr,
    &attr_attr_mult.dev_attr.attr,
    &attr_attr_is_divisor.dev_attr.attr,
    &attr_attr_ops.dev_attr.attr,
    &attr_dev_ops.dev_attr.attr,
    NULL
};

static const struct attribute_group pddf_fan_client_data_group = {
    .attrs = fan_attributes,
};


static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    FAN_DATA *fdata = (FAN_DATA *)(ptr->addr);
    FAN_SYSFS_ATTR_DATA_ENTRY *entry_ptr;


    fdata->fan_attrs[fdata->len] = fdata->fan_attr;
    entry_ptr = get_fan_access_data(fdata->fan_attrs[fdata->len].aname);
    if (entry_ptr != NULL && entry_ptr->a_ptr != NULL)
    {
        fdata->fan_attrs[fdata->len].access_data = entry_ptr->a_ptr ;
    }

    fdata->len++;
    memset(&fdata->fan_attr, 0, sizeof(fdata->fan_attr));


    return count;
}

struct i2c_board_info *i2c_get_fan_board_info(FAN_DATA *fdata, NEW_DEV_ATTR *cdata)
{
    int num = fdata->len;
    int i = 0;
    static struct i2c_board_info board_info;
    FAN_PDATA *fan_platform_data;

    if (strcmp(cdata->dev_type, "fan_ctrl") == 0 || strcmp(cdata->dev_type, "fan_eeprom") == 0 ||
        strcmp(cdata->dev_type, "fan_cpld") == 0 || strcmp(cdata->dev_type, "fan_multifpgapci") == 0) {
        /* Allocate the fan_platform_data */
        fan_platform_data = (FAN_PDATA *)kzalloc(sizeof(FAN_PDATA), GFP_KERNEL);
        fan_platform_data->fan_attrs = (FAN_DATA_ATTR *)kzalloc(num*sizeof(FAN_DATA_ATTR), GFP_KERNEL);

        fan_platform_data->num_fantrays = fdata->num_fantrays;
        fan_platform_data->len = fdata->len;

        for (i=0;i<num;i++)
        {
            fan_platform_data->fan_attrs[i] = fdata->fan_attrs[i];
        }

        board_info = (struct i2c_board_info) {
            .platform_data = fan_platform_data,
        };

        board_info.addr = cdata->dev_addr;
        strcpy(board_info.type, cdata->dev_type);
    }
    else
    {
        printk(KERN_ERR "%s:Unknown type of device %s. Unable to create I2C client for it\n",__FUNCTION__, cdata->dev_type);
    }

    return &board_info;
}


static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    FAN_DATA *fdata = (FAN_DATA *)(ptr->addr);
    NEW_DEV_ATTR *cdata = (NEW_DEV_ATTR *)(ptr->data);
    struct i2c_adapter *adapter;
    struct i2c_board_info *board_info;
    struct i2c_client *client_ptr;

    if (strncmp(buf, "add", strlen(buf)-1)==0)
    {
        adapter = i2c_get_adapter(cdata->parent_bus);
        if (!adapter) {
            printk(KERN_ERR "PDDF_ERROR: %s: Failed to get i2c adapter for bus %d\n", __FUNCTION__, cdata->parent_bus);
            goto clear_data;
        }
        board_info = i2c_get_fan_board_info(fdata, cdata);

        /* Populate the platform data for fan */
        client_ptr = i2c_new_client_device(adapter, board_info);
        
        if(!IS_ERR(client_ptr))
        {
            i2c_put_adapter(adapter);
            pddf_dbg(FAN, KERN_ERR "Created a %s client: 0x%p\n", cdata->i2c_name, (void *)client_ptr);
            add_device_table(cdata->i2c_name, (void*)client_ptr);
        }
        else 
        {
            i2c_put_adapter(adapter);
            goto free_data;
        }
    }
    else if (strncmp(buf, "delete", strlen(buf)-1)==0)
    {
        /*Get the i2c_client handle for the created client*/
        client_ptr = (struct i2c_client *)get_device_table(cdata->i2c_name);
        if (client_ptr)
        {
            pddf_dbg(FAN, KERN_ERR "Removing %s client: 0x%p\n", cdata->i2c_name, (void *)client_ptr);
            i2c_unregister_device(client_ptr);
            delete_device_table(cdata->i2c_name);
        }
        else
        {
            printk(KERN_ERR "Unable to get the client handle for %s\n", cdata->i2c_name);
        }
    }
    else
    {
        printk(KERN_ERR "PDDF_ERROR: %s: Invalid value for dev_ops %s", __FUNCTION__, buf);
    }

    goto clear_data;

free_data:
    if (board_info->platform_data)
    {
        FAN_PDATA *fan_platform_data = board_info->platform_data;
        if (fan_platform_data->fan_attrs)
        {
            printk(KERN_ERR "%s: Unable to create i2c client. Freeing the platform subdata\n", __FUNCTION__);
            kfree(fan_platform_data->fan_attrs);
        }
        printk(KERN_ERR "%s: Unable to create i2c client. Freeing the platform data\n", __FUNCTION__);
        kfree(fan_platform_data);
    }
clear_data:
    memset(fdata, 0, sizeof(FAN_DATA));
    /*TODO: free the data cdata->data if data is dynal=mically allocated*/
    memset(cdata, 0, sizeof(NEW_DEV_ATTR));
    return count;
}


static struct kobject *fan_kobj;
static struct kobject *i2c_kobj;

int __init pddf_data_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;


    pddf_dbg(FAN, "PDDF FAN MODULE.. init\n");

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj) 
        return -ENOMEM;

    fan_kobj = kobject_create_and_add("fan", device_kobj);
    if(!fan_kobj) 
        return -ENOMEM;
    i2c_kobj = kobject_create_and_add("i2c", fan_kobj);
    if(!i2c_kobj) 
        return -ENOMEM;
    
    ret = sysfs_create_group(i2c_kobj, &pddf_clients_data_group);
    if (ret)
    {
        kobject_put(i2c_kobj);
        kobject_put(fan_kobj);
        return ret;
    }
    pddf_dbg(FAN, "CREATED FAN I2C CLIENTS CREATION SYSFS GROUP\n");

    ret = sysfs_create_group(i2c_kobj, &pddf_fan_client_data_group);
    if (ret)
    {
        sysfs_remove_group(i2c_kobj, &pddf_clients_data_group);
        kobject_put(i2c_kobj);
        kobject_put(fan_kobj);
        return ret;
    }
    pddf_dbg(FAN, "CREATED PDDF FAN DATA SYSFS GROUP\n");
    


    return ret;
}

void __exit pddf_data_exit(void)
{
    pddf_dbg(FAN, "PDDF FAN MODULE.. exit\n");
    sysfs_remove_group(i2c_kobj, &pddf_fan_client_data_group);
    sysfs_remove_group(i2c_kobj, &pddf_clients_data_group);
    kobject_put(i2c_kobj);
    kobject_put(fan_kobj);
    pddf_dbg(FAN, KERN_ERR "%s: Removed the kobjects for 'i2c' and 'fan'\n",__FUNCTION__);
    return;
}

module_init(pddf_data_init);
module_exit(pddf_data_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("fan platform data");
MODULE_LICENSE("GPL");
