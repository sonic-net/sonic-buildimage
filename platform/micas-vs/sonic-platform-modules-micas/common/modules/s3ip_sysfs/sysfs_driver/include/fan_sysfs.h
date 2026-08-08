/*
 * A header definition for fan_sysfs driver
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

#ifndef _FAN_SYSFS_H_
#define _FAN_SYSFS_H_

struct s3ip_sysfs_fan_drivers_s {
    int (*get_fan_number)(void);
    int (*get_fan_motor_number)(unsigned int fan_index);
    ssize_t (*get_fan_model_name)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_vendor)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_serial_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_part_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_hardware_version)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_status)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_present)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_led_status)(unsigned int fan_index, char *buf, size_t count);
    int (*set_fan_led_status)(unsigned int fan_index, int status);
    ssize_t (*get_fan_direction)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_status)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_tolerance)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_target)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_max)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_min)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_ratio)(unsigned int fan_index, char *buf, size_t count);
    int (*set_fan_ratio)(unsigned int fan_index, int ratio);
};

extern int s3ip_sysfs_fan_drivers_register(struct s3ip_sysfs_fan_drivers_s *drv);
extern void s3ip_sysfs_fan_drivers_unregister(void);
#define SINGLE_FAN_STATUS_DEBUG_FILE       "/etc/sonic/.status_fan_%d"
#define FAN_ABSENT_STR              "0\n"
#define FAN_OK_STR                  "1\n"
#define FAN_NOTOK_STR               "2\n"
#endif /*_FAN_SYSFS_H_ */
