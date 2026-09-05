/*
 * A header definition for wb_system_driver driver
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

#ifndef _SYSTEM_DRIVER_H_
#define _SYSTEM_DRIVER_H_

typedef enum module_pwr_status_e {
    MODULE_POWER_OFF = 0,
    MODULE_POWER_ON,
} module_pwr_status_t;

ssize_t dfd_system_get_system_value(unsigned int type, int *value);
ssize_t dfd_system_set_system_value(unsigned int type, int value);
ssize_t dfd_system_get_port_power_status(unsigned int type, char *buf, size_t count);

#endif /* _SYSTEM_DRIVER_H_ */
