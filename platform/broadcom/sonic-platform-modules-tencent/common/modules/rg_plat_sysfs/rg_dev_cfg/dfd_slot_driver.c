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

#define SLOT_SIZE                         (256)

int g_dfd_slot_dbg_level = 0;
module_param(g_dfd_slot_dbg_level, int, S_IRUGO | S_IWUSR);

int dfd_get_slot_status(unsigned int slot_index)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_PRESENT_STATUS, RG_MAIN_DEV_SLOT, slot_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot status error, key:0x%x\n",key);
        return ret;
    }
    return status;
}

ssize_t dfd_get_slot_status_str(unsigned int slot_index, char *buf)
{
    int ret;
    if(buf == NULL) {
        DFD_SLOT_DEBUG(DBG_ERROR, "params error.slot_index:%d.",slot_index);
        return -DFD_RV_INVALID_VALUE;
    }
    ret = dfd_get_slot_status(slot_index);
    if(ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot status error,ret:%d, slot_index:%d\n", ret, slot_index);
        return ret;
    }
    memset(buf, 0 , PAGE_SIZE);
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

ssize_t dfd_get_slot_info(unsigned int slot_index, uint8_t cmd, char *buf)
{
    int key, rv;
    char slot_buf[SLOT_SIZE];
    dfd_i2c_dev_t *i2c_dev;

    if (buf == NULL) {
        DFD_SLOT_DEBUG(DBG_ERROR, "buf is NULL, slot index:%d, cmd:%d\n", slot_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, PAGE_SIZE);
    memset(slot_buf, 0, SLOT_SIZE);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, RG_MAIN_DEV_SLOT, slot_index);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DFD_SLOT_DEBUG(DBG_ERROR, "slot i2c dev config error, key=0x%08x\n", key);
        return -DFD_RV_NODE_FAIL;
    }

    rv = dfd_get_fru_board_data(i2c_dev->bus, i2c_dev->addr, cmd, slot_buf, SLOT_SIZE);

    if (rv < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "slot eeprom read failed");
        return -DFD_RV_DEV_FAIL;
    }

    DFD_SLOT_DEBUG(DBG_VERBOSE, "%s\n", slot_buf);
    snprintf(buf, SLOT_SIZE, "%s\n", slot_buf);
    return strlen(buf);
}
