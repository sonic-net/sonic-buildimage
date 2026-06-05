/********************************************************************************
 * Copyright(C) 2020 Micas Network. All rights reserved.
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期        | 作者     |  修改记录
**===============================================================================
**| 2024/08/23  | zhoutenghui  |  创建该文件
**
*********************************************************************************/
#include "sap_api.h"
#include "sap_common.h"

#define EXTPHY_API_EXEC(_func, args...) \
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) { \
        if (g_platform_inited[i] == false) { \
            continue; \
        } \
        if (sap_platform_port_support(unit, port, i) == false) { \
            continue; \
        } \
        if (sap_apis[i]->##_func## == NULL) { \
            continue; \
        } \
        return sap_apis[i]->##_func##(##args); \
    }

// #define API_FILTER(_api) 
extern sap_apis_t sap_extphy_aperta2_apis;
extern sap_apis_t sap_extphy_aperta_apis;
extern sap_apis_t sap_extphy_barchetta_apis;
extern sap_apis_t sap_extphy_credo_88322_apis;
extern sap_apis_t sap_extphy_credo_55321_apis;

static sap_apis_t *sap_apis[EXTPHY_APIS_NUMBER] = {
    &sap_extphy_aperta2_apis,
    &sap_extphy_aperta_apis,
    &sap_extphy_barchetta_apis,
    &sap_extphy_credo_88322_apis,
    &sap_extphy_credo_55321_apis
};

static bool sap_platform_port_support(int unit, int port, int platform_id);

static bool g_platform_inited[EXTPHY_APIS_NUMBER] = {0};

int sap_platform_init(int unit, bool warm_boot)
{
    int i, rv;
    int platform_cnt = 0;
    int platform_init_cnt = 0;
    SAP_LOG_INFO("sap_platform_init start.");
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (sap_apis[i]->sap_platform_init == NULL) {
            SAP_LOG_DBG("chip:%s sap_platform_init is null.", sap_apis[i]->type);
            continue;
        }
        rv = sap_apis[i]->sap_platform_init(unit, warm_boot);
        if (rv != SAP_STATUS_NOT_SUPPORTED) {
            platform_cnt++;
        }
        if (rv == SAP_STATUS_SUCCESS) {
            platform_init_cnt++;
            g_platform_inited[i] = true;
        } else if (rv == SAP_STATUS_FAILURE) {
            SAP_LOG_ERR("sap_platform_init failed, platform_id: %d", i);
        }
    }
    SAP_LOG_INFO("sap_platform_init end.");
    if (platform_cnt) {
        if (platform_cnt == platform_init_cnt) {
            return SAP_STATUS_SUCCESS;
        } else {
            return SAP_STATUS_FAILURE;
        }
    } else {
        return SAP_STATUS_NOT_SUPPORTED;
    }
}

static bool sap_platform_port_support(int unit, int port, int platform_id)
{
    if (platform_id < 0 || platform_id >= EXTPHY_APIS_NUMBER) {
        return false;
    }
    if (g_platform_inited[platform_id] == false) {
        return false;
    }
    if (sap_apis[platform_id]->sap_port_support == NULL) {
        return false;
    }
    if (sap_apis[platform_id]->sap_port_support(unit, port, -1) == true) {
        return true;
    }
    return false;
}

bool sap_port_support(int unit, int port)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_support == NULL) {
            continue;
        }
        if (sap_apis[i]->sap_port_support(unit, port, -1)) {
            return true;
        }
    }
    return false;
}

bool sap_physical_port_support(int unit, int physical_port)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_support == NULL) {
            continue;
        }
        if (sap_apis[i]->sap_port_support(unit, -1, physical_port)) {
            return true;
        }
    }
    return false;
}

int sap_port_speed_set(int unit, int port, int speed)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_speed_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_speed_set(unit, port, speed);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_shell_cmd_run(int unit, int port, char *cmdline)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_shell_cmd_run == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_shell_cmd_run(unit, port, cmdline);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

void sap_techsupport(int unit, int port)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_apis[i]->sap_techsupport == NULL) {
            continue;
        }
        sap_apis[i]->sap_techsupport(unit, port);
    }
}

int sap_temperature_get(int unit, int port, double *temp)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_temperature_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_temperature_get(unit, port, temp);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_highest_temperature_get(double *temp)
{
    int i;
    int err;
    double tmp_temp;

    *temp = 0;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_apis[i]->sap_highest_temperature_get == NULL) {
            continue;
        }
        err = sap_apis[i]->sap_highest_temperature_get(&tmp_temp);
        if (err) {
            SAP_LOG_ERR("highest_temperature_get failed, err: %d", err);
            return err;
        }
        if (tmp_temp > *temp) {
            *temp = tmp_temp;
        }
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_port_linktrain_set(int unit, int port, int if_side, int enable)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_linktrain_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_linktrain_set(unit, port, if_side, enable);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_port_fec_get(int unit, int port, int if_side, sap_fec_e *fec)
{
    int i, rv;

    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_fec_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_fec_get(unit, port, if_side, fec);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_port_prbs_set(int unit, int port, int poly, int inv, int if_side, int lane)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_prbs_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_prbs_set(unit, port, poly, inv, if_side, lane);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_port_prbs_get(int unit, int port, int if_side, sap_prbs_status_t *prbs_info)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_prbs_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_prbs_get(unit, port, if_side, prbs_info);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_port_prbs_ber_get(int unit, int port, int if_side, int time_v, sap_prbs_status_t *prbs_info)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_prbs_ber_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_prbs_ber_get(unit, port, if_side, time_v, prbs_info);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_loopback_set(int unit, int port, int if_side, int lb_dir, int enable)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_loopback_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_loopback_set(unit, port, if_side, lb_dir, enable);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_loopback_get(int unit, int port, int if_side, int lb_dir, int *enable)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_loopback_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_loopback_get(unit, port, if_side, lb_dir, enable);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_autoneg_set(int unit, int port, int if_side, int enable)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_autoneg_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_autoneg_set(unit, port, if_side, enable);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_autoneg_get(int unit, int port, int if_side, int *enable)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_autoneg_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_autoneg_get(unit, port, if_side, enable);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_phy_status_dump(int unit, int port, int level)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_phy_status_dump == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_phy_status_dump(unit, port, level);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_dsc_dump(int unit, int port, int if_side, int flag, int lane)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_dsc_dump == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_dsc_dump(unit, port, if_side, flag, lane);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_eyescan_dump(int unit, int port, int if_side, int lane)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_eyesacn_dump == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_eyesacn_dump(unit, port, if_side, lane);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_port_prbs_clear(int unit, int port, int if_side)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_prbs_clear == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_prbs_clear(unit, port, if_side);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_squelch_set(int unit, int port, int if_side, int tx_rx, int enable, int lane)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_squelch_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_squelch_set(unit, port, if_side, tx_rx, enable, lane);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_polarity_set(int unit, int port, int if_side, int tx_rx, uint32_t polarity, bool override)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_polarity_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_polarity_set(unit, port, if_side, tx_rx, polarity, override);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_polarity_get(int unit, int port, int if_side, int tx_rx, uint32_t *polarity)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_polarity_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_polarity_get(unit, port, if_side, tx_rx, polarity);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_phy_reset(int unit, int port, int flags)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_phy_reset == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_phy_reset(unit, port, flags);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_tx_fir_set(int unit, int port, int if_side, int lane, sap_tx_fir_t tx_fir)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_tx_fir_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_tx_fir_set(unit, port, if_side, lane, tx_fir);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_tx_fir_get(int unit, int port, int if_side, int lane, sap_tx_fir_t *tx_fir)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_tx_fir_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_tx_fir_get(unit, port, if_side, lane, tx_fir);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_reg_set(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t data)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_reg_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_reg_set(unit, port, if_side, devaddr, regaddr, flag, data);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_reg_get(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t *data)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_reg_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_reg_get(unit, port, if_side, devaddr, regaddr, flag, data);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_mib_dump(int unit, int port, int if_side)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_mib_dump == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_mib_dump(unit, port, if_side);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_channel_reach_set(int unit, int port, int if_side, int lane, int nr_er)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_channel_reach_set == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_channel_reach_set(unit, port, if_side, lane, nr_er);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_fw_version_get(int unit, int port, sap_version_t *version_info)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_fw_version_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_fw_version_get(unit, port, version_info);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_eeprom_update(int unit, int port, bool use_file)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_eeprom_update == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_eeprom_update(unit, port, use_file);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}

int sap_port_list_get(sap_port_list_t *port_list)
{
    int i, rv;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        port_list[i].list = NULL;
        port_list[i].port_count = 0;
        port_list[i].type = sap_apis[i]->type;
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_list_get == NULL) {
            continue;
        }
        rv = sap_apis[i]->sap_port_list_get(&port_list[i]);
        if (rv != SAP_STATUS_SUCCESS) {
            port_list[i].list = NULL;
            port_list[i].port_count = 0;
        }
    }
    return SAP_STATUS_SUCCESS;
}

int sap_port_status_get(int unit, int port, sap_port_status_t *port_status)
{
    int i;
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (g_platform_inited[i] == false) {
            continue;
        }
        if (sap_platform_port_support(unit, port, i) == false) {
            continue;
        }
        if (sap_apis[i]->sap_port_status_get == NULL) {
            return SAP_STATUS_NOT_IMPLEMENTED;
        }
        return sap_apis[i]->sap_port_status_get(unit, port, port_status);
    }
    return SAP_STATUS_NOT_SUPPORTED_PORT;
}
