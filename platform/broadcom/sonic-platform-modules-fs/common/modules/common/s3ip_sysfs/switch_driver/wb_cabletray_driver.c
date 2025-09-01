/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_cabletray_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * Read and write functions of cabletray-related attributes
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
#include "dfd_wbtlv_eeprom.h"

#define DFD_CABLETRAY_EEPROM_MODE_TLV_STRING    "tlv"
#define DFD_CABLETRAY_EEPROM_MODE_FRU_STRING    "fru"
#define CABLETRAY_SIZE                          (768)

#define CABLETRAY_DEFAULT_ID                    (1)
#define SUB_CABLETRAY_DEFAULT_ID                (1)

typedef enum cabletray_eeprom_mode_e {
    CABLETRAY_EEPROM_MODE_TLV,     /* TLV */
    CABLETRAY_EEPROM_MODE_FRU,      /*FRU*/
} cabletray_eeprom_mode_t;

int g_dfd_cabletray_dbg_level = 0;
module_param(g_dfd_cabletray_dbg_level, int, S_IRUGO | S_IWUSR);

static char *dfd_get_cabletray_eeprom_path(int index)
{
    uint64_t key;
    char *eeprom_path;

    /* Obtain the eeprom read path */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_PATH, WB_MAIN_DEV_CABLETRAY, index);
    eeprom_path = dfd_ko_cfg_get_item(key);
    if (eeprom_path == NULL) {
        DBG_EEPROM_DEBUG(DBG_ERROR, "get cabletray eeprom path error, e2_type: %d, index: %d, key_name: %s\n",
            WB_MAIN_DEV_CABLETRAY, index, key_to_name(DFD_CFG_ITEM_EEPROM_PATH));
        return NULL;
    }
    return eeprom_path;
}

/**
 * dfd_get_cabletray_slotid_info - Obtaining cabletray slotid Information
 * @index: Number of the cabletray, starting from 1
 * @cmd: cabletray information type, cabletray name :2, cabletray serial number :3, cabletray hardware version :5
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cabletray_slotid_info(unsigned int index, char *buf, size_t count)
{
    uint64_t key;
    int ret, slotid;

    /* Get presence status */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_CABLETRAY_SLOTID, index, 0);
    ret = dfd_info_get_int(key, &slotid, NULL);
    if (ret < 0) {
        DFD_CABLETRAY_DEBUG(DBG_ERROR, "get cabletray slotid error. index: %u, ret: %d\n", index, ret);
        return ret;
    }

    DFD_CABLETRAY_DEBUG(DBG_VERBOSE, "get cabletray slotid success. index:%u, slotid:%d\n", index, slotid);
    return (ssize_t)snprintf(buf, count, "%d\n", slotid);
}

/**
 * dfd_get_cabletray_info - Obtaining cabletray Information
 * @index: Number of the cabletray, starting from 1
 * @cmd: cabletray information type, cabletray name :2, cabletray serial number :3, cabletray hardware version :5
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_cabletray_info(unsigned int cabletray_index, uint8_t cmd, char *buf, size_t count)
{
    int rv;
    char cabletray_buf[CABLETRAY_SIZE];
    char *sysfs_name;

    if (buf == NULL) {
        DFD_CABLETRAY_DEBUG(DBG_ERROR, "buf is NULL, cabletray index: %u, cmd: 0x%x.\n", cabletray_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_CABLETRAY_DEBUG(DBG_ERROR, "buf size error, count: %zu, cabletray index: %u, cmd: 0x%x.\n",
            count, cabletray_index, cmd);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    sysfs_name = dfd_get_cabletray_eeprom_path(cabletray_index);
    if (sysfs_name == NULL) {
        DFD_CABLETRAY_DEBUG(DBG_ERROR, "get cabletray eeprom path failed, cabletray index: %u, cmd: 0x%x.\n",
            cabletray_index, cmd);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    mem_clear(cabletray_buf, sizeof(cabletray_buf));
    rv = dfd_get_fru_product_data_by_file(sysfs_name, cmd, cabletray_buf, sizeof(cabletray_buf));
    if (rv < 0) {
        DFD_CABLETRAY_DEBUG(DBG_ERROR, "cabletray eeprom read failed");
        return -DFD_RV_DEV_FAIL;
    }

    DFD_CABLETRAY_DEBUG(DBG_VERBOSE, "%s\n", cabletray_buf);

    dfd_ko_trim_trailing_spaces(cabletray_buf);
    snprintf(buf, count, "%s\n", cabletray_buf);
    return strlen(buf);
}
