/*
 * An wb_psu_driver driver for psu devcie function
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

#define PSU_SIZE                         (256)
#define WB_GET_PSU_PMBUS_BUS(addr)       (((addr) >> 24) & 0xff)
#define WB_GET_PSU_PMBUS_ADDR(addr)      (((addr) >> 8) & 0xffff)
#define WB_GET_PSU_PMBUS_OFFSET(addr)    ((addr) & 0xff)
#define DFD_PSU_FRU_MODE_E2_STRING       "eeprom"
#define DFD_PSU_FRU_MODE_PMBUS_STRING    "pmbus"

#define PSU_PMBUS_POWER_GOOD        BIT(11)

typedef enum dfd_psu_pmbus_type_e {
    DFD_PSU_PMBUS_TYPE_AC      = 1,
    DFD_PSU_PMBUS_TYPE_DC      = 2,
} dfd_psu_pmbus_type_t;

typedef enum dfd_psu_sysfs_type_e {
    DFD_PSU_SYSFS_TYPE_DC      = 0,
    DFD_PSU_SYSFS_TYPE_AC      = 1,
} dfd_psu_sysfs_type_t;

typedef enum dfd_psu_status_e {
    DFD_PSU_PRESENT_STATUS  = 0,
    DFD_PSU_OUTPUT_STATUS   = 1,
    DFD_PSU_ALERT_STATUS    = 2,
    DFD_PSU_INPUT_STATUS    = 3,
} dfd_psu_status_t;

typedef enum dfd_psu_alarm_e {
    DFD_PSU_NOT_OK        = 0,
    DFD_PSU_OK        = 1,
} dfd_psu_alarm_t;

enum knos_alarm {
    PSU_TERMAL_ERROR    = 0x1,
    PSU_FAN_ERROR       = 0x2,
    PSU_VOL_ERROR       = 0x4,
};

typedef enum psu_fru_mode_e {
    PSU_FRU_MODE_E2,         /* eeprom */
    PSU_FRU_MODE_PMBUS,      /*pmbus*/
} fan_eeprom_mode_t;


/* PMBUS STATUS WORD decode */
#define PSU_STATUS_WORD_CML             (1 << 1)
#define PSU_STATUS_WORD_TEMPERATURE     (1 << 2)
#define PSU_STATUS_WORD_VIN_UV          (1 << 3)
#define PSU_STATUS_WORD_IOUT_OC         (1 << 4)
#define PSU_STATUS_WORD_VOUT_OV         (1 << 5)
#define PSU_STATUS_WORD_OFF             (1 << 6)
#define PSU_STATUS_WORD_BUSY            (1 << 7)
#define PSU_STATUS_WORD_FANS            (1 << 10)
#define PSU_STATUS_WORD_POWER_GOOD      (1 << 11)
#define PSU_STATUS_WORD_INPUT           (1 << 13)
#define PSU_STATUS_WORD_IOUT            (1 << 14)
#define PSU_STATUS_WORD_VOUT            (1 << 15)

#define PSU_VOLTAGE_ERR_OFFSET          (PSU_STATUS_WORD_VOUT | PSU_STATUS_WORD_IOUT | \
                                         PSU_STATUS_WORD_INPUT | PSU_STATUS_WORD_POWER_GOOD| \
                                         PSU_STATUS_WORD_OFF | PSU_STATUS_WORD_VOUT_OV| \
                                         PSU_STATUS_WORD_IOUT_OC | PSU_STATUS_WORD_VIN_UV)

int g_dfd_psu_dbg_level = 0;
module_param(g_dfd_psu_dbg_level, int, S_IRUGO | S_IWUSR);

static int dfd_get_psu_fru_mode(void)
{
    uint64_t key;
    int mode;
    char *name;

    /* string Type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_FRU_MODE, 0, 0);
    name = dfd_ko_cfg_get_item(key);
    if (name == NULL) {
        /* The default EEPROM format is returned */
        DFD_PSU_DEBUG(DBG_VERBOSE, "get psu fru mode config fail, key=%s, use default eeprom mode\n",
            key_to_name(DFD_CFG_ITEM_PSU_FRU_MODE));
        return PSU_FRU_MODE_E2;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu fru mode %s.\n", name);
    if (!strncmp(name, DFD_PSU_FRU_MODE_E2_STRING, strlen(DFD_PSU_FRU_MODE_E2_STRING))) {
        mode = PSU_FRU_MODE_E2;
    } else if (!strncmp(name, DFD_PSU_FRU_MODE_PMBUS_STRING, strlen(DFD_PSU_FRU_MODE_PMBUS_STRING))) {
        mode = PSU_FRU_MODE_PMBUS;
    } else {
        /* The default EEPROM format is returned */
        mode = PSU_FRU_MODE_E2;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "psu fru mode %d.\n", mode);
    return mode;
}

static char *dfd_get_psu_sysfs_name(void)
{
    uint64_t key;
    char *sysfs_name;

    /* string Type configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_SYSFS_NAME, 0, 0);
    sysfs_name = dfd_ko_cfg_get_item(key);
    if (sysfs_name == NULL) {
        DFD_PSU_DEBUG(DBG_VERBOSE, "key_name=%s, sysfs_name is NULL, use default way.\n",
            key_to_name(DFD_CFG_ITEM_PSU_SYSFS_NAME));
    } else {
        DFD_PSU_DEBUG(DBG_VERBOSE, "sysfs_name: %s.\n", sysfs_name);
    }
    return sysfs_name;
}

static void dfd_psu_del_no_print_string(char *buf)
{
    int i, len;

    len = strlen(buf);
    /* Culling noncharacter */
    for (i = 0; i < len; i++) {
        if ((buf[i] < 0x21) || (buf[i] > 0x7E)) {
            buf[i] = '\0';
            break;
        }
    }
    return;
}

/**
 * dfd_get_psu_present_status - Obtain the power supply status
 * @index: Number of the power supply, starting from 1
 * return: 0:Not in the position
 *         1:position
 *       : Negative value - Read failed
 */
int dfd_get_psu_present_status(unsigned int psu_index)
{
    int present_key, present_status;
    int ret;

    /* Get presence status */
    present_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_PRESENT_STATUS);
    ret = dfd_info_get_int(present_key, &present_status, NULL);
    if (ret  < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "dfd_get_psu_status error. psu_index: %u, ret: %d\n",
            psu_index, ret);
        return ret;
    }

    return present_status;
}

/**
 * dfd_get_psu_present_status_str - Obtain power status
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Gets the value on the pmbus register of the power supply
 */
ssize_t dfd_get_psu_present_status_str(unsigned int psu_index, char *buf, size_t count)
{
    int ret;
    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error.psu_index: %u.",psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n",
            count, psu_index);
        return -EINVAL;
    }

    ret = dfd_get_psu_present_status(psu_index);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu status error, ret: %d, psu_index: %u\n", ret, psu_index);
        return ret;
    }
    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", ret);
}

ssize_t dfd_get_psu_pmbus_status(unsigned int psu_index, char *buf, size_t count)
{
    int key;
    int ret;

    /* PMBUS STATUS WORD (0x79) */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_PSU, psu_index, PSU_HW_STATUS);
    ret = dfd_info_get_sensor(key, buf, count, NULL);

    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status info failed, key: 0x%08x, ret: %d\n",
            psu_index, DFD_CFG_ITEM_HWMON_PSU, ret);
    } else {
        DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, pmbus_data = %s \n", psu_index, buf);
    }
    return ret;
}

/**
 * dfd_get_psu_hw_status_str - get psu status str
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Gets the value on the pmbus register of the power supply
 */
ssize_t dfd_get_psu_hw_status_str(unsigned int psu_index, char *buf, size_t count)
{
    int ret;
    int status_word;
    int status;
    int output_key, output_status;
    int alert_key, alert_status;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error, psu_index: %u", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n",
            count, psu_index);
        return -EINVAL;
    }

    /* get psu present status first */
    ret = dfd_get_psu_present_status(psu_index);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu present status error, ret: %d, psu_index: %u\n", ret, psu_index);
        return ret;
    }
    if (ret == PSU_STATUS_ABSENT) {
        return (ssize_t)snprintf(buf, count, "%d\n", PSU_STATUS_ABSENT);
    }

    /* get psu alert and power status from cpld */
    output_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_OUTPUT_STATUS);
    alert_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_ALERT_STATUS);
    ret = dfd_info_get_int(output_key, &output_status, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu output_key error, ret: %d, psu_index: %u\n",
            ret, psu_index);
        return ret;
    }
    ret = dfd_info_get_int(alert_key, &alert_status, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu alert_key error, ret: %d, psu_index: %u\n",
            ret, psu_index);
        return ret;
    }
    DFD_PSU_DEBUG(DBG_VERBOSE, "get psu %u alert: %u, output:  %u.\n", psu_index, alert_status, output_status);
    /* if cpld status not ok */
    if (!alert_status || !output_status) {
    /* jduge psu status from psu pmbus 0x79 */
        ret = dfd_get_psu_pmbus_status(psu_index, buf, count);
        if (ret < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "get psu pmbus status error, ret: %d, psu_index: %u\n", ret, psu_index);
            return ret;
        } else {
            ret = kstrtoint(buf, 0, &status_word);
            if (ret) {
                DFD_PSU_DEBUG(DBG_ERROR, "invalid value: %s \n", buf);
                return -EINVAL;
            }
            DFD_PSU_DEBUG(DBG_VERBOSE, "get psu %u statu reg value: %u.\n", psu_index, status_word);
            status = (status_word & PSU_PMBUS_POWER_GOOD) ? PSU_STATUS_FAIL : PSU_STATUS_WARN;
        }
    } else {
        status = PSU_STATUS_PRESENT;
    }

    mem_clear(buf,  count);
    return (ssize_t)snprintf(buf, count, "%d\n", status);
}

/**
 * dfd_get_psu_status_pmbus_str - Gets the value on the pmbus register of the power supply
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_status_pmbus_str(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    int pmbus_data;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    /* Gets the status from the pmbus register of the power supply */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_PMBUS_REG, psu_index, PSU_SENSOR_NONE);
    ret = dfd_info_get_int(key, &pmbus_data, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status info failed, key_name: %s, ret: %d\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_PMBUS_REG), ret);
        return ret;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, pmbus_data = 0x%x \n", psu_index, pmbus_data);

    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", pmbus_data);
}

/**
 * dfd_get_psu_fan_speed_cal_str - Obtain the formula for calculating the speed of the power supply
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
static int dfd_get_psu_fan_speed_cal_str(int power_type, char *psu_buf, int buf_len)
{
    uint64_t key;
    char *speed_cal;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED_CAL, power_type, 0);
    speed_cal = dfd_ko_cfg_get_item(key);
    if (speed_cal == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "config error, get psu speed cal error, key_name: %s\n",
            key_to_name(DFD_CFG_ITEM_FAN_SPEED_CAL));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    mem_clear(psu_buf, buf_len);
    strlcpy(psu_buf, speed_cal, buf_len);
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu speed cal match ok, speed_cal: %s\n", psu_buf);
    return DFD_RV_OK;
}

/**
 * dfd_get_psu_out_status_str - Obtain the output power status
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_out_status_str(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    int pmbus_data;
    int output_status;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    /* Gets the status from the pmbus register of the power supply */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_PMBUS_REG, psu_index, PSU_OUT_STATUS);
    ret = dfd_info_get_int(key, &pmbus_data, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status info failed, key_name: %s, ret: %d\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_PMBUS_REG), ret);
        return ret;
    }

    output_status = DFD_PSU_OK;
    if (pmbus_data & (PSU_STATUS_WORD_INPUT | PSU_STATUS_WORD_OFF | PSU_STATUS_WORD_POWER_GOOD)) {
        /* The judgment logic of no power is consistent with that of Baidu sysfs */
        output_status = DFD_PSU_NOT_OK;
    }
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, pmbus_data = 0x%x \n", psu_index, pmbus_data);

    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", output_status);
}

/**
 * dfd_psu_product_name_decode - Power name conversion
 * @power_type: Power supply type
 * @psu_buf: Power name buffer
 * @buf_len: psu_buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
static int dfd_psu_product_name_decode(int power_type, char *psu_buf, int buf_len)
{
    uint64_t key;
    char *p_decode_name;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DECODE_POWER_NAME, power_type, 0);
    p_decode_name = dfd_ko_cfg_get_item(key);
    if (p_decode_name == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "config error, get psu decode name error, key_name: %s\n",
            key_to_name(DFD_CFG_ITEM_DECODE_POWER_NAME));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    mem_clear(psu_buf, buf_len);
    strlcpy(psu_buf, p_decode_name, buf_len);
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu name match ok, display psu name: %s\n", psu_buf);
    return DFD_RV_OK;
}

/**
 * dfd_psu_fan_direction_decode - Power duct type conversion
 * @power_type: Power supply type
 * @psu_buf: Power name buffer
 * @buf_len: psu_buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
static int dfd_psu_fan_direction_decode(int power_type, char *psu_buf, int buf_len)
{
    uint64_t key;
    char *p_decode_direction;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DECODE_POWER_FAN_DIR, power_type, 0);
    p_decode_direction = dfd_ko_cfg_get_item(key);
    if (p_decode_direction == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "config error, get psu decode direction error, key_name: %s\n",
            key_to_name(DFD_CFG_ITEM_DECODE_POWER_FAN_DIR));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    mem_clear(psu_buf, buf_len);
    snprintf(psu_buf, buf_len, "%d", *p_decode_direction);
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu%u fan direction match ok, display psu direction: %s\n",
        power_type, psu_buf);
    return DFD_RV_OK;
}

/**
 * dfd_psu_max_output_power - Rated power of supply
 * @power_type: Power supply type
 * @psu_buf: Data buffer
 * @buf_len: psu_buf length
 * return: Success :0
 *       : Failed: A negative value is returned
 */
static int dfd_psu_max_output_power(int power_type, char *psu_buf, int buf_len)
{
    uint64_t key;
    int value;
    int *p_power_max_output_power;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_POWER_RSUPPLY, power_type, 0);
    p_power_max_output_power = dfd_ko_cfg_get_item(key);
    if (p_power_max_output_power == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "config error, get psu input type error, key_name: %s\n",
            key_to_name(DFD_CFG_ITEM_POWER_RSUPPLY));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    value = *p_power_max_output_power;
    mem_clear(psu_buf, buf_len);
    snprintf(psu_buf, buf_len, "%d", value);
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu name %s match max output power %d\n", psu_buf, value);
    return DFD_RV_OK;
}

static int dfd_get_psu_fru_pmbus(unsigned int psu_index, uint8_t cmd, char *buf, size_t buf_len)
{
    uint64_t key;
    int rv, len;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_FRU_PMBUS, psu_index, cmd);
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu index: %d, cmd: %d, key_name: %s\n",
        psu_index, cmd, key_to_name(DFD_CFG_ITEM_PSU_FRU_PMBUS));

    rv = dfd_info_get_sensor(key, buf, buf_len, NULL);
    if (rv < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu fru info by pmbus failed, key_name: %s, rv: %d\n",
            key_to_name(DFD_CFG_ITEM_PSU_FRU_PMBUS), rv);
    } else {
        len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
        DFD_PSU_DEBUG(DBG_VERBOSE, "get psu fru info by pmbus success, value: %s\n", buf);
    }
    return rv;
}

static int dfd_get_psu_type(unsigned int psu_index, dfd_i2c_dev_t *i2c_dev, int *power_type,
               const char *sysfs_name, int fru_mode)
{
    int rv;
    char psu_buf[PSU_SIZE];

    mem_clear(psu_buf, sizeof(psu_buf));
    if (fru_mode == PSU_FRU_MODE_PMBUS) {
        rv = dfd_get_psu_fru_pmbus(psu_index, DFD_DEV_INFO_TYPE_PART_NUMBER, psu_buf, PSU_SIZE);
    } else {
        rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, DFD_DEV_INFO_TYPE_PART_NUMBER, psu_buf,
                PSU_SIZE, sysfs_name);
    }

    if (rv < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu type from eeprom read failed, rv: %d\n", rv);
        return -DFD_RV_DEV_FAIL;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "%s\n", psu_buf);
    dfd_psu_del_no_print_string(psu_buf);

    DFD_PSU_DEBUG(DBG_VERBOSE, "dfd_psu_product_name_decode get psu name %s\n", psu_buf);
    rv = dfd_ko_cfg_get_power_type_by_name((char *)psu_buf, power_type);
    if (rv < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get power type by name[%s] fail, rv: %d\n", psu_buf, rv);
        return -DFD_RV_NO_NODE;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "get psu%u return power_type[0x%x]\n", psu_index, *power_type);
    return DFD_RV_OK;
}

/**
 * dfd_get_psu_info - Get Power Information
 * @index: Number of the power supply, starting from 1
 * @cmd: Power supply information Type, power supply name :2, power supply serial number :3, power supply hardware version :5
 * @buf: Receive buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_psu_info(unsigned int psu_index, uint8_t cmd, char *buf, size_t count)
{
    uint64_t key;
    int rv;
    char psu_buf[PSU_SIZE];
    dfd_i2c_dev_t *i2c_dev;
    int power_type;
    int fru_mode;
    const char *sysfs_name;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u, cmd: 0x%x\n", psu_index, cmd);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u, cmd: 0x%x\n",
            count, psu_index, cmd);
        return -EINVAL;
    }

    fru_mode = dfd_get_psu_fru_mode();
    mem_clear(buf, count);
    mem_clear(psu_buf, PSU_SIZE);
    if (fru_mode == PSU_FRU_MODE_E2) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, WB_MAIN_DEV_PSU, psu_index);
        i2c_dev = dfd_ko_cfg_get_item(key);
        if (i2c_dev == NULL) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu i2c dev config error, key_name: %s\n",
                key_to_name(DFD_CFG_ITEM_OTHER_I2C_DEV));
            return -DFD_RV_DEV_NOTSUPPORT;
        }
        sysfs_name = dfd_get_psu_sysfs_name();
    }

    /* Power E2 product name conversion */
    if (cmd == DFD_DEV_INFO_TYPE_PART_NAME) {
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name, fru_mode);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu get type error, rv: %d\n", rv);
            return -EIO;
        }
        rv = dfd_psu_product_name_decode(power_type, psu_buf, PSU_SIZE);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu name decode error, power_type[0x%x] rv: %d\n",
                power_type, rv);
            return -EIO;
        }
    } else if (cmd == DFD_DEV_INFO_TYPE_FAN_DIRECTION) {
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name, fru_mode);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu get type error, rv: %d\n", rv);
            return -EIO;
        }
        rv = dfd_psu_fan_direction_decode(power_type, psu_buf, PSU_SIZE);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu input type decode error, power_type[0x%x] rv: %d\n",
                power_type, rv);
            return -EIO;
        }
    } else if (cmd == DFD_DEV_INFO_TYPE_MAX_OUTPUT_POWRER) {
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name, fru_mode);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu get type error, rv:%d\n", rv);
            return -EIO;
        }
        rv = dfd_psu_max_output_power(power_type, psu_buf, PSU_SIZE);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu max ouput power error, power_type[0x%x] rv: %d\n",
                power_type, rv);
            return -EIO;
        }
    } else if (cmd == DFD_DEV_INFO_TYPE_SPEED_CAL) {
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name, fru_mode);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu get type error, rv:%d\n", rv);
            return -EIO;
        }
        rv = dfd_get_psu_fan_speed_cal_str(power_type, psu_buf, PSU_SIZE);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu fan speed cal error, power_type[0x%x] rv: %d\n",
                power_type, rv);
            return -EIO;
        }
    } else {
        if (fru_mode == PSU_FRU_MODE_PMBUS) {
            rv = dfd_get_psu_fru_pmbus(psu_index, cmd, psu_buf, PSU_SIZE);
        } else {
            rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, cmd, psu_buf, PSU_SIZE, sysfs_name);
        }
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu eeprom read failed, rv: %d\n", rv);
            return -EIO;
        }
    }

    snprintf(buf, count, "%s\n", psu_buf);
    return strlen(buf);
}

/**
 * dfd_get_psu_input_type - Obtain the power input type
 * @index: Number of the power supply, starting from 1
 * @buf: Receive buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_psu_input_type(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    int data;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_PMBUS_REG, psu_index, PSU_IN_TYPE);
    ret = dfd_info_get_int(key, &data, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status info failed, key_name: %s, ret: %d\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_PMBUS_REG), ret);
        return ret;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, pmbus_data = 0x%x \n", psu_index, data);

    if (data == DFD_PSU_PMBUS_TYPE_AC) {
        return snprintf(buf, count, "%d\n", DFD_PSU_SYSFS_TYPE_AC);
    } else if (data == DFD_PSU_PMBUS_TYPE_DC) {
        return snprintf(buf, count, "%d\n", DFD_PSU_SYSFS_TYPE_DC);
    } else {
        DFD_PSU_DEBUG(DBG_WARN, "get psu%u input type data[%u] unknow, ret: %d\n",
                psu_index, data, ret);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus type data[%u] unknow, ret: %d\n",
        psu_index, data, ret);
    return -EIO;
}

/**
 * dfd_get_psu_in_status_str - Obtain the input power status
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_in_status_str(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    int pmbus_data;
    int input_status;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_PMBUS_REG, psu_index, PSU_IN_STATUS);
    ret = dfd_info_get_int(key, &pmbus_data, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status info failed, key_name: %s, ret: %d\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_PMBUS_REG), ret);
        return ret;
    }

    input_status = DFD_PSU_OK;
    if (pmbus_data & PSU_STATUS_WORD_INPUT) {
        /* no power judgment logic, according to the opinion only bit13 judgment */
        DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, no power, pmbus_data = 0x%x \n", psu_index, pmbus_data);
        input_status = DFD_PSU_NOT_OK;
    }
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, pmbus_data = 0x%x \n", psu_index, pmbus_data);

    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", input_status);
}

ssize_t dfd_get_psu_alarm_status(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    int pmbus_data;
    int alarm;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    /* PMBUS STATUS WORD (0x79) */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_PMBUS_REG, psu_index, PSU_OUT_STATUS);
    ret = dfd_info_get_int(key, &pmbus_data, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status info failed, key_name: %s, ret: %d\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_PMBUS_REG), ret);
        return ret;
    }

    alarm = 0;
    if (pmbus_data & PSU_STATUS_WORD_TEMPERATURE) {
        DFD_PSU_DEBUG(DBG_VERBOSE, "psu%d PSU_TERMAL_ERROR, pmbus_data = 0x%x \n", psu_index, pmbus_data);
        alarm |= PSU_TERMAL_ERROR;
    }

    if (pmbus_data & PSU_STATUS_WORD_FANS) {
        DFD_PSU_DEBUG(DBG_VERBOSE, "psu%d PSU_FAN_ERROR, pmbus_data = 0x%x \n", psu_index, pmbus_data);
        alarm |= PSU_FAN_ERROR;
    }

    if (pmbus_data & PSU_VOLTAGE_ERR_OFFSET) {
        DFD_PSU_DEBUG(DBG_VERBOSE, "psu%d PSU_VOL_ERROR, pmbus_data = 0x%x \n", psu_index, pmbus_data);
        alarm |= PSU_VOL_ERROR;
    }
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, pmbus_data = 0x%x \n", psu_index, pmbus_data);

    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", alarm);
}

/**
 * dfd_get_psu_fan_ratio_str - Gets the target fan rotation rate
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_fan_ratio_str(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    int pmbus_data;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    /* Gets the status from the pmbus register of the power supply */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_PMBUS_REG, psu_index, PSU_FAN_RATIO);
    ret = dfd_info_get_int(key, &pmbus_data, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus fan ratio info failed, key_name: %s, ret: %d\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_PMBUS_REG), ret);
        return ret;
    }

    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "%d\n", pmbus_data);
}

ssize_t dfd_get_psu_threshold_str(unsigned int psu_index, unsigned int type, char *buf, size_t count)
{
    uint64_t key;
    int ret;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }
    key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_PSU, psu_index, type);
    ret = dfd_info_get_sensor(key, buf, count, NULL);
    if (ret < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get psu sensor info error, key_name: %s, ret: %d\n",
            key_to_name(DFD_CFG_ITEM_HWMON_PSU), ret);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "get psu sensor info success, value: %s\n", buf);
    }
    return ret;
}

ssize_t dfd_get_psu_blackbox(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    ssize_t rd_len;
    char *blackbox_path;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error.psu_index: %u.",psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n",
            count, psu_index);
        return -EINVAL;
    }

    /* Obtain the blackbox_info path*/
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_BLACKBOX_INFO, psu_index, 0);
    blackbox_path = dfd_ko_cfg_get_item(key);
    if (blackbox_path == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u blackbox_info path error, key_name: %s\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_BLACKBOX_INFO));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, blackbox_info path: %s\n", psu_index, blackbox_path);

    mem_clear(buf,  count);
    rd_len = dfd_ko_read_file(blackbox_path, 0, buf, count);
    if (rd_len < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "read psu%u blackbox info failed, blackbox_info path: %s, ret: %ld\n",
            psu_index, blackbox_path, rd_len);
    } else {
        DFD_PSU_DEBUG(DBG_VERBOSE, "read psu%u blackbox info success, blackbox_info path: %s, rd_len: %ld\n",
            psu_index, blackbox_path, rd_len);
    }

    return rd_len;
}

ssize_t dfd_get_psu_pmbus(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    ssize_t rd_len;
    char *pmbus_info_path;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error.psu_index: %u.",psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n",
            count, psu_index);
        return -EINVAL;
    }

    /* Obtain the pmbus_info path*/
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_PMBUS_INFO, psu_index, 0);
    pmbus_info_path = dfd_ko_cfg_get_item(key);
    if (pmbus_info_path == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus_info path error, key_name: %s\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_PMBUS_INFO));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, pmbus_info path: %s\n", psu_index, pmbus_info_path);

    mem_clear(buf,  count);
    rd_len = dfd_ko_read_file(pmbus_info_path, 0, buf, count);
    if (rd_len < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "read psu%u pmbus info failed, pmbus_info path: %s, ret: %ld\n",
            psu_index, pmbus_info_path, rd_len);
    } else {
        DFD_PSU_DEBUG(DBG_VERBOSE, "read psu%u pmbus info success, pmbus_info path: %s, rd_len: %ld\n",
            psu_index, pmbus_info_path, rd_len);
    }

    return rd_len;
}

int dfd_clear_psu_blackbox(unsigned int psu_index, uint8_t value)
{
    uint64_t key;
    int ret;
    char *clear_blackbox_info_path;
    uint8_t wr_buf[INFO_INT_MAX_LEN];

    /* Obtain the clear_blackbox path*/
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_CLEAR_BLACKBOX, psu_index, 0);
    clear_blackbox_info_path = dfd_ko_cfg_get_item(key);
    if (clear_blackbox_info_path == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u clear blackbox path error, key_name: %s\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_CLEAR_BLACKBOX));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, clear blackbox path: %s, write value: %u\n",
        psu_index, clear_blackbox_info_path, value);

    mem_clear(wr_buf, sizeof(wr_buf));
    snprintf(wr_buf, sizeof(wr_buf), "%u", value);
    ret = dfd_ko_write_file(clear_blackbox_info_path, 0, wr_buf, strlen(wr_buf));
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "clear psu%u blackbox info failed, ret: %d\n", psu_index, ret);
        return ret;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, clear blackbox info success\n", psu_index);
    return DFD_RV_OK;
}

