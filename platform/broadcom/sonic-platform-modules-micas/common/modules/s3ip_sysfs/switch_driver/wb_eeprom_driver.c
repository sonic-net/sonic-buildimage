/*
 * An wb_eeprom_driver driver for eeprom devcie function
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_tlveeprom.h"

int g_dfd_eeprom_dbg_level = 0;
module_param(g_dfd_eeprom_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * dfd_get_eeprom_size - Gets the data size of the eeprom
 * @e2_type: This section describes the E2 type, including system, PSU, fan, and module E2
 * @index: E2 number
 * return: Succeeded: The data size of the eeprom is returned
 *       : Failed: A negative value is returned
 */
int dfd_get_eeprom_size(int e2_type, int index)
{
    uint64_t key;
    int *p_eeprom_size;

    /* Obtain the eeprom size */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_SIZE, e2_type, index);

    p_eeprom_size = dfd_ko_cfg_get_item(key);
    if (p_eeprom_size == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom size error. key_name:%s\n", 
            key_to_name(DFD_CFG_ITEM_EEPROM_SIZE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return *p_eeprom_size;
}

/**
 * dfd_read_eeprom_data - Read eeprom data
 * @e2_type: This section describes the E2 type, including system, PSU, fan, and module E2
 * @index: E2 number
 * @buf: eeprom data receives buf
 * @offset: The offset address of the read
 * @count: Read length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_read_eeprom_data(int e2_type, int index, char *buf, loff_t offset, size_t count)
{
    uint64_t key;
    ssize_t rd_len;
    char *eeprom_path;

    if (buf == NULL || offset < 0 || count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "params error, offset: 0x%llx, rd_count: %lu.\n",
            offset, count);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Obtain the eeprom read path*/
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_PATH, e2_type, index);
    eeprom_path = dfd_ko_cfg_get_item(key);
    if (eeprom_path == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom path error, e2_type: %d, index: %d, key_name: %s\n",
            e2_type, index, key_to_name(DFD_CFG_ITEM_EEPROM_PATH));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "e2_type: %d, index: %d, path: %s, offset: 0x%llx, \
        rd_count: %lu\n", e2_type, index, eeprom_path, offset, count);

    mem_clear(buf, count);
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

/**
 * dfd_write_eeprom_data - Write eeprom data
 * @e2_type: This section describes the E2 type, including system, PSU, fan, and module E2
 * @index: E2 number
 * @buf: eeprom data buf
 * @offset: The offset address of the write
 * @count: Write length
 * return: Success: The length of the written data is returned
 *       : Failed: A negative value is returned
 */
ssize_t dfd_write_eeprom_data(int e2_type, int index, char *buf, loff_t offset, size_t count)
{
    uint64_t key;
    ssize_t wr_len;
    char *eeprom_path;

    if (buf == NULL || offset < 0 || count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "params error, offset: 0x%llx, count: %lu.\n", offset, count);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Obtain the eeprom read path */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_PATH, e2_type, index);
    eeprom_path = dfd_ko_cfg_get_item(key);
    if (eeprom_path == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom path error, e2_type: %d, index: %d, key_name: %s\n",
            e2_type, index, key_to_name(DFD_CFG_ITEM_EEPROM_PATH));
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

ssize_t dfd_get_eeprom_alias(int e2_type, unsigned int e2_index, char *buf, size_t count)
{
    uint64_t key;
    char *e2_alias;

    if (buf == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "buf size error, count: %lu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_ALIAS, e2_type, e2_index);
    e2_alias = dfd_ko_cfg_get_item(key);
    if (e2_alias == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom alias config error, e2_type: %d, e2_index: %u, key_name: %s\n",
            e2_type, e2_index, key_to_name(DFD_CFG_ITEM_EEPROM_ALIAS));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "%s\n", e2_alias);
    snprintf(buf, count, "%s\n", e2_alias);
    return strlen(buf);
}

ssize_t dfd_get_eeprom_tag(int e2_type, unsigned int e2_index, char *buf, size_t count)
{
    uint64_t key;
    char *e2_tag;

    if (buf == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "buf size error, count: %lu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_TAG, e2_type, e2_index);
    e2_tag = dfd_ko_cfg_get_item(key);
    if (e2_tag == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom tag config error, e2_type: %d, e2_index: %u, key: %s\n",
            e2_type, e2_index, key_to_name(DFD_CFG_ITEM_EEPROM_TAG));
         return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "%s\n", e2_tag);
    snprintf(buf, count, "%s\n", e2_tag);
    return strlen(buf);
}

ssize_t dfd_get_eeprom_type(int e2_type, unsigned int e2_index, char *buf, size_t count)
{
    uint64_t key;
    char *eeprom_type;

    if (buf == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "buf size error, count: %lu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_TYPE, e2_type, e2_index);
    eeprom_type = dfd_ko_cfg_get_item(key);
    if (eeprom_type == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom type config error, e2_type: %d, e2_index: %u, key_name: %s\n",
            e2_type, e2_index, key_to_name(DFD_CFG_ITEM_EEPROM_TYPE));
         return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "%s\n", eeprom_type);
    snprintf(buf, count, "%s\n", eeprom_type);
    return strlen(buf);
}
