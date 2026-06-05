/*
 * debug_device_driver.c
 *
 * This module realize /sys/s3ip/debug attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2023-10-27                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "debug_sysfs.h"
#include "dfd_sysfs_common.h"

#define DEBUG_INFO(fmt, args...) LOG_INFO("debug: ", fmt, ##args)
#define DEBUG_ERR(fmt, args...)  LOG_ERR("debug: ", fmt, ##args)
#define DEBUG_DBG(fmt, args...)  LOG_DBG("debug: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

static int wb_set_debug_and_reset(unsigned int debug_mode)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_debug_and_reset);

    ret = g_drv->set_debug_and_reset(debug_mode);
    return ret;
}


static struct s3ip_sysfs_debug_drivers_s drivers = {
    /*
     * set ODM debug drivers to /sys/s3ip/debug,
     * if not support the function, set corresponding hook to NULL.
     */
    .set_debug_and_reset = wb_set_debug_and_reset,
};

static int __init debug_dev_drv_init(void)
{
    int ret;

    DEBUG_INFO("debug_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_debug_drivers_register(&drivers);
    if (ret < 0) {
        DEBUG_ERR("debug drivers register err, ret %d.\n", ret);
        return ret;
    }
    DEBUG_INFO("debug create success.\n");
    return 0;
}

static void __exit debug_dev_drv_exit(void)
{
    s3ip_sysfs_debug_drivers_unregister();
    DEBUG_INFO("debug_exit success.\n");
    return;
}

module_init(debug_dev_drv_init);
module_exit(debug_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("debug device driver");
