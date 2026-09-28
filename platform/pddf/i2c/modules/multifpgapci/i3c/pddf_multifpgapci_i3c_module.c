/*
 * Copyright (C) Marvell International Ltd. and its affiliates
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
 *	PDDF multifpgapci "i3c" protocol module. Instantiates AXI-I3C master
 *	controllers living in the BAR window(s) of a PCIe FPGA (e.g. the
 *	Marvell pcie_to_i3c design, which exposes one master per BAR at offset
 *	0). Written as a superset: each controller carries its own
 *	i3c_bar_index (default 0) plus an explicit i3c_base_addr (default 0),
 *	so it also covers a generic single-BAR / multi-controller FPGA layout.
 */

#include <asm-generic/errno-base.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/xarray.h>
#include "pddf_client_defs.h"
#include "pddf_multifpgapci_defs.h"
#include "pddf_multifpgapci_i3c_defs.h"

extern void add_device_table(char *name, void *ptr);
extern void delete_device_table(char *name);

DEFINE_XARRAY(i3c_drvdata_map);

/*
 * Register a fixed-rate input clock and a clkdev lookup keyed on the platform
 * device name (con_id NULL) so the controller driver's
 * devm_clk_get_enabled(dev, NULL) resolves. Must run BEFORE
 * platform_device_add() because the driver probes synchronously from there.
 * dev_id is the final "<driver>.<id>" device name, which we build ourselves
 * since dev_name() is only populated during platform_device_add().
 */
static int i3c_controller_register_clocks(struct i3c_controller_drvdata *drvdata,
					  int index, const char *dev_id,
					  unsigned long rate)
{
	struct clk_hw *hw;
	struct clk_lookup *cl;
	char clk_name[NAME_SIZE + 16];

	snprintf(clk_name, sizeof(clk_name), "%s.fck", dev_id);
	hw = clk_hw_register_fixed_rate(NULL, clk_name, NULL, 0, rate);
	if (IS_ERR(hw))
		return PTR_ERR(hw);

	cl = clkdev_hw_create(hw, NULL, "%s", dev_id);
	if (!cl) {
		clk_hw_unregister_fixed_rate(hw);
		return -ENOMEM;
	}

	drvdata->i3c_clks[index] = hw;
	drvdata->i3c_clk_lookups[index] = cl;
	return 0;
}

static void i3c_controller_unregister_clocks(struct i3c_controller_drvdata *drvdata,
					     int index)
{
	if (drvdata->i3c_clk_lookups[index]) {
		clkdev_drop(drvdata->i3c_clk_lookups[index]);
		drvdata->i3c_clk_lookups[index] = NULL;
	}
	if (drvdata->i3c_clks[index]) {
		clk_hw_unregister_fixed_rate(drvdata->i3c_clks[index]);
		drvdata->i3c_clks[index] = NULL;
	}
}

ssize_t new_i3c_master(struct device *dev, struct device_attribute *da,
		       const char *buf, size_t count)
{
	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct i3c_controller_drvdata *i3c_drvdata;
	struct i3c_controller_sysfs_vals *vals;
	struct platform_device *pdev;
	struct resource res;
	resource_size_t bar_start, bar_len, start, len;
	char dev_id[NAME_SIZE + 16];
	int index = 0, error, ret;

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i3c_drvdata = xa_load(&i3c_drvdata_map, dev_index);
	if (!i3c_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find i3c module data for device %s\n",
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
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: I3C Controller %d < 1\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	if (index > NUM_I3C_CONTROLLERS_MAX) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: I3C Controller %d > %d\n",
			 __FUNCTION__, index, NUM_I3C_CONTROLLERS_MAX);
		return -ENODEV;
	}

	// Translate from 1-based (sysfs) to 0-based (internal).
	index -= 1;

	if (i3c_drvdata->i3c_controllers[index]) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s: I3C Controller %d already exists\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	vals = &i3c_drvdata->temp_sysfs_vals;

	if (vals->i3c_driver[0] == '\0') {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: no i3c_driver specified\n",
			 __FUNCTION__);
		return -EINVAL;
	}

	if (vals->i3c_bar_index >= PCI_STD_NUM_BARS) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s: i3c_bar_index %u out of range\n",
			 __FUNCTION__, vals->i3c_bar_index);
		return -EINVAL;
	}

	// Resolve this controller's window directly from the PCI resource. The
	// core driver maps only the first BAR and does not pci_request_regions,
	// so per-BAR resolution here is both necessary (bar_index may be != the
	// mapped one) and free of region conflicts with the controller driver's
	// devm_platform_ioremap_resource().
	bar_start = pci_resource_start(pci_dev, vals->i3c_bar_index);
	bar_len = pci_resource_len(pci_dev, vals->i3c_bar_index);
	if (!bar_start || !bar_len) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s: BAR%u not present on %s\n", __FUNCTION__,
			 vals->i3c_bar_index, pci_name(pci_dev));
		return -ENXIO;
	}

	start = bar_start + vals->i3c_base_addr;
	len = vals->virt_i3c_controller_size ? vals->virt_i3c_controller_size :
					       (bar_len - vals->i3c_base_addr);

	if (vals->i3c_base_addr >= bar_len ||
	    (resource_size_t)vals->i3c_base_addr + len > bar_len) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "%s: window base 0x%x + len 0x%llx exceeds BAR%u len 0x%llx\n",
			 __FUNCTION__, vals->i3c_base_addr,
			 (unsigned long long)len, vals->i3c_bar_index,
			 (unsigned long long)bar_len);
		return -EINVAL;
	}

	pddf_dbg(MULTIFPGA,
		 KERN_INFO
		 "%s: bar%u_start 0x%llx base 0x%x start 0x%llx len 0x%llx index %d\n",
		 __FUNCTION__, vals->i3c_bar_index,
		 (unsigned long long)bar_start, vals->i3c_base_addr,
		 (unsigned long long)start, (unsigned long long)len, index);

	// Device name will be "<driver>.<index+1>"
	snprintf(dev_id, sizeof(dev_id), "%s.%d", vals->i3c_driver, index + 1);

	pdev = platform_device_alloc(vals->i3c_driver, index + 1);
	if (!pdev)
		return -ENOMEM;

	ret = i3c_controller_register_clocks(i3c_drvdata, index, dev_id,
					     vals->i3c_clk_hz);
	if (ret) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s: clock register failed: %d\n",
			 __FUNCTION__, ret);
		goto put_platform_device;
	}

	// Optionally lower SCL below the i3c-core default via a software node
	// property. The controller driver reads this with
	// device_property_read_u32() (see backport patch); a zero value keeps
	// the core default rate.
	if (vals->i3c_scl_hz) {
		struct property_entry props[] = {
			PROPERTY_ENTRY_U32("i3c-scl-hz", vals->i3c_scl_hz),
			{},
		};
		ret = device_create_managed_software_node(&pdev->dev, props,
							  NULL);
		if (ret) {
			pddf_dbg(MULTIFPGA,
				 KERN_ERR "%s: software node create failed: %d\n",
				 __FUNCTION__, ret);
			goto unregister_clocks;
		}
	}

	res = (struct resource)DEFINE_RES_MEM(start, len);
	ret = platform_device_add_resources(pdev, &res, 1);
	if (ret) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s: add resources failed: %d\n",
			 __FUNCTION__, ret);
		goto unregister_clocks;
	}

	pdev->dev.parent = &pci_dev->dev;

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] Register platform dev %s\n",
		 __FUNCTION__, dev_id);
	ret = platform_device_add(pdev);
	if (ret) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "Cannot register platform device: %d\n", ret);
		goto unregister_clocks;
	}

	i3c_drvdata->i3c_controllers[index] = pdev;
	strscpy(i3c_drvdata->i3c_controller_names[index],
		vals->i3c_controller_name,
		sizeof(i3c_drvdata->i3c_controller_names[index]));
	add_device_table(i3c_drvdata->i3c_controller_names[index], pdev);

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] platform dev %s registered\n",
		 __FUNCTION__, dev_id);
	return count;

unregister_clocks:
	i3c_controller_unregister_clocks(i3c_drvdata, index);
put_platform_device:
	platform_device_put(pdev);
	return ret;
}

ssize_t del_i3c_master(struct device *dev, struct device_attribute *da,
		       const char *buf, size_t count)
{
	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct i3c_controller_drvdata *i3c_drvdata;
	int index = 0, error;

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i3c_drvdata = xa_load(&i3c_drvdata_map, dev_index);
	if (!i3c_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find i3c module data for device %s\n",
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
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: I3C Controller %d < 1\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	if (index > NUM_I3C_CONTROLLERS_MAX) {
		pddf_dbg(MULTIFPGA, KERN_ERR "%s: I3C Controller %d > %d\n",
			 __FUNCTION__, index, NUM_I3C_CONTROLLERS_MAX);
		return -ENODEV;
	}

	// Translate from 1-based (sysfs) to 0-based (internal).
	index -= 1;

	if (i3c_drvdata->i3c_controllers[index] == NULL) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "%s: I3C Controller %d doesn't exist\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] Unregister platform dev %d\n",
		 __FUNCTION__, index);
	// Unregister the platform device first so the controller driver's devm
	// cleanup releases (clk_put) the input clock before we drop the clkdev
	// lookup and unregister the fixed-rate clock.
	platform_device_unregister(i3c_drvdata->i3c_controllers[index]);
	i3c_drvdata->i3c_controllers[index] = NULL;
	i3c_controller_unregister_clocks(i3c_drvdata, index);

	if (i3c_drvdata->i3c_controller_names[index][0] != '\0') {
		delete_device_table(i3c_drvdata->i3c_controller_names[index]);
		i3c_drvdata->i3c_controller_names[index][0] = '\0';
	}

	return count;
}

static int pddf_multifpgapci_i3c_attach(struct pci_dev *pci_dev,
					struct kobject *kobj)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct i3c_controller_drvdata *i3c_drvdata;
	int err;

	i3c_drvdata =
		kzalloc(sizeof(struct i3c_controller_drvdata), GFP_KERNEL);
	if (!i3c_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] failed to allocate drvdata for %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENOMEM;
	}

	i3c_drvdata->i3c_kobj = kobject_create_and_add("i3c", kobj);
	if (!i3c_drvdata->i3c_kobj) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] create i3c kobj failed for %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		kfree(i3c_drvdata);
		return -ENOMEM;
	}

	PDDF_DATA_ATTR(
		new_i3c_master, S_IWUSR | S_IRUGO, show_pddf_data,
		new_i3c_master, PDDF_CHAR, NAME_SIZE, (void *)pci_dev, NULL);
	PDDF_DATA_ATTR(
		del_i3c_master, S_IWUSR | S_IRUGO, show_pddf_data,
		del_i3c_master, PDDF_CHAR, NAME_SIZE, (void *)pci_dev, NULL);
	PDDF_DATA_ATTR(
		i3c_bar_index, S_IWUSR | S_IRUGO, show_pddf_data,
		store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		(void *)&i3c_drvdata->temp_sysfs_vals.i3c_bar_index, NULL);
	PDDF_DATA_ATTR(
		i3c_base_addr, S_IWUSR | S_IRUGO, show_pddf_data,
		store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		(void *)&i3c_drvdata->temp_sysfs_vals.i3c_base_addr, NULL);
	PDDF_DATA_ATTR(
		virt_i3c_controller_size, S_IWUSR | S_IRUGO, show_pddf_data,
		store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		(void *)&i3c_drvdata->temp_sysfs_vals.virt_i3c_controller_size,
		NULL);
	PDDF_DATA_ATTR(
		i3c_clk_hz, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_INT_DEC, sizeof(uint32_t),
		(void *)&i3c_drvdata->temp_sysfs_vals.i3c_clk_hz, NULL);
	PDDF_DATA_ATTR(
		i3c_scl_hz, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_INT_DEC, sizeof(uint32_t),
		(void *)&i3c_drvdata->temp_sysfs_vals.i3c_scl_hz, NULL);
	PDDF_DATA_ATTR(
		i3c_driver, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
		PDDF_CHAR, NAME_SIZE,
		(void *)i3c_drvdata->temp_sysfs_vals.i3c_driver, NULL);
	PDDF_DATA_ATTR(
		i3c_controller_name, S_IWUSR | S_IRUGO, show_pddf_data,
		store_pddf_data, PDDF_CHAR, NAME_SIZE,
		(void *)i3c_drvdata->temp_sysfs_vals.i3c_controller_name, NULL);

	i3c_drvdata->attrs.attr_new_i3c_master = attr_new_i3c_master;
	i3c_drvdata->attrs.attr_del_i3c_master = attr_del_i3c_master;
	i3c_drvdata->attrs.attr_i3c_bar_index = attr_i3c_bar_index;
	i3c_drvdata->attrs.attr_i3c_base_addr = attr_i3c_base_addr;
	i3c_drvdata->attrs.attr_virt_i3c_controller_size =
		attr_virt_i3c_controller_size;
	i3c_drvdata->attrs.attr_i3c_clk_hz = attr_i3c_clk_hz;
	i3c_drvdata->attrs.attr_i3c_scl_hz = attr_i3c_scl_hz;
	i3c_drvdata->attrs.attr_i3c_driver = attr_i3c_driver;
	i3c_drvdata->attrs.attr_i3c_controller_name = attr_i3c_controller_name;

	struct attribute *i3c_controller_attrs[NUM_I3C_CONTROLLER_ATTRS + 1] = {
		&i3c_drvdata->attrs.attr_new_i3c_master.dev_attr.attr,
		&i3c_drvdata->attrs.attr_del_i3c_master.dev_attr.attr,
		&i3c_drvdata->attrs.attr_i3c_bar_index.dev_attr.attr,
		&i3c_drvdata->attrs.attr_i3c_base_addr.dev_attr.attr,
		&i3c_drvdata->attrs.attr_virt_i3c_controller_size.dev_attr.attr,
		&i3c_drvdata->attrs.attr_i3c_clk_hz.dev_attr.attr,
		&i3c_drvdata->attrs.attr_i3c_scl_hz.dev_attr.attr,
		&i3c_drvdata->attrs.attr_i3c_driver.dev_attr.attr,
		&i3c_drvdata->attrs.attr_i3c_controller_name.dev_attr.attr,
		NULL,
	};

	memcpy(i3c_drvdata->i3c_controller_attrs, i3c_controller_attrs,
	       sizeof(i3c_drvdata->i3c_controller_attrs));

	i3c_drvdata->i3c_controller_attr_group.attrs =
		i3c_drvdata->i3c_controller_attrs;

	err = sysfs_create_group(i3c_drvdata->i3c_kobj,
				 &i3c_drvdata->i3c_controller_attr_group);
	if (err) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] sysfs_create_group error, status: %d\n",
			 __FUNCTION__, err);
		kobject_put(i3c_drvdata->i3c_kobj);
		kfree(i3c_drvdata);
		return err;
	}
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	xa_store(&i3c_drvdata_map, dev_index, i3c_drvdata, GFP_KERNEL);

	return 0;
}

static void pddf_multifpgapci_i3c_detach(struct pci_dev *pci_dev,
					 struct kobject *kobj)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct i3c_controller_drvdata *i3c_drvdata;
	int i;

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i3c_drvdata = xa_load(&i3c_drvdata_map, dev_index);
	if (!i3c_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find i3c module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}

	for (i = 0; i < NUM_I3C_CONTROLLERS_MAX; i++) {
		if (i3c_drvdata->i3c_controllers[i]) {
			platform_device_unregister(
				i3c_drvdata->i3c_controllers[i]);
			i3c_drvdata->i3c_controllers[i] = NULL;
			i3c_controller_unregister_clocks(i3c_drvdata, i);
			if (i3c_drvdata->i3c_controller_names[i][0] != '\0') {
				delete_device_table(
					i3c_drvdata->i3c_controller_names[i]);
				i3c_drvdata->i3c_controller_names[i][0] = '\0';
			}
		}
	}

	if (i3c_drvdata->i3c_kobj) {
		sysfs_remove_group(i3c_drvdata->i3c_kobj,
				   &i3c_drvdata->i3c_controller_attr_group);
		kobject_put(i3c_drvdata->i3c_kobj);
		i3c_drvdata->i3c_kobj = NULL;
	}

	xa_erase(&i3c_drvdata_map, dev_index);

	kfree(i3c_drvdata);
	i3c_drvdata = NULL;
}

static void pddf_multifpgapci_i3c_map_bar(struct pci_dev *pci_dev,
					  void __iomem *bar_base,
					  unsigned long bar_start,
					  unsigned long bar_len)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct i3c_controller_drvdata *i3c_drvdata;
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i3c_drvdata = xa_load(&i3c_drvdata_map, dev_index);
	if (!i3c_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find i3c module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}
	i3c_drvdata->bar_base = bar_base;
	i3c_drvdata->bar_start = bar_start;
	i3c_drvdata->bar_len = bar_len;
}

static void pddf_multifpgapci_i3c_unmap_bar(struct pci_dev *pci_dev,
					    void __iomem *base,
					    unsigned long bar_start,
					    unsigned long bar_len)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct i3c_controller_drvdata *i3c_drvdata;
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i3c_drvdata = xa_load(&i3c_drvdata_map, dev_index);
	if (!i3c_drvdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find i3c module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}
	i3c_drvdata->bar_base = NULL;
	i3c_drvdata->bar_start = 0;
	i3c_drvdata->bar_len = 0;
}

static struct protocol_ops i3c_protocol_ops = {
	.attach = pddf_multifpgapci_i3c_attach,
	.detach = pddf_multifpgapci_i3c_detach,
	.map_bar = pddf_multifpgapci_i3c_map_bar,
	.unmap_bar = pddf_multifpgapci_i3c_unmap_bar,
	.name = "i3c",
};

static int __init pddf_multifpgapci_i3c_init(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "Loading I3C protocol module\n");
	xa_init(&i3c_drvdata_map);
	return multifpgapci_register_protocol("i3c", &i3c_protocol_ops);
}

static void __exit pddf_multifpgapci_i3c_exit(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "Unloading I3C protocol module\n");
	multifpgapci_unregister_protocol("i3c");
	xa_destroy(&i3c_drvdata_map);
}

module_init(pddf_multifpgapci_i3c_init);
module_exit(pddf_multifpgapci_i3c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marvell Technology, Inc.");
MODULE_DESCRIPTION("PDDF Platform Data for Multiple PCI FPGA I3C controllers.");
