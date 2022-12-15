/*
 * syseeprom_device_driver.c
 *
 * This module realize /sys/s3ip/syseeprom attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "syseeprom_sysfs.h"

#define SYSE2_INFO(fmt, args...)  LOG_INFO("syseeprom: ", fmt, ##args)
#define SYSE2_ERR(fmt, args...)   LOG_ERR("syseeprom: ", fmt, ##args)
#define SYSE2_DBG(fmt, args...)   LOG_DBG("syseeprom: ", fmt, ##args)

static int g_loglevel = 0;

/*****************************************syseeprom*******************************************/
/*
 * demo_get_syseeprom_size - Used to get syseeprom size
 *
 * This function returns the size of syseeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int demo_get_syseeprom_size(void)
{
    /* add vendor codes here */
    return 256;
}

/*
 * demo_read_syseeprom_data - Used to read syseeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read syseeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_read_syseeprom_data(char *buf, loff_t offset, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_write_syseeprom_data - Used to write syseeprom data
 * @buf: Data write buffer
 * @offset: offset address to write syseeprom data
 * @count: length of buf
 *
 * This function returns the written length of syseeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_write_syseeprom_data(char *buf, loff_t offset, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}
/*************************************end of syseeprom****************************************/

static struct s3ip_sysfs_syseeprom_drivers_s drivers = {
    /*
     * set ODM syseeprom drivers to /sys/s3ip/syseeprom,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_syseeprom_size = demo_get_syseeprom_size,
    .read_syseeprom_data = demo_read_syseeprom_data,
    .write_syseeprom_data = demo_write_syseeprom_data,
};

static int __init syseeprom_dev_drv_init(void)
{
    int ret;

    SYSE2_INFO("syseeprom_dev_drv_init...\n");

    ret = s3ip_sysfs_syseeprom_drivers_register(&drivers);
    if (ret < 0) {
        SYSE2_ERR("syseeprom drivers register err, ret %d.\n", ret);
        return ret;
    }
    SYSE2_INFO("syseeprom_dev_drv_init success.\n");
    return 0;
}

static void __exit syseeprom_dev_drv_exit(void)
{
    s3ip_sysfs_syseeprom_drivers_unregister();
    SYSE2_INFO("syseeprom_exit success.\n");
    return;
}

module_init(syseeprom_dev_drv_init);
module_exit(syseeprom_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("syseeprom device driver");
