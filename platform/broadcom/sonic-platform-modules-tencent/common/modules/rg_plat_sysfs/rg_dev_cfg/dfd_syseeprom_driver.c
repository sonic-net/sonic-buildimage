/*
 * Copyright(C) 2001-2012 Ruijie Network. All rights reserved.
 */

#include <linux/module.h>

#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_adapter.h"
#include "./include/dfd_tlveeprom.h"
#include "../rg_dev_sysfs/include/rg_sysfs_common.h"

#define EEPROM_SIZE                          (256)

int g_dfd_syseeprom_dbg_level = 0;
module_param(g_dfd_syseeprom_dbg_level, int, S_IRUGO | S_IWUSR);

ssize_t dfd_get_syseeprom_info(uint8_t type, char *buf)
{
    int key, rv;
    uint32_t len;
    uint8_t eeprom[EEPROM_SIZE];
    uint8_t tlv_buf[EEPROM_SIZE];
    dfd_i2c_dev_t *i2c_dev;
    dfd_tlv_type_t tlv_type;

    if (buf == NULL) {
        DBG_SYSE2_DEBUG(DBG_ERROR, "param error. buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, PAGE_SIZE);
    memset(eeprom, 0, EEPROM_SIZE);
    memset(tlv_buf, 0, EEPROM_SIZE);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_OTHER_I2C_DEV, RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE);
    i2c_dev = dfd_ko_cfg_get_item(key);
    if (i2c_dev == NULL) {
        DBG_SYSE2_DEBUG(DBG_ERROR, "syseeprom i2c dev config error, key=0x%08x\n", key);
        return -DFD_RV_NODE_FAIL;
    }
    rv = dfd_ko_i2c_read(i2c_dev->bus, i2c_dev->addr, 0, eeprom, EEPROM_SIZE);

    if (rv < 0) {
        DBG_SYSE2_DEBUG(DBG_ERROR, "syseeprom read failed.\n");
        return -DFD_RV_DEV_FAIL;
    }

    tlv_type.main_type = type;
    tlv_type.ext_type = DFD_TLVINFO_EXT_TLV_TYPE_DEV_TYPE;
    len = sizeof(tlv_buf);
    rv = dfd_tlvinfo_get_e2prom_info(eeprom, EEPROM_SIZE, &tlv_type, tlv_buf, &len);
    if( rv ) {
        DBG_SYSE2_DEBUG(DBG_ERROR, "get tlv type :0x%02x error, ret:%d\n", tlv_type.main_type, rv);
        return -DFD_RV_TYPE_ERR;
    }
    snprintf(buf, EEPROM_SIZE, "%s\n", tlv_buf);
    return strlen(buf);
}
