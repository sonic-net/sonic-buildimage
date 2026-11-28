/*
 * Copyright(C) 2001-2012 whitebox. All rights reserved.
 */
/*
 * wb_lsw_driver.c
 * Original Author: support 2025-12-24 
 *
 * LSW related attribute read and write functions
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0         support                    2025-12-24          Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"
#include "wb_lsw_driver.h"

int g_dfd_lsw_dbg_level = 0;
module_param(g_dfd_lsw_dbg_level, int, S_IRUGO | S_IWUSR);

static ssize_t dfd_get_lsw_reset_str(unsigned int main_dev_id, unsigned int type,
            char *buf, size_t count);
static int dfd_set_lsw_reset_str(unsigned int main_dev_id, unsigned int type,
            void *val, unsigned int len);

dfd_sysfs_func_map_t lsw_func_table[DFD_LSW_MAX_E] = {
    [DFD_LSW_RESET_E] = {dfd_get_lsw_reset_str, dfd_set_lsw_reset_str},
};

/**
 * dfd_get_lsw_reset_str - Read the LSW reset register value
 * @main_dev_id: index of LSW
 * @type: The type of the LSW
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
static ssize_t dfd_get_lsw_reset_str(unsigned int main_dev_id, unsigned int type,
            char *buf, size_t count)
{
    int ret, value;
    uint64_t key;

    if (buf == NULL) {
        DBG_LSW_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, type: %u\n",
            main_dev_id, type);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_LSW_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, type: %u\n",
            count, main_dev_id, type);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_LSW_DEVICE, main_dev_id, type);
    DBG_LSW_DEBUG(DBG_VERBOSE, "alias key: 0x%08llx\n", key);

    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DBG_LSW_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx, ret:%d\n\n", key, ret);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

/**
 * dfd_set_lsw_reset_str - Set the LSW test register value
 * @main_dev_id: index of LSW
 * @type: The type of the LSW
 * @value: Writes the value of the reset register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
static int dfd_set_lsw_reset_str(unsigned int main_dev_id, unsigned int type, void *val, unsigned int len)
{
    uint64_t key;
    int ret;
    unsigned int *value;

    value = (unsigned int *)val;
    if (*value < 0 || *value > 0xff) {
        DBG_LSW_DEBUG(DBG_ERROR, "can't set lsw%u test reg value = 0x%02x, len = %d\n",
            main_dev_id, *value, len);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_LSW_DEVICE, main_dev_id, type);

    DBG_LSW_DEBUG(DBG_VERBOSE, "set 0x%x to key: %s (0x%08llx) main_dev_id: %d, type: %d\r\n",
        *value, key_to_name(DFD_CFG_ITEM_LSW_DEVICE), key, main_dev_id, type);
    ret = dfd_info_set_int(key, *value);
    if (ret < 0) {
        DBG_LSW_DEBUG(DBG_ERROR, "set lsw%u test reg error, key_name: %s, ret:%d\n",
            main_dev_id, key_to_name(DFD_CFG_ITEM_LSW_DEVICE), ret);
        return ret;
    }
    return DFD_RV_OK;
}
