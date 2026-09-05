/*
 * switch_cpld.c - i2c driver for Silverstone switchboard CPLD1/CPLD2
 * provides sysfs interfaces to access CPLD register and control port LEDs
 *
 * Author: Budsakol Sirirattanasakul
 *
 * Copyright (C) 2019 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/hwmon.h>

#define CPLD1_ADDR      0x30
#define CPLD2_ADDR      0x31

#define SCRATCH_ADDR    0x01
#define LED_OPMODE      0x09
#define LED_TEST        0x0A

struct switch_cpld_data {
        struct mutex lock;
        struct i2c_client *client;
        struct i2c_client *client2;
        uint8_t read_addr;
};

static ssize_t getreg_show(struct device *dev, struct device_attribute *attr,
                           char *buf)
{
        struct switch_cpld_data *data = dev_get_drvdata(dev);
        struct i2c_client *client = data->client;
        int value;

        value = i2c_smbus_read_byte_data(client, data->read_addr);
        if (value < 0)
                return value;

        return sprintf(buf, "0x%.2x\n", value);
}

static ssize_t getreg_store(struct device *dev, struct device_attribute *attr,
                            const char *buf, size_t size)
{
        uint8_t value;
        ssize_t status;
        struct switch_cpld_data *data = dev_get_drvdata(dev);

        status = kstrtou8(buf, 0, &value);
        if (status != 0)
                return status;

        data->read_addr = value;

        return size;
}

static ssize_t setreg_store(struct device *dev, struct device_attribute *attr,
                            const char *buf, size_t size)
{
        uint8_t addr, value;
        ssize_t status;
        struct switch_cpld_data *data = dev_get_drvdata(dev);
        struct i2c_client *client = data->client;
        char *tok;

        tok = strsep((char **)&buf, " ");
        if (tok == NULL)
                return -EINVAL;
        status = kstrtou8(tok, 0, &addr);
        if (status != 0)
                return status;

        tok = strsep((char **)&buf, " ");
        if (tok == NULL)
                return -EINVAL;
        status = kstrtou8(tok, 0, &value);
        if (status != 0)
                return status;

        status = i2c_smbus_write_byte_data(client, addr, value);
        if (status == 0)
                status = size;
        return status;
}

static ssize_t scratch_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
        struct switch_cpld_data *data = dev_get_drvdata(dev);
        struct i2c_client *client = data->client;
        int value;

        value = i2c_smbus_read_byte_data(client, SCRATCH_ADDR);
        if (value < 0)
                return value;

        return sprintf(buf, "0x%.2x\n", value);
}

static ssize_t scratch_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t size)
{
        uint8_t value;
        ssize_t status;
        struct switch_cpld_data *data = dev_get_drvdata(dev);
        struct i2c_client *client = data->client;

        status = kstrtou8(buf, 0, &value);
        if (status != 0)
                return status;
        status = i2c_smbus_write_byte_data(client, SCRATCH_ADDR, value);
        if (status == 0)
                status = size;
        return status;
}

DEVICE_ATTR_RW(getreg);
DEVICE_ATTR_WO(setreg);
DEVICE_ATTR_RW(scratch);

static struct attribute *switch_cpld_attrs[] = {
        &dev_attr_getreg.attr,
        &dev_attr_setreg.attr,
        &dev_attr_scratch.attr,
        NULL,
};
ATTRIBUTE_GROUPS(switch_cpld);

static ssize_t port_led_mode_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
        int led_mode_1, led_mode_2;
        struct switch_cpld_data *data = dev_get_drvdata(dev);
        struct i2c_client *client1 = data->client;
        struct i2c_client *client2 = data->client2;

        led_mode_1 = i2c_smbus_read_byte_data(client1, LED_OPMODE);
        if (led_mode_1 < 0)
                return led_mode_1;

        led_mode_2 = i2c_smbus_read_byte_data(client2, LED_OPMODE);
        if (led_mode_2 < 0)
                return led_mode_2;

        return sprintf(buf, "%s %s\n",
                       led_mode_1 ? "test" : "normal",
                       led_mode_2 ? "test" : "normal");
}

static ssize_t port_led_mode_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
        int status;
        uint8_t led_mode;
        struct switch_cpld_data *data = dev_get_drvdata(dev);
        struct i2c_client *client1 = data->client;
        struct i2c_client *client2 = data->client2;

        if (sysfs_streq(buf, "test"))
                led_mode = 0x01;
        else if (sysfs_streq(buf, "normal"))
                led_mode = 0x00;
        else
                return -EINVAL;

        status = i2c_smbus_write_byte_data(client1, LED_OPMODE, led_mode);
        if (status != 0) {
                return status;
        }

        status = i2c_smbus_write_byte_data(client2, LED_OPMODE, led_mode);
        if (status != 0) {
                return status;
        }

        return size;
}

static ssize_t port_led_color_show(struct device *dev,
                                   struct device_attribute *attr, char *buf)
{
        int led_color1, led_color2;
        struct switch_cpld_data *data = dev_get_drvdata(dev);
        struct i2c_client *client1 = data->client;
        struct i2c_client *client2 = data->client2;

        led_color1 = i2c_smbus_read_byte_data(client1, LED_TEST);
        if (led_color1 < 0)
                return led_color1;

        led_color2 = i2c_smbus_read_byte_data(client2, LED_TEST);
        if (led_color2 < 0)
                return led_color2;

        return sprintf(buf, "%s %s\n",
                       led_color1 == 0x07 ? "off" : led_color1 == 0x06 ? "green" : led_color1 == 0x05 ?
                       "red" : led_color1 == 0x04 ?
                       "yellow" : led_color1 == 0x03 ? "blue" : led_color1 == 0x02 ?  "cyan" :
                       led_color1 == 0x01 ?  "magenta" : "white",
                       led_color2 == 0x07 ? "off" : led_color2 == 0x06 ? "green" : led_color2 == 0x05 ?
                       "red" : led_color2 == 0x04 ?
                       "yellow" : led_color2 == 0x03 ? "blue" : led_color2 == 0x02 ?  "cyan" :
                       led_color2 == 0x01 ?  "magenta" : "white");
}

static ssize_t port_led_color_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t size)
{
        int status;
        uint8_t led_color;
        struct switch_cpld_data *data = dev_get_drvdata(dev);
        struct i2c_client *client1 = data->client;
        struct i2c_client *client2 = data->client2;

        if (sysfs_streq(buf, "off"))
                led_color = 0x07;
        else if (sysfs_streq(buf, "green"))
                led_color = 0x06;
        else if (sysfs_streq(buf, "red"))
                led_color = 0x05;
        else if (sysfs_streq(buf, "yellow"))
                led_color = 0x04;
        else if (sysfs_streq(buf, "blue"))
                led_color = 0x03;
        else if (sysfs_streq(buf, "cyan"))
                led_color = 0x02;
        else if (sysfs_streq(buf, "magenta"))
                led_color = 0x01;
        else if (sysfs_streq(buf, "white"))
                led_color = 0x00;
        else
                return -EINVAL;

        status = i2c_smbus_write_byte_data(client1, LED_TEST, led_color);
        if (status != 0) {
                return status;
        }

        status = i2c_smbus_write_byte_data(client2, LED_TEST, led_color);
        if (status != 0) {
                return status;
        }

        return size;
}

DEVICE_ATTR_RW(port_led_mode);
DEVICE_ATTR_RW(port_led_color);

static struct attribute *sff_led_attrs[] = {
        &dev_attr_port_led_mode.attr,
        &dev_attr_port_led_color.attr,
        NULL,
};

static struct attribute_group sff_led_groups = {
        .attrs = sff_led_attrs,
};

static int switch_cpld_probe(struct i2c_client *client)
{
        int err;
        struct switch_cpld_data *drvdata1, *drvdata2;
        struct device *hwmon_dev1, *hwmon_dev2;
        struct i2c_client *client2;

        if (client->addr != CPLD1_ADDR) {
                dev_err(&client->dev, "probe, bad i2c addr: 0x%x\n",
                        client->addr);
                err = -EINVAL;
                goto exit;
        }

        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
                return -EPFNOSUPPORT;

        /* CPLD1 */
        drvdata1 = devm_kzalloc(&client->dev,
                                sizeof(struct switch_cpld_data), GFP_KERNEL);

        if (!drvdata1) {
                err = -ENOMEM;
                goto exit;
        }

        mutex_init(&drvdata1->lock);
        drvdata1->client = client;
        drvdata1->read_addr = 0x00;
        i2c_set_clientdata(client, drvdata1);
        hwmon_dev1 = devm_hwmon_device_register_with_groups(&client->dev,
                        "CPLD1",
                        drvdata1,
                        switch_cpld_groups);

        if (IS_ERR(hwmon_dev1)) {
                err = PTR_ERR(hwmon_dev1);
                goto exit;
        }

        err = sysfs_create_link(&client->dev.kobj, &hwmon_dev1->kobj, "CPLD1");
        if (err) {
                goto exit;
        }

        /* CPLD2 */
        drvdata2 = devm_kzalloc(&client->dev,
                                sizeof(struct switch_cpld_data), GFP_KERNEL);

        if (!drvdata2) {
                err = -ENOMEM;
                goto err_link;
        }

        client2 = i2c_new_dummy_device(client->adapter, CPLD2_ADDR);
        if (!client2) {
                dev_err(&client->dev, "address 0x%02x unavailable\n",
                        CPLD2_ADDR);
                err = -EADDRINUSE;
                goto err_link;
        }

        mutex_init(&drvdata2->lock);
        drvdata2->read_addr = 0x00;
        drvdata2->client = client2;
        i2c_set_clientdata(client2, drvdata2);

        /* attach client2 to be client2 of CPLD1
           for later use on port led sysfs */
        drvdata1->client2 = client2;

        hwmon_dev2 = devm_hwmon_device_register_with_groups(&client2->dev,
                        "CPLD2",
                        drvdata2,
                        switch_cpld_groups);

        if (IS_ERR(hwmon_dev2)) {
                err = PTR_ERR(hwmon_dev2);
                goto err_client2;
        }

        err = sysfs_create_link(&client2->dev.kobj, &hwmon_dev2->kobj, "CPLD2");
        if (err) {
                goto err_client2;
        }

        //port led

        err = sysfs_create_group(&client->dev.kobj, &sff_led_groups);
        if (err) {
                dev_err(&client->dev,
                        "failed to create sysfs attribute group.\n");
                goto err_link2;
        }

        return 0;

err_link2:
        sysfs_remove_link(&client2->dev.kobj, "CPLD2");

err_client2:
        if (client2)
                i2c_unregister_device(client2);

err_link:
        sysfs_remove_link(&client->dev.kobj, "CPLD1");

exit:
        dev_err(&client->dev, "probe error %d\n", err);
        return err;
}

static void switch_cpld_remove(struct i2c_client *client)
{
        struct switch_cpld_data *data = i2c_get_clientdata(client);

        sysfs_remove_group(&client->dev.kobj, &sff_led_groups);
        sysfs_remove_link(&data->client2->dev.kobj, "CPLD2");
        sysfs_remove_link(&client->dev.kobj, "CPLD1");
        i2c_unregister_device(data->client2);
}

static const struct i2c_device_id switch_cpld_ids[] = {
        { "switch_cpld", 0x30 },
        { }
};

MODULE_DEVICE_TABLE(i2c, switch_cpld_ids);

static struct i2c_driver switch_cpld_driver = {
        .driver = {
                .name   = "switch_cpld",
                .owner = THIS_MODULE,
        },
        .probe          = switch_cpld_probe,
        .remove         = switch_cpld_remove,
        .id_table       = switch_cpld_ids,
};

module_i2c_driver(switch_cpld_driver);

MODULE_AUTHOR("Budsakol Sirirattanasakul<bsirir@celestica.com>");
MODULE_DESCRIPTION("Celestica Silverstone Switchboard CPLD driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
