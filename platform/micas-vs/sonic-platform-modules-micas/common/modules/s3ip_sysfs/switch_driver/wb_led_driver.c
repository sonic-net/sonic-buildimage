/*
 * An wb_led_driver driver for led devcie function
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

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"

int g_dfd_sysled_dbg_level = 0;
module_param(g_dfd_sysled_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * dfd_get_led_status_value - Get LED light status value
 * @led_id See the wb_led_t definition
 * @value 0: Off, 1: green, 2: yellow, 3: red, 4: blue, 5: green, 6: yellow, 7: red
 * @returns: 0 success, negative value: failed
 */
static int dfd_get_led_status_value(uint16_t led_id, uint8_t led_index, int *value)
{
    uint64_t key;
    int ori_value, ret;
    int *p_decode_value;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_LED_STATUS, led_id, led_index);
    ret = dfd_info_get_int(key, &ori_value, NULL);
    if (ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "get led status error, key: %s, ret: %d\n", 
            key_to_name(DFD_CFG_ITEM_LED_STATUS), ret);
        return ret;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_LED_STATUS_DECODE, led_id, ori_value);
    p_decode_value = dfd_ko_cfg_get_item(key);
    if (p_decode_value != NULL) {
        DBG_SYSLED_DEBUG(DBG_VERBOSE, "led id: %u index: %u, ori_value: 0x%x, decode value :0x%x\n",
                led_id, led_index, ori_value, *p_decode_value);
        *value = *p_decode_value;
        return DFD_RV_OK;
    }
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
    int ret, led_value;

    if (buf == NULL) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "param error, buf is NULL. led_id: %u, led_index: %u\n",
            led_id, led_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "buf size error, count: %lu, led_id: %u, led_index: %u\n",
            count, led_id, led_index);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);
    ret = dfd_get_led_status_value(led_id, led_index, &led_value);
    if (ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "get led status error, ret: %d, led_id: %u, led_index: %u\n",
            ret, led_id, led_index);
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", led_value);
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

    if (value < 0 || value > 0xff) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "can not set led status value = %d.\n", value);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_SYSLED_DEBUG(DBG_VERBOSE, "set led id: %u index: %u, status[%d].\n",
        led_id, led_index, value);
    ret = dfd_ko_cfg_get_led_status_decode2_by_regval(value, led_id, &led_value);
    if (ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "get led status register error, ret: %d, led_id: %u, value: %d\n",
            ret, led_id, value);
        return ret;
    }

    DBG_SYSLED_DEBUG(DBG_VERBOSE, "get led[%u] index[%u] status[%d] decode value[%d]\n",
        led_id, led_index, value, led_value);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_LED_STATUS, led_id, led_index);
    ret = dfd_info_set_int(key, led_value);
    if (ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "set led status error, key_name: %s, ret: %d\n", 
            key_to_name(DFD_CFG_ITEM_LED_STATUS), ret);
        return ret;
    }

    return DFD_RV_OK;
}
