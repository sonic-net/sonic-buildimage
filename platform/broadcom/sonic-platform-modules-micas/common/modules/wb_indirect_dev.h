/*
 * A header definition for wb_indirect_dev driver
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

#ifndef __WB_INDIRECT_DEV_H__
#define __WB_INDIRECT_DEV_H__

#include <linux/kallsyms.h>

#define DEV_NAME_LEN        (64)
#define WIDTH_1Byte          (1)
#define WIDTH_2Byte          (2)
#define WIDTH_4Byte          (4)
#define MAX_RW_LEN           (256)

#define mem_clear(data, size) memset((data), 0, (size))

typedef int (*device_func_write)(const char *, uint32_t, uint8_t *, size_t);
typedef int (*device_func_read)(const char *, uint32_t, uint8_t *, size_t );

typedef struct indirect_dev_device_s {
    char  dev_name[DEV_NAME_LEN];
    char  logic_dev_name[DEV_NAME_LEN];
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t indirect_len;
    uint32_t wr_data;
    uint32_t wr_data_width;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t rd_data;
    uint32_t rd_data_width;
    uint32_t opt_ctl;
    uint32_t logic_func_mode;
    int device_flag;
} indirect_dev_device_t;

#endif /* __WB_INDIRECT_DEV_H__ */
