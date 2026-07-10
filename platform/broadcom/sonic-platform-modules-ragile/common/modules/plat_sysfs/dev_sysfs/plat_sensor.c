/*
 * plat_sensor.c
 * Original Author: support 2020-02-17
 *
 * This module create sensor kobjects and attributes in /sys/wb_plat/sensor
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0          support                 2020-02-17          Initial version
 */

#include <linux/slab.h>

#include "./include/plat_switch.h"
#include "./include/sysfs_common.h"

#define SENSOR_INFO(fmt, args...) LOG_INFO("sensor: ", fmt, ##args)
#define SENSOR_ERR(fmt, args...)  LOG_ERR("sensor: ", fmt, ##args)
#define SENSOR_DBG(fmt, args...)  LOG_DBG("sensor: ", fmt, ##args)

struct sensor_t {
    unsigned int in_number;
    unsigned int temp_number;
    struct sensor_in_t *in;
    struct sensor_temp_t *temp;
};

struct sensor_temp_t {
    struct switch_obj *obj;
};

struct sensor_in_t {
    struct switch_obj *obj;
};

static int g_loglevel = 0;
static struct switch_drivers_t *g_drv = NULL;
static struct sensor_t g_sensor;
static struct switch_obj *g_sensor_obj = NULL;

static ssize_t sensor_temp_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_sensor.temp_number);
}

static ssize_t sensor_in_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_sensor.in_number);
}

static ssize_t sensor_voltage_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int in_index;
    int ret;
    struct switch_device_attribute *in_attr;

    check_p(g_drv);
    check_p(g_drv->get_voltage_info);
    in_index = obj->index;

    in_attr = to_switch_device_attr(attr);
    check_p(in_attr);
    SENSOR_DBG("sensor_in_show, in index:0x%x, in type:0x%x\n",in_index, in_attr->type);
    ret = g_drv->get_voltage_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, in_index, in_attr->type, buf);
    if (ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t sensor_temp_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int temp_index;
    int ret;
    struct switch_device_attribute *temp_attr;

    check_p(g_drv);
    check_p(g_drv->get_temp_info);
    temp_index = obj->index;

    temp_attr = to_switch_device_attr(attr);
    check_p(temp_attr);
    SENSOR_DBG("sensor_temp_show, temp index:0x%x, temp type:0x%x\n", temp_index, temp_attr->type);
    ret = g_drv->get_temp_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, temp_attr->type, buf);
    if (ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }
    return ret;
}

/************************************sensor dir and attrs*******************************************/
static struct switch_attribute num_temp_att = __ATTR(num_temp_sensors, S_IRUGO, sensor_temp_number_show, NULL);
static struct switch_attribute num_in_att = __ATTR(num_in_sensors, S_IRUGO, sensor_in_number_show, NULL);

static struct attribute *sensor_dir_attrs[] = {
    &num_temp_att.attr,
    &num_in_att.attr,
    NULL,
};

static struct attribute_group sensor_root_attr_group = {
    .attrs = sensor_dir_attrs,
};

/*******************************temp0 temp1 dir and attrs*******************************************/
static SWITCH_DEVICE_ATTR(temp_input, S_IRUGO, sensor_temp_show, NULL, WB_SENSOR_INPUT);
static SWITCH_DEVICE_ATTR(temp_alias, S_IRUGO, sensor_temp_show, NULL, WB_SENSOR_ALIAS);
static SWITCH_DEVICE_ATTR(temp_type, S_IRUGO, sensor_temp_show, NULL, WB_SENSOR_TYPE);
static SWITCH_DEVICE_ATTR(temp_max, S_IRUGO, sensor_temp_show, NULL, WB_SENSOR_MAX);
static SWITCH_DEVICE_ATTR(temp_max_hyst, S_IRUGO, sensor_temp_show, NULL, WB_SENSOR_MAX_HYST);
static SWITCH_DEVICE_ATTR(temp_min, S_IRUGO, sensor_temp_show, NULL, WB_SENSOR_MIN);

static struct attribute *sensor_temp_attrs[] = {
    &switch_dev_attr_temp_input.switch_attr.attr,
    &switch_dev_attr_temp_alias.switch_attr.attr,
    &switch_dev_attr_temp_type.switch_attr.attr,
    &switch_dev_attr_temp_max.switch_attr.attr,
    &switch_dev_attr_temp_max_hyst.switch_attr.attr,
    &switch_dev_attr_temp_min.switch_attr.attr,
    NULL,
};

static struct attribute_group sensor_temp_attr_group = {
    .attrs = sensor_temp_attrs,
};

/*******************************in0 in1 dir and attrs*******************************************/
static SWITCH_DEVICE_ATTR(in_input, S_IRUGO, sensor_voltage_show, NULL, WB_SENSOR_INPUT);
static SWITCH_DEVICE_ATTR(in_alias, S_IRUGO, sensor_voltage_show, NULL, WB_SENSOR_ALIAS);
static SWITCH_DEVICE_ATTR(in_type, S_IRUGO, sensor_voltage_show, NULL, WB_SENSOR_TYPE);
static SWITCH_DEVICE_ATTR(in_max, S_IRUGO, sensor_voltage_show, NULL, WB_SENSOR_MAX);
static SWITCH_DEVICE_ATTR(in_min, S_IRUGO, sensor_voltage_show, NULL, WB_SENSOR_MIN);
static SWITCH_DEVICE_ATTR(in_crit, S_IRUGO, sensor_voltage_show, NULL, WB_SENSOR_CRIT);

static struct attribute *sensor_in_attrs[] = {
    &switch_dev_attr_in_input.switch_attr.attr,
    &switch_dev_attr_in_alias.switch_attr.attr,
    &switch_dev_attr_in_type.switch_attr.attr,
    &switch_dev_attr_in_max.switch_attr.attr,
    &switch_dev_attr_in_min.switch_attr.attr,
    &switch_dev_attr_in_crit.switch_attr.attr,
    NULL,
};

static struct attribute_group sensor_in_attr_group = {
    .attrs = sensor_in_attrs,
};

static int sensor_in_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct sensor_in_t *curr_sensor;

    curr_sensor = &g_sensor.in[index - 1];
    SENSOR_DBG("create sensor in%d ...\n", index);
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "in%d", index);
    curr_sensor->obj = wb_plat_kobject_create(name, parent);
    if (!curr_sensor->obj) {
        SENSOR_ERR("create sensor in%d object error!\n", index);
        return -EBADRQC;
    }
    curr_sensor->obj->index = index;
    if (sysfs_create_group(&curr_sensor->obj->kobj, &sensor_in_attr_group) != 0) {
        SENSOR_ERR("create sensor in%d attrs error.\n", index);
        wb_plat_kobject_delete(&curr_sensor->obj);
        return -EBADRQC;
    }
    SENSOR_DBG("create sensor in%d ok.\n", index);
    return 0;

}

static void sensor_in_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct sensor_in_t *curr_in;

    curr_in = &g_sensor.in[index - 1];
    if (curr_in->obj) {
        sysfs_remove_group(&curr_in->obj->kobj, &sensor_in_attr_group);
        wb_plat_kobject_delete(&curr_in->obj);
        SENSOR_DBG("delete in%d.\n", index);
    }
    return;
}

static int sensor_temp_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct sensor_temp_t *curr_sensor;

    curr_sensor = &g_sensor.temp[index - 1];
    SENSOR_DBG("create sensor temp%d ...\n", index);
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "temp%d", index);
    curr_sensor->obj = wb_plat_kobject_create(name, parent);
    if (!curr_sensor->obj) {
        SENSOR_ERR("create sensor temp%d object error!\n", index);
        return -EBADRQC;
    }
    curr_sensor->obj->index = index;
    if (sysfs_create_group(&curr_sensor->obj->kobj, &sensor_temp_attr_group) != 0) {
        SENSOR_ERR("create sensor temp%d attrs error.\n", index);
        wb_plat_kobject_delete(&curr_sensor->obj);
        return -EBADRQC;
    }
    SENSOR_DBG("create sensor temp%d ok.\n", index);
    return 0;

}

static void sensor_temp_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct sensor_temp_t *curr_temp;

    curr_temp = &g_sensor.temp[index - 1];
    if (curr_temp->obj) {
        sysfs_remove_group(&curr_temp->obj->kobj, &sensor_temp_attr_group);
        wb_plat_kobject_delete(&curr_temp->obj);
        SENSOR_DBG("delete temp%d.\n", index);
    }
    return;
}

static int sensor_temp_sub_create_kobj_and_attrs(struct kobject *parent, int temp_num)
{
    unsigned int temp_index, i;

    g_sensor.temp = kzalloc(sizeof(struct sensor_temp_t) * temp_num, GFP_KERNEL);
    if (!g_sensor.temp ) {
        SENSOR_ERR("kzalloc g_sensor.temp error, temp number = %d.\n", temp_num);
        return -ENOMEM;
    }
    for (temp_index = 1; temp_index <= temp_num; temp_index++) {
        if (sensor_temp_sub_single_create_kobj_and_attrs(parent, temp_index) != 0 ) {
            goto error;
        }
    }
    return 0;
error:
    for (i = temp_index - 1; i > 0; i--) {
        sensor_temp_sub_single_remove_kobj_and_attrs(i);
    }

    if (g_sensor.temp) {
        kfree(g_sensor.temp);
        g_sensor.temp = NULL;
    }
    return -EBADRQC;
}

static int sensor_in_sub_create_kobj_and_attrs(struct kobject *parent, int in_num)
{
    unsigned int in_index, i;

    g_sensor.in = kzalloc(sizeof(struct sensor_in_t) * in_num, GFP_KERNEL);
    if (!g_sensor.in) {
        SENSOR_ERR("kzalloc g_sensor.in error, in number = %d.\n", in_num);
        return -ENOMEM;
    }

    for (in_index = 1; in_index <= in_num; in_index++) {
        if (sensor_in_sub_single_create_kobj_and_attrs(parent, in_index) != 0 ) {
            goto error;
        }
    }
    return 0;
error:
    for (i = in_index - 1; i > 0; i--) {
        sensor_in_sub_single_remove_kobj_and_attrs(i);
    }

    if (g_sensor.in) {
        kfree(g_sensor.in);
        g_sensor.in = NULL;
    }
    return -EBADRQC;
}

static int sensor_temp_sub_create(void)
{
    int ret, temp_num;

    check_p(g_drv->get_dev_number);
    temp_num = g_drv->get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_TEMP);
    g_sensor.temp_number = temp_num;
    if (temp_num <= 0) {
        SENSOR_DBG("Warning:sensor temp number = %d \n", temp_num);
        return 0;
    }
    ret = sensor_temp_sub_create_kobj_and_attrs(&g_sensor_obj->kobj, temp_num);
    return ret;
}

static int sensor_in_sub_create(void)
{
    int ret, in_num;

    check_p(g_drv->get_dev_number);
    in_num = g_drv->get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_IN);
    g_sensor.in_number = in_num;

    if (in_num <= 0) {
        SENSOR_DBG("Warning:sensor in number = %d \n", in_num);
        return 0;
    }
    ret = sensor_in_sub_create_kobj_and_attrs(&g_sensor_obj->kobj, in_num);
    return ret;
}

static void temp_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct sensor_temp_t * curr_temp;

    curr_temp = &g_sensor.temp[index - 1];
    if (curr_temp->obj) {
        sysfs_remove_group(&curr_temp->obj->kobj, &sensor_temp_attr_group);
        wb_plat_kobject_delete(&curr_temp->obj);
        SENSOR_DBG("delete sensor temp%d.\n", index);
    }
    return;
}

static void in_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct sensor_in_t * curr_in;

    curr_in = &g_sensor.in[index - 1];
    if (curr_in->obj) {
        sysfs_remove_group(&curr_in->obj->kobj, &sensor_in_attr_group);
        wb_plat_kobject_delete(&curr_in->obj);
        SENSOR_DBG("delete sensor in%d.\n", index);
    }
    return;
}

static void sensor_temp_sub_remove(void)
{
    unsigned int temp_index;

    if (g_sensor.temp) {
       for (temp_index = g_sensor.temp_number; temp_index > 0; temp_index--) {
           temp_sub_single_remove_kobj_and_attrs(temp_index);
       }
       kfree(g_sensor.temp);
       g_sensor.temp = NULL;
    }
    return;
}

static void sensor_in_sub_remove(void)
{
    unsigned int in_index;

    if (g_sensor.in) {
       for (in_index = g_sensor.in_number; in_index > 0; in_index--) {
           in_sub_single_remove_kobj_and_attrs(in_index);
       }
       kfree(g_sensor.in);
       g_sensor.in = NULL;
    }
    return;
}

static void sensor_sub_remove(void)
{
    sensor_temp_sub_remove();
    sensor_in_sub_remove();
}

static int sensor_sub_create(void)
{
    int ret;
    /* temp creat */
    ret = sensor_temp_sub_create();
    if (ret < 0) {
        goto temp_err;
    }
    /* Voltage creat */
    ret = sensor_in_sub_create();
    if (ret < 0) {
        goto in_err;
    }
    return 0;
in_err:
    sensor_temp_sub_remove();
temp_err:
    return ret;
}
static void sensor_root_remove(void)
{
    if (g_sensor_obj) {
        sysfs_remove_group(&g_sensor_obj->kobj, &sensor_root_attr_group);
        wb_plat_kobject_delete(&g_sensor_obj);
        SENSOR_DBG("delete sensor root success\n");
    }

    return;
}

static int sensor_root_create(void)
{
    g_sensor_obj = wb_plat_kobject_create("sensor", NULL);
    if (!g_sensor_obj) {
        SENSOR_ERR("wb_plat_kobject_create sensor error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_sensor_obj->kobj, &sensor_root_attr_group) != 0) {
        wb_plat_kobject_delete(&g_sensor_obj);
        SENSOR_ERR("create sensor dir attrs error!\n");
        return -EBADRQC;
    }
    SENSOR_DBG("wb_plat_kobject_create sensor directory and attribute success.\n");
    return 0;
}

static int wb_sensor_init(void)
{
    int ret;

    SENSOR_INFO("wb_sensor_init...\n");
    g_drv = dfd_plat_driver_get();
    check_p(g_drv);

    ret = sensor_root_create();
    if (ret < 0) {
        goto sensor_root_error;
    }

    ret = sensor_sub_create();
    if (ret < 0) {
        goto sensor_sub_error;
    }
    SENSOR_INFO("sensor_init ok.\n");
    return 0;
sensor_sub_error:
    sensor_root_remove();
sensor_root_error:
    return ret;
}

static void wb_sensor_exit(void)
{
    sensor_sub_remove();
    sensor_root_remove();
    SENSOR_INFO("sensor_exit ok.\n");
    return;
}

module_init(wb_sensor_init);
module_exit(wb_sensor_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("sensors sysfs driver");
