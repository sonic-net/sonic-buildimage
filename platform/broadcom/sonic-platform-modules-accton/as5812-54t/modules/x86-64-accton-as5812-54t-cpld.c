/*
 * A hwmon driver for the as5812_54t_cpld
 *
 * Copyright (C) 2013 Accton Technology Corporation.
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
#include <linux/list.h>

static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

#define I2C_RW_RETRY_COUNT                10
#define I2C_RW_RETRY_INTERVAL            60 /* ms */

static ssize_t show_bit(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t set_1bit(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t access(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
             char *buf);
static int as5812_54t_cpld_read_internal(struct i2c_client *client, u8 reg);
static int as5812_54t_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value);

struct as5812_54t_cpld_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
};

/* Addresses scanned for as5812_54t_cpld
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };



#define _ATTR_CONCAT(name,idx) name##idx

#define TRANSCEIVER_ATTR_ID(_attr) \
    _ATTR_CONCAT(MODULE_##_attr##_, 49), \
    _ATTR_CONCAT(MODULE_##_attr##_, 50), \
    _ATTR_CONCAT(MODULE_##_attr##_, 51), \
    _ATTR_CONCAT(MODULE_##_attr##_, 52), \
    _ATTR_CONCAT(MODULE_##_attr##_, 53), \
    _ATTR_CONCAT(MODULE_##_attr##_, 54) 


enum as5812_54t_cpld_sysfs_attributes {
    CPLD_VERSION,
    ACCESS,
    MODULE_PRESENT_ALL,
    /* transceiver attributes */
    TRANSCEIVER_ATTR_ID(PRESENT),
    TRANSCEIVER_ATTR_ID(LPMODE),
    TRANSCEIVER_ATTR_ID(RESET),
};

/* sysfs attributes for hwmon 
 */

/* transceiver attributes */
#define DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_bit, NULL, MODULE_PRESENT_##index);\
    static SENSOR_DEVICE_ATTR(module_lp_mode_##index, S_IRUGO|S_IWUSR, show_bit, set_1bit, MODULE_LPMODE_##index);\
    static SENSOR_DEVICE_ATTR(module_reset_##index, S_IRUGO|S_IWUSR, show_bit, set_1bit, MODULE_RESET_##index)

#define DECLARE_TRANSCEIVER_ATTR(index)  &sensor_dev_attr_module_present_##index.dev_attr.attr, \
	&sensor_dev_attr_module_lp_mode_##index.dev_attr.attr, \
	&sensor_dev_attr_module_reset_##index.dev_attr.attr



static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);
/* transceiver attributes */
static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, show_present_all, NULL, MODULE_PRESENT_ALL);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(49);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(50);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(51);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(52);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(53);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(54);

static struct attribute *as5812_54t_cpld_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_ATTR(49),
    DECLARE_TRANSCEIVER_ATTR(50),
    DECLARE_TRANSCEIVER_ATTR(51),
    DECLARE_TRANSCEIVER_ATTR(52),
    DECLARE_TRANSCEIVER_ATTR(53),
    DECLARE_TRANSCEIVER_ATTR(54),
    NULL
};

static const struct attribute_group as5812_54t_cpld_group = {
    .attrs = as5812_54t_cpld_attributes,
};

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
             char *buf)
{
    int status;
    u8 value  = 0;
    u8 reg = 0x22;
    struct i2c_client *client = to_i2c_client(dev);
    struct as5812_54t_cpld_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    status = as5812_54t_cpld_read_internal(client, reg);
    if (status < 0) {
        goto exit;
    }

    value = ~(u8)status;
    value &= 0x3F;

    mutex_unlock(&data->update_lock);

    /* Return values 49 -> 54 in order */
    return sprintf(buf, "%.2x\n", value);

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static int get_reg_index(struct sensor_device_attribute *attr, u8 *reg, u8 *index, u8 *revert) {
    if (attr->index >= MODULE_RESET_49){
        *reg  = 0x23;
	*index = attr->index - MODULE_RESET_49;
	*revert = 1;
    }else if (attr->index >= MODULE_LPMODE_49){
        *reg  = 0x24;
	*index = attr->index - MODULE_LPMODE_49;
	*revert = 0;
    } else {
        *reg  = 0x22;
	*index = attr->index - MODULE_PRESENT_49;
	*revert = 1;
    }
    return 0;
} 

static ssize_t set_1bit(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    u8 index, revert;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as5812_54t_cpld_data *data = i2c_get_clientdata(client);
    int status, value;
    u8 reg = 0, mask = 0;

    status = kstrtoint(buf, 10, &value);
    if (status) 
	return status;

    get_reg_index(attr, &reg, &index, &revert);
    mask = 0x1 << index;

    mutex_lock(&data->update_lock);
    status = as5812_54t_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    status &= ~(mask);
    value = !value;
    if (!revert)
        value = !value;
    status |= (value << index);  /*low-active*/
    status = as5812_54t_cpld_write_internal(client, reg, status);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_bit(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 index, revert;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as5812_54t_cpld_data *data = i2c_get_clientdata(client);
    int status = 0;
    u8 reg = 0, mask = 0;

    get_reg_index(attr, &reg, &index, &revert);
    mask = 0x1 << index;

    mutex_lock(&data->update_lock);
    status = as5812_54t_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    status = !(status & mask);
    if (!revert)
        status = !status;

    return sprintf(buf, "%d\n", status);

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 reg = 0, mask = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as5812_54t_cpld_data *data = i2c_get_clientdata(client);
    int status = 0;

    switch (attr->index) {
    case CPLD_VERSION:
        reg  = 0x1;
        mask = 0xFF;
        break;
    default:
        break;
    }

    mutex_lock(&data->update_lock);
    status = as5812_54t_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%d\n", (status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t access(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    int status;
    u32 addr, val;
    struct i2c_client *client = to_i2c_client(dev);
    struct as5812_54t_cpld_data *data = i2c_get_clientdata(client);

    if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
        return -EINVAL;
    }

    if (addr > 0xFF || val > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = as5812_54t_cpld_write_internal(client, addr, val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static int as5812_54t_cpld_read_internal(struct i2c_client *client, u8 reg)
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

static int as5812_54t_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
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

static void as5812_54t_cpld_add_client(struct i2c_client *client)
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

static void as5812_54t_cpld_remove_client(struct i2c_client *client)
{
    struct list_head        *list_node = NULL;
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

static int as5812_54t_cpld_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    int status;
    struct as5812_54t_cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct as5812_54t_cpld_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as5812_54t_cpld_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register_with_info(&client->dev, "as5812_54t_cpld",
                                                      NULL, NULL, NULL);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    as5812_54t_cpld_add_client(client);

    /*
     * Bring QSFPs out of reset,
     * This is a temporary fix until the QSFP+_MOD_RST register
     * can be exposed through the driver.
     */
    as5812_54t_cpld_write_internal(client, 0x23, 0x3F);

    dev_info(&client->dev, "%s: cpld '%s'\n",
         dev_name(data->hwmon_dev), client->name);

    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &as5812_54t_cpld_group);
exit_free:
    kfree(data);
exit:
    
    return status;
}

static void as5812_54t_cpld_remove(struct i2c_client *client)
{
    struct as5812_54t_cpld_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as5812_54t_cpld_group);
    kfree(data);
    as5812_54t_cpld_remove_client(client);

}

int as5812_54t_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client->addr == cpld_addr) {
            ret = i2c_smbus_read_byte_data(cpld_node->client, reg);
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as5812_54t_cpld_read);

int as5812_54t_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client->addr == cpld_addr) {
            ret = i2c_smbus_write_byte_data(cpld_node->client, reg, value);
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as5812_54t_cpld_write);

static const struct i2c_device_id as5812_54t_cpld_id[] = {
    { "as5812_54t_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, as5812_54t_cpld_id);

static struct i2c_driver as5812_54t_cpld_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as5812_54t_cpld",
    },
    .probe        = as5812_54t_cpld_probe,
    .remove       = as5812_54t_cpld_remove,
    .id_table     = as5812_54t_cpld_id,
    .address_list = normal_i2c,
};

static int __init as5812_54t_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&as5812_54t_cpld_driver);
}

static void __exit as5812_54t_cpld_exit(void)
{
    i2c_del_driver(&as5812_54t_cpld_driver);
}

module_init(as5812_54t_cpld_init);
module_exit(as5812_54t_cpld_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as5812_54t_cpld driver");
MODULE_LICENSE("GPL");

