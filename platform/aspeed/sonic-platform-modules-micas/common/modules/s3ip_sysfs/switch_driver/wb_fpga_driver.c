/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_fpga_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * Fpga-related attribute read and write functions
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"
#include "wb_fpga_driver.h"

#define FPGA_REG_WIDTH_MAX            (4)
#define FPGA_BOOT_VIEW_PRIMARY        (0)
#define FPGA_BOOT_VIEW_BACKUP         (1)

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
static const dfd_status_desc_map_t dfd_fpga_selftest_status_map[] = {
    {1, "Selftest_Error"},
};
#endif

int g_dfd_fpga_dbg_level = 0;
module_param(g_dfd_fpga_dbg_level, int, S_IRUGO | S_IWUSR);

dfd_sysfs_func_map_t fpga_func_table[DFD_FPGA_MAX_E] = {
    [DFD_FPGA_NAME_E] = {dfd_get_fpga_name, NULL},
    [DFD_FPGA_TYPE_E] = {dfd_get_fpga_type, NULL},
    [DFD_FPGA_VENDOR_E] = {dfd_get_fpga_vendor, NULL},
    [DFD_FPGA_FW_VERSION_E] = {dfd_get_fpga_fw_version, NULL},
    [DFD_FPGA_HW_VERSION_E] = {dfd_get_fpga_hw_version, NULL},
    [DFD_FPGA_SUPPORT_UPGRADE_E] = {dfd_get_fpga_support_upgrade, NULL},
    [DFD_FPGA_UPGRADE_ACTIVE_TYPE_E] = {dfd_get_fpga_upgrade_active_type, NULL},
    [DFD_FPGA_REG_TEST_TYPE_E] = {dfd_get_fpga_testreg_str, dfd_set_fpga_testreg},
    [DFD_FPGA_SELTEST_STATUS_E] = {dfd_get_fpga_selftest_status, NULL},
    [DFD_FPGA_CRAM_STATUS_E] = {dfd_get_fpga_cram_status, NULL},
    [DFD_FPGA_BOOT_VIEW_E] = {dfd_get_fpga_boot_view, NULL},
};

/**
 * dfd_get_fpga_name - Get the FPGA name
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:FPGA number, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_name(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    char *fpga_name;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL. main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_NAME, main_dev_id, fpga_index);
    fpga_name = dfd_ko_cfg_get_item(key);
    if (fpga_name == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, fpga%u name config error, key_name: %s\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_NAME));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "%s\n", fpga_name);
    snprintf(buf, count, "%s\n", fpga_name);
    return strlen(buf);
}

static ssize_t dfd_get_fpga_model(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    int ret, fpga_model_val;
    char *fpga_type;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_MODEL_REG, main_dev_id, fpga_index);
    ret = dfd_info_get_int(key, &fpga_model_val, NULL);
    if (ret < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "get main_dev_id: %u, fpga%u model failed, key_name: %s, ret: %d\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_MODEL_REG), ret);
        return ret;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_MODEL_DECODE, fpga_model_val, 0);
    fpga_type = dfd_ko_cfg_get_item(key);
    if (fpga_type == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, fpga%u decode fpga model val 0x%08x failed\n",
            main_dev_id, fpga_index, fpga_model_val);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE,
        "main_dev_id: %u, fpga%u decode fpga model success, origin value: 0x%08x decode value: %s\n",
        main_dev_id, fpga_index, fpga_model_val, fpga_type);
    snprintf(buf, count, "%s\n", fpga_type);
    return strlen(buf);
}

/**
 * dfd_get_fpga_type - Get FPGA model
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:FPGA number, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_type(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    char *fpga_type;
    ssize_t ret;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_TYPE, main_dev_id, fpga_index);
    fpga_type = dfd_ko_cfg_get_item(key);
    if (fpga_type == NULL) {
        DBG_FPGA_DEBUG(DBG_VERBOSE,
            "main_dev_id: %u, fpga%u type config is NULL, try to get fpga type from fpga model\n",
            main_dev_id, fpga_index);
        /* Unconfigured fpga_type Obtain the device model from fpga_model */
        ret = dfd_get_fpga_model(main_dev_id, fpga_index, buf, count);
        return ret;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "%s\n", fpga_type);
    snprintf(buf, count, "%s\n", fpga_type);
    return strlen(buf);
}

/**
 * dfd_get_fpga_vendor - Obtain the FPGA vendor
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the FPGA starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_vendor(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    char *fpga_vendor;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_VENDOR, main_dev_id, fpga_index);
    fpga_vendor = dfd_ko_cfg_get_item(key);
    if (fpga_vendor == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, fpga%u vendor config error, key_name: %s\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_VENDOR));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "%s\n", fpga_vendor);
    snprintf(buf, count, "%s\n", fpga_vendor);
    return strlen(buf);
}

/**
 * dfd_get_fpga_fw_version - Obtain the FPGA firmware version
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:FPGA number, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_fw_version(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    int rv;
    uint32_t value;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_VERSION, main_dev_id, fpga_index);
    rv = dfd_info_get_int(key, &value, NULL);
    if (rv < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, fpga%u fw config error, key_name: %s, ret: %d\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_VERSION), rv);
        return rv;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "main_dev_id: %u, fpga%u firmware version: %x\n",
        main_dev_id, fpga_index, value);
    snprintf(buf, count, "%08x\n", value);
    return strlen(buf);
}

/**
 * dfd_get_fpga_hw_version - Obtain the hardware version of the FPGA
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index: FPGA number, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_hw_version(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }
    DBG_FPGA_DEBUG(DBG_VERBOSE, "main_dev_id: %u, fpga%u hardware version not support\n",
        main_dev_id, fpga_index);
   return -DFD_RV_DEV_NOTSUPPORT;
}

static int value_convert_to_buf(unsigned int value, uint8_t *buf, int len, int pola)
{
    int i;

    if ((pola != INFO_POLA_POSI) && (pola != INFO_POLA_NEGA)) {
        DBG_FPGA_DEBUG(DBG_ERROR, "unsupport pola mode: %d\n", pola);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, len);
    if (pola == INFO_POLA_POSI) {   /* Big-end mode */
        for (i = 0; i < len; i++) {
            buf[i] = (value >> ((len - i - 1) * 8)) & 0xff;
        }
    } else {    /* Small terminal mode */
        for (i = 0; i < len; i++) {
            buf[i] = (value >> (i * 8)) & 0xff;
        }
    }
    return DFD_RV_OK;
}

/**
 * dfd_set_fpga_testreg - Sets the value of the FPGA test register
 * @main_dev_id: Motherboard :0 Subcard :5
 * @fpga_index: FPGA number, starting from 1
 * @value: Writes the value of the test register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_fpga_testreg(unsigned int main_dev_id, unsigned int fpga_index, void * val, unsigned int len)
{
    uint64_t key;
    int ret;
    uint8_t wr_buf[FPGA_REG_WIDTH_MAX];
    info_ctrl_t *info_ctrl;
    unsigned int *value;

    if (val == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "val is NULL\n");
        return -DFD_RV_INVALID_VALUE;
    }

    value = (unsigned int *)val;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_TEST_REG, main_dev_id, fpga_index);
    /* Get the configuration item read and write control variables */
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_FPGA_DEBUG(DBG_VERBOSE, "main_dev_id: %u, fpga%u get info ctrl failed, key_name: %s\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_TEST_REG));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    if (info_ctrl->fpath[0] == '\0') {
        DBG_FPGA_DEBUG(DBG_VERBOSE, "main_dev_id: %u, fpga%u get fpath failed\n", main_dev_id,
            fpga_index);
         return -DFD_RV_INVALID_VALUE;
    }
    if (info_ctrl->len > FPGA_REG_WIDTH_MAX) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, fpga%u info_ctrl len: %d, unsupport\n",
            main_dev_id, fpga_index, info_ctrl->len);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = value_convert_to_buf(*value, wr_buf, FPGA_REG_WIDTH_MAX, info_ctrl->pola);
    if (ret < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "value: 0x%x convert to buf failed, pola:%d, ret: %d\n",
            *value, info_ctrl->pola, ret);
        return ret;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "main_dev_id: %u, fpga%u fpath: %s, addr: 0x%x, len: %d value: 0x%x\n",
        main_dev_id, fpga_index, info_ctrl->fpath, info_ctrl->addr, info_ctrl->len, *value);
    ret = dfd_ko_write_file(info_ctrl->fpath, info_ctrl->addr, wr_buf, info_ctrl->len);
    if (ret < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "set fpga test reg failed, ret: %d", ret);
        return ret;
    }
    return DFD_RV_OK;
}

/**
 * dfd_get_fpga_testreg - Read the FPGA test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @fpga_index: FPGA number, starting from 1
 * @value: Read the test register value
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_get_fpga_testreg(unsigned int main_dev_id, unsigned int fpga_index, int *value)
{
    uint64_t key;
    int ret;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_TEST_REG, main_dev_id, fpga_index);
    ret = dfd_info_get_int(key, value, NULL);
    if (ret < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, get fpga%u test reg error, key_name: %s, ret: %d\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_TEST_REG), ret);
        return ret;
    }
    return DFD_RV_OK;
}

/**
 * dfd_get_fpga_testreg_str - Read the FPGA test register value
 * @main_dev_id: Motherboard :0 Subcard :5
 * @fpga_index: FPGA number, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_testreg_str(unsigned int main_dev_id, unsigned int fpga_index,
            char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    ret = dfd_get_fpga_testreg(main_dev_id, fpga_index, &value);
    if (ret < 0) {
        return ret;
    }
    return (ssize_t)snprintf(buf, count, "0x%08x\n", value);
}

/**
 * dfd_get_fpga_support_upgrade - Obtain the FPGA support_upgrade
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the FPGA starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_support_upgrade(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    int fpga_support_upgrade;
    int ret;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    fpga_support_upgrade = 0;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_SUPPORT_UPGRADE, main_dev_id, fpga_index);
    ret = dfd_info_get_int(key, &fpga_support_upgrade, NULL);
    if (ret < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, fpga%u support upgrade config error, key_name: %s\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_SUPPORT_UPGRADE));
        return ret;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "%d\n", fpga_support_upgrade);
    snprintf(buf, count, "%d\n", fpga_support_upgrade);
    return strlen(buf);
}

/**
 * dfd_get_fpga_upgrade_active_type - Obtain the FPGA upgrade active type
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the FPGA starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_upgrade_active_type(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    char *fpga_upgrade_active_type;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_UPGRADE_ACTIVE_TYPE, main_dev_id, fpga_index);
    fpga_upgrade_active_type = dfd_ko_cfg_get_item(key);
    if (fpga_upgrade_active_type == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, fpga%u upgrade active type config error, key_name: %s\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_UPGRADE_ACTIVE_TYPE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "%s\n", fpga_upgrade_active_type);
    snprintf(buf, count, "%s\n", fpga_upgrade_active_type);
    return strlen(buf);
}

/**
 * dfd_get_fpga_selftest_status - Obtain the FPGA selftest status
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the FPGA starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_selftest_status(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    int selftest_status;
    int ret;
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    char detail[64];
    size_t detail_len;
    dfd_status_detail_t detail_cfg;
#endif

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_SELFTEST_STATUS, main_dev_id, fpga_index);

    ret = dfd_info_get_int(key, &selftest_status, NULL);
    if (ret < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "failed to get selftest_status, main_dev_id: %u, fpga_index %u, key_name: %s, ret: %d\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_SELFTEST_STATUS), ret);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    mem_clear(detail, sizeof(detail));
    DFD_STATUS_DETAIL_CFG_SET(detail_cfg, selftest_status, 0, dfd_fpga_selftest_status_map, false);
    detail_len = dfd_status_get_detail(&detail_cfg, detail, sizeof(detail));
    if (detail_len > 0) {
        return (ssize_t)snprintf(buf, count, "%d %s\n", selftest_status, detail);
    } else {
        return (ssize_t)snprintf(buf, count, "%d\n", selftest_status);
    }
#else
    snprintf(buf, count, "%d\n", selftest_status);
    return strlen(buf);
#endif
}

/**
 * dfd_get_fpga_boot_view - Obtain the FPGA boot view
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the FPGA starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_boot_view(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    int ret;
    int boot_status;
    int boot_view;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_BOOT_VIEW, main_dev_id, fpga_index);
    ret = dfd_info_get_int(key, &boot_status, NULL);
    if (ret < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "failed to get fpga boot view status, main_dev_id: %u, fpga_index %u, key_name: %s, ret: %d\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_BOOT_VIEW), ret);
        return ret;
    }

    boot_view = (boot_status == 0) ? FPGA_BOOT_VIEW_BACKUP : FPGA_BOOT_VIEW_PRIMARY;
    DBG_FPGA_DEBUG(DBG_VERBOSE, "main_dev_id: %u, fpga%u boot status: 0x%x, boot_view: %d\n",
        main_dev_id, fpga_index, boot_status, boot_view);
    return (ssize_t)snprintf(buf, count, "%d\n", boot_view);
}

/**
 * dfd_get_fpga_selftest_status - Obtain the FPGA cram status
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:The number of the FPGA starts from 0
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_cram_status(unsigned int main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    info_ctrl_t *info_ctrl;
    char seu_path[INFO_STR_CONS_MAX_LEN];
    char cram_status_path[INFO_STR_CONS_MAX_LEN + 32];
    char cram_info_path[INFO_STR_CONS_MAX_LEN + 32];
    char status_buf[16];
    char info_buf[256];
    ssize_t rd_len;
    int cram_status;
    int ret;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %zu, main_dev_id: %u, fpga index: %u\n",
            count, main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_SEU_PATH, main_dev_id, fpga_index);
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, fpga%u seu_path config error, key_name: %s\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_SEU_PATH));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    mem_clear(seu_path, sizeof(seu_path));
    mem_clear(cram_status_path, sizeof(cram_status_path));
    mem_clear(cram_info_path, sizeof(cram_info_path));
    snprintf(seu_path, sizeof(seu_path), "%s", info_ctrl->str_cons);
    snprintf(cram_status_path, sizeof(cram_status_path), "%s/cram_status", seu_path);
    snprintf(cram_info_path, sizeof(cram_info_path), "%s/cram_info", seu_path);

    DBG_FPGA_DEBUG(DBG_VERBOSE, "seu path: %s, cram_status path: %s, cram_info path: %s\n",
        seu_path, cram_status_path, cram_info_path);

    mem_clear(buf, count);
    mem_clear(status_buf, sizeof(status_buf));
    rd_len = dfd_ko_read_file(cram_status_path, 0, status_buf, sizeof(status_buf) - 1);
    if (rd_len < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "read cram_status failed, loc: %s, rd_count: %zu, ret: %zd\n",
            cram_status_path, sizeof(status_buf) - 1, rd_len);
        return rd_len;
    }

    status_buf[rd_len] = '\0';
    ret = kstrtoint(strim(status_buf), 0, &cram_status);
    if (ret < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "parse cram_status failed, path: %s, value: %s, ret: %d\n",
            cram_status_path, status_buf, ret);
        return ret;
    }

    mem_clear(info_buf, sizeof(info_buf));
    rd_len = dfd_ko_read_file(cram_info_path, 0, info_buf, sizeof(info_buf) - 1);
    if (rd_len < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "read cram_info failed, loc: %s, rd_count: %zu, ret: %zd\n",
            cram_info_path, sizeof(info_buf) - 1, rd_len);
        return rd_len;
    }

    info_buf[rd_len] = '\0';
    strim(info_buf);

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    rd_len = snprintf(buf, count, "%d %s\n", cram_status, info_buf);
    DBG_FPGA_DEBUG(DBG_VERBOSE, "read cram_status/cram_info success, cram_status: %d, cram_info: %s, len: %zd\n",
        cram_status, info_buf, rd_len);
#else
    rd_len = snprintf(buf, count, "%d\n", cram_status);
    DBG_FPGA_DEBUG(DBG_VERBOSE, "read cram_status success, cram_status: %d, len: %zd\n", cram_status, rd_len);
#endif

    return rd_len;
}

