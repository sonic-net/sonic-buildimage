/*
 * An I2C multiplexer dirver for alphanetworks snj60d0-320f CPLD
 *
 * Copyright (C) 2021 Alphanetworks Technology Corporation.
 * Fillmore Chen <fillmore_chen@alphanetworks.com>
 *
 * Based on:
 *
 * Copyright (C) 2015 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * Based on:
 *    pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *    pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *    i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *    pca9540.c from Jean Delvare <khali@linux-fr.org>.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>
#include <linux/printk.h>

#define DRIVER_NAME                       "snj60d0_i2c_mux_cpld"
#define I2C_RW_RETRY_COUNT                10
#define I2C_RW_RETRY_INTERVAL             60 /* ms */

#define NUM_OF_CPLD1_CHANS                0x8
#define NUM_OF_CPLD2_CHANS                0x8
#define NUM_OF_CPLD3_CHANS                0x8
#define NUM_OF_CPLD4_CHANS                0x8

#define CPLD_CHANNEL_SELECT_REG           0x06
#define CPLD_DESELECT_CHANNEL             0x00

#define CPLD_REG_ADDR_REVISION            0x00
#define CPLD_REG_ADDR_INTR                0x01
#define CPLD_REG_ADDR_PRESENT             0x03
#define CPLD_REG_ADDR_RESET               0x04
#define CPLD_REG_ADDR_LOWPOWERMODE        0x05
#define CPLD_REG_ADDR_MODSELECT           0x06
#define CPLD_REG_ADDR_PORT_LED_CONTROL    0x07

#define DEBUG_MODE 1

static unsigned int debug = 0;
module_param(debug, uint, S_IRUGO);
MODULE_PARM_DESC(debug, "Set DEBUG mode. Default is disabled.");


#define DEBUG_PRINT(fmt, args...)                                        \
    if (debug == 1)                                                      \
		printk (KERN_INFO "[%s,%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)

static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

enum cpld_mux_type {
    snj60d0_320f_cpld1,
    snj60d0_320f_cpld2,
    snj60d0_320f_cpld3,
    snj60d0_320f_cpld4
};

struct snj60d0_320f_cpld_data {
    enum cpld_mux_type type;
    struct i2c_client *client;
    u8 last_chan;  /* last register value */

    struct device      *hwmon_dev;
    struct mutex        update_lock;
};

struct chip_desc {
    u8   nchans;
    u8   deselectChan;
};

/* Provide specs for the PCA954x types we know about */
static const struct chip_desc chips[] = {
    [snj60d0_320f_cpld1] = {
        .nchans          = NUM_OF_CPLD1_CHANS,
        .deselectChan    = CPLD_DESELECT_CHANNEL,
    },
    [snj60d0_320f_cpld2] = {
        .nchans          = NUM_OF_CPLD2_CHANS,
        .deselectChan    = CPLD_DESELECT_CHANNEL,
    },
    [snj60d0_320f_cpld3] = {
        .nchans          = NUM_OF_CPLD3_CHANS,
        .deselectChan    = CPLD_DESELECT_CHANNEL,
    },
    [snj60d0_320f_cpld4] = {
        .nchans          = NUM_OF_CPLD4_CHANS,
        .deselectChan    = CPLD_DESELECT_CHANNEL,
    }
};

static const struct i2c_device_id snj60d0_320f_cpld_mux_id[] = {
    { "snj60d0_320f_cpld1", snj60d0_320f_cpld1 },
    { "snj60d0_320f_cpld2", snj60d0_320f_cpld2 },
    { "snj60d0_320f_cpld3", snj60d0_320f_cpld3 },
    { "snj60d0_320f_cpld4", snj60d0_320f_cpld4 },
    { }
};
MODULE_DEVICE_TABLE(i2c, snj60d0_320f_cpld_mux_id);

#define TRANSCEIVER_PRESENT_ATTR_ID(index)          MODULE_PRESENT_##index
#define TRANSCEIVER_LPMODE_ATTR_ID(index)   	    MODULE_LPMODE_##index
#define TRANSCEIVER_RESET_ATTR_ID(index)   	        MODULE_RESET_##index
#define CPLD_INTR_ATTR_ID(index)   	                CPLD_INTR_##index
#define CPLD_PORT_LED_ENABLE_ATTR_ID(index)   	    CPLD_PORT_LED_ENABLE_##index

enum snj60d0_320f_cpld_sysfs_attributes {
    CPLD_VERSION,
    ACCESS,
    MODULE_PRESENT_ALL,
    /* transceiver attributes */
    TRANSCEIVER_PRESENT_ATTR_ID(1),
    TRANSCEIVER_PRESENT_ATTR_ID(2),
    TRANSCEIVER_PRESENT_ATTR_ID(3),
    TRANSCEIVER_PRESENT_ATTR_ID(4),
    TRANSCEIVER_PRESENT_ATTR_ID(5),
    TRANSCEIVER_PRESENT_ATTR_ID(6),
    TRANSCEIVER_PRESENT_ATTR_ID(7),
    TRANSCEIVER_PRESENT_ATTR_ID(8),
    TRANSCEIVER_PRESENT_ATTR_ID(9),
    TRANSCEIVER_PRESENT_ATTR_ID(10),
    TRANSCEIVER_PRESENT_ATTR_ID(11),
    TRANSCEIVER_PRESENT_ATTR_ID(12),
    TRANSCEIVER_PRESENT_ATTR_ID(13),
    TRANSCEIVER_PRESENT_ATTR_ID(14),
    TRANSCEIVER_PRESENT_ATTR_ID(15),
    TRANSCEIVER_PRESENT_ATTR_ID(16),
    TRANSCEIVER_PRESENT_ATTR_ID(17),
    TRANSCEIVER_PRESENT_ATTR_ID(18),
    TRANSCEIVER_PRESENT_ATTR_ID(19),
    TRANSCEIVER_PRESENT_ATTR_ID(20),
    TRANSCEIVER_PRESENT_ATTR_ID(21),
    TRANSCEIVER_PRESENT_ATTR_ID(22),
    TRANSCEIVER_PRESENT_ATTR_ID(23),
    TRANSCEIVER_PRESENT_ATTR_ID(24),
    TRANSCEIVER_PRESENT_ATTR_ID(25),
    TRANSCEIVER_PRESENT_ATTR_ID(26),
    TRANSCEIVER_PRESENT_ATTR_ID(27),
    TRANSCEIVER_PRESENT_ATTR_ID(28),
    TRANSCEIVER_PRESENT_ATTR_ID(29),
    TRANSCEIVER_PRESENT_ATTR_ID(30),
    TRANSCEIVER_PRESENT_ATTR_ID(31),
    TRANSCEIVER_PRESENT_ATTR_ID(32),

    TRANSCEIVER_LPMODE_ATTR_ID(1),
    TRANSCEIVER_LPMODE_ATTR_ID(2),
    TRANSCEIVER_LPMODE_ATTR_ID(3),
    TRANSCEIVER_LPMODE_ATTR_ID(4),
    TRANSCEIVER_LPMODE_ATTR_ID(5),
    TRANSCEIVER_LPMODE_ATTR_ID(6),
    TRANSCEIVER_LPMODE_ATTR_ID(7),
    TRANSCEIVER_LPMODE_ATTR_ID(8),
    TRANSCEIVER_LPMODE_ATTR_ID(9),
    TRANSCEIVER_LPMODE_ATTR_ID(10),
    TRANSCEIVER_LPMODE_ATTR_ID(11),
    TRANSCEIVER_LPMODE_ATTR_ID(12),
    TRANSCEIVER_LPMODE_ATTR_ID(13),
    TRANSCEIVER_LPMODE_ATTR_ID(14),
    TRANSCEIVER_LPMODE_ATTR_ID(15),
    TRANSCEIVER_LPMODE_ATTR_ID(16),
    TRANSCEIVER_LPMODE_ATTR_ID(17),
    TRANSCEIVER_LPMODE_ATTR_ID(18),
    TRANSCEIVER_LPMODE_ATTR_ID(19),
    TRANSCEIVER_LPMODE_ATTR_ID(20),
    TRANSCEIVER_LPMODE_ATTR_ID(21),
    TRANSCEIVER_LPMODE_ATTR_ID(22),
    TRANSCEIVER_LPMODE_ATTR_ID(23),
    TRANSCEIVER_LPMODE_ATTR_ID(24),
    TRANSCEIVER_LPMODE_ATTR_ID(25),
    TRANSCEIVER_LPMODE_ATTR_ID(26),
    TRANSCEIVER_LPMODE_ATTR_ID(27),
    TRANSCEIVER_LPMODE_ATTR_ID(28),
    TRANSCEIVER_LPMODE_ATTR_ID(29),
    TRANSCEIVER_LPMODE_ATTR_ID(30),
    TRANSCEIVER_LPMODE_ATTR_ID(31),
    TRANSCEIVER_LPMODE_ATTR_ID(32),
    TRANSCEIVER_RESET_ATTR_ID(1),
    TRANSCEIVER_RESET_ATTR_ID(2),
    TRANSCEIVER_RESET_ATTR_ID(3),
    TRANSCEIVER_RESET_ATTR_ID(4),
    TRANSCEIVER_RESET_ATTR_ID(5),
    TRANSCEIVER_RESET_ATTR_ID(6),
    TRANSCEIVER_RESET_ATTR_ID(7),
    TRANSCEIVER_RESET_ATTR_ID(8),
    TRANSCEIVER_RESET_ATTR_ID(9),
    TRANSCEIVER_RESET_ATTR_ID(10),
    TRANSCEIVER_RESET_ATTR_ID(11),
    TRANSCEIVER_RESET_ATTR_ID(12),
    TRANSCEIVER_RESET_ATTR_ID(13),
    TRANSCEIVER_RESET_ATTR_ID(14),
    TRANSCEIVER_RESET_ATTR_ID(15),
    TRANSCEIVER_RESET_ATTR_ID(16),
    TRANSCEIVER_RESET_ATTR_ID(17),
    TRANSCEIVER_RESET_ATTR_ID(18),
    TRANSCEIVER_RESET_ATTR_ID(19),
    TRANSCEIVER_RESET_ATTR_ID(20),
    TRANSCEIVER_RESET_ATTR_ID(21),
    TRANSCEIVER_RESET_ATTR_ID(22),
    TRANSCEIVER_RESET_ATTR_ID(23),
    TRANSCEIVER_RESET_ATTR_ID(24),
    TRANSCEIVER_RESET_ATTR_ID(25),
    TRANSCEIVER_RESET_ATTR_ID(26),
    TRANSCEIVER_RESET_ATTR_ID(27),
    TRANSCEIVER_RESET_ATTR_ID(28),
    TRANSCEIVER_RESET_ATTR_ID(29),
    TRANSCEIVER_RESET_ATTR_ID(30),
    TRANSCEIVER_RESET_ATTR_ID(31),
    TRANSCEIVER_RESET_ATTR_ID(32),
	CPLD_INTR_ATTR_ID(1),
	CPLD_INTR_ATTR_ID(2),
	CPLD_INTR_ATTR_ID(3),
	CPLD_INTR_ATTR_ID(4),
	CPLD_PORT_LED_ENABLE_ATTR_ID(1),
	CPLD_PORT_LED_ENABLE_ATTR_ID(2),
	CPLD_PORT_LED_ENABLE_ATTR_ID(3),
	CPLD_PORT_LED_ENABLE_ATTR_ID(4),
};
/* sysfs attributes for hwmon
 */
static ssize_t show_led_control(struct device *dev, struct device_attribute *da,
                                char *buf);
static ssize_t show_interrupt(struct device *dev, struct device_attribute *da,
                              char *buf);
static ssize_t show_status(struct device *dev, struct device_attribute *da,
                           char *buf);
static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
                                char *buf);
static ssize_t set_led_control(struct device *dev, struct device_attribute *da,
                               const char *buf, size_t count);
static ssize_t set_lp_mode(struct device *dev, struct device_attribute *da,
                           const char *buf, size_t count);
static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
                            char *buf);
static int snj60d0_320f_cpld_read_internal(struct i2c_client *client, u8 reg);
static int snj60d0_320f_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value);

/* transceiver attributes */
#define DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_status, NULL, MODULE_PRESENT_##index)
#define DECLARE_TRANSCEIVER_PRESENT_ATTR(index)  &sensor_dev_attr_module_present_##index.dev_attr.attr

#define DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_lp_mode_##index, S_IRUGO | S_IWUSR, show_status, set_lp_mode, MODULE_LPMODE_##index); \
    static SENSOR_DEVICE_ATTR(module_reset_##index,   S_IRUGO | S_IWUSR, show_status, set_mode_reset, MODULE_RESET_##index)

#define DECLARE_QSFP_TRANSCEIVER_ATTR(index)  \
	&sensor_dev_attr_module_lp_mode_##index.dev_attr.attr,	\
	&sensor_dev_attr_module_reset_##index.dev_attr.attr

/* cpld interrupt */
#define DECLARE_CPLD_INTR_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(cpld_intr_##index, S_IRUGO, show_interrupt, NULL, CPLD_INTR_##index)
#define DECLARE_CPLD_INTR_ATTR(index)  &sensor_dev_attr_cpld_intr_##index.dev_attr.attr

/* cpld port_led_control */
#define DECLARE_CPLD_PORT_LED_ENABLE_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(cpld_port_led_enable_##index, S_IRUGO | S_IWUSR, show_led_control, set_led_control, CPLD_PORT_LED_ENABLE_##index)
#define DECLARE_CPLD_PORT_LED_ENABLE_ATTR(index)  &sensor_dev_attr_cpld_port_led_enable_##index.dev_attr.attr

static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);
/* transceiver attributes */
static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, show_present_all, NULL, MODULE_PRESENT_ALL);

DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(1);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(2);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(3);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(4);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(5);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(6);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(7);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(8);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(9);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(10);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(11);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(12);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(13);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(14);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(15);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(16);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(17);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(18);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(19);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(20);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(21);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(22);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(23);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(24);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(25);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(26);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(27);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(28);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(29);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(30);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(31);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(32);

DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(1);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(2);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(3);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(4);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(5);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(6);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(7);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(8);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(9);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(10);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(11);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(12);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(13);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(14);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(15);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(16);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(17);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(18);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(19);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(20);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(21);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(22);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(23);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(24);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(25);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(26);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(27);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(28);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(29);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(30);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(31);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(32);
DECLARE_CPLD_INTR_DEVICE_ATTR(1);
DECLARE_CPLD_INTR_DEVICE_ATTR(2);
DECLARE_CPLD_INTR_DEVICE_ATTR(3);
DECLARE_CPLD_INTR_DEVICE_ATTR(4);
DECLARE_CPLD_PORT_LED_ENABLE_DEVICE_ATTR(1);
DECLARE_CPLD_PORT_LED_ENABLE_DEVICE_ATTR(2);
DECLARE_CPLD_PORT_LED_ENABLE_DEVICE_ATTR(3);
DECLARE_CPLD_PORT_LED_ENABLE_DEVICE_ATTR(4);

static struct attribute *snj60d0_320f_cpld1_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_PRESENT_ATTR(1),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(2),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(3),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(4),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(5),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(6),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(7),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(8),
    DECLARE_QSFP_TRANSCEIVER_ATTR(1),
    DECLARE_QSFP_TRANSCEIVER_ATTR(2),
    DECLARE_QSFP_TRANSCEIVER_ATTR(3),
    DECLARE_QSFP_TRANSCEIVER_ATTR(4),
    DECLARE_QSFP_TRANSCEIVER_ATTR(5),
    DECLARE_QSFP_TRANSCEIVER_ATTR(6),
    DECLARE_QSFP_TRANSCEIVER_ATTR(7),
    DECLARE_QSFP_TRANSCEIVER_ATTR(8),
    DECLARE_CPLD_INTR_ATTR(1),
    DECLARE_CPLD_PORT_LED_ENABLE_ATTR(1),
    NULL
};

static const struct attribute_group snj60d0_320f_cpld1_group = {
    .attrs = snj60d0_320f_cpld1_attributes,
};

static struct attribute *snj60d0_320f_cpld2_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_PRESENT_ATTR(9),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(10),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(11),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(12),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(13),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(14),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(15),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(16),
    DECLARE_QSFP_TRANSCEIVER_ATTR(9),
    DECLARE_QSFP_TRANSCEIVER_ATTR(10),
    DECLARE_QSFP_TRANSCEIVER_ATTR(11),
    DECLARE_QSFP_TRANSCEIVER_ATTR(12),
    DECLARE_QSFP_TRANSCEIVER_ATTR(13),
    DECLARE_QSFP_TRANSCEIVER_ATTR(14),
    DECLARE_QSFP_TRANSCEIVER_ATTR(15),
    DECLARE_QSFP_TRANSCEIVER_ATTR(16),
    DECLARE_CPLD_INTR_ATTR(2),
    DECLARE_CPLD_PORT_LED_ENABLE_ATTR(2),
    NULL
};

static const struct attribute_group snj60d0_320f_cpld2_group = {
    .attrs = snj60d0_320f_cpld2_attributes,
};

static struct attribute *snj60d0_320f_cpld3_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_PRESENT_ATTR(17),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(18),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(19),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(20),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(21),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(22),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(23),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(24),
    DECLARE_QSFP_TRANSCEIVER_ATTR(17),
    DECLARE_QSFP_TRANSCEIVER_ATTR(18),
    DECLARE_QSFP_TRANSCEIVER_ATTR(19),
    DECLARE_QSFP_TRANSCEIVER_ATTR(20),
    DECLARE_QSFP_TRANSCEIVER_ATTR(21),
    DECLARE_QSFP_TRANSCEIVER_ATTR(22),
    DECLARE_QSFP_TRANSCEIVER_ATTR(23),
    DECLARE_QSFP_TRANSCEIVER_ATTR(24),
    DECLARE_CPLD_INTR_ATTR(3),
    DECLARE_CPLD_PORT_LED_ENABLE_ATTR(3),
    NULL
};

static const struct attribute_group snj60d0_320f_cpld3_group = {
    .attrs = snj60d0_320f_cpld3_attributes,
};

static struct attribute *snj60d0_320f_cpld4_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_PRESENT_ATTR(25),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(26),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(27),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(28),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(29),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(30),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(31),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(32),
    DECLARE_QSFP_TRANSCEIVER_ATTR(25),
    DECLARE_QSFP_TRANSCEIVER_ATTR(26),
    DECLARE_QSFP_TRANSCEIVER_ATTR(27),
    DECLARE_QSFP_TRANSCEIVER_ATTR(28),
    DECLARE_QSFP_TRANSCEIVER_ATTR(29),
    DECLARE_QSFP_TRANSCEIVER_ATTR(30),
    DECLARE_QSFP_TRANSCEIVER_ATTR(31),
    DECLARE_QSFP_TRANSCEIVER_ATTR(32),
    DECLARE_CPLD_INTR_ATTR(4),
    DECLARE_CPLD_PORT_LED_ENABLE_ATTR(4),
    NULL
};

static const struct attribute_group snj60d0_320f_cpld4_group = {
    .attrs = snj60d0_320f_cpld4_attributes,
};

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
                                char *buf)
{
    int i, status, num_regs = 0;
    u8 values[4]  = {0};
    u8 regs[] = {CPLD_REG_ADDR_PRESENT};
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);

    mutex_lock(&data->update_lock);

    num_regs = 1;

    for (i = 0; i < num_regs; i++) {
        status = snj60d0_320f_cpld_read_internal(client, regs[i]);

        if (status < 0) {
            goto exit;
        }

        values[i] = (u8)status;
    }

    mutex_unlock(&data->update_lock);

    /* Return values 1 -> 8 in order */
    status = sprintf(buf, "%.2x\n", values[0]);

    return status;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_led_control(struct device *dev, struct device_attribute *da,
                                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);
    int status = 0;
    u8 reg = 0, mask = 0, revert = 1;
    
    switch (attr->index)
	{   
        case CPLD_PORT_LED_ENABLE_1:
            reg  = CPLD_REG_ADDR_PORT_LED_CONTROL;
			mask = 0x1;
            break;
        case CPLD_PORT_LED_ENABLE_2:
            reg  = CPLD_REG_ADDR_PORT_LED_CONTROL;
			mask = 0x1;
            break;
        case CPLD_PORT_LED_ENABLE_3:
            reg  = CPLD_REG_ADDR_PORT_LED_CONTROL;
			mask = 0x1;
            break;
        case CPLD_PORT_LED_ENABLE_4:
            reg  = CPLD_REG_ADDR_PORT_LED_CONTROL;
			mask = 0x1;
            break; 
        default:
            return -ENODEV;
    }

    mutex_lock(&data->update_lock);
    status = snj60d0_320f_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", revert ? !(status & mask) : !!(status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_interrupt(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);
    int status = 0;
    u8 reg = 0; 
    
    switch (attr->index)
	{   
        case CPLD_INTR_1:
            reg  = CPLD_REG_ADDR_INTR;
            break;
        case CPLD_INTR_2:
            reg  = CPLD_REG_ADDR_INTR;
            break;
        case CPLD_INTR_3:
            reg  = CPLD_REG_ADDR_INTR;
            break;
        case CPLD_INTR_4:
            reg  = CPLD_REG_ADDR_INTR;
            break; 
        default:
            return -ENODEV;
    }

    mutex_lock(&data->update_lock);
    status = snj60d0_320f_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "0x%x\n", status);

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_status(struct device *dev, struct device_attribute *da,
                           char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);
    int status = 0;
    u8 reg = 0, mask = 0;

    switch (attr->index) {
    case MODULE_PRESENT_1 ... MODULE_PRESENT_8:
        reg  = CPLD_REG_ADDR_PRESENT;
        mask = 0x1 << (attr->index - MODULE_PRESENT_1);
        break;
    case MODULE_PRESENT_9 ... MODULE_PRESENT_16:
        reg  = CPLD_REG_ADDR_PRESENT;
        mask = 0x1 << (attr->index - MODULE_PRESENT_9);
        break;
    case MODULE_PRESENT_17 ... MODULE_PRESENT_24:
        reg  = CPLD_REG_ADDR_PRESENT;
        mask = 0x1 << (attr->index - MODULE_PRESENT_17);
        break;
    case MODULE_PRESENT_25 ... MODULE_PRESENT_32:
        reg  = CPLD_REG_ADDR_PRESENT;
        mask = 0x1 << (attr->index - MODULE_PRESENT_25);
        break;
    case MODULE_LPMODE_1 ... MODULE_LPMODE_8:
        reg  = CPLD_REG_ADDR_LOWPOWERMODE;
        mask = 0x1 << (attr->index - MODULE_LPMODE_1);
        break;
    case MODULE_LPMODE_9 ... MODULE_LPMODE_16:
        reg  = CPLD_REG_ADDR_LOWPOWERMODE;
        mask = 0x1 << (attr->index - MODULE_LPMODE_9);
        break;
    case MODULE_LPMODE_17 ... MODULE_LPMODE_24:
        reg  = CPLD_REG_ADDR_LOWPOWERMODE;
        mask = 0x1 << (attr->index - MODULE_LPMODE_17);
        break;
    case MODULE_LPMODE_25 ... MODULE_LPMODE_32:
        reg  = CPLD_REG_ADDR_LOWPOWERMODE;
        mask = 0x1 << (attr->index - MODULE_LPMODE_25);
        break;
    case MODULE_RESET_1 ... MODULE_RESET_8:
        reg  = CPLD_REG_ADDR_RESET;
        mask = 0x1 << (attr->index - MODULE_RESET_1);
        break;
    case MODULE_RESET_9 ... MODULE_RESET_16:
        reg  = CPLD_REG_ADDR_RESET;
        mask = 0x1 << (attr->index - MODULE_RESET_9);
        break;
    case MODULE_RESET_17 ... MODULE_RESET_24:
        reg  = CPLD_REG_ADDR_RESET;
        mask = 0x1 << (attr->index - MODULE_RESET_17);
        break;
    case MODULE_RESET_25 ... MODULE_RESET_32:
        reg  = CPLD_REG_ADDR_RESET;
        mask = 0x1 << (attr->index - MODULE_RESET_25);
        break;
    default:
        return 0;
    }

    mutex_lock(&data->update_lock);
    status = snj60d0_320f_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", !!(status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t set_led_control(struct device *dev, struct device_attribute *da,
                               const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);

    long on;
    int status= -ENOENT;
    u8 reg = 0, mask = 0;

    if(attr->index < CPLD_PORT_LED_ENABLE_1 || attr->index > CPLD_PORT_LED_ENABLE_4)
        return status;

    status = kstrtol(buf, 10, &on);
    if (status) {
        return status;
    }

	if ((on != 1) && (on != 0))
        return -EINVAL;
		
    switch (attr->index)
	{   
        case CPLD_PORT_LED_ENABLE_1:
            reg  = CPLD_REG_ADDR_PORT_LED_CONTROL;
			mask = 0x1;
            break;
        case CPLD_PORT_LED_ENABLE_2:
            reg  = CPLD_REG_ADDR_PORT_LED_CONTROL;
			mask = 0x1;
            break;
        case CPLD_PORT_LED_ENABLE_3:
            reg  = CPLD_REG_ADDR_PORT_LED_CONTROL;
			mask = 0x1;
            break;
        case CPLD_PORT_LED_ENABLE_4:
            reg  = CPLD_REG_ADDR_PORT_LED_CONTROL;
			mask = 0x1;
            break; 
        default:
            return -ENODEV;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);
    status = snj60d0_320f_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    /* Update led_control status */
    if (on) {
        status &= ~mask;
    }
    else {
        status |= mask;
    }

    status = snj60d0_320f_cpld_write_internal(client, reg, status);
    if (unlikely(status < 0)) {
        goto exit;
    }

    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;

}

static ssize_t set_lp_mode(struct device *dev, struct device_attribute *da,
                           const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);

    long on;
    int status= -ENOENT;
    u8 reg = 0, mask = 0;

    if(attr->index < MODULE_LPMODE_1 || attr->index > MODULE_LPMODE_32)
        return status;

    status = kstrtol(buf, 10, &on);
    if (status) {
        return status;
    }

	if ((on != 1) && (on != 0))
        return -EINVAL;

    switch (attr->index) {
    case MODULE_LPMODE_1 ... MODULE_LPMODE_8:
        reg  = CPLD_REG_ADDR_LOWPOWERMODE;
        mask = 0x1 << (attr->index - MODULE_LPMODE_1);
        break;
    case MODULE_LPMODE_9 ... MODULE_LPMODE_16:
        reg  = CPLD_REG_ADDR_LOWPOWERMODE;
        mask = 0x1 << (attr->index - MODULE_LPMODE_9);
        break;
    case MODULE_LPMODE_17 ... MODULE_LPMODE_24:
        reg  = CPLD_REG_ADDR_LOWPOWERMODE;
        mask = 0x1 << (attr->index - MODULE_LPMODE_17);
        break;
    case MODULE_LPMODE_25 ... MODULE_LPMODE_32:
        reg  = CPLD_REG_ADDR_LOWPOWERMODE;
        mask = 0x1 << (attr->index - MODULE_LPMODE_25);
        break;
    default:
        return 0;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);
    status = snj60d0_320f_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    /* Update lp_mode status */
    if (on) {
        status |= mask;
    }
    else {
        status &= ~mask;
    }

    status = snj60d0_320f_cpld_write_internal(client, reg, status);
    if (unlikely(status < 0)) {
        goto exit;
    }

    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;

}

static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);

    long on;
    int status= -ENOENT;
    u8 reg = 0, mask = 0;

    if(attr->index < MODULE_RESET_1 || attr->index > MODULE_RESET_32)
        return status;

    status = kstrtol(buf, 10, &on);
    if (status) {
        return status;
    }

	if ((on != 1) && (on != 0))
        return -EINVAL;

    switch (attr->index) {
    case MODULE_RESET_1 ... MODULE_RESET_8:
        reg  = CPLD_REG_ADDR_RESET;
        mask = 0x1 << (attr->index - MODULE_RESET_1);
        break;
    case MODULE_RESET_9 ... MODULE_RESET_16:
        reg  = CPLD_REG_ADDR_RESET;
        mask = 0x1 << (attr->index - MODULE_RESET_9);
        break;
    case MODULE_RESET_17 ... MODULE_RESET_24:
        reg  = CPLD_REG_ADDR_RESET;
        mask = 0x1 << (attr->index - MODULE_RESET_17);
        break;
    case MODULE_RESET_25 ... MODULE_RESET_32:
        reg  = CPLD_REG_ADDR_RESET;
        mask = 0x1 << (attr->index - MODULE_RESET_25);
        break;
    default:
        return 0;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);
    status = snj60d0_320f_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    /* Update reset status */
    if (on) {
        status |= mask;
    }
    else {
        status &= ~mask;
    }

    status = snj60d0_320f_cpld_write_internal(client, reg, status);
    if (unlikely(status < 0)) {
        goto exit;
    }

    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;

}

static ssize_t access(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);
    int status;
    u32 addr, val;

    if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
        return -EINVAL;
    }

    if (addr > 0xFF || val > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = snj60d0_320f_cpld_write_internal(client, addr, val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

/* Write to mux register. Don't use i2c_transfer()/i2c_smbus_xfer()
   for this as they will try to lock adapter a second time */
static int snj60d0_320f_cpld_mux_reg_write(struct i2c_adapter *adap,
        struct i2c_client *client, u8 val)
{
    unsigned long orig_jiffies;
    unsigned short flags;
    union i2c_smbus_data data;
    int try;
    s32 res = -EIO;

    data.byte = val;
    flags = client->flags;
    flags &= I2C_M_TEN | I2C_CLIENT_PEC;

    if (adap->algo->smbus_xfer) {
        /* Retry automatically on arbitration loss */
        orig_jiffies = jiffies;
        for (res = 0, try = 0; try <= adap->retries; try++) {
                        res = adap->algo->smbus_xfer(adap, client->addr, flags,
                                                     I2C_SMBUS_WRITE, CPLD_CHANNEL_SELECT_REG,
                                                     I2C_SMBUS_BYTE_DATA, &data);
                        if (res != -EAGAIN)
                            break;
                        if (time_after(jiffies,
                                       orig_jiffies + adap->timeout))
                            break;
                    }
    }
    DEBUG_PRINT("snj60d0_320f_cpld_mux_reg_write: val:0x%x, res:0x%x", val, res);
    return res;
}

static int snj60d0_320f_cpld_mux_select_chan(struct i2c_mux_core *muxc,
        u32 chan)
{
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    u8 regval;
    int ret = 0;

    DEBUG_PRINT("snj60d0_320f_cpld_mux_select_chan: chan:%d", chan);
    regval = 0x1 << chan;
    /* Only select the channel if its different from the last channel */
    if (data->last_chan != regval) {
        ret = snj60d0_320f_cpld_mux_reg_write(muxc->parent, client, regval);
		data->last_chan = ret < 0 ? 0 : regval;
    }
    DEBUG_PRINT("snj60d0_320f_cpld_mux_select_chan: regval:0x%x , ret:0x%x", regval, ret);
    return ret;
}

static int snj60d0_320f_cpld_mux_deselect_mux(struct i2c_mux_core *muxc,
        u32 chan)
{
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;

    /* Deselect active channel */
    data->last_chan = chips[data->type].deselectChan;
    DEBUG_PRINT("snj60d0_320f_cpld_mux_deselect_mux: data->last_chan:%d", data->last_chan);
    return snj60d0_320f_cpld_mux_reg_write(muxc->parent, client, data->last_chan);
}

static void snj60d0_320f_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);

    if (!node) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void snj60d0_320f_cpld_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

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

static ssize_t show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0;
    struct i2c_client *client = to_i2c_client(dev);

    val = i2c_smbus_read_byte_data(client, CPLD_REG_ADDR_REVISION);

    if (val < 0) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(CPLD_REG_ADDR_REVISION) err %d\n", client->addr, val);
    }

    return sprintf(buf, "%d", val);
}

/*
 * I2C init/probing/exit functions
 */
static int snj60d0_320f_cpld_mux_probe(struct i2c_client *client,
                                     const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    int num, force, class;
    struct i2c_mux_core *muxc;
    struct snj60d0_320f_cpld_data *data;
    int ret = 0;
    const struct attribute_group *group = NULL;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
        return -ENODEV;

    muxc = i2c_mux_alloc(adap, &client->dev,
                         chips[id->driver_data].nchans, sizeof(*data), 0,
                         snj60d0_320f_cpld_mux_select_chan, snj60d0_320f_cpld_mux_deselect_mux);
    if (!muxc)
        return -ENOMEM;

    i2c_set_clientdata(client, muxc);
    data = i2c_mux_priv(muxc);
    data->client = client;
    data->type = id->driver_data;
    data->last_chan = chips[data->type].deselectChan;    /* force the first selection */
    mutex_init(&data->update_lock);

    /* Now create an adapter for each channel */
    for (num = 0; num < chips[data->type].nchans; num++) {
        force = 0;              /* dynamic adap number */
        class = 0;              /* no class by default */

        ret = i2c_mux_add_adapter(muxc, force, num, class);

        if (ret) {
            dev_err(&client->dev,
                    "failed to register multiplexed adapter"
                    " %d as bus %d\n", num, force);
            goto add_mux_failed;
        }
    }

    /* Register sysfs hooks */
    switch (data->type) {
    case snj60d0_320f_cpld1:
        group = &snj60d0_320f_cpld1_group;
        /* Bring QSFPs out of reset */
        snj60d0_320f_cpld_write_internal(client, CPLD_REG_ADDR_RESET, 0x0);
        break;
    case snj60d0_320f_cpld2:
        group = &snj60d0_320f_cpld2_group;
        /* Bring QSFPs out of reset */
        snj60d0_320f_cpld_write_internal(client, CPLD_REG_ADDR_RESET, 0x0);
        break;
    case snj60d0_320f_cpld3:
        group = &snj60d0_320f_cpld3_group;
        /* Bring QSFPs out of reset */
        snj60d0_320f_cpld_write_internal(client, CPLD_REG_ADDR_RESET, 0x0);



        break;
    case snj60d0_320f_cpld4:
        group = &snj60d0_320f_cpld4_group;
        /* Bring QSFPs out of reset */
        snj60d0_320f_cpld_write_internal(client, CPLD_REG_ADDR_RESET, 0x0);
        break;
    default:
        break;
    }

    if (group) {
        ret = sysfs_create_group(&client->dev.kobj, group);
        if (ret) {
            goto add_mux_failed;
        }
    }

    if (chips[data->type].nchans) {
        dev_info(&client->dev,
                 "registered %d multiplexed busses for I2C %s\n",
                 num, client->name);
    }
    else {
        dev_info(&client->dev,
                 "device %s registered\n", client->name);
    }

    snj60d0_320f_cpld_add_client(client);

    return 0;

add_mux_failed:
    i2c_mux_del_adapters(muxc);
    return ret;
}

static int snj60d0_320f_cpld_mux_remove(struct i2c_client *client)
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct snj60d0_320f_cpld_data *data = i2c_mux_priv(muxc);
    const struct attribute_group *group = NULL;

    snj60d0_320f_cpld_remove_client(client);

    /* Remove sysfs hooks */
    switch (data->type) {
    case snj60d0_320f_cpld1:
        group = &snj60d0_320f_cpld1_group;
        break;
    case snj60d0_320f_cpld2:
        group = &snj60d0_320f_cpld2_group;
        break;
    case snj60d0_320f_cpld3:
        group = &snj60d0_320f_cpld3_group;
        break;
    case snj60d0_320f_cpld4:
        group = &snj60d0_320f_cpld4_group;
        break;
    default:
        break;
    }

    if (group) {
        sysfs_remove_group(&client->dev.kobj, group);
    }

    i2c_mux_del_adapters(muxc);

    return 0;
}

static int snj60d0_320f_cpld_read_internal(struct i2c_client *client, u8 reg)
{
    int status = 0, retry = I2C_RW_RETRY_COUNT;

    while (retry) {
        status = i2c_smbus_read_byte_data(client, reg);
        if (unlikely(status < 0)) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }
    DEBUG_PRINT("snj60d0_320f_cpld_read_internal: reg:0x%x, status:0x%x", reg, status);
    return status;
}

static int snj60d0_320f_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
{
    int status = 0, retry = I2C_RW_RETRY_COUNT;

    while (retry) {
        status = i2c_smbus_write_byte_data(client, reg, value);
        if (unlikely(status < 0)) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }
    DEBUG_PRINT("snj60d0_320f_cpld_write_internal: reg:0x%x, val:0x%x , status:0x%x", reg, value, status);
    return status;
}

int snj60d0_320f_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = snj60d0_320f_cpld_read_internal(cpld_node->client, reg);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(snj60d0_320f_cpld_read);

int snj60d0_320f_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = snj60d0_320f_cpld_write_internal(cpld_node->client, reg, value);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(snj60d0_320f_cpld_write);

static struct i2c_driver snj60d0_320f_cpld_mux_driver = {
    .driver        = {
        .name     = DRIVER_NAME,
        .owner    = THIS_MODULE,
    },
    .probe         = snj60d0_320f_cpld_mux_probe,
    .remove        = snj60d0_320f_cpld_mux_remove,
    .id_table      = snj60d0_320f_cpld_mux_id,
};

static int __init snj60d0_320f_cpld_mux_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&snj60d0_320f_cpld_mux_driver);
}

static void __exit snj60d0_320f_cpld_mux_exit(void)
{
    i2c_del_driver(&snj60d0_320f_cpld_mux_driver);
}

MODULE_AUTHOR("Fillmore Chen <fillmore_chen@alphanetworks.com>");
MODULE_DESCRIPTION("Alphanetworks snj60d0_320f CPLD driver");
MODULE_LICENSE("GPL");

module_init(snj60d0_320f_cpld_mux_init);
module_exit(snj60d0_320f_cpld_mux_exit);

