/*
 * pcie_device_driver.c
 *
 * This module realize /sys/s3ip/pcie attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "pcie_sysfs.h"
#include "dfd_sysfs_common.h"

#define PCIE_INFO(fmt, args...) LOG_INFO("pcie: ", fmt, ##args)
#define PCIE_ERR(fmt, args...)  LOG_ERR("pcie: ", fmt, ##args)
#define PCIE_DBG(fmt, args...)  LOG_DBG("pcie: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_pcie_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_pcie_number);

    ret = g_drv->get_pcie_number();

    return ret;
}

/*****************************************pcie**********************************************/
/*
 * wb_get_sys_pcie_attr - Used to get sys pcie attr
 * @index: pcie index
 * @type: pcie attr type
 * @buf: pcie attr buffer, NA for not support
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_sys_pcie_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_pcie_attr);

    ret = g_drv->get_pcie_attr(index, type, buf, count);
    return ret;
}

/*
 * wb_set_debug_pcie_attr - Used to set the debug attr of PCIe
 * @type: threshold type
 * @pcie_index: start with 1
 * @value: the value to be set
 *
 * This function returns 0 on success, otherwise it returns a negative value on failed.
 */
static ssize_t wb_set_debug_pcie_attr(unsigned int type, unsigned int pcie_index, const char *value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_debug_pcie_attr);

    ret = g_drv->set_debug_pcie_attr(type, pcie_index, value);
    return ret;
}

/**************************************end of pcie******************************************/

static struct s3ip_sysfs_pcie_drivers_s drivers = {
    /*
     * set ODM pcie drivers to /sys/s3ip/pcie,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_pcie_number = wb_get_pcie_number,
    .get_pcie_attr = wb_get_sys_pcie_attr,
    .set_debug_pcie_attr = wb_set_debug_pcie_attr,
};

static int __init pcie_init(void)
{
    int ret;

    PCIE_INFO("pcie_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_pcie_drivers_register(&drivers);
    if (ret < 0) {
        PCIE_ERR("pcie drivers register err, ret %d.\n", ret);
        return ret;
    }

    PCIE_INFO("pcie create success.\n");
    return 0;
}

static void __exit pcie_exit(void)
{
    s3ip_sysfs_pcie_drivers_unregister();
    PCIE_INFO("pcie_exit ok.\n");
    return;
}

module_init(pcie_init);
module_exit(pcie_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("pcie device driver");