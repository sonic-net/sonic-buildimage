/*
 * CPLD Watchdog Driver
 *
 * Copyright (c) 2018 Ingrasys Corp.
 *
 * Author: Wade He <feng.cf.lee@ingrasys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *	Includes, defines, variables, module parameters, ...
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/* Module and version information */
#define DRV_NAME	"cpld_wdt"
#define DRV_VERSION	"1.0"

/* Includes */
#include <linux/module.h>		/* For module specific items */
#include <linux/moduleparam.h>		/* For new moduleparam's */
#include <linux/types.h>		/* For standard types (like size_t) */
#include <linux/errno.h>		/* For the -ENODEV/... values */
#include <linux/kernel.h>		/* For printk/panic/... */
#include <linux/watchdog.h>		/* For the watchdog specific items */
#include <linux/init.h>			/* For __init/__exit/... */
#include <linux/fs.h>			/* For file operations */
#include <linux/platform_device.h>	/* For platform_driver framework */
#include <linux/pci.h>			/* For pci functions */
#include <linux/ioport.h>		/* For io-port access */
#include <linux/spinlock.h>		/* For spin_lock/spin_unlock/... */
#include <linux/uaccess.h>		/* For copy_to_user/put_user/... */
#include <linux/io.h>			/* For inb/outb/... */
#include <linux/mfd/core.h>
#include <linux/mfd/lpc_ich.h>
#include <linux/kthread.h>
#include <linux/delay.h>


/* Address definitions for the CPLD */
/* CPLD base address */
#define TCOBASE		0x600
/* SMI Control and Enable Register */

#define TCO_RLD		(TCOBASE + 0x00) /* TCO Timer Reload and Curr. Value */
#define TCOv1_TMR	(TCOBASE + 0x01) /* TCOv1 Timer Initial Value	*/
#define TCO_DAT_IN	(TCOBASE + 0x02) /* TCO Data In Register	*/
#define TCO_DAT_OUT	(TCOBASE + 0x03) /* TCO Data Out Register	*/
#define TCO1_STS	(TCOBASE + 0x04) /* Control Watchdog Register	*/
#define TCO2_STS	(TCOBASE + 0x06) /* TCO2 Status Register	*/
#define TCO1_CNT	(TCOBASE + 0x08) /* TCO1 Control Register	*/
#define TCO2_CNT	(TCOBASE + 0x0a) /* TCO2 Control Register	*/
#define TCOv2_TMR	(TCOBASE + 0x12) /* TCOv2 Timer Initial Value	*/

#define DEBUG
#ifdef DEBUG
    #define DEBUG_PRINT(fmt, args...)            \
        printk (KERN_INFO "%s[%d]: " fmt "\r\n", \
                __FUNCTION__, __LINE__, ##args)
#else
    #define DEBUG_PRINT(fmt, args...)
#endif

#define ERROR_MSG(fmt, args...)                  \
        printk(KERN_ERR "%s[%d]: " fmt "\r\n",   \
               __FUNCTION__, __LINE__, ##args)



/* internal variables */
static struct {		/* this is private data for the cpld_wdt device */
	/* the lock for io operations */
	spinlock_t io_lock;
	struct platform_device *dev;
} cpld_wdt_private;

static struct task_struct *cpld_wdt_tsk;
static int data;

static void device_release(struct device *dev)
{
    return;
}

static struct platform_device cpld_wdt = {
    .name               = DRV_NAME,
    .id                 = 0,
    .dev                = {
                .platform_data   = NULL,
                .release         = device_release
    },
};

/* module parameters */
#define WATCHDOG_TIMEOUT 15	/* 15 sec default heartbeat */
static int heartbeat = WATCHDOG_TIMEOUT;  /* in seconds */
module_param(heartbeat, int, 0);
MODULE_PARM_DESC(heartbeat, "Watchdog ping period in seconds. "
	"5..20, default="
				__MODULE_STRING(WATCHDOG_TIMEOUT) ")");

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
	"Watchdog cannot be stopped once started (default="
				__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

   
static int cpld_wdt_check_timeout_range(unsigned int tmrval)
{
	if (tmrval < 5 || tmrval > 20) {
        DEBUG_PRINT("heartbeat out of range, using default=%d\n", WATCHDOG_TIMEOUT);
        heartbeat = WATCHDOG_TIMEOUT;
    } else {
        DEBUG_PRINT("heartbeat using %d seconds\n", heartbeat);
    }

	return 0;
}

/*
 * Some TCO specific functions
 */

static int cpld_wdt_stop(void *arg)
{

	spin_lock(&cpld_wdt_private.io_lock);

	outb(0x1, TCO1_STS);
    
    DEBUG_PRINT("cpld_wdt_stop done");

	spin_unlock(&cpld_wdt_private.io_lock);

	return 0;
}

static int cpld_wdt_ping(void *arg)
{
	spin_lock(&cpld_wdt_private.io_lock);

	/* Reload the timer by writing to the TCO Timer Counter register */
    outb(0x1, TCO1_STS);	/* write 1 to clear bit */
    udelay(100);
    outb(0x3, TCO1_STS);
    
    DEBUG_PRINT("cpld_wdt_ping done");

	spin_unlock(&cpld_wdt_private.io_lock);
	return 0;
}

static int kthread_wdt_ping_loop(void *arg)
{
    int i;

    set_current_state(TASK_INTERRUPTIBLE);

    while(!kthread_should_stop()) {
        DEBUG_PRINT("ping start");
        cpld_wdt_ping(NULL);
        set_current_state(TASK_INTERRUPTIBLE);
        for (i=0;i<heartbeat;i++) {
            msleep_interruptible(1000);
            if(kthread_should_stop()) {
                break;
            }
        }
        DEBUG_PRINT("ping once.");
    }

    return 0;
}


/*
 *	Init & exit routines
 */

static void cpld_wdt_cleanup(void)
{
	/* Stop the timer before we leave */
	if (!nowayout) {
		cpld_wdt_stop(NULL);
        DEBUG_PRINT("nowayout disabled. stop CPLD WDT success.");
    }
    DEBUG_PRINT("cpld_wdt_cleanup done");

}

static int cpld_wdt_probe(struct platform_device *dev)
{
	int ret = -ENODEV;

	spin_lock_init(&cpld_wdt_private.io_lock);

	/* Make sure the watchdog is not running */
	cpld_wdt_stop(NULL);
    
    cpld_wdt_check_timeout_range(heartbeat);
    
    cpld_wdt_tsk = kthread_create(kthread_wdt_ping_loop, &data, "cpld_wdt_tsk");
    if (IS_ERR(cpld_wdt_tsk)) {
        ret = PTR_ERR(cpld_wdt_tsk);
        cpld_wdt_tsk = NULL;
        ERROR_MSG("cpld_wdt_tsk create kthread failed.");
        goto out;
    }
    DEBUG_PRINT("wake_up_process cpld_wdt_tsk");
    wake_up_process(cpld_wdt_tsk);
    DEBUG_PRINT("wake_up_process done");

	return 0;
out:
    return ret;
}

static int cpld_wdt_remove(struct platform_device *dev)
{
    cpld_wdt_cleanup();
    kthread_stop(cpld_wdt_tsk);
    DEBUG_PRINT("cpld_wdt_remove done");

	return 0;
}

static void cpld_wdt_shutdown(struct platform_device *dev)
{
	cpld_wdt_stop(NULL);
    DEBUG_PRINT("cpld_wdt_shutdown done");
}

static struct platform_driver cpld_wdt_driver = {
	.probe          = cpld_wdt_probe,
	.remove         = cpld_wdt_remove,
	.shutdown       = cpld_wdt_shutdown,
	.driver         = {
		.owner  = THIS_MODULE,
		.name   = DRV_NAME,
	},
};

static int __init cpld_wdt_init_module(void)
{
	int err;

	DEBUG_PRINT("Intel TCO WatchDog Timer Driver v%s\n", DRV_VERSION);

	err = platform_driver_register(&cpld_wdt_driver);
	if (err) {
		ERROR_MSG("platform_driver_register error, err=%d\n", err);
        return err;
    }
    DEBUG_PRINT("platform_driver_register done\n");
    err = platform_device_register(&cpld_wdt);
    if (err) {
        printk(KERN_WARNING "Fail to create cpld device\n");
        goto error_cpld;
    }
    

	return 0;
error_cpld:    
    platform_driver_unregister(&cpld_wdt_driver);
    return err;
}

static void __exit cpld_wdt_cleanup_module(void)
{
	platform_device_unregister(&cpld_wdt);
    platform_driver_unregister(&cpld_wdt_driver);
	DEBUG_PRINT("Watchdog Module Unloaded\n");
}

module_init(cpld_wdt_init_module);
module_exit(cpld_wdt_cleanup_module);

MODULE_AUTHOR("Wade He<feng.cf.lee@ingrasys.com>");
MODULE_DESCRIPTION("CPLD Watchdog Timer Kernel Driver");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
