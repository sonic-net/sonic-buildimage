/*
 * transceiver_device_driver.c
 *
 * This module realize /sys/s3ip/transceiver attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
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

static ssize_t wb_set_main_board_debug_sff_attr(unsigned int type, unsigned int sff_index, const char *value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_main_board_debug_sff_attr);

    ret = g_drv->set_main_board_debug_sff_attr(type, sff_index, value);
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
 * wb_get_eth_i2c_bus - Used to get eth i2c bus,
 * filled the value to buf
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_i2c_bus(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_i2c_bus);

    ret = g_drv->get_eth_i2c_bus(eth_index, buf, count);
    return ret;
}

/*
 * wb_get_eth_cage_type - Used to get eth cage_type,
 * filled the value to buf
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_cage_type(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_cage_type);

    ret = g_drv->get_eth_cage_type(eth_index, buf, count);
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
 * wb_get_eth_e2_low_page - Used to get single port e2 low page info
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_e2_low_page(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_e2_low_page);

    ret = g_drv->get_eth_e2_low_page(eth_index, buf, count);
    return ret;
}

/*
 * wb_get_eth_temp - Used to get single port temp
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_temp(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_temp);

    ret = g_drv->get_eth_temp(eth_index, buf, count);
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
 * wb_get_eth_power_group - Used to get single port power group,
 * filled the value to buf
 * @eth_index: start with 1
 * @power_group: Data receiving buffer
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_power_group(unsigned int eth_index, int *power_group)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_power_group);

    ret = g_drv->get_eth_power_group(eth_index, power_group);
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
 * wb_get_eth_attr - Used to get port attr,
 * filled the value to buf
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_attr(unsigned int eth_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_attr);

    ret = g_drv->get_eth_attr(eth_index, type, buf, count);
    return ret;
}

/*
 * wb_set_eth_attr - Used to set port attr,
 * @eth_index: start with 1
 * @status: write status
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_eth_attr(unsigned int eth_index, unsigned int type, int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_eth_attr);

    ret = g_drv->set_eth_attr(eth_index, type, status);
    return ret;
}

/*
 * wb_get_eth_port_led_status - Used to get port led status,
 * filled the value to buf, 0: no interruption, 1: interruption
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_port_led_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_port_led_status);

    ret = g_drv->get_eth_port_led_status(eth_index, buf, count);
    return ret;
}

/*
 * dfd_set_eth_interrupt_status - Used to set port ledstatus,
 * @eth_index: start with 1
 * @data: clear status, 1: clear
 *
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_set_eth_port_led_status(unsigned int eth_index, int data)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_eth_port_led_status);

    ret = g_drv->set_eth_port_led_status(eth_index, data);
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
                   size_t count, int upgrade_flag)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->read_eth_eeprom_data);

    ret = g_drv->read_eth_eeprom_data(eth_index, buf, offset, count, upgrade_flag);
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
                   size_t count, int upgrade_flag)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->write_eth_eeprom_data);

    ret = g_drv->write_eth_eeprom_data(eth_index, buf, offset, count, upgrade_flag);
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

static ssize_t wb_get_sw_init_enable(unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_system_value);

    ret = g_drv->get_system_value(type, buf, count);
    return ret;
}

static ssize_t wb_set_sw_init_enable(unsigned int type,  int value)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_system_value);

    ret = g_drv->set_system_value(type, value);
    return ret;
}

static ssize_t wb_get_sff_power_status_mix_value(char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_sff_power_status_mix_default_value);

    ret = g_drv->get_sff_power_status_mix_default_value();
    if( ret < 0) {
        /* if not set sff power status mix value, default value is 1 */
        ret = 1;
    }

    return ret;
}

static ssize_t wb_get_eth_port_bus_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_port_bus_status);

    ret = g_drv->get_eth_port_bus_status(eth_index, buf, count);
    return ret;
}

/*
 * wb_get_eth_dev_available - Used to get port dev_available status,
 * filled the value to buf, 0: unavailable, 1: available
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_eth_dev_available(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_eth_dev_available);

    ret = g_drv->get_eth_dev_available(eth_index, buf, count);
    return ret;
}

/*
 * wb_set_eth_dev_available - Used to set port dev_available status,
 * @eth_index: start with 1
 * @status: dev_available status, 0: unavailable, 1: available
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_set_eth_dev_available(unsigned int eth_index, int status)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_eth_dev_available);

    ret = g_drv->set_eth_dev_available(eth_index, status);
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
    .get_sff_power_status_mix_value = wb_get_sff_power_status_mix_value,
    .get_transceiver_present_status = wb_get_transceiver_present_status,
    .get_eth_i2c_bus = wb_get_eth_i2c_bus,
    .get_eth_cage_type = wb_get_eth_cage_type,
    .get_eth_power_on_status = wb_get_eth_power_on_status,
    .set_eth_power_on_status = wb_set_eth_power_on_status,
    .get_eth_present_status = wb_get_eth_present_status,
    .get_eth_attr = wb_get_eth_attr,
    .set_eth_attr = wb_set_eth_attr,
    .get_eth_eeprom_size = wb_get_eth_eeprom_size,
    .read_eth_eeprom_data = wb_read_eth_eeprom_data,
    .write_eth_eeprom_data = wb_write_eth_eeprom_data,
    .get_eth_optoe_type = wb_get_eth_optoe_type,
    .set_eth_optoe_type = wb_set_eth_optoe_type,
    .get_eth_power_group = wb_get_eth_power_group,
    .get_eth_e2_low_page = wb_get_eth_e2_low_page,
    .get_eth_temp = wb_get_eth_temp,
    .get_eth_port_led_status = wb_get_eth_port_led_status,
    .set_eth_port_led_status = wb_set_eth_port_led_status,
    .get_sw_init_enable = wb_get_sw_init_enable,
    .set_sw_init_enable = wb_set_sw_init_enable,
    .set_main_board_debug_sff_attr = wb_set_main_board_debug_sff_attr,
    .get_eth_port_bus_status = wb_get_eth_port_bus_status,
    .get_eth_dev_available = wb_get_eth_dev_available,
    .set_eth_dev_available = wb_set_eth_dev_available,
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
