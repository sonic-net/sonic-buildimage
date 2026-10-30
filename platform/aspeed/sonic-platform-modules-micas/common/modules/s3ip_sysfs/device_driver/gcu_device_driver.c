/*
 * gcu_device_driver.c
 *
 * This module realize /sys/s3ip/gcu attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "gcu_sysfs.h"
#include "dfd_sysfs_common.h"

#define GCU_INFO(fmt, args...) LOG_INFO("gcu: ", fmt, ##args)
#define GCU_ERR(fmt, args...)  LOG_ERR("gcu: ", fmt, ##args)
#define GCU_DBG(fmt, args...)  LOG_DBG("gcu: ", fmt, ##args)

static int g_loglevel = 0x0;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_gcu_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_gcu_number);

    ret = g_drv->get_gcu_number();

    return ret;
}

/*****************************************gcu**********************************************/
/*
 * wb_get_sys_gcu_status - Used to get sys gcu attr
 * filgcu the value to buf, gcu status value define as below:
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filgcu buffer,
 * if not support this attributes filgcu "NA" to buf,
 * otherwise it returns a negative value on faigcu.
 */
static ssize_t wb_get_sys_gcu_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_gcu_attr);

    ret = g_drv->get_main_board_gcu_attr(index, type, buf, count);
    return ret;
}

static ssize_t wb_get_sys_gcu_temp_attr(unsigned int gcu_index, unsigned int temp_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_gcu_temp_attr);

    ret = g_drv->get_gcu_temp_attr(gcu_index, temp_index, type, buf, count);
    return ret;
}

static ssize_t wb_get_sys_gcu_vol_attr(unsigned int gcu_index, unsigned int vol_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_gcu_vol_attr);

    ret = g_drv->get_gcu_vol_attr(gcu_index, vol_index, type, buf, count);
    return ret;
}

static ssize_t wb_get_sys_gcu_power_attr(unsigned int gcu_index, unsigned int power_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_gcu_power_attr);

    ret = g_drv->get_gcu_power_attr(gcu_index, power_index, type, buf, count);
    return ret;
}

static int wb_get_gcu_temp_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_gcu_temp_number);

    ret = g_drv->get_gcu_temp_number();
    return ret;
}

static int wb_get_gcu_vol_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_gcu_vol_number);

    ret = g_drv->get_gcu_vol_number();
    return ret;
}

static int wb_get_gcu_power_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_gcu_power_number);

    ret = g_drv->get_gcu_power_number();
    return ret;
}
/**************************************end of gcu******************************************/

static struct s3ip_sysfs_gcu_drivers_s drivers = {
    /*
     * set ODM gcu drivers to /sys/s3ip/gcu,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_gcu_number = wb_get_gcu_number,
    .get_main_board_gcu_attr = wb_get_sys_gcu_attr,
    .get_gcu_temp_attr = wb_get_sys_gcu_temp_attr,
    .get_gcu_vol_attr = wb_get_sys_gcu_vol_attr,
    .get_gcu_power_attr = wb_get_sys_gcu_power_attr,
    .get_gcu_temp_number = wb_get_gcu_temp_number,
    .get_gcu_vol_number = wb_get_gcu_vol_number,
    .get_gcu_power_number = wb_get_gcu_power_number,
};

static int __init gcu_init(void)
{
    int ret;

    GCU_INFO("gcu_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_gcu_drivers_register(&drivers);
    if (ret < 0) {
        GCU_ERR("gcu drivers register err, ret %d.\n", ret);
        return ret;
    }

    GCU_INFO("gcu create success.\n");
    return 0;
}

static void __exit gcu_exit(void)
{
    s3ip_sysfs_gcu_drivers_unregister();
    GCU_INFO("gcu_exit ok.\n");
    return;
}

module_init(gcu_init);
module_exit(gcu_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("bmc S3IP sysfs");
MODULE_DESCRIPTION("gcu device driver");