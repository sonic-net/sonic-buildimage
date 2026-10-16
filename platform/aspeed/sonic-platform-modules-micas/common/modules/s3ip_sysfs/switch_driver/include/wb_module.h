#ifndef _WB_MODULE_H_
#define _WB_MODULE_H_

#include "switch_driver.h"
#include "dfd_cfg.h"

#define mem_clear(data, size) memset((data), 0, (size))

#define DFD_SET_BYTES_KEY1(dev_index, temp_index) \
    (((dev_index & 0xff) << 8) | (temp_index & 0xff))

#define PMBUS_STATUS_WORD_SYSFS       "status_word"
#define PMBUS_STATUS_INPUT_SYSFS      "status_input"
#define PMBUS_STATUS_VOUT_SYSFS       "status_vout"
#define EEPROM_MODE_TLV_STRING          "onie-tlv"
#define EEPROM_MODE_FRU_STRING          "fru"
#define EEPROM_MODE_SYSFRU_T_STRING       "sysfru-t"
#define EEPROM_MODE_SYSFRU_A_STRING       "sysfru-a"

typedef enum dfd_psu_status_e {
    DFD_PSU_PRESENT_STATUS  = 0,
    DFD_PSU_OUTPUT_STATUS   = 1,
    DFD_PSU_ALERT_STATUS    = 2,
    DFD_PSU_INPUT_STATUS    = 3,
    DFD_PSU_STATUS_END,
} dfd_psu_status_t;

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
    DFD_RV_DEBUG_MODE_OFF   = 15,
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
    PSU_IN_VOL_NOTICE_HIGH      = 39,
    PSU_IN_VOL_NOTICE_LOW       = 40,
    PSU_IN_CURR_NOTICE_HIGH     = 41,
    PSU_IN_CURR_NOTICE_LOW      = 42,
    PSU_IN_POWER_NOTICE_HIGH    = 43,
    PSU_IN_POWER_NOTICE_LOW     = 44,
    PSU_OUT_VOL_NOTICE_HIGH     = 45,
    PSU_OUT_VOL_NOTICE_LOW      = 46,
    PSU_OUT_CURR_NOTICE_HIGH    = 47,
    PSU_OUT_CURR_NOTICE_LOW     = 48,
    PSU_OUT_POWER_NOTICE_HIGH   = 49,
    PSU_OUT_POWER_NOTICE_LOW    = 50,
    PSU_IN_VOL_THRESHOLD_CNT    = 51,
    PSU_IN_CURR_THRESHOLD_CNT   = 52,
    PSU_IN_POWER_THRESHOLD_CNT  = 53,
    PSU_OUT_VOL_THRESHOLD_CNT   = 54,
    PSU_OUT_CURR_THRESHOLD_CNT  = 55,
    PSU_OUT_POWER_THRESHOLD_CNT = 56,
    PSU_FAN_SPEED_THRESHOLD_CNT = 57,
    PSU_SENSOR_TYPE_END,
} psu_sensors_type_t;

/* Watchdog type */
typedef enum wb_wdt_type_e {
    WB_WDT_TYPE_NAME         = 0,     /* watchdog identify */
    WB_WDT_TYPE_STATE        = 1,     /* watchdog state */
    WB_WDT_TYPE_TIMELEFT     = 2,     /* watchdog timeleft */
    WB_WDT_TYPE_TIMEOUT      = 3,     /* watchdog timeout */
    WB_WDT_TYPE_ENABLE       = 4,     /* watchdog enable */
    WB_WDT_TYPE_RESET        = 5,     /* watchdog reset */
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

/* Sff page type */
typedef enum wb_sff_e2_page_e {
    WB_SFF_E2_PAGE_LOW       = 0,     /* sff e2 page low */
    WB_SFF_E2_PAGE_HIGH      = 1,     /* sff e2 page high */
} wb_sff_e2_page_t;

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
    DFD_DEV_INFO_TYPE_EXTRA5    = 18,
    DFD_DEV_INFO_TYPE_EXTRA6    = 19,
    DFD_DEV_INFO_TYPE_EXTRA7    = 20,
    DFD_DEV_INFO_I2C_BUS        = 21,
    DFD_DEV_INFO_I2C_ADDR       = 22,
    DFD_DEV_INFO_I2C_PMBUS_BUS  = 23,
    DFD_DEV_INFO_I2C_PMBUS_ADDR = 24,
    DFD_DEV_INFO_PSU_FW_VER     = 25,
    DFD_DEV_INFO_PSU_DFX_INFO     = 26,
    DFD_DEV_INFO_PSU_MATCH_CHECK     = 100,
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
    WB_MAIN_DEV_BMC       = 8,      /* BMC */
    WB_MAIN_DEV_DISK      = 9,      /* DISK */
    WB_MAIN_DEV_MD        = 10,     /* Multiple Device RAID */
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
    WB_MINOR_DEV_MISC_FW   = 12,    /* OTHER DEV FW                     */
    WB_MAIN_DEV_LEAK_DETECTOR = 13, /* LEAK DETECTOR */
    WB_MAIN_DEV_SOL         = 14,   /* SOL */
    WB_MAIN_DEV_GCU         = 15,   /* GCU */
    WB_MAIN_DEV_NVME        = 16,   /* NVMe                             */
    WB_MAIN_DEV_LSW         = 17,   /* LSW                              */
    WB_MAIN_DEV_PCIE        = 18,   /* PCIe                             */
    WB_MAIN_DEV_CLOCK       = 19,   /* CLOCK                            */
    WB_MAIN_DEV_PLL         = 20,   /* PLL                              */
    WB_MAIN_DEV_AVS         = 21,   /* AVS                              */
    WB_MINOR_DEV_DPU        = 22,   /* DPU                              */
} wb_minor_dev_type_t;

typedef enum wb_gcu_main_dev_type_e {
    WB_MAIN_GCU_DEV_ENFLAME_L600    = 0,
    WB_MAIN_GCU_DEV_ENFLAME_L900    = 1,
    WB_MAIN_GCU_DEV_MAX             = 2,
} wb_gcu_main_dev_type_t;

typedef enum wb_nvme_main_dev_type_e {
    WB_MAIN_NVME_DEV_INTEL_D7_P5520_P5620   = 0,
    WB_MIAN_NVME_TYPE_MAX                   = 1,
} wb_nvme_main_dev_type_t;

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
    WB_SENSOR_NOTICE_HIGH = 11,    /* Sensor notice height */
    WB_SENSOR_NOTICE_LOW  = 12,    /* Sensor notice low */
    WB_SENSOR_THRESHOLD_CNT = 13,    /* Sensor threshold cnt */
    WB_SENSOR_FUNC        = 15,    /* Sensor function index */
    WB_SENSOR_END,
} wb_sensor_type_t;

/* sff cpld attribute type */
typedef enum wb_sff_attr_e {
    WB_SFF_POWER_ON              = 1,
    WB_SFF_TX_FAULT              = 2,
    WB_SFF_TX_DIS                = 3,
    WB_SFF_PRESENT_RESERVED      = 4,
    WB_SFF_RX_LOS                = 5,
    WB_SFF_RESET                 = 6,
    WB_SFF_LPMODE                = 7,
    WB_SFF_MODULE_PRESENT        = 8,
    WB_SFF_INTERRUPT             = 9,
    WB_SFF_DIAGNOSTIC            = 10,
    WB_SFF_TX_LOS                = 11,
    WB_SFF_TX_CDR_LOL            = 12,
    WB_SFF_RX_CDR_LOL            = 13,
    WB_SFF_MODULE_STATUS         = 14,
    WB_SFF_DATAPATH_STATUS       = 15,
    WB_SFF_HOST_SNR              = 16,
    WB_SFF_MEDIA_SNR             = 17,
    WB_SFF_TEMP                  = 18,
    WB_SFF_VOLTAGE               = 19,
    WB_SFF_TX_BIAS               = 20,
    WB_SFF_TX_POWER              = 21,
    WB_SFF_RX_POWER              = 22,
    WB_SFF_TX_FLAG_SUP           = 23,
    WB_SFF_RX_FLAG_SUP           = 24,
    WB_SFF_SNR_SUP               = 25,
    WB_SFF_LANE_SPECIFIC         = 26,
    WB_SFF_SNR_SELECT            = 27,
    WB_SFF_ATTR_END,
} wb_sff_attr_t;

/* LED attribute type, Note: The value cannot exceed 255 */
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
    WB_MEM_LED         = 15,     /* Memory fault indicator */
    WB_SYS_HOT_LED     = 16,     /* System hot indicator */
    WB_OCP_LAN_LED     = 17,     /* Network status indicator */
    WB_BMC_SYS_LED     = 100,    /* sysled status from BMC */
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
    DFD_CPLD_SELTEST_STATUS_E,
    DFD_CPLD_BOOT_VIEW_E,
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
    DFD_FPGA_SELTEST_STATUS_E,
    DFD_FPGA_CRAM_STATUS_E,
    DFD_FPGA_BOOT_VIEW_E,
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

/* sol func table */
typedef enum {
    DFD_SOL_NAME_E,
    DFD_SOL_DEVICE_E,
    DFD_SOL_ACTIVE_E,
    DFD_SOL_MAX_E
} sol_device_type;

/* leak func table */
typedef enum {
    DFD_LEAK_NAME_E,
    DFD_LEAK_STATUS_E,
    DFD_LEAK_SIMULATE_STATUS_E,
    DFD_LEAK_PRESENT_E,
    DFD_LEAK_MAX_E
} leak_device_type;

/* cabletray func table */
typedef enum {
    DFD_CABLETRAY_ALIAS_E,
    DFD_CABLETRAY_NAME_E,
    DFD_CABLETRAY_VENDOR_E,
    DFD_CABLETRAY_SN_E,
    DFD_CABLETRAY_PN_E,
    DFD_CABLETRAY_VERSION_E,
    DFD_CABLETRAY_SLOTID_E,
    DFD_CABLETRAY_RACK_SN_E,
    DFD_CABLETRAY_UID_E,
    DFD_CABLETRAY_H_LOCATION_E,
    DFD_CABLETRAY_V_LOCATION_E,
    DFD_CABLETRAY_MAX_E
} cabletray_device_type;

/* gcu func table */
typedef enum {
    DFD_GCU_BUS_NUM,                    /* GPU i2c bus number */
    DFD_GCU_ALIAS_E,                    /* GPU name */
    DFD_GCU_DRIVER_VERSION_E,           /* Driver version information string */
    DFD_GCU_FIRMWARE_VERSION_E,         /* Firmware version information string */
    DFD_GCU_PCIE_VENDOR_ID_E,           /* PCIe Vendor ID in hexadecimal */
    DFD_GCU_PCIE_DEVICE_ID_E,           /* PCIe Device ID in hexadecimal */
    DFD_GCU_PCIE_SUB_VENDOR_ID_E,       /* PCIe Subsystem Vendor ID in hexadecimal */
    DFD_GCU_PCIE_SUB_DEVICE_ID_E,       /* PCIe Subsystem Device ID in hexadecimal */
    DFD_GCU_PRODUCT_TYPE_E,             /* Product type/model identifier string */
    DFD_GCU_PART_NUMBER_E,              /* Manufacturer part number string */
    DFD_GCU_MANUFACTURER_DATE_E,        /* Manufacturing date string (YYYY-MM-DD) */
    DFD_GCU_SERIAL_NUMBER_E,            /* Device serial number string */
    DFD_GCU_HBM_CAPACITY_E,             /* HBM memory capacity in gigabytes */
    DFD_GCU_MFR_NAME_E,                 /* Manufacturer name string */
    DFD_GCU_FREQUENT_MAX_E,             /* Maximum GPU clock frequency in MHz */
    DFD_GCU_ECC_STATUS_E,               /* ECC memory status (0-disabled, 1-enabled) */
    DFD_GCU_BOARD_TYPE_E,               /* Board type/revision identifier */
    DFD_GCU_PCIE_RATE_MAX_E,            /* Maximum supported PCIe rate in GT/s */
    DFD_GCU_PCIE_RATE_CURRENT_E,        /* Current PCIe rate in GT/s */
    DFD_GCU_PCIE_LINK_WIDTH_MAX_E,      /* Maximum PCIe link width */
    DFD_GCU_PCIE_LINK_WIDTH_CURRENT_E,  /* Current PCIe link width */
    DFD_GCU_OPTICAL_MODULE_PESENT_E,    /* Presence status of the optical module */
    DFD_GCU_OPTICAL_MODULE_TEMP_E,      /* Temperature of the optical module */
    DFD_GCU_PERIPHERAL_ERROR_E,         /* Peripheral device error status */
    DFD_GCU_PCIE_ERROR_E,               /* PCIe error status */
    DFD_GCU_HBM_ERROR_E,                /* HBM (High Bandwidth Memory) error status */
    DFD_GCU_HEALTH_STATUS_E,            /* PCIE error status */
    DFD_GCU_ID_DETECT_E,                /* Detect the ID of the device */
    DFD_GCU_ID_MONITOR_FLAG_E,          /* Monitor flag for the ID of the device */
    DFD_GCU_TEMP_NUM_E,                 /* Number of temp node */
    DFD_GCU_VOL_NUM_E,                  /* Number of vol node */
    DFD_GCU_POWER_NUM_E,                /* Number of power node */
    DFD_GCU_SENSOR_VAL_E,               /* Number of power node */
    DFD_GCU_DEVICE_TYPE_MAX_E,          /* Guard value marking end of enum */
} gcu_device_type;

typedef enum {
    DFD_BMC_MANAGED_SENSOR_INPUT_E,
    DFD_BMC_MANAGED_SENSOR_ALIAS_E,
    DFD_BMC_MANAGED_SENSOR_MAX_E,
    DFD_BMC_MANAGED_SENSOR_MIN_E,
    DFD_BMC_MANAGED_SENSOR_HIGH_E,
    DFD_BMC_MANAGED_SENSOR_LOW_E,
    DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E,
    DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E,
    DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E,
} bmc_managed_device_sensor_type;

typedef enum {
    DFD_GCU_TEMP_E = 1,             /* GPU temperature in Celsius */
    DFD_GCU_HBM_TEMP_E,             /* GPU HBM temperature in Celsius */
    DFD_GCU_TEMP_TYPE_MAX,
} gcu_temp_type;

typedef enum {
    DFD_GCU_VOL_E = 1,              /* GPU core voltage in Volts */
    DFD_GCU_HBM_VOL_E,              /* GPU HBM voltage in Volts */
    DFD_GCU_VOL_TYPE_MAX,
} gcu_vol_type;

typedef enum {
    DFD_GCU_POWER_E = 1,            /* GPU current power consumption in Watts */
    DFD_GCU_POWER_MAX_E,            /* Maximum GPU power rating in Watts */
    DFD_GCU_POWER_TYPE_MAX,
} gcu_power_type;

typedef enum {
    HEALTH_LEVEL_NORMAL = 0,
    HEALTH_LEVEL_PERIPHERAL_WARNING = 1,
    HEALTH_LEVEL_PCIE_WARNING = 2,
    HEALTH_LEVEL_HBM_WARNING = 3,
    HEALTH_LEVEL_PERIPHERAL_CRITICAL = 4,
    HEALTH_LEVEL_PCIE_CRITICAL = 5,
    HEALTH_LEVEL_HBM_CRITICAL = 6,
    HEALTH_LEVEL_MAX = 7,
} health_level_t;

/* fan func table */
typedef enum {
    DFD_FAN_BUS_E,
    DFD_FAN_ADDR_E,
    DFD_FAN_STATUS_ROTATION_E,
    DFD_FAN_MAX_E
} fan_device_type;

/* SENSORS attribute type */
typedef enum  {
    WB_FAN_PRESENT_ATTR       = 0,     /* FAN PRESENT */
    WB_FAN_ATTR_END,
} wb_fan_attr_type_t;

typedef enum {
    DFD_LSW_RESET_E,                     /* Get or set LSW reset */
    DFD_LSW_MAX_E,
} wb_lsw_attr_type_t;

typedef enum {
    DFD_PCIE_BDF_E,                      /* Get BDF from config file */
    DFD_PCIE_LINK_STATUS_E,              /* Get link status of PCIe device */
    DFD_PCIE_SPEED_STATUS_E,             /* GET speed status of PCIe device */
    DFD_PCIE_ALIAS_E,                    /* Get alias of PCIe device */
    DFD_PCIE_TYPE_E,                     /* Get type of PCIe device */
    DFD_PCIE_MAX_E,
} wb_pcie_attr_type_t;

typedef enum {
    DFD_CLOCK_STATUS_E,                  /* Get clock status of CLOCK device */
    DFD_CLOCK_ALIAS_E,                   /* Get alias of CLOCK device */
    DFD_CLOCK_MAX_E,
} wb_clock_attr_type_t;

typedef enum {
    DFD_PLL_BUS_STATUS_E,                /* Get bus status of PLL device */
    DFD_PLL_ALIAS_E,                     /* Get alias of PLL device */
    DFD_PLL_MAX_E,
} wb_pll_attr_type_t;

typedef enum {
    DFD_AVS_POWER_STATUS_NUM_E,          /* Get config number of power status */
    DFD_AVS_POWER_STATUS_E,              /* Get power status of AVS device */
    DFD_AVS_PHASE_CURR_NUM_E,            /* Get config number of phase curr */
    DFD_AVS_PHASE_CURR_E,                /* Get phase curr of AVS device */
    DFD_AVS_BUS_STATUS_E,                /* Get bus status of AVS device */
    DFD_AVS_ALIAS_E,                     /* Get alias of AVS device */
    DFD_AVS_CIRCUIT_STATUS_E,            /* Get circuit of AVS device */
    DFD_AVS_CLEAR_FAULTS_E,              /* Clear faults of AVS device */
    DFD_AVS_MAX_E,
} wb_avs_attr_type_t;

/* nvme func table */
typedef enum {
    DFD_NVME_STATUS_E,                   /* NVMe basic status */
    DFD_NVME_HEALTH_STATUS_E,            /* NVMe health status */
    DFD_NVME_PDLU_E,                     /* NVMe percentage drive life used */
    DFD_NVME_PCIE_VENDOR_ID_E,           /* PCIe Vendor ID in hexadecimal */
    DFD_NVME_SERIAL_NUMBER_E,            /* Device serial number string */
    DFD_NVME_FIRMWARE_VERSION_E,         /* Firmware version information string */
    DFD_NVME_BOOTLOADER_VERSION_E,       /* bootloader version */
    DFD_NVME_PCIE_DEVICE_ID_E,           /* PCIe Device ID in hexadecimal */
    DFD_NVME_PCIE_SUB_VENDOR_ID_E,       /* PCIe Subsystem Vendor ID in hexadecimal */
    DFD_NVME_PCIE_SUB_DEVICE_ID_E,       /* PCIe Subsystem Device ID in hexadecimal */
    DFD_NVME_MFR_NAME_E,                 /* Manufacturer name string */
    DFD_NVME_PART_NUMBER_E,              /* Manufacturer part number string */
    DFD_NVME_PSN_E,                      /* Product serial number */
    DFD_NVME_PRODUCT_NAME_E,             /* Product name */
    DFD_NVME_CAPACITY_E,                 /* NVME capacity in T bytes */
    DFD_NVME_INTERFACE_TYPE_E,           /* NVME interface type */
    DFD_NVME_MEDIA_TYPE_E,               /* NVME Media type */
    DFD_NVME_ALIAS_E,                    /* Get NVMe attr's alias name */
    DFD_NVME_SENSOR_INPUT,               /* Inputs of Sensors */
    DFD_NVME_ID_DETECT_E,                /* NVMe ID detect */
    DFD_NVME_MONITOR_FLAG_E,             /* NVMe monitor flag */
    DFD_NVME_TEMP_NUM_E,                 /* Num of temp sensors */
    DFD_NVME_POWER_NUM_E,                 /* Num of power sensors */
    DFD_NVME_OOB_DATA_BUS_NUMBER,        /* Get bus number from OOB block */
    DFD_NVME_VPD_DATA_BUS_NUMBER,        /* Get bus number from VPD block */
    DFD_NVME_MAX_E,                      /* Guard value marking end of enum */
} nvme_device_func_type_t;

typedef enum {
    DFD_NVME_SENSOR_INPUT_E,
    DFD_NVME_SENSOR_ALIAS_E,
    DFD_NVME_SENSOR_MAX_E,
    DFD_NVME_SENSOR_MIN_E,
    DFD_NVME_SENSOR_NORMAL_E,
    DFD_NVME_SENSOR_TYPE_MAX_E,
} nvme_sensor_type;

typedef enum {
    DFD_NVME_TEMP_E,             /* NVME temperature in Celsius */
    DFD_NVME_TEMP_MAX_E,         /* Maximum NVME temperature in Celsius */
    DFD_NVME_TEMP_ALIAS,
    DFD_NVME_TEMP_TYPE_MAX,
} nvme_temp_func_type_t;

typedef enum {
    DFD_NVME_POWER_E,            /* NVME current power consumption in Watts */
    DFD_NVME_POWER_ALIAS,
    DFD_NVME_POWER_TYPE_MAX,
} nvme_power_func_type_t;

/* disk func table */
typedef enum {
    DFD_DISK_ATA_PORT_E = 0,                        /* DISK ATA PORT */
    DFD_DISK_TYPE_E = 1,                            /* DISK disk type, HDD or SDD */
    DFD_DISK_TEMP_MAX_E = 2,                        /* DISK disk temp max */
    DFD_DISK_TEMP_MIN_E = 3,                        /* DISK disk temp mix */
    DFD_DISK_TEMP_HIGH_E = 4,                       /* DISK disk temp high */
    DFD_DISK_TEMP_LOW_E = 5,                        /* DISK disk temp low */
    DFD_DISK_TARGET_LINK_SPEED_E = 6,               /* DISK disk target link speed */
    DFD_DISK_REMAINING_LIFE_THRESHOLD_E = 7,        /* DISK SSD  remaining life threshold */
    DFD_DISK_TEMP_STATUS_E = 8,                     /* DISK disk temp status */
    DFD_DISK_LINK_ALARM_E = 9,                      /* DISK disk link alarm */
    DFD_DISK_WEAR_STATUS_E = 10,                    /* DISK SSD wear status */
    DFD_DISK_LINK_STATUS_E = 11,                    /* DISK disk link status */ 
    DFD_DISK_SPEED_STATUS_E = 12,                   /* DISK disk speed status */
    DFD_DISK_CE_E = 13,                             /* DISK disk correctable error status */
    DFD_DISK_UE_E = 14,                             /* DISK disk uncorrectable error status */
    DFD_DISK_MAX_E,                                 /* Guard value marking end of enum */
} disk_device_func_type_t;

/* md func table */
typedef enum {
    DFD_MD_STATUS_E = 0,                        /* multiple devices raid status */
    DFD_MD_DEV_PATH_E = 1,                      /* software raid device path */
    DFD_MD_MAX_E,                               /* Guard value marking end of enum */
} md_device_func_type_t;

/* dpu func table */
typedef enum {
    DFD_DPU_BUSID_E,
    DFD_DPU_NAME_E,
    DFD_DPU_VENDOR_E,
    DFD_DPU_DEVICE_E,
    DFD_DPU_SN_E,
    DFD_DPU_PN_E,
    DFD_DPU_MAC_E,
    DFD_DPU_ID_MONITOR_FLAG_E,
    DFD_DPU_FW_ALIAS_E,
    DFD_DPU_FW_VERSION_E,
    DFD_DPU_FW_SUP_UP_E,
    DFD_DPU_FW_UP_TYPE_E,
    DFD_DPU_TEMP_ALIAS_E,
    DFD_DPU_TEMP_VALUE_E,
    DFD_DPU_MAX_E
} dpu_device_type;

typedef enum {
    DFD_DPU_FW_BMC_E = 1,
    DFD_DPU_FW_CPLD_E,
    DFD_DPU_FW_UEFI_E,
    DFD_DPU_FW_IMU_E,
    DFD_DPU_FW_MCP_E,
    DFD_DPU_FW_TYPE_MAX_E,
} dpu_fw_type;

typedef enum {
    DFD_DPU_TEMP_BOARD_E = 1,
    DFD_DPU_TEMP_CHIP_E,
    DFD_DPU_TEMP_OPTICAL1_E,
    DFD_DPU_TEMP_OPTICAL2_E,
    DFD_DPU_TEMP_TYPE_MAX_E,
} dpu_temp_type;

typedef enum wb_dpu_main_dev_type_e {
    WB_MAIN_DPU_DEV_JAGUAR,
    WB_MAIN_DPU_DEV_MAX
} wb_dpu_main_dev_type_t;

typedef struct dfd_sysfs_func_map {
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_set_data_func set_func;
}dfd_sysfs_func_map_t;

typedef enum {
    CFG_NO_INDEX,               /* for  DFD_CFG_KEY(x, 0, 0) */
    CFG_INDEX1_ONLY,            /* for  DFD_CFG_KEY(x, index1, 0) */
    CFG_2INDEXES_1,             /* for  DFD_CFG_KEY(x, main_dev, sensor_index) */
    CFG_2INDEXES_2,             /* for  DFD_CFG_KEY(x, sensor_index, type) */
    CFG_INDEX1_INDEX2_CMB_1,    /* INDEX1= (((dev_index & 0xff) << 8) | (sensor_index & 0xff)) INDEX2=main_dev_id & 0x0f) << 4) | (sensor_attr & 0x0f)) */
    CFG_INDEX_TYPE_END
} wb_index_type_t;

typedef enum {
    CFG_INT_DATA,
    CFG_STR_DATA,
    CFG_DATA_TYPE_END,
} debug_data_type_t;

typedef struct dfd_debug_data_key_map {
    uint64_t key_prefix;
    wb_index_type_t index_type;
    debug_data_type_t data_type;
}dfd_debug_data_key_map_t;

typedef enum {
    DFD_DEBUG_VOL,
    DFD_DEBUG_CURR,
    DFD_DEBUG_FAN,
    DFD_DEBUG_PSU,
    DFD_DEBUG_PSU_SENSOR,
    DFD_DEBUG_SFF,
    DFD_DEBUG_TEMP,
    DFD_DEBUG_PCIE,
    DFD_DEBUG_AVS,
    DFD_DEBUG_DISK,
    DFD_DEBUG_MD,
    DFD_DEBUG_END,
} debug_data_dev_class_t;

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
extern int g_dfd_pcie_dbg_level;
extern int g_dfd_avs_dbg_level;
extern int g_dfd_disk_dbg_level;
extern int g_dfd_md_dbg_level;
extern int g_dfd_dpu_dbg_level;
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

#define DBG_LEAK_DETECTOR_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_leak_detector_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_SYSSOL_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_sol_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_DEBUG_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_debug_dbg_level & level) { \
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

#define DBG_GCU_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_gcu_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_NVME_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_nvme_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_LSW_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_lsw_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_PCIE_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_pcie_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_CLOCK_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_clock_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_PLL_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_pll_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_AVS_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_avs_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_DISK_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_disk_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_MD_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_md_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_DPU_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_dpu_dbg_level & level) { \
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
 * dfd_get_switch_serial_number - Get the serial_number of switch from syseeprom
 */
int dfd_get_switch_serial_number(char *buf, size_t count);

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

int dfd_set_init_cmd(unsigned int dev_class, unsigned int dev_index, int sysfs_type);

/** dfd_get_dev_debug_config_data - Get debug data of the device
 * @cfg_type: config type
 * @dev_id: device id
 * @dev_type: device type
 */
int dfd_get_dev_debug_config_data(dfd_cfg_item_id_t cfg_type, unsigned int dev_id, unsigned int dev_type, char *buf, size_t count);

#endif  /* _WB_MODULE_H_ */
