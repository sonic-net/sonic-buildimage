/*
 * rg_cpld.c - A driver for control rg_cpld base on rg_cpld.c
 *
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
 * Copyright (c) 2018 support <support@ragile.com>
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

static const unsigned short rg_i2c_cpld[] = { 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, I2C_CLIENT_END };

#define CPLD_SIZE 256
#define CPLD_I2C_RETRY_TIMES 5
#define CPLD_I2C_RETRY_WAIT_TIME 10

#define COMMON_STR_LEN (256)


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
                
static ssize_t show_fan_rpm_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int index = to_sensor_dev_attr_2(da)->index;
    uint8_t size;
    s32 status;
	s32 ret_t;
    
	ret_t = 0;    
    status = -1;
    size = 0;
    mutex_lock(&data->update_lock);
    status = cpld_i2c_smbus_read_byte_data(client, index);
    if (status < 0) {
        mutex_unlock(&data->update_lock);
        return 0;
    }
    data->data[0] = status;
    status = cpld_i2c_smbus_read_byte_data(client, index + 1);
    if (status < 0) {
        mutex_unlock(&data->update_lock);
        return 0;
    }
    data->data[1] = status;
    DBG_DEBUG("cpld reg pos:0x%x value:0x%x\n",  index, data->data[0]);
    DBG_DEBUG("cpld reg pos:0x%x value:0x%x\n",  index + 1, data->data[1]);
    ret_t = (data->data[1] << 8) + data->data[0] ;
    if (ret_t == 0 ) {
        size = snprintf(buf, CPLD_SIZE, "%d\n", ret_t);
    } else if (ret_t == 0xffff) {
        size = snprintf(buf, CPLD_SIZE, "%d\n", 0);
	} else {
        size = snprintf(buf, CPLD_SIZE, "%d\n", 15000000 / ret_t);
    }
    mutex_unlock(&data->update_lock);
    return size;
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
    return snprintf(buf, COMMON_STR_LEN, "%02x %02x %02x %02x \n", data->data[0], data->data[1], data->data[2], 
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
    return snprintf(buf, COMMON_STR_LEN, "%02x\n", data->data[0]);
}

/* sys */
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, show_cpld_version, NULL, 0);
static SENSOR_DEVICE_ATTR(broad_back_sys, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x72);
static SENSOR_DEVICE_ATTR(broad_front_pwr, S_IRUGO| S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x73);
static SENSOR_DEVICE_ATTR(broad_front_fan, S_IRUGO| S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x74);


/* fan */
static SENSOR_DEVICE_ATTR(fan_present, S_IRUGO, show_cpld_sysfs_value, NULL, 0x30);
static SENSOR_DEVICE_ATTR(fan_status, S_IRUGO, show_cpld_sysfs_value, NULL, 0x31);

static SENSOR_DEVICE_ATTR(fan1_speed_set, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x14);
static SENSOR_DEVICE_ATTR(fan1_1_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x1B);
static SENSOR_DEVICE_ATTR(fan1_2_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x25);
static SENSOR_DEVICE_ATTR(fan1_led, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x3b);


static SENSOR_DEVICE_ATTR(fan2_speed_set, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x15);
static SENSOR_DEVICE_ATTR(fan2_1_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x1D);
static SENSOR_DEVICE_ATTR(fan2_2_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x27);
static SENSOR_DEVICE_ATTR(fan2_led, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x3c);


static SENSOR_DEVICE_ATTR(fan3_speed_set, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x16);
static SENSOR_DEVICE_ATTR(fan3_1_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x1F);
static SENSOR_DEVICE_ATTR(fan3_2_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x29);
static SENSOR_DEVICE_ATTR(fan3_led, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x3d);


static SENSOR_DEVICE_ATTR(fan4_speed_set, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x17);
static SENSOR_DEVICE_ATTR(fan4_1_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x21);
static SENSOR_DEVICE_ATTR(fan4_2_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x2b);
static SENSOR_DEVICE_ATTR(fan4_led, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x3e);

static SENSOR_DEVICE_ATTR(fan5_speed_set, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x18);
static SENSOR_DEVICE_ATTR(fan5_1_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x23);
static SENSOR_DEVICE_ATTR(fan5_2_real_speed, S_IRUGO, show_fan_rpm_value, NULL, 0x2d);
static SENSOR_DEVICE_ATTR(fan5_led, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x3f);

/* sfp */
static SENSOR_DEVICE_ATTR(sfp_enable, S_IRUGO| S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 
0x94);
static SENSOR_DEVICE_ATTR(sfp_presence1, S_IRUGO, show_cpld_sysfs_value, NULL, 0x10);
static SENSOR_DEVICE_ATTR(sfp_presence2, S_IRUGO, show_cpld_sysfs_value, NULL, 0x11);
static SENSOR_DEVICE_ATTR(sfp_presence3, S_IRUGO, show_cpld_sysfs_value, NULL, 0x10);
static SENSOR_DEVICE_ATTR(sfp_presence4, S_IRUGO, show_cpld_sysfs_value, NULL, 0x11);
static SENSOR_DEVICE_ATTR(sfp_led1, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x18);
static SENSOR_DEVICE_ATTR(sfp_led2, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x19);
static SENSOR_DEVICE_ATTR(sfp_led3, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x18);
static SENSOR_DEVICE_ATTR(sfp_led4, S_IRUGO | S_IWUSR ,show_cpld_sysfs_value, set_cpld_sysfs_value, 0x19);
static SENSOR_DEVICE_ATTR(sfp_reset1, S_IRUGO | S_IWUSR ,show_cpld_sysfs_value, set_cpld_sysfs_value, 0x14);
static SENSOR_DEVICE_ATTR(sfp_reset2, S_IRUGO | S_IWUSR ,show_cpld_sysfs_value, set_cpld_sysfs_value, 0x15);
static SENSOR_DEVICE_ATTR(sfp_reset3, S_IRUGO | S_IWUSR ,show_cpld_sysfs_value, set_cpld_sysfs_value, 0x14);
static SENSOR_DEVICE_ATTR(sfp_reset4, S_IRUGO | S_IWUSR ,show_cpld_sysfs_value, set_cpld_sysfs_value, 0x15);

static struct attribute *cpld_bus2_addr_0x0d_sysfs_attrs[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_fan_present.dev_attr.attr,
    &sensor_dev_attr_fan_status.dev_attr.attr,
        
    &sensor_dev_attr_fan1_speed_set.dev_attr.attr,
    &sensor_dev_attr_fan1_1_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan1_2_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan1_led.dev_attr.attr,
    
    &sensor_dev_attr_fan2_speed_set.dev_attr.attr,
    &sensor_dev_attr_fan2_1_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan2_2_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan2_led.dev_attr.attr,
    
    &sensor_dev_attr_fan3_speed_set.dev_attr.attr,
    &sensor_dev_attr_fan3_1_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan3_2_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan3_led.dev_attr.attr,
    
    &sensor_dev_attr_fan4_speed_set.dev_attr.attr,
    &sensor_dev_attr_fan4_1_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan4_2_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan4_led.dev_attr.attr,
    
    &sensor_dev_attr_fan5_speed_set.dev_attr.attr,
    &sensor_dev_attr_fan5_1_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan5_2_real_speed.dev_attr.attr,
    &sensor_dev_attr_fan5_led.dev_attr.attr,
    NULL
};

static struct attribute *cpld_bus6_addr_0x0d_sysfs_attrs[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_broad_back_sys.dev_attr.attr,
    &sensor_dev_attr_broad_front_pwr.dev_attr.attr,    
    &sensor_dev_attr_broad_front_fan.dev_attr.attr,
    &sensor_dev_attr_sfp_enable.dev_attr.attr,
    NULL
};

static struct attribute *cpld_bus8_addr_0x30_sysfs_attrs[] = {
    &sensor_dev_attr_sfp_presence1.dev_attr.attr,
    &sensor_dev_attr_sfp_presence2.dev_attr.attr,
    &sensor_dev_attr_sfp_led1.dev_attr.attr,
    &sensor_dev_attr_sfp_led2.dev_attr.attr,
    &sensor_dev_attr_sfp_reset1.dev_attr.attr,
    &sensor_dev_attr_sfp_reset2.dev_attr.attr,
    NULL
};

static struct attribute *cpld_bus8_addr_0x31_sysfs_attrs[] = {
    &sensor_dev_attr_sfp_presence3.dev_attr.attr,
    &sensor_dev_attr_sfp_presence4.dev_attr.attr,
    &sensor_dev_attr_sfp_led3.dev_attr.attr,
    &sensor_dev_attr_sfp_led4.dev_attr.attr,
    &sensor_dev_attr_sfp_reset3.dev_attr.attr,
    &sensor_dev_attr_sfp_reset4.dev_attr.attr,
    NULL
};

static const struct attribute_group  cpld_bus2_addr_0x0d_sysfs_group = {
    .attrs = cpld_bus2_addr_0x0d_sysfs_attrs,
};

static const struct attribute_group  cpld_bus6_addr_0x0d_sysfs_group = {
    .attrs = cpld_bus6_addr_0x0d_sysfs_attrs,
};

static const struct attribute_group  cpld_bus8_addr_0x30_sysfs_group = {
    .attrs = cpld_bus8_addr_0x30_sysfs_attrs,
};

static const struct attribute_group  cpld_bus8_addr_0x31_sysfs_group = {
    .attrs = cpld_bus8_addr_0x31_sysfs_attrs,
};

struct cpld_attr_match_group {
    int bus_nr;
    unsigned short addr;
    const struct attribute_group   *attr_group_ptr;
    const struct attribute_group   *attr_hwmon_ptr;
};

static struct cpld_attr_match_group g_cpld_attr_match[] = {
    {2, 0x0d, &cpld_bus2_addr_0x0d_sysfs_group, NULL},
    /* {6, 0x0d, &cpld_bus6_addr_0x0d_sysfs_group, NULL}, */
    {8, 0x30, &cpld_bus8_addr_0x30_sysfs_group, NULL},
    {8, 0x31, &cpld_bus8_addr_0x31_sysfs_group, NULL},
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

#if 0
static int cpld_detect(struct i2c_client *new_client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = new_client->adapter;
    int conf;
    DBG_DEBUG("=========cpld_detect(0x%x)===========\n", new_client->addr);
    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA |
                     I2C_FUNC_SMBUS_WORD_DATA))
        return -ENODEV;
    conf = i2c_smbus_read_byte_data(new_client, 0);
    if (!conf)
        return -ENODEV;
	strlcpy(info->type, "rg_cpld", I2C_NAME_SIZE);
    return 0;
}
#endif

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
    { "rg_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver rg_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name   = "rg_cpld",
    },
    .probe      = cpld_probe,
    .remove     = cpld_remove,
    .id_table   = cpld_id,
    //.detect     = cpld_detect,
   // .address_list   = rg_i2c_cpld,
};

module_i2c_driver(rg_cpld_driver);
MODULE_AUTHOR("support <support@ragile.com>");
MODULE_DESCRIPTION("ragile CPLD driver");
MODULE_LICENSE("GPL");
