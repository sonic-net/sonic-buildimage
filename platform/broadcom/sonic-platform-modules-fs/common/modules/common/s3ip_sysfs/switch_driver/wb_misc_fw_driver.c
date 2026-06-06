/*
 * Copyright(C) 2001-2012 whitebox. All rights reserved.
 */
/*
 * wb_misc_fw_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * MISC_FW related attribute read and write functions
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"
#include "wb_misc_fw_driver.h"

int g_dfd_misc_fw_dbg_level = 0;
module_param(g_dfd_misc_fw_dbg_level, int, S_IRUGO | S_IWUSR);

dfd_sysfs_func_map_t misc_fw_func_table[DFD_MISC_FW_MAX_E] = {
    [DFD_MISC_FW_NAME_E] = {dfd_get_misc_fw_name, NULL},
    [DFD_MISC_FW_TYPE_E] = {dfd_get_misc_fw_type, NULL},
    [DFD_MISC_FW_VENDOR_E] = {dfd_get_misc_fw_vendor, NULL},
    [DFD_MISC_FW_FW_VERSION_E] = {dfd_get_misc_fw_fw_version, NULL},
    [DFD_MISC_FW_HW_VERSION_E] = {dfd_get_misc_fw_hw_version, NULL},
    [DFD_MISC_FW_SUPPORT_UPGRADE_E] = {dfd_get_misc_fw_support_upgrade, NULL},
    [DFD_MISC_FW_UPGRADE_ACTIVE_TYPE_E] = {dfd_get_misc_fw_upgrade_active_type, NULL},
};

/**
 * dfd_get_misc_fw_name - Obtain the MISC_FW name
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_name(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count)
{
    uint64_t key;
    char *misc_fw_name;

    if (buf == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "param error, buf is NULL. main_dev_id: %u, misc_fw index: %u\n",
            main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, misc_fw index: %u\n",
            count, main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_MISC_FW_NAME, main_dev_id, misc_fw_index);
    misc_fw_name = dfd_ko_cfg_get_item(key);
    if (misc_fw_name == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "main_dev_id: %u, misc_fw%u name config error, key_name:%s\n",
            main_dev_id, misc_fw_index, key_to_name(DFD_CFG_ITEM_MISC_FW_NAME));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_MISC_FW_DEBUG(DBG_VERBOSE, "%s\n", misc_fw_name);
    snprintf(buf, count, "%s\n", misc_fw_name);
    return strlen(buf);
}

/**
 * dfd_get_misc_fw_type - Obtain the MISC_FW model
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_type(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count)
{
    uint64_t key;
    char *misc_fw_type;

    if (buf == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, misc_fw index: %u\n",
            main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, misc_fw index: %u\n",
            count, main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_MISC_FW_TYPE, main_dev_id, misc_fw_index);
    misc_fw_type = dfd_ko_cfg_get_item(key);
    if (misc_fw_type == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "main_dev_id: %u, misc_fw%u type config error, key_name: %s\n",
            main_dev_id, misc_fw_index, key_to_name(DFD_CFG_ITEM_MISC_FW_TYPE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_MISC_FW_DEBUG(DBG_VERBOSE, "%s\n", misc_fw_type);
    snprintf(buf, count, "%s\n", misc_fw_type);
    return strlen(buf);
}

/**
 * dfd_get_misc_fw_vendor - Obtain the MISC_FW vendor
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_vendor(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count)
{
    uint64_t key;
    char *misc_fw_vendor;

    if (buf == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, misc_fw index: %u\n",
            main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, misc_fw index: %u\n",
            count, main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_MISC_FW_VENDOR, main_dev_id, misc_fw_index);
    misc_fw_vendor = dfd_ko_cfg_get_item(key);
    if (misc_fw_vendor == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "main_dev_id: %u, misc_fw%u vendor config error, key_name: %s\n",
            main_dev_id, misc_fw_index, key_to_name(DFD_CFG_ITEM_MISC_FW_VENDOR));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_MISC_FW_DEBUG(DBG_VERBOSE, "%s\n", misc_fw_vendor);
    snprintf(buf, count, "%s\n", misc_fw_vendor);
    return strlen(buf);
}

/**
 * dfd_get_misc_fw_fw_version - Obtain the MISC_FW firmware version
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_fw_version(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count)
{
    uint64_t key;
    uint32_t value;
    int rv;

    if (buf == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, misc_fw index: %u\n",
            main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, misc_fw index: %u\n",
            count, main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_MISC_FW_VERSION, main_dev_id, misc_fw_index);
    rv = dfd_info_get_int(key, &value, NULL);
    if (rv < 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "main_dev_id: %u, misc_fw%u fw config error, key_name: %s, ret: %d\n",
            main_dev_id, misc_fw_index, key_to_name(DFD_CFG_ITEM_MISC_FW_VERSION), rv);
        return rv;
    }

    DBG_MISC_FW_DEBUG(DBG_VERBOSE, "main_dev_id: %u, misc_fw%u firmware version: %x\n",
        main_dev_id, misc_fw_index, value);
    snprintf(buf, count, "%08x\n", value);
    return strlen(buf);
}

/**
 * dfd_get_misc_fw_hw_version - Obtain the hardware version of the MISC_FW
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_hw_version(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count)
{
    uint64_t key;
    uint32_t value;
    int rv;

    if (buf == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, misc_fw index: %u\n",
            main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, misc_fw index: %u\n",
            count, main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_MISC_FW_HW_VERSION, main_dev_id, misc_fw_index);
    rv = dfd_info_get_int(key, &value, NULL);
    if (rv < 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "main_dev_id: %u, misc_fw%u fw config error, key_name: %s, ret: %d\n",
                       main_dev_id, misc_fw_index, key_to_name(DFD_CFG_ITEM_MISC_FW_HW_VERSION), rv);
        return rv;
    }
    DBG_MISC_FW_DEBUG(DBG_VERBOSE, "main_dev_id: %u, misc_fw%u hardware version 0x%x\n", main_dev_id, misc_fw_index, value);
    snprintf(buf, count, "%02x\n", value);
    return strlen(buf);
}


/**
 * dfd_get_misc_fw_support_upgrade - Obtain the MISC_FW support upgrade
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_support_upgrade(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count)
{
    uint64_t key;
    int misc_fw_support_upgrade;
    int ret;

    if (buf == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, misc_fw index: %u\n",
            main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, misc_fw index: %u\n",
            count, main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    misc_fw_support_upgrade = 0;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_MISC_FW_SUPPORT_UPGRADE, main_dev_id, misc_fw_index);
    ret = dfd_info_get_int(key, &misc_fw_support_upgrade, NULL);
    if (ret < 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "main_dev_id: %u, misc_fw%u support upgrade config error, key_name: %s\n",
            main_dev_id, misc_fw_index, key_to_name(DFD_CFG_ITEM_MISC_FW_SUPPORT_UPGRADE));
        return ret;
    }

    DBG_MISC_FW_DEBUG(DBG_VERBOSE, "%d\n", misc_fw_support_upgrade);
    snprintf(buf, count, "%d\n", misc_fw_support_upgrade);
    return strlen(buf);
}

/**
 * dfd_get_misc_fw_upgrade_active_type - Obtain the MISC_FW upgrade active type
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the MISC_FW starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_misc_fw_upgrade_active_type(unsigned int main_dev_id, unsigned int misc_fw_index, char *buf, size_t count)
{
    uint64_t key;
    char *misc_fw_upgrade_active_type;

    if (buf == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, misc_fw index: %u\n",
            main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, misc_fw index: %u\n",
            count, main_dev_id, misc_fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_MISC_FW_UPGRADE_ACTIVE_TYPE, main_dev_id, misc_fw_index);
    misc_fw_upgrade_active_type = dfd_ko_cfg_get_item(key);
    if (misc_fw_upgrade_active_type == NULL) {
        DBG_MISC_FW_DEBUG(DBG_ERROR, "main_dev_id: %u, misc_fw%u upgrade active type config error, key_name: %s\n",
            main_dev_id, misc_fw_index, key_to_name(DFD_CFG_ITEM_MISC_FW_UPGRADE_ACTIVE_TYPE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_MISC_FW_DEBUG(DBG_VERBOSE, "%s\n", misc_fw_upgrade_active_type);
    snprintf(buf, count, "%s\n", misc_fw_upgrade_active_type);
    return strlen(buf);
}


