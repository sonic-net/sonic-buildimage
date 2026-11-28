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
#include "dfd_sysfs_common.h"

#define SYSE2_INFO(fmt, args...)  LOG_INFO("syseeprom: ", fmt, ##args)
#define SYSE2_ERR(fmt, args...)   LOG_ERR("syseeprom: ", fmt, ##args)
#define SYSE2_DBG(fmt, args...)   LOG_DBG("syseeprom: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/*****************************************syseeprom*******************************************/
/*
 * wb_get_syseeprom_size - Used to get syseeprom size
 *
 * This function returns the size of syseeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int wb_get_syseeprom_size(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_syseeprom_size);

    ret = g_drv->get_syseeprom_size();
    return ret;
}

/*
 * wb_read_syseeprom_data - Used to read syseeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read syseeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_read_syseeprom_data(char *buf, loff_t offset, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->read_syseeprom_data);

    ret = g_drv->read_syseeprom_data(buf, offset, count);
    return ret;
}

/*
 * wb_write_syseeprom_data - Used to write syseeprom data
 * @buf: Data write buffer
 * @offset: offset address to write syseeprom data
 * @count: length of buf
 *
 * This function returns the written length of syseeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_write_syseeprom_data(char *buf, loff_t offset, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->write_syseeprom_data);

    ret = g_drv->write_syseeprom_data(buf, offset, count);
    return ret;
}
/*************************************end of syseeprom****************************************/

/*****************************************debug*******************************************/
/*
 * wb_reload_s3ip_config - Used to reload s3ip config
 *
 * This function returns the reload s3ip config,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static int wb_reload_s3ip_config(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->reload_s3ip_config);

    ret = g_drv->reload_s3ip_config();
    return ret;
}

/*************************************end of debug****************************************/

static struct s3ip_sysfs_syseeprom_drivers_s drivers = {
    /*
     * set ODM syseeprom drivers to /sys/s3ip/syseeprom,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_syseeprom_size = wb_get_syseeprom_size,
    .read_syseeprom_data = wb_read_syseeprom_data,
    .write_syseeprom_data = wb_write_syseeprom_data,
    .reload_s3ip_config = wb_reload_s3ip_config,
};

static int __init syseeprom_dev_drv_init(void)
{
    int ret;

    SYSE2_INFO("syseeprom_dev_drv_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

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
