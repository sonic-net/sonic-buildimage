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
 */

#ifndef __PDDF_MULTIFPGAPCI_I2C_DEFS_H__
#define __PDDF_MULTIFPGAPCI_I2C_DEFS_H__

#include <linux/i2c.h>
#include "linux/types.h"
#include <linux/kobject.h>
#include <linux/pci.h>
#include <linux/sysfs.h>

#include "pddf_client_defs.h"

#define I2C_PCI_MAX_BUS 512

struct i2c_adapter_attrs {
	PDDF_ATTR attr_virt_bus;
	PDDF_ATTR attr_ch_base_offset;
	PDDF_ATTR attr_ch_size;
	PDDF_ATTR attr_num_virt_ch;
	PDDF_ATTR attr_new_i2c_adapter;
	PDDF_ATTR attr_del_i2c_adapter;
};

#define NUM_I2C_ADAPTER_ATTRS \
	(sizeof(struct i2c_adapter_attrs) / sizeof(PDDF_ATTR))

struct i2c_adapter_sysfs_vals {
	uint32_t virt_bus;
	uint32_t ch_base_offset;
	uint32_t ch_size;
	uint32_t num_virt_ch;
};

struct i2c_adapter_data {
	int virt_bus;
	void *__iomem ch_base_addr;
	int ch_size;
	int num_virt_ch;
};

struct i2c_adapter_drvdata {
	struct pci_dev *pci_dev;
	size_t bar_length;
	struct kobject *i2c_kobj;

	// temp_sysfs_vals store temporary values provided by sysfs,
	// which are eventually copied/saved to I2C adapter platform data.
	struct i2c_adapter_sysfs_vals temp_sysfs_vals;

	// platform data
	struct i2c_adapter i2c_adapters[I2C_PCI_MAX_BUS];
	bool i2c_adapter_registered[I2C_PCI_MAX_BUS];
	int virt_bus;
	void *__iomem ch_base_addr;
	int ch_size;
	int num_virt_ch;

	// sysfs attrs
	struct i2c_adapter_attrs attrs;
	struct attribute *i2c_adapter_attrs[NUM_I2C_ADAPTER_ATTRS + 1];
	struct attribute_group i2c_adapter_attr_group;
};

extern int pddf_multifpgapci_i2c_module_init(struct pci_dev *pci_dev,
					     struct kobject *kobj);

// Only called if multifpgapci_i2c_module_init succeeded
extern void pddf_multifpgapci_i2c_module_exit(struct pci_dev *pci_dev,
					      struct kobject *kobj);

extern int
pddf_multifpgapci_i2c_get_adapter_data(struct pci_dev *pci_dev,
				       struct i2c_adapter_data *data);

#endif
