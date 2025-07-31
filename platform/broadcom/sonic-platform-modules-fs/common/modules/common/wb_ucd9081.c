/*
 * wb_ucd9081.c
 *
 * This module create sysfs to get voltage
 * through ucd9081 I2C address.
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2024-01-08                  Initial version
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <wb_bsp_kernel_debug.h>

#define WB_UCD9081_RAIL1H               (0x00)    /* Channel 1 voltage address, high 8 bits */
#define WB_UCD9081_RAIL1L               (0x01)    /* Channel 1 voltage address, low 8 bits */
#define WB_UCD9081_RAIL2H               (0x02)    /* Channel 2 voltage address, high 8 bits */
#define WB_UCD9081_RAIL2L               (0x03)    /* Channel 2 voltage address, low 8 bits */
#define WB_UCD9081_RAIL3H               (0x04)    /* Channel 3 voltage address, high 8 bits */
#define WB_UCD9081_RAIL3L               (0x05)    /* Channel 3 voltage address, low 8 bits */
#define WB_UCD9081_RAIL4H               (0x06)    /* Channel 4 voltage address, high 8 bits */
#define WB_UCD9081_RAIL4L               (0x07)    /* Channel 4 voltage address, low 8 bits */
#define WB_UCD9081_RAIL5H               (0x08)    /* Channel 5 voltage address, high 8 bits */
#define WB_UCD9081_RAIL5L               (0x09)    /* Channel 5 voltage address, low 8 bits */
#define WB_UCD9081_RAIL6H               (0x0a)    /* Channel 6 voltage address, high 8 bits */
#define WB_UCD9081_RAIL6L               (0x0b)    /* Channel 6 voltage address, low 8 bits */
#define WB_UCD9081_RAIL7H               (0x0c)    /* Channel 7 voltage address, high 8 bits */
#define WB_UCD9081_RAIL7L               (0x0d)    /* Channel 7 voltage address, low 8 bits */
#define WB_UCD9081_WADDR1               (0x30)    /* UCD9081 High address register write address, low 8 bits */
#define WB_UCD9081_WADDR2               (0x31)    /* UCD9081 High address register write address, low 8 bits */
#define WB_UCD9081_WDATA1               (0x32)    /* Write WADDR data,low 8 bits */
#define WB_UCD9081_WDATA2               (0x33)    /* Write WADDR data,low 8 bits */

#define WB_UCD9081_FLASHLOCK_REG        (0x2E)
#define WB_UCD9081_FLASHUNLOCK_VAL      (0x02)
#define WB_UCD9081_FLASHLOCK_VAL        (0x0)

#define WB_UCD9081_FLASHLOCK_REFERENCESELECT_REG_H        (0xE1)
#define WB_UCD9081_FLASHLOCK_REFERENCESELECT_REG_L        (0x86)

/* Voltage definition */
#define WB_UCD9081_SELREF_OFFSET    (13)
#define WB_UCD9081_SELREF_0         (0x0)   /* External reference selected (VCC 3.3V) */
#define WB_UCD9081_SELREF_1         (0x1)   /* Internal reference selected (VCC 2.5V) */
#define WB_UCD9081_VREF_EXTERNAL    (3300)  /* 3.3V */
#define WB_UCD9081_VREF_INTERNAL    (2500)  /* 2.5V */
#define WB_UCD9081_VOLTAGE_MASK     (0x3ff)
#define WB_UCD9081_VOLTAGE_DIVIDE   (1024)

#define WB_I2C_RETRY_TIME               (10)
#define WB_I2C_RETRY_SLEEP_TIME         (10000)   /* 10ms */

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

struct ucd9081_data {
    struct i2c_client   *client;
    struct device       *hwmon_dev;
    struct mutex        update_lock;
    u32                 vref;               /* Voltage unit */
};

static s32 wb_i2c_smbus_write_byte_data(const struct i2c_client *client, u8 command, u8 value)
{
    int i;
    s32 ret;

    for (i = 0; i < WB_I2C_RETRY_TIME; i++) {
        ret = i2c_smbus_write_byte_data(client, command, value);
        if (ret >= 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

static s32 wb_i2c_smbus_read_word_data(const struct i2c_client *client, u8 command)
{
    int i;
    s32 ret;

    for (i = 0; i < WB_I2C_RETRY_TIME; i++) {
        ret = i2c_smbus_read_word_data(client, command);
        if (ret >= 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

static s32 wb_i2c_smbus_write_word_data(const struct i2c_client *client, u8 command,
               u16 value)
{
    int i;
    s32 ret;

    for (i = 0; i < WB_I2C_RETRY_TIME; i++) {
        ret = i2c_smbus_write_word_data(client, command, value);
        if (ret >= 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

/* Get 9081 voltage units */
static int ucd9081_get_vref(struct i2c_client *client)
{
    int ret;
    uint16_t wr_val;
    uint16_t ori_addr;
    uint16_t reference_select_val;
    struct ucd9081_data *data;

    data = i2c_get_clientdata(client);
    DEBUG_VERBOSE("%d-%04x: enter ucd9081_get_vref\n", client->adapter->nr,
        client->addr);

    mutex_lock(&data->update_lock);
    /* 0.Backup original WADDR */
    ret = wb_i2c_smbus_read_word_data(client, WB_UCD9081_WADDR1);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 origin addr failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        goto error;
    }
    ori_addr = ret;
    DEBUG_VERBOSE("%d-%04x: save ucd9081 waddr success, ori_addr: 0x%x\n",
            client->adapter->nr, client->addr, ori_addr);

    /* 1.Unlock */
    ret = wb_i2c_smbus_write_byte_data(client, WB_UCD9081_FLASHLOCK_REG, WB_UCD9081_FLASHUNLOCK_VAL);
    if (ret) {
        DEBUG_ERROR("%d-%04x: ucd9081 unlock failed\n", client->adapter->nr,
            client->addr);
        goto error;
    }

    /* 2. Write Voltage configuration flash register address 0xE186 address to WADDR */
    wr_val = (WB_UCD9081_FLASHLOCK_REFERENCESELECT_REG_H << 8) | WB_UCD9081_FLASHLOCK_REFERENCESELECT_REG_L;
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, wr_val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: write ucd9081 waddr failed\n", client->adapter->nr,
            client->addr);
        goto error;
    }

    /* 3. The voltage configuration flash register value is read through WDATA*/
    reference_select_val = wb_i2c_smbus_read_word_data(client, WB_UCD9081_WDATA1);
    if (reference_select_val < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 wdata failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        ret = reference_select_val;
        goto error;
    }

    /* 4.LOCK */
    ret = wb_i2c_smbus_write_byte_data(client, WB_UCD9081_FLASHLOCK_REG, WB_UCD9081_FLASHLOCK_VAL);
    if (ret) {
        DEBUG_ERROR("%d-%04x: ucd9081 flash lock failed\n", client->adapter->nr,
            client->addr);
        goto error;
    }

    /* 5.Restore the original WADDR address */
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, ori_addr);
    if (ret) {
        DEBUG_ERROR("%d-%04x: recover ucd9081 waddr failed\n", client->adapter->nr,
            client->addr);
        goto error;
    }

    /* 5.Calculated voltage unit*/
    DEBUG_VERBOSE("%d-%04x: ucd9081 reference_select_val: 0x%x\n",
                client->adapter->nr, client->addr, reference_select_val);
    if ((reference_select_val >> WB_UCD9081_SELREF_OFFSET) == WB_UCD9081_SELREF_0) {
        data->vref = WB_UCD9081_VREF_EXTERNAL;
    } else {
        data->vref = WB_UCD9081_VREF_INTERNAL;
    }

    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: ucd9081 use vref: %d\n",
                client->adapter->nr, client->addr, data->vref);
    return 0;

error:
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t ucd9081_voltage_show(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    struct ucd9081_data *data;
    struct i2c_client *client;
    uint32_t index, channel, value;
    long voltage;

    data = dev_get_drvdata(dev);
    client = data->client;
    index = to_sensor_dev_attr_2(da)->index;
    channel = to_sensor_dev_attr_2(da)->nr;

    mutex_lock(&data->update_lock);
    ret = wb_i2c_smbus_read_word_data(client, index);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 channel%d voltage reg failed, reg: 0x%x ret: %d\n", client->adapter->nr,
            client->addr, channel, index, ret);
        goto error;
    }
    value = ret;
    /* Terminal conversion */
    value = ((value & 0xff00) >> 8) | ((value & 0xff) << 8);
    DEBUG_VERBOSE("%d-%04x: read ucd9081 channel%d voltage success, reg: 0x%x, value: 0x%x\n",
            client->adapter->nr, client->addr, channel, index, value);

    voltage = ((value & WB_UCD9081_VOLTAGE_MASK) * data->vref) / WB_UCD9081_VOLTAGE_DIVIDE;
    DEBUG_VERBOSE("%d-%04x: ucd9081 channel%d voltage: %ld\n", client->adapter->nr, client->addr, channel, voltage);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%ld\n", voltage);
error:
    mutex_unlock(&data->update_lock);
    return ret;
}

static SENSOR_DEVICE_ATTR_2(in1_input, S_IRUGO, ucd9081_voltage_show, NULL, 1, WB_UCD9081_RAIL1H);
static SENSOR_DEVICE_ATTR_2(in2_input, S_IRUGO, ucd9081_voltage_show, NULL, 2, WB_UCD9081_RAIL2H);
static SENSOR_DEVICE_ATTR_2(in3_input, S_IRUGO, ucd9081_voltage_show, NULL, 3, WB_UCD9081_RAIL3H);
static SENSOR_DEVICE_ATTR_2(in4_input, S_IRUGO, ucd9081_voltage_show, NULL, 4, WB_UCD9081_RAIL4H);
static SENSOR_DEVICE_ATTR_2(in5_input, S_IRUGO, ucd9081_voltage_show, NULL, 5, WB_UCD9081_RAIL5H);
static SENSOR_DEVICE_ATTR_2(in6_input, S_IRUGO, ucd9081_voltage_show, NULL, 6, WB_UCD9081_RAIL6H);
static SENSOR_DEVICE_ATTR_2(in7_input, S_IRUGO, ucd9081_voltage_show, NULL, 7, WB_UCD9081_RAIL7H);

static struct attribute *ucd9081_hwmon_attrs[] = {
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,
    &sensor_dev_attr_in5_input.dev_attr.attr,
    &sensor_dev_attr_in6_input.dev_attr.attr,
    &sensor_dev_attr_in7_input.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(ucd9081_hwmon);

static int ucd9081_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct ucd9081_data *data;

    DEBUG_VERBOSE("bus: %d, addr: 0x%02x do probe.\n", client->adapter->nr, client->addr);
    data = devm_kzalloc(&client->dev, sizeof(struct ucd9081_data), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    ret = ucd9081_get_vref(client);
    if (ret != 0) {
        dev_err(&client->dev, "get ucd9081 vref failed, ret: %d\n", ret);
        return ret;
    }

    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, data, ucd9081_hwmon_groups);
    if (IS_ERR(data->hwmon_dev)) {
        ret = PTR_ERR(data->hwmon_dev);
        dev_err(&client->dev, "Failed to register ucd9081 hwmon device, ret: %d\n", ret);
        return ret;
    }
    dev_info(&client->dev, "ucd9081 (addr:0x%x, nr:%d) probe success\n", client->addr, client->adapter->nr);
    return 0;
}

static void ucd9081_remove(struct i2c_client *client)
{
    struct ucd9081_data *data;

    data = i2c_get_clientdata(client);
    dev_info(&client->dev, "ucd9081 do remove\n");

    hwmon_device_unregister(data->hwmon_dev);
    return;
}

static const struct i2c_device_id ucd9081_id[] = {
    {"wb_ucd9081", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, ucd9081_id);

static const struct of_device_id ucd9081_dev_of_match[] = {
    { .compatible = "ti,wb_ucd9081" },
    { },
};
MODULE_DEVICE_TABLE(of, ucd9081_dev_of_match);

static struct i2c_driver ucd9081_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name   = "wb_ucd9081",
        .of_match_table = ucd9081_dev_of_match,
    },
    .probe      = ucd9081_probe,
    .remove     = ucd9081_remove,
    .id_table = ucd9081_id,
};

module_i2c_driver(ucd9081_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("ucd9081 Driver");
MODULE_LICENSE("GPL");
