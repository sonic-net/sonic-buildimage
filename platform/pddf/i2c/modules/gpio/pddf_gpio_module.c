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
 * PDDF generic kernle module to create the I2C client for pca955x type of GPIO module
 */

#define __STDC_WANT_LIB_EXT1__ 1
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
#include "pddf_gpio_defs.h"


static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
extern void* get_device_table(char *name);
extern void delete_device_table(char *name);

static int base_gpio_num = 0xf0;
GPIO_DATA gpio_data = {0};

/* GPIO CLIENT DATA */
PDDF_DATA_ATTR(gpio_base, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&gpio_data.gpio_base, NULL);
PDDF_DATA_ATTR(dev_ops, S_IWUSR, NULL, do_device_operation, PDDF_CHAR, 8, (void*)&gpio_data, (void*)&pddf_data);



static struct attribute *gpio_attributes[] = {
    &attr_gpio_base.dev_attr.attr,
    &attr_dev_ops.dev_attr.attr,
    NULL
};

static const struct attribute_group pddf_gpio_client_data_group = {
    .attrs = gpio_attributes,
};


struct i2c_board_info *i2c_get_gpio_board_info(GPIO_DATA* mdata, NEW_DEV_ATTR *device_data)
{
    static struct i2c_board_info board_info;
    struct pca953x_platform_data *gpio_platform_data=NULL;
    int def_num_gpios, base;

    gpio_platform_data = (struct pca953x_platform_data *)kzalloc(sizeof (struct pca953x_platform_data), GFP_KERNEL);

    if (strncmp(device_data->dev_type, "pca9554", strlen("pca9554")) == 0 ||
            strncmp(device_data->dev_type, "pca9534", strlen("pca9534")) == 0 ||
            strncmp(device_data->dev_type, "pca9538", strlen("pca9538")) == 0 ||
            strncmp(device_data->dev_type, "tca6408", strlen("tca6408")) == 0 ||
            strncmp(device_data->dev_type, "tca9554", strlen("tca9554")) == 0)
        def_num_gpios = 0x8;
    else if (strncmp(device_data->dev_type, "pca9555", strlen("pca9555")) == 0 ||
            strncmp(device_data->dev_type, "pca9535", strlen("pca9535")) == 0 ||
            strncmp(device_data->dev_type, "pca9539", strlen("pca9539")) == 0 ||
            strncmp(device_data->dev_type, "pca9575", strlen("pca9575")) == 0 ||
            strncmp(device_data->dev_type, "tca6416", strlen("tca6416")) == 0 ||
            strncmp(device_data->dev_type, "tca9539", strlen("tca9539")) == 0)
        def_num_gpios = 0x10;
    else if (strncmp(device_data->dev_type, "tca6424", strlen("tca6424")) == 0)
        def_num_gpios = 0x18;
    else if (strncmp(device_data->dev_type, "pca9698", strlen("pca9698")) == 0 ||
            strncmp(device_data->dev_type, "pca9505", strlen("pca9505")) == 0)
        def_num_gpios = 0x28;
    else
    {
        printk(KERN_ERR "%s: Unknown type of gpio device\n", __FUNCTION__);
        return NULL;
    }

    if(mdata->gpio_base == 0) {
        base = base_gpio_num;
        base_gpio_num += def_num_gpios;
    }
    else {
        base = mdata->gpio_base;
    }

    gpio_platform_data->gpio_base = base;

    board_info = (struct i2c_board_info) {
        .platform_data = gpio_platform_data,
    };

    board_info.addr = device_data->dev_addr;
    strcpy(board_info.type, device_data->dev_type);

    return &board_info;
}


static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    GPIO_DATA *gpio_ptr = (GPIO_DATA *)(ptr->addr);
    NEW_DEV_ATTR *device_ptr = (NEW_DEV_ATTR *)(ptr->data);
    struct i2c_adapter *adapter;
    struct i2c_board_info *board_info;
    struct i2c_client *client_ptr;

    if (strncmp(buf, "add", strlen(buf)-1)==0)
    {
        adapter = i2c_get_adapter(device_ptr->parent_bus);
        board_info = i2c_get_gpio_board_info(gpio_ptr, device_ptr);

        /*pddf_dbg(KERN_ERR "Creating a client %s on 0x%x, platform_data 0x%x\n", board_info->type, board_info->addr, board_info->platform_data);*/
        client_ptr = i2c_new_client_device(adapter, board_info);

        i2c_put_adapter(adapter);
        if (!IS_ERR(client_ptr))
        {
            pddf_dbg(GPIO, KERN_ERR "Created %s client: 0x%p\n", device_ptr->i2c_name, (void *)client_ptr);
            add_device_table(device_ptr->i2c_name, (void*)client_ptr);
        }
        else
        {
            kfree(board_info);
            goto free_data;
        }
    }
    else if (strncmp(buf, "delete", strlen(buf)-1)==0)
    {
        /*Get the i2c_client handle for the created client*/
        client_ptr = (struct i2c_client *)get_device_table(device_ptr->i2c_name);
        if (client_ptr)
        {
            struct pca953x_platform_data *gpio_platform_data = (struct pca953x_platform_data *)client_ptr->dev.platform_data;
            pddf_dbg(GPIO, KERN_ERR "Removing %s client: 0x%p\n", device_ptr->i2c_name, (void *)client_ptr);
            i2c_unregister_device(client_ptr);
            /*TODO: Nullyfy the platform data*/
            if (gpio_platform_data)
                kfree(gpio_platform_data);
            delete_device_table(device_ptr->i2c_name);
        }
        else
        {
            printk(KERN_ERR "Unable to get the client handle for %s\n", device_ptr->i2c_name);
        }
    }
    else
    {
        printk(KERN_ERR "PDDF_ERROR: %s: Invalid value for dev_ops %s", __FUNCTION__, buf);
    }

free_data:

#ifdef __STDC_LIB_EXT1__
    memset_s(gpio_ptr, sizeof(GPIO_DATA), 0, sizeof(GPIO_DATA));
#else
    memset(gpio_ptr, 0, sizeof(GPIO_DATA));
#endif
    /*TODO: free the device_ptr->data is dynamically allocated*/
#ifdef __STDC_LIB_EXT1__
    memset_s(device_ptr, sizeof(NEW_DEV_ATTR), 0 , sizeof(NEW_DEV_ATTR));
#else
    memset(device_ptr, 0 , sizeof(NEW_DEV_ATTR));
#endif

    return count;
}


static struct kobject *gpio_kobj;

int __init gpio_data_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;


    pddf_dbg(GPIO, "GPIO_DATA MODULE.. init\n");

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj)
        return -ENOMEM;

    gpio_kobj = kobject_create_and_add("gpio", device_kobj);
    if(!gpio_kobj)
        return -ENOMEM;


    ret = sysfs_create_group(gpio_kobj, &pddf_clients_data_group);
    if (ret)
    {
        kobject_put(gpio_kobj);
        return ret;
    }
    pddf_dbg(GPIO, "CREATED PDDF I2C CLIENTS CREATION SYSFS GROUP\n");

    ret = sysfs_create_group(gpio_kobj, &pddf_gpio_client_data_group);
    if (ret)
    {
        sysfs_remove_group(gpio_kobj, &pddf_clients_data_group);
        kobject_put(gpio_kobj);
        return ret;
    }
    pddf_dbg(GPIO, "CREATED GPIO DATA SYSFS GROUP\n");

    return ret;
}

void __exit gpio_data_exit(void)
{
    pddf_dbg(GPIO, "GPIO_DATA MODULE.. exit\n");
    sysfs_remove_group(gpio_kobj, &pddf_gpio_client_data_group);
    sysfs_remove_group(gpio_kobj, &pddf_clients_data_group);
    kobject_put(gpio_kobj);
    pddf_dbg(GPIO, KERN_ERR "%s: Removed the kobjects for 'gpio'\n",__FUNCTION__);
    return;
}

module_init(gpio_data_init);
module_exit(gpio_data_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("gpio platform data");
MODULE_LICENSE("GPL");
