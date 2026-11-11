/*
 * A header definition for dfd_cfg_adapter driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
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

#ifndef __DFD_CFG_ADAPTER_H__
#define __DFD_CFG_ADAPTER_H__

#define DFD_KO_CPLD_I2C_RETRY_SLEEP            (10)  /* ms */
#define DFD_KO_CPLD_I2C_RETRY_TIMES            (50 / DFD_KO_CPLD_I2C_RETRY_SLEEP)

#define DFD_KO_CPLD_GET_SLOT(addr)             ((addr >> 24) & 0xff)
#define DFD_KO_CPLD_GET_ID(addr)               ((addr >> 16) & 0xff)
#define DFD_KO_CPLD_GET_INDEX(addr)            (addr & 0xffff)
#define DFD_KO_CPLD_MODE_I2C_STRING            "i2c"
#define DFD_KO_CPLD_MODE_LPC_STRING            "lpc"

#define DFD_KO_OTHER_I2C_GET_MAIN_ID(addr)     ((addr >> 24) & 0xff)
#define DFD_KO_OTHER_I2C_GET_INDEX(addr)       ((addr >> 16) & 0xff)
#define DFD_KO_OTHER_I2C_GET_OFFSET(addr)      (addr & 0xffff)
#define DFD_SYSFS_PATH_MAX_LEN                 (64)

typedef struct dfd_i2c_dev_s {
    int bus;        /* bus number */
    int addr;       /* Bus address */
} dfd_i2c_dev_t;

/* dfd_i2c_dev_t member macro */
typedef enum dfd_i2c_dev_mem_s {
    DFD_I2C_DEV_MEM_BUS,
    DFD_I2C_DEV_MEM_ADDR,
    DFD_I2C_DEV_MEM_END
} dfd_i2c_dev_mem_t;

typedef enum cpld_mode_e {
    DFD_CPLD_MODE_I2C,     /* I2C bus */
    DFD_CPLD_MODE_LPC,      /*LPC bus*/
} cpld_mode_t;

/* i2c access mode */
typedef enum i2c_mode_e {
    DFD_I2C_MODE_NORMAL_I2C,    /* I2C bus */
    DFD_I2C_MODE_SMBUS,         /* SMBUS bus */
} i2c_mode_t;

/* Global variable */
extern char *g_dfd_i2c_dev_mem_str[DFD_I2C_DEV_MEM_END];      /* dfd_i2c_dev_t member string */

/**
 * dfd_ko_cpld_read - cpld read operation
 * @addr: Offset address
 * @buf: data
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_cpld_read(int32_t addr, uint8_t *buf);

/**
 * dfd_ko_cpld_write - cpld write operation
 * @addr: address
 * @data: data
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_cpld_write(int32_t addr, uint8_t val);

/**
 * dfd_ko_i2c_read - I2C read operation
 * @bus: I2C bus
 * @addr: I2C device address
 * @offset:register offset
 * @buf:Read buffer
 * @size:Read length
 * @sysfs_name:sysfs attribute name
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_i2c_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size, const char *sysfs_name);

/**
 * dfd_ko_i2c_write - I2C write operation
 * @bus: I2C bus
 * @addr: I2C device address
 * @offset:register offset
 * @buf:write buffer
 * @size: write length
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_i2c_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size);

/**
 * dfd_ko_read_file - File read operation
 * @fpath: File path
 * @addr: address
 * @val: data
 * @read_bytes: length
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_read_file(char *fpath, int32_t addr, uint8_t *val, int32_t read_bytes);

/**
 * dfd_ko_write_file - File write operation
 * @fpath: File path
 * @addr: address
 * @val:  data
 * @write_bytes: length
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_write_file(char *fpath, int32_t addr, uint8_t *val, int32_t write_bytes);

/**
 * dfd_ko_other_i2c_dev_read - other_i2c read operation
 * @addr: address
 * @val: data
 * @read_len: length
 *
 * @returns: <0 Failed, others succeeded
 */
int32_t dfd_ko_other_i2c_dev_read(int32_t addr, uint8_t *value, int32_t read_len);
#endif /* __DFD_CFG_ADAPTER_H__ */
