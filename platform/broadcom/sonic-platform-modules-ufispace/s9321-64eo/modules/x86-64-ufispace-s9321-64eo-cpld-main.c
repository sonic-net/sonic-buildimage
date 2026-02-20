/*
 * A i2c cpld driver for the ufispace_s9321_64eo
 *
 * Copyright (C) 2017-2019 UfiSpace Technology Corporation.
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

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>


#include "x86-64-ufispace-s9321-64eo-cpld-main.h"

bool mux_en = false;
module_param(mux_en, bool, S_IWUSR|S_IRUSR);

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
    // CPLD Common
    CPLD_MINOR_VER,
    CPLD_MAJOR_VER,
    CPLD_ID,
    CPLD_BUILD_VER,
    CPLD_VERSION_H,
    CPLD_EVT_CTRL,

    // CPLD 1
    CPLD_BOARD_ID_0,
    CPLD_BOARD_ID_1,
    CPLD_MAC_INTR,
    CPLD_PHY_INTR,
    CPLD_CPLDX_INTR,
    CPLD_MAC_THERMAL_INTR,
    CPLD_MISC_INTR,
    CPLD_CPU_INTR,
    CPLD_MAC_MASK,
    CPLD_PHY_MASK,
    CPLD_CPLDX_MASK,
    CPLD_MAC_THERMAL_MASK,
    CPLD_MISC_MASK,
    CPLD_CPU_MASK,
    CPLD_MAC_EVT,
    CPLD_PHY_EVT,
    CPLD_CPLDX_EVT,
    CPLD_MAC_THERMAL_EVT,
    CPLD_MISC_EVT,
    CPLD_MAC_RESET,
    CPLD_BMC_RESET,
    CPLD_USB_RESET,
    CPLD_MISC_RESET,
    CPLD_BRD_PRESENT,
    CPLD_PSU_STATUS,
    CPLD_SYSTEM_PWR,
    CPLD_MAC_SYNCE,
    CPLD_MAC_ROV,
    CPLD_MUX_CTRL,
    CPLD_SYSTEM_LED_SYS,
    CPLD_SYSTEM_LED_FAN,
    CPLD_SYSTEM_LED_PSU_0,
    CPLD_SYSTEM_LED_PSU_1,
    CPLD_SYSTEM_LED_SYNC,
    CPLD_SYSTEM_LED_ID,
    CPLD_MGMT_PORT_0_LED_STATUS,
    CPLD_MGMT_PORT_0_LED_SPEED,
    CPLD_MGMT_PORT_1_LED_STATUS,
    CPLD_MGMT_PORT_1_LED_SPEED,
    CPLD_PORT_LED_CLR,
    CPLD_MISC_PWR,
    DBG_CPLD_MAC_INTR,
    DBG_CPLD_CPLDX_INTR,
    DBG_CPLD_MAC_THERMAL_INTR,
    DBG_CPLD_MISC_INTR,

    //CPLD 2 and CPLD 3
    CPLD_OSFP_PORT_0_7_16_23_INTR,
    CPLD_OSFP_PORT_8_15_24_31_INTR,
    CPLD_OSFP_PORT_32_39_48_55_INTR,
    CPLD_OSFP_PORT_40_47_56_63_INTR,
    CPLD_OSFP_PORT_0_7_16_23_PRES,
    CPLD_OSFP_PORT_8_15_24_31_PRES,
    CPLD_OSFP_PORT_32_39_48_55_PRES,
    CPLD_OSFP_PORT_40_47_56_63_PRES,
    CPLD_OSFP_PORT_0_15_16_31_FUSE,
    CPLD_OSFP_PORT_32_47_48_63_FUSE,
    CPLD_OSFP_PORT_0_7_16_23_STUCK,
    CPLD_OSFP_PORT_8_15_24_31_STUCK,
    CPLD_OSFP_PORT_32_39_48_55_STUCK,
    CPLD_OSFP_PORT_40_47_56_63_STUCK,
    CPLD_OSFP_PORT_0_7_16_23_INTR_MASK,
    CPLD_OSFP_PORT_8_15_24_31_INTR_MASK,
    CPLD_OSFP_PORT_32_39_48_55_INTR_MASK,
    CPLD_OSFP_PORT_40_47_56_63_INTR_MASK,
    CPLD_OSFP_PORT_0_7_16_23_PRES_MASK,
    CPLD_OSFP_PORT_8_15_24_31_PRES_MASK,
    CPLD_OSFP_PORT_32_39_48_55_PRES_MASK,
    CPLD_OSFP_PORT_40_47_56_63_PRES_MASK,
    CPLD_OSFP_PORT_0_15_16_31_FUSE_MASK,
    CPLD_OSFP_PORT_32_47_48_63_FUSE_MASK,
    CPLD_OSFP_PORT_0_7_16_23_STUCK_MASK,
    CPLD_OSFP_PORT_8_15_24_31_STUCK_MASK,
    CPLD_OSFP_PORT_32_39_48_55_STUCK_MASK,
    CPLD_OSFP_PORT_40_47_56_63_STUCK_MASK,
    CPLD_OSFP_PORT_0_7_16_23_INTR_EVENT,
    CPLD_OSFP_PORT_8_15_24_31_INTR_EVENT,
    CPLD_OSFP_PORT_32_39_48_55_INTR_EVENT,
    CPLD_OSFP_PORT_40_47_56_63_INTR_EVENT,
    CPLD_OSFP_PORT_0_7_16_23_PRES_EVENT,
    CPLD_OSFP_PORT_8_15_24_31_PRES_EVENT,
    CPLD_OSFP_PORT_32_39_48_55_PRES_EVENT,
    CPLD_OSFP_PORT_40_47_56_63_PRES_EVENT,
    CPLD_OSFP_PORT_0_15_16_31_FUSE_EVENT,
    CPLD_OSFP_PORT_32_47_48_63_FUSE_EVENT,
    CPLD_OSFP_PORT_0_7_16_23_STUCK_EVENT,
    CPLD_OSFP_PORT_8_15_24_31_STUCK_EVENT,
    CPLD_OSFP_PORT_32_39_48_55_STUCK_EVENT,
    CPLD_OSFP_PORT_40_47_56_63_STUCK_EVENT,
    CPLD_OSFP_PORT_0_7_16_23_RST,
    CPLD_OSFP_PORT_8_15_24_31_RST,
    CPLD_OSFP_PORT_32_39_48_55_RST,
    CPLD_OSFP_PORT_40_47_56_63_RST,
    CPLD_OSFP_PORT_0_7_16_23_LPMODE,
    CPLD_OSFP_PORT_8_15_24_31_LPMODE,
    CPLD_OSFP_PORT_32_39_48_55_LPMODE,
    CPLD_OSFP_PORT_40_47_56_63_LPMODE,
    CPLD_I2C_CONTROL,
    CPLD_I2C_RELAY,
    CPLD_DBG_OSFP_PORT_0_7_16_23_INTR,
    CPLD_DBG_OSFP_PORT_8_15_24_31_INTR,
    CPLD_DBG_OSFP_PORT_32_39_48_55_INTR,
    CPLD_DBG_OSFP_PORT_40_47_56_63_INTR,
    CPLD_DBG_OSFP_PORT_0_7_16_23_PRES,
    CPLD_DBG_OSFP_PORT_8_15_24_31_PRES,
    CPLD_DBG_OSFP_PORT_32_39_48_55_PRES,
    CPLD_DBG_OSFP_PORT_40_47_56_63_PRES,
    CPLD_DBG_OSFP_PORT_0_15_16_31_FUSE,
    CPLD_DBG_OSFP_PORT_32_47_48_63_FUSE,

    //FPGA
    FPGA_MINOR_VER,
    FPGA_MAJOR_VER,
    FPGA_BUILD,
    FPGA_VERSION_H,
    FPGA_CHIP,
    FPGA_MGMT_PORT_0_1_TX_RATE_SEL,
    FPGA_MGMT_PORT_0_1_RX_RATE_SEL,
    FPGA_MGMT_PORT_0_1_TX_DIS,
    FPGA_MGMT_PORT_0_1_TX_FAULT,
    FPGA_MGMT_PORT_0_1_RX_LOS,
    FPGA_MGMT_PORT_0_1_PRES,
    FPGA_MGMT_PORT_0_1_STUCK,
    FPGA_MGMT_PORT_0_1_TX_FAULT_MASK,
    FPGA_MGMT_PORT_0_1_RX_LOS_MASK,
    FPGA_MGMT_PORT_0_1_PRES_MASK,
    FPGA_MGMT_PORT_0_1_STUCK_MASK,
    FPGA_MGMT_PORT_0_1_TX_FAULT_EVENT,
    FPGA_MGMT_PORT_0_1_RX_LOS_EVENT,
    FPGA_MGMT_PORT_0_1_PRES_EVENT,
    FPGA_MGMT_PORT_0_1_STUCK_EVENT,
    FPGA_EVT_CTRL,
    FPGA_LAN_PORT_RELAY,

    //MUX
    IDLE_STATE,

    //BSP DEBUG
    BSP_DEBUG
};

enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_UNK,
};

typedef struct  {
    u16 reg;
    u8 mask;
    u8 data_type;
} attr_reg_map_t;

static attr_reg_map_t attr_reg[]= {

    // CPLD Common
    [CPLD_MINOR_VER]                            = {CPLD_VERSION_REG                              , MASK_0011_1111, DATA_DEC},
    [CPLD_MAJOR_VER]                            = {CPLD_VERSION_REG                              , MASK_1100_0000, DATA_DEC},
    [CPLD_ID]                                   = {CPLD_ID_REG                                   , MASK_ALL      , DATA_DEC},
    [CPLD_BUILD_VER]                            = {CPLD_BUILD_REG                                , MASK_ALL      , DATA_DEC},
    [CPLD_VERSION_H]                            = {NONE_REG                                      , MASK_NONE     , DATA_UNK},
    [CPLD_EVT_CTRL]                             = {CPLD_EVT_CTRL_REG                             , MASK_ALL      , DATA_HEX},

    // CPLD 1
    [CPLD_BOARD_ID_0]                           = {CPLD_BOARD_ID_0_REG                           , MASK_ALL      , DATA_HEX},
    [CPLD_BOARD_ID_1]                           = {CPLD_BOARD_ID_1_REG                           , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_INTR]                             = {CPLD_MAC_INTR_REG                             , MASK_ALL      , DATA_HEX},
    [CPLD_PHY_INTR]                             = {CPLD_PHY_INTR_REG                             , MASK_ALL      , DATA_HEX},
    [CPLD_CPLDX_INTR]                           = {CPLD_CPLDX_INTR_REG                           , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_THERMAL_INTR]                     = {CPLD_THERMAL_INTR_REG                         , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_INTR]                            = {CPLD_MISC_INTR_REG                            , MASK_ALL      , DATA_HEX},
    [CPLD_CPU_INTR]                             = {CPLD_CPU_INTR_REG                             , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_MASK]                             = {CPLD_MAC_MASK_REG                             , MASK_ALL      , DATA_HEX},
    [CPLD_PHY_MASK]                             = {CPLD_PHY_MASK_REG                             , MASK_ALL      , DATA_HEX},
    [CPLD_CPLDX_MASK]                           = {CPLD_CPLDX_MASK_REG                           , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_THERMAL_MASK]                     = {CPLD_THERMAL_MASK_REG                         , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_MASK]                            = {CPLD_MISC_MASK_REG                            , MASK_ALL      , DATA_HEX},
    [CPLD_CPU_MASK]                             = {CPLD_CPU_MASK_REG                             , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_EVT]                              = {CPLD_MAC_EVT_REG                              , MASK_ALL      , DATA_HEX},
    [CPLD_PHY_EVT]                              = {CPLD_PHY_EVT_REG                              , MASK_ALL      , DATA_HEX},
    [CPLD_CPLDX_EVT]                            = {CPLD_CPLDX_EVT_REG                            , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_THERMAL_EVT]                      = {CPLD_THERMAL_EVT_REG                          , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_EVT]                             = {CPLD_MISC_EVT_REG                             , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_RESET]                            = {CPLD_MAC_RESET_REG                            , MASK_ALL      , DATA_HEX},
    [CPLD_BMC_RESET]                            = {CPLD_BMC_RESET_REG                            , MASK_ALL      , DATA_HEX},
    [CPLD_USB_RESET]                            = {CPLD_USB_RESET_REG                            , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_RESET]                           = {CPLD_MISC_RESET_REG                           , MASK_ALL      , DATA_HEX},
    [CPLD_BRD_PRESENT]                          = {CPLD_BRD_PRESENT_REG                          , MASK_ALL      , DATA_HEX},
    [CPLD_PSU_STATUS]                           = {CPLD_PSU_STATUS_REG                           , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_PWR]                           = {CPLD_SYSTEM_PWR_REG                           , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_SYNCE]                            = {CPLD_MAC_SYNCE_REG                            , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_ROV]                              = {CPLD_MAC_ROV_REG                              , MASK_ALL      , DATA_HEX},
    [CPLD_MUX_CTRL]                             = {CPLD_MUX_CTRL_REG                             , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_LED_SYS]                       = {CPLD_SYSTEM_LED_SYS_FAN_REG                   , MASK_0000_1111, DATA_HEX},
    [CPLD_SYSTEM_LED_FAN]                       = {CPLD_SYSTEM_LED_SYS_FAN_REG                   , MASK_1111_0000, DATA_HEX},
    [CPLD_SYSTEM_LED_PSU_0]                     = {CPLD_SYSTEM_LED_PSU_REG                       , MASK_0000_1111, DATA_HEX},
    [CPLD_SYSTEM_LED_PSU_1]                     = {CPLD_SYSTEM_LED_PSU_REG                       , MASK_1111_0000, DATA_HEX},
    [CPLD_SYSTEM_LED_SYNC]                      = {CPLD_SYSTEM_LED_SYNC_ID_REG                   , MASK_0000_1111, DATA_HEX},
    [CPLD_SYSTEM_LED_ID]                        = {CPLD_SYSTEM_LED_SYNC_ID_REG                   , MASK_1110_0000, DATA_HEX},
    [CPLD_MGMT_PORT_0_LED_STATUS]               = {CPLD_SFP_PORT_0_1_LED_REG                     , MASK_0000_1101, DATA_DEC},
    [CPLD_MGMT_PORT_0_LED_SPEED]                = {CPLD_SFP_PORT_0_1_LED_REG                     , MASK_0000_0010, DATA_DEC},
    [CPLD_MGMT_PORT_1_LED_STATUS]               = {CPLD_SFP_PORT_0_1_LED_REG                     , MASK_1101_0000, DATA_DEC},
    [CPLD_MGMT_PORT_1_LED_SPEED]                = {CPLD_SFP_PORT_0_1_LED_REG                     , MASK_0010_0000, DATA_DEC},
    [CPLD_PORT_LED_CLR]                         = {CPLD_PORT_LED_CLR_REG                         , MASK_0000_0001, DATA_DEC},
    [CPLD_MISC_PWR]                             = {CPLD_MISC_PWR_REG                             , MASK_ALL      , DATA_HEX},
    [DBG_CPLD_MAC_INTR]                         = {DBG_CPLD_MAC_INTR_REG                         , MASK_ALL      , DATA_HEX},
    [DBG_CPLD_CPLDX_INTR]                       = {DBG_CPLD_CPLDX_INTR_REG                       , MASK_ALL      , DATA_HEX},
    [DBG_CPLD_MAC_THERMAL_INTR]                 = {DBG_CPLD_THERMAL_INTR_REG                     , MASK_ALL      , DATA_HEX},
    [DBG_CPLD_MISC_INTR]                        = {DBG_CPLD_MISC_INTR_REG                        , MASK_ALL      , DATA_HEX},

    // CPLD 2
    [CPLD_OSFP_PORT_0_7_16_23_INTR]             = {CPLD_OSFP_PORT_0_7_16_23_INTR_REG             , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_INTR]            = {CPLD_OSFP_PORT_8_15_24_31_INTR_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_INTR]           = {CPLD_OSFP_PORT_32_39_48_55_INTR_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_INTR]           = {CPLD_OSFP_PORT_40_47_56_63_INTR_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_PRES]             = {CPLD_OSFP_PORT_0_7_16_23_PRES_REG             , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_PRES]            = {CPLD_OSFP_PORT_8_15_24_31_PRES_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_PRES]           = {CPLD_OSFP_PORT_32_39_48_55_PRES_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_PRES]           = {CPLD_OSFP_PORT_40_47_56_63_PRES_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_15_16_31_FUSE]            = {CPLD_OSFP_PORT_0_15_16_31_FUSE_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_47_48_63_FUSE]           = {CPLD_OSFP_PORT_32_47_48_63_FUSE_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_STUCK]            = {CPLD_OSFP_PORT_0_7_16_23_STUCK_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_STUCK]           = {CPLD_OSFP_PORT_8_15_24_31_STUCK_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_STUCK]          = {CPLD_OSFP_PORT_32_39_48_55_STUCK_REG          , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_STUCK]          = {CPLD_OSFP_PORT_40_47_56_63_STUCK_REG          , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_INTR_MASK]        = {CPLD_OSFP_PORT_0_7_16_23_INTR_MASK_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_INTR_MASK]       = {CPLD_OSFP_PORT_8_15_24_31_INTR_MASK_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_INTR_MASK]      = {CPLD_OSFP_PORT_32_39_48_55_INTR_MASK_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_INTR_MASK]      = {CPLD_OSFP_PORT_40_47_56_63_INTR_MASK_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_PRES_MASK]        = {CPLD_OSFP_PORT_0_7_16_23_PRES_MASK_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_PRES_MASK]       = {CPLD_OSFP_PORT_8_15_24_31_PRES_MASK_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_PRES_MASK]      = {CPLD_OSFP_PORT_32_39_48_55_PRES_MASK_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_PRES_MASK]      = {CPLD_OSFP_PORT_40_47_56_63_PRES_MASK_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_15_16_31_FUSE_MASK]       = {CPLD_OSFP_PORT_0_15_16_31_FUSE_MASK_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_47_48_63_FUSE_MASK]      = {CPLD_OSFP_PORT_32_47_48_63_FUSE_MASK_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_STUCK_MASK]       = {CPLD_OSFP_PORT_0_7_16_23_STUCK_MASK_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_STUCK_MASK]      = {CPLD_OSFP_PORT_8_15_24_31_STUCK_MASK_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_STUCK_MASK]     = {CPLD_OSFP_PORT_32_39_48_55_STUCK_MASK_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_STUCK_MASK]     = {CPLD_OSFP_PORT_40_47_56_63_STUCK_MASK_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_INTR_EVENT]       = {CPLD_OSFP_PORT_0_7_16_23_INTR_EVENT_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_INTR_EVENT]      = {CPLD_OSFP_PORT_8_15_24_31_INTR_EVENT_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_INTR_EVENT]     = {CPLD_OSFP_PORT_32_39_48_55_INTR_EVENT_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_INTR_EVENT]     = {CPLD_OSFP_PORT_40_47_56_63_INTR_EVENT_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_PRES_EVENT]       = {CPLD_OSFP_PORT_0_7_16_23_PRES_EVENT_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_PRES_EVENT]      = {CPLD_OSFP_PORT_8_15_24_31_PRES_EVENT_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_PRES_EVENT]     = {CPLD_OSFP_PORT_32_39_48_55_PRES_EVENT_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_PRES_EVENT]     = {CPLD_OSFP_PORT_40_47_56_63_PRES_EVENT_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_15_16_31_FUSE_EVENT]      = {CPLD_OSFP_PORT_0_15_16_31_FUSE_EVENT_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_47_48_63_FUSE_EVENT]     = {CPLD_OSFP_PORT_32_47_48_63_FUSE_EVENT_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_STUCK_EVENT]      = {CPLD_OSFP_PORT_0_7_16_23_STUCK_EVENT_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_STUCK_EVENT]     = {CPLD_OSFP_PORT_8_15_24_31_STUCK_EVENT_REG     , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_STUCK_EVENT]    = {CPLD_OSFP_PORT_32_39_48_55_STUCK_EVENT_REG    , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_STUCK_EVENT]    = {CPLD_OSFP_PORT_40_47_56_63_STUCK_EVENT_REG    , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_RST]              = {CPLD_OSFP_PORT_0_7_16_23_RST_REG              , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_RST]             = {CPLD_OSFP_PORT_8_15_24_31_RST_REG             , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_RST]            = {CPLD_OSFP_PORT_32_39_48_55_RST_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_RST]            = {CPLD_OSFP_PORT_40_47_56_63_RST_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_0_7_16_23_LPMODE]           = {CPLD_OSFP_PORT_0_7_16_23_LPMODE_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_8_15_24_31_LPMODE]          = {CPLD_OSFP_PORT_8_15_24_31_LPMODE_REG          , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_32_39_48_55_LPMODE]         = {CPLD_OSFP_PORT_32_39_48_55_LPMODE_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_OSFP_PORT_40_47_56_63_LPMODE]         = {CPLD_OSFP_PORT_40_47_56_63_LPMODE_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_I2C_CONTROL]                          = {CPLD_I2C_CONTROL_REG                          , MASK_ALL      , DATA_HEX},
    [CPLD_I2C_RELAY]                            = {CPLD_I2C_RELAY_REG                            , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_0_7_16_23_INTR]         = {CPLD_DBG_OSFP_PORT_0_7_16_23_INTR_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_8_15_24_31_INTR]        = {CPLD_DBG_OSFP_PORT_8_15_24_31_INTR_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_32_39_48_55_INTR]       = {CPLD_DBG_OSFP_PORT_32_39_48_55_INTR_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_40_47_56_63_INTR]       = {CPLD_DBG_OSFP_PORT_40_47_56_63_INTR_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_0_7_16_23_PRES]         = {CPLD_DBG_OSFP_PORT_0_7_16_23_PRES_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_8_15_24_31_PRES]        = {CPLD_DBG_OSFP_PORT_8_15_24_31_PRES_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_32_39_48_55_PRES]       = {CPLD_DBG_OSFP_PORT_32_39_48_55_PRES_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_40_47_56_63_PRES]       = {CPLD_DBG_OSFP_PORT_40_47_56_63_PRES_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_0_15_16_31_FUSE]        = {CPLD_DBG_OSFP_PORT_0_15_16_31_FUSE_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_DBG_OSFP_PORT_32_47_48_63_FUSE]       = {CPLD_DBG_OSFP_PORT_32_47_48_63_FUSE_REG       , MASK_ALL      , DATA_HEX},

    // FPGA
    [FPGA_MINOR_VER]                            = {FPGA_VERSION_REG                              , MASK_0011_1111, DATA_DEC},
    [FPGA_MAJOR_VER]                            = {FPGA_VERSION_REG                              , MASK_1100_0000, DATA_DEC},
    [FPGA_BUILD]                                = {FPGA_BUILD_REG                                , MASK_ALL      , DATA_DEC},
    [FPGA_VERSION_H]                            = {NONE_REG                                      , MASK_NONE     , DATA_UNK},
    [FPGA_CHIP]                                 = {FPGA_CHIP_REG                                 , MASK_ALL      , DATA_DEC},
    [FPGA_MGMT_PORT_0_1_TX_RATE_SEL]            = {FPGA_MGMT_PORT_0_1_TX_RATE_SEL_REG            , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_RX_RATE_SEL]            = {FPGA_MGMT_PORT_0_1_RX_RATE_SEL_REG            , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_TX_DIS]                 = {FPGA_MGMT_PORT_0_1_TX_DIS_REG                 , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_TX_FAULT]               = {FPGA_MGMT_PORT_0_1_TX_FAULT_REG               , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_RX_LOS]                 = {FPGA_MGMT_PORT_0_1_RX_LOS_REG                 , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_PRES]                   = {FPGA_MGMT_PORT_0_1_PRES_REG                   , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_STUCK]                  = {FPGA_MGMT_PORT_0_1_STUCK_REG                  , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_TX_FAULT_MASK]          = {FPGA_MGMT_PORT_0_1_TX_FAULT_MASK_REG          , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_RX_LOS_MASK]            = {FPGA_MGMT_PORT_0_1_RX_LOS_MASK_REG            , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_PRES_MASK]              = {FPGA_MGMT_PORT_0_1_PRES_MASK_REG              , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_STUCK_MASK]             = {FPGA_MGMT_PORT_0_1_STUCK_MASK_REG             , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_TX_FAULT_EVENT]         = {FPGA_MGMT_PORT_0_1_TX_FAULT_EVENT_REG         , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_RX_LOS_EVENT]           = {FPGA_MGMT_PORT_0_1_RX_LOS_EVENT_REG           , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_PRES_EVENT]             = {FPGA_MGMT_PORT_0_1_PRES_EVENT_REG             , MASK_ALL      , DATA_HEX},
    [FPGA_MGMT_PORT_0_1_STUCK_EVENT]            = {FPGA_MGMT_PORT_0_1_STUCK_EVENT_REG            , MASK_ALL      , DATA_HEX},
    [FPGA_EVT_CTRL]                             = {FPGA_EVT_CTRL_REG                             , MASK_ALL      , DATA_HEX},
    [FPGA_LAN_PORT_RELAY]                       = {FPGA_LAN_PORT_RELAY_REG                       , MASK_ALL      , DATA_HEX},

    // MUX
    [IDLE_STATE]                                = {NONE_REG                                      , MASK_NONE     , DATA_UNK},

    //BSP DEBUG
    [BSP_DEBUG]                                 = {NONE_REG                                      , MASK_NONE     , DATA_UNK},
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
static ssize_t cpld_reg_read(struct device *dev, u8 *reg_val, u8 reg, u8 mask);
static ssize_t cpld_reg_write(struct device *dev, u8 reg_val, size_t count, u8 reg, u8 mask);
static ssize_t bsp_read(char *buf, char *str);
static ssize_t bsp_write(const char *buf, char *str, size_t str_len, size_t count);
static ssize_t bsp_callback_show(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t bsp_callback_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t version_h_show(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t led_show(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t led_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);

static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

/* CPLD device id and data */
static const struct i2c_device_id cpld_id[] = {
    { "s9321_64eo_cpld1",  cpld1 },
    { "s9321_64eo_cpld2",  cpld2 },
    { "s9321_64eo_cpld3",  cpld3 },
    { "s9321_64eo_fpga",   fpga },
    {}
};

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, 0x37, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

// CPLD Common
static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver                , cpld, CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver                , cpld, CPLD_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_id                       , cpld, CPLD_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_build_ver                , cpld, CPLD_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_version_h                , version_h, CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR_RW(cpld_evt_ctrl                 , cpld, CPLD_EVT_CTRL);

// CPLD 1
static SENSOR_DEVICE_ATTR_RO(cpld_board_id_0               , cpld, CPLD_BOARD_ID_0);
static SENSOR_DEVICE_ATTR_RO(cpld_board_id_1               , cpld, CPLD_BOARD_ID_1);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_intr                 , cpld, CPLD_MAC_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_phy_intr                 , cpld, CPLD_PHY_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_cpldx_intr               , cpld, CPLD_CPLDX_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_thermal_intr         , cpld, CPLD_MAC_THERMAL_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_misc_intr                , cpld, CPLD_MISC_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_cpu_intr                 , cpld, CPLD_CPU_INTR);
static SENSOR_DEVICE_ATTR_RW(cpld_mac_mask                 , cpld, CPLD_MAC_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_phy_mask                 , cpld, CPLD_PHY_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_cpldx_mask               , cpld, CPLD_CPLDX_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_mac_thermal_mask         , cpld, CPLD_MAC_THERMAL_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_misc_mask                , cpld, CPLD_MISC_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_cpu_mask                 , cpld, CPLD_CPU_MASK);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_evt                  , cpld, CPLD_MAC_EVT);
static SENSOR_DEVICE_ATTR_RO(cpld_phy_evt                  , cpld, CPLD_PHY_EVT);
static SENSOR_DEVICE_ATTR_RO(cpld_cpldx_evt                , cpld, CPLD_CPLDX_EVT);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_thermal_evt          , cpld, CPLD_MAC_THERMAL_EVT);
static SENSOR_DEVICE_ATTR_RO(cpld_misc_evt                 , cpld, CPLD_MISC_EVT);
static SENSOR_DEVICE_ATTR_RW(cpld_mac_reset                , cpld, CPLD_MAC_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_bmc_reset                , cpld, CPLD_BMC_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_usb_reset                , cpld, CPLD_USB_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_misc_reset               , cpld, CPLD_MISC_RESET);
static SENSOR_DEVICE_ATTR_RO(cpld_brd_present              , cpld, CPLD_BRD_PRESENT);
static SENSOR_DEVICE_ATTR_RO(cpld_psu_status               , cpld, CPLD_PSU_STATUS);
static SENSOR_DEVICE_ATTR_RO(cpld_system_pwr               , cpld, CPLD_SYSTEM_PWR);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_synce                , cpld, CPLD_MAC_SYNCE);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_rov                  , cpld, CPLD_MAC_ROV);
static SENSOR_DEVICE_ATTR_RW(cpld_mux_ctrl                 , cpld, CPLD_MUX_CTRL);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sys           , cpld, CPLD_SYSTEM_LED_SYS);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_fan           , cpld, CPLD_SYSTEM_LED_FAN);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_psu_0         , cpld, CPLD_SYSTEM_LED_PSU_0);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_psu_1         , cpld, CPLD_SYSTEM_LED_PSU_1);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sync          , cpld, CPLD_SYSTEM_LED_SYNC);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_id            , cpld, CPLD_SYSTEM_LED_ID);
static SENSOR_DEVICE_ATTR_RW(cpld_mgmt_port_0_led_status   , led , CPLD_MGMT_PORT_0_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(cpld_mgmt_port_0_led_speed    , cpld, CPLD_MGMT_PORT_0_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(cpld_mgmt_port_1_led_status   , led , CPLD_MGMT_PORT_1_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(cpld_mgmt_port_1_led_speed    , cpld, CPLD_MGMT_PORT_1_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(cpld_port_led_clr             , cpld, CPLD_PORT_LED_CLR);
static SENSOR_DEVICE_ATTR_RO(cpld_misc_pwr                 , cpld, CPLD_MISC_PWR);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_mac_intr             , cpld, DBG_CPLD_MAC_INTR);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_cpldx_intr           , cpld, DBG_CPLD_CPLDX_INTR);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_mac_thermal_intr     , cpld, DBG_CPLD_MAC_THERMAL_INTR);
static SENSOR_DEVICE_ATTR_RO(dbg_cpld_misc_intr            , cpld, DBG_CPLD_MISC_INTR);

//CPLD 2 and CPLD 3
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_port_0         , cpld, CPLD_OSFP_PORT_0_7_16_23_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_port_1         , cpld, CPLD_OSFP_PORT_8_15_24_31_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_port_2         , cpld, CPLD_OSFP_PORT_32_39_48_55_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_port_3         , cpld, CPLD_OSFP_PORT_40_47_56_63_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_present_0      , cpld, CPLD_OSFP_PORT_0_7_16_23_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_present_1      , cpld, CPLD_OSFP_PORT_8_15_24_31_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_present_2      , cpld, CPLD_OSFP_PORT_32_39_48_55_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_present_3      , cpld, CPLD_OSFP_PORT_40_47_56_63_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_fuse_0         , cpld, CPLD_OSFP_PORT_0_15_16_31_FUSE);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_intr_fuse_1         , cpld, CPLD_OSFP_PORT_32_47_48_63_FUSE);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_stuck_0             , cpld, CPLD_OSFP_PORT_0_7_16_23_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_stuck_1             , cpld, CPLD_OSFP_PORT_8_15_24_31_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_stuck_2             , cpld, CPLD_OSFP_PORT_32_39_48_55_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_stuck_3             , cpld, CPLD_OSFP_PORT_40_47_56_63_STUCK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_port_0         , cpld, CPLD_OSFP_PORT_0_7_16_23_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_port_1         , cpld, CPLD_OSFP_PORT_8_15_24_31_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_port_2         , cpld, CPLD_OSFP_PORT_32_39_48_55_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_port_3         , cpld, CPLD_OSFP_PORT_40_47_56_63_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_present_0      , cpld, CPLD_OSFP_PORT_0_7_16_23_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_present_1      , cpld, CPLD_OSFP_PORT_8_15_24_31_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_present_2      , cpld, CPLD_OSFP_PORT_32_39_48_55_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_present_3      , cpld, CPLD_OSFP_PORT_40_47_56_63_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_fuse_0         , cpld, CPLD_OSFP_PORT_0_15_16_31_FUSE_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_fuse_1         , cpld, CPLD_OSFP_PORT_32_47_48_63_FUSE_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_stuck_0        , cpld, CPLD_OSFP_PORT_0_7_16_23_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_stuck_1        , cpld, CPLD_OSFP_PORT_8_15_24_31_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_stuck_2        , cpld, CPLD_OSFP_PORT_32_39_48_55_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_mask_stuck_3        , cpld, CPLD_OSFP_PORT_40_47_56_63_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_port_0          , cpld, CPLD_OSFP_PORT_0_7_16_23_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_port_1          , cpld, CPLD_OSFP_PORT_8_15_24_31_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_port_2          , cpld, CPLD_OSFP_PORT_32_39_48_55_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_port_3          , cpld, CPLD_OSFP_PORT_40_47_56_63_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_present_0       , cpld, CPLD_OSFP_PORT_0_7_16_23_PRES_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_present_1       , cpld, CPLD_OSFP_PORT_8_15_24_31_PRES_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_present_2       , cpld, CPLD_OSFP_PORT_32_39_48_55_PRES_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_present_3       , cpld, CPLD_OSFP_PORT_40_47_56_63_PRES_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_fuse_0          , cpld, CPLD_OSFP_PORT_0_15_16_31_FUSE_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_fuse_1          , cpld, CPLD_OSFP_PORT_32_47_48_63_FUSE_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_stuck_0         , cpld, CPLD_OSFP_PORT_0_7_16_23_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_stuck_1         , cpld, CPLD_OSFP_PORT_8_15_24_31_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_stuck_2         , cpld, CPLD_OSFP_PORT_32_39_48_55_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_osfp_evt_stuck_3         , cpld, CPLD_OSFP_PORT_40_47_56_63_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_reset_0             , cpld, CPLD_OSFP_PORT_0_7_16_23_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_reset_1             , cpld, CPLD_OSFP_PORT_8_15_24_31_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_reset_2             , cpld, CPLD_OSFP_PORT_32_39_48_55_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_reset_3             , cpld, CPLD_OSFP_PORT_40_47_56_63_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_lpmode_0            , cpld, CPLD_OSFP_PORT_0_7_16_23_LPMODE);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_lpmode_1            , cpld, CPLD_OSFP_PORT_8_15_24_31_LPMODE);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_lpmode_2            , cpld, CPLD_OSFP_PORT_32_39_48_55_LPMODE);
static SENSOR_DEVICE_ATTR_RW(cpld_osfp_lpmode_3            , cpld, CPLD_OSFP_PORT_40_47_56_63_LPMODE);
static SENSOR_DEVICE_ATTR_RW(cpld_i2c_ctrl                 , cpld, CPLD_I2C_CONTROL);
static SENSOR_DEVICE_ATTR_RW(cpld_i2c_relay                , cpld, CPLD_I2C_RELAY);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_port_0     , cpld, CPLD_DBG_OSFP_PORT_0_7_16_23_INTR);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_port_1     , cpld, CPLD_DBG_OSFP_PORT_8_15_24_31_INTR);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_port_2     , cpld, CPLD_DBG_OSFP_PORT_32_39_48_55_INTR);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_port_3     , cpld, CPLD_DBG_OSFP_PORT_40_47_56_63_INTR);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_present_0  , cpld, CPLD_DBG_OSFP_PORT_0_7_16_23_PRES);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_present_1  , cpld, CPLD_DBG_OSFP_PORT_8_15_24_31_PRES);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_present_2  , cpld, CPLD_DBG_OSFP_PORT_32_39_48_55_PRES);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_present_3  , cpld, CPLD_DBG_OSFP_PORT_40_47_56_63_PRES);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_fuse_0     , cpld, CPLD_DBG_OSFP_PORT_0_15_16_31_FUSE);
static SENSOR_DEVICE_ATTR_RW(dbg_cpld_osfp_intr_fuse_1     , cpld, CPLD_DBG_OSFP_PORT_32_47_48_63_FUSE);

// FPGA
static SENSOR_DEVICE_ATTR_RO(fpga_minor_ver                , cpld, FPGA_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_major_ver                , cpld, FPGA_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_build_ver                , cpld, FPGA_BUILD);
static SENSOR_DEVICE_ATTR_RO(fpga_version_h                , version_h, FPGA_VERSION_H);
static SENSOR_DEVICE_ATTR_RO(fpga_id                       , cpld, FPGA_CHIP);
static SENSOR_DEVICE_ATTR_RW(fpga_sfp28_tx_rate_cap        , cpld, FPGA_MGMT_PORT_0_1_TX_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(fpga_sfp28_rx_rate_cap        , cpld, FPGA_MGMT_PORT_0_1_RX_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(fpga_sfp28_tx_dis             , cpld, FPGA_MGMT_PORT_0_1_TX_DIS);
static SENSOR_DEVICE_ATTR_RO(fpga_sfp28_tx_fault           , cpld, FPGA_MGMT_PORT_0_1_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(fpga_sfp28_rx_los             , cpld, FPGA_MGMT_PORT_0_1_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(fpga_sfp28_intr_present       , cpld, FPGA_MGMT_PORT_0_1_PRES);
static SENSOR_DEVICE_ATTR_RO(fpga_sfp28_stuck              , cpld, FPGA_MGMT_PORT_0_1_STUCK);
static SENSOR_DEVICE_ATTR_RW(fpga_sfp28_mask_tx_fault      , cpld, FPGA_MGMT_PORT_0_1_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(fpga_sfp28_mask_rx_los        , cpld, FPGA_MGMT_PORT_0_1_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(fpga_sfp28_mask_present       , cpld, FPGA_MGMT_PORT_0_1_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(fpga_sfp28_mask_stuck         , cpld, FPGA_MGMT_PORT_0_1_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RO(fpga_sfp28_evt_tx_fault       , cpld, FPGA_MGMT_PORT_0_1_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(fpga_sfp28_evt_rx_los         , cpld, FPGA_MGMT_PORT_0_1_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(fpga_sfp28_evt_present        , cpld, FPGA_MGMT_PORT_0_1_PRES_EVENT);
static SENSOR_DEVICE_ATTR_RO(fpga_sfp28_evt_stuck          , cpld, FPGA_MGMT_PORT_0_1_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RW(fpga_evt_ctrl                 , cpld, FPGA_EVT_CTRL);
static SENSOR_DEVICE_ATTR_RO(fpga_lan_port_relay           , cpld, FPGA_LAN_PORT_RELAY);

//BSP DEBUG
static SENSOR_DEVICE_ATTR_RW(bsp_debug, bsp_callback, BSP_DEBUG);

//MUX
static SENSOR_DEVICE_ATTR_RW(idle_state, idle_state, IDLE_STATE);

/* define support attributes of cpldx */

/* cpld 1 */
static struct attribute *cpld1_attributes[] = {
     // CPLD Common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_evt_ctrl),

    // CPLD 1
    _DEVICE_ATTR(cpld_board_id_0),
    _DEVICE_ATTR(cpld_board_id_1),
    _DEVICE_ATTR(cpld_mac_intr),
    _DEVICE_ATTR(cpld_phy_intr),
    _DEVICE_ATTR(cpld_cpldx_intr),
    _DEVICE_ATTR(cpld_mac_thermal_intr),
    _DEVICE_ATTR(cpld_misc_intr),
    _DEVICE_ATTR(cpld_cpu_intr),
    _DEVICE_ATTR(cpld_mac_mask),
    _DEVICE_ATTR(cpld_phy_mask),
    _DEVICE_ATTR(cpld_cpldx_mask),
    _DEVICE_ATTR(cpld_mac_thermal_mask),
    _DEVICE_ATTR(cpld_misc_mask),
    _DEVICE_ATTR(cpld_cpu_mask),
    _DEVICE_ATTR(cpld_mac_evt),
    _DEVICE_ATTR(cpld_phy_evt),
    _DEVICE_ATTR(cpld_cpldx_evt),
    _DEVICE_ATTR(cpld_mac_thermal_evt),
    _DEVICE_ATTR(cpld_misc_evt),
    _DEVICE_ATTR(cpld_mac_reset),
    _DEVICE_ATTR(cpld_bmc_reset),
    _DEVICE_ATTR(cpld_usb_reset),
    _DEVICE_ATTR(cpld_misc_reset),
    _DEVICE_ATTR(cpld_brd_present),
    _DEVICE_ATTR(cpld_psu_status),
    _DEVICE_ATTR(cpld_system_pwr),
    _DEVICE_ATTR(cpld_mac_synce),
    _DEVICE_ATTR(cpld_mac_rov),
    _DEVICE_ATTR(cpld_mux_ctrl),
    _DEVICE_ATTR(cpld_system_led_sys),
    _DEVICE_ATTR(cpld_system_led_fan),
    _DEVICE_ATTR(cpld_system_led_psu_0),
    _DEVICE_ATTR(cpld_system_led_psu_1),
    _DEVICE_ATTR(cpld_system_led_sync),
    _DEVICE_ATTR(cpld_system_led_id),
    _DEVICE_ATTR(cpld_mgmt_port_0_led_status),
    _DEVICE_ATTR(cpld_mgmt_port_0_led_speed),
    _DEVICE_ATTR(cpld_mgmt_port_1_led_status),
    _DEVICE_ATTR(cpld_mgmt_port_1_led_speed),
    _DEVICE_ATTR(cpld_port_led_clr),
    _DEVICE_ATTR(cpld_misc_pwr),
    _DEVICE_ATTR(dbg_cpld_mac_intr),
    _DEVICE_ATTR(dbg_cpld_cpldx_intr),
    _DEVICE_ATTR(dbg_cpld_mac_thermal_intr),
    _DEVICE_ATTR(dbg_cpld_misc_intr),
    _DEVICE_ATTR(bsp_debug),
    NULL
};

/* cpld 2 */
static struct attribute *cpld2_attributes[] = {
     // CPLD Common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_evt_ctrl),

    // CPLD 2
    _DEVICE_ATTR(cpld_osfp_intr_port_0),
    _DEVICE_ATTR(cpld_osfp_intr_port_1),
    _DEVICE_ATTR(cpld_osfp_intr_port_2),
    _DEVICE_ATTR(cpld_osfp_intr_port_3),
    _DEVICE_ATTR(cpld_osfp_intr_present_0),
    _DEVICE_ATTR(cpld_osfp_intr_present_1),
    _DEVICE_ATTR(cpld_osfp_intr_present_2),
    _DEVICE_ATTR(cpld_osfp_intr_present_3),
    _DEVICE_ATTR(cpld_osfp_intr_fuse_0),
    _DEVICE_ATTR(cpld_osfp_intr_fuse_1),
    _DEVICE_ATTR(cpld_osfp_stuck_0),
    _DEVICE_ATTR(cpld_osfp_stuck_1),
    _DEVICE_ATTR(cpld_osfp_stuck_2),
    _DEVICE_ATTR(cpld_osfp_stuck_3),
    _DEVICE_ATTR(cpld_osfp_mask_port_0),
    _DEVICE_ATTR(cpld_osfp_mask_port_1),
    _DEVICE_ATTR(cpld_osfp_mask_port_2),
    _DEVICE_ATTR(cpld_osfp_mask_port_3),
    _DEVICE_ATTR(cpld_osfp_mask_present_0),
    _DEVICE_ATTR(cpld_osfp_mask_present_1),
    _DEVICE_ATTR(cpld_osfp_mask_present_2),
    _DEVICE_ATTR(cpld_osfp_mask_present_3),
    _DEVICE_ATTR(cpld_osfp_mask_fuse_0),
    _DEVICE_ATTR(cpld_osfp_mask_fuse_1),
    _DEVICE_ATTR(cpld_osfp_mask_stuck_0),
    _DEVICE_ATTR(cpld_osfp_mask_stuck_1),
    _DEVICE_ATTR(cpld_osfp_mask_stuck_2),
    _DEVICE_ATTR(cpld_osfp_mask_stuck_3),
    _DEVICE_ATTR(cpld_osfp_evt_port_0),
    _DEVICE_ATTR(cpld_osfp_evt_port_1),
    _DEVICE_ATTR(cpld_osfp_evt_port_2),
    _DEVICE_ATTR(cpld_osfp_evt_port_3),
    _DEVICE_ATTR(cpld_osfp_evt_present_0),
    _DEVICE_ATTR(cpld_osfp_evt_present_1),
    _DEVICE_ATTR(cpld_osfp_evt_present_2),
    _DEVICE_ATTR(cpld_osfp_evt_present_3),
    _DEVICE_ATTR(cpld_osfp_evt_fuse_0),
    _DEVICE_ATTR(cpld_osfp_evt_fuse_1),
    _DEVICE_ATTR(cpld_osfp_evt_stuck_0),
    _DEVICE_ATTR(cpld_osfp_evt_stuck_1),
    _DEVICE_ATTR(cpld_osfp_evt_stuck_2),
    _DEVICE_ATTR(cpld_osfp_evt_stuck_3),
    _DEVICE_ATTR(cpld_osfp_reset_0),
    _DEVICE_ATTR(cpld_osfp_reset_1),
    _DEVICE_ATTR(cpld_osfp_reset_2),
    _DEVICE_ATTR(cpld_osfp_reset_3),
    _DEVICE_ATTR(cpld_osfp_lpmode_0),
    _DEVICE_ATTR(cpld_osfp_lpmode_1),
    _DEVICE_ATTR(cpld_osfp_lpmode_2),
    _DEVICE_ATTR(cpld_osfp_lpmode_3),
    _DEVICE_ATTR(cpld_i2c_ctrl),
    _DEVICE_ATTR(cpld_i2c_relay),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_port_0),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_port_1),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_port_2),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_port_3),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_present_0),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_present_1),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_present_2),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_present_3),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_fuse_0),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_fuse_1),
    NULL
};

/* cpld 3 */
static struct attribute *cpld3_attributes[] = {
     // CPLD Common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_evt_ctrl),

    // CPLD 3
    _DEVICE_ATTR(cpld_osfp_intr_port_0),
    _DEVICE_ATTR(cpld_osfp_intr_port_1),
    _DEVICE_ATTR(cpld_osfp_intr_port_2),
    _DEVICE_ATTR(cpld_osfp_intr_port_3),
    _DEVICE_ATTR(cpld_osfp_intr_present_0),
    _DEVICE_ATTR(cpld_osfp_intr_present_1),
    _DEVICE_ATTR(cpld_osfp_intr_present_2),
    _DEVICE_ATTR(cpld_osfp_intr_present_3),
    _DEVICE_ATTR(cpld_osfp_intr_fuse_0),
    _DEVICE_ATTR(cpld_osfp_intr_fuse_1),
    _DEVICE_ATTR(cpld_osfp_stuck_0),
    _DEVICE_ATTR(cpld_osfp_stuck_1),
    _DEVICE_ATTR(cpld_osfp_stuck_2),
    _DEVICE_ATTR(cpld_osfp_stuck_3),
    _DEVICE_ATTR(cpld_osfp_mask_port_0),
    _DEVICE_ATTR(cpld_osfp_mask_port_1),
    _DEVICE_ATTR(cpld_osfp_mask_port_2),
    _DEVICE_ATTR(cpld_osfp_mask_port_3),
    _DEVICE_ATTR(cpld_osfp_mask_present_0),
    _DEVICE_ATTR(cpld_osfp_mask_present_1),
    _DEVICE_ATTR(cpld_osfp_mask_present_2),
    _DEVICE_ATTR(cpld_osfp_mask_present_3),
    _DEVICE_ATTR(cpld_osfp_mask_fuse_0),
    _DEVICE_ATTR(cpld_osfp_mask_fuse_1),
    _DEVICE_ATTR(cpld_osfp_mask_stuck_0),
    _DEVICE_ATTR(cpld_osfp_mask_stuck_1),
    _DEVICE_ATTR(cpld_osfp_mask_stuck_2),
    _DEVICE_ATTR(cpld_osfp_mask_stuck_3),
    _DEVICE_ATTR(cpld_osfp_evt_port_0),
    _DEVICE_ATTR(cpld_osfp_evt_port_1),
    _DEVICE_ATTR(cpld_osfp_evt_port_2),
    _DEVICE_ATTR(cpld_osfp_evt_port_3),
    _DEVICE_ATTR(cpld_osfp_evt_present_0),
    _DEVICE_ATTR(cpld_osfp_evt_present_1),
    _DEVICE_ATTR(cpld_osfp_evt_present_2),
    _DEVICE_ATTR(cpld_osfp_evt_present_3),
    _DEVICE_ATTR(cpld_osfp_evt_fuse_0),
    _DEVICE_ATTR(cpld_osfp_evt_fuse_1),
    _DEVICE_ATTR(cpld_osfp_evt_stuck_0),
    _DEVICE_ATTR(cpld_osfp_evt_stuck_1),
    _DEVICE_ATTR(cpld_osfp_evt_stuck_2),
    _DEVICE_ATTR(cpld_osfp_evt_stuck_3),
    _DEVICE_ATTR(cpld_osfp_reset_0),
    _DEVICE_ATTR(cpld_osfp_reset_1),
    _DEVICE_ATTR(cpld_osfp_reset_2),
    _DEVICE_ATTR(cpld_osfp_reset_3),
    _DEVICE_ATTR(cpld_osfp_lpmode_0),
    _DEVICE_ATTR(cpld_osfp_lpmode_1),
    _DEVICE_ATTR(cpld_osfp_lpmode_2),
    _DEVICE_ATTR(cpld_osfp_lpmode_3),
    _DEVICE_ATTR(cpld_i2c_ctrl),
    _DEVICE_ATTR(cpld_i2c_relay),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_port_0),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_port_1),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_port_2),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_port_3),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_present_0),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_present_1),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_present_2),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_present_3),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_fuse_0),
    _DEVICE_ATTR(dbg_cpld_osfp_intr_fuse_1),
    NULL
};

/* fpga */
static struct attribute *fpga_attributes[] = {
    // FPGA
    _DEVICE_ATTR(fpga_minor_ver),
    _DEVICE_ATTR(fpga_major_ver),
    _DEVICE_ATTR(fpga_build_ver),
    _DEVICE_ATTR(fpga_version_h),
    _DEVICE_ATTR(fpga_id),
    _DEVICE_ATTR(fpga_sfp28_tx_rate_cap),
    _DEVICE_ATTR(fpga_sfp28_rx_rate_cap),
    _DEVICE_ATTR(fpga_sfp28_tx_dis),
    _DEVICE_ATTR(fpga_sfp28_tx_fault),
    _DEVICE_ATTR(fpga_sfp28_rx_los),
    _DEVICE_ATTR(fpga_sfp28_intr_present),
    _DEVICE_ATTR(fpga_sfp28_stuck),
    _DEVICE_ATTR(fpga_sfp28_mask_tx_fault),
    _DEVICE_ATTR(fpga_sfp28_mask_rx_los),
    _DEVICE_ATTR(fpga_sfp28_mask_present),
    _DEVICE_ATTR(fpga_sfp28_mask_stuck),
    _DEVICE_ATTR(fpga_sfp28_evt_present),
    _DEVICE_ATTR(fpga_sfp28_evt_tx_fault),
    _DEVICE_ATTR(fpga_sfp28_evt_rx_los),
    _DEVICE_ATTR(fpga_sfp28_evt_stuck),
    _DEVICE_ATTR(fpga_evt_ctrl),
    _DEVICE_ATTR(fpga_lan_port_relay),
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

/* cpld 3 attributes group */
static const struct attribute_group cpld3_group = {
    .attrs = cpld3_attributes,
};

/* fpga attributes group */
static const struct attribute_group fpga_group = {
    .attrs = fpga_attributes,
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
u8 _mask_shift(u8 val, u8 mask)
{
    int shift=0;

    shift = _shift(mask);

    return (val & mask) >> shift;
}

static ssize_t _parse_data(char *buf, unsigned int data, u8 data_type)
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

static int _store_value_check(int index, u8 reg_val, char **range) {
    int ret = 0;
    if(range == NULL) {
        return -2;
    }

    switch (index) {
        case CPLD_MGMT_PORT_0_LED_SPEED:
        case CPLD_MGMT_PORT_1_LED_SPEED:
            if(reg_val != 0 && reg_val != 1) {
                ret = -1;
            }
            *range = "0 or 1";
            break;
        default:
            break;
    }

    return ret;
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
            str_len = sizeof(bsp_debug);
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
    u8 reg_val = 0;
    int ret = 0;

    switch (attr->index) {
        // CPLD Common
        case CPLD_MINOR_VER:
        case CPLD_MAJOR_VER:
        case CPLD_ID:
        case CPLD_BUILD_VER:
        case CPLD_EVT_CTRL:

        //CPLD 1
        case CPLD_BOARD_ID_0:
        case CPLD_BOARD_ID_1:
        case CPLD_MAC_INTR:
        case CPLD_PHY_INTR:
        case CPLD_CPLDX_INTR:
        case CPLD_MAC_THERMAL_INTR:
        case CPLD_MISC_INTR:
        case CPLD_CPU_INTR:
        case CPLD_MAC_MASK:
        case CPLD_PHY_MASK:
        case CPLD_CPLDX_MASK:
        case CPLD_MAC_THERMAL_MASK:
        case CPLD_MISC_MASK:
        case CPLD_CPU_MASK:
        case CPLD_MAC_EVT:
        case CPLD_PHY_EVT:
        case CPLD_CPLDX_EVT:
        case CPLD_MAC_THERMAL_EVT:
        case CPLD_MISC_EVT:
        case CPLD_MAC_RESET:
        case CPLD_BMC_RESET:
        case CPLD_USB_RESET:
        case CPLD_MISC_RESET:
        case CPLD_BRD_PRESENT:
        case CPLD_PSU_STATUS:
        case CPLD_SYSTEM_PWR:
        case CPLD_MAC_SYNCE:
        case CPLD_MAC_ROV:
        case CPLD_MUX_CTRL:
        case CPLD_SYSTEM_LED_SYS:
        case CPLD_SYSTEM_LED_FAN:
        case CPLD_SYSTEM_LED_PSU_0:
        case CPLD_SYSTEM_LED_PSU_1:
        case CPLD_SYSTEM_LED_SYNC:
        case CPLD_SYSTEM_LED_ID:
        case CPLD_MGMT_PORT_0_LED_SPEED:
        case CPLD_MGMT_PORT_1_LED_SPEED:
        case CPLD_PORT_LED_CLR:
        case CPLD_MISC_PWR:
        case DBG_CPLD_MAC_INTR:
        case DBG_CPLD_CPLDX_INTR:
        case DBG_CPLD_MAC_THERMAL_INTR:
        case DBG_CPLD_MISC_INTR:

        //CPLD 2
        case CPLD_OSFP_PORT_0_7_16_23_INTR:
        case CPLD_OSFP_PORT_8_15_24_31_INTR:
        case CPLD_OSFP_PORT_32_39_48_55_INTR:
        case CPLD_OSFP_PORT_40_47_56_63_INTR:
        case CPLD_OSFP_PORT_0_7_16_23_PRES:
        case CPLD_OSFP_PORT_8_15_24_31_PRES:
        case CPLD_OSFP_PORT_32_39_48_55_PRES:
        case CPLD_OSFP_PORT_40_47_56_63_PRES:
        case CPLD_OSFP_PORT_0_15_16_31_FUSE:
        case CPLD_OSFP_PORT_32_47_48_63_FUSE:
        case CPLD_OSFP_PORT_0_7_16_23_STUCK:
        case CPLD_OSFP_PORT_8_15_24_31_STUCK:
        case CPLD_OSFP_PORT_32_39_48_55_STUCK:
        case CPLD_OSFP_PORT_40_47_56_63_STUCK:
        case CPLD_OSFP_PORT_0_7_16_23_INTR_MASK:
        case CPLD_OSFP_PORT_8_15_24_31_INTR_MASK:
        case CPLD_OSFP_PORT_32_39_48_55_INTR_MASK:
        case CPLD_OSFP_PORT_40_47_56_63_INTR_MASK:
        case CPLD_OSFP_PORT_0_7_16_23_PRES_MASK:
        case CPLD_OSFP_PORT_8_15_24_31_PRES_MASK:
        case CPLD_OSFP_PORT_32_39_48_55_PRES_MASK:
        case CPLD_OSFP_PORT_40_47_56_63_PRES_MASK:
        case CPLD_OSFP_PORT_0_15_16_31_FUSE_MASK:
        case CPLD_OSFP_PORT_32_47_48_63_FUSE_MASK:
        case CPLD_OSFP_PORT_0_7_16_23_STUCK_MASK:
        case CPLD_OSFP_PORT_8_15_24_31_STUCK_MASK:
        case CPLD_OSFP_PORT_32_39_48_55_STUCK_MASK:
        case CPLD_OSFP_PORT_40_47_56_63_STUCK_MASK:
        case CPLD_OSFP_PORT_0_7_16_23_INTR_EVENT:
        case CPLD_OSFP_PORT_8_15_24_31_INTR_EVENT:
        case CPLD_OSFP_PORT_32_39_48_55_INTR_EVENT:
        case CPLD_OSFP_PORT_40_47_56_63_INTR_EVENT:
        case CPLD_OSFP_PORT_0_7_16_23_PRES_EVENT:
        case CPLD_OSFP_PORT_8_15_24_31_PRES_EVENT:
        case CPLD_OSFP_PORT_32_39_48_55_PRES_EVENT:
        case CPLD_OSFP_PORT_40_47_56_63_PRES_EVENT:
        case CPLD_OSFP_PORT_0_15_16_31_FUSE_EVENT:
        case CPLD_OSFP_PORT_32_47_48_63_FUSE_EVENT:
        case CPLD_OSFP_PORT_0_7_16_23_STUCK_EVENT:
        case CPLD_OSFP_PORT_8_15_24_31_STUCK_EVENT:
        case CPLD_OSFP_PORT_32_39_48_55_STUCK_EVENT:
        case CPLD_OSFP_PORT_40_47_56_63_STUCK_EVENT:
        case CPLD_OSFP_PORT_0_7_16_23_RST:
        case CPLD_OSFP_PORT_8_15_24_31_RST:
        case CPLD_OSFP_PORT_32_39_48_55_RST:
        case CPLD_OSFP_PORT_40_47_56_63_RST:
        case CPLD_OSFP_PORT_0_7_16_23_LPMODE:
        case CPLD_OSFP_PORT_8_15_24_31_LPMODE:
        case CPLD_OSFP_PORT_32_39_48_55_LPMODE:
        case CPLD_OSFP_PORT_40_47_56_63_LPMODE:
        case CPLD_I2C_CONTROL:
        case CPLD_I2C_RELAY:
        case CPLD_DBG_OSFP_PORT_0_7_16_23_INTR:
        case CPLD_DBG_OSFP_PORT_8_15_24_31_INTR:
        case CPLD_DBG_OSFP_PORT_32_39_48_55_INTR:
        case CPLD_DBG_OSFP_PORT_40_47_56_63_INTR:
        case CPLD_DBG_OSFP_PORT_0_7_16_23_PRES:
        case CPLD_DBG_OSFP_PORT_8_15_24_31_PRES:
        case CPLD_DBG_OSFP_PORT_32_39_48_55_PRES:
        case CPLD_DBG_OSFP_PORT_40_47_56_63_PRES:
        case CPLD_DBG_OSFP_PORT_0_15_16_31_FUSE:
        case CPLD_DBG_OSFP_PORT_32_47_48_63_FUSE:

        //FPGA
        case FPGA_MINOR_VER:
        case FPGA_MAJOR_VER:
        case FPGA_BUILD:
        case FPGA_CHIP:
        case FPGA_MGMT_PORT_0_1_TX_RATE_SEL:
        case FPGA_MGMT_PORT_0_1_RX_RATE_SEL:
        case FPGA_MGMT_PORT_0_1_TX_DIS:
        case FPGA_MGMT_PORT_0_1_TX_FAULT:
        case FPGA_MGMT_PORT_0_1_RX_LOS:
        case FPGA_MGMT_PORT_0_1_PRES:
        case FPGA_MGMT_PORT_0_1_STUCK:
        case FPGA_MGMT_PORT_0_1_TX_FAULT_MASK:
        case FPGA_MGMT_PORT_0_1_RX_LOS_MASK:
        case FPGA_MGMT_PORT_0_1_PRES_MASK:
        case FPGA_MGMT_PORT_0_1_STUCK_MASK:
        case FPGA_MGMT_PORT_0_1_TX_FAULT_EVENT:
        case FPGA_MGMT_PORT_0_1_RX_LOS_EVENT:
        case FPGA_MGMT_PORT_0_1_PRES_EVENT:
        case FPGA_MGMT_PORT_0_1_STUCK_EVENT:
        case FPGA_EVT_CTRL:
        case FPGA_LAN_PORT_RELAY:

            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            data_type = attr_reg[attr->index].data_type;
            break;
        default:
            return -EINVAL;
    }

    ret = cpld_reg_read(dev, &reg_val, reg, mask);
    if( ret < 0) {
        return ret;
    }
    return _parse_data(buf, reg_val, data_type);
}

/* set cpld register value */
static ssize_t cpld_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg_val = 0;
    u8 reg = 0;
    u8 mask = MASK_NONE;
    char *range = NULL;
    int ret = 0;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    ret = _store_value_check(attr->index, reg_val, &range);
    if (ret < 0) {
        if(ret == -2) {
            return -EINVAL;
        } else {
            pr_err("Input is out of range(%s)\n", range);
            return -EINVAL;
        }
    }

    switch (attr->index) {
        // CPLD Common
        case CPLD_EVT_CTRL:

        //CPLD 1
        case CPLD_MAC_MASK:
        case CPLD_PHY_MASK:
        case CPLD_CPLDX_MASK:
        case CPLD_MAC_THERMAL_MASK:
        case CPLD_MISC_MASK:
        case CPLD_CPU_MASK:
        case CPLD_MAC_RESET:
        case CPLD_BMC_RESET:
        case CPLD_USB_RESET:
        case CPLD_MISC_RESET:
        case CPLD_MUX_CTRL:
        case CPLD_SYSTEM_LED_SYS:
        case CPLD_SYSTEM_LED_FAN:
        case CPLD_SYSTEM_LED_PSU_0:
        case CPLD_SYSTEM_LED_PSU_1:
        case CPLD_SYSTEM_LED_SYNC:
        case CPLD_SYSTEM_LED_ID:
        case CPLD_MGMT_PORT_0_LED_SPEED:
        case CPLD_MGMT_PORT_1_LED_SPEED:
        case CPLD_PORT_LED_CLR:

        //CPLD 2
        case CPLD_OSFP_PORT_0_7_16_23_INTR_MASK:
        case CPLD_OSFP_PORT_8_15_24_31_INTR_MASK:
        case CPLD_OSFP_PORT_32_39_48_55_INTR_MASK:
        case CPLD_OSFP_PORT_40_47_56_63_INTR_MASK:
        case CPLD_OSFP_PORT_0_7_16_23_PRES_MASK:
        case CPLD_OSFP_PORT_8_15_24_31_PRES_MASK:
        case CPLD_OSFP_PORT_32_39_48_55_PRES_MASK:
        case CPLD_OSFP_PORT_40_47_56_63_PRES_MASK:
        case CPLD_OSFP_PORT_0_15_16_31_FUSE_MASK:
        case CPLD_OSFP_PORT_32_47_48_63_FUSE_MASK:
        case CPLD_OSFP_PORT_0_7_16_23_STUCK_MASK:
        case CPLD_OSFP_PORT_8_15_24_31_STUCK_MASK:
        case CPLD_OSFP_PORT_32_39_48_55_STUCK_MASK:
        case CPLD_OSFP_PORT_40_47_56_63_STUCK_MASK:
        case CPLD_OSFP_PORT_0_7_16_23_RST:
        case CPLD_OSFP_PORT_8_15_24_31_RST:
        case CPLD_OSFP_PORT_32_39_48_55_RST:
        case CPLD_OSFP_PORT_40_47_56_63_RST:
        case CPLD_OSFP_PORT_0_7_16_23_LPMODE:
        case CPLD_OSFP_PORT_8_15_24_31_LPMODE:
        case CPLD_OSFP_PORT_32_39_48_55_LPMODE:
        case CPLD_OSFP_PORT_40_47_56_63_LPMODE:
        case CPLD_I2C_CONTROL:
        case CPLD_I2C_RELAY:
        case CPLD_DBG_OSFP_PORT_0_7_16_23_INTR:
        case CPLD_DBG_OSFP_PORT_8_15_24_31_INTR:
        case CPLD_DBG_OSFP_PORT_32_39_48_55_INTR:
        case CPLD_DBG_OSFP_PORT_40_47_56_63_INTR:
        case CPLD_DBG_OSFP_PORT_0_7_16_23_PRES:
        case CPLD_DBG_OSFP_PORT_8_15_24_31_PRES:
        case CPLD_DBG_OSFP_PORT_32_39_48_55_PRES:
        case CPLD_DBG_OSFP_PORT_40_47_56_63_PRES:
        case CPLD_DBG_OSFP_PORT_0_15_16_31_FUSE:
        case CPLD_DBG_OSFP_PORT_32_47_48_63_FUSE:

        //FPGA
        case FPGA_MGMT_PORT_0_1_TX_RATE_SEL:
        case FPGA_MGMT_PORT_0_1_RX_RATE_SEL:
        case FPGA_MGMT_PORT_0_1_TX_DIS:
        case FPGA_MGMT_PORT_0_1_TX_FAULT_MASK:
        case FPGA_MGMT_PORT_0_1_RX_LOS_MASK:
        case FPGA_MGMT_PORT_0_1_PRES_MASK:
        case FPGA_MGMT_PORT_0_1_STUCK_MASK:
        case FPGA_EVT_CTRL:

            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_write(dev, reg_val, count, reg, mask);
}

/* get cpld register value */
int _cpld_reg_read(struct device *dev, u8 reg, u8 mask)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
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
                    u8 *reg_val,
                    u8 reg,
                    u8 mask)
{
    int ret = 0;

    if(reg_val == NULL) {
        return -EINVAL;
    }

    ret = _cpld_reg_read(dev, reg, mask);
    if (unlikely(ret < 0)) {
        dev_err(dev, "cpld_reg_read() error, reg_val=%d\n", ret);
        return ret;
    }

    *reg_val = (u8)ret;
    return 0;
}

int _cpld_reg_write(struct device *dev,
                    u8 reg,
                    u8 reg_val)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int ret = 0;

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
               client, reg, reg_val);

    return ret;
}

/* set cpld register value */
static ssize_t cpld_reg_write(struct device *dev,
                    u8 reg_val,
                    size_t count,
                    u8 reg,
                    u8 mask)
{
    u8 reg_val_now, shift;
    int ret = 0;

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

    ret = _cpld_reg_write(dev, reg, reg_val);

    if (unlikely(ret < 0)) {
        dev_err(dev, "cpld_reg_write() error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* get cpld and fpga version register value */
static ssize_t version_h_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int major =-1;
    int minor =-1;
    int build =-1;
    int major_val = -1;
    int minor_val = -1;
    int build_val = -1;

    switch(attr->index) {
        case CPLD_VERSION_H:
            major = CPLD_MAJOR_VER;
            minor = CPLD_MINOR_VER;
            build = CPLD_BUILD_VER;
            break;
        case FPGA_VERSION_H:
            major = FPGA_MAJOR_VER;
            minor = FPGA_MINOR_VER;
            build = FPGA_BUILD;
            break;
        default:
            major=-1;
            minor=-1;
            build=-1;
            break;
    }

    if (major >= 0 && minor >= 0 && build >= 0) {
        major_val = _cpld_reg_read(dev, attr_reg[major].reg, attr_reg[major].mask);
        minor_val = _cpld_reg_read(dev, attr_reg[minor].reg, attr_reg[minor].mask);
        build_val = _cpld_reg_read(dev, attr_reg[build].reg, attr_reg[build].mask);

        if(major_val < 0 || minor_val < 0 || build_val < 0)
            return -EIO ;

        return sprintf(buf, "%d.%02d.%03d", major_val, minor_val, build_val);
    }
    return -EINVAL;
}

static int _get_led_node(int index, led_node_t *node)
{
    color_obj_t mgmt_port_set[COLOR_VAL_MAX] = {
        [0] = {.status = LED_COLOR_DARK           , .val = 0b00000000},
        [1] = {.status = LED_COLOR_DARK           , .val = 0b00000001},
        [2] = {.status = LED_COLOR_DARK           , .val = 0b00000100},
        [3] = {.status = LED_COLOR_DARK           , .val = 0b00000101},
        [4] = {.status = LED_COLOR_GREEN          , .val = 0b00001001},
        [5] = {.status = LED_COLOR_GREEN_BLINK    , .val = 0b00001101},
        [6] = {.status = LED_COLOR_YELLOW         , .val = 0b00001000},
        [7] = {.status = LED_COLOR_YELLOW_BLINK   , .val = 0b00001100},
        [8] = {.val = -1}
    };

    switch (index){
        case CPLD_MGMT_PORT_0_LED_STATUS:
        case CPLD_MGMT_PORT_1_LED_STATUS:
            node->type=TYPE_LED_2_SETS;
            node->reg = attr_reg[index].reg;
            node->mask= attr_reg[index].mask;
            node->color_mask = MASK_0000_1101;
            node->data_type = attr_reg[index].data_type;
            memcpy(node->color_obj, mgmt_port_set, sizeof(mgmt_port_set));
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static ssize_t led_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    led_node_t node = {0};
    int status = LED_COLOR_DARK;
    int led_val = 0;
    int found = 0;
    int i = 0;

    if(_get_led_node(attr->index,&node) != 0) {
        return -EINVAL;		
    }

    led_val=_cpld_reg_read(dev, node.reg, node.mask);

    if(led_val < 0) {
        return led_val;
    }

    for(i= 0; i<COLOR_VAL_MAX; i++) {
        if(node.color_obj[i].val == -1)
            break;

        if((led_val & node.color_mask) == node.color_obj[i].val) {
            status = node.color_obj[i].status;
            found=1;
            break;
        }
    }

    if(found == 0) {
        pr_err("Led value not in definition!!\n");
        return -EINVAL;
    }

    return _parse_data(buf, status, node.data_type);
}

static ssize_t led_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    led_node_t node = {0};
    int status = LED_COLOR_DARK;
    short int val;
    int found = 0;
    int i = 0;

    if(_get_led_node(attr->index,&node) != 0) {
        return -EINVAL;		
    }

    if (kstrtoint(buf, 0, &status) < 0)
        return -EINVAL;

    for(i= 0; i<COLOR_VAL_MAX; i++) {
        if(node.color_obj[i].val == -1)
            break;

        if(status == node.color_obj[i].status) {
            found=1;
            val = node.color_obj[i].val;
            break;
        }
    }

    if(found == 0) {
        pr_err("Led value not in definition!!\n");
        return -EINVAL;
    }

    return cpld_reg_write(dev, (u8)val, count, node.reg, node.mask);
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
static int cpld_probe(struct i2c_client *client,
                    const struct i2c_device_id *dev_id)
{
#else
static int cpld_probe(struct i2c_client *client)
{
    const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
#endif
    int status;
    struct i2c_adapter *adap = client->adapter;
    struct device *dev = &client->dev;
    struct cpld_data *data = NULL;
    struct i2c_mux_core *muxc;

    muxc = i2c_mux_alloc(adap, dev, CPLD_MAX_NCHANS, sizeof(*data), 0,
                mux_select_chan, mux_deselect_mux);

    data = i2c_mux_priv(muxc);
    if (!data)
        return -ENOMEM;

    /* init cpld data for client */
    i2c_set_clientdata(client, muxc);

    data->client = client;
    mutex_init(&data->access_lock);

    if (!i2c_check_functionality(client->adapter,
                I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_info(&client->dev,
            "i2c_check_functionality failed (0x%x)\n",
            client->addr);
        status = -EIO;
        goto exit;
    }

    data->index = dev_id->driver_data;

    /* register sysfs hooks for different cpld group */
    dev_info(&client->dev, "probe cpld with index %d\n", data->index);

    if(mux_en) {
        status = mux_init(dev);
        if (status < 0) {
            dev_warn(dev, "Mux init failed\n");
            goto exit;
        }
    }

    switch (data->index) {
    case cpld1:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld1_group);
        break;
    case cpld2:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld2_group);
        break;
    case cpld3:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld3_group);
        break;
    case fpga:
        status = sysfs_create_group(&client->dev.kobj,
                    &fpga_group);
        break;
    default:
        status = -EINVAL;
    }

    if(mux_en) {
        if(data->chip->nchans > 0){
            status = sysfs_add_file_to_group(&client->dev.kobj,
                        &sensor_dev_attr_idle_state.dev_attr.attr, NULL);
        }
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
    case cpld3:
        sysfs_remove_group(&client->dev.kobj, &cpld3_group);
        break;
    case fpga:
        sysfs_remove_group(&client->dev.kobj, &fpga_group);
        break;
    default:
        break;
    }

    return status;
}

/* cpld drvier remove */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int
#else
static void
#endif
cpld_remove(struct i2c_client *client)
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct device *dev = &client->dev;
    struct cpld_data *data = i2c_mux_priv(muxc);

    if(mux_en) {
        if(data->chip->nchans > 0){
            sysfs_remove_file_from_group(&client->dev.kobj,
                &sensor_dev_attr_idle_state.dev_attr.attr, NULL);
        }
    }

    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &cpld1_group);
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        break;
    case cpld3:
        sysfs_remove_group(&client->dev.kobj, &cpld3_group);
        break;
    case fpga:
        sysfs_remove_group(&client->dev.kobj, &fpga_group);
        break;
    default:
        break;
    }

    if(mux_en) {
        mux_cleanup(dev);
    }

    cpld_remove_client(client);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
    return 0;
#endif
}

MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9321_64eo_cpld",
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

MODULE_AUTHOR("Nonodark Huang<nonodark.huang@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9321_64eo_cpld driver");
MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);