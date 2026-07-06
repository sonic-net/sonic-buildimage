//  * PLD driver for Nokia-7220-IXR-H6-128 Router
//  *
//  * Copyright (C) 2026 Nokia Corporation.
//  *
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation; either version 2 of the License, or
//  * any later version.
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  * GNU General Public License for more details.
//  * see <http://www.gnu.org/licenses/>


#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#define DRIVER_NAME "cb_pld"

// REGISTERS ADDRESS MAP
#define HW_BOARD_VER_REG                 0x00
#define VER_MAJOR_REG                    0x01
#define VER_MINOR_REG                    0x02
#define MISC_REG                         0x05
#define PSU_PRESENT_REG                  0x07
#define SSD_PRESENT_REG                  0x08
#define MUX_SEL_REG                      0x0F
#define RESET_SIGNAL_1_REG               0x20
#define BIOS_RED_REG                     0x37
#define PSU_POWERGOOD_REG                0x51
#define CONSOLE_WDT_REG                  0x63

static const unsigned short cpld_address_list[] = {0x60, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
};

static int cpld_i2c_read(struct cpld_data *data, u8 reg)
{
    int val = 0;
    struct i2c_client *client = data->client;

    val = i2c_smbus_read_byte_data(client, reg);
    if (val < 0) {
         dev_warn(&client->dev, "CB_PLD READ ERROR: reg(0x%02x) err %d\n", reg, val);
    }

    return val;
}

static void cpld_i2c_write(struct cpld_data *data, u8 reg, u8 value)
{
    int res = 0;
    struct i2c_client *client = data->client;

    mutex_lock(&data->update_lock);
    res = i2c_smbus_write_byte_data(client, reg, value);
    if (res < 0) {
        dev_warn(&client->dev, "CB_PLD WRITE ERROR: reg(0x%02x) err %d\n", reg, res);
    }
    mutex_unlock(&data->update_lock);
}

static int cpld_i2c_update_bits(struct cpld_data *data, u8 reg, u8 mask, u8 value)
{
    int ret;
    int val;
    struct i2c_client *client = data->client;

    mutex_lock(&data->update_lock);
    val = i2c_smbus_read_byte_data(client, reg);
    if (val < 0) {
        dev_warn(&client->dev, "CB_PLD READ ERROR: reg(0x%02x) err %d\n", reg, val);
        mutex_unlock(&data->update_lock);
        return val;
    }
    val = (val & ~mask) | (value & mask);
    ret = i2c_smbus_write_byte_data(client, reg, val);
    if (ret < 0) {
        dev_warn(&client->dev, "CB_PLD WRITE ERROR: reg(0x%02x) err %d\n", reg, ret);
    }
    mutex_unlock(&data->update_lock);

    return ret;
}

static ssize_t show_hw_board_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    int val;

    val = cpld_i2c_read(data, HW_BOARD_VER_REG);
    if (val < 0)
        return val;

    return sprintf(buf, "0x%x\n", val);
}

static ssize_t show_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    int reg_major;
    int reg_minor;

    reg_major = cpld_i2c_read(data, VER_MAJOR_REG);
    if (reg_major < 0)
        return reg_major;
    reg_minor = cpld_i2c_read(data, VER_MINOR_REG);
    if (reg_minor < 0)
        return reg_minor;

    return sprintf(buf, "%02x.%02x\n", reg_major, reg_minor);
}

static ssize_t show_misc(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    int val;

    val = cpld_i2c_read(data, MISC_REG);
    if (val < 0)
        return val;

    return sprintf(buf, "0x%x\n", val);
}

static ssize_t set_misc(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 usr_val = 0;

    int ret = kstrtou8(buf, 16, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val != 0x9 && usr_val != 0xb) {
        return -EINVAL;
    }

    cpld_i2c_write(data, MISC_REG, usr_val);

    return count;
}

static ssize_t show_psu_present(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    int val;

    val = cpld_i2c_read(data, PSU_PRESENT_REG);
    if (val < 0)
        return val;

    return sprintf(buf, "%d\n", (val >> sda->index) & 0x1 ? 1 : 0);
}

static ssize_t show_psu_ok(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    int val;

    val = cpld_i2c_read(data, PSU_POWERGOOD_REG);
    if (val < 0)
        return val;

    return sprintf(buf, "%d\n", (val >> sda->index) & 0x1 ? 1 : 0);
}

static ssize_t show_ssd_present(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    int val;

    val = cpld_i2c_read(data, SSD_PRESENT_REG);
    if (val < 0)
        return val;

    return sprintf(buf, "%d\n", (val >> sda->index) & 0x1 ? 1 : 0);
}

static ssize_t show_mux_sel(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    int val;

    val = cpld_i2c_read(data, MUX_SEL_REG);
    if (val < 0)
        return val;

    return sprintf(buf, "%d\n", (val >> sda->index) & 0x1 ? 1 : 0);
}

static ssize_t set_mux_sel(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 usr_val = 0;

    int ret = kstrtou8(buf, 16, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val != 0x0 && usr_val != 0x1) {
        return -EINVAL;
    }

    cpld_i2c_write(data, MUX_SEL_REG, usr_val);

    return count;
}

static ssize_t show_reset_sig(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    int val;

    val = cpld_i2c_read(data, RESET_SIGNAL_1_REG);
    if (val < 0)
        return val;

    return sprintf(buf, "0x%x\n", val);
}

static ssize_t set_reset_sig(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 usr_val = 0;

    int ret = kstrtou8(buf, 16, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val != 0x7f && usr_val != 0xff) {
        return -EINVAL;
    }

    cpld_i2c_write(data, RESET_SIGNAL_1_REG, usr_val);

    return count;
}

static ssize_t show_bios_red(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    int val;

    val = cpld_i2c_read(data, BIOS_RED_REG);
    if (val < 0)
        return val;

    return sprintf(buf, "0x%x\n", val);
}

static ssize_t set_bios_red(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 usr_val = 0;

    int ret = kstrtou8(buf, 16, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val != 0x0 && usr_val != 0x4) {
        return -EINVAL;
    }

    cpld_i2c_write(data, BIOS_RED_REG, usr_val);

    return count;
}

static ssize_t show_console_wdt(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    int val;

    val = cpld_i2c_read(data, CONSOLE_WDT_REG);
    if (val < 0)
        return val;

    return sprintf(buf, "%d\n", (val >> sda->index) & 0x1 ? 1 : 0);
}

static ssize_t set_console_wdt(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = 1 << sda->index;
    ret = cpld_i2c_update_bits(data, CONSOLE_WDT_REG, mask, usr_val << sda->index);
    if (ret < 0)
        return ret;

    return count;
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(hw_board_version, S_IRUGO, show_hw_board_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(misc, S_IRUGO | S_IWUSR, show_misc, set_misc, 0);
static SENSOR_DEVICE_ATTR(psu1_ok, S_IRUGO, show_psu_ok, NULL, 4);
static SENSOR_DEVICE_ATTR(psu2_ok, S_IRUGO, show_psu_ok, NULL, 5);
static SENSOR_DEVICE_ATTR(psu3_ok, S_IRUGO, show_psu_ok, NULL, 6);
static SENSOR_DEVICE_ATTR(psu4_ok, S_IRUGO, show_psu_ok, NULL, 7);
static SENSOR_DEVICE_ATTR(psu1_pres, S_IRUGO, show_psu_present, NULL, 3);
static SENSOR_DEVICE_ATTR(psu2_pres, S_IRUGO, show_psu_present, NULL, 2);
static SENSOR_DEVICE_ATTR(psu3_pres, S_IRUGO, show_psu_present, NULL, 1);
static SENSOR_DEVICE_ATTR(psu4_pres, S_IRUGO, show_psu_present, NULL, 0);
static SENSOR_DEVICE_ATTR(ssd1_pres, S_IRUGO, show_ssd_present, NULL, 1);
static SENSOR_DEVICE_ATTR(ssd2_pres, S_IRUGO, show_ssd_present, NULL, 0);
static SENSOR_DEVICE_ATTR(mux_sel, S_IRUGO | S_IWUSR, show_mux_sel, set_mux_sel, 0);
static SENSOR_DEVICE_ATTR(reset_sig, S_IRUGO | S_IWUSR, show_reset_sig, set_reset_sig, 0);
static SENSOR_DEVICE_ATTR(bios_red, S_IRUGO | S_IWUSR, show_bios_red, set_bios_red, 0);
static SENSOR_DEVICE_ATTR(console_wdt, S_IRUGO | S_IWUSR, show_console_wdt, set_console_wdt, 0);

static struct attribute *cb_pld_attributes[] = {
    &sensor_dev_attr_hw_board_version.dev_attr.attr,
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_misc.dev_attr.attr,
    &sensor_dev_attr_psu1_ok.dev_attr.attr,
    &sensor_dev_attr_psu2_ok.dev_attr.attr,
    &sensor_dev_attr_psu3_ok.dev_attr.attr,
    &sensor_dev_attr_psu4_ok.dev_attr.attr,
    &sensor_dev_attr_psu1_pres.dev_attr.attr,
    &sensor_dev_attr_psu2_pres.dev_attr.attr,
    &sensor_dev_attr_psu3_pres.dev_attr.attr,
    &sensor_dev_attr_psu4_pres.dev_attr.attr,
    &sensor_dev_attr_ssd1_pres.dev_attr.attr,
    &sensor_dev_attr_ssd2_pres.dev_attr.attr,
    &sensor_dev_attr_mux_sel.dev_attr.attr,
    &sensor_dev_attr_reset_sig.dev_attr.attr,
    &sensor_dev_attr_bios_red.dev_attr.attr,
    &sensor_dev_attr_console_wdt.dev_attr.attr,
    NULL
};

static const struct attribute_group cb_pld_group = {
    .attrs = cb_pld_attributes,
};

static int cb_pld_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia CB_PLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &cb_pld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit_sysfs_create_group;
    }

    return 0;

exit_sysfs_create_group:
    kfree(data);
exit:
    return status;
}

static void cb_pld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &cb_pld_group);
    kfree(data);
}

static const struct of_device_id cb_pld_of_ids[] = {
    {
        .compatible = "cb_pld",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, cb_pld_of_ids);

static const struct i2c_device_id cb_pld_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, cb_pld_ids);

static struct i2c_driver cb_pld_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(cb_pld_of_ids),
    },
    .probe        = cb_pld_probe,
    .remove       = cb_pld_remove,
    .id_table     = cb_pld_ids,
    .address_list = cpld_address_list,
};

static int __init cb_pld_init(void)
{
    return i2c_add_driver(&cb_pld_driver);
}

static void __exit cb_pld_exit(void)
{
    i2c_del_driver(&cb_pld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA H6-128 CB_PLD driver");
MODULE_LICENSE("GPL");

module_init(cb_pld_init);
module_exit(cb_pld_exit);
