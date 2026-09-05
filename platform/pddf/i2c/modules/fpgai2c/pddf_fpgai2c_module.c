/*
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
 * Description:
 *   A pddf kernel module to create I2C client for an I2CFPGA
 */

#define __STDC_WANT_LIB_EXT1__ 1
#include <linux/string.h>
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
#include "pddf_fpgai2c_defs.h"

PDDF_FPGAI2C_DATA pddf_fpgai2c_data={0};
EXPORT_SYMBOL(pddf_fpgai2c_data);

static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t store_pddf_fpgai2c_data(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t show_pddf_fpgai2c_data(struct device *dev, struct device_attribute *da, char *buf);

extern void *get_device_table(char *name);
extern void delete_device_table(char *name);


/* MUX CLIENT DATA */
PDDF_DATA_ATTR(dev_ops, S_IWUSR, NULL, do_device_operation, PDDF_CHAR, 8, NULL, (void*)&pddf_data);
PDDF_DATA_ATTR(reg_addr, S_IWUSR|S_IRUGO, show_pddf_fpgai2c_data, store_pddf_fpgai2c_data, PDDF_USHORT, sizeof(unsigned short), (void*)&pddf_fpgai2c_data.reg_addr, NULL);



static struct attribute *fpgai2c_attributes[] = {
	&attr_dev_ops.dev_attr.attr,
	&attr_reg_addr.dev_attr.attr,
	NULL
};

static const struct attribute_group pddf_fpgai2c_client_data_group = {
	.attrs = fpgai2c_attributes,
};


static ssize_t store_pddf_fpgai2c_data(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int ret = 0;
    int num = 0;
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;

    ret = kstrtoint(buf,16,&num);
    if (ret==0)
    {
        mutex_lock(&pddf_fpgai2c_data.fpga_lock);
        *(unsigned short *)(ptr->addr) = (unsigned short)num;
        mutex_unlock(&pddf_fpgai2c_data.fpga_lock);
        pddf_dbg(FPGAI2C, KERN_ERR "Stored value: 0x%x, num: 0x%x\n", *(int*)(ptr->addr), num);
    }

    return count;
}

ssize_t show_pddf_fpgai2c_data(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret = 0;
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    pddf_dbg(FPGAI2C, KERN_ERR "[ READ ] DATA ATTR PTR TYPE:%d, ADDR=%p\n", ptr->type, ptr->addr);

    mutex_lock(&pddf_fpgai2c_data.fpga_lock);
    ret = sprintf(buf, "0x%x\n", *(unsigned short *)(ptr->addr));
    mutex_unlock(&pddf_fpgai2c_data.fpga_lock);

    return ret;
}

static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	PDDF_ATTR *ptr = (PDDF_ATTR *)da;
	NEW_DEV_ATTR *device_ptr = (NEW_DEV_ATTR *)(ptr->data);
	struct i2c_adapter *adapter;
	static struct i2c_board_info board_info;
	struct i2c_client *client_ptr;

	if (strncmp(buf, "add", strlen(buf)-1)==0)
	{
		adapter = i2c_get_adapter(device_ptr->parent_bus);

		if (strncmp(device_ptr->dev_type, "i2c_fpga", strlen("i2c_fpga"))==0)
		{
			board_info = (struct i2c_board_info) {
				.platform_data = (void *)NULL,
			};

			board_info.addr = device_ptr->dev_addr;
			strcpy(board_info.type, device_ptr->dev_type);

			client_ptr = i2c_new_client_device(adapter, &board_info);

			if (!IS_ERR(client_ptr)) {
				i2c_put_adapter(adapter);
				pddf_dbg(FPGAI2C, KERN_ERR "Created %s client: 0x%p\n", device_ptr->i2c_name, (void *)client_ptr);
				add_device_table(device_ptr->i2c_name, (void*)client_ptr);
			}
			else {
				i2c_put_adapter(adapter);
				goto free_data;
			}

		}
		else
		{
			printk(KERN_ERR "%s: Unsupported type of fpga device id - unable to add i2c client\n", __FUNCTION__);
		}
	}
	else if (strncmp(buf, "delete", strlen(buf)-1)==0)
	{
		/*Get the i2c_client handle for the created client*/
		client_ptr = (struct i2c_client *)get_device_table(device_ptr->i2c_name);
		if (client_ptr)
		{
			pddf_dbg(FPGAI2C, KERN_ERR "Removing %s client: 0x%p\n", device_ptr->i2c_name, (void *)client_ptr);
			i2c_unregister_device(client_ptr);
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
	/*TODO: free the device_ptr->data is dynamically allocated*/
#ifdef __STDC_LIB_EXT1__
	memset_s(device_ptr, sizeof(NEW_DEV_ATTR), 0 , sizeof(NEW_DEV_ATTR));
#else
	memset(device_ptr, 0 , sizeof(NEW_DEV_ATTR));
#endif

	return count;
}


static struct kobject *fpgai2c_kobj;

int __init fpgai2c_data_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;


    pddf_dbg(FPGAI2C, "FPGAI2C_DATA MODULE.. init\n");

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj)
        return -ENOMEM;

    fpgai2c_kobj = kobject_create_and_add("fpgai2c", device_kobj);
    if(!fpgai2c_kobj)
        return -ENOMEM;


    ret = sysfs_create_group(fpgai2c_kobj, &pddf_clients_data_group);
    if (ret)
    {
        kobject_put(fpgai2c_kobj);
        return ret;
    }
    pddf_dbg(FPGAI2C, "CREATED PDDF I2C CLIENTS CREATION SYSFS GROUP\n");

    mutex_init(&pddf_fpgai2c_data.fpga_lock);

    ret = sysfs_create_group(fpgai2c_kobj, &pddf_fpgai2c_client_data_group);
    if (ret)
    {
        sysfs_remove_group(fpgai2c_kobj, &pddf_clients_data_group);
        kobject_put(fpgai2c_kobj);
        return ret;
    }
    pddf_dbg(FPGAI2C, "CREATED PDDF I2C CLIENTS CREATION SYSFS GROUP\n");
    return ret;
}

void __exit fpgai2c_data_exit(void)
{
	pddf_dbg(FPGAI2C, "FPGAI2C_DATA MODULE.. exit\n");
	sysfs_remove_group(fpgai2c_kobj, &pddf_fpgai2c_client_data_group);
	sysfs_remove_group(fpgai2c_kobj, &pddf_clients_data_group);
    kobject_put(fpgai2c_kobj);
    pddf_dbg(FPGAI2C, KERN_ERR "%s: Removed the kobjects for 'fpgai2c'\n",__FUNCTION__);
    return;
}

module_init(fpgai2c_data_init);
module_exit(fpgai2c_data_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("fpgai2c platform data");
MODULE_LICENSE("GPL");
