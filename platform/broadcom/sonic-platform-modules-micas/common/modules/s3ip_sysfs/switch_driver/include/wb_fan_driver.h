/*
 * A header definition for wb_fan_driver driver
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

#ifndef _WB_FAN_DRIVER_H_
#define _WB_FAN_DRIVER_H_

/**
 * dfd_get_fan_status_str - Obtaining fan status
 * @index: Number of the fan, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_fan_status_str(unsigned int fan_index, char *buf, size_t count);

/**
 * dfd_get_fan_present_str - Obtaining fan present status
 * @index: Number of the fan, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_fan_present_str(unsigned int fan_index, char *buf, size_t count);

/**
 * dfd_get_fan_info - Obtaining Fan Information
 * @index: Number of the fan, starting from 1
 * @cmd: Fan information type, fan name :2, fan serial number :3, fan hardware version :5
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */

/**
 * dfd_get_fan_motor_status_str - Obtain the fan motor status
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_status_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

ssize_t dfd_get_fan_info(unsigned int fan_index, uint8_t cmd, char *buf, size_t count);

/**
 * dfd_get_fan_speed - Obtain the fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @speed: Speed value
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_get_fan_speed(unsigned int fan_index, unsigned int motor_index,unsigned int *speed);

/**
 * dfd_get_fan_speed_str - Obtain the fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_speed_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

/**
 * dfd_set_fan_pwm - Set the fan speed duty cycle
 * @fan_index: Number of the fan, starting from 1
 * @pwm:   Duty cycle
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_fan_pwm(unsigned int fan_index, int pwm);

/**
 * dfd_get_fan_pwm - Obtain the fan speed duty cycle
 * @fan_index: Number of the fan, starting from 1
 * @pwm:  Duty cycle
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_get_fan_pwm(unsigned int fan_index, int *pwm);

/**
 * dfd_get_fan_pwm_str - Obtain the fan speed duty cycle
 * @fan_index: Number of the fan, starting from 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_pwm_str(unsigned int fan_index, char *buf, size_t count);

/**
 * dfd_get_fan_motor_speed_tolerance_str - Obtain the fan speed tolerance
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_speed_tolerance_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

/**
 * dfd_get_fan_speed_target - Obtain the standard fan speed
 * @fan_index
 * @motor_index
 * @value Standard speed value
 * @returns: Success :0
 *         : Failed: A negative value is returned
 */
int dfd_get_fan_speed_target(unsigned int fan_index, unsigned int motor_index, int *value);

/**
 * dfd_get_fan_motor_speed_target_str - Obtain the standard fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_speed_target_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

/**
 * dfd_get_fan_direction_str - Obtain the fan air duct type
 * @fan_index: Number of the fan, starting from 1
 * @buf :Duct type receives buf
 * @count :Duct type receives buf length
 * @returns: Succeeded: Air duct type String length
 *           Failed: A negative value is returned
 */
ssize_t dfd_get_fan_direction_str(unsigned int fan_index, char *buf, size_t count);

/**
 * dfd_get_fan_motor_speed_max_str - Obtain the standard fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buff
 * @count: Receive buf length
 * return: Success :0
 *       :Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_speed_max_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

/**
 * dfd_get_fan_motor_speed_min_str - Obtain the standard fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Receive buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_speed_min_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count);

/**
 * dfd_get_fan_present_status - Obtain the fan status
 * @index: Number of the fan, starting from 1
 * return: 0:ABSENT
 *         1:PRESENT
 *       : Negative value - Read failed
 */
int dfd_get_fan_present_status(unsigned int fan_index);

#endif /* _WB_FAN_DRIVER_H_ */
