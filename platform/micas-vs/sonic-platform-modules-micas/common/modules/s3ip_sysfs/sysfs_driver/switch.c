/*
 * An switch driver for switch devcie function
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

#include "switch.h"
#include "syseeprom_sysfs.h"

int g_switch_loglevel = 0;

#define SWITCH_INFO(fmt, args...) do {                                        \
    if (g_switch_loglevel & INFO) { \
        printk(KERN_INFO "[SWITCH][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SWITCH_ERR(fmt, args...) do {                                        \
    if (g_switch_loglevel & ERR) { \
        printk(KERN_ERR "[SWITCH][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SWITCH_DBG(fmt, args...) do {                                        \
    if (g_switch_loglevel & DBG) { \
        printk(KERN_DEBUG "[SWITCH][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct syseeprom_s {
    struct bin_attribute bin;
    int creat_eeprom_bin_flag;
};

static struct s3ip_sysfs_syseeprom_drivers_s *g_syseeprom_drv = NULL;
static struct kset *switch_kset;
static struct syseeprom_s g_syseeprom;

static ssize_t switch_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct switch_attribute *attribute;
    struct switch_obj *device;

    attribute = to_switch_attr(attr);
    device = to_switch_obj(kobj);

    if (!attribute->show) {
        return -ENOSYS;
    }

    return attribute->show(device, attribute, buf);
}

static ssize_t switch_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf,
                   size_t len)
{
    struct switch_attribute *attribute;
    struct switch_obj *obj;

    attribute = to_switch_attr(attr);
    obj = to_switch_obj(kobj);

    if (!attribute->store) {
        return -ENOSYS;
    }

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
    return;
}

static struct kobj_type switch_ktype = {
    .sysfs_ops = &switch_sysfs_ops,
    .release = switch_obj_release,
};

static ssize_t syseeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                   char *buf, loff_t offset, size_t count)
{
    ssize_t rd_len;

    check_p(g_syseeprom_drv);
    check_p(g_syseeprom_drv->read_syseeprom_data);

    mem_clear(buf, count);
    rd_len = g_syseeprom_drv->read_syseeprom_data(buf, offset, count);
    if (rd_len < 0) {
        SWITCH_ERR("read syseeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
            offset, count, rd_len);
        return rd_len;
    }
    SWITCH_DBG("read syseeprom data success, offset:0x%llx, read len:%lu, really read len:%ld.\n",
        offset, count, rd_len);
    return rd_len;
}

static ssize_t syseeprom_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                   char *buf, loff_t offset, size_t count)
{
    ssize_t wr_len;

    check_p(g_syseeprom_drv);
    check_p(g_syseeprom_drv->write_syseeprom_data);

    wr_len = g_syseeprom_drv->write_syseeprom_data(buf, offset, count);
    if (wr_len < 0) {
        SWITCH_ERR("write syseeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
            offset, count, wr_len);
        return wr_len;
    }
    SWITCH_DBG("write syseeprom data success, offset:0x%llx, write len:%lu, really write len:%ld.\n",
        offset, count, wr_len);
    return wr_len;
}

static int syseeprom_create_eeprom_attrs(void)
{
    int ret, eeprom_size;

    eeprom_size = g_syseeprom_drv->get_syseeprom_size();
    if (eeprom_size <= 0) {
        SWITCH_ERR("syseeprom size: %d, invalid.\n", eeprom_size);
        return -EINVAL;
    }

    sysfs_bin_attr_init(&g_syseeprom.bin);
    g_syseeprom.bin.attr.name = "syseeprom";
    g_syseeprom.bin.attr.mode = 0644;
    g_syseeprom.bin.read = syseeprom_read;
    g_syseeprom.bin.write = syseeprom_write;
    g_syseeprom.bin.size = eeprom_size;

    ret = sysfs_create_bin_file(&switch_kset->kobj, &g_syseeprom.bin);
    if (ret) {
        SWITCH_ERR("create syseeprom bin error, ret: %d. \n", ret);
        return -EBADRQC;
    }
    SWITCH_DBG("create syseeprom bin file success, eeprom size:%d.\n", eeprom_size);
    g_syseeprom.creat_eeprom_bin_flag = 1;
    return 0;
}

static void syseeprom_remove_eeprom_attrs(void)
{
    if (g_syseeprom.creat_eeprom_bin_flag) {
        sysfs_remove_bin_file(&switch_kset->kobj, &g_syseeprom.bin);
        g_syseeprom.creat_eeprom_bin_flag = 0;
    }

    return;
}

int dev_debug_file_read(char *file_name, unsigned int dev_index, char *buf, int size)
{
    char file_path[DIR_NAME_MAX_LEN];
    loff_t pos;
    struct file *filp;
    int ret;

    mem_clear(file_path, sizeof(file_path));
    mem_clear(buf, size);

    sprintf(file_path, file_name, dev_index);
    filp = filp_open(file_path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        SWITCH_ERR("dev_debug_file open failed, path=%s, ret=%d\n", file_path, ret);
        filp = NULL;
        ret = -ENOENT;
        return ret;
    }

    pos = 0;
    ret = kernel_read(filp, buf, size - 1, &pos);
    if (ret < 0) {
        SWITCH_ERR("dev_debug_file kernel_read failed, path=%s, addr=0, size=%d, ret=%d\n", file_name, size - 1, ret);
        filp_close(filp, NULL);
        return ret;
    }

    filp_close(filp, NULL);
    filp = NULL;
    return 0;
}

int s3ip_sysfs_syseeprom_drivers_register(struct s3ip_sysfs_syseeprom_drivers_s *drv)
{
    int ret;

    SWITCH_INFO("s3ip_sysfs_syseeprom_drivers_register...\n");
    if (g_syseeprom_drv) {
        SWITCH_ERR("g_syseeprom_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_syseeprom_size);
    g_syseeprom_drv = drv;

    ret = syseeprom_create_eeprom_attrs();
    if (ret < 0) {
        SWITCH_ERR("create syseeprom attributes failed, ret: %d\n", ret);
        g_syseeprom_drv = NULL;
        return ret;
    }
    SWITCH_INFO("s3ip_sysfs_syseeprom_drivers_register success.\n");
    return 0;
}

void s3ip_sysfs_syseeprom_drivers_unregister(void)
{
    if (g_syseeprom_drv) {
        syseeprom_remove_eeprom_attrs();
        g_syseeprom_drv = NULL;
        SWITCH_DBG("s3ip_sysfs_syseeprom_drivers_unregister success.\n");
    }

    return;
}

struct switch_obj *switch_kobject_create(const char *name, struct kobject *parent)
{
    struct switch_obj *obj;
    int ret;

    obj = kzalloc(sizeof(*obj), GFP_KERNEL);
    if (!obj) {
        SWITCH_DBG("switch_kobject_create %s kzalloc error", name);
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

void switch_kobject_delete(struct switch_obj **obj)
{
    if (*obj) {
        SWITCH_DBG("%s delete %s.\n", (*obj)->kobj.parent->name, (*obj)->kobj.name);
        kobject_put(&((*obj)->kobj));
        *obj = NULL;
    }
}

static int __init switch_init(void)
{
    SWITCH_INFO("switch_init...\n");

    switch_kset = kset_create_and_add("s3ip", NULL, NULL);
    if (!switch_kset) {
        SWITCH_ERR("create switch_kset error.\n");
        return -ENOMEM;
    }

    SWITCH_INFO("switch_init success.\n");
    return 0;
}

static void __exit switch_exit(void)
{
    if (switch_kset) {
        kset_unregister(switch_kset);
    }

    SWITCH_INFO("switch_exit success.\n");
}

module_init(switch_init);
module_exit(switch_exit);
EXPORT_SYMBOL(s3ip_sysfs_syseeprom_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_syseeprom_drivers_unregister);
module_param(g_switch_loglevel, int, 0644);
MODULE_PARM_DESC(g_switch_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("switch driver");
