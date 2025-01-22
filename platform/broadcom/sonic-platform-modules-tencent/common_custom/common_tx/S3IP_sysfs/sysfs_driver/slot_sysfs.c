/*
 * slot_sysfs.c
 *
 * This module create slot kobjects and attributes in /sys/s3ip/slot
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "slot_sysfs.h"

static int g_slot_loglevel = 0;

#define SLOT_INFO(fmt, args...) do {                                        \
    if (g_slot_loglevel & INFO) { \
        printk(KERN_INFO "[SLOT_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SLOT_ERR(fmt, args...) do {                                        \
    if (g_slot_loglevel & ERR) { \
        printk(KERN_ERR "[SLOT_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SLOT_DBG(fmt, args...) do {                                        \
    if (g_slot_loglevel & DBG) { \
        printk(KERN_DEBUG "[SLOT_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct slot_temp_obj_s {
    struct switch_obj *obj;
};

struct slot_vol_obj_s {
    struct switch_obj *obj;
};

struct slot_curr_obj_s {
    struct switch_obj *obj;
};

struct slot_fpga_obj_s {
    struct switch_obj *obj;
};

struct slot_cpld_obj_s {
    struct switch_obj *obj;
};

struct slot_obj_s {
    unsigned int temp_number;
    unsigned int vol_number;
    unsigned int curr_number;
    unsigned int fpga_number;
    unsigned int cpld_number;
    struct slot_temp_obj_s *temp;
    struct slot_vol_obj_s *vol;
    struct slot_curr_obj_s *curr;
    struct slot_fpga_obj_s *fpga;
    struct slot_cpld_obj_s *cpld;
    struct switch_obj *obj;
};

struct slot_s {
    unsigned int slot_number;
    struct slot_obj_s *slot;
};

static struct slot_s g_slot;
static struct switch_obj *g_slot_obj = NULL;
static struct s3ip_sysfs_slot_drivers_s *g_slot_drv = NULL;

static ssize_t slot_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_slot.slot_number);
}

static ssize_t slot_temp_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int index;

    index = obj->index;
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_slot.slot[index - 1].temp_number);
}

static ssize_t slot_vol_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int index;

    index = obj->index;
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_slot.slot[index - 1].vol_number);
}

static ssize_t slot_curr_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int index;

    index = obj->index;
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_slot.slot[index - 1].curr_number);
}

static ssize_t slot_fpga_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int index;

    index = obj->index;
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_slot.slot[index - 1].fpga_number);
}

static ssize_t slot_cpld_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int index;

    index = obj->index;
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_slot.slot[index - 1].cpld_number);
}

static ssize_t slot_model_name_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int slot_index;
    int ret;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_model_name);

    slot_index = obj->index;
    SLOT_DBG("slot index: %u\n", slot_index);
    ret = g_slot_drv->get_slot_model_name(slot_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u model name failed, ret: %d\n", slot_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_sn_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int slot_index;
    int ret;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_serial_number);

    slot_index = obj->index;
    SLOT_DBG("slot index: %u\n", slot_index);
    ret = g_slot_drv->get_slot_serial_number(slot_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u serial number failed, ret: %d\n", slot_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_pn_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index;
    int ret;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_part_number);

    slot_index = obj->index;
    SLOT_DBG("slot index: %u\n", slot_index);
    ret = g_slot_drv->get_slot_part_number(slot_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u part number failed, ret: %d\n", slot_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_hw_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index;
    int ret;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_hardware_version);

    slot_index = obj->index;
    SLOT_DBG("slot index: %u\n", slot_index);
    ret = g_slot_drv->get_slot_hardware_version(slot_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u hardware version failed, ret: %d\n", slot_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index;
    int ret;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_status);

    slot_index = obj->index;
    SLOT_DBG("slot index: %u\n", slot_index);
    ret = g_slot_drv->get_slot_status(slot_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u status failed, ret: %d\n", slot_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_led_status_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int slot_index;
    int ret;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_led_status);

    slot_index = obj->index;
    SLOT_DBG("slot index: %u\n", slot_index);
    ret = g_slot_drv->get_slot_led_status(slot_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u led status failed, ret: %d\n", slot_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_led_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    unsigned int slot_index;
    int ret, led_status;

    check_p(g_slot_drv);
    check_p(g_slot_drv->set_slot_led_status);

    slot_index = obj->index;
    sscanf(buf, "%d", &led_status);
    SLOT_DBG("slot index: %u, led_status: %d\n", slot_index, led_status);
    ret = g_slot_drv->set_slot_led_status(slot_index, led_status);
    if (ret < 0) {
        SLOT_ERR("set slot%u led_status: %d failed, ret: %d\n", slot_index, led_status, ret);
        return -EIO;
    }
    SLOT_DBG("set slot%u led_status: %d success\n", slot_index, led_status);
    return count;
}

/*************************************slot temp*************************************************/
static ssize_t slot_temp_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_temp_value);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    temp_index = obj->index;

    SLOT_DBG("slot index: %u, temp index: %u\n", slot_index, temp_index);
    ret = g_slot_drv->get_slot_temp_value(slot_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u temp%u value failed, ret: %d\n", slot_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_temp_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_temp_alias);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    temp_index = obj->index;

    SLOT_DBG("slot index: %u, temp index: %u\n", slot_index, temp_index);
    ret = g_slot_drv->get_slot_temp_alias(slot_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u temp%u alias failed, ret: %d\n", slot_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_temp_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_temp_type);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    temp_index = obj->index;

    SLOT_DBG("slot index: %u, temp index: %u\n", slot_index, temp_index);
    ret = g_slot_drv->get_slot_temp_type(slot_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u temp%u type failed, ret: %d\n", slot_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_temp_max_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_temp_max);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    temp_index = obj->index;

    SLOT_DBG("slot index: %u, temp index: %u\n", slot_index, temp_index);
    ret = g_slot_drv->get_slot_temp_max(slot_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u temp%u max threshold failed, ret: %d\n",
            slot_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_temp_max_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int slot_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->set_slot_temp_max);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    temp_index = obj->index;
    ret = g_slot_drv->set_slot_temp_max(slot_index, temp_index, buf, count);
    if (ret < 0) {
        SLOT_ERR("set slot%u temp%u max threshold failed, value: %s, count: %lu, ret: %d\n",
            slot_index, temp_index, buf, count, ret);
        return -EIO;
    }
    SLOT_DBG("set slot%u temp%u max threshold success, value: %s, count: %lu, ret: %d\n",
        slot_index, temp_index, buf, count, ret);
    return count;
}

static ssize_t slot_temp_min_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_temp_min);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    temp_index = obj->index;

    SLOT_DBG("slot index: %u, temp index: %u\n", slot_index, temp_index);
    ret = g_slot_drv->get_slot_temp_min(slot_index, temp_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u temp%u min threshold failed, ret: %d\n",
            slot_index, temp_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_temp_min_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int slot_index, temp_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->set_slot_temp_min);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    temp_index = obj->index;
    ret = g_slot_drv->set_slot_temp_min(slot_index, temp_index, buf, count);
    if (ret < 0) {
        SLOT_ERR("set slot%u temp%u min threshold failed, value: %s, count: %lu, ret: %d\n",
            slot_index, temp_index, buf, count, ret);
        return -EIO;
    }
    SLOT_DBG("set slot%u temp%u min threshold success, value: %s, count: %lu, ret: %d\n",
        slot_index, temp_index, buf, count, ret);
    return count;
}
/**********************************end of slot temp**********************************************/

/*************************************slot vol*************************************************/
static ssize_t slot_vol_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, vol_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_vol_value);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    vol_index = obj->index;

    SLOT_DBG("slot index: %u, vol index: %u\n", slot_index, vol_index);
    ret = g_slot_drv->get_slot_vol_value(slot_index, vol_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u vol%u value failed, ret: %d\n", slot_index, vol_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_vol_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, vol_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_vol_alias);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    vol_index = obj->index;

    SLOT_DBG("slot index: %u, vol index: %u\n", slot_index, vol_index);
    ret = g_slot_drv->get_slot_vol_alias(slot_index, vol_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u vol%u alias failed, ret: %d\n", slot_index, vol_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_vol_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, vol_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_vol_type);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    vol_index = obj->index;

    SLOT_DBG("slot index: %u, vol index: %u\n", slot_index, vol_index);
    ret = g_slot_drv->get_slot_vol_type(slot_index, vol_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u vol%u type failed, ret: %d\n", slot_index, vol_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_vol_max_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, vol_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_vol_max);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    vol_index = obj->index;

    SLOT_DBG("slot index: %u, vol index: %u\n", slot_index, vol_index);
    ret = g_slot_drv->get_slot_vol_max(slot_index, vol_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u vol%u max threshold failed, ret: %d\n",
            slot_index, vol_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_vol_max_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int slot_index, vol_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->set_slot_vol_max);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    vol_index = obj->index;
    ret = g_slot_drv->set_slot_vol_max(slot_index, vol_index, buf, count);
    if (ret < 0) {
        SLOT_ERR("set slot%u vol%u max threshold failed, value: %s, count: %lu, ret: %d\n",
            slot_index, vol_index, buf, count, ret);
        return -EIO;
    }
    SLOT_DBG("set slot%u vol%u max threshold success, value: %s, count: %lu, ret: %d\n",
        slot_index, vol_index, buf, count, ret);
    return count;
}

static ssize_t slot_vol_min_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, vol_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_vol_min);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    vol_index = obj->index;

    SLOT_DBG("slot index: %u, vol index: %u\n", slot_index, vol_index);
    ret = g_slot_drv->get_slot_vol_min(slot_index, vol_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u vol%u min threshold failed, ret: %d\n",
            slot_index, vol_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_vol_min_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int slot_index, vol_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->set_slot_vol_min);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    vol_index = obj->index;
    ret = g_slot_drv->set_slot_vol_min(slot_index, vol_index, buf, count);
    if (ret < 0) {
        SLOT_ERR("set slot%u vol%u min threshold failed, value: %s, count: %lu, ret: %d\n",
            slot_index, vol_index, buf, count, ret);
        return -EIO;
    }
    SLOT_DBG("set slot%u vol%u min threshold success, value: %s, count: %lu, ret: %d\n",
        slot_index, vol_index, buf, count, ret);
    return count;
}

static ssize_t slot_vol_range_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, vol_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_vol_range);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    vol_index = obj->index;

    SLOT_DBG("slot index: %u, vol index: %u\n", slot_index, vol_index);
    ret = g_slot_drv->get_slot_vol_range(slot_index, vol_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u vol%u voltage range failed, ret: %d\n",
            slot_index, vol_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_vol_nominal_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, vol_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_vol_nominal_value);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    vol_index = obj->index;

    SLOT_DBG("slot index: %u, vol index: %u\n", slot_index, vol_index);
    ret = g_slot_drv->get_slot_vol_nominal_value(slot_index, vol_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u vol%u voltage nominal value failed, ret: %d\n",
            slot_index, vol_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}
/**********************************end of slot vol**********************************************/
/*************************************slot curr*************************************************/
static ssize_t slot_curr_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, curr_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_curr_value);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    curr_index = obj->index;

    SLOT_DBG("slot index: %u, curr index: %u\n", slot_index, curr_index);
    ret = g_slot_drv->get_slot_curr_value(slot_index, curr_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u curr%u value failed, ret: %d\n", slot_index, curr_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_curr_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, curr_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_curr_alias);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    curr_index = obj->index;

    SLOT_DBG("slot index: %u, curr index: %u\n", slot_index, curr_index);
    ret = g_slot_drv->get_slot_curr_alias(slot_index, curr_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u curr%u alias failed, ret: %d\n", slot_index, curr_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_curr_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, curr_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_curr_type);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    curr_index = obj->index;

    SLOT_DBG("slot index: %u, curr index: %u\n", slot_index, curr_index);
    ret = g_slot_drv->get_slot_curr_type(slot_index, curr_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u curr%u type failed, ret: %d\n", slot_index, curr_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_curr_max_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, curr_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_curr_max);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    curr_index = obj->index;

    SLOT_DBG("slot index: %u, curr index: %u\n", slot_index, curr_index);
    ret = g_slot_drv->get_slot_curr_max(slot_index, curr_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u curr%u max threshold failed, ret: %d\n",
            slot_index, curr_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_curr_max_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int slot_index, curr_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->set_slot_curr_max);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    curr_index = obj->index;
    ret = g_slot_drv->set_slot_curr_max(slot_index, curr_index, buf, count);
    if (ret < 0) {
        SLOT_ERR("set slot%u curr%u max threshold failed, value: %s, count: %lu, ret: %d\n",
            slot_index, curr_index, buf, count, ret);
        return -EIO;
    }
    SLOT_DBG("set slot%u curr%u max threshold success, value: %s, count: %lu, ret: %d\n",
        slot_index, curr_index, buf, count, ret);
    return count;
}

static ssize_t slot_curr_min_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, curr_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_curr_min);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    curr_index = obj->index;

    SLOT_DBG("slot index: %u, curr index: %u\n", slot_index, curr_index);
    ret = g_slot_drv->get_slot_curr_min(slot_index, curr_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u curr%u min threshold failed, ret: %d\n",
            slot_index, curr_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_curr_min_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int slot_index, curr_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->set_slot_curr_min);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    curr_index = obj->index;
    ret = g_slot_drv->set_slot_curr_min(slot_index, curr_index, buf, count);
    if (ret < 0) {
        SLOT_ERR("set slot%u curr%u min threshold failed, value: %s, count: %lu, ret: %d\n",
            slot_index, curr_index, buf, count, ret);
        return -EIO;
    }
    SLOT_DBG("set slot%u curr%u min threshold success, value: %s, count: %lu, ret: %d\n",
        slot_index, curr_index, buf, count, ret);
    return count;
}
/**********************************end of slot curr**********************************************/
/*************************************slot fpga*************************************************/
static ssize_t slot_fpga_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, fpga_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_fpga_alias);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    fpga_index = obj->index;

    SLOT_DBG("slot index: %u, fpga index: %u\n", slot_index, fpga_index);
    ret = g_slot_drv->get_slot_fpga_alias(slot_index, fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u fpga%u alias failed, ret: %d\n", slot_index, fpga_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_fpga_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, fpga_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_fpga_type);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    fpga_index = obj->index;

    SLOT_DBG("slot index: %u, fpga index: %u\n", slot_index, fpga_index);
    ret = g_slot_drv->get_slot_fpga_type(slot_index, fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u fpga%u type failed, ret: %d\n", slot_index, fpga_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_fpga_fw_version_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int slot_index, fpga_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_fpga_firmware_version);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    fpga_index = obj->index;

    SLOT_DBG("slot index: %u, fpga index: %u\n", slot_index, fpga_index);
    ret = g_slot_drv->get_slot_fpga_firmware_version(slot_index, fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u fpga%u firmware version failed, ret: %d\n", slot_index, fpga_index,
            ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_fpga_board_version_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int slot_index, fpga_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_fpga_board_version);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    fpga_index = obj->index;

    SLOT_DBG("slot index: %u, fpga index: %u\n", slot_index, fpga_index);
    ret = g_slot_drv->get_slot_fpga_board_version(slot_index, fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u fpga%u board version failed, ret: %d\n", slot_index, fpga_index,
            ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_fpga_test_reg_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int slot_index, fpga_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_fpga_test_reg);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    fpga_index = obj->index;

    SLOT_DBG("slot index: %u, fpga index: %u\n", slot_index, fpga_index);
    ret = g_slot_drv->get_slot_fpga_test_reg(slot_index, fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u fpga%u test register failed, ret: %d\n", slot_index, fpga_index,
            ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_fpga_test_reg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int slot_index, fpga_index, value;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->set_slot_fpga_test_reg);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    fpga_index = obj->index;
    sscanf(buf, "0x%x", &value);
    ret = g_slot_drv->set_slot_fpga_test_reg(slot_index, fpga_index, value);
    if (ret < 0) {
        SLOT_ERR("set slot%u fpga%u test reg failed, value:0x%x, ret: %d.\n",
            slot_index, fpga_index, value, ret);
        return -EIO;
    }
    SLOT_DBG("set slot%u fpga%u test reg success, value: 0x%x.\n", slot_index, fpga_index, value);
    return count;
}
/**********************************end of slot fpga**********************************************/
/*************************************slot cpld*************************************************/
static ssize_t slot_cpld_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, cpld_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_cpld_alias);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    cpld_index = obj->index;

    SLOT_DBG("slot index: %u, cpld index: %u\n", slot_index, cpld_index);
    ret = g_slot_drv->get_slot_cpld_alias(slot_index, cpld_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u cpld%u alias failed, ret: %d\n", slot_index, cpld_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_cpld_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, cpld_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_cpld_type);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    cpld_index = obj->index;

    SLOT_DBG("slot index: %u, cpld index: %u\n", slot_index, cpld_index);
    ret = g_slot_drv->get_slot_cpld_type(slot_index, cpld_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u cpld%u type failed, ret: %d\n", slot_index, cpld_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_cpld_fw_version_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int slot_index, cpld_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_cpld_firmware_version);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    cpld_index = obj->index;

    SLOT_DBG("slot index: %u, cpld index: %u\n", slot_index, cpld_index);
    ret = g_slot_drv->get_slot_cpld_firmware_version(slot_index, cpld_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u cpld%u firmware version failed, ret: %d\n", slot_index, cpld_index,
            ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_cpld_board_version_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int slot_index, cpld_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_cpld_board_version);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    cpld_index = obj->index;

    SLOT_DBG("slot index: %u, cpld index: %u\n", slot_index, cpld_index);
    ret = g_slot_drv->get_slot_cpld_board_version(slot_index, cpld_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u cpld%u board version failed, ret: %d\n", slot_index, cpld_index,
            ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_cpld_test_reg_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int slot_index, cpld_index;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->get_slot_cpld_test_reg);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    cpld_index = obj->index;

    SLOT_DBG("slot index: %u, cpld index: %u\n", slot_index, cpld_index);
    ret = g_slot_drv->get_slot_cpld_test_reg(slot_index, cpld_index, buf, PAGE_SIZE);
    if (ret < 0) {
        SLOT_ERR("get slot%u cpld%u test register failed, ret: %d\n", slot_index, cpld_index,
            ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_cpld_test_reg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int slot_index, cpld_index, value;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_slot_drv);
    check_p(g_slot_drv->set_slot_cpld_test_reg);

    p_obj = to_switch_obj(obj->kobj.parent);
    slot_index = p_obj->index;
    cpld_index = obj->index;
    sscanf(buf, "0x%x", &value);
    ret = g_slot_drv->set_slot_cpld_test_reg(slot_index, cpld_index, value);
    if (ret < 0) {
        SLOT_ERR("set slot%u cpld%u test reg failed, value:0x%x, ret: %d.\n",
            slot_index, cpld_index, value, ret);
        return -EIO;
    }
    SLOT_DBG("set slot%u cpld%u test reg success, value: 0x%x.\n", slot_index, cpld_index, value);
    return count;
}
/**********************************end of slot cpld**********************************************/
/**********************************slot dir and attrs********************************************/
static struct switch_attribute slot_number_attr = __ATTR(number, S_IRUGO, slot_number_show, NULL);

static struct attribute *slot_dir_attrs[] = {
    &slot_number_attr.attr,
    NULL,
};

static struct attribute_group slot_root_attr_group = {
    .attrs = slot_dir_attrs,
};

/*******************************slot[1-n] dir and attrs*******************************************/
static struct switch_attribute slot_model_name_attr = __ATTR(model_name, S_IRUGO, slot_model_name_show, NULL);
static struct switch_attribute slot_hw_attr = __ATTR(hardware_version, S_IRUGO, slot_hw_show, NULL);
static struct switch_attribute slot_sn_attr = __ATTR(serial_number, S_IRUGO, slot_sn_show, NULL);
static struct switch_attribute slot_pn_attr = __ATTR(part_number, S_IRUGO, slot_pn_show, NULL);
static struct switch_attribute slot_status_attr = __ATTR(status, S_IRUGO, slot_status_show, NULL);
static struct switch_attribute slot_led_status_attr = __ATTR(led_status, S_IRUGO | S_IWUSR, slot_led_status_show, slot_led_status_store);
static struct switch_attribute num_temp_sensors_attr = __ATTR(num_temp_sensors, S_IRUGO, slot_temp_number_show, NULL);
static struct switch_attribute num_vol_sensors_attr = __ATTR(num_vol_sensors, S_IRUGO, slot_vol_number_show, NULL);
static struct switch_attribute num_curr_sensors_attr = __ATTR(num_curr_sensors, S_IRUGO, slot_curr_number_show, NULL);
static struct switch_attribute num_fpga_attr = __ATTR(num_fpgas, S_IRUGO, slot_fpga_number_show, NULL);
static struct switch_attribute num_cpld_attr = __ATTR(num_cplds, S_IRUGO, slot_cpld_number_show, NULL);

static struct attribute *slot_attrs[] = {
    &slot_model_name_attr.attr,
    &slot_hw_attr.attr,
    &slot_sn_attr.attr,
    &slot_pn_attr.attr,
    &slot_status_attr.attr,
    &slot_led_status_attr.attr,
    &num_temp_sensors_attr.attr,
    &num_vol_sensors_attr.attr,
    &num_curr_sensors_attr.attr,
    &num_fpga_attr.attr,
    &num_cpld_attr.attr,
    NULL,
};

static struct attribute_group slot_attr_group = {
    .attrs = slot_attrs,
};

/*******************************slot temp[1-n] dir and attrs*******************************************/
static struct switch_attribute slot_temp_alias_attr = __ATTR(alias, S_IRUGO, slot_temp_alias_show, NULL);
static struct switch_attribute slot_temp_type_attr = __ATTR(type, S_IRUGO, slot_temp_type_show, NULL);
static struct switch_attribute slot_temp_max_attr = __ATTR(max, S_IRUGO | S_IWUSR, slot_temp_max_show, slot_temp_max_store);
static struct switch_attribute slot_temp_min_attr = __ATTR(min,  S_IRUGO | S_IWUSR, slot_temp_min_show, slot_temp_min_store);
static struct switch_attribute slot_temp_value_attr = __ATTR(value, S_IRUGO, slot_temp_value_show, NULL);

static struct attribute *slot_temp_attrs[] = {
    &slot_temp_alias_attr.attr,
    &slot_temp_type_attr.attr,
    &slot_temp_max_attr.attr,
    &slot_temp_min_attr.attr,
    &slot_temp_value_attr.attr,
    NULL,
};

static struct attribute_group slot_temp_attr_group = {
    .attrs = slot_temp_attrs,
};

/*******************************slot vol[1-n] dir and attrs*******************************************/
static struct switch_attribute slot_vol_alias_attr = __ATTR(alias, S_IRUGO, slot_vol_alias_show, NULL);
static struct switch_attribute slot_vol_type_attr = __ATTR(type, S_IRUGO, slot_vol_type_show, NULL);
static struct switch_attribute slot_vol_max_attr = __ATTR(max, S_IRUGO | S_IWUSR, slot_vol_max_show, slot_vol_max_store);
static struct switch_attribute slot_vol_min_attr = __ATTR(min,  S_IRUGO | S_IWUSR, slot_vol_min_show, slot_vol_min_store);
static struct switch_attribute slot_vol_value_attr = __ATTR(value, S_IRUGO, slot_vol_value_show, NULL);
static struct switch_attribute slot_vol_range_attr = __ATTR(range, S_IRUGO, slot_vol_range_show, NULL);
static struct switch_attribute slot_vol_nominal_value_attr = __ATTR(nominal_value, S_IRUGO, slot_vol_nominal_value_show, NULL);

static struct attribute *slot_vol_attrs[] = {
    &slot_vol_alias_attr.attr,
    &slot_vol_type_attr.attr,
    &slot_vol_max_attr.attr,
    &slot_vol_min_attr.attr,
    &slot_vol_value_attr.attr,
    &slot_vol_range_attr.attr,
    &slot_vol_nominal_value_attr.attr,
    NULL,
};

static struct attribute_group slot_vol_attr_group = {
    .attrs = slot_vol_attrs,
};

/*******************************slot curr[1-n] dir and attrs*******************************************/
static struct switch_attribute slot_curr_alias_attr = __ATTR(alias, S_IRUGO, slot_curr_alias_show, NULL);
static struct switch_attribute slot_curr_type_attr = __ATTR(type, S_IRUGO, slot_curr_type_show, NULL);
static struct switch_attribute slot_curr_max_attr = __ATTR(max, S_IRUGO | S_IWUSR, slot_curr_max_show, slot_curr_max_store);
static struct switch_attribute slot_curr_min_attr = __ATTR(min,  S_IRUGO | S_IWUSR, slot_curr_min_show, slot_curr_min_store);
static struct switch_attribute slot_curr_value_attr = __ATTR(value, S_IRUGO, slot_curr_value_show, NULL);

static struct attribute *slot_curr_attrs[] = {
    &slot_curr_alias_attr.attr,
    &slot_curr_type_attr.attr,
    &slot_curr_max_attr.attr,
    &slot_curr_min_attr.attr,
    &slot_curr_value_attr.attr,
    NULL,
};

static struct attribute_group slot_curr_attr_group = {
    .attrs = slot_curr_attrs,
};

/*******************************slot fpga[1-n] dir and attrs*******************************************/
static struct switch_attribute slot_fpga_alias_attr = __ATTR(alias, S_IRUGO, slot_fpga_alias_show, NULL);
static struct switch_attribute slot_fpga_type_attr = __ATTR(type, S_IRUGO, slot_fpga_type_show, NULL);
static struct switch_attribute slot_fpga_fw_version_attr = __ATTR(firmware_version, S_IRUGO, slot_fpga_fw_version_show, NULL);
static struct switch_attribute slot_fpga_board_version_attr = __ATTR(board_version, S_IRUGO, slot_fpga_board_version_show, NULL);
static struct switch_attribute slot_fpga_test_reg_attr = __ATTR(reg_test, S_IRUGO | S_IWUSR, slot_fpga_test_reg_show, slot_fpga_test_reg_store);

static struct attribute *slot_fpga_attrs[] = {
    &slot_fpga_alias_attr.attr,
    &slot_fpga_type_attr.attr,
    &slot_fpga_fw_version_attr.attr,
    &slot_fpga_board_version_attr.attr,
    &slot_fpga_test_reg_attr.attr,
    NULL,
};

static struct attribute_group slot_fpga_attr_group = {
    .attrs = slot_fpga_attrs,
};

/*******************************slot cpld[1-n] dir and attrs*******************************************/
static struct switch_attribute slot_cpld_alias_attr = __ATTR(alias, S_IRUGO, slot_cpld_alias_show, NULL);
static struct switch_attribute slot_cpld_type_attr = __ATTR(type, S_IRUGO, slot_cpld_type_show, NULL);
static struct switch_attribute slot_cpld_fw_version_attr = __ATTR(firmware_version, S_IRUGO, slot_cpld_fw_version_show, NULL);
static struct switch_attribute slot_cpld_board_version_attr = __ATTR(board_version, S_IRUGO, slot_cpld_board_version_show, NULL);
static struct switch_attribute slot_cpld_test_reg_attr = __ATTR(reg_test, S_IRUGO | S_IWUSR, slot_cpld_test_reg_show, slot_cpld_test_reg_store);

static struct attribute *slot_cpld_attrs[] = {
    &slot_cpld_alias_attr.attr,
    &slot_cpld_type_attr.attr,
    &slot_cpld_fw_version_attr.attr,
    &slot_cpld_board_version_attr.attr,
    &slot_cpld_test_reg_attr.attr,
    NULL,
};

static struct attribute_group slot_cpld_attr_group = {
    .attrs = slot_cpld_attrs,
};

/***************************************slot cpld*****************************************/
static void slotindex_single_cpld_remove_kobj_and_attrs(struct slot_obj_s *curr_slot,
                unsigned int cpld_index)
{
    struct slot_cpld_obj_s *curr_cpld;

    curr_cpld = &curr_slot->cpld[cpld_index - 1];
    if (curr_cpld->obj) {
        sysfs_remove_group(&curr_cpld->obj->kobj, &slot_cpld_attr_group);
        switch_kobject_delete(&curr_cpld->obj);
        SLOT_DBG("delete slot%u cpld%u dir and attrs success.\n", curr_slot->obj->index,
            cpld_index);
    }
    return;
}

static int slotindex_single_cpld_create_kobj_and_attrs(struct slot_obj_s *curr_slot,
               unsigned int cpld_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct slot_cpld_obj_s *curr_cpld;

    curr_cpld = &curr_slot->cpld[cpld_index - 1];
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "cpld%u", cpld_index);
    curr_cpld->obj = switch_kobject_create(name, &curr_slot->obj->kobj);
    if (!curr_cpld->obj) {
        SLOT_ERR("create slot%u %s object error!\n", curr_slot->obj->index, name);
        return -ENOMEM;
    }
    curr_cpld->obj->index = cpld_index;
    if (sysfs_create_group(&curr_cpld->obj->kobj, &slot_cpld_attr_group) != 0) {
        SLOT_ERR("create slot%u %s attrs error.\n", curr_slot->obj->index, name);
        switch_kobject_delete(&curr_cpld->obj);
        return -EBADRQC;
    }
    SLOT_DBG("create slot%u %s success.\n", curr_slot->obj->index, name);
    return 0;
}

static void slotindex_cpld_remove_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int cpld_index, cpld_num;

    if (curr_slot->cpld) {
        cpld_num = curr_slot->cpld_number;
        for (cpld_index = cpld_num; cpld_index > 0; cpld_index--) {
            slotindex_single_cpld_remove_kobj_and_attrs(curr_slot, cpld_index);
        }
        kfree(curr_slot->cpld);
        curr_slot->cpld = NULL;
    }
    return;
}

static int slotindex_cpld_create_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int cpld_index, i, cpld_num;

    cpld_num = curr_slot->cpld_number;
    curr_slot->cpld = kzalloc(sizeof(struct slot_cpld_obj_s) * cpld_num, GFP_KERNEL);
    if (!curr_slot->cpld) {
        SLOT_ERR("kzalloc slot cpld error, slot index: %u, cpld number: %u.\n",
            curr_slot->obj->index, cpld_num);
        return -ENOMEM;
    }

    for (cpld_index = 1; cpld_index <= cpld_num; cpld_index++) {
        if (slotindex_single_cpld_create_kobj_and_attrs(curr_slot, cpld_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = cpld_index; i > 0; i--) {
        slotindex_single_cpld_remove_kobj_and_attrs(curr_slot, i);
    }
    kfree(curr_slot->cpld);
    curr_slot->cpld = NULL;
    return -EBADRQC;
}

/* create slot cpld[1-n] directory and attributes*/
static int slot_cpld_create(void)
{
    int cpld_num;
    unsigned int slot_index, i;
    struct slot_obj_s *curr_slot;

    check_p(g_slot_drv->get_slot_cpld_number);
    for (slot_index = 1; slot_index <= g_slot.slot_number; slot_index++) {
        cpld_num = g_slot_drv->get_slot_cpld_number(slot_index);
        if (cpld_num <= 0) {
            SLOT_DBG("slot%u cpld number: %d, don't need to create cpld* dirs and attrs.\n",
                slot_index, cpld_num);
            continue;
        }
        curr_slot = &g_slot.slot[slot_index - 1];
        curr_slot->cpld_number = cpld_num;
        if (slotindex_cpld_create_kobj_and_attrs(curr_slot) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = slot_index; i > 0; i--) {
        curr_slot = &g_slot.slot[i - 1];
        slotindex_cpld_remove_kobj_and_attrs(curr_slot);
    }
    return -EBADRQC;
}

/* delete slot cpld[1-n] directory and attributes*/
static void slot_cpld_remove(void)
{
    unsigned int slot_index;
    struct slot_obj_s *curr_slot;

    if (g_slot.slot) {
        for (slot_index = g_slot.slot_number; slot_index > 0; slot_index--) {
            curr_slot = &g_slot.slot[slot_index - 1];
            slotindex_cpld_remove_kobj_and_attrs(curr_slot);
            curr_slot->cpld_number = 0;
        }
    }
    return;
}
/************************************end of slot cpld**************************************/
/***************************************slot fpga*****************************************/
static void slotindex_single_fpga_remove_kobj_and_attrs(struct slot_obj_s *curr_slot,
                unsigned int fpga_index)
{
    struct slot_fpga_obj_s *curr_fpga;

    curr_fpga = &curr_slot->fpga[fpga_index - 1];
    if (curr_fpga->obj) {
        sysfs_remove_group(&curr_fpga->obj->kobj, &slot_fpga_attr_group);
        switch_kobject_delete(&curr_fpga->obj);
        SLOT_DBG("delete slot%u fpga%u dir and attrs success.\n", curr_slot->obj->index,
            fpga_index);
    }
    return;
}

static int slotindex_single_fpga_create_kobj_and_attrs(struct slot_obj_s *curr_slot,
               unsigned int fpga_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct slot_fpga_obj_s *curr_fpga;

    curr_fpga = &curr_slot->fpga[fpga_index - 1];
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "fpga%u", fpga_index);
    curr_fpga->obj = switch_kobject_create(name, &curr_slot->obj->kobj);
    if (!curr_fpga->obj) {
        SLOT_ERR("create slot%u %s object error!\n", curr_slot->obj->index, name);
        return -ENOMEM;
    }
    curr_fpga->obj->index = fpga_index;
    if (sysfs_create_group(&curr_fpga->obj->kobj, &slot_fpga_attr_group) != 0) {
        SLOT_ERR("create slot%u %s attrs error.\n", curr_slot->obj->index, name);
        switch_kobject_delete(&curr_fpga->obj);
        return -EBADRQC;
    }
    SLOT_DBG("create slot%u %s success.\n", curr_slot->obj->index, name);
    return 0;
}

static void slotindex_fpga_remove_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int fpga_index, fpga_num;

    if (curr_slot->fpga) {
        fpga_num = curr_slot->fpga_number;
        for (fpga_index = fpga_num; fpga_index > 0; fpga_index--) {
            slotindex_single_fpga_remove_kobj_and_attrs(curr_slot, fpga_index);
        }
        kfree(curr_slot->fpga);
        curr_slot->fpga = NULL;
    }
    return;
}

static int slotindex_fpga_create_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int fpga_index, i, fpga_num;

    fpga_num = curr_slot->fpga_number;
    curr_slot->fpga = kzalloc(sizeof(struct slot_fpga_obj_s) * fpga_num, GFP_KERNEL);
    if (!curr_slot->fpga) {
        SLOT_ERR("kzalloc slot fpga error, slot index: %u, fpga number: %u.\n",
            curr_slot->obj->index, fpga_num);
        return -ENOMEM;
    }

    for (fpga_index = 1; fpga_index <= fpga_num; fpga_index++) {
        if (slotindex_single_fpga_create_kobj_and_attrs(curr_slot, fpga_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = fpga_index; i > 0; i--) {
        slotindex_single_fpga_remove_kobj_and_attrs(curr_slot, i);
    }
    kfree(curr_slot->fpga);
    curr_slot->fpga = NULL;
    return -EBADRQC;
}

/* create slot fpga[1-n] directory and attributes*/
static int slot_fpga_create(void)
{
    int fpga_num;
    unsigned int slot_index, i;
    struct slot_obj_s *curr_slot;

    check_p(g_slot_drv->get_slot_fpga_number);
    for (slot_index = 1; slot_index <= g_slot.slot_number; slot_index++) {
        fpga_num = g_slot_drv->get_slot_fpga_number(slot_index);
        if (fpga_num <= 0) {
            SLOT_DBG("slot%u fpga number: %d, don't need to create fpga* dirs and attrs.\n",
                slot_index, fpga_num);
            continue;
        }
        curr_slot = &g_slot.slot[slot_index - 1];
        curr_slot->fpga_number = fpga_num;
        if (slotindex_fpga_create_kobj_and_attrs(curr_slot) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = slot_index; i > 0; i--) {
        curr_slot = &g_slot.slot[i - 1];
        slotindex_fpga_remove_kobj_and_attrs(curr_slot);
    }
    return -EBADRQC;
}

/* delete slot fpga[1-n] directory and attributes*/
static void slot_fpga_remove(void)
{
    unsigned int slot_index;
    struct slot_obj_s *curr_slot;

    if (g_slot.slot) {
        for (slot_index = g_slot.slot_number; slot_index > 0; slot_index--) {
            curr_slot = &g_slot.slot[slot_index - 1];
            slotindex_fpga_remove_kobj_and_attrs(curr_slot);
            curr_slot->fpga_number = 0;
        }
    }
    return;
}
/************************************end of slot fpga**************************************/
/*************************************slot current*****************************************/
static void slotindex_single_curr_remove_kobj_and_attrs(struct slot_obj_s *curr_slot,
                unsigned int curr_index)
{
    struct slot_curr_obj_s *curr;

    curr = &curr_slot->curr[curr_index - 1];
    if (curr->obj) {
        sysfs_remove_group(&curr->obj->kobj, &slot_curr_attr_group);
        switch_kobject_delete(&curr->obj);
        SLOT_DBG("delete slot%u curr_sensor%u dir and attrs success.\n", curr_slot->obj->index,
            curr_index);
    }
    return;
}

static int slotindex_single_curr_create_kobj_and_attrs(struct slot_obj_s *curr_slot,
               unsigned int curr_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct slot_curr_obj_s *curr;

    curr = &curr_slot->curr[curr_index - 1];
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "curr_sensor%u", curr_index);
    curr->obj = switch_kobject_create(name, &curr_slot->obj->kobj);
    if (!curr->obj) {
        SLOT_ERR("create slot%u %s object error!\n", curr_slot->obj->index, name);
        return -ENOMEM;
    }
    curr->obj->index = curr_index;
    if (sysfs_create_group(&curr->obj->kobj, &slot_curr_attr_group) != 0) {
        SLOT_ERR("create slot%u %s attrs error.\n", curr_slot->obj->index, name);
        switch_kobject_delete(&curr->obj);
        return -EBADRQC;
    }
    SLOT_DBG("create slot%u %s success.\n", curr_slot->obj->index, name);
    return 0;
}

static void slotindex_curr_remove_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int curr_index, curr_num;

    if (curr_slot->curr) {
        curr_num = curr_slot->curr_number;
        for (curr_index = curr_num; curr_index > 0; curr_index--) {
            slotindex_single_curr_remove_kobj_and_attrs(curr_slot, curr_index);
        }
        kfree(curr_slot->curr);
        curr_slot->curr = NULL;
    }
    return;
}

static int slotindex_curr_create_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int curr_index, i, curr_num;

    curr_num = curr_slot->curr_number;
    curr_slot->curr = kzalloc(sizeof(struct slot_curr_obj_s) * curr_num, GFP_KERNEL);
    if (!curr_slot->curr) {
        SLOT_ERR("kzalloc slot curr error, slot index: %u, curr number: %u.\n",
            curr_slot->obj->index, curr_num);
        return -ENOMEM;
    }

    for (curr_index = 1; curr_index <= curr_num; curr_index++) {
        if (slotindex_single_curr_create_kobj_and_attrs(curr_slot, curr_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = curr_index; i > 0; i--) {
        slotindex_single_curr_remove_kobj_and_attrs(curr_slot, i);
    }
    kfree(curr_slot->curr);
    curr_slot->curr = NULL;
    return -EBADRQC;
}

/* create slot curr_snesor[1-n] directory and attributes*/
static int slot_curr_create(void)
{
    int curr_num;
    unsigned int slot_index, i;
    struct slot_obj_s *curr_slot;

    check_p(g_slot_drv->get_slot_curr_number);
    for (slot_index = 1; slot_index <= g_slot.slot_number; slot_index++) {
        curr_num = g_slot_drv->get_slot_curr_number(slot_index);
        if (curr_num <= 0) {
            SLOT_DBG("slot%u curr number: %d, don't need to create curr_sensor* dirs and attrs.\n",
                slot_index, curr_num);
            continue;
        }
        curr_slot = &g_slot.slot[slot_index - 1];
        curr_slot->curr_number = curr_num;
        if (slotindex_curr_create_kobj_and_attrs(curr_slot) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = slot_index; i > 0; i--) {
        curr_slot = &g_slot.slot[i - 1];
        slotindex_curr_remove_kobj_and_attrs(curr_slot);
    }
    return -EBADRQC;
}

/* delete slot curr_sensor[1-n] directory and attributes*/
static void slot_curr_remove(void)
{
    unsigned int slot_index;
    struct slot_obj_s *curr_slot;

    if (g_slot.slot) {
        for (slot_index = g_slot.slot_number; slot_index > 0; slot_index--) {
            curr_slot = &g_slot.slot[slot_index - 1];
            slotindex_curr_remove_kobj_and_attrs(curr_slot);
            curr_slot->curr_number = 0;
        }
    }
    return;
}
/**********************************end of slot current************************************/
/*************************************slot voltage****************************************/
static void slotindex_single_vol_remove_kobj_and_attrs(struct slot_obj_s *curr_slot,
                unsigned int vol_index)
{
    struct slot_vol_obj_s *curr_vol;

    curr_vol = &curr_slot->vol[vol_index - 1];
    if (curr_vol->obj) {
        sysfs_remove_group(&curr_vol->obj->kobj, &slot_vol_attr_group);
        switch_kobject_delete(&curr_vol->obj);
        SLOT_DBG("delete slot%u vol_sensor%u dir and attrs success.\n", curr_slot->obj->index,
            vol_index);
    }
    return;
}

static int slotindex_single_vol_create_kobj_and_attrs(struct slot_obj_s *curr_slot,
               unsigned int vol_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct slot_vol_obj_s *curr_vol;

    curr_vol = &curr_slot->vol[vol_index - 1];
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "vol_sensor%u", vol_index);
    curr_vol->obj = switch_kobject_create(name, &curr_slot->obj->kobj);
    if (!curr_vol->obj) {
        SLOT_ERR("create slot%u %s object error!\n", curr_slot->obj->index, name);
        return -ENOMEM;
    }
    curr_vol->obj->index = vol_index;
    if (sysfs_create_group(&curr_vol->obj->kobj, &slot_vol_attr_group) != 0) {
        SLOT_ERR("create slot%u %s attrs error.\n", curr_slot->obj->index, name);
        switch_kobject_delete(&curr_vol->obj);
        return -EBADRQC;
    }
    SLOT_DBG("create slot%u %s success.\n", curr_slot->obj->index, name);
    return 0;
}

static void slotindex_vol_remove_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int vol_index, vol_num;

    if (curr_slot->vol) {
        vol_num = curr_slot->vol_number;
        for (vol_index = vol_num; vol_index > 0; vol_index--) {
            slotindex_single_vol_remove_kobj_and_attrs(curr_slot, vol_index);
        }
        kfree(curr_slot->vol);
        curr_slot->vol = NULL;
    }
    return;
}

static int slotindex_vol_create_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int vol_index, i, vol_num;

    vol_num = curr_slot->vol_number;
    curr_slot->vol = kzalloc(sizeof(struct slot_vol_obj_s) * vol_num, GFP_KERNEL);
    if (!curr_slot->vol) {
        SLOT_ERR("kzalloc slot vol error, slot index: %u, vol number: %u.\n",
            curr_slot->obj->index, vol_num);
        return -ENOMEM;
    }

    for (vol_index = 1; vol_index <= vol_num; vol_index++) {
        if (slotindex_single_vol_create_kobj_and_attrs(curr_slot, vol_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = vol_index; i > 0; i--) {
        slotindex_single_vol_remove_kobj_and_attrs(curr_slot, i);
    }
    kfree(curr_slot->vol);
    curr_slot->vol = NULL;
    return -EBADRQC;
}

/* create slot vol_snesor[1-n] directory and attributes*/
static int slot_vol_create(void)
{
    int vol_num;
    unsigned int slot_index, i;
    struct slot_obj_s *curr_slot;

    check_p(g_slot_drv->get_slot_vol_number);
    for (slot_index = 1; slot_index <= g_slot.slot_number; slot_index++) {
        vol_num = g_slot_drv->get_slot_vol_number(slot_index);
        if (vol_num <= 0) {
            SLOT_DBG("slot%u vol number: %d, don't need to create vol_sensor* dirs and attrs.\n",
                slot_index, vol_num);
            continue;
        }
        curr_slot = &g_slot.slot[slot_index - 1];
        curr_slot->vol_number = vol_num;
        if (slotindex_vol_create_kobj_and_attrs(curr_slot) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = slot_index; i > 0; i--) {
        curr_slot = &g_slot.slot[i - 1];
        slotindex_vol_remove_kobj_and_attrs(curr_slot);
    }
    return -EBADRQC;
}

/* delete slot vol_sensor[1-n] directory and attributes*/
static void slot_vol_remove(void)
{
    unsigned int slot_index;
    struct slot_obj_s *curr_slot;

    if (g_slot.slot) {
        for (slot_index = g_slot.slot_number; slot_index > 0; slot_index--) {
            curr_slot = &g_slot.slot[slot_index - 1];
            slotindex_vol_remove_kobj_and_attrs(curr_slot);
            curr_slot->vol_number = 0;
        }
    }
    return;
}
/**********************************end of slot voltage************************************/
/***************************************slot temp*****************************************/
static void slotindex_single_temp_remove_kobj_and_attrs(struct slot_obj_s *curr_slot,
                unsigned int temp_index)
{
    struct slot_temp_obj_s *curr_temp;

    curr_temp = &curr_slot->temp[temp_index - 1];
    if (curr_temp->obj) {
        sysfs_remove_group(&curr_temp->obj->kobj, &slot_temp_attr_group);
        switch_kobject_delete(&curr_temp->obj);
        SLOT_DBG("delete slot%u temp_sensor%u dir and attrs success.\n", curr_slot->obj->index,
            temp_index);
    }
    return;
}

static int slotindex_single_temp_create_kobj_and_attrs(struct slot_obj_s *curr_slot,
               unsigned int temp_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct slot_temp_obj_s *curr_temp;

    curr_temp = &curr_slot->temp[temp_index - 1];
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "temp_sensor%u", temp_index);
    curr_temp->obj = switch_kobject_create(name, &curr_slot->obj->kobj);
    if (!curr_temp->obj) {
        SLOT_ERR("create slot%u %s object error!\n", curr_slot->obj->index, name);
        return -ENOMEM;
    }
    curr_temp->obj->index = temp_index;
    if (sysfs_create_group(&curr_temp->obj->kobj, &slot_temp_attr_group) != 0) {
        SLOT_ERR("create slot%u %s attrs error.\n", curr_slot->obj->index, name);
        switch_kobject_delete(&curr_temp->obj);
        return -EBADRQC;
    }
    SLOT_DBG("create slot%u %s success.\n", curr_slot->obj->index, name);
    return 0;
}

static void slotindex_temp_remove_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int temp_index, temp_num;

    if (curr_slot->temp) {
        temp_num = curr_slot->temp_number;
        for (temp_index = temp_num; temp_index > 0; temp_index--) {
            slotindex_single_temp_remove_kobj_and_attrs(curr_slot, temp_index);
        }
        kfree(curr_slot->temp);
        curr_slot->temp = NULL;
    }
    return;
}

static int slotindex_temp_create_kobj_and_attrs(struct slot_obj_s *curr_slot)
{
    unsigned int temp_index, i, temp_num;

    temp_num = curr_slot->temp_number;
    curr_slot->temp = kzalloc(sizeof(struct slot_temp_obj_s) * temp_num, GFP_KERNEL);
    if (!curr_slot->temp) {
        SLOT_ERR("kzalloc slot temp error, slot index: %u, temp number: %u.\n",
            curr_slot->obj->index, temp_num);
        return -ENOMEM;
    }

    for (temp_index = 1; temp_index <= temp_num; temp_index++) {
        if (slotindex_single_temp_create_kobj_and_attrs(curr_slot, temp_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = temp_index; i > 0; i--) {
        slotindex_single_temp_remove_kobj_and_attrs(curr_slot, i);
    }
    kfree(curr_slot->temp);
    curr_slot->temp = NULL;
    return -EBADRQC;
}

/* create slot temp_sensor[1-n] directory and attributes*/
static int slot_temp_create(void)
{
    int temp_num;
    unsigned int slot_index, i;
    struct slot_obj_s *curr_slot;

    check_p(g_slot_drv->get_slot_temp_number);
    for (slot_index = 1; slot_index <= g_slot.slot_number; slot_index++) {
        temp_num = g_slot_drv->get_slot_temp_number(slot_index);
        if (temp_num <= 0) {
            SLOT_DBG("slot%u temp number: %d, don't need to create temp_sensor* dirs and attrs.\n",
                slot_index, temp_num);
            continue;
        }
        curr_slot = &g_slot.slot[slot_index - 1];
        curr_slot->temp_number = temp_num;
        if (slotindex_temp_create_kobj_and_attrs(curr_slot) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = slot_index; i > 0; i--) {
        curr_slot = &g_slot.slot[i - 1];
        slotindex_temp_remove_kobj_and_attrs(curr_slot);
    }
    return -EBADRQC;
}

/* delete slot temp_sensor[1-n] directory and attributes*/
static void slot_temp_remove(void)
{
    unsigned int slot_index;
    struct slot_obj_s *curr_slot;

    if (g_slot.slot) {
        for (slot_index = g_slot.slot_number; slot_index > 0; slot_index--) {
            curr_slot = &g_slot.slot[slot_index - 1];
            slotindex_temp_remove_kobj_and_attrs(curr_slot);
            curr_slot->temp_number = 0;
        }
    }
    return;
}
/************************************end of slot temp**************************************/

static int slot_child_obj_create(void)
{
    int ret;

    if (g_slot.slot_number <= 0) {
        SLOT_DBG("slot number: %u, skip to create slot child dirs and attrs.\n",
            g_slot.slot_number);
        return 0;
    }
    /* temp create */
    ret = slot_temp_create();
    if (ret < 0) {
        goto temp_err;
    }
    /* Voltage create */
    ret = slot_vol_create();
    if(ret < 0) {
        goto vol_err;
    }
    /* current create */
    ret = slot_curr_create();
    if(ret < 0) {
        goto curr_err;
    }
    /* fpga create */
    ret = slot_fpga_create();
    if(ret < 0) {
        goto fpga_err;
    }
    /* cplf create */
    ret = slot_cpld_create();
    if(ret < 0) {
        goto cpld_err;
    }
    return 0;
cpld_err:
    slot_fpga_remove();
fpga_err:
    slot_curr_remove();
curr_err:
    slot_vol_remove();
vol_err:
    slot_temp_remove();
temp_err:
    return ret;
}

static void slot_child_obj_remove(void)
{
    slot_cpld_remove();
    slot_fpga_remove();
    slot_curr_remove();
    slot_vol_remove();
    slot_temp_remove();
    return;
}

static void slot_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct slot_obj_s *curr_slot;

    curr_slot = &g_slot.slot[index - 1];
    if (curr_slot->obj) {
        sysfs_remove_group(&curr_slot->obj->kobj, &slot_attr_group);
        switch_kobject_delete(&curr_slot->obj);
        SLOT_DBG("delete slot%u dir and attrs success.\n", index);
    }

    return;
}

static int slot_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct slot_obj_s * curr_slot;

    curr_slot = &g_slot.slot[index - 1];
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "slot%u", index);
    curr_slot->obj = switch_kobject_create(name, parent);
    if (!curr_slot->obj) {
        SLOT_ERR("create %s object error!\n", name);
        return -EBADRQC;
    }
    curr_slot->obj->index = index;
    if (sysfs_create_group(&curr_slot->obj->kobj, &slot_attr_group) != 0) {
        SLOT_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&curr_slot->obj);
        return -EBADRQC;
    }
    SLOT_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

static int slot_sub_create_kobj_and_attrs(struct kobject *parent, int slot_num)
{
    unsigned int slot_index, i;

    g_slot.slot = kzalloc(sizeof(struct slot_obj_s) * slot_num, GFP_KERNEL);
    if (!g_slot.slot) {
        SLOT_ERR("kzalloc slot.slot error, slot number = %d.\n", slot_num);
        return -ENOMEM;
    }

    for(slot_index = 1; slot_index <= slot_num; slot_index++) {
        if(slot_sub_single_create_kobj_and_attrs(parent, slot_index) != 0 ) {
            goto error;
        }
    }
    return 0;
error:
    for(i = slot_index; i > 0; i--) {
        slot_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_slot.slot);
    g_slot.slot = NULL;
    return -EBADRQC;
}

/* create slot[1-n] directory and attributes*/
static int slot_sub_create(void)
{
    int ret;

    ret = slot_sub_create_kobj_and_attrs(&g_slot_obj->kobj, g_slot.slot_number);
    return ret;
}

/* delete slot[1-n] directory and attributes*/
static void slot_sub_remove(void)
{
    unsigned int slot_index;

    if (g_slot.slot) {
        for (slot_index = g_slot.slot_number; slot_index > 0; slot_index--) {
            slot_sub_single_remove_kobj_and_attrs(slot_index);
        }
        kfree(g_slot.slot);
        g_slot.slot = NULL;
    }
    g_slot.slot_number = 0;
    return;
}

/* create slot directory and number attributes*/
static int slot_root_create(void)
{
    g_slot_obj = switch_kobject_create("slot", NULL);
    if (!g_slot_obj) {
        SLOT_ERR("switch_kobject_create slot error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_slot_obj->kobj, &slot_root_attr_group) != 0) {
        switch_kobject_delete(&g_slot_obj);
        SLOT_ERR("create slot dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete slot directory and number attributes*/
static void slot_root_remove(void)
{
    if (g_slot_obj) {
        sysfs_remove_group(&g_slot_obj->kobj, &slot_root_attr_group);
        switch_kobject_delete(&g_slot_obj);
    }

    return;
}

int s3ip_sysfs_slot_drivers_register(struct s3ip_sysfs_slot_drivers_s *drv)
{
    int ret, slot_num;

    SLOT_INFO("s3ip_sysfs_slot_drivers_register...\n");
    if (g_slot_drv) {
        SLOT_ERR("g_slot_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_slot_number);
    g_slot_drv = drv;

    slot_num = g_slot_drv->get_slot_number();
    if (slot_num <= 0) {
        SLOT_ERR("slot number: %d, don't need to create slot dirs and attrs.\n", slot_num);
        g_slot_drv = NULL;
        return -EINVAL;
    }

    memset(&g_slot, 0, sizeof(struct slot_s));
    g_slot.slot_number = slot_num;
    ret = slot_root_create();
    if (ret < 0) {
        SLOT_ERR("create slot root dir and attrs failed, ret: %d\n", ret);
        g_slot_drv = NULL;
        return ret;
    }

    ret = slot_sub_create();
    if (ret < 0) {
        SLOT_ERR("create slot sub dir and attrs failed, ret: %d\n", ret);
        slot_root_remove();
        g_slot_drv = NULL;
        return ret;
    }

    ret = slot_child_obj_create();
    if (ret < 0) {
        SLOT_ERR("create slot child dir and attrs failed, ret: %d\n", ret);
        slot_sub_remove();
        slot_root_remove();
        g_slot_drv = NULL;
        return ret;
    }
    SLOT_INFO("s3ip_sysfs_slot_drivers_register success.\n");
    return 0;
}

void s3ip_sysfs_slot_drivers_unregister(void)
{
    if (g_slot_drv) {
        slot_child_obj_remove();
        slot_sub_remove();
        slot_root_remove();
        g_slot_drv = NULL;
        SLOT_DBG("s3ip_sysfs_slot_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_slot_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_slot_drivers_unregister);
module_param(g_slot_loglevel, int, 0644);
MODULE_PARM_DESC(g_slot_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
