/*
 * A header definition for wb_cpld_driver driver
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

#ifndef _WB_CPLD_DRIVER_H_
#define _WB_CPLD_DRIVER_H_

/**
 * dfd_get_cpld_name - Obtain the CPLD name
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_name(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_get_cpld_type - Obtain the CPLD model
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_type(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_get_cpld_fw_version - Obtain the CPLD firmware version
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_fw_version(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_get_cpld_hw_version - Obtain the hardware version of the CPLD
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_hw_version(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count);

/**
 * dfd_set_cpld_testreg - Set the CPLD test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @cpld_index:The number of the CPLD starts from 0
 * @value: Writes the value of the test register
 * return: Success:0
 *       :Failed: A negative value is returned
 */
int dfd_set_cpld_testreg(uint8_t main_dev_id, unsigned int cpld_index, int value);

/**
 * dfd_get_cpld_testreg - Read the CPLD test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @cpld_index:The number of the CPLD starts from 0
 * @value: Read the test register value
 * return: Success:0
 *       :Failed: A negative value is returned
 */
int dfd_get_cpld_testreg(uint8_t main_dev_id, unsigned int cpld_index, int *value);

/**
 * dfd_get_cpld_testreg_str - Read the CPLD test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @cpld_index: The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_testreg_str(uint8_t main_dev_id, unsigned int cpld_index,
            char *buf, size_t count);

#endif /* _WB_CPLD_DRIVER_H_ */
