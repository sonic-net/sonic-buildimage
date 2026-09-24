/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_sensor_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * SENSOR Read and write functions related to attributes
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"
#include "dfd_frueeprom.h"
#include "dfd_cfg_file.h"
#include "wb_sensors_driver.h"

#define DFD_FORMAT_STR_MAX_LEN          (32)
#define MULTI_TEMPS_NUM_MAX             (32)
#define TEMP_DATA_BUF_LEN               (32)
#define TEMP_MEDIAN_SAMPLE_NUM          (3)
#define TEMP_MEDIAN_MAX_SAMPLE_NUM      (9)
#define DFD_VOL_INPUT_ACCESS_MAX_COUNT  (256)

int g_dfd_sensor_dbg_level = 0;
module_param(g_dfd_sensor_dbg_level, int, S_IRUGO | S_IWUSR);

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
static const dfd_status_desc_map_t dfd_temp_status_map[] = {
    {TEMP_STATUS_MAJOR_WARNING, "Major_Error"},
    {TEMP_STATUS_FATAL_WARNING, "Fatal_Error"},
    {TEMP_STATUS_FETCH_ERROR, "Fetch_Error"},
};
#endif

dfd_debug_data_key_map_t vol_dbg_key_table[WB_SENSOR_END] = {
    [WB_SENSOR_INPUT]        = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA },
    [WB_SENSOR_ALIAS]        = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_STR_DATA},
    [WB_SENSOR_TYPE]         = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_MAX]          = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_MAX_HYST]     = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_MIN]          = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_CRIT]         = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_RANGE]        = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_NOMINAL_VAL]  = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_HIGH]         = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_LOW]          = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_NOTICE_HIGH]  = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_NOTICE_LOW]   = {DFD_CFG_ITEM_HWMON_IN, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
};

dfd_debug_data_key_map_t curr_dbg_key_table[WB_SENSOR_END]= {
    [WB_SENSOR_INPUT]        = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_ALIAS]        = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_STR_DATA},
    [WB_SENSOR_TYPE]         = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_MAX]          = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_MAX_HYST]     = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_MIN]          = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_CRIT]         = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_RANGE]        = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_NOMINAL_VAL]  = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_HIGH]         = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_LOW]          = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_NOTICE_HIGH]  = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_NOTICE_LOW]   = {DFD_CFG_ITEM_HWMON_CURR, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
};

dfd_debug_data_key_map_t temp_dbg_key_table[WB_SENSOR_END]= {
    [WB_SENSOR_INPUT]        = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_ALIAS]        = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_STR_DATA},
    [WB_SENSOR_TYPE]         = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_MAX]          = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_MAX_HYST]     = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_MIN]          = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_CRIT]         = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_RANGE]        = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_NOMINAL_VAL]  = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_HIGH]         = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_LOW]          = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_NOTICE_HIGH]  = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
    [WB_SENSOR_NOTICE_LOW]   = {DFD_CFG_ITEM_HWMON_TEMP, CFG_INDEX1_INDEX2_CMB_1, CFG_INT_DATA},
};

static const dfd_status_desc_map_t dfd_sensor_vol_status_map[] = {
    {VOL_STATUS_ABNORMAL, "Voltage_Abnormal"},
};

static int dfd_get_hwmon_in_access_count(uint16_t key_index1, bool is_pre)
{
    int *p_access_count;
    uint64_t key;
    int access_count;

    key = DFD_CFG_KEY((is_pre ? DFD_CFG_ITEM_HWMON_IN_PRE_ACCESS_NUM : DFD_CFG_ITEM_HWMON_IN_POST_ACCESS_NUM),
        key_index1, 0);
    p_access_count = dfd_ko_cfg_get_item(key);
    if (p_access_count == NULL) {
        return 0;
    }

    access_count = *p_access_count;
    if ((access_count <= 0) || (access_count > DFD_VOL_INPUT_ACCESS_MAX_COUNT)) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "%s hwmon in access count invalid, key: 0x%08llx, count: %d\n",
            is_pre ? "pre" : "post", key, access_count);
        return -DFD_RV_INVALID_VALUE;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "%s hwmon in access count key: 0x%08llx, count: %d\n",
        is_pre ? "pre" : "post", key, access_count);
    return access_count;
}

static int dfd_run_hwmon_in_access_single_action(uint16_t key_index1, uint8_t key_index2, bool is_pre)
{
    int rv;
    uint64_t key;
    info_ctrl_t *info_ctrl;

    key = DFD_CFG_KEY((is_pre ? DFD_CFG_ITEM_HWMON_IN_PRE_ACCESS : DFD_CFG_ITEM_HWMON_IN_POST_ACCESS),
        key_index1, key_index2);

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "hwmon in access action key: 0x%08llx\n", key);

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "%s hwmon in access action not configured, key: 0x%08llx\n",
            is_pre ? "pre" : "post", key);
        return DFD_RV_OK;
    }

    if (info_ctrl->mode != INFO_CTRL_MODE_CFG) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "%s hwmon in access mode invalid, key: 0x%08llx, mode: %d\n",
            is_pre ? "pre" : "post", key, info_ctrl->mode);
        return -DFD_RV_TYPE_ERR;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "before access action, key: 0x%08llx, int_cons: %d\n", key, info_ctrl->int_cons);
    rv = dfd_info_set_int(key, info_ctrl->int_cons);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "%s hwmon in access action failed, key: 0x%08llx, rv: %d\n",
            is_pre ? "pre" : "post", key, rv);
        return rv;
    }

    return DFD_RV_OK;
}

static int dfd_run_hwmon_in_access_action(uint16_t key_index1, uint8_t key_index2, bool is_pre)
{
    int rv;
    int access_count;
    int action_index;

    access_count = dfd_get_hwmon_in_access_count(key_index1, is_pre);
    if (access_count <= 0) {
        return access_count;
    }

    for (action_index = 0; action_index < access_count; action_index++) {
        rv = dfd_run_hwmon_in_access_single_action(key_index1, action_index, is_pre);
        if (rv < 0) {
            DFD_SENSOR_DEBUG(DBG_ERROR, "%s hwmon in access action[%d] failed, key_index1: 0x%x, rv: %d\n",
                is_pre ? "pre" : "post", action_index, key_index1, rv);
            return rv;
        }
    }

    return DFD_RV_OK;
}

typedef enum {
    WB_SENSER_VOL_FUNC_GET_STATUS_DIRECT = 0,
    WB_SENSER_VOL_FUNC_END,
} wb_sensor_vol_func_type_t;

typedef enum {
    WB_SENSER_POWER_FUNC_ORGINAL = 0,
    WB_SENSER_POWER_FUNC_IN_CURR_MULTI,
    WB_SENSER_POWER_FUNC_END,
} wb_sensor_power_func_type_t;

typedef enum hwmon_multi_temps_mode_en {
    HWMON_MULTI_TEMPS_MODE_MIN = 0,
    HWMON_MULTI_TEMPS_MODE_AVERAGE = 1,
    HWMON_MULTI_TEMPS_MODE_MAX = 2,
    HWMON_MULTI_TEMPS_MODE_END,
} hwmon_multi_temps_mode_t;

#if 0
static int dfd_get_sensor_median_by_key(uint64_t key, char *buf, size_t count);
#endif
static int dfd_get_median_value(int *values, int value_num, int *median);

static int dfd_deal_hwmon_buf(uint8_t *buf, int buf_len, uint8_t *buf_new, int *buf_len_new,
               info_ctrl_t *info_ctrl, int coefficient, int addend)
{
    int i, tmp_len;
    int exp, decimal, divisor;
    int org_value, tmp_value;
    int div_result, div_mod;
    char fmt_str[DFD_FORMAT_STR_MAX_LEN];

    exp = info_ctrl->int_cons;         /* Numerical conversion index */
    decimal = info_ctrl->bit_offset;   /* Decimal point retention number */

    /* No conversion is required, just copy the value */
    if ((exp <= 0) && (coefficient == 1) && (addend == 0)) {
        DBG_DEBUG(DBG_VERBOSE, "exponent %d, coefficient: %d, addend: %d, don't need transform, buf_len: %d, buf_len_new: %d\n",
            exp, coefficient, addend, buf_len, *buf_len_new);
        snprintf(buf_new, *buf_len_new, "%s", buf);
        *buf_len_new = strlen(buf_new);
        return DFD_RV_OK;
    }

    divisor = 1;
    for (i = 0; i < exp; i++) {
        divisor *= 10;
    }
    org_value = simple_strtol(buf, NULL, 10);
    DBG_DEBUG(DBG_VERBOSE, "original value: %d, exp: %d, divisor: %d, decimal: %d, coefficient: %d, addend: %d\n",
        org_value, exp, divisor, decimal, coefficient, addend);

    org_value = (org_value + addend) * coefficient;
    if (org_value < 0) {
        tmp_value = 0 - org_value;
    } else {
        tmp_value = org_value;
    }
    div_result = tmp_value / divisor;
    div_mod = tmp_value % divisor;
    DBG_DEBUG(DBG_VERBOSE, "tmp_value: %d, divisor: %d, div_result: %d, div_mod: %d\n",
        tmp_value, divisor, div_result, div_mod);
    /* don't need to keep the decimal, just round it */
    if (decimal == 0) {
        snprintf(buf_new, *buf_len_new, "%d\n", org_value < 0 ? -div_result : div_result);
        *buf_len_new = strlen(buf_new);
        return DFD_RV_OK;
    }
    mem_clear(fmt_str, sizeof(fmt_str));
    if (org_value < 0) {
        snprintf(fmt_str, sizeof(fmt_str), "-%%d.%%0%dd\n",exp);
    } else {
        snprintf(fmt_str, sizeof(fmt_str), "%%d.%%0%dd\n",exp);
    }
    DBG_DEBUG(DBG_VERBOSE, "format string: %s",fmt_str);
    snprintf(buf_new, *buf_len_new, fmt_str, div_result, div_mod);
    *buf_len_new = strlen(buf_new);
    tmp_len = *buf_len_new;
    /* Keep decimal places only when the number of decimal places is reduced */
    if (decimal > 0) {
        for (i = 0; i < *buf_len_new; i++) {
            if (buf_new[i] == '.') {
                if ( i + decimal + 2 <= *buf_len_new ) {
                    buf_new[i + decimal + 1 ] = '\n';
                    buf_new[i + decimal + 2 ] = '\0';
                    *buf_len_new = strlen(buf_new);
                    DBG_DEBUG(DBG_VERBOSE, "deal decimal[%d] ok, str len:%d, value:%s\n",
                        decimal, *buf_len_new, buf_new);
                }
                break;
            }
        }
        if (tmp_len == *buf_len_new) {
            DBG_DEBUG(DBG_WARN, "deal decimal[%d] failed, use original value:%s\n", decimal,
                buf_new);
        }
    }
    return DFD_RV_OK;
}

static int dfd_check_valid_flag_key(uint64_t sensor_valid_flag_key)
{
    int rv;
    info_ctrl_t *info_ctrl;
    int sensor_valid_flag, sensor_valid_val;

    rv = 0;
    info_ctrl = dfd_ko_cfg_get_item(sensor_valid_flag_key);
    if (info_ctrl == NULL) {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "no cfg %s, think it is valid default\n", key_to_name(DFD_CFG_ITEM_ID(sensor_valid_flag_key)));
        return rv;
    } else {
        sensor_valid_flag = 0;
        rv = dfd_info_get_int(sensor_valid_flag_key, &sensor_valid_flag, NULL);
        if (rv < 0) {
            DFD_SENSOR_DEBUG(DBG_ERROR, "get sensor_valid_flag fail, key_name: %s, rv: %d\n",
                key_to_name(DFD_CFG_ITEM_ID(sensor_valid_flag_key)), rv);
            return rv;
        }
        sensor_valid_val = info_ctrl->int_cons;
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "sensor_valid_flag: %d, rv:%d, sensor_valid_val: %d\n",
                sensor_valid_flag, rv, sensor_valid_val);

        if (sensor_valid_flag != sensor_valid_val) {
            rv = -DFD_RV_INVALID_VALUE;
            DFD_SENSOR_DEBUG(DBG_ERROR, "sensor is invalid, do not get sensor info. key_name: %s\n",
                key_to_name(DFD_CFG_ITEM_ID(sensor_valid_flag_key)));
            return rv;
        } else {
            rv = 0;
        }
    }
    return rv;
}

static int dfd_get_sensor_median_cfg_item(uint8_t sensor_type, int *median_cfg_item)
{
    if (median_cfg_item == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "median_cfg_item is NULL\n");
        return -DFD_RV_INVALID_VALUE;
    }

    switch (sensor_type) {
    case WB_MINOR_DEV_TEMP:
        *median_cfg_item = DFD_CFG_ITEM_TEMP_SENSOR_GET_MEDIAN;
        break;
    case WB_MINOR_DEV_IN:
        *median_cfg_item = DFD_CFG_ITEM_VOL_SENSOR_GET_MEDIAN;
        break;
    case WB_MINOR_DEV_CURR:
        *median_cfg_item = DFD_CFG_ITEM_CURR_SENSOR_GET_MEDIAN;
        break;
    case WB_MINOR_DEV_POWER:
        *median_cfg_item = DFD_CFG_ITEM_POWER_SENSOR_GET_MEDIAN;
        break;
    default:
        DFD_SENSOR_DEBUG(DBG_ERROR, "Unknow sensor type: %u\n", sensor_type);
        return -DFD_RV_INVALID_VALUE;
    }

    return DFD_RV_OK;
}

static int dfd_sensor_need_get_median(uint8_t sensor_type, uint16_t key_index1, uint8_t key_index2)
{
    int rv;
    int median_cfg_item;
    uint64_t median_key;
    info_ctrl_t *median_cfg;

    rv = dfd_get_sensor_median_cfg_item(sensor_type, &median_cfg_item);
    if (rv < 0) {
        return 0;
    }

    median_key = DFD_CFG_KEY(median_cfg_item, key_index1, key_index2);
    median_cfg = dfd_ko_cfg_get_item(median_key);
    if (median_cfg == NULL) {
        return 0;
    }

    return 1;
}

static int dfd_get_power_original_info(uint16_t key_index1, uint8_t key_index2, char *buf, size_t count)
{
    int rv;
    uint64_t key, sensor_valid_flag_key;
    info_hwmon_buf_f pfunc;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_POWER, key_index1, key_index2);
    sensor_valid_flag_key = DFD_CFG_KEY(DFD_CFG_ITEM_POWER_VALID_FLAG, key_index1, key_index2);
    rv = dfd_check_valid_flag_key(sensor_valid_flag_key);
    if (rv != 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "Failed to get sensor_valid_flag, key_name: %s, rv: %d\n",
            key_to_name(DFD_CFG_ITEM_ID(sensor_valid_flag_key)), rv);
        return rv;
    }

    pfunc = dfd_deal_hwmon_buf;
    rv = dfd_info_get_sensor(key, buf, count, pfunc);
    return rv;
}

static int dfd_get_power_vol_curr_multi_info(uint16_t key_index1, uint8_t key_index2, char *buf, size_t count)
{
    int rv;
    uint64_t key, sensor_valid_flag_key;
    info_hwmon_buf_f pfunc;
    char buf_tmp[INFO_INT_MAX_LEN] = {0};
    int vol_tmp, curr_tmp;
    long long int power_tmp;

    rv = 0;
    vol_tmp = 0;
    curr_tmp = 0;
    power_tmp = 0;

    sensor_valid_flag_key = DFD_CFG_KEY(DFD_CFG_ITEM_POWER_VALID_FLAG, key_index1, key_index2);
    rv = dfd_check_valid_flag_key(sensor_valid_flag_key);
    if (rv != 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "Failed to get sensor_valid_flag, key_name: %s, rv: %d\n",
            key_to_name(DFD_CFG_ITEM_ID(sensor_valid_flag_key)), rv);
        return rv;
    }

    pfunc = dfd_deal_hwmon_buf;
    /* 1. fetch vol value */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_POWER_IN_MULTI, key_index1, key_index2);
    rv = dfd_info_get_sensor(key, buf_tmp, sizeof(buf_tmp), pfunc);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "Failed to get sensor_vol, key_name: %s, rv: %d\n",
            key_to_name(DFD_CFG_ITEM_ID(key)), rv);
        return rv;
    }
    vol_tmp = simple_strtol(buf_tmp, NULL, 10);

    /* 2. fetch curr value */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_POWER_CURR_MULTI, key_index1, key_index2);
    rv = dfd_info_get_sensor(key, buf_tmp, sizeof(buf_tmp), pfunc);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "Failed to get sensor_curr, key_name: %s, rv: %d\n",
            key_to_name(DFD_CFG_ITEM_ID(key)), rv);
        return rv;
    }
    curr_tmp = simple_strtol(buf_tmp, NULL, 10);

    power_tmp = (long long int)vol_tmp * curr_tmp;

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "vol_tmp:%d, curr_tmp:%d, power_tmp:%lld\n", vol_tmp, curr_tmp, power_tmp);

    rv = snprintf(buf, count, "%lld", power_tmp);

    return rv;
}

static int dfd_get_power_input_info(uint8_t main_dev_id, uint16_t key_index1, uint8_t key_index2, char *buf, size_t count)
{
    int rv;
    uint8_t key_index_tmp;
    uint64_t key;
    char buf_tmp[INFO_INT_MAX_LEN] = {0};
    int func_index;

    func_index = 0;

    key_index_tmp = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, WB_SENSOR_FUNC);

    /* 1. get function index */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_POWER, key_index1, key_index_tmp);
    rv = dfd_info_get_sensor(key, buf_tmp, sizeof(buf_tmp), NULL);
    if (rv == -DFD_RV_DEV_NOTSUPPORT) {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "sensor function index is invalid, do not get sensor info. key_name: %s, key_index1: 0x%x, key_index_tmp: 0x%x\n",
            key_to_name(DFD_CFG_ITEM_ID(key)), key_index1, key_index_tmp);
        return dfd_get_power_original_info(key_index1, key_index2, buf, count);
    }

    func_index = simple_strtol(buf_tmp, NULL, 10);

    if (func_index < WB_SENSER_POWER_FUNC_ORGINAL || func_index >= WB_SENSER_POWER_FUNC_END) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "sensor function index is invalid, do not get sensor info. func_index: %d\n",
            func_index);
        return -EINVAL;
    }

    switch (func_index)
    {
    case WB_SENSER_POWER_FUNC_ORGINAL:
        return dfd_get_power_original_info(key_index1, key_index2, buf, count);
    case WB_SENSER_POWER_FUNC_IN_CURR_MULTI:
        return dfd_get_power_vol_curr_multi_info(key_index1, key_index2, buf, count);
    default:
        DFD_SENSOR_DEBUG(DBG_ERROR, "sensor function index is invalid, do not get sensor info. func_index: %d\n",
            func_index);
        return -EINVAL;
    }
}

static ssize_t dfd_get_vol_status_direct(uint8_t main_dev_id, uint8_t dev_index,
                unsigned int sensor_index, char *buf, size_t count)
{
    int rv, ret;
    uint64_t key;
    char buf_tmp[INFO_INT_MAX_LEN];
    int func_index;
    int vol_status;
    int print_len;
    uint16_t key_index1;
    uint8_t key_index2;
    char *key_name;

    func_index = -1;
    key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, sensor_index);
    key_index2 = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, WB_SENSOR_FUNC);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_IN, key_index1, key_index2);
    key_name = key_to_name(DFD_CFG_ITEM_ID(key));

    mem_clear(buf_tmp, INFO_INT_MAX_LEN);
    rv = dfd_info_get_sensor(key, buf_tmp, sizeof(buf_tmp), NULL);
    if (rv < 0) {
        DFD_SENSOR_DEBUG((rv == -DFD_RV_DEV_NOTSUPPORT ? DBG_VERBOSE : DBG_ERROR),
                        "%s to get sensor function, key_name: %s, key_index1: 0x%x, "
                        "key_index2: 0x%x, rv: %d\n",
                        rv == -DFD_RV_DEV_NOTSUPPORT ? "Not support" : "Failed",
                        key_name, key_index1, key_index2, rv);
        return rv;
    }

    ret = kstrtoint(buf_tmp, 0, &func_index);
    if (ret != 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "invaild func index ret: %d, buf: %s.\n", ret, buf_tmp);
        return -EINVAL;
    }
    if (func_index != WB_SENSER_VOL_FUNC_GET_STATUS_DIRECT) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "Not support func index: %d\n", func_index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    /* Read the raw input status value directly instead of deriving it later. */
    key_index2 = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, WB_SENSOR_INPUT);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_IN, key_index1, key_index2);
    key_name = key_to_name(DFD_CFG_ITEM_ID(key));
    vol_status = VOL_STATUS_ABNORMAL;   /* default fail. */
    rv = dfd_info_get_int(key, &vol_status, NULL);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "do not get sensor info, rv = %d."
            " key_name: %s, key_index1: 0x%x, key_index2: 0x%x\n",
            rv, key_name, key_index1, key_index2);
        return rv;
    }
    DFD_SENSOR_DEBUG(DBG_VERBOSE, "get sensor status directly, vol_status: %d."
        " key_name: %s, key_index1: 0x%x, key_index2: 0x%x\n",
        vol_status, key_name, key_index1, key_index2);

    DFD_STATUS_DETAIL_INFO_SET(dfd_sensor_vol_status_map, vol_status, VOL_STATUS_OK, false, buf,
        count, NULL, 0);
    print_len = strlen(buf);
    if (print_len >= (int)count) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "len = %d, count = %d\n", print_len, (int)count);
        return -ENOSPC;
    }

    return (ssize_t)print_len;
}

static ssize_t dfd_query_vol_status_via_validity_check(uint8_t main_dev_id, uint8_t dev_index,
                unsigned int sensor_index, char *buf, size_t count)
{
    ssize_t ret;
    int rv;
    char buf_tmp[INFO_INT_MAX_LEN];
    char extra_info[DFD_DETAIL_INFO_BUF_MAX_LEN];
    int current_vol;
    int max_vol;
    int min_vol;
    size_t extra_info_len;

    current_vol = 0;
    max_vol = 0;
    min_vol = 0;

    /* 1. get current vol */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = dfd_get_voltage_info(main_dev_id, dev_index, sensor_index, WB_SENSOR_INPUT,
              buf_tmp, sizeof(buf_tmp));
    if (ret < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "failed to get current vol, main_dev_id=%u, dev_index: %u, sensor_index: %u, senser_type: %u\n",
            main_dev_id, dev_index, sensor_index, WB_SENSOR_INPUT);
        return ret;
    } else {
        rv = kstrtoint(buf_tmp, 0, &current_vol);
        if (rv != 0) {
            DFD_SENSOR_DEBUG(DBG_ERROR, "invaild current vol ret: %zd, buf: %s.\n", ret, buf_tmp);
            return -EINVAL;
        }
    }

    /* 2. get max */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = dfd_get_voltage_info(main_dev_id, dev_index, sensor_index, WB_SENSOR_MAX,
              buf_tmp, sizeof(buf_tmp));
    if (ret < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "failed to get max vol, main_dev_id=%u, dev_index: %u, sensor_index: %u, senser_type: %u\n",
            main_dev_id, dev_index, sensor_index, WB_SENSOR_MAX);
        return ret;
    } else {
        rv = kstrtoint(buf_tmp, 0, &max_vol);
        if (rv != 0) {
            DFD_SENSOR_DEBUG(DBG_ERROR, "invaild max vol ret: %zd, buf: %s.\n", ret, buf_tmp);
            return -EINVAL;
        }
    }

    /* 3. get min */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = dfd_get_voltage_info(main_dev_id, dev_index, sensor_index, WB_SENSOR_MIN,
              buf_tmp, sizeof(buf_tmp));
    if (ret < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "failed to get min vol, main_dev_id=%u, dev_index: %u, sensor_index: %u, senser_type: %u\n",
            main_dev_id, dev_index, sensor_index, WB_SENSOR_MIN);
        return ret;
    } else {
        rv = kstrtoint(buf_tmp, 0, &min_vol);
        if (rv != 0) {
            DFD_SENSOR_DEBUG(DBG_ERROR, "invaild min vol ret: %zd, buf: %s.\n", ret, buf_tmp);
            return -EINVAL;
        }
    }

    if (current_vol >= max_vol || current_vol <= min_vol) {
        mem_clear(extra_info, sizeof(extra_info));
        extra_info_len = snprintf(extra_info, sizeof(extra_info), "(%dmv %s)", current_vol,
                        (current_vol >= max_vol)
                            ? "Exceed the upper threshold" : "Fall below the lower threshold");
        DFD_STATUS_DETAIL_INFO_SET(dfd_sensor_vol_status_map, VOL_STATUS_ABNORMAL, VOL_STATUS_OK,
            false, buf, count, extra_info, extra_info_len);
        return (ssize_t)strlen(buf);
    }

    return (ssize_t)snprintf(buf, count, "%d\n", VOL_STATUS_OK);
}

static int dfd_get_multi_temp_mode(uint8_t dev_index, uint8_t sensor_index)
{
    int *p_multi_temp_mode;
    uint16_t key_index1;
    uint64_t key;

    key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, sensor_index);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_MULTI_TEMPS_MODE, key_index1, 0);
    p_multi_temp_mode = dfd_ko_cfg_get_item(key);
    if (p_multi_temp_mode == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get multi temp mode fail, dev_index:0x%x, sensor_index:0x%u\n",
                                    dev_index, sensor_index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (*p_multi_temp_mode >= HWMON_MULTI_TEMPS_MODE_END || *p_multi_temp_mode < HWMON_MULTI_TEMPS_MODE_MIN) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "Unknow multi temp mode: %d\n", *p_multi_temp_mode);
        return -DFD_RV_INVALID_VALUE;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "temp%u's multi_temp_mode:%d\n", sensor_index, *p_multi_temp_mode);
    return *p_multi_temp_mode;
}

static int dfd_get_arr_min_max_average(int *p_arr, int arr_unit_num, int *p_min, int *p_max, int *p_average)
{
    int i, sum;

    if (p_arr == NULL || p_min == NULL || p_max == NULL || p_average == NULL || arr_unit_num <= 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "invalid para: %p %p %p %p %d\n", p_arr, p_min, p_max, p_average, arr_unit_num);
        return -DFD_RV_INVALID_VALUE;
    }

    *p_max = p_arr[0];
    *p_min = p_arr[0];
    sum = 0;

    for (i = 0; i < arr_unit_num; i++) {
        if (p_arr[i] > *p_max) {
            *p_max = p_arr[i];
        }
        if (p_arr[i] < *p_min) {
            *p_min = p_arr[i];
        }
        sum += p_arr[i];
    }

    *p_average= sum / arr_unit_num;
    return DFD_RV_OK;
}

static int dfd_get_median_value(int *values, int value_num, int *median)
{
    int i;
    int j;
    int tmp;
    int sorted_values[TEMP_MEDIAN_MAX_SAMPLE_NUM];

    if (values == NULL || median == NULL || value_num <= 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "invalid para values:%p median:%p value_num:%d\n",
            values, median, value_num);
        return -DFD_RV_INVALID_VALUE;
    }

    if ((value_num % 2) == 0 || value_num > TEMP_MEDIAN_MAX_SAMPLE_NUM) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "invalid value_num:%d, expect odd and <= %d\n",
            value_num, TEMP_MEDIAN_MAX_SAMPLE_NUM);
        return -DFD_RV_INVALID_VALUE;
    }

    for (i = 0; i < value_num; i++) {
        sorted_values[i] = values[i];
    }

    for (i = 0; i < value_num - 1; i++) {
        for (j = i + 1; j < value_num; j++) {
            if (sorted_values[i] > sorted_values[j]) {
                tmp = sorted_values[i];
                sorted_values[i] = sorted_values[j];
                sorted_values[j] = tmp;
            }
        }
    }

    *median = sorted_values[value_num / 2];
    return DFD_RV_OK;
}

#if 0
static int dfd_get_sensor_median_by_key(uint64_t key, char *buf, size_t count)
{
    int i, rv;
    int temp_arr[TEMP_MEDIAN_SAMPLE_NUM];
    int median;
    char temp_buf[INFO_INT_MAX_LEN];
    info_hwmon_buf_f pfunc;

    pfunc = dfd_deal_hwmon_buf;
    for (i = 0; i < TEMP_MEDIAN_SAMPLE_NUM; i++) {
        mem_clear(temp_buf, sizeof(temp_buf));
        rv = dfd_info_get_sensor(key, temp_buf, sizeof(temp_buf), pfunc);
        if (rv < 0) {
            DFD_SENSOR_DEBUG(DBG_ERROR, "read temp for median failed, try %d, rv:%d\n", i + 1, rv);
            return rv;
        }
        temp_arr[i] = simple_strtol(temp_buf, NULL, 10);
    }

    rv = dfd_get_median_value(temp_arr, TEMP_MEDIAN_SAMPLE_NUM, &median);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get median temp failed, rv:%d\n", rv);
        return rv;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "temp median from %d reads => %d\n",
        TEMP_MEDIAN_SAMPLE_NUM, median);
    return snprintf(buf, count, "%d\n", median);
}
#endif

static int dfd_get_one_multi_temp(uint8_t dev_index, uint8_t sensor_index, uint8_t sub_temp_index, int *p_temp_val)
{
    int rv;
    uint16_t key_index1;
    uint64_t key;
    char buf_tmp[TEMP_DATA_BUF_LEN];
    info_ctrl_t *key_info_ctrl;
    info_hwmon_buf_f pfunc;

    /* 1. judge input para */
    if (p_temp_val == NULL) {
         DFD_SENSOR_DEBUG(DBG_ERROR, "null point\n");
        return -DFD_RV_INVALID_VALUE;
    }

    DFD_SENSOR_DEBUG(DBG_ERROR, "dev_index:%u, sensor_index:%u, sub_temp_index:%u\n",
                     dev_index, sensor_index, sub_temp_index);

    /* 2. judge whether the cfg exists */
    key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, sensor_index);
    key =  DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_MULTI_TEMP, key_index1, sub_temp_index);
    key_info_ctrl = dfd_ko_cfg_get_item(key);
    if (key_info_ctrl == NULL) {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "no find dfd cfg: hwmon_multi_temp_0x%04x_0x%02x\n", key_index1, sub_temp_index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    /* 3. get temp to buf_tmp */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    pfunc = dfd_deal_hwmon_buf;
    rv = dfd_info_get_sensor(key, buf_tmp, sizeof(buf_tmp), pfunc);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "multi_temp%u sub_temp%u read fail, rv:%d\n", sensor_index, sub_temp_index, rv);
        return -DFD_RV_DEV_FAIL;
    }
    DFD_SENSOR_DEBUG(DBG_VERBOSE, "buf_tmp:%s\n", buf_tmp);

    /* 4. switch buf info to int data */
    rv = sscanf(buf_tmp, "%d", p_temp_val);
    if (rv != 1) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "multi_temp%u sub_temp%u switch temp data fail, rv:%d\n",
                                     sensor_index, sub_temp_index, rv);
        return -DFD_RV_DEV_FAIL;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "multi_temp%u sub_temp%u temp val: %d\n", sensor_index, sub_temp_index, *p_temp_val);
    return DFD_RV_OK;
}

static int dfd_get_temp_val_by_multi_temp_mode(int multi_temp_mode, uint8_t dev_index, uint8_t sensor_index,
                                  char *buf, size_t count)
{
    int i, rv, temp_val;
    uint8_t valid_temp_num;
    int multi_temps[MULTI_TEMPS_NUM_MAX];
    int max, min, average;

    mem_clear(multi_temps, sizeof(multi_temps));
    valid_temp_num = 0;
    for (i = 0; i < MULTI_TEMPS_NUM_MAX; i++) {
        temp_val = 0;
        rv = dfd_get_one_multi_temp(dev_index, sensor_index, i, &temp_val);
        if (rv != DFD_RV_OK) {
            if (rv == -DFD_RV_DEV_NOTSUPPORT) {
                /* cfg nonexist indicates that the final cfg has been reached, break */
                break;
            } else {
                /* read temp fail, skip to read next */
                continue;
            }
        }

        multi_temps[valid_temp_num] = temp_val;
        valid_temp_num++;
    }

    if (valid_temp_num == 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "no valid temp\n");
        return -DFD_RV_DEV_FAIL;
    }

    rv = dfd_get_arr_min_max_average(multi_temps, valid_temp_num, &min, &max, &average);
    if (rv) {
        return -DFD_RV_INVALID_VALUE;
    }
    DFD_SENSOR_DEBUG(DBG_VERBOSE, "multi_temp%u: min:%d, max:%d, average:%d\n", sensor_index, min, max, average);

    if (multi_temp_mode == HWMON_MULTI_TEMPS_MODE_MIN) {
        return snprintf(buf, count, "%d\n", min);
    } else if (multi_temp_mode == HWMON_MULTI_TEMPS_MODE_MAX) {
        return snprintf(buf, count, "%d\n", max);
    } else {
        return snprintf(buf, count, "%d\n", average);
    }
}

static int dfd_get_sensor_info_once(uint8_t main_dev_id, uint8_t dev_index, uint8_t sensor_type,
               uint8_t sensor_index, uint8_t sensor_attr, char *buf, size_t count)
{
    uint64_t key, sensor_valid_flag_key;
    uint16_t key_index1;
    uint8_t key_index2;
    int rv;
    info_hwmon_buf_f pfunc;
    int multi_temp_mode;

    key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, sensor_index);
    key_index2 = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, sensor_attr);

    if (sensor_type == WB_MINOR_DEV_POWER && sensor_attr == WB_SENSOR_INPUT) {
        return dfd_get_power_input_info(main_dev_id, key_index1, key_index2, buf, count);
    }

    if (sensor_type == WB_MINOR_DEV_TEMP) {
        if (sensor_attr == WB_SENSOR_INPUT) {
            multi_temp_mode = dfd_get_multi_temp_mode(dev_index, sensor_index);
            if (multi_temp_mode < 0) {
                /* multi_temp_mode not exist, read temp by default way,  */
                if (multi_temp_mode == -DFD_RV_DEV_NOTSUPPORT) {
                    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_TEMP, key_index1, key_index2);
                    sensor_valid_flag_key = DFD_CFG_KEY(DFD_CFG_ITEM_TEMP_VALID_FLAG, key_index1, key_index2);
                } else {
                    return -DFD_RV_INVALID_VALUE;
                }
            } else {
                return dfd_get_temp_val_by_multi_temp_mode(multi_temp_mode, dev_index, sensor_index, buf, count);
            }
        } else {
            key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_TEMP, key_index1, key_index2);
            sensor_valid_flag_key = DFD_CFG_KEY(DFD_CFG_ITEM_TEMP_VALID_FLAG, key_index1, key_index2);
        }
    } else if (sensor_type == WB_MINOR_DEV_IN) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_IN, key_index1, key_index2);
        sensor_valid_flag_key = DFD_CFG_KEY(DFD_CFG_ITEM_VOL_VALID_FLAG, key_index1, key_index2);
    } else if (sensor_type == WB_MINOR_DEV_CURR) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_CURR, key_index1, key_index2);
        sensor_valid_flag_key = DFD_CFG_KEY(DFD_CFG_ITEM_CURR_VALID_FLAG, key_index1, key_index2);
    } else if (sensor_type == WB_MINOR_DEV_POWER) {
           return dfd_get_power_original_info(key_index1, key_index2, buf, count);
    } else {
        DFD_SENSOR_DEBUG(DBG_ERROR, "Unknow sensor type: %u\n",sensor_type);
        return -DFD_RV_INVALID_VALUE;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "main_dev_id: %u, dev_index: 0x%x, sensor_type: %d, \
        sensor_index: 0x%x,  sensor_attr: 0x%x, key: 0x%08llx, flag_key: 0x%08llx\\n",
        main_dev_id, dev_index, sensor_type, sensor_index, sensor_attr, key, sensor_valid_flag_key);

    rv = dfd_check_valid_flag_key(sensor_valid_flag_key);
    if (rv != 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "failed to run dfd_check_valid_flag_key , rv: %d\n", rv);
        return rv;
    }

    pfunc = dfd_deal_hwmon_buf;
    rv = dfd_info_get_sensor(key, buf, count, pfunc);
    return rv;
}

static int dfd_get_sensor_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t sensor_type,
               uint8_t sensor_index, uint8_t sensor_attr, char *buf, size_t count)
{
    int i, rv;
    uint16_t key_index1;
    uint8_t key_index2;
    int values[TEMP_MEDIAN_SAMPLE_NUM];
    int median;
    char tmp_buf[INFO_INT_MAX_LEN];

    key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, sensor_index);
    key_index2 = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, sensor_attr);

    if (!dfd_sensor_need_get_median(sensor_type, key_index1, key_index2)) {
        return dfd_get_sensor_info_once(main_dev_id, dev_index, sensor_type,
                   sensor_index, sensor_attr, buf, count);
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "sensor median mode, sensor_type: %u, key_index1: 0x%x, "
        "key_index2: 0x%x\n", sensor_type, key_index1, key_index2);

    for (i = 0; i < TEMP_MEDIAN_SAMPLE_NUM; i++) {
        mem_clear(tmp_buf, sizeof(tmp_buf));
        rv = dfd_get_sensor_info_once(main_dev_id, dev_index, sensor_type,
                 sensor_index, sensor_attr, tmp_buf, sizeof(tmp_buf));
        if (rv < 0) {
            DFD_SENSOR_DEBUG(DBG_ERROR, "get sensor info for median failed, try %d, rv: %d\n",
                i + 1, rv);
            return rv;
        }
        values[i] = simple_strtol(tmp_buf, NULL, 10);
    }

    rv = dfd_get_median_value(values, TEMP_MEDIAN_SAMPLE_NUM, &median);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get median value failed, rv: %d\n", rv);
        return rv;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "sensor median from %d reads => %d\n",
        TEMP_MEDIAN_SAMPLE_NUM, median);
    return snprintf(buf, count, "%d\n", median);
}

/**
 * dfd_get_temp_info - Get temperature information
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1/psu1
 * @temp_index: Temperature index, starting at 1
 * @temp_type: Read type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_temp_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t temp_index,
            uint8_t temp_attr, char *buf, size_t count)
{
    int rv;

    if (buf == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "param error, buf is NULL\n");
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    rv = dfd_get_sensor_info(main_dev_id, dev_index, WB_MINOR_DEV_TEMP, temp_index, temp_attr,
             buf, count);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get temp info error, rv: %d\n", rv);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "get temp info success, value: %s\n", buf);
    }
    return rv;
}

/**
 * dfd_get_voltage_info - Get voltage information
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @in_index: Voltage index, starting at 1
 * @in_type: Voltage type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_voltage_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t in_index,
            uint8_t in_attr, char *buf, size_t count)
{
    int rv;
    int post_rv;
    uint16_t key_index1;
    uint8_t key_index2;
    bool need_access;

    if (buf == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, in_index);
    key_index2 = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, in_attr);
    need_access = (in_attr == WB_SENSOR_INPUT);

    if (need_access) {
        rv = dfd_run_hwmon_in_access_action(key_index1, key_index2, true);
        if (rv < 0) {
            DFD_SENSOR_DEBUG(DBG_ERROR, "pre hwmon in access action failed, rv: %d\n", rv);
            return rv;
        }
    }

    rv = dfd_get_sensor_info(main_dev_id, dev_index, WB_MINOR_DEV_IN, in_index, in_attr, buf,
             count);

    if (need_access) {
        post_rv = dfd_run_hwmon_in_access_action(key_index1, key_index2, false);
        if (rv >= 0 && post_rv < 0) {
            rv = post_rv;
        } else if (rv < 0 && post_rv < 0) {
            DFD_SENSOR_DEBUG(DBG_ERROR, "post hwmon in access action failed after value read error, rv: %d\n",
                post_rv);
        }
    }

    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get voltage info error, rv: %d\n", rv);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "get voltage info success, value: %s\n", buf);
    }

    return rv;
}

/**
 * dfd_get_power_info - Get power information
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @in_index: Power index, starting at 1
 * @in_type: Power type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_power_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t in_index,
            uint8_t in_attr, char *buf, size_t count)
{
    int rv;

    if (buf == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }
    rv = dfd_get_sensor_info(main_dev_id, dev_index, WB_MINOR_DEV_POWER, in_index, in_attr, buf,
             count);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get power info error, rv: %d\n", rv);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "get power info success, value: %s\n", buf);
    }
    return rv;
}

/**
 * dfd_get_current_info - Get current information
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @in_index: Current index, starting at 1
 * @in_type: Current type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_current_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t curr_index,
            uint8_t curr_attr, char *buf, size_t count)
{
    int rv;

    if (buf == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }
    rv = dfd_get_sensor_info(main_dev_id, dev_index, WB_MINOR_DEV_CURR, curr_index, curr_attr,
             buf, count);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get current info error, rv: %d\n", rv);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "get current info success, value: %s\n", buf);
    }
    return rv;
}

/**
 * dfd_get_psu_sensor_info - Obtain PMBUS information about the power supply
 * @psu_index: Power index, starting at 1
 * @sensor_type: Type of the obtained pmbus information
 * @buf: pmbus results are stored in buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_psu_sensor_info(uint8_t psu_index, uint8_t sensor_type, char *buf, size_t count)
{
    uint64_t key;
    int rv;
    info_hwmon_buf_f pfunc;

    if (buf == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "param error. buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }
    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_PSU, psu_index, sensor_type);
    DFD_SENSOR_DEBUG(DBG_VERBOSE, "psu index: %d, sensor type: %d, key_name: %s,\n", psu_index,
        sensor_type, key_to_name(DFD_CFG_ITEM_HWMON_PSU));
    pfunc = dfd_deal_hwmon_buf;
    rv = dfd_info_get_sensor(key, buf, count, pfunc);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get psu sensor info error, key_name: %s, rv: %d\n",
            key_to_name(DFD_CFG_ITEM_HWMON_PSU), rv);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "get psu sensor info success, value: %s\n", buf);
    }
    return rv;
}

/**
 * dfd_get_main_board_monitor_flag - Get Monitor flag info
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @sensor_type: Type of the obtained pmbus information
 * @in_type: Voltage type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
int dfd_get_main_board_monitor_flag(uint8_t main_dev_id, uint8_t dev_index, uint8_t sensor_type,
        uint8_t sensor_index, char *buf, size_t count)
{
    uint64_t key;
    uint16_t key_index1;
    uint8_t key_index2;
    int rv, sensor_type_key, decode_key;
    int data;
    info_ctrl_t *info_ctrl;
    int *p_decode_value;

    key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, sensor_index);
    key_index2 = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, 0); /* 4bytes. currently low bytes is 0. */
    if (sensor_type == WB_MINOR_DEV_TEMP) {
        sensor_type_key = DFD_CFG_ITEM_HWMON_TEMP_MONITOR_FLAG;
        decode_key = DFD_CFG_ITEM_HWMON_TEMP_MONITOR_DC;
    } else if (sensor_type == WB_MINOR_DEV_IN) {
        sensor_type_key = DFD_CFG_ITEM_HWMON_IN_MONITOR_FLAG;
        decode_key = DFD_CFG_ITEM_HWMON_IN_MONITOR_FLAG_DC;
    } else if (sensor_type == WB_MINOR_DEV_CURR) {
        sensor_type_key = DFD_CFG_ITEM_HWMON_CURR_MONITOR_FLAG;
        decode_key = DFD_CFG_ITEM_HWMON_CURR_MONITOR_FLAG_DC;
    } else {
        DFD_SENSOR_DEBUG(DBG_ERROR, "Unknow sensor type: %u\n",sensor_type);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    key = DFD_CFG_KEY(sensor_type_key, key_index1, key_index2);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_VERBOSE, "get info ctrl failed, key=0x%08llx\n", key);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", WB_SENSOR_MONITOR_YES);
    }

    rv = dfd_info_get_int(key, &data, NULL);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get monitor flag error, key_name: %s, rv: %d\n",
            key_to_name(sensor_type_key), rv);
        return rv;
    }

    key = DFD_CFG_KEY(decode_key, key_index1, data);
    p_decode_value = dfd_ko_cfg_get_item(key);
    if (p_decode_value == NULL) {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "status needn't decode. value:0x%x\n", data);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "ori_value:0x%x, decoded value:0x%x\n", data, *p_decode_value);
        data = *p_decode_value;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "main_dev_id: %u, dev_index: 0x%x, sensor_index: 0x%x, \
        key_name: %s, data = %d\n", main_dev_id, dev_index, sensor_index, key_to_name(sensor_type_key), data);

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", data);
}

/**
 * dfd_get_temp_status - Get Monitor flag info
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @sensor_index: index of temp
 * @buf: temp status is stored in buf
 * @count: sizeof buf
 * return: Success: Returns the length of buf
 */
int dfd_get_temp_status(uint8_t main_dev_id, uint8_t dev_index,
        unsigned int sensor_index, char *buf, size_t count)
{
    ssize_t ret;
    char buf_tmp[INFO_INT_MAX_LEN];
    int current_temp;
    int max_temp;
    int high_temp;
    int low_temp;
    int min_temp;
    int status;
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    char detail[64];
    size_t detail_len;
    dfd_status_detail_t detail_cfg;
#endif

    current_temp = 0;
    max_temp = 0;
    high_temp = 0;
    low_temp = 0;
    min_temp = 0;

    /* 1. get current temp */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = dfd_get_temp_info(main_dev_id, dev_index, sensor_index, WB_SENSOR_INPUT,
              buf_tmp, sizeof(buf_tmp));
    if (ret < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "failed to get current temp, main_dev_id=%u, dev_index: %u, sensor_index: %u, senser_type: %u\n",
            main_dev_id, dev_index, sensor_index, WB_SENSOR_INPUT);
        status = TEMP_STATUS_FETCH_ERROR;
        goto create_result;
    } else {
        current_temp = simple_strtol(buf_tmp, NULL, 10);
    }

    /* 2. get max temp */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = dfd_get_temp_info(main_dev_id, dev_index, sensor_index, WB_SENSOR_MAX,
              buf_tmp, sizeof(buf_tmp));
    if (ret < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "failed to get max temp, main_dev_id=%u, dev_index: %u, sensor_index: %u, senser_type: %u\n",
            main_dev_id, dev_index, sensor_index, WB_SENSOR_MAX);
        return ret;
    } else {
        max_temp = simple_strtol(buf_tmp, NULL, 10);
    }

    /* 3. get high temp */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = dfd_get_temp_info(main_dev_id, dev_index, sensor_index, WB_SENSOR_HIGH,
              buf_tmp, sizeof(buf_tmp));
    if (ret < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "failed to get high temp, main_dev_id=%u, dev_index: %u, sensor_index: %u, senser_type: %u\n",
            main_dev_id, dev_index, sensor_index, WB_SENSOR_HIGH);
        return ret;
    } else {
        high_temp = simple_strtol(buf_tmp, NULL, 10);
    }

    /* 4. get low temp */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = dfd_get_temp_info(main_dev_id, dev_index, sensor_index, WB_SENSOR_LOW,
              buf_tmp, sizeof(buf_tmp));
    if (ret < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "failed to get low temp, main_dev_id=%u, dev_index: %u, sensor_index: %u, senser_type: %u\n",
            main_dev_id, dev_index, sensor_index, WB_SENSOR_LOW);
        return ret;
    } else {
        low_temp = simple_strtol(buf_tmp, NULL, 10);
    }

    /* 5. get min temp */
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = dfd_get_temp_info(main_dev_id, dev_index, sensor_index, WB_SENSOR_MIN,
              buf_tmp, sizeof(buf_tmp));
    if (ret < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "failed to get min temp, main_dev_id=%u, dev_index: %u, sensor_index: %u, senser_type: %u\n",
            main_dev_id, dev_index, sensor_index, WB_SENSOR_MIN);
        return ret;
    } else {
        min_temp = simple_strtol(buf_tmp, NULL, 10);
    }

    if (current_temp >= max_temp || current_temp <= min_temp) {
        status = TEMP_STATUS_FATAL_WARNING;
    } else if (current_temp >= high_temp || current_temp <= low_temp) {
        status = TEMP_STATUS_MAJOR_WARNING;
    } else {
        status = TEMP_STATUS_OK;
    }

create_result:
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    mem_clear(detail, sizeof(detail));
    DFD_STATUS_DETAIL_CFG_SET(detail_cfg, status, TEMP_STATUS_OK, dfd_temp_status_map, false);
    detail_len = dfd_status_get_detail(&detail_cfg, detail, sizeof(detail));
    if (detail_len > 0) {
        return (ssize_t)snprintf(buf, count, "0x%x %s\n", status, detail);
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", status);
    }
#else
    return (ssize_t)snprintf(buf, count, "0x%x\n", status);
#endif
}

/**
 * dfd_get_vol_status - Get Monitor flag info
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @sensor_index: index of temp
 * @buf: temp status is stored in buf
 * @count: sizeof buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
int dfd_get_vol_status(uint8_t main_dev_id, uint8_t dev_index,
        unsigned int sensor_index, char *buf, size_t count)
{
    ssize_t ret;

    if (buf == NULL || count == 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "invalid output buffer, buf: %p, count: %zu\n", buf, count);
        return -EINVAL;
    }

    /* get vol status direct */
    ret = dfd_get_vol_status_direct(main_dev_id, dev_index, sensor_index, buf, count);
    if (ret != -DFD_RV_DEV_NOTSUPPORT) {
        DFD_SENSOR_DEBUG(DBG_VERBOSE,
            "main_dev_id=%u, dev_index: %u, sensor_index: %u\n",
            main_dev_id, dev_index, sensor_index);
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "ret = %zd. buf = %s\n", ret, buf);
        return ret;
    }

    /*
     * If direct voltage status retrieval is not supported,
     * determine the status based on voltage value validity.
     */
    return dfd_query_vol_status_via_validity_check(main_dev_id, dev_index, sensor_index, buf,
            count);
}