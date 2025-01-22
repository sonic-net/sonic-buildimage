/*
 * cpld_device_driver.c
 *
 * This module realize /sys/s3ip/cpld attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "cpld_sysfs.h"

#define CPLD_INFO(fmt, args...) LOG_INFO("cpld: ", fmt, ##args)
#define CPLD_ERR(fmt, args...)  LOG_ERR("cpld: ", fmt, ##args)
#define CPLD_DBG(fmt, args...)  LOG_DBG("cpld: ", fmt, ##args)

static int g_loglevel = 0;

/******************************************CPLD***********************************************/
static int demo_get_main_board_cpld_number(void)
{
    /* add vendor codes here */
    return 1;
}

/*
 * demo_get_main_board_cpld_alias - Used to identify the location of cpld,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_cpld_alias(unsigned int cpld_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_cpld_type - Used to get cpld model name
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_cpld_type(unsigned int cpld_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_cpld_firmware_version - Used to get cpld firmware version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_cpld_firmware_version(unsigned int cpld_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_cpld_board_version - Used to get cpld board version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_cpld_board_version(unsigned int cpld_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_main_board_cpld_test_reg - Used to test cpld register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_main_board_cpld_test_reg(unsigned int cpld_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_main_board_cpld_test_reg - Used to test cpld register write
 * @cpld_index: start with 1
 * @value: value write to cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_main_board_cpld_test_reg(unsigned int cpld_index, unsigned int value)
{
    /* add vendor codes here */
    return -ENOSYS;
}
/***************************************end of CPLD*******************************************/

static struct s3ip_sysfs_cpld_drivers_s drivers = {
    /*
     * set ODM CPLD drivers to /sys/s3ip/cpld,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_cpld_number = demo_get_main_board_cpld_number,
    .get_main_board_cpld_alias = demo_get_main_board_cpld_alias,
    .get_main_board_cpld_type = demo_get_main_board_cpld_type,
    .get_main_board_cpld_firmware_version = demo_get_main_board_cpld_firmware_version,
    .get_main_board_cpld_board_version = demo_get_main_board_cpld_board_version,
    .get_main_board_cpld_test_reg = demo_get_main_board_cpld_test_reg,
    .set_main_board_cpld_test_reg = demo_set_main_board_cpld_test_reg,
};

static int __init cpld_device_driver_init(void)
{
    int ret;

    CPLD_INFO("cpld_init...\n");

    ret = s3ip_sysfs_cpld_drivers_register(&drivers);
    if (ret < 0) {
        CPLD_ERR("cpld drivers register err, ret %d.\n", ret);
        return ret;
    }

    CPLD_INFO("cpld_init success.\n");
    return 0;
}

static void __exit cpld_device_driver_exit(void)
{
    s3ip_sysfs_cpld_drivers_unregister();
    CPLD_INFO("cpld_exit success.\n");
    return;
}

module_init(cpld_device_driver_init);
module_exit(cpld_device_driver_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("cpld device driver");
