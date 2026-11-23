/*
 * An temp_sensor_sysfs driver for temperature sysfs devcie function
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
#include "temp_sensor_sysfs.h"

static int g_temp_sensor_loglevel = 0;

#define TEMP_SENSOR_INFO(fmt, args...) do {                                        \
    if (g_temp_sensor_loglevel & INFO) { \
        printk(KERN_INFO "[TEMP_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define TEMP_SENSOR_ERR(fmt, args...) do {                                        \
    if (g_temp_sensor_loglevel & ERR) { \
        printk(KERN_ERR "[TEMP_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define TEMP_SENSOR_DBG(fmt, args...) do {                                        \
    if (g_temp_sensor_loglevel & DBG) { \
        printk(KERN_DEBUG "[TEMP_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct temp_sensor_obj_s {
    struct switch_obj *obj;
};

struct temp_sensor_s {
    unsigned int temp_number;
    struct temp_sensor_obj_s *temp;
};

static struct s3ip_sysfs_temp_sensor_drivers_s *g_temp_sensor_drv = NULL;
static struct temp_sensor_s g_temp_sensor;
static struct switch_obj *g_temp_sensor_obj = NULL;

static ssize_t temp_sensor_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_temp_sensor.temp_number);
}

static ssize_t temp_sensor_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int temp_index;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->get_main_board_temp_value);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    return g_temp_sensor_drv->get_main_board_temp_value(temp_index, buf, PAGE_SIZE);
}

static ssize_t temp_sensor_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int temp_index;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->get_main_board_temp_alias);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    return g_temp_sensor_drv->get_main_board_temp_alias(temp_index, buf, PAGE_SIZE);
}

static ssize_t temp_sensor_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int temp_index;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->get_main_board_temp_type);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    return g_temp_sensor_drv->get_main_board_temp_type(temp_index, buf, PAGE_SIZE);
}

static ssize_t temp_sensor_max_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int temp_index;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->get_main_board_temp_max);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    return g_temp_sensor_drv->get_main_board_temp_max(temp_index, buf, PAGE_SIZE);
}

static ssize_t temp_sensor_max_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int temp_index;
    int ret;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->set_main_board_temp_max);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    ret = g_temp_sensor_drv->set_main_board_temp_max(temp_index, buf, count);
    if (ret < 0) {
        TEMP_SENSOR_ERR("set temp%u max threshold failed, value: %s, count: %lu, ret: %d\n",
            temp_index, buf, count, ret);
        return ret;
    }
    TEMP_SENSOR_DBG("set temp%u max threshold success, value: %s, count: %lu, ret: %d\n",
        temp_index, buf, count, ret);
    return count;
}

static ssize_t temp_sensor_min_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int temp_index;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->get_main_board_temp_min);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    return g_temp_sensor_drv->get_main_board_temp_min(temp_index, buf, PAGE_SIZE);
}

static ssize_t temp_sensor_min_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int temp_index;
    int ret;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->set_main_board_temp_min);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    ret = g_temp_sensor_drv->set_main_board_temp_min(temp_index, buf, count);
    if (ret < 0) {
        TEMP_SENSOR_ERR("set temp%u min threshold failed, value: %s, count: %lu, ret: %d\n",
            temp_index, buf, count, ret);
        return ret;
    }
    TEMP_SENSOR_DBG("set temp%u min threshold success, value: %s, count: %lu, ret: %d\n",
        temp_index, buf, count, ret);
    return count;
}

static ssize_t temp_sensor_high_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int temp_index;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->get_main_board_temp_high);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    return g_temp_sensor_drv->get_main_board_temp_high(temp_index, buf, PAGE_SIZE);
}

static ssize_t temp_sensor_high_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int temp_index;
    int ret;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->set_main_board_temp_high);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    ret = g_temp_sensor_drv->set_main_board_temp_high(temp_index, buf, count);
    if (ret < 0) {
        TEMP_SENSOR_ERR("set temp%u high threshold failed, value: %s, count: %lu, ret: %d\n",
            temp_index, buf, count, ret);
        return ret;
    }
    TEMP_SENSOR_DBG("set temp%u high threshold success, value: %s, count: %lu, ret: %d\n",
        temp_index, buf, count, ret);
    return count;
}

static ssize_t temp_sensor_low_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int temp_index;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->get_main_board_temp_low);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    return g_temp_sensor_drv->get_main_board_temp_low(temp_index, buf, PAGE_SIZE);
}

static ssize_t temp_sensor_low_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int temp_index;
    int ret;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->set_main_board_temp_low);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    ret = g_temp_sensor_drv->set_main_board_temp_low(temp_index, buf, count);
    if (ret < 0) {
        TEMP_SENSOR_ERR("set temp%u low threshold failed, value: %s, count: %lu, ret: %d\n",
            temp_index, buf, count, ret);
        return ret;
    }
    TEMP_SENSOR_DBG("set temp%u low threshold success, value: %s, count: %lu, ret: %d\n",
        temp_index, buf, count, ret);
    return count;
}

static ssize_t temp_sensor_monitor_flag_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int temp_index;

    check_p(g_temp_sensor_drv);
    check_p(g_temp_sensor_drv->get_main_board_temp_monitor_flag);

    temp_index = obj->index;
    TEMP_SENSOR_DBG("temp index: %u\n", temp_index);
    return g_temp_sensor_drv->get_main_board_temp_monitor_flag(temp_index, buf, PAGE_SIZE);
}

/************************************temp_sensor dir and attrs*******************************************/
static struct switch_attribute num_temp_att = __ATTR(number, S_IRUGO, temp_sensor_number_show, NULL);

static struct attribute *temp_sensor_dir_attrs[] = {
    &num_temp_att.attr,
    NULL,
};

static struct attribute_group temp_sensor_root_attr_group = {
    .attrs = temp_sensor_dir_attrs,
};

/*******************************temp1 temp2 dir and attrs*******************************************/
static struct switch_attribute temp_value_attr = __ATTR(value, S_IRUGO, temp_sensor_value_show, NULL);
static struct switch_attribute temp_alias_attr = __ATTR(alias, S_IRUGO, temp_sensor_alias_show, NULL);
static struct switch_attribute temp_type_attr = __ATTR(type, S_IRUGO, temp_sensor_type_show, NULL);
static struct switch_attribute temp_max_attr = __ATTR(max, S_IRUGO | S_IWUSR, temp_sensor_max_show, temp_sensor_max_store);
static struct switch_attribute temp_min_attr = __ATTR(min,  S_IRUGO | S_IWUSR, temp_sensor_min_show, temp_sensor_min_store);
static struct switch_attribute temp_high_attr = __ATTR(high, S_IRUGO | S_IWUSR, temp_sensor_high_show, temp_sensor_high_store);
static struct switch_attribute temp_low_attr = __ATTR(low,  S_IRUGO | S_IWUSR, temp_sensor_low_show, temp_sensor_low_store);
static struct switch_attribute temp_monitor_flag_attr = __ATTR(monitor_flag,  S_IRUGO, temp_sensor_monitor_flag_show, NULL);

static struct attribute *temp_sensor_attrs[] = {
    &temp_value_attr.attr,
    &temp_alias_attr.attr,
    &temp_type_attr.attr,
    &temp_max_attr.attr,
    &temp_min_attr.attr,
    &temp_high_attr.attr,
    &temp_low_attr.attr,
    &temp_monitor_flag_attr.attr,
    NULL,
};

static struct attribute_group temp_sensor_attr_group = {
    .attrs = temp_sensor_attrs,
};

static int temp_sensor_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct temp_sensor_obj_s *temp_sensor;

    temp_sensor = &g_temp_sensor.temp[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "temp%u", index);
    temp_sensor->obj = switch_kobject_create(name, parent);
    if (!temp_sensor->obj) {
        TEMP_SENSOR_ERR("create %s object error.\n", name);
        return -ENOMEM;
    }
    temp_sensor->obj->index = index;
    if (sysfs_create_group(&temp_sensor->obj->kobj, &temp_sensor_attr_group) != 0) {
        TEMP_SENSOR_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&temp_sensor->obj);
        return -EBADRQC;
    }
    TEMP_SENSOR_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

static void temp_sensor_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct temp_sensor_obj_s *temp_sensor;

    temp_sensor = &g_temp_sensor.temp[index - 1];
    if (temp_sensor->obj) {
        sysfs_remove_group(&temp_sensor->obj->kobj, &temp_sensor_attr_group);
        switch_kobject_delete(&temp_sensor->obj);
        TEMP_SENSOR_DBG("delete temp%u dir and attrs success.\n", index);
    }

    return;
}

static int temp_sensor_sub_create_kobj_and_attrs(struct kobject *parent, int temp_num)
{
    unsigned int temp_index, i;

    g_temp_sensor.temp = kzalloc(sizeof(struct temp_sensor_obj_s) * temp_num, GFP_KERNEL);
    if (!g_temp_sensor.temp) {
        TEMP_SENSOR_ERR("kzalloc g_temp_sensor.temp error, temp number: %d.\n", temp_num);
        return -ENOMEM;
    }

    for (temp_index = 1; temp_index <= temp_num; temp_index++) {
        if (temp_sensor_sub_single_create_kobj_and_attrs(parent, temp_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = temp_index; i > 0; i--) {
        temp_sensor_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_temp_sensor.temp);
    g_temp_sensor.temp = NULL;
    return -EBADRQC;
}

/* create temp[1-n] directory and attributes*/
static int temp_sensor_sub_create(void)
{
    int ret;

    ret = temp_sensor_sub_create_kobj_and_attrs(&g_temp_sensor_obj->kobj,
              g_temp_sensor.temp_number);
    return ret;
}

/* delete temp[1-n] directory and attributes*/
static void temp_sensor_sub_remove(void)
{
    unsigned int temp_index;

    if (g_temp_sensor.temp) {
        for (temp_index = g_temp_sensor.temp_number; temp_index > 0; temp_index--) {
            temp_sensor_sub_single_remove_kobj_and_attrs(temp_index);
        }
        kfree(g_temp_sensor.temp);
        g_temp_sensor.temp = NULL;
    }

    return;
}

/* create temp_sensor directory and number attributes */
static int temp_sensor_root_create(void)
{
    g_temp_sensor_obj = switch_kobject_create("temp_sensor", NULL);
    if (!g_temp_sensor_obj) {
        TEMP_SENSOR_ERR("switch_kobject_create temp_sensor error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_temp_sensor_obj->kobj, &temp_sensor_root_attr_group) != 0) {
        switch_kobject_delete(&g_temp_sensor_obj);
        TEMP_SENSOR_ERR("create temp_sensor dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete temp_sensor directory and number attributes */
static void temp_sensor_root_remove(void)
{
    if (g_temp_sensor_obj) {
        sysfs_remove_group(&g_temp_sensor_obj->kobj, &temp_sensor_root_attr_group);
        switch_kobject_delete(&g_temp_sensor_obj);
    }

    return;
}

int s3ip_sysfs_temp_sensor_drivers_register(struct s3ip_sysfs_temp_sensor_drivers_s *drv)
{
    int ret, temp_num;

    TEMP_SENSOR_INFO("s3ip_sysfs_temp_sensor_drivers_register...\n");
    if (g_temp_sensor_drv) {
        TEMP_SENSOR_ERR("g_temp_sensor_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_temp_number);
    g_temp_sensor_drv = drv;

    temp_num = g_temp_sensor_drv->get_main_board_temp_number();
    if (temp_num <= 0) {
        TEMP_SENSOR_ERR("temp sensor number: %d, don't need to create temp_sensor dirs and attrs.\n",
            temp_num);
        g_temp_sensor_drv = NULL;
        return -EINVAL;
    }
    mem_clear(&g_temp_sensor, sizeof(struct temp_sensor_s));
    g_temp_sensor.temp_number = temp_num;
    ret = temp_sensor_root_create();
    if (ret < 0) {
        TEMP_SENSOR_ERR("create temp_sensor root dir and attrs failed, ret: %d\n", ret);
        g_temp_sensor_drv = NULL;
        return ret;
    }

    ret = temp_sensor_sub_create();
    if (ret < 0) {
        TEMP_SENSOR_ERR("create temp_sensor sub dir and attrs failed, ret: %d\n", ret);
        temp_sensor_root_remove();
        g_temp_sensor_drv = NULL;
        return ret;
    }
    TEMP_SENSOR_INFO("s3ip_sysfs_temp_sensor_drivers_register success\n");
    return ret;
}

void s3ip_sysfs_temp_sensor_drivers_unregister(void)
{
    if (g_temp_sensor_drv) {
        temp_sensor_sub_remove();
        temp_sensor_root_remove();
        g_temp_sensor_drv = NULL;
        TEMP_SENSOR_DBG("s3ip_sysfs_temp_sensor_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_temp_sensor_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_temp_sensor_drivers_unregister);
module_param(g_temp_sensor_loglevel, int, 0644);
MODULE_PARM_DESC(g_temp_sensor_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
