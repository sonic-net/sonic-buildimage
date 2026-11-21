// SPDX-License-Identifier: GPL-2.0-or-later
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define LMK03328_VENDOR_ID_REG          (0x2)
#define LMK03328_VENDOR_ID              (0x0232)
#define LMK03328_INT_LIVE               (0x0D)
#define LMK03328_INTCTL                 (0x11)
#define LMK03328_INTCTL_RESET           (0x1)
#define LMK03328_INT_FLAG               (0x10)
#define LMK03328_INT_FLAG_RESET         (0x0)
#define LMK03328_LOL1_BIT               (7)
#define LMK03328_LOL2_BIT               (4)
#define LMK03328_LOS1_BIT               (6)
#define LMK03328_LOS2_BIT               (3)
#define LMK03328_CAL1_BIT               (5)
#define LMK03328_CAL2_BIT               (2)

#define LMK03328_CLOCK_STATUS_NORMAL         (0)
#define LMK03328_CLOCK_STATUS_ABNORMAL       (1)

enum chips {
    lmk03328,
};

struct wb_lmk03328_data {
    struct i2c_client *client;
    struct mutex update_lock;
    struct attribute_group *sysfs_group;
    int vendor_id;
};

static int wb_lmk03328_check_vendor_id(struct wb_lmk03328_data *data)
{
    int val;
    mutex_lock(&data->update_lock);
    val = i2c_smbus_read_word_data(data->client, LMK03328_VENDOR_ID_REG);
    mutex_unlock(&data->update_lock);
    if (val < 0) {
        DEBUG_ERROR("Failed to read vendor_id: %d\n", val);
        return val;
    }
    return ((val & 0xffff) == data->vendor_id) ? 0 : -ENODEV;
}

static ssize_t wb_lmk03328_show_vendor_id(struct device *dev, struct device_attribute *da, char *buf)
{
    struct wb_lmk03328_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int val;

    mutex_lock(&data->update_lock);
    val = i2c_smbus_read_word_data(client, LMK03328_VENDOR_ID_REG);
    mutex_unlock(&data->update_lock);
    if (val < 0) {
        DEBUG_ERROR("Failed to read vendor_id: %d\n", val);
        return val;
    }
    return snprintf(buf, PAGE_SIZE, "0x%x\n", val & 0xffff);
}

static ssize_t wb_lmk03328_show_bus_status(struct device *dev, struct device_attribute *da, char *buf)
{
    struct wb_lmk03328_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int val;
    int status;

    mutex_lock(&data->update_lock);
    val = i2c_smbus_read_word_data(client, LMK03328_VENDOR_ID_REG);
    mutex_unlock(&data->update_lock);
    if (val < 0) {
        DEBUG_ERROR("show bus_status failed to read vendor_id: %d\n", val);
        return val;
    }
    status = ((val & 0xffff) == data->vendor_id) ? LMK03328_CLOCK_STATUS_NORMAL : LMK03328_CLOCK_STATUS_ABNORMAL;
    return snprintf(buf, PAGE_SIZE, "%d\n", status);
}

static ssize_t show_lmk03328_bit_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct wb_lmk03328_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int bit_pos = to_sensor_dev_attr_2(da)->index;
    int reg = to_sensor_dev_attr_2(da)->nr;
    int val;
    int bit_val = 0;

    mutex_lock(&data->update_lock);
    val = i2c_smbus_read_byte_data(client, reg);
    mutex_unlock(&data->update_lock);
    if (val < 0) {
        DEBUG_ERROR("Failed to read reg 0x%x: %d\n", reg, val);
        return val;
    }
    DEBUG_VERBOSE("Reg 0x%x: 0x%x\n", reg, val);
    bit_val = (val >> bit_pos) & 0x1;
    return snprintf(buf, PAGE_SIZE, "%d\n", bit_val);
}

static int wb_lmk03328_init(struct wb_lmk03328_data *data)
{
    int ret;
    struct i2c_client *client = data->client;

    mutex_lock(&data->update_lock);
    ret = i2c_smbus_write_word_data(client, LMK03328_INTCTL, LMK03328_INTCTL_RESET);
    ret += i2c_smbus_write_word_data(client, LMK03328_INT_FLAG, LMK03328_INT_FLAG_RESET);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t wb_lmk03328_reset_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct wb_lmk03328_data *data = dev_get_drvdata(dev);
    int ret;
    int val = 0;
    if (kstrtoint(buf, 0, &val) || val != 1) {
        DEBUG_ERROR("invalid reset value: %s\n", buf);
        return -EINVAL;
    }

    ret = wb_lmk03328_init(data);
    if (ret < 0) {
        DEBUG_ERROR("LMK03328 reset failed: %d\n", ret);
        return ret;
    }
    DEBUG_INFO("LMK03328 reset successfully\n");
    return count;
}

static DEVICE_ATTR(reset, S_IWUSR, NULL, wb_lmk03328_reset_store);
static SENSOR_DEVICE_ATTR(vendor_id, S_IRUGO, wb_lmk03328_show_vendor_id, NULL, 0);
static SENSOR_DEVICE_ATTR(bus_status, S_IRUGO, wb_lmk03328_show_bus_status, NULL, 0);
static SENSOR_DEVICE_ATTR_2(lol_1, S_IRUGO, show_lmk03328_bit_value, NULL, LMK03328_INT_LIVE, LMK03328_LOL1_BIT);
static SENSOR_DEVICE_ATTR_2(lol_2, S_IRUGO, show_lmk03328_bit_value, NULL, LMK03328_INT_LIVE, LMK03328_LOL2_BIT);
static SENSOR_DEVICE_ATTR_2(los_1, S_IRUGO, show_lmk03328_bit_value, NULL, LMK03328_INT_LIVE, LMK03328_LOS1_BIT);
static SENSOR_DEVICE_ATTR_2(los_2, S_IRUGO, show_lmk03328_bit_value, NULL, LMK03328_INT_LIVE, LMK03328_LOS2_BIT);
static SENSOR_DEVICE_ATTR_2(cal_1, S_IRUGO, show_lmk03328_bit_value, NULL, LMK03328_INT_LIVE, LMK03328_CAL1_BIT);
static SENSOR_DEVICE_ATTR_2(cal_2, S_IRUGO, show_lmk03328_bit_value, NULL, LMK03328_INT_LIVE, LMK03328_CAL2_BIT);

static struct attribute *wb_lmk03328_attrs[] = {
    &sensor_dev_attr_vendor_id.dev_attr.attr,
    &sensor_dev_attr_bus_status.dev_attr.attr,
    &sensor_dev_attr_lol_1.dev_attr.attr,
    &sensor_dev_attr_lol_2.dev_attr.attr,
    &sensor_dev_attr_los_1.dev_attr.attr,
    &sensor_dev_attr_los_2.dev_attr.attr,
    &sensor_dev_attr_cal_1.dev_attr.attr,
    &sensor_dev_attr_cal_2.dev_attr.attr,
    &dev_attr_reset.attr,
    NULL
};

static struct attribute_group wb_lmk03328_attr_group = {
    .attrs = wb_lmk03328_attrs,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
static int wb_lmk03328_probe(struct i2c_client *client)
#else
static int wb_lmk03328_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
    struct wb_lmk03328_data *data;
    int ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    const struct i2c_device_id *id = i2c_client_get_device_id(client);
#endif
    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "Failed to allocate memory\n");
        return -ENOMEM;
    }

    data->client = client;
    mutex_init(&data->update_lock);
    i2c_set_clientdata(client, data);

    switch (id->driver_data) {
    case lmk03328:
        data->sysfs_group = &wb_lmk03328_attr_group;
        data->vendor_id = LMK03328_VENDOR_ID;
        break;
    default:
        dev_err(&client->dev, "Unknown chip id %ld\n", id->driver_data);
        return -ENODEV;
    }

    ret = wb_lmk03328_check_vendor_id(data);
    if (ret) {
        dev_err(&client->dev, "vendor_id mismatch or read failed: %d\n", ret);
        return ret;
    }

    ret = sysfs_create_group(&client->dev.kobj, data->sysfs_group);
    if (ret < 0) {
        dev_err(&client->dev, "wb_lmk03328 sysfs_create_group failed %d\n", ret);
        return ret;
    }

    dev_info(&client->dev, "wb_lmk03328 probe success\n");
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
static void wb_lmk03328_remove(struct i2c_client *client)
#else
static int wb_lmk03328_remove(struct i2c_client *client)
#endif
{
    struct wb_lmk03328_data *data = i2c_get_clientdata(client);
    if (data->sysfs_group) {
        dev_info(&client->dev, "wb_lmk03328 unregister sysfs group\n");
        sysfs_remove_group(&client->dev.kobj, (const struct attribute_group *)data->sysfs_group);
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    return 0;
#endif
}

static const struct i2c_device_id wb_lmk03328_id[] = {
    { "wb_lmk03328", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, wb_lmk03328_id);

static struct i2c_driver wb_lmk03328_driver = {
    .driver = {
        .name = "wb_lmk03328",
    },
    .probe = wb_lmk03328_probe,
    .remove = wb_lmk03328_remove,
    .id_table = wb_lmk03328_id,
};

module_i2c_driver(wb_lmk03328_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("Minimal wb_lmk03328 driver exporting vendor_id");
MODULE_LICENSE("GPL");


