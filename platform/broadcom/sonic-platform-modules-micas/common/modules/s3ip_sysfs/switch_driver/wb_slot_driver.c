/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_slot_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * Subcard related attribute read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "./include/wb_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_adapter.h"
#include "./include/dfd_cfg_info.h"
#include "./include/dfd_frueeprom.h"

#define SLOT_SIZE                         (256)

int g_dfd_slot_dbg_level = 0;
module_param(g_dfd_slot_dbg_level, int, S_IRUGO | S_IWUSR);

static char *dfd_get_slot_sysfs_name(void)
{
    int key;
    char *sysfs_name;

    /* string Type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SLOT_SYSFS_NAME, 0, 0);
    sysfs_name = dfd_ko_cfg_get_item(key);
    if (sysfs_name == NULL) {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "key=0x%08x, sysfs_name is NULL, use default way.\n", key);
    } else {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "sysfs_name: %s.\n", sysfs_name);
    }
    return sysfs_name;
}

/**
 * dfd_get_slot_status - Gets the subcard status
 * @index: Number of the sub-card, starting from 1
 * return: 0:ABSENT
 *         1:OK
 *       : Negative value - Read failed
 */
static int dfd_get_slot_status(unsigned int slot_index)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_PRESENT_STATUS, WB_MAIN_DEV_SLOT, slot_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot status error, key:0x%x\n",key);
        return ret;
    }
    return status;
}

/**
 * dfd_get_slot_status_str - Gets the subcard status
 * @slot_index: Number of the sub-card, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_slot_status_str(unsigned int slot_index, char *buf, size_t count)
{
    int ret;
    if (buf == NULL) {
        DFD_SLOT_DEBUG(DBG_ERROR, "params error.slot_index:%d.",slot_index);
        return -DFD_RV_INVALID_VALUE;
    }
    ret = dfd_get_slot_status(slot_index);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot status error,ret:%d, slot_index:%d\n", ret, slot_index);
        return ret;
    }
    memset(buf, 0, count);
    return (ssize_t)snprintf(buf, count, "%d\n", ret);
}

/**
 * dfd_get_slot_info - Obtain the subcard information
 * @slot_index: Number of the sub-card, starting from 1
 * @cmd: Subcard information type, subcard name :2, subcard serial number :3, subcard hardware version number :5
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_slot_info(unsigned int slot_index, uint8_t cmd, char *buf, size_t count)
{
    int key, rv;
    char slot_buf[SLOT_SIZE];
    dfd_i2c_dev_t *i2c_dev;
    const char *sysfs_name;

    if (buf == NULL) {
        DFD_SLOT_DEBUG(DBG_ERROR, "buf is NULL, slot index:%d, cmd:%d\n", slot_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, count);
    memset(slot_buf, 0, SLOT_SIZE);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, WB_MAIN_DEV_SLOT, slot_index);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DFD_SLOT_DEBUG(DBG_ERROR, "slot i2c dev config error, key=0x%08x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    sysfs_name = dfd_get_slot_sysfs_name();
    rv = dfd_get_fru_board_data(i2c_dev->bus, i2c_dev->addr, cmd, slot_buf, SLOT_SIZE, sysfs_name);

    if (rv < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "slot eeprom read failed");
        return -DFD_RV_DEV_FAIL;
    }

    DFD_SLOT_DEBUG(DBG_VERBOSE, "%s\n", slot_buf);
    snprintf(buf, count, "%s\n", slot_buf);
    return strlen(buf);
}

ssize_t dfd_get_slot_power_status_str(unsigned int slot_index, char *buf, size_t count)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_POWER_STATUS, WB_MAIN_DEV_SLOT, slot_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot status error, key:0x%x\r\n",key);
        return ret;
    }
    memset(buf, 0, count);
    return (ssize_t)snprintf(buf, count, "%d\n", status);
}

int dfd_set_slot_power_status_str(unsigned int slot_index, int value)
{
    int key, ret;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_POWER_STATUS, WB_MAIN_DEV_SLOT, slot_index);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "set led status error, key:0x%x,ret:%d\r\n", key, ret);
        return ret;
    }
    return ret;
}
