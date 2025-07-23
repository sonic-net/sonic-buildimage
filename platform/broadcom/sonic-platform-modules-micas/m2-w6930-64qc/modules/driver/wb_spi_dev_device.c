/*
 * An wb_spi_dev_device driver for spi device function
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
#include <linux/spi/spi.h>

#include <wb_spi_dev.h>
#include "wb_spi_master.h"

#define SPI_DEVICE_MAX_NUM (64)

static int g_wb_spi_dev_device_debug = 0;
static int g_wb_spi_dev_device_error = 0;

module_param(g_wb_spi_dev_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_spi_dev_device_error, int, S_IRUGO | S_IWUSR);

#define WB_SPI_DEV_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_spi_dev_device_debug) { \
        printk(KERN_INFO "[WB_SPI_DEV_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_SPI_DEV_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_spi_dev_device_error) { \
        printk(KERN_ERR "[WB_SPI_DEV_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static spi_dev_device_t spi_dev_device_data0 = {
    .spi_dev_name = "fpga1",
    .data_bus_width = 4,
    .addr_bus_width = 2,
    .per_rd_len = 4,
    .per_wr_len = 4,
    .spi_len = 0xe000,
};

static struct spi_device *g_spi_device[SPI_DEVICE_MAX_NUM];

struct spi_board_info spi_dev_device_info[] = {
    {
        .modalias           = "wb-spi-dev",
        .max_speed_hz       = 6250000,
        .bus_num            = 1,
        .chip_select        = 0,
        .mode               = SPI_MODE_3,
        .platform_data      = &spi_dev_device_data0,
    },
};

static int __init wb_spi_dev_device_init(void)
{
    int i;
    struct spi_master *master;
    struct spi_device *spi;
    int spi_dev_num;

    WB_SPI_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");

    spi_dev_num = ARRAY_SIZE(spi_dev_device_info);
    if (spi_dev_num > SPI_DEVICE_MAX_NUM) {
        printk(KERN_ERR "spi_dev_num[%d] is bigger than max_num[%d].\n",
            spi_dev_num, SPI_DEVICE_MAX_NUM);
        return -EINVAL;
    }

    for (i = 0; i < ARRAY_SIZE(spi_dev_device_info); i++) {
        master = wb_spi_master_busnum_to_master(spi_dev_device_info[i].bus_num);
        if (!master) {
            printk(KERN_ERR "get bus_num %u spi master failed.\n",
                spi_dev_device_info[i].bus_num);
            continue;
        }
        spi = spi_new_device(master, &spi_dev_device_info[i]);
        put_device(&master->dev);
        if (spi) {
            g_spi_device[i] = spi;
        } else {
            g_spi_device[i] = NULL;
            printk(KERN_ERR "Failed to register spi dev device %s at bus %d!\n",
                spi_dev_device_info[i].modalias, spi_dev_device_info[i].bus_num);
            continue;
        }
    }
    return 0;
}

static void __exit wb_spi_dev_device_exit(void)
{
    int i;

    WB_SPI_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(spi_dev_device_info) - 1; i >= 0; i--) {
        if (g_spi_device[i]) {
            spi_unregister_device(g_spi_device[i]);
            g_spi_device[i] = NULL;
        }
    }
}

module_init(wb_spi_dev_device_init);
module_exit(wb_spi_dev_device_exit);
MODULE_DESCRIPTION("SPI DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
