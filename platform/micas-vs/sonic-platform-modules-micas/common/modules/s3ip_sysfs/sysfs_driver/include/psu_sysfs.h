/*
 * A header definition for psu_sysfs driver
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

#ifndef _PSU_SYSFS_H_
#define _PSU_SYSFS_H_

struct s3ip_sysfs_psu_drivers_s {
    int (*get_psu_number)(void);
    int (*get_psu_temp_number)(unsigned int psu_index);
    ssize_t (*get_psu_model_name)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_vendor)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_date)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_hw_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_alarm)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_serial_number)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_part_number)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_hardware_version)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_type)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_curr)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_vol)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_power)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_curr)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_vol)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_power)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_max_power)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_present_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_status_pmbus)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_speed)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_ratio)(unsigned int psu_index, char *buf, size_t count);
    int (*set_psu_fan_ratio)(unsigned int psu_index, int ratio);
    ssize_t (*get_psu_fan_direction)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_led_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_speed_cal)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_alias)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_type)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_value)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_attr_threshold)(unsigned int psu_index, unsigned int type,  char *buf, size_t count);
    int (*get_psu_eeprom_size)(unsigned int psu_index);
    ssize_t (*read_psu_eeprom_data)(unsigned int psu_index, char *buf, loff_t offset, size_t count);
    ssize_t (*get_psu_blackbox_info)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_pmbus_info)(unsigned int psu_index, char *buf, size_t count);
    int (*clear_psu_blackbox)(unsigned int psu_index, uint8_t value);
};

extern int s3ip_sysfs_psu_drivers_register(struct s3ip_sysfs_psu_drivers_s *drv);
extern void s3ip_sysfs_psu_drivers_unregister(void);
#define SINGLE_PSU_PRESENT_DEBUG_FILE       "/etc/sonic/.present_psu_%d"

#endif /*_PSU_SYSFS_H_ */
