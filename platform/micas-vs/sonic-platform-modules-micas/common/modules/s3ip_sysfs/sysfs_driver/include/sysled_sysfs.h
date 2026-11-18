/*
 * A header definition for sysled_sysfs driver
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

#ifndef _SYSLED_SYSFS_H_
#define _SYSLED_SYSFS_H_

struct s3ip_sysfs_sysled_drivers_s {
    ssize_t (*get_sys_led_status)(char *buf, size_t count);
    int (*set_sys_led_status)(int status);
    ssize_t (*get_bmc_led_status)(char *buf, size_t count);
    int (*set_bmc_led_status)(int status);
    ssize_t (*get_sys_fan_led_status)(char *buf, size_t count);
    int (*set_sys_fan_led_status)(int status);
    ssize_t (*get_sys_psu_led_status)(char *buf, size_t count);
    int (*set_sys_psu_led_status)(int status);
    ssize_t (*get_id_led_status)(char *buf, size_t count);
    int (*set_id_led_status)(int status);
};

extern int s3ip_sysfs_sysled_drivers_register(struct s3ip_sysfs_sysled_drivers_s *drv);
extern void s3ip_sysfs_sysled_drivers_unregister(void);
#endif /*_SYSLED_SYSFS_H_ */
