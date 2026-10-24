/*
 * Copyright(C) 2001-2025 whitebox Network. All rights reserved.
 */
/*
 * wb_gcu_driver.c
 * Original Author: [Your Name] [Date]
 *
 * GCU (GPU Computing Unit) related properties read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0                                    2025-09-18        Initial version
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/limits.h>

#include "wb_gcu_driver.h"
#include "wb_gcu_l600_l900_driver.h"

int g_dfd_gcu_dbg_level = 0x0;
module_param(g_dfd_gcu_dbg_level, int, S_IRUGO | S_IWUSR);

gcu_id_info_t gcu_id_info_table[GCU_MAX_NUMBER] = {
    [0 ... GCU_MAX_NUMBER-1] = { -1, -1, -1}
};

gcu_list_t gcu_list[WB_MAIN_GCU_DEV_MAX] = {
    {"enflame_L600", WB_MAIN_GCU_DEV_ENFLAME_L600,
            0xa0, 0xa1, 2, 0x1e36, 0xc042,
            enflame_L600_L900_gcu_attr_table,
            enflame_L600_L900_gcu_temp_attr_table, 2,
            enflame_L600_L900_gcu_vol_attr_table, 2,
            enflame_L600_L900_gcu_power_attr_table, 2,
            enflame_l600_900_health_rules, 3,
            enflame_L600_maps, L600_MAPPING_COUNT,
            l600_l900_gcu_func_table, DFD_GCU_DEVICE_TYPE_MAX_E,
            l600_l900_gcu_temp_func_table, DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E,
            l600_l900_gcu_vol_func_table, DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E,
            l600_l900_gcu_power_func_table, DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E,
    },
    {"enflame_L900", WB_MAIN_GCU_DEV_ENFLAME_L900,
        0xa0, 0xa1, 2, 0x1e36, 0xc043,
        enflame_L600_L900_gcu_attr_table,
        enflame_L600_L900_gcu_temp_attr_table, 2,
        enflame_L600_L900_gcu_vol_attr_table, 2,
        enflame_L600_L900_gcu_power_attr_table, 2,
        enflame_l600_900_health_rules, 3,
        enflame_L900_maps, L900_MAPPING_COUNT,
        l600_l900_gcu_func_table, DFD_GCU_DEVICE_TYPE_MAX_E,
        l600_l900_gcu_temp_func_table, DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E,
        l600_l900_gcu_vol_func_table, DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E,
        l600_l900_gcu_power_func_table, DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E,
    }
};

static int find_map_by_name(unsigned int gcu_type, char *map_name)
{
    int i;

    if (!map_name || gcu_type >= WB_MAIN_GCU_DEV_MAX) {
        return -DFD_RV_INVALID_VALUE;
    }

    for (i = 0; i < gcu_list[gcu_type].gcu_map_number; i++) {
        if (strcmp(gcu_list[gcu_type].gcu_map[i].name, map_name) == 0) {
            return i;
        }
    }

    DBG_GCU_DEBUG(DBG_ERROR, "find_map_by_name failed, nothing matched.\n");

    return -DFD_RV_INVALID_VALUE;
}

static int format_and_copy_to_buf_reverse(char *buf, ssize_t buf_size, uint8_t *val, int valid_size, gcu_base_type mode)
{
    int i, count, temp;
    unsigned long decimal_val;
    int ret;
    char *ptr;
    char temp_buf[INFO_INT_MAX_LEN];

    DBG_GCU_DEBUG(DBG_VERBOSE, "format_and_copy_to_buf_reverse called.\n");

    if (buf == NULL || val == NULL || valid_size <= 0 || buf_size <= 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "Invalid parameters\n");
        return -DFD_RV_INVALID_VALUE;
    }

    ptr = buf;

    if (mode & ASCII_BASE) {
        DBG_GCU_DEBUG(DBG_VERBOSE, "mode is ASCII_BASE.\n");
        for (i = 0; i < valid_size / 2; i++) {
            temp = val[i];
            val[i] = val[valid_size - 1 - i];
            val[valid_size - 1 - i] = temp;
        }

        if (valid_size > 0) {
            val[valid_size] = '\0';
        }

        if ((valid_size + 2) > buf_size) {
            DBG_GCU_DEBUG(DBG_ERROR, "Buffer overflow\n");
            return -DFD_RV_INVALID_VALUE;
        }

        return snprintf(buf, valid_size + 2, "%s\n", val);
    }

    count = valid_size * 2 + 2;
    if (count > buf_size) {
        return -DFD_RV_INVALID_VALUE;
    }

    for (i = 0; i < valid_size; i++) {
        ret = snprintf(ptr, count, "%02x", val[valid_size - i - 1]);
        if (ret < 0 || ret >= count) {
            DBG_GCU_DEBUG(DBG_ERROR, "Buffer overflow or error occurred\n");
            return -DFD_RV_INVALID_VALUE;
        }

        ptr += 2;
        count -= 2;
    }

    while (1) {
        if (buf[0] != '0') {
            break;
        }

        if (strlen(buf) == 1 && buf[0] == '0') {
            break;
        }

        memmove(buf, buf + 1, strlen(buf) + 1);
    }

    DBG_GCU_DEBUG(DBG_VERBOSE, "buf is  %s.\n", buf);
    if (mode & DECIMAL_BASE) {
        DBG_GCU_DEBUG(DBG_VERBOSE, "mode is  DECIMAL_BASE.\n");
        if (kstrtoul(buf, 16, &decimal_val) != 0) {
            DBG_GCU_DEBUG(DBG_ERROR, "kstrtoul failed, buf: %s\n", buf);
            return -DFD_RV_INVALID_VALUE;
        }

        if (mode & X1000_BASE) {
            if (decimal_val > ULONG_MAX / 1000) {
                DBG_GCU_DEBUG(DBG_ERROR, "Reverse function multiplication overflow: %lu * 1000\n", decimal_val);
                return -DFD_RV_INVALID_VALUE;
            }
            decimal_val *= 1000;
        } else if (mode & X1000000_BASE) {
            if (decimal_val > ULONG_MAX / 1000000) {
                DBG_GCU_DEBUG(DBG_ERROR, "Reverse function multiplication overflow: %lu * 1000000\n", decimal_val);
                return -DFD_RV_INVALID_VALUE;
            }
            decimal_val *= 1000000;
        }

        return snprintf(buf, buf_size, "%lu\n", decimal_val);
    } else if (mode & HEX_BASE) {
        ret = snprintf(temp_buf, sizeof(temp_buf), "0x%s\n", buf);
        if (ret < 0) {
            DBG_GCU_DEBUG(DBG_ERROR, "Buffer overflow or error occurred\n");
            return -DFD_RV_INVALID_VALUE;
        }

        return snprintf(buf, sizeof(temp_buf), "%s", temp_buf);
    }

    return -DFD_RV_INVALID_VALUE;
}

static int format_and_copy_to_buf_forward(char *buf, ssize_t buf_size, uint8_t *val, int valid_size, gcu_base_type mode)
{
    int i, count;
    unsigned long decimal_val;
    int ret;
    char *ptr;

    if (buf == NULL || val == NULL || valid_size <= 0 || buf_size <= 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "Invalid parameters\n");
        return -DFD_RV_INVALID_VALUE;
    }

    ptr = buf;
    decimal_val = 0;

    if (mode & ASCII_BASE) {
        if ((valid_size + 2) > buf_size) {
            DBG_GCU_DEBUG(DBG_ERROR, "Buffer overflow\n");
            return -DFD_RV_INVALID_VALUE;
        }
        return snprintf(buf, valid_size + 2, "%.*s\n", valid_size, val);
    } else if (mode & DECIMAL_BASE) {
        if (valid_size > sizeof(decimal_val)) {
            DBG_GCU_DEBUG(DBG_ERROR, "valid_size exceeds capacity of decimal_val: %d vs %zu\n",
                 valid_size, sizeof(decimal_val));
            return -DFD_RV_INVALID_VALUE;
        }

        for (i = 0; i < valid_size; i++) {
            decimal_val = (decimal_val << 8) | val[i];
        }

        if (mode & X1000_BASE) {
            if (decimal_val > ULONG_MAX / 1000) {
                DBG_GCU_DEBUG(DBG_ERROR, "Multiplication overflow: %lu * 1000\n", decimal_val);
                return -DFD_RV_INVALID_VALUE;
            }
            decimal_val *= 1000;
        } else if (mode & X1000000_BASE) {
            if (decimal_val > ULONG_MAX / 1000000) {
                DBG_GCU_DEBUG(DBG_ERROR, "Multiplication overflow: %lu * 1000000\n", decimal_val);
                return -DFD_RV_INVALID_VALUE;
            }
            decimal_val *= 1000000;
        }

        return snprintf(buf, buf_size, "%lu\n", decimal_val);
    } else if (mode & HEX_BASE) {
        count = valid_size * 2 + 2;
        if (count > buf_size) {
            return -DFD_RV_INVALID_VALUE;
        }

        for (i = 0; i < valid_size; i++) {
            ret = snprintf(ptr, count, "%02x", val[i]);
            if (ret < 0 || ret >= count) {
                DBG_GCU_DEBUG(DBG_ERROR, "Buffer overflow or error occurred\n");
                return -DFD_RV_INVALID_VALUE;
            }

            ptr += 2;
            count -= 2;
        }
    } else {
        DBG_GCU_DEBUG(DBG_ERROR, "invalid mode.\n");
        return -EINVAL;
    }

    return strlen(buf);
}

static int format_and_copy_to_buf_mapping(char *buf, uint8_t *val, int valid_size, unsigned int gcu_type, unsigned int map_index)
{
    int i;

    if (map_index >= gcu_list[gcu_type].gcu_map_number || map_index < 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "gcu map_index is invalid.\n");
         return -DFD_RV_INVALID_VALUE;
    }

    for (i = 0; i < gcu_list[gcu_type].gcu_map[map_index].map_size; i++) {
        if (val[0] == gcu_list[gcu_type].gcu_map[map_index].map[i].reg_value) {
            /* match */
            return snprintf(buf, valid_size,
                "%s\n", gcu_list[gcu_type].gcu_map[map_index].map[i].str);
        }
    }

    DBG_GCU_DEBUG(DBG_ERROR, "gcu mapping nothing match.\n");
    return -DFD_RV_INVALID_VALUE;
}

int data_to_buf(char *buf, ssize_t buf_size, uint8_t *val, int valid_size, unsigned int gcu_type, unsigned int mode, char *map_name)
{
    int map_index;

    if (buf == NULL || val == NULL || valid_size <= 0 || buf_size <= 0 || valid_size >= buf_size) {
        DBG_GCU_DEBUG(DBG_ERROR, "Invalid parameters\n");
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_GCU_DEBUG(DBG_VERBOSE, "valid_size is %d, gcu_type: %d, mode is 0x%x.\n",
        valid_size, gcu_type, mode);
    switch (mode & ORDER_DIRECTION_MASK) {
        case REVERSE_BASE:
            return format_and_copy_to_buf_reverse(buf, buf_size, val, valid_size, mode);
        case FORWARD_BASE:
            return format_and_copy_to_buf_forward(buf, buf_size, val, valid_size, mode);
        default:
            break;
    }

    if ((mode & MAPPING_BASE) && ((mode & ORDER_DIRECTION_MASK) == 0)) {
        if (map_name == NULL) {
            DBG_GCU_DEBUG(DBG_ERROR, "Invalid map_name\n");
            return -DFD_RV_INVALID_VALUE;
        }

        map_index = find_map_by_name(gcu_type, map_name);
        if (map_index < 0) {
            DBG_GCU_DEBUG(DBG_ERROR, "find_map_by_name failed\n");
            return -DFD_RV_INVALID_VALUE;
        }

        return format_and_copy_to_buf_mapping(buf, val, valid_size, gcu_type, map_index);
    }

    return -DFD_RV_INVALID_VALUE;
}

static uint64_t build_default_key(gcu_key_mode_t mode,
                                    uint64_t cfg_item_type,
                                    int gcu_type,
                                    unsigned int main_dev_id,
                                    unsigned int sub_dev_id,
                                    unsigned int sensor_index)
{
    if (mode == GCU_KEY_MODE_SENSOR) {
        /* index1 is gcutype(ffff)gcuid(ff)_functype(ff)tempid(ff) */
        return DFD_CFG_KEY(cfg_item_type,
                           DFD_GET_GCU_KEY1(gcu_list[gcu_type].gcu_main_id, main_dev_id),
                           DFD_GET_GCU_KEY2(sub_dev_id, sensor_index));
    } else {
        /* index1 is gcutype(ffff)_gcuid(ff)functype(ff) */
        return DFD_CFG_KEY(cfg_item_type,
                           gcu_list[gcu_type].gcu_main_id,
                           DFD_GET_GCU_COMMON_KEY2(main_dev_id, sub_dev_id));
    }
}

static int get_gcu_para_val_generic(unsigned int main_dev_id, unsigned int sub_dev_id,
                                    unsigned int sensor_index, int gcu_type,
                                    char *val, char *buf, gcu_reg_attr_t func_attr,
                                    unsigned int count, uint64_t cfg_item_type,
                                    gcu_key_mode_t mode)
{
    int ret, bus_num;
    uint64_t key;
    info_ctrl_t *info_ctrl;

    key = build_default_key(mode, cfg_item_type, gcu_type, main_dev_id, sub_dev_id, sensor_index);
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_GCU_DEBUG(DBG_VERBOSE, "%s", val);
        return snprintf(buf, count, "%s", val);
    }

    DBG_GCU_DEBUG(DBG_VERBOSE,
        "main_dev_id: %u, sub_dev_id %u default config not exit, key_name: %s, begin to get from i2c.\n",
        main_dev_id, sub_dev_id, key_to_name(cfg_item_type));

    key = DFD_CFG_KEY(DFD_CFG_ITEM_GCU_DEVICE, gcu_list[gcu_type].gcu_main_id,
                      DFD_GET_GCU_COMMON_KEY2(main_dev_id, DFD_GCU_BUS_NUM));

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_GCU_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    bus_num = dfd_ko_extract_i2c_bus_number(info_ctrl->fpath);
    if (bus_num < 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "get i2c bus number fail, main_dev_id: %u, sub_dev_id: %u\n",
                      main_dev_id, sub_dev_id);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    ret = dfd_ko_i2c_dev_smbus_read(bus_num,
                                    info_ctrl->addr,
                                    func_attr.offset,
                                    val, func_attr.size);
    if (ret < 0) {
        DBG_GCU_DEBUG(DBG_ERROR,
            "dfd_ko_i2c_dev_smbus_read failed, bus: %d, addr: 0x%x, offset: 0x%x, ret: %d\n",
            bus_num, info_ctrl->addr, func_attr.offset, ret);
        return ret;
    }

    return 0;
}

static int get_gcu_para_sensor_val_generic(unsigned int main_dev_id, unsigned int sub_dev_id,
                                    unsigned int sensor_index, int gcu_type,
                                    char *val, char *buf, gcu_reg_attr_t func_attr,
                                    unsigned int count, uint64_t cfg_item_type,
                                    gcu_key_mode_t mode)
{
    int ret, bus_num;
    uint64_t key;
    info_ctrl_t *info_ctrl;

    key = build_default_key(mode, cfg_item_type, gcu_type, main_dev_id, sub_dev_id, sensor_index);
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_GCU_DEBUG(DBG_VERBOSE, "%s", val);
        return snprintf(buf, count, "%s", val);
    } else if (IS_TARGET_SENSOR_STATIC(sub_dev_id)) {
        DBG_GCU_DEBUG(DBG_VERBOSE, "key %llx, mode %x gcu_type %x main_dev_id %x sub_dev_id %x sensor_index %x", key, mode, gcu_type, main_dev_id, sub_dev_id, sensor_index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_GCU_DEBUG(DBG_VERBOSE,
        "main_dev_id: %u, sub_dev_id %u default config not exit, key_name: %s, begin to get from i2c.\n",
        main_dev_id, sub_dev_id, key_to_name(cfg_item_type));

    key = DFD_CFG_KEY(DFD_CFG_ITEM_GCU_DEVICE, gcu_list[gcu_type].gcu_main_id,
                      DFD_GET_GCU_COMMON_KEY2(main_dev_id, DFD_GCU_BUS_NUM));

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_GCU_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    bus_num = dfd_ko_extract_i2c_bus_number(info_ctrl->fpath);
    if (bus_num < 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "get i2c bus number fail, main_dev_id: %u, sub_dev_id: %u\n",
                      main_dev_id, sub_dev_id);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    ret = dfd_ko_i2c_dev_smbus_read(bus_num,
                                    info_ctrl->addr,
                                    func_attr.offset,
                                    val, func_attr.size);
    if (ret < 0) {
        DBG_GCU_DEBUG(DBG_ERROR,
            "dfd_ko_i2c_dev_smbus_read failed, bus: %d, addr: 0x%x, offset: 0x%x, ret: %d\n",
            bus_num, info_ctrl->addr, func_attr.offset, ret);
        return ret;
    }

    return 0;
}

int get_gcu_para_val(unsigned int main_dev_id, unsigned int sub_dev_id,
                          unsigned int sensor_index, int gcu_type,
                          char *val, char *buf, gcu_reg_attr_t func_attr, unsigned int count)
{
    return get_gcu_para_val_generic(main_dev_id, sub_dev_id, sensor_index, gcu_type,
                                 val, buf, func_attr, count,
                                 DFD_CFG_ITEM_GCU_DEVICE, GCU_KEY_MODE_COMMON);
}

int get_gcu_temp_para_val(unsigned int main_dev_id, unsigned int sub_dev_id,
                               unsigned int sensor_index, int gcu_type,
                               char *val, char *buf, gcu_reg_attr_t func_attr, unsigned int count)
{
    return get_gcu_para_sensor_val_generic(main_dev_id, sub_dev_id, sensor_index, gcu_type,
                                 val, buf, func_attr, count,
                                 DFD_CFG_ITEM_GCU_DEVICE_TEMP, GCU_KEY_MODE_SENSOR);
}

int get_gcu_vol_para_val(unsigned int main_dev_id, unsigned int sub_dev_id,
                               unsigned int sensor_index, int gcu_type,
                               char *val, char *buf, gcu_reg_attr_t func_attr, unsigned int count)
{
    return get_gcu_para_sensor_val_generic(main_dev_id, sub_dev_id, sensor_index, gcu_type,
                                 val, buf, func_attr, count,
                                 DFD_CFG_ITEM_GCU_DEVICE_VOL, GCU_KEY_MODE_SENSOR);
}

int get_gcu_power_para_val(unsigned int main_dev_id, unsigned int sub_dev_id,
                               unsigned int sensor_index, int gcu_type,
                               char *val, char *buf, gcu_reg_attr_t func_attr, unsigned int count)
{
    return get_gcu_para_sensor_val_generic(main_dev_id, sub_dev_id, sensor_index, gcu_type,
                                 val, buf, func_attr, count,
                                 DFD_CFG_ITEM_GCU_DEVICE_POWER, GCU_KEY_MODE_SENSOR);
}

ssize_t dfd_get_gcu_alias(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, gcu_type;
    uint64_t key;

    if (buf == NULL || main_dev_id < 1 || main_dev_id > GCU_MAX_NUMBER) {
        DBG_GCU_DEBUG(DBG_ERROR, "param error, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_GCU_DEBUG(DBG_VERBOSE, "dfd_get_gcu_alias has been called main_dev_id:0x%x sub_dev_id:0x%x.\n", main_dev_id, sub_dev_id);
    mem_clear(buf, count);
    mem_clear(val, INFO_INT_MAX_LEN + 1);

    gcu_type = gcu_id_info_table[main_dev_id - 1].gcu_type;
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX || sub_dev_id > DFD_GCU_DEVICE_TYPE_MAX_E - 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, gcu_type %d.\n",
            main_dev_id, sub_dev_id, gcu_type);

        return -EINVAL;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_GCU_DEVICE, gcu_list[gcu_type].gcu_main_id,
            DFD_GET_GCU_KEY2(main_dev_id, sub_dev_id));
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret < 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "dfd_get_value_from_info failed, main_dev_id: %u, sub_dev_id: %u, ret: %d\n",
            main_dev_id, sub_dev_id, ret);
        return ret;
    }

    snprintf(buf, count, "%s", val);

    return strlen(buf);
}

ssize_t dfd_get_gcu_attr_common(unsigned int main_dev_id, unsigned int sub_dev_id,
                                      char *buf, size_t count, char *map_table_name)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, gcu_type;

    if (buf == NULL || main_dev_id < 1 || main_dev_id > GCU_MAX_NUMBER) {
        DBG_GCU_DEBUG(DBG_ERROR, "param error, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_GCU_DEBUG(DBG_VERBOSE, "main_dev_id:0x%x sub_dev_id:0x%x.\n", main_dev_id, sub_dev_id);

    mem_clear(buf, count);
    mem_clear(val, INFO_INT_MAX_LEN + 1);

    gcu_type = gcu_id_info_table[main_dev_id - 1].gcu_type;
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX || sub_dev_id > DFD_GCU_DEVICE_TYPE_MAX_E - 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, gcu_type %d.\n",
            main_dev_id, sub_dev_id, gcu_type);
        return -EINVAL;
    }

    if (gcu_list[gcu_type].attr_table[sub_dev_id].size > count) {
        DBG_GCU_DEBUG(DBG_ERROR, "Size is too large, gcu_type: %d, sub_dev_id: %u, size: %d count: %zu.\n",
            gcu_type, sub_dev_id, gcu_list[gcu_type].attr_table[sub_dev_id].size, count);
        return -EINVAL;
    }

    ret = get_gcu_para_val(main_dev_id, sub_dev_id, 0, gcu_type, val, buf,
                         gcu_list[gcu_type].attr_table[sub_dev_id], count);
    if (ret < 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "get_gcu_para_val failed, main_dev_id: %u, sub_dev_id: %u, ret: %d\n",
            main_dev_id, sub_dev_id, ret);
        return ret;
    }

    if (ret > 0) {
        return strlen(buf);
    }

    return data_to_buf(buf, count, val,
        gcu_list[gcu_type].attr_table[sub_dev_id].size,
        gcu_type, gcu_list[gcu_type].attr_table[sub_dev_id].base,
        map_table_name);
}

int dfd_check_gcu_type_is_valid(unsigned int main_dev_id, char *buf)
{
    int gcu_type;

    if (buf == NULL || main_dev_id < 1 || main_dev_id > GCU_MAX_NUMBER) {
        DBG_GCU_DEBUG(DBG_ERROR, "param error, main_dev_id: %u\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    gcu_type = gcu_id_info_table[main_dev_id - 1].gcu_type;
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX) {
        DBG_GCU_DEBUG(DBG_ERROR, "main_dev_id: %u config error, gcu_type %d.\n", main_dev_id,  gcu_type);
        return -EINVAL;
    }

    return gcu_type;
}

int find_gpu_type_via_single_i2c(unsigned int slot) {
    info_ctrl_t *info_ctrl;
    uint64_t key;
    uint32_t bus_num;
    uint16_t value;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    const char *pos;
    int rv, i;

    for (i = 0; i < WB_MAIN_GCU_DEV_MAX; i++) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_GCU_DEVICE, gcu_list[i].gcu_main_id,
                DFD_GET_GCU_KEY2(slot, DFD_GCU_BUS_NUM));

        /* Entry check */
        if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key))) {
            DBG_GCU_DEBUG(DBG_ERROR, "input arguments error, key: 0x%08llx\n", key);
            continue;
        }

        info_ctrl = dfd_ko_cfg_get_item(key);
        if (info_ctrl == NULL) {
            DBG_GCU_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n gcu_list[i].gcu_main_id: 0x%x DFD_GET_GCU_KEY2(slot, DFD_GCU_BUS_NUM): 0x%x",
                    key, gcu_list[i].gcu_main_id, DFD_GET_GCU_KEY2(slot, DFD_GCU_BUS_NUM));
            continue;
        }

        if (info_ctrl->mode != INFO_CTRL_MODE_CFG || info_ctrl->src != INFO_SRC_FILE) {
            continue;
        }

        pos = strstr(info_ctrl->fpath, "i2c-");
        if (pos == NULL ) {
            continue;
        }

        DBG_GCU_DEBUG(DBG_ERROR, "find_gpu_type_via_single_i2c pos is %s.\n", pos);
        if(kstrtouint(pos + 4, 10, &bus_num)) {
            DBG_GCU_DEBUG(DBG_ERROR, "find_gpu_type_via_single_i2c kstrtouint failed.\n");
            continue;
        }

        DBG_GCU_DEBUG(DBG_VERBOSE, "find gcu bus num, bus num is %d\n", bus_num);

        /* Read vendor ID */
        rv = dfd_ko_i2c_dev_smbus_read(bus_num,
                info_ctrl->addr,
                gcu_list[i].ven_addr,
                val, 2);

        if (rv < 0) {
            DBG_GCU_DEBUG(DBG_VERBOSE, "bus num %d slave addr 0x%x get vendor id(0x%x) failed.\n", bus_num, info_ctrl->addr, gcu_list[i].ven_addr);
            continue;
        }

        DBG_GCU_DEBUG(DBG_VERBOSE, "Read vendor ID success val[0]:0x%x  val[1]:0x%x. \n", val[0], val[1]);

        value = (val[1] << 8) | val[0];
        if (gcu_list[i].vendor_id != value) {
            continue;
        }

        /* Read device ID */
        rv = dfd_ko_i2c_dev_smbus_read(bus_num,
                info_ctrl->addr,
                gcu_list[i].dev_addr,
                val, 2);

        if (rv < 0) {
            DBG_GCU_DEBUG(DBG_VERBOSE, "bus num %d slave addr 0x%x get device id(0x%x) failed.\n", bus_num, info_ctrl->addr, gcu_list[i].dev_addr);
            continue;
        }

        DBG_GCU_DEBUG(DBG_VERBOSE, "Read device ID success val[0]:0x%x  val[1]:0x%x. \n", val[0], val[1]);
        value = (val[1] << 8) | val[0];
        if (gcu_list[i].device_id  != value) {
            continue;
        }

        return i;
    }

    return -ENOMEM;
}
