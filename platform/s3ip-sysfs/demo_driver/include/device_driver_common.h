#ifndef _DEVICE_DRIVER_COMMON_H_
#define _DEVICE_DRIVER_COMMON_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/delay.h>

enum LOG_LEVEL{
    INFO = 0x1,
    ERR  = 0x2,
    DBG  = 0x4,
    ALL  = 0xf
};

#define LOG_INFO(_prefix, fmt, args...) do { \
    if (g_loglevel & INFO) { \
        printk( KERN_INFO _prefix "%s "fmt, __FUNCTION__, ##args); \
    } \
} while (0)

#define LOG_ERR(_prefix, fmt, args...) do { \
    if (g_loglevel & ERR) { \
        printk( KERN_ERR _prefix "%s "fmt, __FUNCTION__, ##args); \
    } \
} while (0)

#define LOG_DBG(_prefix, fmt, args...) do { \
    if (g_loglevel & DBG) { \
        printk( KERN_DEBUG _prefix "%s "fmt, __FUNCTION__, ##args); \
    } \
} while (0)

#define check_pfun(p) do { \
    if (p == NULL) { \
        if (g_loglevel & ERR) { \
            printk( KERN_ERR "%s, %s is NULL.\n", __FUNCTION__, #p); \
        } \
        return -ENOSYS; \
    } \
} while(0)

#define check_p(p) check_pfun(p)

#endif /* _DEVICE_DRIVER_COMMON_H_ */
