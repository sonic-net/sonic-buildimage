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
 * A pddf kernel module to create i2C client for optics 
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
#include "pddf_xcvr_defs.h"

static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
extern void* get_device_table(char *name);
extern void delete_device_table(char *name);

XCVR_DATA xcvr_data = {0};

/* XCVR CLIENT DATA */
PDDF_DATA_ATTR(dev_idx, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&xcvr_data.idx, NULL);

PDDF_DATA_ATTR(attr_name, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, (void*)&xcvr_data.xcvr_attr.aname, NULL);
PDDF_DATA_ATTR(attr_devtype, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 8, (void*)&xcvr_data.xcvr_attr.devtype, NULL);
PDDF_DATA_ATTR(attr_devname, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 8, (void*)&xcvr_data.xcvr_attr.devname, NULL);
PDDF_DATA_ATTR(attr_devaddr, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&xcvr_data.xcvr_attr.devaddr, NULL);
PDDF_DATA_ATTR(attr_offset, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&xcvr_data.xcvr_attr.offset, NULL);
PDDF_DATA_ATTR(attr_mask, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&xcvr_data.xcvr_attr.mask, NULL);
PDDF_DATA_ATTR(attr_cmpval, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&xcvr_data.xcvr_attr.cmpval, NULL);
PDDF_DATA_ATTR(attr_len, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&xcvr_data.xcvr_attr.len, NULL);
PDDF_DATA_ATTR(attr_ops, S_IWUSR, NULL, do_attr_operation, PDDF_CHAR, 8, (void*)&xcvr_data, NULL);
PDDF_DATA_ATTR(dev_ops, S_IWUSR, NULL, do_device_operation, PDDF_CHAR, 8, (void*)&xcvr_data, (void*)&pddf_data);


static struct attribute *xcvr_attributes[] = {
    &attr_dev_idx.dev_attr.attr,

    &attr_attr_name.dev_attr.attr,
    &attr_attr_devtype.dev_attr.attr,
    &attr_attr_devname.dev_attr.attr,
    &attr_attr_devaddr.dev_attr.attr,
    &attr_attr_offset.dev_attr.attr,
    &attr_attr_mask.dev_attr.attr,
    &attr_attr_cmpval.dev_attr.attr,
    &attr_attr_len.dev_attr.attr,
    &attr_attr_ops.dev_attr.attr,
    &attr_dev_ops.dev_attr.attr,
    NULL
};

static const struct attribute_group pddf_xcvr_client_data_group = {
    .attrs = xcvr_attributes,
};


static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    XCVR_DATA *pdata = (XCVR_DATA *)(ptr->addr);

    pdata->xcvr_attrs[pdata->len] = pdata->xcvr_attr;
    pdata->len++;
    memset(&pdata->xcvr_attr, 0, sizeof(pdata->xcvr_attr));


    return count;
}

/*PDDF_DATA_ATTR(dev_ops, S_IWUSR, NULL, do_device_operation, PDDF_CHAR, 8, (void*)&pddf_attr, (void*)NULL);*/
static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int i = 0;
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    XCVR_DATA *pdata = (XCVR_DATA *)(ptr->addr);
    NEW_DEV_ATTR *cdata = (NEW_DEV_ATTR *)(ptr->data);

    struct i2c_adapter *adapter;
    struct i2c_board_info board_info;
    struct i2c_client *client_ptr;

    /* Populate the platform data for xcvr */
    if (strncmp(buf, "add", strlen(buf)-1)==0)
    {
        if (strcmp(cdata->dev_type, "pddf_xcvr")==0)
        {
            int num = pdata->len;
            XCVR_PDATA *xcvr_platform_data;
            
            adapter = i2c_get_adapter(cdata->parent_bus);
            /* Allocate the xcvr_platform_data */
            xcvr_platform_data = (XCVR_PDATA *)kzalloc(sizeof(XCVR_PDATA), GFP_KERNEL);
            xcvr_platform_data->xcvr_attrs = (XCVR_ATTR *)kzalloc(num*sizeof(XCVR_ATTR), GFP_KERNEL);


            xcvr_platform_data->idx = pdata->idx;
            xcvr_platform_data->len = pdata->len;

            for (i=0;i<num;i++)
            {
                xcvr_platform_data->xcvr_attrs[i] = pdata->xcvr_attrs[i];
            }

            board_info = (struct i2c_board_info) {
                .platform_data = xcvr_platform_data,
            };

            board_info.addr = cdata->dev_addr;
            strcpy(board_info.type, cdata->dev_type);

            client_ptr = i2c_new_client_device(adapter, &board_info);
            if (!IS_ERR(client_ptr)) {
                i2c_put_adapter(adapter);
                pddf_dbg(XCVR, KERN_ERR "Created a %s client: 0x%p\n", cdata->i2c_name, (void *)client_ptr);
                add_device_table(cdata->i2c_name, (void*)client_ptr);
            }
            else
            {
                i2c_put_adapter(adapter);
                goto free_data;
            }
        }
        else if((strcmp(cdata->dev_type, "optoe1")==0) ||
                (strcmp(cdata->dev_type, "optoe2")==0) ||
                (strcmp(cdata->dev_type, "optoe3")==0))
        {

            adapter = i2c_get_adapter(cdata->parent_bus);
            board_info = (struct i2c_board_info) {
                .platform_data = (void *)NULL,
            };

            board_info.addr = cdata->dev_addr;
            strcpy(board_info.type, cdata->dev_type);

            client_ptr = i2c_new_client_device(adapter, &board_info);
            if(!IS_ERR(client_ptr)) {
                i2c_put_adapter(adapter);
                pddf_dbg(XCVR, KERN_ERR "Created %s, type:%s client: 0x%p\n", cdata->i2c_name, cdata->dev_type, (void *)client_ptr);
                add_device_table(cdata->i2c_name, (void*)client_ptr);
            }
            else
            {
                i2c_put_adapter(adapter);
                printk(KERN_ERR "Error creating a client %s on 0x%x, client_ptr:0x%p\n", board_info.type, board_info.addr, (void *)client_ptr);
                goto free_data;
            }
        }
        else
        {
            printk(KERN_ERR "%s:Unknown type of device %s. Unable to create I2C client for it\n",__FUNCTION__, cdata->dev_type);
        }
    }
    else if (strncmp(buf, "delete", strlen(buf)-1)==0)
    {
        /*Get the i2c_client handle for the created client*/
        client_ptr = (struct i2c_client *)get_device_table(cdata->i2c_name);
        if (client_ptr)
        {
            pddf_dbg(XCVR, KERN_ERR "Removing %s client: 0x%p\n", cdata->i2c_name, (void *)client_ptr);
            i2c_unregister_device(client_ptr);
            delete_device_table(cdata->i2c_name);
        }
        else
        {
            pddf_dbg(XCVR, KERN_ERR "Unable to get the client handle for %s\n", cdata->i2c_name);
        }
    }
    else
    {
        printk(KERN_ERR "PDDF_ERROR: %s: Invalid value for dev_ops %s", __FUNCTION__, buf);
    }

    goto clear_data;

free_data:
    if (board_info.platform_data)
    {
        XCVR_PDATA *xcvr_platform_data = board_info.platform_data;
        if (xcvr_platform_data->xcvr_attrs)
        {
            printk(KERN_ERR "%s: Unable to create i2c client. Freeing the platform subdata\n", __FUNCTION__);
            kfree(xcvr_platform_data->xcvr_attrs);
        }
        printk(KERN_ERR "%s: Unable to create i2c client. Freeing the platform data\n", __FUNCTION__);
        kfree(xcvr_platform_data);
    }

clear_data:
    memset(pdata, 0, sizeof(XCVR_DATA));
    /*TODO: free the data cdata->data if data is dynal=mically allocated*/
    memset(cdata, 0, sizeof(NEW_DEV_ATTR));
    return count;
}

struct kobject *xcvr_kobj;
struct kobject *i2c_kobj;
int __init pddf_data_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;

    pddf_dbg(XCVR, KERN_ERR "XCVR PDDF MODULE.. init\n");

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj) 
        return -ENOMEM;

    xcvr_kobj = kobject_create_and_add("xcvr", device_kobj);
    if(!xcvr_kobj) 
        return -ENOMEM;
    i2c_kobj = kobject_create_and_add("i2c", xcvr_kobj);
    if(!i2c_kobj) 
        return -ENOMEM;
    
    ret = sysfs_create_group(i2c_kobj, &pddf_clients_data_group);
    if (ret)
    {
        kobject_put(i2c_kobj);
        kobject_put(xcvr_kobj);
        return ret;
    }
    pddf_dbg(XCVR, "CREATED SFP I2C CLIENTS CREATION SYSFS GROUP\n");

    ret = sysfs_create_group(i2c_kobj, &pddf_xcvr_client_data_group);
    if (ret)
    {
        sysfs_remove_group(i2c_kobj, &pddf_clients_data_group);
        kobject_put(i2c_kobj);
        kobject_put(xcvr_kobj);
        return ret;
    }
    pddf_dbg(XCVR, "CREATED PDDF SFP DATA SYSFS GROUP\n");
    
    return ret;
}

void __exit pddf_data_exit(void)
{

    pddf_dbg(XCVR, "XCVR PDDF MODULE.. exit\n");
    sysfs_remove_group(i2c_kobj, &pddf_xcvr_client_data_group);
    sysfs_remove_group(i2c_kobj, &pddf_clients_data_group);
    kobject_put(i2c_kobj);
    kobject_put(xcvr_kobj);
    pddf_dbg(XCVR, KERN_ERR "%s: Removed the kobjects for 'i2c' and 'xcvr'\n",__FUNCTION__);

    return;
}

module_init(pddf_data_init);
module_exit(pddf_data_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("sfp platform data");
MODULE_LICENSE("GPL");
