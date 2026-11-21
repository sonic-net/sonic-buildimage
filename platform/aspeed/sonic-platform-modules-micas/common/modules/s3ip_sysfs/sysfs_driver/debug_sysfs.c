/*
 * debug_sysfs.c
 *
 * This module create debug kobjects and attributes in /sys/s3ip/debug
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2023-10-27                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "debug_sysfs.h"
#include "fan_sysfs.h"
#include "psu_sysfs.h"
#include "transceiver_sysfs.h"

static int g_debug_loglevel = 0;
/* g_reset: 1 means debug mode is not supported, 0 means debug mode is available */
static unsigned int g_reset = 1;
#define DEBUG_INFO(fmt, args...) do {                                        \
    if (g_debug_loglevel & INFO) { \
        printk(KERN_INFO "[DEBUG_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define DEBUG_ERR(fmt, args...) do {                                        \
    if (g_debug_loglevel & ERR) { \
        printk(KERN_ERR "[DEBUG_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define DEBUG_DBG(fmt, args...) do {                                        \
    if (g_debug_loglevel & DBG) { \
        printk(KERN_DEBUG "[DEBUG_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static struct switch_obj *g_debug_obj = NULL;
static struct s3ip_sysfs_debug_drivers_s *g_debug_drv = NULL;


static ssize_t eth_debug_present_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return eth_debug_present_get(buf);
}

static ssize_t eth_debug_present_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    return eth_debug_present_set(buf, count);
}

static ssize_t fan_debug_present_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return fan_debug_present_get(buf);
}

static ssize_t fan_debug_present_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    return fan_debug_present_set(buf, count);
}

static ssize_t psu_debug_present_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return psu_debug_present_get(buf);
}

static ssize_t psu_debug_present_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    return psu_debug_present_set(buf, count);
}


static ssize_t debug_and_reset_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_reset);
}


static ssize_t debug_and_reset_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{

    int ret, reset, debug_on;

    check_p(g_debug_drv);
    check_p(g_debug_drv->set_debug_and_reset);

    reset = 0;
    ret = kstrtoint(buf, 0, &reset);
    if (ret != 0) {
        DEBUG_ERR("invaild reset ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (reset != 0 && reset != 1) {
        DEBUG_ERR("param invalid, reset must be 0 or 1: %d.\n", reset);
        return -EINVAL;
    }
    /* g_reset: 1 means debug mode is not supported, 0 means debug mode is available */
    debug_on = (reset == 1) ? 0 : 1;
    ret = g_debug_drv->set_debug_and_reset(debug_on);
    if (ret < 0) {
        DEBUG_ERR("set reset: %d failed, ret: %d\n", reset, ret);
        return ret;
    }

    g_reset = reset;
    DEBUG_DBG("set debug reset: %d success\n", reset);
    return count;

}

/************************************debug*******************************************/
static struct switch_attribute present_on_eth_attr = __ATTR(present_on_eth, S_IRUGO | S_IWUSR, eth_debug_present_show, eth_debug_present_store);
static struct switch_attribute present_on_fan_attr = __ATTR(present_on_fan, S_IRUGO | S_IWUSR, fan_debug_present_show, fan_debug_present_store);
static struct switch_attribute present_on_psu_attr = __ATTR(present_on_psu, S_IRUGO | S_IWUSR, psu_debug_present_show, psu_debug_present_store);
static struct switch_attribute debug_mode_reset_attr = __ATTR(debug_mode_reset, S_IRUGO | S_IWUSR, debug_and_reset_show, debug_and_reset_store);

static struct attribute *debug_dir_attrs[] = {
    &present_on_eth_attr.attr,
    &present_on_fan_attr.attr,
    &present_on_psu_attr.attr,
    &debug_mode_reset_attr.attr,
    NULL,
};

static struct attribute_group debug_attr_group = {
    .attrs = debug_dir_attrs,
};

/* create debug directory and attributes */
static int debug_root_create(void)
{
    g_debug_obj = switch_kobject_create("debug", NULL);
    if (!g_debug_obj) {
        DEBUG_ERR("switch_kobject_create debug error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_debug_obj->kobj, &debug_attr_group) != 0) {
        switch_kobject_delete(&g_debug_obj);
        DEBUG_ERR("create debug dir attrs error!\n");
        return -EBADRQC;
    }

    return 0;
}

/* delete debug directory and attributes */
static void debug_root_remove(void)
{
    if (g_debug_obj) {
        sysfs_remove_group(&g_debug_obj->kobj, &debug_attr_group);
        switch_kobject_delete(&g_debug_obj);
    }

    return;
}

int s3ip_sysfs_debug_drivers_register(struct s3ip_sysfs_debug_drivers_s *drv)
{
    int ret;

    DEBUG_INFO("s3ip_sysfs_debug_drivers_register...\n");
    if (g_debug_drv) {
        DEBUG_ERR("g_debug_drv is not NULL, can't register\n");
        return -EPERM;
    }
    check_p(drv);
    g_debug_drv = drv;

    ret = debug_root_create();
    if (ret < 0) {
        DEBUG_ERR("debug create error.\n");
        g_debug_drv = NULL;
        return ret;
    }

    DEBUG_INFO("s3ip_sysfs_debug_drivers_register success\n");
    return 0;
}

void s3ip_sysfs_debug_drivers_unregister(void)
{
    debug_root_remove();
    DEBUG_DBG("s3ip_sysfs_debug_drivers_unregister success.\n");

    return;
}

EXPORT_SYMBOL(s3ip_sysfs_debug_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_debug_drivers_unregister);
module_param(g_debug_loglevel, int, 0644);
MODULE_PARM_DESC(g_debug_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
