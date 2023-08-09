/*
 * Copyright 2019 Broadcom.
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
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
 *
 * A pddf kernel driver module for a FAN controller 
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/kobject.h>
#include "pddf_client_defs.h"
#include "pddf_fan_defs.h"
#include "pddf_fan_driver.h"
#include "pddf_fan_api.h"

#define DRVNAME "pddf_fan"

struct pddf_ops_t pddf_fan_ops = {
	.pre_init = NULL,
	.post_init = NULL,

	.pre_probe = NULL,
	.post_probe = pddf_fan_post_probe_default,

	.pre_remove = NULL,
	.post_remove = NULL,

	.pre_exit = NULL,
	.post_exit = NULL,
};
EXPORT_SYMBOL(pddf_fan_ops);



FAN_SYSFS_ATTR_DATA data_fan1_present = {FAN1_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan1_present);
FAN_SYSFS_ATTR_DATA data_fan2_present = {FAN2_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan2_present);
FAN_SYSFS_ATTR_DATA data_fan3_present = {FAN3_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan3_present);
FAN_SYSFS_ATTR_DATA data_fan4_present = {FAN4_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan4_present);
FAN_SYSFS_ATTR_DATA data_fan5_present = {FAN5_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan5_present);
FAN_SYSFS_ATTR_DATA data_fan6_present = {FAN6_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan6_present);
FAN_SYSFS_ATTR_DATA data_fan7_present = {FAN7_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan7_present);
FAN_SYSFS_ATTR_DATA data_fan8_present = {FAN8_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan8_present);
FAN_SYSFS_ATTR_DATA data_fan9_present = {FAN9_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan9_present);
FAN_SYSFS_ATTR_DATA data_fan10_present = {FAN10_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan10_present);
FAN_SYSFS_ATTR_DATA data_fan11_present = {FAN11_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan11_present);
FAN_SYSFS_ATTR_DATA data_fan12_present = {FAN12_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan12_present);
FAN_SYSFS_ATTR_DATA data_fan13_present = {FAN13_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan13_present);
FAN_SYSFS_ATTR_DATA data_fan14_present = {FAN14_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan14_present);
FAN_SYSFS_ATTR_DATA data_fan15_present = {FAN15_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan15_present);
FAN_SYSFS_ATTR_DATA data_fan16_present = {FAN16_PRESENT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_present_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan16_present);


FAN_SYSFS_ATTR_DATA data_fan1_direction = {FAN1_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan1_direction);
FAN_SYSFS_ATTR_DATA data_fan2_direction = {FAN2_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan2_direction);
FAN_SYSFS_ATTR_DATA data_fan3_direction = {FAN3_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan3_direction);
FAN_SYSFS_ATTR_DATA data_fan4_direction = {FAN4_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan4_direction);
FAN_SYSFS_ATTR_DATA data_fan5_direction = {FAN5_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan5_direction);
FAN_SYSFS_ATTR_DATA data_fan6_direction = {FAN6_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan6_direction);
FAN_SYSFS_ATTR_DATA data_fan7_direction = {FAN7_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan7_direction);
FAN_SYSFS_ATTR_DATA data_fan8_direction = {FAN8_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan8_direction);
FAN_SYSFS_ATTR_DATA data_fan9_direction = {FAN9_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan9_direction);
FAN_SYSFS_ATTR_DATA data_fan10_direction = {FAN10_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan10_direction);
FAN_SYSFS_ATTR_DATA data_fan11_direction = {FAN11_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan11_direction);
FAN_SYSFS_ATTR_DATA data_fan12_direction = {FAN12_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan12_direction);
FAN_SYSFS_ATTR_DATA data_fan13_direction = {FAN13_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan13_direction);
FAN_SYSFS_ATTR_DATA data_fan14_direction = {FAN14_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan14_direction);
FAN_SYSFS_ATTR_DATA data_fan15_direction = {FAN15_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan15_direction);
FAN_SYSFS_ATTR_DATA data_fan16_direction = {FAN16_DIRECTION, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_direction_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan16_direction);


FAN_SYSFS_ATTR_DATA data_fan1_input = {FAN1_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan1_input);                                                                                                              
FAN_SYSFS_ATTR_DATA data_fan2_input = {FAN2_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan2_input);                                                                                                              
FAN_SYSFS_ATTR_DATA data_fan3_input = {FAN3_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan3_input);                                                                                                              
FAN_SYSFS_ATTR_DATA data_fan4_input = {FAN4_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan4_input);                                                                                                              
FAN_SYSFS_ATTR_DATA data_fan5_input = {FAN5_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan5_input);                                                                                                              
FAN_SYSFS_ATTR_DATA data_fan6_input = {FAN6_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan6_input);
FAN_SYSFS_ATTR_DATA data_fan7_input = {FAN7_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan7_input);             
FAN_SYSFS_ATTR_DATA data_fan8_input = {FAN8_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan8_input);             
FAN_SYSFS_ATTR_DATA data_fan9_input = {FAN9_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan9_input);             
FAN_SYSFS_ATTR_DATA data_fan10_input = {FAN10_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan10_input);              
FAN_SYSFS_ATTR_DATA data_fan11_input = {FAN11_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan11_input);             
FAN_SYSFS_ATTR_DATA data_fan12_input = {FAN12_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan12_input);
FAN_SYSFS_ATTR_DATA data_fan13_input = {FAN13_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan13_input);
FAN_SYSFS_ATTR_DATA data_fan14_input = {FAN14_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan14_input);
FAN_SYSFS_ATTR_DATA data_fan15_input = {FAN15_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan15_input);
FAN_SYSFS_ATTR_DATA data_fan16_input = {FAN16_INPUT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_rpm_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan16_input);


FAN_SYSFS_ATTR_DATA data_fan1_pwm = {FAN1_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan1_pwm);             
FAN_SYSFS_ATTR_DATA data_fan2_pwm = {FAN2_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan2_pwm);             
FAN_SYSFS_ATTR_DATA data_fan3_pwm = {FAN3_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan3_pwm);             
FAN_SYSFS_ATTR_DATA data_fan4_pwm = {FAN4_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan4_pwm);              
FAN_SYSFS_ATTR_DATA data_fan5_pwm = {FAN5_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan5_pwm);             
FAN_SYSFS_ATTR_DATA data_fan6_pwm = {FAN6_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan6_pwm);
FAN_SYSFS_ATTR_DATA data_fan7_pwm = {FAN7_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan7_pwm);             
FAN_SYSFS_ATTR_DATA data_fan8_pwm = {FAN8_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan8_pwm);             
FAN_SYSFS_ATTR_DATA data_fan9_pwm = {FAN9_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan9_pwm);             
FAN_SYSFS_ATTR_DATA data_fan10_pwm = {FAN10_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan10_pwm);              
FAN_SYSFS_ATTR_DATA data_fan11_pwm = {FAN11_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan11_pwm);             
FAN_SYSFS_ATTR_DATA data_fan12_pwm = {FAN12_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan12_pwm);
FAN_SYSFS_ATTR_DATA data_fan13_pwm = {FAN13_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan13_pwm);
FAN_SYSFS_ATTR_DATA data_fan14_pwm = {FAN14_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan14_pwm);
FAN_SYSFS_ATTR_DATA data_fan15_pwm = {FAN15_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan15_pwm);
FAN_SYSFS_ATTR_DATA data_fan16_pwm = {FAN16_PWM, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_pwm_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_pwm_default, NULL, NULL};
EXPORT_SYMBOL(data_fan16_pwm);


FAN_SYSFS_ATTR_DATA data_fan1_fault = {FAN1_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan1_fault);
FAN_SYSFS_ATTR_DATA data_fan2_fault = {FAN2_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan2_fault);
FAN_SYSFS_ATTR_DATA data_fan3_fault = {FAN3_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan3_fault);
FAN_SYSFS_ATTR_DATA data_fan4_fault = {FAN4_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan4_fault);
FAN_SYSFS_ATTR_DATA data_fan5_fault = {FAN5_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan5_fault);
FAN_SYSFS_ATTR_DATA data_fan6_fault = {FAN6_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan6_fault);
FAN_SYSFS_ATTR_DATA data_fan7_fault = {FAN7_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan7_fault);
FAN_SYSFS_ATTR_DATA data_fan8_fault = {FAN8_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan8_fault);
FAN_SYSFS_ATTR_DATA data_fan9_fault = {FAN9_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan9_fault);
FAN_SYSFS_ATTR_DATA data_fan10_fault = {FAN10_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan10_fault);
FAN_SYSFS_ATTR_DATA data_fan11_fault = {FAN11_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan11_fault);
FAN_SYSFS_ATTR_DATA data_fan12_fault = {FAN12_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan12_fault);
FAN_SYSFS_ATTR_DATA data_fan13_fault = {FAN13_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan13_fault);
FAN_SYSFS_ATTR_DATA data_fan14_fault = {FAN14_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan14_fault);
FAN_SYSFS_ATTR_DATA data_fan15_fault = {FAN15_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan15_fault);
FAN_SYSFS_ATTR_DATA data_fan16_fault = {FAN16_FAULT, S_IRUGO, fan_show_default, NULL, sonic_i2c_get_fan_fault_default, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan16_fault);

/* Derived attributes like status (should be derived from 'presence' and 'speed'/'fault' attributes) etc */
FAN_SYSFS_ATTR_DATA data_fan1_status = {FAN1_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan1_status);
FAN_SYSFS_ATTR_DATA data_fan2_status = {FAN2_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan2_status);
FAN_SYSFS_ATTR_DATA data_fan3_status = {FAN3_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan3_status);
FAN_SYSFS_ATTR_DATA data_fan4_status = {FAN4_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan4_status);
FAN_SYSFS_ATTR_DATA data_fan5_status = {FAN5_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan5_status);
FAN_SYSFS_ATTR_DATA data_fan6_status = {FAN6_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan6_status);
FAN_SYSFS_ATTR_DATA data_fan7_status = {FAN7_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan7_status);
FAN_SYSFS_ATTR_DATA data_fan8_status = {FAN8_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan8_status);
FAN_SYSFS_ATTR_DATA data_fan9_status = {FAN9_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan9_status);
FAN_SYSFS_ATTR_DATA data_fan10_status = {FAN10_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan10_status);
FAN_SYSFS_ATTR_DATA data_fan11_status = {FAN11_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan11_status);
FAN_SYSFS_ATTR_DATA data_fan12_status = {FAN12_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan12_status);
FAN_SYSFS_ATTR_DATA data_fan13_status = {FAN13_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan13_status);
FAN_SYSFS_ATTR_DATA data_fan14_status = {FAN14_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan14_status);
FAN_SYSFS_ATTR_DATA data_fan15_status = {FAN15_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan15_status);
FAN_SYSFS_ATTR_DATA data_fan16_status = {FAN16_STATUS, S_IRUGO, fan_show_status, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan16_status);

/* Some generic fan attributes */
FAN_SYSFS_ATTR_DATA data_fan_duty_cycle = {FAN_DUTY_CYCLE, S_IRUGO | S_IWUSR, fan_show_default, NULL, sonic_i2c_get_fan_dc_default, NULL, fan_store_default, NULL, sonic_i2c_set_fan_dc_default, NULL, NULL};
EXPORT_SYMBOL(data_fan_duty_cycle);

FAN_SYSFS_ATTR_DATA data_fan_model_name = {FAN_MODEL_NAME, S_IRUGO, fan_show_string, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan_model_name);

FAN_SYSFS_ATTR_DATA data_fan_serial_num = {FAN_SERIAL_NUM, S_IRUGO, fan_show_string, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan_serial_num);

FAN_SYSFS_ATTR_DATA data_fan_part_num = {FAN_PART_NUM, S_IRUGO, fan_show_string, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan_part_num);

FAN_SYSFS_ATTR_DATA data_fan_hw_version = {FAN_HW_VERSION, S_IRUGO, fan_show_string, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(data_fan_hw_version);

FAN_SYSFS_ATTR_DATA_ENTRY fan_sysfs_attr_data_tbl[]=
{
	{ "fan1_present", &data_fan1_present},
    { "fan2_present", &data_fan2_present},
    { "fan3_present", &data_fan3_present},
    { "fan4_present", &data_fan4_present},
    { "fan5_present", &data_fan5_present},
    { "fan6_present", &data_fan6_present},
	{ "fan7_present", &data_fan7_present},
    { "fan8_present", &data_fan8_present},
    { "fan9_present", &data_fan9_present},
    { "fan10_present", &data_fan10_present},
    { "fan11_present", &data_fan11_present},
    { "fan12_present", &data_fan12_present},
    { "fan13_present", &data_fan13_present},
    { "fan14_present", &data_fan14_present},
    { "fan15_present", &data_fan15_present},
    { "fan16_present", &data_fan16_present},
	{ "fan1_direction", &data_fan1_direction},
    { "fan2_direction", &data_fan2_direction},
    { "fan3_direction", &data_fan3_direction},
    { "fan4_direction", &data_fan4_direction},
    { "fan5_direction", &data_fan5_direction},
    { "fan6_direction", &data_fan6_direction},
	{ "fan7_direction", &data_fan7_direction},
    { "fan8_direction", &data_fan8_direction},
    { "fan9_direction", &data_fan9_direction},
    { "fan10_direction", &data_fan10_direction},
    { "fan11_direction", &data_fan11_direction},
    { "fan12_direction", &data_fan12_direction},
    { "fan13_direction", &data_fan13_direction},
    { "fan14_direction", &data_fan14_direction},
    { "fan15_direction", &data_fan15_direction},
    { "fan16_direction", &data_fan16_direction},
	{ "fan1_input", &data_fan1_input},
    { "fan2_input", &data_fan2_input},
    { "fan3_input", &data_fan3_input},
    { "fan4_input", &data_fan4_input},
    { "fan5_input", &data_fan5_input},
    { "fan6_input", &data_fan6_input},
	{ "fan7_input", &data_fan7_input},
    { "fan8_input", &data_fan8_input},
    { "fan9_input", &data_fan9_input},
    { "fan10_input", &data_fan10_input},
    { "fan11_input", &data_fan11_input},
    { "fan12_input", &data_fan12_input},
    { "fan13_input", &data_fan13_input},
    { "fan14_input", &data_fan14_input},
    { "fan15_input", &data_fan15_input},
    { "fan16_input", &data_fan16_input},
	{ "fan1_pwm", &data_fan1_pwm},
    { "fan2_pwm", &data_fan2_pwm},
    { "fan3_pwm", &data_fan3_pwm},
    { "fan4_pwm", &data_fan4_pwm},
    { "fan5_pwm", &data_fan5_pwm},
    { "fan6_pwm", &data_fan6_pwm},
	{ "fan7_pwm", &data_fan7_pwm},
    { "fan8_pwm", &data_fan8_pwm},
    { "fan9_pwm", &data_fan9_pwm},
    { "fan10_pwm", &data_fan10_pwm},
    { "fan11_pwm", &data_fan11_pwm},
    { "fan12_pwm", &data_fan12_pwm},
    { "fan13_pwm", &data_fan13_pwm},
    { "fan14_pwm", &data_fan14_pwm},
    { "fan15_pwm", &data_fan15_pwm},
    { "fan16_pwm", &data_fan16_pwm},
	{ "fan1_fault", &data_fan1_fault},
    { "fan2_fault", &data_fan2_fault},
    { "fan3_fault", &data_fan3_fault},
    { "fan4_fault", &data_fan4_fault},
    { "fan5_fault", &data_fan5_fault},
    { "fan6_fault", &data_fan6_fault},
	{ "fan7_fault", &data_fan7_fault},
    { "fan8_fault", &data_fan8_fault},
    { "fan9_fault", &data_fan9_fault},
    { "fan10_fault", &data_fan10_fault},
    { "fan11_fault", &data_fan11_fault},
    { "fan12_fault", &data_fan12_fault},
    { "fan13_fault", &data_fan13_fault},
    { "fan14_fault", &data_fan14_fault},
    { "fan15_fault", &data_fan15_fault},
    { "fan16_fault", &data_fan16_fault},
	{ "fan1_status", &data_fan1_status},
    { "fan2_status", &data_fan2_status},
    { "fan3_status", &data_fan3_status},
    { "fan4_status", &data_fan4_status},
    { "fan5_status", &data_fan5_status},
    { "fan6_status", &data_fan6_status},
	{ "fan7_status", &data_fan7_status},
    { "fan8_status", &data_fan8_status},
    { "fan9_status", &data_fan9_status},
    { "fan10_status", &data_fan10_status},
    { "fan11_status", &data_fan11_status},
    { "fan12_status", &data_fan12_status},
    { "fan13_status", &data_fan13_status},
    { "fan14_status", &data_fan14_status},
    { "fan15_status", &data_fan15_status},
    { "fan16_status", &data_fan16_status},
    { "fan_duty_cycle", &data_fan_duty_cycle},
    { "fan_model_name", &data_fan_model_name},
    { "fan_serial_num", &data_fan_serial_num},
    { "fan_part_num", &data_fan_part_num},
    { "fan_hw_version", &data_fan_hw_version},
};

void *get_fan_access_data(char *name)
{
	int i=0;
	for(i=0; i<(sizeof(fan_sysfs_attr_data_tbl)/sizeof(fan_sysfs_attr_data_tbl[0])); i++)
	{
		if(strcmp(name, fan_sysfs_attr_data_tbl[i].name) ==0)
		{
			return &fan_sysfs_attr_data_tbl[i];
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_fan_access_data);



static int pddf_fan_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct fan_data *data;
    int status=0,i,num, j=0;
	FAN_PDATA *fan_platform_data;
    FAN_DATA_ATTR *data_attr;
    FAN_SYSFS_ATTR_DATA_ENTRY *sysfs_data_entry;
    FAN_SYSFS_ATTR_DATA_ENTRY *extra_sysfs_data_entry;
	char new_duplicate_str[ATTR_NAME_LEN] = "";
	char new_default_str[ATTR_NAME_LEN] = "";
    int idx = 0;

	if (client == NULL) {
        printk("NULL Client.. \n");
        goto exit;
    }

	if (pddf_fan_ops.pre_probe)
	{
		status = (pddf_fan_ops.pre_probe)(client, dev_id);
		if (status != 0)
			goto exit;
	}

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        status = -EIO;
        goto exit;
    }

	/* Add support for a pre probe function */
    data = kzalloc(sizeof(struct fan_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    dev_info(&client->dev, "chip found\n");


	/*Take control of the platform data*/
	fan_platform_data = (FAN_PDATA *)(client->dev.platform_data);
	num = fan_platform_data->len;
	data->num_attr = num;

	for (i=0;i<num;i++)
	{
		/*struct attribute *aptr = NULL;*/
		struct sensor_device_attribute *dy_ptr = NULL;
        data_attr = fan_platform_data->fan_attrs + i;
		sysfs_data_entry = get_fan_access_data(data_attr->aname);
		if (sysfs_data_entry == NULL)
		{
			printk(KERN_ERR "%s: Wrong attribute name provided by user '%s'\n", __FUNCTION__, data_attr->aname);
			continue;
		}
			
		dy_ptr = (struct sensor_device_attribute *)kzalloc(sizeof(struct sensor_device_attribute)+ATTR_NAME_LEN, GFP_KERNEL);
        dy_ptr->dev_attr.attr.name = (char *)&dy_ptr[1];
        strcpy((char *)dy_ptr->dev_attr.attr.name, data_attr->aname);
        dy_ptr->dev_attr.attr.mode = sysfs_data_entry->a_ptr->mode;
        dy_ptr->dev_attr.show = sysfs_data_entry->a_ptr->show;
        dy_ptr->dev_attr.store = sysfs_data_entry->a_ptr->store;
        dy_ptr->index = sysfs_data_entry->a_ptr->index;

        data->fan_attribute_list[i] = &dy_ptr->dev_attr.attr;
        strcpy(data->attr_info[i].name, data_attr->aname);
        data->attr_info[i].valid = 0;
		mutex_init(&data->attr_info[i].update_lock);

		/*Create a duplicate entry i.e. show, store funcs etc and other access data is same as data_attr->aname*/
        idx = dy_ptr->index;
		get_fan_duplicate_sysfs(idx, new_duplicate_str);
		if (strcmp(new_duplicate_str,""))
		{
			dy_ptr = (struct sensor_device_attribute *)kzalloc(sizeof(struct sensor_device_attribute)+ATTR_NAME_LEN, GFP_KERNEL);
			dy_ptr->dev_attr.attr.name = (char *)&dy_ptr[1];
			strcpy((char *)dy_ptr->dev_attr.attr.name, new_duplicate_str);
			dy_ptr->dev_attr.attr.mode = sysfs_data_entry->a_ptr->mode;
			dy_ptr->dev_attr.show = sysfs_data_entry->a_ptr->show;
			dy_ptr->dev_attr.store = sysfs_data_entry->a_ptr->store;
			dy_ptr->index = sysfs_data_entry->a_ptr->index;

			data->fan_attribute_list[num+j] = &dy_ptr->dev_attr.attr;
			j++;
			strcpy(new_duplicate_str, "");
		}
		/*Create a default sysfs entry which might not be present in the JSON file*/
		get_fan_extra_default_sysfs(idx, new_default_str);
		if (strcmp(new_default_str,""))
		{
		    extra_sysfs_data_entry = get_fan_access_data(new_default_str);
            if (extra_sysfs_data_entry == NULL)
            {
                printk(KERN_ERR "%s: Invalid name for extra default attribute '%s'. No access data exists\n", __FUNCTION__, new_default_str);
                continue;
            }
			dy_ptr = (struct sensor_device_attribute *)kzalloc(sizeof(struct sensor_device_attribute)+ATTR_NAME_LEN, GFP_KERNEL);
			dy_ptr->dev_attr.attr.name = (char *)&dy_ptr[1];
			strcpy((char *)dy_ptr->dev_attr.attr.name, new_default_str);
			dy_ptr->dev_attr.attr.mode = extra_sysfs_data_entry->a_ptr->mode;
			dy_ptr->dev_attr.show = extra_sysfs_data_entry->a_ptr->show;
			dy_ptr->dev_attr.store = extra_sysfs_data_entry->a_ptr->store;
			dy_ptr->index = extra_sysfs_data_entry->a_ptr->index;

			data->fan_attribute_list[num+j] = &dy_ptr->dev_attr.attr;
            strcpy(data->attr_info[num+j].name, new_default_str);
            data->attr_info[num+j].valid = 0;
		    mutex_init(&data->attr_info[num+j].update_lock);
			j++;
			strcpy(new_default_str, "");
		}
	}
	data->fan_attribute_list[i+j] = NULL;
	data->fan_attribute_group.attrs = data->fan_attribute_list;

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->fan_attribute_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register_with_info(&client->dev, client->name, NULL, NULL, NULL);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: fan '%s'\n",
         dev_name(data->hwmon_dev), client->name);

	/* Add a support for post probe function */
	if (pddf_fan_ops.post_probe)
	{
		status = (pddf_fan_ops.post_probe)(client, dev_id);
		if (status != 0)
			goto exit_remove;
	}

	return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &data->fan_attribute_group);
exit_free:
	/* Free all the allocated attributes */
    for (i=0; data->fan_attribute_list[i]!=NULL; i++)
    {
        struct sensor_device_attribute *ptr = (struct sensor_device_attribute *)data->fan_attribute_list[i];
        kfree(ptr);
    }
    pddf_dbg(FAN, KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
    kfree(data);
exit:
    return status;
}

static int pddf_fan_remove(struct i2c_client *client)
{
	int i = 0, ret = 0;
	struct sensor_device_attribute *ptr = NULL;
    struct fan_data *data = i2c_get_clientdata(client);
	FAN_PDATA *platdata = (FAN_PDATA *)client->dev.platform_data;
	FAN_DATA_ATTR *platdata_sub = platdata->fan_attrs;

	if (pddf_fan_ops.pre_remove)
	{
		ret = (pddf_fan_ops.pre_remove)(client);
		if (ret!=0)
			printk(KERN_ERR "FAN pre_remove function failed\n");
	}

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &data->fan_attribute_group);
    for (i=0; data->fan_attribute_list[i]!=NULL; i++)
    {
        ptr = (struct sensor_device_attribute *)data->fan_attribute_list[i];
        kfree(ptr);
    }
    pddf_dbg(FAN, KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
    kfree(data);

	if (platdata_sub) {
		printk(KERN_DEBUG "%s: Freeing platform subdata\n", __FUNCTION__);
		kfree(platdata_sub);
	}
	if (platdata) {
		printk(KERN_DEBUG "%s: Freeing platform data\n", __FUNCTION__);
		kfree(platdata);
	}

    if (pddf_fan_ops.post_remove)
    {
        ret = (pddf_fan_ops.post_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "FAN post_remove function failed\n");
    }

    return 0;
}

/* Addresses to scan */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

static const struct i2c_device_id pddf_fan_id[] = {
    { "fan_ctrl", 0 },
    { "fan_cpld", 1 },
    {}
};
MODULE_DEVICE_TABLE(i2c, pddf_fan_id);

static struct i2c_driver pddf_fan_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = DRVNAME,
    },
    .probe        = pddf_fan_probe,
    .remove       = pddf_fan_remove,
    .id_table     = pddf_fan_id,
    .address_list = normal_i2c,
};

static int __init pddf_fan_init(void)
{
	int status = 0;

	if (pddf_fan_ops.pre_init)
	{
		status = (pddf_fan_ops.pre_init)();
		if (status!=0)
			return status;
	}

	status = i2c_add_driver(&pddf_fan_driver);
	if (status!=0)
		return status;

	if (pddf_fan_ops.post_init)
    {
        status = (pddf_fan_ops.post_init)();
        if (status!=0)
            return status;
    }
	return status;

}

static void __exit pddf_fan_exit(void)
{
	if (pddf_fan_ops.pre_exit) (pddf_fan_ops.pre_exit)();
    i2c_del_driver(&pddf_fan_driver);
	if (pddf_fan_ops.post_exit) (pddf_fan_ops.post_exit)();
}

module_init(pddf_fan_init);
module_exit(pddf_fan_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("pddf_fan driver");
MODULE_LICENSE("GPL");
