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

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"
#include "dfd_frueeprom.h"
#include "dfd_wbtlv_eeprom.h"

#define SLOT_SIZE                         (256)

int g_dfd_slot_dbg_level = 0;
module_param(g_dfd_slot_dbg_level, int, S_IRUGO | S_IWUSR);

static char *dfd_get_slot_sysfs_name(void)
{
    uint64_t key;
    char *sysfs_name;

    /* string Type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SLOT_SYSFS_NAME, 0, 0);
    sysfs_name = dfd_ko_cfg_get_item(key);
    if (sysfs_name == NULL) {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "key_name=%s, sysfs_name is NULL, use default way.\n",
            key_to_name(DFD_CFG_ITEM_SLOT_SYSFS_NAME));
    } else {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "sysfs_name: %s.\n", sysfs_name);
    }
    return sysfs_name;
}

/**
 * dfd_get_slot_present - Gets the subcard present
 * @index: Number of the sub-card, starting from 1
 * return: 0: ABSENT
 *         1: PRESENT
 *        Negative value - Read failed
 */
static int dfd_get_slot_present(unsigned int slot_index)
{
    uint64_t key;
    int ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_PRESENT_STATUS, WB_MAIN_DEV_SLOT, slot_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot present error, key_name:%s\n",
            key_to_name(DFD_CFG_ITEM_DEV_PRESENT_STATUS));
        return ret;
    }
    return status;
}

/**
 * dfd_get_slot_present_str - Gets the subcard present
 * @slot_index: Number of the sub-card, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_slot_present_str(unsigned int slot_index, char *buf, size_t count)
{
    int ret;

    if (buf == NULL) {
        DFD_SLOT_DEBUG(DBG_ERROR, "params error.slot_index:%d.",slot_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %zu, slot index: %u\n",
            count, slot_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_slot_present(slot_index);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot present error,ret:%d, slot_index:%d\n", ret, slot_index);
        return ret;
    }
    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", ret);
}

/**
 * dfd_get_slot_power_status - Gets the subcard power status
 * @index: Number of the sub-card, starting from 1
 * return: 0:power off
 *         1:power on
 *         -DFD_RV_DEV_NOTSUPPORT: NOT SUPPORT
 *         Negative value - Read failed
 */
static int dfd_get_slot_power_status(unsigned int slot_index)
{
    uint64_t key;
    int ret;
    int status;

    /**
     * The 'SLOT_POWER_STATUS' config is a read-only register that indicates the power status of the slot;
     * the 'POWER_STATUS' config is a read-write register used to control the power states of the slot.
     */
    /* first get slot power status through slot_power_status config(read-only register) */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SLOT_POWER_STATUS, slot_index, 0);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret >= 0) {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "get slot[%u] power status success, key_name: %s, status: %d\n",
            slot_index, key_to_name(DFD_CFG_ITEM_SLOT_POWER_STATUS), status);
        return status;
    }
    /* get slot power status through slot_power_status config failed*/
    if (ret != -DFD_RV_DEV_NOTSUPPORT) {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "slot[%u] power status config not found, key_name: %s, ret: %d\n",
            slot_index, key_to_name(DFD_CFG_ITEM_SLOT_POWER_STATUS), ret);
        return ret;
    }
    /* not support get slot power status through slot_power_status config */
    /* try to get slot power status through power_status config(read-write register) */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_POWER_STATUS, WB_MAIN_DEV_SLOT, slot_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot[%u] power status failed, key_name: %s, ret: %d\n",
            slot_index, key_to_name(DFD_CFG_ITEM_POWER_STATUS), ret);
        return ret;
    }
    return status;
}

/**
 * dfd_get_slot_status - Gets the subcard status
 * @index: Number of the sub-card, starting from 1
 * return: 0:ABSENT
 *         1:OK
 *         2:NOT OK
 *       : Negative value - Read failed
 */
static int dfd_get_slot_status(unsigned int slot_index)
{
    int status;

    /* first get slot present */
    status = dfd_get_slot_present(slot_index);
    if (status != DEV_PRESENT) {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "slot index: %u, get present status: %d\n", slot_index, status);
        return status;
    }

    /* get slot power status */
    status = dfd_get_slot_power_status(slot_index);
    if (status < 0) {
        /*not support slot power status, but slot is present, return STATUS_OK */
        if (status == -DFD_RV_DEV_NOTSUPPORT) {
            DFD_SLOT_DEBUG(DBG_VERBOSE, "get slot[%u] status through slot present status\n", slot_index);
            return STATUS_OK;
        }
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot[%u] power status failed, ret: %d\n", slot_index, status);
        return status;
    }
    if (status == WB_SLOT_POWER_ON) {
        return STATUS_OK;
    }
    return STATUS_NOT_OK;
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
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %zu, slot index: %u\n",
            count, slot_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_slot_status(slot_index);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot status error,ret:%d, slot_index:%d\n", ret, slot_index);
        return ret;
    }
    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", ret);
}

static int dfd_get_slot_card_type_by_name(int bus, int addr, uint8_t *buf, uint32_t buf_len, const char *sysfs_name)
{
    int rv;
    int slot_card_type;

    /* get slot name */
    rv = dfd_get_fru_board_data(bus, addr, DFD_DEV_INFO_TYPE_NAME, buf, buf_len, sysfs_name);
    if (rv < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot card type by slot name failed, rv: %d\n", rv);
        return rv;
    }

    /* slot name converted to slot card type */
    rv = dfd_ko_cfg_get_slot_card_type_by_name(buf, &slot_card_type);
    if (rv < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot card type by slot name failed, Unsupport slot name: %s, rv: %d\n", buf, rv);
        return -DFD_RV_TYPE_ERR;
    }

    mem_clear(buf, buf_len);
    snprintf(buf, buf_len, "0x%x", slot_card_type);
    return DFD_RV_OK;
}

static int dfd_get_slot_card_type_by_fileid(int bus, int addr, int cmd, uint8_t *buf, uint32_t buf_len, const char *sysfs_name)
{
    int rv;

    rv = dfd_get_fru_board_data(bus, addr, cmd, buf, buf_len, sysfs_name);
    if (rv < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "read slot card type through fru file filed, rv: %d\n", rv);
        return rv;
    }
    /* Check fru file is started with 0x or 0X */
    if (strncmp(buf, "0x", 2) == 0 || strncmp(buf, "0X", 2) == 0) {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "read slot card type through fru file id success, slot card type: %s\n", buf);
        return DFD_RV_OK;
    }
    DFD_SLOT_DEBUG(DBG_ERROR, "read slot card type through fru file id failed, fru file id: %s\n", buf);
    return -DFD_RV_DEV_NOTSUPPORT;
}

static int dfd_get_slot_fru_info(int bus, int addr, int cmd, uint8_t *buf, uint32_t buf_len, const char *sysfs_name)
{
    int rv;

    /* not read slot card type */
    if (cmd != DFD_DEV_INFO_TYPE_DEV_TYPE) {
        rv = dfd_get_fru_board_data(bus, addr, cmd, buf, buf_len, sysfs_name);
        return rv;
    }

    /* read slot card type */
    DFD_SLOT_DEBUG(DBG_VERBOSE, "Try to get slot card type through fru file id\n");
    rv = dfd_get_slot_card_type_by_fileid(bus, addr, cmd, buf, buf_len, sysfs_name);
    if (rv == -DFD_RV_DEV_NOTSUPPORT) {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "Get slot card type through fru file id failed, try to get slot card type by name\n");
        rv = dfd_get_slot_card_type_by_name(bus, addr, buf, buf_len, sysfs_name);
    }
    if (rv < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "Get slot card type failed, ret: %d\n", rv);
    }
    return rv;
}

/**
 * dfd_get_slot_info - Obtain the subcard information
 * @slot_index: Number of the sub-card, starting from 1
 * @cmd: Subcard information type,
 *       slot name :2, serial number :3, hardware version :5, slot card type: 6
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_slot_info(unsigned int slot_index, uint8_t cmd, char *buf, size_t count)
{
    uint64_t key;
    int rv;
    char slot_buf[SLOT_SIZE];
    dfd_i2c_dev_t *i2c_dev;
    const char *sysfs_name;
    uint8_t e2_head[4];

    if (buf == NULL) {
        DFD_SLOT_DEBUG(DBG_ERROR, "buf is NULL, slot index:%d, cmd:%d\n", slot_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "buf size error, count: %zu, slot index: %u, cmd: 0x%x.\n",
            count, slot_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    mem_clear(slot_buf, SLOT_SIZE);
    mem_clear(e2_head, sizeof(e2_head));

    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, WB_MAIN_DEV_SLOT, slot_index);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DFD_SLOT_DEBUG(DBG_ERROR, "slot i2c dev config error, key_name=%s\n",
            key_to_name(DFD_CFG_ITEM_OTHER_I2C_DEV));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    sysfs_name = dfd_get_slot_sysfs_name();

    /* read eeprom head */
    rv = dfd_ko_i2c_read(i2c_dev->bus, i2c_dev->addr, 0, e2_head, sizeof(e2_head), sysfs_name);
    if (rv < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "read slot eeprom head failed, bus: %d, addr: 0x%x, rv: %d\n",
            i2c_dev->bus, i2c_dev->addr, rv);
        return -DFD_RV_DEV_FAIL;
    }
    /* wb tlv eeprom format */
    if (memcmp(e2_head, WB_TLV_EEPROM_HEAD, sizeof(e2_head)) == 0) {
        DFD_SLOT_DEBUG(DBG_VERBOSE, "slot[%u] match wb tlv eeprom format\n", slot_index);
        if (!dfd_wb_tlv_support_type(cmd)) {
            return -DFD_RV_DEV_NOTSUPPORT;
        }
        rv = dfd_wb_tlv_eeprom_read(i2c_dev->bus, i2c_dev->addr, cmd, slot_buf, SLOT_SIZE, sysfs_name);
    } else {    /* fru eeprom format */
        DFD_SLOT_DEBUG(DBG_VERBOSE, "parse slot[%u] eeprom through fru format\n", slot_index);
        rv = dfd_get_slot_fru_info(i2c_dev->bus, i2c_dev->addr, cmd, slot_buf, SLOT_SIZE, sysfs_name);
    }

    if (rv < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "slot eeprom read failed");
        return rv;
    }

    DFD_SLOT_DEBUG(DBG_VERBOSE, "%s\n", slot_buf);
    snprintf(buf, count, "%s\n", slot_buf);
    return strlen(buf);
}

ssize_t dfd_get_slot_power_ctrl_status_str(unsigned int slot_index, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_POWER_STATUS, WB_MAIN_DEV_SLOT, slot_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot status error, key_name: %s,ret:%d\r\n",
            key_to_name(DFD_CFG_ITEM_POWER_STATUS), ret);
        return ret;
    }
    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", status);
}

int dfd_set_slot_power_ctrl_status_str(unsigned int slot_index, int value)
{
    uint64_t key;
    int ret;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_POWER_STATUS, WB_MAIN_DEV_SLOT, slot_index);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "set slot status error, key_name: %s,ret:%d\r\n",
            key_to_name(DFD_CFG_ITEM_POWER_STATUS), ret);
        return ret;
    }
    return ret;
}
