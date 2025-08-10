// SPDX-License-Identifier: GPL-2.0+
/*
 * xcvr-cls.c - front panel port control.
 *
 * Nicholas Wu <nicwu@celestica.com>
 * Copyright (C) 2019 Celestica Corp.
 */

#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/hwmon.h>
#include "fpga_xcvr.h"

/* FPGA front panel  */
#define PORT_TEST           0
#define PORT_CR_STATUS      0x4

/*
 * Port control degister
 * TXDIS    : active high, RW
*/

#define CTRL_TXDIS      BIT(3)

/*
 * Port status register
 * TXFAULT  : active high, RO
 * RXLOS    : active high, RO
 * MODABS   : active high, RO, for SFP
*/
#define STAT_TXFAULT    BIT(2)
#define STAT_MODABS     BIT(1)
#define STAT_RXLOS      BIT(0)

/*
 * port_data - optical port data
 * @xcvr: xcvr memory accessor
 * @name: port name
 * @index: front panel port index starting from 1
 */
struct port_data {
	struct xcvr_priv *xcvr;
	const char *name;
	unsigned int index;
};

/*
 * xcvr_priv - port xcvr private data
 * @dev: device for reference
 * @base: virtual base address
 * @num_ports: number of front panel ports
 * @fp_devs: list of front panel port devices
 */
struct xcvr_priv {
	struct device* dev;
	void __iomem *base;
	int port_reg_size;
	int num_ports;
	struct device **fp_devs;
};

static inline void port_setreg(struct xcvr_priv *xcvr, int reg, int index, u8 value)
{
	return iowrite8(value, xcvr->base + reg + (index - 1) * xcvr->port_reg_size);
}

static inline u8 port_getreg(struct xcvr_priv *xcvr, int reg, int index)
{
	return ioread8(xcvr->base + reg + (index - 1) * xcvr->port_reg_size);
}

static ssize_t sfp_modabs_show(struct device *dev, 
			       struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_CR_STATUS, index);
	return sprintf(buf, "%d\n", (data & STAT_MODABS)?1:0);
}

static ssize_t sfp_txfault_show(struct device *dev, 
			       struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_CR_STATUS, index);
	return sprintf(buf, "%d\n", (data & STAT_TXFAULT)?1:0);
}

static ssize_t sfp_rxlos_show(struct device *dev, 
			      struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_CR_STATUS, index);
	return sprintf(buf, "%d\n", (data & STAT_RXLOS)?1:0);
}

static ssize_t sfp_txdisable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_CR_STATUS, index);
	return sprintf(buf, "%d\n", (data & CTRL_TXDIS)?1:0);
}

static ssize_t sfp_txdisable_store(struct device *dev, 
				   struct device_attribute *attr, 
				   const char *buf, size_t size)
{
	ssize_t status;
	long value;
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	status = kstrtol(buf, 0, &value);
	if (status == 0) {
		data = port_getreg(port_data->xcvr, PORT_CR_STATUS, index);
		if (value == 0)
			data &= ~CTRL_TXDIS;
		else
			data |= CTRL_TXDIS;
		port_setreg(port_data->xcvr, PORT_CR_STATUS, index, data);
		status = size;
	}
	return status;
}

DEVICE_ATTR_RO(sfp_modabs);
DEVICE_ATTR_RO(sfp_txfault);
DEVICE_ATTR_RO(sfp_rxlos);
DEVICE_ATTR_RW(sfp_txdisable);

/* sfp_attrs */
static struct attribute *sfp_attrs[] = {
	&dev_attr_sfp_modabs.attr,
	&dev_attr_sfp_txfault.attr,
	&dev_attr_sfp_rxlos.attr,
	&dev_attr_sfp_txdisable.attr,
	NULL
};

ATTRIBUTE_GROUPS(sfp);

/* A single port device init */
static struct device* init_port(struct device *dev, 
				struct xcvr_priv *xcvr, 
				struct port_info info, 
				const struct attribute_group **groups) 
{
	struct port_data *new_data;

	new_data = devm_kzalloc(dev, sizeof(struct port_data), GFP_KERNEL);
	if (!new_data)
		return ERR_PTR(-ENOMEM);

	new_data->index = info.index;
	new_data->name = info.name;
	new_data->xcvr = xcvr;

	return devm_hwmon_device_register_with_groups(dev, 
						      info.name, 
						      new_data, 
						      groups);
}

static void xcvr_cleanup(struct xcvr_priv *xcvr)
{
	struct device *dev;
	struct port_data *data;
	int i;

	for (i = 0; i < xcvr->num_ports; i++){
		dev = xcvr->fp_devs[i];
		if (dev == NULL)
			continue;
		data = dev_get_drvdata(dev);
		sysfs_remove_link(&xcvr->dev->kobj, data->name);
	}
}

static int cls_xcvr_probe(struct platform_device *pdev)
{

	struct xcvr_priv *xcvr;
	struct cls_xcvr_platform_data *pdata;
	struct resource *res;
	int ret;
	int i;

	struct device **port_devs;

	xcvr = devm_kzalloc(&pdev->dev, sizeof(struct xcvr_priv), GFP_KERNEL);
	if (!xcvr){
		ret = -ENOMEM;
		goto err_exit;
	}

	dev_set_drvdata(&pdev->dev, xcvr);

	/* mmap resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		xcvr->base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(xcvr->base)){
			ret = PTR_ERR(xcvr->base);
			goto err_exit;
		}
	}

	pdata = dev_get_platdata(&pdev->dev);
	xcvr->dev = &pdev->dev;

	if (pdata) {
		/* assign pdata */
		xcvr->num_ports = pdata->num_ports;
		xcvr->port_reg_size = pdata->port_reg_size;
	}

	/* alloc front panel device list */
	port_devs = devm_kzalloc(&pdev->dev, 
				 xcvr->num_ports * sizeof(struct device*), 
				 GFP_KERNEL);
	if (!port_devs){
		ret = -ENOMEM;
		goto err_exit;
	}


	if (pdata) {
		/* create each device attrs group determined by type */
		for (i = 0; i < pdata->num_ports; i++) {
			struct device *fp_dev;
			fp_dev = init_port(&pdev->dev,
					   xcvr, 
					   pdata->devices[i], 
					   sfp_groups);
			if (IS_ERR(fp_dev)) {
				dev_err(&pdev->dev, 
					"Failed to init port %s\n", 
					pdata->devices[i].name);
				ret = PTR_ERR(fp_dev);
				goto dev_clean_up;
			}

			dev_info(&pdev->dev, 
				 "Register port %s\n", 
				 pdata->devices[i].name);

			WARN(sysfs_create_link(&pdev->dev.kobj, 
					       &fp_dev->kobj, 
					       pdata->devices[i].name),
			     "can't create symlink to %s\n", pdata->devices[i].name);
			port_devs[i] = fp_dev;
			fp_dev = NULL;
		}
		xcvr->fp_devs = port_devs;
	}

	return 0;

dev_clean_up:
	xcvr_cleanup(xcvr);
err_exit:
	return ret;

}


static int cls_xcvr_remove(struct platform_device *pdev)
{
	struct xcvr_priv *xcvr = dev_get_drvdata(&pdev->dev);
	xcvr_cleanup(xcvr);
	return 0;
}

static struct platform_driver cls_xcvr_driver = {
	.probe = cls_xcvr_probe,
	.remove = cls_xcvr_remove,
	.driver = {
		.name = "cls-xcvr",
	},
};

static int __init drv_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&cls_xcvr_driver);
	return rc; 
}

static void __exit drv_exit(void)
{
    platform_driver_unregister(&cls_xcvr_driver);
}

module_init(drv_init);
module_exit(drv_exit);

MODULE_AUTHOR("Nicholas Wu<nicwu@celestica.com>");
MODULE_DESCRIPTION("Celestica xcvr control driver");
MODULE_VERSION("2.0.0");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cls-xcvr");

