/*
 * An transceiver_device_driver driver for sff devcie function
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
#include "transceiver_sysfs.h"
#include "dfd_sysfs_common.h"

#define SFF_INFO(fmt, args...) LOG_INFO("sff: ", fmt, ##args)
#define SFF_ERR(fmt, args...)  LOG_ERR("sff: ", fmt, ##args)
#define SFF_DBG(fmt, args...)  LOG_DBG("sff: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/****************************************transceiver******************************************/
static int wb_get_eth_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_number);

    ret = g_drv->get_eth_number();
    return ret;
}

/*
 * wb_get_transceiver_power_on_status - Used to get the whole machine port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_transceiver_power_on_status(char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_transceiver_power_on_status);

    ret = g_drv->get_transceiver_power_on_status(buf, count);
    return ret;
}

/*
 * wb_set_transceiver_power_on_status - Used to set the whole machine port power on status,
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_transceiver_power_on_status(int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_transceiver_power_on_status);

    ret = g_drv->set_transceiver_power_on_status(status);
    return ret;
}

/*
 * wb_get_transceiver_present_status - Used to get the whole machine port present status,
 * filled the value to buf, 0: absent, 1: present
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_transceiver_present_status(char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_transceiver_present_status);

    ret = g_drv->get_transceiver_present_status(buf, count);
    return ret;
}

/*
 * wb_get_eth_power_on_status - Used to get single port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_power_on_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_power_on_status);

    ret = g_drv->get_eth_power_on_status(eth_index, buf, count);
    return ret;
}

/*
 * wb_set_eth_power_on_status - Used to set single port power on status,
 * @eth_index: start with 1
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_eth_power_on_status(unsigned int eth_index, int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_eth_power_on_status);

    ret = g_drv->set_eth_power_on_status(eth_index, status);
    return ret;
}

/*
 * wb_get_eth_tx_fault_status - Used to get port tx_fault status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_tx_fault_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_tx_fault_status);

    ret = g_drv->get_eth_tx_fault_status(eth_index, buf, count);
    return ret;
}

/*
 * wb_get_eth_tx_disable_status - Used to get port tx_disable status,
 * filled the value to buf, 0: tx_enable, 1: tx_disable
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_tx_disable_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_tx_disable_status);

    ret = g_drv->get_eth_tx_disable_status(eth_index, buf, count);
    return ret;
}

/*
 * wb_set_eth_tx_disable_status - Used to set port tx_disable status,
 * @eth_index: start with 1
 * @status: tx_disable status, 0: tx_enable, 1: tx_disable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_eth_tx_disable_status(unsigned int eth_index, int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_eth_tx_disable_status);

    ret = g_drv->set_eth_tx_disable_status(eth_index, status);
    return ret;
}

/*
 * wb_get_eth_present_status - Used to get port present status,
 * filled the value to buf, 1: present, 0: absent
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_present_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_present_status);

    ret = g_drv->get_eth_present_status(eth_index, buf, count);
    return ret;
}

/*
 * wb_get_eth_rx_los_status - Used to get port rx_los status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_rx_los_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_rx_los_status);

    ret = g_drv->get_eth_rx_los_status(eth_index, buf, count);
    return ret;
}

/*
 * wb_get_eth_reset_status - Used to get port reset status,
 * filled the value to buf, 0: unreset, 1: reset
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_reset_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_reset_status);

    ret = g_drv->get_eth_reset_status(eth_index, buf, count);
    return ret;
}

/*
 * wb_set_eth_reset_status - Used to set port reset status,
 * @eth_index: start with 1
 * @status: reset status, 0: unreset, 1: reset
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_eth_reset_status(unsigned int eth_index, int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_eth_reset_status);

    ret = g_drv->set_eth_reset_status(eth_index, status);
    return ret;
}

/*
 * wb_get_eth_low_power_mode_status - Used to get port low power mode status,
 * filled the value to buf, 0: high power mode, 1: low power mode
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_low_power_mode_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_low_power_mode_status);

    ret = g_drv->get_eth_low_power_mode_status(eth_index, buf, count);
    return ret;
}

/*
 * wb_get_eth_interrupt_status - Used to get port interruption status,
 * filled the value to buf, 0: no interruption, 1: interruption
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_interrupt_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_interrupt_status);

    ret = g_drv->get_eth_interrupt_status(eth_index, buf, count);
    return ret;
}

/*
 * wb_get_eth_eeprom_size - Used to get port eeprom size
 *
 * This function returns the size of port eeprom,
 * otherwise it returns a negative value on failed.
 */
static int wb_get_eth_eeprom_size(unsigned int eth_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_eeprom_size);

    ret = g_drv->get_eth_eeprom_size(eth_index);
    return ret;
}

/*
 * wb_read_eth_eeprom_data - Used to read port eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read port eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_read_eth_eeprom_data(unsigned int eth_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->read_eth_eeprom_data);

    ret = g_drv->read_eth_eeprom_data(eth_index, buf, offset, count);
    return ret;
}

/*
 * wb_write_eth_eeprom_data - Used to write port eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write port eeprom data
 * @count: length of buf
 *
 * This function returns the written length of port eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_write_eth_eeprom_data(unsigned int eth_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->write_eth_eeprom_data);

    ret = g_drv->write_eth_eeprom_data(eth_index, buf, offset, count);
    return ret;
}

static ssize_t wb_get_eth_optoe_type(unsigned int sff_index, int *optoe_type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_optoe_type);

    ret = g_drv->get_eth_optoe_type(sff_index, optoe_type, buf, count);
    return ret;
}

static ssize_t wb_set_eth_optoe_type(unsigned int sff_index, int optoe_type)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_eth_optoe_type);

    ret = g_drv->set_eth_optoe_type(sff_index, optoe_type);
    return ret;
}

/************************************end of transceiver***************************************/

static struct s3ip_sysfs_transceiver_drivers_s drivers = {
    /*
     * set ODM transceiver drivers to /sys/s3ip/transceiver,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_eth_number = wb_get_eth_number,
    .get_transceiver_power_on_status = wb_get_transceiver_power_on_status,
    .set_transceiver_power_on_status = wb_set_transceiver_power_on_status,
    .get_transceiver_present_status = wb_get_transceiver_present_status,
    .get_eth_power_on_status = wb_get_eth_power_on_status,
    .set_eth_power_on_status = wb_set_eth_power_on_status,
    .get_eth_tx_fault_status = wb_get_eth_tx_fault_status,
    .get_eth_tx_disable_status = wb_get_eth_tx_disable_status,
    .set_eth_tx_disable_status = wb_set_eth_tx_disable_status,
    .get_eth_present_status = wb_get_eth_present_status,
    .get_eth_rx_los_status = wb_get_eth_rx_los_status,
    .get_eth_reset_status = wb_get_eth_reset_status,
    .set_eth_reset_status = wb_set_eth_reset_status,
    .get_eth_low_power_mode_status = wb_get_eth_low_power_mode_status,
    .get_eth_interrupt_status = wb_get_eth_interrupt_status,
    .get_eth_eeprom_size = wb_get_eth_eeprom_size,
    .read_eth_eeprom_data = wb_read_eth_eeprom_data,
    .write_eth_eeprom_data = wb_write_eth_eeprom_data,
    .get_eth_optoe_type = wb_get_eth_optoe_type,
    .set_eth_optoe_type = wb_set_eth_optoe_type,
};

static int __init sff_dev_drv_init(void)
{
    int ret;

    SFF_INFO("sff_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_sff_drivers_register(&drivers);
    if (ret < 0) {
        SFF_ERR("transceiver drivers register err, ret %d.\n", ret);
        return ret;
    }
    SFF_INFO("sff_init success.\n");
    return 0;
}

static void __exit sff_dev_drv_exit(void)
{
    s3ip_sysfs_sff_drivers_unregister();
    SFF_INFO("sff_exit success.\n");
    return;
}

module_init(sff_dev_drv_init);
module_exit(sff_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("transceiver device driver");
