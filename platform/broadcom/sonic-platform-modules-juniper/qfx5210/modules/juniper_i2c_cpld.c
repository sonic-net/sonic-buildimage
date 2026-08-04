/*
 * A hwmon driver for the juniper_i2c_cpld
 *
 * Tested and validated on Juniper QFX5210
 * Ciju Rajan K <crajank@juniper.net>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/list.h>


#define MAX_PORT_NUM				    64
#define I2C_RW_RETRY_COUNT				10
#define I2C_RW_RETRY_INTERVAL			60 /* ms */

#define I2C_ADDR_CPLD1  0x60
#define I2C_ADDR_CPLD2  0x62
#define I2C_ADDR_CPLD3  0x64
#define CPLD_ADDRS {I2C_ADDR_CPLD1, I2C_ADDR_CPLD2, I2C_ADDR_CPLD3}


/*
 * Number of additional attribute pointers to allocate
 * with each call to krealloc
 */
#define ATTR_ALLOC_SIZE	1   /*For last attribute which is NUll.*/

#define NAME_SIZE		24
#define MAX_RESP_LENGTH	48

typedef ssize_t (*show_func)( struct device *dev,
                              struct device_attribute *attr,
                              char *buf);
typedef ssize_t (*store_func)(struct device *dev,
                              struct device_attribute *attr,
                              const char *buf, size_t count);

enum models {
    AS7712_32X,
    AS7716_32X,
    qfx5210_64X,
    AS7312_54X,
    PLAIN_CPLD,    /*No attribute but add i2c addr to the list.*/
    NUM_MODEL
};

enum sfp_func {
    HAS_SFP     = 1<<0 ,
    HAS_QSFP    = 1<<1 ,
};

enum common_attrs {
    CMN_VERSION,
    CMN_ACCESS,
    CMN_PRESENT_ALL,
    NUM_COMMON_ATTR
};

enum sfp_attrs {
    SFP_PRESENT,
    SFP_RESET,
    SFP_LP_MODE,
    NUM_SFP_ATTR
};

struct cpld_sensor {
    struct cpld_sensor *next;
    char name[NAME_SIZE+1];	/* sysfs sensor name */
    struct device_attribute attribute;
    bool update;		/* runtime sensor update needed */
    int data;		/* Sensor data. Negative if there was a read error */

    u8 reg;		    /* register */
    u8 mask;		/* bit mask */
    bool invert;	/* inverted value*/

};

#define to_cpld_sensor(_attr) \
	container_of(_attr, struct cpld_sensor, attribute)

struct cpld_data {
    struct device *dev;
    struct device *hwmon_dev;

    int num_attributes;
    struct attribute_group group;

    enum models model;
    struct cpld_sensor *sensors;
    struct mutex update_lock;
    bool valid;
    unsigned long last_updated;	/* in jiffies */

    int  attr_index;
    u16  sfp_num;
    u8   sfp_types;
    struct model_attrs *cmn_attr;
};

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};


struct base_attrs {
    const char *name;
    umode_t mode;
    show_func  get;
    store_func set;
};

struct attrs {
    int reg;
    bool invert;
    struct base_attrs *base;
};

struct model_attrs {
    struct attrs **cmn;
    struct attrs **portly;
};


static ssize_t show_bit(struct device *dev,
                        struct device_attribute *devattr, char *buf);
static ssize_t show_presnet_all(struct device *dev,
                                struct device_attribute *devattr, char *buf);
static ssize_t set_1bit(struct device *dev, struct device_attribute *da,
                        const char *buf, size_t count);
static ssize_t set_byte(struct device *dev, struct device_attribute *da,
                        const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count);

int juniper_i2c_cpld_read(u8 cpld_addr, u8 reg);
int juniper_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);


struct base_attrs common_attrs[NUM_COMMON_ATTR] =
{
    [CMN_VERSION] = {"version", S_IRUGO, show_bit, NULL},
    [CMN_ACCESS] =  {"access",  S_IWUSR, NULL, set_byte},
    [CMN_PRESENT_ALL] = {"module_present_all", S_IRUGO, show_presnet_all, NULL},
};

struct attrs as7712_common[] = {
    [CMN_VERSION]   = {0x01, false, &common_attrs[CMN_VERSION]},
    [CMN_ACCESS]    = {0x00, false, &common_attrs[CMN_ACCESS]},
    [CMN_PRESENT_ALL] = {0x30, false, &common_attrs[CMN_PRESENT_ALL]},
};
struct attrs qfx5210_common[] = {
    [CMN_VERSION]   = {0x01, false, &common_attrs[CMN_VERSION]},
    [CMN_ACCESS]    = {0x00, false, &common_attrs[CMN_ACCESS]},
    [CMN_PRESENT_ALL] = {0x30, false, &common_attrs[CMN_PRESENT_ALL]},
};
struct attrs as7312_common[] = {
    [CMN_VERSION]   = {0x01, false, &common_attrs[CMN_VERSION]},
    [CMN_ACCESS]    = {0x00, false, &common_attrs[CMN_ACCESS]},
    [CMN_PRESENT_ALL] = {-1, false, &common_attrs[CMN_PRESENT_ALL]},
};
struct attrs plain_common[] = {
    [CMN_VERSION] = {0x01, false, &common_attrs[CMN_VERSION]},
};

struct base_attrs portly_attrs[] =
{
    [SFP_PRESENT] = {"module_present", S_IRUGO, show_bit, NULL},
    // Only root user will have the privilege to write to sysfs
    // [SFP_RESET] = {"module_reset", S_IRUGO|S_IWUGO, show_bit, set_1bit},
    [SFP_RESET] = {"module_reset", S_IRUGO|S_IWUSR, show_bit, set_1bit},
};

struct attrs as7712_port[] = {
    {0x30, true, &portly_attrs[SFP_PRESENT]},
    {0x04, true, &portly_attrs[SFP_RESET]},
};

struct attrs qfx5210_port[] = {
    {0x70, true, &portly_attrs[SFP_PRESENT]},
    {0x40, true, &portly_attrs[SFP_RESET]},
};

struct attrs *as7712_cmn_list[] = {
    &as7712_common[CMN_VERSION],
    &as7712_common[CMN_ACCESS],
    &as7712_common[CMN_PRESENT_ALL],
    NULL
};

struct attrs *qfx5210_cmn_list[] = {
    &qfx5210_common[CMN_VERSION],
    &qfx5210_common[CMN_ACCESS],
    &qfx5210_common[CMN_PRESENT_ALL],
    NULL
};

struct attrs *as7312_cmn_list[] = {
    &as7312_common[CMN_VERSION],
    &as7312_common[CMN_ACCESS],
    &as7312_common[CMN_PRESENT_ALL],
    NULL
};

struct attrs *plain_cmn_list[] = {
    &plain_common[CMN_VERSION],
    NULL
};

struct attrs *as7712_port_list[] = {
    &as7712_port[SFP_PRESENT],
    &as7712_port[SFP_RESET],
    NULL
};
struct attrs *qfx5210_port_list[] = {
    &qfx5210_port[SFP_PRESENT],
    &qfx5210_port[SFP_RESET],
    NULL
};

struct model_attrs models_attr[NUM_MODEL] = {
    {.cmn = as7712_cmn_list, .portly=as7712_port_list},
    {.cmn = as7712_cmn_list, .portly=as7712_port_list}, /*7716's as 7712*/
    {.cmn = qfx5210_cmn_list, .portly=qfx5210_port_list},
    {.cmn = as7312_cmn_list, .portly=qfx5210_port_list},
    {.cmn = plain_cmn_list,  .portly=NULL},
};

static LIST_HEAD(cpld_client_list);
static struct mutex	 list_lock;
/* Addresses scanned for juniper_i2c_cpld
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

static int get_sfp_spec(int model, u16 *num, u8 *types)
{
    switch (model) {
    case AS7712_32X:
    case AS7716_32X:
        *num = 32;
        *types = HAS_QSFP;
        break;
    case qfx5210_64X:
        *num = 64;
        *types = HAS_QSFP;
        break;        
    case AS7312_54X:
        *num = 54;
        *types = HAS_QSFP|HAS_SFP;        
    default:
        *types = 0;
        *num = 0;
        break;        
    }

    return 0;
}

static int get_present_reg(int model, u8 port, u8 *cpld_addr, u8 *reg, u8 *num)
{
    u8 cpld_address[] = CPLD_ADDRS;
    
    switch (model) {
    case AS7312_54X:
        if (port < 48) {
            *cpld_addr = cpld_address[1 + port/24];
            *reg = 0x09 + (port%24)/8;
            *num = 8;
        }
        else
        {
            *reg = 0x18;
            *num = 4;
            *cpld_addr = ( port < 52)? cpld_address[1]: cpld_address[2];
        }
        break;
    default:
        return -EINVAL;      
    }
}
        

/*Assume the bits for ports are listed in-a-row.*/
static int get_reg_bit(u8 reg_start, int port,
                       u8 *reg ,u8 *mask)
{
    *reg = reg_start + ((port)/8);
    *mask = 1 << ((port)%8);

    return 0;
}

static int cpld_write_internal(
    struct i2c_client *client, u8 reg, u8 value)
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

static int cpld_read_internal(struct i2c_client *client, u8 reg)
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


/*Turn a numberic array into string with " " between each element.
 * e.g., {0x11, 0x33, 0xff, 0xf1}  => "11 33 ff f1" 
 */
static ssize_t array_stringify(char *buf, u8 *input, size_t size) {

    int i;
    char t[MAX_RESP_LENGTH+1];

    buf[0] = '\0';
    for (i = 0; i < size; i++) {
        snprintf(t, MAX_RESP_LENGTH, "%x ", input[i]);
        strncat(buf, t, MAX_RESP_LENGTH);
    }

    if (strlen(buf) > 0)
        buf[strlen(buf)-1] = '\0'; /*Remove tailing blank*/

    return snprintf(buf, MAX_RESP_LENGTH, "%s\n", buf);
}

static ssize_t show_presnet_all_distinct(struct device *dev,
                                struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 i, value, reg;
    u8 cpld_addr, num;
    u8 _value[8];    
    u64 *values = (u64 *)_value;
    
    values = 0;
    mutex_lock(&data->update_lock);
    while(i < data->sfp_num)
    {
        get_present_reg(data->model, i, &cpld_addr, &reg, &num);
        if(cpld_addr == client->addr)
           value = cpld_read_internal(client, reg);    
        else           
           value = juniper_i2c_cpld_read(cpld_addr, reg);
           
        if (unlikely(value < 0)) {
            goto exit;
        }        

        *values |= (value&((1<<(num))-1)) << i;
        i += num; 
    }
    mutex_unlock(&data->update_lock);

    *values = cpu_to_le64(*values);
    return array_stringify(buf, _value, i);
exit:
    mutex_unlock(&data->update_lock);
    return value;    
}

static ssize_t show_presnet_all(struct device *dev,
                                struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    struct cpld_sensor *sensor = to_cpld_sensor(devattr);
    u8 i, values[MAX_RESP_LENGTH/8];

    if (sensor->reg < 0) {
        return show_presnet_all_distinct(dev, devattr, buf);
    }
    
    mutex_lock(&data->update_lock);
    for (i = 0; i < ((data->sfp_num+7)/8); i++) {
        values[i] = cpld_read_internal(client, sensor->reg + i);
        if (unlikely(values[i] < 0)) {
            goto exit;
        }
    }
    mutex_unlock(&data->update_lock);
    return array_stringify(buf, values, i);
    
exit:
    mutex_unlock(&data->update_lock);
    return values[i];
}

static ssize_t show_bit(struct device *dev,
                        struct device_attribute *devattr, char *buf)
{
    int value;
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    struct cpld_sensor *sensor = to_cpld_sensor(devattr);

    mutex_lock(&data->update_lock);
    value = cpld_read_internal(client, sensor->reg);
    value = value & sensor->mask;
    if (sensor->invert)
        value = !value;
    mutex_unlock(&data->update_lock);

    return snprintf(buf, PAGE_SIZE, "%x\n", value);
}

static ssize_t set_1bit(struct device *dev, struct device_attribute *devattr,
                        const char *buf, size_t count)
{
    long is_reset;
    int value, status;
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    struct cpld_sensor *sensor = to_cpld_sensor(devattr);
    u8 cpld_bit, reg;

    status = kstrtol(buf, 10, &is_reset);
    if (status) {
        return status;
    }
    reg = sensor->reg;
    cpld_bit = sensor->mask;
    mutex_lock(&data->update_lock);
    value = cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }

    if (sensor->invert)
        is_reset = !is_reset;

    if (is_reset) {
        value |= cpld_bit;
    }
    else {
        value &= ~cpld_bit;
    }

    status = cpld_write_internal(client, reg, value);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t set_byte(struct device *dev, struct device_attribute *da,
                        const char *buf, size_t count)
{
    return access(dev, da, buf,  count);
}

static ssize_t access(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count)
{
    int status;
    u32 addr, val;
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);

    if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
        return -EINVAL;
    }

    if (addr > 0xFF || val > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = cpld_write_internal(client, addr, val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static void juniper_i2c_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node =
        kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);

    if (!node) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n",
                client->addr);
        return;
    }
    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void juniper_i2c_cpld_remove_client(struct i2c_client *client)
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

static int cpld_add_attribute(struct cpld_data *data, struct attribute *attr)
{
    int new_max_attrs = ++data->num_attributes + ATTR_ALLOC_SIZE;
    void *new_attrs = krealloc(data->group.attrs,
                               new_max_attrs * sizeof(void *),
                               GFP_KERNEL);
    if (!new_attrs)
        return -ENOMEM;
    data->group.attrs = new_attrs;


    data->group.attrs[data->num_attributes-1] = attr;
    data->group.attrs[data->num_attributes] = NULL;

    return 0;
}

static void cpld_dev_attr_init(struct device_attribute *dev_attr,
                               const char *name, umode_t mode,
                               show_func show, store_func store)
{
    sysfs_attr_init(&dev_attr->attr);
    dev_attr->attr.name = name;
    dev_attr->attr.mode = mode;
    dev_attr->show = show;
    dev_attr->store = store;
}

static struct cpld_sensor * add_sensor(struct cpld_data *data,
                                       const char *name,
                                       u8 reg, u8 mask, bool invert,
                                       bool update, umode_t mode,
                                       show_func  get,  store_func set)
{
    struct cpld_sensor *sensor;
    struct device_attribute *a;

    sensor = devm_kzalloc(data->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return NULL;
    a = &sensor->attribute;

    snprintf(sensor->name, sizeof(sensor->name), name);
    sensor->reg = reg;
    sensor->mask = mask;
    sensor->update = update;
    sensor->invert = invert;
    cpld_dev_attr_init(a, sensor->name,
                       mode,
                       get, set);

    if (cpld_add_attribute(data, &a->attr))
        return NULL;

    sensor->next = data->sensors;
    data->sensors = sensor;

    return sensor;
}

static int add_attributes_cmn(struct cpld_data *data, struct attrs **cmn)
{
    u8 reg, i ;
    bool invert;
    struct attrs *a;
    struct base_attrs *b;

    if (NULL == cmn)
        return -1;

    for (i = 0; cmn[i]; i++)
    {
        a = cmn[i];

        reg = a->reg;
        invert = a->invert;

        b = a->base;
        if (NULL == b)
            break;

        if (add_sensor(data, b->name,
                       reg, 0xff, invert,
                       true, b->mode,
                       b->get, b->set) == NULL)
        {
            return -ENOMEM;
        }
    }
    return 0;
}

static int add_attributes_portly(struct cpld_data *data, struct attrs **pa)
{
    char name[NAME_SIZE+1];
    int i, j;
    u8 reg, mask, invert;
    struct attrs *a;
    struct base_attrs *b;

    if (NULL == pa)
        return -1;


    for (i = 0; pa[i]; i++) {
        a = pa[i];

        invert = a->invert;
        b = a->base;
        if (b == NULL)
            break;

        for (j = 0; j < data->sfp_num; j++)
        {
            snprintf(name, NAME_SIZE, "%s_%d", b->name, j+1);
            get_reg_bit(a->reg, j, &reg, &mask);

            if (add_sensor(data, name, reg, mask, invert,
                           true, b->mode,  b->get,  b->set) == NULL)
            {
                return -ENOMEM;
            }
        }
    }
    return 0;
}

static int add_attributes(struct i2c_client *client,
                          struct cpld_data *data)
{
    struct model_attrs *m = data->cmn_attr;

    if (m == NULL)
        return -EINVAL;

    /* Common attributes.*/
    add_attributes_cmn(data, m->cmn);

    /* Port-wise attributes.*/
    add_attributes_portly(data, m->portly);

    return 0;
}

static int juniper_i2c_cpld_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    int status;
    struct cpld_data *data = NULL;
    struct device *dev = &client->dev;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        return -EIO;
    }

    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

    data->model = dev_id->driver_data;
    data->cmn_attr = &models_attr[data->model];
    get_sfp_spec(data->model, &data->sfp_num, &data->sfp_types);

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    data->dev = dev;
    dev_info(dev, "chip found\n");

    status = add_attributes(client, data);
    if (status)
        goto out_kfree;

    /*
     * If there are no attributes, something is wrong.
     * Bail out instead of trying to register nothing.
     */
    if (!data->num_attributes) {
        dev_err(dev, "No attributes found\n");
        status = -ENODEV;
        goto out_kfree;
    }

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->group);
    if (status) {
        goto out_kfree;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    juniper_i2c_cpld_add_client(client);
    dev_info(dev, "%s: cpld '%s'\n",
             dev_name(data->hwmon_dev), client->name);

    return 0;
exit_remove:
    sysfs_remove_group(&client->dev.kobj, &data->group);
out_kfree:
    kfree(data->group.attrs);
    return status;

}

static int juniper_i2c_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &data->group);
    kfree(data->group.attrs);
    juniper_i2c_cpld_remove_client(client);
    return 0;
}

int juniper_i2c_cpld_read(u8 cpld_addr, u8 reg)
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
EXPORT_SYMBOL(juniper_i2c_cpld_read);

int juniper_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
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
EXPORT_SYMBOL(juniper_i2c_cpld_write);


static const struct i2c_device_id juniper_i2c_cpld_id[] = {
    { "cpld_as7712", AS7712_32X},
    { "cpld_as7716", AS7716_32X},
    { "cpld_qfx5210", qfx5210_64X},
    { "cpld_as7312", AS7312_54X},    
    { "cpld_plain", PLAIN_CPLD},
    { },
};
MODULE_DEVICE_TABLE(i2c, juniper_i2c_cpld_id);

static struct i2c_driver juniper_i2c_cpld_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name = "juniper_i2c_cpld",
    },
    .probe		= juniper_i2c_cpld_probe,
    .remove	   	= juniper_i2c_cpld_remove,
    .id_table     = juniper_i2c_cpld_id,
    .address_list = normal_i2c,
};


static int __init juniper_i2c_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&juniper_i2c_cpld_driver);
}

static void __exit juniper_i2c_cpld_exit(void)
{
    i2c_del_driver(&juniper_i2c_cpld_driver);
}

module_init(juniper_i2c_cpld_init);
module_exit(juniper_i2c_cpld_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("juniper_i2c_cpld driver");
MODULE_LICENSE("GPL");
