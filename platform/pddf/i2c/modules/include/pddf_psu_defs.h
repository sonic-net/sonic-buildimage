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
 *  Platform PSU defines/structures header file
 */

#ifndef __PDDF_PSU_DEFS_H__
#define __PDDF_PSU_DEFS_H__


#define MAX_NUM_PSU 5
#define MAX_PSU_ATTRS 32
#define ATTR_NAME_LEN 32
#define STR_ATTR_SIZE 32
#define DEV_TYPE_LEN 32
#define ATTR_DATA_FORMAT_SIZE 32

/* Each client has this additional data 
 */

typedef struct PSU_DATA_ATTR
{
    char aname[ATTR_NAME_LEN];                    // attr name, taken from enum psu_sysfs_attributes
    char data_format[ATTR_DATA_FORMAT_SIZE];      // PMBUS number format, either linear11, linear16, or direct   
    char devtype[DEV_TYPE_LEN];       // either a 'eeprom' or 'cpld', or 'pmbus' attribute
    char devname[DEV_TYPE_LEN];       // Name of the device from where this sysfs attr is read
    uint32_t devaddr;
    uint32_t offset;
    uint32_t mask;
    uint32_t cmpval;
    uint32_t len;
    int m;
    int b;
    int r;
    void *access_data;
}PSU_DATA_ATTR;

typedef struct PSU_SYSFS_ATTR_DATA
{
    int index;
    unsigned short mode;
    ssize_t (*show)(struct device *dev, struct device_attribute *da, char *buf);
    int (*pre_get)(void *client, PSU_DATA_ATTR *adata, void *data);
    int (*do_get)(void *client, PSU_DATA_ATTR *adata, void *data);
    int (*post_get)(void *client, PSU_DATA_ATTR *adata, void *data);
    ssize_t (*store)(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
    int (*pre_set)(void *client, PSU_DATA_ATTR *adata, void *data);
    int (*do_set)(void *client, PSU_DATA_ATTR *adata, void *data);
    int (*post_set)(void *client, PSU_DATA_ATTR *adata, void *data);
    void *data;
} PSU_SYSFS_ATTR_DATA;

typedef struct PSU_SYSFS_ATTR_DATA_ENTRY
{
    char name[ATTR_NAME_LEN];
    PSU_SYSFS_ATTR_DATA *a_ptr;
} PSU_SYSFS_ATTR_DATA_ENTRY;


/* PSU CLIENT DATA - PLATFORM DATA FOR PSU CLIENT */
typedef struct PSU_DATA
{
    int idx;    // psu index
    int num_psu_fans;
    int num_psu_thermals;
    u32 psu_temp_high_thresh_bitmap;
    PSU_DATA_ATTR psu_attr;
    int len;             // no of valid attributes for this psu client
    PSU_DATA_ATTR psu_attrs[MAX_PSU_ATTRS]; 
}PSU_DATA;

typedef struct PSU_PDATA
{
    int idx;                    // psu index
    int num_psu_fans;      // num of fans supported by the PSU
    int num_psu_thermals; // num of thermals supported by the PSU
    /*
     * Bitmap of supported thermal thresholds
     * Bit 0 (LSB) corresponds to thermal sensor 1, bit 1 to sensor 2, etc.
     * A bit value of 1 means the sensor supports high threshold.
     * Needed as some PSUs may not support high threshold for all thermal sensors.
     */
    u32 psu_temp_high_thresh_bitmap;
    int len;             // no of valid attributes for this psu client
    PSU_DATA_ATTR *psu_attrs; 
}PSU_PDATA;

extern int board_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int board_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

#endif
