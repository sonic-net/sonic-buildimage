/*
 * Copyright(C) 2001-2012 Ruijie Network. All rights reserved.
 */

#include <linux/module.h>

#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_info.h"
#include "./include/dfd_cfg_adapter.h"

#define LED_STATUS_MEM                       (9)

int g_dfd_sysled_dbg_level = 0;
module_param(g_dfd_sysled_dbg_level, int, S_IRUGO | S_IWUSR);

typedef enum dfd_led_status_e {
    DFD_LED_DARK         = 0,
    DFD_LED_GREEN        = 1,
    DFD_LED_YELLOW       = 2,
    DFD_LED_RED          = 3,
    DFD_LED_GREEN_FLASH  = 4,
    DFD_LED_YELLOW_FLASH = 5,
    DFD_LED_RED_FLASH    = 6,
    DFD_LED_BLUE         = 7,
    DFD_LED_END          = 8,
} dfd_led_status_t;

static int g_dfd_rg_led_status[LED_STATUS_MEM] = {
    DFD_LED_DARK,
    DFD_LED_RED_FLASH,
    DFD_LED_RED,
    DFD_LED_GREEN_FLASH,
    DFD_LED_GREEN,
    DFD_LED_YELLOW_FLASH,
    DFD_LED_YELLOW,
    DFD_LED_DARK,
    DFD_LED_BLUE,
};

static int dfd_get_led_status_value(uint16_t led_id, uint8_t led_index, int *value)
{
    int key, ori_value, ret, value_tmp;
    int *p_decode_value;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_LED_STATUS, led_id, led_index);
    ret = dfd_info_get_int(key, &ori_value, NULL);
    if (ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "get led status error, key:0x%x,ret:%d\n", key, ret);
        return ret;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_LED_STATUS_DECODE, led_id, ori_value);
    p_decode_value = dfd_ko_cfg_get_item(key);
    if (p_decode_value == NULL) {
        DFD_FAN_DEBUG(DBG_VERBOSE, "led id:%d index:%d, status needn't decode.value:0x%x\n", led_id, led_index, ori_value);
        value_tmp = ori_value;
    } else {
        DFD_FAN_DEBUG(DBG_VERBOSE, "led id:%d index:%d,, ori_value:0x%x,decode value:0x%x\n", led_id, led_index, ori_value, *p_decode_value);
        value_tmp = *p_decode_value;
    }
    if(value_tmp >=0 && value_tmp < LED_STATUS_MEM) {
        *value = g_dfd_rg_led_status[value_tmp];
        return DFD_RV_OK;
    }
    return -DFD_RV_INVALID_VALUE;
}

ssize_t dfd_get_led_status(uint16_t led_id, uint8_t led_index, char *buf)
{
    int ret, led_value;

    if(buf == NULL) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "param error, buf is NULL. led_id:%d, led_index:%d\n",
            led_id, led_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_led_status_value(led_id, led_index, &led_value);
    if(ret < 0) {
        DBG_SYSLED_DEBUG(DBG_ERROR, "get led status error,ret:%d, led_id:%d, led_index:%d\n", ret, led_id, led_index);
        return ret;
    }
    memset(buf, 0 , PAGE_SIZE);
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", led_value);
}
