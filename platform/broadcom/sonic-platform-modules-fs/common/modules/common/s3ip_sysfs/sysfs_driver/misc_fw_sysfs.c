/*
 * misc_fw_sysfs.c
 *
 * This module create misc_fw kobjects and attributes in /sys/s3ip/misc_fw
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>
#include <linux/fs.h>

#include "switch.h"
#include "misc_fw_sysfs.h"

static int g_misc_fw_loglevel = 0;

#define MISC_FW_INFO(fmt, args...) do {                                        \
    if (g_misc_fw_loglevel & INFO) { \
        printk(KERN_INFO "[MISC_FW_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define MISC_FW_ERR(fmt, args...) do {                                        \
    if (g_misc_fw_loglevel & ERR) { \
        printk(KERN_ERR "[MISC_FW_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define MISC_FW_DBG(fmt, args...) do {                                        \
    if (g_misc_fw_loglevel & DBG) { \
        printk(KERN_DEBUG "[MISC_FW_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct misc_fw_obj_s {
    struct switch_obj *obj;
};

struct misc_fw_s {
    unsigned int misc_fw_number;
    struct misc_fw_obj_s *misc_fw;
};

static struct misc_fw_s g_misc_fw;
static struct switch_obj *g_misc_fw_obj = NULL;
static struct s3ip_sysfs_misc_fw_drivers_s *g_misc_fw_drv = NULL;

static ssize_t misc_fw_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_misc_fw.misc_fw_number);
}

static ssize_t misc_fw_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int misc_fw_index;
    struct switch_device_attribute *misc_fw_attr;

    check_p(g_misc_fw_drv);
    check_p(g_misc_fw_drv->get_main_board_misc_fw_attr);

    misc_fw_index = obj->index;
    MISC_FW_DBG("misc_fw index: %u\n", misc_fw_index);
    misc_fw_attr = to_switch_device_attr(attr);
    check_p(misc_fw_attr);

    return g_misc_fw_drv->get_main_board_misc_fw_attr(misc_fw_index, misc_fw_attr->type, buf, PAGE_SIZE);
}

#if 0
static ssize_t misc_fw_attr_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int misc_fw_index, value;
    int ret;
    struct switch_device_attribute *misc_fw_attr;

    check_p(g_misc_fw_drv);
    check_p(g_misc_fw_drv->set_main_board_misc_fw_attr);

    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        MISC_FW_ERR("Invaild value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    misc_fw_index = obj->index;
    misc_fw_attr = to_switch_device_attr(attr);
    check_p(misc_fw_attr);
    ret = g_misc_fw_drv->set_main_board_misc_fw_attr(misc_fw_index, misc_fw_attr->type, value);
    if (ret < 0) {
        MISC_FW_ERR("set misc_fw%u test reg failed, value:0x%x, ret: %d.\n", misc_fw_index, value, ret);
        return ret;
    }
    MISC_FW_DBG("set misc_fw%u test reg success, value: 0x%x.\n", misc_fw_index, value);
    return count;
}
#endif

/************************************misc_fw dir and attrs*******************************************/
static struct switch_attribute misc_fw_number_att = __ATTR(number, S_IRUGO, misc_fw_number_show, NULL);

static struct attribute *misc_fw_dir_attrs[] = {
    &misc_fw_number_att.attr,
    NULL,
};

static struct attribute_group misc_fw_root_attr_group = {
    .attrs = misc_fw_dir_attrs,
};

/*******************************misc_fw[1-n] dir and attrs*******************************************/
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, misc_fw_attr_show, NULL, DFD_MISC_FW_NAME_E);
static SWITCH_DEVICE_ATTR(type, S_IRUGO, misc_fw_attr_show, NULL, DFD_MISC_FW_TYPE_E);
static SWITCH_DEVICE_ATTR(firmware_version, S_IRUGO, misc_fw_attr_show, NULL, DFD_MISC_FW_FW_VERSION_E);
static SWITCH_DEVICE_ATTR(board_version, S_IRUGO, misc_fw_attr_show, NULL, DFD_MISC_FW_HW_VERSION_E);
static SWITCH_DEVICE_ATTR(vendor, S_IRUGO, misc_fw_attr_show, NULL, DFD_MISC_FW_VENDOR_E);
static SWITCH_DEVICE_ATTR(support_upgrade, S_IRUGO, misc_fw_attr_show, NULL, DFD_MISC_FW_SUPPORT_UPGRADE_E);
static SWITCH_DEVICE_ATTR(upgrade_active_type, S_IRUGO, misc_fw_attr_show, NULL, DFD_MISC_FW_UPGRADE_ACTIVE_TYPE_E);


static struct attribute *misc_fw_attrs[] = {
    &switch_dev_attr_alias.switch_attr.attr,
    &switch_dev_attr_type.switch_attr.attr,
    &switch_dev_attr_firmware_version.switch_attr.attr,
    &switch_dev_attr_board_version.switch_attr.attr,
    &switch_dev_attr_vendor.switch_attr.attr,
    &switch_dev_attr_support_upgrade.switch_attr.attr,
    &switch_dev_attr_upgrade_active_type.switch_attr.attr,
    NULL,
};

static struct attribute_group misc_fw_attr_group = {
    .attrs = misc_fw_attrs,
};

static int misc_fw_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct misc_fw_obj_s *curr_misc_fw;

    curr_misc_fw = &g_misc_fw.misc_fw[index - 1];
    if (curr_misc_fw->obj) {
        sysfs_remove_group(&curr_misc_fw->obj->kobj, &misc_fw_attr_group);
        switch_kobject_delete(&curr_misc_fw->obj);
        MISC_FW_DBG("delete misc_fw%u dir and attrs success.\n", index);
    }

    return 0;
}

static int misc_fw_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[16];
    struct misc_fw_obj_s *curr_misc_fw;

    curr_misc_fw = &g_misc_fw.misc_fw[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "misc_fw%u", index);
    curr_misc_fw->obj = switch_kobject_create(name, parent);
    if (!curr_misc_fw->obj) {
        MISC_FW_ERR("create %s object error!\n", name);
        return -EBADRQC;
    }
    curr_misc_fw->obj->index = index;
    if (sysfs_create_group(&curr_misc_fw->obj->kobj, &misc_fw_attr_group) != 0) {
        MISC_FW_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&curr_misc_fw->obj);
        return -EBADRQC;
    }
    MISC_FW_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

static int misc_fw_sub_create_kobj_and_attrs(struct kobject *parent, int misc_fw_num)
{
    unsigned int misc_fw_index, i;

    g_misc_fw.misc_fw = kzalloc(sizeof(struct misc_fw_obj_s) * misc_fw_num, GFP_KERNEL);
    if (!g_misc_fw.misc_fw) {
        MISC_FW_ERR("kzalloc g_misc_fw.misc_fw error, misc_fw number = %d.\n", misc_fw_num);
        return -ENOMEM;
    }

    for (misc_fw_index = 1; misc_fw_index <= misc_fw_num; misc_fw_index++) {
        if (misc_fw_sub_single_create_kobj_and_attrs(parent, misc_fw_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = misc_fw_index - 1; i > 0; i--) {
        misc_fw_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_misc_fw.misc_fw);
    g_misc_fw.misc_fw = NULL;
    return -EBADRQC;
}

/* create misc_fw[1-n] directory and attributes*/
static int misc_fw_sub_create(void)
{
    int ret;

    ret = misc_fw_sub_create_kobj_and_attrs(&g_misc_fw_obj->kobj, g_misc_fw.misc_fw_number);
    return ret;
}

/* delete misc_fw[1-n] directory and attributes*/
static void misc_fw_sub_remove(void)
{
    unsigned int misc_fw_index;

    if (g_misc_fw.misc_fw) {
        for (misc_fw_index = g_misc_fw.misc_fw_number; misc_fw_index > 0; misc_fw_index--) {
            misc_fw_sub_single_remove_kobj_and_attrs(misc_fw_index);
        }
        kfree(g_misc_fw.misc_fw);
        g_misc_fw.misc_fw = NULL;
    }
    g_misc_fw.misc_fw_number = 0;
    return;
}

/* create misc_fw directory and number attributes */
static int misc_fw_root_create(void)
{
    g_misc_fw_obj = switch_kobject_create("misc_fw", NULL);
    if (!g_misc_fw_obj) {
        MISC_FW_ERR("switch_kobject_create misc_fw error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_misc_fw_obj->kobj, &misc_fw_root_attr_group) != 0) {
        switch_kobject_delete(&g_misc_fw_obj);
        MISC_FW_ERR("create misc_fw dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete misc_fw directory and number attributes */
static void misc_fw_root_remove(void)
{
    if (g_misc_fw_obj) {
        sysfs_remove_group(&g_misc_fw_obj->kobj, &misc_fw_root_attr_group);
        switch_kobject_delete(&g_misc_fw_obj);
    }

    return;
}

int s3ip_sysfs_misc_fw_drivers_register(struct s3ip_sysfs_misc_fw_drivers_s *drv)
{
    int ret, misc_fw_num;

    MISC_FW_INFO("s3ip_sysfs_misc_fw_drivers_register...\n");
    if (g_misc_fw_drv) {
        MISC_FW_ERR("g_misc_fw_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_misc_fw_number);
    g_misc_fw_drv = drv;

    misc_fw_num = g_misc_fw_drv->get_main_board_misc_fw_number();
    if (misc_fw_num <= 0) {
        MISC_FW_ERR("misc_fw number: %d, don't need to create misc_fw dirs and attrs.\n", misc_fw_num);
        g_misc_fw_drv = NULL;
        return -EINVAL;
    }

    mem_clear(&g_misc_fw, sizeof(struct misc_fw_s));
    g_misc_fw.misc_fw_number = misc_fw_num;
    ret = misc_fw_root_create();
    if (ret < 0) {
        MISC_FW_ERR("create misc_fw root dir and attrs failed, ret: %d\n", ret);
        g_misc_fw_drv = NULL;
        return ret;
    }
    ret = misc_fw_sub_create();
    if (ret < 0) {
        MISC_FW_ERR("create misc_fw sub dir and attrs failed, ret: %d\n", ret);
        misc_fw_root_remove();
        g_misc_fw_drv = NULL;
        return ret;
    }
    MISC_FW_INFO("s3ip_sysfs_misc_fw_drivers_register success\n");
    return 0;
}

void s3ip_sysfs_misc_fw_drivers_unregister(void)
{
    if (g_misc_fw_drv) {
        misc_fw_sub_remove();
        misc_fw_root_remove();
        g_misc_fw_drv = NULL;
        MISC_FW_DBG("s3ip_sysfs_misc_fw_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_misc_fw_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_misc_fw_drivers_unregister);
module_param(g_misc_fw_loglevel, int, 0644);
MODULE_PARM_DESC(g_misc_fw_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
