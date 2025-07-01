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

/* CPLD-I2C-MASTER-1 */
static fpga_pca954x_device_t fpga_pca954x_device_data0 = {
    .i2c_bus = 2,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 27,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data1 = {
    .i2c_bus = 27,
    .i2c_addr = 0x76,
    .pca9548_base_nr = 42,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

/* CPLD-I2C-MASTER-4 */
static fpga_pca954x_device_t fpga_pca954x_device_data2 = {
    .i2c_bus = 3,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 28,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data3 = {
    .i2c_bus = 28,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 50,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

/* CPLD-I2C-MASTER-2 */
static fpga_pca954x_device_t fpga_pca954x_device_data4 = {
    .i2c_bus = 4,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 29,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data5 = {
    .i2c_bus =29,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 58,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

/* CPLD-I2C-MASTER-3 */
static fpga_pca954x_device_t fpga_pca954x_device_data6 = {
    .i2c_bus = 5,
    .i2c_addr = 0x71,
    .pca9548_base_nr = 30,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data7 = {
    .i2c_bus = 30,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 66,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

/* fpga-i2c-1 */
static fpga_pca954x_device_t fpga_pca954x_device_data8 = {
    .i2c_bus = 6,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 31,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data9 = {
    .i2c_bus = 31,
    .i2c_addr = 0x76,
    .pca9548_base_nr = 74,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data10 = {
    .i2c_bus = 7,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 32,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data11 = {
    .i2c_bus = 32,
    .i2c_addr = 0x76,
    .pca9548_base_nr = 82,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data12 = {
    .i2c_bus = 8,
    .i2c_addr = 0x71,
    .pca9548_base_nr = 33,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data13 = {
    .i2c_bus = 33,
    .i2c_addr = 0x76,
    .pca9548_base_nr = 90,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data14 = {
    .i2c_bus = 9,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 98,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data15 = {
    .i2c_bus = 10,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 102,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data16 = {
    .i2c_bus = 11,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 106,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data17 = {
    .i2c_bus = 12,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 110,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data18 = {
    .i2c_bus = 13,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 114,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data19 = {
    .i2c_bus = 14,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 118,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data20 = {
    .i2c_bus = 15,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 122,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data21 = {
    .i2c_bus = 16,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 126,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data22 = {
    .i2c_bus = 17,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 130,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data23 = {
    .i2c_bus = 18,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 134,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data24 = {
    .i2c_bus = 19,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 138,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data25 = {
    .i2c_bus = 20,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 142,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data26 = {
    .i2c_bus = 21,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 146,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data27 = {
    .i2c_bus = 22,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 150,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data28 = {
    .i2c_bus = 23,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 154,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data29 = {
    .i2c_bus = 24,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 158,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data30 = {
    .i2c_bus = 25,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 162,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data31 = {
    .i2c_bus = 26,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 166,
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
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data8,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data9,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data10,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data11,
    },
    {
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data12,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data13,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data14,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data15,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data16,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data17,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data18,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data19,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data20,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data21,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data22,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data23,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data24,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data25,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data26,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data27,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data28,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data29,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data30,
    },
    {
        .type = "wb_fpga_pca9544",
        .platform_data = &fpga_pca954x_device_data31,
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
