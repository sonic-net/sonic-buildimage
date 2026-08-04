/*
 * Juniper Networks TMC fpga LEDs driver
 *
 * Copyright (C) 2018 Juniper Networks
 * Author: Ciju Rajan K <crajank@juniper.net>
 *
 * This driver is based on I2CS fpga LEDs driver by Georgi Vlaev
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/io.h>

/* Max LEDs supported by this driver (2bits of control per LED in 32bit reg) */
#define NUM_LEDS	3

struct tmc_led {
	struct led_classdev lc;
	struct work_struct work;
	int on;
	int bit;
	void __iomem *addr;
};

struct tmc_led_data {
	int num_leds;
	struct tmc_led *leds;
};

struct led_table
{
	const char *name;
	int reg;
};

static struct led_table qfx5200_tmc_led_data[] = {
	{
		.name = "system",
		.reg = 0,
	},
	{
		.name = "beacon",
		.reg = 3,
	},
	{
		.name = "master",
		.reg = 5,
	}
};

static void jnx_tmc_leds_work(struct work_struct *work)
{
	struct tmc_led *led = container_of(work, struct tmc_led, work);
	u32 value = ~(-1);

	value = ioread32(led->addr);

	if (led->on) {
		if (!strncmp(led->lc.name, "beacon", 6)) {
			value &= ~BIT(led->bit + 1);
			value |= BIT(led->bit);
		} else {
			value |= BIT(led->bit) | BIT(led->bit + 1);
		}
	} else {
		value &= ~(BIT(led->bit) | BIT(led->bit + 1));
	}

	iowrite32(value, led->addr);
}

static void jnx_tmc_leds_brightness_set(struct led_classdev *lc,
				 enum led_brightness brightness)
{
	struct tmc_led *led = container_of(lc, struct tmc_led, lc);

	led->on = (brightness != LED_OFF);
	schedule_work(&led->work);
}

static int jnx_tmc_leds_init_one(struct device *dev,
				struct tmc_led_data *ild,
				int num, void __iomem *addr)
{
	struct tmc_led *led;
	int ret;

	led = &ild->leds[num];

	led->addr = addr;

	led->lc.name = qfx5200_tmc_led_data[num].name;
	led->bit = qfx5200_tmc_led_data[num].reg;
	led->lc.brightness_set = jnx_tmc_leds_brightness_set;


	ret = devm_led_classdev_register(dev, &led->lc);
	if (ret)
		return ret;

	INIT_WORK(&led->work, jnx_tmc_leds_work);

	return 0;
}

static int jnx_tmc_leds_init(struct device *dev, struct tmc_led_data *ild,
				struct resource *res)
{
	int ret, idx = 0;
	void __iomem *addr;

	if (!dev->parent) {
		dev_err(dev, "dev->parent is null\n");
		return -ENODEV;
	}

	addr = devm_ioremap(dev, res->start, resource_size(res));
	if (!addr) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}

	ild->num_leds = NUM_LEDS;
	ild->leds = devm_kzalloc(dev, sizeof(struct tmc_led) * NUM_LEDS,
					GFP_KERNEL);
	if (!ild->leds) {
		dev_err(dev, "LED allocation failed\n");
		return -ENOMEM;
	}

	for (idx=0; idx<NUM_LEDS; idx++) {
		ret = jnx_tmc_leds_init_one(dev, ild, idx, addr);
		if (ret)
			return ret;
	}

	return 0;
}

static int tmc_leds_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct tmc_led_data *ild;
	int ret;
	struct resource *res;

	ild = devm_kzalloc(dev, sizeof(*ild), GFP_KERNEL);
	if (!ild) {
		dev_err(dev, "ild allocation failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, ild);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "res allocation failed\n");
		return -ENODEV;
	}

	ret = jnx_tmc_leds_init(dev, ild, res);
	if (ret < 0)
		return ret;

	return 0;
}

static int tmc_leds_remove(struct platform_device *pdev)
{
	struct tmc_led_data *ild = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < ild->num_leds; i++) {
		devm_led_classdev_unregister(&pdev->dev, &ild->leds[i].lc);
		cancel_work_sync(&ild->leds[i].work);
	}

	if (ild) {
		if (ild->leds)
			devm_kfree(&pdev->dev, ild->leds);
		devm_kfree(&pdev->dev, ild);
	}

	return 0;
}

static struct platform_driver jnx_tmc_leds_driver = {
	.driver = {
		.name  = "leds-tmc",
		.owner  = THIS_MODULE,
	},
	.probe = tmc_leds_probe,
	.remove = tmc_leds_remove,
};

static int __init jnx_tmc_leds_driver_init(void)
{
	int ret = -1;

	ret = platform_driver_register(&jnx_tmc_leds_driver);
	
	return ret;

}

static void __exit jnx_tmc_leds_driver_exit(void)
{
	platform_driver_unregister(&jnx_tmc_leds_driver);
}

module_init(jnx_tmc_leds_driver_init);
module_exit(jnx_tmc_leds_driver_exit);

MODULE_DESCRIPTION("Juniper Networks TMC leds driver");
MODULE_AUTHOR("Ciju Rajan K <crajank@juniper.net>");
MODULE_LICENSE("GPL");
