/*
 * A header definition for wb_led_driver driver
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

#ifndef _WB_LED_DRIVER_H_
#define _WB_LED_DRIVER_H_

/**
 * dfd_get_led_status - Get LED and other status
 * @led_id: led lamp type
 * @led_index: led light offset
 * @buf: LED light status receives buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_led_status(uint16_t led_id, uint8_t led_index, char *buf, size_t count);

/**
 * dfd_set_led_status - Set LED light status
 * @led_id: led lamp type
 * @led_index: led light offset
 * @value: LED light status value
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_set_led_status(uint16_t led_id, uint8_t led_index, int value);

#endif /* _WB_LED_DRIVER_H_ */
