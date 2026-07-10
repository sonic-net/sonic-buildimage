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

#define PSU_SIZE                         (256)

typedef enum dfd_psu_status_e {
    DFD_PSU_PRESENT_STATUS  = 0,
    DFD_PSU_OUTPUT_STATUS   = 1,
    DFD_PSU_ALERT_STATUS    = 2,
} dfd_psu_status_t;

typedef enum dfd_psu_present_status_e {
    ABSENT  = 0,
    PRESENT = 1,
} dfd_psu_present_status_t;

typedef enum dfd_psu_output_status_e {
    FAULT = 0,
    NORMAL = 1,
} dfd_psu_output_status_t;

int g_dfd_psu_dbg_level = 0;
module_param(g_dfd_psu_dbg_level, int, S_IRUGO | S_IWUSR);

int dfd_get_psu_present_status(unsigned int psu_index)
{
    int ret, present_key, present_status;

    present_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_PRESENT_STATUS);
    ret = dfd_info_get_int(present_key, &present_status, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "dfd_get_psu_present_status error. psu_index:%d, ret:%d\n",
            psu_index, ret);
        return ret;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "dfd_get_psu_present_status success. psu_index:%d, status:%d\n",
        psu_index, present_status);
    return present_status;
}

int dfd_get_psu_output_status(unsigned int psu_index)
{
    int ret, output_key, output_status;

    output_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_OUTPUT_STATUS);
    ret = dfd_info_get_int(output_key, &output_status, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "dfd_get_psu_output_status error. psu_index:%d, ret:%d\n",
            psu_index, ret);
        return ret;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "dfd_get_psu_output_status success. psu_index:%d, status:%d\n",
        psu_index, output_status);
    return output_status;
}

int dfd_get_psu_alert_status(unsigned int psu_index)
{
    int ret, alert_key, alert_status;

    alert_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_ALERT_STATUS);
    ret = dfd_info_get_int(alert_key, &alert_status, NULL);
    if (ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "dfd_get_psu_alert_status error. psu_index:%d, ret:%d\n",
            psu_index, ret);
        return ret;
    }

    DFD_PSU_DEBUG(DBG_VERBOSE, "dfd_get_psu_alert_status success. psu_index:%d, status:%d\n",
        psu_index, alert_status);
    return alert_status;
}

int dfd_get_psu_status(unsigned int psu_index)
{
    int present_key, present_status;
    int output_key, output__status;
    int alert_key, alert_status;
    int ret1, ret2;

    present_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_PRESENT_STATUS);
    ret1 = dfd_info_get_int(present_key, &present_status, NULL);
    if (ret1 < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "dfd_get_psu_status error. psu_index:%d, ret:%d\n", psu_index, ret1);
        return ret1;
    }
    if(present_status != PRESENT) {
        return STATUS_ABSENT;
    }

    output_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_OUTPUT_STATUS);
    alert_key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_STATUS, psu_index, DFD_PSU_ALERT_STATUS);
    ret1 = dfd_info_get_int(output_key, &output__status, NULL);
    ret2 = dfd_info_get_int(alert_key, &alert_status, NULL);
    if( ret1 < 0 || ret2 < 0 ||output__status != NORMAL || alert_status != NORMAL) {
        return STATUS_NOT_OK;
    }
    return STATUS_OK;
}

ssize_t dfd_get_psu_status_str(unsigned int psu_index, char *buf)
{
    int ret;
    if(buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "params error.psu_index:%d.",psu_index);
        return -DFD_RV_INVALID_VALUE;
    }
    ret = dfd_get_psu_status(psu_index);
    if(ret < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "get psu status error,ret:%d, psu_index:%d\n", ret, psu_index);
        return ret;
    }
    memset(buf, 0 , PAGE_SIZE);
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static int dfd_psu_product_name_decode(char *psu_buf, int buf_len)
{
    int key, i;
    char *p_psu_name, *p_decode_name;
    int *psu_type_num;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_NUM, RG_MAIN_DEV_PSU, RG_MINOR_DEV_PSU);
    psu_type_num = dfd_ko_cfg_get_item(key);
    if (psu_type_num == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "get product psu type number error, key:0x%x\n", key);
        return -DFD_RV_NO_NODE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_NAME, 0, 0);
    p_decode_name = dfd_ko_cfg_get_item(key);
    if (p_decode_name == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "config error.psu decode name is NULL., key:0x%x\n", key);
        return -DFD_RV_NO_NODE;
    }

    for (i = 1; i <= *psu_type_num; i++) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_PSU_NAME, i, 0);
        p_psu_name = dfd_ko_cfg_get_item(key);
        if (p_psu_name == NULL) {
            DFD_PSU_DEBUG(DBG_ERROR, "config error.get psu name error., key:0x%x\n", key);
            return -DFD_RV_NO_NODE;
        }
        if (!strncmp(psu_buf, p_psu_name, strlen(p_psu_name))) {
            DFD_PSU_DEBUG(DBG_VERBOSE, "psu name match ok, org name:%s\n", p_psu_name);
            memset(psu_buf, 0, buf_len);
            strncpy(psu_buf, p_decode_name, buf_len -1);
            return DFD_RV_OK;
        }
    }
    DFD_PSU_DEBUG(DBG_ERROR, "psu name:%s error.can't match.\n", psu_buf);
    return -DFD_RV_DEV_NOTSUPPORT;
}

ssize_t dfd_get_psu_info(unsigned int psu_index, uint8_t cmd, char *buf)
{
    int key, rv;
    char psu_buf[PSU_SIZE];
    dfd_i2c_dev_t *i2c_dev;

    if (buf == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "buf is NULL, psu index:%d, cmd:%d\n", psu_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, PAGE_SIZE);
    memset(psu_buf, 0, PSU_SIZE);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, RG_MAIN_DEV_PSU, psu_index);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DFD_PSU_DEBUG(DBG_ERROR, "psu i2c dev config error, key=0x%08x\n", key);
        return -DFD_RV_NODE_FAIL;
    }

    rv = dfd_get_fru_data(i2c_dev->bus, i2c_dev->addr, cmd, psu_buf, PSU_SIZE);

    if (rv < 0) {
        DFD_PSU_DEBUG(DBG_ERROR, "psu eeprom read failed");
        return -DFD_RV_DEV_FAIL;
    }
    DFD_PSU_DEBUG(DBG_VERBOSE, "%s\n", psu_buf);

    if (cmd == DFD_DEV_INFO_TYPE_PART_NAME) {
        rv = dfd_psu_product_name_decode(psu_buf, PSU_SIZE);
        if (rv < 0) {
            DFD_PSU_DEBUG(DBG_ERROR, "psu name decode error. rv:%d.\n", rv);
            return rv;
        }
    }
    snprintf(buf, PSU_SIZE, "%s\n", psu_buf);
    return strlen(buf);
}
