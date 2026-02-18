/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_eeprom_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * eeprom related attribute read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_tlveeprom.h"
#include "wb_eeprom_driver.h"
#include "dfd_cfg_info.h"

int g_dfd_eeprom_dbg_level = 0;
module_param(g_dfd_eeprom_dbg_level, int, S_IRUGO | S_IWUSR);

dfd_sysfs_func_map_t eeprom_func_table[DFD_EEPROM_MAX_E] = {
    [DFD_EEPROM_ALIAS_E] = {dfd_get_eeprom_alias, NULL},
    [DFD_EEPROM_TAG_E] = {dfd_get_eeprom_tag, NULL},
    [DFD_EEPROM_TYPE_E] = {dfd_get_eeprom_type, NULL},
    [DFD_EEPROM_WRITE_PROTECTION_E] = {dfd_get_eeprom_wp_reg_str, dfd_set_eeprom_write_protection},
    [DFD_EEPROM_I2C_BSU_E] = {dfd_get_eeprom_bus, NULL},
    [DFD_EEPROM_I2C_ADDR_E] = {dfd_get_eeprom_addr, NULL},
};

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
        if (e2_type == WB_MAIN_DEV_SFF) {
            DBG_EEPROM_DEBUG(DBG_VERBOSE, "sff e2 user default size, e2_type: %d, index: %d, default_size: 0x%x\n", 
                e2_type, index, SFF_E2_DEFAULT_SIZE);
            return SFF_E2_DEFAULT_SIZE;
        }
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom size error. e2_type: %d, e2_index: %d, key_name:%s\n", 
            e2_type, index, key_to_name(DFD_CFG_ITEM_EEPROM_SIZE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "get e2 size success, e2_type: %d, index: %d, size: 0x%x\n", 
                e2_type, index, *p_eeprom_size);
    return *p_eeprom_size;
}

static int get_eeprom_path_by_bus(int e2_type, int index, char *eeprom_path, size_t count) 
{
    uint64_t key;
    int *eeprom_bus;

    if (e2_type == WB_MAIN_DEV_SFF) {
        /* Obtain the eeprom bus*/
        key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_BUS, e2_type, index);
        eeprom_bus = dfd_ko_cfg_get_item(key);
        if (eeprom_bus == NULL) {
            DBG_EEPROM_DEBUG(DBG_ERROR, "get e2 path fail, e2_type: %d, index: %d\n", e2_type, index);
            return -DFD_RV_DEV_NOTSUPPORT;
        }

        snprintf(eeprom_path, count, SFF_E2_PATH, *eeprom_bus);
        DBG_EEPROM_DEBUG(DBG_VERBOSE, "e2_type: %d, eeprom_bus: %d, path: %s\n", e2_type, *eeprom_bus, eeprom_path);
        return 0;
    } else {
        DBG_EEPROM_DEBUG(DBG_VERBOSE, "unsupported e2_type: %d, index: %d\n", e2_type, index);
    }

    return -DFD_RV_DEV_NOTSUPPORT;
}

static int dfd_get_eeprom_base_offset(int e2_type, int index, int *offset)
{
    uint64_t key;
    int *p_eeprom_bank, eeprom_size;

    *offset = 0;
    if (e2_type == WB_MAIN_DEV_SFF) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_BANK, e2_type, index);
        p_eeprom_bank = dfd_ko_cfg_get_item(key);
        if (p_eeprom_bank != NULL) {
            DBG_EEPROM_DEBUG(DBG_VERBOSE, "get eeprom bank success. e2_type: %d, index: %d, value:%d\n", 
                e2_type, index, *p_eeprom_bank);
            if (*p_eeprom_bank < 0) {
                DBG_EEPROM_DEBUG(DBG_ERROR, "eeprom bank invalid, e2_type: %d, index: %d, value:%d\n", 
                e2_type, index, *p_eeprom_bank);
                return -EINVAL;
            }
            eeprom_size = dfd_get_eeprom_size(e2_type, index);
            if (eeprom_size < 0) {
                return eeprom_size;
            }
            *offset += (*p_eeprom_bank) * eeprom_size;
            DBG_EEPROM_DEBUG(DBG_VERBOSE, "get eeprom base_offset success. e2_type: %d, index: %d, bank: %d, base_offset:0x%x\n", 
                e2_type, index, *p_eeprom_bank, *offset);
        } else {
            DBG_EEPROM_DEBUG(DBG_VERBOSE, "eeprom not config bank. e2_type: %d, index: %d, base_offset:0x%x\n", 
                e2_type, index, *offset);
        }
    }

    return 0;
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
    char tmp_eeprom_path[E2_PATH_SIZE];
    int ret;
    int p_eeprom_offset;

    if (buf == NULL || offset < 0 || count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "params error, offset: 0x%llx, rd_count: %zu.\n",
            offset, count);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Obtain the eeprom read path*/
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_PATH, e2_type, index);
    eeprom_path = dfd_ko_cfg_get_item(key);
    if (eeprom_path == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom path error, e2_type: %d, index: %d, key_name: %s\n",
            e2_type, index, key_to_name(DFD_CFG_ITEM_EEPROM_PATH));
        mem_clear(tmp_eeprom_path, sizeof(tmp_eeprom_path));
        ret = get_eeprom_path_by_bus(e2_type, index, tmp_eeprom_path, sizeof(tmp_eeprom_path));
        if (ret < 0) {
            return ret;
        }
        eeprom_path = tmp_eeprom_path;
    }

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "e2_type: %d, index: %d, path: %s, offset: 0x%llx, \
        rd_count: %zu\n", e2_type, index, eeprom_path, offset, count);

    ret = dfd_get_eeprom_base_offset(e2_type, index, &p_eeprom_offset);
    if (ret < 0) {
        return ret;
    }
    offset += p_eeprom_offset;

    mem_clear(buf, count);
    rd_len = dfd_ko_read_file(eeprom_path, offset, buf, count);
    if (rd_len < 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "read eeprom data failed, loc: %s, offset: 0x%llx, \
        rd_count: %zu, ret: %zd,\n", eeprom_path, offset, count, rd_len);
    } else {
        DBG_EEPROM_DEBUG(DBG_VERBOSE, "read eeprom data success, loc: %s, offset: 0x%llx, \
            rd_count: %zu, rd_len: %zd,\n", eeprom_path, offset, count, rd_len);
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
    char tmp_eeprom_path[E2_PATH_SIZE];
    int ret;
    int p_eeprom_offset;

    if (buf == NULL || offset < 0 || count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "params error, offset: 0x%llx, count: %zu.\n", offset, count);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Obtain the eeprom read path */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_PATH, e2_type, index);
    eeprom_path = dfd_ko_cfg_get_item(key);
    if (eeprom_path == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get eeprom path error, e2_type: %d, index: %d, key_name: %s\n",
            e2_type, index, key_to_name(DFD_CFG_ITEM_EEPROM_PATH));
        mem_clear(tmp_eeprom_path, sizeof(tmp_eeprom_path));
        ret = get_eeprom_path_by_bus(e2_type, index, tmp_eeprom_path, sizeof(tmp_eeprom_path));
        if (ret < 0) {
            return ret;
        }
        eeprom_path = tmp_eeprom_path;
    }

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "e2_type: %d, index: %d, path: %s, offset: 0x%llx, \
        wr_count: %zu.\n", e2_type, index, eeprom_path, offset, count);

    ret = dfd_get_eeprom_base_offset(e2_type, index, &p_eeprom_offset);
    if (ret < 0) {
        return ret;
    }
    offset += p_eeprom_offset;

    wr_len = dfd_ko_write_file(eeprom_path, offset, buf, count);
    if (wr_len < 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "write eeprom data failed, loc:%s, offset: 0x%llx, \
            wr_count: %zu, ret: %zd.\n", eeprom_path, offset, count, wr_len);
    } else {
        DBG_EEPROM_DEBUG(DBG_VERBOSE, "write eeprom data success, loc:%s, offset: 0x%llx, \
            wr_count: %zu, wr_len: %zd.\n", eeprom_path, offset, count, wr_len);
    }

    return wr_len;
}

ssize_t dfd_get_eeprom_alias(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count)
{
    uint64_t key;
    char *e2_alias;

    if (buf == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
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

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "%s\n", e2_alias);
    snprintf(buf, count, "%s\n", e2_alias);
    return strlen(buf);
}

ssize_t dfd_get_eeprom_tag(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count)
{
    uint64_t key;
    char *e2_tag;

    if (buf == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
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

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "%s\n", e2_tag);
    snprintf(buf, count, "%s\n", e2_tag);
    return strlen(buf);
}

ssize_t dfd_get_eeprom_type(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count)
{
    uint64_t key;
    char *eeprom_type;

    if (buf == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
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

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "%s\n", eeprom_type);
    snprintf(buf, count, "%s\n", eeprom_type);
    return strlen(buf);
}

ssize_t dfd_get_eeprom_bus(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count)
{
    uint64_t key;
    uint32_t *eeprom_bus;

    if (buf == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_BUS, e2_type, e2_index);
    eeprom_bus = dfd_ko_cfg_get_item(key);
    if (eeprom_bus == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "eeprom_bus config error, key_name: %s\n",
            key_to_name(DFD_CFG_ITEM_EEPROM_BUS));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "%d\n", *eeprom_bus);
    snprintf(buf, count, "%d\n", *eeprom_bus);
    return strlen(buf);
}

ssize_t dfd_get_eeprom_addr(unsigned int e2_type, unsigned int e2_index, char *buf, size_t count)
{
    uint64_t key;
    uint32_t *eeprom_addr;

    if (buf == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_ADDR, e2_type, e2_index);
    eeprom_addr = dfd_ko_cfg_get_item(key);
    if (eeprom_addr == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "eeprom_addr config error, key_name: %s\n",
            key_to_name(DFD_CFG_ITEM_EEPROM_ADDR));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_EEPROM_DEBUG(DBG_VERBOSE, "0x%x\n", *eeprom_addr);
    snprintf(buf, count, "0x%x\n", *eeprom_addr);
    return strlen(buf);
}

int dfd_set_eeprom_write_protection(unsigned int e2_type, unsigned int e2_index, void *val, unsigned int len)
{
    uint64_t key;
    int ret;
    unsigned int *value;

    value = (unsigned int *)val;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_WP_REG, e2_type, e2_index);
    ret = dfd_info_set_int(key, *value ? 1 : 0);
    if (ret < 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "e2type: %u, e2_index %u reg error, key_name: %s, ret:%d\n",
            e2_type, e2_index, key_to_name(DFD_CFG_ITEM_EEPROM_WP_REG), ret);
        return ret;
    }
    return DFD_RV_OK;
}

int dfd_get_eeprom_write_protection(unsigned int e2_type, unsigned int e2_index, int *value)
{
    uint64_t key;
    int ret;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_WP_REG, e2_type, e2_index);
    ret = dfd_info_get_int(key, value, NULL);
    if (ret < 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "e2type: %u, e2_index %u reg error, key_name: %s, ret: %d\n",
            e2_type, e2_index, key_to_name(DFD_CFG_ITEM_EEPROM_WP_REG), ret);
        return ret;
    }
    return DFD_RV_OK;
}

ssize_t dfd_get_eeprom_wp_reg_str(unsigned int e2_type, unsigned int e2_index,
            char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, cpld index: %u\n",
            e2_type, e2_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, cpld index: %u\n",
            count, e2_type, e2_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    ret = dfd_get_eeprom_write_protection(e2_type, e2_index, &value);
    if (ret < 0) {
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "0x%02x\n", value);
}

