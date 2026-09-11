/*
 * Copyright(C) 2001-2025 whitebox Network. All rights reserved.
 */
/*
 * wb_clock_driver.c
 * Original Author: [Your Name] [Date]
 *
 * CLOCK (GPU Computing Unit) related properties read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0                                    2025-12-24        Initial version
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_clock_driver.h"

int g_dfd_clock_dbg_level = 0;
module_param(g_dfd_clock_dbg_level, int, S_IRUGO | S_IWUSR);

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
static const dfd_status_desc_map_t dfd_clock_status_map[] = {
    {DFD_CLOCK_FAULT, "Clock_Error"},
};
#endif

dfd_sysfs_func_map_t clock_func_table[DFD_CLOCK_MAX_E] = {
    [DFD_CLOCK_STATUS_E] = {dfd_clock_get_status, NULL},
    [DFD_CLOCK_ALIAS_E] = {dfd_clock_get_alias, NULL},
};

ssize_t dfd_clock_get_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint64_t key;
    int status;
    int ret;
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    char detail[64];
    size_t detail_len;
    dfd_status_detail_t detail_cfg;
#endif

    if (buf == NULL) {
        DBG_CLOCK_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_CLOCK_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CLOCK_DEVICE, main_dev_id, sub_dev_id);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DBG_CLOCK_DEBUG(DBG_ERROR, "failed to get status, main_dev_id: %u, key_name: %s, ret: %d\n",
            main_dev_id, key_to_name(DFD_CFG_ITEM_CLOCK_DEVICE), ret);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return -DFD_RV_DEV_NOTSUPPORT;
        } else {
            status = DFD_CLOCK_FAULT;
        }
    }

    if (status != 0) {
        status = DFD_CLOCK_FAULT;
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    mem_clear(detail, sizeof(detail));
    DFD_STATUS_DETAIL_CFG_SET(detail_cfg, status, 0, dfd_clock_status_map, true);
    detail_len = dfd_status_get_detail(&detail_cfg, detail, sizeof(detail));
    if (detail_len > 0) {
        return (ssize_t)snprintf(buf, count, "0x%x %s\n", status, detail);
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", status);
    }
#else
    snprintf(buf, count, "0x%x\n", status);
    return strlen(buf);
#endif
}

/* CLOCK get alias */
ssize_t dfd_clock_get_alias(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    info_ctrl_t *info_ctrl;
    uint64_t key;

    if (buf == NULL) {
        DBG_CLOCK_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_CLOCK_DEBUG(DBG_VERBOSE, "dfd_clock_get_alias has been called, main_dev_id: %u, sub_dev_id: %u, count: %zu.\n",
        main_dev_id, sub_dev_id, count);

    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CLOCK_DEVICE, main_dev_id, sub_dev_id);
    DBG_CLOCK_DEBUG(DBG_VERBOSE, "alias key: 0x%08llx\n", key);

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_CLOCK_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return (size_t)snprintf(buf, count, "%s\n", info_ctrl->str_cons);
}
