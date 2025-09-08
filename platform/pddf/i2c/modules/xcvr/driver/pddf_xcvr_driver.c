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
 * A pddf kernel driver module for Optic component
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
#include "pddf_xcvr_api.h"


struct pddf_ops_t pddf_xcvr_ops = {
    .pre_init = NULL,
    .post_init = NULL,

    .pre_probe = NULL,
    .post_probe = NULL,

    .pre_remove = NULL,
    .post_remove = NULL,

    .pre_exit = NULL,
    .post_exit = NULL,
};
EXPORT_SYMBOL(pddf_xcvr_ops);

XCVR_SYSFS_ATTR_OPS xcvr_ops[XCVR_ATTR_MAX] = {
    {XCVR_PRESENT, get_module_presence, NULL, sonic_i2c_get_mod_pres, NULL, NULL, NULL, NULL, NULL},
    {XCVR_RESET, get_module_reset, NULL, sonic_i2c_get_mod_reset, NULL, set_module_reset, NULL, sonic_i2c_set_mod_reset, NULL},
    {XCVR_INTR_STATUS, get_module_intr_status, NULL, sonic_i2c_get_mod_intr_status, NULL, NULL, NULL, NULL, NULL},
    {XCVR_LPMODE, get_module_lpmode, NULL, sonic_i2c_get_mod_lpmode, NULL, set_module_lpmode, NULL, sonic_i2c_set_mod_lpmode, NULL},
    {XCVR_RXLOS, get_module_rxlos, NULL, sonic_i2c_get_mod_rxlos, NULL, NULL, NULL, NULL, NULL},
    {XCVR_TXDISABLE, get_module_txdisable, NULL, sonic_i2c_get_mod_txdisable, NULL, set_module_txdisable, NULL, sonic_i2c_set_mod_txdisable, NULL},
    {XCVR_TXFAULT, get_module_txfault, NULL, sonic_i2c_get_mod_txfault, NULL, NULL, NULL, NULL, NULL},
};
EXPORT_SYMBOL(xcvr_ops);


/* sysfs attributes  
 */
static SENSOR_DEVICE_ATTR(xcvr_present, S_IWUSR|S_IRUGO, get_module_presence,   NULL, XCVR_PRESENT);
static SENSOR_DEVICE_ATTR(xcvr_reset,   S_IWUSR|S_IRUGO, get_module_reset, set_module_reset, XCVR_RESET);
static SENSOR_DEVICE_ATTR(xcvr_intr_status, S_IWUSR|S_IRUGO, get_module_intr_status, NULL, XCVR_INTR_STATUS);
static SENSOR_DEVICE_ATTR(xcvr_lpmode,  S_IWUSR|S_IRUGO, get_module_lpmode, set_module_lpmode, XCVR_LPMODE);
static SENSOR_DEVICE_ATTR(xcvr_rxlos,   S_IWUSR|S_IRUGO, get_module_rxlos, NULL, XCVR_RXLOS);
static SENSOR_DEVICE_ATTR(xcvr_txdisable,   S_IWUSR|S_IRUGO, get_module_txdisable, set_module_txdisable, XCVR_TXDISABLE);
static SENSOR_DEVICE_ATTR(xcvr_txfault, S_IWUSR|S_IRUGO, get_module_txfault, NULL, XCVR_TXFAULT);

/* List of all the xcvr attribute structures 
 * to get name, use sensor_dev_attr_<>.dev_attr.attr.name
 * to get the id, use sensor_dev_attr_<>.dev_attr.index 
 */
static struct sensor_device_attribute *xcvr_attr_list[MAX_XCVR_ATTRS] = {
    &sensor_dev_attr_xcvr_present,
    &sensor_dev_attr_xcvr_reset,
    &sensor_dev_attr_xcvr_intr_status,
    &sensor_dev_attr_xcvr_lpmode,
    &sensor_dev_attr_xcvr_rxlos,
    &sensor_dev_attr_xcvr_txdisable,
    &sensor_dev_attr_xcvr_txfault,
};

static struct attribute *xcvr_attributes[MAX_XCVR_ATTRS] = {NULL};

static const struct attribute_group xcvr_group = {
    .attrs = xcvr_attributes,
};

enum xcvr_intf 
{
    XCVR_CTRL_INTF,
};

static const struct i2c_device_id xcvr_ids[] = {
    { "pddf_xcvr", XCVR_CTRL_INTF },
    {}
};

MODULE_DEVICE_TABLE(i2c, xcvr_ids);

static int xcvr_probe(struct i2c_client *client)
{
    struct xcvr_data *data;
    int status =0;
    int i,j,num;
    XCVR_PDATA *xcvr_platform_data;
    XCVR_ATTR *attr_data;
    struct i2c_device_id *dev_id;

    if (client == NULL) {
        pddf_dbg(XCVR, "NULL Client.. \n");
        goto exit;
    }

    if (pddf_xcvr_ops.pre_probe)
    {
        dev_id = i2c_match_id(xcvr_ids, client);
        status = (pddf_xcvr_ops.pre_probe)(client, dev_id);
        if (status != 0)
            goto exit;
    }

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct xcvr_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;

    dev_info(&client->dev, "chip found\n");

    /* Take control of the platform data */
    xcvr_platform_data = (XCVR_PDATA *)(client->dev.platform_data);
    num = xcvr_platform_data->len;
    data->index = xcvr_platform_data->idx - 1;
    mutex_init(&data->update_lock);

    /* Add supported attr in the 'attributes' list */
    for (i=0; i<num; i++)
    {
        struct attribute *aptr = NULL;
        attr_data = xcvr_platform_data->xcvr_attrs + i;
        for(j=0;j<XCVR_ATTR_MAX;j++)
        {
            aptr = &xcvr_attr_list[j]->dev_attr.attr;

            if (strncmp(aptr->name, attr_data->aname, strlen(attr_data->aname))==0)
                break;
        }
        
        if (j<XCVR_ATTR_MAX)
            xcvr_attributes[i] = &xcvr_attr_list[j]->dev_attr.attr;

    }
    xcvr_attributes[i] = NULL;

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &xcvr_group);
    if (status) {
        goto exit_free;
    }

    data->xdev = hwmon_device_register_with_groups(&client->dev, client->name, NULL, NULL);
    if (IS_ERR(data->xdev)) {
        status = PTR_ERR(data->xdev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: xcvr '%s'\n",
         dev_name(data->xdev), client->name);
    
    /* Add a support for post probe function */
    if (pddf_xcvr_ops.post_probe)
    {
        dev_id = i2c_match_id(xcvr_ids, client);
        status = (pddf_xcvr_ops.post_probe)(client, dev_id);
        if (status != 0)
            goto exit_remove;
    }


    return 0;


exit_remove:
    sysfs_remove_group(&client->dev.kobj, &xcvr_group);
exit_free:
    kfree(data);
exit:
    
    return status;
}

static void xcvr_remove(struct i2c_client *client)
{
    int ret = 0;
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *platdata = (XCVR_PDATA *)client->dev.platform_data;
    XCVR_ATTR *platdata_sub = platdata->xcvr_attrs;

    if (pddf_xcvr_ops.pre_remove)
    {
        ret = (pddf_xcvr_ops.pre_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "FAN pre_remove function failed\n");
    }

    hwmon_device_unregister(data->xdev);
    sysfs_remove_group(&client->dev.kobj, &xcvr_group);
    kfree(data);

    if (platdata_sub) {
        pddf_dbg(XCVR, KERN_DEBUG "%s: Freeing platform subdata\n", __FUNCTION__);
        kfree(platdata_sub);
    }
    if (platdata) {
        pddf_dbg(XCVR, KERN_DEBUG "%s: Freeing platform data\n", __FUNCTION__);
        kfree(platdata);
    }
    
    if (pddf_xcvr_ops.post_remove)
    {
        ret = (pddf_xcvr_ops.post_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "FAN post_remove function failed\n");
    }
}

static struct i2c_driver xcvr_driver = {
    /*.class        = I2C_CLASS_HWMON,*/
    .driver = {
        .name     = "xcvr",
        .owner    = THIS_MODULE,
    },
    .probe        = xcvr_probe,
    .remove       = xcvr_remove,
    .id_table     = xcvr_ids,
};


/*int __init xcvr_init(void)*/
int xcvr_init(void)
{
    int ret = 0;

    if (pddf_xcvr_ops.pre_init)
    {
        ret = (pddf_xcvr_ops.pre_init)();
        if (ret!=0)
            return ret;
    }

    pddf_dbg(XCVR, KERN_ERR "PDDF XCVR DRIVER.. init Invoked..\n");
    ret = i2c_add_driver(&xcvr_driver);
    if (ret!=0)
        return ret;

    if (pddf_xcvr_ops.post_init)
    {
        ret = (pddf_xcvr_ops.post_init)();
        if (ret!=0)
            return ret;
    }
    
    return ret;
}
EXPORT_SYMBOL(xcvr_init);

void xcvr_exit(void)
{
    pddf_dbg(XCVR, "PDDF XCVR DRIVER.. exit\n");
    if (pddf_xcvr_ops.pre_exit) (pddf_xcvr_ops.pre_exit)();
    i2c_del_driver(&xcvr_driver);
    if (pddf_xcvr_ops.post_exit) (pddf_xcvr_ops.post_exit)();

}
EXPORT_SYMBOL(xcvr_exit);

module_init(xcvr_init);
module_exit(xcvr_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Driver for transceiver operations");
MODULE_LICENSE("GPL");
