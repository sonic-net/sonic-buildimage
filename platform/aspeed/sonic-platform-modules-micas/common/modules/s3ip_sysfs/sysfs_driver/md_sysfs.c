/*
 * md_sysfs.c
 *
 * This module create md kobjects and attributes in /sys/s3ip/md
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2025-09-18                 S3IP sysfs
 */

#include "switch.h"
#include "md_sysfs.h"

static int g_md_loglevel = 0;

#define MD_INFO(fmt, args...) do { \
    if (g_md_loglevel & INFO) { \
        printk(KERN_INFO "[MD_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define MD_ERR(fmt, args...) do { \
    if (g_md_loglevel & ERR) { \
        printk(KERN_ERR "[MD_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define MD_DBG(fmt, args...) do { \
    if (g_md_loglevel & DBG) { \
        printk(KERN_DEBUG "[MD_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct md_obj_s {
    struct switch_obj *obj;
};

struct md_s {
    unsigned int md_number;
    struct md_obj_s *md;
};

static struct md_s g_md;
static struct switch_obj *g_md_obj = NULL;
static struct s3ip_sysfs_md_drivers_s *g_md_drv = NULL;

static ssize_t md_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_md.md_number);
}

static ssize_t md_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *md_attr;

    check_p(g_md_drv);
    check_p(g_md_drv->get_md_attr);

    index = obj->index;
    MD_DBG("md index: %u\n", index);

    md_attr = to_switch_device_attr(attr);
    check_p(md_attr);

    return g_md_drv->get_md_attr(index, md_attr->type, buf, PAGE_SIZE);
}

static ssize_t sys_md_attr_dbg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    unsigned int md_index;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_md_drv);
    check_p(g_md_drv->set_debug_md_attr);

    md_index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);

    ret = g_md_drv->set_debug_md_attr(tmp_attr->type, md_index, buf);
    if (ret < 0) {
        MD_ERR("md index: %u , type: %d , debug_value : %s, set fail.\n", md_index, tmp_attr->type, buf);
        return -EIO;
    }
    MD_DBG("md index: %u , type: %d , debug_value : %s, set success.\n", md_index, tmp_attr->type, buf);
    return count;
}
/************************************md dir and attr*******************************************/
static struct switch_attribute md_number_attr = __ATTR(number, S_IRUGO, md_number_show, NULL);

static struct attribute *md_root_attrs[] = {
    &md_number_attr.attr,
    NULL,
};

static struct attribute_group md_root_attr_group = {
    .attrs = md_root_attrs,
};

/************************************md[1-n] attr*******************************************/
static SWITCH_DEVICE_ATTR(status, S_IRUGO | S_IWUSR, md_attr_show, sys_md_attr_dbg_store, DFD_MD_STATUS_E);
static SWITCH_DEVICE_ATTR(dev_path, S_IRUGO, md_attr_show, NULL, DFD_MD_DEV_PATH_E);

static struct attribute *md_attrs[] = {
    &switch_dev_attr_status.switch_attr.attr,
    &switch_dev_attr_dev_path.switch_attr.attr,
    NULL,
};

static struct attribute_group md_attr_group = {
    .attrs = md_attrs,
};

static int md_root_create(void)
{
    g_md_obj = switch_kobject_create("md", NULL);
    if (!g_md_obj) {
        MD_ERR("create MD sysfs object failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_md_obj->kobj, &md_root_attr_group) != 0) {
        switch_kobject_delete(&g_md_obj);
        MD_ERR("create MD root attributes failed\n");
        return -EBADRQC;
    }

    return 0;
}

static int md_device_create(int index)
{
    char name[32];
    struct md_obj_s *md;
    int ret;

    md = &g_md.md[index - 1];

    snprintf(name, sizeof(name), "md%u", index);
    md->obj = switch_kobject_create(name, &g_md_obj->kobj);
    if (!md->obj) {
        MD_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    md->obj->index = index;

    ret = sysfs_create_group(&md->obj->kobj, &md_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&md->obj);
        MD_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    MD_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int md_devices_create(void)
{
    int i, ret;
    struct md_obj_s *md;

    g_md.md = kzalloc(sizeof(struct md_obj_s) * g_md.md_number, GFP_KERNEL);
    if (!g_md.md) {
        MD_ERR("allocate md objects failed\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_md.md_number; i++) {
        ret = md_device_create(i);
        if (ret) {
            while (--i >= 1) {
                md = &g_md.md[i - 1];
                if (md->obj) {
                    sysfs_remove_group(&md->obj->kobj, &md_attr_group);
                    switch_kobject_delete(&md->obj);
                }
            }
            kfree(g_md.md);
            g_md.md = NULL;
            return ret;
        }
    }

    return 0;
}

static void md_device_remove(int index)
{
    struct md_obj_s *md = &g_md.md[index - 1];
    if (md->obj) {
        sysfs_remove_group(&md->obj->kobj, &md_attr_group);
        switch_kobject_delete(&md->obj);
        MD_DBG("removed md%d\n", index);
    }
}

static void md_devices_remove(void)
{
    int i;
    struct md_obj_s *md;

    if (g_md.md) {
        for (i = g_md.md_number; i > 0; i--) {
            md = &g_md.md[i - 1];
            md_device_remove(i);
        }

        kfree(g_md.md);
        g_md.md = NULL;
    }

    g_md.md_number = 0;
}

static void md_sysfs_remove(void)
{
    md_devices_remove();

    if (g_md_obj) {
        sysfs_remove_group(&g_md_obj->kobj, &md_root_attr_group);
        switch_kobject_delete(&g_md_obj);
        g_md_obj = NULL;
    }
}

static int md_sysfs_create(void)
{
    int ret;

    ret = md_root_create();
    if (ret) {
        return ret;
    }

    ret = md_devices_create();
    if (ret) {
        md_sysfs_remove();
        MD_ERR("create md dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    return 0;
}

int s3ip_sysfs_md_drivers_register(struct s3ip_sysfs_md_drivers_s *drv)
{
    int md_number;
    int ret;

    MD_INFO("s3ip_sysfs_md_drivers_register...\n");
    if (g_md_drv) {
        MD_ERR("g_md_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_md_number);
    g_md_drv = drv;

    md_number = g_md_drv->get_md_number();
    if (md_number <= 0) {
        g_md_drv = NULL;
        MD_ERR("Invalid md number: %d\n", md_number);
        return -EINVAL;
    }

    mem_clear(&g_md, sizeof(struct md_s));

    g_md.md_number = md_number;

    ret = md_sysfs_create();
    if (ret) {
        g_md_drv = NULL;
        return ret;
    }

    MD_INFO("Registered MD driver with %d devices\n", md_number);

    return 0;
}

void s3ip_sysfs_md_drivers_unregister(void)
{
    if (g_md_drv) {
        md_sysfs_remove();
        g_md_drv = NULL;
        MD_INFO("MD driver unregistered\n");
    }
}

EXPORT_SYMBOL(s3ip_sysfs_md_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_md_drivers_unregister);
module_param(g_md_loglevel, int, 0644);
MODULE_PARM_DESC(g_md_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");