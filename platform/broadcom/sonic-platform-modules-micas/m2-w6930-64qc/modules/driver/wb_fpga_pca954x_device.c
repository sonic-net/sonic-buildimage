/*
 * An wb_fpga_pca954x_device driver for fpga pca954x device function
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

#include <linux/module.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <fpga_i2c.h>

static int g_wb_fpga_pca954x_device_debug = 0;
static int g_wb_fpga_pca954x_device_error = 0;

module_param(g_wb_fpga_pca954x_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_fpga_pca954x_device_error, int, S_IRUGO | S_IWUSR);

#define WB_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_fpga_pca954x_device_debug) { \
        printk(KERN_INFO "[WB_FPGA_PCA954X_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_FPGA_PCA954X_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_fpga_pca954x_device_error) { \
        printk(KERN_ERR "[WB_FPGA_PCA954X_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static fpga_pca954x_device_t fpga_pca954x_device_data0 = {
    .i2c_bus = 2,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 74,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data1 = {
    .i2c_bus = 74,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 75,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data2 = {
    .i2c_bus = 199,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 200,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data3 = {
    .i2c_bus = 200,
    .i2c_addr = 0x76,
    .pca9548_base_nr = 83,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data4 = {
    .i2c_bus = 3,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 91,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data5 = {
    .i2c_bus = 91,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 92,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data6 = {
    .i2c_bus = 4,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 100,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data7 = {
    .i2c_bus = 100,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 101,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data8 = {
    .i2c_bus = 5,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 109,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data9 = {
    .i2c_bus = 6,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 117,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data10 = {
    .i2c_bus = 7,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 125,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data11 = {
    .i2c_bus = 8,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 133,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data12 = {
    .i2c_bus = 9,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 137,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data13 = {
    .i2c_bus = 10,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 141,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data14 = {
    .i2c_bus = 11,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 145,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data15 = {
    .i2c_bus = 12,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 149,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data16 = {
    .i2c_bus = 13,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 153,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data17 = {
    .i2c_bus = 14,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 157,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data18 = {
    .i2c_bus = 15,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 161,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data19 = {
    .i2c_bus = 16,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 165,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data20 = {
    .i2c_bus = 17,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 169,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data21 = {
    .i2c_bus = 18,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 173,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data22 = {
    .i2c_bus = 19,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 177,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data23 = {
    .i2c_bus = 20,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 181,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data24 = {
    .i2c_bus = 21,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 185,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data25 = {
    .i2c_bus = 22,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 189,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data26 = {
    .i2c_bus = 23,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 193,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data27 = {
    .i2c_bus = 24,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 135,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data28 = {
    .i2c_bus = 25,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 136,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data29 = {
    .i2c_bus = 26,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 139,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data30 = {
    .i2c_bus = 27,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 140,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data31 = {
    .i2c_bus = 28,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 143,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data32 = {
    .i2c_bus = 29,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 144,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data33 = {
    .i2c_bus = 30,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 147,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data34 = {
    .i2c_bus = 31,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 148,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data35 = {
    .i2c_bus = 32,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 151,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data36 = {
    .i2c_bus = 33,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 152,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data37 = {
    .i2c_bus = 34,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 155,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data38 = {
    .i2c_bus = 35,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 156,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data39 = {
    .i2c_bus = 36,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 159,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data40 = {
    .i2c_bus = 37,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 160,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data41 = {
    .i2c_bus = 38,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 163,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data42 = {
    .i2c_bus = 39,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 164,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data43 = {
    .i2c_bus = 40,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 167,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data44 = {
    .i2c_bus = 41,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 168,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data45 = {
    .i2c_bus = 42,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 171,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data46 = {
    .i2c_bus = 43,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 172,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data47 = {
    .i2c_bus = 44,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 175,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data48 = {
    .i2c_bus = 45,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 176,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data49 = {
    .i2c_bus = 46,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 179,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data50 = {
    .i2c_bus = 47,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 180,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data51 = {
    .i2c_bus = 48,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 183,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data52 = {
    .i2c_bus = 49,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 184,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data53 = {
    .i2c_bus = 50,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 187,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data54 = {
    .i2c_bus = 51,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 188,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data55 = {
    .i2c_bus = 52,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 191,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data56 = {
    .i2c_bus = 53,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 192,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data57 = {
    .i2c_bus = 54,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 195,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data58 = {
    .i2c_bus = 55,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 196,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data59 = {
    .i2c_bus = 56,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 197,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data60 = {
    .i2c_bus = 57,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 198,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

struct i2c_board_info fpga_pca954x_device_info[] = {
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data0,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data1,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data2,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data3,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data4,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data5,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data6,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data7,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data8,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data9,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data10,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data11,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data12,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data13,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data14,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data15,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data16,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data17,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data18,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data19,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data20,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data21,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data22,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data23,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data24,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data25,
    },
    {
        .type = "wb_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data26,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data27,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data28,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data29,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data30,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data31,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data32,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data33,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data34,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data35,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data36,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data37,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data38,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data39,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data40,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data41,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data42,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data43,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data44,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data45,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data46,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data47,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data48,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data49,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data50,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data51,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data52,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data53,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data54,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data55,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data56,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data57,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data58,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data59,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data60,
    },
};

static int __init wb_fpga_pca954x_device_init(void)
{
    int i;
    struct i2c_adapter *adap;
    struct i2c_client *client;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    WB_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(fpga_pca954x_device_info); i++) {
        fpga_pca954x_device_data = fpga_pca954x_device_info[i].platform_data;
        fpga_pca954x_device_info[i].addr = fpga_pca954x_device_data->i2c_addr;
        adap = i2c_get_adapter(fpga_pca954x_device_data->i2c_bus);
        if (adap == NULL) {
            fpga_pca954x_device_data->client = NULL;
            printk(KERN_ERR "get i2c bus %d adapter fail.\n", fpga_pca954x_device_data->i2c_bus);
            continue;
        }
        client = i2c_new_client_device(adap, &fpga_pca954x_device_info[i]);
        if (!client) {
            fpga_pca954x_device_data->client = NULL;
            printk(KERN_ERR "Failed to register fpga pca954x device %d at bus %d!\n",
                fpga_pca954x_device_data->i2c_addr, fpga_pca954x_device_data->i2c_bus);
        } else {
            fpga_pca954x_device_data->client = client;
        }
        i2c_put_adapter(adap);
    }
    return 0;
}

static void __exit wb_fpga_pca954x_device_exit(void)
{
    int i;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    WB_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(fpga_pca954x_device_info) - 1; i >= 0; i--) {
        fpga_pca954x_device_data = fpga_pca954x_device_info[i].platform_data;
        if (fpga_pca954x_device_data->client) {
            i2c_unregister_device(fpga_pca954x_device_data->client);
            fpga_pca954x_device_data->client = NULL;
        }
    }
}

module_init(wb_fpga_pca954x_device_init);
module_exit(wb_fpga_pca954x_device_exit);
MODULE_DESCRIPTION("FPGA PCA954X Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
