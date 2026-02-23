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
 * Description:
 *  Platform FAN defines/structures header file
 */

#ifndef __PDDF_FAN_DEFS_H__
#define __PDDF_FAN_DEFS_H__


#define MAX_NUM_FAN 12
#define MAX_FAN_ATTRS 128
#define ATTR_NAME_LEN 32
#define STR_ATTR_SIZE 32
#define DEV_TYPE_LEN 32

/* Each client has this additional data 
 */

typedef struct FAN_DATA_ATTR
{
    char aname[ATTR_NAME_LEN];                    // attr name, taken from enum fan_sysfs_attributes
    char devtype[DEV_TYPE_LEN];       // Type of FAN controller, i.e EMC2305, EMC2302, or FAN-CPLD etc
    char devname[DEV_TYPE_LEN];       // Name of the device from where this informatin is to be read
    uint32_t devaddr;
    uint32_t offset;
    uint32_t mask;
    uint32_t cmpval;
    uint32_t len;
    int mult;                       // Multiplication factor to get the actual data
    uint8_t is_divisor;                     // Check if the value is a divisor and mult is dividend
    void *access_data;
}FAN_DATA_ATTR;

typedef struct FAN_SYSFS_ATTR_DATA
{
    int index;
    unsigned short mode;
    ssize_t (*show)(struct device *dev, struct device_attribute *da, char *buf);
    int (*pre_get)(void *client, FAN_DATA_ATTR *adata, void *data);
    int (*do_get)(void *client, FAN_DATA_ATTR *adata, void *data);
    int (*post_get)(void *client, FAN_DATA_ATTR *adata, void *data);
    ssize_t (*store)(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
    int (*pre_set)(void *client, FAN_DATA_ATTR *adata, void *data);
    int (*do_set)(void *client, FAN_DATA_ATTR *adata, void *data);
    int (*post_set)(void *client, FAN_DATA_ATTR *adata, void *data);
    void *data;
} FAN_SYSFS_ATTR_DATA;

typedef struct FAN_SYSFS_ATTR_DATA_ENTRY
{
    char name[ATTR_NAME_LEN];
    FAN_SYSFS_ATTR_DATA *a_ptr;
} FAN_SYSFS_ATTR_DATA_ENTRY;


/* FAN CLIENT DATA - PLATFORM DATA FOR FAN CLIENT */
typedef struct FAN_DATA
{
    int num_fantrays;                    // num of fans controlled by this fan client
    FAN_DATA_ATTR fan_attr;
    int len;             // no of valid attributes for this fan client
    FAN_DATA_ATTR fan_attrs[MAX_FAN_ATTRS];
}FAN_DATA;

typedef struct FAN_PDATA
{
    int num_fantrays;                    // num of fans controlled by this fan client
    int len;             // no of valid attributes for this fan client
    FAN_DATA_ATTR *fan_attrs; 
}FAN_PDATA;

struct pddf_fan_ops_t
{
    /*Fan duty cycle conversion ops*/
    uint32_t (*duty_cycle_to_reg_value)(uint32_t dc);
    uint32_t (*reg_value_to_duty_cycle)(uint32_t reg_val);
};

extern int board_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int board_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

extern int board_i2c_fpga_read(unsigned short cpld_addr, u8 reg);
extern int board_i2c_fpga_write(unsigned short cpld_addr, u8 reg, u8 value);
#endif
