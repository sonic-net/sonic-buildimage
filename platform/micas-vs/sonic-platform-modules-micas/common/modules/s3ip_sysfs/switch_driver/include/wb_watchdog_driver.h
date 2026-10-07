/*
 * A header definition for wb_watchdog_driver driver
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

#ifndef _WB_WATCHDOG_DRIVER_H_
#define _WB_WATCHDOG_DRIVER_H_

/**
 * dfd_get_watchdog_info - Get watchdog information
 * @type: Watchdog information type
 * @buf: Receive buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_watchdog_info(uint8_t type, char *buf, size_t count);

ssize_t dfd_watchdog_get_status_str(char *buf, size_t count);
ssize_t dfd_watchdog_get_status(char *buf, size_t count);
ssize_t dfd_watchdog_set_status(int value);

#endif /* _WB_WATCHDOG_DRIVER_H_ */
