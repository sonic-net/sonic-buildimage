/*
 * An cpld_device_driver driver for cpld devcie function
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
#include "cpld_sysfs.h"
#include "dfd_sysfs_common.h"

#define CPLD_INFO(fmt, args...) LOG_INFO("cpld: ", fmt, ##args)
#define CPLD_ERR(fmt, args...)  LOG_ERR("cpld: ", fmt, ##args)
#define CPLD_DBG(fmt, args...)  LOG_DBG("cpld: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/******************************************CPLD***********************************************/
static int wb_get_main_board_cpld_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_cpld_number);

    ret = g_drv->get_main_board_cpld_number();
    return ret;
}

/*
 * wb_get_main_board_cpld_alias - Used to identify the location of cpld,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_cpld_alias(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_cpld_alias);

    ret = g_drv->get_main_board_cpld_alias(cpld_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_cpld_type - Used to get cpld model name
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_cpld_type(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_cpld_type);

    ret = g_drv->get_main_board_cpld_type(cpld_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_cpld_firmware_version - Used to get cpld firmware version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_cpld_firmware_version(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_cpld_firmware_version);

    ret = g_drv->get_main_board_cpld_firmware_version(cpld_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_cpld_board_version - Used to get cpld board version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_cpld_board_version(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_cpld_board_version);

    ret = g_drv->get_main_board_cpld_board_version(cpld_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_cpld_test_reg - Used to test cpld register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_cpld_test_reg(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_cpld_test_reg);

    ret = g_drv->get_main_board_cpld_test_reg(cpld_index, buf, count);
    return ret;
}

/*
 * wb_set_main_board_cpld_test_reg - Used to test cpld register write
 * @cpld_index: start with 1
 * @value: value write to cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_main_board_cpld_test_reg(unsigned int cpld_index, unsigned int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_main_board_cpld_test_reg);

    ret = g_drv->set_main_board_cpld_test_reg(cpld_index, value);
    return ret;
}
/***************************************end of CPLD*******************************************/

static struct s3ip_sysfs_cpld_drivers_s drivers = {
    /*
     * set ODM CPLD drivers to /sys/s3ip/cpld,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_cpld_number = wb_get_main_board_cpld_number,
    .get_main_board_cpld_alias = wb_get_main_board_cpld_alias,
    .get_main_board_cpld_type = wb_get_main_board_cpld_type,
    .get_main_board_cpld_firmware_version = wb_get_main_board_cpld_firmware_version,
    .get_main_board_cpld_board_version = wb_get_main_board_cpld_board_version,
    .get_main_board_cpld_test_reg = wb_get_main_board_cpld_test_reg,
    .set_main_board_cpld_test_reg = wb_set_main_board_cpld_test_reg,
};

static int __init cpld_device_driver_init(void)
{
    int ret;

    CPLD_INFO("cpld_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_cpld_drivers_register(&drivers);
    if (ret < 0) {
        CPLD_ERR("cpld drivers register err, ret %d.\n", ret);
        return ret;
    }

    CPLD_INFO("cpld_init success.\n");
    return 0;
}

static void __exit cpld_device_driver_exit(void)
{
    s3ip_sysfs_cpld_drivers_unregister();
    CPLD_INFO("cpld_exit success.\n");
    return;
}

module_init(cpld_device_driver_init);
module_exit(cpld_device_driver_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("cpld device driver");
