/*
 * Copyright(C) 2013 whitebox Network. All rights reserved.
 */
/*
 * dfd_cfg_info.c
 * Original Author:  sonic_rd@whitebox 2020-02-17
 *
 * dfd database data information extraction module, the data into int, buf two types
 *
 */

#include <linux/string.h>
#include <linux/types.h>
#include <linux/slab.h>

#include "wb_module.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_file.h"

/* CPLD_VOLATGE_VALUE_MODE1 */
/* high 8 bit + high 4 bit(bit4-bit7) */
#define DFD_GET_CPLD_VOLATGE_CODE_VALUE(value)        ((value >> 4)& 0xfff)
#define DFD_GET_CPLD_VOLATGE_REAL_VALUE(code_val, k)  ((code_val * 16 * 33 * k) / ((65536 - 5000) * 10))

/* CPLD_VOLATGE_VALUE_MODE2 */
/* high 8 bit + low 4 bit(bit0-bit3) */
#define DFD_GET_CPLD_VOLATGE_CODE_VALUE2(value)        (((value & 0xff00) >> 4) + (value & 0xf))
#define DFD_GET_CPLD_VOLATGE_REAL_VALUE2(code_val, k)  ((code_val * 33 * k) / 40950)

typedef enum cpld_volatge_value_s {
    CPLD_VOLATGE_VALUE_MODE1 = 0,
    CPLD_VOLATGE_VALUE_MODE2 = 1,
} cpld_volatge_value_t;

#define VALID_MAC_TEMP_MAX      (120)
#define VALID_MAC_TEMP_MIN      (-40)
#define MAC_TEMP_INVALID        (-99999999)

/* info_ctrl_t member string */
char *g_info_ctrl_mem_str[INFO_CTRL_MEM_END] = {
    ".mode",
    ".int_cons",
    ".src",
    ".frmt",
    ".pola",
    ".val_pola",
    ".fpath",
    ".addr",
    ".len",
    ".val_len",
    ".bit_offset",
    ".str_cons",
    ".val_type",
    ".int_extra1",
    ".int_extra2",
    ".int_extra3",
};

/* info_ctrl_mode_t enumeration string */
char *g_info_ctrl_mode_str[INFO_CTRL_MODE_END] = {
    "none",
    "config",
    "constant",
    "tlv",
    "str_constant",
};

/* info_src_t enumeration string */
char *g_info_src_str[INFO_SRC_END] = {
    "none",
    "cpld",
    "fpga",
    "other_i2c",
    "file",
    "logic_file",
};

/* info_frmt_t enumeration string */
char *g_info_frmt_str[INFO_FRMT_END] = {
    "none",
    "bit",
    "byte",
    "num_bytes",
    "num_str",
    "num_buf",
    "buf",
};

/* info_pola_t enumeration string */
char *g_info_pola_str[INFO_POLA_END] = {
    "none",
    "positive",
    "negative",
};

/* info_val_type_t enumeration string */
char *g_info_val_type_str[INFO_VAL_TYPE_END] = {
    "normal",
    "fixed_key",
    "nega_key",
};

/* Read information from the cpld */
static int dfd_read_info_from_cpld(int32_t addr, int read_bytes, uint8_t *val)
{
    int i, rv;

    for (i = 0; i < read_bytes; i++) {
        rv = dfd_ko_cpld_read(addr, &(val[i]));
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "read info[addr=0x%x read_bytes=%d] from cpld fail, reading_byte=%d rv=%d\n",
                addr, read_bytes, i, rv);
            return rv;
        }
        addr++;
    }

    return read_bytes;
}

/* Write information to the cpld */
static int dfd_write_info_to_cpld(int32_t addr, int write_bytes, uint8_t *val)
{
    int i, rv;

    for (i = 0; i < write_bytes; i++) {
        rv = dfd_ko_cpld_write(addr, val[i]);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "write info[addr=0x%x val=0x%x] to cpld fail, rv=%d\n", addr, val[i], rv);
            return rv;
        }
        DBG_DEBUG(DBG_VERBOSE, "write info[addr=0x%x val=0x%x] success\n", addr, val[i]);
        addr++;
    }

    return write_bytes;
}

/* Read information from other_i2c */
static int dfd_read_info_from_other_i2c(int32_t addr, int read_bytes, uint8_t *val)
{
    int rv;

    rv = dfd_ko_other_i2c_dev_read(addr, val, read_bytes);
    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "read info[addr=0x%x read_bytes=%d] from othre i2c fail, rv=%d\r\n",
            addr, read_bytes, rv);
        return rv;
    }

    return read_bytes;
}

/* Read information */
static int dfd_read_info(info_src_t src, char *fpath, int32_t addr, int read_bytes, uint8_t *val)
{
    int rv = 0;

    /* Read data from different sources */
    switch (src) {
    case INFO_SRC_CPLD:
        rv = dfd_read_info_from_cpld(addr, read_bytes, val);
        break;
    case INFO_SRC_FPGA:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "not support read info from fpga\n");
        break;
    case INFO_SRC_OTHER_I2C:
        rv = dfd_read_info_from_other_i2c(addr, read_bytes, val);
        break;
    case INFO_SRC_LOGIC_FILE:
    case INFO_SRC_FILE:
        rv = dfd_ko_read_file(fpath, addr, val, read_bytes);
        break;
    default:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "info src[%d] error\n", src);
        break;
    }

    return rv;
}

/* Write message */
static int dfd_write_info(info_src_t src, char *fpath, int32_t addr, int write_bytes, uint8_t *val)
{
    int rv = 0;

    /* Write data to separate sources */
    switch (src) {
    case INFO_SRC_CPLD:
        rv = dfd_write_info_to_cpld(addr, write_bytes, val);
        break;
    case INFO_SRC_FPGA:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "not support write info to fpga\n");
        break;
    case INFO_SRC_OTHER_I2C:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "not support write info to other i2c\n");
        break;
    case INFO_SRC_LOGIC_FILE:
    case INFO_SRC_FILE:
        rv = dfd_ko_write_file(fpath, addr, val, write_bytes);
        break;
    default:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "info src[%d] error\n", src);
        break;
    }

    return rv;
}

static void dfd_val_type_handle(info_ctrl_t *info_ctrl, uint8_t *val, uint8_t is_write)
{
    uint8_t ori_val;

    if (info_ctrl == NULL || val == NULL) {
        DBG_DEBUG(DBG_ERROR, "input arguments error.\n");
        return;
    }

    if (!is_write) {
        switch (info_ctrl->val_type) {
        case INFO_VAL_TYPE_NORMAL:
            /* Normal value does not need to be processed */
            DBG_DEBUG(DBG_VERBOSE, "info ctrl val_type[%d]: Read normal value[0x%x] "
                "does not need to be processed.\n", info_ctrl->val_type, *val);
            break;
        case INFO_VAL_TYPE_FIXED_KEY:
        case INFO_VAL_TYPE_NEGA_KEY:
            /* take valid bit0 */
            ori_val = *val;
            *val =  ori_val & 0x01;
            DBG_DEBUG(DBG_VERBOSE, "info ctrl val_type[%d]: Read value[0x%x] take valid bit0 is %d.\n",
                info_ctrl->val_type, ori_val, *val);
            break;
        default:
            DBG_DEBUG(DBG_ERROR, "info ctrl val_type[%d] invalid.\n", info_ctrl->val_type);
            break;
        }
    } else {
        switch (info_ctrl->val_type) {
        case INFO_VAL_TYPE_NORMAL:
            /* Normal value does not need to be processed */
            DBG_DEBUG(DBG_VERBOSE, "info ctrl val_type[%d]: Write normal value[0x%x] "
                "does not need to be processed.\n", info_ctrl->val_type, *val);
            break;
        case INFO_VAL_TYPE_FIXED_KEY:
            /* The fixed key is already included in the configuration value and does not need to be processed */
            DBG_DEBUG(DBG_VERBOSE, "info ctrl val_type[%d]: Write fixed key value[0x%x] "
                "does not need to be processed.\n", info_ctrl->val_type, *val);
            break;
        case INFO_VAL_TYPE_NEGA_KEY:
            /* bit[1-7]: Reverse the high 7 bits of the register address, Bit0: a valid bit */
            ori_val = *val;
            *val = (~info_ctrl->addr & 0xfe) | (ori_val & 0x01);
            DBG_DEBUG(DBG_VERBOSE, "info ctrl val_type[%d]: Write negative key value[0x%x] "
                "calculated from addr[0x%x] and value[0x%x].\n",
                info_ctrl->val_type, *val, info_ctrl->addr, ori_val);
            break;
        default:
            DBG_DEBUG(DBG_ERROR, "info ctrl val_type[%d] invalid.\n", info_ctrl->val_type);
            break;
        }
    }

    return;
}

static int dfd_get_info_value(info_ctrl_t *info_ctrl, int *ret, info_num_buf_to_value_f pfun)
{
    int i, rv;
    int read_bytes, readed_bytes, int_tmp, val_tmp;
    uint8_t val[INFO_INT_MAX_LEN + 1] = {0};
    int32_t bit_offset, bit_len;

    if (info_ctrl->mode == INFO_CTRL_MODE_CONS) {
        *ret = info_ctrl->int_cons;
        return DFD_RV_OK;
    }
    if (info_ctrl->mode == INFO_CTRL_MODE_TLV) {
        return INFO_CTRL_MODE_TLV;
    }

    if (IS_INFO_FRMT_BIT(info_ctrl->frmt)) {
        bit_offset = info_ctrl->bit_offset;
        bit_len = info_ctrl->len;   /* info_ctrl->len is read bit len */
        if (!INFO_BIT_OFFSET_VALID(bit_offset)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl bit_offsest[%d] invalid\n", bit_offset);
            return -DFD_RV_TYPE_ERR;
        }
        /* info_ctrl->val_len is multi-byte read data len */
        if (info_ctrl->val_len <= 0) {
            read_bytes = 1;
        } else {
            read_bytes = info_ctrl->val_len;
        }
        if (read_bytes > INFO_INT_RW_MAX_LEN) {
            DBG_DEBUG(DBG_ERROR, "info ctrl multi-byte read data len[%d] exceed max len: %d\n",
                read_bytes, INFO_INT_RW_MAX_LEN);
            return -DFD_RV_TYPE_ERR;
        }

        if ((bit_len <= 0) || ((bit_offset + bit_len) > read_bytes * 8)) {
            DBG_DEBUG(DBG_ERROR, "Invalid info ctrl bit_offsest[%d] and bit_len[%d] exceed read_bytes[%d]\n",
                bit_offset, bit_len, read_bytes);
            return -DFD_RV_TYPE_ERR;
        }
        DBG_DEBUG(DBG_VERBOSE, "info ctrl bit_offsest[%d], bit_len[%d] read_bytes[%d]\n",
            bit_offset, bit_len, read_bytes);
    } else if (IS_INFO_FRMT_NUM_STR(info_ctrl->frmt)) {
        read_bytes = info_ctrl->len;
        if (!INFO_INT_LEN_VALAID(read_bytes)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl read num_str len[%d] invalid\n", read_bytes);
            return -DFD_RV_TYPE_ERR;
        }
    } else if (IS_INFO_FRMT_BYTE(info_ctrl->frmt) || IS_INFO_FRMT_NUM_BUF(info_ctrl->frmt)) {
        read_bytes = info_ctrl->len;
        if ((read_bytes <= 0) || (read_bytes > INFO_INT_RW_MAX_LEN)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl read byte/num_bytes/num_buf len[%d] exceed max len: %d\n",
                read_bytes, INFO_INT_RW_MAX_LEN);
            return -DFD_RV_TYPE_ERR;
        }
    } else {
        DBG_DEBUG(DBG_ERROR, "info ctrl info format[%d] error\n", info_ctrl->frmt);
        return -DFD_RV_TYPE_ERR;
    }

    readed_bytes = dfd_read_info(info_ctrl->src, info_ctrl->fpath, info_ctrl->addr, read_bytes, &(val[0]));
    if (readed_bytes <= 0) {
        DBG_DEBUG(DBG_ERROR, "read int info[src=%s frmt=%s fpath=%s addr=0x%x read_bytes=%d] fail, rv=%d\n",
            g_info_src_str[info_ctrl->src], g_info_frmt_str[info_ctrl->frmt], info_ctrl->fpath,
            info_ctrl->addr, read_bytes, readed_bytes);
        return -DFD_RV_DEV_FAIL;
    }

    int_tmp = 0;
    if (IS_INFO_FRMT_BIT(info_ctrl->frmt)) {
        if (readed_bytes != read_bytes) {
            DBG_DEBUG(DBG_ERROR, "info ctrl multi-byte read failed, read_bytes: %d, readed_bytes: %d",
                read_bytes, readed_bytes);
            return -DFD_RV_DEV_FAIL;
        }
        val_tmp = 0;
        /* val_pola negative means little endian */
        if (info_ctrl->val_pola == INFO_POLA_NEGA) {
            for (i = 0; i < readed_bytes; i++) {
                val_tmp |= (val[i] << (i * 8));
            }
        } else {    /* val_pola positive means big endian */
            for (i = 0; i < readed_bytes; i++) {
                val_tmp |= (val[i] << ((readed_bytes -i -1) * 8));
            }
        }
        DBG_DEBUG(DBG_VERBOSE, "info ctrl multi-byte read , val_pola: %s, val_tmp: 0x%x",
            g_info_pola_str[info_ctrl->val_pola], val_tmp);

        /* pola negative means negation */
        if (info_ctrl->pola == INFO_POLA_NEGA) {
            val_tmp = ~val_tmp;
        }
        DBG_DEBUG(DBG_VERBOSE, "info ctrl multi-byte read , bit_pola: %s, val_tmp: 0x%x",
            g_info_pola_str[info_ctrl->pola], val_tmp);
        val_tmp = (val_tmp >> bit_offset) & (GENMASK(bit_len - 1, 0));
        if (pfun) {
            rv = pfun((uint8_t *)&val_tmp, sizeof(val_tmp), &int_tmp);
            if (rv < 0) {
                DBG_DEBUG(DBG_ERROR, "info ctrl bit process fail, rv=%d\n", rv);
                return rv;
            }
        } else {
            int_tmp = val_tmp;
        }
    } else if (IS_INFO_FRMT_BYTE(info_ctrl->frmt)) {
        if (readed_bytes != read_bytes) {
            DBG_DEBUG(DBG_ERROR, "info ctrl read byte/num_bytes failed, read_bytes: %d, readed_bytes: %d",
                read_bytes, readed_bytes);
            return -DFD_RV_DEV_FAIL;
        }
        int_tmp = 0;
        for (i = 0; i < info_ctrl->len; i++) {
            if (info_ctrl->pola == INFO_POLA_NEGA) {
                int_tmp |= val[info_ctrl->len - i - 1];
            } else {
                int_tmp |= val[i];
            }
            if (i != (info_ctrl->len - 1)) {
                int_tmp <<= 8;
            }
        }

        if (info_ctrl->frmt == INFO_FRMT_BYTE) {
            dfd_val_type_handle(info_ctrl, (uint8_t *)&int_tmp, 0);
        }
    } else if (IS_INFO_FRMT_NUM_STR(info_ctrl->frmt)) {
        val[readed_bytes] = '\0';
        int_tmp = 0;
        rv = kstrtoint((char *)(&(val[0])), 0, &int_tmp);
        if (rv) {
            DBG_DEBUG(DBG_ERROR, "Failed to convert string: %s to int type data, rv: %d\n",
                (char *)(&(val[0])), rv);
            return -DFD_RV_INVALID_VALUE;
        }
    } else {
        if (pfun == NULL) {
            DBG_DEBUG(DBG_ERROR, "info ctrl number buf process function is null\n");
            return -DFD_RV_INDEX_INVALID;
        }
        rv = pfun(val, readed_bytes, &int_tmp);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "info ctrl number buf process fail, rv=%d\n", rv);
            return rv;
        }
    }

    *ret = int_tmp;
    DBG_DEBUG(DBG_VERBOSE, "read int info[src=%s frmt=%s pola=%s fpath=%s addr=0x%x len=%d bit_offset=%d] success, ret=%d\n",
            g_info_src_str[info_ctrl->src], g_info_frmt_str[info_ctrl->frmt], g_info_pola_str[info_ctrl->pola],
            info_ctrl->fpath, info_ctrl->addr, info_ctrl->len, info_ctrl->bit_offset, *ret);
    return DFD_RV_OK;
}

/**
 * dfd_info_get_int - Get int type information
 * @key: Search keyword of the configuration item
 * @ret: int type information
 * @pfun: num buf type data conversion function
 *
 * @returns: 0 Success, <0 failure
 */
int dfd_info_get_int(uint64_t key, int *ret, info_num_buf_to_value_f pfun)
{
    int rv;
    info_ctrl_t *info_ctrl;

    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key)) || (ret == NULL)) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, key=0x%08llx\n", key);
        return -DFD_RV_INDEX_INVALID;
    }

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_DEBUG(DBG_VERBOSE, "get info ctrl value, key=0x%08llx\n", key);
    rv = dfd_get_info_value(info_ctrl, ret, pfun);
    return rv;
}

/**
 * dfd_info_get_buf - Get buf type information
 * @key: Search keyword of the configuration item
 * @buf: information buf
 * @buf_len: buf length, which must be no less than info_ctrl->len
 * @pfun: Data conversion function pointer
 *
 * @returns: <0 Success, <0 failure
 */
int dfd_info_get_buf(uint64_t key, uint8_t *buf, int buf_len, info_buf_to_buf_f pfun)
{
    int rv;
    int read_bytes, buf_real_len;
    uint8_t buf_tmp[INFO_BUF_MAX_LEN];
    info_ctrl_t *info_ctrl;

    /* Entry check */
    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key)) || (buf == NULL)) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, key=0x%08llx\n", key);
        return -DFD_RV_INDEX_INVALID;
    }

    /* Get the configuration item read and write control variables */
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    /* Failed to return the non-configured mode */
    if (info_ctrl->mode != INFO_CTRL_MODE_CFG) {
        DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08llx] mode[%d] invalid\n", key, info_ctrl->mode);
        return -DFD_RV_TYPE_ERR;
    }

    /* Parameter check */
    if (!IS_INFO_FRMT_BUF(info_ctrl->frmt) || !INFO_BUF_LEN_VALAID(info_ctrl->len)
            || (buf_len <= info_ctrl->len)) {
        DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08llx] format=%d or len=%d invlaid, buf_len=%d\n",
            key, info_ctrl->frmt, info_ctrl->len, buf_len);
        return -DFD_RV_TYPE_ERR;
    }

    /* Read information */
    read_bytes = dfd_read_info(info_ctrl->src, info_ctrl->fpath, info_ctrl->addr, info_ctrl->len, buf_tmp);
    if (read_bytes <= 0) {
        DBG_DEBUG(DBG_ERROR, "read buf info[key=0x%08llx src=%s frmt=%s fpath=%s addr=0x%x len=%d] fail, rv=%d\n",
            key, g_info_src_str[info_ctrl->src], g_info_frmt_str[info_ctrl->frmt], info_ctrl->fpath,
            info_ctrl->addr, info_ctrl->len, read_bytes);
        return -DFD_RV_DEV_FAIL;
    }

    /* Data conversion processing */
    if (pfun) {
        buf_real_len = buf_len;
        rv = pfun(buf_tmp, read_bytes, buf, &buf_real_len);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08llx] buf process fail, rv=%d\n", key, rv);
            return -DFD_RV_DEV_FAIL;
        }
    } else {
        buf_real_len = read_bytes;
        memcpy(buf, buf_tmp, read_bytes);
    }

    return buf_real_len;
}

/**
 * dfd_2key_info_get_buf - Get buf type information
 * @key: indicates the search keyword of the configuration item
 * @buf: Message buf
 * @buf_len: indicates the buf length, which must be no less than info_ctrl->len
 * @pfun: Data conversion function pointer
 *
 * @returns: <0 fails, others succeed
 */
static int dfd_2key_info_get_buf(info_ctrl_t *info_ctrl, uint8_t *buf, int buf_len, info_hwmon_buf_f pfun)
{
    int rv;
    int read_bytes, buf_real_len;
    uint8_t buf_tmp[INFO_BUF_MAX_LEN];
    char fpath[INFO_FPATH_MAX_LEN];
    int coefficient, addend;

    /* Parameter check */
    if (!IS_INFO_FRMT_BUF(info_ctrl->frmt) || !INFO_BUF_LEN_VALAID(info_ctrl->len)
            || (buf_len <= info_ctrl->len)) {
        DBG_DEBUG(DBG_ERROR, "key_path info ctrl format=%d or len=%d invlaid, buf_len=%d\n",
            info_ctrl->frmt, info_ctrl->len, buf_len);
        return -DFD_RV_TYPE_ERR;
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));
    rv = kfile_iterate_dir(info_ctrl->fpath, DFD_HWMON_NAME, buf_tmp, INFO_BUF_MAX_LEN);
    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "dir patch:%s, can find name %s dir \n",
            info_ctrl->fpath, DFD_HWMON_NAME);
        return -DFD_RV_NO_NODE;
    }
    mem_clear(fpath, sizeof(fpath));
    snprintf(fpath, sizeof(fpath), "%s%s/%s",
        info_ctrl->fpath, buf_tmp, info_ctrl->str_cons);
    DBG_DEBUG(DBG_VERBOSE, "match ok path: %s\n", fpath);

    mem_clear(buf_tmp, sizeof(buf_tmp));
    /* Read information */
    read_bytes = dfd_read_info(info_ctrl->src, fpath, info_ctrl->addr, info_ctrl->len, buf_tmp);
    if (read_bytes <= 0) {
        DBG_DEBUG(DBG_ERROR, "read buf info[src: %s frmt: %s fpath: %s addr: 0x%x len: %d] fail, rv=%d\n",
            g_info_src_str[info_ctrl->src], g_info_src_str[info_ctrl->frmt], fpath,
            info_ctrl->addr, info_ctrl->len, read_bytes);
        return -DFD_RV_DEV_FAIL;
    }

    /* Data conversion processing */
    if (pfun) {
        buf_real_len = buf_len;
        coefficient = info_ctrl->int_extra1;
        addend = info_ctrl->int_extra2;
        if (coefficient != 0) {
            rv = pfun(buf_tmp, read_bytes, buf, &buf_real_len, info_ctrl, coefficient, addend);
        } else {
            rv = pfun(buf_tmp, read_bytes, buf, &buf_real_len, info_ctrl, 1, addend);
        }
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "info ctrl buf process fail, rv=%d\n", rv);
            return -DFD_RV_DEV_FAIL;
        }
    } else {
        buf_real_len = read_bytes;
        memcpy(buf, buf_tmp, buf_real_len);
    }
    return buf_real_len;
}

/**
 * dfd_info_set_int - Set the int type information
 * @key: Search keyword of the configuration item
 * @val: int type information
 *
 * @returns: 0 succeeds, <0 fails
 */
int dfd_info_set_int(uint64_t key, int val)
{
    int rv, i;
    int write_bytes, wr_val_tmp, rd_val_tmp;
    uint32_t bit_mask, bit_mask_tmp;
    info_ctrl_t *info_ctrl;
    uint8_t *val_buf;
    uint8_t wr_buf[INFO_INT_RW_MAX_LEN];
    uint8_t rd_buf[INFO_INT_RW_MAX_LEN];
    int32_t bit_offset, bit_len;

    mem_clear(wr_buf, sizeof(wr_buf));
    mem_clear(rd_buf, sizeof(rd_buf));
    val_buf = &wr_buf[0];
    /* Entry check */
    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key))) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, key=0x%08llx\n", key);
        return -DFD_RV_INDEX_INVALID;
    }

    /* Get the configuration item read and write control variables */
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    /* Non-configuration is not processed */
    if (info_ctrl->mode != INFO_CTRL_MODE_CFG) {
        DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08llx] mode[%d] warnning\n", key, info_ctrl->mode);
        return -DFD_RV_TYPE_ERR;
    }

    /* Information conversion */
    if (IS_INFO_FRMT_BIT(info_ctrl->frmt)) {
        /* Bit offset check */
        if (!INFO_BIT_OFFSET_VALID(info_ctrl->bit_offset)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08llx] bit_offsest[%d] invalid\n",
                key, info_ctrl->bit_offset);
            return -DFD_RV_TYPE_ERR;
        }

        /* info_ctrl->val_len is multi-byte write data len */
        if (info_ctrl->val_len <= 0) {
            write_bytes = 1;
        } else {
            write_bytes = info_ctrl->val_len;
        }
        if (write_bytes > sizeof(wr_buf)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl multi-byte write data len[%d] exceed max len: %d\n",
                write_bytes, INFO_INT_RW_MAX_LEN);
            return -DFD_RV_TYPE_ERR;
        }
        bit_len = info_ctrl->len;   /* info_ctrl->len is write bit len */
        bit_offset = info_ctrl->bit_offset;
        if ((bit_len <= 0) || ((bit_offset + bit_len) > write_bytes * 8)) {
            DBG_DEBUG(DBG_ERROR, "Invalid info ctrl bit_offsest[%d] and bit_len[%d] exceed write_bytes[%d]\n",
                bit_offset, bit_len, write_bytes);
            return -DFD_RV_TYPE_ERR;
        }
        DBG_DEBUG(DBG_VERBOSE, "info ctrl write_bytes: %d, bit_offsest[%d] bit_len[%d] origin write val: 0x%x\n",
            write_bytes, bit_offset, bit_len, val);

        /* Write process */
        /* 1. Left shift the value by bit_offset
         * 2. Based on info_ctrl->pola, decide whether to negate the value (val) before writing
         * 3. Generate the bit_mask (GENMASK(bit_offset + bit_len - 1, bit_offset))
         * 4. Generate bit_mask_tmp (GENMASK(write_bytes * 8 - 1, 0))
         * 5. If bit_mask == bit_mask_tmp, it's a full byte write, no need for a read-modify-write operation, otherwise perform read-modify-write
         * 6. First support the read operation, convert it to the corresponding integer value based on info_ctrl->val_pola
         * 7. Use the bit_mask and shifted value to get the final value to be written
         * 8. Based on info_ctrl->val_pola, get the write buffer for the value to be written
         */
        wr_val_tmp = (val << bit_offset);         /* The value is shifted to the corresponding bit */
        if (info_ctrl->pola == INFO_POLA_NEGA) {    /* Negative polarity data is reversed */
            wr_val_tmp = ~wr_val_tmp;
        }
        bit_mask = GENMASK(bit_offset + bit_len - 1, bit_offset);
        bit_mask_tmp = GENMASK((write_bytes * 8) -1, 0);
        DBG_DEBUG(DBG_VERBOSE, "info ctrl wr_val_tmp: 0x%x, bit_mask: 0x%08x, bit_mask_tmp: 0x%08x\n",
            wr_val_tmp, bit_mask, bit_mask_tmp);

        if (bit_mask != bit_mask_tmp) {
            rv = dfd_read_info(info_ctrl->src, info_ctrl->fpath, info_ctrl->addr, write_bytes, rd_buf);
            if (rv != write_bytes) {
                DBG_DEBUG(DBG_ERROR,
                    "read original info[src=%d][fpath=%s][addr=0x%x] fail. read len = %d, rv = %d\n",
                    info_ctrl->src, info_ctrl->fpath, info_ctrl->addr, write_bytes, rv);
                return -DFD_RV_DEV_FAIL;
            }
            rd_val_tmp = 0;
            /* val_pola negative means little endian */
            if (info_ctrl->val_pola == INFO_POLA_NEGA) {
                for (i = 0; i < write_bytes; i++) {
                    rd_val_tmp |= (rd_buf[i] << (i * 8));
                }
            } else {    /* val_pola positive means big endian */
                for (i = 0; i < write_bytes; i++) {
                    rd_val_tmp |= (rd_buf[i] << ((write_bytes -i -1) * 8));
                }
            }
            wr_val_tmp = (rd_val_tmp & (~bit_mask)) | (wr_val_tmp & bit_mask);
            DBG_DEBUG(DBG_VERBOSE, "info ctrl read before write, read val: 0x%x, bit_mask: 0x%08x, write val: 0x%x\n",
                rd_val_tmp, bit_mask, wr_val_tmp);
        }

        /* val_pola negative means little endian */
        if (info_ctrl->val_pola == INFO_POLA_NEGA) {
            for (i = 0; i < write_bytes; i++) {
                wr_buf[i] = (wr_val_tmp >> (i * 8)) & 0xff;
            }
        } else {    /* big endian mode */
            for (i = 0; i < write_bytes; i++) {
                wr_buf[i] = (wr_val_tmp >> ((write_bytes - i - 1) * 8)) & 0xff;
            }
        }
    } else if (IS_INFO_FRMT_BYTE(info_ctrl->frmt) || IS_INFO_FRMT_NUM_BUF(info_ctrl->frmt)) {
        /* Length check */
        if (info_ctrl->len > sizeof(wr_buf)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08llx] len[%d] invalid\n", key, info_ctrl->len);
            return -DFD_RV_TYPE_ERR;
        }

        write_bytes = info_ctrl->len;
        if (info_ctrl->pola == INFO_POLA_NEGA) {   /* little endian mode */
            for (i = 0; i < write_bytes; i++) {
                wr_buf[i] = (val >> (i * 8)) & 0xff;
            }
        } else {
            for (i = 0; i < write_bytes; i++) {
                wr_buf[i] = (val >> ((write_bytes - i - 1) * 8)) & 0xff;
            }
        }

        if (info_ctrl->frmt == INFO_FRMT_BYTE) {
            dfd_val_type_handle(info_ctrl, &wr_buf[0], 1);
        }
    } else if (IS_INFO_FRMT_NUM_STR(info_ctrl->frmt)) {
        val_buf = info_ctrl->str_cons;
        write_bytes = strlen(info_ctrl->str_cons);
        if (write_bytes <= 0) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08llx] write num_str: fpath: %s, len[%d] invalid\n",
                key, info_ctrl->fpath, write_bytes);
            return -DFD_RV_INVALID_VALUE;
        }
        DBG_DEBUG(DBG_VERBOSE, "info ctrl[key=0x%08llx], write num_str: fpath: %s, write val: %s, len: %d\n",
            key, info_ctrl->fpath, val_buf, write_bytes);
    } else {
        DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08llx] format[%d] error\n", key, info_ctrl->frmt);
        return -DFD_RV_TYPE_ERR;
    }

    /* Write message */
    rv = dfd_write_info(info_ctrl->src, info_ctrl->fpath, info_ctrl->addr, write_bytes, val_buf);
    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "write int info[src=%s frmt=%s fpath=%s addr=0x%x len=%d val=%d] fail, rv=%d\n",
            g_info_src_str[info_ctrl->src], g_info_frmt_str[info_ctrl->frmt], info_ctrl->fpath,
            info_ctrl->addr, info_ctrl->len, val, rv);
        return -DFD_RV_DEV_FAIL;
    }

    DBG_DEBUG(DBG_VERBOSE, "write int info[src=%s frmt=%s pola=%s fpath=%s addr=0x%x len=%d bit_offset=%d val=%d] success\n",
            g_info_src_str[info_ctrl->src], g_info_frmt_str[info_ctrl->frmt], g_info_pola_str[info_ctrl->pola],
            info_ctrl->fpath, info_ctrl->addr, info_ctrl->len, info_ctrl->bit_offset, val);
    return DFD_RV_OK;
}

static long dfd_info_reg2data_linear(uint64_t key, int data)
{
    s16 exponent;
    s32 mantissa;
    long val;
    info_ctrl_t *info_ctrl;

    exponent = 0;
    val = 0;
    /* Get the configuration item read and write control variables */
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    switch (info_ctrl->int_extra1) {
    case LINEAR11:
        exponent = ((s16)data) >> 11;
        mantissa = ((s16)((data & 0x7ff) << 5)) >> 5;
        val = mantissa;
        val = val * 1000L;
        break;
    case LINEAR16:
        break;
    default:
        break;
    }

    if (exponent >= 0) {
        val <<= exponent;
    } else {
        val >>= -exponent;
    }

    return val;
}

static long dfd_info_reg2data_tmp464(int data)
{
    s16 tmp_val;
    long val;

    DBG_DEBUG(DBG_VERBOSE, "reg2data_tmp464, data=%d\n", data);

    /* Positive number:data/8*0.0625 */
    if (data >= 0) {
        val = data*625/80;
    /* Negative number: The first bit is the sign bit and the rest is inverted +1 */
    } else {
        tmp_val = ~(data & 0x7ff) + 1;
        val = tmp_val*625/80;
    }

    return val;
}

static long dfd_get_mac_temp(int data, int32_t mac_type)
{
    int tmp_val;
    long val;

    DBG_DEBUG(DBG_VERBOSE, "reg2data_raw_val, data=0x%d\n", data);

    tmp_val = data >> 4;

    switch (mac_type) {
    case MAC_TH5:
        val = 476359 - (((tmp_val - 2) * 317704) / 2000);
        break;
    case MAC_TH4:
        val = 356070 - (((tmp_val - 2) * 237340) / 2000);
        break;
    case MAC_TD5:
        val = 504416 - (((tmp_val - 2) * 329300) / 2000);
        break;
    case MAC_TD3:
        val = 434100 - (12500000 / (data * 100 - 1) * 535);
        break;
    case MAC_TH6:
        val = 378850 - (((tmp_val - 2) * 259680) / 2000);
        break;
    default:
        DBG_DEBUG(DBG_VERBOSE, "unsuport mac type, return raw value\n");
        return data;
    }

    DBG_DEBUG(DBG_VERBOSE, "reg2data_to_temp, val=0x%ld\n", val);
    return val;
}

static int dfd_info_get_cpld_voltage(uint64_t key, uint32_t *value)
{
    int rv;
    uint32_t vol_ref_tmp, vol_ref;
    uint32_t vol_curr_tmp, vol_curr;
    info_ctrl_t *info_ctrl;
    info_ctrl_t info_ctrl_tmp;
    uint32_t vol_coefficient;

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    vol_coefficient = (uint32_t)info_ctrl->int_extra2;

    rv = dfd_get_info_value(info_ctrl, &vol_curr_tmp, NULL);
    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "get cpld current voltage error, addr:0x%x, rv = %d\n", info_ctrl->addr, rv);
        return rv;
    }
    if (info_ctrl->int_extra3 == CPLD_VOLATGE_VALUE_MODE2) {
        vol_curr_tmp = DFD_GET_CPLD_VOLATGE_CODE_VALUE2(vol_curr_tmp);
        vol_curr = DFD_GET_CPLD_VOLATGE_REAL_VALUE2(vol_curr_tmp, info_ctrl->int_extra2);
        DBG_DEBUG(DBG_VERBOSE, "vol_curr_tmp = 0x%x, vol_curr = 0x%x, is same.\n", vol_curr_tmp, vol_curr);
    } else {
        vol_curr_tmp = DFD_GET_CPLD_VOLATGE_CODE_VALUE(vol_curr_tmp);
        if (info_ctrl->addr == info_ctrl->int_extra1) {
            vol_curr = DFD_GET_CPLD_VOLATGE_REAL_VALUE(vol_curr_tmp, vol_coefficient);
            DBG_DEBUG(DBG_VERBOSE, "current voltage is reference voltage, vol_curr_tmp: 0x%x, coefficient: %u, vol_curr: %u\n",
                vol_curr_tmp, vol_coefficient, vol_curr);
        } else {
            memcpy(&info_ctrl_tmp, info_ctrl, sizeof(info_ctrl_t));
            info_ctrl_tmp.addr = info_ctrl->int_extra1;
            rv = dfd_get_info_value(&info_ctrl_tmp, &vol_ref_tmp, NULL);
            if (rv < 0) {
                DBG_DEBUG(DBG_ERROR, "get cpld reference voltage error, addr: 0x%x, rv: %d\n", info_ctrl_tmp.addr, rv);
                return rv;
            }
            vol_ref = DFD_GET_CPLD_VOLATGE_CODE_VALUE(vol_ref_tmp);
            DBG_DEBUG(DBG_VERBOSE, "vol_ref_tmp: 0x%x, vol_ref: 0x%x\n", vol_ref_tmp, vol_ref);
            vol_curr = (vol_curr_tmp * vol_coefficient) / vol_ref;
            DBG_DEBUG(DBG_VERBOSE, "vol_curr_tmp: 0x%x, vol_ref: 0x%x, coefficient: %u, vol_curr: %u\n",
                vol_curr_tmp, vol_ref, vol_coefficient, vol_curr);
        }
    }
    *value = vol_curr;
    return DFD_RV_OK;
}

static int dfd_info_get_cpld_temperature(uint64_t key, int *value)
{
    int rv;
    int temp_reg;
    info_ctrl_t *info_ctrl;
    long val;

    /* Get the configuration item read and write control variables */
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    /* Read the temperature value */
    rv = dfd_info_get_int(key, &temp_reg, NULL);
    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "get cpld current temperature error, addr:0x%x, rv =%d\n", info_ctrl->addr, rv);
        return rv;
    }
    DBG_DEBUG(DBG_VERBOSE, "get cpld temp:0x%08x, extra1 0x%x\n", temp_reg, info_ctrl->int_extra1);

    switch (info_ctrl->int_extra1) {
    case LINEAR11:
        val = dfd_info_reg2data_linear(key, temp_reg);
        break;
    case TMP464:
        val = dfd_info_reg2data_tmp464(temp_reg);
        break;
    case MAC_TH5:
    case MAC_TH4:
    case MAC_TD5:
    case MAC_TD3:
    case MAC_TH6:
        val = dfd_get_mac_temp(temp_reg, info_ctrl->int_extra1);
        break;
    default:
        val = temp_reg;
        break;
    }

    if ((val / 1000 < VALID_MAC_TEMP_MIN) || (val / 1000 > VALID_MAC_TEMP_MAX)) {
        DBG_DEBUG(DBG_ERROR, "mac temp invalid, temp = %ld\n", val);
        val = MAC_TEMP_INVALID;
    }
    DBG_DEBUG(DBG_VERBOSE, "calc temp:%ld \n", val);
    *value = val;

    return DFD_RV_OK;
}

static int dfd_info_get_sensor_value(uint64_t key, uint8_t *buf, int buf_len, info_hwmon_buf_f pfun)
{
    int rv, buf_real_len;
    uint32_t value;
    int temp_value;
    uint8_t buf_tmp[INFO_BUF_MAX_LEN];
    info_ctrl_t *info_ctrl;

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_ERROR, "get info ctrl fail, key=0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (DFD_CFG_ITEM_ID(key) == DFD_CFG_ITEM_HWMON_IN 
        && (info_ctrl->src == INFO_SRC_CPLD || info_ctrl->src == INFO_SRC_LOGIC_FILE)) {
        rv = dfd_info_get_cpld_voltage(key, &value);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "get cpld voltage failed.key=0x%08llx, rv:%d\n", key, rv);
            return -DFD_RV_DEV_NOTSUPPORT;
        }
        DBG_DEBUG(DBG_VERBOSE, "get cpld voltage ok, value:%u\n", value);
        mem_clear(buf_tmp, sizeof(buf_tmp));
        snprintf(buf_tmp, sizeof(buf_tmp), "%u\n", value);
        buf_real_len = strlen(buf_tmp);
        if (buf_len <= buf_real_len) {
            DBG_DEBUG(DBG_ERROR, "length not enough.buf_len:%d,need length:%d\n", buf_len, buf_real_len);
            return -DFD_RV_DEV_FAIL;
        }
        if (pfun) {
            buf_real_len = buf_len;
            rv = pfun(buf_tmp, strlen(buf_tmp), buf, &buf_real_len, info_ctrl, 1, 0);
            if (rv < 0) {
                DBG_DEBUG(DBG_ERROR, "deal date error.org value:%s, buf_len:%d, rv=%d\n",
                    buf_tmp, buf_len, rv);
                return -DFD_RV_DEV_NOTSUPPORT;
            }
        } else {
            memcpy(buf, buf_tmp, buf_real_len);
        }
        return buf_real_len;
    } else if (DFD_CFG_ITEM_ID(key) == DFD_CFG_ITEM_HWMON_TEMP 
        && (info_ctrl->src == INFO_SRC_CPLD || info_ctrl->src == INFO_SRC_LOGIC_FILE)) {
        rv = dfd_info_get_cpld_temperature(key, &temp_value);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "get cpld temperature failed.key=0x%08llx, rv:%d\n", key, rv);
            return -DFD_RV_DEV_NOTSUPPORT;
        }
        DBG_DEBUG(DBG_VERBOSE, "get cpld temperature ok, value:%d buf_len %d\n", temp_value, buf_len);
        mem_clear(buf_tmp, sizeof(buf_tmp));
        snprintf(buf_tmp, sizeof(buf_tmp), "%d\n", temp_value);
        buf_real_len = strlen(buf_tmp);
        if (buf_len <= buf_real_len) {
            DBG_DEBUG(DBG_ERROR, "length not enough.buf_len:%d,need length:%d\n", buf_len, buf_real_len);
            return -DFD_RV_DEV_FAIL;
        }
        DBG_DEBUG(DBG_VERBOSE, "buf_real_len %d\n", buf_real_len);
        memcpy(buf, buf_tmp, buf_real_len);
        return buf_real_len;
    }

    DBG_DEBUG(DBG_ERROR, "not support mode. key:0x%08llx\n", key);
    return -DFD_RV_MODE_NOTSUPPORT;
}

/**
 * dfd_info_get_sensor - Get sensors
 * @key: HWMON Configures the key
 * @buf:Result storage
 * @buf_len: buf Length
 *
 * @returns: <0 Failure, other success
 */
int dfd_info_get_sensor(uint64_t key, char *buf, int buf_len, info_hwmon_buf_f pfun)
{
    info_ctrl_t *key_info_ctrl;
    int rv;

    /* Entry check */
    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key)) ||
        (buf == NULL) || buf_len <= 0) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, key: 0x%08llx, buf_len: %d\n",
            key, buf_len);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, buf_len);
    /* Get the configuration item read and write control variables */
    key_info_ctrl = dfd_ko_cfg_get_item(key);
    if (key_info_ctrl == NULL) {
        DBG_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    /* String type */
    if (key_info_ctrl->mode == INFO_CTRL_MODE_SRT_CONS) {
        snprintf(buf, buf_len, "%s\n", key_info_ctrl->str_cons);
        DBG_DEBUG(DBG_VERBOSE, "get sensor value through string config, key: 0x%08llx, value: %s\n", key, buf);
        return strlen(buf);
    }
    /* int constant type */
    if (key_info_ctrl->mode == INFO_CTRL_MODE_CONS) {
        snprintf(buf, buf_len, "%d\n", key_info_ctrl->int_cons);
        DBG_DEBUG(DBG_VERBOSE, "get sensor value through int config, key: 0x%08llx, value: %d\n", key, key_info_ctrl->int_cons);
        return strlen(buf);
    }

    /* Read from the hwmon file */
    if (key_info_ctrl->mode == INFO_CTRL_MODE_CFG && key_info_ctrl->src == INFO_SRC_FILE) {
        if (strstr(key_info_ctrl->fpath, "hwmon") != NULL) {
            DBG_DEBUG(DBG_VERBOSE, "get sensor value through hwmon, key: 0x%08llx\n", key);
            rv = dfd_2key_info_get_buf(key_info_ctrl, buf, buf_len, pfun);
            if (rv < 0) {
                DBG_DEBUG(DBG_VERBOSE, "get sensor value through hwmon failed, key: 0x%08llx, rv: %d\n", key, rv);
            }
            return rv;
        } else {
            DBG_DEBUG(DBG_VERBOSE, "get sensor value, key:0x%08llx\n", key);
            rv = dfd_info_get_buf(key, buf, buf_len, NULL);
            if (rv < 0) {
                DBG_DEBUG(DBG_VERBOSE, "get sensor value failed, key:0x%08llx, rv:%d\n", key, rv);
            }
            return rv;
        }
    }

    /* Read from the cpld, logic file and so on */
    rv = dfd_info_get_sensor_value(key, buf, buf_len, pfun);
    if ( rv < 0) {
        DBG_DEBUG(DBG_ERROR, "get sensor value failed, key: 0x%08llx, rv: %d\n", key, rv);
    }
    return rv;
}

/**
 * @buf:Input and result store
 *
 */
void dfd_info_del_no_print_string(char *buf)
{
    int i, j, len;

    len = strlen(buf);
    /* Culling noncharacter */
    for (i = 0; i < len; i++) {
        if ((buf[i] < 0x21) || (buf[i] > 0x7E)) {
            for (j = i; j < len - 1; j++) {
                buf[j] = buf[j + 1];
            }
            buf[j] = '\0';
            len--;
            i--;
        }
    }
    return;
}
