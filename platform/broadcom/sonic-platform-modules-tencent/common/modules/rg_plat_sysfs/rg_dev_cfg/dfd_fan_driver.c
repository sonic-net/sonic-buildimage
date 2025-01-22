/*
 * Copyright(C) 2001-2012 Ruijie Network. All rights reserved.
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_adapter.h"
#include "./include/dfd_cfg_info.h"
#include "./include/dfd_frueeprom.h"
#include "../rg_dev_sysfs/include/rg_sysfs_common.h"

#define DFD_FAN_EEPROM_MODE_TLV_STRING    "tlv"
#define DFD_FAN_EEPROM_MODE_FRU_STRING    "fru"
#define FAN_SIZE                          (256)

typedef enum fan_status_e {
    FAN_ABSENT = 0,
    FAN_OK     = 1,
    FAN_NOT_OK = 2,
} fan_status_t;

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

typedef enum fan_direction_e {
    FRONT_TO_BACK  = 0,
    BACK_TO_FRONT  = 1,
    FAN_DIRECTION_END  = 2,
} fan_direction_t;

int g_dfd_fan_dbg_level = 0;
module_param(g_dfd_fan_dbg_level, int, S_IRUGO | S_IWUSR);

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

static int dfd_fan_tlv_eeprom_read(int bus, int addr, uint8_t cmd, char *buf)
{
    dfd_dev_head_info_t info;
    char tmp_tlv_len[sizeof(uint16_t)];
    char *tlv_data;
    dfd_dev_tlv_info_t *tlv;
    int buf_len;
    int rv, match_flag;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL\n");
        return -DFD_RV_INVALID_VALUE;
    }

    rv = dfd_ko_i2c_read(bus, addr, 0, (uint8_t *)&info, sizeof(dfd_dev_head_info_t));
    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan may not present.\n");
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    memcpy(tmp_tlv_len, (uint8_t *)&info.tlv_len, sizeof(uint16_t));
    info.tlv_len = (tmp_tlv_len[0] << 8) + tmp_tlv_len[1];

    if ((info.tlv_len <= 0 ) || (info.tlv_len > 0xFF)) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan maybe not set mac.\n");
        return -DFD_RV_TYPE_ERR;
    }
    DFD_FAN_DEBUG(DBG_VERBOSE, "info.tlv_len:%d\n", info.tlv_len);

    tlv_data = (uint8_t *)kmalloc(info.tlv_len, GFP_KERNEL);
    if (tlv_data == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "tlv_data kmalloc failed \n");
        return -DFD_RV_NO_MEMORY;
    }
    memset(tlv_data, 0, info.tlv_len);

    rv = dfd_ko_i2c_read(bus, addr, sizeof(dfd_dev_head_info_t), tlv_data, info.tlv_len);
    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR,"fan eeprom read failed\n");
        kfree(tlv_data);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    buf_len = FAN_SIZE - 1;
    match_flag = 0;
    for(tlv = (dfd_dev_tlv_info_t *)tlv_data; (ulong)tlv < (ulong)tlv_data + info.tlv_len;) {
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
        DFD_FAN_DEBUG(DBG_ERROR,"can't find fan tlv date. bus:%d, addr:0x%02x, tlv type:%d.\n", bus, addr, cmd);
        return -DFD_RV_TYPE_ERR;
    }
    return buf_len;
}

int dfd_get_fan_roll_status(unsigned int fan_index, unsigned int motor_index)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_ROLL_STATUS, fan_index, motor_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan roll status error, fan:%d,motor:%d\n",
            fan_index, motor_index);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan%u motor%u get fan roll status success, status:%d.\n",
        fan_index, motor_index, status);
    return status;
}

int dfd_get_fan_present_status(unsigned int fan_index)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_PRESENT_STATUS, RG_MAIN_DEV_FAN, fan_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan%u get present status error, key:0x%x\n", fan_index, key);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan%u get present status success, status:%d.\n", fan_index, status);
    return status;
}

int dfd_get_fan_status(unsigned int fan_index)
{
    int motor_num, motor_index, status, errcnt;

    status = dfd_get_fan_present_status(fan_index);
    if(status != PRESENT ) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan index:%d, status:%d\n",fan_index, status);
        return status;
    }

    motor_num = dfd_get_dev_number(RG_MAIN_DEV_FAN, RG_MINOR_DEV_MOTOR);
    if(motor_num <= 0 ) {
        DFD_FAN_DEBUG(DBG_ERROR, "get motor number error:%d\n",motor_num);
        return -DFD_RV_DEV_FAIL;
    }
    errcnt = 0;
    for(motor_index = 0; motor_index < motor_num; motor_index++) {
        status = dfd_get_fan_roll_status(fan_index, motor_index);
        if(status < 0) {
            DFD_FAN_DEBUG(DBG_ERROR,  "get fan roll status error, fan index:%d, motor index:%d, status:%d\n",
                fan_index, motor_index, status);
            return status;
        }
        if(status != MOTOR_ROLL) {
            DFD_FAN_DEBUG(DBG_ERROR,
                "stall:fan index:%d, motor index:%d, status:%d\n",fan_index, motor_index, status);
            errcnt++;
        }
    }
    if (errcnt > 0) {
        return FAN_NOT_OK;
    }
    return FAN_OK;
}

ssize_t dfd_get_fan_status_str(unsigned int fan_index, char *buf)
{
    int ret;

    if(buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "params error.fan_index:%d.",fan_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_fan_status(fan_index);
    if(ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan status error,ret:%d, fan_index:%d\n", ret, fan_index);
        return ret;
    }
    memset(buf, 0, PAGE_SIZE);
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

ssize_t dfd_get_fan_info(unsigned int fan_index, uint8_t cmd, char *buf)
{
    int key, rv, eeprom_mode;
    char fan_buf[FAN_SIZE];
    dfd_i2c_dev_t *i2c_dev;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "buf is NULL, fan index:%d, cmd:0x%x.\n", fan_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, PAGE_SIZE);
    memset(fan_buf, 0, FAN_SIZE);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, RG_MAIN_DEV_FAN, fan_index);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan i2c dev config error, key=0x%08x\n", key);
        return -DFD_RV_NODE_FAIL;
    }

    eeprom_mode = dfd_get_fan_eeprom_mode();
    if(eeprom_mode == FAN_EEPROM_MODE_TLV) {
        rv = dfd_fan_tlv_eeprom_read(i2c_dev->bus, i2c_dev->addr, cmd, fan_buf);
    } else {
        rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, cmd, fan_buf, FAN_SIZE);
    }

    if (rv < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan eeprom read failed");
        return -DFD_RV_DEV_FAIL;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "%s\n", fan_buf);
    snprintf(buf, FAN_SIZE, "%s\n", fan_buf);
    return strlen(buf);
}

ssize_t dfd_get_fan_speed(unsigned int fan_index, unsigned int motor_index,unsigned int *speed)
{
    int key, ret, speed_tmp;

    if (speed == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED, fan_index, motor_index);
    ret = dfd_info_get_int(key, &speed_tmp, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan speed error, key:0x%x,ret:%d\n",key, ret);
        return ret;
    }

    if (speed_tmp == 0 || speed_tmp == 0xffff) {
        *speed = 0;
    } else {
        *speed = 15000000 / speed_tmp;
    }
    return DFD_RV_OK;
}

int dfd_set_fan_speed_level(unsigned int fan_index, unsigned int motor_index, int level)
{
    int key, ret;

    if (level < 0 || level > 0xff) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, can not set fan speed level: %d.\n",
            fan_index, motor_index, level);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_RATIO, fan_index, motor_index);
    ret = dfd_info_set_int(key, level);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, set fan level 0x%02x error, key:0x%x,ret:%d\n",
            fan_index, motor_index, level, key, ret);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan:%u, motor:%u, set fan speed level 0x%02x success.\n",
        fan_index, motor_index, level);
    return DFD_RV_OK;
}

int dfd_set_fan_pwm(unsigned int fan_index, unsigned int motor_index, int pwm)
{
    int ret, data;

    if (pwm < 0 || pwm > 100) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, can't set pwm: %d.\n",
            fan_index, motor_index, pwm);
        return -DFD_RV_INVALID_VALUE;
    }

    data = pwm * 255 / 100;
    ret = dfd_set_fan_speed_level(fan_index, motor_index, data);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, set fan ratio:%d error, ret:%d\n",
            fan_index, motor_index, data, ret);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan:%u, motor:%u, set fan ratio %d success.\n",
        fan_index, motor_index, data);
    return DFD_RV_OK;
}

int dfd_get_fan_speed_level(unsigned int fan_index, unsigned int motor_index, int *level)
{
    int key, ret, speed_level;

    if (level == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_RATIO, fan_index, motor_index);
    ret = dfd_info_get_int(key, &speed_level, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, get fan speed level error, key:0x%x,ret:%d\n",
            fan_index, motor_index, key, ret);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan:%u, motor:%u, get fan speed level success, value:0x%02x.\n",
        fan_index, motor_index, speed_level);
    *level = speed_level;
    return DFD_RV_OK;
}

int dfd_get_fan_pwm(unsigned int fan_index, unsigned int motor_index, int *pwm)
{
    int ret, level;

    if (pwm == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_fan_speed_level(fan_index, motor_index, &level);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, get fan pwm error, ret:%d\n",
            fan_index, motor_index, ret);
        return ret;
    }

    if ((level * 100) % 255 > 0) {
        *pwm = level * 100 / 255 + 1;
    } else {
        *pwm = level * 100 / 255;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan:%u, motor:%u, get fan pwm success, value:%d.\n",
        fan_index, motor_index, *pwm);
    return DFD_RV_OK;
}

int dfd_get_fan_speed_tolerance(unsigned int fan_index, unsigned int motor_index, int *value)
{
    int key;
    int *p_fan_speed_tolerance;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED_TOLERANCE, fan_index, motor_index);
    p_fan_speed_tolerance = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_tolerance == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan speed tolerance failed, key:0x%x\n",key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_speed_tolerance;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan speed tolerance ok, key:0x%x, value:%d\n",key, *value);
    return DFD_RV_OK;
}

int dfd_get_fan_speed_target(unsigned int fan_index, unsigned int motor_index, int *value)
{
    int key;
    int *p_fan_speed_target;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED_TARGET, fan_index, motor_index);
    p_fan_speed_target = dfd_ko_cfg_get_item(key);
    if (p_fan_speed_target == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan speed target failed, key:0x%x\n",key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_speed_target;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan speed target ok, key:0x%x, value:%d\n",key, *value);
    return DFD_RV_OK;
}

int dfd_get_fan_direction(unsigned int fan_index, unsigned int motor_index, int *value)
{
    int key;
    int *p_fan_direction;

    if (value == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_DIRECTION, fan_index, motor_index);
    p_fan_direction = dfd_ko_cfg_get_item(key);
    if (p_fan_direction == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan direction failed, key:0x%x\n",key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    *value = *p_fan_direction;
    DFD_FAN_DEBUG(DBG_VERBOSE, "get fan direction ok, key:0x%x, value:%d\n",key, *value);
    return DFD_RV_OK;
}

ssize_t dfd_get_fan_direction_str(unsigned int fan_index, unsigned int motor_index, char *buf)
{
    int ret, value;

    if (buf == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. buf is NULL. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_fan_direction(fan_index, motor_index, &value);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan direction string failed, ret:%d, fan_index:%d, motor_index:%d\n",
            ret, fan_index, motor_index);
        return ret;
    }
    memset(buf, 0, PAGE_SIZE);
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", value);
}
