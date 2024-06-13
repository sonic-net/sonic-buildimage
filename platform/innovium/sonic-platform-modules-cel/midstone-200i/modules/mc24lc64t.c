/*
 * mc24lc64t.c - driver for Microchip 24LC64T
 *
 * Copyright (C) 2017 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

struct mc24lc64t_data {
        struct i2c_client       *fake_client;
        struct mutex            update_lock;
};

static ssize_t mc24lc64t_read(struct file *filp, struct kobject *kobj,
                            struct bin_attribute *bin_attr,
                            char *buf, loff_t off, size_t count)
{
        struct i2c_client *client = kobj_to_i2c_client(kobj);
        struct mc24lc64t_data *drvdata = i2c_get_clientdata(client);
        unsigned long timeout, read_time, i = 0;
        int status;

        mutex_lock(&drvdata->update_lock);

        if (i2c_smbus_write_byte_data(client, off>>8, off))
        {
                status = -EIO;
                goto exit;
        }

        msleep(1);

begin:

        if (i < count)
        {
                timeout = jiffies + msecs_to_jiffies(25); /* 25 mS timeout*/
                do {
                        read_time = jiffies;

                        status = i2c_smbus_read_byte(client);
                        if (status >= 0)
                        {
                                buf[i++] = status;
                                goto begin;
                        }
                } while (time_before(read_time, timeout));

                status = -ETIMEDOUT;
                goto exit;
        }

        status = count;

exit:
        mutex_unlock(&drvdata->update_lock);

        return status;
}

static struct bin_attribute mc24lc64t_bit_attr = {
        .attr = {
                .name = "eeprom",
                .mode = S_IRUGO,
        },
        .size = 65536,
        .read = mc24lc64t_read,
};

static int mc24lc64t_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
        struct i2c_adapter *adapter = client->adapter;
        struct mc24lc64t_data *drvdata;
        int err;

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WRITE_BYTE_DATA
                                     | I2C_FUNC_SMBUS_READ_BYTE))
                return -EPFNOSUPPORT;

        if (!(drvdata = devm_kzalloc(&client->dev,
                        sizeof(struct mc24lc64t_data), GFP_KERNEL)))
                return -ENOMEM;

        drvdata->fake_client = i2c_new_dummy_device(client->adapter, client->addr + 1);
        if (!drvdata->fake_client)
                return -ENOMEM;

        i2c_set_clientdata(client, drvdata);
        mutex_init(&drvdata->update_lock);

        err = sysfs_create_bin_file(&client->dev.kobj, &mc24lc64t_bit_attr);
        if (err)
                i2c_unregister_device(drvdata->fake_client);

        return err;
}

static int mc24lc64t_remove(struct i2c_client *client)
{
        struct mc24lc64t_data *drvdata = i2c_get_clientdata(client);

        i2c_unregister_device(drvdata->fake_client);

        sysfs_remove_bin_file(&client->dev.kobj, &mc24lc64t_bit_attr);

        return 0;
}

static const struct i2c_device_id mc24lc64t_id[] = {
        { "24lc64t", 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, mc24lc64t_id);

static struct i2c_driver mc24lc64t_driver = {
        .driver = {
                .name   = "mc24lc64t",
                .owner = THIS_MODULE,
        },
        .probe          = mc24lc64t_probe,
        .remove         = mc24lc64t_remove,
        .id_table       = mc24lc64t_id,
};

module_i2c_driver(mc24lc64t_driver);

MODULE_AUTHOR("Abhisit Sangjan <asang@celestica.com>");
MODULE_DESCRIPTION("Microchip 24LC64T Driver");
MODULE_LICENSE("GPL");
