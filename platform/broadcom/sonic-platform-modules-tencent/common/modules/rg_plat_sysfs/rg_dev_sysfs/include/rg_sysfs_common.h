#ifndef _RG_SYSFS_COMMON_H_
#define _RG_SYSFS_COMMON_H_

#define RG_GET_SFF_SLOT(port)           (((port) >> 24) & 0xff)
#define RG_GET_SFF_INDEX(port)          (((port) >> 16) & 0xff)
#define RG_GET_SFF_I2C_ADAPTER(port)    (((port) >> 8) & 0xff)
#define RG_GET_SFF_SPEED_MODE(port)     ((port) & 0xff)

#define TLV_CODE_PRODUCT_NAME   (0x21)
#define TLV_CODE_PART_NUMBER    (0x22)
#define TLV_CODE_SERIAL_NUMBER  (0x23)
#define TLV_CODE_MAC_BASE       (0x24)
#define TLV_CODE_MANUF_DATE     (0x25)
#define TLV_CODE_DEVICE_VERSION (0x26)
#define TLV_CODE_LABEL_REVISION (0x27)
#define TLV_CODE_PLATFORM_NAME  (0x28)
#define TLV_CODE_ONIE_VERSION   (0x29)
#define TLV_CODE_MAC_SIZE       (0x2A)
#define TLV_CODE_MANUF_NAME     (0x2B)
#define TLV_CODE_MANUF_COUNTRY  (0x2C)
#define TLV_CODE_VENDOR_NAME    (0x2D)
#define TLV_CODE_DIAG_VERSION   (0x2E)
#define TLV_CODE_SERVICE_TAG    (0x2F)
#define TLV_CODE_VENDOR_EXT     (0xFD)
#define TLV_CODE_CRC_32         (0xFE)

#define DIR_NAME_MAX_LEN        (64)

#define RG_SYSFS_DEV_ERROR         "no_support"
/* sysfs directory name */
#define RG_FAN_SYSFS_NAME          "fan"
#define RG_PSU_SYSFS_NAME          "psu"
#define RG_SLOT_SYSFS_NAME         "slot"
#define RG_CPLD_SYSFS_NAME         "cpld"
#define RG_VOLTAGE_SYSFS_NAME      "in"
#define RG_TEMP_SYSFS_NAME         "temp"
#define RG_SFF_SYSFS_NAME          "sff"
#define RG_SFF_25GE_SYSFS_NAME     "Eth25GE"
#define RG_SFF_100GE_SYSFS_NAME    "Eth100GE"

typedef enum dfd_dev_info_type_e {
    DFD_DEV_INFO_TYPE_MAC         = 1,
    DFD_DEV_INFO_TYPE_NAME        = 2,
    DFD_DEV_INFO_TYPE_SN          = 3,
    DFD_DEV_INFO_TYPE_PWR_CONS    = 4,
    DFD_DEV_INFO_TYPE_HW_INFO     = 5,
    DFD_DEV_INFO_TYPE_DEV_TYPE    = 6,
    DFD_DEV_INFO_TYPE_PART_NAME   = 7,
    DFD_DEV_INFO_TYPE_PART_NUMBER = 8,  /* part number */
} dfd_dev_tlv_type_t;

typedef enum rg_led_e {
    RG_SYS_LED_FRONT   = 0,
    RG_SYS_LED_REAR    = 1,
    RG_BMC_LED_FRONT   = 2,
    RG_BMC_LED_REAR    = 3,
    RG_FAN_LED_FRONT   = 4,
    RG_FAN_LED_REAR    = 5,
    RG_PSU_LED_FRONT   = 6,
    RG_PSU_LED_REAR    = 7,
    RG_ID_LED_FRONT    = 8,
    RG_ID_LED_REAR     = 9,
    RG_FAN_LED_MODULE  = 10,
    RG_PSU_LED_MODULE  = 11,
    RG_SLOT_LED_MODULE = 12,
} rg_led_t;

typedef enum rg_main_dev_type_e {
    RG_MAIN_DEV_MAINBOARD = 0,
    RG_MAIN_DEV_FAN       = 1,
    RG_MAIN_DEV_PSU       = 2,
    RG_MAIN_DEV_SFF       = 3,
    RG_MAIN_DEV_CPLD      = 4,      /* CPLD */
    RG_MAIN_DEV_SLOT      = 5,
} rg_main_dev_type_t;

typedef enum rg_minor_dev_type_e {
    RG_MINOR_DEV_NONE  = 0,    /* None */
    RG_MINOR_DEV_TEMP  = 1,
    RG_MINOR_DEV_IN    = 2,
    RG_MINOR_DEV_CURR  = 3,
    RG_MINOR_DEV_POWER = 4,
    RG_MINOR_DEV_MOTOR = 5,
    RG_MINOR_DEV_PSU   = 6,
} rg_minor_dev_type_t;

typedef enum rg_sensor_type_e {
    RG_SENSOR_INPUT    = 0,
    RG_SENSOR_ALIAS    = 1,
    RG_SENSOR_TYPE     = 2,
    RG_SENSOR_MAX      = 3,
    RG_SENSOR_MAX_HYST = 4,
    RG_SENSOR_MIN      = 5,
    RG_SENSOR_CRIT     = 6,
} rg_sensor_type_t;

typedef enum rg_sff_speed_type_e {
    RG_SFF_SPEED_NONE  = 0,    /* None */
    RG_SFF_SPEED_25GE  = 1,
    RG_SFF_SPEED_100GE = 2,
    RG_SFF_SPEED_50GE  = 3,
} rg_sff_speed_type_t;

typedef enum rg_sff_cpld_attr_e {
    RG_SFF_POWER_ON      = 0x01,
    RG_SFF_TX_FAULT,
    RG_SFF_TX_DIS,
    RG_SFF_PRE_N,
    RG_SFF_RX_LOS,
    RG_SFF_RESET,
    RG_SFF_LPMODE,
    RG_SFF_MODULE_PRESENT,
    RG_SFF_INTERRUPT,
} rg_sff_cpld_attr_t;

typedef enum rg_sff_eeprom_attr_e {
    RG_SFF_TYPE           = 0x01,
    RG_SFF_HW_VERSION,
    RG_SFF_SERIAL_NUM,
    RG_SFF_MANUFACTURE_NAME,
    RG_SFF_MODEL_NAME,
    RG_SFF_CONNECTOR,
    RG_SFF_ENCODING,
    RG_SFF_EXT_IDENTIFIER,
    RG_SFF_EXT_RATESELECT_COMPLIANCE,
    RG_SFF_CABLE_LENGTH,
    RG_SFF_NOMINAL_BIT_RATE,
    RG_SFF_SEPECIFICATION_COMPLIANCE,
    RG_SFF_VENDOR_DATE,
    RG_SFF_VENDOR_OUI,
    RG_SFF_TEMPERATURE,
    RG_SFF_VOLTAGE,
    RG_SFF_RXPOWER,
    RG_SFF_TXBIAS,
    RG_SFF_TXPOWER,
} rg_sff_eeprom_attr_t;

struct switch_drivers_t{
    /* device */
    int (*get_dev_number) (unsigned int main_dev_id, unsigned int minor_dev_id);
    /* fan */
    int (*get_fan_number) (void);
    int (*get_fan_status) (unsigned int fan_index);
    ssize_t (*get_fan_info) (unsigned int fan_index, uint8_t cmd, char* buf);
    ssize_t (*get_fan_speed) (unsigned int fan_index, unsigned int motor_index, unsigned int *speed);
    int (*get_fan_pwm) (unsigned int fan_index, unsigned int motor_index, int *pwm);
    int (*set_fan_pwm) (unsigned int fan_index, unsigned int motor_index, int pwm);
    int (*get_fan_speed_tolerance) (unsigned int fan_index, unsigned int motor_index, int *value);
    int (*get_fan_speed_target) (unsigned int fan_index, unsigned int motor_index, int *value);
    int (*get_fan_direction) (unsigned int fan_index, unsigned int motor_index, int *value);
    ssize_t (*get_fan_status_str) (unsigned int fan_index, char* buf);
    ssize_t (*get_fan_direction_str) (unsigned int fan_index, unsigned int motor_index, char* buf);
    int (*get_fan_present_status)(unsigned int fan_index);
    int (*get_fan_roll_status)(unsigned int fan_index, unsigned int motor_index);
    int (*get_fan_speed_level)(unsigned int fan_index, unsigned int motor_index, int *level);
    int (*set_fan_speed_level)(unsigned int fan_index, unsigned int motor_index, int level);
    /* syseeprom */
    ssize_t (*get_syseeprom_info) (uint8_t cmd, char* buf);
    /* cpld */
    ssize_t (*get_cpld_name) (unsigned int cpld_index, char* buf);
    ssize_t (*get_cpld_type) (unsigned int cpld_index, char* buf);
    ssize_t (*get_cpld_version) (unsigned int cpld_index, char* buf);
    int (*get_cpld_testreg) (unsigned int cpld_index, int *value);
    int (*set_cpld_testreg) (unsigned int cpld_index, int value);
    /* led */
    ssize_t (*get_led_status) (uint16_t led_id, uint8_t led_index, char *buf);
    /* slot */
    ssize_t (*get_slot_status_str) (unsigned int slot_index, char* buf);
    ssize_t (*get_slot_info) (unsigned int slot_index, uint8_t cmd, char *buf);
    /* sensors */
    ssize_t (*get_temp_info)( uint8_t main_dev_id, uint8_t dev_index,
            uint8_t temp_index, uint8_t temp_attr, char *buf);
    ssize_t (*get_voltage_info)( uint8_t main_dev_id, uint8_t dev_index,
            uint8_t in_index, uint8_t in_attr, char *buf);
    /* psu */
    ssize_t (*get_psu_info)( unsigned int psu_index, uint8_t cmd, char *buf);
    ssize_t (*get_psu_status_str) (unsigned int psu_index, char *buf);
    ssize_t (*get_psu_sensor_info)( uint8_t psu_index, uint8_t sensor_type, char *buf);
    int (*get_psu_present_status)(unsigned int psu_index);
    int (*get_psu_output_status)(unsigned int psu_index);
    int (*get_psu_alert_status)(unsigned int psu_index);
    /* sff */
    int (*get_sff_id) (unsigned int sff_index);
    int (*set_sff_cpld_info)(unsigned int sff_index, int cpld_reg_type, int value);
    ssize_t (*get_sff_cpld_info)( unsigned int sff_index, int cpld_reg_type, char *buf, int len);
    ssize_t (*get_sff_eeprom_info)(unsigned int sff_index, const char *attr_name, char *buf, int buf_len);
    ssize_t (*get_sff_dir_name)(unsigned int sff_index, char *buf, int buf_len);
    int (*get_sff_polling_size) (void);
    ssize_t (*get_sff_polling_data)(unsigned int sff_index, char *buf, loff_t offset, size_t count);
};

extern struct switch_drivers_t * dfd_plat_driver_get(void);

#endif /*_RG_SYSFS_COMMON_H_ */
