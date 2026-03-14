/*
 * A hwmon driver for the snh60b0-640f_system_cpld
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


#define SYS_LED_REG      0x8
#define FAN12_LED_REG    0x9
#define FAN34_LED_REG    0xA
#define SYS_RESET1_REG   0x2
#define SWI_CTRL_REG     0x4

#define SYS_LOCATOR_LED_BITS    0x01
#define SYS_PWR_LED_BITS        0x0E
#define SYS_STATUS_LED_BITS     0x70
#define FAN135_LED_BITS         0x07
#define FAN246_LED_BITS         0x38
#define REST_BUTTON_BITS        0x0


extern int alpha_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int alpha_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

static ssize_t sys_led_read(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sys_led_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static LIST_HEAD(cpld_client_list);
static struct mutex	 list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

/* Addresses scanned for snh60b0-640f_system_cpld */
static const unsigned short normal_i2c[] = { 0x5F, I2C_CLIENT_END };


enum sysfs_sys_attributes {
  SYS_LOCATOR,
  SYS_PWR,
  SYS_STATUS,
  FAN1_LED,
  FAN2_LED,
  FAN3_LED,
  FAN4_LED,
  SYS_REST1,
  SWI_CTRL,
};

static SENSOR_DEVICE_ATTR(sys_locator, (0660), sys_led_read, sys_led_write, SYS_LOCATOR);
static SENSOR_DEVICE_ATTR(sys_pwr,     (0660), sys_led_read, sys_led_write, SYS_PWR);
static SENSOR_DEVICE_ATTR(sys_status,  (0600), sys_led_read, sys_led_write, SYS_STATUS);
static SENSOR_DEVICE_ATTR(fan1_led,    (0660), sys_led_read, sys_led_write, FAN1_LED);
static SENSOR_DEVICE_ATTR(fan2_led,    (0660), sys_led_read, sys_led_write, FAN2_LED);
static SENSOR_DEVICE_ATTR(fan3_led,    (0660), sys_led_read, sys_led_write, FAN3_LED);
static SENSOR_DEVICE_ATTR(fan4_led,    (0660), sys_led_read, sys_led_write, FAN4_LED);
static SENSOR_DEVICE_ATTR(sys_reset1,  (0660), sys_led_read, sys_led_write, SYS_REST1);
static SENSOR_DEVICE_ATTR(swi_ctrl,    (0660), sys_led_read, NULL, SWI_CTRL);

static struct attribute *snh60b0_sys_attributes[] = {
  &sensor_dev_attr_sys_locator.dev_attr.attr,
  &sensor_dev_attr_sys_pwr.dev_attr.attr,
  &sensor_dev_attr_sys_status.dev_attr.attr,
  &sensor_dev_attr_fan1_led.dev_attr.attr,
  &sensor_dev_attr_fan2_led.dev_attr.attr,
  &sensor_dev_attr_fan3_led.dev_attr.attr,
  &sensor_dev_attr_fan4_led.dev_attr.attr,
  &sensor_dev_attr_sys_reset1.dev_attr.attr,
  &sensor_dev_attr_swi_ctrl.dev_attr.attr,
  NULL
};

static const struct attribute_group snh60b0_sys_group = {
  .attrs = snh60b0_sys_attributes,
};


static ssize_t sys_led_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0;
    u8 command;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    switch(sda->index) {
        case SYS_LOCATOR:
        case SYS_PWR:
        case SYS_STATUS:
          command = SYS_LED_REG;
          break;
        case FAN1_LED:
        case FAN2_LED:
        case FAN3_LED:
        case FAN4_LED:
          command = FAN12_LED_REG + (sda->index - FAN1_LED)/2;
          break;
        case SYS_REST1:
          command = SYS_RESET1_REG;
          break;
        case SWI_CTRL:
          command = SWI_CTRL_REG;
          break;
    }

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    switch(sda->index) {
        case SYS_LOCATOR:
          res = (val & SYS_LOCATOR_LED_BITS) >> 0;
          break;
        case SYS_PWR:
          res = (val & SYS_PWR_LED_BITS) >> 1;
          break;
        case SYS_STATUS:
          res = (val & SYS_STATUS_LED_BITS) >> 4;
          break;
        case FAN1_LED:
        case FAN3_LED:
          res = (val & FAN135_LED_BITS) >> 0;
          break;
        case FAN2_LED:
        case FAN4_LED:
          res = (val & FAN246_LED_BITS) >> 3;
          break;
        case SYS_REST1:
          res = val;
          break;
        case SWI_CTRL:
          res = (val & REST_BUTTON_BITS) >> 0;
          break;
    }

    return sprintf(buf, "%d\n", res);
}

static ssize_t sys_led_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    int error, write, command, read;

    error = kstrtoint(buf, 10, &write);
    if (error)
      return error;


    switch(sda->index) {
        case SYS_LOCATOR:
          if(write < 0 || write > 2)
            return -EINVAL;
        case SYS_PWR:
        case SYS_STATUS:
          if (write < 0 || write > 7)
            return -EINVAL;
          command = SYS_LED_REG;
          break;
        case FAN1_LED:
        case FAN2_LED:
        case FAN3_LED:
        case FAN4_LED:
          if (write < 0 || write > 7)
            return -EINVAL;
          command = FAN12_LED_REG + (sda->index - FAN1_LED)/2;
          break;
        case SYS_REST1:
          if (write < 0 || write > 15)
            return -EINVAL;
          command = SYS_RESET1_REG;
          break;
    }

    read = i2c_smbus_read_byte_data(client, command);
    if (read < 0) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n", client->addr, read);
    }

    switch(sda->index) {
        case SYS_LOCATOR:
          read &= ~SYS_LOCATOR_LED_BITS;
          read |= write << 0;
          break;
        case SYS_PWR:
          read &= ~SYS_PWR_LED_BITS;
          read |= write << 1;
          break;
        case SYS_STATUS:
          read &= ~SYS_STATUS_LED_BITS;
          read |= write << 4;
          break;
        case FAN1_LED:
        case FAN3_LED:
          read &= ~FAN135_LED_BITS;
          read |= write << 0;
          break;
        case FAN2_LED:
        case FAN4_LED:
          read &= ~FAN246_LED_BITS;
          read |= write << 3;
          break;
        case SYS_REST1:
          read = write;
          break;
    }

    i2c_smbus_write_byte_data(client, command, read);

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

static int alpha_i2c_cpld_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    status = sysfs_create_group(&client->dev.kobj, &snh60b0_sys_group);
    if (status) {
      goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    alpha_i2c_cpld_add_client(client);

    return 0;

exit:
    return status;
}

static int alpha_i2c_cpld_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &snh60b0_sys_group);
    alpha_i2c_cpld_remove_client(client);

    return 0;
}

static const struct i2c_device_id alpha_i2c_cpld_id[] = {
    { "snh60b0_system_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, alpha_i2c_cpld_id);

static struct i2c_driver alpha_i2c_cpld_driver = {
    .class		= I2C_CLASS_HWMON,
    .driver = {
        .name = "snh60b0_system_cpld",
    },
    .probe		= alpha_i2c_cpld_probe,
    .remove	   	= alpha_i2c_cpld_remove,
    .id_table	= alpha_i2c_cpld_id,
    .address_list = normal_i2c,
};


static int __init alpha_i2c_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&alpha_i2c_cpld_driver);
}

static void __exit alpha_i2c_cpld_exit(void)
{
    i2c_del_driver(&alpha_i2c_cpld_driver);
}


MODULE_AUTHOR("Alpha-SID6");
MODULE_DESCRIPTION("alpha_system_cpld driver");
MODULE_LICENSE("GPL");

module_init(alpha_i2c_cpld_init);
module_exit(alpha_i2c_cpld_exit);
