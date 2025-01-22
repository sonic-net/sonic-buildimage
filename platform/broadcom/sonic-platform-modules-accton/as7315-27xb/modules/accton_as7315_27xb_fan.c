/*
 * A hwmon driver for the Accton as5710 54x fan contrl
 *
 * Copyright (C) 2013 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/printk.h>


#define DRV_NAME "as5812_54x_fan"

#define FAN_MAX_NUMBER                   5
#define FAN_SPEED_TACH_TO_RPM_STEP       175
#define FAN_SPEED_PWM_STEPS              31
#define FAN_DUTY_CYCLE_MIN               0   /* 10% ??*/
#define FAN_DUTY_CYCLE_MAX               100  /* 100% */


#define I2C_RW_RETRY_COUNT				10
#define I2C_RW_RETRY_INTERVAL			60 /* ms */
#define ATTR_ALLOC_SIZE	1   /*For last attribute which is NUll.*/
#define NAME_SIZE		24

typedef ssize_t (*show_func)( struct device *dev,
                              struct device_attribute *attr,
                              char *buf);
typedef ssize_t (*store_func)(struct device *dev,
                              struct device_attribute *attr,
                              const char *buf, size_t count);


struct fan_sensor {
    struct fan_sensor *next;
    char name[NAME_SIZE+1];	/* sysfs sensor name */
    struct device_attribute attribute;
    bool update;		/* runtime sensor update needed */
    int data;		/* Sensor data. Negative if there was a read error */

    u8 reg;		    /* register */
    u8 mask;		/* bit mask */
    bool invert;	/* inverted value*/

};

#define to_fan_sensor(_attr) \
	container_of(_attr, struct fan_sensor, attribute)


struct model_attrs {
    struct attrs **cmn;
    struct attrs **indiv;
};


struct fan_data_t {
    struct device *dev;
    struct device   *hwmon_dev;

    int num_attributes;
    struct attribute_group group;
    struct fan_sensor *sensors;
    int  attr_index;
    struct model_attrs *attrs;

    struct mutex     update_lock;
    char             valid;           /* != 0 if registers are valid */
    unsigned long    last_updated;    /* In jiffies */

    u8 fan_num;

    u8               status[FAN_MAX_NUMBER];     /* inner first fan status */
    u32              speed[FAN_MAX_NUMBER];      /* inner first fan speed */
    u8               direction[FAN_MAX_NUMBER];  /* reconrd the direction of inner first and second fans */
    u32              duty_cycle[FAN_MAX_NUMBER]; /* control the speed of inner first and second fans */
    u8               r_status[FAN_MAX_NUMBER];   /* inner second fan status */
    u32              r_speed[FAN_MAX_NUMBER];    /* inner second fan speed */
};

static ssize_t show_bit(struct device *dev,
                        struct device_attribute *devattr, char *buf);
static ssize_t show_byte(struct device *dev,
                         struct device_attribute *devattr, char *buf);
static ssize_t set_1bit(struct device *dev, struct device_attribute *da,
                        const char *buf, size_t count);
static ssize_t set_pwm(struct device *dev, struct device_attribute *da,
                       const char *buf, size_t count);
static ssize_t show_pwm(struct device *dev,
                        struct device_attribute *devattr, char *buf);
static ssize_t show_rpm(struct device *dev,
                        struct device_attribute *devattr, char *buf);

struct base_attrs {
    const char *name;
    umode_t mode;
    show_func  get;
    store_func set;
};

struct attrs {
    int reg;
    int mask;
    bool invert;
    struct base_attrs *base;
};

enum common_attrs {
    BRD_VERSION,
    PLD_VERSION,
    PLD_SVERSION,
    NUM_COMMON_ATTR
};

struct base_attrs common_base_attrs[NUM_COMMON_ATTR] =
{
    [BRD_VERSION] = {"borad_ver", S_IRUGO, show_byte, NULL},
    [PLD_VERSION] = {"cpld_ver", S_IRUGO, show_byte, NULL},
    [PLD_SVERSION] = {"cpld_subver", S_IRUGO, show_byte, NULL},
};

struct attrs common_attrs[] = {
    [BRD_VERSION]  = {0x00, -1, false, &common_base_attrs[BRD_VERSION]},
    [PLD_VERSION]  = {0x01, -1, false, &common_base_attrs[PLD_VERSION]},
    [PLD_SVERSION] = {0x02, -1, false, &common_base_attrs[PLD_SVERSION]},
};

struct attrs *as7315_cmn_list[] = {
    &common_attrs[BRD_VERSION],
    &common_attrs[PLD_VERSION],
    &common_attrs[PLD_SVERSION],
    NULL
};

enum fan_attrs {
    _ENABLE,
    _PRESENT,
    _FAULT,
    _SPEED_RPM,
    _PWM,
    _DIRECTION,
    NUM_FAN_ATTRS
};

struct base_attrs tray_base_attrs[NUM_FAN_ATTRS] =
{
    {"enable", S_IRUGO|S_IWUSR, show_bit, set_1bit},
    {"present", S_IRUGO, show_bit, NULL},
    {"fault", S_IRUGO, show_bit, NULL},
    {"input", S_IRUGO, show_rpm, NULL},
    {"pwm", S_IRUGO|S_IWUSR, show_pwm, set_pwm},
    {"dir", S_IRUGO, show_bit, NULL},
};

struct attrs as7315_module[NUM_FAN_ATTRS] = {
    {0x10, -1, false, &tray_base_attrs[_ENABLE]},
    {0x22,  0,  true, &tray_base_attrs[_PRESENT]},
    {0x22,  1, false, &tray_base_attrs[_FAULT]},
    {0x20,  0, false, &tray_base_attrs[_SPEED_RPM]},
    {0x21,  0, false, &tray_base_attrs[_PWM]},
    {-1,   -1, false, &tray_base_attrs[_DIRECTION]},
};

struct attrs *as7315_mod_list[] = {
    &as7315_module[_ENABLE],
    &as7315_module[_PRESENT],
    &as7315_module[_FAULT],
    &as7315_module[_SPEED_RPM],
    &as7315_module[_PWM],
    NULL
};

struct model_attrs models_attr = {
    .cmn = as7315_cmn_list,
    .indiv = as7315_mod_list,
};


static const struct i2c_device_id as7315_fan_id[] = {
    { "as7315_fan", 0},
    { },
};
MODULE_DEVICE_TABLE(i2c, as7315_fan_id);


static int cpld_write_internal(
    struct i2c_client *client, u8 reg, u8 value)
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

static int cpld_read_internal(struct i2c_client *client, u8 reg)
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

static ssize_t show_bit(struct device *dev,
                        struct device_attribute *devattr, char *buf)
{
    int value;
    struct i2c_client *client = to_i2c_client(dev);
    struct fan_data_t *data = i2c_get_clientdata(client);
    struct fan_sensor *sensor = to_fan_sensor(devattr);

    mutex_lock(&data->update_lock);
    value = cpld_read_internal(client, sensor->reg);
    if (unlikely(value < 0)) {
        mutex_unlock(&data->update_lock);
        return value;
    }
    value = value & sensor->mask;
    if (sensor->invert)
        value = !value;
    mutex_unlock(&data->update_lock);

    return snprintf(buf, PAGE_SIZE, "%x\n", !!value);
}

static ssize_t _read_1byte(struct device *dev,
                           struct device_attribute *devattr, u8 *data)
{
    int rv;
    struct i2c_client *client = to_i2c_client(dev);
    struct fan_data_t *fdata = i2c_get_clientdata(client);
    struct fan_sensor *sensor = to_fan_sensor(devattr);

    mutex_lock(&fdata->update_lock);
    rv = cpld_read_internal(client, sensor->reg);
    if (unlikely(rv < 0)) {
        mutex_unlock(&fdata->update_lock);
        return rv;
    }
    mutex_unlock(&fdata->update_lock);
    *data = rv;
    return 0;
}

static ssize_t show_byte(struct device *dev,
                         struct device_attribute *devattr, char *buf)
{
    u8 data;
    int rv;

    rv =_read_1byte(dev, devattr, &data);
    if (unlikely(rv < 0)) {
        return rv;
    }
    return snprintf(buf, PAGE_SIZE, "0x%x\n", data);
}


static u32 reg_val_to_duty_cycle(u8 reg_val)
{
    return ((u32)(reg_val+1) * 156 + 88) / 100;
}
static u8 duty_cycle_to_reg_val(u8 duty_cycle)
{
    return ((u32)duty_cycle * 100 / 155) - 1;
}

static ssize_t show_pwm(struct device *dev,
                        struct device_attribute *devattr, char *buf)
{
    u8 data;
    int rv;

    rv =_read_1byte(dev, devattr, &data);
    if (unlikely(rv < 0)) {
        return rv;
    }
    data = reg_val_to_duty_cycle(data);
    data = (data > FAN_DUTY_CYCLE_MAX)? FAN_DUTY_CYCLE_MAX: data;
    return snprintf(buf, PAGE_SIZE, "%d\n", data);
}

static ssize_t show_rpm(struct device *dev,
                        struct device_attribute *devattr, char *buf)
{
    u8 data;
    int rv;

    rv =_read_1byte(dev, devattr, &data);
    if (unlikely(rv < 0)) {
        return rv;
    }
    rv = data * FAN_SPEED_TACH_TO_RPM_STEP;
    return snprintf(buf, PAGE_SIZE, "%d\n", rv);
}
static ssize_t set_1bit(struct device *dev, struct device_attribute *devattr,
                        const char *buf, size_t count)
{
    long is_reset;
    int value, status;
    struct i2c_client *client = to_i2c_client(dev);
    struct fan_data_t *data = i2c_get_clientdata(client);
    struct fan_sensor *sensor = to_fan_sensor(devattr);
    u8 cpld_bit, reg;

    status = kstrtol(buf, 10, &is_reset);
    if (status) {
        return status;
    }
    reg = sensor->reg;
    cpld_bit = sensor->mask;
    mutex_lock(&data->update_lock);
    value = cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    if (sensor->invert)
        is_reset = !is_reset;

    if (is_reset) {
        value |= cpld_bit;
    }
    else {
        value &= ~cpld_bit;
    }

    status = cpld_write_internal(client, reg, value);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}


static ssize_t set_pwm(struct device *dev, struct device_attribute *devattr,
                       const char *buf, size_t count)
{
    long value;
    int  status;
    struct i2c_client *client = to_i2c_client(dev);
    struct fan_data_t *data = i2c_get_clientdata(client);
    struct fan_sensor *sensor = to_fan_sensor(devattr);
    u8 reg;

    status = kstrtol(buf, 10, &value);
    if (status) {
        return status;
    }

    value = (value > FAN_DUTY_CYCLE_MAX )? FAN_DUTY_CYCLE_MAX: value;
    value = duty_cycle_to_reg_val(value);
    reg = sensor->reg;

    mutex_lock(&data->update_lock);
    status = cpld_write_internal(client, reg, value);
    if (unlikely(status < 0)) {
        mutex_unlock(&data->update_lock);
        return status;
    }
    mutex_unlock(&data->update_lock);
    return count;
}



static int _add_attribute(struct fan_data_t *data, struct attribute *attr)
{
    int new_max_attrs = ++data->num_attributes + ATTR_ALLOC_SIZE;
    void *new_attrs = krealloc(data->group.attrs,
                               new_max_attrs * sizeof(void *),
                               GFP_KERNEL);

    if (!new_attrs)
        return -ENOMEM;

    data->group.attrs = new_attrs;
    data->group.attrs[data->num_attributes-1] = attr;
    data->group.attrs[data->num_attributes] = NULL;

    return 0;
}

static void cpld_dev_attr_init(struct device_attribute *dev_attr,
                               const char *name, umode_t mode,
                               show_func show, store_func store)
{
    sysfs_attr_init(&dev_attr->attr);
    dev_attr->attr.name = name;
    dev_attr->attr.mode = mode;
    dev_attr->show = show;
    dev_attr->store = store;
}

static struct fan_sensor * _add_sensor(struct fan_data_t *data,
                                       const char *name,
                                       u8 reg, u8 mask, bool invert,
                                       bool update, umode_t mode,
                                       show_func  get,  store_func set)
{
    struct fan_sensor *sensor;
    struct device_attribute *a;

    sensor = devm_kzalloc(data->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return NULL;
    a = &sensor->attribute;

    snprintf(sensor->name, sizeof(sensor->name), name);
    sensor->reg = reg;
    sensor->mask = mask;
    sensor->update = update;
    sensor->invert = invert;
    cpld_dev_attr_init(a, sensor->name,
                       mode,
                       get, set);

    if (_add_attribute(data, &a->attr))
        return NULL;

    sensor->next = data->sensors;
    data->sensors = sensor;

    return sensor;
}

static int _add_attributes_cmn(struct fan_data_t *data, struct attrs **cmn)
{
    u8 reg, i ;
    bool invert;
    struct attrs *a;
    struct base_attrs *b;

    if (NULL == cmn)
        return -1;

    for (i = 0; cmn[i]; i++)
    {
        a = cmn[i];
        reg = a->reg;
        invert = a->invert;



        b = a->base;
        if (NULL == b)
            break;


        if (_add_sensor(data, b->name,
                        reg, 0xff, invert,
                        true, b->mode,
                        b->get, b->set) == NULL)
        {
            return -ENOMEM;
        }
    }
    return 0;
}

static int _add_attributes_indiv(struct fan_data_t *data, struct attrs **pa)
{
    char name[NAME_SIZE+1];
    int i, j, mask;
    u8 reg, invert, reg_start, jump;
    struct attrs *a;
    struct base_attrs *b;

    if (NULL == pa)
        return -EFAULT;

    jump = 0x10;
    for (i = 0; pa[i]; i++) {
        a = pa[i];
        reg_start = a->reg;

        if (reg < 0)
            break;

        b = a->base;
        if (b == NULL)
            break;
        invert = a->invert;
        for (j = 0; j < data->fan_num; j++)
        {

            snprintf(name, NAME_SIZE, "fan%d_%s", j+1, b->name);
            /*If mask < 0, mask is as index*/
            if (a->mask < 0) {
                mask = 1 << (j%8);
                reg = reg_start;
            } else { /*If mask >= 0, means to get full byte and reg need to shift*/
                mask = 1 << (a->mask);
                reg = reg_start + ((j%8)*jump);
            }
            if (_add_sensor(data, name, reg, mask, invert,
                            true, b->mode,  b->get,  b->set) == NULL)
            {
                return -ENOMEM;
            }
        }
    }


    return 0;
}

static int _add_attributes(struct i2c_client *client,
                           struct fan_data_t *data)
{
    struct model_attrs *m = data->attrs;

    if (m == NULL)
        return -EINVAL;

    /* Common attributes.*/
    _add_attributes_cmn(data, m->cmn);

    /* Port-wise attributes.*/
    _add_attributes_indiv(data, m->indiv);

    return 0;
}

static int fan_probe(struct i2c_client *client,
                     const struct i2c_device_id *dev_id)
{
    int status;
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    struct fan_data_t *data = NULL;
    struct device *dev = &client->dev;


    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;

    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

    data->attrs = &models_attr;
    data->fan_num = FAN_MAX_NUMBER;
    mutex_init(&data->update_lock);
    data->dev = dev;
    dev_info(dev, "chip found\n");


    status = _add_attributes(client, data);
    if (status) {
        return status;
    }

    if (!data->num_attributes) {
        dev_err(&client->dev, "No attributes found\n");
        return -ENODEV;
    }

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->group);
    if (status) {
        goto out_kfree;
    }

    data->hwmon_dev = hwmon_device_register_with_info(&client->dev,
                      client->name, NULL, NULL, NULL);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_rm_sys;
    }

    i2c_set_clientdata(client, data);
    dev_info(dev, "%s: cpld '%s'\n",
             dev_name(data->dev), client->name);

    return 0;

exit_rm_sys:
    sysfs_remove_group(&client->dev.kobj, &data->group);
out_kfree:
    kfree(data->group.attrs);
    return status;
}


static void fan_remove(struct i2c_client *client)
{
    struct fan_data_t *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &data->group);
    kfree(data->group.attrs);
}


static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

static struct i2c_driver as7315_i2c_fan_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name = DRV_NAME,
    },
    .probe		= fan_probe,
    .remove	   	= fan_remove,
    .id_table     = as7315_fan_id,
    .address_list = normal_i2c,
};

static int __init accton_as7315_27xb_fan_init(void)
{
    return i2c_add_driver(&as7315_i2c_fan_driver);
}

static void __exit accton_as7315_27xb_fan_exit(void)
{
    i2c_del_driver(&as7315_i2c_fan_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("accton_as7315_27xb_fan driver");
MODULE_LICENSE("GPL");

module_init(accton_as7315_27xb_fan_init);
module_exit(accton_as7315_27xb_fan_exit);

