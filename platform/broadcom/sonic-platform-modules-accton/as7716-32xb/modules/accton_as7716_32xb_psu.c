/*
 * An hwmon driver for accton as7716_32xbb Power Module
 *
 * Copyright (C) 2014 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
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

#define MAX_MODEL_NAME          16
#define DC12V_FAN_DIR_OFFSET    0x34
#define DC12V_FAN_DIR_LEN       3
#define STRING_TO_DEC_VALUE		10

//static ssize_t show_string(struct device *dev, struct device_attribute *da, char *buf);
static int as7716_32xb_psu_read_block(struct i2c_client *client, u8 command, u8 *data,int data_len);
extern int as7716_32xb_cpld_read (unsigned short cpld_addr, u8 reg);

/* Addresses scanned 
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data 
 */
struct as7716_32xb_psu_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8  index;           /* PSU index */
    u8  status;          /* Status(present/power_good) register read from CPLD */
    u8  present;
    u8  power_good;
    char model_name[MAX_MODEL_NAME+1]; /* Model name, read from eeprom */
    char fan_dir[DC12V_FAN_DIR_LEN+1]; /* DC12V fan direction */
};

enum as7716_32xb_psu_sysfs_attributes {
    PSU_PRESENT,
    PSU_MODEL_NAME,
    PSU_POWER_GOOD,
    PSU_FAN_DIR /* For DC12V only */
};

/* sysfs attributes for hwmon 
 */
static ssize_t psu_info_store(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t psu_info_show(struct device *dev, struct device_attribute *da,
             char *buf);
static SENSOR_DEVICE_ATTR(psu_present,  S_IWUSR|S_IRUGO, psu_info_show, psu_info_store, PSU_PRESENT);
static SENSOR_DEVICE_ATTR(psu_model_name, S_IWUSR|S_IRUGO, psu_info_show, psu_info_store, PSU_MODEL_NAME);
static SENSOR_DEVICE_ATTR(psu_power_good, S_IWUSR|S_IRUGO, psu_info_show, psu_info_store, PSU_POWER_GOOD);
static SENSOR_DEVICE_ATTR(psu_fan_dir, S_IWUSR|S_IRUGO, psu_info_show, psu_info_store, PSU_FAN_DIR);


static struct attribute *as7716_32xb_psu_attributes[] = {
    &sensor_dev_attr_psu_present.dev_attr.attr,
    &sensor_dev_attr_psu_model_name.dev_attr.attr,
    &sensor_dev_attr_psu_power_good.dev_attr.attr,
    &sensor_dev_attr_psu_fan_dir.dev_attr.attr,
    NULL
};

static ssize_t psu_info_show(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_psu_data *data = i2c_get_clientdata(client);
    int status = -EINVAL;
    mutex_lock(&data->update_lock);
    switch (attr->index)
    {
        case PSU_PRESENT:
            status = snprintf(buf, PAGE_SIZE - 1, "%d\r\n", data->present);
            break;
        case PSU_MODEL_NAME:
            status = snprintf(buf, PAGE_SIZE - 1, "%s\r\n", data->model_name);
            break;
        case PSU_POWER_GOOD:
            status = snprintf(buf, PAGE_SIZE - 1, "%d\r\n", data->power_good);
            break;
        case PSU_FAN_DIR:
            status = snprintf(buf, PAGE_SIZE - 1, "%s\r\n", data->fan_dir);
            break;             
        default :
            break;
    }
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t psu_info_store(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_psu_data *data = i2c_get_clientdata(client);
    long keyin = 0;
    int status = -EINVAL;
    mutex_lock(&data->update_lock);
    switch (attr->index)
    {
        case PSU_PRESENT:
            status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
	        if (status)
		        goto fail_exit;
	        if (keyin > 1 || keyin < 0)
		        goto fail_exit;
		    data->present=keyin;
            break;
        case PSU_MODEL_NAME:
            memcpy(data->model_name, buf, MAX_MODEL_NAME); 
            break;
        case PSU_POWER_GOOD:
            status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
	        if (status)
	            goto fail_exit;
	        if (keyin > 1 || keyin < 0)
	            goto fail_exit;
		    data->power_good=keyin;
            break;
        case PSU_FAN_DIR:
            memcpy(data->fan_dir, buf, DC12V_FAN_DIR_LEN);
            break;        
        default :
            goto fail_exit;
    }
    mutex_unlock(&data->update_lock);
    return count;
    
fail_exit:
    mutex_unlock(&data->update_lock);
    return -EINVAL;
}

static const struct attribute_group as7716_32xb_psu_group = {
    .attrs = as7716_32xb_psu_attributes,
};

static int as7716_32xb_psu_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct as7716_32xb_psu_data *data;
    int status;

    data = kzalloc(sizeof(struct as7716_32xb_psu_data), GFP_KERNEL);
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
    status = sysfs_create_group(&client->dev.kobj, &as7716_32xb_psu_group);
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
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_psu_group);
exit_free:
    kfree(data);
exit:
    
    return status;
}

static void as7716_32xb_psu_remove(struct i2c_client *client)
{
    struct as7716_32xb_psu_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_psu_group);
    kfree(data);

}

enum psu_index 
{ 
    as7716_32xb_psu1, 
    as7716_32xb_psu2
};

static const struct i2c_device_id as7716_32xb_psu_id[] = {
    { "as7716_32xb_psu1", as7716_32xb_psu1 },
    { "as7716_32xb_psu2", as7716_32xb_psu2 },
    {}
};
MODULE_DEVICE_TABLE(i2c, as7716_32xb_psu_id);

static struct i2c_driver as7716_32xb_psu_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as7716_32xb_psu",
    },
    .probe        = as7716_32xb_psu_probe,
    .remove       = as7716_32xb_psu_remove,
    .id_table     = as7716_32xb_psu_id,
    .address_list = normal_i2c,
};

static int as7716_32xb_psu_read_block(struct i2c_client *client, u8 command, u8 *data,
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

enum psu_type {
    PSU_TYPE_AC_110V,
    PSU_TYPE_DC_48V,
    PSU_TYPE_DC_12V
};

struct model_name_info {
    enum psu_type type;
    u8 offset;
    u8 length;
    char* model_name;
};

struct model_name_info models[] = {
{PSU_TYPE_AC_110V, 0x20, 8,  "YM-2651Y"},
{PSU_TYPE_DC_48V,  0x20, 8,  "YM-2651V"},
{PSU_TYPE_DC_12V,  0x00, 11, "PSU-12V-750"},
};

static int as7716_32xb_psu_model_name_get(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_psu_data *data = i2c_get_clientdata(client);
    int i, status;

    for (i = 0; i < ARRAY_SIZE(models); i++) {
        memset(data->model_name, 0, sizeof(data->model_name));

        status = as7716_32xb_psu_read_block(client, models[i].offset,
                                           data->model_name, models[i].length);
        if (status < 0) {
            data->model_name[0] = '\0';
            dev_dbg(&client->dev, "unable to read model name from (0x%x) offset(0x%x)\n", 
                                  client->addr, models[i].offset);
            return status;
        }
        else {
            data->model_name[models[i].length] = '\0';
        }

        /* Determine if the model name is known, if not, read next index
         */
        if (strncmp(data->model_name, models[i].model_name, models[i].length) == 0) {
            return 0;
        }
        else {
            data->model_name[0] = '\0';
        }
    }

    return -ENODATA;
}

static int __init as7716_32xb_psu_init(void)
{
    return i2c_add_driver(&as7716_32xb_psu_driver);
}

static void __exit as7716_32xb_psu_exit(void)
{
    i2c_del_driver(&as7716_32xb_psu_driver);
}

module_init(as7716_32xb_psu_init);
module_exit(as7716_32xb_psu_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as7716_32xb_psu driver");
MODULE_LICENSE("GPL");

