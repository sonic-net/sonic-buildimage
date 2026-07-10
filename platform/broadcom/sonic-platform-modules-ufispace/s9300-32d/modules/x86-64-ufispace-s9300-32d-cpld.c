/*
 * A i2c cpld driver for the ufispace_s9300_32d
 *
 * Copyright (C) 2017-2019 UfiSpace Technology Corporation.
 * Jason Tsai <jason.cy.tsai@ufispace.com>
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
#include <linux/version.h>
#include "x86-64-ufispace-s9300-32d-cpld.h"

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

/* CPLD sysfs attributes index  */
enum s9300_cpld_sysfs_attributes {
    /* CPLD1 */
    CPLD_ACCESS_REG,
    CPLD_REGISTER_VAL,
    CPLD_SKU_ID,
    CPLD_HW_REV,
    CPLD_DEPH_REV,
    CPLD_BUILD_REV,
    CPLD_ID_TYPE,
    CPLD_MAJOR_VER,
    CPLD_MINOR_VER,
    CPLD_BUILD_VER,
    CPLD_VERION_H,
    CPLD_ID,
    CPLD_MAC_INTR,
    CPLD_10G_PHY_INTR,
    CPLD_CPLD_FRU_INTR,
    CPLD_THERMAL_ALERT_INTR,
    CPLD_MISC_INTR,
    CPLD_SYSTEM_INTR,
    CPLD_MAC_INTR_MASK,
    CPLD_10G_PHY_INTR_MASK,
    CPLD_CPLD_FRU_INTR_MASK,
    CPLD_THERMAL_ALERT_INTR_MASK,
    CPLD_MISC_INTR_MASK,
    CPLD_MAC_INTR_EVENT,
    CPLD_10G_PHY_INTR_EVENT,
    CPLD_CPLD_FRU_INTR_EVENT,
    CPLD_THERMAL_ALERT_INTR_EVENT,
    CPLD_MISC_INTR_EVENT,
    CPLD_MAC_RST,
    CPLD_10G_PHY_RST,
    CPLD_BMC_RST,
    CPLD_USB_RST,
    CPLD_MUX_RST,
    CPLD_MISC_RST,
    CPLD_BMC_WATCHDOG,
    CPLD_DAU_BD_PRES,
    CPLD_PSU_STATUS,
    CPLD_SYS_PW_STATUS,
    CPLD_MISC,
    CPLD_MUX_CTRL,
    CPLD_MAC_QSFP_SEL_CTRL,
    CPLD_SYS_LED_CTRL_1,
    CPLD_SYS_LED_CTRL_2,
    CPLD_BEACON_LED_CTRL,
    CPLD_PORT_LED_CLR_CTRL,
    CPLD_EVENT_DETECT_CTRL,
    /* CPLD2 */
    CPLD_QSFPDD_MOD_INT_G0,
    CPLD_QSFPDD_MOD_INT_G1,
    CPLD_QSFPDD_MOD_INT_G2,
    CPLD_QSFPDD_MOD_INT_G3,
    CPLD_QSFPDD_PRES_G0,
    CPLD_QSFPDD_PRES_G1,
    CPLD_QSFPDD_PRES_G2,
    CPLD_QSFPDD_PRES_G3,
    CPLD_QSFPDD_FUSE_INT_G0,
    CPLD_QSFPDD_FUSE_INT_G1,
    CPLD_QSFPDD_FUSE_INT_G2,
    CPLD_QSFPDD_FUSE_INT_G3,
    CPLD_SFP_TXFAULT,
    CPLD_SFP_ABS,
    CPLD_SFP_RXLOS,
    CPLD_QSFPDD_MOD_INT_MASK_G0,
    CPLD_QSFPDD_MOD_INT_MASK_G1,
    CPLD_QSFPDD_MOD_INT_MASK_G2,
    CPLD_QSFPDD_MOD_INT_MASK_G3,
    CPLD_QSFPDD_PRES_MASK_G0,
    CPLD_QSFPDD_PRES_MASK_G1,
    CPLD_QSFPDD_PRES_MASK_G2,
    CPLD_QSFPDD_PRES_MASK_G3,
    CPLD_QSFPDD_FUSE_INT_MASK_G0,
    CPLD_QSFPDD_FUSE_INT_MASK_G1,
    CPLD_QSFPDD_FUSE_INT_MASK_G2,
    CPLD_QSFPDD_FUSE_INT_MASK_G3,
    CPLD_SFP_TXFAULT_MASK,
    CPLD_SFP_ABS_MASK,
    CPLD_SFP_RXLOS_MASK,
    CPLD_QSFPDD_MOD_INT_EVENT_G0,
    CPLD_QSFPDD_MOD_INT_EVENT_G1,
    CPLD_QSFPDD_MOD_INT_EVENT_G2,
    CPLD_QSFPDD_MOD_INT_EVENT_G3,
    CPLD_QSFPDD_PRES_EVENT_G0,
    CPLD_QSFPDD_PRES_EVENT_G1,
    CPLD_QSFPDD_PRES_EVENT_G2,
    CPLD_QSFPDD_PRES_EVENT_G3,
    CPLD_QSFPDD_FUSE_INT_EVENT_G0,
    CPLD_QSFPDD_FUSE_INT_EVENT_G1,
    CPLD_QSFPDD_FUSE_INT_EVENT_G2,
    CPLD_QSFPDD_FUSE_INT_EVENT_G3,
    CPLD_SFP_TXFAULT_EVENT,
    CPLD_SFP_ABS_EVENT,
    CPLD_SFP_RXLOS_EVENT,
    CPLD_QSFPDD_RESET_CTRL_G0,
    CPLD_QSFPDD_RESET_CTRL_G1,
    CPLD_QSFPDD_RESET_CTRL_G2,
    CPLD_QSFPDD_RESET_CTRL_G3,
    CPLD_QSFPDD_LP_MODE_G0,
    CPLD_QSFPDD_LP_MODE_G1,
    CPLD_QSFPDD_LP_MODE_G2,
    CPLD_QSFPDD_LP_MODE_G3,
    CPLD_SFP_TX_DIS,
    CPLD_SFP_RS,
    CPLD_SFP_TS,
    CPLD_PORT_INT_STATUS,

    //BSP DEBUG
    BSP_DEBUG
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
static ssize_t read_access_register(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_access_register(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_register_value(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_register_value(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_hw_rev_cb(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t read_cpld_version_cb(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t read_cpld_callback(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t write_cpld_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_cpld_version_h_cb(struct device *dev,
        struct device_attribute *da, char *buf);
// cpld access api
static ssize_t read_cpld_reg(struct device *dev, char *buf, u8 reg);
static ssize_t write_cpld_reg(struct device *dev, const char *buf, size_t count, u8 reg);
static bool read_cpld_reg_raw_byte(struct device *dev, u8 reg, u8 *val, int *errno);
static bool read_cpld_reg_raw_int(struct device *dev, u8 reg, int *val);
// bsp debug api
static ssize_t read_bsp(char *buf, char *str);
static ssize_t write_bsp(const char *buf, char *str, size_t str_len, size_t count);
static ssize_t read_bsp_callback(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t write_bsp_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);

static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

struct cpld_data {
    int index;                  /* CPLD index */
    struct mutex access_lock;       /* mutex for cpld access */
    u8 access_reg;              /* register to access */
};

/* CPLD device id and data */
static const struct i2c_device_id s9300_cpld_id[] = {
    { "s9300_32d_cpld1",  cpld1 },
    { "s9300_32d_cpld2",  cpld2 },
    { "s9300_32d_cpld3",  cpld3 },
    {}
};

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for s9300_cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */
/* CPLD1 */
static SENSOR_DEVICE_ATTR(cpld_access_register, S_IWUSR | S_IRUGO, \
        read_access_register, write_access_register, CPLD_ACCESS_REG);
static SENSOR_DEVICE_ATTR(cpld_register_value, S_IWUSR | S_IRUGO, \
        read_register_value, write_register_value, CPLD_REGISTER_VAL);
static SENSOR_DEVICE_ATTR(cpld_sku_id, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SKU_ID);
static SENSOR_DEVICE_ATTR(cpld_hw_rev, S_IRUGO, \
        read_hw_rev_cb, NULL, CPLD_HW_REV);
static SENSOR_DEVICE_ATTR(cpld_deph_rev, S_IRUGO, \
        read_hw_rev_cb, NULL, CPLD_DEPH_REV);
static SENSOR_DEVICE_ATTR(cpld_build_rev, S_IRUGO, \
        read_hw_rev_cb, NULL, CPLD_BUILD_REV);
static SENSOR_DEVICE_ATTR(cpld_id_type, S_IRUGO, \
        read_hw_rev_cb, NULL, CPLD_ID_TYPE);
static SENSOR_DEVICE_ATTR(cpld_major_ver, S_IRUGO, \
        read_cpld_version_cb, NULL, CPLD_MAJOR_VER);
static SENSOR_DEVICE_ATTR(cpld_minor_ver, S_IRUGO, \
        read_cpld_version_cb, NULL, CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR(cpld_build_ver, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_BUILD_VER);
static SENSOR_DEVICE_ATTR(cpld_version_h, S_IRUGO, \
        read_cpld_version_h_cb, NULL, CPLD_VERION_H);
static SENSOR_DEVICE_ATTR(cpld_id, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_ID);
static SENSOR_DEVICE_ATTR(cpld_mac_intr, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_MAC_INTR);
static SENSOR_DEVICE_ATTR(cpld_10g_phy_intr, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_10G_PHY_INTR);
static SENSOR_DEVICE_ATTR(cpld_cpld_fru_intr, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_CPLD_FRU_INTR);
static SENSOR_DEVICE_ATTR(cpld_thermal_alert_intr, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_THERMAL_ALERT_INTR);
static SENSOR_DEVICE_ATTR(cpld_misc_intr, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_MISC_INTR);
static SENSOR_DEVICE_ATTR(cpld_system_intr, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SYSTEM_INTR);
static SENSOR_DEVICE_ATTR(cpld_mac_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_MAC_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_10g_phy_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_10G_PHY_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_cpld_fru_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_CPLD_FRU_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_thermal_alert_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_THERMAL_ALERT_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_misc_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_MISC_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_mac_intr_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_MAC_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_10g_phy_intr_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_10G_PHY_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_cpld_fru_intr_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_CPLD_FRU_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_thermal_alert_intr_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_THERMAL_ALERT_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_misc_intr_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_MISC_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_mac_rst, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_MAC_RST);
static SENSOR_DEVICE_ATTR(cpld_10g_phy_rst, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_10G_PHY_RST);
static SENSOR_DEVICE_ATTR(cpld_bmc_rst, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_BMC_RST);
static SENSOR_DEVICE_ATTR(cpld_usb_rst, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_USB_RST);
static SENSOR_DEVICE_ATTR(cpld_mux_rst, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_MUX_RST);
static SENSOR_DEVICE_ATTR(cpld_misc_rst, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_MISC_RST);
static SENSOR_DEVICE_ATTR(cpld_bmc_watchdog, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_BMC_WATCHDOG);
static SENSOR_DEVICE_ATTR(cpld_dau_bd_pres, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_DAU_BD_PRES);
static SENSOR_DEVICE_ATTR(cpld_psu_status, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_PSU_STATUS);
static SENSOR_DEVICE_ATTR(cpld_sys_pw_status, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SYS_PW_STATUS);
static SENSOR_DEVICE_ATTR(cpld_misc, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_MISC);
static SENSOR_DEVICE_ATTR(cpld_mux_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_MUX_CTRL);
static SENSOR_DEVICE_ATTR(cpld_mac_qsfp_sel_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_MAC_QSFP_SEL_CTRL);
static SENSOR_DEVICE_ATTR(cpld_sys_led_ctrl_1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SYS_LED_CTRL_1);
static SENSOR_DEVICE_ATTR(cpld_sys_led_ctrl_2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SYS_LED_CTRL_2);
static SENSOR_DEVICE_ATTR(cpld_beacon_led_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_BEACON_LED_CTRL);
static SENSOR_DEVICE_ATTR(cpld_port_led_clr_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_PORT_LED_CLR_CTRL);
static SENSOR_DEVICE_ATTR(cpld_event_detect_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_EVENT_DETECT_CTRL);
/* CPLD2 */
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_G3);
static SENSOR_DEVICE_ATTR(cpld_sfp_txfault, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_TXFAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_abs, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_ABS);
static SENSOR_DEVICE_ATTR(cpld_sfp_rxlos, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_RXLOS);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_mask_g0, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_MOD_INT_MASK_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_mask_g1, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_MOD_INT_MASK_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_mask_g2, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_MOD_INT_MASK_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_mask_g3, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_MOD_INT_MASK_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_mask_g0, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_PRES_MASK_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_mask_g1, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_PRES_MASK_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_mask_g2, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_PRES_MASK_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_mask_g3, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_PRES_MASK_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_mask_g0, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_FUSE_INT_MASK_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_mask_g1, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_FUSE_INT_MASK_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_mask_g2, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_FUSE_INT_MASK_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_mask_g3, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_QSFPDD_FUSE_INT_MASK_G3);
static SENSOR_DEVICE_ATTR(cpld_sfp_txfault_mask, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_SFP_TXFAULT_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_abs_mask, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_SFP_ABS_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_rxlos_mask, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_SFP_RXLOS_MASK);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_event_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_EVENT_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_event_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_EVENT_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_event_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_EVENT_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_event_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_EVENT_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_event_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_EVENT_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_event_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_EVENT_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_event_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_EVENT_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_event_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_EVENT_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_event_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_EVENT_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_event_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_EVENT_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_event_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_EVENT_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_event_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_EVENT_G3);
static SENSOR_DEVICE_ATTR(cpld_sfp_txfault_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_TXFAULT_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_abs_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_ABS_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_rxlos_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_RXLOS_EVENT);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_reset_ctrl_g0, \
	    S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_RESET_CTRL_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_reset_ctrl_g1, \
        S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_RESET_CTRL_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_reset_ctrl_g2, \
        S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_RESET_CTRL_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_reset_ctrl_g3, \
        S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_RESET_CTRL_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_lp_mode_g0, \
        S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_LP_MODE_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_lp_mode_g1, \
        S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_LP_MODE_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_lp_mode_g2, \
        S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_LP_MODE_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_lp_mode_g3, \
        S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_LP_MODE_G3);
static SENSOR_DEVICE_ATTR(cpld_sfp_tx_dis, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_SFP_TX_DIS);
static SENSOR_DEVICE_ATTR(cpld_sfp_rs, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_SFP_RS);
static SENSOR_DEVICE_ATTR(cpld_sfp_ts, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_SFP_TS);
static SENSOR_DEVICE_ATTR(cpld_port_int_status, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_PORT_INT_STATUS);
//BSP DEBUG
static SENSOR_DEVICE_ATTR(bsp_debug, S_IRUGO | S_IWUSR, \
        read_bsp_callback, write_bsp_callback, BSP_DEBUG);

/* define support attributes of cpldx , total 3 */
/* cpld 1 */
static struct attribute *s9300_cpld1_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_sku_id.dev_attr.attr,
    &sensor_dev_attr_cpld_hw_rev.dev_attr.attr,
    &sensor_dev_attr_cpld_deph_rev.dev_attr.attr,
    &sensor_dev_attr_cpld_build_rev.dev_attr.attr,
    &sensor_dev_attr_cpld_id_type.dev_attr.attr,
    &sensor_dev_attr_cpld_major_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_minor_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_build_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_version_h.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_10g_phy_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_cpld_fru_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_thermal_alert_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_system_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_10g_phy_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_cpld_fru_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_thermal_alert_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_10g_phy_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_cpld_fru_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_thermal_alert_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_10g_phy_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_usb_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_mux_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_watchdog.dev_attr.attr,
    &sensor_dev_attr_cpld_dau_bd_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_psu_status.dev_attr.attr,
    &sensor_dev_attr_cpld_sys_pw_status.dev_attr.attr,
    &sensor_dev_attr_cpld_misc.dev_attr.attr,
    &sensor_dev_attr_cpld_mux_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_qsfp_sel_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_sys_led_ctrl_1.dev_attr.attr,
    &sensor_dev_attr_cpld_sys_led_ctrl_2.dev_attr.attr,
    &sensor_dev_attr_cpld_beacon_led_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_port_led_clr_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_event_detect_ctrl.dev_attr.attr,
    &sensor_dev_attr_bsp_debug.dev_attr.attr,
    NULL
};

/* cpld 2 */
static struct attribute *s9300_cpld2_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_major_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_minor_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_build_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_version_h.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_txfault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_abs.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_rxlos.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_mask_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_mask_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_mask_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_mask_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_mask_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_mask_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_mask_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_mask_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_mask_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_mask_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_mask_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_mask_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_txfault_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_abs_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_rxlos_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_event_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_event_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_event_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_event_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_event_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_event_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_event_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_event_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_event_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_event_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_event_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_event_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_txfault_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_abs_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_rxlos_event.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_reset_ctrl_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_reset_ctrl_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_reset_ctrl_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_reset_ctrl_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_lp_mode_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_lp_mode_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_lp_mode_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_lp_mode_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_tx_dis.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_rs.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_ts.dev_attr.attr,
    &sensor_dev_attr_cpld_port_int_status.dev_attr.attr,
    NULL
};

/* cpld 3 */
static struct attribute *s9300_cpld3_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_major_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_minor_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_build_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_version_h.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    NULL
};

/* cpld 1 attributes group */
static const struct attribute_group s9300_cpld1_group = {
    .attrs = s9300_cpld1_attributes,
};
/* cpld 2 attributes group */
static const struct attribute_group s9300_cpld2_group = {
    .attrs = s9300_cpld2_attributes,
};
/* cpld 3 attributes group */
static const struct attribute_group s9300_cpld3_group = {
    .attrs = s9300_cpld3_attributes,
};

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
static ssize_t read_bsp(char *buf, char *str)
{
    ssize_t len=0;

    len=sprintf(buf, "%s", str);
    BSP_LOG_R("reg_val=%s", str);

    return len;
}

/* set bsp value */
static ssize_t write_bsp(const char *buf, char *str, size_t str_len, size_t count)
{
    snprintf(str, str_len, "%s", buf);
    BSP_LOG_W("reg_val=%s", str);

    return count;
}

/* get bsp parameter value */
static ssize_t read_bsp_callback(struct device *dev,
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
    return read_bsp(buf, str);
}

/* set bsp parameter value */
static ssize_t write_bsp_callback(struct device *dev,
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
            ret = write_bsp(buf, str, str_len, count);

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

/* read access register from cpld data */
static ssize_t read_access_register(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg = data->access_reg;

    return sprintf(buf, "0x%x\n", reg);
}

/* write access register to cpld data */
static ssize_t write_access_register(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;

    if (kstrtou8(buf, 0, &reg) < 0)
        return -EINVAL;

    data->access_reg = reg;
    return count;
}

/* read the value of access register in cpld data */
static ssize_t read_register_value(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg = data->access_reg;
    int reg_val;

    I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);

    if (reg_val < 0)
        return reg_val;

    return sprintf(buf, "0x%x\n", reg_val);
}

/* wrtie the value to access register in cpld data */
static ssize_t write_register_value(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int ret = -EIO;
    u8 reg = data->access_reg;
    u8 reg_val;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock, client, reg, reg_val);

    if (unlikely(ret < 0)) {
        dev_err(dev, "I2C_WRITE_BYTE_DATA error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* get cpld register value */
static ssize_t read_cpld_reg(struct device *dev,
                    char *buf,
                    u8 reg)
{
    int reg_val;

    if (read_cpld_reg_raw_int(dev, reg, &reg_val))
        return sprintf(buf, "0x%02x\n", reg_val);
    else
        return reg_val;
}

static bool read_cpld_reg_raw_int(struct device *dev, u8 reg, int *val)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    I2C_READ_BYTE_DATA(*val, &data->access_lock, client, reg);
    if (unlikely(*val < 0)) {
        dev_err(dev, "read_cpld_reg_raw_int() error, return=%d\n", *val);
        return false;
    }
    return true;
}

static bool read_cpld_reg_raw_byte(struct device *dev, u8 reg, u8 *val, int *errno)
{
    int reg_val;

    if (read_cpld_reg_raw_int(dev, reg, &reg_val)) {
        *val = (u8)reg_val;
        return true;
    } else {
        *errno = reg_val;
        return false;
    }
}

/* handle read for attributes */
static ssize_t read_cpld_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;

    switch (attr->index) {
        case CPLD_SKU_ID:
             reg = CPLD_SKU_ID_REG;
             break;
        case CPLD_ID:
             reg = CPLD_ID_REG;
             break;
        case CPLD_BUILD_VER:
             reg = CPLD_BUILD_VER_REG;
             break;
        case CPLD_MAC_INTR:
             reg = CPLD_MAC_INTR_REG;
             break;
        case CPLD_10G_PHY_INTR:
             reg = CPLD_10G_PHY_INTR_REG;
             break;
        case CPLD_CPLD_FRU_INTR:
             reg = CPLD_CPLD_FRU_INTR_REG;
             break;
        case CPLD_THERMAL_ALERT_INTR:
             reg = CPLD_THERMAL_ALERT_INTR_REG;
             break;
        case CPLD_MISC_INTR:
             reg = CPLD_MISC_INTR_REG;
             break;
        case CPLD_SYSTEM_INTR:
             reg = CPLD_SYSTEM_INTR_REG;
             break;
        case CPLD_MAC_INTR_MASK:
             reg = CPLD_MAC_INTR_MASK_REG;
             break;
        case CPLD_10G_PHY_INTR_MASK:
             reg = CPLD_10G_PHY_INTR_MASK_REG;
             break;
        case CPLD_CPLD_FRU_INTR_MASK:
             reg = CPLD_CPLD_FRU_INTR_MASK_REG;
             break;
        case CPLD_THERMAL_ALERT_INTR_MASK:
             reg = CPLD_THERMAL_ALERT_INTR_MASK_REG;
             break;
        case CPLD_MISC_INTR_MASK:
             reg = CPLD_MISC_INTR_MASK_REG;
             break;
        case CPLD_MAC_INTR_EVENT:
             reg = CPLD_MAC_INTR_EVENT_REG;
             break;
        case CPLD_10G_PHY_INTR_EVENT:
             reg = CPLD_10G_PHY_INTR_EVENT_REG;
             break;
        case CPLD_CPLD_FRU_INTR_EVENT:
             reg = CPLD_CPLD_FRU_INTR_EVENT_REG;
             break;
        case CPLD_THERMAL_ALERT_INTR_EVENT:
             reg = CPLD_THERMAL_ALERT_INTR_EVENT_REG;
             break;
        case CPLD_MISC_INTR_EVENT:
             reg = CPLD_MISC_INTR_EVENT_REG;
             break;
        case CPLD_MAC_RST:
             reg = CPLD_MAC_RST_REG;
             break;
        case CPLD_10G_PHY_RST:
             reg = CPLD_10G_PHY_RST_REG;
             break;
        case CPLD_BMC_RST:
             reg = CPLD_BMC_RST_REG;
             break;
        case CPLD_USB_RST:
             reg = CPLD_USB_RST_REG;
             break;
        case CPLD_MUX_RST:
             reg = CPLD_MUX_RST_REG;
             break;
        case CPLD_MISC_RST:
             reg = CPLD_MISC_RST_REG;
             break;
        case CPLD_BMC_WATCHDOG:
             reg = CPLD_BMC_WATCHDOG_REG;
             break;
        case CPLD_DAU_BD_PRES:
             reg = CPLD_DAU_BD_PRES_REG;
             break;
        case CPLD_PSU_STATUS:
             reg = CPLD_PSU_STATUS_REG;
             break;
        case CPLD_SYS_PW_STATUS:
             reg = CPLD_SYS_PW_STATUS_REG;
             break;
        case CPLD_MISC:
             reg = CPLD_MISC_REG;
             break;
        case CPLD_MUX_CTRL:
             reg = CPLD_MUX_CTRL_REG;
             break;
        case CPLD_MAC_QSFP_SEL_CTRL:
             reg = CPLD_MAC_QSFP_SEL_CTRL_REG;
             break;
        case CPLD_SYS_LED_CTRL_1:
             reg = CPLD_SYS_LED_CTRL_1_REG;
             break;
        case CPLD_SYS_LED_CTRL_2:
             reg = CPLD_SYS_LED_CTRL_2_REG;
             break;
        case CPLD_BEACON_LED_CTRL:
             reg = CPLD_BEACON_LED_CTRL_REG;
             break;
        case CPLD_PORT_LED_CLR_CTRL:
             reg = CPLD_PORT_LED_CLR_CTRL_REG;
             break;
        case CPLD_EVENT_DETECT_CTRL:
             reg = CPLD_EVENT_DETECT_CTRL_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_G0:
             reg = CPLD_QSFPDD_MOD_INT_G0_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_G1:
             reg = CPLD_QSFPDD_MOD_INT_G1_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_G2:
             reg = CPLD_QSFPDD_MOD_INT_G2_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_G3:
             reg = CPLD_QSFPDD_MOD_INT_G3_REG;
             break;
        case CPLD_QSFPDD_PRES_G0:
             reg = CPLD_QSFPDD_PRES_G0_REG;
             break;
        case CPLD_QSFPDD_PRES_G1:
             reg = CPLD_QSFPDD_PRES_G1_REG;
             break;
        case CPLD_QSFPDD_PRES_G2:
             reg = CPLD_QSFPDD_PRES_G2_REG;
             break;
        case CPLD_QSFPDD_PRES_G3:
             reg = CPLD_QSFPDD_PRES_G3_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_G0:
             reg = CPLD_QSFPDD_FUSE_INT_G0_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_G1:
             reg = CPLD_QSFPDD_FUSE_INT_G1_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_G2:
             reg = CPLD_QSFPDD_FUSE_INT_G2_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_G3:
             reg = CPLD_QSFPDD_FUSE_INT_G3_REG;
             break;
        case CPLD_SFP_TXFAULT:
             reg = CPLD_SFP_TXFAULT_REG;
             break;
        case CPLD_SFP_ABS:
             reg = CPLD_SFP_ABS_REG;
             break;
        case CPLD_SFP_RXLOS:
             reg = CPLD_SFP_RXLOS_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_MASK_G0:
             reg = CPLD_QSFPDD_MOD_INT_MASK_G0_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_MASK_G1:
             reg = CPLD_QSFPDD_MOD_INT_MASK_G1_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_MASK_G2:
             reg = CPLD_QSFPDD_MOD_INT_MASK_G2_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_MASK_G3:
             reg = CPLD_QSFPDD_MOD_INT_MASK_G3_REG;
             break;
        case CPLD_QSFPDD_PRES_MASK_G0:
             reg = CPLD_QSFPDD_PRES_MASK_G0_REG;
             break;
        case CPLD_QSFPDD_PRES_MASK_G1:
             reg = CPLD_QSFPDD_PRES_MASK_G1_REG;
             break;
        case CPLD_QSFPDD_PRES_MASK_G2:
             reg = CPLD_QSFPDD_PRES_MASK_G2_REG;
             break;
        case CPLD_QSFPDD_PRES_MASK_G3:
             reg = CPLD_QSFPDD_PRES_MASK_G3_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_MASK_G0:
             reg = CPLD_QSFPDD_FUSE_INT_MASK_G0_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_MASK_G1:
             reg = CPLD_QSFPDD_FUSE_INT_MASK_G1_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_MASK_G2:
             reg = CPLD_QSFPDD_FUSE_INT_MASK_G2_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_MASK_G3:
             reg = CPLD_QSFPDD_FUSE_INT_MASK_G3_REG;
             break;
        case CPLD_SFP_TXFAULT_MASK:
             reg = CPLD_SFP_TXFAULT_MASK_REG;
             break;
        case CPLD_SFP_ABS_MASK:
             reg = CPLD_SFP_ABS_MASK_REG;
             break;
        case CPLD_SFP_RXLOS_MASK:
             reg = CPLD_SFP_RXLOS_MASK_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_EVENT_G0:
             reg = CPLD_QSFPDD_MOD_INT_EVENT_G0_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_EVENT_G1:
             reg = CPLD_QSFPDD_MOD_INT_EVENT_G1_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_EVENT_G2:
             reg = CPLD_QSFPDD_MOD_INT_EVENT_G2_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_EVENT_G3:
             reg = CPLD_QSFPDD_MOD_INT_EVENT_G3_REG;
             break;
        case CPLD_QSFPDD_PRES_EVENT_G0:
             reg = CPLD_QSFPDD_PRES_EVENT_G0_REG;
             break;
        case CPLD_QSFPDD_PRES_EVENT_G1:
             reg = CPLD_QSFPDD_PRES_EVENT_G1_REG;
             break;
        case CPLD_QSFPDD_PRES_EVENT_G2:
             reg = CPLD_QSFPDD_PRES_EVENT_G2_REG;
             break;
        case CPLD_QSFPDD_PRES_EVENT_G3:
             reg = CPLD_QSFPDD_PRES_EVENT_G3_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_EVENT_G0:
             reg = CPLD_QSFPDD_FUSE_INT_EVENT_G0_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_EVENT_G1:
             reg = CPLD_QSFPDD_FUSE_INT_EVENT_G1_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_EVENT_G2:
             reg = CPLD_QSFPDD_FUSE_INT_EVENT_G2_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_EVENT_G3:
             reg = CPLD_QSFPDD_FUSE_INT_EVENT_G3_REG;
             break;
        case CPLD_SFP_TXFAULT_EVENT:
             reg = CPLD_SFP_TXFAULT_EVENT_REG;
             break;
        case CPLD_SFP_ABS_EVENT:
             reg = CPLD_SFP_ABS_EVENT_REG;
             break;
        case CPLD_SFP_RXLOS_EVENT:
             reg = CPLD_SFP_RXLOS_EVENT_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G0:
             reg = CPLD_QSFPDD_RESET_CTRL_G0_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G1:
             reg = CPLD_QSFPDD_RESET_CTRL_G1_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G2:
             reg = CPLD_QSFPDD_RESET_CTRL_G2_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G3:
             reg = CPLD_QSFPDD_RESET_CTRL_G3_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G0:
             reg = CPLD_QSFPDD_LP_MODE_G0_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G1:
             reg = CPLD_QSFPDD_LP_MODE_G1_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G2:
             reg = CPLD_QSFPDD_LP_MODE_G2_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G3:
             reg = CPLD_QSFPDD_LP_MODE_G3_REG;
             break;
        case CPLD_SFP_TX_DIS:
             reg = CPLD_SFP_TX_DIS_REG;
             break;
        case CPLD_SFP_RS:
             reg = CPLD_SFP_RS_REG;
             break;
        case CPLD_SFP_TS:
             reg = CPLD_SFP_TS_REG;
             break;
        case CPLD_PORT_INT_STATUS:
             reg = CPLD_PORT_INT_STATUS_REG;
             break;
        default:
            return -EINVAL;
    }
    return read_cpld_reg(dev, buf, reg);
}

/* handle read for hw_rev attributes */
static ssize_t read_hw_rev_cb(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = CPLD_HW_REV_REG;
    u8 reg_val = 0;
    int errno = 0;
    u8 res;

    if (!read_cpld_reg_raw_byte(dev, reg, &reg_val, &errno))
        return errno;

    switch (attr->index) {
        case CPLD_HW_REV:
             HW_REV_GET(reg_val, res);
             break;
        case CPLD_DEPH_REV:
             DEPH_REV_GET(reg_val, res);
             break;
        case CPLD_BUILD_REV:
             BUILD_REV_GET(reg_val, res);
             break;
        case CPLD_ID_TYPE:
             ID_TYPE_GET(reg_val, res);
             break;
        default:
            return -EINVAL;
    }
    return sprintf(buf, "0x%02x\n", res);
}

/* handle read for cpld_version attributes */
static ssize_t read_cpld_version_cb(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = CPLD_VERSION_REG;
    u8 reg_val = 0;
    int errno = 0;
    u8 res;

    if (!read_cpld_reg_raw_byte(dev, reg, &reg_val, &errno))
        return errno;

    switch (attr->index) {
        case CPLD_MAJOR_VER:
             CPLD_MAJOR_VERSION_GET(reg_val, res);
             break;
        case CPLD_MINOR_VER:
             CPLD_MINOR_VERSION_GET(reg_val, res);
             break;
        default:
            return -EINVAL;
    }
    return sprintf(buf, "0x%02x\n", res);
}

/* handle read human-readable string for cpld_version attributes */
static ssize_t read_cpld_version_h_cb(struct device *dev,
        struct device_attribute *da, char *buf)
{
    u8 reg = CPLD_VERSION_REG;
    u8 reg_val = 0;
    int errno = 0;
    u8 major, minor, build;

    //get major/minor register value
    if(!read_cpld_reg_raw_byte(dev, reg, &reg_val, &errno))
        return errno;
    CPLD_MAJOR_VERSION_GET(reg_val, major);
    CPLD_MINOR_VERSION_GET(reg_val, minor);

    //get build register value
    reg = CPLD_BUILD_VER_REG;
    if(!read_cpld_reg_raw_byte(dev, reg, &build, &errno))
        return errno;

    //version string format : xx.xx.xxx
    return sprintf(buf, "%d.%02d.%03d\n", major, minor, build);
}

/* handle write for attributes */
static ssize_t write_cpld_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;

    switch (attr->index) {
        case CPLD_MAC_INTR_MASK:
             reg = CPLD_MAC_INTR_MASK_REG;
             break;
        case CPLD_10G_PHY_INTR_MASK:
             reg = CPLD_10G_PHY_INTR_MASK_REG;
             break;
        case CPLD_CPLD_FRU_INTR_MASK:
             reg = CPLD_CPLD_FRU_INTR_MASK_REG;
             break;
        case CPLD_THERMAL_ALERT_INTR_MASK:
             reg = CPLD_THERMAL_ALERT_INTR_MASK_REG;
             break;
        case CPLD_MISC_INTR_MASK:
             reg = CPLD_MISC_INTR_MASK_REG;
             break;
        case CPLD_MAC_RST:
             reg = CPLD_MAC_RST_REG;
             break;
        case CPLD_10G_PHY_RST:
             reg = CPLD_10G_PHY_RST_REG;
             break;
        case CPLD_BMC_RST:
             reg = CPLD_BMC_RST_REG;
             break;
        case CPLD_USB_RST:
             reg = CPLD_USB_RST_REG;
             break;
        case CPLD_MUX_RST:
             reg = CPLD_MUX_RST_REG;
             break;
        case CPLD_MISC_RST:
             reg = CPLD_MISC_RST_REG;
             break;
        case CPLD_BMC_WATCHDOG:
             reg = CPLD_BMC_WATCHDOG_REG;
             break;
        case CPLD_MUX_CTRL:
             reg = CPLD_MUX_CTRL_REG;
             break;
        case CPLD_MAC_QSFP_SEL_CTRL:
             reg = CPLD_MAC_QSFP_SEL_CTRL_REG;
             break;
        case CPLD_BEACON_LED_CTRL:
             reg = CPLD_BEACON_LED_CTRL_REG;
             break;
        case CPLD_PORT_LED_CLR_CTRL:
             reg = CPLD_PORT_LED_CLR_CTRL_REG;
             break;
        case CPLD_EVENT_DETECT_CTRL:
             reg = CPLD_EVENT_DETECT_CTRL_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_MASK_G0:
             reg = CPLD_QSFPDD_MOD_INT_MASK_G0_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_MASK_G1:
             reg = CPLD_QSFPDD_MOD_INT_MASK_G1_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_MASK_G2:
             reg = CPLD_QSFPDD_MOD_INT_MASK_G2_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_MASK_G3:
             reg = CPLD_QSFPDD_MOD_INT_MASK_G3_REG;
             break;
        case CPLD_QSFPDD_PRES_MASK_G0:
             reg = CPLD_QSFPDD_PRES_MASK_G0_REG;
             break;
        case CPLD_QSFPDD_PRES_MASK_G1:
             reg = CPLD_QSFPDD_PRES_MASK_G1_REG;
             break;
        case CPLD_QSFPDD_PRES_MASK_G2:
             reg = CPLD_QSFPDD_PRES_MASK_G2_REG;
             break;
        case CPLD_QSFPDD_PRES_MASK_G3:
             reg = CPLD_QSFPDD_PRES_MASK_G3_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_MASK_G0:
             reg = CPLD_QSFPDD_FUSE_INT_MASK_G0_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_MASK_G1:
             reg = CPLD_QSFPDD_FUSE_INT_MASK_G1_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_MASK_G2:
             reg = CPLD_QSFPDD_FUSE_INT_MASK_G2_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_MASK_G3:
             reg = CPLD_QSFPDD_FUSE_INT_MASK_G3_REG;
             break;
        case CPLD_SFP_TXFAULT_MASK:
             reg = CPLD_SFP_TXFAULT_MASK_REG;
             break;
        case CPLD_SFP_ABS_MASK:
             reg = CPLD_SFP_ABS_MASK_REG;
             break;
        case CPLD_SFP_RXLOS_MASK:
             reg = CPLD_SFP_RXLOS_MASK_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G0:
             reg = CPLD_QSFPDD_RESET_CTRL_G0_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G1:
             reg = CPLD_QSFPDD_RESET_CTRL_G1_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G2:
             reg = CPLD_QSFPDD_RESET_CTRL_G2_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G3:
             reg = CPLD_QSFPDD_RESET_CTRL_G3_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G0:
             reg = CPLD_QSFPDD_LP_MODE_G0_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G1:
             reg = CPLD_QSFPDD_LP_MODE_G1_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G2:
             reg = CPLD_QSFPDD_LP_MODE_G2_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G3:
             reg = CPLD_QSFPDD_LP_MODE_G3_REG;
             break;
        case CPLD_SFP_TX_DIS:
             reg = CPLD_SFP_TX_DIS_REG;
             break;
        case CPLD_SFP_RS:
             reg = CPLD_SFP_RS_REG;
             break;
        case CPLD_SFP_TS:
             reg = CPLD_SFP_TS_REG;
             break;
        default:
            return -EINVAL;
    }
    return write_cpld_reg(dev, buf, count, reg);
}

/* set cpld register value */
static ssize_t write_cpld_reg(struct device *dev,
                    const char *buf,
                    size_t count,
                    u8 reg)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
               client, reg, reg_val);

    if (unlikely(ret < 0)) {
        dev_err(dev, "I2C_WRITE_BYTE_DATA error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* add valid cpld client to list */
static void s9300_cpld_add_client(struct i2c_client *client)
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
static void s9300_cpld_remove_client(struct i2c_client *client)
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
static int s9300_cpld_probe(struct i2c_client *client,
                    const struct i2c_device_id *dev_id)
{
#else
static int s9300_cpld_probe(struct i2c_client *client)
{
    const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
#endif
    int status;
    struct cpld_data *data = NULL;
    int ret = -EPERM;
    int idx;

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

    CPLD_ID_ID_GET(ret,  idx);

    if (INVALID(idx, cpld1, cpld3)) {
        dev_info(&client->dev,
            "cpld id %d(device) not valid\n", idx);
        //status = -EPERM;
        //goto exit;
    }

    data->index = dev_id->driver_data;

    /* register sysfs hooks for different cpld group */
    dev_info(&client->dev, "probe cpld with index %d\n", data->index);
    switch (data->index) {
    case cpld1:
        status = sysfs_create_group(&client->dev.kobj,
                    &s9300_cpld1_group);
        break;
    case cpld2:
        status = sysfs_create_group(&client->dev.kobj,
                    &s9300_cpld2_group);
        break;
    case cpld3:
        status = sysfs_create_group(&client->dev.kobj,
                    &s9300_cpld3_group);
        break;
    default:
        status = -EINVAL;
    }

    if (status)
        goto exit;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    s9300_cpld_add_client(client);

    return 0;
exit:
    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &s9300_cpld1_group);
        break;
    case cpld2:
    	  sysfs_remove_group(&client->dev.kobj, &s9300_cpld2_group);
        break;
    case cpld3:
    	  sysfs_remove_group(&client->dev.kobj, &s9300_cpld3_group);
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
s9300_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);

    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &s9300_cpld1_group);
        break;
    case cpld2:
    	  sysfs_remove_group(&client->dev.kobj, &s9300_cpld2_group);
        break;
    case cpld3:
    	  sysfs_remove_group(&client->dev.kobj, &s9300_cpld3_group);
        break;
    }

    s9300_cpld_remove_client(client);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
    return 0;
#endif
}

MODULE_DEVICE_TABLE(i2c, s9300_cpld_id);

static struct i2c_driver s9300_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9300_32d_cpld",
    },
    .probe = s9300_cpld_probe,
    .remove = s9300_cpld_remove,
    .id_table = s9300_cpld_id,
    .address_list = cpld_i2c_addr,
};

/* provide cpld register read */
/* cpld_idx indicate the index of cpld device */
int s9300_cpld_read(u8 cpld_idx,
                u8 reg)
{
    struct list_head *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;
    struct cpld_data *data;

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                    struct cpld_client_node, list);
        data = i2c_get_clientdata(cpld_node->client);
        if (data->index == cpld_idx) {
            DEBUG_PRINT("cpld_idx=%d, read reg 0x%02x",
                    cpld_idx, reg);
            I2C_READ_BYTE_DATA(ret, &data->access_lock,
                    cpld_node->client, reg);
            DEBUG_PRINT("cpld_idx=%d, read reg 0x%02x = 0x%02x",
                    cpld_idx, reg, ret);
        break;
        }
    }

    return ret;
}
EXPORT_SYMBOL(s9300_cpld_read);

/* provide cpld register write */
/* cpld_idx indicate the index of cpld device */
int s9300_cpld_write(u8 cpld_idx,
                u8 reg,
                u8 value)
{
    struct list_head *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;
    struct cpld_data *data;

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                    struct cpld_client_node, list);
        data = i2c_get_clientdata(cpld_node->client);

        if (data->index == cpld_idx) {
                        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                        cpld_node->client,
                        reg, value);
            DEBUG_PRINT("cpld_idx=%d, write reg 0x%02x val 0x%02x, ret=%d",
                            cpld_idx, reg, value, ret);
            break;
        }
    }

    return ret;
}
EXPORT_SYMBOL(s9300_cpld_write);

static int __init s9300_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&s9300_cpld_driver);
}

static void __exit s9300_cpld_exit(void)
{
    i2c_del_driver(&s9300_cpld_driver);
}

MODULE_AUTHOR("Leo Lin <leo.yt.lin@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9300_cpld driver");
MODULE_LICENSE("GPL");

module_init(s9300_cpld_init);
module_exit(s9300_cpld_exit);
