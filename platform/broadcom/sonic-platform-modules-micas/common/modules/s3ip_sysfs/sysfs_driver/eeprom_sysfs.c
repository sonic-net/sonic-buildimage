/*
 * An eeprom_sysfs driver for eeprom sysfs devcie function
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
#include "eeprom_sysfs.h"

static int g_eeprom_loglevel = 0;

#define EEPROM_INFO(fmt, args...) do {                                        \
    if (g_eeprom_loglevel & INFO) { \
        printk(KERN_INFO "[EEPROM_SYSFS][func:%s line:%d]"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define EEPROM_ERR(fmt, args...) do {                                        \
    if (g_eeprom_loglevel & ERR) { \
        printk(KERN_ERR "[EEPROM_SYSFS][func:%s line:%d]"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define EEPROM_DBG(fmt, args...) do {                                        \
    if (g_eeprom_loglevel & DBG) { \
        printk(KERN_DEBUG "[EEPROM_SYSFS][func:%s line:%d]"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct eeprom_obj_s {
    struct switch_obj *eeprom_obj;
    struct bin_attribute bin;
    int eeprom_creat_bin_flag;
};

struct eeprom_s {
    unsigned int eeprom_number;
    struct eeprom_obj_s *eeprom;
};

static struct eeprom_s g_eeprom;
static struct switch_obj *g_eeprom_obj = NULL;
static struct s3ip_sysfs_eeprom_drivers_s *g_eeprom_drv = NULL;

static ssize_t eeprom_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_eeprom.eeprom_number);
}

static ssize_t eeprom_size_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    struct eeprom_obj_s *curr_eeprom;

    EEPROM_DBG("get eeprom size, eeprom index: %u\n", obj->index);
    curr_eeprom = &g_eeprom.eeprom[obj->index - 1];
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%ld\n", curr_eeprom->bin.size);
}

static ssize_t eeprom_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eeprom_index;

    check_p(g_eeprom_drv);
    check_p(g_eeprom_drv->get_eeprom_alias);

    eeprom_index = obj->index;
    EEPROM_DBG("get eeprom alias, eeprom index: %u\n", eeprom_index);
    return g_eeprom_drv->get_eeprom_alias(eeprom_index, buf, PAGE_SIZE);
}

static ssize_t eeprom_tag_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eeprom_index;

    check_p(g_eeprom_drv);
    check_p(g_eeprom_drv->get_eeprom_tag);

    eeprom_index = obj->index;
    EEPROM_DBG("get eeprom tag, eeprom index: %u\n", eeprom_index);
    return g_eeprom_drv->get_eeprom_tag(eeprom_index, buf, PAGE_SIZE);
}

static ssize_t eeprom_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int eeprom_index;

    check_p(g_eeprom_drv);
    check_p(g_eeprom_drv->get_eeprom_type);

    eeprom_index = obj->index;
    EEPROM_DBG("get eeprom type, eeprom index: %u\n", eeprom_index);
    return g_eeprom_drv->get_eeprom_type(eeprom_index, buf, PAGE_SIZE);
}

static ssize_t eeprom_eeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                   char *buf, loff_t offset, size_t count)
{
    struct switch_obj *eeprom_obj;
    ssize_t rd_len;
    unsigned int eeprom_index;

    check_p(g_eeprom_drv);
    check_p(g_eeprom_drv->read_eeprom_data);

    eeprom_obj = to_switch_obj(kobj);
    eeprom_index = eeprom_obj->index;
    mem_clear(buf, count);
    rd_len = g_eeprom_drv->read_eeprom_data(eeprom_index, buf, offset, count);
    if (rd_len < 0) {
        EEPROM_ERR("read eeprom%u eeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
            eeprom_index, offset, count, rd_len);
        return rd_len;
    }

    EEPROM_DBG("read eeprom%u eeprom data success, offset:0x%llx, read len:%lu, really read len:%ld.\n",
        eeprom_index, offset, count, rd_len);

    return rd_len;
}

static ssize_t eeprom_eeprom_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                   char *buf, loff_t offset, size_t count)
{
    struct switch_obj *eeprom_obj;
    ssize_t wr_len;
    unsigned int eeprom_index;

    check_p(g_eeprom_drv);
    check_p(g_eeprom_drv->write_eeprom_data);

    eeprom_obj = to_switch_obj(kobj);
    eeprom_index = eeprom_obj->index;
    wr_len = g_eeprom_drv->write_eeprom_data(eeprom_index, buf, offset, count);
    if (wr_len < 0) {
        EEPROM_ERR("write eeprom%u eeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
            eeprom_index, offset, count, wr_len);
        return wr_len;
    }

    EEPROM_DBG("write eeprom%u eeprom data success, offset:0x%llx, write len:%lu, really write len:%ld.\n",
        eeprom_index, offset, count, wr_len);

    return wr_len;
}

/************************************eeprom* signal attrs*******************************************/
static struct switch_attribute eeprom_alias_attr = __ATTR(alias, S_IRUGO | S_IWUSR, eeprom_alias_show, NULL);
static struct switch_attribute eeprom_tag_attr = __ATTR(tag, S_IRUGO | S_IWUSR, eeprom_tag_show, NULL);
static struct switch_attribute eeprom_size_attr = __ATTR(size, S_IRUGO, eeprom_size_show, NULL);
static struct switch_attribute eeprom_type_attr = __ATTR(type, S_IRUGO | S_IWUSR, eeprom_type_show, NULL);

static struct attribute *eeprom_signal_attrs[] = {
    &eeprom_alias_attr.attr,
    &eeprom_tag_attr.attr,
    &eeprom_size_attr.attr,
    &eeprom_type_attr.attr,
    NULL,
};

static struct attribute_group eeprom_signal_attr_group = {
    .attrs = eeprom_signal_attrs,
};

/*******************************eeprom dir and attrs*******************************************/
static struct switch_attribute eeprom_number_attr = __ATTR(number, S_IRUGO, eeprom_number_show, NULL);

static struct attribute *eeprom_dir_attrs[] = {
    &eeprom_number_attr.attr,
    NULL,
};

static struct attribute_group eeprom_eeprom_attr_group = {
    .attrs = eeprom_dir_attrs,
};

/* create eeprom* eeprom attributes */
static int eeprom_sub_single_create_eeprom_attrs(unsigned int index)
{
    int ret, eeprom_size;
    struct eeprom_obj_s *curr_eeprom;

    check_p(g_eeprom_drv->get_eeprom_size);
    eeprom_size = g_eeprom_drv->get_eeprom_size(index);
    if (eeprom_size <= 0) {
        EEPROM_ERR("Invalid eeprom size, eeprom index: %u, eeprom_size: %d\n",
                 index, eeprom_size);
        return -EINVAL;
    }

    curr_eeprom = &g_eeprom.eeprom[index - 1];
    sysfs_bin_attr_init(&curr_eeprom->bin);
    curr_eeprom->bin.attr.name = "data";
    curr_eeprom->bin.attr.mode = 0644;
    curr_eeprom->bin.read = eeprom_eeprom_read;
    curr_eeprom->bin.write = eeprom_eeprom_write;
    curr_eeprom->bin.size = eeprom_size;

    ret = sysfs_create_bin_file(&curr_eeprom->eeprom_obj->kobj, &curr_eeprom->bin);
    if (ret) {
        EEPROM_ERR("eeprom%u, create eeprom bin error, ret: %d. \n", index, ret);
        return -EBADRQC;
    }

    EEPROM_DBG("eeprom%u, create bin file success, eeprom size: %d.\n", index, eeprom_size);
    curr_eeprom->eeprom_creat_bin_flag = 1;
    return 0;
}

static int eeprom_sub_single_create_kobj(struct kobject *parent, unsigned int index)
{
    struct eeprom_obj_s *curr_eeprom;
    char eeprom_dir_name[DIR_NAME_MAX_LEN];

    curr_eeprom = &g_eeprom.eeprom[index - 1];
    mem_clear(eeprom_dir_name, sizeof(eeprom_dir_name));
    snprintf(eeprom_dir_name, sizeof(eeprom_dir_name), "eeprom%d", index);
    curr_eeprom->eeprom_obj = switch_kobject_create(eeprom_dir_name, parent);
    if (!curr_eeprom->eeprom_obj) {
        EEPROM_ERR("create eeprom%d object error! \n", index);
        return -EBADRQC;
    }
    curr_eeprom->eeprom_obj->index = index;
    if (sysfs_create_group(&curr_eeprom->eeprom_obj->kobj, &eeprom_signal_attr_group) != 0) {
        switch_kobject_delete(&curr_eeprom->eeprom_obj);
        return -EBADRQC;
    }

    EEPROM_DBG("create eeprom%d dir and attrs success\n", index);
    return 0;
}

/* remove eeprom directory and attributes */
static void eeprom_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct eeprom_obj_s *curr_eeprom;

    curr_eeprom = &g_eeprom.eeprom[index - 1];
    if (curr_eeprom->eeprom_obj) {
        if (curr_eeprom->eeprom_creat_bin_flag) {
            sysfs_remove_bin_file(&curr_eeprom->eeprom_obj->kobj, &curr_eeprom->bin);
            curr_eeprom->eeprom_creat_bin_flag = 0;
        }
        sysfs_remove_group(&curr_eeprom->eeprom_obj->kobj, &eeprom_signal_attr_group);
        switch_kobject_delete(&curr_eeprom->eeprom_obj);
    }

    return;
}

static int eeprom_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    int ret;

    ret = eeprom_sub_single_create_kobj(parent, index);
    if (ret < 0) {
        EEPROM_ERR("create eeprom%d dir error.\n", index);
        return ret;
    }

    ret = eeprom_sub_single_create_eeprom_attrs(index);
    if (ret < 0) {
        eeprom_sub_single_remove_kobj_and_attrs(index);
        EEPROM_ERR("create eeprom%d data error.\n", index);
        return ret;
    }
    return 0;
}

static int eeprom_sub_create_kobj_and_attrs(struct kobject *parent, int eeprom_num)
{
    unsigned int eeprom_index, i;

    g_eeprom.eeprom = kzalloc(sizeof(struct eeprom_obj_s) * eeprom_num, GFP_KERNEL);
    if (!g_eeprom.eeprom) {
        EEPROM_ERR("kzalloc g_eeprom.eeprom error, eeprom number = %d.\n", eeprom_num);
        return -ENOMEM;
    }

    for (eeprom_index = 1; eeprom_index <= eeprom_num; eeprom_index++) {
        if (eeprom_sub_single_create_kobj_and_attrs(parent, eeprom_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = eeprom_index; i > 0; i--) {
        eeprom_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_eeprom.eeprom);
    g_eeprom.eeprom = NULL;
    return -EBADRQC;
}

/* create eeprom directory and attributes */
static int eeprom_sub_create(void)
{
    int ret;

    ret = eeprom_sub_create_kobj_and_attrs(&g_eeprom_obj->kobj, g_eeprom.eeprom_number);
    return ret;
}

/* delete eeprom directory and attributes */
static void eeprom_sub_remove(void)
{
    unsigned int eeprom_index;

    if (g_eeprom.eeprom) {
        for (eeprom_index = g_eeprom.eeprom_number; eeprom_index > 0; eeprom_index--) {
            eeprom_sub_single_remove_kobj_and_attrs(eeprom_index);
        }
        kfree(g_eeprom.eeprom);
        g_eeprom.eeprom = NULL;
    }
    g_eeprom.eeprom_number = 0;
    return;
}

/* create eeprom directory and attributes */
static int eeprom_eeprom_create(void)
{
    g_eeprom_obj = switch_kobject_create("eeprom", NULL);
    if (!g_eeprom_obj) {
        EEPROM_ERR("switch_kobject_create eeprom error!\n");
        return -ENOMEM;
    }
    g_eeprom_obj->index = 0;
    if (sysfs_create_group(&g_eeprom_obj->kobj, &eeprom_eeprom_attr_group) != 0) {
        switch_kobject_delete(&g_eeprom_obj);
        EEPROM_ERR("create eeprom dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete eeprom directory and attributes */
static void eeprom_eeprom_remove(void)
{
    if (g_eeprom_obj) {
        sysfs_remove_group(&g_eeprom_obj->kobj, &eeprom_eeprom_attr_group);
        switch_kobject_delete(&g_eeprom_obj);
    }

    return;
}

int s3ip_sysfs_eeprom_drivers_register(struct s3ip_sysfs_eeprom_drivers_s *drv)
{
    int ret, eeprom_num;

    EEPROM_INFO("s3ip_sysfs_eeprom_drivers_register...\n");
    if (g_eeprom_drv) {
        EEPROM_ERR("g_eeprom_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_eeprom_number);
    g_eeprom_drv = drv;

    eeprom_num = g_eeprom_drv->get_eeprom_number();
    if (eeprom_num <= 0) {
        EEPROM_ERR("eeprom number: %d, don't need to create eeprom dirs and attrs.\n", eeprom_num);
        g_eeprom_drv = NULL;
        return -EINVAL;
    }

    mem_clear(&g_eeprom, sizeof(struct eeprom_s));
    g_eeprom.eeprom_number = eeprom_num;
    ret = eeprom_eeprom_create();
    if (ret < 0) {
        EEPROM_ERR("create eeprom root dir and attrs failed, ret: %d\n", ret);
        g_eeprom_drv = NULL;
        return ret;
    }
    ret = eeprom_sub_create();
    if (ret < 0) {
        EEPROM_ERR("create eeprom sub dir and attrs failed, ret: %d\n", ret);
        eeprom_eeprom_remove();
        g_eeprom_drv = NULL;
        return ret;
    }
    EEPROM_INFO("s3ip_sysfs_eeprom_drivers_register success\n");
    return ret;
}

void s3ip_sysfs_eeprom_drivers_unregister(void)
{
    if (g_eeprom_drv) {
        eeprom_sub_remove();
        eeprom_eeprom_remove();
        g_eeprom_drv = NULL;
        EEPROM_DBG("s3ip_sysfs_eeprom_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_eeprom_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_eeprom_drivers_unregister);
module_param(g_eeprom_loglevel, int, 0644);
MODULE_PARM_DESC(g_eeprom_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
