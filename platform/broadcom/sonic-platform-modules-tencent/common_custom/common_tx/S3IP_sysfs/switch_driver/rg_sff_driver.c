/*
 * Copyright(C) 2001-2022 Ruijie Network. All rights reserved.
 */
/*
 * rg_sff_driver.c
 * Original Author: sonic_rd@ruijie.com.cn 2020-02-17
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@ruijie.com.cn         2020-02-17          Initial version
 */

#include <linux/module.h>

#include "rg_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"

int g_dfd_sff_dbg_level = 0;
module_param(g_dfd_sff_dbg_level, int, S_IRUGO | S_IWUSR);

int dfd_set_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, int value)
{
    int key, ret;

    if ((value != 0) && (value != 1)) {
        DFD_SFF_DEBUG(DBG_ERROR, "sff%u cpld reg type %d, can't set invalid value: %d\n",
            sff_index, cpld_reg_type, value);
        return -DFD_RV_INVALID_VALUE;
    }
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set sff%u cpld reg type %d error, key: 0x%x, ret: %d.\n",
            sff_index, cpld_reg_type, key, ret);
        return ret;
    }

    return DFD_RV_OK;
}

ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, char *buf, size_t count)
{
    int key, ret, value;

    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL. sff_index: %u, cpld_reg_type: %d\n",
            sff_index, cpld_reg_type);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "buf size error, count: %lu, sff index: %u, cpld_reg_type: %d\n",
            count, sff_index, cpld_reg_type);
        return -DFD_RV_INVALID_VALUE;
    }
    memset(buf, 0 , count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff%u cpld reg type %d error, key: 0x%x, ret: %d\n",
            sff_index, cpld_reg_type, key, ret);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}
