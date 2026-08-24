/*
 * An hwmon driver for the Wistron Redundant Power Module
 *
 */

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

#define DRIVER_DESCRIPTION_NAME     "wistron i2c psu driver"
#define MAX_FAN_DUTY_CYCLE          100

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data
 */
struct wistron_i2c_psu_data {
    struct device      *hwmon_dev;
    struct mutex        lock;
    int                 v_in;
    int                 v_out;
    int                 i_in;
    int                 i_out;
    int                 p_in;
    int                 p_out;
    int                 temp_input;
    int                 fan_fault;
    int                 fan_duty_cycle;
    int                 fan_speed;
    int                 pmbus_revision;
    u8                  mfr_id[16];
    u8                  mfr_model[16];
    u8                  mfr_revsion[4];
    u8                  mfr_serial[32];
};

static ssize_t get_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

enum wistron_i2c_psu_sysfs_attributes {
    PSU_V_IN,
    PSU_V_OUT,
    PSU_I_IN,
    PSU_I_OUT,
    PSU_P_IN,
    PSU_P_OUT,
    PSU_PMBUS_REVISION,
    PSU_TEMP1_INPUT,
    PSU_FAN1_FAULT,
    PSU_FAN1_DUTY_CYCLE,
    PSU_FAN1_SPEED,
    PSU_MFR_ID,
    PSU_MFR_MODEL,
    PSU_MFR_REVISION,
    PSU_MFR_SERIAL,
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(psu_v_in,                S_IWUSR | S_IRUGO, get_value,  set_value, PSU_V_IN);
static SENSOR_DEVICE_ATTR(psu_v_out,               S_IWUSR | S_IRUGO, get_value,  set_value, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(psu_i_in,                S_IWUSR | S_IRUGO, get_value,  set_value, PSU_I_IN);
static SENSOR_DEVICE_ATTR(psu_i_out,               S_IWUSR | S_IRUGO, get_value,  set_value, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(psu_p_in,                S_IWUSR | S_IRUGO, get_value,  set_value, PSU_P_IN);
static SENSOR_DEVICE_ATTR(psu_p_out,               S_IWUSR | S_IRUGO, get_value,  set_value, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(psu_pmbus_revision,      S_IWUSR | S_IRUGO, get_value,  set_value, PSU_PMBUS_REVISION);
static SENSOR_DEVICE_ATTR(psu_temp1_input,         S_IWUSR | S_IRUGO, get_value,  set_value, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(psu_fan1_fault,          S_IWUSR | S_IRUGO, get_value,  set_value, PSU_FAN1_FAULT);
static SENSOR_DEVICE_ATTR(psu_fan1_duty_cycle,     S_IWUSR | S_IRUGO, get_value,  set_value, PSU_FAN1_DUTY_CYCLE);
static SENSOR_DEVICE_ATTR(psu_fan1_speed_rpm,      S_IWUSR | S_IRUGO, get_value,  set_value, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(psu_mfr_id,              S_IWUSR | S_IRUGO, get_value,  set_value, PSU_MFR_ID);
static SENSOR_DEVICE_ATTR(psu_mfr_model,           S_IWUSR | S_IRUGO, get_value,  set_value, PSU_MFR_MODEL);
static SENSOR_DEVICE_ATTR(psu_mfr_revision,        S_IWUSR | S_IRUGO, get_value,  set_value, PSU_MFR_REVISION);
static SENSOR_DEVICE_ATTR(psu_mfr_serial,          S_IWUSR | S_IRUGO, get_value,  set_value, PSU_MFR_SERIAL);

/*Duplicate nodes for lm-sensors.*/
static SENSOR_DEVICE_ATTR(in3_input,               S_IRUGO,           get_value,  NULL,      PSU_V_OUT);
static SENSOR_DEVICE_ATTR(curr2_input,             S_IRUGO,           get_value,  NULL,      PSU_I_OUT);
static SENSOR_DEVICE_ATTR(power2_input,            S_IRUGO,           get_value,  NULL,      PSU_P_OUT);
static SENSOR_DEVICE_ATTR(temp1_input,             S_IRUGO,           get_value,  NULL,      PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(fan1_input,              S_IRUGO,           get_value,  NULL,      PSU_FAN1_SPEED);

static struct attribute *wistron_i2c_psu_attributes[] = {
    &sensor_dev_attr_psu_v_in.dev_attr.attr,
    &sensor_dev_attr_psu_v_out.dev_attr.attr,
    &sensor_dev_attr_psu_i_in.dev_attr.attr,
    &sensor_dev_attr_psu_i_out.dev_attr.attr,
    &sensor_dev_attr_psu_p_in.dev_attr.attr,
    &sensor_dev_attr_psu_p_out.dev_attr.attr,
    &sensor_dev_attr_psu_pmbus_revision.dev_attr.attr,
    &sensor_dev_attr_psu_temp1_input.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_fault.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_duty_cycle.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_id.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_revision.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_serial.dev_attr.attr,
    /*Duplicate nodes for lm-sensors.*/
    &sensor_dev_attr_curr2_input.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_power2_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    NULL
};

static ssize_t get_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct wistron_i2c_psu_data     *data = i2c_get_clientdata(client);
    size_t                          ret_count=0;

    mutex_lock(&data->lock);
    switch (attr->index)
    {
        case PSU_V_IN:
        {
            ret_count = sprintf(buf, "%d", data->v_in);
            break;
        }
        case PSU_V_OUT:
        {
            ret_count = sprintf(buf, "%d", data->v_out);
            break;
        }
        case PSU_I_IN:
        {
            ret_count = sprintf(buf, "%d", data->i_in);
            break;
        }
        case PSU_I_OUT:
        {
            ret_count = sprintf(buf, "%d", data->i_out);
            break;
        }
        case PSU_P_IN:
        {
            ret_count = sprintf(buf, "%d", data->p_in);
            break;
        }
        case PSU_P_OUT:
        {
            ret_count = sprintf(buf, "%d", data->p_out);
            break;
        }
        case PSU_PMBUS_REVISION:
        {
            ret_count = sprintf(buf, "%d", data->pmbus_revision);
            break;
        }
        case PSU_TEMP1_INPUT:
        {
            ret_count = sprintf(buf, "%d", data->temp_input);
            break;
        }
        case PSU_FAN1_FAULT:
        {
            ret_count = sprintf(buf, "%d", data->fan_fault);
            break;
        }
        case PSU_FAN1_DUTY_CYCLE:
        {
            ret_count = sprintf(buf, "%d", data->fan_duty_cycle);
            break;
        }
        case PSU_FAN1_SPEED:
        {
            ret_count = sprintf(buf, "%d", data->fan_speed);
            break;
        }
        case PSU_MFR_ID:
        {
            ret_count = sprintf(buf, "%s", data->mfr_id);
            break;
        }
        case PSU_MFR_MODEL:
        {
            ret_count = sprintf(buf, "%s", data->mfr_model);
            break;
        }
        case PSU_MFR_REVISION:
        {
            ret_count = sprintf(buf, "%s", data->mfr_revsion);
            break;
        }
        case PSU_MFR_SERIAL:
        {
            ret_count = sprintf(buf, "%s", data->mfr_serial);
            break;
        }
        default:
            break;
    }
    mutex_unlock(&data->lock);
    return ret_count;
}

static ssize_t set_value
(
    struct device *dev,
    struct device_attribute *da,
    const char *buf,
    size_t count
)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct wistron_i2c_psu_data     *data = i2c_get_clientdata(client);
    int                             error, value;

    mutex_lock(&data->lock);

    switch (attr->index)
    {
        case PSU_V_IN:
        {
            error = kstrtoint(buf, 10, &data->v_in);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_V_OUT:
        {
            error = kstrtoint(buf, 10, &data->v_out);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_I_IN:
        {
            error = kstrtoint(buf, 10, &data->i_in);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_I_OUT:
        {
            error = kstrtoint(buf, 10, &data->i_out);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_P_IN:
        {
            error = kstrtoint(buf, 10, &data->p_in);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_P_OUT:
        {
            error = kstrtoint(buf, 10, &data->p_out);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_PMBUS_REVISION:
        {
            error = kstrtoint(buf, 10, &data->pmbus_revision);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_TEMP1_INPUT:
        {
            error = kstrtoint(buf, 10, &data->temp_input);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_FAN1_FAULT:
        {
            error = kstrtoint(buf, 10, &data->fan_fault);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_FAN1_DUTY_CYCLE:
        {
            error = kstrtoint(buf, 10, &value);
            if (error)
            {
                goto exit_err;
            }

            if (value < 0 || value > MAX_FAN_DUTY_CYCLE)
            {
                error = -EINVAL;
                goto exit_err;
            }

            data->fan_duty_cycle = value;
            break;
        }
        case PSU_FAN1_SPEED:
        {
            error = kstrtoint(buf, 10, &data->fan_speed);
            if (error)
            {
                goto exit_err;
            }

            break;
        }
        case PSU_MFR_ID:
        {
            memzero_explicit(&data->mfr_id, sizeof(data->mfr_id));
            strncpy(data->mfr_id, buf, sizeof(data->mfr_id)-1);
            break;
        }
        case PSU_MFR_MODEL:
        {
            memzero_explicit(&data->mfr_model, sizeof(data->mfr_model));
            strncpy(data->mfr_model, buf, sizeof(data->mfr_model)-1);
            break;
        }
        case PSU_MFR_REVISION:
        {
            memzero_explicit(&data->mfr_revsion, sizeof(data->mfr_revsion));
            strncpy(data->mfr_revsion, buf, sizeof(data->mfr_revsion)-1);
            break;
        }
        case PSU_MFR_SERIAL:
        {
            memzero_explicit(&data->mfr_serial, sizeof(data->mfr_serial));
            strncpy(data->mfr_serial, buf, sizeof(data->mfr_serial)-1);
            break;
        }
        default:
            break;
    }

    mutex_unlock(&data->lock);
    return count;

exit_err:
    mutex_unlock(&data->lock);
    return error;
}

static const struct attribute_group wistron_i2c_psu_group = {
    .attrs = wistron_i2c_psu_attributes,
};

static int wistron_i2c_psu_probe(struct i2c_client *client)
{
    struct wistron_i2c_psu_data *data;
    int                         status;

    data = kzalloc(sizeof(struct wistron_i2c_psu_data), GFP_KERNEL);
    if (!data)
    {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->lock);
    dev_info(&client->dev, "device found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &wistron_i2c_psu_group);
    if (status)
    {
        goto exit_free;
    }

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, "wistron_i2c_psu", NULL, NULL);
    if (IS_ERR(data->hwmon_dev))
    {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: psu '%s'\n", dev_name(data->hwmon_dev), client->name);
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &wistron_i2c_psu_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void wistron_i2c_psu_remove(struct i2c_client *client)
{
    struct wistron_i2c_psu_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &wistron_i2c_psu_group);
    kfree(data);
}

/* Support psu moduel
 */
static const struct i2c_device_id wistron_i2c_psu_id[] = {
    { "acbel_fshxxx", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, wistron_i2c_psu_id);

static struct i2c_driver wistron_i2c_psu_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "wistron_i2c_psu",
    },
    .probe        = wistron_i2c_psu_probe,
    .remove       = wistron_i2c_psu_remove,
    .id_table     = wistron_i2c_psu_id,
    .address_list = normal_i2c,
};

static int __init wistron_i2c_psu_init(void)
{
    return i2c_add_driver(&wistron_i2c_psu_driver);
}

static void __exit wistron_i2c_psu_exit(void)
{
    i2c_del_driver(&wistron_i2c_psu_driver);
}

MODULE_AUTHOR("Haowei Chung <haowei_chung@wistron.com>");
MODULE_DESCRIPTION(DRIVER_DESCRIPTION_NAME);
MODULE_LICENSE("GPL");

module_init(wistron_i2c_psu_init);
module_exit(wistron_i2c_psu_exit);
