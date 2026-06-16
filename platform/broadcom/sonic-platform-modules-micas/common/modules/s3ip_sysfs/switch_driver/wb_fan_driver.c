/*
 * An wb_fan_driver driver for fan devcie function
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

#include <linux/module.h>
#include <linux/slab.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"
#include "dfd_frueeprom.h"

#define DFD_FAN_EEPROM_MODE_TLV_STRING    "tlv"
#define DFD_FAN_EEPROM_MODE_FRU_STRING    "fru"
#define FAN_SIZE                          (256)

typedef enum fan_present_status_e {
    ABSENT  = 0,
    PRESENT = 1,
} fan_present_status_t;

typedef enum fan_motor_status_e {
    MOTOR_STALL = 0,
    MOTOR_ROLL  = 1,
} fan_motor_status_t;

typedef enum fan_eeprom_mode_e {
    FAN_EEPROM_MODE_TLV,     /* TLV */
    FAN_EEPROM_MODE_FRU,      /*FRU*/
} fan_eeprom_mode_t;

typedef struct  dfd_dev_head_info_s {
    uint8_t   ver;                       /* The version number defined in the E2PROM file, initially 0x01 */
    uint8_t   flag;                      /* The new version E2PROM is identified as 0x7E */
    uint8_t   hw_ver;                    /* It consists of two parts: the main version number and the revised version */
    uint8_t   type;                      /* Hardware type definition information */
    int16_t   tlv_len;                   /* Valid data length (16 bits) */
} dfd_dev_head_info_t;

typedef struct dfd_dev_tlv_info_s {
    uint8_t  type;                       /* Data type */
    uint8_t  len;                        /* Data length */
    uint8_t  data[0];                    /* Data */
} dfd_dev_tlv_info_t;

/* Specifies the fixed value of the fan speed */
typedef enum wb_fan_threshold_e {
    FAN_SPEED_MIN   = 1,        /* Minimum value */
    FAN_SPEED_MAX   = 2,        /* Maximum value */
    FAN_SPEED_TOLERANCE = 3,    /* tolerance */
    FAN_SPEED_TARGET_0 = 0x10,  /* index of the rated speed when PWM=0x */
    FAN_SPEED_TARGET_10 = 0x11,
    FAN_SPEED_TARGET_20 = 0x12,
    FAN_SPEED_TARGET_30 = 0x13,
    FAN_SPEED_TARGET_40 = 0x14,
    FAN_SPEED_TARGET_50 = 0x15,
    FAN_SPEED_TARGET_60 = 0x16,
    FAN_SPEED_TARGET_70 = 0x17,
    FAN_SPEED_TARGET_80 = 0x18,
    FAN_SPEED_TARGET_90 = 0x19,
    FAN_SPEED_TARGET_100 = 0x1a, /* index of the rated speed when PWM=100 */

} wb_fan_threshold_t;

/* fan_threshold_[Threshold type(high 8bit)+Master device type(low 8bit)]_[subdevice ID(high 4bit)+Front and rear motor id(low 4bit)] */
#define DFD_GET_FAN_THRESHOLD_KEY1(threshold_type, main_dev_id) \
    (((threshold_type & 0xff) << 8) | (main_dev_id & 0xff))
#define DFD_GET_FAN_THRESHOLD_KEY2(sub_type_id, motor_id) \
        (((sub_type_id & 0x0f) << 4) | (motor_id & 0x0f))

int g_dfd_fan_dbg_level = 0;
module_param(g_dfd_fan_dbg_level, int, S_IRUGO | S_IWUSR);

static char *dfd_get_fan_sysfs_name(void)
{
    uint64_t key;
    char *sysfs_name;

    /* string type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SYSFS_NAME, 0, 0);
    sysfs_name = dfd_ko_cfg_get_item(key);
    if (sysfs_name == NULL) {
        DFD_FAN_DEBUG(DBG_VERBOSE, "key_name=%s, sysfs_name is NULL, use default way.\n", 
            key_to_name(DFD_CFG_ITEM_FAN_SYSFS_NAME));
    } else {
        DFD_FAN_DEBUG(DBG_VERBOSE, "sysfs_name: %s.\n", sysfs_name);
    }
    return sysfs_name;
}

/**
 * dfd_get_fan_eeprom_mode - The fan type E2 is obtained
 * return: 0:TLV
 *         1:FRU
 *       : Negative value - Read failed
 */
static int dfd_get_fan_eeprom_mode(void)
{
    uint64_t key;
    int mode;
    char *name;

    /* string type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_E2_MODE, 0, 0);
    name = dfd_ko_cfg_get_item(key);
    if (name == NULL) {
        /* By default, the TLV format is returned */
        DFD_FAN_DEBUG(DBG_WARN, "get fan eeprom mode fail, key_name=%s\n", 
            key_to_name(DFD_CFG_ITEM_FAN_E2_MODE));
        return FAN_EEPROM_MODE_TLV;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan eeprom mode_name %s.\n", name);
    if (!strncmp(name, DFD_FAN_EEPROM_MODE_TLV_STRING, strlen(DFD_FAN_EEPROM_MODE_TLV_STRING))) {
        mode = FAN_EEPROM_MODE_TLV;
    } else if (!strncmp(name, DFD_FAN_EEPROM_MODE_FRU_STRING, strlen(DFD_FAN_EEPROM_MODE_FRU_STRING))) {
        mode = FAN_EEPROM_MODE_FRU;
    } else {
        /* The default TLV mode is returned */
        mode = FAN_EEPROM_MODE_TLV;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan eeprom mode %d.\n", mode);
    return mode;
}

static int dfd_fan_tlv_eeprom_read(int bus, int addr, uint8_t cmd, char *buf, int len,
               const char *sysfs_name)
{
    dfd_dev_head_info_t info;
    char tmp_tlv_len[sizeof(uint16_t)];
    char *tlv_data;
    dfd_dev_tlv_info_t *tlv;
    int buf_len;
    int rv, match_flag;

    rv = dfd_ko_i2c_read(bus, addr, 0, (uint8_t *)&info, sizeof(dfd_dev_head_info_t), sysfs_name);
    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "read fan i2c failed, bus: %d, addr: 0x%x, rv: %d\n",
            bus, addr, rv);
        return -DFD_RV_DEV_FAIL;
    }

    /* convert TLV_LEN */
    memcpy(tmp_tlv_len, (uint8_t *)&info.tlv_len, sizeof(uint16_t));
    info.tlv_len = (tmp_tlv_len[0] << 8) + tmp_tlv_len[1];

    if ((info.tlv_len <= 0) || (info.tlv_len > 0xFF)) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan maybe not set mac.\n");
        return -DFD_RV_TYPE_ERR;
    }
    DFD_FAN_DEBUG(DBG_VERBOSE, "info.tlv_len: %d\n", info.tlv_len);

    tlv_data = (uint8_t *)kmalloc(info.tlv_len, GFP_KERNEL);
    if (tlv_data == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "tlv_data kmalloc failed \n");
        return -DFD_RV_NO_MEMORY;
    }
    mem_clear(tlv_data, info.tlv_len);

    rv = dfd_ko_i2c_read(bus, addr, sizeof(dfd_dev_head_info_t), tlv_data, info.tlv_len, sysfs_name);
    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR,"fan eeprom read failed\n");
        kfree(tlv_data);
        return -DFD_RV_DEV_FAIL;
    }

    buf_len = len - 1;
    match_flag = 0;
    for (tlv = (dfd_dev_tlv_info_t *)tlv_data; (ulong)tlv < (ulong)tlv_data + info.tlv_len;) {
        DFD_FAN_DEBUG(DBG_VERBOSE, "tlv: %p, tlv->type: 0x%x, tlv->len: 0x%x info->tlv_len: 0x%x\n",
            tlv, tlv->type, tlv->len, info.tlv_len);
        if (tlv->type == cmd && tlv->len <= buf_len) {
            DFD_FAN_DEBUG(DBG_VERBOSE, "find tlv data, copy...\n");
            memcpy(buf, (uint8_t *)tlv->data, tlv->len);
            buf_len = (uint32_t)tlv->len;
            match_flag = 1;
            break;
        }
        tlv = (dfd_dev_tlv_info_t *)((uint8_t*)tlv + sizeof(dfd_dev_tlv_info_t) + tlv->len);
    }
    kfree(tlv_data);
    if (match_flag == 0) {
        DFD_FAN_DEBUG(DBG_ERROR,"can't find fan tlv date. bus: %d, addr: 0x%02x, tlv type: %d.\n",
            bus, addr, cmd);
        return -DFD_RV_TYPE_ERR;
    }
    return buf_len;
}

/**
 * dfd_get_fan_present_status - Obtain the fan running status
 * @index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * return: 0:STALL
 *         1:ROLL
 *       : Negative value - Read failed
 */
static int dfd_get_fan_roll_status(unsigned int fan_index, unsigned int motor_index)
{
    uint64_t key;
    int ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_ROLL_STATUS, fan_index, motor_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan roll status error, fan: %u, motor: %u, key_name: %s\n",
            fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_ROLL_STATUS));
        return ret;
    }
    return status;
}

/**
 * dfd_get_fan_present_status - Obtain the fan status
 * @index: Number of the fan, starting from 1
 * return: 0:ABSENT
 *         1:PRESENT
 *       : Negative value - Read failed
 */
int dfd_get_fan_present_status(unsigned int fan_index)
{
    uint64_t key;
    int ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_PRESENT_STATUS, WB_MAIN_DEV_FAN, fan_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan present status error, key_name: %s\n", 
            key_to_name(DFD_CFG_ITEM_DEV_PRESENT_STATUS));
        return ret;
    }
    return status;
}

/**
 * dfd_get_fan_status - Obtaining fan status
 * @index: Number of the fan, starting from 1
 * return: 0:ABSENT
 *         1:OK
 *         2:NOT OK
 *       : Negative value - Read failed
 */
static int dfd_get_fan_status(unsigned int fan_index)
{
    int motor_num, motor_index, status, errcnt;

    /* Obtaining fan status */
    status = dfd_get_fan_present_status(fan_index);
    if (status != PRESENT) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan index: %u, status: %d\n", fan_index, status);
        return status;
    }

    /* Get the motor running state */
    motor_num = dfd_get_dev_number(WB_MAIN_DEV_FAN, WB_MINOR_DEV_MOTOR);
    if (motor_num <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get motor number error: %d\n", motor_num);
        return -DFD_RV_DEV_FAIL;
    }
    errcnt = 0;
    for (motor_index = 1; motor_index <= motor_num; motor_index++) {
        status = dfd_get_fan_roll_status(fan_index, motor_index);
        if (status < 0) {
            DFD_FAN_DEBUG(DBG_ERROR,  "get fan roll status error, fan index: %u, motor index: %d, status: %d\n",
                fan_index, motor_index, status);
            return status;
        }
        if (status != MOTOR_ROLL) {
            DFD_FAN_DEBUG(DBG_ERROR,
                "stall:fan index: %u, motor index: %d, status: %d\n",fan_index, motor_index, status);
            errcnt++;
        }
    }
    if (errcnt > 0) {
        return FAN_STATUS_NOT_OK;
    }
    return FAN_STATUS_OK;
}

/**
 * dfd_get_fan_status_str - Obtaining fan status
 * @index: Number of the fan, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_fan_status_str(unsigned int fan_index, char *buf, size_t count)
{
    int ret;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "params error, fan_index: %u count: %lu",
            fan_index, count);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u\n",
            count, fan_index);
        return -DFD_RV_INVALID_VALUE;
    }
    ret = dfd_get_fan_status(fan_index);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan status error, ret: %d, fan_index: %u\n",
            ret, fan_index);
        return ret;
    }
    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", ret);
}


/**
 * dfd_get_fan_present_str - Obtaining fan present status
 * @index: Number of the fan, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_fan_present_str(unsigned int fan_index, char *buf, size_t count)
{
    int ret;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "params error, fan_index: %u count: %lu",
            fan_index, count);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u\n",
            count, fan_index);
        return -DFD_RV_INVALID_VALUE;
    }
    ret = dfd_get_fan_present_status(fan_index);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan present status error, ret: %d, fan_index: %u\n",
            ret, fan_index);
        return ret;
    }
    mem_clear(buf,  count);
    return (ssize_t)snprintf(buf, count, "%d\n", ret);
}

/**
 * dfd_get_fan_motor_status_str - Obtain the fan motor status
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_status_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count)
{
    int ret;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL, fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u, motor index: %u\n",
            count, fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }
    ret = dfd_get_fan_roll_status(fan_index, motor_index);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan motor status error, ret: %d, fan_index: %u, motor index: %u\n",
            ret, fan_index, motor_index);
        return ret;
    }
    mem_clear(buf,  count);
    return (ssize_t)snprintf(buf, count, "%d\n", ret);
}

/**
 * dfd_fan_product_name_decode - Fan name conversion
 * @psu_buf: Original fan name
 * @buf_len: fan_buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
static int dfd_fan_product_name_decode(char *fan_buf, int buf_len)
{
    uint64_t key;
    int i, j;
    char *p_fan_name, *p_decode_name;
    int *fan_type_num;
    int *fan_display_num;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_NUM, WB_MAIN_DEV_FAN, WB_MINOR_DEV_FAN);
    fan_display_num = dfd_ko_cfg_get_item(key);
    if (fan_display_num == NULL) {
        DFD_FAN_DEBUG(DBG_VERBOSE, "get fan display name number error, key_name:%s, \
            skip fan name decode\n", key_to_name(DFD_CFG_ITEM_DEV_NUM));
        return DFD_RV_OK;
    }

    for (i = 1; i <= *fan_display_num; i++) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_TYPE_NUM, i, 0);
        fan_type_num = dfd_ko_cfg_get_item(key);
        if (fan_type_num == NULL) {
            DFD_FAN_DEBUG(DBG_ERROR, "config error, get fan type number error, key_name: %s\n", 
                key_to_name(DFD_CFG_ITEM_FAN_TYPE_NUM));
            return -DFD_RV_DEV_NOTSUPPORT;
        }
        for (j = 1; j <= *fan_type_num; j++) {
            key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_NAME, i, j);
            p_fan_name = dfd_ko_cfg_get_item(key);
            if (p_fan_name == NULL) {
                DFD_FAN_DEBUG(DBG_ERROR, "config error, get fan origin name error, key_name: %s\n", 
                    key_to_name(DFD_CFG_ITEM_FAN_NAME));
                return -DFD_RV_DEV_NOTSUPPORT;
            }
            if (!strncmp(fan_buf, p_fan_name, strlen(p_fan_name))) {
                key = DFD_CFG_KEY(DFD_CFG_ITEM_DECODE_FAN_NAME, i, 0);
                p_decode_name = dfd_ko_cfg_get_item(key);
                if (p_decode_name == NULL) {
                    DFD_FAN_DEBUG(DBG_ERROR, "config error, get fan decode name error, key_name: %s\n", 
                        key_to_name(DFD_CFG_ITEM_DECODE_FAN_NAME));
                    return -DFD_RV_DEV_NOTSUPPORT;
                }
                mem_clear(fan_buf, buf_len);
                strlcpy(fan_buf, p_decode_name, buf_len);
                DFD_FAN_DEBUG(DBG_VERBOSE, "fan name match ok, display fan name: %s.\n", fan_buf);
                return DFD_RV_OK;
            }
        }
    }

    DFD_FAN_DEBUG(DBG_ERROR, "fan name: %s error, can't match.\n", fan_buf);
    return -DFD_RV_DEV_NOTSUPPORT;
}

/**
 * dfd_get_fan_info - Obtaining Fan Information
 * @index: Number of the fan, starting from 1
 * @cmd: Fan information type, fan name :2, fan serial number :3, fan hardware version :5
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_info(unsigned int fan_index, uint8_t cmd, char *buf, size_t count)
{
    uint64_t key;
    int rv, eeprom_mode;
    char fan_buf[FAN_SIZE];
    dfd_i2c_dev_t *i2c_dev;
    const char *sysfs_name;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL, fan index: %u, cmd: 0x%x.\n", fan_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u, cmd: 0x%x.\n",
            count, fan_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, WB_MAIN_DEV_FAN, fan_index);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DFD_FAN_DEBUG(DBG_VERBOSE, "can't find fan%u I2C dfd config, key_name: %s\n", 
            fan_index, key_to_name(DFD_CFG_ITEM_OTHER_I2C_DEV));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    sysfs_name = dfd_get_fan_sysfs_name();
    eeprom_mode = dfd_get_fan_eeprom_mode();
    mem_clear(fan_buf, FAN_SIZE);
    if (eeprom_mode == FAN_EEPROM_MODE_TLV) {
        if (cmd == DFD_DEV_INFO_TYPE_PART_NUMBER) {
            DFD_FAN_DEBUG(DBG_VERBOSE, "fan tlv not have part_number attributes\n");
            return -DFD_RV_DEV_NOTSUPPORT;
        }
        rv = dfd_fan_tlv_eeprom_read(i2c_dev->bus, i2c_dev->addr, cmd, fan_buf, FAN_SIZE, sysfs_name);
    } else {
        if (cmd == DFD_DEV_INFO_TYPE_VENDOR) {
            rv = dfd_get_fru_board_data(i2c_dev->bus, i2c_dev->addr, cmd, fan_buf, FAN_SIZE, sysfs_name);
        } else {
            rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, cmd, fan_buf, FAN_SIZE, sysfs_name);
        }
    }

    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan eeprom read failed");
        return -DFD_RV_DEV_FAIL;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "%s\n", fan_buf);
    /* Fan product name conversion */
    if (cmd == DFD_DEV_INFO_TYPE_NAME) {
        rv = dfd_fan_product_name_decode(fan_buf, FAN_SIZE);
        if (rv < 0) {
            DFD_FAN_DEBUG(DBG_ERROR, "fan name decode error. rv: %d\n", rv);
        }
    }

    snprintf(buf, count, "%s\n", fan_buf);
    return strlen(buf);
}

/**
 * dfd_get_fan_speed - Obtain the fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @speed: Speed value
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_get_fan_speed(unsigned int fan_index, unsigned int motor_index, unsigned int *speed)
{
    uint64_t key;
    int ret, speed_tmp;

    if (speed == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED, fan_index, motor_index);
    ret = dfd_info_get_int(key, &speed_tmp, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed error, key: %s, ret: %d\n",
            fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_SPEED), ret);
        return ret;
    }

    if (speed_tmp == 0 || speed_tmp == 0xffff) {
        *speed = 0;
    } else {
        *speed = 15000000 / speed_tmp;
    }
    return DFD_RV_OK;
}

/**
 * dfd_get_fan_speed_str - Obtain the fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_speed_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count)
{
    int ret;
    unsigned int speed;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL, fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u, motor index: %u\n",
            count, fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }
    ret = dfd_get_fan_speed(fan_index, motor_index, &speed);
    if (ret < 0) {
        return ret;
    }
    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", speed);
}

/**
 * dfd_set_fan_pwm - set the fan speed duty cycle
 * @fan_index: Number of the fan, starting from 1
 * @pwm:   Duty cycle
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_fan_pwm(unsigned int fan_index, int pwm)
{
    uint64_t key;
    int ret, data;

    if (pwm < 0 || pwm > 100) {
        DFD_FAN_DEBUG(DBG_ERROR, "can not set pwm = %d.\n", pwm);
        return -DFD_RV_INVALID_VALUE;
    }

    data = pwm * 255 / 100;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_RATIO, fan_index, 0);
    ret = dfd_info_set_int(key, data);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "set fan%u ratio error, key_name: %s,ret: %d\n",
            fan_index, key_to_name(DFD_CFG_ITEM_FAN_RATIO), ret);
        return ret;
    }
    return DFD_RV_OK;
}

/**
 * dfd_get_fan_pwm - Obtain the fan speed duty cycle
 * @fan_index: Number of the fan, starting from 1
 * @pwm:   Duty cycle
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_get_fan_pwm(unsigned int fan_index, int *pwm)
{
    uint64_t key;
    int ret, ratio;

    if (pwm == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u\n", fan_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_RATIO, fan_index, 0);
    ret = dfd_info_get_int(key, &ratio, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u ratio error, key_name: %s,ret: %d\n",
            fan_index, key_to_name(DFD_CFG_ITEM_FAN_RATIO), ret);
        return ret;
    }
    if ((ratio * 100) % 255 > 0) {
        *pwm = ratio * 100 / 255 + 1;
    } else {
        *pwm = ratio * 100 / 255;
    }
    return DFD_RV_OK;
}

/**
 * dfd_get_fan_pwm_str - Obtain the fan speed duty cycle
 * @fan_index: Number of the fan, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_pwm_str(unsigned int fan_index, char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL, fan index: %u\n", fan_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u\n", count,
            fan_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_fan_pwm(fan_index, &value);
    if (ret < 0) {
        return ret;
    }
    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

static int dfd_get_fan_type(unsigned int fan_index, int *fan_type, int *fan_sub_type)
{
    int rv;
    char fan_buf[FAN_SIZE];

    /* Get the fan name */
    rv = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_NAME, fan_buf, FAN_SIZE);
    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u name error, ret: %d\n", fan_index, rv);
        return rv;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "%s\n", fan_buf);
    dfd_info_del_no_print_string(fan_buf);

    DFD_FAN_DEBUG(DBG_VERBOSE, "dfd_fan_product_name_decode get fan name %s\n", fan_buf);
    rv = dfd_ko_cfg_get_fan_type_by_name((char *)fan_buf, fan_type, fan_sub_type);
    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u type by name error, ret: %d\n", fan_index, rv);
        return -DFD_RV_NO_NODE;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u type %d subtype %d by name ok\n", fan_index, *fan_type, *fan_sub_type);
    return DFD_RV_OK;
}

/**
 * dfd_get_fan_speed_target - Obtain the standard fan speed
 * @fan_index
 * @motor_index
 * @value Standard speed value
 * @returns: 0 success, negative value: failed
 */
int dfd_get_fan_speed_target(unsigned int fan_index, unsigned int motor_index, int *value)
{
    uint64_t key;
    int *p_fan_speed_target;
    int key1, key2, fan_type, fan_sub_type, pwm;
    int ret;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Get the fan type */
    ret = dfd_get_fan_type(fan_index, &fan_type, &fan_sub_type);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan get type error, rv: %d\n", ret);
        return -EIO;
    }

    /* Get current PWM */
    ret = dfd_get_fan_pwm(fan_index, &pwm);
    if (ret < 0) {
        return ret;
    }

    /* Gets the rated speed corresponding to the current PWM */
    key1 = DFD_GET_FAN_THRESHOLD_KEY1((pwm / 10 + FAN_SPEED_TARGET_0), WB_MAIN_DEV_FAN);
    key2 = DFD_GET_FAN_THRESHOLD_KEY2(fan_type, motor_index);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_THRESHOLD, key1, key2);
    p_fan_speed_target = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_target == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed target failed, key_name: %s\n",
            fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_THRESHOLD));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_speed_target;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u motor%u speed target ok, key_name: %s, value: %d\n",
        fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_THRESHOLD), *value);
    return DFD_RV_OK;
}

/**
 * dfd_get_fan_motor_speed_target_str - Obtain the standard fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_speed_target_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL, fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u, motor index: %u\n",
            count, fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    ret = dfd_get_fan_speed_target(fan_index, motor_index, &value);
    if (ret < 0) {
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

/**
 * dfd_get_fan_motor_speed_tolerance - Obtain the fan speed tolerance
 * @fan_index
 * @motor_index
 * @value Speed tolerance
 */
static int dfd_get_fan_motor_speed_tolerance(unsigned int fan_index, unsigned int motor_index, int *value)
{
    uint64_t key;
    int *p_fan_speed_tolerance;
    int target, ret;
    int key1, key2, fan_type, fan_sub_type;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u, motor index: %u.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Get the fan type */
    ret = dfd_get_fan_type(fan_index, &fan_type, &fan_sub_type);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan get type error, ret: %d\n", ret);
        return -EIO;
    }

    /* Obtain the error rate of the fan */
    key1 = DFD_GET_FAN_THRESHOLD_KEY1(FAN_SPEED_TOLERANCE, WB_MAIN_DEV_FAN);
    key2 = DFD_GET_FAN_THRESHOLD_KEY2(fan_type, motor_index);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_THRESHOLD, key1, key2);
    p_fan_speed_tolerance = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_tolerance == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed tolerance failed, key_name: %s\n",
            fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_THRESHOLD));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    /* Obtain the fan speed target */
    ret = dfd_get_fan_speed_target(fan_index, motor_index, &target);
    if (ret < 0) {
        return ret;
    }

    /* error rpm = Rated speed corresponding to the current pwm * Fan error ratio / 100 */
    *value = target * *p_fan_speed_tolerance / 100;

    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u motor%u speed tolerance ok, key: %s, tolerance rate: %d, value: %d\n",
        fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_THRESHOLD), *p_fan_speed_tolerance, *value);
    return DFD_RV_OK;
}

/**
 * dfd_get_fan_motor_speed_tolerance_str - Obtain the fan speed tolerance
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Duct type receives buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_speed_tolerance_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL, fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u, motor index: %u\n",
            count, fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    ret = dfd_get_fan_motor_speed_tolerance(fan_index, motor_index, &value);
    if (ret < 0) {
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

/**
 * dfd_get_fan_direction - Obtain the fan air duct type
 * @fan_index:  The fan offset starts from 1
 * @value 0:F2B, 1:B2F
 * @returns: 0 success, negative value: failed
 */
static int dfd_get_fan_direction(unsigned int fan_index, int *value)
{
    uint64_t key;
    int *p_fan_dirction;
    int fan_type, fan_sub_type;
    int rv;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error, fan index: %u\n", fan_index);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Get the fan type */
    rv = dfd_get_fan_type(fan_index, &fan_type, &fan_sub_type);
    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan get type error, rv: %d\n", rv);
        return -EIO;
    }

    /* Obtain the fan direction based on the fan type and subtype */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_DIRECTION, fan_type, fan_sub_type);
    p_fan_dirction = dfd_ko_cfg_get_item(key);
    if (p_fan_dirction == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u direction failed, key_name: %s\n",
            fan_index, key_to_name(DFD_CFG_ITEM_FAN_DIRECTION));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_dirction;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u direction success, key_name: %s, value: %d\n",
        fan_index, key_to_name(DFD_CFG_ITEM_FAN_DIRECTION), *value);
    return DFD_RV_OK;
}

/**
 * dfd_get_fan_direction_str - Obtain the fan air duct type
 * @fan_index:The fan offset starts from 1
 * @buf :Duct type receives buf
 * @count :Duct type receives buf length
 * @returns: Succeeded: Air duct type String length
 *           Failure: negative value
 */
ssize_t dfd_get_fan_direction_str(unsigned int fan_index, char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error, buf is NULL, fan index: %u.\n", fan_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error, buf is NULL, fan index: %u.\n", fan_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_fan_direction(fan_index, &value);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan direction string failed, ret: %d, fan_index: %u\n",
            ret, fan_index);
        return ret;
    }
    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

/**
 * dfd_get_fan_motor_speed_max - Obtain the maximum fan speed
 * @fan_index
 * @motor_index
 * @value Maximum fan speed
 * @returns: 0 success, negative value: failed
 */
static int dfd_get_fan_motor_speed_max(unsigned int fan_index, unsigned int motor_index, int *value)
{
    uint64_t key;
    int *p_fan_speed_max;
    int key1, key2, fan_type, fan_sub_type;
    int rv;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error, fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Get the fan type */
    rv = dfd_get_fan_type(fan_index, &fan_type, &fan_sub_type);
    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan get type error, rv: %d\n", rv);
        return -EIO;
    }

    key1 = DFD_GET_FAN_THRESHOLD_KEY1(FAN_SPEED_MAX, WB_MAIN_DEV_FAN);
    key2 = DFD_GET_FAN_THRESHOLD_KEY2(fan_type, motor_index);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_THRESHOLD, key1, key2);
    p_fan_speed_max = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_max == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed max failed, key_name: %s\n",
            fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_THRESHOLD));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_speed_max;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u motor%u speed max success, key_name: %s, value: %d\n",
        fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_THRESHOLD), *value);
    return DFD_RV_OK;
}

/**
 * dfd_get_fan_motor_speed_max_str - Obtain the standard fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_speed_max_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL, fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u, motor index: %u\n",
            count, fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    ret = dfd_get_fan_motor_speed_max(fan_index, motor_index, &value);
    if (ret < 0) {
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

/**
 * dfd_get_fan_motor_speed_min - Obtain the minimum fan speed
 * @fan_index
 * @motor_index
 * @value Minimum fan speed
 * @returns: 0 success, negative value: failed
 */
static int dfd_get_fan_motor_speed_min(unsigned int fan_index, unsigned int motor_index, int *value)
{
    uint64_t key;
    int *p_fan_speed_min;
    int key1, key2, fan_type, fan_sub_type;
    int rv;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    /* Get the fan type */
    rv = dfd_get_fan_type(fan_index, &fan_type, &fan_sub_type);
    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan get type error, rv: %d\n", rv);
        return -EIO;
    }

    key1 = DFD_GET_FAN_THRESHOLD_KEY1(FAN_SPEED_MIN, WB_MAIN_DEV_FAN);
    key2 = DFD_GET_FAN_THRESHOLD_KEY2(fan_type, motor_index);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_THRESHOLD, key1, key2);
    p_fan_speed_min = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_min == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed min failed, key_name: %s\n",
            fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_THRESHOLD));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_speed_min;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u motor%u speed min success, key_name: %s, value: %d\n",
        fan_index, motor_index, key_to_name(DFD_CFG_ITEM_FAN_THRESHOLD), *value);
    return DFD_RV_OK;
}

/**
 * dfd_get_fan_motor_speed_min_str - Obtain the standard fan speed
 * @fan_index: Number of the fan, starting from 1
 * @motor_index: Motor number, starting with 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fan_motor_speed_min_str(unsigned int fan_index, unsigned int motor_index,
            char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL, fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf size error, count: %lu, fan index: %u, motor index: %u\n",
            count, fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    ret = dfd_get_fan_motor_speed_min(fan_index, motor_index, &value);
    if (ret < 0) {
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}
