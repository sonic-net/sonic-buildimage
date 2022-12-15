/*
 * psu_sysfs.c
 *
 * This module create psu kobjects and attributes in /sys/s3ip/psu
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "psu_sysfs.h"

static int g_psu_loglevel = 0;

#define PSU_INFO(fmt, args...) do {                                        \
    if (g_psu_loglevel & INFO) { \
        printk(KERN_INFO "[PSU_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PSU_ERR(fmt, args...) do {                                        \
    if (g_psu_loglevel & ERR) { \
        printk(KERN_ERR "[PSU_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PSU_DBG(fmt, args...) do {                                        \
    if (g_psu_loglevel & DBG) { \
        printk(KERN_DEBUG "[PSU_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct temp_obj_s {
    struct switch_obj *obj;
};

struct psu_obj_s {
    unsigned int temp_number;
    struct temp_obj_s *temp;
    struct switch_obj *obj;
};

struct psu_s{
    unsigned int psu_number;
    struct psu_obj_s *psu;
};

static struct psu_s g_psu;
static struct switch_obj *g_psu_obj = NULL;
static struct s3ip_sysfs_psu_drivers_s *g_psu_drv = NULL;

static ssize_t psu_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_psu.psu_number);
}

static ssize_t psu_temp_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;

    index = obj->index;
    PSU_DBG("psu index: %u\n",index);
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_psu.psu[index - 1].temp_number);
}

static ssize_t psu_model_name_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_model_name);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_model_name(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u model name failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_hw_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_hardware_version);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_hardware_version(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u hardware version failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_sn_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_serial_number);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_serial_number(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u serial number failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_pn_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_part_number);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_part_number(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u part number failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_type);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_type(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u type failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_in_curr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_in_curr);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_in_curr(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u input current failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_in_vol_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_in_vol);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_in_vol(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u input voltage failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_in_power_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_in_power);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_in_power(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u input power failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_out_curr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_out_curr);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_out_curr(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u output current failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_out_vol_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_out_vol);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_out_vol(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u output voltage failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_out_power_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_out_power);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_out_power(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u outout power failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_out_max_power_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_out_max_power);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_out_max_power(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u outout max power failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_present_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_present_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_present_status(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u present status failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_out_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_out_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_out_status(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u outout status failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_in_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_in_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_in_status(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u input status failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_fan_speed_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_fan_speed);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_fan_speed(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u fan speed failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

ssize_t psu_fan_ratio_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_fan_ratio);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_fan_ratio(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u fan ratio failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_fan_ratio_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int psu_index;
    int ret, ratio;

    check_p(g_psu_drv);
    check_p(g_psu_drv->set_psu_fan_ratio);

    psu_index = obj->index;
    sscanf(buf, "%d", &ratio);
    if (ratio < 0 || ratio > 100) {
        PSU_ERR("param invalid, can not set ratio: %d.\n", ratio);
        return -EINVAL;
    }
    PSU_DBG("psu index: %u, ratio: %d\n", psu_index, ratio);
    ret = g_psu_drv->set_psu_fan_ratio(psu_index, ratio);
    if (ret < 0) {
        PSU_ERR("set psu%u ratio: %d failed, ret: %d\n",
            psu_index, ratio, ret);
        return -EIO;
    }
    PSU_DBG("set psu%u, ratio: %d success\n", psu_index, ratio);
    return count;
}

static ssize_t psu_fan_direction_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_fan_direction);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_fan_direction(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u fan direction failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_led_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_led_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_led_status(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u led status failed, ret: %d\n", psu_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_temp_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_temp_value);

    p_obj = to_switch_obj(obj->kobj.parent);
    psu_index = p_obj->index;
    temp_index = obj->index;

    PSU_DBG("psu index: %u, temp index: %u\n", psu_index, temp_index);
    ret = g_psu_drv->get_psu_temp_value(psu_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u temp%u value failed, ret: %d\n", psu_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_temp_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_temp_alias);

    p_obj = to_switch_obj(obj->kobj.parent);
    psu_index = p_obj->index;
    temp_index = obj->index;

    PSU_DBG("psu index: %u, temp index: %u\n", psu_index, temp_index);
    ret = g_psu_drv->get_psu_temp_alias(psu_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u temp%u alias failed, ret: %d\n", psu_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_temp_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_temp_type);

    p_obj = to_switch_obj(obj->kobj.parent);
    psu_index = p_obj->index;
    temp_index = obj->index;

    PSU_DBG("psu index: %u, temp index: %u\n", psu_index, temp_index);
    ret = g_psu_drv->get_psu_temp_type(psu_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u temp%u type failed, ret: %d\n", psu_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_temp_max_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_temp_max);

    p_obj = to_switch_obj(obj->kobj.parent);
    psu_index = p_obj->index;
    temp_index = obj->index;

    PSU_DBG("psu index: %u, temp index: %u\n", psu_index, temp_index);
    ret = g_psu_drv->get_psu_temp_max(psu_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u temp%u max threshold failed, ret: %d\n",
            psu_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_temp_max_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int psu_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_psu_drv);
    check_p(g_psu_drv->set_psu_temp_max);

    p_obj = to_switch_obj(obj->kobj.parent);
    psu_index = p_obj->index;
    temp_index = obj->index;
    ret = g_psu_drv->set_psu_temp_max(psu_index, temp_index, buf, count);
    if (ret < 0) {
        PSU_ERR("set psu%u temp%u max threshold failed, value: %s, count: %lu, ret: %d\n",
            psu_index, temp_index, buf, count, ret);
        return -EIO;
    }
    PSU_DBG("set psu%u temp%u max threshold success, value: %s, count: %lu, ret: %d\n",
        psu_index, temp_index, buf, count, ret);
    return count;
}

static ssize_t psu_temp_min_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_temp_min);

    p_obj = to_switch_obj(obj->kobj.parent);
    psu_index = p_obj->index;
    temp_index = obj->index;

    PSU_DBG("psu index: %u, temp index: %u\n", psu_index, temp_index);
    ret = g_psu_drv->get_psu_temp_min(psu_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u temp%u min threshold failed, ret: %d\n",
            psu_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t psu_temp_min_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int psu_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_psu_drv);
    check_p(g_psu_drv->set_psu_temp_min);

    p_obj = to_switch_obj(obj->kobj.parent);
    psu_index = p_obj->index;
    temp_index = obj->index;
    ret = g_psu_drv->set_psu_temp_min(psu_index, temp_index, buf, count);
    if (ret < 0) {
        PSU_ERR("set psu%u temp%u min threshold failed, value: %s, count: %lu, ret: %d\n",
            psu_index, temp_index, buf, count, ret);
        return -EIO;
    }
    PSU_DBG("set psu%u temp%u min threshold success, value: %s, count: %lu, ret: %d\n",
        psu_index, temp_index, buf, count, ret);
    return count;
}

/************************************psu dir and attrs*******************************************/
static struct switch_attribute psu_number_att = __ATTR(number, S_IRUGO, psu_number_show, NULL);

static struct attribute *psu_dir_attrs[] = {
    &psu_number_att.attr,
    NULL,
};

static struct attribute_group psu_root_attr_group = {
    .attrs = psu_dir_attrs,
};

/*******************************psu[1-n] dir and attrs*******************************************/
static struct switch_attribute psu_model_name_attr = __ATTR(model_name, S_IRUGO, psu_model_name_show, NULL);
static struct switch_attribute psu_hw_attr = __ATTR(hardware_version, S_IRUGO, psu_hw_show, NULL);
static struct switch_attribute psu_sn_attr = __ATTR(serial_number, S_IRUGO, psu_sn_show, NULL);
static struct switch_attribute psu_pn_attr = __ATTR(part_number, S_IRUGO, psu_pn_show, NULL);
static struct switch_attribute psu_type_attr = __ATTR(type, S_IRUGO, psu_type_show, NULL);
static struct switch_attribute psu_in_curr_attr = __ATTR(in_curr, S_IRUGO, psu_in_curr_show, NULL);
static struct switch_attribute psu_in_vol_attr = __ATTR(in_vol, S_IRUGO, psu_in_vol_show, NULL);
static struct switch_attribute psu_in_power_attr = __ATTR(in_power, S_IRUGO, psu_in_power_show, NULL);
static struct switch_attribute psu_out_curr_attr = __ATTR(out_curr, S_IRUGO, psu_out_curr_show, NULL);
static struct switch_attribute psu_out_vol_attr = __ATTR(out_vol, S_IRUGO, psu_out_vol_show, NULL);
static struct switch_attribute psu_out_power_attr = __ATTR(out_power, S_IRUGO, psu_out_power_show, NULL);
static struct switch_attribute psu_out_max_power_attr = __ATTR(out_max_power, S_IRUGO, psu_out_max_power_show, NULL);
static struct switch_attribute psu_num_temps_attr = __ATTR(num_temp_sensors, S_IRUGO, psu_temp_number_show, NULL);
static struct switch_attribute psu_present_attr = __ATTR(present, S_IRUGO, psu_present_status_show, NULL);
static struct switch_attribute psu_out_status_attr = __ATTR(out_status, S_IRUGO, psu_out_status_show, NULL);
static struct switch_attribute psu_in_status_attr = __ATTR(in_status, S_IRUGO, psu_in_status_show, NULL);
static struct switch_attribute psu_fan_speed_attr = __ATTR(fan_speed, S_IRUGO, psu_fan_speed_show, NULL);
static struct switch_attribute psu_fan_ratio_attr = __ATTR(fan_ratio, S_IRUGO | S_IWUSR, psu_fan_ratio_show, psu_fan_ratio_store);
static struct switch_attribute psu_fan_direction_attr = __ATTR(fan_direction, S_IRUGO, psu_fan_direction_show, NULL);
static struct switch_attribute psu_led_status_attr = __ATTR(led_status, S_IRUGO, psu_led_status_show, NULL);

static struct attribute *psu_attrs[] = {
    &psu_model_name_attr.attr,
    &psu_hw_attr.attr,
    &psu_sn_attr.attr,
    &psu_pn_attr.attr,
    &psu_type_attr.attr,
    &psu_in_curr_attr.attr,
    &psu_in_vol_attr.attr,
    &psu_in_power_attr.attr,
    &psu_out_curr_attr.attr,
    &psu_out_vol_attr.attr,
    &psu_out_power_attr.attr,
    &psu_out_max_power_attr.attr,
    &psu_num_temps_attr.attr,
    &psu_present_attr.attr,
    &psu_out_status_attr.attr,
    &psu_in_status_attr.attr,
    &psu_fan_speed_attr.attr,
    &psu_fan_ratio_attr.attr,
    &psu_fan_direction_attr.attr,
    &psu_led_status_attr.attr,
    NULL,
};

static struct attribute_group psu_attr_group = {
    .attrs = psu_attrs,
};

/*******************************psu temp[1-n] dir and attrs*******************************************/
static struct switch_attribute psu_temp_alias_attr = __ATTR(alias, S_IRUGO, psu_temp_alias_show, NULL);
static struct switch_attribute psu_temp_type_attr = __ATTR(type, S_IRUGO, psu_temp_type_show, NULL);
static struct switch_attribute psu_temp_max_attr = __ATTR(max, S_IRUGO | S_IWUSR, psu_temp_max_show, psu_temp_max_store);
static struct switch_attribute psu_temp_min_attr = __ATTR(min,  S_IRUGO | S_IWUSR, psu_temp_min_show, psu_temp_min_store);
static struct switch_attribute psu_temp_value_attr = __ATTR(value, S_IRUGO, psu_temp_value_show, NULL);

static struct attribute *psu_temp_attrs[] = {
    &psu_temp_alias_attr.attr,
    &psu_temp_type_attr.attr,
    &psu_temp_max_attr.attr,
    &psu_temp_min_attr.attr,
    &psu_temp_value_attr.attr,
    NULL,
};

static struct attribute_group psu_temp_attr_group = {
    .attrs = psu_temp_attrs,
};

static void psuindex_single_temp_remove_kobj_and_attrs(struct psu_obj_s *curr_psu, unsigned int temp_index)
{
    struct temp_obj_s *curr_temp; /* point to temp1 temp2...*/

    curr_temp = &curr_psu->temp[temp_index - 1];
    if (curr_temp->obj) {
        sysfs_remove_group(&curr_temp->obj->kobj, &psu_temp_attr_group);
        switch_kobject_delete(&curr_temp->obj);
        PSU_DBG("delete psu%u temp%u dir and attrs success.\n", curr_psu->obj->index, temp_index);
    }
    return;
}

static int psuindex_single_temp_create_kobj_and_attrs(struct psu_obj_s *curr_psu, unsigned int temp_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct temp_obj_s *curr_temp; /* point to temp1 temp2...*/

    curr_temp = &curr_psu->temp[temp_index - 1];
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "temp%u", temp_index);
    curr_temp->obj = switch_kobject_create(name, &curr_psu->obj->kobj);
    if (!curr_temp->obj) {
        PSU_ERR("create psu%u, %s object error!\n", curr_psu->obj->index, name);
        return -ENOMEM;
    }
    curr_temp->obj->index = temp_index;
    if (sysfs_create_group(&curr_temp->obj->kobj, &psu_temp_attr_group) != 0) {
        PSU_ERR("create psu%u, %s attrs error.\n", curr_psu->obj->index, name);
        switch_kobject_delete(&curr_temp->obj);
        return -EBADRQC;
    }
    PSU_DBG("create psu%u, %s success.\n", curr_psu->obj->index, name);
    return 0;
}

static int psuindex_temp_create_kobj_and_attrs(struct psu_obj_s *curr_psu)
{
    unsigned int temp_index, i, temp_num;

    temp_num = curr_psu->temp_number;
    curr_psu->temp = kzalloc(sizeof(struct temp_obj_s) * temp_num, GFP_KERNEL);
    if (!curr_psu->temp) {
        PSU_ERR("kzalloc temp error, psu index: %u, temp number: %u.\n",
            curr_psu->obj->index, temp_num);
        return -ENOMEM;
    }
    for(temp_index = 1; temp_index <= temp_num; temp_index++) {
        if(psuindex_single_temp_create_kobj_and_attrs(curr_psu, temp_index) != 0 ) {
            goto temp_error;
        }
    }
    return 0;
temp_error:
    for(i = temp_index; i > 0; i--) {
        psuindex_single_temp_remove_kobj_and_attrs(curr_psu, i);
    }
    kfree(curr_psu->temp);
    curr_psu->temp = NULL;
    return -EBADRQC;
}

static void psuindex_temp_remove_kobj_and_attrs(struct psu_obj_s *curr_psu)
{
    unsigned int temp_index, temp_num;

    if (curr_psu->temp) {
        temp_num = curr_psu->temp_number;
        for (temp_index = temp_num; temp_index > 0; temp_index--) {
            psuindex_single_temp_remove_kobj_and_attrs(curr_psu, temp_index);
        }
        kfree(curr_psu->temp);
        curr_psu->temp = NULL;
    }
    return;
}

/* create psu temp[1-n] directory and attributes*/
static int psu_temp_create(void)
{
    int psu_num, temp_num;
    unsigned int psu_index, i;
    struct psu_obj_s *curr_psu;     /* point to psu1 psu2...*/

    psu_num = g_psu.psu_number;
    if (psu_num <= 0) {
        PSU_DBG("psu number: %d, skip to create temp* dirs and attrs.\n", psu_num);
        return 0;
    }

    check_p(g_psu_drv->get_psu_temp_number);
    for(psu_index = 1; psu_index <= psu_num; psu_index++) {
        temp_num = g_psu_drv->get_psu_temp_number(psu_index);
        if (temp_num <= 0) {
            PSU_DBG("psu%u temp number: %d, don't need to create temp* dirs and attrs.\n",
                psu_index, temp_num);
            continue;
        }
        curr_psu = &g_psu.psu[psu_index - 1];
        curr_psu->temp_number = temp_num;
        if(psuindex_temp_create_kobj_and_attrs(curr_psu) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for(i = psu_index; i > 0; i--) {
        curr_psu = &g_psu.psu[i - 1];
        psuindex_temp_remove_kobj_and_attrs(curr_psu);
    }
    return -EBADRQC;
}

/* delete psu temp[1-n] directory and attributes*/
static void psu_temp_remove(void)
{
    unsigned int psu_index;
    struct psu_obj_s *curr_psu;

    if (g_psu.psu) {
        for(psu_index = g_psu.psu_number; psu_index > 0; psu_index--) {
            curr_psu = &g_psu.psu[psu_index - 1];
            psuindex_temp_remove_kobj_and_attrs(curr_psu);
            curr_psu->temp_number = 0;
        }
    }
    return;
}

static int psu_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct psu_obj_s *curr_psu;

    curr_psu = &g_psu.psu[index - 1];
    if (curr_psu->obj) {
        sysfs_remove_group(&curr_psu->obj->kobj, &psu_attr_group);
        switch_kobject_delete(&curr_psu->obj);
        PSU_DBG("delete psu%u dir and attrs success.\n", index);
    }
    return 0;
}

static int psu_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct psu_obj_s *curr_psu;

    curr_psu = &g_psu.psu[index - 1];
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "psu%u", index);
    curr_psu->obj = switch_kobject_create(name, parent);
    if (!curr_psu->obj) {
        PSU_ERR("create %s object error!\n", name);
         return -ENOMEM;
    }
    curr_psu->obj->index = index;
    if (sysfs_create_group(&curr_psu->obj->kobj, &psu_attr_group) != 0) {
        PSU_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&curr_psu->obj);
        return -EBADRQC;
    }
    PSU_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

static int psu_sub_create_kobj_and_attrs(struct kobject *parent, int psu_num)
{
    unsigned int psu_index, i;

    g_psu.psu = kzalloc(sizeof(struct psu_obj_s) * psu_num, GFP_KERNEL);
    if (!g_psu.psu) {
        PSU_ERR("kzalloc psu.psu error, psu number = %d.\n", psu_num);
        return -ENOMEM;
    }

    for(psu_index = 1; psu_index <= psu_num; psu_index++) {
        if(psu_sub_single_create_kobj_and_attrs(parent, psu_index) != 0 ) {
            goto error;
        }
    }
    return 0;
error:
    for(i = psu_index; i > 0; i--) {
        psu_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_psu.psu);
    g_psu.psu = NULL;
    return -EBADRQC;
}

/* create psu[1-n] directory and attributes*/
static int psu_sub_create(void)
{
    int ret;

    ret = psu_sub_create_kobj_and_attrs(&g_psu_obj->kobj, g_psu.psu_number);
    return ret;
}

/* delete psu[1-n] directory and attributes*/
static void psu_sub_remove(void)
{
    unsigned int psu_index;

    if (g_psu.psu) {
        for (psu_index = g_psu.psu_number; psu_index > 0; psu_index--) {
            psu_sub_single_remove_kobj_and_attrs(psu_index);
        }
        kfree(g_psu.psu);
        g_psu.psu = NULL;
    }
    g_psu.psu_number = 0;
    return;
}

/* create psu directory and number attributes*/
static int psu_root_create(void)
{
    g_psu_obj = switch_kobject_create("psu", NULL);
    if (!g_psu_obj) {
        PSU_ERR("switch_kobject_create psu error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_psu_obj->kobj, &psu_root_attr_group) != 0) {
        switch_kobject_delete(&g_psu_obj);
        PSU_ERR("create psu dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete psu directory and number attributes*/
static void psu_root_remove(void)
{
    if (g_psu_obj) {
        sysfs_remove_group(&g_psu_obj->kobj, &psu_root_attr_group);
        switch_kobject_delete(&g_psu_obj);
        PSU_DBG("delete psu dir and attrs success.\n");
    }
    return;
}

int s3ip_sysfs_psu_drivers_register(struct s3ip_sysfs_psu_drivers_s *drv)
{
    int ret, psu_num;

    PSU_INFO("s3ip_sysfs_psu_drivers_register...\n");
    if (g_psu_drv) {
        PSU_ERR("g_psu_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_psu_number);
    g_psu_drv = drv;

    psu_num = g_psu_drv->get_psu_number();
    if (psu_num <= 0) {
        PSU_ERR("psu number: %d, don't need to create psu dirs and attrs.\n", psu_num);
        g_psu_drv = NULL;
        return -EINVAL;
    }

    memset(&g_psu, 0, sizeof(struct psu_s));
    g_psu.psu_number = psu_num;
    ret = psu_root_create();
    if (ret < 0) {
        PSU_ERR("create psu root dir and attrs failed, ret: %d\n", ret);
        g_psu_drv = NULL;
        return ret;
    }

    ret = psu_sub_create();
    if (ret < 0) {
        PSU_ERR("create psu sub dir and attrs failed, ret: %d\n", ret);
        psu_root_remove();
        g_psu_drv = NULL;
        return ret;
    }

    ret = psu_temp_create();
    if (ret < 0) {
        PSU_ERR("create psu temp dir and attrs failed, ret: %d\n", ret);
        psu_sub_remove();
        psu_root_remove();
        g_psu_drv = NULL;
        return ret;
    }
    PSU_INFO("s3ip_sysfs_psu_drivers_register success.\n");
    return 0;
}

void s3ip_sysfs_psu_drivers_unregister(void)
{
    if (g_psu_drv) {
        psu_temp_remove();
        psu_sub_remove();
        psu_root_remove();
        g_psu_drv = NULL;
        PSU_DBG("s3ip_sysfs_psu_drivers_unregister success.\n");
    }

    return;
}

EXPORT_SYMBOL(s3ip_sysfs_psu_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_psu_drivers_unregister);
module_param(g_psu_loglevel, int, 0644);
MODULE_PARM_DESC(g_psu_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
