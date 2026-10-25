/*
 * A hwmon driver for the as7535_32xb_cpld
 *
 * Copyright (C) 2019  Edgecore Networks Corporation.
 * Jostar Yang <brandon_chuang@edge-core.com>
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

#define DRVNAME "as7535_32xb_cpld"

static LIST_HEAD(cpld_client_list);
static struct mutex	 list_lock;

struct cpld_client_node {
	struct i2c_client *client;
	struct list_head list;
};

enum cpld_type {
	as7535_32xb_cpld
};

#define I2C_RW_RETRY_COUNT 10
#define I2C_RW_RETRY_INTERVAL 60 /* ms */

static ssize_t show_status(struct device *dev, struct device_attribute *da,
			 char *buf);
static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t set_tx_disable(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
			char *buf);

struct as7535_32xb_cpld_data {
	struct mutex update_lock;
	u8 index; /* CPLD index */
};

/* Addresses scanned for as7535_32xb_cpld
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

#define TRANSCEIVER_PRESENT_ATTR_ID(index) MODULE_PRESENT_##index
#define TRANSCEIVER_RESET_ATTR_ID(index) MODULE_RESET_##index
#define TRANSCEIVER_RXLOS_ATTR_ID(index) MODULE_RXLOS_##index
#define TRANSCEIVER_TXFAULT_ATTR_ID(index) MODULE_TXFAULT_##index
#define TRANSCEIVER_TXDISABLE_ATTR_ID(index) MODULE_TXDISABLE_##index

enum as7535_32xb_cpld_sysfs_attributes {
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
	TRANSCEIVER_PRESENT_ATTR_ID(28),
	TRANSCEIVER_PRESENT_ATTR_ID(30),
	TRANSCEIVER_PRESENT_ATTR_ID(27),
	TRANSCEIVER_PRESENT_ATTR_ID(29),
	TRANSCEIVER_TXDISABLE_ATTR_ID(1),
	TRANSCEIVER_TXDISABLE_ATTR_ID(2),
	TRANSCEIVER_TXDISABLE_ATTR_ID(3),
	TRANSCEIVER_TXDISABLE_ATTR_ID(4),
	TRANSCEIVER_TXDISABLE_ATTR_ID(5),
	TRANSCEIVER_TXDISABLE_ATTR_ID(6),
	TRANSCEIVER_TXDISABLE_ATTR_ID(7),
	TRANSCEIVER_TXDISABLE_ATTR_ID(8),
	TRANSCEIVER_TXDISABLE_ATTR_ID(9),
	TRANSCEIVER_TXDISABLE_ATTR_ID(10),
	TRANSCEIVER_TXDISABLE_ATTR_ID(11),
	TRANSCEIVER_TXDISABLE_ATTR_ID(12),
	TRANSCEIVER_TXDISABLE_ATTR_ID(13),
	TRANSCEIVER_TXDISABLE_ATTR_ID(14),
	TRANSCEIVER_TXDISABLE_ATTR_ID(15),
	TRANSCEIVER_TXDISABLE_ATTR_ID(16),
	TRANSCEIVER_TXDISABLE_ATTR_ID(17),
	TRANSCEIVER_TXDISABLE_ATTR_ID(18),
	TRANSCEIVER_TXDISABLE_ATTR_ID(19),
	TRANSCEIVER_TXDISABLE_ATTR_ID(20),
	TRANSCEIVER_TXDISABLE_ATTR_ID(21),
	TRANSCEIVER_TXDISABLE_ATTR_ID(22),
	TRANSCEIVER_TXDISABLE_ATTR_ID(23),
	TRANSCEIVER_TXDISABLE_ATTR_ID(24),
	TRANSCEIVER_TXDISABLE_ATTR_ID(25),
	TRANSCEIVER_TXDISABLE_ATTR_ID(26),
	TRANSCEIVER_RXLOS_ATTR_ID(1),
	TRANSCEIVER_RXLOS_ATTR_ID(2),
	TRANSCEIVER_RXLOS_ATTR_ID(3),
	TRANSCEIVER_RXLOS_ATTR_ID(4),
	TRANSCEIVER_RXLOS_ATTR_ID(5),
	TRANSCEIVER_RXLOS_ATTR_ID(6),
	TRANSCEIVER_RXLOS_ATTR_ID(7),
	TRANSCEIVER_RXLOS_ATTR_ID(8),
	TRANSCEIVER_RXLOS_ATTR_ID(9),
	TRANSCEIVER_RXLOS_ATTR_ID(10),
	TRANSCEIVER_RXLOS_ATTR_ID(11),
	TRANSCEIVER_RXLOS_ATTR_ID(12),
	TRANSCEIVER_RXLOS_ATTR_ID(13),
	TRANSCEIVER_RXLOS_ATTR_ID(14),
	TRANSCEIVER_RXLOS_ATTR_ID(15),
	TRANSCEIVER_RXLOS_ATTR_ID(16),
	TRANSCEIVER_RXLOS_ATTR_ID(17),
	TRANSCEIVER_RXLOS_ATTR_ID(18),
	TRANSCEIVER_RXLOS_ATTR_ID(19),
	TRANSCEIVER_RXLOS_ATTR_ID(20),
	TRANSCEIVER_RXLOS_ATTR_ID(21),
	TRANSCEIVER_RXLOS_ATTR_ID(22),
	TRANSCEIVER_RXLOS_ATTR_ID(23),
	TRANSCEIVER_RXLOS_ATTR_ID(24),
	TRANSCEIVER_RXLOS_ATTR_ID(25),
	TRANSCEIVER_RXLOS_ATTR_ID(26),
	TRANSCEIVER_TXFAULT_ATTR_ID(1),
	TRANSCEIVER_TXFAULT_ATTR_ID(2),
	TRANSCEIVER_TXFAULT_ATTR_ID(3),
	TRANSCEIVER_TXFAULT_ATTR_ID(4),
	TRANSCEIVER_TXFAULT_ATTR_ID(5),
	TRANSCEIVER_TXFAULT_ATTR_ID(6),
	TRANSCEIVER_TXFAULT_ATTR_ID(7),
	TRANSCEIVER_TXFAULT_ATTR_ID(8),
	TRANSCEIVER_TXFAULT_ATTR_ID(9),
	TRANSCEIVER_TXFAULT_ATTR_ID(10),
	TRANSCEIVER_TXFAULT_ATTR_ID(11),
	TRANSCEIVER_TXFAULT_ATTR_ID(12),
	TRANSCEIVER_TXFAULT_ATTR_ID(13),
	TRANSCEIVER_TXFAULT_ATTR_ID(14),
	TRANSCEIVER_TXFAULT_ATTR_ID(15),
	TRANSCEIVER_TXFAULT_ATTR_ID(16),
	TRANSCEIVER_TXFAULT_ATTR_ID(17),
	TRANSCEIVER_TXFAULT_ATTR_ID(18),
	TRANSCEIVER_TXFAULT_ATTR_ID(19),
	TRANSCEIVER_TXFAULT_ATTR_ID(20),
	TRANSCEIVER_TXFAULT_ATTR_ID(21),
	TRANSCEIVER_TXFAULT_ATTR_ID(22),
	TRANSCEIVER_TXFAULT_ATTR_ID(23),
	TRANSCEIVER_TXFAULT_ATTR_ID(24),
	TRANSCEIVER_TXFAULT_ATTR_ID(25),
	TRANSCEIVER_TXFAULT_ATTR_ID(26),
	TRANSCEIVER_RESET_ATTR_ID(28),
	TRANSCEIVER_RESET_ATTR_ID(30),
	TRANSCEIVER_RESET_ATTR_ID(27),
	TRANSCEIVER_RESET_ATTR_ID(29),
	MODULE_PRESENT_ALL,
	MODULE_RXLOS_ALL,
	CPLD_VERSION,
	ACCESS,
};

/* sysfs attributes for hwmon
 */

/* qsfp transceiver attributes */
#define DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_status, \
								NULL, MODULE_PRESENT_##index); \
	static SENSOR_DEVICE_ATTR(module_reset_##index, S_IRUGO | S_IWUSR, \
								show_status, set_control, MODULE_RESET_##index)
#define DECLARE_QSFP_TRANSCEIVER_ATTR(index) \
	&sensor_dev_attr_module_present_##index.dev_attr.attr, \
	&sensor_dev_attr_module_reset_##index.dev_attr.attr

/* sfp transceiver attributes */
#define DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_status, \
								NULL, MODULE_PRESENT_##index); \
	static SENSOR_DEVICE_ATTR(module_tx_disable_##index, S_IRUGO | S_IWUSR, \
								show_status, set_tx_disable, \
								MODULE_TXDISABLE_##index); \
	static SENSOR_DEVICE_ATTR(module_tx_fault_##index, S_IRUGO, show_status, \
								NULL, MODULE_TXFAULT_##index); \
	static SENSOR_DEVICE_ATTR(module_rx_los_##index, S_IRUGO, show_status, \
								NULL, MODULE_RXLOS_##index)

#define DECLARE_SFP_TRANSCEIVER_ATTR(index) \
	&sensor_dev_attr_module_present_##index.dev_attr.attr, \
	&sensor_dev_attr_module_tx_disable_##index.dev_attr.attr, \
	&sensor_dev_attr_module_tx_fault_##index.dev_attr.attr, \
	&sensor_dev_attr_module_rx_los_##index.dev_attr.attr

static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);

static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, show_present_all, \
						NULL, MODULE_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(module_rx_los_all, S_IRUGO, show_rxlos_all, \
						NULL, MODULE_RXLOS_ALL);

/* transceiver attributes */
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(1);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(2);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(3);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(4);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(5);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(6);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(7);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(8);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(9);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(10);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(11);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(12);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(13);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(14);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(15);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(16);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(17);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(18);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(19);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(20);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(21);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(22);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(23);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(24);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(25);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(26);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(27);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(28);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(29);
DECLARE_QSFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(30);

static struct attribute *as7535_32xb_cpld_attributes[] = {
	/* transceiver attributes */
	DECLARE_SFP_TRANSCEIVER_ATTR(1),
	DECLARE_SFP_TRANSCEIVER_ATTR(2),
	DECLARE_SFP_TRANSCEIVER_ATTR(3),
	DECLARE_SFP_TRANSCEIVER_ATTR(4),
	DECLARE_SFP_TRANSCEIVER_ATTR(5),
	DECLARE_SFP_TRANSCEIVER_ATTR(6),
	DECLARE_SFP_TRANSCEIVER_ATTR(7),
	DECLARE_SFP_TRANSCEIVER_ATTR(8),
	DECLARE_SFP_TRANSCEIVER_ATTR(9),
	DECLARE_SFP_TRANSCEIVER_ATTR(10),
	DECLARE_SFP_TRANSCEIVER_ATTR(11),
	DECLARE_SFP_TRANSCEIVER_ATTR(12),
	DECLARE_SFP_TRANSCEIVER_ATTR(13),
	DECLARE_SFP_TRANSCEIVER_ATTR(14),
	DECLARE_SFP_TRANSCEIVER_ATTR(15),
	DECLARE_SFP_TRANSCEIVER_ATTR(16),
	DECLARE_SFP_TRANSCEIVER_ATTR(17),
	DECLARE_SFP_TRANSCEIVER_ATTR(18),
	DECLARE_SFP_TRANSCEIVER_ATTR(19),
	DECLARE_SFP_TRANSCEIVER_ATTR(20),
	DECLARE_SFP_TRANSCEIVER_ATTR(21),
	DECLARE_SFP_TRANSCEIVER_ATTR(22),
	DECLARE_SFP_TRANSCEIVER_ATTR(23),
	DECLARE_SFP_TRANSCEIVER_ATTR(24),
	DECLARE_SFP_TRANSCEIVER_ATTR(25),
	DECLARE_SFP_TRANSCEIVER_ATTR(26),
	DECLARE_QSFP_TRANSCEIVER_ATTR(27),
	DECLARE_QSFP_TRANSCEIVER_ATTR(28),
	DECLARE_QSFP_TRANSCEIVER_ATTR(29),
	DECLARE_QSFP_TRANSCEIVER_ATTR(30),
	&sensor_dev_attr_version.dev_attr.attr,
	&sensor_dev_attr_access.dev_attr.attr,
	&sensor_dev_attr_module_present_all.dev_attr.attr,
	&sensor_dev_attr_module_rx_los_all.dev_attr.attr,
	NULL
};

static const struct attribute_group as7535_32xb_cpld_group = {
	.attrs = as7535_32xb_cpld_attributes,
};

static const struct attribute_group* cpld_groups[] = {
	&as7535_32xb_cpld_group
};

int as7535_32xb_cpld_read(int bus_num, unsigned short cpld_addr, u8 reg)
{
	struct list_head *list_node = NULL;
	struct cpld_client_node *cpld_node = NULL;
	int ret = -EPERM;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list)
	{
		cpld_node = list_entry(list_node, struct cpld_client_node, list);

		if (cpld_node->client->addr == cpld_addr
			&& cpld_node->client->adapter->nr == bus_num) {
			ret = i2c_smbus_read_byte_data(cpld_node->client, reg);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(as7535_32xb_cpld_read);

int as7535_32xb_cpld_write(int bus_num, unsigned short cpld_addr, u8 reg, u8 value)
{
	struct list_head *list_node = NULL;
	struct cpld_client_node *cpld_node = NULL;
	int ret = -EIO;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list)
	{
		cpld_node = list_entry(list_node, struct cpld_client_node, list);

		if (cpld_node->client->addr == cpld_addr
			&& cpld_node->client->adapter->nr == bus_num) {
			ret = i2c_smbus_write_byte_data(cpld_node->client, reg, value);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(as7535_32xb_cpld_write);

static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
			char *buf)
{
	int i, status;
	u8 regs_cpld[] = { 0x11, 0x12, 0x13, 0x14, 0x10 };
	u8 values[ARRAY_SIZE(regs_cpld)] = { 0 };
	u8 *regs[] = { regs_cpld };
	u8  size[] = { ARRAY_SIZE(regs_cpld) };
	struct i2c_client *client = to_i2c_client(dev);
	struct as7535_32xb_cpld_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	for (i = 0; i < size[data->index]; i++) {
		status = as7535_32xb_cpld_read(12, 0x61, regs[data->index][i]);
		if (status < 0)
			goto exit;

		values[i] = ~(u8)status;
	}

	mutex_unlock(&data->update_lock);

	/* Return values in order */
	return sprintf(buf, "%.2x %.2x %.2x %.2x\n",values[0],values[1],values[2],
	(unsigned int)(((values[3] & 0x3)) | ((values[4] & BIT(2)) >> 2) |
			((values[4] & BIT(0)) << 1) | ((values[4] & BIT(3)) >> 1) |
			((values[4] & BIT(1)) << 2)));

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t show_rxlos_all(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int i, status;
	u8 regs_cpld[] = { 0x21, 0x22, 0x23, 0x24 };
	u8 values[ARRAY_SIZE(regs_cpld)] = { 0 };
	u8 *regs[] = { regs_cpld };
	u8  size[] = { ARRAY_SIZE(regs_cpld) };
	struct i2c_client *client = to_i2c_client(dev);
	struct as7535_32xb_cpld_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	for (i = 0; i < size[data->index]; i++) {
		status = as7535_32xb_cpld_read(12, 0x61, regs[data->index][i]);
		if (status < 0)
			goto exit;

		values[i] = (u8)status;
	}

	mutex_unlock(&data->update_lock);

	/* Return values in order */
	return sprintf(buf, "%.2x %.2x %.2x %.2x\n",
						values[0], values[1], values[2], (values[3] & 0x3));

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t show_status(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct as7535_32xb_cpld_data *data = i2c_get_clientdata(client);
	int status = 0;
	u8 reg = 0, mask = 0, invert = 0;

	switch (attr->index) {
	case MODULE_PRESENT_1 ... MODULE_PRESENT_8:
		invert = 1;
		reg = 0x11;
		mask = 0x1 << (attr->index - MODULE_PRESENT_1);
		break;
	case MODULE_PRESENT_9 ... MODULE_PRESENT_16:
		invert = 1;
		reg = 0x12;
		mask = 0x1 << (attr->index - MODULE_PRESENT_9);
		break;
	case MODULE_PRESENT_17 ... MODULE_PRESENT_24:
		invert = 1;
		reg = 0x13;
		mask = 0x1 << (attr->index - MODULE_PRESENT_17);
		break;
	case MODULE_PRESENT_25 ... MODULE_PRESENT_26:
		invert = 1;
		reg = 0x14;
		mask = 0x1 << (attr->index - MODULE_PRESENT_25);
		break;
	case MODULE_PRESENT_28 ... MODULE_PRESENT_29:
		invert = 1;
		reg = 0x10;
		mask = 0x1 << (attr->index - MODULE_PRESENT_28);
		break;
	case MODULE_TXDISABLE_1 ... MODULE_TXDISABLE_8:
		reg = 0xA;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_1);
		break;
	case MODULE_TXDISABLE_9 ... MODULE_TXDISABLE_16:
		reg = 0xB;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_9);
		break;
	case MODULE_TXDISABLE_17 ... MODULE_TXDISABLE_24:
		reg = 0xC;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_17);
		break;
	case MODULE_TXDISABLE_25 ... MODULE_TXDISABLE_26:
		reg = 0xD;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_25);
		break;
	case MODULE_TXFAULT_1 ... MODULE_TXFAULT_8:
		reg = 0x15;
		mask = 0x1 << (attr->index - MODULE_TXFAULT_1);
		break;
	case MODULE_TXFAULT_9 ... MODULE_TXFAULT_16:
		reg = 0x16;
		mask = 0x1 << (attr->index - MODULE_TXFAULT_9);
		break;
	case MODULE_TXFAULT_17 ... MODULE_TXFAULT_24:
		reg = 0x17;
		mask = 0x1 << (attr->index - MODULE_TXFAULT_17);
		break;
	case MODULE_TXFAULT_25 ... MODULE_TXFAULT_26:
		reg = 0x18;
		mask = 0x1 << (attr->index - MODULE_TXFAULT_25);
		break;
	case MODULE_RXLOS_1 ... MODULE_RXLOS_8:
		reg = 0x21;
		mask = 0x1 << (attr->index - MODULE_RXLOS_1);
		break;
	case MODULE_RXLOS_9 ... MODULE_RXLOS_16:
		reg = 0x22;
		mask = 0x1 << (attr->index - MODULE_RXLOS_9);
		break;
	case MODULE_RXLOS_17 ... MODULE_RXLOS_24:
		reg = 0x23;
		mask = 0x1 << (attr->index - MODULE_RXLOS_17);
		break;
	case MODULE_RXLOS_25 ... MODULE_RXLOS_26:
		reg = 0x24;
		mask = 0x1 << (attr->index - MODULE_RXLOS_25);
		break;
	case MODULE_RESET_28 ... MODULE_RESET_29:
		invert = 1;
		reg = 0x8;
		mask = 0x1 << (attr->index - MODULE_RESET_28);
		break;
 	default:
		return -ENXIO;
	}

	mutex_lock(&data->update_lock);

	status = as7535_32xb_cpld_read(12, 0x61, reg);
	if (unlikely(status < 0))
		goto exit;

	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", invert? !(status & mask): !!(status & mask));

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t set_tx_disable(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct as7535_32xb_cpld_data *data = i2c_get_clientdata(client);
	long disable;
	int status, val;
	u8 reg = 0, mask = 0;

	status = kstrtol(buf, 10, &disable);
	if (status)
		return status;

	switch (attr->index) {
	case MODULE_TXDISABLE_1 ... MODULE_TXDISABLE_8:
		reg = 0xA;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_1);
		break;
	case MODULE_TXDISABLE_9 ... MODULE_TXDISABLE_16:
		reg = 0xB;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_9);
		break;
	case MODULE_TXDISABLE_17 ... MODULE_TXDISABLE_24:
		reg = 0xC;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_17);
		break;
	case MODULE_TXDISABLE_25 ... MODULE_TXDISABLE_26:
		reg = 0xD;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_25);
		break;
	default:
		return 0;
	}
	/* Read current status */
	mutex_lock(&data->update_lock);
	val = as7535_32xb_cpld_read(12, 0x61, reg);
	if (unlikely(status < 0))
		goto exit;

	/* Update tx_disable status */
	if (disable)
		val |= mask;
	else
		val &= ~mask;

	status = as7535_32xb_cpld_write(12, 0x61, reg, val);
	if (unlikely(status < 0))
		goto exit;

	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct as7535_32xb_cpld_data *data = i2c_get_clientdata(client);
	long reset;
	int status;
	u8 reg = 0, mask = 0;

	status = kstrtol(buf, 10, &reset);
	if (status)
		return status;

	switch (attr->index) {
	case MODULE_RESET_28 ... MODULE_RESET_29:
		reg = 0x8;
		mask = 0x1 << (attr->index - MODULE_RESET_28);
		break;
	default:
		return -ENXIO;
	}

	mutex_lock(&data->update_lock);
	/* Read current status */
	status = as7535_32xb_cpld_read(12, 0x61, reg);
	if (unlikely(status < 0))
		goto exit;

	/* Update reset status */
	if (reset)
		status &= ~mask;
	else
		status |= mask;

	status = as7535_32xb_cpld_write(12, 0x61, reg, status);
	if (unlikely(status < 0))
		goto exit;

	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static void as7535_32xb_cpld_add_client(struct i2c_client *client)
{
	struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node),
											GFP_KERNEL);

	if (!node) {
		dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n",
								client->addr);
		return;
	}

	node->client = client;

	mutex_lock(&list_lock);
	list_add(&node->list, &cpld_client_list);
	mutex_unlock(&list_lock);
}

static void as7535_32xb_cpld_remove_client(struct i2c_client *client)
{
	struct list_head *list_node = NULL;
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

static ssize_t access(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	int status;
	u32 reg, val;
	struct i2c_client *client = to_i2c_client(dev);
	struct as7535_32xb_cpld_data *data = i2c_get_clientdata(client);

	if (sscanf(buf, "0x%x 0x%x", &reg, &val) != 2)
		return -EINVAL;

	if (reg > 0xFF || val > 0xFF)
		return -EINVAL;

	mutex_lock(&data->update_lock);

	status = as7535_32xb_cpld_write(12, 0x61, reg, val);
	if (unlikely(status < 0))
		goto exit;

	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t show_version(struct device *dev, struct device_attribute *da,
							char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct as7535_32xb_cpld_data *data = i2c_get_clientdata(client);
	int status = 0;
	u8 bus, addr, reg;

	switch (attr->index) {
	case CPLD_VERSION:
		bus = 12;
		addr = 0x61;
		reg = 0x1;
		break;
	default:
		return -ENXIO;
	}

	mutex_lock(&data->update_lock);

	status = as7535_32xb_cpld_read(bus, addr, reg);
	if (unlikely(status < 0)) {
		mutex_unlock(&data->update_lock);
		goto exit;
	}

	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", status);
exit:
	return status;
}


static int as7535_32xb_cpld_probe(struct i2c_client *client,
			const struct i2c_device_id *dev_id)
{
	int status;
	struct as7535_32xb_cpld_data *data = NULL;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n",
								client->addr);
		status = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct as7535_32xb_cpld_data), GFP_KERNEL);
	if (!data) {
		status = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	data->index = dev_id->driver_data;
	mutex_init(&data->update_lock);
	dev_info(&client->dev, "chip found\n");

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, cpld_groups[data->index]);
	if (status)
		goto exit_free;

	as7535_32xb_cpld_add_client(client);

	dev_info(&client->dev, "%s: cpld '%s'\n",
							dev_name(&client->dev), client->name);

	return 0;

exit_free:
	kfree(data);
exit:

	return status;
}

static int as7535_32xb_cpld_remove(struct i2c_client *client)
{
	struct as7535_32xb_cpld_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, cpld_groups[data->index]);
	kfree(data);
	as7535_32xb_cpld_remove_client(client);

	return 0;
}

static const struct i2c_device_id as7535_32xb_cpld_id[] = {
	{ "as7535_32xb_cpld", as7535_32xb_cpld },
	{}
};
MODULE_DEVICE_TABLE(i2c, as7535_32xb_cpld_id);

static struct i2c_driver as7535_32xb_cpld_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name = DRVNAME,
	},
	.probe = as7535_32xb_cpld_probe,
	.remove = as7535_32xb_cpld_remove,
	.id_table = as7535_32xb_cpld_id,
	.address_list = normal_i2c,
};

static int __init as7535_32xb_cpld_init(void)
{
	mutex_init(&list_lock);
	return i2c_add_driver(&as7535_32xb_cpld_driver);
}

static void __exit as7535_32xb_cpld_exit(void)
{
	i2c_del_driver(&as7535_32xb_cpld_driver);
}

module_init(as7535_32xb_cpld_init);
module_exit(as7535_32xb_cpld_exit);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@edge-core.com>");
MODULE_DESCRIPTION("as7535_32xb_cpld driver");
MODULE_LICENSE("GPL");
