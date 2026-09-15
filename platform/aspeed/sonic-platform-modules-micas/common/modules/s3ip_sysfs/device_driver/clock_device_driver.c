/*
 * clock_device_driver.c
 *
 * This module realize /sys/s3ip/clock attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "clock_sysfs.h"
#include "dfd_sysfs_common.h"

#define CLOCK_INFO(fmt, args...) LOG_INFO("clock: ", fmt, ##args)
#define CLOCK_ERR(fmt, args...)  LOG_ERR("clock: ", fmt, ##args)
#define CLOCK_DBG(fmt, args...)  LOG_DBG("clock: ", fmt, ##args)

static int g_loglevel = 0xf;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_clock_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_clock_number);

    ret = g_drv->get_clock_number();

    return ret;
}

/*****************************************clock**********************************************/
/*
 * wb_get_sys_clock_attr - Used to get sys clock attr
 * @index: clock index
 * @type: clock attr type
 * @buf: clock attr buffer, NA for not support
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_sys_clock_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_clock_attr);

    ret = g_drv->get_clock_attr(index, type, buf, count);
    return ret;
}

/**************************************end of clock******************************************/

static struct s3ip_sysfs_clock_drivers_s drivers = {
    /*
     * set ODM clock drivers to /sys/s3ip/clock,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_clock_number = wb_get_clock_number,
    .get_clock_attr = wb_get_sys_clock_attr,
};

static int __init clock_init(void)
{
    int ret;

    CLOCK_INFO("clock_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_clock_drivers_register(&drivers);
    if (ret < 0) {
        CLOCK_ERR("clock drivers register err, ret %d.\n", ret);
        return ret;
    }

    CLOCK_INFO("clock create success.\n");
    return 0;
}

static void __exit clock_exit(void)
{
    s3ip_sysfs_clock_drivers_unregister();
    CLOCK_INFO("clock_exit ok.\n");
    return;
}

module_init(clock_init);
module_exit(clock_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("clock device driver");