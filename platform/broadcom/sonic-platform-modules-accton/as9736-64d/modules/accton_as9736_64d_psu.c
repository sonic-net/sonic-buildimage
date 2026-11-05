/*
 * An hwmon driver for accton as9736_64d Power Module
 *
 * Copyright (C) 2014 Accton Technology Corporation.
 * Michael Shih <michael_shih@edge-core.com>
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
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
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#define __STDC_WANT_LIB_EXT1__ 1
#include <linux/string.h>

#define MAX_MODEL_NAME          12
#define MAX_SERIAL_NUMBER       11

static ssize_t show_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_string(struct device *dev, struct device_attribute *da, char *buf);
static int as9736_64d_psu_read_block(struct i2c_client *client, u8 command, u8 *data,int data_len);
extern int as9736_64d_cpld_read(unsigned short cpld_addr, u8 reg);

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { 0x50, 0x51, I2C_CLIENT_END };

/* Each client has this additional data
 */
struct as9736_64d_psu_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8  index;           /* PSU index */
    u8  status_present;           /* Status(present) register read from CPLD */
    u8  status_pwr_good;          /* Status(power_good) register read from CPLD */
    char model_name[MAX_MODEL_NAME+1]; /* Model name, read from eeprom */
    char serial_number[MAX_SERIAL_NUMBER+1];
};

static struct as9736_64d_psu_data *as9736_64d_psu_update_device(struct device *dev);

enum as9736_64d_psu_sysfs_attributes {
    PSU_PRESENT,
    PSU_MODEL_NAME,
    PSU_POWER_GOOD,
    PSU_SERIAL_NUMBER
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(psu_present,    S_IRUGO, show_status,    NULL, PSU_PRESENT);
static SENSOR_DEVICE_ATTR(psu_model_name, S_IRUGO, show_string,    NULL, PSU_MODEL_NAME);
static SENSOR_DEVICE_ATTR(psu_power_good, S_IRUGO, show_status,    NULL, PSU_POWER_GOOD);
static SENSOR_DEVICE_ATTR(psu_serial_number, S_IRUGO, show_string, NULL, PSU_SERIAL_NUMBER);


static struct attribute *as9736_64d_psu_attributes[] = {
    &sensor_dev_attr_psu_present.dev_attr.attr,
    &sensor_dev_attr_psu_model_name.dev_attr.attr,
    &sensor_dev_attr_psu_power_good.dev_attr.attr,
    &sensor_dev_attr_psu_serial_number.dev_attr.attr,
    NULL
};

static ssize_t show_status(struct device *dev, struct device_attribute *da,
                           char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct as9736_64d_psu_data *data = as9736_64d_psu_update_device(dev);
    u8 status = 0;

    if (attr->index == PSU_PRESENT) {
        status = !(data->status_present >> data->index & 0x1);
    }
    else { /* PSU_POWER_GOOD */
        status = (data->status_pwr_good >> (1-data->index) & 0x1);
    }

    return sprintf(buf, "%d\n", status);
}

static ssize_t show_string(struct device *dev, struct device_attribute *da,
                               char *buf)
{
   struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct as9736_64d_psu_data *data = as9736_64d_psu_update_device(dev);
    char *ptr = NULL;

    if (!data->valid) {
        return -EIO;
    }

	switch (attr->index) {
	case PSU_MODEL_NAME:
		ptr = data->model_name;
		break;
	case PSU_SERIAL_NUMBER:
		ptr = data->serial_number;
		break;
	default:
		return -EINVAL;
	}

    return sprintf(buf, "%s\n", ptr);
}

static const struct attribute_group as9736_64d_psu_group = {
    .attrs = as9736_64d_psu_attributes,
};

static int as9736_64d_psu_probe(struct i2c_client *client,
                                const struct i2c_device_id *dev_id)
{
    struct as9736_64d_psu_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct as9736_64d_psu_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;
    data->index = dev_id->driver_data;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as9736_64d_psu_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: psu '%s'\n",
             dev_name(data->hwmon_dev), client->name);

    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &as9736_64d_psu_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static int as9736_64d_psu_remove(struct i2c_client *client)
{
    struct as9736_64d_psu_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as9736_64d_psu_group);
    kfree(data);

    return 0;
}

enum psu_index
{
    as9736_64d_psu1,
    as9736_64d_psu2
};

static const struct i2c_device_id as9736_64d_psu_id[] = {
    { "as9736_64d_psu1", as9736_64d_psu1 },
    { "as9736_64d_psu2", as9736_64d_psu2 },
    {}
};
MODULE_DEVICE_TABLE(i2c, as9736_64d_psu_id);

static struct i2c_driver as9736_64d_psu_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as9736_64d_psu",
    },
    .probe        = as9736_64d_psu_probe,
    .remove       = as9736_64d_psu_remove,
    .id_table     = as9736_64d_psu_id,
    .address_list = normal_i2c,
};

static int as9736_64d_psu_read_block(struct i2c_client *client, u8 command, u8 *data,
                                     int data_len)
{
    int result = 0;
    int retry_count = 5;

    while (retry_count) {
        retry_count--;

        result = i2c_smbus_read_i2c_block_data(client, command, data_len, data);

        if (unlikely(result < 0)) {
            msleep(10);
            continue;
        }

        if (unlikely(result != data_len)) {
            result = -EIO;
            msleep(10);
            continue;
        }

        result = 0;
        break;
    }

    return result;
}

static struct as9736_64d_psu_data *as9736_64d_psu_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as9736_64d_psu_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
            || !data->valid) {
        int status;
        int status_present, status_pwr_good;
        int power_good = 0;

        dev_dbg(&client->dev, "Starting as9736_64d update\n");

        /* Read psu status */
        status_present = as9736_64d_cpld_read(0x60, 0x14);
        status_pwr_good = as9736_64d_cpld_read(0x60, 0x90);

        if ( (status_present < 0) || (status_pwr_good < 0) ) {
            dev_dbg(&client->dev, "cpld reg 0x60 offse 0x14 is %d\n", status_present);
            dev_dbg(&client->dev, "cpld reg 0x60 offset 0x90 is %d\n", status_pwr_good);
        }
        else {
            data->status_present = status_present;
            data->status_pwr_good = status_pwr_good;
        }

        /* Read model name */
#ifdef __STDC_LIB_EXT1__
        memset_s(data->model_name, sizeof(data->model_name), 0, sizeof(data->model_name));
#else
        memset(data->model_name, 0, sizeof(data->model_name));
#endif
#ifdef __STDC_LIB_EXT1__
        memset_s(data->serial_number, sizeof(data->model_name), 0, sizeof(data->serial_number));
#else
        memset(data->serial_number, 0, sizeof(data->serial_number));
#endif
        power_good = (data->status_pwr_good >> (1-data->index) & 0x1);
       
        if (power_good) {
            status = as9736_64d_psu_read_block(client, 0x12, data->model_name,
                                               ARRAY_SIZE(data->model_name)-1);                                               
            if (status < 0) {
                data->model_name[0] = '\0';
                dev_dbg(&client->dev, "unable to read model name from (0x%x)\n", client->addr);
            }
            else {
                data->model_name[ARRAY_SIZE(data->model_name)-1] = '\0';
                
            }
             /* Read from offset 0x2e ~ 0x3d (16 bytes) */
            status = as9736_64d_psu_read_block(client, 0x3f, data->serial_number, MAX_SERIAL_NUMBER);
            if (status < 0)
            {
                data->serial_number[0] = '\0';
                dev_dbg(&client->dev, "unable to read model name from (0x%x) offset(0x2e)\n", client->addr);
            }
            data->serial_number[MAX_SERIAL_NUMBER-1]='\0';
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return data;
}

module_i2c_driver(as9736_64d_psu_driver);

MODULE_AUTHOR("Michael Shih <michael_shih@edge-core.com>");
MODULE_DESCRIPTION("as9736_64d_psu driver");
MODULE_LICENSE("GPL");

