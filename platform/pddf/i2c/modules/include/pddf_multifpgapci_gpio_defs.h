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

#ifndef __PDDF_MULTIFPGAPCI_GPIO_DEFS_H__
#define __PDDF_MULTIFPGAPCI_GPIO_DEFS_H__

#include <linux/i2c-mux.h>
#include <linux/kobject.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/sysfs.h>

#include "pddf_client_defs.h"

#define MAX_MULTIFPGAPCI_GPIO_LINES 64

struct pddf_multifpgapci_gpio_line_pdata {
	// Offset within FPGA
	uint32_t offset;
	uint32_t bit;
	// GPIO_LINE_DIRECTION_IN = 0 or GPIO_LINE_DIRECTION_OUT = 1
	int direction;
};

struct pddf_multifpgapci_gpio_chip_pdata {
	// The number of GPIOs handled by this controller.
	int ngpio;
	struct pci_dev *fpga;
	struct pddf_multifpgapci_gpio_line_pdata
		chan_data[MAX_MULTIFPGAPCI_GPIO_LINES];
};

struct gpio_line_attrs {
	PDDF_ATTR attr_bit;
	PDDF_ATTR attr_offset;
	PDDF_ATTR attr_direction;
	PDDF_ATTR attr_create;
};

#define NUM_GPIO_LINE_ATTRS (sizeof(struct gpio_line_attrs) / sizeof(PDDF_ATTR))

struct gpio_chip_attrs {
	struct gpio_line_attrs line_attrs;
	PDDF_ATTR attr_ngpio;
	PDDF_ATTR attr_create;
};

#define NUM_GPIO_CHIP_ATTRS                                                  \
	((sizeof(struct gpio_chip_attrs) - sizeof(struct gpio_line_attrs)) / \
	 sizeof(PDDF_ATTR))

struct gpio_chip_drvdata {
	struct kobject *gpio_kobj;
	// pdata is passed to gpio platform driver.
	struct pddf_multifpgapci_gpio_chip_pdata pdata;
	// temp_line_data is mutated by sysfs attrs and copied to pdata.
	struct pddf_multifpgapci_gpio_line_pdata temp_line_data;
	// sysfs attrs
	struct gpio_chip_attrs attrs;
	struct kobject *line_kobj;
	struct attribute *gpio_line_attrs[NUM_GPIO_LINE_ATTRS + 1];
	struct attribute_group gpio_line_attr_group;
	struct attribute *gpio_chip_attrs[NUM_GPIO_CHIP_ATTRS + 1];
	struct attribute_group gpio_chip_attr_group;
};

extern int pddf_multifpgapci_gpio_module_init(struct pci_dev *pci_dev,
					      struct kobject *kobj);

// Only called if multifpgapci_gpio_module_init succeeded
extern void pddf_multifpgapci_gpio_module_exit(struct pci_dev *pci_dev,
					       struct kobject *kobj);

#endif
