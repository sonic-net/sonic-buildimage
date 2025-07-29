#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_system_driver.h"
#include "switch_driver.h"
#include "dfd_frueeprom.h"
#include "dfd_tlveeprom.h"

#define NODE_MAX_LEN               (64)
#define E2_EXTRA_NODE_MAX_LEN      (128)
#define DFD_E2PROM_MAX_LEN          (256)

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

static int dfd_system_need_reverse(unsigned int type, int index)
{
    int ret;
    ret = is_equal_to_extra1(DFD_CFG_ITEM_REVERSE_BMC_SYSTEM, type, index);
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
int dfd_get_cmd_count(unsigned int type)
{
    uint64_t key;
    int cmd_num;
    int *p_cmd_num;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_BMC_SYSTEM_CMD_NUM, type, 0);
    p_cmd_num = dfd_ko_cfg_get_item(key);
    if (p_cmd_num == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get cmd number failed, key_name:%s\n",
            key_to_name(DFD_CFG_ITEM_BMC_SYSTEM_CMD_NUM));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    cmd_num = *p_cmd_num;
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get cmd number ok, type:0x%x, number:%d\n", type, cmd_num);
    return cmd_num;
}

void dfd_cmd_delay(unsigned int usdelay)
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

static ssize_t dfd_system_get_system_value_internal(unsigned int type, int *value, bool need_match)
{
    uint64_t key;
    int ret;
    info_ctrl_t *info_ctrl;
    int *p_decode_value;
    int need_reverse;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_BMC_SYSTEM, type, 0);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get info ctrl fail, key_name: %s, type=0x%x\n",
            key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "get, key_name: %s, type=0x%x\n",
        key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type);
    ret = dfd_info_get_int(key, value, NULL);
    if (ret < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get system value error, key_name: %s, type=0x%x, ret:%d\n",
            key_to_name(DFD_CFG_ITEM_BMC_SYSTEM), type, ret);
        return ret;
    }

    need_reverse = dfd_system_need_reverse(type, 0);
    if (need_reverse < 0) {
        return need_reverse;
    }
    if (need_reverse) {
        if (IS_INFO_FRMT_BIT(info_ctrl->frmt)) {
            *value = ~(*value) & (GENMASK(info_ctrl->len - 1, 0));
        } else {
            *value = ~(*value);
        }
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SYSTEM_STATUS_DECODE, type, *value);
    p_decode_value = dfd_ko_cfg_get_item(key);
    if (p_decode_value == NULL) {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "type:%d, status needn't decode. value:0x%x\n", type, *value);
        if (need_match) {
            return -DFD_RV_INVALID_VALUE;
        }
    } else {
        DFD_SYSTEM_DEBUG(DBG_VERBOSE, "type:%d, ori_value:0x%x, decoded value:0x%x\n", type, *value, *p_decode_value);
        *value = *p_decode_value;
    }

    return DFD_RV_OK;
}

ssize_t dfd_system_get_system_value(unsigned int type, int *value)
{
    return dfd_system_get_system_value_internal(type, value, false);
}

ssize_t dfd_system_get_system_value_match_status(unsigned int type, int *value)
{
    return dfd_system_get_system_value_internal(type, value, true);
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
    ret = dfd_get_cmd_count(type_detail);
    if(ret <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get cmd number, type_detail=0x%x\n", type_detail);
        return -DFD_RV_DEV_NOTSUPPORT;
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

        need_reverse = dfd_system_need_reverse(type_detail, cmd_i);
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
    ret = dfd_get_cmd_count(type_detail);
    if(ret <= 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get cmd number, type_detail=0x%x\n", type_detail);
        return -DFD_RV_DEV_NOTSUPPORT;
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
        return EEPROM_MODE_TLV;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "eeprom mode_name %s.\n", name);
    if (!strncmp(name, EEPROM_MODE_TLV_STRING, strlen(EEPROM_MODE_TLV_STRING))) {
        mode = EEPROM_MODE_TLV;
    } else if (!strncmp(name, EEPROM_MODE_FRU_STRING, strlen(EEPROM_MODE_FRU_STRING))) {
        mode = EEPROM_MODE_FRU;
    } else if (!strncmp(name, EEPROM_MODE_TX_FRU_STRING, strlen(EEPROM_MODE_TX_FRU_STRING))) {
        mode = EEPROM_MODE_FRU;
    } else {
        /* The default TLV mode is returned */
        mode = EEPROM_MODE_TLV;
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
ssize_t dfd_get_system_info(wb_main_dev_type_t index, int sub_index, uint8_t cmd, char *buf, size_t count)
{
    int rv;
    int eeprom_mode;
    char system_buf[E2_EXTRA_NODE_MAX_LEN];
    char *sysfs_name;
    u_int8_t eeprom[DFD_E2PROM_MAX_LEN];
    uint32_t buf_leng;

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
    sysfs_name = dfd_get_system_info_eeprom_path(index, sub_index);
    if (sysfs_name == NULL) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "get system eeprom path failed, index: %u, cmd: 0x%x.\n",
            sub_index, cmd);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    eeprom_mode = dfd_get_system_eeprom_mode(index, sub_index);
    mem_clear(system_buf, E2_EXTRA_NODE_MAX_LEN);
    buf_leng = DFD_E2PROM_MAX_LEN;
    if (eeprom_mode == EEPROM_MODE_TLV) {
        rv = dfd_ko_read_file(sysfs_name, 0, eeprom, DFD_E2PROM_MAX_LEN);
        if (rv < 0) {
            DFD_SYSTEM_DEBUG(DBG_ERROR, "read i2c failed, sysname: %s, rv: %d\n",
                sysfs_name, rv);
            return -DFD_RV_DEV_FAIL;
        }

        rv = dfd_tlvinfo_get_e2prom_info(eeprom, DFD_E2PROM_MAX_LEN, cmd, system_buf, &buf_leng);
    } else {
        rv = dfd_get_fru_product_data_by_file(sysfs_name, cmd, system_buf, E2_EXTRA_NODE_MAX_LEN);
    }

    if (rv < 0) {
        DFD_SYSTEM_DEBUG(DBG_ERROR, "system eeprom read failed");
        return -DFD_RV_DEV_FAIL;
    }

    DFD_SYSTEM_DEBUG(DBG_VERBOSE, "%s\n", system_buf);

    snprintf(buf, count, "%s\n", system_buf);
    return strlen(buf);
}

