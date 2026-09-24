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
 * A PDDF kernel module to create sysfs for multiple PCI FPGAs.
 * Borrows from the PDDF fpgapci module by Broadcom.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/sysfs.h>

#include "pddf_client_defs.h"
#include "pddf_multifpgapci_defs.h"

#define MAX_PCI_IDS 16

static struct kobject *multifpgapci_kobj = NULL;
struct pci_device_id fpgapci_ids[MAX_PCI_IDS + 1];
static int num_pci_ids = 0;

extern int pddf_multifpgapci_register(const struct pci_device_id *ids,
				      struct kobject *kobj);

static ssize_t dev_operation(struct device *dev, struct device_attribute *da,
			     const char *buf, size_t count);
static ssize_t register_pci_device_id(struct device *dev,
				      struct device_attribute *da,
				      const char *buf, size_t count);

PDDF_DATA_ATTR(dev_ops, S_IWUSR | S_IRUGO, show_pddf_data, dev_operation,
	       PDDF_CHAR, NAME_SIZE, NULL, NULL);
PDDF_DATA_ATTR(register_pci_device_id, S_IWUSR | S_IRUGO, show_pddf_data,
	       register_pci_device_id, PDDF_CHAR, NAME_SIZE, NULL, NULL);

struct attribute *attrs_multifpgapci[] = {
	&attr_dev_ops.dev_attr.attr,
	&attr_register_pci_device_id.dev_attr.attr,
	NULL,
};

struct attribute_group attr_group_multifpgapci = {
	.attrs = attrs_multifpgapci,
};

ssize_t dev_operation(struct device *dev, struct device_attribute *da,
		      const char *buf, size_t count)
{
	if (strncmp(buf, "multifpgapci_init", strlen("multifpgapci_init")) ==
	    0) {
		if (num_pci_ids > 0) {
			pddf_multifpgapci_register(fpgapci_ids,
						   multifpgapci_kobj);
		} else {
			pddf_dbg(
				MULTIFPGA,
				KERN_ERR
				"PDDF_ERROR %s: No FPGA PCI IDs are registered yet\n",
				__FUNCTION__);
			return -EINVAL;
		}
	} else {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "PDDF_ERROR %s: Invalid value for dev_ops %s\n",
			 __FUNCTION__, buf);
		return -EINVAL;
	}

	return count;
}

int add_fpgapci_id(unsigned short vendor, unsigned short device)
{
	if (num_pci_ids < MAX_PCI_IDS) {
		fpgapci_ids[num_pci_ids].vendor = vendor;
		fpgapci_ids[num_pci_ids].device = device;
		fpgapci_ids[num_pci_ids].subvendor = PCI_ANY_ID;
		fpgapci_ids[num_pci_ids].subdevice = PCI_ANY_ID;
		fpgapci_ids[num_pci_ids].class = PCI_ANY_ID;
		fpgapci_ids[num_pci_ids].class_mask = 0;
		fpgapci_ids[num_pci_ids].driver_data = 0;
		num_pci_ids++;
		// Null-terminate the array
		fpgapci_ids[num_pci_ids] = (struct pci_device_id){ 0 };
		pddf_dbg(MULTIFPGA,
			 KERN_INFO
			 "%s Registered vendor: 0x%04x, device: 0x%04x\n",
			 __FUNCTION__, vendor, device);
		return 0;
	} else {
		pddf_dbg(
			MULTIFPGA,
			KERN_ERR
			"PDDF_ERROR %s: Maximum number of FPGA PCI IDs reached\n",
			__FUNCTION__);
		return -1;
	}
}

ssize_t register_pci_device_id(struct device *dev, struct device_attribute *da,
			       const char *buf, size_t count)
{
	unsigned int vendor, device;
	int sscanf_result;

	// We expect the vendor and device ID in hex format
	// Try parsing with "0x" prefix first
	sscanf_result = sscanf(buf, "0x%x 0x%x", &vendor, &device);
	if (sscanf_result == 2) {
		if (add_fpgapci_id((unsigned short)vendor,
				   (unsigned short)device) == 0) {
			return count;
		}
		return -ENOSPC;
	}

	// Try parsing without "0x" prefix
	sscanf_result = sscanf(buf, "%x %x", &vendor, &device);
	if (sscanf_result == 2) {
		if (add_fpgapci_id((unsigned short)vendor,
				   (unsigned short)device) == 0) {
			return count;
		}
		return -ENOSPC;
	}

	pddf_dbg(MULTIFPGA,
		 KERN_ERR
		 "%s Failed to register pci device ids, unexpected format\n",
		 __FUNCTION__);
	return -EINVAL;
}

int __init pddf_multifpgapci_module_init(void)
{
	int ret = 0;
	struct kobject *device_kobj;

	pddf_dbg(MULTIFPGA, KERN_INFO "%s ..\n", __FUNCTION__);

	device_kobj = get_device_i2c_kobj();
	if (!device_kobj) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s get_device_i2c_kobj failed ..\n",
			 __FUNCTION__);
		return -ENOMEM;
	}

	multifpgapci_kobj = kobject_create_and_add("multifpgapci", device_kobj);
	if (!multifpgapci_kobj) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s create multifpgapci kobj failed ..\n",
			 __FUNCTION__);
		return -ENOMEM;
	}

	ret = sysfs_create_group(multifpgapci_kobj, &attr_group_multifpgapci);
	if (ret) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "%s create multifpgapci sysfs attributes failed ..\n",
			 __FUNCTION__);
		return ret;
	}

	return 0;
}

void __exit pddf_multifpgapci_module_exit(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "%s ..\n", __FUNCTION__);
	KOBJ_FREE(multifpgapci_kobj)
	return;
}

module_init(pddf_multifpgapci_module_init);
module_exit(pddf_multifpgapci_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION("PDDF module for systems with multiple PCI FPGAs");
