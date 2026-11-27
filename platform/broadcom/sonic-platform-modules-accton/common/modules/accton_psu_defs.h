/*
 * Copyright 2022 Accton Technology Corporation.
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
 *
 * Description:
 *  Platform PSU defines/structures header file
 */

#ifndef ACCTON_PSU_DEFS_H
#define ACCTON_PSU_DEFS_H

typedef struct PSU_STATUS_ENTRY
{
    int (*get_presence)(void *client);
    int (*get_powergood)(void *client);
} PSU_STATUS_ENTRY;

#endif /* ACCTON_PSU_DEFS_H */