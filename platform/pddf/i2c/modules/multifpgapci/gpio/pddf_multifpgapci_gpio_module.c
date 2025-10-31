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

#include <asm-generic/errno-base.h>
#include <linux/device.h>
#include <linux/gfp_types.h>
#include <linux/kobject.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>

#include "pddf_client_defs.h"
#include "pddf_multifpgapci_defs.h"
#include "pddf_multifpgapci_gpio_defs.h"

DEFINE_XARRAY(gpio_drvdata_map);

static ssize_t create_chip(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct gpio_chip_drvdata *gpio_drvdata;
	struct platform_device *pdev;

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	gpio_drvdata = xa_load(&gpio_drvdata_map, dev_index);
	if (!gpio_drvdata) {
		pddf_dbg(FPGA,
			 KERN_ERR
			 "[%s] unable to find gpio data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENODEV;
	}

	if (strncmp(buf, "init", strlen("init")) != 0) {
		pddf_dbg(FPGA,
			 KERN_ERR
			 "[%s] Unexpected input: %s - Expected input: init\n",
			 __FUNCTION__, buf);
		return -EINVAL;
	}

	pdev = platform_device_register_data(&gpio_drvdata->pdata.fpga->dev,
					     "pddf-multifpgapci-gpio",
					     PLATFORM_DEVID_AUTO,
					     &gpio_drvdata->pdata,
					     sizeof(gpio_drvdata->pdata));
	if (!pdev) {
		pddf_dbg(FPGA,
			 KERN_ERR "[%s] error allocating platform device\n",
			 __FUNCTION__);
		return -ENOMEM;
	}

	return count;
}

static ssize_t create_line(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct gpio_chip_drvdata *node;

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	node = xa_load(&gpio_drvdata_map, dev_index);
	if (!node) {
		pddf_dbg(FPGA,
			 KERN_ERR
			 "[%s] unable to find gpio data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENODEV;
	}

	if (strncmp(buf, "init", strlen("init")) != 0) {
		pddf_dbg(FPGA,
			 KERN_ERR
			 "[%s] Unexpected input: %s - Expected input: init\n",
			 __FUNCTION__, buf);
		return -EINVAL;
	}

	if (node->pdata.ngpio >= MAX_MULTIFPGAPCI_GPIO_LINES) {
		pddf_dbg(FPGA, KERN_ERR "[%s] Cannot exceed %d GPIO lines\n",
			 __FUNCTION__, MAX_MULTIFPGAPCI_GPIO_LINES);
		return -EINVAL;
	}

	node->pdata.chan_data[node->pdata.ngpio++] = node->temp_line_data;

	return count;
}

static int pddf_multifpgapci_gpio_attach(struct pci_dev *pci_dev,
					 struct kobject *kobj)
{
	struct gpio_chip_drvdata *gpio_drvdata;
	int err;
	pddf_dbg(MULTIFPGA, KERN_INFO "%s start\n", __FUNCTION__);

	gpio_drvdata = kzalloc(sizeof(struct gpio_chip_drvdata), GFP_KERNEL);
	if (!gpio_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] failed to allocate drvdata for %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENOMEM;
	}

	gpio_drvdata->gpio_kobj = kobject_create_and_add("gpio", kobj);
	if (!gpio_drvdata->gpio_kobj) {
		pddf_dbg(MULTIFPGA, KERN_ERR "[%s] create gpio kobj failed\n",
			 __FUNCTION__);
		err = -ENOMEM;
		goto return_err;
	}

	gpio_drvdata->pdata.fpga = pci_dev;

	PDDF_DATA_ATTR(
		ngpio, S_IRUGO, show_pddf_data, NULL, PDDF_UINT32,
		sizeof(uint32_t), (void *)&gpio_drvdata->pdata.ngpio, NULL);
	PDDF_DATA_ATTR(
		create_chip, S_IWUSR, NULL, create_chip, PDDF_CHAR,
		32, (void *)pci_dev, NULL);

	gpio_drvdata->attrs.attr_ngpio = attr_ngpio;
	gpio_drvdata->attrs.attr_create = attr_create_chip;

	gpio_drvdata->gpio_chip_attrs[0] =
		&gpio_drvdata->attrs.attr_ngpio.dev_attr.attr;
	gpio_drvdata->gpio_chip_attrs[1] =
		&gpio_drvdata->attrs.attr_create.dev_attr.attr;
	gpio_drvdata->gpio_chip_attrs[2] = NULL;

	gpio_drvdata->gpio_chip_attr_group.attrs =
		gpio_drvdata->gpio_chip_attrs;

	err = sysfs_create_group(gpio_drvdata->gpio_kobj,
				 &gpio_drvdata->gpio_chip_attr_group);
	if (err) {
		pddf_dbg(
			MULTIFPGA,
			KERN_ERR
			"[%s] unable create sysfs files for device %s - error %d\n",
			__FUNCTION__, pci_name(pci_dev), err);
		goto remove_gpio_kobj;
	}

	gpio_drvdata->line_kobj =
		kobject_create_and_add("line", gpio_drvdata->gpio_kobj);
	if (!gpio_drvdata->line_kobj) {
		pddf_dbg(MULTIFPGA, KERN_ERR "[%s] create line kobj failed\n",
			 __FUNCTION__);
		err = -ENOMEM;
		goto remove_gpio_chip_attr_group;
	}

	PDDF_DATA_ATTR(
		bit, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_INT_HEX, sizeof(uint32_t),
		(void *)&gpio_drvdata->temp_line_data.bit, NULL);
	PDDF_DATA_ATTR(
		offset, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_INT_HEX, sizeof(uint32_t),
		(void *)&gpio_drvdata->temp_line_data.offset, NULL);
	PDDF_DATA_ATTR(
		direction, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_INT_HEX, sizeof(uint32_t),
		(void *)&gpio_drvdata->temp_line_data.direction, NULL);
	PDDF_DATA_ATTR(
		create_line, S_IWUSR, NULL, create_line,
		PDDF_CHAR, 32, (void *)pci_dev, NULL);

	gpio_drvdata->attrs.line_attrs.attr_bit = attr_bit;
	gpio_drvdata->attrs.line_attrs.attr_offset = attr_offset;
	gpio_drvdata->attrs.line_attrs.attr_direction = attr_direction;
	gpio_drvdata->attrs.line_attrs.attr_create = attr_create_line;

	gpio_drvdata->gpio_line_attrs[0] =
		&gpio_drvdata->attrs.line_attrs.attr_bit.dev_attr.attr;
	gpio_drvdata->gpio_line_attrs[1] =
		&gpio_drvdata->attrs.line_attrs.attr_offset.dev_attr.attr;
	gpio_drvdata->gpio_line_attrs[2] =
		&gpio_drvdata->attrs.line_attrs.attr_direction.dev_attr.attr;
	gpio_drvdata->gpio_line_attrs[3] =
		&gpio_drvdata->attrs.line_attrs.attr_create.dev_attr.attr;
	gpio_drvdata->gpio_line_attrs[4] = NULL;

	gpio_drvdata->gpio_line_attr_group.attrs =
		gpio_drvdata->gpio_line_attrs;

	err = sysfs_create_group(gpio_drvdata->line_kobj,
				 &gpio_drvdata->gpio_line_attr_group);
	if (err) {
		pddf_dbg(
			MULTIFPGA,
			KERN_ERR
			"[%s] unable to create sysfs files for group %s - error %d\n",
			__FUNCTION__, pci_name(pci_dev), err);
		goto free_gpio_line_kobj;
	}

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	xa_store(&gpio_drvdata_map, dev_index, gpio_drvdata, GFP_KERNEL);
	pddf_dbg(MULTIFPGA, KERN_INFO "%s done!\n", __FUNCTION__);
	return 0;

free_gpio_line_kobj:
	kobject_put(gpio_drvdata->line_kobj);

remove_gpio_chip_attr_group:
	sysfs_remove_group(gpio_drvdata->gpio_kobj,
			   &gpio_drvdata->gpio_chip_attr_group);

remove_gpio_kobj:
	kobject_put(gpio_drvdata->gpio_kobj);

return_err:
	return err;
}

static void pddf_multifpgapci_gpio_detach(struct pci_dev *pci_dev,
					  struct kobject *kobj)
{
	struct gpio_chip_drvdata *gpio_drvdata;

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	gpio_drvdata = xa_load(&gpio_drvdata_map, dev_index);
	if (!gpio_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find gpio module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}

	if (gpio_drvdata->line_kobj) {
		sysfs_remove_group(gpio_drvdata->line_kobj,
				   &gpio_drvdata->gpio_line_attr_group);
		kobject_put(gpio_drvdata->line_kobj);
		gpio_drvdata->line_kobj = NULL;
	}
	if (gpio_drvdata->gpio_kobj) {
		sysfs_remove_group(gpio_drvdata->gpio_kobj,
				   &gpio_drvdata->gpio_chip_attr_group);
		kobject_put(gpio_drvdata->gpio_kobj);
		gpio_drvdata->gpio_kobj = NULL;
	}

	kfree(gpio_drvdata);
	xa_erase(&gpio_drvdata_map, (unsigned long)pci_dev);
}

static void pddf_multifpgapci_gpio_map_bar(struct pci_dev *pci_dev,
					   void __iomem *bar_base,
					   unsigned long bar_start,
					   unsigned long bar_len)
{
}

static void pddf_multifpgapci_gpio_unmap_bar(struct pci_dev *pci_dev,
					     void __iomem *bar_base,
					     unsigned long bar_start,
					     unsigned long bar_len)
{
}

static struct protocol_ops gpio_protocol_ops = {
	.attach = pddf_multifpgapci_gpio_attach,
	.detach = pddf_multifpgapci_gpio_detach,
	.map_bar = pddf_multifpgapci_gpio_map_bar,
	.unmap_bar = pddf_multifpgapci_gpio_unmap_bar,
	.name = "gpio",
};

static int __init pddf_multifpgapci_gpio_init(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "Loading GPIO protocol module\n");
	xa_init(&gpio_drvdata_map);
	return multifpgapci_register_protocol("gpio", &gpio_protocol_ops);
}

static void __exit pddf_multifpgapci_gpio_exit(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "Unloading GPIO protocol module\n");
	multifpgapci_unregister_protocol("gpio");
	xa_destroy(&gpio_drvdata_map);
}

module_init(pddf_multifpgapci_gpio_init);
module_exit(pddf_multifpgapci_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION("PDDF Platform Data for Multiple PCI FPGA GPIOs.");
