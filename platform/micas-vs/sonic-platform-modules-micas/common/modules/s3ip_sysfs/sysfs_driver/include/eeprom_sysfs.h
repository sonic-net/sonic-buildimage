/*
 * A header definition for eeprom_sysfs driver
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

#ifndef _EEPROM_SYSFS_H_
#define _EEPROM_SYSFS_H_

struct s3ip_sysfs_eeprom_drivers_s {
    int (*get_eeprom_number)(void);
    ssize_t (*get_eeprom_alias)(unsigned int e2_index, char *buf, size_t count);
    ssize_t (*get_eeprom_tag)(unsigned int e2_index, char *buf, size_t count);
    ssize_t (*get_eeprom_type)(unsigned int e2_index, char *buf, size_t count);
    int (*get_eeprom_size)(unsigned int e2_index);
    ssize_t (*read_eeprom_data)(unsigned int e2_index, char *buf, loff_t offset, size_t count);
    ssize_t (*write_eeprom_data)(unsigned int e2_index, char *buf, loff_t offset, size_t count);
};

extern int s3ip_sysfs_eeprom_drivers_register(struct s3ip_sysfs_eeprom_drivers_s *drv);
extern void s3ip_sysfs_eeprom_drivers_unregister(void);
#endif /*_EEPROM_SYSFS_H_ */
