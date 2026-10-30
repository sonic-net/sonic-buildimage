/*
 * wb_dpu_jaguar_driver.c
 *
 * DPU model-specific tables for Jaguar DPU.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/kernel.h>

#include "wb_dpu_driver.h"
#include "wb_dpu_jaguar_driver.h"
#include "dfd_frueeprom.h"

static ssize_t dfd_get_dpu_temp_common(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static int jaguar_create_eeprom_device(unsigned int main_dev_id, int bus_num, int addr);
static void jaguar_destroy_eeprom_device(unsigned int main_dev_id);
static int jaguar_validate_slot_type(unsigned int slot);
static ssize_t jaguar_get_dpu_eeprom_info(unsigned int main_dev_id, uint8_t cmd, char *buf, size_t count,
    int bus_num, int eeprom_addr, bool is_board_info);

static struct i2c_client *jaguar_eeprom_client[DPU_MAX_NUMBER];
static DEFINE_MUTEX(jaguar_eeprom_lock);
static DEFINE_MUTEX(jaguar_monitor_lock);
static bool jaguar_e2_created_flag[DPU_MAX_NUMBER];

dpu_func_attr_t jaguar_dpu_fw_attr_table[DFD_DPU_FW_TYPE_MAX_E] = {
    [DFD_DPU_FW_BMC_E] = {"BMC Firmware Version",       0x23,       1,      DPU_FORWARD_BASE | DPU_HEX_BASE},
    [DFD_DPU_FW_CPLD_E] = {"CPLD Firmware Version",     0x24,       1,      DPU_FORWARD_BASE | DPU_HEX_BASE},
    [DFD_DPU_FW_UEFI_E] = {"UEFI Firmware Version",     0x25,       1,      DPU_FORWARD_BASE | DPU_HEX_BASE},
    [DFD_DPU_FW_IMU_E] = {"IMU Firmware Version",       0x26,       1,      DPU_FORWARD_BASE | DPU_HEX_BASE},
    [DFD_DPU_FW_MCP_E] = {"MCP Firmware Version",       0x27,       1,      DPU_FORWARD_BASE | DPU_HEX_BASE},
};

dpu_func_attr_t jaguar_dpu_temp_attr_table[DFD_DPU_TEMP_TYPE_MAX_E] = {
    [DFD_DPU_TEMP_BOARD_E] = {"Board Temperature",              0x31,       1,      DPU_FORWARD_BASE | DPU_DECIMAL_BASE | DPU_X1000_BASE},
    [DFD_DPU_TEMP_CHIP_E] = {"Chip Temperature",                0x32,       1,      DPU_FORWARD_BASE | DPU_DECIMAL_BASE | DPU_X1000_BASE},
    [DFD_DPU_TEMP_OPTICAL1_E] = {"Optical module1 Temperature", 0x33,       1,      DPU_FORWARD_BASE | DPU_DECIMAL_BASE | DPU_X1000_BASE},
    [DFD_DPU_TEMP_OPTICAL2_E] = {"Optical module2 Temperature", 0x34,       1,      DPU_FORWARD_BASE | DPU_DECIMAL_BASE | DPU_X1000_BASE},
};

dfd_sysfs_func_map_t jaguar_dpu_func_table[DFD_DPU_MAX_E] = {
    [DFD_DPU_NAME_E] = {dfd_get_dpu_name, NULL},
    [DFD_DPU_VENDOR_E] = {dfd_get_dpu_vendor, NULL},
    [DFD_DPU_DEVICE_E] = {dfd_get_dpu_device, NULL},
    [DFD_DPU_SN_E] = {dfd_get_dpu_sn, NULL},
    [DFD_DPU_PN_E] = {dfd_get_dpu_pn, NULL},
    [DFD_DPU_MAC_E] = {dfd_get_dpu_mac, NULL},
    [DFD_DPU_ID_MONITOR_FLAG_E] = {dfd_get_monitor_flag, NULL},
    [DFD_DPU_FW_ALIAS_E] = {dfd_get_dpu_fw_alias, NULL},
    [DFD_DPU_FW_VERSION_E] = {dfd_get_dpu_fw_version, NULL},
    [DFD_DPU_FW_SUP_UP_E] = {dfd_get_dpu_fw_support_upgrade, NULL},
    [DFD_DPU_FW_UP_TYPE_E] = {dfd_get_dpu_fw_upgrade_type, NULL},
    [DFD_DPU_TEMP_ALIAS_E] = {dfd_get_dpu_temp_alias, NULL},
};

dfd_sysfs_func_map_t jaguar_dpu_temp_func_table[DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E] = {
    [DFD_BMC_MANAGED_SENSOR_INPUT_E] = {dfd_get_dpu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_ALIAS_E] = {dfd_get_dpu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_MAX_E] = {dfd_get_dpu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_MIN_E] = {dfd_get_dpu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_HIGH_E] = {dfd_get_dpu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_LOW_E] = {dfd_get_dpu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E] = {dfd_get_dpu_temp_common, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E] = {dfd_get_dpu_temp_common, NULL},
};

static int jaguar_create_eeprom_device(unsigned int main_dev_id, int bus_num, int addr)
{
    struct i2c_adapter *adapter;
    struct i2c_board_info info;
    unsigned int dpu_id;
    int ret;

    if (main_dev_id < 1 || main_dev_id > DPU_MAX_NUMBER) {
        return -DFD_RV_INVALID_VALUE;
    }

    dpu_id = main_dev_id - 1;

    mutex_lock(&jaguar_eeprom_lock);

    if (jaguar_eeprom_client[dpu_id]) {
        i2c_unregister_device(jaguar_eeprom_client[dpu_id]);
        jaguar_eeprom_client[dpu_id] = NULL;
    }

    ret = 0;
    adapter = i2c_get_adapter(bus_num);
    if (!adapter) {
        ret = -ENODEV;
        goto out_unlock;
    }

    memset(&info, 0, sizeof(info));
    strscpy(info.type, "24c128", I2C_NAME_SIZE);
    info.addr = addr;

    jaguar_eeprom_client[dpu_id] = i2c_new_client_device(adapter, &info);
    if (IS_ERR(jaguar_eeprom_client[dpu_id])) {
        ret = PTR_ERR(jaguar_eeprom_client[dpu_id]);
        jaguar_eeprom_client[dpu_id] = NULL;
    }

    i2c_put_adapter(adapter);

out_unlock:
    mutex_unlock(&jaguar_eeprom_lock);
    return ret;
}

static void jaguar_destroy_eeprom_device(unsigned int main_dev_id)
{
    unsigned int dpu_id;

    if (main_dev_id < 1 || main_dev_id > DPU_MAX_NUMBER) {
        return;
    }

    dpu_id = main_dev_id - 1;

    mutex_lock(&jaguar_eeprom_lock);
    if (jaguar_eeprom_client[dpu_id]) {
        i2c_unregister_device(jaguar_eeprom_client[dpu_id]);
        jaguar_eeprom_client[dpu_id] = NULL;
    }

    mutex_unlock(&jaguar_eeprom_lock);
}

static int jaguar_validate_slot_type(unsigned int slot)
{
    info_ctrl_t *info_ctrl;
    uint64_t key;
    uint16_t value;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int rv;
    int bus_num;
    int dpu_type;

    dpu_type = WB_MAIN_DPU_DEV_JAGUAR;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_DEVICE, dpu_type,
        DFD_GET_DPU_COMMON_KEY2(slot, DFD_DPU_BUSID_E));

    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key))) {
        DBG_DPU_DEBUG(DBG_ERROR, "input arguments error, key: 0x%08llx\n", key);
        return -DFD_RV_INVALID_VALUE;
    }

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DPU_DEBUG(DBG_VERBOSE,
            "can't find dfd config, key: 0x%08llx, dpu_type: 0x%x, KEY2: 0x%x\n",
            key, dpu_type,
            DFD_GET_DPU_COMMON_KEY2(slot, DFD_DPU_BUSID_E));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (info_ctrl->mode != INFO_CTRL_MODE_CFG || info_ctrl->src != INFO_SRC_FILE) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    bus_num = dfd_ko_extract_i2c_bus_number(info_ctrl->fpath);
    if (bus_num < 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "bus_num error.\n");
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_DPU_DEBUG(DBG_VERBOSE, "find dpu bus num, dpu_type %d, bus num %d\n",
        dpu_type, bus_num);

    rv = dfd_ko_i2c_dev_smbus_read(bus_num, info_ctrl->addr, dpu_list[dpu_type].ven_addr, val, 2);
    if (rv < 0) {
        DBG_DPU_DEBUG(DBG_VERBOSE,
            "bus num %d slave addr 0x%x get vendor id(0x%x) failed.\n",
            bus_num, info_ctrl->addr, dpu_list[dpu_type].ven_addr);
        return rv;
    }

    value = (val[1] << 8) | val[0];
    DBG_DPU_DEBUG(DBG_VERBOSE,
        "bus[%d], addr[0x%x], ven_addr[0x%x], ven_val[0x%x], Read vendor ID success value[0x%x].\n",
        bus_num, info_ctrl->addr, dpu_list[dpu_type].ven_addr,
        dpu_list[dpu_type].vendor_id, value);
    if (dpu_list[dpu_type].vendor_id != value) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    rv = dfd_ko_i2c_dev_smbus_read(bus_num, info_ctrl->addr, dpu_list[dpu_type].dev_addr, val, 2);
    if (rv < 0) {
        DBG_DPU_DEBUG(DBG_VERBOSE,
            "bus num %d slave addr 0x%x get device id(0x%x) failed.\n",
            bus_num, info_ctrl->addr, dpu_list[dpu_type].dev_addr);
        return rv;
    }

    value = (val[1] << 8) | val[0];
    DBG_DPU_DEBUG(DBG_VERBOSE,
        "bus[%d], addr[0x%x], dev_addr[0x%x], dev_val[0x%x], Read device ID success value[0x%x].\n",
        bus_num, info_ctrl->addr, dpu_list[dpu_type].dev_addr,
        dpu_list[dpu_type].device_id, value);
    if (dpu_list[dpu_type].device_id != value) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return 0;
}

static int jaguar_get_dpu_vendor_id(unsigned int main_dev_id, int dpu_type, uint16_t *vendor_id)
{
    info_ctrl_t *info_ctrl;
    uint64_t key;
    uint8_t val[INFO_INT_MAX_LEN + 1] = {0};
    int bus_num;
    int rv;

    if (vendor_id == NULL || dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX) {
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_DEVICE, dpu_type,
        DFD_GET_DPU_COMMON_KEY2(main_dev_id, DFD_DPU_BUSID_E));

    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key))) {
        DBG_DPU_DEBUG(DBG_ERROR, "input arguments error, key: 0x%08llx\n", key);
        return -DFD_RV_INVALID_VALUE;
    }

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "can't find dfd config, key: 0x%08llx, dpu_type: 0x%x, KEY2: 0x%x\n",
            key, dpu_type,
            DFD_GET_DPU_COMMON_KEY2(main_dev_id, DFD_DPU_BUSID_E));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    bus_num = dfd_ko_extract_i2c_bus_number(info_ctrl->fpath);
    if (bus_num < 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "bus_num error.\n");
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    rv = dfd_ko_i2c_dev_smbus_read(bus_num, info_ctrl->addr, dpu_list[dpu_type].ven_addr, val, 2);
    if (rv < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "bus num %d slave addr 0x%x get vendor id(0x%x) failed.\n",
            bus_num, info_ctrl->addr, dpu_list[dpu_type].ven_addr);
        return rv;
    }

    *vendor_id = (val[1] << 8) | val[0];
    DBG_DPU_DEBUG(DBG_VERBOSE,
        "bus[%d], addr[0x%x], ven_addr[0x%x], Read vendor ID success value[0x%x].\n",
        bus_num, info_ctrl->addr, dpu_list[dpu_type].ven_addr, *vendor_id);

    return 0;
}

static int jaguar_get_dpu_device_id(unsigned int main_dev_id, int dpu_type, uint16_t *device_id)
{
    info_ctrl_t *info_ctrl;
    uint64_t key;
    uint8_t val[INFO_INT_MAX_LEN + 1] = {0};
    int bus_num;
    int rv;

    if (device_id == NULL || dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX) {
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_DEVICE, dpu_type,
        DFD_GET_DPU_COMMON_KEY2(main_dev_id, DFD_DPU_BUSID_E));

    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key))) {
        DBG_DPU_DEBUG(DBG_ERROR, "input arguments error, key: 0x%08llx\n", key);
        return -DFD_RV_INVALID_VALUE;
    }

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "can't find dfd config, key: 0x%08llx, dpu_type: 0x%x, KEY2: 0x%x\n",
            key, dpu_type,
            DFD_GET_DPU_COMMON_KEY2(main_dev_id, DFD_DPU_BUSID_E));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    bus_num = dfd_ko_extract_i2c_bus_number(info_ctrl->fpath);
    if (bus_num < 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "bus_num error.\n");
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    rv = dfd_ko_i2c_dev_smbus_read(bus_num, info_ctrl->addr, dpu_list[dpu_type].dev_addr, val, 2);
    if (rv < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "bus num %d slave addr 0x%x get device id(0x%x) failed.\n",
            bus_num, info_ctrl->addr, dpu_list[dpu_type].dev_addr);
        return rv;
    }

    *device_id = (val[1] << 8) | val[0];
    DBG_DPU_DEBUG(DBG_VERBOSE,
        "bus[%d], addr[0x%x], dev_addr[0x%x], Read device ID success value[0x%x].\n",
        bus_num, info_ctrl->addr, dpu_list[dpu_type].dev_addr, *device_id);

    return 0;
}

static ssize_t jaguar_get_dpu_eeprom_info(unsigned int main_dev_id, uint8_t cmd, char *buf, size_t count,
    int bus_num, int eeprom_addr, bool is_board_info)
{
    unsigned int dpu_id;
    int rv;
    char dpu_buf[DPU_EEPROM_SIZE];
    char sysfs_name[INFO_FPATH_MAX_LEN];

    dpu_id = main_dev_id;

    if (dpu_id < 1 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "dpu_id error, dpu_id:%u\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (buf == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR, "buf is NULL, cmd: 0x%x.\n", cmd);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "buf size error, count: %zu, cmd: 0x%x.\n", count, cmd);
        return -DFD_RV_INVALID_VALUE;
    }
    if (bus_num < 0) {
        DBG_DPU_DEBUG(DBG_ERROR, "invalid bus num: %d, cmd: 0x%x.\n", bus_num, cmd);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    mem_clear(buf, count);

    mutex_lock(&jaguar_eeprom_lock);
    if (jaguar_eeprom_client[dpu_id - 1] == NULL) {
        mutex_unlock(&jaguar_eeprom_lock);
        DBG_DPU_DEBUG(DBG_ERROR, "eeprom client not ready, dpu_id:%u\n", dpu_id);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    snprintf(sysfs_name, sizeof(sysfs_name), "/sys/bus/i2c/devices/%d-%04x/eeprom", bus_num, eeprom_addr);
    mem_clear(dpu_buf, sizeof(dpu_buf));
    DBG_DPU_DEBUG(DBG_VERBOSE, "sysfs_name: %s\n", sysfs_name);
    if (is_board_info) {
        rv = dfd_get_fru_board_data_by_file(sysfs_name, cmd, dpu_buf, sizeof(dpu_buf));
        if (rv) {
            mutex_unlock(&jaguar_eeprom_lock);
            DBG_DPU_DEBUG(DBG_ERROR, "dpu eeprom read failed");
            return -DFD_RV_DEV_FAIL;
        }
    } else {
        rv = dfd_get_fru_product_data_by_file(sysfs_name, cmd, dpu_buf, sizeof(dpu_buf));
        if (rv) {
            mutex_unlock(&jaguar_eeprom_lock);
            DBG_DPU_DEBUG(DBG_ERROR, "dpu eeprom read failed");
            return -DFD_RV_DEV_FAIL;
        }
    }

    mutex_unlock(&jaguar_eeprom_lock);

    DBG_DPU_DEBUG(DBG_VERBOSE, "%s\n", dpu_buf);

    dfd_ko_trim_trailing_spaces(dpu_buf);
    snprintf(buf, count, "%s\n", dpu_buf);
    return strlen(buf);
}

ssize_t dfd_get_dpu_name(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int bus_num;
    int dpu_type;
    unsigned int dpu_id;

    dpu_id = main_dev_id;
    if (dpu_id <= 0 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "dpu_id error, dpu_id:%u\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX ) {
        return -DFD_RV_INVALID_VALUE;
    }

    bus_num = dfd_get_dpu_bus_num(dpu_id);
    return jaguar_get_dpu_eeprom_info(dpu_id, DFD_DEV_INFO_TYPE_NAME, buf, count, bus_num,
        dpu_list[dpu_type].eeprom_addr, false);
}

ssize_t dfd_get_dpu_vendor(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int dpu_type;
    uint16_t vendor_id;
    int rv;
    unsigned int dpu_id;

    dpu_id = main_dev_id;
    if (dpu_id <= 0 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "dpu_id error, dpu_id:%u\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX ) {
        return -DFD_RV_INVALID_VALUE;
    }

    rv = jaguar_get_dpu_vendor_id(dpu_id, dpu_type, &vendor_id);
    if (rv == 0) {
        return (ssize_t)snprintf(buf, count, "0x%x\n", vendor_id);
    }

    DBG_DPU_DEBUG(DBG_ERROR, "get vendor id by i2c failed, dpu_id:%u, rv:%d\n", dpu_id, rv);
    return rv;
}

ssize_t dfd_get_dpu_device(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int dpu_type;
    uint16_t device_id;
    int rv;
    unsigned int dpu_id;

    dpu_id = main_dev_id;
    if (dpu_id <= 0 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "dpu_id error, dpu_id:%u\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX ) {
        return -DFD_RV_INVALID_VALUE;
    }

    rv = jaguar_get_dpu_device_id(dpu_id, dpu_type, &device_id);
    if (rv == 0) {
        return (ssize_t)snprintf(buf, count, "0x%x\n", device_id);
    }

    DBG_DPU_DEBUG(DBG_ERROR, "get device id by i2c failed, dpu_id:%u, rv:%d\n", dpu_id, rv);
    return rv;
}

ssize_t dfd_get_dpu_sn(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int bus_num;
    int dpu_type;
    unsigned int dpu_id;

    dpu_id = main_dev_id;
    if (dpu_id <= 0 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "dpu_id error, dpu_id:%d\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX ) {
        return -DFD_RV_INVALID_VALUE;
    }

    bus_num = dfd_get_dpu_bus_num(dpu_id);
    return jaguar_get_dpu_eeprom_info(dpu_id, DFD_DEV_INFO_TYPE_SN, buf, count, bus_num,
        dpu_list[dpu_type].eeprom_addr, false);
}

ssize_t dfd_get_dpu_pn(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int bus_num;
    int dpu_type;
    unsigned int dpu_id;

    dpu_id = main_dev_id;
    if (dpu_id <= 0 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "dpu_id error, dpu_id:%d\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX ) {
        return -DFD_RV_INVALID_VALUE;
    }

    bus_num = dfd_get_dpu_bus_num(dpu_id);
    return jaguar_get_dpu_eeprom_info(dpu_id, DFD_DEV_INFO_TYPE_PART_NUMBER, buf, count, bus_num,
        dpu_list[dpu_type].eeprom_addr, false);
}

ssize_t dfd_get_dpu_mac(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int bus_num;
    int dpu_type;
    int dpu_id;

    dpu_id = main_dev_id;
    if (dpu_id <= 0 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "dpu_id error, dpu_id:%d\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX ) {
        return -DFD_RV_INVALID_VALUE;
    }

    bus_num = dfd_get_dpu_bus_num(dpu_id);
    DBG_DPU_DEBUG(DBG_VERBOSE, "dfd_get_dpu_mac: bus_num=%d, eeprom_addr=0x%x\n",
        bus_num, dpu_list[dpu_type].eeprom_addr);
    return jaguar_get_dpu_eeprom_info(dpu_id, DFD_DEV_INFO_TYPE_EXTRA1, buf, count, bus_num,
        dpu_list[dpu_type].eeprom_addr, false);
}

ssize_t dfd_get_monitor_flag(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int dpu_type, dpu_mf_num;
    int *m_num;
    int rv, data, tmp_data, i;
    int *p_decode_value;
    info_ctrl_t *info_ctrl;
    uint64_t key;
    int bus_num;
    ssize_t ret;
    unsigned int dpu_id;

    dpu_id = main_dev_id;
    if (dpu_id < 1 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "Invalid dpu_id: %u\n", dpu_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_DPU_DEBUG(DBG_VERBOSE, "dfd_get_monitor_flag: main_dev_id: %u, sub_dev_id: %u\n",
        main_dev_id, sub_dev_id);

    mutex_lock(&jaguar_monitor_lock);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_MONITOR_NODE_NUM, dpu_id, 0);
    m_num = dfd_ko_cfg_get_item(key);
    if (m_num == NULL) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get monitor flag number failed, dpu_id: %u key : %llx\n",
            dpu_id, key);
        ret = -DFD_RV_DEV_NOTSUPPORT;
        goto out_unlock;
    }

    dpu_mf_num = *m_num;

    tmp_data = 1;
    for (i = 1; i <= dpu_mf_num; i++) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_MONITOR_FLAG, dpu_id, i);
        info_ctrl = dfd_ko_cfg_get_item(key);
        if (info_ctrl == NULL) {
            DBG_DPU_DEBUG(DBG_VERBOSE, "get info ctrl failed, key=0x%08llx\n", key);
            ret = (ssize_t)snprintf(buf, count, "%d\n", WB_SENSOR_MONITOR_YES);
            goto out_unlock;
        }

        rv = dfd_info_get_int(key, &data, NULL);
        if (rv < 0) {
            DBG_DPU_DEBUG(DBG_ERROR, "get monitor flag error, rv: %d\n", rv);
            ret = (ssize_t)snprintf(buf, count, "%d\n", 0);
            goto out_unlock;
        }

        key = DFD_CFG_KEY(DFD_CFG_ITEM_DPU_DECODE_MONITOR_FLAG, dpu_id, data);
        p_decode_value = dfd_ko_cfg_get_item(key);
        if (p_decode_value == NULL) {
            DBG_DPU_DEBUG(DBG_VERBOSE, "status needn't decode. value:0x%x\n", data);
        } else {
            DBG_DPU_DEBUG(DBG_VERBOSE, "ori_value:0x%x, decoded value:0x%x\n", data, *p_decode_value);
            data = *p_decode_value;
        }

        DBG_DPU_DEBUG(DBG_VERBOSE, "dpu_id: %u, data = %d\n", dpu_id, data);

        tmp_data &= data;
        if (tmp_data == 0) {
            DBG_DPU_DEBUG(DBG_VERBOSE, "tmp_data == 0 data = %d, break\n", data);
            break;
        }
    }

    dpu_type = WB_MAIN_DPU_DEV_JAGUAR;

    if (dpu_id_info_table[dpu_id - 1].monitor_last_flag == -1) {
        dpu_id_info_table[dpu_id - 1].monitor_last_flag = tmp_data;
        if (tmp_data == 0) {
            DBG_DPU_DEBUG(DBG_ERROR, "dpu is offline, we cant monitor it.\n");
            goto monitor_unready;
        }

        DBG_DPU_DEBUG(DBG_VERBOSE,
            "first monitor dpu_id: %u, tmp_data = %d last_flag = %d\n",
            dpu_id, tmp_data, dpu_id_info_table[dpu_id - 1].monitor_last_flag);
        rv = jaguar_validate_slot_type(dpu_id);
        if (rv < 0) {
            DBG_DPU_DEBUG(DBG_ERROR, "dpu_id: %u, sub_dev_id %u config error.\n",
                dpu_id, sub_dev_id);
            goto monitor_type_mismatch;
        }
        goto monitor_ready;

    } else if (tmp_data == 1 && dpu_id_info_table[dpu_id - 1].monitor_last_flag == 0) {
            rv = jaguar_validate_slot_type(dpu_id);
            if (rv < 0) {
                DBG_DPU_DEBUG(DBG_ERROR, "dpu_id: %u, sub_dev_id %u config error.\n",
                    dpu_id, sub_dev_id);
                goto monitor_type_mismatch;
            }

            goto monitor_ready;
    } else if (tmp_data == 0) {
            DBG_DPU_DEBUG(DBG_ERROR, "dpu is offline, we cant monitor it.\n");
            goto monitor_unready;
    } else {
        DBG_DPU_DEBUG(DBG_VERBOSE, "monitor flag no change, dpu_id: %u, tmp_data = %d\n",
            dpu_id, tmp_data);
    }
    

    ret = (ssize_t)snprintf(buf, count, "%d\n", 1);
    goto out_unlock;

monitor_ready:
    dpu_id_info_table[dpu_id - 1].dpu_type = dpu_type;
    if (!jaguar_e2_created_flag[dpu_id - 1]) {
        bus_num = dfd_get_dpu_bus_num(dpu_id);
        rv = jaguar_create_eeprom_device(dpu_id, bus_num, dpu_list[dpu_type].eeprom_addr);
        if (rv < 0) {
            DBG_DPU_DEBUG(DBG_ERROR,
                "create jaguar eeprom device failed, bus_num:%d, addr:0x%x, ret:%d\n",
                bus_num, dpu_list[dpu_type].eeprom_addr, rv);
            goto monitor_unready;
        }
        jaguar_e2_created_flag[dpu_id - 1] = true;
    }

    DBG_DPU_DEBUG(DBG_VERBOSE,
        "slot(%d) dpu_type is %d.\n",
        main_dev_id, dpu_id_info_table[dpu_id - 1].dpu_type);
    dpu_id_info_table[dpu_id - 1].monitor_last_flag = tmp_data;
    ret = (ssize_t)snprintf(buf, count, "%d\n", 1);
    goto out_unlock;

monitor_unready:
    jaguar_destroy_eeprom_device(dpu_id);
    jaguar_e2_created_flag[dpu_id - 1] = false;
    dpu_id_info_table[dpu_id - 1].dpu_type = -1;
    dpu_id_info_table[dpu_id - 1].monitor_last_flag = 0;
    ret = (ssize_t)snprintf(buf, count, "%d\n", 0);
    goto out_unlock;

monitor_type_mismatch:
    jaguar_destroy_eeprom_device(dpu_id);
    jaguar_e2_created_flag[dpu_id - 1] = false;
    dpu_id_info_table[dpu_id - 1].dpu_type = -1;
    dpu_id_info_table[dpu_id - 1].monitor_last_flag = 0;
    ret = rv;

out_unlock:
    mutex_unlock(&jaguar_monitor_lock);
    return ret;
}

static ssize_t dfd_get_dpu_temp_common(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int ret, dpu_type;
    int dpu_id, temp_index;
    uint8_t val[INFO_INT_MAX_LEN + 1];

    dpu_id = DFD_DPU_GET_SENSOR_DPU_ID(main_dev_id);
    if (buf == NULL || dpu_id < 1 || dpu_id > DPU_MAX_NUMBER) {
        DBG_DPU_DEBUG(DBG_ERROR, "param error, main_dev_id: 0x%x, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_DPU_DEBUG(DBG_VERBOSE,
        "dfd_get_dpu_temp_common has been called main_dev_id:0x%x sub_dev_id:0x%x.\n",
        main_dev_id, sub_dev_id);

    mem_clear(buf, count);
    dpu_type = dfd_get_dpu_type_by_slot(dpu_id);
    if (dpu_type < 0 || dpu_type >= WB_MAIN_DPU_DEV_MAX || sub_dev_id > DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E - 1) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "main_dev_id: %u, sub_dev_id %u config error, dpu_type %d.\n",
            main_dev_id, sub_dev_id, dpu_type);
        return -DFD_RV_INVALID_VALUE;
    }

    temp_index = DFD_DPU_GET_SENSOR_SENSOR_ID(main_dev_id);
    if (temp_index <= 0 || temp_index >= DFD_DPU_TEMP_TYPE_MAX_E || (dpu_list[dpu_type].temp_attr_table[temp_index].size > INFO_INT_MAX_LEN)) {
        DBG_DPU_DEBUG(DBG_ERROR, "temp_index %d or size out of range.\n", temp_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(val, INFO_INT_MAX_LEN + 1);
    ret = get_dpu_temp_para_val(dpu_id, sub_dev_id, temp_index, dpu_type, val, buf,
        dpu_list[dpu_type].temp_attr_table[temp_index], count);
    if (ret < 0) {
        DBG_DPU_DEBUG(DBG_ERROR,
            "get_dpu_temp_para_val failed, main_dev_id: 0x%x, sub_dev_id: %u, ret: %d\n",
            main_dev_id, sub_dev_id, ret);
        return ret;
    }

    if (ret > 0) {
        return ret;
    }

    if (temp_index == DFD_DPU_TEMP_OPTICAL1_E || temp_index == DFD_DPU_TEMP_OPTICAL2_E) {
        if (val[0] == DPU_OP_NOT_PRESENT || val[0] == DPU_OP_ERROR) {
            DBG_DPU_DEBUG(DBG_ERROR, "invalid temp value read from device, value: 0x%x\n", val[0]);
            return -DFD_RV_DEV_NOTSUPPORT;
        }
    }

    if (IS_TARGET_SENSOR_STATIC(sub_dev_id)) {
        return strlen(buf);
    }

    return dpu_data_to_buf(buf, count, val,
        dpu_list[dpu_type].temp_attr_table[temp_index].size,
        dpu_list[dpu_type].temp_attr_table[temp_index].base);
}
