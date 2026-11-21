/*
 * md_device_driver.c
 *
 * This module realize /sys/s3ip/md attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2026-01-07                  S3IP sysfs
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/string.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_md_driver.h" 

#define MD_STATUS_FILE_FORMAT "/etc/sonic/md/md%u_status"

int g_dfd_md_dbg_level = 0;
module_param(g_dfd_md_dbg_level, int, S_IRUGO | S_IWUSR);

dfd_sysfs_func_map_t md_func_table[DFD_MD_MAX_E] = {
    [DFD_MD_STATUS_E] = {dfd_get_md_status, NULL},
    [DFD_MD_DEV_PATH_E] = {dfd_get_md_dev_path, NULL},
};

dfd_debug_data_key_map_t md_dbg_key_table[DFD_MD_MAX_E] = {
    [DFD_MD_STATUS_E] = {DFD_CFG_ITEM_MD_INFO, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_MD_DEV_PATH_E] = {DFD_CFG_ITEM_MD_INFO, CFG_2INDEXES_2, CFG_STR_DATA},
};

ssize_t dfd_get_md_dev_path(unsigned int dev_index, unsigned int type, char *buf, size_t count)
{
    uint64_t key;
    int ret;

    if (buf == NULL) {
        DBG_MD_DEBUG(DBG_ERROR, "param error, buf is NULL. dev_index: %u, type: %u\n",
            dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_MD_DEBUG(DBG_ERROR, "buf size error, count: %zu, dev_index: %u, type: %u\n",
            count, dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }


    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_MD_INFO, dev_index, type);
    ret = dfd_get_value_from_info(key, buf, count);
    if (ret < 0) {
        DBG_MD_DEBUG(DBG_ERROR, "get md info error, key_name: %s, ret: %d\n",
            key_to_name(DFD_CFG_ITEM_MD_INFO), ret);
    } else {
        DBG_MD_DEBUG(DBG_VERBOSE, "get md info success, value: %s\n", buf);
    }
  
    return ret;
}

ssize_t dfd_get_md_status(unsigned int dev_index, unsigned int type, char *buf, size_t count)
{
    char filename[256];
    ssize_t ret;

    if (buf == NULL) {
        DBG_MD_DEBUG(DBG_ERROR, "param error, buf is NULL. dev_index: %u, type: %u\n",
            dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_MD_DEBUG(DBG_ERROR, "buf size error, count: %zu, dev_index: %u, type: %u\n",
            count, dev_index, type);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_dev_debug_config_data(DFD_CFG_ITEM_MD_INFO, dev_index, type, buf, count);
    if (ret >= 0) {
        DBG_MD_DEBUG(DBG_VERBOSE, "get md info from debug node, value: %s\n", buf);
        return ret;
    } else {
        DBG_MD_DEBUG(DBG_VERBOSE, "failed to get md info from debug node, dev_index: %u, type: %u, ret=%zd\n",
            dev_index, type, ret);
    }

    mem_clear(buf, count);
    snprintf(filename, sizeof(filename), MD_STATUS_FILE_FORMAT, dev_index);
    return dfd_ko_read_file(filename, 0, buf, count);
}
