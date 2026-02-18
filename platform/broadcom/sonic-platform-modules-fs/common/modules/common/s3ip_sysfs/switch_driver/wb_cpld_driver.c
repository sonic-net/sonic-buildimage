/*
 * Copyright(C) 2001-2012 whitebox. All rights reserved.
 */
/*
 * wb_cpld_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * CPLD related attribute read and write functions
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"
#include "wb_cpld_driver.h"

int g_dfd_cpld_dbg_level = 0;
module_param(g_dfd_cpld_dbg_level, int, S_IRUGO | S_IWUSR);

dfd_sysfs_func_map_t cpld_func_table[DFD_CPLD_MAX_E] = {
    [DFD_CPLD_NAME_E] = {dfd_get_cpld_name, NULL},
    [DFD_CPLD_TYPE_E] = {dfd_get_cpld_type, NULL},
    [DFD_CPLD_VENDOR_E] = {dfd_get_cpld_vendor, NULL},
    [DFD_CPLD_FW_VERSION_E] = {dfd_get_cpld_fw_version, NULL},
    [DFD_CPLD_HW_VERSION_E] = {dfd_get_cpld_hw_version, NULL},
    [DFD_CPLD_SUPPORT_UPGRADE_E] = {dfd_get_cpld_support_upgrade, NULL},
    [DFD_CPLD_UPGRADE_ACTIVE_TYPE_E] = {dfd_get_cpld_upgrade_active_type, NULL},
    [DFD_CPLD_REG_TEST_TYPE_E] = {dfd_get_cpld_testreg_str, dfd_set_cpld_testreg},
};

/**
 * dfd_get_cpld_name - Obtain the CPLD name
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_name(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    uint64_t key;
    char *cpld_name;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL. main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_NAME, main_dev_id, cpld_index);
    cpld_name = dfd_ko_cfg_get_item(key);
    if (cpld_name == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u name config error, key_name:%s\n",
            main_dev_id, cpld_index, key_to_name(DFD_CFG_ITEM_CPLD_NAME));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%s\n", cpld_name);
    snprintf(buf, count, "%s\n", cpld_name);
    return strlen(buf);
}

/**
 * dfd_get_cpld_type - Obtain the CPLD model
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_type(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    uint64_t key;
    char *cpld_type;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_TYPE, main_dev_id, cpld_index);
    cpld_type = dfd_ko_cfg_get_item(key);
    if (cpld_type == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u type config error, key_name: %s\n",
            main_dev_id, cpld_index, key_to_name(DFD_CFG_ITEM_CPLD_TYPE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%s\n", cpld_type);
    snprintf(buf, count, "%s\n", cpld_type);
    return strlen(buf);
}

/**
 * dfd_get_cpld_vendor - Obtain the CPLD vendor
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_vendor(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    uint64_t key;
    char *cpld_vendor;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_VENDOR, main_dev_id, cpld_index);
    cpld_vendor = dfd_ko_cfg_get_item(key);
    if (cpld_vendor == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u vendor config error, key_name: %s\n",
            main_dev_id, cpld_index, key_to_name(DFD_CFG_ITEM_CPLD_VENDOR));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%s\n", cpld_vendor);
    snprintf(buf, count, "%s\n", cpld_vendor);
    return strlen(buf);
}

/**
 * dfd_get_cpld_fw_version - Obtain the CPLD firmware version
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_fw_version(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    uint64_t key;
    uint32_t value;
    int rv;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_VERSION, main_dev_id, cpld_index);
    rv = dfd_info_get_int(key, &value, NULL);
    if (rv < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u fw config error, key_name: %s, ret: %d\n",
            main_dev_id, cpld_index, key_to_name(DFD_CFG_ITEM_CPLD_VERSION), rv);
        return rv;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "main_dev_id: %u, cpld%u firmware version: %x\n",
        main_dev_id, cpld_index, value);
    snprintf(buf, count, "%08x\n", value);
    return strlen(buf);
}

/**
 * dfd_get_cpld_hw_version - Obtain the hardware version of the CPLD
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_hw_version(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    uint64_t key;
    uint32_t value;
    int rv;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_HW_VERSION, main_dev_id, cpld_index);
    rv = dfd_info_get_int(key, &value, NULL);
    if (rv < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u fw config error, key_name: %s, ret: %d\n",
                       main_dev_id, cpld_index, key_to_name(DFD_CFG_ITEM_CPLD_HW_VERSION), rv);
        return rv;
    }
    DBG_CPLD_DEBUG(DBG_VERBOSE, "main_dev_id: %u, cpld%u hardware version 0x%x\n", main_dev_id, cpld_index, value);
    snprintf(buf, count, "%02x\n", value);
    return strlen(buf);
}

/**
 * dfd_set_cpld_testreg - Set the CPLD test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @cpld_index:The number of the CPLD starts from 0
 * @value: Writes the value of the test register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_cpld_testreg(unsigned int main_dev_id, unsigned int cpld_index, void *val, unsigned int len)
{
    uint64_t key;
    int ret;
    unsigned int *value;

    value = (unsigned int *)val;
    if (*value < 0 || *value > 0xff) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, can't set cpld%u test reg value = 0x%02x, len = %d\n",
            main_dev_id, cpld_index, *value, len);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_TEST_REG, main_dev_id, cpld_index);
    ret = dfd_info_set_int(key, *value);
    if (ret < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, set cpld%u test reg error, key_name: %s, ret:%d\n",
            main_dev_id, cpld_index, key_to_name(DFD_CFG_ITEM_CPLD_TEST_REG), ret);
        return ret;
    }
    return DFD_RV_OK;
}

/**
 * dfd_get_cpld_testreg - Read the CPLD test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @cpld_index: The number of the CPLD starts from 0
 * @value: Read the test register value
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_get_cpld_testreg(unsigned int main_dev_id, unsigned int cpld_index, int *value)
{
    uint64_t key;
    int ret;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_TEST_REG, main_dev_id, cpld_index);
    ret = dfd_info_get_int(key, value, NULL);
    if (ret < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, get cpld%u test reg error, key_name: %s, ret: %d\n",
            main_dev_id, cpld_index, key_to_name(DFD_CFG_ITEM_CPLD_TEST_REG), ret);
        return ret;
    }
    return DFD_RV_OK;
}

/**
 * dfd_get_cpld_testreg_str - Read the CPLD test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @cpld_index: The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_testreg_str(unsigned int main_dev_id, unsigned int cpld_index,
            char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    ret = dfd_get_cpld_testreg(main_dev_id, cpld_index, &value);
    if (ret < 0) {
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "0x%02x\n", value);
}

/**
 * dfd_get_cpld_support_upgrade - Obtain the CPLD support upgrade
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_support_upgrade(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    uint64_t key;
    int cpld_support_upgrade;
    int ret;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    cpld_support_upgrade = 0;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_SUPPORT_UPGRADE, main_dev_id, cpld_index);
    ret = dfd_info_get_int(key, &cpld_support_upgrade, NULL);
    if (ret < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u support upgrade config error, key_name: %s\n",
            main_dev_id, cpld_index, key_to_name(DFD_CFG_ITEM_CPLD_SUPPORT_UPGRADE));
        return ret;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%d\n", cpld_support_upgrade);
    snprintf(buf, count, "%d\n", cpld_support_upgrade);
    return strlen(buf);
}

/**
 * dfd_get_cpld_upgrade_active_type - Obtain the CPLD upgrade active type
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the CPLD starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cpld_upgrade_active_type(unsigned int main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    uint64_t key;
    char *cpld_upgrade_active_type;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_UPGRADE_ACTIVE_TYPE, main_dev_id, cpld_index);
    cpld_upgrade_active_type = dfd_ko_cfg_get_item(key);
    if (cpld_upgrade_active_type == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u upgrade active type config error, key_name: %s\n",
            main_dev_id, cpld_index, key_to_name(DFD_CFG_ITEM_CPLD_UPGRADE_ACTIVE_TYPE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%s\n", cpld_upgrade_active_type);
    snprintf(buf, count, "%s\n", cpld_upgrade_active_type);
    return strlen(buf);
}


