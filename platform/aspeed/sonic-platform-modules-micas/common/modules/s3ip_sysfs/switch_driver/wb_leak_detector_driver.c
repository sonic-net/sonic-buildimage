/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_leak_detector_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * leak_detector related properties read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_leak_detector_driver.h"

int g_dfd_leak_detector_dbg_level = 0;
module_param(g_dfd_leak_detector_dbg_level, int, S_IRUGO | S_IWUSR);

dfd_sysfs_func_map_t leak_func_table[DFD_LEAK_MAX_E] = {
    [DFD_LEAK_NAME_E] = {dfd_get_leak_name, NULL},
    [DFD_LEAK_STATUS_E] = {dfd_get_leak_status, NULL},
    [DFD_LEAK_SIMULATE_STATUS_E] = {dfd_get_leak_simulate_status, dfd_set_leak_simulate_status},
    [DFD_LEAK_PRESENT_E] = {dfd_get_leak_present, NULL},
};

ssize_t dfd_get_leak_name(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint64_t key;
    char *string_value;

    if (buf == NULL) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_LEAK_NAME, main_dev_id, sub_dev_id);
    string_value = dfd_ko_cfg_get_item(key);
    if (string_value == NULL) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, key_name: %s\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_LEAK_NAME));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_LEAK_DETECTOR_DEBUG(DBG_VERBOSE, "%s\n", string_value);
    snprintf(buf, count, "%s\n", string_value);
    return strlen(buf);
}

static int dfd_get_leak_present_value(unsigned int main_dev_id, unsigned int sub_dev_id, int *value)
{
    uint64_t key;
    int ret;

    *value = 0;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_LEAK_PRESENT, main_dev_id, sub_dev_id);
    ret = dfd_info_get_int(key, value, NULL);
    if (ret < 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, 
            "main_dev_id: %u, sub_dev_id %u config error, key_name: %s\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_LEAK_PRESENT));
        return ret;
    }

    return DFD_RV_OK;
}

ssize_t dfd_get_leak_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint64_t key;
    int value, present;
    int ret;

    if (buf == NULL) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, 
            "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, 
            "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    present = -1;
    ret = dfd_get_leak_present_value(main_dev_id, sub_dev_id, &present);
    if (ret < 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, 
            "main_dev_id: %u, sub_dev_id: %u, ret: %d\n", main_dev_id, sub_dev_id, ret);
        return ret;
    }

    if (present != 1) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, 
            "main_dev_id: %u, sub_dev_id: %u, present = %d\n", 
            main_dev_id, sub_dev_id, present);
        value = LEAK_DETECTOR_STATUS_ABSENT;
        goto finish;
    }

    mem_clear(buf, count);
    value = 0;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_LEAK_DETECTOR_STATUS, main_dev_id, sub_dev_id);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, 
            "main_dev_id: %u, sub_dev_id %u config error, key_name: %s\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_LEAK_DETECTOR_STATUS));
        return ret;
    }
    DBG_LEAK_DETECTOR_DEBUG(DBG_VERBOSE, "%d\n", value);
    value = (value == 0) ? LEAK_DETECTOR_STATUS_NOT_OK : LEAK_DETECTOR_STATUS_OK;
finish:
    DBG_LEAK_DETECTOR_DEBUG(DBG_VERBOSE, "%d\n", value);
    snprintf(buf, count, "%d\n", value);
    return strlen(buf);
}

ssize_t dfd_get_leak_simulate_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint64_t key;
    int value;
    int ret;

    if (buf == NULL) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    value = 0;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_LEAK_SIMULATE_STATUS, main_dev_id, sub_dev_id);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, key_name: %s\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_LEAK_SIMULATE_STATUS));
        return ret;
    }

    DBG_LEAK_DETECTOR_DEBUG(DBG_VERBOSE, "%d\n", value);
    snprintf(buf, count, "%d\n", value);
    return strlen(buf);
}

/**
 * dfd_set_leak_detector_status - Set LEAK_DETECTOR light status
 * @main_dev_id: unuse
 * @sub_dev_id: leak_detector light offset
 * @value: LEAK_DETECTOR light status value
 * return: Success: Returns the length of buf
 *       : Faileak_detector: A negative value is returned
 */
int dfd_set_leak_simulate_status(unsigned int main_dev_id, unsigned int sub_dev_id, void *val, unsigned int len)
{
    uint64_t key;
    int ret;
    unsigned int *value;

    value = (unsigned int *)val;
    if (*value < 0 || *value > 0xff) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "main_dev_id: %u, can't set sub_dev_id %u  value = 0x%02x, len = %d\n",
            main_dev_id, sub_dev_id, *value, len);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_LEAK_SIMULATE_STATUS, main_dev_id, sub_dev_id);
    ret = dfd_info_set_int(key, *value);
    if (ret < 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "main_dev_id: %u, set sub_dev_id %u error, key_name: %s, ret:%d\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_LEAK_SIMULATE_STATUS), ret);
        return ret;
    }
    return DFD_RV_OK;
}

ssize_t dfd_get_leak_present(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int value;
    int ret;

    if (buf == NULL) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    value = 0;
    ret = dfd_get_leak_present_value(main_dev_id, sub_dev_id, &value);
    if (ret < 0) {
        DBG_LEAK_DETECTOR_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u, ret = %d\n",
            main_dev_id, sub_dev_id, ret);
        return ret;
    }

    DBG_LEAK_DETECTOR_DEBUG(DBG_VERBOSE, "%d\n", value);
    snprintf(buf, count, "%d\n", value);
    return strlen(buf);
}

