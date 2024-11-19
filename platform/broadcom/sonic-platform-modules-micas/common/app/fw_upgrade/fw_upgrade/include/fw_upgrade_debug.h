/*
 * A header definition for fw_upgrade_debug driver
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

#ifndef __FW_UPGRADE_DEBUG_H__
#define __FW_UPGRADE_DEBUG_H__

#include <string.h>

#define DEBUG_INFO_LEN  20
#define DEBUG_FILE      "/tmp/.fw_upgrade_debug"
#define DEBUG_ON_ALL    "3"
#define DEBUG_ON_KERN   "2"
#define DEBUG_ON_INFO   "1"
#define DEBUG_OFF_INFO  "0"

#define mem_clear(data, size) memset((data), 0, (size))

enum debug_s {
    DEBUG_OFF = 0,
    DEBUG_APP_ON,
    DEBUG_KERN_ON,
    DEBUG_ALL_ON,
    DEBUG_IGNORE,
};

extern int fw_upgrade_debug(void);

#endif /* End of __FW_UPGRADE_DEBUG_H__ */
