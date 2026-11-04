/*
 * A header definition for fpga_sysfs driver
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

#ifndef _FPGA_SYSFS_H_
#define _FPGA_SYSFS_H_

struct s3ip_sysfs_fpga_drivers_s {
    int (*get_main_board_fpga_number)(void);
    ssize_t (*get_main_board_fpga_alias)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_type)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_firmware_version)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_board_version)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_test_reg)(unsigned int fpga_index, char *buf, size_t count);
    int (*set_main_board_fpga_test_reg)(unsigned int fpga_index, unsigned int value);
};

extern int s3ip_sysfs_fpga_drivers_register(struct s3ip_sysfs_fpga_drivers_s *drv);
extern void s3ip_sysfs_fpga_drivers_unregister(void);
#endif /*_FPGA_SYSFS_H_ */
