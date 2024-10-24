/*
 * Watchdog driver for the Midstone 200i
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


#define DRIVER_NAME "ms200i_wdt"

#define CONF_GPIOBASE      0x8000F848
#define PCI_CONFIG_ADDRESS 0x0CF8
#define PCI_CONFIG_DATA    0x0CFC

// WDT_CTRL is GPIO 32
// GPIOBUS: 2 ,bit: 0th
#define WDT_CTRL_ENB_REG   0x30
#define WDT_CTRL_DIR_REG   0x34
#define WDT_CTRL_LVL_REG   0x38
#define WDT_CTRL_MASK      (unsigned long)(1<<(32%32))

// WDT_FEED is GPIO 15
// GPIOBUS: 1 ,bit: 15th
#define WDT_FEED_ENB_REG   0x00
#define WDT_FEED_DIR_REG   0x04
#define WDT_FEED_LVL_REG   0x0C
#define WDT_FEED_MASK      (unsigned long)(1<<(15%32))

static bool nowayout = WATCHDOG_NOWAYOUT;

// For enabling Debug message
//#define ENAB_DEBUG
//#define ENAB_DEBUG_GPIO

struct ms200i_wdt_drvdata {
    struct watchdog_device wdt;
    struct mutex lock;
    unsigned int gpiobase;
};

static struct resource ms200i_wdt_resources[] = {
        {
                .flags  = IORESOURCE_IO,
        },
};

static void ms200i_wdt_dev_release( struct device * dev)
{
        return;
}

static struct platform_device ms200i_wdt_dev = {
        .name           = DRIVER_NAME,
        .id             = -1,
        .num_resources  = ARRAY_SIZE(ms200i_wdt_resources),
        .resource       = ms200i_wdt_resources,
        .dev = {
            .release = ms200i_wdt_dev_release,
        }
};

static int ms200i_wdt_start(struct watchdog_device *wdt_dev)
{
        struct ms200i_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
        unsigned char reset_ctrl = 0x00;
        unsigned long enab, gpio ,dir;
        unsigned int base_addr;
#ifdef ENAB_DEBUG
        printk(KERN_INFO "WDT Start");
#endif
        mutex_lock(&drvdata->lock);

        base_addr = drvdata->gpiobase;

        enab = inl(base_addr + WDT_CTRL_ENB_REG);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "ctrl enab %16X",enab);
#endif
        enab |= WDT_CTRL_MASK;
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "ctrl enab %16X",enab);
#endif
        outl(enab, base_addr + WDT_CTRL_ENB_REG);
#ifdef ENAB_DEBUG_GPIO
        enab = inl(base_addr + WDT_CTRL_ENB_REG);
        printk(KERN_INFO "ctrl enab %16X",enab);
#endif

        enab = inl(base_addr + WDT_FEED_ENB_REG);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed enab %16X",enab);
#endif
        enab |= WDT_FEED_MASK;
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed enab %16X",enab);
#endif
        outl(enab, base_addr + WDT_FEED_ENB_REG);
#ifdef ENAB_DEBUG_GPIO
        enab = inl(base_addr + WDT_FEED_ENB_REG);
        printk(KERN_INFO "feed enab %16X",enab);
#endif

        dir = inl(base_addr + WDT_CTRL_DIR_REG);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "ctrl dir %16X",dir);
#endif
        dir &= ~WDT_FEED_MASK;
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "ctrl dir %16X",dir);
#endif
        outl(dir, base_addr + WDT_CTRL_DIR_REG);
#ifdef ENAB_DEBUG_GPIO
        dir = inl(base_addr + WDT_CTRL_DIR_REG);
        printk(KERN_INFO "ctrl dir %16X",dir);
#endif

        dir = inl(base_addr + WDT_FEED_DIR_REG);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed dir %16X",dir);
#endif
        dir &= ~WDT_FEED_MASK;
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed dir %16X",dir);
#endif
        outl(dir, base_addr + WDT_FEED_DIR_REG);
#ifdef ENAB_DEBUG_GPIO
        dir = inl(base_addr + WDT_FEED_DIR_REG);
        printk(KERN_INFO "feed dir %16X",dir);
#endif

        gpio = inl(base_addr + WDT_CTRL_LVL_REG);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "ctrl gpio %16X",gpio);
#endif
        gpio &= ~WDT_CTRL_MASK;
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "ctrl gpio %16X",gpio);
#endif
        outl_p( gpio, base_addr + WDT_CTRL_LVL_REG);
#ifdef ENAB_DEBUG_GPIO
        gpio = inl(base_addr + WDT_CTRL_LVL_REG);
        printk(KERN_INFO "ctrl gpio %16X",gpio);
#endif

        mutex_unlock(&drvdata->lock);
        printk(KERN_INFO "WDT Start Finish");
        return 0;
}

static int ms200i_wdt_stop(struct watchdog_device *wdt_dev)
{
        struct ms200i_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
        unsigned long gpio;
        unsigned long base_addr;
#ifdef ENAB_DEBUG
        printk(KERN_INFO "WDT Stop");
#endif
        mutex_lock(&drvdata->lock);

        base_addr = drvdata->gpiobase;

        gpio = inl(base_addr + WDT_CTRL_ENB_REG);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "ctrl gpio %16X",gpio);
#endif
        gpio &= ~(WDT_CTRL_MASK);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "ctrl gpio %16X",gpio);
#endif
        outl_p(gpio, base_addr + WDT_CTRL_ENB_REG);
#ifdef ENAB_DEBUG_GPIO
        gpio = inl(base_addr + WDT_CTRL_ENB_REG);
        printk(KERN_INFO "ctrl gpio %16X",gpio);
#endif

        gpio = inl(base_addr + WDT_FEED_ENB_REG);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed gpio %16X",gpio);
#endif
        gpio &= ~(WDT_CTRL_MASK);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed gpio %16X",gpio);
#endif
        outl_p(gpio, base_addr + WDT_FEED_ENB_REG);
#ifdef ENAB_DEBUG_GPIO
        gpio = inl(base_addr + WDT_FEED_ENB_REG);
        printk(KERN_INFO "feed gpio %16X",gpio);
#endif

        mutex_unlock(&drvdata->lock);

        return 0;
}

static int ms200i_wdt_ping(struct watchdog_device *wdt_dev)
{
        struct ms200i_wdt_drvdata *drvdata = watchdog_get_drvdata(wdt_dev);
        unsigned long gpio;
        unsigned long base_addr;

#ifdef ENAB_DEBUG
        printk(KERN_INFO "WDT PING");
#endif
        mutex_lock(&drvdata->lock);

        base_addr = drvdata->gpiobase;

        gpio = inl(base_addr + WDT_FEED_LVL_REG);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed gpio %16X",gpio);
#endif
        gpio &= ~WDT_FEED_MASK;
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed gpio %16X",gpio);
#endif
        outl_p( gpio, base_addr + WDT_FEED_LVL_REG);
#ifdef ENAB_DEBUG_GPIO
        gpio = inl(base_addr + WDT_FEED_LVL_REG);
        printk(KERN_INFO "feed gpio %16X",gpio);
#endif
        mdelay(10);

        gpio = inl(base_addr + WDT_FEED_LVL_REG);
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed gpio %16X",gpio);
#endif
        gpio |= WDT_FEED_MASK;
#ifdef ENAB_DEBUG_GPIO
        printk(KERN_INFO "feed gpio %16X",gpio);
#endif
        outl_p( gpio, base_addr + WDT_FEED_LVL_REG);
#ifdef ENAB_DEBUG_GPIO
        gpio = inl(base_addr + WDT_FEED_LVL_REG);
        printk(KERN_INFO "feed gpio %16X",gpio);
#endif

        mutex_unlock(&drvdata->lock);
#ifdef ENAB_DEBUG
        printk(KERN_INFO "WDT PING FINISH");
#endif
        return 0;
}

static const struct watchdog_info ms200i_wdt_info = {
        .options = WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
        .identity = "MS200i Watchdog",
};

static const struct watchdog_ops ms200i_wdt_ops = {
        .owner = THIS_MODULE,
        .start = ms200i_wdt_start,
        .stop = ms200i_wdt_stop,
        .ping = ms200i_wdt_ping,
};

static int ms200i_wdt_probe(struct platform_device *pdev)
{
        struct ms200i_wdt_drvdata *drvdata;
        int ret;
        unsigned int base_addr;

        drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata),
                                   GFP_KERNEL);
        if (!drvdata) {
                ret = -ENOMEM;
                goto err;
        }

        outl(CONF_GPIOBASE, PCI_CONFIG_ADDRESS);
        base_addr = inl(PCI_CONFIG_DATA);
        // remove last bit , that is hardware inditcate.
        base_addr &= ~(0x01);

        if(base_addr == 0){
            printk(KERN_INFO "can't get gpio base address");
        }else{
            printk(KERN_INFO "gpio base address : %8.8X",base_addr);
        }

        mutex_init(&drvdata->lock);

        drvdata->gpiobase = base_addr;
        drvdata->wdt.info = &ms200i_wdt_info;
        drvdata->wdt.ops = &ms200i_wdt_ops;

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

static int ms200i_wdt_remove(struct platform_device *pdev)
{
        struct ms200i_wdt_drvdata *drvdata = platform_get_drvdata(pdev);

        watchdog_unregister_device(&drvdata->wdt);
#ifdef ENAB_DEBUG
        printk(KERN_INFO "MS200i WDT Remove");
#endif
        return 0;
}

static struct platform_driver ms200i_wdt_drv = {
        .probe = ms200i_wdt_probe,
        .remove = ms200i_wdt_remove,
        .driver = {
                .name = DRIVER_NAME,
        },
};

int ms200i_wdt_init(void)
{
#ifdef ENAB_DEBUG
        printk(KERN_INFO "MS200i WDT Init");
#endif
        platform_device_register(&ms200i_wdt_dev);
        platform_driver_register(&ms200i_wdt_drv);

        return 0;
}

void ms200i_wdt_exit(void)
{
#ifdef ENAB_DEBUG
        printk(KERN_INFO "MS200i WDT Exit");
#endif
        platform_driver_unregister(&ms200i_wdt_drv);
        platform_device_unregister(&ms200i_wdt_dev);
}

module_init(ms200i_wdt_init);
module_exit(ms200i_wdt_exit);

MODULE_AUTHOR("Sittisak Sinprem <ssinprem@celestica.com>");
MODULE_DESCRIPTION("Midstone ms200i Watchdog");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ms200i-watchdog");
