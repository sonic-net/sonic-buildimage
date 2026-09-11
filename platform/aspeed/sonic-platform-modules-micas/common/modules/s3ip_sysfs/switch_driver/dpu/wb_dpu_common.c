/*
 * wb_dpu_common.c
 *
 * DPU related attribute read and write functions (common part)
 */

#include <linux/module.h>
#include <linux/kernel.h>

#include "wb_dpu_driver.h"
#include "wb_dpu_jaguar_driver.h"

int g_dfd_dpu_dbg_level = 0x0;
module_param(g_dfd_dpu_dbg_level, int, S_IRUGO | S_IWUSR);

dpu_device_type_t dpu_id_info_table[DPU_MAX_NUMBER] = {
    [0 ... DPU_MAX_NUMBER - 1] = { -1, -1, -1 }
};

dpu_list_t dpu_list[WB_MAIN_DPU_DEV_MAX] = {
    [WB_MAIN_DPU_DEV_JAGUAR] = {
        .name = "jaguar_dpu",
        .eeprom_addr = 0x50,
        .ven_addr = 0x02,
        .dev_addr = 0x00,
        .size = 2,
        .vendor_id = 0x531f,
        .device_id = 0x0480,
        .dpu_attr_funcs = jaguar_dpu_func_table,
        .dpu_attr_funcs_size = DFD_DPU_MAX_E,
        .fw_attr_table = jaguar_dpu_fw_attr_table,
        .fw_number = DFD_DPU_FW_TYPE_MAX_E - 1,
        .temp_attr_table = jaguar_dpu_temp_attr_table,
        .temp_number = DFD_DPU_TEMP_TYPE_MAX_E - 1,
        .dpu_temp_funcs = jaguar_dpu_temp_func_table,
        .dpu_temp_funcs_size = DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E,
    },
};

int dfd_common_get_dpu_fw_number(void)
{
    int i;
    int max_fw_number;

    max_fw_number = 0;
    for (i = 0; i < WB_MAIN_DPU_DEV_MAX; i++) {
        max_fw_number = WB_MAX(max_fw_number, dpu_list[i].fw_number);
    }

    return max_fw_number;
}

int dfd_common_get_dpu_temp_number(void)
{
    int i;
    int max_temp_number;

    max_temp_number = 0;
    for (i = 0; i < WB_MAIN_DPU_DEV_MAX; i++) {
        max_temp_number = WB_MAX(max_temp_number, dpu_list[i].temp_number);
    }

    return max_temp_number;
}

static ssize_t dfd_get_dpu_attr_funcs(unsigned int type,
    dfd_sysfs_func_map_t **func_table, int dpu_type)
{
    if (type == DFD_DPU_ID_MONITOR_FLAG_E) {
        *func_table = dpu_list[dpu_type].dpu_attr_funcs;
        return dpu_list[dpu_type].dpu_attr_funcs_size;
    }

    *func_table = dpu_list[dpu_type].dpu_attr_funcs;
    return dpu_list[dpu_type].dpu_attr_funcs_size;
}

static int dfd_get_valid_dpu_type_by_index(unsigned int index, int *dpu_type)
{
    if ((index < 1) || (index > DPU_MAX_NUMBER) || (dpu_type == NULL)) {
        DBG_DPU_DEBUG(DBG_ERROR,"invalid input, index:%u\n", index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (!DFD_DPU_TYPE_IS_VALID(dpu_id_info_table[index - 1].dpu_type)) {
        *dpu_type = dpu_id_info_table[index - 1].dpu_type;
        DBG_DPU_DEBUG(DBG_ERROR,
            "invalid dpu type by index, index:%u, dpu_type:%d\n",
            index, *dpu_type);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    *dpu_type = dpu_id_info_table[index - 1].dpu_type;
    return DFD_RV_OK;
}

static ssize_t dfd_dpu_get_monitor_flag(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    int dpu_type;
    int i;
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_func_map_t *func_table;

    ret = dfd_get_valid_dpu_type_by_index(index, &dpu_type);
    if (ret == 0) {
        ret = dfd_get_dpu_attr_funcs(type, &func_table, dpu_type);
        if (func_table != NULL) {
            get_func = dfd_get_sysfs_value_func(func_table, type, ret);
            if (get_func != NULL) {
                ret = get_func(index, type, buf, count);
                if (ret >= 0) {
                    return ret;
                }

                DBG_DPU_DEBUG(DBG_WARN,
                    "get monitor flag from cached dpu type failed, index:%u, type:%u, dpu_type:%d, ret:%zd\n",
                    index, type, dpu_type, ret);
            }
        } else {
            DBG_DPU_DEBUG(DBG_WARN,
                "get monitor flag func table failed, index:%u, type:%u, dpu_type:%d, ret:%zd\n",
                index, type, dpu_type, ret);
        }
    }

    for (i = 0; i < WB_MAIN_DPU_DEV_MAX; i++) {
        if (ret == 0 && i == dpu_type) {
            continue;
        }

        ret = dfd_get_dpu_attr_funcs(type, &func_table, i);
        if (func_table == NULL) {
            continue;
        }

        get_func = dfd_get_sysfs_value_func(func_table, type, ret);
        if (get_func == NULL) {
            continue;
        }

        ret = get_func(index, type, buf, count);
        if (ret >= 0) {
            return ret;
        }

        DBG_DPU_DEBUG(DBG_WARN,
            "probe monitor flag failed, index:%u, type:%u, dpu_type:%d, ret:%zd\n",
            index, type, i, ret);
    }

    DBG_DPU_DEBUG(DBG_ERROR,
        "get monitor flag unsupported after probing all dpu types, index:%u, type:%u\n",
        index, type);
    return -DFD_RV_DEV_NOTSUPPORT;
}

int dfd_get_dpu_type_by_slot(unsigned int main_dev_id)
{
    int ret;
    int dpu_type;
    unsigned int dpu_id;

    dpu_id = main_dev_id;
    if (dpu_id < 1 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "invalid dpu_id:%u\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_valid_dpu_type_by_index(dpu_id, &dpu_type);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get valid dpu type failed, dpu_id:%u, dpu_type:%d, ret:%d\n",
            dpu_id, dpu_type, ret);
        return ret;
    }

    return dpu_type;
}

int dfd_get_dpu_bus_num(int main_dev_id)
{
    info_ctrl_t *info_ctrl;
    uint64_t key;
    int dpu_type, dpu_id;

    dpu_id = main_dev_id;

    if (dpu_id <= 0 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "dpu_id error, dpu_id:%d\n", dpu_id);
        return -1;
    }

    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "get dpu type failed, dpu_id:%d, ret:%d\n", dpu_id, dpu_type);
        return dpu_type;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_DEVICE, dpu_type,
        DFD_GET_DPU_COMMON_KEY2(dpu_id, DFD_DPU_BUSID_E));

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DPU_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -1;
    }

    return dfd_ko_extract_i2c_bus_number(info_ctrl->fpath);
}

static int get_dpu_para_val(unsigned int dpu_id, unsigned int sub_dev_id, int dpu_type,
    char *val, char *buf, dpu_func_attr_t func_attr, unsigned int count)
{
    int ret, bus_num;
    uint64_t key;
    info_ctrl_t *info_ctrl;

    if (dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX) {
        DBG_DPU_DEBUG(DBG_ERROR, "dpu_type error, dpu_type:%d\n", dpu_type);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_DEVICE, dpu_type,
        DFD_GET_DPU_COMMON_KEY2(dpu_id, sub_dev_id));
    ret = dfd_info_get_dpu(key, val, INFO_INT_MAX_LEN);
    if (ret > 0) {
        DBG_DPU_DEBUG(DBG_VERBOSE, "%s\n", val);
        snprintf(buf, count, "%s\n", val);
        return strlen(buf);
    }

    DBG_DPU_DEBUG(DBG_VERBOSE,
        "dpu_id: %u, sub_dev_id %u default config not exit, key_name: %s, begin to get from i2c.\n",
        dpu_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_DPU_DEVICE));
    key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_DEVICE, dpu_type,
        DFD_GET_DPU_COMMON_KEY2(dpu_id, DFD_DPU_BUSID_E));

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DPU_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    bus_num = dfd_ko_extract_i2c_bus_number(info_ctrl->fpath);
    if (bus_num < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get i2c bus number fail, dpu_id: %u, sub_dev_id: %u\n",
            dpu_id, sub_dev_id);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    ret = dfd_ko_i2c_dev_smbus_read(bus_num, info_ctrl->addr, func_attr.offset, val, func_attr.size);
    if (ret) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "dfd_ko_i2c_dev_smbus_read failed, bus: %d, addr: 0x%x, offset: 0x%x, ret: %d\n",
            bus_num, info_ctrl->addr, func_attr.offset, ret);
        return -DFD_RV_INVALID_VALUE;
    }

    return DFD_RV_OK;
}

static uint64_t build_default_key(dpu_key_mode_t mode, uint64_t cfg_item_type, int dpu_type,
    unsigned int dpu_id, unsigned int sub_dev_id, unsigned int sensor_index)
{
    if (mode == DPU_KEY_MODE_SENSOR) {
        return DFD_CFG_KEY(cfg_item_type,
            DFD_GET_DPU_KEY1(dpu_type, dpu_id),
            DFD_GET_DPU_COMMON_KEY2(sub_dev_id, sensor_index));
    } else {
        return DFD_CFG_KEY(cfg_item_type,
            dpu_type,
            DFD_GET_DPU_COMMON_KEY2(dpu_id, sub_dev_id));
    }
}

static int get_dpu_para_sensor_val_generic(unsigned int dpu_id, unsigned int sub_dev_id,
    unsigned int sensor_index, int dpu_type, char *val, char *buf, dpu_func_attr_t func_attr,
    unsigned int count, uint64_t cfg_item_type, dpu_key_mode_t mode)
{
    int ret, bus_num;
    uint64_t key;
    info_ctrl_t *info_ctrl;

    key = build_default_key(mode, cfg_item_type, dpu_type, dpu_id, sub_dev_id, sensor_index);
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_DPU_DEBUG(DBG_VERBOSE, "%s", val);
        return snprintf(buf, count, "%s", val);
    } else if (IS_TARGET_SENSOR_STATIC(sub_dev_id)) {
        DBG_DPU_DEBUG(DBG_VERBOSE,
            "key %llx, mode %x dpu_type %x dpu_id %x sub_dev_id %x sensor_index %x",
            key, mode, dpu_type, dpu_id, sub_dev_id, sensor_index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_DPU_DEBUG(DBG_VERBOSE,
        "dpu_id: %u, sub_dev_id %u default config not exit, key_name: %s, begin to get from i2c.\n",
        dpu_id, sub_dev_id, key_to_name(cfg_item_type));

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_DEVICE, dpu_type,
        DFD_GET_DPU_COMMON_KEY2(dpu_id, DFD_DPU_BUSID_E));

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DPU_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    bus_num = dfd_ko_extract_i2c_bus_number(info_ctrl->fpath);
    if (bus_num < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get i2c bus number fail, dpu_id: %u, sub_dev_id: %u\n",
            dpu_id, sub_dev_id);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    ret = dfd_ko_i2c_dev_smbus_read(bus_num, info_ctrl->addr, func_attr.offset, val, func_attr.size);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "dfd_ko_i2c_dev_smbus_read failed, bus: %d, addr: 0x%x, offset: 0x%x, ret: %d\n",
            bus_num, info_ctrl->addr, func_attr.offset, ret);
        return ret;
    }

    return DFD_RV_OK;
}

int get_dpu_temp_para_val(unsigned int dpu_id, unsigned int sub_dev_id,
    unsigned int sensor_index, int dpu_type, char *val, char *buf, dpu_func_attr_t func_attr,
    unsigned int count)
{
    return get_dpu_para_sensor_val_generic(dpu_id, sub_dev_id, sensor_index, dpu_type,
        val, buf, func_attr, count, DFD_CFG_ITEM_DPU_DEVICE_TEMP, DPU_KEY_MODE_SENSOR);
}

ssize_t dfd_common_get_dpu_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    int dpu_type;
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_func_map_t *func_table;

    if (type == DFD_DPU_ID_MONITOR_FLAG_E) {
        ret = dfd_dpu_get_monitor_flag(index, type, buf, count);
        if (ret < 0) {
            DBG_DPU_DEBUG(DBG_ERROR,
                "get dpu monitor flag failed, index:%u, ret:%zd\n",
                index, ret);
            if (ret == -DFD_RV_DEV_NOTSUPPORT) {
                return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
            } else {
                return (ssize_t)snprintf(buf, count, "%d\n", 0);
            }
        }

        return ret;
    }

    ret = dfd_get_valid_dpu_type_by_index(index, &dpu_type);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get valid dpu type failed, index:%u, type:%u, ret:%zd\n",
            index, type, ret);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_dpu_attr_funcs(type, &func_table, dpu_type);
    if (func_table == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get dpu attr func table failed, index:%u, type:%u, dpu_type:%d, ret:%zd\n",
            index, type, dpu_type, ret);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    get_func = dfd_get_sysfs_value_func(func_table, type, ret);
    if (get_func == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get dpu attr func failed, index:%u, type:%u, dpu_type:%d, table_size:%zd\n",
            index, type, dpu_type, ret);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "execute dpu attr func failed, index:%u, type:%u, dpu_type:%d, ret:%zd\n",
            index, type, dpu_type, ret);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

int dfd_common_set_dpu_attr(unsigned int index, unsigned int type, unsigned int value)
{
    int ret;
    int dpu_type;
    dfd_sysfs_set_data_func set_func;
    dfd_sysfs_func_map_t *func_table;

    ret = dfd_get_valid_dpu_type_by_index(index, &dpu_type);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get valid dpu type failed, index:%u, type:%u, value:%u, ret:%d\n",
            index, type, value, ret);
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = dfd_get_dpu_attr_funcs(type, &func_table, dpu_type);
    if (func_table == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get dpu set func table failed, index:%u, type:%u, dpu_type:%d, ret:%d\n",
            index, type, dpu_type, ret);
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    set_func = dfd_set_sysfs_value_func(func_table, type, ret);
    if (set_func == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get dpu set func failed, index:%u, type:%u, dpu_type:%d, table_size:%d\n",
            index, type, dpu_type, ret);
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = set_func(index, type, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "execute dpu set func unsupported, index:%u, type:%u, dpu_type:%d, value:%u\n",
            index, type, dpu_type, value);
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

ssize_t dfd_common_get_dpu_fw_attr(unsigned int index, unsigned int fw_index, unsigned int type,
    char *buf, size_t count)
{
    ssize_t ret;
    int dpu_type;
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_func_map_t *func_table;

    ret = dfd_get_valid_dpu_type_by_index(index, &dpu_type);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get valid dpu type failed, index:%u, fw_index:%u, type:%u, ret:%zd\n",
            index, fw_index, type, ret);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_dpu_attr_funcs(type, &func_table, dpu_type);
    if (func_table == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get dpu fw func table failed, index:%u, fw_index:%u, type:%u, dpu_type:%d, ret:%zd\n",
            index, fw_index, type, dpu_type, ret);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    get_func = dfd_get_sysfs_value_func(func_table, type, ret);
    if (get_func == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get dpu fw func failed, index:%u, fw_index:%u, type:%u, dpu_type:%d, table_size:%zd\n",
            index, fw_index, type, dpu_type, ret);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(DFD_COM_DPU_FW_DPU_FW_ID(index, fw_index), type, buf, count);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "execute dpu fw func failed, index:%u, fw_index:%u, type:%u, dpu_type:%d, ret:%zd\n",
            index, fw_index, type, dpu_type, ret);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

static ssize_t dfd_get_dpu_temp_funcs(unsigned int index, unsigned int type,
    dfd_sysfs_func_map_t **func_table)
{
    int dpu_type, ret;

    ret = dfd_get_valid_dpu_type_by_index(index, &dpu_type);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get valid dpu type failed, index:%u, type:%u, ret:%d\n",
            index, type, ret);
        *func_table = NULL;
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    *func_table = dpu_list[dpu_type].dpu_temp_funcs;
    return dpu_list[dpu_type].dpu_temp_funcs_size;
}

ssize_t dfd_common_get_dpu_temp_attr(unsigned int dpu_index, unsigned int temp_index, unsigned int type,
    char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_func_map_t *func_table;

    ret = dfd_get_dpu_temp_funcs(dpu_index, type, &func_table);
    if (func_table == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get dpu temp func table failed, dpu_index:%u, temp_index:%u, type:%u, ret:%zd\n",
            dpu_index, temp_index, type, ret);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    get_func = dfd_get_sysfs_value_func(func_table, type, ret);
    if (get_func == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get dpu temp func failed, dpu_index:%u, temp_index:%u, type:%u, table_size:%zd\n",
            dpu_index, temp_index, type, ret);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(DFD_COM_DPU_SENSOR_DPU_SENSOR_ID(dpu_index, temp_index), type, buf, count);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "execute dpu temp func failed, dpu_index:%u, temp_index:%u, type:%u, ret:%zd\n",
            dpu_index, temp_index, type, ret);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

ssize_t dfd_get_dpu_fw_alias(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int dpu_type, fw_index;
    unsigned int dpu_id;

    dpu_id = DFD_DPU_GET_SENSOR_DPU_ID(main_dev_id);
    if (dpu_id < 1 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "Invalid dpu_id: %d\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (buf == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR, "param error, main_dev_id: %u, sub_dev_id: %u\n", main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_DPU_DEBUG(DBG_VERBOSE, "dfd_get_dpu_fw_alias has been called main_dev_id:0x%x sub_dev_id:0x%x.\n",
        main_dev_id, sub_dev_id);

    mem_clear(buf, count);

    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, dpu_type %d.\n",
            main_dev_id, sub_dev_id, dpu_type);
        return -DFD_RV_INVALID_VALUE;
    }

    fw_index = DFD_DPU_GET_SENSOR_SENSOR_ID(main_dev_id);
    if (fw_index >= DFD_DPU_FW_TYPE_MAX_E || fw_index == 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "fw_index %d out of range.\n", fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    return (ssize_t)snprintf(buf, count, "%s\n", dpu_list[dpu_type].fw_attr_table[fw_index].name);
}

ssize_t dfd_get_dpu_fw_version(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, dpu_type, fw_index;
    unsigned int dpu_id;

    dpu_id = DFD_DPU_GET_SENSOR_DPU_ID(main_dev_id);
    if (dpu_id < 1 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "Invalid dpu_id: %u\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (buf == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "param error, buf is NULL, main_dev_id: 0x%x, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_DPU_DEBUG(DBG_VERBOSE,
        "dfd_get_dpu_fw_version has been called, main_dev_id: 0x%x, sub_dev_id: %u\n",
        main_dev_id, sub_dev_id);

    mem_clear(buf, count);
    mem_clear(val, INFO_INT_MAX_LEN + 1);

    dpu_type = dpu_id_info_table[dpu_id - 1].dpu_type;
    if (dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "main_dev_id: 0x%x, sub_dev_id %u config error, dpu_type %d.\n",
            main_dev_id, sub_dev_id, dpu_type);
        return -DFD_RV_INVALID_VALUE;
    }

    fw_index = DFD_DPU_GET_SENSOR_FW_ID(main_dev_id);
    if (fw_index >= DFD_DPU_FW_TYPE_MAX_E || fw_index == 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "fw_index %d out of range.\n", fw_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = get_dpu_para_val(dpu_id, sub_dev_id, dpu_type, val, buf,
        dpu_list[dpu_type].fw_attr_table[fw_index], count);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get_dpu_para_val failed, main_dev_id: 0x%x, sub_dev_id: %u, ret: %d\n",
            main_dev_id, sub_dev_id, ret);
        return ret;
    }

    return dpu_data_to_buf(buf, count, val,
        dpu_list[dpu_type].fw_attr_table[fw_index].size,
        dpu_list[dpu_type].fw_attr_table[fw_index].base);
}

ssize_t dfd_get_dpu_fw_support_upgrade(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    return -DFD_RV_DEV_NOTSUPPORT;
}

ssize_t dfd_get_dpu_fw_upgrade_type(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    return -DFD_RV_DEV_NOTSUPPORT;
}

ssize_t dfd_get_dpu_temp_alias(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int dpu_type;
    int dpu_id, temp_index;

    dpu_id = DFD_DPU_GET_SENSOR_DPU_ID(main_dev_id);
    if (dpu_id < 1 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "Invalid dpu_id: %d\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (buf == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR, "param error, main_dev_id: %u, sub_dev_id: %u\n", main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_DPU_DEBUG(DBG_VERBOSE,
        "dfd_get_dpu_temp_alias has been called main_dev_id:0x%x sub_dev_id:0x%x.\n",
        main_dev_id, sub_dev_id);

    mem_clear(buf, count);

    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "main_dev_id: %u, sub_dev_id %u config error, dpu_type %d.\n",
            main_dev_id, sub_dev_id, dpu_type);
        return -DFD_RV_INVALID_VALUE;
    }

    temp_index = DFD_DPU_GET_SENSOR_SENSOR_ID(main_dev_id);
    if (temp_index >= DFD_DPU_TEMP_TYPE_MAX_E) {
        DBG_DPU_DEBUG(DBG_ERROR, "temp_index %d out of range.\n", temp_index);
        return -DFD_RV_INVALID_VALUE;
    }

    return (ssize_t)snprintf(buf, count, "%s\n", dpu_list[dpu_type].temp_attr_table[temp_index].name);
}

static int format_and_copy_to_buf_reverse(char *buf, ssize_t buf_size, uint8_t *val, int valid_size, dpu_base_type mode)
{
    int i, count, temp;
    unsigned long decimal_val;
    int ret;
    char *ptr;
    char temp_buf[INFO_INT_MAX_LEN];

    DBG_DPU_DEBUG(DBG_VERBOSE, "format_and_copy_to_buf_reverse called.\n");

    if (buf == NULL || val == NULL || valid_size <= 0 || buf_size <= 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "Invalid parameters\n");
        return -DFD_RV_INVALID_VALUE;
    }

    ptr = buf;

    if (mode & DPU_ASCII_BASE) {
        DBG_DPU_DEBUG(DBG_VERBOSE, "mode is ASCII_BASE.\n");
        for (i = 0; i < valid_size / 2; i++) {
            temp = val[i];
            val[i] = val[valid_size - 1 - i];
            val[valid_size - 1 - i] = temp;
        }

        if (valid_size > 0 && valid_size <= INFO_INT_MAX_LEN) {
            val[valid_size] = '\0';
        } else {
            DBG_DPU_DEBUG(DBG_ERROR, "valid_size %d is out of range.\n", valid_size);
            return -DFD_RV_INVALID_VALUE;
        }

        if ((valid_size + 2) > buf_size) {
            DBG_DPU_DEBUG(DBG_ERROR, "Buffer overflow\n");
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
            DBG_DPU_DEBUG(DBG_ERROR, "Buffer overflow or error occurred\n");
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

    DBG_DPU_DEBUG(DBG_VERBOSE, "buf is  %s.\n", buf);
    if (mode & DPU_DECIMAL_BASE) {
        DBG_DPU_DEBUG(DBG_VERBOSE, "mode is  DECIMAL_BASE.\n");
        if (kstrtoul(buf, 16, &decimal_val) != 0) {
            DBG_DPU_DEBUG(DBG_ERROR, "kstrtoul failed, buf: %s\n", buf);
            return -DFD_RV_INVALID_VALUE;
        }

        if (mode & DPU_X1000_BASE) {
            if (decimal_val > ULONG_MAX / 1000) {
                DBG_DPU_DEBUG(DBG_ERROR, "Reverse function multiplication overflow: %lu * 1000\n", decimal_val);
                return -DFD_RV_INVALID_VALUE;
            }
            decimal_val *= 1000;
        } else if (mode & DPU_X1000000_BASE) {
            if (decimal_val > ULONG_MAX / 1000000) {
                DBG_DPU_DEBUG(DBG_ERROR, "Reverse function multiplication overflow: %lu * 1000000\n", decimal_val);
                return -DFD_RV_INVALID_VALUE;
            }
            decimal_val *= 1000000;
        }

        return snprintf(buf, buf_size, "%lu\n", decimal_val);
    } else if (mode & DPU_HEX_BASE) {
        size_t out_size;

        ret = snprintf(temp_buf, sizeof(temp_buf), "0x%s\n", buf);
        if (ret < 0) {
            DBG_DPU_DEBUG(DBG_ERROR, "Buffer overflow or error occurred\n");
            return -DFD_RV_INVALID_VALUE;
        }

        out_size = sizeof(temp_buf);
        if ((size_t)buf_size < out_size) {
            out_size = (size_t)buf_size;
        }

        return snprintf(buf, out_size, "%s", temp_buf);
    }

    return -DFD_RV_INVALID_VALUE;
}

static int format_and_copy_to_buf_forward(char *buf, ssize_t buf_size, uint8_t *val, int valid_size, dpu_base_type mode)
{
    int i, count;
    unsigned long decimal_val;
    int ret;
    char *ptr;
    char temp_buf[INFO_INT_MAX_LEN];

    if (buf == NULL || val == NULL || valid_size <= 0 || buf_size <= 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "Invalid parameters\n");
        return -DFD_RV_INVALID_VALUE;
    }

    ptr = buf;

    if (mode & DPU_ASCII_BASE) {
        if ((valid_size + 2) > buf_size) {
            DBG_DPU_DEBUG(DBG_ERROR, "Buffer overflow\n");
            return -DFD_RV_INVALID_VALUE;
        }
        return snprintf(buf, valid_size + 2, "%.*s\n", valid_size, val);
    }

    if (!(mode & (DPU_DECIMAL_BASE | DPU_HEX_BASE))) {
        DBG_DPU_DEBUG(DBG_ERROR, "invalid mode.\n");
        return -DFD_RV_INVALID_VALUE;
    }

    count = valid_size * 2 + 2;
    if (count > buf_size) {
        return -DFD_RV_INVALID_VALUE;
    }

    for (i = 0; i < valid_size; i++) {
        ret = snprintf(ptr, count, "%02x", val[i]);
        if (ret < 0 || ret >= count) {
            DBG_DPU_DEBUG(DBG_ERROR, "Buffer overflow or error occurred\n");
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

    DBG_DPU_DEBUG(DBG_VERBOSE, "buf is %s.\n", buf);
    if (mode & DPU_DECIMAL_BASE) {
        if (kstrtoul(buf, 16, &decimal_val) != 0) {
            DBG_DPU_DEBUG(DBG_ERROR, "kstrtoul failed, buf: %s\n", buf);
            return -DFD_RV_INVALID_VALUE;
        }

        if (mode & DPU_X1000_BASE) {
            if (decimal_val > ULONG_MAX / 1000) {
                DBG_DPU_DEBUG(DBG_ERROR, "Forward function multiplication overflow: %lu * 1000\n", decimal_val);
                return -DFD_RV_INVALID_VALUE;
            }
            decimal_val *= 1000;
        } else if (mode & DPU_X1000000_BASE) {
            if (decimal_val > ULONG_MAX / 1000000) {
                DBG_DPU_DEBUG(DBG_ERROR, "Forward function multiplication overflow: %lu * 1000000\n", decimal_val);
                return -DFD_RV_INVALID_VALUE;
            }
            decimal_val *= 1000000;
        }

        return snprintf(buf, buf_size, "%lu\n", decimal_val);
    } else if (mode & DPU_HEX_BASE) {
        size_t out_size;

        ret = snprintf(temp_buf, sizeof(temp_buf), "0x%s\n", buf);
        if (ret < 0) {
            DBG_DPU_DEBUG(DBG_ERROR, "Buffer overflow or error occurred\n");
            return -DFD_RV_INVALID_VALUE;
        }

        out_size = sizeof(temp_buf);
        if ((size_t)buf_size < out_size) {
            out_size = (size_t)buf_size;
        }

        return snprintf(buf, out_size, "%s", temp_buf);
    }

    return -DFD_RV_INVALID_VALUE;
}

int dpu_data_to_buf(char *buf, ssize_t buf_size, uint8_t *val, int valid_size, unsigned int mode)
{
    if (buf == NULL || val == NULL || valid_size <= 0 || buf_size <= 0 || valid_size >= buf_size) {
        DBG_DPU_DEBUG(DBG_ERROR, "Invalid parameters\n");
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_DPU_DEBUG(DBG_VERBOSE, "valid_size is %d, mode is 0x%x.\n", valid_size, mode);
    switch (mode & ORDER_DIRECTION_MASK) {
        case DPU_REVERSE_BASE:
            return format_and_copy_to_buf_reverse(buf, buf_size, val, valid_size, mode);
        case DPU_FORWARD_BASE:
            return format_and_copy_to_buf_forward(buf, buf_size, val, valid_size, mode);
        default:
            break;
    }

    return -DFD_RV_INVALID_VALUE;
}

