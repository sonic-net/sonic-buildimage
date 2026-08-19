/*
 * A header definition for switch driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _SWITCH_H_
#define _SWITCH_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "switch_driver.h"
#include "wb_module.h"

#define DIR_NAME_MAX_LEN        (64)
#define DEBUG_FILE_SIZE     (64)
#define DEV_PRESEN_STR          "1\n"
#define DEV_ABSENT_STR          "0\n"
#define mem_clear(data, size) memset((data), 0, (size))

enum LOG_LEVEL {
    INFO = 0x1,
    ERR  = 0x2,
    DBG  = 0x4,
    ALL  = 0xf
};

extern int g_switch_loglevel;

#define check_pfun(p) do { \
    if (p == NULL) { \
        if (g_switch_loglevel & ERR) { \
            printk(KERN_ERR "%s, %s is NULL.\n", __FUNCTION__, #p); \
        } \
        return -WB_SYSFS_RV_UNSUPPORT; \
    } \
} while (0)

#define check_p(p) check_pfun(p)

#define to_switch_obj(x) container_of(x, struct switch_obj, kobj)
#define to_switch_attr(x) container_of(x, struct switch_attribute, attr)
#define to_switch_device_attr(x) container_of(x, struct switch_device_attribute, switch_attr)

#define SWITCH_ATTR(_name, _mode, _show, _store, _type)    \
    { .switch_attr = __ATTR(_name, _mode, _show, _store),  \
      .type = _type }

#define SWITCH_DEVICE_ATTR(_name, _mode, _show, _store, _type) \
struct switch_device_attribute switch_dev_attr_##_name         \
        = SWITCH_ATTR(_name, _mode, _show, _store, _type)

struct switch_obj {
    struct kobject kobj;
    unsigned int index;
};

/* a custom attribute that works just for a struct switch_obj. */
struct switch_attribute {
    struct attribute attr;
    ssize_t (*show)(struct switch_obj *foo, struct switch_attribute *attr, char *buf);
    ssize_t (*store)(struct switch_obj *foo, struct switch_attribute *attr, const char *buf, size_t count);
};

struct switch_device_attribute {
    struct switch_attribute switch_attr;
    int type;
};

struct switch_obj *switch_kobject_create(const char *name, struct kobject *parent);
void switch_kobject_delete(struct switch_obj **obj);
int dev_debug_file_read(char *file_name, unsigned int dev_index, char *buf, int size);

#endif /* _SWITCH_H_ */
