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

/* enable dev_dbg print out */
//#define DEBUG

#define __STDC_WANT_LIB_EXT1__ 1
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>

/* Addresses to scan */
static const unsigned short normal_i2c[] = { /*0x50, 0x51, 0x52, 0x53, 0x54,
                    0x55, 0x56, 0x57,*/ I2C_CLIENT_END };

/* Size of EEPROM in bytes */
#define EEPROM_SIZE     512

#define SLICE_BITS      (6)
#define SLICE_SIZE      (1 << SLICE_BITS)
#define SLICE_NUM       (EEPROM_SIZE/SLICE_SIZE)

/* Each client has this additional data */
struct eeprom_data {
    struct mutex update_lock;
    u8 valid;           /* bitfield, bit!=0 if slice is valid */
    unsigned long last_updated[SLICE_NUM];  /* In jiffies, 8 slices */
    u8 data[EEPROM_SIZE];       /* Register values */
};


static void sys_eeprom_update_client(struct i2c_client *client, u8 slice)
{
    struct eeprom_data *data = i2c_get_clientdata(client);
    int i, j;
    int ret;
    int addr;

    mutex_lock(&data->update_lock);

    if (!(data->valid & (1 << slice)) ||
        time_after(jiffies, data->last_updated[slice] + 300 * HZ)) {
        dev_dbg(&client->dev, "Starting eeprom update, slice %u\n", slice);

        addr = slice << SLICE_BITS;

        ret = i2c_smbus_write_byte_data(client, (u8)((addr >> 8) & 0xFF), (u8)(addr & 0xFF));
        /* select the eeprom address */
        if (ret < 0) {
            dev_err(&client->dev, "address set failed\n");
            goto exit;
        }

        if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_BYTE)) {
            goto exit;
        }

        for (i = slice << SLICE_BITS; i < (slice + 1) << SLICE_BITS; i+= SLICE_SIZE) {
            for (j = i; j < (i+SLICE_SIZE); j++) {
                int res;

                res = i2c_smbus_read_byte(client);
                if (res < 0) {
                    goto exit;
                }

                data->data[j] = res & 0xFF;
            }
        }

        data->last_updated[slice] = jiffies;
        data->valid |= (1 << slice);
    }
exit:
    mutex_unlock(&data->update_lock);
}

static ssize_t sys_eeprom_read(struct file *filp, struct kobject *kobj,
               struct bin_attribute *bin_attr,
               char *buf, loff_t off, size_t count)
{
    struct i2c_client *client = to_i2c_client(container_of(kobj, struct device, kobj));
    struct eeprom_data *data = i2c_get_clientdata(client);
    u8 slice;

    if (off > EEPROM_SIZE) {
        return 0;
    }
    if (off + count > EEPROM_SIZE) {
        count = EEPROM_SIZE - off;
    }
    if (count == 0) {
        return 0;
    }

    /* Only refresh slices which contain requested bytes */
    for (slice = off >> SLICE_BITS; slice <= (off + count - 1) >> SLICE_BITS; slice++) {
        sys_eeprom_update_client(client, slice);
    }

    memcpy(buf, &data->data[off], count);

    return count;
}

static ssize_t sys_eeprom_write(struct file *filp, struct kobject *kobj,
                       struct bin_attribute *bin_attr,
                       char *buf, loff_t off, size_t count)
{
    struct i2c_client *client = to_i2c_client(container_of(kobj, struct device, kobj));
    struct eeprom_data *data = i2c_get_clientdata(client);
    int ret;
    int i;
    u8 cmd;
    u16 value16;

    dev_dbg(&client->dev, "sys_eeprom_write off=%d, count=%d\n", (int)off, (int)count);

    if (off > EEPROM_SIZE) {
        return 0;
    }
    if (off + count > EEPROM_SIZE) {
        count = EEPROM_SIZE - off;
    }
    if (count == 0) {
        return 0;
    }

    mutex_lock(&data->update_lock);

    for(i=0; i < count; i++) {
        /* write command */
        cmd = (off >> 8) & 0xff;
        value16 = off & 0xff;
        value16 |= buf[i] << 8;
        ret = i2c_smbus_write_word_data(client, cmd, value16);

        if (ret < 0) {
            dev_err(&client->dev, "write address failed at %d \n", (int)off);
            goto exit;
        }

        off++;

        /* need to wait for write complete */
        udelay(10000);
    }
exit:
    mutex_unlock(&data->update_lock);
    /* force to update client when reading */
    for(i=0; i < SLICE_NUM; i++) {
        data->last_updated[i] = 0;
    }

    return count;
}

static struct bin_attribute sys_eeprom_attr = {
    .attr = {
        .name = "eeprom",
        .mode = S_IRUGO | S_IWUSR,
    },
    .size = EEPROM_SIZE,
    .read = sys_eeprom_read,
    .write = sys_eeprom_write,
};

/* Return 0 if detection is successful, -ENODEV otherwise */
static int sys_eeprom_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;

    /* EDID EEPROMs are often 24C00 EEPROMs, which answer to all
       addresses 0x50-0x57, but we only care about 0x51 and 0x55. So decline
       attaching to addresses >= 0x56 on DDC buses */
    if (!(adapter->class & I2C_CLASS_SPD) && client->addr >= 0x56) {
        return -ENODEV;
    }

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_BYTE)
     && !i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
        return -ENODEV;
    }

    strlcpy(info->type, "eeprom", I2C_NAME_SIZE);

    return 0;
}

static int sys_eeprom_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{
    struct eeprom_data *data;
    int err;

    if (!(data = kzalloc(sizeof(struct eeprom_data), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }

#ifdef __STDC_LIB_EXT1__
    memset_s(data->data, EEPROM_SIZE, 0xff, EEPROM_SIZE);
#else
    memset(data->data, 0xff, EEPROM_SIZE);
#endif
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    /* create the sysfs eeprom file */
    err = sysfs_create_bin_file(&client->dev.kobj, &sys_eeprom_attr);
    if (err) {
        goto exit_kfree;
    }

    return 0;

exit_kfree:
    kfree(data);
exit:
    return err;
}

static int sys_eeprom_remove(struct i2c_client *client)
{
    sysfs_remove_bin_file(&client->dev.kobj, &sys_eeprom_attr);
    kfree(i2c_get_clientdata(client));

    return 0;
}

static const struct i2c_device_id sys_eeprom_id[] = {
    { "sys_eeprom", 0 },
    { }
};

static struct i2c_driver sys_eeprom_driver = {
    .driver = {
        .name   = "sys_eeprom",
    },
    .probe      = sys_eeprom_probe,
    .remove     = sys_eeprom_remove,
    .id_table   = sys_eeprom_id,

    .class      = I2C_CLASS_DDC | I2C_CLASS_SPD,
    .detect     = sys_eeprom_detect,
    .address_list   = normal_i2c,
};

module_i2c_driver(sys_eeprom_driver);

MODULE_AUTHOR("Nonodark Huang <nonodark.huang@ufispace.com>");
MODULE_DESCRIPTION("UfiSpace System EEPROM driver");
MODULE_LICENSE("GPL");
