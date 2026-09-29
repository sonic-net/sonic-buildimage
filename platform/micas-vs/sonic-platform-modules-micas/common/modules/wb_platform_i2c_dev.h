/*
 * A header definition for wb_platform_i2c_dev driver
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

#ifndef __WB_PLATFORM_I2C_DEV_H__
#define __WB_PLATFORM_I2C_DEV_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define I2C_DEV_NAME_MAX_LEN (64)

typedef struct platform_i2c_dev_device_s {
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    char i2c_name[I2C_DEV_NAME_MAX_LEN];
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
    int device_flag;
} platform_i2c_dev_device_t;

#endif
