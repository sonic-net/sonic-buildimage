/*
 * curr_sensor_device_driver.c
 *
 * This module realize /sys/s3ip/curr_sensor attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "curr_sensor_sysfs.h"

#define CURR_SENSOR_INFO(fmt, args...) LOG_INFO("curr_sensor: ", fmt, ##args)
#define CURR_SENSOR_ERR(fmt, args...)  LOG_ERR("curr_sensor: ", fmt, ##args)
#define CURR_SENSOR_DBG(fmt, args...)  LOG_DBG("curr_sensor: ", fmt, ##args)

static int g_loglevel = 0;

/*************************************main board current***************************************/
static int demo_get_main_board_curr_number(void)
{
    /* add vendor codes here */
    return 1;
}

/*
 * demo_get_main_board_curr_alias - Used to identify the location of the current sensor,
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_curr_alias(unsigned int curr_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_curr_type - Used to get the model of current sensor,
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_curr_type(unsigned int curr_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_curr_max - Used to get the maximum threshold of current sensor
 * filled the value to buf, and the value keep three decimal places
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_curr_max(unsigned int curr_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_main_board_curr_max - Used to set the maximum threshold of current sensor
 * get value from buf and set it to maximum threshold of current sensor
 * @curr_index: start with 1
 * @buf: the buf store the data to be set
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_main_board_curr_max(unsigned int curr_index, const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_curr_min - Used to get the minimum threshold of current sensor
 * filled the value to buf, and the value keep three decimal places
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_curr_min(unsigned int curr_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_main_board_curr_min - Used to set the minimum threshold of current sensor
 * get value from buf and set it to minimum threshold of current sensor
 * @curr_index: start with 1
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_main_board_curr_min(unsigned int curr_index, const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_curr_value - Used to get the input value of current sensor
 * filled the value to buf, and the value keep three decimal places
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_curr_value(unsigned int curr_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}
/*********************************end of main board current************************************/

static struct s3ip_sysfs_curr_sensor_drivers_s drivers = {
    /*
     * set ODM current sensor drivers to /sys/s3ip/curr_sensor,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_curr_number = demo_get_main_board_curr_number,
    .get_main_board_curr_alias = demo_get_main_board_curr_alias,
    .get_main_board_curr_type = demo_get_main_board_curr_type,
    .get_main_board_curr_max = demo_get_main_board_curr_max,
    .set_main_board_curr_max = demo_set_main_board_curr_max,
    .get_main_board_curr_min = demo_get_main_board_curr_min,
    .set_main_board_curr_min = demo_set_main_board_curr_min,
    .get_main_board_curr_value = demo_get_main_board_curr_value,
};

static int __init curr_sensor_dev_drv_init(void)
{
    int ret;

    CURR_SENSOR_INFO("curr_sensor_init...\n");

    ret = s3ip_sysfs_curr_sensor_drivers_register(&drivers);
    if (ret < 0) {
        CURR_SENSOR_ERR("curr sensor drivers register err, ret %d.\n", ret);
        return ret;
    }

    CURR_SENSOR_INFO("curr_sensor_init success.\n");
    return 0;
}

static void __exit curr_sensor_dev_drv_exit(void)
{
    s3ip_sysfs_curr_sensor_drivers_unregister();
    CURR_SENSOR_INFO("curr_sensor_exit success.\n");
    return;
}

module_init(curr_sensor_dev_drv_init);
module_exit(curr_sensor_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("current sensors device driver");
