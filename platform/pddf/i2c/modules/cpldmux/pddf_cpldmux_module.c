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
 * PDDF generic module for cpldmux device
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
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include "pddf_client_defs.h"
#include "pddf_cpldmux_defs.h"

PDDF_CPLDMUX_DATA pddf_cpldmux_data={0};
PDDF_CPLDMUX_CHAN_DATA pddf_cpldmux_chan_data={0};
EXPORT_SYMBOL(pddf_cpldmux_data);

static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t do_chan_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

extern void *get_device_table(char *name);
extern void delete_device_table(char *name);


/* CPLDMUX CLIENT DATA */
PDDF_DATA_ATTR(dev_ops, S_IWUSR, NULL, do_device_operation, PDDF_CHAR, 8, (void*)&pddf_cpldmux_data, (void*)&pddf_data);
PDDF_DATA_ATTR(chan_ops, S_IWUSR, NULL, do_chan_operation, PDDF_CHAR, 8, (void*)&pddf_cpldmux_data, NULL);
PDDF_DATA_ATTR(base_chan, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&pddf_cpldmux_data.base_chan, NULL);
PDDF_DATA_ATTR(num_chan, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&pddf_cpldmux_data.num_chan, NULL);
PDDF_DATA_ATTR(chan_cache, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&pddf_cpldmux_data.chan_cache, NULL);
PDDF_DATA_ATTR(cpld_name, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, (void*)&pddf_cpldmux_data.cpld_name, NULL);
PDDF_DATA_ATTR(chan, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&pddf_cpldmux_chan_data.chan_num, NULL);
PDDF_DATA_ATTR(dev, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 128, (void*)&pddf_cpldmux_chan_data.chan_device, NULL);
PDDF_DATA_ATTR(cpld_devaddr, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&pddf_cpldmux_chan_data.cpld_devaddr, NULL);
PDDF_DATA_ATTR(cpld_offset, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&pddf_cpldmux_chan_data.cpld_offset, NULL);
PDDF_DATA_ATTR(cpld_sel, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&pddf_cpldmux_chan_data.cpld_sel, NULL);
PDDF_DATA_ATTR(cpld_desel, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&pddf_cpldmux_chan_data.cpld_desel, NULL);

static struct attribute *cpldmux_attributes[] = {
	&attr_dev_ops.dev_attr.attr,
	&attr_chan_ops.dev_attr.attr,
	&attr_base_chan.dev_attr.attr,
    &attr_num_chan.dev_attr.attr,
    &attr_chan_cache.dev_attr.attr,
    &attr_cpld_name.dev_attr.attr,
    &attr_chan.dev_attr.attr,
    &attr_dev.dev_attr.attr,
    &attr_cpld_devaddr.dev_attr.attr,
    &attr_cpld_offset.dev_attr.attr,
    &attr_cpld_sel.dev_attr.attr,
    &attr_cpld_desel.dev_attr.attr,
	NULL
};

static const struct attribute_group pddf_cpldmux_client_data_group = {
	.attrs = cpldmux_attributes,
};



static ssize_t do_chan_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    PDDF_CPLDMUX_DATA *cpldmux_data = (PDDF_CPLDMUX_DATA *)(ptr->addr);
    int index;

    pddf_dbg(CPLDMUX, KERN_ERR "%s: Adding channel %d\n", __FUNCTION__, pddf_cpldmux_chan_data.chan_num);
    index = pddf_cpldmux_chan_data.chan_num;
    cpldmux_data->chan_data[index] = pddf_cpldmux_chan_data;

    memset(&pddf_cpldmux_chan_data, 0, sizeof(pddf_cpldmux_chan_data));


    return count;
}

static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	PDDF_ATTR *ptr = (PDDF_ATTR *)da;
	NEW_DEV_ATTR *device_ptr = (NEW_DEV_ATTR *)(ptr->data);
    PDDF_CPLDMUX_DATA *cpldmux_data = (PDDF_CPLDMUX_DATA *)(ptr->addr);
    PDDF_CPLDMUX_PDATA *cpldmux_platform_data = NULL;
    struct platform_device *plat_dev = NULL;
    struct i2c_client *client_ptr = NULL;
    struct pci_dev *pci_dev = NULL;
    int ret=0, i=0;

	if (strncmp(buf, "add", strlen(buf)-1)==0)
	{
		/* Allocate the cpldmux_platform_data */
		cpldmux_platform_data = (PDDF_CPLDMUX_PDATA *)kzalloc(
			sizeof(PDDF_CPLDMUX_PDATA) +
				cpldmux_data->num_chan *
					sizeof(PDDF_CPLDMUX_CHAN_DATA),
			GFP_KERNEL);
		if (!cpldmux_platform_data) {
			printk("%s(%d): kzalloc failure.\n", __func__,
			       __LINE__);
			goto clear_data;
		}
		cpldmux_platform_data->chan_data =
			(PDDF_CPLDMUX_CHAN_DATA *)(cpldmux_platform_data + 1);

		cpldmux_platform_data->parent_bus = device_ptr->parent_bus;
		cpldmux_platform_data->base_chan = cpldmux_data->base_chan;
		cpldmux_platform_data->num_chan = cpldmux_data->num_chan;
		cpldmux_platform_data->chan_cache = cpldmux_data->chan_cache;
		for (i = 0; i < cpldmux_data->num_chan; i++) {
			cpldmux_platform_data->chan_data[i] =
				cpldmux_data->chan_data[i];
		}

		plat_dev = platform_device_alloc(device_ptr->dev_type,
						 device_ptr->dev_id);
		if (!plat_dev) {
			printk("%s(%d): platform_device_alloc failure.\n",
			       __func__, __LINE__);
			goto free_data;
		}

		plat_dev->dev.platform_data = cpldmux_platform_data;
		if (strncmp(device_ptr->dev_type, "cpld_mux",
			    strlen("cpld_mux")) == 0) {
			cpldmux_platform_data->dev_type = CPLD_MUX;
			/*Get the i2c_client handle for the CPLD which drives this cpldmux*/
			client_ptr = (struct i2c_client *)get_device_table(
				cpldmux_data->cpld_name);
			if (client_ptr == NULL) {
				pddf_dbg(
					CPLDMUX,
					KERN_ERR
					"Unable to get the CPLD client %s for %s cpldmux\n",
					cpldmux_data->cpld_name,
					device_ptr->i2c_name);
				printk(KERN_ERR
				       "Unable to get the CPLD client %s for %s cpldmux\n",
				       cpldmux_data->cpld_name,
				       device_ptr->i2c_name);
				goto free_data;
			}
			cpldmux_platform_data->cpld = client_ptr;
		} else if (strncmp(device_ptr->dev_type, "multifpgapci_mux",
				   strlen("multifpgapci_mux")) == 0) {
			cpldmux_platform_data->dev_type = MULTIFPGAPCI_MUX;
			pci_dev = (struct pci_dev *)get_device_table(
				cpldmux_data->cpld_name
			);
			if (pci_dev == NULL) {
				printk(KERN_ERR
				       "PDDF_CPLDMUX: Unable to get pci_dev of %s for %s\n",
				       cpldmux_data->cpld_name,
				       device_ptr->i2c_name);
				goto free_data;
			}
			cpldmux_platform_data->fpga_pci_dev = pci_dev_get(pci_dev);
		} else {
			printk(KERN_ERR
			       "%s: Unsupported type of cpldmux - unable to add i2c client\n",
			       __FUNCTION__);
			goto free_data;
		}
		pddf_dbg(
			CPLDMUX,
			KERN_ERR
			"Creating a %s platform_device 0x%p, platform_data 0x%p\n",
			plat_dev->name, (void *)plat_dev,
			(void *)cpldmux_platform_data);
		ret = platform_device_add(plat_dev);
		if (ret) {
			pddf_dbg(
				CPLDMUX,
				KERN_ERR
				"Unable to create cpld_mux (%s) device: Error %d\n",
				device_ptr->i2c_name, ret);
			goto free_data;
		} else {
			add_device_table(device_ptr->i2c_name,
					 (void *)plat_dev);
		}
	}
	else if (strncmp(buf, "delete", strlen(buf)-1)==0)
	{
		/*Get the i2c_client handle for the created client*/
		plat_dev = (struct platform_device *)get_device_table(device_ptr->i2c_name);
		if (plat_dev)
		{
			pddf_dbg(CPLDMUX, KERN_ERR "Removing %s device: 0x%p\n", device_ptr->i2c_name, (void *)plat_dev);
			pddf_dbg(CPLDMUX, KERN_ERR "Freeing the memory held by device: 0x%p\n", (void *)plat_dev);
			pci_dev_put(((PDDF_CPLDMUX_PDATA *)plat_dev->dev.platform_data)->fpga_pci_dev);
			platform_device_del(plat_dev);
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
    goto clear_data;

free_data:
    /* Free the allocated memory for platform and channel data */
    cpldmux_platform_data = plat_dev->dev.platform_data;
    if (cpldmux_platform_data)
    {
        printk(KERN_ERR "%s: Unable to register a cpldmux device. Freeing the platform data\n", __FUNCTION__);
        kfree(cpldmux_platform_data);
        plat_dev->dev.platform_data = NULL;
    }

    /* Put the platform device structure */
    platform_device_put(plat_dev);
clear_data:
    memset(cpldmux_data, 0, sizeof(PDDF_CPLDMUX_DATA));
    /*TODO: free the data device_ptr->data if data is dynamically allocated*/
    memset(device_ptr, 0, sizeof(NEW_DEV_ATTR));
    return count;
}


static struct kobject *cpldmux_kobj;

int __init cpldmux_data_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;


    pddf_dbg(CPLDMUX, "CPLDMUX_DATA MODULE.. init\n");

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj) 
        return -ENOMEM;

    cpldmux_kobj = kobject_create_and_add("cpldmux", device_kobj);
    if(!cpldmux_kobj) 
        return -ENOMEM;


    ret = sysfs_create_group(cpldmux_kobj, &pddf_clients_data_group);
    if (ret)
    {
        kobject_put(cpldmux_kobj);
        return ret;
    }
    pddf_dbg(CPLDMUX, "CREATED PDDF I2C CLIENTS CREATION SYSFS GROUP\n");


    ret = sysfs_create_group(cpldmux_kobj, &pddf_cpldmux_client_data_group);
    if (ret)
    {
        sysfs_remove_group(cpldmux_kobj, &pddf_clients_data_group);
        kobject_put(cpldmux_kobj);
        return ret;
    }
    pddf_dbg(CPLDMUX, "CREATED PDDF I2C CLIENTS CREATION SYSFS GROUP\n");
    return ret;
}

void __exit cpldmux_data_exit(void)
{
	pddf_dbg(CPLDMUX, "CPLDMUX_DATA MODULE.. exit\n");
	sysfs_remove_group(cpldmux_kobj, &pddf_cpldmux_client_data_group);
	sysfs_remove_group(cpldmux_kobj, &pddf_clients_data_group);
    kobject_put(cpldmux_kobj);
    pddf_dbg(CPLDMUX, KERN_ERR "%s: Removed the kobjects for 'cpldmux'\n",__FUNCTION__);
    return;
}

module_init(cpldmux_data_init);
module_exit(cpldmux_data_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("cpldmux platform data");
MODULE_LICENSE("GPL");
