/*
 * A header definition for wb_ocores driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __WB_I2C_OCORES_H__
#define __WB_I2C_OCORES_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define I2C_OCORES_DEV_NAME_MAX_LEN (64)

typedef struct i2c_ocores_device_s {
    uint32_t big_endian;
    char dev_name[I2C_OCORES_DEV_NAME_MAX_LEN];
    int adap_nr;
    uint32_t dev_base;
    uint32_t reg_shift;
    uint32_t reg_io_width;
    uint32_t ip_clock_khz;
    uint32_t bus_clock_khz;
    uint32_t reg_access_mode;

    uint32_t irq_type;
    uint32_t irq_offset;
    uint32_t pci_domain;
    uint32_t pci_bus;
    uint32_t pci_slot;
    uint32_t pci_fn;
    uint32_t check_pci_id;
    uint32_t pci_id;
    int device_flag;
} i2c_ocores_device_t;

#endif
