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
 * A pddf kernel module for system status registers
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
#include "pddf_sysstatus_defs.h"



SYSSTATUS_DATA sysstatus_data = {0};

extern int board_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int board_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t show_sysstatus_data(struct device *dev, struct device_attribute *da, char *buf);


PDDF_DATA_ATTR(attr_name, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, 
             (void*)&sysstatus_data.sysstatus_addr_attr.aname, NULL);
PDDF_DATA_ATTR(attr_devaddr, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, 
              sizeof(uint32_t),  (void*)&sysstatus_data.sysstatus_addr_attr.devaddr , NULL);
PDDF_DATA_ATTR(attr_offset, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32,
              sizeof(uint32_t), (void*)&sysstatus_data.sysstatus_addr_attr.offset, NULL);
PDDF_DATA_ATTR(attr_mask, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, 
              sizeof(uint32_t), (void*)&sysstatus_data.sysstatus_addr_attr.mask , NULL);
PDDF_DATA_ATTR(attr_len, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, 
              sizeof(uint32_t), (void*)&sysstatus_data.sysstatus_addr_attr.len , NULL);
PDDF_DATA_ATTR(attr_ops, S_IWUSR, NULL, do_attr_operation, PDDF_CHAR, 8, (void*)&sysstatus_data, NULL);



static struct attribute *sysstatus_addr_attributes[] = {
    &attr_attr_name.dev_attr.attr,
    &attr_attr_devaddr.dev_attr.attr,
    &attr_attr_offset.dev_attr.attr,
    &attr_attr_mask.dev_attr.attr,
    &attr_attr_len.dev_attr.attr,
    &attr_attr_ops.dev_attr.attr,
    NULL
};

PDDF_DATA_ATTR(board_info, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, 32, NULL, NULL);
PDDF_DATA_ATTR(cpld1_version, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(cpld2_version, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(cpld3_version, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(power_module_status, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(system_reset1, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(system_reset2, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(system_reset3, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(system_reset4, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(system_reset5, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(system_reset6, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(system_reset7, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(system_reset8, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(misc1, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(misc2, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);
PDDF_DATA_ATTR(misc3, S_IWUSR|S_IRUGO, show_sysstatus_data, NULL, PDDF_UINT32, sizeof(uint32_t), NULL, NULL);



static struct attribute *sysstatus_data_attributes[] = {
    &attr_board_info.dev_attr.attr,
    &attr_cpld1_version.dev_attr.attr,
    &attr_cpld2_version.dev_attr.attr,
    &attr_cpld3_version.dev_attr.attr,
    &attr_power_module_status.dev_attr.attr,
    &attr_system_reset1.dev_attr.attr,
    &attr_system_reset2.dev_attr.attr,
    &attr_system_reset3.dev_attr.attr,
    &attr_system_reset4.dev_attr.attr,
    &attr_system_reset5.dev_attr.attr,
    &attr_system_reset6.dev_attr.attr,
    &attr_system_reset7.dev_attr.attr,
    &attr_system_reset8.dev_attr.attr,
    &attr_misc1.dev_attr.attr,
    &attr_misc2.dev_attr.attr,
    &attr_misc3.dev_attr.attr,
    NULL
};



static const struct attribute_group pddf_sysstatus_addr_group = {
    .attrs = sysstatus_addr_attributes,
};


static const struct attribute_group pddf_sysstatus_data_group = {
    .attrs = sysstatus_data_attributes,
};


static struct kobject *sysstatus_addr_kobj;
static struct kobject *sysstatus_data_kobj;



ssize_t show_sysstatus_data(struct device *dev, struct device_attribute *da, char *buf)
{

    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    SYSSTATUS_DATA *data = &sysstatus_data;
    struct SYSSTATUS_ADDR_ATTR *sysstatus_addr_attrs = NULL;
    int i, status ;


    for (i=0;i<MAX_ATTRS;i++)
    {
        if (strcmp(data->sysstatus_addr_attrs[i].aname, attr->dev_attr.attr.name) == 0 )
        {
            sysstatus_addr_attrs = &data->sysstatus_addr_attrs[i];
            
        }
    }

    if (sysstatus_addr_attrs==NULL )
    {
        printk(KERN_DEBUG "%s is not supported attribute for this client\n",data->sysstatus_addr_attrs[i].aname);
        status = 0;
    }
    else
    {
        status = board_i2c_cpld_read( sysstatus_addr_attrs->devaddr, sysstatus_addr_attrs->offset);
    }
    
    return sprintf(buf, "0x%x\n", (status&sysstatus_addr_attrs->mask)); 

}



static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    SYSSTATUS_DATA *pdata = (SYSSTATUS_DATA *)(ptr->addr);
    
    pdata->sysstatus_addr_attrs[pdata->len] = pdata->sysstatus_addr_attr;
    pdata->len++;
    pddf_dbg(SYSSTATUS, KERN_ERR "%s: Populating the data for %s\n", __FUNCTION__, pdata->sysstatus_addr_attr.aname);
    memset(&pdata->sysstatus_addr_attr, 0, sizeof(pdata->sysstatus_addr_attr));


    return count;
}




int __init sysstatus_data_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;


    pddf_dbg(SYSSTATUS, "PDDF SYSSTATUS MODULE.. init\n");

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj) 
        return -ENOMEM;

    sysstatus_addr_kobj = kobject_create_and_add("sysstatus", device_kobj);
    if(!sysstatus_addr_kobj) 
        return -ENOMEM;
    
    sysstatus_data_kobj = kobject_create_and_add("sysstatus_data", sysstatus_addr_kobj);
    if(!sysstatus_data_kobj) 
        return -ENOMEM;
    
    
    ret = sysfs_create_group(sysstatus_addr_kobj, &pddf_sysstatus_addr_group);
    if (ret)
    {
        kobject_put(sysstatus_addr_kobj);
        return ret;
    }

    ret = sysfs_create_group(sysstatus_data_kobj, &pddf_sysstatus_data_group);
    if (ret)
    {
        sysfs_remove_group(sysstatus_addr_kobj, &pddf_sysstatus_addr_group);
        kobject_put(sysstatus_data_kobj);
        kobject_put(sysstatus_addr_kobj);
        return ret;
    }


    return ret;
}

void __exit sysstatus_data_exit(void)
{
    pddf_dbg(SYSSTATUS, "PDDF SYSSTATUS  MODULE.. exit\n");
    sysfs_remove_group(sysstatus_data_kobj, &pddf_sysstatus_data_group);
    sysfs_remove_group(sysstatus_addr_kobj, &pddf_sysstatus_addr_group);
    kobject_put(sysstatus_data_kobj);
    kobject_put(sysstatus_addr_kobj);
    pddf_dbg(SYSSTATUS, KERN_ERR "%s: Removed the kobjects for 'SYSSTATUS'\n",__FUNCTION__);
    return;
}

module_init(sysstatus_data_init);
module_exit(sysstatus_data_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("SYSSTATUS platform data");
MODULE_LICENSE("GPL");
