/*
 * Copyright(C) 2001-2012 Ruijie Network. All rights reserved.
 */

#include <linux/module.h>

#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_adapter.h"
#include "./include/dfd_cfg_info.h"
#include "./include/dfd_frueeprom.h"

int g_dfd_cpld_dbg_level = 0;
module_param(g_dfd_cpld_dbg_level, int, S_IRUGO | S_IWUSR);

ssize_t dfd_get_cpld_name(unsigned int cpld_index, char *buf)
{
    int key;
    char *cpld_name;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error. buf is NULL.cpld index:%d", cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, PAGE_SIZE);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_NAME, cpld_index, 0);
    cpld_name = dfd_ko_cfg_get_item(key);
    if (cpld_name == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "cpld name config error, key=0x%08x\n", key);
        return -DFD_RV_NODE_FAIL;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%s\n", cpld_name);
    snprintf(buf, PAGE_SIZE, "%s\n", cpld_name);
    return strlen(buf);
}

ssize_t dfd_get_cpld_type(unsigned int cpld_index, char *buf)
{
    int key;
    char *cpld_type;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error. buf is NULL.cpld index:%d\n", cpld_index);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, PAGE_SIZE);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_TYPE, cpld_index, 0);
    cpld_type = dfd_ko_cfg_get_item(key);
    if (cpld_type == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "cpld type config error, key=0x%08x\n", key);
        return -DFD_RV_NODE_FAIL;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%s\n", cpld_type);
    snprintf(buf, PAGE_SIZE, "%s\n", cpld_type);
    return strlen(buf);
}

ssize_t dfd_get_cpld_version(unsigned int cpld_index, char *buf)
{
    int key, rv;
    uint32_t value;

    if (buf == NULL) {
        DBG_CPLD_DEBUG(DBG_ERROR, "param error. buf is NULL.");
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, PAGE_SIZE);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_VERSION, cpld_index, 0);
    rv = dfd_info_get_int(key, &value, NULL);
    if (rv < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "cpld version config error, key=0x%08x\n", key);
        return -DFD_RV_NODE_FAIL;
    }

    DBG_CPLD_DEBUG(DBG_VERBOSE, "%x\n", value);
    snprintf(buf, PAGE_SIZE, "%08x\n", value);
    return strlen(buf);
}

int dfd_set_cpld_testreg(unsigned int cpld_index, int value)
{
    int key, ret;

    if (value < 0 || value > 0xff) {
        DBG_CPLD_DEBUG(DBG_ERROR, "can not set cpld test register value = 0x%02x.\n", value);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_TEST_REG, cpld_index, 0);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "set cpld test register error, key:0x%x,ret:%d\n",key, ret);
        return ret;
    }
    return DFD_RV_OK;
}

int dfd_get_cpld_testreg(unsigned int cpld_index, int *value)
{
    int key, ret;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_TEST_REG, cpld_index, 0);
    ret = dfd_info_get_int(key, value, NULL);
    if (ret < 0) {
        DBG_CPLD_DEBUG(DBG_ERROR, "get cpld test register error, key:0x%x,ret:%d\n",key, ret);
        return ret;
    }
    return DFD_RV_OK;
}
