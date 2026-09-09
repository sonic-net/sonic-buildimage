/*
 * A header definition for wb_module driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _WB_MODULE_H_
#define _WB_MODULE_H_

#include "switch_driver.h"

#define mem_clear(data, size) memset((data), 0, (size))
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
    PSU_HW_STATUS          = 27,
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
} dfd_dev_tlv_type_t;

/* Master device type */
typedef enum wb_main_dev_type_e {
    WB_MAIN_DEV_MAINBOARD = 0,      /* Main board */
    WB_MAIN_DEV_FAN       = 1,      /* FAN */
    WB_MAIN_DEV_PSU       = 2,      /* PSU */
    WB_MAIN_DEV_SFF       = 3,      /* Optical module */
    WB_MAIN_DEV_CPLD      = 4,      /* CPLD */
    WB_MAIN_DEV_SLOT      = 5,      /* Daughter card */
} wb_main_dev_type_t;

/* Subdevice type */
typedef enum wb_minor_dev_type_e {
    WB_MINOR_DEV_NONE  = 0,    /* None */
    WB_MINOR_DEV_TEMP  = 1,    /* temperature*/
    WB_MINOR_DEV_IN    = 2,    /* voltage */
    WB_MINOR_DEV_CURR  = 3,    /* current */
    WB_MINOR_DEV_POWER = 4,    /* power */
    WB_MINOR_DEV_MOTOR = 5,    /* motor */
    WB_MINOR_DEV_PSU   = 6,    /* Power supply type */
    WB_MINOR_DEV_FAN   = 7,    /* Fan model */
    WB_MINOR_DEV_CPLD  = 8,    /* CPLD */
    WB_MINOR_DEV_FPGA  = 9,    /* FPGA */
    WB_MINOR_DEV_EEPROM = 10,  /* EEPROM */
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
    WB_BMC_LED_FRONT   = 2,      /* BMC indicator on the front panel */
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
} wb_led_t;

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
extern int g_dfd_custom_dbg_level;

#define WB_MIN(a, b)   ((a) < (b) ? (a) : (b))
#define WB_MAX(a, b)   ((a) > (b) ? (a) : (b))

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
#endif  /* _WB_MODULE_H_ */
