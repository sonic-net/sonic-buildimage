/*
 * Copyright(C) 2001-2025 whitebox Network. All rights reserved.
 */
/*
 * wb_pll_driver.c
 * Original Author: [Your Name] [Date]
 *
 * PLL (GPU Computing Unit) related properties read and write function
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
#include "wb_pll_driver.h"

int g_dfd_pll_dbg_level = 0;
module_param(g_dfd_pll_dbg_level, int, S_IRUGO | S_IWUSR);

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
static const dfd_status_desc_map_t dfd_pll_status_map[] = {
    {(1U << DFD_CLOCK_FAULT_XTAL), "Xtal_Error"},
    {(1U << DFD_CLOCK_FAULT_BUS),  "Bus_Error"},
    {(1U << DFD_CLOCK_FAULT_APLL), "Pll_Error"},
};
#endif

static ssize_t dfd_pll_get_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_pll_get_alias(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);

dfd_sysfs_func_map_t pll_func_table[DFD_PLL_MAX_E] = {
    [DFD_PLL_BUS_STATUS_E] = {dfd_pll_get_status, NULL},
    [DFD_PLL_ALIAS_E] = {dfd_pll_get_alias, NULL},
};

static ssize_t dfd_pll_get_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
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
        DBG_PLL_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_PLL_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PLL_DEVICE, main_dev_id, sub_dev_id);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DBG_PLL_DEBUG(DBG_ERROR, "failed to get status, main_dev_id: %u, key_name: %s, ret: %d\n",
            main_dev_id, key_to_name(DFD_CFG_ITEM_PLL_DEVICE), ret);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return -DFD_RV_DEV_NOTSUPPORT;
        } else {
            status = 1U << DFD_CLOCK_FAULT_BUS;
        }
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    mem_clear(detail, sizeof(detail));
    DFD_STATUS_DETAIL_CFG_SET(detail_cfg, status, 0, dfd_pll_status_map, true);
    detail_len = dfd_status_get_detail(&detail_cfg, detail, sizeof(detail));
    if (detail_len > 0) {
        return (ssize_t)snprintf(buf, count, "0x%x %s\n", status, detail);
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", status);
    }
#else
    return (ssize_t)snprintf(buf, count, "%d\n", status);
#endif
}

/* PLL get alias */
static ssize_t dfd_pll_get_alias(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    info_ctrl_t *info_ctrl;
    uint64_t key;

    if (buf == NULL) {
        DBG_PLL_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_PLL_DEBUG(DBG_VERBOSE, "dfd_pll_get_alias has been called, main_dev_id: %u, sub_dev_id: %u, count: %zu.\n",
        main_dev_id, sub_dev_id, count);

    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PLL_DEVICE, main_dev_id, sub_dev_id);
    DBG_PLL_DEBUG(DBG_VERBOSE, "alias key: 0x%08llx\n", key);

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_PLL_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return (size_t)snprintf(buf, count, "%s\n", info_ctrl->str_cons);
}