/*
 * Copyright(C) 2001-2025 whitebox Network. All rights reserved.
 */
/*
 * wb_avs_driver.c
 * Original Author: [Your Name] [Date]
 *
 * AVS (GPU Computing Unit) related properties read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0                                    2025-12-24        Initial version
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_avs_driver.h"

int g_dfd_avs_dbg_level = 0;
module_param(g_dfd_avs_dbg_level, int, S_IRUGO | S_IWUSR);

static ssize_t dfd_avs_get_power_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_avs_get_bus_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_avs_get_alias(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_avs_get_circuit_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);

dfd_sysfs_func_map_t avs_func_table[DFD_AVS_MAX_E] = {
    [DFD_AVS_POWER_STATUS_E] = {dfd_avs_get_power_status, NULL},
    [DFD_AVS_BUS_STATUS_E] = {dfd_avs_get_bus_status, NULL},
    [DFD_AVS_ALIAS_E] = {dfd_avs_get_alias, NULL},
    [DFD_AVS_CIRCUIT_STATUS_E] = {dfd_avs_get_circuit_status, NULL},
};

dfd_debug_data_key_map_t avs_dbg_key_table[DFD_AVS_MAX_E] = {
    [DFD_AVS_POWER_STATUS_E] = {DFD_CFG_ITEM_AVS_DEVICE, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_AVS_BUS_STATUS_E] = {DFD_CFG_ITEM_AVS_DEVICE, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_AVS_ALIAS_E] = {DFD_CFG_ITEM_AVS_DEVICE, CFG_2INDEXES_2, CFG_STR_DATA},
    [DFD_AVS_CIRCUIT_STATUS_E] = {DFD_CFG_ITEM_AVS_DEVICE, CFG_2INDEXES_2, CFG_STR_DATA},
};

static const int avs_fault_bits[AVS_POWER_FAULT_MAX] = {
   [AVS_PHASE_LOSS_FAULT] = BIT(0),
   [AVS_VIN_OV_FAULT] = BIT(1),
   [AVS_VOUT_OV_FAULT] = BIT(2),
   [AVS_VIN_UV_FAULT] = BIT(3),
   [AVS_VOUT_UV_FAULT] = BIT(4),
   [AVS_IIN_OC_WARNING] = BIT(5),
   [AVS_IOUT_OC_FAULT] = BIT(6),
   [AVS_OT_FAULT] = BIT(7),
   [AVS_PHASE_CURRENT_IMBALANCE_FAULT] = BIT(8),
};

static const dfd_status_desc_map_t dfd_avs_power_status_map[] = {
    {avs_fault_bits[AVS_PHASE_LOSS_FAULT], "Avs_Phase_Loss_Error"},
    {avs_fault_bits[AVS_VIN_OV_FAULT], "Avs_Vin_Ov_Error"},
    {avs_fault_bits[AVS_VOUT_OV_FAULT], "Avs_Vout_Ov_Error"},
    {avs_fault_bits[AVS_VIN_UV_FAULT], "Avs_Vin_Uv_Error"},
    {avs_fault_bits[AVS_VOUT_UV_FAULT], "Avs_Vout_Uv_Error"},
    {avs_fault_bits[AVS_IIN_OC_WARNING], "Avs_Iin_Oc_Error"},
    {avs_fault_bits[AVS_IOUT_OC_FAULT], "Avs_Iout_Oc_Error"},
    {avs_fault_bits[AVS_OT_FAULT], "Avs_Ot_Error"},
    {avs_fault_bits[AVS_PHASE_CURRENT_IMBALANCE_FAULT], "Avs_Phase_Current_Imbalance_Error"},
};

static const dfd_status_desc_map_t dfd_avs_bus_status_map[] = {
    {AVS_BUS_STATUS_ABNORMAL, "Avs_Bus_Abnormal"},
};

static const dfd_status_desc_map_t dfd_avs_circuit_status_map[] = {
    {AVS_CIRCUIT_STATUS_ABNORMAL, "Avs_Circuit_Abnormal"},
};


static ssize_t dfd_avs_get_power_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint64_t key;
    unsigned int sub_key, sub_key_phase;
    int i, j, tmp_value, target_value, ret, status_number;
    int tmp_curr;
    int phase_num, phase_cur_threshold, phase_cur_max, phase_cur_min;
    info_ctrl_t *key_info_ctrl;
    uint8_t tmp_str[INFO_BUF_MAX_LEN] = {0};

    if (buf == NULL) {
        DBG_AVS_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_AVS_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    target_value = 0;
    phase_cur_max = INT_MAX;
    phase_cur_min = INT_MIN;
    mem_clear(buf, count);

    ret = dfd_get_dev_debug_config_data(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id,
            DFD_AVS_POWER_STATUS_NUM_E, buf, count);
    if (ret >= 0) {
        DBG_AVS_DEBUG(DBG_VERBOSE, "get avs info from debug node, value: %s\n", buf);
        return ret;
    } else {
        DBG_AVS_DEBUG(DBG_VERBOSE, 
            "failed to get avs info from debug node, main_dev_id: %u, type: %u, ret=%d\n",
            main_dev_id, DFD_AVS_POWER_STATUS_NUM_E, ret);
    }

    /* 1. get number of power status */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id, DFD_AVS_POWER_STATUS_NUM_E);
    key_info_ctrl = dfd_ko_cfg_get_item(key);
    if (key_info_ctrl == NULL) {
        DBG_AVS_DEBUG(DBG_ERROR, "can't find dfd config, key: 0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    if (key_info_ctrl->int_cons <= 0) {
        DBG_AVS_DEBUG(DBG_VERBOSE, "dfd int_cons config error, key: 0x%08llx, int_cons: %d\n",
            key, key_info_ctrl->int_cons);
        return -DFD_RV_INVALID_VALUE;
    }

    status_number = key_info_ctrl->int_cons;
    DBG_AVS_DEBUG(DBG_VERBOSE, "avs number of power status check: %d\r\n", status_number);

    for (i = 0; i < status_number; i++) {
        sub_key = DFD_AVS_POWER_STATUS_E << 8 | i;

        key = DFD_CFG_KEY(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id, sub_key);

        key_info_ctrl = dfd_ko_cfg_get_item(key);
        if (key_info_ctrl == NULL) {
            DBG_AVS_DEBUG(DBG_ERROR, "can't find dfd config, key: 0x%08llx\n", key);
            return -DFD_RV_DEV_NOTSUPPORT;
        }
        if (key_info_ctrl->int_extra1 < 0 || key_info_ctrl->int_extra1 >= AVS_POWER_FAULT_MAX) {
            DBG_AVS_DEBUG(DBG_ERROR, "dfd extra1 config error, key: 0x%08llx, extra1: %d\n",
                key, key_info_ctrl->int_extra1);
            return -DFD_RV_INVALID_VALUE;
        }

        /* handle phase current imbalance error separately. */
        if (key_info_ctrl->int_extra1 == AVS_PHASE_CURRENT_IMBALANCE_FAULT) {
            /* get threshold */
            phase_cur_threshold = key_info_ctrl->int_cons;

            key = DFD_CFG_KEY(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id, DFD_AVS_PHASE_CURR_NUM_E);
            key_info_ctrl = dfd_ko_cfg_get_item(key);
            if (key_info_ctrl == NULL) {
                DBG_AVS_DEBUG(DBG_ERROR, "can't find dfd config, key: 0x%08llx\n", key);
                return -DFD_RV_DEV_NOTSUPPORT;
            }
            if (key_info_ctrl->int_cons <= 0) {
                DBG_AVS_DEBUG(DBG_VERBOSE, "dfd int_cons config error, key: 0x%08llx, int_cons: %d\n",
                    key, key_info_ctrl->int_cons);
                return -DFD_RV_INVALID_VALUE;
            }

            phase_num = key_info_ctrl->int_cons;
            for (j = 0; j < phase_num; j++) {
                sub_key_phase = DFD_AVS_PHASE_CURR_E << 8 | j;
                key = DFD_CFG_KEY(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id, sub_key_phase);
                ret = dfd_info_get_sensor(key, tmp_str, sizeof(tmp_str), NULL);
                if (ret < 0) {
                    DBG_AVS_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_key_phase 0x%x config error, key_name: %s, ret: %d\n",
                        main_dev_id, sub_key_phase, key_to_name(DFD_CFG_ITEM_AVS_DEVICE), ret);
                    return -DFD_RV_DEV_NOTSUPPORT;
                }

                ret = kstrtoint(tmp_str, 0, &tmp_curr);
                if (ret < 0) {
                    DBG_AVS_DEBUG(DBG_ERROR, "curr value is illegal, tmp_str: %s, ret: %d\r\n", tmp_str, ret);
                    return -DFD_RV_INVALID_VALUE;
                }

                if (j == 0) {
                    phase_cur_max = tmp_curr;
                    phase_cur_min = tmp_curr;
                } else if (tmp_curr > phase_cur_max) {
                    phase_cur_max = tmp_curr;
                } else if (tmp_curr < phase_cur_min) {
                    phase_cur_min = tmp_curr;
                }
            }

            DBG_AVS_DEBUG(DBG_VERBOSE, "phase_cur_max: %d, phase_cur_min: %d, phase_cur_threshold: %d\n",
                phase_cur_max, phase_cur_min, phase_cur_threshold);
            if (phase_cur_max - phase_cur_min >= phase_cur_threshold) {
                target_value |= avs_fault_bits[AVS_PHASE_CURRENT_IMBALANCE_FAULT];
            }

            continue;
        }
        
        ret = dfd_info_get_int(key, &tmp_value, NULL);
        if (ret < 0) {
            DBG_AVS_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id 0x%x config error, key_name: %s, ret: %d\n",
                main_dev_id, sub_key, key_to_name(DFD_CFG_ITEM_AVS_DEVICE), ret);
            return -DFD_RV_DEV_NOTSUPPORT;
        }

        /* get mask */
        if (key_info_ctrl->int_cons > 0) {
            DBG_AVS_DEBUG(DBG_VERBOSE, "key: 0x%08llx, tmp_value: 0x%x, mask: 0x%x, extra1: %d\n",
                key, tmp_value, key_info_ctrl->int_cons, key_info_ctrl->int_extra1);
            tmp_value &= key_info_ctrl->int_cons;
        }

        if (tmp_value > 0) {
            DBG_AVS_DEBUG(DBG_VERBOSE, "key: 0x%08llx, value is: 0x%x\n", key, tmp_value);
            /* get fault type */
            target_value |= avs_fault_bits[key_info_ctrl->int_extra1];
        }
    }

    /* avs clear faults */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id, DFD_AVS_CLEAR_FAULTS_E);
    ret = dfd_info_set_int(key, 1);
    if (ret < 0) {
        DBG_AVS_DEBUG(DBG_ERROR, "main_dev_id: %u, set avs clear faults error, key_name: %s, ret:%d\n",
            main_dev_id, key_to_name(DFD_CFG_ITEM_AVS_DEVICE), ret);
        return ret;
    }

    DFD_STATUS_DETAIL_INFO_SET(dfd_avs_power_status_map, target_value, 0, true, buf, count, NULL,
        0);
    return (ssize_t)strlen(buf);
}

static ssize_t dfd_avs_get_bus_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint64_t key;
    int pmbus_status, ret;

    if (buf == NULL) {
        DBG_AVS_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_AVS_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    ret = dfd_get_dev_debug_config_data(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id,
            sub_dev_id, buf, count);
    if (ret >= 0) {
        DBG_AVS_DEBUG(DBG_VERBOSE, "get avs info from debug node, value: %s\n", buf);
        return ret;
    } else {
        DBG_AVS_DEBUG(DBG_VERBOSE, 
            "failed to get avs info from debug node, main_dev_id: %u, sub_dev_id: %u, ret=%d\n",
            main_dev_id, sub_dev_id, ret);
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id, sub_dev_id);

    DBG_AVS_DEBUG(DBG_VERBOSE, "dfd_avs_get_bus_status has been called, main_dev_id: %u, sub_dev_id: %u, count: %zu.\n",
        main_dev_id, sub_dev_id, count);

    ret = dfd_info_get_int(key, &pmbus_status, NULL);
    if (ret < 0) {
        DBG_AVS_DEBUG(DBG_ERROR, "failed to get pmbus_status, main_dev_id: %u, sub_dev_id %u, key_name: %s, ret: %d\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_AVS_DEVICE), ret);
        return ret;
    }

    DFD_STATUS_DETAIL_INFO_SET(dfd_avs_bus_status_map, pmbus_status, AVS_BUS_STATUS_OK,
        false, buf, count, NULL, 0);
    return (ssize_t)strlen(buf);
}

/* AVS get alias */
static ssize_t dfd_avs_get_alias(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    info_ctrl_t *info_ctrl;
    uint64_t key;

    if (buf == NULL) {
        DBG_AVS_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_AVS_DEBUG(DBG_VERBOSE, "dfd_avs_get_alias has been called, main_dev_id: %u, sub_dev_id: %u, count: %zu.\n",
        main_dev_id, sub_dev_id, count);

    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id, sub_dev_id);
    DBG_AVS_DEBUG(DBG_VERBOSE, "alias key: 0x%08llx\n", key);

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_AVS_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return (size_t)snprintf(buf, count, "%s\n", info_ctrl->str_cons);
}

static ssize_t dfd_avs_get_circuit_status(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    int circuit_status;
    uint8_t tmp_str[INFO_BUF_MAX_LEN] = {0};
    int tmp_curr;
    info_ctrl_t *key_info_ctrl;

    if (buf == NULL) {
        DBG_AVS_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_AVS_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, sub_dev_id: %u\n",
            count, main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    ret = dfd_get_dev_debug_config_data(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id,
            sub_dev_id, buf, count);
    if (ret >= 0) {
        DBG_AVS_DEBUG(DBG_VERBOSE, "get avs info from debug node, value: %s\n", buf);
        return ret;
    } else {
        DBG_AVS_DEBUG(DBG_VERBOSE, 
            "failed to get avs info from debug node, main_dev_id: %u, sub_dev_id: %u, ret=%d\n",
            main_dev_id, sub_dev_id, ret);
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_AVS_DEVICE, main_dev_id, sub_dev_id);
    ret = dfd_info_get_sensor(key, tmp_str, sizeof(tmp_str), NULL);
    if (ret < 0) {
        DBG_AVS_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error, key_name: %s, ret: %d\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_AVS_DEVICE), ret);
        return ret;
    }

    key_info_ctrl = dfd_ko_cfg_get_item(key);
    if (key_info_ctrl == NULL) {
        DBG_AVS_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    
    ret = kstrtoint(tmp_str, 0, &tmp_curr);
    if (ret < 0) {
        DBG_AVS_DEBUG(DBG_ERROR, "curr value is illegal, tmp_str: %s, ret: %d\r\n", tmp_str, ret);
        return -DFD_RV_INVALID_VALUE;
    }

    if (tmp_curr > key_info_ctrl->int_cons) {
        DBG_AVS_DEBUG(DBG_ERROR, "curr is error, main_dev_id: %u, sub_dev_id %u, key_name: %s, curr: %d, int_cons: %d\n",
            main_dev_id, sub_dev_id, key_to_name(DFD_CFG_ITEM_AVS_DEVICE), tmp_curr, key_info_ctrl->int_cons);
        circuit_status = AVS_CIRCUIT_STATUS_ABNORMAL;
    } else {
        DBG_AVS_DEBUG(DBG_VERBOSE, "The current value is normal, curr: %d\r\n", tmp_curr);
        circuit_status = AVS_CIRCUIT_STATUS_OK;
    }
    DFD_STATUS_DETAIL_INFO_SET(dfd_avs_circuit_status_map, circuit_status, AVS_CIRCUIT_STATUS_OK,
        false, buf, count, NULL, 0);
    return (ssize_t)strlen(buf);
}
