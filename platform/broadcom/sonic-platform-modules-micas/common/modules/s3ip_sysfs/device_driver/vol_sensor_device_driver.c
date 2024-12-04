/*
 * An vol_sensor_device_driver driver for voltage devcie function
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
#include "vol_sensor_sysfs.h"
#include "dfd_sysfs_common.h"

#define VOL_SENSOR_INFO(fmt, args...) LOG_INFO("vol_sensor: ", fmt, ##args)
#define VOL_SENSOR_ERR(fmt, args...)  LOG_ERR("vol_sensor: ", fmt, ##args)
#define VOL_SENSOR_DBG(fmt, args...)  LOG_DBG("vol_sensor: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/*************************************main board voltage***************************************/
static int wb_get_main_board_vol_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_vol_number);

    ret = g_drv->get_main_board_vol_number();
    return ret;
}

/*
 * wb_get_main_board_vol_alias - Used to identify the location of the voltage sensor,
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_vol_alias(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_vol_alias);

    ret = g_drv->get_main_board_vol_alias(vol_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_vol_type - Used to get the model of voltage sensor,
 * such as udc90160, tps53622 and so on
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_vol_type(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_vol_type);

    ret = g_drv->get_main_board_vol_type(vol_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_vol_max - Used to get the maximum threshold of voltage sensor
 * filled the value to buf, the value is integer with mV
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_vol_max(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_vol_max);

    ret = g_drv->get_main_board_vol_max(vol_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_vol_min - Used to get the minimum threshold of voltage sensor
 * filled the value to buf, the value is integer with mV
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_vol_min(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_vol_min);

    ret = g_drv->get_main_board_vol_min(vol_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_vol_range - Used to get the output error value of voltage sensor
 * filled the value to buf
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_vol_range(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_vol_range);

    ret = g_drv->get_main_board_vol_range(vol_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_vol_nominal_value - Used to get the nominal value of voltage sensor
 * filled the value to buf
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_vol_nominal_value(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_vol_nominal_value);

    ret = g_drv->get_main_board_vol_nominal_value(vol_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_vol_value - Used to get the input value of voltage sensor
 * filled the value to buf, the value is integer with mV
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_vol_value(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_vol_value);

    ret = g_drv->get_main_board_vol_value(vol_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_vol_monitor_flag - Used to get the monitor flag of voltage sensor
 * filled the value to buf, the value is integer with mV
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_vol_monitor_flag(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_vol_monitor_flag);

    ret = g_drv->get_main_board_vol_monitor_flag(vol_index, buf, count);
    return ret;
}
/*********************************end of main board voltage************************************/

static struct s3ip_sysfs_vol_sensor_drivers_s drivers = {
    /*
     * set ODM voltage sensor drivers to /sys/s3ip/vol_sensor,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_vol_number = wb_get_main_board_vol_number,
    .get_main_board_vol_alias = wb_get_main_board_vol_alias,
    .get_main_board_vol_type = wb_get_main_board_vol_type,
    .get_main_board_vol_max = wb_get_main_board_vol_max,
    .get_main_board_vol_min = wb_get_main_board_vol_min,
    .get_main_board_vol_range = wb_get_main_board_vol_range,
    .get_main_board_vol_nominal_value = wb_get_main_board_vol_nominal_value,
    .get_main_board_vol_value = wb_get_main_board_vol_value,
    .get_main_board_vol_monitor_flag = wb_get_main_board_vol_monitor_flag,
};

static int __init vol_sensor_dev_drv_init(void)
{
    int ret;

    VOL_SENSOR_INFO("vol_sensor_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_vol_sensor_drivers_register(&drivers);
    if (ret < 0) {
        VOL_SENSOR_ERR("vol sensor drivers register err, ret %d.\n", ret);
        return ret;
    }
    VOL_SENSOR_INFO("vol_sensor_init success.\n");
    return 0;
}

static void __exit vol_sensor_dev_drv_exit(void)
{
    s3ip_sysfs_vol_sensor_drivers_unregister();
    VOL_SENSOR_INFO("vol_sensor_exit success.\n");
    return;
}

module_init(vol_sensor_dev_drv_init);
module_exit(vol_sensor_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("voltage sensors device driver");
