// SPDX-License-Identifier: GPL-2.0+
/*
 * xcvr-cls.c - front panel port control.
 *
 * Pradchaya Phucharoen <pphuchar@celestica.com>
 * Copyright (C) 2019 Celestica Corp.
 */

#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/hwmon.h>
#include "xcvr-cls.h"

/* FPGA front panel  */
#define PORT_CTRL           0
#define PORT_STATUS         0x4
#define PORT_INT_STATUS     0x8
#define PORT_INT_MASK       0xC

/*
 * Port control degister
 * LPMOD    : active high, RW
 * RST      : active low,  RW
 * TXDIS    : active high, RW
*/
#define CTRL_LPMOD      BIT(6)
#define CTRL_RST_L      BIT(4)
#define CTRL_TXDIS      BIT(0)

/*
 * Port status register
 * IRQ      : active low, RO
 * PRESENT  : active low,  RO, for QSFP
 * TXFAULT  : active high, RO
 * RXLOS    : active high, RO
 * MODABS   : active high, RO, for SFP
*/
#define STAT_IRQ_L      BIT(5)
#define STAT_PRESENT_L  BIT(4)
#define STAT_TXFAULT    BIT(2)
#define STAT_RXLOS      BIT(1)
#define STAT_MODABS     BIT(0)

/*
 * NOTE: Interrupt and mask must be expose as bitfeild.
 *       Because the registers of interrupt flags are read-clear.
 *
 * Port interrupt flag resgister
 * INT_N    : interrupt flag, set when INT_N is assert.
 * PRESENT  : interrupt flag, set when QSFP module plugin/plugout.
 * RXLOS    : interrupt flag, set when rxlos is assert.
 * MODABS   : interrupt flag, set when SFP module plugin/plugout.
*/
#define INTR_INT_N      BIT(5)
#define INTR_PRESENT    BIT(4)
#define INTR_TXFAULT    BIT(2)
#define INTR_RXLOS      BIT(1)
#define INTR_MODABS     BIT(0)

/* 
 * Port interrupt mask register
 * INT_N     : active low
 * PRESENT   : active low
 * RXLOS_INT : active low
 * MODABS    : active low
*/
#define MASK_INT_N_L      BIT(5)
#define MASK_PRESENT_L    BIT(4)
#define MASK_TXFAULT_L    BIT(2)
#define MASK_RXLOS_L      BIT(1)
#define MASK_MODABS_L     BIT(0)


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

static ssize_t qsfp_modprsL_show(struct device *dev, 
				 struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_STATUS, index);
	return sprintf(buf, "%d\n", (data & STAT_PRESENT_L)?1:0);
}

static ssize_t qsfp_irqL_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_STATUS, index);
	return sprintf(buf, "%d\n", (data & STAT_IRQ_L)?1:0);
}

static ssize_t qsfp_lpmode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_CTRL, index);
	return sprintf(buf, "%d\n", (data & CTRL_LPMOD)?1:0);
}

static ssize_t qsfp_lpmode_store(struct device *dev, 
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
		data = port_getreg(port_data->xcvr, PORT_CTRL, index);
		if (value == 0)
			data &= ~CTRL_LPMOD;
		else
			data |= CTRL_LPMOD;
		port_setreg(port_data->xcvr, PORT_CTRL, index, data);
		status = size;
	}
	return status;
}

static ssize_t qsfp_resetL_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_CTRL, index);
	return sprintf(buf, "%d\n", (data & CTRL_RST_L)?1:0);
}

static ssize_t qsfp_resetL_store(struct device *dev, 
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
		data = port_getreg(port_data->xcvr, PORT_CTRL, index);
		if (value == 0)
			data &= ~CTRL_RST_L;
		else
			data |= CTRL_RST_L;
		port_setreg(port_data->xcvr, PORT_CTRL, index, data);
		status = size;
	}
	return status;
}

static ssize_t sfp_modabs_show(struct device *dev, 
			       struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_STATUS, index);
	return sprintf(buf, "%d\n", (data & STAT_MODABS)?1:0);
}

static ssize_t sfp_txfault_show(struct device *dev, 
			       struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_STATUS, index);
	return sprintf(buf, "%d\n", (data & STAT_TXFAULT)?1:0);
}

static ssize_t sfp_rxlos_show(struct device *dev, 
			      struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_STATUS, index);
	return sprintf(buf, "%d\n", (data & STAT_RXLOS)?1:0);
}

static ssize_t sfp_txdisable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_CTRL, index);
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
		data = port_getreg(port_data->xcvr, PORT_CTRL, index);
		if (value == 0)
			data &= ~CTRL_TXDIS;
		else
			data |= CTRL_TXDIS;
		port_setreg(port_data->xcvr, PORT_CTRL, index, data);
		status = size;
	}
	return status;
}

static ssize_t interrupt_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_INT_STATUS, index);
	return sprintf(buf, "0x%2.2x\n", data);
}

static ssize_t interrupt_store(struct device *dev, 
			       struct device_attribute *attr, 
			       const char *buf, size_t size)
{
	ssize_t status;
	long value;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	status = kstrtoul(buf, 0, &value);
	if (status == 0) {
		port_setreg(port_data->xcvr, PORT_INT_STATUS, index, value);
		status = size;
	}
	return status;
}

static ssize_t interrupt_mask_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	u8 data;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	data = port_getreg(port_data->xcvr, PORT_INT_MASK, index);
	return sprintf(buf, "0x%2.2x\n", data);
}

static ssize_t interrupt_mask_store(struct device *dev, 
				    struct device_attribute *attr, 
				    const char *buf, size_t size)
{
	ssize_t status;
	long value;
	struct port_data *port_data = dev_get_drvdata(dev);
	unsigned int index = port_data->index;

	status = kstrtoul(buf, 0, &value);
	if (status == 0) {
		port_setreg(port_data->xcvr, PORT_INT_MASK, index, value);
		status = size;
	}
	return status;
}

DEVICE_ATTR_RO(qsfp_modprsL);
DEVICE_ATTR_RO(qsfp_irqL);
DEVICE_ATTR_RW(qsfp_lpmode);
DEVICE_ATTR_RW(qsfp_resetL);

DEVICE_ATTR_RO(sfp_modabs);
DEVICE_ATTR_RO(sfp_txfault);
DEVICE_ATTR_RO(sfp_rxlos);
DEVICE_ATTR_RW(sfp_txdisable);

DEVICE_ATTR_RW(interrupt);
DEVICE_ATTR_RW(interrupt_mask);

/* qsfp_attrs */
static struct attribute *qsfp_attrs[] = {
	&dev_attr_qsfp_modprsL.attr,
	&dev_attr_qsfp_lpmode.attr,
	&dev_attr_qsfp_resetL.attr,
	&dev_attr_interrupt.attr,
	&dev_attr_interrupt_mask.attr,
	NULL
};

/* sfp_attrs */
static struct attribute *sfp_attrs[] = {
	&dev_attr_sfp_modabs.attr,
	&dev_attr_sfp_txfault.attr,
	&dev_attr_sfp_rxlos.attr,
	&dev_attr_sfp_txdisable.attr,
	&dev_attr_interrupt.attr,
	&dev_attr_interrupt_mask.attr,
	NULL
};

ATTRIBUTE_GROUPS(qsfp);
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

			if (pdata->devices[i].type == SFP){
				fp_dev = init_port(&pdev->dev,
						   xcvr, 
						   pdata->devices[i], 
						   sfp_groups);
			}else{
				fp_dev = init_port(&pdev->dev,
						   xcvr,  
						   pdata->devices[i], 
						   qsfp_groups);
			}
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


static void cls_xcvr_remove(struct platform_device *pdev)
{
	struct xcvr_priv *xcvr = dev_get_drvdata(&pdev->dev);
	xcvr_cleanup(xcvr);
}


static struct platform_driver cls_xcvr_driver = {
	.probe = cls_xcvr_probe,
	.remove = cls_xcvr_remove,
	.driver = {
		.name = "cls-xcvr",
	},
};

module_platform_driver(cls_xcvr_driver);

MODULE_AUTHOR("Pradchaya Phucharoen<pphuchar@celestica.com>");
MODULE_DESCRIPTION("Celestica xcvr control driver");
MODULE_VERSION("0.0.1-3");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cls-xcvr");
