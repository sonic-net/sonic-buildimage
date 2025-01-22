/*
 * Copyright(C) 2001-2022 Ruijie Network. All rights reserved.
 */
/*
 * rg_psu_driver.c
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

#define PSU_SIZE                         (256)
#define RG_GET_PSU_PMBUS_BUS(addr)       (((addr) >> 24) & 0xff)
#define RG_GET_PSU_PMBUS_ADDR(addr)      (((addr) >> 8) & 0xffff)
#define RG_GET_PSU_PMBUS_OFFSET(addr)    ((addr) & 0xff)

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
} dfd_psu_status_t;

int g_dfd_psu_dbg_level = 0;
module_param(g_dfd_psu_dbg_level, int, S_IRUGO | S_IWUSR);

static char *dfd_get_psu_sysfs_name(void)
{
    int key;
    char *sysfs_name;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_SYSFS_NAME, 0, 0);
    sysfs_name = dfd_ko_cfg_get_item(key);
    if (sysfs_name == NULL) {
        DFD_PSU_DEBUG(DBG_VERBOSE, "key=0x%08x, sysfs_name is NULL, use default way.\n", key);
    } else {
        DFD_PSU_DEBUG(DBG_VERBOSE, "sysfs_name: %s.\n", sysfs_name);
    }
    return sysfs_name;
}

static void dfd_psu_del_no_print_string(char *buf)
{
    int i, len;

    len = strlen(buf);
    for (i = 0; i < len; i++) {
        if ((buf[i] < 0x21) || (buf[i] > 0x7E)) {
            buf[i] = '\0';
            break;
        }
    }
    return ;
}

static int dfd_get_psu_present_status(unsigned int psu_index)
{
    int present_key, present_status;
    int ret;

    present_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_PRESENT_STATUS);
    ret = dfd_info_get_int(present_key, &present_status, NULL);
    if (ret  < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "dfd_get_psu_status error. psu_index: %u, ret: %d\n",
            psu_index, ret);
        return ret;
    }

    return present_status;
}

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
        return -EIO;
    }
    memset(buf, 0, count);
    return (ssize_t)snprintf(buf, count, "%d\n", ret);
}

ssize_t dfd_get_psu_out_status_str(unsigned int psu_index, char *buf, size_t count)
{
    int ret;
    int output_key, output_status;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error, psu_index: %u", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n",
            count, psu_index);
        return -EINVAL;
    }

    output_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_OUTPUT_STATUS);
    ret = dfd_info_get_int(output_key, &output_status, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu out status error, ret: %d, psu_index: %u\n",
            ret, psu_index);
        return -EIO;
    }
    memset(buf, 0, count);
    return (ssize_t)snprintf(buf, count, "%d\n", output_status);
}

ssize_t dfd_get_psu_in_status_str(unsigned int psu_index, char *buf, size_t count)
{
    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error, psu_index: %u", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n",
            count, psu_index);
        return -EINVAL;
    }

    memset(buf, 0 , count);
    return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
}

static int dfd_psu_product_name_decode(int power_type, char *psu_buf, int buf_len)
{
    int key;
    char *p_decode_name;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DECODE_POWER_NAME, power_type, 0);
    p_decode_name = dfd_ko_cfg_get_item(key);
    if (p_decode_name == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "config error, get psu decode name error, key: 0x%x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    memset(psu_buf, 0, buf_len);
    strncpy(psu_buf, p_decode_name, buf_len - 1);
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu name match ok, display psu name: %s\n", psu_buf);
    return DFD_RV_OK;
}

static int dfd_psu_fan_direction_decode(int power_type, char *psu_buf, int buf_len)
{
    int key;
    char *p_decode_direction;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DECODE_POWER_FAN_DIR, power_type, 0);
    p_decode_direction = dfd_ko_cfg_get_item(key);
    if (p_decode_direction == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "config error, get psu decode direction error, key: 0x%x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    memset(psu_buf, 0, buf_len);
    snprintf(psu_buf, buf_len, "%d", *p_decode_direction);
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu%u fan direction match ok, display psu direction: %s\n",
        power_type, psu_buf);
    return DFD_RV_OK;
}

static int dfd_psu_max_output_power(int power_type, char *psu_buf, int buf_len)
{
    int key, value;
    int *p_power_max_output_power;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_POWER_RSUPPLY, power_type, 0);
    p_power_max_output_power = dfd_ko_cfg_get_item(key);
    if (p_power_max_output_power == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "config error, get psu input type error, key: 0x%x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    value = *p_power_max_output_power;
    memset(psu_buf, 0, buf_len);
    snprintf(psu_buf, buf_len, "%d", value);
    DFD_PSU_DEBUG(DBG_VERBOSE, "psu name %s match max output power %d\n", psu_buf, value);
    return DFD_RV_OK;
}

static int dfd_get_psu_type(unsigned int psu_index, dfd_i2c_dev_t *i2c_dev, int *power_type,
               const char *sysfs_name)
{
    int rv;
    char psu_buf[PSU_SIZE];

    rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, DFD_DEV_INFO_TYPE_PART_NUMBER, psu_buf,
             PSU_SIZE, sysfs_name);
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

    DFD_PSU_DEBUG(DBG_VERBOSE, "get psu[%u] bus[%d] addr[0x%x] return power_type[0x%x]\n",
            psu_index, i2c_dev->bus, i2c_dev->addr, *power_type);
    return DFD_RV_OK;
}

ssize_t dfd_get_psu_info(unsigned int psu_index, uint8_t cmd, char *buf, size_t count)
{
    int key, rv;
    char psu_buf[PSU_SIZE];
    dfd_i2c_dev_t *i2c_dev;
    int power_type;
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

    memset(buf, 0, count);
    memset(psu_buf, 0, PSU_SIZE);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, RG_MAIN_DEV_PSU, psu_index);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "psu i2c dev config error, key: 0x%08x\n", key);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }
    sysfs_name = dfd_get_psu_sysfs_name();
    if (cmd == DFD_DEV_INFO_TYPE_PART_NAME) {
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name);
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
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name);
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
        rv = dfd_get_psu_type(psu_index, i2c_dev, &power_type, sysfs_name);
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
    } else {
        rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, cmd, psu_buf, PSU_SIZE, sysfs_name);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu eeprom read failed, rv: %d\n", rv);
            return -EIO;
        }
    }
    snprintf(buf, count, "%s\n", psu_buf);
    return strlen(buf);
}

static int dfd_get_psu_pmbus_id(unsigned int psu_index, uint8_t type, uint32_t *p_psu_pmbus_id)
{
    int key;
    int *p_psu_pmbus_id_buf;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_PMBUS_ID, psu_index, type);
    p_psu_pmbus_id_buf = dfd_ko_cfg_get_item(key);
    if (p_psu_pmbus_id_buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu pmbus id error, key: 0x%x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    *p_psu_pmbus_id = *p_psu_pmbus_id_buf;
    DFD_PSU_DEBUG(DBG_VERBOSE, "get psu pmbus id ok, psu index: %u, id: 0x%x\n",
        psu_index, *p_psu_pmbus_id);
    return DFD_RV_OK;
}

static int dfd_get_psu_pmbus_byte(unsigned int psu_index, uint8_t cmd, uint8_t *buf)
{
    int rv;
    uint8_t bus, offset;
    uint16_t addr;
    uint32_t p_psu_pmbus_id;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u, cmd: 0x%x\n", psu_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    rv = dfd_get_psu_pmbus_id(psu_index, cmd, &p_psu_pmbus_id);
    if (rv < 0) {
        DFD_PSU_DEBUG(DBG_WARN, "get psu%u pmbus %u info failed, rv: %d\n", psu_index, cmd, rv);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    bus = RG_GET_PSU_PMBUS_BUS(p_psu_pmbus_id);
    addr = RG_GET_PSU_PMBUS_ADDR(p_psu_pmbus_id);
    offset = RG_GET_PSU_PMBUS_OFFSET(p_psu_pmbus_id);
    rv = dfd_ko_i2c_read(bus, addr, offset, buf, 1, NULL);
    if (rv < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "read psu%u pmbus %u info(bus: %d, addr: 0x%02x, offset:%d) failed, rv: %d\n",
            psu_index, cmd, bus, addr, offset, rv);
        return -DFD_RV_DEV_FAIL;
    }
    return DFD_RV_OK;
}

ssize_t dfd_get_psu_input_type(unsigned int psu_index, char *buf, size_t count)
{
    int ret;
    uint8_t data;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index: %u\n", psu_index);
        return -EINVAL;
    }
    if (count <= 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf size error, count: %lu, psu index: %u\n", count, psu_index);
        return -EINVAL;
    }

    ret = dfd_get_psu_pmbus_byte(psu_index, PSU_IN_TYPE, &data);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        DFD_PSU_DEBUG(DBG_WARN, "get psu%u pmbus type info don't support\n", psu_index);
        return snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    } else if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus type info failed, ret: %d\n", psu_index, ret);
        return -EIO;
    }

    if (data == DFD_PSU_PMBUS_TYPE_AC) {
        return snprintf(buf, count, "%d\n", DFD_PSU_SYSFS_TYPE_AC);
    } else if (data == DFD_PSU_PMBUS_TYPE_DC) {
        return snprintf(buf, count, "%d\n", DFD_PSU_SYSFS_TYPE_DC);
    }

    DFD_PSU_DEBUG(DBG_ERROR, "get psu%u pmbus type data[%u] unknow, ret: %d\n",
        psu_index, data, ret);
    return -EIO;
}
