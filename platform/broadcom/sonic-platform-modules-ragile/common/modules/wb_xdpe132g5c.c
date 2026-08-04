/*
 * xdpe132g5c_i2c_drv.c
 *
 * This module create sysfs to set AVS and create hwmon to get out power
 * through xdpe132g5c I2C address.
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-09-17                  Initial version
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define WB_I2C_RETRY_SLEEP_TIME          (10000)   /* 10ms */
#define WB_I2C_RETRY_TIME                (10)
#define WB_XDPE_I2C_PAGE_ADDR            (0xff)
#define WB_XDPE_I2C_VOUT_MODE            (0x40)
#define WB_XDPE_I2C_VOUT_COMMAND         (0x42)
#define WB_XDPE_I2C_VOUT_PAGE            (0x06)
#define WB_XDPE_VOUT_MAX_THRESHOLD       ((0xFFFF * 1000L * 1000L) / (256))
#define WB_XDPE_VOUT_MIN_THRESHOLD       (0)

static int g_wb_xdpe_debug = 0;
static int g_wb_xdpe_error = 0;

module_param(g_wb_xdpe_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_xdpe_error, int, S_IRUGO | S_IWUSR);

#define WB_XDPE_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_xdpe_debug) { \
        printk(KERN_INFO "[WB_XDPE][VER][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_XDPE_ERROR(fmt, args...) do {                                        \
    if (g_wb_xdpe_error) { \
        printk(KERN_ERR "[WB_XDPE][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct xdpe_data {
    struct i2c_client *client;
    struct device *hwmon_dev;
    struct mutex update_lock;
    long vout_max;
    long vout_min;
};

typedef struct xdpe_vout_data_s {
    u8 vout_mode;
    int vout_precision;
} xdpe_vout_data_t;

static xdpe_vout_data_t g_xdpe_vout_group[] = {
    {.vout_mode = 0x18, .vout_precision = 256},
    {.vout_mode = 0x17, .vout_precision = 512},
    {.vout_mode = 0x16, .vout_precision = 1024},
    {.vout_mode = 0x15, .vout_precision = 2048},
    {.vout_mode = 0x14, .vout_precision = 4096},
};

static s32 wb_i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command)
{
    int i;
    s32 ret;

    for (i = 0; i < WB_I2C_RETRY_TIME; i++) {
        ret = i2c_smbus_read_byte_data(client, command);
        if (ret >= 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

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

static long calc_power_linear11_data(int data)
{
    s16 exponent;
    s32 mantissa;
    long val;

    exponent = ((s16)data) >> 11;
    mantissa = ((s16)((data & 0x7ff) << 5)) >> 5;
    val = mantissa;
    val = val * 1000L * 1000L;

    if (exponent >= 0) {
        val <<= exponent;
    } else {
        val >>= -exponent;
    }
    return val;
}

static int read_xdpe_power_value(const struct i2c_client *client, u8 page, u8 reg, long *value)
{
    int ret, data;

    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, page);
    if (ret < 0) {
        WB_XDPE_ERROR("%d-%04x: set xdpe page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, page, ret);
        return ret;
    }
    data = wb_i2c_smbus_read_word_data(client, reg);
    if (data < 0) {
        WB_XDPE_ERROR("%d-%04x: read xdpe page%u reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, page, reg, data);
        return data;
    }
    *value = calc_power_linear11_data(data);
    WB_XDPE_VERBOSE("%d-%04x: page%u reg: 0x%x rd_data: 0x%x, decode linear11 value: %ld\n",
        client->adapter->nr, client->addr, page, reg, data, *value);
    return 0;
}

static ssize_t xdpe_power_value_show(struct device *dev, struct device_attribute *da,
                   char *buf)
{
    int ret, ori_page;
    u16 sensor_h, sensor_l;
    u8 page, reg;
    struct sensor_device_attribute *attr;
    struct i2c_client *client;
    struct xdpe_data *data;
    long value1, value2;

    data = dev_get_drvdata(dev);
    client = data->client;
    attr = to_sensor_dev_attr(da);
    sensor_h = ((attr->index) >> 16) & 0xffff;
    sensor_l = (attr->index) & 0xffff;

    mutex_lock(&data->update_lock);

    ori_page = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_PAGE_ADDR);
    if (ori_page < 0) {
        WB_XDPE_ERROR("%d-%04x: read xdpe origin page failed, ret: %d\n", client->adapter->nr,
            client->addr, ori_page);
        mutex_unlock(&data->update_lock);
        return ori_page;
    }
    value1 = 0;
    value2 = 0;

    if (sensor_h) {
        page = (sensor_h >> 8) & 0xff;
        reg = sensor_h & 0xff;
        ret = read_xdpe_power_value(client, page, reg, &value1);
        if (ret < 0) {
            WB_XDPE_ERROR("%d-%04x: read xdpe sensor high sensor page%u reg: 0x%x failed, ret: %d\n",
                client->adapter->nr, client->addr, page, reg, ret);
            goto error;
        }
        WB_XDPE_VERBOSE("%d-%04x: read xdpe sensor high sensor page%u reg: 0x%x success, value: %ld\n",
            client->adapter->nr, client->addr, page, reg, value1);
    }

    page = (sensor_l >> 8) & 0xff;
    reg = sensor_l & 0xff;
    ret = read_xdpe_power_value(client, page, reg, &value2);
    if (ret < 0) {
        WB_XDPE_ERROR("%d-%04x: read xdpe sensor low sensor page%u reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, page, reg, ret);
        goto error;
    }
    WB_XDPE_VERBOSE("%d-%04x: read xdpe sensor low sensor page%u reg: 0x%x success, value: %ld\n",
        client->adapter->nr, client->addr, page, reg, value2);

    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%ld\n", value1 + value2);
error:
    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return ret;
}

static int xdpe_get_vout_precision(const struct i2c_client *client, int *vout_precision)
{
    int i, vout_mode, a_size;

    vout_mode = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_VOUT_MODE);
    if (vout_mode < 0) {
        WB_XDPE_ERROR("%d-%04x: read xdpe vout mode reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_MODE, vout_mode);
        return vout_mode;
    }

    a_size = ARRAY_SIZE(g_xdpe_vout_group);
    for (i = 0; i < a_size; i++) {
        if (g_xdpe_vout_group[i].vout_mode == vout_mode) {
            *vout_precision = g_xdpe_vout_group[i].vout_precision;
            WB_XDPE_VERBOSE("%d-%04x: match, vout mode: 0x%x, precision: %d\n",
                client->adapter->nr, client->addr, vout_mode, *vout_precision);
            break;
        }
    }
    if (i == a_size) {
        WB_XDPE_ERROR("%d-%04x: invalid vout mode: 0x%x\n",client->adapter->nr, client->addr,
            vout_mode);
        return -EINVAL;
    }
    return 0;
}

static ssize_t xdpe_avs_vout_show(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret, ori_page, vout_cmd, vout_precision;
    struct i2c_client *client;
    struct xdpe_data *data;
    long vout;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    ori_page = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_PAGE_ADDR);
    if (ori_page < 0) {
        WB_XDPE_ERROR("%d-%04x: read xdpe origin page failed, ret: %d\n", client->adapter->nr,
            client->addr, ori_page);
        mutex_unlock(&data->update_lock);
        return ori_page;
    }

    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, WB_XDPE_I2C_VOUT_PAGE);
    if (ret < 0) {
        WB_XDPE_ERROR("%d-%04x: set xdpe avs vout page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, WB_XDPE_I2C_VOUT_PAGE, ret);
        goto error;
    }

    ret = xdpe_get_vout_precision(client, &vout_precision);
    if (ret < 0) {
        WB_XDPE_ERROR("%d-%04x: get xdpe avs vout precision failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        goto error;
    }

    vout_cmd = wb_i2c_smbus_read_word_data(client, WB_XDPE_I2C_VOUT_COMMAND);
    if (vout_cmd < 0) {
        ret = vout_cmd;
        WB_XDPE_ERROR("%d-%04x: read xdpe vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_COMMAND, ret);
        goto error;
    }

    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);

    vout = vout_cmd * 1000L * 1000L / vout_precision;
    WB_XDPE_VERBOSE("%d-%04x: vout: %ld, vout_cmd: 0x%x, precision: %d\n", client->adapter->nr,
        client->addr, vout, vout_cmd, vout_precision);
    return snprintf(buf, PAGE_SIZE, "%ld\n", vout);
error:
    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t xdpe_avs_vout_store(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    int ret, ori_page, vout_cmd, vout_cmd_set, vout_precision;
    struct i2c_client *client;
    struct xdpe_data *data;
    long vout, vout_max, vout_min;

    client = to_i2c_client(dev);
    ret = kstrtol(buf, 10, &vout);
    if (ret) {
        WB_XDPE_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    data = i2c_get_clientdata(client);
    vout_max = data->vout_max;
    vout_min = data->vout_min;
    if ((vout > vout_max) || (vout < vout_min)) {
        WB_XDPE_ERROR("%d-%04x: vout value: %ld, out of range [%ld, %ld] \n", client->adapter->nr,
            client->addr, vout, vout_min, vout_max);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    ori_page = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_PAGE_ADDR);
    if (ori_page < 0) {
        WB_XDPE_ERROR("%d-%04x: read xdpe origin page failed, ret: %d\n", client->adapter->nr,
            client->addr, ori_page);
        mutex_unlock(&data->update_lock);
        return ori_page;
    }

    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, WB_XDPE_I2C_VOUT_PAGE);
    if (ret < 0) {
        WB_XDPE_ERROR("%d-%04x: set xdpe avs vout page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, WB_XDPE_I2C_VOUT_PAGE, ret);
        goto error;
    }

    ret = xdpe_get_vout_precision(client, &vout_precision);
    if (ret < 0) {
        WB_XDPE_ERROR("%d-%04x: get xdpe avs vout precision failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        goto error;
    }

    vout_cmd_set = (vout * vout_precision) / (1000L * 1000L);
    if (vout_cmd_set > 0xffff) {
        WB_XDPE_ERROR("%d-%04x: invalid value, vout %ld, vout_precision: %d, vout_cmd_set: 0x%x\n",
            client->adapter->nr, client->addr, vout, vout_precision, vout_cmd_set);
        ret = -EINVAL;
        goto error;
    }
    ret = wb_i2c_smbus_write_word_data(client, WB_XDPE_I2C_VOUT_COMMAND, vout_cmd_set);
    if (ret < 0) {
        WB_XDPE_ERROR("%d-%04x: set xdpe vout cmd reg: 0x%x,  value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_COMMAND, vout_cmd_set, ret);
        goto error;
    }

    vout_cmd = wb_i2c_smbus_read_word_data(client, WB_XDPE_I2C_VOUT_COMMAND);
    if (vout_cmd < 0) {
        ret = vout_cmd;
        WB_XDPE_ERROR("%d-%04x: read xdpe vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_COMMAND, ret);
        goto error;
    }
    if (vout_cmd != vout_cmd_set) {
        ret = -EIO;
        WB_XDPE_ERROR("%d-%04x: vout cmd value check error, vout cmd read: 0x%x, vout cmd set: 0x%x\n",
            client->adapter->nr, client->addr, vout_cmd, vout_cmd_set);
        goto error;

    }

    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    WB_XDPE_VERBOSE("%d-%04x: set vout cmd success, vout %ld, vout_precision: %d, vout_cmd_set: 0x%x\n",
        client->adapter->nr, client->addr, vout, vout_precision, vout_cmd_set);
    return count;
error:
    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t xdpe_avs_vout_max_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client;
    struct xdpe_data *data;
    long vout_max;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    vout_max = data->vout_max;
    return snprintf(buf, PAGE_SIZE, "%ld\n", vout_max);
}

static ssize_t xdpe_avs_vout_max_store(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    int ret;
    struct i2c_client *client;
    struct xdpe_data *data;
    long vout_max;

    client = to_i2c_client(dev);
    ret = kstrtol(buf, 10, &vout_max);
    if (ret) {
        WB_XDPE_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }
    WB_XDPE_VERBOSE("%d-%04x: vout max threshold: %ld", client->adapter->nr, client->addr,
        vout_max);
    data = i2c_get_clientdata(client);
    data->vout_max = vout_max;
    return count;
}

static ssize_t xdpe_avs_vout_min_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client;
    struct xdpe_data *data;
    long vout_min;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    vout_min = data->vout_min;
    return snprintf(buf, PAGE_SIZE, "%ld\n", vout_min);
}

static ssize_t xdpe_avs_vout_min_store(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    int ret;
    struct i2c_client *client;
    struct xdpe_data *data;
    long vout_min;

    client = to_i2c_client(dev);
    ret = kstrtol(buf, 10, &vout_min);
    if (ret) {
        WB_XDPE_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }
    WB_XDPE_VERBOSE("%d-%04x: vout min threshold: %ld", client->adapter->nr, client->addr,
        vout_min);
    data = i2c_get_clientdata(client);
    data->vout_min = vout_min;
    return count;
}

/* xdpe hwmon */
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO ,xdpe_power_value_show, NULL, 0x072c);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO ,xdpe_power_value_show, NULL, 0x0b2c);
static SENSOR_DEVICE_ATTR(power3_input, S_IRUGO  ,xdpe_power_value_show, NULL, 0x072c0b2c);

static struct attribute *xdpe_hwmon_attrs[] = {
    &sensor_dev_attr_power1_input.dev_attr.attr,
    &sensor_dev_attr_power2_input.dev_attr.attr,
    &sensor_dev_attr_power3_input.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(xdpe_hwmon);

/* xdpe sysfs */
static SENSOR_DEVICE_ATTR(avs_vout, S_IRUGO | S_IWUSR, xdpe_avs_vout_show, xdpe_avs_vout_store, 0);
static SENSOR_DEVICE_ATTR(avs_vout_max, S_IRUGO | S_IWUSR, xdpe_avs_vout_max_show, xdpe_avs_vout_max_store, 0);
static SENSOR_DEVICE_ATTR(avs_vout_min, S_IRUGO | S_IWUSR, xdpe_avs_vout_min_show, xdpe_avs_vout_min_store, 0);

static struct attribute *xdpe132g5c_sysfs_attrs[] = {
    &sensor_dev_attr_avs_vout.dev_attr.attr,
    &sensor_dev_attr_avs_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs_vout_min.dev_attr.attr,
    NULL,
};

static const struct attribute_group xdpe132g5c_sysfs_attrs_group = {
    .attrs = xdpe132g5c_sysfs_attrs,
};

static int xdpe132g5c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct xdpe_data *data;
    int ret;

    WB_XDPE_VERBOSE("bus: %d, addr: 0x%02x do probe.\n", client->adapter->nr, client->addr);
    data = devm_kzalloc(&client->dev, sizeof(struct xdpe_data), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    ret = sysfs_create_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
    if (ret != 0) {
        dev_err(&client->dev, "Create xdpe132g5c sysfs failed, ret: %d\n", ret);
        return ret;
    }
    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, data,
                          xdpe_hwmon_groups);
    if (IS_ERR(data->hwmon_dev)) {
        ret = PTR_ERR(data->hwmon_dev);
        sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
        dev_err(&client->dev, "Failed to register xdpe hwmon device, ret: %d\n", ret);
        return ret;
    }
    data->vout_max = WB_XDPE_VOUT_MAX_THRESHOLD;
    data->vout_min = WB_XDPE_VOUT_MIN_THRESHOLD;
    dev_info(&client->dev, "xdpe132g5c probe success\n");
    return 0;
}

static int xdpe132g5c_remove(struct i2c_client *client)
{
    struct xdpe_data *data;

    WB_XDPE_VERBOSE("bus: %d, addr: 0x%02x do remove\n", client->adapter->nr, client->addr);
    data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
    return 0;
}

static const struct i2c_device_id xdpe132g5c_id[] = {
    {"wb_xdpe132g5c", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, xdpe132g5c_id);

static const struct of_device_id __maybe_unused xdpe132g5c_of_match[] = {
    {.compatible = "infineon,wb_xdpe132g5c"},
    {}
};
MODULE_DEVICE_TABLE(of, xdpe132g5c_of_match);

static struct i2c_driver wb_xdpe132g5c_driver = {
    .driver = {
        .name = "wb_xdpe132g5c",
        .of_match_table = of_match_ptr(xdpe132g5c_of_match),
    },
    .probe      = xdpe132g5c_probe,
    .remove     = xdpe132g5c_remove,
    .id_table   = xdpe132g5c_id,
};

module_i2c_driver(wb_xdpe132g5c_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("I2C driver for Infineon XDPE132 family");
