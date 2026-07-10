/*
 * temp_sensor_device_driver.c
 *
 * This module realize /sys/s3ip/temp_sensor attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "temp_sensor_sysfs.h"

#define TEMP_SENSOR_INFO(fmt, args...) LOG_INFO("temp_sensor: ", fmt, ##args)
#define TEMP_SENSOR_ERR(fmt, args...)  LOG_ERR("temp_sensor: ", fmt, ##args)
#define TEMP_SENSOR_DBG(fmt, args...)  LOG_DBG("temp_sensor: ", fmt, ##args)

static int g_loglevel = 0;

/***************************************main board temp*****************************************/
/*
 * demo_get_main_board_temp_number - Used to get main board temperature sensors number,
 *
 * This function returns main board temperature sensors by your switch,
 * If there is no main board temperature sensors, returns 0,
 * otherwise it returns a negative value on failed.
 */
static int demo_get_main_board_temp_number(void)
{
    /* add vendor codes here */
    return 1;
}

/*
 * demo_get_main_board_temp_alias - Used to identify the location of the temperature sensor,
 * such as air_inlet, air_outlet and so on.
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_temp_alias(unsigned int temp_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_temp_type - Used to get the model of temperature sensor,
 * such as lm75, tmp411 and so on
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_temp_type(unsigned int temp_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_temp_max - Used to get the maximum threshold of temperature sensor
 * filled the value to buf, and the value keep three decimal places
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_temp_max(unsigned int temp_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_main_board_temp_max - Used to set the maximum threshold of temperature sensor
 * get value from buf and set it to maximum threshold of temperature sensor
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '80.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_main_board_temp_max(unsigned int temp_index, const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_temp_min - Used to get the minimum threshold of temperature sensor
 * filled the value to buf, and the value keep three decimal places
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_temp_min(unsigned int temp_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_main_board_temp_min - Used to set the minimum threshold of temperature sensor
 * get value from buf and set it to minimum threshold of temperature sensor
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_main_board_temp_min(unsigned int temp_index, const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_temp_value - Used to get the input value of temperature sensor
 * filled the value to buf, and the value keep three decimal places
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_temp_value(unsigned int temp_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}
/***********************************end of main board temp*************************************/

static struct s3ip_sysfs_temp_sensor_drivers_s drivers = {
    /*
     * set ODM temperature sensor drivers to /sys/s3ip/temp_sensor,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_temp_number = demo_get_main_board_temp_number,
    .get_main_board_temp_alias = demo_get_main_board_temp_alias,
    .get_main_board_temp_type = demo_get_main_board_temp_type,
    .get_main_board_temp_max = demo_get_main_board_temp_max,
    .set_main_board_temp_max = demo_set_main_board_temp_max,
    .get_main_board_temp_min = demo_get_main_board_temp_min,
    .set_main_board_temp_min = demo_set_main_board_temp_min,
    .get_main_board_temp_value = demo_get_main_board_temp_value,
};

static int __init temp_sensor_dev_drv_init(void)
{
    int ret;

    TEMP_SENSOR_INFO("temp_sensor_init...\n");

    ret = s3ip_sysfs_temp_sensor_drivers_register(&drivers);
    if (ret < 0) {
        TEMP_SENSOR_ERR("temp sensor drivers register err, ret %d.\n", ret);
        return ret;
    }
    TEMP_SENSOR_INFO("temp_sensor_init success.\n");
    return 0;
}

static void __exit temp_sensor_dev_drv_exit(void)
{
    s3ip_sysfs_temp_sensor_drivers_unregister();
    TEMP_SENSOR_INFO("temp_sensor_exit success.\n");
    return;
}

module_init(temp_sensor_dev_drv_init);
module_exit(temp_sensor_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("temperature sensors device driver");
