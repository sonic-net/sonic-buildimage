/*
 * A header definition for curr_sensor_sysfs driver
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

#ifndef _CURR_SENSOR_SYSFS_H_
#define _CURR_SENSOR_SYSFS_H_

struct s3ip_sysfs_curr_sensor_drivers_s {
    int (*get_main_board_curr_number)(void);
    ssize_t (*get_main_board_curr_alias)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_type)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_max)(unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_max)(unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_min)(unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_min)(unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_value)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_monitor_flag)(unsigned int curr_index, char *buf, size_t count);
};

extern int s3ip_sysfs_curr_sensor_drivers_register(struct s3ip_sysfs_curr_sensor_drivers_s *drv);
extern void s3ip_sysfs_curr_sensor_drivers_unregister(void);
#endif /*_CURR_SENSOR_SYSFS_H_ */
