/*
 * A header definition for dfd_fan_driver driver
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

#ifndef _DFD_FAN_DRIVER_H_
#define _DFD_FAN_DRIVER_H_

ssize_t dfd_get_fan_speed(unsigned int fan_index, unsigned int motor_index,unsigned int *speed);

int dfd_set_fan_pwm(unsigned int fan_index, unsigned int motor_index, int pwm);

int dfd_get_fan_pwm(unsigned int fan_index, unsigned int motor_index, int *pwm);

int dfd_get_fan_present_status(unsigned int fan_index);

int dfd_get_fan_roll_status(unsigned int fan_index, unsigned int motor_index);

int dfd_get_fan_speed_level(unsigned int fan_index, unsigned int motor_index, int *level);

int dfd_set_fan_speed_level(unsigned int fan_index, unsigned int motor_index, int level);

#endif /* _DFD_FAN_DRIVER_H_ */
