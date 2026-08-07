/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_led_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * led related properties read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "switch_driver.h"
#include "wb_led_driver.h"
#include <wb_platform_common.h>

int g_dfd_sysled_dbg_level = 0;
module_param(g_dfd_sysled_dbg_level, int, S_IRUGO | S_IWUSR);

static int dfd_get_led_reg_num(uint16_t led_id)
{
    uint64_t key;
    int reg_num;
    int *p_reg_num;

    /* get led register number, default is 1 if not configured */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_LED_REG_NUM, led_id, 0);
    p_reg_num = dfd_ko_cfg_get_item(key);
    if (p_reg_num == NULL) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "led register number config not found, use default value: 1\n");
        reg_num = 1;
    } else {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "led register number: %d\n", *p_reg_num);
        reg_num = *p_reg_num;
    }

    if (reg_num <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "Invalid led register number: %d\n", reg_num);
        return -DFD_RV_INVALID_VALUE;
    }
    return reg_num;
}

/**
 * dfd_get_led_status_value - Get LED light status value
 * @led_id See the wb_led_t definition
 * @value 0: Off, 1: green, 2: yellow, 3: red, 4: blue, 5: green, 6: yellow, 7: red
 * @returns: 0 success, negative value: failed
 */
static int dfd_get_led_status_value(uint16_t led_id, uint8_t led_index, int *value)
{
    uint64_t key;
    int ori_value, ret, ori_led_id;
    int *p_decode_value;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_LED_STATUS, led_id, led_index);
    ret = dfd_info_get_int(key, &ori_value, NULL);
    if (ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "get led status error, key: %s, led_id: 0x%x, led_index: %u, ret: %d\n",
            key_to_name(DFD_CFG_ITEM_LED_STATUS), led_id, led_index, ret);
        return ret;
    }

    DBG_SYSLED_DEBUG(DBG_VERBOSE, "get led origin status success, led_id: 0x%x, led_index: %u, ori_value: 0x%x\n",
            led_id, led_index, ori_value);

    /* led_id = (reg_index << 8 |ori_led_id & 0xff), The led status decode is only related to the ori_led_id */
    ori_led_id = led_id & 0xff;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_LED_STATUS_DECODE, ori_led_id, ori_value);
    p_decode_value = dfd_ko_cfg_get_item(key);
    if (p_decode_value != NULL) {
        DBG_SYSLED_DEBUG(DBG_VERBOSE, "decode led status success, led id: 0x%x, ori_value: 0x%x, decode value: 0x%x\n",
                ori_led_id, ori_value, *p_decode_value);
        *value = *p_decode_value;
        return DFD_RV_OK;
    }
    DBG_SYSLED_DEBUG(DBG_ERROR, "decode led status error, key: %s, led_id: 0x%x, ori_value: 0x%x\n",
        key_to_name(DFD_CFG_ITEM_LED_STATUS_DECODE), ori_led_id, ori_value);
    return -DFD_RV_INVALID_VALUE;
}

/**
 * dfd_get_led_status - Get LED light status
 * @led_id: led lamp type
 * @led_index: led light offset
 * @buf: LED light status receives buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_led_status(uint16_t led_id, uint8_t led_index, char *buf, size_t count)
{
    int ret, first_led_value, current_led_value;
    int reg_index, reg_num;
    uint16_t tmp_led_id;

    if (buf == NULL) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "param error, buf is NULL. led_id: %u, led_index: %u\n",
            led_id, led_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "buf size error, count: %zu, led_id: %u, led_index: %u\n",
            count, led_id, led_index);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    /* get led register number */
    reg_num = dfd_get_led_reg_num(led_id);
    if (reg_num <= 0) {
        return reg_num;
    }

    /* read first register value */
    ret = dfd_get_led_status_value(led_id, led_index, &first_led_value);
    if (ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "get led status error, ret: %d, led_id: %u, led_index: %u\n",
            ret, led_id, led_index);
        return ret;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "led_id: %u, led_index: %u, first_led_value: %d\n", led_id, led_index, first_led_value);

    /* Read the remaining led registers value */
    for (reg_index = 1; reg_index < reg_num; reg_index++) {
        tmp_led_id = (reg_index << 8) | (led_id & 0xff);
        ret = dfd_get_led_status_value(tmp_led_id, led_index, &current_led_value);
        if (ret < 0) {
            DBG_SYSLED_DEBUG(DBG_ERROR, "get led status error, ret: %d, led_id: 0x%x, led_index: %u\n",
                ret, tmp_led_id, led_index);
            return ret;
        }
        if (current_led_value != first_led_value) {
            DBG_SYSLED_DEBUG(DBG_ERROR, "led status inconsistent, led_id: 0x%x, led_index: %u, first_led_value: %d, current_led_value: %d\n",
                tmp_led_id, led_index, first_led_value, current_led_value);
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_STATUS_UNKNOWN);
        }
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "led status consistent, led_id: 0x%x, led_index: %u, current_led_value: %d\n",
            tmp_led_id, led_index, current_led_value);
    }

    return (ssize_t)snprintf(buf, count, "%d\n", first_led_value);
}

/**
 * dfd_set_led_status - Set LED light status
 * @led_id: led lamp type
 * @led_index: led light offset
 * @value: LED light status value
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_set_led_status(uint16_t led_id, uint8_t led_index, int value)
{
    uint64_t key;
    int ret, led_value;
    int reg_index, reg_num;
    uint16_t tmp_led_id;

    if (value < 0 || value > 0xff) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "can not set led status value = %d.\n", value);
        return -DFD_RV_INVALID_VALUE;
    }

    /* get led register number */
    reg_num = dfd_get_led_reg_num(led_id);
    if (reg_num <= 0) {
        return reg_num;
    }

    ret = dfd_ko_cfg_get_led_status_decode2_by_regval(value, led_id, &led_value);
    if (ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "get led status register error, ret: %d, led_id: 0x%x, led_index: %d, value: %d\n",
            ret, led_id, led_index, value);
        return ret;
    }
    DBG_SYSLED_DEBUG(DBG_VERBOSE, "get led_id: 0x%x, led_index: %d, origin value: %d, decode value: 0x%x\n",
        led_id, led_index, value, led_value);

    for (reg_index = 0; reg_index < reg_num; reg_index++) {
        tmp_led_id = (reg_index << 8) | (led_id & 0xff);
        DBG_SYSLED_DEBUG(DBG_VERBOSE, "set led id: 0x%x, led_index: %u, value: 0x%x\n",
            tmp_led_id, led_index, led_value);

        /* case: led set or get not from same reg */
        key = DFD_CFG_KEY(DFD_CFG_ITEM_CTRL_LED_STATUS, tmp_led_id, led_index);
        ret = dfd_info_set_int(key, led_value);
        if (ret < 0 && ret != -DFD_RV_DEV_NOTSUPPORT) {
            DBG_SYSLED_DEBUG(DBG_ERROR, "set led status ctrl error, key_name: %s, led id: 0x%x, led_index: %u, value: 0x%x, ret: %d\n",
                key_to_name(DFD_CFG_ITEM_CTRL_LED_STATUS), tmp_led_id, led_index, led_value, ret);
            return ret;
        }

        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            key = DFD_CFG_KEY(DFD_CFG_ITEM_LED_STATUS, tmp_led_id, led_index);
            ret = dfd_info_set_int(key, led_value);
            if (ret < 0) {
                DBG_SYSLED_DEBUG(DBG_ERROR, "set led status error, key_name: %s, led id: 0x%x, led_index: %u, value: 0x%x, ret: %d\n",
                    key_to_name(DFD_CFG_ITEM_LED_STATUS), tmp_led_id, led_index, led_value, ret);
                return ret;
            }
        }

        DBG_SYSLED_DEBUG(DBG_VERBOSE, "set led id: 0x%x, led_index: %u, value: 0x%x success\n",
            tmp_led_id, led_index, led_value);
    }

    return DFD_RV_OK;
}
