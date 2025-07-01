/*
 * An fan_sysfs driver for fan sysfs devcie function
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
#include "fan_sysfs.h"

static int g_fan_loglevel = 0;
static bool g_fan_status_debug  = 0;

#define FAN_INFO(fmt, args...) do {                                        \
    if (g_fan_loglevel & INFO) { \
        printk(KERN_INFO "[FAN_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FAN_ERR(fmt, args...) do {                                        \
    if (g_fan_loglevel & ERR) { \
        printk(KERN_ERR "[FAN_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FAN_DBG(fmt, args...) do {                                        \
    if (g_fan_loglevel & DBG) { \
        printk(KERN_DEBUG "[FAN_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct motor_obj_s {
    struct switch_obj *obj;
};

struct fan_obj_s {
    unsigned int motor_number;
    struct motor_obj_s *motor;
    struct switch_obj *obj;
};

struct fan_s {
    unsigned int fan_number;
    struct fan_obj_s *fan;
};

static struct fan_s g_fan;
static struct switch_obj *g_fan_obj = NULL;
static struct s3ip_sysfs_fan_drivers_s *g_fan_drv = NULL;

static ssize_t fan_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_fan.fan_number);
}

static ssize_t fan_motor_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int index;

    index = obj->index;
    FAN_DBG("fan_motor_number_show, fan index: %u\n", index);

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_fan.fan[index - 1].motor_number);
}

static ssize_t fan_model_name_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_model_name);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_model_name(fan_index, buf, PAGE_SIZE);
}

static ssize_t fan_vendor_show(struct switch_obj *obj, struct switch_attribute *attr,
                               char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_vendor);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_vendor(fan_index, buf, PAGE_SIZE);
}

static ssize_t fan_sn_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_serial_number);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_serial_number(fan_index, buf, PAGE_SIZE);
}

static ssize_t fan_pn_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_part_number);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_part_number(fan_index, buf, PAGE_SIZE);
}

static ssize_t fan_hw_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_hardware_version);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_hardware_version(fan_index, buf, PAGE_SIZE);
}

static ssize_t fan_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index;
    int ret, res;
    char debug_file_buf[DEBUG_FILE_SIZE];

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_status);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    ret = g_fan_drv->get_fan_status(fan_index, buf, PAGE_SIZE);
    if (ret < 0) {
        FAN_ERR("get fan%u status failed, ret: %d\n", fan_index, ret);
        return ret;
    }

    if (g_fan_status_debug) {
        FAN_INFO("s3ip sysfs fan status debug is enable\n");

        if ((strncmp(buf, SWITCH_DEV_NO_SUPPORT, strlen(SWITCH_DEV_NO_SUPPORT)) == 0) || (strncmp(buf, SWITCH_DEV_ERROR, strlen(SWITCH_DEV_ERROR)) == 0)) {
            FAN_DBG("fan%d status sysfs unsupport or error\n", fan_index);
            return ret;
        }

        if (strcmp(buf, FAN_ABSENT_STR) == 0) {
            FAN_DBG("fan%d absent, return act value\n", fan_index);
            return ret;
        }

        mem_clear(debug_file_buf, sizeof(debug_file_buf));
        res = dev_debug_file_read(SINGLE_FAN_STATUS_DEBUG_FILE, fan_index, debug_file_buf, sizeof(debug_file_buf));
        if (res) {
            FAN_ERR("fan%u status debug file read failed, ret: %d\n", fan_index, res);
            return ret;
        }

        if ((strcmp(debug_file_buf, FAN_ABSENT_STR) == 0) || (strcmp(debug_file_buf, FAN_OK_STR) == 0) || (strcmp(debug_file_buf, FAN_NOTOK_STR) == 0)) {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s", debug_file_buf);
        } else {
            FAN_ERR("fan%d status debug file value err, value: %s, not 0 1 or 2\n", fan_index, debug_file_buf);
            return ret;
        }
    }
    return ret;
}

static ssize_t fan_present_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_present);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_present(fan_index, buf, PAGE_SIZE);
}

static ssize_t fan_led_status_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_led_status);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_led_status(fan_index, buf, PAGE_SIZE);
}

static ssize_t fan_led_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    unsigned int fan_index;
    int ret, led_status;

    check_p(g_fan_drv);
    check_p(g_fan_drv->set_fan_led_status);

    fan_index = obj->index;
    ret = kstrtoint(buf, 0, &led_status);
    if (ret != 0) {
        FAN_ERR("invaild fan led status ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }
    FAN_DBG("fan index: %u, led_status: %d\n", fan_index, led_status);
    ret = g_fan_drv->set_fan_led_status(fan_index, led_status);
    if (ret < 0) {
        FAN_ERR("set fan%u led_status: %d failed, ret: %d\n", fan_index, led_status, ret);
        return ret;
    }
    FAN_DBG("set fan%u led_status: %d success\n", fan_index, led_status);
    return count;
}

static ssize_t fan_motor_status_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_motor_status);

    p_obj = to_switch_obj(obj->kobj.parent);
    fan_index = p_obj->index;
    motor_index = obj->index;
    FAN_DBG("fan index: %u, motor index: %d\n", fan_index, motor_index);
    return g_fan_drv->get_fan_motor_status(fan_index, motor_index, buf, PAGE_SIZE);
}

static ssize_t fan_motor_speed_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_motor_speed);

    p_obj = to_switch_obj(obj->kobj.parent);
    fan_index = p_obj->index;
    motor_index = obj->index;
    FAN_DBG("fan index: %u, motor index: %d\n", fan_index, motor_index);
    return g_fan_drv->get_fan_motor_speed(fan_index, motor_index, buf, PAGE_SIZE);
}

static ssize_t fan_motor_speed_tolerance_show(struct switch_obj *obj,
                   struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_motor_speed_tolerance);

    p_obj = to_switch_obj(obj->kobj.parent);
    fan_index = p_obj->index;
    motor_index = obj->index;
    FAN_DBG("fan index: %u, motor index: %d\n", fan_index, motor_index);
    return g_fan_drv->get_fan_motor_speed_tolerance(fan_index, motor_index, buf, PAGE_SIZE);
}

static ssize_t fan_motor_speed_target_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_motor_speed_target);

    p_obj = to_switch_obj(obj->kobj.parent);
    fan_index = p_obj->index;
    motor_index = obj->index;
    FAN_DBG("fan index: %u, motor index: %d\n", fan_index, motor_index);
    return g_fan_drv->get_fan_motor_speed_target(fan_index, motor_index, buf, PAGE_SIZE);
}

static ssize_t fan_motor_speed_max_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_motor_speed_max);

    p_obj = to_switch_obj(obj->kobj.parent);
    fan_index = p_obj->index;
    motor_index = obj->index;
    FAN_DBG("fan index: %u, motor index: %d\n", fan_index, motor_index);
    return g_fan_drv->get_fan_motor_speed_max(fan_index, motor_index, buf, PAGE_SIZE);
}

static ssize_t fan_motor_speed_min_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_motor_speed_min);

    p_obj = to_switch_obj(obj->kobj.parent);
    fan_index = p_obj->index;
    motor_index = obj->index;
    FAN_DBG("fan index: %u, motor index: %d\n", fan_index, motor_index);
    return g_fan_drv->get_fan_motor_speed_min(fan_index, motor_index, buf, PAGE_SIZE);
}

ssize_t fan_ratio_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_ratio);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_ratio(fan_index, buf, PAGE_SIZE);
}

static ssize_t fan_ratio_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int fan_index;
    int ret, ratio;

    check_p(g_fan_drv);
    check_p(g_fan_drv->set_fan_ratio);

    fan_index = obj->index;

    ret = kstrtoint(buf, 0, &ratio);
    if (ret != 0) {
        FAN_ERR("invaild fan ratio ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (ratio < 0 || ratio > 100) {
        FAN_ERR("param invalid, can not set ratio: %d.\n", ratio);
        return -EINVAL;
    }
    FAN_DBG("fan index: %u, ratio: %d\n", fan_index, ratio);
    ret = g_fan_drv->set_fan_ratio(fan_index, ratio);
    if (ret < 0) {
        FAN_ERR("set fan%u  ratio: %d failed, ret: %d\n", fan_index, ratio, ret);
        return ret;
    }
    FAN_DBG("set fan%u, ratio: %d success\n", fan_index, ratio);
    return count;
}

ssize_t fan_direction_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_direction);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_direction(fan_index, buf, PAGE_SIZE);
}

/************************************fan dir and attrs*******************************************/
static struct switch_attribute fan_number_att = __ATTR(number, S_IRUGO, fan_number_show, NULL);

static struct attribute *fan_dir_attrs[] = {
    &fan_number_att.attr,
    NULL,
};

static struct attribute_group fan_root_attr_group = {
    .attrs = fan_dir_attrs,
};

/*******************************fan1 fan2 dir and attrs*******************************************/
static struct switch_attribute fan_model_name_attr = __ATTR(model_name, S_IRUGO, fan_model_name_show, NULL);
static struct switch_attribute fan_vendor_attr = __ATTR(vendor, S_IRUGO, fan_vendor_show, NULL);
static struct switch_attribute fan_sn_attr = __ATTR(serial_number, S_IRUGO, fan_sn_show, NULL);
static struct switch_attribute fan_pn_attr = __ATTR(part_number, S_IRUGO, fan_pn_show, NULL);
static struct switch_attribute fan_hw_attr = __ATTR(hardware_version, S_IRUGO, fan_hw_show, NULL);
static struct switch_attribute fan_num_motors_attr = __ATTR(motor_number, S_IRUGO, fan_motor_number_show, NULL);
static struct switch_attribute fan_status_attr = __ATTR(status, S_IRUGO, fan_status_show, NULL);
static struct switch_attribute fan_present_attr = __ATTR(present, S_IRUGO, fan_present_show, NULL);
static struct switch_attribute fan_led_status_attr = __ATTR(led_status, S_IRUGO | S_IWUSR, fan_led_status_show, fan_led_status_store);
static struct switch_attribute fan_direction_attr = __ATTR(direction, S_IRUGO, fan_direction_show, NULL);
static struct switch_attribute fan_ratio_attr = __ATTR(ratio, S_IRUGO | S_IWUSR, fan_ratio_show, fan_ratio_store);

static struct attribute *fan_attrs[] = {
    &fan_model_name_attr.attr,
    &fan_vendor_attr.attr,
    &fan_sn_attr.attr,
    &fan_pn_attr.attr,
    &fan_hw_attr.attr,
    &fan_num_motors_attr.attr,
    &fan_status_attr.attr,
    &fan_present_attr.attr,
    &fan_led_status_attr.attr,
    &fan_direction_attr.attr,
    &fan_ratio_attr.attr,
    NULL,
};

static struct attribute_group fan_attr_group = {
    .attrs = fan_attrs,
};

/*******************************motor1 motor2 dir and attrs*******************************************/
static struct switch_attribute motor_status_attr = __ATTR(status, S_IRUGO, fan_motor_status_show, NULL);
static struct switch_attribute motor_speed_attr = __ATTR(speed, S_IRUGO, fan_motor_speed_show, NULL);
static struct switch_attribute motor_speed_tolerance_attr = __ATTR(speed_tolerance, S_IRUGO, fan_motor_speed_tolerance_show, NULL);
static struct switch_attribute motor_speed_target_attr = __ATTR(speed_target, S_IRUGO, fan_motor_speed_target_show, NULL);
static struct switch_attribute motor_speed_max_attr = __ATTR(speed_max, S_IRUGO, fan_motor_speed_max_show, NULL);
static struct switch_attribute motor_speed_min_attr = __ATTR(speed_min, S_IRUGO, fan_motor_speed_min_show, NULL);

static struct attribute *motor_attrs[] = {
    &motor_status_attr.attr,
    &motor_speed_attr.attr,
    &motor_speed_tolerance_attr.attr,
    &motor_speed_target_attr.attr,
    &motor_speed_max_attr.attr,
    &motor_speed_min_attr.attr,
    NULL,
};

static struct attribute_group motor_attr_group = {
    .attrs = motor_attrs,
};

static void fanindex_single_motor_remove_kobj_and_attrs(struct fan_obj_s *curr_fan, unsigned int motor_index)
{
    struct motor_obj_s *curr_motor; /* point to motor1 motor2...*/

    curr_motor = &curr_fan->motor[motor_index - 1];
    if (curr_motor->obj) {
        sysfs_remove_group(&curr_motor->obj->kobj, &motor_attr_group);
        switch_kobject_delete(&curr_motor->obj);
        FAN_DBG("delete fan%u motor%u dir and attrs success.\n", curr_fan->obj->index,
            motor_index);
    }
    return;
}

static int fanindex_single_motor_create_kobj_and_attrs(struct fan_obj_s *curr_fan, unsigned int motor_index)
{
    char name[8];
    struct motor_obj_s *curr_motor; /* point to motor1 motor2...*/

    curr_motor = &curr_fan->motor[motor_index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "motor%u", motor_index);
    curr_motor->obj = switch_kobject_create(name, &curr_fan->obj->kobj);
    if (!curr_motor->obj) {
        FAN_ERR("create fan%u, motor%u object error!\n", curr_fan->obj->index, motor_index);
        return -ENOMEM;
    }

    curr_motor->obj->index = motor_index;
    if (sysfs_create_group(&curr_motor->obj->kobj, &motor_attr_group) != 0) {
        FAN_ERR("create fan%u, motor%u attrs error.\n", curr_fan->obj->index, motor_index);
        switch_kobject_delete(&curr_motor->obj);
        return -EBADRQC;
    }
    FAN_DBG("create fan%u, motor%u dir and attrs success.\n", curr_fan->obj->index, motor_index);
    return 0;
}

/* create motor[1-n] directory and attributes in fan directory */
static int fanindex_motor_create_kobj_and_attrs(struct fan_obj_s *curr_fan)
{
    unsigned int motor_index, i, motor_num;

    motor_num = curr_fan->motor_number;
    curr_fan->motor = kzalloc(sizeof(struct motor_obj_s) * motor_num, GFP_KERNEL);
    if (!curr_fan->motor) {
        FAN_ERR("kzalloc motor error, fan index: %u, motor number: %d.\n",
            curr_fan->obj->index, motor_num);
        return -ENOMEM;
    }
    for (motor_index = 1; motor_index <= motor_num; motor_index++) {
        if (fanindex_single_motor_create_kobj_and_attrs(curr_fan, motor_index) != 0) {
            goto motor_error;
        }
    }
    return 0;
motor_error:
    for (i = motor_index; i > 0; i--) {
        fanindex_single_motor_remove_kobj_and_attrs(curr_fan, i);
    }
    kfree(curr_fan->motor);
    curr_fan->motor = NULL;
    return -EBADRQC;
}

/* delete motor[1-n] directory and attributes in fan directory */
static void fanindex_motor_remove_kobj_and_attrs(struct fan_obj_s *curr_fan)
{
    unsigned int motor_index, motor_num;

    if (curr_fan->motor) {
        motor_num = curr_fan->motor_number;
        for (motor_index = motor_num; motor_index > 0; motor_index--) {
            fanindex_single_motor_remove_kobj_and_attrs(curr_fan, motor_index);
        }
        kfree(curr_fan->motor);
        curr_fan->motor = NULL;
    }

    return;
}

/* create motor[1-n] directory and attributes */
static int fan_motor_create(void)
{
    int fan_num, motor_num;
    unsigned int fan_index, i;
    struct fan_obj_s *curr_fan;     /* point to fan1 fan2...*/

    fan_num = g_fan.fan_number;
    if (fan_num <= 0) {
        FAN_DBG("fan number: %d, skip to create motor* dirs and attrs.\n", fan_num);
        return 0;
    }

    check_p(g_fan_drv->get_fan_motor_number);

    for (fan_index = 1; fan_index <= fan_num; fan_index++) {
        motor_num = g_fan_drv->get_fan_motor_number(fan_index);
        if (motor_num <= 0) {
            FAN_DBG("fan%u motor number: %d, don't need to create motor* dirs and attrs.\n",
                fan_index, motor_num);
            continue;
        }
        curr_fan = &g_fan.fan[fan_index - 1];
        curr_fan->motor_number = motor_num;
        if (fanindex_motor_create_kobj_and_attrs(curr_fan) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = fan_index; i > 0; i--) {
        curr_fan = &g_fan.fan[i - 1];
        fanindex_motor_remove_kobj_and_attrs(curr_fan);
    }
    return -EBADRQC;
}

/* delete motor[1-n] directory and attributes */
static void fan_motor_remove(void)
{
    unsigned int fan_index;
    struct fan_obj_s *curr_fan;

    if (g_fan.fan) {
        for (fan_index = g_fan.fan_number; fan_index > 0; fan_index--) {
            curr_fan = &g_fan.fan[fan_index - 1];
            fanindex_motor_remove_kobj_and_attrs(curr_fan);
            curr_fan->motor_number = 0;
        }
    }
    return;
}

static int fan_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct fan_obj_s *curr_fan;

    curr_fan = &g_fan.fan[index - 1];
    if (curr_fan->obj) {
        sysfs_remove_group(&curr_fan->obj->kobj, &fan_attr_group);
        switch_kobject_delete(&curr_fan->obj);
        FAN_DBG("delete fan%u dir and attrs success.\n", index);
    }
    return 0;
}

static int fan_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[8];
    struct fan_obj_s *curr_fan;

    curr_fan = &g_fan.fan[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "fan%u", index);
    curr_fan->obj = switch_kobject_create(name, parent);
    if (!curr_fan->obj) {
        FAN_ERR("create %s object error!\n", name);
        return -ENOMEM;
    }

    curr_fan->obj->index = index;
    if (sysfs_create_group(&curr_fan->obj->kobj, &fan_attr_group) != 0) {
        FAN_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&curr_fan->obj);
        return -EBADRQC;
    }
    FAN_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

/* create fan[1-n] directory and attributes */
static int fan_sub_create_kobj_and_attrs(struct kobject *parent, int fan_num)
{
    unsigned int fan_index, i;

    g_fan.fan = kzalloc(sizeof(struct fan_obj_s) * fan_num, GFP_KERNEL);
    if (!g_fan.fan) {
        FAN_ERR("kzalloc fan.fan error, fan number: %d.\n", fan_num);
        return -ENOMEM;
    }

    for (fan_index = 1; fan_index <= fan_num; fan_index++) {
        if (fan_sub_single_create_kobj_and_attrs(parent, fan_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = fan_index; i > 0; i--) {
        fan_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_fan.fan);
    g_fan.fan = NULL;
    return -EBADRQC;
}

static int fan_sub_create(void)
{
    int ret;

    ret = fan_sub_create_kobj_and_attrs(&g_fan_obj->kobj, g_fan.fan_number);
    return ret;
}

/* delete fan[1-n] directory and attributes */
static void fan_sub_remove(void)
{
    unsigned int fan_index;

    if (g_fan.fan) {
        for (fan_index = g_fan.fan_number; fan_index > 0; fan_index--) {
            fan_sub_single_remove_kobj_and_attrs(fan_index);
        }
        kfree(g_fan.fan);
        g_fan.fan = NULL;
    }
    g_fan.fan_number = 0;

    return;
}

/* create fan directory and number attributes */
static int fan_root_create(void)
{
    g_fan_obj = switch_kobject_create("fan", NULL);
    if (!g_fan_obj) {
        FAN_ERR("switch_kobject_create fan error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_fan_obj->kobj, &fan_root_attr_group) != 0) {
        switch_kobject_delete(&g_fan_obj);
        FAN_ERR("create fan dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete fan directory and number attributes */
static void fan_root_remove(void)
{
    if (g_fan_obj) {
        sysfs_remove_group(&g_fan_obj->kobj, &fan_root_attr_group);
        switch_kobject_delete(&g_fan_obj);
        FAN_DBG("delete fan dir and attrs success.\n");
    }
    return;
}

int s3ip_sysfs_fan_drivers_register(struct s3ip_sysfs_fan_drivers_s *drv)
{
    int ret, fan_num;

    FAN_INFO("s3ip_sysfs_fan_drivers_register...\n");
    if (g_fan_drv) {
        FAN_ERR("g_fan_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_fan_number);
    g_fan_drv = drv;

    fan_num = g_fan_drv->get_fan_number();
    if (fan_num <= 0) {
        FAN_ERR("fan number: %d, don't need to create fan dirs and attrs.\n", fan_num);
        g_fan_drv = NULL;
        return -EINVAL;
    }

    mem_clear(&g_fan, sizeof(struct fan_s));
    g_fan.fan_number = fan_num;
    ret = fan_root_create();
    if (ret < 0) {
        FAN_ERR("create fan root dir and attrs failed, ret: %d\n", ret);
        g_fan_drv = NULL;
        return ret;
    }

    ret = fan_sub_create();
    if (ret < 0) {
        FAN_ERR("create fan sub dir and attrs failed, ret: %d\n", ret);
        fan_root_remove();
        g_fan_drv = NULL;
        return ret;
    }

    ret = fan_motor_create();
    if (ret < 0) {
        FAN_ERR("create fan motor dir and attrs failed, ret: %d\n", ret);
        fan_sub_remove();
        fan_root_remove();
        g_fan_drv = NULL;
        return ret;
    }
    FAN_INFO("s3ip_sysfs_fan_drivers_register success.\n");
    return 0;
}

void s3ip_sysfs_fan_drivers_unregister(void)
{
    if (g_fan_drv) {
        fan_motor_remove();
        fan_sub_remove();
        fan_root_remove();
        g_fan_drv = NULL;
        FAN_DBG("s3ip_sysfs_fan_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_fan_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_fan_drivers_unregister);
module_param(g_fan_loglevel, int, 0644);
MODULE_PARM_DESC(g_fan_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
module_param(g_fan_status_debug, bool, 0644);
MODULE_PARM_DESC(g_fan_status_debug, "the fan present debug switch(0: disable, 1:enable, defalut: 0).\n");
