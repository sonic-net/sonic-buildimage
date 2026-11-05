/*
 * An hwmon driver for the CPR-4011-4Mxx Redundant Power Module
 *
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
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
#if 0
#define DEBUG
#endif

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>

#define MAX_FAN_DUTY_CYCLE 100

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { 0x3c, 0x3d, 0x3e, 0x3f, I2C_CLIENT_END };

/* Each client has this additional data
 */
struct cpr_4011_4mxx_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8   vout_mode;     /* Register value */
    u16  v_in;          /* Register value */
    u16  v_out;         /* Register value */
    u16  i_in;          /* Register value */
    u16  i_out;         /* Register value */
    u16  p_in;          /* Register value */
    u16  p_out;         /* Register value */
    u16  temp_input[2]; /* Register value */
    u8   fan_fault;     /* Register value */
    u16  fan_duty_cycle[2];  /* Register value */
    u16  fan_speed[2];  /* Register value */
};

static ssize_t show_linear(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_fan_fault(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_vout(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_fan_duty_cycle(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static int cpr_4011_4mxx_write_word(struct i2c_client *client, u8 reg, u16 value);
static struct cpr_4011_4mxx_data *cpr_4011_4mxx_update_device(struct device *dev);

enum cpr_4011_4mxx_sysfs_attributes {
    PSU_V_IN,
    PSU_V_OUT,
    PSU_I_IN,
    PSU_I_OUT,
    PSU_P_IN,
    PSU_P_OUT,
    PSU_TEMP1_INPUT,
    PSU_FAN1_FAULT,
    PSU_FAN1_DUTY_CYCLE,
    PSU_FAN1_SPEED,
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(psu_v_in,        S_IRUGO, show_linear,      NULL, PSU_V_IN);
static SENSOR_DEVICE_ATTR(psu_v_out,       S_IRUGO, show_vout,        NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(psu_i_in,        S_IRUGO, show_linear,      NULL, PSU_I_IN);
static SENSOR_DEVICE_ATTR(psu_i_out,       S_IRUGO, show_linear,      NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(psu_p_in,        S_IRUGO, show_linear,      NULL, PSU_P_IN);
static SENSOR_DEVICE_ATTR(psu_p_out,       S_IRUGO, show_linear,      NULL, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(psu_temp1_input, S_IRUGO, show_linear,      NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(psu_fan1_fault,  S_IRUGO, show_fan_fault,   NULL, PSU_FAN1_FAULT);
static SENSOR_DEVICE_ATTR(psu_fan1_duty_cycle_percentage, S_IWUSR | S_IRUGO, show_linear, set_fan_duty_cycle, PSU_FAN1_DUTY_CYCLE);
static SENSOR_DEVICE_ATTR(psu_fan1_speed_rpm, S_IRUGO, show_linear,   NULL, PSU_FAN1_SPEED);


/*Duplicate nodes for lm-sensors.*/
static SENSOR_DEVICE_ATTR(in1_input,    S_IRUGO, show_linear,      NULL, PSU_V_IN);
static SENSOR_DEVICE_ATTR(in2_input,    S_IRUGO, show_vout,        NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(curr1_input,  S_IRUGO, show_linear,      NULL, PSU_I_IN);
static SENSOR_DEVICE_ATTR(curr2_input,  S_IRUGO, show_linear,      NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, show_linear,      NULL, PSU_P_IN);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, show_linear,      NULL, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_linear,      NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(fan1_fault,  S_IRUGO, show_fan_fault,   NULL, PSU_FAN1_FAULT);
static SENSOR_DEVICE_ATTR(pwm1_input, S_IRUGO, show_linear, NULL, PSU_FAN1_DUTY_CYCLE);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_linear,   NULL, PSU_FAN1_SPEED);



static struct attribute *cpr_4011_4mxx_attributes[] = {
    &sensor_dev_attr_psu_v_in.dev_attr.attr,
    &sensor_dev_attr_psu_v_out.dev_attr.attr,
    &sensor_dev_attr_psu_i_in.dev_attr.attr,
    &sensor_dev_attr_psu_i_out.dev_attr.attr,
    &sensor_dev_attr_psu_p_in.dev_attr.attr,
    &sensor_dev_attr_psu_p_out.dev_attr.attr,
    &sensor_dev_attr_psu_temp1_input.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_fault.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_duty_cycle_percentage.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_speed_rpm.dev_attr.attr,
    /*Duplicate nodes for lm-sensors.*/
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_curr1_input.dev_attr.attr,
    &sensor_dev_attr_curr2_input.dev_attr.attr,
    &sensor_dev_attr_power1_input.dev_attr.attr,
    &sensor_dev_attr_power2_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_fan1_fault.dev_attr.attr,
    &sensor_dev_attr_pwm1_input.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    NULL
};

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static ssize_t set_fan_duty_cycle(struct device *dev, struct device_attribute *da,
                                  const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpr_4011_4mxx_data *data = i2c_get_clientdata(client);
    int nr = (attr->index == PSU_FAN1_DUTY_CYCLE) ? 0 : 1;
    long speed;
    int error;

    error = kstrtol(buf, 10, &speed);
    if (error)
        return error;

    if (speed < 0 || speed > MAX_FAN_DUTY_CYCLE)
        return -EINVAL;

    mutex_lock(&data->update_lock);
    data->fan_duty_cycle[nr] = speed;
    cpr_4011_4mxx_write_word(client, 0x3B + nr, data->fan_duty_cycle[nr]);
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t show_linear(struct device *dev, struct device_attribute *da,
                           char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct cpr_4011_4mxx_data *data = cpr_4011_4mxx_update_device(dev);

    u16 value = 0;
    int exponent, mantissa;
    int multiplier = 1000;

    switch (attr->index) {
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
    case PSU_TEMP1_INPUT:
        value = data->temp_input[0];
        break;
    case PSU_FAN1_DUTY_CYCLE:
        multiplier = 1;
        value = data->fan_duty_cycle[0];
        break;
    case PSU_FAN1_SPEED:
        multiplier = 1;
        value = data->fan_speed[0];
        break;
    default:
        break;
    }

    exponent = two_complement_to_int(value >> 11, 5, 0x1f);
    mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);

    return (exponent >= 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * multiplier) :
           sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static ssize_t show_fan_fault(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct cpr_4011_4mxx_data *data = cpr_4011_4mxx_update_device(dev);

    u8 shift = (attr->index == PSU_FAN1_FAULT) ? 7 : 6;

    return sprintf(buf, "%d\n", data->fan_fault >> shift);
}

static ssize_t show_vout(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    struct cpr_4011_4mxx_data *data = cpr_4011_4mxx_update_device(dev);
    int exponent, mantissa;
    int multiplier = 1000;

    exponent = two_complement_to_int(data->vout_mode, 5, 0x1f);
    mantissa = data->v_out;

    return (exponent > 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * multiplier) :
           sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static const struct attribute_group cpr_4011_4mxx_group = {
    .attrs = cpr_4011_4mxx_attributes,
};

static int cpr_4011_4mxx_probe(struct i2c_client *client)
{
    struct cpr_4011_4mxx_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter,
                                 I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct cpr_4011_4mxx_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &cpr_4011_4mxx_group);
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
    sysfs_remove_group(&client->dev.kobj, &cpr_4011_4mxx_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void cpr_4011_4mxx_remove(struct i2c_client *client)
{
    struct cpr_4011_4mxx_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &cpr_4011_4mxx_group);
    kfree(data);

}

static const struct i2c_device_id cpr_4011_4mxx_id[] = {
    { "cpr_4011_4mxx", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, cpr_4011_4mxx_id);

static struct i2c_driver cpr_4011_4mxx_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "cpr_4011_4mxx",
    },
    .probe        = cpr_4011_4mxx_probe,
    .remove       = cpr_4011_4mxx_remove,
    .id_table     = cpr_4011_4mxx_id,
    .address_list = normal_i2c,
};

static int cpr_4011_4mxx_read_byte(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}

static int cpr_4011_4mxx_read_word(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_word_data(client, reg);
}

static int cpr_4011_4mxx_write_word(struct i2c_client *client, u8 reg, u16 value)
{
    return i2c_smbus_write_word_data(client, reg, value);
}

struct reg_data_byte {
    u8   reg;
    u8  *value;
};

struct reg_data_word {
    u8   reg;
    u16 *value;
};

static struct cpr_4011_4mxx_data *cpr_4011_4mxx_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpr_4011_4mxx_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
            || !data->valid) {
        int i, status;
        struct reg_data_byte regs_byte[] = { {0x20, &data->vout_mode},
            {0x81, &data->fan_fault}
        };
        struct reg_data_word regs_word[] = { {0x88, &data->v_in},
            {0x8b, &data->v_out},
            {0x89, &data->i_in},
            {0x8c, &data->i_out},
            {0x96, &data->p_out},
            {0x97, &data->p_in},
            {0x8d, &(data->temp_input[0])},
            {0x8e, &(data->temp_input[1])},
            {0x3b, &(data->fan_duty_cycle[0])},
            {0x3c, &(data->fan_duty_cycle[1])},
            {0x90, &(data->fan_speed[0])},
            {0x91, &(data->fan_speed[1])}
        };

        dev_dbg(&client->dev, "Starting cpr_4011_4mxx update\n");

        /* Read byte data */
        for (i = 0; i < ARRAY_SIZE(regs_byte); i++) {
            status = cpr_4011_4mxx_read_byte(client, regs_byte[i].reg);

            if (status < 0) {
                dev_dbg(&client->dev, "reg %d, err %d\n",
                        regs_byte[i].reg, status);
            }
            else {
                *(regs_byte[i].value) = status;
            }
        }

        /* Read word data */
        for (i = 0; i < ARRAY_SIZE(regs_word); i++) {
            status = cpr_4011_4mxx_read_word(client, regs_word[i].reg);

            if (status < 0) {
                dev_dbg(&client->dev, "reg %d, err %d\n",
                        regs_word[i].reg, status);
            }
            else {
                *(regs_word[i].value) = status;
            }
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return data;
}

static int __init cpr_4011_4mxx_init(void)
{
    return i2c_add_driver(&cpr_4011_4mxx_driver);
}

static void __exit cpr_4011_4mxx_exit(void)
{
    i2c_del_driver(&cpr_4011_4mxx_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("CPR_4011_4MXX driver");
MODULE_LICENSE("GPL");

module_init(cpr_4011_4mxx_init);
module_exit(cpr_4011_4mxx_exit);
