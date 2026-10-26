/*
 * Copyright (C)  Roger Ho <roger530_ho@edge-core.com>
 *
 * This module provides support for accessing the Accton CPLD and 
 * retrieving the status of QSFP-DD/SFP port.
 * This includes the:
 *     Accton as9737_32db FPGA/CPLD2/CPLD3
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
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>

#define I2C_RW_RETRY_COUNT             (10)
#define I2C_RW_RETRY_INTERVAL          (60) /* ms */

#define PORT_NUM                       (32 + 2)  /* 32 QSFPDDs + 2 SFPs */

/* FPGA */
#define FPGA_BOARD_INFO_REG            (0x00)
#define FPGA_VERSION_REG               (0x01)

/* CPLD 2 */
#define CPLD2_VERSION_REG              (0x01)
#define XCVR_P8_P1_PRESENT_REG         (0x12)
#define XCVR_P16_P9_PRESENT_REG        (0x13)
#define XCVR_P8_P1_RESET_REG           (0x14)
#define XCVR_P16_P9_RESET_REG          (0x15)
#define XCVR_P8_P1_LPMODE_REG          (0x60)
#define XCVR_P16_P9_LPMODE_REG         (0x61)
#define XCVR_P8_P1_PWR_EN_REG          (0X81)
#define XCVR_P16_P9_PWR_EN_REG         (0X82)
#define XCVR_P8_P1_PWR_GOOD_REG        (0X83)
#define XCVR_P16_P9_PWR_GOOD_REG       (0X84)

/* CPLD 3 */
#define CPLD3_VERSION_REG              (0x01)
#define XCVR_P24_P17_PRESENT_REG       (0x12)
#define XCVR_P32_P25_PRESENT_REG       (0x13)
#define XCVR_P24_P17_RESET_REG         (0x14)
#define XCVR_P32_P25_RESET_REG         (0x15)
#define SFP_PRESENT_REG                (0x20)
#define SFP_TXDIS_REG                  (0x21)
#define SFP_RATE_SEL                   (0x20)
#define SFP_RXLOSS_REG                 (0x26)
#define SFP_TXFAULT_REG                (0x27)
#define XCVR_P24_P17_LPMODE_REG        (0x60)
#define XCVR_P32_P25_LPMODE_REG        (0x61)
#define XCVR_P24_P17_PWR_EN_REG        (0X81)
#define XCVR_P32_P25_PWR_EN_REG        (0X82)
#define XCVR_P24_P17_PWR_GOOD_REG      (0X83)
#define XCVR_P32_P25_PWR_GOOD_REG      (0X84)


static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

enum cpld_type {
    as9737_32db_fpga,
    as9737_32db_cpld2,
    as9737_32db_cpld3
};

struct as9737_32db_cpld_data {
    enum cpld_type   type;
    u8               reg;
    struct mutex     update_lock;
};

static const struct i2c_device_id as9737_32db_cpld_id[] = {
    { "as9737_32db_fpga", as9737_32db_fpga},
    { "as9737_32db_cpld2", as9737_32db_cpld2 },
    { "as9737_32db_cpld3", as9737_32db_cpld3 },
    { }
};
MODULE_DEVICE_TABLE(i2c, as9737_32db_cpld_id);

#define QDD_TRANSCEIVER_ATTR_ID(index) \
    MODULE_PRESENT_##index =     (index - 1), \
    MODULE_LPMODE_##index =      (index - 1) + (PORT_NUM * 1), \
    MODULE_RESET_##index =       (index - 1) + (PORT_NUM * 2), \
    MODULE_PWR_GOOD_##index =    (index - 1) + (PORT_NUM * 3), \
    MODULE_PWR_ENABLE_##index =  (index - 1) + (PORT_NUM * 4)

#define SFP_TRANSCEIVER_ATTR_ID(index) \
    MODULE_PRESENT_##index =     (index - 1), \
    MODULE_TX_DISABLE_##index =  (index - 1) + (PORT_NUM * 5), \
    MODULE_RX_LOS_##index =      (index - 1) + (PORT_NUM * 6), \
    MODULE_TX_FAULT_##index =    (index - 1) + (PORT_NUM * 7)

/*
 * MODULE_PRESENT_1     ... MODULE_PRESENT_34    =>   0 ...  33
 * MODULE_LPMODE_1      ... MODULE_LPMODE_32     =>  34 ...  65
 * MODULE_RESET_1       ... MODULE_RESET_32      =>  68 ...  99
 * MODULE_PWR_GOOD_1    ... MODULE_PWR_GOOD_32   => 102 ... 133
 * MODULE_PWR_ENABLE_1  ... MODULE_PWR_ENABLE_32 => 136 ... 167
 * MODULE_TX_DISABLE_33 ... MODULE_TX_DISABLE_34 => 202 ... 203
 * MODULE_RX_LOS_33     ... MODULE_RX_LOS_34     => 236 ... 237
 * MODULE_TX_FAULT_33   ... MODULE_TX_FAULT_34   => 270 ... 271
 */

enum as9737_32db_cpld_sysfs_attributes {
    /* transceiver attributes */
    QDD_TRANSCEIVER_ATTR_ID(1),
    QDD_TRANSCEIVER_ATTR_ID(2),
    QDD_TRANSCEIVER_ATTR_ID(3),
    QDD_TRANSCEIVER_ATTR_ID(4),
    QDD_TRANSCEIVER_ATTR_ID(5),
    QDD_TRANSCEIVER_ATTR_ID(6),
    QDD_TRANSCEIVER_ATTR_ID(7),
    QDD_TRANSCEIVER_ATTR_ID(8),
    QDD_TRANSCEIVER_ATTR_ID(9),
    QDD_TRANSCEIVER_ATTR_ID(10),
    QDD_TRANSCEIVER_ATTR_ID(11),
    QDD_TRANSCEIVER_ATTR_ID(12),
    QDD_TRANSCEIVER_ATTR_ID(13),
    QDD_TRANSCEIVER_ATTR_ID(14),
    QDD_TRANSCEIVER_ATTR_ID(15),
    QDD_TRANSCEIVER_ATTR_ID(16),
    QDD_TRANSCEIVER_ATTR_ID(17),
    QDD_TRANSCEIVER_ATTR_ID(18),
    QDD_TRANSCEIVER_ATTR_ID(19),
    QDD_TRANSCEIVER_ATTR_ID(20),
    QDD_TRANSCEIVER_ATTR_ID(21),
    QDD_TRANSCEIVER_ATTR_ID(22),
    QDD_TRANSCEIVER_ATTR_ID(23),
    QDD_TRANSCEIVER_ATTR_ID(24),
    QDD_TRANSCEIVER_ATTR_ID(25),
    QDD_TRANSCEIVER_ATTR_ID(26),
    QDD_TRANSCEIVER_ATTR_ID(27),
    QDD_TRANSCEIVER_ATTR_ID(28),
    QDD_TRANSCEIVER_ATTR_ID(29),
    QDD_TRANSCEIVER_ATTR_ID(30),
    QDD_TRANSCEIVER_ATTR_ID(31),
    QDD_TRANSCEIVER_ATTR_ID(32),
    SFP_TRANSCEIVER_ATTR_ID(33),
    SFP_TRANSCEIVER_ATTR_ID(34),
    BOARD_INFO,
    VERSION,
    ACCESS,
};

/* sysfs attributes for hwmon */
static ssize_t show(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_status(struct device *dev, struct device_attribute *da, 
                          const char *buf, size_t count);
static ssize_t reg_read(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t reg_write(struct device *dev, struct device_attribute *da,
                         const char *buf, size_t count);
static int as9737_32db_cpld_read_internal(struct i2c_client *client, u8 reg);
static int as9737_32db_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value);

/* transceiver attributes */
#define DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_status, NULL, MODULE_PRESENT_##index); \
    static SENSOR_DEVICE_ATTR(module_reset_##index, S_IRUGO|S_IWUSR, show_status, set_status, MODULE_RESET_##index); \
    static SENSOR_DEVICE_ATTR(module_lpmode_##index, S_IRUGO|S_IWUSR, show_status, set_status, MODULE_LPMODE_##index); \
    static SENSOR_DEVICE_ATTR(module_pwr_good_##index, S_IRUGO, show_status, NULL, MODULE_PWR_GOOD_##index); \
    static SENSOR_DEVICE_ATTR(module_pwr_enable_##index, S_IRUGO|S_IWUSR, show_status, set_status, MODULE_PWR_ENABLE_##index)

#define DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_status, NULL, MODULE_PRESENT_##index); \
    static SENSOR_DEVICE_ATTR(module_tx_disable_##index, S_IRUGO | S_IWUSR, show_status, set_status, MODULE_TX_DISABLE_##index); \
    static SENSOR_DEVICE_ATTR(module_rx_los_##index, S_IRUGO, show_status, NULL, MODULE_RX_LOS_##index); \
    static SENSOR_DEVICE_ATTR(module_tx_fault_##index, S_IRUGO, show_status, NULL, MODULE_TX_FAULT_##index)

/* declare transceiver attributes callback function */
static SENSOR_DEVICE_ATTR(board_info, S_IRUGO, show, NULL, BOARD_INFO);
static SENSOR_DEVICE_ATTR(version, S_IRUGO, show, NULL, VERSION);
static SENSOR_DEVICE_ATTR(access, S_IRUGO|S_IWUSR, reg_read, reg_write, ACCESS);

DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(1);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(2);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(3);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(4);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(5);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(6);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(7);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(8);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(9);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(10);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(11);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(12);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(13);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(14);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(15);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(16);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(17);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(18);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(19);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(20);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(21);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(22);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(23);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(24);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(25);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(26);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(27);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(28);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(29);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(30);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(31);
DECLARE_QDD_TRANSCEIVER_SENSOR_DEVICE_ATTR(32);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(33);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(34);

#define DECLARE_QDD_TRANSCEIVER_ATTR(index)  \
    &sensor_dev_attr_module_present_##index.dev_attr.attr, \
    &sensor_dev_attr_module_reset_##index.dev_attr.attr, \
    &sensor_dev_attr_module_lpmode_##index.dev_attr.attr, \
    &sensor_dev_attr_module_pwr_good_##index.dev_attr.attr, \
    &sensor_dev_attr_module_pwr_enable_##index.dev_attr.attr

#define DECLARE_SFP_TRANSCEIVER_ATTR(index)  \
    &sensor_dev_attr_module_present_##index.dev_attr.attr, \
    &sensor_dev_attr_module_tx_disable_##index.dev_attr.attr, \
    &sensor_dev_attr_module_rx_los_##index.dev_attr.attr, \
    &sensor_dev_attr_module_tx_fault_##index.dev_attr.attr

static struct attribute *as9737_32db_cpld2_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    DECLARE_QDD_TRANSCEIVER_ATTR(1),
    DECLARE_QDD_TRANSCEIVER_ATTR(2),
    DECLARE_QDD_TRANSCEIVER_ATTR(3),
    DECLARE_QDD_TRANSCEIVER_ATTR(4),
    DECLARE_QDD_TRANSCEIVER_ATTR(5),
    DECLARE_QDD_TRANSCEIVER_ATTR(6),
    DECLARE_QDD_TRANSCEIVER_ATTR(7),
    DECLARE_QDD_TRANSCEIVER_ATTR(8),
    DECLARE_QDD_TRANSCEIVER_ATTR(9),
    DECLARE_QDD_TRANSCEIVER_ATTR(10),
    DECLARE_QDD_TRANSCEIVER_ATTR(11),
    DECLARE_QDD_TRANSCEIVER_ATTR(12),
    DECLARE_QDD_TRANSCEIVER_ATTR(13),
    DECLARE_QDD_TRANSCEIVER_ATTR(14),
    DECLARE_QDD_TRANSCEIVER_ATTR(15),
    DECLARE_QDD_TRANSCEIVER_ATTR(16),
    NULL
};

static const struct attribute_group as9737_32db_cpld2_group = {
    .attrs = as9737_32db_cpld2_attributes,
};

static struct attribute *as9737_32db_cpld3_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    DECLARE_QDD_TRANSCEIVER_ATTR(17),
    DECLARE_QDD_TRANSCEIVER_ATTR(18),
    DECLARE_QDD_TRANSCEIVER_ATTR(19),
    DECLARE_QDD_TRANSCEIVER_ATTR(20),
    DECLARE_QDD_TRANSCEIVER_ATTR(21),
    DECLARE_QDD_TRANSCEIVER_ATTR(22),
    DECLARE_QDD_TRANSCEIVER_ATTR(23),
    DECLARE_QDD_TRANSCEIVER_ATTR(24),
    DECLARE_QDD_TRANSCEIVER_ATTR(25),
    DECLARE_QDD_TRANSCEIVER_ATTR(26),
    DECLARE_QDD_TRANSCEIVER_ATTR(27),
    DECLARE_QDD_TRANSCEIVER_ATTR(28),
    DECLARE_QDD_TRANSCEIVER_ATTR(29),
    DECLARE_QDD_TRANSCEIVER_ATTR(30),
    DECLARE_QDD_TRANSCEIVER_ATTR(31),
    DECLARE_QDD_TRANSCEIVER_ATTR(32),
    DECLARE_SFP_TRANSCEIVER_ATTR(33),
    DECLARE_SFP_TRANSCEIVER_ATTR(34),
    NULL
};

static const struct attribute_group as9737_32db_cpld3_group = {
    .attrs = as9737_32db_cpld3_attributes,
};

static struct attribute *as9737_32db_fpga_attributes[] = {
    &sensor_dev_attr_board_info.dev_attr.attr,
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    NULL
};

static const struct attribute_group as9737_32db_fpga_group = {
    .attrs = as9737_32db_fpga_attributes,
};

struct attribute_mapping {
    u16 attr_base;
    u8 reg;
    u8 revert;
};

static const struct attribute_mapping attribute_mappings[] = {
    [MODULE_PRESENT_1 ... MODULE_PRESENT_8] = {MODULE_PRESENT_1, XCVR_P8_P1_PRESENT_REG, 1},
    [MODULE_PRESENT_9 ... MODULE_PRESENT_16] = {MODULE_PRESENT_9, XCVR_P16_P9_PRESENT_REG, 1},
    [MODULE_PRESENT_17 ... MODULE_PRESENT_24] = {MODULE_PRESENT_17, XCVR_P24_P17_PRESENT_REG, 1},
    [MODULE_PRESENT_25 ... MODULE_PRESENT_32] = {MODULE_PRESENT_25, XCVR_P32_P25_PRESENT_REG, 1},
    /* Bit 1:SFP P2, Bit 0: SFP P1 */
    [MODULE_PRESENT_33 ... MODULE_PRESENT_34] = {MODULE_PRESENT_33, SFP_PRESENT_REG, 1},

    [MODULE_LPMODE_1 ... MODULE_LPMODE_8] = {MODULE_LPMODE_1, XCVR_P8_P1_LPMODE_REG, 0},
    [MODULE_LPMODE_9 ... MODULE_LPMODE_16] = {MODULE_LPMODE_9, XCVR_P16_P9_LPMODE_REG, 0},
    [MODULE_LPMODE_17 ... MODULE_LPMODE_24] = {MODULE_LPMODE_17, XCVR_P24_P17_LPMODE_REG, 0},
    [MODULE_LPMODE_25 ... MODULE_LPMODE_32] = {MODULE_LPMODE_25, XCVR_P32_P25_LPMODE_REG, 0},

    [MODULE_RESET_1 ... MODULE_RESET_8] = {MODULE_RESET_1, XCVR_P8_P1_RESET_REG, 1},
    [MODULE_RESET_9 ... MODULE_RESET_16] = {MODULE_RESET_9, XCVR_P16_P9_RESET_REG, 1},
    [MODULE_RESET_17 ... MODULE_RESET_24] = {MODULE_RESET_17, XCVR_P24_P17_RESET_REG, 1},
    [MODULE_RESET_25 ... MODULE_RESET_32] = {MODULE_RESET_25, XCVR_P32_P25_RESET_REG, 1},

    [MODULE_PWR_GOOD_1 ... MODULE_PWR_GOOD_8] = {MODULE_PWR_GOOD_1, XCVR_P8_P1_PWR_GOOD_REG, 0},
    [MODULE_PWR_GOOD_9 ... MODULE_PWR_GOOD_16] = {MODULE_PWR_GOOD_1, XCVR_P16_P9_PWR_GOOD_REG, 0},
    [MODULE_PWR_GOOD_17 ... MODULE_PWR_GOOD_24] = {MODULE_PWR_GOOD_1, XCVR_P24_P17_PWR_GOOD_REG, 0},
    [MODULE_PWR_GOOD_25 ... MODULE_PWR_GOOD_32] = {MODULE_PWR_GOOD_1, XCVR_P32_P25_PWR_GOOD_REG, 0},

    [MODULE_PWR_ENABLE_1 ... MODULE_PWR_ENABLE_8] = {MODULE_PWR_ENABLE_1, XCVR_P8_P1_PWR_EN_REG, 0},
    [MODULE_PWR_ENABLE_9 ... MODULE_PWR_ENABLE_16] = {MODULE_PWR_ENABLE_1, XCVR_P16_P9_PWR_EN_REG, 0},
    [MODULE_PWR_ENABLE_17 ... MODULE_PWR_ENABLE_24] = {MODULE_PWR_ENABLE_1, XCVR_P24_P17_PWR_EN_REG, 0},
    [MODULE_PWR_ENABLE_25 ... MODULE_PWR_ENABLE_32] = {MODULE_PWR_ENABLE_1, XCVR_P32_P25_PWR_EN_REG, 0},

    /* Bit 1:SFP P2, Bit 0: SFP P1 */
    [MODULE_TX_DISABLE_33 ... MODULE_TX_DISABLE_34] ={MODULE_TX_DISABLE_33, SFP_TXDIS_REG, 0},
    [MODULE_RX_LOS_33 ... MODULE_RX_LOS_34] = {MODULE_RX_LOS_33, SFP_RXLOSS_REG, 0},
    [MODULE_TX_FAULT_33 ... MODULE_TX_FAULT_34] = {MODULE_TX_FAULT_33, SFP_TXFAULT_REG, 0},
};

static ssize_t show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as9737_32db_cpld_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;
    int val = 0;

    switch(attr->index)
    {
        case VERSION:
            switch (data->type) {
                case as9737_32db_fpga:
                    reg = FPGA_VERSION_REG;
                    break;
                case as9737_32db_cpld2:
                    reg = CPLD2_VERSION_REG;
                    break;
                case as9737_32db_cpld3:
                    reg = CPLD3_VERSION_REG;
                    break;
                default:
                    break;
            }
            break;
        case BOARD_INFO:
            reg = FPGA_BOARD_INFO_REG;
            break;
        default:
            break;
    }

    val = i2c_smbus_read_byte_data(client, reg);

    if (val < 0) {
        dev_dbg(&client->dev, "cpld(0x%02x) reg(0x%02x) err %d\n",
                              client->addr, reg, val);
    }

    return sprintf(buf, "%d\n", val);
}

static ssize_t show_status(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as9737_32db_cpld_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int status = 0;
    u8 reg = 0, reg_val = 0, bits_shift = 0;

    switch(attr->index)
    {
        case MODULE_PRESENT_1 ... MODULE_TX_FAULT_34:
            reg = attribute_mappings[attr->index].reg;
            mutex_lock(&data->update_lock);
            status = as9737_32db_cpld_read_internal(client, reg);
            if (unlikely(status < 0)) {
                goto exit;
            }
            mutex_unlock(&data->update_lock);
            reg_val = status;

            bits_shift = attr->index - attribute_mappings[attr->index].attr_base;
            reg_val = (reg_val >> bits_shift) & 0x01;
            if (attribute_mappings[attr->index].revert) {
                reg_val = !reg_val;
            }

            status = sprintf(buf, "%d\n", reg_val);
            break;
        default:
            break;
    }

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t set_status(struct device *dev, struct device_attribute *da,
                          const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as9737_32db_cpld_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int status = 0;
    u8 input;
    u8 reg = 0, mask = 0, should_set_bit = 0;

    status = kstrtou8(buf, 10, &input);
    if (status) {
        return status;
    }

    reg = attribute_mappings[attr->index].reg;

    mask = 0x01 << (attr->index - attribute_mappings[attr->index].attr_base);
    should_set_bit = attribute_mappings[attr->index].revert ? !input : input;

    mutex_lock(&data->update_lock);
    status = as9737_32db_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    if (should_set_bit) {
        status |= mask;
    } else {
        status &= ~mask;
    }
    status = as9737_32db_cpld_write_internal(client, reg, status);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t reg_read(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as9737_32db_cpld_data *data = i2c_get_clientdata(client);
    int reg_val, status = 0;

    mutex_lock(&data->update_lock);
    reg_val = as9737_32db_cpld_read_internal(client, data->reg);
    if (unlikely(reg_val < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    status = sprintf(buf, "0x%02x\n", reg_val);

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t reg_write(struct device *dev, struct device_attribute *da,
                         const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as9737_32db_cpld_data *data = i2c_get_clientdata(client);
    int args, status;
    char *opt, tmp[32] = {0};
    char *tmp_p;
    size_t copy_size;
    u8 input[2] = {0};

    copy_size = (count < sizeof(tmp)) ? count : sizeof(tmp) - 1;
    #ifdef __STDC_LIB_EXT1__
    memcpy_s(tmp, copy_size, buf, copy_size);
    #else
    memcpy(tmp, buf, copy_size);
    #endif
    tmp[copy_size] = '\0';

    args = 0;
    tmp_p = tmp;
    while (args < 2 && (opt = strsep(&tmp_p, " ")) != NULL) {
        if (kstrtou8(opt, 16, &input[args]) == 0) {
            args++;
        }
    }

    switch(args)
    {
        case 2:
            /* Write value to register */
            mutex_lock(&data->update_lock);
            status = as9737_32db_cpld_write_internal(client, input[0], input[1]);
            if (unlikely(status < 0)) {
                goto exit;
            }
            mutex_unlock(&data->update_lock);
            break;
        case 1:
            /* Read value from register */
            data->reg = input[0];
            break;
        default:
            return -EINVAL;
    }

    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static void as9737_32db_cpld_add_client(struct i2c_client *client)
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

static void as9737_32db_cpld_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
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

/*
 * I2C init/probing/exit functions
 */
static int as9737_32db_cpld_probe(struct i2c_client *client,
                                  const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    struct as9737_32db_cpld_data *data;
    int ret = -ENODEV;
    const struct attribute_group *group = NULL;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
        goto exit;

    data = kzalloc(sizeof(struct as9737_32db_cpld_data), GFP_KERNEL);
    if (!data) {
        ret = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    data->type = id->driver_data;

    /* Register sysfs hooks */
    switch (data->type) {
        case as9737_32db_fpga:
            data->reg = FPGA_VERSION_REG;
            group = &as9737_32db_fpga_group;
            break;
        case as9737_32db_cpld2:
            data->reg = CPLD2_VERSION_REG;
            group = &as9737_32db_cpld2_group;
            break;
        case as9737_32db_cpld3:
            data->reg = CPLD3_VERSION_REG;
            group = &as9737_32db_cpld3_group;
            break;
        default:
            break;
    }

    if (group) {
        ret = sysfs_create_group(&client->dev.kobj, group);
        if (ret) {
            goto exit_free;
        }
    }

    as9737_32db_cpld_add_client(client);
    return 0;

exit_free:
    kfree(data);
exit:
    return ret;
}

static void as9737_32db_cpld_remove(struct i2c_client *client)
{
    struct as9737_32db_cpld_data *data = i2c_get_clientdata(client);
    const struct attribute_group *group = NULL;

    as9737_32db_cpld_remove_client(client);

    /* Remove sysfs hooks */
    switch (data->type) {
        case as9737_32db_fpga:
            group = &as9737_32db_fpga_group;
            break;
        case as9737_32db_cpld2:
            group = &as9737_32db_cpld2_group;
            break;
        case as9737_32db_cpld3:
            group = &as9737_32db_cpld3_group;
            break;
        default:
            break;
    }

    if (group) {
        sysfs_remove_group(&client->dev.kobj, group);
    }

    kfree(data);
}

static int as9737_32db_cpld_read_internal(struct i2c_client *client, u8 reg)
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

static int as9737_32db_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
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

int as9737_32db_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as9737_32db_cpld_read_internal(cpld_node->client, reg);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as9737_32db_cpld_read);

int as9737_32db_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as9737_32db_cpld_write_internal(cpld_node->client, reg, value);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as9737_32db_cpld_write);

static struct i2c_driver as9737_32db_cpld_driver = {
    .driver        = {
        .name    = "as9737_32db_cpld",
        .owner    = THIS_MODULE,
    },
    .probe        = as9737_32db_cpld_probe,
    .remove        = as9737_32db_cpld_remove,
    .id_table    = as9737_32db_cpld_id,
};

static int __init as9737_32db_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&as9737_32db_cpld_driver);
}

static void __exit as9737_32db_cpld_exit(void)
{
    i2c_del_driver(&as9737_32db_cpld_driver);
}

MODULE_AUTHOR("Roger Ho <roger530_ho@edge-core.com>");
MODULE_DESCRIPTION("AS9373-32DB CPLD driver");
MODULE_LICENSE("GPL");

module_init(as9737_32db_cpld_init);
module_exit(as9737_32db_cpld_exit);
