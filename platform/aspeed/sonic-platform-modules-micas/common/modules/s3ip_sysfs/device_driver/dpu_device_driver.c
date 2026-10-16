/*
 * dpu_device_driver.c
 *
 * This module realize /sys/s3ip/dpu attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "dpu_sysfs.h"
#include "dfd_sysfs_common.h"

#define DPU_INFO(fmt, args...) LOG_INFO("dpu: ", fmt, ##args)
#define DPU_ERR(fmt, args...)  LOG_ERR("dpu: ", fmt, ##args)
#define DPU_DBG(fmt, args...)  LOG_DBG("dpu: ", fmt, ##args)

static int g_loglevel = 0xf;
static struct switch_drivers_s *g_drv = NULL;

/******************************************DPU***********************************************/
static int wb_get_dpu_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_dpu_number);

    ret = g_drv->get_dpu_number();
    return ret;
}

static int wb_get_dpu_fw_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_dpu_fw_number);

    ret = g_drv->get_dpu_fw_number();
    return ret;
}

static int wb_get_dpu_temp_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_dpu_temp_number);

    ret = g_drv->get_dpu_temp_number();
    return ret;
}

/*
 * wb_set_dpu_test_reg - Used to test dpu register write
 * @dpu_index: start with 1
 * @value: value write to dpu
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_dpu_attr(unsigned int dpu_index, unsigned int type, unsigned int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_dpu_attr);

    ret = g_drv->set_dpu_attr(dpu_index, type, value);
    return ret;
}

/*
 * wb_get_dpu_attr - Used to get the attr of dpu,
 * @dpu_index: start with 1
 * @type: get type
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_dpu_attr(unsigned int dpu_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_dpu_attr);

    ret = g_drv->get_dpu_attr(dpu_index, type, buf, count);
    return ret;
}

static ssize_t wb_get_dpu_fw_attr(unsigned int dpu_index, unsigned int fw_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_dpu_fw_attr);

    ret = g_drv->get_dpu_fw_attr(dpu_index, fw_index, type, buf, count);
    return ret;
}

static ssize_t wb_get_dpu_temp_attr(unsigned int dpu_index, unsigned int temp_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_dpu_temp_attr);

    ret = g_drv->get_dpu_temp_attr(dpu_index, temp_index, type, buf, count);
    return ret;
}


/***************************************end of DPU*******************************************/

static struct s3ip_sysfs_dpu_drivers_s drivers = {
    /*
     * set ODM DPU drivers to /sys/s3ip/dpu,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_dpu_number = wb_get_dpu_number,
    .get_dpu_fw_number = wb_get_dpu_fw_number,
    .get_dpu_temp_number = wb_get_dpu_temp_number,
    .set_dpu_attr = wb_set_dpu_attr,
    .get_dpu_attr = wb_get_dpu_attr,
    .get_dpu_fw_attr = wb_get_dpu_fw_attr,
    .get_dpu_temp_attr = wb_get_dpu_temp_attr,
};

static int __init dpu_device_driver_init(void)
{
    int ret;

    DPU_INFO("dpu_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_dpu_drivers_register(&drivers);
    if (ret < 0) {
        DPU_ERR("dpu drivers register err, ret %d.\n", ret);
        return ret;
    }

    DPU_INFO("dpu_init success.\n");
    return 0;
}

static void __exit dpu_device_driver_exit(void)
{
    s3ip_sysfs_dpu_drivers_unregister();
    DPU_INFO("dpu_exit success.\n");
    return;
}

module_init(dpu_device_driver_init);
module_exit(dpu_device_driver_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("dpu device driver");
