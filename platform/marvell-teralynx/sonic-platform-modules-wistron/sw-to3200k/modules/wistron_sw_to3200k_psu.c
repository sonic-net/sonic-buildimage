
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>

#define MAX_MODEL_NAME          16
#define MAX_SERIAL_NUMBER       19

static ssize_t get_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_status(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_string(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_string(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x51, 0x52, I2C_CLIENT_END };

/* Each client has this additional data */
struct sw_to3200k_psu_data {
    struct device    *hwmon_dev;
    struct mutex     lock;
    int              index;                      /* PSU index */
    int              present;                    /* PSU present */
    int              pwr_good;                   /* PSU power good */
    int              psu_fan_dir;                /* PSU fan direction 0:afo, 1:afi */
    char             model_name[MAX_MODEL_NAME]; /* Model name */
    char             serial_number[MAX_SERIAL_NUMBER];
};

enum sw_to3200k_psu_sysfs_attributes {
    PSU_PRESENT,
    PSU_MODEL_NAME,
    PSU_POWER_GOOD,
    PSU_SERIAL_NUMBER,
    PSU_FAN_DIR
};

/* sysfs attributes for hwmon */
static SENSOR_DEVICE_ATTR(psu_present,       S_IWUSR | S_IRUGO, get_status, set_status, PSU_PRESENT);
static SENSOR_DEVICE_ATTR(psu_model_name,    S_IWUSR | S_IRUGO, get_string, set_string, PSU_MODEL_NAME);
static SENSOR_DEVICE_ATTR(psu_power_good,    S_IWUSR | S_IRUGO, get_status, set_status, PSU_POWER_GOOD);
static SENSOR_DEVICE_ATTR(psu_serial_number, S_IWUSR | S_IRUGO, get_string, set_string, PSU_SERIAL_NUMBER);
static SENSOR_DEVICE_ATTR(psu_fan_dir,       S_IWUSR | S_IRUGO, get_status, set_status, PSU_FAN_DIR);

static struct attribute *sw_to3200k_psu_attributes[] = {
    &sensor_dev_attr_psu_present.dev_attr.attr,
    &sensor_dev_attr_psu_model_name.dev_attr.attr,
    &sensor_dev_attr_psu_power_good.dev_attr.attr,
    &sensor_dev_attr_psu_serial_number.dev_attr.attr,
    &sensor_dev_attr_psu_fan_dir.dev_attr.attr,
    NULL
};

static ssize_t get_status(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_psu_data      *data = i2c_get_clientdata(client);
    int                             status = 0;

    mutex_lock(&data->lock);
    if (attr->index == PSU_PRESENT)
    {
        status = data->present;
    }
    else if (attr->index == PSU_POWER_GOOD)
    {
        status = data->pwr_good;
    }
    else
    {
        status = data->psu_fan_dir;
    }
    mutex_unlock(&data->lock);
    return sprintf(buf, "%d", status);
}

static ssize_t set_status(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_psu_data      *data = i2c_get_clientdata(client);
    int                             error, status;

    error = kstrtoint(buf, 10, &status);
    if (error)
    {
        printk("psu_set_status error=%d\n", error);
        return error;
    }

    mutex_lock(&data->lock);
    if (attr->index == PSU_PRESENT)
    {
        data->present = status;
    }
    else if (attr->index == PSU_POWER_GOOD)
    {
        data->pwr_good = status;
    }
    else
    {
        data->psu_fan_dir = status;
    }
    mutex_unlock(&data->lock);
    return count;
}

static ssize_t get_string(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_psu_data      *data = i2c_get_clientdata(client);
    ssize_t                         count;

    mutex_lock(&data->lock);
    if (attr->index == PSU_MODEL_NAME)
    {
        count = sprintf(buf, "%s", data->model_name);
    }
    else
    {
        count = sprintf(buf, "%s", data->serial_number);
    }
    mutex_unlock(&data->lock);
    return count;
}

static ssize_t set_string(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_psu_data      *data = i2c_get_clientdata(client);
    char                            tmp_str[32];

    memzero_explicit(&tmp_str, sizeof(tmp_str));
    if (attr->index == PSU_MODEL_NAME)
    {
        if (sscanf(buf, "%16s", tmp_str) != 1)
        {
            return -EINVAL;
        }
    }
    else
    {
        if (sscanf(buf, "%19s", tmp_str) != 1)
        {
            return -EINVAL;
        }
    }

    mutex_lock(&data->lock);
    if (attr->index == PSU_MODEL_NAME)
    {
        strcpy(data->model_name, tmp_str);
    }
    else
    {
        strcpy(data->serial_number, tmp_str);
    }
    mutex_unlock(&data->lock);
    return count;
}

static const struct attribute_group sw_to3200k_psu_group = {
    .attrs = sw_to3200k_psu_attributes,
};

static int sw_to3200k_psu_probe(struct i2c_client *client)
{
    const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
    struct sw_to3200k_psu_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK))
    {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct sw_to3200k_psu_data), GFP_KERNEL);
    if (!data)
    {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->index = dev_id->driver_data;
    mutex_init(&data->lock);

    dev_info(&client->dev, "device found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &sw_to3200k_psu_group);
    if (status)
    {
        goto exit_free;
    }

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, "wistron_psu", NULL, NULL);
    if (IS_ERR(data->hwmon_dev))
    {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: psu '%s'\n", dev_name(data->hwmon_dev), client->name);
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &sw_to3200k_psu_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void sw_to3200k_psu_remove(struct i2c_client *client)
{
    struct sw_to3200k_psu_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &sw_to3200k_psu_group);
    kfree(data);
}

enum psu_index
{
    sw_to3200k_psu1,
    sw_to3200k_psu2
};

static const struct i2c_device_id sw_to3200k_psu_id[] = {
    { "sw_to3200k_psu1", sw_to3200k_psu1 },
    { "sw_to3200k_psu2", sw_to3200k_psu2 },
    {}
};
MODULE_DEVICE_TABLE(i2c, sw_to3200k_psu_id);

static struct i2c_driver sw_to3200k_psu_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "sw_to3200k_psu",
    },
    .probe        = sw_to3200k_psu_probe,
    .remove       = sw_to3200k_psu_remove,
    .id_table     = sw_to3200k_psu_id,
    .address_list = normal_i2c,
};

module_i2c_driver(sw_to3200k_psu_driver);

MODULE_AUTHOR("Haowei Chung <haowei_chung@wistron.com>");
MODULE_DESCRIPTION("sw_to3200k_psu driver");
MODULE_LICENSE("GPL");
