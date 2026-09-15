/*
 * Copyright (C) 2021 Ciena Corporation
 * Author: Marc St-Amand <mstamand@ciena.com>
 *
 * This driver is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 2 of the License.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/types.h>

#include "reset-fpga.h"
#include "reset-sysfs.h"

struct ciena_gpio_reset_data {
	bool                         negative;
	int                          gpio_count;
	const char                 **names;
	struct gpio_desc           **gpiod;
	struct reset_controller_dev  rcdev;
};

static void ciena_gpio_reset_id_fixup(struct reset_controller_dev *rcdev,
				      unsigned long *id, bool *negative)
{
	struct ciena_gpio_reset_data *data;
	const char *reg_name;

	if (rcdev->dev) reg_name = dev_name(rcdev->dev);
	else reg_name = rcdev->of_node->name;

	data = container_of(rcdev, struct ciena_gpio_reset_data, rcdev);
	*negative = data->negative;

	if (ciena_fpga_bit_inversed & *id) {
		*negative ^= true;
		*id ^= ciena_fpga_bit_inversed;
		pr_debug("%s: %s: dysfunctional %s bit 0x%lx\n", reg_name,
			 __func__, *negative ? "negative" : "positive" , *id);
	}

	BUG_ON(*id >= data->gpio_count);
}

static int ciena_gpio_reset_assert(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	struct ciena_gpio_reset_data *data;
	const char *nm = "???";
	bool negative = false;

	data = container_of(rcdev, struct ciena_gpio_reset_data, rcdev);

	if (data->names && (id < data->gpio_count) && data->names[id])
		nm = data->names[id];

	if (rcdev->dev) dev_info(rcdev->dev, "assert pin %lx %s\n", id, nm);
	else pr_info("%s: assert pin %lx %s\n", rcdev->of_node->name, id, nm);

	ciena_gpio_reset_id_fixup(rcdev, &id, &negative);

	gpiod_set_value(data->gpiod[id], negative ? 0 : 1);

	return 0;
}


static int ciena_gpio_reset_deassert(struct reset_controller_dev *rcdev,
				     unsigned long id)
{
	struct ciena_gpio_reset_data *data;
	const char *nm = "???";
	bool negative = false;

	data = container_of(rcdev, struct ciena_gpio_reset_data, rcdev);

	if (data->names && (id < data->gpio_count) && data->names[id])
		nm = data->names[id];

	if (rcdev->dev) dev_info(rcdev->dev, "deassert bit %lx %s\n", id, nm);
	else pr_info("%s: deassert bit %lx %s\n", rcdev->of_node->name, id, nm);

	ciena_gpio_reset_id_fixup(rcdev, &id, &negative);

	gpiod_set_value(data->gpiod[id], negative ? 1 : 0);

	return 0;
}


static int ciena_gpio_reset_reset(struct reset_controller_dev *rcdev,
				  unsigned long id)
{
	ciena_gpio_reset_assert(rcdev, id);

	udelay(500);

	ciena_gpio_reset_deassert(rcdev, id);

	return 0;
}


static int ciena_gpio_reset_status(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	struct ciena_gpio_reset_data *data;
	bool negative = false;
	int value;

	data = container_of(rcdev, struct ciena_gpio_reset_data, rcdev);

	ciena_gpio_reset_id_fixup(rcdev, &id, &negative);

	value = gpiod_get_value(data->gpiod[id]);

	return value ^ negative;
}


static struct reset_control_ops ciena_gpio_reset_ops = {
	.assert		= ciena_gpio_reset_assert,
	.deassert	= ciena_gpio_reset_deassert,
	.status		= ciena_gpio_reset_status,
	.reset		= ciena_gpio_reset_reset
};

static int ciena_gpio_of_next(struct platform_device *pdev,
			      struct device_node *node,
			      struct ciena_gpio_reset_data *data)
{
	char label[sizeof("very_long_gpio_reset_device_name#65535")];
	enum gpiod_flags flags = GPIOD_OUT_HIGH;
	int index = data->gpio_count;
	struct gpio_desc *gd;
	int rc;

	if (data->negative) flags = GPIOD_OUT_LOW;

	snprintf(label, sizeof(label), "%s#%d", dev_name(&pdev->dev), index);
	label[sizeof(label) - 1] = '\0';

	gd = devm_fwnode_gpiod_get_index(&pdev->dev, of_fwnode_handle(node), "reset",
					 index, flags, label);

	if (NULL == gd) return -EPROBE_DEFER;

	if (IS_ERR(gd)) {
		rc = PTR_ERR(gd);
		if (-ENOENT != rc) {
			dev_err(&pdev->dev, "%s gpio index %d failed (%d)\n",
				node->full_name, index, rc);
			return rc;
		}
		/* ENOENT means the gpio list is exhausted */
		if (0 == index) return -ENOENT;
		data->gpiod = devm_kmalloc_array(&pdev->dev, data->gpio_count,
						 sizeof(gd), GFP_KERNEL);
		if (NULL == data->gpiod) return -ENOMEM;
		return 0;
	}

	data->gpio_count++;
	rc = ciena_gpio_of_next(pdev, node, data);
	if (!rc) data->gpiod[index] = gd;

	return rc;
}

static int ciena_gpio_reset_parse_of(struct platform_device *pdev,
				     struct device_node *node,
				     struct ciena_gpio_reset_data *data)
{
	struct ciena_sysfs_reset_names reset_names;
	const u32 *prop;
	int max = 0;
	int rc;

	if (!node) return 0;

	prop = of_get_property(node, "ciena,negative-logic", NULL);
	if (prop) data->negative = true;

	rc = ciena_gpio_of_next(pdev, node, data);

	if (!rc && data->gpio_count)
		max = (int) ciena_sysfs_of_reset_names(node, &reset_names);

	if (max) {
		if (data->gpio_count != max) {
			dev_warn(&pdev->dev, "device tree expects %d resets, "
				 "%d pins found\n", max, data->gpio_count);
			max = data->gpio_count;
		}

		data->names = devm_kmalloc_array(&pdev->dev, max,
						 sizeof(*data->names),
						 GFP_KERNEL);
		if (NULL == data->names) {
			dev_err(&pdev->dev, "no memory for %d names\n", max);
			return -ENOMEM;
		}

		while (max--) {
			if (RESET_SYSFS_MAX_NAMES <= max)
				data->names[max] = NULL;
			else
				data->names[max] = reset_names.nm[max];
		}
	}

	return rc;
}

static int ciena_gpio_reset_probe(struct platform_device *pdev)
{
	struct ciena_gpio_reset_data *data;
	struct device_node *node = pdev->dev.of_node;
	int rc = -ENODEV;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) return -ENOMEM;

	if (node) {
		if (!of_find_property(node, "#reset-cells", NULL)) {
			dev_err(&pdev->dev, "%s no #reset-cells property\n",
				node->full_name);
			return -EINVAL;
		}
		rc = ciena_gpio_reset_parse_of(pdev, node, data);
	}
	if (rc) return rc;

	data->rcdev.owner = THIS_MODULE;
	data->rcdev.ops = &ciena_gpio_reset_ops;
	data->rcdev.dev = &pdev->dev;
	data->rcdev.of_node = node;
	data->rcdev.nr_resets = data->gpio_count;

	rc = reset_controller_register(&data->rcdev);
	if (rc) dev_err(&pdev->dev, "%s: registration failed (%d)\n",
			__func__, rc);
	else dev_set_drvdata(&pdev->dev, data);

	return rc;
}

static void ciena_gpio_reset_remove(struct platform_device *pdev)
{
	struct ciena_gpio_reset_data *data = platform_get_drvdata(pdev);
	reset_controller_unregister(&data->rcdev);
}


static const struct of_device_id ciena_gpio_reset_dt_ids[] = {
	{
		.compatible = "ciena,gpio-reset",
		.type       = "reset-controller",
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ciena_gpio_reset_dt_ids);

static struct platform_driver ciena_gpio_reset_driver = {
	.probe	  = ciena_gpio_reset_probe,
	.remove	  = ciena_gpio_reset_remove,
	.driver   = {
		.name		= RESET_GPIO_DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= ciena_gpio_reset_dt_ids,
	},
};
module_platform_driver(ciena_gpio_reset_driver);

MODULE_AUTHOR("Marc St-Amand");
MODULE_DESCRIPTION("Ciena GPIO Reset Controller Driver");
MODULE_LICENSE("GPL");
