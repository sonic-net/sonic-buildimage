/*
 * An vol_sensor_sysfs driver for voltage sysfs devcie function
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
#include "vol_sensor_sysfs.h"

static int g_vol_sensor_loglevel = 0;

#define VOL_SENSOR_INFO(fmt, args...) do {                                        \
    if (g_vol_sensor_loglevel & INFO) { \
        printk(KERN_INFO "[VOL_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define VOL_SENSOR_ERR(fmt, args...) do {                                        \
    if (g_vol_sensor_loglevel & ERR) { \
        printk(KERN_ERR "[VOL_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define VOL_SENSOR_DBG(fmt, args...) do {                                        \
    if (g_vol_sensor_loglevel & DBG) { \
        printk(KERN_DEBUG "[VOL_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct vol_sensor_obj_s {
    struct switch_obj *obj;
};

struct vol_sensor_s {
    unsigned int vol_number;
    struct vol_sensor_obj_s *vol;
};

static struct s3ip_sysfs_vol_sensor_drivers_s *g_vol_sensor_drv = NULL;
static struct vol_sensor_s g_vol_sensor;
static struct switch_obj *g_vol_sensor_obj = NULL;

static ssize_t vol_sensor_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_vol_sensor.vol_number);
}

static ssize_t vol_sensor_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int vol_index;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->get_main_board_vol_value);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    return g_vol_sensor_drv->get_main_board_vol_value(vol_index, buf, PAGE_SIZE);
}

static ssize_t vol_sensor_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int vol_index;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->get_main_board_vol_alias);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    return g_vol_sensor_drv->get_main_board_vol_alias(vol_index, buf, PAGE_SIZE);
}

static ssize_t vol_sensor_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int vol_index;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->get_main_board_vol_type);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    return g_vol_sensor_drv->get_main_board_vol_type(vol_index, buf, PAGE_SIZE);
}

static ssize_t vol_sensor_max_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int vol_index;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->get_main_board_vol_max);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    return g_vol_sensor_drv->get_main_board_vol_max(vol_index, buf, PAGE_SIZE);
}

static ssize_t vol_sensor_max_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int vol_index;
    int ret;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->set_main_board_vol_max);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    ret = g_vol_sensor_drv->set_main_board_vol_max(vol_index, buf, count);
    if (ret < 0) {
        VOL_SENSOR_ERR("set vol%u max threshold failed, value: %s, count: %lu, ret: %d\n",
            vol_index, buf, count, ret);
        return ret;
    }
    VOL_SENSOR_DBG("set vol%u max threshold success, value: %s, count: %lu, ret: %d\n",
        vol_index, buf, count, ret);
    return count;
}

static ssize_t vol_sensor_min_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int vol_index;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->get_main_board_vol_min);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    return g_vol_sensor_drv->get_main_board_vol_min(vol_index, buf, PAGE_SIZE);
}

static ssize_t vol_sensor_min_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int vol_index;
    int ret;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->set_main_board_vol_min);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    ret = g_vol_sensor_drv->set_main_board_vol_min(vol_index, buf, count);
    if (ret < 0) {
        VOL_SENSOR_ERR("set vol%u min threshold failed, value: %s, count: %lu, ret: %d\n",
            vol_index, buf, count, ret);
        return ret;
    }
    VOL_SENSOR_DBG("set vol%u min threshold success, value: %s, count: %lu, ret: %d\n",
        vol_index, buf, count, ret);
    return count;
}

static ssize_t vol_sensor_range_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int vol_index;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->get_main_board_vol_range);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    return g_vol_sensor_drv->get_main_board_vol_range(vol_index, buf, PAGE_SIZE);
}

static ssize_t vol_sensor_nominal_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int vol_index;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->get_main_board_vol_nominal_value);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    return g_vol_sensor_drv->get_main_board_vol_nominal_value(vol_index, buf, PAGE_SIZE);
}

static ssize_t vol_sensor_monitor_flag_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int vol_index;

    check_p(g_vol_sensor_drv);
    check_p(g_vol_sensor_drv->get_main_board_vol_monitor_flag);

    vol_index = obj->index;
    VOL_SENSOR_DBG("vol index: %u\n", vol_index);
    return g_vol_sensor_drv->get_main_board_vol_monitor_flag(vol_index, buf, PAGE_SIZE);
}

/************************************vol_sensor dir and attrs*******************************************/
static struct switch_attribute num_vol_att = __ATTR(number, S_IRUGO, vol_sensor_number_show, NULL);

static struct attribute *vol_sensor_dir_attrs[] = {
    &num_vol_att.attr,
    NULL,
};

static struct attribute_group vol_sensor_root_attr_group = {
    .attrs = vol_sensor_dir_attrs,
};

/*******************************vol1 vol2 dir and attrs*******************************************/
static struct switch_attribute vol_value_attr = __ATTR(value, S_IRUGO, vol_sensor_value_show, NULL);
static struct switch_attribute vol_alias_attr = __ATTR(alias, S_IRUGO, vol_sensor_alias_show, NULL);
static struct switch_attribute vol_type_attr = __ATTR(type, S_IRUGO, vol_sensor_type_show, NULL);
static struct switch_attribute vol_max_attr = __ATTR(max, S_IRUGO | S_IWUSR, vol_sensor_max_show, vol_sensor_max_store);
static struct switch_attribute vol_min_attr = __ATTR(min,  S_IRUGO | S_IWUSR, vol_sensor_min_show, vol_sensor_min_store);
static struct switch_attribute vol_range_attr = __ATTR(range, S_IRUGO, vol_sensor_range_show, NULL);
static struct switch_attribute vol_nominal_value_attr = __ATTR(nominal_value, S_IRUGO, vol_sensor_nominal_value_show, NULL);
static struct switch_attribute vol_monitor_flag_attr = __ATTR(monitor_flag,  S_IRUGO, vol_sensor_monitor_flag_show, NULL);

static struct attribute *vol_sensor_attrs[] = {
    &vol_value_attr.attr,
    &vol_alias_attr.attr,
    &vol_type_attr.attr,
    &vol_max_attr.attr,
    &vol_min_attr.attr,
    &vol_range_attr.attr,
    &vol_nominal_value_attr.attr,
    &vol_monitor_flag_attr.attr,
    NULL,
};

static struct attribute_group vol_sensor_attr_group = {
    .attrs = vol_sensor_attrs,
};

static int vol_sensor_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct vol_sensor_obj_s *vol_sensor;

    vol_sensor = &g_vol_sensor.vol[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "vol%u", index);
    vol_sensor->obj = switch_kobject_create(name, parent);
    if (!vol_sensor->obj) {
        VOL_SENSOR_ERR("create %s object error.\n", name);
        return -ENOMEM;
    }

    vol_sensor->obj->index = index;
    if (sysfs_create_group(&vol_sensor->obj->kobj, &vol_sensor_attr_group) != 0) {
        VOL_SENSOR_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&vol_sensor->obj);
        return -EBADRQC;
    }
    VOL_SENSOR_DBG("create %s dir and attrs success.\n", name);

    return 0;
}

static void vol_sensor_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct vol_sensor_obj_s *vol_sensor;

    vol_sensor = &g_vol_sensor.vol[index - 1];
    if (vol_sensor->obj) {
        sysfs_remove_group(&vol_sensor->obj->kobj, &vol_sensor_attr_group);
        switch_kobject_delete(&vol_sensor->obj);
        VOL_SENSOR_DBG("delete vol%u dir and attrs success.\n", index);
    }

    return;
}

static int vol_sensor_sub_create_kobj_and_attrs(struct kobject *parent, int vol_num)
{
    unsigned int vol_index, i;

    g_vol_sensor.vol = kzalloc(sizeof(struct vol_sensor_obj_s) * vol_num, GFP_KERNEL);
    if (!g_vol_sensor.vol) {
        VOL_SENSOR_ERR("kzalloc g_vol_sensor.vol error, vol number: %d.\n", vol_num);
        return -ENOMEM;
    }

    for (vol_index = 1; vol_index <= vol_num; vol_index++) {
        if (vol_sensor_sub_single_create_kobj_and_attrs(parent, vol_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = vol_index; i > 0; i--) {
        vol_sensor_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_vol_sensor.vol);
    g_vol_sensor.vol = NULL;
    return -EBADRQC;
}

/* create vol[1-n] directory and attributes*/
static int vol_sensor_sub_create(void)
{
    int ret;

    ret = vol_sensor_sub_create_kobj_and_attrs(&g_vol_sensor_obj->kobj, g_vol_sensor.vol_number);
    return ret;
}

/* delete vol[1-n] directory and attributes*/
static void vol_sensor_sub_remove(void)
{
    unsigned int vol_index;

    if (g_vol_sensor.vol) {
        for (vol_index = g_vol_sensor.vol_number; vol_index > 0; vol_index--) {
            vol_sensor_sub_single_remove_kobj_and_attrs(vol_index);
        }
        kfree(g_vol_sensor.vol);
        g_vol_sensor.vol = NULL;
    }

    return;
}

/* create vol_sensor directory and number attributes */
static int vol_sensor_root_create(void)
{
    g_vol_sensor_obj = switch_kobject_create("vol_sensor", NULL);
    if (!g_vol_sensor_obj) {
        VOL_SENSOR_ERR("switch_kobject_create vol_sensor error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_vol_sensor_obj->kobj, &vol_sensor_root_attr_group) != 0) {
        switch_kobject_delete(&g_vol_sensor_obj);
        VOL_SENSOR_ERR("create vol_sensor dir attrs error!\n");
        return -EBADRQC;
    }

    return 0;
}

/* delete vol_sensor directory and number attributes */
static void vol_sensor_root_remove(void)
{
    if (g_vol_sensor_obj) {
        sysfs_remove_group(&g_vol_sensor_obj->kobj, &vol_sensor_root_attr_group);
        switch_kobject_delete(&g_vol_sensor_obj);
    }

    return;
}

int s3ip_sysfs_vol_sensor_drivers_register(struct s3ip_sysfs_vol_sensor_drivers_s *drv)
{
    int ret, vol_num;

    VOL_SENSOR_INFO("s3ip_sysfs_vol_sensor_drivers_register...\n");
    if (g_vol_sensor_drv) {
        VOL_SENSOR_ERR("g_vol_sensor_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_vol_number);
    g_vol_sensor_drv = drv;

    vol_num = g_vol_sensor_drv->get_main_board_vol_number();
    if (vol_num <= 0) {
        VOL_SENSOR_ERR("vol sensor number: %d, don't need to create vol_sensor dirs and attrs.\n",
            vol_num);
        g_vol_sensor_drv = NULL;
        return -EINVAL;
    }
    mem_clear(&g_vol_sensor, sizeof(struct vol_sensor_s));
    g_vol_sensor.vol_number = vol_num;
    ret = vol_sensor_root_create();
    if (ret < 0) {
        VOL_SENSOR_ERR("create vol_sensor root dir and attrs failed, ret: %d\n", ret);
        g_vol_sensor_drv = NULL;
        return ret;
    }

    ret = vol_sensor_sub_create();
    if (ret < 0) {
        VOL_SENSOR_ERR("create vol_sensor sub dir and attrs failed, ret: %d\n", ret);
        vol_sensor_root_remove();
        g_vol_sensor_drv = NULL;
        return ret;
    }
    VOL_SENSOR_INFO("s3ip_sysfs_vol_sensor_drivers_register success\n");
    return ret;
}

void s3ip_sysfs_vol_sensor_drivers_unregister(void)
{
    if (g_vol_sensor_drv) {
        vol_sensor_sub_remove();
        vol_sensor_root_remove();
        g_vol_sensor_drv = NULL;
        VOL_SENSOR_DBG("s3ip_sysfs_vol_sensor_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_vol_sensor_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_vol_sensor_drivers_unregister);
module_param(g_vol_sensor_loglevel, int, 0644);
MODULE_PARM_DESC(g_vol_sensor_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
