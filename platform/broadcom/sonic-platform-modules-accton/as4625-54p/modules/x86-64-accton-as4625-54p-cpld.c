/*
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This module supports the accton cpld that hold the channel select
 * mechanism for other i2c slave devices, such as SFP.
 * This includes the:
 *	 Accton as4625 CPLD1/CPLD2/CPLD3
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
    as4625_cpld1
};

struct as4625_cpld_data {
    enum cpld_type   type;
    struct device   *hwmon_dev;
    struct mutex     update_lock;
};

static const struct i2c_device_id as4625_cpld_id[] = {
    { "as4625_cpld1", as4625_cpld1 },
    { }
};
MODULE_DEVICE_TABLE(i2c, as4625_cpld_id);

#define TRANSCEIVER_PRESENT_ATTR_ID(index)   	MODULE_PRESENT_##index
#define TRANSCEIVER_TXDISABLE_ATTR_ID(index)   	MODULE_TXDISABLE_##index
#define TRANSCEIVER_RXLOS_ATTR_ID(index)   		MODULE_RXLOS_##index
#define TRANSCEIVER_TXFAULT_ATTR_ID(index)   	MODULE_TXFAULT_##index

enum as4625_cpld_sysfs_attributes {
	VERSION_MAJOR,
	VERSION_MINOR,
	PCB_ID,
	PCB_VERSION,
	POWER_ENABLE_MAINBOARD,
	POWER_ENABLE_POE,
	SYSTEM_THERMAL_SHUTDOWN,
	ACCESS,
	/* transceiver attributes */
	TRANSCEIVER_PRESENT_ATTR_ID(49),
	TRANSCEIVER_PRESENT_ATTR_ID(50),
	TRANSCEIVER_PRESENT_ATTR_ID(51),
	TRANSCEIVER_PRESENT_ATTR_ID(52),
	TRANSCEIVER_PRESENT_ATTR_ID(53),
	TRANSCEIVER_PRESENT_ATTR_ID(54),
	TRANSCEIVER_TXDISABLE_ATTR_ID(49),
	TRANSCEIVER_TXDISABLE_ATTR_ID(50),
	TRANSCEIVER_TXDISABLE_ATTR_ID(51),
	TRANSCEIVER_TXDISABLE_ATTR_ID(52),
	TRANSCEIVER_TXDISABLE_ATTR_ID(53),
	TRANSCEIVER_TXDISABLE_ATTR_ID(54),
	TRANSCEIVER_RXLOS_ATTR_ID(49),
	TRANSCEIVER_RXLOS_ATTR_ID(50),
	TRANSCEIVER_RXLOS_ATTR_ID(51),
	TRANSCEIVER_RXLOS_ATTR_ID(52),
	TRANSCEIVER_RXLOS_ATTR_ID(53),
	TRANSCEIVER_RXLOS_ATTR_ID(54),
	TRANSCEIVER_TXFAULT_ATTR_ID(49),
	TRANSCEIVER_TXFAULT_ATTR_ID(50),
	TRANSCEIVER_TXFAULT_ATTR_ID(51),
	TRANSCEIVER_TXFAULT_ATTR_ID(52),
	TRANSCEIVER_TXFAULT_ATTR_ID(53),
	TRANSCEIVER_TXFAULT_ATTR_ID(54)
};

/* sysfs attributes for hwmon
 */
static ssize_t show_status(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t access(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
             char *buf);
static int as4625_cpld_read_internal(struct i2c_client *client, u8 reg);
static int as4625_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value);

/* transceiver attributes */
#define DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(module_present_##index, S_IRUGO, show_status, NULL, MODULE_PRESENT_##index); \
	static SENSOR_DEVICE_ATTR(module_tx_disable_##index, S_IRUGO | S_IWUSR, show_status, set_control, MODULE_TXDISABLE_##index); \
	static SENSOR_DEVICE_ATTR(module_rx_los_##index, S_IRUGO, show_status, NULL, MODULE_RXLOS_##index); \
	static SENSOR_DEVICE_ATTR(module_tx_fault_##index, S_IRUGO, show_status, NULL, MODULE_TXFAULT_##index)
#define DECLARE_SFP_TRANSCEIVER_ATTR(index)  \
	&sensor_dev_attr_module_present_##index.dev_attr.attr, \
	&sensor_dev_attr_module_tx_disable_##index.dev_attr.attr, \
	&sensor_dev_attr_module_rx_los_##index.dev_attr.attr, \
	&sensor_dev_attr_module_tx_fault_##index.dev_attr.attr

static SENSOR_DEVICE_ATTR(version_major, S_IRUGO, show_version, NULL, VERSION_MAJOR);
static SENSOR_DEVICE_ATTR(version_minor, S_IRUGO, show_version, NULL, VERSION_MINOR);
static SENSOR_DEVICE_ATTR(pcb_id, S_IRUGO, show_version, NULL, PCB_ID);
static SENSOR_DEVICE_ATTR(pcb_version, S_IRUGO, show_version, NULL, PCB_VERSION);
static SENSOR_DEVICE_ATTR(pwr_enable_mb, S_IRUGO | S_IWUSR, show_status, set_control, POWER_ENABLE_MAINBOARD);
static SENSOR_DEVICE_ATTR(pwr_enable_poe, S_IRUGO | S_IWUSR, show_status, set_control, POWER_ENABLE_POE);
static SENSOR_DEVICE_ATTR(thermal_shutdown, S_IRUGO | S_IWUSR, show_status, set_control, SYSTEM_THERMAL_SHUTDOWN);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);

/* transceiver attributes */
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(49);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(50);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(51);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(52);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(53);
DECLARE_SFP_TRANSCEIVER_SENSOR_DEVICE_ATTR(54);

static struct attribute *as4625_cpld1_attributes[] = {
    &sensor_dev_attr_version_major.dev_attr.attr,
    &sensor_dev_attr_version_minor.dev_attr.attr,
    &sensor_dev_attr_pcb_id.dev_attr.attr,
    &sensor_dev_attr_pcb_version.dev_attr.attr,
    &sensor_dev_attr_pwr_enable_mb.dev_attr.attr,
    &sensor_dev_attr_pwr_enable_poe.dev_attr.attr,
    &sensor_dev_attr_thermal_shutdown.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
	/* transceiver attributes */
	DECLARE_SFP_TRANSCEIVER_ATTR(49),
	DECLARE_SFP_TRANSCEIVER_ATTR(50),
	DECLARE_SFP_TRANSCEIVER_ATTR(51),
	DECLARE_SFP_TRANSCEIVER_ATTR(52),
	DECLARE_SFP_TRANSCEIVER_ATTR(53),
	DECLARE_SFP_TRANSCEIVER_ATTR(54),
	NULL
};

static const struct attribute_group as4625_cpld1_group = {
	.attrs = as4625_cpld1_attributes,
};

static ssize_t show_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as4625_cpld_data *data = i2c_get_clientdata(client);
	int status = 0;
	u8 reg = 0, mask = 0, reverse = 0;

	switch (attr->index) {
	case MODULE_TXDISABLE_49 ... MODULE_TXDISABLE_54:
		reg  = 0x5;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_49);
		break;
	case MODULE_PRESENT_49 ... MODULE_PRESENT_54:
		reg  = 0x6;
		mask = 0x1 << (attr->index - MODULE_PRESENT_49);
		reverse = 1;
		break;
	case MODULE_RXLOS_49 ... MODULE_RXLOS_54:
		reg  = 0x7;
		mask = 0x1 << (attr->index - MODULE_RXLOS_49);
		break;
	case MODULE_TXFAULT_49 ... MODULE_TXFAULT_54:
		reg  = 0x8;
		mask = 0x1 << (attr->index - MODULE_TXFAULT_49);
		break;
	case POWER_ENABLE_MAINBOARD:
		reg  = 0x3;
		mask = 0x1;
		reverse = 1;
		break;
	case POWER_ENABLE_POE:
		reg  = 0x21;
		mask = 0x1;
		break;
	case SYSTEM_THERMAL_SHUTDOWN:
		reg  = 0x27;
		mask = 0x1;
		break;
	default:
		return 0;
	}

    mutex_lock(&data->update_lock);
	status = as4625_cpld_read_internal(client, reg);
	if (unlikely(status < 0)) {
		goto exit;
	}
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", reverse ? !(status & mask) : !!(status & mask));

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t set_control(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct as4625_cpld_data *data = i2c_get_clientdata(client);
	long value;
	int status;
    u8 reg = 0, mask = 0;

	status = kstrtol(buf, 10, &value);
	if (status) {
		return status;
	}

	switch (attr->index) {
	case MODULE_TXDISABLE_49 ... MODULE_TXDISABLE_54:
		reg  = 0x5;
		mask = 0x1 << (attr->index - MODULE_TXDISABLE_49);
		break;
	case POWER_ENABLE_MAINBOARD:
		reg  = 0x3;
		mask = 0x1;
		value = !value;
		break;
	case POWER_ENABLE_POE:
		reg  = 0x21;
		mask = 0x1;
		break;
	case SYSTEM_THERMAL_SHUTDOWN:
		reg  = 0x27;
		mask = 0x1;
		break;
	default:
		return -EINVAL;
	}

    /* Read current status */
    mutex_lock(&data->update_lock);
	status = as4625_cpld_read_internal(client, reg);
	if (unlikely(status < 0)) {
		goto exit;
	}

	/* Update tx_disable status */
	if (value)
		status |= mask;
	else
		status &= ~mask;

    status = as4625_cpld_write_internal(client, reg, status);
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
    struct as4625_cpld_data *data = i2c_get_clientdata(client);

	if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
		return -EINVAL;
	}

	if (addr > 0xFF || val > 0xFF) {
		return -EINVAL;
	}

	mutex_lock(&data->update_lock);
	status = as4625_cpld_write_internal(client, addr, val);
	if (unlikely(status < 0)) {
		goto exit;
	}
	mutex_unlock(&data->update_lock);
	return count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static void as4625_cpld_add_client(struct i2c_client *client)
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

static void as4625_cpld_remove_client(struct i2c_client *client)
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

static ssize_t show_version(struct device *dev, struct device_attribute *da,
				char *buf)
{
	int val = 0, reg = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

	switch (attr->index) {
	case PCB_ID:
	case PCB_VERSION:
		reg = 0x0;
		break;
	case VERSION_MAJOR:
		reg = 0x1;
		break;
	case VERSION_MINOR:
		reg = 0x2;
		break;
	default:
		break;
	}

	val = i2c_smbus_read_byte_data(client, reg);
	if (val < 0) {
		dev_dbg(&client->dev, "cpld(0x%x) reg(0x1) err %d\n",
					client->addr, val);
		return val;
	}

	if (attr->index == PCB_ID)
		val = (val >> 3) & 0x7; /* bit 3-5 */
	else if (attr->index == PCB_VERSION)
		val &= 0x7; /* bit 0-2 */

	return sprintf(buf, "%d\n", val);
}

/*
 * I2C init/probing/exit functions
 */
static int as4625_cpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	struct as4625_cpld_data *data;
	int ret = -ENODEV;
	const struct attribute_group *group = NULL;

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
		goto exit;

	data = kzalloc(sizeof(struct as4625_cpld_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
	data->type = id->driver_data;

    /* Register sysfs hooks */
    switch (data->type) {
    case as4625_cpld1:
        group = &as4625_cpld1_group;
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

    as4625_cpld_add_client(client);
    return 0;

exit_free:
    kfree(data);
exit:
	return ret;
}

static int as4625_cpld_remove(struct i2c_client *client)
{
    struct as4625_cpld_data *data = i2c_get_clientdata(client);
    const struct attribute_group *group = NULL;

    as4625_cpld_remove_client(client);

    /* Remove sysfs hooks */
    switch (data->type) {
    case as4625_cpld1:
        group = &as4625_cpld1_group;
        break;
    default:
        break;
    }

    if (group) {
        sysfs_remove_group(&client->dev.kobj, group);
    }

    kfree(data);
    return 0;
}

static int as4625_cpld_read_internal(struct i2c_client *client, u8 reg)
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

static int as4625_cpld_write_internal(struct i2c_client *client, u8 reg, u8 value)
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

int as4625_cpld_read(unsigned short cpld_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as4625_cpld_read_internal(cpld_node->client, reg);
    		break;
        }
    }

	mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as4625_cpld_read);

int as4625_cpld_write(unsigned short cpld_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

	mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == cpld_addr) {
            ret = as4625_cpld_write_internal(cpld_node->client, reg, value);
            break;
        }
    }

	mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as4625_cpld_write);

static struct i2c_driver as4625_cpld_driver = {
	.driver		= {
		.name	= "as4625_cpld",
		.owner	= THIS_MODULE,
	},
	.probe		= as4625_cpld_probe,
	.remove		= as4625_cpld_remove,
	.id_table	= as4625_cpld_id,
};

static int __init as4625_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&as4625_cpld_driver);
}

static void __exit as4625_cpld_exit(void)
{
    i2c_del_driver(&as4625_cpld_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("Accton I2C CPLD driver");
MODULE_LICENSE("GPL");

module_init(as4625_cpld_init);
module_exit(as4625_cpld_exit);
