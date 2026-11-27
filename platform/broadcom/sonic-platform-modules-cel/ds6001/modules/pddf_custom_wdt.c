/*
 * Copyright (C) 2026 Celestica Inc.
 *
 * pddf_wdt.c - Watchdog driver for the Celestica DS6000 platform,
 * based on the Linux watchdog framework.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/ioport.h>
#include <linux/jiffies.h>

#define DRV_NAME "cpld_wdt"
#define DRV_VERSION "2.0"

/* CPLD Register Definitions */
#define REBOOT_CAUSE_REG 0xA07
#define WDT_SET_TIMER_H_BIT_REG 0xA81
#define WDT_SET_TIMER_M_BIT_REG 0xA82
#define WDT_SET_TIMER_L_BIT_REG 0xA83
#define WDT_TIMER_H_BIT_REG 0xA84
#define WDT_TIMER_M_BIT_REG 0xA85
#define WDT_TIMER_L_BIT_REG 0xA86
#define WDT_ENABLE_REG 0xA87
#define WDT_PUNCH_REG 0xA89

/* Register Values */
#define WDT_ENABLE 0x01
#define WDT_DISABLE 0x00
#define WDT_PUNCH 0x00
#define WDT_RESET_REASON 0x66

#define MAX_TIMEOUT_MS 0xFFFFFF /* 24-bit value in milliseconds */
#define DEFAULT_TIMEOUT_S 180

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started. (default:" __stringify(WATCHDOG_NOWAYOUT) ")");

static int timeout = DEFAULT_TIMEOUT_S;
module_param(timeout, int, 0);
MODULE_PARM_DESC(timeout, "Watchdog timeout in seconds. (default: 180)");

struct pddf_wdt_data {
	struct watchdog_device wdd;
	struct mutex lock; /* for register access */
	struct resource *res;
	u8 last_reboot_reason;
};

static int pddf_wdt_ping(struct watchdog_device *wdd)
{
	struct pddf_wdt_data *pdata = watchdog_get_drvdata(wdd);

	mutex_lock(&pdata->lock);
	outb(WDT_PUNCH, WDT_PUNCH_REG);
	mutex_unlock(&pdata->lock);

	return 0;
}

static int pddf_wdt_set_timeout(struct watchdog_device *wdd, unsigned int new_timeout)
{
	struct pddf_wdt_data *pdata = watchdog_get_drvdata(wdd);
	unsigned long timeout_ms = new_timeout * 1000;

	if (timeout_ms > MAX_TIMEOUT_MS)
		timeout_ms = MAX_TIMEOUT_MS;

	mutex_lock(&pdata->lock);
	outb((timeout_ms >> 16) & 0xFF, WDT_SET_TIMER_H_BIT_REG);
	outb((timeout_ms >> 8) & 0xFF, WDT_SET_TIMER_M_BIT_REG);
	outb(timeout_ms & 0xFF, WDT_SET_TIMER_L_BIT_REG);
	mutex_unlock(&pdata->lock);

	wdd->timeout = timeout_ms / 1000;
	return 0;
}

static int pddf_wdt_start(struct watchdog_device *wdd)
{
	struct pddf_wdt_data *pdata = watchdog_get_drvdata(wdd);

	mutex_lock(&pdata->lock);
	outb(WDT_ENABLE, WDT_ENABLE_REG);
	outb(WDT_PUNCH, WDT_PUNCH_REG); /* Pet it right away */
	mutex_unlock(&pdata->lock);

	return 0;
}

static int pddf_wdt_stop(struct watchdog_device *wdd)
{
	struct pddf_wdt_data *pdata = watchdog_get_drvdata(wdd);

	mutex_lock(&pdata->lock);
	outb(WDT_DISABLE, WDT_ENABLE_REG);
	mutex_unlock(&pdata->lock);

	return 0;
}

static unsigned int pddf_wdt_get_timeleft(struct watchdog_device *wdd)
{
	struct pddf_wdt_data *pdata = watchdog_get_drvdata(wdd);
	unsigned int timeleft_ms;

	mutex_lock(&pdata->lock);
	timeleft_ms = inb(WDT_TIMER_H_BIT_REG);
	timeleft_ms = (timeleft_ms << 8) | inb(WDT_TIMER_M_BIT_REG);
	timeleft_ms = (timeleft_ms << 8) | inb(WDT_TIMER_L_BIT_REG);
	mutex_unlock(&pdata->lock);

	return timeleft_ms / 1000;
}

static const struct watchdog_info pddf_wdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = DRV_NAME,
};

static const struct watchdog_ops pddf_wdt_ops = {
	.owner = THIS_MODULE,
	.start = pddf_wdt_start,
	.stop = pddf_wdt_stop,
	.ping = pddf_wdt_ping,
	.set_timeout = pddf_wdt_set_timeout,
	.get_timeleft = pddf_wdt_get_timeleft,
};

static ssize_t bootreason_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct watchdog_device *wdd = dev_get_drvdata(dev);
	struct pddf_wdt_data *pdata = watchdog_get_drvdata(wdd);

	return scnprintf(buf, PAGE_SIZE, "0x%02x\n", pdata->last_reboot_reason);
}

static DEVICE_ATTR_RO(bootreason);

static ssize_t settimeout_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct watchdog_device *wdd = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "%u\n", wdd->timeout);
}

static ssize_t settimeout_store(struct device *dev,
				 struct device_attribute *attr, const char *buf, size_t count)
{
	struct watchdog_device *wdd = dev_get_drvdata(dev);
	unsigned int val;
	int err;

	err = kstrtouint(buf, 0, &val);
	if (err)
		return err;

	/* The watchdog core checks this, but we do it here for safety */
	if (val < wdd->min_timeout || val > wdd->max_timeout)
		return -EINVAL;

	pddf_wdt_set_timeout(wdd, val);

	return count;
}

static DEVICE_ATTR_RW(settimeout);

static struct attribute *pddf_wdt_attrs[] = {
	&dev_attr_bootreason.attr,
	&dev_attr_settimeout.attr,
	NULL,
};

static const struct attribute_group pddf_wdt_attr_group = {
	.attrs = pddf_wdt_attrs,
};

static const struct attribute_group *pddf_wdt_groups[] = {
	&pddf_wdt_attr_group,
	NULL
};

static int pddf_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pddf_wdt_data *pdata;
	struct watchdog_device *wdd;
	int ret;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!pdata->res) {
		dev_err(dev, "failed to get I/O resource\n");
		return -ENODEV;
	}

	if (!devm_request_region(dev, pdata->res->start, resource_size(pdata->res), pdev->name)) {
		dev_err(dev, "failed to request I/O region\n");
		return -EBUSY;
	}

	mutex_init(&pdata->lock);
	platform_set_drvdata(pdev, pdata);

	wdd = &pdata->wdd;
	wdd->info = &pddf_wdt_info;
	wdd->ops = &pddf_wdt_ops;
	wdd->min_timeout = 1;
	wdd->max_timeout = MAX_TIMEOUT_MS / 1000;
	wdd->parent = dev;
	wdd->timeout = clamp_val(timeout, wdd->min_timeout, wdd->max_timeout);
	wdd->groups = pddf_wdt_groups;
	watchdog_set_drvdata(wdd, pdata);
	watchdog_set_nowayout(wdd, nowayout);

	pdata->last_reboot_reason = inb(REBOOT_CAUSE_REG);
	if (pdata->last_reboot_reason == WDT_RESET_REASON)
		wdd->bootstatus = WDIOF_CARDRESET;

	pddf_wdt_set_timeout(wdd, wdd->timeout);

	ret = devm_watchdog_register_device(dev, wdd);
	if (ret) {
		dev_err(dev, "failed to register watchdog device\n");
		return ret;
	}

	dev_info(dev, "initialized (timeout=%ds, nowayout=%d)\n", wdd->timeout, nowayout);

	return 0;
}

static void pddf_wdt_remove(struct platform_device *pdev)
{
	/* devm-managed resources will be cleaned up automatically */
	return;
}

static struct platform_driver pddf_wdt_driver = {
	.probe = pddf_wdt_probe,
	.remove = pddf_wdt_remove,
	.driver = {
		.name = DRV_NAME,
	},
};

static struct resource pddf_wdt_resources[] = {
	{
		.start = 0xA00,
		.end = 0xA89,
		.flags = IORESOURCE_IO,
		.name = DRV_NAME,
	},
};

static void pddf_wdt_dev_release(struct device *dev)
{
	/* Statically allocated device, nothing to do. */
}

static struct platform_device pddf_wdt_device = {
	.name = DRV_NAME,
	.id = -1,
	.num_resources = ARRAY_SIZE(pddf_wdt_resources),
	.resource = pddf_wdt_resources,
	.dev = {
		.release = pddf_wdt_dev_release,
	},
};

static int __init pddf_wdt_init_module(void)
{
	int ret;

	pr_info("CPLD Watchdog Timer Driver v%s\n", DRV_VERSION);

	ret = platform_device_register(&pddf_wdt_device);
	if (ret)
		return ret;

	ret = platform_driver_register(&pddf_wdt_driver);
	if (ret)
		platform_device_unregister(&pddf_wdt_device);

	return ret;
}

static void __exit pddf_wdt_cleanup_module(void)
{
	platform_driver_unregister(&pddf_wdt_driver);
	platform_device_unregister(&pddf_wdt_device);
	pr_info("Watchdog Module Unloaded\n");
}

module_init(pddf_wdt_init_module);
module_exit(pddf_wdt_cleanup_module);

MODULE_DESCRIPTION("CPLD Watchdog Driver for PDDF");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");
