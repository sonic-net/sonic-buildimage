/*
 * An wb_mdio_gpio_device driver for mdio gpio device function
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/platform_data/mdio-gpio.h>
#include <linux/mdio-gpio.h>
#include <linux/gpio.h>
#include <linux/gpio/machine.h>

static int gpio_mdc = 44;
module_param(gpio_mdc, int, S_IRUGO | S_IWUSR);

static int gpio_mdio = 45;
module_param(gpio_mdio, int, S_IRUGO | S_IWUSR);

static char *gpio_chip_name = NULL;
module_param(gpio_chip_name, charp, 0644);
MODULE_PARM_DESC(str_var, "A string variable for GPIO controller");

static int g_wb_mdio_gpio_device_debug = 0;
static int g_wb_mdio_gpio_device_error = 0;

module_param(g_wb_mdio_gpio_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_mdio_gpio_device_error, int, S_IRUGO | S_IWUSR);

#define WB_MIDO_GPIO_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_mdio_gpio_device_debug) { \
        printk(KERN_INFO "[WB_MDIO_GPIO_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_MIDO_GPIO_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_mdio_gpio_device_error) { \
        printk(KERN_ERR "[WB_MDIO_GPIO_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct mdio_gpio_platform_data mdio_gpio_device_data = {
    .phy_mask = 0,
    .phy_ignore_ta_mask = 0,
};

static void wb_mdio_gpio_device_release(struct device *dev)
{
    return;
}

static struct platform_device mdio_gpio_device = {
    .name   = "mdio-gpio",
    .num_resources  = 0,
    .id             = -1,
    .dev    = {
        .platform_data  = &mdio_gpio_device_data,
        .release = wb_mdio_gpio_device_release,
    },
};

static struct gpiod_lookup_table wb_mdio_gpio_table = {
    .dev_id = "mdio-gpio",
    .table = {
        GPIO_LOOKUP_IDX("wb_gpio_d1500", 44, NULL, MDIO_GPIO_MDC,
                GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP_IDX("wb_gpio_d1500", 45, NULL, MDIO_GPIO_MDIO,
                GPIO_ACTIVE_HIGH),
        { },
    },
};

static int __init wb_mdio_gpio_device_init(void)
{
    int err;

    WB_MIDO_GPIO_DEVICE_DEBUG_VERBOSE("wb_mdio_gpio_device_init enter!\n");
    wb_mdio_gpio_table.table[0].chip_hwnum = gpio_mdc;
    wb_mdio_gpio_table.table[1].chip_hwnum = gpio_mdio;

    if (gpio_chip_name) {
        wb_mdio_gpio_table.table[0].key = gpio_chip_name;
        wb_mdio_gpio_table.table[1].key = gpio_chip_name;
    }

    gpiod_add_lookup_table(&wb_mdio_gpio_table);

    err = platform_device_register(&mdio_gpio_device);
    if (err < 0) {
        printk(KERN_ERR "register mdio gpio device fail(%d). \n", err);
        gpiod_remove_lookup_table(&wb_mdio_gpio_table);
        return -1;
    }
    return 0;

}

static void __exit wb_mdio_gpio_device_exit(void)
{
    WB_MIDO_GPIO_DEVICE_DEBUG_VERBOSE("wb_mdio_gpio_device_exit enter!\n");
    platform_device_unregister(&mdio_gpio_device);
    gpiod_remove_lookup_table(&wb_mdio_gpio_table);
}

module_init(wb_mdio_gpio_device_init);
module_exit(wb_mdio_gpio_device_exit);
MODULE_DESCRIPTION("WB MDIO GPIO Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
