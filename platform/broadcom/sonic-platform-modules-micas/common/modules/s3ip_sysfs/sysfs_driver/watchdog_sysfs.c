/*
 * An watchdog_sysfs driver for watchdog sysfs devcie function
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

#include <linux/slab.h>

#include "switch.h"
#include "watchdog_sysfs.h"

static int g_wdt_loglevel = 0;

#define WDT_INFO(fmt, args...) do {                                        \
    if (g_wdt_loglevel & INFO) { \
        printk(KERN_INFO "[WDT_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WDT_ERR(fmt, args...) do {                                        \
    if (g_wdt_loglevel & ERR) { \
        printk(KERN_ERR "[WDT_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WDT_DBG(fmt, args...) do {                                        \
    if (g_wdt_loglevel & DBG) { \
        printk(KERN_DEBUG "[WDT_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

typedef enum wdt_enable_status_e {
    WDT_DISENABLE        = 0,     /* close watchdog */
    WDT_ENABLE           = 1,     /* open watchdog */
} wdt_enable_status_t;

static struct switch_obj *g_watchdog_obj = NULL;
static struct s3ip_sysfs_watchdog_drivers_s *g_wdt_drv = NULL;

static ssize_t watchdog_identify_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_wdt_drv);
    check_p(g_wdt_drv->get_watchdog_identify);

    return g_wdt_drv->get_watchdog_identify(buf, PAGE_SIZE);
}

static ssize_t watchdog_timeleft_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_wdt_drv);
    check_p(g_wdt_drv->get_watchdog_timeleft);

    return g_wdt_drv->get_watchdog_timeleft(buf, PAGE_SIZE);
}

static ssize_t watchdog_timeout_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_wdt_drv);
    check_p(g_wdt_drv->get_watchdog_timeout);

    return g_wdt_drv->get_watchdog_timeout(buf, PAGE_SIZE);
}

static ssize_t watchdog_timeout_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    int ret, value;

    check_p(g_wdt_drv);
    check_p(g_wdt_drv->set_watchdog_timeout);

    sscanf(buf, "%d", &value);
    if (value < 0) {
        WDT_ERR("invaild timeout value: %d, can't set watchdog timeout\n", value);
        return -EINVAL;
    }

    ret = g_wdt_drv->set_watchdog_timeout(value);
    if (ret < 0) {
        WDT_ERR("set watchdog timeout value: %d failed, ret: %d\n", value, ret);
        return ret;
    }
    WDT_DBG("set watchdog timeout value: %d success\n", ret);
    return count;
}

static ssize_t watchdog_enable_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_wdt_drv);
    check_p(g_wdt_drv->get_watchdog_enable_status);

    return g_wdt_drv->get_watchdog_enable_status(buf, PAGE_SIZE);
}

static ssize_t watchdog_enable_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    int ret, value;

    check_p(g_wdt_drv);
    check_p(g_wdt_drv->set_watchdog_enable_status);

    sscanf(buf, "%d", &value);
    if ((value != WDT_DISENABLE) && (value != WDT_ENABLE)) {
        WDT_ERR("invaild enable value: %d, can't set watchdog enable status\n", value);
        return -EINVAL;
    }

    ret = g_wdt_drv->set_watchdog_enable_status(value);
    if (ret < 0) {
        WDT_ERR("set watchdog enable status %d failed, ret: %d\n", value, ret);
        return ret;
    }
    WDT_DBG("set watchdog enable status %d success\n", ret);
    return count;
}

static ssize_t watchdog_reset_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret, value;

    check_p(g_wdt_drv);
    check_p(g_wdt_drv->set_watchdog_reset);

    ret = kstrtoint(buf, 0, &value);
    if (ret) {
        WDT_ERR("invalid value: %s \n", buf);
        return -EINVAL;
    }

    ret = g_wdt_drv->set_watchdog_reset(value);
    if (ret < 0) {
        WDT_ERR("set watchdog reset %d failed, ret: %d\n", value, ret);
        return ret;
    }
    WDT_DBG("set watchdog reset %d success\n", ret);
    return count;
}

/************************************watchdog*******************************************/
static struct switch_attribute watchdog_identify_attr = __ATTR(identify, S_IRUGO, watchdog_identify_show, NULL);
static struct switch_attribute watchdog_timeleft_attr = __ATTR(timeleft, S_IRUGO, watchdog_timeleft_show, NULL);
static struct switch_attribute watchdog_timeout_attr = __ATTR(timeout, S_IRUGO | S_IWUSR, watchdog_timeout_show, watchdog_timeout_store);
static struct switch_attribute watchdog_enable_attr = __ATTR(enable, S_IRUGO | S_IWUSR, watchdog_enable_status_show, watchdog_enable_status_store);
static struct switch_attribute watchdog_reset_attr = __ATTR(reset, S_IWUSR, NULL, watchdog_reset_store);

static struct attribute *watchdog_dir_attrs[] = {
    &watchdog_identify_attr.attr,
    &watchdog_timeleft_attr.attr,
    &watchdog_timeout_attr.attr,
    &watchdog_enable_attr.attr,
    &watchdog_reset_attr.attr,
    NULL,
};

static struct attribute_group watchdog_attr_group = {
    .attrs = watchdog_dir_attrs,
};

/* create watchdog directory and attributes */
static int watchdog_root_create(void)
{
    g_watchdog_obj = switch_kobject_create("watchdog", NULL);
    if (!g_watchdog_obj) {
        WDT_ERR("switch_kobject_create watchdog error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_watchdog_obj->kobj, &watchdog_attr_group) != 0) {
        switch_kobject_delete(&g_watchdog_obj);
        WDT_ERR("create fan dir attrs error!\n");
        return -EBADRQC;
    }

    return 0;
}

/* delete watchdog directory and attributes */
static void watchdog_root_remove(void)
{
    if (g_watchdog_obj) {
        sysfs_remove_group(&g_watchdog_obj->kobj, &watchdog_attr_group);
        switch_kobject_delete(&g_watchdog_obj);
    }

    return;
}

int s3ip_sysfs_watchdog_drivers_register(struct s3ip_sysfs_watchdog_drivers_s *drv)
{
    int ret;

    WDT_INFO("s3ip_sysfs_watchdog_drivers_register...\n");
    if (g_wdt_drv) {
        WDT_ERR("g_wdt_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    g_wdt_drv = drv;

    ret = watchdog_root_create();
    if (ret < 0) {
        WDT_ERR("watchdog create error.\n");
        g_wdt_drv = NULL;
        return ret;
    }
    WDT_INFO("s3ip_sysfs_watchdog_drivers_register success\n");
    return 0;
}

void s3ip_sysfs_watchdog_drivers_unregister(void)
{
    if (g_wdt_drv) {
        watchdog_root_remove();
        g_wdt_drv = NULL;
        WDT_DBG("s3ip_sysfs_watchdog_drivers_unregister success.\n");
    }

    return;
}

EXPORT_SYMBOL(s3ip_sysfs_watchdog_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_watchdog_drivers_unregister);
module_param(g_wdt_loglevel, int, 0644);
MODULE_PARM_DESC(g_wdt_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
