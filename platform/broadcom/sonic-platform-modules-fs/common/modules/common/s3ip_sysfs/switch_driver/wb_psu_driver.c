/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_psu_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * Power related properties read and write functions
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"
#include "dfd_frueeprom.h"
#include "dfd_cfg_file.h"

#define PSU_SIZE                         (256)
#define WB_GET_PSU_PMBUS_BUS(addr)       (((addr) >> 16) & 0xffff)
#define WB_GET_PSU_PMBUS_ADDR(addr)      ((addr) & 0xffff)
#define DFD_PSU_FRU_MODE_E2_STRING       "eeprom"
#define DFD_PSU_FRU_MODE_PMBUS_STRING    "pmbus"

#define PSU_PMBUS_POWER_GOOD          BIT(11)
#define PSU_PMBUS_TEMPERATURE         BIT(2)
#define PSU_PMBUS_IOUT_OC             BIT(4)
#define PSU_PMBUS_FAN                 BIT(10)
#define PSU_PMBUS_INPUT               BIT(13)
#define PSU_PMBUS_IOUT_POUT           BIT(14)
#define PSU_PMBUS_VOUT                BIT(15)

typedef enum psu_status_e {
    PSU_STATUS_ABSENT               = 0, /* psu absent */
    PSU_STATUS_OK                   = 1, /* psu present and status ok */
    PSU_STATUS_WARN                 = 2, /* psu present and status warn (pmbus 0x79 bit11 value 0)*/
    PSU_STATUS_OUTPUT_FAIL          = 3, /* psu present and status fail (pmbus 0x79 bit11 value 1 and bit13 value 0)*/
    PSU_STATUS_INPUT_OUTPUT_FAIL    = 4, /* psu present and status fail (pmbus 0x79 bit11 value 1 and bit13 value 1)*/
} psu_status_t;

typedef enum psu_status_word_e {
    PSU_VOUT_FAULT = 0x8000,
    PSU_IOUT_FAULT = 0x4000,
    PSU_INPUT_FAULT = 0x2000,
    PSU_MFR_FAULT = 0x1000,
    PSU_PG_FAULT = 0x0800,
    PSU_FAN_FAULT = 0x0400,
    PSU_OFF_FAULT = 0x0040,
    PSU_TEMP_FAULT = 0x0004,
} psu_status_word_t;

typedef struct {
    const char *sysfs_name;
    const char *attr_name;
    uint32_t mask;
} psu_pmbus_status_extra_info_t;

typedef struct {
    const char *attr_name;
    uint32_t bit_offset;
    const psu_pmbus_status_extra_info_t *extra_info;
    size_t extra_info_len;
} psu_pmbus_status_info_t;

static const psu_pmbus_status_extra_info_t psu_pmbus_input_fault_infos[] = {
    {
        .sysfs_name = PMBUS_STATUS_INPUT_SYSFS,
        .attr_name = "INPUT_UNDER_FAULT",
        .mask = (BIT(3) | BIT(4)),
    },
    {
        .sysfs_name = PMBUS_STATUS_INPUT_SYSFS,
        .attr_name = "INPUT_OVER_FAULT",
        .mask = BIT(7),
    },
};

static const psu_pmbus_status_extra_info_t psu_pmbus_output_fault_infos[] = {
    {
        .sysfs_name = PMBUS_STATUS_VOUT_SYSFS,
        .attr_name = "VOUT_OVER_FAULT",
        .mask = BIT(7),
    },
    {
        .sysfs_name = PMBUS_STATUS_VOUT_SYSFS,
        .attr_name = "VOUT_UNDER_FAULT",
        .mask = BIT(4),
    },
};

static const psu_pmbus_status_info_t psu_pmbus_status_fault_infos[] = {
    {
        .attr_name = "TEMPERATURE_FAULT",
        .bit_offset = PSU_PMBUS_TEMPERATURE,
        .extra_info = NULL,
        .extra_info_len = 0
    },
    {
        .attr_name = "IOUT_OC_FAULT",
        .bit_offset = PSU_PMBUS_IOUT_OC,
        .extra_info = NULL,
        .extra_info_len = 0
    },
    {
        .attr_name = "FAN_FAULT",
        .bit_offset = PSU_PMBUS_FAN,
        .extra_info = NULL,
        .extra_info_len = 0
    },
    {
        .attr_name = "INPUT_FAULT",
        .bit_offset = PSU_PMBUS_INPUT,
        .extra_info = psu_pmbus_input_fault_infos,
        .extra_info_len = ARRAY_SIZE(psu_pmbus_input_fault_infos)
    },
    {
        .attr_name = "VOUT_FAULT",
        .bit_offset = PSU_PMBUS_VOUT,
        .extra_info = psu_pmbus_output_fault_infos,
        .extra_info_len = ARRAY_SIZE(psu_pmbus_output_fault_infos)
    },
};

static const psu_pmbus_status_extra_info_t psu_pmbus_input_warn_infos[] = {
    {
        .sysfs_name = PMBUS_STATUS_INPUT_SYSFS,
        .attr_name = "INPUT_UNDER_WARN",
        .mask = BIT(5),
    },
    {
        .sysfs_name = PMBUS_STATUS_INPUT_SYSFS,
        .attr_name = "INPUT_OVER_WARN",
        .mask = BIT(6),
    },
};

static const psu_pmbus_status_info_t psu_pmbus_status_warn_infos[] = {
    {
        .attr_name = "TEMPERATURE_WARN",
        .bit_offset = PSU_PMBUS_TEMPERATURE,
        .extra_info = NULL,
        .extra_info_len = 0
    },
    {
        .attr_name = "FAN_WARN",
        .bit_offset = PSU_PMBUS_FAN,
        .extra_info = NULL,
        .extra_info_len = 0
    },
    {
        .attr_name = "INPUT_WARN",
        .bit_offset = PSU_PMBUS_INPUT,
        .extra_info = psu_pmbus_input_warn_infos,
        .extra_info_len = ARRAY_SIZE(psu_pmbus_input_warn_infos)
    },
    {
        .attr_name = "IOUT_POUT_WARN",
        .bit_offset = PSU_PMBUS_IOUT_POUT,
        .extra_info = NULL,
        .extra_info_len = 0
    },
};

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
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n",
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

static ssize_t dfd_get_psu_pmbus_path(unsigned int psu_index, char *sysfs_path, int size)
{
    uint16_t addr;
    uint16_t bus;
    int key;
    uint32_t *psu_pmbus_addr;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_PMBUS_ADDR, psu_index, 0);
    psu_pmbus_addr = dfd_ko_cfg_get_item(key);
    if (psu_pmbus_addr == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu pmbus addr error, key: 0x%x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    addr = WB_GET_PSU_PMBUS_ADDR(*psu_pmbus_addr);
    bus = WB_GET_PSU_PMBUS_BUS(*psu_pmbus_addr);

    snprintf(sysfs_path, size, "/sys/bus/i2c/devices/%d-%04x/", bus, addr);
    return 0;
}

static ssize_t dfd_get_psu_pmbus_val(unsigned int psu_index, int *val, const char *sysfs_name)
{
    int ret, rd_len;
    char psu_pmbus_path[DFD_SYSFS_PATH_MAX_LEN];
    char sysfs_path[DFD_SYSFS_PATH_MAX_LEN];
    char tmp_buf[INFO_BUF_MAX_LEN];

    mem_clear(psu_pmbus_path, sizeof(psu_pmbus_path));
    ret = dfd_get_psu_pmbus_path(psu_index, psu_pmbus_path, sizeof(psu_pmbus_path));
    if (ret < 0) {
        return ret;
    }

    /* get real sysfs */
    mem_clear(sysfs_path, sizeof(sysfs_path));
    snprintf(sysfs_path, sizeof(sysfs_path), "%s%s", psu_pmbus_path, sysfs_name);

    /* get sysfs value */
    mem_clear(tmp_buf, sizeof(tmp_buf));
    rd_len = dfd_ko_read_file(sysfs_path, 0, tmp_buf, sizeof(tmp_buf));
    if (rd_len < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "read psu%d pmbus val failed, pmbus_info path: %s, ret: %d\n",
            psu_index, sysfs_path, rd_len);
        return rd_len;
    } else {
        DFD_PSU_DEBUG(DBG_VERBOSE, "read psu%u pmbus val success, pmbus_info path: %s, rd_len: %d\n",
            psu_index, sysfs_path, rd_len);
    }

    ret = kstrtoint(tmp_buf, 0, val);
    if (ret != 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "invaild psu pmbus val ret: %d, buf: %s.\n", ret, tmp_buf);
        return -EINVAL;
    }

    return 0;
}

static ssize_t dfd_get_psu_out_and_alert_status(unsigned int psu_index, int *output_status, int *alert_status)
{
    int ret;
    int output_key, alert_key;

    output_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_OUTPUT_STATUS);
    alert_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_ALERT_STATUS);
    ret = dfd_info_get_int(output_key, output_status, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu output_key error, ret: %d, psu_index: %u\n",
            ret, psu_index);
        return ret;
    }
    ret = dfd_info_get_int(alert_key, alert_status, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu alert_key error, ret: %d, psu_index: %u\n",
            ret, psu_index);
        return ret;
    }
    DFD_PSU_DEBUG(DBG_VERBOSE, "get psu %u alert: %u, output:  %u.\n", psu_index, *alert_status, *output_status);
    return 0;
}


/**
 * dfd_get_psu_status_str - get psu status str
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Gets the value on the pmbus register of the power supply
 */
ssize_t dfd_get_psu_status_str(unsigned int psu_index, char *buf, size_t count)
{
    int ret;
    int status_word;
    int status;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error, psu_index: %u", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n",
            count, psu_index);
        return -EINVAL;
    }

    status_word = 0;
    ret = dfd_get_psu_pmbus_val(psu_index, &status_word, PMBUS_STATUS_WORD_SYSFS);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status failed, ret: %d\n", psu_index, ret);
        return ret;
    }

    status = 0;
    mem_clear(buf, count);
    if (status_word < 0) {
        return status_word;
    } else {
        status = (status_word & PSU_OFF_FAULT) ? (status | 0x02) : status;
        status = (status_word & PSU_FAN_FAULT) ? (status | 0x04) : status;
        status = (status_word & PSU_VOUT_FAULT) ? (status | 0x08) : status;
        status = (status_word & PSU_IOUT_FAULT) ? (status | 0x10) : status;
        status = (status_word & PSU_INPUT_FAULT) ? (status | 0x20) : status;
        status = (status_word & PSU_TEMP_FAULT) ? (status | 0x40) : status;
    }
    return (ssize_t)snprintf(buf, count, "0x%x\n", status);
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
    int alert_status, output_status;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error, psu_index: %u", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n",
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
    ret = dfd_get_psu_out_and_alert_status(psu_index, &output_status, &alert_status);
    if (ret < 0) {
        return ret;
    }

    /* if cpld status not ok */
    if (alert_status || !output_status) {
    /* jduge psu status from psu pmbus 0x79 */
        status_word = 0;
        ret = dfd_get_psu_pmbus_val(psu_index, &status_word, PMBUS_STATUS_WORD_SYSFS);
        if (ret < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "get psu pmbus status error, ret: %d, psu_index: %u\n", ret, psu_index);
            return ret;
        }

        DFD_PSU_DEBUG(DBG_VERBOSE, "get psu %u statu reg value: %u.\n", psu_index, status_word);
        if (status_word & PSU_PMBUS_POWER_GOOD) {
            if (status_word & PSU_PMBUS_INPUT) {
                status = PSU_STATUS_INPUT_OUTPUT_FAIL;
            } else {
                status = PSU_STATUS_OUTPUT_FAIL;
            }
        } else {
            status = PSU_STATUS_WARN;
        }
    } else {
        status = PSU_STATUS_OK;
    }

    mem_clear(buf,  count);
    return (ssize_t)snprintf(buf, count, "%d\n", status);
}

/**
 * dfd_get_psu_hw_detail_status_str - get psu detail status str
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Gets the value on the pmbus register of the power supply
 */
ssize_t dfd_get_psu_hw_detail_status_str(unsigned int psu_index, char *buf, size_t count)
{
    int ret, i, j, len;
    int status_word;
    int tmp_val;
    const psu_pmbus_status_info_t *status_infos = NULL;
    size_t status_infos_size = 0;
    int alert_status, output_status;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error, psu_index: %u", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n",
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
        return (ssize_t)snprintf(buf, count, "ABSENT\n");
    }

    /* get psu alert and power status from cpld */
    ret = dfd_get_psu_out_and_alert_status(psu_index, &output_status, &alert_status);
    if (ret < 0) {
        return ret;
    }

    /* if cpld status ok */
    if (!alert_status && output_status) {
        return (ssize_t)snprintf(buf, count, "OK\n");
    }

    status_word = 0;
    ret = dfd_get_psu_pmbus_val(psu_index, &status_word, PMBUS_STATUS_WORD_SYSFS);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu pmbus status error, ret: %d, psu_index: %u\n", ret, psu_index);
        return ret;
    }

    if (status_word & PSU_PMBUS_POWER_GOOD) {
        status_infos = psu_pmbus_status_fault_infos;
        status_infos_size = ARRAY_SIZE(psu_pmbus_status_fault_infos);
    } else {
        status_infos = psu_pmbus_status_warn_infos;
        status_infos_size = ARRAY_SIZE(psu_pmbus_status_warn_infos);
    }

    len = 0;
    for (i = 0; i < status_infos_size; i++) {
        if (status_word & status_infos[i].bit_offset) {
            len += scnprintf(buf + len, count - len, "%s\n", status_infos[i].attr_name);
            for (j = 0; j < status_infos[i].extra_info_len; j++) {
                tmp_val = 0;
                ret = dfd_get_psu_pmbus_val(psu_index, &tmp_val, status_infos[i].extra_info[j].sysfs_name);
                if (ret < 0) {
                    DFD_PSU_DEBUG(DBG_ERROR, "get psu pmbus sysfs error, ret: %d, psu_index: %u, sysfs_name:%s\n", \
                        ret, psu_index, status_infos[i].extra_info[j].sysfs_name);
                    return ret;
                }
                if (tmp_val & status_infos[i].extra_info[j].mask) {
                    len += scnprintf(buf + len, count - len, "  %s\n", status_infos[i].extra_info[j].attr_name);
                }
            }
        }
    }

    return len;
}

/**
 * dfd_get_psu_status_pmbus_str - Gets the value on the pmbus register of the power supply
 * @index: Number of the power supply, starting from 1
 * return: Success: Length of the status string
 *       : Negative value - Read failed
 */
ssize_t dfd_get_psu_status_pmbus_str(unsigned int psu_index, char *buf, size_t count)
{
    int ret;
    int pmbus_data;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    /* Gets the status from the pmbus register of the power supply */
    pmbus_data = 0;
    ret = dfd_get_psu_pmbus_val(psu_index, &pmbus_data, PMBUS_STATUS_WORD_SYSFS);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status failed,  ret: %d\n", psu_index, ret);
        return ret;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, pmbus_data = 0x%x \n", psu_index, pmbus_data);

    mem_clear(buf, count);
    return (ssize_t)snprintf(buf, count, "0x%x\n", pmbus_data);
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
    int ret;
    int pmbus_data;
    int output_status;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    /* Gets the status from the pmbus register of the power supply */
    pmbus_data = 0;
    ret = dfd_get_psu_pmbus_val(psu_index, &pmbus_data, PMBUS_STATUS_WORD_SYSFS);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status failed, ret: %d\n", psu_index, ret);
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
    dfd_info_del_no_print_string(psu_buf);

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
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u, cmd: 0x%x\n",
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
            return rv;
        }
        rv = dfd_psu_product_name_decode(power_type, psu_buf, PSU_SIZE);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu name decode error, power_type[0x%x] rv: %d\n",
                power_type, rv);
            return rv;
        }
    } else if (cmd == DFD_DEV_INFO_TYPE_FAN_DIRECTION) {
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name, fru_mode);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu get type error, rv: %d\n", rv);
            return rv;
        }
        rv = dfd_psu_fan_direction_decode(power_type, psu_buf, PSU_SIZE);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu input type decode error, power_type[0x%x] rv: %d\n",
                power_type, rv);
            return rv;
        }
    } else if (cmd == DFD_DEV_INFO_TYPE_MAX_OUTPUT_POWRER) {
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name, fru_mode);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu get type error, rv:%d\n", rv);
            return rv;
        }
        rv = dfd_psu_max_output_power(power_type, psu_buf, PSU_SIZE);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu max ouput power error, power_type[0x%x] rv: %d\n",
                power_type, rv);
            return rv;
        }
    } else if (cmd == DFD_DEV_INFO_TYPE_SPEED_CAL) {
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name, fru_mode);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu get type error, rv:%d\n", rv);
            return rv;
        }
        rv = dfd_get_psu_fan_speed_cal_str(power_type, psu_buf, PSU_SIZE);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu fan speed cal error, power_type[0x%x] rv: %d\n",
                power_type, rv);
            return rv;
        }
    } else {
        if (fru_mode == PSU_FRU_MODE_PMBUS) {
            rv = dfd_get_psu_fru_pmbus(psu_index, cmd, psu_buf, PSU_SIZE);
        } else {
            rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, cmd, psu_buf, PSU_SIZE, sysfs_name);
        }
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu eeprom read failed, rv: %d\n", rv);
            return rv;
        }
    }

    /* sn and part_number maybe include noncharacter */
    if (cmd == DFD_DEV_INFO_TYPE_PART_NUMBER ||
        cmd == DFD_DEV_INFO_TYPE_SN ||
        cmd == DFD_DEV_INFO_TYPE_VENDOR) {
        dfd_ko_trim_trailing_spaces(psu_buf);
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
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n", count, psu_index);
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
    int ret;
    int pmbus_data;
    int input_status;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    pmbus_data = 0;
    ret = dfd_get_psu_pmbus_val(psu_index, &pmbus_data, PMBUS_STATUS_WORD_SYSFS);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status failed, ret: %d\n", psu_index, ret);
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
    int ret;
    int pmbus_data;
    int alarm;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    /* PMBUS STATUS WORD (0x79) */
    pmbus_data = 0;
    ret = dfd_get_psu_pmbus_val(psu_index, &pmbus_data, PMBUS_STATUS_WORD_SYSFS);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus status failed, ret: %d\n", psu_index, ret);
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
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n", count, psu_index);
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
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n", count, psu_index);
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
    char *blackbox_path;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error.psu_index: %u.",psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n",
            count, psu_index);
        return -EINVAL;
    }

    /* Obtain the blackbox_info path*/
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_BLACKBOX_PATH, psu_index, 0);
    blackbox_path = dfd_ko_cfg_get_item(key);
    if (blackbox_path == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u blackbox_info path error, key_name: %s\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_BLACKBOX_PATH));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, blackbox_info path: %s\n", psu_index, blackbox_path);

    mem_clear(buf,  count);
    return (ssize_t)snprintf(buf, count, "%s\n", blackbox_path);
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
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n",
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
        DFD_PSU_DEBUG(DBG_ERROR, "read psu%u pmbus info failed, pmbus_info path: %s, ret: %zd\n",
            psu_index, pmbus_info_path, rd_len);
    } else {
        DFD_PSU_DEBUG(DBG_VERBOSE, "read psu%u pmbus info success, pmbus_info path: %s, rd_len: %zd\n",
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

ssize_t dfd_get_psu_support_upgrade_func(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    int psu_support_upgrade;
    int ret;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "param error, buf is NULL, psu index: %u\n",
            psu_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n",
            count, psu_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    psu_support_upgrade = 0;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_SUPPORT_UPGRADE, psu_index, 0);
    ret = dfd_info_get_int(key, &psu_support_upgrade, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "psu%u support upgrade config error, key_name: %s\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_SUPPORT_UPGRADE));
        return ret;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "%d\n", psu_support_upgrade);
    snprintf(buf, count, "%d\n", psu_support_upgrade);
    return strlen(buf);
}

ssize_t dfd_get_psu_upgrade_active_type_func(unsigned int psu_index, char *buf, size_t count)
{
    uint64_t key;
    char *psu_upgrade_active_type;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "param error, buf is NULL, psu index: %u\n",
            psu_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %zu, psu index: %u\n",
            count, psu_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_UPGRADE_ACTIVE_TYPE, psu_index, 0);
    psu_upgrade_active_type = dfd_ko_cfg_get_item(key);
    if (psu_upgrade_active_type == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "psu%u support upgrade config error, key_name: %s\n",
            psu_index, key_to_name(DFD_CFG_ITEM_PSU_UPGRADE_ACTIVE_TYPE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "%s\n", psu_upgrade_active_type);
    snprintf(buf, count, "%s\n", psu_upgrade_active_type);
    return strlen(buf);
}

int dfd_set_psu_reset_func(unsigned int psu_index, uint8_t value)
{
    uint64_t key;
    int ret;
    char *psu_reset_info_path;
    uint8_t wr_buf[INFO_INT_MAX_LEN];

    /* get current step cfg */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_RESET, psu_index, 0);
    psu_reset_info_path = dfd_ko_cfg_get_item(key);
    if (psu_reset_info_path == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu_reset_info_path fail, key_name=%s, psu_index=0x%x\n",
            key_to_name(DFD_CFG_ITEM_PSU_RESET), psu_index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, psu reset path: %s, write value: %u\n",
        psu_index, psu_reset_info_path, value);

    mem_clear(wr_buf, sizeof(wr_buf));
    snprintf(wr_buf, sizeof(wr_buf), "%u", value);
    ret = dfd_ko_write_file(psu_reset_info_path, 0, wr_buf, strlen(wr_buf));
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "set_psu_reset_func value error, key_name=%s, psu%u, value=%d, ret:%d\n",
            key_to_name(DFD_CFG_ITEM_PSU_RESET), psu_index, value, ret);
        return ret;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "psu_index: %u, psu reset info success\n", psu_index);

    return DFD_RV_OK;
}

