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

#define PSU_INFO(fmt, args...) LOG_INFO("psu: ", fmt, ##args)
#define PSU_ERR(fmt, args...)  LOG_ERR("psu: ", fmt, ##args)
#define PSU_DBG(fmt, args...)  LOG_DBG("psu: ", fmt, ##args)

static int g_loglevel = 0;

/********************************************psu**********************************************/
static int demo_get_psu_number(void)
{
    /* add vendor codes here */
    return 1;
}

static int demo_get_psu_temp_number(unsigned int psu_index)
{
    /* add vendor codes here */
    return 1;
}

/*
 * demo_get_psu_model_name - Used to get psu model name,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_model_name(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_serial_number - Used to get psu serial number,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_serial_number(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_part_number - Used to get psu part number,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_part_number(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_hardware_version - Used to get psu hardware version,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_hardware_version(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_type - Used to get the input type of psu
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
static ssize_t demo_get_psu_type(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_in_curr - Used to get the input current of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_in_curr(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_in_vol - Used to get the input voltage of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_in_vol(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_in_power - Used to get the input power of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_in_power(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_out_curr - Used to get the output current of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_out_curr(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_out_vol - Used to get the output voltage of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_out_vol(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_out_power - Used to get the output power of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_out_power(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_out_max_power - Used to get the output max power of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_out_max_power(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_present_status - Used to get psu present status
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
static ssize_t demo_get_psu_present_status(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_in_status - Used to get psu input status
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
static ssize_t demo_get_psu_in_status(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_out_status - Used to get psu output status
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
static ssize_t demo_get_psu_out_status(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_fan_speed - Used to get psu fan speed
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_fan_speed(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_fan_ratio - Used to get the ratio of psu fan
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_fan_ratio(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_psu_fan_ratio - Used to set the ratio of psu fan
 * @psu_index: start with 1
 * @ratio: from 0 to 100
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_psu_fan_ratio(unsigned int psu_index, int ratio)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_fan_direction - Used to get psu air flow direction,
 * filled the value to buf, air flow direction define as below:
 * 0: F2B
 * 1: B2F
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_fan_direction(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_led_status - Used to get psu led status
 * filled the value to buf, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4：blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8：blue light flashing
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_led_status(unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_temp_alias - Used to identify the location of the temperature sensor of psu,
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_temp_alias(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_temp_type - Used to get the model of temperature sensor of psu,
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_temp_type(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_temp_max - Used to get the maximum threshold of temperature sensor of psu,
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_temp_max(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_psu_temp_max - Used to set the maximum threshold of temperature sensor of psu,
 * get value from buf and set it to maximum threshold of psu temperature sensor
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '80.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_psu_temp_max(unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_temp_min - Used to get the minimum threshold of temperature sensor of psu,
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_temp_min(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_psu_temp_min - Used to set the minimum threshold of temperature sensor of psu,
 * get value from buf and set it to minimum threshold of psu temperature sensor
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_psu_temp_min(unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_psu_temp_value - Used to get the input value of temperature sensor of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_psu_temp_value(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}
/****************************************end of psu*******************************************/

static struct s3ip_sysfs_psu_drivers_s drivers = {
    /*
     * set ODM psu drivers to /sys/s3ip/psu,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_psu_number = demo_get_psu_number,
    .get_psu_temp_number = demo_get_psu_temp_number,
    .get_psu_model_name = demo_get_psu_model_name,
    .get_psu_serial_number = demo_get_psu_serial_number,
    .get_psu_part_number = demo_get_psu_part_number,
    .get_psu_hardware_version = demo_get_psu_hardware_version,
    .get_psu_type = demo_get_psu_type,
    .get_psu_in_curr = demo_get_psu_in_curr,
    .get_psu_in_vol = demo_get_psu_in_vol,
    .get_psu_in_power = demo_get_psu_in_power,
    .get_psu_out_curr = demo_get_psu_out_curr,
    .get_psu_out_vol = demo_get_psu_out_vol,
    .get_psu_out_power = demo_get_psu_out_power,
    .get_psu_out_max_power = demo_get_psu_out_max_power,
    .get_psu_present_status = demo_get_psu_present_status,
    .get_psu_in_status = demo_get_psu_in_status,
    .get_psu_out_status = demo_get_psu_out_status,
    .get_psu_fan_speed = demo_get_psu_fan_speed,
    .get_psu_fan_ratio = demo_get_psu_fan_ratio,
    .set_psu_fan_ratio = demo_set_psu_fan_ratio,
    .get_psu_fan_direction = demo_get_psu_fan_direction,
    .get_psu_led_status = demo_get_psu_led_status,
    .get_psu_temp_alias = demo_get_psu_temp_alias,
    .get_psu_temp_type = demo_get_psu_temp_type,
    .get_psu_temp_max = demo_get_psu_temp_max,
    .set_psu_temp_max = demo_set_psu_temp_max,
    .get_psu_temp_min = demo_get_psu_temp_min,
    .set_psu_temp_min = demo_set_psu_temp_min,
    .get_psu_temp_value = demo_get_psu_temp_value,
};

static int __init psu_dev_drv_init(void)
{
    int ret;

    PSU_INFO("psu_init...\n");

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
