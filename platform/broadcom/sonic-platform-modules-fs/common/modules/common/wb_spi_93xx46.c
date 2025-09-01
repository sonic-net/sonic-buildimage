/*
 * EEPROMs access control driver for display configuration EEPROMs
 * on DigsyMTC board.
 *
 * (C) 2011 DENX Software Engineering, Anatolij Gustschin <agust@denx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/eeprom_93xx46.h>
#include <wb_spi_master.h>
#include <wb_bsp_kernel_debug.h>

#define DEFAULT_SPI_BUS_NUM     (0)
#define DEFAULT_SPI_CS_NUM      (0)
#define DEFAULT_SPI_HZ          (100000)

#define GPIO_EEPROM_CS          (-1)

int spi_bus_num = DEFAULT_SPI_BUS_NUM;
int spi_cs_gpio = GPIO_EEPROM_CS;
int spi_hz = DEFAULT_SPI_HZ;

module_param(spi_bus_num, int, S_IRUGO | S_IWUSR);
module_param(spi_cs_gpio, int, S_IRUGO | S_IWUSR);
module_param(spi_hz, int, S_IRUGO | S_IWUSR);

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

struct eeprom_93xx46_platform_data eeprom_data = {
    .flags      = EE_ADDR16,
    .quirks     = EEPROM_93XX46_QUIRK_SINGLE_WORD_READ,
};

struct spi_board_info eeprom_93xx46_info __initdata = {
        .modalias           = "wb_93xx46",
        .max_speed_hz       = DEFAULT_SPI_HZ,
        .bus_num            = DEFAULT_SPI_BUS_NUM,
        .chip_select        = DEFAULT_SPI_CS_NUM,
        .mode               = SPI_MODE_0 | SPI_CS_HIGH,
        .controller_data    = (void *)GPIO_EEPROM_CS,
        .platform_data      = &eeprom_data,
};

static struct spi_device *g_spi_device;

static int __init wb_spi_93xx46_init(void)
{
    struct spi_master *master;

    DEBUG_VERBOSE("Enter.\n");

    eeprom_93xx46_info.bus_num = spi_bus_num;
    eeprom_93xx46_info.controller_data = (void *)(long)spi_cs_gpio;
    eeprom_93xx46_info.max_speed_hz = spi_hz;
    master = wb_spi_master_busnum_to_master(eeprom_93xx46_info.bus_num);
    if (!master) {
        DEBUG_ERROR("get bus_num %u spi master failed.\n",
            eeprom_93xx46_info.bus_num);
        return -EINVAL;
    }

    g_spi_device = spi_new_device(master, &eeprom_93xx46_info);
    put_device(&master->dev);
    if (!g_spi_device) {
        DEBUG_ERROR("register spi new device failed.\n");
        return -EPERM;
    }

    if (debug) {
        dev_info(&g_spi_device->dev, "register %u bus_num spi 93xx46 eeprom success\n",
            eeprom_93xx46_info.bus_num);
    }

    return 0;
}

static void __exit wb_spi_93xx46_exit(void)
{
    spi_unregister_device(g_spi_device);

    if (debug) {
        dev_info(&g_spi_device->dev, "unregister spi 93xx46 eeprom success\n");
    }

    return;
}

module_init(wb_spi_93xx46_init);
module_exit(wb_spi_93xx46_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("create 93xx46 eeprom device");
MODULE_LICENSE("GPL");
