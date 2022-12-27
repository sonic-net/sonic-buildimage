#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <fpga_i2c.h>

static int g_rg_fpga_i2c_debug = 0;
static int g_rg_fpga_i2c_error = 0;

module_param(g_rg_fpga_i2c_debug, int, S_IRUGO | S_IWUSR);
module_param(g_rg_fpga_i2c_error, int, S_IRUGO | S_IWUSR);

#define RG_FPGA_I2C_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_rg_fpga_i2c_debug) { \
        printk(KERN_INFO "[RG_FPGA_I2C][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define RG_FPGA_I2C_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_rg_fpga_i2c_error) { \
        printk(KERN_ERR "[RG_FPGA_I2C][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data0 = {
    .adap_nr                 = 2,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x300,
    .i2c_filter              = 0x304,
    .i2c_stretch             = 0x308,
    .i2c_ext_9548_exits_flag = 0x30c,
    .i2c_ext_9548_addr       = 0x310,
    .i2c_ext_9548_chan       = 0x314,
    .i2c_in_9548_chan        = 0x318,
    .i2c_slave               = 0x31c,
    .i2c_reg                 = 0x320,
    .i2c_reg_len             = 0x330,
    .i2c_data_len            = 0x334,
    .i2c_ctrl                = 0x338,
    .i2c_status              = 0x33c,
    .i2c_err_vec             = 0x348,
    .i2c_data_buf            = 0x380,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x80,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data1 = {
    .adap_nr                 = 3,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x400,
    .i2c_filter              = 0x404,
    .i2c_stretch             = 0x408,
    .i2c_ext_9548_exits_flag = 0x40c,
    .i2c_ext_9548_addr       = 0x410,
    .i2c_ext_9548_chan       = 0x414,
    .i2c_in_9548_chan        = 0x418,
    .i2c_slave               = 0x41c,
    .i2c_reg                 = 0x420,
    .i2c_reg_len             = 0x430,
    .i2c_data_len            = 0x434,
    .i2c_ctrl                = 0x438,
    .i2c_status              = 0x43c,
    .i2c_err_vec             = 0x448,
    .i2c_data_buf            = 0x480,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x80,
    .i2c_reset_on            = 0x00000002,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data0 = {
    .adap_nr                 = 4,
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
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};
static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data1 = {
    .adap_nr                 = 5,
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
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000002,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data2 = {
    .adap_nr                 = 6,
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
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000004,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data3 = {
    .adap_nr                 = 7,
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
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000008,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data4 = {
    .adap_nr                 = 8,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3000,
    .i2c_filter              = 0x3004,
    .i2c_stretch             = 0x3008,
    .i2c_ext_9548_exits_flag = 0x300c,
    .i2c_ext_9548_addr       = 0x3010,
    .i2c_ext_9548_chan       = 0x3014,
    .i2c_in_9548_chan        = 0x3018,
    .i2c_slave               = 0x301c,
    .i2c_reg                 = 0x3020,
    .i2c_reg_len             = 0x3030,
    .i2c_data_len            = 0x3034,
    .i2c_ctrl                = 0x3038,
    .i2c_status              = 0x303c,
    .i2c_err_vec             = 0x3048,
    .i2c_data_buf            = 0x3080,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000010,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data5 = {
    .adap_nr                 = 9,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3100,
    .i2c_filter              = 0x3104,
    .i2c_stretch             = 0x3108,
    .i2c_ext_9548_exits_flag = 0x310c,
    .i2c_ext_9548_addr       = 0x3110,
    .i2c_ext_9548_chan       = 0x3114,
    .i2c_in_9548_chan        = 0x3118,
    .i2c_slave               = 0x311c,
    .i2c_reg                 = 0x3120,
    .i2c_reg_len             = 0x3130,
    .i2c_data_len            = 0x3134,
    .i2c_ctrl                = 0x3138,
    .i2c_status              = 0x313c,
    .i2c_err_vec             = 0x3148,
    .i2c_data_buf            = 0x3180,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000020,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data6 = {
    .adap_nr                 = 10,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3200,
    .i2c_filter              = 0x3204,
    .i2c_stretch             = 0x3208,
    .i2c_ext_9548_exits_flag = 0x320c,
    .i2c_ext_9548_addr       = 0x3210,
    .i2c_ext_9548_chan       = 0x3214,
    .i2c_in_9548_chan        = 0x3218,
    .i2c_slave               = 0x321c,
    .i2c_reg                 = 0x3220,
    .i2c_reg_len             = 0x3230,
    .i2c_data_len            = 0x3234,
    .i2c_ctrl                = 0x3238,
    .i2c_status              = 0x323c,
    .i2c_err_vec             = 0x3248,
    .i2c_data_buf            = 0x3280,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000040,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data7 = {
    .adap_nr                 = 11,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3300,
    .i2c_filter              = 0x3304,
    .i2c_stretch             = 0x3308,
    .i2c_ext_9548_exits_flag = 0x330c,
    .i2c_ext_9548_addr       = 0x3310,
    .i2c_ext_9548_chan       = 0x3314,
    .i2c_in_9548_chan        = 0x3318,
    .i2c_slave               = 0x331c,
    .i2c_reg                 = 0x3320,
    .i2c_reg_len             = 0x3330,
    .i2c_data_len            = 0x3334,
    .i2c_ctrl                = 0x3338,
    .i2c_status              = 0x333c,
    .i2c_err_vec             = 0x3348,
    .i2c_data_buf            = 0x3380,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000080,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data8 = {
    .adap_nr                 = 12,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3400,
    .i2c_filter              = 0x3404,
    .i2c_stretch             = 0x3408,
    .i2c_ext_9548_exits_flag = 0x340c,
    .i2c_ext_9548_addr       = 0x3410,
    .i2c_ext_9548_chan       = 0x3414,
    .i2c_in_9548_chan        = 0x3418,
    .i2c_slave               = 0x341c,
    .i2c_reg                 = 0x3420,
    .i2c_reg_len             = 0x3430,
    .i2c_data_len            = 0x3434,
    .i2c_ctrl                = 0x3438,
    .i2c_status              = 0x343c,
    .i2c_err_vec             = 0x3448,
    .i2c_data_buf            = 0x3480,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000100,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data9 = {
    .adap_nr                 = 13,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3500,
    .i2c_filter              = 0x3504,
    .i2c_stretch             = 0x3508,
    .i2c_ext_9548_exits_flag = 0x350c,
    .i2c_ext_9548_addr       = 0x3510,
    .i2c_ext_9548_chan       = 0x3514,
    .i2c_in_9548_chan        = 0x3518,
    .i2c_slave               = 0x351c,
    .i2c_reg                 = 0x3520,
    .i2c_reg_len             = 0x3530,
    .i2c_data_len            = 0x3534,
    .i2c_ctrl                = 0x3538,
    .i2c_status              = 0x353c,
    .i2c_err_vec             = 0x3548,
    .i2c_data_buf            = 0x3580,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000200,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data10 = {
    .adap_nr                 = 14,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3600,
    .i2c_filter              = 0x3604,
    .i2c_stretch             = 0x3608,
    .i2c_ext_9548_exits_flag = 0x360c,
    .i2c_ext_9548_addr       = 0x3610,
    .i2c_ext_9548_chan       = 0x3614,
    .i2c_in_9548_chan        = 0x3618,
    .i2c_slave               = 0x361c,
    .i2c_reg                 = 0x3620,
    .i2c_reg_len             = 0x3630,
    .i2c_data_len            = 0x3634,
    .i2c_ctrl                = 0x3638,
    .i2c_status              = 0x363c,
    .i2c_err_vec             = 0x3648,
    .i2c_data_buf            = 0x3680,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000400,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data11 = {
    .adap_nr                 = 15,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3700,
    .i2c_filter              = 0x3704,
    .i2c_stretch             = 0x3708,
    .i2c_ext_9548_exits_flag = 0x370c,
    .i2c_ext_9548_addr       = 0x3710,
    .i2c_ext_9548_chan       = 0x3714,
    .i2c_in_9548_chan        = 0x3718,
    .i2c_slave               = 0x371c,
    .i2c_reg                 = 0x3720,
    .i2c_reg_len             = 0x3730,
    .i2c_data_len            = 0x3734,
    .i2c_ctrl                = 0x3738,
    .i2c_status              = 0x373c,
    .i2c_err_vec             = 0x3748,
    .i2c_data_buf            = 0x3780,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000800,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data12 = {
    .adap_nr                 = 16,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3800,
    .i2c_filter              = 0x3804,
    .i2c_stretch             = 0x3808,
    .i2c_ext_9548_exits_flag = 0x380c,
    .i2c_ext_9548_addr       = 0x3810,
    .i2c_ext_9548_chan       = 0x3814,
    .i2c_in_9548_chan        = 0x3818,
    .i2c_slave               = 0x381c,
    .i2c_reg                 = 0x3820,
    .i2c_reg_len             = 0x3830,
    .i2c_data_len            = 0x3834,
    .i2c_ctrl                = 0x3838,
    .i2c_status              = 0x383c,
    .i2c_err_vec             = 0x3848,
    .i2c_data_buf            = 0x3880,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00001000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data13 = {
    .adap_nr                 = 17,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3900,
    .i2c_filter              = 0x3904,
    .i2c_stretch             = 0x3908,
    .i2c_ext_9548_exits_flag = 0x390c,
    .i2c_ext_9548_addr       = 0x3910,
    .i2c_ext_9548_chan       = 0x3914,
    .i2c_in_9548_chan        = 0x3918,
    .i2c_slave               = 0x391c,
    .i2c_reg                 = 0x3920,
    .i2c_reg_len             = 0x3930,
    .i2c_data_len            = 0x3934,
    .i2c_ctrl                = 0x3938,
    .i2c_status              = 0x393c,
    .i2c_err_vec             = 0x3948,
    .i2c_data_buf            = 0x3980,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00002000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data14 = {
    .adap_nr                 = 18,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3a00,
    .i2c_filter              = 0x3a04,
    .i2c_stretch             = 0x3a08,
    .i2c_ext_9548_exits_flag = 0x3a0c,
    .i2c_ext_9548_addr       = 0x3a10,
    .i2c_ext_9548_chan       = 0x3a14,
    .i2c_in_9548_chan        = 0x3a18,
    .i2c_slave               = 0x3a1c,
    .i2c_reg                 = 0x3a20,
    .i2c_reg_len             = 0x3a30,
    .i2c_data_len            = 0x3a34,
    .i2c_ctrl                = 0x3a38,
    .i2c_status              = 0x3a3c,
    .i2c_err_vec             = 0x3a48,
    .i2c_data_buf            = 0x3a80,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00004000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data15 = {
    .adap_nr                 = 19,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3b00,
    .i2c_filter              = 0x3b04,
    .i2c_stretch             = 0x3b08,
    .i2c_ext_9548_exits_flag = 0x3b0c,
    .i2c_ext_9548_addr       = 0x3b10,
    .i2c_ext_9548_chan       = 0x3b14,
    .i2c_in_9548_chan        = 0x3b18,
    .i2c_slave               = 0x3b1c,
    .i2c_reg                 = 0x3b20,
    .i2c_reg_len             = 0x3b30,
    .i2c_data_len            = 0x3b34,
    .i2c_ctrl                = 0x3b38,
    .i2c_status              = 0x3b3c,
    .i2c_err_vec             = 0x3b48,
    .i2c_data_buf            = 0x3b80,
    .dev_name                = "/dev/fpga0",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00008000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data0 = {
    .adap_nr                 = 20,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x1000,
    .i2c_filter              = 0x1004,
    .i2c_stretch             = 0x1008,
    .i2c_ext_9548_exits_flag = 0x100c,
    .i2c_ext_9548_addr       = 0x1010,
    .i2c_ext_9548_chan       = 0x1014,
    .i2c_in_9548_chan        = 0x1018,
    .i2c_slave               = 0x101c,
    .i2c_reg                 = 0x1020,
    .i2c_reg_len             = 0x1030,
    .i2c_data_len            = 0x1034,
    .i2c_ctrl                = 0x1038,
    .i2c_status              = 0x103c,
    .i2c_err_vec             = 0x1048,
    .i2c_data_buf            = 0x1080,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x80,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
    .i2c_offset_reg          = 0xac,
    .i2c_data_buf_len_reg    = 0xa4,
};

static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data1 = {
    .adap_nr                 = 21,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x1100,
    .i2c_filter              = 0x1104,
    .i2c_stretch             = 0x1108,
    .i2c_ext_9548_exits_flag = 0x110c,
    .i2c_ext_9548_addr       = 0x1110,
    .i2c_ext_9548_chan       = 0x1114,
    .i2c_in_9548_chan        = 0x1118,
    .i2c_slave               = 0x111c,
    .i2c_reg                 = 0x1120,
    .i2c_reg_len             = 0x1130,
    .i2c_data_len            = 0x1134,
    .i2c_ctrl                = 0x1138,
    .i2c_status              = 0x113c,
    .i2c_err_vec             = 0x1148,
    .i2c_data_buf            = 0x1180,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x84,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data2 = {
    .adap_nr                 = 22,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x1200,
    .i2c_filter              = 0x1204,
    .i2c_stretch             = 0x1208,
    .i2c_ext_9548_exits_flag = 0x120c,
    .i2c_ext_9548_addr       = 0x1210,
    .i2c_ext_9548_chan       = 0x1214,
    .i2c_in_9548_chan        = 0x1218,
    .i2c_slave               = 0x121c,
    .i2c_reg                 = 0x1220,
    .i2c_reg_len             = 0x1230,
    .i2c_data_len            = 0x1234,
    .i2c_ctrl                = 0x1238,
    .i2c_status              = 0x123c,
    .i2c_err_vec             = 0x1248,
    .i2c_data_buf            = 0x1280,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x88,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data3 = {
    .adap_nr                 = 23,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x1300,
    .i2c_filter              = 0x1304,
    .i2c_stretch             = 0x1308,
    .i2c_ext_9548_exits_flag = 0x130c,
    .i2c_ext_9548_addr       = 0x1310,
    .i2c_ext_9548_chan       = 0x1314,
    .i2c_in_9548_chan        = 0x1318,
    .i2c_slave               = 0x131c,
    .i2c_reg                 = 0x1320,
    .i2c_reg_len             = 0x1330,
    .i2c_data_len            = 0x1334,
    .i2c_ctrl                = 0x1338,
    .i2c_status              = 0x133c,
    .i2c_err_vec             = 0x1348,
    .i2c_data_buf            = 0x1380,
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x8c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data4 = {
    .adap_nr                 = 24,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x4c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data5 = {
    .adap_nr                 = 25,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x50,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data0 = {
    .adap_nr                 = 26,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data1 = {
    .adap_nr                 = 27,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000002,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data2 = {
    .adap_nr                 = 28,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000004,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data3 = {
    .adap_nr                 = 29,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000008,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data4 = {
    .adap_nr                 = 30,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000010,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data5 = {
    .adap_nr                 = 31,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000020,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data6 = {
    .adap_nr                 = 32,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000040,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data7 = {
    .adap_nr                 = 33,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000080,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data8 = {
    .adap_nr                 = 34,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000100,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data9 = {
    .adap_nr                 = 35,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000200,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data10 = {
    .adap_nr                 = 36,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000400,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data11 = {
    .adap_nr                 = 37,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000800,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data12 = {
    .adap_nr                 = 38,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00001000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data13 = {
    .adap_nr                 = 39,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00002000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data14 = {
    .adap_nr                 = 40,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00004000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data15 = {
    .adap_nr                 = 41,
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
    .dev_name                = "/dev/fpga1",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00008000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_i2c_bus_device_data0 = {
    .adap_nr                 = 42,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x300,
    .i2c_filter              = 0x304,
    .i2c_stretch             = 0x308,
    .i2c_ext_9548_exits_flag = 0x30c,
    .i2c_ext_9548_addr       = 0x310,
    .i2c_ext_9548_chan       = 0x314,
    .i2c_in_9548_chan        = 0x318,
    .i2c_slave               = 0x31c,
    .i2c_reg                 = 0x320,
    .i2c_reg_len             = 0x330,
    .i2c_data_len            = 0x334,
    .i2c_ctrl                = 0x338,
    .i2c_status              = 0x33c,
    .i2c_err_vec             = 0x348,
    .i2c_data_buf            = 0x380,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x80,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_i2c_bus_device_data1 = {
    .adap_nr                 = 43,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x400,
    .i2c_filter              = 0x404,
    .i2c_stretch             = 0x408,
    .i2c_ext_9548_exits_flag = 0x40c,
    .i2c_ext_9548_addr       = 0x410,
    .i2c_ext_9548_chan       = 0x414,
    .i2c_in_9548_chan        = 0x418,
    .i2c_slave               = 0x41c,
    .i2c_reg                 = 0x420,
    .i2c_reg_len             = 0x430,
    .i2c_data_len            = 0x434,
    .i2c_ctrl                = 0x438,
    .i2c_status              = 0x43c,
    .i2c_err_vec             = 0x448,
    .i2c_data_buf            = 0x480,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x80,
    .i2c_reset_on            = 0x00000002,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data0 = {
    .adap_nr                 = 44,
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
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data1 = {
    .adap_nr                 = 45,
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
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000002,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data2 = {
    .adap_nr                 = 46,
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
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000004,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data3 = {
    .adap_nr                 = 47,
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
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000008,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data4 = {
    .adap_nr                 = 48,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3000,
    .i2c_filter              = 0x3004,
    .i2c_stretch             = 0x3008,
    .i2c_ext_9548_exits_flag = 0x300c,
    .i2c_ext_9548_addr       = 0x3010,
    .i2c_ext_9548_chan       = 0x3014,
    .i2c_in_9548_chan        = 0x3018,
    .i2c_slave               = 0x301c,
    .i2c_reg                 = 0x3020,
    .i2c_reg_len             = 0x3030,
    .i2c_data_len            = 0x3034,
    .i2c_ctrl                = 0x3038,
    .i2c_status              = 0x303c,
    .i2c_err_vec             = 0x3048,
    .i2c_data_buf            = 0x3080,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000010,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data5 = {
    .adap_nr                 = 49,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3100,
    .i2c_filter              = 0x3104,
    .i2c_stretch             = 0x3108,
    .i2c_ext_9548_exits_flag = 0x310c,
    .i2c_ext_9548_addr       = 0x3110,
    .i2c_ext_9548_chan       = 0x3114,
    .i2c_in_9548_chan        = 0x3118,
    .i2c_slave               = 0x311c,
    .i2c_reg                 = 0x3120,
    .i2c_reg_len             = 0x3130,
    .i2c_data_len            = 0x3134,
    .i2c_ctrl                = 0x3138,
    .i2c_status              = 0x313c,
    .i2c_err_vec             = 0x3148,
    .i2c_data_buf            = 0x3180,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000020,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data6 = {
    .adap_nr                 = 50,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3200,
    .i2c_filter              = 0x3204,
    .i2c_stretch             = 0x3208,
    .i2c_ext_9548_exits_flag = 0x320c,
    .i2c_ext_9548_addr       = 0x3210,
    .i2c_ext_9548_chan       = 0x3214,
    .i2c_in_9548_chan        = 0x3218,
    .i2c_slave               = 0x321c,
    .i2c_reg                 = 0x3220,
    .i2c_reg_len             = 0x3230,
    .i2c_data_len            = 0x3234,
    .i2c_ctrl                = 0x3238,
    .i2c_status              = 0x323c,
    .i2c_err_vec             = 0x3248,
    .i2c_data_buf            = 0x3280,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000040,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data7 = {
    .adap_nr                 = 51,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3300,
    .i2c_filter              = 0x3304,
    .i2c_stretch             = 0x3308,
    .i2c_ext_9548_exits_flag = 0x330c,
    .i2c_ext_9548_addr       = 0x3310,
    .i2c_ext_9548_chan       = 0x3314,
    .i2c_in_9548_chan        = 0x3318,
    .i2c_slave               = 0x331c,
    .i2c_reg                 = 0x3320,
    .i2c_reg_len             = 0x3330,
    .i2c_data_len            = 0x3334,
    .i2c_ctrl                = 0x3338,
    .i2c_status              = 0x333c,
    .i2c_err_vec             = 0x3348,
    .i2c_data_buf            = 0x3380,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000080,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data8 = {
    .adap_nr                 = 52,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3400,
    .i2c_filter              = 0x3404,
    .i2c_stretch             = 0x3408,
    .i2c_ext_9548_exits_flag = 0x340c,
    .i2c_ext_9548_addr       = 0x3410,
    .i2c_ext_9548_chan       = 0x3414,
    .i2c_in_9548_chan        = 0x3418,
    .i2c_slave               = 0x341c,
    .i2c_reg                 = 0x3420,
    .i2c_reg_len             = 0x3430,
    .i2c_data_len            = 0x3434,
    .i2c_ctrl                = 0x3438,
    .i2c_status              = 0x343c,
    .i2c_err_vec             = 0x3448,
    .i2c_data_buf            = 0x3480,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000100,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data9 = {
    .adap_nr                 = 53,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3500,
    .i2c_filter              = 0x3504,
    .i2c_stretch             = 0x3508,
    .i2c_ext_9548_exits_flag = 0x350c,
    .i2c_ext_9548_addr       = 0x3510,
    .i2c_ext_9548_chan       = 0x3514,
    .i2c_in_9548_chan        = 0x3518,
    .i2c_slave               = 0x351c,
    .i2c_reg                 = 0x3520,
    .i2c_reg_len             = 0x3530,
    .i2c_data_len            = 0x3534,
    .i2c_ctrl                = 0x3538,
    .i2c_status              = 0x353c,
    .i2c_err_vec             = 0x3548,
    .i2c_data_buf            = 0x3580,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000200,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data10 = {
    .adap_nr                 = 54,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3600,
    .i2c_filter              = 0x3604,
    .i2c_stretch             = 0x3608,
    .i2c_ext_9548_exits_flag = 0x360c,
    .i2c_ext_9548_addr       = 0x3610,
    .i2c_ext_9548_chan       = 0x3614,
    .i2c_in_9548_chan        = 0x3618,
    .i2c_slave               = 0x361c,
    .i2c_reg                 = 0x3620,
    .i2c_reg_len             = 0x3630,
    .i2c_data_len            = 0x3634,
    .i2c_ctrl                = 0x3638,
    .i2c_status              = 0x363c,
    .i2c_err_vec             = 0x3648,
    .i2c_data_buf            = 0x3680,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000400,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data11 = {
    .adap_nr                 = 55,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3700,
    .i2c_filter              = 0x3704,
    .i2c_stretch             = 0x3708,
    .i2c_ext_9548_exits_flag = 0x370c,
    .i2c_ext_9548_addr       = 0x3710,
    .i2c_ext_9548_chan       = 0x3714,
    .i2c_in_9548_chan        = 0x3718,
    .i2c_slave               = 0x371c,
    .i2c_reg                 = 0x3720,
    .i2c_reg_len             = 0x3730,
    .i2c_data_len            = 0x3734,
    .i2c_ctrl                = 0x3738,
    .i2c_status              = 0x373c,
    .i2c_err_vec             = 0x3748,
    .i2c_data_buf            = 0x3780,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000800,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data12 = {
    .adap_nr                 = 56,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3800,
    .i2c_filter              = 0x3804,
    .i2c_stretch             = 0x3808,
    .i2c_ext_9548_exits_flag = 0x380c,
    .i2c_ext_9548_addr       = 0x3810,
    .i2c_ext_9548_chan       = 0x3814,
    .i2c_in_9548_chan        = 0x3818,
    .i2c_slave               = 0x381c,
    .i2c_reg                 = 0x3820,
    .i2c_reg_len             = 0x3830,
    .i2c_data_len            = 0x3834,
    .i2c_ctrl                = 0x3838,
    .i2c_status              = 0x383c,
    .i2c_err_vec             = 0x3848,
    .i2c_data_buf            = 0x3880,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00001000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data13 = {
    .adap_nr                 = 57,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3900,
    .i2c_filter              = 0x3904,
    .i2c_stretch             = 0x3908,
    .i2c_ext_9548_exits_flag = 0x390c,
    .i2c_ext_9548_addr       = 0x3910,
    .i2c_ext_9548_chan       = 0x3914,
    .i2c_in_9548_chan        = 0x3918,
    .i2c_slave               = 0x391c,
    .i2c_reg                 = 0x3920,
    .i2c_reg_len             = 0x3930,
    .i2c_data_len            = 0x3934,
    .i2c_ctrl                = 0x3938,
    .i2c_status              = 0x393c,
    .i2c_err_vec             = 0x3948,
    .i2c_data_buf            = 0x3980,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00002000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data14 = {
    .adap_nr                 = 58,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3a00,
    .i2c_filter              = 0x3a04,
    .i2c_stretch             = 0x3a08,
    .i2c_ext_9548_exits_flag = 0x3a0c,
    .i2c_ext_9548_addr       = 0x3a10,
    .i2c_ext_9548_chan       = 0x3a14,
    .i2c_in_9548_chan        = 0x3a18,
    .i2c_slave               = 0x3a1c,
    .i2c_reg                 = 0x3a20,
    .i2c_reg_len             = 0x3a30,
    .i2c_data_len            = 0x3a34,
    .i2c_ctrl                = 0x3a38,
    .i2c_status              = 0x3a3c,
    .i2c_err_vec             = 0x3a48,
    .i2c_data_buf            = 0x3a80,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00004000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data15 = {
    .adap_nr                 = 59,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x3b00,
    .i2c_filter              = 0x3b04,
    .i2c_stretch             = 0x3b08,
    .i2c_ext_9548_exits_flag = 0x3b0c,
    .i2c_ext_9548_addr       = 0x3b10,
    .i2c_ext_9548_chan       = 0x3b14,
    .i2c_in_9548_chan        = 0x3b18,
    .i2c_slave               = 0x3b1c,
    .i2c_reg                 = 0x3b20,
    .i2c_reg_len             = 0x3b30,
    .i2c_data_len            = 0x3b34,
    .i2c_ctrl                = 0x3b38,
    .i2c_status              = 0x3b3c,
    .i2c_err_vec             = 0x3b48,
    .i2c_data_buf            = 0x3b80,
    .dev_name                = "/dev/fpga2",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 2,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00008000,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static void rg_fpga_i2c_bus_device_release(struct device *dev)
{
    return;
}

static struct platform_device fpga_i2c_bus_device[] = {
    {
        .name   = "rg-fpga-i2c",
        .id = 1,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data0,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 2,
        .dev    = {
            .platform_data  = &fpga0_i2c_bus_device_data1,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 3,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data0,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 4,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data1,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 5,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data2,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 6,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data3,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 7,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data4,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 8,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data5,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 9,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data6,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 10,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data7,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 11,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data8,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 12,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data9,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 13,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data10,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 14,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data11,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 15,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data12,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 16,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data13,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 17,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data14,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 18,
        .dev    = {
            .platform_data  = &fpga0_dom_i2c_bus_device_data15,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 19,
        .dev    = {
            .platform_data  = &fpga1_i2c_bus_device_data0,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 20,
        .dev    = {
            .platform_data  = &fpga1_i2c_bus_device_data1,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 21,
        .dev    = {
            .platform_data  = &fpga1_i2c_bus_device_data2,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 22,
        .dev    = {
            .platform_data  = &fpga1_i2c_bus_device_data3,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 23,
        .dev    = {
            .platform_data  = &fpga1_i2c_bus_device_data4,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 24,
        .dev    = {
            .platform_data  = &fpga1_i2c_bus_device_data5,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 25,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data0,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 26,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data1,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 27,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data2,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 28,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data3,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 29,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data4,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 30,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data5,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 31,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data6,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 32,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data7,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 33,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data8,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 34,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data9,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 35,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data10,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 36,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data11,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 37,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data12,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 38,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data13,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 39,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data14,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 40,
        .dev    = {
            .platform_data  = &fpga1_dom_i2c_bus_device_data15,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 41,
        .dev    = {
            .platform_data  = &fpga2_i2c_bus_device_data0,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 42,
        .dev    = {
            .platform_data  = &fpga2_i2c_bus_device_data1,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 43,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data0,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 44,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data1,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 45,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data2,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 46,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data3,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 47,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data4,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 48,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data5,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 49,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data6,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 50,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data7,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 51,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data8,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 52,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data9,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 53,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data10,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 54,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data11,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 55,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data12,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 56,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data13,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 57,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data14,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
    {
        .name   = "rg-fpga-i2c",
        .id = 58,
        .dev    = {
            .platform_data  = &fpga2_dom_i2c_bus_device_data15,
            .release = rg_fpga_i2c_bus_device_release,
        },
    },
};

static int __init rg_fpga_i2c_bus_device_init(void)
{
    int i;
    int ret = 0;
    fpga_i2c_bus_device_t *fpga_i2c_bus_device_data;

    RG_FPGA_I2C_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(fpga_i2c_bus_device); i++) {
        fpga_i2c_bus_device_data = fpga_i2c_bus_device[i].dev.platform_data;
        ret = platform_device_register(&fpga_i2c_bus_device[i]);
        if (ret < 0) {
            fpga_i2c_bus_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "rg-fpga-i2c.%d register failed!\n", i + 1);
        } else {
            fpga_i2c_bus_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit rg_fpga_i2c_bus_device_exit(void)
{
    int i;
    fpga_i2c_bus_device_t *fpga_i2c_bus_device_data;

    RG_FPGA_I2C_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(fpga_i2c_bus_device) - 1; i >= 0; i--) {
        fpga_i2c_bus_device_data = fpga_i2c_bus_device[i].dev.platform_data;
        if (fpga_i2c_bus_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&fpga_i2c_bus_device[i]);
        }
    }
}

module_init(rg_fpga_i2c_bus_device_init);
module_exit(rg_fpga_i2c_bus_device_exit);
MODULE_DESCRIPTION("FPGA I2C Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic_rd@ruijie.com.cn");
