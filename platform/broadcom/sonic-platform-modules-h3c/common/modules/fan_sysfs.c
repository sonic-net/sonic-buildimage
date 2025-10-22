/*
* Copyright (c) 2019  <sonic@h3c.com>
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
/*公有文件引入*/
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>
/*私有文件*/
#include "pub.h"
#include "bsp_base.h"
#include "static_ktype.h"
#include "fan.h"

module_param_named(fan_debug_level, bsp_module_debug_level[BSP_FAN_MODULE], int, 0644);
MODULE_PARM_DESC(fan_debug_level, "DEBUG 0x4, ERROR 0x2, INFO 0x1, ALL 0x7, DEBUG_OFF 0x0; Default value is ERROR");

struct kobject *kobj_fan_root = NULL;
struct kobject *kobj_fan_debug = NULL;    //风扇debug节点
char fan_name_str[MAX_FAN_NUM][MAX_HWMON_NAME_LEN];
fan_info_st fan_info[MAX_FAN_NUM];

static ssize_t bsp_sysfs_fan_custom_set_attr(struct device *kobj, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    fan_info_st *fan_ptr = NULL;
    int fan_index = 0;
    int temp = 0;
    int ret = ERROR_SUCCESS;

    if ((struct kobject *)kobj != kobj_fan_root && (struct kobject *)kobj != kobj_fan_debug)
    {
        fan_ptr = container_of((struct kobject *)kobj, fan_info_st, kobj_fan);
        fan_index = fan_ptr->fan_index;
    }

    if (sscanf(buf, "%d", &temp) <= 0 && (RAW_DATA != (attr->index)))
    {
        DBG_ECHO(DEBUG_INFO, "Format '%s' error, integer expected!", buf);
        return -EINVAL;
    }

    switch (attr->index)
    {
        case FAN_SPEED_PWM:
            if (!bsp_fan_custom_set_fan_pwm(0, temp))
            {
                ret = -EIO;
            }
            break;
        case LED_STATUS: //设置风扇led
            ret = bsp_fan_custom_set_led_status(fan_index, temp);
            break;
        case RAW_DATA:
            ret = bsp_fan_custom_set_raw_data(fan_index, buf, count);
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "Not found attribte %d for fan", attr->index + 1);
            ret = -ENOSYS;
            break;
    }

    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "fan set attribte %d failed", attr->index + 1);
        count = ret;
    }
    return count;
}

static ssize_t bsp_sysfs_fan_custom_motor_get_attr(struct device *kobj, struct device_attribute *da, char *buf)
{
    ssize_t index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    fan_motor_st *motor = container_of((struct kobject *)kobj, fan_motor_st, kobj_motor);
    int motor_index = motor->motor_index;
    int fan_index = motor->fan_index;

    switch (attr->index)
    {
        case MOTOR_STATUS:
            index = bsp_fan_custom_motor_get_status(fan_index, motor_index, buf);
            break;
        case MOTOR_SPEED:
            index = bsp_fan_custom_motor_get_speed(fan_index, motor_index, buf);
            break;
        case MOTOR_RATIO:
            index = bsp_fan_custom_motor_get_ratio(fan_index, motor_index, buf);
            break;
        case MOTOR_TOLERANCE:
            index = bsp_fan_custom_motor_get_tolerance(fan_index, motor_index, buf);
            break;
        case MOTOR_TARGET:
            index = bsp_fan_custom_motor_get_target(fan_index, motor_index, buf);
            break;
        case MOTOR_DIRECTION:
            index = bsp_fan_custom_motor_get_direction(fan_index, motor_index, buf);
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "Not found attribte %d for fan motor", attr->index);
            index = -ENOSYS;
            break;
    }
    return index;
}

static ssize_t bsp_sysfs_fan_custom_get_attr(struct device *kobj, struct device_attribute *da, char *buf)
{
    ssize_t index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    fan_info_st *fan_ptr = NULL;
    int fan_index = 0;
    u8 fan_pwm = 0;
    unsigned int num;

    if ((struct kobject *)kobj != kobj_fan_root && (struct kobject *)kobj != kobj_fan_debug)
    {
        fan_ptr = container_of((struct kobject *)kobj, fan_info_st, kobj_fan);
        fan_index = fan_ptr->fan_index;
    }

    switch (attr->index)
    {
        case NUM_FANS:
            num = bsp_fan_custom_get_num_fans();
            index = scnprintf(buf,  PAGE_SIZE, "%d\n", num);
            break;
        case VENDOR:
            index = bsp_fan_custom_get_vendor(fan_index, buf);
            break;
        case SN:
            index = bsp_fan_custom_get_sn(fan_index, buf);
            break;
        case PN:
            index = bsp_fan_custom_get_pn(fan_index, buf);
            break;
        case PRODUCT_NAME:
            index = bsp_fan_custom_get_product_name(fan_index, buf);
            break;
        case HW_VERSION:
            index = bsp_fan_custom_get_hw_version(fan_index, buf);
            break;
        case NUM_MOTORS:
            index = bsp_fan_custom_get_num_motors(buf);
            break;
        case STATUS:
            index = bsp_fan_custom_get_status(fan_index, buf);
            break;
        case LED_STATUS:
            index = bsp_fan_custom_get_led_status(fan_index, buf);
            break;
        case FAN_SPEED_PWM:
            if (bsp_fan_custom_get_fan_pwm(fan_index, &fan_pwm))
            {
                index = scnprintf(buf, PAGE_SIZE, "%d\n", fan_pwm);
            }
            else
            {
                index = -EIO;
            }
            break;
        case RAW_DATA:
            index = bsp_fan_custom_get_raw_data(fan_index, buf);
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "Not found attribte %d for fan", attr->index);
            index = -ENOSYS;
            break;
    }

    return index;
}

static ssize_t bsp_sysfs_fan_get_attr(struct device *dev, struct device_attribute *da, char *buf)
{
    int motor_index = 0;
    int index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int fan_index = bsp_sysfs_fan_get_index_from_dev(dev);
    u8 pwm = 0;
    u16 fan_motor_speed = 0;

    if (fan_index == -1)
    {
        return -EINVAL;
    }

    switch (attr->index)
    {
        case FAN1_MIN:
        case FAN2_MIN:
            index = scnprintf(buf, PAGE_SIZE, "%d\n", bsp_get_board_data()->fan_min_speed);
            break;
        case FAN1_MAX:
        case FAN2_MAX:
            index = scnprintf(buf, PAGE_SIZE, "%d\n", bsp_get_board_data()->fan_max_speed);
            break;
        case FAN1_INPUT:
        case FAN2_INPUT:
            motor_index = attr->index == FAN1_INPUT ? 0 : 1;
            if (bsp_cpld_get_fan_speed(&fan_motor_speed, fan_index, motor_index) != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "cpld get speed failed!");
            }
            index = scnprintf(buf, PAGE_SIZE, "%d\n", fan_motor_speed);
            break;
        case FAN1_PULSES:
        case FAN2_PULSES:
            if (bsp_cpld_get_fan_pwm_reg(&pwm) != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "cpld get pwm failed!");
            }
            index = scnprintf(buf, PAGE_SIZE, "%d\n", pwm);
            break;
        case FAN1_LABEL:
        case FAN2_LABEL:
            motor_index = attr->index == FAN1_LABEL ? 0 : 1;
            index = scnprintf(buf, PAGE_SIZE, "motor%d(%s)\n", motor_index + 1, motor_index == 1 ? "outer" : "inner");
            break;
        case FAN1_ENABLE:
        case FAN2_ENABLE:
            index = sprintf(buf, "1\n");
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "Not support!");
            index = -ENOSYS;
            break;
    }
    return index;
}

static SENSOR_DEVICE_ATTR(fan1_min,   S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN1_MIN);
static SENSOR_DEVICE_ATTR(fan1_max,   S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN1_MAX);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN1_INPUT);
static SENSOR_DEVICE_ATTR(pwm1,       S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN1_PULSES);
static SENSOR_DEVICE_ATTR(fan1_label, S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN1_LABEL);
static SENSOR_DEVICE_ATTR(fan1_enable, S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN1_ENABLE);
static SENSOR_DEVICE_ATTR(fan2_min,   S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN2_MIN);
static SENSOR_DEVICE_ATTR(fan2_max,   S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN2_MAX);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN2_INPUT);
static SENSOR_DEVICE_ATTR(pwm2,       S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN2_PULSES);
static SENSOR_DEVICE_ATTR(fan2_label, S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN2_LABEL);
static SENSOR_DEVICE_ATTR(fan2_enable, S_IRUGO, bsp_sysfs_fan_get_attr, NULL, FAN2_ENABLE);

static SENSOR_DEVICE_ATTR(num,     S_IRUGO, bsp_sysfs_fan_custom_get_attr, NULL, NUM_FANS);
static SENSOR_DEVICE_ATTR(vendor, S_IRUGO, bsp_sysfs_fan_custom_get_attr, NULL, VENDOR);
static SENSOR_DEVICE_ATTR(model_name, S_IRUGO, bsp_sysfs_fan_custom_get_attr, NULL, PRODUCT_NAME);
static SENSOR_DEVICE_ATTR(serial_number, S_IRUGO, bsp_sysfs_fan_custom_get_attr, NULL, SN);
static SENSOR_DEVICE_ATTR(part_number, S_IRUGO, bsp_sysfs_fan_custom_get_attr, NULL, PN);
static SENSOR_DEVICE_ATTR(hardware_version, S_IRUGO, bsp_sysfs_fan_custom_get_attr, NULL, HW_VERSION);
static SENSOR_DEVICE_ATTR(num_motors, S_IRUGO, bsp_sysfs_fan_custom_get_attr, NULL, NUM_MOTORS);
static SENSOR_DEVICE_ATTR(status,     S_IRUGO, bsp_sysfs_fan_custom_get_attr, NULL, STATUS);
static SENSOR_DEVICE_ATTR(led_status, S_IRUGO | S_IWUSR, bsp_sysfs_fan_custom_get_attr, bsp_sysfs_fan_custom_set_attr, LED_STATUS);
/*
static SENSOR_DEVICE_ATTR(motor_speed,     S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_SPEED);
static SENSOR_DEVICE_ATTR(motor_target,    S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_TARGET);
static SENSOR_DEVICE_ATTR(motor_tolerance, S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_TOLERANCE);
static SENSOR_DEVICE_ATTR(motor_ratio,     S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_RATIO);
static SENSOR_DEVICE_ATTR(motor_direction, S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_DIRECTION);
*/
static SENSOR_DEVICE_ATTR(motor_status,    S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_STATUS);
static SENSOR_DEVICE_ATTR(speed,     S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_SPEED);
static SENSOR_DEVICE_ATTR(speed_target,    S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_TARGET);
static SENSOR_DEVICE_ATTR(speed_tolerance, S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_TOLERANCE);
static SENSOR_DEVICE_ATTR(ratio,     S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_RATIO);
static SENSOR_DEVICE_ATTR(direction, S_IRUGO, bsp_sysfs_fan_custom_motor_get_attr, NULL, MOTOR_DIRECTION);
static SENSOR_DEVICE_ATTR(fan_speed_pwm,          S_IRUGO | S_IWUSR, bsp_sysfs_fan_custom_get_attr, bsp_sysfs_fan_custom_set_attr, FAN_SPEED_PWM);

#if (FT_TEST == 1)
static SENSOR_DEVICE_ATTR(raw_data,       S_IRUGO | S_IWUSR, bsp_sysfs_fan_custom_get_attr, bsp_sysfs_fan_custom_set_attr, RAW_DATA);
#else
static SENSOR_DEVICE_ATTR(raw_data,       S_IRUGO, bsp_sysfs_fan_custom_get_attr, NULL, RAW_DATA);
#endif

BSPMODULE_DEBUG_ATTR_DEF(debug, BSP_FAN_MODULE);
BSPMODULE_DEBUG_RW_ATTR_DEF(loglevel, BSP_FAN_MODULE);

//attribute array
static struct attribute *fan_attrs[] =
{
    &sensor_dev_attr_fan1_min.dev_attr.attr,
    &sensor_dev_attr_fan1_max.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    &sensor_dev_attr_pwm1.dev_attr.attr,
    &sensor_dev_attr_fan1_label.dev_attr.attr,
    &sensor_dev_attr_fan1_enable.dev_attr.attr,
    &sensor_dev_attr_fan2_min.dev_attr.attr,
    &sensor_dev_attr_fan2_max.dev_attr.attr,
    &sensor_dev_attr_fan2_input.dev_attr.attr,
    &sensor_dev_attr_pwm2.dev_attr.attr,
    &sensor_dev_attr_fan2_label.dev_attr.attr,
    &sensor_dev_attr_fan2_enable.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(fan);

static struct attribute *fan_custom_attributes[] =
{
    &sensor_dev_attr_vendor.dev_attr.attr,
    &sensor_dev_attr_model_name.dev_attr.attr,
    &sensor_dev_attr_serial_number.dev_attr.attr,
    &sensor_dev_attr_part_number.dev_attr.attr,
    &sensor_dev_attr_hardware_version.dev_attr.attr,
    &sensor_dev_attr_num_motors.dev_attr.attr,
    &sensor_dev_attr_status.dev_attr.attr,
    &sensor_dev_attr_led_status.dev_attr.attr,
    &sensor_dev_attr_raw_data.dev_attr.attr,
    NULL
};

static struct attribute *fan_custom_moter_attributes[] =
{
    /*    &sensor_dev_attr_motor_speed.dev_attr.attr,
        &sensor_dev_attr_motor_target.dev_attr.attr,
        &sensor_dev_attr_motor_tolerance.dev_attr.attr,
        &sensor_dev_attr_motor_ratio.dev_attr.attr,
        &sensor_dev_attr_motor_direction.dev_attr.attr,
        */
    &sensor_dev_attr_motor_status.dev_attr.attr,
    &sensor_dev_attr_speed.dev_attr.attr,
    &sensor_dev_attr_speed_target.dev_attr.attr,
    &sensor_dev_attr_speed_tolerance.dev_attr.attr,
    &sensor_dev_attr_ratio.dev_attr.attr,
    &sensor_dev_attr_direction.dev_attr.attr,
    NULL
};

static struct attribute *fan_debug_attributes[] =
{
    &sensor_dev_attr_fan_speed_pwm.dev_attr.attr,
    NULL
};

static const struct attribute_group fan_custom_attribute_group =
{
    .attrs = fan_custom_attributes,
};

static const struct attribute_group fan_custom_motor_attribute_group =
{
    .attrs = fan_custom_moter_attributes,
};

static const struct attribute_group fan_debug_attribute_group =
{
    .attrs = fan_debug_attributes,
};

static struct attribute *fan_customer_device_attributes_num_fan[] =
{
    &sensor_dev_attr_num.dev_attr.attr,
    &bspmodule_debug.attr,
    &bspmodule_loglevel.attr,
    NULL
};

static const struct attribute_group fan_customer_group_num_fan =
{
    .attrs = fan_customer_device_attributes_num_fan,
};

#if 0/*by jzq*/
int h3c_adjust_fan_speed(void)
{
    //获取温度bsp_sensor_get_max6696_temp(0, 0~2, return)
    //设置转速bsp_cpld_set_fan_pwm_reg()
    int i = 0;
    int j = 0;
    s8 curr_max_temperature = 0;
    s8 temp_temperature = 0;

    u16 curve_temp_min = 0;
    u16 curve_temp_max = 0;
    u8  curve_pwm_min = 0;
    u8  curve_pwm_max = 0;
    u8  target_pwm = 0;
    u8  target_pwm_with_fan_absent = 0;
    u8  current_running_fan_num = 0;
    u16 k = 0;
    size_t fan_num = 0;
    size_t max6696_num = 0;

    board_static_data *bd = bsp_get_board_data();
    fan_num = bd->fan_num;
    max6696_num = bd->max6696_num;

    if (fan_mon_not_adjust_speed != 0)
    {
        DBG_ECHO(DEBUG_DBG, "fan speed adjustment is stopped. fan_mon_not_adjust_speed=%d", fan_mon_not_adjust_speed);
        return ERROR_SUCCESS;
    }

    for (i = 0; i < fan_num; i++)
    {
        if (bsp_fan_get_status(i) == FAN_STATUS_OK)
            current_running_fan_num ++;
    }

    if (current_running_fan_num == 0)
    {
        SYSLOG(LOG_LEVEL_CRIT, "WARNING:No fan is normal!");
        return ERROR_FAILED;
    }

    for (j = 0; j < max6696_num; j++)
    {
        for (i = 0; i < MAX6696_SPOT_NUM; i++)
        {
            if (bsp_sensor_get_max6696_temp(j, i, &temp_temperature) == ERROR_SUCCESS)
            {
                curr_max_temperature = curr_max_temperature < temp_temperature ? temp_temperature : curr_max_temperature;
            }
            else
            {
                DBG_ECHO(DEBUG_INFO, "fan task get max6696(%d) temperature %d failed ", j, i);
            }
        }
    }

    DBG_ECHO(DEBUG_DBG, "current temp = %d", curr_max_temperature);

    curve_pwm_max = bd->fan_max_speed_pwm;
    curve_pwm_min = bd->fan_min_speed_pwm;
    curve_temp_min = bd->fan_temp_low;
    curve_temp_max = bd->fan_temp_high;

    if (curr_max_temperature <= curve_temp_min)
    {
        target_pwm = curve_pwm_min;
    }
    else if (curr_max_temperature >= curve_temp_max)
    {
        target_pwm = curve_pwm_max;
    }
    else
    {
        //避免小数，整体值放大1000倍，再缩小1000倍
        k = (u16)(curve_pwm_max - curve_pwm_min) * 1000 / (curve_temp_max - curve_temp_min);
        target_pwm = curve_pwm_max - k * (curve_temp_max - curr_max_temperature) / 1000;
    }

    target_pwm_with_fan_absent = (fan_num * target_pwm  / current_running_fan_num);   //按风扇数量线性计算总转速

    if (target_pwm_with_fan_absent > curve_pwm_max)
    {
        target_pwm_with_fan_absent = curve_pwm_max;
    }

    if (bsp_cpld_set_fan_pwm_reg(target_pwm_with_fan_absent) != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "set fan pwm 0x%x failed!", target_pwm_with_fan_absent);
    }

    DBG_ECHO(DEBUG_DBG, "k = %d, target_pwm = 0x%x", k, target_pwm_with_fan_absent);

    return ERROR_SUCCESS;
}
#endif

int fan_sysfs_init(void)
{
    int ret = ERROR_SUCCESS;
    int i = 0, j = 0;
    char temp_str[128] = {0};
    int bsp_fan_num;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "fan_sysfs_init get bd failed");
        return -EINVAL;
    }

    INIT_PRINT("module init started");

    bsp_fan_get_fan_number(&bsp_fan_num);

    memset(fan_info, 0, sizeof(fan_info));

    //create node for switch
    kobj_fan_root = kobject_create_and_add("fan", kobj_switch);

    if (kobj_fan_root == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "create kobj_fan_root failed!\n");
        ret = -ENOMEM;
        goto exit;
    }

    //CHECK_CREATE_SYSFS_FILE(kobj_fan_root, sensor_dev_attr_num_fans.dev_attr, ret);
    ret = sysfs_create_group(kobj_fan_root, &fan_customer_group_num_fan);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create num group failed");

    //build sysfs directory
    for (i = 0; i < bsp_fan_num; i++)
    {
        sprintf(temp_str, "fan%d", (i + 1));
        fan_info[i].fan_index = i;
        ret = kobject_init_and_add(&fan_info[i].kobj_fan, &static_kobj_ktype, kobj_fan_root, temp_str);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "create kobj_fan %d failed!", (i + 1));
        ret = sysfs_create_group(&(fan_info[i].kobj_fan), &fan_custom_attribute_group);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "create fan %d custom attribute group failed!", i + 1);

        //add motor group
        for (j = 0; j < bd->motors_per_fan; j++)
        {
            sprintf(temp_str, "motor%d", j);
            fan_info[i].fan_motor[j].motor_index = j;
            fan_info[i].fan_motor[j].fan_index = i;

            ret = kobject_init_and_add(&(fan_info[i].fan_motor[j].kobj_motor), &static_kobj_ktype, &(fan_info[i].kobj_fan), temp_str);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "kobject int and add for motor%d failed!", j);

            ret = sysfs_create_group(&(fan_info[i].fan_motor[j].kobj_motor), &fan_custom_motor_attribute_group);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "create motor%d attribute group failed!", j);
        }

        scnprintf(fan_name_str[i], sizeof(fan_name_str[i]), "Fan%d", (i + 1));
        // for sensors
        fan_info[i].parent_hwmon = hwmon_device_register_with_groups(NULL, fan_name_str[i], NULL, fan_groups);
        if (IS_ERR(fan_info[i].parent_hwmon))
        {
            fan_info[i].parent_hwmon = NULL;
            CHECK_IF_NULL_GOTO_EXIT(-ENODEV, ret, fan_info[i].parent_hwmon, "fan %d hwmon register failed", i + 1);
        }
    }

    //create fan debug node
    kobj_fan_debug = kobject_create_and_add("fan", kobj_debug);
    CHECK_IF_NULL_GOTO_EXIT(-ENOMEM, ret, kobj_fan_debug, "create fan debug kobject failed!"); \

    ret = sysfs_create_group(kobj_fan_debug, &fan_debug_attribute_group);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create fan debug attribute group failed!");

exit:
    return ret;
}

void fan_sysfs_release_kobj(void)
{
    int bsp_fan_num = 0;
    int i = 0, j = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "fan_sysfs_release_kobj get bd failed");
        return ;
    }
    bsp_fan_get_fan_number(&bsp_fan_num);

    for (i = 0; i < bsp_fan_num; i++)
    {
        for (j = 0; j < bd->motors_per_fan; j++)
        {
            if (fan_info[i].fan_motor[j].kobj_motor.state_initialized)
            {
                sysfs_remove_group(&(fan_info[i].fan_motor[j].kobj_motor), &fan_custom_motor_attribute_group);
                kobject_put(&(fan_info[i].fan_motor[j].kobj_motor));
            }
        }
        if (fan_info[i].kobj_fan.state_initialized)
        {
            sysfs_remove_group(&(fan_info[i].kobj_fan), &fan_custom_attribute_group);
            kobject_put(&(fan_info[i].kobj_fan));
        }

        if (fan_info[i].parent_hwmon != NULL)
        {
            hwmon_device_unregister(fan_info[i].parent_hwmon);
        }
    }

    if (kobj_fan_debug != NULL)
    {
        sysfs_remove_group(kobj_fan_debug, &fan_debug_attribute_group);
        kobject_put(kobj_fan_debug);
    }

    if (kobj_fan_root != NULL)
    {
        sysfs_remove_group(kobj_fan_root, &fan_customer_group_num_fan);
        kobject_put(kobj_fan_root);
    }
    return;
}

//设置初始化入口函数
static int __init fan_init(void)
{
    int ret = ERROR_SUCCESS;
    ret = fan_sysfs_init();
    if (ret != 0)
    {
        DBG_ECHO(DEBUG_ERR, "fan module init failed!\n");
        fan_sysfs_release_kobj();
    }
    else
    {
        INIT_PRINT("module init finished and success!");
    }

    eeprom_table_init(I2C_DEV_FAN);

    return ret;
}

//设置出口函数
static void __exit fan_exit(void)
{
    fan_sysfs_release_kobj();
    INIT_PRINT("module uninstalled !\n");
    return;
}

module_init(fan_init);
module_exit(fan_exit);
MODULE_AUTHOR("Wang Xue <wang.xue@h3c.com>");
MODULE_DESCRIPTION("h3c system eeprom driver");
MODULE_LICENSE("Dual BSD/GPL");
