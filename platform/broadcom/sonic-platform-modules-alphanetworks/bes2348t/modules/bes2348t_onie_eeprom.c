/*
 * A driver for alphanetworks_bes2348t ONIE EEPROM
 *
 * Copyright (C) 2020 Alphanetworks Technology Corporation.
 * Robin Chen <Robin_chen@Alphanetworks.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * see <http://www.gnu.org/licenses/>
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
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>

#define EEPROM_SIZE      256

static ssize_t onie_read(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t onie_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

/* Each client has this additional data
 */

struct bes2348t_onie_eeprom_data
{
    struct mutex        update_lock;
    unsigned char       onie_eeprom[EEPROM_SIZE];
};


/* Addresses scanned for bes2348t_onie_eeprom */
static const unsigned short normal_i2c[] = { 0x56, I2C_CLIENT_END };

enum bes2348t_onie_eeprom_sysfs_attributes {
    ONIE_RW,
};

static SENSOR_DEVICE_ATTR(eeprom, (0660), onie_read, onie_write, ONIE_RW);

static struct attribute *bes2348t_onie_attributes[] = {
  &sensor_dev_attr_eeprom.dev_attr.attr,
  NULL
};

static const struct attribute_group bes2348t_onie_group = {
  .attrs = bes2348t_onie_attributes,
};


static ssize_t onie_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0;
    u8 command;
    __u8 read_write;
    unsigned short offset = 0;
    union i2c_smbus_data temp;
    struct i2c_client *client = to_i2c_client(dev);
    struct bes2348t_onie_eeprom_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    read_write = I2C_SMBUS_WRITE;
    offset = offset & 0x3fff;
    temp.byte = (u8)offset;
    res = i2c_smbus_xfer(client->adapter, client->addr, client->flags=0,
                            read_write, 0, 2, &temp);
    res = i2c_smbus_xfer(client->adapter, client->addr, client->flags=0,
                            read_write, 0, 2, &temp);
    for( offset=0 ; offset < EEPROM_SIZE ; ++offset )
    {
      read_write = I2C_SMBUS_READ;
      res = i2c_smbus_xfer(client->adapter, client->addr, client->flags=0,
                           read_write, 0, 1, &temp);
      if (!res)
      {
        data->onie_eeprom[offset] = temp.byte;
      }
    }
    memcpy(buf, data->onie_eeprom, EEPROM_SIZE);

    mutex_unlock(&data->update_lock);

    return EEPROM_SIZE;
}

static ssize_t onie_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    int error, write, command, read;

    error = kstrtoint(buf, 10, &write);
    if (error)
      return error;

    if (write < 0 || write > 255)
      return -EINVAL;

    /* Not support yet */

    return count;
}

static int onie_eeprom_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    struct bes2348t_onie_eeprom_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct bes2348t_onie_eeprom_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &bes2348t_onie_group);
    if (status) {
        goto exit_free;
    }

    return 0;

exit_free:
    kfree(data);
exit:
    return status;
}

static int onie_eeprom_remove(struct i2c_client *client)
{
    struct bes2348t_onie_eeprom_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &bes2348t_onie_group);
    kfree(data);

    return 0;
}

static const struct i2c_device_id onie_eeprom_id[] = {
    { "bes2348t_eeprom", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, onie_eeprom_id);

static struct i2c_driver onie_eeprom_driver = {
    .driver = {
        .name = "bes2348t_eeprom",
    },
    .probe     = onie_eeprom_probe,
    .remove    = onie_eeprom_remove,
    .id_table  = onie_eeprom_id,
    .address_list = normal_i2c,
};


static int __init onie_eeprom_init(void)
{
    return i2c_add_driver(&onie_eeprom_driver);
}

static void __exit onie_eeprom_exit(void)

{
    i2c_del_driver(&onie_eeprom_driver);
}

module_init(onie_eeprom_init);
module_exit(onie_eeprom_exit);

MODULE_AUTHOR("Alpha-SID6");
MODULE_DESCRIPTION("ONIE EEPROM Driver");
MODULE_LICENSE("GPL");
