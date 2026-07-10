/*
 * A hwmon PSU monitoring driver for Juniper QFX5200 platform
 *
 * Copyright (C) 2020 Juniper Networks.
 * Ciju Rajan K <crajank@juniper.net>
 *
 * Based on ym2651.c by Brandon Chuang <brandon_chuang@accton.com.tw>
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

#define		TEMP_INT_MASK		(0xFFC0)
#define 	V_IN_INT_MASK		(0xFFE0)	
#define 	V_OUT_INT_MASK		(0xFF00)
#define		INVALID_READING		(0xFFFF)
#define 	TEMP_FR_LEN		6
#define 	V_IN_FR_LEN		5
#define 	V_OUT_FR_LEN		8

static const unsigned short normal_i2c[] = { 0x58, 0x58, I2C_CLIENT_END };

struct psu_data {
	struct device	*hwmon_dev;
	struct mutex	update_lock;
	char		valid;           /* !=0 if registers are valid */
	unsigned long	last_updated;    /* In jiffies */
	u16  		v_out;
	u16  		i_out;
	u16  		v_in;
	u16  		i_in;
	u16  		temp1_input;
	u16  		temp2_input;
	u16  		temp3_input;
	u16  		temp4_input;
	u16  		fan1_input;
	u16  		fan2_input;
};

static ssize_t show_word(struct device *dev, struct device_attribute *da,
                         char *buf);

static struct psu_data *psu_update_device(struct device *dev);

enum psu_sysfs_attributes {
    PSU_V_OUT,
    PSU_I_OUT,
    PSU_V_IN,
    PSU_I_IN,
    PSU_TEMP1_INPUT,
    PSU_TEMP2_INPUT,
    PSU_TEMP3_INPUT,
    PSU_TEMP4_INPUT,
    PSU_FAN1_RPM,
    PSU_FAN2_RPM,
};

/*
 * sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(in3_input,	S_IRUGO,  show_word,	NULL,	PSU_V_OUT);
static SENSOR_DEVICE_ATTR(curr2_input,	S_IRUGO,  show_word,	NULL,	PSU_I_OUT);
static SENSOR_DEVICE_ATTR(psu_v_in,     S_IRUGO,  show_word,	NULL,	PSU_V_IN);
static SENSOR_DEVICE_ATTR(psu_i_in,     S_IRUGO,  show_word,	NULL,	PSU_I_IN);
static SENSOR_DEVICE_ATTR(temp1_input,	S_IRUGO,  show_word,	NULL,	PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(temp2_input,	S_IRUGO,  show_word,	NULL,	PSU_TEMP2_INPUT);
static SENSOR_DEVICE_ATTR(temp3_input,	S_IRUGO,  show_word,	NULL,	PSU_TEMP3_INPUT);
static SENSOR_DEVICE_ATTR(temp4_input,	S_IRUGO,  show_word,	NULL,	PSU_TEMP4_INPUT);
static SENSOR_DEVICE_ATTR(fan1_input,	S_IRUGO,  show_word,	NULL,	PSU_FAN1_RPM);
static SENSOR_DEVICE_ATTR(fan2_input,	S_IRUGO,  show_word,	NULL,	PSU_FAN2_RPM);

static struct attribute *psu_attributes[] = {
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_curr2_input.dev_attr.attr,
	&sensor_dev_attr_psu_v_in.dev_attr.attr,
	&sensor_dev_attr_psu_i_in.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	&sensor_dev_attr_temp4_input.dev_attr.attr,
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	NULL
};

static short convert_to_decimal(u16 value,
		u16 data_mask,
		u8 fr_len) 
{
        return (((short)value >= 0) ? ((value & data_mask) >> fr_len) : 
                (-((((~(value & data_mask)) >> fr_len) + 1))));
}

static ssize_t show_word(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct psu_data *data = psu_update_device(dev);
    u32 value = 0x0000;

    switch (attr->index) {
        case PSU_V_OUT:
		value = 1000 * convert_to_decimal(data->v_out, 
						V_OUT_INT_MASK, 
						V_OUT_FR_LEN);
		break;
	case PSU_V_IN:
		value = convert_to_decimal(data->v_in, 
						V_IN_INT_MASK, 
						V_IN_FR_LEN);
		break;
	case PSU_I_IN:
		value = data->i_in;
		break;
	case PSU_I_OUT:
		value = 1000 * data->i_out / 64;
		break;
	case PSU_FAN1_RPM:
		value = data->fan1_input;
		break;
	case PSU_FAN2_RPM:
		value = data->fan2_input;
		break;
	case PSU_TEMP1_INPUT:
		value = 1000 * convert_to_decimal(data->temp1_input, 
						TEMP_INT_MASK, 
						TEMP_FR_LEN);
		break;
	case PSU_TEMP2_INPUT:
		value = 1000 * convert_to_decimal(data->temp2_input, 
						TEMP_INT_MASK, 
						TEMP_FR_LEN);
		break;
	case PSU_TEMP3_INPUT:
		value = 1000 * convert_to_decimal(data->temp3_input, 
						TEMP_INT_MASK, 
						TEMP_FR_LEN);
		break;
	case PSU_TEMP4_INPUT:
		value = 1000 * convert_to_decimal(data->temp4_input, 
						TEMP_INT_MASK, 
						TEMP_FR_LEN);
		break;
    }

    return sprintf(buf, "%d\n", (int)value);
}

static const struct attribute_group jnx_psu_group = {
    .attrs = psu_attributes,
};

static int jnx_psu_probe(struct i2c_client *client,
                         const struct i2c_device_id *dev_id)
{
    struct psu_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter,
                                 I2C_FUNC_SMBUS_BYTE_DATA |
                                 I2C_FUNC_SMBUS_WORD_DATA |
                                 I2C_FUNC_SMBUS_I2C_BLOCK)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct psu_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    dev_dbg(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &jnx_psu_group);
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
    sysfs_remove_group(&client->dev.kobj, &jnx_psu_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static int jnx_psu_remove(struct i2c_client *client)
{
    struct psu_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &jnx_psu_group);
    kfree(data);

    return 0;
}

enum {
	JPSU,
};

static const struct i2c_device_id psu_id[] = {
    { "jpsu", JPSU },
    {}
};
MODULE_DEVICE_TABLE(i2c, psu_id);


static struct i2c_driver jnx_psu_monitor_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "jnx-psu-monitor",
    },
    .probe	  = jnx_psu_probe,
    .remove	  = jnx_psu_remove,
    .id_table	  = psu_id,
    .address_list = normal_i2c,
};


static int psu_read_word(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_word_data(client, reg);
}

struct reg_data_word {
    u8   reg;
    u16 *value;
};

static struct psu_data *psu_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct psu_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
            || !data->valid) {
        int i, status;
        struct reg_data_word regs_word[] = {
            {0x00, &data->temp1_input},
            {0x01, &data->temp2_input},
            {0x02, &data->temp3_input},
            {0x03, &data->temp4_input},
            {0x28, &data->v_out},
            {0x32, &data->v_in},
            {0x33, &data->i_out},
            {0x3D, &data->i_in},
            {0x20, &data->fan1_input},
            {0x21, &data->fan2_input}
	};

        dev_dbg(dev, "Starting psu monitoring update\n");

        /* Read word data */
        for (i = 0; i < ARRAY_SIZE(regs_word); i++) {
            status = psu_read_word(client, regs_word[i].reg);

            if (status < 0)
            {
                dev_dbg(&client->dev, "reg %d, err %d\n",
                        regs_word[i].reg, status);
                *(regs_word[i].value) = 0;
            }
            else {
                u16 value = (u16)status;
                (INVALID_READING != value) ? (*(regs_word[i].value) = status) :
                                    (*(regs_word[i].value) = 0);
            }
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return data;
}

module_i2c_driver(jnx_psu_monitor_driver);

MODULE_AUTHOR("Ciju Rajan K <crajank@juniper.net>");
MODULE_DESCRIPTION("Juniper PSU monitoring driver for QFX5200");
MODULE_LICENSE("GPL");

