/*
 * Copyright (C) 1998, 1999  Frodo Looijaard <frodol@dds.nl> and
 *                           Philip Edelbrock <phil@netroedge.com>
 * Copyright (C) 2003 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2003 IBM Corp.
 * Copyright (C) 2004 Jean Delvare <jdelvare@suse.de>
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
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/capability.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>

/* Size of EEPROM in bytes */
#define EEPROM_SIZE     256

#define VENDOR_NAME_LEN              8
#define VENDOR_NAME_START_BYTE              0xC

#define FSP_PRODUCT_NAME_LEN             8
#define FSP_PRODUCT_OPTION_NAME_LEN     11
#define FSP_SERIAL_NUMBER_LEN           19
#define FSP_HW_REVISION_LEN              3

#define FSP_PRODUCT_NAME_START_BYTE            0x20
#define FSP_PRODUCT_OPTION_NAME_START_BYTE     0x29
#define FSP_SERIAL_NUMBER_START_BYTE           0x35
#define FSP_HW_REVISION_START_BYTE             0x57


#define GOS_PRODUCT_NAME_LEN            13
#define GOS_SERIAL_NUMBER_LEN           19
#define GOS_HW_REVISION_LEN              3

#define GOS_PRODUCT_NAME_START_BYTE            0x15
#define GOS_SERIAL_NUMBER_START_BYTE           0x3d
#define GOS_HW_REVISION_START_BYTE             0x39

#define GOS_STRING	"GOSPOWER"

enum psu_vendor {
    PSU_FSP = 0,
    PSU_GOS,
};

/* Each client has this additional data */
struct eeprom_data {
    struct mutex update_lock;
    u8 valid;           /* bitfield, bit!=0 if slice is valid */
    unsigned long last_updated[8];  /* In jiffies, 8 slices */
    u8 data[EEPROM_SIZE];       /* Register values */
	u8 vendor_code;
};

static ssize_t get_vendor_name(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct eeprom_data *data = i2c_get_clientdata(client);
    int i;

    for (i = VENDOR_NAME_START_BYTE ; i < (VENDOR_NAME_START_BYTE + VENDOR_NAME_LEN) ; i++)
        sprintf(buf + strlen(buf), "%c", data->data[i]);
    sprintf(buf + strlen(buf), "\n");
    return (ssize_t)strlen(buf);;
}

static ssize_t get_product_name(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct eeprom_data *data = i2c_get_clientdata(client);
    int i;

	if (data->vendor_code == PSU_FSP) {
		for (i = FSP_PRODUCT_NAME_START_BYTE ; i < (FSP_PRODUCT_NAME_START_BYTE + FSP_PRODUCT_NAME_LEN) ; i++)
			sprintf(buf + strlen(buf), "%c", data->data[i]);
		for (i = FSP_PRODUCT_OPTION_NAME_START_BYTE ; i < (FSP_PRODUCT_OPTION_NAME_START_BYTE + FSP_PRODUCT_OPTION_NAME_LEN) ; i++)
			sprintf(buf + strlen(buf), "%c", data->data[i]);
	} else {
		for (i = GOS_PRODUCT_NAME_START_BYTE ; i < (GOS_PRODUCT_NAME_START_BYTE + GOS_PRODUCT_NAME_LEN) ; i++)
			sprintf(buf + strlen(buf), "%c", data->data[i]);
	}
    sprintf(buf + strlen(buf), "\n");
    return (ssize_t)strlen(buf);;
}

static ssize_t get_serial_number(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct eeprom_data *data = i2c_get_clientdata(client);
    int i;

	if (data->vendor_code == PSU_FSP) {
		for (i = FSP_SERIAL_NUMBER_START_BYTE ; i < (FSP_SERIAL_NUMBER_START_BYTE + FSP_SERIAL_NUMBER_LEN) ; i++)
			sprintf(buf + strlen(buf), "%c", data->data[i]);
	} else {
		for (i = GOS_SERIAL_NUMBER_START_BYTE ; i < (GOS_SERIAL_NUMBER_START_BYTE + GOS_SERIAL_NUMBER_LEN) ; i++)
			sprintf(buf + strlen(buf), "%c", data->data[i]);
	}

    sprintf(buf + strlen(buf), "\n");
    return (ssize_t)strlen(buf);;
}

static ssize_t get_hw_revision(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct eeprom_data *data = i2c_get_clientdata(client);
    int i;

	if (data->vendor_code == PSU_FSP) {
		for (i = FSP_HW_REVISION_START_BYTE ; i < (FSP_HW_REVISION_START_BYTE + FSP_HW_REVISION_LEN) ; i++)
			sprintf(buf + strlen(buf), "%c", data->data[i]);
	} else {
		for (i = GOS_HW_REVISION_START_BYTE ; i < (GOS_HW_REVISION_START_BYTE + GOS_HW_REVISION_LEN) ; i++)
			sprintf(buf + strlen(buf), "%c", data->data[i]);
	}

    sprintf(buf + strlen(buf), "\n");
    return (ssize_t)strlen(buf);;
}

static DEVICE_ATTR(vendor_name, S_IRUGO, get_vendor_name, NULL);
static DEVICE_ATTR(product_name, S_IRUGO, get_product_name, NULL);
static DEVICE_ATTR(serial_number, S_IRUGO, get_serial_number, NULL);
static DEVICE_ATTR(hw_revision, S_IRUGO, get_hw_revision, NULL);

static struct attribute *_i2c_eeprom_attr[] = {
    &dev_attr_vendor_name.attr,
    &dev_attr_product_name.attr,
    &dev_attr_serial_number.attr,
    &dev_attr_hw_revision.attr,
    NULL
};

static const struct attribute_group _i2c_eeprom_attr_grp = {
    .attrs = _i2c_eeprom_attr,
};

static void eeprom_update_client(struct i2c_client *client, u8 slice)
{
    struct eeprom_data *data = i2c_get_clientdata(client);
    int i;

    mutex_lock(&data->update_lock);

    if (!(data->valid & (1 << slice)) ||
        time_after(jiffies, data->last_updated[slice] + 300 * HZ)) {
        dev_dbg(&client->dev, "Starting eeprom update, slice %u\n", slice);

        if (i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
            for (i = slice << 5; i < (slice + 1) << 5; i += 24)
                if (i2c_smbus_read_i2c_block_data(client, i,
                            24, data->data + i)
                            != 24)
                    goto exit;
        } else {
            for (i = slice << 5; i < (slice + 1) << 5; i += 2) {
                int word = i2c_smbus_read_word_data(client, i);
                if (word < 0)
                    goto exit;
                data->data[i] = word & 0xff;
                data->data[i + 1] = word >> 8;
            }
        }
        data->last_updated[slice] = jiffies;
        data->valid |= (1 << slice);
    }
exit:
    mutex_unlock(&data->update_lock);
}

static ssize_t eeprom_read(struct file *filp, struct kobject *kobj,
               struct bin_attribute *bin_attr,
               char *buf, loff_t off, size_t count)
{
    struct i2c_client *client = to_i2c_client(kobj_to_dev(kobj));
    struct eeprom_data *data = i2c_get_clientdata(client);
    u8 slice;

    /* Only refresh slices which contain requested bytes */
    for (slice = off >> 5; slice <= (off + count - 1) >> 5; slice++)
        eeprom_update_client(client, slice);

    memcpy(buf, &data->data[off], count);

    return count;
}


static struct bin_attribute eeprom_attr = {
    .attr = {
        .name = "eeprom",
        .mode = S_IRUGO,
    },
    .size = EEPROM_SIZE,
    .read = eeprom_read,
};

static int eeprom_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{
    struct eeprom_data *data;
    int err;
    u8 slice;

    if (!(data = kzalloc(sizeof(struct eeprom_data), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }

    memset(data->data, 0xff, EEPROM_SIZE);
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    /* create the sysfs eeprom file */
    err = sysfs_create_bin_file(&client->dev.kobj, &eeprom_attr);
    if (err) {
        goto exit_kfree;
    }

    /* perform dummy read eeprom*/
    for (slice = 0 >> 5; slice <= (EEPROM_SIZE - 1) >> 5; slice++)
        eeprom_update_client(client, slice);

	data->vendor_code = PSU_FSP;

	if (data->data[VENDOR_NAME_START_BYTE] == 0x47)
		data->vendor_code = PSU_GOS;

    err = sysfs_create_group(&client->dev.kobj, &_i2c_eeprom_attr_grp);
    if (err) {
        dev_dbg(&client->dev, "Exit, failed to create sysfs, rc = 0x%x", err);
        goto exit_kfree;
    }
    return 0;

exit_kfree:
    kfree(data);
exit:
    return err;
}

static void eeprom_remove(struct i2c_client *client)
{
    sysfs_remove_bin_file(&client->dev.kobj, &eeprom_attr);
    kfree(i2c_get_clientdata(client));

}

static const struct i2c_device_id eeprom_id[] = {
    { "wistron_eeprom", 0 },
    { }
};

static struct i2c_driver eeprom_driver = {
    .driver = {
        .name   = "wistron_eeprom",
    },
    .probe      = eeprom_probe,
    .remove     = eeprom_remove,
    .id_table   = eeprom_id,
};

module_i2c_driver(eeprom_driver);

MODULE_AUTHOR("switchsupport@wistron.com");
MODULE_DESCRIPTION ("SONiC platform driver for PSU EEPROM");
MODULE_LICENSE("GPL");
