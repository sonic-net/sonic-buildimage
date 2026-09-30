/*
 * A header definition for wb_slot_driver driver
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

#ifndef _WB_SLOT_DRIVER_H_
#define _WB_SLOT_DRIVER_H_

/**
 * dfd_get_slot_status_str - Gets the subcard status
 * @slot_index: Number of the sub-card, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_slot_status_str(unsigned int slot_index, char *buf, size_t count);

/**
 * dfd_get_slot_info - Obtain the subcard information
 * @slot_index: Number of the sub-card, starting from 1
 * @cmd: Card information type, sub-card name :2, sub-card serial number :3, sub-card hardware version number :5
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_slot_info(unsigned int slot_index, uint8_t cmd, char *buf, size_t count);

/**
 * dfd_get_slot_power_status_str - get power status of slot
 * @slot_index: Number of the sub-card, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_slot_power_status_str(unsigned int slot_index, char *buf, size_t count);

/**
 * dfd_set_slot_power_status_str - set power status of slot
 * @slot_index: Number of the sub-card, starting from 1
 * @value: Power status of slot
 * return: Success: 0 is returned
 *       : Failed: A negative value is returned
 */
int dfd_set_slot_power_status_str(unsigned int slot_index, int value);

#endif /* _WB_SLOT_DRIVER_H_ */
