/*
 * An dfd_cfg_adapter driver for cfg of adapter devcie function
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

#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/uio.h>
#include "wb_module.h"
#include "dfd_cfg_file.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"

/* dfd_i2c_dev_t member string */
char *g_dfd_i2c_dev_mem_str[DFD_I2C_DEV_MEM_END] = {
    ".bus",
    ".addr",
};

static dfd_i2c_dev_t* dfd_ko_get_cpld_i2c_dev(int sub_slot, int cpld_id)
{
    uint64_t key;
    dfd_i2c_dev_t *i2c_dev;

    /* Read configuration value */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_I2C_DEV, sub_slot, cpld_id);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DBG_DEBUG(DBG_ERROR, "get cpld[%d] i2c dev config fail, key_name=%s\n",
            cpld_id, key_to_name(DFD_CFG_ITEM_CPLD_I2C_DEV));
        return NULL;
    }

    return i2c_dev;
}

static int dfd_ko_i2c_block_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    struct file *fp;
    struct i2c_client client;
    int i;
    int rv = 0;
    char i2c_path[32];

    mem_clear(i2c_path, 32);
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", bus);

    fp = filp_open(i2c_path, O_RDWR, S_IRUSR | S_IWUSR);
    if (IS_ERR(fp)) {
        DBG_DEBUG(DBG_ERROR, "i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = addr;

    if (i2c_check_functionality(client.adapter, I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
        for (i = 0; i < size; i += 32) {
                rv = i2c_smbus_read_i2c_block_data(&client, offset + i, WB_MIN(32, size-i), buf + i);
            if (rv < 0) {
                DBG_DEBUG(DBG_ERROR, "i2c_block read failed, rv = %d\n", rv);
                rv = -DFD_RV_DEV_FAIL;
                goto out;
            }
            if (rv != 32) {
                break;
            }
        }
        rv = DFD_RV_OK;
    } else {
        rv = -DFD_RV_DEV_NOTSUPPORT;
    }

out:
    filp_close(fp, NULL);
    return rv;
}

static int32_t dfd_ko_i2c_smbus_transfer(int read_write, int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    int rv;
    struct i2c_adapter *i2c_adap;
    union i2c_smbus_data data;

    i2c_adap = i2c_get_adapter(bus);
    if (i2c_adap == NULL) {
        DBG_DEBUG(DBG_ERROR, "get i2c bus[%d] adapter fail\n", bus);
        return -DFD_RV_DEV_FAIL;
    }

    /* Operation i2c */
    if (read_write == I2C_SMBUS_WRITE) {
        data.byte = *buf;
    } else {
        data.byte = 0;
    }
    rv = i2c_smbus_xfer(i2c_adap, addr, 0, read_write, offset, I2C_SMBUS_BYTE_DATA, &data);
    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "i2c dev[bus=%d addr=0x%x offset=0x%x size=%d rw=%d] transfer fail, rv=%d\n",
            bus, addr, offset, size, read_write, rv);
        rv = -DFD_RV_DEV_FAIL;
    } else {
        DBG_DEBUG(DBG_VERBOSE, "i2c dev[bus=%d addr=0x%x offset=0x%x size=%d rw=%d] transfer success\n",
            bus, addr, offset, size, read_write);
        rv = DFD_RV_OK;
    }

    if (read_write == I2C_SMBUS_READ) {
        if (rv == DFD_RV_OK) {
            *buf = data.byte;
        } else {
            *buf = 0;
        }
    }

    i2c_put_adapter(i2c_adap);
    return rv;
}

static int32_t dfd_ko_i2c_read_bulk_data(int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    int i, rv;
    for (i = 0; i < DFD_KO_CPLD_I2C_RETRY_TIMES; i++) {
        rv = dfd_ko_i2c_block_read(bus, addr, offset, buf, size);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "[%d] read[offset=0x%x] fail, rv %d\r\n", i, addr, rv);
            msleep(DFD_KO_CPLD_I2C_RETRY_SLEEP);
        } else {
            DBG_DEBUG(DBG_VERBOSE, "[%d] read[offset=0x%x] success\r\n",
                i, addr);
            break;
        }
    }
    return rv;
}

static int32_t dfd_ko_i2c_read_data(int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    int i, rv;
    for (i = 0; i < DFD_KO_CPLD_I2C_RETRY_TIMES; i++) {
        rv = dfd_ko_i2c_smbus_transfer(I2C_SMBUS_READ, bus, addr, offset, buf, size);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "[%d]cpld read[offset=0x%x] fail, rv %d\n", i, addr, rv);
            msleep(DFD_KO_CPLD_I2C_RETRY_SLEEP);
        } else {
            DBG_DEBUG(DBG_VERBOSE, "[%d]cpld read[offset=0x%x] success, value=0x%x\n",
                i, addr, *buf);
            break;
        }
    }
    return rv;
}

static int32_t dfd_ko_i2c_write_data(int bus, int addr, int offset, uint8_t data, uint32_t size)
{
    int i, rv;
    for (i = 0; i < DFD_KO_CPLD_I2C_RETRY_TIMES; i++) {
        rv = dfd_ko_i2c_smbus_transfer(I2C_SMBUS_WRITE, bus, addr, offset, &data, size);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "[%d]cpld write[offset=0x%x] fail, rv=%d\n", i, addr, rv);
            msleep(DFD_KO_CPLD_I2C_RETRY_SLEEP);
        } else {
            DBG_DEBUG(DBG_VERBOSE, "[%d]cpld write[offset=0x%x, data=%d] success\n", i, addr, data);
            break;
        }
    }

    return rv;
}

/**
 * dfd_ko_cpld_i2c_read - cpld read operation
 * @offset: Offset address
 * @buf: data
 *
 * @returns: <0 Failure, other success
 */
static int32_t dfd_ko_cpld_i2c_read(int32_t addr, uint8_t *buf)
{
    int rv;
    int sub_slot, cpld_id, cpld_addr;
    dfd_i2c_dev_t *i2c_dev;

    if (buf == NULL) {
        DBG_DEBUG(DBG_ERROR, "input arguments error\n");
        return -DFD_RV_INDEX_INVALID;
    }

    /* Obtain the i2c device bus addr */
    sub_slot = DFD_KO_CPLD_GET_SLOT(addr);
    cpld_id = DFD_KO_CPLD_GET_ID(addr);
    cpld_addr = DFD_KO_CPLD_GET_INDEX(addr);

    i2c_dev = dfd_ko_get_cpld_i2c_dev(sub_slot, cpld_id);
    if (i2c_dev == NULL) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    rv = dfd_ko_i2c_read_data(i2c_dev->bus, i2c_dev->addr, cpld_addr, buf, sizeof(uint8_t));

    return rv;
}

/**
 * dfd_ko_cpld_i2c_write - cpld WRITE OPERATION
 * @offset: Offset address
 * @buf: data
 *
 * @returns: <0 Failure, other success
 */
static int32_t dfd_ko_cpld_i2c_write(int32_t addr, uint8_t data)
{
    int rv;
    int sub_slot, cpld_id, cpld_addr;
    dfd_i2c_dev_t *i2c_dev;

    /* Obtain the i2c device bus addr */
    sub_slot = DFD_KO_CPLD_GET_SLOT(addr);
    cpld_id = DFD_KO_CPLD_GET_ID(addr);
    cpld_addr = DFD_KO_CPLD_GET_INDEX(addr);

    i2c_dev = dfd_ko_get_cpld_i2c_dev(sub_slot, cpld_id);
    if (i2c_dev == NULL) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    rv = dfd_ko_i2c_write_data(i2c_dev->bus, i2c_dev->addr, cpld_addr, data, sizeof(uint8_t));

    return rv;
}

#ifdef CONFIG_X86
/**
 * dfd_ko_cpld_io_read - cpld io spatial read operation
 * @offset: address
 * @buf: data
 *
 * @returns: <0 Failure, other success
 */
static int32_t dfd_ko_cpld_io_read(int32_t addr, uint8_t *buf)
{
    uint64_t key;
    int cpld_id, sub_slot, offset;
    int *tmp;
    uint16_t io_port;

    sub_slot = DFD_KO_CPLD_GET_SLOT(addr);
    cpld_id = DFD_KO_CPLD_GET_ID(addr);
    offset = DFD_KO_CPLD_GET_INDEX(addr);

    /* int Type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_LPC_DEV, sub_slot, cpld_id);
    tmp = dfd_ko_cfg_get_item(key);
    if (tmp == NULL) {
        DBG_DEBUG(DBG_ERROR,"get cpld io base config fail, key_name=%s\n",
            key_to_name(DFD_CFG_ITEM_CPLD_LPC_DEV));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    io_port = (u16)(*tmp) + offset;
    *buf = inb(io_port);
    DBG_DEBUG(DBG_VERBOSE, "read cpld io port addr 0x%x, data 0x%x\n", io_port, *buf);

    return DFD_RV_OK;

}
#endif

#ifdef CONFIG_X86
/**
 * dfd_ko_cpld_io_write - cpld io space WRITE OPERATION
 * @addr: address
 * @data: data
 *
 * @returns: <0 Failure, other success
 */
static int32_t dfd_ko_cpld_io_write(int32_t addr, uint8_t data)
{
    uint64_t key;
    int cpld_id, sub_slot, offset;
    int *tmp;
    uint16_t io_port;

    sub_slot = DFD_KO_CPLD_GET_SLOT(addr);
    cpld_id = DFD_KO_CPLD_GET_ID(addr);
    offset = DFD_KO_CPLD_GET_INDEX(addr);

    /* int Type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_LPC_DEV, sub_slot, cpld_id);
    tmp = dfd_ko_cfg_get_item(key);
    if (tmp == NULL) {
        DBG_DEBUG(DBG_ERROR, "get cpld io base config fail, key_name=%s\n",
            key_to_name(DFD_CFG_ITEM_CPLD_LPC_DEV));
        return -1;
    }

    io_port = (u16)(*tmp) + offset;
    DBG_DEBUG(DBG_VERBOSE, "write cpld io port addr 0x%x, data 0x%x\n", io_port, data);
    outb(data, (u16)io_port);

    return DFD_RV_OK;
}
#endif

static int dfd_cfg_get_cpld_mode(int sub_slot, int cpld_id, int *mode)
{
    uint64_t key;
    char *name;

    if (mode == NULL) {
        DBG_DEBUG(DBG_ERROR, "input arguments error\n");
        return -DFD_RV_TYPE_ERR;
    }

    /* string type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CPLD_MODE, sub_slot, cpld_id);
    name = dfd_ko_cfg_get_item(key);
    if (name == NULL) {
        DBG_DEBUG(DBG_ERROR, "get cpld[%d] mode info ctrl fail, key_name=%s\n",
            cpld_id, key_to_name(DFD_CFG_ITEM_CPLD_MODE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_DEBUG(DBG_VERBOSE, "cpld_id %d mode_name %s.\n", cpld_id, name);
    if (!strncmp(name, DFD_KO_CPLD_MODE_I2C_STRING, strlen(DFD_KO_CPLD_MODE_I2C_STRING))) {
        *mode = DFD_CPLD_MODE_I2C;
    } else if (!strncmp(name, DFD_KO_CPLD_MODE_LPC_STRING, strlen(DFD_KO_CPLD_MODE_LPC_STRING))) {
        *mode = DFD_CPLD_MODE_LPC;
    } else {
        /* The default mode is I2C */
        *mode = DFD_CPLD_MODE_I2C;
    }

    DBG_DEBUG(DBG_VERBOSE, "cpld_id %d mode %d.\n", cpld_id, *mode);
    return 0;
}

/**
 * dfd_ko_cpld_read - cpld read operation, read only one byte
 * @offset: Offset address
 * @buf: data
 *
 * @returns: <0 Failure, other success
 */
int32_t dfd_ko_cpld_read(int32_t addr, uint8_t *buf)
{
    int ret;
    int sub_slot, cpld_id;
    int cpld_mode;

    sub_slot = DFD_KO_CPLD_GET_SLOT(addr);
    cpld_id = DFD_KO_CPLD_GET_ID(addr);
    /* cpld mode, including I2C and LPC. Other modes are not supported */
    ret = dfd_cfg_get_cpld_mode(sub_slot, cpld_id, &cpld_mode);
    if (ret) {
        DBG_DEBUG(DBG_WARN, "drv_get_cpld_mode sub_slot %d cpldid %d faile, set default i2c mode.\n", sub_slot, cpld_id);
        cpld_mode = DFD_CPLD_MODE_I2C;
    }

    if (cpld_mode == DFD_CPLD_MODE_I2C) {
        ret = dfd_ko_cpld_i2c_read(addr, buf);
    } else if (cpld_mode == DFD_CPLD_MODE_LPC) {
#ifdef CONFIG_X86
        ret = dfd_ko_cpld_io_read(addr, buf);
#else
        DBG_DEBUG(DBG_ERROR, "ERROR:only x86 arch support cpld_mode %d.\n", cpld_mode);
        ret = -DFD_RV_DEV_NOTSUPPORT;
#endif
    } else {
        DBG_DEBUG(DBG_ERROR, "cpld_mode %d invalid.\n", cpld_mode);
        ret = -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_DEBUG(DBG_VERBOSE, "addr 0x%x val 0x%x ret %d\n", addr, *buf, ret);
    return ret;
}

/**
 * dfd_ko_cpld_write - cpld WRITE OPERATION Write a byte
 * @offset: Offset address
 * @buf: data
 *
 * @returns: <0 Failure, other success
 */
int32_t dfd_ko_cpld_write(int32_t addr, uint8_t val)
{
    int ret;
    int sub_slot, cpld_id, cpld_mode;

    sub_slot = DFD_KO_CPLD_GET_SLOT(addr);
    cpld_id = DFD_KO_CPLD_GET_ID(addr);

    ret = dfd_cfg_get_cpld_mode(sub_slot, cpld_id, &cpld_mode);
    if (ret) {
        DBG_DEBUG(DBG_ERROR, "drv_get_cpld_mode sub_slot %d cpldid %d faile, set default local_bus mode.\n", sub_slot, cpld_id);
        cpld_mode = DFD_CPLD_MODE_I2C;
    }

    if (cpld_mode == DFD_CPLD_MODE_I2C) {
        ret = dfd_ko_cpld_i2c_write(addr, val);
    } else if (cpld_mode == DFD_CPLD_MODE_LPC) {
#ifdef CONFIG_X86
        ret = dfd_ko_cpld_io_write(addr, val);
#else
        DBG_DEBUG(DBG_ERROR, "ERROR:only x86 arch support cpld_mode %d.\n", cpld_mode);
        ret = -DFD_RV_DEV_NOTSUPPORT;
#endif
    } else {
        DBG_DEBUG(DBG_ERROR, "cpld_mode %d invalid.\n", cpld_mode);
        ret = -DFD_RV_MODE_INVALID;
    }

    DBG_DEBUG(DBG_VERBOSE, "addr 0x%x val 0x%x ret %d\n", addr, val, ret);
    return ret;
}

/**
 * dfd_ko_i2c_read_tmp - I2C read operation
 * @bus: I2C BUS Device address
 * @offset:Register offset
 * @buf:Read buffer
 * @size:Read length
 * @returns: <0 Failure, other success
 */
static int32_t dfd_ko_i2c_read_tmp(int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    int i, rv;

    for (i = 0; i < size; i++) {
        rv = dfd_ko_i2c_read_data(bus, addr, offset, &buf[i], sizeof(uint8_t));
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "dfd_ko_i2c_read_data[bus=%d addr=0x%x offset=0x%x]fail, rv=%d\n",
                bus, addr, offset, rv);
            return rv;
        }
        offset++;
    }

    return size;
}

/**
 * dfd_ko_i2c_write - I2C  WRITE OPERATION
 * @bus: I2C BUS
 * @addr: I2C Device address
 * @offset:Register offset
 * @buf: Write buffer
 * @size:Write length
 * @returns: <0 Failure, other success
 */
int32_t dfd_ko_i2c_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    int i, rv;

    for (i = 0; i < size; i++) {
        rv = dfd_ko_i2c_write_data(bus, addr, offset, buf[i], sizeof(uint8_t));
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "dfd_ko_i2c_write[bus=%d addr=0x%x offset=0x%x]fail, rv=%d\n",
                bus, addr, offset, rv);
            return rv;
        }
        offset++;
    }

    return size;

}

/**
 * dfd_ko_read_file - File read operation
 * @fpath: File path
 * @addr: address
 * @val: data
 * @read_bytes: length
 *
 * @returns: <0 Failure, other success
 */
int32_t dfd_ko_read_file(char *fpath, int32_t addr, uint8_t *val, int32_t read_bytes)
{
    int32_t ret;
    struct file *filp;
    loff_t pos;

    struct kvec iov = {
        .iov_base = val,
        .iov_len = min_t(size_t, read_bytes, MAX_RW_COUNT),
    };
    struct iov_iter iter;

    if ((fpath == NULL) || (val == NULL) || (addr < 0) || (read_bytes < 0)) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, addr=%d read_bytes=%d\n", addr, read_bytes);
        return -DFD_RV_INDEX_INVALID;
    }

    /* Open file */
    filp = filp_open(fpath, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        DBG_DEBUG(DBG_ERROR, "open file[%s] fail\n", fpath);
        return -DFD_RV_DEV_FAIL;
    }
    /* Location file */
    pos = addr;
    iov_iter_kvec(&iter, ITER_DEST, &iov, 1, iov.iov_len);
    ret = vfs_iter_read(filp, &iter, &pos, 0);
    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "vfs_iter_read failed, path=%s, addr=%d, size=%d, ret=%d\n", fpath, addr, read_bytes, ret);
        ret = -DFD_RV_DEV_FAIL;
    }
    filp_close(filp, NULL);
    return ret;
}

/**
 * dfd_ko_other_i2c_dev_read - other_i2c read operation
 * @addr: address
 * @val: data
 * @read_bytes: length
 *
 * @returns: <0 Failure, other success
 */
int32_t dfd_ko_other_i2c_dev_read(int32_t addr, uint8_t *value, int32_t read_len)
{
    uint64_t key;
    int rv;
    int e2p_main_id, e2p_index, e2p_addr;
    dfd_i2c_dev_t *i2c_dev;

    if ((value == NULL) || (read_len <= 0)) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, read_len=%d\r\n", read_len);
        return -1;
    }

    e2p_main_id = DFD_KO_OTHER_I2C_GET_MAIN_ID(addr);
    e2p_index = DFD_KO_OTHER_I2C_GET_INDEX(addr);
    e2p_addr = DFD_KO_OTHER_I2C_GET_OFFSET(addr);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, e2p_main_id, e2p_index);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DBG_DEBUG(DBG_ERROR, "psu i2c dev config error, key_name: %s\r\n",
            key_to_name(DFD_CFG_ITEM_OTHER_I2C_DEV));
        return -DFD_RV_NODE_FAIL;
    }

    rv = dfd_ko_i2c_read_bulk_data(i2c_dev->bus, i2c_dev->addr, e2p_addr, value, read_len);
    DBG_DEBUG(DBG_VERBOSE, "dfd_ko_other_i2c_dev_read, value[0] = 0x%x\n", value[0]);
    DBG_DEBUG(DBG_VERBOSE, "dfd_ko_other_i2c_dev_read, value[1] = 0x%x\n", value[1]);
    return rv;
}

/**
 * dfd_ko_i2c_read - I2C read operation
 * @bus: I2C BUS
 * @addr: I2C Device address
 * @offset:Register offset
 * @buf:Read buffer
 * @size:Read length
 * @sysfs_name:sysfs attribute name
 * @returns: <0 Failure, other success
 */
int32_t dfd_ko_i2c_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size, const char *sysfs_name)
{
    int rv;
    char sysfs_path[DFD_SYSFS_PATH_MAX_LEN];

    if (buf == NULL) {
        DBG_DEBUG(DBG_ERROR, "params error, buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }

    if (sysfs_name == NULL) {   /* Read in i2c mode */
        DBG_DEBUG(DBG_VERBOSE, "using i2c_smbus_xfer, bus:%d, addr:0x%x, offset:0x%x, read size:%d.\n",
            bus, addr, offset, size);
        rv = dfd_ko_i2c_read_tmp(bus, addr, offset, buf, size);
    } else {    /* Read by sysfs */
        mem_clear(sysfs_path, sizeof(sysfs_path));
        snprintf(sysfs_path, sizeof(sysfs_path), "/sys/bus/i2c/devices/%d-%04x/%s",
            bus, addr, sysfs_name);
        DBG_DEBUG(DBG_VERBOSE, "using sysfs, sysfs_path:%s, offset:0x%x, read size:%d.\n",
            sysfs_path, offset, size);
        rv = dfd_ko_read_file(sysfs_path, offset, buf, size);
    }

    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "dfd_ko_i2c_read failed.\n");
    } else {
        DBG_DEBUG(DBG_VERBOSE, "dfd_ko_i2c_read success.\n");
    }

    return rv;
}

/**
 * dfd_ko_write_file - file WRITE OPERATION
 * @fpath: file path
 * @addr: address
 * @val: data
 * @write_bytes: length
 *
 * @returns: <0 Failure, other success
 */
int32_t dfd_ko_write_file(char *fpath, int32_t addr, uint8_t *val, int32_t write_bytes)
{
    int32_t ret;
    struct file *filp;
    loff_t pos;

    struct kvec iov = {
        .iov_base = val,
        .iov_len = min_t(size_t, write_bytes, MAX_RW_COUNT),
    };
    struct iov_iter iter;

    if ((fpath == NULL) || (val == NULL) || (addr < 0) || (write_bytes <= 0)) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, addr=%d write_bytes=%d\n", addr, write_bytes);
        return -DFD_RV_INDEX_INVALID;
    }

     /* Open file */
    filp = filp_open(fpath, O_RDWR, 777);
    if (IS_ERR(filp)) {
        DBG_DEBUG(DBG_ERROR, "open file[%s] fail\n", fpath);
        return -DFD_RV_DEV_FAIL;
    }
    /* Location file */
    pos = addr;
    iov_iter_kvec(&iter, ITER_SOURCE, &iov, 1, iov.iov_len);
    ret = vfs_iter_write(filp, &iter, &pos, 0);
    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR,"vfs_iter_write failed, path=%s, addr=%d, size=%d, ret=%d\n", fpath, addr, write_bytes, ret);
        ret = -DFD_RV_DEV_FAIL;
    }
    vfs_fsync(filp, 1);
    filp_close(filp, NULL);
    return ret;
}
