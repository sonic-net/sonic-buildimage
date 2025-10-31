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
 * A PDDF kernel driver for managing the creation of i2c adapters and
 * various IP blocks in a system with multiple PCI FPGAs.
 * Borrows from the PDDF fpgapci module by Broadcom.
 */

#include <asm-generic/errno-base.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmi.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irqdomain.h>
#include <linux/jiffies.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#include "pddf_client_defs.h"
#include "pddf_multifpgapci_defs.h"

#define BDF_NAME_SIZE 32
#define DEVICE_NAME_SIZE 32
#define DEBUG 0
#define DRIVER_NAME "pddf_multifpgapci"
#define MAX_PCI_NUM_BARS 6

extern void add_device_table(char *name, void *ptr);
extern void* get_device_table(char *name);
extern void delete_device_table(char *name);

static bool driver_registered = false;
struct pci_driver pddf_multifpgapci_driver;
static struct kobject *multifpgapci_kobj = NULL;

int default_multifpgapci_readpci(struct pci_dev *pci_dev, uint32_t offset,
		uint32_t *output);
int default_multifpgapci_writepci(struct pci_dev *pci_dev, uint32_t val,
				   uint32_t offset);

int (*ptr_multifpgapci_readpci)(struct pci_dev *,
				uint32_t, uint32_t *) = default_multifpgapci_readpci;
int (*ptr_multifpgapci_writepci)(struct pci_dev *, uint32_t,
				  uint32_t) = default_multifpgapci_writepci;
EXPORT_SYMBOL(ptr_multifpgapci_readpci);
EXPORT_SYMBOL(ptr_multifpgapci_writepci);

struct pddf_attrs {
	PDDF_ATTR attr_dev_ops;
};

#define NUM_FPGA_ATTRS \
	(sizeof(struct pddf_attrs) / sizeof(PDDF_ATTR))

struct pddf_multi_fpgapci_ops_t pddf_multi_fpgapci_ops;

EXPORT_SYMBOL(pddf_multi_fpgapci_ops);

struct fpga_data_node {
	char bdf[BDF_NAME_SIZE];
	char dev_name[DEVICE_NAME_SIZE]; // device_name as defined in pddf-device.json.
	struct kobject *kobj;
	struct pci_dev *dev;
	void __iomem *fpga_ctl_addr;
	unsigned long bar_start;
	struct list_head list;

	// sysfs attrs
	struct pddf_attrs attrs;
	struct attribute *fpga_attrs[NUM_FPGA_ATTRS + 1];
	struct attribute_group fpga_attr_group;
	bool fpga_attr_group_initialized;
	bool pddf_clients_data_group_initialized;
};

LIST_HEAD(fpga_list);

// PCI devices for a protocol
struct protocol_pci_entry {
	struct list_head list;
	struct pci_dev *pci_dev;
};

// Protocol module registered with the driver
struct protocol_module {
	struct list_head list;
	char name[32];
	struct protocol_ops *ops;
	struct list_head
		pci_devices; // List of PCI devices this protocol is initialized on
	struct mutex lock;
};

// Structure for collecting items needed to call into protocol modules without locks
struct fpga_work_item {
	struct list_head list;
	struct pci_dev *pci_dev;
	struct kobject *kobj;
	attach_fn attach;
	detach_fn detach;
	map_bar_fn map_bar;
	unmap_bar_fn unmap_bar;
	void __iomem *bar_base;
	unsigned long bar_start;
	unsigned long bar_len;
};

static LIST_HEAD(protocol_modules);
static DEFINE_MUTEX(protocol_modules_lock);
static int num_protocols = 0;

struct mutex fpga_list_lock;

static ssize_t dev_operation(struct device *dev, struct device_attribute *da,
			     const char *buf, size_t count);
static int map_bars(const char *bdf,
		    struct pddf_multifpgapci_drvdata *pci_privdata,
		    struct pci_dev *dev);
static void map_entire_bar(unsigned long barStart, unsigned long barLen,
			   struct pddf_multifpgapci_drvdata *pci_privdata,
			   struct fpga_data_node *fpga_data);
static int pddf_multifpgapci_probe(struct pci_dev *dev,
				   const struct pci_device_id *id);
static void pddf_multifpgapci_remove(struct pci_dev *dev);
static void free_bars(struct pddf_multifpgapci_drvdata *pci_privdata,
		      struct pci_dev *dev);

static int protocol_add_pci_dev(struct protocol_module *proto,
				struct pci_dev *pci_dev);
static int protocol_remove_pci_dev(struct protocol_module *proto,
				   struct pci_dev *pci_dev);
static void run_attach(struct pci_dev *pci_dev, struct kobject *kobj);
static void attach_protocols_for_fpga(struct pci_dev *pci_dev,
				      struct kobject *kobj);
static void run_detach(struct pci_dev *pci_dev, struct kobject *kobj);
static void detach_protocols_for_fpga(struct pci_dev *pci_dev,
				      struct kobject *kobj);
static void attach_protocols_for_all_fpgas(struct protocol_module *proto);
static void detach_protocols_for_all_fpgas(struct protocol_module *proto);
static void run_map_bar(struct pci_dev *pci_dev, void __iomem *bar_base,
			unsigned long bar_start, unsigned long bar_len);
static void run_bar_op_for_all_fpgas(struct protocol_module *proto, bool map);
static void run_unmap_bar(struct pci_dev *pci_dev, void __iomem *bar_base,
			  unsigned long bar_start, unsigned long bar_len);

int default_multifpgapci_readpci(struct pci_dev *pci_dev, uint32_t offset,
				 uint32_t *output)
{
	if (!pci_dev) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s pci_dev is NULL\n",
			 __FUNCTION__);
		return -ENODEV;
	}

	struct pddf_multifpgapci_drvdata *pci_drvdata =
		dev_get_drvdata(&pci_dev->dev);
	if (!pci_drvdata) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s pci_drvdata is NULL\n",
			 __FUNCTION__);
		return -ENODEV;
	}
	if (!pci_drvdata->bar_initialized) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s pci bar not initialized\n",
			 __FUNCTION__);
		return -ENODEV;
	}
	*output = ioread32(pci_drvdata->fpga_data_base_addr + offset);

	return 0;
}

int default_multifpgapci_writepci(struct pci_dev *pci_dev, uint32_t val,
				   uint32_t offset)
{
	if (!pci_dev) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s pci_dev is NULL\n",
			 __FUNCTION__);
		return -ENODEV;
	}

	struct pddf_multifpgapci_drvdata *pci_drvdata =
		dev_get_drvdata(&pci_dev->dev);
	if (!pci_drvdata) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s pci_drvdata is NULL\n",
			 __FUNCTION__);
		return -ENODEV;
	}
	if (!pci_drvdata->bar_initialized) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s pci bar not initialized\n",
			 __FUNCTION__);
		return -ENODEV;
	}
	iowrite32(val, pci_drvdata->fpga_data_base_addr + offset);

	return 0;
}

void free_pci_drvdata(struct pci_dev *pci_dev)
{
	pci_set_drvdata(pci_dev, NULL);
}

void free_sysfs_attr_groups(struct fpga_data_node *node)
{
	if (node->fpga_attr_group_initialized)
		sysfs_remove_group(node->kobj, &node->fpga_attr_group);

	if (node->pddf_clients_data_group_initialized)
		sysfs_remove_group(node->kobj, &pddf_clients_data_group);
}

void delete_fpga_data_node(const char *bdf)
{
	struct fpga_data_node *node, *tmp_node;
	struct fpga_data_node *found_node = NULL;

	// Find and remove from global list by holding the lock so that
	// all further cleanup can be performed without the need for a lock.
	mutex_lock(&fpga_list_lock);
	list_for_each_entry_safe(node, tmp_node, &fpga_list, list) {
		if (strcmp(node->bdf, bdf) == 0) {
			list_del(&node->list);
			found_node = node;
			break;
		}
	}
	mutex_unlock(&fpga_list_lock);

	if (!found_node)
		return;

	// Cleanup
	detach_protocols_for_fpga(found_node->dev, found_node->kobj);
	free_pci_drvdata(found_node->dev);
	free_sysfs_attr_groups(found_node);
	KOBJ_FREE(found_node->kobj);
	kfree(found_node);
}

void delete_all_fpga_data_nodes(void)
{
	struct fpga_data_node *node, *tmp;
	struct list_head local_list;

	// Clear the global list after copying over the pointer
	mutex_lock(&fpga_list_lock);
	local_list = fpga_list;
	INIT_LIST_HEAD(&fpga_list);
	mutex_unlock(&fpga_list_lock);

	// Work on the local copy without the need for a lock
	list_for_each_entry_safe(node, tmp, &local_list, list) {
		detach_protocols_for_fpga(node->dev, node->kobj);
		free_pci_drvdata(node->dev);
		free_sysfs_attr_groups(node);
		KOBJ_FREE(node->kobj);
		kfree(node);
	}
}

struct fpga_data_node *get_fpga_data_node(const char *bdf)
{
	struct fpga_data_node *node;
	struct fpga_data_node *found_node = NULL;

	mutex_lock(&fpga_list_lock);

	list_for_each_entry(node, &fpga_list, list) {
		if (strcmp(node->bdf, bdf) == 0) {
			found_node = node;
			break;
		}
	}

	mutex_unlock(&fpga_list_lock);

	return found_node;
}

void __iomem *get_fpga_ctl_addr_impl(const char *bdf)
{
	struct fpga_data_node *node = get_fpga_data_node(bdf);

	if (!node) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] No matching fpga data node\n",
			 __FUNCTION__);
		return NULL;
	}

	return node->fpga_ctl_addr;
}

void __iomem *(*get_fpga_ctl_addr)(const char *bdf) = get_fpga_ctl_addr_impl;

EXPORT_SYMBOL(get_fpga_ctl_addr);

static int pddf_pci_add_fpga(char *bdf, struct pci_dev *dev)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "%s ..\n", __FUNCTION__);
	int ret = 0;
	struct fpga_data_node *fpga_data =
		kzalloc(sizeof(*fpga_data), GFP_KERNEL);

	if (!fpga_data) {
		return -ENOMEM;
	}

	strscpy(fpga_data->bdf, bdf, NAME_SIZE);

	fpga_data->kobj = kobject_create_and_add(bdf, multifpgapci_kobj);
	if (!fpga_data->kobj) {
		ret = -ENOMEM;
		goto free_fpga_data;
	}

	fpga_data->dev = dev;

	PDDF_DATA_ATTR(
		dev_ops, S_IWUSR | S_IRUGO, NULL, dev_operation,
		PDDF_CHAR, NAME_SIZE, NULL, (void *)&pddf_data);

	// Setup sysfs attributes
	fpga_data->attrs.attr_dev_ops = attr_dev_ops;

	fpga_data->fpga_attrs[0] = &fpga_data->attrs.attr_dev_ops.dev_attr.attr;
	fpga_data->fpga_attrs[1] = NULL;
	fpga_data->fpga_attr_group.attrs = fpga_data->fpga_attrs;

	// Add to FPGA list
	mutex_lock(&fpga_list_lock);
	list_add(&fpga_data->list, &fpga_list);
	mutex_unlock(&fpga_list_lock);

	// Attach all registered protocols to this new FPGA
	attach_protocols_for_fpga(dev, fpga_data->kobj);

	ret = sysfs_create_group(fpga_data->kobj, &fpga_data->fpga_attr_group);
	if (ret) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] create pddf_clients_data_group failed: %d\n",
			 __FUNCTION__, ret);
		goto free_fpga_kobj;
	}
	fpga_data->fpga_attr_group_initialized = true;

	ret = sysfs_create_group(fpga_data->kobj, &pddf_clients_data_group);
	if (ret) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] sysfs_create_group failed: %d\n",
			 __FUNCTION__, ret);
		goto free_fpga_attr_group;
	}
	fpga_data->pddf_clients_data_group_initialized = true;

	return 0;

free_fpga_attr_group:
	sysfs_remove_group(fpga_data->kobj, &fpga_data->fpga_attr_group);
	fpga_data->fpga_attr_group_initialized = false;
free_fpga_kobj:
	attach_protocols_for_fpga(dev, fpga_data->kobj);
	kobject_put(fpga_data->kobj);
free_fpga_data:
	kfree(fpga_data);
	return ret;
}

ssize_t dev_operation(struct device *dev, struct device_attribute *da,
		      const char *buf, size_t count)
{
	PDDF_ATTR *ptr = (PDDF_ATTR *)da;
	NEW_DEV_ATTR *cdata = (NEW_DEV_ATTR *)(ptr->data);
	struct pci_dev *pci_dev = NULL;

	if (strncmp(buf, "fpgapci_init", strlen("fpgapci_init")) == 0) {
		pddf_dbg(MULTIFPGA, KERN_INFO "%s ..\n", __FUNCTION__);
		int err = 0;
		struct pddf_multifpgapci_drvdata *pci_privdata = 0;
		const char *bdf = dev->kobj.name;

		struct fpga_data_node *fpga_node = get_fpga_data_node(bdf);
		if (!fpga_node) {
			pddf_dbg(MULTIFPGA,
				 KERN_ERR "[%s] no matching fpga data node\n",
				 __FUNCTION__);
			return -ENODEV;
		}
		if (cdata->i2c_name[0] == 0) {
			pddf_dbg(MULTIFPGA,
				 KERN_ERR "[%s] no i2c_name specified\n",
				 __FUNCTION__);
			return -EINVAL;
		}

		pddf_dbg(MULTIFPGA, KERN_INFO "Initializing %s as %s\n", cdata->i2c_name, bdf);
		strscpy(fpga_node->dev_name, cdata->i2c_name, sizeof(fpga_node->dev_name));

		// Save pci_dev to hash table for clients to use.
		pci_dev = fpga_node->dev;
		add_device_table(fpga_node->dev_name, (void *)pci_dev_get(pci_dev));

		pci_privdata =
			(struct pddf_multifpgapci_drvdata *)dev_get_drvdata(
				&pci_dev->dev);

		if (map_bars(bdf, pci_privdata, pci_dev)) {
			pddf_dbg(MULTIFPGA, KERN_ERR "error_map_bars\n");
			pci_release_regions(pci_dev);
		}

		if (pddf_multi_fpgapci_ops.post_device_operation) {
			pddf_dbg(MULTIFPGA,
				 KERN_INFO
				 "[%s] Invoking post_device_operation\n",
				 __FUNCTION__);
			err = pddf_multi_fpgapci_ops.post_device_operation(
				pci_dev);
			if (err) {
				pddf_dbg(
					MULTIFPGA,
					KERN_ERR
					"[%s] post_device_operation failed with error %d\n",
					__FUNCTION__, err);
			}
		}
	} else if (strncmp(buf, "fpgapci_deinit", strlen("fpgapci_deinit")) == 0) {
		if (cdata->i2c_name[0] == 0) {
			pddf_dbg(MULTIFPGA,
				 KERN_ERR "[%s] no i2c_name specified\n",
				 __FUNCTION__);
			return -EINVAL;
		}

		pci_dev = (struct pci_dev *)get_device_table(cdata->i2c_name);
		if (pci_dev) {
			delete_device_table(cdata->i2c_name);
			pci_dev_put(pci_dev);
		}
	}

	return count;
}

static int pddf_pci_config_data(struct pci_dev *dev)
{
	unsigned short vendorId = 0xFFFF, deviceId = 0xFFFF;
	char revisionId = 0xFF, classDev = 0xFF, classProg = 0xFF;
	char irqLine = 0xFF, irqPin = 0xFF;

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] PCI Config Data\n", __FUNCTION__);

	// Accessing the configuration region of the PCI device
	pci_read_config_word(dev, PCI_VENDOR_ID, &vendorId);
	pci_read_config_word(dev, PCI_DEVICE_ID, &deviceId);
	pci_read_config_byte(dev, PCI_REVISION_ID, &revisionId);
	pci_read_config_byte(dev, PCI_CLASS_PROG, &classProg);
	pci_read_config_byte(dev, PCI_CLASS_DEVICE, &classDev);

	pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irqPin);
	if (pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irqLine)) {
		pddf_dbg(MULTIFPGA, KERN_ERR "\tPCI_INTERRUPT_LINE Error\n");
	}

	pddf_dbg(MULTIFPGA,
		 KERN_INFO
		 "\t[venId, devId]=[0x%x;0x%x] [group, class]=[%x;%x]\n",
		 vendorId, deviceId, classProg, classDev);
	pddf_dbg(MULTIFPGA,
		 KERN_INFO "\trevisionId=0x%x, irq_line=0x%x, irq_support=%s\n",
		 revisionId, irqLine, (irqPin == 0) ? "No" : "Yes");

	return 0;
}

static int map_bars(const char *bdf,
		    struct pddf_multifpgapci_drvdata *pci_privdata,
		    struct pci_dev *dev)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "%s - %s\n", __FUNCTION__, pci_name(dev));
	unsigned long barFlags, barStart, barEnd, barLen;
	int i;

	struct fpga_data_node *fpga_node = get_fpga_data_node(bdf);
	if (!fpga_node) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] No matching fpga data node\n",
			 __FUNCTION__);
		return -ENODEV;
	}

	int FPGAPCI_BAR_INDEX = -1;

	for (i = 0; i < MAX_PCI_NUM_BARS; i++) {
		if ((barLen = pci_resource_len(dev, i)) != 0 &&
		    (barStart = pci_resource_start(dev, i)) != 0) {
			barFlags = pci_resource_flags(dev, i);
			barEnd = pci_resource_end(dev, i);
			pddf_dbg(
				MULTIFPGA,
				KERN_INFO
				"[%s] PCI_BASE_ADDRESS_%d 0x%08lx-0x%08lx bar_len=0x%lx"
				" flags 0x%08lx IO_mapped=%s Mem_mapped=%s\n",
				__FUNCTION__, i, barStart, barEnd, barLen,
				barFlags,
				(barFlags & IORESOURCE_IO) ? "Yes" : "No",
				(barFlags & IORESOURCE_MEM) ? "Yes" : "No");
			FPGAPCI_BAR_INDEX = i;
			break;
		}
	}

	if (FPGAPCI_BAR_INDEX != -1) {
		map_entire_bar(barStart, barLen, pci_privdata, fpga_node);
		fpga_node->bar_start = barStart;
		pci_privdata->bar_start = barStart;
		pci_privdata->bar_initialized = true;
	} else {
		pddf_dbg(MULTIFPGA, KERN_INFO "[%s] Failed to find BAR\n",
			 __FUNCTION__);
		return -1;
	}

	pddf_dbg(MULTIFPGA,
		 KERN_INFO
		 "[%s] fpga_ctl_addr:0x%p fpga_data__base_addr:0x%p"
		 " bar_index[%d] fpgapci_bar_len:0x%08lx barStart: 0x%08lx\n",
		 __FUNCTION__, fpga_node->fpga_ctl_addr,
		 pci_privdata->fpga_data_base_addr, FPGAPCI_BAR_INDEX,
		 pci_privdata->bar_length, barStart);

	return 0;
}

static void map_entire_bar(unsigned long barStart, unsigned long barLen,
			   struct pddf_multifpgapci_drvdata *pci_privdata,
			   struct fpga_data_node *fpga_node)
{
	void __iomem *bar_base;
	pddf_dbg(MULTIFPGA, KERN_INFO "%s - %s\n", __FUNCTION__,
		 pci_name(fpga_node->dev));

	bar_base = ioremap_cache(barStart, barLen);

	pci_privdata->bar_length = barLen;
	pci_privdata->fpga_data_base_addr = bar_base;
	fpga_node->fpga_ctl_addr = pci_privdata->fpga_data_base_addr;

	// Notify all protocols about BAR mapping
	run_map_bar(fpga_node->dev, bar_base, barStart, barLen);
}

static void free_bars(struct pddf_multifpgapci_drvdata *pci_privdata,
		      struct pci_dev *dev)
{
	// Notify all protocols about BAR unmapping before freeing
	run_unmap_bar(dev, pci_privdata->fpga_data_base_addr,
		      pci_privdata->bar_start, pci_privdata->bar_length);

	pci_iounmap(dev, pci_privdata->fpga_data_base_addr);
	pci_privdata->fpga_data_base_addr = NULL;
}

static int pddf_multifpgapci_probe(struct pci_dev *dev,
				   const struct pci_device_id *id)
{
	struct pddf_multifpgapci_drvdata *pci_privdata = 0;
	int err = 0;
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s]\n", __FUNCTION__);

	// TODO: Use pci_name(dev) instead?
	unsigned int domain_number = pci_domain_nr(dev->bus);
	unsigned int bus_number = dev->bus->number;
	unsigned int device_number = PCI_SLOT(dev->devfn);
	unsigned int function_number = PCI_FUNC(dev->devfn);

	char bdf[BDF_NAME_SIZE];
	snprintf(bdf, BDF_NAME_SIZE, "%04x:%02x:%02x.%x", domain_number,
		 bus_number, device_number, function_number);
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] Probed FPGA with bdf: %s\n",
		 __FUNCTION__, bdf);

	if ((err = pci_enable_device(dev))) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] pci_enable_device failed. dev:%s err:%#x\n",
			 __FUNCTION__, pci_name(dev), err);
		goto error_enable_dev;
	}

	// Enable DMA
	pci_set_master(dev);

	// Request MMIO/IOP resources - reserve PCI I/O and memory resources
	// DRIVER_NAME shows up in /proc/iomem
	if ((err = pci_request_regions(dev, DRIVER_NAME)) < 0) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] pci_request_regions failed. dev:%s err:%#x\n",
			 __FUNCTION__, pci_name(dev), err);
		goto error_pci_req;
	}

	pci_privdata =
		kzalloc(sizeof(struct pddf_multifpgapci_drvdata), GFP_KERNEL);

	if (!pci_privdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] couldn't allocate pci_privdata memory\n",
			 __FUNCTION__);
		err = -ENOMEM;
		goto error_pci_req;
	}

	pci_privdata->pci_dev = dev;
	pci_set_drvdata(dev, pci_privdata);
	pddf_pci_config_data(dev);
	pddf_pci_add_fpga(bdf, dev);

	return 0;

error_pci_req:
	pci_disable_device(dev);
error_enable_dev:
	return err;
}

// Initialize the driver module (but not any device) and register
// the module with the kernel PCI subsystem.
int pddf_multifpgapci_register(const struct pci_device_id *ids,
			       struct kobject *kobj)
{
	int ret;

	pddf_multifpgapci_driver.name = DRIVER_NAME;
	pddf_multifpgapci_driver.id_table = ids;
	pddf_multifpgapci_driver.probe = pddf_multifpgapci_probe;
	pddf_multifpgapci_driver.remove = pddf_multifpgapci_remove;

	if (!driver_registered) {
		multifpgapci_kobj = kobject_get(kobj);
	}

	ret = pci_register_driver(&pddf_multifpgapci_driver);
	if (ret != 0) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: Failed to register driver\n",
			 __FUNCTION__);
		return -EINVAL;
	}

	driver_registered = true;
	return ret;
}

EXPORT_SYMBOL(pddf_multifpgapci_register);

static void pddf_multifpgapci_remove(struct pci_dev *dev)
{
	struct pddf_multifpgapci_drvdata *pci_privdata = 0;

	if (dev == 0) {
		pddf_dbg(MULTIFPGA, KERN_ERR "[%s]: dev is 0\n", __FUNCTION__);
		return;
	}

	pci_privdata =
		(struct pddf_multifpgapci_drvdata *)dev_get_drvdata(&dev->dev);

	if (!pci_privdata) {
		pddf_dbg(MULTIFPGA, KERN_ERR "[%s]: pci_privdata is NULL\n",
			 __FUNCTION__);
		return;
	}

	delete_fpga_data_node(pci_name(dev));
	free_bars(pci_privdata, dev);
	pci_disable_device(dev);
	pci_release_regions(dev);
	kfree(pci_privdata);
}

// Add cleanup function for module exit
static void cleanup_all_protocols(void)
{
	struct protocol_module *proto, *tmp_proto;
	struct list_head local_list;

	// Move the list to a local one to be able to process without lock
	mutex_lock(&protocol_modules_lock);
	local_list = protocol_modules;
	INIT_LIST_HEAD(&protocol_modules);
	mutex_unlock(&protocol_modules_lock);

	// Work on local copy without any locks
	list_for_each_entry_safe(proto, tmp_proto, &local_list, list) {
		detach_protocols_for_all_fpgas(proto);
		mutex_destroy(&proto->lock);
		kfree(proto);
	}
}

// Add PCI device to protocol's list - returns 1 if added, 0 if already exists
static int protocol_add_pci_dev(struct protocol_module *proto,
				struct pci_dev *pci_dev)
{
	struct protocol_pci_entry *entry;
	int added = 0;

	mutex_lock(&proto->lock);

	// Check if already exists
	list_for_each_entry(entry, &proto->pci_devices, list) {
		if (entry->pci_dev == pci_dev) {
			goto unlock; // Already exists
		}
	}

	// Add new entry
	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (entry) {
		entry->pci_dev = pci_dev;
		list_add(&entry->list, &proto->pci_devices);
		added = 1;
	}

unlock:
	mutex_unlock(&proto->lock);
	return added;
}

// Remove PCI device from protocol's list - returns 1 if removed, 0 if not found
static int protocol_remove_pci_dev(struct protocol_module *proto,
				   struct pci_dev *pci_dev)
{
	struct protocol_pci_entry *entry, *tmp;
	int removed = 0;

	mutex_lock(&proto->lock);
	list_for_each_entry_safe(entry, tmp, &proto->pci_devices, list) {
		if (entry->pci_dev == pci_dev) {
			list_del(&entry->list);
			kfree(entry);
			removed = 1;
			break;
		}
	}
	mutex_unlock(&proto->lock);

	return removed;
}

static void run_attach(struct pci_dev *pci_dev, struct kobject *kobj)
{
	struct protocol_module *proto;
	attach_fn *attach;
	int i = 0;

	attach = kzalloc(sizeof(*attach) * num_protocols, GFP_KERNEL);
	if (!attach) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "%s failed to allocate memory for attach array\n",
			 __FUNCTION__);
		return;
	}

	mutex_lock(&protocol_modules_lock);
	list_for_each_entry(proto, &protocol_modules, list) {
		if (protocol_add_pci_dev(proto, pci_dev) &&
		    proto->ops->attach) {
			attach[i++] = proto->ops->attach;
		}
	}
	mutex_unlock(&protocol_modules_lock);

	// Now run the attach without lock
	for (int j = 0; j < i; j++) {
		int ret = attach[j](pci_dev, kobj);
		if (ret) {
			pddf_dbg(MULTIFPGA,
				 KERN_ERR "Protocol attach failed on %s: %d\n",
				 pci_name(pci_dev), ret);
		}
	}
	kfree(attach);
}

static void attach_protocols_for_fpga(struct pci_dev *pci_dev,
				      struct kobject *kobj)
{
	if (num_protocols > 0) {
		run_attach(pci_dev, kobj);
	}
}

static void run_detach(struct pci_dev *pci_dev, struct kobject *kobj)
{
	struct protocol_module *proto;
	detach_fn *detach;
	int i = 0;

	detach = kzalloc(sizeof(*detach) * num_protocols, GFP_KERNEL);
	if (!detach) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "%s failed to allocate memory for detach array\n",
			 __FUNCTION__);
		return;
	}
	mutex_lock(&protocol_modules_lock);
	list_for_each_entry(proto, &protocol_modules, list) {
		if (protocol_remove_pci_dev(proto, pci_dev) &&
		    proto->ops->detach) {
			detach[i++] = proto->ops->detach;
		}
	}
	mutex_unlock(&protocol_modules_lock);

	for (int j = 0; j < i; j++) {
		detach[j](pci_dev, kobj);
	}
	kfree(detach);
}

static void detach_protocols_for_fpga(struct pci_dev *pci_dev,
				      struct kobject *kobj)
{
	if (num_protocols > 0) {
		run_detach(pci_dev, kobj);
	}
}

static void attach_protocols_for_all_fpgas(struct protocol_module *proto)
{
	struct fpga_data_node *fpga_node;
	struct fpga_work_item *work_item, *tmp;
	struct list_head work_list;

	INIT_LIST_HEAD(&work_list);

	// Build work list under lock
	mutex_lock(&fpga_list_lock);
	list_for_each_entry(fpga_node, &fpga_list, list) {
		work_item = kzalloc(sizeof(*work_item), GFP_KERNEL);
		if (work_item) {
			work_item->pci_dev = fpga_node->dev;
			work_item->kobj = fpga_node->kobj;
			list_add(&work_item->list, &work_list);
		}
	}
	mutex_unlock(&fpga_list_lock);

	// Execute work items without locks
	list_for_each_entry_safe(work_item, tmp, &work_list, list) {
		attach_protocols_for_fpga(work_item->pci_dev, work_item->kobj);
		list_del(&work_item->list);
		kfree(work_item);
	}
}

static void detach_protocols_for_all_fpgas(struct protocol_module *proto)
{
	struct protocol_pci_entry *pci_entry, *tmp_pci;

	// No locking needed - proto is already yanked out of the global list
	list_for_each_entry_safe(pci_entry, tmp_pci, &proto->pci_devices,
				 list) {
		if (proto->ops->detach) {
			struct fpga_data_node *fpga_node = get_fpga_data_node(
				pci_name(pci_entry->pci_dev));
			if (fpga_node) {
				proto->ops->detach(pci_entry->pci_dev,
						   fpga_node->kobj);
			}
		}
		kfree(pci_entry);
	}
}

static void run_map_bar(struct pci_dev *pci_dev, void __iomem *bar_base,
			unsigned long bar_start, unsigned long bar_len)
{
	struct protocol_module *proto;
	map_bar_fn *map_bar;
	int i = 0;
	pddf_dbg(MULTIFPGA, KERN_INFO "%s - %s\n", __FUNCTION__,
		 pci_name(pci_dev));

	map_bar = kzalloc(sizeof(*map_bar) * num_protocols, GFP_KERNEL);
	if (!map_bar) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "%s failed to allocate memory for map_bar array\n",
			 __FUNCTION__);
		return;
	}

	mutex_lock(&protocol_modules_lock);
	list_for_each_entry(proto, &protocol_modules, list) {
		pddf_dbg(MULTIFPGA, KERN_INFO "%s - protocol %s\n",
			 __FUNCTION__, proto->name);
		if (proto->ops->map_bar) {
			map_bar[i++] = proto->ops->map_bar;
		}
	}
	mutex_unlock(&protocol_modules_lock);

	// Execute map_bar calls without locks
	for (int j = 0; j < i; j++) {
		pddf_dbg(MULTIFPGA, KERN_INFO "%s - map_bar %s\n", __FUNCTION__,
			 pci_name(pci_dev));
		map_bar[j](pci_dev, bar_base, bar_start, bar_len);
	}
	kfree(map_bar);
}

static void run_bar_op_for_all_fpgas(struct protocol_module *proto, bool map)
{
	struct fpga_data_node *fpga_node;
	struct fpga_work_item *work_item, *tmp;
	struct list_head work_list;

	INIT_LIST_HEAD(&work_list);

	// Build work list under lock
	mutex_lock(&fpga_list_lock);
	list_for_each_entry(fpga_node, &fpga_list, list) {
		work_item = kzalloc(sizeof(*work_item), GFP_KERNEL);
		if (work_item) {
			work_item->pci_dev = fpga_node->dev;
			work_item->kobj = fpga_node->kobj;
			work_item->map_bar = proto->ops->map_bar;
			// Get bar length from pci_privdata
			struct pddf_multifpgapci_drvdata *pci_privdata =
				dev_get_drvdata(&fpga_node->dev->dev);
			if (pci_privdata) {
				work_item->bar_start = pci_privdata->bar_start;
				work_item->bar_base =
					pci_privdata->fpga_data_base_addr;
				work_item->bar_len = pci_privdata->bar_length;
			}
			list_add(&work_item->list, &work_list);
		}
	}
	mutex_unlock(&fpga_list_lock);

	// Execute work items without locks
	list_for_each_entry_safe(work_item, tmp, &work_list, list) {
		if (work_item->map_bar) {
			if (map) {
				work_item->map_bar(work_item->pci_dev,
						   work_item->bar_base,
						   work_item->bar_start,
						   work_item->bar_len);
			} else {
				work_item->unmap_bar(work_item->pci_dev,
						     work_item->bar_base,
						     work_item->bar_start,
						     work_item->bar_len);
			}
		}
		list_del(&work_item->list);
		kfree(work_item);
	}
}

static void run_unmap_bar(struct pci_dev *pci_dev, void __iomem *bar_base,
			  unsigned long bar_start, unsigned long bar_len)
{
	struct protocol_module *proto;
	unmap_bar_fn *unmap_bar;
	int i = 0;

	unmap_bar = kzalloc(sizeof(*unmap_bar) * num_protocols, GFP_KERNEL);
	if (!unmap_bar) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "%s failed to allocate memory for unmap_bar array\n",
			 __FUNCTION__);
		return;
	}

	mutex_lock(&protocol_modules_lock);
	list_for_each_entry(proto, &protocol_modules, list) {
		if (proto->ops->unmap_bar) {
			unmap_bar[i++] = proto->ops->unmap_bar;
		}
	}
	mutex_unlock(&protocol_modules_lock);

	// Execute unmap_bar calls without locks
	for (int j = 0; j < i; j++) {
		unmap_bar[j](pci_dev, bar_base, bar_start, bar_len);
	}
	kfree(unmap_bar);
}

int multifpgapci_register_protocol(const char *name, struct protocol_ops *ops)
{
	struct protocol_module *proto;

	proto = kzalloc(sizeof(*proto), GFP_KERNEL);
	if (!proto)
		return -ENOMEM;

	strscpy(proto->name, name, sizeof(proto->name));
	proto->ops = ops;
	INIT_LIST_HEAD(&proto->pci_devices);
	mutex_init(&proto->lock);

	// Add to registry
	mutex_lock(&protocol_modules_lock);
	list_add(&proto->list, &protocol_modules);
	num_protocols++;
	mutex_unlock(&protocol_modules_lock);

	// Attach protocol to all existing FPGAs
	attach_protocols_for_all_fpgas(proto);

	// Map BARs for all existing FPGAs
	run_bar_op_for_all_fpgas(proto, true);

	pddf_dbg(MULTIFPGA, KERN_INFO "Registered protocol: %s\n", name);
	return 0;
}
EXPORT_SYMBOL(multifpgapci_register_protocol);

void multifpgapci_unregister_protocol(const char *name)
{
	struct protocol_module *proto, *tmp_proto;
	struct protocol_module *found_proto = NULL;

	// Find and remove protocol from registry
	mutex_lock(&protocol_modules_lock);
	list_for_each_entry_safe(proto, tmp_proto, &protocol_modules, list) {
		if (strcmp(proto->name, name) == 0) {
			list_del(&proto->list);
			found_proto = proto;
			num_protocols--;
			break;
		}
	}
	mutex_unlock(&protocol_modules_lock);

	if (!found_proto)
		return;

	// Unmap BARs for all FPGAs first
	run_bar_op_for_all_fpgas(found_proto, false);

	// Detach protocol from all FPGAs without locks
	detach_protocols_for_all_fpgas(found_proto);
	mutex_destroy(&found_proto->lock);
	kfree(found_proto);
	pddf_dbg(MULTIFPGA, KERN_INFO "Unregistered protocol: %s\n", name);
}
EXPORT_SYMBOL(multifpgapci_unregister_protocol);

unsigned long multifpgapci_get_pci_dev_index(struct pci_dev *pci_dev)
{
	unsigned int domain = pci_domain_nr(pci_dev->bus);
	unsigned int bus = pci_dev->bus->number;
	unsigned int slot = PCI_SLOT(pci_dev->devfn);
	unsigned int func = PCI_FUNC(pci_dev->devfn);
	return (domain << 16) | (bus << 8) | (slot << 3) | func;
}
EXPORT_SYMBOL(multifpgapci_get_pci_dev_index);

int __init pddf_multifpgapci_driver_init(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "%s ..\n", __FUNCTION__);
	mutex_init(&fpga_list_lock);
	return 0;
}

void __exit pddf_multifpgapci_driver_exit(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "%s ..\n", __FUNCTION__);

	// Cleanup all protocols first
	cleanup_all_protocols();

	if (driver_registered) {
		// Unregister this driver from the PCI bus driver
		pci_unregister_driver(&pddf_multifpgapci_driver);
		driver_registered = false;
	}

	delete_all_fpga_data_nodes();
	KOBJ_FREE(multifpgapci_kobj);

	return;
}

module_init(pddf_multifpgapci_driver_init);
module_exit(pddf_multifpgapci_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION("PDDF driver for systems with multiple PCI FPGAs");
