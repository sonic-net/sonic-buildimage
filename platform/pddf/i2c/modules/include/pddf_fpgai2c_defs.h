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
 * Description:
 *	Platform FPGA I2C defines/structures header file
 */

#ifndef __PDDF_FPGAI2C_DEFS_H__
#define __PDDF_FPGAI2C_DEFS_H__

/* FPGAI2C DATA - DATA FOR I2C FPGA CLIENT READ/WRITE*/
typedef struct FPGAI2C_DATA
{
    struct mutex fpga_lock;
    uint16_t reg_addr;
}PDDF_FPGAI2C_DATA;


#endif //__PDDF_FPGAI2C_DEFS_H__
