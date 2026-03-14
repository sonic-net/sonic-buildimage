/*
 * A hwmon driver for the as7716_32xbb_cpld
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
#include <linux/list.h>

static LIST_HEAD(cpld_client_list);
static struct mutex	 list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

#define I2C_RW_RETRY_COUNT				10
#define I2C_RW_RETRY_INTERVAL			60 /* ms */
#define STRING_TO_DEC_VALUE		10

static ssize_t show_present(struct device *dev, struct device_attribute *da,
                            char *buf);
static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
                                char *buf);
static ssize_t access(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
                            char *buf);
static ssize_t get_mode_reset(struct device *dev, struct device_attribute *da,
                              char *buf);
static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count);

static int as7716_32xb_cpld_read_internal(struct i2c_client *client, u8 reg);
static int as7716_32xb_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value);

static ssize_t sfp_value_show(struct device *dev, struct device_attribute *da,
                              char *buf);
static ssize_t sfp_value_store(struct device *dev, struct device_attribute *da,
                               const char *buf, size_t size);


#define PORT_NUM_MAX 32

struct as7716_32xb_cpld_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    unsigned int     present[PORT_NUM_MAX];
    unsigned int     reset[PORT_NUM_MAX];
};
enum port_id {
    PORT1_ID,
    PORT2_ID,
    PORT3_ID,
    PORT4_ID,
    PORT5_ID,
    PORT6_ID,
    PORT7_ID,
    PORT8_ID,
    PORT9_ID,
    PORT10_ID,
    PORT11_ID,
    PORT12_ID,
    PORT13_ID,
    PORT14_ID,
    PORT15_ID,
    PORT16_ID,
    PORT17_ID,
    PORT18_ID,
    PORT19_ID,
    PORT20_ID,
    PORT21_ID,
    PORT22_ID,
    PORT23_ID,
    PORT24_ID,
    PORT25_ID,
    PORT26_ID,
    PORT27_ID,
    PORT28_ID,
    PORT29_ID,
    PORT30_ID,
    PORT31_ID,
    PORT32_ID
};

/* Addresses scanned for as7716_32xbb_cpld
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

#define TRANSCEIVER_PRESENT_ATTR_ID(index)   MODULE_PRESENT_##index
#define TRANSCEIVER_RESET_ATTR_ID(index)     MODULE_RESET_##index

enum as7716_32xb_cpld_sysfs_attributes {
    CPLD_VERSION,
    ACCESS,
    MODULE_PRESENT_ALL,
    /* transceiver attributes */
    TRANSCEIVER_PRESENT_ATTR_ID(1),
    TRANSCEIVER_PRESENT_ATTR_ID(2),
    TRANSCEIVER_PRESENT_ATTR_ID(3),
    TRANSCEIVER_PRESENT_ATTR_ID(4),
    TRANSCEIVER_PRESENT_ATTR_ID(5),
    TRANSCEIVER_PRESENT_ATTR_ID(6),
    TRANSCEIVER_PRESENT_ATTR_ID(7),
    TRANSCEIVER_PRESENT_ATTR_ID(8),
    TRANSCEIVER_PRESENT_ATTR_ID(9),
    TRANSCEIVER_PRESENT_ATTR_ID(10),
    TRANSCEIVER_PRESENT_ATTR_ID(11),
    TRANSCEIVER_PRESENT_ATTR_ID(12),
    TRANSCEIVER_PRESENT_ATTR_ID(13),
    TRANSCEIVER_PRESENT_ATTR_ID(14),
    TRANSCEIVER_PRESENT_ATTR_ID(15),
    TRANSCEIVER_PRESENT_ATTR_ID(16),
    TRANSCEIVER_PRESENT_ATTR_ID(17),
    TRANSCEIVER_PRESENT_ATTR_ID(18),
    TRANSCEIVER_PRESENT_ATTR_ID(19),
    TRANSCEIVER_PRESENT_ATTR_ID(20),
    TRANSCEIVER_PRESENT_ATTR_ID(21),
    TRANSCEIVER_PRESENT_ATTR_ID(22),
    TRANSCEIVER_PRESENT_ATTR_ID(23),
    TRANSCEIVER_PRESENT_ATTR_ID(24),
    TRANSCEIVER_PRESENT_ATTR_ID(25),
    TRANSCEIVER_PRESENT_ATTR_ID(26),
    TRANSCEIVER_PRESENT_ATTR_ID(27),
    TRANSCEIVER_PRESENT_ATTR_ID(28),
    TRANSCEIVER_PRESENT_ATTR_ID(29),
    TRANSCEIVER_PRESENT_ATTR_ID(30),
    TRANSCEIVER_PRESENT_ATTR_ID(31),
    TRANSCEIVER_PRESENT_ATTR_ID(32),
    TRANSCEIVER_RESET_ATTR_ID(1),
    TRANSCEIVER_RESET_ATTR_ID(2),
    TRANSCEIVER_RESET_ATTR_ID(3),
    TRANSCEIVER_RESET_ATTR_ID(4),
    TRANSCEIVER_RESET_ATTR_ID(5),
    TRANSCEIVER_RESET_ATTR_ID(6),
    TRANSCEIVER_RESET_ATTR_ID(7),
    TRANSCEIVER_RESET_ATTR_ID(8),
    TRANSCEIVER_RESET_ATTR_ID(9),
    TRANSCEIVER_RESET_ATTR_ID(10),
    TRANSCEIVER_RESET_ATTR_ID(11),
    TRANSCEIVER_RESET_ATTR_ID(12),
    TRANSCEIVER_RESET_ATTR_ID(13),
    TRANSCEIVER_RESET_ATTR_ID(14),
    TRANSCEIVER_RESET_ATTR_ID(15),
    TRANSCEIVER_RESET_ATTR_ID(16),
    TRANSCEIVER_RESET_ATTR_ID(17),
    TRANSCEIVER_RESET_ATTR_ID(18),
    TRANSCEIVER_RESET_ATTR_ID(19),
    TRANSCEIVER_RESET_ATTR_ID(20),
    TRANSCEIVER_RESET_ATTR_ID(21),
    TRANSCEIVER_RESET_ATTR_ID(22),
    TRANSCEIVER_RESET_ATTR_ID(23),
    TRANSCEIVER_RESET_ATTR_ID(24),
    TRANSCEIVER_RESET_ATTR_ID(25),
    TRANSCEIVER_RESET_ATTR_ID(26),
    TRANSCEIVER_RESET_ATTR_ID(27),
    TRANSCEIVER_RESET_ATTR_ID(28),
    TRANSCEIVER_RESET_ATTR_ID(29),
    TRANSCEIVER_RESET_ATTR_ID(30),
    TRANSCEIVER_RESET_ATTR_ID(31),
    TRANSCEIVER_RESET_ATTR_ID(32),
};

/* sysfs attributes for hwmon
 */

/* transceiver attributes */
/*present*/
#define DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(module_present_##index, S_IWUSR|S_IRUGO, sfp_value_show, sfp_value_store, MODULE_PRESENT_##index)
#define DECLARE_TRANSCEIVER_ATTR(index)  &sensor_dev_attr_module_present_##index.dev_attr.attr

/*reset*/
//#define DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(index) \
//	static SENSOR_DEVICE_ATTR(module_reset_##index, S_IWUSR | S_IRUGO, get_mode_reset, set_mode_reset, MODULE_RESET_##index)
//#define DECLARE_TRANSCEIVER_RESET_ATTR(index)  &sensor_dev_attr_module_reset_##index.dev_attr.attr
#define DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(index) \
	static SENSOR_DEVICE_ATTR(module_reset_##index, S_IWUSR | S_IRUGO, sfp_value_show, sfp_value_store, MODULE_RESET_##index)
#define DECLARE_TRANSCEIVER_RESET_ATTR(index)  &sensor_dev_attr_module_reset_##index.dev_attr.attr




static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);
/* transceiver attributes */
static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, show_present_all, NULL, MODULE_PRESENT_ALL);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(1);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(2);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(3);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(4);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(5);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(6);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(7);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(8);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(9);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(10);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(11);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(12);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(13);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(14);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(15);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(16);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(17);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(18);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(19);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(20);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(21);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(22);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(23);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(24);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(25);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(26);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(27);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(28);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(29);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(30);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(31);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(32);

DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(1);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(2);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(3);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(4);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(5);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(6);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(7);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(8);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(9);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(10);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(11);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(12);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(13);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(14);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(15);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(16);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(17);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(18);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(19);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(20);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(21);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(22);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(23);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(24);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(25);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(26);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(27);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(28);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(29);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(30);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(31);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_RESET_ATTR(32);


static struct attribute *as7716_32xb_cpld_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    /* transceiver attributes */
    &sensor_dev_attr_module_present_all.dev_attr.attr,
    DECLARE_TRANSCEIVER_ATTR(1),
    DECLARE_TRANSCEIVER_ATTR(2),
    DECLARE_TRANSCEIVER_ATTR(3),
    DECLARE_TRANSCEIVER_ATTR(4),
    DECLARE_TRANSCEIVER_ATTR(5),
    DECLARE_TRANSCEIVER_ATTR(6),
    DECLARE_TRANSCEIVER_ATTR(7),
    DECLARE_TRANSCEIVER_ATTR(8),
    DECLARE_TRANSCEIVER_ATTR(9),
    DECLARE_TRANSCEIVER_ATTR(10),
    DECLARE_TRANSCEIVER_ATTR(11),
    DECLARE_TRANSCEIVER_ATTR(12),
    DECLARE_TRANSCEIVER_ATTR(13),
    DECLARE_TRANSCEIVER_ATTR(14),
    DECLARE_TRANSCEIVER_ATTR(15),
    DECLARE_TRANSCEIVER_ATTR(16),
    DECLARE_TRANSCEIVER_ATTR(17),
    DECLARE_TRANSCEIVER_ATTR(18),
    DECLARE_TRANSCEIVER_ATTR(19),
    DECLARE_TRANSCEIVER_ATTR(20),
    DECLARE_TRANSCEIVER_ATTR(21),
    DECLARE_TRANSCEIVER_ATTR(22),
    DECLARE_TRANSCEIVER_ATTR(23),
    DECLARE_TRANSCEIVER_ATTR(24),
    DECLARE_TRANSCEIVER_ATTR(25),
    DECLARE_TRANSCEIVER_ATTR(26),
    DECLARE_TRANSCEIVER_ATTR(27),
    DECLARE_TRANSCEIVER_ATTR(28),
    DECLARE_TRANSCEIVER_ATTR(29),
    DECLARE_TRANSCEIVER_ATTR(30),
    DECLARE_TRANSCEIVER_ATTR(31),
    DECLARE_TRANSCEIVER_ATTR(32),
    DECLARE_TRANSCEIVER_RESET_ATTR(1),
    DECLARE_TRANSCEIVER_RESET_ATTR(2),
    DECLARE_TRANSCEIVER_RESET_ATTR(3),
    DECLARE_TRANSCEIVER_RESET_ATTR(4),
    DECLARE_TRANSCEIVER_RESET_ATTR(5),
    DECLARE_TRANSCEIVER_RESET_ATTR(6),
    DECLARE_TRANSCEIVER_RESET_ATTR(7),
    DECLARE_TRANSCEIVER_RESET_ATTR(8),
    DECLARE_TRANSCEIVER_RESET_ATTR(9),
    DECLARE_TRANSCEIVER_RESET_ATTR(10),
    DECLARE_TRANSCEIVER_RESET_ATTR(11),
    DECLARE_TRANSCEIVER_RESET_ATTR(12),
    DECLARE_TRANSCEIVER_RESET_ATTR(13),
    DECLARE_TRANSCEIVER_RESET_ATTR(14),
    DECLARE_TRANSCEIVER_RESET_ATTR(15),
    DECLARE_TRANSCEIVER_RESET_ATTR(16),
    DECLARE_TRANSCEIVER_RESET_ATTR(17),
    DECLARE_TRANSCEIVER_RESET_ATTR(18),
    DECLARE_TRANSCEIVER_RESET_ATTR(19),
    DECLARE_TRANSCEIVER_RESET_ATTR(20),
    DECLARE_TRANSCEIVER_RESET_ATTR(21),
    DECLARE_TRANSCEIVER_RESET_ATTR(22),
    DECLARE_TRANSCEIVER_RESET_ATTR(23),
    DECLARE_TRANSCEIVER_RESET_ATTR(24),
    DECLARE_TRANSCEIVER_RESET_ATTR(25),
    DECLARE_TRANSCEIVER_RESET_ATTR(26),
    DECLARE_TRANSCEIVER_RESET_ATTR(27),
    DECLARE_TRANSCEIVER_RESET_ATTR(28),
    DECLARE_TRANSCEIVER_RESET_ATTR(29),
    DECLARE_TRANSCEIVER_RESET_ATTR(30),
    DECLARE_TRANSCEIVER_RESET_ATTR(31),
    DECLARE_TRANSCEIVER_RESET_ATTR(32),
    NULL
};

static const struct attribute_group as7716_32xb_cpld_group = {
    .attrs = as7716_32xb_cpld_attributes,
};

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
                                char *buf)
{
    int i, status;
    u8 values[4]  = {0};
    u8 regs[] = {0x30, 0x31, 0x32, 0x33};
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_cpld_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    for (i = 0; i < ARRAY_SIZE(regs); i++) {
        status = as7716_32xb_cpld_read_internal(client, regs[i]);

        if (status < 0) {
            goto exit;
        }

        values[i] = ~(u8)status;
    }

    mutex_unlock(&data->update_lock);

    /* Return values 1 -> 32 in order */
    return sprintf(buf, "%.2x %.2x %.2x %.2x\n",
                   values[0], values[1], values[2],
                   values[3]);

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static int sfp_array_index_get(int attr_idx)
{
    switch(attr_idx)
    {
    case TRANSCEIVER_PRESENT_ATTR_ID(1):
    case TRANSCEIVER_RESET_ATTR_ID(1):
        return PORT1_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(2):
    case TRANSCEIVER_RESET_ATTR_ID(2):
        return PORT2_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(3):
    case TRANSCEIVER_RESET_ATTR_ID(3):
        return PORT3_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(4):
    case TRANSCEIVER_RESET_ATTR_ID(4):
        return PORT4_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(5):
    case TRANSCEIVER_RESET_ATTR_ID(5):
        return PORT5_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(6):
    case TRANSCEIVER_RESET_ATTR_ID(6):
        return PORT6_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(7):
    case TRANSCEIVER_RESET_ATTR_ID(7):
        return PORT7_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(8):
    case TRANSCEIVER_RESET_ATTR_ID(8):
        return PORT8_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(9):
    case TRANSCEIVER_RESET_ATTR_ID(9):
        return PORT9_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(10):
    case TRANSCEIVER_RESET_ATTR_ID(10):
        return PORT10_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(11):
    case TRANSCEIVER_RESET_ATTR_ID(11):
        return PORT11_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(12):
    case TRANSCEIVER_RESET_ATTR_ID(12):
        return PORT12_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(13):
    case TRANSCEIVER_RESET_ATTR_ID(13):
        return PORT13_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(14):
    case TRANSCEIVER_RESET_ATTR_ID(14):
        return PORT14_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(15):
    case TRANSCEIVER_RESET_ATTR_ID(15):
        return PORT15_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(16):
    case TRANSCEIVER_RESET_ATTR_ID(16):
        return PORT16_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(17):
    case TRANSCEIVER_RESET_ATTR_ID(17):
        return PORT17_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(18):
    case TRANSCEIVER_RESET_ATTR_ID(18):
        return PORT18_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(19):
    case TRANSCEIVER_RESET_ATTR_ID(19):
        return PORT19_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(20):
    case TRANSCEIVER_RESET_ATTR_ID(20):
        return PORT20_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(21):
    case TRANSCEIVER_RESET_ATTR_ID(21):
        return PORT21_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(22):
    case TRANSCEIVER_RESET_ATTR_ID(22):
        return PORT22_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(23):
    case TRANSCEIVER_RESET_ATTR_ID(23):
        return PORT23_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(24):
    case TRANSCEIVER_RESET_ATTR_ID(24):
        return PORT24_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(25):
    case TRANSCEIVER_RESET_ATTR_ID(25):
        return PORT25_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(26):
    case TRANSCEIVER_RESET_ATTR_ID(26):
        return PORT26_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(27):
    case TRANSCEIVER_RESET_ATTR_ID(27):
        return PORT27_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(28):
    case TRANSCEIVER_RESET_ATTR_ID(28):
        return PORT28_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(29):
    case TRANSCEIVER_RESET_ATTR_ID(29):
        return PORT29_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(30):
    case TRANSCEIVER_RESET_ATTR_ID(30):
        return PORT30_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(31):
    case TRANSCEIVER_RESET_ATTR_ID(31):
        return PORT31_ID;
    case TRANSCEIVER_PRESENT_ATTR_ID(32):
    case TRANSCEIVER_RESET_ATTR_ID(32):
        return PORT32_ID;
    default :
        return -1;
    }
}


static ssize_t sfp_value_store(struct device *dev, struct device_attribute *da,
                               const char *buf, size_t size)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_cpld_data *data = i2c_get_clientdata(client);
    int status = -EINVAL;
    int index;
    long keyin = 0;

    mutex_lock(&data->update_lock);
    switch (attr->index)
    {
    case MODULE_PRESENT_1 ... MODULE_PRESENT_8:
    case MODULE_PRESENT_9 ... MODULE_PRESENT_16:
    case MODULE_PRESENT_17 ... MODULE_PRESENT_24:
    case MODULE_PRESENT_25 ... MODULE_PRESENT_32:
        index=sfp_array_index_get(attr->index);
        if(index < 0 || index > PORT_NUM_MAX -1)
            break;
        status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
        if(keyin < 0 || keyin > 1)
            break;
        data->present[index]=keyin;
        break;
    case MODULE_RESET_1 ... MODULE_RESET_8:
    case MODULE_RESET_9 ... MODULE_RESET_16:
    case MODULE_RESET_17 ... MODULE_RESET_24:
    case MODULE_RESET_25 ... MODULE_RESET_32:
        index=sfp_array_index_get(attr->index);
        if(index < 0 || index > PORT_NUM_MAX -1)
            break;
        status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
        if(keyin < 0 || keyin > 1)
            break;
        data->reset[index]=keyin;
        break;
    default :
        break;
    }
    mutex_unlock(&data->update_lock);
    return size;
}
static ssize_t sfp_value_show(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_cpld_data *data = i2c_get_clientdata(client);
    ssize_t ret = 0;
    int index;
    int status = -EINVAL;

    mutex_lock(&data->update_lock);
    switch (attr->index)
    {
    case MODULE_PRESENT_1 ... MODULE_PRESENT_8:
    case MODULE_PRESENT_9 ... MODULE_PRESENT_16:
    case MODULE_PRESENT_17 ... MODULE_PRESENT_24:
    case MODULE_PRESENT_25 ... MODULE_PRESENT_32:
        index=sfp_array_index_get(attr->index);
        if(index < 0 || index > PORT_NUM_MAX -1)
            break;
        status = sprintf(buf, "%u\n", data->present[index]);
        break;
    case MODULE_RESET_1 ... MODULE_RESET_8:
    case MODULE_RESET_9 ... MODULE_RESET_16:
    case MODULE_RESET_17 ... MODULE_RESET_24:
    case MODULE_RESET_25 ... MODULE_RESET_32:
        index=sfp_array_index_get(attr->index);
        if(index < 0 || index > PORT_NUM_MAX -1)
            break;
        status = sprintf(buf, "%u\n", data->reset[index]);
        break;
    default :
        break;
    }

    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_present(struct device *dev, struct device_attribute *da,
                            char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_cpld_data *data = i2c_get_clientdata(client);
    int status = 0;
    u8 reg = 0, mask = 0;

    switch (attr->index) {
    case MODULE_PRESENT_1 ... MODULE_PRESENT_8:
        reg  = 0x30;
        mask = 0x1 << (attr->index - MODULE_PRESENT_1);
        break;
    case MODULE_PRESENT_9 ... MODULE_PRESENT_16:
        reg  = 0x31;
        mask = 0x1 << (attr->index - MODULE_PRESENT_9);
        break;
    case MODULE_PRESENT_17 ... MODULE_PRESENT_24:
        reg  = 0x32;
        mask = 0x1 << (attr->index - MODULE_PRESENT_17);
        break;
    case MODULE_PRESENT_25 ... MODULE_PRESENT_32:
        reg  = 0x33;
        mask = 0x1 << (attr->index - MODULE_PRESENT_25);
        break;
    default:
        return 0;
    }


    mutex_lock(&data->update_lock);
    status = as7716_32xb_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", !(status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_version(struct device *dev, struct device_attribute *da,
                            char *buf)
{
    u8 reg = 0, mask = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_cpld_data *data = i2c_get_clientdata(client);
    int status = 0;

    switch (attr->index) {
    case CPLD_VERSION:
        reg  = 0x1;
        mask = 0xFF;
        break;
    default:
        break;
    }

    mutex_lock(&data->update_lock);
    status = as7716_32xb_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%d\n", (status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t access(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count)
{
    int status;
    u32 addr, val;
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_cpld_data *data = i2c_get_clientdata(client);

    if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
        return -EINVAL;
    }

    if (addr > 0xFF || val > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = as7716_32xb_cpld_write_internal(client, addr, val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static int as7716_32xb_cpld_read_internal(struct i2c_client *client, u8 reg)
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

static int as7716_32xb_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
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

static void as7716_32xb_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);

    if (!node) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void as7716_32xb_cpld_remove_client(struct i2c_client *client)
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

static int as7716_32xb_cpld_probe(struct i2c_client *client,
                                  const struct i2c_device_id *dev_id)
{
    int status;
    struct as7716_32xb_cpld_data *data = NULL;

    data = kzalloc(sizeof(struct as7716_32xb_cpld_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as7716_32xb_cpld_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    as7716_32xb_cpld_add_client(client);

    dev_info(&client->dev, "%s: cpld '%s'\n",
             dev_name(data->hwmon_dev), client->name);

    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_cpld_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void as7716_32xb_cpld_remove(struct i2c_client *client)
{
    struct as7716_32xb_cpld_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_cpld_group);
    kfree(data);
    as7716_32xb_cpld_remove_client(client);

}

int as7716_32xb_cpld_read(unsigned short cpld_addr, u8 reg)
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
EXPORT_SYMBOL(as7716_32xb_cpld_read);

int as7716_32xb_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
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
EXPORT_SYMBOL(as7716_32xb_cpld_write);

static ssize_t get_mode_reset(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_cpld_data *data = i2c_get_clientdata(client);
    int status = 0;
    u8 reg = 0, mask = 0;

    switch (attr->index) {
    case MODULE_RESET_1 ... MODULE_RESET_8:
        reg  = 0x04;
        mask = 0x1 << (attr->index - MODULE_RESET_1);
        break;
    case MODULE_RESET_9 ... MODULE_RESET_16:
        reg  = 0x05;
        mask = 0x1 << (attr->index - MODULE_RESET_9);
        break;
    case MODULE_RESET_17 ... MODULE_RESET_24:
        reg  = 0x06;
        mask = 0x1 << (attr->index - MODULE_RESET_17);
        break;
    case MODULE_RESET_25 ... MODULE_RESET_32:
        reg  = 0x07;
        mask = 0x1 << (attr->index - MODULE_RESET_25);
        break;
    default:
        return 0;
    }


    mutex_lock(&data->update_lock);
    status = as7716_32xb_cpld_read_internal(client, reg);

    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\r\n", !(status & mask));

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_cpld_data *data = i2c_get_clientdata(client);
    long reset;
    int status=0, val, error;
    u8 reg = 0, mask = 0;


    error = kstrtol(buf, 10, &reset);
    if (error) {
        return error;
    }
    switch (attr->index) {
    case MODULE_RESET_1 ... MODULE_RESET_8:
        reg  = 0x04;
        mask = 0x1 << (attr->index - MODULE_RESET_1);
        break;
    case MODULE_RESET_9 ... MODULE_RESET_16:
        reg  = 0x05;
        mask = 0x1 << (attr->index - MODULE_RESET_9);
        break;
    case MODULE_RESET_17 ... MODULE_RESET_24:
        reg  = 0x06;
        mask = 0x1 << (attr->index - MODULE_RESET_17);
        break;
    case MODULE_RESET_25 ... MODULE_RESET_32:
        reg  = 0x07;
        mask = 0x1 << (attr->index - MODULE_RESET_25);
        break;
    default:
        return 0;
    }
    mutex_lock(&data->update_lock);

    status = as7716_32xb_cpld_read_internal(client, reg);
    if (unlikely(status < 0)) {
        goto exit;
    }
    /* Update lp_mode status */
    if (reset)
    {
        val = status&(~mask);
    }
    else
    {
        val =status | (mask);
    }

    status = as7716_32xb_cpld_write_internal(client, reg, val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}



static const struct i2c_device_id as7716_32xb_cpld_id[] = {
    { "as7716_32xb_cpld1", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, as7716_32xb_cpld_id);

static struct i2c_driver as7716_32xb_cpld_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as7716_32xb_cpld1",
    },
    .probe        = as7716_32xb_cpld_probe,
    .remove       = as7716_32xb_cpld_remove,
    .id_table     = as7716_32xb_cpld_id,
    .address_list = normal_i2c,
};

static int __init as7716_32xb_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&as7716_32xb_cpld_driver);
}

static void __exit as7716_32xb_cpld_exit(void)
{
    i2c_del_driver(&as7716_32xb_cpld_driver);
}

module_init(as7716_32xb_cpld_init);
module_exit(as7716_32xb_cpld_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as7716_32xb_cpld driver");
MODULE_LICENSE("GPL");

