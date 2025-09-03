/*
 * An eeprom_device_driver driver for eeprom devcie function
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
#include "eeprom_sysfs.h"
#include "dfd_sysfs_common.h"

#define EEPROM_INFO(fmt, args...)  LOG_INFO("eeprom: ", fmt, ##args)
#define EEPROM_ERR(fmt, args...)   LOG_ERR("eeprom: ", fmt, ##args)
#define EEPROM_DBG(fmt, args...)   LOG_DBG("eeprom: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/*****************************************eeprom*******************************************/

/*
 * wb_get_eeprom_number - Used to get eeprom number
 *
 * This function returns the size of eeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int wb_get_eeprom_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_eeprom_number);

    ret = g_drv->get_eeprom_number();
    return ret;
}

/*
 * wb_get_eeprom_size - Used to get eeprom size
 *
 * This function returns the size of eeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int wb_get_eeprom_size(unsigned int e2_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_eeprom_size);

    ret = g_drv->get_eeprom_size(e2_index);
    return ret;
}

/*
 * wb_get_eeprom_alias - Used to get eeprom alias
 *
 * This function returns the size of eeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eeprom_alias(unsigned int e2_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eeprom_alias);

    ret = g_drv->get_eeprom_alias(e2_index, buf, count);
    return ret;
}

/*
 * wb_get_eeprom_tag - Used to get eeprom tag
 *
 * This function returns the size of eeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eeprom_tag(unsigned int e2_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eeprom_tag);

    ret = g_drv->get_eeprom_tag(e2_index, buf, count);
    return ret;
}

/*
 * wb_get_eeprom_type - Used to get eeprom type
 *
 * This function returns the size of eeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eeprom_type(unsigned int e2_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eeprom_type);

    ret = g_drv->get_eeprom_type(e2_index, buf, count);
    return ret;
}


/*
 * wb_read_eeprom_data - Used to read eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_read_eeprom_data(unsigned int e2_index, char *buf, loff_t offset, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->read_eeprom_data);

    ret = g_drv->read_eeprom_data(e2_index, buf, offset, count);
    return ret;
}

/*
 * wb_write_eeprom_data - Used to write eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write eeprom data
 * @count: length of buf
 *
 * This function returns the written length of eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_write_eeprom_data(unsigned int e2_index, char *buf, loff_t offset, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->write_eeprom_data);

    ret = g_drv->write_eeprom_data(e2_index, buf, offset, count);
    return ret;
}
/*************************************end of eeprom****************************************/

static struct s3ip_sysfs_eeprom_drivers_s drivers = {
    /*
     * set ODM eeprom drivers to /sys/s3ip/eeprom,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_eeprom_number = wb_get_eeprom_number,
    .get_eeprom_alias = wb_get_eeprom_alias,
    .get_eeprom_tag = wb_get_eeprom_tag,
    .get_eeprom_type = wb_get_eeprom_type,
    .get_eeprom_size = wb_get_eeprom_size,
    .read_eeprom_data = wb_read_eeprom_data,
    .write_eeprom_data = wb_write_eeprom_data,
};

static int __init eeprom_dev_drv_init(void)
{
    int ret;

    EEPROM_INFO("eeprom_dev_drv_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_eeprom_drivers_register(&drivers);
    if (ret < 0) {
        EEPROM_ERR("eeprom drivers register err, ret %d.\n", ret);
        return ret;
    }
    EEPROM_INFO("eeprom_dev_drv_init success.\n");
    return 0;
}

static void __exit eeprom_dev_drv_exit(void)
{
    s3ip_sysfs_eeprom_drivers_unregister();
    EEPROM_INFO("eeprom_exit success.\n");
    return;
}

module_init(eeprom_dev_drv_init);
module_exit(eeprom_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("eeprom device driver");
