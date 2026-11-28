/*
 * fan_device_driver.c
 *
 * This module realize /sys/s3ip/fan attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "fan_sysfs.h"
#include "dfd_sysfs_common.h"

#define FAN_INFO(fmt, args...) LOG_INFO("fan: ", fmt, ##args)
#define FAN_ERR(fmt, args...)  LOG_ERR("fan: ", fmt, ##args)
#define FAN_DBG(fmt, args...)  LOG_DBG("fan: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/********************************************fan**********************************************/
static ssize_t wb_set_fan_dev_debug_fan_attr(unsigned int type, unsigned int fan_index, const char *value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_fan_dev_debug_fan_attr);

    ret = g_drv->set_fan_dev_debug_fan_attr(type, fan_index, value);
    return ret;
}

static int wb_get_fan_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_number);

    ret = g_drv->get_fan_number();
    return ret;
}

static int wb_get_fan_motor_number(unsigned int fan_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_motor_number);

    ret = g_drv->get_fan_motor_number(fan_index);
    return ret;
}

/*
 * wb_get_fan_model_name - Used to get fan model name,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_model_name(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_model_name);

    ret = g_drv->get_fan_model_name(fan_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_vendor - Used to get vendor,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_vendor(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_vendor);

    ret = g_drv->get_fan_vendor(fan_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_serial_number - Used to get fan serial number,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_serial_number(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_serial_number);

    ret = g_drv->get_fan_serial_number(fan_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_part_number - Used to get fan part number,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_part_number(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_part_number);

    ret = g_drv->get_fan_part_number(fan_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_hardware_version - Used to get fan hardware version,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_hardware_version(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_hardware_version);

    ret = g_drv->get_fan_hardware_version(fan_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_status - Used to get fan status,
 * filled the value to buf, fan status define as below:
 * 0: ABSENT
 * 1: OK
 * 2: NOT OK
 *
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_status(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_status);

    ret = g_drv->get_fan_status(fan_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_present - Used to get fan present status,
 * filled the value to buf, fan status define as below:
 * 0: ABSENT
 * 1: PRESENT
 *
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_present(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_present);

    ret = g_drv->get_fan_present(fan_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_led_status - Used to get fan led status
 * filled the value to buf, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4:blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8:blue light flashing
 *
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_led_status(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_led_status);

    ret = g_drv->get_fan_led_status(fan_index, buf, count);
    return ret;
}

/*
 * wb_set_fan_led_status - Used to set fan led status
 * @fan_index: start with 1
 * @status: led status, led status value define as below:
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
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_fan_led_status(unsigned int fan_index, int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_fan_led_status);

    ret = g_drv->set_fan_led_status(fan_index, status);
    return ret;
}

/*
 * wb_get_fan_direction - Used to get fan air flow direction,
 * filled the value to buf, air flow direction define as below:
 * 0: F2B
 * 1: B2F
 *
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_direction(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_direction);

    ret = g_drv->get_fan_direction(fan_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_motor_status - Used to get fan motor status
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_motor_status(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_motor_status);

    ret = g_drv->get_fan_motor_status(fan_index, motor_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_motor_speed - Used to get fan motor speed
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_motor_speed(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_motor_speed);

    ret = g_drv->get_fan_motor_speed(fan_index, motor_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_motor_speed_tolerance - Used to get fan motor speed tolerance
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_motor_speed_tolerance(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_motor_speed_tolerance);

    ret = g_drv->get_fan_motor_speed_tolerance(fan_index, motor_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_motor_speed_target - Used to get fan motor speed target
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_motor_speed_target(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_motor_speed_target);

    ret = g_drv->get_fan_motor_speed_target(fan_index, motor_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_motor_speed_max - Used to get the maximum threshold of fan motor
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_motor_speed_max(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_motor_speed_max);

    ret = g_drv->get_fan_motor_speed_max(fan_index, motor_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_motor_speed_min - Used to get the minimum threshold of fan motor
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_motor_speed_min(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_motor_speed_min);

    ret = g_drv->get_fan_motor_speed_min(fan_index, motor_index, buf, count);
    return ret;
}

static ssize_t wb_get_fan_motor_speed_threshold_cnt(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_motor_speed_threshold_cnt);

    ret = g_drv->get_fan_motor_speed_threshold_cnt(fan_index, motor_index, buf, count);
    return ret;
}

/*
 * wb_get_fan_ratio - Used to get the ratio of fan
 * filled the value to buf
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_ratio(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_ratio);

    ret = g_drv->get_fan_ratio(fan_index, buf, count);
    return ret;
}

/*
 * wb_set_fan_ratio - Used to set the ratio of fan
 * @fan_index: start with 1
 * @ratio: motor speed ratio, from 0 to 100
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_fan_ratio(unsigned int fan_index, int ratio)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_fan_ratio);

    ret = g_drv->set_fan_ratio(fan_index, ratio);
    return ret;
}

/*
 * wb_get_main_board_fan_attr - Used to get the attr of fan,
 * @fan_index: start with 1
 * @type: get type
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fan_attr(unsigned int fan_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_attr);

    ret = g_drv->get_fan_attr(fan_index, type, buf, count);
    return ret;
}

/*
 * wb_set_main_board_fan_test_reg - Used to test fan register write
 * @fan_index: start with 1
 * @value: value write to fan
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_fan_attr(unsigned int fan_index, unsigned int type, unsigned int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_fan_attr);

    ret = g_drv->set_fan_attr(fan_index, type, value);
    return ret;
}

/*
 * wb_get_fan_eeprom_size - Used to get eeprom size
 *
 * This function returns the size of eeprom,
 * otherwise it returns a negative value on failed.
 */
static int wb_get_fan_eeprom_size(unsigned int fan_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_eeprom_size);

    ret = g_drv->get_fan_eeprom_size(fan_index);
    return ret;
}

/*
 * wb_read_fan_eeprom_data - Used to read eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_read_fan_eeprom_data(unsigned int fan_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->read_fan_eeprom_data);

    ret = g_drv->read_fan_eeprom_data(fan_index, buf, offset, count);
    return ret;
}

/*
 * wb_write_fan_eeprom_data - Used to write eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write eeprom data
 * @count: length of buf
 *
 * This function returns the written length of eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_write_fan_eeprom_data(unsigned int fan_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->write_fan_eeprom_data);

    ret = g_drv->write_fan_eeprom_data(fan_index, buf, offset, count);
    return ret;
}

/*
 * wb_get_fanctrl_duration_max - Used to get the maximum fanctrl_duration
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_fanctrl_duration_max(char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_fanctrl_duration_max);

    ret = g_drv->get_fanctrl_duration_max(buf, count);
    return ret;
}

/****************************************end of fan*******************************************/

static struct s3ip_sysfs_fan_drivers_s drivers = {
    /*
     * set ODM fan drivers to /sys/s3ip/fan,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_fan_number = wb_get_fan_number,
    .get_fan_motor_number = wb_get_fan_motor_number,
    .get_fan_model_name = wb_get_fan_model_name,
    .get_fan_vendor = wb_get_fan_vendor,
    .get_fan_serial_number = wb_get_fan_serial_number,
    .get_fan_part_number = wb_get_fan_part_number,
    .get_fan_hardware_version = wb_get_fan_hardware_version,
    .get_fan_status = wb_get_fan_status,
    .get_fan_present = wb_get_fan_present,
    .get_fan_led_status = wb_get_fan_led_status,
    .set_fan_led_status = wb_set_fan_led_status,
    .get_fan_direction = wb_get_fan_direction,
    .get_fan_motor_status = wb_get_fan_motor_status,
    .get_fan_motor_speed = wb_get_fan_motor_speed,
    .get_fan_motor_speed_tolerance = wb_get_fan_motor_speed_tolerance,
    .get_fan_motor_speed_target = wb_get_fan_motor_speed_target,
    .get_fan_motor_speed_max = wb_get_fan_motor_speed_max,
    .get_fan_motor_speed_min = wb_get_fan_motor_speed_min,
    .get_fan_motor_speed_threshold_cnt = wb_get_fan_motor_speed_threshold_cnt,
    .get_fan_ratio = wb_get_fan_ratio,
    .set_fan_ratio = wb_set_fan_ratio,
    .get_fan_attr = wb_get_fan_attr,
    .set_fan_attr = wb_set_fan_attr,
    .get_fan_eeprom_size = wb_get_fan_eeprom_size,
    .read_fan_eeprom_data = wb_read_fan_eeprom_data,
    .write_fan_eeprom_data = wb_write_fan_eeprom_data,
    .set_fan_dev_debug_fan_attr = wb_set_fan_dev_debug_fan_attr,
    .get_fanctrl_duration_max = wb_get_fanctrl_duration_max,
};

static int __init fan_dev_drv_init(void)
{
    int ret;

    FAN_INFO("fan_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_fan_drivers_register(&drivers);
    if (ret < 0) {
        FAN_ERR("fan drivers register err, ret %d.\n", ret);
        return ret;
    }

    FAN_INFO("fan_init success.\n");
    return 0;
}

static void __exit fan_dev_drv_exit(void)
{
    s3ip_sysfs_fan_drivers_unregister();
    FAN_INFO("fan_exit success.\n");
    return;
}

module_init(fan_dev_drv_init);
module_exit(fan_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("fan device driver");
