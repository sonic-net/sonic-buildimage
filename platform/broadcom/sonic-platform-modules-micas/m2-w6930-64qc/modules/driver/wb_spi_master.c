/*
 * An wb_spi_master driver for spi master device function
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
#include <linux/device.h>
#include <linux/device/bus.h>

#include "wb_spi_master.h"

static int g_wb_spi_master_debug = 0;
static int g_wb_spi_master_error = 0;

module_param(g_wb_spi_master_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_spi_master_error, int, S_IRUGO | S_IWUSR);

#define WB_SPI_MASTER_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_spi_master_debug) { \
        printk(KERN_INFO "[WB_SPI_MASTER][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_SPI_MASTER_ERROR(fmt, args...) do {                                        \
    if (g_wb_spi_master_error) { \
        printk(KERN_ERR "[WB_SPI_MASTER][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static struct device g_wb_spi_device;
/* point to spi_master_class in drivers/spi/spi.c */
static struct class  *g_wb_spi_master_class = NULL;
static struct spi_controller *g_wb_spi_controller = NULL;

static int __spi_controller_match(struct device *dev, const void *data)
{
    struct spi_controller *ctlr;
    const u16 *bus_num = data;

    ctlr = container_of(dev, struct spi_controller, dev);
    return ctlr->bus_num == *bus_num;
}

/**
 * wb_spi_master_busnum_to_master - look up master associated with bus_num
 * @bus_num: the master's bus number
 * Context: can sleep
 *
 * Return: the SPI master structure on success, else NULL.
 */
struct spi_controller *wb_spi_master_busnum_to_master(u16 bus_num)
{
    struct device *dev;
	struct spi_controller *ctlr = NULL;
    
    WB_SPI_MASTER_VERBOSE("Enter.\n");

    if (g_wb_spi_master_class == NULL) {
        WB_SPI_MASTER_ERROR("get g_wb_spi_master_class fail.\n");
        return NULL;
    }
    
    dev = class_find_device(g_wb_spi_master_class, NULL, &bus_num, __spi_controller_match);
    if (dev) {
        ctlr = container_of(dev, struct spi_controller, dev);
    }
    /* reference got in class_find_device */
    return ctlr;
}
EXPORT_SYMBOL_GPL(wb_spi_master_busnum_to_master);

static int __init wb_spi_master_init(void)
{
    struct device *dev;
    struct spi_board_info chip;

    WB_SPI_MASTER_VERBOSE("Enter!\n");

    device_initialize(&g_wb_spi_device);

    g_wb_spi_controller = spi_alloc_master(&g_wb_spi_device, sizeof(struct spi_board_info));
    if (g_wb_spi_controller == NULL) {
        WB_SPI_MASTER_ERROR("spi_alloc_master failed.\n");
        return -ENOMEM;
    }

    g_wb_spi_master_class = g_wb_spi_controller->dev.class;
    if (g_wb_spi_master_class == NULL) {
        WB_SPI_MASTER_ERROR("get class_spi_master failed.\n");
        kfree(g_wb_spi_controller);
        g_wb_spi_controller = NULL;
        return -EINVAL;
    }
    printk(KERN_INFO "WB spi master init success.");

    return 0;
}

static void __exit wb_spi_master_exit(void)
{
    WB_SPI_MASTER_VERBOSE("Enter!\n");
    if (g_wb_spi_controller != NULL) {
        kfree(g_wb_spi_controller);
    }

    return;
}

module_init(wb_spi_master_init);
module_exit(wb_spi_master_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("create spi device");
MODULE_LICENSE("GPL");
