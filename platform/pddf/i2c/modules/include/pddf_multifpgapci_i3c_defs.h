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
 *	Platform MULTIFPGAPCI I3C defines/structures header file
 */

#ifndef __PDDF_MULTIFPGAPCI_I3C_DEFS_H__
#define __PDDF_MULTIFPGAPCI_I3C_DEFS_H__

#include "linux/platform_device.h"
#include "linux/types.h"
#include <linux/kobject.h>
#include <linux/pci.h>
#include <linux/sysfs.h>

#include "pddf_client_defs.h"

#ifndef NAME_SIZE
#define NAME_SIZE 32
#endif
#define NUM_I3C_CONTROLLERS_MAX 16

struct i3c_controller_attrs {
	PDDF_ATTR attr_i3c_bar_index;
	PDDF_ATTR attr_i3c_base_addr;
	PDDF_ATTR attr_virt_i3c_controller_size;
	PDDF_ATTR attr_i3c_clk_hz;
	PDDF_ATTR attr_i3c_scl_hz;
	PDDF_ATTR attr_new_i3c_master;
	PDDF_ATTR attr_del_i3c_master;
	PDDF_ATTR attr_i3c_driver;
	PDDF_ATTR attr_i3c_controller_name;
};

#define NUM_I3C_CONTROLLER_ATTRS \
	(sizeof(struct i3c_controller_attrs) / sizeof(PDDF_ATTR))

struct i3c_controller_sysfs_vals {
	// i3c_bar_index selects which PCIe BAR of the FPGA holds this
	// controller's AXI window (pcie_to_i3c: one BAR per master).
	uint32_t i3c_bar_index;
	// i3c_base_addr is the controller's offset within that BAR (0 for the
	// BAR-per-controller layout; explicit per-controller for a packed
	// single-BAR / multi-controller FPGA).
	uint32_t i3c_base_addr;
	uint32_t virt_i3c_controller_size;
	// i3c_clk_hz is the AXI/functional input clock feeding the core.
	uint32_t i3c_clk_hz;
	// i3c_scl_hz optionally lowers SCL below the core default; 0 keeps the
	// i3c-core default rate.
	uint32_t i3c_scl_hz;
	char i3c_driver[NAME_SIZE];
	char i3c_controller_name[NAME_SIZE];
};

struct i3c_controller_drvdata {
	struct kobject *i3c_kobj;
	void __iomem *bar_base;
	unsigned long bar_start;
	unsigned long bar_len;
	// temp_sysfs_vals store temporary values provided by sysfs,
	// which are eventually consumed when a controller is created.
	struct i3c_controller_sysfs_vals temp_sysfs_vals;
	struct platform_device *i3c_controllers[NUM_I3C_CONTROLLERS_MAX];
	// Per-controller fixed-rate input clock + clkdev lookup, registered
	// before platform_device_add so devm_clk_get_enabled(dev, NULL) resolves.
	struct clk_hw *i3c_clks[NUM_I3C_CONTROLLERS_MAX];
	struct clk_lookup *i3c_clk_lookups[NUM_I3C_CONTROLLERS_MAX];
	// Per-controller name kept for delete_device_table() on teardown.
	char i3c_controller_names[NUM_I3C_CONTROLLERS_MAX][NAME_SIZE];
	// sysfs attrs
	struct i3c_controller_attrs attrs;
	struct attribute *i3c_controller_attrs[NUM_I3C_CONTROLLER_ATTRS + 1];
	struct attribute_group i3c_controller_attr_group;
};

#endif
