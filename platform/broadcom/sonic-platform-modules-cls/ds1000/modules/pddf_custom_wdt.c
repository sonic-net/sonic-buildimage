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

#define WDT_CONTROL_BASE             0xA100
#define TEST_SCRATCH_REG             0xA101
#define REBOOT_CAUSE_REG             0xA105
#define WDT_SET_TIMER_H_BIT_REG      0xA161
#define WDT_SET_TIMER_M_BIT_REG      0xA162
#define WDT_SET_TIMER_L_BIT_REG      0xA163
#define WDT_TIMER_H_BIT_REG          0xA164
#define WDT_TIMER_M_BIT_REG          0xA165
#define WDT_TIMER_L_BIT_REG          0xA166
#define WDT_ENABLE_REG               0xA167
#define WDT_FEED_REG                 0xA168
#define WDT_PUNCH_REG                0xA169
#define WDT_START_FEED               0x01
#define WDT_STOP_FEED                0x00

#define POWER_CYCLE_RESET	    0x00
#define POWER_ON_RESET		    0x11
#define SOFT_SET_WARM_RESET	    0x22
#define SOFT_SET_COLD_RESET	    0x33
#define CPU_WARM_RESET		    0x44
#define CPU_COLD_RESET		    0x55
#define CPU_GPIO_WARM_RESET	    0x66
#define WDT_RESET		        0x77
#define CPU_OVERLOAD_RESET	    0x88
#define INSUFFICIENT_FAN_SPEED_RESET 0xAA


#define MAX_TIMER_VALUE     0xffffff
#define DEFUALT_TIMER_VALUE 180000    /* 180s */
#define WDT_ENABLE          0x01
#define WDT_DISABLE         0x00
#define WDT_RESTART         0x00
#define DRV_NAME 	        "cpld_wdt"
#define DRV_VERSION	        "1.0.0"
#define DEV_NAME	        "cpld_wdt"

struct wdt_data {
    unsigned long           opened;
    struct mutex            lock;
    char                    expect_close;
    struct watchdog_info    ident;
    int        	            timeout;
    int                     timer_val;
    char                    caused_reboot;  /* last reboot was by the watchdog */
    struct resource         *res;
};

struct cpld_wdt_private {
    struct platform_device *pdev;
    struct watchdog_device wddev;
    struct cdev cdev;
    struct miscdevice mdev;
    bool suspended;
    struct wdt_data wdat;
};

//struct class *cpld_wdt;
static const int max_timeout = MAX_TIMER_VALUE;

static int timeout = DEFUALT_TIMER_VALUE;	/* default 180s */
module_param(timeout, int, 0);
MODULE_PARM_DESC(timeout, "Start watchdog timer on module load with"
                 " given initial timeout(unit: ms)."
                 " Zero (default) disables this feature.");

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0644);
MODULE_PARM_DESC(nowayout, "Disable watchdog shutdown on close");

static unsigned int watchdog_get_timeleft(struct cpld_wdt_private *wdt)
{
    int time = 0;

    mutex_lock(&wdt->wdat.lock);

    time = inb(WDT_TIMER_H_BIT_REG);
    time = time << 8 | inb(WDT_TIMER_M_BIT_REG);
    time = time << 8 | inb(WDT_TIMER_L_BIT_REG);
    time = time/1000;
    mutex_unlock(&wdt->wdat.lock);

    return time;
}
static int watchdog_get_timeout(struct cpld_wdt_private *wdt)
{
    int timeout = 0;
    if(!wdt)
        return -EINVAL;

    mutex_lock(&wdt->wdat.lock);
    timeout = inb(WDT_SET_TIMER_H_BIT_REG);
    timeout = timeout << 8 | inb(WDT_SET_TIMER_M_BIT_REG);
    timeout = timeout << 8 | inb(WDT_SET_TIMER_L_BIT_REG);
    timeout=timeout/1000;
    mutex_unlock(&wdt->wdat.lock);

    return timeout;
}
static int watchdog_set_timeout(struct cpld_wdt_private *wdt, unsigned int timeout)
{
    unsigned char val = 0;
    if(!wdt)
        return -EINVAL;

    if (timeout <= 0
        || timeout >  max_timeout) {
        pr_err("watchdog timeout out of range\n");
        return -EINVAL;
    }

    mutex_lock(&wdt->wdat.lock);

    wdt->wdat.timeout = timeout;
    if (timeout > MAX_TIMER_VALUE) {
        wdt->wdat.timer_val = MAX_TIMER_VALUE;
    } else {
        wdt->wdat.timer_val = timeout;
    }
    /* Set timer value */
    //pr_crit("Watchdog Timeout:0x%06x\n", wdt->wdat.timer_val);

    outb((wdt->wdat.timer_val >> 16) & 0xff, WDT_SET_TIMER_H_BIT_REG);
    outb((wdt->wdat.timer_val >> 8) & 0xff, WDT_SET_TIMER_M_BIT_REG);
    outb(wdt->wdat.timer_val & 0xff, WDT_SET_TIMER_L_BIT_REG);

    mutex_unlock(&wdt->wdat.lock);

    return 0;
}

static int watchdog_ping(struct cpld_wdt_private *wdt)
{
    if(!wdt)
        return -EINVAL;

    mutex_lock(&wdt->wdat.lock);

    /* start feed watchdog */
    outb(WDT_START_FEED, WDT_FEED_REG);
    /* stop feed watchdog */
    outb(WDT_STOP_FEED, WDT_FEED_REG);

    mutex_unlock(&wdt->wdat.lock);

    return 0;
}

static void watchdog_keepalive(struct cpld_wdt_private *wdt)
{
    unsigned char val = 0;
    if(!wdt)
        return;

    mutex_lock(&wdt->wdat.lock);

    val = inb(WDT_FEED_REG);

    val &= 0x1;

    val = ~val;

    val &= 0x1;
    /* start feed watchdog */
    outb(val, WDT_FEED_REG);

    mutex_unlock(&wdt->wdat.lock);
    return;
}

static int watchdog_start(struct cpld_wdt_private *wdt)
{
    if(!wdt)
        return -EINVAL;

    /* Make sure we don't die as soon as the watchdog is enabled below */
    //watchdog_keepalive();
    mutex_lock(&wdt->wdat.lock);
    outb(WDT_ENABLE, WDT_ENABLE_REG);
    outb(WDT_RESTART, WDT_PUNCH_REG);
    mutex_unlock(&wdt->wdat.lock);

    return 0;
}

static int watchdog_stop(struct cpld_wdt_private *wdt)
{
    if(!wdt)
        return -EINVAL;

    mutex_lock(&wdt->wdat.lock);
    outb(WDT_DISABLE, WDT_ENABLE_REG);
    mutex_unlock(&wdt->wdat.lock);

    return 0;
}

static char watchdog_get_reason(struct cpld_wdt_private *p)
{
    char status = 0;

    if (!p)
        return -1;
    mutex_lock(&p->wdat.lock);
    status = inb(REBOOT_CAUSE_REG);
    mutex_unlock(&p->wdat.lock);

    return status;
}

static bool watchdog_is_running(struct cpld_wdt_private *wdt)
{
    /*
     * if we fail to determine the watchdog's status assume it to be
     * running to be on the safe side
     */

    bool is_running = true;

    mutex_lock(&wdt->wdat.lock);
    is_running = inb(WDT_ENABLE_REG);
    mutex_unlock(&wdt->wdat.lock);

    return is_running;
}

static const struct watchdog_info ident = {
    .options =              WDIOF_SETTIMEOUT |
        WDIOF_KEEPALIVEPING |
        WDIOF_MAGICCLOSE,
    .firmware_version =     0,
    .identity =             DRV_NAME,
};

static ssize_t identity_show(struct device *dev, struct device_attribute *attr,
                             char *buf)
{
    struct cpld_wdt_private *wdt = dev_get_drvdata(dev);
    if(!wdt)
        return -EINVAL;
    return sprintf(buf, "%s\n", wdt->wdat.ident.identity);
}

static DEVICE_ATTR_RO(identity);


static ssize_t state_show(struct device *dev, struct device_attribute *attr,
                          char *buf)
{
    struct cpld_wdt_private *wdt = dev_get_drvdata(dev);
    bool state = watchdog_is_running(wdt);
    if(true == state)
        return sprintf(buf, "active\n");
    else
        return sprintf(buf, "inactive\n");
}

static DEVICE_ATTR_RO(state);

static ssize_t status_show(struct device *dev, struct device_attribute *attr,
                           char *buf)
{
    unsigned int status;
    struct cpld_wdt_private *wdt = dev_get_drvdata(dev);
    if(!wdt)
        return -EINVAL;

    return sprintf(buf, "0x%x\n", status);
}

static DEVICE_ATTR_RO(status);

static ssize_t reason_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    char bootstatus;
    struct cpld_wdt_private *wdt = dev_get_drvdata(dev);
    if(!wdt)
        return -EINVAL;

    bootstatus = watchdog_get_reason(wdt);

    return sprintf(buf, "0x%02x\n", bootstatus);
}

static DEVICE_ATTR_RO(reason);

static ssize_t timeleft_show(struct device *dev, struct device_attribute *attr,
                             char *buf)
{
    unsigned int timeleft;
    struct cpld_wdt_private *wdt = dev_get_drvdata(dev);
    if(!wdt)
        return -EINVAL;

    timeleft = watchdog_get_timeleft(wdt);

    return sprintf(buf, "%u\n", timeleft);

}

static DEVICE_ATTR_RO(timeleft);


static ssize_t timeout_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
    unsigned int timeout;
    struct cpld_wdt_private *wdt = dev_get_drvdata(dev);
    if(!wdt)
        return -EINVAL;

    timeout = watchdog_get_timeout(wdt);

    return sprintf(buf, "%u\n", timeout);
}
static DEVICE_ATTR_RO(timeout);


static struct attribute *wdt_attrs[] = {
    &dev_attr_state.attr,
    &dev_attr_identity.attr,
    &dev_attr_status.attr,
    &dev_attr_reason.attr,
    &dev_attr_timeleft.attr,
    &dev_attr_timeout.attr,
    NULL,
};

static const struct attribute_group wdt_group = {
    .attrs = wdt_attrs,
};

static int watchdog_open(struct inode *inode, struct file *file)
{
    struct cpld_wdt_private *wdt;

    wdt = container_of(file->private_data, struct cpld_wdt_private, mdev);

    /* If the watchdog is alive we don't need to start it again */

    if (test_and_set_bit(0, &wdt->wdat.opened))
        return -EBUSY;

    //watchdog_start(wdt);

    if (nowayout)
        __module_get(THIS_MODULE);

    wdt->wdat.expect_close = 0;


    return nonseekable_open(inode, file);
}

static int watchdog_release(struct inode *inode, struct file *file)
{
    struct cpld_wdt_private *p;
    p = container_of(file->private_data, struct cpld_wdt_private, mdev);

    if(!p)
        return -EINVAL;

    clear_bit(0, &p->wdat.opened);

    if (!p->wdat.expect_close) {
        //watchdog_keepalive(p);
        //pr_crit("Unexpected close, Not stopping watchdog!\n");
    } else if (!nowayout) {
        //pr_crit("Cpld Watchdog Stopped!\n");
        watchdog_stop(p);
    }
    return 0;
}

/*
 *      watchdog_write:
 *      @file: file handle to the watchdog
 *      @buf: buffer to write
 *      @count: count of bytes
 *      @ppos: pointer to the position to write. No seeks allowed
 *
 *      A write to a watchdog device is defined as a keepalive signal. Any
 *      write of data will do, as we we don't define content meaning.
 */

static ssize_t watchdog_write(struct file *file, const char __user *buf,
                              size_t count, loff_t *ppos)
{
    struct cpld_wdt_private *p;
    p = container_of(file->private_data, struct cpld_wdt_private, mdev);

    if(!p)
        return -EINVAL;


    if (count) {
        if (!nowayout) {
            size_t i;

            /* In case it was set long ago */
            bool expect_close = false;

            for (i = 0; i != count; i++) {
                char c;
                if (get_user(c, buf + i))
                    return -EFAULT;
                expect_close = (c == 'V');
            }

            /* Properly order writes across fork()ed processes */
            mutex_lock(&p->wdat.lock);
            p->wdat.expect_close = expect_close;
            mutex_unlock(&p->wdat.lock);
        }

        /* someone wrote to us, we should restart timer */
        watchdog_keepalive(p);
    }
    return count;
}

/*
 *      watchdog_ioctl:
 *      @inode: inode of the device
 *      @file: file handle to the device
 *      @cmd: watchdog command
 *      @arg: argument pointer
 *
 *      The watchdog API defines a common set of functions for all watchdogs
 *      according to their available features.
 */
static long watchdog_ioctl(struct file *file, unsigned int cmd,
                           unsigned long arg)
{
    int status;
    int new_options;
    int new_timeout;
    unsigned int val;
    union {
        struct watchdog_info __user *ident;
        int __user *i;
    } uarg;

    uarg.i = (int __user *)arg;

    struct cpld_wdt_private *p;
    p = container_of(file->private_data, struct cpld_wdt_private, mdev);
    if(!p)
        return -EINVAL;

    switch (cmd) {
        case WDIOC_GETSUPPORT:
            return copy_to_user(uarg.ident, &p->wdat.ident,
                                sizeof(p->wdat.ident)) ? -EFAULT : 0;

        case WDIOC_GETSTATUS:
            status = watchdog_is_running(p);
            return put_user(status, uarg.i);

        case WDIOC_GETBOOTSTATUS:
            //status = watchdog_get_bootstatus(p);
            return put_user(status, uarg.i);

        case WDIOC_SETOPTIONS:
            if (get_user(new_options, uarg.i)){
                return -EFAULT;
            }

            if (new_options & WDIOS_DISABLECARD){
                return watchdog_stop(p);
            }

            if (new_options & WDIOS_ENABLECARD){
                return watchdog_start(p);
            }

            return 0;

        case WDIOC_KEEPALIVE:
            watchdog_keepalive(p);
            return 0;

        case WDIOC_SETTIMEOUT:
            if (get_user(new_timeout, uarg.i))
                return -EFAULT;
            new_timeout = new_timeout*1000;
            if (watchdog_set_timeout(p, new_timeout))
                return -EINVAL;

            val = watchdog_get_timeout(p);
            return put_user(val, uarg.i);
        case WDIOC_GETTIMEOUT:
            val = watchdog_get_timeout(p);
            return put_user(val, uarg.i);

        case WDIOC_GETTIMELEFT:
            val = watchdog_get_timeleft(p);
            return put_user(val, uarg.i);
        default:
            return -ENOTTY;

    }
}

static int watchdog_notify_sys(struct notifier_block *this, unsigned long code,
                               void *unused)
{
    if (code == SYS_DOWN || code == SYS_HALT)
        //watchdog_stop(p);

        pr_err("CPLD Watchdog did not Stop!\n");
    return NOTIFY_DONE;
}

static const struct file_operations watchdog_fops = {
    .owner		= THIS_MODULE,
    .open		= watchdog_open,
    .release	= watchdog_release,
    .write		= watchdog_write,
    .unlocked_ioctl	= watchdog_ioctl,
};

static struct miscdevice watchdog_miscdev = {
    //.minor		= WATCHDOG_MINOR,
    .name		= DEV_NAME,
    .fops		= &watchdog_fops,
};

static struct notifier_block watchdog_notifier = {
    .notifier_call = watchdog_notify_sys,
};

static int cpld_wdt_probe(struct platform_device *pdev)
{
    int wdt_reboot_cause, err = 0;
    unsigned char ver = 0;
    struct device *dev = &pdev->dev;

    struct cpld_wdt_private *p;

    p = devm_kzalloc(dev, sizeof(*p), GFP_KERNEL);
    if (!p)
        return -ENOMEM;


    mutex_init(&(p->wdat.lock));

    p->wdat.ident.options = WDIOC_SETTIMEOUT
        | WDIOF_MAGICCLOSE
        | WDIOF_KEEPALIVEPING
        | WDIOC_GETTIMELEFT;

    snprintf(p->wdat.ident.identity,
             sizeof(p->wdat.ident.identity), "%s", DRV_NAME);

    wdt_reboot_cause = inb(REBOOT_CAUSE_REG);  // REBOOT_CAUSE
    p->wdat.caused_reboot = wdt_reboot_cause;
    ver = inb(WDT_CONTROL_BASE);
    pr_info("Watchdog CPLD Version:0x%02x\n",
            ver);

    if (timeout) {
        if (timeout <= 0
            || timeout >  max_timeout) {
            pr_err("starting timeout out of range\n");
            err = -EINVAL;
            return err;
        }

        //watchdog_start(p);


        if (timeout > MAX_TIMER_VALUE) {
            watchdog_set_timeout(p, MAX_TIMER_VALUE);
        } else {
            watchdog_set_timeout(p, timeout);
        }

        if (nowayout)
            __module_get(THIS_MODULE);

        pr_info("watchdog started with initial timeout of %u Second(s)\n",
                timeout/1000);
    }

    err = watchdog_set_timeout(p, timeout);
    if (err)
        return err;

    err = register_reboot_notifier(&watchdog_notifier);
    if (err)
        return err;
    p->mdev = watchdog_miscdev;
    err = misc_register(&p->mdev);
    if (err) {
        pr_err("cannot register miscdev on minor=%d\n",
               watchdog_miscdev.minor);
        return err;;
    }

    /*p->wdat.res = platform_get_resource(pdev, IORESOURCE_IO, WDT_CONTROL_BASE);
      if (!p->wdat.res)
      return -ENODEV;

      if (!devm_request_region(dev, p->wdat.res->start,
      resource_size(p->wdat.res),
      pdev->name)) {
      return -EBUSY;
      }
      */
    err = sysfs_create_group(&pdev->dev.kobj, &wdt_group);
    if (err) {
        printk(KERN_ERR "Cannot create sysfs for cpld_wdt.\n");
        return err;
    }

    platform_set_drvdata(pdev, p);
    dev_set_drvdata(dev, p);

    pr_info("initialized.  sec (nowayout=%d)\n",
            nowayout);

    return 0;
}

static void cpld_wdt_remove(struct platform_device *pdev)
{
    struct cpld_wdt_private *p = platform_get_drvdata(pdev);

    if (p) {
        sysfs_remove_group(&pdev->dev.kobj, &wdt_group);
        misc_deregister(&p->mdev);
        unregister_reboot_notifier(&watchdog_notifier);
    }
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
        .start  = 0xA100,
        .end    = 0xA1F2,
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
    pr_info("Watchdog Module Unloaded\n");
}

module_init(cpld_wdt_init_module);
module_exit(cpld_wdt_cleanup_module);


MODULE_DESCRIPTION("Cpld Watchdog Driver");
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Nicholas <nicwu@celestica.com>");
MODULE_LICENSE("GPL");
