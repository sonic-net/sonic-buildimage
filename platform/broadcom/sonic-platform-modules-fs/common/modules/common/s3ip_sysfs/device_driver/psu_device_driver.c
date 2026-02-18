/*
 * psu_device_driver.c
 *
 * This module realize /sys/s3ip/psu attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "psu_sysfs.h"
#include "dfd_sysfs_common.h"

#define PSU_INFO(fmt, args...) LOG_INFO("psu: ", fmt, ##args)
#define PSU_ERR(fmt, args...)  LOG_ERR("psu: ", fmt, ##args)
#define PSU_DBG(fmt, args...)  LOG_DBG("psu: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/********************************************psu**********************************************/
static int wb_get_psu_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_number);

    ret = g_drv->get_psu_number();
    return ret;
}

static int wb_get_psu_temp_number(unsigned int psu_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_temp_number);

    ret = g_drv->get_psu_temp_number(psu_index);
    return ret;
}

/*
 * wb_get_psu_attr - Used to get psu attr,
 * @psu_index: start with 1
 * @type: attr type
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_attr(unsigned int psu_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_attr);

    ret = g_drv->get_psu_attr(psu_index, type, buf, count);
    return ret;
}

/*
 * wb_get_psu_status - Used to get psu status,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_status);

    ret = g_drv->get_psu_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_hw_status - Used to get psu status,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_hw_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_hw_status);

    ret = g_drv->get_psu_hw_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_hw_detail_status - Used to get psu detail status,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_hw_detail_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_hw_detail_status);

    ret = g_drv->get_psu_hw_detail_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_alarm - Used to get psu alarm status,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_alarm(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_alarm);

    ret = g_drv->get_psu_alarm(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_type - Used to get the input type of psu
 * filled the value to buf, input type value define as below:
 * 0: DC
 * 1: AC
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_type(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_type);

    ret = g_drv->get_psu_type(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_sensor_attr - Used to get the input current of psu
 * filled the value to buf, the value is integer with mA
 * @psu_index: start with 1
 * @type: attr type
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_sensor_attr(unsigned int psu_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_sensor_attr);

    ret = g_drv->get_psu_sensor_attr(psu_index, type, buf, count);
    return ret;
}

static ssize_t wb_get_psu_attr_threshold(unsigned int psu_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_attr_threshold);

    ret = g_drv->get_psu_attr_threshold(psu_index, type, buf, count);
    return ret;
}

/*
 * wb_get_psu_present_status - Used to get psu present status
 * filled the value to buf, psu present status define as below:
 * 0: ABSENT
 * 1: PRESENT
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_present_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_present_status);

    ret = g_drv->get_psu_present_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_in_status - Used to get psu input status
 * filled the value to buf, psu input status define as below:
 * 0: NOT OK
 * 1: OK
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_in_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_in_status);

    ret = g_drv->get_psu_in_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_status_pmbus - Used to get psu status from pmbus
 * filled the value to buf, psu output status define as below:
 * 0: NOT OK
 * 1: OK
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_status_pmbus(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_status_pmbus);

    ret = g_drv->get_psu_status_pmbus(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_out_status - Used to get psu output status
 * filled the value to buf, psu output status define as below:
 * 0: NOT OK
 * 1: OK
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_out_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_out_status);

    ret = g_drv->get_psu_out_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_fan_ratio - Used to get the ratio of psu fan
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_fan_ratio(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_fan_ratio);

    ret = g_drv->get_psu_fan_ratio(psu_index, buf, count);
    return ret;
}

/*
 * wb_set_psu_fan_ratio - Used to set the ratio of psu fan
 * @psu_index: start with 1
 * @ratio: from 0 to 100
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_psu_fan_ratio(unsigned int psu_index, int ratio)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_psu_fan_ratio);

    ret = g_drv->set_psu_fan_ratio(psu_index, ratio);
    return ret;
}

/*
 * wb_get_psu_led_status - Used to get psu led status
 * filled the value to buf, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4: blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8: blue light flashing
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_led_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_led_status);

    ret = g_drv->get_psu_led_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_get_psu_temp_attr - Used to get threshold of temperature sensor of psu,
 * filled the value to buf, the value is integer with millidegree Celsius
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_temp_attr(unsigned int psu_index, unsigned int temp_index,
                    unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_temp_attr);

    ret = g_drv->get_psu_temp_attr(psu_index, temp_index, type, buf, count);
    return ret;
}

/*
 * wb_get_psu_eeprom_size - Used to get psu eeprom size
 *
 * This function returns the size of port eeprom,
 * otherwise it returns a negative value on failed.
 */
static int wb_get_psu_eeprom_size(unsigned int psu_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_eeprom_size);

    ret = g_drv->get_psu_eeprom_size(psu_index);
    return ret;
}

/*
 * wb_read_psu_eeprom_data - Used to read psu eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read psu eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_read_psu_eeprom_data(unsigned int psu_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->read_psu_eeprom_data);

    ret = g_drv->read_psu_eeprom_data(psu_index, buf, offset, count);
    return ret;
}

/*
 * wb_get_psu_blackbox_path - Used to get psu blackbox information path,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_blackbox_path(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_blackbox_path);

    ret = g_drv->get_psu_blackbox_path(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_pmbus_info - Used to get psu pmbus information,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_pmbus_info(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_pmbus_info);

    ret = g_drv->get_psu_pmbus_info(psu_index, buf, count);
    return ret;
}

/*
 * wb_clear_psu_blackbox - Used to clear psu blackbox information
 * @psu_index: start with 1
 * @value: 1
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_clear_psu_blackbox(unsigned int psu_index, uint8_t value)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->clear_psu_blackbox);

    ret = g_drv->clear_psu_blackbox(psu_index, value);
    return ret;
}

/*
 * wb_get_psu_support_upgrade - Used to get psu support upgrade,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_support_upgrade(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_support_upgrade);

    ret = g_drv->get_psu_support_upgrade(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_upgrade_active_type - Used to get psu upgrade active type,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_upgrade_active_type(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_upgrade_active_type);

    ret = g_drv->get_psu_upgrade_active_type(psu_index, buf, count);
    return ret;
}

/*
 * wb_set_psu_reset - Used to reset psu
 * @psu_index: start with 1
 * @value: 1
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_psu_reset(unsigned int psu_index, uint8_t value)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_psu_reset);

    ret = g_drv->set_psu_reset(psu_index, value);
    return ret;
}

/****************************************end of psu*******************************************/

static struct s3ip_sysfs_psu_drivers_s drivers = {
    /*
     * set ODM psu drivers to /sys/s3ip/psu,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_psu_number = wb_get_psu_number,
    .get_psu_temp_number = wb_get_psu_temp_number,
    .get_psu_attr = wb_get_psu_attr,
    .get_psu_status = wb_get_psu_status,
    .get_psu_hw_status = wb_get_psu_hw_status,
    .get_psu_hw_detail_status = wb_get_psu_hw_detail_status,
    .get_psu_alarm = wb_get_psu_alarm,
    .get_psu_type = wb_get_psu_type,
    .get_psu_sensor_attr = wb_get_psu_sensor_attr,
    .get_psu_present_status = wb_get_psu_present_status,
    .get_psu_status_pmbus = wb_get_psu_status_pmbus,
    .get_psu_in_status = wb_get_psu_in_status,
    .get_psu_out_status = wb_get_psu_out_status,
    .get_psu_fan_ratio = wb_get_psu_fan_ratio,
    .set_psu_fan_ratio = wb_set_psu_fan_ratio,
    .get_psu_led_status = wb_get_psu_led_status,
    .get_psu_temp_attr = wb_get_psu_temp_attr,
    .get_psu_attr_threshold = wb_get_psu_attr_threshold,
    .get_psu_eeprom_size = wb_get_psu_eeprom_size,
    .read_psu_eeprom_data = wb_read_psu_eeprom_data,
    .get_psu_blackbox_path = wb_get_psu_blackbox_path,
    .get_psu_pmbus_info = wb_get_psu_pmbus_info,
    .clear_psu_blackbox = wb_clear_psu_blackbox,
    .get_psu_support_upgrade = wb_get_psu_support_upgrade,
    .get_psu_upgrade_active_type = wb_get_psu_upgrade_active_type,
    .set_psu_reset = wb_set_psu_reset,
};

static int __init psu_dev_drv_init(void)
{
    int ret;

    PSU_INFO("psu_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_psu_drivers_register(&drivers);
    if (ret < 0) {
        PSU_ERR("psu drivers register err, ret %d.\n", ret);
        return ret;
    }
    PSU_INFO("psu_init success.\n");
    return 0;
}

static void __exit psu_dev_drv_exit(void)
{
    s3ip_sysfs_psu_drivers_unregister();
    PSU_INFO("psu_exit ok.\n");

    return;
}

module_init(psu_dev_drv_init);
module_exit(psu_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("psu device driver");
