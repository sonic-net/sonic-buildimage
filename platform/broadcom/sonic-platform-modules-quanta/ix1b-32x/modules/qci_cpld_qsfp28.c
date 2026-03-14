/*
 * A CPLD driver for monitor QSFP28 module I/O
 *
 * The CPLD is customize by Quanta for controlling QSFP28 module signals,
 * they are RESET , INTERREPT , Module_Present, LPMODE
 * Each CPLD control 16 modules, each module use 4 bits in register.
 *
 * Copyright (C) 2015 Quanta Inc.
 *
 * Author: Luffy Cheng <luffy.cheng@quantatw.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/idr.h>

static DEFINE_IDA(cpld_ida);

enum platform_type {
	QSFP28 = 0,
	NONE
};

static struct class *cpld_class = NULL;

struct sfp_data {
	struct i2c_client *cpld_client;
	char name[8];
	u8 port_id;
	u8 cpld_port;
};

struct cpld_data {
	struct mutex lock;
	struct device *port_dev[16];
	struct sfp_data *port_data[16];
};

static int cpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id);
static int cpld_remove(struct i2c_client *client);

static const struct i2c_device_id cpld_id[] = {
	{ "CPLD-QSFP28", QSFP28 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "qci_cpld_qsfp28",
	},
	.probe		= cpld_probe,
	.remove		= cpld_remove,
	.id_table	= cpld_id,
//	.address_list	= normal_i2c,
};

#define CPLD_ID_PREFIX "port-"
#define CPLD_ID_FORMAT CPLD_ID_PREFIX "%d"

#define RESET_MASK 		0x08
#define INTERRUPT_MASK	 	0x04
#define MODULE_PRESENT_MASK 	0x02
#define LPMODE_MASK 		0x01

static inline u8 get_group_cmd(u8 group)
{
	//FIXME: if group cmd change
	return (group + 1);
}

static inline u8 port_remapping(u8 phy_port)
{
	/* FIXME: implement by hardware design */
	/* The CPLD register port mapping is weird :
	 * MSB -------- LSB		(word data)
	 * P3	P4	P1	P2		(per port 4 bits)
	 * For easy coding bit shift, we treat it as hw port swap
	 */
	return (phy_port % 2) ? (phy_port - 1) : (phy_port + 1);
}

static ssize_t get_reset(struct device *dev,
			     struct device_attribute *devattr,
			     char *buf)
{
	struct sfp_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->cpld_client;
	u8 group = (u8)(data->cpld_port / 4);
	u8 group_port = data->cpld_port % 4;
	s32 value;

	dev_dbg(&client->dev, "port_id %d => cpld_port %d, group %d(%d)\n", data->port_id,
				data->cpld_port + 1, group + 1, group_port + 1);

	value = i2c_smbus_read_word_data(client, get_group_cmd(group));
	if (value < 0)
		return -ENODEV;

	dev_dbg(&client->dev, "read group%d value= %x\n", group + 1, value);

	value >>= (group_port * 4);
	value &= RESET_MASK;

	return sprintf(buf, "%d\n", value ? 1 : 0);
}

static ssize_t get_interrupt(struct device *dev,
			     struct device_attribute *devattr,
			     char *buf)
{
	struct sfp_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->cpld_client;
	u8 group = (u8)(data->cpld_port / 4);
	u8 group_port = data->cpld_port % 4;
	s32 value;

	dev_dbg(&client->dev, "port_id %d => cpld_port %d, group %d(%d)\n", data->port_id,
				data->cpld_port + 1, group + 1, group_port + 1);

	value = i2c_smbus_read_word_data(client, get_group_cmd(group));
	if (value < 0)
		return -ENODEV;

	dev_dbg(&client->dev, "read group%d value= %x\n", group + 1, value);

	value >>= (group_port * 4);
	value &= INTERRUPT_MASK;

	return sprintf(buf, "%d\n", value ? 1 : 0);
}

static ssize_t get_module_present(struct device *dev,
			     struct device_attribute *devattr,
			     char *buf)
{
	struct sfp_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->cpld_client;
	u8 group = (u8)(data->cpld_port / 4);
	u8 group_port = data->cpld_port % 4;
	s32 value;

	dev_dbg(&client->dev, "port_id %d => cpld_port %d, group %d(%d)\n", data->port_id,
				data->cpld_port + 1, group + 1, group_port + 1);

	value = i2c_smbus_read_word_data(client, get_group_cmd(group));
	if (value < 0)
		return -ENODEV;

	dev_dbg(&client->dev, "read group%d value= %x\n", group + 1, value);

	value >>= (group_port * 4);
	value &= MODULE_PRESENT_MASK;

	//FIXME: if present is not low active
	return sprintf(buf, "%d\n", value ? 0 : 1);
}

static ssize_t get_lpmode(struct device *dev,
			     struct device_attribute *devattr,
			     char *buf)
{
	struct sfp_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->cpld_client;
	u8 group = (u8)(data->cpld_port / 4);
	u8 group_port = data->cpld_port % 4;
	s32 value;

	dev_dbg(&client->dev, "port_id %d => cpld_port %d, group %d(%d)\n", data->port_id,
				data->cpld_port + 1, group + 1, group_port + 1);

	value = i2c_smbus_read_word_data(client, get_group_cmd(group));
	if (value < 0)
		return -ENODEV;

	dev_dbg(&client->dev, "read group%d value= %x\n", group + 1, value);

	value >>= (group_port * 4);
	value &= LPMODE_MASK;

	return sprintf(buf, "%d\n", value ? 1 : 0);
}

static ssize_t set_reset(struct device *dev,
			    struct device_attribute *devattr,
			    const char *buf,
			    size_t count)
{
	struct sfp_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->cpld_client;
	u8 group = (u8)(data->cpld_port / 4);
	u8 group_port = data->cpld_port % 4;
	s32 value;
	long disable;

	dev_dbg(&client->dev, "port_id %d => cpld_port %d, group %d(%d)\n", data->port_id,
				data->cpld_port + 1, group + 1, group_port + 1);

	if (kstrtol(buf, 0, &disable))
		return -EINVAL;

	if ((disable != 1) && (disable != 0))
		return -EINVAL;

//	mutex_lock(&data->lock);
	value = i2c_smbus_read_word_data(client, get_group_cmd(group));
	if (value < 0)
		return -ENODEV;

	dev_dbg(&client->dev, "read group%d value= %x\n", group + 1, value);

	value &= ~(RESET_MASK << (group_port * 4));
	if (disable)
		value |= (RESET_MASK << (group_port * 4));

	dev_dbg(&client->dev, "write group%d value= %x\n", group + 1, value);

	i2c_smbus_write_word_data(client, get_group_cmd(group), (u16)value);
//	mutex_unlock(&data->lock);

	return count;
}

static ssize_t set_lpmode(struct device *dev,
			    struct device_attribute *devattr,
			    const char *buf,
			    size_t count)
{
	struct sfp_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->cpld_client;
	u8 group = (u8)(data->cpld_port / 4);
	u8 group_port = data->cpld_port % 4;
	s32 value;
	long disable;

	dev_dbg(&client->dev, "port_id %d => cpld_port %d, group %d(%d)\n", data->port_id,
				data->cpld_port + 1, group + 1, group_port + 1);

	if (kstrtol(buf, 0, &disable))
		return -EINVAL;

	if ((disable != 1) && (disable != 0))
		return -EINVAL;

//	mutex_lock(&data->lock);
	value = i2c_smbus_read_word_data(client, get_group_cmd(group));
	if (value < 0)
		return -ENODEV;

	dev_dbg(&client->dev, "read group%d value= %x\n", group + 1, value);

	value &= ~(LPMODE_MASK << (group_port * 4));
	if (disable)
		value |= (LPMODE_MASK << (group_port * 4));

	dev_dbg(&client->dev, "write group%d value= %x\n", group + 1, value);

	i2c_smbus_write_word_data(client, get_group_cmd(group), (u16)value);
//	mutex_unlock(&data->lock);

	return count;
}

static DEVICE_ATTR(reset, S_IWUSR | S_IRUGO, get_reset, set_reset);
static DEVICE_ATTR(lpmode, S_IWUSR | S_IRUGO, get_lpmode, set_lpmode);
static DEVICE_ATTR(module_present, S_IRUGO, get_module_present, NULL);
static DEVICE_ATTR(interrupt, S_IRUGO, get_interrupt, NULL);

static const struct attribute *sfp_attrs[] = {
	&dev_attr_reset.attr,
	&dev_attr_lpmode.attr,
	&dev_attr_module_present.attr,
	&dev_attr_interrupt.attr,
	NULL,
};

static const struct attribute_group sfp_attr_group = {
	.attrs = (struct attribute **) sfp_attrs,
};

static int cpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct cpld_data *data;
	struct sfp_data *port_data;
	struct device *port_dev;
	int port_nr, i, err;

	if (!cpld_class)
	{
		cpld_class = class_create(THIS_MODULE, "cpld-qsfp28");
		if (IS_ERR(cpld_class)) {
			pr_err("couldn't create sysfs class\n");
			return PTR_ERR(cpld_class);
		}
	}

	data = devm_kzalloc(&client->dev, sizeof(struct cpld_data),
			    GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* register sfp port data to sysfs */
	for (i = 0; i < 16; i++)
	{
		port_nr = ida_simple_get(&cpld_ida, 1, 99, GFP_KERNEL);
		if (port_nr < 0)
			return ERR_PTR(port_nr);

		port_data = kzalloc(sizeof(struct sfp_data), GFP_KERNEL);

		port_dev = device_create(cpld_class, &client->dev, MKDEV(0,0), port_data, CPLD_ID_FORMAT, port_nr);
		if (IS_ERR(port_dev)) {
			err = PTR_ERR(port_dev);
			// printk("err_status\n");
		}

		data->port_dev[i] = port_dev;
		data->port_data[i] = port_data;

		dev_info(&client->dev, "Register qsfp28 port-%d\n", port_nr);

		/* FIXME: implement Logical/Physical port remapping */
	 	//port_data->cpld_port = i;
		port_data->cpld_port = port_remapping(i);
		sprintf(port_data->name, "port-%d", port_nr);
		port_data->port_id = port_nr;
		dev_set_drvdata(port_dev, port_data);
		port_dev->init_name = port_data->name;
		port_data->cpld_client = client;

		err = sysfs_create_group(&port_dev->kobj, &sfp_attr_group);
		// if (status)	printk("err status\n");
	}

	i2c_set_clientdata(client, data);
	mutex_init(&data->lock);

	dev_info(&client->dev, "%s device found\n", client->name);


	return 0;

//FIXME: implement error check
exit_remove:
//	sysfs_remove_group(&client->dev.kobj, &data->attrs);
	return err;
}

/* FIXME: for older kernel doesn't with idr_is_empty function, implement here */
static int idr_has_entry(int id, void *p, void *data)
{
	return 1;
}

static bool cpld_idr_is_empty(struct idr *idp)
{
	return !idr_for_each(idp, idr_has_entry, NULL);
}

static int cpld_remove(struct i2c_client *client)
{
	struct cpld_data *data = i2c_get_clientdata(client);
	int i, id;

	for (i = 15; i >= 0; i--)
	{
		dev_info(data->port_dev[i], "Remove qsfp28 port-%d\n", data->port_data[i]->port_id);
		device_unregister(data->port_dev[i]);
		ida_simple_remove(&cpld_ida, data->port_data[i]->port_id);
		kfree(data->port_data[i]);
	}

	if (ida_is_empty(&cpld_ida))
	{
		class_destroy(cpld_class);
		cpld_class = NULL;
	}

	return 0;
}

module_i2c_driver(cpld_driver);

MODULE_AUTHOR("Luffy Cheng <luffy.cheng@quantatw.com>");
MODULE_DESCRIPTION("Quanta Switch SFP CPLD driver");
MODULE_LICENSE("GPL");
