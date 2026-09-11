/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_module.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * dfd kernel code, DM-independent code extracted
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 *   *  v1.1    sonic_rd@whitebox         2021-08-26          S3IP sysfs
 */
#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "wb_system_driver.h"

int g_dfd_dbg_level = 0;   /* Debug level */
module_param(g_dfd_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * wb_dev_cfg_init - dfd module initialization
 *
 * @returns:<0 Failed, otherwise succeeded
 */
int32_t wb_dev_cfg_init(void)
{
    return dfd_dev_cfg_init(true);
}

/**
 * wb_dev_cfg_exit - dfd module exit
 *
 * @returns: void
 */

void wb_dev_cfg_exit(void)
{
    dfd_dev_cfg_exit();
    return;
}

/**
 * dfd_get_dev_number - Get the number of devices
 * @main_dev_id:Master device number
 * @minor_dev_id:Secondary device number
 * @returns: <0 failed, otherwise number of devices is returned
 */
int dfd_get_dev_number(unsigned int main_dev_id, unsigned int minor_dev_id)
{
    uint64_t key;
    int dev_num;
    int *p_dev_num;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_NUM, main_dev_id, minor_dev_id);
    p_dev_num = dfd_ko_cfg_get_item(key);
    if (p_dev_num == NULL) {
        DBG_DEBUG(DBG_ERROR, "get device number failed, key_name:%s\n",
            key_to_name(DFD_CFG_ITEM_DEV_NUM));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    dev_num = *p_dev_num;
    DBG_DEBUG(DBG_VERBOSE, "get device number ok, number:%d\n",dev_num);
    return dev_num;
}

/**
 * dfd_get_switch_serial_number - Get the serial_number of switch from syseeprom
 */
int dfd_get_switch_serial_number(char *buf, size_t count)
{
    int ret;

    /* Default to use tag name: syseeprom */
    ret = dfd_get_system_info(WB_MAIN_DEV_MAINBOARD, DFD_DEV_INFO_TYPE_SN, buf, count);
    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "get switch serial number failed, ret:%d\n", ret);
        return ret;
    }

    return DFD_RV_OK;
}

/**
 * dfd_get_reg_key - Get the value of reg key
 * @main_dev_id:Master device number
 * @minor_dev_id:Secondary device number
 * @key_index2:key index
 * @key_value:return get key value
 * @returns: <0 failed
 */
int dfd_get_reg_key(unsigned int main_dev_id, unsigned int minor_dev_id,
    int key_index2, int *key_value)
{
    uint64_t key;
    uint16_t key_index1;
    int *p_key_value;

    key_index1 = DFD_SET_BYTES_KEY1(main_dev_id, minor_dev_id);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_REG_KEY, key_index1, key_index2);
    p_key_value = dfd_ko_cfg_get_item(key);
    if (p_key_value == NULL) {
        DBG_DEBUG(DBG_ERROR, "get reg key failed, key_name:%s\n",
            key_to_name(DFD_CFG_ITEM_REG_KEY));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *key_value = *p_key_value;
    DBG_DEBUG(DBG_VERBOSE, "get reg key ok, value:%d\n", *key_value);
    return DFD_RV_OK;
}

/**
 * dfd_get_sysfs_decode - Map to the value of sysfs value
 * @main_dev_id:Master device number
 * @minor_dev_id:Secondary device number
 * @value:need to convert
 * @sysfs_value:sysfs value
 * @returns: <0 failed
 */
int dfd_get_sysfs_decode(unsigned int main_dev_id, unsigned int minor_dev_id,
    int value, int *sysfs_value)
{
    uint64_t key;
    uint16_t index1;
    int *p_sysfs_value;

    index1 = DFD_SET_BYTES_KEY1(main_dev_id, minor_dev_id);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SYSFS_DECODE, index1, value);
    p_sysfs_value = dfd_ko_cfg_get_item(key);
    if (p_sysfs_value == NULL) {
        DBG_DEBUG(DBG_ERROR, "get sysfs decode failed, key_name:%s\n",
            key_to_name(DFD_CFG_ITEM_SYSFS_DECODE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *sysfs_value = *p_sysfs_value;
    DBG_DEBUG(DBG_VERBOSE, "value[0x%x] convert to sysfs value[0x%x]\n", value, *sysfs_value);
    return DFD_RV_OK;
}

dfd_sysfs_get_data_func dfd_get_sysfs_value_func(dfd_sysfs_func_map_t *func_map, unsigned int type, unsigned int max_len)
{
    if (type >= max_len || (func_map == NULL)) {
        DBG_DEBUG(DBG_ERROR, "Invalid type: %d (max: %d), func_map = %p\n", type, max_len - 1, func_map);
        return NULL;
    }

    if (func_map[type].get_func == NULL) {
        DBG_DEBUG(DBG_ERROR, "get_func not implemented\n");
        return NULL;
    }

    return func_map[type].get_func;
}

dfd_sysfs_set_data_func dfd_set_sysfs_value_func(dfd_sysfs_func_map_t *func_map, unsigned int type, unsigned int max_len)
{
    if (type >= max_len || (func_map == NULL)) {
        DBG_DEBUG(DBG_ERROR, "Invalid type: %d (max: %d), func_map = %p\n", type, max_len - 1, func_map);
        return NULL;
    }

    if (func_map[type].set_func == NULL) {
        DBG_DEBUG(DBG_ERROR, "set_func not implemented\n");
        return NULL;
    }

    return func_map[type].set_func;
}

int dfd_set_init_cmd(unsigned int dev_class, unsigned int dev_index, int sysfs_type)
{
    int ret;
    uint64_t all_init_key, single_init_key;
    uint64_t dev_key;
    info_ctrl_t *info_ctrl_all, *info_ctrl_single;

    switch (dev_class) {
    case WB_MAIN_DEV_SFF:
        dev_key = DFD_CFG_ITEM_SFF_INIT_CMD;
        single_init_key = DFD_CFG_KEY(dev_key, dev_index, sysfs_type);
        break;
    default:
        DBG_DEBUG(DBG_ERROR, "unsupport dev[%d]\n", dev_class);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    all_init_key = DFD_CFG_KEY(DFD_CFG_ITEM_INIT_CMD, dev_class, sysfs_type);
    info_ctrl_all = dfd_ko_cfg_get_item(all_init_key);
    info_ctrl_single = dfd_ko_cfg_get_item(single_init_key);
    if (info_ctrl_single != NULL) {
        ret = dfd_info_set_int(single_init_key, info_ctrl_single->int_cons);
        if (ret < 0) {
            DBG_DEBUG(DBG_ERROR, "set value error, key_name=%s, sysfs_type=0x%x, value=%d, ret:0x%x\n",
                key_to_name(dev_key), sysfs_type, info_ctrl_single->int_cons, ret);
            return ret;
        }
    } else if (info_ctrl_all != NULL) {
        ret = dfd_info_set_int(all_init_key, info_ctrl_all->int_cons);
        if (ret < 0) {
            DBG_DEBUG(DBG_ERROR, "set value error, key_name=%s, sysfs_type=0x%x, value=%d, ret:0x%x\n",
                key_to_name(DFD_CFG_ITEM_INIT_CMD), sysfs_type, info_ctrl_all->int_cons, ret);
            return ret;
        }
    } else {
        DBG_DEBUG(DBG_VERBOSE, "not have init ctrl, skip\n");
    }

    return 0;
}

/** dfd_get_dev_debug_config_data - Get debug data of the device
 * @cfg_type: config type
 * @dev_id: device id
 * @dev_type: device type
 */
int dfd_get_dev_debug_config_data(dfd_cfg_item_id_t cfg_type, unsigned int dev_id, unsigned int dev_type, char *buf, size_t count)
{    
    uint64_t key;
    int debug_mode;
    char tmp_str[INFO_BUF_MAX_LEN] = {0};
    char *tmp_str_ptr;
    int ret;

    if (buf == NULL || count == 0) {
        DBG_DEBUG(DBG_ERROR, "param error, buf is NULL or count is 0, dev_id: %u, dev_type: %u\n",
            dev_id, dev_type);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    debug_mode = dfd_ko_check_cfg_node_debug_mode();

    if (debug_mode != NODE_DEBUG_ON) {
        DBG_DEBUG(DBG_VERBOSE, "debug mode is off, can't get debug data from debug node, dev_id: %u, dev_type: %u\n",
            dev_id, dev_type);
        return -DFD_RV_DEBUG_MODE_OFF;
    }

    DBG_DEBUG(DBG_VERBOSE, "debug mode is on, try to get link status from debug node, dev_id: %u, dev_type: %u\n",
        dev_id, dev_type);
    key = DFD_CFG_KEY(cfg_type, dev_id, dev_type);
    ret = dfd_get_value_from_info(key, tmp_str, (int)sizeof(tmp_str));
    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to get value from debug node, key=0x%08llx, ret=%d\n", key, ret);
        return -DFD_RV_NO_NODE;
    }

    tmp_str[sizeof(tmp_str) - 1] = '\0';

    DBG_DEBUG(DBG_VERBOSE, "get value from debug node success, value: %s\n", tmp_str);
    tmp_str_ptr = strstrip(tmp_str);
    if (!tmp_str_ptr) {
        DBG_DEBUG(DBG_ERROR, "strstrip failed, tmp_str: %s\n", tmp_str);
        return -DFD_RV_INVALID_VALUE;
    }
    ret = snprintf(buf, count, "%s\n", tmp_str_ptr);
    if (ret < 0 || (size_t)ret >= count) {
        DBG_DEBUG(DBG_ERROR, "snprintf failed, ret: %d, tmp_str_ptr: %s\n", ret, tmp_str_ptr);
        return -DFD_RV_DEV_FAIL;
    }    
    return strlen(buf);
}