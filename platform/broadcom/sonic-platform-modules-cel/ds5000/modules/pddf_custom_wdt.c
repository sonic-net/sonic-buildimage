/***************************************************************************
 *   Copyright (C) 2021 Celestica  Corp                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <linux/sysfs.h>
#include <linux/hwmon-sysfs.h>
#include <linux/string.h>
#include <linux/delay.h>
	
#define WDT_SET_TIMER_H_BIT_REG      0xA181
#define WDT_SET_TIMER_M_BIT_REG      0xA182
#define WDT_SET_TIMER_L_BIT_REG      0xA183
#define WDT_TIMER_H_BIT_REG          0xA184
#define WDT_TIMER_M_BIT_REG          0xA185
#define WDT_TIMER_L_BIT_REG          0xA186
#define WDT_ENABLE_REG               0xA187
#define WDT_FEED_REG                 0xA188
#define WDT_PUNCH_REG                0xA189
#define WDT_START_FEED               0x01
#define WDT_STOP_FEED                0x00

#define MAX_TIMER_VALUE              (0xffffff / 1000)
#define DEFUALT_TIMER_VALUE          180    /* 180s */
#define WDT_ENABLE                   0x01
#define WDT_DISABLE                  0x00
#define WDT_RESTART                  0x00
#define DRV_NAME 	                 "cpld_wdt"
#define DRV_VERSION	                 "1.0.0"

static int simulate = 0;
module_param(simulate, int , S_IRUGO);

/*
 * struct watchdog_core_data - watchdog core internal data
 * @dev:	The watchdog's internal device
 * @cdev:	The watchdog's Character device.
 * @wdd:	Pointer to watchdog device.
 * @lock:	Lock for watchdog core.
 * @status:	Watchdog core internal status bits.
 */
struct watchdog_core_data {
	struct device dev;
	struct cdev cdev;
};
		 
struct cpld_wdt {
	void __iomem           *regs;
	struct watchdog_device	wdd;
	struct mutex            lock;
	u32                     timeout;
	int                     simulate_en;
	int                     simulate_timeout;
	char                    sysfs_buf[100];
};

static const struct watchdog_info cpld_wdt_ident = {
	.options	= WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT |
			  WDIOF_MAGICCLOSE,
	.identity	= "Celestica CPLD Watchdog",
};

extern void cpld_b_lock(void);
extern void cpld_b_unlock(void);

#define to_cpld_wdt(wdd)	container_of(wdd, struct cpld_wdt, wdd)

static inline int cpld_wdt_is_enabled(struct cpld_wdt *cpld_wdt)
{
	bool is_running = true;

	if (!simulate){
		cpld_b_lock();
		is_running = inb(WDT_ENABLE_REG) & WDT_START_FEED;
		cpld_b_unlock();
	}else{
		is_running = cpld_wdt->simulate_en;
		pr_info("cpld_wdt_is_enabled: %d\n", cpld_wdt->simulate_en);
	}
	
	return is_running;	
}

static int cpld_wdt_ping(struct watchdog_device *wdd)
{
	struct cpld_wdt *cpld_wdt = to_cpld_wdt(wdd);

	if (!simulate){
		cpld_b_lock();
		outb(WDT_START_FEED, WDT_FEED_REG);
		msleep(100);		
		outb(WDT_STOP_FEED, WDT_FEED_REG);
		cpld_b_unlock();
	}else{
		cpld_wdt->simulate_timeout += 1;
		pr_info("cpld_wdt_ping\n");
	}
	
	return 0;
}

static int cpld_wdt_set_timeout(struct watchdog_device *wdd, unsigned int timeout)
{
	unsigned int val;
	struct cpld_wdt *cpld_wdt = to_cpld_wdt(wdd);

	mutex_lock(&cpld_wdt->lock);
	
	cpld_wdt->timeout = timeout;
	if (cpld_wdt->timeout >  MAX_TIMER_VALUE)
		cpld_wdt->timeout = MAX_TIMER_VALUE;

	if (cpld_wdt->timeout <= 0)
		cpld_wdt->timeout = DEFUALT_TIMER_VALUE;
		
	if (!simulate){
		val = cpld_wdt->timeout * 1000;
		cpld_b_lock();
		outb((val >> 16) & 0xff, WDT_SET_TIMER_H_BIT_REG);
		outb((val >> 8) & 0xff, WDT_SET_TIMER_M_BIT_REG);
		outb(val & 0xff, WDT_SET_TIMER_L_BIT_REG);
		outb(WDT_RESTART, WDT_PUNCH_REG);
		cpld_b_unlock();
	}else{
		cpld_wdt->simulate_timeout = cpld_wdt->timeout;
		pr_info("cpld_wdt_set_timeout %d\n", cpld_wdt->timeout);
	}

	mutex_unlock(&cpld_wdt->lock);
	return 0;
}

static int cpld_wdt_start(struct watchdog_device *wdd)
{
	struct cpld_wdt *cpld_wdt = to_cpld_wdt(wdd);

	if (!simulate){
		cpld_b_lock();
		outb(WDT_ENABLE, WDT_ENABLE_REG);
		outb(WDT_RESTART, WDT_PUNCH_REG);
		cpld_b_unlock();
	}else{
		cpld_wdt->simulate_en = 1;
		pr_info("cpld_wdt_start\n");
	}
	
	return 0;
}

static int cpld_wdt_stop(struct watchdog_device *wdd)
{
	struct cpld_wdt *cpld_wdt = to_cpld_wdt(wdd);

	if (!simulate){
		cpld_b_lock();
		outb(WDT_DISABLE, WDT_ENABLE_REG);
		cpld_b_unlock();
	}else{
		cpld_wdt->simulate_en = 0;
		pr_info("cpld_wdt_stop\n");
	}
	
	return 0;
}

static int cpld_wdt_restart(struct watchdog_device *wdd,
			  unsigned long action, void *data)
{
	struct cpld_wdt *cpld_wdt = to_cpld_wdt(wdd);

	if (!simulate){
		cpld_b_lock();
		outb(WDT_RESTART, WDT_PUNCH_REG);
		cpld_b_unlock();
	}else{
		cpld_wdt->simulate_timeout = DEFUALT_TIMER_VALUE;
		pr_info("cpld_wdt_restart\n");
	}
	
	return 0;
}

static unsigned int cpld_wdt_get_timeleft(struct watchdog_device *wdd)
{
	int time = 20000;
	struct cpld_wdt *cpld_wdt = to_cpld_wdt(wdd);
	
	if (!simulate){
		mutex_lock(&cpld_wdt->lock);
		cpld_b_lock();
		time = inb(WDT_TIMER_H_BIT_REG);
		time = time << 8 | inb(WDT_TIMER_M_BIT_REG);
		time = time << 8 | inb(WDT_TIMER_L_BIT_REG);
		time = time/1000;
		cpld_b_unlock();
		mutex_unlock(&cpld_wdt->lock);
	}else{
		time = cpld_wdt->simulate_timeout;
		if (cpld_wdt->simulate_timeout)
			cpld_wdt->simulate_timeout --;
		pr_info("cpld_wdt_get_timeleft\n");
	}
	
	return time;
}

static const struct watchdog_ops cpld_wdt_ops = {
	.owner        = THIS_MODULE,
	.start        = cpld_wdt_start,
	.stop         = cpld_wdt_stop,
	.ping         = cpld_wdt_ping,
	.set_timeout  = cpld_wdt_set_timeout,
	.get_timeleft = cpld_wdt_get_timeleft,
	.restart      = cpld_wdt_restart,
};

static ssize_t settimeout_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct cpld_wdt *cpld_wdt = dev_get_drvdata(dev);
	
	mutex_lock(&cpld_wdt->lock);
	ret = sprintf(buf,"%d\n", cpld_wdt->timeout);
	mutex_unlock(&cpld_wdt->lock);
    
	return ret;
}

static ssize_t settimeout_store(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t count)
{
	unsigned long timeout;
	struct cpld_wdt *cpld_wdt = dev_get_drvdata(dev);
    int ret = 0;
   
    ret = kstrtoul(buf, 0, &timeout);
	if (ret != 0) {
        return ret;
    }
	cpld_wdt_set_timeout(&cpld_wdt->wdd, (unsigned int)timeout);

    return count;
}

DEVICE_ATTR(settimeout, 0664, settimeout_show, settimeout_store);

static int cpld_wdt_probe(struct platform_device *pdev)
{
	struct watchdog_core_data *wd_data;
	struct device *dev = &pdev->dev;
	struct watchdog_device *wdd;
	struct cpld_wdt *cpld_wdt;
	int err;

	cpld_wdt = devm_kzalloc(dev, sizeof(*cpld_wdt), GFP_KERNEL);
	if (!cpld_wdt)
		return -ENOMEM;
	
	mutex_init(&(cpld_wdt->lock));

	wdd = &cpld_wdt->wdd;
	wdd->info = &cpld_wdt_ident;
	wdd->ops = &cpld_wdt_ops;
	wdd->min_timeout = 1;
	wdd->max_timeout = MAX_TIMER_VALUE;
	wdd->parent = dev;

	watchdog_set_drvdata(wdd, cpld_wdt);
	watchdog_init_timeout(wdd, 0, dev);

	if (simulate)
		pr_info("watchdog device is simulate\n");	
	/*
	 * If the watchdog is already running, use its already configured
	 * timeout. Otherwise use the default or the value provided through
	 * devicetree.
	 */
	if (cpld_wdt_is_enabled(cpld_wdt)) {
		wdd->timeout = cpld_wdt_get_timeleft(wdd);
		set_bit(WDOG_HW_RUNNING, &wdd->status);
	} else {
		wdd->timeout = DEFUALT_TIMER_VALUE;
		watchdog_init_timeout(wdd, DEFUALT_TIMER_VALUE, dev);
	}

	platform_set_drvdata(pdev, cpld_wdt);

	watchdog_set_restart_priority(wdd, 128);

	err = watchdog_register_device(wdd);
	if(err < 0)
		pr_info("watchdog_register_device Failed. err:%d\n", err);	

	wd_data = wdd->wd_data;
	err = device_create_file(&wd_data->dev, &dev_attr_settimeout);
	if(err < 0)
		pr_info("device_create_file Failed. err:%d\n", err);	
	
	return err;
}

static int cpld_wdt_remove(struct platform_device *pdev)
{
	struct cpld_wdt *cpld_wdt = platform_get_drvdata(pdev);

	watchdog_unregister_device(&cpld_wdt->wdd);

	return 0;
}

static struct platform_driver cpld_wdt_driver = {
        .probe          = cpld_wdt_probe,
        .remove         = cpld_wdt_remove,
        .driver         = {
                .name   = DRV_NAME,
        },
};

static struct resource cpld_wdt_resources[] = {
    {
        .start  = 0xA181,
        .end    = 0xA189,
        .flags  = IORESOURCE_IO,
    },
};

static void wdt_dev_release( struct device * dev)
{
    return;
}

static struct platform_device cpld_wdt_dev = {
    .name           = DRV_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(cpld_wdt_resources),
    .resource       = cpld_wdt_resources,
    .dev = {
        .release = wdt_dev_release,
    }
};

static int __init cpld_wdt_init_module(void)
{
	int err = 0;
     
	err = platform_device_register(&cpld_wdt_dev);
	err += platform_driver_register(&cpld_wdt_driver);
	if(err < 0)
		pr_info("Platform Device/Driver Register Failed. err:%d\n", err);

	pr_info("CPLD WatchDog Timer Driver v%s\n", DRV_VERSION);
	return err;
}

static void __exit cpld_wdt_cleanup_module(void)
{
	platform_driver_unregister(&cpld_wdt_driver);
	platform_device_unregister(&cpld_wdt_dev);
	cpld_b_lock();
	outb(WDT_DISABLE, WDT_ENABLE_REG);
	cpld_b_unlock();	
	pr_info("Watchdog Module Unloaded\n");
}

module_init(cpld_wdt_init_module);
module_exit(cpld_wdt_cleanup_module);

MODULE_DESCRIPTION("Cpld Watchdog Driver");
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Fan Xinghua <xingfan@celestica.com>");
MODULE_LICENSE("GPL");
