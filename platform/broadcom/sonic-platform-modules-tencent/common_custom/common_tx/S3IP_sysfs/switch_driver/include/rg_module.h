#ifndef _RG_MODULE_H_
#define _RG_MODULE_H_

#include "switch_driver.h"

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
} psu_sensors_type_t;

typedef enum rg_wdt_type_e {
    RG_WDT_TYPE_NAME         = 0,     /* watchdog identify */
    RG_WDT_TYPE_STATE        = 1,     /* watchdog state */
    RG_WDT_TYPE_TIMELEFT     = 2,     /* watchdog timeleft */
    RG_WDT_TYPE_TIMEOUT      = 3,     /* watchdog timeout */
    RG_WDT_TYPE_ENABLE       = 4,     /* watchdog enable */
} rg_wdt_type_t;

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
} dfd_dev_tlv_type_t;

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
    RG_MINOR_DEV_FAN   = 7,
    RG_MINOR_DEV_CPLD  = 8,    /* CPLD */
    RG_MINOR_DEV_FPGA  = 9,    /* FPGA */
} rg_minor_dev_type_t;

typedef enum rg_sensor_type_e {
    RG_SENSOR_INPUT       = 0,
    RG_SENSOR_ALIAS       = 1,
    RG_SENSOR_TYPE        = 2,
    RG_SENSOR_MAX         = 3,
    RG_SENSOR_MAX_HYST    = 4,
    RG_SENSOR_MIN         = 5,
    RG_SENSOR_CRIT        = 6,
    RG_SENSOR_RANGE       = 7,
    RG_SENSOR_NOMINAL_VAL = 8,
} rg_sensor_type_t;

typedef enum rg_sff_cpld_attr_e {
    RG_SFF_POWER_ON      = 0x01,
    RG_SFF_TX_FAULT,
    RG_SFF_TX_DIS,
    RG_SFF_PRESENT_RESERVED,
    RG_SFF_RX_LOS,
    RG_SFF_RESET,
    RG_SFF_LPMODE,
    RG_SFF_MODULE_PRESENT,
    RG_SFF_INTERRUPT,
} rg_sff_cpld_attr_t;

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

extern int g_dfd_dbg_level;
extern int g_dfd_fan_dbg_level;
extern int g_dfd_fru_dbg_level;
extern int g_dfd_eeprom_dbg_level;
extern int g_dfd_cpld_dbg_level;
extern int g_dfd_fpga_dbg_level;
extern int g_dfd_sysled_dbg_level;
extern int g_dfd_slot_dbg_level;
extern int g_dfd_sensor_dbg_level;
extern int g_dfd_psu_dbg_level;
extern int g_dfd_sff_dbg_level;
extern int g_dfd_watchdog_dbg_level;

#define DBG_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_FAN_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_fan_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_FRU_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_fru_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_EEPROM_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_eeprom_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_CPLD_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_cpld_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_FPGA_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_fpga_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DBG_SYSLED_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_sysled_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_SLOT_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_slot_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_SENSOR_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_sensor_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_PSU_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_psu_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_SFF_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_sff_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#define DFD_WDT_DEBUG(level, fmt, arg...) do { \
    if (g_dfd_watchdog_dbg_level & level) { \
        if(level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

int32_t rg_dev_cfg_init(void);

void rg_dev_cfg_exit(void);

int dfd_get_dev_number(unsigned int main_dev_id, unsigned int minor_dev_id);
#endif  /* _RG_MODULE_H_ */
