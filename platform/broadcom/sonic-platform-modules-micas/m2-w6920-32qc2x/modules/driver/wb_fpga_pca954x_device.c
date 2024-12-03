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


static fpga_pca954x_device_t fpga_pca954x_device_data1 = {
    .i2c_bus = 3,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 26,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data2 = {
    .i2c_bus = 4,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 27,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data3 = {
    .i2c_bus = 27,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 39,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data4 = {
    .i2c_bus =5,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 28,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data5 = {
    .i2c_bus = 28,
    .i2c_addr = 0x71,
    .pca9548_base_nr = 47,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data6 = {
    .i2c_bus = 6,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 29,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data7 = {
    .i2c_bus = 29,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 55,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data8 = {
    .i2c_bus = 7,
    .i2c_addr = 0x71,
    .pca9548_base_nr = 30,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data9 = {
    .i2c_bus = 8,
    .i2c_addr = 0x70,
    .pca9548_base_nr =38,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data10 = {
    .i2c_bus = 9,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 63,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data11 = {
    .i2c_bus = 10,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 65,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data12 = {
    .i2c_bus = 11,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 67,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data13 = {
    .i2c_bus = 12,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 69,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data14 = {
    .i2c_bus = 13,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 71,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data15 = {
    .i2c_bus = 14,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 73,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data16 = {
    .i2c_bus = 15,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 75,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data17 = {
    .i2c_bus = 16,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 77,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data18 = {
    .i2c_bus = 17,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 79,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data19 = {
    .i2c_bus = 18,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 81,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data20 = {
    .i2c_bus = 19,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 83,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data21 = {
    .i2c_bus = 20,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 85,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data22 = {
    .i2c_bus = 21,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 87,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data23 = {
    .i2c_bus = 22,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 89,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data24 = {
    .i2c_bus = 23,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 91,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data25 = {
    .i2c_bus = 24,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 93,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data26 = {
    .i2c_bus = 25,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 95,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

struct i2c_board_info fpga_pca954x_device_info[] = {
    {
        .type = "wb_fpga_pca9541",
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
        .type = "wb_fpga_pca9541",
        .platform_data = &fpga_pca954x_device_data9,
    },
    {
        .type = "wb_fpga_pca9542",
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
