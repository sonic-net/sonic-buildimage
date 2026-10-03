/*
 * power_sensor_device_driver.c
 *
 * This module realize /sys/s3ip/power_sensor attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "power_sensor_sysfs.h"
#include "dfd_sysfs_common.h"

#define POWER_SENSOR_INFO(fmt, args...) LOG_INFO("power_sensor: ", fmt, ##args)
#define POWER_SENSOR_ERR(fmt, args...)  LOG_ERR("power_sensor: ", fmt, ##args)
#define POWER_SENSOR_DBG(fmt, args...)  LOG_DBG("power_sensor: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/*************************************main board power***************************************/
static int wb_get_main_board_power_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_power_number);

    ret = g_drv->get_main_board_power_number();
    return ret;
}

/*
 * wb_get_main_board_power_alias - Used to identify the location of the power sensor,
 * @power_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_power_alias(unsigned int power_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_power_alias);

    ret = g_drv->get_main_board_power_alias(power_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_power_type - Used to get the model of power sensor,
 * such as udc90160, tps53622 and so on
 * @power_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_power_type(unsigned int power_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_power_type);

    ret = g_drv->get_main_board_power_type(power_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_power_max - Used to get the maximum threshold of power sensor
 * filled the value to buf, the value is integer with mV
 * @power_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_power_max(unsigned int power_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_power_max);

    ret = g_drv->get_main_board_power_max(power_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_power_min - Used to get the minimum threshold of power sensor
 * filled the value to buf, the value is integer with mV
 * @power_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_power_min(unsigned int power_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_power_min);

    ret = g_drv->get_main_board_power_min(power_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_power_value - Used to get the input value of power sensor
 * filled the value to buf, the value is integer with mV
 * @power_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_power_value(unsigned int power_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_power_value);

    ret = g_drv->get_main_board_power_value(power_index, buf, count);
    return ret;
}

/*
 * wb_get_main_board_power_monitor_flag - Used to get the monitor flag of power sensor
 * filled the value to buf, the value is integer with mV
 * @power_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_power_monitor_flag(unsigned int power_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_power_monitor_flag);

    ret = g_drv->get_main_board_power_monitor_flag(power_index, buf, count);
    return ret;
}
/*********************************end of main board power************************************/

static struct s3ip_sysfs_power_sensor_drivers_s drivers = {
    /*
     * set ODM power sensor drivers to /sys/s3ip/power_sensor,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_power_number = wb_get_main_board_power_number,
    .get_main_board_power_alias = wb_get_main_board_power_alias,
    .get_main_board_power_type = wb_get_main_board_power_type,
    .get_main_board_power_max = wb_get_main_board_power_max,
    .get_main_board_power_min = wb_get_main_board_power_min,
    .get_main_board_power_value = wb_get_main_board_power_value,
    .get_main_board_power_monitor_flag = wb_get_main_board_power_monitor_flag,
};

static int __init power_sensor_dev_drv_init(void)
{
    int ret;

    POWER_SENSOR_INFO("power_sensor_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_power_sensor_drivers_register(&drivers);
    if (ret < 0) {
        POWER_SENSOR_ERR("power sensor drivers register err, ret %d.\n", ret);
        return ret;
    }
    POWER_SENSOR_INFO("power_sensor_init success.\n");
    return 0;
}

static void __exit power_sensor_dev_drv_exit(void)
{
    s3ip_sysfs_power_sensor_drivers_unregister();
    POWER_SENSOR_INFO("power_sensor_exit success.\n");
    return;
}

module_init(power_sensor_dev_drv_init);
module_exit(power_sensor_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("power sensors device driver");
