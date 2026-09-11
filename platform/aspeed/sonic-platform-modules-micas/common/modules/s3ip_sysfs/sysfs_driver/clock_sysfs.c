/*
 * clock_sysfs.c
 *
 * This module create clock kobjects and attributes in /sys/s3ip/clock
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2025-09-18                 S3IP sysfs
 */

#include "switch.h"
#include "clock_sysfs.h"

static int g_clock_loglevel = 0;

#define CLOCK_INFO(fmt, args...) do { \
    if (g_clock_loglevel & INFO) { \
        printk(KERN_INFO "[CLOCK_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define CLOCK_ERR(fmt, args...) do { \
    if (g_clock_loglevel & ERR) { \
        printk(KERN_ERR "[CLOCK_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define CLOCK_DBG(fmt, args...) do { \
    if (g_clock_loglevel & DBG) { \
        printk(KERN_DEBUG "[CLOCK_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct clock_obj_s {
    struct switch_obj *obj;
};

struct clock_s {
    unsigned int clock_number;
    struct clock_obj_s *clock;
};

static struct clock_s g_clock;
static struct switch_obj *g_clock_obj = NULL;
static struct s3ip_sysfs_clock_drivers_s *g_clock_drv = NULL;

static ssize_t clock_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_clock.clock_number);
}

static ssize_t sys_clock_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *clock_attr;

    check_p(g_clock_drv);
    check_p(g_clock_drv->get_clock_attr);

    index = obj->index;
    CLOCK_DBG("clock index: %u\n", index);

    clock_attr = to_switch_device_attr(attr);
    check_p(clock_attr);

    return g_clock_drv->get_clock_attr(index, clock_attr->type, buf, PAGE_SIZE);
}

/************************************clock dir and attr*******************************************/
static struct switch_attribute clock_number_attr = __ATTR(number, S_IRUGO, clock_number_show, NULL);

static struct attribute *clock_root_attrs[] = {
    &clock_number_attr.attr,
    NULL,
};

static struct attribute_group clock_root_attr_group = {
    .attrs = clock_root_attrs,
};

/************************************clock[1-n] attr*******************************************/
static SWITCH_DEVICE_ATTR(status, S_IRUGO, sys_clock_attr_show, NULL, DFD_CLOCK_STATUS_E);
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, sys_clock_attr_show, NULL, DFD_CLOCK_ALIAS_E);

static struct attribute *clock_attrs[] = {
    &switch_dev_attr_status.switch_attr.attr,
    &switch_dev_attr_alias.switch_attr.attr,
    NULL,
};

static struct attribute_group clock_attr_group = {
    .attrs = clock_attrs,
};

static int clock_root_create(void)
{
    g_clock_obj = switch_kobject_create("clock", NULL);
    if (!g_clock_obj) {
        CLOCK_ERR("create CLOCK sysfs object failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_clock_obj->kobj, &clock_root_attr_group) != 0) {
        switch_kobject_delete(&g_clock_obj);
        CLOCK_ERR("create CLOCK root attributes failed\n");
        return -EBADRQC;
    }

    return 0;
}

static int clock_device_create(int index)
{
    char name[32];
    struct clock_obj_s *clock;
    int ret;

    clock = &g_clock.clock[index - 1];

    snprintf(name, sizeof(name), "clock%u", index);
    clock->obj = switch_kobject_create(name, &g_clock_obj->kobj);
    if (!clock->obj) {
        CLOCK_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    clock->obj->index = index;

    ret = sysfs_create_group(&clock->obj->kobj, &clock_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&clock->obj);
        CLOCK_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    CLOCK_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int clock_devices_create(void)
{
    int i, ret;
    struct clock_obj_s *clock;

    g_clock.clock = kzalloc(sizeof(struct clock_obj_s) * g_clock.clock_number, GFP_KERNEL);
    if (!g_clock.clock) {
        CLOCK_ERR("allocate clock objects failed\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_clock.clock_number; i++) {
        ret = clock_device_create(i);
        if (ret) {
            while (--i >= 1) {
                clock = &g_clock.clock[i - 1];
                if (clock->obj) {
                    sysfs_remove_group(&clock->obj->kobj, &clock_attr_group);
                    switch_kobject_delete(&clock->obj);
                }
            }
            kfree(g_clock.clock);
            g_clock.clock = NULL;
            return ret;
        }
    }

    return 0;
}

static void clock_device_remove(int index)
{
    struct clock_obj_s *clock = &g_clock.clock[index - 1];
    if (clock->obj) {
        sysfs_remove_group(&clock->obj->kobj, &clock_attr_group);
        switch_kobject_delete(&clock->obj);
        CLOCK_DBG("removed clock%d\n", index);
    }
}

static void clock_devices_remove(void)
{
    int i;
    struct clock_obj_s *clock;

    if (g_clock.clock) {
        for (i = g_clock.clock_number; i > 0; i--) {
            clock = &g_clock.clock[i - 1];
            clock_device_remove(i);
        }

        kfree(g_clock.clock);
        g_clock.clock = NULL;
    }

    g_clock.clock_number = 0;
}

static void clock_sysfs_remove(void)
{
    clock_devices_remove();

    if (g_clock_obj) {
        sysfs_remove_group(&g_clock_obj->kobj, &clock_root_attr_group);
        switch_kobject_delete(&g_clock_obj);
        g_clock_obj = NULL;
    }
}

static int clock_sysfs_create(void)
{
    int ret;

    ret = clock_root_create();
    if (ret) {
        return ret;
    }

    ret = clock_devices_create();
    if (ret) {
        clock_sysfs_remove();
        CLOCK_ERR("create clock dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    return 0;
}

int s3ip_sysfs_clock_drivers_register(struct s3ip_sysfs_clock_drivers_s *drv)
{
    int clock_number;
    int ret;

    CLOCK_INFO("s3ip_sysfs_clock_drivers_register...\n");
    if (g_clock_drv) {
        CLOCK_ERR("g_clock_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_clock_number);
    g_clock_drv = drv;

    clock_number = g_clock_drv->get_clock_number();
    if (clock_number <= 0) {
        g_clock_drv = NULL;
        CLOCK_ERR("Invalid clock number: %d\n", clock_number);
        return -EINVAL;
    }

    mem_clear(&g_clock, sizeof(struct clock_s));

    g_clock.clock_number = clock_number;

    ret = clock_sysfs_create();
    if (ret) {
        g_clock_drv = NULL;
        return ret;
    }

    CLOCK_INFO("Registered CLOCK driver with %d devices\n", clock_number);

    return 0;
}

void s3ip_sysfs_clock_drivers_unregister(void)
{
    if (g_clock_drv) {
        clock_sysfs_remove();
        g_clock_drv = NULL;
        CLOCK_INFO("CLOCK driver unregistered\n");
    }
}

EXPORT_SYMBOL(s3ip_sysfs_clock_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_clock_drivers_unregister);
module_param(g_clock_loglevel, int, 0644);
MODULE_PARM_DESC(g_clock_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");