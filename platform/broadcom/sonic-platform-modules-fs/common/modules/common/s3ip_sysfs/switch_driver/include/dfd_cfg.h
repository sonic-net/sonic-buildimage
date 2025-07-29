#ifndef __DFD_CFG_H__
#define __DFD_CFG_H__

#include <linux/list.h>

#define DFD_KO_FILE_NAME_DIR       "/etc/s3ip_sysfs_cfg/file_name/"       /* Library configuration file name directory */
#define DFD_KO_CFG_FILE_DIR        "/etc/s3ip_sysfs_cfg/cfg_file/"        /* Library configuration file directory */
#define DFD_PUB_CARDTYPE_FILE      "/sys/module/platform_common/parameters/dfd_my_type"

#define DFD_CFG_CMDLINE_MAX_LEN (256)   /* The maximum length of the command line is specified */
#define DFD_CFG_NAME_MAX_LEN    (256)   /* The maximum length of a name is specified */
#define DFD_CFG_VALUE_MAX_LEN   (256)   /* The maximum length of the configuration value */
#define DFD_CFG_STR_MAX_LEN     (64)    /* The maximum length of a character string is specified */
#define DFD_CFG_CPLD_NUM_MAX    (16)    /* Maximum number of cpld */
#define DFD_PRODUCT_ID_LENGTH   (8)
#define DFD_PID_BUF_LEN         (32)
#define DFD_TEMP_NAME_BUF_LEN   (32)

#define DFD_CFG_EMPTY_VALUE     (-1)    /* Null configuration value */
#define DFD_CFG_INVALID_VALUE   (0)     /* Configuring an illegal value */

/* Set the key value of the binary tree */
#define DFD_CFG_KEY(item, index1, index2) \
    (((((uint64_t)item) & 0xffff) << 24) | (((index1) & 0xffff) << 8) | ((index2) & 0xff))
#define DFD_CFG_ITEM_ID(key)    (((key) >> 24) & 0xffff)
#define DFD_CFG_INDEX1(key)     (((key) >> 8) & 0xffff)
#define DFD_CFG_INDEX2(key)     ((key)& 0xff)

/* Index range */
#define INDEX_NOT_EXIST     (-1)
#define INDEX1_MAX           (0xffff)
#define INDEX2_MAX           (0xff)

#define DFD_CFG_ITEM_ALL \
    DFD_CFG_ITEM(DFD_CFG_ITEM_NONE, "none", INDEX_NOT_EXIST, INDEX_NOT_EXIST)                       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_DEV_NUM, "dev_num", INDEX1_MAX, INDEX2_MAX)                 \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SFF_POWER_GROUP, "sff_power_group", INDEX1_MAX, INDEX_NOT_EXIST)                 \
    DFD_CFG_ITEM(DFD_CFG_ITEM_BMC_SYSTEM_CMD_NUM, "bmc_system_cmd_num", INDEX1_MAX, INDEX_NOT_EXIST)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_THRESHOLD, "fan_threshold", INDEX1_MAX, INDEX2_MAX)   \
    DFD_CFG_ITEM(DFD_CFG_ITEM_LED_STATUS_DECODE, "led_status_decode", INDEX1_MAX, INDEX2_MAX)   \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SYSTEM_STATUS_DECODE, "system_status_decode", INDEX1_MAX, INDEX2_MAX)   \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_LPC_DEV, "cpld_lpc_dev", INDEX1_MAX, DFD_CFG_CPLD_NUM_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_TYPE_NUM, "fan_type_num", INDEX1_MAX, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_EEPROM_SIZE, "eeprom_size", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_EEPROM_BANK, "eeprom_bank", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_DECODE_POWER_FAN_DIR, "decode_power_fan_dir", INDEX1_MAX, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_WATCHDOG_ID, "watchdog_id", INDEX1_MAX, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_POWER_RSUPPLY, "power_rate_supply", INDEX1_MAX, INDEX_NOT_EXIST)           \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_DIRECTION, "fan_direction", INDEX1_MAX, INDEX2_MAX)   \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_TEMP_MONITOR_DC, "dc_monitor_flag_hwmon_temp", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_IN_MONITOR_FLAG_DC, "dc_monitor_flag_hwmon_in", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_CURR_MONITOR_FLAG_DC, "dc_monitor_flag_hwmon_curr", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_REG_KEY, "reg_key", INDEX1_MAX, INDEX2_MAX)                 \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SYSFS_DECODE, "sysfs_decode", INDEX1_MAX, INDEX2_MAX)                 \
    DFD_CFG_ITEM(DFD_CFG_ITEM_EEPROM_BUS, "eeprom_bus", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_EEPROM_ADDR, "eeprom_addr", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_WITHOUT_E2, "fan_without_e2", INDEX_NOT_EXIST, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_PMBUS_ADDR, "psu_pmbus_addr", INDEX1_MAX, INDEX_NOT_EXIST)            \
    DFD_CFG_ITEM(DFD_CFG_ITEM_INT_END, "end_int", INDEX_NOT_EXIST, INDEX_NOT_EXIST)                 \
                                                                                                    \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_MODE, "mode_cpld", INDEX1_MAX, DFD_CFG_CPLD_NUM_MAX)                 \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_NAME, "cpld_name", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_TYPE, "cpld_type", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_VENDOR, "cpld_vendor", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_UPGRADE_ACTIVE_TYPE, "cpld_upgrade_active_type", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FPGA_NAME, "fpga_name", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FPGA_TYPE, "fpga_type", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FPGA_VENDOR, "fpga_vendor", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FPGA_UPGRADE_ACTIVE_TYPE, "fpga_upgrade_active_type", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FPGA_MODEL_DECODE, "fpga_model_decode", INDEX1_MAX, INDEX_NOT_EXIST)   \
    DFD_CFG_ITEM(DFD_CFG_ITEM_MISC_FW_NAME, "misc_fw_name", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_MISC_FW_TYPE, "misc_fw_type", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_MISC_FW_UPGRADE_ACTIVE_TYPE, "misc_fw_upgrade_active_type", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_MISC_FW_VENDOR, "misc_fw_vendor", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_E2_MODE, "fan_e2_mode", INDEX_NOT_EXIST, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_FRU_MODE, "psu_fru_mode", INDEX_NOT_EXIST, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_SYSFS_NAME, "fan_sysfs_name", INDEX_NOT_EXIST, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_POWER_NAME, "power_name", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_NAME, "fan_name", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_DECODE_POWER_NAME, "decode_power_name", INDEX1_MAX, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_SPEED_CAL, "fan_speed_cal", INDEX1_MAX, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_DECODE_FAN_NAME, "decode_fan_name", INDEX1_MAX, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_EEPROM_PATH, "eeprom_path", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_WATCHDOG_NAME, "watchdog_name", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_SYSFS_NAME, "psu_sysfs_name", INDEX_NOT_EXIST, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SLOT_SYSFS_NAME, "slot_sysfs_name", INDEX_NOT_EXIST, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_MY_SLOT_ID_SYSFS_NAME, "my_slot_id_sysfs_name", INDEX_NOT_EXIST, INDEX_NOT_EXIST)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_EEPROM_ALIAS, "eeprom_alias", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_EEPROM_TAG, "eeprom_tag", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_EEPROM_TYPE, "eeprom_type", INDEX1_MAX, INDEX2_MAX)       \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_BLACKBOX_PATH, "psu_blackbox_path", INDEX1_MAX, INDEX_NOT_EXIST)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_PMBUS_INFO, "psu_pmbus_info", INDEX1_MAX, INDEX_NOT_EXIST)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_CLEAR_BLACKBOX, "psu_clear_blackbox", INDEX1_MAX, INDEX_NOT_EXIST)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_RESET, "psu_reset", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_UPGRADE_ACTIVE_TYPE, "psu_upgrade_active_type", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_LOGIC_DEV, "cpld_logic_dev", INDEX1_MAX, INDEX2_MAX)     \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SLOT_CARD_TYPE_TO_NAME, "slot_card_type_to_name", INDEX1_MAX, INDEX_NOT_EXIST)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_STRING_END, "end_string", INDEX_NOT_EXIST, INDEX_NOT_EXIST)           \
                                                                                                    \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_I2C_DEV, "cpld_i2c_dev", INDEX1_MAX, INDEX2_MAX)           \
    DFD_CFG_ITEM(DFD_CFG_ITEM_OTHER_I2C_DEV, "other_i2c_dev", INDEX1_MAX, INDEX2_MAX)         \
    DFD_CFG_ITEM(DFD_CFG_ITEM_I2C_DEV_END, "end_i2c_dev", INDEX_NOT_EXIST, INDEX_NOT_EXIST)         \
                                                                                                    \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_ROLL_STATUS, "fan_roll_status", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_SPEED, "fan_speed", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FAN_RATIO, "fan_ratio", INDEX1_MAX, INDEX_NOT_EXIST)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_LED_STATUS, "led_status", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_VERSION, "cpld_version", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_HW_VERSION, "cpld_hw_version", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_TEST_REG, "cpld_test_reg", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CPLD_SUPPORT_UPGRADE, "cpld_support_upgrade", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_MISC_FW_VERSION, "misc_fw_version", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_MISC_FW_HW_VERSION, "misc_fw_hw_version", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_DEV_PRESENT_STATUS, "dev_present_status", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_STATUS, "psu_status", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_TEMP, "hwmon_temp", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_TEMP_VALID_FLAG, "temp_valid_flag", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_VOL_VALID_FLAG, "vol_valid_flag", INDEX1_MAX, INDEX2_MAX)    \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CURR_VALID_FLAG, "curr_valid_flag", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_POWER_VALID_FLAG, "power_valid_flag", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_TEMP_MONITOR_FLAG, "monitor_flag_hwmon_temp", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_IN, "hwmon_in", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_IN_MONITOR_FLAG, "monitor_flag_hwmon_in", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_CURR, "hwmon_curr", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_CURR_MONITOR_FLAG, "monitor_flag_hwmon_curr", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_PSU, "hwmon_psu", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SFF_OPTOE_TYPE, "sff_optoe_type", INDEX1_MAX, INDEX_NOT_EXIST)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_HWMON_POWER, "hwmon_power", INDEX1_MAX, INDEX2_MAX) \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SFF_CPLD_REG, "sff_cpld_reg", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PORT_POWER_GROUP_REG, "port_power_group_reg", INDEX1_MAX, INDEX_NOT_EXIST)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FPGA_VERSION, "fpga_version", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FPGA_TEST_REG, "fpga_test_reg", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FPGA_MODEL_REG, "fpga_model_reg", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_FPGA_SUPPORT_UPGRADE, "fpga_support_upgrade", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_PMBUS_REG, "psu_pmbus_reg", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_SUPPORT_UPGRADE, "psu_support_upgrade", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_WATCHDOG_DEV, "watchdog_dev", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_BMC_SYSTEM, "bmc_system", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PRE_CHECK_BMC_SYSTEM, "pre_check_bmc_system", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CHECK_VAL_BMC_SYSTEM, "check_val_bmc_system", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_REVERSE_BMC_SYSTEM, "reverse_bmc_system", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_PSU_FRU_PMBUS, "psu_fru_pmbus", INDEX1_MAX, INDEX2_MAX)              \
    DFD_CFG_ITEM(DFD_CFG_ITEM_POWER_STATUS, "power_status", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_SLOT_POWER_STATUS, "slot_power_status", INDEX1_MAX, INDEX_NOT_EXIST)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_WITHOUT_E2_MODEL_NAME, "without_e2_model_name", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_WITHOUT_E2_SN, "without_e2_sn", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_WITHOUT_E2_PN, "without_e2_pn", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_WITHOUT_E2_HW_VER, "without_e2_hw_ver", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_WITHOUT_E2_VENDOR, "without_e2_vendor", INDEX1_MAX, INDEX2_MAX)  \
    DFD_CFG_ITEM(DFD_CFG_ITEM_MISC_FW_SUPPORT_UPGRADE, "misc_fw_support_upgrade", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_EEPROM_WP_REG, "eeprom_write_protection", INDEX1_MAX, INDEX2_MAX)          \
    DFD_CFG_ITEM(DFD_CFG_ITEM_CABLETRAY_SLOTID, "cabletray_slotid", INDEX1_MAX, INDEX_NOT_EXIST)   \
    DFD_CFG_ITEM(DFD_CFG_ITEM_INFO_CTRL_END, "end_info_ctrl", INDEX_NOT_EXIST, INDEX_NOT_EXIST)     \

/* Configuration item id enumeration definition */
#ifdef DFD_CFG_ITEM
#undef DFD_CFG_ITEM
#endif
#define DFD_CFG_ITEM(_id, _name, _index_min, _index_max)    _id,
typedef enum dfd_cfg_item_id_s {
    DFD_CFG_ITEM_ALL
} dfd_cfg_item_id_t;

#define DFD_CFG_ITEM_IS_INT(item_id) \
    (((item_id) > DFD_CFG_ITEM_NONE) && ((item_id) < DFD_CFG_ITEM_INT_END))

#define DFD_CFG_ITEM_IS_STRING(item_id) \
    (((item_id) > DFD_CFG_ITEM_INT_END) && ((item_id) < DFD_CFG_ITEM_STRING_END))

#define DFD_CFG_ITEM_IS_I2C_DEV(item_id) \
    (((item_id) > DFD_CFG_ITEM_STRING_END) && ((item_id) < DFD_CFG_ITEM_I2C_DEV_END))

#define DFD_CFG_ITEM_IS_INFO_CTRL(item_id) \
    (((item_id) > DFD_CFG_ITEM_I2C_DEV_END) && ((item_id) < DFD_CFG_ITEM_INFO_CTRL_END))

/* Index value range structure */
typedef struct index_range_s {
    int index1_max;             /* The primary index indicates the maximum value */
    int index2_max;             /* Indicates the maximum value of the secondary index */
} index_range_t;

/* Register value conversion node */
typedef struct val_convert_node_s {
    struct list_head lst;
    int int_val;                        /* Integer value */
    char str_val[DFD_CFG_STR_MAX_LEN];  /* String value */
    int index1;                         /* Index value 1 */
    int index2;                         /* Index value 2 */
} val_convert_node_t;

/**
 * dfd_ko_cfg_get_item - Get configuration item
 * @key: Node key
 *
 * @returns: The NULL configuration item does not exist, and other configuration items are successful
 */
void *dfd_ko_cfg_get_item(uint64_t key);

/**
 * dfd_ko_cfg_show_item - Display configuration items
 * @key: Node key
 */
void dfd_ko_cfg_show_item(uint64_t key);

/**
 * dfd_dev_cfg_init - Module initialization
 *
 * @returns: <0 Failed, otherwise succeeded
 */
int32_t dfd_dev_cfg_init(bool re_init);

/**
 * dfd_dev_cfg_exit - Module exit
 *
 * @returns: void
 */
void dfd_dev_cfg_exit(void);

/* Strip out Spaces and carriage returns */
void dfd_ko_cfg_del_space_lf_cr(char *str);

/* Strip tail spaces */
void dfd_ko_trim_trailing_spaces(char *str);

void dfd_ko_cfg_del_lf_cr(char *str);

/**
 * dfd_ko_cfg_get_fan_direction_by_name - obtain the air duct type by fan name
 * @fan_name: Fan name
 * @fan_direction: Duct type
 *
 * @returns: 0 Succeeded, otherwise failed
 */
int dfd_ko_cfg_get_fan_direction_by_name(char *fan_name, int *fan_direction);

/**
 * dfd_ko_cfg_get_power_type_by_name - obtain the power supply type by power supply name
 * @power_name: Power supply name
 * @power_type: Power supply type
 * @returns: 0 Succeeded, otherwise failed
 */
int dfd_ko_cfg_get_power_type_by_name(char *power_name, int *power_type);

/**
 * dfd_ko_cfg_get_slot_card_type_by_name - obtain the slot card type by slot name
 * @slot_name: slot name
 * @slot_card_type: slot card type
 *
 * @returns: 0 Succeeded, otherwise failed
 */
int dfd_ko_cfg_get_slot_card_type_by_name(char *slot_name, int *slot_card_type);

/**
 * dfd_ko_cfg_get_led_status_decode2_by_regval - Reverse check the register value of the led status
 * @regval: Defined led values
 * @index1: led type
 * @value: Gets the register value of the led status
 * @returns: 0 Succeeded, otherwise failed
 */
int dfd_ko_cfg_get_led_status_decode2_by_regval(int regval, int index1, int *value);

/**
 * dfd_ko_cfg_get_fan_direction_by_name - obtain the fan type by fan name
 * @fan_name: Fan name
 * @fan_type: Fan type
 * @sub_type: Fan sub-type
 *
 * @returns: 0 Succeeded, otherwise failed
 */
 int dfd_ko_cfg_get_fan_type_by_name(char *fan_name, int *fan_type, int *sub_type);

 /**
 * key_to_name - convert to name by key
 * @key: Fan name
 *
 * @returns: name
 */
char *key_to_name(uint64_t key);

#endif /* __DFD_CFG_H__ */
