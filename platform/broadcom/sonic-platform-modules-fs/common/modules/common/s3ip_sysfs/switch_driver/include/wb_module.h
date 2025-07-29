#ifndef _WB_MODULE_H_
#define _WB_MODULE_H_

#include "switch_driver.h"

#define mem_clear(data, size) memset((data), 0, (size))

#define DFD_SET_BYTES_KEY1(dev_index, temp_index) \
    (((dev_index & 0xff) << 8) | (temp_index & 0xff))

#define PMBUS_STATUS_WORD_SYSFS       "status_word"
#define PMBUS_STATUS_INPUT_SYSFS      "status_input"
#define PMBUS_STATUS_VOUT_SYSFS       "status_vout"
#define EEPROM_MODE_TLV_STRING          "onie-tlv"
#define EEPROM_MODE_FRU_STRING          "fru"
#define EEPROM_MODE_TX_FRU_STRING       "tx-fru"

typedef enum dfd_rv_s {
    DFD_RV_OK               = 0,
    DFD_RV_INIT_ERR         = 1,
    DFD_RV_SLOT_INVALID     = 2,
    DFD_RV_MODE_INVALID     = 3,
    DFD_RV_MODE_NOTSUPPORT  = 4,
    DFD_RV_TYPE_ERR         = 5,
    DFD_RV_DEV_NOTSUPPORT   = 6,
    DFD_RV_DEV_FAIL         = 7,
    DFD_RV_INDEX_INVALID    = 8,
    DFD_RV_NO_INTF          = 9,
    DFD_RV_NO_NODE          = 10,
    DFD_RV_NODE_FAIL        = 11,
    DFD_RV_INVALID_VALUE    = 12,
    DFD_RV_NO_MEMORY        = 13,
    DFD_RV_CHECK_FAIL       = 14,
} dfd_rv_t;

typedef enum status_mem_e {
    STATUS_ABSENT  = 0,
    STATUS_OK      = 1,
    STATUS_NOT_OK  = 2,
    STATUS_MEM_END = 3,
} status_mem_t;

/* psu PMBUS */
typedef enum psu_sensors_type_e {
    PSU_SENSOR_NONE    = 0,
    PSU_IN_VOL         = 1,
    PSU_IN_CURR        = 2,
    PSU_IN_POWER       = 3,
    PSU_OUT_VOL        = 4,
    PSU_OUT_CURR       = 5,
    PSU_OUT_POWER      = 6,
    PSU_FAN_SPEED      = 7,
    PSU_OUT_MAX_POWERE = 8,
    PSU_OUT_STATUS     = 9,
    PSU_IN_STATUS      = 10,
    PSU_IN_TYPE        = 11,
    PSU_FAN_RATIO      = 12,
    PSU_IN_VOL_MAX     = 13,
    PSU_IN_CURR_MAX    = 14,
    PSU_IN_VOL_MIN     = 15,
    PSU_IN_CURR_MIN    = 16,
    PSU_OUT_VOL_MAX     = 17,
    PSU_OUT_CURR_MAX    = 18,
    PSU_OUT_VOL_MIN     = 19,
    PSU_OUT_CURR_MIN    = 20,
    PSU_FAN_SPEED_MAX   = 21,
    PSU_FAN_SPEED_MIN   = 22,
    PSU_IN_POWER_MAX    = 23,
    PSU_IN_POWER_MIN    = 24,
    PSU_OUT_POWER_MAX   = 25,
    PSU_OUT_POWER_MIN   = 26,
    PSU_IN_VOL_HIGH     = 27,
    PSU_IN_VOL_LOW      = 28,
    PSU_IN_CURR_HIGH    = 29,
    PSU_IN_CURR_LOW     = 30,
    PSU_IN_POWER_HIGH   = 31,
    PSU_IN_POWER_LOW    = 32,
    PSU_OUT_VOL_HIGH    = 33,
    PSU_OUT_VOL_LOW     = 34,
    PSU_OUT_CURR_HIGH   = 35,
    PSU_OUT_CURR_LOW    = 36,
    PSU_OUT_POWER_HIGH  = 37,
    PSU_OUT_POWER_LOW   = 38,
} psu_sensors_type_t;

/* Watchdog type */
typedef enum wb_wdt_type_e {
    WB_WDT_TYPE_NAME         = 0,     /* watchdog identify */
    WB_WDT_TYPE_STATE        = 1,     /* watchdog state */
    WB_WDT_TYPE_TIMELEFT     = 2,     /* watchdog timeleft */
    WB_WDT_TYPE_TIMEOUT      = 3,     /* watchdog timeout */
    WB_WDT_TYPE_ENABLE       = 4,     /* watchdog enable */
} wb_wdt_type_t;

/* Port Power Status */
typedef enum wb_port_power_status_e {
    WB_PORT_POWER_OFF        = 0,     /* port power off */
    WB_PORT_POWER_ON         = 1,     /* port power on */
} wb_port_power_status_t;

/* slot Power Status */
typedef enum wb_slot_power_status_e {
    WB_SLOT_POWER_OFF        = 0,     /* slot power off */
    WB_SLOT_POWER_ON         = 1,     /* slot power on */
} wb_slot_power_status_t;

/* sensor monitor or not */
typedef enum wb_sensor_monitor_flag_e {
    WB_SENSOR_MONITOR_NO    = 0,     /* sensor not monitor  */
    WB_SENSOR_MONITOR_YES   = 1,     /* sensor monitor  */
} wb_sensor_monitor_flag_t;

typedef enum dfd_dev_info_type_e {
    DFD_DEV_INFO_TYPE_MAC               = 1,
    DFD_DEV_INFO_TYPE_NAME              = 2,
    DFD_DEV_INFO_TYPE_SN                = 3,
    DFD_DEV_INFO_TYPE_PWR_CONS          = 4,
    DFD_DEV_INFO_TYPE_HW_INFO           = 5,
    DFD_DEV_INFO_TYPE_DEV_TYPE          = 6,
    DFD_DEV_INFO_TYPE_PART_NAME         = 7,
    DFD_DEV_INFO_TYPE_PART_NUMBER       = 8,  /* part number */
    DFD_DEV_INFO_TYPE_FAN_DIRECTION     = 9,
    DFD_DEV_INFO_TYPE_MAX_OUTPUT_POWRER = 10, /* max_output_power */
    DFD_DEV_INFO_TYPE_SPEED_CAL = 11,
    DFD_DEV_INFO_TYPE_ASSET_TAG = 12,
    DFD_DEV_INFO_TYPE_VENDOR    = 13,
    DFD_DEV_INFO_TYPE_EXTRA1    = 14,
    DFD_DEV_INFO_TYPE_EXTRA2    = 15,
    DFD_DEV_INFO_TYPE_EXTRA3    = 16,
    DFD_DEV_INFO_TYPE_EXTRA4    = 17,
} dfd_dev_tlv_type_t;

/* Master device type */
typedef enum wb_main_dev_type_e {
    WB_MAIN_DEV_MAINBOARD = 0,      /* Main board */
    WB_MAIN_DEV_FAN       = 1,      /* FAN */
    WB_MAIN_DEV_PSU       = 2,      /* PSU */
    WB_MAIN_DEV_SFF       = 3,      /* Optical module */
    WB_MAIN_DEV_CPLD      = 4,      /* CPLD */
    WB_MAIN_DEV_SLOT      = 5,      /* Sub card */
    WB_MAIN_DEV_CABLETRAY = 6,      /* CABLETRAY */
    WB_MAIN_DEV_CHASSIS   = 7,      /* CHASSIS */
} wb_main_dev_type_t;

/* Subdevice type */
typedef enum wb_minor_dev_type_e {
    WB_MINOR_DEV_NONE       = 0,    /* None                             */
    WB_MINOR_DEV_TEMP       = 1,    /* Temperature                      */
    WB_MINOR_DEV_IN         = 2,    /* Voltage                          */
    WB_MINOR_DEV_CURR       = 3,    /* Current                          */
    WB_MINOR_DEV_POWER      = 4,    /* Power                            */
    WB_MINOR_DEV_MOTOR      = 5,    /* Motor                            */
    WB_MINOR_DEV_PSU        = 6,    /* Power supply type                */
    WB_MINOR_DEV_FAN        = 7,    /* Fan model                        */
    WB_MINOR_DEV_CPLD       = 8,    /* CPLD                             */
    WB_MINOR_DEV_FPGA       = 9,    /* FPGA                             */
    WB_MINOR_DEV_EEPROM     = 10,   /* EEPROM                           */
    WB_MINOR_DEV_MY_SLOT_ID = 11,   /* Slot ID in chassis               */
    WB_MINOR_DEV_MISC_FW   = 12,   /* OTHER DEV FW                     */
} wb_minor_dev_type_t;

/* SENSORS attribute type */
typedef enum wb_sensor_type_e {
    WB_SENSOR_INPUT       = 0,     /* Sensor value */
    WB_SENSOR_ALIAS       = 1,     /* Sensor nickname */
    WB_SENSOR_TYPE        = 2,     /* Sensor type*/
    WB_SENSOR_MAX         = 3,     /* Sensor maximum */
    WB_SENSOR_MAX_HYST    = 4,     /* Sensor hysteresis value */
    WB_SENSOR_MIN         = 5,     /* Sensor minimum */
    WB_SENSOR_CRIT        = 6,     /* Sensor crit value */
    WB_SENSOR_RANGE       = 7,     /* Sensor error value */
    WB_SENSOR_NOMINAL_VAL = 8,     /* Nominal value of the sensor */
    WB_SENSOR_HIGH        = 9,     /* Sensor height */
    WB_SENSOR_LOW         = 10,    /* Sensor low */
} wb_sensor_type_t;

/* sff cpld attribute type */
typedef enum wb_sff_cpld_attr_e {
    WB_SFF_POWER_ON      = 0x01,
    WB_SFF_TX_FAULT,
    WB_SFF_TX_DIS,
    WB_SFF_PRESENT_RESERVED,
    WB_SFF_RX_LOS,
    WB_SFF_RESET,
    WB_SFF_LPMODE,
    WB_SFF_MODULE_PRESENT,
    WB_SFF_INTERRUPT,
} wb_sff_cpld_attr_t;

/* LED attribute type */
typedef enum wb_led_e {
    WB_SYS_LED_FRONT   = 0,      /* Front panel SYS light */
    WB_SYS_LED_REAR    = 1,      /* SYS light on rear panel */
    WB_BMC_LED         = 2,      /* BMC indicator on the front panel */
    WB_BMC_LED_REAR    = 3,      /* BMC indicator on the rear panel */
    WB_FAN_LED_FRONT   = 4,      /* Front panel fan light */
    WB_FAN_LED_REAR    = 5,      /* Rear panel fan light */
    WB_PSU_LED_FRONT   = 6,      /* Front panel power light */
    WB_PSU_LED_REAR    = 7,      /* Rear panel power light */
    WB_ID_LED_FRONT    = 8,      /* Front panel positioning light */
    WB_ID_LED_REAR     = 9,      /* Rear panel positioning light */
    WB_FAN_LED_MODULE  = 10,     /* Fan module indicator */
    WB_PSU_LED_MODULE  = 11,     /* Power module indicator */
    WB_SLOT_LED_MODULE = 12,     /* Sub-card status indicator */
    WB_PORT_LED        = 13,     /* port statu led */
    BMC_HOST_SYS_LED   = 14,     /* BMC light led when HOST feed dog timeout */
} wb_led_t;

typedef enum eeprom_mode_e {
    EEPROM_MODE_TLV,     /* TLV */
    EEPROM_MODE_FRU,      /*FRU*/
} eeprom_mode_t;

typedef ssize_t (*dfd_sysfs_get_data_func)(unsigned int index1,
                                     unsigned int index2,
                                     char *buf,
                                     size_t count);

typedef int (*dfd_sysfs_set_data_func)(unsigned int index1,
                                     unsigned int index2,
                                     void *value,
                                     unsigned int len);

/* cpld func table */
typedef enum {
    DFD_CPLD_NAME_E,
    DFD_CPLD_TYPE_E,
    DFD_CPLD_VENDOR_E,
    DFD_CPLD_FW_VERSION_E,
    DFD_CPLD_HW_VERSION_E,
    DFD_CPLD_SUPPORT_UPGRADE_E,
    DFD_CPLD_UPGRADE_ACTIVE_TYPE_E,
    DFD_CPLD_REG_TEST_TYPE_E,
    DFD_CPLD_MAX_E
} cpld_device_type;

/* fpga func table */
typedef enum {
    DFD_FPGA_NAME_E,
    DFD_FPGA_TYPE_E,
    DFD_FPGA_VENDOR_E,
    DFD_FPGA_FW_VERSION_E,
    DFD_FPGA_HW_VERSION_E,
    DFD_FPGA_SUPPORT_UPGRADE_E,
    DFD_FPGA_UPGRADE_ACTIVE_TYPE_E,
    DFD_FPGA_REG_TEST_TYPE_E,
    DFD_FPGA_MAX_E
} fpga_device_type;

/* misc_fw func table */
typedef enum {
    DFD_MISC_FW_NAME_E,
    DFD_MISC_FW_TYPE_E,
    DFD_MISC_FW_VENDOR_E,
    DFD_MISC_FW_FW_VERSION_E,
    DFD_MISC_FW_HW_VERSION_E,
    DFD_MISC_FW_SUPPORT_UPGRADE_E,
    DFD_MISC_FW_UPGRADE_ACTIVE_TYPE_E,
    DFD_MISC_FW_MAX_E
} misc_fw_device_type;

/* eeprom table */
typedef enum {
    DFD_EEPROM_ALIAS_E,
    DFD_EEPROM_TAG_E,
    DFD_EEPROM_TYPE_E,
    DFD_EEPROM_WRITE_PROTECTION_E,
    DFD_EEPROM_I2C_BSU_E,
    DFD_EEPROM_I2C_ADDR_E,
    DFD_EEPROM_MAX_E
} eeprom_device_type;

typedef struct dfd_sysfs_func_map {
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_set_data_func set_func;
}dfd_sysfs_func_map_t;

extern int g_dfd_dbg_level;
extern int g_dfd_fan_dbg_level;
extern int g_dfd_fru_dbg_level;
extern int g_dfd_wb_tlv_dbg_level;
extern int g_dfd_eeprom_dbg_level;
extern int g_dfd_cpld_dbg_level;
extern int g_dfd_fpga_dbg_level;
extern int g_dfd_sysled_dbg_level;
extern int g_dfd_slot_dbg_level;
extern int g_dfd_sensor_dbg_level;
extern int g_dfd_psu_dbg_level;
extern int g_dfd_sff_dbg_level;
extern int g_dfd_watchdog_dbg_level;
extern int g_dfd_custom_dbg_level;
extern int g_dfd_cabletray_dbg_level;
extern int g_dfd_misc_fw_dbg_level;

#define WB_MIN(a, b)   ((a) < (b) ? (a) : (b))
#define WB_MAX(a, b)   ((a) > (b) ? (a) : (b))
/* method of opening debug interface: echo 0xff > /sys/module/s3ip_switch_driver/parameters/g_dfd_dbg_level */
#define DBG_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_FAN_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_fan_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_FRU_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_fru_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_EEPROM_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_eeprom_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_CPLD_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_cpld_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_FPGA_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_fpga_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_SYSLED_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_sysled_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_SLOT_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_slot_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_SENSOR_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_sensor_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_PSU_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_psu_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_SFF_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_sff_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_WDT_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_watchdog_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_SYSTEM_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_custom_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_WBTLV_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_wb_tlv_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_CABLETRAY_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_slot_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_MISC_FW_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_misc_fw_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

/**
 * wb_dev_cfg_init - dfd module initialization
 *
 * @returns: <0 Failed, otherwise succeeded
 */
int32_t wb_dev_cfg_init(void);

/**
 * wb_dev_cfg_exit - dfd module exit
 *
 * @returns: void
 */

void wb_dev_cfg_exit(void);

/**
 * dfd_get_dev_number - Get the number of devices
 * @main_dev_id:Master device number
 * @minor_dev_id:Secondary device number
 * @returns: <0 failed, otherwise number of devices is returned
 */
int dfd_get_dev_number(unsigned int main_dev_id, unsigned int minor_dev_id);

/**
 * dfd_get_reg_key - Get the value of reg key
 * @main_dev_id:Master device number
 * @minor_dev_id:Secondary device number
 * @key_index2:key index
 * @key_value:return get key value
 * @returns: <0 failed
 */
int dfd_get_reg_key(unsigned int main_dev_id, unsigned int minor_dev_id, 
    int key_index2, int *key_value);

/**
 * dfd_get_sysfs_decode - Map to the value of sysfs value
 * @main_dev_id:Master device number
 * @minor_dev_id:Secondary device number
 * @value:need to convert
 * @sysfs_value:sysfs value
 * @returns: <0 failed
 */
int dfd_get_sysfs_decode(unsigned int main_dev_id, unsigned int minor_dev_id, 
    int value, int *sysfs_value);

dfd_sysfs_get_data_func dfd_get_sysfs_value_func(dfd_sysfs_func_map_t *func_map, unsigned int type, unsigned int max_len);
dfd_sysfs_set_data_func dfd_set_sysfs_value_func(dfd_sysfs_func_map_t *func_map, unsigned int type, unsigned int max_len);

#endif  /* _WB_MODULE_H_ */
