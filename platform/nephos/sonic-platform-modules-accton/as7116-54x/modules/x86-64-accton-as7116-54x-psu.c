/*
 * An hwmon driver for accton as7116_54x Power Module
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

#define PSU_STATUS_I2C_ADDR			0x60
#define PSU_STATUS_I2C_REG_OFFSET	0x2

#define MODEL_NAME_LEN				11
#define MODEL_NAME_REG_OFFSET		0x20

#define SERIAL_NUM_LEN				18
#define SERIAL_NUM_REG_OFFSET		0x35

#define IS_POWER_GOOD(id, value)	(!!(value & BIT(id*4 + 1)))
#define IS_PRESENT(id, value)		(!(value & BIT(id*4)))

static ssize_t show_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_string(struct device *dev, struct device_attribute *da, char *buf);
static int as7116_54x_psu_read_block(struct i2c_client *client, u8 command, u8 *data,int data_len);
extern int as7116_54x_cpld_read(unsigned short cpld_addr, u8 reg);
static struct as7116_54x_psu_data *as7116_54x_psu_update_device(struct device *dev, u8 update_eeprom);

/* Addresses scanned 
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data 
 */
struct as7116_54x_psu_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8  index;           /* PSU index */
    u8  status;          /* Status(present/power_good) register read from CPLD */
	char model_name[MODEL_NAME_LEN+1];	/* Model name, read from eeprom */
	char serial[SERIAL_NUM_LEN+1];		/* Serial number, read from eeprom*/
};             

enum as7116_54x_psu_sysfs_attributes {
	PSU_PRESENT,
	PSU_MODEL_NAME,
	PSU_POWER_GOOD,
	PSU_SERIAL_NUMBER
};

/* sysfs attributes for hwmon 
 */
static SENSOR_DEVICE_ATTR(psu_present,    S_IRUGO, show_status, NULL, PSU_PRESENT);
static SENSOR_DEVICE_ATTR(psu_model_name, S_IRUGO, show_string, NULL, PSU_MODEL_NAME);
static SENSOR_DEVICE_ATTR(psu_power_good, S_IRUGO, show_status, NULL, PSU_POWER_GOOD);
static SENSOR_DEVICE_ATTR(psu_serial_numer, S_IRUGO, show_string,	NULL, PSU_SERIAL_NUMBER);

static struct attribute *as7116_54x_psu_attributes[] = {
    &sensor_dev_attr_psu_present.dev_attr.attr,
    &sensor_dev_attr_psu_model_name.dev_attr.attr,
    &sensor_dev_attr_psu_power_good.dev_attr.attr,
    &sensor_dev_attr_psu_serial_numer.dev_attr.attr,
    NULL
};

static ssize_t show_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct as7116_54x_psu_data *data = as7116_54x_psu_update_device(dev, 0);
    u8 status = 0;

    if (!data->valid) {
        return -EIO;
    }

    if (attr->index == PSU_PRESENT) {
		status = IS_PRESENT(data->index, data->status);
    }
    else { /* PSU_POWER_GOOD */
		status = IS_POWER_GOOD(data->index, data->status);
    }

    return sprintf(buf, "%d\n", status);
}

static ssize_t show_string(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct as7116_54x_psu_data *data = as7116_54x_psu_update_device(dev, 1);
    char *ptr = NULL;

    if (!data->valid) {
        return -EIO;
    }

	if (!IS_PRESENT(data->index, data->status)) {
		return -ENXIO;
	}

    if (attr->index == PSU_MODEL_NAME) {
        ptr = data->model_name;
    }
    else { /* PSU_SERIAL_NUBMER */
        ptr = data->serial;
    }

    return sprintf(buf, "%s\n", ptr);
}

static const struct attribute_group as7116_54x_psu_group = {
    .attrs = as7116_54x_psu_attributes,
};

static int as7116_54x_psu_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct as7116_54x_psu_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct as7116_54x_psu_data), GFP_KERNEL);
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
    status = sysfs_create_group(&client->dev.kobj, &as7116_54x_psu_group);
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
    sysfs_remove_group(&client->dev.kobj, &as7116_54x_psu_group);
exit_free:
    kfree(data);
exit:
    
    return status;
}

static int as7116_54x_psu_remove(struct i2c_client *client)
{
    struct as7116_54x_psu_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as7116_54x_psu_group);
    kfree(data);
    
    return 0;
}

enum psu_index 
{ 
    as7116_54x_psu1,
	as7116_54x_psu2
};

static const struct i2c_device_id as7116_54x_psu_id[] = {
    { "as7116_54x_psu1", as7116_54x_psu1 },
    { "as7116_54x_psu2", as7116_54x_psu2 },
    {}
};
MODULE_DEVICE_TABLE(i2c, as7116_54x_psu_id);

static struct i2c_driver as7116_54x_psu_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as7116_54x_psu",
    },
    .probe        = as7116_54x_psu_probe,
    .remove       = as7116_54x_psu_remove,
    .id_table     = as7116_54x_psu_id,
    .address_list = normal_i2c,
};

static int as7116_54x_psu_read_block(struct i2c_client *client, u8 command, u8 *data,
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

static struct as7116_54x_psu_data *as7116_54x_psu_update_device(struct device *dev, u8 update_eeprom)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as7116_54x_psu_data *data = i2c_get_clientdata(client);
    
    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
        || !data->valid) {
        int status;

        data->valid = 0;
        dev_dbg(&client->dev, "Starting as7116_54x update\n");

		/* Read psu status */
		status = as7116_54x_cpld_read(PSU_STATUS_I2C_ADDR, PSU_STATUS_I2C_REG_OFFSET);
		
		if (status < 0) {
			dev_dbg(&client->dev, "cpld reg (0x%x) err %d\n", PSU_STATUS_I2C_ADDR, status);
			goto exit;
		}
		else {
			data->status = status;
		}
		
		
		memset(data->model_name, 0, sizeof(data->model_name));
		memset(data->serial, 0, sizeof(data->serial));
		
		if (update_eeprom && IS_PRESENT(data->index, data->status)) {
			/* Read model name */
			status = as7116_54x_psu_read_block(client, MODEL_NAME_REG_OFFSET, data->model_name, 
											   ARRAY_SIZE(data->model_name)-1);

			if (status < 0) {
				data->model_name[0] = '\0';
				dev_dbg(&client->dev, "unable to read model name from (0x%x)\n", client->addr);
				goto exit;
			}
			else {
				/* Skip the meaningless data byte 8*/
				data->model_name[8] = data->model_name[9];
				data->model_name[9] = data->model_name[10];
				data->model_name[10] = '\0';
			}

			/* Read serial number */
			status = as7116_54x_psu_read_block(client, SERIAL_NUM_REG_OFFSET, data->serial, 
											   ARRAY_SIZE(data->serial)-1);

			if (status < 0) {
				data->serial[0] = '\0';
				dev_dbg(&client->dev, "unable to read serial number from (0x%x)\n", client->addr);
				goto exit;
			}
			else {
				data->serial[SERIAL_NUM_LEN] = '\0';
			}
		}
		
		data->last_updated = jiffies;
		data->valid = 1;
	}

exit:
	mutex_unlock(&data->update_lock);

	return data;
}

static int __init as7116_54x_psu_init(void)
{
    //extern int platform_accton_as7116_54x(void);
    //if (!platform_accton_as7116_54x()) {
    //    return -ENODEV;
    //}

    return i2c_add_driver(&as7116_54x_psu_driver);
}

static void __exit as7116_54x_psu_exit(void)
{
    i2c_del_driver(&as7116_54x_psu_driver);
}

module_init(as7116_54x_psu_init);
module_exit(as7116_54x_psu_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as7116_54x_psu driver");
MODULE_LICENSE("GPL");

