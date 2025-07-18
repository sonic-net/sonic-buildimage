// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This module supports the accton cpld that hold the channel select
 * mechanism for other i2c slave devices, such as SFP.
 * This includes the:
 *	 Accton es9618xx CPLD1/CPLD2
 *
 * Based on:
 *	pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *	pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *	i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *	pca9540.c from Jean Delvare <khali@linux-fr.org>.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>

#define I2C_RW_RETRY_COUNT				10
#define I2C_RW_RETRY_INTERVAL			60 /* ms */

static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

enum cpld_type {
    es9618xx_cpld1,
    es9618xx_cpld2
};

struct es9618xx_cpld_data {
    enum cpld_type   type;
    struct device   *hwmon_dev;
    struct mutex     update_lock;
};

static const struct i2c_device_id es9618xx_cpld_id[] = {
    { "es9618xx_cpld1", es9618xx_cpld1 },
    { "es9618xx_cpld2", es9618xx_cpld2 },
    { }
};
MODULE_DEVICE_TABLE(i2c, es9618xx_cpld_id);

#define TRANSCEIVER_PRESENT_ATTR_ID(index)   	MODULE_PRESENT_##index
#define TRANSCEIVER_RESET_ATTR_ID(index)        MODULE_RESET_##index
#define TRANSCEIVER_LPMODE_ATTR_ID(index)        MODULE_LPMODE_##index

enum es9618xx_cpld_sysfs_attributes {
	BOARD_ID,
	CPLD_VERSION,
	ACCESS,
	MODULE_PRESENT_ALL,
	MODULE_RXLOS_ALL,
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
	TRANSCEIVER_RESET_ATTR_ID(1),
	TRANSCEIVER_RESET_ATTR_ID(2),
	TRANSCEIVER_RESET_ATTR_ID(3),
	TRANSCEIVER_RESET_ATTR_ID(4),
	TRANSCEIVER_RESET_ATTR_ID(5),
	TRANSCEIVER_RESET_ATTR_ID(6),
	TRANSCEIVER_RESET_ATTR_ID(7),
	TRANSCEIVER_RESET_ATTR_ID(8),
	TRANSCEIVER_RESET_ATTR_ID(9),
	TRANSCEIVER_RESET_ATTR_ID(10),
	TRANSCEIVER_RESET_ATTR_ID(11),
	TRANSCEIVER_RESET_ATTR_ID(12),
	TRANSCEIVER_RESET_ATTR_ID(13),
	TRANSCEIVER_RESET_ATTR_ID(14),
	TRANSCEIVER_RESET_ATTR_ID(15),
	TRANSCEIVER_RESET_ATTR_ID(16),
	TRANSCEIVER_LPMODE_ATTR_ID(1),
	TRANSCEIVER_LPMODE_ATTR_ID(2),
	TRANSCEIVER_LPMODE_ATTR_ID(3),
	TRANSCEIVER_LPMODE_ATTR_ID(4),
	TRANSCEIVER_LPMODE_ATTR_ID(5),
	TRANSCEIVER_LPMODE_ATTR_ID(6),
	TRANSCEIVER_LPMODE_ATTR_ID(7),
	TRANSCEIVER_LPMODE_ATTR_ID(8),
	TRANSCEIVER_LPMODE_ATTR_ID(9),
	TRANSCEIVER_LPMODE_ATTR_ID(10),
	TRANSCEIVER_LPMODE_ATTR_ID(11),
	TRANSCEIVER_LPMODE_ATTR_ID(12),
	TRANSCEIVER_LPMODE_ATTR_ID(13),
	TRANSCEIVER_LPMODE_ATTR_ID(14),
	TRANSCEIVER_LPMODE_ATTR_ID(15),
	TRANSCEIVER_LPMODE_ATTR_ID(16)
};

/* sysfs attributes for hwmon
 */
static ssize_t show_status(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t show_board_id(struct device *dev, struct device_attribute *da,
             char *buf);
static int es9618xx_cpld_read_internal(struct i2c_client *client, u8 reg);
static int es9618xx_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value);

/* transceiver attributes */
#define DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_status, NULL, MODULE_PRESENT_##index); \
	static SENSOR_DEVICE_ATTR(module_reset_##index, S_IRUGO | S_IWUSR, show_status, set_control, MODULE_RESET_##index); \
	static SENSOR_DEVICE_ATTR(module_lpmode_##index, S_IRUGO | S_IWUSR, show_status, set_control, MODULE_LPMODE_##index)

#define DECLARE_TRANSCEIVER_PRESENT_ATTR(index) &sensor_dev_attr_module_present_##index.dev_attr.attr
#define DECLARE_TRANSCEIVER_RESET_ATTR(index)   &sensor_dev_attr_module_reset_##index.dev_attr.attr
#define DECLARE_TRANSCEIVER_LPMODE_ATTR(index)   &sensor_dev_attr_module_lpmode_##index.dev_attr.attr

static SENSOR_DEVICE_ATTR(board_id, S_IRUGO, show_board_id, NULL, BOARD_ID);
static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);

/* transceiver attributes */
static SENSOR_DEVICE_ATTR(module_present_all, S_IRUGO, show_present_all, NULL, MODULE_PRESENT_ALL);

DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(1);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(2);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(3);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(4);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(5);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(6);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(7);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(8);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(9);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(10);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(11);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(12);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(13);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(14);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(15);
DECLARE_TRANSCEIVER_SENSOR_DEVICE_ATTR(16);

static struct attribute *es9618xx_cpld1_attributes[] = {
    &sensor_dev_attr_board_id.dev_attr.attr,
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
	NULL
};

static const struct attribute_group es9618xx_cpld1_group = {
	.attrs = es9618xx_cpld1_attributes,
};

static struct attribute *es9618xx_cpld2_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
	/* transceiver attributes */
	&sensor_dev_attr_module_present_all.dev_attr.attr,
	DECLARE_TRANSCEIVER_PRESENT_ATTR(1),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(2),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(3),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(4),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(5),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(6),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(7),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(8),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(9),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(10),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(11),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(12),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(13),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(14),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(15),
	DECLARE_TRANSCEIVER_PRESENT_ATTR(16),
    DECLARE_TRANSCEIVER_RESET_ATTR(1),
	DECLARE_TRANSCEIVER_RESET_ATTR(2),
	DECLARE_TRANSCEIVER_RESET_ATTR(3),
	DECLARE_TRANSCEIVER_RESET_ATTR(4),
	DECLARE_TRANSCEIVER_RESET_ATTR(5),
	DECLARE_TRANSCEIVER_RESET_ATTR(6),
	DECLARE_TRANSCEIVER_RESET_ATTR(7),
	DECLARE_TRANSCEIVER_RESET_ATTR(8),
	DECLARE_TRANSCEIVER_RESET_ATTR(9),
	DECLARE_TRANSCEIVER_RESET_ATTR(10),
	DECLARE_TRANSCEIVER_RESET_ATTR(11),
	DECLARE_TRANSCEIVER_RESET_ATTR(12),
	DECLARE_TRANSCEIVER_RESET_ATTR(13),
	DECLARE_TRANSCEIVER_RESET_ATTR(14),
	DECLARE_TRANSCEIVER_RESET_ATTR(15),
	DECLARE_TRANSCEIVER_RESET_ATTR(16),
    DECLARE_TRANSCEIVER_LPMODE_ATTR(1),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(2),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(3),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(4),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(5),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(6),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(7),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(8),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(9),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(10),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(11),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(12),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(13),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(14),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(15),
	DECLARE_TRANSCEIVER_LPMODE_ATTR(16),
	NULL
};

static const struct attribute_group es9618xx_cpld2_group = {
	.attrs = es9618xx_cpld2_attributes,
};


static ssize_t show_present_all(struct device *dev, struct device_attribute *da,
             char *buf)
{
	int i, status;
	u8 values[2]  = {0};
    u8 regs_cpld2[] = {0x14, 0x15};
    u8 *regs[] = {NULL, regs_cpld2, NULL};
    u8  size[] = {0, ARRAY_SIZE(regs_cpld2), 0};
	struct i2c_client *client = to_i2c_client(dev);
	struct es9618xx_cpld_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

    for (i = 0; i < size[data->type]; i++) {
        status = es9618xx_cpld_read_internal(client, regs[data->type][i]);
        if (status < 0) {
            goto exit;
        }

        values[i] = ~(u8)status;
    }

	mutex_unlock(&data->update_lock);

    /* Return values in order */
    return sprintf(buf, "%.2x %.2x\n", values[0], values[1]);

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t show_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct es9618xx_cpld_data *data = i2c_get_clientdata(client);
	int status = 0;
	u8 reg = 0, mask = 0, invert = 1;

	switch (attr->index) {
	case MODULE_PRESENT_1 ... MODULE_PRESENT_8:
		reg  = 0x14;
		mask = 0x1 << (attr->index - MODULE_PRESENT_1);
		break;
	case MODULE_PRESENT_9 ... MODULE_PRESENT_16:
		reg  = 0x15;
		mask = 0x1 << (attr->index - MODULE_PRESENT_9);
		break;
	case MODULE_RESET_1 ... MODULE_RESET_8:
		reg  = 0x60;
		mask = 0x1 << (attr->index - MODULE_RESET_1);
		break;
	case MODULE_RESET_9 ... MODULE_RESET_16:
		reg  = 0x61;
		mask = 0x1 << (attr->index - MODULE_RESET_9);
		break;
	case MODULE_LPMODE_1 ... MODULE_LPMODE_8:
		reg  = 0x64;
		mask = 0x1 << (attr->index - MODULE_LPMODE_1);
		break;
	case MODULE_LPMODE_9 ... MODULE_LPMODE_16:
		reg  = 0x65;
		mask = 0x1 << (attr->index - MODULE_LPMODE_9);
		break;
	default:
		return 0;
	}

    mutex_lock(&data->update_lock);
	status = es9618xx_cpld_read_internal(client, reg);
	if (unlikely(status < 0)) {
		goto exit;
	}
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", invert ? !(status & mask) : !!(status & mask));

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct es9618xx_cpld_data *data = i2c_get_clientdata(client);
	long value;
	int status;
    u8 reg = 0, mask = 0;

	status = kstrtol(buf, 10, &value);
	if (status) {
		return status;
	}

	switch (attr->index) {
	case MODULE_RESET_1 ... MODULE_RESET_8:
		reg  = 0x60;
		mask = 0x1 << (attr->index - MODULE_RESET_1);
		break;
	case MODULE_RESET_9 ... MODULE_RESET_16:
		reg  = 0x61;
		mask = 0x1 << (attr->index - MODULE_RESET_9);
		break;
	case MODULE_LPMODE_1 ... MODULE_LPMODE_8:
		reg  = 0x64;
		mask = 0x1 << (attr->index - MODULE_LPMODE_1);
		break;
	case MODULE_LPMODE_9 ... MODULE_LPMODE_16:
		reg  = 0x65;
		mask = 0x1 << (attr->index - MODULE_LPMODE_9);
		break;
	default:
		return 0;
	}

    /* Read current status */
    mutex_lock(&data->update_lock);
	status = es9618xx_cpld_read_internal(client, reg);
	if (unlikely(status < 0)) {
		goto exit;
	}

	/* Update reset/lpmode status */
    value = !value; /* reset/lpmode value is inverted in CPLD */

	if (value) {
		status |= mask;
	}
	else {
		status &= ~mask;
	}

    status = es9618xx_cpld_write_internal(client, reg, status);
	if (unlikely(status < 0)) {
		goto exit;
	}

    mutex_unlock(&data->update_lock);
    return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t access(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	int status;
	u32 addr, val;
    struct i2c_client *client = to_i2c_client(dev);
    struct es9618xx_cpld_data *data = i2c_get_clientdata(client);

	if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
		return -EINVAL;
	}

	if (addr > 0xFF || val > 0xFF) {
		return -EINVAL;
	}

	mutex_lock(&data->update_lock);
	status = es9618xx_cpld_write_internal(client, addr, val);
	if (unlikely(status < 0)) {
		goto exit;
	}
	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static void es9618xx_cpld_add_client(struct i2c_client *client)
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

static void es9618xx_cpld_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
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

static ssize_t show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ver = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct es9618xx_cpld_data *data = i2c_get_clientdata(client);

	ver = i2c_smbus_read_byte_data(client, 0x1);

	if (ver < 0) {
		dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n", client->addr, ver);
		return ver;
	}

	if (data->type == es9618xx_cpld1) {
		int subver = i2c_smbus_read_byte_data(client, 0x2);

		if (subver < 0) {
			dev_dbg(&client->dev, "cpld(0x%x) reg(0x2) err %d\n", client->addr, subver);
			return subver;
		}
		return sprintf(buf, "%d.%d\n", ver, subver);
	}

	return sprintf(buf, "%d\n", ver);
}

static ssize_t show_board_id(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0;
    struct i2c_client *client = to_i2c_client(dev);

	val = i2c_smbus_read_byte_data(client, 0x0);

    if (val < 0) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(0x0) err %d\n", client->addr, val);
        return val;
    }

    return sprintf(buf, "%d\n", (val >> 3) & 0x7);
}

/*
 * I2C init/probing/exit functions
 */
static int es9618xx_cpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	struct es9618xx_cpld_data *data;
	int ret = -ENODEV;
	const struct attribute_group *group = NULL;

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
		goto exit;

	data = kzalloc(sizeof(struct es9618xx_cpld_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
	data->type = id->driver_data;

    /* Register sysfs hooks */
    switch (data->type) {
    case es9618xx_cpld1:
        group = &es9618xx_cpld1_group;
        break;
    case es9618xx_cpld2:
        group = &es9618xx_cpld2_group;
        break;
    default:
        break;
    }

    if (group) {
        ret = sysfs_create_group(&client->dev.kobj, group);
        if (ret) {
            goto exit_free;
        }
    }

    es9618xx_cpld_add_client(client);
    return 0;

exit_free:
    kfree(data);
exit:
	return ret;
}

static void es9618xx_cpld_remove(struct i2c_client *client)
{
    struct es9618xx_cpld_data *data = i2c_get_clientdata(client);
    const struct attribute_group *group = NULL;

    es9618xx_cpld_remove_client(client);

    /* Remove sysfs hooks */
    switch (data->type) {
    case es9618xx_cpld1:
        group = &es9618xx_cpld1_group;
        break;
    case es9618xx_cpld2:
        group = &es9618xx_cpld2_group;
        break;
    default:
        break;
    }

    if (group) {
        sysfs_remove_group(&client->dev.kobj, group);
    }

    kfree(data);
}

static int es9618xx_cpld_read_internal(struct i2c_client *client, u8 reg)
{
	int status = 0, retry = I2C_RW_RETRY_COUNT;

	while (retry) {
		status = i2c_smbus_read_byte_data(client, reg);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

    return status;
}

static int es9618xx_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
{
	int status = 0, retry = I2C_RW_RETRY_COUNT;

	while (retry) {
		status = i2c_smbus_write_byte_data(client, reg, value);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

    return status;
}

int es9618xx_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = es9618xx_cpld_read_internal(cpld_node->client, reg);
    		break;
        }
    }

	mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(es9618xx_cpld_read);

int es9618xx_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

	mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = es9618xx_cpld_write_internal(cpld_node->client, reg, value);
            break;
        }
    }

	mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(es9618xx_cpld_write);

static struct i2c_driver es9618xx_cpld_driver = {
	.driver		= {
		.name	= "es9618xx_cpld",
		.owner	= THIS_MODULE,
	},
	.probe		= es9618xx_cpld_probe,
	.remove		= es9618xx_cpld_remove,
	.id_table	= es9618xx_cpld_id,
};

static int __init es9618xx_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&es9618xx_cpld_driver);
}

static void __exit es9618xx_cpld_exit(void)
{
    i2c_del_driver(&es9618xx_cpld_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("Accton es9618xx CPLD driver");
MODULE_LICENSE("GPL");

module_init(es9618xx_cpld_init);
module_exit(es9618xx_cpld_exit);
