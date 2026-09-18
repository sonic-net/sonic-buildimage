/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * dfd_wbtlv_eeprom.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * wb tlv Information analysis
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "dfd_cfg_adapter.h"
#include "wb_module.h"
#include "dfd_wbtlv_eeprom.h"

int g_dfd_wb_tlv_dbg_level = 0;
module_param(g_dfd_wb_tlv_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * dfd_wb_tlv_support_type - Check cmd is wb tlv support type or not
 * @cmd: Only suooprt 2: Product name, 3: product serial number 5: hardware version number 6: product ID
 * @returns: true: support, false: not support
 */
bool dfd_wb_tlv_support_type(uint8_t cmd)
{
    if ((cmd == DFD_DEV_INFO_TYPE_NAME) || (cmd == DFD_DEV_INFO_TYPE_SN)
        || (cmd == DFD_DEV_INFO_TYPE_HW_INFO) || (cmd == DFD_DEV_INFO_TYPE_DEV_TYPE)) {
        DBG_WBTLV_DEBUG(DBG_VERBOSE, "Support wb tlv type: %u\n", cmd);
        return true;
    }
    DBG_WBTLV_DEBUG(DBG_VERBOSE, "Unsupport wb tlv type: %u\n", cmd);
    return false;
}

/**
 * dfd_wb_tlv_eeprom_read - Obtain wb tlv eeprom information
 * @bus: E2 bus number
 * @addr: E2 Device address
 * @cmd: 2: Product name, 3: product serial number 5: hardware version number 6: product ID
 * @buf:Data is stored in buf
 * @len:buf length
 * @sysfs_name:sysfs attribute name
 * @returns: 0 success, negative value: failed
 */
int dfd_wb_tlv_eeprom_read(int bus, int addr, uint8_t cmd, char *buf, int len,
               const char *sysfs_name)
{
    dfd_dev_head_info_t info;
    char tmp_tlv_len[sizeof(uint16_t)];
    char *tlv_data;
    dfd_dev_tlv_info_t *tlv;
    int buf_len;
    int rv, match_flag, i;
    int offset = 0;

    rv = dfd_ko_i2c_read(bus, addr, 0, (uint8_t *)&info, sizeof(dfd_dev_head_info_t), sysfs_name);
    if (rv < 0) {
        DBG_WBTLV_DEBUG(DBG_ERROR, "read i2c failed, bus: %d, addr: 0x%x, rv: %d\n",
            bus, addr, rv);
        return -DFD_RV_DEV_FAIL;
    }

    /* convert TLV_LEN */
    memcpy(tmp_tlv_len, (uint8_t *)&info.tlv_len, sizeof(uint16_t));
    info.tlv_len = (tmp_tlv_len[0] << 8) + tmp_tlv_len[1];

    if ((info.tlv_len <= 0) || (info.tlv_len > 0xFF)) {
        DBG_WBTLV_DEBUG(DBG_ERROR, "fan maybe not set mac.\n");
        return -DFD_RV_TYPE_ERR;
    }
    DBG_WBTLV_DEBUG(DBG_VERBOSE, "info.tlv_len: %d\n", info.tlv_len);

    tlv_data = (uint8_t *)kmalloc(info.tlv_len, GFP_KERNEL);
    if (tlv_data == NULL) {
        DBG_WBTLV_DEBUG(DBG_ERROR, "tlv_data kmalloc failed \n");
        return -DFD_RV_NO_MEMORY;
    }
    mem_clear(tlv_data, info.tlv_len);

    rv = dfd_ko_i2c_read(bus, addr, sizeof(dfd_dev_head_info_t), tlv_data, info.tlv_len, sysfs_name);
    if (rv < 0) {
        DBG_WBTLV_DEBUG(DBG_ERROR,"fan eeprom read failed\n");
        kfree(tlv_data);
        return -DFD_RV_DEV_FAIL;
    }

    buf_len = len - 1;
    match_flag = 0;
    for (tlv = (dfd_dev_tlv_info_t *)tlv_data; (ulong)tlv < (ulong)tlv_data + info.tlv_len;) {
        DBG_WBTLV_DEBUG(DBG_VERBOSE, "tlv: %p, tlv->type: 0x%x, tlv->len: 0x%x info->tlv_len: 0x%x\n",
            tlv, tlv->type, tlv->len, info.tlv_len);
        if (tlv->type == cmd && tlv->len <= buf_len) {
            DBG_WBTLV_DEBUG(DBG_VERBOSE, "find tlv data, copy...\n");
            /* product id */
            if (cmd == DFD_DEV_INFO_TYPE_DEV_TYPE) {
                offset += scnprintf(buf + offset, buf_len - offset, "0x");
                for (i = 0; i < tlv->len; i++) {
                    offset += scnprintf(buf + offset, buf_len - offset, "%02x", tlv->data[i]);
                }
            } else {
                memcpy(buf, (uint8_t *)tlv->data, tlv->len);
            }

            buf_len = (uint32_t)tlv->len;
            match_flag = 1;
            break;
        }
        tlv = (dfd_dev_tlv_info_t *)((uint8_t*)tlv + sizeof(dfd_dev_tlv_info_t) + tlv->len);
    }
    kfree(tlv_data);
    if (match_flag == 0) {
        DBG_WBTLV_DEBUG(DBG_ERROR,"can't find fan tlv date. bus: %d, addr: 0x%02x, tlv type: %d.\n",
            bus, addr, cmd);
        return -DFD_RV_TYPE_ERR;
    }
    return buf_len;
}

