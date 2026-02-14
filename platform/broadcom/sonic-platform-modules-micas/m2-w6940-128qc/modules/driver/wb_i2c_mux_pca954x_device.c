/*
 * An wb_i2c_mux_pca954x_device driver for pca954x i2c load device function
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

static int g_wb_i2c_mux_pca954x_device_debug = 0;
static int g_wb_i2c_mux_pca954x_device_error = 0;

module_param(g_wb_i2c_mux_pca954x_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_i2c_mux_pca954x_device_error, int, S_IRUGO | S_IWUSR);

#define WB_I2C_MUX_PCA954X_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_i2c_mux_pca954x_device_debug) { \
        printk(KERN_INFO "[WB_I2C_MUX_PCA954X_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_I2C_MUX_PCA954X_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_i2c_mux_pca954x_device_error) { \
        printk(KERN_ERR "[WB_I2C_MUX_PCA954X_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data0 = {
    .i2c_bus                        = 1,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 25,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000001,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000001,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data1 = {
    .i2c_bus                        = 2,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 33,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000002,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000002,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data2 = {
    .i2c_bus                        = 3,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 41,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000004,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000004,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data3 = {
    .i2c_bus                        = 4,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 49,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000008,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000008,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data4 = {
    .i2c_bus                        = 5,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 57,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000010,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000010,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data5 = {
    .i2c_bus                        = 6,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 65,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000020,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000020,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data6 = {
    .i2c_bus                        = 7,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 73,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000040,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000040,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data7 = {
    .i2c_bus                        = 8,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 81,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000080,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000080,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data8 = {
    .i2c_bus                        = 9,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 89,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000100,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000100,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data9 = {
    .i2c_bus                        = 10,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 97,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000200,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000200,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data10 = {
    .i2c_bus                        = 11,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 105,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000400,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000400,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data11 = {
    .i2c_bus                        = 12,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 113,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00000800,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00000800,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data12 = {
    .i2c_bus                        = 13,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 121,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00001000,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00001000,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data13 = {
    .i2c_bus                        = 14,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 129,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00002000,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00002000,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data14 = {
    .i2c_bus                        = 15,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 137,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00004000,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00004000,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data15 = {
    .i2c_bus                        = 16,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 145,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00008000,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00008000,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data16 = {
    .i2c_bus                        = 21,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 153,
    .pca9548_reset_type             = PCA9548_RESET_FILE,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .file_attr.dev_name         = "/dev/fpga0",
        .file_attr.offset           = 0x84,
        .file_attr.mask             = 0x00010000,
        .file_attr.reset_on         = 0x00000000,
        .file_attr.reset_off        = 0x00010000,
        .file_attr.width            = 4,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data17 = {
    .i2c_bus                        = 22,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 161,
};


struct i2c_board_info i2c_mux_pca954x_device_info[] = {
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data0,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data1,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data2,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data3,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data4,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data5,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data6,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data7,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data8,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data9,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data10,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data11,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data12,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data13,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data14,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data15,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data16,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data17,
    },
};

static int __init wb_i2c_mux_pca954x_device_init(void)
{
    int i;
    struct i2c_adapter *adap;
    struct i2c_client *client;
    i2c_mux_pca954x_device_t *i2c_mux_pca954x_device_data;

    WB_I2C_MUX_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(i2c_mux_pca954x_device_info); i++) {
        i2c_mux_pca954x_device_data = i2c_mux_pca954x_device_info[i].platform_data;
        i2c_mux_pca954x_device_info[i].addr = i2c_mux_pca954x_device_data->i2c_addr;
        adap = i2c_get_adapter(i2c_mux_pca954x_device_data->i2c_bus);
        if (adap == NULL) {
            i2c_mux_pca954x_device_data->client = NULL;
            printk(KERN_ERR "get i2c bus %d adapter fail.\n", i2c_mux_pca954x_device_data->i2c_bus);
            continue;
        }
        client = i2c_new_client_device(adap, &i2c_mux_pca954x_device_info[i]);
        if (!client) {
            i2c_mux_pca954x_device_data->client = NULL;
            printk(KERN_ERR "Failed to register pca954x device %d at bus %d!\n",
                i2c_mux_pca954x_device_data->i2c_addr, i2c_mux_pca954x_device_data->i2c_bus);
        } else {
            i2c_mux_pca954x_device_data->client = client;
        }
        i2c_put_adapter(adap);
    }
    return 0;
}

static void __exit wb_i2c_mux_pca954x_device_exit(void)
{
    int i;
    i2c_mux_pca954x_device_t *i2c_mux_pca954x_device_data;

    WB_I2C_MUX_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(i2c_mux_pca954x_device_info) - 1; i >= 0; i--) {
        i2c_mux_pca954x_device_data = i2c_mux_pca954x_device_info[i].platform_data;
        if (i2c_mux_pca954x_device_data->client) {
            i2c_unregister_device(i2c_mux_pca954x_device_data->client);
            i2c_mux_pca954x_device_data->client = NULL;
        }
    }
}

module_init(wb_i2c_mux_pca954x_device_init);
module_exit(wb_i2c_mux_pca954x_device_exit);
MODULE_DESCRIPTION("I2C MUX PCA954X Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
