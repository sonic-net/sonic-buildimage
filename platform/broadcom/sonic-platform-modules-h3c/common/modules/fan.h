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
#ifndef _FAN_H_
#define _FAN_H_

#define MODULE_NAME "fan"
#define DBG_ECHO(level, fmt, args...) DEBUG_PRINT(bsp_module_debug_level[BSP_FAN_MODULE], level, BSP_LOG_FILE, fmt, ##args)

#define FAN_EEPROM_STRING_MAX_LENGTH 128

#define FAN_VENDOR_DELTA "DELTA"
#define FAN_VENDOR_FOXOCNN "FOXCONN"

typedef struct
{
    int motor_index;
    // int speed;
    int fan_index;
    // int direction;
    // int motor_status;
    struct kobject kobj_motor;
} fan_motor_st;

typedef struct
{
    //int status;
    int fan_index;
    //int pwm;
    struct device *parent_hwmon;
    struct kobject kobj_fan;
    fan_motor_st fan_motor[MAX_FAN_MOTER_NUM];
} fan_info_st;

enum fan_hwmon_sysfs_attributes
{
    FAN1_MIN,
    FAN1_MAX,
    FAN1_INPUT,
    FAN1_PULSES,
    FAN1_SPEED,
    FAN1_LABEL,
    FAN1_ENABLE,
    FAN1_ALARM,
    FAN2_MIN,
    FAN2_MAX,
    FAN2_INPUT,
    FAN2_PULSES,
    FAN2_SPEED,
    FAN2_LABEL,
    FAN2_ENABLE,
    FAN2_ALARM,
    FAN_HWMON_ATTR_BUTT,
};

enum fan_sensor_sysfs_attributes
{
    NUM_FANS = FAN_HWMON_ATTR_BUTT + 1,
    VENDOR,
    PRODUCT_NAME,
    SN,
    PN,
    HW_VERSION,
    NUM_MOTORS,
    MOTOR_SPEED,
    MOTOR_TOLERANCE,
    MOTOR_TARGET,
    MOTOR_RATIO,
    MOTOR_DIRECTION,
    MOTOR_STATUS,
    STATUS,
    LED_STATUS,
    FAN_SPEED_PWM,
    RAW_DATA
};

extern int bsp_fan_get_status(int fan_index);
extern int bsp_fan_motor_get_status(int fan_index, int motor_index, int *status);
extern char *bsp_fan_get_status_string(int status);
extern int bsp_fan_get_status_color(int status);
extern int bsp_fan_get_fan_number(int *fan_number);
extern int bsp_fan_get_info_from_eeprom_by_h3c(int fan_index, int info_type, OUT u8 *info_data);
extern int bsp_fan_get_info_from_eeprom_by_tlv(int fan_index, int info_type, OUT u8 *info_data);
extern int bsp_fan_get_info_from_eeprom(int fan_index, int info_type, OUT u8 *info_data);
extern int bsp_fan_eeprom_write_protect_set(int index, int enable_disable_flag);
extern int bsp_sysfs_fan_get_index_from_dev(struct device *dev);

extern unsigned int bsp_fan_custom_get_num_fans(void);
extern ssize_t bsp_fan_custom_get_vendor(unsigned int index, char *buf);
extern ssize_t bsp_fan_custom_get_sn(unsigned int index, char *buf);
extern ssize_t bsp_fan_custom_get_pn(unsigned int index, char *buf);
extern ssize_t bsp_fan_custom_get_product_name(unsigned int index, char *buf);
extern ssize_t bsp_fan_custom_get_hw_version(unsigned int index, char *buf);
extern int bsp_fan_custom_get_num_motors(char *buf);
extern int bsp_fan_custom_get_status(int index, char *buf);
extern int bsp_fan_custom_get_led_status(int index, char *buf);
extern bool bsp_fan_custom_get_fan_pwm(unsigned int fan_index, u8 *buf);
extern int bsp_fan_custom_get_raw_data(int index, char *buf);
extern ssize_t bsp_fan_custom_get_speed(unsigned int fan_index, unsigned int *speed);
extern bool bsp_fan_custom_set_fan_pwm(unsigned int fan_index, u8 pwm);
extern int bsp_fan_custom_set_led_status(int fan_index, int led_status);
extern int bsp_fan_custom_set_raw_data(int fan_index, const char *buf, size_t count);

extern int bsp_fan_custom_motor_get_status(int fan_index, int motor_index, char *buf);
extern int bsp_fan_custom_motor_get_speed(int fan_index, int motor_index, char *buf);
extern int bsp_fan_custom_motor_get_ratio(int fan_index, int motor_index, char *buf);
extern int bsp_fan_custom_motor_get_tolerance(int fan_index, int motor_index, char *buf);
extern int bsp_fan_custom_motor_get_target(int fan_index, int motor_index, char *buf);
extern int bsp_fan_custom_motor_get_direction(int fan_index, int motor_index, char *buf);

#endif
