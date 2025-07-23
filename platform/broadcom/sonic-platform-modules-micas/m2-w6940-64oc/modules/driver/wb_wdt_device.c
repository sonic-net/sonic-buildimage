/*
 * An wb_wdt_device driver for watchdog device function
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

#include <wb_wdt.h>

static int g_wb_wdt_device_debug = 0;
static int g_wb_wdt_device_error = 0;

module_param(g_wb_wdt_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_wdt_device_error, int, S_IRUGO | S_IWUSR);

#define WB_WDT_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_wdt_device_debug) { \
        printk(KERN_INFO "[WB_WDT_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_WDT_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_wdt_device_error) { \
        printk(KERN_ERR "[WB_WDT_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static wb_wdt_device_t wb_wdt_device_data_0 = {
    .feed_wdt_type = 1,
    .hw_margin = 408000,
    .feed_time = 9000,
    .config_dev_name = "/dev/cpld1",
    .config_mode = 1,
    .priv_func_mode = 3,
    .enable_reg = 0xb0,
    .enable_val = 0x1,
    .disable_val = 0x0,
    .enable_mask = 0x1,
    .timeout_cfg_reg = 0xb1,
    .timeleft_cfg_reg = 0xb2,
    .hw_algo = "toggle",
    .wdt_config_mode.gpio_wdt = {
        .gpio = 346,
        .flags = 1
    },
    .timer_accuracy = 1600,          /* 1.6s */
    .sysfs_index = SYSFS_NO_CFG,
};

/* sys led */
static wb_wdt_device_t wb_wdt_device_data_1 = {
    .feed_wdt_type = 2,
    .hw_margin = 180000,
    .feed_time = 30000,
    .config_dev_name = "/dev/cpld1",
    .config_mode = 2,
    .priv_func_mode = 3,
    .enable_reg = 0xba,
    .enable_val = 0x1,
    .disable_val = 0x0,
    .enable_mask = 0x1,
    .timeout_cfg_reg = 0xbc,
    .timeleft_cfg_reg = 0xbd,
    .hw_algo = "toggle",
    .wdt_config_mode.logic_wdt = {
        .feed_dev_name = "/dev/cpld1",
        .feed_reg = 0xbb,
        .active_val = 0x01,
        .logic_func_mode = 4,
    },
    .timer_accuracy = 6000,          /* 6s */
    .sysfs_index = SYSFS_NO_CFG,
};

static void wb_wdt_device_release(struct device *dev)
{
    return;
}

static struct platform_device wb_wdt_device[] = {
    {
        .name   = "wb_wdt",
        .id = 0,
        .dev    = {
            .platform_data  = &wb_wdt_device_data_0,
            .release = wb_wdt_device_release,
        },
    },
    {
        .name   = "wb_wdt",
        .id = 1,
        .dev    = {
            .platform_data  = &wb_wdt_device_data_1,
            .release = wb_wdt_device_release,
        },
    },
};

static int __init wb_wdt_device_init(void)
{
    int i;
    int ret = 0;
    wb_wdt_device_t *wb_wdt_device_data;

    WB_WDT_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(wb_wdt_device); i++) {
        wb_wdt_device_data = wb_wdt_device[i].dev.platform_data;
        ret = platform_device_register(&wb_wdt_device[i]);
        if (ret < 0) {
            wb_wdt_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "rg-wdt.%d register failed!\n", i + 1);
        } else {
            wb_wdt_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_wdt_device_exit(void)
{
    int i;
    wb_wdt_device_t *wb_wdt_device_data;

    WB_WDT_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(wb_wdt_device) - 1; i >= 0; i--) {
        wb_wdt_device_data = wb_wdt_device[i].dev.platform_data;
        if (wb_wdt_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&wb_wdt_device[i]);
        }
    }
}

module_init(wb_wdt_device_init);
module_exit(wb_wdt_device_exit);
MODULE_DESCRIPTION("WB WDT Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
