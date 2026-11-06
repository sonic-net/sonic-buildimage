/*
 * Copyright(C) 2001-2025 whitebox Network. All rights reserved.
 */
/*
 * wb_pcie_driver.c
 * Original Author: [Your Name] [Date]
 *
 * PCIE (GPU Computing Unit) related properties read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0                                    2025-12-24        Initial version
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/namei.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_pcie_driver.h"

int g_dfd_pcie_dbg_level = 0;
module_param(g_dfd_pcie_dbg_level, int, S_IRUGO | S_IWUSR);

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
static const dfd_status_desc_map_t dfd_pcie_link_status_map[] = {
    {PCIE_STATUS_ABNORMAL, "Link_Down"},
};

static const dfd_status_desc_map_t dfd_pcie_speed_status_map[] = {
    {PCIE_STATUS_ABNORMAL, "Speed_Downgrade"},
};
#endif

static ssize_t dfd_pcie_get_link_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_pcie_get_speed_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_pcie_get_constant_str(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);

dfd_sysfs_func_map_t pcie_func_table[DFD_PCIE_MAX_E] = {
    [DFD_PCIE_LINK_STATUS_E] = {dfd_pcie_get_link_status, NULL},
    [DFD_PCIE_SPEED_STATUS_E] = {dfd_pcie_get_speed_status, NULL},
    [DFD_PCIE_ALIAS_E] = {dfd_pcie_get_constant_str, NULL},
    [DFD_PCIE_TYPE_E] = {dfd_pcie_get_constant_str, NULL},
};

dfd_debug_data_key_map_t pcie_dbg_key_table[DFD_PCIE_MAX_E] = {
    [DFD_PCIE_LINK_STATUS_E] = {DFD_CFG_ITEM_PCIE_DEVICE, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_PCIE_SPEED_STATUS_E] = {DFD_CFG_ITEM_PCIE_DEVICE, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_PCIE_ALIAS_E] = {DFD_CFG_ITEM_PCIE_DEVICE, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_PCIE_TYPE_E] = {DFD_CFG_ITEM_PCIE_DEVICE, CFG_2INDEXES_2, CFG_STR_DATA},
}; 

static ssize_t dfd_pcie_get_file_str_value(const char *file_path, char *bdf, char *target_str, size_t target_str_len)
{
    struct path file_path_struct;
    ssize_t ret;
    char tmp_file[INFO_BUF_MAX_LEN] = {0};

    if (file_path == NULL || bdf == NULL || target_str == NULL || target_str_len == 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "failed to read file value, file_path == NULL: %d, bdf == NULL: %d, target_str == NULL: %d, target_str_len: %zu\n",
            file_path == NULL, bdf == NULL, target_str == NULL, target_str_len);
            return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(target_str, target_str_len);
    snprintf(tmp_file, sizeof(tmp_file), file_path, bdf);

    ret = kern_path(tmp_file, LOOKUP_REVAL | LOOKUP_FOLLOW, &file_path_struct);
    if (ret < 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "file path is invalid or does not exist, file_path: %s, ret: %zd\n", tmp_file, ret);
        if (ret == -ENOENT || ret == -ENOTDIR) {
            return -DFD_RV_NO_NODE;
        }
        return -DFD_RV_DEV_FAIL;
    }

    path_put(&file_path_struct);

    ret = dfd_ko_read_file(tmp_file, 0, target_str, target_str_len);
    if (ret < 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "failed to read file value, file_path: %s, ret: %zd\n", tmp_file, ret);
        return ret;
    }

    dfd_info_del_no_print_string(target_str);

    return ret;
}

static ssize_t dfd_pcie_get_link_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    char tmp_file[INFO_BUF_MAX_LEN] = {0};
    char tmp_bdf[PCIE_DEVICE_BDF_MAX_LENGTH] = {0};
    uint8_t tmp_str[INFO_BUF_MAX_LEN] = {0};
    ssize_t ret;
    uint32_t tmp_width_current;
    uint32_t tmp_width_max;
    int status;
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    char detail[64];
    size_t detail_len;
    dfd_status_detail_t detail_cfg;
#endif

    ret = 0;

    if (buf == NULL) {
        DBG_PCIE_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\r\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_dev_debug_config_data(DFD_CFG_ITEM_PCIE_DEVICE, main_dev_id, sub_dev_id, buf, count);
    if (ret >= 0) {
        DBG_PCIE_DEBUG(DBG_VERBOSE, "get link status from debug node, value: %s\n", buf);
        return ret;
    } else {
        DBG_PCIE_DEBUG(DBG_VERBOSE, "failed to get link status from debug node, main_dev_id: %u, sub_dev_id: %u, ret=%zd\n",
            main_dev_id, sub_dev_id, ret);
    }

    /* 1. get BDF */
    ret = dfd_pcie_get_constant_str(main_dev_id, DFD_PCIE_BDF_E, tmp_bdf, sizeof(tmp_bdf));
    if (ret <= 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "The BDF of the target device is not configured, main_dev_id: %u, DFD_PCIE_BDF_E: %u\n",
            main_dev_id, DFD_PCIE_BDF_E);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    dfd_info_del_no_print_string(tmp_bdf);
    DBG_PCIE_DEBUG(DBG_VERBOSE, "BDF is: %s!\r\n", tmp_bdf);

    /* 2. get current width */
    ret = dfd_pcie_get_file_str_value(PCIE_DEVICE_CURRENT_LINK_WIDTH, tmp_bdf, tmp_str, sizeof(tmp_str));
    if (ret < 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "failed to read link width, file_path: %s, bdf: %s, ret: %zd,\n", PCIE_DEVICE_CURRENT_LINK_WIDTH, tmp_bdf, ret);
        if (ret == -DFD_RV_NO_NODE) {
            status = PCIE_STATUS_ABNORMAL;
            goto end;
        } else {
            return ret;
        }
    }

    ret = kstrtouint(tmp_str, 0, &tmp_width_current);

    if (ret) {
        DBG_PCIE_DEBUG(DBG_ERROR, "current width is illegal, current tmp_str: %s\r\n", tmp_str);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_PCIE_DEBUG(DBG_VERBOSE, "current_link_width is: %d!\r\n", tmp_width_current);

    /* 3. get max width */
    mem_clear(tmp_file, sizeof(tmp_file));

    ret = dfd_pcie_get_file_str_value(PCIE_DEVICE_MAX_LINK_WIDTH, tmp_bdf, tmp_str, sizeof(tmp_str));
    if (ret < 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "failed to read link max width, file_path: %s, bdf: %s, ret: %zd,\n", PCIE_DEVICE_MAX_LINK_WIDTH, tmp_bdf, ret);
        if (ret == -DFD_RV_NO_NODE) {
            status = PCIE_STATUS_ABNORMAL;
            goto end;
        } else {
            return ret;
        }
    }

    ret = kstrtouint(tmp_str, 0, &tmp_width_max);

    if (ret) {
        DBG_PCIE_DEBUG(DBG_ERROR, "current width is illegal, max tmp_str: %s\r\n", tmp_str);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_PCIE_DEBUG(DBG_VERBOSE, "max_link_width is: %u!\r\n", tmp_width_max);

    /* 4. compare */
    mem_clear(buf, count);
    if (tmp_width_current == PCIE_DEVICE_ABNORMAL_VALUE || tmp_width_current < tmp_width_max) {
        DBG_PCIE_DEBUG(DBG_ERROR, "current width is illegal, tmp_width_current: %u, tmp_width_max: %u\r\n", tmp_width_current, tmp_width_max);
        status = PCIE_STATUS_ABNORMAL;
    } else {
        DBG_PCIE_DEBUG(DBG_VERBOSE, "current_link_width is: %u!\r\n", tmp_width_max);
        status = PCIE_STATUS_OK;
    }

end:
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    mem_clear(detail, sizeof(detail));
    DFD_STATUS_DETAIL_CFG_SET(detail_cfg, status, PCIE_STATUS_OK, dfd_pcie_link_status_map, false);
    detail_len = dfd_status_get_detail(&detail_cfg, detail, sizeof(detail));
    if (detail_len > 0) {
        return (ssize_t)snprintf(buf, count, "%d %s\n", status, detail);
    } else {
        return (ssize_t)snprintf(buf, count, "%d\n", status);
    }
#else
    snprintf(buf, count, "%d\n", status);
    return strlen(buf);
#endif
}

static ssize_t dfd_pcie_get_speed_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    char tmp_bdf[PCIE_DEVICE_BDF_MAX_LENGTH] = {0};
    uint8_t tmp_str_current[INFO_BUF_MAX_LEN] = {0};
    uint8_t tmp_str_max[INFO_BUF_MAX_LEN] = {0};
    ssize_t ret;
    int status;
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    char detail[64];
    size_t detail_len;
    dfd_status_detail_t detail_cfg;
#endif

    if (buf == NULL) {
        DBG_PCIE_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = 0;
    mem_clear(buf, count);

    ret = dfd_get_dev_debug_config_data(DFD_CFG_ITEM_PCIE_DEVICE, main_dev_id, sub_dev_id, buf, count);
    if (ret >= 0) {
        DBG_PCIE_DEBUG(DBG_VERBOSE, "get link status from debug node, value: %s\n", buf);
        return ret;
    } else {
        DBG_PCIE_DEBUG(DBG_VERBOSE, "failed to get link status from debug node, main_dev_id: %u, sub_dev_id: %u, ret=%zd\n",
            main_dev_id, sub_dev_id, ret);
    }

    /* 1. get BDF */
    ret = dfd_pcie_get_constant_str(main_dev_id, DFD_PCIE_BDF_E, tmp_bdf, sizeof(tmp_bdf));
    if (ret <= 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "The BDF of the target device is not configured, main_dev_id: %u, DFD_PCIE_BDF_E: %u\n",
            main_dev_id, DFD_PCIE_BDF_E);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    dfd_info_del_no_print_string(tmp_bdf);
    DBG_PCIE_DEBUG(DBG_VERBOSE, "BDF is: %s!\r\n", tmp_bdf);

    /* 2. get current speed */
    ret = dfd_pcie_get_file_str_value(PCIE_DEVICE_CURRENT_LINK_SPEED, tmp_bdf, tmp_str_current, sizeof(tmp_str_current));
    if (ret < 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "failed to read current speed, file_path: %s, bdf: %s, ret: %zd,\n",
            PCIE_DEVICE_CURRENT_LINK_SPEED, tmp_bdf, ret);
        if (ret == -DFD_RV_NO_NODE) {
            status = PCIE_STATUS_ABNORMAL;
            goto end;
        } else {
            return ret;
        }
    }

    /* 3. get max speed */
    ret = dfd_pcie_get_file_str_value(PCIE_DEVICE_MAX_LINK_SPEED, tmp_bdf, tmp_str_max, sizeof(tmp_str_max));
    if (ret < 0) {
        DBG_PCIE_DEBUG(DBG_ERROR, "failed to read max speed, file_path: %s, bdf: %s, ret: %zd,\n",
            PCIE_DEVICE_MAX_LINK_SPEED, tmp_bdf, ret);
        if (ret == -DFD_RV_NO_NODE) {
            status = PCIE_STATUS_ABNORMAL;
            goto end;
        } else {
            return ret;
        }
    }

    /* 4. compare
     * if current_speed == Unknown or current_speed |= max_speed:
     *     return 1;
     * else
     *     return 0;
     */
    if (!strcmp(tmp_str_current, PCIE_DEVICE_INVALID_VALUE) || strcmp(tmp_str_current, tmp_str_max)) {
        DBG_PCIE_DEBUG(DBG_ERROR, "current speed is illegal, current speed: %s, max speed: %s\r\n", tmp_str_current, tmp_str_max);
        status = PCIE_STATUS_ABNORMAL;
    } else {
        DBG_PCIE_DEBUG(DBG_VERBOSE, "current speed is %s, max speed is %s!\r\n", tmp_str_current, tmp_str_max);
        status = PCIE_STATUS_OK;
    }

end:
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    mem_clear(detail, sizeof(detail));
    DFD_STATUS_DETAIL_CFG_SET(detail_cfg, status, PCIE_STATUS_OK, dfd_pcie_speed_status_map, false);
    detail_len = dfd_status_get_detail(&detail_cfg, detail, sizeof(detail));
    if (detail_len > 0) {
        return (ssize_t)snprintf(buf, count, "%d %s\n", status, detail);
    } else {
        return (ssize_t)snprintf(buf, count, "%d\n", status);
    }
#else
    snprintf(buf, count, "%d\n", status);
    return strlen(buf);
#endif
}

/* PCIE get constant str */
static ssize_t dfd_pcie_get_constant_str(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    info_ctrl_t *info_ctrl;
    uint64_t key;

    if (buf == NULL) {
        DBG_PCIE_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_PCIE_DEBUG(DBG_VERBOSE, "dfd_pcie_get_constant_str has been called, main_dev_id: %u, sub_dev_id: %u, count: %zu.\n",
        main_dev_id, sub_dev_id, count);

    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PCIE_DEVICE, main_dev_id, sub_dev_id);
    DBG_PCIE_DEBUG(DBG_VERBOSE, "alias key: 0x%08llx\n", key);

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return (size_t)snprintf(buf, count, "%s\n", info_ctrl->str_cons);
}
