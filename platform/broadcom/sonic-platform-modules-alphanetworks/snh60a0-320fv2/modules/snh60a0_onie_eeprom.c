/*
 * A driver for alphanetworks_snh60a0_320fv2 onie eeprom
 *
 * Copyright (C) 2018 Alphanetworks Technology Corporation.
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
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>

#define EEPROM_SIZE      256

#define SYS_LED_REG      0x8
#define FAN12_LED_REG    0x9
#define FAN34_LED_REG    0xA
#define FAN56_LED_REG    0xB
#define SYS_RESET1_REG   0x2

#define SYS_LOCATOR_LED_BITS    0x01
#define SYS_PWR_LED_BITS        0x0E
#define SYS_STATUS_LED_BITS     0x70
#define FAN135_LED_BITS         0x07
#define FAN246_LED_BITS         0x38


extern int alpha_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int alpha_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

static ssize_t onie_read(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t onie_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static LIST_HEAD(cpld_client_list);
static struct mutex	 list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

/* Addresses scanned for alphanetworks_snh60a0_320fv2_onie_eeprom */
static const unsigned short normal_i2c[] = { 0x56, I2C_CLIENT_END };


enum sysfs_onie_attributes {
  ONIE_RW,
};

static SENSOR_DEVICE_ATTR(eeprom, (0660), onie_read, onie_write, ONIE_RW);

static struct attribute *alphanetworks_snh60a0_320fv2_onie_attributes[] = {
  &sensor_dev_attr_eeprom.dev_attr.attr,
  NULL
};

static const struct attribute_group alphanetworks_snh60a0_320fv2_onie_group = {
  .attrs = alphanetworks_snh60a0_320fv2_onie_attributes,
};


static ssize_t onie_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0;
    u8 command;
    struct i2c_client *client = to_i2c_client(dev);
    __u8 read_write;
    unsigned short offset = 0;
    union i2c_smbus_data temp;
    char rbuf[EEPROM_SIZE];

    for( offset=0 ; offset < EEPROM_SIZE ; ++offset )
    {
      read_write = I2C_SMBUS_WRITE;
      offset = offset & 0x3fff;
      temp.byte = (u8)offset;
      res = i2c_smbus_xfer(client->adapter, client->addr, client->flags=0,
                           read_write, 0, 2, &temp);
      res = i2c_smbus_xfer(client->adapter, client->addr, client->flags=0,
                           read_write, 0, 2, &temp);

      read_write = I2C_SMBUS_READ;
      temp.byte = 0xaa;
      res = i2c_smbus_xfer(client->adapter, client->addr, client->flags=0,
                           read_write, 0, 1, &temp);
      if (!res)
      {
        res = temp.byte;
        rbuf[offset] = (char)temp.byte;
      }

      read_write = I2C_SMBUS_READ;
      temp.byte = 0xbb;
      res = i2c_smbus_xfer(client->adapter, client->addr, client->flags=0,
                           read_write, 0, 1, &temp);
      if (!res)
      {
        res = temp.byte;
      }
    }

    memcpy(buf, rbuf, EEPROM_SIZE);
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



static void alpha_i2c_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);

    if (!node) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void alpha_i2c_cpld_remove_client(struct i2c_client *client)
{
    struct list_head		*list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(cpld_node);
    }

    mutex_unlock(&list_lock);
}

static int onie_eeprom_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    status = sysfs_create_group(&client->dev.kobj, &alphanetworks_snh60a0_320fv2_onie_group);
    if (status) {
      goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    alpha_i2c_cpld_add_client(client);

    return 0;

exit:
    return status;
}

static int onie_eeprom_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &alphanetworks_snh60a0_320fv2_onie_group);
    alpha_i2c_cpld_remove_client(client);

    return 0;
}

static const struct i2c_device_id onie_eeprom_id[] = {
    { "snh60a0_onie_eeprom", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, onie_eeprom_id);

static struct i2c_driver onie_eeprom_driver = {
    .class		= I2C_CLASS_HWMON,
    .driver = {
        .name = "snh60a0_onie_eeprom",
    },
    .probe		= onie_eeprom_probe,
    .remove	   	= onie_eeprom_remove,
    .id_table	= onie_eeprom_id,
    .address_list = normal_i2c,
};


static int __init onie_eeprom_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&onie_eeprom_driver);
}

static void __exit onie_eeprom_exit(void)

{
    i2c_del_driver(&onie_eeprom_driver);
}


MODULE_AUTHOR("Alpha-SID6");
MODULE_DESCRIPTION("onie eeprom driver");
MODULE_LICENSE("GPL");

module_init(onie_eeprom_init);
module_exit(onie_eeprom_exit);
