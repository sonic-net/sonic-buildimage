/*
 * An wb_fpga_i2c_bus_device driver for fpga i2c device function
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
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <fpga_i2c.h>

static int g_wb_fpga_i2c_debug = 0;
static int g_wb_fpga_i2c_error = 0;

module_param(g_wb_fpga_i2c_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_fpga_i2c_error, int, S_IRUGO | S_IWUSR);

#define WB_FPGA_I2C_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_fpga_i2c_debug) { \
        printk(KERN_INFO "[WB_FPGA_I2C][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_FPGA_I2C_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_fpga_i2c_error) { \
        printk(KERN_ERR "[WB_FPGA_I2C][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static fpga_i2c_bus_device_t fpga_i2c_bus_device_data0 = {
    .adap_nr                    = 3,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x500,
    .i2c_filter                 = 0x504,
    .i2c_stretch                = 0x508,
    .i2c_ext_9548_exits_flag    = 0x50c,
    .i2c_ext_9548_addr          = 0x510,
    .i2c_ext_9548_chan          = 0x514,
    .i2c_in_9548_chan           = 0x518,
    .i2c_slave                  = 0x51c,
    .i2c_reg                    = 0x520,
    .i2c_reg_len                = 0x530,
    .i2c_data_len               = 0x534,
    .i2c_ctrl                   = 0x538,
    .i2c_status                 = 0x53c,
    .i2c_data_buf               = 0x580,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x80,
    .i2c_reset_on               = 0x00000001,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};


static fpga_i2c_bus_device_t fpga_i2c_bus_device_data1 = {
    .adap_nr                    = 4,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x600,
    .i2c_filter                 = 0x604,
    .i2c_stretch                = 0x608,
    .i2c_ext_9548_exits_flag    = 0x60c,
    .i2c_ext_9548_addr          = 0x610,
    .i2c_ext_9548_chan          = 0x614,
    .i2c_in_9548_chan           = 0x618,
    .i2c_slave                  = 0x61c,
    .i2c_reg                    = 0x620,
    .i2c_reg_len                = 0x630,
    .i2c_data_len               = 0x634,
    .i2c_ctrl                   = 0x638,
    .i2c_status                 = 0x63c,
    .i2c_data_buf               = 0x680,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x84,
    .i2c_reset_on               = 0x00000001,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_i2c_bus_device_data2 = {
    .adap_nr                    = 5,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x700,
    .i2c_filter                 = 0x704,
    .i2c_stretch                = 0x708,
    .i2c_ext_9548_exits_flag    = 0x70c,
    .i2c_ext_9548_addr          = 0x710,
    .i2c_ext_9548_chan          = 0x714,
    .i2c_in_9548_chan           = 0x718,
    .i2c_slave                  = 0x71c,
    .i2c_reg                    = 0x720,
    .i2c_reg_len                = 0x730,
    .i2c_data_len               = 0x734,
    .i2c_ctrl                   = 0x738,
    .i2c_status                 = 0x73c,
    .i2c_data_buf               = 0x780,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x88,
    .i2c_reset_on               = 0x00000001,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_i2c_bus_device_data3 = {
    .adap_nr                    = 6,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x16800,
    .i2c_filter                 = 0x16804,
    .i2c_stretch                = 0x16808,
    .i2c_ext_9548_exits_flag    = 0x1680c,
    .i2c_ext_9548_addr          = 0x16810,
    .i2c_ext_9548_chan          = 0x16814,
    .i2c_in_9548_chan           = 0x16818,
    .i2c_slave                  = 0x1681c,
    .i2c_reg                    = 0x16820,
    .i2c_reg_len                = 0x16830,
    .i2c_data_len               = 0x16834,
    .i2c_ctrl                   = 0x16838,
    .i2c_status                 = 0x1683c,
    .i2c_data_buf               = 0x16900,
    .i2c_data_buf_len           = 256,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x8c,
    .i2c_reset_on               = 0x0000001,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_i2c_bus_device_data4 = {
    .adap_nr                    = 7,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x900,
    .i2c_filter                 = 0x904,
    .i2c_stretch                = 0x908,
    .i2c_ext_9548_exits_flag    = 0x90c,
    .i2c_ext_9548_addr          = 0x910,
    .i2c_ext_9548_chan          = 0x914,
    .i2c_in_9548_chan           = 0x918,
    .i2c_slave                  = 0x91c,
    .i2c_reg                    = 0x920,
    .i2c_reg_len                = 0x930,
    .i2c_data_len               = 0x934,
    .i2c_ctrl                   = 0x938,
    .i2c_status                 = 0x93c,
    .i2c_data_buf               = 0x980,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0xac,
    .i2c_reset_on               = 0x00000001,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_i2c_bus_device_data5 = {
    .adap_nr                    = 8,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0xf00,
    .i2c_filter                 = 0xf04,
    .i2c_stretch                = 0xf08,
    .i2c_ext_9548_exits_flag    = 0xf0c,
    .i2c_ext_9548_addr          = 0xf10,
    .i2c_ext_9548_chan          = 0xf14,
    .i2c_in_9548_chan           = 0xf18,
    .i2c_slave                  = 0xf1c,
    .i2c_reg                    = 0xf20,
    .i2c_reg_len                = 0xf30,
    .i2c_data_len               = 0xf34,
    .i2c_ctrl                   = 0xf38,
    .i2c_status                 = 0xf3c,
    .i2c_data_buf               = 0xf80,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0xac,
    .i2c_reset_on               = 0x00000002,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data0 = {
    .adap_nr                    = 9,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x2c00,
    .i2c_filter                 = 0x2c04,
    .i2c_stretch                = 0x2c08,
    .i2c_ext_9548_exits_flag    = 0x2c0c,
    .i2c_ext_9548_addr          = 0x2c10,
    .i2c_ext_9548_chan          = 0x2c14,
    .i2c_in_9548_chan           = 0x2c18,
    .i2c_slave                  = 0x2c1c,
    .i2c_reg                    = 0x2c20,
    .i2c_reg_len                = 0x2c30,
    .i2c_data_len               = 0x2c34,
    .i2c_ctrl                   = 0x2c38,
    .i2c_status                 = 0x2c3c,
    .i2c_data_buf               = 0x2c80,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000001,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data1 = {
    .adap_nr                    = 10,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x2d00,
    .i2c_filter                 = 0x2d04,
    .i2c_stretch                = 0x2d08,
    .i2c_ext_9548_exits_flag    = 0x2d0c,
    .i2c_ext_9548_addr          = 0x2d10,
    .i2c_ext_9548_chan          = 0x2d14,
    .i2c_in_9548_chan           = 0x2d18,
    .i2c_slave                  = 0x2d1c,
    .i2c_reg                    = 0x2d20,
    .i2c_reg_len                = 0x2d30,
    .i2c_data_len               = 0x2d34,
    .i2c_ctrl                   = 0x2d38,
    .i2c_status                 = 0x2d3c,
    .i2c_data_buf               = 0x2d80,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000002,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data2 = {
    .adap_nr                    = 11,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x2e00,
    .i2c_filter                 = 0x2e04,
    .i2c_stretch                = 0x2e08,
    .i2c_ext_9548_exits_flag    = 0x2e0c,
    .i2c_ext_9548_addr          = 0x2e10,
    .i2c_ext_9548_chan          = 0x2e14,
    .i2c_in_9548_chan           = 0x2e18,
    .i2c_slave                  = 0x2e1c,
    .i2c_reg                    = 0x2e20,
    .i2c_reg_len                = 0x2e30,
    .i2c_data_len               = 0x2e34,
    .i2c_ctrl                   = 0x2e38,
    .i2c_status                 = 0x2e3c,
    .i2c_data_buf               = 0x2e80,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000004,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data3 = {
    .adap_nr                    = 12,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x2f00,
    .i2c_filter                 = 0x2f04,
    .i2c_stretch                = 0x2f08,
    .i2c_ext_9548_exits_flag    = 0x2f0c,
    .i2c_ext_9548_addr          = 0x2f10,
    .i2c_ext_9548_chan          = 0x2f14,
    .i2c_in_9548_chan           = 0x2f18,
    .i2c_slave                  = 0x2f1c,
    .i2c_reg                    = 0x2f20,
    .i2c_reg_len                = 0x2f30,
    .i2c_data_len               = 0x2f34,
    .i2c_ctrl                   = 0x2f38,
    .i2c_status                 = 0x2f3c,
    .i2c_data_buf               = 0x2f80,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000008,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data4 = {
    .adap_nr                    = 13,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3000,
    .i2c_filter                 = 0x3004,
    .i2c_stretch                = 0x3008,
    .i2c_ext_9548_exits_flag    = 0x300c,
    .i2c_ext_9548_addr          = 0x3010,
    .i2c_ext_9548_chan          = 0x3014,
    .i2c_in_9548_chan           = 0x3018,
    .i2c_slave                  = 0x301c,
    .i2c_reg                    = 0x3020,
    .i2c_reg_len                = 0x3030,
    .i2c_data_len               = 0x3034,
    .i2c_ctrl                   = 0x3038,
    .i2c_status                 = 0x303c,
    .i2c_data_buf               = 0x3080,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000010,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data5 = {
    .adap_nr                    = 14,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3100,
    .i2c_filter                 = 0x3104,
    .i2c_stretch                = 0x3108,
    .i2c_ext_9548_exits_flag    = 0x310c,
    .i2c_ext_9548_addr          = 0x3110,
    .i2c_ext_9548_chan          = 0x3114,
    .i2c_in_9548_chan           = 0x3118,
    .i2c_slave                  = 0x311c,
    .i2c_reg                    = 0x3120,
    .i2c_reg_len                = 0x3130,
    .i2c_data_len               = 0x3134,
    .i2c_ctrl                   = 0x3138,
    .i2c_status                 = 0x313c,
    .i2c_data_buf               = 0x3180,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000020,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data6 = {
    .adap_nr                    = 15,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3200,
    .i2c_filter                 = 0x3204,
    .i2c_stretch                = 0x3208,
    .i2c_ext_9548_exits_flag    = 0x320c,
    .i2c_ext_9548_addr          = 0x3210,
    .i2c_ext_9548_chan          = 0x3214,
    .i2c_in_9548_chan           = 0x3218,
    .i2c_slave                  = 0x321c,
    .i2c_reg                    = 0x3220,
    .i2c_reg_len                = 0x3230,
    .i2c_data_len               = 0x3234,
    .i2c_ctrl                   = 0x3238,
    .i2c_status                 = 0x323c,
    .i2c_data_buf               = 0x3280,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000040,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data7 = {
    .adap_nr                    = 16,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3300,
    .i2c_filter                 = 0x3304,
    .i2c_stretch                = 0x3308,
    .i2c_ext_9548_exits_flag    = 0x330c,
    .i2c_ext_9548_addr          = 0x3310,
    .i2c_ext_9548_chan          = 0x3314,
    .i2c_in_9548_chan           = 0x3318,
    .i2c_slave                  = 0x331c,
    .i2c_reg                    = 0x3320,
    .i2c_reg_len                = 0x3330,
    .i2c_data_len               = 0x3334,
    .i2c_ctrl                   = 0x3338,
    .i2c_status                 = 0x333c,
    .i2c_data_buf               = 0x3380,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000080,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data8 = {
    .adap_nr                    = 17,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3400,
    .i2c_filter                 = 0x3404,
    .i2c_stretch                = 0x3408,
    .i2c_ext_9548_exits_flag    = 0x340c,
    .i2c_ext_9548_addr          = 0x3410,
    .i2c_ext_9548_chan          = 0x3414,
    .i2c_in_9548_chan           = 0x3418,
    .i2c_slave                  = 0x341c,
    .i2c_reg                    = 0x3420,
    .i2c_reg_len                = 0x3430,
    .i2c_data_len               = 0x3434,
    .i2c_ctrl                   = 0x3438,
    .i2c_status                 = 0x343c,
    .i2c_data_buf               = 0x3480,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000100,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data9 = {
    .adap_nr                    = 18,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3500,
    .i2c_filter                 = 0x3504,
    .i2c_stretch                = 0x3508,
    .i2c_ext_9548_exits_flag    = 0x350c,
    .i2c_ext_9548_addr          = 0x3510,
    .i2c_ext_9548_chan          = 0x3514,
    .i2c_in_9548_chan           = 0x3518,
    .i2c_slave                  = 0x351c,
    .i2c_reg                    = 0x3520,
    .i2c_reg_len                = 0x3530,
    .i2c_data_len               = 0x3534,
    .i2c_ctrl                   = 0x3538,
    .i2c_status                 = 0x353c,
    .i2c_data_buf               = 0x3580,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000200,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data10 = {
    .adap_nr                    = 19,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3600,
    .i2c_filter                 = 0x3604,
    .i2c_stretch                = 0x3608,
    .i2c_ext_9548_exits_flag    = 0x360c,
    .i2c_ext_9548_addr          = 0x3610,
    .i2c_ext_9548_chan          = 0x3614,
    .i2c_in_9548_chan           = 0x3618,
    .i2c_slave                  = 0x361c,
    .i2c_reg                    = 0x3620,
    .i2c_reg_len                = 0x3630,
    .i2c_data_len               = 0x3634,
    .i2c_ctrl                   = 0x3638,
    .i2c_status                 = 0x363c,
    .i2c_data_buf               = 0x3680,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000400,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data11 = {
    .adap_nr                    = 20,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3700,
    .i2c_filter                 = 0x3704,
    .i2c_stretch                = 0x3708,
    .i2c_ext_9548_exits_flag    = 0x370c,
    .i2c_ext_9548_addr          = 0x3710,
    .i2c_ext_9548_chan          = 0x3714,
    .i2c_in_9548_chan           = 0x3718,
    .i2c_slave                  = 0x371c,
    .i2c_reg                    = 0x3720,
    .i2c_reg_len                = 0x3730,
    .i2c_data_len               = 0x3734,
    .i2c_ctrl                   = 0x3738,
    .i2c_status                 = 0x373c,
    .i2c_data_buf               = 0x3780,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00000800,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data12 = {
    .adap_nr                    = 21,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3800,
    .i2c_filter                 = 0x3804,
    .i2c_stretch                = 0x3808,
    .i2c_ext_9548_exits_flag    = 0x380c,
    .i2c_ext_9548_addr          = 0x3810,
    .i2c_ext_9548_chan          = 0x3814,
    .i2c_in_9548_chan           = 0x3818,
    .i2c_slave                  = 0x381c,
    .i2c_reg                    = 0x3820,
    .i2c_reg_len                = 0x3830,
    .i2c_data_len               = 0x3834,
    .i2c_ctrl                   = 0x3838,
    .i2c_status                 = 0x383c,
    .i2c_data_buf               = 0x3880,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00001000,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data13 = {
    .adap_nr                    = 22,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3900,
    .i2c_filter                 = 0x3904,
    .i2c_stretch                = 0x3908,
    .i2c_ext_9548_exits_flag    = 0x390c,
    .i2c_ext_9548_addr          = 0x3910,
    .i2c_ext_9548_chan          = 0x3914,
    .i2c_in_9548_chan           = 0x3918,
    .i2c_slave                  = 0x391c,
    .i2c_reg                    = 0x3920,
    .i2c_reg_len                = 0x3930,
    .i2c_data_len               = 0x3934,
    .i2c_ctrl                   = 0x3938,
    .i2c_status                 = 0x393c,
    .i2c_data_buf               = 0x3980,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00002000,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data14 = {
    .adap_nr                    = 23,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3a00,
    .i2c_filter                 = 0x3a04,
    .i2c_stretch                = 0x3a08,
    .i2c_ext_9548_exits_flag    = 0x3a0c,
    .i2c_ext_9548_addr          = 0x3a10,
    .i2c_ext_9548_chan          = 0x3a14,
    .i2c_in_9548_chan           = 0x3a18,
    .i2c_slave                  = 0x3a1c,
    .i2c_reg                    = 0x3a20,
    .i2c_reg_len                = 0x3a30,
    .i2c_data_len               = 0x3a34,
    .i2c_ctrl                   = 0x3a38,
    .i2c_status                 = 0x3a3c,
    .i2c_data_buf               = 0x3a80,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00004000,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data15 = {
    .adap_nr                    = 24,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3b00,
    .i2c_filter                 = 0x3b04,
    .i2c_stretch                = 0x3b08,
    .i2c_ext_9548_exits_flag    = 0x3b0c,
    .i2c_ext_9548_addr          = 0x3b10,
    .i2c_ext_9548_chan          = 0x3b14,
    .i2c_in_9548_chan           = 0x3b18,
    .i2c_slave                  = 0x3b1c,
    .i2c_reg                    = 0x3b20,
    .i2c_reg_len                = 0x3b30,
    .i2c_data_len               = 0x3b34,
    .i2c_ctrl                   = 0x3b38,
    .i2c_status                 = 0x3b3c,
    .i2c_data_buf               = 0x3b80,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00008000,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};

static fpga_i2c_bus_device_t fpga_mac_dom_device_data16 = {
    .adap_nr                    = 25,
    .i2c_timeout                = 3000,
    .i2c_scale                  = 0x3c00,
    .i2c_filter                 = 0x3c04,
    .i2c_stretch                = 0x3c08,
    .i2c_ext_9548_exits_flag    = 0x3c0c,
    .i2c_ext_9548_addr          = 0x3c10,
    .i2c_ext_9548_chan          = 0x3c14,
    .i2c_in_9548_chan           = 0x3c18,
    .i2c_slave                  = 0x3c1c,
    .i2c_reg                    = 0x3c20,
    .i2c_reg_len                = 0x3c30,
    .i2c_data_len               = 0x3c34,
    .i2c_ctrl                   = 0x3c38,
    .i2c_status                 = 0x3c3c,
    .i2c_data_buf               = 0x3c80,
    .dev_name                   = "/dev/fpga0",
    .i2c_scale_value            = 0x4e,
    .i2c_filter_value           = 0x7c,
    .i2c_stretch_value          = 0x7c,
    .i2c_func_mode              = 2,
    .i2c_adap_reset_flag        = 1,
    .i2c_reset_addr             = 0x7c,
    .i2c_reset_on               = 0x00010000,
    .i2c_reset_off              = 0x00000000,
    .i2c_rst_delay_b            = 0,
    .i2c_rst_delay              = 1,
    .i2c_rst_delay_a            = 1,
};


static void wb_fpga_i2c_bus_device_release(struct device *dev)
{
    return;
}

static struct platform_device fpga_i2c_bus_device[] = {
    {
        .name   = "wb-fpga-i2c",
        .id = 1,
        .dev    = {
            .platform_data  = &fpga_i2c_bus_device_data0,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 2,
        .dev    = {
            .platform_data  = &fpga_i2c_bus_device_data1,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 3,
        .dev    = {
            .platform_data  = &fpga_i2c_bus_device_data2,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 4,
        .dev    = {
            .platform_data  = &fpga_i2c_bus_device_data3,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 5,
        .dev    = {
            .platform_data  = &fpga_i2c_bus_device_data4,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 6,
        .dev    = {
            .platform_data  = &fpga_i2c_bus_device_data5,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 7,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data0,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 8,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data1,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 9,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data2,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 10,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data3,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 11,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data4,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 12,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data5,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 13,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data6,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 14,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data7,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 15,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data8,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 16,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data9,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 17,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data10,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 18,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data11,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 19,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data12,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 20,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data13,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 21,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data14,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 22,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data15,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 23,
        .dev    = {
            .platform_data  = &fpga_mac_dom_device_data16,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
};

static int __init wb_fpga_i2c_bus_device_init(void)
{
    int i;
    int ret = 0;
    fpga_i2c_bus_device_t *fpga_i2c_bus_device_data;

    WB_FPGA_I2C_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(fpga_i2c_bus_device); i++) {
        fpga_i2c_bus_device_data = fpga_i2c_bus_device[i].dev.platform_data;
        ret = platform_device_register(&fpga_i2c_bus_device[i]);
        if (ret < 0) {
            fpga_i2c_bus_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-fpga-i2c.%d register failed!\n", i + 1);
        } else {
            fpga_i2c_bus_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_fpga_i2c_bus_device_exit(void)
{
    int i;
    fpga_i2c_bus_device_t *fpga_i2c_bus_device_data;

    WB_FPGA_I2C_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(fpga_i2c_bus_device) - 1; i >= 0; i--) {
        fpga_i2c_bus_device_data = fpga_i2c_bus_device[i].dev.platform_data;
        if (fpga_i2c_bus_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&fpga_i2c_bus_device[i]);
        }
    }
}

module_init(wb_fpga_i2c_bus_device_init);
module_exit(wb_fpga_i2c_bus_device_exit);
MODULE_DESCRIPTION("FPGA I2C Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
