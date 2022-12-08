/*
/* drivers/char/watchdog/ctc-wdt.c
 *
 * Watchdog driver for CTC TSINGMA, based on ARM SP805 watchdog module
 *
 * Copyright (C) 2010 ST Microelectronics
 * Viresh Kumar <vireshk@kernel.org>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2 or later. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/device.h>
#include <linux/resource.h>
#include <linux/amba/bus.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include "../include/sysctl.h"
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

/* default timeout in seconds */
#define DEFAULT_TIMEOUT		60

#define MODULE_NAME		"ctc-wdt"

/* watchdog register offsets and masks */
#define WDTLOAD			0x000
#define LOAD_MIN	0x00000001
#define LOAD_MAX	0xFFFFFFFF
#define WDTVALUE		0x004
#define WDTCONTROL		0x008
	/* control register masks */
#define	INT_ENABLE	(1 << 0)
#define	RESET_ENABLE	(1 << 1)
#define WDTINTCLR		0x00C
#define WDTRIS			0x010
#define WDTMIS			0x014
#define INT_MASK	(1 << 0)
#define WDTLOCK			0xC00
#define	UNLOCK		0x1ACCE551
#define	LOCK		0x00000001

/* TsingMa SoC  */
#define WDTCLK_MAX		500000000UL

/**
 * struct ctc_wdt: ctc wdt device structure
 * @wdd: instance of struct watchdog_device
 * @lock: spin lock protecting dev structure and io access
 * @base: base address of wdt
 * @clk: clock structure of wdt
 * @adev: amba device structure of wdt
 * @status: current status of wdt
 * @load_val: load value to be set for current timeout
 */
struct ctc_wdt {
	struct watchdog_device wdd;
	spinlock_t lock;
	void __iomem *base;
	struct clk *clk;
	struct amba_device *adev;
	unsigned int load_val;
	struct regmap *regmap_base;
};

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
		 "Set to 1 to keep watchdog running after device release");

/* This routine finds load value that will reset system in required timeout */
static int wdt_setload(struct watchdog_device *wdd, unsigned int timeout)
{
	struct ctc_wdt *wdt = watchdog_get_drvdata(wdd);
	u64 load, rate;

	rate = clk_get_rate(wdt->clk);

	/*
	 * ctc wdt runs counter with given value twice, after the end of first
	 * counter it gives an interrupt and then starts counter again. If
	 * interrupt already occurred then it resets the system. This is why
	 * load is half of what should be required.
	 */
	load = div_u64(rate, 2) * timeout - 1;

	load = (load > LOAD_MAX) ? LOAD_MAX : load;
	load = (load < LOAD_MIN) ? LOAD_MIN : load;

	spin_lock(&wdt->lock);
	wdt->load_val = load;
	/* roundup timeout to closest positive integer value */
	wdd->timeout = div_u64((load + 1) * 2 + (rate / 2), rate);
	spin_unlock(&wdt->lock);

	return 0;
}

/* returns number of seconds left for reset to occur */
static unsigned int wdt_timeleft(struct watchdog_device *wdd)
{
	struct ctc_wdt *wdt = watchdog_get_drvdata(wdd);
	u64 load, rate;

	rate = clk_get_rate(wdt->clk);

	spin_lock(&wdt->lock);
	load = readl_relaxed(wdt->base + WDTVALUE);

	/*If the interrupt is inactive then time left is WDTValue + WDTLoad. */
	if (!(readl_relaxed(wdt->base + WDTRIS) & INT_MASK))
		load += wdt->load_val + 1;
	spin_unlock(&wdt->lock);

	return div_u64(load, rate);
}

static int wdt_config(struct watchdog_device *wdd, bool ping)
{
	struct ctc_wdt *wdt = watchdog_get_drvdata(wdd);
	int ret;

	if (!ping) {

		ret = clk_prepare_enable(wdt->clk);
		if (ret) {
			dev_err(&wdt->adev->dev, "clock enable fail");
			return ret;
		}
	}

	spin_lock(&wdt->lock);

	writel_relaxed(UNLOCK, wdt->base + WDTLOCK);
	writel_relaxed(wdt->load_val, wdt->base + WDTLOAD);
	writel_relaxed(INT_MASK, wdt->base + WDTINTCLR);

	if (!ping)
		writel_relaxed(INT_ENABLE | RESET_ENABLE, wdt->base +
			       WDTCONTROL);

	writel_relaxed(LOCK, wdt->base + WDTLOCK);

	/* Flush posted writes. */
	readl_relaxed(wdt->base + WDTLOCK);
	spin_unlock(&wdt->lock);

	return 0;
}

static int wdt_ping(struct watchdog_device *wdd)
{
	return wdt_config(wdd, true);
}

/* enables watchdog timers reset */
static int wdt_enable(struct watchdog_device *wdd)
{
	return wdt_config(wdd, false);
}

/* disables watchdog timers reset */
static int wdt_disable(struct watchdog_device *wdd)
{
	struct ctc_wdt *wdt = watchdog_get_drvdata(wdd);

	spin_lock(&wdt->lock);

	writel_relaxed(UNLOCK, wdt->base + WDTLOCK);
	writel_relaxed(0, wdt->base + WDTCONTROL);
	writel_relaxed(LOCK, wdt->base + WDTLOCK);

	/* Flush posted writes. */
	readl_relaxed(wdt->base + WDTLOCK);
	spin_unlock(&wdt->lock);

	clk_disable_unprepare(wdt->clk);

	return 0;
}

static int wdt_restart(struct watchdog_device *wdd, unsigned long action,
		       void *cmd)
{
	struct ctc_wdt *wdt = watchdog_get_drvdata(wdd);

	spin_lock(&wdt->lock);

	writel_relaxed(UNLOCK, wdt->base + WDTLOCK);
	writel_relaxed(0, wdt->base + WDTCONTROL);
	writel_relaxed(1, wdt->base + WDTLOAD);
	writel_relaxed(INT_ENABLE | RESET_ENABLE, wdt->base + WDTCONTROL);
	writel_relaxed(LOCK, wdt->base + WDTLOCK);

	spin_unlock(&wdt->lock);

	mdelay(100);

	return 0;
}

static int wdt_set_pretimeout(struct watchdog_device *wdd,
			      unsigned int new_pretimeout)
{
	struct ctc_wdt *wdt = watchdog_get_drvdata(wdd);
	u64 load, rate;

	rate = clk_get_rate(wdt->clk);

	load = rate * new_pretimeout - 1;
	load = (load > LOAD_MAX) ? LOAD_MAX : load;
	load = (load < LOAD_MIN) ? LOAD_MIN : load;

	spin_lock(&wdt->lock);
	wdt->load_val = load;
	/* roundup timeout to closest positive integer value */
	wdd->pretimeout = div_u64((load + 1) + (rate / 2), rate);

	spin_unlock(&wdt->lock);

	return 0;
}

static irqreturn_t ctc_wdt_irq(int irq, void *dev_id)
{
	struct ctc_wdt *wdt = dev_id;

	watchdog_notify_pretimeout(&wdt->wdd);

	return IRQ_HANDLED;
}

static const struct watchdog_info wdt_info = {
	.options =
	    WDIOF_MAGICCLOSE | WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING |
	    WDIOF_PRETIMEOUT,
	.identity = MODULE_NAME,
};

static const struct watchdog_ops wdt_ops = {
	.owner = THIS_MODULE,
	.start = wdt_enable,
	.stop = wdt_disable,
	.ping = wdt_ping,
	.set_timeout = wdt_setload,
	.get_timeleft = wdt_timeleft,
	.restart = wdt_restart,
	.set_pretimeout = wdt_set_pretimeout,
};

static int ctc_wdt_probe(struct amba_device *adev, const struct amba_id *id)
{
	struct ctc_wdt *wdt;
	int ret = 0;
	u64 rate;
	unsigned int fdc;

	wdt = devm_kzalloc(&adev->dev, sizeof(*wdt), GFP_KERNEL);
	if (!wdt) {
		ret = -ENOMEM;
		goto err;
	}

	wdt->base = devm_ioremap_resource(&adev->dev, &adev->res);
	if (IS_ERR(wdt->base))
		return PTR_ERR(wdt->base);

	wdt->clk = devm_clk_get(&adev->dev, NULL);
	if (IS_ERR(wdt->clk)) {
		dev_warn(&adev->dev, "Clock not found\n");
		ret = PTR_ERR(wdt->clk);
		goto err;
	}
	wdt->regmap_base = syscon_regmap_lookup_by_phandle(adev->dev.of_node,
							   "ctc,sysctrl");
	if (IS_ERR(wdt->regmap_base))
		return PTR_ERR(wdt->regmap_base);

	/* reset wdt module */
	regmap_write(wdt->regmap_base,
		     offsetof(struct SysCtl_regs, SysWdtResetCtl), 0x3);
	regmap_write(wdt->regmap_base,
		     offsetof(struct SysCtl_regs, SysWdtResetCtl), 0x0);

	/*
	 * TsingMa SoC wdt reference clock  is obtained by clockSub frequency
	 * division,which is 500Mhz.So we need to set the frequency division
	 * register according to the configured clock.
	 */

	rate = clk_get_rate(wdt->clk);
	if (rate < 0 || rate > WDTCLK_MAX) {
		dev_err(&adev->dev, "Clock out of range\n");
		goto err;
	}

	fdc = div_u64(WDTCLK_MAX, rate);
	regmap_write(wdt->regmap_base, offsetof(struct SysCtl_regs, SysWdt0Cnt),
		     fdc);
	regmap_write(wdt->regmap_base, offsetof(struct SysCtl_regs, SysWdt1Cnt),
		     fdc);
	wdt->adev = adev;
	wdt->wdd.info = &wdt_info;
	wdt->wdd.ops = &wdt_ops;
	wdt->wdd.parent = &adev->dev;

	spin_lock_init(&wdt->lock);
	watchdog_set_nowayout(&wdt->wdd, nowayout);
	watchdog_set_drvdata(&wdt->wdd, wdt);
	wdt_setload(&wdt->wdd, DEFAULT_TIMEOUT);
	ret = devm_request_irq(&adev->dev, adev->irq[0], ctc_wdt_irq,
			       0, "ctc-wdt", wdt);
	if (ret < 0) {
		dev_err(&adev->dev, "devm_request_irq() failed: %d\n", ret);
		goto err;
	}
	ret = watchdog_register_device(&wdt->wdd);
	if (ret) {
		dev_err(&adev->dev, "watchdog_register_device() failed: %d\n",
			ret);
		goto err;
	}
	amba_set_drvdata(adev, wdt);

	dev_info(&adev->dev, "registration successful\n");
	return 0;

err:
	dev_err(&adev->dev, "Probe Failed!!!\n");
	return ret;
}

static void ctc_wdt_remove(struct amba_device *adev)
{
	struct ctc_wdt *wdt = amba_get_drvdata(adev);

	watchdog_unregister_device(&wdt->wdd);
	watchdog_set_drvdata(&wdt->wdd, NULL);

	return;
}

static int __maybe_unused ctc_wdt_suspend(struct device *dev)
{
	struct ctc_wdt *wdt = dev_get_drvdata(dev);

	if (watchdog_active(&wdt->wdd))
		return wdt_disable(&wdt->wdd);

	return 0;
}

static int __maybe_unused ctc_wdt_resume(struct device *dev)
{
	struct ctc_wdt *wdt = dev_get_drvdata(dev);

	if (watchdog_active(&wdt->wdd))
		return wdt_enable(&wdt->wdd);

	return 0;
}

static SIMPLE_DEV_PM_OPS(ctc_wdt_dev_pm_ops, ctc_wdt_suspend, ctc_wdt_resume);

static struct amba_id ctc_wdt_ids[] = {
	/* Centec TsingMa SoC WDT ID */
	{
	 .id = 0x001bb824,
	 .mask = 0x00ffffff,
	 },
	{0, 0},
};

MODULE_DEVICE_TABLE(amba, ctc_wdt_ids);

static struct amba_driver ctc_wdt_driver = {
	.drv = {
		.name = MODULE_NAME,
		.pm = &ctc_wdt_dev_pm_ops,
		},
	.id_table = ctc_wdt_ids,
	.probe = ctc_wdt_probe,
	.remove = ctc_wdt_remove,
};

module_amba_driver(ctc_wdt_driver);

MODULE_AUTHOR("lius <lius@centecnetworks.com>");
MODULE_DESCRIPTION("ARM CTC Watchdog Driver");
MODULE_LICENSE("GPL");
