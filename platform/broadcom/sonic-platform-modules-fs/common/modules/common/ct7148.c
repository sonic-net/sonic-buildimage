// SPDX-License-Identifier: GPL-2.0-or-later
/* ct7318.c
 *
 * Copyright (C) 2023 sonic_rd@whitebox
 *
 */

/*
 * Driver for the sensylink CT7318 temperature sensor IC.
 * Supported models: CT7318
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
#include <linux/of_device.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <wb_bsp_kernel_debug.h>

/* debug switch level */
typedef enum {
    DBG_START,
    DBG_VERBOSE,
    DBG_KEY,
    DBG_WARN,
    DBG_ERROR,
    DBG_END,
} dbg_level_t;

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

enum chips { ct7318 };

/* The CT7318 registers */
#define CT7318_CONFIG_REG_1             0x09
#define CT7318_CONVERSION_RATE_REG      0x0A
#define CT7318_MANUFACTURER_ID_REG      0xFE
#define CT7318_DEVICE_ID_REG            0xFF

static const u8 CT7318_TEMP_MSB[2] = { 0x00, 0x01 };
static const u8 CT7318_TEMP_LSB[2] = { 0x15, 0x10 };

/* Flags */
#define CT7318_CONFIG_SHUTDOWN          0x40
#define CT7318_CONFIG_RANGE             0x04

/* Manufacturer / Device ID's */
#define CT7318_MANUFACTURER_ID          0x59
#define CT7318_DEVICE_ID                0x8D
#define CT7318_TEMP_INVALID_RETRY_TIMES    (3)

static const struct i2c_device_id ct7318_id[] = {
    { "ct7318", 2 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ct7318_id);

static const struct of_device_id ct7318_of_match[] = {
    {.compatible = "sensylink,ct7318"},
    { },
};
MODULE_DEVICE_TABLE(of, ct7318_of_match);

struct ct7318_data {
    struct i2c_client *client;
    struct mutex update_lock;
    u32 temp_config[5];
    struct hwmon_channel_info temp_info;
    const struct hwmon_channel_info *info[2];
    struct hwmon_chip_info chip;
    char valid;
    unsigned long last_updated;
    unsigned long channels;
    u8 config;
    s16 temp[4];
    enum chips kind;
    unsigned int update_interval;	/* in milliseconds */
};

/* input temp threshold check */
typedef struct ct7318_temp_threshold_s {
    int chip_type;
    int temp_max;
    int temp_min;
} ct7318_temp_threshold_t;

static ct7318_temp_threshold_t g_ct7318_input_threshold_info[] = {
    {
        .chip_type = ct7318,
        .temp_max = 127000,
        .temp_min = -55000,
    },
};


static int ct7318_input_temp_check(struct ct7318_data *data, int input_val)
{
    int i, size;

    size = ARRAY_SIZE(g_ct7318_input_threshold_info);

    for (i = 0; i < size; i++) {
        if (g_ct7318_input_threshold_info[i].chip_type == data->kind) {
            if ((input_val > g_ct7318_input_threshold_info[i].temp_max)
                    || (input_val < g_ct7318_input_threshold_info[i].temp_min)) {
                dev_dbg(&data->client->dev, "input temp: %d not in range[%d, %d]\n",
                    input_val, g_ct7318_input_threshold_info[i].temp_min,
                    g_ct7318_input_threshold_info[i].temp_max);
                return -EINVAL;
            }
            dev_dbg(&data->client->dev, "input temp: %d in range[%d, %d]", input_val,
                g_ct7318_input_threshold_info[i].temp_min, g_ct7318_input_threshold_info[i].temp_max);
            return 0;
        }
    }
    return 0;
}

static int ct7318_register_to_temp(s16 reg)
{
    s16 tmp_val;
    int val;

    DEBUG_VERBOSE("reg_data, data=0x%04x \n", reg);

    /* Positive number:reg*0.125 */
    if (!(reg & 0x400)) {
        val = reg * 125;
        /* Negative number: The first bit is the sign bit, and the rest is inverted +1 */
    } else {
        tmp_val = ((~((s16)reg)) & 0x7ff) + 1;
        DEBUG_VERBOSE("ct7318, tmp_val=0x%08x -- %d\n", tmp_val, tmp_val);
        val = -(tmp_val * 125);
    }

    DEBUG_VERBOSE("ct7318 reg2data, val=0x%08x -- %d \n", val, val);

    return val;
}

static struct ct7318_data *ct7318_update_device(struct device *dev)
{
    struct ct7318_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int i;

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + msecs_to_jiffies(data->update_interval)) || !data->valid) {

        for (i = 0; i < data->channels; i++) {
            data->temp[i] = i2c_smbus_read_byte_data(client, CT7318_TEMP_MSB[i]) << 3;
            data->temp[i] |= (i2c_smbus_read_byte_data(client, CT7318_TEMP_LSB[i]) >> 5);
        }
        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return data;
}

static int ct7318_read(struct device *dev, enum hwmon_sensor_types type,
                       u32 attr, int channel, long *val)
{
    struct ct7318_data *data;
    int ret, i;

    switch (attr) {
        case hwmon_temp_input:
            for (i = 0; i < CT7318_TEMP_INVALID_RETRY_TIMES; i++) {
                data = ct7318_update_device(dev);
                if (IS_ERR(data)) {
                    return PTR_ERR(data);
                }
                *val = ct7318_register_to_temp(data->temp[channel]);
                /* do input_temp_check */
                ret = ct7318_input_temp_check(data, *val);
                if (ret == 0) { /* input temp check ok */
                    return 0;
                }
                if ((i + 1) < CT7318_TEMP_INVALID_RETRY_TIMES) {
                    msleep(data->update_interval);
                }
            }
            dev_info(&data->client->dev, "temp_input value: %ld invalid\n", *val);
            return -EINVAL;
        case hwmon_temp_fault:
            /*
            * The OPEN bit signals a fault. This is bit 0 of the temperature
            * register (low byte).
            */
            return 0;
        default:
            return -EOPNOTSUPP;
    }
}

static umode_t ct7318_is_visible(const void *data, enum hwmon_sensor_types type, u32 attr, int channel)
{
    switch (attr) {
        case hwmon_temp_fault:
            if (channel == 0) {
                return 0;
            }
            return 0444;
        case hwmon_temp_input:
            return 0444;
        default:
            return 0;
    }
}

static const struct hwmon_ops ct7318_ops = {
    .read = ct7318_read,
    .is_visible = ct7318_is_visible,
};

static int ct7318_probe(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct device *hwmon_dev;
    struct ct7318_data *data;
    int i;

    data = devm_kzalloc(dev, sizeof(struct ct7318_data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

    mutex_init(&data->update_lock);

    data->channels = i2c_match_id(ct7318_id, client)->driver_data;
    data->client = client;
    data->update_interval = 500;

    for (i = 0; i < data->channels; i++) {
        data->temp_config[i] = HWMON_T_INPUT;
    }

    data->chip.ops = &ct7318_ops;
    data->chip.info = data->info;

    data->info[0] = &data->temp_info;

    data->temp_info.type = hwmon_temp;
    data->temp_info.config = data->temp_config;

    hwmon_dev = devm_hwmon_device_register_with_info(dev, client->name, data, &data->chip, NULL);
    return PTR_ERR_OR_ZERO(hwmon_dev);
}

static struct i2c_driver ct7318_driver = {
    .class = I2C_CLASS_HWMON,
    .driver = {
        .name   = "ct7318",
        .of_match_table = of_match_ptr(ct7318_of_match),
    },
    .probe_new = ct7318_probe,
    .id_table = ct7318_id,
};

module_i2c_driver(ct7318_driver);

MODULE_AUTHOR("sonic_rd@whitebox");
MODULE_DESCRIPTION("Sensylink CT7318 temperature sensor driver");
MODULE_LICENSE("GPL");
