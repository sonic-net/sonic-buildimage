/*
 * An hwmon driver for the 3Y Power YESM1300AM Power Module
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

/* PMBus Protocol. */
#define PSU_REG_VOUT_MODE                0x20
#define PSU_REG_READ_VIN                 0x88
#define PSU_REG_READ_IIN                 0x89
#define PSU_REG_READ_VOUT                0x8B
#define PSU_REG_READ_IOUT                0x8C
#define PSU_REG_READ_TEMPERATURE_1       0x8D
#define PSU_REG_READ_FAN_SPEED_1         0x90
#define PSU_REG_READ_POUT                0x96
#define PSU_REG_READ_PIN                 0x97
#define PSU_REG_MFR_ID                   0x99
#define PSU_REG_MFR_MODEL                0x9A
#define PSU_REG_MFR_POUT_MAX             0xA7

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = {0x58, 0x59, I2C_CLIENT_END};

/* Each client has this additional data
 */
struct yesm1300am_data
{
    struct device *hwmon_dev;
    struct mutex update_lock;
    char valid;                 /* !=0 if registers are valid */
    unsigned long last_updated; /* In jiffies */
    u8 vout_mode;               /* Register value */
    u16 v_in;                   /* Register value */
    u16 v_out;                  /* Register value */
    u16 i_in;                   /* Register value */
    u16 i_out;                  /* Register value */
    u16 p_in;                   /* Register value */
    u16 p_out;                  /* Register value */
    u16 temp1_input;            /* Register value */
    u16 fan_speed;              /* Register value */
    u8 mfr_id[10];              /* Register value */
    u8 mfr_model[12];           /* Register value */
    u16 mfr_pout_max;           /* Register value */
};

static ssize_t show_vout_by_mode(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_linear(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_ascii(struct device *dev, struct device_attribute *da, char *buf);
static struct yesm1300am_data *yesm1300am_update_device(struct device *dev);

enum yesm1300am_sysfs_attributes
{
    PSU_V_IN,
    PSU_V_OUT,
    PSU_I_IN,
    PSU_I_OUT,
    PSU_P_IN,
    PSU_P_OUT_UW,
    PSU_P_OUT,
    PSU_TEMP1_INPUT,
    PSU_FAN1_SPEED,
	PSU_MFR_ID,
	PSU_MODEL_NAME,
	PSU_MFR_POUT_MAX
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(psu_v_in, S_IRUGO, show_linear, NULL, PSU_V_IN);
static SENSOR_DEVICE_ATTR(psu_v_out, S_IRUGO, show_vout_by_mode, NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(psu_i_in, S_IRUGO, show_linear, NULL, PSU_I_IN);
static SENSOR_DEVICE_ATTR(psu_i_out, S_IRUGO, show_linear, NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(psu_p_in, S_IRUGO, show_linear, NULL, PSU_P_IN);
static SENSOR_DEVICE_ATTR(psu_p_out, S_IRUGO, show_linear, NULL, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(psu_temp1_input, S_IRUGO, show_linear, NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(psu_fan1_speed_rpm, S_IRUGO, show_linear, NULL, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(psu_mfr_id, S_IRUGO, show_ascii, NULL, PSU_MFR_ID);
static SENSOR_DEVICE_ATTR(psu_model_name, S_IRUGO, show_ascii, NULL, PSU_MODEL_NAME);
static SENSOR_DEVICE_ATTR(psu_mfr_pout_max, S_IRUGO, show_linear, NULL, PSU_MFR_POUT_MAX);

/*Duplicate nodes for lm-sensors.*/
static SENSOR_DEVICE_ATTR(in3_input, S_IRUGO, show_vout_by_mode, NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(curr2_input, S_IRUGO, show_linear, NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, show_linear, NULL, PSU_P_OUT_UW);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_linear, NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_linear, NULL, PSU_FAN1_SPEED);

static struct attribute *yesm1300am_attributes[] = {
    &sensor_dev_attr_psu_v_in.dev_attr.attr,
    &sensor_dev_attr_psu_v_out.dev_attr.attr,
    &sensor_dev_attr_psu_i_in.dev_attr.attr,
    &sensor_dev_attr_psu_i_out.dev_attr.attr,
    &sensor_dev_attr_psu_p_in.dev_attr.attr,
    &sensor_dev_attr_psu_p_out.dev_attr.attr,
    &sensor_dev_attr_psu_temp1_input.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_id.dev_attr.attr,
    &sensor_dev_attr_psu_model_name.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_pout_max.dev_attr.attr,
    /*Duplicate nodes for lm-sensors.*/
    &sensor_dev_attr_curr2_input.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_power2_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    NULL};

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16 valid_data = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static ssize_t show_linear(struct device *dev, struct device_attribute *da,
                           char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct yesm1300am_data *data = yesm1300am_update_device(dev);

    u16 value = 0;
    int exponent, mantissa;
    int multiplier = 1000;

    switch (attr->index)
    {
    case PSU_V_IN:
        value = data->v_in;
        break;
    case PSU_I_IN:
        value = data->i_in;
        break;
    case PSU_I_OUT:
        value = data->i_out;
        break;
    case PSU_P_IN:
        value = data->p_in;
        break;
    case PSU_P_OUT:
        value = data->p_out;
        break;
    case PSU_P_OUT_UW:
        value = data->p_out;
        multiplier = 1000000;
        break;
    case PSU_TEMP1_INPUT:
        value = data->temp1_input;
        break;
    case PSU_FAN1_SPEED:
        value = data->fan_speed;
        multiplier = 1;
        break;
    case PSU_MFR_POUT_MAX:
        value = data->mfr_pout_max;
        break;
    }

    exponent = two_complement_to_int(value >> 11, 5, 0x1f);
    mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);
    return (exponent >= 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * multiplier) : sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static ssize_t show_ascii(struct device *dev, struct device_attribute *da,
                          char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct yesm1300am_data *data = yesm1300am_update_device(dev);
    u8 *ptr = NULL;

    switch (attr->index)
    {
    case PSU_MFR_ID: /* psu_mfr_id */
        ptr = data->mfr_id;
        break;
    case PSU_MODEL_NAME: /* psu_mfr_model */
        ptr = data->mfr_model;
        break;
    default:
        return 0;
    }

    return sprintf(buf, "%s\n", ptr);
}

static ssize_t show_vout_by_mode(struct device *dev, struct device_attribute *da,
                                 char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct yesm1300am_data *data = yesm1300am_update_device(dev);
    int exponent, mantissa;
    int multiplier = 1000;

    if (!data->valid)
    {
        return 0;
    }

    exponent = two_complement_to_int(data->vout_mode, 5, 0x1f);
    switch (attr->index)
    {
    case PSU_V_OUT:
        mantissa = data->v_out;
        break;
    default:
        return 0;
    }

    return (exponent > 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * multiplier) : sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static const struct attribute_group yesm1300am_group = {
    .attrs = yesm1300am_attributes,
};

static int yesm1300am_probe(struct i2c_client *client,
                         const struct i2c_device_id *dev_id)
{
    struct yesm1300am_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter,
                                 I2C_FUNC_SMBUS_BYTE_DATA |
                                 I2C_FUNC_SMBUS_WORD_DATA |
                                 I2C_FUNC_SMBUS_I2C_BLOCK))
    {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct yesm1300am_data), GFP_KERNEL);
    if (!data)
    {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &yesm1300am_group);
    if (status)
    {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev))
    {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: psu '%s'\n",
             dev_name(data->hwmon_dev), client->name);

    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &yesm1300am_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static int yesm1300am_remove(struct i2c_client *client)
{
    struct yesm1300am_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &yesm1300am_group);
    kfree(data);

    return 0;
}

static const struct i2c_device_id yesm1300am_id[] = {
    {"yesm1300am", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, yesm1300am_id);

static struct i2c_driver yesm1300am_driver = {
    .class = I2C_CLASS_HWMON,
    .driver = {
        .name = "yesm1300am",
    },
    .probe = yesm1300am_probe,
    .remove = yesm1300am_remove,
    .id_table = yesm1300am_id,
    .address_list = normal_i2c,
};

static int yesm1300am_read_byte(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}

static int yesm1300am_read_word(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_word_data(client, reg);
}

static int yesm1300am_read_block(struct i2c_client *client, u8 command, u8 *data,
                              int data_len)
{
    int result = i2c_smbus_read_i2c_block_data(client, command, data_len, data);

    if (unlikely(result < 0))
        goto abort;
    if (unlikely(result != data_len))
    {
        result = -EIO;
        goto abort;
    }

    result = 0;

abort:
    return result;
}

struct reg_data_byte
{
    u8 reg;
    u8 *value;
};

struct reg_data_word
{
    u8 reg;
    u16 *value;
};

static struct yesm1300am_data *yesm1300am_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct yesm1300am_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) || !data->valid)
    {
        int i, status;
        u8 command;
        struct reg_data_byte regs_byte[] = {{PSU_REG_VOUT_MODE, &data->vout_mode}};
        struct reg_data_word regs_word[] = {{PSU_REG_READ_VIN, &data->v_in},
                                            {PSU_REG_READ_VOUT, &data->v_out},
                                            {PSU_REG_READ_IIN, &data->i_in},
                                            {PSU_REG_READ_IOUT, &data->i_out},
                                            {PSU_REG_READ_PIN, &data->p_in},
                                            {PSU_REG_READ_POUT, &data->p_out},
                                            {PSU_REG_READ_TEMPERATURE_1, &data->temp1_input},
                                            {PSU_REG_READ_FAN_SPEED_1, &data->fan_speed},
                                            {PSU_REG_MFR_POUT_MAX, &data->mfr_pout_max}};

        dev_dbg(&client->dev, "Starting yesm1300am update\n");

        /* Read byte data */
        for (i = 0; i < ARRAY_SIZE(regs_byte); i++)
        {
            status = yesm1300am_read_byte(client, regs_byte[i].reg);

            if (status < 0)
            {
                dev_dbg(&client->dev, "reg %d, err %d\n",
                        regs_byte[i].reg, status);
                *(regs_byte[i].value) = 0;
            }
            else
            {
                *(regs_byte[i].value) = status;
            }
        }

        /* Read word data */
        for (i = 0; i < ARRAY_SIZE(regs_word); i++)
        {
            status = yesm1300am_read_word(client, regs_word[i].reg);

            if (status < 0)
            {
                dev_dbg(&client->dev, "reg %d, err %d\n",
                        regs_word[i].reg, status);
                *(regs_word[i].value) = 0;
            }
            else
            {
                *(regs_word[i].value) = status;
            }
        }

        /* Read mfr_id */
        command = PSU_REG_MFR_ID;
        status = yesm1300am_read_block(client, command, data->mfr_id,
                                    ARRAY_SIZE(data->mfr_id) - 1);
        data->mfr_id[ARRAY_SIZE(data->mfr_id) - 1] = '\0';
        strncpy(data->mfr_id, (u8 *)&data->mfr_id + 1, ARRAY_SIZE(data->mfr_id) - 1);

        if (status < 0)
            dev_dbg(&client->dev, "reg %d, err %d\n", command, status);

        /* Read mfr_model */
        command = PSU_REG_MFR_MODEL;
        status = yesm1300am_read_block(client, command, data->mfr_model,
                                    ARRAY_SIZE(data->mfr_model) - 1);
        data->mfr_model[ARRAY_SIZE(data->mfr_model) - 1] = '\0';
        strncpy(data->mfr_model, (u8 *)&data->mfr_model + 1, ARRAY_SIZE(data->mfr_model) - 1);

        if (status < 0)
            dev_dbg(&client->dev, "reg %d, err %d\n", command, status);

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return data;
}

module_i2c_driver(yesm1300am_driver);

MODULE_AUTHOR("Alpha-SID6");
MODULE_DESCRIPTION("3Y Power yesm1300am driver");
MODULE_LICENSE("GPL");
