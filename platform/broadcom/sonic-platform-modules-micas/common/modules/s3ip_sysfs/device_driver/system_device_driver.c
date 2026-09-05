/*
 * An system_device_driver driver for system devcie function
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
#include "system_sysfs.h"
#include "dfd_sysfs_common.h"

#define TEMP_SENSOR_INFO(fmt, args...) LOG_INFO("system: ", fmt, ##args)
#define TEMP_SENSOR_ERR(fmt, args...)  LOG_ERR("system: ", fmt, ##args)
#define TEMP_SENSOR_DBG(fmt, args...)  LOG_DBG("system: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

static ssize_t wb_set_system_value(unsigned int type,  int value)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_system_value);

    ret = g_drv->set_system_value(type, value);
    return ret;
}

static ssize_t wb_get_system_value(unsigned int type, int *value, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_system_value);

    ret = g_drv->get_system_value(type, value, buf, count);
    return ret;
}

static ssize_t wb_get_system_port_power_status(unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_system_port_power_status);

    ret = g_drv->get_system_port_power_status(type, buf, count);
    return ret;
}
/***********************************end of main board temp*************************************/

static struct s3ip_sysfs_system_drivers_s drivers = {
    /*
     * set ODM system drivers to /sys/s3ip/system,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_system_value = wb_get_system_value,
    .set_system_value = wb_set_system_value,
    .get_system_port_power_status = wb_get_system_port_power_status,
};

static int __init system_dev_drv_init(void)
{
    int ret;

    TEMP_SENSOR_INFO("system_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_system_drivers_register(&drivers);
    if (ret < 0) {
        TEMP_SENSOR_ERR("temp sensor drivers register err, ret %d.\n", ret);
        return ret;
    }
    TEMP_SENSOR_INFO("system_init success.\n");
    return 0;
}

static void __exit system_dev_drv_exit(void)
{
    s3ip_sysfs_system_drivers_unregister();
    TEMP_SENSOR_INFO("system_exit success.\n");
    return;
}

module_init(system_dev_drv_init);
module_exit(system_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("system device driver");
