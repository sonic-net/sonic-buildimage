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
 *	Platform CPLDMUX defines/structures header file
 */

#ifndef __PDDF_CPLDMUX_DEFS_H__
#define __PDDF_CPLDMUX_DEFS_H__

#include <linux/i2c-mux.h>

#define MAX_CPLDMUX_CHAN 64

typedef struct CPLDMUX_CHAN_DATA
{
    int chan_num;
    char chan_device[128]; /* Could be multiple devices, mentioned as " " seperated array */
    int cpld_devaddr;
    int cpld_offset;
    int cpld_sel;
    int cpld_desel;
}PDDF_CPLDMUX_CHAN_DATA;

/* CPLDMUX DATA - DATA FOR CPLDMUX CLIENT*/
typedef struct CPLDMUX_DATA
{
    int base_chan;
    int num_chan;
    int chan_cache;
    uint32_t cpld_addr;
    char cpld_name[32];
    PDDF_CPLDMUX_CHAN_DATA chan_data[MAX_CPLDMUX_CHAN];
}PDDF_CPLDMUX_DATA;

typedef enum
{
   CPLD_MUX,
   MULTIFPGAPCI_MUX,
}PDDF_CPLDMUX_DEV_TYPE;

typedef struct CPLDMUX_PDATA
{
    int parent_bus;
    int base_chan;
    int num_chan;
    int chan_cache;
    PDDF_CPLDMUX_DEV_TYPE dev_type;
    struct i2c_client *cpld;
    struct pci_dev
        *fpga_pci_dev; // identifies the FPGA when dev_type is MULTIFPGAPCI_MUX
    PDDF_CPLDMUX_CHAN_DATA *chan_data;
}PDDF_CPLDMUX_PDATA;

typedef struct CPLDMUX_PRIV_DATA{
    struct i2c_adapter *parent;
    uint32_t last_chan;  /* Will be used in case channel caching is enabled */
    PDDF_CPLDMUX_PDATA data;
}PDDF_CPLDMUX_PRIV_DATA;

typedef struct pldmux_ops_t
{
    int (*select)(struct i2c_mux_core *muxc, uint32_t chan);
    int (*deselect)(struct i2c_mux_core *muxc, uint32_t chan);
}PDDF_CPLDMUX_OPS;

int pddf_cpldmux_select_default(struct i2c_mux_core *muxc, uint32_t chan);
int pddf_cpldmux_deselect_default(struct i2c_mux_core *muxc, uint32_t chan);

extern int board_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int board_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

#endif
