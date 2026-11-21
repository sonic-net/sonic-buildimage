/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_sol_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * sol related properties read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0          support                    2025-07-17        Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_sol_driver.h"

int g_dfd_sol_dbg_level = 0;
module_param(g_dfd_sol_dbg_level, int, S_IRUGO | S_IWUSR);

#define DFD_SOL_ROUTING_SUPPORT                         (1)
#define DFD_SOL_MAX_DECODE_NUM                          (2)
#define DFD_SOL_MAX_DECODE_LEN                          (128)

dfd_sysfs_func_map_t sol_func_table[DFD_SOL_MAX_E] = {
    [DFD_SOL_NAME_E] = {dfd_get_sol_name, NULL},
    [DFD_SOL_DEVICE_E] = {dfd_get_sol_device, NULL},
    [DFD_SOL_ACTIVE_E] = {dfd_get_sol_active, dfd_set_sol_active},
};

ssize_t dfd_get_sol_name(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint64_t key;
    char *string_value;

    if (buf == NULL) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SOL_NAME, main_dev_id, sub_dev_id);
    string_value = dfd_ko_cfg_get_item(key);
    if (string_value == NULL) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, key_name: %s\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_SOL_NAME));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_SYSSOL_DEBUG(DBG_VERBOSE, "%s\n", string_value);
    snprintf(buf, count, "%s\n", string_value);
    return strlen(buf);
}

ssize_t dfd_get_sol_device(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint64_t key;
    char *string_value;

    if (buf == NULL) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SOL_DEVICE, main_dev_id, sub_dev_id);
    string_value = dfd_ko_cfg_get_item(key);
    if (string_value == NULL) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, key_name: %s\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_SOL_DEVICE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_SYSSOL_DEBUG(DBG_VERBOSE, "%s\n", string_value);
    snprintf(buf, count, "%s\n", string_value);
    return strlen(buf);
}

ssize_t dfd_get_sol_active(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int ret, value, i;
    info_ctrl_t *info_ctrl;
    uint64_t key;
    char *string_value;
    unsigned char ori_string[DFD_SOL_MAX_DECODE_LEN];
    char bracketed_value[DFD_SOL_MAX_DECODE_LEN];
    char *p_decode_value;

    if (buf == NULL) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    mem_clear(bracketed_value, DFD_SOL_MAX_DECODE_LEN);
    mem_clear(ori_string, DFD_SOL_MAX_DECODE_LEN);

    value = 0;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SOL_ACTIVE, main_dev_id, sub_dev_id);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_SYSSOL_DEBUG(DBG_VERBOSE, "key_name: %s, main_dev_id: 0x%x  sub_dev_id 0x%x not found.\n",
            key_to_name(DFD_CFG_ITEM_SOL_ACTIVE), main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (info_ctrl->int_extra1 == DFD_SOL_ROUTING_SUPPORT) {
        if (info_ctrl->mode == INFO_CTRL_MODE_CFG && info_ctrl->src == INFO_SRC_FILE) {
            DBG_SYSSOL_DEBUG(DBG_VERBOSE, "Support uart routing, parse in the specific way.\n");
            ret = dfd_info_get_buf(key, ori_string, DFD_SOL_MAX_DECODE_LEN, NULL);
            if (ret < 0) {
                DBG_SYSSOL_DEBUG(DBG_ERROR, "get uart routing failed, key:0x%08llx, ret:%d\n", key, ret);
                return ret;
            }

            for (i = 0; i < DFD_SOL_MAX_DECODE_NUM; i++) {
                key = DFD_CFG_KEY(DFD_CFG_ITEM_SOL_ACTIVE_DECODE, main_dev_id, i);
                string_value = dfd_ko_cfg_get_item(key);
                if (string_value == NULL) {
                    DBG_SYSSOL_DEBUG(DBG_VERBOSE, "key_name: %s, main_dev_id: 0x%x  sub_dev_id 0x%x not found.\n",
                        key_to_name(DFD_CFG_ITEM_SOL_ACTIVE_DECODE), main_dev_id, i);
                    continue;
                }

                snprintf(bracketed_value, sizeof(bracketed_value), "[%s]", string_value);
                if (strstr(ori_string, bracketed_value) != NULL) {
                    snprintf(buf, count, "%d\n", i);
                    return strlen(buf);
                }
            }
        }

        return -DFD_RV_INVALID_VALUE;
    }

    DBG_SYSSOL_DEBUG(DBG_VERBOSE, "Don't support uart routing, parse in the normal way.\n");
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, key_name: %s\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_SOL_ACTIVE));
        return ret;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SOL_ACTIVE_DECODE, main_dev_id, value);
    p_decode_value = dfd_ko_cfg_get_item(key);
    if (p_decode_value != NULL) {
        value = simple_strtol(p_decode_value, NULL, 0);
        DBG_SYSSOL_DEBUG(DBG_VERBOSE, "decode sol active status success, sol id: 0x%x, decode value: 0x%x\n",
            main_dev_id, *p_decode_value);
    }

    return snprintf(buf, count, "%d\n", value);
}

int dfd_set_sol_active(unsigned int main_dev_id, unsigned int sub_dev_id, void *val, unsigned int len)
{
    uint64_t key;
    int ret;
    int *value;
    char *string_value;
    char *p_decode_value;
    info_ctrl_t *info_ctrl;

    value = (int *)val;
    if (*value < 0 || *value > 0xff) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "main_dev_id: %u, can't set sub_dev_id %u  value = 0x%02x, len = %d\n",
            main_dev_id, sub_dev_id, *value, len);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SOL_ACTIVE, main_dev_id, sub_dev_id);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_SYSSOL_DEBUG(DBG_VERBOSE, "key_name: %s, main_dev_id: 0x%x  sub_dev_id 0x%x not found, use origin get system method\n",
            key_to_name(DFD_CFG_ITEM_SOL_ACTIVE), main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (info_ctrl->int_extra1 == DFD_SOL_ROUTING_SUPPORT) {
        DBG_SYSSOL_DEBUG(DBG_VERBOSE, "Support uart routing, parse in the specific way.\n");
        key = DFD_CFG_KEY(DFD_CFG_ITEM_SOL_ACTIVE_DECODE, main_dev_id, *value);
        string_value = dfd_ko_cfg_get_item(key);
        if (string_value == NULL) {
            DBG_SYSSOL_DEBUG(DBG_ERROR, "key_name: %s, main_dev_id: 0x%x value 0x%x not found, use origin get system method\n",
                key_to_name(DFD_CFG_ITEM_SOL_ACTIVE_DECODE), main_dev_id, *value);
            return -DFD_RV_INVALID_VALUE;
        }

        ret = dfd_ko_write_file(info_ctrl->fpath, 0, (uint8_t *)string_value, strlen(string_value));
        if (ret < 0) {
            DBG_SYSSOL_DEBUG(DBG_ERROR, "sol routing write file failed, path:%s, ret: %d\n", info_ctrl->fpath, ret);
            return -DFD_RV_INVALID_VALUE;
        }

        return DFD_RV_OK;
    }

    DBG_SYSSOL_DEBUG(DBG_VERBOSE, "Don't support uart routing, parse in the normal way.\n");

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SOL_ACTIVE_DECODE, main_dev_id, *value);
    p_decode_value = dfd_ko_cfg_get_item(key);
    if (p_decode_value != NULL) {
        DBG_SYSSOL_DEBUG(DBG_VERBOSE, "decode sol active status success, sol id: 0x%x, value: 0x%x, decode value: 0x%x\n",
                main_dev_id, *value, *p_decode_value);
        *value = simple_strtol(p_decode_value, NULL, 0);
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SOL_ACTIVE, main_dev_id, sub_dev_id);
    ret = dfd_info_set_int(key, *value);
    if (ret < 0) {
        DBG_SYSSOL_DEBUG(DBG_ERROR, "main_dev_id: %u, set sub_dev_id %u error, key_name: %s, ret:%d\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_SOL_ACTIVE), ret);
        return ret;
    }

    return DFD_RV_OK;
}


