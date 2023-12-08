/*
 * A hwmon driver for the as7315_i2c_cpld
 *
 * Copyright (C) 2019 Accton Technology Corporation.
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
#include <linux/i2c-mux.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/printk.h>


#define DRV_NAME    "as7315_i2c_cpld"
#define NUM_SFP      24
#define NUM_QSFP     3
#define NUM_ALL_PORTS    27

#define  SFP_1ST_PRST_REG   0x10
#define  QSFP_1ST_PRST_REG   0x18

#define  SFP_1ST_RXLOS_REG   0x13
#define  MUX_CHANNEL_SELECT_REG 0x80


enum models {
    MDL_CPLD_SFP,
    MDL_CPLD_QSFP,
    PLAIN_CPLD,    /*No attribute but add i2c addr to the list.*/
    NUM_MODEL
};


#define MAX_PORT_NUM				    64
#define I2C_RW_RETRY_COUNT				10
#define I2C_RW_RETRY_INTERVAL			60 /* ms */

/*
 * Number of additional attribute pointers to allocate
 * with each call to krealloc
 */
#define ATTR_ALLOC_SIZE	1   /*For last attribute which is NUll.*/

#define NAME_SIZE		24

typedef ssize_t (*show_func)( struct device *dev,
                              struct device_attribute *attr,
                              char *buf);
typedef ssize_t (*store_func)(struct device *dev,
                              struct device_attribute *attr,
                              const char *buf, size_t count);

enum port_type {
    PT_SFP,
    PT_QSFP,
};

enum common_attrs {
    CMN_VERSION,
    CMN_ACCESS,
    CMN_PRESENT_ALL,
    CMN_RXLOS_ALL,
    NUM_COMMON_ATTR
};

enum sfp_sb_attrs {
    SFP_PRESENT,
    SFP_RESET,
    SFP_TX_DIS,
    SFP_TX_FAULT,
    SFP_RX_LOS,
    QSFP_PRESENT,
    QSFP_LP_MODE,
    NUM_SFP_SB_ATTR
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
    struct model_attrs *attrs;

    /*For mux function*/
    struct i2c_mux_core *muxc;
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
static ssize_t show_rxlos_all(struct device *dev,
                              struct device_attribute *devattr, char *buf);
static ssize_t show_presnet_all(struct device *dev,
                                struct device_attribute *devattr, char *buf);
static ssize_t set_1bit(struct device *dev, struct device_attribute *da,
                        const char *buf, size_t count);
static ssize_t set_byte(struct device *dev, struct device_attribute *da,
                        const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count);

int accton_i2c_cpld_read(u8 cpld_addr, u8 reg);
int accton_i2c_cpld_write(u8 cpld_addr, u8 reg, u8 value);


struct base_attrs common_base_attrs[NUM_COMMON_ATTR] =
{
    [CMN_VERSION] = {"version", S_IRUGO, show_bit, NULL},
    [CMN_ACCESS] =  {"access",  S_IWUSR, NULL, set_byte},
    [CMN_PRESENT_ALL] = {"module_present_all", S_IRUGO, show_presnet_all, NULL},
    [CMN_RXLOS_ALL] = {"rx_los_all", S_IRUGO, show_rxlos_all, NULL},
};

struct attrs common_attrs[] = {
    [CMN_VERSION]   = {0x01, false, &common_base_attrs[CMN_VERSION]},
    [CMN_ACCESS]    = {-1, false, &common_base_attrs[CMN_ACCESS]},
    [CMN_PRESENT_ALL] = {-1, false, &common_base_attrs[CMN_PRESENT_ALL]},
    [CMN_RXLOS_ALL] = {-1, false, &common_base_attrs[CMN_RXLOS_ALL]},
};
struct attrs plain_common[] = {
    [CMN_VERSION] = {0x01, false, &common_base_attrs[CMN_VERSION]},
};

struct base_attrs portly_attrs[] =
{
    [SFP_PRESENT] = {"present", S_IRUGO, show_bit, NULL},
    [SFP_RESET] = {0},
    [SFP_TX_DIS] =  {"tx_disable", S_IRUGO|S_IWUSR, show_bit, set_1bit},
    [SFP_TX_FAULT] =  {"tx_fault", S_IRUGO|S_IWUSR, show_bit, set_1bit},
    [SFP_RX_LOS] =  {"rx_los", S_IRUGO|S_IWUSR, show_bit, set_1bit},
    [QSFP_PRESENT] = {"present", S_IRUGO, show_bit, NULL},
    [QSFP_LP_MODE] = {"low_power_mode", S_IRUGO|S_IWUSR, show_bit, set_1bit},
};

struct attrs as7315_port[NUM_SFP_SB_ATTR] = {
    {SFP_1ST_PRST_REG, true, &portly_attrs[SFP_PRESENT]},
    {0},
    {0x12, false, &portly_attrs[SFP_TX_DIS]},
    {0x11, false, &portly_attrs[SFP_TX_FAULT]},
    {SFP_1ST_RXLOS_REG, false, &portly_attrs[SFP_RX_LOS]},
    {QSFP_1ST_PRST_REG, true, &portly_attrs[QSFP_PRESENT]},
    {0x19, false, &portly_attrs[QSFP_LP_MODE]},
};

struct attrs *as7315_cmn_list[] = {
    &common_attrs[CMN_VERSION],
    &common_attrs[CMN_ACCESS],
    &common_attrs[CMN_PRESENT_ALL],
    &common_attrs[CMN_RXLOS_ALL],
    NULL
};

struct attrs *plain_cmn_list[] = {
    &plain_common[CMN_VERSION],
    NULL
};

struct attrs *as7315_qsfp_list[] = {
    &as7315_port[QSFP_PRESENT],
    &as7315_port[QSFP_LP_MODE],
    NULL
};

struct attrs *as7315_sfp_list[] = {
    &as7315_port[SFP_PRESENT],
    &as7315_port[SFP_TX_DIS],
    &as7315_port[SFP_TX_FAULT],
    &as7315_port[SFP_RX_LOS],
    NULL
};

struct model_attrs models_attr[NUM_MODEL] = {
    {.cmn = as7315_cmn_list, .portly=as7315_sfp_list},
    {.cmn = as7315_cmn_list, .portly=as7315_qsfp_list},
    {.cmn = plain_cmn_list,  .portly=NULL},
};

static LIST_HEAD(cpld_client_list);
static struct mutex	 list_lock;
/* Addresses scanned for as7315_i2c_cpld
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

struct chip_desc {
    u8   nchans;
    u8   deselectChan;
};

/* Provide specs for the PCA954x types we know about */
static const struct chip_desc chips[] = {
    [MDL_CPLD_SFP] = {0},
    [MDL_CPLD_QSFP] = {
        .nchans        = 4, /*Fan + 3*LM75*/
        .deselectChan  = 0,
    },
};

static const struct i2c_device_id as7315_cpld_id[] = {
    { "as7315_cpld1", MDL_CPLD_SFP},
    { "as7315_cpld2", MDL_CPLD_QSFP},
    { },
};
MODULE_DEVICE_TABLE(i2c, as7315_cpld_id);




static int get_sfp_spec(int model, u16 *num, u8 *types)
{
    switch (model) {
    case MDL_CPLD_SFP:
        *num = NUM_SFP;
        *types = PT_SFP;
        break;
    case MDL_CPLD_QSFP:
        *num = NUM_QSFP;
        *types = PT_QSFP;
        break;
    default:
        *types = 0;
        *num = 0;
        break;
    }

    return 0;
}

/*num: how many bits can be applied for this read.*/
static int get_present_reg(int model, u8 port, u8 *reg, u8 *num)
{

    switch (model) {
    case MDL_CPLD_SFP:
        if (port < NUM_SFP) {
            *reg = SFP_1ST_PRST_REG + (port/8)*4;
            *num = (NUM_SFP > 8)? 8 : NUM_SFP;
            return 0;
        }
        break;
    case MDL_CPLD_QSFP:
        if (port < NUM_QSFP) {
            *reg = QSFP_1ST_PRST_REG + (port/8);
            *num = (NUM_QSFP > 8)? 8 : NUM_QSFP;
            return 0;
        }
        break;
    default:
        return -EINVAL;
    }
    return -EINVAL;
}

static int get_rxlos_reg(int model, u8 port, u8 *reg, u8 *num)
{

    switch (model) {
    case MDL_CPLD_SFP:
        if (port < NUM_SFP) {
            *reg = SFP_1ST_RXLOS_REG + (port/8)*4;
            *num = (NUM_SFP > 8)? 8 : NUM_SFP;
            return 0;
        }
        break;
    default:
        return -EINVAL;
    }
    return -EINVAL;
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

static ssize_t show_rxlos_all(struct device *dev,
                              struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 i, value, reg, num;
    u64  values;

    i = num = reg =0;
    values = 0;
    mutex_lock(&data->update_lock);
    while (i < data->sfp_num)
    {
        /*No rxlos for QSFP*/
        if (data->model == MDL_CPLD_QSFP) {
            values = 0;
            break;
        }
        get_rxlos_reg(data->model, i, &reg, &num);
        if (!(num > 0))
        {
            value = -EINVAL;
            goto exit;
        }
        value = cpld_read_internal(client, reg);
        if (unlikely(value < 0)) {
            goto exit;
        }
        values |= (value &((1<<(num))-1)) << i;
        i += num;
    }
    mutex_unlock(&data->update_lock);
    values = cpu_to_le64(values);
    return snprintf(buf, sizeof(values)+1, "%llx\n", values);
exit:
    mutex_unlock(&data->update_lock);
    return value;
}


static ssize_t show_presnet_all(struct device *dev,
                                struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 i, value, reg, num;
    u64  values;

    i = num = reg =0;
    values = 0;
    mutex_lock(&data->update_lock);
    while (i < data->sfp_num)
    {
        get_present_reg(data->model, i, &reg, &num);
        if (!(num > 0))
        {
            value = -EINVAL;
            goto exit;
        }
        value = cpld_read_internal(client, reg);
        if (unlikely(value < 0)) {
            goto exit;
        }
        values |= (~value&((1<<(num))-1)) << i;
        i += num;
    }
    mutex_unlock(&data->update_lock);
    values = cpu_to_le64(values);
    return snprintf(buf, sizeof(values)+1, "%llx\n", values);
exit:
    mutex_unlock(&data->update_lock);
    return value;
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

    return snprintf(buf, PAGE_SIZE, "%x\n", !!value);
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

static void accton_i2c_cpld_add_client(struct i2c_client *client)
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

static void accton_i2c_cpld_remove_client(struct i2c_client *client)
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
    u8 reg, mask, invert, reg_start, jump;
    struct attrs *a;
    struct base_attrs *b;

    if (NULL == pa)
        return -EFAULT;

    jump = 0;
    if (data->sfp_types == PT_SFP) {
        jump = 4;   /*SFP sideband registers in set of 4 bytes.*/
    } else if (data->sfp_types == PT_QSFP) {
        jump = 1;
    }

    for (i = 0; pa[i]; i++) {
        a = pa[i];
        reg_start = a->reg;
        invert = a->invert;
        b = a->base;
        if (b == NULL)
            break;

        for (j = 0; j < data->sfp_num; j++)
        {
            snprintf(name, NAME_SIZE, "%s_%d", b->name, j+1);
            reg = reg_start + ((j/8)*jump);
            mask = 1 << ((j)%8);
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
    struct model_attrs *m = data->attrs;

    if (m == NULL)
        return -EINVAL;

    /* Common attributes.*/
    add_attributes_cmn(data, m->cmn);

    /* Port-wise attributes.*/
    add_attributes_portly(data, m->portly);

    return 0;
}

/* Write to mux register. Don't use i2c_transfer()/i2c_smbus_xfer()
   for this as they will try to lock adapter a second time */
static int _cpld_mux_reg_write(struct i2c_adapter *adap,
                               struct i2c_client *client, u8 val)
{
    unsigned long orig_jiffies;
    unsigned short flags;
    union i2c_smbus_data data;
    int try;
    s32 res = -EIO;

    data.byte = val;
    flags = client->flags;
    flags &= ~(I2C_M_TEN | I2C_CLIENT_PEC);

    if (adap->algo->smbus_xfer) {
        /* Retry automatically on arbitration loss */
        orig_jiffies = jiffies;
        for (res = 0, try = 0; try <= adap->retries; try++) {
                        res = adap->algo->smbus_xfer(adap, client->addr, flags,
                                                     I2C_SMBUS_WRITE, MUX_CHANNEL_SELECT_REG,
                                                     I2C_SMBUS_BYTE_DATA, &data);
                        if (res != -EAGAIN)
                            break;
                        if (time_after(jiffies,
                                       orig_jiffies + adap->timeout))
                            break;
                    }
    }

    return res;
}


static int _mux_select_chan(struct i2c_mux_core *muxc,
                            u32 chan)
{
    u8 regval;
    struct i2c_client *client = i2c_mux_priv(muxc);
    int ret = 0;

    regval = (chan+1) << 5;
    ret = _cpld_mux_reg_write(muxc->parent, client, regval);
    if (unlikely(ret < 0)) {
        return ret;
    }
    return ret;
}


static int _prealloc_attrs(struct cpld_data *data)
{
    void *new_attrs = krealloc(data->group.attrs,
                               ATTR_ALLOC_SIZE * sizeof(void *),
                               GFP_KERNEL);
    if (!new_attrs)
        return -ENOMEM;
    data->group.attrs = new_attrs;

    return 0;
}

static int _add_sysfs_attributes(struct i2c_client *client,
                                 struct cpld_data *data)
{
    int status;

    status = add_attributes(client, data);
    if (status) {
        return status;
    }

    /*If there are no attributes, something is wrong.
     * Bail out instead of trying to register nothing.
     */
    if (!data->num_attributes) {
        dev_err(&client->dev, "No attributes found\n");
        return -ENODEV;
    }

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->group);
    if (status) {
        return status;
    }

    return 0;
}


static int _add_mux_channels(struct i2c_client *client,
                             const struct i2c_device_id *id, struct cpld_data *data)
{
    int num, force, class;
    int status;
    struct i2c_mux_core *muxc;
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    int model = id->driver_data;

    data->muxc = i2c_mux_alloc(adap, &client->dev,
                               chips[model].nchans,
                               sizeof(client), 0,
                               _mux_select_chan, NULL);

    if (!data->muxc) {
        return ENOMEM;
    }
    muxc = data->muxc;
    muxc->priv = client;
    for (num = 0; num < chips[model].nchans; num++) {
        force = 0;			  /* dynamic adap number */
        class = 0;			  /* no class by default */
        status = i2c_mux_add_adapter(muxc, force, num, class);
        if (status)
            return status ;
    }
    dev_info(&client->dev,
             "registered %d multiplexed busses for I2C %s\n",
             num,  client->name);

    return 0;
}

static int as7315_i2c_cpld_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    int status;
    struct cpld_data *data = NULL;
    struct device *dev = &client->dev;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;

    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }
    data->model = dev_id->driver_data;
    data->attrs = &models_attr[data->model];
    get_sfp_spec(data->model, &data->sfp_num, &data->sfp_types);
    mutex_init(&data->update_lock);
    data->dev = dev;
    dev_info(dev, "chip found\n");


    status = _prealloc_attrs(data);
    if (status)
        return -ENOMEM;

    status = _add_sysfs_attributes(client, data);
    if (status)
        goto out_kfree;

    status = _add_mux_channels(client, dev_id, data);
    if (status)
        goto exit_rm_sys;

    i2c_set_clientdata(client, data);
    accton_i2c_cpld_add_client(client);
    dev_info(dev, "%s: cpld '%s'\n",
             dev_name(data->dev), client->name);

    return 0;

exit_rm_sys:
    sysfs_remove_group(&client->dev.kobj, &data->group);
out_kfree:
    kfree(data->group.attrs);
    return status;
}


static void as7315_i2c_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    struct i2c_mux_core *muxc = data->muxc;

    sysfs_remove_group(&client->dev.kobj, &data->group);
    kfree(data->group.attrs);
    if(muxc) {
        i2c_mux_del_adapters(muxc);
    }
    accton_i2c_cpld_remove_client(client);
}

int accton_i2c_cpld_read(u8 cpld_addr, u8 reg)
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
EXPORT_SYMBOL(accton_i2c_cpld_read);

int accton_i2c_cpld_write(u8 cpld_addr, u8 reg, u8 value)
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
EXPORT_SYMBOL(accton_i2c_cpld_write);


static struct i2c_driver as7315_i2c_cpld_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name = DRV_NAME,
    },
    .probe		= as7315_i2c_cpld_probe,
    .remove	   	= as7315_i2c_cpld_remove,
    .id_table     = as7315_cpld_id,
    .address_list = normal_i2c,
};


static int __init as7315_i2c_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&as7315_i2c_cpld_driver);
}

static void __exit as7315_i2c_cpld_exit(void)
{
    i2c_del_driver(&as7315_i2c_cpld_driver);
}

module_init(as7315_i2c_cpld_init);
module_exit(as7315_i2c_cpld_exit);

MODULE_AUTHOR("Roy Lee <roy_lee@accton.com.tw>");
MODULE_DESCRIPTION("as7315_i2c_cpld driver");
MODULE_LICENSE("GPL");
