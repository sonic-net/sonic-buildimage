/*
 * A header definition for switch_driver driver
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

#ifndef _SWITCH_DRIVER_H_
#define _SWITCH_DRIVER_H_

#define SWITCH_DEV_NO_SUPPORT         "NA"
#define SWITCH_DEV_ERROR              "ACCESS FAILED"
#define WB_SYSFS_RV_UNSUPPORT         (999)

typedef enum dbg_level_e {
    DBG_VERBOSE = 0x01,
    DBG_WARN    = 0x02,
    DBG_ERROR   = 0x04,
} dbg_level_t;

typedef enum fan_status_e {
    FAN_STATUS_ABSENT  = 0,
    FAN_STATUS_OK      = 1,
    FAN_STATUS_NOT_OK  = 2,
} fan_status_t;

typedef enum led_status_e {
    LED_STATUS_DARK         = 0,
    LED_STATUS_GREEN        = 1,
    LED_STATUS_YELLOW       = 2,
    LED_STATUS_RED          = 3,
    LED_STATUS_BLUE         = 4,
    LED_STATUS_GREEN_FLASH  = 5,
    LED_STATUS_YELLOW_FLASH = 6,
    LED_STATUS_RED_FLASH    = 7,
} led_status_t;

typedef enum air_flow_direction_e {
    F2B = 0, /* air enters from the front of the cabinet, and exhausts from the back */
    B2F = 1, /* air enters from the back of the cabinet, and exhausts from the front */
} air_flow_direction_t;

typedef enum psu_input_type_e {
    POWER_DC = 0,
    POWER_AC = 1,
} psu_input_type_t;

typedef enum psu_status_e {
    PSU_STATUS_ABSENT  = 0, /* psu absent */
    PSU_STATUS_PRESENT = 1, /* psu present and status ok */
    PSU_STATUS_WARN    = 2, /* psu present and status warn (pmbus 0x79 bit11 value 0)*/
    PSU_STATUS_FAIL    = 3, /* psu present and status fail (pmbus 0x79 bit11 value 1)*/
} psu_status_t;

typedef enum psu_status_word_e {
    PSU_VOUT_FAULT = 0x8000,
    PSU_IOUT_FAULT = 0x4000,
    PSU_INPUT_FAULT = 0x2000,
    PSU_MFR_FAULT = 0x1000,
    PSU_PG_FAULT = 0x0800,
    PSU_FAN_FAULT = 0x0400,
    PSU_OFF_FAULT = 0x0040,
    PSU_TEMP_FAULT = 0x0004,
} psu_status_word_t;

typedef enum psu_io_status_e {
    PSU_IO_STATUS_ABNORMAL = 0,
    PSU_IO_STATUS_NORMAL  = 1,
} psu_io_status_t;

typedef enum dev_status_e {
    DEV_ABSENT  = 0, /* dev absent */
    DEV_PRESENT = 1, /* dev present */
} dev_status_t;

extern int g_switch_dbg_level;

#define SWITCH_DEBUG(level, fmt, arg...) do { \
    if (g_switch_dbg_level & level) { \
        if (level >= DBG_ERROR) { \
            printk(KERN_ERR "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } else { \
            printk(KERN_INFO "[DBG-%d]:<%s, %d>:"fmt, level, __FUNCTION__, __LINE__, ##arg); \
        } \
    } \
} while (0)

#endif  /* _SWITCH_DRIVER_H_ */
