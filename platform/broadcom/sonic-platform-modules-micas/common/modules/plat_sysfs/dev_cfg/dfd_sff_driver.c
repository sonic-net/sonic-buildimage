/*
 * An dfd_sff_driver driver for dfd sff function
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

#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_info.h"
#include "./include/dfd_cfg_adapter.h"
#include "../dev_sysfs/include/sysfs_common.h"

int g_dfd_sff_dbg_level = 0;
module_param(g_dfd_sff_dbg_level, int, S_IRUGO | S_IWUSR);

ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, char *buf, int len)
{
    int key, ret, value;

    if(buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL. sff_index:%d, cpld_reg_type:%d.\n",
            sff_index, cpld_reg_type);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff cpld reg error, key:0x%x,ret:%d.\n", key, ret);
        return ret;
    }

    mem_clear(buf, len);
    return (ssize_t)snprintf(buf, len, "%d\n", value);
}

ssize_t dfd_get_sff_dir_name(unsigned int sff_index, char *buf, int buf_len)
{
    int key;
    char *sff_dir_name;

    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error. buf is NULL.sff index:%d", sff_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, buf_len);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_DIR_NAME, sff_index, 0);
    sff_dir_name = dfd_ko_cfg_get_item(key);
    if (sff_dir_name == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "sff dir name config error, key=0x%08x\n", key);
        return -DFD_RV_NODE_FAIL;
    }

    DFD_SFF_DEBUG(DBG_VERBOSE, "%s\n", sff_dir_name);
    snprintf(buf, buf_len, "%s", sff_dir_name);
    return strlen(buf);

}
