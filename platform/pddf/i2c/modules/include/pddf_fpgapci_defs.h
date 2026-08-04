/*
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
 *	Platform FPGAPCI defines/structures header file
 */

#ifndef __PDDF_FPGAPCI_DEFS_H__
#define __PDDF_FPGAPCI_DEFS_H__


// FPGA
typedef struct
{
  uint32_t vendor_id;
  uint32_t device_id;
  uint32_t virt_bus;
  uint32_t virt_i2c_ch;
  uint32_t data_base_offset;
  uint32_t data_size;
  uint32_t i2c_ch_base_offset;
  uint32_t i2c_ch_size;
} FPGA_OPS_DATA;

/*****************************************
 *  kobj list
 *****************************************/

struct kobject *fpgapci_kobj=NULL;

/*****************************************
 * Static Data provided from user
 * space JSON data file
 *****************************************/
#define NAME_SIZE 32




#endif
