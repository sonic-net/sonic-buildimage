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
 * Description
 *  FAN driver related data structures
 */
#ifndef __PDDF_FAN_DRIVER_H__
#define __PDDF_FAN_DRIVER_H__

enum fan_sysfs_attributes {
    FAN1_PRESENT,
    FAN2_PRESENT,
    FAN3_PRESENT,
    FAN4_PRESENT,
    FAN5_PRESENT,
    FAN6_PRESENT,
    FAN7_PRESENT,
    FAN8_PRESENT,
    FAN9_PRESENT,
    FAN10_PRESENT,
    FAN11_PRESENT,
    FAN12_PRESENT,
    FAN13_PRESENT,
    FAN14_PRESENT,
    FAN15_PRESENT,
    FAN16_PRESENT,
    FAN1_DIRECTION,
    FAN2_DIRECTION,
    FAN3_DIRECTION,
    FAN4_DIRECTION,
    FAN5_DIRECTION,
    FAN6_DIRECTION,
    FAN7_DIRECTION,
    FAN8_DIRECTION,
    FAN9_DIRECTION,
    FAN10_DIRECTION,
    FAN11_DIRECTION,
    FAN12_DIRECTION,
    FAN13_DIRECTION,
    FAN14_DIRECTION,
    FAN15_DIRECTION,
    FAN16_DIRECTION,
    FAN1_INPUT,
    FAN2_INPUT,
    FAN3_INPUT,
    FAN4_INPUT,
    FAN5_INPUT,
    FAN6_INPUT,
    FAN7_INPUT,
    FAN8_INPUT,
    FAN9_INPUT,
    FAN10_INPUT,
    FAN11_INPUT,
    FAN12_INPUT,
    FAN13_INPUT,
    FAN14_INPUT,
    FAN15_INPUT,
    FAN16_INPUT,
    FAN1_PWM,
    FAN2_PWM,
    FAN3_PWM,
    FAN4_PWM,
    FAN5_PWM,
    FAN6_PWM,
    FAN7_PWM,
    FAN8_PWM,
    FAN9_PWM,
    FAN10_PWM,
    FAN11_PWM,
    FAN12_PWM,
    FAN13_PWM,
    FAN14_PWM,
    FAN15_PWM,
    FAN16_PWM,
    FAN1_FAULT,
    FAN2_FAULT,
    FAN3_FAULT,
    FAN4_FAULT,
    FAN5_FAULT,
    FAN6_FAULT,
    FAN7_FAULT,
    FAN8_FAULT,
    FAN9_FAULT,
    FAN10_FAULT,
    FAN11_FAULT,
    FAN12_FAULT,
    FAN13_FAULT,
    FAN14_FAULT,
    FAN15_FAULT,
    FAN16_FAULT,
    FAN1_STATUS,
    FAN2_STATUS,
    FAN3_STATUS,
    FAN4_STATUS,
    FAN5_STATUS,
    FAN6_STATUS,
    FAN7_STATUS,
    FAN8_STATUS,
    FAN9_STATUS,
    FAN10_STATUS,
    FAN11_STATUS,
    FAN12_STATUS,
    FAN13_STATUS,
    FAN14_STATUS,
    FAN15_STATUS,
    FAN16_STATUS,
    FAN_DUTY_CYCLE,
    FAN_MODEL_NAME,
    FAN_SERIAL_NUM,
    FAN_PART_NUM,
    FAN_HW_VERSION,
	FAN_MAX_ATTR 
};
/* Each client has this additional data */
struct fan_attr_info {
	char				name[ATTR_NAME_LEN];
    struct mutex		update_lock;
    char				valid;           /* != 0 if registers are valid */
    unsigned long		last_updated;    /* In jiffies */
	union {
        char strval[STR_ATTR_SIZE];
        int  intval;
        u16  shortval;
        u8   charval;
    }val;
};

struct fan_data {
    struct device			*hwmon_dev;
	int						num_attr;
	struct attribute		*fan_attribute_list[MAX_FAN_ATTRS];
	struct attribute_group	fan_attribute_group;
	struct fan_attr_info	attr_info[MAX_FAN_ATTRS];
};

#endif
