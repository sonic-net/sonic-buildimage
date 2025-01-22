/*
 * Copyright(C) 2001-2022 Ruijie Network. All rights reserved.
 */
/*
 * rg_cpld_driver.c
 * Original Author: sonic_rd@ruijie.com.cn 2020-02-17
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@ruijie.com.cn         2020-02-17          Initial version
 */

#include <linux/module.h>

#include "rg_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"

int g_dfd_cpld_dbg_level = 0;
module_param(g_dfd_cpld_dbg_level, int, S_IRUGO | S_IWUSR);

ssize_t dfd_get_cpld_name(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    int key;
    char *cpld_name;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL. main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_NAME, main_dev_id, cpld_index);
    cpld_name = dfd_ko_cfg_get_item(key);
    if (cpld_name == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u name config error, key: 0x%08x\n",
            main_dev_id, cpld_index, key);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%s\n", cpld_name);
    snprintf(buf, count, "%s\n", cpld_name);
    return strlen(buf);
}

ssize_t dfd_get_cpld_type(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    int key;
    char *cpld_type;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_TYPE, main_dev_id, cpld_index);
    cpld_type = dfd_ko_cfg_get_item(key);
    if (cpld_type == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u type config error, key: 0x%08x\n",
            main_dev_id, cpld_index, key);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%s\n", cpld_type);
    snprintf(buf, count, "%s\n", cpld_type);
    return strlen(buf);
}

ssize_t dfd_get_cpld_fw_version(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{
    int key, rv;
    uint32_t value;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_VERSION, main_dev_id, cpld_index);
    rv = dfd_info_get_int(key, &value, NULL);
    if (rv < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u fw config error, key: 0x%08x, ret: %d\n",
            main_dev_id, cpld_index, key, rv);
        if (rv == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
        return rv;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "main_dev_id: %u, cpld%u firmware version: %x\n",
        main_dev_id, cpld_index, value);
    snprintf(buf, count, "%08x\n", value);
    return strlen(buf);
}

ssize_t dfd_get_cpld_hw_version(uint8_t main_dev_id, unsigned int cpld_index, char *buf, size_t count)
{

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }
    DBG_CPLD_DEBUG(DBG_VERBOSE, "main_dev_id: %u, cpld%u hardware version not support\n",
        main_dev_id, cpld_index);
    return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
}

int dfd_set_cpld_testreg(uint8_t main_dev_id, unsigned int cpld_index, int value)
{
    int key, ret;

    if (value < 0 || value > 0xff) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, can't set cpld%u test reg value = 0x%02x\n",
            main_dev_id, cpld_index, value);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_TEST_REG, main_dev_id, cpld_index);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, set cpld%u test reg error, key: 0x%x, ret:%d\n",
            main_dev_id, cpld_index, key, ret);
        return ret;
    }
    return DFD_RV_OK;
}

int dfd_get_cpld_testreg(uint8_t main_dev_id, unsigned int cpld_index, int *value)
{
    int key, ret;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_TEST_REG, main_dev_id, cpld_index);
    ret = dfd_info_get_int(key, value, NULL);
    if (ret < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "main_dev_id: %u, get cpld%u test reg error, key: 0x%x, ret: %d\n",
            main_dev_id, cpld_index, key, ret);
        return ret;
    }
    return DFD_RV_OK;
}

ssize_t dfd_get_cpld_testreg_str(uint8_t main_dev_id, unsigned int cpld_index,
            char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, cpld index: %u\n",
            count, main_dev_id, cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, count);
    ret = dfd_get_cpld_testreg(main_dev_id, cpld_index, &value);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "0x%02x\n", value);
}
