#include <linux/string.h>
#include <linux/types.h>
#include <linux/slab.h>

#include "../include/dfd_module.h"
#include "../include/dfd_cfg_adapter.h"
#include "../include/dfd_cfg.h"
#include "../include/dfd_cfg_info.h"
#include "../include/dfd_cfg_file.h"
#include "../../dev_sysfs/include/sysfs_common.h"

#define DFD_HWMON_NAME                                "hwmon"
#define DFD_GET_CPLD_VOLATGE_CODE_VALUE(value)        ((value >> 4)& 0xfff)
#define DFD_GET_CPLD_VOLATGE_REAL_VALUE(code_val, k)  ((code_val * 16 * 33 * k) / ((65536 - 5000) * 10))

char *g_info_ctrl_mem_str[INFO_CTRL_MEM_END] = {
    ".mode",
    ".int_cons",
    ".src",
    ".frmt",
    ".pola",
    ".fpath",
    ".addr",
    ".len",
    ".bit_offset",
    ".str_cons",
    ".int_extra1",
    ".int_extra2",
};

char *g_info_ctrl_mode_str[INFO_CTRL_MODE_END] = {
    "none",
    "config",
    "constant",
    "tlv",
    "str_constant",
};

char *g_info_src_str[INFO_SRC_END] = {
    "none",
    "cpld",
    "fpga",
    "other_i2c",
    "file",
};

char *g_info_frmt_str[INFO_FRMT_END] = {
    "none",
    "bit",
    "byte",
    "num_bytes",
    "num_str",
    "num_buf",
    "buf",
};

char *g_info_pola_str[INFO_POLA_END] = {
    "none",
    "positive",
    "negative",
};

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

static int dfd_write_info_to_cpld(int32_t addr, int write_bytes, uint8_t *val, uint8_t bit_mask)
{
    int rv;
    uint8_t val_tmp;

    if (bit_mask != 0xff) {
        rv = dfd_ko_cpld_read(addr, &val_tmp);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "read original info[addr=0x%x] from cpld fail, rv=%d\n", addr, rv);
            return -1;
        }

        val_tmp = (val_tmp & (~bit_mask)) | (val[0] & bit_mask);
    } else {
        val_tmp = val[0];
    }

    rv = dfd_ko_cpld_write(addr, val_tmp);
    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "write info[addr=0x%x val=0x%x] to cpld fail, rv=%d\n", addr, val_tmp, rv);
        return -1;
    }

    return 0;
}

static int dfd_read_info(info_src_t src, char *fpath, int32_t addr, int read_bytes, uint8_t *val)
{
    int rv = 0;

    switch (src) {
    case INFO_SRC_CPLD:
        rv = dfd_read_info_from_cpld(addr, read_bytes, val);
        break;
    case INFO_SRC_FPGA:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "not support read info from fpga\n");
        break;
    case INFO_SRC_OTHER_I2C:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "not support read info from other i2c\n");
        break;
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

static int dfd_write_info(info_src_t src, char *fpath, int32_t addr, int write_bytes, uint8_t *val, uint8_t bit_mask)
{
    int rv = 0;

    switch (src) {
    case INFO_SRC_CPLD:
        rv = dfd_write_info_to_cpld(addr, write_bytes, val, bit_mask);
        break;
    case INFO_SRC_FPGA:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "not support write info to fpga\n");
        break;
    case INFO_SRC_OTHER_I2C:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "not support write info to other i2c\n");
        break;
    case INFO_SRC_FILE:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "not support write info to file\n");
        break;
    default:
        rv = -1;
        DBG_DEBUG(DBG_ERROR, "info src[%d] error\n", src);
        break;
    }

    return rv;
}

int dfd_info_get_int(int key, int *ret, info_num_buf_to_value_f pfun)
{
    int i, rv;
    int read_bytes, readed_bytes, int_tmp;
    uint8_t byte_tmp, val[INFO_INT_MAX_LEN + 1] = {0};
    info_ctrl_t *info_ctrl;

    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key)) || (ret == NULL)) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, key=0x%08x\n", key);
        return -DFD_RV_INDEX_INVALID;
    }

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (info_ctrl->mode == INFO_CTRL_MODE_CONS) {
        *ret = info_ctrl->int_cons;
        return DFD_RV_OK;
    } else if (info_ctrl->mode == INFO_CTRL_MODE_TLV) {
        return INFO_CTRL_MODE_TLV;
    }

    if (IS_INFO_FRMT_BIT(info_ctrl->frmt)) {

        if (!INFO_BIT_OFFSET_VALID(info_ctrl->bit_offset)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] bit_offsest[%d] invalid\n",
                key, info_ctrl->bit_offset);
            return -DFD_RV_TYPE_ERR;
        }

        read_bytes = 1;
    } else if (IS_INFO_FRMT_BYTE(info_ctrl->frmt) || IS_INFO_FRMT_NUM_STR(info_ctrl->frmt)
            || IS_INFO_FRMT_NUM_BUF(info_ctrl->frmt)) {

        if (!INFO_INT_LEN_VALAID(info_ctrl->len)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] len[%d] invalid\n", key, info_ctrl->len);
            return -DFD_RV_TYPE_ERR;
        }
        read_bytes = info_ctrl->len;
    } else {
        DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] info format[%d] error\n", key, info_ctrl->frmt);
        return -DFD_RV_TYPE_ERR;
    }

    readed_bytes = dfd_read_info(info_ctrl->src, info_ctrl->fpath, info_ctrl->addr, read_bytes, &(val[0]));
    if (readed_bytes <= 0) {
        DBG_DEBUG(DBG_ERROR, "read int info[key=0x%08x src=%s frmt=%s fpath=%s addr=0x%x read_bytes=%d] fail, rv=%d\n",
            key, g_info_src_str[info_ctrl->src], g_info_frmt_str[info_ctrl->frmt], info_ctrl->fpath,
            info_ctrl->addr, read_bytes, readed_bytes);
        return -DFD_RV_DEV_FAIL;
    }

    if (IS_INFO_FRMT_BIT(info_ctrl->frmt)) {

        if (info_ctrl->pola == INFO_POLA_NEGA) {
            val[0] = ~val[0];
        }

        byte_tmp = (val[0] >> info_ctrl->bit_offset) & (~(0xff << info_ctrl->len));

        if (pfun) {
            rv = pfun(&byte_tmp, sizeof(byte_tmp), &int_tmp);
            if (rv < 0) {
                DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] bit process fail, rv=%d\n", key, rv);
                return rv;
            }
        } else {
            int_tmp = (int)byte_tmp;
        }
    } else if (IS_INFO_FRMT_BYTE(info_ctrl->frmt)) {

        int_tmp = 0;
        for (i = 0; i < info_ctrl->len; i++) {
            if (info_ctrl->pola == INFO_POLA_NEGA) {
                int_tmp |=  val[info_ctrl->len - i - 1];
            } else {
                int_tmp |=  val[i];
            }

            if (i != (info_ctrl->len - 1)) {
                int_tmp <<= 8;
            }
        }
    } else if (IS_INFO_FRMT_NUM_STR(info_ctrl->frmt)) {

        val[readed_bytes] = '\0';
        int_tmp = simple_strtol((char *)(&(val[0])), NULL, 10);
    } else {
        if (pfun == NULL) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] number buf process function is null\n", key);
            return -DFD_RV_INDEX_INVALID;
        }

        rv = pfun(val, readed_bytes, &int_tmp);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] number buf process fail, rv=%d\n", key, rv);
            return rv;
        }
    }

    *ret = int_tmp;
    DBG_DEBUG(DBG_VERBOSE, "read int info[key=0x%08x src=%s frmt=%s pola=%s fpath=%s addr=0x%x len=%d bit_offset=%d] success, ret=%d\n",
            key, g_info_src_str[info_ctrl->src], g_info_frmt_str[info_ctrl->frmt], g_info_pola_str[info_ctrl->pola],
            info_ctrl->fpath, info_ctrl->addr, info_ctrl->len, info_ctrl->bit_offset, *ret);
    return DFD_RV_OK;
}

int dfd_info_get_buf(int key, uint8_t *buf, int buf_len, info_buf_to_buf_f pfun)
{
    int rv;
    int read_bytes, buf_real_len;
    uint8_t buf_tmp[INFO_BUF_MAX_LEN];
    info_ctrl_t *info_ctrl;

    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key)) || (buf == NULL)) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, key=0x%08x\n", key);
        return -DFD_RV_INDEX_INVALID;
    }

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (info_ctrl->mode != INFO_CTRL_MODE_CFG) {
        DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] mode[%d] invalid\n", key, info_ctrl->mode);
        return -DFD_RV_TYPE_ERR;
    }

    if (!IS_INFO_FRMT_BUF(info_ctrl->frmt) || !INFO_BUF_LEN_VALAID(info_ctrl->len)
            || (buf_len <= info_ctrl->len)) {
        DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] format=%d or len=%d invlaid, buf_len=%d\n",
            key, info_ctrl->frmt, info_ctrl->len, buf_len);
        return -DFD_RV_TYPE_ERR;
    }

    read_bytes = dfd_read_info(info_ctrl->src, info_ctrl->fpath, info_ctrl->addr, info_ctrl->len, buf_tmp);
    if (read_bytes <= 0) {
        DBG_DEBUG(DBG_ERROR, "read buf info[key=0x%08x src=%s frmt=%s fpath=%s addr=0x%x len=%d] fail, rv=%d\n",
            key, g_info_src_str[info_ctrl->src], g_info_frmt_str[info_ctrl->frmt], info_ctrl->fpath,
            info_ctrl->addr, info_ctrl->len, read_bytes);
        return -DFD_RV_DEV_FAIL;
    }

    if (pfun) {
        buf_real_len = buf_len;
        rv = pfun(buf_tmp, read_bytes, buf, &buf_real_len);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] buf process fail, rv=%d\n", key, rv);
            return -DFD_RV_DEV_NOTSUPPORT;
        }
    } else {
        buf_real_len = read_bytes;
        memcpy(buf, buf_tmp, read_bytes);
    }

    return buf_real_len;
}

static int dfd_2key_info_get_buf(info_ctrl_t *info_ctrl, uint8_t *buf, int buf_len, info_hwmon_buf_f pfun)
{
    int rv;
    int read_bytes, buf_real_len;
    uint8_t buf_tmp[INFO_BUF_MAX_LEN];
    char temp_fpath[INFO_FPATH_MAX_LEN];

    if (!IS_INFO_FRMT_BUF(info_ctrl->frmt) || !INFO_BUF_LEN_VALAID(info_ctrl->len)
            || (buf_len <= info_ctrl->len)) {
        DBG_DEBUG(DBG_ERROR, "key_path info ctrl format=%d or len=%d invlaid, buf_len=%d\n",
            info_ctrl->frmt, info_ctrl->len, buf_len);
        return -DFD_RV_TYPE_ERR;
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));
    rv = kfile_iterate_dir(info_ctrl->fpath, DFD_HWMON_NAME, buf_tmp, INFO_BUF_MAX_LEN);
    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "dir patch:%s ,can find name %s dir \n",
            info_ctrl->fpath, DFD_HWMON_NAME);
        return -DFD_RV_NO_NODE;
    }
    mem_clear(temp_fpath, sizeof(temp_fpath));
    snprintf(temp_fpath, sizeof(temp_fpath), "%s%s/%s",
        info_ctrl->fpath, buf_tmp, info_ctrl->str_cons);
    DBG_DEBUG(DBG_VERBOSE, "match ok path = %s \n", temp_fpath);

    mem_clear(buf_tmp, sizeof(buf_tmp));

    read_bytes = dfd_read_info(info_ctrl->src, temp_fpath, info_ctrl->addr, info_ctrl->len, buf_tmp);
    if (read_bytes <= 0) {
        DBG_DEBUG(DBG_ERROR, "read buf info[src=%s frmt=%s fpath=%s addr=0x%x len=%d] fail, rv=%d\n",
            g_info_src_str[info_ctrl->src], g_info_src_str[info_ctrl->frmt], temp_fpath,
            info_ctrl->addr, info_ctrl->len, read_bytes);
        return -DFD_RV_DEV_FAIL;
    }

    if (pfun) {
        buf_real_len = buf_len;
        rv = pfun(buf_tmp, read_bytes, buf, &buf_real_len, info_ctrl);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "info ctrl buf process fail, rv=%d\n", rv);
            return -DFD_RV_DEV_NOTSUPPORT;
        }
    } else {
        buf_real_len = read_bytes;
        memcpy(buf, buf_tmp, buf_real_len);
    }
    return buf_real_len;
}

int dfd_info_set_int(int key, int val)
{
    int rv;
    int write_bytes;
    uint8_t byte_tmp, bit_mask;
    info_ctrl_t *info_ctrl;

    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key))) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, key=0x%08x\n", key);
        return -DFD_RV_INDEX_INVALID;
    }

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (info_ctrl->mode != INFO_CTRL_MODE_CFG) {
        DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] mode[%d] warnning\n", key, info_ctrl->mode);
        return -DFD_RV_TYPE_ERR;
    }

    if (IS_INFO_FRMT_BIT(info_ctrl->frmt)) {

        if (!INFO_BIT_OFFSET_VALID(info_ctrl->bit_offset)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] bit_offsest[%d] invalid\n",
                key, info_ctrl->bit_offset);
            return -DFD_RV_TYPE_ERR;
        }

        write_bytes = 1;

        byte_tmp = (uint8_t)(val & 0xff);
        byte_tmp <<= info_ctrl->bit_offset;
        if (info_ctrl->pola == INFO_POLA_NEGA) {
            byte_tmp = ~byte_tmp;
        }

        bit_mask = (~(0xff << info_ctrl->len)) << info_ctrl->bit_offset;
    } else if (IS_INFO_FRMT_BYTE(info_ctrl->frmt)) {

        if (!INFO_INT_LEN_VALAID(info_ctrl->len)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] len[%d] invalid\n", key, info_ctrl->len);
            return -DFD_RV_TYPE_ERR;
        }

        write_bytes = 1;

        byte_tmp = (uint8_t)(val & 0xff);

        bit_mask = 0xff;
    } else if (IS_INFO_FRMT_NUM_STR(info_ctrl->frmt)) {

        DBG_DEBUG(DBG_ERROR, "not support str int set\n");
        return -1;
    } else if (IS_INFO_FRMT_NUM_BUF(info_ctrl->frmt)) {

        if (!INFO_INT_LEN_VALAID(info_ctrl->len)) {
            DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] len[%d] invalid\n", key, info_ctrl->len);
            return -DFD_RV_TYPE_ERR;
        }

        write_bytes = 1;

        byte_tmp = (uint8_t)(val & 0xff);

        bit_mask = 0xff;
    } else {
        DBG_DEBUG(DBG_ERROR, "info ctrl[key=0x%08x] format[%d] error\n", key, info_ctrl->frmt);
        return -DFD_RV_TYPE_ERR;
    }

    rv = dfd_write_info(info_ctrl->src, info_ctrl->fpath, info_ctrl->addr, write_bytes,
            &byte_tmp, bit_mask);
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

static int dfd_info_get_cpld_voltage(int key, int *value)
{
    int rv, addr_tmp;
    int vol_ref_tmp, vol_ref;
    int vol_curr_tmp, vol_curr;
    info_ctrl_t *info_ctrl;

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_WARN, "get info ctrl fail, key=0x%08x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

     rv = dfd_info_get_int(key, &vol_curr_tmp, NULL);
     if(rv < 0) {
         DBG_DEBUG(DBG_ERROR, "get cpld current voltage error, addr:0x%x, rv =%d\n", info_ctrl->addr, rv);
         return rv;
     }
     vol_curr_tmp = DFD_GET_CPLD_VOLATGE_CODE_VALUE(vol_curr_tmp);
     if(info_ctrl->addr == info_ctrl->int_extra1) {

         vol_curr = DFD_GET_CPLD_VOLATGE_REAL_VALUE(vol_curr_tmp, info_ctrl->int_extra2);
     } else {

         addr_tmp = info_ctrl->addr;
         info_ctrl->addr = info_ctrl->int_extra1;
         rv = dfd_info_get_int(key, &vol_ref_tmp, NULL);
         info_ctrl->addr = addr_tmp;
         if(rv < 0) {
             DBG_DEBUG(DBG_ERROR, "get cpld reference voltage error, addr:0x%x rv:%d\n", info_ctrl->addr, rv);
             return rv;
         }
         vol_ref = DFD_GET_CPLD_VOLATGE_CODE_VALUE(vol_ref_tmp);
         vol_curr = (vol_curr_tmp * info_ctrl->int_extra2) / vol_ref;
     }
    *value = vol_curr;
     return DFD_RV_OK;
}

static int dfd_info_get_sensor_value(int key, uint8_t *buf, int buf_len, info_hwmon_buf_f pfun)
{
    int rv, buf_real_len;
    int value;
    uint8_t buf_tmp[INFO_BUF_MAX_LEN];
    info_ctrl_t *info_ctrl;

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_ERROR, "get info ctrl fail, key=0x%08x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if ( DFD_CFG_ITEM_ID(key) == DFD_CFG_ITEM_HWMON_IN && info_ctrl->src == INFO_SRC_CPLD) {

        rv = dfd_info_get_cpld_voltage(key, &value);
        if(rv < 0) {
            DBG_DEBUG(DBG_ERROR, "get cpld voltage failed.key=0x%08x, rv:%d\n", key, rv);
            return -DFD_RV_DEV_NOTSUPPORT;
        }
        DBG_DEBUG(DBG_VERBOSE, "get cpld voltage ok, value:%d\n", value);
        mem_clear(buf_tmp, sizeof(buf_tmp));
        snprintf(buf_tmp, sizeof(buf_tmp), "%d\n", value);
        buf_real_len = strlen(buf_tmp);
        if(buf_len <= buf_real_len) {
            DBG_DEBUG(DBG_ERROR, "length not enough.buf_len:%d,need length:%d\n", buf_len, buf_real_len);
            return -DFD_RV_DEV_FAIL;
        }
        if (pfun) {
            buf_real_len = buf_len;
            rv = pfun(buf_tmp, strlen(buf_tmp), buf, &buf_real_len, info_ctrl);
            if (rv < 0) {
                DBG_DEBUG(DBG_ERROR, "deal date error.org value:%s, buf_len:%d, rv=%d\n",
                    buf_tmp, buf_len, rv);
                return -DFD_RV_DEV_NOTSUPPORT;
            }
        } else {
            memcpy(buf, buf_tmp, buf_real_len);
        }
        return buf_real_len;
    }

    DBG_DEBUG(DBG_ERROR, "not support mode. key:0x%08x\n", key);
    return -DFD_RV_MODE_NOTSUPPORT;
}

int dfd_info_get_sensor(uint32_t key, char *buf, int buf_len, info_hwmon_buf_f pfun)
{
    info_ctrl_t *key_info_ctrl;
    int rv;

    if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key)) ||
        (buf == NULL) || buf_len <= 0) {
        DBG_DEBUG(DBG_ERROR, "input arguments error, key_path=0x%08x, buf_len:%d.\n",
            key, buf_len);
        return -DFD_RV_INVALID_VALUE;
    }

    key_info_ctrl = dfd_ko_cfg_get_item(key);
    if (key_info_ctrl == NULL) {
        DBG_DEBUG(DBG_ERROR, "key_path info error, key=0x%08x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    mem_clear(buf, buf_len);

    if (key_info_ctrl->mode == INFO_CTRL_MODE_SRT_CONS) {
        snprintf(buf, buf_len, "%s\n", key_info_ctrl->str_cons);
        DBG_DEBUG(DBG_VERBOSE, "get sensor value through string config, key=0x%08x, value:%s\n", key, buf);
        return strlen(buf);
    }

    if (key_info_ctrl->mode == INFO_CTRL_MODE_CFG && key_info_ctrl->src == INFO_SRC_FILE) {
        DBG_DEBUG(DBG_VERBOSE, "get sensor value through hwmon, key:0x%08x\n", key);
        rv = dfd_2key_info_get_buf(key_info_ctrl, buf, buf_len, pfun);
        if (rv < 0) {
            DBG_DEBUG(DBG_VERBOSE, "get sensor value through hwmon failed, key:0x%08x, rv:%d\n", key, rv);
        }
        return rv;
    }
    rv = dfd_info_get_sensor_value(key, buf, buf_len, pfun);
    if( rv < 0) {
        DBG_DEBUG(DBG_ERROR, "get sensor value failed, key=0x%08x, rv:%d.\n", key, rv);
    }
    return rv;
}
