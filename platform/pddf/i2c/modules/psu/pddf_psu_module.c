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
 * A pddf kernel module to create I2C client for PSU 
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
#include "pddf_psu_defs.h"


static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
extern void *get_psu_access_data(char *);
extern void* get_device_table(char *name);
extern void delete_device_table(char *name);

PSU_DATA psu_data = {0};



/* PSU CLIENT DATA */
PDDF_DATA_ATTR(psu_idx, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&psu_data.idx, NULL);
PDDF_DATA_ATTR(psu_fans, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&psu_data.num_psu_fans, NULL);
PDDF_DATA_ATTR(psu_thermals, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&psu_data.num_psu_thermals, NULL);
PDDF_DATA_ATTR(psu_temp_high_thresh_bitmap, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&psu_data.psu_temp_high_thresh_bitmap, NULL);


PDDF_DATA_ATTR(attr_name, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, (void*)&psu_data.psu_attr.aname, NULL);
PDDF_DATA_ATTR(attr_devtype, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 8, (void*)&psu_data.psu_attr.devtype, NULL);
PDDF_DATA_ATTR(attr_devname, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 8, (void*)&psu_data.psu_attr.devname, NULL);
PDDF_DATA_ATTR(attr_data_format, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, (void*)&psu_data.psu_attr.data_format, NULL);
PDDF_DATA_ATTR(attr_devaddr, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&psu_data.psu_attr.devaddr, NULL);
PDDF_DATA_ATTR(attr_offset, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&psu_data.psu_attr.offset, NULL);
PDDF_DATA_ATTR(attr_mask, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&psu_data.psu_attr.mask, NULL);
PDDF_DATA_ATTR(attr_cmpval, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&psu_data.psu_attr.cmpval, NULL);
PDDF_DATA_ATTR(attr_len, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&psu_data.psu_attr.len, NULL);
PDDF_DATA_ATTR(attr_m, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&psu_data.psu_attr.m, NULL);
PDDF_DATA_ATTR(attr_b, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&psu_data.psu_attr.b, NULL);
PDDF_DATA_ATTR(attr_r, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&psu_data.psu_attr.r, NULL);
PDDF_DATA_ATTR(attr_ops, S_IWUSR, NULL, do_attr_operation, PDDF_CHAR, 8, (void*)&psu_data, NULL);
PDDF_DATA_ATTR(dev_ops, S_IWUSR, NULL, do_device_operation, PDDF_CHAR, 8, (void*)&psu_data, (void*)&pddf_data);



static struct attribute *psu_attributes[] = {
    &attr_psu_idx.dev_attr.attr,
    &attr_psu_fans.dev_attr.attr,
    &attr_psu_thermals.dev_attr.attr,
    &attr_psu_temp_high_thresh_bitmap.dev_attr.attr,

    &attr_attr_name.dev_attr.attr,
    &attr_attr_devtype.dev_attr.attr,
    &attr_attr_devname.dev_attr.attr,
    &attr_attr_data_format.dev_attr.attr,
    &attr_attr_devaddr.dev_attr.attr,
    &attr_attr_offset.dev_attr.attr,
    &attr_attr_mask.dev_attr.attr,
    &attr_attr_cmpval.dev_attr.attr,
    &attr_attr_len.dev_attr.attr,
    &attr_attr_m.dev_attr.attr,
    &attr_attr_b.dev_attr.attr,
    &attr_attr_r.dev_attr.attr,
    &attr_attr_ops.dev_attr.attr,
    &attr_dev_ops.dev_attr.attr,
    NULL
};

static const struct attribute_group pddf_psu_client_data_group = {
    .attrs = psu_attributes,
};


static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    PSU_DATA *pdata = (PSU_DATA *)(ptr->addr);
    PSU_SYSFS_ATTR_DATA_ENTRY *access_ptr;


    pdata->psu_attrs[pdata->len] = pdata->psu_attr;
    access_ptr = get_psu_access_data(pdata->psu_attrs[pdata->len].aname);
    if (access_ptr != NULL && access_ptr->a_ptr != NULL)
    {
        pdata->psu_attrs[pdata->len].access_data = access_ptr->a_ptr ;
    }


    pdata->len++;
    memset(&pdata->psu_attr, 0, sizeof(pdata->psu_attr));


    return count;
}

struct i2c_board_info *i2c_get_psu_board_info(PSU_DATA *pdata, NEW_DEV_ATTR *cdata)
{
    int num = pdata->len;
    int i = 0;
    static struct i2c_board_info board_info;
    PSU_PDATA *psu_platform_data;
    
    
    if (strcmp(cdata->dev_type, "psu_pmbus")==0 || strcmp(cdata->dev_type, "psu_eeprom")==0 )
    {
        /* Allocate the psu_platform_data */
        psu_platform_data = (PSU_PDATA *)kzalloc(sizeof(PSU_PDATA), GFP_KERNEL);
        psu_platform_data->psu_attrs = (PSU_DATA_ATTR *)kzalloc(num*sizeof(PSU_DATA_ATTR), GFP_KERNEL);


        psu_platform_data->idx = pdata->idx;
        psu_platform_data->num_psu_fans = pdata->num_psu_fans;
        psu_platform_data->num_psu_thermals = pdata->num_psu_thermals;
        psu_platform_data->psu_temp_high_thresh_bitmap = pdata->psu_temp_high_thresh_bitmap;
        psu_platform_data->len = pdata->len;

        for (i=0;i<num;i++)
        {
            psu_platform_data->psu_attrs[i] = pdata->psu_attrs[i];
        }

        board_info = (struct i2c_board_info) {
            .platform_data = psu_platform_data,
        };

        board_info.addr = cdata->dev_addr;
        strcpy(board_info.type, cdata->dev_type);
    }
    else
    {
        printk(KERN_ERR "%s:Unknown type of device %s. Unable to clreate I2C client for it\n",__FUNCTION__, cdata->dev_type);
    }

    return &board_info;
}


static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    PSU_DATA *pdata = (PSU_DATA *)(ptr->addr);
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
        board_info = i2c_get_psu_board_info(pdata, cdata);

        /* Populate the platform data for psu */
        client_ptr = i2c_new_client_device(adapter, board_info);
        
        if(!IS_ERR(client_ptr))
        {
            i2c_put_adapter(adapter);
            pddf_dbg(PSU, KERN_ERR "Created a %s client: 0x%p\n", cdata->i2c_name , (void *)client_ptr);
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
            pddf_dbg(PSU, KERN_ERR "Removing %s client: 0x%p\n", cdata->i2c_name, (void *)client_ptr);
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
        PSU_PDATA *psu_platform_data = board_info->platform_data;
        if (psu_platform_data->psu_attrs)
        {
            printk(KERN_ERR "%s: Unable to create i2c client. Freeing the platform subdata\n", __FUNCTION__);
            kfree(psu_platform_data->psu_attrs);
        }
        printk(KERN_ERR "%s: Unable to create i2c client. Freeing the platform data\n", __FUNCTION__);
        kfree(psu_platform_data);
    }

clear_data:
    memset(pdata, 0, sizeof(PSU_DATA));
    /*TODO: free the data cdata->data if data is dynal=mically allocated*/
    memset(cdata, 0, sizeof(NEW_DEV_ATTR));
    return count;
}


static struct kobject *psu_kobj;
static struct kobject *i2c_kobj;

int __init pddf_data_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;


    pddf_dbg(PSU, "PDDF_DATA MODULE.. init\n");

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj) 
        return -ENOMEM;

    psu_kobj = kobject_create_and_add("psu", device_kobj);
    if(!psu_kobj) 
        return -ENOMEM;
    i2c_kobj = kobject_create_and_add("i2c", psu_kobj);
    if(!i2c_kobj) 
        return -ENOMEM;
    
    ret = sysfs_create_group(i2c_kobj, &pddf_clients_data_group);
    if (ret)
    {
        kobject_put(i2c_kobj);
        kobject_put(psu_kobj);
        return ret;
    }
    pddf_dbg(PSU, "CREATED PSU I2C CLIENTS CREATION SYSFS GROUP\n");

    ret = sysfs_create_group(i2c_kobj, &pddf_psu_client_data_group);
    if (ret)
    {
        sysfs_remove_group(i2c_kobj, &pddf_clients_data_group);
        kobject_put(i2c_kobj);
        kobject_put(psu_kobj);
        return ret;
    }
    pddf_dbg(PSU, "CREATED PDDF PSU DATA SYSFS GROUP\n");
    
    return ret;
}

void __exit pddf_data_exit(void)
{

    pddf_dbg(PSU, "PDDF_DATA MODULE.. exit\n");
    sysfs_remove_group(i2c_kobj, &pddf_psu_client_data_group);
    sysfs_remove_group(i2c_kobj, &pddf_clients_data_group);
    kobject_put(i2c_kobj);
    kobject_put(psu_kobj);
    pddf_dbg(PSU, KERN_ERR "%s: Removed the kobjects for 'i2c' and 'psu'\n",__FUNCTION__);

    return;
}

module_init(pddf_data_init);
module_exit(pddf_data_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("psu platform data");
MODULE_LICENSE("GPL");
