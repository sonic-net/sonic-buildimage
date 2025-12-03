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
#include "pddf_multifpgapci_mdio_defs.h"

#define NAME_SIZE 32

#ifndef KOBJ_FREE
#define KOBJ_FREE(obj) \
	if (obj)       \
		kobject_put(obj);
#endif

struct pddf_multifpgapci_drvdata {
	struct pci_dev *pci_dev;
	resource_size_t bar_start;
	void *__iomem fpga_data_base_addr;
	size_t bar_length;
	bool bar_initialized;
};

// FPGA
typedef struct {
	uint32_t data_base_offset;
	uint32_t data_size;
} FPGA_OPS_DATA;

struct pddf_multi_fpgapci_ops_t {
	int (*post_device_operation)(struct pci_dev *);
};

// Protocol function pointer types
typedef int (*attach_fn)(struct pci_dev *pci_dev, struct kobject *kobj);
typedef void (*detach_fn)(struct pci_dev *pci_dev, struct kobject *kobj);
typedef void (*map_bar_fn)(struct pci_dev *pci_dev, void *__iomem bar_base,
			   unsigned long bar_start, unsigned long bar_len);
typedef void (*unmap_bar_fn)(struct pci_dev *pci_dev, void *__iomem bar_base,
			     unsigned long bar_start, unsigned long bar_len);

// Protocol operations structure
struct protocol_ops {
	attach_fn attach;
	detach_fn detach;
	map_bar_fn map_bar;
	unmap_bar_fn unmap_bar;
	const char *name;
};

extern struct pddf_multi_fpgapci_ops_t pddf_multi_fpgapci_ops;

extern int (*ptr_multifpgapci_readpci)(struct pci_dev *, uint32_t, uint32_t *);
extern int (*ptr_multifpgapci_writepci)(struct pci_dev *, uint32_t, uint32_t);

extern int multifpgapci_register_protocol(const char *name,
					  struct protocol_ops *ops);
extern void multifpgapci_unregister_protocol(const char *name);
extern unsigned long multifpgapci_get_pci_dev_index(struct pci_dev *pci_dev);

#endif
