/*
 * An wb_system_driver driver for system devcie function
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
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_system_driver.h"
#include "switch_driver.h"

#define NODE_MAX_LEN        (64)

int g_dfd_custom_dbg_level = 0;
module_param(g_dfd_custom_dbg_level, int, S_IRUGO | S_IWUSR);

/* Get current function step number */
int dfd_get_cmd_count(unsigned int type)
{
    uint64_t key;
    int cmd_num;
    int *p_cmd_num;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_BMC_SYSTEM_CMD_NUM, type, 0);
    p_cmd_num = dfd_ko_cfg_get_item(key);
    if (p_cmd_num == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get cmd number failed, key_name:%s\n",
            key_to_name(DFD_CFG_ITEM_BMC_SYSTEM_CMD_NUM));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    cmd_num = *p_cmd_num;
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get cmd number ok, type:0x%x, number:%d\n", type, cmd_num);
    return cmd_num;
}

void dfd_cmd_delay(unsigned int usdelay)
{
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "usdelay:%d\n", usdelay);
    usleep_range(usdelay, usdelay + 1);
    return;
}

ssize_t dfd_system_get_system_value(unsigned int type, int *value)
{
    uint64_t key;
    int ret;
    info_ctrl_t *info_ctrl;
    int *p_decode_value;


    key = DFD_CFG_KEY(DFD_CFG_ITEM_BMC_SYSTEM, type, 0);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get info ctrl fail, key_name: %s, type=0x%x\n",
            key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get, key_name: %s, type=0x%x\n",
        key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type);
    ret = dfd_info_get_int(key, value, NULL);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get system value error, key_name: %s, type=0x%x, ret:%d\n",
            key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type, ret);
        return ret;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SYSTEM_STATUS_DECODE, type, *value);
    p_decode_value = dfd_ko_cfg_get_item(key);
    if (p_decode_value == NULL) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "type:%d, status needn't decode. value:0x%x\n", type, *value);
    } else {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "type:%d, ori_value:0x%x, decoded value:0x%x\n", type, *value, *p_decode_value);
        *value = *p_decode_value;
    }
    return DFD_RV_OK;
}

static int dfd_system_check_value_i(unsigned int type_detail, int cmd_i)
{
    uint64_t key;
    int ret, i;
    info_ctrl_t *info_ctrl;
    int tmp_value, retry_times;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM, type_detail, cmd_i);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key=%s, type_detail=0x%x, cmd_i=%d, don't need to check value\n",
            key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i);
        return DFD_RV_OK;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type_detail=0x%x, cmd_i=%d, start to check value,\n",
        key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i);
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "check value, except value: %d, retry_times: %d, sleep_time: %dus\n",
        info_ctrl->int_extra1, info_ctrl->int_extra2, info_ctrl->int_extra3);

    if (info_ctrl->int_extra2 <= 0) {
        retry_times = 1;
    } else {
        retry_times = info_ctrl->int_extra2;
    }

    for (i = 0; i < retry_times; i++) {
        ret = dfd_info_get_int(key, &tmp_value, NULL);
        if (ret < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "key_name=%s, type_detail=0x%x, cmd_i=%d, get check value error, ret: %d\n",
                key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i, ret);
            return ret;
        }
        if (tmp_value == info_ctrl->int_extra1) {
            DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type_detail=0x%x, cmd_i=%d, check value ok, get value: %d, except value: %d\n",
                key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i, tmp_value, info_ctrl->int_extra1);
            return DFD_RV_OK;
        }
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type_detail=0x%x, cmd_i=%d, check value failed, get value: %d, except value: %d, retry: %d\n",
            key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i, tmp_value, info_ctrl->int_extra1, i + 1);

        if (info_ctrl->int_extra3 > 0) {
            dfd_cmd_delay(info_ctrl->int_extra3);
        }
    }

    DFD_SYSTEM_DEBUG(DBG_ERROR, "key_name=%s, type_detail=0x%x, cmd_i=%d, check value failed, get value: %d, except value: %d\n",
        key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i, tmp_value, info_ctrl->int_extra1);
    return -DFD_RV_CHECK_FAIL;
}

ssize_t dfd_system_set_system_value(unsigned int type, int value)
{
    uint64_t key;
    int ret, cmd_i, cmd_count;
    info_ctrl_t *info_ctrl;
    unsigned int type_detail;
    int tmp_value;

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "set system value, type=0x%x, value=%d\n", type, value);
    /* get step number */
    type_detail = type | (value & 0xff);
    ret = dfd_get_cmd_count(type_detail);
    if(ret <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get cmd number, type_detail=0x%x\n", type_detail);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    cmd_count = ret;
    /* exec each step */
    for(cmd_i = 0; cmd_i < cmd_count; cmd_i++) {
        /* first do pre check */
        key = DFD_CFG_KEY(DFD_CFG_ITEM_PRE_CHECK_BMC_SYSTEM, type_detail, cmd_i);
        info_ctrl = dfd_ko_cfg_get_item(key);
        if (info_ctrl) {
            DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type_detail=0x%x, cmd_i=%d, start to pre check\n",
                key_to_name(DFD_CFG_ITEM_PRE_CHECK_BMC_SYSTEM), type_detail, cmd_i);
            ret = dfd_info_get_int(key, &tmp_value, NULL);
            if (ret < 0) {
                DFD_SYSTEM_DEBUG(DBG_ERROR, "key_name=%s, type_detail=0x%x, cmd_i=%d, get pre check value error, ret: %d\n",
                    key_to_name(DFD_CFG_ITEM_PRE_CHECK_BMC_SYSTEM), type_detail, cmd_i, ret);
                return ret;
            }
            if (tmp_value != info_ctrl->int_extra1) {
                DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type_detail=0x%x, cmd_i=%d, pre check error, get value: %d, except value: %d, skip this step\n",
                    key_to_name(DFD_CFG_ITEM_PRE_CHECK_BMC_SYSTEM), type_detail, cmd_i, tmp_value, info_ctrl->int_extra1);
                continue;
            }
            DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type_detail=0x%x, cmd_i=%d, pre check ok, get value: %d, except value: %d\n",
                key_to_name(DFD_CFG_ITEM_PRE_CHECK_BMC_SYSTEM), type_detail, cmd_i, tmp_value, info_ctrl->int_extra1);
        }
        /* get current step cfg */
        key = DFD_CFG_KEY(DFD_CFG_ITEM_BMC_SYSTEM, type_detail, cmd_i);
        info_ctrl = dfd_ko_cfg_get_item(key);
        if (info_ctrl == NULL) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "get info ctrl fail, key_name=%s, type_detail=0x%x, cmd_i=%d\n",
                key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type_detail, cmd_i);
            return -DFD_RV_DEV_NOTSUPPORT;
        }

        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "set, key_name=%s, type_detail=0x%x, cmd_i=%d\n",
            key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type_detail, cmd_i);
        /* set int type info */
        ret = dfd_info_set_int(key, info_ctrl->int_cons);
        if (ret < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "set system value error, key_name=%s, type_detail=0x%x, cmd_i=%d, value=%d, ret:%d\n",
                key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type_detail, cmd_i, value, ret);
            return ret;
        }

        /* delay if it has */
        if(info_ctrl->int_extra1 > 0) {
            dfd_cmd_delay(info_ctrl->int_extra1);
        }

        /* check value */
        ret = dfd_system_check_value_i(type_detail, cmd_i);
        if (ret < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "set system value check value error, ret: %d\n", ret);
            return ret;
        }
    }

    return DFD_RV_OK;
}

ssize_t dfd_system_get_port_power_status(unsigned int type, char *buf, size_t count)
{
    int ret, cmd_i, cmd_count;
    unsigned int type_detail;

    /* get step number */
    type_detail = type;
    ret = dfd_get_cmd_count(type_detail);
    if(ret <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get cmd number, type_detail=0x%x\n", type_detail);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    cmd_count = ret;
    /* exec each step */
    for(cmd_i = 0; cmd_i < cmd_count; cmd_i++) {
        /* check value */
        ret = dfd_system_check_value_i(type_detail, cmd_i);
        if (ret < 0) {
            if(ret == -DFD_RV_CHECK_FAIL) {
                return (ssize_t)snprintf(buf, count, "%d\n", WB_PORT_POWER_ON);
            }
            DFD_SYSTEM_DEBUG(DBG_ERROR, "set system value check value error, ret: %d\n", ret);
            return ret;
        }
    }

    return (ssize_t)snprintf(buf, count, "%d\n", WB_PORT_POWER_OFF);
}
