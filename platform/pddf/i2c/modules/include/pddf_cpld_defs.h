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
 *	Platform CPLD defines/structures header file
 */

#ifndef __PDDF_CPLD_DEFS_H__
#define __PDDF_CPLD_DEFS_H__

#define CPLD_CLIENT_NAME_LEN 32
/* CPLD DATA - DATA FOR CPLD CLIENT READ/WRITE*/
typedef struct CPLD_DATA
{
    struct mutex cpld_lock;
    uint16_t reg_addr;
}PDDF_CPLD_DATA;


#endif
