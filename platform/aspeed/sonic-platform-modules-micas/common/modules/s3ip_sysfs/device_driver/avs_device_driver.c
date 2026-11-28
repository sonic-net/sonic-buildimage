/*
 * avs_device_driver.c
 *
 * This module realize /sys/s3ip/avs attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "avs_sysfs.h"
#include "dfd_sysfs_common.h"

#define AVS_INFO(fmt, args...) LOG_INFO("avs: ", fmt, ##args)
#define AVS_ERR(fmt, args...)  LOG_ERR("avs: ", fmt, ##args)
#define AVS_DBG(fmt, args...)  LOG_DBG("avs: ", fmt, ##args)

static int g_loglevel = 0xf;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_avs_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_avs_number);

    ret = g_drv->get_avs_number();

    return ret;
}

/*****************************************avs**********************************************/
/*
 * wb_get_sys_avs_attr - Used to get sys avs attr
 * @index: avs index
 * @type: avs attr type
 * @buf: avs attr buffer, NA for not support
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_sys_avs_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_avs_attr);

    ret = g_drv->get_avs_attr(index, type, buf, count);
    return ret;
}

/*
 * wb_set_debug_avs_attr - Used to set the debug attr of AVS
 * @type: threshold type
 * @index: start with 1
 * @value: the value to be set
 *
 * This function returns 0 on success, otherwise it returns a negative value on failed.
 */
static ssize_t wb_set_debug_avs_attr(unsigned int type, unsigned int index, const char *value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_debug_avs_attr);

    ret = g_drv->set_debug_avs_attr(type, index, value);
    return ret;
}

/**************************************end of avs******************************************/

static struct s3ip_sysfs_avs_drivers_s drivers = {
    /*
     * set ODM avs drivers to /sys/s3ip/avs,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_avs_number = wb_get_avs_number,
    .get_avs_attr = wb_get_sys_avs_attr,
    .set_debug_avs_attr = wb_set_debug_avs_attr,
};

static int __init avs_init(void)
{
    int ret;

    AVS_INFO("avs_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_avs_drivers_register(&drivers);
    if (ret < 0) {
        AVS_ERR("avs drivers register err, ret %d.\n", ret);
        return ret;
    }

    AVS_INFO("avs create success.\n");
    return 0;
}

static void __exit avs_exit(void)
{
    s3ip_sysfs_avs_drivers_unregister();
    AVS_INFO("avs_exit ok.\n");
    return;
}

module_init(avs_init);
module_exit(avs_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("avs device driver");