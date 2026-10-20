/*
 * A header definition for wb_fpga_driver driver
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

#ifndef _WB_FPGA_DRIVER_H_
#define _WB_FPGA_DRIVER_H_

/**
 * dfd_get_fpga_name -Get the FPGA name
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:Number of the FPGA, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_name(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count);

/**
 * dfd_get_fpga_type - Get FPGA model
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:Number of the FPGA, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_type(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count);

/**
 * dfd_get_fpga_fw_version - Obtain the FPGA firmware version
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:Number of the FPGA, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_fw_version(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count);

/**
 * dfd_get_fpga_hw_version - Obtain the hardware version of the FPGA
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:Number of the FPGA, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_hw_version(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count);

/**
 * dfd_set_fpga_testreg - Sets the value of the FPGA test register
 * @main_dev_id: Motherboard :0 Subcard :5
 * @fpga_index:Number of the FPGA, starting from 1
 * @value:   Writes the value of the test register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_fpga_testreg(uint8_t main_dev_id, unsigned int fpga_index, int value);

/**
 * dfd_get_fpga_testreg - Read the FPGA test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @fpga_index: Number of the FPGA, starting from 1
 * @value:   Read the test register value
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_get_fpga_testreg(uint8_t main_dev_id, unsigned int fpga_index, int *value);

/**
 * dfd_get_fpga_testreg_str - Read the FPGA test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @fpga_index:Number of the FPGA, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_testreg_str(uint8_t main_dev_id, unsigned int fpga_index,
            char *buf, size_t count);

#endif /* _WB_FPGA_DRIVER_H_ */
