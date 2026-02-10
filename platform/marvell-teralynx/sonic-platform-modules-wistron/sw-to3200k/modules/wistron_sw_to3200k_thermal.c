
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

#define STRING_TO_DEC_VALUE     10
#define STRING_TO_HEX_VALUE     16

#define TEMP1_MAX_DEFAULT       75000
#define TEMP1_MAX_HYST_DEFAULT  70000

/* Addresses scanned */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data */
struct sw_to3200k_thermal_data {
    struct device      *hwmon_dev;
    struct mutex        lock;
    u8                  index;
    unsigned int        temp1_input;
    unsigned int        temp1_max;
    unsigned int        temp1_max_hyst;
};

enum sw_to3200k_thermal_sysfs_attributes {
    TEMP1_INPUT,
    TEMP1_MAX,
    TEMP1_MAX_HYST
};

/* sysfs attributes for hwmon */
static ssize_t get_temp_info(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_temp_info(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

static SENSOR_DEVICE_ATTR(temp1_input,      S_IWUSR|S_IRUGO, get_temp_info, set_temp_info, TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(temp1_max,        S_IWUSR|S_IRUGO, get_temp_info, set_temp_info, TEMP1_MAX);
static SENSOR_DEVICE_ATTR(temp1_max_hyst,   S_IWUSR|S_IRUGO, get_temp_info, set_temp_info, TEMP1_MAX_HYST);

static struct attribute *sw_to3200k_thermal_attributes[] = {
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_temp1_max.dev_attr.attr,
    &sensor_dev_attr_temp1_max_hyst.dev_attr.attr,
    NULL
};

static ssize_t get_temp_info(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_thermal_data  *data = i2c_get_clientdata(client);
    int                             status = -EINVAL;

    mutex_lock(&data->lock);
    switch (attr->index)
    {
        case TEMP1_INPUT:
            status = snprintf(buf, PAGE_SIZE - 1, "%d", data->temp1_input);
            break;
        case TEMP1_MAX:
            status = snprintf(buf, PAGE_SIZE - 1, "%d", TEMP1_MAX_DEFAULT);
            break;
        case TEMP1_MAX_HYST:
            status = snprintf(buf, PAGE_SIZE - 1, "%d", TEMP1_MAX_HYST_DEFAULT);
            break;
        default :
            break;
    }
    mutex_unlock(&data->lock);
    return status;
}

static ssize_t set_temp_info
(
    struct device *dev,
    struct device_attribute *da,
    const char *buf,
    size_t size
)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_thermal_data  *data = i2c_get_clientdata(client);
    long                            keyin = 0;
    int                             error;

    error = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
    if (error)
    {
        return error;
    }

    mutex_lock(&data->lock);
    switch (attr->index)
    {
        case TEMP1_INPUT:
            data->temp1_input = keyin;
            break;
        default :
            break;
    }

    mutex_unlock(&data->lock);
    return size;
}

static const struct attribute_group sw_to3200k_thermal_group = {
    .attrs = sw_to3200k_thermal_attributes,
};

static int sw_to3200k_thermal_probe(struct i2c_client *client)
{
    const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
    struct sw_to3200k_thermal_data  *data;
    int                             status;
    data = kzalloc(sizeof(struct sw_to3200k_thermal_data), GFP_KERNEL);
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
    status = sysfs_create_group(&client->dev.kobj, &sw_to3200k_thermal_group);
    if (status)
    {
        goto exit_free;
    }

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, "wistron_thermal", NULL, NULL);
    if (IS_ERR(data->hwmon_dev))
    {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: thermal '%s'\n", dev_name(data->hwmon_dev), client->name);
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &sw_to3200k_thermal_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void sw_to3200k_thermal_remove(struct i2c_client *client)
{
    struct sw_to3200k_thermal_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &sw_to3200k_thermal_group);
    kfree(data);
}


static const struct i2c_device_id sw_to3200k_thermal_id[] = {
    { "sw_to3200k_thermal", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, sw_to3200k_thermal_id);

static struct i2c_driver sw_to3200k_thermal_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "sw_to3200k_thermal",
    },
    .probe        = sw_to3200k_thermal_probe,
    .remove       = sw_to3200k_thermal_remove,
    .id_table     = sw_to3200k_thermal_id,
    .address_list = normal_i2c,
};

static int __init sw_to3200k_thermal_init(void)
{
    return i2c_add_driver(&sw_to3200k_thermal_driver);
}

static void __exit sw_to3200k_thermal_exit(void)
{
    i2c_del_driver(&sw_to3200k_thermal_driver);
}

module_init(sw_to3200k_thermal_init);
module_exit(sw_to3200k_thermal_exit);

MODULE_AUTHOR("Haowei Chung <haowei_chung@wistron.com>");
MODULE_DESCRIPTION("sw_to3200k_thermal driver");
MODULE_LICENSE("GPL");
