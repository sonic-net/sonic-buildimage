/*
 * Copyright 2019 Broadcom.
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
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
 *  Platform MUX defines/structures header file
 */

#ifndef __PAL_MUX_DEFS_H__
#define __PAL_MUX_DEFS_H__

/* MUX CLIENT DATA - PLATFORM DATA FOR PSU CLIENT */
typedef struct MUX_DATA
{
    int virt_bus;       // Virtual base bus number of the mux channels
}MUX_DATA;

#endif
