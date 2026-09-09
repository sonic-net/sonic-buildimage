/*
 * A header definition for wb_io_dev driver
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

#ifndef __WB_IO_DEV_H__
#define __WB_IO_DEV_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define IO_DEV_NAME_MAX_LEN (64)

#define IO_DATA_WIDTH_1         (1)
#define IO_DATA_WIDTH_2         (2)
#define IO_DATA_WIDTH_4         (4)

typedef struct io_dev_device_s {
    char io_dev_name[IO_DEV_NAME_MAX_LEN];
    uint32_t io_base;
    uint32_t io_len;
    uint32_t indirect_addr;
    uint32_t wr_data;
    uint32_t wr_data_width;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t rd_data;
    uint32_t rd_data_width;
    uint32_t opt_ctl;
    int device_flag;
} io_dev_device_t;

#endif
