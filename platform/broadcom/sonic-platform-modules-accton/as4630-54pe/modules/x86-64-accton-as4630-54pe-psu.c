/*
 * An hwmon driver for accton as4630_54pe Power Module
 *
 * Copyright (C) 2014 Accton Technology Corporation.
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
#include <linux/dmi.h>

#define MAX_MODEL_NAME          20
#define MAX_SERIAL_NUMBER       19
#define FAN_DIR_LEN 4

const char FAN_DIR_F2B[] = "F2B";
const char FAN_DIR_B2F[] = "B2F";

static int models_min_offset = 0;
static ssize_t show_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_string(struct device *dev, struct device_attribute *da, char *buf);
static int as4630_54pe_psu_read_block(struct i2c_client *client, u8 command, u8 *data,int data_len);
extern int as4630_54pe_cpld_read(unsigned short cpld_addr, u8 reg);

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { 0x50, 0x51, I2C_CLIENT_END };

/* Each client has this additional data
 */
struct as4630_54pe_psu_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8  index;           /* PSU index */
    u8  status;          /* Status(present/power_good) register read from CPLD */
    char model_name[MAX_MODEL_NAME]; /* Model name, read from eeprom */
    char serial_number[MAX_SERIAL_NUMBER];
    char fan_dir[FAN_DIR_LEN];
};

static struct as4630_54pe_psu_data *as4630_54pe_psu_update_device(struct device *dev);

enum as4630_54pe_psu_sysfs_attributes {
    PSU_PRESENT,
    PSU_MODEL_NAME,
    PSU_POWER_GOOD,
    PSU_SERIAL_NUMBER,
    PSU_FAN_DIR
};

enum psu_type {
    PSU_YPEB1200,          /* F2B */
    PSU_YPEB1200AM,        /* F2B */
    PSU_UP1K21R_1085G,     /* F2B */
    UNKNOWN_PSU
};

struct model_name_info {
    enum psu_type type;
    u8 offset;
    u8 length;
    char* model_name;
};

struct model_name_info models[] = {
{PSU_YPEB1200,   0x20, 11, "YPEB1200"},
{PSU_YPEB1200AM,   0x20, 11, "YPEB1200AM"}, /* Replace YPEB1200-AM to YPEB1200AM */
{PSU_UP1K21R_1085G,   0x20, 13, "UP1K21R-1085G"},
};

struct serial_number_info {
    u8 offset;
    u8 length;
};

struct serial_number_info serials[] = {
    [PSU_YPEB1200] = {0x35, 17},
    [PSU_YPEB1200AM] = {0x35, 18},
    [PSU_UP1K21R_1085G] =  {0x3B, 9},
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(psu_present,    S_IRUGO, show_status,    NULL, PSU_PRESENT);
static SENSOR_DEVICE_ATTR(psu_model_name, S_IRUGO, show_string,    NULL, PSU_MODEL_NAME);
static SENSOR_DEVICE_ATTR(psu_power_good, S_IRUGO, show_status,    NULL, PSU_POWER_GOOD);
static SENSOR_DEVICE_ATTR(psu_serial_number, S_IRUGO, show_string, NULL, PSU_SERIAL_NUMBER);
static SENSOR_DEVICE_ATTR(psu_fan_dir,    S_IRUGO, show_string,    NULL, PSU_FAN_DIR);

static struct attribute *as4630_54pe_psu_attributes[] = {
    &sensor_dev_attr_psu_present.dev_attr.attr,
    &sensor_dev_attr_psu_model_name.dev_attr.attr,
    &sensor_dev_attr_psu_power_good.dev_attr.attr,
    &sensor_dev_attr_psu_serial_number.dev_attr.attr,
    &sensor_dev_attr_psu_fan_dir.dev_attr.attr,
    NULL
};

static ssize_t show_status(struct device *dev, struct device_attribute *da,
                           char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct as4630_54pe_psu_data *data = as4630_54pe_psu_update_device(dev);
    u8 status = 0;

    if (attr->index == PSU_PRESENT) {
        if(data->index==0)
            status = !( (data->status >> 5) & 0x1);
        else
            status = !( (data->status >> 1) & 0x1);
    }
    else { /* PSU_POWER_GOOD */
        if(data->index==0)
           status = ( (data->status >> 6) & 0x1);
        else
           status = ( (data->status >> 2) & 0x1);
    }

    return sprintf(buf, "%d\n", status);
}

static ssize_t show_string(struct device *dev, struct device_attribute *da,
                               char *buf)
{
   struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct as4630_54pe_psu_data *data = as4630_54pe_psu_update_device(dev);
    char *ptr = NULL;

    if (!data->valid) {
        return -EIO;
    }

	switch (attr->index) {
	case PSU_MODEL_NAME:
		ptr = data->model_name;
		break;
	case PSU_SERIAL_NUMBER:
		ptr = data->serial_number;
		break;
	case PSU_FAN_DIR:
		ptr = data->fan_dir;
		break;
	default:
		return -EINVAL;
	}

    return sprintf(buf, "%s\n", ptr);
}

static const struct attribute_group as4630_54pe_psu_group = {
    .attrs = as4630_54pe_psu_attributes,
};

static int find_models_min_offset(void) {
    int i, min_offset = models[0].offset;

    for(i = 1; i < (sizeof(models) / sizeof(models[0])); i++) {
        if(models[i].offset < min_offset) {
            min_offset = models[i].offset;
        }
    }

    return min_offset;
}

static int as4630_54pe_psu_probe(struct i2c_client *client,
                                const struct i2c_device_id *dev_id)
{
    struct as4630_54pe_psu_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct as4630_54pe_psu_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;
    data->index = dev_id->driver_data;
    mutex_init(&data->update_lock);
    models_min_offset = find_models_min_offset();

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as4630_54pe_psu_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: psu '%s'\n",
             dev_name(data->hwmon_dev), client->name);

    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &as4630_54pe_psu_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void as4630_54pe_psu_remove(struct i2c_client *client)
{
    struct as4630_54pe_psu_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as4630_54pe_psu_group);
    kfree(data);
}

enum psu_index
{
    as4630_54pe_psu1,
    as4630_54pe_psu2
};

static const struct i2c_device_id as4630_54pe_psu_id[] = {
    { "as4630_54pe_psu1", as4630_54pe_psu1 },
    { "as4630_54pe_psu2", as4630_54pe_psu2 },
    {}
};
MODULE_DEVICE_TABLE(i2c, as4630_54pe_psu_id);

static struct i2c_driver as4630_54pe_psu_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as4630_54pe_psu",
    },
    .probe        = as4630_54pe_psu_probe,
    .remove       = as4630_54pe_psu_remove,
    .id_table     = as4630_54pe_psu_id,
    .address_list = normal_i2c,
};

static int as4630_54pe_psu_read_block(struct i2c_client *client, u8 command, u8 *data,
                                     int data_len)
{
    int result = 0;
    int retry_count = 5;

    while (retry_count) {
        retry_count--;

        result = i2c_smbus_read_i2c_block_data(client, command, data_len, data);

        if (unlikely(result < 0)) {
            msleep(10);
            continue;
        }

        if (unlikely(result != data_len)) {
            result = -EIO;
            msleep(10);
            continue;
        }

        result = 0;
        break;
    }

    return result;
}

static struct as4630_54pe_psu_data *as4630_54pe_psu_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as4630_54pe_psu_data *data = i2c_get_clientdata(client);
    char temp_model_name[MAX_MODEL_NAME] = {0};

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
            || !data->valid) {
        int status;
        int i, power_good = 0;

        dev_dbg(&client->dev, "Starting as4630_54pe update\n");

        /* Read psu status */
        status = as4630_54pe_cpld_read(0x60, 0x22);
        if (status < 0) {
            dev_dbg(&client->dev, "cpld reg 0x60 err %d\n", status);
        }
        else {
            data->status = status;
        }

        /* Read model name */
        memset(data->model_name, 0, sizeof(data->model_name));
        memset(data->serial_number, 0, sizeof(data->serial_number));
        memset(data->fan_dir, 0, sizeof(data->fan_dir));
        power_good = (data->status >> (data->index==0? 6:2)) & 0x1;

        if (power_good) {
            enum psu_type type = UNKNOWN_PSU;

            status = as4630_54pe_psu_read_block(client, models_min_offset,
                                               temp_model_name,
                                               ARRAY_SIZE(temp_model_name));
            if (status < 0) {
                dev_dbg(&client->dev, "unable to read model name from (0x%x)\n", client->addr);
                goto exit;
            }

            for (i = 0; i < ARRAY_SIZE(models); i++) {
                if ((models[i].length+1) > ARRAY_SIZE(data->model_name)) {
                    dev_dbg(&client->dev,
                            "invalid models[%d].length(%d), should not exceed the size of data->model_name(%ld)\n",
                            i, models[i].length, ARRAY_SIZE(data->model_name));
                    continue;
                }

                snprintf(data->model_name, models[i].length + 1, "%s",
                         temp_model_name + (models[i].offset - models_min_offset));

                if(!strncmp(data->model_name, "YPEB1200", strlen("YPEB1200")))
                {
                        if (data->model_name[9]=='A' && data->model_name[10]=='M')
                        {
                           data->model_name[8]='A';
                           data->model_name[9]='M';
                           data->model_name[strlen("YPEB1200AM")]='\0';
                        }
                        else
                           data->model_name[strlen("YPEB1200")]='\0';
                }

                /* Determine if the model name is known, if not, read next index */
                if (strcmp(data->model_name, models[i].model_name) == 0) {
                    type = models[i].type;
                    break;
                }

                data->model_name[0] = '\0';
            }

            switch (type) {
            case PSU_YPEB1200:
            case PSU_YPEB1200AM:
            case PSU_UP1K21R_1085G:
                memcpy(data->fan_dir, FAN_DIR_F2B, sizeof(FAN_DIR_F2B));
                break;
            default:
                dev_dbg(&client->dev, "Unknown PSU type for fan direction\n");
                break;
            }

            if (type < ARRAY_SIZE(serials)) {
                if ((serials[type].length+1) > ARRAY_SIZE(data->serial_number)) {
                    dev_dbg(&client->dev,
                            "invalid serials[%d].length(%d), should not exceed the size of data->serial_number(%ld)\n",
                            type, serials[type].length, ARRAY_SIZE(data->serial_number));
                    goto exit;
                }

                memset(data->serial_number, 0, sizeof(data->serial_number));
                status = as4630_54pe_psu_read_block(client, serials[type].offset,
                                                data->serial_number,
                                                serials[type].length);
                if (status < 0) {
                    dev_dbg(&client->dev,
                            "unable to read serial from (0x%x) offset(0x%02x)\n",
                            client->addr, serials[type].length);
                    goto exit;
                }
                else {
                    data->serial_number[serials[type].length]= '\0';
                }
            }
            else {
                dev_dbg(&client->dev, "invalid PSU type(%d)\n", type);
                goto exit;
            }
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

exit:
    mutex_unlock(&data->update_lock);

    return data;
}

module_i2c_driver(as4630_54pe_psu_driver);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as4630_54pe_psu driver");
MODULE_LICENSE("GPL");

