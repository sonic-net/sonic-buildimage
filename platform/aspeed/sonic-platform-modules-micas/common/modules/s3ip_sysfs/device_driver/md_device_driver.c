/*
 * md_device_driver.c
 *
 * This module realize /sys/s3ip/md attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "md_sysfs.h"
#include "dfd_sysfs_common.h"

#define MD_INFO(fmt, args...) LOG_INFO("md: ", fmt, ##args)
#define MD_ERR(fmt, args...)  LOG_ERR("md: ", fmt, ##args)
#define MD_DBG(fmt, args...)  LOG_DBG("md: ", fmt, ##args)

static int g_loglevel = 0x0;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_md_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_md_number);

    ret = g_drv->get_md_number();

    return ret;
}

/*****************************************md**********************************************/
/*
 * wb_get_md_attr - Used to get md attr
 * @index: md index
 * @type: md attr type
 * @buf: md attr buffer, NA for not support
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_md_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_md_attr);

    ret = g_drv->get_md_attr(index, type, buf, count);
    return ret;
}

/*
 * wb_set_debug_md_attr - Used to set the debug attr of MD
 * @type: threshold type
 * @md_index: start with 1
 * @value: the value to be set
 *
 * This function returns 0 on success, otherwise it returns a negative value on failed.
 */
static ssize_t wb_set_debug_md_attr(unsigned int type, unsigned int md_index, const char *value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_debug_md_attr);

    ret = g_drv->set_debug_md_attr(type, md_index, value);
    return ret;
}

/**************************************end of md******************************************/

static struct s3ip_sysfs_md_drivers_s drivers = {
    /*
     * set ODM md drivers to /sys/s3ip/md,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_md_number = wb_get_md_number,
    .get_md_attr = wb_get_md_attr,
    .set_debug_md_attr = wb_set_debug_md_attr,
};

static int __init md_init(void)
{
    int ret;

    MD_INFO("md_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_md_drivers_register(&drivers);
    if (ret < 0) {
        MD_ERR("md drivers register err, ret %d.\n", ret);
        return ret;
    }

    MD_INFO("md create success.\n");
    return 0;
}

static void __exit md_exit(void)
{
    s3ip_sysfs_md_drivers_unregister();
    MD_INFO("md_exit ok.\n");
    return;
}

module_init(md_init);
module_exit(md_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("md device driver");