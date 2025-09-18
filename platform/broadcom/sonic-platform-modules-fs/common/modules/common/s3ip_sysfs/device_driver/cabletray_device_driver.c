/*
 * cabletray_device_driver.c
 *
 * This module realize /sys/s3ip/cabletray attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "cabletray_sysfs.h"
#include "dfd_sysfs_common.h"

#define CABLETRAY_INFO(fmt, args...) LOG_INFO("cabletray: ", fmt, ##args)
#define CABLETRAY_ERR(fmt, args...)  LOG_ERR("cabletray: ", fmt, ##args)
#define CABLETRAY_DBG(fmt, args...)  LOG_DBG("cabletray: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/********************************************cabletray**********************************************/
static int wb_get_cabletray_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_cabletray_number);

    ret = g_drv->get_cabletray_number();
    return ret;
}

/*
 * wb_get_cabletray_name - Used to get cabletray name,
 * @cabletray_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_cabletray_name(unsigned int cabletray_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_cabletray_name);

    ret = g_drv->get_cabletray_name(cabletray_index, buf, count);
    return ret;
}


/*
 * wb_get_cabletray_alias - Used to identify the location of cabletray,
 * @cabletray_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_cabletray_alias(unsigned int cabletray_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_cabletray_alias);

    ret = g_drv->get_cabletray_alias(cabletray_index, buf, count);
    return ret;
}

/*
 * wb_get_cabletray_manufacturer - Used to get manufacturer,
 * @cabletray_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_cabletray_manufacturer(unsigned int cabletray_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_cabletray_manufacturer);

    ret = g_drv->get_cabletray_manufacturer(cabletray_index, buf, count);
    return ret;
}

/*
 * wb_get_cabletray_serial_number - Used to get cabletray serial number,
 * @cabletray_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_cabletray_serial_number(unsigned int cabletray_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_cabletray_serial_number);

    ret = g_drv->get_cabletray_serial_number(cabletray_index, buf, count);
    return ret;
}

/*
 * wb_get_cabletray_part_number - Used to get cabletray part number,
 * @cabletray_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_cabletray_part_number(unsigned int cabletray_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_cabletray_part_number);

    ret = g_drv->get_cabletray_part_number(cabletray_index, buf, count);
    return ret;
}

/*
 * wb_get_cabletray_version - Used to get cabletray hardware version,
 * @cabletray_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_cabletray_version(unsigned int cabletray_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_cabletray_version);

    ret = g_drv->get_cabletray_version(cabletray_index, buf, count);
    return ret;
}

/*
 * wb_get_cabletray_eeprom_size - Used to get cabletray eeprom size
 *
 * This function returns the size of port eeprom,
 * otherwise it returns a negative value on failed.
 */
 static int wb_get_cabletray_eeprom_size(unsigned int cabletray_index)
 {
     int ret;
 
     check_p(g_drv);
     check_p(g_drv->get_cabletray_eeprom_size);
 
     ret = g_drv->get_cabletray_eeprom_size(cabletray_index);
     return ret;
 }


/*
 * wb_read_cabletray_eeprom_data - Used to get cabletray hardware eeprom,
 * @cabletray_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_read_cabletray_eeprom_data(unsigned int cabletray_index, char *buf, loff_t offset, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->read_cabletray_eeprom_data);

    ret = g_drv->read_cabletray_eeprom_data(cabletray_index, buf, offset, count);
    return ret;
}

/*
 * wb_get_cabletray_slotid - Used to get cabletray hardware slotid,
 * @cabletray_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_cabletray_slotid(unsigned int cabletray_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_cabletray_slotid);

    ret = g_drv->get_cabletray_slotid(cabletray_index, buf, count);
    return ret;
}

/****************************************end of cabletray*******************************************/

static struct s3ip_sysfs_cabletray_drivers_s drivers = {
    /*
     * set ODM cabletray drivers to /sys/s3ip/cabletray,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_cabletray_number = wb_get_cabletray_number,
    .get_cabletray_name = wb_get_cabletray_name,
    .get_cabletray_alias = wb_get_cabletray_alias,
    .get_cabletray_manufacturer = wb_get_cabletray_manufacturer,
    .get_cabletray_serial_number = wb_get_cabletray_serial_number,
    .get_cabletray_part_number = wb_get_cabletray_part_number,
    .get_cabletray_version = wb_get_cabletray_version,
    .get_cabletray_slotid = wb_get_cabletray_slotid,
    .get_cabletray_eeprom_size  = wb_get_cabletray_eeprom_size,
    .read_cabletray_eeprom_data = wb_read_cabletray_eeprom_data,
};

static int __init cabletray_dev_drv_init(void)
{
    int ret;

    CABLETRAY_INFO("cabletray_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_cabletray_drivers_register(&drivers);
    if (ret < 0) {
        CABLETRAY_ERR("cabletray drivers register err, ret %d.\n", ret);
        return ret;
    }

    CABLETRAY_INFO("cabletray_init success.\n");
    return 0;
}

static void __exit cabletray_dev_drv_exit(void)
{
    s3ip_sysfs_cabletray_drivers_unregister();
    CABLETRAY_INFO("cabletray_exit success.\n");
    return;
}

module_init(cabletray_dev_drv_init);
module_exit(cabletray_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("cabletray device driver");
