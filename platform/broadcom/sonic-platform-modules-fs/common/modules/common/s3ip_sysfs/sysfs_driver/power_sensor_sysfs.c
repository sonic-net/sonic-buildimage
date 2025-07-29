/*
 * power_sensor_sysfs.c
 *
 * This module create power sensor kobjects and attributes in /sys/s3ip/power_sensor
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "power_sensor_sysfs.h"

static int g_power_sensor_loglevel = 0;

#define POWER_SENSOR_INFO(fmt, args...) do {                                        \
    if (g_power_sensor_loglevel & INFO) { \
        printk(KERN_INFO "[POWER_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define POWER_SENSOR_ERR(fmt, args...) do {                                        \
    if (g_power_sensor_loglevel & ERR) { \
        printk(KERN_ERR "[POWER_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define POWER_SENSOR_DBG(fmt, args...) do {                                        \
    if (g_power_sensor_loglevel & DBG) { \
        printk(KERN_DEBUG "[POWER_SENSOR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct power_sensor_obj_s {
    struct switch_obj *obj;
};

struct power_sensor_s {
    unsigned int power_number;
    struct power_sensor_obj_s *power;
};

static struct s3ip_sysfs_power_sensor_drivers_s *g_power_sensor_drv = NULL;
static struct power_sensor_s g_power_sensor;
static struct switch_obj *g_power_sensor_obj = NULL;

static ssize_t power_sensor_number_show(struct switch_obj *obj, struct switch_attribute *attr,
                   char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_power_sensor.power_number);
}

static ssize_t power_sensor_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int power_index;

    check_p(g_power_sensor_drv);
    check_p(g_power_sensor_drv->get_main_board_power_value);

    power_index = obj->index;
    POWER_SENSOR_DBG("power index: %u\n", power_index);
    return g_power_sensor_drv->get_main_board_power_value(power_index, buf, PAGE_SIZE);
}

static ssize_t power_sensor_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int power_index;

    check_p(g_power_sensor_drv);
    check_p(g_power_sensor_drv->get_main_board_power_alias);

    power_index = obj->index;
    POWER_SENSOR_DBG("power index: %u\n", power_index);
    return g_power_sensor_drv->get_main_board_power_alias(power_index, buf, PAGE_SIZE);
}

static ssize_t power_sensor_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int power_index;

    check_p(g_power_sensor_drv);
    check_p(g_power_sensor_drv->get_main_board_power_type);

    power_index = obj->index;
    POWER_SENSOR_DBG("power index: %u\n", power_index);
    return g_power_sensor_drv->get_main_board_power_type(power_index, buf, PAGE_SIZE);
}

static ssize_t power_sensor_max_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int power_index;

    check_p(g_power_sensor_drv);
    check_p(g_power_sensor_drv->get_main_board_power_max);

    power_index = obj->index;
    POWER_SENSOR_DBG("power index: %u\n", power_index);
    return g_power_sensor_drv->get_main_board_power_max(power_index, buf, PAGE_SIZE);
}

static ssize_t power_sensor_max_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int power_index;
    int ret;

    check_p(g_power_sensor_drv);
    check_p(g_power_sensor_drv->set_main_board_power_max);

    power_index = obj->index;
    POWER_SENSOR_DBG("power index: %u\n", power_index);
    ret = g_power_sensor_drv->set_main_board_power_max(power_index, buf, count);
    if (ret < 0) {
        POWER_SENSOR_ERR("set power%u max threshold failed, value: %s, count: %zu, ret: %d\n",
            power_index, buf, count, ret);
        return ret;
    }
    POWER_SENSOR_DBG("set power%u max threshold success, value: %s, count: %zu, ret: %d\n",
        power_index, buf, count, ret);
    return count;
}

static ssize_t power_sensor_min_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int power_index;

    check_p(g_power_sensor_drv);
    check_p(g_power_sensor_drv->get_main_board_power_min);

    power_index = obj->index;
    POWER_SENSOR_DBG("power index: %u\n", power_index);
    return g_power_sensor_drv->get_main_board_power_min(power_index, buf, PAGE_SIZE);
}

static ssize_t power_sensor_min_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int power_index;
    int ret;

    check_p(g_power_sensor_drv);
    check_p(g_power_sensor_drv->set_main_board_power_min);

    power_index = obj->index;
    POWER_SENSOR_DBG("power index: %u\n", power_index);
    ret = g_power_sensor_drv->set_main_board_power_min(power_index, buf, count);
    if (ret < 0) {
        POWER_SENSOR_ERR("set power%u min threshold failed, value: %s, count: %zu, ret: %d\n",
            power_index, buf, count, ret);
        return ret;
    }
    POWER_SENSOR_DBG("set power%u min threshold success, value: %s, count: %zu, ret: %d\n",
        power_index, buf, count, ret);
    return count;
}

static ssize_t power_sensor_monitor_flag_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int power_index;

    check_p(g_power_sensor_drv);
    check_p(g_power_sensor_drv->get_main_board_power_monitor_flag);

    power_index = obj->index;
    POWER_SENSOR_DBG("power index: %u\n", power_index);
    return g_power_sensor_drv->get_main_board_power_monitor_flag(power_index, buf, PAGE_SIZE);
}

/************************************power_sensor dir and attrs*******************************************/
static struct switch_attribute num_power_att = __ATTR(number, S_IRUGO, power_sensor_number_show, NULL);

static struct attribute *power_sensor_dir_attrs[] = {
    &num_power_att.attr,
    NULL,
};

static struct attribute_group power_sensor_root_attr_group = {
    .attrs = power_sensor_dir_attrs,
};

/*******************************power1 power2 dir and attrs*******************************************/
static struct switch_attribute power_value_attr = __ATTR(value, S_IRUGO, power_sensor_value_show, NULL);
static struct switch_attribute power_alias_attr = __ATTR(alias, S_IRUGO, power_sensor_alias_show, NULL);
static struct switch_attribute power_type_attr = __ATTR(type, S_IRUGO, power_sensor_type_show, NULL);
static struct switch_attribute power_max_attr = __ATTR(max, S_IRUGO | S_IWUSR, power_sensor_max_show, power_sensor_max_store);
static struct switch_attribute power_min_attr = __ATTR(min,  S_IRUGO | S_IWUSR, power_sensor_min_show, power_sensor_min_store);
static struct switch_attribute power_monitor_flag_attr = __ATTR(monitor_flag,  S_IRUGO, power_sensor_monitor_flag_show, NULL);

static struct attribute *power_sensor_attrs[] = {
    &power_value_attr.attr,
    &power_alias_attr.attr,
    &power_type_attr.attr,
    &power_max_attr.attr,
    &power_min_attr.attr,
    &power_monitor_flag_attr.attr,
    NULL,
};

static struct attribute_group power_sensor_attr_group = {
    .attrs = power_sensor_attrs,
};

static int power_sensor_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct power_sensor_obj_s *power_sensor;

    power_sensor = &g_power_sensor.power[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "power%u", index);
    power_sensor->obj = switch_kobject_create(name, parent);
    if (!power_sensor->obj) {
        POWER_SENSOR_ERR("create %s object error.\n", name);
        return -ENOMEM;
    }

    power_sensor->obj->index = index;
    if (sysfs_create_group(&power_sensor->obj->kobj, &power_sensor_attr_group) != 0) {
        POWER_SENSOR_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&power_sensor->obj);
        return -EBADRQC;
    }
    POWER_SENSOR_DBG("create %s dir and attrs success.\n", name);

    return 0;
}

static void power_sensor_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct power_sensor_obj_s *power_sensor;

    power_sensor = &g_power_sensor.power[index - 1];
    if (power_sensor->obj) {
        sysfs_remove_group(&power_sensor->obj->kobj, &power_sensor_attr_group);
        switch_kobject_delete(&power_sensor->obj);
        POWER_SENSOR_DBG("delete power%u dir and attrs success.\n", index);
    }

    return;
}

static int power_sensor_sub_create_kobj_and_attrs(struct kobject *parent, int power_num)
{
    unsigned int power_index, i;

    g_power_sensor.power = kzalloc(sizeof(struct power_sensor_obj_s) * power_num, GFP_KERNEL);
    if (!g_power_sensor.power) {
        POWER_SENSOR_ERR("kzalloc g_power_sensor.power error, power number: %d.\n", power_num);
        return -ENOMEM;
    }

    for (power_index = 1; power_index <= power_num; power_index++) {
        if (power_sensor_sub_single_create_kobj_and_attrs(parent, power_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = power_index - 1; i > 0; i--) {
        power_sensor_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_power_sensor.power);
    g_power_sensor.power = NULL;
    return -EBADRQC;
}

/* create power[1-n] directory and attributes*/
static int power_sensor_sub_create(void)
{
    int ret;

    ret = power_sensor_sub_create_kobj_and_attrs(&g_power_sensor_obj->kobj, g_power_sensor.power_number);
    return ret;
}

/* delete power[1-n] directory and attributes*/
static void power_sensor_sub_remove(void)
{
    unsigned int power_index;

    if (g_power_sensor.power) {
        for (power_index = g_power_sensor.power_number; power_index > 0; power_index--) {
            power_sensor_sub_single_remove_kobj_and_attrs(power_index);
        }
        kfree(g_power_sensor.power);
        g_power_sensor.power = NULL;
    }

    return;
}

/* create power_sensor directory and number attributes */
static int power_sensor_root_create(void)
{
    g_power_sensor_obj = switch_kobject_create("power_sensor", NULL);
    if (!g_power_sensor_obj) {
        POWER_SENSOR_ERR("switch_kobject_create power_sensor error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_power_sensor_obj->kobj, &power_sensor_root_attr_group) != 0) {
        switch_kobject_delete(&g_power_sensor_obj);
        POWER_SENSOR_ERR("create power_sensor dir attrs error!\n");
        return -EBADRQC;
    }

    return 0;
}

/* delete power_sensor directory and number attributes */
static void power_sensor_root_remove(void)
{
    if (g_power_sensor_obj) {
        sysfs_remove_group(&g_power_sensor_obj->kobj, &power_sensor_root_attr_group);
        switch_kobject_delete(&g_power_sensor_obj);
    }

    return;
}

int s3ip_sysfs_power_sensor_drivers_register(struct s3ip_sysfs_power_sensor_drivers_s *drv)
{
    int ret, power_num;

    POWER_SENSOR_INFO("s3ip_sysfs_power_sensor_drivers_register...\n");
    if (g_power_sensor_drv) {
        POWER_SENSOR_ERR("g_power_sensor_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_power_number);
    g_power_sensor_drv = drv;

    power_num = g_power_sensor_drv->get_main_board_power_number();
    if (power_num <= 0) {
        POWER_SENSOR_ERR("power sensor number: %d, don't need to create power_sensor dirs and attrs.\n",
            power_num);
        g_power_sensor_drv = NULL;
        return -EINVAL;
    }
    mem_clear(&g_power_sensor, sizeof(struct power_sensor_s));
    g_power_sensor.power_number = power_num;
    ret = power_sensor_root_create();
    if (ret < 0) {
        POWER_SENSOR_ERR("create power_sensor root dir and attrs failed, ret: %d\n", ret);
        g_power_sensor_drv = NULL;
        return ret;
    }

    ret = power_sensor_sub_create();
    if (ret < 0) {
        POWER_SENSOR_ERR("create power_sensor sub dir and attrs failed, ret: %d\n", ret);
        power_sensor_root_remove();
        g_power_sensor_drv = NULL;
        return ret;
    }
    POWER_SENSOR_INFO("s3ip_sysfs_power_sensor_drivers_register success\n");
    return ret;
}

void s3ip_sysfs_power_sensor_drivers_unregister(void)
{
    if (g_power_sensor_drv) {
        power_sensor_sub_remove();
        power_sensor_root_remove();
        g_power_sensor_drv = NULL;
        POWER_SENSOR_DBG("s3ip_sysfs_power_sensor_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_power_sensor_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_power_sensor_drivers_unregister);
module_param(g_power_sensor_loglevel, int, 0644);
MODULE_PARM_DESC(g_power_sensor_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
