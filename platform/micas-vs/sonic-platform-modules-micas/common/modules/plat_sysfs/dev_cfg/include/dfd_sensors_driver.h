/*
 * A header definition for dfd_sensor_driver driver
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

#ifndef _DFD_SENSORS_DRIVER_H_
#define _DFD_SENSORS_DRIVER_H_

ssize_t dfd_get_temp_info(uint8_t main_dev_id, uint8_t dev_index,
            uint8_t temp_index, uint8_t temp_attr, char *buf);

ssize_t dfd_get_voltage_info(uint8_t main_dev_id, uint8_t dev_index,
            uint8_t in_index, uint8_t in_attr, char *buf);

#endif /* _DFD_SENSORS_DRIVER_H_ */
