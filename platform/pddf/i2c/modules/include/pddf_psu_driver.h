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
 *  PSU driver data structures
 */
#ifndef __PDDF_PSU_DRIVER_H__
#define __PDDF_PSU_DRIVER_H__

enum psu_sysfs_attributes {
    PSU_PRESENT,
    PSU_MODEL_NAME,
    PSU_POWER_GOOD,
	PSU_MFR_ID,
	PSU_SERIAL_NUM,
	PSU_FAN_DIR,
    PSU_V_OUT,
    PSU_V_OUT_MIN,
    PSU_V_OUT_MAX,
    PSU_I_OUT,
    PSU_P_OUT, /* This is in micro watts to comply with lm-sensors */
    PSU_P_OUT_MAX,
    PSU_FAN1_SPEED,
    PSU_TEMP1_INPUT,
    PSU_TEMP1_HIGH_THRESHOLD,
    PSU_TEMP2_INPUT,
    PSU_TEMP2_HIGH_THRESHOLD,
    PSU_TEMP3_INPUT,
    PSU_TEMP3_HIGH_THRESHOLD,
    PSU_V_IN,
    PSU_I_IN,
	PSU_P_IN,
	PSU_ATTR_MAX
};


/* Every client has psu_data which is divided into per attribute data */
struct psu_attr_info {
	char				name[ATTR_NAME_LEN];
	struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
	u8					status;
	union {
		char strval[STR_ATTR_SIZE];
		int	 intval;
		u16	 shortval;
		u8   charval;
	}val;
};
struct psu_data {
	struct device			*hwmon_dev;
	u8						index;
	int						num_psu_fans;
	int						num_psu_thermals;
	/*
	 * Bitmap of supported thermal thresholds
	 * Bit 0 (LSB) corresponds to thermal sensor 1, bit 1 to sensor 2, etc.
	 * A bit value of 1 means the sensor supports high threshold.
	 * Needed as some PSUs may not support high threshold for all thermal sensors.
	 */
	u32						psu_temp_high_thresh_bitmap;
	int						num_attr;
	struct attribute		*psu_attribute_list[MAX_PSU_ATTRS];
	struct attribute_group	psu_attribute_group;
	struct psu_attr_info	attr_info[MAX_PSU_ATTRS];
};


#endif
