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
    .config_dev_name = "/dev/cpld0",
    .hw_algo = "eigenvalues",
    .config_mode = 2,       /* Logic dev feed watchdog */
    .priv_func_mode = 3,    /* IO */
    .enable_val = 0xa5,
    .disable_val = 0x0,
    .enable_mask = 0xff,
    .enable_reg = 0x68,
    .timeout_cfg_reg = 0x66,
    .timeleft_cfg_reg = 0x69,
    .hw_margin = 90000,     /* timeout */
    .timer_accuracy_reg_flag = 1,
    .timer_accuracy_reg = 0x65,
    .timer_accuracy_reg_val = 0x80, /* 1s */
    .timer_accuracy = 1000,
    .timer_update_reg_flag = 1,
    .timer_update_reg = 0x67,
    .timer_update_reg_val = 0x01,
    .feed_wdt_type = 0,     /* watchdog device */
    .wdt_config_mode.logic_wdt = {
        .feed_dev_name = "/dev/cpld0",
        .logic_func_mode = 0x03,    /* IO */
        .feed_reg = 0x64,
        .active_val = 0x01
    },
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
