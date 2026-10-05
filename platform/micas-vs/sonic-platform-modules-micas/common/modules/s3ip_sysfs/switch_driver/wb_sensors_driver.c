/*
 * An wb_sensors_driver driver for snesors devcie function
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
#include <linux/slab.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"
#include "dfd_frueeprom.h"
#include "dfd_cfg_file.h"

#define DFD_GET_TEMP_SENSOR_KEY1(dev_index, temp_index) \
    (((dev_index & 0xff) << 8) | (temp_index & 0xff))
#define DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, temp_type) \
        (((main_dev_id & 0x0f) << 4) | (temp_type & 0x0f))
#define DFD_FORMAT_STR_MAX_LEN          (32)

int g_dfd_sensor_dbg_level = 0;
module_param(g_dfd_sensor_dbg_level, int, S_IRUGO | S_IWUSR);

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
        snprintf(buf_new, *buf_len_new, "%d\n", div_result);
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
                if (i + decimal + 2 <= *buf_len_new) {
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

static int dfd_get_sensor_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t sensor_type,
               uint8_t sensor_index, uint8_t sensor_attr, char *buf, size_t count)
{
    uint64_t key;
    uint16_t key_index1;
    uint8_t key_index2;
    int rv;
    info_hwmon_buf_f pfunc;

    key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, sensor_index);
    key_index2 = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, sensor_attr);
    if (sensor_type == WB_MINOR_DEV_TEMP) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_TEMP, key_index1, key_index2);
    } else if (sensor_type == WB_MINOR_DEV_IN) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_IN, key_index1, key_index2);
    } else if (sensor_type == WB_MINOR_DEV_CURR) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_CURR, key_index1, key_index2);
    } else {
        DFD_SENSOR_DEBUG(DBG_ERROR, "Unknow sensor type: %u\n",sensor_type);
        return -DFD_RV_INVALID_VALUE;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "main_dev_id: %u, dev_index: 0x%x, sensor_index: 0x%x, \
        sensor_attr: 0x%x, key: 0x%08llx\n", main_dev_id, dev_index, sensor_index, sensor_attr, key);

    pfunc = dfd_deal_hwmon_buf;
    rv = dfd_info_get_sensor(key, buf, count, pfunc);
    return rv;
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
        DFD_SENSOR_DEBUG(DBG_ERROR, "buf size error, count: %lu\n", count);
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

    if (buf == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "param error buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "buf size error, count: %lu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }
    rv = dfd_get_sensor_info(main_dev_id, dev_index, WB_MINOR_DEV_IN, in_index, in_attr, buf,
             count);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get voltage info error, rv: %d\n", rv);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "get voltage info success, value: %s\n", buf);
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
        DFD_SENSOR_DEBUG(DBG_ERROR, "buf size error, count: %lu\n", count);
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
        DFD_SENSOR_DEBUG(DBG_ERROR, "buf size error, count: %lu\n", count);
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
        return -DFD_RV_INVALID_VALUE;
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

