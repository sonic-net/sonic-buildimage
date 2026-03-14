/*
 * I2C multiplexer
 *
 * Copyright (C) 2014 Accton Technology Corporation.
 *
 * This module supports the accton cpld that hold the channel select
 * mechanism for other i2c slave devices, such as SFP.
 * This includes the:
 *     Accton as6712_32x CPLD1/CPLD2/CPLD3
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

#define I2C_RW_RETRY_COUNT                10
#define I2C_RW_RETRY_INTERVAL            60 /* ms */

#define NUM_OF_CPLD1_CHANS 0x0
#define NUM_OF_CPLD2_CHANS 0x10
#define NUM_OF_CPLD3_CHANS 0x10
#define CPLD_CHANNEL_SELECT_REG 0x2
#define CPLD_DESELECT_CHANNEL   0xFF

#define ACCTON_I2C_CPLD_MUX_MAX_NCHANS  NUM_OF_CPLD3_CHANS

static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

enum cpld_mux_type {
    as6712_32x_cpld2,
    as6712_32x_cpld3,
    as6712_32x_cpld1
};

struct as6712_32x_cpld_data {
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
    [as6712_32x_cpld1] = {
    .nchans        = NUM_OF_CPLD1_CHANS,
    .deselectChan  = NUM_OF_CPLD1_CHANS,
    },
    [as6712_32x_cpld2] = {
    .nchans        = NUM_OF_CPLD2_CHANS,
    .deselectChan  = NUM_OF_CPLD2_CHANS,
    },
    [as6712_32x_cpld3] = {
    .nchans        = NUM_OF_CPLD3_CHANS,
    .deselectChan  = NUM_OF_CPLD3_CHANS,
    }
};

static const struct i2c_device_id as6712_32x_cpld_mux_id[] = {
    { "as6712_32x_cpld1", as6712_32x_cpld1 },
    { "as6712_32x_cpld2", as6712_32x_cpld2 },
    { "as6712_32x_cpld3", as6712_32x_cpld3 },
    { }
};
MODULE_DEVICE_TABLE(i2c, as6712_32x_cpld_mux_id);

#define TRANSCEIVER_PRESENT_ATTR_ID(index)       MODULE_PRESENT_##index
#define TRANSCEIVER_TXDISABLE_ATTR_ID(index)       MODULE_TXDISABLE_##index
#define TRANSCEIVER_RXLOS_ATTR_ID(index)           MODULE_RXLOS_##index
#define TRANSCEIVER_TXFAULT_ATTR_ID(index)       MODULE_TXFAULT_##index
#define TRANSCEIVER_LPMODE_ATTR_ID(index)   	MODULE_LPMODE_##index
#define TRANSCEIVER_RESET_ATTR_ID(index)       MODULE_RESET_##index

enum as6712_32x_cpld_sysfs_attributes {
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
    /*Reset*/
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
};

/* sysfs attributes for hwmon 
 */
static ssize_t show_status(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t set_status(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
             char *buf);
static int as6712_32x_cpld_read_internal(struct i2c_client *client, u8 reg);
static int as6712_32x_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value);

/* transceiver attributes */
#define DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_status, NULL, MODULE_PRESENT_##index); \
    static SENSOR_DEVICE_ATTR(module_reset_##index, S_IWUSR|S_IRUGO, show_status, set_status, MODULE_RESET_##index); \
    static SENSOR_DEVICE_ATTR(module_lp_mode_##index, S_IRUGO | S_IWUSR, show_status, set_status, MODULE_LPMODE_##index)
#define DECLARE_TRANSCEIVER_ATTR(index) \
    &sensor_dev_attr_module_present_##index.dev_attr.attr, \
    &sensor_dev_attr_module_reset_##index.dev_attr.attr, \
    &sensor_dev_attr_module_lp_mode_##index.dev_attr.attr

static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);
/* transceiver attributes */
static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, show_present_all, NULL, MODULE_PRESENT_ALL);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(1);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(2);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(3);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(4);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(5);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(6);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(7);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(8);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(9);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(10);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(11);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(12);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(13);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(14);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(15);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(16);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(17);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(18);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(19);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(20);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(21);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(22);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(23);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(24);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(25);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(26);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(27);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(28);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(29);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(30);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(31);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(32);


static struct attribute *as6712_32x_cpld1_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    NULL
};

static const struct attribute_group as6712_32x_cpld1_group = {
    .attrs = as6712_32x_cpld1_attributes,
};

static struct attribute *as6712_32x_cpld2_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_ATTR(1),
    DECLARE_TRANSCEIVER_ATTR(2),
    DECLARE_TRANSCEIVER_ATTR(3),
    DECLARE_TRANSCEIVER_ATTR(4),
    DECLARE_TRANSCEIVER_ATTR(5),
    DECLARE_TRANSCEIVER_ATTR(6),
    DECLARE_TRANSCEIVER_ATTR(7),
    DECLARE_TRANSCEIVER_ATTR(8),
    DECLARE_TRANSCEIVER_ATTR(9),
    DECLARE_TRANSCEIVER_ATTR(10),
    DECLARE_TRANSCEIVER_ATTR(11),
    DECLARE_TRANSCEIVER_ATTR(12),
    DECLARE_TRANSCEIVER_ATTR(13),
    DECLARE_TRANSCEIVER_ATTR(14),
    DECLARE_TRANSCEIVER_ATTR(15),
    DECLARE_TRANSCEIVER_ATTR(16),
    NULL
};

static const struct attribute_group as6712_32x_cpld2_group = {
    .attrs = as6712_32x_cpld2_attributes,
};

static struct attribute *as6712_32x_cpld3_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_ATTR(17),
    DECLARE_TRANSCEIVER_ATTR(18),
    DECLARE_TRANSCEIVER_ATTR(19),
    DECLARE_TRANSCEIVER_ATTR(20),
    DECLARE_TRANSCEIVER_ATTR(21),
    DECLARE_TRANSCEIVER_ATTR(22),
    DECLARE_TRANSCEIVER_ATTR(23),
    DECLARE_TRANSCEIVER_ATTR(24),
    DECLARE_TRANSCEIVER_ATTR(25),
    DECLARE_TRANSCEIVER_ATTR(26),
    DECLARE_TRANSCEIVER_ATTR(27),
    DECLARE_TRANSCEIVER_ATTR(28),
    DECLARE_TRANSCEIVER_ATTR(29),
    DECLARE_TRANSCEIVER_ATTR(30),
    DECLARE_TRANSCEIVER_ATTR(31),
    DECLARE_TRANSCEIVER_ATTR(32),
    NULL
};

static const struct attribute_group as6712_32x_cpld3_group = {
    .attrs = as6712_32x_cpld3_attributes,
};

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
             char *buf)
{
    int i, status;
    u8 values[2] = {0};
    u8 regs[] = {0xA, 0xB};
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct as6712_32x_cpld_data *data = i2c_mux_priv(muxc);

    mutex_lock(&data->update_lock);

    for (i = 0; i < ARRAY_SIZE(regs); i++) {
        status = as6712_32x_cpld_read_internal(client, regs[i]);
        
        if (status < 0) {
            goto exit;
        }

        values[i] = ~(u8)status;
    }

    mutex_unlock(&data->update_lock);

    /* Return values 1 -> 32 in order */
    return sprintf(buf, "%.2x %.2x\n", values[0], values[1]);

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t set_status(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct as6712_32x_cpld_data *data = i2c_mux_priv(muxc);
    
    int status = 0, reverse = 0;
    u8  reg = 0, mask = 0;
    u32 val, para;

    if (sscanf(buf, "%d", &para) != 1) {
        return -EINVAL;
    }

    switch (attr->index) {
    case MODULE_RESET_1 ... MODULE_RESET_32:
        reg  = 0x4 + (((attr->index - MODULE_RESET_1)/8)%2);
        mask = 0x1 << ((attr->index - MODULE_RESET_1)%8);
        reverse = 1;
        break;
    case MODULE_LPMODE_1 ... MODULE_LPMODE_32:
        reg  = 0xC + (((attr->index - MODULE_LPMODE_1)/8)%2);
        mask = 0x1 << ((attr->index - MODULE_LPMODE_1)%8);
        break;
    default:
        return 0;
    }

    mutex_lock(&data->update_lock);
    status = as6712_32x_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    val = status & ~mask;
    if (!para && reverse) { /*0 means reset*/
        val |= mask;
    }

    status = as6712_32x_cpld_write_internal(client, reg, val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

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
    struct as6712_32x_cpld_data *data = i2c_mux_priv(muxc);    
    int status = 0;
    u8 reg = 0, mask = 0;

    switch (attr->index) {
    case MODULE_PRESENT_1 ... MODULE_PRESENT_8:
        reg  = 0xA;
        mask = 0x1 << (attr->index - MODULE_PRESENT_1);
        break;
    case MODULE_PRESENT_9 ... MODULE_PRESENT_16:
        reg  = 0xB;
        mask = 0x1 << (attr->index - MODULE_PRESENT_9);
        break;
    case MODULE_PRESENT_17 ... MODULE_PRESENT_24:
        reg  = 0xA;
        mask = 0x1 << (attr->index - MODULE_PRESENT_17);
        break;
    case MODULE_PRESENT_25 ... MODULE_PRESENT_32:
        reg  = 0xB;
        mask = 0x1 << (attr->index - MODULE_PRESENT_25);
        break;
    case MODULE_RESET_1 ... MODULE_RESET_32:
        reg  = 0x4 + (((attr->index - MODULE_RESET_1)/8)%2);
        mask = 0x1 << ((attr->index - MODULE_RESET_1)%8);
        break;
    case MODULE_LPMODE_1 ... MODULE_LPMODE_32:
        reg  = 0xC + (((attr->index - MODULE_LPMODE_1)/8)%2);
        mask = 0x1 << ((attr->index - MODULE_LPMODE_1)%8);
        break;

    default:
        return 0;
    }

    mutex_lock(&data->update_lock);
    status = as6712_32x_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", !(status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t access(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    int status;
    u32 addr, val;
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct as6712_32x_cpld_data *data = i2c_mux_priv(muxc);

    if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
        return -EINVAL;
    }

    if (addr > 0xFF || val > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = as6712_32x_cpld_write_internal(client, addr, val);
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
static int as6712_32x_cpld_mux_reg_write(struct i2c_adapter *adap,
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

static int as6712_32x_cpld_mux_select_chan(struct i2c_mux_core *muxc,
        u32 chan)
{
    struct as6712_32x_cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    u8 regval;
    int ret = 0;
    regval = chan;

    /* Only select the channel if its different from the last channel */
    if (data->last_chan != regval) {
        ret = as6712_32x_cpld_mux_reg_write(muxc->parent, client, regval);
        data->last_chan = regval;
    }

    return ret;
}

static int as6712_32x_cpld_mux_deselect_mux(struct i2c_mux_core *muxc,
        u32 chan)
{
    struct as6712_32x_cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;

    /* Deselect active channel */
    data->last_chan = chips[data->type].deselectChan;

    return as6712_32x_cpld_mux_reg_write(muxc->parent, client, data->last_chan);
}

static void as6712_32x_cpld_add_client(struct i2c_client *client)
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

static void as6712_32x_cpld_remove_client(struct i2c_client *client)
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
static int as6712_32x_cpld_mux_probe(struct i2c_client *client,
             const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    int force, class;
    struct i2c_mux_core *muxc;
    struct as6712_32x_cpld_data *data;
    int chan = 0;
    int ret = -ENODEV;
    const struct attribute_group *group = NULL;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
        return -ENODEV;

    muxc = i2c_mux_alloc(adap, &client->dev,
                         chips[id->driver_data].nchans,
                         sizeof(*data), 0,
                         as6712_32x_cpld_mux_select_chan,
                         as6712_32x_cpld_mux_deselect_mux);
    if (!muxc)
        return -ENOMEM;

    i2c_set_clientdata(client, muxc);
    data = i2c_mux_priv(muxc);
    data->client = client;
    mutex_init(&data->update_lock);
    
    if (data->type == as6712_32x_cpld2 || data->type == as6712_32x_cpld3) {
        data->type = id->driver_data;
        data->last_chan = chips[data->type].deselectChan; /* force the first selection */

        /* Now create an adapter for each channel */
        for (chan = 0; chan < chips[data->type].nchans; chan++) {
            force = 0;              /* dynamic adap number */
            class = 0;              /* no class by default */

            ret = i2c_mux_add_adapter(muxc, force, chan, class);

            if (ret) {
                ret = -ENODEV;
                dev_err(&client->dev, "failed to register multiplexed adapter %d\n", chan);
                goto exit_mux_register;
            }
        }

        dev_info(&client->dev, "registered %d multiplexed busses for I2C mux %s\n", 
                                chan, client->name);
    }

    /* Register sysfs hooks */
    switch (data->type) {
    case as6712_32x_cpld1:
        group = &as6712_32x_cpld1_group;
        break;
    case as6712_32x_cpld2:
        group = &as6712_32x_cpld2_group;
        break;
    case as6712_32x_cpld3:
        group = &as6712_32x_cpld3_group;
        break;
    default:
        break;
    }

    if (group) {
        ret = sysfs_create_group(&client->dev.kobj, group);
        if (ret) {
            goto exit_mux_register;
        }
    }

    if (chips[data->type].nchans) {
        dev_info(&client->dev,
             "registered %d multiplexed busses for I2C %s\n",
             chan, client->name);
    }
    else {
        dev_info(&client->dev,
             "device %s registered\n", client->name);
    }

    as6712_32x_cpld_add_client(client);

    return 0;

exit_mux_register:
    i2c_mux_del_adapters(muxc);
    kfree(data);
    return ret;
} 

static void as6712_32x_cpld_mux_remove(struct i2c_client *client)
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct as6712_32x_cpld_data *data = i2c_mux_priv(muxc);
    const struct attribute_group *group = NULL;

    as6712_32x_cpld_remove_client(client);

    /* Remove sysfs hooks */
    switch (data->type) {
    case as6712_32x_cpld1:
        group = &as6712_32x_cpld1_group;
        break;
    case as6712_32x_cpld2:
        group = &as6712_32x_cpld2_group;
        break;
    case as6712_32x_cpld3:
        group = &as6712_32x_cpld3_group;
        break;
    default:
        break;
    }

    if (group) {
        sysfs_remove_group(&client->dev.kobj, group);
    }

    i2c_mux_del_adapters(muxc);

}

static int as6712_32x_cpld_read_internal(struct i2c_client *client, u8 reg)
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

static int as6712_32x_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
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

int as6712_32x_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as6712_32x_cpld_read_internal(cpld_node->client, reg);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as6712_32x_cpld_read);

int as6712_32x_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as6712_32x_cpld_write_internal(cpld_node->client, reg, value);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as6712_32x_cpld_write);

static struct i2c_driver as6712_32x_cpld_mux_driver = {
    .driver        = {
        .name    = "as6712_32x_cpld",
        .owner    = THIS_MODULE,
    },
    .probe        = as6712_32x_cpld_mux_probe,
    .remove        = as6712_32x_cpld_mux_remove,
    .id_table    = as6712_32x_cpld_mux_id,
};

static int __init as6712_32x_cpld_mux_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&as6712_32x_cpld_mux_driver);
}

static void __exit as6712_32x_cpld_mux_exit(void)
{
    i2c_del_driver(&as6712_32x_cpld_mux_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("Accton as6712-32x CPLD driver");
MODULE_LICENSE("GPL");

module_init(as6712_32x_cpld_mux_init);
module_exit(as6712_32x_cpld_mux_exit);

