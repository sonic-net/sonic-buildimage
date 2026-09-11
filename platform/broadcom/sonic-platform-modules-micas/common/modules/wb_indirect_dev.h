/*
 * An wb_indirect_dev driver for indirect adapter device function
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
#include <wb_logic_dev_common.h>

typedef struct indirect_dev_device_s {
    char dev_name[MAX_NAME_SIZE];
    char dev_alias[MAX_NAME_SIZE];
    char logic_dev_name[MAX_NAME_SIZE];
    uint32_t data_bus_width;
    uint32_t indirect_len;
    uint32_t wr_data;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t rd_data;
    uint32_t opt_ctl;
    uint32_t logic_func_mode;
    uint32_t lock_mode;
    int device_flag;
    uint32_t log_num;                           /* The number of write record registers is 64 at most */
    uint32_t log_index[BSP_KEY_DEVICE_NUM_MAX]; /* Write record register address = (base_addr & 0xFFFF) | (len << 16) */
    device_status_check_t status_check;
} indirect_dev_device_t;

#endif /* __WB_INDIRECT_DEV_H__ */
