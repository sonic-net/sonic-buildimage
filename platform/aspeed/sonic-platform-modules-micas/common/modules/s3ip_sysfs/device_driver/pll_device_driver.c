/*
 * pll_device_driver.c
 *
 * This module realize /sys/s3ip/pll attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "pll_sysfs.h"
#include "dfd_sysfs_common.h"

#define PLL_INFO(fmt, args...) LOG_INFO("pll: ", fmt, ##args)
#define PLL_ERR(fmt, args...)  LOG_ERR("pll: ", fmt, ##args)
#define PLL_DBG(fmt, args...)  LOG_DBG("pll: ", fmt, ##args)

static int g_loglevel = 0xf;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_pll_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_pll_number);

    ret = g_drv->get_pll_number();

    return ret;
}

/*****************************************pll**********************************************/
/*
 * wb_get_sys_pll_attr - Used to get sys pll attr
 * @index: pll index
 * @type: pll attr type
 * @buf: pll attr buffer, NA for not support
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_sys_pll_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_pll_attr);

    ret = g_drv->get_pll_attr(index, type, buf, count);
    return ret;
}

/**************************************end of pll******************************************/

static struct s3ip_sysfs_pll_drivers_s drivers = {
    /*
     * set ODM pll drivers to /sys/s3ip/pll,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_pll_number = wb_get_pll_number,
    .get_pll_attr = wb_get_sys_pll_attr,
};

static int __init pll_init(void)
{
    int ret;

    PLL_INFO("pll_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_pll_drivers_register(&drivers);
    if (ret < 0) {
        PLL_ERR("pll drivers register err, ret %d.\n", ret);
        return ret;
    }

    PLL_INFO("pll create success.\n");
    return 0;
}

static void __exit pll_exit(void)
{
    s3ip_sysfs_pll_drivers_unregister();
    PLL_INFO("pll_exit ok.\n");
    return;
}

module_init(pll_init);
module_exit(pll_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("pll device driver");