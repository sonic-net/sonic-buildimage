/*
 * lsw_device_driver.c
 *
 * This module realize /sys/s3ip/lsw attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "lsw_sysfs.h"
#include "dfd_sysfs_common.h"

#define LSW_INFO(fmt, args...) LOG_INFO("lsw: ", fmt, ##args)
#define LSW_ERR(fmt, args...)  LOG_ERR("lsw: ", fmt, ##args)
#define LSW_DBG(fmt, args...)  LOG_DBG("lsw: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_lsw_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_lsw_number);

    ret = g_drv->get_lsw_number();

    return ret;
}

/*****************************************lsw**********************************************/
/*
 * wb_get_sys_lsw_attr - Used to get sys lsw attr
 * @index: lsw index
 * @type: lsw attr type
 * @buf: lsw attr buffer, NA for not support
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_sys_lsw_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_lsw_attr);

    ret = g_drv->get_lsw_attr(index, type, buf, count);
    return ret;
}

static int wb_set_sys_lsw_attr(unsigned int index, unsigned int type, unsigned int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_lsw_attr);

    ret = g_drv->set_lsw_attr(index, type, status);
    return ret;
}

static struct s3ip_sysfs_lsw_drivers_s drivers = {
    /*
     * set ODM lsw drivers to /sys/s3ip/lsw,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_lsw_number = wb_get_lsw_number,
    .get_lsw_attr = wb_get_sys_lsw_attr,
    .set_lsw_attr = wb_set_sys_lsw_attr,
};

static int __init lsw_init(void)
{
    int ret;

    LSW_INFO("lsw_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_lsw_drivers_register(&drivers);
    if (ret < 0) {
        LSW_ERR("lsw drivers register err, ret %d.\n", ret);
        return ret;
    }

    LSW_INFO("lsw create success.\n");
    return 0;
}

static void __exit lsw_exit(void)
{
    s3ip_sysfs_lsw_drivers_unregister();
    LSW_INFO("lsw_exit ok.\n");
    return;
}

module_init(lsw_init);
module_exit(lsw_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("lsw device driver");