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
 * A pddf kernel module to create access-data attributes for client creation
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
#include <linux/hashtable.h>
#include "pddf_client_defs.h"



NEW_DEV_ATTR pddf_data={0};
EXPORT_SYMBOL(pddf_data);
int showall = 0;


/* CLIENT CREATION DATA ATTR LIST */
PDDF_DATA_ATTR(i2c_type, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, (void*)&pddf_data.i2c_type, NULL);
PDDF_DATA_ATTR(i2c_name, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, (void*)&pddf_data.i2c_name, NULL);
PDDF_DATA_ATTR(parent_bus, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&pddf_data.parent_bus, NULL);
PDDF_DATA_ATTR(dev_type, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, (void*)&pddf_data.dev_type, NULL);
PDDF_DATA_ATTR(dev_id, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&pddf_data.dev_id, NULL);
PDDF_DATA_ATTR(dev_addr, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&pddf_data.dev_addr, NULL);
PDDF_DATA_ATTR(error, S_IRUGO, show_error_code, NULL, PDDF_INT_DEC, sizeof(int), (void*)&pddf_data.error, (void*)&pddf_data.errstr);



static struct attribute *pddf_clients_data_attributes[] = {
    &attr_i2c_type.dev_attr.attr,
    &attr_i2c_name.dev_attr.attr,
    &attr_parent_bus.dev_attr.attr,
    &attr_dev_type.dev_attr.attr,
    &attr_dev_id.dev_attr.attr,
    &attr_dev_addr.dev_attr.attr,
    &attr_error.dev_attr.attr,
    NULL
};

struct attribute_group pddf_clients_data_group = {
    .attrs = pddf_clients_data_attributes,
};
EXPORT_SYMBOL(pddf_clients_data_group);



PDDF_DATA_ATTR(showall, S_IRUGO, show_all_devices, NULL, PDDF_INT_DEC, sizeof(int), (void *)&showall, NULL);

static struct attribute *pddf_allclients_data_attributes[] = {
    &attr_showall.dev_attr.attr,
    NULL
};
struct attribute_group pddf_allclients_data_group = {
    .attrs = pddf_allclients_data_attributes,
};





void set_attr_data(void * ptr)
{
    pddf_data.data=ptr;
}

ssize_t show_all_devices(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret = 0;
    PDDF_ATTR *pptr = (PDDF_ATTR *)da;
    int *ptr = (int *)pptr->addr;

    traverse_device_table();
    ret = sprintf(buf, "Total Devices: %d\n", *ptr);

    return ret;
}

ssize_t show_error_code(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret = 0;
    PDDF_ATTR *pptr = (PDDF_ATTR *)da;
    NEW_DEV_ATTR *ptr = ( NEW_DEV_ATTR *)pptr->addr;

    ret = sprintf(buf, "0x%x:%s\n", (ptr->error), ptr->errstr);

    return ret;
}

void set_error_code(int ecode, char *estr)
{
    pddf_data.error = ecode;
    strcpy(pddf_data.errstr, estr);
    return;
}
EXPORT_SYMBOL(set_error_code);

ssize_t show_pddf_data(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret = 0;
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    /*pddf_dbg(KERN_ERR "[ READ ] DATA ATTR PTR TYPE:%d, ADDR=%p\n", ptr->type, ptr->addr);*/
    switch(ptr->type)
    {
        case PDDF_CHAR:
            ret = sprintf(buf, "%s\n", ptr->addr);
            break;
        case PDDF_UCHAR:
            ret = sprintf(buf, "%d\n", *(unsigned char*)(ptr->addr));
            break;
        case PDDF_INT_DEC:
            ret = sprintf(buf, "%d\n", *(int*)(ptr->addr));
            break;
        case PDDF_INT_HEX:
            ret = sprintf(buf, "0x%x\n", *(int*)(ptr->addr));
            break;
        case PDDF_USHORT:
            ret = sprintf(buf, "0x%x\n", *(unsigned short *)(ptr->addr));
            break;
        case PDDF_UINT32:
            ret = sprintf(buf, "0x%x\n", *(uint32_t *)(ptr->addr));
            break;
        default:
            break;
    }

    return ret;
}
EXPORT_SYMBOL(show_pddf_data);

ssize_t store_pddf_data(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int ret = 0, num = 0;


    PDDF_ATTR *ptr = (PDDF_ATTR *)da;

    switch(ptr->type)
    {
        case PDDF_CHAR:
            strncpy(ptr->addr, buf, strlen(buf)-1); // to discard newline char form buf
            ptr->addr[strlen(buf)-1] = '\0';
            break;
        case PDDF_UCHAR:
            ret = kstrtoint(buf,10,&num);
            if (ret==0)
                *(unsigned char *)(ptr->addr) = (unsigned char)num;
            break;
        case PDDF_INT_DEC:
            ret = kstrtoint(buf,10,&num);
            if (ret==0)
                *(int *)(ptr->addr) = num;
            break;
        case PDDF_INT_HEX:
            ret = kstrtoint(buf,16,&num);
            if (ret==0)
                *(int *)(ptr->addr) = num;
            break;
        case PDDF_USHORT:
            ret = kstrtouint(buf,16,&num);
            if (ret==0)
                *(unsigned short *)(ptr->addr) = (unsigned short)num;
            break;
        case PDDF_UINT32:
            ret = kstrtouint(buf,16,&num);
            if (ret==0)
                *(uint32_t *)(ptr->addr) = (uint32_t)num;
            break;
        default:
            break;
    }

    return count;
}
EXPORT_SYMBOL(store_pddf_data);



DEFINE_HASHTABLE(htable, 8);

int get_hash(char *name)
{
    int i=0;
    int hash=0;
    for(i=0; i<strlen(name); i++)
    {
        hash+=name[i];
    }
    return hash;
}

void init_device_table(void)
{
    hash_init(htable);
}

void add_device_table(char *name, void *ptr)
{
    PDEVICE *hdev=kmalloc(sizeof(PDEVICE), GFP_KERNEL );
    if(!hdev)return;
    strcpy(hdev->name, name);
    hdev->data = ptr;
    pddf_dbg(CLIENT, KERN_ERR "%s: Adding ptr 0x%p to the hash table\n", __FUNCTION__, ptr);
    hash_add(htable, &hdev->node, get_hash(hdev->name));
}
EXPORT_SYMBOL(add_device_table);

void* get_device_table(char *name)
{
    PDEVICE *dev=NULL;
    int i=0;
    
    hash_for_each(htable, i, dev, node) {
        if(strcmp(dev->name, name)==0) {
            return (void *)dev->data;
        }
    }

    return NULL;
}
EXPORT_SYMBOL(get_device_table);

void delete_device_table(char *name)
{
    PDEVICE *dev=NULL;
    int i=0;
    
    hash_for_each(htable, i, dev, node) {
        if(strcmp(dev->name, name)==0) {
            pddf_dbg(CLIENT, KERN_ERR "found entry to delete: %s  0x%p\n", dev->name, dev->data);
            hash_del(&(dev->node));
        }
    }
    return;
}
EXPORT_SYMBOL(delete_device_table);

void traverse_device_table(void )
{
    PDEVICE *dev=NULL;
    int i=0;
    hash_for_each(htable, i, dev, node) {
        pddf_dbg(CLIENT, KERN_ERR "Entry[%d]: %s : 0x%p\n", i, dev->name, dev->data);
    }
    showall = i;
}
EXPORT_SYMBOL(traverse_device_table);

struct kobject *device_kobj;
static struct kobject *pddf_kobj;

struct kobject *get_device_i2c_kobj(void)
{
    return device_kobj;
}

EXPORT_SYMBOL(get_device_i2c_kobj);

int __init pddf_data_init(void)
{
    int ret = 0;


    pddf_dbg(CLIENT, "PDDF_DATA MODULE.. init\n");

    pddf_kobj = kobject_create_and_add("pddf", kernel_kobj);
    if(!pddf_kobj) {
        return -ENOMEM;
    }
    device_kobj = kobject_create_and_add("devices", pddf_kobj);
    if(!device_kobj) {
        return -ENOMEM;
    }

    init_device_table();

    ret = sysfs_create_group(device_kobj, &pddf_allclients_data_group);
    if (ret)
    {
        kobject_put(device_kobj);
        return ret;
    }
    pddf_dbg(CLIENT, "CREATED PDDF ALLCLIENTS CREATION SYSFS GROUP\n");



    return ret;
}

void __exit pddf_data_exit(void)
{

    pddf_dbg(CLIENT, "PDDF_DATA MODULE.. exit\n");
    sysfs_remove_group(device_kobj, &pddf_allclients_data_group);

    kobject_put(device_kobj);
    kobject_put(pddf_kobj);
    pddf_dbg(CLIENT, KERN_ERR "%s: Removed the kernle object for 'pddf' and 'device' \n", __FUNCTION__);
    return;
}

module_init(pddf_data_init);
module_exit(pddf_data_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("pddf data");
MODULE_LICENSE("GPL");

