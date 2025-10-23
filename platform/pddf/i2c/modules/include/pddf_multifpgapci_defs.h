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

#ifndef __PDDF_MULTIFPGAPCI_DEFS_H__
#define __PDDF_MULTIFPGAPCI_DEFS_H__

#include "linux/types.h"
#include <linux/pci.h>

#include "pddf_multifpgapci_gpio_defs.h"
#include "pddf_multifpgapci_i2c_defs.h"

#define NAME_SIZE 32
#define KOBJ_FREE(obj) \
	if (obj)       \
		kobject_put(obj);

struct pddf_multifpgapci_drvdata {
	struct pci_dev *pci_dev;
	resource_size_t bar_start;
	void *__iomem fpga_data_base_addr;
	// i2c
	size_t bar_length;
	struct kobject *i2c_kobj;
	struct i2c_adapter_drvdata i2c_adapter_drvdata;
	bool i2c_adapter_drvdata_initialized;
	// gpio
	struct kobject *gpio_kobj;
	struct gpio_chip_drvdata gpio_chip_drvdata;
	bool gpio_chip_drvdata_initialized;
};

// FPGA
typedef struct {
	uint32_t data_base_offset;
	uint32_t data_size;
} FPGA_OPS_DATA;

struct pddf_multi_fpgapci_ops_t {
	int (*post_device_operation)(struct pci_dev *);
};

extern struct pddf_multi_fpgapci_ops_t pddf_multi_fpgapci_ops;

extern int (*ptr_multifpgapci_readpci)(struct pci_dev *, uint32_t, uint32_t *);
extern int (*ptr_multifpgapci_writepci)(struct pci_dev *, uint32_t, uint32_t);

#endif
