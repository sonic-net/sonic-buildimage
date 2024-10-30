/*
 * An curr_sensor_sysfs driver for current sysfs devcie function
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
#include "curr_sensor_sysfs.h"

static int g_curr_sensor_loglevel = 0;

#define CURR_SENSOR_INFO(fmt, args...) do {                                        \
    if (g_curr_sensor_loglevel & INFO) { \
        printk(KERN_INFO "[CURR_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define CURR_SENSOR_ERR(fmt, args...) do {                                        \
    if (g_curr_sensor_loglevel & ERR) { \
        printk(KERN_ERR "[CURR_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define CURR_SENSOR_DBG(fmt, args...) do {                                        \
    if (g_curr_sensor_loglevel & DBG) { \
        printk(KERN_DEBUG "[CURR_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct curr_sensor_obj_s {
    struct switch_obj *obj;
};

struct curr_sensor_s {
    unsigned int curr_number;
    struct curr_sensor_obj_s *curr;
};

static struct s3ip_sysfs_curr_sensor_drivers_s *g_curr_sensor_drv = NULL;
static struct curr_sensor_s g_curr_sensor;
static struct switch_obj *g_curr_sensor_obj = NULL;

static ssize_t curr_sensor_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_curr_sensor.curr_number);
}

static ssize_t curr_sensor_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int curr_index;

    check_p(g_curr_sensor_drv);
    check_p(g_curr_sensor_drv->get_main_board_curr_value);

    curr_index = obj->index;
    CURR_SENSOR_DBG("curr index: %u\n", curr_index);
    return g_curr_sensor_drv->get_main_board_curr_value(curr_index, buf, PAGE_SIZE);
}

static ssize_t curr_sensor_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int curr_index;

    check_p(g_curr_sensor_drv);
    check_p(g_curr_sensor_drv->get_main_board_curr_alias);

    curr_index = obj->index;
    CURR_SENSOR_DBG("curr index: %u\n", curr_index);
    return g_curr_sensor_drv->get_main_board_curr_alias(curr_index, buf, PAGE_SIZE);
}

static ssize_t curr_sensor_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int curr_index;

    check_p(g_curr_sensor_drv);
    check_p(g_curr_sensor_drv->get_main_board_curr_type);

    curr_index = obj->index;
    CURR_SENSOR_DBG("curr index: %u\n", curr_index);
    return g_curr_sensor_drv->get_main_board_curr_type(curr_index, buf, PAGE_SIZE);
}

static ssize_t curr_sensor_max_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int curr_index;

    check_p(g_curr_sensor_drv);
    check_p(g_curr_sensor_drv->get_main_board_curr_max);

    curr_index = obj->index;
    CURR_SENSOR_DBG("curr index: %u\n", curr_index);
    return g_curr_sensor_drv->get_main_board_curr_max(curr_index, buf, PAGE_SIZE);
}

static ssize_t curr_sensor_max_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int curr_index;
    int ret;

    check_p(g_curr_sensor_drv);
    check_p(g_curr_sensor_drv->set_main_board_curr_max);

    curr_index = obj->index;
    CURR_SENSOR_DBG("curr index: %u\n", curr_index);
    ret = g_curr_sensor_drv->set_main_board_curr_max(curr_index, buf, count);
    if (ret < 0) {
        CURR_SENSOR_ERR("set curr%u max threshold failed, value: %s, count: %lu, ret: %d\n",
            curr_index, buf, count, ret);
        return ret;
    }
    CURR_SENSOR_DBG("set curr%u max threshold success, value: %s, count: %lu, ret: %d\n",
        curr_index, buf, count, ret);
    return count;
}

static ssize_t curr_sensor_min_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int curr_index;

    check_p(g_curr_sensor_drv);
    check_p(g_curr_sensor_drv->get_main_board_curr_min);

    curr_index = obj->index;
    CURR_SENSOR_DBG("curr index: %u\n", curr_index);
    return g_curr_sensor_drv->get_main_board_curr_min(curr_index, buf, PAGE_SIZE);
}

static ssize_t curr_sensor_min_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int curr_index;
    int ret;

    check_p(g_curr_sensor_drv);
    check_p(g_curr_sensor_drv->set_main_board_curr_min);

    curr_index = obj->index;
    CURR_SENSOR_DBG("curr index: %u\n", curr_index);
    ret = g_curr_sensor_drv->set_main_board_curr_min(curr_index, buf, count);
    if (ret < 0) {
        CURR_SENSOR_ERR("set curr%u min threshold failed, value: %s, count: %lu, ret: %d\n",
            curr_index, buf, count, ret);
        return ret;
    }
    CURR_SENSOR_DBG("set curr%u min threshold success, value: %s, count: %lu, ret: %d\n",
        curr_index, buf, count, ret);
    return count;
}

static ssize_t curr_sensor_monitor_flag_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int curr_index;

    check_p(g_curr_sensor_drv);
    check_p(g_curr_sensor_drv->get_main_board_curr_monitor_flag);

    curr_index = obj->index;
    CURR_SENSOR_DBG("curr index: %u\n", curr_index);
    return g_curr_sensor_drv->get_main_board_curr_monitor_flag(curr_index, buf, PAGE_SIZE);
}
/************************************curr_sensor dir and attrs*******************************************/
static struct switch_attribute num_curr_att = __ATTR(number, S_IRUGO, curr_sensor_number_show, NULL);

static struct attribute *curr_sensor_dir_attrs[] = {
    &num_curr_att.attr,
    NULL,
};

static struct attribute_group curr_sensor_root_attr_group = {
    .attrs = curr_sensor_dir_attrs,
};

/*******************************curr1 curr2 dir and attrs*******************************************/
static struct switch_attribute curr_value_attr = __ATTR(value, S_IRUGO, curr_sensor_value_show, NULL);
static struct switch_attribute curr_alias_attr = __ATTR(alias, S_IRUGO, curr_sensor_alias_show, NULL);
static struct switch_attribute curr_type_attr = __ATTR(type, S_IRUGO, curr_sensor_type_show, NULL);
static struct switch_attribute curr_max_attr = __ATTR(max, S_IRUGO | S_IWUSR, curr_sensor_max_show, curr_sensor_max_store);
static struct switch_attribute curr_min_attr = __ATTR(min,  S_IRUGO | S_IWUSR, curr_sensor_min_show, curr_sensor_min_store);
static struct switch_attribute curr_monitor_flag_attr = __ATTR(monitor_flag,  S_IRUGO, curr_sensor_monitor_flag_show, NULL);

static struct attribute *curr_sensor_attrs[] = {
    &curr_value_attr.attr,
    &curr_alias_attr.attr,
    &curr_type_attr.attr,
    &curr_max_attr.attr,
    &curr_min_attr.attr,
    &curr_monitor_flag_attr.attr,
    NULL,
};

static struct attribute_group curr_sensor_attr_group = {
    .attrs = curr_sensor_attrs,
};

static int curr_sensor_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct curr_sensor_obj_s *curr_sensor;

    curr_sensor = &g_curr_sensor.curr[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "curr%u", index);
    curr_sensor->obj = switch_kobject_create(name, parent);
    if (!curr_sensor->obj) {
        CURR_SENSOR_ERR("create %s object error.\n", name);
        return -ENOMEM;
    }
    curr_sensor->obj->index = index;
    if (sysfs_create_group(&curr_sensor->obj->kobj, &curr_sensor_attr_group) != 0) {
        CURR_SENSOR_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&curr_sensor->obj);
        return -EBADRQC;
    }
    CURR_SENSOR_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

static void curr_sensor_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct curr_sensor_obj_s *curr_sensor;

    curr_sensor = &g_curr_sensor.curr[index - 1];
    if (curr_sensor->obj) {
        sysfs_remove_group(&curr_sensor->obj->kobj, &curr_sensor_attr_group);
        switch_kobject_delete(&curr_sensor->obj);
        CURR_SENSOR_DBG("delete curr%u dir and attrs success.\n", index);
    }

    return;
}

static int curr_sensor_sub_create_kobj_and_attrs(struct kobject *parent, int curr_num)
{
    unsigned int curr_index, i;

    g_curr_sensor.curr = kzalloc(sizeof(struct curr_sensor_obj_s) * curr_num, GFP_KERNEL);
    if (!g_curr_sensor.curr) {
        CURR_SENSOR_ERR("kzalloc g_curr_sensor.curr error, curr number: %d.\n", curr_num);
        return -ENOMEM;
    }

    for (curr_index = 1; curr_index <= curr_num; curr_index++) {
        if (curr_sensor_sub_single_create_kobj_and_attrs(parent, curr_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = curr_index; i > 0; i--) {
        curr_sensor_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_curr_sensor.curr);
    g_curr_sensor.curr = NULL;
    return -EBADRQC;
}

/* create curr[1-n] directory and attributes*/
static int curr_sensor_sub_create(void)
{
    int ret;

    ret = curr_sensor_sub_create_kobj_and_attrs(&g_curr_sensor_obj->kobj,
              g_curr_sensor.curr_number);
    return ret;
}

/* delete curr[1-n] directory and attributes*/
static void curr_sensor_sub_remove(void)
{
    unsigned int curr_index;

    if (g_curr_sensor.curr) {
        for (curr_index = g_curr_sensor.curr_number; curr_index > 0; curr_index--) {
            curr_sensor_sub_single_remove_kobj_and_attrs(curr_index);
        }
        kfree(g_curr_sensor.curr);
        g_curr_sensor.curr = NULL;
    }
    g_curr_sensor.curr_number = 0;
    return;
}

/* create curr_sensor directory and number attributes */
static int curr_sensor_root_create(void)
{
    g_curr_sensor_obj = switch_kobject_create("curr_sensor", NULL);
    if (!g_curr_sensor_obj) {
        CURR_SENSOR_ERR("switch_kobject_create curr_sensor error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_curr_sensor_obj->kobj, &curr_sensor_root_attr_group) != 0) {
        switch_kobject_delete(&g_curr_sensor_obj);
        CURR_SENSOR_ERR("create curr_sensor dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete curr_sensor directory and number attributes */
static void curr_sensor_root_remove(void)
{
    if (g_curr_sensor_obj) {
        sysfs_remove_group(&g_curr_sensor_obj->kobj, &curr_sensor_root_attr_group);
        switch_kobject_delete(&g_curr_sensor_obj);
    }

    return;
}

int s3ip_sysfs_curr_sensor_drivers_register(struct s3ip_sysfs_curr_sensor_drivers_s *drv)
{
    int ret, curr_num;

    CURR_SENSOR_INFO("s3ip_sysfs_curr_sensor_drivers_register...\n");
    if (g_curr_sensor_drv) {
        CURR_SENSOR_ERR("g_curr_sensor_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_curr_number);
    g_curr_sensor_drv = drv;

    curr_num = g_curr_sensor_drv->get_main_board_curr_number();
    if (curr_num <= 0) {
        CURR_SENSOR_ERR("curr sensor number: %d, don't need to create curr_sensor dirs and attrs.\n",
            curr_num);
        g_curr_sensor_drv = NULL;
        return -EINVAL;
    }
    mem_clear(&g_curr_sensor, sizeof(struct curr_sensor_s));
    g_curr_sensor.curr_number = curr_num;
    ret = curr_sensor_root_create();
    if (ret < 0) {
        CURR_SENSOR_ERR("create curr_sensor root dir and attrs failed, ret: %d\n", ret);
        g_curr_sensor_drv = NULL;
        return ret;
    }

    ret = curr_sensor_sub_create();
    if (ret < 0) {
        CURR_SENSOR_ERR("create curr_sensor sub dir and attrs failed, ret: %d\n", ret);
        curr_sensor_root_remove();
        g_curr_sensor_drv = NULL;
        return ret;
    }
    CURR_SENSOR_INFO("s3ip_sysfs_curr_sensor_drivers_register success\n");
    return ret;
}

void s3ip_sysfs_curr_sensor_drivers_unregister(void)
{
    if (g_curr_sensor_drv) {
        curr_sensor_sub_remove();
        curr_sensor_root_remove();
        g_curr_sensor_drv = NULL;
        CURR_SENSOR_DBG("s3ip_sysfs_curr_sensor_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_curr_sensor_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_curr_sensor_drivers_unregister);
module_param(g_curr_sensor_loglevel, int, 0644);
MODULE_PARM_DESC(g_curr_sensor_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
