/*
 * disk_device_driver.c
 *
 * This module realize /sys/s3ip/disk attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "disk_sysfs.h"
#include "dfd_sysfs_common.h"

#define DISK_INFO(fmt, args...) LOG_INFO("disk: ", fmt, ##args)
#define DISK_ERR(fmt, args...)  LOG_ERR("disk: ", fmt, ##args)
#define DISK_DBG(fmt, args...)  LOG_DBG("disk: ", fmt, ##args)

static int g_loglevel = 0x0;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_disk_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_disk_number);

    ret = g_drv->get_disk_number();

    return ret;
}

/*****************************************disk**********************************************/
/*
 * wb_get_disk_attr - Used to get disk attr
 * @index: disk index
 * @type: disk attr type
 * @buf: disk attr buffer, NA for not support
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_disk_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_disk_attr);

    ret = g_drv->get_disk_attr(index, type, buf, count);
    return ret;
}

/*
 * wb_set_debug_disk_attr - Used to set the debug attr of DISK
 * @type: threshold type
 * @disk_index: start with 1
 * @value: the value to be set
 *
 * This function returns 0 on success, otherwise it returns a negative value on failed.
 */
static ssize_t wb_set_debug_disk_attr(unsigned int type, unsigned int disk_index, const char *value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_debug_disk_attr);

    ret = g_drv->set_debug_disk_attr(type, disk_index, value);
    return ret;
}

/**************************************end of disk******************************************/

static struct s3ip_sysfs_disk_drivers_s drivers = {
    /*
     * set ODM disk drivers to /sys/s3ip/disk,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_disk_number = wb_get_disk_number,
    .get_disk_attr = wb_get_disk_attr,
    .set_debug_disk_attr = wb_set_debug_disk_attr,
};

static int __init disk_init(void)
{
    int ret;

    DISK_INFO("disk_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_disk_drivers_register(&drivers);
    if (ret < 0) {
        DISK_ERR("disk drivers register err, ret %d.\n", ret);
        return ret;
    }

    DISK_INFO("disk create success.\n");
    return 0;
}

static void __exit disk_exit(void)
{
    s3ip_sysfs_disk_drivers_unregister();
    DISK_INFO("disk_exit ok.\n");
    return;
}

module_init(disk_init);
module_exit(disk_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("disk device driver");