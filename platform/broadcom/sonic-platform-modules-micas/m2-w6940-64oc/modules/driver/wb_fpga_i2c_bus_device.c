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
    .adap_nr                 = 2,
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
    .i2c_adap_reset_flag     = 0,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

/* CPLD-I2C-MASTER-4 */
static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data1 = {
    .adap_nr                 = 3,
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
    .i2c_adap_reset_flag     = 0,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

/* CPLD-I2C-MASTER-2 */
static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data2 = {
    .adap_nr                 = 4,
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
    .dev_name                = "/dev/cpld3",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 6,
    .i2c_adap_reset_flag     = 0,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

/* CPLD-I2C-MASTER-3 */
static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data3 = {
    .adap_nr                 = 5,
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
    .i2c_adap_reset_flag     = 0,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data4 = {
    .adap_nr                 = 6,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x1400,
    .i2c_filter              = 0x1404,
    .i2c_stretch             = 0x1408,
    .i2c_ext_9548_exits_flag = 0x140c,
    .i2c_ext_9548_addr       = 0x1410,
    .i2c_ext_9548_chan       = 0x1414,
    .i2c_in_9548_chan        = 0x1418,
    .i2c_slave               = 0x141c,
    .i2c_reg                 = 0x1420,
    .i2c_reg_len             = 0x1430,
    .i2c_data_len            = 0x1434,
    .i2c_ctrl                = 0x1438,
    .i2c_status              = 0x143c,
    .i2c_err_vec             = 0x1448,
    .i2c_data_buf            = 0x1480,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x4c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data5 = {
    .adap_nr                 = 7,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x1500,
    .i2c_filter              = 0x1504,
    .i2c_stretch             = 0x1508,
    .i2c_ext_9548_exits_flag = 0x150c,
    .i2c_ext_9548_addr       = 0x1510,
    .i2c_ext_9548_chan       = 0x1514,
    .i2c_in_9548_chan        = 0x1518,
    .i2c_slave               = 0x151c,
    .i2c_reg                 = 0x1520,
    .i2c_reg_len             = 0x1530,
    .i2c_data_len            = 0x1534,
    .i2c_ctrl                = 0x1538,
    .i2c_status              = 0x153c,
    .i2c_err_vec             = 0x1548,
    .i2c_data_buf            = 0x1580,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x50,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data6 = {
    .adap_nr                 = 8,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x1800,
    .i2c_filter              = 0x1804,
    .i2c_stretch             = 0x1808,
    .i2c_ext_9548_exits_flag = 0x180c,
    .i2c_ext_9548_addr       = 0x1810,
    .i2c_ext_9548_chan       = 0x1814,
    .i2c_in_9548_chan        = 0x1818,
    .i2c_slave               = 0x181c,
    .i2c_reg                 = 0x1820,
    .i2c_reg_len             = 0x1830,
    .i2c_data_len            = 0x1834,
    .i2c_ctrl                = 0x1838,
    .i2c_status              = 0x183c,
    .i2c_err_vec             = 0x1848,
    .i2c_data_buf            = 0x1880,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x54,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data7 = {
    .adap_nr                 = 9,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x1900,
    .i2c_filter              = 0x1904,
    .i2c_stretch             = 0x1908,
    .i2c_ext_9548_exits_flag = 0x190c,
    .i2c_ext_9548_addr       = 0x1910,
    .i2c_ext_9548_chan       = 0x1914,
    .i2c_in_9548_chan        = 0x1918,
    .i2c_slave               = 0x191c,
    .i2c_reg                 = 0x1920,
    .i2c_reg_len             = 0x1930,
    .i2c_data_len            = 0x1934,
    .i2c_ctrl                = 0x1938,
    .i2c_status              = 0x193c,
    .i2c_err_vec             = 0x1948,
    .i2c_data_buf            = 0x1980,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x58,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data8 = {
    .adap_nr                 = 10,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x1a00,
    .i2c_filter              = 0x1a04,
    .i2c_stretch             = 0x1a08,
    .i2c_ext_9548_exits_flag = 0x1a0c,
    .i2c_ext_9548_addr       = 0x1a10,
    .i2c_ext_9548_chan       = 0x1a14,
    .i2c_in_9548_chan        = 0x1a18,
    .i2c_slave               = 0x1a1c,
    .i2c_reg                 = 0x1a20,
    .i2c_reg_len             = 0x1a30,
    .i2c_data_len            = 0x1a34,
    .i2c_ctrl                = 0x1a38,
    .i2c_status              = 0x1a3c,
    .i2c_err_vec             = 0x1a48,
    .i2c_data_buf            = 0x1a80,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data0 = {
    .adap_nr                 = 11,
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
    .i2c_data_buf            = 0x2080,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000002,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data1 = {
    .adap_nr                 = 12,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2100,
    .i2c_filter              = 0x2104,
    .i2c_stretch             = 0x2108,
    .i2c_ext_9548_exits_flag = 0x210c,
    .i2c_ext_9548_addr       = 0x2110,
    .i2c_ext_9548_chan       = 0x2114,
    .i2c_in_9548_chan        = 0x2118,
    .i2c_slave               = 0x211c,
    .i2c_reg                 = 0x2120,
    .i2c_reg_len             = 0x2130,
    .i2c_data_len            = 0x2134,
    .i2c_ctrl                = 0x2138,
    .i2c_status              = 0x213c,
    .i2c_err_vec             = 0x2148,
    .i2c_data_buf            = 0x2180,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x0000004,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data2 = {
    .adap_nr                 = 13,
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
    .i2c_data_buf            = 0x2280,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000008,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data3 = {
    .adap_nr                 = 14,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2300,
    .i2c_filter              = 0x2304,
    .i2c_stretch             = 0x2308,
    .i2c_ext_9548_exits_flag = 0x230c,
    .i2c_ext_9548_addr       = 0x2310,
    .i2c_ext_9548_chan       = 0x2314,
    .i2c_in_9548_chan        = 0x2318,
    .i2c_slave               = 0x231c,
    .i2c_reg                 = 0x2320,
    .i2c_reg_len             = 0x2330,
    .i2c_data_len            = 0x2334,
    .i2c_ctrl                = 0x2338,
    .i2c_status              = 0x233c,
    .i2c_err_vec             = 0x2348,
    .i2c_data_buf            = 0x2380,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000010,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data4 = {
    .adap_nr                 = 15,
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
    .i2c_data_buf            = 0x2480,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000020,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data5 = {
    .adap_nr                 = 16,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2500,
    .i2c_filter              = 0x2504,
    .i2c_stretch             = 0x2508,
    .i2c_ext_9548_exits_flag = 0x250c,
    .i2c_ext_9548_addr       = 0x2510,
    .i2c_ext_9548_chan       = 0x2514,
    .i2c_in_9548_chan        = 0x2518,
    .i2c_slave               = 0x251c,
    .i2c_reg                 = 0x2520,
    .i2c_reg_len             = 0x2530,
    .i2c_data_len            = 0x2534,
    .i2c_ctrl                = 0x2538,
    .i2c_status              = 0x253c,
    .i2c_err_vec             = 0x2548,
    .i2c_data_buf            = 0x2580,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000040,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data6 = {
    .adap_nr                 = 17,
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
    .i2c_data_buf            = 0x2680,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000080,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data7 = {
    .adap_nr                 = 18,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2700,
    .i2c_filter              = 0x2704,
    .i2c_stretch             = 0x2708,
    .i2c_ext_9548_exits_flag = 0x270c,
    .i2c_ext_9548_addr       = 0x2710,
    .i2c_ext_9548_chan       = 0x2714,
    .i2c_in_9548_chan        = 0x2718,
    .i2c_slave               = 0x271c,
    .i2c_reg                 = 0x2720,
    .i2c_reg_len             = 0x2730,
    .i2c_data_len            = 0x2734,
    .i2c_ctrl                = 0x2738,
    .i2c_status              = 0x273c,
    .i2c_err_vec             = 0x2748,
    .i2c_data_buf            = 0x2780,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000100,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data8 = {
    .adap_nr                 = 19,
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
    .i2c_data_buf            = 0x2880,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000200,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data9 = {
    .adap_nr                 = 20,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2900,
    .i2c_filter              = 0x2904,
    .i2c_stretch             = 0x2908,
    .i2c_ext_9548_exits_flag = 0x290c,
    .i2c_ext_9548_addr       = 0x2910,
    .i2c_ext_9548_chan       = 0x2914,
    .i2c_in_9548_chan        = 0x2918,
    .i2c_slave               = 0x291c,
    .i2c_reg                 = 0x2920,
    .i2c_reg_len             = 0x2930,
    .i2c_data_len            = 0x2934,
    .i2c_ctrl                = 0x2938,
    .i2c_status              = 0x293c,
    .i2c_err_vec             = 0x2948,
    .i2c_data_buf            = 0x2980,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000400,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data10 = {
    .adap_nr                 = 21,
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
    .i2c_data_buf            = 0x2a80,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000800,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data11 = {
    .adap_nr                 = 22,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2b00,
    .i2c_filter              = 0x2b04,
    .i2c_stretch             = 0x2b08,
    .i2c_ext_9548_exits_flag = 0x2b0c,
    .i2c_ext_9548_addr       = 0x2b10,
    .i2c_ext_9548_chan       = 0x2b14,
    .i2c_in_9548_chan        = 0x2b18,
    .i2c_slave               = 0x2b1c,
    .i2c_reg                 = 0x2b20,
    .i2c_reg_len             = 0x2b30,
    .i2c_data_len            = 0x2b34,
    .i2c_ctrl                = 0x2b38,
    .i2c_status              = 0x2b3c,
    .i2c_err_vec             = 0x2b48,
    .i2c_data_buf            = 0x2b80,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00001000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data12 = {
    .adap_nr                 = 23,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2c00,
    .i2c_filter              = 0x2c04,
    .i2c_stretch             = 0x2c08,
    .i2c_ext_9548_exits_flag = 0x2c0c,
    .i2c_ext_9548_addr       = 0x2c10,
    .i2c_ext_9548_chan       = 0x2c14,
    .i2c_in_9548_chan        = 0x2c18,
    .i2c_slave               = 0x2c1c,
    .i2c_reg                 = 0x2c20,
    .i2c_reg_len             = 0x2c30,
    .i2c_data_len            = 0x2c34,
    .i2c_ctrl                = 0x2c38,
    .i2c_status              = 0x2c3c,
    .i2c_err_vec             = 0x2c48,
    .i2c_data_buf            = 0x2c80,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00002000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data13 = {
    .adap_nr                 = 24,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2d00,
    .i2c_filter              = 0x2d04,
    .i2c_stretch             = 0x2d08,
    .i2c_ext_9548_exits_flag = 0x2d0c,
    .i2c_ext_9548_addr       = 0x2d10,
    .i2c_ext_9548_chan       = 0x2d14,
    .i2c_in_9548_chan        = 0x2d18,
    .i2c_slave               = 0x2d1c,
    .i2c_reg                 = 0x2d20,
    .i2c_reg_len             = 0x2d30,
    .i2c_data_len            = 0x2d34,
    .i2c_ctrl                = 0x2d38,
    .i2c_status              = 0x2d3c,
    .i2c_err_vec             = 0x2d48,
    .i2c_data_buf            = 0x2d80,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00004000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data14 = {
    .adap_nr                 = 25,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2e00,
    .i2c_filter              = 0x2e04,
    .i2c_stretch             = 0x2e08,
    .i2c_ext_9548_exits_flag = 0x2e0c,
    .i2c_ext_9548_addr       = 0x2e10,
    .i2c_ext_9548_chan       = 0x2e14,
    .i2c_in_9548_chan        = 0x2e18,
    .i2c_slave               = 0x2e1c,
    .i2c_reg                 = 0x2e20,
    .i2c_reg_len             = 0x2e30,
    .i2c_data_len            = 0x2e34,
    .i2c_ctrl                = 0x2e38,
    .i2c_status              = 0x2e3c,
    .i2c_err_vec             = 0x2e48,
    .i2c_data_buf            = 0x2e80,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00008000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data15 = {
    .adap_nr                 = 26,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x2f00,
    .i2c_filter              = 0x2f04,
    .i2c_stretch             = 0x2f08,
    .i2c_ext_9548_exits_flag = 0x2f0c,
    .i2c_ext_9548_addr       = 0x2f10,
    .i2c_ext_9548_chan       = 0x2f14,
    .i2c_in_9548_chan        = 0x2f18,
    .i2c_slave               = 0x2f1c,
    .i2c_reg                 = 0x2f20,
    .i2c_reg_len             = 0x2f30,
    .i2c_data_len            = 0x2f34,
    .i2c_ctrl                = 0x2f38,
    .i2c_status              = 0x2f3c,
    .i2c_err_vec             = 0x2f48,
    .i2c_data_buf            = 0x2f80,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 3,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00010000,
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
        .id = 9,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data8,
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
            .platform_data  = &fpga0_dom_i2c_bus_device_data8,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 19,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data9,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 20,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data10,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 21,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data11,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 22,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data12,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 23,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data13,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 24,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data14,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "wb-fpga-i2c",
        .id = 25,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data15,
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
