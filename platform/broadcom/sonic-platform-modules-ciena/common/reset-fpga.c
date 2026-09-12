/*
 * Copyright 2016 Ciena Inc
 *
 * based on
 * Allwinner SoCs Reset Controller driver
 *
 * Copyright 2013 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset-controller.h>
#include <linux/types.h>

#include "reset-fpga.h"
#include "reset-sysfs.h"

/*
 * Support a simple register based reset-contoroller
 *
 * Currently we assume that when a bit is set, the reset line
 * is asserted.
 */

struct ciena_fpga_reset_data {
	struct mutex                 lock;
	struct i2c_client           *i2cbase;
	struct regmap               *parent_regmap;
	void __iomem                *membase;
	bool                         shared_io;
	bool                         negative;
	bool                         use_raw_value;
	int                          width;
	u32                          i2ccmd;
	struct reset_controller_dev  rcdev;
	uint32_t                     max_names;
	const char                 **names;
	uint32_t                   (*rd)(struct ciena_fpga_reset_data *data);
	void                       (*wr)(struct ciena_fpga_reset_data *data,
					 uint32_t value);
};


static uint32_t ciena_i2c_rd(struct ciena_fpga_reset_data *data)
{
	s32 result = 0;

	switch (data->width) {
	case sizeof(u8):
		result = i2c_smbus_read_byte_data(data->i2cbase, data->i2ccmd);
		break;

	case sizeof(u16):
		result = i2c_smbus_read_word_data(data->i2cbase, data->i2ccmd);
		break;

	default:
		BUG();
	}

	return (uint32_t) result;
}

static void ciena_i2c_wr(struct ciena_fpga_reset_data *data, uint32_t value)
{
	switch (data->width) {
	case sizeof(u8):
		i2c_smbus_write_byte_data(data->i2cbase, data->i2ccmd, value);
		break;

	case sizeof(u16):
		i2c_smbus_write_word_data(data->i2cbase, data->i2ccmd, value);
		break;

	default:
		BUG();
	}
}

static uint32_t ciena_regmap_rd(struct ciena_fpga_reset_data *data)
{
	unsigned int offset = data->membase - (void *) NULL;
	unsigned int val    = ~0;
	int          rc;

	rc = regmap_read(data->parent_regmap, offset, &val);
	BUG_ON(rc);

	return val;
}

static void ciena_regmap_wr(struct ciena_fpga_reset_data *data, uint32_t value)
{
	unsigned int offset = data->membase - (void *) NULL;
	int          rc;

	rc = regmap_write(data->parent_regmap, offset, value);
	BUG_ON(rc);
}

static uint32_t ciena_reg_rd(struct ciena_fpga_reset_data *data)
{
	switch (data->width) {
	case sizeof(u8):
		return (uint32_t) readb(data->membase);

	case sizeof(u16):
		return (uint32_t) readw(data->membase);

	case sizeof(u32):
		return (uint32_t) readl(data->membase);

	default:
		BUG();
	}

	return 0;
}


static void ciena_reg_wr(struct ciena_fpga_reset_data *data, uint32_t value)
{
	switch (data->width) {
	case sizeof(u8):
		writeb(value, data->membase);
		break;

	case sizeof(u16):
		writew(value, data->membase);
		break;

	case sizeof(u32):
		writel(value, data->membase);
		break;

	default:
		BUG();
	}
}

static void ciena_fpga_reset_id_fixup(struct reset_controller_dev *rcdev,
				      unsigned long *id, bool *negative)
{
	struct ciena_fpga_reset_data *data;
	const char *reg_name;

	if (rcdev->dev) reg_name = dev_name(rcdev->dev);
	else reg_name = rcdev->of_node->name;

	data = container_of(rcdev, struct ciena_fpga_reset_data, rcdev);
	*negative = data->negative;

	if (ciena_fpga_bit_inversed & *id) {
		*negative ^= true;
		*id ^= ciena_fpga_bit_inversed;
		pr_debug("%s: %s: dysfunctional %s bit 0x%lx\n", reg_name,
			 __func__, *negative ? "negative" : "positive" , *id);
	}
}

static int ciena_fpga_reset_assert(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	struct ciena_fpga_reset_data *data;
	const char *nm = "???";
	bool negative = false;
	uint32_t reg;

	data = container_of(rcdev, struct ciena_fpga_reset_data, rcdev);

	if (data->use_raw_value) {
		pr_info("%s: assert value 0x%lx\n",
			(rcdev->dev ? dev_name(rcdev->dev) :
			 rcdev->of_node->name),
			id);
		data->wr(data, id);
		return 0;
	}

	if (data->names && (id < data->max_names) && data->names[id])
		nm = data->names[id];

	if (rcdev->dev) dev_info(rcdev->dev, "assert bit %lx %s\n", id, nm);
	else pr_info("%s: assert bit %lx %s\n", rcdev->of_node->name, id, nm);

	ciena_fpga_reset_id_fixup(rcdev, &id, &negative);

	mutex_lock(&data->lock);

	reg = data->rd(data);
	if (negative) data->wr(data, reg & ~BIT(id));
	else data->wr(data, reg | BIT(id));

	mutex_unlock(&data->lock);

	return 0;
}


static int ciena_fpga_reset_deassert(struct reset_controller_dev *rcdev,
				     unsigned long id)
{
	struct ciena_fpga_reset_data *data;
	const char *nm = "???";
	bool negative = false;
	uint32_t reg;

	data = container_of(rcdev, struct ciena_fpga_reset_data, rcdev);

	if (data->use_raw_value) {
		pr_info("%s: nothing to deassert\n",
			(rcdev->dev ? dev_name(rcdev->dev) :
			 rcdev->of_node->name));
		return 0;
	}

	if (data->names && (id < data->max_names) && data->names[id])
		nm = data->names[id];

	if (rcdev->dev) dev_info(rcdev->dev, "deassert bit %lx %s\n", id, nm);
	else pr_info("%s: deassert bit %lx %s\n", rcdev->of_node->name, id, nm);

	ciena_fpga_reset_id_fixup(rcdev, &id, &negative);

	mutex_lock(&data->lock);

	reg = data->rd(data);
	if (negative) data->wr(data, reg | BIT(id));
	else data->wr(data, reg & ~BIT(id));

	mutex_unlock(&data->lock);

	return 0;
}


static int ciena_fpga_reset_reset(struct reset_controller_dev *rcdev,
				  unsigned long id)
{
	ciena_fpga_reset_assert(rcdev, id);

	udelay(500);

	ciena_fpga_reset_deassert(rcdev, id);

	return 0;
}


static int ciena_fpga_reset_status(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	struct ciena_fpga_reset_data *data;
	bool negative = false;
	uint32_t reg;
	int value;

	data = container_of(rcdev, struct ciena_fpga_reset_data, rcdev);

	if (data->use_raw_value) {
		reg = data->rd(data);
		value = (reg==id) ? 1 : 0;
		return value;
	}

	ciena_fpga_reset_id_fixup(rcdev, &id, &negative);

	reg = data->rd(data);

	value = (reg & BIT(id)) ? 1 : 0;

	return value ^ negative;
}


static struct reset_control_ops ciena_fpga_reset_ops = {
	.assert		= ciena_fpga_reset_assert,
	.deassert	= ciena_fpga_reset_deassert,
	.status		= ciena_fpga_reset_status,
	.reset		= ciena_fpga_reset_reset
};


static int ciena_fpga_reset_parse_of(struct platform_device *pdev,
				     struct device_node *node,
				     struct ciena_fpga_reset_data *data)
{
	struct ciena_sysfs_reset_names reset_names;
	struct device_node *i2c_parent;
	const char *i2c_of_parent;
	const u32 *prop;
	u32 max;
	int rc;

	if (!node)
		return 0;

	if (!of_property_read_string(node, "i2c-parent", &i2c_of_parent)) {
		dev_dbg(&pdev->dev, "has i2c-parent %s\n", i2c_of_parent);
		i2c_parent = of_find_node_by_path(i2c_of_parent);
		if (i2c_parent) {
			dev_dbg(&pdev->dev, "found i2c-parent %s\n",
				i2c_parent->full_name);
			data->i2cbase = of_find_i2c_device_by_node(i2c_parent);
			if (!data->i2cbase) {
				dev_dbg(&pdev->dev, "deferring probe of %s\n",
					node->full_name);
				return -EPROBE_DEFER;
			}
			rc = of_property_read_u32(node, "i2c-reg",
						  &data->i2ccmd);
			if (rc) {
				dev_err(&pdev->dev, "no i2c-reg (%d)\n", rc);
				return rc;
			}
		}
		else {
			dev_err(&pdev->dev, "cannot find parent %s\n",
				i2c_of_parent);
			return -ENODEV;
		}
	}

	max = ciena_sysfs_of_reset_names(node, &reset_names);
	if (max) {
		data->max_names = max;
		data->names     = devm_kmalloc_array(&pdev->dev, max,
						     sizeof(*data->names),
						     GFP_KERNEL);
		if (NULL == data->names) {
			dev_err(&pdev->dev, "no memory for %u names\n", max);
			return -ENOMEM;
		}

		while (max--)
			data->names[max] = reset_names.nm[max];
	}

	prop = of_get_property(node, "ciena,shared-io", NULL);
	if (prop)
		data->shared_io = true;

	prop = of_get_property(node, "ciena,negative-logic", NULL);
	if (prop)
		data->negative = true;

	prop = of_get_property(node, "ciena,8-bit", NULL);
	if (prop) {
		data->width = sizeof(u8);
		return 0;
	}

	prop = of_get_property(node, "ciena,16-bit", NULL);
	if (prop) {
		data->width = sizeof(u16);
		return 0;
	}

	prop = of_get_property(node, "ciena,32-bit", NULL);
	if (prop) {
		data->width = sizeof(u32);
		return 0;
	}

	return 0;
}

static int ciena_fpga_reset_parse_pdata(struct platform_device *pdev,
					struct ciena_fpga_reset_data *data)
{
	struct ciena_fpga_reset_pdata *pdata = pdev->dev.platform_data;

	if (pdata) {
		/* use the platform data if it is provided */
		data->width         = pdata->reg_size;
		data->shared_io     = (0 != pdata->shared_io);
		data->negative      = (0 != pdata->negative);
		data->use_raw_value = (0 != pdata->use_raw_value);
		data->i2cbase       = pdata->i2cdev;
		data->i2ccmd        = pdata->i2creg;
		data->parent_regmap = pdata->parent_regmap;
		data->max_names     = pdata->num_names;
		data->names         = pdata->reset_names;
		return 0;
	}

	return -EINVAL;
}

static int ciena_fpga_reset_i2c_init(struct platform_device *pdev,
				     struct ciena_fpga_reset_data *data)
{
	const u8 max_cmd = ~0;
	int rc;

	if (max_cmd < data->i2ccmd) {
		dev_err(&pdev->dev, "reg too large (%u)\n", data->i2ccmd);
		return -EINVAL;
	}

	switch (data->width) {
		case sizeof(u8):
			rc = i2c_check_functionality(data->i2cbase->adapter,
						     I2C_FUNC_SMBUS_BYTE_DATA);
			break;
		case sizeof(u16):
			rc = i2c_check_functionality(data->i2cbase->adapter,
						     I2C_FUNC_SMBUS_WORD_DATA);
			break;
		default:
			dev_err(&pdev->dev, "invalid data width (%d)\n",
				data->width);
			return -EINVAL;
	}

	if (0 == rc) {
		dev_err(&pdev->dev, "missing smbus functionality\n");
		return -EIO;
	}

	data->rd = ciena_i2c_rd;
	data->wr = ciena_i2c_wr;

	return 0;
}

static int ciena_fpga_reset_regmap_init(struct platform_device *pdev,
					struct ciena_fpga_reset_data *data)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_REG, 0);
	if (!res) {
		dev_err(&pdev->dev, "%s: no register resource\n", __func__);
		return -EINVAL;
	}

	data->membase = ((void *) NULL) + res->start;

	data->rd = ciena_regmap_rd;
	data->wr = ciena_regmap_wr;

	return 0;
}

static int ciena_fpga_reset_mem_init(struct platform_device *pdev,
				     struct ciena_fpga_reset_data *data)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "%s: no memory resource\n", __func__);
		return -EINVAL;
	}

	if (data->shared_io) {
		resource_size_t size;
		size = resource_size(res);

		data->membase = devm_ioremap(&pdev->dev,
					     res->start, size);
	} else {
		data->membase = devm_ioremap_resource(&pdev->dev, res);
	}

	if (IS_ERR(data->membase)) {
		dev_err(&pdev->dev, "%s: failed to allocate resource (%ld)\n",
			__func__, PTR_ERR(data->membase));
		return PTR_ERR(data->membase);
	}

	data->rd = ciena_reg_rd;
	data->wr = ciena_reg_wr;

	return 0;
}

static int ciena_fpga_reset_probe(struct platform_device *pdev)
{
	struct ciena_fpga_reset_data *data;
	struct device_node *node = pdev->dev.of_node;
	struct device_node *parent;
	int rc;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) return -ENOMEM;

	mutex_init(&data->lock);

	if (node) {
		if (!of_find_property(node, "#reset-cells", NULL)) {
			dev_err(&pdev->dev, "%s no #reset-cells property\n",
				node->full_name);
			return -EINVAL;
		}

		parent = of_get_parent(node);
		if (parent) {
			ciena_fpga_reset_parse_of(pdev, parent, data);
			of_node_put(parent);
		}

		rc = ciena_fpga_reset_parse_of(pdev, node, data);
	}
	else rc = ciena_fpga_reset_parse_pdata(pdev, data);
	if (rc) return rc;

	data->rcdev.owner = THIS_MODULE;
	data->rcdev.ops = &ciena_fpga_reset_ops;
	data->rcdev.dev = &pdev->dev;
	data->rcdev.of_node = node;
	data->rcdev.nr_resets = data->width * 8;

	if (data->i2cbase)
		rc = ciena_fpga_reset_i2c_init(pdev, data);
	else if (data->parent_regmap)
		rc = ciena_fpga_reset_regmap_init(pdev, data);
	else
		rc = ciena_fpga_reset_mem_init(pdev, data);

	if (rc) goto out_put;

	rc = reset_controller_register(&data->rcdev);
	if (rc) {
		dev_err(&pdev->dev, "%s: registration failed (%d)\n",
			__func__, rc);
		goto out_put;
	}

	dev_set_drvdata(&pdev->dev, data);

	dev_info(&pdev->dev, "created with %u reset pin%s\n",
		 data->rcdev.nr_resets, 1 < data->rcdev.nr_resets ? "s" : "");

	return 0;

out_put:
	if (data->i2cbase) put_device(&data->i2cbase->dev);
	return rc;
}


static void ciena_fpga_reset_remove(struct platform_device *pdev)
{
	struct ciena_fpga_reset_data *data = platform_get_drvdata(pdev);

	reset_controller_unregister(&data->rcdev);
	if (data->i2cbase) put_device(&data->i2cbase->dev);
}


static const struct of_device_id ciena_fpga_reset_dt_ids[] = {
	{
		.compatible = "ciena,fpga-reset",
		.type       = "reset-controller",
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ciena_fpga_reset_dt_ids);

static struct platform_driver ciena_fpga_reset_driver = {
	.probe	  = ciena_fpga_reset_probe,
	.remove	  = ciena_fpga_reset_remove,
	.driver   = {
		.name		= RESET_FPGA_DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= ciena_fpga_reset_dt_ids,
	},
};
module_platform_driver(ciena_fpga_reset_driver);

MODULE_AUTHOR("Dell Drummond");
MODULE_DESCRIPTION("Ciena FPGA Reset Controller Driver");
MODULE_LICENSE("GPL");
