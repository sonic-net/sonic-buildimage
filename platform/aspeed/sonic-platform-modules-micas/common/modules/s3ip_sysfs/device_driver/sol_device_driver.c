/*
 * sol_device_driver.c
 *
 * This module realize /sys/s3ip/sol attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "sol_sysfs.h"
#include "dfd_sysfs_common.h"

#define LEAK_DETECTOR_INFO(fmt, args...) LOG_INFO("sol: ", fmt, ##args)
#define LEAK_DETECTOR_ERR(fmt, args...)  LOG_ERR("sol: ", fmt, ##args)
#define LEAK_DETECTOR_DBG(fmt, args...)  LOG_DBG("sol: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_sys_sol_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_sol_number);

    ret = g_drv->get_main_board_sol_number();
    return ret;
}

/*****************************************sol**********************************************/


/* Similar to wb_set_sys_sol_attr */
static int wb_set_sys_sol_attr(unsigned int index, unsigned int type, unsigned int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_main_board_sol_attr);

    ret = g_drv->set_main_board_sol_attr(index, type, status);
    return ret;
}

/* Similar to wb_get_sys_sol_attr */
static ssize_t wb_get_sys_sol_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_sol_attr);

    ret = g_drv->get_main_board_sol_attr(index, type, buf, count);
    return ret;
}

/**************************************end of sol******************************************/

static struct s3ip_sysfs_sol_drivers_s drivers = {
    .get_main_board_sol_number = wb_get_sys_sol_number,
    .set_main_board_sol_attr = wb_set_sys_sol_attr,
    .get_main_board_sol_attr = wb_get_sys_sol_attr,
};

static int __init sol_init(void)
{
    int ret;

    LEAK_DETECTOR_INFO("sol_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_sol_drivers_register(&drivers);
    if (ret < 0) {
        LEAK_DETECTOR_ERR("sol drivers register err, ret %d.\n", ret);
        return ret;
    }

    LEAK_DETECTOR_INFO("sol create success.\n");
    return 0;
}

static void __exit sol_exit(void)
{
    s3ip_sysfs_sol_drivers_unregister();
    LEAK_DETECTOR_INFO("sol_exit ok.\n");
    return;
}

module_init(sol_init);
module_exit(sol_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("sol device driver");