/*
 * An wb_sff_driver driver for sff devcie function
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
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"

int g_dfd_sff_dbg_level = 0;
module_param(g_dfd_sff_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * dfd_set_sff_cpld_info - Example Set the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @value: Writes the value to the register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, int value)
{
    uint64_t key;
    int ret;

    if ((value != 0) && (value != 1)) {
        DFD_SFF_DEBUG(DBG_ERROR, "sff%u cpld reg type %d, can't set invalid value: %d\n",
            sff_index, cpld_reg_type, value);
        return -DFD_RV_INVALID_VALUE;
    }
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set sff%u cpld reg type %d error, key_name: %s, ret: %d.\n",
            sff_index, cpld_reg_type, key_to_name(DFD_CFG_ITEM_SFF_CPLD_REG), ret);
        return ret;
    }

    return DFD_RV_OK;
}

/**
 * dfd_get_sff_cpld_info - Obtain the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, char *buf, size_t count)
{
    uint64_t key;
    int ret, value;

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
    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff%u cpld reg type %d error, key_name: %s, ret: %d\n",
            sff_index, cpld_reg_type, key_to_name(DFD_CFG_ITEM_SFF_CPLD_REG), ret);
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

/**
 * dfd_get_single_eth_optoe_type - get sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_optoe_type(unsigned int sff_index, int *optoe_type)
{
    uint64_t key;
    int ret, value;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_OPTOE_TYPE, sff_index, 0);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff optoe type error, key_name: %s, ret:%d.\n", 
			key_to_name(DFD_CFG_ITEM_SFF_OPTOE_TYPE), ret);
        return ret;
    }

    /* assic int to int */
    *optoe_type = value - '0';
    return ret;
}

/**
 * dfd_set_single_eth_optoe_type - set sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_set_single_eth_optoe_type(unsigned int sff_index, int optoe_type)
{
    uint64_t key;
    int ret, value;

    /* int to assic int */
    value = optoe_type + '0';
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_OPTOE_TYPE, sff_index, 0);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set sff optoe type error, key_name: %s, ret:%d.\n", 
			key_to_name(DFD_CFG_ITEM_SFF_OPTOE_TYPE), ret);
        return ret;
    }

    return ret;
}
