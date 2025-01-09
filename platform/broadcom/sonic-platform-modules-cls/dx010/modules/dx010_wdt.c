/*
 * Watchdog driver for the Seastone DX010
 *
 * Copyright (C) 2017 Celestica Corp.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/delay.h>


#define DRIVER_NAME "dx010_wdt"

#define RESET_CTRL		0x102
#define WDT_MASK		0x04
#define WDI_GPIO_DIR	0x504
#define WDI_GPIO		0x508

static bool nowayout = WATCHDOG_NOWAYOUT;

struct dx010_wdt_drvdata {
	struct watchdog_device wdt;
	struct mutex lock;
};

static struct resource dx010_wdt_resources[] = {
        {
                .flags  = IORESOURCE_IO,
        },
};

static void dx010_wdt_dev_release( struct device * dev)
{
        return;
}

static struct platform_device dx010_wdt_dev = {
        .name           = DRIVER_NAME,
        .id             = -1,
        .num_resources  = ARRAY_SIZE(dx010_wdt_resources),
        .resource       = dx010_wdt_resources,
        .dev = {
                        .release = dx010_wdt_dev_release,
        }
};

static int dx010_wdt_start(struct watchdog_device *wdt_dev)
{
        struct dx010_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
        unsigned char reset_ctrl = 0x00;
        unsigned long gpio ,dir;

        mutex_lock(&drvdata->lock);

        gpio = inl(WDI_GPIO);
        gpio |= 1 << 15;
        outl(gpio, WDI_GPIO);

        outl((inl(WDI_GPIO_DIR) & (~(1 << 15))), WDI_GPIO_DIR);

        reset_ctrl = inb(RESET_CTRL);

        gpio = inl(WDI_GPIO);
        gpio &= ~(1 << 15);
        outl_p( gpio, WDI_GPIO );

        mdelay(10);

        gpio = inl(WDI_GPIO);
        gpio |= (1 << 15);
        outl_p( gpio, WDI_GPIO );

        reset_ctrl |= WDT_MASK;
        outb(reset_ctrl, RESET_CTRL);

        mutex_unlock(&drvdata->lock);

        return 0;
}

static int dx010_wdt_stop(struct watchdog_device *wdt_dev)
{
        struct dx010_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
        unsigned long reset_ctrl;

        mutex_lock(&drvdata->lock);

        reset_ctrl = inb(RESET_CTRL);
        reset_ctrl &= ~WDT_MASK;
        outb(reset_ctrl, RESET_CTRL);

        mutex_unlock(&drvdata->lock);

        return 0;
}

static int dx010_wdt_ping(struct watchdog_device *wdt_dev)
{
        struct dx010_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
        unsigned long gpio;

        mutex_lock(&drvdata->lock);

        gpio = inl(WDI_GPIO);
        gpio &= ~(1 << 15);
        outl_p( gpio, WDI_GPIO );

        mdelay(10);

        gpio = inl(WDI_GPIO);
        gpio |= (1 << 15);
        outl_p( gpio, WDI_GPIO );

        mutex_unlock(&drvdata->lock);

        return 0;
}

static const struct watchdog_info dx010_wdt_info = {
        .options = WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
        .identity = "DX010 Watchdog",
};

static const struct watchdog_ops dx010_wdt_ops = {
        .owner = THIS_MODULE,
        .start = dx010_wdt_start,
        .stop = dx010_wdt_stop,
        .ping = dx010_wdt_ping,
};

static int dx010_wdt_probe(struct platform_device *pdev)
{
        struct dx010_wdt_drvdata *drvdata;
        int ret;

        drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata),
                                   GFP_KERNEL);
        if (!drvdata) {
                ret = -ENOMEM;
                goto err;
        }

        mutex_init(&drvdata->lock);

        drvdata->wdt.info = &dx010_wdt_info;
        drvdata->wdt.ops = &dx010_wdt_ops;

        watchdog_set_nowayout(&drvdata->wdt, nowayout);
        watchdog_set_drvdata(&drvdata->wdt, drvdata);

        ret = watchdog_register_device(&drvdata->wdt);
        if (ret != 0) {
                dev_err(&pdev->dev, "watchdog_register_device() failed: %d\n",
                        ret);
                goto err;
        }

        platform_set_drvdata(pdev, drvdata);

err:
        return ret;
}

static int dx010_wdt_remove(struct platform_device *pdev)
{
        struct dx010_wdt_drvdata *drvdata = platform_get_drvdata(pdev);

        watchdog_unregister_device(&drvdata->wdt);

        return 0;
}

static struct platform_driver dx010_wdt_drv = {
        .probe = dx010_wdt_probe,
        .remove = dx010_wdt_remove,
        .driver = {
                .name = DRIVER_NAME,
        },
};

int dx010_wdt_init(void)
{
        platform_device_register(&dx010_wdt_dev);
        platform_driver_register(&dx010_wdt_drv);

        return 0;
}

void dx010_wdt_exit(void)
{
        platform_driver_unregister(&dx010_wdt_drv);
        platform_device_unregister(&dx010_wdt_dev);
}

module_init(dx010_wdt_init);
module_exit(dx010_wdt_exit);

MODULE_AUTHOR("Abhisit Sangjan <asang@celestica.com>");
MODULE_AUTHOR("Pariwat Leamsumran <pleamsum@celestica.com>");
MODULE_DESCRIPTION("Seastone DX010 Watchdog");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dx010-watchdog");
