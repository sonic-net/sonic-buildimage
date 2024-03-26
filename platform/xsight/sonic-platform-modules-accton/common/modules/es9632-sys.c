/*
 * An hwmon driver for accton es9632 Power Module
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

#define EEPROM_SIZE 256

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data
 */
struct es9632_sys_data {
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    u8 eeprom[EEPROM_SIZE];
};

static ssize_t show_eeprom(struct device *dev, struct device_attribute *da,
             char *buf);

enum es9632_sys_sysfs_attributes {
    SYS_EEPROM
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(eeprom, S_IRUGO, show_eeprom, NULL, SYS_EEPROM);

static struct attribute *es9632_sys_attributes[] = {
    &sensor_dev_attr_eeprom.dev_attr.attr,
    NULL
};

static ssize_t show_eeprom(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct es9632_sys_data *data = i2c_get_clientdata(client);
    int status = 0;

    mutex_lock(&data->update_lock);

    if (!data->valid) {
        int i = 0;

        status = i2c_smbus_write_byte_data(client, 0, 0);
        if (unlikely(status < 0)) {
            goto exit;
		}

        for (i = 0; i < sizeof(data->eeprom); i++) {
            status = i2c_smbus_read_byte(client);
			if (unlikely(status < 0)) {
				goto exit;
			}

            data->eeprom[i] = status;
        }

        data->valid = 1;
    }

    memcpy(buf, data->eeprom, sizeof(data->eeprom));
    mutex_unlock(&data->update_lock);
    return 256;

exit:
    mutex_unlock(&data->update_lock);
	return status;
}

static const struct attribute_group es9632_sys_group = {
    .attrs = es9632_sys_attributes,
};

static int es9632_sys_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct es9632_sys_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct es9632_sys_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &es9632_sys_group);
    if (status) {
        goto exit_free;
    }

    return 0;

exit_free:
    kfree(data);
exit:
    return status;
}

static int es9632_sys_remove(struct i2c_client *client)
{
    struct es9632_sys_data *data = i2c_get_clientdata(client);

    sysfs_remove_group(&client->dev.kobj, &es9632_sys_group);
    kfree(data);

    return 0;
}

static const struct i2c_device_id es9632_sys_id[] = {
    { "es9632_sys", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, es9632_sys_id);

static struct i2c_driver es9632_sys_driver = {
    .class        = 0,
    .driver = {
        .name     = "es9632_sys",
    },
    .probe        = es9632_sys_probe,
    .remove       = es9632_sys_remove,
    .id_table     = es9632_sys_id,
    .address_list = normal_i2c,
};

module_i2c_driver(es9632_sys_driver);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@edge-core.com>");
MODULE_DESCRIPTION("es9632_sys driver");
MODULE_LICENSE("GPL");
