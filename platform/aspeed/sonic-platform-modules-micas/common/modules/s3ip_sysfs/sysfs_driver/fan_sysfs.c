/*
 * fan_sysfs.c
 *
 * This module create fan kobjects and attributes in /sys/s3ip/fan
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <wb_platform_common.h>

#include "switch.h"
#include "fan_sysfs.h"

static int g_fan_loglevel = 0;
static bool g_fan_status_debug  = 0;
static int fanctrl_mode = 0;
static int fanctrl_fixed_ratio = 100;
static int fanctrl_duration = 0;
static time64_t fanctrl_duration_update_time = 0;
static int fan_redundancy_num = -1;
static bool fanctrl_mode_flag = true;
static DEFINE_MUTEX(fan_mutex);

#define FAN_REDUNDANCY_NUM_MAX          (64)
#define FANCTRL_MODE_ENABLE_VALUE       (1)
#define FANCTRL_MODE_RATIO_MIN          (30)
#define FANCTRL_MODE_RATIO_MAX          (100)

#define MAX_FAN_NUMBER  (32)  /* max fan number in device */
static DECLARE_BITMAP(g_fan_present_debug, MAX_FAN_NUMBER);
/* simulate present condition. 0: pull out, 1: plug in*/

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
    struct bin_attribute bin;
    int fan_creat_bin_flag;
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
    int motor_num;

    index = obj->index;
    FAN_DBG("fan_motor_number_show, fan index: %u\n", index);


    if(test_bit(index - 1, g_fan_present_debug) == 0) {
        FAN_DBG("fan:%d open present debug, return not present.\n", index);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    motor_num = g_fan_drv->get_fan_motor_number(index);
    if (motor_num < 0) {
        if (motor_num == -WB_SYSFS_RV_ABSENT) {
            FAN_DBG("fan:%d get fan absent.\n", index);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
        FAN_ERR("fan:%d get motor number failed, return value: %d.\n", index, motor_num);

        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_ERROR);
    }

    /* present */
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", motor_num);
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

    if(test_bit(fan_index - 1, g_fan_present_debug) == 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", FAN_STATUS_ABSENT);
    }

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

    if(test_bit(fan_index - 1, g_fan_present_debug) == 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", FAN_STATUS_ABSENT);
    }

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
    led_status = 0;
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

static ssize_t fan_motor_speed_threshold_cnt_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_motor_speed_threshold_cnt);

    p_obj = to_switch_obj(obj->kobj.parent);
    fan_index = p_obj->index;
    motor_index = obj->index;
    FAN_DBG("fan index: %u, motor index: %d\n", fan_index, motor_index);
    return g_fan_drv->get_fan_motor_speed_threshold_cnt(fan_index, motor_index, buf, PAGE_SIZE);
}

static ssize_t fan_ratio_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
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

    ratio = 0;
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

static ssize_t fan_attr_dbg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    unsigned int fan_index;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_fan_drv);
    check_p(g_fan_drv->set_fan_dev_debug_fan_attr);

    fan_index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);

    ret = g_fan_drv->set_fan_dev_debug_fan_attr(tmp_attr->type, fan_index, buf);
    if (ret < 0) {
        FAN_ERR("fan index: %u , type: %d , debug_value : %s, set fail.\n", fan_index, tmp_attr->type, buf);
        return -EIO;
    }
    FAN_DBG("fan index: %u , type: %d , debug_value : %s, set success.\n", fan_index, tmp_attr->type, buf);
    return count;
}

static ssize_t fan_direction_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_direction);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    return g_fan_drv->get_fan_direction(fan_index, buf, PAGE_SIZE);
}

static ssize_t fanctrl_mode_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", fanctrl_mode);
}

static ssize_t fanctrl_mode_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    int val;
    int set_flag;

    set_flag = 0;
    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret != 0) {
        FAN_ERR("invaild fan manual mode ratio ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (val != 0 && val != FANCTRL_MODE_ENABLE_VALUE) {
        FAN_ERR("param invalid, can not set ratio: %d.\n", val);
        return -EINVAL;
    }

    mutex_lock(&fan_mutex);
    if (fanctrl_mode_flag) {
        set_flag = 1;
        fanctrl_mode = val;
        if (val == 0) {
            fanctrl_fixed_ratio = FANCTRL_MODE_RATIO_MAX;
            /* fanctrl_mode_flag = false; */
        }

        FAN_DBG("set fanctrl_mode success. fanctrl_mode = %d \n", fanctrl_mode);
    }
    mutex_unlock(&fan_mutex);

    if (set_flag == 0) {
        FAN_ERR("mode not set\n");
        return -EINVAL;
    }

    return count;
}

static ssize_t fanctrl_fixed_ratio_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", fanctrl_fixed_ratio);
}

static ssize_t fanctrl_fixed_ratio_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    int val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret != 0) {
        FAN_ERR("invaild fan manual mode ratio ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (val < FANCTRL_MODE_RATIO_MIN || val > FANCTRL_MODE_RATIO_MAX) {
        FAN_ERR("param invalid, can not set ratio: %d.\n", val);
        return -EINVAL;
    }

    mutex_lock(&fan_mutex);
    fanctrl_fixed_ratio = val;
    fanctrl_mode_flag = true;
    FAN_DBG("set fanctrl_fixed_ratio success. fanctrl_mode = %d fanctrl_mode_flag = %d  \n", fanctrl_fixed_ratio, fanctrl_mode_flag);
    mutex_unlock(&fan_mutex);

    return count;
}

static ssize_t fanctrl_fixed_ratio_min_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", FANCTRL_MODE_RATIO_MIN);
}

static ssize_t fanctrl_duration_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", fanctrl_duration);
}

static ssize_t fanctrl_duration_update_time_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%lld\n", fanctrl_duration_update_time);
}

static ssize_t fanctrl_duration_store(struct switch_obj *obj, struct switch_attribute *attr,
                    const char *buf, size_t count)
{
    int ret, duration, duration_max;
    char *max_buf;

    max_buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!max_buf) {
        FAN_ERR("Failed to allocate memory for max_buf\n");
        return -ENOMEM;
    }

    ret = kstrtoint(buf, 0, &duration);
    if (ret != 0) {
        FAN_ERR("invaild fanctrl_duration ret: %d, buf: %s.\n", ret, buf);
        kfree(max_buf);
        return -EINVAL;
    }

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fanctrl_duration_max);
    ret = g_fan_drv->get_fanctrl_duration_max(max_buf, PAGE_SIZE);
    if (ret < 0) {
        FAN_ERR("Failed to get fanctrl_duration_max, ret: %d\n", ret);
        kfree(max_buf);
        return ret;
    }

    ret = kstrtoint(max_buf, 0, &duration_max);
    if (ret != 0) {
        FAN_ERR("Failed to convert duration_max to integer, ret: %d\n", ret);
        kfree(max_buf);
        return -EINVAL;
    }

    if (duration < 0 || duration > duration_max) {
        FAN_ERR("param invalid, can not set fanctrl_duration: %d.\n", duration);
        kfree(max_buf);
        return -EINVAL;
    }

    mutex_lock(&fan_mutex);
    fanctrl_duration = duration;
    fanctrl_duration_update_time = ktime_get_boottime_seconds();
    FAN_DBG("set fanctrl_duration success. fanctrl_duration = %d\n", fanctrl_duration);
    mutex_unlock(&fan_mutex);

    kfree(max_buf);
    return count;
}

static ssize_t fanctrl_duration_max_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fanctrl_duration_max);

    FAN_DBG("sysfs get fanctrl_duration_max");
    return g_fan_drv->get_fanctrl_duration_max(buf, PAGE_SIZE);
}

static ssize_t fan_redundancy_num_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    if (fan_redundancy_num == -1) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", fan_redundancy_num);
}

static ssize_t fan_redundancy_num_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    int val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret != 0) {
        FAN_ERR("invaild value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (val < 0 || val > FAN_REDUNDANCY_NUM_MAX) {
        FAN_ERR("param invalid, can not set redundancy num: %d.\n", val);
        return -EINVAL;
    }

    fan_redundancy_num = val;
    FAN_DBG("set fan_redundancy_num success. fan_redundancy_num = %d\n", fan_redundancy_num);

    return count;
}

static ssize_t fan_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index;
    struct switch_device_attribute *fan_attr;

    check_p(g_fan_drv);
    check_p(g_fan_drv->get_fan_attr);

    fan_index = obj->index;
    FAN_DBG("fan index: %u\n", fan_index);
    fan_attr = to_switch_device_attr(attr);
    check_p(fan_attr);

    return g_fan_drv->get_fan_attr(fan_index, fan_attr->type, buf, PAGE_SIZE);
}

/*
static ssize_t fan_attr_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int fan_index, value;
    int ret;
    struct switch_device_attribute *fan_attr;

    check_p(g_fan_drv);
    check_p(g_fan_drv->set_fan_attr);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        FAN_ERR("Invaild value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    fan_index = obj->index;
    fan_attr = to_switch_device_attr(attr);
    check_p(fan_attr);
    ret = g_fan_drv->set_fan_attr(fan_index, fan_attr->type, value);
    if (ret < 0) {
        FAN_ERR("set fan%u test reg failed, value:0x%x, ret: %d.\n", fan_index, value, ret);
        return ret;
    }
    FAN_DBG("set fan%u test reg success, value: 0x%x.\n", fan_index, value);
    return count;
}
*/

ssize_t fan_debug_present_get(char *buf)
{
    unsigned int i;

    memset(buf, 0, PAGE_SIZE);
    for (i = 0; i < g_fan.fan_number; i++)
    {
        buf[i] = test_bit(i, g_fan_present_debug) ? '1' : '0';
    }
    buf[i] = '\n';
    return (ssize_t)strlen(buf);
}

ssize_t fan_debug_present_set(const char* buf, size_t count)
{
    unsigned int i;

    for (i = 0; (i < count - 1) && i < g_fan.fan_number; i++) {
        if (buf[i] != '0' && buf[i] != '1') {
            FAN_ERR("param invalid, can not set debug present: %d.\n", buf[i]);
            return -EINVAL;
        }

        if (buf[i] - '0') {
            set_bit(i, g_fan_present_debug);
        } else {
            clear_bit(i, g_fan_present_debug);
        }
    }

    return count;
}

static ssize_t fan_eeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                   char *buf, loff_t offset, size_t count)
{
    struct switch_obj *fan_obj;
    ssize_t ret;
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->read_fan_eeprom_data);

    fan_obj = to_switch_obj(kobj);
    fan_index = fan_obj->index;

    FAN_DBG("start read fan%u eeprom data\n", fan_index);

    if(test_bit(fan_index - 1, g_fan_present_debug) == 0) {
        FAN_DBG("fan:%d open present debug, return not present.\n", fan_index);
        return -ENOENT;
    }

    mem_clear(buf, count);
    ret = g_fan_drv->read_fan_eeprom_data(fan_index, buf, offset, count);
    if (ret < 0) {
        if (ret == -WB_SYSFS_RV_ABSENT) {
            FAN_DBG("read fan%u eeprom data absent.\n", fan_index);
            return -ENOENT;
        }
        FAN_ERR("read fan%u eeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
            fan_index, offset, count, ret);
        return -EIO;
    }

    FAN_DBG("read fan%u eeprom data success, offset:0x%llx, read len:%lu, really read len:%ld.\n",
        fan_index, offset, count, ret);

    return ret;
}

static ssize_t fan_eeprom_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                   char *buf, loff_t offset, size_t count)
{
    struct switch_obj *fan_obj;
    ssize_t ret;
    unsigned int fan_index;

    check_p(g_fan_drv);
    check_p(g_fan_drv->write_fan_eeprom_data);

    fan_obj = to_switch_obj(kobj);
    fan_index = fan_obj->index;

    FAN_DBG("start write fan%u eeprom data\n", fan_index);

    if(test_bit(fan_index - 1, g_fan_present_debug) == 0) {
        FAN_DBG("fan:%d open present debug, return not present.\n", fan_index);
        return -ENOENT;
    }

    ret = g_fan_drv->write_fan_eeprom_data(fan_index, buf, offset, count);
    if (ret < 0) {
        if (ret == -WB_SYSFS_RV_ABSENT) {
            FAN_DBG("write fan%u eeprom data absent.\n", fan_index);
            return -ENOENT;
        }
        FAN_ERR("write fan%u eeprom data error, offset: 0x%llx, write len: %lu, ret: %ld.\n",
            fan_index, offset, count, ret);
        return -EIO;
    }

    FAN_DBG("write fan%u eeprom data success, offset:0x%llx, write len:%lu, really write len:%ld.\n",
        fan_index, offset, count, ret);

    return ret;
}
/************************************fan dir and attrs*******************************************/
static struct switch_attribute fan_number_att = __ATTR(number, S_IRUGO, fan_number_show, NULL);
static struct switch_attribute fanctrl_mode_att = __ATTR(fanctrl_mode, S_IRUGO | S_IWUSR, fanctrl_mode_show, fanctrl_mode_store);
static struct switch_attribute fanctrl_fixed_ratio_att = __ATTR(fanctrl_fixed_ratio, S_IRUGO | S_IWUSR, fanctrl_fixed_ratio_show, fanctrl_fixed_ratio_store);
static struct switch_attribute fanctrl_fixed_ratio_min_att = __ATTR(fanctrl_fixed_ratio_min, S_IRUGO, fanctrl_fixed_ratio_min_show, NULL);
static struct switch_attribute redundancy_num_att = __ATTR(redundancy_num, S_IRUGO | S_IWUSR, fan_redundancy_num_show, fan_redundancy_num_store);
static struct switch_attribute fanctrl_duration_att = __ATTR(fanctrl_duration, S_IRUGO | S_IWUSR, fanctrl_duration_show, fanctrl_duration_store);
static struct switch_attribute fanctrl_duration_update_time_att = __ATTR(fanctrl_duration_update_time, S_IRUGO, fanctrl_duration_update_time_show, NULL);
static struct switch_attribute fanctrl_duration_max_att = __ATTR(fanctrl_duration_max, S_IRUGO, fanctrl_duration_max_show, NULL);

static struct attribute *fan_dir_attrs[] = {
    &fan_number_att.attr,
    &fanctrl_mode_att.attr,
    &fanctrl_fixed_ratio_att.attr,
    &fanctrl_fixed_ratio_min_att.attr,
    &fanctrl_duration_att.attr,
    &fanctrl_duration_update_time_att.attr,
    &fanctrl_duration_max_att.attr,
    &redundancy_num_att.attr,
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
static struct switch_attribute fan_led_status_attr = __ATTR(led_status, S_IRUGO | S_IWUSR, fan_led_status_show, fan_led_status_store);
static struct switch_attribute fan_direction_attr = __ATTR(direction, S_IRUGO, fan_direction_show, NULL);
static struct switch_attribute fan_ratio_attr = __ATTR(ratio, S_IRUGO | S_IWUSR, fan_ratio_show, fan_ratio_store);
static SWITCH_DEVICE_ATTR(present, S_IRUGO | S_IWUSR, fan_present_show, fan_attr_dbg_store, WB_FAN_PRESENT_ATTR);
static SWITCH_DEVICE_ATTR(bus, S_IRUGO, fan_attr_show, NULL, DFD_FAN_BUS_E);
static SWITCH_DEVICE_ATTR(addr, S_IRUGO, fan_attr_show, NULL, DFD_FAN_ADDR_E);
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
static SWITCH_DEVICE_ATTR(status, S_IRUGO, fan_attr_show, NULL, DFD_FAN_STATUS_ROTATION_E);
static struct switch_attribute fan_status_rotation_attr = __ATTR(status_rotation, S_IRUGO, fan_status_show, NULL);
#else
static SWITCH_DEVICE_ATTR(status_rotation, S_IRUGO, fan_attr_show, NULL, DFD_FAN_STATUS_ROTATION_E);
static struct switch_attribute fan_status_attr = __ATTR(status, S_IRUGO, fan_status_show, NULL);
#endif

static struct attribute *fan_attrs[] = {
    &fan_model_name_attr.attr,
    &fan_vendor_attr.attr,
    &fan_sn_attr.attr,
    &fan_pn_attr.attr,
    &fan_hw_attr.attr,
    &fan_num_motors_attr.attr,
    &fan_led_status_attr.attr,
    &fan_direction_attr.attr,
    &fan_ratio_attr.attr,
    &switch_dev_attr_present.switch_attr.attr,
    &switch_dev_attr_bus.switch_attr.attr,
    &switch_dev_attr_addr.switch_attr.attr,
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    &fan_status_rotation_attr.attr,
    &switch_dev_attr_status.switch_attr.attr,
#else
    &fan_status_attr.attr,
    &switch_dev_attr_status_rotation.switch_attr.attr,
#endif
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
static struct switch_attribute motor_speed_threshold_cnt_attr = __ATTR(speed_threshold_cnt, S_IRUGO, fan_motor_speed_threshold_cnt_show, NULL);

static struct attribute *motor_attrs[] = {
    &motor_status_attr.attr,
    &motor_speed_attr.attr,
    &motor_speed_tolerance_attr.attr,
    &motor_speed_target_attr.attr,
    &motor_speed_max_attr.attr,
    &motor_speed_min_attr.attr,
    &motor_speed_threshold_cnt_attr.attr,
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
    for (i = motor_index - 1; i > 0; i--) {
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
    for (i = fan_index - 1; i > 0; i--) {
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
        if (curr_fan->fan_creat_bin_flag) {
            sysfs_remove_bin_file(&curr_fan->obj->kobj, &curr_fan->bin);
            curr_fan->fan_creat_bin_flag = 0;
        }
        sysfs_remove_group(&curr_fan->obj->kobj, &fan_attr_group);
        switch_kobject_delete(&curr_fan->obj);
        FAN_DBG("delete fan%u dir and attrs success.\n", index);
    }
    return 0;
}

/* create eeprom attributes */
static int fan_sub_single_create_eeprom_attrs(unsigned int index)
{
    int ret, eeprom_size;
    struct fan_obj_s *curr_fan;

    check_p(g_fan_drv->get_fan_eeprom_size);
    eeprom_size = g_fan_drv->get_fan_eeprom_size(index);
    if (eeprom_size <= 0) {
        FAN_INFO("fan%u, eeprom_size: %d, don't need to creat eeprom attr.\n",
            index, eeprom_size);
        return 0;
    }

    curr_fan = &g_fan.fan[index - 1];
    sysfs_bin_attr_init(&curr_fan->bin);
    curr_fan->bin.attr.name = "eeprom";
    curr_fan->bin.attr.mode = 0644;
    curr_fan->bin.read = fan_eeprom_read;
    curr_fan->bin.write = fan_eeprom_write;
    curr_fan->bin.size = eeprom_size;

    ret = sysfs_create_bin_file(&curr_fan->obj->kobj, &curr_fan->bin);
    if (ret) {
        FAN_ERR("fan%u, create eeprom bin error, ret: %d. \n", index, ret);
        return -EBADRQC;
    }

    FAN_DBG("fan%u, create bin file success, eeprom size:%d.\n", index, eeprom_size);
    curr_fan->fan_creat_bin_flag = 1;
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

    fan_sub_single_create_eeprom_attrs(index);
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
    for (i = fan_index - 1; i > 0; i--) {
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

    bitmap_fill(g_fan_present_debug, MAX_FAN_NUMBER);
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
EXPORT_SYMBOL(fan_debug_present_get);
EXPORT_SYMBOL(fan_debug_present_set);
module_param(g_fan_loglevel, int, 0644);
MODULE_PARM_DESC(g_fan_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
module_param(g_fan_status_debug, bool, 0644);
MODULE_PARM_DESC(g_fan_status_debug, "the fan present debug switch(0: disable, 1:enable, defalut: 0).\n");
