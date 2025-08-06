/*
 * Copyright 2025 Nexthop Systems Inc.
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
 *	Platform MULTIFPGAPCI defines/structures header file
 */

#include <asm-generic/errno-base.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/string.h>

#include "pddf_multifpgapci_defs.h"
#include "pddf_multifpgapci_i2c_defs.h"

int (*pddf_i2c_multifpgapci_add_numbered_bus)(struct i2c_adapter *, int) = NULL;
EXPORT_SYMBOL(pddf_i2c_multifpgapci_add_numbered_bus);

ssize_t new_i2c_adapter(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	int index, error;

	error = kstrtoint(buf, 10, &index);
	if (error != 0) {
		pddf_dbg(MULTIFPGA, KERN_ERR "Error converting string: %d\n",
			 error);
		return -EINVAL;
	}

	if (index < 0 || index >= I2C_PCI_MAX_BUS) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] I2C Adapter %d out of range [0, %d)\n",
			 __FUNCTION__, index, I2C_PCI_MAX_BUS);
		return -ENODEV;
	}

	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct pddf_multifpgapci_drvdata *pci_drvdata =
		pci_get_drvdata(pci_dev);
	struct i2c_adapter_drvdata *i2c_privdata =
		&pci_drvdata->i2c_adapter_drvdata;
	struct i2c_adapter *i2c_adapters = i2c_privdata->i2c_adapters;

	if (i2c_privdata->i2c_adapter_registered[index]) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] I2C Adapter %d already exists\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	i2c_adapters[index].owner = THIS_MODULE;
	i2c_adapters[index].class = I2C_CLASS_HWMON | I2C_CLASS_SPD;

	// /dev/i2c-xxx for FPGA LOGIC I2C channel controller 1-7
	i2c_adapters[index].nr = index + i2c_privdata->virt_bus;
	sprintf(i2c_adapters[index].name, "i2c-pci-%d",
		index + i2c_privdata->virt_bus);

	// Set up the sysfs linkage to our parent device
	i2c_adapters[index].dev.parent = &pci_dev->dev;

	if (pddf_i2c_multifpgapci_add_numbered_bus == NULL) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "PDDF_I2C ERROR %s: MULTIFPGAPCIE add numbered bus "
			 "failed because fpga custom algo module is not loaded",
			 __func__);
		return -ENODEV;
	}
	if ((pddf_i2c_multifpgapci_add_numbered_bus(&i2c_adapters[index],
						    index) != 0)) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "Cannot add bus %d to algorithm layer\n",
			 index);
		return -ENODEV;
	}
	i2c_privdata->i2c_adapter_registered[index] = true;
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] Registered bus id: %s\n",
		 __FUNCTION__, kobject_name(&i2c_adapters[index].dev.kobj));

	return count;
}

ssize_t del_i2c_adapter(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	int index, error;

	error = kstrtoint(buf, 10, &index);
	if (error != 0) {
		pddf_dbg(MULTIFPGA, KERN_ERR "Error converting string: %d\n",
			 error);
		return -EINVAL;
	}

	if (index < 0 || index >= I2C_PCI_MAX_BUS) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] I2C Adapter %d out of range [0, %d)\n",
			 __FUNCTION__, index, I2C_PCI_MAX_BUS);
		return -ENODEV;
	}

	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct i2c_adapter_drvdata *i2c_privdata =
		(struct i2c_adapter_drvdata *)_ptr->addr;
	struct i2c_adapter *i2c_adapters = i2c_privdata->i2c_adapters;

	if (!i2c_privdata->i2c_adapter_registered[index]) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s: I2C Adapter %d doesn't exist\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	pddf_dbg(MULTIFPGA,
		 KERN_INFO "[%s] Attempting delete of bus index: %d\n",
		 __FUNCTION__, index);

	i2c_del_adapter(&i2c_adapters[index]);

	i2c_privdata->i2c_adapter_registered[index] = false;

	return count;
}

int pddf_multifpgapci_i2c_module_init(struct pci_dev *pci_dev,
				      struct kobject *kobj)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct pddf_multifpgapci_drvdata *pci_drvdata =
		pci_get_drvdata(pci_dev);
	struct i2c_adapter_drvdata *i2c_privdata =
		&pci_drvdata->i2c_adapter_drvdata;
	int err;

	memset(i2c_privdata->i2c_adapter_registered, 0,
	       sizeof(i2c_privdata->i2c_adapter_registered));

	PDDF_DATA_ATTR(
		virt_bus, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_UINT32, sizeof(uint32_t),
		(void *)&i2c_privdata->temp_sysfs_vals.virt_bus, NULL);
	PDDF_DATA_ATTR(
		ch_base_offset, S_IWUSR | S_IRUGO, show_pddf_data,
		store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		(void *)&i2c_privdata->temp_sysfs_vals.ch_base_offset, NULL);
	PDDF_DATA_ATTR(
		ch_size, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_UINT32, sizeof(uint32_t),
		(void *)&i2c_privdata->temp_sysfs_vals.ch_size, NULL);
	PDDF_DATA_ATTR(
		num_virt_ch, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_UINT32, sizeof(uint32_t),
		(void *)&i2c_privdata->temp_sysfs_vals.num_virt_ch, NULL);
	PDDF_DATA_ATTR(
		new_i2c_adapter, S_IWUSR | S_IRUGO, show_pddf_data,
		new_i2c_adapter, PDDF_CHAR, NAME_SIZE, (void *)pci_dev, NULL);
	PDDF_DATA_ATTR(
		del_i2c_adapter, S_IWUSR | S_IRUGO, show_pddf_data,
		del_i2c_adapter, PDDF_CHAR, NAME_SIZE, (void *)i2c_privdata, NULL);
	
	i2c_privdata->attrs.attr_virt_bus = attr_virt_bus;
	i2c_privdata->attrs.attr_ch_base_offset = attr_ch_base_offset;
	i2c_privdata->attrs.attr_ch_size = attr_ch_size;
	i2c_privdata->attrs.attr_num_virt_ch = attr_num_virt_ch;
	i2c_privdata->attrs.attr_new_i2c_adapter = attr_new_i2c_adapter;
	i2c_privdata->attrs.attr_del_i2c_adapter = attr_del_i2c_adapter;

	// All the attributes are added in above step are put into here to be
	// put in sysfs group
	struct attribute *i2c_adapter_attrs[NUM_I2C_ADAPTER_ATTRS + 1] = {
		&i2c_privdata->attrs.attr_virt_bus.dev_attr.attr,
		&i2c_privdata->attrs.attr_ch_base_offset.dev_attr.attr,
		&i2c_privdata->attrs.attr_ch_size.dev_attr.attr,
		&i2c_privdata->attrs.attr_num_virt_ch.dev_attr.attr,
		&i2c_privdata->attrs.attr_new_i2c_adapter.dev_attr.attr,
		&i2c_privdata->attrs.attr_del_i2c_adapter.dev_attr.attr,
		NULL,
	};

	memcpy(i2c_privdata->i2c_adapter_attrs, i2c_adapter_attrs,
	       sizeof(i2c_privdata->i2c_adapter_attrs));

	i2c_privdata->i2c_adapter_attr_group.attrs =
		i2c_privdata->i2c_adapter_attrs;

	err = sysfs_create_group(kobj, &i2c_privdata->i2c_adapter_attr_group);
	if (err) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] sysfs_create_group error, status: %d\n",
			 __FUNCTION__, err);
		return err;
	}

	return 0;
}
EXPORT_SYMBOL(pddf_multifpgapci_i2c_module_init);

void pddf_multifpgapci_i2c_module_exit(struct pci_dev *pci_dev,
				       struct kobject *kobj)
{
	struct pddf_multifpgapci_drvdata *pci_drvdata =
		pci_get_drvdata(pci_dev);
	struct i2c_adapter_drvdata *i2c_privdata =
		&pci_drvdata->i2c_adapter_drvdata;
	int i;
	for (i = 0; i < I2C_PCI_MAX_BUS; i++) {
		if (i2c_privdata->i2c_adapter_registered[i]) {
			pddf_dbg(MULTIFPGA,
				 KERN_INFO "[%s] deleting i2c adapter: %s\n",
				 __FUNCTION__,
				 i2c_privdata->i2c_adapters[i].name);
			i2c_del_adapter(&i2c_privdata->i2c_adapters[i]);
		}
	}
	sysfs_remove_group(kobj, &i2c_privdata->i2c_adapter_attr_group);
};
EXPORT_SYMBOL(pddf_multifpgapci_i2c_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION("PDDF Platform Data for Multiple PCI FPGA I2C adapters.");