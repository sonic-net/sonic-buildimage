/*
 * An watchdog_device_driver driver for watchdog devcie function
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

#include <linux/slab.h>

#include "device_driver_common.h"
#include "watchdog_sysfs.h"
#include "dfd_sysfs_common.h"

#define WDT_INFO(fmt, args...) LOG_INFO("watchdog: ", fmt, ##args)
#define WDT_ERR(fmt, args...)  LOG_ERR("watchdog: ", fmt, ##args)
#define WDT_DBG(fmt, args...)  LOG_DBG("watchdog: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/****************************************watchdog*********************************************/
/*
 * wb_get_watchdog_identify - Used to get watchdog identify, such as iTCO_wdt
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_watchdog_identify(char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_watchdog_identify);

    ret = g_drv->get_watchdog_identify(buf, count);
    return ret;
}

/*
 * wb_get_watchdog_timeleft - Used to get watchdog timeleft,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_watchdog_timeleft(char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_watchdog_timeleft);

    ret = g_drv->get_watchdog_timeleft(buf, count);
    return ret;
}

/*
 * wb_get_watchdog_timeout - Used to get watchdog timeout,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_watchdog_timeout(char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_watchdog_timeout);

    ret = g_drv->get_watchdog_timeout(buf, count);
    return ret;
}

/*
 * wb_set_watchdog_timeout - Used to set watchdog timeout,
 * @value: timeout value
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_watchdog_timeout(int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_watchdog_timeout);

    ret = g_drv->set_watchdog_timeout(value);
    return ret;
}

/*
 * wb_get_watchdog_enable_status - Used to get watchdog enable status,
 * filled the value to buf, 0: disable, 1: enable
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_watchdog_enable_status(char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_watchdog_enable_status);

    ret = g_drv->get_watchdog_enable_status(buf, count);
    return ret;
}

/*
 * wb_set_watchdog_enable_status - Used to set watchdog enable status,
 * @value: enable status value, 0: disable, 1: enable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_watchdog_enable_status(int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_watchdog_enable_status);

    ret = g_drv->set_watchdog_enable_status(value);
    return ret;
}

/*
 * wb_set_watchdog_reset - Used to feed watchdog,
 * @value: any value to feed watchdog
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_watchdog_reset(int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_watchdog_reset);

    ret = g_drv->set_watchdog_reset(value);
    return ret;
}

/*************************************end of watchdog*****************************************/

static struct s3ip_sysfs_watchdog_drivers_s drivers = {
    /*
     * set ODM watchdog sensor drivers to /sys/s3ip/watchdog,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_watchdog_identify = wb_get_watchdog_identify,
    .get_watchdog_timeleft = wb_get_watchdog_timeleft,
    .get_watchdog_timeout = wb_get_watchdog_timeout,
    .set_watchdog_timeout = wb_set_watchdog_timeout,
    .get_watchdog_enable_status = wb_get_watchdog_enable_status,
    .set_watchdog_enable_status = wb_set_watchdog_enable_status,
    .set_watchdog_reset = wb_set_watchdog_reset,
};

static int __init watchdog_dev_drv_init(void)
{
    int ret;

    WDT_INFO("watchdog_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_watchdog_drivers_register(&drivers);
    if (ret < 0) {
        WDT_ERR("watchdog drivers register err, ret %d.\n", ret);
        return ret;
    }
    WDT_INFO("watchdog create success.\n");
    return 0;
}

static void __exit watchdog_dev_drv_exit(void)
{
    s3ip_sysfs_watchdog_drivers_unregister();
    WDT_INFO("watchdog_exit success.\n");
    return;
}

module_init(watchdog_dev_drv_init);
module_exit(watchdog_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("watchdog device driver");
