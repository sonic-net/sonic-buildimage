/*
 * Copyright (C)  Ang Liu <ang_liu@accton.com.cn>
 *
 * An I2C device dirver for Accton CPLD
 *
 * Based on:
 *  pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *  pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *  i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *  pca9540.c from Jean Delvare <khali@linux-fr.org>.
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
#include <linux/of_device.h>
#include "accton-cpld.h"

#define I2C_RW_RETRY_COUNT              10
#define I2C_RW_RETRY_INTERVAL           60 /* ms */

static LIST_HEAD(cpld_client_list);
static struct mutex     list_lock;

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

struct cplddev_data {
    enum cpld_type   type;
    struct mutex     update_lock;
    u8               reg_offset;
};

#ifdef CONFIG_OF
static const struct of_device_id accton_cpld_i2c_of_ids[] = {
	{
		.compatible = "accton,ctrl_cpld",
		.data = (void *) CTRL_CPLD,
	},
	{
		.compatible = "accton,port_cpld",
		.data = (void *) PORT_CPLD,
	},
	{
		.compatible = "accton,port_cpld2",
		.data = (void *) PORT_CPLD2,
	},
	{},
};
MODULE_DEVICE_TABLE(of, accton_cpld_i2c_of_ids);
#endif

static const struct i2c_device_id accton_cpld_i2c_id[] = {
	{ "ctrl_cpld",   CTRL_CPLD },
	{ "port_cpld",   PORT_CPLD },
	{ "port_cpld2",  PORT_CPLD2 },
	{},
};
MODULE_DEVICE_TABLE(i2c, accton_cpld_i2c_id);

enum accton_cpld_i2c_sysfs_attributes {
    CPLD_VERSION,
    ACCESS,
    ACCESS_REG_OFFSET,
    ACCESS_REG_VALUE,
};

/* sysfs attributes for cpld access
 */
static ssize_t access(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t write_reg_value(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count);
static ssize_t read_reg_value(struct device *dev, struct device_attribute *da,
                            char *buf);
static ssize_t store_reg_offset(struct device *dev, struct device_attribute *da,
                      const char *buf, size_t count);
static ssize_t show_reg_offset(struct device *dev, struct device_attribute *da,
                            char *buf);
static int accton_cpld_i2c_read_internal(struct i2c_client *client, u8 reg);
static int accton_cpld_i2c_write_internal(struct i2c_client *client, u8 reg, u8 value);

static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);
static SENSOR_DEVICE_ATTR(reg_offset, S_IWUSR | S_IRUGO, show_reg_offset, store_reg_offset, ACCESS_REG_OFFSET);
static SENSOR_DEVICE_ATTR(reg_value, S_IWUSR | S_IRUGO, read_reg_value, write_reg_value, ACCESS_REG_VALUE);

static struct attribute *accton_cpld_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    &sensor_dev_attr_reg_offset.dev_attr.attr,
    &sensor_dev_attr_reg_value.dev_attr.attr,
    NULL
};

static const struct attribute_group accton_cpld_group = {
    .attrs = accton_cpld_attributes,
};

static ssize_t access(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    int status = 0;
    u32 reg = 0, val = 0, mask = 0xff;
    u8 cpld_write_val, cpld_cur_val;
    struct i2c_client *client = to_i2c_client(dev);
    struct cplddev_data *data = i2c_get_clientdata(client);

    status = sscanf(buf, "0x%x 0x%x 0x%x", &reg, &val, &mask);
    if (status != 2 && status != 3) {
        return -EINVAL;
    }

    if (reg > 0xFF || val > 0xFF || mask > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = accton_cpld_i2c_read_internal(client, reg);
	if (unlikely(status < 0)) {
		goto exit;
	}

    cpld_cur_val = (u8)status;
    cpld_cur_val = cpld_cur_val & (~mask);

    cpld_write_val = cpld_cur_val | (val & mask);

    status = accton_cpld_i2c_write_internal(client, reg, cpld_write_val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    int status = 0;
    int major_version = 0;
    int minor_version = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct cplddev_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);
    status = accton_cpld_i2c_read_internal(client, 0x0);
    if (unlikely(status < 0)) {
        goto exit;
    }
    minor_version = status;

    status = accton_cpld_i2c_read_internal(client, 0x1);
    if (unlikely(status < 0)) {
        goto exit;
    }
    major_version = status;
    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%02x.%02x\n", major_version, minor_version);

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t read_reg_value(struct device *dev, struct device_attribute *attr,  char *buf)
{
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct cplddev_data *data = i2c_get_clientdata(client);
    u8 reg = data->reg_offset;

    mutex_lock(&data->update_lock);
    status = accton_cpld_i2c_read_internal(client, reg);
    if (unlikely(status < 0)) {
        dev_err(dev, "read reg(0x%x) err %d\n", reg, status);
    }
    mutex_unlock(&data->update_lock);

    if (status < 0)
        return status;
    else
        return sprintf(buf, "reg:0x%02x val:0x%02x\n", reg, status);
}

static ssize_t write_reg_value(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    int status = 0;
    u8 cpld_write_val, cpld_cur_val;
    u32 val = 0, mask = 0xff;
    struct i2c_client *client = to_i2c_client(dev);
    struct cplddev_data *data = i2c_get_clientdata(client);
    u8 reg = data->reg_offset;

    status = sscanf(buf, "0x%x 0x%x", &val, &mask);
    if (status != 1 && status != 2) {
        return -EINVAL;
    }

    if (val > 0xFF || mask > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = accton_cpld_i2c_read_internal(client, reg);
	if (unlikely(status < 0)) {
		goto exit;
	}

    cpld_cur_val = (u8)status;
    cpld_cur_val = cpld_cur_val & (~mask);

    cpld_write_val = cpld_cur_val | (val & mask);

    status = accton_cpld_i2c_write_internal(client, reg, cpld_write_val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t show_reg_offset(struct device *dev, struct device_attribute *attr,  char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cplddev_data *data = i2c_get_clientdata(client);

    return sprintf(buf, "offset:0x%02x\n", data->reg_offset);
}

static ssize_t store_reg_offset(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cplddev_data *data = i2c_get_clientdata(client);
    u32 offset = 0;

    if (sscanf(buf, "0x%x", &offset) != 1) {
        return -EINVAL;
    }

    data->reg_offset = (u8)offset;
    return count;
}

static void accton_cpld_i2c_add_client(struct i2c_client *client)
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

static void accton_cpld_i2c_remove_client(struct i2c_client *client)
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

/*
 * I2C init/probing/exit functions
 */
static int accton_cpld_i2c_probe(struct i2c_client *client)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    struct cplddev_data *data;
    const struct i2c_device_id *id;
    int ret = -ENODEV;
    const struct attribute_group *group = NULL;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
        goto exit;

    data = kzalloc(sizeof(struct cplddev_data), GFP_KERNEL);
    if (!data) {
        ret = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    if (client->dev.of_node) {
        data->type = (enum cpld_type)of_device_get_match_data(&client->dev);
    } else {
        id = i2c_match_id(accton_cpld_i2c_id, client);
        if (!id) {
            ret = -ENODEV;
            goto exit_free;
        }
        data->type = id->driver_data;
    }

    /* Register sysfs hooks */
    switch (data->type) {
	case CTRL_CPLD:
	case PORT_CPLD:
	case PORT_CPLD2:
        group = &accton_cpld_group;
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

    accton_cpld_i2c_add_client(client);
    return 0;

exit_free:
    kfree(data);
exit:
    return ret;
}

static void accton_cpld_i2c_remove(struct i2c_client *client)
{
    struct cplddev_data *data = i2c_get_clientdata(client);
    const struct attribute_group *group = NULL;

    accton_cpld_i2c_remove_client(client);

    /* Remove sysfs hooks */
    switch (data->type) {
	case CTRL_CPLD:
	case PORT_CPLD:
	case PORT_CPLD2:
        group = &accton_cpld_group;
        break;
    default:
        break;
    }

    if (group) {
        sysfs_remove_group(&client->dev.kobj, group);
    }

    kfree(data);

    return;
}

static int accton_cpld_i2c_read_internal(struct i2c_client *client, u8 reg)
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

static int accton_cpld_i2c_write_internal(struct i2c_client *client, u8 reg, u8 value)
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

int accton_cpld_i2c_read(unsigned short cpld_type, u8 reg)
{
    struct list_head   *list_node = NULL;
	struct cplddev_data *cplddev = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
		cplddev = i2c_get_clientdata(cpld_node->client);

        if (cplddev->type == cpld_type) {
            ret = accton_cpld_i2c_read_internal(cpld_node->client, reg);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(accton_cpld_i2c_read);

int accton_cpld_i2c_write(unsigned short cpld_type, u8 reg, u8 val)
{
    struct list_head   *list_node = NULL;
	struct cplddev_data *cplddev = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
		cplddev = i2c_get_clientdata(cpld_node->client);

        if (cplddev->type == cpld_type) {
            ret = accton_cpld_i2c_write_internal(cpld_node->client, reg, val);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(accton_cpld_i2c_write);

int accton_cpld_i2c_write_with_mask(unsigned short cpld_type, u8 reg, u8 val, u8 mask)
{
	struct list_head *list_node = NULL;
	struct cpld_client_node *cpld_node = NULL;
	struct cplddev_data *cplddev = NULL;
	int status = -EIO;
	u8 cpld_write_val, cpld_cur_val;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_client_list) {
		cpld_node = list_entry(list_node, struct cpld_client_node, list);
		cplddev = i2c_get_clientdata(cpld_node->client);

		if (cplddev->type == cpld_type) {
			status = accton_cpld_i2c_read_internal(cpld_node->client, reg);
			if (unlikely(status < 0)) {
				goto exit;
			}

			cpld_cur_val = (u8)status;
			cpld_cur_val = cpld_cur_val & (~mask);

			cpld_write_val = cpld_cur_val | (val & mask);

			status = accton_cpld_i2c_write_internal(cpld_node->client, reg, cpld_write_val);
			if (unlikely(status < 0)) {
				goto exit;
			}
			break;
		}
	}

exit:
	mutex_unlock(&list_lock);
	return status;
}
EXPORT_SYMBOL(accton_cpld_i2c_write_with_mask);

static struct i2c_driver accton_cpld_i2c_driver = {
    .driver     = {
        .name   = "accton_cpld_i2c",
        .owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(accton_cpld_i2c_of_ids),
    },
    .probe      = accton_cpld_i2c_probe,
    .remove     = accton_cpld_i2c_remove,
    .id_table   = accton_cpld_i2c_id,
};

static int __init accton_cpld_i2c_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&accton_cpld_i2c_driver);
}

static void __exit accton_cpld_i2c_exit(void)
{
    i2c_del_driver(&accton_cpld_i2c_driver);
}

MODULE_AUTHOR("Ang Liu <ang_liu@accton.com.cn>");
MODULE_DESCRIPTION("accton cpld i2c driver");
MODULE_LICENSE("GPL v2");

module_init(accton_cpld_i2c_init);
module_exit(accton_cpld_i2c_exit);

