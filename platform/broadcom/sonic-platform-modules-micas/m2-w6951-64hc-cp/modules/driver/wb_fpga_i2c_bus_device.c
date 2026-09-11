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

/* CPLD-I2C-MASTER-1 */
static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data0 = {
    .adap_nr                 = 401,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x00,
    .i2c_filter              = 0x04,
    .i2c_stretch             = 0x08,
    .i2c_ext_9548_exits_flag = 0x0c,
    .i2c_ext_9548_addr       = 0x10,
    .i2c_ext_9548_chan       = 0x14,
    .i2c_in_9548_chan        = 0x18,
    .i2c_slave               = 0x1c,
    .i2c_reg                 = 0x20,
    .i2c_reg_len             = 0x30,
    .i2c_data_len            = 0x34,
    .i2c_ctrl                = 0x38,
    .i2c_status              = 0x3c,
    .i2c_err_vec             = 0x48,
    .i2c_data_buf            = 0x100,
    .i2c_data_buf_len        = 256,
    .dev_name                = "/dev/cpld2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 6,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

/* CPLD-I2C-MASTER-4 */
static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data1 = {
    .adap_nr                 = 402,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x00,
    .i2c_filter              = 0x04,
    .i2c_stretch             = 0x08,
    .i2c_ext_9548_exits_flag = 0x0c,
    .i2c_ext_9548_addr       = 0x10,
    .i2c_ext_9548_chan       = 0x14,
    .i2c_in_9548_chan        = 0x18,
    .i2c_slave               = 0x1c,
    .i2c_reg                 = 0x20,
    .i2c_reg_len             = 0x30,
    .i2c_data_len            = 0x34,
    .i2c_ctrl                = 0x38,
    .i2c_status              = 0x3c,
    .i2c_err_vec             = 0x48,
    .i2c_data_buf            = 0x100,
    .i2c_data_buf_len        = 256,
    .dev_name                = "/dev/cpld5",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 6,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

/* CPLD-I2C-MASTER-3 */
static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data2 = {
    .adap_nr                 = 403,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x00,
    .i2c_filter              = 0x04,
    .i2c_stretch             = 0x08,
    .i2c_ext_9548_exits_flag = 0x0c,
    .i2c_ext_9548_addr       = 0x10,
    .i2c_ext_9548_chan       = 0x14,
    .i2c_in_9548_chan        = 0x18,
    .i2c_slave               = 0x1c,
    .i2c_reg                 = 0x20,
    .i2c_reg_len             = 0x30,
    .i2c_data_len            = 0x34,
    .i2c_ctrl                = 0x38,
    .i2c_status              = 0x3c,
    .i2c_err_vec             = 0x48,
    .i2c_data_buf            = 0x100,
    .i2c_data_buf_len        = 256,
    .dev_name                = "/dev/cpld4",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 6,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data3 = {
    .adap_nr                 = 404,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2000,
    .i2c_filter              = 0x2004,
    .i2c_stretch             = 0x2008,
    .i2c_ext_9548_exits_flag = 0x200c,
    .i2c_ext_9548_addr       = 0x2010,
    .i2c_ext_9548_chan       = 0x2014,
    .i2c_in_9548_chan        = 0x2018,
    .i2c_slave               = 0x201c,
    .i2c_reg                 = 0x2020,
    .i2c_reg_len             = 0x2030,
    .i2c_data_len            = 0x2034,
    .i2c_ctrl                = 0x2038,
    .i2c_status              = 0x203c,
    .i2c_err_vec             = 0x2048,
    .i2c_data_buf            = 0x2100,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x207c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data4 = {
    .adap_nr                 = 405,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2200,
    .i2c_filter              = 0x2204,
    .i2c_stretch             = 0x2208,
    .i2c_ext_9548_exits_flag = 0x220c,
    .i2c_ext_9548_addr       = 0x2210,
    .i2c_ext_9548_chan       = 0x2214,
    .i2c_in_9548_chan        = 0x2218,
    .i2c_slave               = 0x221c,
    .i2c_reg                 = 0x2220,
    .i2c_reg_len             = 0x2230,
    .i2c_data_len            = 0x2234,
    .i2c_ctrl                = 0x2238,
    .i2c_status              = 0x223c,
    .i2c_err_vec             = 0x2248,
    .i2c_data_buf            = 0x2300,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x227c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data5 = {
    .adap_nr                 = 406,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2400,
    .i2c_filter              = 0x2404,
    .i2c_stretch             = 0x2408,
    .i2c_ext_9548_exits_flag = 0x240c,
    .i2c_ext_9548_addr       = 0x2410,
    .i2c_ext_9548_chan       = 0x2414,
    .i2c_in_9548_chan        = 0x2418,
    .i2c_slave               = 0x241c,
    .i2c_reg                 = 0x2420,
    .i2c_reg_len             = 0x2430,
    .i2c_data_len            = 0x2434,
    .i2c_ctrl                = 0x2438,
    .i2c_status              = 0x243c,
    .i2c_err_vec             = 0x2448,
    .i2c_data_buf            = 0x2500,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x247c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data6 = {
    .adap_nr                 = 407,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2600,
    .i2c_filter              = 0x2604,
    .i2c_stretch             = 0x2608,
    .i2c_ext_9548_exits_flag = 0x260c,
    .i2c_ext_9548_addr       = 0x2610,
    .i2c_ext_9548_chan       = 0x2614,
    .i2c_in_9548_chan        = 0x2618,
    .i2c_slave               = 0x261c,
    .i2c_reg                 = 0x2620,
    .i2c_reg_len             = 0x2630,
    .i2c_data_len            = 0x2634,
    .i2c_ctrl                = 0x2638,
    .i2c_status              = 0x263c,
    .i2c_err_vec             = 0x2648,
    .i2c_data_buf            = 0x2700,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x267c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data7 = {
    .adap_nr                 = 408,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2800,
    .i2c_filter              = 0x2804,
    .i2c_stretch             = 0x2808,
    .i2c_ext_9548_exits_flag = 0x280c,
    .i2c_ext_9548_addr       = 0x2810,
    .i2c_ext_9548_chan       = 0x2814,
    .i2c_in_9548_chan        = 0x2818,
    .i2c_slave               = 0x281c,
    .i2c_reg                 = 0x2820,
    .i2c_reg_len             = 0x2830,
    .i2c_data_len            = 0x2834,
    .i2c_ctrl                = 0x2838,
    .i2c_status              = 0x283c,
    .i2c_err_vec             = 0x2848,
    .i2c_data_buf            = 0x2900,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x287c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data8 = {
    .adap_nr                 = 432,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2a00,
    .i2c_filter              = 0x2a04,
    .i2c_stretch             = 0x2a08,
    .i2c_ext_9548_exits_flag = 0x2a0c,
    .i2c_ext_9548_addr       = 0x2a10,
    .i2c_ext_9548_chan       = 0x2a14,
    .i2c_in_9548_chan        = 0x2a18,
    .i2c_slave               = 0x2a1c,
    .i2c_reg                 = 0x2a20,
    .i2c_reg_len             = 0x2a30,
    .i2c_data_len            = 0x2a34,
    .i2c_ctrl                = 0x2a38,
    .i2c_status              = 0x2a3c,
    .i2c_err_vec             = 0x2a48,
    .i2c_data_buf            = 0x2b00,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x2a7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data0 = {
    .adap_nr                 = 409,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2200,
    .i2c_filter              = 0x2204,
    .i2c_stretch             = 0x2208,
    .i2c_ext_9548_exits_flag = 0x220c,
    .i2c_ext_9548_addr       = 0x2210,
    .i2c_ext_9548_chan       = 0x2214,
    .i2c_in_9548_chan        = 0x2218,
    .i2c_slave               = 0x221c,
    .i2c_reg                 = 0x2220,
    .i2c_reg_len             = 0x2230,
    .i2c_data_len            = 0x2234,
    .i2c_ctrl                = 0x2238,
    .i2c_status              = 0x223c,
    .i2c_err_vec             = 0x2248,
    .i2c_data_buf            = 0x2300,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x227c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data0 = {
    .adap_nr                 = 410,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x4000,
    .i2c_filter              = 0x4004,
    .i2c_stretch             = 0x4008,
    .i2c_ext_9548_exits_flag = 0x400c,
    .i2c_ext_9548_addr       = 0x4010,
    .i2c_ext_9548_chan       = 0x4014,
    .i2c_in_9548_chan        = 0x4018,
    .i2c_slave               = 0x401c,
    .i2c_reg                 = 0x4020,
    .i2c_reg_len             = 0x4030,
    .i2c_data_len            = 0x4034,
    .i2c_ctrl                = 0x4038,
    .i2c_status              = 0x403c,
    .i2c_err_vec             = 0x4048,
    .i2c_data_buf            = 0x4100,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x407c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data1 = {
    .adap_nr                 = 411,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x4400,
    .i2c_filter              = 0x4404,
    .i2c_stretch             = 0x4408,
    .i2c_ext_9548_exits_flag = 0x440c,
    .i2c_ext_9548_addr       = 0x4410,
    .i2c_ext_9548_chan       = 0x4414,
    .i2c_in_9548_chan        = 0x4418,
    .i2c_slave               = 0x441c,
    .i2c_reg                 = 0x4420,
    .i2c_reg_len             = 0x4430,
    .i2c_data_len            = 0x4434,
    .i2c_ctrl                = 0x4438,
    .i2c_status              = 0x443c,
    .i2c_err_vec             = 0x4448,
    .i2c_data_buf            = 0x4500,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x447c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data2 = {
    .adap_nr                 = 412,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x4800,
    .i2c_filter              = 0x4804,
    .i2c_stretch             = 0x4808,
    .i2c_ext_9548_exits_flag = 0x480c,
    .i2c_ext_9548_addr       = 0x4810,
    .i2c_ext_9548_chan       = 0x4814,
    .i2c_in_9548_chan        = 0x4818,
    .i2c_slave               = 0x481c,
    .i2c_reg                 = 0x4820,
    .i2c_reg_len             = 0x4830,
    .i2c_data_len            = 0x4834,
    .i2c_ctrl                = 0x4838,
    .i2c_status              = 0x483c,
    .i2c_err_vec             = 0x4848,
    .i2c_data_buf            = 0x4900,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x487c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data3 = {
    .adap_nr                 = 413,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x4c00,
    .i2c_filter              = 0x4c04,
    .i2c_stretch             = 0x4c08,
    .i2c_ext_9548_exits_flag = 0x4c0c,
    .i2c_ext_9548_addr       = 0x4c10,
    .i2c_ext_9548_chan       = 0x4c14,
    .i2c_in_9548_chan        = 0x4c18,
    .i2c_slave               = 0x4c1c,
    .i2c_reg                 = 0x4c20,
    .i2c_reg_len             = 0x4c30,
    .i2c_data_len            = 0x4c34,
    .i2c_ctrl                = 0x4c38,
    .i2c_status              = 0x4c3c,
    .i2c_err_vec             = 0x4c48,
    .i2c_data_buf            = 0x4d00,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x4c7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data4 = {
    .adap_nr                 = 414,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x5000,
    .i2c_filter              = 0x5004,
    .i2c_stretch             = 0x5008,
    .i2c_ext_9548_exits_flag = 0x500c,
    .i2c_ext_9548_addr       = 0x5010,
    .i2c_ext_9548_chan       = 0x5014,
    .i2c_in_9548_chan        = 0x5018,
    .i2c_slave               = 0x501c,
    .i2c_reg                 = 0x5020,
    .i2c_reg_len             = 0x5030,
    .i2c_data_len            = 0x5034,
    .i2c_ctrl                = 0x5038,
    .i2c_status              = 0x503c,
    .i2c_err_vec             = 0x5048,
    .i2c_data_buf            = 0x5100,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x507c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data5 = {
    .adap_nr                 = 415,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x5400,
    .i2c_filter              = 0x5404,
    .i2c_stretch             = 0x5408,
    .i2c_ext_9548_exits_flag = 0x540c,
    .i2c_ext_9548_addr       = 0x5410,
    .i2c_ext_9548_chan       = 0x5414,
    .i2c_in_9548_chan        = 0x5418,
    .i2c_slave               = 0x541c,
    .i2c_reg                 = 0x5420,
    .i2c_reg_len             = 0x5430,
    .i2c_data_len            = 0x5434,
    .i2c_ctrl                = 0x5438,
    .i2c_status              = 0x543c,
    .i2c_err_vec             = 0x5448,
    .i2c_data_buf            = 0x5500,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x547c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data6 = {
    .adap_nr                 = 416,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x5800,
    .i2c_filter              = 0x5804,
    .i2c_stretch             = 0x5808,
    .i2c_ext_9548_exits_flag = 0x580c,
    .i2c_ext_9548_addr       = 0x5810,
    .i2c_ext_9548_chan       = 0x5814,
    .i2c_in_9548_chan        = 0x5818,
    .i2c_slave               = 0x581c,
    .i2c_reg                 = 0x5820,
    .i2c_reg_len             = 0x5830,
    .i2c_data_len            = 0x5834,
    .i2c_ctrl                = 0x5838,
    .i2c_status              = 0x583c,
    .i2c_err_vec             = 0x5848,
    .i2c_data_buf            = 0x5900,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x587c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data7 = {
    .adap_nr                 = 417,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x5c00,
    .i2c_filter              = 0x5c04,
    .i2c_stretch             = 0x5c08,
    .i2c_ext_9548_exits_flag = 0x5c0c,
    .i2c_ext_9548_addr       = 0x5c10,
    .i2c_ext_9548_chan       = 0x5c14,
    .i2c_in_9548_chan        = 0x5c18,
    .i2c_slave               = 0x5c1c,
    .i2c_reg                 = 0x5c20,
    .i2c_reg_len             = 0x5c30,
    .i2c_data_len            = 0x5c34,
    .i2c_ctrl                = 0x5c38,
    .i2c_status              = 0x5c3c,
    .i2c_err_vec             = 0x5c48,
    .i2c_data_buf            = 0x5d00,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x5c7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data0 = {
    .adap_nr                 = 418,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x4000,
    .i2c_filter              = 0x4004,
    .i2c_stretch             = 0x4008,
    .i2c_ext_9548_exits_flag = 0x400c,
    .i2c_ext_9548_addr       = 0x4010,
    .i2c_ext_9548_chan       = 0x4014,
    .i2c_in_9548_chan        = 0x4018,
    .i2c_slave               = 0x401c,
    .i2c_reg                 = 0x4020,
    .i2c_reg_len             = 0x4030,
    .i2c_data_len            = 0x4034,
    .i2c_ctrl                = 0x4038,
    .i2c_status              = 0x403c,
    .i2c_err_vec             = 0x4048,
    .i2c_data_buf            = 0x4100,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x407c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data1 = {
    .adap_nr                 = 419,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x4400,
    .i2c_filter              = 0x4404,
    .i2c_stretch             = 0x4408,
    .i2c_ext_9548_exits_flag = 0x440c,
    .i2c_ext_9548_addr       = 0x4410,
    .i2c_ext_9548_chan       = 0x4414,
    .i2c_in_9548_chan        = 0x4418,
    .i2c_slave               = 0x441c,
    .i2c_reg                 = 0x4420,
    .i2c_reg_len             = 0x4430,
    .i2c_data_len            = 0x4434,
    .i2c_ctrl                = 0x4438,
    .i2c_status              = 0x443c,
    .i2c_err_vec             = 0x4448,
    .i2c_data_buf            = 0x4500,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x447c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data2 = {
    .adap_nr                 = 420,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x4800,
    .i2c_filter              = 0x4804,
    .i2c_stretch             = 0x4808,
    .i2c_ext_9548_exits_flag = 0x480c,
    .i2c_ext_9548_addr       = 0x4810,
    .i2c_ext_9548_chan       = 0x4814,
    .i2c_in_9548_chan        = 0x4818,
    .i2c_slave               = 0x481c,
    .i2c_reg                 = 0x4820,
    .i2c_reg_len             = 0x4830,
    .i2c_data_len            = 0x4834,
    .i2c_ctrl                = 0x4838,
    .i2c_status              = 0x483c,
    .i2c_err_vec             = 0x4848,
    .i2c_data_buf            = 0x4900,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x487c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data3 = {
    .adap_nr                 = 421,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x4c00,
    .i2c_filter              = 0x4c04,
    .i2c_stretch             = 0x4c08,
    .i2c_ext_9548_exits_flag = 0x4c0c,
    .i2c_ext_9548_addr       = 0x4c10,
    .i2c_ext_9548_chan       = 0x4c14,
    .i2c_in_9548_chan        = 0x4c18,
    .i2c_slave               = 0x4c1c,
    .i2c_reg                 = 0x4c20,
    .i2c_reg_len             = 0x4c30,
    .i2c_data_len            = 0x4c34,
    .i2c_ctrl                = 0x4c38,
    .i2c_status              = 0x4c3c,
    .i2c_err_vec             = 0x4c48,
    .i2c_data_buf            = 0x4d00,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x4c7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data4 = {
    .adap_nr                 = 422,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x5000,
    .i2c_filter              = 0x5004,
    .i2c_stretch             = 0x5008,
    .i2c_ext_9548_exits_flag = 0x500c,
    .i2c_ext_9548_addr       = 0x5010,
    .i2c_ext_9548_chan       = 0x5014,
    .i2c_in_9548_chan        = 0x5018,
    .i2c_slave               = 0x501c,
    .i2c_reg                 = 0x5020,
    .i2c_reg_len             = 0x5030,
    .i2c_data_len            = 0x5034,
    .i2c_ctrl                = 0x5038,
    .i2c_status              = 0x503c,
    .i2c_err_vec             = 0x5048,
    .i2c_data_buf            = 0x5100,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x507c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data5 = {
    .adap_nr                 = 423,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x5400,
    .i2c_filter              = 0x5404,
    .i2c_stretch             = 0x5408,
    .i2c_ext_9548_exits_flag = 0x540c,
    .i2c_ext_9548_addr       = 0x5410,
    .i2c_ext_9548_chan       = 0x5414,
    .i2c_in_9548_chan        = 0x5418,
    .i2c_slave               = 0x541c,
    .i2c_reg                 = 0x5420,
    .i2c_reg_len             = 0x5430,
    .i2c_data_len            = 0x5434,
    .i2c_ctrl                = 0x5438,
    .i2c_status              = 0x543c,
    .i2c_err_vec             = 0x5448,
    .i2c_data_buf            = 0x5500,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x547c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data6 = {
    .adap_nr                 = 424,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x5800,
    .i2c_filter              = 0x5804,
    .i2c_stretch             = 0x5808,
    .i2c_ext_9548_exits_flag = 0x580c,
    .i2c_ext_9548_addr       = 0x5810,
    .i2c_ext_9548_chan       = 0x5814,
    .i2c_in_9548_chan        = 0x5818,
    .i2c_slave               = 0x581c,
    .i2c_reg                 = 0x5820,
    .i2c_reg_len             = 0x5830,
    .i2c_data_len            = 0x5834,
    .i2c_ctrl                = 0x5838,
    .i2c_status              = 0x583c,
    .i2c_err_vec             = 0x5848,
    .i2c_data_buf            = 0x5900,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x587c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data7 = {
    .adap_nr                 = 425,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x5c00,
    .i2c_filter              = 0x5c04,
    .i2c_stretch             = 0x5c08,
    .i2c_ext_9548_exits_flag = 0x5c0c,
    .i2c_ext_9548_addr       = 0x5c10,
    .i2c_ext_9548_chan       = 0x5c14,
    .i2c_in_9548_chan        = 0x5c18,
    .i2c_slave               = 0x5c1c,
    .i2c_reg                 = 0x5c20,
    .i2c_reg_len             = 0x5c30,
    .i2c_data_len            = 0x5c34,
    .i2c_ctrl                = 0x5c38,
    .i2c_status              = 0x5c3c,
    .i2c_err_vec             = 0x5c48,
    .i2c_data_buf            = 0x5d00,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x5c7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data1 = {
    .adap_nr                 = 434,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2000,
    .i2c_filter              = 0x2004,
    .i2c_stretch             = 0x2008,
    .i2c_ext_9548_exits_flag = 0x200c,
    .i2c_ext_9548_addr       = 0x2010,
    .i2c_ext_9548_chan       = 0x2014,
    .i2c_in_9548_chan        = 0x2018,
    .i2c_slave               = 0x201c,
    .i2c_reg                 = 0x2020,
    .i2c_reg_len             = 0x2030,
    .i2c_data_len            = 0x2034,
    .i2c_ctrl                = 0x2038,
    .i2c_status              = 0x203c,
    .i2c_err_vec             = 0x2048,
    .i2c_data_buf            = 0x2100,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x207c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data2 = {
    .adap_nr                 = 436,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2400,
    .i2c_filter              = 0x2404,
    .i2c_stretch             = 0x2408,
    .i2c_ext_9548_exits_flag = 0x240c,
    .i2c_ext_9548_addr       = 0x2410,
    .i2c_ext_9548_chan       = 0x2414,
    .i2c_in_9548_chan        = 0x2418,
    .i2c_slave               = 0x241c,
    .i2c_reg                 = 0x2420,
    .i2c_reg_len             = 0x2430,
    .i2c_data_len            = 0x2434,
    .i2c_ctrl                = 0x2438,
    .i2c_status              = 0x243c,
    .i2c_err_vec             = 0x2448,
    .i2c_data_buf            = 0x2500,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x247c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
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
            .platform_data  = &fpga0_i2c_bus_device_data0,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 2,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data1,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 3,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data2,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 4,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data3,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 5,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data4,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 6,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data5,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 7,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data6,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 8,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data7,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 10,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data0,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 11,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data1,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 12,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data2,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 13,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data3,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 14,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data4,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 15,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data5,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 16,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data6,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 17,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data7,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 18,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data0,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 19,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data1,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 20,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data2,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 21,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data3,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 22,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data4,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 23,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data5,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 24,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data6,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 25,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data7,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 26,
        .dev    = {
            .platform_data  = &fpga1_i2c_bus_device_data0,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 27,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data8,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 28,
        .dev    = {
            .platform_data  = &fpga1_i2c_bus_device_data1,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 29,
        .dev    = {
            .platform_data  = &fpga1_i2c_bus_device_data2,
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
