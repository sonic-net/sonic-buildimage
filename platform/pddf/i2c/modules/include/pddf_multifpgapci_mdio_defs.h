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

#ifndef __PDDF_MULTIFPGAPCI_MDIO_DEFS_H__
#define __PDDF_MULTIFPGAPCI_MDIO_DEFS_H__

#include "linux/types.h"
#include <linux/kobject.h>
#include <linux/pci.h>
#include <linux/phy.h>
#include <linux/sysfs.h>

#include "pddf_client_defs.h"

#define MDIO_MAX_BUS 512

struct mdio_bus_attrs {
	PDDF_ATTR attr_ch_base_offset;
	PDDF_ATTR attr_ch_size;
	PDDF_ATTR attr_num_virt_ch;
	PDDF_ATTR attr_new_mdio_bus;
	PDDF_ATTR attr_del_mdio_bus;
};

#define NUM_MDIO_BUS_ATTRS (sizeof(struct mdio_bus_attrs) / sizeof(PDDF_ATTR))

struct mdio_bus_sysfs_vals {
	uint32_t ch_base_offset;
	uint32_t ch_size;
	uint32_t num_virt_ch;
};

struct mdio_bus_drvdata {
	struct pci_dev *pci_dev;
	size_t bar_length;
	struct kobject *mdio_kobj;

	// temp_sysfs_vals store temporary values provided by sysfs,
	// which are eventually copied/saved to MDIO bus platform data.
	struct mdio_bus_sysfs_vals temp_sysfs_vals;

	// platform data
	struct mii_bus *mdio_buses[MDIO_MAX_BUS];
	bool mdio_bus_registered[MDIO_MAX_BUS];
	void *__iomem ch_base_addr;
	int ch_size;
	int num_virt_ch;

	// sysfs attrs
	struct mdio_bus_attrs attrs;
	struct attribute *mdio_bus_attrs[NUM_MDIO_BUS_ATTRS + 1];
	struct attribute_group mdio_bus_attr_group;
};

struct fpga_mdio_priv {
	void __iomem *reg_base; // Base address for this MDIO instance
	struct mutex lock; // Mutex for this MDIO instance
	int last_read_value;
};

struct mdio_fpga_ops {
	int (*read)(struct mii_bus *bus, int phy_id, int regnum);
	int (*write)(struct mii_bus *bus, int phy_id, int regnum, u16 val);
};

extern struct mdio_fpga_ops *mdio_fpga_algo_ops;

#endif
