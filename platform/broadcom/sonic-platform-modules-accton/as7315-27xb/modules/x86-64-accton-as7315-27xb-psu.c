/*
 * An hwmon driver for accton as7315_27xb Power Module
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
#include <linux/printk.h>

#define DRV_NAME        "as7315_27xb_psu"
#define PSU_STATUS_I2C_ADDR			0x64
#define PSU_STATUS_I2C_REG_OFFSET	0x2
#define USE_BYTE_ACCESS     0       /*Somehow i2c block access is failed on this platform.*/
#define UPDATE_PERIOD			    (HZ*2)
#define MAX_OUTPUT_LENGTH   		32
#define BASIC_EEPROM_SIZE   		32

#define IS_POWER_GOOD(id, value)	(!!(value & BIT(id + 2)))
#define IS_PRESENT(id, value)		(!(value & BIT(id)))

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data
 */
struct as7315_27xb_psu_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8  index;           /* PSU index */
    u8  status;          /* Status(present/power_good) register read from CPLD */
    char eeprom[BASIC_EEPROM_SIZE*2];    /* EEPROM*/
};


enum as7315_27xb_psu_sysfs_attributes {
    PSU_INDEX,
    PSU_PRESENT,
    PSU_MODEL_NAME,
    PSU_POWER_GOOD,
    PSU_SERIAL_NUMBER
};

enum psu_type {
    PSU_YM_1401_A,      /* AC110V - B2F */
    PSU_YM_2401_JCR,    /* AC110V - F2B */
    PSU_YM_2401_JDR,    /* AC110V - B2F */
    PSU_YM_2401_TCR,    /* AC110V - B2F */
    PSU_CPR_4011_4M11,  /* AC110V - F2B */
    PSU_CPR_4011_4M21,  /* AC110V - B2F */
    PSU_CPR_6011_2M11,  /* AC110V - F2B */
    PSU_CPR_6011_2M21,  /* AC110V - B2F */
    PSU_UM400D_01G,     /* DC48V  - F2B */
    PSU_UM400D01_01G,   /* DC48V  - B2F */
    PSU_BEL_TOT120,     /* DC48V  - N/A */
    PSU_TYPE_MAX
};

struct model_info {
    enum psu_type type;
    u8 offset;
    char* model_name;
    u8 serial_offset;
};

struct model_info models[] = {
    {PSU_YM_1401_A,     0x20, "YM-1401ACR",0x35},
    {PSU_YM_2401_JCR,   0x20, "YM-2401JCR",0x35},
    {PSU_YM_2401_JDR,   0x20, "YM-2401JDR",0x35},
    {PSU_YM_2401_TCR,   0x20, "YM-2401TCR",0x35},
    {PSU_CPR_4011_4M11, 0x26, "CPR-4011-4M11",0x47},
    {PSU_CPR_4011_4M21, 0x26, "CPR-4011-4M21",0x47},
    {PSU_CPR_6011_2M11, 0x26, "CPR-6011-2M11",0x46},
    {PSU_CPR_6011_2M21, 0x26, "CPR-6011-2M21",0x46},
    {PSU_UM400D_01G,    0x50, "um400d01G",0x50},
    {PSU_UM400D01_01G,  0x50, "um400d01-01G",0x50},
    {PSU_BEL_TOT120,    0x0A, "CRXT-T0T120",0x18},
};

static ssize_t show_index(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_model_name(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_serial_number(struct device *dev, struct device_attribute *da,char *buf);
static int as7315_27xb_psu_block_read(struct i2c_client *client, u8 command, u8 *data,int data_len);
static int as7315_27xb_psu_model_name_get(
    struct device *dev, char *buf);
static int as7315_27xb_psu_serial_number_get(
    struct device *dev, enum psu_type type, char *out);
static struct as7315_27xb_psu_data *as7315_27xb_psu_update_device(struct device *dev);
extern int accton_i2c_cpld_read(u8 cpld_addr, u8 reg);

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(psu_index,      S_IRUGO, show_index,     NULL, PSU_INDEX);
static SENSOR_DEVICE_ATTR(psu_present,    S_IRUGO, show_status,    NULL, PSU_PRESENT);
static SENSOR_DEVICE_ATTR(psu_model_name, S_IRUGO, show_model_name,NULL, PSU_MODEL_NAME);
static SENSOR_DEVICE_ATTR(psu_serial, S_IRUGO, show_serial_number, NULL, PSU_SERIAL_NUMBER);
static SENSOR_DEVICE_ATTR(psu_power_good, S_IRUGO, show_status,    NULL, PSU_POWER_GOOD);

static struct attribute *as7315_27xb_psu_attributes[] = {
    &sensor_dev_attr_psu_index.dev_attr.attr,
    &sensor_dev_attr_psu_present.dev_attr.attr,
    &sensor_dev_attr_psu_model_name.dev_attr.attr,
    &sensor_dev_attr_psu_serial.dev_attr.attr,
    &sensor_dev_attr_psu_power_good.dev_attr.attr,
    NULL
};

static ssize_t show_index(struct device *dev, struct device_attribute *da,
                          char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as7315_27xb_psu_data *data = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", data->index);
}

static ssize_t show_status(struct device *dev, struct device_attribute *da,
                           char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct as7315_27xb_psu_data *data = as7315_27xb_psu_update_device(dev);
    u8 status = 0;

    if (!data->valid) {
        return sprintf(buf, "0\n");
    }

    if (attr->index == PSU_PRESENT) {
        status = IS_PRESENT(data->index, data->status);
    }
    else { /* PSU_POWER_GOOD */
        status = IS_POWER_GOOD(data->index, data->status);
    }

    return sprintf(buf, "%d\n", status);
}

static ssize_t show_serial_number(struct device *dev, struct device_attribute *da,
                                  char *buf)
{
    int i;
    struct as7315_27xb_psu_data *data = as7315_27xb_psu_update_device(dev);

    if (!data->valid) {
        return 0;
    }

    if (!IS_PRESENT(data->index, data->status)) {
        return 0;
    }

    i = as7315_27xb_psu_model_name_get(dev, buf);
    if ( i < 0) {
        return -ENXIO;
    }

    if (as7315_27xb_psu_serial_number_get(dev, i, buf) < 0) {
        return -ENXIO;
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t show_model_name(struct device *dev, struct device_attribute *da,
                               char *buf)
{
    struct as7315_27xb_psu_data *data = as7315_27xb_psu_update_device(dev);

    if (!data->valid) {
        return 0;
    }

    if (!IS_PRESENT(data->index, data->status)) {
        return 0;
    }

    if (as7315_27xb_psu_model_name_get(dev, buf) < 0) {
        return -ENXIO;
    }

    return sprintf(buf, "%s\n", buf);
}

static const struct attribute_group as7315_27xb_psu_group = {
    .attrs = as7315_27xb_psu_attributes,
};

static int as7315_27xb_psu_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    struct as7315_27xb_psu_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct as7315_27xb_psu_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;
    data->index = dev_id->driver_data;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as7315_27xb_psu_group);
    if (status) {
        goto exit_free;
    }
    data->hwmon_dev = hwmon_device_register_with_info(&client->dev,
                      client->name, NULL, NULL, NULL);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: psu '%s'\n",
             dev_name(data->hwmon_dev), client->name);

    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &as7315_27xb_psu_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void as7315_27xb_psu_remove(struct i2c_client *client)
{
    struct as7315_27xb_psu_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as7315_27xb_psu_group);
    kfree(data);

}

enum psu_index
{
    as7315_27xb_psu1,
    as7315_27xb_psu2
};

static const struct i2c_device_id as7315_27xb_psu_id[] = {
    { "as7315_27xb_psu1", as7315_27xb_psu1 },
    { "as7315_27xb_psu2", as7315_27xb_psu2 },
    {}
};
MODULE_DEVICE_TABLE(i2c, as7315_27xb_psu_id);

static struct i2c_driver as7315_27xb_psu_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = DRV_NAME,
    },
    .probe        = as7315_27xb_psu_probe,
    .remove       = as7315_27xb_psu_remove,
    .id_table     = as7315_27xb_psu_id,
    .address_list = normal_i2c,
};

static int as7315_27xb_psu_block_read(struct i2c_client *client,
                                      u8 command, u8 *data,  int max_len)
{
    int result;

    u8 i, offset;

    if (i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
        for (i = 0; i < max_len; i += 32) {
            offset = i + command ;
            result = i2c_smbus_read_i2c_block_data(client, offset,
                                                   32, data + i);

            if (result != 32) {
                result = -EIO;
                goto abort;
            }
        }

    } else {
        for (i = 0; i < max_len; i += 2) {
            int word;
            offset = i + command ;
            word = i2c_smbus_read_word_data(client, offset);
            if (word < 0) {
                result = -EIO;
                goto abort;
            }
            data[i] = word & 0xff;
            data[i + 1] = word >> 8;
        }
    }
    result = 0;
abort:
    return result;
}

static int as7315_27xb_psu_serial_number_get(
    struct device *dev, enum psu_type type, char *out)
{
    char *serial;
    struct as7315_27xb_psu_data *data = as7315_27xb_psu_update_device(dev);

    if (type >= PSU_TYPE_MAX) {
        return -EINVAL;
    }

    if (!data->valid) {
        out[0] = '\0';
        return 0;
    }

    serial = data->eeprom + models[type].serial_offset;
    strncpy(out, serial, MAX_OUTPUT_LENGTH);
    return 0;
}

static int find_model_name_from_eeprom( char *eeprom)
{
    int i;
    char *name;

    for (i = 0; i < ARRAY_SIZE(models); i++) {
        name = eeprom + models[i].offset;
        if (strncmp(name, models[i].model_name,
                    strlen(models[i].model_name)) == 0) {
            break;
        }
    }

    return (i == ARRAY_SIZE(models))? -EINVAL: i;
}
static int as7315_27xb_psu_model_name_get(
    struct device *dev, char *buf)
{
    int i;
    struct as7315_27xb_psu_data *data = as7315_27xb_psu_update_device(dev);

    if (!data->valid) {
        return sprintf(buf, "0\n");
    }

    /* Determine if the model name is known, if not, read next index
     */
    i = find_model_name_from_eeprom(data->eeprom);
    if (i < 0) {
        return -ENODATA;
    }

    mutex_lock(&data->update_lock);
    strncpy(buf,  models[i].model_name, MAX_OUTPUT_LENGTH);
    /*Work-around for some special models*/
    if (i == PSU_YM_2401_JCR || i == PSU_YM_2401_JDR ||
            i == PSU_YM_1401_A || i == PSU_YM_2401_TCR) {
        /* Skip the meaningless data byte 8*/
        buf[8] = buf[9];
        buf[9] = buf[10];
        buf = '\0';
    }

    mutex_unlock(&data->update_lock);
    return i;
}

static struct as7315_27xb_psu_data *as7315_27xb_psu_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as7315_27xb_psu_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);
    if (time_after(jiffies, data->last_updated + UPDATE_PERIOD)
            || !data->valid) {
        int status = -1;

        dev_dbg(&client->dev, "Starting as7315_27xb update\n");
        data->valid = 0;

        /* Read psu status */
        status = accton_i2c_cpld_read(PSU_STATUS_I2C_ADDR, PSU_STATUS_I2C_REG_OFFSET);
        if (status < 0) {
            dev_dbg(&client->dev,
                    "cpld reg (0x%x) err %d\n", PSU_STATUS_I2C_ADDR, status);
            goto exit;
        }
        else {
            data->status = status;
        }


        /*Read the eeprom of psu*/
        memset(data->eeprom, 0, sizeof(data->eeprom));
        status = as7315_27xb_psu_block_read(client, 0,
                                            data->eeprom, sizeof(data->eeprom));

        if (status < 0) {
            dev_dbg(&client->dev, "unable to read eeprom from (0x%x)\n",
                    client->addr);
            goto exit;
        }
        data->last_updated = jiffies;
        data->valid = 1;
    }

exit:
    mutex_unlock(&data->update_lock);
    return data;
}

module_i2c_driver(as7315_27xb_psu_driver);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("accton as7315_27xb_psu driver");
MODULE_LICENSE("GPL");

