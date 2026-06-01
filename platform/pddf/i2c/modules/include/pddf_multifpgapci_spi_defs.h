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

#ifndef __PDDF_MULTIFPGAPCI_SPI_DEFS_H__
#define __PDDF_MULTIFPGAPCI_SPI_DEFS_H__

#include "linux/platform_device.h"
#include "linux/types.h"
#include <linux/i2c-mux.h>
#include <linux/kobject.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/sysfs.h>

#include "pddf_client_defs.h"

#define NAME_SIZE 32
#define NUM_SPI_CONTROLLERS_MAX 8

struct spi_controller_attrs {
	PDDF_ATTR attr_virt_spi_controllers;
	PDDF_ATTR attr_virt_spi_controller_size;
	PDDF_ATTR attr_spi_base_addr;
	PDDF_ATTR attr_new_spi_controller;
	PDDF_ATTR attr_del_spi_controller;
	PDDF_ATTR attr_spi_num_cs;
	PDDF_ATTR attr_spi_driver;
	PDDF_ATTR attr_spi_controller_name;
};

#define NUM_SPI_CONTROLLER_ATTRS \
	(sizeof(struct spi_controller_attrs) / sizeof(PDDF_ATTR))

struct spi_controller_sysfs_vals {
	uint32_t virt_spi_controllers;
	uint32_t virt_spi_controller_size;
	uint32_t spi_base_addr;
	uint32_t spi_num_cs;
	char spi_driver[NAME_SIZE];
	char spi_controller_name[NAME_SIZE];
};

struct spi_controller_drvdata {
	struct kobject *spi_kobj;
	void __iomem *bar_base;
	unsigned long bar_start;
	unsigned long bar_len;
	// temp_sysfs_vals store temporary values provided by sysfs,
	// which are eventually copied/saved to SPI controller platform data.
	struct spi_controller_sysfs_vals temp_sysfs_vals;
	struct platform_device *spi_controllers[NUM_SPI_CONTROLLERS_MAX];
	char spi_controller_name[NAME_SIZE];
	// sysfs attrs
	struct spi_controller_attrs attrs;
	struct attribute *spi_controller_attrs[NUM_SPI_CONTROLLER_ATTRS + 1];
	struct attribute_group spi_controller_attr_group;
};

#endif
