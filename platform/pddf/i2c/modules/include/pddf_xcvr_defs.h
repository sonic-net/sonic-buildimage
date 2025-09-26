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
 *  Platform SFP defines/structures header file
 */

#ifndef __PDDF_XCVR_DEFS_H__
#define __PDDF_XCVR_DEFS_H__


#define MAX_NUM_XCVR 5
#define MAX_XCVR_ATTRS 20

typedef struct XCVR_ATTR
{
    char aname[32];                    // attr name, taken from enum xcvr_sysfs_attributes
    char devtype[32];       // either a 'eeprom' or 'cpld', or 'pmbus' attribute
    char devname[32];       // name of the device from where this sysfs is to be read
    uint32_t devaddr;
    uint32_t offset;
    uint32_t mask;
    uint32_t cmpval;
    uint32_t len;

    int (*pre_access)(void *client, void *data);
    int (*do_access)(void *client, void *data);
    int (*post_access)(void *client, void *data);

}XCVR_ATTR;

/* XCVR CLIENT DATA - PLATFORM DATA FOR XCVR CLIENT */
typedef struct XCVR_DATA
{
    int idx;                    // xcvr index
    XCVR_ATTR xcvr_attr;
    int len;             // no of valid attributes for this xcvr client
    XCVR_ATTR xcvr_attrs[MAX_XCVR_ATTRS];
}XCVR_DATA;

typedef struct XCVR_PDATA
{
    int idx;                    // xcvr index
    unsigned short addr;      // i2c address of the device
    int len;             // no of valid attributes for this xcvr client
    XCVR_ATTR *xcvr_attrs; 
}XCVR_PDATA;


#define BIT_INDEX(i)            (1ULL << (i))

/* List of valid port types */
typedef enum xcvr_port_type_e {
    PDDF_PORT_TYPE_INVALID,
    PDDF_PORT_TYPE_NOT_PRESENT,
    PDDF_PORT_TYPE_SFP,
    PDDF_PORT_TYPE_SFP_PLUS,
    PDDF_PORT_TYPE_QSFP,
    PDDF_PORT_TYPE_QSFP_PLUS,
    PDDF_PORT_TYPE_QSFP28
} xcvr_port_type_t;

/* Each client has this additional data
 */
struct xcvr_data {
    struct device       *xdev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    int                 index;           /* port index */
    xcvr_port_type_t    port_type;
    uint32_t            modpres;
    uint32_t            reset;
    uint32_t            intr_status;
    uint32_t            lpmode;
    uint32_t            rxlos;
    uint32_t            txdisable;
    uint32_t            txfault;
};

typedef struct XCVR_SYSFS_ATTR_OPS
{
    int index;
    ssize_t (*show)(struct device *dev, struct device_attribute *da, char *buf);
    int (*pre_get)(struct i2c_client *client, XCVR_ATTR *adata, struct xcvr_data *data);
    int (*do_get)(struct i2c_client *client, XCVR_ATTR *adata, struct xcvr_data *data);
    int (*post_get)(struct i2c_client *client, XCVR_ATTR *adata, struct xcvr_data *data);
    ssize_t (*store)(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
    int (*pre_set)(struct i2c_client *client, XCVR_ATTR *adata, struct xcvr_data *data);
    int (*do_set)(struct i2c_client *client, XCVR_ATTR *adata, struct xcvr_data *data);
    int (*post_set)(struct i2c_client *client, XCVR_ATTR *adata, struct xcvr_data *data);
} XCVR_SYSFS_ATTR_OPS;

enum xcvr_sysfs_attributes {
    XCVR_PRESENT,
    XCVR_RESET,
    XCVR_INTR_STATUS,
    XCVR_LPMODE,
    XCVR_RXLOS,
    XCVR_TXDISABLE,
    XCVR_TXFAULT,
    XCVR_ATTR_MAX
};

extern int board_i2c_cpld_read_new(unsigned short cpld_addr, char *name, u8 reg);
extern int board_i2c_cpld_write_new(unsigned short cpld_addr, char *name, u8 reg, u8 value);

#endif 
