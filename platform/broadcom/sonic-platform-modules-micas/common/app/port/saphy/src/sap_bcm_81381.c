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
/* Includes */
#include <epdm.h>
#include <epdm_sec.h>

#include "sap_api.h"
#include "sap_common.h"
#include "sap_mdio.h"
#include "epdm_bcm_common_defines.h"
#include "list.h"

#define SAP_BCM_LOG_ERR          SAP_LOG_ERR
#define SAP_BCM_LOG_WARN         SAP_LOG_WARN
#define SAP_BCM_LOG_DBG          SAP_LOG_DBG
#define SAP_BCM_LOG_INFO         SAP_LOG_INFO
#define SAP_BCM_LOG_NOTICE       SAP_LOG_NOTICE

#define uint32_t unsigned int
#define uint8_t  unsigned short int
#define BCM_FAULT_TERMINATE_GENERATE    (0)
#define BCM_FAULT_PASS_THROUGH          (1)
#define BCM_IF_SIDE_STR(if_side)        ((if_side==PLP_SYS_IF_SIDE)?"sys":"line")
#define MAX_PHY_NUM          (64)
#define BCM_PHY_MAX_LANE     (8)
#define ALL_LANE_MAP         (0xFF)
#define BCM_PHY_MAX_PORT_NUM (16)
#define BCM_INVALID_PORT     (-1)
#define MAX_LINE_STRING_LEN  (128)
#define DEFAULT_IF_MODE      bcmplpInterfaceModeIEEE
#define REF_CLOCK            bcm_pm_RefClk156Mhz
#define SYS_LL_MODE             0x3
#define SYS_CLOCK_MODE          0
#define SYS_FAILOVER_LANEMAP    0x00
#define LINE_LL_MODE            0x3
#define LINE_CLOCK_MODE         0
#define LINE_FAILOVER_LANEMAP   0x00
#define PORT_LANE_SPEED_10G  (10000)
#define PORT_LANE_SPEED_20G  (20000)
#define PORT_LANE_SPEED_25G  (25000)
#define PORT_LANE_SPEED_50G  (50000)
#define PORT_LANE_SPEED_100G (100000)
#define SPEED_10G            (10000)
#define SPEED_25G            (25000)
#define SPEED_40G            (40000)
#define SPEED_100G           (100000)
#define SPEED_200G           (200000)
#define SPEED_400G           (400000)
#define SPEED_800G           (800000)
#define PLP_LB_DIGITAL       (1)
#define PLP_LB_REMOTE        (2)
#define CHIP_NAME            "barchetta"
#define BCM_PLP_DEVICE_AUX_MODE bcm_plp_barchetta_device_aux_modes_t
#define BCM_FW_NONE_LOAD        (0)
#define BCM_FW_UNICAST_LOAD     (1)
#define BCM_FW_BROADCAST_LOAD   (2)
#define BCM_FW_EEPROM_LOAD      (3)
#define BCM_FW_UPGRADE          (4)
#define BCM_FW_LOAD_SKIP        (5)
#define BCM_FW_LOAD_FP          (6)  /* FULL process */
#define AVS_FW_control_Adr            (0xA3A0) /*FW_REGS_fwreg_3A0_Adr*/
#define AVS_FW_status_Adr             (0xA3A1) /*FW_REGS_fwreg_3A1_Adr*/
#define AVS_regulator0_config_Adr     (0xA3A2) /*FW_REGS_fwreg_3A2_Adr*/
#define AVS_regulator1_config_Adr     (0xA3A3) /*FW_REGS_fwreg_3A3_Adr*/
#define AVS_I2C_address0_Adr          (0xA3A4) /*FW_REGS_fwreg_3A4_Adr*/
#define AVS_I2C_address1_Adr          (0xA3A5) /*FW_REGS_fwreg_3A5_Adr*/
#define AVS_Vout_control_Adr          (0xA3A6) /*FW_REGS_fwreg_3A6_Adr*/
#define AVS_Vout_mV_status_Adr        (0x8269) /*GEN_CNTRLS_GPREeg_19_Adr*/
#define AVS_Vout_response_Adr         (0x8267) /*GEN_CNTRLS_GPREeg_17_Adr*/
#define BCM_81381_CHIP_ID_ADDR        (0x18500)
#define BCM_81381_CHIP_ID             (0x1381)
// FW version 0xd007, FW CRC 0x3537
// #define MDIO_DEBUG
// #define FW_REPAIRE_ENABLE
#define MUTIL_THREAD
#define barchetta_VCO_53G bcmplpVco53G /* bcmplpVco53G = 1 */
#define barchetta_VCO_51G bcmplpVco51G /* bcmplpVco51G = 0 */
#define MODULATION_MODE_GET(lane_data_rete) ((lane_data_rete >= bcmplpLaneDataRate_53P125G) ? bcmplpModulationPAM4 : bcmplpModulationNRZ)
#define GET_PORT_LANE_NUM(lane_map, lane_num) \
    do { \
        int bit_cnt, lane_cnt; \
        for (bit_cnt = 0, lane_cnt = 0; bit_cnt < (sizeof(lane_map) * 8); bit_cnt++) { \
            if ((lane_map & (1 << bit_cnt)) != 0) { \
                lane_cnt++; \
            } \
        } \
        lane_num = lane_cnt; \
    } while (0)

#define GET_PORT_SINGLE_LANE_INDEX(lane_map, lane_idx) \
    do { \
        int bit_cnt; \
        for (bit_cnt = 0; bit_cnt < (sizeof(lane_map) * 8); bit_cnt++) { \
            if ((lane_map & (1 << bit_cnt)) != 0) { \
                lane_idx = bit_cnt; \
                break; \
            } \
        } \
    } while (0)

#define GET_PORT_LANE_RATE(speed, lane_num, lane_rate) \
    do { \
        if ((speed / lane_num) == PORT_LANE_SPEED_10G) { \
            lane_rate = bcmplpLaneDataRate_10P3125G; \
        } else if ((speed / lane_num) == PORT_LANE_SPEED_20G) { \
            lane_rate = bcmplpLaneDataRate_20P625G; \
        } else if ((speed / lane_num) == PORT_LANE_SPEED_25G) { \
            lane_rate = bcmplpLaneDataRate_25P78125G; \
        } else if ((speed / lane_num) == PORT_LANE_SPEED_50G) { \
            lane_rate = bcmplpLaneDataRate_53P125G; \
        } else if ((speed / lane_num) == PORT_LANE_SPEED_100G) { \
            lane_rate = bcmplpLaneDataRate_106P25G; \
        } \
    } while (0)

#define GET_PORT_SINGLE_LANE_MAP(lane_map, offset, lane_offset) \
    do { \
        int bit_cnt, lane_cnt; \
        for (bit_cnt = 0, lane_cnt = 0; bit_cnt < (sizeof(lane_map) * 8); bit_cnt++) { \
            if ((lane_map & (1 << bit_cnt)) != 0) { \
                lane_cnt++; \
            } \
            if (lane_cnt == offset) {\
                lane_offset = (1 << bit_cnt);\
                break;\
            }\
        } \
    } while (0)

#define GET_LANE_SWAP_OCTAL(lane_swap_map, lane_index) \
                ((lane_swap_map) >> (4 * (lane_index)) & 0xf)

#define SAP_POINT_CHECK_RV(ptr, ptr_name, rv) \
do { \
    if (ptr == NULL) { \
        SAP_BCM_LOG_ERR("%s is null, rv:%d", ptr_name, rv); \
        return rv; \
    } \
} while(0)

#define SAP_BCM_RV_CHECK(phy_info, string, rv) \
    do { \
        if (rv != BCM_PM_IF_SUCCESS) { \
            SAP_BCM_LOG_ERR("phy_addr:%d, %s failed, rv=%d", phy_info->phy_addr, string, rv); \
            return SAP_STATUS_FAILURE; \
        } \
    } while(0)
    
#define SAP_RV_CHECK(phy_info, string, rv) \
    do { \
        if (rv != SAP_STATUS_SUCCESS) { \
            SAP_BCM_LOG_ERR("phy_addr:%d, %s failed, rv=%d", phy_info->phy_addr, string, rv); \
            return SAP_STATUS_FAILURE; \
        } \
    } while(0)

#define SAP_BCM_CFG_FILE    "/usr/share/sonic/hwsku/sap_bcm_81381.cfg"

typedef struct {
    int speed;
    int lanes;
    int fec;
    int lane_date_rate;
    bool an_supported;
} sap_bcm_speed_mode_table;

sap_bcm_speed_mode_table barchetta_speed_mode_table[] = {
    {SPEED_10G,  1, bcmplpNoFEC,     bcmpLplaneDataRate_10P3125G,  true},
    {SPEED_25G,  1, bcmplpNoFEC,     bcmplpLaneDataRate_25P78125G, true},
    {SPEED_25G,  1, bcmplpRS528,     bcmplpLaneDataRate_25P78125G, true},
};

typedef struct {
    int speed;
    int lanes;
    bool macsec_en;
    bool ptp_en;
    int latency;
} sap_bcm_latency_table;

sap_bcm_latency_table barchetta_ingress_latency_table[] = {
    {SPEED_800G, 8, false, false, 109}
};

sap_bcm_latency_table barchetta_egress_latency_table[] = {
    {SPEED_800G, 8, false, false, 109}
};

/* phy配置信息节点 */
typedef struct sap_bcm_phy_info_s {
    char    *chip_name; // barchetta
    uint8_t  loaded;
    uint8_t  unit;
    uint32_t phy_addr;
    uint8_t  card_id;
    uint8_t  mdio_id;
    uint8_t  mdio_type;
    uint32_t macsec_option;
    uint32_t ptp_option;
    uint8_t  vco_octal_0_sys;
    uint8_t  vco_octal_0_line;
    uint8_t  vco_octal_1_sys;
    uint8_t  vco_octal_1_line;
    int      tx_pol_sys;
    int      rx_pol_sys;
    int      tx_pol_line;
    int      rx_pol_line;
    int      tx_lane_map_line[2];
    int      rx_lane_map_line[2];
    int      tx_lane_map_sys[2];
    int      rx_lane_map_sys[2];
    int      profile_id;
    uint8_t  lane_num;
    uint8_t  port_num;
    int      lanes[BCM_PHY_MAX_PORT_NUM];     /* mac side physical ports */
    int      ports[BCM_PHY_MAX_PORT_NUM];     /* mac side logicl ports */
    int      ports_lane_num[BCM_PHY_MAX_PORT_NUM]; /* mac side port lane num */
    mdio_read_func mdio_read;
    mdio_write_func mdio_write;
    pthread_mutex_t mutex;
} sap_bcm_phy_info_t;

typedef struct sap_bcm_port_info_s {
    int inited;
    int unit;
    int port;
    int phy_lane0;
    int lanes;
    int speed;
    int linktrain_sys;
    int linktrain_line;
    int lanemap_sys;
    int lanemap_line;
    int fec_sys;
    int fec_line;
    int if_type_sys;
    int if_type_line;
    int force_nr_sys;
    int force_nr_line;
    int force_er_sys;
    int force_er_line;
    sap_bcm_phy_info_t *phy_info;
} sap_bcm_port_info_t;

static int sap_bcm_platform_init(int unit, bool warm_boot);
static int sap_bcm_platform_init_pthread_join(int unit);
static bool sap_bcm_port_support(int unit, int port, int physical_port);
static void sap_bcm_techsupport(int unit, int port);
static int sap_bcm_port_remove(int unit, int port);
static int sap_bcm_create_port(int unit, int port, sap_port_resource_t *port_resource);
static int sap_bcm_speed_set(int unit, int port, int speed);
static int sap_bcm_fec_set(int unit, int port, sap_fec_e fec_type);
static int sap_slot_init(int unit, int slot_id);
static int sap_barchetta_config_init(int unit);
static int sap_barchetta_fw_load(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_phy_init(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_pre_port_init(sap_bcm_port_info_t *port_info);
static int sap_barchetta_port_init(sap_bcm_port_info_t *port_info);
static int sap_barchetta_post_port_init(sap_bcm_port_info_t *port_info);
static int sap_barchetta_lane_init(int unit, int port, int if_side);
static int sap_barchetta_fw_repair(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_clean_up(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_fw_load_unicast(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_fw_load_broadcast(sap_bcm_phy_info_t **phy_info_list, int phy_num);
static int sap_barchetta_fw_load_eeprom(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_fw_load_eeprom_skip(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_fw_load_none(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_fw_load_full_process(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_fw_set_eeprom(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_fw_soft_reset(sap_bcm_phy_info_t *phy_info);
static int sap_barchetta_fw_hard_reset(sap_bcm_phy_info_t *phy_info);
static int sap_bcm_avs_init(sap_bcm_phy_info_t *phy_info);
static int sap_bcm_phy_reset(int unit, int port, int flags);
static int sap_port_info_get(int unit, int port, sap_bcm_port_info_t **port_info);
static int sap_phy_access_get(int unit, int port, int if_side, bcm_plp_access_t* phy_access);
static int sap_phy_info_get(int unit, int port, sap_bcm_phy_info_t **phy_info);
static int sap_bcm_port_prbs_set(int unit, int port, int poly, int inv, int if_side, int lane);
static int sap_bcm_port_prbs_get(int unit, int port, int if_side, sap_prbs_status_t *prbs_info);
static int sap_bcm_port_prbs_clear(int unit, int port, int if_side);
static int sap_bcm_port_prbs_ber_get(int unit, int port, int if_side, int time_v, sap_prbs_status_t *prbs_info);
static int sap_bcm_linktraining_set(int unit, int port, int if_side, int enable);
static int sap_bcm_tx_fir_set(int unit, int port, int if_side, int lane, sap_tx_fir_t tx_fir);
static int sap_bcm_tx_fir_get(int unit, int port, int if_side, int lane, sap_tx_fir_t *tx_fir);
static int sap_bcm_tx_fir_init(int unit, int port, int if_side);
static int sap_bcm_loopback_set(int unit, int port, int if_side, int lb_dir, int enable);
static int sap_bcm_loopback_get(int unit, int port, int if_side, int lb_dir, int *enable);
static int sap_bcm_autopeak_set(int unit, int port, int if_side, int enable);
static int sap_bcm_squelch_set(int unit, int port, int if_side, int tx_rx, int enable, int lane);
static int sap_bcm_reg_set(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t data);
static int sap_bcm_reg_get(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t *data);
static int sap_bcm_autoneg_set(int unit, int port, int if_side, int enable);
static int sap_bcm_autoneg_status_get(int unit, int port, int if_side, int *enable);
static int sap_bcm_polarity_set(int unit, int port, int if_side, int tx_rx, uint32_t polarity, bool override);
static int sap_bcm_polarity_get(int unit, int port, int if_side, int tx_rx, uint32_t *polarity);
static int sap_bcm_phy_status_dump(int unit, int port, int flags);
static int sap_bcm_dsc_dump(int unit, int port, int if_side, int flag, int lane);
static int sap_bcm_eyescan_dump(int unit, int port, int if_side, int lane);
static int sap_bcm_fec_mib_dump(int unit, int port, int if_side);
static int sap_bcm_mib_dump(int unit, int port, int if_side);
static int sap_bcm_get_profile_info(int profile_id, int speed, int split_num, int port_id, sap_bcm_port_info_t *port_info);
static int sap_bcm_admin_set(int unit, int port, int if_side, int enable);
static int sap_bcm_admin_get(int unit, int port, int if_side, int *enable);
static int sap_bcm_mutex_take(uint32_t phy_id, void* platform_ctxt);
static int sap_bcm_mutex_give(uint32_t phy_id, void* platform_ctxt);
static int sap_bcm_mdio_read(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data);
static int sap_bcm_mdio_write(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int data);
static int sap_bcm_channel_reach_set(int unit, int port, int if_side, int lane, int nr_er);
static int sap_bcm_temperature_get(int unit, int port, double *temp);
static int sap_bcm_flow_control_set(int unit, int port, int if_side, bcm_plp_mac_flow_control_t flow_option);
static int sap_bcm_highest_temperature_get(double *temp);
static void *sap_barchetta_phy_init_thread(void *arg);
static int  sap_bcm_version_get(int unit, int port, sap_version_t *version_info);
static int sap_bcm_eeprom_update(int unit, int port, bool use_file);
static int sap_bcm_port_list_get(sap_port_list_t *port_list);
static int sap_bcm_port_status_get(int unit, int port, sap_port_status_t *port_status);
static int sap_bcm_prbs_change(int poly, bcm_plp_pm_prbs_poly_t *bcm_poly);

static struct list_head global_port_list_head;
static uint16_t global_port_num = 0;
static sap_bcm_phy_info_t *g_bcm_phy_infos;
static int g_slot_num = 0;
static int g_phy_num = 0;
static int g_fw_load_method = 0;
static bool g_warm_boot = false;
static int g_i2c_slave_en = 0;
static int g_avs_i2c_resp_addr = 0;

static uint32_t get_bits(uint32_t value, uint32_t mask) {
    return value & mask;
}

static uint32_t set_bits(uint32_t value, uint32_t mask, uint32_t bits) {
    value &= ~mask;
    return value | (bits & mask);
}

static uint32_t get_mask(int offset, int len) {
    int i;
    int mask;
    for (i=offset; i<len; i++) {
        mask |= 0x1<<i;
    }
    return mask;
}

static int sap_bcm_platform_init(int unit, bool warm_boot)
{
    int slot_id, rv;

    INIT_LIST_HEAD(&global_port_list_head);
    if (fileExists(SAP_BCM_CFG_FILE) != 1) {
        SAP_BCM_LOG_ERR("%s no found", SAP_BCM_CFG_FILE);
        return SAP_STATUS_NOT_SUPPORTED;
    }
    rv = sap_barchetta_config_init(unit);
    if (rv == SAP_STATUS_NOT_SUPPORTED) {
        return rv;
    } else if (SAP_STATUS_SUCCESS != rv) {
        SAP_BCM_LOG_ERR("sap_bcm_config_init fail");
        return SAP_STATUS_FAILURE;
    }
    if (warm_boot) {
        g_warm_boot = true;
    }

    for (slot_id = 0; slot_id < g_slot_num; slot_id++) {
        if (sap_slot_init(unit, slot_id) != SAP_STATUS_SUCCESS) {
#ifndef SAPHY_BINARY
            return SAP_STATUS_FAILURE;
#endif
        }
    }
    return SAP_STATUS_SUCCESS;
}

/* 普通降速 */
// 仅修改lane_data_rate和fec
/* flexport降速 */
// 需要core_reset并重新设置vco
// bcm_port_resource_speed_config_validate 通过这个函数判断是否需要重置vco（多端口同时降速）
// sap_extphy_speed_config_validate
// sap_extphy_fec_config_validate
// validate结果有两种：
// 一：支持但是需要重设vco，重设vco会导致同phy端口震荡，因此需要mac侧先shutdown这些端口。
// 重设vco还会导致同octal端口同时降速，因此需要mac侧也配合同时降速。
// 二：完全不支持
/* 单octal多端口 和 单octal单端口 */
/* 单octal多端口 进行flex降速需要实现多端口同时降速 */
// static void* plp_slot_monitor_thread(void *arg)


static int sap_bcm_fec_set(int unit, int port, sap_fec_e fec_type)
{
    // 改变vco需要init_fw，只是改变speed需要mode_set
    return 0;
}

static int sap_bcm_speed_set(int unit, int port, int speed)
{
    // 改变vco需要init_fw，只是改变speed需要mode_set
    return 0;
}

// typedef struct {
//     int phy_lane0;
//     int speed;
//     int host_lanes;
//     int line_lanes;
//     sap_fec_e host_fec_type;
//     sap_fec_e line_fec_type;
//     int link_training;
// } sap_port_resource_t;

static int sap_bcm_create_port(int unit, int port, sap_port_resource_t *port_resource)
{
    // 创建对应的port list node，并检查是否可以初始化phy（）
    // 需要有当前Phy的vco配置缓存，有SDK的get接口也可以，如果当前vco不支持，则重新初始化phy
    /* 如果是拆分之后创建端口，则只有第一个口创建时可能需要重置phy的vco，后续子口不需要，
       因为拆分创建的子口必然使用相同vco，所以初始化Phy的时候可以使用第一个子口的vco配置初始化所有octal(必须是关联端口被remove的octal) */
    /* 拆分重置vco，直接传首个子口的vco给phy进行初始化 */
    /* 降速重置vco，也是需要批量降速，同样是传首个口的vco给phy */
    /* 目前mac侧一个Pm有8条lane，对应phy的一个octal */
    return 0;
}

static int sap_bcm_port_remove(int unit, int port)
{
    // 将关联的lane设为空闲，可将lane_data_rate设置为bcmpLplaneDataRateNone
    // modulation_mode=bcmplpModulationNONE
    // port_type
    return 0;
}

static void sap_bcm_techsupport(int unit, int port)
{
    int i, rv;
    int phy_id, side;
    char dump_string[MAX_LINE_STRING_LEN];
    sap_bcm_port_info_t *port_info;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t phy_access;

    uint32_t link_status_get;

    for (phy_id=0; phy_id < g_phy_num; phy_id++) {
        if (g_bcm_phy_infos[phy_id].unit != unit) {
            continue;
        }
        if (port != -1) {
            rv = sap_port_info_get(unit, port, &port_info);
            if (rv != SAP_STATUS_SUCCESS) {
                continue;
            }
            if (port_info->phy_info != &g_bcm_phy_infos[phy_id]){
                continue;
            }
        }
        phy_info = &g_bcm_phy_infos[phy_id];
        SAP_BCM_LOG_DBG("unit: %d, PHY-0x%x, chip_name: %s", unit, phy_info->phy_addr, phy_info->chip_name);
        SAP_BCM_LOG_DBG("PHY-0x%x, phy_lanes: %s", phy_info->phy_addr, _dump_int_array(phy_info->lanes,
                BCM_PHY_MAX_PORT_NUM, dump_string, sizeof(dump_string)));
        SAP_BCM_LOG_DBG("PHY-0x%x, ports: %s", phy_info->phy_addr, _dump_int_array(phy_info->ports,
                BCM_PHY_MAX_PORT_NUM, dump_string, sizeof(dump_string)));
        SAP_BCM_LOG_DBG("PHY-0x%x, ports_lane_num: %s", phy_info->phy_addr, _dump_int_array(phy_info->ports_lane_num,
                BCM_PHY_MAX_PORT_NUM, dump_string, sizeof(dump_string)));
        // SAP_BCM_LOG_DBG("PHY-0x%x, speeds: %s", phy_info->phy_addr, _dump_int_array(phy_info->speeds,
        //         BCM_PHY_MAX_PORT_NUM, dump_string, sizeof(dump_string)));
        SAP_BCM_LOG_DBG("PHY-0x%x, sys tx_lane_map_oct1: 0x%x, tx_lane_map_oct2: 0x%x", phy_info->phy_addr, phy_info->tx_lane_map_sys[0], phy_info->tx_lane_map_sys[1]);
        SAP_BCM_LOG_DBG("PHY-0x%x, sys rx_lane_map_oct1: 0x%x, rx_lane_map_oct2: 0x%x", phy_info->phy_addr, phy_info->rx_lane_map_sys[0], phy_info->rx_lane_map_sys[1]);
        SAP_BCM_LOG_DBG("PHY-0x%x, line tx_lane_map_oct1: 0x%x, tx_lane_map_oct2: 0x%x", phy_info->phy_addr, phy_info->tx_lane_map_line[0], phy_info->tx_lane_map_line[1]);
        SAP_BCM_LOG_DBG("PHY-0x%x, line rx_lane_map_oct1: 0x%x, rx_lane_map_oct2: 0x%x", phy_info->phy_addr, phy_info->rx_lane_map_line[0], phy_info->rx_lane_map_line[1]);
        SAP_BCM_LOG_DBG("PHY-0x%x, tx_polarity_flip_sys: 0x%x, rx_polarity_flip_sys: 0x%x", phy_info->phy_addr, phy_info->tx_pol_sys, phy_info->rx_pol_sys);
        SAP_BCM_LOG_DBG("PHY-0x%x, tx_polarity_flip_line: 0x%x, rx_polarity_flip_line: 0x%x", phy_info->phy_addr, phy_info->tx_pol_line, phy_info->rx_pol_line);

        for (i = 0; i<BCM_PHY_MAX_PORT_NUM; i++) {
            rv = sap_port_info_get(unit, g_bcm_phy_infos[phy_id].ports[i], &port_info);
            if (rv != SAP_STATUS_SUCCESS) {
                continue;
            }
            SAP_BCM_LOG_DBG("PHY-0x%x, port: %d, phy_lane0: %d, speed: %d, fec_sys: %d, fec_line: %d, lane_map_sys:0x%x, lane_map_line:0x%x",
                phy_info->phy_addr, port_info->port, port_info->phy_lane0, port_info->speed, port_info->fec_sys, port_info->fec_line, port_info->lanemap_sys, port_info->lanemap_line);
        }

        for (side = BCM_SYSTEM_SIDE; side >= BCM_LINE_SIDE; side--) {
            memset(&phy_access, 0, sizeof(phy_access));
            sap_phy_access_get(unit, port, side, &phy_access);
            rv = bcm_plp_link_status_get(phy_info->chip_name, phy_access, &link_status_get);
            if (rv != BCM_PM_IF_SUCCESS) {
                SAP_BCM_LOG_ERR("FAIL: Failed to get link status for lane-map 0x%x of PHY-0x%x at %s side (ret = %d)!\n",
                phy_access.lane_map, phy_access.phy_addr, (side == BCM_SYSTEM_SIDE) ? "System" : "Line", rv);
            }
            rv = bcm_plp_link_status_get(phy_info->chip_name, phy_access, &link_status_get);
            if (rv != BCM_PM_IF_SUCCESS) {
                SAP_BCM_LOG_ERR("FAIL: Failed to get link status for lane-map 0x%x of PHY-0x%x at %s side (ret = %d)!",
                phy_access.lane_map, phy_access.phy_addr, (side == BCM_SYSTEM_SIDE) ? "System" : "Line", rv);
            }
            SAP_BCM_LOG_DBG("Link status for lane-map 0x%x of PHY-0x%x at %s side is : %d)!",
            phy_access.lane_map, phy_access.phy_addr, (side == BCM_SYSTEM_SIDE) ? "System" : "Line", link_status_get);
        }
    }
}

static bool sap_bcm_port_support(int unit, int port, int physical_port)
{
    /* TODO */
    int rv;
    sap_bcm_port_info_t *port_info;
    rv = sap_port_info_get(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        return false;
    }
    return true;
}

sap_apis_t sap_extphy_barchetta_apis = {
    .type                           = "BCM_81381",
    .sap_platform_init              = sap_bcm_platform_init,
    .sap_port_support               = sap_bcm_port_support,
    .sap_techsupport                = sap_bcm_techsupport,
    .sap_port_autoneg_set           = sap_bcm_autoneg_set,
    .sap_port_autoneg_get           = sap_bcm_autoneg_status_get,
    .sap_port_linktrain_set         = sap_bcm_linktraining_set,
    .sap_port_prbs_set              = sap_bcm_port_prbs_set,
    .sap_port_prbs_get              = sap_bcm_port_prbs_get,
    .sap_port_prbs_clear            = sap_bcm_port_prbs_clear,
    .sap_port_prbs_ber_get          = sap_bcm_port_prbs_ber_get,
    .sap_loopback_set               = sap_bcm_loopback_set,
    .sap_loopback_get               = sap_bcm_loopback_get,
    .sap_phy_status_dump            = sap_bcm_phy_status_dump,
    .sap_dsc_dump                   = sap_bcm_dsc_dump,
    .sap_eyesacn_dump               = sap_bcm_eyescan_dump,
    .sap_squelch_set                = sap_bcm_squelch_set,
    .sap_polarity_set               = sap_bcm_polarity_set,
    .sap_polarity_get               = sap_bcm_polarity_get,
    .sap_phy_reset                  = sap_bcm_phy_reset,
    .sap_reg_set                    = sap_bcm_reg_set,
    .sap_reg_get                    = sap_bcm_reg_get,
    .sap_tx_fir_set                 = sap_bcm_tx_fir_set,
    .sap_tx_fir_get                 = sap_bcm_tx_fir_get,
    .sap_mib_dump                   = sap_bcm_mib_dump,
    // .sap_mib_dump                   = sap_bcm_fec_mib_dump,
    .sap_channel_reach_set          = sap_bcm_channel_reach_set,
    .sap_temperature_get            = sap_bcm_temperature_get,
    .sap_highest_temperature_get    = sap_bcm_highest_temperature_get,
    .sap_fw_version_get             = sap_bcm_version_get,
    .sap_eeprom_update              = sap_bcm_eeprom_update,
    .sap_port_list_get              = sap_bcm_port_list_get,
    .sap_port_status_get            = sap_bcm_port_status_get
};

static int sap_port_info_get(int unit, int port, sap_bcm_port_info_t **port_info)
{
    sap_port_info_t *tmp;
    list_for_each_entry(tmp, &global_port_list_head, node, sap_port_info_t) {
        if (tmp->unit != unit || tmp->port != port) {
            continue;
        }
        *port_info = tmp->port_info;
        return SAP_STATUS_SUCCESS;
    }
    return SAP_STATUS_ITEM_NOT_FOUND;
}

static int read_count = 0;
static int write_count = 0;
static int sap_bcm_mdio_read(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int *data)
{
    int rv;
    if (user_acc == NULL) {
        return -3;
    }
    sleep(0.01); // for phy init
    sap_bcm_phy_info_t *phy_info = (sap_bcm_phy_info_t *)user_acc;
    // reg_addr = 0x40000000 | reg_addr;
    rv = phy_info->mdio_read(NULL, phy_addr, reg_addr, data);
    // SAP_BCM_LOG_DBG("phy: 0x%x, read reg_addr: 0x%x, data: 0x%x, rv: %d", phy_addr, reg_addr, *data, rv);
#ifdef MDIO_DEBUG
    printf("phy: 0x%x, read reg_addr: 0x%x, data: 0x%x, rv: %d\n", phy_addr, reg_addr, *data, rv);
    if(read_count++ % 100 == 0) {
        // printf("read_count: %d\n", read_count);
    }
#endif
    return rv;
}

static int sap_bcm_mdio_write(void *user_acc, unsigned int phy_addr, unsigned int reg_addr, unsigned int data)
{
    int rv;
    if (user_acc == NULL) {
        return -3;
    }
    sleep(0.01); // for phy init
    sap_bcm_phy_info_t *phy_info = (sap_bcm_phy_info_t *)user_acc;
    // reg_addr = 0x40000000 | reg_addr;
    rv = phy_info->mdio_write(NULL, phy_addr, reg_addr, data);
    // SAP_BCM_LOG_DBG("phy: 0x%x, write reg_addr: 0x%x, data: 0x%x, rv: %d", phy_addr, reg_addr, data, rv);
#ifdef MDIO_DEBUG
    printf("phy: 0x%x, write reg_addr: 0x%x, data: 0x%x, rv: %d\n", phy_addr, reg_addr, data, rv);
    if(write_count++ % 100 == 0) {
        // printf("write_count: %d\n", write_count);
    }
#endif
    return rv;
}

static int sap_barchetta_chip_id_get(sap_bcm_phy_info_t *phy_info, uint32_t phy_addr, uint32_t *chip_id)
{
    int rv;
    uint32_t data;

    *chip_id = 0xFFFF;
    rv = phy_info->mdio_read(NULL, phy_addr, BCM_81381_CHIP_ID_ADDR, &data);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("mdio_read fail rv: %d\n", rv);
        return rv;
    } else {
        SAP_BCM_LOG_ERR("mdio_read chip id addr:0x%x date:0x%x\n", BCM_81381_CHIP_ID_ADDR, data);
        *chip_id = data;
    }

    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_config_init(int unit)
{
    int i, j, rv, val;
    int vals[SAP_CFG_FILE_ARR_MAX];
    int ports[BCM_PHY_MAX_PORT_NUM];
    int speeds[BCM_PHY_MAX_PORT_NUM];
    int linktrain_sys[BCM_PHY_MAX_PORT_NUM];
    int linktrain_line[BCM_PHY_MAX_PORT_NUM];
    int ports_lane_num[BCM_PHY_MAX_PORT_NUM];
    int phy_addrs[MAX_PHY_NUM];
    char config_prefix[SAP_CFG_FILE_STR_MAX];
    int index, port, port_id;
    int g_linktrain_sys = 0;
    int g_linktrain_line = 0;
    uint32_t chip_id;
    sap_bcm_port_info_t *port_info;
    sap_bcm_phy_info_t *phy_info;
    if (unit !=0) {
        return SAP_STATUS_SUCCESS;
    }
    /* plp_slot_num */
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, "plp_slot_num", &g_slot_num, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("plp_slot_num get empty.It think no support when cfg get empty.");
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    /* plp_fw_load_method */
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, "plp_fw_load_method", &g_fw_load_method, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("plp_fw_load_method get empty.It think no support when cfg get empty.");
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    /* plp_port_linktrain_sys */
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, "plp_port_linktrain_sys", &g_linktrain_sys, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("plp_port_linktrain_sys get empty.It think no support when cfg get empty.");
    }
    /* plp_port_linktrain_line */
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, "plp_port_linktrain_line", &g_linktrain_line, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("plp_port_linktrain_line get empty.It think no support when cfg get empty.");
    }
    /* plp_i2c_slave_en */
    sap_get_cfg_info(SAP_BCM_CFG_FILE, "plp_i2c_slave_en", &g_i2c_slave_en, false);
    /* avs_i2c_resp_addr */
    sap_get_cfg_info(SAP_BCM_CFG_FILE, "avs_i2c_resp_addr", &g_avs_i2c_resp_addr, false);
    /* phy_addrs */
    memset(phy_addrs, -1, sizeof(phy_addrs));
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, "plp_phy_addrs", phy_addrs, true);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("plp_phy_addrs get empty.");
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    g_phy_num = 0;
    for (i=0; i<MAX_PHY_NUM; i++) {
        if (phy_addrs[i] == -1) {
            break;
        }
        g_phy_num++;
    }
    g_bcm_phy_infos = (sap_bcm_phy_info_t *)malloc(sizeof(sap_bcm_phy_info_t) * g_phy_num);
    for (i=0; i<g_phy_num; i++) {
        phy_info = &g_bcm_phy_infos[i];
        phy_info->loaded = 0;
        /* phy_addr */
        phy_info->phy_addr = (unsigned int)phy_addrs[i];
        /* chip_name */
        phy_info->chip_name = CHIP_NAME;
        phy_info->tx_pol_line = 0;
        phy_info->tx_pol_sys = 0;
        phy_info->rx_pol_line = 0;
        phy_info->rx_pol_sys = 0;
        rv = pthread_mutex_init(&phy_info->mutex, NULL); //PTHREAD_MUTEX_RECURSIVE
        if (rv) {
            SAP_BCM_LOG_ERR("PHY-0x%x pthread_mutex_init failed with rv: %d", phy_info->phy_addr, rv);
        }
        /* card_id */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_card_id:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }
        phy_info->card_id = val;
        /* mdio_id */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_mdio_id:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }
        phy_info->mdio_id = val;
        /* mdio_type */
        snprintf(config_prefix, sizeof(config_prefix), "plp_mdio_accs_type:%d", val);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }
        phy_info->mdio_type = val;
        /* mdio_func */
        rv = sap_get_mdio_write_func(phy_info->mdio_type, &phy_info->mdio_write);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_ERR("PHY-0x%x mdio write func not reg!", phy_info->phy_addr);
            return SAP_STATUS_FAILURE;
        }
        rv = sap_get_mdio_read_func(phy_info->mdio_type, &phy_info->mdio_read);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_ERR("PHY-0x%x mdio read func not reg!", phy_info->phy_addr);
            return SAP_STATUS_FAILURE;
        }
        /* check chip id */
        chip_id = 0xFFFF;
        rv = sap_barchetta_chip_id_get(phy_info, phy_info->phy_addr, &chip_id);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_ERR("PHY-0x%x sap_barchetta_chip_id_get fail", phy_info->phy_addr);
            return SAP_STATUS_FAILURE;
        } else {
            if (chip_id == 0xFFFF) {
                SAP_BCM_LOG_ERR("PHY-0x%x sap_barchetta_chip_id_get chip_id is invalid", phy_info->phy_addr);
                return SAP_STATUS_FAILURE;
            }
            if (chip_id != BCM_81381_CHIP_ID) {
                SAP_BCM_LOG_ERR("PHY-0x%x sap_barchetta_chip_id_get chip_id isn't 81381", phy_info->phy_addr);
                free(g_bcm_phy_infos);
                return SAP_STATUS_NOT_SUPPORTED;
            }
        }
        /* unit */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_unit_id:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }else{
            phy_info->unit = val;
        }
        /*macsec_option*/
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_macsec_option:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }else{
            phy_info->macsec_option = val;
        }
        /*ptp_option*/
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_ptp_option:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }else{
            phy_info->ptp_option = val;
        }
        /* profile_id */
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_init_profile_id:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }else{
            phy_info->profile_id = val;
        }
        /*tx_polarity_flip_line*/
        snprintf(config_prefix, sizeof(config_prefix), "plp_tx_polarity_flip_line:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        }else{
            phy_info->tx_pol_line = val;
        }
        /*rx_polarity_flip_line*/
        snprintf(config_prefix, sizeof(config_prefix), "plp_rx_polarity_flip_line:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        }else{
            phy_info->rx_pol_line = val;
        }
        /*tx_polarity_flip_sys*/
        snprintf(config_prefix, sizeof(config_prefix), "plp_tx_polarity_flip_sys:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        }else{
            phy_info->tx_pol_sys = val;
        }
        /*rx_polarity_flip_sys*/
        snprintf(config_prefix, sizeof(config_prefix), "plp_rx_polarity_flip_sys:0x%x", phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        }else{
            phy_info->rx_pol_sys = val;
        }
        /* tx_lane_map_line*/
        memset(&phy_info->tx_lane_map_line, -1, sizeof(phy_info->tx_lane_map_line));
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_tx_lane_map_line:0x%x", phy_addrs[i]);
        sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, phy_info->tx_lane_map_line, true);
        /* rx_lane_map_line*/
        memset(&phy_info->rx_lane_map_line, -1, sizeof(phy_info->rx_lane_map_line));
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_rx_lane_map_line:0x%x", phy_addrs[i]);
        sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, phy_info->rx_lane_map_line, true);
        /* tx_lane_map_sys*/
        memset(&phy_info->tx_lane_map_sys, -1, sizeof(phy_info->tx_lane_map_sys));
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_tx_lane_map_sys:0x%x", phy_addrs[i]);
        sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, phy_info->tx_lane_map_sys, true);
        /* rx_lane_map_sys*/
        memset(&phy_info->rx_lane_map_sys, -1, sizeof(phy_info->rx_lane_map_sys));
        snprintf(config_prefix, sizeof(config_prefix), "plp_phy_rx_lane_map_sys:0x%x", phy_addrs[i]);
        sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, phy_info->rx_lane_map_sys, true);
        /* vco */
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_vco_octal_0_sys", phy_info->profile_id);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }else{
            phy_info->vco_octal_0_sys = val;
        }
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_vco_octal_0_line", phy_info->profile_id);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }else{
            phy_info->vco_octal_0_line = val;
        }
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_vco_octal_1_sys", phy_info->profile_id);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }else{
            phy_info->vco_octal_1_sys = val;
        }
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_vco_octal_1_line", phy_info->profile_id);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }else{
            phy_info->vco_octal_1_line = val;
        }
        /* lanes */
        memset(vals, -1, sizeof(vals));
        memset(phy_info->lanes, -1, sizeof(phy_info->lanes));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_lanes:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, vals, true);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }
        phy_info->lane_num = 0;
        for (j = 0; j < BCM_PHY_MAX_PORT_NUM; j++) {
            if (vals[j] == -1) {
                break;
            }
            phy_info->lanes[j] = vals[j];
            /* lane_num */
            phy_info->lane_num++;
        }
        /* ports */
        memset(ports, -1, sizeof(ports));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_ports:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, ports, true);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }
        /* ports_lane_num */
        memset(ports_lane_num, -1, sizeof(ports_lane_num));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_ports_lane_num:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, ports_lane_num, true);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }
        /* speeds */
        memset(speeds, -1, sizeof(speeds));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_speed_mode:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, speeds, true);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
            return SAP_STATUS_ITEM_NOT_FOUND;
        }
        /* linktrain_sys */
        memset(linktrain_sys, -1, sizeof(linktrain_sys));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_linktrain_sys:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, linktrain_sys, true);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        }
        /* linktrain_line */
        memset(linktrain_line, -1, sizeof(linktrain_line));
        snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_init_linktrain_line:0x%x", phy_info->profile_id, phy_addrs[i]);
        rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, linktrain_line, true);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        }
        // memset(phy_info->speeds, -1, sizeof(phy_info->speeds));
        memset(phy_info->ports, -1, sizeof(phy_info->ports));
        memset(phy_info->ports_lane_num, -1, sizeof(phy_info->ports_lane_num));
        index = 0;
        for (j = 0; j<BCM_PHY_MAX_PORT_NUM; j++) {
            if (ports[j] == -1) {
                break;
            }
            index += (j>0) ? ports_lane_num[j-1] : 0;
            phy_info->ports[index] = ports[j];
            SAP_BCM_LOG_DBG("ports[%d] = %d", index, ports[j]);
            phy_info->ports_lane_num[index] = ports_lane_num[j];
            SAP_BCM_LOG_DBG("ports_lane_num[%d] = %d", index, ports_lane_num[j]);
        }
        port_id = 0;
        for (index = 0; index< BCM_PHY_MAX_PORT_NUM; index ++) {
            port = phy_info->ports[index];
            if (port == -1) {
                continue;
            }
            port_info = (sap_bcm_port_info_t *)malloc(sizeof(sap_bcm_port_info_t));
            if (port_info == NULL) {
                SAP_BCM_LOG_ERR("malloc failed!");
                continue;
            }
            sap_port_info_t *sap_port_info = (sap_port_info_t *)malloc(sizeof(sap_port_info_t));
            if (sap_port_info == NULL) {
                SAP_BCM_LOG_ERR("malloc failed!");
                free(port_info);
                continue;
            }
            port_info->inited         = 0; 
            port_info->phy_info       = phy_info; 
            port_info->port           = port;
            port_info->unit           = phy_info->unit;
            port_info->phy_lane0      = phy_info->lanes[index];
            port_info->speed          = speeds[port_id];
            port_info->lanes          = phy_info->ports_lane_num[index];
            port_info->linktrain_sys  = (linktrain_sys[port_id] != -1) ? linktrain_sys[port_id] : g_linktrain_sys;
            port_info->linktrain_line = (linktrain_line[port_id] != -1) ? linktrain_line[port_id] : g_linktrain_line;
            sap_bcm_get_profile_info(phy_info->profile_id, port_info->speed, phy_info->lane_num/port_info->lanes, index, port_info);
            sap_port_info->unit = port_info->unit;
            sap_port_info->port = port_info->port;
            // sap_port_info->phy_lane0 = phy_info->lanes[port_info->phy_lane0];
            sap_port_info->phy_lane0 = port_info->phy_lane0;
            sap_port_info->port_info = port_info;
            global_port_num++;
            list_add_tail(&sap_port_info->node, &global_port_list_head);
            port_id++;
        }
    }
    return SAP_STATUS_SUCCESS;
}

#ifdef MUTIL_THREAD
static int sap_slot_init(int unit, int slot_id)
{
    pthread_t tid;
    int i, rv, count;
    sap_bcm_phy_info_t *phy_info;

    if (g_fw_load_method == BCM_FW_BROADCAST_LOAD) {
        count = 0;
        sap_bcm_phy_info_t *phy_infos[g_phy_num];
        for (i=0; i<g_phy_num; i++) {
            phy_info = &g_bcm_phy_infos[i];
            if (phy_info->card_id != slot_id) {
                continue;
            }
            if (phy_info->unit != unit) {
                continue;
            }
            phy_infos[count++] = phy_info;
        }
        sap_barchetta_fw_load_broadcast(phy_infos, count);
        return SAP_STATUS_SUCCESS;
    }

    for (i=0; i<g_phy_num; i++) {
        phy_info = &g_bcm_phy_infos[i];
        if (phy_info->card_id != slot_id) {
            continue;
        }
        if (phy_info->unit != unit) {
            continue;
        }

        rv = pthread_create(&tid, NULL, sap_barchetta_phy_init_thread, (void *)phy_info);
        if (rv) {
            SAP_BCM_LOG_ERR("pthread_create error: %s\n", strerror(rv));
        }
    }
    return SAP_STATUS_SUCCESS;
}
#else
static int sap_slot_init(int unit, int slot_id)
{
    pthread_t tid;
    int i, rv, count;
    sap_bcm_phy_info_t *phy_info;
    sap_port_info_t *sap_port_info;
    sap_bcm_port_info_t *port_info;
    if (g_fw_load_method == BCM_FW_BROADCAST_LOAD) {
        count = 0;
        sap_bcm_phy_info_t *phy_infos[g_phy_num];
        for (i=0; i<g_phy_num; i++) {
            phy_info = &g_bcm_phy_infos[i];
            if (phy_info->card_id != slot_id) {
                continue;
            }
            if (phy_info->unit != unit) {
                continue;
            }
            phy_infos[count++] = phy_info;
        }
        sap_barchetta_fw_load_broadcast(phy_infos, count);
        return SAP_STATUS_SUCCESS;
    }
    for (i=0; i<g_phy_num; i++) {
        phy_info = &g_bcm_phy_infos[i];
        if (phy_info->card_id != slot_id) {
            continue;
        }
        if (phy_info->unit != unit) {
            continue;
        }
        rv = sap_barchetta_fw_load(phy_info);
        if (rv != SAP_STATUS_SUCCESS) {
            rv = sap_barchetta_clean_up(phy_info);
            if (rv != SAP_STATUS_SUCCESS) {
                SAP_LOG_WARN("PHY-0x%x sap_barchetta_clean_up failed with rv: %d", phy_info->phy_addr, rv);
            }
            rv = sap_barchetta_fw_load(phy_info);
            if (rv != SAP_STATUS_SUCCESS) {
                SAP_BCM_LOG_ERR("PHY-0x%x fw load failed again with rv: %d", phy_info->phy_addr, rv);
                continue;
            } else {
                SAP_LOG_NOTICE("PHY-0x%x init success!", phy_info->phy_addr);
            }
        }
        /* phy init */
        rv = sap_barchetta_phy_init(phy_info);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_ERR("PHY-0x%x init failed with rv: %d", phy_info->phy_addr, rv);
            continue;
        } else {
            phy_info->loaded = 1;
        }
        /* pre port init */
        list_for_each_entry(sap_port_info, &global_port_list_head, node, sap_port_info_t) {
            port_info = sap_port_info->port_info;
            if (port_info->phy_info != phy_info) {
                continue;
            }
            rv = sap_barchetta_pre_port_init(port_info);
            if (rv != SAP_STATUS_SUCCESS) {
                SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_barchetta_pre_port_init failed with rv: %d", port_info->unit, port_info->port, rv);
                continue;
            } else {
                SAP_BCM_LOG_INFO("unit[%d] port[%d], sap_barchetta_pre_port_init succeed", port_info->unit, port_info->port);
            }
        }
        /* port init */
        list_for_each_entry(sap_port_info, &global_port_list_head, node, sap_port_info_t) {
            port_info = sap_port_info->port_info;
            if (port_info->phy_info != phy_info) {
                continue;
            }
            rv = sap_barchetta_port_init(port_info);
            if (rv != SAP_STATUS_SUCCESS) {
                SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_barchetta_port_init failed with rv: %d", port_info->unit, port_info->port, rv);
                continue;
            } else {
                SAP_BCM_LOG_INFO("unit[%d] port[%d], sap_barchetta_port_init succeed", port_info->unit, port_info->port);
                port_info->inited = 1;
            }
        }
        /* post port init */
        list_for_each_entry(sap_port_info, &global_port_list_head, node, sap_port_info_t) {
            port_info = sap_port_info->port_info;
            if (port_info->phy_info != phy_info) {
                continue;
            }
            rv = sap_barchetta_post_port_init(port_info);
            if (rv != SAP_STATUS_SUCCESS) {
                SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_barchetta_post_port_init failed with rv: %d", port_info->unit, port_info->port, rv);
                continue;
            } else {
                SAP_BCM_LOG_INFO("unit[%d] port[%d], sap_barchetta_post_port_init succeed", port_info->unit, port_info->port);
            }
        }
    }
    return SAP_STATUS_SUCCESS;
}
#endif

static void *sap_barchetta_phy_init_thread(void *arg)
{
    int rv, retry_cnt;
    sap_bcm_phy_info_t *phy_info;
    sap_bcm_port_info_t *port_info;
    sap_port_info_t *sap_port_info;
    rv = pthread_detach(pthread_self());
    if (rv) {
        SAP_BCM_LOG_ERR("pthread_detach error: %s\n", strerror(rv));
        pthread_exit(NULL);
    }
    if (arg == NULL) {
        pthread_exit(NULL);
    }
    phy_info = (sap_bcm_phy_info_t *)arg;
    retry_cnt = 0;
    rv = sap_barchetta_fw_load(phy_info);
    while(rv != SAP_STATUS_SUCCESS && retry_cnt < 100) {
        retry_cnt++;
        SAP_BCM_LOG_ERR("PHY-0x%x fw load failed with rv: %d, retry_cnt: %d", phy_info->phy_addr, rv, retry_cnt);
        sleep(5);
        rv = sap_barchetta_clean_up(phy_info);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_LOG_WARN("PHY-0x%x sap_barchetta_clean_up failed with rv: %d", phy_info->phy_addr, rv);
        }
        rv = sap_barchetta_fw_load(phy_info);
    }
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x fw load failed with rv: %d", phy_info->phy_addr, rv);
    }
    /* phy init */
    rv = sap_barchetta_phy_init(phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x init failed with rv: %d", phy_info->phy_addr, rv);
        pthread_exit(NULL);
    } else {
        phy_info->loaded = 1;
    }
    /* pre port init */
    list_for_each_entry(sap_port_info, &global_port_list_head, node, sap_port_info_t) {
        port_info = sap_port_info->port_info;
        if (port_info->phy_info != phy_info) {
            continue;
        }
        rv = sap_barchetta_pre_port_init(port_info);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_barchetta_pre_port_init failed with rv: %d", port_info->unit, port_info->port, rv);
            pthread_exit(NULL);
        } else {
            SAP_BCM_LOG_INFO("unit[%d] port[%d], sap_barchetta_pre_port_init succeed", port_info->unit, port_info->port);
        }
    }
    /* port init */
    list_for_each_entry(sap_port_info, &global_port_list_head, node, sap_port_info_t) {
        port_info = sap_port_info->port_info;
        if (port_info->phy_info != phy_info) {
            continue;
        }
        rv = sap_barchetta_port_init(port_info);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_barchetta_port_init failed with rv: %d", port_info->unit, port_info->port, rv);
            pthread_exit(NULL);
        } else {
            SAP_BCM_LOG_INFO("unit[%d] port[%d], sap_barchetta_port_init succeed", port_info->unit, port_info->port);
            port_info->inited = 1;
        }
    }
    /* post port init */
    list_for_each_entry(sap_port_info, &global_port_list_head, node, sap_port_info_t) {
        port_info = sap_port_info->port_info;
        if (port_info->phy_info != phy_info) {
            continue;
        }
        rv = sap_barchetta_post_port_init(port_info);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_barchetta_post_port_init failed with rv: %d", port_info->unit, port_info->port, rv);
            pthread_exit(NULL);
        } else {
            SAP_BCM_LOG_INFO("unit[%d] port[%d], sap_barchetta_post_port_init succeed", port_info->unit, port_info->port);
        }
    }

    pthread_exit(NULL);
}

static int sap_barchetta_fw_load(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    int lane_index;
    uint32_t read_tx_pol, read_rx_pol;
    uint32_t fw_ver, fw_crc;
    bcm_plp_access_t plp_access;
    sap_bcm_port_info_t *port_info;
    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;

    bcm_plp_mutex_info_t mutex_info = {
        .mutex_take = sap_bcm_mutex_take,
        .mutex_give = sap_bcm_mutex_give
    };
    rv = bcm_plp_mutex_info_set(phy_info->chip_name, plp_access, &mutex_info);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_mutex_info_set failed with rv: %d", phy_info->phy_addr, rv);
        return rv;
    }
    if (g_fw_load_method == BCM_FW_UNICAST_LOAD) {
        // rv = sap_barchetta_clean_up(phy_info);
        // if (rv != SAP_STATUS_SUCCESS) {
        //     SAP_LOG_WARN("PHY-0x%x sap_barchetta_clean_up failed with rv: %d", phy_info->phy_addr, rv);
        // }
        // rv = sap_barchetta_fw_repair(phy_info);
        // if (rv != SAP_STATUS_SUCCESS) {
        //     SAP_BCM_LOG_ERR("PHY-0x%x fw repair failed with rv: %d", phy_info->phy_addr, rv);
        //     return rv;
        // }
        // rv = sap_barchetta_fw_soft_reset(phy_info);
        rv = sap_barchetta_fw_load_unicast(phy_info);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("PHY-0x%x sap_barchetta_fw_load_unicast failed with rv: %d", phy_info->phy_addr, rv);
            return rv;
        }
    } else if (g_fw_load_method == BCM_FW_EEPROM_LOAD) {
        rv = sap_barchetta_fw_load_eeprom(phy_info);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("PHY-0x%x sap_barchetta_fw_load_eeprom failed with rv: %d", phy_info->phy_addr, rv);
            return rv;
        }
    } else if (g_fw_load_method == BCM_FW_NONE_LOAD) {
        rv = sap_barchetta_fw_load_none(phy_info);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("PHY-0x%x sap_barchetta_fw_load_none failed with rv: %d", phy_info->phy_addr, rv);
            return rv;
        }
    } else if (g_fw_load_method == BCM_FW_UPGRADE) {
        rv = sap_barchetta_fw_set_eeprom(phy_info);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("PHY-0x%x sap_barchetta_fw_set_eeprom failed with rv: %d", phy_info->phy_addr, rv);
            return rv;
        }
    } else if (g_fw_load_method == BCM_FW_LOAD_SKIP) {
        rv = sap_barchetta_fw_load_eeprom_skip(phy_info);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("PHY-0x%x sap_barchetta_fw_load_eeprom_skip failed with rv: %d", phy_info->phy_addr, rv);
            return rv;
        }
    } else if (g_fw_load_method == BCM_FW_LOAD_FP) {
        rv = sap_barchetta_fw_load_full_process(phy_info);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("PHY-0x%x sap_barchetta_fw_load_eeprom_skip failed with rv: %d", phy_info->phy_addr, rv);
            return rv;
        }
    }

// #ifdef FW_REPAIRE_ENABLE
//     rv = sap_barchetta_clean_up(phy_info);
//     if (rv != SAP_STATUS_SUCCESS) {
//         SAP_LOG_WARN("PHY-0x%x sap_barchetta_clean_up failed with rv: %d", phy_info->phy_addr, rv);
//     }
//     SAP_BCM_LOG_ERR("PHY-0x%x fw load failed, try to repair it!", phy_info->phy_addr);
//     rv = sap_barchetta_fw_repair(phy_info);
//     if (rv != SAP_STATUS_SUCCESS) {
//         SAP_BCM_LOG_ERR("PHY-0x%x fw repair failed with rv: %d", phy_info->phy_addr, rv);
//     }
//     rv = sap_barchetta_clean_up(phy_info);
//     if (rv != SAP_STATUS_SUCCESS) {
//         SAP_LOG_WARN("PHY-0x%x sap_barchetta_clean_up failed with rv: %d", phy_info->phy_addr, rv);
//     }
//     SAP_LOG_NOTICE("PHY-0x%x fw load again!", phy_info->phy_addr);
//     rv = sap_barchetta_fw_load(phy_info);
//     if (rv != SAP_STATUS_SUCCESS) {
//         SAP_BCM_LOG_ERR("PHY-0x%x fw load failed again with rv: %d", phy_info->phy_addr, rv);
//         continue;
//     } else {
//         SAP_LOG_NOTICE("PHY-0x%x init success!", phy_info->phy_addr);
//     }
// #endif

    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_phy_init(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    int lane_index;
    uint32_t read_tx_pol, read_rx_pol;
    uint32_t fw_ver, fw_crc;
    bcm_plp_access_t plp_access;
    bcm_plp_logical_lane_map_t sys_lane_map, line_lane_map;
    sap_bcm_port_info_t *port_info;
    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;

    rv = bcm_plp_firmware_info_get(phy_info->chip_name, plp_access, &fw_ver, &fw_crc);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_firmware_info_get failed rv: %d", phy_info->phy_addr, rv);
        return rv;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x FW version 0x%x, FW CRC 0x%x", phy_info->phy_addr, fw_ver, fw_crc);
    }

    /* 线序初始化 */
    memset(&sys_lane_map, 0, sizeof(bcm_plp_logical_lane_map_t));
    memset(&line_lane_map, 0, sizeof(bcm_plp_logical_lane_map_t));
    sys_lane_map.num_of_lanes = BCM_PHY_MAX_LANE;
    line_lane_map.num_of_lanes = BCM_PHY_MAX_LANE;
    for (lane_index = 0; lane_index < BCM_PHY_MAX_LANE; lane_index++) {
        sys_lane_map.rx_lane_list[lane_index] = GET_LANE_SWAP_OCTAL(phy_info->rx_lane_map_sys[0], lane_index);
        sys_lane_map.tx_lane_list[lane_index] = GET_LANE_SWAP_OCTAL(phy_info->tx_lane_map_sys[0], lane_index);
        line_lane_map.rx_lane_list[lane_index] = GET_LANE_SWAP_OCTAL(phy_info->rx_lane_map_line[0], lane_index);
        line_lane_map.tx_lane_list[lane_index] = GET_LANE_SWAP_OCTAL(phy_info->tx_lane_map_line[0], lane_index);
    }
    plp_access.if_side  = BCM_SYSTEM_SIDE;
    rv = bcm_plp_logical_lane_set(phy_info->chip_name, plp_access, sys_lane_map);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_logical_lane_set failed at system side with rv: %d", phy_info->phy_addr, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("PASS: Successfully set rx logical lane (sys : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at System side!",
                sys_lane_map.rx_lane_list[0], sys_lane_map.rx_lane_list[1], sys_lane_map.rx_lane_list[2], sys_lane_map.rx_lane_list[3],
                sys_lane_map.rx_lane_list[4], sys_lane_map.rx_lane_list[5], sys_lane_map.rx_lane_list[6], sys_lane_map.rx_lane_list[7],
                plp_access.phy_addr);
    SAP_BCM_LOG_DBG("PASS: Successfully set tx logical lane (sys : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at System side!",
                sys_lane_map.tx_lane_list[0], sys_lane_map.tx_lane_list[1], sys_lane_map.tx_lane_list[2], sys_lane_map.tx_lane_list[3],
                sys_lane_map.tx_lane_list[4], sys_lane_map.tx_lane_list[5], sys_lane_map.tx_lane_list[6], sys_lane_map.tx_lane_list[7],
                plp_access.phy_addr);

    plp_access.if_side  = BCM_LINE_SIDE;
    rv = bcm_plp_logical_lane_set(phy_info->chip_name, plp_access, line_lane_map);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_logical_lane_set failed for lane-map: 0x%x at system side with rv: %d", phy_info->phy_addr, plp_access.lane_map, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("PASS: Successfully set rx logical lane (line : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at System side!",
                line_lane_map.rx_lane_list[0], line_lane_map.rx_lane_list[1], line_lane_map.rx_lane_list[2], line_lane_map.rx_lane_list[3],
                line_lane_map.rx_lane_list[4], line_lane_map.rx_lane_list[5], line_lane_map.rx_lane_list[6], line_lane_map.rx_lane_list[7],
                plp_access.phy_addr);
    SAP_BCM_LOG_DBG("PASS: Successfully set tx logical lane (line : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at System side!",
                line_lane_map.tx_lane_list[0], line_lane_map.tx_lane_list[1], line_lane_map.tx_lane_list[2], line_lane_map.tx_lane_list[3],
                line_lane_map.tx_lane_list[4], line_lane_map.tx_lane_list[5], line_lane_map.tx_lane_list[6], line_lane_map.tx_lane_list[7],
                plp_access.phy_addr);

    memset(&sys_lane_map, 0, sizeof(bcm_plp_logical_lane_map_t));
    plp_access.if_side = BCM_SYSTEM_SIDE;
    rv = bcm_plp_logical_lane_get(phy_info->chip_name, plp_access, &sys_lane_map);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_logical_lane_get failed for lane-map: 0x%x at system side with rv: %d", phy_info->phy_addr, plp_access.lane_map, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("PASS: Successfully get rx logical lane (sys : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at System side!",
                sys_lane_map.rx_lane_list[0], sys_lane_map.rx_lane_list[1], sys_lane_map.rx_lane_list[2], sys_lane_map.rx_lane_list[3],
                sys_lane_map.rx_lane_list[4], sys_lane_map.rx_lane_list[5], sys_lane_map.rx_lane_list[6], sys_lane_map.rx_lane_list[7],
                plp_access.phy_addr);
    SAP_BCM_LOG_DBG("PASS: Successfully get tx logical lane (sys : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at System side!",
                sys_lane_map.tx_lane_list[0], sys_lane_map.tx_lane_list[1], sys_lane_map.tx_lane_list[2], sys_lane_map.tx_lane_list[3],
                sys_lane_map.tx_lane_list[4], sys_lane_map.tx_lane_list[5], sys_lane_map.tx_lane_list[6], sys_lane_map.tx_lane_list[7],
                plp_access.phy_addr);

    memset(&line_lane_map, 0, sizeof(bcm_plp_logical_lane_map_t));
    plp_access.if_side = BCM_SYSTEM_SIDE;
    rv = bcm_plp_logical_lane_get(phy_info->chip_name, plp_access, &line_lane_map);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_logical_lane_get failed for lane-map: 0x%x at system side with rv: %d", phy_info->phy_addr, plp_access.lane_map, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("PASS: Successfully get rx logical lane (line : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at System side!",
                line_lane_map.rx_lane_list[0], line_lane_map.rx_lane_list[1], line_lane_map.rx_lane_list[2], line_lane_map.rx_lane_list[3],
                line_lane_map.rx_lane_list[4], line_lane_map.rx_lane_list[5], line_lane_map.rx_lane_list[6], line_lane_map.rx_lane_list[7],
                plp_access.phy_addr);
    SAP_BCM_LOG_DBG("PASS: Successfully get tx logical lane (line : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) for PHY-0x%x at System side!",
                line_lane_map.tx_lane_list[0], line_lane_map.tx_lane_list[1], line_lane_map.tx_lane_list[2], line_lane_map.tx_lane_list[3],
                line_lane_map.tx_lane_list[4], line_lane_map.tx_lane_list[5], line_lane_map.tx_lane_list[6], line_lane_map.tx_lane_list[7],
                plp_access.phy_addr);

    /* 极性初始化 */
    plp_access.lane_map = ALL_LANE_MAP;
    plp_access.if_side  = BCM_SYSTEM_SIDE;
    rv = bcm_plp_polarity_get(phy_info->chip_name, plp_access, &read_tx_pol, &read_rx_pol);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_polarity_get failed at system side with rv: %d", phy_info->phy_addr, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("PASS: Successfully get the polarity (tx_pol : 0x%x, rx_pol : 0x%x) for PHY-0x%x at System side!",
                read_tx_pol, read_rx_pol, plp_access.phy_addr);
    read_tx_pol = read_tx_pol ^ phy_info->tx_pol_sys;
    read_rx_pol = read_rx_pol ^ phy_info->rx_pol_sys;

    rv = bcm_plp_polarity_set(phy_info->chip_name, plp_access, read_tx_pol, read_rx_pol);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_polarity_set failed at system side with rv: %d", phy_info->phy_addr, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("PASS: Successfully set the polarity (tx_pol : 0x%x, rx_pol : 0x%x) for PHY-0x%x at System side!",
                read_tx_pol, read_rx_pol, plp_access.phy_addr);

    plp_access.lane_map = ALL_LANE_MAP;
    plp_access.if_side  = BCM_LINE_SIDE;
    rv = bcm_plp_polarity_get(phy_info->chip_name, plp_access, &read_tx_pol, &read_rx_pol);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_polarity_get failed at line side with rv: %d", phy_info->phy_addr, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("PASS: Successfully get the polarity (tx_pol : 0x%x, rx_pol : 0x%x) for PHY-0x%x at Line side!",
                read_tx_pol, read_rx_pol, plp_access.phy_addr);

    read_tx_pol = read_tx_pol ^ phy_info->tx_pol_line;
    read_rx_pol = read_rx_pol ^ phy_info->rx_pol_line;
    rv = bcm_plp_polarity_set(phy_info->chip_name, plp_access, read_tx_pol, read_rx_pol);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_polarity_set failed at line side with rv: %d", phy_info->phy_addr, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("PASS: Successfully set the polarity (tx_pol : 0x%x, rx_pol : 0x%x) for PHY-0x%x at Line side!",
                read_tx_pol, read_rx_pol, plp_access.phy_addr);
    // if (g_i2c_slave_en == 1) {
    //     rv = bcm_plp_reg64_value_set(phy_info->chip_name, plp_access, 1, 0x52002010, 0x0000006b);
    //     if (rv != 0) {
    //         SAP_BCM_LOG_ERR("PHY-0x%x, bcm_plp_reg64_value_set failed with rv:%d, i2c slave enable failed.", phy_info->phy_addr, rv);
    //     }
    //     rv = bcm_plp_reg64_value_set(phy_info->chip_name, plp_access, 1, 0x52002014, 0x00000fff);
    //     if (rv != 0) {
    //         SAP_BCM_LOG_ERR("PHY-0x%x, bcm_plp_reg64_value_set failed with rv:%d, i2c slave enable failed.", phy_info->phy_addr, rv);
    //     }
    // } else {
    //     rv = sap_bcm_avs_init(phy_info);
    //     if (rv != SAP_STATUS_SUCCESS) {
    //         SAP_BCM_LOG_ERR("PHY-0x%x, sap_bcm_avs_init failed with rv:%d", phy_info->phy_addr, rv);
    //     }
    // }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_avs_init(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    uint16_t avs_cfg1;
    uint32_t cfg1, cntrl, sts, req;
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    plp_access.if_side = 0;
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    plp_access.lane_map = ALL_LANE_MAP;
    bcm_plp_reg_value_get(phy_info->chip_name, plp_access, 1, AVS_I2C_address0_Adr, &cfg1);
    bcm_plp_reg_value_get(phy_info->chip_name, plp_access, 1, AVS_FW_control_Adr, &cntrl);
    bcm_plp_reg_value_get(phy_info->chip_name, plp_access, 1, AVS_FW_status_Adr, &sts);
    bcm_plp_reg_value_get(phy_info->chip_name, plp_access, 1, AVS_Vout_mV_status_Adr, &req);
    SAP_BCM_LOG_INFO("PHY-0x%x, AVS_CFG1: %04X | AVS_CNTRL: %04X || AVS_STS: %04X | AVS_VOUT_REQ: %04X",
            phy_info->phy_addr, cfg1, cntrl, sts, req);
    avs_cfg1 = (g_avs_i2c_resp_addr) ? ((0x80 | g_avs_i2c_resp_addr) << 8) : 0;
    bcm_plp_reg_value_set(phy_info->chip_name, plp_access, 1, AVS_I2C_address0_Adr, avs_cfg1);
    SAP_BCM_LOG_INFO("PHY-0x%x, set AVS_I2C_address0_Adr: %04X", phy_info->phy_addr, avs_cfg1);
    /* enable AVS FW */
    bcm_plp_reg_value_set(phy_info->chip_name, plp_access, 1, AVS_FW_control_Adr, 0x5);
    bcm_plp_reg_value_get(phy_info->chip_name, plp_access, 1, AVS_I2C_address0_Adr, &cfg1);
    bcm_plp_reg_value_get(phy_info->chip_name, plp_access, 1, AVS_FW_control_Adr, &cntrl);
    bcm_plp_reg_value_get(phy_info->chip_name, plp_access, 1, AVS_FW_status_Adr, &sts);
    bcm_plp_reg_value_get(phy_info->chip_name, plp_access, 1, AVS_Vout_mV_status_Adr, &req);
    SAP_BCM_LOG_INFO("PHY-0x%x, AVS_CFG1: %04X | AVS_CNTRL: %04X || AVS_STS: %04X | AVS_VOUT_REQ: %04X",
            phy_info->phy_addr, cfg1, cntrl, sts, req);
    
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_clean_up(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    bcm_plp_mac_access_t mac_access;
    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&mac_access, 0, sizeof(bcm_plp_mac_access_t));
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    memcpy(&mac_access.phy_info, &plp_access, sizeof(bcm_plp_access_t));
    rv = bcm_plp_mac_cleanup(phy_info->chip_name, mac_access);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_mac_cleanup failed with rv: 0x%x", plp_access.phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_LOG_NOTICE("PHY-0x%x cleanup-mac succeed", plp_access.phy_addr);
    }
    rv = bcm_plp_cleanup(phy_info->chip_name, plp_access);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_cleanup failed with rv: 0x%x", plp_access.phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_LOG_NOTICE("PHY-0x%x cleanup succeed", plp_access.phy_addr);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_fw_repair(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    bcm_plp_firmware_load_type_t firmware_load_type;
    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodProgEEPROM;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL;
    // rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
    //         &firmware_load_type, bcmpmFirmwareBroadcastCoreReset);
    // if (rv != BCM_PM_IF_SUCCESS) {
    //     SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastCoreReset API err %d", phy_info->phy_addr, rv);
    // } else {
    //     SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastCoreReset API success", phy_info->phy_addr);
    // }
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastFirmwareExecute);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastFirmwareExecute API err %d", phy_info->phy_addr, rv);
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastFirmwareExecute API success", phy_info->phy_addr);
    }
    sleep(3);
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_fw_soft_reset(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    bcm_plp_firmware_load_type_t firmware_load_type;

    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));

    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodProgEEPROM;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL;
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastCoreReset);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastCoreReset API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastCoreReset API success", phy_info->phy_addr);
    }
    return SAP_STATUS_SUCCESS;
}


static int sap_barchetta_fw_hard_reset(sap_bcm_phy_info_t *phy_info)
{
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_fw_set_eeprom(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    bcm_plp_firmware_load_type_t firmware_load_type;

    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    // plp_access.flags = BCM_PLP_WARM_BOOT;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodProgEEPROM;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL;
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastFirmwareExecute);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastFirmwareExecute API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastFirmwareExecute API success", phy_info->phy_addr);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_fw_load_eeprom(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    bcm_plp_firmware_load_type_t firmware_load_type;
    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodProgEEPROM;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL;

    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastNone);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastNone API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastNone API success", phy_info->phy_addr);
    }

    sleep(1);
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_fw_load_eeprom_skip(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    bcm_plp_firmware_load_type_t firmware_load_type;
    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodProgEEPROM;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadSkip;
    firmware_load_type.firmware_loader = NULL;

    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastNone);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastNone API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastNone API success", phy_info->phy_addr);
    }

    sleep(1);
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_fw_load_broadcast(sap_bcm_phy_info_t **phy_info_list, int phy_num)
{
    int i, rv;
    bcm_plp_access_t plp_access;
    bcm_plp_firmware_load_type_t firmware_load_type;
    sap_bcm_phy_info_t *phy_info;


    for (i=0; i<phy_num; i++) {
        phy_info = phy_info_list[i];

        memset(&plp_access, 0, sizeof(bcm_plp_access_t));
        memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
        plp_access.phy_addr = phy_info->phy_addr;
        firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
        firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
        firmware_load_type.firmware_loader = NULL;

        rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
                &firmware_load_type, bcmpmFirmwareBroadcastCoreReset);
        if (rv != BCM_PM_IF_SUCCESS) {
            SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastCoreReset API err %d", phy_info->phy_addr, rv);
            return SAP_STATUS_FAILURE;
        } else {
            SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastCoreReset API success", phy_info->phy_addr);
        }
        rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
                &firmware_load_type, bcmpmFirmwareBroadcastEnable);
        if (rv != BCM_PM_IF_SUCCESS) {
            SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastEnable API err %d", phy_info->phy_addr, rv);
            return SAP_STATUS_FAILURE;
        } else {
            SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastEnable API success", phy_info->phy_addr);
        }
    }
    phy_info = phy_info_list[0];
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    plp_access.phy_addr = phy_info->phy_addr;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL;

    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastFirmwareExecute);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastFirmwareExecute API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastFirmwareExecute API success", phy_info->phy_addr);
    }

    for (i=0; i<phy_num; i++) {
        phy_info = phy_info_list[i];

        memset(&plp_access, 0, sizeof(bcm_plp_access_t));
        memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
        plp_access.phy_addr = phy_info->phy_addr;
        firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
        firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
        firmware_load_type.firmware_loader = NULL;

        rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
                &firmware_load_type, bcmpmFirmwareBroadcastFirmwareVerify);
        if (rv != BCM_PM_IF_SUCCESS) {
            SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastFirmwareVerify API err %d", phy_info->phy_addr, rv);
            return SAP_STATUS_FAILURE;
        } else {
            SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastFirmwareVerify API success", phy_info->phy_addr);
        }
        rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
                &firmware_load_type, bcmpmFirmwareBroadcastEnd);
        if (rv != BCM_PM_IF_SUCCESS) {
            SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastEnd API err %d", phy_info->phy_addr, rv);
            return SAP_STATUS_FAILURE;
        } else {
            SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastEnd API success", phy_info->phy_addr);
        }
    }

    sleep(1);
    return SAP_STATUS_SUCCESS;
}

/* for warm_boot */
static int sap_barchetta_fw_load_none(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    bcm_plp_firmware_load_type_t firmware_load_type;

    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodNone;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadSkip;
    firmware_load_type.firmware_loader = NULL;
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastNone);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastNone API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastNone API success", phy_info->phy_addr);
    }
    sleep(1);
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_fw_load_full_process(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    bcm_plp_firmware_load_type_t firmware_load_type;

    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL;
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastCoreReset);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastCoreReset API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastCoreReset API success", phy_info->phy_addr);
    }
    sleep(3);
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastEnable);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastEnable API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastEnable API success", phy_info->phy_addr);
    }
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastFirmwareExecute);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastFirmwareExecute API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastFirmwareExecute API success", phy_info->phy_addr);
    }
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastFirmwareVerify);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastFirmwareVerify API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastFirmwareVerify API success", phy_info->phy_addr);
    }
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastEnd);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastEnd API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastEnd API success", phy_info->phy_addr);
    }

    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_fw_load_unicast(sap_bcm_phy_info_t *phy_info)
{
    int rv;
    bcm_plp_access_t plp_access;
    bcm_plp_firmware_load_type_t firmware_load_type;

    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    plp_access.platform_ctxt = phy_info;
    plp_access.phy_addr = phy_info->phy_addr;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL;
    rv = bcm_plp_init_fw_bcast(phy_info->chip_name, plp_access, sap_bcm_mdio_read, sap_bcm_mdio_write,
            &firmware_load_type, bcmpmFirmwareBroadcastNone);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcmpmFirmwareBroadcastNone API err %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x bcmpmFirmwareBroadcastNone API success", phy_info->phy_addr);
    }
    sleep(1);
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_tx_fir_init(int unit, int port, int if_side)
{
    int i;
    int rv = 0;
    int medium = SAP_MEDIUM_INVALID;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    bcm_plp_pam4_tx_t tx_info;
    sap_tx_fir_t sap_tx_info;
    bcm_plp_serdes_tx_tap_mode_t serdes_mode;
    uint32_t lane_num, lane_map, lane_idx;
    BCM_PLP_DEVICE_AUX_MODE aux_mode;
    int speed = 0, if_type = 0, ref_clk = 0, if_mode = 0;

    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    lane_map = phy_access.lane_map;
    GET_PORT_LANE_NUM(lane_map, lane_num);
    for (i=1; i <= lane_num; i++) {
        GET_PORT_SINGLE_LANE_MAP(lane_map, i, phy_access.lane_map);
        GET_PORT_SINGLE_LANE_INDEX(phy_access.lane_map, lane_idx);
        memset(&aux_mode, 0, sizeof(BCM_PLP_DEVICE_AUX_MODE));
        rv = bcm_plp_mode_config_get(phy_info->chip_name, phy_access, &speed, &if_type, &ref_clk,
            &if_mode, (void*)&aux_mode);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_mode_config_get failed with rv:%d", unit, port, rv);
            return SAP_STATUS_FAILURE;
        }
        if (aux_mode.modulation_mode == bcmplpModulationPAM4) {
            serdes_mode = bcmplpTapModePAM4_6TAP;
        } else if (aux_mode.modulation_mode == bcmplpModulationNRZ) {
            serdes_mode = bcmplpTapModeNRZ_LP_3TAP;
        } else {
            SAP_BCM_LOG_ERR("unit[%d] port[%d] lane[%d], port mode err, mode = %d", unit, port, i, aux_mode.modulation_mode);
            return SAP_STATUS_FAILURE;
        }
        rv = tx_fir_cfg_parse(SAP_BCM_CFG_FILE, phy_access.phy_addr, if_side, lane_idx, serdes_mode, medium, &sap_tx_info);
        if (rv != SAP_STATUS_SUCCESS) {
            // SAP_LOG_NOTICE("unit[%d] port[%d], tx_fir_cfg_parse got empty with rv:%d", unit, port, rv);
            break;
        }
        memset(&tx_info, 0, sizeof(bcm_plp_pam4_tx_t));
        rv = bcm_plp_pam4_tx_get(phy_info->chip_name, phy_access, &tx_info);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_pam4_tx_get failed with rv:%d", unit, port, rv);
            return SAP_STATUS_FAILURE;
        }
        tx_info.serdes_tx_tap_mode = serdes_mode;
        tx_info.main  = sap_tx_info.main;
        tx_info.pre   = sap_tx_info.pre;
        tx_info.post  = sap_tx_info.post;
        tx_info.post2 = sap_tx_info.post2;
        tx_info.post3 = sap_tx_info.post3;
        tx_info.pre2  = sap_tx_info.pre2;
        tx_info.pre3  = sap_tx_info.pre3;
        if ((tx_info.main == 0) && (tx_info.pre == 0) && (tx_info.post == 0) && (tx_info.post2 == 0) && (tx_info.post3 == 0) && (tx_info.pre2 == 0)) {
            return SAP_STATUS_INVALID_PARAMETER;
        }
        rv = bcm_plp_pam4_tx_set(phy_info->chip_name, phy_access, &tx_info);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_pam4_tx_set failed with rv:%d", unit, port, rv);
            return SAP_STATUS_FAILURE;
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_polarity_init(sap_bcm_phy_info_t *phy_info)
{
    // sap_bcm_port_info_t *port_info;
    // int rv, tx_rx, offset, if_side, lane_idx, unit, port;
    // list_for_each_entry(port_info, &global_port_list_head, node, sap_bcm_port_info_t) {
    //     if (port_info->phy_info != phy_info) {
    //         continue;
    //     }
    //     unit = port_info->unit;
    //     port = port_info->port;
    //     for(lane_idx=0; lane_idx<BCM_PHY_MAX_PORT_NUM; lane_idx++) {
    //         if (phy_info->ports[lane_idx] != port) {
    //             continue;
    //         }
    //         offset = lane_idx;
    //         break;
    //     }
    //     // BCM_SYSTEM_SIDE
    //     // BCM_LINE_SIDE
    //     rv = sap_bcm_polarity_set(unit, port, int if_side, int tx_rx, uint32_t polarity, 0);
    // }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_flow_control_set(int unit, int port, int if_side, bcm_plp_mac_flow_control_t flow_option)
{
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    bcm_plp_mac_access_t mac_access;
    bcm_plp_mac_flow_control_t flow_option_get = 0;
    memset(&mac_access, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    memcpy(&mac_access.phy_info, &phy_access, sizeof(bcm_plp_access_t));
    rv = bcm_plp_mac_flow_control_set(phy_info->chip_name, mac_access, flow_option);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("FAIL: Failed to set flow control option for lane-map 0x%x of PHY-0x%x(ret = %d)!",
        mac_access.phy_info.lane_map, mac_access.phy_info.phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PASS: Successfully set flow control option %d for lane-map 0x%x of PHY-0x%x!",
        flow_option, mac_access.phy_info.lane_map, mac_access.phy_info.phy_addr);
    }
    rv = bcm_plp_mac_flow_control_get(phy_info->chip_name, mac_access, &flow_option_get);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("FAIL: Failed to get flow control option for lane-map 0x%x of PHY-0x%x(ret = %d)!",
        mac_access.phy_info.lane_map, mac_access.phy_info.phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PASS: Successfully get flow control option %d for lane-map 0x%x of PHY-0x%x!",
        flow_option_get, mac_access.phy_info.lane_map, mac_access.phy_info.phy_addr);
    }
    if (flow_option != flow_option_get) {
        SAP_BCM_LOG_ERR("FAIL: flow options does not match for lane-map 0x%x of PHY-0x%x!",
        mac_access.phy_info.lane_map, mac_access.phy_info.phy_addr);
        return SAP_STATUS_FAILURE;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_pre_port_init(sap_bcm_port_info_t *port_info)
{
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_post_port_init(sap_bcm_port_info_t *port_info)
{
    int rv;

    if (port_info->linktrain_sys == 1) {
        rv = sap_bcm_linktraining_set(port_info->unit, port_info->port, PLP_SYS_IF_SIDE, true);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], sys side sap_bcm_linktraining_set failed with rv: %d", port_info->unit, port_info->port, rv);
            return SAP_STATUS_FAILURE;
        }
    } else {
        /* 强制预加重 */
        // rv = sap_bcm_tx_fir_init(port_info->unit, port_info->port, PLP_SYS_IF_SIDE);
        // if (rv != SAP_STATUS_SUCCESS) {
        //     SAP_LOG_NOTICE("unit[%d] port[%d], sys side sap_bcm_tx_fir_init failed with rv: %d", port_info->unit, port_info->port, rv);
        //     return SAP_STATUS_FAILURE;
        // }
        // rv = sap_barchetta_lane_init(port_info->unit, port_info->port, PLP_SYS_IF_SIDE);
        // if (rv != SAP_STATUS_SUCCESS) {
        //     SAP_LOG_NOTICE("unit[%d] port[%d], sys side sap_barchetta_lane_init failed with rv: %d", port_info->unit, port_info->port, rv);
        //     return SAP_STATUS_FAILURE;
        // }
    }
    if (port_info->linktrain_line == 1) {
        rv = sap_bcm_linktraining_set(port_info->unit, port_info->port, PLP_LINE_IF_SIDE, true);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], line side sap_bcm_linktraining_set failed with rv: %d", port_info->unit, port_info->port, rv);
            return SAP_STATUS_FAILURE;
        }
    } else {
        /* 强制预加重 */
        // rv = sap_bcm_tx_fir_init(port_info->unit, port_info->port, PLP_LINE_IF_SIDE);
        // if (rv != SAP_STATUS_SUCCESS) {
        //     SAP_LOG_NOTICE("unit[%d] port[%d], line side sap_bcm_tx_fir_init failed with rv: %d", port_info->unit, port_info->port, rv);
        //     return SAP_STATUS_FAILURE;
        // }
        // rv = sap_barchetta_lane_init(port_info->unit, port_info->port, PLP_LINE_IF_SIDE);
        // if (rv != SAP_STATUS_SUCCESS) {
        //     SAP_LOG_NOTICE("unit[%d] port[%d], line side sap_barchetta_lane_init failed with rv: %d", port_info->unit, port_info->port, rv);
        //     return SAP_STATUS_FAILURE;
        // }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_port_init(sap_bcm_port_info_t *port_info)
{
    int lane_index, rv;
    bcm_plp_access_t plp_access;
    bcm_plp_mac_access_t mac_access;
    bcm_plp_barchetta_device_aux_modes_t aux_mode_sys_set, aux_mode_line_set;
    bcm_plp_port_type_t port_type;
    bcm_plp_lane_data_rate_t sys_lane_data_rete, line_lane_data_rete;
    int sys_lane_count, line_lane_count;
    if (port_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&plp_access, 0, sizeof(bcm_plp_access_t));
    memset(&mac_access, 0, sizeof(bcm_plp_mac_access_t));
    memset(&aux_mode_sys_set,  0, sizeof(bcm_plp_barchetta_device_aux_modes_t));
    memset(&aux_mode_line_set, 0, sizeof(bcm_plp_barchetta_device_aux_modes_t));
    plp_access.platform_ctxt = port_info->phy_info;
    plp_access.phy_addr = (port_info->phy_info)->phy_addr;
    GET_PORT_LANE_NUM(port_info->lanemap_sys, sys_lane_count);
    GET_PORT_LANE_NUM(port_info->lanemap_line, line_lane_count);

    GET_PORT_LANE_RATE(port_info->speed, sys_lane_count, sys_lane_data_rete);
    GET_PORT_LANE_RATE(port_info->speed, line_lane_count, line_lane_data_rete);
    plp_access.lane_map = port_info->lanemap_sys;
    plp_access.if_side  = BCM_SYSTEM_SIDE;
    aux_mode_sys_set.lane_data_rate           = sys_lane_data_rete;
    aux_mode_sys_set.modulation_mode          = MODULATION_MODE_GET(sys_lane_data_rete);
    aux_mode_sys_set.clock_mode               = SYS_CLOCK_MODE;
    aux_mode_sys_set.ll_mode                  = SYS_LL_MODE;
    aux_mode_sys_set.failover_lane_map        = SYS_FAILOVER_LANEMAP;

    rv = bcm_plp_mode_config_set((port_info->phy_info)->chip_name, plp_access, port_info->speed,
                                port_info->if_type_sys, REF_CLOCK, DEFAULT_IF_MODE, (void *)&aux_mode_sys_set);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x lane_map:0x%x if_side:%d lane_data_rate:%d", plp_access.phy_addr, plp_access.lane_map, plp_access.if_side, aux_mode_sys_set.lane_data_rate);
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_mode_config_set failed with rv: %d", plp_access.phy_addr, rv);
        return rv;
    }
    plp_access.lane_map = port_info->lanemap_line;
    plp_access.if_side  = BCM_LINE_SIDE;
    aux_mode_line_set.lane_data_rate          = line_lane_data_rete;
    aux_mode_line_set.modulation_mode         = MODULATION_MODE_GET(line_lane_data_rete);
    aux_mode_line_set.clock_mode              = SYS_CLOCK_MODE;
    aux_mode_line_set.ll_mode                 = SYS_LL_MODE;
    aux_mode_line_set.failover_lane_map       = SYS_FAILOVER_LANEMAP;

    rv = bcm_plp_mode_config_set((port_info->phy_info)->chip_name, plp_access, port_info->speed,
                                port_info->if_type_line, REF_CLOCK, DEFAULT_IF_MODE, (void *)&aux_mode_line_set);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_mode_config_set failed with rv: %d", plp_access.phy_addr, rv);
        return rv;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_barchetta_lane_init(int unit, int port, int if_side)
{
    int i, rv;
    int lane_num;
    sap_bcm_port_info_t *port_info;
    rv = sap_port_info_get(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_port_info_get failed!", unit, port);
        return SAP_STATUS_FAILURE;
    }
    if (if_side == PLP_SYS_IF_SIDE) {
        GET_PORT_LANE_NUM(port_info->lanemap_sys, lane_num);
        for (i = 0; i < lane_num; i++) {
            int nr_er;
            if ((port_info->force_nr_sys) & (1 << i)) {
                nr_er = 0;
            }else if ((port_info->force_er_sys) & (1 << i)) {
                nr_er = 1;
            }
            rv = sap_bcm_channel_reach_set(port_info->unit, port_info->port, if_side, i, nr_er);
            if (rv != SAP_STATUS_SUCCESS) {
                SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_bcm_channel_reach_set failed with rv:%d", unit, port, rv);
                return rv;
            }
        }
    } else {
        GET_PORT_LANE_NUM(port_info->lanemap_line, lane_num);
        for (i = 0; i < lane_num; i++) {
            int nr_er;
            if ((port_info->force_nr_line) & (1 << i)) {
                nr_er = 0;
            }else if ((port_info->force_er_line) & (1 << i)) {
                nr_er = 1;
            }
            rv = sap_bcm_channel_reach_set(port_info->unit, port_info->port, if_side, i, nr_er);
            if (rv != SAP_STATUS_SUCCESS) {
                SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_bcm_channel_reach_set failed with rv:%d", unit, port, rv);
                return rv;
            }
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_get_profile_info(int profile_id, int speed, int split_num, int port_id, sap_bcm_port_info_t *port_info)
{
    int rv, val;
    char config_prefix[SAP_CFG_FILE_STR_MAX];
    if (port_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:fec_line", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->fec_line = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:fec_sys", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->fec_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:iftype_line", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->if_type_line = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:iftype_sys", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->if_type_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:lanemap_line", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->lanemap_line = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:lanemap_sys", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->lanemap_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:force_nr_sys", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->force_nr_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:force_nr_line", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->force_nr_line = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:force_er_sys", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->force_er_sys = val;
    }
    snprintf(config_prefix, sizeof(config_prefix), "plp_profile_%d_speed_%d_split_%d.%d:force_er_line", profile_id, speed, split_num, port_id);
    rv = sap_get_cfg_info(SAP_BCM_CFG_FILE, config_prefix, &val, false);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_DBG("%s get empty.", config_prefix);
        return SAP_STATUS_FAILURE; 
    } else {
        port_info->force_er_line = val;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_phy_info_get(int unit, int port, sap_bcm_phy_info_t **phy_info)
{
    int rv;
    sap_bcm_port_info_t *port_info;
    if (phy_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    rv = sap_port_info_get(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_port_info_get failed!", unit, port);
        return SAP_STATUS_FAILURE;
    }
    *phy_info = port_info->phy_info;
    return SAP_STATUS_SUCCESS;
}

static int sap_phy_access_get(int unit, int port, int if_side, bcm_plp_access_t *phy_access)
{
    int rv;
    sap_bcm_port_info_t *port_info;
    if (phy_access == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    rv = sap_port_info_get(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_port_info_get failed!", unit, port);
        return SAP_STATUS_FAILURE;
    }
    if_side = (if_side == PLP_SYS_IF_SIDE) ? BCM_SYSTEM_SIDE : BCM_LINE_SIDE;
    phy_access->platform_ctxt = port_info->phy_info;
    phy_access->if_side = if_side;
    phy_access->phy_addr = (port_info->phy_info)->phy_addr;
    if (if_side == BCM_SYSTEM_SIDE) {
        phy_access->lane_map = port_info->lanemap_sys;
    } else {
        phy_access->lane_map = port_info->lanemap_line;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_prbs_change(int poly, bcm_plp_pm_prbs_poly_t *bcm_poly)
{
    SAP_POINT_CHECK_RV(bcm_poly, "bcm_poly", SAP_STATUS_INVALID_PARAMETER);
    switch(poly) {
        case SAP_PRBS_POLY_P7:
            *bcm_poly = bcm_pm_PrbsPoly7;
            break;
        case SAP_PRBS_POLY_P9:
            *bcm_poly = bcm_pm_PrbsPoly9;
            break;
        case SAP_PRBS_POLY_P11:
            *bcm_poly = bcm_pm_PrbsPoly11;
            break;
        case SAP_PRBS_POLY_P13:
            *bcm_poly = bcm_pm_PrbsPoly13;
            break;
        case SAP_PRBS_POLY_P15:
            *bcm_poly = bcm_pm_PrbsPoly15;
            break;
        case SAP_PRBS_POLY_P23:
            *bcm_poly = bcm_pm_PrbsPoly23;
            break;
        case SAP_PRBS_POLY_P31:
            *bcm_poly = bcm_pm_PrbsPoly31;
            break;
        case SAP_PRBS_POLY_P49:
            *bcm_poly = bcm_pm_PrbsPoly49;
            break;
        case bcm_pm_PrbsPoly58:
            *bcm_poly = bcm_pm_PrbsPoly58;
            break;
        default:
            return SAP_STATUS_INVALID_PARAMETER;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_port_prbs_set(int unit, int port, int poly, int inv, int if_side, int lane)
{
    int rv = 0;
    uint32_t tx_rx = 0, lb = 0, ena_dis = 1;
    // uint32_t tx_rx = 0, inv = 0, lb = 0, ena_dis = 1;
    bcm_plp_access_t phy_access;
    bcm_plp_pm_prbs_poly_t bcm_poly;
    sap_bcm_phy_info_t *phy_info;
    uint32_t lane_num, lane_map;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    lane_map = phy_access.lane_map;
    GET_PORT_LANE_NUM(lane_map, lane_num);
    if (lane > lane_num) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], lane:%d, lane_num:%d, lane index error", unit, port, lane, lane_num);
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane != 0) {
        GET_PORT_SINGLE_LANE_MAP(lane_map, lane, phy_access.lane_map);
        SAP_BCM_LOG_DBG("unit[%d] port[%d], lane[%d] if_side[%d], lane_map is 0x%x", unit, port, lane, if_side, phy_access.lane_map);
    } else {
        SAP_BCM_LOG_DBG("unit[%d] port[%d], if_side[%d], lane_map is 0x%x", unit, port, if_side, phy_access.lane_map);
    }
    SAP_RV_CHECK(phy_info, "sap_bcm_prbs_change", sap_bcm_prbs_change(poly, &bcm_poly));
    rv = bcm_plp_prbs_set(phy_info->chip_name, phy_access, tx_rx, bcm_poly, inv, lb, ena_dis);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_prbs_set failed with rv:%d", unit, port, rv);
        return SAP_STATUS_FAILURE;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_port_prbs_get(int unit, int port, int if_side, sap_prbs_status_t *prbs_info)
{
    int rv = 0;
    uint32_t prbs_lock = 0, lock_loss = 0, err_cnt = 0;
    bcm_plp_access_t phy_access;
    sap_bcm_phy_info_t *phy_info;
    uint32_t lane_i, lane_map, lane_num;
    if (prbs_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    lane_map = phy_access.lane_map;
    GET_PORT_LANE_NUM(lane_map, lane_num);
    for (lane_i = 1; lane_i <= lane_num; lane_i++) {
        prbs_lock = 0;
        lock_loss = 0;
        err_cnt = 0;
        GET_PORT_SINGLE_LANE_MAP(lane_map, lane_i, phy_access.lane_map);
        SAP_BCM_LOG_DBG("unit[%d] port[%d], lane[%d] if_side[%d], lane_map is 0x%x", unit, port, lane_i, if_side, phy_access.lane_map);
        /* 读清的 */
        rv = bcm_plp_prbs_status_get(phy_info->chip_name, phy_access, &prbs_lock, &lock_loss, &err_cnt);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_prbs_status_get failed with rv:%d", unit, port, rv);
        }
        prbs_info[lane_i - 1].is_get = true;
        prbs_info[lane_i - 1].lane_err_cnt = err_cnt;
        prbs_info[lane_i - 1].lock_loss = lock_loss;
        prbs_info[lane_i - 1].prbs_lock = prbs_lock;
    }
    return SAP_STATUS_SUCCESS;
}
/* time_v 毫秒*/
static int sap_bcm_port_prbs_ber_get(int unit, int port, int if_side, int time_v, sap_prbs_status_t *prbs_info)
{
    int i;
    int rv = 0;
    bcm_plp_access_t phy_access;
    sap_bcm_phy_info_t *phy_info;
    int speed = 0, if_type = 0, ref_clk = 0, if_mode = 0;
    BCM_PLP_DEVICE_AUX_MODE aux_mode;
    int lane_num;
    double ber;
    uint32_t count;

    if (prbs_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    sap_bcm_port_prbs_get(unit, port, if_side, prbs_info);
    memset(&aux_mode, 0, sizeof(BCM_PLP_DEVICE_AUX_MODE));
    rv = bcm_plp_mode_config_get(phy_info->chip_name, phy_access, &speed, &if_type, &ref_clk,
        &if_mode, (void*)&aux_mode);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_mode_config_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_FAILURE;
    }
    for (i = 0; i < BCM_PHY_MAX_LANE; i++) {
        if (prbs_info[i].is_get != true) {
            continue;
        }
        ber = (((double)prbs_info[i].lane_err_cnt)/((double)time_v/1000))/((double)aux_mode.lane_data_rate*1024*1024);
        SAP_BCM_LOG_DBG("Unit[%d] Port[%3d] Lane[%d] err_cnt[%10d] Ber %4.2e, time_v[%d]", unit, port, i, prbs_info[i].lane_err_cnt, ber, time_v);
        prbs_info[i].ber = ber;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_port_prbs_clear(int unit, int port, int if_side)
{
    int rv = 0;
    uint32_t tx_rx = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    /* tx_rx == 0 means both dir */
    rv = bcm_plp_prbs_clear(phy_info->chip_name, phy_access, tx_rx);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_prbs_clear failed with rv:%d", unit, port, rv);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_linktraining_set(int unit, int port, int if_side, int enable)
{
    int rv = 0;
    uint32_t en_value;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    bcm_plp_dsrds_firmware_lane_config_t dsrds_lane_config;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }

    if (if_side == PLP_SYS_IF_SIDE) {
        memset(&dsrds_lane_config, 0, sizeof(bcm_plp_dsrds_firmware_lane_config_t));
        rv = bcm_plp_dsrds_firmware_lane_config_get(phy_info->chip_name, phy_access, &dsrds_lane_config);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_dsrds_firmware_lane_config_get failed with rv:%d", unit, port, rv);
        }
        /* enable auto peaking */
        dsrds_lane_config.AutoPeakingEnable = 1;
        dsrds_lane_config.OppositeCdrFirst = 0;
        rv = bcm_plp_dsrds_firmware_lane_config_set(phy_info->chip_name, phy_access, dsrds_lane_config);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_dsrds_firmware_lane_config_set failed with rv:%d", unit, port, rv);
        }
        en_value = (enable ? 0x11 : 0x0);
    } else {
        en_value = (enable == 0x1 ? 0x1 : 0x0);
    }

    rv = bcm_plp_force_tx_training_set(phy_info->chip_name, phy_access, en_value);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_force_tx_training_set failed with rv:%d", unit, port, rv);
    }
    sleep(1);  /** Sleep for one second after training enabled ***/
    en_value = 0;
    rv = bcm_plp_force_tx_training_get(phy_info->chip_name, phy_access, &en_value);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_force_tx_training_get failed with rv:%d", unit, port, rv);
    } else {
        SAP_BCM_LOG_DBG("unit[%d] port[%d], training get at %s side with training get value : 0x%x", unit, port, BCM_IF_SIDE_STR(if_side), en_value);
    }
    /* recover force channel reach mode and tx_fir*/
    if (!enable) {
        rv = sap_barchetta_lane_init(unit, port, if_side);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_LOG_NOTICE("unit[%d] port[%d], sap_barchetta_lane_init at %s side failed with rv: %d", unit, port, BCM_IF_SIDE_STR(if_side), rv);
            return SAP_STATUS_FAILURE;
        }
        rv = sap_bcm_tx_fir_init(unit, port, if_side);
        if (rv != SAP_STATUS_SUCCESS) {
            SAP_LOG_NOTICE("unit[%d] port[%d], sap_bcm_tx_fir_init at %s side failed with rv: %d", unit, port, BCM_IF_SIDE_STR(if_side), rv);
            return SAP_STATUS_FAILURE;
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_tx_fir_set(int unit, int port, int if_side, int lane, sap_tx_fir_t tx_fir)
{
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    bcm_plp_pam4_tx_t tx_info;
    uint32_t lane_num, lane_map;
    int speed = 0, if_type = 0, ref_clk = 0, if_mode = 0;
    BCM_PLP_DEVICE_AUX_MODE aux_mode;

    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    lane_map = phy_access.lane_map;
    GET_PORT_LANE_NUM(lane_map, lane_num);
    if (lane > lane_num) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], lane:%d, lane_num:%d, lane index error", unit, port, lane, lane_num);
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane != 0) {
        GET_PORT_SINGLE_LANE_MAP(lane_map, lane, phy_access.lane_map);
        SAP_BCM_LOG_DBG("unit[%d] port[%d], lane[%d] if_side[%d], lane_map is 0x%x", unit, port, lane, if_side, phy_access.lane_map);
    }
    memset(&tx_info, 0, sizeof(bcm_plp_pam4_tx_t));
    rv = bcm_plp_pam4_tx_get(phy_info->chip_name, phy_access, &tx_info);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_pam4_tx_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_FAILURE;
    }
    SAP_BCM_LOG_DBG("bcm_plp_pam4_tx_get config get phy_addr = %d, pre = %d, main = %d, post = %d, "
        "post2 = %d, post3 = %d, amp = %d, pre2 = %d, serdes_tx_tap_mode = %d, tx_precode = %d",
        phy_access.phy_addr, tx_info.pre, tx_info.main, tx_info.post, tx_info.post2, tx_info.post3,
        tx_info.amp, tx_info.pre2, tx_info.serdes_tx_tap_mode, tx_info.tx_precode);

    memset(&aux_mode, 0, sizeof(BCM_PLP_DEVICE_AUX_MODE));
    rv = bcm_plp_mode_config_get(phy_info->chip_name, phy_access, &speed, &if_type, &ref_clk,
        &if_mode, (void*)&aux_mode);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_mode_config_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_FAILURE;
    }
    if (aux_mode.modulation_mode == bcmplpModulationPAM4) {
        tx_info.serdes_tx_tap_mode = bcmplpTapModePAM4_6TAP;
    } else if (aux_mode.modulation_mode == bcmplpModulationNRZ) {
        tx_info.serdes_tx_tap_mode = bcmplpTapModeNRZ_LP_3TAP;
    } else {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], port mode err, mode=%d", unit, port, aux_mode.modulation_mode);
        return SAP_STATUS_FAILURE;
    }
    if ((tx_fir.main == 0) && (tx_fir.pre == 0) && (tx_fir.post == 0) && (tx_fir.post2 == 0) && (tx_fir.post3 == 0) && (tx_fir.pre2 == 0)) {
        return SAP_STATUS_FAILURE;
    }
    tx_info.pre2  = tx_fir.pre2;
    tx_info.pre   = tx_fir.pre;
    tx_info.main  = tx_fir.main;
    tx_info.post  = tx_fir.post;
    tx_info.post2 = tx_fir.post2;
    tx_info.post3 = tx_fir.post3;
    rv = bcm_plp_pam4_tx_set(phy_info->chip_name, phy_access, &tx_info);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_pam4_tx_set failed with rv:%d", unit, port, rv);
        return SAP_STATUS_FAILURE;
    }
    SAP_BCM_LOG_DBG("bcm_plp_pam4_tx_set config set phy_addr = %d, pre = %d, main = %d, post = %d, "
        "post2 = %d, post3 = %d, amp = %d, pre2 = %d,  serdes_tx_tap_mode = %d, tx_precode = %d",
        phy_access.phy_addr, tx_info.pre, tx_info.main, tx_info.post, tx_info.post2, tx_info.post3,
        tx_info.amp, tx_info.pre2, tx_info.serdes_tx_tap_mode, tx_info.tx_precode);

    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_tx_fir_get(int unit, int port, int if_side, int lane, sap_tx_fir_t *tx_fir)
{
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    bcm_plp_pam4_tx_t tx_info;
    uint32_t lane_num, lane_map;
    int speed = 0, if_type = 0, ref_clk = 0, if_mode = 0;
    BCM_PLP_DEVICE_AUX_MODE aux_mode;

    if (tx_fir == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    lane_map = phy_access.lane_map;
    GET_PORT_LANE_NUM(lane_map, lane_num);
    if (lane > lane_num) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], lane:%d, lane_num:%d, lane index error", unit, port, lane, lane_num);
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane != 0) {
        GET_PORT_SINGLE_LANE_MAP(lane_map, lane, phy_access.lane_map);
        SAP_BCM_LOG_DBG("unit[%d] port[%d], %s side, lane_map is 0x%x", unit, port, BCM_IF_SIDE_STR(if_side), phy_access.lane_map);
    } else {
        SAP_BCM_LOG_ERR("lane: %d invalid!", lane);
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&tx_info, 0, sizeof(bcm_plp_pam4_tx_t));
    rv = bcm_plp_pam4_tx_get(phy_info->chip_name, phy_access, &tx_info);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_pam4_tx_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_FAILURE;
    }
    SAP_BCM_LOG_DBG("bcm_plp_pam4_tx_get config get phy_addr = %d, pre = %d, main = %d, post = %d, "
        "post2 = %d, post3 = %d, amp = %d, pre2 = %d, serdes_tx_tap_mode = %d, tx_precode = %d",
        phy_access.phy_addr, tx_info.pre, tx_info.main, tx_info.post, tx_info.post2, tx_info.post3,
        tx_info.amp, tx_info.pre2, tx_info.serdes_tx_tap_mode, tx_info.tx_precode);

    tx_fir->pre2  = tx_info.pre2;
    tx_fir->pre   = tx_info.pre;
    tx_fir->main  = tx_info.main;
    tx_fir->post  = tx_info.post;
    tx_fir->post2 = tx_info.post2;
    tx_fir->post3 = tx_info.post3;

    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_dsc_dump(int unit, int port, int if_side, int flag, int lane)
{
    int rv = 0;
    uint32_t link_sts = 0, lt_en = 0, training_fail = 0, trained = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    bcm_plp_mac_access_t mac_access;
    uint32_t lane_num, lane_map;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    memset(&mac_access, 0, sizeof(bcm_plp_mac_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    lane_map = phy_access.lane_map;
    GET_PORT_LANE_NUM(lane_map, lane_num);
    if (lane > lane_num) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], lane is error, lane:%d, lane_num:%d", unit, port, lane, lane_num);
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane != 0) {
        GET_PORT_SINGLE_LANE_MAP(lane_map, lane, phy_access.lane_map);
        SAP_BCM_LOG_DBG("unit[%d] port[%d], %s side, lane_map is 0x%x", unit, port, phy_access.lane_map, BCM_IF_SIDE_STR(if_side));
    }
    rv = bcm_plp_link_status_get(phy_info->chip_name, phy_access, &link_sts);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_link_status_get failed with rv:%d", unit, port, rv);
    } else {
        SAP_BCM_LOG_DBG("unit[%d] port[%d], link status = %d", unit, port, link_sts);
    }
    rv = bcm_plp_force_tx_training_status_get(phy_info->chip_name, phy_access, &lt_en, &training_fail, &trained);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_force_tx_training_status_get failed with rv:%d", unit, port, rv);
    } else {
        SAP_BCM_LOG_DBG("unit[%d] port[%d], lt_en=%d training_fail=%d trained=%d", unit, port, lt_en, training_fail, trained);
    }
    phy_access.flags = flag;
    rv = bcm_plp_phy_status_dump(phy_info->chip_name, phy_access);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_phy_status_dump failed with rv:%d", unit, port, rv);
    }
    // if (flag == 0) {
    //     phy_access.flags = BCM_PLP_INTERNAL_DUMP_L1;
    //     rv = bcm_plp_phy_status_dump(phy_info->chip_name, phy_access);
    //     if (rv != 0) {
    //         SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_phy_status_dump failed with rv:%d", unit, port, rv);
    //     }
    // } else {
    //     memcpy(&mac_access.phy_info, &phy_access, sizeof(bcm_plp_access_t));
    //     rv = bcm_plp_pcs_diagnostic_dump(phy_info->chip_name, mac_access);
    //     if (rv != 0) {
    //         SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_pcs_diagnostic_dump failed with rv:%d", unit, port, rv);
    //     }
    // }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_loopback_set(int unit, int port, int if_side, int lb_dir, int enable)
{
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;

    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = bcm_plp_loopback_set(phy_info->chip_name, phy_access, lb_dir, enable);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_loopback_set failed with rv:%d", unit, port, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("bcm_plp_loopback_set config set %s side lb_dir = %d, enable = %d",
                 BCM_IF_SIDE_STR(phy_access.if_side), lb_dir, enable);

    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_loopback_get(int unit, int port, int if_side, int lb_dir, int *enable)
{
    uint32_t en_value;
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;

    if (enable == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = bcm_plp_loopback_get(phy_info->chip_name, phy_access, lb_dir, &en_value);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_loopback_get failed with rv:%d", unit, port, rv);
        return rv;
    }
    *enable = (int)en_value;
    SAP_BCM_LOG_DBG("bcm_plp_loopback_get config get %s side lb_dir = %d, enable = %d",
            phy_access.if_side ? "Sys" : "Line", lb_dir, *enable);

    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_eyescan_dump(int unit, int port, int if_side, int lane)
{
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    uint32_t lane_num, lane_map;

    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    lane_map = phy_access.lane_map;
    GET_PORT_LANE_NUM(lane_map, lane_num);
    if (lane > lane_num) {
        SAP_BCM_LOG_ERR("lane is error, lane:%d, lane_num:%d", lane, lane_num);
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane != 0) {
        GET_PORT_SINGLE_LANE_MAP(lane_map, lane, phy_access.lane_map);
        SAP_BCM_LOG_DBG("unit[%d] port[%d], lane_map is 0x%x", unit, port, phy_access.lane_map);
    }
    rv = bcm_plp_display_eye_scan(phy_info->chip_name, phy_access);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_display_eye_scan failed with rv:%d", unit, port, rv);
        return rv;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_autopeak_set(int unit, int port, int if_side, int enable)
{
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    bcm_plp_dsrds_firmware_lane_config_t dsrds_lane_config;

    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    memset(&dsrds_lane_config, 0, sizeof(bcm_plp_dsrds_firmware_lane_config_t));
    rv = bcm_plp_dsrds_firmware_lane_config_get(phy_info->chip_name, phy_access, &dsrds_lane_config);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_dsrds_firmware_lane_config_get failed with rv:%d", unit, port, rv);
    }
    /* enable auto peaking */
    dsrds_lane_config.AutoPeakingEnable = enable ? 1 : 0;
    dsrds_lane_config.OppositeCdrFirst = 0;
    rv = bcm_plp_dsrds_firmware_lane_config_set(phy_info->chip_name, phy_access, dsrds_lane_config);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_dsrds_firmware_lane_config_set failed with rv:%d", unit, port, rv);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_squelch_set(int unit, int port, int if_side, int tx_rx, int enable, int lane)
{
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    uint32_t lane_num, lane_map;

    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    lane_map = phy_access.lane_map;
    GET_PORT_LANE_NUM(lane_map, lane_num);
    if (lane > lane_num) {
        SAP_BCM_LOG_ERR("lane is error, lane:%d, lane_num:%d", lane, lane_num);
        return SAP_STATUS_INVALID_PARAMETER;
    }
    if (lane != 0) {
        GET_PORT_SINGLE_LANE_MAP(lane_map, lane, phy_access.lane_map);
        SAP_BCM_LOG_DBG("unit[%d] port[%d], lane_map is 0x%x", unit, port, phy_access.lane_map);
    }
    if (tx_rx == PLP_TX_DIR) {
        rv = bcm_plp_tx_lane_control_set(phy_info->chip_name, phy_access, enable ? bcmpmTxSquelchOff : bcmpmTxSquelchOn);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_tx_lane_control_set failed with rv:%d", unit, port, rv);
            return rv;
        }
    } else {
        rv = bcm_plp_rx_lane_control_set(phy_info->chip_name, phy_access, enable ? bcmpmRxSquelchOff : bcmpmRxSquelchOn);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_rx_lane_control_set failed with rv:%d", unit, port, rv);
            return rv;
        }
    }

    SAP_BCM_LOG_DBG("squelch operate success, dir:%s, enable:%d", (tx_rx == PLP_TX_DIR) ? "tx" : "rx", enable);
    return SAP_STATUS_SUCCESS;
}

// typedef enum bcm_plp_fec_type_e {
//     bcmplpFecType91 = 0,              /* RS 528 / CL91 FEC */
//     bcmplpFecType74,                  /* Base-R / CL74 FEC */
//     bcmplpFecType108,                 /* RS 108 FEC */
//     bcmplpFecTypeRS544,               /* RS 544 FEC */
//     bcmplpFecTypeRS272,               /* RS 272 FEC */
//     bcmplpFecTypeRS544_2XN,           /* RS 544_2XN FEC */
//     bcmplpFecTypeRS272_2XN            /* RS 272_2XN FEC */
// } bcm_plp_fec_type_t;
// typedef int (*__bcm_plp_fec_corrected_error_counter_f)(bcm_plp_access_t phy_info, bcm_plp_fec_type_t fec_type, unsigned int *count);
// typedef int (*__bcm_plp_fec_uncorrected_error_counter_f)(bcm_plp_access_t phy_info, bcm_plp_fec_type_t fec_type, unsigned int *count);


static int sap_bcm_fec_mib_dump(int unit, int port, int if_side)
{
    // int rv = 0, index = 0;
    // sap_bcm_phy_info_t *phy_info;
    // bcm_plp_access_t  phy_access;
    // bcm_plp_fec_status_t fec_status;
    // bcm_plp_fec_type_t fec_type;
    // unsigned int count;
    // memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    // memset(&fec_status, 0, sizeof(bcm_plp_fec_status_t));
    // rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    // if (rv != SAP_STATUS_SUCCESS) {
    //     SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
    //     return SAP_STATUS_ITEM_NOT_FOUND;
    // }
    // rv = sap_phy_info_get(unit, port, &phy_info);
    // if (rv != SAP_STATUS_SUCCESS) {
    //     SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
    //     return SAP_STATUS_ITEM_NOT_FOUND;
    // }
    // SAP_BCM_LOG_INFO("phy_addr=0x%-3x lane_map=0x%-8x unit=%-2d port=%-3d", phy_access.phy_addr, phy_access.lane_map, unit, port);
    // for (fec_type = bcmplpFecType91; fec_type <=bcmplpFecTypeRS272_2XN; fec_type++) {
    //     count = 0;
    //     rv = bcm_plp_fec_corrected_error_counter(phy_info->chip_name, phy_access, fec_type, &count);
    //     if (rv != 0) {
    //         SAP_BCM_LOG_ERR("unit:%d, port:%d, bcm_plp_fec_corrected_error_counter failed with rv:%d", unit, port, rv);
    //     } else {
    //         SAP_BCM_LOG_INFO("unit:%d, port:%d, fec_type:%d, corrected_error: %u", unit, port, fec_type, count);
    //     }
    //     count = 0;
    //     rv = bcm_plp_fec_uncorrected_error_counter(phy_info->chip_name, phy_access, fec_type, &count);
    //     if (rv != 0) {
    //         SAP_BCM_LOG_ERR("unit:%d, port:%d, bcm_plp_fec_uncorrected_error_counter failed with rv:%d", unit, port, rv);
    //     } else {
    //         SAP_BCM_LOG_INFO("unit:%d, port:%d, fec_type:%d, uncorrected_error: %u", unit, port, fec_type, count);
    //     }
    // }
    return SAP_STATUS_SUCCESS;
}

static int g_inited = 0;
static int sap_bcm_fec_mib_dump_extend(int unit, int port, int if_side)
{
    int rv = 0, index = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    bcm_plp_fec_status_t fec_status;
    bcm_plp_fec_type_t fec_type;
    unsigned int count;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    memset(&fec_status, 0, sizeof(bcm_plp_fec_status_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    /*  */
    SAP_BCM_LOG_INFO("phy_addr=0x%-3x lane_map=0x%-8x unit=%-2d port=%-3d", phy_access.phy_addr, phy_access.lane_map, unit, port);
    /*  */
    if (g_inited == 0) {
        fec_status.fec_status_init = bcmplpFecStatusInitEnable;
        g_inited = 1;
    }
    rv = bcm_plp_fec_status(phy_info->chip_name, phy_access, &fec_status);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("bcm_plp_fec_status_get failed with rv:%d port_map: 0x%x", rv, phy_access.lane_map);
    } else {
        SAP_BCM_LOG_INFO("fec_status.align_lol_sticky=%u",                   fec_status.align_lol_sticky);
        SAP_BCM_LOG_INFO("fec_status.align_lock=%u",                         fec_status.align_lock);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.ieee_uncorr_cnt=%u",        fec_status.fec_err_cnt.ieee_uncorr_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.ieee_symbols_corr_cnt=%u",  fec_status.fec_err_cnt.ieee_symbols_corr_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.sys_am_lock=%u",            fec_status.fec_err_cnt.sys_am_lock);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.line_am_lock=%u",           fec_status.fec_err_cnt.line_am_lock);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.ieee_kp4_dec_ctrl=%u",      fec_status.fec_err_cnt.ieee_kp4_dec_ctrl);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.ieee_kp4_stat_ctrl=%u",     fec_status.fec_err_cnt.ieee_kp4_stat_ctrl);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.ieee_fec_lane_mapping=%u",  fec_status.fec_err_cnt.ieee_fec_lane_mapping);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.ieee_kp4_hi_ser_th_sp=%u",  fec_status.fec_err_cnt.ieee_kp4_hi_ser_th_sp);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.tot_frame_rev_cnt=%llu",    fec_status.fec_err_cnt.tot_frame_rev_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.tot_frame_rev_cnt=%llu",    fec_status.fec_err_cnt.tot_frame_rev_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.tot_frame_corr_cnt=%llu",   fec_status.fec_err_cnt.tot_frame_corr_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.tot_frame_uncorr_cnt=%llu", fec_status.fec_err_cnt.tot_frame_uncorr_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.tot_symbols_corr_cnt=%llu", fec_status.fec_err_cnt.tot_symbols_corr_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.tot_bits_corr_cnt[0]=%llu", fec_status.fec_err_cnt.tot_bits_corr_cnt[0]);
        SAP_BCM_LOG_INFO("fec_status.fec_err_cnt.tot_bits_corr_cnt[1]=%llu", fec_status.fec_err_cnt.tot_bits_corr_cnt[1]);
        for (index = 0; index < BCM_PLP_FEC_TOT_FRAMES_ERR_NUM; index++) {
            SAP_BCM_LOG_INFO("fec_status->fec_err_cnt.bcm_plp_tot_frames_err_cnt[%d]=%llu", index, fec_status.fec_err_cnt.bcm_plp_tot_frames_err_cnt[index]);
        }
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.ieee_fec_sts.ieee_rsfec_ctrl=%hu",       fec_status.fec_dump_status.ieee_fec_sts.ieee_rsfec_ctrl);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.ieee_fec_sts.ieee_rsfec_stat=%hu",       fec_status.fec_dump_status.ieee_fec_sts.ieee_rsfec_stat);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.ieee_fec_sts.ieee_fec_lane_mapping=%hu", fec_status.fec_dump_status.ieee_fec_sts.ieee_fec_lane_mapping);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.ieee_fec_sts.ieee_corr_cw_cnt=%hu",      fec_status.fec_dump_status.ieee_fec_sts.ieee_corr_cw_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.ieee_fec_sts.ieee_uncorr_cw_cnt=%hu",    fec_status.fec_dump_status.ieee_fec_sts.ieee_uncorr_cw_cnt);
        for (index = 0; index < 8; index++) {
            SAP_BCM_LOG_INFO("fec_status.fec_dump_status.ieee_fec_sts.ieee_symbols_corr_cnt_fln[%d]=%hu", index, fec_status.fec_dump_status.ieee_fec_sts.ieee_symbols_corr_cnt_fln[index]);
        }
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_1=%hu", fec_status.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_1);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_3=%hu", fec_status.fec_dump_status.ieee_fec_sts.ieee_fecpcs_alignment_status_3);
        for (index = 0; index < 8; index++) {
            SAP_BCM_LOG_INFO("fec_status.fec_dump_status.ieee_fec_sts.ieee_fecpcs_lane_mapping[%d]=%hu", index, fec_status.fec_dump_status.ieee_fec_sts.ieee_fecpcs_lane_mapping[index]);
        }
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_sts.igbox_clsn_sticky=%hhu",           fec_status.fec_dump_status.fec_sts.igbox_clsn_sticky);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_sts.am_lolock_sticky=%hhu",            fec_status.fec_dump_status.fec_sts.am_lolock_sticky);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_sts.dgbox_clsn_sticky=%hhu",           fec_status.fec_dump_status.fec_sts.dgbox_clsn_sticky);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_sts.hi_ser_sticky=%hhu",               fec_status.fec_dump_status.fec_sts.hi_ser_sticky);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_sts.xdec_err_sticky=%hhu",             fec_status.fec_dump_status.fec_sts.xdec_err_sticky);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_sts.fec_link_stat=%hhu",               fec_status.fec_dump_status.fec_sts.fec_link_stat);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_sts.fec_link_stat_sticky=%hhu",        fec_status.fec_dump_status.fec_sts.fec_link_stat_sticky);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_a_err_cnt.tot_frame_rev_cnt=%llu",     fec_status.fec_dump_status.fec_a_err_cnt.tot_frame_rev_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_a_err_cnt.tot_frame_corr_cnt=%llu",    fec_status.fec_dump_status.fec_a_err_cnt.tot_frame_corr_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_a_err_cnt.tot_frame_uncorr_cnt=%llu",  fec_status.fec_dump_status.fec_a_err_cnt.tot_frame_uncorr_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_a_err_cnt.tot_symbols_corr_cnt =%llu", fec_status.fec_dump_status.fec_a_err_cnt.tot_symbols_corr_cnt);
        for (index = 0; index < 2; index++) {
            SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[%d]=%llu", index, fec_status.fec_dump_status.fec_a_err_cnt.tot_bits_corr_cnt[index]);
        }
        for (index = 0; index < 16; index++) {
            SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_a_err_cnt.tot_frames_err_cnt[%d]=%llu", index, fec_status.fec_dump_status.fec_a_err_cnt.tot_frames_err_cnt[index]);
        }
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_b_err_cnt.tot_frame_rev_cnt=%llu",     fec_status.fec_dump_status.fec_b_err_cnt.tot_frame_rev_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_b_err_cnt.tot_frame_corr_cnt=%llu",    fec_status.fec_dump_status.fec_b_err_cnt.tot_frame_corr_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_b_err_cnt.tot_frame_uncorr_cnt=%llu",  fec_status.fec_dump_status.fec_b_err_cnt.tot_frame_uncorr_cnt);
        SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_b_err_cnt.tot_symbols_corr_cnt =%llu", fec_status.fec_dump_status.fec_b_err_cnt.tot_symbols_corr_cnt);
        for (index = 0; index < 2; index++) {
            SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_b_err_cnt.tot_bits_corr_cnt[%d]=%llu", index, fec_status.fec_dump_status.fec_b_err_cnt.tot_bits_corr_cnt[index]);
        }
        for (index = 0; index < 16; index++) {
            SAP_BCM_LOG_INFO("fec_status.fec_dump_status.fec_b_err_cnt.tot_frames_err_cnt[%d]=%llu", index, fec_status.fec_dump_status.fec_b_err_cnt.tot_frames_err_cnt[index]);
        }
    }
    return SAP_STATUS_SUCCESS;
}

static const char *bcm_plp_mib_stat_str[] = {
        "BCM_PLP_MIB_STAT_R64",
        "BCM_PLP_MIB_STAT_R127",
        "BCM_PLP_MIB_STAT_R255",
        "BCM_PLP_MIB_STAT_R511",
        "BCM_PLP_MIB_STAT_R1023",
        "BCM_PLP_MIB_STAT_R1518",
        "BCM_PLP_MIB_STAT_RMGV",
        "BCM_PLP_MIB_STAT_R2047",
        "BCM_PLP_MIB_STAT_R4095",
        "BCM_PLP_MIB_STAT_R9216",
        "BCM_PLP_MIB_STAT_R16383",
        "BCM_PLP_MIB_STAT_RBCA",
        "BCM_PLP_MIB_STAT_RPROG0",
        "BCM_PLP_MIB_STAT_RPROG1",
        "BCM_PLP_MIB_STAT_RPROG2",
        "BCM_PLP_MIB_STAT_RPROG3",
        "BCM_PLP_MIB_STAT_RPKT",
        "BCM_PLP_MIB_STAT_RPOK",
        "BCM_PLP_MIB_STAT_RUCA",
        "BCM_PLP_MIB_STAT_RESERVED0",
        "BCM_PLP_MIB_STAT_RMCA",
        "BCM_PLP_MIB_STAT_RXPF",
        "BCM_PLP_MIB_STAT_RXPP",
        "BCM_PLP_MIB_STAT_RXCF",
        "BCM_PLP_MIB_STAT_RFCS",
        "BCM_PLP_MIB_STAT_RERPKT",
        "BCM_PLP_MIB_STAT_RFLR",
        "BCM_PLP_MIB_STAT_RJBR",
        "BCM_PLP_MIB_STAT_RMTUE",
        "BCM_PLP_MIB_STAT_ROVR",
        "BCM_PLP_MIB_STAT_RVLN",
        "BCM_PLP_MIB_STAT_RDVLN",
        "BCM_PLP_MIB_STAT_RXUO",
        "BCM_PLP_MIB_STAT_RXUDA",
        "BCM_PLP_MIB_STAT_RXWSA",
        "BCM_PLP_MIB_STAT_RPRM",
        "BCM_PLP_MIB_STAT_RPFC0",
        "BCM_PLP_MIB_STAT_RPFCOFF0",
        "BCM_PLP_MIB_STAT_RPFC1",
        "BCM_PLP_MIB_STAT_RPFCOFF1",
        "BCM_PLP_MIB_STAT_RPFC2",
        "BCM_PLP_MIB_STAT_RPFCOFF2",
        "BCM_PLP_MIB_STAT_RPFC3",
        "BCM_PLP_MIB_STAT_RPFCOFF3",
        "BCM_PLP_MIB_STAT_RPFC4",
        "BCM_PLP_MIB_STAT_RPFCOFF4",
        "BCM_PLP_MIB_STAT_RPFC5",
        "BCM_PLP_MIB_STAT_RPFCOFF5",
        "BCM_PLP_MIB_STAT_RPFC6",
        "BCM_PLP_MIB_STAT_RPFCOFF6",
        "BCM_PLP_MIB_STAT_RPFC7",
        "BCM_PLP_MIB_STAT_RPFCOFF7",
        "BCM_PLP_MIB_STAT_RUND",
        "BCM_PLP_MIB_STAT_RFRG",
        "BCM_PLP_MIB_STAT_RRPKT",
        "BCM_PLP_MIB_STAT_RESERVED1",
        "BCM_PLP_MIB_STAT_T64",
        "BCM_PLP_MIB_STAT_T127",
        "BCM_PLP_MIB_STAT_T255",
        "BCM_PLP_MIB_STAT_T511",
        "BCM_PLP_MIB_STAT_T1023",
        "BCM_PLP_MIB_STAT_T1518",
        "BCM_PLP_MIB_STAT_TMGV",
        "BCM_PLP_MIB_STAT_T2047",
        "BCM_PLP_MIB_STAT_T4095",
        "BCM_PLP_MIB_STAT_T9216",
        "BCM_PLP_MIB_STAT_T16383",
        "BCM_PLP_MIB_STAT_TBC",
        "BCM_PLP_MIB_STAT_TPFC0",
        "BCM_PLP_MIB_STAT_TPFCOFF0",
        "BCM_PLP_MIB_STAT_TPFC1",
        "BCM_PLP_MIB_STAT_TPFCOFF1",
        "BCM_PLP_MIB_STAT_TPFC2",
        "BCM_PLP_MIB_STAT_TPFCOFF2",
        "BCM_PLP_MIB_STAT_TPFC3",
        "BCM_PLP_MIB_STAT_TPFCOFF3",
        "BCM_PLP_MIB_STAT_TPFC4",
        "BCM_PLP_MIB_STAT_TPFCOFF4",
        "BCM_PLP_MIB_STAT_TPFC5",
        "BCM_PLP_MIB_STAT_TPFCOFF5",
        "BCM_PLP_MIB_STAT_TPFC6",
        "BCM_PLP_MIB_STAT_TPFCOFF6",
        "BCM_PLP_MIB_STAT_TPFC7",
        "BCM_PLP_MIB_STAT_TPFCOFF7",
        "BCM_PLP_MIB_STAT_TPKT",
        "BCM_PLP_MIB_STAT_TPOK",
        "BCM_PLP_MIB_STAT_TUCA",
        "BCM_PLP_MIB_STAT_TUF",
        "BCM_PLP_MIB_STAT_TMCA",
        "BCM_PLP_MIB_STAT_TXPF",
        "BCM_PLP_MIB_STAT_TXPP",
        "BCM_PLP_MIB_STAT_TXCF",
        "BCM_PLP_MIB_STAT_TFCS",
        "BCM_PLP_MIB_STAT_TERR",
        "BCM_PLP_MIB_STAT_TOVR",
        "BCM_PLP_MIB_STAT_TJBR",
        "BCM_PLP_MIB_STAT_TRPKT",
        "BCM_PLP_MIB_STAT_TFRG",
        "BCM_PLP_MIB_STAT_TVLN",
        "BCM_PLP_MIB_STAT_TDVLN",
        "BCM_PLP_MIB_STAT_RBYT",
        "BCM_PLP_MIB_STAT_RRBYT",
        "BCM_PLP_MIB_STAT_TBYT",
    };

static int sap_bcm_mib_dump(int unit, int port, int if_side)
{
    int rv = 0;
    plp_uint64_t count;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t phy_access;
    bcm_plp_mac_access_t mac_access;
    bcm_plp_mib_stat_type_t stat_type;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    memset(&mac_access, 0, sizeof(bcm_plp_mac_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    memcpy(&mac_access.phy_info, &phy_access, sizeof(bcm_plp_access_t));
    for (stat_type = BCM_PLP_MIB_STAT_R64; stat_type <= BCM_PLP_MIB_STAT_TBYT; stat_type++) {
        count = 0;
        rv = bcm_plp_mac_mib_stat_get(phy_info->chip_name, mac_access, stat_type, &count);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_mac_mib_stat_get for stat: %s at %s side failed with rv:%d", 
                        unit, port, bcm_plp_mib_stat_str[stat_type], BCM_IF_SIDE_STR(if_side), rv);
        } else {
            SAP_CLI_LOG("unit[%d] port[%d] side[%s], %s: %llu", unit, port, BCM_IF_SIDE_STR(if_side), bcm_plp_mib_stat_str[stat_type], count);
            // SAP_BCM_LOG_INFO("unit[%d] port[%d] side[%s], %s: %llu", unit, port, BCM_IF_SIDE_STR(if_side), bcm_plp_mib_stat_str[stat_type], count);
        }
    }

    // int threshold, mtu, enable;
    // rv = bcm_plp_mac_runt_threshold_get(phy_info->chip_name, mac_access, &threshold);
    // printf("unit[%d] port[%d], threshold_get:%d, rv:%d\n", unit, port, threshold, rv);

    // threshold = 1;
    // rv = bcm_plp_mac_runt_threshold_set(phy_info->chip_name, mac_access, threshold);
    // printf("unit[%d] port[%d], threshold_set:%d, rv:%d\n", unit, port, threshold, rv);

    // rv = bcm_plp_mac_runt_threshold_get(phy_info->chip_name, mac_access, &threshold);
    // printf("unit[%d] port[%d], threshold_get:%d, rv:%d\n", unit, port, threshold, rv);

    // rv = bcm_plp_mac_max_packet_size_get(phy_info->chip_name, mac_access, &mtu);
    // printf("unit[%d] port[%d], max_packet_size:%d, rv:%d\n", unit, port, mtu, rv);

    // rv = bcm_plp_mac_store_and_forward_mode_get(phy_info->chip_name, mac_access, &enable);
    // printf("unit[%d] port[%d], sf_enable_get:%d, rv:%d\n", unit, port, enable, rv);

    // enable = 1;
    // rv = bcm_plp_mac_store_and_forward_mode_set(phy_info->chip_name, mac_access, enable);
    // printf("unit[%d] port[%d], sf_enable_set:%d, rv:%d\n", unit, port, enable, rv);

    // rv = bcm_plp_mac_store_and_forward_mode_get(phy_info->chip_name, mac_access, &enable);
    // printf("unit[%d] port[%d], sf_enable_get:%d, rv:%d\n", unit, port, enable, rv);

    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_reg_set(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t data)
{
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    uint32_t lane_i, lane_map, lane_num;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    if (flag == 64) {
        rv = bcm_plp_reg64_value_set(phy_info->chip_name, phy_access, devaddr, regaddr, data);
    } else if (flag == 0 || flag == 32) {
        rv = bcm_plp_reg_value_set(phy_info->chip_name, phy_access, devaddr, regaddr, data);
    }
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_reg_value_set failed with rv:%d", unit, port, rv);
    } else {
        SAP_BCM_LOG_DBG("phy_addr=0x%-3x lane_map=0x%-8x unit=%-2d port=%-3d data=0x%llx", phy_access.phy_addr, phy_access.lane_map, unit, port, data);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_reg_get(int unit, int port, int if_side, int devaddr, int regaddr, int flag, uint64_t *data)
{
    int rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    uint32_t lane_i, lane_map, lane_num;
    short unsigned int phy_rd_data;
    uint32_t phy_reg_addr;
    uint32_t phy_id = 0;
    
    if (data == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    if (flag == 64) {
        long long unsigned int tmp_data = 0;
        rv = bcm_plp_reg64_value_get(phy_info->chip_name, phy_access, devaddr, regaddr, &tmp_data);
        *data = (uint64_t)tmp_data;
    } else if (flag == 0 || flag == 32) {
        uint32_t data32 = 0;
        rv = bcm_plp_reg_value_get(phy_info->chip_name, phy_access, devaddr, regaddr, &data32);
        *data = (uint64_t)data32;
    }
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_reg_value_get failed with rv:%d, devaddr[0x%x] regaddr[0x%x]", unit, port, rv, devaddr, regaddr);
    } else {
        SAP_BCM_LOG_DBG("phy_addr=0x%-3x lane_map=0x%-8x unit=%-2d port=%-3d, data=0x%lx", phy_access.phy_addr, phy_access.lane_map, unit, port, *data);
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_autoneg_set(int unit, int port, int if_side, int enable)
{
    int rv = 0;
    uint16_t tech_ability, fec_ability;
    int speed = 0, if_type = 0, ref_clk = 0, if_mode = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;
    bcm_plp_an_config_t an_config;
    bcm_plp_barchetta_device_aux_modes_t aux_mode;

    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    if (enable == true) {
        memset(&aux_mode, 0, sizeof(bcm_plp_barchetta_device_aux_modes_t));
        rv = bcm_plp_mode_config_get(phy_info->chip_name, phy_access, &speed, &if_type, &ref_clk,
            &if_mode, (void*)&aux_mode);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_mode_config_get failed with rv:%d", unit, port, rv);
            // return SAP_STATUS_FAILURE;
            return SAP_STATUS_SUCCESS;
        }
        memset(&an_config, 0, sizeof(bcm_plp_an_config_t));
        if (speed == SPEED_10G) {
            an_config.tech_ability = bcmplpAnCap10G_KR;
        } else if (speed == SPEED_25G) {
            an_config.tech_ability = bcmplpAnCap25G_KR;
        } else if (speed == SPEED_40G) {
            an_config.tech_ability = bcmplpAnCap40G_CR4;
        } else if (speed == SPEED_100G) {
            an_config.tech_ability = bcmplpAnCap100G_CR4;
        } else if (speed == SPEED_400G) {
            an_config.tech_ability = bcmplpAnCap400G_CR8_KR8;
        } else {
            SAP_BCM_LOG_ERR("speed unkown, speed:%d", speed);
            // return SAP_STATUS_FAILURE;
            return SAP_STATUS_SUCCESS;
        }
        tech_ability = an_config.tech_ability & 0xffff;
        fec_ability = 0;
        SAP_BCM_LOG_DBG("tech_ability:%d", an_config.tech_ability);
        an_config.cl72_en = true;
        rv = bcm_plp_cl73_ability_set(phy_info->chip_name, phy_access, tech_ability, fec_ability, 0, an_config);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_cl73_ability_set failed with rv:%d", unit, port, rv);
            // return SAP_STATUS_FAILURE;
            return SAP_STATUS_SUCCESS;
        }
    }
    phy_access.flags = BCM_PLP_AN_MODE_CL37;
    rv = bcm_plp_cl73_set(phy_info->chip_name,phy_access, enable);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_cl73_set failed with rv:%d", unit, port, rv);
        // return SAP_STATUS_FAILURE;
        return SAP_STATUS_SUCCESS;
    }
    //TODO: set port speed or not
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_autoneg_status_get(int unit, int port, int if_side, int *enable)
{
    int rv = 0;
    uint32_t enable_get = 0, an_done_get = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;

    if (enable == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = bcm_plp_cl73_get(phy_info->chip_name, phy_access, &enable_get, &an_done_get);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_cl73_get failed with rv:%d", unit, port, rv);
        return rv;
    }
    *enable = enable_get;
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_phy_status_dump(int unit, int port, int level)
{
    int rv = 0;
    int flags = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t  phy_access;

    if (level == 0) {
        flags = BCM_PLP_INTERNAL_CHIP_DUMP;
    } else if (level == 1) {
        flags = BCM_PLP_INTERNAL_DUMP_L1;
    } else if (level == 2) {
        flags = BCM_PLP_INTERNAL_DUMP_L2;
    } else if (level > 2) {
        flags = BCM_PLP_INTERNAL_DUMP_L3;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    /* sys side dump */
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, PLP_SYS_IF_SIDE, &phy_access);
    phy_access.flags = flags;
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = bcm_plp_phy_status_dump(phy_info->chip_name, phy_access);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_phy_status_dump failed with rv:%d", unit, port, rv);
        return SAP_STATUS_FAILURE;
    }
    /* line side dump */
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, PLP_LINE_IF_SIDE, &phy_access);
    phy_access.flags = flags;
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = bcm_plp_phy_status_dump(phy_info->chip_name, phy_access);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_phy_status_dump failed with rv:%d", unit, port, rv);
        return SAP_STATUS_FAILURE;
    }
    return SAP_STATUS_SUCCESS;
}
/*! \brief POLARITY configuration set.
 *
 *	This API retives the configured TX/RX PRBS. 
 *  @param polarity        极性配置，若端口lane不连续，极性要和lane对齐
 *  @param tx_rx           0: means tx, 1: means rx \n
 *  @param override        表示是直接覆盖原本的极性，还是将原本的极性进行翻转 \n
 *
 *	@return SAP_STATUS_SUCCESS(0) for success and corresponding error code on failure
 */
static int sap_bcm_polarity_set(int unit, int port, int if_side, int tx_rx, uint32_t polarity, bool override)
{
    int rv = 0;
    int lane_idx;
    int offset = -1;
    int mask;
    uint32_t read_tx_pol, read_rx_pol;
    sap_bcm_phy_info_t *phy_info;
    sap_bcm_port_info_t *port_info;
    bcm_plp_access_t  phy_access;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed!", unit, port);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_port_info_get(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_port_info_get failed!", unit, port);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed!", unit, port);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    if (if_side == PLP_SYS_IF_SIDE) {
        mask = port_info->lanemap_sys;
    } else {
        mask = port_info->lanemap_line;
    }
    /* 对齐首位 */
    for(lane_idx=0; lane_idx<BCM_PHY_MAX_PORT_NUM; lane_idx++) {
        if (((mask >> lane_idx) & 0x1) == 1) {
            offset = lane_idx;
            break;
        }
    }
    if (offset == -1) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_bcm_polarity_set lane offset error!", unit, port);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    polarity = polarity << offset;
    rv = bcm_plp_polarity_get(phy_info->chip_name, phy_access, &read_tx_pol, &read_rx_pol);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_polarity_get failed with rv:%d", unit, port, rv);
        return rv;
    }
    SAP_BCM_LOG_DBG("PASS: Successfully get the polarity (tx_pol: 0x%x, rx_pol: 0x%x) for lane-map 0x%x of PHY-0x%x at %s side!",
        read_tx_pol, read_rx_pol, phy_access.lane_map, phy_access.phy_addr, BCM_IF_SIDE_STR(if_side));
    /* tx=1 rx=0 */
    if (tx_rx == PLP_TX_DIR) {
        if (override == true) {
            read_tx_pol = polarity;
        } else {
            read_tx_pol = read_tx_pol ^ polarity;
        }
    } else {
        if (override == true) {
            read_rx_pol = polarity;
        } else {
            read_rx_pol = read_rx_pol ^ polarity;
        }
    }
    rv = bcm_plp_polarity_set(phy_info->chip_name, phy_access, read_tx_pol, read_rx_pol);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_polarity_set failed with rv:%d", unit, port, rv);
        return rv;
    } else {
        SAP_BCM_LOG_DBG("PASS: Successfully set the polarity (tx_pol: 0x%x, rx_pol: 0x%x) for lane-map 0x%x of PHY-0x%x at %s side!",
        read_tx_pol, read_rx_pol, phy_access.lane_map, phy_access.phy_addr, BCM_IF_SIDE_STR(if_side));
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_polarity_get(int unit, int port, int if_side, int tx_rx, uint32_t *polarity)
{
    int rv = 0;
    int mask;
    int lane_idx;
    int offset = -1;
    uint32_t read_tx_pol, read_rx_pol;
    sap_bcm_phy_info_t *phy_info;
    sap_bcm_port_info_t *port_info;
    bcm_plp_access_t phy_access;
    if (polarity == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_port_info_get(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_port_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    if (if_side == PLP_SYS_IF_SIDE) {
        mask = port_info->lanemap_sys;
    } else {
        mask = port_info->lanemap_line;
    }
    /* 对齐首位 */
    for(lane_idx=0; lane_idx<BCM_PHY_MAX_PORT_NUM; lane_idx++) {
        if (((mask >> lane_idx) & 0x1) == 1) {
            offset = lane_idx;
            break;
        }
    }
    if (offset == -1) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_polarity_get lane offset error!", unit, port);
        return SAP_STATUS_FAILURE;
    }
    rv = bcm_plp_polarity_get(phy_info->chip_name, phy_access, &read_tx_pol, &read_rx_pol);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_polarity_get failed with rv:%d", unit, port, rv);
        return rv;
    }
    if (tx_rx == PLP_TX_DIR) {
        *polarity = get_bits(read_tx_pol, mask) >> offset;
    } else {
        *polarity = get_bits(read_rx_pol, mask) >> offset;
    }
    return SAP_STATUS_SUCCESS;
}

/*! \brief Chip Reset
 *
 *	This API is used to do Hard/Soft reset on the specified PHY.
 *  Hard reset erases firmware along with the contents of PHY registers.
 *  Soft reset preserves firmware and erases the contents of PHY registers.
 *
 *  @param phy_info  <pre> Represents PHY access\n
 *  @param reset_mode      Reset modes\n
 *                         0 - Hard reset (not supported by all PHYs)\n
 *                         1 - Soft reset 
 *  @param reset_direction Reserved for future use </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
static int sap_bcm_phy_reset(int unit, int port, int flags)
{
    int rv;
    bcm_plp_access_t phy_access;
    sap_bcm_phy_info_t *phy_info;
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, PLP_SYS_IF_SIDE, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    if (flags & 0x1) {
        rv = bcm_plp_reset_set(phy_info->chip_name, phy_access, 1, 0);
    } else {
        rv = bcm_plp_reset_set(phy_info->chip_name, phy_access, 0, 0);
    }
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_reset_set failed with rv:%d", unit, port, rv);
        return SAP_STATUS_FAILURE;
    }
    rv = sap_barchetta_clean_up(phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_barchetta_clean_up failed with rv:%d", unit, port, rv);
    }
    phy_info->loaded = 0;
    return rv;
}

static int sap_bcm_admin_set(int unit, int port, int if_side, int enable)
{
    int rv = 0;
    uint32_t en_value;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t phy_access;
    bcm_plp_mac_access_t mac_access;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    memset(&mac_access, 0, sizeof(bcm_plp_mac_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    if (enable == PLP_PORT_ENABLE) {
        en_value = 1;
    } else {
        en_value = 0;
    }
    memcpy(&mac_access.phy_info, &phy_access, sizeof(bcm_plp_access_t));
    /* 0 == tx */
    rv = bcm_plp_port_enable_set(phy_info->chip_name, mac_access, 0, en_value);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_port_enable_set for tx at %s side failed with rv:%d", 
                    unit, port, BCM_IF_SIDE_STR(if_side), rv);
        return rv;
    }
    /* 1 == rx */
    rv = bcm_plp_port_enable_set(phy_info->chip_name, mac_access, 1, en_value);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_port_enable_set for rx at %s side failed with rv:%d", 
                    unit, port, BCM_IF_SIDE_STR(if_side), rv);
        return rv;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_admin_get(int unit, int port, int if_side, int *enable)
{
    int rv = 0;
    int en_value = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t phy_access;
    bcm_plp_mac_access_t mac_access;
    if (enable == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    memset(&mac_access, 0, sizeof(bcm_plp_mac_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    memcpy(&mac_access.phy_info, &phy_access, sizeof(bcm_plp_access_t));
    rv = bcm_plp_port_enable_get(phy_info->chip_name, mac_access, 0, &en_value);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_port_enable_get failed with rv:%d", unit, port, rv);
        return rv;
    }
    if (en_value == 1) {
        *enable = PLP_PORT_ENABLE;
    } else {
        *enable = PLP_PORT_DISABLE;
    }
    rv = bcm_plp_port_enable_get(phy_info->chip_name, mac_access, 1, &en_value);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_port_enable_get failed with rv:%d", unit, port, rv);
        return rv;
    }
    if (en_value == 1) {
        *enable &= PLP_PORT_ENABLE;
    } else {
        *enable &= PLP_PORT_DISABLE;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_mutex_take(uint32_t phy_id, void* platform_ctxt)
{
    int i, rv;
    if (platform_ctxt == NULL) {
        return SAP_STATUS_FAILURE;
    }
    sap_bcm_phy_info_t *phy_info = (sap_bcm_phy_info_t *)platform_ctxt;
    // SAP_BCM_LOG_DBG("phy_id: 0x%x, sap_bcm_mutex_take", phy_id);
    pthread_mutex_lock(&phy_info->mutex);
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_mutex_give(uint32_t phy_id, void* platform_ctxt)
{
    int i, rv;
    if (platform_ctxt == NULL) {
        return SAP_STATUS_FAILURE;
    }
    sap_bcm_phy_info_t *phy_info = (sap_bcm_phy_info_t *)platform_ctxt;
    // SAP_BCM_LOG_DBG("phy_id: 0x%x, sap_bcm_mutex_give", phy_id);
    pthread_mutex_unlock(&phy_info->mutex);
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_channel_reach_set(int unit, int port, int if_side, int lane, int nr_er)
{
    int i, rv = 0;
    int en_value;
    int lane_map, lane_num;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t phy_access;
    bcm_plp_pm_firmware_lane_config_t lane_config;
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, if_side, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    lane_map = phy_access.lane_map;
    GET_PORT_LANE_NUM(lane_map, lane_num);
    for (i=1; i <= lane_num; i++) {
        if ((lane != 0) && (lane != i)) {
            continue;
        }
        GET_PORT_SINGLE_LANE_MAP(lane_map, i, phy_access.lane_map);
        memset(&lane_config, 0, sizeof(bcm_plp_pm_firmware_lane_config_t));
        rv = bcm_plp_firmware_lane_config_get(phy_info->chip_name, phy_access, &lane_config);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_firmware_lane_config_get failed with rv:%d", unit, port, rv);
            return rv;
        }
        if (nr_er == 0) {
            lane_config.ForceNS = 1;
            lane_config.ForceES = 0;
        } else if (nr_er == 1) {
            lane_config.ForceNS = 0;
            lane_config.ForceES = 1;
        }
        rv = bcm_plp_firmware_lane_config_set(phy_info->chip_name, phy_access, &lane_config);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_firmware_lane_config_set failed with rv:%d", unit, port, rv);
            return rv;
        }
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_temperature_get(int unit, int port, double *temp)
{
    int i, rv = 0;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t phy_access;
    bcm_plp_core_diagnostics_t core_diag;
    if (temp == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, PLP_SYS_IF_SIDE, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = bcm_plp_core_diagnostics_get(phy_info->chip_name, phy_access, &core_diag);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], bcm_plp_core_diagnostics_get failed with rv:%d", unit, port, rv);
        return rv;
    }
    *temp = core_diag.temperature;
    return SAP_STATUS_SUCCESS; 
}

static int sap_bcm_highest_temperature_get(double *temp)
{
    int i, rv;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t phy_access;
    bcm_plp_core_diagnostics_t core_diag;
    if (temp == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    *temp = 0;
    for (i = 0; i < g_phy_num; i++) {
        phy_info = &g_bcm_phy_infos[i];
        if (phy_info->loaded == 0) {
            continue;
        }
        phy_access.platform_ctxt = phy_info;
        phy_access.phy_addr = phy_info->phy_addr;
        phy_access.lane_map = 0xffff;
        rv = bcm_plp_core_diagnostics_get(phy_info->chip_name, phy_access, &core_diag);
        if (rv != 0) {
            SAP_BCM_LOG_ERR("PHY-0x%x, bcm_plp_core_diagnostics_get failed with rv:%d", phy_info->phy_addr, rv);
            continue;
        }
        if (core_diag.temperature > *temp) {
            *temp = core_diag.temperature;
        }
    }
    return SAP_STATUS_SUCCESS; 
}

static int sap_bcm_version_get(int unit, int port, sap_version_t *version_info)
{
    int rv;
    uint32_t fw_ver, fw_crc, rev_id;
    unsigned short  api_ver = 0, en_ver = 0;
    bcm_plp_access_t phy_access;
    sap_bcm_phy_info_t *phy_info;
    char chip_name[16];
    if (version_info == NULL) {
        return SAP_STATUS_INVALID_PARAMETER;
    }
    memset(version_info, 0, sizeof(bcm_plp_access_t));
    memset(&phy_access, 0, sizeof(bcm_plp_access_t));
    rv = sap_phy_access_get(unit, port, PLP_SYS_IF_SIDE, &phy_access);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_access_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }
    version_info->phy_id = phy_info->phy_addr;
    rv = bcm_plp_firmware_info_get(phy_info->chip_name, phy_access, &fw_ver, &fw_crc);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_firmware_info_get failed rv: %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x FW version 0x%x, FW CRC 0x%x", phy_info->phy_addr, fw_ver, fw_crc);
        version_info->fw_ver = fw_ver;
        version_info->fw_crc = fw_crc;
    }
    rv = bcm_plp_rev_id(phy_info->chip_name, phy_access, &rev_id);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_rev_id failed rv: %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x rev_id 0x%x", phy_info->phy_addr, rev_id);
        version_info->rev_id = rev_id;
    }
    rv = bcm_plp_driver_version_get(phy_info->chip_name, phy_access, chip_name, &api_ver, &en_ver);
    if (rv != 0) {
        SAP_BCM_LOG_ERR("PHY-0x%x bcm_plp_driver_version_get failed rv: %d", phy_info->phy_addr, rv);
        return SAP_STATUS_FAILURE;
    } else {
        SAP_BCM_LOG_INFO("PHY-0x%x chip_name: %s, majar_ver:%d, minor_ver:%d", phy_info->phy_addr, chip_name, api_ver, en_ver);
        snprintf(version_info->chip_name, sizeof(version_info->chip_name), "%s", chip_name);
        version_info->drv_major_ver = api_ver;
        version_info->drv_minor_ver = en_ver;
    }
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_eeprom_update(int unit, int port, bool use_file)
{
    int rv;
    sap_bcm_phy_info_t *phy_info;

    rv = sap_phy_info_get(unit, port, &phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_phy_info_get failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }

    if (use_file == true) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], use_file func not implemented", unit, port);
        return SAP_STATUS_NOT_IMPLEMENTED;
    }

    rv = sap_barchetta_fw_repair(phy_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("unit[%d] port[%d], sap_barchetta_fw_repair failed with rv:%d", unit, port, rv);
        return SAP_STATUS_ITEM_NOT_FOUND;
    }

    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_port_list_get(sap_port_list_t *port_list)
{
    port_list->list = &global_port_list_head;
    port_list->port_count = global_port_num;
    return SAP_STATUS_SUCCESS;
}

static int sap_bcm_port_status_get(int unit, int port, sap_port_status_t *port_status)
{
    int i, rv;
    int phy_id, side;
    char dump_string[MAX_LINE_STRING_LEN];
    sap_bcm_port_info_t *port_info;
    sap_bcm_phy_info_t *phy_info;
    bcm_plp_access_t phy_access;
    uint32_t link_status_get;
    SAP_POINT_CHECK_RV(port_status, "port_status", SAP_STATUS_INVALID_PARAMETER);

    rv = sap_port_info_get(unit, port, &port_info);
    if (rv != SAP_STATUS_SUCCESS) {
        SAP_BCM_LOG_ERR("sap_port_info_get failed, unit:%d, port:%d", unit, port);
        return rv;
    }
    phy_info = port_info->phy_info;

    port_status->speed = port_info->speed;
    port_status->fec_sys = port_info->fec_sys;
    port_status->fec_line = port_info->fec_line;
    port_status->host_admin = port_info->inited;
    port_status->line_admin = port_info->inited;

    memset(&phy_access, 0, sizeof(phy_access));
    side = BCM_SYSTEM_SIDE;
    SAP_RV_CHECK(phy_info, "sap_phy_access_get", sap_phy_access_get(unit, port, side, &phy_access));
    rv = bcm_plp_link_status_get(phy_info->chip_name, phy_access, &link_status_get);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("FAIL: Failed to get link status for lane-map 0x%x of PHY-0x%x at %s side (ret = %d)!\n",
        phy_access.lane_map, phy_access.phy_addr, (side == BCM_SYSTEM_SIDE) ? "System" : "Line", rv);
    } else {
        port_status->host_link_up = link_status_get;
    }
    
    memset(&phy_access, 0, sizeof(phy_access));
    side = BCM_LINE_SIDE;
    SAP_RV_CHECK(phy_info, "sap_phy_access_get", sap_phy_access_get(unit, port, side, &phy_access));
    rv = bcm_plp_link_status_get(phy_info->chip_name, phy_access, &link_status_get);
    if (rv != BCM_PM_IF_SUCCESS) {
        SAP_BCM_LOG_ERR("FAIL: Failed to get link status for lane-map 0x%x of PHY-0x%x at %s side (ret = %d)!\n",
        phy_access.lane_map, phy_access.phy_addr, (side == BCM_SYSTEM_SIDE) ? "System" : "Line", rv);
    } else {
        port_status->line_link_up = link_status_get;
    }

    return SAP_STATUS_SUCCESS;
}
