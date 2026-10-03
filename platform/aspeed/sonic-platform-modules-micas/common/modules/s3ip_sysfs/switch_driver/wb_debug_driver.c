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
 *   *  v1.0          support                  2025-07-17        Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "wb_fan_driver.h"
#include "wb_psu_driver.h"
#include "wb_sff_driver.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_sensors_driver.h"
#include "wb_pcie_driver.h"
#include "wb_avs_driver.h"
#include "wb_debug_driver.h"
#include "wb_disk_driver.h"
#include "wb_md_driver.h"

int g_dfd_debug_dbg_level = 0;
module_param(g_dfd_debug_dbg_level, int, S_IRUGO | S_IWUSR);

dfd_debug_data_key_map_t *all_dbg_key_tables[DFD_DEBUG_END] = {
    [DFD_DEBUG_VOL]            = vol_dbg_key_table,
    [DFD_DEBUG_CURR]           = curr_dbg_key_table,
    [DFD_DEBUG_FAN]            = fan_dbg_key_table,
    [DFD_DEBUG_PSU]            = psu_status_dbg_key_table,
    [DFD_DEBUG_PSU_SENSOR]     = psu_sensor_dbg_key_table,
    [DFD_DEBUG_SFF]            = sff_dbg_key_table,
    [DFD_DEBUG_TEMP]           = temp_dbg_key_table,
    [DFD_DEBUG_PCIE]           = pcie_dbg_key_table,
    [DFD_DEBUG_AVS]            = avs_dbg_key_table,
    [DFD_DEBUG_DISK]           = disk_dbg_key_table,
    [DFD_DEBUG_MD]             = md_dbg_key_table,
};
size_t all_dbg_key_table_size[DFD_DEBUG_END] = {
    [DFD_DEBUG_VOL]            = WB_SENSOR_END,
    [DFD_DEBUG_CURR]           = WB_SENSOR_END,
    [DFD_DEBUG_FAN]            = WB_FAN_ATTR_END,
    [DFD_DEBUG_PSU]            = DFD_PSU_STATUS_END,
    [DFD_DEBUG_PSU_SENSOR]     = PSU_SENSOR_TYPE_END,
    [DFD_DEBUG_SFF]            = WB_SFF_ATTR_END,
    [DFD_DEBUG_TEMP]           = WB_SENSOR_END,
    [DFD_DEBUG_PCIE]           = DFD_PCIE_MAX_E,
    [DFD_DEBUG_AVS]            = DFD_AVS_MAX_E,
    [DFD_DEBUG_DISK]           = DFD_DISK_MAX_E,
    [DFD_DEBUG_MD]             = DFD_MD_MAX_E,
};

static dfd_debug_data_key_map_t *dfd_get_debug_data_key_map(debug_data_dev_class_t dev_class, int attr_type)
{
    if (dev_class < 0 || dev_class >= DFD_DEBUG_END) {
        DBG_DEBUG_DEBUG(DBG_ERROR, "dev_class out of range, dev_class: %d  attr_type %d.\n", dev_class, attr_type);
        return NULL;
    }
    if (attr_type < 0 || attr_type >= all_dbg_key_table_size[dev_class]) {
        DBG_DEBUG_DEBUG(DBG_ERROR, "attr_type out of range , dev_class: %d  attr_type %d.\n", dev_class, attr_type);
        return NULL;
    }
    if (all_dbg_key_tables[dev_class] == NULL) {
        DBG_DEBUG_DEBUG(DBG_ERROR, "get_key_map fail, dev_class: %d  attr_type %d.\n", dev_class, attr_type);
        return NULL;
    }

    return &all_dbg_key_tables[dev_class][attr_type];
}


/**
 * dfd_set_common_debug_data - Set the vol test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @sensor_index:The index of the sensor
 * @sensor_attr:Such as max ,min ,alias ,value and so on
 * @key:debug key
 * @index_type:index combine way
 * @value: Writes the value of the test register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
static int dfd_set_common_debug_data(unsigned int main_dev_id, unsigned int dev_index, unsigned int sensor_index,
        int sensor_attr, uint64_t key_prefix, wb_index_type_t index_type, debug_data_type_t data_type, const void *val)
{
    uint64_t key;
    int ret;
    uint16_t key_index1;
    uint8_t key_index2;

    switch (index_type){
    case CFG_NO_INDEX:
        key = DFD_CFG_KEY(key_prefix, 0, 0);
        break;
    case CFG_INDEX1_ONLY:
        key = DFD_CFG_KEY(key_prefix, sensor_index, 0);
        break;
    case CFG_2INDEXES_1:
        key = DFD_CFG_KEY(key_prefix, main_dev_id, sensor_index);
        break;
    case CFG_2INDEXES_2:
        key = DFD_CFG_KEY(key_prefix, sensor_index, sensor_attr);
        break;
    case CFG_INDEX1_INDEX2_CMB_1:
        key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, sensor_index);
        key_index2 = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, sensor_attr);
        key = DFD_CFG_KEY(key_prefix, key_index1, key_index2);
        break;
    default:
        DBG_DEBUG_DEBUG(DBG_ERROR, "unsupport index_type %d, main_dev_id: %u, dev_index id %u,set sensor id %u  key_name: %s\n",
            index_type, main_dev_id, dev_index, sensor_index,  key_to_name(key_prefix));
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_dev_set_debug_data((void *)val, key, data_type);
    if (ret < 0) {
        DBG_DEBUG_DEBUG(DBG_ERROR, "main_dev_id: %u, dev_index id %u, set sensor id %u debug data error, key_name: %s, ret:%d\n",
            main_dev_id, dev_index, sensor_index, key_to_name(key_prefix), ret);
        return ret;
    }

    return DFD_RV_OK;
}

/**
 * dfd_debug_data_common_attr - Set the common debug value
 * @dev_class: DFD_DEBUG_VOL. DFD_DEBUG_CURR
 * @attr_type: e.g :max, min, value, high_max
 * @sensor_index:The number of the sensor
 * @sensor_attr:Such as max ,min ,alias ,value and so on
 * @value: Writes the value of the debug_data
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_debug_data_common_attr(wb_main_dev_type_t main_dev_type, wb_minor_dev_type_t minor_dev_type,
    debug_data_dev_class_t dev_class, unsigned int attr_type, unsigned int sensor_index, const char *value)
{
    int ret;
    dfd_debug_data_key_map_t *key_map;

    key_map = dfd_get_debug_data_key_map(dev_class, attr_type);
    if (key_map == NULL) {
        DBG_DEBUG_DEBUG(DBG_ERROR, "get_key_map null, dev_class: %d  attr_type %d.\n", dev_class, attr_type);
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    if (key_map->key_prefix == 0) {
        DBG_DEBUG_DEBUG(DBG_ERROR, "key prefix is none, dev_class: %d  attr_type %d.\n", dev_class, attr_type);
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    ret = dfd_set_common_debug_data(main_dev_type, minor_dev_type,
        sensor_index, attr_type, key_map->key_prefix, key_map->index_type, key_map->data_type, (void *)value);
    if (ret < 0) {
        DBG_DEBUG_DEBUG(DBG_ERROR, "dfd_debug_data_common_attr fail, dev_class: %d  attr_type %d.\n", dev_class, attr_type);
        return ret;
    }

    return ret;
}
