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
static bool g_psu_present_debug = 0;

#define PSU_RESET_ENABLE_VALUE          (1)
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
    struct bin_attribute bin;
    int psu_creat_bin_flag;
};

struct psu_s {
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

static ssize_t psu_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_attr);

    psu_index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);
    return g_psu_drv->get_psu_attr(psu_index, tmp_attr->type, buf, PAGE_SIZE);
}

static ssize_t psu_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_type);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_type(psu_index, buf, PAGE_SIZE);
}

static ssize_t psu_sensor_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_sensor_attr);

    psu_index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);
    return g_psu_drv->get_psu_sensor_attr(psu_index, tmp_attr->type, buf, PAGE_SIZE);
}

static ssize_t psu_present_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret, res;
    char debug_file_buf[DEBUG_FILE_SIZE];

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_present_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    ret = g_psu_drv->get_psu_present_status(psu_index, buf, PAGE_SIZE);
    if (ret < 0) {
        PSU_ERR("get psu%u present status failed, ret: %d\n", psu_index, ret);
        return ret;
    }

    if (g_psu_present_debug) {
        PSU_INFO("s3ip sysfs psu present debug is enable\n");
        if (strcmp(buf, DEV_ABSENT_STR) == 0) {
            PSU_DBG("psu%d absent, return act value\n", psu_index);
            return ret;
        }

        if ((strncmp(buf, SWITCH_DEV_NO_SUPPORT, strlen(SWITCH_DEV_NO_SUPPORT)) == 0) || (strncmp(buf, SWITCH_DEV_ERROR, strlen(SWITCH_DEV_ERROR)) == 0)) {
            PSU_DBG("psu%d status sysfs unsupport or error\n", psu_index);
            return ret;
        }

        mem_clear(debug_file_buf, sizeof(debug_file_buf));
        res = dev_debug_file_read(SINGLE_PSU_PRESENT_DEBUG_FILE, psu_index, debug_file_buf, sizeof(debug_file_buf));
        if (res) {
            PSU_ERR("psu%u present debug file read failed, ret: %d\n", psu_index, res);
            return ret;
        }

        if ((strcmp(debug_file_buf, DEV_PRESEN_STR) == 0) || (strcmp(debug_file_buf, DEV_ABSENT_STR) == 0)) {
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%s", debug_file_buf);
        } else {
            PSU_ERR("psu%d present debug file value err, value: %s, not 0 or 1\n", psu_index, debug_file_buf);
            return ret;
        }
    }

    return ret;
}

static ssize_t get_psu_status_pmbus_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_status_pmbus);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_status_pmbus(psu_index, buf, PAGE_SIZE);
}

static ssize_t get_psu_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_status(psu_index, buf, PAGE_SIZE);
}

static ssize_t get_psu_hw_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_hw_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_hw_status(psu_index, buf, PAGE_SIZE);
}

static ssize_t get_psu_hw_detail_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_hw_detail_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_hw_detail_status(psu_index, buf, PAGE_SIZE);
}

static ssize_t get_psu_alarm_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_alarm);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_alarm(psu_index, buf, PAGE_SIZE);
}

static ssize_t psu_out_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_out_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_out_status(psu_index, buf, PAGE_SIZE);
}

static ssize_t psu_in_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_in_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_in_status(psu_index, buf, PAGE_SIZE);
}

ssize_t psu_fan_ratio_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_fan_ratio);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_fan_ratio(psu_index, buf, PAGE_SIZE);
}

static ssize_t psu_fan_ratio_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int psu_index;
    int ret, ratio;

    check_p(g_psu_drv);
    check_p(g_psu_drv->set_psu_fan_ratio);

    psu_index = obj->index;
    ratio = 0;
    ret = kstrtoint(buf, 0, &ratio);
    if (ret != 0) {
        PSU_ERR("invaild psu fan ratio ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }
    if (ratio < 0 || ratio > 100) {
        PSU_ERR("param invalid, can not set ratio: %d.\n", ratio);
        return -EINVAL;
    }
    PSU_DBG("psu index: %u, ratio: %d\n", psu_index, ratio);
    ret = g_psu_drv->set_psu_fan_ratio(psu_index, ratio);
    if (ret < 0) {
        PSU_ERR("set psu%u ratio: %d failed, ret: %d\n",
            psu_index, ratio, ret);
        return ret;
    }
    PSU_DBG("set psu%u, ratio: %d success\n", psu_index, ratio);
    return count;
}

static ssize_t psu_led_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_led_status);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_led_status(psu_index, buf, PAGE_SIZE);
}

static ssize_t psu_temp_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index, temp_index;
    struct switch_obj *p_obj;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_temp_attr);

    p_obj = to_switch_obj(obj->kobj.parent);
    psu_index = p_obj->index;
    temp_index = obj->index;
    PSU_DBG("psu index: %u, temp index: %u\n", psu_index, temp_index);

    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);
    return g_psu_drv->get_psu_temp_attr(psu_index, temp_index, tmp_attr->type, buf, PAGE_SIZE);
}

static ssize_t psu_attr_threshold_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_attr_threshold);

    psu_index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);
    return g_psu_drv->get_psu_attr_threshold(psu_index, tmp_attr->type, buf, PAGE_SIZE);
}

static ssize_t psu_eeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                               char *buf, loff_t offset, size_t count)
{
    struct switch_obj *psu_obj;
    ssize_t rd_len;
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->read_psu_eeprom_data);

    psu_obj = to_switch_obj(kobj);
    psu_index = psu_obj->index;
    mem_clear(buf,  count);
    rd_len = g_psu_drv->read_psu_eeprom_data(psu_index, buf, offset, count);
    if (rd_len < 0) {
        PSU_ERR("read psu%u eeprom data error, offset: 0x%llx, read len: %zu, ret: %zd.\n",
            psu_index, offset, count, rd_len);
        return rd_len;
    }

    PSU_DBG("read psu%u eeprom data success, offset:0x%llx, read len:%zu, really read len:%zd.\n",
        psu_index, offset, count, rd_len);

    return rd_len;
}

static ssize_t psu_blackbox_path_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_blackbox_path);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_blackbox_path(psu_index, buf, PAGE_SIZE);
}

static ssize_t psu_pmbus_info_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_pmbus_info);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_pmbus_info(psu_index, buf, PAGE_SIZE);
}

static ssize_t psu_clear_blackbox_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int psu_index;
    int ret;
    uint8_t value;

    check_p(g_psu_drv);
    check_p(g_psu_drv->clear_psu_blackbox);

    psu_index = obj->index;
    value = 0;
    ret = kstrtou8(buf, 0, &value);
    if (ret != 0) {
        PSU_ERR("Invaild value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }
    if (value != 1) {
        PSU_ERR("Invaild value: %u, only support write 1 to clear psu blackbox information\n", value);
        return -EINVAL;
    }
    PSU_DBG("psu index: %u, clear psu blackbox information\n", psu_index);
    ret = g_psu_drv->clear_psu_blackbox(psu_index, value);
    if (ret < 0) {
        PSU_ERR("clear psu%u blackbox information failed, ret: %d\n",
            psu_index, ret);
        return ret;
    }
    PSU_DBG("clear psu%u blackbox information success\n", psu_index);
    return count;
}

static ssize_t get_psu_support_upgrade_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_support_upgrade);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_support_upgrade(psu_index, buf, PAGE_SIZE);
}

static ssize_t get_psu_upgrade_active_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->get_psu_upgrade_active_type);

    psu_index = obj->index;
    PSU_DBG("psu index: %u\n", psu_index);
    return g_psu_drv->get_psu_upgrade_active_type(psu_index, buf, PAGE_SIZE);
}

static ssize_t psu_reset_store(struct switch_obj *obj, struct switch_attribute *attr,
                                  const char* buf, size_t count)
{
    int ret, value;
    unsigned int psu_index;

    check_p(g_psu_drv);
    check_p(g_psu_drv->set_psu_reset);

    psu_index = obj->index;
    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        PSU_ERR("Invaild value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }
    if (value != PSU_RESET_ENABLE_VALUE) {
        PSU_ERR("Invaild value: %u, only support write 1 to reset psu\n", value);
        return -EINVAL;
    }
    PSU_DBG("psu index: %u, reset psu\n", psu_index);
    ret = g_psu_drv->set_psu_reset(psu_index, 1);
    if (ret < 0) {
        PSU_ERR("clear psu%u blackbox information failed, ret: %d\n",
            psu_index, ret);
        return ret;
    }
    PSU_DBG("clear psu%u blackbox information success\n", psu_index);

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
static struct switch_attribute psu_type_attr = __ATTR(type, S_IRUGO, psu_type_show, NULL);
static struct switch_attribute psu_num_temps_attr = __ATTR(num_temp_sensors, S_IRUGO, psu_temp_number_show, NULL);
static struct switch_attribute psu_present_attr = __ATTR(present, S_IRUGO, psu_present_status_show, NULL);
static struct switch_attribute status_fr_pmbus_attr = __ATTR(status_fr_pmbus, S_IRUGO, get_psu_status_pmbus_show, NULL);
static struct switch_attribute psu_status_attr = __ATTR(status, S_IRUGO, get_psu_status_show, NULL);
static struct switch_attribute psu_hw_status_attr = __ATTR(hw_status, S_IRUGO, get_psu_hw_status_show, NULL);
static struct switch_attribute psu_hw_detail_status_attr = __ATTR(hw_detail_status, S_IRUGO, get_psu_hw_detail_status_show, NULL);
static struct switch_attribute psu_alarm_attr = __ATTR(alarm, S_IRUGO, get_psu_alarm_show, NULL);
static struct switch_attribute psu_out_status_attr = __ATTR(out_status, S_IRUGO, psu_out_status_show, NULL);
static struct switch_attribute psu_in_status_attr = __ATTR(in_status, S_IRUGO, psu_in_status_show, NULL);
static struct switch_attribute psu_fan_ratio_attr = __ATTR(fan_ratio, S_IRUGO | S_IWUSR, psu_fan_ratio_show, psu_fan_ratio_store);
static struct switch_attribute psu_led_status_attr = __ATTR(led_status, S_IRUGO, psu_led_status_show, NULL);
static struct switch_attribute psu_blackbox_path_attr = __ATTR(blackbox_path, S_IRUGO, psu_blackbox_path_show, NULL);
static struct switch_attribute psu_pmbus_info_attr = __ATTR(pmbus_info, S_IRUGO, psu_pmbus_info_show, NULL);
static struct switch_attribute psu_clear_blackbox_attr = __ATTR(clear_blackbox, S_IWUSR, NULL, psu_clear_blackbox_store);
static struct switch_attribute psu_support_upgrade_attr = __ATTR(support_upgrade, S_IRUGO, get_psu_support_upgrade_show, NULL);
static struct switch_attribute psu_upgrade_active_type_attr = __ATTR(upgrade_active_type, S_IRUGO, get_psu_upgrade_active_type_show, NULL);
static struct switch_attribute psu_reset_attr = __ATTR(psu_reset, S_IWUSR, NULL, psu_reset_store);
static SWITCH_DEVICE_ATTR(model_name, S_IRUGO, psu_attr_show, NULL, DFD_DEV_INFO_TYPE_PART_NAME);
static SWITCH_DEVICE_ATTR(vendor, S_IRUGO, psu_attr_show, NULL, DFD_DEV_INFO_TYPE_VENDOR);
static SWITCH_DEVICE_ATTR(date, S_IRUGO, psu_attr_show, NULL, DFD_DEV_INFO_TYPE_ASSET_TAG);
static SWITCH_DEVICE_ATTR(serial_number, S_IRUGO, psu_attr_show, NULL, DFD_DEV_INFO_TYPE_SN);
static SWITCH_DEVICE_ATTR(part_number, S_IRUGO, psu_attr_show, NULL, DFD_DEV_INFO_TYPE_PART_NUMBER);
static SWITCH_DEVICE_ATTR(hardware_version, S_IRUGO, psu_attr_show, NULL, DFD_DEV_INFO_TYPE_HW_INFO);
static SWITCH_DEVICE_ATTR(out_max_power, S_IRUGO, psu_attr_show, NULL, DFD_DEV_INFO_TYPE_MAX_OUTPUT_POWRER);
static SWITCH_DEVICE_ATTR(fan_direction, S_IRUGO, psu_attr_show, NULL, DFD_DEV_INFO_TYPE_FAN_DIRECTION);
static SWITCH_DEVICE_ATTR(fan_speed_cal, S_IRUGO, psu_attr_show, NULL, DFD_DEV_INFO_TYPE_SPEED_CAL);

static SWITCH_DEVICE_ATTR(in_curr, S_IRUGO, psu_sensor_attr_show, NULL, PSU_IN_CURR);
static SWITCH_DEVICE_ATTR(in_vol, S_IRUGO, psu_sensor_attr_show, NULL, PSU_IN_VOL);
static SWITCH_DEVICE_ATTR(in_power, S_IRUGO, psu_sensor_attr_show, NULL, PSU_IN_POWER);
static SWITCH_DEVICE_ATTR(out_curr, S_IRUGO, psu_sensor_attr_show, NULL, PSU_OUT_CURR);
static SWITCH_DEVICE_ATTR(out_vol, S_IRUGO, psu_sensor_attr_show, NULL, PSU_OUT_VOL);
static SWITCH_DEVICE_ATTR(out_power, S_IRUGO, psu_sensor_attr_show, NULL, PSU_OUT_POWER);
static SWITCH_DEVICE_ATTR(fan_speed, S_IRUGO, psu_sensor_attr_show, NULL, PSU_FAN_SPEED);

static SWITCH_DEVICE_ATTR(in_vol_max, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_VOL_MAX);
static SWITCH_DEVICE_ATTR(in_vol_high, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_VOL_HIGH);
static SWITCH_DEVICE_ATTR(in_vol_low, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_VOL_LOW);
static SWITCH_DEVICE_ATTR(in_vol_min, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_VOL_MIN);
static SWITCH_DEVICE_ATTR(in_curr_max, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_CURR_MAX);
static SWITCH_DEVICE_ATTR(in_curr_high, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_CURR_HIGH);
static SWITCH_DEVICE_ATTR(in_curr_low, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_CURR_LOW);
static SWITCH_DEVICE_ATTR(in_curr_min, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_CURR_MIN);
static SWITCH_DEVICE_ATTR(in_power_max, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_POWER_MAX);
static SWITCH_DEVICE_ATTR(in_power_high, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_POWER_HIGH);
static SWITCH_DEVICE_ATTR(in_power_low, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_POWER_LOW);
static SWITCH_DEVICE_ATTR(in_power_min, S_IRUGO, psu_attr_threshold_show, NULL, PSU_IN_POWER_MIN);
static SWITCH_DEVICE_ATTR(out_vol_max, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_VOL_MAX);
static SWITCH_DEVICE_ATTR(out_vol_high, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_VOL_HIGH);
static SWITCH_DEVICE_ATTR(out_vol_low, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_VOL_LOW);
static SWITCH_DEVICE_ATTR(out_vol_min, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_VOL_MIN);
static SWITCH_DEVICE_ATTR(out_curr_max, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_CURR_MAX);
static SWITCH_DEVICE_ATTR(out_curr_high, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_CURR_HIGH);
static SWITCH_DEVICE_ATTR(out_curr_low, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_CURR_LOW);
static SWITCH_DEVICE_ATTR(out_curr_min, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_CURR_MIN);
static SWITCH_DEVICE_ATTR(out_power_max, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_POWER_MAX);
static SWITCH_DEVICE_ATTR(out_power_high, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_POWER_HIGH);
static SWITCH_DEVICE_ATTR(out_power_low, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_POWER_LOW);
static SWITCH_DEVICE_ATTR(out_power_min, S_IRUGO, psu_attr_threshold_show, NULL, PSU_OUT_POWER_MIN);
static SWITCH_DEVICE_ATTR(fan_speed_max, S_IRUGO, psu_attr_threshold_show, NULL, PSU_FAN_SPEED_MAX);
static SWITCH_DEVICE_ATTR(fan_speed_min, S_IRUGO, psu_attr_threshold_show, NULL, PSU_FAN_SPEED_MIN);

static struct attribute *psu_attrs[] = {
    &psu_type_attr.attr,
    &psu_num_temps_attr.attr,
    &psu_present_attr.attr,
    &status_fr_pmbus_attr.attr,
    &psu_status_attr.attr,
    &psu_hw_status_attr.attr,
    &psu_hw_detail_status_attr.attr,
    &psu_alarm_attr.attr,
    &psu_out_status_attr.attr,
    &psu_in_status_attr.attr,
    &psu_fan_ratio_attr.attr,
    &psu_led_status_attr.attr,
    &psu_blackbox_path_attr.attr,
    &psu_pmbus_info_attr.attr,
    &psu_clear_blackbox_attr.attr,
    &psu_support_upgrade_attr.attr,
    &psu_upgrade_active_type_attr.attr,
    &psu_reset_attr.attr,
    &switch_dev_attr_model_name.switch_attr.attr,
    &switch_dev_attr_vendor.switch_attr.attr,
    &switch_dev_attr_date.switch_attr.attr,
    &switch_dev_attr_serial_number.switch_attr.attr,
    &switch_dev_attr_part_number.switch_attr.attr,
    &switch_dev_attr_hardware_version.switch_attr.attr,
    &switch_dev_attr_out_max_power.switch_attr.attr,
    &switch_dev_attr_fan_direction.switch_attr.attr,
    &switch_dev_attr_fan_speed_cal.switch_attr.attr,

    &switch_dev_attr_in_curr.switch_attr.attr,
    &switch_dev_attr_in_vol.switch_attr.attr,
    &switch_dev_attr_in_power.switch_attr.attr,
    &switch_dev_attr_out_curr.switch_attr.attr,
    &switch_dev_attr_out_vol.switch_attr.attr,
    &switch_dev_attr_out_power.switch_attr.attr,
    &switch_dev_attr_fan_speed.switch_attr.attr,

    &switch_dev_attr_in_vol_max.switch_attr.attr,
    &switch_dev_attr_in_vol_high.switch_attr.attr,
    &switch_dev_attr_in_vol_low.switch_attr.attr,
    &switch_dev_attr_in_vol_min.switch_attr.attr,
    &switch_dev_attr_in_curr_max.switch_attr.attr,
    &switch_dev_attr_in_curr_high.switch_attr.attr,
    &switch_dev_attr_in_curr_low.switch_attr.attr,
    &switch_dev_attr_in_curr_min.switch_attr.attr,
    &switch_dev_attr_in_power_max.switch_attr.attr,
    &switch_dev_attr_in_power_high.switch_attr.attr,
    &switch_dev_attr_in_power_low.switch_attr.attr,
    &switch_dev_attr_in_power_min.switch_attr.attr,
    &switch_dev_attr_out_vol_max.switch_attr.attr,
    &switch_dev_attr_out_vol_high.switch_attr.attr,
    &switch_dev_attr_out_vol_low.switch_attr.attr,
    &switch_dev_attr_out_vol_min.switch_attr.attr,
    &switch_dev_attr_out_curr_max.switch_attr.attr,
    &switch_dev_attr_out_curr_high.switch_attr.attr,
    &switch_dev_attr_out_curr_low.switch_attr.attr,
    &switch_dev_attr_out_curr_min.switch_attr.attr,
    &switch_dev_attr_out_power_max.switch_attr.attr,
    &switch_dev_attr_out_power_high.switch_attr.attr,
    &switch_dev_attr_out_power_low.switch_attr.attr,
    &switch_dev_attr_out_power_min.switch_attr.attr,
    &switch_dev_attr_fan_speed_max.switch_attr.attr,
    &switch_dev_attr_fan_speed_min.switch_attr.attr,
    NULL,
};

static struct attribute_group psu_attr_group = {
    .attrs = psu_attrs,
};

/*******************************psu temp[1-n] dir and attrs*******************************************/
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, psu_temp_attr_show, NULL, WB_SENSOR_ALIAS);
static SWITCH_DEVICE_ATTR(type, S_IRUGO, psu_temp_attr_show, NULL, WB_SENSOR_TYPE);
static SWITCH_DEVICE_ATTR(value, S_IRUGO, psu_temp_attr_show, NULL, WB_SENSOR_INPUT);
static SWITCH_DEVICE_ATTR(max, S_IRUGO, psu_temp_attr_show, NULL, WB_SENSOR_MAX);
static SWITCH_DEVICE_ATTR(high, S_IRUGO, psu_temp_attr_show, NULL, WB_SENSOR_HIGH);
static SWITCH_DEVICE_ATTR(low, S_IRUGO, psu_temp_attr_show, NULL, WB_SENSOR_LOW);
static SWITCH_DEVICE_ATTR(min, S_IRUGO, psu_temp_attr_show, NULL, WB_SENSOR_MIN);

static struct attribute *psu_temp_attrs[] = {
    &switch_dev_attr_alias.switch_attr.attr,
    &switch_dev_attr_type.switch_attr.attr,
    &switch_dev_attr_value.switch_attr.attr,
    &switch_dev_attr_max.switch_attr.attr,
    &switch_dev_attr_high.switch_attr.attr,
    &switch_dev_attr_low.switch_attr.attr,
    &switch_dev_attr_min.switch_attr.attr,
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
    mem_clear(name, sizeof(name));
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
    for(i = temp_index - 1; i > 0; i--) {
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
    for(i = psu_index - 1; i > 0; i--) {
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

/* create psu* eeprom attributes */
static int psu_sub_single_create_eeprom_attrs(unsigned int index)
{
    int ret, eeprom_size;
    struct psu_obj_s *curr_psu;

    check_p(g_psu_drv->get_psu_eeprom_size);
    eeprom_size = g_psu_drv->get_psu_eeprom_size(index);
    if (eeprom_size <= 0) {
        PSU_INFO("psu%u, eeprom_size: %d, don't need to creat eeprom attr.\n",
            index, eeprom_size);
        return 0;
    }

    curr_psu = &g_psu.psu[index - 1];
    sysfs_bin_attr_init(&curr_psu->bin);
    curr_psu->bin.attr.name = "eeprom";
    curr_psu->bin.attr.mode = 0444;
    curr_psu->bin.read = psu_eeprom_read;
    curr_psu->bin.size = eeprom_size;

    ret = sysfs_create_bin_file(&curr_psu->obj->kobj, &curr_psu->bin);
    if (ret) {
        PSU_ERR("psu%u, create eeprom bin error, ret: %d. \n", index, ret);
        return -EBADRQC;
    }

    PSU_DBG("psu%u, create bin file success, eeprom size:%d.\n", index, eeprom_size);
    curr_psu->psu_creat_bin_flag = 1;
    return 0;
}

static int psu_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct psu_obj_s *curr_psu;

    curr_psu = &g_psu.psu[index - 1];
    if (curr_psu->obj) {
        if (curr_psu->psu_creat_bin_flag) {
            sysfs_remove_bin_file(&curr_psu->obj->kobj, &curr_psu->bin);
            curr_psu->psu_creat_bin_flag = 0;
        }
        sysfs_remove_group(&curr_psu->obj->kobj, &psu_attr_group);
        switch_kobject_delete(&curr_psu->obj);
        PSU_DBG("delete psu%u dir and attrs success.\n", index);
    }
    return 0;
}

static int psu_sub_single_create_kobj(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct psu_obj_s *curr_psu;

    curr_psu = &g_psu.psu[index - 1];
    mem_clear(name, sizeof(name));
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


static int psu_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    int ret;

    ret = psu_sub_single_create_kobj(parent, index);
    if (ret < 0) {
        PSU_ERR("create psu%d dir error.\n", index);
        return ret;
    }

    psu_sub_single_create_eeprom_attrs(index);
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

    for (psu_index = 1; psu_index <= psu_num; psu_index++) {
        if (psu_sub_single_create_kobj_and_attrs(parent, psu_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = psu_index - 1; i > 0; i--) {
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

    mem_clear(&g_psu, sizeof(struct psu_s));
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
module_param(g_psu_present_debug, bool, 0644);
MODULE_PARM_DESC(g_psu_present_debug, "the psu present debug switch(0: disable, 1:enable, defalut: 0).\n");
