/*
 * <bsn.cl fy=2013 v=gpl>
 *
 *        Copyright (C) 2017 Delta Networks, Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under  the terms ofthe GNU General Public License as
 * published by the Free Software Foundation;  either version 2 of the  License,
 * or (at your option) any later version.
 *
 *
 * </bsn.cl>
 *
 *
 * A hwmon driver for the SMSC EMC2305 fan controller
 * Complete datasheet is available (6/2013) at:
 * http://www.smsc.com/media/Downloads_Public/Data_Sheets/2305.pdf
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>


static ssize_t set_pwm(struct device *dev, struct device_attribute *devattr,
                       const char *buf, size_t count);
static ssize_t show_pwm(struct device *dev, struct device_attribute *devattr,
                        char *buf);
static ssize_t set_fan(struct device *dev, struct device_attribute *devattr,
                       const char *buf, size_t count);
static ssize_t show_fan(struct device *dev, struct device_attribute *devattr,
                        char *buf);
static ssize_t set_fan_percentage(struct device *dev, struct device_attribute *devattr,
                       const char *buf, size_t count);
static ssize_t show_fan_percentage(struct device *dev, struct device_attribute *  devattr,
                        char *buf);
static const unsigned short normal_i2c[] = { 0x2C, 0x2D, 0x2E, 0x2F, 0x4C,
                                             0x4D, I2C_CLIENT_END
                                           };


#define EMC2305_REG_DEVICE 0xFD
#define EMC2305_REG_VENDOR 0xFE

//#define FAN_MINIMUN     0x33   /*20%*/
#define FAN_MINIMUN     0x0   /*0%*/
#define FAN_RPM_BASED             0xAB

#define EMC2305_REG_FAN_DRIVE(n) (0x30 + 0x10 * n)
#define EMC2305_REG_FAN_MIN_DRIVE(n) (0x38 + 0x10 * n)
#define EMC2305_REG_FAN_TACH(n) (0x3E + 0x10 * n)
#define EMC2305_REG_FAN_CONF(n) (0x32 + 0x10 * n)
#define EMC2305_REG_FAN_REAR_H_RPM(n) (0x3D + 0x10 * n)
#define EMC2305_REG_FAN_REAR_L_RPM(n) (0x3C + 0x10 * n)

#define EMC2305_DEVICE 0x34
#define EMC2305_VENDOR 0x5D
#define MAX_FAN_SPEED 23000

struct emc2305_data
{
  struct device   *hwmon_dev;
  struct attribute_group  attrs;
  struct mutex    lock;
};

static int emc2305_probe(struct i2c_client *client,
                         const struct i2c_device_id *id);
static int emc2305_detect(struct i2c_client *client,
                          struct i2c_board_info *info);
static int emc2305_remove(struct i2c_client *client);

static const struct i2c_device_id emc2305_id[] =
{
  { "emc2305", 0 },
  { }
};
MODULE_DEVICE_TABLE(i2c, emc2305_id);

static struct i2c_driver emc2305_driver =
{
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "emc2305",
  },
  .probe    = emc2305_probe,
  .remove   = emc2305_remove,
  .id_table = emc2305_id,
  .detect   = emc2305_detect,
  .address_list = normal_i2c,
};

static SENSOR_DEVICE_ATTR(fan1_input, S_IWUSR | S_IRUGO, show_fan, set_fan, 0);
static SENSOR_DEVICE_ATTR(fan2_input, S_IWUSR | S_IRUGO, show_fan, set_fan, 1);
static SENSOR_DEVICE_ATTR(fan3_input, S_IWUSR | S_IRUGO, show_fan, set_fan, 2);
static SENSOR_DEVICE_ATTR(fan4_input, S_IWUSR | S_IRUGO, show_fan, set_fan, 3);
static SENSOR_DEVICE_ATTR(fan5_input, S_IWUSR | S_IRUGO, show_fan, set_fan, 4);
static SENSOR_DEVICE_ATTR(fan1_input_percentage, S_IWUSR | S_IRUGO, show_fan_percentage, set_fan_percentage, 0);
static SENSOR_DEVICE_ATTR(fan2_input_percentage, S_IWUSR | S_IRUGO, show_fan_percentage, set_fan_percentage, 1);
static SENSOR_DEVICE_ATTR(fan3_input_percentage, S_IWUSR | S_IRUGO, show_fan_percentage, set_fan_percentage, 2);
static SENSOR_DEVICE_ATTR(fan4_input_percentage, S_IWUSR | S_IRUGO, show_fan_percentage, set_fan_percentage, 3);
static SENSOR_DEVICE_ATTR(fan5_input_percentage, S_IWUSR | S_IRUGO, show_fan_percentage, set_fan_percentage, 4);
static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR | S_IRUGO, show_pwm, set_pwm, 0);
static SENSOR_DEVICE_ATTR(pwm2, S_IWUSR | S_IRUGO, show_pwm, set_pwm, 1);
static SENSOR_DEVICE_ATTR(pwm3, S_IWUSR | S_IRUGO, show_pwm, set_pwm, 2);
static SENSOR_DEVICE_ATTR(pwm4, S_IWUSR | S_IRUGO, show_pwm, set_pwm, 3);
static SENSOR_DEVICE_ATTR(pwm5, S_IWUSR | S_IRUGO, show_pwm, set_pwm, 4);

static struct attribute *emc2305_attr[] =
{
  &sensor_dev_attr_fan1_input.dev_attr.attr,
  &sensor_dev_attr_fan2_input.dev_attr.attr,
  &sensor_dev_attr_fan3_input.dev_attr.attr,
  &sensor_dev_attr_fan4_input.dev_attr.attr,
  &sensor_dev_attr_fan5_input.dev_attr.attr,
  &sensor_dev_attr_fan1_input_percentage.dev_attr.attr,
  &sensor_dev_attr_fan2_input_percentage.dev_attr.attr,
  &sensor_dev_attr_fan3_input_percentage.dev_attr.attr,
  &sensor_dev_attr_fan4_input_percentage.dev_attr.attr,
  &sensor_dev_attr_fan5_input_percentage.dev_attr.attr,
  &sensor_dev_attr_pwm1.dev_attr.attr,
  &sensor_dev_attr_pwm2.dev_attr.attr,
  &sensor_dev_attr_pwm3.dev_attr.attr,
  &sensor_dev_attr_pwm4.dev_attr.attr,
  &sensor_dev_attr_pwm5.dev_attr.attr,
  NULL
};

static ssize_t show_fan_percentage(struct device *dev, struct device_attribute *  devattr,
                        char *buf)
{
  struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
  struct i2c_client *client = to_i2c_client(dev);
  struct emc2305_data *data = i2c_get_clientdata(client);
  int val;

  mutex_lock(&data->lock);
  val = i2c_smbus_read_word_swapped(client,
                                    EMC2305_REG_FAN_TACH(attr->index));
  mutex_unlock(&data->lock);
  /* Left shift 3 bits for showing correct RPM */
  val = val >> 3;
  if ((int)(3932160 * 2 / (val > 0 ? val : 1) == 960))return sprintf(buf,     "%d\n", 0);
  return sprintf(buf, "%d\n", (int)(3932160 * 2 / (val > 0 ? val : 1) * 100 / MAX_FAN_SPEED));
}


static ssize_t set_fan_percentage(struct device *dev, struct device_attribute *devattr,
                       const char *buf, size_t count)
{
  struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
  struct i2c_client *client = to_i2c_client(dev);
  struct emc2305_data *data = i2c_get_clientdata(client);
  unsigned long  hsb, lsb;
  unsigned long  tech;
  unsigned long val;
  int ret;

  ret = kstrtoul(buf, 10, &val);
  if (ret)
  {
    return ret;
  }
  if (val > 100)
  {
    return -EINVAL;
  }

  if (val <= 5)
  {
    hsb = 0xff; /*high bit*/
    lsb = 0xe0; /*low bit*/
  }
  else
  {
    val = val * 230;
    tech = (3932160 * 2) / (val > 0 ? val : 1);
    hsb = (uint8_t)(((tech << 3) >> 8) & 0x0ff);
    lsb = (uint8_t)((tech << 3) & 0x0f8);
  }

  mutex_lock(&data->lock);
  i2c_smbus_write_byte_data(client, EMC2305_REG_FAN_REAR_H_RPM(attr->index), hsb);
  i2c_smbus_write_byte_data(client, EMC2305_REG_FAN_REAR_L_RPM(attr->index), lsb);
  mutex_unlock(&data->lock);
  return count;
}


static ssize_t show_fan(struct device *dev, struct device_attribute *devattr,
                        char *buf)
{
  struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
  struct i2c_client *client = to_i2c_client(dev);
  struct emc2305_data *data = i2c_get_clientdata(client);
  int val;


  mutex_lock(&data->lock);
  val = i2c_smbus_read_word_swapped(client,
                                    EMC2305_REG_FAN_TACH(attr->index));
  mutex_unlock(&data->lock);
  /* Left shift 3 bits for showing correct RPM */
  val = val >> 3;
  return sprintf(buf, "%d\n", 3932160 * 2 / (val > 0 ? val : 1));
}

static ssize_t set_fan(struct device *dev, struct device_attribute *devattr,
                       const char *buf, size_t count)
{
  struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
  struct i2c_client *client = to_i2c_client(dev);
  struct emc2305_data *data = i2c_get_clientdata(client);
  unsigned long  hsb, lsb;
  unsigned long  tech;
  unsigned long val;
  int ret;

  ret = kstrtoul(buf, 10, &val);
  if (ret)
  {
    return ret;
  }
  if (val > 23000)
  {
    return -EINVAL;
  }

  if (val <= 960)
  {
    hsb = 0xff; /*high bit*/
    lsb = 0xe0; /*low bit*/
  }
  else
  {
    tech = (3932160 * 2) / (val > 0 ? val : 1);
    hsb = (uint8_t)(((tech << 3) >> 8) & 0x0ff);
    lsb = (uint8_t)((tech << 3) & 0x0f8);
  }

  mutex_lock(&data->lock);
  i2c_smbus_write_byte_data(client, EMC2305_REG_FAN_REAR_H_RPM(attr->index), hsb);
  i2c_smbus_write_byte_data(client, EMC2305_REG_FAN_REAR_L_RPM(attr->index), lsb);
  mutex_unlock(&data->lock);
  return count;
}

static ssize_t show_pwm(struct device *dev, struct device_attribute *devattr,
                        char *buf)
{
  struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
  struct i2c_client *client = to_i2c_client(dev);
  struct emc2305_data *data = i2c_get_clientdata(client);
  int val;

  mutex_lock(&data->lock);
  val = i2c_smbus_read_byte_data(client,
                                 EMC2305_REG_FAN_DRIVE(attr->index));
  mutex_unlock(&data->lock);
  return sprintf(buf, "%d\n", val);
}

static ssize_t set_pwm(struct device *dev, struct device_attribute *devattr,
                       const char *buf, size_t count)
{
  struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
  struct i2c_client *client = to_i2c_client(dev);
  struct emc2305_data *data = i2c_get_clientdata(client);
  unsigned long val;
  int ret;

  ret = kstrtoul(buf, 10, &val);
  if (ret)
  {
    return ret;
  }
  if (val > 255)
  {
    return -EINVAL;
  }

  mutex_lock(&data->lock);
  i2c_smbus_write_byte_data(client,
                            EMC2305_REG_FAN_DRIVE(attr->index),
                            val);
  mutex_unlock(&data->lock);
  return count;
}

static int emc2305_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  struct i2c_adapter *adapter = client->adapter;
  int vendor, device;

  if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA |
                               I2C_FUNC_SMBUS_WORD_DATA))
  {
    return -ENODEV;
  }

  vendor = i2c_smbus_read_byte_data(client, EMC2305_REG_VENDOR);
  if (vendor != EMC2305_VENDOR)
  {
    return -ENODEV;
  }

  device = i2c_smbus_read_byte_data(client, EMC2305_REG_DEVICE);
  if (device != EMC2305_DEVICE)
  {
    return -ENODEV;
  }

  strlcpy(info->type, "emc2305", I2C_NAME_SIZE);

  return 0;
}

static int emc2305_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  struct emc2305_data *data;
  int err;
  int i;

  data = devm_kzalloc(&client->dev, sizeof(struct emc2305_data),
                      GFP_KERNEL);
  if (!data)
  {
    return -ENOMEM;
  }

  i2c_set_clientdata(client, data);
  mutex_init(&data->lock);

  dev_info(&client->dev, "%s chip found\n", client->name);

  data->attrs.attrs = emc2305_attr;
  err = sysfs_create_group(&client->dev.kobj, &data->attrs);
  if (err)
  {
    return err;
  }

  data->hwmon_dev = hwmon_device_register(&client->dev);
  if (IS_ERR(data->hwmon_dev))
  {
    err = PTR_ERR(data->hwmon_dev);
    goto exit_remove;
  }

  for (i = 0; i < 5; i++)
  {
    /* set minimum drive to 0% */
    i2c_smbus_write_byte_data(client, EMC2305_REG_FAN_MIN_DRIVE(i), FAN_MINIMUN);
    i2c_smbus_write_byte_data(client, EMC2305_REG_FAN_CONF(i), FAN_RPM_BASED);
  }

  return 0;

exit_remove:
  sysfs_remove_group(&client->dev.kobj, &data->attrs);
  return err;
}

static int emc2305_remove(struct i2c_client *client)
{
  struct emc2305_data *data = i2c_get_clientdata(client);

  hwmon_device_unregister(data->hwmon_dev);
  sysfs_remove_group(&client->dev.kobj, &data->attrs);
  return 0;
}

module_i2c_driver(emc2305_driver);

MODULE_AUTHOR("Neal Tai<neal.tai@deltaww.com>");
MODULE_DESCRIPTION("SMSC EMC2305 fan controller driver");
MODULE_LICENSE("GPL");
