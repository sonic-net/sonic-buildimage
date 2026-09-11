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

#include "wb_gcu_driver.h"
#include "wb_gcu_l600_l900_driver.h"

static ssize_t dfd_get_gcu_power_common(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_gcu_vol_common(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_gcu_temp_common(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_gcu_attr_nomap(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_pcie_rate_max(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_pcie_rate_current(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_pcie_link_width_max(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_pcie_link_width_current(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_health_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_temp_number(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_vol_number(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_get_power_number(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_op_module_present(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_op_module_temp(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_id_detect(unsigned int slot, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_monitor_flag(unsigned int slot, unsigned int sub_dev_id, char *buf, size_t count);

gcu_reg_attr_t enflame_L600_L900_gcu_attr_table[DFD_GCU_DEVICE_TYPE_MAX_E] = {
    [DFD_GCU_ALIAS_E] =                     {"Driver Version",              0x0,    4,  0,                              NULL},
    [DFD_GCU_DRIVER_VERSION_E] =            {"Driver Version",              0x06,   8,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_FIRMWARE_VERSION_E] =          {"Firmware Version",            0x07,   3,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_PCIE_VENDOR_ID_E] =            {"PCIe Vendor ID",              0xA0,   2,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_PCIE_DEVICE_ID_E] =            {"PCIe Device ID",              0xA1,   2,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_PCIE_SUB_VENDOR_ID_E] =        {"PCIe Sub Vendor ID",          0xA2,   2,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_PCIE_SUB_DEVICE_ID_E] =        {"PCIe Sub Device ID",          0xA3,   2,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_PRODUCT_TYPE_E] =              {"Product Model",               0xA5,   20, FORWARD_BASE | ASCII_BASE,      NULL},
    [DFD_GCU_PART_NUMBER_E] =               {"Part Number",                 0xA6,   20, FORWARD_BASE | ASCII_BASE,      NULL},
    [DFD_GCU_MANUFACTURER_DATE_E] =         {"Manufacturing Date",          0xA7,   4,  REVERSE_BASE | DECIMAL_BASE,    NULL},
    [DFD_GCU_SERIAL_NUMBER_E] =             {"Serial Number",               0xA8,   16, FORWARD_BASE | ASCII_BASE,      NULL},
    [DFD_GCU_HBM_CAPACITY_E] =              {"HBM Capacity",                0xA9,   1,  REVERSE_BASE | DECIMAL_BASE,    NULL},
    [DFD_GCU_MFR_NAME_E] =                  {"Manufacturer",                0xAA,   8,  FORWARD_BASE | ASCII_BASE,      NULL},
    [DFD_GCU_FREQUENT_MAX_E] =              {"GCU Max Frequency",           0xAC,   2,  REVERSE_BASE | DECIMAL_BASE,    NULL},
    [DFD_GCU_ECC_STATUS_E] =                {"ECC Status",                  0x10,   1,  REVERSE_BASE | DECIMAL_BASE,    NULL},
    [DFD_GCU_BOARD_TYPE_E] =                {"Board Type",                  0xA4,   1,  REVERSE_BASE | DECIMAL_BASE,    NULL},
    [DFD_GCU_PCIE_RATE_MAX_E] =             {"PCIe Max Rate",               0xAD,   6,  MAPPING_BASE,                   "default_pcie_rate_map"},
    [DFD_GCU_PCIE_RATE_CURRENT_E] =         {"PCIe Current Rate",           0xAE,   6,  MAPPING_BASE,                   "default_pcie_rate_map"},
    [DFD_GCU_PCIE_LINK_WIDTH_MAX_E] =       {"PCIe Max Link Width",         0xAF,   5,  MAPPING_BASE,                   "default_pcie_width_map"},
    [DFD_GCU_PCIE_LINK_WIDTH_CURRENT_E] =   {"PCIe Current Link Width",     0xB0,   5,  MAPPING_BASE,                   "default_pcie_width_map"},
    [DFD_GCU_OPTICAL_MODULE_PESENT_E] =     {"Optical Module Presence",     0xB1,   2,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_OPTICAL_MODULE_TEMP_E] =       {"Optical Module Temperature",  0xB2,   2,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_PERIPHERAL_ERROR_E] =          {"Peripheral Error",            0xF0,   2,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_PCIE_ERROR_E] =                {"PCIe Error",                  0xF1,   2,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_HBM_ERROR_E] =                 {"HBM Error",                   0xF3,   2,  REVERSE_BASE | HEX_BASE,        NULL},
    [DFD_GCU_HEALTH_STATUS_E] =             {"Health status",               0x0,    4,  0,                              NULL},
    [DFD_GCU_TEMP_NUM_E] =                  {"Temp node number",            0x0,    4,  DECIMAL_BASE,                   NULL},
    [DFD_GCU_VOL_NUM_E] =                   {"Vol node number",             0x0,    4,  DECIMAL_BASE,                   NULL},
    [DFD_GCU_POWER_NUM_E] =                 {"Power node number",           0x0,    4,  DECIMAL_BASE,                   NULL},
};

dfd_sysfs_func_map_t l600_l900_gcu_func_table[DFD_GCU_DEVICE_TYPE_MAX_E] = {
    [DFD_GCU_ALIAS_E] = {dfd_get_gcu_alias, NULL},
    [DFD_GCU_DRIVER_VERSION_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_FIRMWARE_VERSION_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_PCIE_VENDOR_ID_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_PCIE_DEVICE_ID_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_PCIE_SUB_VENDOR_ID_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_PCIE_SUB_DEVICE_ID_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_PRODUCT_TYPE_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_PART_NUMBER_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_MANUFACTURER_DATE_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_SERIAL_NUMBER_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_HBM_CAPACITY_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_MFR_NAME_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_FREQUENT_MAX_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_ECC_STATUS_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_BOARD_TYPE_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_PCIE_RATE_MAX_E] = {dfd_get_pcie_rate_max, NULL},
    [DFD_GCU_PCIE_RATE_CURRENT_E] = {dfd_get_pcie_rate_current, NULL},
    [DFD_GCU_PCIE_LINK_WIDTH_MAX_E] = {dfd_get_pcie_link_width_max, NULL},
    [DFD_GCU_PCIE_LINK_WIDTH_CURRENT_E] = {dfd_get_pcie_link_width_current, NULL},
    [DFD_GCU_OPTICAL_MODULE_PESENT_E] = {dfd_get_op_module_present, NULL},
    [DFD_GCU_OPTICAL_MODULE_TEMP_E] = {dfd_get_op_module_temp, NULL},
    [DFD_GCU_PERIPHERAL_ERROR_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_PCIE_ERROR_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_HBM_ERROR_E] = {dfd_get_gcu_attr_nomap, NULL},
    [DFD_GCU_HEALTH_STATUS_E] = {dfd_get_health_status, NULL},
    [DFD_GCU_ID_DETECT_E] = {dfd_get_id_detect, NULL},
    [DFD_GCU_ID_MONITOR_FLAG_E] = {dfd_get_monitor_flag, NULL},
    [DFD_GCU_TEMP_NUM_E] = {dfd_get_temp_number, NULL},
    [DFD_GCU_VOL_NUM_E] = {dfd_get_vol_number, NULL},
    [DFD_GCU_POWER_NUM_E] = {dfd_get_power_number, NULL},
};

gcu_reg_attr_t enflame_L600_L900_gcu_temp_attr_table[DFD_GCU_TEMP_TYPE_MAX] = {
    [DFD_GCU_TEMP_E] =      {"C_TEMP",      0x03,   1,  REVERSE_BASE | DECIMAL_BASE | X1000_BASE,   NULL},
    [DFD_GCU_HBM_TEMP_E] =  {"HBM_TEMP",    0x04,   1,  REVERSE_BASE | DECIMAL_BASE | X1000_BASE,   NULL},
};

dfd_sysfs_func_map_t l600_l900_gcu_temp_func_table[DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E] = {
    [DFD_BMC_MANAGED_SENSOR_INPUT_E] = {dfd_get_gcu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_ALIAS_E] = {dfd_get_gcu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_MAX_E] = {dfd_get_gcu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_MIN_E] = {dfd_get_gcu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_HIGH_E] = {dfd_get_gcu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_LOW_E] = {dfd_get_gcu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E] = {dfd_get_gcu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E] = {dfd_get_gcu_temp_common, NULL},
};

gcu_reg_attr_t enflame_L600_L900_gcu_vol_attr_table[DFD_GCU_VOL_TYPE_MAX] = {
    [DFD_GCU_VOL_E] = {"C_VOL",      0x02, 2, REVERSE_BASE | DECIMAL_BASE,        NULL},
    [DFD_GCU_HBM_VOL_E] = {"HBM_VOL",   0x05, 2, REVERSE_BASE | DECIMAL_BASE,        NULL},
};

dfd_sysfs_func_map_t l600_l900_gcu_vol_func_table[DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E] = {
    [DFD_BMC_MANAGED_SENSOR_INPUT_E] = {dfd_get_gcu_vol_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_ALIAS_E] = {dfd_get_gcu_vol_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_MAX_E] = {dfd_get_gcu_vol_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_MIN_E] = {dfd_get_gcu_vol_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_HIGH_E] = {dfd_get_gcu_vol_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_LOW_E] = {dfd_get_gcu_vol_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E] = {dfd_get_gcu_vol_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E] = {dfd_get_gcu_vol_common, NULL},
};

gcu_reg_attr_t enflame_L600_L900_gcu_power_attr_table[DFD_GCU_POWER_TYPE_MAX] = {
    [DFD_GCU_POWER_E] = {"C_PWR",           0x01, 2, REVERSE_BASE | DECIMAL_BASE | X1000000_BASE,        NULL},
    [DFD_GCU_POWER_MAX_E] = {"C_MPWR",      0xAB, 2, REVERSE_BASE | DECIMAL_BASE | X1000000_BASE,        NULL},
};

dfd_sysfs_func_map_t l600_l900_gcu_power_func_table[DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E] = {
    [DFD_BMC_MANAGED_SENSOR_INPUT_E] = {dfd_get_gcu_power_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_ALIAS_E] = {dfd_get_gcu_power_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_MAX_E] = {dfd_get_gcu_power_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_MIN_E] = {dfd_get_gcu_power_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_HIGH_E] = {dfd_get_gcu_power_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_LOW_E] = {dfd_get_gcu_power_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E] = {dfd_get_gcu_power_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E] = {dfd_get_gcu_power_common, NULL},
};

health_check_rule_t enflame_l600_900_health_rules[L600_HEALTH_RULES_COUNT] = {
    {
        .error_type = DFD_GCU_PERIPHERAL_ERROR_E,
        .warning_bitmask = (1 << 14),
        .critical_bitmask = (1 << 15),
        .warning_status = HEALTH_PERI_WARNING,
        .critical_status = HEALTH_PERI_CRITICAL
    },
    {
        .error_type = DFD_GCU_PCIE_ERROR_E,
        .warning_bitmask = (1 << 14),
        .critical_bitmask = (1 << 15),
        .warning_status = HEALTH_PCIE_WARNING,
        .critical_status = HEALTH_PCIE_CRITICAL
    },
    {
        .error_type = DFD_GCU_HBM_ERROR_E,
        .warning_bitmask = (1 << 14),
        .critical_bitmask = (1 << 15),
        .warning_status = HEALTH_HBM_WARNING,
        .critical_status = HEALTH_HBM_CRITICAL
    }
};

gcu_map_t enflame_L600_maps[L600_MAPPING_COUNT] = {
    {"default_pcie_rate_map", default_pcie_rate_map, PCIE_RATE_MAP_SIZE},
    {"default_pcie_width_map", default_pcie_width_map, PCIE_WIDTH_MAP_SIZE},
};

gcu_map_t enflame_L900_maps[L900_MAPPING_COUNT] = {
    {"default_pcie_rate_map", default_pcie_rate_map, PCIE_RATE_MAP_SIZE},
    {"default_pcie_width_map", default_pcie_width_map, PCIE_WIDTH_MAP_SIZE},
};

static ssize_t dfd_get_gcu_power_common(unsigned int main_dev_id, unsigned int sub_dev_id,
                                      char *buf, size_t count)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, gcu_type, power_index;
    int dev_id;

    dev_id = DFD_GET_SENSOR_GCUID(main_dev_id);

    if (buf == NULL || dev_id > GCU_MAX_NUMBER || dev_id < 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "param error, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_GCU_DEBUG(DBG_VERBOSE, "main_dev_id:0x%x sub_dev_id:0x%x.\n", main_dev_id, sub_dev_id);

    mem_clear(buf, count);
    mem_clear(val, INFO_INT_MAX_LEN + 1);

    gcu_type = gcu_id_info_table[dev_id - 1].gcu_type;
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX || sub_dev_id > DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E - 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, gcu_type %d.\n",
            main_dev_id, sub_dev_id, gcu_type);
        return -EINVAL;
    }

    power_index = DFD_GET_GCU_SENSOR_ID(main_dev_id);
    if (power_index > gcu_list[gcu_type].power_number) {
        DBG_GCU_DEBUG(DBG_ERROR, "power_index %d out of range.\n", power_index);
        return -EINVAL;
    }

    ret = get_gcu_power_para_val(dev_id, sub_dev_id, power_index, gcu_type, val, buf,
                               gcu_list[gcu_type].power_attr_table[power_index], count);
    if (ret < 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "get_gcu_power_para_val failed, main_dev_id: %u, sub_dev_id: %u, ret: %d\n",
            main_dev_id, sub_dev_id, ret);
        return ret;
    }

    if (IS_TARGET_SENSOR_STATIC(sub_dev_id)) {
        return strlen(buf);
    }

    return data_to_buf(buf, count, val,
        gcu_list[gcu_type].power_attr_table[power_index].size,
        gcu_type, gcu_list[gcu_type].power_attr_table[power_index].base,
        NULL);
}

static ssize_t dfd_get_gcu_vol_common(unsigned int main_dev_id, unsigned int sub_dev_id,
                                    char *buf, size_t count)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, gcu_type, vol_index;
    int dev_id;

    dev_id = DFD_GET_SENSOR_GCUID(main_dev_id);

    if (buf == NULL || dev_id > GCU_MAX_NUMBER || dev_id < 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "param error, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_GCU_DEBUG(DBG_VERBOSE, "main_dev_id:0x%x sub_dev_id:0x%x.\n", main_dev_id, sub_dev_id);

    mem_clear(buf, count);
    mem_clear(val, INFO_INT_MAX_LEN + 1);

    gcu_type = gcu_id_info_table[dev_id - 1].gcu_type;
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX || sub_dev_id > DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E - 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, gcu_type %d.\n",
            main_dev_id, sub_dev_id, gcu_type);
        return -EINVAL;
    }

    vol_index = DFD_GET_GCU_SENSOR_ID(main_dev_id);
    if (vol_index > gcu_list[gcu_type].vol_number) {
        DBG_GCU_DEBUG(DBG_ERROR, "vol_index %d out of range.\n", vol_index);
        return -EINVAL;
    }

    ret = get_gcu_vol_para_val(dev_id, sub_dev_id, vol_index, gcu_type, val, buf,
                             gcu_list[gcu_type].vol_attr_table[vol_index], count);
    if (ret < 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "get_gcu_vol_para_val failed, main_dev_id: %u, sub_dev_id: %u, ret: %d\n",
            main_dev_id, sub_dev_id, ret);
        return ret;
    }

    if (IS_TARGET_SENSOR_STATIC(sub_dev_id)) {
        return strlen(buf);
    }

    return data_to_buf(buf, count, val,
        gcu_list[gcu_type].vol_attr_table[vol_index].size,
        gcu_type, gcu_list[gcu_type].vol_attr_table[vol_index].base,
        NULL);
}

static ssize_t dfd_get_gcu_temp_common(unsigned int main_dev_id, unsigned int sub_dev_id,
                                      char *buf, size_t count)
{
   int ret, gcu_type, temp_index;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int dev_id;
    /* gcu id  */
    dev_id = DFD_GET_SENSOR_GCUID(main_dev_id);

    if (buf == NULL || dev_id > GCU_MAX_NUMBER || dev_id < 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "param error, main_dev_id: 0x%x, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_GCU_DEBUG(DBG_VERBOSE, "main_dev_id:0x%x sub_dev_id:0x%x.\n", main_dev_id, sub_dev_id);

    mem_clear(buf, count);
    mem_clear(val, INFO_INT_MAX_LEN + 1);

    gcu_type = gcu_id_info_table[dev_id - 1].gcu_type;
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX || sub_dev_id > DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E - 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, gcu_type %d.\n",
            main_dev_id, sub_dev_id, gcu_type);
        return -EINVAL;
    }

    temp_index = DFD_GET_GCU_SENSOR_ID(main_dev_id);
    if (temp_index > gcu_list[gcu_type].temp_number) {
        DBG_GCU_DEBUG(DBG_ERROR, "temp_index %d out of range.\n", temp_index);
        return -EINVAL;
    }

    ret = get_gcu_temp_para_val(dev_id, sub_dev_id, temp_index, gcu_type, val, buf,
                          gcu_list[gcu_type].temp_attr_table[temp_index], count);
    if (ret < 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "get_gcu_temp_para_val failed, main_dev_id: %u, sub_dev_id: %u, ret: %d\n",
            main_dev_id, sub_dev_id, ret);
        return ret;
    }

    if (IS_TARGET_SENSOR_STATIC(sub_dev_id)) {
        return strlen(buf);
    }

    return data_to_buf(buf, count, val,
        gcu_list[gcu_type].temp_attr_table[temp_index].size,
        gcu_type, gcu_list[gcu_type].temp_attr_table[temp_index].base,
        NULL);
}

static ssize_t dfd_get_gcu_attr_nomap(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    return dfd_get_gcu_attr_common(main_dev_id, sub_dev_id, buf, count, NULL);
}

static ssize_t dfd_get_pcie_rate_max(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    return dfd_get_gcu_attr_common(main_dev_id, sub_dev_id, buf, count, "default_pcie_rate_map");
}

static ssize_t dfd_get_pcie_rate_current(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    return dfd_get_gcu_attr_common(main_dev_id, sub_dev_id, buf, count, "default_pcie_rate_map");
}

static ssize_t dfd_get_pcie_link_width_max(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    return dfd_get_gcu_attr_common(main_dev_id, sub_dev_id, buf, count, "default_pcie_width_map");
}

static ssize_t dfd_get_pcie_link_width_current(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    return dfd_get_gcu_attr_common(main_dev_id, sub_dev_id, buf, count, "default_pcie_width_map");
}

static ssize_t dfd_get_health_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    health_check_rule_t *rule;
    int ret, gcu_type, i, health_staus;

    if (buf == NULL || main_dev_id < 1 || main_dev_id > GCU_MAX_NUMBER) {
        DBG_GCU_DEBUG(DBG_ERROR, "param error, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_GCU_DEBUG(DBG_VERBOSE, "dfd_get_hbm_error has been called main_dev_id:0x%x sub_dev_id:0x%x.\n", main_dev_id, sub_dev_id);

    mem_clear(buf, count);
    mem_clear(val, INFO_INT_MAX_LEN + 1);

    gcu_type = gcu_id_info_table[main_dev_id - 1].gcu_type;
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX || sub_dev_id > DFD_GCU_DEVICE_TYPE_MAX_E - 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, gcu_type %d.\n",
            main_dev_id, sub_dev_id, gcu_type);
        return -DFD_RV_INVALID_VALUE;
    }

    health_staus = 0;

    if (gcu_list[gcu_type].health_rules == NULL ||
        gcu_list[gcu_type].health_rule_number <= 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u: Invalid health rules config\n",
                    main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    for (i = 0; i < gcu_list[gcu_type].health_rule_number; i++) {
        rule = &gcu_list[gcu_type].health_rules[i];
        if (rule == NULL) {
            DBG_GCU_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u gcu_type %d config error, rule is null\n",
                main_dev_id, sub_dev_id, gcu_type);
            return -EINVAL;
        }

        ret = get_gcu_para_val(main_dev_id, sub_dev_id, 0, gcu_type, val, buf, gcu_list[gcu_type].attr_table[rule->error_type], count);
        if (ret < 0) {
            DBG_GCU_DEBUG(DBG_ERROR, "get_gcu_para_val failed, main_dev_id: %u, sub_dev_id: %u, ret: %d\n",
                main_dev_id, sub_dev_id, ret);
            return ret;
        }

        DBG_GCU_DEBUG(DBG_VERBOSE, "%s: 0x%x\n",
            gcu_list[gcu_type].attr_table[rule->error_type].name, val[0]);

        if (CHECK_BITS_ALL(val, rule->warning_bitmask)) {
            health_staus |= rule->warning_status;
        }

        if (CHECK_BITS_ALL(val, rule->critical_bitmask)) {
            health_staus |= rule->critical_status;
        }
    }

    return snprintf(buf, count > gcu_list[gcu_type].attr_table[sub_dev_id].size + 2 ?
        gcu_list[gcu_type].attr_table[sub_dev_id].size + 2 : count,
        "0x%x\n", health_staus);
}

static ssize_t dfd_get_temp_number(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int gcu_type;

    gcu_type = dfd_check_gcu_type_is_valid(main_dev_id, buf);
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX) {
        return -EINVAL;
    }

    mem_clear(buf, count);

    return snprintf(buf, count, "%d\n", gcu_list[gcu_type].temp_number);
}

static ssize_t dfd_get_vol_number(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    int gcu_type;

    gcu_type = dfd_check_gcu_type_is_valid(main_dev_id, buf);
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX) {
        return -EINVAL;
    }

    mem_clear(buf, count);

    return snprintf(buf, count, "%d\n", gcu_list[gcu_type].vol_number);
}

static ssize_t dfd_get_power_number(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int gcu_type;

    gcu_type = dfd_check_gcu_type_is_valid(main_dev_id, buf);
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX) {
        return -EINVAL;
    }

    mem_clear(buf, count);

    return snprintf(buf, count, "%d\n", gcu_list[gcu_type].power_number);
}

static ssize_t dfd_get_op_module_present(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    DBG_GCU_DEBUG(DBG_WARN, "dfd_get_op_module_present not implemented yet\n");
    return -DFD_RV_DEV_NOTSUPPORT;
}

static ssize_t dfd_get_op_module_temp(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    DBG_GCU_DEBUG(DBG_WARN, "dfd_get_op_module_temp not implemented yet\n");
    return -DFD_RV_DEV_NOTSUPPORT;
}

/* ID Detect */
static ssize_t dfd_get_id_detect(unsigned int slot, unsigned int sub_dev_id, char *buf, size_t count)
{
    int gcu_type;

    if (buf == NULL || slot < 1 || slot > GCU_MAX_NUMBER) {
        DBG_GCU_DEBUG(DBG_ERROR, "param error, slot: %u, sub_dev_id: %u\n",
            slot, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "buf size error, count: %zu, slot: %u, sub_dev_id: %u\n",
            count, slot, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    gcu_type = find_gpu_type_via_single_i2c(slot);
    if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX || sub_dev_id > DFD_GCU_DEVICE_TYPE_MAX_E - 1) {
        DBG_GCU_DEBUG(DBG_ERROR, "gcu_type: %d slot: %u, sub_dev_id %u config error.\n",
            gcu_type, slot, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_GCU_DEBUG(DBG_VERBOSE, "slot: %u, type %s has been detect.\n",
            slot, gcu_list[gcu_type].name);

    gcu_id_info_table[slot - 1].gcu_type = gcu_type;

    mem_clear(buf, count);
    snprintf(buf, count, "%s\n", gcu_list[gcu_type].name);

    return strlen(buf);
}

static ssize_t dfd_get_monitor_flag(unsigned int slot, unsigned int sub_dev_id, char *buf, size_t count)
{
    int gcu_type, gcu_mf_num;
    int *m_num;
    int rv, data, tmp_data, i;
    int *p_decode_value;
    info_ctrl_t *info_ctrl;
    uint64_t key;

    if (buf == NULL || slot < 1 || slot > GCU_MAX_NUMBER) {
        DBG_GCU_DEBUG(DBG_ERROR, "param error, slot: %u, sub_dev_id: %u\n",
            slot, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_GCU_MONITOR_NODE_NUM, slot, 0);
    m_num = dfd_ko_cfg_get_item(key);
    if (m_num == NULL) {
        DBG_GCU_DEBUG(DBG_ERROR, "get monitor flag number failed, slot: %u key : %llx\n", slot, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    gcu_mf_num = *m_num;
    if (gcu_mf_num <= 0) {
        DBG_GCU_DEBUG(DBG_ERROR, "get monitor flag number < 0, slot: %u key : %llx\n", slot, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    tmp_data = 1;
    for (i = 1; i <= gcu_mf_num; i++) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_GCU_MONITOR_FLAG, slot, i);
        info_ctrl = dfd_ko_cfg_get_item(key);
        if (info_ctrl == NULL) {
            DBG_GCU_DEBUG(DBG_VERBOSE, "get info ctrl failed, key=0x%08llx\n", key);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", WB_SENSOR_MONITOR_YES);
        }

        rv = dfd_info_get_int(key, &data, NULL);
        if (rv < 0) {
            DBG_GCU_DEBUG(DBG_ERROR, "get monitor flag error, rv: %d\n", rv);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
        }

        key = DFD_CFG_KEY(DFD_CFG_ITEM_GCU_DECODE_MONITOR_FLAG, slot, data);
        p_decode_value = dfd_ko_cfg_get_item(key);
        if (p_decode_value == NULL) {
            DBG_GCU_DEBUG(DBG_VERBOSE, "status needn't decode. value:0x%x\n", data);
        } else {
            DBG_GCU_DEBUG(DBG_VERBOSE, "ori_value:0x%x, decoded value:0x%x\n", data, *p_decode_value);
            data = *p_decode_value;
        }

        DBG_GCU_DEBUG(DBG_VERBOSE, "slot: %u, data = %d\n", slot, data);

        tmp_data &= data;
        if (tmp_data == 0) {
            DBG_GCU_DEBUG(DBG_VERBOSE, "tmp_data == 0 data = %d, break\n", data);
            break;
        }
    }

    if (gcu_id_info_table[slot - 1].monitor_last_flag == -1) {
        gcu_id_info_table[slot - 1].monitor_last_flag = tmp_data;
        DBG_GCU_DEBUG(DBG_VERBOSE, "first monitor slot: %u, tmp_data = %d last_flag = %d\n",
            slot, tmp_data, gcu_id_info_table[slot - 1].monitor_last_flag);
        gcu_type = find_gpu_type_via_single_i2c(slot);
        if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX) {
            DBG_GCU_DEBUG(DBG_ERROR, "slot: %u, sub_dev_id %u config error.\n",
                slot, sub_dev_id);
            goto monitor_unready;
        }

        goto monitor_ready;

    } else {
        if (tmp_data == 1 && gcu_id_info_table[slot - 1].monitor_last_flag == 0) {
            gcu_type = find_gpu_type_via_single_i2c(slot);
            if (gcu_type < 0 || gcu_type >= WB_MAIN_GCU_DEV_MAX) {
                DBG_GCU_DEBUG(DBG_ERROR, "slot: %u, sub_dev_id %u config error.\n",
                    slot, sub_dev_id);
                goto monitor_unready;
            }

            goto monitor_ready;
        } else if (tmp_data == 0) {
            DBG_GCU_DEBUG(DBG_ERROR, "gpu is offline, we can't monitor it.\n");
            goto monitor_unready;
        }
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 1);

monitor_ready:
    gcu_id_info_table[slot - 1].gcu_type = gcu_type;
    DBG_GCU_DEBUG(DBG_VERBOSE, "slot(%d) gcu_type is %d.\n", slot, gcu_id_info_table[slot - 1].gcu_type);
    gcu_id_info_table[slot - 1].monitor_last_flag = tmp_data;
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 1);

monitor_unready:
    gcu_id_info_table[slot - 1].monitor_last_flag = 0;
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
}