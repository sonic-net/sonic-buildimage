/*
 * S9100-32X I2C CPLD driver
 *
 * Copyright (C) 2017 Ingrasys, Inc.
 * Wade He <feng.lee.usa@ingrasys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include "i2c_cpld.h"

#ifdef DEBUG
    #define DEBUG_PRINT(fmt, args...)                                        \
        printk(KERN_INFO "%s[%d]: " fmt "\r\n", \
               __FUNCTION__, __LINE__, ##args)
#else
    #define DEBUG_PRINT(fmt, args...)
#endif

#define I2C_READ_BYTE_DATA(ret, lock, i2c_client, reg) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_read_byte_data(i2c_client, reg); \
    mutex_unlock(lock); \
}
#define I2C_WRITE_BYTE_DATA(ret, lock, i2c_client, reg, val) \
{ \
        mutex_lock(lock); \
        ret = i2c_smbus_write_byte_data(i2c_client, reg, val); \
        mutex_unlock(lock); \
}

/* CPLD sysfs attributes index  */
enum i2c_cpld_sysfs_attributes {
    CPLD_ACCESS_REG,
    CPLD_REGISTER_VAL,
    CPLD_PORT_START,
    CPLD_PORTS,
    CPLD_VERSION,
    CPLD_ID,
    CPLD_BOARD_TYPE,
    CPLD_EXT_BOARD_TYPE,
    CPLD_PW_GOOD,
    CPLD_PW_ABS,
};

/* CPLD sysfs attributes hook functions  */
static ssize_t read_access_register(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_access_register(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_register_value(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_register_value(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_cpld_version(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_board_type(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_ext_board_type(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_pw_good(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_pw_abs(struct device *dev,
                struct device_attribute *da, char *buf);

static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

struct cpld_data {
    int index;                    /* CPLD index */
    struct mutex access_lock;        /* mutex for cpld access */
    u8 access_reg;                /* register to access */
};

/* CPLD device id and data */
static const struct i2c_device_id i2c_cpld_id[] = {
    { "i2c_cpld",  i2c_cpld },
    {}
};

/* Addresses scanned for i2c_cpld */
static const unsigned short cpld_i2c_addr[] = { 0x33, I2C_CLIENT_END };
/* platform sysfs object */
extern struct kobject *s9230_64x_kobj;


/* define all support register access of cpld in attribute */
static SENSOR_DEVICE_ATTR(cpld_access_register, S_IWUSR | S_IRUGO,
        read_access_register, write_access_register, CPLD_ACCESS_REG);
static SENSOR_DEVICE_ATTR(cpld_register_value, S_IWUSR | S_IRUGO,
        read_register_value, write_register_value, CPLD_REGISTER_VAL);
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO,
                read_cpld_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(cpld_board_type, S_IRUGO,
                read_board_type, NULL, CPLD_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(cpld_ext_board_type, S_IRUGO,
        read_ext_board_type, NULL, CPLD_EXT_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(cpld_pw_good, S_IRUGO,
        read_pw_good, NULL, CPLD_PW_GOOD);
static SENSOR_DEVICE_ATTR(cpld_pw_abs, S_IRUGO,
        read_pw_abs, NULL, CPLD_PW_ABS);


/* define support attributes of cpldx , total 5 */
/* cpld 1 */
static struct attribute *i2c_cpld_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_board_type.dev_attr.attr,
    &sensor_dev_attr_cpld_ext_board_type.dev_attr.attr,
    &sensor_dev_attr_cpld_pw_good.dev_attr.attr,
    &sensor_dev_attr_cpld_pw_abs.dev_attr.attr,
    NULL
};

/* cpld 1 attributes group */
static const struct attribute_group i2c_cpld_group = {
    .attrs = i2c_cpld_attributes,
};

/* read access register from cpld data */
static ssize_t read_access_register(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg = data->access_reg;

    return sprintf(buf, "0x%x\n", reg);
}

/* write access register to cpld data */
static ssize_t write_access_register(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;

    if (kstrtou8(buf, 0, &reg) < 0)
        return -EINVAL;

    data->access_reg = reg;
    return count;
}

/* read the value of access register in cpld data */
static ssize_t read_register_value(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg = data->access_reg;
    int reg_val;

    I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);

    if (reg_val < 0)
        return -1;

    return sprintf(buf, "0x%x\n", reg_val);
}

/* wrtie the value to access register in cpld data */
static ssize_t write_register_value(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int ret = -EIO;
    u8 reg = data->access_reg;
    u8 reg_val;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock, client, reg, reg_val);

    return count;
}

/* get cpdl version register value */
static ssize_t read_cpld_version(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_VERSION) {
        reg = CPLD_VERSION_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get board type register value */
static ssize_t read_board_type(struct device *dev,
                struct device_attribute *da,
                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_BOARD_TYPE) {
        reg = CPLD_BOARD_TYPE_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get extend board type register value */
static ssize_t read_ext_board_type(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_EXT_BOARD_TYPE) {
        reg = CPLD_EXT_BOARD_TYPE_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get extend board type register value */
static ssize_t read_pw_good(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_PW_GOOD) {
        reg = CPLD_PW_GOOD_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get extend board type register value */
static ssize_t read_pw_abs(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_PW_ABS) {
        reg = CPLD_PW_ABS_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* add valid cpld client to list */
static void i2c_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = NULL;

    node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);
    if (!node) {
        dev_info(&client->dev,
            "Can't allocate cpld_client_node for index %d\n",
            client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

/* remove exist cpld client in list */
static void i2c_cpld_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);
    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                struct cpld_client_node, list);

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

/* cpld drvier probe */
static int i2c_cpld_probe(struct i2c_client *client,
                    const struct i2c_device_id *dev_id)
{
    int status;
    struct cpld_data *data = NULL;

    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    /* init cpld data for client */
    i2c_set_clientdata(client, data);
    mutex_init(&data->access_lock);

    if (!i2c_check_functionality(client->adapter,
                I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_info(&client->dev,
            "i2c_check_functionality failed (0x%x)\n",
            client->addr);
        status = -EIO;
        goto exit;
    }


    status = sysfs_create_group(&client->dev.kobj,&i2c_cpld_group);

    if (status)
        goto exit;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    i2c_cpld_add_client(client);

    return 0;
exit:
    sysfs_remove_group(&client->dev.kobj, &i2c_cpld_group);
    return status;
}

/* cpld drvier remove */
static int i2c_cpld_remove(struct i2c_client *client)
{

    sysfs_remove_group(&client->dev.kobj, &i2c_cpld_group);

    i2c_cpld_remove_client(client);
    return 0;
}

MODULE_DEVICE_TABLE(i2c, i2c_cpld_id);

static struct i2c_driver i2c_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "i2c_cpld",
    },
    .probe = i2c_cpld_probe,
    .remove = i2c_cpld_remove,
    .id_table = i2c_cpld_id,
    .address_list = cpld_i2c_addr,
};

static int __init i2c_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&i2c_cpld_driver);
}

static void __exit i2c_cpld_exit(void)
{
    i2c_del_driver(&i2c_cpld_driver);
}

MODULE_AUTHOR("Wade He <feng.lee.usa@ingrasys.com>");
MODULE_DESCRIPTION("Ingrasys S9100-32X Platform i2c cpld driver");
MODULE_LICENSE("GPL");

module_init(i2c_cpld_init);
module_exit(i2c_cpld_exit);



