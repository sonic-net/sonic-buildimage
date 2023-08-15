/*
 * A i2c cpld driver for the ufispace_s9110_32x
 *
 * Copyright (C) 2022 UfiSpace Technology Corporation.
 * Nonodark Huang <nonodark.huang@ufispace.com>
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
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
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include "x86-64-ufispace-s9110-32x-cpld.h"

#if !defined(SENSOR_DEVICE_ATTR_RO)
#define SENSOR_DEVICE_ATTR_RO(_name, _func, _index)		\
	SENSOR_DEVICE_ATTR(_name, 0444, _func##_show, NULL, _index)
#endif

#if !defined(SENSOR_DEVICE_ATTR_RW)
#define SENSOR_DEVICE_ATTR_RW(_name, _func, _index)		\
	SENSOR_DEVICE_ATTR(_name, 0644, _func##_show, _func##_store, _index)

#endif

#if !defined(SENSOR_DEVICE_ATTR_WO)
#define SENSOR_DEVICE_ATTR_WO(_name, _func, _index)		\
	SENSOR_DEVICE_ATTR(_name, 0200, NULL, _func##_store, _index)
#endif


#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) \
    printk(KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define BSP_LOG_R(fmt, args...) \
    _bsp_log (LOG_READ, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_LOG_W(fmt, args...) \
    _bsp_log (LOG_WRITE, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)

#define I2C_READ_BYTE_DATA(ret, lock, i2c_client, reg) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_read_byte_data(i2c_client, reg); \
    mutex_unlock(lock); \
    BSP_LOG_R("cpld[%d], reg=0x%03x, reg_val=0x%02x", data->index, reg, ret); \
}

#define I2C_WRITE_BYTE_DATA(ret, lock, i2c_client, reg, val) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_write_byte_data(i2c_client, reg, val); \
    mutex_unlock(lock); \
    BSP_LOG_W("cpld[%d], reg=0x%03x, reg_val=0x%02x", data->index, reg, val); \
}

#define _DEVICE_ATTR(_name)     \
    &sensor_dev_attr_##_name.dev_attr.attr


/* CPLD sysfs attributes index  */
enum cpld_sysfs_attributes {
    //CPLD 1 & CPLD 2
    CPLD_MAJOR_VER,
    CPLD_MINOR_VER,
    CPLD_ID,
    CPLD_BUILD,
    CPLD_VERSION_H,
    CPLD_CHIP,
    CPLD_EVT_CTRL,

    //CPLD 1
    CPLD_BOARD_ID_0,
    CPLD_BOARD_ID_1,
    CPLD_SKU_EXT,

    CPLD_MAC_INTR,
    CPLD_HWM_INTR,
    CPLD_CPLD2_INTR,
    CPLD_PTP_INTR,
    CPLD_SYSTEM_INTR,

    CPLD_MAC_MASK,
    CPLD_HWM_MASK,
    CPLD_CPLD2_MASK,
    CPLD_PTP_MASK,
    CPLD_SYSTEM_MASK,

    CPLD_MAC_EVT,
    CPLD_HWM_EVT,
    CPLD_CPLD2_EVT,
    
    CPLD_MAC_RESET,
    CPLD_SYSTEM_RESET,
    CPLD_BMC_NTM_RESET,
    CPLD_USB_RESET,
    CPLD_I2C_MUX_RESET,
    CPLD_MISC_RESET,

    CPLD_BRD_PRESENT,
    CPLD_PSU_STATUS,
    CPLD_SYSTEM_PWR,
    CPLD_MAC_SYNCE,
    CPLD_MAC_AVS,
    CPLD_SYSTEM_STATUS,
    CPLD_WATCHDOG,
    CPLD_BOOT_SELECT,
    CPLD_MUX_CTRL,
    CPLD_MISC_CTRL_1,
    CPLD_MISC_CTRL_2,
    CPLD_MAC_TEMP,

    CPLD_SYSTEM_LED_PSU,
    CPLD_SYSTEM_LED_SYS,
    CPLD_SYSTEM_LED_FAN,
    CPLD_SYSTEM_LED_ID,

    DBG_CPLD_MAC_INTR,
    DBG_CPLD_HWM_INTR,
    DBG_CPLD_CPLD2_INTR,
    DBG_CPLD_PTP_INTR,

    //CPLD 2
    CPLD_QSFP_ABS_0_7,
    CPLD_QSFP_ABS_8_15,
    CPLD_QSFP_ABS_16_23,
    CPLD_QSFP_ABS_24_31,

    CPLD_QSFP_INTR_0_7,
    CPLD_QSFP_INTR_8_15,
    CPLD_QSFP_INTR_16_23,
    CPLD_QSFP_INTR_24_31,

    CPLD_SFP_ABS_0_1,
    CPLD_SFP_RXLOS_0_1,
    CPLD_SFP_TXFLT_0_1,

    CPLD_QSFP_MASK_ABS_0_7,
    CPLD_QSFP_MASK_ABS_8_15,
    CPLD_QSFP_MASK_ABS_16_23,
    CPLD_QSFP_MASK_ABS_24_31,

    CPLD_QSFP_MASK_INTR_0_7,
    CPLD_QSFP_MASK_INTR_8_15,
    CPLD_QSFP_MASK_INTR_16_23,
    CPLD_QSFP_MASK_INTR_24_31,

    CPLD_SFP_MASK_ABS_0_1,
    CPLD_SFP_MASK_RXLOS_0_1,
    CPLD_SFP_MASK_TXFLT_0_1,

    CPLD_QSFP_EVT_ABS_0_7,
    CPLD_QSFP_EVT_ABS_8_15,
    CPLD_QSFP_EVT_ABS_16_23,
    CPLD_QSFP_EVT_ABS_24_31,

    CPLD_QSFP_EVT_INTR_0_7,
    CPLD_QSFP_EVT_INTR_8_15,
    CPLD_QSFP_EVT_INTR_16_23,
    CPLD_QSFP_EVT_INTR_24_31,

    CPLD_SFP_EVT_ABS_0_1,
    CPLD_SFP_EVT_RXLOS_0_1,
    CPLD_SFP_EVT_TXFLT_0_1,

    CPLD_QSFP_RESET_0_7,
    CPLD_QSFP_RESET_8_15,
    CPLD_QSFP_RESET_16_23,
    CPLD_QSFP_RESET_24_31,

    CPLD_QSFP_LPMODE_0_7,
    CPLD_QSFP_LPMODE_8_15,
    CPLD_QSFP_LPMODE_16_23,
    CPLD_QSFP_LPMODE_24_31,

    CPLD_SFP_TXDIS_0_1,
    CPLD_SFP_TS_0_1,
    CPLD_SFP_RS_0_1,

    DBG_CPLD_QSFP_ABS_0_7,
    DBG_CPLD_QSFP_ABS_8_15,
    DBG_CPLD_QSFP_ABS_16_23,
    DBG_CPLD_QSFP_ABS_24_31,

    DBG_CPLD_QSFP_INTR_0_7,
    DBG_CPLD_QSFP_INTR_8_15,
    DBG_CPLD_QSFP_INTR_16_23,
    DBG_CPLD_QSFP_INTR_24_31,

    DBG_CPLD_SFP_ABS_0_1,
    DBG_CPLD_SFP_RXLOS_0_1,
    DBG_CPLD_SFP_TXFLT_0_1,

    //BSP DEBUG
    BSP_DEBUG
};

enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_UNK,
};

typedef struct  {
    u8 reg;
    u8 mask;
    u8 data_type;
} attr_reg_map_t;

static attr_reg_map_t attr_reg[]= {

    //CPLD 1 & CPLD 2
    [CPLD_MAJOR_VER]        = {CPLD_VERSION_REG          , MASK_1100_0000, DATA_DEC},
    [CPLD_MINOR_VER]        = {CPLD_VERSION_REG          , MASK_0011_1111, DATA_DEC},
    [CPLD_ID]               = {CPLD_ID_REG               , MASK_0000_0111, DATA_DEC},
    [CPLD_BUILD]            = {CPLD_BUILD_REG            , MASK_ALL      , DATA_DEC},
    [CPLD_VERSION_H]        = {CPLD_NONE_REG             , MASK_NONE     , DATA_UNK},
    [CPLD_CHIP]             = {CPLD_CHIP_REG             , MASK_ALL      , DATA_HEX},
    [CPLD_EVT_CTRL]         = {CPLD_EVT_CTRL_REG         , MASK_ALL      , DATA_HEX},

    //CPLD 1
    [CPLD_BOARD_ID_0]       = {CPLD_BOARD_ID_0_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_BOARD_ID_1]       = {CPLD_BOARD_ID_1_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_SKU_EXT]          = {CPLD_SKU_EXT_REG          , MASK_ALL      , DATA_DEC},

    [CPLD_MAC_INTR]         = {CPLD_MAC_INTR_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_HWM_INTR]         = {CPLD_HWM_INTR_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_CPLD2_INTR]       = {CPLD_CPLD2_INTR_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_PTP_INTR]         = {CPLD_PTP_INTR_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_INTR]      = {CPLD_SYSTEM_INTR_REG      , MASK_ALL      , DATA_HEX},

    [CPLD_MAC_MASK]         = {CPLD_MAC_MASK_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_HWM_MASK]         = {CPLD_HWM_MASK_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_CPLD2_MASK]       = {CPLD_CPLD2_MASK_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_PTP_MASK]         = {CPLD_PTP_MASK_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_MASK]      = {CPLD_SYSTEM_MASK_REG      , MASK_ALL      , DATA_HEX},

    [CPLD_MAC_EVT]          = {CPLD_MAC_EVT_REG          , MASK_ALL      , DATA_HEX},
    [CPLD_HWM_EVT]          = {CPLD_HWM_EVT_REG          , MASK_ALL      , DATA_HEX},
    [CPLD_CPLD2_EVT]        = {CPLD_CPLD2_EVT_REG        , MASK_ALL      , DATA_HEX},

    [CPLD_MAC_RESET]        = {CPLD_MAC_RESET_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_RESET]     = {CPLD_SYSTEM_RESET_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_BMC_NTM_RESET]    = {CPLD_BMC_NTM_RESET_REG    , MASK_ALL      , DATA_HEX},
    [CPLD_USB_RESET]        = {CPLD_USB_RESET_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_I2C_MUX_RESET]    = {CPLD_I2C_MUX_RESET_REG    , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_RESET]       = {CPLD_MISC_RESET_REG       , MASK_ALL      , DATA_HEX},

    [CPLD_BRD_PRESENT]     = {CPLD_BRD_PRESENT_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_PSU_STATUS]      = {CPLD_PSU_STATUS_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_PWR]      = {CPLD_SYSTEM_PWR_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_SYNCE]       = {CPLD_MAC_SYNCE_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_AVS]         = {CPLD_MAC_AVS_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_STATUS]   = {CPLD_SYSTEM_STATUS_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_WATCHDOG]        = {CPLD_WATCHDOG_REG          , MASK_ALL      , DATA_HEX},
    [CPLD_BOOT_SELECT]     = {CPLD_BOOT_SELECT_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_MUX_CTRL]        = {CPLD_MUX_CTRL_REG          , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_CTRL_1]     = {CPLD_MISC_CTRL_1_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_CTRL_2]     = {CPLD_MISC_CTRL_2_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_TEMP]        = {CPLD_MAC_TEMP_REG          , MASK_ALL      , DATA_HEX},

    [CPLD_SYSTEM_LED_PSU]  = {CPLD_SYSTEM_LED_PSU_REG    , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_LED_SYS]  = {CPLD_SYSTEM_LED_SYS_REG    , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_LED_FAN]  = {CPLD_SYSTEM_LED_FAN_REG    , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_LED_ID]   = {CPLD_SYSTEM_LED_ID_REG     , MASK_ALL      , DATA_HEX},

    [DBG_CPLD_MAC_INTR]    = {DBG_CPLD_MAC_INTR_REG      , MASK_ALL      , DATA_HEX},
    [DBG_CPLD_HWM_INTR]    = {DBG_CPLD_HWM_INTR_REG      , MASK_ALL      , DATA_HEX},
    [DBG_CPLD_CPLD2_INTR]  = {DBG_CPLD_CPLD2_INTR_REG    , MASK_ALL      , DATA_HEX},
    [DBG_CPLD_PTP_INTR]    = {DBG_CPLD_PTP_INTR_REG      , MASK_ALL      , DATA_HEX},

    //CPLD 2
    [CPLD_QSFP_ABS_0_7]        = {CPLD_QSFP_ABS_0_7_REG         , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_ABS_8_15]       = {CPLD_QSFP_ABS_8_15_REG        , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_ABS_16_23]      = {CPLD_QSFP_ABS_16_23_REG       , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_ABS_24_31]      = {CPLD_QSFP_ABS_24_31_REG       , MASK_ALL  , DATA_HEX},

    [CPLD_QSFP_INTR_0_7]       = {CPLD_QSFP_INTR_0_7_REG        , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_INTR_8_15]      = {CPLD_QSFP_INTR_8_15_REG       , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_INTR_16_23]     = {CPLD_QSFP_INTR_16_23_REG      , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_INTR_24_31]     = {CPLD_QSFP_INTR_24_31_REG      , MASK_ALL  , DATA_HEX},

    [CPLD_SFP_ABS_0_1]         = {CPLD_SFP_ABS_0_1_REG          , MASK_ALL  , DATA_HEX},
    [CPLD_SFP_RXLOS_0_1]       = {CPLD_SFP_RXLOS_0_1_REG        , MASK_ALL  , DATA_HEX},
    [CPLD_SFP_TXFLT_0_1]       = {CPLD_SFP_TXFLT_0_1_REG        , MASK_ALL  , DATA_HEX},

    [CPLD_QSFP_MASK_ABS_0_7]   = {CPLD_QSFP_MASK_ABS_0_7_REG    , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_MASK_ABS_8_15]  = {CPLD_QSFP_MASK_ABS_8_15_REG   , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_MASK_ABS_16_23] = {CPLD_QSFP_MASK_ABS_16_23_REG  , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_MASK_ABS_24_31] = {CPLD_QSFP_MASK_ABS_24_31_REG  , MASK_ALL  , DATA_HEX},

    [CPLD_QSFP_MASK_INTR_0_7]  = {CPLD_QSFP_MASK_INTR_0_7_REG   , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_MASK_INTR_8_15] = {CPLD_QSFP_MASK_INTR_8_15_REG  , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_MASK_INTR_16_23]= {CPLD_QSFP_MASK_INTR_16_23_REG , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_MASK_INTR_24_31]= {CPLD_QSFP_MASK_INTR_24_31_REG , MASK_ALL  , DATA_HEX},

    [CPLD_SFP_MASK_ABS_0_1]    = {CPLD_SFP_MASK_ABS_0_1_REG     , MASK_ALL  , DATA_HEX},
    [CPLD_SFP_MASK_RXLOS_0_1]  = {CPLD_SFP_MASK_RXLOS_0_1_REG   , MASK_ALL  , DATA_HEX},
    [CPLD_SFP_MASK_TXFLT_0_1]  = {CPLD_SFP_MASK_TXFLT_0_1_REG   , MASK_ALL  , DATA_HEX},

    [CPLD_QSFP_EVT_ABS_0_7]    = {CPLD_QSFP_EVT_ABS_0_7_REG     , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_EVT_ABS_8_15]   = {CPLD_QSFP_EVT_ABS_8_15_REG    , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_EVT_ABS_16_23]  = {CPLD_QSFP_EVT_ABS_16_23_REG   , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_EVT_ABS_24_31]  = {CPLD_QSFP_EVT_ABS_24_31_REG   , MASK_ALL  , DATA_HEX},

    [CPLD_QSFP_EVT_INTR_0_7]   = {CPLD_QSFP_EVT_INTR_0_7_REG    , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_EVT_INTR_8_15]  = {CPLD_QSFP_EVT_INTR_8_15_REG   , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_EVT_INTR_16_23] = {CPLD_QSFP_EVT_INTR_16_23_REG  , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_EVT_INTR_24_31] = {CPLD_QSFP_EVT_INTR_24_31_REG  , MASK_ALL  , DATA_HEX},

    [CPLD_SFP_EVT_ABS_0_1]     = {CPLD_SFP_EVT_ABS_0_1_REG      , MASK_ALL  , DATA_HEX},
    [CPLD_SFP_EVT_RXLOS_0_1]   = {CPLD_SFP_EVT_RXLOS_0_1_REG    , MASK_ALL  , DATA_HEX},
    [CPLD_SFP_EVT_TXFLT_0_1]   = {CPLD_SFP_EVT_TXFLT_0_1_REG    , MASK_ALL  , DATA_HEX},

    [CPLD_QSFP_RESET_0_7]      = {CPLD_QSFP_RESET_0_7_REG       , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_RESET_8_15]     = {CPLD_QSFP_RESET_8_15_REG      , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_RESET_16_23]    = {CPLD_QSFP_RESET_16_23_REG     , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_RESET_24_31]    = {CPLD_QSFP_RESET_24_31_REG     , MASK_ALL  , DATA_HEX},

    [CPLD_QSFP_LPMODE_0_7]     = {CPLD_QSFP_LPMODE_0_7_REG      , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_LPMODE_8_15]    = {CPLD_QSFP_LPMODE_8_15_REG     , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_LPMODE_16_23]   = {CPLD_QSFP_LPMODE_16_23_REG    , MASK_ALL  , DATA_HEX},
    [CPLD_QSFP_LPMODE_24_31]   = {CPLD_QSFP_LPMODE_24_31_REG    , MASK_ALL  , DATA_HEX},

    [CPLD_SFP_TXDIS_0_1]       = {CPLD_SFP_TXDIS_0_1_REG        , MASK_ALL  , DATA_HEX},
    [CPLD_SFP_TS_0_1]          = {CPLD_SFP_TS_0_1_REG           , MASK_ALL  , DATA_HEX},
    [CPLD_SFP_RS_0_1]          = {CPLD_SFP_RS_0_1_REG           , MASK_ALL  , DATA_HEX},

    [DBG_CPLD_QSFP_ABS_0_7]    = {DBG_CPLD_QSFP_ABS_0_7_REG     , MASK_ALL  , DATA_HEX},
    [DBG_CPLD_QSFP_ABS_8_15]   = {DBG_CPLD_QSFP_ABS_8_15_REG    , MASK_ALL  , DATA_HEX},
    [DBG_CPLD_QSFP_ABS_16_23]  = {DBG_CPLD_QSFP_ABS_16_23_REG   , MASK_ALL  , DATA_HEX},
    [DBG_CPLD_QSFP_ABS_24_31]  = {DBG_CPLD_QSFP_ABS_24_31_REG   , MASK_ALL  , DATA_HEX},

    [DBG_CPLD_QSFP_INTR_0_7]   = {DBG_CPLD_QSFP_INTR_0_7_REG    , MASK_ALL  , DATA_HEX},
    [DBG_CPLD_QSFP_INTR_8_15]  = {DBG_CPLD_QSFP_INTR_8_15_REG   , MASK_ALL  , DATA_HEX},
    [DBG_CPLD_QSFP_INTR_16_23] = {DBG_CPLD_QSFP_INTR_16_23_REG  , MASK_ALL  , DATA_HEX},
    [DBG_CPLD_QSFP_INTR_24_31] = {DBG_CPLD_QSFP_INTR_24_31_REG  , MASK_ALL  , DATA_HEX},

    [DBG_CPLD_SFP_ABS_0_1]     = {DBG_CPLD_SFP_ABS_0_1_REG      , MASK_ALL  , DATA_HEX},
    [DBG_CPLD_SFP_RXLOS_0_1]   = {DBG_CPLD_SFP_RXLOS_0_1_REG    , MASK_ALL  , DATA_HEX},
    [DBG_CPLD_SFP_TXFLT_0_1]   = {DBG_CPLD_SFP_TXFLT_0_1_REG    , MASK_ALL  , DATA_HEX},

    //BSP DEBUG
    [BSP_DEBUG]                = {CPLD_NONE_REG                 , MASK_NONE , DATA_UNK},
};

enum bsp_log_types {
    LOG_NONE,
    LOG_RW,
    LOG_READ,
    LOG_WRITE
};

enum bsp_log_ctrl {
    LOG_DISABLE,
    LOG_ENABLE
};

/* CPLD sysfs attributes hook functions  */
static ssize_t cpld_show(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t cpld_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static u8 _cpld_reg_read(struct device *dev, u8 reg, u8 mask);
static ssize_t cpld_reg_read(struct device *dev, char *buf, u8 reg, u8 mask, u8 data_type);
static ssize_t cpld_reg_write(struct device *dev, const char *buf, size_t count, u8 reg, u8 mask);
static ssize_t bsp_read(char *buf, char *str);
static ssize_t bsp_write(const char *buf, char *str, size_t str_len, size_t count);
static ssize_t bsp_callback_show(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t bsp_callback_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t cpld_version_h_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf);

static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

struct cpld_data {
    int index;                  /* CPLD index */
    struct mutex access_lock;   /* mutex for cpld access */
    u8 access_reg;              /* register to access */
};

/* CPLD device id and data */
static const struct i2c_device_id cpld_id[] = {
    { "s9110_32x_cpld1",  cpld1 },
    { "s9110_32x_cpld2",  cpld2 },
    {}
};

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

// CPLD 1 & CPLD2
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver           , cpld, CPLD_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver           , cpld, CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_id                  , cpld, CPLD_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_build               , cpld, CPLD_BUILD);
static SENSOR_DEVICE_ATTR_RO(cpld_version_h           , cpld_version_h, CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR_RO(cpld_chip                , cpld, CPLD_CHIP);
static SENSOR_DEVICE_ATTR_RW(cpld_evt_ctrl            , cpld, CPLD_EVT_CTRL);

//CPLD 1
static SENSOR_DEVICE_ATTR_RO(cpld_board_id_0          , cpld, CPLD_BOARD_ID_0);
static SENSOR_DEVICE_ATTR_RO(cpld_board_id_1          , cpld, CPLD_BOARD_ID_1);
static SENSOR_DEVICE_ATTR_RO(cpld_sku_ext             , cpld, CPLD_SKU_EXT);

static SENSOR_DEVICE_ATTR_RO(cpld_mac_intr            , cpld, CPLD_MAC_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_hwm_intr            , cpld, CPLD_HWM_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_cpld2_intr          , cpld, CPLD_CPLD2_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_ptp_intr            , cpld, CPLD_PTP_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_system_intr         , cpld, CPLD_SYSTEM_INTR);

static SENSOR_DEVICE_ATTR_RW(cpld_mac_mask            , cpld, CPLD_MAC_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_hwm_mask            , cpld, CPLD_HWM_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_cpld2_mask          , cpld, CPLD_CPLD2_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_ptp_mask            , cpld, CPLD_PTP_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_system_mask         , cpld, CPLD_SYSTEM_MASK);

static SENSOR_DEVICE_ATTR_RO(cpld_mac_evt             , cpld, CPLD_MAC_EVT);
static SENSOR_DEVICE_ATTR_RO(cpld_hwm_evt             , cpld, CPLD_HWM_EVT);
static SENSOR_DEVICE_ATTR_RO(cpld_cpld2_evt           , cpld, CPLD_CPLD2_EVT);

static SENSOR_DEVICE_ATTR_RW(cpld_mac_reset           , cpld, CPLD_MAC_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_system_reset        , cpld, CPLD_SYSTEM_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_bmc_ntm_reset       , cpld, CPLD_BMC_NTM_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_usb_reset           , cpld, CPLD_USB_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_i2c_mux_reset       , cpld, CPLD_I2C_MUX_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_misc_reset          , cpld, CPLD_MISC_RESET);

static SENSOR_DEVICE_ATTR_RO(cpld_brd_present         , cpld, CPLD_BRD_PRESENT);
static SENSOR_DEVICE_ATTR_RO(cpld_psu_status          , cpld, CPLD_PSU_STATUS);
static SENSOR_DEVICE_ATTR_RO(cpld_system_pwr          , cpld, CPLD_SYSTEM_PWR);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_synce           , cpld, CPLD_MAC_SYNCE);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_avs             , cpld, CPLD_MAC_AVS);
static SENSOR_DEVICE_ATTR_RO(cpld_system_status       , cpld, CPLD_SYSTEM_STATUS);
static SENSOR_DEVICE_ATTR_RO(cpld_watchdog            , cpld, CPLD_WATCHDOG);
static SENSOR_DEVICE_ATTR_RW(cpld_boot_select         , cpld, CPLD_BOOT_SELECT);
static SENSOR_DEVICE_ATTR_RW(cpld_mux_ctrl            , cpld, CPLD_MUX_CTRL);
static SENSOR_DEVICE_ATTR_RW(cpld_misc_ctrl_1         , cpld, CPLD_MISC_CTRL_1);
static SENSOR_DEVICE_ATTR_RW(cpld_misc_ctrl_2         , cpld, CPLD_MISC_CTRL_2);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_temp            , cpld, CPLD_MAC_TEMP);

static SENSOR_DEVICE_ATTR_RO(cpld_system_led_psu      , cpld, CPLD_SYSTEM_LED_PSU);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sys      , cpld, CPLD_SYSTEM_LED_SYS);
static SENSOR_DEVICE_ATTR_RO(cpld_system_led_fan      , cpld, CPLD_SYSTEM_LED_FAN);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_id       , cpld, CPLD_SYSTEM_LED_ID);

static SENSOR_DEVICE_ATTR_RO(dbg_cpld_mac_intr        , cpld, DBG_CPLD_MAC_INTR);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_hwm_intr        , cpld, DBG_CPLD_HWM_INTR);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_cpld2_intr      , cpld, DBG_CPLD_CPLD2_INTR);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_ptp_intr        , cpld, DBG_CPLD_PTP_INTR);

//CPLD 2
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_abs_0_7        , cpld, CPLD_QSFP_ABS_0_7);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_abs_8_15       , cpld, CPLD_QSFP_ABS_8_15);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_abs_16_23      , cpld, CPLD_QSFP_ABS_16_23);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_abs_24_31      , cpld, CPLD_QSFP_ABS_24_31);

static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_intr_0_7       , cpld, CPLD_QSFP_INTR_0_7);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_intr_8_15      , cpld, CPLD_QSFP_INTR_8_15);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_intr_16_23     , cpld, CPLD_QSFP_INTR_16_23);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_intr_24_31     , cpld, CPLD_QSFP_INTR_24_31);

static SENSOR_DEVICE_ATTR_RO(cpld_sfp_abs_0_1         , cpld, CPLD_SFP_ABS_0_1);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_rxlos_0_1       , cpld, CPLD_SFP_RXLOS_0_1);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_txflt_0_1       , cpld, CPLD_SFP_TXFLT_0_1);

static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_mask_abs_0_7   , cpld, CPLD_QSFP_MASK_ABS_0_7);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_mask_abs_8_15  , cpld, CPLD_QSFP_MASK_ABS_8_15);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_mask_abs_16_23 , cpld, CPLD_QSFP_MASK_ABS_16_23);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_mask_abs_24_31 , cpld, CPLD_QSFP_MASK_ABS_24_31);

static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_mask_intr_0_7  , cpld, CPLD_QSFP_MASK_INTR_0_7);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_mask_intr_8_15 , cpld, CPLD_QSFP_MASK_INTR_8_15);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_mask_intr_16_23, cpld, CPLD_QSFP_MASK_INTR_16_23);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_mask_intr_24_31, cpld, CPLD_QSFP_MASK_INTR_24_31);

static SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_abs_0_1    , cpld, CPLD_SFP_MASK_ABS_0_1);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_rxlos_0_1  , cpld, CPLD_SFP_MASK_RXLOS_0_1);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_txflt_0_1  , cpld, CPLD_SFP_MASK_TXFLT_0_1);

static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_evt_abs_0_7    , cpld, CPLD_QSFP_EVT_ABS_0_7);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_evt_abs_8_15   , cpld, CPLD_QSFP_EVT_ABS_8_15);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_evt_abs_16_23  , cpld, CPLD_QSFP_EVT_ABS_16_23);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_evt_abs_24_31  , cpld, CPLD_QSFP_EVT_ABS_24_31);

static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_evt_intr_0_7   , cpld, CPLD_QSFP_EVT_INTR_0_7);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_evt_intr_8_15  , cpld, CPLD_QSFP_EVT_INTR_8_15);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_evt_intr_16_23 , cpld, CPLD_QSFP_EVT_INTR_16_23);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_evt_intr_24_31 , cpld, CPLD_QSFP_EVT_INTR_24_31);

static SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_abs_0_1     , cpld, CPLD_SFP_EVT_ABS_0_1);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_rxlos_0_1   , cpld, CPLD_SFP_EVT_RXLOS_0_1);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_txflt_0_1   , cpld, CPLD_SFP_EVT_TXFLT_0_1);

static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_reset_0_7      , cpld, CPLD_QSFP_RESET_0_7);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_reset_8_15     , cpld, CPLD_QSFP_RESET_8_15);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_reset_16_23    , cpld, CPLD_QSFP_RESET_16_23);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_reset_24_31    , cpld, CPLD_QSFP_RESET_24_31);

static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_lpmode_0_7     , cpld, CPLD_QSFP_LPMODE_0_7);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_lpmode_8_15    , cpld, CPLD_QSFP_LPMODE_8_15);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_lpmode_16_23   , cpld, CPLD_QSFP_LPMODE_16_23);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_lpmode_24_31   , cpld, CPLD_QSFP_LPMODE_24_31);

static SENSOR_DEVICE_ATTR_RW(cpld_sfp_txdis_0_1       , cpld, CPLD_SFP_TXDIS_0_1);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_ts_0_1          , cpld, CPLD_SFP_TS_0_1);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_rs_0_1          , cpld, CPLD_SFP_RS_0_1);

static SENSOR_DEVICE_ATTR_RO(dbg_cpld_qsfp_abs_0_7    , cpld, DBG_CPLD_QSFP_ABS_0_7);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_qsfp_abs_8_15   , cpld, DBG_CPLD_QSFP_ABS_8_15);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_qsfp_abs_16_23  , cpld, DBG_CPLD_QSFP_ABS_16_23);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_qsfp_abs_24_31  , cpld, DBG_CPLD_QSFP_ABS_24_31);

static SENSOR_DEVICE_ATTR_RO(dbg_cpld_qsfp_intr_0_7   , cpld, DBG_CPLD_QSFP_INTR_0_7);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_qsfp_intr_8_15  , cpld, DBG_CPLD_QSFP_INTR_8_15);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_qsfp_intr_16_23 , cpld, DBG_CPLD_QSFP_INTR_16_23);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_qsfp_intr_24_31 , cpld, DBG_CPLD_QSFP_INTR_24_31);

static SENSOR_DEVICE_ATTR_RO(dbg_cpld_sfp_abs_0_1     , cpld, DBG_CPLD_SFP_ABS_0_1);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_sfp_rxlos_0_1   , cpld, DBG_CPLD_SFP_RXLOS_0_1);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_sfp_txflt_0_1   , cpld, DBG_CPLD_SFP_TXFLT_0_1);

//BSP DEBUG
static SENSOR_DEVICE_ATTR_RW(bsp_debug                , bsp_callback, BSP_DEBUG);

/* define support attributes of cpldx */

/* cpld 1 */
static struct attribute *cpld1_attributes[] = {
    _DEVICE_ATTR(cpld_board_id_0),
    _DEVICE_ATTR(cpld_board_id_1),
    _DEVICE_ATTR(cpld_sku_ext),

    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_chip),

    _DEVICE_ATTR(cpld_mac_intr),
    _DEVICE_ATTR(cpld_hwm_intr),
    _DEVICE_ATTR(cpld_cpld2_intr),
    _DEVICE_ATTR(cpld_ptp_intr),
    _DEVICE_ATTR(cpld_system_intr),

    _DEVICE_ATTR(cpld_mac_mask),
    _DEVICE_ATTR(cpld_hwm_mask),
    _DEVICE_ATTR(cpld_cpld2_mask),
    _DEVICE_ATTR(cpld_ptp_mask),
    _DEVICE_ATTR(cpld_system_mask),

    _DEVICE_ATTR(cpld_mac_evt),
    _DEVICE_ATTR(cpld_hwm_evt),
    _DEVICE_ATTR(cpld_cpld2_evt),

    _DEVICE_ATTR(cpld_evt_ctrl),

    _DEVICE_ATTR(cpld_mac_reset),
    _DEVICE_ATTR(cpld_system_reset),
    _DEVICE_ATTR(cpld_bmc_ntm_reset),
    _DEVICE_ATTR(cpld_usb_reset),
    _DEVICE_ATTR(cpld_i2c_mux_reset),
    _DEVICE_ATTR(cpld_misc_reset),

    _DEVICE_ATTR(cpld_brd_present),
    _DEVICE_ATTR(cpld_psu_status),
    _DEVICE_ATTR(cpld_system_pwr),
    _DEVICE_ATTR(cpld_mac_synce),
    _DEVICE_ATTR(cpld_mac_avs),
    _DEVICE_ATTR(cpld_system_status),
    _DEVICE_ATTR(cpld_watchdog),
    _DEVICE_ATTR(cpld_boot_select),
    _DEVICE_ATTR(cpld_mux_ctrl),
    _DEVICE_ATTR(cpld_misc_ctrl_1),
    _DEVICE_ATTR(cpld_misc_ctrl_2),
    _DEVICE_ATTR(cpld_mac_temp),

    _DEVICE_ATTR(cpld_system_led_psu),
    _DEVICE_ATTR(cpld_system_led_sys),
    _DEVICE_ATTR(cpld_system_led_fan),
    _DEVICE_ATTR(cpld_system_led_id),

    _DEVICE_ATTR(dbg_cpld_mac_intr),
    _DEVICE_ATTR(dbg_cpld_hwm_intr),
    _DEVICE_ATTR(dbg_cpld_cpld2_intr),
    _DEVICE_ATTR(dbg_cpld_ptp_intr),

    _DEVICE_ATTR(bsp_debug),

    NULL
};

/* cpld 2 */
static struct attribute *cpld2_attributes[] = {

    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_chip),

    _DEVICE_ATTR(cpld_qsfp_abs_0_7),
    _DEVICE_ATTR(cpld_qsfp_abs_8_15),
    _DEVICE_ATTR(cpld_qsfp_abs_16_23),
    _DEVICE_ATTR(cpld_qsfp_abs_24_31),

    _DEVICE_ATTR(cpld_qsfp_intr_0_7),
    _DEVICE_ATTR(cpld_qsfp_intr_8_15),
    _DEVICE_ATTR(cpld_qsfp_intr_16_23),
    _DEVICE_ATTR(cpld_qsfp_intr_24_31),

    _DEVICE_ATTR(cpld_sfp_abs_0_1),
    _DEVICE_ATTR(cpld_sfp_rxlos_0_1),
    _DEVICE_ATTR(cpld_sfp_txflt_0_1),

    _DEVICE_ATTR(cpld_qsfp_mask_abs_0_7),
    _DEVICE_ATTR(cpld_qsfp_mask_abs_8_15),
    _DEVICE_ATTR(cpld_qsfp_mask_abs_16_23),
    _DEVICE_ATTR(cpld_qsfp_mask_abs_24_31),

    _DEVICE_ATTR(cpld_qsfp_mask_intr_0_7),
    _DEVICE_ATTR(cpld_qsfp_mask_intr_8_15),
    _DEVICE_ATTR(cpld_qsfp_mask_intr_16_23),
    _DEVICE_ATTR(cpld_qsfp_mask_intr_24_31),

    _DEVICE_ATTR(cpld_sfp_mask_abs_0_1),
    _DEVICE_ATTR(cpld_sfp_mask_rxlos_0_1),
    _DEVICE_ATTR(cpld_sfp_mask_txflt_0_1),

    _DEVICE_ATTR(cpld_qsfp_evt_abs_0_7),
    _DEVICE_ATTR(cpld_qsfp_evt_abs_8_15),
    _DEVICE_ATTR(cpld_qsfp_evt_abs_16_23),
    _DEVICE_ATTR(cpld_qsfp_evt_abs_24_31),

    _DEVICE_ATTR(cpld_qsfp_evt_intr_0_7),
    _DEVICE_ATTR(cpld_qsfp_evt_intr_8_15),
    _DEVICE_ATTR(cpld_qsfp_evt_intr_16_23),
    _DEVICE_ATTR(cpld_qsfp_evt_intr_24_31),

    _DEVICE_ATTR(cpld_sfp_evt_abs_0_1),
    _DEVICE_ATTR(cpld_sfp_evt_rxlos_0_1),
    _DEVICE_ATTR(cpld_sfp_evt_txflt_0_1),

    _DEVICE_ATTR(cpld_evt_ctrl),

    _DEVICE_ATTR(cpld_qsfp_reset_0_7),
    _DEVICE_ATTR(cpld_qsfp_reset_8_15),
    _DEVICE_ATTR(cpld_qsfp_reset_16_23),
    _DEVICE_ATTR(cpld_qsfp_reset_24_31),

    _DEVICE_ATTR(cpld_qsfp_lpmode_0_7),
    _DEVICE_ATTR(cpld_qsfp_lpmode_8_15),
    _DEVICE_ATTR(cpld_qsfp_lpmode_16_23),
    _DEVICE_ATTR(cpld_qsfp_lpmode_24_31),

    _DEVICE_ATTR(cpld_sfp_txdis_0_1),
    _DEVICE_ATTR(cpld_sfp_ts_0_1),
    _DEVICE_ATTR(cpld_sfp_rs_0_1),

    _DEVICE_ATTR(dbg_cpld_qsfp_abs_0_7),
    _DEVICE_ATTR(dbg_cpld_qsfp_abs_8_15),
    _DEVICE_ATTR(dbg_cpld_qsfp_abs_16_23),
    _DEVICE_ATTR(dbg_cpld_qsfp_abs_24_31),

    _DEVICE_ATTR(dbg_cpld_qsfp_intr_0_7),
    _DEVICE_ATTR(dbg_cpld_qsfp_intr_8_15),
    _DEVICE_ATTR(dbg_cpld_qsfp_intr_16_23),
    _DEVICE_ATTR(dbg_cpld_qsfp_intr_24_31),

    _DEVICE_ATTR(dbg_cpld_sfp_abs_0_1),
    _DEVICE_ATTR(dbg_cpld_sfp_rxlos_0_1),
    _DEVICE_ATTR(dbg_cpld_sfp_txflt_0_1),

    NULL
};

/* cpld 1 attributes group */
static const struct attribute_group cpld1_group = {
    .attrs = cpld1_attributes,
};

/* cpld 2 attributes group */
static const struct attribute_group cpld2_group = {
    .attrs = cpld2_attributes,
};

/* reg shift */
static u8 _shift(u8 mask)
{
    int i=0, mask_one=1;

    for(i=0; i<8; ++i) {
        if ((mask & mask_one) == 1)
            return i;
        else
            mask >>= 1;
    }

    return -1;
}

/* reg mask and shift */
static u8 _mask_shift(u8 val, u8 mask)
{
    int shift=0;

    shift = _shift(mask);

    return (val & mask) >> shift;
}

static u8 _parse_data(char *buf, unsigned int data, u8 data_type)
{
    if(buf == NULL) {
        return -1;
    }

    if(data_type == DATA_HEX) {
        return sprintf(buf, "0x%02x", data);
    } else if(data_type == DATA_DEC) {
        return sprintf(buf, "%u", data);
    } else {
        return -1;
    }
    return 0;
}

static int _bsp_log(u8 log_type, char *fmt, ...)
{
    if ((log_type==LOG_READ  && enable_log_read) ||
        (log_type==LOG_WRITE && enable_log_write)) {
        va_list args;
        int r;

        va_start(args, fmt);
        r = vprintk(fmt, args);
        va_end(args);

        return r;
    } else {
        return 0;
    }
}

static int _config_bsp_log(u8 log_type)
{
    switch(log_type) {
        case LOG_NONE:
            enable_log_read = LOG_DISABLE;
            enable_log_write = LOG_DISABLE;
            break;
        case LOG_RW:
            enable_log_read = LOG_ENABLE;
            enable_log_write = LOG_ENABLE;
            break;
        case LOG_READ:
            enable_log_read = LOG_ENABLE;
            enable_log_write = LOG_DISABLE;
            break;
        case LOG_WRITE:
            enable_log_read = LOG_DISABLE;
            enable_log_write = LOG_ENABLE;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

/* get bsp value */
static ssize_t bsp_read(char *buf, char *str)
{
    ssize_t len=0;

    len=sprintf(buf, "%s", str);
    BSP_LOG_R("reg_val=%s", str);

    return len;
}

/* set bsp value */
static ssize_t bsp_write(const char *buf, char *str, size_t str_len, size_t count)
{
    snprintf(str, str_len, "%s", buf);
    BSP_LOG_W("reg_val=%s", str);

    return count;
}

/* get bsp parameter value */
static ssize_t bsp_callback_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len=0;
    char *str=NULL;

    switch (attr->index) {
        case BSP_DEBUG:
            str = bsp_debug;
            str_len = sizeof(bsp_debug);
            break;
        default:
            return -EINVAL;
    }
    return bsp_read(buf, str);
}

/* set bsp parameter value */
static ssize_t bsp_callback_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len=0;
    char *str=NULL;
    ssize_t ret = 0;
    u8 bsp_debug_u8 = 0;

    switch (attr->index) {
        case BSP_DEBUG:
            str = bsp_debug;
            str_len = sizeof(str);
            ret = bsp_write(buf, str, str_len, count);

            if (kstrtou8(buf, 0, &bsp_debug_u8) < 0) {
                return -EINVAL;
            } else if (_config_bsp_log(bsp_debug_u8) < 0) {
                return -EINVAL;
            }
            return ret;
        default:
            return -EINVAL;
    }
    return 0;
}

/* get cpld register value */
static ssize_t cpld_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;
    u8 mask = MASK_NONE;
    u8 data_type=DATA_UNK;

    switch (attr->index) {
        //CPLD 1 & CPLD 2
        case CPLD_MAJOR_VER:
        case CPLD_MINOR_VER:
        case CPLD_ID:
        case CPLD_BUILD:
        case CPLD_CHIP:
        case CPLD_EVT_CTRL:

        //CPLD 1
        case CPLD_BOARD_ID_0:
        case CPLD_BOARD_ID_1:
        case CPLD_SKU_EXT:

        case CPLD_MAC_INTR:
        case CPLD_HWM_INTR:
        case CPLD_CPLD2_INTR:
        case CPLD_PTP_INTR:
        case CPLD_SYSTEM_INTR:

        case CPLD_MAC_MASK:
        case CPLD_HWM_MASK:
        case CPLD_CPLD2_MASK:
        case CPLD_PTP_MASK:
        case CPLD_SYSTEM_MASK:

        case CPLD_MAC_EVT:
        case CPLD_HWM_EVT:
        case CPLD_CPLD2_EVT:
        
        case CPLD_MAC_RESET:
        case CPLD_SYSTEM_RESET:
        case CPLD_BMC_NTM_RESET:
        case CPLD_USB_RESET:
        case CPLD_I2C_MUX_RESET:
        case CPLD_MISC_RESET:

        case CPLD_BRD_PRESENT:
        case CPLD_PSU_STATUS:
        case CPLD_SYSTEM_PWR:
        case CPLD_MAC_SYNCE:
        case CPLD_MAC_AVS:
        case CPLD_SYSTEM_STATUS:
        case CPLD_WATCHDOG:
        case CPLD_BOOT_SELECT:
        case CPLD_MUX_CTRL:
        case CPLD_MISC_CTRL_1:
        case CPLD_MISC_CTRL_2:
        case CPLD_MAC_TEMP:

        case CPLD_SYSTEM_LED_PSU:
        case CPLD_SYSTEM_LED_SYS:
        case CPLD_SYSTEM_LED_FAN:
        case CPLD_SYSTEM_LED_ID:

        case DBG_CPLD_MAC_INTR:
        case DBG_CPLD_HWM_INTR:
        case DBG_CPLD_CPLD2_INTR:
        case DBG_CPLD_PTP_INTR:

        //CPLD 2
        case CPLD_QSFP_ABS_0_7:
        case CPLD_QSFP_ABS_8_15:
        case CPLD_QSFP_ABS_16_23:
        case CPLD_QSFP_ABS_24_31:

        case CPLD_QSFP_INTR_0_7:
        case CPLD_QSFP_INTR_8_15:
        case CPLD_QSFP_INTR_16_23:
        case CPLD_QSFP_INTR_24_31:

        case CPLD_SFP_ABS_0_1:
        case CPLD_SFP_RXLOS_0_1:
        case CPLD_SFP_TXFLT_0_1:

        case CPLD_QSFP_MASK_ABS_0_7:
        case CPLD_QSFP_MASK_ABS_8_15:
        case CPLD_QSFP_MASK_ABS_16_23:
        case CPLD_QSFP_MASK_ABS_24_31:

        case CPLD_QSFP_MASK_INTR_0_7:
        case CPLD_QSFP_MASK_INTR_8_15:
        case CPLD_QSFP_MASK_INTR_16_23:
        case CPLD_QSFP_MASK_INTR_24_31:

        case CPLD_SFP_MASK_ABS_0_1:
        case CPLD_SFP_MASK_RXLOS_0_1:
        case CPLD_SFP_MASK_TXFLT_0_1:

        case CPLD_QSFP_EVT_ABS_0_7:
        case CPLD_QSFP_EVT_ABS_8_15:
        case CPLD_QSFP_EVT_ABS_16_23:
        case CPLD_QSFP_EVT_ABS_24_31:

        case CPLD_QSFP_EVT_INTR_0_7:
        case CPLD_QSFP_EVT_INTR_8_15:
        case CPLD_QSFP_EVT_INTR_16_23:
        case CPLD_QSFP_EVT_INTR_24_31:

        case CPLD_SFP_EVT_ABS_0_1:
        case CPLD_SFP_EVT_RXLOS_0_1:
        case CPLD_SFP_EVT_TXFLT_0_1:

        case CPLD_QSFP_RESET_0_7:
        case CPLD_QSFP_RESET_8_15:
        case CPLD_QSFP_RESET_16_23:
        case CPLD_QSFP_RESET_24_31:

        case CPLD_QSFP_LPMODE_0_7:
        case CPLD_QSFP_LPMODE_8_15:
        case CPLD_QSFP_LPMODE_16_23:
        case CPLD_QSFP_LPMODE_24_31:

        case CPLD_SFP_TXDIS_0_1:
        case CPLD_SFP_TS_0_1:
        case CPLD_SFP_RS_0_1:

        case DBG_CPLD_QSFP_ABS_0_7:
        case DBG_CPLD_QSFP_ABS_8_15:
        case DBG_CPLD_QSFP_ABS_16_23:
        case DBG_CPLD_QSFP_ABS_24_31:

        case DBG_CPLD_QSFP_INTR_0_7:
        case DBG_CPLD_QSFP_INTR_8_15:
        case DBG_CPLD_QSFP_INTR_16_23:
        case DBG_CPLD_QSFP_INTR_24_31:

        case DBG_CPLD_SFP_ABS_0_1:
        case DBG_CPLD_SFP_RXLOS_0_1:
        case DBG_CPLD_SFP_TXFLT_0_1:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            data_type = attr_reg[attr->index].data_type;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_read(dev, buf, reg, mask, data_type);
}

/* set cpld register value */
static ssize_t cpld_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;
    u8 mask = MASK_NONE;

    switch (attr->index) {

        // CPLD 1 & CPLD2
        case CPLD_EVT_CTRL:

        //CPLD 1
        case CPLD_MAC_MASK:
        case CPLD_HWM_MASK:
        case CPLD_CPLD2_MASK:
        case CPLD_PTP_MASK:
        case CPLD_SYSTEM_MASK:
        case CPLD_MAC_RESET:
        case CPLD_SYSTEM_RESET:
        case CPLD_BMC_NTM_RESET:
        case CPLD_USB_RESET:
        case CPLD_I2C_MUX_RESET:
        case CPLD_MISC_RESET:
        case CPLD_BOOT_SELECT:
        case CPLD_MUX_CTRL:
        case CPLD_MISC_CTRL_1:
        case CPLD_MISC_CTRL_2:
        case CPLD_SYSTEM_LED_PSU:
        case CPLD_SYSTEM_LED_SYS:
        case CPLD_SYSTEM_LED_FAN:
        case CPLD_SYSTEM_LED_ID:

        //CPLD 2
        case CPLD_QSFP_MASK_ABS_0_7:
        case CPLD_QSFP_MASK_ABS_8_15:
        case CPLD_QSFP_MASK_ABS_16_23:
        case CPLD_QSFP_MASK_ABS_24_31:
        case CPLD_QSFP_MASK_INTR_0_7:
        case CPLD_QSFP_MASK_INTR_8_15:
        case CPLD_QSFP_MASK_INTR_16_23:
        case CPLD_QSFP_MASK_INTR_24_31:
        case CPLD_SFP_MASK_ABS_0_1:
        case CPLD_SFP_MASK_RXLOS_0_1:
        case CPLD_SFP_MASK_TXFLT_0_1:
        case CPLD_QSFP_RESET_0_7:
        case CPLD_QSFP_RESET_8_15:
        case CPLD_QSFP_RESET_16_23:
        case CPLD_QSFP_RESET_24_31:
        case CPLD_QSFP_LPMODE_0_7:
        case CPLD_QSFP_LPMODE_8_15:
        case CPLD_QSFP_LPMODE_16_23:
        case CPLD_QSFP_LPMODE_24_31:
        case CPLD_SFP_TXDIS_0_1:
        case CPLD_SFP_TS_0_1:
        case CPLD_SFP_RS_0_1:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_write(dev, buf, count, reg, mask);
}

/* get cpld register value */
static u8 _cpld_reg_read(struct device *dev,
                    u8 reg,
                    u8 mask)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int reg_val;

    I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);

    if (unlikely(reg_val < 0)) {
        return reg_val;
    } else {
        reg_val=_mask_shift(reg_val, mask);
        return reg_val;
    }
}

/* get cpld register value */
static ssize_t cpld_reg_read(struct device *dev,
                    char *buf,
                    u8 reg,
                    u8 mask,
                    u8 data_type)
{
    int reg_val;

    reg_val = _cpld_reg_read(dev, reg, mask);
    if (unlikely(reg_val < 0)) {
        dev_err(dev, "cpld_reg_read() error, reg_val=%d\n", reg_val);
        return reg_val;
    } else {
        return _parse_data(buf, reg_val, data_type);
    }
}

/* set cpld register value */
static ssize_t cpld_reg_write(struct device *dev,
                    const char *buf,
                    size_t count,
                    u8 reg,
                    u8 mask)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg_val, reg_val_now, shift;
    int ret = 0;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    //apply continuous bits operation if mask is specified, discontinuous bits are not supported
    if (mask != MASK_ALL) {
        reg_val_now = _cpld_reg_read(dev, reg, MASK_ALL);
        if (unlikely(reg_val_now < 0)) {
            dev_err(dev, "cpld_reg_write() error, reg_val_now=%d\n", reg_val_now);
            return reg_val_now;
        } else {
            //clear bits in reg_val_now by the mask
            reg_val_now &= ~mask;
            //get bit shift by the mask
            shift = _shift(mask);
            //calculate new reg_val
            reg_val = reg_val_now | (reg_val << shift);
        }
    }

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
               client, reg, reg_val);

    if (unlikely(ret < 0)) {
        dev_err(dev, "cpld_reg_write() error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* get qsfp port config register value */
static ssize_t cpld_version_h_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == CPLD_VERSION_H) {
        return sprintf(buf, "%d.%02d.%03d",
                _cpld_reg_read(dev, attr_reg[CPLD_MAJOR_VER].reg, attr_reg[CPLD_MAJOR_VER].mask),
                _cpld_reg_read(dev, attr_reg[CPLD_MINOR_VER].reg, attr_reg[CPLD_MINOR_VER].mask),
                _cpld_reg_read(dev, attr_reg[CPLD_BUILD].reg, attr_reg[CPLD_BUILD].mask));
    }
    return -1;
}

/* add valid cpld client to list */
static void cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = NULL;

    node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);
    if (!node) {
        dev_info(&client->dev,
            "Can't allocate cpld_client_node for index %d\n",
            client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

/* remove exist cpld client in list */
static void cpld_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);
    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                struct cpld_client_node, list);

        if (cpld_node->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(cpld_node);
    }
    mutex_unlock(&list_lock);
}

/* cpld drvier probe */
static int cpld_probe(struct i2c_client *client,
                    const struct i2c_device_id *dev_id)
{
    int status;
    struct cpld_data *data = NULL;
    int ret = -EPERM;

    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    /* init cpld data for client */
    i2c_set_clientdata(client, data);
    mutex_init(&data->access_lock);

    if (!i2c_check_functionality(client->adapter,
                I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_info(&client->dev,
            "i2c_check_functionality failed (0x%x)\n",
            client->addr);
        status = -EIO;
        goto exit;
    }

    /* get cpld id from device */
    ret = i2c_smbus_read_byte_data(client, CPLD_ID_REG);

    if (ret < 0) {
        dev_info(&client->dev,
            "fail to get cpld id (0x%x) at addr (0x%x)\n",
            CPLD_ID_REG, client->addr);
        status = -EIO;
        goto exit;
    }

    if (INVALID(ret, cpld1, cpld2)) {
        dev_info(&client->dev,
            "cpld id %d(device) not valid\n", ret);
        //status = -EPERM;
        //goto exit;
    }

    data->index = dev_id->driver_data;

    /* register sysfs hooks for different cpld group */
    dev_info(&client->dev, "probe cpld with index %d\n", data->index);
    switch (data->index) {
    case cpld1:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld1_group);
        break;
    case cpld2:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld2_group);
        break;
    default:
        status = -EINVAL;
    }

    if (status)
        goto exit;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    cpld_add_client(client);

    return 0;
exit:
    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &cpld1_group);
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        break;
    default:
        break;
    }
    return status;
}

/* cpld drvier remove */
static int cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);

    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &cpld1_group);
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        break;
    }

    cpld_remove_client(client);
    return 0;
}

#if 0 /* FIXME */
#define I2C_RW_RETRY_COUNT  3
#define I2C_RW_RETRY_INTERVAL 60

static int s9110_32x_cpld_read_internal(struct i2c_client *client, u8 reg)
{
    int retry = I2C_RW_RETRY_COUNT;
    int reg_val = 0;
    struct cpld_data *data = i2c_get_clientdata(client);

    while (retry) {
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (unlikely(reg_val < 0)) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;

            if (retry == 0) {
                dev_err(&client->dev, "%s() retry %d times but still failed, reg=%x\n", __func__, I2C_RW_RETRY_COUNT, reg);
            }

            continue;
        }

        break;
    }

    return reg_val;
}

static int s9110_32x_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
{
    int ret = 0, retry = I2C_RW_RETRY_COUNT;
    struct cpld_data *data = i2c_get_clientdata(client);

    while (retry) {
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock, client, reg, value);
        if (unlikely(ret < 0)) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;

            if (retry == 0) {
                dev_err(&client->dev, "%s() retry %d times but still failed, reg=%x\n", __func__, I2C_RW_RETRY_COUNT, reg);
            }

            continue;
        }
        break;
    }

    return ret;
}


int s9110_32x_cpld_psu_mux_sel(u8 mux_sel)
{
    unsigned short cpld_addr = cpld_i2c_addr[0];
    u8 reg = CPLD_MUX_CTRL_REG;
    u8 reg_val = 0;
    u8 psu_mux_mask = 0x06;
    u8 mux_sel_val = 0;

    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    switch(mux_sel) {
    case 0:
        //psu 0
        mux_sel_val = 0x04;
        break;
    case 1:
        //psu 1
        mux_sel_val = 0x02;
        break;
    default:
        //bmc
        mux_sel_val = psu_mux_mask;
        break;
    }

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            //read current reg value
            reg_val = s9110_32x_cpld_read_internal(cpld_node->client, reg);
            //clear psu_mux_sel bits (bit 1 and 2)
            reg_val &= ~psu_mux_mask;
            //modify psu_mux_sel bits (bit 1 and 2)
            reg_val |= mux_sel_val;
            //write reg value
            s9110_32x_cpld_write_internal(cpld_node->client, reg, reg_val);

            break;
        } else {
            pr_err("cpld_node->client->addr=%x, cpld_addr=%x\n", cpld_node->client->addr, cpld_addr);
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(s9110_32x_cpld_psu_mux_sel);
#endif

MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9110_32x_cpld",
    },
    .probe = cpld_probe,
    .remove = cpld_remove,
    .id_table = cpld_id,
    .address_list = cpld_i2c_addr,
};

static int __init cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&cpld_driver);
}

static void __exit cpld_exit(void)
{
    i2c_del_driver(&cpld_driver);
}

MODULE_AUTHOR("Nonodark Huang <nonodark.huang@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9110_32x_cpld driver");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
