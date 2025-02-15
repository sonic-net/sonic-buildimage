/*
 * An wb_fpga_driver driver for fpga devcie function
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

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_adapter.h"
#include "dfd_cfg_info.h"

#define FPGA_REG_WIDTH_MAX            (4)

int g_dfd_fpga_dbg_level = 0;
module_param(g_dfd_fpga_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * dfd_get_fpga_name - Get the FPGA name
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:FPGA number, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_name(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{
    uint64_t key;
    char *fpga_name;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL. main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }

    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, fpga index: %u\n",
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

static ssize_t dfd_get_fpga_model(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count)
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
ssize_t dfd_get_fpga_type(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count)
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
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, fpga index: %u\n",
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
 * dfd_get_fpga_fw_version - Obtain the FPGA firmware version
 * @main_dev_id: Motherboard :0 Subcard :5
 * @index:FPGA number, starting from 1
 * @buf: Receive buf
 * @count: Accept the buf length
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_fpga_fw_version(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count)
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
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, fpga index: %u\n",
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
    snprintf(buf, count, "0x%08x\n", value);
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
ssize_t dfd_get_fpga_hw_version(uint8_t main_dev_id, unsigned int fpga_index, char *buf, size_t count)
{

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, fpga index: %u\n",
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
int dfd_set_fpga_testreg(uint8_t main_dev_id, unsigned int fpga_index, unsigned int value)
{
    uint64_t key;
    int ret;
    uint8_t wr_buf[FPGA_REG_WIDTH_MAX];
    info_ctrl_t *info_ctrl;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FPGA_TEST_REG, main_dev_id, fpga_index);
    /* Get the configuration item read and write control variables */
    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_FPGA_DEBUG(DBG_VERBOSE, "main_dev_id: %u, fpga%u get info ctrl failed, key_name: %s\n",
            main_dev_id, fpga_index, key_to_name(DFD_CFG_ITEM_FPGA_TEST_REG));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (info_ctrl->len > FPGA_REG_WIDTH_MAX) {
        DBG_FPGA_DEBUG(DBG_ERROR, "main_dev_id: %u, fpga%u info_ctrl len: %d, unsupport\n",
            main_dev_id, fpga_index, info_ctrl->len);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = value_convert_to_buf(value, wr_buf, FPGA_REG_WIDTH_MAX, info_ctrl->pola);
    if (ret < 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "value: 0x%x convert to buf failed, pola:%d, ret: %d\n",
            value, info_ctrl->pola, ret);
        return ret;
    }

    DBG_FPGA_DEBUG(DBG_VERBOSE, "main_dev_id: %u, fpga%u fpath: %s, addr: 0x%x, len: %d value: 0x%x\n",
        main_dev_id, fpga_index, info_ctrl->fpath, info_ctrl->addr, info_ctrl->len, value);
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
int dfd_get_fpga_testreg(uint8_t main_dev_id, unsigned int fpga_index, int *value)
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
ssize_t dfd_get_fpga_testreg_str(uint8_t main_dev_id, unsigned int fpga_index,
            char *buf, size_t count)
{
    int ret, value;

    if (buf == NULL) {
        DBG_FPGA_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, fpga index: %u\n",
            main_dev_id, fpga_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DBG_FPGA_DEBUG(DBG_ERROR, "buf size error, count: %lu, main_dev_id: %u, fpga index: %u\n",
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
