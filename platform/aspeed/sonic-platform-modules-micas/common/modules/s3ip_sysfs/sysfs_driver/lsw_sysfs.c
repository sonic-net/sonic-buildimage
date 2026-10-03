/*
 * lsw_sysfs.c
 *
 * This module create lsw kobjects and attributes in /sys/s3ip/lsw
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2025-12-24                 S3IP sysfs
 */

#include "switch.h"
#include "lsw_sysfs.h"

static int g_lsw_loglevel = 0;

#define LSW_INFO(fmt, args...) do { \
    if (g_lsw_loglevel & INFO) { \
        printk(KERN_INFO "[LSW_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define LSW_ERR(fmt, args...) do { \
    if (g_lsw_loglevel & ERR) { \
        printk(KERN_ERR "[LSW_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define LSW_DBG(fmt, args...) do { \
    if (g_lsw_loglevel & DBG) { \
        printk(KERN_DEBUG "[LSW_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct lsw_obj_s {
    struct switch_obj *obj;
};

struct lsw_s {
    unsigned int lsw_number;
    struct lsw_obj_s *lsw;
};

static struct lsw_s g_lsw;
static struct switch_obj *g_lsw_obj = NULL;
static struct s3ip_sysfs_lsw_drivers_s *g_lsw_drv = NULL;

static ssize_t lsw_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_lsw.lsw_number);
}

static ssize_t lsw_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *lsw_attr;

    check_p(g_lsw_drv);
    check_p(g_lsw_drv->get_lsw_attr);

    index = obj->index;
    LSW_DBG("lsw index: %u\n", index);

    lsw_attr = to_switch_device_attr(attr);
    check_p(lsw_attr);

    return g_lsw_drv->get_lsw_attr(index, lsw_attr->type, buf, PAGE_SIZE);
}

static ssize_t lsw_attr_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int index, value;
    int ret;
    struct switch_device_attribute *lsw_attr;

    check_p(g_lsw_drv);
    check_p(g_lsw_drv->set_lsw_attr);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        LSW_ERR("Invaild value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    index = obj->index;
    lsw_attr = to_switch_device_attr(attr);
    check_p(lsw_attr);
    ret = g_lsw_drv->set_lsw_attr(index, lsw_attr->type, value);
    if (ret < 0) {
        LSW_ERR("failed to set lsw%u reset, value:0x%x, ret: %d.\n", index, value, ret);
        return ret;
    }
    LSW_DBG("set lsw%u reset succeed, value: 0x%x.\n", index, value);
    return count;
}

/************************************lsw dir and attr*******************************************/
static struct switch_attribute lsw_number_attr = __ATTR(number, S_IRUGO, lsw_number_show, NULL);

static struct attribute *lsw_root_attrs[] = {
    &lsw_number_attr.attr,
    NULL,
};

static struct attribute_group lsw_root_attr_group = {
    .attrs = lsw_root_attrs,
};

/************************************lsw[1-n] attr*******************************************/
static SWITCH_DEVICE_ATTR(reset, S_IRUGO | S_IWUSR, lsw_attr_show, lsw_attr_store, DFD_LSW_RESET_E);

static struct attribute *lsw_attrs[] = {
    &switch_dev_attr_reset.switch_attr.attr,
    NULL,
};

static struct attribute_group lsw_attr_group = {
    .attrs = lsw_attrs,
};

static int lsw_root_create(void)
{
    g_lsw_obj = switch_kobject_create("lsw", NULL);
    if (!g_lsw_obj) {
        LSW_ERR("create LSW sysfs object failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_lsw_obj->kobj, &lsw_root_attr_group) != 0) {
        switch_kobject_delete(&g_lsw_obj);
        LSW_ERR("create LSW root attributes failed\n");
        return -EBADRQC;
    }

    return 0;
}

static int lsw_device_create(int index)
{
    char name[32];
    struct lsw_obj_s *lsw;
    int ret;

    lsw = &g_lsw.lsw[index - 1];

    snprintf(name, sizeof(name), "lsw%u", index);
    lsw->obj = switch_kobject_create(name, &g_lsw_obj->kobj);
    if (!lsw->obj) {
        LSW_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    lsw->obj->index = index;

    ret = sysfs_create_group(&lsw->obj->kobj, &lsw_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&lsw->obj);
        LSW_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    LSW_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int lsw_devices_create(void)
{
    int i, ret;
    struct lsw_obj_s *lsw;

    g_lsw.lsw = kzalloc(sizeof(struct lsw_obj_s) * g_lsw.lsw_number, GFP_KERNEL);
    if (!g_lsw.lsw) {
        LSW_ERR("allocate lsw objects failed\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_lsw.lsw_number; i++) {
        ret = lsw_device_create(i);
        if (ret) {
            while (--i >= 1) {
                lsw = &g_lsw.lsw[i - 1];
                if (lsw->obj) {
                    sysfs_remove_group(&lsw->obj->kobj, &lsw_attr_group);
                    switch_kobject_delete(&lsw->obj);
                }
            }
            kfree(g_lsw.lsw);
            g_lsw.lsw = NULL;
            return ret;
        }
    }

    return 0;
}

static void lsw_device_remove(int index)
{
    struct lsw_obj_s *lsw = &g_lsw.lsw[index - 1];
    if (lsw->obj) {
        sysfs_remove_group(&lsw->obj->kobj, &lsw_attr_group);
        switch_kobject_delete(&lsw->obj);
        LSW_DBG("removed lsw%d\n", index);
    }
}

static void lsw_devices_remove(void)
{
    int i;
    struct lsw_obj_s *lsw;

    if (g_lsw.lsw) {
        for (i = g_lsw.lsw_number; i > 0; i--) {
            lsw = &g_lsw.lsw[i - 1];
            lsw_device_remove(i);
        }

        kfree(g_lsw.lsw);
        g_lsw.lsw = NULL;
    }

    g_lsw.lsw_number = 0;
}

static void lsw_sysfs_remove(void)
{
    lsw_devices_remove();

    if (g_lsw_obj) {
        sysfs_remove_group(&g_lsw_obj->kobj, &lsw_root_attr_group);
        switch_kobject_delete(&g_lsw_obj);
        g_lsw_obj = NULL;
    }
}

static int lsw_sysfs_create(void)
{
    int ret;

    ret = lsw_root_create();
    if (ret) {
        return ret;
    }

    ret = lsw_devices_create();
    if (ret) {
        lsw_sysfs_remove();
        LSW_ERR("create lsw dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    return 0;
}

int s3ip_sysfs_lsw_drivers_register(struct s3ip_sysfs_lsw_drivers_s *drv)
{
    int lsw_number;
    int ret;

    LSW_INFO("s3ip_sysfs_lsw_drivers_register...\n");
    if (g_lsw_drv) {
        LSW_ERR("g_lsw_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_lsw_number);
    g_lsw_drv = drv;

    lsw_number = g_lsw_drv->get_lsw_number();
    if (lsw_number <= 0) {
        g_lsw_drv = NULL;
        LSW_ERR("Invalid lsw number: %d\n", lsw_number);
        return -EINVAL;
    }

    mem_clear(&g_lsw, sizeof(struct lsw_s));

    g_lsw.lsw_number = lsw_number;

    ret = lsw_sysfs_create();
    if (ret) {
        g_lsw_drv = NULL;
        return ret;
    }

    LSW_INFO("Registered LSW driver with %d devices\n", lsw_number);

    return 0;
}

void s3ip_sysfs_lsw_drivers_unregister(void)
{
    if (g_lsw_drv) {
        lsw_sysfs_remove();
        g_lsw_drv = NULL;
        LSW_INFO("LSW driver unregistered\n");
    }
}

EXPORT_SYMBOL(s3ip_sysfs_lsw_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_lsw_drivers_unregister);
module_param(g_lsw_loglevel, int, 0644);
MODULE_PARM_DESC(g_lsw_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");