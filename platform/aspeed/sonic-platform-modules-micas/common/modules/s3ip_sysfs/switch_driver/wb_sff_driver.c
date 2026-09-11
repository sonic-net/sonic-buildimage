/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_sff_driver.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * sff related attribute read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 */

#include <linux/module.h>
#include <linux/delay.h>
#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_sff_driver.h"
#include "wb_eeprom_driver.h"
#include "transceiver_sysfs.h"
#include "switch_driver.h"

int g_dfd_sff_dbg_level = 0;
module_param(g_dfd_sff_dbg_level, int, S_IRUGO | S_IWUSR);

#define SFP_TEMP_TYPE_EEPROM    (1)
#define SFP_TEMP_TYPE_PAGE      (2)
#define SFP_TEMP_TYPE_DEFAULT SFP_TEMP_TYPE_EEPROM
#define TYPE_FROM_E2            (1)
#define TYPE_FROM_PAGE          (2)

dfd_debug_data_key_map_t sff_dbg_key_table[WB_SFF_ATTR_END] = {
    [WB_SFF_MODULE_PRESENT] = {DFD_CFG_ITEM_SFF_CPLD_REG, CFG_2INDEXES_2, CFG_INT_DATA},
}; 

static const sff_attr_offset_map_t sff_attr_offset_map[] = {
    {WB_SFF_RX_LOS,           -1,                        QSFP_RX_LOS_OFFSET,        CMIS_RX_LOS_OFFSET},
    {WB_SFF_TX_DIS,           -1,                        QSFP_TX_DISABLE_OFFSET,    CMIS_TX_DISABLE_OFFSET},
    {WB_SFF_TX_FAULT,         -1,                        QSFP_TX_FAULT_OFFSET,      CMIS_TX_FAULT_OFFSET},
    {WB_SFF_TX_LOS,           -1,                        -1,                        CMIS_TX_LOS_OFFSET},
    {WB_SFF_TX_CDR_LOL,       -1,                        -1,                        CMIS_TX_CDR_LOL_OFFSET},
    {WB_SFF_RX_CDR_LOL,       -1,                        -1,                        CMIS_RX_CDR_LOL_OFFSET},
    {WB_SFF_MODULE_STATUS,    -1,                        -1,                        CMIS_MODULE_STATUS_OFFSET},
    {WB_SFF_DATAPATH_STATUS,  -1,                        -1,                        CMIS_DATAPATH_STATUS_OFFSET},
    {WB_SFF_HOST_SNR,         -1,                        -1,                        CMIS_HOST_SNR_OFFSET},
    {WB_SFF_MEDIA_SNR,        -1,                        -1,                        CMIS_MEDIA_SNR_OFFSET},
    {WB_SFF_TEMP,             SFP_TEMP_OFFSET,           QSFP_TEMP_OFFSET,          CMIS_TEMP_OFFSET},
    {WB_SFF_TX_BIAS,          -1,                        -1,                        CMIS_TX_BIAS_OFFSET},
    {WB_SFF_TX_POWER,         -1,                        -1,                        CMIS_TX_POWER_OFFSET},
    {WB_SFF_RX_POWER,         -1,                        -1,                        CMIS_RX_POWER_OFFSET},
    {WB_SFF_VOLTAGE,          -1,                        -1,                        CMIS_VOLTAGE_OFFSET},
    {WB_SFF_TX_FLAG_SUP,      -1,                        -1,                        CMIS_TX_FLAG_SUP_OFFSET},
    {WB_SFF_RX_FLAG_SUP,      -1,                        -1,                        CMIS_RX_FLAG_SUP_OFFSET},
    {WB_SFF_SNR_SUP,          -1,                        -1,                        CMIS_SNR_SUP_OFFSET},
    {WB_SFF_LANE_SPECIFIC,    -1,                        -1,                        CMIS_LANE_SPECIFIC_OFFSET},
    {WB_SFF_SNR_SELECT,       -1,                        -1,                        CMIS_SNR_SELECT_OFFSET},
};

dfd_sysfs_func_map_t sff_func_table[WB_SFF_ATTR_END] = {
    [WB_SFF_RESET] = {dfd_get_sff_cpld_info, dfd_set_sff_cpld_value},
    [WB_SFF_LPMODE] = {dfd_get_sff_cpld_info, dfd_set_sff_cpld_value},
    [WB_SFF_INTERRUPT] = {dfd_get_sff_cpld_info, dfd_set_sff_cpld_value},
    [WB_SFF_RX_LOS] = {dfd_get_sff_rx_los, NULL},
    [WB_SFF_TX_LOS] = {dfd_get_sff_tx_los, NULL},
    [WB_SFF_TX_DIS] = {dfd_get_sff_tx_disable, NULL},
    [WB_SFF_TX_FAULT] = {dfd_get_sff_tx_fault, NULL},
    [WB_SFF_TX_CDR_LOL] = {dfd_get_sff_tx_cdr_lol, NULL},
    [WB_SFF_RX_CDR_LOL] = {dfd_get_sff_rx_cdr_lol, NULL},
    [WB_SFF_MODULE_STATUS] = {dfd_get_sff_module_status, NULL},
    [WB_SFF_DATAPATH_STATUS] = {dfd_get_sff_datapath_status, NULL},
    [WB_SFF_HOST_SNR] = {dfd_get_sff_host_snr, NULL},
    [WB_SFF_MEDIA_SNR] = {dfd_get_sff_media_snr, NULL},
    [WB_SFF_DIAGNOSTIC] = {dfd_get_sff_diagnostic, NULL},
};

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
#define SFF_RX_LOS_ERROR_DESC      "Rx_Los_Error"
#define SFF_TX_DISABLE_ERROR_DESC  "Tx_Disable_Error"
#define SFF_TX_FAULT_ERROR_DESC    "Tx_Fault_Error"
#define SFF_TX_LOS_ERROR_DESC      "Tx_Los_Error"
#define SFF_TX_CDR_LOL_ERROR_DESC  "Tx_Cdr_Lol_Error"
#define SFF_RX_CDR_LOL_ERROR_DESC  "Rx_Cdr_Lol_Error"

static const char * const dfd_sff_datapath_state_map[] = {
    [1] = "DeActive",
    [2] = "Init",
    [3] = "DeInit",
    [4] = "Active",
    [5] = "TurnOn",
    [6] = "TurnOff",
    [7] = "Initialized",
};

static size_t dfd_sff_append_datapath_list(char *buf, size_t buf_len, unsigned int datapath_status)
{
    int i;
    unsigned int lane_status;
    const char *state;
    size_t len;

    len = 0;
    for (i = 0; i < SFF_LANE_NUM; i++) {
        lane_status = (datapath_status >> (i * 4)) & 0xF;
        if (lane_status == 4 || lane_status >= ARRAY_SIZE(dfd_sff_datapath_state_map)) {
            state = NULL;
        } else {
            state = dfd_sff_datapath_state_map[lane_status];
        }

        if (state != NULL) {
            len += scnprintf(buf + len, buf_len - len,
                (len == 0) ? "Lane%d=%s" : ",Lane%d=%s", i, state);
            if (len >= buf_len) {
                break;
            }
        }
    }
    return len;
}

static ssize_t dfd_sff_try_format_lane_fault(char *buf, size_t count, unsigned char lane_flags, const char *error_desc)
{
    char detail[64];
    int lane;
    bool has_lane;
    size_t len;

    if (lane_flags == 0 || error_desc == NULL) {
        return 0;
    }

    mem_clear(detail, sizeof(detail));
    len = 0;
    has_lane = false;
    for (lane = 0; lane < 8; lane++) {
        if ((lane_flags >> lane) & 0x1) {
            len += scnprintf(detail + len, sizeof(detail) - len, has_lane ? ",%d" : "Lane%d", lane);
            has_lane = true;
        }

        if (len >= sizeof(detail)) {
            break;
        }
    }

    if (len == 0) {
        return 0;
    }

    return (ssize_t)snprintf(buf, count, "0x%x %s %s\n", lane_flags, detail, error_desc);
}
#endif

static int dfd_get_eth_protocol_type(unsigned int sff_index, char *protocol_type,
    int data_src)
{
    int ret;
    char reg_val;
    char e2_low_page_info[DFD_E2PROM_PAGE_LEN + 1];

    if (protocol_type == NULL) {
        return -DFD_RV_INVALID_VALUE;
    }

    if (data_src == TYPE_FROM_PAGE) {
        mem_clear(e2_low_page_info, sizeof(e2_low_page_info));
        ret = dfd_get_sff_e2_page_info(sff_index, WB_SFF_E2_PAGE_LOW,
            e2_low_page_info, sizeof(e2_low_page_info));
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "get eth%u page info failed, ret: %d\n", sff_index, ret);
            return ret;
        }
        reg_val = e2_low_page_info[PORT_TYPE_OFFSET];
    } else {
        ret = dfd_read_eeprom_data(WB_MAIN_DEV_SFF, sff_index, &reg_val, SFF_E2_IDENTIFIER_REG, 1);
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "get eth%u e2 protocol type failed, ret: %d\n", sff_index, ret);
            /* not present or read failed */
            return ret;
        }
    }

    switch (reg_val) {
    case SFF_Identifier_SFP:
        *protocol_type = SFF_PROTOCOL_SFP;
        break;
    case SFF_Identifier_QSFP_PLUS:
    case SFF_Identifier_QSFP28:
        *protocol_type = SFF_PROTOCOL_QSFP;
        break;
    case SFF_Identifier_QSFPDD:
    case SFF_Identifier_OSFP:
    case SFF_Identifier_DSFP:
    case SFF_Identifier_QSFP_CMIS:
        *protocol_type = SFF_PROTOCOL_CMIS;
        break;
    default:
        DFD_SFF_DEBUG(DBG_ERROR, "protocol type:%d not support.\n", reg_val);
        return -DFD_RV_INVALID_VALUE;
    }

    return 0;
}

static int dfd_get_sff_temp_type(void)
{
    int *temp_type;
    uint64_t key;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFP_TEMP_TYPE, 0, 0);
    temp_type = dfd_ko_cfg_get_item(key);
    if (temp_type == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "undefined SFP_TEMP_TYPE, use default:%d\n", 
            SFP_TEMP_TYPE_DEFAULT);
        return SFP_TEMP_TYPE_DEFAULT;
    }

    if (*temp_type != SFP_TEMP_TYPE_EEPROM && *temp_type != SFP_TEMP_TYPE_PAGE) {
        DFD_SFF_DEBUG(DBG_ERROR, "invalid sfp_temp_type=%d\n", *temp_type);
        return -DFD_RV_DEV_FAIL;
    }
    
    return *temp_type;
}

static int dfd_get_offset_by_protocol_type(char protocol_type, int type)
{
    int i;
    int offset;

    if (protocol_type != SFF_PROTOCOL_SFP &&
        protocol_type != SFF_PROTOCOL_QSFP &&
        protocol_type != SFF_PROTOCOL_CMIS) {
        DFD_SFF_DEBUG(DBG_ERROR, "get offset protocol_type:%d not support\n", protocol_type);
        return -DFD_RV_INVALID_VALUE;
    }

    for (i = 0; i < ARRAY_SIZE(sff_attr_offset_map); i++) {
        if (sff_attr_offset_map[i].attr_type == type) {
            offset = -1;

            if (protocol_type == SFF_PROTOCOL_SFP) {
                offset = sff_attr_offset_map[i].sfp_offset;
            } else if (protocol_type == SFF_PROTOCOL_QSFP) {
                offset = sff_attr_offset_map[i].qsfp_offset;
            } else if (protocol_type == SFF_PROTOCOL_CMIS) {
                offset = sff_attr_offset_map[i].cmis_offset;
            }

            if (offset < 0) {
                DFD_SFF_DEBUG(DBG_ERROR, "attr_type:%d not support for protocol:%d\n", 
                    type, protocol_type);
                return -DFD_RV_INVALID_VALUE;
            }

            return offset;
        }
    }

    DFD_SFF_DEBUG(DBG_ERROR, "get offset attr_type:%d not support\n", type);
    return -DFD_RV_INVALID_VALUE;
}

static int get_temperature_mC(unsigned int sff_index, int data_src, int *temp_value)
{
    int ret;
    int temp_offset;
    char protocol_type;
    unsigned char temp_msb;
    unsigned char temp_lsb;
    int temp_raw;
    int temp_c;
    char temp_buf[2] = {0};
    char e2_low_page_info[DFD_E2PROM_PAGE_LEN + 1];

    if (temp_value == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "convert sff temp raw temp_value is NULL\n");
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, data_src);
    if (ret < 0) {
        return ret;
    }

    ret = dfd_get_offset_by_protocol_type(protocol_type, WB_SFF_TEMP);
    if (ret < 0) {
        return ret;
    }
    temp_offset = ret;

    if (data_src == TYPE_FROM_PAGE) {
        if (temp_offset + 1 > DFD_E2PROM_PAGE_LEN) {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff%u temp offset out of range, offset: %d\n",
                sff_index, temp_offset);
            return -DFD_RV_INVALID_VALUE;
        }

        mem_clear(e2_low_page_info, sizeof(e2_low_page_info));
        ret = dfd_get_sff_e2_page_info(sff_index, WB_SFF_E2_PAGE_LOW,
            e2_low_page_info, sizeof(e2_low_page_info));
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff%u e2 page info error, ret: %d\n", sff_index, ret);
            return ret;
        }

        temp_msb = (unsigned char)e2_low_page_info[temp_offset];
        temp_lsb = (unsigned char)e2_low_page_info[temp_offset + 1];
    } else if (data_src == TYPE_FROM_E2) {
        ret = dfd_read_eeprom_data(WB_MAIN_DEV_SFF, sff_index, temp_buf, temp_offset, sizeof(temp_buf));
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff%u eeprom temp info error, ret: %d\n", sff_index, ret);
            return ret;
        }

        temp_msb = (unsigned char)temp_buf[0];
        temp_lsb = (unsigned char)temp_buf[1];
    } else {
        return -DFD_RV_INVALID_VALUE;
    }

    temp_raw = (int)(s16)(((u16)temp_msb << 8) | (u16)temp_lsb);
    temp_c = temp_raw / 256;

    if (temp_c > PORT_TEMP_MAX || temp_c < PORT_TEMP_MIN) {
        DFD_SFF_DEBUG(DBG_ERROR, "sff temp raw value out of range, temp_c: %d\n", temp_c);
        return -DFD_RV_TYPE_ERR;
    }

    *temp_value = (temp_raw * 1000) / 256;
    return 0;
}

static int dfd_get_sff_e2_info(unsigned int sff_index, unsigned int type, char protocol_type, char *buf, int buf_len)
{
    int ret;
    loff_t offset;

    DFD_SFF_DEBUG(DBG_VERBOSE, "index:%d, type:%d, prot_type:%d, buf_len:%d", 
        sff_index, type, protocol_type, buf_len);

    ret = dfd_get_offset_by_protocol_type(protocol_type, type);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get offset failed when get attr, protocol_type: %d, type: %d\n", 
            protocol_type, type);
        return ret;
    }
    offset = ret;

    ret = dfd_read_eeprom_data(WB_MAIN_DEV_SFF, sff_index, buf, offset, buf_len);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get eth%u module e2 value failed, ret: %d\n", sff_index, ret);
        return ret;
    }

    return ret;
}

static int dfd_set_sff_e2_info(unsigned int sff_index, unsigned int type, char protocol_type, char *buf, int buf_len)
{
    int ret;
    loff_t offset;

    DFD_SFF_DEBUG(DBG_VERBOSE, "index:%d, type:%d, prot_type:%d, buf_len:%d", 
        sff_index, type, protocol_type, buf_len);

    ret = dfd_get_offset_by_protocol_type(protocol_type, type);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get offset failed when get attr, protocol_type: %d, type: %d\n", 
            protocol_type, type);
        return ret;
    }
    offset = ret;

    ret = dfd_write_eeprom_data(WB_MAIN_DEV_SFF, sff_index, buf, offset, buf_len);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set eth%u module e2 value failed, ret: %d\n", sff_index, ret);
        return ret;
    }

    return ret;
}

ssize_t dfd_get_sff_rx_los(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char rx_los, rx_los_sup;
    char protocol_type;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
        DFD_SFF_DEBUG(DBG_VERBOSE, "get eth%u module type is SFP\n", sff_index);
        return dfd_get_sff_cpld_info(sff_index, type, buf, count);
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    /* get rx_los support info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_RX_FLAG_SUP, protocol_type, &rx_los_sup, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }
    if (!(rx_los_sup & SFF_RX_LOS_SUP_MASK)) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, &rx_los, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    ret = dfd_sff_try_format_lane_fault(buf, count, rx_los, SFF_RX_LOS_ERROR_DESC);
    if (ret > 0) {
        return ret;
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", rx_los);
    }
#else
    return (ssize_t)snprintf(buf, count, "0x%x\n", rx_los);
#endif
}

ssize_t dfd_get_sff_tx_disable(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char tx_disable;
    char protocol_type;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
        DFD_SFF_DEBUG(DBG_VERBOSE, "get eth%u module type is SFP\n", sff_index);
        return dfd_get_sff_cpld_info(sff_index, type, buf, count);
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, &tx_disable, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    ret = dfd_sff_try_format_lane_fault(buf, count, tx_disable, SFF_TX_DISABLE_ERROR_DESC);
    if (ret > 0) {
        return ret;
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", tx_disable);
    }
#else
    return (ssize_t)snprintf(buf, count, "0x%x\n", tx_disable);
#endif
}

ssize_t dfd_get_sff_tx_fault(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char tx_fault, tx_fault_sup;
    char protocol_type;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
        DFD_SFF_DEBUG(DBG_VERBOSE, "get eth%u module type is SFP\n", sff_index);
        return dfd_get_sff_cpld_info(sff_index, type, buf, count);
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    /* get tx_fault support info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_TX_FLAG_SUP, protocol_type, &tx_fault_sup, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }
    if (!(tx_fault_sup & SFF_TX_FAULT_SUP_MASK)) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, &tx_fault, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    ret = dfd_sff_try_format_lane_fault(buf, count, tx_fault, SFF_TX_FAULT_ERROR_DESC);
    if (ret > 0) {
        return ret;
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", tx_fault);
    }
#else
    return (ssize_t)snprintf(buf, count, "0x%x\n", tx_fault);
#endif
}

ssize_t dfd_get_sff_tx_los(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char tx_los, tx_los_sup;
    char protocol_type;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    /* get tx_los support info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_TX_FLAG_SUP, protocol_type, &tx_los_sup, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }
    if (!(tx_los_sup & SFF_TX_LOS_SUP_MASK)) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, &tx_los, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    ret = dfd_sff_try_format_lane_fault(buf, count, tx_los, SFF_TX_LOS_ERROR_DESC);
    if (ret > 0) {
        return ret;
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", tx_los);
    }
#else
    return (ssize_t)snprintf(buf, count, "0x%x\n", tx_los);
#endif
}


ssize_t dfd_get_sff_tx_cdr_lol(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char tx_cdr_lol, tx_cdr_lol_sup;
    char protocol_type;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    /* get tx_cdr_lol support info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_TX_FLAG_SUP, protocol_type, &tx_cdr_lol_sup, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }
    if (!(tx_cdr_lol_sup & SFF_TX_CDR_LOL_SUP_MASK)) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, &tx_cdr_lol, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    ret = dfd_sff_try_format_lane_fault(buf, count, tx_cdr_lol, SFF_TX_CDR_LOL_ERROR_DESC);
    if (ret > 0) {
        return ret;
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", tx_cdr_lol);
    }
#else
    return (ssize_t)snprintf(buf, count, "0x%x\n", tx_cdr_lol);
#endif
}


ssize_t dfd_get_sff_rx_cdr_lol(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char rx_cdr_lol, rx_cdr_lol_sup;
    char protocol_type;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    /* get rx_cdr_lol support info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_RX_FLAG_SUP, protocol_type, &rx_cdr_lol_sup, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }
    if (!(rx_cdr_lol_sup & SFF_RX_CDR_LOL_SUP_MASK)) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, &rx_cdr_lol, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    ret = dfd_sff_try_format_lane_fault(buf, count, rx_cdr_lol, SFF_RX_CDR_LOL_ERROR_DESC);
    if (ret > 0) {
        return ret;
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", rx_cdr_lol);
    }
#else
    return (ssize_t)snprintf(buf, count, "0x%x\n", rx_cdr_lol);
#endif
}

ssize_t dfd_get_sff_module_status(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char value;
    char protocol_type;
    char module_status;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, &value, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

    module_status = (value >> 1) & SFF_MODULE_STATUS_MASK;

    return (ssize_t)snprintf(buf, count, "%d\n", module_status);
}

ssize_t dfd_get_sff_datapath_status(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret, i;
    unsigned char status_value[4];
    char protocol_type;
    unsigned int datapath_status;
    int status_len;
#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    char detail[128];
    size_t len;
#endif

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    status_len = SFF_LANE_NUM / 2;
    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, status_value, status_len);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

    datapath_status = 0;
    for (i = 0; i < status_len; ++i) {
        datapath_status |= ((unsigned int)status_value[i]) << (i * 8);
    }

#if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2
    mem_clear(detail, sizeof(detail));
    len = 0;
    len = dfd_sff_append_datapath_list(detail, sizeof(detail), datapath_status);
    if (len > 0) {
        return (ssize_t)snprintf(buf, count, "0x%x %s\n", datapath_status, detail);
    } else {
        return (ssize_t)snprintf(buf, count, "0x%x\n", datapath_status);
    }
#else
    return (ssize_t)snprintf(buf, count, "0x%x\n", datapath_status);
#endif
}

ssize_t dfd_get_sff_host_snr(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char host_snr_sup;
    unsigned char value[SFF_GET_VALUE_LEN_MAX];
    char snr_select;
    char protocol_type;
    int i;
    int written;
    int remaining;
    unsigned short raw_value;
    int snr_int, snr_frac;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    /* get host_snr support info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_SNR_SUP, protocol_type, &host_snr_sup, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, WB_SFF_SNR_SUP, ret);
        return ret;
    }
    if (!(host_snr_sup & SFF_HOST_SNR_SUP_MASK)) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    /* set DiagnosticsSelector to 06h to get snr info */
    snr_select = SFF_SNR_SELSET_VALUE;
    ret = dfd_set_sff_e2_info(sff_index, WB_SFF_SNR_SELECT, protocol_type, &snr_select, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, WB_SFF_SNR_SELECT, ret);
        return ret;
    }

    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, value, SFF_GET_VALUE_LEN_MAX);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

    written = 0;
    remaining = count;
    for (i = 0; i < SFF_LANE_NUM; i++) {
        raw_value = (value[i * 2 + 1] << 8) | value[i * 2];
        if (raw_value == 0) {
            if (remaining > 0) {
                written += snprintf(buf + written, remaining, "lane%d: 0\n", i);
                if (written < count) {
                    remaining = count - written;
                }
            }
        } else {
            snr_int = raw_value / 256;
            snr_frac = (raw_value % 256) * 1000 / 256;
            if (remaining > 0) {
                written += snprintf(buf + written, remaining, "lane%d: %d.%03d\n", i, snr_int, snr_frac);
                if (written < count) {
                    remaining = count - written;
                }
            }
        }
    }

    if (written == 0) {
        written = snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    return (ssize_t)written;
}

ssize_t dfd_get_sff_media_snr(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char media_snr_sup;
    unsigned char value[SFF_GET_VALUE_LEN_MAX];
    char snr_select;
    char protocol_type;
    int i;
    int written;
    int remaining;
    unsigned short raw_value;
    int snr_int, snr_frac;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    /* get media_snr support info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_SNR_SUP, protocol_type, &media_snr_sup, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, WB_SFF_SNR_SUP, ret);
        return ret;
    }
    if (!(media_snr_sup & SFF_MEDIA_SNR_SUP_MASK)) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    /* set DiagnosticsSelector to 06h to get snr info */
    snr_select = SFF_SNR_SELSET_VALUE;
    ret = dfd_set_sff_e2_info(sff_index, WB_SFF_SNR_SELECT, protocol_type, &snr_select, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, WB_SFF_SNR_SELECT, ret);
        return ret;
    }

    ret = dfd_get_sff_e2_info(sff_index, type, protocol_type, value, SFF_GET_VALUE_LEN_MAX);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get_sff_e2_info failed, sff index: %u, type: %u, ret: %d\n", 
            sff_index, type, ret);
        return ret;
    }

    written = 0;
    remaining = count;
    for (i = 0; i < SFF_LANE_NUM; i++) {
        raw_value = (value[i * 2 + 1] << 8) | value[i * 2];
        if (raw_value == 0) {
            if (remaining > 0) {
                written += snprintf(buf + written, remaining, "lane%d: 0\n", i);
                if (written < count) {
                    remaining = count - written;
                }
            }
        } else {
            snr_int = raw_value / 256;
            snr_frac = (raw_value % 256) * 1000 / 256;
            if (remaining > 0) {
                written += snprintf(buf + written, remaining, "lane%d: %d.%03d\n", i, snr_int, snr_frac);
                if (written < count) {
                    remaining = count - written;
                }
            }
        }
    }

    if (written == 0) {
        written = snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    return (ssize_t)written;
}

/* Get temperature string */
static int dfd_get_temperature_str(unsigned int sff_index, char protocol_type, 
                                   char *buf, int count)
{
    int ret;
    int temp_mC;
    int temp_abs_mC;
    int temp_int;
    int temp_frac;

    ret = get_temperature_mC(sff_index, TYPE_FROM_E2, &temp_mC);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Failed to get temperature, sff index: %u, ret: %d\n", 
                     sff_index, ret);
        return snprintf(buf, count, "Temperature: NA C\n");
    }

    temp_abs_mC = (temp_mC < 0) ? -temp_mC : temp_mC;
    temp_int = temp_abs_mC / 1000;
    temp_frac = (temp_abs_mC % 1000) / 10;

    if (temp_mC < 0) {
        return snprintf(buf, count, "Temperature: -%d.%02d C\n", temp_int, temp_frac);
    }
    return snprintf(buf, count, "Temperature: %d.%02d C\n", temp_int, temp_frac);
}

/* Get voltage string */
static int dfd_get_voltage_str(unsigned int sff_index, char protocol_type, 
                               char *buf, int count)
{
    unsigned char volt_value[2];
    int ret, volt_raw, volt_int, volt_frac;
    
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_VOLTAGE, protocol_type, volt_value, 2);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Failed to get voltage, sff index: %u, ret: %d\n", sff_index, ret);
        return snprintf(buf, count, "Voltage    : NA V\n");
    }
    volt_raw = (volt_value[0] << 8) | volt_value[1];
    volt_int = volt_raw / 10000;
    volt_frac = (volt_raw % 10000) * 100 / 10000;
    return snprintf(buf, count, "Voltage    : %d.%02d V\n", volt_int, volt_frac);
}

static int dfd_fill_na_strings(int lane_num, const char *prefix, const char *suffix, 
                char *buf, int count, int written)
{
    int i, remaining = count - written;
    char format[64];

    snprintf(format, sizeof(format), "%s%%d %s: NA %%s\n", prefix, suffix);
    for (i = 0; i < lane_num && remaining > 0; i++) {
        written += snprintf(buf + written, remaining, format, 
            i + 1, (strstr(suffix, "Bias") != NULL) ? "mA" : "mW");
        remaining = count - written;
    }
    return written;
}

/* Get TX bias current string */
static int dfd_get_tx_bias_str(unsigned int sff_index, char protocol_type, int lane_num, char *buf, int count)
{
    unsigned char tx_bias_value[SFF_GET_VALUE_LEN_MAX];
    char sup_value;
    int ret, i;
    unsigned int raw_bias;
    int bias_int, bias_frac;
    int written;
    int remaining;
    int tx_bias_factor;

    written = 0;
    remaining = count;
    /* get tx bias support and factor info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_LANE_SPECIFIC, protocol_type, &sup_value, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Failed to get lane specific, sff index: %u, ret: %d\n", sff_index, ret);
        return dfd_fill_na_strings(lane_num, "Tx", "Bias   ", buf, count, written);
    }
    if (!(sup_value & SFF_TX_BIAS_SUP_MASK)) {
        return dfd_fill_na_strings(lane_num, "Tx", "Bias   ", buf, count, written);
    }

    if ((sup_value & SFF_BIAS_FACTOR_SUP_MASK) == SFF_BIAS_FACTOR_X2) {
        tx_bias_factor = 2;
    } else if ((sup_value & SFF_BIAS_FACTOR_SUP_MASK) == SFF_BIAS_FACTOR_X4) {
        tx_bias_factor = 4;
    } else {
        tx_bias_factor = 1;
    }

    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_TX_BIAS, protocol_type, tx_bias_value, lane_num * 2);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Failed to get TX bias, sff index: %u, ret: %d\n", sff_index, ret);
        return dfd_fill_na_strings(lane_num, "Tx", "Bias   ", buf, count, written);
    }

    for (i = 0; i < lane_num && remaining > 0; i++) {
        raw_bias = ((tx_bias_value[i * 2] << 8) | tx_bias_value[i * 2 + 1]) * tx_bias_factor;
        if (raw_bias == 0) {
            written += snprintf(buf + written, remaining, "Tx%d Bias   : 0 mA\n", i + 1);
        } else {
            bias_int = (raw_bias * 2) / 1000;
            bias_frac = ((raw_bias * 2) % 1000) * 100 / 1000;
            written += snprintf(buf + written, remaining, "Tx%d Bias   : %d.%02d mA\n",
                i + 1, bias_int, bias_frac);
        }
        remaining = count - written;
    }

    return written;
}

/* Get TX power string */
static int dfd_get_tx_power_str(unsigned int sff_index, char protocol_type, int lane_num, char *buf, int count)
{
    unsigned char tx_power_value[SFF_GET_VALUE_LEN_MAX];
    char sup_value;
    int ret, i;
    unsigned short raw_power;
    int power_int, power_frac;
    int written;
    int remaining;

    written = 0;
    remaining = count;
    /* get tx power support info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_LANE_SPECIFIC, protocol_type, &sup_value, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Failed to get lane specific, sff index: %u, ret: %d\n", sff_index, ret);
        return dfd_fill_na_strings(lane_num, "Tx", "Power  ", buf, count, written);
    }
    if (!(sup_value & SFF_TX_POWER_SUP_MASK)) {
        return dfd_fill_na_strings(lane_num, "Tx", "Power  ", buf, count, written);
    }

    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_TX_POWER, protocol_type, tx_power_value, lane_num * 2);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Failed to get TX power, sff index: %u, ret: %d\n", sff_index, ret);
        return dfd_fill_na_strings(lane_num, "Tx", "Power  ", buf, count, written);
    }

    for (i = 0; i < lane_num && remaining > 0; i++) {
        raw_power = (tx_power_value[i * 2] << 8) | tx_power_value[i * 2 + 1];
        if (raw_power == 0) {
            written += snprintf(buf + written, remaining, "Tx%d Power  : 0 mW\n", i + 1);
        } else {
            power_int = raw_power / 10000;
            power_frac = (raw_power % 10000) * 100 / 10000;
            written += snprintf(buf + written, remaining, "Tx%d Power  : %d.%02d mW\n", 
                i + 1, power_int, power_frac);
        }
        remaining = count - written;
    }

    return written;
}

/* Get RX power string */
static int dfd_get_rx_power_str(unsigned int sff_index, char protocol_type, int lane_num, char *buf, int count)
{
    unsigned char rx_power_value[SFF_GET_VALUE_LEN_MAX];
    char sup_value;
    int ret, i;
    unsigned short raw_power;
    int power_int, power_frac;
    int written;
    int remaining;

    written = 0;
    remaining = count;
    /* get rx power support info */
    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_LANE_SPECIFIC, protocol_type, &sup_value, 1);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Failed to get lane specific, sff index: %u, ret: %d\n", sff_index, ret);
        return dfd_fill_na_strings(lane_num, "Rx", "Power  ", buf, count, written);
    }
    if (!(sup_value & SFF_RX_POWER_SUP_MASK)) {
        return dfd_fill_na_strings(lane_num, "Rx", "Power  ", buf, count, written);
    }

    ret = dfd_get_sff_e2_info(sff_index, WB_SFF_RX_POWER, protocol_type, rx_power_value, lane_num * 2);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Failed to get RX power, sff index: %u, ret: %d\n", sff_index, ret);
        return dfd_fill_na_strings(lane_num, "Rx", "Power  ", buf, count, written);
    }

    for (i = 0; i < lane_num && remaining > 0; i++) {
        raw_power = (rx_power_value[i * 2] << 8) | rx_power_value[i * 2 + 1];
        if (raw_power == 0) {
            written += snprintf(buf + written, remaining, "Rx%d Power  : 0 mW\n", i + 1);
        } else {
            power_int = raw_power / 10000;
            power_frac = (raw_power % 10000) * 100 / 10000;
            written += snprintf(buf + written, remaining, "Rx%d Power  : %d.%02d mW\n", 
                i + 1, power_int, power_frac);
        }
        remaining = count - written;
    }

    return written;
}

ssize_t dfd_get_sff_diagnostic(unsigned int sff_index, unsigned int type, char *buf, size_t count)
{
    int ret;
    char protocol_type;
    int written;
    int remaining;

    if (buf == NULL || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sff_index: %u, type: %u\n", sff_index, type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    ret = dfd_get_eth_protocol_type(sff_index, &protocol_type, TYPE_FROM_E2);
    if (ret < 0) {
        return ret;
    }

    switch(protocol_type) {
    case SFF_PROTOCOL_SFP:
    case SFF_PROTOCOL_QSFP:
        return -DFD_RV_DEV_NOTSUPPORT;
    case SFF_PROTOCOL_CMIS:
        break;
    default:
        return -DFD_RV_INVALID_VALUE;
    }

    written = 0;
    remaining = count;
    /* 1. Get temperature */
    written += dfd_get_temperature_str(sff_index, protocol_type, buf + written, remaining);
    remaining = count - written;

    /* 2. Get voltage */
    if (remaining > 0) {
        written += dfd_get_voltage_str(sff_index, protocol_type, buf + written, remaining);
        remaining = count - written;
    }
    
    /* 3. Get TX bias current */
    if (remaining > 0) {
        written += dfd_get_tx_bias_str(sff_index, protocol_type, SFF_LANE_NUM, buf + written, remaining);
        remaining = count - written;
    }
    
    /* 4. Get TX power */
    if (remaining > 0) {
        written += dfd_get_tx_power_str(sff_index, protocol_type, SFF_LANE_NUM, buf + written, remaining);
        remaining = count - written;
    }
    
    /* 5. Get RX power */
    if (remaining > 0) {
        written += dfd_get_rx_power_str(sff_index, protocol_type, SFF_LANE_NUM, buf + written, remaining);
    }

    return (ssize_t)written;
}

/**
 * dfd_get_sff_power_status_mix_default_value 
 * return: Success : the value present the power-on states of all Ethernet ports are inconsistent,
 *       : Failed: A negative value is returned
 */
int dfd_get_sff_power_status_mix_default_value(void) {
    uint64_t key;
    int *p_mix_default_value;

    /* mix value, determine whether to use the configured default value */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_POWER_STATUS_MIX_DEFAULT_VALUE, 0, 0);
    p_mix_default_value = dfd_ko_cfg_get_item(key);
    if (p_mix_default_value == NULL) {
        DFD_SFF_DEBUG(DBG_WARN, "p_mix_default_value, is null. use default.\n");
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return *p_mix_default_value;
}

/**
 * dfd_set_sff_cpld_info - Example Set the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @value: Writes the value to the register
 * return: Success :0
 *       : Failed: A negative value is returned
 */
int dfd_set_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, int value)
{
    uint64_t key;
    int ret;
    int reg_value;

    if ((value != 0) && (value != 1)) {
        DFD_SFF_DEBUG(DBG_ERROR, "sff%u cpld reg type %d, can't set invalid value: %d\n",
            sff_index, cpld_reg_type, value);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_set_init_cmd(WB_MAIN_DEV_SFF, sff_index, cpld_reg_type);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set sff%u cpld reg type %d error, ret: %d.\n",
            sff_index, cpld_reg_type, ret);
        return ret;
    }

    reg_value = value;
    ret = dfd_get_reg_key(WB_MAIN_DEV_SFF, cpld_reg_type, value, &reg_value);
    if (ret < 0) {
        if (ret != -DFD_RV_DEV_NOTSUPPORT) {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff type %d reg key err[%d].\n", cpld_reg_type, ret);
            return ret;
        }
        DFD_SFF_DEBUG(DBG_WARN, "get sff type %d reg key failed[%d], using default value.\n", cpld_reg_type, ret);
        reg_value = value;
    }

    DFD_SFF_DEBUG(DBG_VERBOSE, "set sff%u cpld reg type %d value 0x%x\n", sff_index, cpld_reg_type, reg_value);
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_set_int(key, reg_value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set sff%u cpld reg type %d error, key_name: %s, ret: %d.\n",
            sff_index, cpld_reg_type, key_to_name(DFD_CFG_ITEM_SFF_CPLD_REG), ret);
        return ret;
    }

    return DFD_RV_OK;
}

int dfd_set_sff_cpld_value(unsigned int sff_index, unsigned int cpld_reg_type, void * val, unsigned int len)
{
    int status;

    if (!val) {
        DFD_SFF_DEBUG(DBG_ERROR, "Invalid parameters\n");
        return -DFD_RV_INVALID_VALUE;
    }

    status = *((int *)val);
    return dfd_set_sff_cpld_info(sff_index, cpld_reg_type, status);
}

ssize_t dfd_get_sff_cpld_info_value(unsigned int sff_index, int cpld_reg_type, int *sysfs_decode_value)
{
    uint64_t key;
    int ret, value;

    if (sysfs_decode_value == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, sysfs_decode_value is NULL. sff_index: %u, cpld_reg_type: %d\n", sff_index, cpld_reg_type);
        return -DFD_RV_INVALID_VALUE;
    }

    /* get sff status form different reg */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_STATUS, sff_index, cpld_reg_type);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0 && ret != -DFD_RV_DEV_NOTSUPPORT) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff%u cpld status type %d error, key_name: %s, ret: %d\n",
            sff_index, cpld_reg_type, key_to_name(DFD_CFG_ITEM_SFF_CPLD_STATUS), ret);
        return ret;
    }

    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
        ret = dfd_info_get_int(key, &value, NULL);
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff%u cpld reg type %d error, key_name: %s, ret: %d\n",
                sff_index, cpld_reg_type, key_to_name(DFD_CFG_ITEM_SFF_CPLD_REG), ret);
            return ret;
        }
    }

    *sysfs_decode_value = value;
    ret = dfd_get_sysfs_decode(WB_MAIN_DEV_SFF, cpld_reg_type, value, sysfs_decode_value);
    if (ret < 0) {
        if (ret != -DFD_RV_DEV_NOTSUPPORT) {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff type %d value[0x%x] map to sysfs value table failed[%d], using default value.\n", 
                cpld_reg_type, value, ret);
            return ret;
        }
        DFD_SFF_DEBUG(DBG_WARN, "get sff type %d value[0x%x] map to sysfs value table failed[%d], using default value.\n", 
                cpld_reg_type, value, ret);
        *sysfs_decode_value = value;
    }

    return 0;
}

/**
 * dfd_get_sff_cpld_info - Obtain the CPLD register status of the optical module
 * @sff_index: Optical module number, starting from 1
 * @cpld_reg_type: Optical module CPLD register type
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, unsigned int cpld_reg_type, char *buf, size_t count)
{
    int ret, sysfs_decode_value;

    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL. sff_index: %u, cpld_reg_type: %d\n", sff_index, cpld_reg_type);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "buf size error, count: %zu, sff index: %u, cpld_reg_type: %d\n", count, sff_index, cpld_reg_type);
        return -DFD_RV_INVALID_VALUE;
    }
    mem_clear(buf, count);

    sysfs_decode_value = 0;
    ret = dfd_get_sff_cpld_info_value(sff_index, cpld_reg_type, &sysfs_decode_value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff type %d map to sysfs value table failed[%d], using default value.\n", 
            cpld_reg_type, ret);
        return ret;
    }

    return (ssize_t)snprintf(buf, count, "%d\n", sysfs_decode_value);
}

/**
 * dfd_get_sff_e2_page_info - Obtain the optical module E2 page info 
 * @sff_index: Optical module number, starting from 1
 * @page_type: 0-low page(offset 0~127); 1-high page(offset 128~255)
 * @buf: Optical module E2 receives low/high page information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_e2_page_info(unsigned int sff_index, int page_type, char *buf, size_t count)
{
    uint64_t key_status, key_data;
    int ret, status, read_len, i;

    read_len = -DFD_RV_DEV_FAIL;
    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL. sff_index: %u, page_type: %d\n",
            sff_index, page_type);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "buf size error, count: %zu, sff index: %u, page_type: %d\n",
            count, sff_index, page_type);
        return -DFD_RV_INVALID_VALUE;
    }

    for (i = 0; i < SFF_POLLING_RETRY_TIMES; i++) {
        if (i > 0) {
            usleep_range(SFF_POLLING_RETRY_MSLEEP, SFF_POLLING_RETRY_MSLEEP + 1);
        }

        key_status = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_E2_PAGE_STATUS, sff_index, page_type);
        ret = dfd_info_get_int(key_status, &status, NULL);
        if (ret < 0) {
            if (ret == -DFD_RV_DEV_NOTSUPPORT) {
                DFD_SFF_DEBUG(DBG_VERBOSE, "no support to get sff%u page type %d status\n",
                    sff_index, page_type);
                return ret;
            }
            DFD_SFF_DEBUG(DBG_ERROR, "get sff%u page type %d status 1 error, key_name: %s, ret: %d\n",
                sff_index, page_type, key_to_name(DFD_CFG_ITEM_SFF_E2_PAGE_STATUS), ret);
            continue;
        }

        if (status != 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "status error, status1: 0x%x. sff_index: %u, page_type: %d\n",
                status, sff_index, page_type);
            ret = -DFD_RV_DEV_FAIL;
            continue;
        }
        
        mem_clear(buf, count);
        key_data = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_E2_PAGE_INFO, sff_index, page_type);
        ret = dfd_info_get_buf(key_data, buf, count, NULL);
        if (ret < 0) {
            if (ret == -DFD_RV_DEV_NOTSUPPORT) {
                DFD_SFF_DEBUG(DBG_VERBOSE, "no support to get sff%u page type %d info\n",
                    sff_index, page_type);
                return ret;
            }
            DFD_SFF_DEBUG(DBG_ERROR, "get sff%u page type %d info error, key_name: %s, ret: %d\n",
                sff_index, page_type, key_to_name(DFD_CFG_ITEM_SFF_E2_PAGE_INFO), ret);
            continue;
        }
        read_len = ret;

        ret = dfd_info_get_int(key_status, &status, NULL);
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff%u page type %d status 2 error, key_name: %s, ret: %d\n",
                sff_index, page_type, key_to_name(DFD_CFG_ITEM_SFF_E2_PAGE_STATUS), ret);
            continue;
        }

        if (status != 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "status error, status2: 0x%x. sff_index: %u, page_type: %d\n",
                status, sff_index, page_type);
            ret = -DFD_RV_DEV_FAIL;
            continue;
        }

        break;
    }
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff%u page type %d info error\n",
            sff_index, page_type);
        return ret;
    }
    return read_len;
}

/**
 * dfd_get_sff_temp - Obtain the optical module temp info 
 * @sff_index: Optical module number, starting from 1
 * @buf: Optical module temp information
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_sff_temp(unsigned int sff_index, char *buf, size_t count)
{
    int ret;
    int temp_mode;
    int temp_mC;

    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL. sff_index: %u\n",
            sff_index);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "buf size error, count: %zu, sff index: %u\n",
            count, sff_index);
        return -DFD_RV_INVALID_VALUE;
    }

    temp_mode = dfd_get_sff_temp_type();
    switch (temp_mode) {
    case SFP_TEMP_TYPE_PAGE:
        ret = get_temperature_mC(sff_index, TYPE_FROM_PAGE, &temp_mC);
        break;
    case SFP_TEMP_TYPE_EEPROM:
        ret = get_temperature_mC(sff_index, TYPE_FROM_E2, &temp_mC);
        break;
    default:
        return temp_mode;
    }

    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff%u temp failed, mode: %d, ret: %d\n",
            sff_index, temp_mode, ret);
        return ret;
    }

    mem_clear(buf, count);

    return (ssize_t)snprintf(buf, count, "%d\n", temp_mC);
}

static int get_optoe_type_path_by_bus(int index, char *optoe_type_path, size_t count) 
{
    uint64_t key;
    int *i2c_bus;

    /* Obtain the eeprom bus*/
    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_BUS, WB_MAIN_DEV_SFF, index);
    i2c_bus = dfd_ko_cfg_get_item(key);
    if (i2c_bus == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff%d i2c bus fail.\n", index);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    snprintf(optoe_type_path, count, SFF_OPTOE_TYPE_PATH, *i2c_bus);
    DFD_SFF_DEBUG(DBG_VERBOSE, "sff%d, i2c_bus: %d, optoe_type_path: %s\n", index, *i2c_bus, optoe_type_path);
    return 0;
}

/**
 * dfd_get_single_eth_optoe_type - get sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_optoe_type(unsigned int sff_index, int *optoe_type)
{
    uint64_t key;
    int ret, value;
    uint8_t temp_type;
    char optoe_type_path[OPTOE_TYPE_PATH_SIZE];

    mem_clear(optoe_type_path, sizeof(optoe_type_path));
    ret = get_optoe_type_path_by_bus(sff_index, optoe_type_path, sizeof(optoe_type_path));
    if (ret == 0) {
        temp_type = 0;
        ret = dfd_ko_read_file(optoe_type_path, 0, &temp_type, OPTOE_TYPE_RW_LEN);
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "read optoe_type failed, loc: %s, ret: %d,\n", optoe_type_path, ret);
        } else {
            DFD_SFF_DEBUG(DBG_VERBOSE, "read optoe_type success, loc: %s, rd_len: %d,\n", optoe_type_path, ret);
            *optoe_type = temp_type - '0';
        }
        return ret;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_OPTOE_TYPE, sff_index, 0);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff optoe type error, key_name: %s, ret:%d.\n", 
			key_to_name(DFD_CFG_ITEM_SFF_OPTOE_TYPE), ret);
        return ret;
    }

    /* assic int to int */
    *optoe_type = value - '0';
    return ret;
}

/**
 * dfd_set_single_eth_optoe_type - set sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_set_single_eth_optoe_type(unsigned int sff_index, int optoe_type)
{
    uint64_t key;
    int ret, value;
    uint8_t temp_type;
    char optoe_type_path[OPTOE_TYPE_PATH_SIZE];

    mem_clear(optoe_type_path, sizeof(optoe_type_path));
    ret = get_optoe_type_path_by_bus(sff_index, optoe_type_path, sizeof(optoe_type_path));
    if (ret == 0) {
        temp_type = (uint8_t)optoe_type + '0';
        ret = dfd_ko_write_file(optoe_type_path, 0, &temp_type, OPTOE_TYPE_RW_LEN);
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "write optoe_type failed, loc: %s, ret: %d,\n", optoe_type_path, ret);
        } else {
            DFD_SFF_DEBUG(DBG_VERBOSE, "write optoe_type success, loc: %s, rd_len: %d,\n", optoe_type_path, ret);
        }
        return ret;
    }

    /* int to assic int */
    value = optoe_type + '0';
    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_OPTOE_TYPE, sff_index, 0);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set sff optoe type error, key_name: %s, ret:%d.\n", 
			key_to_name(DFD_CFG_ITEM_SFF_OPTOE_TYPE), ret);
        return ret;
    }

    return ret;
}

/**
 * dfd_get_single_eth_i2c_bus - get sff i2c_bus
 * @sff_index: Optical module number, starting from 1
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_i2c_bus(unsigned int sff_index, char *buf, size_t count)
{
    uint64_t key;
    int *i2c_bus;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_EEPROM_BUS, WB_MAIN_DEV_SFF, sff_index);
    i2c_bus = dfd_ko_cfg_get_item(key);
    if (i2c_bus == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff i2c bus error, key_name: %s.\n", key_to_name(DFD_CFG_ITEM_EEPROM_BUS));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return (ssize_t)snprintf(buf, count, "%d\n", *i2c_bus);
}

/**
 * dfd_get_single_eth_cage_type - get sff cage_type
 * @sff_index: Optical module number, starting from 1
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_cage_type(unsigned int sff_index, char *buf, size_t count)
{
    uint64_t key;
    int *cage_type;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CAGE_TYPE, sff_index, 0);
    cage_type = dfd_ko_cfg_get_item(key);
    if (cage_type == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff cage type error, key_name: %s.\n", 
            key_to_name(DFD_CFG_ITEM_SFF_CAGE_TYPE));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return (ssize_t)snprintf(buf, count, "%d\n", *cage_type);
}

/**
 * dfd_get_single_eth_power_group - get sff port power group
 * @sff_index: Optical module number, starting from 1
 * @power_group: power group id
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_power_group(unsigned int sff_index, int *power_group)
{
    uint64_t key;
    int *p_value;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_POWER_GROUP, sff_index, 0);
    p_value = dfd_ko_cfg_get_item(key);
    if (p_value == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff power group id error, key_name: %s.\n",
                      key_to_name(DFD_CFG_ITEM_SFF_POWER_GROUP));
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    *power_group = *p_value;
    return 0;
}

/**
 * dfd_get_single_eth_port_bus_status - get sff bus status
 * @sff_index: Optical module number, starting from 1
 * @buf: Optical module E2 receives information from buf
 * @count: buf length
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
int dfd_get_single_eth_port_bus_status(unsigned int sff_index, char *buf, size_t count)
{
    ssize_t ret;
    ssize_t upg_ret;
    int status;
    char tmp_byte;

    /* check i2c bus status by reading data from the optical module's E2 */
    ret = dfd_read_eeprom_data(WB_MAIN_DEV_SFF, sff_index, &tmp_byte, 0, 1);
    if (ret < 0) {
        upg_ret = dfd_read_eeprom_upg_data(WB_MAIN_DEV_SFF, sff_index, &tmp_byte, 0, 1);
        if (upg_ret < 0) {
            if (ret == -DFD_RV_DEV_NOTSUPPORT && upg_ret == -DFD_RV_DEV_NOTSUPPORT) {
                return -DFD_RV_DEV_NOTSUPPORT;
            }
            status = SFF_I2C_STATUS_ABNORMAL;
        } else {
            status = SFF_I2C_STATUS_NORMAL;
        }
    } else {
        status = SFF_I2C_STATUS_NORMAL;
    }
    return (ssize_t)snprintf(buf, count, "%d\n", status);
}

/**
 * dfd_set_sff_power_group_state - Set the power state of an SFP power group.
 * @group: Power group number, starting from 1.
 * @group_state: Power state to set for the group (0 for off, 1 for on).
 *
 * Returns: 0 on success, or a negative error code on failure.
 */
int dfd_set_sff_power_group_state(int group, int group_state)
{
    uint64_t key;
    int ret;

    /** Validate the power state value */
    if ((group_state != 0) && (group_state != 1)) {
        DFD_SFF_DEBUG(DBG_ERROR, "Power group %u, can't set invalid value: %d\n",
                      group, group_state);
        return -DFD_RV_INVALID_VALUE;
    }

    /** Construct the key for the power group configuration item */
    key = DFD_CFG_KEY(DFD_CFG_ITEM_PORT_POWER_GROUP_REG, group, 0);

    /** Set the power state for the specified SFP power group */
    ret = dfd_info_set_int(key, group_state);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "Set power group %u error, key_name: %s, ret: %d.\n",
                      group, key_to_name(DFD_CFG_ITEM_PORT_POWER_GROUP_REG), ret);
        return ret;
    }

    DFD_SFF_DEBUG(DBG_VERBOSE, "Successfully set power state for power group %u to %d.\n",
                  group, group_state);
    return DFD_RV_OK;
}

/**
 * dfd_get_sff_power_group_state - Obtain the power group state.
 * @group: Power group number, starting from 1.
 * @buf: Buffer to store the power group state information.
 * @count: buf length.
 * return: Success: Returns the length of fill buf.
 *        Failed: A negative value is returned.
 */
ssize_t dfd_get_sff_power_group_state(int group, char *buf, size_t count)
{
    uint64_t key;
    int ret, power_group_state;

    /* Check for NULL buffer and non-positive count */
    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL. group: %u\n", group);
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "buf size error, count: %zu, group: %u\n", count, group);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_PORT_POWER_GROUP_REG, group, 0);

    ret = dfd_info_get_int(key, &power_group_state, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get power group %u state error, key_name: %s, ret: %d\n",
                      group, key_to_name(DFD_CFG_ITEM_PORT_POWER_GROUP_REG), ret);
        return ret;
    }

    DFD_SFF_DEBUG(DBG_VERBOSE, "Successfully get power state for power group %u state: %d.\n",
                  group, power_group_state);

    return (ssize_t)snprintf(buf, count, "%d\n", power_group_state);
}

ssize_t dfd_get_transceiver_power_on_status_str(char *buf, size_t count)
{
    ssize_t ret;
    unsigned int eth_index, eth_num;
    int power_on_status, power_on_eth_num;
    int rv;

    /* Check for NULL buffer and non-positive count */
    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }
    if (count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "buf size error, count: %zu\n", count);
        return -DFD_RV_INVALID_VALUE;
    }

    rv = dfd_get_dev_number(WB_MAIN_DEV_SFF, WB_MINOR_DEV_NONE);
    if (rv <= 0) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
    }
    eth_num = rv;

    mem_clear(buf, count);
    power_on_status = 0;
    power_on_eth_num = 0;

    for (eth_index = 1; eth_index <= eth_num; eth_index++) {
        DFD_SFF_DEBUG(DBG_VERBOSE, "eth index: %u\n", eth_index);
        ret = dfd_get_sff_cpld_info_value(eth_index, WB_SFF_POWER_ON, &power_on_status);
        if (ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "get eth%u power status failed, ret: %zd\n", eth_index, ret);
            return ret;
        }
        if (power_on_status) {
            power_on_eth_num++;
        }
    }

    /* mix value, determine whether to use the configured default value */
    if (power_on_eth_num > 0 && power_on_eth_num < eth_num) {
        power_on_status = dfd_get_sff_power_status_mix_default_value();
    }

    if (power_on_status < 0) {
        if (power_on_status == -DFD_RV_DEV_NOTSUPPORT) {
            power_on_status = SFF_POWER_STATUS_MIX_DEFAULT_VALUE;
        } else {
            DFD_SFF_DEBUG(DBG_ERROR, "get sff power on status failed, status: %d\n", power_on_status);
            return power_on_status;
        }
    }

    return (ssize_t)snprintf(buf, count, "%d\n", power_on_status);

}

/**
 * dfd_get_sff_present_status - Obtain the optical module present status
 * @sff_index: Optical module number, starting from 1
 * return: 0:ABSENT
 *         1:PRESENT
 *       : Negative value - Read failed
 */
int dfd_get_sff_present_status(unsigned int sff_index)
{
    int ret;
    int sysfs_decode_value;

    sysfs_decode_value = -1;

    ret = dfd_get_sff_cpld_info_value(sff_index, WB_SFF_MODULE_PRESENT, &sysfs_decode_value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff%u present status error, ret: %d\n", sff_index, ret);
        return ret;
    }

    return sysfs_decode_value;
}


