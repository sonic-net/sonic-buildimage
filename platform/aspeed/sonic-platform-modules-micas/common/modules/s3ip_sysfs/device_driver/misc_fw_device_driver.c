/*
 * misc_fw_device_driver.c
 *
 * This module realize /sys/s3ip/misc_fw attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "misc_fw_sysfs.h"
#include "dfd_sysfs_common.h"

#define MISC_FW_INFO(fmt, args...) LOG_INFO("misc_fw: ", fmt, ##args)
#define MISC_FW_ERR(fmt, args...)  LOG_ERR("misc_fw: ", fmt, ##args)
#define MISC_FW_DBG(fmt, args...)  LOG_DBG("misc_fw: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/******************************************MISC_FW***********************************************/
static int wb_get_main_board_misc_fw_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_misc_fw_number);

    ret = g_drv->get_main_board_misc_fw_number();
    return ret;
}

/*
 * wb_get_main_board_misc_fw_attr - Used to get the attr of misc_fw,
 * @misc_fw_index: start with 1
 * @type: get type
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_main_board_misc_fw_attr(unsigned int misc_fw_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_misc_fw_attr);

    ret = g_drv->get_main_board_misc_fw_attr(misc_fw_index, type, buf, count);
    return ret;
}

/*
 * wb_set_main_board_misc_fw_test_reg - Used to test misc_fw register write
 * @misc_fw_index: start with 1
 * @value: value write to misc_fw
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_main_board_misc_fw_attr(unsigned int misc_fw_index, unsigned int type, unsigned int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_main_board_misc_fw_attr);

    ret = g_drv->set_main_board_misc_fw_attr(misc_fw_index, type, value);
    return ret;
}

/***************************************end of MISC_FW*******************************************/

static struct s3ip_sysfs_misc_fw_drivers_s drivers = {
    /*
     * set ODM MISC_FW drivers to /sys/s3ip/misc_fw,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_misc_fw_number = wb_get_main_board_misc_fw_number,
    .get_main_board_misc_fw_attr = wb_get_main_board_misc_fw_attr,
    .set_main_board_misc_fw_attr = wb_set_main_board_misc_fw_attr,
};

static int __init misc_fw_device_driver_init(void)
{
    int ret;

    MISC_FW_INFO("misc_fw_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_misc_fw_drivers_register(&drivers);
    if (ret < 0) {
        MISC_FW_ERR("misc_fw drivers register err, ret %d.\n", ret);
        return ret;
    }

    MISC_FW_INFO("misc_fw_init success.\n");
    return 0;
}

static void __exit misc_fw_device_driver_exit(void)
{
    s3ip_sysfs_misc_fw_drivers_unregister();
    MISC_FW_INFO("misc_fw_exit success.\n");
    return;
}

module_init(misc_fw_device_driver_init);
module_exit(misc_fw_device_driver_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("misc_fw device driver");
