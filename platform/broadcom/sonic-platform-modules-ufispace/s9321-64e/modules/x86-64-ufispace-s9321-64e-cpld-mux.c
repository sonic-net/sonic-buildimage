/*
 * A i2c cpld driver for the ufispace_s9321_64e
 *
 * Copyright (C) 2017-2019 UfiSpace Technology Corporation.
 * Nonodark Huang <nonodark.huang@ufispace.com>
 *
 * Based on i2c-mux-pca954x.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
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

#include <linux/delay.h>

#include "x86-64-ufispace-s9321-64e-cpld-main.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

/* Provide specs for the cpld mux types we know about */
const struct chip_desc chips[] = {
    [cpld1] = {
        .nchans = 0,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        .id = { .manufacturer_id = I2C_DEVICE_ID_NONE },
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
    },
    [cpld2] = {
        .nchans = 32,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        .id = { .manufacturer_id = I2C_DEVICE_ID_NONE },
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
    },
    [cpld3] = {
        .nchans = 32,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        .id = { .manufacturer_id = I2C_DEVICE_ID_NONE },
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
    },
    [fpga] = {
        .nchans = 2,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        .id = { .manufacturer_id = I2C_DEVICE_ID_NONE },
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
    },
};


typedef u8 chan_map[CPLD_MAX_NCHANS];
static const chan_map chans_map[] = {
    [cpld1] = {0},
    [cpld2] = {
        // port 0     port 1        port 2        port 3
        (0b00000001), (0b00000010), (0b00000011), (0b00000100),
        // port 4     port 5        port 6        port 7
        (0b00000101), (0b00000110), (0b00000111), (0b00001000),
        // port 8     port 9        port 10       port 11
        (0b00001001), (0b00001010), (0b00001011), (0b00001100),
        // port 12    port 13       port 14       port 15
        (0b00001101), (0b00001110), (0b00001111), (0b00010000),
        // port 32    port 33       port 34       port 35
        (0b00010001), (0b00010010), (0b00010011), (0b00010100),
        // port 36    port 37       port 38       port 39
        (0b00010101), (0b00010110), (0b00010111), (0b00011000),
        // port 40    port 41       port 42       port 43
        (0b00011001), (0b00011010), (0b00011011), (0b00011100),
        // port 44    port 45       port 46       port 47
        (0b00011101), (0b00011110), (0b00011111), (0b00100000),
    },
    [cpld3] = {
        // port 16    port 17       port 18       port 19
        (0b00000001), (0b00000010), (0b00000011), (0b00000100),
        // port 20    port 21       port 22       port 23
        (0b00000101), (0b00000110), (0b00000111), (0b00001000),
        // port 24    port 25       port 26       port 27
        (0b00001001), (0b00001010), (0b00001011), (0b00001100),
        // port 28    port 29       port 30       port 31
        (0b00001101), (0b00001110), (0b00001111), (0b00010000),
        // port 48    port 49       port 50       port 51
        (0b00010001), (0b00010010), (0b00010011), (0b00010100),
        // port 52    port 53       port 54       port 55
        (0b00010101), (0b00010110), (0b00010111), (0b00011000),
        // port 56    port 57       port 58       port 59
        (0b00011001), (0b00011010), (0b00011011), (0b00011100),
        // port 60    port 61       port 62       port 63
        (0b00011101), (0b00011110), (0b00011111), (0b00100000),
    },
    [fpga] = {
        // port 64   port 65
        (0b0000001), (0b00000010),
    },
};

typedef u32 port_map[CPLD_MAX_NCHANS];
static const port_map ports_map[] = {
    [cpld1] = {-1},
    [cpld2] = {
        0 , 1 , 2 , 3,
        4 , 5 , 6 , 7,
        8 , 9 , 10, 11,
        12, 13, 14, 15,
        32, 33, 34, 35,
        36, 37, 38, 39,
        40, 41, 42, 43,
        44, 45, 46, 47,
    },
    [cpld3] = {
        16, 17, 18, 19,
        20, 21, 22, 23,
        24, 25, 26, 27,
        28, 29, 30, 31,
        48, 49, 50, 51,
        52, 53, 54, 55,
        56, 57, 58, 59,
        60, 61, 62, 63,
    },
    [fpga] = {
        64, 65,
    },
};

typedef struct  {
    u16 reg;
    u16 evt_reg;
    u8 mask;
    u8 block_status;
} port_block_map_t;

static port_block_map_t ports_block_map[] = {
    [0]=  {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0001, PORT_NONE_BLOCK},
    [1]=  {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0010, PORT_NONE_BLOCK},
    [2]=  {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0100, PORT_NONE_BLOCK},
    [3]=  {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_1000, PORT_NONE_BLOCK},
    [4]=  {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0001_0000, PORT_NONE_BLOCK},
    [5]=  {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0010_0000, PORT_NONE_BLOCK},
    [6]=  {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0100_0000, PORT_NONE_BLOCK},
    [7]=  {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_1000_0000, PORT_NONE_BLOCK},
    [8]=  {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0001, PORT_NONE_BLOCK},
    [9]=  {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0010, PORT_NONE_BLOCK},
    [10]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0100, PORT_NONE_BLOCK},
    [11]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_1000, PORT_NONE_BLOCK},
    [12]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0001_0000, PORT_NONE_BLOCK},
    [13]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0010_0000, PORT_NONE_BLOCK},
    [14]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0100_0000, PORT_NONE_BLOCK},
    [15]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_1000_0000, PORT_NONE_BLOCK},
    [16]= {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0001, PORT_NONE_BLOCK},
    [17]= {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0010, PORT_NONE_BLOCK},
    [18]= {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0100, PORT_NONE_BLOCK},
    [19]= {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_1000, PORT_NONE_BLOCK},
    [20]= {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0001_0000, PORT_NONE_BLOCK},
    [21]= {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0010_0000, PORT_NONE_BLOCK},
    [22]= {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0100_0000, PORT_NONE_BLOCK},
    [23]= {.reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_REG  , .evt_reg=CPLD_QSFPDD_PORT_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_1000_0000, PORT_NONE_BLOCK},
    [24]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0001, PORT_NONE_BLOCK},
    [25]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0010, PORT_NONE_BLOCK},
    [26]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0100, PORT_NONE_BLOCK},
    [27]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_1000, PORT_NONE_BLOCK},
    [28]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0001_0000, PORT_NONE_BLOCK},
    [29]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0010_0000, PORT_NONE_BLOCK},
    [30]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0100_0000, PORT_NONE_BLOCK},
    [31]= {.reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_REG , .evt_reg=CPLD_QSFPDD_PORT_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_1000_0000, PORT_NONE_BLOCK},
    [32]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0000_0001, PORT_NONE_BLOCK},
    [33]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0000_0010, PORT_NONE_BLOCK},
    [34]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0000_0100, PORT_NONE_BLOCK},
    [35]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0000_1000, PORT_NONE_BLOCK},
    [36]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0001_0000, PORT_NONE_BLOCK},
    [37]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0010_0000, PORT_NONE_BLOCK},
    [38]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0100_0000, PORT_NONE_BLOCK},
    [39]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_1000_0000, PORT_NONE_BLOCK},
    [40]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0000_0001, PORT_NONE_BLOCK},
    [41]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0000_0010, PORT_NONE_BLOCK},
    [42]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0000_0100, PORT_NONE_BLOCK},
    [43]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0000_1000, PORT_NONE_BLOCK},
    [44]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0001_0000, PORT_NONE_BLOCK},
    [45]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0010_0000, PORT_NONE_BLOCK},
    [46]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0100_0000, PORT_NONE_BLOCK},
    [47]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_1000_0000, PORT_NONE_BLOCK},
    [48]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0000_0001, PORT_NONE_BLOCK},
    [49]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0000_0010, PORT_NONE_BLOCK},
    [50]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0000_0100, PORT_NONE_BLOCK},
    [51]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0000_1000, PORT_NONE_BLOCK},
    [52]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0001_0000, PORT_NONE_BLOCK},
    [53]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0010_0000, PORT_NONE_BLOCK},
    [54]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_0100_0000, PORT_NONE_BLOCK},
    [55]= {.reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_32_39_48_55_STUCK_EVENT_REG, .mask=MASK_1000_0000, PORT_NONE_BLOCK},
    [56]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0000_0001, PORT_NONE_BLOCK},
    [57]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0000_0010, PORT_NONE_BLOCK},
    [58]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0000_0100, PORT_NONE_BLOCK},
    [59]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0000_1000, PORT_NONE_BLOCK},
    [60]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0001_0000, PORT_NONE_BLOCK},
    [61]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0010_0000, PORT_NONE_BLOCK},
    [62]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_0100_0000, PORT_NONE_BLOCK},
    [63]= {.reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_REG, .evt_reg=CPLD_QSFPDD_PORT_40_47_56_63_STUCK_EVENT_REG, .mask=MASK_1000_0000, PORT_NONE_BLOCK},
    [64]= {.reg=FPGA_MGMT_PORT_0_1_STUCK_REG          , .evt_reg=FPGA_MGMT_PORT_0_1_STUCK_EVENT_REG          , .mask=MASK_0000_0001, PORT_NONE_BLOCK},
    [65]= {.reg=FPGA_MGMT_PORT_0_1_STUCK_REG          , .evt_reg=FPGA_MGMT_PORT_0_1_STUCK_EVENT_REG          , .mask=MASK_0000_0010, PORT_NONE_BLOCK},

};

int port_chan_get_from_reg(u8 val, int index, int *chan, int *port)
{
    u32 i = 0;
    u32 cmp = 0;

    if(index == fpga) {
        cmp = val & ~(FPGA_LAN_PORT_RELAY_ENABLE);
    } else {
        cmp = val & ~(CPLD_I2C_ENABLE_CHN_SEL);
    }

    if(cmp != 0) {
        for(i = 0;i < NELEMS(chans_map[index]);i++) {
            if(chans_map[index][i] == cmp) {
                *chan = i;
                *port = ports_map[index][i];
                return 0;
            }
        }
    }

    *chan = -1;
    *port = -1;

    return EINVAL;
}

int mux_reg_get(struct i2c_adapter *adap, struct i2c_client *client)
{
    int ret;
	union i2c_smbus_data i2c_data;
	struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int i2c_relay_reg = CPLD_I2C_RELAY_REG;
    unsigned long stop_time;
    u32 try_times = 0;

    if(data->index == fpga) {
        i2c_relay_reg = FPGA_LAN_PORT_RELAY_REG;
    } else {
        i2c_relay_reg = CPLD_I2C_RELAY_REG;
    }

	/* Start a round of trying to claim the bus */
	stop_time = jiffies + msecs_to_jiffies(CPLD_MUX_TIMEOUT);
    do {
        try_times += 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)

        ret = __i2c_smbus_xfer(adap, client->addr, client->flags,
                    I2C_SMBUS_READ, i2c_relay_reg,
                    I2C_SMBUS_BYTE_DATA, &i2c_data);
#else
        ret = adap->algo->smbus_xfer(adap, client->addr, client->flags,
                            I2C_SMBUS_READ, i2c_relay_reg,
                            I2C_SMBUS_BYTE_DATA, &i2c_data);

#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) */
        if(ret != 0) {
            mdelay(CPLD_MUX_RETRY_WAIT);
        } else {
            break;
        }
    } while (time_before(jiffies, stop_time));

    if(ret != 0) {
        pr_info("Fail to get cpld mux. dev_index(%d) reg(0x%x) retry(%d)\n",
             data->index, i2c_relay_reg, try_times -1);
    }

	return (ret < 0) ? ret : i2c_data.byte;
}

static int _port_block_status_get(struct i2c_adapter *adap, struct i2c_client *client, int port, int is_evt)
{
    int ret;
	union i2c_smbus_data i2c_data;
    int reg = 0;
    u8 mask = MASK_ALL;
    unsigned long stop_time;
    u32 try_times = 0;

    if(port >= 0 && port <= NELEMS(ports_block_map)) {
        if(is_evt == 1) {
            reg = ports_block_map[port].evt_reg;
        } else {
            reg = ports_block_map[port].reg;
        }
        mask = ports_block_map[port].mask;
    } else {
        return 0;
    }

	/* Start a round of trying to claim the bus */
	stop_time = jiffies + msecs_to_jiffies(CPLD_MUX_TIMEOUT);
    do {
        try_times += 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)

        ret = __i2c_smbus_xfer(adap, client->addr, client->flags,
                    I2C_SMBUS_READ, reg,
                    I2C_SMBUS_BYTE_DATA, &i2c_data);
#else
        ret = adap->algo->smbus_xfer(adap, client->addr, client->flags,
                            I2C_SMBUS_READ, reg,
                            I2C_SMBUS_BYTE_DATA, &i2c_data);

#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) */
        if(ret != 0) {
            mdelay(CPLD_MUX_RETRY_WAIT);
        } else {
            break;
        }
    } while (time_before(jiffies, stop_time));

    if(ret < 0) {
        /*
         *  When the return value is negative, it means the I2C bus is abnormal and we can't
         *  retrieve the stuck status. We treat this as no block.
         */
        return PORT_NONE_BLOCK;
    } else if(_mask_shift(i2c_data.byte, mask) == 0) {
        return PORT_BLOCK;
    } else {
        return PORT_NONE_BLOCK;
    }
}

static int mux_reg_write(struct i2c_adapter *adap, struct i2c_client *client, u8 val, u32 *try_times)
{
    int ret = 0;
    union i2c_smbus_data i2c_data;
	struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int i2c_relay_reg = CPLD_I2C_RELAY_REG;
    unsigned long stop_time;
    u32 tries = 0;

    i2c_data.byte = val;
    if(data->index == fpga) {
        i2c_relay_reg = FPGA_LAN_PORT_RELAY_REG;
    } else {
        i2c_relay_reg = CPLD_I2C_RELAY_REG;
    }

	/* Start a round of trying to claim the bus */
	stop_time = jiffies + msecs_to_jiffies(CPLD_MUX_TIMEOUT);
    do {

        /*
        *  Write to mux register. Don't use i2c_transfer()/i2c_smbus_xfer()
        *  for this as they will try to lock adapter a second time
        */

        tries += 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
        ret = __i2c_smbus_xfer(adap, client->addr, client->flags,
                    I2C_SMBUS_WRITE, i2c_relay_reg,
                    I2C_SMBUS_BYTE_DATA, &i2c_data);
#else
        ret = adap->algo->smbus_xfer(adap, client->addr, client->flags,
                            I2C_SMBUS_WRITE, i2c_relay_reg,
                            I2C_SMBUS_BYTE_DATA, &i2c_data);
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) */

        if(ret != 0) {
            mdelay(CPLD_MUX_RETRY_WAIT);
        } else {
            break;
        }
    } while (time_before(jiffies, stop_time));

    * try_times = tries;
    return ret;
}

int mux_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
    struct cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    struct device *dev = &client->dev;

    u8 set_val;
    int ret = 0;
    int chan_val = 0;

    switch (data->index)
    {
        case cpld1:
        case cpld2:
        case cpld3:
            set_val = CPLD_I2C_ENABLE_CHN_SEL;
            break;
        case fpga:
            set_val = FPGA_LAN_PORT_RELAY_ENABLE;
            break;
        default:
            dev_err(dev, "Invalid device index\n");
            ret=EINVAL;
            goto exit;
    }

    if(chan >= data->chip->nchans) {
        dev_err(dev, "Invalid channel (%d)>=(%d)\n",chan, data->chip->nchans);
            ret=EINVAL;
            goto exit;
    }

    chan_val = chans_map[data->index][chan];
    set_val |= chan_val;

    /* Only select the channel if its different from the last channel */
    if (data->last_chan != set_val) {
        u32 try_times = 0;
        int port = ports_map[data->index][chan];
        ret = mux_reg_write(muxc->parent, client, set_val, &try_times);
        if(ret != 0) {
            pr_info("Fail to set cpld mux. port(%d) chan(%d) reg_val(0x%x) retry(%d)\n",
                port, chan, set_val, try_times -1);
        } else {
            if(try_times -1 > 0){
                pr_info("Success to set cpld mux. port(%d) chan(%d) reg_val(0x%x) retry(%d)\n",
                    port, chan, set_val, try_times -1);
            }

            if(_port_block_status_get(muxc->parent, client, port, 0) == PORT_BLOCK) {
                if(ports_block_map[port].block_status != PORT_BLOCK) {
                    ports_block_map[port].block_status = PORT_BLOCK;
                    pr_warn("port(%d) is blocked by CPLD/FPGA\n", port);
                }
            } else {
                if(ports_block_map[port].block_status != PORT_NONE_BLOCK) {
                    ports_block_map[port].block_status = PORT_NONE_BLOCK;
                    pr_warn("port(%d) is recovered by the CPLD/FPGA\n", port);
                }
            }
        }
        data->last_chan = set_val;
    }

exit:
    return ret;
}

int mux_deselect_mux(struct i2c_mux_core *muxc, u32 chan)
{
    struct cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    s32 idle_state;
    u32 ret = 0;

    idle_state = READ_ONCE(data->idle_state);
    if (idle_state >= 0) {
        /* Set the mux back to a predetermined channel */
        ret = mux_select_chan(muxc, idle_state);
    } else if (idle_state == MUX_IDLE_DISCONNECT) {
        u32 try_times = 0;
        int port = ports_map[data->index][chan];

        /* Deselect active channel */
        if(data->index == fpga) {
            data->last_chan = FPGA_MUX_CHN_OFF;
        } else {
            data->last_chan = CPLD_MUX_CHN_OFF;
        }
        ret = mux_reg_write(muxc->parent, client, data->last_chan, &try_times);
        if(ret != 0) {
            pr_info("Fail to close cpld mux. port(%d) chan(%d) retry(%d)\n",
                port, chan, try_times -1);

        } else {
            if(try_times -1 > 0){
                pr_info("Success to close cpld mux. port(%d) chan(%d) retry(%d)\n",
                    port, chan, try_times -1);
            }
        }
    }

    return ret;
}

ssize_t idle_state_show(struct device *dev,
                    struct device_attribute *attr,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int rv = 0;

    rv = sprintf(buf, "%d\n", READ_ONCE(data->idle_state));
    return rv;
}

ssize_t idle_state_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int val;
    int ret;

    ret = kstrtoint(buf, 0, &val);
    if (ret < 0)
        return ret;

    if (val != MUX_IDLE_AS_IS && val != MUX_IDLE_DISCONNECT &&
        (val < 0 || val >= data->chip->nchans))
        return -EINVAL;

    i2c_lock_bus(muxc->parent, I2C_LOCK_SEGMENT);

    WRITE_ONCE(data->idle_state, val);

    /*
     * Set the mux into a state consistent with the new
     * idle_state.
     */
    if (data->last_chan || val != MUX_IDLE_DISCONNECT)
        ret = mux_deselect_mux(muxc, 0);

    i2c_unlock_bus(muxc->parent, I2C_LOCK_SEGMENT);

    return ret < 0 ? ret : count;
}

int mux_init(struct device *dev)
{
    int ret = 0;
    u8 reg_val = 0;
    int num = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int i2c_relay_reg = CPLD_I2C_RELAY_REG;
    u32 chan_off = CPLD_MUX_CHN_OFF;

    data->chip = &chips[data->index];

    data->idle_state = MUX_IDLE_DISCONNECT;
    if (device_property_read_u32(dev, "idle-state", &data->idle_state)) {
        if (device_property_read_bool(dev, "i2c-mux-idle-disconnect"))
            data->idle_state = MUX_IDLE_DISCONNECT;
    }

    /*
    * Write the mux register at addr to verify
    * that the mux is in fact present. This also
    * initializes the mux to a channel
    * or disconnected state.
    */

    if(data->chip->nchans > 0){
        if(data->index == fpga) {
            i2c_relay_reg = FPGA_LAN_PORT_RELAY_REG;
            // close multiplexer channel
            chan_off = FPGA_MUX_CHN_OFF;
        } else {
            i2c_relay_reg = CPLD_I2C_RELAY_REG;
            // close multiplexer channel
            chan_off = CPLD_MUX_CHN_OFF;
            // enable mux functionality for the legacy I2C interface instead of using FPGA.
            ret = _cpld_reg_read(dev, CPLD_I2C_CONTROL_REG, MASK_ALL);
            if (ret < 0) {
                dev_err(dev, "Fail to enable mux functionality\n");
                goto exit;
            }
            reg_val = (u8)ret;
            reg_val |= (CPLD_I2C_ENABLE_BRIDGE);
            ret = _cpld_reg_write(dev, CPLD_I2C_CONTROL_REG, reg_val);
            if (ret < 0) {
                dev_err(dev, "Fail to enable mux functionality\n");
                goto exit;
            }
        }

        if (data->idle_state >= 0) {
            /* Set the mux back to a predetermined channel */
            ret = mux_select_chan(muxc, data->idle_state);
        } else {
            u32 try_times = 0;
            data->last_chan = chan_off;
            ret = mux_reg_write(muxc->parent, client, data->last_chan, &try_times);
        }

        if (ret < 0) {
            goto exit;
        }
    }

    /* Now create an adapter for each channel */
    for (num = 0; num < data->chip->nchans; num++) {
        ret = i2c_mux_add_adapter(muxc, 0, num, 0);
        if (ret)
            goto exit;
    }

    return 0;

exit:
    mux_cleanup(dev);
    return ret;
}

void mux_cleanup(struct device *dev)
{
    u8 reg_val = 0;
    int ret = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);

    if(data->index == fpga) {
        _cpld_reg_write(dev, FPGA_LAN_PORT_RELAY_REG, FPGA_MUX_CHN_OFF);

    } else {
        _cpld_reg_write(dev, CPLD_I2C_RELAY_REG, CPLD_MUX_CHN_OFF);

        ret = _cpld_reg_read(dev, CPLD_I2C_CONTROL_REG, MASK_ALL);
        reg_val = (ret >= 0) ?(u8)ret:0;
        reg_val &= ~(CPLD_I2C_ENABLE_BRIDGE);
        _cpld_reg_write(dev, CPLD_I2C_CONTROL_REG, reg_val);
    }
    i2c_mux_del_adapters(muxc);
}
