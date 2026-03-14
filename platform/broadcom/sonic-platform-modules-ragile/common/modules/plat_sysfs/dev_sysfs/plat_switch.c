/*
 * plat_switch.c
 * Original Author: support 2020-02-17
 *
 * This module create a kset in sysfs called /sys/wb_plat
 * Then other switch kobjects are created and assigned to this kset,
 * such as "board", "cpld", "fan", "psu", "sff", ...
 *
 * History
 *  [Version]        [Author]          [Date]             [Description]
 *   *  v1.0          support         2020-02-17          Initial version
 */
#include "./include/plat_switch.h"

#define SWITCH_INFO(fmt, args...)  LOG_INFO("switch: ", fmt, ##args)
#define SWITCH_ERR(fmt, args...)   LOG_ERR("switch: ", fmt, ##args)
#define SWITCH_DBG(fmt, args...)   LOG_DBG("switch: ", fmt, ##args)

static int g_loglevel = 0;

static ssize_t switch_attr_show(struct kobject *kobj,
                   struct attribute *attr, char *buf)
{
    struct switch_attribute *attribute;
    struct switch_obj *device;

    attribute = to_switch_attr(attr);
    device = to_switch_obj(kobj);

    if (!attribute->show)
        return -ENOSYS;

    return attribute->show(device, attribute, buf);
}

static ssize_t switch_attr_store(struct kobject *kobj,
                   struct attribute *attr, const char *buf, size_t len)
{
    struct switch_attribute *attribute;
    struct switch_obj *obj;

    attribute = to_switch_attr(attr);
    obj = to_switch_obj(kobj);

    if (!attribute->store)
        return -ENOSYS;

    return attribute->store(obj, attribute, buf, len);
}

static const struct sysfs_ops switch_sysfs_ops = {
    .show = switch_attr_show,
    .store = switch_attr_store,
};

static void switch_obj_release(struct kobject *kobj)
{
    struct switch_obj *obj;

    obj = to_switch_obj(kobj);
    kfree(obj);
}

static struct kobj_type switch_ktype = {
    .sysfs_ops = &switch_sysfs_ops,
    .release = switch_obj_release,
    .default_attrs = NULL,
};

static struct kset *switch_kset;

struct switch_obj *wb_plat_kobject_create(const char *name, struct kobject *parent)
{
    struct switch_obj *obj = NULL;
    int ret = 0;

    obj = kzalloc(sizeof(*obj), GFP_KERNEL);
    if (!obj) {
        SWITCH_DBG("wb_plat_kobject_create %s kzalloc error", name);
        return NULL;
   }

    obj->kobj.kset = switch_kset;

    ret = kobject_init_and_add(&obj->kobj, &switch_ktype, parent, "%s", name);
    if (ret) {
        kobject_put(&obj->kobj);
        SWITCH_DBG("kobject_init_and_add %s error", name);
        return NULL;
    }

    return obj;
}

void wb_plat_kobject_delete(struct switch_obj **obj)
{
    if (*obj) {
        SWITCH_DBG("%s delete %s.\n", (*obj)->kobj.parent->name, (*obj)->kobj.name);
        kobject_put(&((*obj)->kobj));
        *obj = NULL;
    }
}

static int __init switch_init(void)
{
    SWITCH_INFO("...\n");

    switch_kset = kset_create_and_add("wb_plat", NULL, NULL);
    if (!switch_kset) {
        SWITCH_ERR("create switch_kset error.\n");
        return -ENOMEM;
    }

    SWITCH_INFO("ok.\n");
    return 0;
}

static void __exit switch_exit(void)
{
    if (switch_kset) {
        kset_unregister(switch_kset);
    }

    SWITCH_INFO("ok.\n");
}

module_init(switch_init);
module_exit(switch_exit);
EXPORT_SYMBOL(wb_plat_kobject_create);
EXPORT_SYMBOL(wb_plat_kobject_delete);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("Switch driver");
