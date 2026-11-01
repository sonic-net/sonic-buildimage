#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <asm/io.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_system_driver.h"
#include "switch_driver.h"
#include "dfd_frueeprom.h"
#include "dfd_tlveeprom.h"
#include "system_sysfs.h"
#include "wb_eeprom_driver.h"
#include <wb_platform_common.h>

#define NODE_MAX_LEN               (64)
#define E2_EXTRA_NODE_MAX_LEN      (128)
#define DFD_E2PROM_MAX_LEN         (256)
#define DFD_MAX_CMD_COUNT          (256)
#define DEFAULT_DECODE_STRING       "Normal"
#define DEFAULT_DECODE_UNKNOWN_STRING       "Unknown"
#define SYS_EEPROM_STRING           "syseeprom"

#define MEM_ISOLATION_TYPE_D17               (0)
#define MEM_ISOLATION_D17_TRIGGER_IO_ADDR    (0xb2)
#define MEM_ISOLATION_D17_DATA_IO_ADDR       (0xb3)
#define MEM_ISOLATION_D17_PARAM_VALUE        (0x80)
#define MEM_ISOLATION_D17_TRIGGER_VALUE      (0x3c)
#define MEM_ISOLATION_STATUS_BIT             (1 << 1)

int g_dfd_custom_dbg_level = 0;
module_param(g_dfd_custom_dbg_level, int, S_IRUGO | S_IWUSR);

static int is_equal_to_extra1(uint64_t dfd_cfg_item, unsigned int type, int index)
{
    uint64_t key;
    int tmp_value, ret;
    info_ctrl_t *info_ctrl;

    key = DFD_CFG_KEY(dfd_cfg_item, type, index);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type=0x%x, index=%d, start get value\n",
            key_to_name(dfd_cfg_item), type, index);
        ret = dfd_info_get_int(key, &tmp_value, NULL);
        if (ret < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "key_name=%s, type=0x%x, index=%d, get value error, ret: %d\n",
                key_to_name(dfd_cfg_item), type, index, ret);
            return ret;
        }
        if (tmp_value != info_ctrl->int_extra1) {
            DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type=0x%x, index=%d, value: %d not equal extra1: %d\n",
                key_to_name(dfd_cfg_item), type, index, tmp_value, info_ctrl->int_extra1);
            return DFD_CONFIG_NOT_EQUAL_TO_EXTRA1;
        }
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type=0x%x, index=%d, value: %d is equal extra1: %d\n",
            key_to_name(dfd_cfg_item), type, index, tmp_value, info_ctrl->int_extra1);
        return DFD_CONFIG_EQUAL_TO_EXTRA1;
    }
    return DFD_CONFIG_IGNORE_TO_EXTRA1;
}

static int dfd_system_is_pre_check_ok(unsigned int type, int index)
{
    int ret;
    ret = is_equal_to_extra1(DFD_CFG_ITEM_PRE_CHECK_BMC_SYSTEM, type, index);
    if (ret < 0) {
        return ret;
    }

    if (ret == DFD_CONFIG_NOT_EQUAL_TO_EXTRA1) {
        /* do not do next step */
        return 0;
    }

    return 1;
}

static int dfd_system_need_reverse(bool is_new_get_op, unsigned int type, int index)
{
    int ret;
    uint64_t dfd_cfg_item;

    /*
     * In the original method, get system value and set system value use the same reverse configuration(reverse_bmc_system).
     * In the new method, get system value and set system value reverse configurations are separated.
     * set system value: using reverse_bmc_system(DFD_CFG_ITEM_REVERSE_BMC_SYSTEM)
     * get system value: using get_reverse_bmc_system(DFD_CFG_ITEM_GET_REVERSE_BMC_SYSTEM)
     */
    if (is_new_get_op) {
        dfd_cfg_item = DFD_CFG_ITEM_GET_REVERSE_BMC_SYSTEM;
    } else {
        dfd_cfg_item = DFD_CFG_ITEM_REVERSE_BMC_SYSTEM;
    }
    ret = is_equal_to_extra1(dfd_cfg_item, type, index);
    if (ret < 0) {
        return ret;
    }

    if (ret != DFD_CONFIG_EQUAL_TO_EXTRA1) {
        /* do not need reverse */
        return 0;
    }

    return 1;
}

/* Get current function step number */
static int dfd_get_cmd_count(bool is_new_get_op, unsigned int type)
{
    uint64_t key, dfd_cfg_item;
    int cmd_num;
    int *p_cmd_num;
    /*
     * Originally, only the set operation supported multi-step operations, using bmc_system_cmd_num.
     * The new method get operation also supports multi-step operations.
     * using get_bmc_system_cmd_num(DFD_CFG_ITEM_GET_BMC_SYSTEM_CMD_NUM)
     * If not configured, the default value 1
     */
    if (is_new_get_op) {
        dfd_cfg_item = DFD_CFG_ITEM_GET_BMC_SYSTEM_CMD_NUM;
    } else {
        dfd_cfg_item = DFD_CFG_ITEM_BMC_SYSTEM_CMD_NUM;
    }

    key = DFD_CFG_KEY(dfd_cfg_item, type, 0);
    p_cmd_num = dfd_ko_cfg_get_item(key);
    if (p_cmd_num == NULL) {
        if (is_new_get_op) {
            DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get operation, key_name: %s, type: 0x%x not config, use default value 1\n",
                key_to_name(DFD_CFG_ITEM_GET_BMC_SYSTEM_CMD_NUM), type);
            cmd_num = 1;
            return cmd_num;
        }
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "set operation, get cmd number failed, key_name: %s, type: 0x%x\n",
            key_to_name(dfd_cfg_item), type);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    cmd_num = *p_cmd_num;
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get cmd number ok, key_name: %s, type: 0x%x, number: %d\n",
        key_to_name(dfd_cfg_item), type, cmd_num);
    if ((cmd_num <= 0) || (cmd_num > DFD_MAX_CMD_COUNT)) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name: %s, type: 0x%x, invalid cmd number:%d\n",
            key_to_name(dfd_cfg_item), type, cmd_num);
        return -DFD_RV_INVALID_VALUE;
    }
    return cmd_num;
}

static void dfd_cmd_delay(unsigned int usdelay)
{
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "usdelay:%d\n", usdelay);
    usleep_range(usdelay, usdelay + 1);
    return;
}

static char *dfd_get_sys_slot_sysfs_name(void)
{
    uint64_t key;
    char *sysfs_name;

    /* Get the configuration item for the sysfs name */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_MY_SLOT_ID_SYSFS_NAME, 0, 0);
    sysfs_name = dfd_ko_cfg_get_item(key);

    if (sysfs_name == NULL) {
        /* If configuration is not available, use the default value "eeprom" */
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, sysfs_name is NULL, using default value 'eeprom'.\n",
            key_to_name(DFD_CFG_ITEM_MY_SLOT_ID_SYSFS_NAME));
        sysfs_name = "eeprom";
    } else {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "sysfs_name: %s.\n", sysfs_name);
    }

    return sysfs_name;
}

/**
 * dfd_get_bmc_present_status - Obtain the bmc present status
 * return: 0:absent
 *         1:present
 *       : Negative value - Read failed
 */
static int dfd_get_bmc_present_status(void)
{
    uint64_t present_key;
    uint32_t present_status;
    int ret;

    /* Get presence status */
    present_key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_PRESENT_STATUS, WB_MAIN_DEV_BMC, 0);
    ret = dfd_info_get_int(present_key, &present_status, NULL);
    if (ret  < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "dfd_get_bmc_present_status error. ret: %d\n", ret);
        return ret;
    }

    return present_status;
}

static int dfd_convert_bmc_sw_status_to_bmc_status(int bmc_sw_status)
{
    switch (bmc_sw_status) {
    case BMC_SW_OK:
        return BMC_OK;
    case BMC_SW_UNKNOWN:
        return BMC_UNKNOWN;
    case BMC_SW_NOTICE:
        return BMC_NOTICE;
    case BMC_SW_WARN:
        return BMC_WARN;
    case BMC_SW_ERROR:
        return BMC_ERROR;
    case BMC_SW_OTHER_STATUS:
    default:
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get bmc sw status invalid: (%d)\n", bmc_sw_status);
        return BMC_UNKNOWN;
    }
}

ssize_t dfd_get_bmc_status(int bmc_sw_status, char *buf, size_t count)
{
    int present;

    if (buf == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "buf is null\n");
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "buf size error, count: %zu.\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    present = dfd_get_bmc_present_status();
    if (present < 0) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get bmc present status fail:%d\n", present);
        return present;
    }

    if (!present) {
       DFD_SYSTEM_DEBUG(DBG_VERBOSE, "bmc is absent:%d\n", BMC_ABSENT);
       return (ssize_t)snprintf(buf, count, "%d\n", BMC_ABSENT);
    }

    bmc_sw_status = dfd_convert_bmc_sw_status_to_bmc_status(bmc_sw_status);

    return (ssize_t)snprintf(buf, count, "%d\n", bmc_sw_status);
}

/**
 * dfd_get_my_slot_id - Obtain the sys information
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_my_slot_id(char *buf, size_t count)
{
    uint64_t key;
    int rv;
    char slot_buf[E2_EXTRA_NODE_MAX_LEN];
    dfd_i2c_dev_t *i2c_dev;
    const char *sysfs_name;

    if (buf == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "buf size error, count: %zu.\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    mem_clear(slot_buf, E2_EXTRA_NODE_MAX_LEN);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_MY_SLOT_ID);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "sys slot i2c dev config error, key_name=%s\n",
            key_to_name(DFD_CFG_ITEM_OTHER_I2C_DEV));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    sysfs_name = dfd_get_sys_slot_sysfs_name();
    rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, DFD_DEV_INFO_TYPE_EXTRA1, slot_buf, E2_EXTRA_NODE_MAX_LEN, sysfs_name);
    if (rv < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "sys slot eeprom read failed, rv = %d", rv);
        return rv;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "%s\n", slot_buf);

    return (ssize_t)snprintf(buf, count, "%s\n", slot_buf);
}

static bool dfd_check_need_system_decode(unsigned int type, int step_index)
{
    int ret, ori_value;
    unsigned int type_detail;

    type_detail = type | step_index;
    ret = dfd_ko_cfg_is_system_status_decode_exist(type_detail, &ori_value);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "system_status_decode_0x%04x not found, don't need to decode value\n",
            type_detail);
        return false;
    }
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "system_status_decode_0x%04x_0x%02x is found, need to decode value\n",
        type_detail, ori_value);
    return true;
}

static int dfd_get_system_decode_value(unsigned int type, int step_index, int ori_val, int *decode_val)
{
    uint64_t key;
    int *p_decode_value;
    int *p_default_decode_value;
    unsigned int type_detail;

    /* decode value */
    type_detail = type | step_index;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SYSTEM_STATUS_DECODE, type_detail, ori_val);
    p_decode_value = dfd_ko_cfg_get_item(key);
    if (p_decode_value != NULL) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "system_status_decode type: 0x%x, ori_value: 0x%x, decoded value: 0x%x\n",
            type_detail, ori_val, *p_decode_value);
        *decode_val = *p_decode_value;
        return DFD_RV_OK;
    }
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "system_status_decode type: 0x%x, ori_value: 0x%x, decode config not found\n",
        type_detail, ori_val);
    /* decode failed, determine whether to use default_system_status_decode_value */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEFAULT_SYSTEM_STATUS_DECODE_VALUE, type_detail, 0);
    p_default_decode_value = dfd_ko_cfg_get_item(key);
    if (p_default_decode_value == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "system_status_decode type: 0x%x, ori_value: 0x%x, decode failed\n",
            type_detail, ori_val);
        return -DFD_RV_INVALID_VALUE;
    }
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "system_status_decode type: 0x%x, return default_system_status_decode_value: 0x%x\n",
        type_detail, *p_default_decode_value);
    *decode_val = *p_default_decode_value;
    return DFD_RV_OK;
}

static int dfd_get_system_value_step(bool is_new_get_op, unsigned int type, int step_index, int *value)
{
    uint64_t key, val_cfg_item;
    int ret, tmp_value, need_reverse;
    info_ctrl_t *info_ctrl;

    if (is_new_get_op) {
        val_cfg_item = DFD_CFG_ITEM_GET_BMC_SYSTEM;
    } else {
        val_cfg_item = DFD_CFG_ITEM_BMC_SYSTEM;
    }

    /* get current step cfg */
    key = DFD_CFG_KEY(val_cfg_item, type, step_index);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get info ctrl fail, key_name: %s, type: 0x%x, step_index: %d\n",
            key_to_name(val_cfg_item), type, step_index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    tmp_value = 0;
    ret = dfd_info_get_int(key, &tmp_value, NULL);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get system value error, key_name: %s, type: 0x%x, step_index: %d, ret: %d\n",
            key_to_name(val_cfg_item), type, step_index, ret);
        return ret;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get system value success: key_name: %s, type: 0x%x, step_index: %d, value: 0x%x\n",
        key_to_name(val_cfg_item), type, step_index, tmp_value);

    need_reverse = dfd_system_need_reverse(is_new_get_op, type, step_index);
    if (need_reverse < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get need_reverse value error, type: 0x%x, step_index: %d, ret: %d\n",
            type, step_index, need_reverse);
        return need_reverse;
    }
    if (need_reverse) {
        if (IS_INFO_FRMT_BIT(info_ctrl->frmt)) {
            *value = ~(tmp_value) & (GENMASK(info_ctrl->len - 1, 0));
        } else {
            *value = ~(tmp_value);
        }
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get system value success: origin value: 0x%x, reverse value: 0x%x\n",
            tmp_value, *value);
    } else {
        *value = tmp_value;
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get system value don't need reverse, value: 0x%x\n", *value);
    }
    return 0;
}

static ssize_t dfd_get_system_value_internal(bool is_new_get_op, unsigned int type,
                   int step_index, int *value, bool *decode_err)
{
    int ret, ori_value, decode_value;
    bool need_decode_value;

    /* get value */
    ret = dfd_get_system_value_step(is_new_get_op, type, step_index, &ori_value);
    if (ret < 0) {
        return ret;
    }
    *decode_err = false;
    /* Check whether decode value is needed */
    need_decode_value = dfd_check_need_system_decode(type, step_index);
    if (need_decode_value == false) { /* don't need decode value, return ori_value */
        *value = ori_value;
        return DFD_RV_OK;
    }

    /* decode value */
    ret = dfd_get_system_decode_value(type, step_index, ori_value, &decode_value);
    if (ret < 0) {
        /* decode value error */
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "type: 0x%x, step_index: %d, ori_value: 0x%x, decode value failed.\n",
            type, step_index, ori_value);
        *value = ori_value;
        *decode_err = true;
        return DFD_RV_OK;
    }
    /* decode value success */
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "type: 0x%x, step_index: %d, ori_value: 0x%x, decoded value: 0x%x\n",
        type, step_index, ori_value, decode_value);
    *value = decode_value;
    return DFD_RV_OK;
}

static ssize_t dfd_get_system_value_origin(unsigned int type, char *buf, size_t count)
{
    int ret, value;
    bool decode_err_flag;

    ret = dfd_get_system_value_internal(false, type, 0, &value, &decode_err_flag);
    /* get value failed */
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get system value origin failed, type: 0x%x,  ret: %d\n",
            type, ret);
        return ret;
    }
    /* get value success, but decode value failed, status unknown */
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get system value origin success, type: 0x%x, value: 0x%x, decode_err_flag: %d\n",
        type, value, decode_err_flag);
    if (decode_err_flag == true) {
        return (ssize_t)snprintf(buf, count, "%s: 0x%x\n", SWITCH_STATUS_UNKNOWN, value);
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

static ssize_t dfd_get_system_value_new(unsigned int type, char *buf, size_t count)
{
    uint64_t key;
    int ret, cmd_count, cmd_i;
    int value, first_value;
    bool decode_err_flag, mix_value_flag;
    int *p_mix_default_value;

    /* get system cmd count */
    cmd_count = dfd_get_cmd_count(true, type);
    if (cmd_count <= 0) {
        if (cmd_count == 0) {
            return -DFD_RV_INVALID_VALUE;
        }
        return cmd_count;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get system value, cmd count: %d\n", cmd_count);

    mix_value_flag = false;
    for (cmd_i = 0; cmd_i < cmd_count; cmd_i++) {
        ret = dfd_get_system_value_internal(true, type, cmd_i, &value, &decode_err_flag);
        /* get value failed */
        if (ret < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "get system value new failed, type: 0x%x, step_index: 0x%x, ret: %d\n",
                type, cmd_i, ret);
            return ret;
        }
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get system value new success, type: 0x%x, step_index: 0x%x, value: 0x%x, decode_err_flag: %d\n",
            type, cmd_i, value, decode_err_flag);
        /* get value success, but decode value failed, status unknown */
        if (decode_err_flag == true) {
            return (ssize_t)snprintf(buf, count, "%s: system_status_decode error, type: 0x%x, step_index: 0x%x, value: 0x%x\n",
                SWITCH_STATUS_UNKNOWN, type, cmd_i, value);
        }
        /* first step, record the first value */
        if (cmd_i == 0) {
            first_value = value;
        } else {
            if (value != first_value) {
                mix_value_flag = true;
                DFD_SYSTEM_DEBUG(DBG_ERROR, "type: 0x%x, step_index: 0x%x, value: 0x%x, not equal to first value: 0x%x\n",
                    type, cmd_i, value, first_value);
                break;
            } else {
                DFD_SYSTEM_DEBUG(DBG_VERBOSE, "type: 0x%x, step_index: 0x%x, value: 0x%x, equal to first value: 0x%x\n",
                    type, cmd_i, value, first_value);
            }
        }
    }
    /* All decoded values are the same, return first_decode_value */
    if (mix_value_flag == false) {
        return (ssize_t)snprintf(buf, count, "%d\n", first_value);
    }
    /* mix value, determine whether to use the configured default value */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_BMC_SYSTEM_MIX_DEFAULT_VALUE, type, 0);
    p_mix_default_value = dfd_ko_cfg_get_item(key);
    if (p_mix_default_value == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "type: 0x%x, value is inconsistent, return UNKNOWN\n", type);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_STATUS_UNKNOWN);
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "type: 0x%x, value is inconsistent, return mix_default_value: 0x%x\n",
        type, *p_mix_default_value);
    return (ssize_t)snprintf(buf, count, "%d\n", *p_mix_default_value);
}

ssize_t dfd_system_get_system_value(unsigned int type, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    info_ctrl_t *info_ctrl;

    if (buf == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "param error buf is NULL, type: 0x%x\n", type);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "buf size error, count: %zu, type: 0x%x\n", count, type);
        return -DFD_RV_INVALID_VALUE;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get system value, type: 0x%x\n", type);
    /* Check if the get_bmc_system config exists. If not, use the original method. */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_GET_BMC_SYSTEM, type, 0);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name: %s, type: 0x%x not found, use origin get system method\n",
            key_to_name(DFD_CFG_ITEM_GET_BMC_SYSTEM), type);
        return dfd_get_system_value_origin(type, buf, count);
    }

    ret = dfd_get_system_value_new(type, buf, count);
    return ret;
}

static int dfd_system_check_value_i(unsigned int type_detail, int cmd_i)
{
    uint64_t key;
    int ret, i;
    info_ctrl_t *info_ctrl;
    int tmp_value, retry_times;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM, type_detail, cmd_i);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key=%s, type_detail=0x%x, cmd_i=%d, don't need to check value\n",
            key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i);
        return DFD_RV_OK;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type_detail=0x%x, cmd_i=%d, start to check value,\n",
        key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i);
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "check value, except value: %d, retry_times: %d, sleep_time: %dus\n",
        info_ctrl->int_extra1, info_ctrl->int_extra2, info_ctrl->int_extra3);

    if (info_ctrl->int_extra2 <= 0) {
        retry_times = 1;
    } else {
        retry_times = info_ctrl->int_extra2;
    }

    for (i = 0; i < retry_times; i++) {
        ret = dfd_info_get_int(key, &tmp_value, NULL);
        if (ret < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "key_name=%s, type_detail=0x%x, cmd_i=%d, get check value error, ret: %d\n",
                key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i, ret);
            return ret;
        }
        if (tmp_value == info_ctrl->int_extra1) {
            DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type_detail=0x%x, cmd_i=%d, check value ok, get value: %d, except value: %d\n",
                key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i, tmp_value, info_ctrl->int_extra1);
            return DFD_RV_OK;
        }
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "key_name=%s, type_detail=0x%x, cmd_i=%d, check value failed, get value: %d, except value: %d, retry: %d\n",
            key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i, tmp_value, info_ctrl->int_extra1, i + 1);

        if (info_ctrl->int_extra3 > 0) {
            dfd_cmd_delay(info_ctrl->int_extra3);
        }
    }

    DFD_SYSTEM_DEBUG(DBG_ERROR, "key_name=%s, type_detail=0x%x, cmd_i=%d, check value failed, get value: %d, except value: %d\n",
        key_to_name(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM), type_detail, cmd_i, tmp_value, info_ctrl->int_extra1);
    return -DFD_RV_CHECK_FAIL;
}

ssize_t dfd_system_set_system_value(unsigned int type, int value)
{
    uint64_t key;
    int ret, cmd_i, cmd_count;
    info_ctrl_t *info_ctrl;
    unsigned int type_detail;
    int need_reverse, pre_check_ok;

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "set system value, type=0x%x, value=%d\n", type, value);
    /* get step number */
    type_detail = type | (value & 0xff);
    ret = dfd_get_cmd_count(false, type_detail);
    if (ret <= 0) {
        if (ret == 0) {
            return -DFD_RV_INVALID_VALUE;
        }
        return ret;
    }

    cmd_count = ret;
    /* exec each step */
    for(cmd_i = 0; cmd_i < cmd_count; cmd_i++) {
        /* first do pre check */
        pre_check_ok = dfd_system_is_pre_check_ok(type_detail, cmd_i);
        if (pre_check_ok < 0) {
            return pre_check_ok;
        }
        if (!pre_check_ok) {
            continue;
        }
        /* otherwise pre_check ok or not need pre_check */

        need_reverse = dfd_system_need_reverse(false, type_detail, cmd_i);
        if (need_reverse < 0) {
            return need_reverse;
        }

        /* get current step cfg */
        key = DFD_CFG_KEY(DFD_CFG_ITEM_BMC_SYSTEM, type_detail, cmd_i);
        info_ctrl = dfd_ko_cfg_get_item(key);
        if (info_ctrl == NULL) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "get info ctrl fail, key_name=%s, type_detail=0x%x, cmd_i=%d\n",
                key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type_detail, cmd_i);
            return -DFD_RV_DEV_NOTSUPPORT;
        }

        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "set, key_name=%s, type_detail=0x%x, cmd_i=%d\n",
            key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type_detail, cmd_i);
        /* set int type info */
        ret = dfd_info_set_int(key, need_reverse ? (~info_ctrl->int_cons) : info_ctrl->int_cons);
        if (ret < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "set system value error, key_name=%s, type_detail=0x%x, cmd_i=%d, value=%d, ret:%d\n",
                key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type_detail, cmd_i, value, ret);
            return ret;
        }

        /* delay if it has */
        if(info_ctrl->int_extra1 > 0) {
            dfd_cmd_delay(info_ctrl->int_extra1);
        }

        /* check value */
        ret = dfd_system_check_value_i(type_detail, cmd_i);
        if (ret < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "set system value check value error, ret: %d\n", ret);
            return ret;
        }
    }

    return DFD_RV_OK;
}

ssize_t dfd_system_get_port_power_status(unsigned int type, char *buf, size_t count)
{
    int ret, cmd_i, cmd_count;
    unsigned int type_detail;

    /* get step number */
    type_detail = type;
    ret = dfd_get_cmd_count(false, type_detail);
    if (ret <= 0) {
        if (ret == 0) {
            return -DFD_RV_INVALID_VALUE;
        }
        return ret;
    }

    cmd_count = ret;
    /* exec each step */
    for(cmd_i = 0; cmd_i < cmd_count; cmd_i++) {
        /* check value */
        ret = dfd_system_check_value_i(type_detail, cmd_i);
        if (ret < 0) {
            if(ret == -DFD_RV_CHECK_FAIL) {
                return (ssize_t)snprintf(buf, count, "%d\n", WB_PORT_POWER_ON);
            }
            DFD_SYSTEM_DEBUG(DBG_ERROR, "set system value check value error, ret: %d\n", ret);
            return ret;
        }
    }

    return (ssize_t)snprintf(buf, count, "%d\n", WB_PORT_POWER_OFF);
}

ssize_t dfd_system_get_bmc_view(char *buf, size_t count)
{
    int ret;

    mem_clear(buf, count);
#ifdef CONFIG_X86
    DFD_SYSTEM_DEBUG(DBG_ERROR, "ERROR:x86 arch not support path:%s \n", BMC_VIEW_FILE);
    ret = -DFD_RV_DEV_NOTSUPPORT;
#else
    ret = dfd_ko_read_file(BMC_VIEW_FILE, 0, buf, count);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "bmc view read file failed, path:%s, ret: %d\n", BMC_VIEW_FILE, ret);
    }
#endif
    return ret;
}

ssize_t dfd_system_set_bmc_switch(const char* buf, size_t count)
{
    int ret;
#ifdef CONFIG_X86
    DFD_SYSTEM_DEBUG(DBG_ERROR, "ERROR:x86 arch not support path:%s \n", BMC_SWITCH_FILE);
    ret = -DFD_RV_DEV_NOTSUPPORT;
#else
	ret = dfd_ko_write_file(BMC_SWITCH_FILE, 0, (uint8_t *)buf, count);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "bmc switch write file failed, path:%s, ret: %d\n", BMC_SWITCH_FILE, ret);
    }
#endif
    return ret;
}

ssize_t dfd_system_get_bmc_dualboot_wdt_status(char *buf, size_t count)
{
    int ret;

    mem_clear(buf, count);
#ifdef CONFIG_X86
    DFD_SYSTEM_DEBUG(DBG_ERROR, "ERROR:x86 arch not support path:%s \n", BMC_WDT_FILE);
    ret = -DFD_RV_DEV_NOTSUPPORT;
#else
    ret = dfd_ko_read_file(BMC_WDT_FILE, 0, buf, count);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "bmc wdt read file failed, path:%s, ret: %d\n", BMC_WDT_FILE, ret);
    }
#endif
    return ret;
}

ssize_t dfd_system_set_bmc_dualboot_wdt(const char* buf, size_t count)
{
    int ret;
#ifdef CONFIG_X86
    DFD_SYSTEM_DEBUG(DBG_ERROR, "ERROR:x86 arch not support path:%s \n", BMC_WDT_FILE);
    ret = -DFD_RV_DEV_NOTSUPPORT;
#else
	ret = dfd_ko_write_file(BMC_WDT_FILE, 0, (uint8_t *)buf, count);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "bmc wdt write file failed, path:%s, ret: %d\n", BMC_WDT_FILE, ret);
    }
#endif
    return ret;
}

static int dfd_get_system_eeprom_mode(wb_main_dev_type_t index, int sub_index)
{
    uint64_t key;
    int mode;
    char *name;

    /* string type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_TYPE, index, sub_index);
    name = dfd_ko_cfg_get_item(key);
    if (name == NULL) {
        /* By default, the TLV format is returned */
        DFD_SYSTEM_DEBUG(DBG_WARN, "get eeprom mode fail, key_name=%s\n",
            key_to_name(DFD_CFG_ITEM_EEPROM_TYPE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "eeprom mode_name %s.\n", name);
    if (!strncmp(name, EEPROM_MODE_TLV_STRING, strlen(EEPROM_MODE_TLV_STRING))) {
        mode = EEPROM_MODE_TLV;
    } else if (!strncmp(name, EEPROM_MODE_FRU_STRING, strlen(EEPROM_MODE_FRU_STRING))) {
        mode = EEPROM_MODE_FRU;
    } else if (!strncmp(name, EEPROM_MODE_SYSFRU_T_STRING, strlen(EEPROM_MODE_SYSFRU_T_STRING))) {
        mode = EEPROM_MODE_FRU;
    } else if (!strncmp(name, EEPROM_MODE_SYSFRU_A_STRING, strlen(EEPROM_MODE_SYSFRU_A_STRING))) {
        mode = EEPROM_MODE_FRU;
    } else {
        mode = -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "eeprom mode %d.\n", mode);
    return mode;
}


static char *dfd_get_system_info_eeprom_path(int index, int sub_index)
{
    uint64_t key;
    char *eeprom_path;

    /* Obtain the eeprom read path */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_PATH, index, sub_index);
    eeprom_path = dfd_ko_cfg_get_item(key);
    if (eeprom_path == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get system_info eeprom path error, e2_type: %d, index: %d, key_name: %s\n",
            index, sub_index, key_to_name(DFD_CFG_ITEM_EEPROM_PATH));
        return NULL;
    }
    return eeprom_path;
}

/**
 * dfd_get_system_info - Obtaining Fan Information
 * @index: Number of the main dev
 * @sub_index: sub index of main dev
 * @cmd: System information type
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_system_info(wb_main_dev_type_t index, uint8_t cmd, char *buf, size_t count)
{
    int rv;
    int eeprom_mode;
    char system_buf[E2_EXTRA_NODE_MAX_LEN];
    char *sysfs_name;
    u_int8_t eeprom[DFD_E2PROM_MAX_LEN];
    uint32_t buf_len;
    int i;
    char tag_name[NODE_MAX_LEN];
    int ret;
    int sub_index;

    if (buf == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "buf is NULL, cmd: 0x%x.\n", cmd);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "buf size error, count: %zu, cmd: 0x%x.\n",
            count, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    rv = dfd_get_dev_number(index, WB_MINOR_DEV_EEPROM);
    if (rv < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "read eeprom number failed\n");
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    for (i = 1; i <= rv; i++) {
        mem_clear(tag_name, sizeof(tag_name));
        ret = dfd_get_eeprom_tag(index, i, tag_name, sizeof(tag_name));
        if (ret < 0) {
            continue;
        }
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "eeprom%d tag:%s\n", i, tag_name);
        if (strstr(tag_name, SYS_EEPROM_STRING) != NULL) {
            sub_index = i;
            break;
        }
    }

    if (i > rv) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "not find syseeprom\n");
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    sysfs_name = dfd_get_system_info_eeprom_path(index, sub_index);
    if (sysfs_name == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get system eeprom path failed, index: %u, cmd: 0x%x.\n",
            sub_index, cmd);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    eeprom_mode = dfd_get_system_eeprom_mode(index, sub_index);
    if (eeprom_mode < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get system eeprom mode failed, index: %u, cmd: 0x%x, ret %d.\n",
            sub_index, cmd, eeprom_mode);
        return eeprom_mode;
    }

    mem_clear(system_buf, sizeof(system_buf));
    buf_len = DFD_E2PROM_MAX_LEN;
    if (eeprom_mode == EEPROM_MODE_TLV) {
        rv = dfd_ko_read_file(sysfs_name, 0, eeprom, sizeof(eeprom));
        if (rv < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "read i2c failed, sysname: %s, rv: %d\n",
                sysfs_name, rv);
            return -DFD_RV_DEV_FAIL;
        }

        rv = dfd_tlvinfo_get_e2prom_info(eeprom, sizeof(eeprom), cmd, system_buf, &buf_len);
    } else {
        rv = dfd_get_fru_product_data_by_file(sysfs_name, cmd, system_buf, sizeof(system_buf));
    }

    if (rv < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "system eeprom read failed");
        return -DFD_RV_DEV_FAIL;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "%s\n", system_buf);

    snprintf(buf, count, "%s\n", system_buf);
    return strlen(buf);
}

static int dfd_get_system_decode_string_value(unsigned int type, int ori_val, char *decode_val, int len)
{
    uint64_t key;
    int p_decode_value;
    int i;
    int current_pos = 0;
    int written;
    int ret;
    info_ctrl_t *info_ctrl;

    if (decode_val == NULL || len <= 0) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "Invalid buffer: decode_val is NULL or len <= 0\n");
        return -DFD_RV_DEV_FAIL;
    }

    if (ori_val == 0) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE,
            "No valid bits found (all bits have no config or no bits set), using default: %s\n",
            DEFAULT_DECODE_STRING);

        current_pos = snprintf(decode_val, len, "%s\n", DEFAULT_DECODE_STRING);
        return current_pos;
    }

    for (i = 0; i < INDEX2_MAX; i++) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_SYSTEM_PWRUP_DECODE_STRING, ori_val, i);
        info_ctrl = dfd_ko_cfg_get_item(key);
        if (info_ctrl == NULL) {
            DFD_SYSTEM_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
            break;
        }

        ret = dfd_info_get_int(key, &p_decode_value, NULL);
        if (ret < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "main_dev_id: %u, cpld%u support upgrade config error, key_name: %s\n",
                ori_val, i, key_to_name(DFD_CFG_ITEM_SYSTEM_PWRUP_DECODE_STRING));
            return ret;
        }

        written = snprintf(decode_val + current_pos, len - current_pos, "reg:0x%x = 0x%x\n", info_ctrl->addr, p_decode_value);
        if (written < 0) {
            DFD_SYSTEM_DEBUG(DBG_VERBOSE, "snprintf error: written = %d\n", written);
            return -DFD_RV_DEV_FAIL;
        }

        current_pos += written;
    }

    if (current_pos == 0) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE,
            "No valid bits found (all bits have no config or no bits set), using default: %s\n",
            DEFAULT_DECODE_UNKNOWN_STRING);

        current_pos = snprintf(decode_val, len, "%s\n", DEFAULT_DECODE_UNKNOWN_STRING);
    }

    return current_pos;
}

ssize_t dfd_system_get_system_value_decode_string(unsigned int type, char *buf, size_t count)
{
    int ret;
    int tmp_value;

    /* get value */
    ret = dfd_get_system_value_step(false, type, 0, &tmp_value);
    if (ret < 0) {
        return ret;
    }

    /* decode value */
    ret = dfd_get_system_decode_string_value(type, tmp_value, buf, count);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "decode string failed, value 0x%x\n", tmp_value);
        return -DFD_RV_DEV_FAIL;
    }

    return ret;
}

static int dfd_get_mem_isolation_type(unsigned int type, int *mem_isolation_type)
{
    uint64_t key;
    info_ctrl_t *info_ctrl;

    if (mem_isolation_type == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "param error, mem_isolation_type is NULL\n");
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_BMC_SYSTEM, type, 0);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get mem isolation type failed, key_name=%s, type=0x%x\n",
            key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (info_ctrl->mode != INFO_CTRL_MODE_CFG) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "mem isolation config mode invalid, type=0x%x, mode=%d\n",
            type, info_ctrl->mode);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    *mem_isolation_type = info_ctrl->int_cons;
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "mem isolation type config, type=0x%x, int_cons=%d\n",
        type, *mem_isolation_type);
    return DFD_RV_OK;
}

static int dfd_system_d17_get_mem_isolation_origin_value(uint8_t *value)
{
    if (value == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "param error, value is NULL\n");
        return -DFD_RV_INVALID_VALUE;
    }

    outb(MEM_ISOLATION_D17_PARAM_VALUE, MEM_ISOLATION_D17_DATA_IO_ADDR);
    outb(MEM_ISOLATION_D17_TRIGGER_VALUE, MEM_ISOLATION_D17_TRIGGER_IO_ADDR);
    *value = inb(MEM_ISOLATION_D17_DATA_IO_ADDR);

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "read d17 mem isolation origin value success, value:0x%x\n", *value);
    return DFD_RV_OK;
}

static int dfd_system_d17_set_mem_isolation_origin_value(uint8_t value)
{
    outb(value, MEM_ISOLATION_D17_DATA_IO_ADDR);
    outb(MEM_ISOLATION_D17_TRIGGER_VALUE, MEM_ISOLATION_D17_TRIGGER_IO_ADDR);

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "set d17 mem isolation origin value success, value:0x%x\n", value);
    return DFD_RV_OK;
}

static ssize_t dfd_system_d17_get_mem_isolation_value(unsigned int type, char *buf, size_t count)
{
    int ret;
    int value;
    uint8_t origin_value;

    ret = dfd_system_d17_get_mem_isolation_origin_value(&origin_value);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get mem isolation origin value failed, type:0x%x ret:%d\n", type, ret);
        return ret;
    }

    value = !!(origin_value & MEM_ISOLATION_STATUS_BIT);
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get mem isolation value success, type:0x%x origin_value:0x%x value:%d\n",
        type, origin_value, value);
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

ssize_t dfd_system_get_mem_isolation_value(unsigned int type, char *buf, size_t count)
{
    int ret;
    int mem_isolation_type;

    if (buf == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "param error buf is NULL, type: 0x%x\n", type);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "buf size error, count: %zu, type: 0x%x\n", count, type);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_mem_isolation_type(type, &mem_isolation_type);
    if (ret < 0) {
        return ret;
    }

    if (mem_isolation_type == MEM_ISOLATION_TYPE_D17) {
        return dfd_system_d17_get_mem_isolation_value(type, buf, count);
    }

    DFD_SYSTEM_DEBUG(DBG_ERROR, "mem isolation type not support, type=0x%x, int_cons=%d\n",
        type, mem_isolation_type);
    return -DFD_RV_DEV_NOTSUPPORT;
}

static ssize_t dfd_set_d17_mem_isolation_value(unsigned int type, int value)
{
    int ret;
    uint8_t origin_value;
    uint8_t new_value;

    ret = dfd_system_d17_get_mem_isolation_origin_value(&origin_value);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get mem isolation origin value before set failed, type:0x%x ret:%d\n", type, ret);
        return ret;
    }

    if (value == MEM_ISOLATION_ENABLE) {
        new_value = origin_value | MEM_ISOLATION_STATUS_BIT;
    } else {
        new_value = origin_value & (uint8_t)(~MEM_ISOLATION_STATUS_BIT);
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "set mem isolation, type=0x%x, origin_value=0x%x, new_value=0x%x\n",
        type, origin_value, new_value);
    ret = dfd_system_d17_set_mem_isolation_origin_value(new_value);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "set mem isolation origin value failed, type:0x%x ret:%d\n", type, ret);
        return ret;
    }

    return DFD_RV_OK;
}

ssize_t dfd_system_set_mem_isolation_value(unsigned int type, int value)
{
    int ret;
    int mem_isolation_type;

    if (value != MEM_ISOLATION_ENABLE && value != MEM_ISOLATION_DISABLE) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "set mem isolation invalid value, type=0x%x value=%d\n", type, value);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_mem_isolation_type(type, &mem_isolation_type);
    if (ret < 0) {
        return ret;
    }

    if (mem_isolation_type == MEM_ISOLATION_TYPE_D17) {
        return dfd_set_d17_mem_isolation_value(type, value);
    }

    DFD_SYSTEM_DEBUG(DBG_ERROR, "mem isolation type not support, type=0x%x, int_cons=%d\n",
        type, mem_isolation_type);
    return -DFD_RV_DEV_NOTSUPPORT;
}
