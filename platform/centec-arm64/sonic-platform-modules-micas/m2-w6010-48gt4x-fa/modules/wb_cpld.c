/*
 * wb_cpld.c - A driver for control wb_cpld base on wb_cpld.c
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
    char                valid;       /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8          data[CPLD_SIZE]; /* Register value */
};

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

    status = -1;
    mutex_lock(&data->update_lock);
    status = cpld_i2c_smbus_read_i2c_block_data(client, 0, 4, data->data);
    if (status < 0) {
        mutex_unlock(&data->update_lock);
        return 0;
    }
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%02x %02x %02x %02x \n", data->data[0], data->data[1], data->data[2],
    data->data[3]);
}

static ssize_t show_cpld_sysfs_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    s32 status;

    status = -1;
    mutex_lock(&data->update_lock);
    status = cpld_i2c_smbus_read_byte_data(client, attr->index);
    if (status < 0) {
        mutex_unlock(&data->update_lock);
        return 0;
    }
    data->data[0]  = status;
    DBG_DEBUG("cpld reg pos:0x%x value:0x%02x\n",  attr->index, data->data[0]);
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%02x\n", data->data[0]);
}

/* sys */
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, show_cpld_version, NULL, 0);

/* sfp */
static SENSOR_DEVICE_ATTR(sfp_presence1, S_IRUGO, show_cpld_sysfs_value, NULL, 0x30);
static SENSOR_DEVICE_ATTR(cable_led1, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x20);
static SENSOR_DEVICE_ATTR(cable_led2, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x21);
static SENSOR_DEVICE_ATTR(cable_led3, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x22);
static SENSOR_DEVICE_ATTR(cable_led4, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x23);
static SENSOR_DEVICE_ATTR(cable_led5, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x24);
static SENSOR_DEVICE_ATTR(cable_led6, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x25);
static SENSOR_DEVICE_ATTR(sfp_led1, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x26);
static SENSOR_DEVICE_ATTR(sfp_drop_record1, S_IRUGO , show_cpld_sysfs_value, NULL, 0x38);
static SENSOR_DEVICE_ATTR(sfp_tx_fault1, S_IRUGO , show_cpld_sysfs_value, NULL, 0x50);
static SENSOR_DEVICE_ATTR(sfp_rx_loss1, S_IRUGO , show_cpld_sysfs_value, NULL, 0x70);
/* tx-disbale */
static SENSOR_DEVICE_ATTR(tx_disable, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x90);
static SENSOR_DEVICE_ATTR(tx_write_protect, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x0e);

static struct attribute *mac_cpld_0x30_sysfs_attrs[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_sfp_presence1.dev_attr.attr,
    &sensor_dev_attr_cable_led1.dev_attr.attr,
    &sensor_dev_attr_cable_led2.dev_attr.attr,
    &sensor_dev_attr_cable_led3.dev_attr.attr,
    &sensor_dev_attr_cable_led4.dev_attr.attr,
    &sensor_dev_attr_cable_led5.dev_attr.attr,
    &sensor_dev_attr_cable_led6.dev_attr.attr,
    &sensor_dev_attr_sfp_led1.dev_attr.attr,
    &sensor_dev_attr_tx_disable.dev_attr.attr,
    &sensor_dev_attr_tx_write_protect.dev_attr.attr,
    &sensor_dev_attr_sfp_drop_record1.dev_attr.attr,
    &sensor_dev_attr_sfp_tx_fault1.dev_attr.attr,
    &sensor_dev_attr_sfp_rx_loss1.dev_attr.attr,
    NULL
};

static const struct attribute_group  mac_cpld_0x30_sysfs_group = {
    .attrs = mac_cpld_0x30_sysfs_attrs,
};

struct cpld_attr_match_group {
    int bus_nr;           /* I2C-BUS number */
    unsigned short addr;    /* device adress */
    const struct attribute_group   *attr_group_ptr;/* SYS attribute pointer */
    const struct attribute_group   *attr_hwmon_ptr;/* HWMON Attribute pointer */
};

static struct cpld_attr_match_group g_cpld_attr_match[] = {
    {3, 0x30, &mac_cpld_0x30_sysfs_group, NULL},

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

static int cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
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
            sysfs_remove_group(&client->dev.kobj,  (const struct attribute_group *)sysfs_group);
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

static int cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    const struct attribute_group *sysfs_group, *hwmon_group;

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

    return 0;
}

static const struct i2c_device_id cpld_id[] = {
    { "wb_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver wb_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name   = "wb_cpld",
    },
    .probe      = cpld_probe,
    .remove     = cpld_remove,
    .id_table   = cpld_id,
};

module_i2c_driver(wb_cpld_driver);
MODULE_AUTHOR("wk <support>");
MODULE_DESCRIPTION("CPLD driver");
MODULE_LICENSE("GPL");
