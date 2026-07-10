/*
 * vol_sensor_device_driver.c
 *
 * This module realize /sys/s3ip/vol_sensor attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "vol_sensor_sysfs.h"

#define VOL_SENSOR_INFO(fmt, args...) LOG_INFO("vol_sensor: ", fmt, ##args)
#define VOL_SENSOR_ERR(fmt, args...)  LOG_ERR("vol_sensor: ", fmt, ##args)
#define VOL_SENSOR_DBG(fmt, args...)  LOG_DBG("vol_sensor: ", fmt, ##args)

static int g_loglevel = 0;

/*************************************main board voltage***************************************/
static int demo_get_main_board_vol_number(void)
{
    /* add vendor codes here */
    return 1;
}

/*
 * demo_get_main_board_vol_alias - Used to identify the location of the voltage sensor,
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_vol_alias(unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_vol_type - Used to get the model of voltage sensor,
 * such as udc90160, tps53622 and so on
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_vol_type(unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_vol_max - Used to get the maximum threshold of voltage sensor
 * filled the value to buf, and the value keep three decimal places
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_vol_max(unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_main_board_vol_max - Used to set the maximum threshold of volatge sensor
 * get value from buf and set it to maximum threshold of volatge sensor
 * @vol_index: start with 1
 * @buf: the buf store the data to be set, eg '3.567'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_main_board_vol_max(unsigned int vol_index, const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_vol_min - Used to get the minimum threshold of voltage sensor
 * filled the value to buf, and the value keep three decimal places
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_vol_min(unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_main_board_vol_min - Used to set the minimum threshold of voltage sensor
 * get value from buf and set it to minimum threshold of voltage sensor
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '3.123'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_main_board_vol_min(unsigned int vol_index, const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_vol_range - Used to get the output error value of voltage sensor
 * filled the value to buf
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_vol_range(unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_vol_nominal_value - Used to get the nominal value of voltage sensor
 * filled the value to buf
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_vol_nominal_value(unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_vol_value - Used to get the input value of voltage sensor
 * filled the value to buf, and the value keep three decimal places
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_vol_value(unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}
/*********************************end of main board voltage************************************/

static struct s3ip_sysfs_vol_sensor_drivers_s drivers = {
    /*
     * set ODM voltage sensor drivers to /sys/s3ip/vol_sensor,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_vol_number = demo_get_main_board_vol_number,
    .get_main_board_vol_alias = demo_get_main_board_vol_alias,
    .get_main_board_vol_type = demo_get_main_board_vol_type,
    .get_main_board_vol_max = demo_get_main_board_vol_max,
    .set_main_board_vol_max = demo_set_main_board_vol_max,
    .get_main_board_vol_min = demo_get_main_board_vol_min,
    .set_main_board_vol_min = demo_set_main_board_vol_min,
    .get_main_board_vol_range = demo_get_main_board_vol_range,
    .get_main_board_vol_nominal_value = demo_get_main_board_vol_nominal_value,
    .get_main_board_vol_value = demo_get_main_board_vol_value,
};

static int __init vol_sensor_dev_drv_init(void)
{
    int ret;

    VOL_SENSOR_INFO("vol_sensor_init...\n");

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
