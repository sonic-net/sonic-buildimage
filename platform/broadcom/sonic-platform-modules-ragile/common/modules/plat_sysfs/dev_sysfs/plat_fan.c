/*
 * plat_fan.c
 * Original Author: support  2020-02-17
 *
 * This module create fan kobjects and attributes in /sys/wb_plat/fan
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0          support                  2020-02-17          Initial version
 */

#include <linux/slab.h>

#include "./include/plat_switch.h"
#include "./include/sysfs_common.h"

#define FAN_INFO(fmt, args...) LOG_INFO("fan: ", fmt, ##args)
#define FAN_ERR(fmt, args...)  LOG_ERR("fan: ", fmt, ##args)
#define FAN_DBG(fmt, args...)  LOG_DBG("fan: ", fmt, ##args)

struct motor_obj_t{
    struct switch_obj *obj;
};

struct fan_obj_t{
    unsigned int motor_number;
    struct motor_obj_t *motor;
    struct switch_obj *obj;
};

struct fan_t{
    unsigned int fan_number;
    struct fan_obj_t *fan;
};

static int g_loglevel = 0;
static struct fan_t g_fan;
static struct switch_obj *g_fan_obj = NULL;
static struct switch_drivers_t *g_drv = NULL;

static ssize_t fan_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_fan.fan_number);
}

static ssize_t fan_motor_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;

    index = obj->index;
    FAN_DBG("fan_motor_number_show,fan index:%d\n",index);

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_fan.fan[index-1].motor_number);
}

static ssize_t fan_roll_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_fan_roll_status);

    p_obj = to_switch_obj(obj->kobj.parent);
    check_p(p_obj);

    fan_index = p_obj->index;
    motor_index = obj->index;

    ret = g_drv->get_fan_roll_status(fan_index, motor_index);
    if (ret < 0 ) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t fan_present_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index;
    int ret;

    fan_index = obj->index;
    FAN_DBG("fan_present_status_show, fan index:%d\n",fan_index);
    check_p(g_drv);
    check_p(g_drv->get_fan_present_status);

    ret = g_drv->get_fan_present_status(fan_index);
    if(ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t fan_speed_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index, motor_index, speed;
    int ret;
    struct switch_obj *p_obj;

    check_p(g_drv);
    check_p(g_drv->get_fan_speed);

    p_obj = to_switch_obj(obj->kobj.parent);
    check_p(p_obj);

    fan_index = p_obj->index;
    motor_index = obj->index;

    ret = g_drv->get_fan_speed(fan_index, motor_index, &speed);
    if(ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", speed);
}

static ssize_t fan_motor_ratio_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;
    int ret, pwm;

    check_p(g_drv);
    check_p(g_drv->get_fan_pwm);

    p_obj = to_switch_obj(obj->kobj.parent);
    check_p(p_obj);
    fan_index = p_obj->index;
    motor_index = obj->index;
    ret = g_drv->get_fan_pwm(fan_index, motor_index, &pwm);

    if (ret < 0 ) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", pwm);
}

static ssize_t fan_motor_ratio_store(struct switch_obj *obj, struct switch_attribute *attr,
                                    const char* buf, size_t count)
{
    unsigned int fan_index, motor_index;
    struct switch_obj *p_obj;
    int ret, pwm;

    check_p(g_drv);
    check_p(g_drv->set_fan_pwm);

    p_obj = to_switch_obj(obj->kobj.parent);
    check_p(p_obj);

    fan_index = p_obj->index;
    motor_index = obj->index;
    sscanf(buf, "%d", &pwm);

    if (pwm < 0 || pwm > 100) {
        FAN_ERR("can not set pwm = %d.\n", pwm);
        return -EINVAL;
    }
    ret = g_drv->set_fan_pwm(fan_index, motor_index, pwm);
    if (ret < 0) {
        FAN_ERR("can not set pwm = %d.\n", pwm);
        return -EIO;
    }
    return count;
}

/************************************fan dir and attrs*******************************************/
static struct switch_attribute fan_number_att = __ATTR(num_fans, S_IRUGO, fan_number_show, NULL);

static struct attribute *fan_dir_attrs[] = {
    &fan_number_att.attr,
    NULL,
};

static struct attribute_group fan_root_attr_group = {
    .attrs = fan_dir_attrs,
};

/*******************************fan1 fan2 dir and attrs*******************************************/
static struct switch_attribute fan_num_motors_att = __ATTR(num_motors, S_IRUGO, fan_motor_number_show, NULL);
static struct switch_attribute fan_present_att = __ATTR(present, S_IRUGO, fan_present_status_show, NULL);

static struct attribute *fan_attrs[] = {
    &fan_num_motors_att.attr,
    &fan_present_att.attr,
    NULL,
};

static struct attribute_group fan_attr_group = {
    .attrs = fan_attrs,
};

/*******************************motor0 motor1 dir and attrs*******************************************/
static struct switch_attribute motor_speed_att = __ATTR(speed, S_IRUGO, fan_speed_show, NULL);
static struct switch_attribute motor_status_att = __ATTR(status, S_IRUGO, fan_roll_status_show, NULL);
static struct switch_attribute motor_ratio_att = __ATTR(ratio, S_IRUGO | S_IWUSR, fan_motor_ratio_show, fan_motor_ratio_store);

static struct attribute *motor_attrs[] = {
    &motor_speed_att.attr,
    &motor_status_att.attr,
    &motor_ratio_att.attr,
    NULL,
};

static struct attribute_group motor_attr_group = {
    .attrs = motor_attrs,
};

static void fanindex_single_motor_remove_kobj_and_attrs(struct fan_obj_t * curr_fan, unsigned int motor_index)
{
    struct motor_obj_t *curr_motor; /* point to motor0 motor1...*/

    curr_motor = &curr_fan->motor[motor_index];
    if (curr_motor->obj) {
        sysfs_remove_group(&curr_motor->obj->kobj, &motor_attr_group);
        wb_plat_kobject_delete(&curr_motor->obj);
        FAN_DBG("delete fan:%d motor%d.\n", curr_fan->obj->index, motor_index);
    }
    return;
}

static int fanindex_single_motor_create_kobj_and_attrs(struct fan_obj_t * curr_fan, unsigned int motor_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct motor_obj_t *curr_motor; /* point to motor0 motor1...*/

    FAN_DBG("create fan_index:%d, motor%d ...\n", curr_fan->obj->index, motor_index);

    curr_motor = &curr_fan->motor[motor_index];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "motor%d", motor_index);
    curr_motor->obj = wb_plat_kobject_create(name, &curr_fan->obj->kobj);
    if (!curr_motor->obj) {
        FAN_ERR("create fan_index:%d, motor%d object error!\n", curr_fan->obj->index, motor_index);
        return -EBADRQC;
    }
    curr_motor->obj->index = motor_index;
    if (sysfs_create_group(&curr_motor->obj->kobj, &motor_attr_group) != 0) {
        FAN_ERR("create fan_index:%d, motor%d attrs error.\n", curr_fan->obj->index, motor_index);
        wb_plat_kobject_delete(&curr_motor->obj);
        return -EBADRQC;
    }
    FAN_DBG("create fan_index:%d, motor%d ok.\n", curr_fan->obj->index, motor_index);
    return 0;
}

static int fanindex_motor_create_kobj_and_attrs(struct fan_obj_t * curr_fan, int motor_num)
{
    int motor_index, i;

    curr_fan->motor = kzalloc(sizeof(struct motor_obj_t) * motor_num, GFP_KERNEL);
    if (!curr_fan->motor) {
        FAN_ERR("kzalloc motor error, fan index = %d, motor number = %d.\n", curr_fan->obj->index, motor_num);
        return -ENOMEM;
    }
    curr_fan->motor_number = motor_num;
    for (motor_index = 0; motor_index < motor_num; motor_index++) {
        if (fanindex_single_motor_create_kobj_and_attrs(curr_fan, motor_index) != 0) {
            goto motor_error;
        }
    }
    return 0;
motor_error:
    for(i = motor_index - 1; i >= 0; i--) {
        fanindex_single_motor_remove_kobj_and_attrs(curr_fan, i);
    }
    if(curr_fan->motor) {
        kfree(curr_fan->motor);
        curr_fan->motor = NULL;
    }
    return -EBADRQC;
}

static void fanindex_motor_remove_kobj_and_attrs(struct fan_obj_t *curr_fan, int motor_num)
{
    int motor_index;

    for (motor_index = motor_num - 1; motor_index >= 0; motor_index--) {
        fanindex_single_motor_remove_kobj_and_attrs(curr_fan, motor_index);
    }
    return;
}

static int fan_motor_create(void)
{
    int fan_num, motor_num;
    unsigned int fan_index, i;
    struct fan_obj_t *curr_fan;     /* point to fan1 fan2...*/

    check_p(g_drv->get_dev_number);

    motor_num = g_drv->get_dev_number(WB_MAIN_DEV_FAN, WB_MINOR_DEV_MOTOR);
    if (motor_num <= 0) {
        FAN_ERR("get fan motor number error, motor_num:%d error.\n", motor_num);
        return -ENODEV;
    }

    fan_num = g_fan.fan_number;
    for (fan_index = 1; fan_index <= fan_num; fan_index++) {
        curr_fan = &g_fan.fan[fan_index - 1];
        if (fanindex_motor_create_kobj_and_attrs(curr_fan, motor_num) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = fan_index - 1; i > 0; i--) {
        curr_fan = &g_fan.fan[i - 1];
        motor_num = curr_fan->motor_number;
        fanindex_motor_remove_kobj_and_attrs(curr_fan, motor_num);
    }
    return -EBADRQC;
}

static void fan_motor_remove(void)
{
    unsigned int fan_index;
    struct fan_obj_t *curr_fan;

    if (g_fan.fan) {
       for (fan_index = g_fan.fan_number; fan_index > 0; fan_index--) {
           curr_fan = &g_fan.fan[fan_index - 1];
           if (curr_fan->motor) {
               fanindex_motor_remove_kobj_and_attrs(curr_fan, curr_fan->motor_number);
               kfree(curr_fan->motor);
               curr_fan->motor = NULL;
               curr_fan->motor_number = 0;
           }
       }
    }
    return;
}

static void fan_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct fan_obj_t *curr_fan;

    curr_fan = &g_fan.fan[index - 1];
    if (curr_fan->obj) {
        sysfs_remove_group(&curr_fan->obj->kobj, &fan_attr_group);
        wb_plat_kobject_delete(&curr_fan->obj);
        FAN_DBG("delete fan%d.\n", index);
    }
    return;
}

static int fan_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct fan_obj_t *curr_fan;

    curr_fan = &g_fan.fan[index - 1];
    FAN_DBG("create fan%d ...\n", index);
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "fan%d", index);
    curr_fan->obj = wb_plat_kobject_create(name, parent);
    if (!curr_fan->obj) {
        FAN_ERR("create fan%d object error!\n", index);
        return -EBADRQC;
    }
    curr_fan->obj->index = index;
    if (sysfs_create_group(&curr_fan->obj->kobj, &fan_attr_group) != 0) {
        FAN_ERR("create fan%d attrs error.\n", index);
        wb_plat_kobject_delete(&curr_fan->obj);
        return -EBADRQC;
    }
    FAN_DBG("create fan%d ok.\n", index);
    return 0;
}

static int fan_sub_create_kobj_and_attrs(struct kobject *parent, int fan_num)
{
    unsigned int fan_index, i;

    g_fan.fan = kzalloc(sizeof(struct fan_obj_t) * fan_num, GFP_KERNEL);
    if (!g_fan.fan) {
        FAN_ERR("kzalloc fan.fan error, fan number = %d.\n", fan_num);
        return -ENOMEM;
    }

    for (fan_index = 1; fan_index <= fan_num; fan_index++) {
        if(fan_sub_single_create_kobj_and_attrs(parent, fan_index) != 0 ) {
            goto error;
        }
    }
    return 0;
error:
    for (i = fan_index - 1; i > 0; i--) {
        fan_sub_single_remove_kobj_and_attrs(i);
    }
    if (g_fan.fan) {
        kfree(g_fan.fan);
        g_fan.fan = NULL;
    }
    return -EBADRQC;
}

static int fan_sub_create(void)
{
    int ret, fan_num;

    check_p(g_drv->get_dev_number);
    fan_num = g_drv->get_dev_number(WB_MAIN_DEV_FAN, WB_MINOR_DEV_NONE);
    if (fan_num < 0) {
        FAN_ERR("fan number = %d error.\n", fan_num);
        return -EINVAL;
    }
    g_fan.fan_number = fan_num;
    ret = fan_sub_create_kobj_and_attrs(&g_fan_obj->kobj, fan_num);
    return ret;
}

static void fan_sub_remove(void)
{
    unsigned int fan_index;

    if (g_fan.fan) {
       for (fan_index = g_fan.fan_number; fan_index > 0; fan_index--) {
           fan_sub_single_remove_kobj_and_attrs(fan_index);
       }
       kfree(g_fan.fan);
    }
    mem_clear(&g_fan, sizeof(struct fan_t));
    return;
}

static int fan_root_create(void)
{
    g_fan_obj = wb_plat_kobject_create("fan", NULL);
    if (!g_fan_obj) {
        FAN_ERR("wb_plat_kobject_create fan error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_fan_obj->kobj, &fan_root_attr_group) != 0) {
        wb_plat_kobject_delete(&g_fan_obj);
        FAN_ERR("create fan dir attrs error!\n");
        return -EBADRQC;
    }
    FAN_DBG("wb_plat_kobject_create fan directory and attribute success.\n");
    return 0;
}

static void fan_root_remove(void)
{
    if (g_fan_obj) {
        sysfs_remove_group(&g_fan_obj->kobj, &fan_root_attr_group);
        wb_plat_kobject_delete(&g_fan_obj);
        FAN_DBG("delete fan root success\n");
    }

    return;
}

static int fan_init(void)
{
    int ret;

    FAN_INFO("fan_init...\n");
    g_drv = dfd_plat_driver_get();
    check_p(g_drv);

    ret = fan_root_create();
    if (ret < 0) {
        goto fan_root_error;
    }

    ret = fan_sub_create();
    if (ret < 0) {
        goto fan_sub_error;
    }

    ret = fan_motor_create();
    if (ret < 0) {
        goto fan_motor_error;
    }

    FAN_INFO("fan_init ok.\n");
    return 0;
fan_motor_error:
    fan_sub_remove();
fan_sub_error:
    fan_root_remove();
fan_root_error:
    return ret;
}

static void fan_exit(void)
{
    fan_motor_remove();
    fan_sub_remove();
    fan_root_remove();
    FAN_INFO("fan_exit ok.\n");
    return ;
}

module_init(fan_init);
module_exit(fan_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("fan sysfs driver");
