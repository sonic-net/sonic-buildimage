#include <linux/module.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <fpga_i2c.h>

static int g_rg_fpga_pca954x_device_debug = 0;
static int g_rg_fpga_pca954x_device_error = 0;

module_param(g_rg_fpga_pca954x_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_rg_fpga_pca954x_device_error, int, S_IRUGO | S_IWUSR);

#define RG_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_rg_fpga_pca954x_device_debug) { \
        printk(KERN_INFO "[RG_FPGA_PCA954X_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define RG_FPGA_PCA954X_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_rg_fpga_pca954x_device_error) { \
        printk(KERN_ERR "[RG_FPGA_PCA954X_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static fpga_pca954x_device_t fpga0_i2c0_pca954x_device_data = {
    .i2c_bus = 2,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 60,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_i2c1_pca954x_device_data = {
    .i2c_bus = 3,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 61, /* 61~64 */
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_i2c0_pca954x_device_data = {
    .i2c_bus = 20,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 65,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_i2c1_pca954x_device_data = {
    .i2c_bus = 21,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 66,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_i2c2_pca954x_device_data = {
    .i2c_bus = 22,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 67,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_i2c3_pca954x_device_data = {
    .i2c_bus = 23,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 68,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_i2c4_pca954x_device_data = {
    .i2c_bus = 24,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 69,  /* 69~76 */
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_i2c5_pca954x_device_data = {
    .i2c_bus = 25,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 77,  /* 77~78 */
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_i2c0_in0_pca954x_device_data = {
    .i2c_bus = 65,
    .i2c_addr = 0x76,
    .pca9548_base_nr      = 79,  /* 79~86 */
    .fpga_9548_flag       = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga1_i2c1_in0_pca954x_device_data = {
    .i2c_bus = 66,
    .i2c_addr = 0x77,
    .pca9548_base_nr      = 87, /* 87~94 */
    .fpga_9548_flag       = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga1_i2c2_in0_pca954x_device_data = {
    .i2c_bus = 67,
    .i2c_addr = 0x77,
    .pca9548_base_nr      = 95, /* 95~102 */
    .fpga_9548_flag       = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga1_i2c3_in0_pca954x_device_data = {
    .i2c_bus = 68,
    .i2c_addr = 0x77,
    .pca9548_base_nr      = 103, /* 103~110 */
    .fpga_9548_flag       = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga2_i2c0_pca954x_device_data = {
    .i2c_bus = 42,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 111,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_i2c1_pca954x_device_data = {
    .i2c_bus = 43,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 112, /* 112~115 */
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c0_pca954x_device_data = {
    .i2c_bus = 4,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 124,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c1_pca954x_device_data = {
    .i2c_bus = 5,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 126,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c2_pca954x_device_data = {
    .i2c_bus = 6,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 128,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c3_pca954x_device_data = {
    .i2c_bus = 7,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 130,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c4_pca954x_device_data = {
    .i2c_bus = 8,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 132,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c5_pca954x_device_data = {
    .i2c_bus = 9,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 134,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c6_pca954x_device_data = {
    .i2c_bus = 10,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 136,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c7_pca954x_device_data = {
    .i2c_bus = 11,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 138,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c8_pca954x_device_data = {
    .i2c_bus = 12,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 140,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c9_pca954x_device_data = {
    .i2c_bus = 13,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 142,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c10_pca954x_device_data = {
    .i2c_bus = 14,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 144,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c11_pca954x_device_data = {
    .i2c_bus = 15,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 146,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c12_pca954x_device_data = {
    .i2c_bus = 16,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 148,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c13_pca954x_device_data = {
    .i2c_bus = 17,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 150,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c14_pca954x_device_data = {
    .i2c_bus = 18,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 152,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga0_dom_i2c15_pca954x_device_data = {
    .i2c_bus = 19,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 154,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c0_pca954x_device_data = {
    .i2c_bus = 26,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 156,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c1_pca954x_device_data = {
    .i2c_bus = 27,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 160,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c2_pca954x_device_data = {
    .i2c_bus = 28,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 164,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c3_pca954x_device_data = {
    .i2c_bus = 29,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 168,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c4_pca954x_device_data = {
    .i2c_bus = 30,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 172,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c5_pca954x_device_data = {
    .i2c_bus = 31,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 176,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c6_pca954x_device_data = {
    .i2c_bus = 32,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 180,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c7_pca954x_device_data = {
    .i2c_bus = 33,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 184,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c8_pca954x_device_data = {
    .i2c_bus = 34,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 188,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c9_pca954x_device_data = {
    .i2c_bus = 35,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 192,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c10_pca954x_device_data = {
    .i2c_bus = 36,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 196,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c11_pca954x_device_data = {
    .i2c_bus = 37,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 200,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c12_pca954x_device_data = {
    .i2c_bus = 38,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 204,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c13_pca954x_device_data = {
    .i2c_bus = 39,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 208,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c14_pca954x_device_data = {
    .i2c_bus = 40,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 212,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga1_dom_i2c15_pca954x_device_data = {
    .i2c_bus = 41,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 216,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c0_pca954x_device_data = {
    .i2c_bus = 44,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 220,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c1_pca954x_device_data = {
    .i2c_bus = 45,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 222,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c2_pca954x_device_data = {
    .i2c_bus = 46,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 224,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c3_pca954x_device_data = {
    .i2c_bus = 47,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 226,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c4_pca954x_device_data = {
    .i2c_bus = 48,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 228,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c5_pca954x_device_data = {
    .i2c_bus = 49,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 230,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c6_pca954x_device_data = {
    .i2c_bus = 50,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 232,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c7_pca954x_device_data = {
    .i2c_bus = 51,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 234,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c8_pca954x_device_data = {
    .i2c_bus = 52,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 236,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c9_pca954x_device_data = {
    .i2c_bus = 53,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 238,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c10_pca954x_device_data = {
    .i2c_bus = 54,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 240,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c11_pca954x_device_data = {
    .i2c_bus = 55,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 242,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c12_pca954x_device_data = {
    .i2c_bus = 56,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 244,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c13_pca954x_device_data = {
    .i2c_bus = 57,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 246,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c14_pca954x_device_data = {
    .i2c_bus = 58,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 248,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga2_dom_i2c15_pca954x_device_data = {
    .i2c_bus = 59,
    .i2c_addr = 0x70,
    .pca9548_base_nr      = 250,
    .fpga_9548_flag       = 1,
    .fpga_9548_reset_flag = 0,
};

struct i2c_board_info fpga_pca954x_device_info[] = {
    {
        .type = "rg_fpga_pca9541",
        .platform_data = &fpga0_i2c0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga0_i2c1_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9541",
        .platform_data = &fpga1_i2c0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9541",
        .platform_data = &fpga1_i2c1_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9541",
        .platform_data = &fpga1_i2c2_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9541",
        .platform_data = &fpga1_i2c3_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9548",
        .platform_data = &fpga1_i2c4_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga1_i2c5_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9548",
        .platform_data = &fpga1_i2c0_in0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9548",
        .platform_data = &fpga1_i2c1_in0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9548",
        .platform_data = &fpga1_i2c2_in0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9548",
        .platform_data = &fpga1_i2c3_in0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9541",
        .platform_data = &fpga2_i2c0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga2_i2c1_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c1_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c2_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c3_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c4_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c5_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c6_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c7_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c8_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c9_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c10_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c11_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c12_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c13_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c14_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga0_dom_i2c15_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c1_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c2_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c3_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c4_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c5_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c6_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c7_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c8_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c9_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c10_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c11_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c12_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c13_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c14_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9545",
        .platform_data = &fpga1_dom_i2c15_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c0_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c1_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c2_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c3_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c4_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c5_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c6_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c7_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c8_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c9_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c10_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c11_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c12_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c13_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c14_pca954x_device_data,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga2_dom_i2c15_pca954x_device_data,
    },
};

static int __init rg_fpga_pca954x_device_init(void)
{
    int i;
    struct i2c_adapter *adap;
    struct i2c_client *client;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    RG_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
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

static void __exit rg_fpga_pca954x_device_exit(void)
{
    int i;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    RG_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(fpga_pca954x_device_info) - 1; i >= 0; i--) {
        fpga_pca954x_device_data = fpga_pca954x_device_info[i].platform_data;
        if (fpga_pca954x_device_data->client) {
            i2c_unregister_device(fpga_pca954x_device_data->client);
            fpga_pca954x_device_data->client = NULL;
        }
    }
}

module_init(rg_fpga_pca954x_device_init);
module_exit(rg_fpga_pca954x_device_exit);
MODULE_DESCRIPTION("RG FPGA PCA954X Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic_rd@ruijie.com.cn");
