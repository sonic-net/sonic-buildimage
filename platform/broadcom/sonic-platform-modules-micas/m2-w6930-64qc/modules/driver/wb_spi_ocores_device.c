/*
 * An wb_spi_ocores_device driver for spi ocores device function
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

#include <wb_spi_ocores.h>

static int g_wb_spi_ocores_device_debug = 0;
static int g_wb_spi_ocores_device_error = 0;

module_param(g_wb_spi_ocores_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_spi_ocores_device_error, int, S_IRUGO | S_IWUSR);

#define WB_SPI_OCORE_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_spi_ocores_device_debug) { \
        printk(KERN_INFO "[WB_SPI_OCORE_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_SPI_OCORE_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_spi_ocores_device_error) { \
        printk(KERN_ERR "[WB_SPI_OCORE_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static spi_ocores_device_t spi_ocores_device_data0 = {
    .bus_num = 1,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 2,
    .dev_base = 0x1900,
    .reg_shift = 2,
    .reg_io_width = 4,
    .clock_frequency = 125000000,
    .irq_flag = 0,
    .num_chipselect = 8,
};

static void wb_spi_ocores_device_release(struct device *dev)
{
    return;
}

static struct platform_device spi_ocores_device[] = {
    {
        .name   = "wb-spioc",
        .id = 1,
        .dev    = {
            .platform_data  = &spi_ocores_device_data0,
            .release = wb_spi_ocores_device_release,
        },
    },
};

static int __init wb_spi_ocores_device_init(void)
{
    int i;
    int ret = 0;
    spi_ocores_device_t *spi_ocores_device_data;

    WB_SPI_OCORE_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(spi_ocores_device); i++) {
        spi_ocores_device_data = spi_ocores_device[i].dev.platform_data;
        ret = platform_device_register(&spi_ocores_device[i]);
        if (ret < 0) {
            spi_ocores_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-spi-oc.%d register failed!\n", i + 1);
        } else {
            spi_ocores_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_spi_ocores_device_exit(void)
{
    int i;
    spi_ocores_device_t *spi_ocores_device_data;

    WB_SPI_OCORE_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(spi_ocores_device) - 1; i >= 0; i--) {
        spi_ocores_device_data = spi_ocores_device[i].dev.platform_data;
        if (spi_ocores_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&spi_ocores_device[i]);
        }
    }
}

module_init(wb_spi_ocores_device_init);
module_exit(wb_spi_ocores_device_exit);
MODULE_DESCRIPTION("SPI OCORES Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
