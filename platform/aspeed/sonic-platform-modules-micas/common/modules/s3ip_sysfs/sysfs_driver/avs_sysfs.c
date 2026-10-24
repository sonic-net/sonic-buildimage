/*
 * avs_sysfs.c
 *
 * This module create avs kobjects and attributes in /sys/s3ip/avs
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2025-09-18                 S3IP sysfs
 */

#include "switch.h"
#include "avs_sysfs.h"

static int g_avs_loglevel = 0;

#define AVS_INFO(fmt, args...) do { \
    if (g_avs_loglevel & INFO) { \
        printk(KERN_INFO "[AVS_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define AVS_ERR(fmt, args...) do { \
    if (g_avs_loglevel & ERR) { \
        printk(KERN_ERR "[AVS_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define AVS_DBG(fmt, args...) do { \
    if (g_avs_loglevel & DBG) { \
        printk(KERN_DEBUG "[AVS_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct avs_obj_s {
    struct switch_obj *obj;
};

struct avs_s {
    unsigned int avs_number;
    struct avs_obj_s *avs;
};

static struct avs_s g_avs;
static struct switch_obj *g_avs_obj = NULL;
static struct s3ip_sysfs_avs_drivers_s *g_avs_drv = NULL;

static ssize_t avs_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_avs.avs_number);
}

static ssize_t sys_avs_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *avs_attr;

    check_p(g_avs_drv);
    check_p(g_avs_drv->get_avs_attr);

    index = obj->index;
    AVS_DBG("avs index: %u\n", index);

    avs_attr = to_switch_device_attr(attr);
    check_p(avs_attr);

    return g_avs_drv->get_avs_attr(index, avs_attr->type, buf, PAGE_SIZE);
}

static ssize_t sys_avs_attr_dbg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    unsigned int index;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_avs_drv);
    check_p(g_avs_drv->set_debug_avs_attr);

    index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);

    ret = g_avs_drv->set_debug_avs_attr(tmp_attr->type, index, buf);
    if (ret < 0) {
        AVS_ERR("index: %u, type: %d, debug_value : %s, set fail.\n", index, tmp_attr->type, buf);
        return -EIO;
    }
    AVS_DBG("index: %u, type: %d, debug_value : %s, set success.\n", index, tmp_attr->type, buf);
    return count;
}

/************************************avs dir and attr*******************************************/
static struct switch_attribute avs_number_attr = __ATTR(number, S_IRUGO, avs_number_show, NULL);

static struct attribute *avs_root_attrs[] = {
    &avs_number_attr.attr,
    NULL,
};

static struct attribute_group avs_root_attr_group = {
    .attrs = avs_root_attrs,
};

/************************************avs[1-n] attr*******************************************/
static SWITCH_DEVICE_ATTR(power_status, S_IRUGO | S_IWUSR, sys_avs_attr_show,
        sys_avs_attr_dbg_store, DFD_AVS_POWER_STATUS_E);
static SWITCH_DEVICE_ATTR(bus_status, S_IRUGO | S_IWUSR, sys_avs_attr_show, 
        sys_avs_attr_dbg_store, DFD_AVS_BUS_STATUS_E);
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, sys_avs_attr_show, NULL, DFD_AVS_ALIAS_E);
static SWITCH_DEVICE_ATTR(circuit_status, S_IRUGO | S_IWUSR, sys_avs_attr_show,
        sys_avs_attr_dbg_store, DFD_AVS_CIRCUIT_STATUS_E);

static struct attribute *avs_attrs[] = {
    &switch_dev_attr_power_status.switch_attr.attr,
    &switch_dev_attr_bus_status.switch_attr.attr,
    &switch_dev_attr_alias.switch_attr.attr,
    &switch_dev_attr_circuit_status.switch_attr.attr,
    NULL,
};

static struct attribute_group avs_attr_group = {
    .attrs = avs_attrs,
};

static int avs_root_create(void)
{
    g_avs_obj = switch_kobject_create("avs", NULL);
    if (!g_avs_obj) {
        AVS_ERR("create AVS sysfs object failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_avs_obj->kobj, &avs_root_attr_group) != 0) {
        switch_kobject_delete(&g_avs_obj);
        AVS_ERR("create AVS root attributes failed\n");
        return -EBADRQC;
    }

    return 0;
}

static int avs_device_create(int index)
{
    char name[32];
    struct avs_obj_s *avs;
    int ret;

    avs = &g_avs.avs[index - 1];

    snprintf(name, sizeof(name), "avs%u", index);
    avs->obj = switch_kobject_create(name, &g_avs_obj->kobj);
    if (!avs->obj) {
        AVS_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    avs->obj->index = index;

    ret = sysfs_create_group(&avs->obj->kobj, &avs_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&avs->obj);
        AVS_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    AVS_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int avs_devices_create(void)
{
    int i, ret;
    struct avs_obj_s *avs;

    g_avs.avs = kzalloc(sizeof(struct avs_obj_s) * g_avs.avs_number, GFP_KERNEL);
    if (!g_avs.avs) {
        AVS_ERR("allocate avs objects failed\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_avs.avs_number; i++) {
        ret = avs_device_create(i);
        if (ret) {
            while (--i >= 1) {
                avs = &g_avs.avs[i - 1];
                if (avs->obj) {
                    sysfs_remove_group(&avs->obj->kobj, &avs_attr_group);
                    switch_kobject_delete(&avs->obj);
                }
            }
            kfree(g_avs.avs);
            g_avs.avs = NULL;
            return ret;
        }
    }

    return 0;
}

static void avs_device_remove(int index)
{
    struct avs_obj_s *avs = &g_avs.avs[index - 1];
    if (avs->obj) {
        sysfs_remove_group(&avs->obj->kobj, &avs_attr_group);
        switch_kobject_delete(&avs->obj);
        AVS_DBG("removed avs%d\n", index);
    }
}

static void avs_devices_remove(void)
{
    int i;
    struct avs_obj_s *avs;

    if (g_avs.avs) {
        for (i = g_avs.avs_number; i > 0; i--) {
            avs = &g_avs.avs[i - 1];
            avs_device_remove(i);
        }

        kfree(g_avs.avs);
        g_avs.avs = NULL;
    }

    g_avs.avs_number = 0;
}

static void avs_sysfs_remove(void)
{
    avs_devices_remove();

    if (g_avs_obj) {
        sysfs_remove_group(&g_avs_obj->kobj, &avs_root_attr_group);
        switch_kobject_delete(&g_avs_obj);
        g_avs_obj = NULL;
    }
}

static int avs_sysfs_create(void)
{
    int ret;

    ret = avs_root_create();
    if (ret) {
        return ret;
    }

    ret = avs_devices_create();
    if (ret) {
        avs_sysfs_remove();
        AVS_ERR("create avs dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    return 0;
}

int s3ip_sysfs_avs_drivers_register(struct s3ip_sysfs_avs_drivers_s *drv)
{
    int avs_number;
    int ret;

    AVS_INFO("s3ip_sysfs_avs_drivers_register...\n");
    if (g_avs_drv) {
        AVS_ERR("g_avs_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_avs_number);
    g_avs_drv = drv;

    avs_number = g_avs_drv->get_avs_number();
    if (avs_number <= 0) {
        g_avs_drv = NULL;
        AVS_ERR("Invalid avs number: %d\n", avs_number);
        return -EINVAL;
    }

    mem_clear(&g_avs, sizeof(struct avs_s));

    g_avs.avs_number = avs_number;

    ret = avs_sysfs_create();
    if (ret) {
        g_avs_drv = NULL;
        return ret;
    }

    AVS_INFO("Registered AVS driver with %d devices\n", avs_number);

    return 0;
}

void s3ip_sysfs_avs_drivers_unregister(void)
{
    if (g_avs_drv) {
        avs_sysfs_remove();
        g_avs_drv = NULL;
        AVS_INFO("AVS driver unregistered\n");
    }
}

EXPORT_SYMBOL(s3ip_sysfs_avs_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_avs_drivers_unregister);
module_param(g_avs_loglevel, int, 0644);
MODULE_PARM_DESC(g_avs_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");