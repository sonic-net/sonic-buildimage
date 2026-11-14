/*
 * leak_detector_sysfs.c
 *
 * This module create leak_detector kobjects and attributes in /sys/s3ip/leak_detector
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "leak_detector_sysfs.h"

static int g_leak_detector_loglevel = 0;

#define LEAK_DETECTOR_INFO(fmt, args...) do {                                        \
    if (g_leak_detector_loglevel & INFO) { \
        printk(KERN_INFO "[LEAK_DETECTOR_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define LEAK_DETECTOR_ERR(fmt, args...) do {                                        \
    if (g_leak_detector_loglevel & ERR) { \
        printk(KERN_ERR "[LEAK_DETECTOR_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define LEAK_DETECTOR_DBG(fmt, args...) do {                                        \
    if (g_leak_detector_loglevel & DBG) { \
        printk(KERN_DEBUG "[LEAK_DETECTOR_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct leak_detector_obj_s {
    struct switch_obj *obj;
};

struct leak_detector_s {
    unsigned int leak_detector_number;
    struct leak_detector_obj_s *leak_detector;
};

static struct leak_detector_s g_leak_detector;
static struct switch_obj *g_leak_detector_obj = NULL;

static struct s3ip_sysfs_leak_detector_drivers_s *g_leak_detector_drv = NULL;

static ssize_t leak_detector_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_leak_detector.leak_detector_number);
}

static ssize_t sys_leak_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *leak_attr;

    check_p(g_leak_detector_drv);
    check_p(g_leak_detector_drv->get_main_board_leak_attr);

    index = obj->index;
    LEAK_DETECTOR_DBG("leak_detector index: %u\n", index);
    leak_attr = to_switch_device_attr(attr);
    check_p(leak_attr);

    return g_leak_detector_drv->get_main_board_leak_attr(index, leak_attr->type, buf, PAGE_SIZE);
}

static ssize_t sys_leak_attr_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    int ret, value;
    unsigned int index;
    struct switch_device_attribute *leak_attr;

    check_p(g_leak_detector_drv);
    check_p(g_leak_detector_drv->set_main_board_leak_attr);

    index = obj->index;
    LEAK_DETECTOR_DBG("leak_detector index: %u\n", index);

    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        LEAK_DETECTOR_ERR("invaild leak_detector status ret: %d, can't set simulate leak_detector status\n", ret);
        LEAK_DETECTOR_ERR("invaild leak_detector status buf: %s\n", buf);
        return -EINVAL;
    }

    leak_attr = to_switch_device_attr(attr);
    check_p(leak_attr);

    ret = g_leak_detector_drv->set_main_board_leak_attr(index, leak_attr->type, value);
    if (ret < 0) {
        LEAK_DETECTOR_ERR("set simulate leak_detector status %d faield, ret: %d\n", value, ret);
        return ret;
    }

    LEAK_DETECTOR_DBG("set simulate leak_detector status %d success\n", value);
    return count;
}

/************************************leak_detector dir and attrs*******************************************/
static struct switch_attribute leak_detector_number_attr = __ATTR(number, S_IRUGO, leak_detector_number_show, NULL);

static struct attribute *leak_detector_dir_attrs[] = {
    &leak_detector_number_attr.attr,
    NULL,
};

static struct attribute_group leak_detector_root_attr_group = {
    .attrs = leak_detector_dir_attrs,
};

/************************************leak_detector[1-n] and attrs*******************************************/
static SWITCH_DEVICE_ATTR(name, S_IRUGO, sys_leak_attr_show, NULL, DFD_LEAK_NAME_E);
static SWITCH_DEVICE_ATTR(status, S_IRUGO, sys_leak_attr_show, NULL, DFD_LEAK_STATUS_E);
static SWITCH_DEVICE_ATTR(simulate_status, S_IRUGO | S_IWUSR, sys_leak_attr_show, sys_leak_attr_store, DFD_LEAK_SIMULATE_STATUS_E);
static SWITCH_DEVICE_ATTR(present, S_IRUGO, sys_leak_attr_show, NULL, DFD_LEAK_PRESENT_E);

static struct attribute *leak_detector_attrs[] = {
    &switch_dev_attr_name.switch_attr.attr,
    &switch_dev_attr_status.switch_attr.attr,
    &switch_dev_attr_simulate_status.switch_attr.attr,
    &switch_dev_attr_present.switch_attr.attr,
    NULL,
};

static struct attribute_group leak_detector_attr_group = {
    .attrs = leak_detector_attrs,
};

/* create syseleak_detector directory and attributes*/
static int leak_detector_root_create(void)
{
    g_leak_detector_obj = switch_kobject_create("leak_detector", NULL);
    if (!g_leak_detector_obj) {
        LEAK_DETECTOR_ERR("switch_kobject_create leak_detector error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_leak_detector_obj->kobj, &leak_detector_root_attr_group) != 0) {
        switch_kobject_delete(&g_leak_detector_obj);
        LEAK_DETECTOR_ERR("create leak_detector dir attrs error!\n");
        return -EBADRQC;
    }

    return 0;
}

/* delete syseleak_detector directory and attributes*/
static void leak_detector_root_remove(void)
{
    if (g_leak_detector_obj) {
        sysfs_remove_group(&g_leak_detector_obj->kobj, &leak_detector_root_attr_group);
        switch_kobject_delete(&g_leak_detector_obj);
    }

    return;
}

static int leak_detector_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct leak_detector_obj_s *leak_detector;

    leak_detector = &g_leak_detector.leak_detector[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "leak_detector%u", index);
    leak_detector->obj = switch_kobject_create(name, parent);

    if (!leak_detector->obj) {
        LEAK_DETECTOR_ERR("create %s object error.\n", name);
        return -ENOMEM;
    }

    leak_detector->obj->index = index;
    if (sysfs_create_group(&leak_detector->obj->kobj, &leak_detector_attr_group) != 0) {
        LEAK_DETECTOR_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&leak_detector->obj);
        return -EBADRQC;
    }

    LEAK_DETECTOR_ERR("create %s dir and attrs success.\n", name);
    return 0;
}

static void leak_detector_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct leak_detector_obj_s *leak_detector;

    leak_detector = &g_leak_detector.leak_detector[index - 1];
    if (leak_detector->obj) {
        sysfs_remove_group(&leak_detector->obj->kobj, &leak_detector_attr_group);
        switch_kobject_delete(&leak_detector->obj);
        LEAK_DETECTOR_DBG("delete leak_detector%u dir and attrs success.\n", index);
    }

    return;
}

static int leak_detector_sub_create_kobj_and_attrs(struct kobject *parent, int leak_detector_number)
{
    unsigned int leak_detector_index, i;

    g_leak_detector.leak_detector = kzalloc(sizeof(struct leak_detector_obj_s) * leak_detector_number, GFP_KERNEL);
    if (!g_leak_detector.leak_detector) {
        LEAK_DETECTOR_ERR("kzalloc g_leak_detector.leak_detector error, leak_detector number: %d.\n", leak_detector_number);
        return -ENOMEM;
    }

    for (leak_detector_index = 1; leak_detector_index <= leak_detector_number; leak_detector_index++) {
        if (leak_detector_sub_single_create_kobj_and_attrs(parent, leak_detector_index) != 0) {
            goto error;
        }
    }

    return 0;

error:
    for (i = leak_detector_index - 1; i > 0; i--) {
        leak_detector_sub_single_remove_kobj_and_attrs(i);
    }

    kfree(g_leak_detector.leak_detector);
    g_leak_detector.leak_detector = NULL;

    return -EBADRQC;
}

/* create leak_detetor[1-n] directory and attributes*/
static int leak_detector_sub_create(void)
{
    int ret;

    ret = leak_detector_sub_create_kobj_and_attrs(&g_leak_detector_obj->kobj,
              g_leak_detector.leak_detector_number);
    return ret;
}

int s3ip_sysfs_leak_detector_drivers_register(struct s3ip_sysfs_leak_detector_drivers_s *drv)
{
    int ret, leak_detector_number;

    LEAK_DETECTOR_INFO("s3ip_sysfs_leak_detector_drivers_register...\n");
    if (g_leak_detector_drv) {
        LEAK_DETECTOR_ERR("g_leak_detector_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_leak_number);
    g_leak_detector_drv = drv;

    leak_detector_number = g_leak_detector_drv->get_main_board_leak_number();
    if (leak_detector_number <= 0) {
        LEAK_DETECTOR_ERR("leak_detector_number is %d, can't register\n", leak_detector_number);
        g_leak_detector_drv = NULL;
        return -EINVAL;
    }

    mem_clear(&g_leak_detector, sizeof(struct leak_detector_s));
    g_leak_detector.leak_detector_number = leak_detector_number;

    ret = leak_detector_root_create();
    if (ret < 0) {
        LEAK_DETECTOR_ERR("leak_detector create error.\n");
        g_leak_detector.leak_detector_number = 0;
        g_leak_detector_drv = NULL;
        return ret;
    }

    ret = leak_detector_sub_create();
    if (ret < 0) {
        LEAK_DETECTOR_ERR("create leak_detector sub dir and attrs failed, ret: %d\n", ret);
        leak_detector_root_remove();
        g_leak_detector.leak_detector_number = 0;
        g_leak_detector_drv = NULL;
        return ret;
    }

    LEAK_DETECTOR_INFO("s3ip_sysfs_leak_detector_drivers_register success\n");
    return 0;
}

/* delete leak_detector[1-n] directory and attributes*/
static void leak_detector_sub_remove(void)
{
    unsigned int leak_detector_index;

    if (g_leak_detector.leak_detector) {
        for (leak_detector_index = g_leak_detector.leak_detector_number; leak_detector_index > 0; leak_detector_index--) {
            leak_detector_sub_single_remove_kobj_and_attrs(leak_detector_index);
        }

        kfree(g_leak_detector.leak_detector);
        g_leak_detector.leak_detector = NULL;
    }

    g_leak_detector.leak_detector_number = 0;
    return;
}

void s3ip_sysfs_leak_detector_drivers_unregister(void)
{
    if (g_leak_detector_drv) {
        leak_detector_sub_remove();
        leak_detector_root_remove();
        g_leak_detector_drv = NULL;
        LEAK_DETECTOR_DBG("s3ip_sysfs_leak_detector_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_leak_detector_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_leak_detector_drivers_unregister);
module_param(g_leak_detector_loglevel, int, 0644);
MODULE_PARM_DESC(g_leak_detector_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");