/*
 * Copyright(C) 2001-2022 Ruijie Network. All rights reserved.
 */
/*
 * rg_fan_driver.c
 * Original Author: sonic_rd@ruijie.com.cn 2020-02-17
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@ruijie.com.cn         2020-02-17          Initial version
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "rg_module.h"
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
    uint8_t   ver;
    uint8_t   flag;
    uint8_t   hw_ver;
    uint8_t   type;
    int16_t   tlv_len;
} dfd_dev_head_info_t;

typedef struct dfd_dev_tlv_info_s {
    uint8_t  type;
    uint8_t  len;
    uint8_t  data[0];
} dfd_dev_tlv_info_t;

int g_dfd_fan_dbg_level = 0;
module_param(g_dfd_fan_dbg_level, int, S_IRUGO | S_IWUSR);

static char *dfd_get_fan_sysfs_name(void)
{
    int key;
    char *sysfs_name;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SYSFS_NAME, 0, 0);
    sysfs_name = dfd_ko_cfg_get_item(key);
    if (sysfs_name == NULL) {
        DFD_FAN_DEBUG(DBG_VERBOSE, "key=0x%08x, sysfs_name is NULL, use default way.\n", key);
    } else {
        DFD_FAN_DEBUG(DBG_VERBOSE, "sysfs_name: %s.\n", sysfs_name);
    }
    return sysfs_name;
}

static int dfd_get_fan_eeprom_mode(void)
{
    int key, mode;
    char *name;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_E2_MODE, 0, 0);
    name = dfd_ko_cfg_get_item(key);
    if (name == NULL) {
        DFD_FAN_DEBUG(DBG_WARN, "get fan eeprom mode fail, key=0x%08x\n", key);
        return FAN_EEPROM_MODE_TLV;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan eeprom mode_name %s.\n", name);
    if (!strncmp(name, DFD_FAN_EEPROM_MODE_TLV_STRING, strlen(DFD_FAN_EEPROM_MODE_TLV_STRING))) {
        mode = FAN_EEPROM_MODE_TLV;
    } else if (!strncmp(name, DFD_FAN_EEPROM_MODE_FRU_STRING, strlen(DFD_FAN_EEPROM_MODE_FRU_STRING))) {
        mode = FAN_EEPROM_MODE_FRU;
    } else {
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

    memcpy(tmp_tlv_len, (uint8_t *)&info.tlv_len, sizeof(uint16_t));
    info.tlv_len = (tmp_tlv_len[0] << 8) + tmp_tlv_len[1];

    if ((info.tlv_len <= 0 ) || (info.tlv_len > 0xFF)) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan maybe not set mac.\n");
        return -DFD_RV_TYPE_ERR;
    }
    DFD_FAN_DEBUG(DBG_VERBOSE, "info.tlv_len: %d\n", info.tlv_len);

    tlv_data = (uint8_t *)kmalloc(info.tlv_len, GFP_KERNEL);
    if (tlv_data == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "tlv_data kmalloc failed \n");
        return -DFD_RV_NO_MEMORY;
    }
    memset(tlv_data, 0, info.tlv_len);

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
         if (tlv->type == cmd && tlv->len <= buf_len ) {
            DFD_FAN_DEBUG(DBG_VERBOSE, "find tlv data, copy...\n");
            memcpy(buf, (uint8_t *)tlv->data, tlv->len);
            buf_len = (uint32_t)tlv->len;
            match_flag = 1;
            break;
         }
        tlv = (dfd_dev_tlv_info_t *)((uint8_t* )tlv + sizeof(dfd_dev_tlv_info_t) + tlv->len);
    }
    kfree(tlv_data);
    if (match_flag == 0) {
        DFD_FAN_DEBUG(DBG_ERROR,"can't find fan tlv date. bus: %d, addr: 0x%02x, tlv type: %d.\n",
            bus, addr, cmd);
        return -DFD_RV_TYPE_ERR;
    }
    return buf_len;
}

static int dfd_get_fan_roll_status(unsigned int fan_index, unsigned int motor_index)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_ROLL_STATUS, fan_index, motor_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan roll status error, fan: %u, motor: %u\n",
            fan_index, motor_index);
        return ret;
    }
    return status;
}

static int dfd_get_fan_present_status(unsigned int fan_index)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_PRESENT_STATUS, RG_MAIN_DEV_FAN, fan_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan present status error, key: 0x%x\n", key);
        return ret;
    }
    return status;
}

static int dfd_get_fan_status(unsigned int fan_index)
{
    int motor_num, motor_index, status, errcnt;

    status = dfd_get_fan_present_status(fan_index);
    if (status != PRESENT) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan index: %u, status: %d\n", fan_index, status);
        return status;
    }

    motor_num = dfd_get_dev_number(RG_MAIN_DEV_FAN, RG_MINOR_DEV_MOTOR);
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
    memset(buf, 0, count);
    return (ssize_t)snprintf(buf, count, "%d\n", ret);
}

static int dfd_fan_product_name_decode(char *fan_buf, int buf_len)
{
    int key, i, j;
    char *p_fan_name, *p_decode_name;
    int *fan_type_num;
    int *fan_display_num;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_NUM, RG_MAIN_DEV_FAN, RG_MINOR_DEV_FAN);
    fan_display_num = dfd_ko_cfg_get_item(key);
    if (fan_display_num == NULL) {
        DFD_FAN_DEBUG(DBG_VERBOSE, "get fan display name number error, key:0x%x, \
            skip fan name decode\n", key);
        return DFD_RV_OK;
    }

    for (i = 1; i <= *fan_display_num; i++) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_TYPE_NUM, i, 0);
        fan_type_num = dfd_ko_cfg_get_item(key);
        if (fan_type_num == NULL) {
            DFD_FAN_DEBUG(DBG_ERROR, "config error, get fan type number error, key: 0x%x\n", key);
            return -DFD_RV_DEV_NOTSUPPORT;
        }
        for (j = 1; j <= *fan_type_num; j++) {
            key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_NAME, i, j);
            p_fan_name = dfd_ko_cfg_get_item(key);
            if (p_fan_name == NULL) {
                DFD_FAN_DEBUG(DBG_ERROR, "config error, get fan origin name error, key: 0x%x\n", key);
                return -DFD_RV_DEV_NOTSUPPORT;
            }
            if (!strncmp(fan_buf, p_fan_name, strlen(p_fan_name))) {
                key = DFD_CFG_KEY(DFD_CFG_ITEM_DECODE_FAN_NAME, i, 0);
                p_decode_name = dfd_ko_cfg_get_item(key);
                if (p_decode_name == NULL) {
                    DFD_FAN_DEBUG(DBG_ERROR, "config error, get fan decode name error, key: 0x%x\n", key);
                    return -DFD_RV_DEV_NOTSUPPORT;
                }
                memset(fan_buf, 0, buf_len);
                strncpy(fan_buf, p_decode_name, buf_len -1);
                DFD_FAN_DEBUG(DBG_VERBOSE, "fan name match ok, display fan name: %s.\n", fan_buf);
                return DFD_RV_OK;
            }
        }
    }

    DFD_FAN_DEBUG(DBG_ERROR, "fan name: %s error, can't match.\n", fan_buf);
    return -DFD_RV_DEV_NOTSUPPORT;
}

ssize_t dfd_get_fan_info(unsigned int fan_index, uint8_t cmd, char *buf, size_t count)
{
    int key, rv, eeprom_mode;
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

    memset(buf, 0, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, RG_MAIN_DEV_FAN, fan_index);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DFD_FAN_DEBUG(DBG_VERBOSE, "can't find fan%u I2C dfd config, key: 0x%08x\n", fan_index, key);
        return snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    sysfs_name = dfd_get_fan_sysfs_name();
    eeprom_mode = dfd_get_fan_eeprom_mode();
    memset(fan_buf, 0, FAN_SIZE);
    if (eeprom_mode == FAN_EEPROM_MODE_TLV) {
        if (cmd == DFD_DEV_INFO_TYPE_PART_NUMBER) {
            DFD_FAN_DEBUG(DBG_VERBOSE, "fan tlv not have part_number attributes\n");
            return snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
        rv = dfd_fan_tlv_eeprom_read(i2c_dev->bus, i2c_dev->addr, cmd, fan_buf, FAN_SIZE, sysfs_name);
    } else {
        rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, cmd, fan_buf, FAN_SIZE, sysfs_name);
    }

    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan eeprom read failed");
        return -DFD_RV_DEV_FAIL;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "%s\n", fan_buf);
    if (cmd == DFD_DEV_INFO_TYPE_NAME) {
        rv = dfd_fan_product_name_decode(fan_buf, FAN_SIZE);
        if (rv < 0) {
            DFD_FAN_DEBUG(DBG_ERROR, "fan name decode error. rv: %d\n", rv);
            return rv;
        }
    }

    snprintf(buf, count, "%s\n", fan_buf);
    return strlen(buf);
}

int dfd_get_fan_speed(unsigned int fan_index, unsigned int motor_index, unsigned int *speed)
{
    int key, ret, speed_tmp;

    if (speed == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED, fan_index, motor_index);
    ret = dfd_info_get_int(key, &speed_tmp, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed error, key: 0x%x, ret: %d\n",
            fan_index, motor_index, key, ret);
        return ret;
    }

    if (speed_tmp == 0 || speed_tmp == 0xffff) {
        *speed = 0;
    } else {
        *speed = 15000000 / speed_tmp;
    }
    return DFD_RV_OK;
}

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
    memset(buf, 0, count);
    return (ssize_t)snprintf(buf, count, "%d\n", speed);
}

int dfd_set_fan_pwm(unsigned int fan_index, int pwm)
{
    int key, ret, data;

    if (pwm < 0 || pwm > 100) {
        DFD_FAN_DEBUG(DBG_ERROR, "can not set pwm = %d.\n", pwm);
        return -DFD_RV_INVALID_VALUE;
    }

    data = pwm * 255 / 100;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_RATIO, fan_index, 0);
    ret = dfd_info_set_int(key, data);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "set fan%u ratio error, key: 0x%x,ret: %d\n",
            fan_index, key, ret);
        return ret;
    }
    return DFD_RV_OK;
}

int dfd_get_fan_pwm(unsigned int fan_index, int *pwm)
{
    int key, ret, ratio;

    if (pwm == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u\n", fan_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_RATIO, fan_index, 0);
    ret = dfd_info_get_int(key, &ratio, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u ratio error, key: 0x%x,ret: %d\n",
            fan_index, key, ret);
        return ret;
    }
    if ((ratio * 100) % 255 > 0) {
        *pwm = ratio * 100 / 255 + 1;
    } else {
        *pwm = ratio * 100 / 255;
    }
    return DFD_RV_OK;
}

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
    memset(buf, 0, count);
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

static int dfd_get_fan_motor_speed_tolerance(unsigned int fan_index, unsigned int motor_index, int *value)
{
    int key;
    int *p_fan_speed_tolerance;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u, motor index: %u.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED_TOLERANCE, fan_index, motor_index);
    p_fan_speed_tolerance = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_tolerance == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed tolerance failed, key: 0x%x\n",
            fan_index, motor_index, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_speed_tolerance;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u motor%u speed tolerance ok, key: 0x%x, value: %d\n",
        fan_index, motor_index, key, *value);
    return DFD_RV_OK;
}

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

    memset(buf, 0, count);
    ret = dfd_get_fan_motor_speed_tolerance(fan_index, motor_index, &value);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

int dfd_get_fan_speed_target(unsigned int fan_index, unsigned int motor_index, int *value)
{
    int key;
    int *p_fan_speed_target;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED_TARGET, fan_index, motor_index);
    p_fan_speed_target = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_target == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed target failed, key: 0x%x\n",
            fan_index, motor_index, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_speed_target;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u motor%u speed target ok, key: 0x%x, value: %d\n",
        fan_index, motor_index, key, *value);
    return DFD_RV_OK;
}

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

    memset(buf, 0, count);
    ret = dfd_get_fan_speed_target(fan_index, motor_index, &value);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

static int dfd_get_fan_direction(unsigned int fan_index, int *value)
{
    int fan_direction, ret;
    char fan_name[FAN_SIZE];

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error, fan index: %u\n", fan_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_NAME, fan_name, FAN_SIZE);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u name error, ret: %d\n", fan_index, ret);
        return ret;
    }
    dfd_ko_cfg_del_space_lf_cr(fan_name);
    DFD_FAN_DEBUG(DBG_VERBOSE, "fan%u name: %s\n", fan_index, fan_name);

    ret = dfd_ko_cfg_get_fan_direction_by_name(fan_name, &fan_direction);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u direction failde, fan name: %s, ret: %d\n",
            fan_index, fan_name, ret);
        return ret;
    }
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u direction ok, fan name: %s, direction: %d\n",
        fan_index, fan_name, fan_direction);
    *value = fan_direction;
    return DFD_RV_OK;
}

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
    memset(buf, 0, count);
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

static int dfd_get_fan_motor_speed_max(unsigned int fan_index, unsigned int motor_index, int *value)
{
    int key;
    int *p_fan_speed_max;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error, fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED_MAX, fan_index, motor_index);
    p_fan_speed_max = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_max == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed max failed, key: 0x%x\n",
            fan_index, motor_index, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_speed_max;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u motor%u speed max success, key: 0x%x, value: %d\n",
        fan_index, motor_index, key, *value);
    return DFD_RV_OK;
}

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

    memset(buf, 0, count);
    ret = dfd_get_fan_motor_speed_max(fan_index, motor_index, &value);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}

static int dfd_get_fan_motor_speed_min(unsigned int fan_index, unsigned int motor_index, int *value)
{
    int key;
    int *p_fan_speed_min;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index: %u, motor index: %u\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED_MIN, fan_index, motor_index);
    p_fan_speed_min = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_min == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan%u motor%u speed min failed, key: 0x%x\n",
            fan_index, motor_index, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_speed_min;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan%u motor%u speed min success, key: 0x%x, value: %d\n",
        fan_index, motor_index, key, *value);
    return DFD_RV_OK;
}

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

    memset(buf, 0, count);
    ret = dfd_get_fan_motor_speed_min(fan_index, motor_index, &value);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", value);
}
