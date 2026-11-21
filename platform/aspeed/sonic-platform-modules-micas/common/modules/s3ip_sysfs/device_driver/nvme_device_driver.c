/*
 * nvme_device_driver.c
 *
 * This module realize /sys/s3ip/nvme attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "nvme_sysfs.h"
#include "dfd_sysfs_common.h"

#define NVME_INFO(fmt, args...) LOG_INFO("nvme: ", fmt, ##args)
#define NVME_ERR(fmt, args...)  LOG_ERR("nvme: ", fmt, ##args)
#define NVME_DBG(fmt, args...)  LOG_DBG("nvme: ", fmt, ##args)

static int g_loglevel = 0x0;
static struct switch_drivers_s *g_drv = NULL;

static int wb_get_nvme_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_nvme_number);

    ret = g_drv->get_nvme_number();

    return ret;
}

/*****************************************nvme**********************************************/
/*
 * wb_get_sys_nvme_attr - Used to get sys nvme attr
 * @index: nvme index
 * @type: nvme attr type
 * @buf: nvme attr buffer, NA for not support
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_sys_nvme_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_nvme_attr);

    ret = g_drv->get_nvme_attr(index, type, buf, count);
    return ret;
}

static int wb_set_sys_nvme_attr(unsigned int index, unsigned int type, unsigned int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_nvme_attr);

    ret = g_drv->set_nvme_attr(index, type, status);
    return ret;
}

/*
 * wb_get_sys_nvme_temp_attr - Used to get sys nvme temp
 * @nvme_index: nvme index
 * @temp_index: temp index
 * @type: nvme attr type
 * @buf: nvme attr buffer
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_sys_nvme_temp_attr(unsigned int nvme_index, unsigned int temp_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_nvme_temp_attr);

    ret = g_drv->get_nvme_temp_attr(nvme_index, temp_index, type, buf, count);
    return ret;
}

/*
 * wb_get_sys_nvme_power_attr - Used to get sys nvme power
 * @nvme_index: nvme index
 * @power_index: power index
 * @type: nvme attr type
 * @buf: nvme attr buffer
 * @count: length of the buf
 *
 * Return: >= 0 for success, < 0 for fail
 */
static ssize_t wb_get_sys_nvme_power_attr(unsigned int nvme_index, unsigned int power_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_nvme_power_attr);

    ret = g_drv->get_nvme_power_attr(nvme_index, power_index, type, buf, count);
    return ret;
}

/*
 * wb_get_nvme_temp_number - Used to get nvme temp number
 *
 * Return: >= 0 for success, < 0 for fail
 */
static int wb_get_nvme_temp_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_nvme_temp_number);

    ret = g_drv->get_nvme_temp_number();
    return ret;
}

/*
 * wb_get_nvme_power_number - Used to get nvme power number
 *
 * Return: >= 0 for success, < 0 for fail
*/
static int wb_get_nvme_power_number(void)
{
    int ret;
 
    check_p(g_drv);
    check_p(g_drv->get_nvme_power_number);
 
    ret = g_drv->get_nvme_power_number();
    return ret;
}
/**************************************end of nvme******************************************/

static struct s3ip_sysfs_nvme_drivers_s drivers = {
    /*
     * set ODM nvme drivers to /sys/s3ip/nvme,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_nvme_number = wb_get_nvme_number,
    .get_nvme_attr = wb_get_sys_nvme_attr,
    .set_nvme_attr = wb_set_sys_nvme_attr,
    .get_nvme_temp_attr = wb_get_sys_nvme_temp_attr,
    .get_nvme_power_attr = wb_get_sys_nvme_power_attr,
    .get_nvme_temp_number = wb_get_nvme_temp_number,
    .get_nvme_power_number = wb_get_nvme_power_number,
};

static int __init nvme_init(void)
{
    int ret;

    NVME_INFO("nvme_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_nvme_drivers_register(&drivers);
    if (ret < 0) {
        NVME_ERR("nvme drivers register err, ret %d.\n", ret);
        return ret;
    }

    NVME_INFO("nvme create success.\n");
    return 0;
}

static void __exit nvme_exit(void)
{
    s3ip_sysfs_nvme_drivers_unregister();
    NVME_INFO("nvme_exit ok.\n");
    return;
}

module_init(nvme_init);
module_exit(nvme_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("nvme device driver");