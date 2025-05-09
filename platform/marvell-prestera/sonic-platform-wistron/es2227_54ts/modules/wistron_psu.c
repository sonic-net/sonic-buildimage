/*
 * Wistron Generic PMBUS driver
 *
 *
 * Based on pmbus_core driver and ltc2978 driver
 *
 * Author:
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
#include <linux/delay.h>
#include <linux/pmbus.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include "pmbus.h"

static int pmbus_read_block_data(struct i2c_client *client, u8 command, u8 *data)
{
    int result = 0;
    int retry_count = 3;

    while (retry_count) {
        retry_count--;

        result = i2c_smbus_read_i2c_block_data(client, command, I2C_SMBUS_BLOCK_MAX, data);

        if (result < 0) {
            msleep(10);
            continue;
        }

        result = 0;
        break;
    }

    return result;
}

/* ----------------------------------------------------------------------------
 * export function for specified use
 * ----------------------------------------------------------------------------
 */

static ssize_t wistron_psu_get_vendor(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = pmbus_read_block_data(client, PMBUS_MFR_ID, block_buffer);
    if (ret < 0) {
        dev_dbg(&client->dev, "Failed to read Manufacturer ID\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);

}

static ssize_t wistron_psu_get_model(struct device *dev,
                struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = pmbus_read_block_data(client, PMBUS_MFR_MODEL, block_buffer);
    if (ret < 0) {
        dev_dbg(&client->dev, "Failed to read Manufacturer Model\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);
}

static ssize_t wistron_psu_get_version(struct device *dev,
                struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = pmbus_read_block_data(client, PMBUS_MFR_REVISION, block_buffer);
    if (ret < 0) {
        dev_dbg(&client->dev, "Failed to read Manufacturer Revision\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);
}

static ssize_t wistron_psu_get_location(struct device *dev,
                struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = pmbus_read_block_data(client, PMBUS_MFR_LOCATION, block_buffer);
    if (ret < 0) {
        dev_dbg(&client->dev, "Failed to read Manufacture Location\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);
}

static ssize_t wistron_psu_get_serial(struct device *dev,
                struct device_attribute *da, char *buf)
{
    int ret, len;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1], *str;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    ret = pmbus_read_block_data(client, PMBUS_MFR_SERIAL, block_buffer);
    if (ret < 0) {
        dev_dbg(&client->dev, "Failed to read Manufacturer Serial\n");
        return ret;
    }
    len = block_buffer[0];
    block_buffer[(len+1)] = '\0';
    str = &(block_buffer[1]);

    return snprintf(buf, PAGE_SIZE, "%s\n", str);
}

static DEVICE_ATTR(mfr_id, S_IRUGO, wistron_psu_get_vendor, NULL);
static DEVICE_ATTR(mfr_model, S_IRUGO, wistron_psu_get_model, NULL);
static DEVICE_ATTR(mfr_revision, S_IRUGO, wistron_psu_get_version, NULL);
static DEVICE_ATTR(mfr_location, S_IRUGO, wistron_psu_get_location, NULL);
static DEVICE_ATTR(mfr_serial, S_IRUGO, wistron_psu_get_serial, NULL);

static struct attribute *wistron_pmbus_attributes[] = {
    &dev_attr_mfr_id.attr,
    &dev_attr_mfr_model.attr,
    &dev_attr_mfr_revision.attr,
    &dev_attr_mfr_location.attr,
    &dev_attr_mfr_serial.attr,
    NULL
};

static const struct attribute_group wistron_pmbus_group = {
    .attrs = wistron_pmbus_attributes,
};

static const struct i2c_device_id pmbus_id[] = {
    {"wistron_psu", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, pmbus_id);

struct pmbus_driver_info wistron_psu_info = {
    .pages = 1,
    .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN |
        PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT |
        PMBUS_HAVE_FAN12 | PMBUS_HAVE_STATUS_FAN12 |
        PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 |
        PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP |
        PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
        PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
        PMBUS_HAVE_POUT,
};

static int pmbus_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
    struct device *dev = &client->dev;
    int ret;

    dev_info(dev, "wistron-psu pmbus_probe\n");

    if (!i2c_check_functionality(client->adapter,
            I2C_FUNC_SMBUS_READ_WORD_DATA))
        return -ENODEV;

    /* Register sysfs hooks */
    ret = sysfs_create_group(&client->dev.kobj, &wistron_pmbus_group);
    if (ret) {
        dev_err(dev, "Failed to create sysfs entries\n");
        goto exit;
    }

	return pmbus_do_probe(client, &wistron_psu_info);

exit:
    return ret;
}

static struct i2c_driver pmbus_driver = {
    .driver = {
        .name = "wistron-psu",
    },
    .probe = pmbus_probe,
    .id_table = pmbus_id,
};

static int __init pmbus_driver_init(void)
{
    return i2c_add_driver(&pmbus_driver);
}

static void __exit pmbus_driver_exit(void)
{
    i2c_del_driver(&pmbus_driver);
}

module_init(pmbus_driver_init);
module_exit(pmbus_driver_exit);

MODULE_AUTHOR("Wistron");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("SONiC platform driver for Wistron PSU");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(PMBUS);
