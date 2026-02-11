
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

/* QSFP-DD: page0 (low page (128 byte)), page 0 (high page (128 byte)), page 2 (high page (128 byte)), page11 (high page (128 byte))*/
#define EEPROM_DATA_SIZE        128

/* Addresses scanned */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };
#define MAX_PORT_NAME_LEN       20

enum sysfs_fan_attributes {
    OOM_LP_MODE,
    OOM_TEMP,
    OOM_EEPROM_LOW,
    OOM_EEPROM_PG0,
    OOM_EEPROM_PG1,
    OOM_EEPROM_PG2,
    OOM_EEPROM_PG3,
    OOM_EEPROM_PG11,
    OOM_PORT_NAME,
    OOM_ATTR_MAX
};

/* Each client has this additional data */
struct sw_to3200k_oom_data {
    struct device   *hwmon_dev;
    struct mutex    lock;
    u8              index;
    int             lp_mode;
    int             temp;
    unsigned char   eeproml[EEPROM_DATA_SIZE];
    unsigned char   eeprom0[EEPROM_DATA_SIZE];
    unsigned char   eeprom1[EEPROM_DATA_SIZE];
    unsigned char   eeprom2[EEPROM_DATA_SIZE];
    unsigned char   eeprom3[EEPROM_DATA_SIZE];
    unsigned char   eeprom11[EEPROM_DATA_SIZE];
    char            port_name[MAX_PORT_NAME_LEN];
};

/* sysfs attributes for hwmon */
static ssize_t get_oom_value(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_oom_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_oom_info(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_oom_info(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t get_port_name(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_port_name(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static SENSOR_DEVICE_ATTR(lp_mode, S_IWUSR | S_IRUGO, get_oom_value, set_oom_value, OOM_LP_MODE);
static SENSOR_DEVICE_ATTR(temp, S_IWUSR | S_IRUGO, get_oom_value, set_oom_value, OOM_TEMP);
static SENSOR_DEVICE_ATTR(eeprom_low, S_IWUSR | S_IRUGO, get_oom_info, set_oom_info, OOM_EEPROM_LOW);
static SENSOR_DEVICE_ATTR(eeprom_pg0, S_IWUSR | S_IRUGO, get_oom_info, set_oom_info, OOM_EEPROM_PG0);
static SENSOR_DEVICE_ATTR(eeprom_pg1, S_IWUSR | S_IRUGO, get_oom_info, set_oom_info, OOM_EEPROM_PG1);
static SENSOR_DEVICE_ATTR(eeprom_pg2, S_IWUSR | S_IRUGO, get_oom_info, set_oom_info, OOM_EEPROM_PG2);
static SENSOR_DEVICE_ATTR(eeprom_pg3, S_IWUSR | S_IRUGO, get_oom_info, set_oom_info, OOM_EEPROM_PG3);
static SENSOR_DEVICE_ATTR(eeprom_pg11, S_IWUSR | S_IRUGO, get_oom_info, set_oom_info, OOM_EEPROM_PG11);
static SENSOR_DEVICE_ATTR(port_name, S_IRUGO | S_IWUSR, get_port_name, set_port_name, OOM_PORT_NAME);

static struct attribute *sw_to3200k_oom_attributes[] = {
    &sensor_dev_attr_lp_mode.dev_attr.attr,
    &sensor_dev_attr_temp.dev_attr.attr,
    &sensor_dev_attr_eeprom_low.dev_attr.attr,
    &sensor_dev_attr_eeprom_pg0.dev_attr.attr,
    &sensor_dev_attr_eeprom_pg1.dev_attr.attr,
    &sensor_dev_attr_eeprom_pg2.dev_attr.attr,
    &sensor_dev_attr_eeprom_pg3.dev_attr.attr,
    &sensor_dev_attr_eeprom_pg11.dev_attr.attr,
    &sensor_dev_attr_port_name.dev_attr.attr,
    NULL
};

static ssize_t get_oom_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_oom_data      *data = i2c_get_clientdata(client);
    int                             value = 0;

    mutex_lock(&data->lock);
    if (attr->index == OOM_LP_MODE)
    {
        value = data->lp_mode;
    }
    else
    {
        value = data->temp;
    }
    mutex_unlock(&data->lock);
    return sprintf(buf, "%d", value);
}

static ssize_t set_oom_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_oom_data      *data = i2c_get_clientdata(client);
    int                             error, value;

    error = kstrtoint(buf, 10, &value);
    if (error)
    {
        printk("oom_set_value error=%d\n", error);
        return error;
    }

    mutex_lock(&data->lock);
    if (attr->index == OOM_LP_MODE)
    {
        data->lp_mode = value;
    }
    else
    {
        data->temp = value;
    }
    mutex_unlock(&data->lock);
    return count;
}

static ssize_t get_oom_info(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_oom_data      *data = i2c_get_clientdata(client);

    mutex_lock(&data->lock);
    switch (attr->index)
    {
        case OOM_EEPROM_LOW:
        {
            memcpy(buf, data->eeproml, EEPROM_DATA_SIZE);
            break;
        }
        case OOM_EEPROM_PG0:
        {
            memcpy(buf, data->eeprom0, EEPROM_DATA_SIZE);
            break;
        }
        case OOM_EEPROM_PG1:
        {
            memcpy(buf, data->eeprom1, EEPROM_DATA_SIZE);
            break;
        }
        case OOM_EEPROM_PG2:
        {
            memcpy(buf, data->eeprom2, EEPROM_DATA_SIZE);
            break;
        }
        case OOM_EEPROM_PG3:
        {
            memcpy(buf, data->eeprom3, EEPROM_DATA_SIZE);
            break;
        }
        case OOM_EEPROM_PG11:
        {
            memcpy(buf, data->eeprom11, EEPROM_DATA_SIZE);
            break;
        }
        default:
            break;
    }
    mutex_unlock(&data->lock);
    return EEPROM_DATA_SIZE;
}

static ssize_t set_oom_info(struct device *dev, struct device_attribute *da, const char *buf, size_t size)
{
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
    struct i2c_client               *client = to_i2c_client(dev);
    struct sw_to3200k_oom_data      *data = i2c_get_clientdata(client);
    int                             i=0, j=0, k=0;
    unsigned char                   str[3];
    unsigned int                    val;

    k=0;
    mutex_lock(&data->lock);
    memset(str, 0x0, 3);
    if (strlen(buf) >= EEPROM_DATA_SIZE)
    {
        for (i=0; i < strlen(buf) ; i++)
        {
            for (j=0;j<2; j++)
            {
                str[j]=buf[i+j];
            }
            sscanf(str, "%x", &val);
            i=j+i-1;
            if (k>=EEPROM_DATA_SIZE)
            {
                break;
            }

            switch (attr->index)
            {
                case OOM_EEPROM_LOW:
                {
                    data->eeproml[k]=(unsigned char)val;
                    break;
                }
                case OOM_EEPROM_PG0:
                {
                    data->eeprom0[k]=(unsigned char)val;
                    break;
                }
                case OOM_EEPROM_PG1:
                {
                    data->eeprom1[k]=(unsigned char)val;
                    break;
                }
                case OOM_EEPROM_PG2:
                {
                    data->eeprom2[k]=(unsigned char)val;
                    break;
                }
                case OOM_EEPROM_PG3:
                {
                    data->eeprom3[k]=(unsigned char)val;
                    break;
                }
                case OOM_EEPROM_PG11:
                {
                    data->eeprom11[k]=(unsigned char)val;
                    break;
                }
                default:
                    break;
            }

            k++;
        }
    }
    else
    {
        switch (attr->index)
        {
            case OOM_EEPROM_LOW:
            {
                memzero_explicit(&data->eeproml, sizeof(data->eeproml));
                break;
            }
            case OOM_EEPROM_PG0:
            {
                memzero_explicit(&data->eeprom0, sizeof(data->eeprom0));
                break;
            }
            case OOM_EEPROM_PG2:
            {
                memzero_explicit(&data->eeprom2, sizeof(data->eeprom2));
                break;
            }
            case OOM_EEPROM_PG3:
            {
                memzero_explicit(&data->eeprom3, sizeof(data->eeprom3));
                break;
            }
            case OOM_EEPROM_PG11:
            {
                memzero_explicit(&data->eeprom11, sizeof(data->eeprom11));
                break;
            }
            default:
                break;
        }
    }

    mutex_unlock(&data->lock);
    return size;
}

static ssize_t get_port_name(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client           *client = to_i2c_client(dev);
    struct sw_to3200k_oom_data  *data = i2c_get_clientdata(client);
    ssize_t                     count;

    mutex_lock(&data->lock);
    count = sprintf(buf, "%s", data->port_name);
    mutex_unlock(&data->lock);
    return count;
}

static ssize_t set_port_name(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client           *client = to_i2c_client(dev);
    struct sw_to3200k_oom_data  *data = i2c_get_clientdata(client);
    char                        port_name[MAX_PORT_NAME_LEN];

    if (sscanf(buf, "%19s", port_name) != 1)
    {
        return -EINVAL;
    }

    mutex_lock(&data->lock);
    strcpy(data->port_name, port_name);
    mutex_unlock(&data->lock);
    return count;
}

static const struct attribute_group sw_to3200k_oom_group = {
    .attrs = sw_to3200k_oom_attributes,
};

static int sw_to3200k_oom_probe(struct i2c_client *client)
{
    const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
    struct sw_to3200k_oom_data *data;
    int status;
    data = kzalloc(sizeof(struct sw_to3200k_oom_data), GFP_KERNEL);
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
    status = sysfs_create_group(&client->dev.kobj, &sw_to3200k_oom_group);
    if (status)
    {
        goto exit_free;
    }

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, "wistron_oom", NULL, NULL);
    if (IS_ERR(data->hwmon_dev))
    {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: oom '%s'\n", dev_name(data->hwmon_dev), client->name);
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &sw_to3200k_oom_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void sw_to3200k_oom_remove(struct i2c_client *client)
{
    struct sw_to3200k_oom_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &sw_to3200k_oom_group);
    kfree(data);
}

static const struct i2c_device_id sw_to3200k_oom_id[] = {
    { "sw_to3200k_oom", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, sw_to3200k_oom_id);

static struct i2c_driver sw_to3200k_oom_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "sw_to3200k_oom",
    },
    .probe        = sw_to3200k_oom_probe,
    .remove       = sw_to3200k_oom_remove,
    .id_table     = sw_to3200k_oom_id,
    .address_list = normal_i2c,
};

static int __init sw_to3200k_oom_init(void)
{
    return i2c_add_driver(&sw_to3200k_oom_driver);
}

static void __exit sw_to3200k_oom_exit(void)
{
    i2c_del_driver(&sw_to3200k_oom_driver);
}

module_init(sw_to3200k_oom_init);
module_exit(sw_to3200k_oom_exit);

MODULE_AUTHOR("Haowei Chung <haowei_chung@wistron.com>");
MODULE_DESCRIPTION("sw_to3200k_oom driver");
MODULE_LICENSE("GPL");
