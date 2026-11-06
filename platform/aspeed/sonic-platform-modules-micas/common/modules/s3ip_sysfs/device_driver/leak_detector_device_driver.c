/*
 * leak_detector_device_driver.c
 *
 * This module realize /sys/s3ip/leak_detector attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "leak_detector_sysfs.h"
#include "dfd_sysfs_common.h"

#define LEAK_DETECTOR_INFO(fmt, args...) LOG_INFO("leak_detector: ", fmt, ##args)
#define LEAK_DETECTOR_ERR(fmt, args...)  LOG_ERR("leak_detector: ", fmt, ##args)
#define LEAK_DETECTOR_DBG(fmt, args...)  LOG_DBG("leak_detector: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_leak_detector_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_leak_number);

    ret = g_drv->get_main_board_leak_number();
    return ret;
}

/*****************************************leak_detector**********************************************/
/*
 * wb_get_sys_leak_detector_status - Used to get sys leak_detector attr
 * filleak_detector the value to buf, leak_detector status value define as below:
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filleak_detector buffer,
 * if not support this attributes filleak_detector "NA" to buf,
 * otherwise it returns a negative value on faileak_detector.
 */
static ssize_t wb_get_sys_leak_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_leak_attr);

    ret = g_drv->get_main_board_leak_attr(index, type, buf, count);
    return ret;
}

static int wb_set_sys_leak_attr(unsigned int index, unsigned int type, unsigned int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_main_board_leak_attr);

    ret = g_drv->set_main_board_leak_attr(index, type, status);
    return ret;
}
/**************************************end of leak_detector******************************************/

static struct s3ip_sysfs_leak_detector_drivers_s drivers = {
    /*
     * set ODM leak_detector drivers to /sys/s3ip/leak_detector,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_leak_number = wb_get_leak_detector_number,
    .set_main_board_leak_attr = wb_set_sys_leak_attr,
    .get_main_board_leak_attr = wb_get_sys_leak_attr,
};

static int __init leak_detector_init(void)
{
    int ret;

    LEAK_DETECTOR_INFO("leak_detector_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_leak_detector_drivers_register(&drivers);
    if (ret < 0) {
        LEAK_DETECTOR_ERR("leak_detector drivers register err, ret %d.\n", ret);
        return ret;
    }

    LEAK_DETECTOR_INFO("leak_detector create success.\n");
    return 0;
}

static void __exit leak_detector_exit(void)
{
    s3ip_sysfs_leak_detector_drivers_unregister();
    LEAK_DETECTOR_INFO("leak_detector_exit ok.\n");
    return;
}

module_init(leak_detector_init);
module_exit(leak_detector_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("leak_detector device driver");