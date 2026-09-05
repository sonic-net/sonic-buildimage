/*
 * A hwmon driver for the alphanetworks_snh60a0_320fv2_power_cpld
 *
 * Copyright (C) 2018 Alphanetworks Technology Corporation.
 * Robin Chen <Robin_chen@Alphanetworks.com>
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * any later version. 
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 * see <http://www.gnu.org/licenses/>
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
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>


#define DRIVER_NAME        "snh60a0_power_cpld"
#define PSU1_STATUS_REG    0x3
#define PSU2_STATUS_REG    0x4
#define FAN_PWM_REG        0x23

#define PSU_PRESENT_BIT    0x4
#define PSU_POWER_BIT      0x2
#define FAN_PRESENT_BIT    0x2


static ssize_t psu_show_status(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t fan_pwm_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t set_fan_pwm(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t fan_show_status(struct device *dev, struct device_attribute *attr, char *buf);

static LIST_HEAD(cpld_client_list);
static struct mutex	 list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

/* Addresses scanned for alphanetworks_snh60a0_320fv2_power_cpld */
static const unsigned short normal_i2c[] = { 0x5E, I2C_CLIENT_END };

struct alphanetworks_snh60a0_320fv2_pwr_cpld_data {
  struct device      *hwmon_dev;
  struct mutex        update_lock;
  char model_name[9]; /* Model name, read from eeprom */
};


enum sysfs_psu_attributes {
  PSU1_PRESENT,
  PSU2_PRESENT,
  PSU1_POWER_GOOD,
  PSU2_POWER_GOOD,
  FAN_PWM,
  FAN1_FAULT,
  FAN2_FAULT,
  FAN3_FAULT,
  FAN4_FAULT,
  FAN5_FAULT,
  FAN6_FAULT,
  FAN1_PRESENT=0x10,
  FAN2_PRESENT,
  FAN3_PRESENT,
  FAN4_PRESENT,
  FAN5_PRESENT,
  FAN6_PRESENT,
  FAN1_FRONT_SPEED_RPM,
  FAN1_REAR_SPEED_RPM,
  FAN2_FRONT_SPEED_RPM,
  FAN2_REAR_SPEED_RPM,
  FAN3_FRONT_SPEED_RPM,
  FAN3_REAR_SPEED_RPM,
  FAN4_FRONT_SPEED_RPM,
  FAN4_REAR_SPEED_RPM,
  FAN5_FRONT_SPEED_RPM,
  FAN5_REAR_SPEED_RPM,
  FAN6_FRONT_SPEED_RPM,
  FAN6_REAR_SPEED_RPM,
};

static SENSOR_DEVICE_ATTR(psu1_present, S_IRUGO, psu_show_status, NULL, PSU1_PRESENT);
static SENSOR_DEVICE_ATTR(psu2_present, S_IRUGO, psu_show_status, NULL, PSU2_PRESENT);
static SENSOR_DEVICE_ATTR(psu1_power_good, S_IRUGO, psu_show_status, NULL, PSU1_POWER_GOOD);
static SENSOR_DEVICE_ATTR(psu2_power_good, S_IRUGO, psu_show_status, NULL, PSU2_POWER_GOOD);
static SENSOR_DEVICE_ATTR(fan_pwm, (0660), fan_pwm_show, set_fan_pwm, FAN_PWM);
static SENSOR_DEVICE_ATTR(fan1_present, S_IRUGO, fan_show_status, NULL, FAN1_PRESENT);
static SENSOR_DEVICE_ATTR(fan2_present, S_IRUGO, fan_show_status, NULL, FAN2_PRESENT);
static SENSOR_DEVICE_ATTR(fan3_present, S_IRUGO, fan_show_status, NULL, FAN3_PRESENT);
static SENSOR_DEVICE_ATTR(fan4_present, S_IRUGO, fan_show_status, NULL, FAN4_PRESENT);
static SENSOR_DEVICE_ATTR(fan5_present, S_IRUGO, fan_show_status, NULL, FAN5_PRESENT);
static SENSOR_DEVICE_ATTR(fan6_present, S_IRUGO, fan_show_status, NULL, FAN6_PRESENT);
static SENSOR_DEVICE_ATTR(fan1_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN1_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan2_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN2_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan3_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN3_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan4_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN4_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan5_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN5_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan6_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN6_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan1_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN1_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan2_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN2_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan3_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN3_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan4_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN4_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan5_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN5_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan6_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN6_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan1_fault, S_IRUGO, fan_show_status, NULL, FAN1_FAULT); static SENSOR_DEVICE_ATTR(fan11_fault, S_IRUGO, fan_show_status, NULL, FAN1_FAULT);
static SENSOR_DEVICE_ATTR(fan2_fault, S_IRUGO, fan_show_status, NULL, FAN2_FAULT); static SENSOR_DEVICE_ATTR(fan12_fault, S_IRUGO, fan_show_status, NULL, FAN2_FAULT);
static SENSOR_DEVICE_ATTR(fan3_fault, S_IRUGO, fan_show_status, NULL, FAN3_FAULT); static SENSOR_DEVICE_ATTR(fan13_fault, S_IRUGO, fan_show_status, NULL, FAN3_FAULT);
static SENSOR_DEVICE_ATTR(fan4_fault, S_IRUGO, fan_show_status, NULL, FAN4_FAULT); static SENSOR_DEVICE_ATTR(fan14_fault, S_IRUGO, fan_show_status, NULL, FAN4_FAULT);
static SENSOR_DEVICE_ATTR(fan5_fault, S_IRUGO, fan_show_status, NULL, FAN5_FAULT); static SENSOR_DEVICE_ATTR(fan15_fault, S_IRUGO, fan_show_status, NULL, FAN5_FAULT);
static SENSOR_DEVICE_ATTR(fan6_fault, S_IRUGO, fan_show_status, NULL, FAN6_FAULT); static SENSOR_DEVICE_ATTR(fan16_fault, S_IRUGO, fan_show_status, NULL, FAN6_FAULT);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, fan_show_status, NULL, FAN1_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan11_input, S_IRUGO, fan_show_status, NULL, FAN1_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, fan_show_status, NULL, FAN2_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan12_input, S_IRUGO, fan_show_status, NULL, FAN2_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO, fan_show_status, NULL, FAN3_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan13_input, S_IRUGO, fan_show_status, NULL, FAN3_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO, fan_show_status, NULL, FAN4_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan14_input, S_IRUGO, fan_show_status, NULL, FAN4_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan5_input, S_IRUGO, fan_show_status, NULL, FAN5_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan15_input, S_IRUGO, fan_show_status, NULL, FAN5_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan6_input, S_IRUGO, fan_show_status, NULL, FAN6_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan16_input, S_IRUGO, fan_show_status, NULL, FAN6_REAR_SPEED_RPM);


static struct attribute *alphanetworks_snh60a0_320fv2_psu_attributes[] = {
  &sensor_dev_attr_psu1_present.dev_attr.attr,
  &sensor_dev_attr_psu2_present.dev_attr.attr,
  &sensor_dev_attr_psu1_power_good.dev_attr.attr,
  &sensor_dev_attr_psu2_power_good.dev_attr.attr,
  &sensor_dev_attr_fan_pwm.dev_attr.attr,
  &sensor_dev_attr_fan1_present.dev_attr.attr,
  &sensor_dev_attr_fan2_present.dev_attr.attr,
  &sensor_dev_attr_fan3_present.dev_attr.attr,
  &sensor_dev_attr_fan4_present.dev_attr.attr,
  &sensor_dev_attr_fan5_present.dev_attr.attr,
  &sensor_dev_attr_fan6_present.dev_attr.attr,
  &sensor_dev_attr_fan1_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan2_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan3_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan4_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan5_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan6_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan1_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan2_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan3_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan4_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan5_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan6_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan1_fault.dev_attr.attr,  &sensor_dev_attr_fan11_fault.dev_attr.attr,
  &sensor_dev_attr_fan2_fault.dev_attr.attr,  &sensor_dev_attr_fan12_fault.dev_attr.attr,
  &sensor_dev_attr_fan3_fault.dev_attr.attr,  &sensor_dev_attr_fan13_fault.dev_attr.attr,
  &sensor_dev_attr_fan4_fault.dev_attr.attr,  &sensor_dev_attr_fan14_fault.dev_attr.attr,
  &sensor_dev_attr_fan5_fault.dev_attr.attr,  &sensor_dev_attr_fan15_fault.dev_attr.attr,
  &sensor_dev_attr_fan6_fault.dev_attr.attr,  &sensor_dev_attr_fan16_fault.dev_attr.attr,
  &sensor_dev_attr_fan1_input.dev_attr.attr,  &sensor_dev_attr_fan11_input.dev_attr.attr,
  &sensor_dev_attr_fan2_input.dev_attr.attr,  &sensor_dev_attr_fan12_input.dev_attr.attr,
  &sensor_dev_attr_fan3_input.dev_attr.attr,  &sensor_dev_attr_fan13_input.dev_attr.attr,
  &sensor_dev_attr_fan4_input.dev_attr.attr,  &sensor_dev_attr_fan14_input.dev_attr.attr,
  &sensor_dev_attr_fan5_input.dev_attr.attr,  &sensor_dev_attr_fan15_input.dev_attr.attr,
  &sensor_dev_attr_fan6_input.dev_attr.attr,  &sensor_dev_attr_fan16_input.dev_attr.attr,
  NULL
};

static const struct attribute_group alphanetworks_snh60a0_320fv2_psu_group = {
  .attrs = alphanetworks_snh60a0_320fv2_psu_attributes,
};


static ssize_t psu_show_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0;
    u8 command;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    switch(sda->index) {
        case PSU1_PRESENT:
        case PSU1_POWER_GOOD:
          command = PSU1_STATUS_REG;
          break;
        case PSU2_PRESENT:
        case PSU2_POWER_GOOD:
          command = PSU2_STATUS_REG;
          break;
    }

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    switch(sda->index) {
        case PSU1_PRESENT:
        case PSU2_PRESENT:
          res = (val & PSU_PRESENT_BIT ? 1 : 0 );
          break;
        case PSU1_POWER_GOOD:
        case PSU2_POWER_GOOD:
          res = (val & PSU_POWER_BIT ? 1 : 0 );
          break;
    }

    return sprintf(buf, "%d\n", res);
}

static ssize_t fan_pwm_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0;
    struct i2c_client *client = to_i2c_client(dev);

    val = i2c_smbus_read_byte_data(client, FAN_PWM_REG);

    if (val < 0) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    return sprintf(buf, "%d", val);
}

static ssize_t set_fan_pwm(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    int error, value;

    error = kstrtoint(buf, 10, &value);
    if (error)
      return error;

    if (value < 0 || value > 0xFF)
      return -EINVAL;

    i2c_smbus_write_byte_data(client, FAN_PWM_REG, value);

    return count;
}

static ssize_t fan_show_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    struct i2c_client *client = to_i2c_client(dev);
    //    struct as7712_32x_fan_data *data = as7712_32x_fan_update_device(dev);
    ssize_t ret = 0;
    int val, val2;

    switch (sda->index) {
        /* case FAN_DUTY_CYCLE_PERCENTAGE: */
        /* { */
        /*   u32 duty_cycle = reg_val_to_duty_cycle(data->reg_val[FAN_DUTY_CYCLE_PERCENTAGE]); */
        /*   ret = sprintf(buf, "%u\n", duty_cycle); */
        /*   break; */
        /* } */
        case FAN1_FRONT_SPEED_RPM:
        case FAN2_FRONT_SPEED_RPM:
        case FAN3_FRONT_SPEED_RPM:
        case FAN4_FRONT_SPEED_RPM:
        case FAN5_FRONT_SPEED_RPM:
        case FAN6_FRONT_SPEED_RPM:
        case FAN1_REAR_SPEED_RPM:
        case FAN2_REAR_SPEED_RPM:
        case FAN3_REAR_SPEED_RPM:
        case FAN4_REAR_SPEED_RPM:
        case FAN5_REAR_SPEED_RPM:
        case FAN6_REAR_SPEED_RPM:
          val = i2c_smbus_read_byte_data(client, sda->index);
          ret = sprintf(buf, "%d\n", val * 150);
          break;
        case FAN1_PRESENT:
        case FAN2_PRESENT:
        case FAN3_PRESENT:
        case FAN4_PRESENT:
        case FAN5_PRESENT:
        case FAN6_PRESENT:
          val = i2c_smbus_read_byte_data(client, sda->index);
          ret = sprintf(buf, "%d\n", (val & FAN_PRESENT_BIT) ? 1 : 0);
          break;
        case FAN1_FAULT:
        case FAN2_FAULT:
        case FAN3_FAULT:
        case FAN4_FAULT:
        case FAN5_FAULT:
        case FAN6_FAULT:
          val  = i2c_smbus_read_byte_data(client, (sda->index - FAN1_FAULT)*2 + FAN1_FRONT_SPEED_RPM);
          val2 = i2c_smbus_read_byte_data(client, (sda->index - FAN1_FAULT)*2 + FAN1_REAR_SPEED_RPM);
          ret = sprintf(buf, "%d\n", (val|val2) ? 0 : 1);
          break;
        default:
          break;
    }

  return ret;
}


static void alpha_i2c_cpld_add_client(struct i2c_client *client)
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

static void alpha_i2c_cpld_remove_client(struct i2c_client *client)
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

static int alpha_i2c_cpld_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    int status;
    struct alphanetworks_snh60a0_320fv2_pwr_cpld_data* data;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct alphanetworks_snh60a0_320fv2_pwr_cpld_data), GFP_KERNEL);
    if (!data) {
      status = -ENOMEM;
      goto exit;
    }

    status = sysfs_create_group(&client->dev.kobj, &alphanetworks_snh60a0_320fv2_psu_group);
    if (status) {
      goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    alpha_i2c_cpld_add_client(client);

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
      status = PTR_ERR(data->hwmon_dev);
      goto exit;
    }

    dev_info(&client->dev, "%s: pwr_cpld '%s'\n",
             dev_name(data->hwmon_dev), client->name);

    return 0;

exit:
    return status;
}

static int alpha_i2c_cpld_remove(struct i2c_client *client)
{
    struct alphanetworks_snh60a0_320fv2_pwr_cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &alphanetworks_snh60a0_320fv2_psu_group);
    alpha_i2c_cpld_remove_client(client);
    kfree(data);

    return 0;
}

static const struct i2c_device_id alpha_i2c_cpld_id[] = {
    { DRIVER_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, alpha_i2c_cpld_id);

static struct i2c_driver alpha_i2c_cpld_driver = {
    .class		= I2C_CLASS_HWMON,
    .driver = {
        .name = DRIVER_NAME,
    },
    .probe		= alpha_i2c_cpld_probe,
    .remove	   	= alpha_i2c_cpld_remove,
    .id_table	= alpha_i2c_cpld_id,
    .address_list = normal_i2c,
};

int alpha_i2c_cpld_read(unsigned short cpld_addr, u8 reg)
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
EXPORT_SYMBOL(alpha_i2c_cpld_read);

int alpha_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
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
EXPORT_SYMBOL(alpha_i2c_cpld_write);

static int __init alpha_i2c_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&alpha_i2c_cpld_driver);
}

static void __exit alpha_i2c_cpld_exit(void)
{
    i2c_del_driver(&alpha_i2c_cpld_driver);
}

static struct dmi_system_id alphanetworks_snh60a0_320fv2_dmi_table[] = {
    {
        .ident = "Alpha Zion",
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "Alpha"),
            DMI_MATCH(DMI_PRODUCT_NAME, "Zion"),
        },
    }
};

int platform_alphanetworks_snh60a0_320fv2(void)
{
    return dmi_check_system(alphanetworks_snh60a0_320fv2_dmi_table);
}
EXPORT_SYMBOL(platform_alphanetworks_snh60a0_320fv2);

MODULE_AUTHOR("Alpha-SID6");
MODULE_DESCRIPTION("alpha_power_cpld driver");
MODULE_LICENSE("GPL");

module_init(alpha_i2c_cpld_init);
module_exit(alpha_i2c_cpld_exit);
