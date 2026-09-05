/*
 * Copyright 2025 Nexthop Systems Inc.
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
 */

#include <asm-generic/errno-base.h>
#include <linux/gpio/driver.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>

#include "pddf_client_defs.h"
#include "pddf_multifpgapci_defs.h"
#include "pddf_multifpgapci_gpio_defs.h"

struct pddf_multifpgapci_gpio {
	struct pddf_multifpgapci_gpio_chip_pdata pdata;
	struct mutex lock;
	struct gpio_chip chip;
};

static int pddf_multifpgapci_gpio_get_direction(struct gpio_chip *chip,
						unsigned int offset)
{
	struct pddf_multifpgapci_gpio *gpio = gpiochip_get_data(chip);
	return gpio->pdata.chan_data[offset].direction;
}

static int pddf_multifpgapci_gpio_get(struct gpio_chip *chip,
				      unsigned int offset)
{
	int status = 0;
	struct pddf_multifpgapci_gpio *gpio = gpiochip_get_data(chip);
	struct pddf_multifpgapci_gpio_line_pdata line =
		gpio->pdata.chan_data[offset];
	int temp;

	mutex_lock(&gpio->lock);
	status = ptr_multifpgapci_readpci(gpio->pdata.fpga, line.offset, &temp);
	mutex_unlock(&gpio->lock);

	if (status < 0)
		return status;

	return !!(temp & BIT(line.bit));
}

static int pddf_multifpgapci_gpio_set_internal(struct gpio_chip *chip,
				       unsigned int offset, int value)
{
	int status = 0;
	struct pddf_multifpgapci_gpio *gpio = gpiochip_get_data(chip);
	struct pddf_multifpgapci_gpio_line_pdata line =
		gpio->pdata.chan_data[offset];
	int temp;

	mutex_lock(&gpio->lock);
	status = ptr_multifpgapci_readpci(gpio->pdata.fpga, line.offset, &temp);
	if (status)
		goto exit;
	temp &= ~BIT(line.bit);
	temp |= (!!value) << line.bit;
	status = ptr_multifpgapci_writepci(gpio->pdata.fpga, temp, line.offset);
exit:
	if (status)
		printk(KERN_ERR "%s: Error status = %d", __FUNCTION__, status);

	mutex_unlock(&gpio->lock);

	return status;
}

static void pddf_multifpgapci_gpio_set(struct gpio_chip *chip,
				       unsigned int offset, int value)
{
	pddf_multifpgapci_gpio_set_internal(chip, offset, value);
}

static int pddf_multifpgapci_gpio_direction_input(struct gpio_chip *chip,
						  unsigned offset)
{
	struct pddf_multifpgapci_gpio *gpio = gpiochip_get_data(chip);
	gpio->pdata.chan_data[offset].direction = GPIO_LINE_DIRECTION_IN;
	return 0;
}

static int pddf_multifpgapci_gpio_direction_output(struct gpio_chip *chip,
						   unsigned offset, int value)
{
	struct pddf_multifpgapci_gpio *gpio = gpiochip_get_data(chip);
	gpio->pdata.chan_data[offset].direction = GPIO_LINE_DIRECTION_OUT;
	return pddf_multifpgapci_gpio_set_internal(chip, offset, value);
}

static const struct gpio_chip template_chip = {
	.label = "pddf-multifpgapci-gpio",
	.owner = THIS_MODULE,
	.direction_input = pddf_multifpgapci_gpio_direction_input,
	.direction_output = pddf_multifpgapci_gpio_direction_output,
	.get_direction = pddf_multifpgapci_gpio_get_direction,
	.get = pddf_multifpgapci_gpio_get,
	.set = pddf_multifpgapci_gpio_set,
	.base = -1,
};

static int pddf_multifpgapci_gpio_probe(struct platform_device *pdev)
{
	struct pddf_multifpgapci_gpio *gpio;
	struct pddf_multifpgapci_gpio_chip_pdata *pdata =
		dev_get_platdata(&pdev->dev);

	gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;

	gpio->pdata = *pdata;
	mutex_init(&gpio->lock);

	gpio->chip = template_chip;
	gpio->chip.parent = pdev->dev.parent;
	gpio->chip.ngpio = pdata->ngpio;

	return devm_gpiochip_add_data(&pdev->dev, &gpio->chip, gpio);
}

static struct platform_driver pddf_multifpgapci_gpio_driver = {
	.driver = {
		.name = "pddf-multifpgapci-gpio",
	},
	.probe = pddf_multifpgapci_gpio_probe,
};
module_platform_driver(pddf_multifpgapci_gpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION("PDDF Driver for Multiple PCI FPGA GPIOs.");
