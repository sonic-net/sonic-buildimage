/*
 * A header definition for wb_spi_ocores driver
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

#ifndef __WB_SPI_OCORES_H__
#define __WB_SPI_OCORES_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define SPI_OCORES_DEV_NAME_MAX_LEN (64)

typedef struct spi_ocores_device_s {
    uint32_t bus_num;
    uint32_t big_endian;
    char dev_name[SPI_OCORES_DEV_NAME_MAX_LEN];
    uint32_t reg_access_mode;
    uint32_t dev_base;
    uint32_t reg_shift;
    uint32_t reg_io_width;
    uint32_t clock_frequency;
    uint32_t num_chipselect;
    uint32_t irq_flag;
    int device_flag;
} spi_ocores_device_t;

#endif
