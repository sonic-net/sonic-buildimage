/*
 * An hwmon driver for the 3Y Power YM-2651Y Power Module
 *
 * Tested and validated on Juniper QFX5210
 * Ciju Rajan K <crajank@juniper.net>
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

#define MAX_FAN_DUTY_CYCLE 100

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { 0x58, 0x5b, I2C_CLIENT_END };

enum chips {
	YM2651,
	YM2401,
	YM2851,
};

/* Each client has this additional data
 */
struct ym2651y_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8   capability;     /* Register value */
    u16  status_word;    /* Register value */
    u8   fan_fault;      /* Register value */
    u8   over_temp;      /* Register value */
    u16  v_out;          /* Register value */
    u16  i_out;          /* Register value */
    u16  p_out;          /* Register value */
    u16  temp;           /* Register value */
    u16  fan_speed;      /* Register value */
    u16  fan_duty_cycle[2];  /* Register value */
    u8   fan_dir[4];     /* Register value */
    u8   pmbus_revision; /* Register value */
    u8   mfr_id[10];     /* Register value */
    u8   mfr_model[10];  /* Register value */
    u8   mfr_revsion[3]; /* Register value */
    u16  mfr_vin_min;    /* Register value */
    u16  mfr_vin_max;    /* Register value */
    u16  mfr_iin_max;    /* Register value */
    u16  mfr_iout_max;   /* Register value */
    u16  mfr_pin_max;    /* Register value */
    u16  mfr_pout_max;   /* Register value */
    u16  mfr_vout_min;   /* Register value */
    u16  mfr_vout_max;   /* Register value */
};

static ssize_t show_byte(struct device *dev, struct device_attribute *da,
                         char *buf);
static ssize_t show_word(struct device *dev, struct device_attribute *da,
                         char *buf);
static ssize_t show_linear(struct device *dev, struct device_attribute *da,
                           char *buf);
static ssize_t show_fan_fault(struct device *dev, struct device_attribute *da,
                              char *buf);
static ssize_t show_over_temp(struct device *dev, struct device_attribute *da,
                              char *buf);
static ssize_t show_ascii(struct device *dev, struct device_attribute *da,
                          char *buf);
static struct ym2651y_data *ym2651y_update_device(struct device *dev);
static ssize_t set_fan_duty_cycle(struct device *dev, struct device_attribute *da,
                                  const char *buf, size_t count);
static int ym2651y_write_word(struct i2c_client *client, u8 reg, u16 value);

enum ym2651y_sysfs_attributes {
    PSU_POWER_ON = 0,
    PSU_TEMP_FAULT,
    PSU_POWER_GOOD,
    PSU_FAN1_FAULT,
    PSU_FAN_DIRECTION,
    PSU_OVER_TEMP,
    PSU_V_OUT,
    PSU_I_OUT,
    PSU_P_OUT,
    PSU_P_OUT_UV,     /*In Unit of microVolt, instead of mini.*/
    PSU_TEMP1_INPUT,
    PSU_FAN1_SPEED,
    PSU_FAN1_DUTY_CYCLE,
    PSU_PMBUS_REVISION,
    PSU_MFR_ID,
    PSU_MFR_MODEL,
    PSU_MFR_REVISION,
    PSU_MFR_VIN_MIN,
    PSU_MFR_VIN_MAX,
    PSU_MFR_VOUT_MIN,
    PSU_MFR_VOUT_MAX,
    PSU_MFR_IIN_MAX,
    PSU_MFR_IOUT_MAX,
    PSU_MFR_PIN_MAX,
    PSU_MFR_POUT_MAX
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(psu_power_on,    S_IRUGO, show_word,      NULL, PSU_POWER_ON);
static SENSOR_DEVICE_ATTR(psu_temp_fault,  S_IRUGO, show_word,      NULL, PSU_TEMP_FAULT);
static SENSOR_DEVICE_ATTR(psu_power_good,  S_IRUGO, show_word,      NULL, PSU_POWER_GOOD);
static SENSOR_DEVICE_ATTR(psu_fan1_fault,  S_IRUGO, show_fan_fault, NULL, PSU_FAN1_FAULT);
static SENSOR_DEVICE_ATTR(psu_over_temp,   S_IRUGO, show_over_temp, NULL, PSU_OVER_TEMP);
static SENSOR_DEVICE_ATTR(psu_v_out,       S_IRUGO, show_linear,    NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(psu_i_out,       S_IRUGO, show_linear,    NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(psu_p_out,       S_IRUGO, show_linear,    NULL, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(psu_temp1_input, S_IRUGO, show_linear,    NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(psu_fan1_speed_rpm, S_IRUGO, show_linear, NULL, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(psu_fan1_duty_cycle_percentage, S_IWUSR | S_IRUGO, show_linear, set_fan_duty_cycle, PSU_FAN1_DUTY_CYCLE);
static SENSOR_DEVICE_ATTR(psu_fan_dir,     S_IRUGO, show_ascii,     NULL, PSU_FAN_DIRECTION);
static SENSOR_DEVICE_ATTR(psu_pmbus_revision, S_IRUGO, show_byte,   NULL, PSU_PMBUS_REVISION);
static SENSOR_DEVICE_ATTR(psu_mfr_id,         S_IRUGO, show_ascii,  NULL, PSU_MFR_ID);
static SENSOR_DEVICE_ATTR(psu_mfr_model,      S_IRUGO, show_ascii,  NULL, PSU_MFR_MODEL);
static SENSOR_DEVICE_ATTR(psu_mfr_revision,    S_IRUGO, show_ascii, NULL, PSU_MFR_REVISION);
static SENSOR_DEVICE_ATTR(psu_mfr_vin_min,    S_IRUGO, show_linear, NULL, PSU_MFR_VIN_MIN);
static SENSOR_DEVICE_ATTR(psu_mfr_vin_max,    S_IRUGO, show_linear, NULL, PSU_MFR_VIN_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_vout_min,   S_IRUGO, show_linear, NULL, PSU_MFR_VOUT_MIN);
static SENSOR_DEVICE_ATTR(psu_mfr_vout_max,   S_IRUGO, show_linear, NULL, PSU_MFR_VOUT_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_iin_max,   S_IRUGO, show_linear, NULL, PSU_MFR_IIN_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_iout_max,   S_IRUGO, show_linear, NULL, PSU_MFR_IOUT_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_pin_max,   S_IRUGO, show_linear, NULL, PSU_MFR_PIN_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_pout_max,   S_IRUGO, show_linear, NULL, PSU_MFR_POUT_MAX);

/*Duplicate nodes for lm-sensors.*/
static SENSOR_DEVICE_ATTR(in3_input, S_IRUGO, show_linear,    NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(curr2_input, S_IRUGO, show_linear,    NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, show_linear,    NULL, PSU_P_OUT_UV);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_linear,    NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_linear, NULL, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(temp1_fault,  S_IRUGO, show_word,      NULL, PSU_TEMP_FAULT);

static struct attribute *ym2651y_attributes[] = {
    &sensor_dev_attr_psu_power_on.dev_attr.attr,
    &sensor_dev_attr_psu_temp_fault.dev_attr.attr,
    &sensor_dev_attr_psu_power_good.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_fault.dev_attr.attr,
    &sensor_dev_attr_psu_over_temp.dev_attr.attr,
    &sensor_dev_attr_psu_v_out.dev_attr.attr,
    &sensor_dev_attr_psu_i_out.dev_attr.attr,
    &sensor_dev_attr_psu_p_out.dev_attr.attr,
    &sensor_dev_attr_psu_temp1_input.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_duty_cycle_percentage.dev_attr.attr,
    &sensor_dev_attr_psu_fan_dir.dev_attr.attr,
    &sensor_dev_attr_psu_pmbus_revision.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_id.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_revision.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_vin_min.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_vin_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_pout_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_iin_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_pin_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_vout_min.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_vout_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_iout_max.dev_attr.attr,
    /*Duplicate nodes for lm-sensors.*/
    &sensor_dev_attr_curr2_input.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_power2_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    &sensor_dev_attr_temp1_fault.dev_attr.attr,
    NULL
};

static ssize_t show_byte(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct ym2651y_data *data = ym2651y_update_device(dev);

    return (attr->index == PSU_PMBUS_REVISION) ? sprintf(buf, "%d\n", data->pmbus_revision) :
           sprintf(buf, "0\n");
}

static ssize_t show_word(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct ym2651y_data *data = ym2651y_update_device(dev);
    u16 status = 0;

    switch (attr->index) {
    case PSU_POWER_ON: /* psu_power_on, low byte bit 6 of status_word, 0=>ON, 1=>OFF */
        status = (data->status_word & 0x40) ? 0 : 1;
        break;
    case PSU_TEMP_FAULT: /* psu_temp_fault, low byte bit 2 of status_word, 0=>Normal, 1=>temp fault */
        status = (data->status_word & 0x4) >> 2;
        break;
    case PSU_POWER_GOOD: /* psu_power_good, high byte bit 3 of status_word, 0=>OK, 1=>FAIL */
        status = (data->status_word & 0x800) ? 0 : 1;
        break;
    }

    return sprintf(buf, "%d\n", status);
}

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
    struct ym2651y_data *data = i2c_get_clientdata(client);
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
    ym2651y_write_word(client, 0x3B + nr, data->fan_duty_cycle[nr]);
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t show_linear(struct device *dev, struct device_attribute *da,
                           char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct ym2651y_data *data = ym2651y_update_device(dev);

    u16 value = 0;
    int exponent, mantissa;
    int multiplier = 1000;

    switch (attr->index) {
    case PSU_V_OUT:
        value = data->v_out;
        break;
    case PSU_I_OUT:
        value = data->i_out;
        break;
    case PSU_P_OUT_UV:
        multiplier = 1000000;  /*For lm-sensors, unit is micro-Volt.*/
    /*Passing through*/
    case PSU_P_OUT:
        value = data->p_out;
        break;
    case PSU_TEMP1_INPUT:
        value = data->temp;
        break;
    case PSU_FAN1_SPEED:
        value = data->fan_speed;
        multiplier = 1;
        break;
    case PSU_FAN1_DUTY_CYCLE:
        value = data->fan_duty_cycle[0];
        multiplier = 1;
        break;
    case PSU_MFR_VIN_MIN:
        value = data->mfr_vin_min;
        break;
    case PSU_MFR_VIN_MAX:
        value = data->mfr_vin_max;
        break;
    case PSU_MFR_VOUT_MIN:
        value = data->mfr_vout_min;
        break;
    case PSU_MFR_VOUT_MAX:
        value = data->mfr_vout_max;
        break;
    case PSU_MFR_PIN_MAX:
        value = data->mfr_pin_max;
        break;
    case PSU_MFR_POUT_MAX:
        value = data->mfr_pout_max;
        break;
    case PSU_MFR_IOUT_MAX:
        value = data->mfr_iout_max;
        break;
    case PSU_MFR_IIN_MAX:
        value = data->mfr_iin_max;
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
    struct ym2651y_data *data = ym2651y_update_device(dev);

    u8 shift = (attr->index == PSU_FAN1_FAULT) ? 7 : 6;

    return sprintf(buf, "%d\n", data->fan_fault >> shift);
}

static ssize_t show_over_temp(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct ym2651y_data *data = ym2651y_update_device(dev);

    return sprintf(buf, "%d\n", data->over_temp >> 7);
}

static ssize_t show_ascii(struct device *dev, struct device_attribute *da,
                          char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct ym2651y_data *data = ym2651y_update_device(dev);
    u8 *ptr = NULL;

    switch (attr->index) {
    case PSU_FAN_DIRECTION: /* psu_fan_dir */
        ptr = data->fan_dir;
        break;
    case PSU_MFR_ID: /* psu_mfr_id */
        ptr = data->mfr_id;
        break;
    case PSU_MFR_MODEL: /* psu_mfr_model */
        ptr = data->mfr_model;
        break;
    case PSU_MFR_REVISION: /* psu_mfr_revision */
        ptr = data->mfr_revsion;
        break;
    default:
        return 0;
    }

    return sprintf(buf, "%s\n", ptr);
}

static const struct attribute_group ym2651y_group = {
    .attrs = ym2651y_attributes,
};

static int ym2651y_probe(struct i2c_client *client,
                         const struct i2c_device_id *dev_id)
{
    struct ym2651y_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter,
                                 I2C_FUNC_SMBUS_BYTE_DATA |
                                 I2C_FUNC_SMBUS_WORD_DATA |
                                 I2C_FUNC_SMBUS_I2C_BLOCK)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct ym2651y_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &ym2651y_group);
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
    sysfs_remove_group(&client->dev.kobj, &ym2651y_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static int ym2651y_remove(struct i2c_client *client)
{
    struct ym2651y_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &ym2651y_group);
    kfree(data);

    return 0;
}

static const struct i2c_device_id ym2651y_id[] = {
    { "ym2651", YM2651 },
    { "ym2401", YM2401 },
    { "ym2851", YM2851 },
    {}
};
MODULE_DEVICE_TABLE(i2c, ym2651y_id);

static struct i2c_driver ym2651y_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name    = "ym2651",
    },
    .probe      = ym2651y_probe,
    .remove      = ym2651y_remove,
    .id_table = ym2651y_id,
    .address_list = normal_i2c,
};

static int ym2651y_read_byte(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}

static int ym2651y_read_word(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_word_data(client, reg);
}

static int ym2651y_write_word(struct i2c_client *client, u8 reg, u16 value)
{
    return i2c_smbus_write_word_data(client, reg, value);
}

static int ym2651y_read_block(struct i2c_client *client, u8 command, u8 *data,
                              int data_len)
{
    int result = i2c_smbus_read_i2c_block_data(client, command, data_len, data);

    if (unlikely(result < 0))
        goto abort;
    if (unlikely(result != data_len)) {
        result = -EIO;
        goto abort;
    }

    result = 0;

abort:
    return result;
}

struct reg_data_byte {
    u8   reg;
    u8  *value;
};

struct reg_data_word {
    u8   reg;
    u16 *value;
};

static struct ym2651y_data *ym2651y_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct ym2651y_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
            || !data->valid) {
        int i, status;
        u8 command;
        u8 fan_dir[5] = {0};
        struct reg_data_byte regs_byte[] = { {0x19, &data->capability},
            {0x7d, &data->over_temp},
            {0x81, &data->fan_fault},
            {0x98, &data->pmbus_revision}
        };
        struct reg_data_word regs_word[] = { {0x79, &data->status_word},
            {0x8b, &data->v_out},
            {0x8c, &data->i_out},
            {0x96, &data->p_out},
            {0x8d, &data->temp},
            {0x3b, &(data->fan_duty_cycle[0])},
            {0x3c, &(data->fan_duty_cycle[1])},
            {0x90, &data->fan_speed},
            {0xa0, &data->mfr_vin_min},
            {0xa1, &data->mfr_vin_max},
            {0xa2, &data->mfr_iin_max},
            {0xa3, &data->mfr_pin_max},
            {0xa4, &data->mfr_vout_min},
            {0xa5, &data->mfr_vout_max},
            {0xa6, &data->mfr_iout_max},
            {0xa7, &data->mfr_pout_max}
        };

        dev_dbg(&client->dev, "Starting ym2651 update\n");

        /* Read byte data */
        for (i = 0; i < ARRAY_SIZE(regs_byte); i++) {
            status = ym2651y_read_byte(client, regs_byte[i].reg);

            if (status < 0)
            {
                dev_dbg(&client->dev, "reg %d, err %d\n",
                        regs_byte[i].reg, status);
                *(regs_byte[i].value) = 0;
            }
            else {
                *(regs_byte[i].value) = status;
            }
        }

        /* Read word data */
        for (i = 0; i < ARRAY_SIZE(regs_word); i++) {
            status = ym2651y_read_word(client, regs_word[i].reg);

            if (status < 0)
            {
                dev_dbg(&client->dev, "reg %d, err %d\n",
                        regs_word[i].reg, status);
                *(regs_word[i].value) = 0;
            }
            else {
                *(regs_word[i].value) = status;
            }
        }

        /* Read fan_direction */
        command = 0xC3;
        status = ym2651y_read_block(client, command, fan_dir, ARRAY_SIZE(fan_dir)-1);

        if (status < 0) {
            dev_dbg(&client->dev, "reg %d, err %d\n", command, status);
        }

        strncpy(data->fan_dir, fan_dir+1, ARRAY_SIZE(data->fan_dir)-1);
        data->fan_dir[ARRAY_SIZE(data->fan_dir)-1] = '\0';

        /* Read mfr_id */
        command = 0x99;
        status = ym2651y_read_block(client, command, data->mfr_id,
                                    ARRAY_SIZE(data->mfr_id)-1);
        data->mfr_id[ARRAY_SIZE(data->mfr_id)-1] = '\0';

        if (status < 0)
            dev_dbg(&client->dev, "reg %d, err %d\n", command, status);

        /* Read mfr_model */
        command = 0x9a;
        status = ym2651y_read_block(client, command, data->mfr_model,
                                    ARRAY_SIZE(data->mfr_model)-1);
        data->mfr_model[ARRAY_SIZE(data->mfr_model)-1] = '\0';

        if (status < 0)
            dev_dbg(&client->dev, "reg %d, err %d\n", command, status);

        /* Read mfr_revsion */
        command = 0x9b;
        status = ym2651y_read_block(client, command, data->mfr_revsion,
                                    ARRAY_SIZE(data->mfr_revsion)-1);
        data->mfr_revsion[ARRAY_SIZE(data->mfr_revsion)-1] = '\0';

        if (status < 0)
            dev_dbg(&client->dev, "reg %d, err %d\n", command, status);

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return data;
}

module_i2c_driver(ym2651y_driver);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("3Y Power YM-2651Y driver");
MODULE_LICENSE("GPL");


