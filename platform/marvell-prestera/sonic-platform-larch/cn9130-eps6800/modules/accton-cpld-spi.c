/*
 * An SPI device dirver for Accton CPLD
 *
 * Copyright (C) 2023 Accton Technology Corporation.
 * Ang Liu <ang_liu@accton.com.cn>
 *
 * This module supports the accton cpld slave devices, such as SFP.
 * This includes the:
 *     Accton CTRL_CPLD/PORT_CPLD
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include "accton-cpld.h"

#define SPI_RW_RETRY_COUNT      3
#define SPI_RW_RETRY_INTERVAL   5       /* ms */
#define CPLD_READ_OPCODE        0x08
#define CPLD_WRITE_OPCODE       0x04
#define CPLD_REG_ADDR_BIT       16
#define CPLD_OPCODE_ADDR_BIT    24
#define CPLD_WRITE_DATA_LEN     3
#define CPLD_READ_DATA_LEN      1
#define CPLD_TEST_REG			0x05

struct cplddev_data {
	enum    cpld_type type;
	struct  spi_device  *spi;
	u8      tx_buffer[CPLD_WRITE_DATA_LEN];
	u8      rx_buffer[CPLD_READ_DATA_LEN];
	u8      reg_offset;

	struct mutex    update_lock;
};

static LIST_HEAD(cpld_device_list);
static struct mutex     list_lock;

struct cpld_device_node {
	struct spi_device *spi;
	struct list_head   list;
};

enum accton_cpld_sysfs_attributes {
	CPLD_VERSION,
	ACCESS,
	ACCESS_REG_OFFSET,
	ACCESS_REG_VALUE,
};

/* sysfs attributes for cpld access */
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
static int accton_cpld_read_internal(struct spi_device *spi, u8 reg);
static int accton_cpld_write_internal(struct spi_device *spi, u8 reg, u8 value);

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
	struct cplddev_data *cplddev = dev_get_drvdata(dev);

	status = sscanf(buf, "0x%x 0x%x 0x%x", &reg, &val, &mask);
	if (status != 2 && status != 3) {
		return -EINVAL;
	}

	if (reg > 0xFF || val > 0xFF || mask > 0xFF) {
		return -EINVAL;
	}

	mutex_lock(&cplddev->update_lock);
	status = accton_cpld_read_internal(cplddev->spi, reg);
	if (unlikely(status < 0)) {
		goto exit;
	}

	cpld_cur_val = (u8)status;
	cpld_cur_val = cpld_cur_val & (~mask);

	cpld_write_val = cpld_cur_val | (val & mask);

	status = accton_cpld_write_internal(cplddev->spi, reg, cpld_write_val);
	if (unlikely(status < 0)) {
		goto exit;
	}
	mutex_unlock(&cplddev->update_lock);
	return count;

exit:
	mutex_unlock(&cplddev->update_lock);
	return status;
}

static ssize_t show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status = 0;
	int major_version = 0;
	int minor_version = 0;
	struct cplddev_data *cplddev = dev_get_drvdata(dev);

	mutex_lock(&cplddev->update_lock);
	status = accton_cpld_read_internal(cplddev->spi, 0x0);
	if (unlikely(status < 0)) {
		goto exit;
	}
	major_version = status;

	status = accton_cpld_read_internal(cplddev->spi, 0x1);
	if (unlikely(status < 0)) {
		goto exit;
	}
	minor_version = status;

	mutex_unlock(&cplddev->update_lock);

	major_version &= 0xff;
	minor_version &= 0xff;

	/* 0xFF mean cpld no repsonse */
	if ((major_version == 0xFF) && (minor_version == 0xFF))
	{
		return sprintf(buf, "Unknown\n");
	}

	return sprintf(buf, "%02x.%02x\n", major_version, minor_version);

exit:
	mutex_unlock(&cplddev->update_lock);
	return status;
}

static ssize_t read_reg_value(struct device *dev, struct device_attribute *attr,  char *buf)
{
	int status = 0;
	struct cplddev_data *cplddev = dev_get_drvdata(dev);
	u8 reg = cplddev->reg_offset;

	mutex_lock(&cplddev->update_lock);
	status = accton_cpld_read_internal(cplddev->spi, reg);
	if (unlikely(status < 0)) {
		dev_err(dev, "read reg(0x%x) err %d\n", reg, status);
	}
	mutex_unlock(&cplddev->update_lock);

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
	struct cplddev_data *cplddev = dev_get_drvdata(dev);
	u8 reg = cplddev->reg_offset;

	status = sscanf(buf, "0x%x 0x%x", &val, &mask);
	if (status != 1 && status != 2) {
		return -EINVAL;
	}

	if (val > 0xFF || mask > 0xFF) {
		return -EINVAL;
	}

	mutex_lock(&cplddev->update_lock);
	status = accton_cpld_read_internal(cplddev->spi, reg);
	if (unlikely(status < 0)) {
		goto exit;
	}

	cpld_cur_val = (u8)status;
	cpld_cur_val = cpld_cur_val & (~mask);

	cpld_write_val = cpld_cur_val | (val & mask);

	status = accton_cpld_write_internal(cplddev->spi, reg, cpld_write_val);
	if (unlikely(status < 0)) {
		goto exit;
	}
	mutex_unlock(&cplddev->update_lock);
	return count;

exit:
	mutex_unlock(&cplddev->update_lock);
	return status;
}

static ssize_t show_reg_offset(struct device *dev, struct device_attribute *attr,  char *buf)
{
	struct cplddev_data *cplddev = dev_get_drvdata(dev);

	return sprintf(buf, "offset:0x%02x\n", cplddev->reg_offset);
}

static ssize_t store_reg_offset(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct cplddev_data *cplddev = dev_get_drvdata(dev);
	u32 offset = 0;

	if (sscanf(buf, "0x%x", &offset) != 1) {
		return -EINVAL;
	}

	cplddev->reg_offset = (u8)offset;
	return count;
}

static int accton_cpld_read_internal(struct spi_device *spi, u8 reg)
{
	int status = 0;
	unsigned int retry = SPI_RW_RETRY_COUNT;
	struct cplddev_data *cplddev = spi_get_drvdata(spi);

	cplddev->tx_buffer[0] = CPLD_READ_OPCODE;
	cplddev->tx_buffer[1] = reg;
	cplddev->tx_buffer[2] = 0;

	while (retry) {
		status = spi_write_then_read(spi, cplddev->tx_buffer,
					CPLD_WRITE_DATA_LEN, cplddev->rx_buffer, CPLD_READ_DATA_LEN);

		if (unlikely(status < 0)) {
			msleep(SPI_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}
		break;
	}

	if (0 == status)
	{
		status = cplddev->rx_buffer[0];
	}

	return status;
}

static int accton_cpld_write_internal(struct spi_device *spi, u8 reg, u8 value)
{
	int status = 0;
	unsigned int retry = SPI_RW_RETRY_COUNT;
	struct cplddev_data *cplddev = spi_get_drvdata(spi);

	cplddev->tx_buffer[0] = CPLD_WRITE_OPCODE;
	cplddev->tx_buffer[1] = reg;
	cplddev->tx_buffer[2] = value;

	while (retry) {
		status = spi_write_then_read(spi, cplddev->tx_buffer,
						CPLD_WRITE_DATA_LEN, NULL, 0);

		if (unlikely(status < 0)) {
			msleep(SPI_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}
		break;
	}

	return status;
}

int accton_cpld_read(unsigned short cpld_type, u8 reg)
{
	struct list_head   *list_node = NULL;
	struct cpld_device_node *cpld_node = NULL;
	struct cplddev_data *cplddev = NULL;
	int ret = -EPERM;

	mutex_lock(&list_lock);
	list_for_each(list_node, &cpld_device_list) {
		cpld_node = list_entry(list_node, struct cpld_device_node, list);

		cplddev = spi_get_drvdata(cpld_node->spi);
		if (cplddev->type == cpld_type) {
			ret = accton_cpld_read_internal(cpld_node->spi, reg);
			break;
		}
	}
	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(accton_cpld_read);

int accton_cpld_write(unsigned short cpld_type, u8 reg, u8 val)
{
	struct list_head *list_node = NULL;
	struct cpld_device_node *cpld_node = NULL;
	struct cplddev_data *cplddev = NULL;
	int ret = -EIO;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_device_list) {
		cpld_node = list_entry(list_node, struct cpld_device_node, list);

		cplddev = spi_get_drvdata(cpld_node->spi);
		if (cplddev->type == cpld_type) {
			ret = accton_cpld_write_internal(cpld_node->spi, reg, val);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(accton_cpld_write);

int accton_cpld_write_with_mask(unsigned short cpld_type, u8 reg, u8 val, u8 mask)
{
	struct list_head *list_node = NULL;
	struct cpld_device_node *cpld_node = NULL;
	struct cplddev_data *cplddev = NULL;
	int status = -EIO;
	u8 cpld_write_val, cpld_cur_val;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_device_list) {
		cpld_node = list_entry(list_node, struct cpld_device_node, list);

		cplddev = spi_get_drvdata(cpld_node->spi);
		if (cplddev->type == cpld_type) {
			status = accton_cpld_read_internal(cplddev->spi, reg);
			if (unlikely(status < 0)) {
				goto exit;
			}

			cpld_cur_val = (u8)status;
			cpld_cur_val = cpld_cur_val & (~mask);

			cpld_write_val = cpld_cur_val | (val & mask);

			status = accton_cpld_write_internal(cpld_node->spi, reg, cpld_write_val);
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
EXPORT_SYMBOL(accton_cpld_write_with_mask);

static int accton_cpld_add_device(struct spi_device *spi)
{
	struct cplddev_data *cplddev = dev_get_drvdata(&spi->dev);
	struct cpld_device_node *node = kzalloc(sizeof(struct cpld_device_node), GFP_KERNEL);

	if (!node) {
		dev_err(&spi->dev, "Can't allocate cpld_device_node, type = %d\n", cplddev->type);
		return -1;
	}

	node->spi = spi;

	mutex_lock(&list_lock);
	list_add(&node->list, &cpld_device_list);
	mutex_unlock(&list_lock);

	return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id accton_cpld_of_ids[] = {
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
MODULE_DEVICE_TABLE(of, accton_cpld_of_ids);
#endif

static const struct spi_device_id accton_cpld_ids[] = {
	{ "ctrl_cpld",   CTRL_CPLD },
	{ "port_cpld",   PORT_CPLD },
	{ "port_cpld2",  PORT_CPLD2 },
	{},
};
MODULE_DEVICE_TABLE(spi, accton_cpld_ids);

static void accton_cpld_remove_device(struct spi_device *spi)
{
	struct list_head *list_node = NULL;
	struct cpld_device_node *cpld_node = NULL;
	int found = 0;

	mutex_lock(&list_lock);

	list_for_each(list_node, &cpld_device_list) {
		cpld_node = list_entry(list_node, struct cpld_device_node, list);

		if (cpld_node->spi == spi) {
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

static int accton_cpld_probe(struct spi_device *spi)
{
	const struct of_device_id *match;
	struct cplddev_data *cplddev;
	const struct attribute_group *group = NULL;
	int ret = 0;
	int type;

	match = of_match_device(accton_cpld_of_ids, &spi->dev);
	if (match)
		type = (int)(uintptr_t)match->data;
	else
		type = spi_get_device_id(spi)->driver_data;

	/* Allocate driver data */
	cplddev = kzalloc(sizeof(*cplddev), GFP_KERNEL);
	if (!cplddev)
		return -ENOMEM;

	/* Initialize the driver data */
	cplddev->spi = spi;
	cplddev->type = type;
	spi_set_drvdata(spi, cplddev);

	ret = spi_setup(spi);
	if (ret) {
		dev_err(&spi->dev, "cpld device %d spi_setup fail\n", cplddev->type);
		goto spi_setup_failed;
	}

	mutex_init(&cplddev->update_lock);

	/* Register sysfs hooks */
	switch (cplddev->type) {
	case CTRL_CPLD:
	case PORT_CPLD:
	case PORT_CPLD2:
		group = &accton_cpld_group;
		break;
	default:
		;
	}

	ret = accton_cpld_read_internal(cplddev->spi, CPLD_TEST_REG);
	if (ret == 0xff) {
		dev_err(&spi->dev, "cpld device %d test fail\n", cplddev->type);
		goto cpld_test_failed;
	}

	if (group) {
		ret = sysfs_create_group(&spi->dev.kobj, group);
		if (ret) {
			dev_err(&spi->dev, "cpld device %d sysfs create fail\n", cplddev->type);
			goto add_sysfs_failed;
		}
	}

	ret = accton_cpld_add_device(spi);
	if (ret) {
		dev_err(&spi->dev, "cpld device %d add_device fail\n", cplddev->type);
		goto add_device_failed;
	}

	dev_info(&spi->dev, "cpld device %d registered\n", cplddev->type);

	return 0;

add_device_failed:
	if (group) {
		sysfs_remove_group(&spi->dev.kobj, group);
	}

add_sysfs_failed:
cpld_test_failed:
spi_setup_failed:
	kfree(cplddev);
	return ret;
}

static void accton_cpld_remove(struct spi_device *spi)
{
	const struct attribute_group *group = NULL;
	struct cplddev_data *cplddev = dev_get_drvdata(&spi->dev);

	/* Remove sysfs hooks */
	switch (cplddev->type) {
	case CTRL_CPLD:
	case PORT_CPLD:
	case PORT_CPLD2:
		group = &accton_cpld_group;
		break;
	default:
		;
	}

	if (group) {
		sysfs_remove_group(&spi->dev.kobj, group);
	}

	accton_cpld_remove_device(spi);
	kfree(cplddev);

	return;
}

static struct spi_driver accton_cpld_driver = {
	.driver = {
		.name   = "accton_cpld",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(accton_cpld_of_ids),
	},
	.probe      = accton_cpld_probe,
	.remove     = accton_cpld_remove,
	.id_table   = accton_cpld_ids,
};

static int accton_cpld_init(void)
{
	int status;

	mutex_init(&list_lock);
	status = spi_register_driver(&accton_cpld_driver);
	if (status < 0) {
		printk(KERN_ERR "register accton cpld spi driver fail!\n");
	}
	return status;
}

static void accton_cpld_exit(void)
{
	spi_unregister_driver(&accton_cpld_driver);
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Ang Liu <ang_liu@accton.com.cn>");
MODULE_DESCRIPTION("Accton SPI CPLD driver");

module_init(accton_cpld_init);
module_exit(accton_cpld_exit);
