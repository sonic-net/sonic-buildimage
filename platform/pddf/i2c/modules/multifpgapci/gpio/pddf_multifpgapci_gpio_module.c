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

static ssize_t create_chip(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pddf_multifpgapci_gpio_chip_pdata *pdata =
		(struct pddf_multifpgapci_gpio_chip_pdata *)_ptr->addr;
	struct platform_device *pdev;

	if (strncmp(buf, "init", strlen("init")) != 0) {
		pddf_dbg(FPGA,
			 KERN_ERR
			 "[%s] Unexpected input: %s - Expected input: init\n",
			 __FUNCTION__, buf);
		return -EINVAL;
	}

	pdev = platform_device_register_data(&pdata->fpga->dev,
					     "pddf-multifpgapci-gpio",
					     PLATFORM_DEVID_AUTO, pdata,
					     sizeof(*pdata));
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
	struct gpio_chip_drvdata *node = (struct gpio_chip_drvdata *)_ptr->addr;

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

int pddf_multifpgapci_gpio_module_init(struct pci_dev *pci_dev,
				       struct kobject *kobj)
{
	struct pddf_multifpgapci_drvdata *pci_drvdata =
		dev_get_drvdata(&pci_dev->dev);
	struct gpio_chip_drvdata *gpio_drvdata =
		&pci_drvdata->gpio_chip_drvdata;
	int err;

	gpio_drvdata->pdata.fpga = pci_dev;

	PDDF_DATA_ATTR(
		ngpio, S_IRUGO, show_pddf_data, NULL, PDDF_UINT32,
		sizeof(uint32_t), (void *)&gpio_drvdata->pdata.ngpio, NULL);
	PDDF_DATA_ATTR(
		create_chip, S_IWUSR, NULL, create_chip, PDDF_CHAR,
		32, (void *)gpio_drvdata, NULL);

	gpio_drvdata->attrs.attr_ngpio = attr_ngpio;
	gpio_drvdata->attrs.attr_create = attr_create_chip;

	gpio_drvdata->gpio_chip_attrs[0] =
		&gpio_drvdata->attrs.attr_ngpio.dev_attr.attr;
	gpio_drvdata->gpio_chip_attrs[1] =
		&gpio_drvdata->attrs.attr_create.dev_attr.attr;
	gpio_drvdata->gpio_chip_attrs[2] = NULL;

	gpio_drvdata->gpio_chip_attr_group.attrs =
		gpio_drvdata->gpio_chip_attrs;

	err = sysfs_create_group(kobj, &gpio_drvdata->gpio_chip_attr_group);
	if (err)
		goto return_err;

	gpio_drvdata->line_kobj = kobject_create_and_add("line", kobj);
	if (!gpio_drvdata->line_kobj) {
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
		create_line, S_IWUSR, NULL, create_line, PDDF_CHAR, 32,
		(void *)&gpio_drvdata->pdata, NULL);

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
	if (err)
		goto free_gpio_line_kobj;

	return 0;

free_gpio_line_kobj:
	kobject_put(gpio_drvdata->line_kobj);

remove_gpio_chip_attr_group:
	sysfs_remove_group(kobj, &gpio_drvdata->gpio_chip_attr_group);

return_err:
	return err;
}
EXPORT_SYMBOL(pddf_multifpgapci_gpio_module_init);

void pddf_multifpgapci_gpio_module_exit(struct pci_dev *pci_dev,
					struct kobject *kobj)
{
	struct pddf_multifpgapci_drvdata *pci_drvdata =
		pci_get_drvdata(pci_dev);
	struct gpio_chip_drvdata *gpio_drvdata =
		&pci_drvdata->gpio_chip_drvdata;
	sysfs_remove_group(gpio_drvdata->line_kobj,
			   &gpio_drvdata->gpio_line_attr_group);
	kobject_put(gpio_drvdata->line_kobj);
	sysfs_remove_group(kobj, &gpio_drvdata->gpio_chip_attr_group);
};
EXPORT_SYMBOL(pddf_multifpgapci_gpio_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION("PDDF Platform Data for Multiple PCI FPGA GPIOs.");
