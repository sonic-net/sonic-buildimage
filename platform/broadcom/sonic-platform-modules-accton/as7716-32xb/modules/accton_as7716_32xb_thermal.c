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

#define STRING_TO_DEC_VALUE		10
#define STRING_TO_HEX_VALUE		16

#define TEMP1_MAX_HYST_DEFAULT  75000
#define TEMP1_MAX_DEFAULT       80000
/* Addresses scanned 
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data 
 */
struct as7716_32xb_thermal_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    u8  index;
    unsigned int        temp1_input;
    unsigned int        temp1_max_hyst;
    unsigned int        temp1_max;
};



enum as7716_32xb_thermal_sysfs_attributes {
    TEMP1_INPUT,
    TEMP1_MAX_HYST,
    TEMP1_MAX
};

/* sysfs attributes for hwmon 
 */

static ssize_t temp_info_store(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t temp_info_show(struct device *dev, struct device_attribute *da,
             char *buf);
static SENSOR_DEVICE_ATTR(temp1_input,  S_IWUSR|S_IRUGO, temp_info_show, temp_info_store, TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(temp1_max_hyst, S_IWUSR|S_IRUGO, temp_info_show, temp_info_store, TEMP1_MAX_HYST);
static SENSOR_DEVICE_ATTR(temp1_max, S_IWUSR|S_IRUGO, temp_info_show, temp_info_store, TEMP1_MAX);


static struct attribute *as7716_32xb_thermal_attributes[] = {
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_temp1_max_hyst.dev_attr.attr,
    &sensor_dev_attr_temp1_max.dev_attr.attr,
    NULL
};


static ssize_t temp_info_show(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_thermal_data *data = i2c_get_clientdata(client);
    int status = -EINVAL;
    
    mutex_lock(&data->update_lock);
    switch (attr->index)
    {
        case TEMP1_INPUT:
            status = snprintf(buf, PAGE_SIZE - 1, "%d\r\n", data->temp1_input);
            break;
        case TEMP1_MAX_HYST:
            status = snprintf(buf, PAGE_SIZE - 1, "%d\r\n", TEMP1_MAX_HYST_DEFAULT);
            break;
        case TEMP1_MAX:
            status = snprintf(buf, PAGE_SIZE - 1, "%d\r\n", TEMP1_MAX_DEFAULT);
            break;
        default :
            break;
    }
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t temp_info_store(struct device *dev, struct device_attribute *da,
			const char *buf, size_t size)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_thermal_data *data = i2c_get_clientdata(client);
    long keyin = 0;
    int status = -EINVAL;
    
    mutex_lock(&data->update_lock);
    status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
    switch (attr->index)
    {
        case TEMP1_INPUT:
            data->temp1_input=keyin;
            break;
        case TEMP1_MAX_HYST:
		    data->temp1_max_hyst=keyin;
            break;
        case TEMP1_MAX:
		    data->temp1_max=keyin;
            break;
        default :
            break;
    }
    mutex_unlock(&data->update_lock);
    return size;

}

static const struct attribute_group as7716_32xb_thermal_group = {
    .attrs = as7716_32xb_thermal_attributes,
};

static int as7716_32xb_thermal_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct as7716_32xb_thermal_data *data;
    int status;

    data = kzalloc(sizeof(struct as7716_32xb_thermal_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }
    i2c_set_clientdata(client, data);
    data->index = dev_id->driver_data;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as7716_32xb_thermal_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: thermal '%s'\n",
         dev_name(data->hwmon_dev), client->name);
    
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_thermal_group);
exit_free:
    kfree(data);
exit:
    
    return status;
}

static void as7716_32xb_thermal_remove(struct i2c_client *client)
{
    struct as7716_32xb_thermal_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_thermal_group);
    kfree(data);
    
}


static const struct i2c_device_id as7716_32xb_thermal_id[] = {
    { "as7716_32xb_thermal", 0 },    
    {}
};
MODULE_DEVICE_TABLE(i2c, as7716_32xb_thermal_id);

static struct i2c_driver as7716_32xb_thermal_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as7716_32xb_thermal",
    },
    .probe        = as7716_32xb_thermal_probe,
    .remove       = as7716_32xb_thermal_remove,
    .id_table     = as7716_32xb_thermal_id,
    .address_list = normal_i2c,
};






static int __init as7716_32xb_thermal_init(void)
{
    return i2c_add_driver(&as7716_32xb_thermal_driver);
}

static void __exit as7716_32xb_thermal_exit(void)
{
    i2c_del_driver(&as7716_32xb_thermal_driver);
}

module_init(as7716_32xb_thermal_init);
module_exit(as7716_32xb_thermal_exit);

MODULE_AUTHOR("Jostar yang <jostar_yang@accton.com.tw>");
MODULE_DESCRIPTION("as7716_32xb_thermal driver");
MODULE_LICENSE("GPL");
