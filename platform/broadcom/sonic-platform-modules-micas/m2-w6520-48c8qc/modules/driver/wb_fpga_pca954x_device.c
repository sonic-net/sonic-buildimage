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

#include <wb_i2c_mux_pca954x.h>
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
    .i2c_bus = 3,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 62,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data1 = {
    .i2c_bus = 4,
    .i2c_addr = 0x71,
    .pca9548_base_nr = 70,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data2 = {
    .i2c_bus = 5,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 78,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

struct i2c_board_info fpga_pca954x_device_info[] = {
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data0,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data1,
    },
    {
        .type = "wb_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data2,
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
