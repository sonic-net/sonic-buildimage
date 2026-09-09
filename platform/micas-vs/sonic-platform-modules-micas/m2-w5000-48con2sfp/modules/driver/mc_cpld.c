/*
 * mc_cpld.c - A driver for control mc_cpld base on mc_cpld.c
 *
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
 * Copyright (c) 2018 wk <zhengwenkai@micas.com.cn>
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
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/version.h>
#include "mc_cpld.h"


/* debug switch level */ 
typedef enum {
    DBG_START,
    DBG_VERBOSE,
    DBG_KEY,
    DBG_WARN,
    DBG_ERROR,
    DBG_END,
} dbg_level_t;

static int debuglevel = 0;
module_param(debuglevel, int, S_IRUGO | S_IWUSR);

#define DBG_DEBUG(fmt, arg...)  do { \
    if ( debuglevel > DBG_START && debuglevel < DBG_ERROR) { \
          printk(KERN_INFO "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else if ( debuglevel >= DBG_ERROR ) {   \
        printk(KERN_ERR "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else {    } \
} while (0)

#define DBG_ERROR(fmt, arg...)  do { \
     if ( debuglevel > DBG_START) {  \
        printk(KERN_ERR "[ERROR]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
       } \
 } while (0)


#define CPLD_SIZE 256
#define CPLD_I2C_RETRY_TIMES 5          /* changed the number of retry time to 5 */
#define CPLD_I2C_RETRY_WAIT_TIME 10     /* Delay 10ms before operation */


struct cpld_data {
    struct i2c_client   *client;
    struct device       *hwmon_dev;
    struct mutex        update_lock;
    char                valid;              /* !=0 if registers are valid */
    unsigned long       last_updated;       /* In jiffies */
    u8          data[CPLD_SIZE];            /* Register value */
};

static struct i2c_client *g_cpld_client = NULL;

static s32 cpld_i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command)
{
    int try;
    s32 ret;

    ret = -1;
    for (try = 0; try < CPLD_I2C_RETRY_TIMES; try++) {
       if ((ret = i2c_smbus_read_byte_data(client, command) ) >= 0 )
            break;
       msleep(CPLD_I2C_RETRY_WAIT_TIME);
    }
    return ret;
}

static s32 cpld_i2c_smbus_write_byte_data(const struct i2c_client *client, u8 command, u8 value)
{
    int try;
    s32 ret;

    ret = -1;
    for (try = 0; try < CPLD_I2C_RETRY_TIMES; try++) {
       if ((ret = i2c_smbus_write_byte_data(client, command, value) ) >= 0 )
            break;
       msleep(CPLD_I2C_RETRY_WAIT_TIME);
    }
    return ret;
}

static s32 cpld_i2c_smbus_read_i2c_block_data(const struct i2c_client *client,
                u8 command, u8 length, u8 *values)
{
    int try;
    s32 ret;

    ret = -1;
    for (try = 0; try < CPLD_I2C_RETRY_TIMES; try++) {
       if ((ret = i2c_smbus_read_i2c_block_data(client, command, length, values) ) >= 0 )
            break;
       msleep(CPLD_I2C_RETRY_WAIT_TIME);
    }
    return ret;
}

static ssize_t set_cpld_sysfs_value(struct device *dev, struct device_attribute *da, const char *buf, size_t 
count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    unsigned long val;
    int err;
    
    err = kstrtoul(buf, 16, &val);
    if (err)
        return err;
    if ((val < 0) || (val > 0xff)) {
        DBG_ERROR("please enter 0x00 ~ 0xff\n");
        return -1;
    }
    mutex_lock(&data->update_lock);
    data->data[0] = (u8)val;
    DBG_DEBUG("pos: 0x%02x count = %ld, data = 0x%02x\n", attr->index, count, data->data[0]);
    i2c_smbus_write_byte_data(client, attr->index, data->data[0]);
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t show_cpld_version(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    s32 status;
    int size;

    status = -1;
    size = 0;
    mutex_lock(&data->update_lock);
    status = cpld_i2c_smbus_read_i2c_block_data(client, 0, 4, data->data);
    if (status < 0) {
        mutex_unlock(&data->update_lock);
        return 0;
    }
    size = snprintf(buf, CPLD_SIZE, "%02x %02x %02x %02x \n", data->data[0], data->data[1], data->data[2], 
    data->data[3]);
    mutex_unlock(&data->update_lock);
    return size;
}

static ssize_t show_cpld_sysfs_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    s32 status;
    int size;
    
    status = -1;
    size = 0;
    mutex_lock(&data->update_lock);
    status = cpld_i2c_smbus_read_byte_data(client, attr->index);
    if (status < 0) {
        mutex_unlock(&data->update_lock);
        return 0;
    }
    data->data[0] = status;
    DBG_DEBUG("cpld reg pos:0x%x value:0x%02x\n",  attr->index, data->data[0]);
    size = snprintf(buf, CPLD_SIZE, "%02x\n", data->data[0]);
    mutex_unlock(&data->update_lock);
    return size;
}

int port_cpld_read_sfp_status(u8 reg, u8 *value)
{
    struct cpld_data *data;
    s32 ret;
    int ret_val = 0;

    if (!g_cpld_client || !value) {
        DBG_ERROR("cpld client not init or param null\n");
        return -EINVAL;
    }

    data = i2c_get_clientdata(g_cpld_client);
    mutex_lock(&data->update_lock);
    
    ret = cpld_i2c_smbus_read_byte_data(g_cpld_client, reg);
    if (ret < 0) {
        DBG_ERROR("read reg 0x%x failed, ret:%d\n", reg, ret);
        ret_val = -EIO;
        goto out;
    }

    *value = (u8)ret;
    DBG_DEBUG("read reg 0x%x value:0x%02x\n", reg, *value);

out:
    mutex_unlock(&data->update_lock);
    return ret_val;
}
EXPORT_SYMBOL_GPL(port_cpld_read_sfp_status);

int port_cpld_ctl_sfp_led(u8 reg, u8 value)
{
    struct cpld_data *data;
    s32 ret;
    int ret_val = 0;

    if (!g_cpld_client) {
        DBG_ERROR("cpld client not init\n");
        return -EINVAL;
    }

    data = i2c_get_clientdata(g_cpld_client);
    mutex_lock(&data->update_lock);

    ret = cpld_i2c_smbus_write_byte_data(g_cpld_client, reg, value);
    if (ret < 0) {
        DBG_ERROR("write reg 0x%x failed, ret:%d\n", reg, ret);
        ret_val = -EIO;
        goto out;
    }

    DBG_DEBUG("write reg 0x%x value:0x%02x\n", reg, value);

out:
    mutex_unlock(&data->update_lock);
    return ret_val;
}
EXPORT_SYMBOL_GPL(port_cpld_ctl_sfp_led);

/* sys */
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, show_cpld_version, NULL, 0);

/* sfp */
static SENSOR_DEVICE_ATTR(sfp_presence, S_IRUGO, show_cpld_sysfs_value, NULL, 0x22);
static SENSOR_DEVICE_ATTR(sfp_drop, S_IRUGO, show_cpld_sysfs_value, NULL, 0x23);
static SENSOR_DEVICE_ATTR(sfp_fault, S_IRUGO, show_cpld_sysfs_value, NULL, 0x24);
static SENSOR_DEVICE_ATTR(sfp_rxlos, S_IRUGO, show_cpld_sysfs_value, NULL, 0x25);
static SENSOR_DEVICE_ATTR(sfp_txdis0, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x20);
static SENSOR_DEVICE_ATTR(sfp_txdis1, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x21);

// /* port led */
// static SENSOR_DEVICE_ATTR(port_led0, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x40);
// static SENSOR_DEVICE_ATTR(port_led1, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x41);
// static SENSOR_DEVICE_ATTR(port_led2, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x42);
// static SENSOR_DEVICE_ATTR(port_led3, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x43);
// static SENSOR_DEVICE_ATTR(port_led4, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x44);
// static SENSOR_DEVICE_ATTR(port_led5, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x45);
// static SENSOR_DEVICE_ATTR(port_led6, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x46);

static struct attribute *cpld_0x30_sysfs_attrs[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_sfp_presence.dev_attr.attr,
    &sensor_dev_attr_sfp_drop.dev_attr.attr,
    &sensor_dev_attr_sfp_fault.dev_attr.attr,
    &sensor_dev_attr_sfp_rxlos.dev_attr.attr,
    &sensor_dev_attr_sfp_txdis0.dev_attr.attr,
    &sensor_dev_attr_sfp_txdis1.dev_attr.attr,
    // &sensor_dev_attr_sfp_protect0.dev_attr.attr,
    // &sensor_dev_attr_port_led0.dev_attr.attr,
    // &sensor_dev_attr_port_led1.dev_attr.attr,
    // &sensor_dev_attr_port_led2.dev_attr.attr,
    // &sensor_dev_attr_port_led3.dev_attr.attr,
    // &sensor_dev_attr_port_led4.dev_attr.attr,
    // &sensor_dev_attr_port_led5.dev_attr.attr,
    // &sensor_dev_attr_port_led6.dev_attr.attr,
    NULL
};

static const struct attribute_group  cpld_0x30_sysfs_group = {
    .attrs = cpld_0x30_sysfs_attrs,
};


struct cpld_attr_match_group {
    int bus_nr;                                         /* I2C-BUS number */
    unsigned short addr;                                /* device adress */
    const struct attribute_group   *attr_group_ptr;     /* SYS attribute pointer */
    const struct attribute_group   *attr_hwmon_ptr;     /* HWMON attribute pointer */
};

static struct cpld_attr_match_group g_cpld_attr_match[] = {
    {5, 0x30, &cpld_0x30_sysfs_group, NULL},
};

static  const struct attribute_group *cpld_get_attr_group(struct i2c_client *client, int is_hwmon)
{
    int i;
    struct cpld_attr_match_group *group;
    
    for (i = 0; i < ARRAY_SIZE(g_cpld_attr_match); i++) {
        group = &g_cpld_attr_match[i];
        DBG_DEBUG("is_hwmon %d i %d client(nr:%d,addr:0x%x), group(nr:%d,addr:0x0%x) .\n", is_hwmon,
            i, client->adapter->nr, client->addr, group->bus_nr, group->addr);
        if ((client->addr == group->addr) && (client->adapter->nr == group->bus_nr)) {
            DBG_DEBUG("is_hwmon %d i %d nr %d addr %d .\n", is_hwmon, i, client->adapter->nr, client->addr);
            return (is_hwmon) ? (group->attr_hwmon_ptr) : (group->attr_group_ptr);
        }
    }

    DBG_DEBUG("is_hwmon %d nr %d addr %d dismatch, return NULL.\n", is_hwmon, client->adapter->nr, client->addr);
    return NULL;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
#else
static int cpld_probe(struct i2c_client *client)
#endif
{
    struct cpld_data *data;
    int status;
    const struct attribute_group *sysfs_group, *hwmon_group;

    status = -1;
    DBG_DEBUG("=========cpld_probe(addr:0x%x, nr:%d)===========\n", client->addr, client->adapter->nr);
    data = devm_kzalloc(&client->dev, sizeof(struct cpld_data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }
    
    data->client = client;
    i2c_set_clientdata(client, data);
    g_cpld_client = client;
    mutex_init(&data->update_lock);

    sysfs_group = NULL;
    sysfs_group = cpld_get_attr_group(client, 0);
    if (sysfs_group) {
        status = sysfs_create_group(&client->dev.kobj, sysfs_group);
        DBG_DEBUG("=========(addr:0x%x, nr:%d) sysfs_create_group status %d===========\n", client->addr, client->adapter->nr, status);
        if (status != 0) {
            DBG_ERROR("sysfs_create_group status %d.\n", status);
            goto error; 
        }
    } else {
        DBG_DEBUG("=========(addr:0x%x, nr:%d) no sysfs_create_group \n", client->addr, client->adapter->nr);
    }
    
    hwmon_group = NULL;
    hwmon_group = cpld_get_attr_group(client, 1);
    if (hwmon_group) {
        data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, data, (const struct attribute_group **)hwmon_group);
        if (IS_ERR(data->hwmon_dev)) {
            if (sysfs_group) {
                sysfs_remove_group(&client->dev.kobj, (const struct attribute_group *)sysfs_group);
            }
            DBG_ERROR("hwmon_device_register_with_groups failed ret %ld.\n", PTR_ERR(data->hwmon_dev));
            return PTR_ERR(data->hwmon_dev);
        }
        DBG_DEBUG("=========(addr:0x%x, nr:%d) hwmon_device_register_with_groups success===========\n", client->addr, client->adapter->nr);
        if (status != 0) {
            DBG_ERROR("sysfs_create_group status %d.\n", status);
            goto error; 
        }
    } else {
        DBG_DEBUG("=========(addr:0x%x, nr:%d) no hwmon_device_register_with_groups \n", client->addr, client->adapter->nr);
    }

error:
    return status;

}

static void cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    const struct attribute_group *sysfs_group, *hwmon_group;
    g_cpld_client = NULL;
    
    DBG_DEBUG("=========cpld_remove(addr:0x%x, nr:%d)===========\n", client->addr, client->adapter->nr);

    /* To be added the corresponding uninstall operation */
    sysfs_group = NULL;
    sysfs_group = cpld_get_attr_group(client, 0);
    if (sysfs_group) {
        DBG_DEBUG("=========(addr:0x%x, nr:%d) do sysfs_remove_group \n", client->addr, client->adapter->nr);
        sysfs_remove_group(&client->dev.kobj, (const struct attribute_group *)sysfs_group);
    } else {
        DBG_DEBUG("=========(addr:0x%x, nr:%d) no sysfs_remove_group \n", client->addr, client->adapter->nr);
    }
    
    hwmon_group = NULL;
    hwmon_group = cpld_get_attr_group(client, 1);
    if (hwmon_group) {
        DBG_DEBUG("=========(addr:0x%x, nr:%d) do hwmon_device_unregister \n", client->addr, client->adapter->nr);
        hwmon_device_unregister(data->hwmon_dev);
    } else {
        DBG_DEBUG("=========(addr:0x%x, nr:%d) no hwmon_device_unregister \n", client->addr, client->adapter->nr);
    }   
    
    return;
}

static const struct i2c_device_id cpld_id[] = {
    { "mc_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver mc_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name   = "mc_cpld",
    },
    .probe      = cpld_probe,
    .remove     = cpld_remove,
    .id_table   = cpld_id,
};

module_i2c_driver(mc_cpld_driver);
MODULE_AUTHOR("wk <zhengwenkai@micas.com.cn>");
MODULE_DESCRIPTION("micas CPLD driver");
MODULE_LICENSE("GPL");
