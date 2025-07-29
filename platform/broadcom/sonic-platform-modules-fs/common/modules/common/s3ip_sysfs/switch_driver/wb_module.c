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

int g_dfd_dbg_level = 0;   /* Debug level */
module_param(g_dfd_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * wb_dev_cfg_init - dfd module initialization
 *
 * @returns:<0 Failed, otherwise succeeded
 */
int32_t wb_dev_cfg_init(void)
{
    return dfd_dev_cfg_init(false);
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

