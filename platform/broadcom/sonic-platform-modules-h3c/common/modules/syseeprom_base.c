/*
* Copyright (c) 2019  <sonic@h3c.com>
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
/*公有文件引入*/
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon-sysfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/byteorder.h>
//#include <stdbool.h>
/*私有文件*/
#include "pub.h"
#include "bsp_base.h"
#include "i2c_dev_reg.h"
#include "syseeprom.h"

unsigned long crc32(unsigned long, const unsigned char *, unsigned);
uint32_t crc32_block_endian0(uint32_t, const void *, unsigned, uint32_t *);
uint32_t *crc32_filltable(uint32_t *, int);

u8 eeprom_raw[CONFIG_SYS_EEPROM_SIZE] = {0};

uint32_t *global_crc32_table = NULL;
uint32_t global_crc_table_buffer[256] = {0};

typedef struct tlvinfo_tlv_s tlvinfo_tlv_t;
typedef struct tlvinfo_header_s tlvinfo_header_t;

parse_table fan_table[MAX_FAN_NUM];
parse_table psu_table[MAX_PSU_NUM];
parse_table sys_eeprom_table[1];

extern struct mutex bsp_manu_lock;

static inline bool is_valid_tlvinfo_header(tlvinfo_header_t *hdr)
{
    int max_size = TLV_TOTAL_LEN_MAX;

    if (NULL == hdr)
    {
        return FALSE;
    }
    if ((strcmp(hdr->signature, TLV_INFO_ID_STRING) == 0) &&
        (hdr->version == TLV_INFO_VERSION) &&
        (be16_to_cpu(hdr->totallen) <= max_size))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

bool is_valid_tlv(tlvinfo_tlv_t *tlv)
{
    if (NULL == tlv)
    {
        return FALSE;
    }

    if ((0x00 != tlv->type) && (0xFF != tlv->type))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//设置板卡eeprom写保护, ENABLE为开启写保护, DISABLE为关闭写保护
int bsp_syseeprom_write_protect_set(int enable_disable_flag)
{
    u8 set_value = 0;
    board_static_data *bdata = bsp_get_board_data();
    switch (enable_disable_flag)
    {
        case ENABLE:
            set_value = 0x3;
            break;
        case DISABLE:
            set_value = 0x0;
            break;
        default:
            return -EINVAL;
            break;
    }
    return bsp_cpld_write_part(set_value, bdata->cpld_addr_eeprom_write_protect, bdata->cpld_mask_eeprom_write_protect, bdata->cpld_offs_eeprom_write_protect);
}

int bsp_syseeprom_write_buf(u8 *buf, size_t count)
{
    int i = 0;
    int ret = ERROR_SUCCESS;
    u8 temp = 0;
    board_static_data *bdata = bsp_get_board_data();

    if (count > bdata->eeprom_used_size)
    {
        DBG_ECHO(DEBUG_ERR, "param err: %d bytes larger than eeprom used size %d, abort!", (int)count, (int)bdata->eeprom_used_size)
        return -EINVAL;
    }

    //关闭eeprom写保护
    bsp_syseeprom_write_protect_set(DISABLE);
    //前边用buf覆盖，后边全写0
    for (i = 0; i < bdata->eeprom_used_size; i++)
    {
        temp = (i < count) ? buf[i] : 0;
        ret += bsp_i2c_24LC128_eeprom_write_byte(bdata->i2c_addr_eeprom, i, temp, I2C_DEV_EEPROM);
    }
    //打开eeprom写保护
    bsp_syseeprom_write_protect_set(ENABLE);
    DBG_ECHO(DEBUG_INFO, "write eeprom ret=%d, %d bytes writed.", ret, (int)count)
    return ret;
}

int decode_tlv_value(tlvinfo_tlv_t *tlv, char *value)
{
    int i;
    int len;

    switch (tlv->type)
    {
        case TLV_CODE_PRODUCT_NAME:
        case TLV_CODE_PART_NUMBER:
        case TLV_CODE_SERIAL_NUMBER:
        case TLV_CODE_MANUF_DATE:
        case TLV_CODE_LABEL_REVISION:
        case TLV_CODE_PLATFORM_NAME:
        case TLV_CODE_ONIE_VERSION:
        case TLV_CODE_MANUF_NAME:
        case TLV_CODE_MANUF_COUNTRY:
        case TLV_CODE_VENDOR_NAME:
        case TLV_CODE_DIAG_VERSION:
        case TLV_CODE_SERVICE_TAG:
        case TLV_CODE_SVENDOR:
        case TLV_CODE_HW_VERSION:
        case TLV_CODE_VENDOR_EXT:
            memcpy(value, tlv->value, tlv->length);
            value[tlv->length] = 0;
            break;
        case TLV_CODE_MAC_BASE:
            scnprintf(value, TLV_DECODE_VALUE_MAX_LEN, "%02X:%02X:%02X:%02X:%02X:%02X",
                      tlv->value[0], tlv->value[1], tlv->value[2], tlv->value[3], tlv->value[4], tlv->value[5]);
            break;
        case TLV_CODE_DEVICE_VERSION:
            scnprintf(value, TLV_DECODE_VALUE_MAX_LEN, "%c", tlv->value[0]);
            break;
        case TLV_CODE_MAC_SIZE:
            scnprintf(value, TLV_DECODE_VALUE_MAX_LEN, "%u", (tlv->value[0] << 8) | tlv->value[1]);
            break;
        case TLV_CODE_CRC_32:
            scnprintf(value, TLV_DECODE_VALUE_MAX_LEN, "0x%02X%02X%02X%02X",
                      tlv->value[0], tlv->value[1], tlv->value[2], tlv->value[3]);
            break;
        default:
            value[0] = 0;
            len = 0;

            for (i = 0; (i < TLV_VALUE_MAX_LEN) && (i < tlv->length); i++)
            {
                len += scnprintf(value + len, TLV_DECODE_VALUE_MAX_LEN - len, "0x%02X ", tlv->value[i]);
            }

            if ((len > 0) && (value[len - 1] == ' '))
            {
                value[len - 1] = '\n';
            }
            break;
    }

    return tlv->length;
}

// ONIE-crc部分
int onie_validate_crc(const unsigned char *eeprom_binary, unsigned short int binary_length)
{
    tlvinfo_header_t *eeprom_hdr = (tlvinfo_header_t *)eeprom_binary;
    tlvinfo_tlv_t *eeprom_crc;
    unsigned int calc_crc;
    unsigned int stored_crc;
    if (!is_valid_tlvinfo_header(eeprom_hdr))
    {
        DBG_ECHO(DEBUG_ERR, "onie validate: tlvinfo hdr is invalid")
        return -EINVAL;
    }
    eeprom_crc = (tlvinfo_tlv_t *)&eeprom_binary[sizeof(tlvinfo_header_t) + be16_to_cpu(binary_length) - (sizeof(tlvinfo_tlv_t) + 4)];
    if ((eeprom_crc->type != TLV_CODE_CRC_32) || (eeprom_crc->length != 4))
    {
        DBG_ECHO(DEBUG_ERR, "onie validate: tlvinfo crc is invalid")
        return -EINVAL;
    }
    calc_crc = crc32(0, (void *)eeprom_binary, sizeof(tlvinfo_header_t) + be16_to_cpu(binary_length) - 4);
    stored_crc = ((eeprom_crc->value[0] << 24) | (eeprom_crc->value[1] << 16) | (eeprom_crc->value[2] << 8) | eeprom_crc->value[3]);
    if (calc_crc == stored_crc)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "onie validate: tlvinfo crc != calc_crc, failed")
        return -EINVAL;
    }
}

uint32_t *crc32_filltable(uint32_t *crc_table, int endian)
{
    uint32_t polynomial = endian ? 0x04c11db7 : 0xedb88320;
    uint32_t c;
    int i, j;
    if (!crc_table)
        // crc_table = kmalloc(256 * sizeof(uint32_t),0);
        crc_table = global_crc_table_buffer;
    for (i = 0; i < 256; i++)
    {
        c = endian ? (i << 24) : i;
        for (j = 8; j; j--)
        {
            if (endian)
                c = (c & 0x80000000) ? ((c << 1) ^ polynomial) : (c << 1);
            else
                c = (c & 1) ? ((c >> 1) ^ polynomial) : (c >> 1);
        }
        *crc_table++ = c;
    }
    return crc_table - 256;
}

uint32_t crc32_block_endian0(uint32_t val, const void *buf, unsigned len, uint32_t *crc_table)
{
    const void *end = (uint8_t *)buf + len;
    while (buf != end)
    {
        val = crc_table[(uint8_t)val ^ * (uint8_t *)buf] ^ (val >> 8);
        buf = (uint8_t *)buf + 1;
    }
    return val;
}

unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned len)
{
    if (!global_crc32_table)
    {
        global_crc32_table = crc32_filltable(NULL, 0);
    }
    return crc32_block_endian0(crc ^ 0xffffffffL, buf, len, global_crc32_table) ^ 0xffffffffL;
}

// ONIE部分
int bsp_check_tlv_invalid(char *tlv)
{
    int rv = ERROR_SUCCESS;
    if ((0 == strcmp(tlv, "Not found tlv in eeprom")) || (0 == strcmp(tlv, "\0")) || (0 == strcmp(tlv, "read eeprom error")))
    {
        rv = -1;
    }
    return rv;
}

int bsp_is_tlv_format_data(u16 dev_i2c_address, I2C_DEVICE_E i2c_device_index, u8 *tlv_format)
{
    int ret = ERROR_SUCCESS;
    u16 usAddr = 0;
    u16 read_length = 8;
    u8 tlv_header_info[8] = {0};
    if ((i2c_device_index >= I2C_DEV_FAN) && (i2c_device_index <= I2C_DEV_FAN_BUTT))
    {
        usAddr = 0;
    }
    else if ((i2c_device_index >= I2C_DEV_PSU) && (i2c_device_index <= I2C_DEV_PSU_BUTT))
    {
        usAddr = PSU_1600W_MANU_ADDR;
    }
    else
    {
        usAddr = 0;
    }
    ret = bsp_i2c_24LC128_eeprom_read_bytes(dev_i2c_address, usAddr, read_length, tlv_header_info, i2c_device_index);
    if (ret != ERROR_SUCCESS)
    {
        goto exit;
    }
    *tlv_format = strcmp(tlv_header_info, "TlvInfo") ? 0 : 1;
exit:
    return ret;
}

int bsp_get_tlv_eeprom(I2C_DEVICE_E enDevice, u8 *buf_data)
{
    u16 len = 0;
    u16 usAddr = 0;
    int ret = ERROR_SUCCESS;
    u8 len_buf[2] = {0};
    char dev_log_name[20] = {0};
    board_static_data *bdata = bsp_get_board_data();
    if (buf_data == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "buff_date = NULL")
        return -EFAULT;
    }
    if ((enDevice >= I2C_DEV_FAN) && (enDevice < I2C_DEV_FAN_BUTT))
    {
        usAddr = 0;
        sprintf(dev_log_name, "Fan-%d", enDevice - I2C_DEV_FAN + 1);
    }
    else if ((enDevice >= I2C_DEV_PSU) && (enDevice < I2C_DEV_PSU_BUTT))
    {
        usAddr = PSU_1600W_MANU_ADDR;
        sprintf(dev_log_name, "Psu-%d", enDevice - I2C_DEV_PSU + 1);
    }
    else
    {
        usAddr = 0;
        sprintf(dev_log_name, "Syseeprom-1");
    }
    mutex_lock(&bsp_manu_lock);
    /*读取deivce epprom 长度*/
    ret = bsp_i2c_24LC128_eeprom_read_bytes(bdata->i2c_addr_eeprom, usAddr + 9, 2, len_buf, enDevice);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "%s get tlv eeprom length failed", dev_log_name)
    len = (len_buf[0] << 8) | len_buf[1];
    if (len > SYS_EEPROM_SIZE - sizeof(struct tlvinfo_header_s))
    {
        ret = -EDOM;
        goto exit;
    }

    ret = bsp_i2c_24LC128_eeprom_read_bytes(bdata->i2c_addr_eeprom, usAddr, sizeof(struct tlvinfo_header_s) + len, buf_data, enDevice);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "%s read tlv eeprom failed", dev_log_name)

exit:
    mutex_unlock(&bsp_manu_lock);
    return ret;
}

int onie_tlv_eeprom_decode_to_table(I2C_DEVICE_E enDevice, parse_table *ptable)
{
    tlvinfo_tlv_t *eeprom_tlv = NULL;
    tlvinfo_header_t *eeprom_hdr = NULL;
    int tlv_end;
    int curr_tlv;
    int ret = ERROR_SUCCESS;
    int tmp;
    int tlv_index = 0;
    u8 buf_data[SYS_EEPROM_SIZE] = {0};
    /*获取device tlv eeprom*/
    ret = bsp_get_tlv_eeprom(enDevice, buf_data);
    if (ret != ERROR_SUCCESS)
    {
        goto exit;
    }

    eeprom_hdr = (tlvinfo_header_t *)buf_data;
    ret = onie_validate_crc(buf_data, eeprom_hdr->totallen);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "onie_validate_crc ret ERROR_FAILED.");
    if (NULL == ptable)
    {
        ret = -EINVAL;
        CHECK_IF_ERROR_GOTO_EXIT(ret, "ptable not exists");
    }
    curr_tlv = sizeof(tlvinfo_header_t);
    tlv_end = sizeof(tlvinfo_header_t) + be16_to_cpu(eeprom_hdr->totallen);
    while (curr_tlv < tlv_end)
    {
        /*解析读出的eeprom, 将对应的tlv_type放在ptable的对应位置,
            如果tlv_type不存在，对应位置的tlv_attr_table为全0*/
        eeprom_tlv = (tlvinfo_tlv_t *)&buf_data[curr_tlv];
        tmp = is_valid_tlv(eeprom_tlv);
        CHECK_IF_ZERO_GOTO_EXIT(-EINVAL, ret, tmp, "TLV field starting at EEPROM offset %d is invalid", curr_tlv);
        if ((TLV_ATTR_START <= eeprom_tlv->type && eeprom_tlv->type <= TLV_ATTR_END))
        {
            tlv_index = eeprom_tlv->type - TLV_ATTR_START;
        }
        else if (eeprom_tlv->type == TLV_CODE_VENDOR_EXT)
        {
            tlv_index = MAX_TLV_NUM - 2;
        }
        else if (eeprom_tlv->type == TLV_CODE_CRC_32)
        {
            tlv_index = MAX_TLV_NUM - 1;
        }
        else
        {
            curr_tlv += (sizeof(tlvinfo_tlv_t) + eeprom_tlv->length);
            continue;
        }
        ptable->tlv_attr_table[tlv_index].type = eeprom_tlv->type;
        /*存放在tlv_attr_table中的len为value的长度而不是一段tlv的长度*/
        ptable->tlv_attr_table[tlv_index].length = eeprom_tlv->length;
        memcpy(ptable->tlv_attr_table[tlv_index].value, eeprom_tlv->value, eeprom_tlv->length);
        /*记录字段在eeprom的偏移值*/
        ptable->tlv_attr_table[tlv_index].offset = curr_tlv;
        // printk(KERN_ERR"type:%x, len:%d, offset:%d\n",eeprom_tlv->type, eeprom_tlv->length, curr_tlv);
        curr_tlv += (sizeof(tlvinfo_tlv_t) + eeprom_tlv->length);
    }
    // DBG_ECHO(DEBUG_ERR, "tlv init success")
exit:
    return ret;
}

int onie_ntlv_eeprom_decode_to_table(I2C_DEVICE_E enDevice, parse_table *ptable)
{
    int i = 0;
    u16 start_offset = 0;
    u16 read_length = 0;
    u8 info_data[FAN_MAX_TYPE_COUNT] = {0};
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: onie_ntlv_eeprom_decode_to_table get bd failed");
        return -EINVAL;
    }
    for (i = FAN_VENDOR; i <= FAN_HW_VERSION; i++)
    {
        switch (i)
        {
            case FAN_VENDOR:
                start_offset = REG_ADDR_FAN_VENDOR_NAME;
                read_length = FAN_VENDOR_NAME_BYTE_COUNT;
                break;
            case FAN_PRODUCT_NAME:
                start_offset = REG_ADDR_FAN_PDT_NAME;
                read_length = FAN_PDT_NAME_BYTE_COUNT;
                break;
            case FAN_SN:
            case FAN_PN:
                start_offset = REG_ADDR_FAN_SN;
                read_length = FAN_SN_BYTE_COUNT;
                break;
            case FAN_HW_VERSION:
                start_offset = REG_ADDR_FAN_HW_VER;
                read_length = FAN_HW_VER_BYTE_COUNT;
                break;
        }
        memset(info_data, 0, sizeof(info_data));
        ret = bsp_i2c_24LC128_eeprom_read_bytes(bd->i2c_addr_eeprom, start_offset, read_length, info_data, enDevice);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "fan %u get raw_data failed", enDevice - I2C_DEV_FAN + 1)
            continue;
        }
        memcpy(&(ptable->tlv_attr_table[i - FAN_VENDOR].value), info_data, read_length);
    }
    return ret;
}

int eeprom_table_update(I2C_DEVICE_E enDevice, parse_table *ptable)
{
    int device_index;
    u8 is_tlv = 0;
    int ret = ERROR_SUCCESS;
    board_static_data *bdata = bsp_get_board_data();
    struct mutex *plock = NULL;
    char dev_log_name[20] = {0};
    // DBG_ECHO(DEBUG_ERR, "tlv table init")
    if ((enDevice >= I2C_DEV_FAN) && (enDevice < I2C_DEV_FAN_BUTT))
    {
        device_index = enDevice - I2C_DEV_FAN;
        plock = fan_table_lock + device_index;
        sprintf(dev_log_name, "fan %d", device_index + 1);
    }
    else if ((enDevice >= I2C_DEV_PSU) && (enDevice < I2C_DEV_PSU_BUTT))
    {
        device_index = enDevice - I2C_DEV_PSU;
        plock = psu_table_lock + device_index;
        sprintf(dev_log_name, "psu %d", device_index + 1);
    }
    else
    {
        device_index = 0;
        plock = syseeprom_table_lock + device_index;
        sprintf(dev_log_name, "syseeprom %d", device_index + 1);
    }

    mutex_lock(plock);
    memset(ptable->tlv_attr_table, 0, sizeof(parse_eeprom_tlv) * MAX_TLV_NUM);
    /*
     * 更新table前会先判断eeprom是否为tlv格式:
     * ret！=0表示因读取eeprom错误而导致获取is_tlv失败；
     * is_tlv=0表示是非tlv，is_tlv=1表示是tlv结构.
     */
    ret = bsp_is_tlv_format_data(bdata->i2c_addr_eeprom, enDevice, &is_tlv);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "%s get tlv format failed", dev_log_name)
    if (1 == is_tlv)
    {
        ret = onie_tlv_eeprom_decode_to_table(enDevice, ptable);
        if (ret != ERROR_SUCCESS)
        {
            goto exit;
        }
        if (ptable->is_tlv_type != 1)
        {
            ret = EXCHANGE_FLAG;
        }
        ptable->is_tlv_type = 1;
    }
    else
    {
        if ((enDevice >= I2C_DEV_FAN) && (enDevice < I2C_DEV_FAN_BUTT))
        {
            ret = onie_ntlv_eeprom_decode_to_table(enDevice, ptable);
            if (ret != ERROR_SUCCESS)
            {
                goto exit;
            }
            if (ptable->is_tlv_type != 0)
            {
                ret = EXCHANGE_FLAG;
            }
            ptable->is_tlv_type = 0;
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "%s raw_date not tlv type is unexcepted", dev_log_name)
            ret = -EINVAL;
        }
    }
exit:
    mutex_unlock(plock);
    return ret;
}

void eeprom_table_init(I2C_DEVICE_E enDevice)
{
    // init eprom parse table

    int i;
    int table_len = 0;
    int ret = ERROR_SUCCESS;
    parse_table *ptable = NULL;
    board_static_data *bdata = bsp_get_board_data();
    u8 absent = 0;
    const char *dev_log_name = "Invalid";
    if (enDevice == I2C_DEV_FAN)
    {
        table_len = bdata->fan_num;
        ptable = fan_table;
        dev_log_name = "Fan";
    }
    else if (enDevice == I2C_DEV_PSU)
    {
        table_len = bdata->psu_num;
        ptable = psu_table;
        dev_log_name = "Psu";
    }
    else if (enDevice == I2C_DEV_EEPROM)
    {
        table_len = 1;
        ptable = sys_eeprom_table;
        dev_log_name = "Syseeprom";
    }

    for (i = 0; i < table_len; i++)
    {
        if (enDevice == I2C_DEV_FAN)
        {
            (void)bsp_cpld_get_fan_absent(&absent, i);
            if (absent == CODE_FAN_ABSENT)
                continue;
        }
        if (enDevice == I2C_DEV_PSU)
        {
            (void)bsp_cpld_get_psu_absent(&absent, i);
            if (absent == 1)
                continue;
        }
        ret = eeprom_table_update(enDevice + i, ptable + i);
        if (ERROR_SUCCESS == ret)
        {
            DBG_ECHO(DEBUG_DBG, "%s %d table init success", dev_log_name, i + 1)
        }
        else if (0 < ret)
        {
            DBG_ECHO(DEBUG_DBG, "%s %d not tlv device", dev_log_name, i + 1)
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "%s %d table init failed", dev_log_name, i + 1)
        }
    }
}

int bsp_syseeprom_get_onie_tlv(I2C_DEVICE_E enDevice, unsigned char tlv_type, char *tlv_info_string)
{
    /*fan 获取tlv格式eeprom*/
    int attr_index = 0;
    int device_index = 0;
    int offset = 0;
    u16 usAddr = 0;
    u16 i2c_addr;
    u_int8_t tlv_len = 0;
    int ret = ERROR_SUCCESS;
    u8 buf_data[MAX_TLV_LEN] = {0};
    int flag = -1;
    parse_table *ptable = NULL;
    board_static_data *bdata = bsp_get_board_data();

    /*通过deiceID 来判断是fan psu syseeprom*/
    if ((enDevice >= I2C_DEV_FAN) && (enDevice < I2C_DEV_FAN_BUTT))
    {
        usAddr = 0;
        device_index = enDevice - I2C_DEV_FAN;
        ptable = fan_table + device_index;
        i2c_addr = bdata->i2c_addr_eeprom;
    }
    else if ((enDevice >= I2C_DEV_PSU) && (enDevice < I2C_DEV_PSU_BUTT))
    {
        usAddr = PSU_1600W_MANU_ADDR;
        device_index = enDevice - I2C_DEV_PSU;
        ptable = psu_table + device_index;
        i2c_addr = bdata->i2c_addr_eeprom;
    }
    else
    {
        usAddr = 0;
        device_index = 0;
        ptable = sys_eeprom_table + device_index;
        i2c_addr = bdata->i2c_addr_eeprom;
    }

    /*将属性放在parse_talbe.tlv_attr_table中对应的位置，前二十种tlv_type 属性值是连续的,
        syseeprom中的vendor_ext, crc的属性值和前面的属性不连续*/
    if ((TLV_ATTR_START <= tlv_type) && (tlv_type <= TLV_ATTR_END))
    {
        attr_index = tlv_type - TLV_ATTR_START;
    }
    else if (tlv_type == TLV_CODE_VENDOR_EXT)
    {
        attr_index = MAX_TLV_NUM - 2;
    }
    else if (tlv_type == TLV_CODE_CRC_32)
    {
        attr_index = MAX_TLV_NUM - 1;
    }
    else
    {
        ret = -EINVAL;
        DBG_ECHO(DEBUG_ERR, "attr_type out of range")
        goto exit;
    }

    if (ptable->tlv_attr_table[attr_index].type == tlv_type)
    {
        offset = ptable->tlv_attr_table[attr_index].offset;
        tlv_len = sizeof(tlvinfo_tlv_t) + ptable->tlv_attr_table[attr_index].length;
        ret = bsp_i2c_24LC128_eeprom_read_bytes(i2c_addr, usAddr + offset, tlv_len, buf_data, enDevice);
        /*如果eeprom读取失败可能是由于非tlv格式的原因,这种情况去更新eeprom_table*/
        CHECK_IF_ERROR(ret, "get onie tlv failed, i2c_device:%u eeprom addr:%x read len %d read failed", i2c_addr, usAddr + offset, tlv_len)
        /*只有当前读取的值和表中的值相同才不进行更新*/
        if (!((ret != ERROR_SUCCESS) || (0 != memcmp(buf_data, &(ptable->tlv_attr_table[attr_index]), tlv_len))))
        {
            flag = 0;
        }
    }

    if (flag != 0)
    {
        /*如果当前读出来的tlv和talbe中的tlv不一样更新table, 然后从talbe中获取对应type的数据*/
        DBG_ECHO(DEBUG_DBG, "old_talbe != new_table eeprom_table_update")
        ret = eeprom_table_update(enDevice, ptable);
        if (ret < ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "get onie tlv eeprom table update failed");
            goto exit;
        }
        /*update后如果是tlv格式的则直接返回table中的value*/
        if (ret == ERROR_SUCCESS)
        {
            if (ptable->tlv_attr_table[attr_index].type == tlv_type)
            {
                offset = ptable->tlv_attr_table[attr_index].offset;
                tlv_len = sizeof(tlvinfo_tlv_t) + ptable->tlv_attr_table[attr_index].length;
                memcpy(buf_data, &(ptable->tlv_attr_table[attr_index]), tlv_len);
            }
            else
            {
                ret = -EINVAL;
                DBG_ECHO(DEBUG_ERR, "After tlv table updated, tlv type %x still get failed", tlv_type)
                goto exit;
            }
        }
        else
        {
            /*风扇从eeprom从tlv切换到非tlv 使用bsp_fan_get_info_from_eeprom_by_h3c读取非tlv格式的eeprom 返回EXCHANGE_FLAG*/
            if ((enDevice >= I2C_DEV_FAN) && (enDevice < I2C_DEV_FAN_BUTT))
            {
                goto exit;
            }
            ret = ERROR_SUCCESS;
        }
    }
    /*解析tlv格式数据*/
    decode_tlv_value((tlvinfo_tlv_t *)&buf_data, tlv_info_string);

exit:
    return ret;
}

ssize_t bsp_syseeprom_sysfs_read_model_name(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_PRODUCT_NAME, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_part_number(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_PART_NUMBER, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_manuf_date(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_MANUF_DATE, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_device_version(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_DEVICE_VERSION, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_mac_address(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_MAC_BASE, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

void bsp_syseeprom_get_switch_mac(unsigned char *inMac, unsigned int step, unsigned char *outMac)
{
    int i = 0;
    unsigned int sum = 0;

    for (i = 5; i >= 0; i--)
    {
        sum = (unsigned int)(*(inMac + i) + step);
        if (sum > 0xff)
        {
            step = 1;
            *(outMac + i) = (unsigned char)(sum - 0x100);
        }
        else
        {
            step = 0;
            *(outMac + i) = (unsigned char)(sum);
        }
    }
}

ssize_t bsp_syseeprom_sysfs_read_switch_mac_address(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    unsigned char mac_arr[7] = {0};
    unsigned char switch_mac[7] = {0};
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};

    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_MAC_BASE, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        ret = sscanf(tlv_info_string, "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX", &(mac_arr[0]), &(mac_arr[1]), &(mac_arr[2]), &(mac_arr[3]), &(mac_arr[4]), &(mac_arr[5]));
        if (6 != ret)
        {
            len = -EINVAL;
        }
        else
        {
            bsp_syseeprom_get_switch_mac(mac_arr, 1, switch_mac);
            len = scnprintf(buf, PAGE_SIZE, "%02X:%02X:%02X:%02X:%02X:%02X\n", switch_mac[0], switch_mac[1], switch_mac[2], switch_mac[3], switch_mac[4], switch_mac[5]);
        }
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_label_revision(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_LABEL_REVISION, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_platform_name(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_PLATFORM_NAME, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_onie_version(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_ONIE_VERSION, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_manufacturer(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_MANUF_NAME, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_manuf_country(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_MANUF_COUNTRY, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_vendor_name(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_VENDOR_NAME, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_diag_version(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_DIAG_VERSION, tlv_info_string);

    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_service_tag(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_SERVICE_TAG, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_vendor_ext(char *buf)
{
    ssize_t len = 0;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    int ret = ERROR_SUCCESS;

    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_VENDOR_EXT, tlv_info_string);

    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_mac_nums(char *buf)
{
    ssize_t len = 0;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    int ret = ERROR_SUCCESS;

    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_MAC_SIZE, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_serial_number(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};
    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_SERIAL_NUMBER, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_read_crc32(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN] = {0};

    ret = bsp_syseeprom_get_onie_tlv(I2C_DEV_EEPROM, TLV_CODE_CRC_32, tlv_info_string);
    if (ERROR_SUCCESS == ret)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", tlv_info_string);
    }
    else
    {
        len = ret;
    }
    return len;
}

//读eeprom
ssize_t bsp_syseeprom_sysfs_read_eeprom(char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    board_static_data *bdata = bsp_get_board_data();
    len = bdata->eeprom_used_size;
    if (ERROR_SUCCESS != bsp_i2c_24LC128_eeprom_read_bytes(bdata->i2c_addr_eeprom, 0, len, buf, I2C_DEV_EEPROM))
    {
        len = ret;
    }
    return len;
}

EXPORT_SYMBOL(bsp_syseeprom_get_onie_tlv);
EXPORT_SYMBOL(bsp_check_tlv_invalid);
EXPORT_SYMBOL(bsp_syseeprom_write_protect_set);
EXPORT_SYMBOL(crc32);
EXPORT_SYMBOL(eeprom_raw);

EXPORT_SYMBOL(fan_table);
EXPORT_SYMBOL(psu_table);
EXPORT_SYMBOL(sys_eeprom_table);
EXPORT_SYMBOL(eeprom_table_init);
EXPORT_SYMBOL(eeprom_table_update);
