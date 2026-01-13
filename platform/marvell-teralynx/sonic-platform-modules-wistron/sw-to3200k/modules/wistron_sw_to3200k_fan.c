
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define DRVNAME                 "sw_to3200k_fan"

static ssize_t get_fan_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_fan_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_version(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_version(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

enum fan_id {
    FAN1_ID,
    FAN2_ID,
    FAN3_ID,
    FAN4_ID,
    FAN5_ID,
    FAN6_ID,
    FAN7_ID
};

enum sysfs_fan_attributes {
    FAN_DUTY_CYCLE_PERCENTAGE, /* Only one CPLD register to control duty cycle for all fans */
    FAN1_PRESENCE,
    FAN2_PRESENCE,
    FAN3_PRESENCE,
    FAN4_PRESENCE,
    FAN5_PRESENCE,
    FAN6_PRESENCE,
    FAN7_PRESENCE,
    FAN1_ENABLE,
    FAN2_ENABLE,
    FAN3_ENABLE,
    FAN4_ENABLE,
    FAN5_ENABLE,
    FAN6_ENABLE,
    FAN7_ENABLE,
	FAN1_DIRECTION,
    FAN2_DIRECTION,
    FAN3_DIRECTION,
    FAN4_DIRECTION,
    FAN5_DIRECTION,
    FAN6_DIRECTION,
    FAN7_DIRECTION,
    FAN1_FRONT_SPEED_RPM,
    FAN2_FRONT_SPEED_RPM,
    FAN3_FRONT_SPEED_RPM,
    FAN4_FRONT_SPEED_RPM,
    FAN5_FRONT_SPEED_RPM,
    FAN6_FRONT_SPEED_RPM,
    FAN7_FRONT_SPEED_RPM,
    FAN1_REAR_SPEED_RPM,
    FAN2_REAR_SPEED_RPM,
    FAN3_REAR_SPEED_RPM,
    FAN4_REAR_SPEED_RPM,
    FAN5_REAR_SPEED_RPM,
    FAN6_REAR_SPEED_RPM,
    FAN7_REAR_SPEED_RPM,
    CPLD_VERSION,
    FAN_ATTR_MAX
};

/* Each client has this additional data */
struct sw_to3200k_fan_data {
    struct device   *hwmon_dev;
    struct mutex     update_lock;
    int              version;
    int              fan_val[FAN_ATTR_MAX];
};

/* version */
static SENSOR_DEVICE_ATTR(version, S_IWUSR | S_IRUGO, get_version, set_version, CPLD_VERSION);

/* Define attributes */
#define DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_duty_cycle_percentage, S_IWUSR | S_IRUGO, get_fan_value, set_fan_value, FAN##index##_DUTY_CYCLE_PERCENTAGE)
#define DECLARE_FAN_DUTY_CYCLE_ATTR(index) &sensor_dev_attr_fan##index##_duty_cycle_percentage.dev_attr.attr

#define DECLARE_FAN_PRESENCE_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_presence, S_IWUSR | S_IRUGO, get_fan_value, set_fan_value, FAN##index##_PRESENCE)
#define DECLARE_FAN_PRESENCE_ATTR(index) &sensor_dev_attr_fan##index##_presence.dev_attr.attr

#define DECLARE_FAN_ENABLE_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_enable, S_IWUSR | S_IRUGO, get_fan_value, set_fan_value, FAN##index##_ENABLE)
#define DECLARE_FAN_ENABLE_ATTR(index) &sensor_dev_attr_fan##index##_enable.dev_attr.attr

#define DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(index) \
    static SENSOR_DEVICE_ATTR(fan##index##_direction, S_IWUSR | S_IRUGO, get_fan_value, set_fan_value, FAN##index##_DIRECTION)
#define DECLARE_FAN_DIRECTION_ATTR(index) &sensor_dev_attr_fan##index##_direction.dev_attr.attr

#define DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(index, index2) \
    static SENSOR_DEVICE_ATTR(fan##index##_front_speed_rpm, S_IWUSR | S_IRUGO, get_fan_value, set_fan_value, FAN##index##_FRONT_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index##_rear_speed_rpm, S_IWUSR | S_IRUGO, get_fan_value, set_fan_value, FAN##index##_REAR_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index##_input, S_IWUSR | S_IRUGO, get_fan_value, set_fan_value, FAN##index##_FRONT_SPEED_RPM);\
    static SENSOR_DEVICE_ATTR(fan##index2##_input, S_IWUSR | S_IRUGO, get_fan_value, set_fan_value, FAN##index##_REAR_SPEED_RPM)
#define DECLARE_FAN_SPEED_RPM_ATTR(index, index2)  &sensor_dev_attr_fan##index##_front_speed_rpm.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index##_rear_speed_rpm.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index##_input.dev_attr.attr, \
                                           &sensor_dev_attr_fan##index2##_input.dev_attr.attr

/* 7 fan speed(rpm) attributes in this platform */
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(1,11);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(2,12);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(3,13);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(4,14);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(5,15);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(6,16);
DECLARE_FAN_SPEED_RPM_SENSOR_DEV_ATTR(7,17);

/* 7 fan presence */
DECLARE_FAN_PRESENCE_SENSOR_DEV_ATTR(1);
DECLARE_FAN_PRESENCE_SENSOR_DEV_ATTR(2);
DECLARE_FAN_PRESENCE_SENSOR_DEV_ATTR(3);
DECLARE_FAN_PRESENCE_SENSOR_DEV_ATTR(4);
DECLARE_FAN_PRESENCE_SENSOR_DEV_ATTR(5);
DECLARE_FAN_PRESENCE_SENSOR_DEV_ATTR(6);
DECLARE_FAN_PRESENCE_SENSOR_DEV_ATTR(7);

/* 7 fan enable */
DECLARE_FAN_ENABLE_SENSOR_DEV_ATTR(1);
DECLARE_FAN_ENABLE_SENSOR_DEV_ATTR(2);
DECLARE_FAN_ENABLE_SENSOR_DEV_ATTR(3);
DECLARE_FAN_ENABLE_SENSOR_DEV_ATTR(4);
DECLARE_FAN_ENABLE_SENSOR_DEV_ATTR(5);
DECLARE_FAN_ENABLE_SENSOR_DEV_ATTR(6);
DECLARE_FAN_ENABLE_SENSOR_DEV_ATTR(7);

/* 7 fan direction */
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(1);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(2);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(3);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(4);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(5);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(6);
DECLARE_FAN_DIRECTION_SENSOR_DEV_ATTR(7);

/* 1 fan duty cycle attribute in this platform */
DECLARE_FAN_DUTY_CYCLE_SENSOR_DEV_ATTR();

static struct attribute *sw_to3200k_fan_attributes[] = {
    /* fan related attributes */
    DECLARE_FAN_SPEED_RPM_ATTR(1,11),
    DECLARE_FAN_SPEED_RPM_ATTR(2,12),
    DECLARE_FAN_SPEED_RPM_ATTR(3,13),
    DECLARE_FAN_SPEED_RPM_ATTR(4,14),
    DECLARE_FAN_SPEED_RPM_ATTR(5,15),
    DECLARE_FAN_SPEED_RPM_ATTR(6,16),
    DECLARE_FAN_SPEED_RPM_ATTR(7,17),
    DECLARE_FAN_PRESENCE_ATTR(1),
    DECLARE_FAN_PRESENCE_ATTR(2),
    DECLARE_FAN_PRESENCE_ATTR(3),
    DECLARE_FAN_PRESENCE_ATTR(4),
    DECLARE_FAN_PRESENCE_ATTR(5),
    DECLARE_FAN_PRESENCE_ATTR(6),
    DECLARE_FAN_PRESENCE_ATTR(7),
    DECLARE_FAN_ENABLE_ATTR(1),
    DECLARE_FAN_ENABLE_ATTR(2),
    DECLARE_FAN_ENABLE_ATTR(3),
    DECLARE_FAN_ENABLE_ATTR(4),
    DECLARE_FAN_ENABLE_ATTR(5),
    DECLARE_FAN_ENABLE_ATTR(6),
    DECLARE_FAN_ENABLE_ATTR(7),
	DECLARE_FAN_DIRECTION_ATTR(1),
    DECLARE_FAN_DIRECTION_ATTR(2),
    DECLARE_FAN_DIRECTION_ATTR(3),
    DECLARE_FAN_DIRECTION_ATTR(4),
    DECLARE_FAN_DIRECTION_ATTR(5),
    DECLARE_FAN_DIRECTION_ATTR(6),
    DECLARE_FAN_DIRECTION_ATTR(7),
    DECLARE_FAN_DUTY_CYCLE_ATTR(),
    &sensor_dev_attr_version.dev_attr.attr,
    NULL
};

static ssize_t get_version(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client               *client = to_i2c_client(dev);
	struct sw_to3200k_fan_data     *data = i2c_get_clientdata(client);
	int                             version;

	mutex_lock(&data->update_lock);
	version = data->version;
	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d", version);
}

static ssize_t set_version(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	struct i2c_client               *client = to_i2c_client(dev);
	struct sw_to3200k_fan_data     *data = i2c_get_clientdata(client);
	int                             error, version;

	error = kstrtoint(buf, 10, &version);
	if (error)
	{
		return error;
	}

	mutex_lock(&data->update_lock);
	data->version = version;
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t get_fan_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_fan_data      *data = i2c_get_clientdata(client);
    int                             value = 0;

    mutex_lock(&data->update_lock);
    value = data->fan_val[attr->index];
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%d", value);
}

static ssize_t set_fan_value
(
    struct device           *dev,
    struct device_attribute *da,
    const char              *buf,
    size_t                  count
)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_fan_data      *data = i2c_get_clientdata(client);
    int                             error, value;

    error = kstrtoint(buf, 10, &value);
    if (error)
    {
        printk("fan_set_value error=%d\n", error);
        return error;
    }

    mutex_lock(&data->update_lock);
    data->fan_val[attr->index] = value;
    mutex_unlock(&data->update_lock);
    return count;
}

static const struct attribute_group sw_to3200k_fan_group = {
    .attrs = sw_to3200k_fan_attributes,
};

static int sw_to3200k_fan_probe(struct i2c_client *client)
{
    struct sw_to3200k_fan_data  *data;
    int                         status;

    data = kzalloc(sizeof(struct sw_to3200k_fan_data), GFP_KERNEL);
    if (!data)
    {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "device found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &sw_to3200k_fan_group);
    if (status)
    {
        goto exit_free;
    }

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, "wistron_fan", NULL, NULL);
    if (IS_ERR(data->hwmon_dev))
    {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: fan '%s'\n", dev_name(data->hwmon_dev), client->name);
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &sw_to3200k_fan_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void sw_to3200k_fan_remove(struct i2c_client *client)
{
    struct sw_to3200k_fan_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &sw_to3200k_fan_group);
}

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x66, I2C_CLIENT_END };

static const struct i2c_device_id sw_to3200k_fan_id[] = {
    { "sw_to3200k_fan", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, sw_to3200k_fan_id);

static struct i2c_driver sw_to3200k_fan_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = DRVNAME,
    },
    .probe        = sw_to3200k_fan_probe,
    .remove       = sw_to3200k_fan_remove,
    .id_table     = sw_to3200k_fan_id,
    .address_list = normal_i2c,
};

module_i2c_driver(sw_to3200k_fan_driver);

MODULE_AUTHOR("Haowei Chung <haowei_chung@wistron.com>");
MODULE_DESCRIPTION("sw_to3200k_fan driver");
MODULE_LICENSE("GPL");
