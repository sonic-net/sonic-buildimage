/*
 * Copyright(C) 2001-2022 Ruijie Network. All rights reserved.
 */
/*
 * rg_eeprom_driver.c
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
#include "dfd_tlveeprom.h"

int g_dfd_eeprom_dbg_level = 0;
module_param(g_dfd_eeprom_dbg_level, int, S_IRUGO | S_IWUSR);

int dfd_get_eeprom_size(int e2_type, int index)
{
    int key;
    int *p_eeprom_size;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_SIZE, e2_type, index);

    p_eeprom_size = dfd_ko_cfg_get_item(key);
    if (p_eeprom_size == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom size error. key:0x%x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return *p_eeprom_size;
}

ssize_t dfd_read_eeprom_data(int e2_type, int index, char *buf, loff_t offset, size_t count)
{
    int key;
    ssize_t rd_len;
    char *eeprom_path;

    if (buf == NULL || offset < 0 || count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "params error, offset: 0x%llx, rd_count: %lu.\n",
            offset, count);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_PATH, e2_type, index);
    eeprom_path = dfd_ko_cfg_get_item(key);
    if (eeprom_path == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom path error, e2_type: %d, index: %d, key: 0x%08x\n",
            e2_type, index, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "e2_type: %d, index: %d, path: %s, offset: 0x%llx, \
        rd_count: %lu\n", e2_type, index, eeprom_path, offset, count);

    memset(buf, 0, count);
    rd_len = dfd_ko_read_file(eeprom_path, offset, buf, count);
    if (rd_len < 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "read eeprom data failed, loc: %s, offset: 0x%llx, \
        rd_count: %lu, ret: %ld,\n", eeprom_path, offset, count, rd_len);
    } else {
        DBG_EEPROM_DEBUG(DBG_VERBOSE, "read eeprom data success, loc: %s, offset: 0x%llx, \
            rd_count: %lu, rd_len: %ld,\n", eeprom_path, offset, count, rd_len);
    }

    return rd_len;
}

ssize_t dfd_write_eeprom_data(int e2_type, int index, char *buf, loff_t offset, size_t count)
{
    int key;
    ssize_t wr_len;
    char *eeprom_path;

    if (buf == NULL || offset < 0 || count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "params error, offset: 0x%llx, count: %lu.\n", offset, count);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_PATH, e2_type, index);
    eeprom_path = dfd_ko_cfg_get_item(key);
    if (eeprom_path == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom path error, e2_type: %d, index: %d, key: 0x%08x\n",
            e2_type, index, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "e2_type: %d, index: %d, path: %s, offset: 0x%llx, \
        wr_count: %lu.\n", e2_type, index, eeprom_path, offset, count);

    wr_len = dfd_ko_write_file(eeprom_path, offset, buf, count);
    if (wr_len < 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "write eeprom data failed, loc:%s, offset: 0x%llx, \
            wr_count: %lu, ret: %ld.\n", eeprom_path, offset, count, wr_len);
    } else {
        DBG_EEPROM_DEBUG(DBG_VERBOSE, "write eeprom data success, loc:%s, offset: 0x%llx, \
            wr_count: %lu, wr_len: %ld.\n", eeprom_path, offset, count, wr_len);
    }

    return wr_len;
}