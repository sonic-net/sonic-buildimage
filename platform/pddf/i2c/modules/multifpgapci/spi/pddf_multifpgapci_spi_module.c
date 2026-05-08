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
#include <linux/string.h>
#include <linux/xarray.h>
#include <linux/pci.h>
#include <linux/spi/spi.h>
#include <linux/spi/xilinx_spi.h>
#include <linux/platform_device.h>
#include "pddf_client_defs.h"
#include "pddf_multifpgapci_defs.h"
#include "pddf_multifpgapci_gpio_defs.h"
#include "pddf_multifpgapci_spi_defs.h"

extern void delete_device_table(char *name);

DEFINE_XARRAY(spi_drvdata_map);

ssize_t new_spi_controller(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct spi_controller_drvdata *spi_drvdata;
	int index = 0, error;

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	spi_drvdata = xa_load(&spi_drvdata_map, dev_index);
	if (!spi_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find spi module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENODEV;
	}

	error = kstrtoint(buf, 10, &index);
	if (error != 0) {
		pddf_dbg(MULTIFPGA, KERN_ERR "Error converting string: %d\n",
			 error);
		return error;
	}

	if (index < 1) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: SPI Controller %d < 1\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	if (index > NUM_SPI_CONTROLLERS_MAX) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: SPI Controller %d >= %d\n",
			 __FUNCTION__, index, NUM_SPI_CONTROLLERS_MAX);
		return -ENODEV;
	}

	// We're translating index here from 1 based (in sysfs) to 0 based
	// (inside the driver)
	index -= 1;

	if (spi_drvdata->spi_controllers[index]) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s: SPI Controller %d already exists\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	unsigned long spi_start = spi_drvdata->bar_start +
				  spi_drvdata->temp_sysfs_vals.spi_base_addr;
	pddf_dbg(
		MULTIFPGA,
		KERN_INFO
		"%s: fpga_data_base_addr: 0x%08lx spi_start 0x%08lx index %d \n",
		__FUNCTION__, spi_drvdata->bar_start, spi_start,
		 index);

	struct resource res = { 0 };

	// Here we add 1 to index so the device will be spi1.cs (cs == chip select number)
	struct platform_device *pdev = platform_device_alloc(
		spi_drvdata->temp_sysfs_vals.spi_driver, index + 1);

	if (!pdev) {
		return -ENOMEM;
	}

	struct xspi_platform_data xpd = { 0 };
	xpd.bits_per_word = 8;
	xpd.devices = NULL;
	xpd.num_devices = 0;
	xpd.num_chipselect = spi_drvdata->temp_sysfs_vals.spi_num_cs;

	res.start = spi_start;
	res.end = spi_start +
		  spi_drvdata->temp_sysfs_vals.virt_spi_controller_size - 1;
	res.flags = IORESOURCE_MEM;
	platform_device_add_resources(pdev, &res, 1);

	pdev->dev.parent = &pci_dev->dev;
	platform_device_add_data(pdev, &xpd, sizeof(xpd));

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] Register platform dev %d\n",
		 __FUNCTION__, index);
	int ret = platform_device_add(pdev);
	if (ret) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "Cannot register platform device: %d\n", ret);
		goto put_platform_device;
	}

	spi_drvdata->spi_controllers[index] = pdev;
	strscpy(spi_drvdata->spi_controller_name,
		spi_drvdata->temp_sysfs_vals.spi_controller_name,
		sizeof(spi_drvdata->spi_controller_name));

	struct spi_controller *controller = platform_get_drvdata(pdev);

	pddf_dbg(MULTIFPGA,
			 KERN_INFO
			 "[%s] platform dev %d registered. name: %s. controller: %p\n",
			 __FUNCTION__, index, spi_drvdata->spi_controller_name, controller);

	add_device_table(spi_drvdata->spi_controller_name, controller);
	return count;

put_platform_device:
	platform_device_put(pdev);
	return ret;
}

ssize_t del_spi_controller(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct spi_controller_drvdata *spi_drvdata;
	int index = 0, error;

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	spi_drvdata = xa_load(&spi_drvdata_map, dev_index);
	if (!spi_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find spi module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENODEV;
	}

	error = kstrtoint(buf, 10, &index);
	if (error != 0) {
		pddf_dbg(MULTIFPGA, KERN_ERR "Error converting string: %d\n",
			 error);
		return error;
	}

	if (index < 1) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: SPI Controller %d < 1\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	if (index > NUM_SPI_CONTROLLERS_MAX) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: SPI Controller %d >= %d\n",
			 __FUNCTION__, index, NUM_SPI_CONTROLLERS_MAX);
		return -ENODEV;
	}

	// We're translating index here from 1 based (in sysfs) to 0 based
	// (inside the driver)
	index -= 1;

	if (spi_drvdata->spi_controllers[index] == NULL) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s: SPI Controller %d doesn't exist\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] Unregister platform dev %d\n",
		 __FUNCTION__, index);
	platform_device_unregister(spi_drvdata->spi_controllers[index]);
	spi_drvdata->spi_controllers[index] = NULL;
	delete_device_table(spi_drvdata->spi_controller_name);

	return count;
}

static int pddf_multifpgapci_spi_attach(struct pci_dev *pci_dev,
					struct kobject *kobj)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct spi_controller_drvdata *spi_drvdata;
	int err;

	spi_drvdata =
		kzalloc(sizeof(struct spi_controller_drvdata), GFP_KERNEL);
	if (!spi_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] failed to allocate drvdata for %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENOMEM;
	}

	spi_drvdata->spi_kobj = kobject_create_and_add("spi", kobj);
	if (!spi_drvdata->spi_kobj) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] create spi kobj failed for %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENOMEM;
	}

	PDDF_DATA_ATTR(
		new_spi_controller, S_IWUSR | S_IRUGO, show_pddf_data,
		new_spi_controller, PDDF_CHAR, NAME_SIZE, (void *)pci_dev,
		NULL);
	PDDF_DATA_ATTR(
		del_spi_controller, S_IWUSR | S_IRUGO, show_pddf_data,
		del_spi_controller, PDDF_CHAR, NAME_SIZE, (void *)pci_dev,
		NULL);
	PDDF_DATA_ATTR(
		virt_spi_controllers, S_IWUSR | S_IRUGO, show_pddf_data,
		store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		(void *)&spi_drvdata->temp_sysfs_vals.virt_spi_controllers,
		NULL);
	PDDF_DATA_ATTR(
		virt_spi_controller_size, S_IWUSR | S_IRUGO, show_pddf_data,
		store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		(void *)&spi_drvdata->temp_sysfs_vals.virt_spi_controller_size,
		NULL);
	PDDF_DATA_ATTR(
		spi_base_addr, S_IWUSR | S_IRUGO, show_pddf_data,
		store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		(void *)&spi_drvdata->temp_sysfs_vals.spi_base_addr, NULL);
	PDDF_DATA_ATTR(
		spi_num_cs, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_UINT32, sizeof(uint32_t),
		(void *)&spi_drvdata->temp_sysfs_vals.spi_num_cs, NULL);
	PDDF_DATA_ATTR(
		spi_driver, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_CHAR, NAME_SIZE,
		(void *)spi_drvdata->temp_sysfs_vals.spi_driver, NULL);
	PDDF_DATA_ATTR(
		spi_controller_name, S_IWUSR | S_IRUGO, show_pddf_data,
		store_pddf_data, PDDF_CHAR, NAME_SIZE,
		(void *)spi_drvdata->temp_sysfs_vals.spi_controller_name, NULL);

	spi_drvdata->attrs.attr_new_spi_controller = attr_new_spi_controller;
	spi_drvdata->attrs.attr_del_spi_controller = attr_del_spi_controller;
	spi_drvdata->attrs.attr_virt_spi_controllers = attr_virt_spi_controllers;
	spi_drvdata->attrs.attr_virt_spi_controller_size =
		attr_virt_spi_controller_size;
	spi_drvdata->attrs.attr_spi_base_addr = attr_spi_base_addr;
	spi_drvdata->attrs.attr_spi_num_cs = attr_spi_num_cs;
	spi_drvdata->attrs.attr_spi_driver = attr_spi_driver;
	spi_drvdata->attrs.attr_spi_controller_name = attr_spi_controller_name;

	struct attribute *spi_controller_attrs[NUM_SPI_CONTROLLER_ATTRS + 1] = {
		&spi_drvdata->attrs.attr_new_spi_controller.dev_attr.attr,
		&spi_drvdata->attrs.attr_del_spi_controller.dev_attr.attr,
		&spi_drvdata->attrs.attr_virt_spi_controllers.dev_attr.attr,
		&spi_drvdata->attrs.attr_virt_spi_controller_size.dev_attr.attr,
		&spi_drvdata->attrs.attr_spi_base_addr.dev_attr.attr,
		&spi_drvdata->attrs.attr_spi_num_cs.dev_attr.attr,
		&spi_drvdata->attrs.attr_spi_driver.dev_attr.attr,
		&spi_drvdata->attrs.attr_spi_controller_name.dev_attr.attr,
		NULL,
	};

	memcpy(spi_drvdata->spi_controller_attrs, spi_controller_attrs,
	       sizeof(spi_drvdata->spi_controller_attrs));

	spi_drvdata->spi_controller_attr_group.attrs =
		spi_drvdata->spi_controller_attrs;

	err = sysfs_create_group(spi_drvdata->spi_kobj,
				 &spi_drvdata->spi_controller_attr_group);
	if (err) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] sysfs_create_group error, status: %d\n",
			 __FUNCTION__, err);
		return err;
	}
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	xa_store(&spi_drvdata_map, dev_index, spi_drvdata, GFP_KERNEL);

	return 0;
}

static void pddf_multifpgapci_spi_detach(struct pci_dev *pci_dev,
					 struct kobject *kobj)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct spi_controller_drvdata *spi_drvdata;
	int i;

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	spi_drvdata = xa_load(&spi_drvdata_map, dev_index);
	if (!spi_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find spi module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}

	for (i = 0; i < NUM_SPI_CONTROLLERS_MAX; i++) {
		if (spi_drvdata->spi_controllers[i]) {
			platform_device_unregister(
				spi_drvdata->spi_controllers[i]);
		}
	}

	if (spi_drvdata->spi_kobj) {
		sysfs_remove_group(spi_drvdata->spi_kobj,
				   &spi_drvdata->spi_controller_attr_group);
		kobject_put(spi_drvdata->spi_kobj);
		spi_drvdata->spi_kobj = NULL;
	}
	
	xa_erase(&spi_drvdata_map, dev_index);
	
	kfree(spi_drvdata);
	spi_drvdata = NULL;
}

static void pddf_multifpgapci_spi_map_bar(struct pci_dev *pci_dev,
					  void __iomem *bar_base,
					  unsigned long bar_start,
					  unsigned long bar_len)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct spi_controller_drvdata *spi_drvdata;
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	spi_drvdata = xa_load(&spi_drvdata_map, dev_index);
	if (!spi_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find spi module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}
	spi_drvdata->bar_base = bar_base;
	spi_drvdata->bar_start = bar_start;
	spi_drvdata->bar_len = bar_len;
}

static void pddf_multifpgapci_spi_unmap_bar(struct pci_dev *pci_dev,
					    void __iomem *base,
					    unsigned long bar_start,
					    unsigned long bar_len)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct spi_controller_drvdata *spi_drvdata;
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	spi_drvdata = xa_load(&spi_drvdata_map, dev_index);
	if (!spi_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find spi module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}
	spi_drvdata->bar_start = 0;
	spi_drvdata->bar_len = 0;
}

static struct protocol_ops spi_protocol_ops = {
	.attach = pddf_multifpgapci_spi_attach,
	.detach = pddf_multifpgapci_spi_detach,
	.map_bar = pddf_multifpgapci_spi_map_bar,
	.unmap_bar = pddf_multifpgapci_spi_unmap_bar,
	.name = "spi",
};

static int __init pddf_multifpgapci_spi_init(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "Loading SPI protocol module\n");
	xa_init(&spi_drvdata_map);
	return multifpgapci_register_protocol("spi", &spi_protocol_ops);
}

static void __exit pddf_multifpgapci_spi_exit(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "Unloading SPI protocol module\n");
	multifpgapci_unregister_protocol("spi");
	xa_destroy(&spi_drvdata_map);
}

module_init(pddf_multifpgapci_spi_init);
module_exit(pddf_multifpgapci_spi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION("PDDF Platform Data for Multiple PCI FPGA SPI controllers.");
