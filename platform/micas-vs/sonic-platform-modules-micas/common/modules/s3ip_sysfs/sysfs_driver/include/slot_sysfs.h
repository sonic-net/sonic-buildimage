/*
 * A header definition for slot_sysfs driver
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

#ifndef _SLOT_SYSFS_H_
#define _SLOT_SYSFS_H_

struct s3ip_sysfs_slot_drivers_s {
    int (*get_slot_number)(void);
    int (*get_slot_temp_number)(unsigned int slot_index);
    int (*get_slot_vol_number)(unsigned int slot_index);
    int (*get_slot_curr_number)(unsigned int slot_index);
    int (*get_slot_cpld_number)(unsigned int slot_index);
    int (*get_slot_fpga_number)(unsigned int slot_index);
    ssize_t (*get_slot_model_name)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_vendor)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_serial_number)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_part_number)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_hardware_version)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_status)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_led_status)(unsigned int slot_index, char *buf, size_t count);
    int (*set_slot_led_status)(unsigned int slot_index, int status);
    ssize_t (*get_slot_power_status)(unsigned int slot_index, char *buf, size_t count);
    int (*set_slot_power_status)(unsigned int slot_index, int status);
    ssize_t (*get_slot_temp_alias)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_slot_temp_type)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_slot_temp_max)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_slot_temp_max)(unsigned int slot_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_slot_temp_min)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_slot_temp_min)(unsigned int slot_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_slot_temp_value)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_alias)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_type)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_max)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    int (*set_slot_vol_max)(unsigned int slot_index, unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_slot_vol_min)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    int (*set_slot_vol_min)(unsigned int slot_index, unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_slot_vol_range)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_nominal_value)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_value)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_curr_alias)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_slot_curr_type)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_slot_curr_max)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    int (*set_slot_curr_max)(unsigned int slot_index, unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_slot_curr_min)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    int (*set_slot_curr_min)(unsigned int slot_index, unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_slot_curr_value)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_alias)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_type)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_firmware_version)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_board_version)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_test_reg)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    int (*set_slot_fpga_test_reg)(unsigned int slot_index, unsigned int fpga_index, unsigned int value);
    ssize_t (*get_slot_cpld_alias)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_slot_cpld_type)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_slot_cpld_firmware_version)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_slot_cpld_board_version)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_slot_cpld_test_reg)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    int (*set_slot_cpld_test_reg)(unsigned int slot_index, unsigned int cpld_index, unsigned int value);
};

extern int s3ip_sysfs_slot_drivers_register(struct s3ip_sysfs_slot_drivers_s *drv);
extern void s3ip_sysfs_slot_drivers_unregister(void);
#endif /*_SLOT_SYSFS_H_ */
