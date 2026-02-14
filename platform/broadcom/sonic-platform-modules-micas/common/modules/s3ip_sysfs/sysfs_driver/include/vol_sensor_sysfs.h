/*
 * A header definition for vol_sensor_sysfs driver
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

#ifndef _VOL_SENSOR_SYSFS_H_
#define _VOL_SENSOR_SYSFS_H_

struct s3ip_sysfs_vol_sensor_drivers_s {
    int (*get_main_board_vol_number)(void);
    ssize_t (*get_main_board_vol_alias)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_type)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_max)(unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_max)(unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_min)(unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_min)(unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_range)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_nominal_value)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_value)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_monitor_flag)(unsigned int vol_index, char *buf, size_t count);
};

extern int s3ip_sysfs_vol_sensor_drivers_register(struct s3ip_sysfs_vol_sensor_drivers_s *drv);
extern void s3ip_sysfs_vol_sensor_drivers_unregister(void);
#endif /*_VOL_SENSOR_SYSFS_H_ */
