/*
 * fpga_device_driver.c
 *
 * This module realize /sys/s3ip/fpga attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "fpga_sysfs.h"
#include "dfd_sysfs_common.h"

#define FPGA_INFO(fmt, args...) LOG_INFO("fpga: ", fmt, ##args)
#define FPGA_ERR(fmt, args...)  LOG_ERR("fpga: ", fmt, ##args)
#define FPGA_DBG(fmt, args...)  LOG_DBG("fpga: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/******************************************FPGA***********************************************/
static int rg_get_main_board_fpga_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_fpga_number);

    ret = g_drv->get_main_board_fpga_number();
    return ret;
}

/*
 * rg_get_main_board_fpga_alias - Used to identify the location of fpga,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t rg_get_main_board_fpga_alias(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_fpga_alias);

    ret = g_drv->get_main_board_fpga_alias(fpga_index, buf, count);
    return ret;
}

/*
 * rg_get_main_board_fpga_type - Used to get fpga model name
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t rg_get_main_board_fpga_type(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_fpga_type);

    ret = g_drv->get_main_board_fpga_type(fpga_index, buf, count);
    return ret;
}

/*
 * rg_get_main_board_fpga_firmware_version - Used to get fpga firmware version,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t rg_get_main_board_fpga_firmware_version(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_fpga_firmware_version);

    ret = g_drv->get_main_board_fpga_firmware_version(fpga_index, buf, count);
    return ret;
}

/*
 * rg_get_main_board_fpga_board_version - Used to get fpga board version,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t rg_get_main_board_fpga_board_version(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_fpga_board_version);

    ret = g_drv->get_main_board_fpga_board_version(fpga_index, buf, count);
    return ret;
}

/*
 * rg_get_main_board_fpga_test_reg - Used to test fpga register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t rg_get_main_board_fpga_test_reg(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_main_board_fpga_test_reg);

    ret = g_drv->get_main_board_fpga_test_reg(fpga_index, buf, count);
    return ret;
}

/*
 * rg_set_main_board_fpga_test_reg - Used to test fpga register write
 * @fpga_index: start with 1
 * @value: value write to fpga
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int rg_set_main_board_fpga_test_reg(unsigned int fpga_index, unsigned int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_main_board_fpga_test_reg);

    ret = g_drv->set_main_board_fpga_test_reg(fpga_index, value);
    return ret;
}
/***************************************end of FPGA*******************************************/

static struct s3ip_sysfs_fpga_drivers_s drivers = {
    /*
     * set ODM FPGA drivers to /sys/s3ip/fpga,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_fpga_number = rg_get_main_board_fpga_number,
    .get_main_board_fpga_alias = rg_get_main_board_fpga_alias,
    .get_main_board_fpga_type = rg_get_main_board_fpga_type,
    .get_main_board_fpga_firmware_version = rg_get_main_board_fpga_firmware_version,
    .get_main_board_fpga_board_version = rg_get_main_board_fpga_board_version,
    .get_main_board_fpga_test_reg = rg_get_main_board_fpga_test_reg,
    .set_main_board_fpga_test_reg = rg_set_main_board_fpga_test_reg,
};

static int __init fpga_dev_drv_init(void)
{
    int ret;

    FPGA_INFO("fpga_init...\n");
    g_drv = switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_fpga_drivers_register(&drivers);
    if (ret < 0) {
        FPGA_ERR("fpga drivers register err, ret %d.\n", ret);
        return ret;
    }
    FPGA_INFO("fpga_init success.\n");
    return 0;
}

static void __exit fpga_dev_drv_exit(void)
{
    s3ip_sysfs_fpga_drivers_unregister();
    FPGA_INFO("fpga_exit success.\n");
    return;
}

module_init(fpga_dev_drv_init);
module_exit(fpga_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("fpga device driver");
