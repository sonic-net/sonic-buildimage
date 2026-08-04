/*
 * An I2C multiplexer dirver for accton as5712 CPLD
 *
 * Copyright (C) 2015 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This module supports the accton cpld that hold the channel select
 * mechanism for other i2c slave devices, such as SFP.
 * This includes the:
 *     Accton as5712_54x CPLD1/CPLD2/CPLD3
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

#define I2C_RW_RETRY_COUNT                10
#define I2C_RW_RETRY_INTERVAL            60 /* ms */

#define NUM_OF_CPLD1_CHANS 0x0
#define NUM_OF_CPLD2_CHANS 0x18
#define NUM_OF_CPLD3_CHANS 0x1E
#define CPLD_CHANNEL_SELECT_REG 0x2
#define CPLD_DESELECT_CHANNEL   0xFF

static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

enum cpld_mux_type {
    as5712_54x_cpld2,
    as5712_54x_cpld3,
    as5712_54x_cpld1
};

struct as5712_54x_cpld_data {
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
    [as5712_54x_cpld1] = {
        .nchans        = NUM_OF_CPLD1_CHANS,
        .deselectChan  = CPLD_DESELECT_CHANNEL,
    },
    [as5712_54x_cpld2] = {
        .nchans        = NUM_OF_CPLD2_CHANS,
        .deselectChan  = CPLD_DESELECT_CHANNEL,
    },
    [as5712_54x_cpld3] = {
        .nchans        = NUM_OF_CPLD3_CHANS,
        .deselectChan  = CPLD_DESELECT_CHANNEL,
    }
};

static const struct i2c_device_id as5712_54x_cpld_mux_id[] = {
    { "as5712_54x_cpld1", as5712_54x_cpld1 },
    { "as5712_54x_cpld2", as5712_54x_cpld2 },
    { "as5712_54x_cpld3", as5712_54x_cpld3 },
    { }
};
MODULE_DEVICE_TABLE(i2c, as5712_54x_cpld_mux_id);

#define TRANSCEIVER_PRESENT_ATTR_ID(index)       MODULE_PRESENT_##index
#define TRANSCEIVER_TXDISABLE_ATTR_ID(index)       MODULE_TXDISABLE_##index
#define TRANSCEIVER_RXLOS_ATTR_ID(index)           MODULE_RXLOS_##index
#define TRANSCEIVER_TXFAULT_ATTR_ID(index)       MODULE_TXFAULT_##index
#define TRANSCEIVER_LPMODE_ATTR_ID(index)   	MODULE_LPMODE_##index
#define TRANSCEIVER_RESET_ATTR_ID(index)   	    MODULE_RESET_##index

enum as5712_54x_cpld1_sysfs_attributes {
    CPLD_VERSION,
    ACCESS,
    MODULE_PRESENT_ALL,
    MODULE_RXLOS_ALL,
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
    TRANSCEIVER_PRESENT_ATTR_ID(33),
    TRANSCEIVER_PRESENT_ATTR_ID(34),
    TRANSCEIVER_PRESENT_ATTR_ID(35),
    TRANSCEIVER_PRESENT_ATTR_ID(36),
    TRANSCEIVER_PRESENT_ATTR_ID(37),
    TRANSCEIVER_PRESENT_ATTR_ID(38),
    TRANSCEIVER_PRESENT_ATTR_ID(39),
    TRANSCEIVER_PRESENT_ATTR_ID(40),
    TRANSCEIVER_PRESENT_ATTR_ID(41),
    TRANSCEIVER_PRESENT_ATTR_ID(42),
    TRANSCEIVER_PRESENT_ATTR_ID(43),
    TRANSCEIVER_PRESENT_ATTR_ID(44),
    TRANSCEIVER_PRESENT_ATTR_ID(45),
    TRANSCEIVER_PRESENT_ATTR_ID(46),
    TRANSCEIVER_PRESENT_ATTR_ID(47),
    TRANSCEIVER_PRESENT_ATTR_ID(48),
    TRANSCEIVER_PRESENT_ATTR_ID(49),
    TRANSCEIVER_PRESENT_ATTR_ID(50),
    TRANSCEIVER_PRESENT_ATTR_ID(51),
    TRANSCEIVER_PRESENT_ATTR_ID(52),
    TRANSCEIVER_PRESENT_ATTR_ID(53),
    TRANSCEIVER_PRESENT_ATTR_ID(54),
    TRANSCEIVER_TXDISABLE_ATTR_ID(1),
    TRANSCEIVER_TXDISABLE_ATTR_ID(2),
    TRANSCEIVER_TXDISABLE_ATTR_ID(3),
    TRANSCEIVER_TXDISABLE_ATTR_ID(4),
    TRANSCEIVER_TXDISABLE_ATTR_ID(5),
    TRANSCEIVER_TXDISABLE_ATTR_ID(6),
    TRANSCEIVER_TXDISABLE_ATTR_ID(7),
    TRANSCEIVER_TXDISABLE_ATTR_ID(8),
    TRANSCEIVER_TXDISABLE_ATTR_ID(9),
    TRANSCEIVER_TXDISABLE_ATTR_ID(10),
    TRANSCEIVER_TXDISABLE_ATTR_ID(11),
    TRANSCEIVER_TXDISABLE_ATTR_ID(12),
    TRANSCEIVER_TXDISABLE_ATTR_ID(13),
    TRANSCEIVER_TXDISABLE_ATTR_ID(14),
    TRANSCEIVER_TXDISABLE_ATTR_ID(15),
    TRANSCEIVER_TXDISABLE_ATTR_ID(16),
    TRANSCEIVER_TXDISABLE_ATTR_ID(17),
    TRANSCEIVER_TXDISABLE_ATTR_ID(18),
    TRANSCEIVER_TXDISABLE_ATTR_ID(19),
    TRANSCEIVER_TXDISABLE_ATTR_ID(20),
    TRANSCEIVER_TXDISABLE_ATTR_ID(21),
    TRANSCEIVER_TXDISABLE_ATTR_ID(22),
    TRANSCEIVER_TXDISABLE_ATTR_ID(23),
    TRANSCEIVER_TXDISABLE_ATTR_ID(24),
    TRANSCEIVER_TXDISABLE_ATTR_ID(25),
    TRANSCEIVER_TXDISABLE_ATTR_ID(26),
    TRANSCEIVER_TXDISABLE_ATTR_ID(27),
    TRANSCEIVER_TXDISABLE_ATTR_ID(28),
    TRANSCEIVER_TXDISABLE_ATTR_ID(29),
    TRANSCEIVER_TXDISABLE_ATTR_ID(30),
    TRANSCEIVER_TXDISABLE_ATTR_ID(31),
    TRANSCEIVER_TXDISABLE_ATTR_ID(32),
    TRANSCEIVER_TXDISABLE_ATTR_ID(33),
    TRANSCEIVER_TXDISABLE_ATTR_ID(34),
    TRANSCEIVER_TXDISABLE_ATTR_ID(35),
    TRANSCEIVER_TXDISABLE_ATTR_ID(36),
    TRANSCEIVER_TXDISABLE_ATTR_ID(37),
    TRANSCEIVER_TXDISABLE_ATTR_ID(38),
    TRANSCEIVER_TXDISABLE_ATTR_ID(39),
    TRANSCEIVER_TXDISABLE_ATTR_ID(40),
    TRANSCEIVER_TXDISABLE_ATTR_ID(41),
    TRANSCEIVER_TXDISABLE_ATTR_ID(42),
    TRANSCEIVER_TXDISABLE_ATTR_ID(43),
    TRANSCEIVER_TXDISABLE_ATTR_ID(44),
    TRANSCEIVER_TXDISABLE_ATTR_ID(45),
    TRANSCEIVER_TXDISABLE_ATTR_ID(46),
    TRANSCEIVER_TXDISABLE_ATTR_ID(47),
    TRANSCEIVER_TXDISABLE_ATTR_ID(48),
    TRANSCEIVER_RXLOS_ATTR_ID(1),
    TRANSCEIVER_RXLOS_ATTR_ID(2),
    TRANSCEIVER_RXLOS_ATTR_ID(3),
    TRANSCEIVER_RXLOS_ATTR_ID(4),
    TRANSCEIVER_RXLOS_ATTR_ID(5),
    TRANSCEIVER_RXLOS_ATTR_ID(6),
    TRANSCEIVER_RXLOS_ATTR_ID(7),
    TRANSCEIVER_RXLOS_ATTR_ID(8),
    TRANSCEIVER_RXLOS_ATTR_ID(9),
    TRANSCEIVER_RXLOS_ATTR_ID(10),
    TRANSCEIVER_RXLOS_ATTR_ID(11),
    TRANSCEIVER_RXLOS_ATTR_ID(12),
    TRANSCEIVER_RXLOS_ATTR_ID(13),
    TRANSCEIVER_RXLOS_ATTR_ID(14),
    TRANSCEIVER_RXLOS_ATTR_ID(15),
    TRANSCEIVER_RXLOS_ATTR_ID(16),
    TRANSCEIVER_RXLOS_ATTR_ID(17),
    TRANSCEIVER_RXLOS_ATTR_ID(18),
    TRANSCEIVER_RXLOS_ATTR_ID(19),
    TRANSCEIVER_RXLOS_ATTR_ID(20),
    TRANSCEIVER_RXLOS_ATTR_ID(21),
    TRANSCEIVER_RXLOS_ATTR_ID(22),
    TRANSCEIVER_RXLOS_ATTR_ID(23),
    TRANSCEIVER_RXLOS_ATTR_ID(24),
    TRANSCEIVER_RXLOS_ATTR_ID(25),
    TRANSCEIVER_RXLOS_ATTR_ID(26),
    TRANSCEIVER_RXLOS_ATTR_ID(27),
    TRANSCEIVER_RXLOS_ATTR_ID(28),
    TRANSCEIVER_RXLOS_ATTR_ID(29),
    TRANSCEIVER_RXLOS_ATTR_ID(30),
    TRANSCEIVER_RXLOS_ATTR_ID(31),
    TRANSCEIVER_RXLOS_ATTR_ID(32),
    TRANSCEIVER_RXLOS_ATTR_ID(33),
    TRANSCEIVER_RXLOS_ATTR_ID(34),
    TRANSCEIVER_RXLOS_ATTR_ID(35),
    TRANSCEIVER_RXLOS_ATTR_ID(36),
    TRANSCEIVER_RXLOS_ATTR_ID(37),
    TRANSCEIVER_RXLOS_ATTR_ID(38),
    TRANSCEIVER_RXLOS_ATTR_ID(39),
    TRANSCEIVER_RXLOS_ATTR_ID(40),
    TRANSCEIVER_RXLOS_ATTR_ID(41),
    TRANSCEIVER_RXLOS_ATTR_ID(42),
    TRANSCEIVER_RXLOS_ATTR_ID(43),
    TRANSCEIVER_RXLOS_ATTR_ID(44),
    TRANSCEIVER_RXLOS_ATTR_ID(45),
    TRANSCEIVER_RXLOS_ATTR_ID(46),
    TRANSCEIVER_RXLOS_ATTR_ID(47),
    TRANSCEIVER_RXLOS_ATTR_ID(48),
    TRANSCEIVER_TXFAULT_ATTR_ID(1),
    TRANSCEIVER_TXFAULT_ATTR_ID(2),
    TRANSCEIVER_TXFAULT_ATTR_ID(3),
    TRANSCEIVER_TXFAULT_ATTR_ID(4),
    TRANSCEIVER_TXFAULT_ATTR_ID(5),
    TRANSCEIVER_TXFAULT_ATTR_ID(6),
    TRANSCEIVER_TXFAULT_ATTR_ID(7),
    TRANSCEIVER_TXFAULT_ATTR_ID(8),
    TRANSCEIVER_TXFAULT_ATTR_ID(9),
    TRANSCEIVER_TXFAULT_ATTR_ID(10),
    TRANSCEIVER_TXFAULT_ATTR_ID(11),
    TRANSCEIVER_TXFAULT_ATTR_ID(12),
    TRANSCEIVER_TXFAULT_ATTR_ID(13),
    TRANSCEIVER_TXFAULT_ATTR_ID(14),
    TRANSCEIVER_TXFAULT_ATTR_ID(15),
    TRANSCEIVER_TXFAULT_ATTR_ID(16),
    TRANSCEIVER_TXFAULT_ATTR_ID(17),
    TRANSCEIVER_TXFAULT_ATTR_ID(18),
    TRANSCEIVER_TXFAULT_ATTR_ID(19),
    TRANSCEIVER_TXFAULT_ATTR_ID(20),
    TRANSCEIVER_TXFAULT_ATTR_ID(21),
    TRANSCEIVER_TXFAULT_ATTR_ID(22),
    TRANSCEIVER_TXFAULT_ATTR_ID(23),
    TRANSCEIVER_TXFAULT_ATTR_ID(24),
    TRANSCEIVER_TXFAULT_ATTR_ID(25),
    TRANSCEIVER_TXFAULT_ATTR_ID(26),
    TRANSCEIVER_TXFAULT_ATTR_ID(27),
    TRANSCEIVER_TXFAULT_ATTR_ID(28),
    TRANSCEIVER_TXFAULT_ATTR_ID(29),
    TRANSCEIVER_TXFAULT_ATTR_ID(30),
    TRANSCEIVER_TXFAULT_ATTR_ID(31),
    TRANSCEIVER_TXFAULT_ATTR_ID(32),
    TRANSCEIVER_TXFAULT_ATTR_ID(33),
    TRANSCEIVER_TXFAULT_ATTR_ID(34),
    TRANSCEIVER_TXFAULT_ATTR_ID(35),
    TRANSCEIVER_TXFAULT_ATTR_ID(36),
    TRANSCEIVER_TXFAULT_ATTR_ID(37),
    TRANSCEIVER_TXFAULT_ATTR_ID(38),
    TRANSCEIVER_TXFAULT_ATTR_ID(39),
    TRANSCEIVER_TXFAULT_ATTR_ID(40),
    TRANSCEIVER_TXFAULT_ATTR_ID(41),
    TRANSCEIVER_TXFAULT_ATTR_ID(42),
    TRANSCEIVER_TXFAULT_ATTR_ID(43),
    TRANSCEIVER_TXFAULT_ATTR_ID(44),
    TRANSCEIVER_TXFAULT_ATTR_ID(45),
    TRANSCEIVER_TXFAULT_ATTR_ID(46),
    TRANSCEIVER_TXFAULT_ATTR_ID(47),
    TRANSCEIVER_TXFAULT_ATTR_ID(48),
    TRANSCEIVER_LPMODE_ATTR_ID(49),
    TRANSCEIVER_LPMODE_ATTR_ID(50),
    TRANSCEIVER_LPMODE_ATTR_ID(51),
    TRANSCEIVER_LPMODE_ATTR_ID(52),
    TRANSCEIVER_LPMODE_ATTR_ID(53),
    TRANSCEIVER_LPMODE_ATTR_ID(54),
    TRANSCEIVER_RESET_ATTR_ID(49),
    TRANSCEIVER_RESET_ATTR_ID(50),
    TRANSCEIVER_RESET_ATTR_ID(51),
    TRANSCEIVER_RESET_ATTR_ID(52),
    TRANSCEIVER_RESET_ATTR_ID(53),
    TRANSCEIVER_RESET_ATTR_ID(54),
};

/* sysfs attributes for hwmon
 */
static ssize_t show_status(struct device *dev, struct device_attribute *da,
                           char *buf);
static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
                                char *buf);
static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
                              char *buf);
static ssize_t set_tx_disable(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count);
static ssize_t set_lp_mode(struct device *dev, struct device_attribute *da,
                           const char *buf, size_t count);
static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
                            char *buf);
static int as5712_54x_cpld_read_internal(struct i2c_client *client, u8 reg);
static int as5712_54x_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value);

/* transceiver attributes */
#define DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_status, NULL, MODULE_PRESENT_##index)
#define DECLARE_TRANSCEIVER_PRESENT_ATTR(index)  &sensor_dev_attr_module_present_##index.dev_attr.attr

#define DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_tx_disable_##index, S_IRUGO | S_IWUSR, show_status, set_tx_disable, MODULE_TXDISABLE_##index); \
    static SENSOR_DEVICE_ATTR(module_rx_los_##index, S_IRUGO, show_status, NULL, MODULE_RXLOS_##index); \
    static SENSOR_DEVICE_ATTR(module_tx_fault_##index, S_IRUGO, show_status, NULL, MODULE_TXFAULT_##index)

#define DECLARE_SFP_TRANSCEIVER_ATTR(index)  \
    &sensor_dev_attr_module_tx_disable_##index.dev_attr.attr, \
    &sensor_dev_attr_module_rx_los_##index.dev_attr.attr, \
    &sensor_dev_attr_module_tx_fault_##index.dev_attr.attr

#define DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_lp_mode_##index, S_IRUGO | S_IWUSR, show_status, set_lp_mode, MODULE_LPMODE_##index); \
    static SENSOR_DEVICE_ATTR(module_reset_##index,     S_IWUSR | S_IRUGO, show_status, set_mode_reset, MODULE_RESET_##index)

#define DECLARE_QSFP_TRANSCEIVER_ATTR(index)  \
	&sensor_dev_attr_module_lp_mode_##index.dev_attr.attr,	\
	&sensor_dev_attr_module_reset_##index.dev_attr.attr


static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);
/* transceiver attributes */
static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, show_present_all, NULL, MODULE_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(module_rx_los_all, S_IRUGO, show_rxlos_all, NULL, MODULE_RXLOS_ALL);
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
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(33);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(34);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(35);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(36);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(37);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(38);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(39);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(40);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(41);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(42);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(43);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(44);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(45);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(46);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(47);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(48);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(49);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(50);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(51);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(52);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(53);
DECLARE_TRANSCEIVER_PRESENT_SENSOR_DEVICE_ATTR(54);

DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(1);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(2);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(3);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(4);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(5);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(6);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(7);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(8);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(9);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(10);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(11);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(12);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(13);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(14);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(15);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(16);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(17);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(18);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(19);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(20);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(21);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(22);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(23);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(24);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(25);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(26);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(27);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(28);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(29);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(30);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(31);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(32);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(33);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(34);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(35);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(36);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(37);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(38);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(39);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(40);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(41);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(42);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(43);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(44);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(45);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(46);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(47);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(48);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(49);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(50);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(51);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(52);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(53);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(54);

static struct attribute *as5712_54x_cpld1_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    NULL
};

static const struct attribute_group as5712_54x_cpld1_group = {
    .attrs = as5712_54x_cpld1_attributes,
};

static struct attribute *as5712_54x_cpld2_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    &sensor_dev_attr_module_rx_los_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_PRESENT_ATTR(1),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(2),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(3),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(4),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(5),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(6),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(7),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(8),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(9),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(10),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(11),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(12),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(13),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(14),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(15),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(16),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(17),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(18),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(19),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(20),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(21),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(22),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(23),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(24),
    DECLARE_SFP_TRANSCEIVER_ATTR(1),
    DECLARE_SFP_TRANSCEIVER_ATTR(2),
    DECLARE_SFP_TRANSCEIVER_ATTR(3),
    DECLARE_SFP_TRANSCEIVER_ATTR(4),
    DECLARE_SFP_TRANSCEIVER_ATTR(5),
    DECLARE_SFP_TRANSCEIVER_ATTR(6),
    DECLARE_SFP_TRANSCEIVER_ATTR(7),
    DECLARE_SFP_TRANSCEIVER_ATTR(8),
    DECLARE_SFP_TRANSCEIVER_ATTR(9),
    DECLARE_SFP_TRANSCEIVER_ATTR(10),
    DECLARE_SFP_TRANSCEIVER_ATTR(11),
    DECLARE_SFP_TRANSCEIVER_ATTR(12),
    DECLARE_SFP_TRANSCEIVER_ATTR(13),
    DECLARE_SFP_TRANSCEIVER_ATTR(14),
    DECLARE_SFP_TRANSCEIVER_ATTR(15),
    DECLARE_SFP_TRANSCEIVER_ATTR(16),
    DECLARE_SFP_TRANSCEIVER_ATTR(17),
    DECLARE_SFP_TRANSCEIVER_ATTR(18),
    DECLARE_SFP_TRANSCEIVER_ATTR(19),
    DECLARE_SFP_TRANSCEIVER_ATTR(20),
    DECLARE_SFP_TRANSCEIVER_ATTR(21),
    DECLARE_SFP_TRANSCEIVER_ATTR(22),
    DECLARE_SFP_TRANSCEIVER_ATTR(23),
    DECLARE_SFP_TRANSCEIVER_ATTR(24),
    NULL
};

static const struct attribute_group as5712_54x_cpld2_group = {
    .attrs = as5712_54x_cpld2_attributes,
};

static struct attribute *as5712_54x_cpld3_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    &sensor_dev_attr_module_rx_los_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_PRESENT_ATTR(25),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(26),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(27),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(28),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(29),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(30),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(31),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(32),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(33),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(34),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(35),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(36),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(37),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(38),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(39),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(40),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(41),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(42),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(43),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(44),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(45),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(46),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(47),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(48),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(49),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(50),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(51),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(52),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(53),
    DECLARE_TRANSCEIVER_PRESENT_ATTR(54),
    DECLARE_SFP_TRANSCEIVER_ATTR(25),
    DECLARE_SFP_TRANSCEIVER_ATTR(26),
    DECLARE_SFP_TRANSCEIVER_ATTR(27),
    DECLARE_SFP_TRANSCEIVER_ATTR(28),
    DECLARE_SFP_TRANSCEIVER_ATTR(29),
    DECLARE_SFP_TRANSCEIVER_ATTR(30),
    DECLARE_SFP_TRANSCEIVER_ATTR(31),
    DECLARE_SFP_TRANSCEIVER_ATTR(32),
    DECLARE_SFP_TRANSCEIVER_ATTR(33),
    DECLARE_SFP_TRANSCEIVER_ATTR(34),
    DECLARE_SFP_TRANSCEIVER_ATTR(35),
    DECLARE_SFP_TRANSCEIVER_ATTR(36),
    DECLARE_SFP_TRANSCEIVER_ATTR(37),
    DECLARE_SFP_TRANSCEIVER_ATTR(38),
    DECLARE_SFP_TRANSCEIVER_ATTR(39),
    DECLARE_SFP_TRANSCEIVER_ATTR(40),
    DECLARE_SFP_TRANSCEIVER_ATTR(41),
    DECLARE_SFP_TRANSCEIVER_ATTR(42),
    DECLARE_SFP_TRANSCEIVER_ATTR(43),
    DECLARE_SFP_TRANSCEIVER_ATTR(44),
    DECLARE_SFP_TRANSCEIVER_ATTR(45),
    DECLARE_SFP_TRANSCEIVER_ATTR(46),
    DECLARE_SFP_TRANSCEIVER_ATTR(47),
    DECLARE_SFP_TRANSCEIVER_ATTR(48),
    DECLARE_QSFP_TRANSCEIVER_ATTR(49),
    DECLARE_QSFP_TRANSCEIVER_ATTR(50),
    DECLARE_QSFP_TRANSCEIVER_ATTR(51),
    DECLARE_QSFP_TRANSCEIVER_ATTR(52),
    DECLARE_QSFP_TRANSCEIVER_ATTR(53),
    DECLARE_QSFP_TRANSCEIVER_ATTR(54),
    NULL
};

static const struct attribute_group as5712_54x_cpld3_group = {
    .attrs = as5712_54x_cpld3_attributes,
};

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
                                char *buf)
{
    int i, status, num_regs = 0;
    u8 values[4]  = {0};
    u8 regs[] = {0x6, 0x7, 0x8, 0x14};
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);

    mutex_lock(&data->update_lock);

    num_regs = (data->type == as5712_54x_cpld2) ? 3 : 4;

    for (i = 0; i < num_regs; i++) {
        status = as5712_54x_cpld_read_internal(client, regs[i]);

        if (status < 0) {
            goto exit;
        }

        values[i] = ~(u8)status;
    }

    mutex_unlock(&data->update_lock);

    /* Return values 1 -> 54 in order */
    if (data->type == as5712_54x_cpld2) {
        status = sprintf(buf, "%.2x %.2x %.2x\n",
                         values[0], values[1], values[2]);
    }
    else { /* as5712_54x_cpld3 */
        values[3] &= 0x3F;
        status = sprintf(buf, "%.2x %.2x %.2x %.2x\n",
                         values[0], values[1], values[2], values[3]);
    }

    return status;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    int i, status;
    u8 values[3]  = {0};
    u8 regs[] = {0xF, 0x10, 0x11};
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);

    mutex_lock(&data->update_lock);

    for (i = 0; i < ARRAY_SIZE(regs); i++) {
        status = as5712_54x_cpld_read_internal(client, regs[i]);

        if (status < 0) {
            goto exit;
        }

        values[i] = (u8)status;
    }

    mutex_unlock(&data->update_lock);

    /* Return values 1 -> 24 in order */
    return sprintf(buf, "%.2x %.2x %.2x\n", values[0], values[1], values[2]);

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
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);
    int status = 0;
    u8 reg = 0, mask = 0, revert = 0;

    switch (attr->index) {
    case MODULE_PRESENT_1 ... MODULE_PRESENT_8:
        reg  = 0x6;
        mask = 0x1 << (attr->index - MODULE_PRESENT_1);
        break;
    case MODULE_PRESENT_9 ... MODULE_PRESENT_16:
        reg  = 0x7;
        mask = 0x1 << (attr->index - MODULE_PRESENT_9);
        break;
    case MODULE_PRESENT_17 ... MODULE_PRESENT_24:
        reg  = 0x8;
        mask = 0x1 << (attr->index - MODULE_PRESENT_17);
        break;
    case MODULE_PRESENT_25 ... MODULE_PRESENT_32:
        reg  = 0x6;
        mask = 0x1 << (attr->index - MODULE_PRESENT_25);
        break;
    case MODULE_PRESENT_33 ... MODULE_PRESENT_40:
        reg  = 0x7;
        mask = 0x1 << (attr->index - MODULE_PRESENT_33);
        break;
    case MODULE_PRESENT_41 ... MODULE_PRESENT_48:
        reg  = 0x8;
        mask = 0x1 << (attr->index - MODULE_PRESENT_41);
        break;
    case MODULE_PRESENT_49:
        reg  = 0x14;
        mask = 0x1;
        break;
    case MODULE_PRESENT_50:
        reg  = 0x14;
        mask = 0x4;
        break;
    case MODULE_PRESENT_51:
        reg  = 0x14;
        mask = 0x10;
        break;
    case MODULE_PRESENT_52:
        reg  = 0x14;
        mask = 0x2;
        break;
    case MODULE_PRESENT_53:
        reg  = 0x14;
        mask = 0x8;
        break;
    case MODULE_PRESENT_54:
        reg  = 0x14;
        mask = 0x20;
        break;
    case MODULE_TXFAULT_1 ... MODULE_TXFAULT_8:
        reg  = 0x9;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_1);
        break;
    case MODULE_TXFAULT_9 ... MODULE_TXFAULT_16:
        reg  = 0xA;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_9);
        break;
    case MODULE_TXFAULT_17 ... MODULE_TXFAULT_24:
        reg  = 0xB;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_17);
        break;
    case MODULE_TXFAULT_25 ... MODULE_TXFAULT_32:
        reg  = 0x9;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_25);
        break;
    case MODULE_TXFAULT_33 ... MODULE_TXFAULT_40:
        reg  = 0xA;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_33);
        break;
    case MODULE_TXFAULT_41 ... MODULE_TXFAULT_48:
        reg  = 0xB;
        mask = 0x1 << (attr->index - MODULE_TXFAULT_41);
        break;
    case MODULE_TXDISABLE_1 ... MODULE_TXDISABLE_8:
        reg  = 0xC;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_1);
        break;
    case MODULE_TXDISABLE_9 ... MODULE_TXDISABLE_16:
        reg  = 0xD;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_9);
        break;
    case MODULE_TXDISABLE_17 ... MODULE_TXDISABLE_24:
        reg  = 0xE;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_17);
        break;
    case MODULE_TXDISABLE_25 ... MODULE_TXDISABLE_32:
        reg  = 0xC;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_25);
        break;
    case MODULE_TXDISABLE_33 ... MODULE_TXDISABLE_40:
        reg  = 0xD;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_33);
        break;
    case MODULE_TXDISABLE_41 ... MODULE_TXDISABLE_48:
        reg  = 0xE;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_41);
        break;
    case MODULE_RXLOS_1 ... MODULE_RXLOS_8:
        reg  = 0xF;
        mask = 0x1 << (attr->index - MODULE_RXLOS_1);
        break;
    case MODULE_RXLOS_9 ... MODULE_RXLOS_16:
        reg  = 0x10;
        mask = 0x1 << (attr->index - MODULE_RXLOS_9);
        break;
    case MODULE_RXLOS_17 ... MODULE_RXLOS_24:
        reg  = 0x11;
        mask = 0x1 << (attr->index - MODULE_RXLOS_17);
        break;
    case MODULE_RXLOS_25 ... MODULE_RXLOS_32:
        reg  = 0xF;
        mask = 0x1 << (attr->index - MODULE_RXLOS_25);
        break;
    case MODULE_RXLOS_33 ... MODULE_RXLOS_40:
        reg  = 0x10;
        mask = 0x1 << (attr->index - MODULE_RXLOS_33);
        break;
    case MODULE_RXLOS_41 ... MODULE_RXLOS_48:
        reg  = 0x11;
        mask = 0x1 << (attr->index - MODULE_RXLOS_41);
        break;
    case MODULE_LPMODE_49 ... MODULE_LPMODE_54:
        reg  = 0x16;
        mask = 0x1 << (attr->index - MODULE_LPMODE_49);
        break;
    case MODULE_RESET_49 ... MODULE_RESET_54:
        reg  = 0x15;
        mask = 0x1 << (attr->index - MODULE_RESET_49);
        break;
    default:
        return 0;
    }

    if (attr->index >= MODULE_PRESENT_1 && attr->index <= MODULE_PRESENT_54) {
        revert = 1;
    }

    mutex_lock(&data->update_lock);
    status = as5712_54x_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", revert ? !(status & mask) : !!(status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t set_tx_disable(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);
    long disable;
    int status;
    u8 reg = 0, mask = 0;

    status = kstrtol(buf, 10, &disable);
    if (status) {
        return status;
    }

    switch (attr->index) {
    case MODULE_TXDISABLE_1 ... MODULE_TXDISABLE_8:
        reg  = 0xC;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_1);
        break;
    case MODULE_TXDISABLE_9 ... MODULE_TXDISABLE_16:
        reg  = 0xD;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_9);
        break;
    case MODULE_TXDISABLE_17 ... MODULE_TXDISABLE_24:
        reg  = 0xE;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_17);
        break;
    case MODULE_TXDISABLE_25 ... MODULE_TXDISABLE_32:
        reg  = 0xC;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_25);
        break;
    case MODULE_TXDISABLE_33 ... MODULE_TXDISABLE_40:
        reg  = 0xD;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_33);
        break;
    case MODULE_TXDISABLE_41 ... MODULE_TXDISABLE_48:
        reg  = 0xE;
        mask = 0x1 << (attr->index - MODULE_TXDISABLE_41);
        break;
    default:
        return 0;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);
    status = as5712_54x_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    /* Update tx_disable status */
    if (disable) {
        status |= mask;
    }
    else {
        status &= ~mask;
    }

    status = as5712_54x_cpld_write_internal(client, reg, status);
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
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);

    long on;
    int status= -ENOENT;
    u8 reg = 0x16, mask = 0;

    if(attr->index < MODULE_LPMODE_49 || attr->index > MODULE_LPMODE_54)
        return status;

    status = kstrtol(buf, 10, &on);
    if (status) {
        return status;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);
    status = as5712_54x_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    mask = 0x1 << (attr->index - MODULE_LPMODE_49);

    /* Update lp_mode status */
    if (on) {
        status |= mask;
    }
    else {
        status &= ~mask;
    }

    status = as5712_54x_cpld_write_internal(client, reg, status);
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
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);

    long on;
    int status= -ENOENT;
    u8 reg = 0x15, mask = 0;

    if(attr->index < MODULE_RESET_49 || attr->index > MODULE_RESET_54)
        return status;

    status = kstrtol(buf, 10, &on);
    if (status) {
        return status;
    }

    /* Read current status */
    mutex_lock(&data->update_lock);
    status = as5712_54x_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    mask = 0x1 << (attr->index - MODULE_RESET_49);

    /* Update tx_disable status */
    if (on) {
        status |= mask;
    }
    else {
        status &= ~mask;
    }

    status = as5712_54x_cpld_write_internal(client, reg, status);
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
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);
    int status;
    u32 addr, val;

    if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
        return -EINVAL;
    }

    if (addr > 0xFF || val > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = as5712_54x_cpld_write_internal(client, addr, val);
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
static int as5712_54x_cpld_mux_reg_write(struct i2c_adapter *adap,
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

    return res;
}

static int as5712_54x_cpld_mux_select_chan(struct i2c_mux_core *muxc,
        u32 chan)
{
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    u8 regval;
    int ret = 0;

    regval = chan;
    /* Only select the channel if its different from the last channel */
    if (data->last_chan != regval) {
        ret = as5712_54x_cpld_mux_reg_write(muxc->parent, client, regval);
        data->last_chan = regval;
    }

    return ret;
}

static int as5712_54x_cpld_mux_deselect_mux(struct i2c_mux_core *muxc,
        u32 chan)
{
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;

    /* Deselect active channel */
    data->last_chan = chips[data->type].deselectChan;

    return as5712_54x_cpld_mux_reg_write(muxc->parent, client, data->last_chan);
}

static void as5712_54x_cpld_add_client(struct i2c_client *client)
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

static void as5712_54x_cpld_remove_client(struct i2c_client *client)
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

    val = i2c_smbus_read_byte_data(client, 0x1);

    if (val < 0) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    return sprintf(buf, "%d", val);
}

/*
 * I2C init/probing/exit functions
 */
static int as5712_54x_cpld_mux_probe(struct i2c_client *client)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    const struct i2c_device_id *id = i2c_client_get_device_id(client);
    int num, force, class;
    struct i2c_mux_core *muxc;
    struct as5712_54x_cpld_data *data;
    int ret = 0;
    const struct attribute_group *group = NULL;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
        return -ENODEV;

    muxc = i2c_mux_alloc(adap, &client->dev,
                         chips[id->driver_data].nchans, sizeof(*data), 0,
                         as5712_54x_cpld_mux_select_chan, as5712_54x_cpld_mux_deselect_mux);
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

        ret = i2c_mux_add_adapter(muxc, force, num);

        if (ret) {
            dev_err(&client->dev,
                    "failed to register multiplexed adapter"
                    " %d as bus %d\n", num, force);
            goto add_mux_failed;
        }
    }

    /* Register sysfs hooks */
    switch (data->type) {
    case as5712_54x_cpld1:
        group = &as5712_54x_cpld1_group;
        break;
    case as5712_54x_cpld2:
        group = &as5712_54x_cpld2_group;
        break;
    case as5712_54x_cpld3:
        group = &as5712_54x_cpld3_group;

        /* Bring QSFPs out of reset */
        as5712_54x_cpld_write_internal(client, 0x15, 0x3F);
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

    as5712_54x_cpld_add_client(client);

    return 0;

add_mux_failed:
    i2c_mux_del_adapters(muxc);
    return ret;
}

static void as5712_54x_cpld_mux_remove(struct i2c_client *client)
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct as5712_54x_cpld_data *data = i2c_mux_priv(muxc);
    const struct attribute_group *group = NULL;

    as5712_54x_cpld_remove_client(client);

    /* Remove sysfs hooks */
    switch (data->type) {
    case as5712_54x_cpld1:
        group = &as5712_54x_cpld1_group;
        break;
    case as5712_54x_cpld2:
        group = &as5712_54x_cpld2_group;
        break;
    case as5712_54x_cpld3:
        group = &as5712_54x_cpld3_group;
        break;
    default:
        break;
    }

    if (group) {
        sysfs_remove_group(&client->dev.kobj, group);
    }

    i2c_mux_del_adapters(muxc);

}

static int as5712_54x_cpld_read_internal(struct i2c_client *client, u8 reg)
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

    return status;
}

static int as5712_54x_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
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

    return status;
}

int as5712_54x_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as5712_54x_cpld_read_internal(cpld_node->client, reg);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as5712_54x_cpld_read);

int as5712_54x_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as5712_54x_cpld_write_internal(cpld_node->client, reg, value);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as5712_54x_cpld_write);

static struct i2c_driver as5712_54x_cpld_mux_driver = {
    .driver        = {
        .name    = "as5712_54x_cpld",
        .owner    = THIS_MODULE,
    },
    .probe        = as5712_54x_cpld_mux_probe,
    .remove        = as5712_54x_cpld_mux_remove,
    .id_table    = as5712_54x_cpld_mux_id,
};

static int __init as5712_54x_cpld_mux_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&as5712_54x_cpld_mux_driver);
}

static void __exit as5712_54x_cpld_mux_exit(void)
{
    i2c_del_driver(&as5712_54x_cpld_mux_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("Accton as5712-54x CPLD driver");
MODULE_LICENSE("GPL");

module_init(as5712_54x_cpld_mux_init);
module_exit(as5712_54x_cpld_mux_exit);

