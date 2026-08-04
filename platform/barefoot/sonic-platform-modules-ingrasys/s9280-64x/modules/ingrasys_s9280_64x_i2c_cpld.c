/*
 * A i2c cpld driver for the ingrasys_s9280_64x
 *
 * Copyright (C) 2017 Ingrasys Technology Corporation.
 * Leo Lin <feng.lee.usa@ingrasys.com>
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
#include "ingrasys_s9280_64x_i2c_cpld.h"

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) \
    printk(KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define I2C_READ_BYTE_DATA(ret, lock, i2c_client, reg) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_read_byte_data(i2c_client, reg); \
    mutex_unlock(lock); \
}
#define I2C_WRITE_BYTE_DATA(ret, lock, i2c_client, reg, val) \
{ \
        mutex_lock(lock); \
        ret = i2c_smbus_write_byte_data(i2c_client, reg, val); \
        mutex_unlock(lock); \
}

/* CPLD sysfs attributes index  */
enum s9280_64x_cpld_sysfs_attributes {
    CPLD_ACCESS_REG,
    CPLD_REGISTER_VAL,
    CPLD_PORT_START,
    CPLD_PORTS,
    CPLD_VERSION,
    CPLD_ID,
    CPLD_BOARD_TYPE,
    CPLD_EXT_BOARD_TYPE,
    CPLD_QSFP_PORT_STATUS_1,
    CPLD_QSFP_PORT_STATUS_2,
    CPLD_QSFP_PORT_STATUS_3,
    CPLD_QSFP_PORT_STATUS_4,
    CPLD_QSFP_PORT_STATUS_5,
    CPLD_QSFP_PORT_STATUS_6,
    CPLD_QSFP_PORT_STATUS_7,
    CPLD_QSFP_PORT_STATUS_8,
    CPLD_QSFP_PORT_STATUS_9,
    CPLD_QSFP_PORT_STATUS_10,
    CPLD_QSFP_PORT_STATUS_11,
    CPLD_QSFP_PORT_STATUS_12,
    CPLD_QSFP_PORT_STATUS_13,
    CPLD_QSFP_PORT_CONFIG_1,
    CPLD_QSFP_PORT_CONFIG_2,
    CPLD_QSFP_PORT_CONFIG_3,
    CPLD_QSFP_PORT_CONFIG_4,
    CPLD_QSFP_PORT_CONFIG_5,
    CPLD_QSFP_PORT_CONFIG_6,
    CPLD_QSFP_PORT_CONFIG_7,
    CPLD_QSFP_PORT_CONFIG_8,
    CPLD_QSFP_PORT_CONFIG_9,
    CPLD_QSFP_PORT_CONFIG_10,
    CPLD_QSFP_PORT_CONFIG_11,
    CPLD_QSFP_PORT_CONFIG_12,
    CPLD_QSFP_PORT_CONFIG_13,
    CPLD_QSFP_PORT_INTERRUPT,
    CPLD_SFP_PORT_STATUS,
    CPLD_SFP_PORT_CONFIG,
    CPLD_10GMUX_CONFIG,
    CPLD_BMC_STATUS,
    CPLD_BMC_WATCHDOG,
    CPLD_USB_STATUS,
    CPLD_RESET_CONTROL,
    CPLD_SFP_LED,
    CPLD_SFP_LED_BLINK,
    CPLD_QSFP_LED_1,
    CPLD_QSFP_LED_2,
    CPLD_QSFP_LED_3,
    CPLD_QSFP_LED_4,
    CPLD_QSFP_LED_5,
    CPLD_QSFP_LED_6,
    CPLD_QSFP_LED_7,
    CPLD_QSFP_LED_8,
    CPLD_QSFP_LED_9,
    CPLD_QSFP_LED_10,
    CPLD_QSFP_LED_11,
    CPLD_QSFP_LED_12,
    CPLD_QSFP_LED_13,
    CPLD_QSFP_LED_14,
    CPLD_QSFP_LED_15,
    CPLD_QSFP_LED_16,
    CPLD_QSFP_LED_BLINK,
    CPLD_RTMR_RESET,
    CPLD_ROV_STATUS,

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
static ssize_t get_qsfp_port_start(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t get_qsfp_ports(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_cpld_version(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_cpld_id(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_board_type(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_ext_board_type(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_qsfp_port_status(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_qsfp_port_config(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_qsfp_port_config(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_qsfp_port_interrupt(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_sfp_port_status(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_sfp_port_config(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_sfp_port_config(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_10gmux_config(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_10gmux_config(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_bmc_status(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_bmc_watchdog(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_bmc_watchdog(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_usb_status(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_reset_control(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_reset_control(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_sfp_led(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_sfp_led(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_sfp_led_blink(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_sfp_led_blink(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_qsfp_led(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_qsfp_led(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_qsfp_led_blink(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_qsfp_led_blink(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_rtmr_reset(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_rtmr_reset(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_rov_status(struct device *dev,
                struct device_attribute *da, char *buf);

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
static const struct i2c_device_id ingrasys_i2c_cpld_id[] = {
    { "ingrasys_cpld1",  cpld1 },
    { "ingrasys_cpld2",  cpld2 },
    { "ingrasys_cpld3",  cpld3 },
    { "ingrasys_cpld4",  cpld4 },
    { "ingrasys_cpld5",  cpld5 },
    {}
};

/* Addresses scanned for ingrasys_i2c_cpld */
static const unsigned short cpld_i2c_addr[] = { 0x33, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */
static SENSOR_DEVICE_ATTR(cpld_access_register, S_IWUSR | S_IRUGO,
        read_access_register, write_access_register, CPLD_ACCESS_REG);
static SENSOR_DEVICE_ATTR(cpld_register_value, S_IWUSR | S_IRUGO,
        read_register_value, write_register_value, CPLD_REGISTER_VAL);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_start, S_IRUGO,
            get_qsfp_port_start, NULL, CPLD_PORT_START);
static SENSOR_DEVICE_ATTR(cpld_qsfp_ports, S_IRUGO,
                get_qsfp_ports, NULL, CPLD_PORTS);
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO,
                read_cpld_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(cpld_id, S_IRUGO, read_cpld_id, NULL, CPLD_ID);
static SENSOR_DEVICE_ATTR(cpld_board_type, S_IRUGO,
                read_board_type, NULL, CPLD_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(cpld_ext_board_type, S_IRUGO,
        read_ext_board_type, NULL, CPLD_EXT_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_1, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_2, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_2);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_3, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_3);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_4, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_4);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_5, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_5);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_6, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_6);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_7, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_7);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_8, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_8);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_9, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_9);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_10, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_10);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_11, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_11);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_12, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_12);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_status_13, S_IRUGO,
        read_qsfp_port_status, NULL, CPLD_QSFP_PORT_STATUS_13);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_1, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config, 
                CPLD_QSFP_PORT_CONFIG_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_2, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_2);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_3, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_3);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_4, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_4);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_5, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_5);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_6, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_6);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_7, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_7);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_8, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_8);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_9, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_9);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_10, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_10);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_11, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_11);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_12, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_12);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_config_13, S_IWUSR | S_IRUGO,
                read_qsfp_port_config, write_qsfp_port_config,
                CPLD_QSFP_PORT_CONFIG_13);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_interrupt, S_IRUGO,
        read_qsfp_port_interrupt, NULL, CPLD_QSFP_PORT_INTERRUPT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_status, S_IRUGO,
            read_sfp_port_status, NULL, CPLD_SFP_PORT_STATUS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_config, S_IWUSR | S_IRUGO,
    read_sfp_port_config, write_sfp_port_config, CPLD_SFP_PORT_CONFIG);
static SENSOR_DEVICE_ATTR(cpld_10gmux_config, S_IWUSR | S_IRUGO,
                read_10gmux_config, write_10gmux_config,
                CPLD_10GMUX_CONFIG);
static SENSOR_DEVICE_ATTR(cpld_bmc_status, S_IRUGO,
                read_bmc_status, NULL, CPLD_BMC_STATUS);
static SENSOR_DEVICE_ATTR(cpld_bmc_watchdog, S_IWUSR | S_IRUGO,
                read_bmc_watchdog, write_bmc_watchdog,
                CPLD_BMC_WATCHDOG);
static SENSOR_DEVICE_ATTR(cpld_usb_status, S_IRUGO,
                read_usb_status, NULL, CPLD_USB_STATUS);
static SENSOR_DEVICE_ATTR(cpld_reset_control, S_IWUSR | S_IRUGO,
                read_reset_control, write_reset_control,
                CPLD_BMC_WATCHDOG);
static SENSOR_DEVICE_ATTR(cpld_sfp_led, S_IWUSR | S_IRUGO,
                read_sfp_led, write_sfp_led, CPLD_SFP_LED);
static SENSOR_DEVICE_ATTR(cpld_sfp_led_blink, S_IWUSR | S_IRUGO,
                read_sfp_led_blink, write_sfp_led_blink, CPLD_SFP_LED_BLINK);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_1, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_2, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_2);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_3, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_3);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_4, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_4);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_5, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_5);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_6, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_6);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_7, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_7);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_8, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_8);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_9, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_9);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_10, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_10);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_11, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_11);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_12, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_12);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_13, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_13);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_14, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_14);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_15, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_15);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_16, S_IWUSR | S_IRUGO,
                read_qsfp_led, write_qsfp_led, CPLD_QSFP_LED_16);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_blink, S_IWUSR | S_IRUGO,
                read_qsfp_led_blink, write_qsfp_led_blink, CPLD_QSFP_LED_BLINK);
static SENSOR_DEVICE_ATTR(cpld_rtmr_reset, S_IWUSR | S_IRUGO,
                read_rtmr_reset, write_rtmr_reset, CPLD_RTMR_RESET);
static SENSOR_DEVICE_ATTR(cpld_rov_status, S_IRUGO,
                read_rov_status, NULL, CPLD_ROV_STATUS);

/* define support attributes of cpldx , total 5 */
/* cpld 1 */
static struct attribute *s9280_64x_cpld1_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_start.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_ports.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_board_type.dev_attr.attr,
    &sensor_dev_attr_cpld_ext_board_type.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_11.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_12.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_11.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_12.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_interrupt.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_status.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_config.dev_attr.attr,
    &sensor_dev_attr_cpld_10gmux_config.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_status.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_watchdog.dev_attr.attr,
    &sensor_dev_attr_cpld_usb_status.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_control.dev_attr.attr,    
    &sensor_dev_attr_cpld_sfp_led.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_led_blink.dev_attr.attr,
    &sensor_dev_attr_cpld_rtmr_reset.dev_attr.attr,
    &sensor_dev_attr_cpld_rov_status.dev_attr.attr,
    NULL
};

/* cpld 2 / cpld 3 / cpld 4 / cpld 5 */
static struct attribute *s9280_64x_cpld2345_attributes[] = {
&sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_start.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_ports.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_11.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_12.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_status_13.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_11.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_12.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_config_13.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_interrupt.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_11.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_12.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_13.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_14.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_15.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_16.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_blink.dev_attr.attr,
    NULL
};

/* cpld 1 attributes group */
static const struct attribute_group s9280_64x_cpld1_group = {
    .attrs = s9280_64x_cpld1_attributes,
};
/* cpld 2/3/4/5 attributes group */
static const struct attribute_group s9280_64x_cpld2345_group = {
    .attrs = s9280_64x_cpld2345_attributes,
};

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
        return -1;

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

    return count;
}

/* get qsfp port start number of the cpld device */
/* the start number use to tranlate qsfp port to cpld port */
/* the cpld port use to access the qsfp port register in cpld */
static ssize_t get_qsfp_port_start(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int port_base_num;

    if (attr->index == CPLD_PORT_START) {
        if (data->index == cpld1) {
            port_base_num = 1;
        } else {
            port_base_num = CPLD_1_PORT_NUM +
                CPLD_2_PORT_NUM*(data->index - 1) + 1;
        }
        return sprintf(buf, "%d\n", port_base_num);
    }
    return -1;
}

/* get total qsfp port which contain register in the cpld device */
static ssize_t get_qsfp_ports(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int ports;

    if (attr->index == CPLD_PORTS) {
        if (data->index == cpld1)
            ports = CPLD_1_PORT_NUM;
        else
            ports = CPLD_2_PORT_NUM;
        return sprintf(buf, "%d\n", ports);
    }
    return -1;
}

/* get cpdl version register value */
static ssize_t read_cpld_version(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_VERSION) {
        reg = CPLD_VERSION_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get cpdl id register value */
static ssize_t read_cpld_id(struct device *dev,
                struct device_attribute *da,
                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_ID) {
        reg = CPLD_ID_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get board type register value */
static ssize_t read_board_type(struct device *dev,
                struct device_attribute *da,
                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_BOARD_TYPE) {
        reg = CPLD_BOARD_TYPE_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get extend board type register value */
static ssize_t read_ext_board_type(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_EXT_BOARD_TYPE) {
        reg = CPLD_EXT_BOARD_TYPE_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get qsfp port status register value */
static ssize_t read_qsfp_port_status(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index >= CPLD_QSFP_PORT_STATUS_1 &&
        attr->index <= CPLD_QSFP_PORT_STATUS_13) {
        reg = CPLD_QSFP_PORT_STATUS_BASE_REG +
            (attr->index - CPLD_QSFP_PORT_STATUS_1);
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get qsfp port config register value */
static ssize_t read_qsfp_port_config(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index >= CPLD_QSFP_PORT_CONFIG_1 &&
        attr->index <= CPLD_QSFP_PORT_CONFIG_13) {
        reg = CPLD_QSFP_PORT_CONFIG_BASE_REG +
            (attr->index - CPLD_QSFP_PORT_CONFIG_1);
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to qsfp port config register */
static ssize_t write_qsfp_port_config(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index >= CPLD_QSFP_PORT_CONFIG_1 &&
        attr->index <= CPLD_QSFP_PORT_CONFIG_13) {
        reg = CPLD_QSFP_PORT_CONFIG_BASE_REG +
            (attr->index - CPLD_QSFP_PORT_CONFIG_1);
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get qsfp port interrupt register value */
static ssize_t read_qsfp_port_interrupt(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_QSFP_PORT_INTERRUPT) {
        reg = CPLD_QSFP_PORT_INTERRUPT_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get sfp port status register value */
static ssize_t read_sfp_port_status(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_SFP_PORT_STATUS) {
        reg = CPLD_SFP_PORT_STATUS_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get sfp port config register value */
static ssize_t read_sfp_port_config(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_SFP_PORT_CONFIG) {
        reg = CPLD_SFP_PORT_CONFIG_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to sfp port config register */
static ssize_t write_sfp_port_config(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_SFP_PORT_CONFIG) {
        reg = CPLD_SFP_PORT_CONFIG_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get 10g mux config register value */
static ssize_t read_10gmux_config(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_10GMUX_CONFIG) {
        reg = CPLD_10GMUX_CONFIG_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to 10g mux config register */
static ssize_t write_10gmux_config(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_10GMUX_CONFIG) {
        reg = CPLD_10GMUX_CONFIG_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get bmc status register value */
static ssize_t read_bmc_status(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_BMC_STATUS) {
        reg = CPLD_BMC_STATUS_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get bmc watchdog register value */
static ssize_t read_bmc_watchdog(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_BMC_WATCHDOG) {
        reg = CPLD_BMC_WATCHDOG_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to bmc watchdog register */
static ssize_t write_bmc_watchdog(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_BMC_WATCHDOG) {
        reg = CPLD_BMC_WATCHDOG_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get usb status register value */
static ssize_t read_usb_status(struct device *dev,
                struct device_attribute *da,
                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_USB_STATUS) {
        reg = CPLD_USB_STATUS_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get reset control register value */
static ssize_t read_reset_control(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_RESET_CONTROL) {
        reg = CPLD_RESET_CONTROL_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to reset control register */
static ssize_t write_reset_control(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_RESET_CONTROL) {
        reg = CPLD_RESET_CONTROL_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get sfp port 0/1 led register */
static ssize_t read_sfp_led(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_SFP_LED) {
        reg = CPLD_SFP_LED_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to sfp 0/1 port led register */
static ssize_t write_sfp_led(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_SFP_LED) {
        reg = CPLD_SFP_LED_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get sfp port 0/1 led blink register */
static ssize_t read_sfp_led_blink(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_SFP_LED_BLINK) {
        reg = CPLD_SFP_LED_BLINK_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to sfp port  0/1 led blink register */
static ssize_t write_sfp_led_blink(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_SFP_LED_BLINK) {
        reg = CPLD_SFP_LED_BLINK_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get qsfp port led register */
static ssize_t read_qsfp_led(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index >= CPLD_QSFP_LED_1 &&
        attr->index <= CPLD_QSFP_LED_16) {
        reg = CPLD_QSFP_LED_BASE_REG + (attr->index - CPLD_QSFP_LED_1);
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to qsfp port led register */
static ssize_t write_qsfp_led(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index >= CPLD_QSFP_LED_1 &&
        attr->index <= CPLD_QSFP_LED_16) {
        reg = CPLD_QSFP_LED_BASE_REG + (attr->index - CPLD_QSFP_LED_1);
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get qsfp 16 port led blink register value in 64 bit map */
/*
    each register for 2 port, each port has 4 channel for led blink
    bit   64       56       48       40      32     24     16     8     0
    port   16/15  14/13  12/11  10/9   8/7   6/5   4/3  2/1  
  */
static ssize_t read_qsfp_led_blink(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val, i;
    u64 bitmap = 0;

    if (attr->index == CPLD_QSFP_LED_BLINK) {
        for (i = 0; i  <= 7; i++) {
            reg = CPLD_QSFP_LED_BLINK_BASE_REG + i;
            I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
            if (reg_val < 0)
                return -1;
            reg_val = reg_val & 0xff;
            bitmap =  bitmap | (reg_val<<(i*8));
        }
        return sprintf(buf, "0x%llx\n", bitmap);
    }
    return -1;
}

/* set 64 bit map value to qsfp port led blink register */
/*
    each register for 2 port, each port has 4 channel for led blink
    bit   63     56     48     40    32   24   16   8   0
    port    16/15  14/13  12/11  10/9  8/7  6/5  4/3 2/1  
  */
static ssize_t write_qsfp_led_blink(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val, i;
    int ret;
    u64 bitmap = 0;

    if (kstrtou64(buf, 0, &bitmap) < 0)
        return -EINVAL;

    if (attr->index == CPLD_QSFP_LED_BLINK) {
        for (i = 0; i  <= 7; i++) {
            reg = CPLD_QSFP_LED_BLINK_BASE_REG + i;
            reg_val = (u8)((bitmap >> i*8) & 0xFF);
            I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
        }
    }
    return count;
}

/* get retimer reset register */
static ssize_t read_rtmr_reset(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_RTMR_RESET) {
        reg = CPLD_RTMR_RESET_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to retimer reset register */
static ssize_t write_rtmr_reset(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_RTMR_RESET) {
        reg = CPLD_RTMR_RESET_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get rov status register */
static ssize_t read_rov_status(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_ROV_STATUS) {
        reg = CPLD_ROV_STATUS_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* add valid cpld client to list */
static void ingrasys_i2c_cpld_add_client(struct i2c_client *client)
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
static void ingrasys_i2c_cpld_remove_client(struct i2c_client *client)
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
static int ingrasys_i2c_cpld_probe(struct i2c_client *client,
                    const struct i2c_device_id *dev_id)
{
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

    if (INVALID(idx, cpld1, cpld5)) {
        dev_info(&client->dev,
            "cpld id %d(device) not valid\n", idx);
        //status = -EPERM;
        //goto exit;
    }

#if 0
    /* change client name for each cpld with index */
    snprintf(client->name, sizeof(client->name), "%s_%d", client->name,
            data->index);
#endif

              data->index = dev_id->driver_data;

    /* register sysfs hooks for different cpld group */
    dev_info(&client->dev, "probe cpld with index %d\n", data->index);
    switch (data->index) {
    case cpld1:
        status = sysfs_create_group(&client->dev.kobj,
                    &s9280_64x_cpld1_group);
        break;
    case cpld2:
    case cpld3:
    case cpld4:
    case cpld5:
        status = sysfs_create_group(&client->dev.kobj,
                    &s9280_64x_cpld2345_group);
        break;
    default:
        status = -EINVAL;
    }

    if (status)
        goto exit;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    ingrasys_i2c_cpld_add_client(client);

    return 0;
exit:
    sysfs_remove_group(&client->dev.kobj, &s9280_64x_cpld2345_group);
    return status;
}

/* cpld drvier remove */
static int ingrasys_i2c_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);

    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &s9280_64x_cpld1_group);
        break;
    case cpld2:
    case cpld3:
    case cpld4:
    case cpld5:
        sysfs_remove_group(&client->dev.kobj,
                    &s9280_64x_cpld2345_group);
        break;
    }

    ingrasys_i2c_cpld_remove_client(client);
    return 0;
}

MODULE_DEVICE_TABLE(i2c, ingrasys_i2c_cpld_id);

static struct i2c_driver ingrasys_i2c_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "ingrasys_i2c_cpld",
    },
    .probe = ingrasys_i2c_cpld_probe,
    .remove = ingrasys_i2c_cpld_remove,
    .id_table = ingrasys_i2c_cpld_id,
    .address_list = cpld_i2c_addr,
};

/* provid cpld register read */
/* cpld_idx indicate the index of cpld device */
int ingrasys_i2c_cpld_read(u8 cpld_idx,
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
EXPORT_SYMBOL(ingrasys_i2c_cpld_read);

/* provid cpld register write */
/* cpld_idx indicate the index of cpld device */
int ingrasys_i2c_cpld_write(u8 cpld_idx,
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
EXPORT_SYMBOL(ingrasys_i2c_cpld_write);

/* provid qsfp port status register read */
/* port_num indicate the front panel qsfp port number */
int ingrasys_i2c_cpld_get_qsfp_port_status_val(u8 port_num)
{
    u8 cpld_idx, cpld_port, reg;
    int reg_val;

    if (INVALID(port_num, QSFP_MIN_PORT_NUM, QSFP_MAX_PORT_NUM)) {
        DEBUG_PRINT("invalid input value %d", port_num);
        return -1;
    }
    QSFP_TO_CPLD_IDX(port_num, cpld_idx, cpld_port);
    reg = QSFP_PORT_STATUS_REG(cpld_port);
    DEBUG_PRINT("port_num=%d, cpld_idx=%d, cpld_port=%d, reg=0x%x",
                    port_num, cpld_idx, cpld_port, reg);
    reg_val = ingrasys_i2c_cpld_read(cpld_idx, reg);
    return reg_val;
}
EXPORT_SYMBOL(ingrasys_i2c_cpld_get_qsfp_port_status_val);

/* provid qsfp port config register read */
/* port_num indicate the front panel qsfp port number */
int ingrasys_i2c_cpld_get_qsfp_port_config_val(u8 port_num)
{
    u8 cpld_idx, cpld_port, reg;
    int reg_val;

    if (INVALID(port_num, QSFP_MIN_PORT_NUM, QSFP_MAX_PORT_NUM)) {
        DEBUG_PRINT("invalid input value %d", port_num);
        return -1;
    }
    QSFP_TO_CPLD_IDX(port_num, cpld_idx, cpld_port);
    reg = QSFP_PORT_CONFIG_REG(cpld_port);
    DEBUG_PRINT("port_num=%d, cpld_idx=%d, cpld_port=%d, reg=0x%x",
                    port_num, cpld_idx, cpld_port, reg);
    reg_val = ingrasys_i2c_cpld_read(cpld_idx, reg);
    return reg_val;
}
EXPORT_SYMBOL(ingrasys_i2c_cpld_get_qsfp_port_config_val);

/* provid qsfp port config register write */
/* port_num indicate the front panel qsfp port number */
int ingrasys_i2c_cpld_set_qsfp_port_config_val(u8 port_num,
                            u8 reg_val)
{
    u8 cpld_idx, cpld_port, reg, ret;

    if (INVALID(port_num, QSFP_MIN_PORT_NUM, QSFP_MAX_PORT_NUM)) {
        DEBUG_PRINT("invalid input value %d", port_num);
        return -1;
    }
    QSFP_TO_CPLD_IDX(port_num, cpld_idx, cpld_port);
    reg = QSFP_PORT_CONFIG_REG(cpld_port);
    DEBUG_PRINT("port_num=%d, cpld_idx=%d, cpld_port=%d, reg=0x%x",
                    port_num, cpld_idx, cpld_port, reg);
    ret = ingrasys_i2c_cpld_write(cpld_idx, reg, reg_val);
    return ret;
}
EXPORT_SYMBOL(ingrasys_i2c_cpld_set_qsfp_port_config_val);

/* provid sfp port 0/1 status register read */
int ingrasys_i2c_cpld_get_sfp_port_status_val(void)
{
    u8 cpld_idx, reg;
    int reg_val;

    cpld_idx = cpld1;
    reg = CPLD_SFP_PORT_STATUS_REG;
    DEBUG_PRINT("cpld_idx=%d, reg=0x%x",
                    cpld_idx, reg);
    reg_val = ingrasys_i2c_cpld_read(cpld_idx, reg);
    return reg_val;
}
EXPORT_SYMBOL(ingrasys_i2c_cpld_get_sfp_port_status_val);

/* provid qsfp port config register read */
/* port_num indicate the front panel qsfp port number */
int ingrasys_i2c_cpld_get_sfp_port_config_val(void)
{
    u8 cpld_idx, reg;
    int reg_val;

    cpld_idx = cpld1;
    reg = CPLD_SFP_PORT_CONFIG_REG;
    DEBUG_PRINT("cpld_idx=%d, reg=0x%x",
                cpld_idx, reg);
    reg_val = ingrasys_i2c_cpld_read(cpld_idx, reg);
    return reg_val;
}
EXPORT_SYMBOL(ingrasys_i2c_cpld_get_sfp_port_config_val);

/* provid qsfp port config register write */
/* port_num indicate the front panel qsfp port number */
int ingrasys_i2c_cpld_set_sfp_port_config_val(u8 reg_val)
{
    u8 cpld_idx, reg, ret;

    cpld_idx = cpld1;
    reg = CPLD_SFP_PORT_CONFIG_REG;
    DEBUG_PRINT("cpld_idx=%d, reg=0x%x",
                cpld_idx, reg);
    ret = ingrasys_i2c_cpld_write(cpld_idx, reg, reg_val);
    return ret;
}
EXPORT_SYMBOL(ingrasys_i2c_cpld_set_sfp_port_config_val);

static int __init ingrasys_i2c_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&ingrasys_i2c_cpld_driver);
}

static void __exit ingrasys_i2c_cpld_exit(void)
{
    i2c_del_driver(&ingrasys_i2c_cpld_driver);
}

MODULE_AUTHOR("Leo Lin <feng.lee.usa@ingrasys.com>");
MODULE_DESCRIPTION("ingrasys_i2c_cpld driver");
MODULE_LICENSE("GPL");

module_init(ingrasys_i2c_cpld_init);
module_exit(ingrasys_i2c_cpld_exit);

