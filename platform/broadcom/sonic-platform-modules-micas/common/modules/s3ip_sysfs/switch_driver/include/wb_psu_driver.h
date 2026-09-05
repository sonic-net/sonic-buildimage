/*
 * A header definition for wb_psu_driver driver
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

#ifndef _WB_PSU_DRIVER_H_
#define _WB_PSU_DRIVER_H_

/**
 * dfd_get_psu_info - Get Power Information
 * @index: Number of the power supply, starting from 1
 * @cmd: Power supply information Type, power supply name :2, power supply serial number :3, power supply hardware version :5
 * @buf: Receive buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_psu_info(unsigned int psu_index, uint8_t cmd, char *buf, size_t count);

/**
 * dfd_get_psu_present_status_str - Get Power status
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_present_status_str(unsigned int psu_index, char *buf, size_t count);

/**
 * dfd_get_psu_out_status_str - Get the output power status
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_out_status_str(unsigned int psu_index, char *buf, size_t count);

/**
 * dfd_get_psu_status_pmbus_str - Gets the value on the pmbus register of the power supply
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_status_pmbus_str(unsigned int psu_index, char *buf, size_t count);

/**
 * dfd_get_psu_in_status_str - Get the input power status
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_in_status_str(unsigned int psu_index, char *buf, size_t count);

/**
 * dfd_get_psu_input_type - Get the power input type
 * @index: Number of the power supply, starting from 1
 * @buf: Receive buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_psu_input_type(unsigned int psu_index, char *buf, size_t count);

/**
 * dfd_get_psu_alarm_status - Get power PMBUS WORD STATUS status
 * @index: Number of the power supply, starting from 1
 * return: Success:return psu output status
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_psu_alarm_status(unsigned int psu_index, char *buf, size_t count);

/**
 * dfd_get_psu_fan_ratio_str - Gets the target fan rotation rate
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_fan_ratio_str(unsigned int psu_index, char *buf, size_t count);
ssize_t dfd_set_psu_fan_ratio_str(unsigned int psu_index, int pwm);
/**
 * dfd_get_psu_pmbus_status - Get power Status Word
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_pmbus_status(unsigned int psu_index, char *buf, size_t count);
/**
 * dfd_get_psu_present_status - Obtain the power supply status
 * @index: Number of the power supply, starting from 1
 * return: 0:Not in the position
 *         1:position
 *       : Negative value - Read failed
 */
int dfd_get_psu_present_status(unsigned int psu_index);

ssize_t dfd_get_psu_threshold_str(unsigned int psu_index, unsigned int type, char *buf, size_t count);

ssize_t dfd_get_psu_hw_status_str(unsigned int psu_index, char *buf, size_t count);

ssize_t dfd_get_psu_blackbox(unsigned int psu_index, char *buf, size_t count);
ssize_t dfd_get_psu_pmbus(unsigned int psu_index, char *buf, size_t count);
int dfd_clear_psu_blackbox(unsigned int psu_index, uint8_t value);
#endif /* _WB_PSU_DRIVER_H_ */
