/*
 *
 * $Id: phymod.xml,v 1.1.2.5 Broadcom SDK $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */

#include <phymod/phymod.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_dispatch.h>
#include <bcmi_aperta_tscbh_xgxs_defs.h>
#include <aperta_tscbh.h>
#include <aperta_tscbh/tier1/aperta_tbhmod.h>
#include <aperta_tscbh/tier1/aperta_tbhmod_sc_lkup_table.h>
#include <aperta_tscbh/tier1/tbhPCSRegEnums.h>
#include "blackhawk/tier1/blackhawk_cfg_seq.h"
#include "blackhawk/tier1/blackhawk_tsc_enum.h"
#include "blackhawk/tier1/blackhawk_tsc_common.h"
#include "blackhawk/tier1/blackhawk_tsc_interface.h"
#include "blackhawk/tier1/blackhawk_tsc_dependencies.h"
#include "blackhawk/tier1/blackhawk_tsc_internal.h"
#include "blackhawk/tier1/blackhawk_tsc_functions.h"
#include "blackhawk/tier1/public/blackhawk_api_uc_vars_rdwr_defns_public.h"
#include "blackhawk/tier1/blackhawk_tsc_access.h"
#include <tier1/aperta_reg_access.h>
#include <tier1/aperta_cfg_seq.h>
#include <aperta_tscbh/tier1/aperta_tbhmod_1588_lkup_table.h>
#include <bcmi_aperta_tscbh_xgxs_defs.h>




extern unsigned char plp_aperta_blackhawk_ucode[];
extern unsigned int plp_aperta_blackhawk_ucode_len;
extern unsigned short plp_aperta_blackhawk_ucode_crc;
extern unsigned short plp_aperta_blackhawk_ucode_stack_size;

extern uint32_t plp_aperta_spd_id_entry_26[APERTA_TSCBH_SPEED_ID_TABLE_SIZE][APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_spd_id_entry_25[APERTA_TSCBH_SPEED_ID_TABLE_SIZE][APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_spd_id_entry_20[APERTA_TSCBH_SPEED_ID_TABLE_SIZE][APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_am_table_entry[APERTA_TSCBH_AM_TABLE_SIZE][APERTA_TSCBH_AM_ENTRY_SIZE];
extern uint32_t plp_aperta_um_table_entry[APERTA_TSCBH_UM_TABLE_SIZE][APERTA_TSCBH_UM_ENTRY_SIZE];
extern uint32_t plp_aperta_speed_priority_mapping_table[APERTA_TSCBH_SPEED_PRIORITY_MAPPING_TABLE_SIZE][APERTA_TSCBH_SPEED_PRIORITY_MAPPING_ENTRY_SIZE];

extern const aperta_ts_table_entry plp_aperta_ts_table_rx_sop[APERTA_TBHMOD_SPEED_MODE_COUNT];
extern const aperta_ts_table_entry plp_aperta_ts_table_tx_sop[APERTA_TBHMOD_SPEED_MODE_COUNT];



#define APERTA_TSCBH_HW_SPEED_ID_TABLE_SIZE   64
#define APERTA_TSCBH_HW_AM_TABLE_SIZE    64
#define APERTA_TSCBH_HW_UM_TABLE_SIZE    64

#define REF_CLOCK_312P5           312500000
#define APERTA_TSCBH_ID0                 0x600d
#define APERTA_TSCBH_ID1                 0x8770
#define APERTA_TSCBH_SERDES_ID           0x25 /* 0x9008 Main0_serdesID - Serdes ID Register */
#define APERTA_TSCBH_MODEL               0x26
#define APERTA_TSCBH_PHY_ALL_LANES       0xff
#define APERTA_TSCBH_TX_TAP_NUM          12
#define APERTA_TSCBH_FORCED_SPEED_ID_OFFSET 56

#define APERTA_TSCBH_CORE_TO_PHY_ACCESS(_phy_access, _core_access) \
    do{\
        PHYMOD_MEMCPY(&(_phy_access)->access, &(_core_access)->access, sizeof((_phy_access)->access));\
        (_phy_access)->type           = (_core_access)->type; \
        (_phy_access)->port_loc       = (_core_access)->port_loc; \
        (_phy_access)->device_op_mode = (_core_access)->device_op_mode; \
        (_phy_access)->access.lane_mask = APERTA_TSCBH_PHY_ALL_LANES; \
    }while(0)

#define APERTA_TSCBH_VCO_NONE 0x0
#define APERTA_TSCBH_VCO_20G 0x1
#define APERTA_TSCBH_VCO_25G 0x2
#define APERTA_TSCBH_VCO_26G 0x4
#define APERTA_SPEED_ID_INDEX_100G_4_LANE_CL73_KR4     0x6
#define APERTA_SPEED_ID_INDEX_100G_4_LANE_CL73_CR4     0x7
#define APERTA_TSCBH_MAX_NUM_AM_RETRY          3


int plp_aperta_tscbh_core_identify(const plp_aperta_phymod_core_access_t* core, uint32_t core_id, uint32_t* is_identified)
{
    PHYID2r_t id2;
    PHYID3r_t id3;
    MAIN0_SERDESIDr_t serdesid;
    int ioerr = 0;
    plp_aperta_phymod_phy_access_t phy;

    PHYMOD_MEMCPY(&phy, core, sizeof(phy));

    *is_identified = 0;
    ioerr += READ_PHYID2r(&phy, &id2);
    ioerr += READ_PHYID3r(&phy, &id3);

    if (PHYID2r_REGID1f_GET(id2) == APERTA_TSCBH_ID0 &&
       (PHYID3r_REGID2f_GET(id3) == APERTA_TSCBH_ID1)) {
        /* PHY IDs match - now check PCS model */
        ioerr += READ_MAIN0_SERDESIDr(&phy, &serdesid);
        if ( (MAIN0_SERDESIDr_MODEL_NUMBERf_GET(serdesid)) == APERTA_TSCBH_SERDES_ID)  {
            *is_identified = 1;
        }
    }
    return ioerr ? PHYMOD_E_IO : PHYMOD_E_NONE;
}


int plp_aperta_tscbh_core_info_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_core_info_t* info)
{
    int rv = 0;
    MAIN0_SERDESIDr_t serdes_id;
    PHYID2r_t id2;
    PHYID3r_t id3;
    plp_aperta_phymod_phy_access_t phy;

    PHYMOD_MEMCPY(&phy, core, sizeof(phy));

    rv = READ_MAIN0_SERDESIDr(&phy, &serdes_id);

    info->serdes_id = MAIN0_SERDESIDr_GET(serdes_id);
    info->serdes_id = MAIN0_SERDESIDr_GET(serdes_id);

    PHYMOD_IF_ERR_RETURN(READ_PHYID2r(&phy, &id2));
    PHYMOD_IF_ERR_RETURN(READ_PHYID3r(&phy, &id3));

    info->phy_id0 = (uint16_t) id2.v[0];
    info->phy_id1 = (uint16_t) id3.v[0];

    return rv;
}


int plp_aperta_tscbh_core_lane_map_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_lane_map_t* lane_map)
{
    plp_aperta_phymod_phy_access_t phy;
    uint8_t tx_lane_map_physical[8], lane, lane_outer = 0, lane_inner = 0;
    TX_X1_TX_LN_SWPr_t TX_X1_TX_LN_SWPr_reg;
    unsigned int pcs_lane_swap = 0;
 
    PHYMOD_MEMCPY(&phy, core, sizeof(plp_aperta_phymod_phy_access_t));

    /*for the physical 0 through 3 using MPP0 */
    TX_X1_TX_LN_SWPr_CLR(TX_X1_TX_LN_SWPr_reg);
    phy.access.lane_mask = 0x1 << 0;
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_TX_LN_SWPr(&phy, &TX_X1_TX_LN_SWPr_reg));
    tx_lane_map_physical[0] = TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL0_SELf_GET(TX_X1_TX_LN_SWPr_reg);
    tx_lane_map_physical[1] = TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL1_SELf_GET(TX_X1_TX_LN_SWPr_reg);
    tx_lane_map_physical[2] = TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL2_SELf_GET(TX_X1_TX_LN_SWPr_reg);
    tx_lane_map_physical[3] = TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL3_SELf_GET(TX_X1_TX_LN_SWPr_reg);

    /*for the physical 4 through 7 using MPP1 */
    phy.access.lane_mask = 0x1 << 4;
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_TX_LN_SWPr(&phy, &TX_X1_TX_LN_SWPr_reg));
    tx_lane_map_physical[4] = TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL0_SELf_GET(TX_X1_TX_LN_SWPr_reg);
    tx_lane_map_physical[5] = TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL1_SELf_GET(TX_X1_TX_LN_SWPr_reg);
    tx_lane_map_physical[6] = TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL2_SELf_GET(TX_X1_TX_LN_SWPr_reg);
    tx_lane_map_physical[7] = TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL3_SELf_GET(TX_X1_TX_LN_SWPr_reg);

    /* next need to adjust the MPP1 logical lane offset */
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE / 2; lane++){
        if (tx_lane_map_physical[4 + lane] > 4) {
            tx_lane_map_physical[4 + lane] -= 4;
        } else {
            tx_lane_map_physical[4 + lane] += 4;
        }
        tx_lane_map_physical[4 + lane] %= 8;
    }

    /* first need to translate logical lane based on physical lane based lane map */
    for (lane_outer = 0; lane_outer < APERTA_TSCBH_NOF_LANES_IN_CORE; lane_outer++){
        for (lane_inner = 0; lane_inner < APERTA_TSCBH_NOF_LANES_IN_CORE; lane_inner++){
            if (lane_outer == tx_lane_map_physical[lane_inner]) {
                pcs_lane_swap |= lane_inner << (lane_outer*4);
                break;
            }
        }
    }
    lane_map->num_of_lanes = 8;
    for(lane = 0; lane < 8; lane ++) {
        lane_map->lane_map_tx[lane] = (pcs_lane_swap >> (lane * 4)) & 0xF;
        lane_map->lane_map_rx[lane] = (pcs_lane_swap >> (lane * 4)) & 0xF;
    }

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_core_reset_set(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_reset_mode_t reset_mode, plp_aperta_phymod_reset_direction_t direction)
{
    return PHYMOD_E_NONE;

}

int plp_aperta_tscbh_core_reset_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_reset_mode_t reset_mode, plp_aperta_phymod_reset_direction_t* direction)
{
    return PHYMOD_E_NONE;

}

int plp_aperta_tscbh_phy_tx_lane_control_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_tx_lane_control_t tx_control)
{
    phymod_firmware_lane_config_t fw_lane_config;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_IF_ERR_RETURN (plp_aperta_tscbh_phy_firmware_lane_config_get(phy, &fw_lane_config));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    switch (tx_control) {
        case phymodTxTrafficDisable:
            PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_tx_lane_control(&phy_copy, 0, APERTA_TBHMOD_TX_LANE_TRAFFIC_DISABLE));
            break;
        case phymodTxTrafficEnable:
            /* whenever the second inpout which is enable is set, then the thrid parameter is do not care */
            PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_tx_lane_control(&phy_copy, 1, APERTA_TBHMOD_TX_LANE_ILLEGAL));
            break;
        case phymodTxReset:
            PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_tx_lane_control(&phy_copy, 0, APERTA_TBHMOD_TX_LANE_RESET));
            break;
        case phymodTxElectricalIdleEnable:
            if (fw_lane_config.LaneConfigFromPCS == 0) {
                PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_electrical_idle_set(&phy_copy, 1));
            } else {
                return PHYMOD_E_PARAM;
            }
            break;
        case phymodTxElectricalIdleDisable:
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_electrical_idle_set(&phy_copy, 0));
            break;
        case phymodTxSquelchOn:
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_disable(&phy_copy, 1));
            break;
        case phymodTxSquelchOff:
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_disable(&phy_copy, 0));
            break;
        default:
            return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;

}


int plp_aperta_tscbh_phy_tx_lane_control_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_tx_lane_control_t *tx_control)
{
    int reset, tx_lane;
    uint32_t lb_enable;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane;
    uint8_t tx_disable = 0;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    pm_phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_disable_get(&pm_phy_copy, &tx_disable));

    /* next check if PMD loopback is on */
    if (tx_disable) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_dig_lpbk_get(&pm_phy_copy, &lb_enable));
        if (lb_enable) tx_disable = 0;
    }

    if(tx_disable) {
        *tx_control = phymodTxSquelchOn;
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_tx_lane_control_get(&pm_phy_copy, &reset, &tx_lane));
        if (!reset) {
            *tx_control = phymodTxReset;
        } else if (!tx_lane) {
            *tx_control = phymodTxTrafficDisable;
        } else {
            *tx_control = phymodTxSquelchOff;
        }
    }
    return PHYMOD_E_NONE;
}

/*Rx control*/
int plp_aperta_tscbh_phy_rx_lane_control_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_rx_lane_control_t rx_control)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    switch (rx_control) {
        case phymodRxReset:
            PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_rx_lane_control(&pm_phy_copy, 0));
            PHYMOD_USLEEP(500);
            PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_rx_lane_control(&pm_phy_copy, 1));
            break;
        case phymodRxSquelchOn:
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_force_signal_detect(&pm_phy_copy, 1, 0));
            break;
        case phymodRxSquelchOff:
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_force_signal_detect(&pm_phy_copy, 0, 0));
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_rx_lane_control_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_rx_lane_control_t* rx_control)
{
    int reset, rx_squelch_enable;
    uint32_t lb_enable;
    uint8_t force_en, force_val;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    /* first get the force enabled bit and forced value */
    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_force_signal_detect_get(&pm_phy_copy, &force_en, &force_val));

    if (force_en & (!force_val)) {
        rx_squelch_enable = 1;
    } else {
        rx_squelch_enable = 0;
    }

    /* next check if PMD loopback is on */
    if (rx_squelch_enable) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_dig_lpbk_get(&pm_phy_copy, &lb_enable));
        if (lb_enable) rx_squelch_enable = 0;
    }
    if(rx_squelch_enable) {
        *rx_control = phymodRxSquelchOn;
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_rx_lane_control_get(&pm_phy_copy, &reset));
        if (reset == 0) {
            *rx_control = phymodRxReset;
        } else {
            *rx_control = phymodRxSquelchOff;
        }
    }
    return PHYMOD_E_NONE;

}

#if 0
/* load tscbh fw. the fw_loader parameter is valid just for external fw load*/
STATIC
int _aperta_tscbh_core_firmware_load(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config)
{
    plp_aperta_phymod_phy_access_t  phy_copy;

    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));

    switch(init_config->firmware_load_method){
    case phymodFirmwareLoadMethodInternal:
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_ucode_mdio_load(&phy_copy, plp_aperta_blackhawk_ucode, plp_aperta_blackhawk_ucode_len));
        break;
    case phymodFirmwareLoadMethodExternal:
        PHYMOD_NULL_CHECK(init_config->firmware_loader);
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_ucode_init(&phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_pram_firmware_enable(&phy_copy, 1, 0));
        PHYMOD_IF_ERR_RETURN(init_config->firmware_loader(core, plp_aperta_blackhawk_ucode_len, plp_aperta_blackhawk_ucode));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_pram_firmware_enable(&phy_copy, 0, 0));
        break;
    case phymodFirmwareLoadMethodNone:
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("illegal fw load method %u"), init_config->firmware_load_method));
    }

    return PHYMOD_E_NONE;
}
#endif

int plp_aperta_tscbh_phy_firmware_core_config_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_firmware_core_config_t fw_config)
{
    struct blackhawk_tsc_uc_core_config_st serdes_firmware_core_config;
    uint32_t rst_status = 0;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_MEMSET(&serdes_firmware_core_config, 0, sizeof(serdes_firmware_core_config));
    serdes_firmware_core_config.field.core_cfg_from_pcs = fw_config.CoreConfigFromPCS;

    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_core_soft_reset_read(&pm_phy_copy, &rst_status));
    if (rst_status) PHYMOD_IF_ERR_RETURN (plp_aperta_blackhawk_tsc_core_dp_reset(&pm_phy_copy, 1));
    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_INTERNAL_set_uc_core_config(&pm_phy_copy, serdes_firmware_core_config));
    if (rst_status) PHYMOD_IF_ERR_RETURN (plp_aperta_blackhawk_tsc_core_dp_reset(&pm_phy_copy, 0));

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_firmware_core_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_firmware_core_config_t* fw_config)
{

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_firmware_lane_config_get(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t* fw_config)
{
    struct blackhawk_tsc_uc_lane_config_st lane_config;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_MEMSET(&lane_config, 0x0, sizeof(lane_config));
    PHYMOD_MEMSET(fw_config, 0, sizeof(*fw_config));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_get_uc_lane_cfg(&pm_phy_copy, &lane_config));

    fw_config->LaneConfigFromPCS     = lane_config.field.lane_cfg_from_pcs;
    fw_config->AnEnabled             = lane_config.field.an_enabled;
    fw_config->DfeOn                 = lane_config.field.dfe_on;
    fw_config->LpDfeOn               = lane_config.field.dfe_lp_mode;
    fw_config->ForceBrDfe            = lane_config.field.force_brdfe_on;
    fw_config->MediaType             = lane_config.field.media_type;
    fw_config->UnreliableLos         = lane_config.field.unreliable_los;
    fw_config->Cl72AutoPolEn         = lane_config.field.cl72_auto_polarity_en;
    fw_config->ScramblingDisable     = lane_config.field.scrambling_dis;
    fw_config->Cl72RestTO            = lane_config.field.cl72_restart_timeout_en;
    fw_config->ForceES     = lane_config.field.force_es;
    fw_config->ForceNS      = lane_config.field.force_ns;
    fw_config->LinkPartnerPrecoderEn     = lane_config.field.lp_has_prec_en;
    fw_config->ForcePAM4Mode         = lane_config.field.force_pam4_mode;
    fw_config->ForceNRZMode          = lane_config.field.force_nrz_mode;

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_pam4_tx_set(const plp_aperta_phymod_phy_access_t* phy, const phymod_pam4_tx_t* tx)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i, port_start_lane, port_num_lane;
    uint32_t lane_reset, pcs_lane_enable, port_lane_mask = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*get the start lane of the port lane mask */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_port_start_lane_get(&phy_copy, &port_start_lane, &port_num_lane));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_mask_get(port_start_lane, port_num_lane, &port_lane_mask));

    phy_copy.access.lane_mask = 1 << port_start_lane;

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_enable_get(&phy_copy, &pcs_lane_enable));

    /*first check if lane is in reset */
    phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset_get(&phy_copy, &lane_reset));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << port_start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_disable_set(&phy_copy));

        /* add the pcs disable SW WAR */
        phy_copy.access.lane_mask = port_lane_mask;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_reset_sw_war(&phy_copy));
    }
    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));
    }

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        /*next check 3 tap mode or 6 tap mode */
        if (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP ||
                tx->serdes_tx_tap_mode == phymodTxTapModePAM4_LP_3TAP ) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_apply_txfir_cfg(&phy_copy,
                                                 (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP) ? NRZ_LP_3TAP : PAM4_LP_3TAP,
                                                 0,
                                                 tx->pre,
                                                 tx->main,
                                                 tx->post,
                                                 0,
                                                 0));
        } else {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_apply_txfir_cfg(&phy_copy,
                                                 (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_6TAP) ? NRZ_6TAP: PAM4_6TAP,
                                                 tx->pre2,
                                                 tx->pre,
                                                 tx->main,
                                                 tx->post,
                                                 tx->post2,
                                                 tx->post3));
        }
    }

    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << port_start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_enable_set(&phy_copy));
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_media_type_tx_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_media_typed_t media, plp_aperta_phymod_tx_t* tx)
{

    return PHYMOD_E_NONE;

}

/*
 * set lane swapping for core
 */

int plp_aperta_tscbh_core_lane_map_set(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_lane_map_t* lane_map)
{
    plp_aperta_phymod_core_access_t  core_copy;
    plp_aperta_phymod_phy_access_t  phy_copy;
    uint32_t lane, pcs_tx_swap = 0, pcs_rx_swap = 0, reg_val =0;
    uint8_t pmd_tx_addr[8], pmd_rx_addr[8];
    plp_aperta_phymod_phy_access_t *sa__;
    ucode_info_t ucode_info;

    if (lane_map->num_of_lanes != APERTA_TSCBH_NOF_LANES_IN_CORE){
        return PHYMOD_E_CONFIG;
    }

    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));
    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));
    core_copy.access.lane_mask = 0x1;
    phy_copy.access.lane_mask = 0x1;

    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++){
        if ((lane_map->lane_map_tx[lane] >= APERTA_TSCBH_NOF_LANES_IN_CORE)||
             (lane_map->lane_map_rx[lane] >= APERTA_TSCBH_NOF_LANES_IN_CORE)){
            return PHYMOD_E_CONFIG;
        }
        /*encode each lane as four bits*/
        
        pcs_tx_swap += lane_map->lane_map_tx[lane]<<(lane*4);
        pcs_rx_swap += lane_map->lane_map_rx[lane]<<(lane*4);
    }
    /* PMD lane addr is based on PCS logical to physical mapping*/
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++){
        pmd_tx_addr[((pcs_tx_swap >> (lane*4)) & 0xf)] = lane;
        pmd_rx_addr[((pcs_rx_swap >> (lane*4)) & 0xf)] = lane;
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy,0x1800d22e , &reg_val));
    ucode_info.stack_size = (reg_val & 0x3FFC) >> 2;
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy,0x1800d228 , &reg_val));
    if ( reg_val == 0) {
        ucode_info.ucode_size = 75000;
    } else {
        ucode_info.ucode_size = 83921;
    }
    phy_copy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 1));
    phy_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 1));
    
    sa__ =  &phy_copy;
        PHYMOD_IF_ERR_RETURN(
        plp_aperta_blackhawk_tsc_uc_reset_with_info(sa__, 1, ucode_info));
    /*for (micro_idx = 0; micro_idx < 4; micro_idx++) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_blackhawk_tsc_set_micro_idx(sa__, micro_idx));
        PHYMOD_IF_ERR_RETURN(
           wrc_micro_core_rstb(0));
    }*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_pcs_tx_lane_swap(&phy_copy, pcs_tx_swap));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_pcs_rx_lane_swap(&phy_copy, pcs_rx_swap));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_map_lanes(&phy_copy, APERTA_TSCBH_NOF_LANES_IN_CORE, pmd_tx_addr, pmd_rx_addr));

        PHYMOD_IF_ERR_RETURN(
        plp_aperta_blackhawk_tsc_uc_reset_with_info(sa__, 0, ucode_info));
 
    /*for (micro_idx = 3; micro_idx >= 0; micro_idx--) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_blackhawk_tsc_set_micro_idx(sa__, micro_idx));
        PHYMOD_IF_ERR_RETURN(
           wrc_micro_core_rstb(1));
    }*/

    /* Wait for ucactive*/
    PHYMOD_IF_ERR_RETURN
         (plp_aperta_blackhawk_tsc_wait_uc_active(&phy_copy));
    phy_copy.access.lane_mask = 0x1;
    phy_copy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 0));
    phy_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 0));

    return PHYMOD_E_NONE;
}

#if 0
STATIC
int _aperta_tscbh_speed_config_get(uint32_t speed, uint32_t *pll_multiplier, uint32_t *is_pam4, uint32_t *osr_mode)
{
    return PHYMOD_E_NONE;
}
#endif

int _plp_aperta_tscbh_phy_firmware_lane_config_set(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_config)
{
    uint32_t is_warm_boot;
    struct blackhawk_tsc_uc_lane_config_st serdes_firmware_config;
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;
    uint32_t rst_status;
    uint8_t flr_support = 0;

    PHYMOD_MEMSET(&serdes_firmware_config, 0x0, sizeof(serdes_firmware_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_is_flr_supported(&phy_copy, &flr_support));
    if (flr_support && fw_config.ForceES) {
        PHYMOD_DEBUG_ERROR(("ERROR :: FLR ucode is downloaded: Can not enable ForceExtenedReach with FLR ucode\n"));
        return PHYMOD_E_CONFIG;
    }

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        serdes_firmware_config.field.lane_cfg_from_pcs      = fw_config.LaneConfigFromPCS;
        serdes_firmware_config.field.an_enabled             = fw_config.AnEnabled;
        serdes_firmware_config.field.dfe_on                 = fw_config.DfeOn;
        serdes_firmware_config.field.force_brdfe_on         = fw_config.ForceBrDfe;
        /* serdes_firmware_config.field.cl72_emulation_en = fw_config.Cl72Enable; */
        serdes_firmware_config.field.scrambling_dis         = fw_config.ScramblingDisable;
        serdes_firmware_config.field.unreliable_los         = fw_config.UnreliableLos;
        serdes_firmware_config.field.media_type             = fw_config.MediaType;
        serdes_firmware_config.field.dfe_lp_mode            = fw_config.LpDfeOn;
        serdes_firmware_config.field.cl72_auto_polarity_en  = fw_config.Cl72AutoPolEn;
        serdes_firmware_config.field.cl72_restart_timeout_en = fw_config.Cl72RestTO;
        serdes_firmware_config.field.force_es               = fw_config.ForceES;
        serdes_firmware_config.field.force_ns               = fw_config.ForceNS;
        serdes_firmware_config.field.force_nrz_mode         = fw_config.ForceNRZMode;
        serdes_firmware_config.field.force_pam4_mode        = fw_config.ForcePAM4Mode;
        serdes_firmware_config.field.lp_has_prec_en         = fw_config.LinkPartnerPrecoderEn;

        PHYMOD_IF_ERR_RETURN(PHYMOD_IS_WRITE_DISABLED(&phy->access, &is_warm_boot));

        if (!is_warm_boot) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_lane_soft_reset_get(&phy_copy, &rst_status));
            if (!rst_status) PHYMOD_IF_ERR_RETURN (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_set_uc_lane_cfg(&phy_copy, serdes_firmware_config));
            if (!rst_status) PHYMOD_IF_ERR_RETURN (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));
            (void)rst_status;
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_firmware_lane_config_set(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_config)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    uint32_t lane_reset, pcs_lane_enable;
    int start_lane, num_lane;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset_get(&phy_copy, &lane_reset));

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_enable_get(&phy_copy, &pcs_lane_enable));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_disable_set(&phy_copy));
        /* add the pcs disable SW WAR */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_reset_sw_war(&phy_copy));
    }

    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));
    }

    PHYMOD_IF_ERR_RETURN
         (_plp_aperta_tscbh_phy_firmware_lane_config_set(phy, fw_config));

    if (!lane_reset) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_enable_set(&phy_copy));
    }

    return PHYMOD_E_NONE;
}


/* reset rx sequencer
 * flags - unused parameter
 */
int plp_aperta_tscbh_phy_rx_restart(const plp_aperta_phymod_phy_access_t* phy)
{

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_polarity_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_polarity_t* polarity)
{
    plp_aperta_phymod_phy_access_t phy_copy, pm_phy_copy;
    int i, start_lane, num_lane;
    uint32_t lane_reset, pcs_lane_enable;
    uint32_t tx_polarity, rx_polarity;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset_get(&pm_phy_copy, &lane_reset));

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_enable_get(&pm_phy_copy, &pcs_lane_enable));

    /* disable pcs lane if pcs lane was enabled */
    if (pcs_lane_enable) {
        pm_phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_disable_set(&pm_phy_copy));
        /* add the pcs disable SW WAR */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_reset_sw_war(&pm_phy_copy));
    }

    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 1));
    }

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        tx_polarity = ((polarity->tx_polarity >> i) & 0x1);
        rx_polarity = ((polarity->rx_polarity >> i) & 0x1);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tx_rx_polarity_set(&phy_copy, tx_polarity, rx_polarity));
    }

    /* release the ln dp reset */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane was enabled*/
    if (pcs_lane_enable) {
        pm_phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_enable_set(&pm_phy_copy));
    }

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_polarity_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_polarity_t* polarity)
{
    int start_lane, num_lane, i;
    plp_aperta_phymod_polarity_t temp_pol;
    plp_aperta_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    polarity->tx_polarity = 0;
    polarity->rx_polarity = 0;
    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        temp_pol.tx_polarity = 0;
        temp_pol.rx_polarity = 0;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tx_rx_polarity_get(&phy_copy, &temp_pol.tx_polarity, &temp_pol.rx_polarity));
        polarity->tx_polarity |= ((temp_pol.tx_polarity & 0x1) << i);
        polarity->rx_polarity |= ((temp_pol.rx_polarity & 0x1) << i);
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_pam4_tx_get(const plp_aperta_phymod_phy_access_t* phy, phymod_pam4_tx_t* tx)
{
    uint8_t pmd_tx_tap_mode;
    uint16_t tx_tap_nrz_mode = 0;
    int16_t val;
    plp_aperta_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* read current tx tap mode */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_tx_tap_mode_get(&phy_copy, &pmd_tx_tap_mode));

    /*read current tx NRZ mode control info */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_tx_nrz_mode_get(&phy_copy, &tx_tap_nrz_mode));

    if (pmd_tx_tap_mode == 0) {
        /* 3 tap mode */
        if (tx_tap_nrz_mode) {
            tx->serdes_tx_tap_mode = phymodTxTapModeNRZ_LP_3TAP;
        } else {
            tx->serdes_tx_tap_mode = phymodTxTapModePAM4_LP_3TAP;
        }
    } else {
        if (tx_tap_nrz_mode) {
            tx->serdes_tx_tap_mode = phymodTxTapModeNRZ_6TAP;
        } else {
            tx->serdes_tx_tap_mode = phymodTxTapModePAM4_6TAP;
        }
    }
    /*next check 3 tap mode or 6 tap mode */
    if (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP ||
          tx->serdes_tx_tap_mode == phymodTxTapModePAM4_LP_3TAP) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP0, &val));
            tx->pre = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP1, &val));
            tx->main = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP2, &val));
            tx->post = val;
            tx->pre2 = 0;
            tx->post2 = 0;
            tx->post3 = 0;
    } else {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP0, &val));
            tx->pre2 = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP1, &val));
            tx->pre = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP2, &val));
            tx->main = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP3, &val));
            tx->post = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP4, &val));
            tx->post2 = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_read_tx_afe(&phy_copy, TX_AFE_TAP5, &val));
            tx->post3 = val;
    }

    return PHYMOD_E_NONE;
}



int plp_aperta_tscbh_phy_tx_override_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_tx_override_t* tx_override)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
             (plp_aperta_blackhawk_tsc_tx_pi_freq_override(&phy_copy,
                                                 tx_override->phase_interpolator.enable,
                                                 tx_override->phase_interpolator.value));
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_tx_override_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_tx_override_t* tx_override)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int16_t temp_value;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tx_pi_control_get(&phy_copy, &temp_value));

    tx_override->phase_interpolator.value = (int32_t) temp_value;
    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_rx_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_rx_t* rx)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;
    uint8_t uc_lane_stopped;
    phymod_phy_signalling_method_t signalling_mode;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* next read the PAM4 mode or  not */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_signalling_mode_status_get(&phy_copy, &signalling_mode));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));

    for (i = 0; i < num_lane; i++) {
        int j = 0;
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        /* first check if uc lane is stopped already */
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_uc_lane_status(&phy_copy, &uc_lane_stopped));
        if (!uc_lane_stopped) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(&phy_copy, 1));
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_VGA, (int8_t) rx->vga.value));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_PF, (int8_t) rx->peaking_filter.value));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_PF2, (int8_t) rx->low_freq_peaking_filter.value));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_PF3, (int8_t) rx->high_freq_peaking_filter.value));

        for (j = 0 ; j < rx->num_of_dfe_taps ; j++){
            switch (j) {
                case 0:
                    if (signalling_mode == phymodSignallingMethodNRZ) {
                        PHYMOD_IF_ERR_RETURN
                            (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE1, (int8_t) rx->dfe[j].value));
                    } else {
                        if (rx->dfe[0].enable) {
                            PHYMOD_DEBUG_ERROR(("ERROR :: DFE1 is not supported on PAM4 mode \n"));
                            return PHYMOD_E_PARAM;
                        }
                    }
                    break;
                case 1:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE2, (int8_t) rx->dfe[j].value));
                    break;
                case 2:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE3, (int8_t) rx->dfe[j].value));
                    break;
                case 3:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE4, (int8_t) rx->dfe[j].value));
                    break;
                case 4:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE5, (int8_t) rx->dfe[j].value));
                    break;
                case 5:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE6, (int8_t) rx->dfe[j].value));
                    break;
                case 6:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE7, (int8_t) rx->dfe[j].value));
                    break;
                case 7:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE8, (int8_t) rx->dfe[j].value));
                    break;
                case 8:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE9, (int8_t) rx->dfe[j].value));
                    break;
                case 9:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE10, (int8_t) rx->dfe[j].value));
                    break;
                case 10:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE11,(int8_t)  rx->dfe[j].value));
                    break;
                case 11:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE12, (int8_t) rx->dfe[j].value));
                    break;
                case 12:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE13, (int8_t) rx->dfe[j].value));
                    break;
                case 13:
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_write_rx_afe(&phy_copy, RX_AFE_DFE14, (int8_t) rx->dfe[j].value));
                    break;
                default:
                    return PHYMOD_E_PARAM;
            }
        }
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_rx_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_rx_t* rx)
{
    int j;
    int8_t val;
    plp_aperta_phymod_phy_access_t phy_copy;
    phymod_phy_signalling_method_t signalling_mode;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* next read the PAM4 mode or  not */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_signalling_mode_status_get(&phy_copy, &signalling_mode));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_VGA, &val));
    rx->vga.value = val;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_PF, &val));
    rx->peaking_filter.value = val;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_PF2, &val));
    rx->low_freq_peaking_filter.value = val;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_PF3, &val));
    rx->high_freq_peaking_filter.value = val;

    rx->num_of_dfe_taps = 14;

    for (j = 0 ; j < rx->num_of_dfe_taps ; j++){
        switch (j) {
            case 0:
                if (signalling_mode == phymodSignallingMethodNRZ) {
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE1, &val));
                    rx->dfe[0].enable = 1;
                } else {
                    /* for PAM4 mode, DFE1 tap is not supported */
                    rx->dfe[0].enable = 0;
                }
                break;
            case 1:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE2, &val));
                break;
            case 2:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE3, &val));
                break;
            case 3:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE4, &val));
                break;
            case 4:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE5, &val));
                break;
            case 5:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE6, &val));
                break;
            case 6:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE7, &val));
                break;
            case 7:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE8, &val));
                break;
            case 8:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE9, &val));
                break;
            case 9:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE10, &val));
                break;
            case 10:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE11, &val));
                break;
            case 11:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE12, &val));
                break;
            case 12:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE13, &val));
                break;
            case 13:
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_read_rx_afe(&phy_copy, RX_AFE_DFE14, &val));
                break;
            default:
                return PHYMOD_E_PARAM;
        }
        rx->dfe[j].value = val;
    }

    for (j = 1 ; j < rx->num_of_dfe_taps ; j++){
        rx->dfe[j].enable = 1;
    }
    rx->vga.enable = 1;
    rx->peaking_filter.enable = 1;
    rx->low_freq_peaking_filter.enable = 1;
    rx->high_freq_peaking_filter.enable = 1;

    return PHYMOD_E_NONE;
}

/*PHY Rx adaptation resume*/
int plp_aperta_tscbh_phy_rx_adaptation_resume(const plp_aperta_phymod_phy_access_t* phy)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    uint8_t uc_lane_stopped;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_uc_lane_status(&phy_copy, &uc_lane_stopped));
    if (uc_lane_stopped) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(&phy_copy, 0));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_reset_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_reset_t* reset)
{

    return PHYMOD_E_UNAVAIL;
}


int plp_aperta_tscbh_phy_reset_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_reset_t* reset)
{

    return PHYMOD_E_UNAVAIL;

}


int plp_aperta_tscbh_phy_power_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_power_t* power)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy, *sa__;
    int start_lane, num_lane, i;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    sa__ = &pm_phy_copy;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        sa__->access.lane_mask = 1 << (start_lane + i);
        if (power->tx == phymodPowerOff) {
            EFUN(plp_aperta_blackhawk_tsc_lane_pwrdn(sa__, PWRDN_TX));
            PHYMOD_USLEEP(2000);
        } else if (power->tx == phymodPowerOn) {
            EFUN(wr_ln_tx_s_pwrdn(0x0));
            PHYMOD_USLEEP(2000);
        } else if (power->tx == phymodPowerOffOn) {
            EFUN(plp_aperta_blackhawk_tsc_lane_pwrdn(sa__, PWRDN_TX));
            PHYMOD_USLEEP(2000);
            EFUN(wr_ln_tx_s_pwrdn(0x0));
            PHYMOD_USLEEP(2000);
        }
        if (power->rx == phymodPowerOff) {
            EFUN(plp_aperta_blackhawk_tsc_lane_pwrdn(sa__, PWRDN_RX));
            PHYMOD_USLEEP(2000);
        } else if (power->rx == phymodPowerOn) {
            EFUN(wr_ln_rx_s_pwrdn(0x0));
            PHYMOD_USLEEP(2000);
        } else if (power->rx == phymodPowerOffOn) {
            EFUN(plp_aperta_blackhawk_tsc_lane_pwrdn(sa__, PWRDN_RX));
            PHYMOD_USLEEP(5000);
            EFUN(wr_ln_rx_s_pwrdn(0x0));
            PHYMOD_USLEEP(2000);
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_power_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_power_t* power)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy, *sa__;
    int start_lane, num_lane, i;
    err_code_t __err = 0;
    uint8_t value = 0;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    sa__ = &pm_phy_copy;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        sa__->access.lane_mask = 1 << (start_lane + i);
        value = rd_ln_tx_s_pwrdn();
        if (__err) {
            return __err;
        }
        power->tx = value ? phymodPowerOff : phymodPowerOn; 
        value = rd_ln_rx_s_pwrdn();
        if (__err) {
            return __err;
        }
        power->rx = value ? phymodPowerOff: phymodPowerOn; 
        break;
    }
    return PHYMOD_E_NONE;
}

/* This function based on num_lane, data_rate and fec_type
 * assign force speed SW speed_id.
 */
STATIC
int _plp_aperta_tscbh_phy_speed_id_set(int num_lane,
                            uint32_t data_rate,
                            phymod_fec_type_t fec_type,
                            aperta_tbhmod_spd_intfc_type_t* spd_intf)
{
    if (num_lane == 1) {
        switch (data_rate) {
            case 10000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = APERTA_TBHMOD_SPD_10000_XFI;
                } else if (fec_type == phymod_fec_CL74) {
                    *spd_intf = APERTA_TBHMOD_SPD_10G_FEC_BASE_R_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 12000:
                *spd_intf = APERTA_TBHMOD_SPD_12P5G_BRCM_KR1;
                break;
            case 20000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = APERTA_TBHMOD_SPD_20000_XFI;
                } else if (fec_type == phymod_fec_CL74) {
                    *spd_intf = APERTA_TBHMOD_SPD_20G_FEC_BASE_R_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 25000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = APERTA_TBHMOD_SPD_25000_XFI;
                } else if (fec_type == phymod_fec_CL74) {
                    *spd_intf = APERTA_TBHMOD_SPD_25G_FEC_BASE_R_KR1_CR1;
                } else if (fec_type == phymod_fec_CL91) {
                    *spd_intf = APERTA_TBHMOD_SPD_25G_FEC_RS_FEC_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 50000:
                if (fec_type == phymod_fec_CL91) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_BRCM_FEC_528_CR1_KR1;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_IEEE_KR1_CR1;
                } else if (fec_type == phymod_fec_RS272) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_BRCM_FEC_272_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            default:
                return PHYMOD_E_UNAVAIL;
        }
    } else if (num_lane == 2) {
        switch (data_rate) {
            case 40000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = APERTA_TBHMOD_SPD_40G_MLD_X2;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 50000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_MLD_X2;
                } else if (fec_type == phymod_fec_CL91) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_MLD_FEC_528_X2;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_50G_BRCM_FEC_544_CR2_KR2;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 100000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_BRCM_NOFEC_KR2_CR2;
                } else if (fec_type == phymod_fec_CL91) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_BRCM_FEC_528_KR2_CR2;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_IEEE_KR2_CR2;
                } else if (fec_type == phymod_fec_RS272) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_BRCM_FEC_272_KR2_CR2;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            default:
                return PHYMOD_E_UNAVAIL;
        }
    } else if (num_lane == 4) {
        switch (data_rate) {
            case 40000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = APERTA_TBHMOD_SPD_40G_MLD_X4;
                } else if (fec_type == phymod_fec_CL74) {
                    *spd_intf = APERTA_TBHMOD_SPD_40G_FEC_BASE_R_KR4_CR4;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 100000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_MLD_NO_FEC_X4;
                } else if (fec_type == phymod_fec_CL91) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_MLD_X4;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_100G_BRCM_FEC_544_1XN_KR4_CR4;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 200000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = APERTA_TBHMOD_SPD_200G_BRCM_NO_FEC_KR4_CR4;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = APERTA_TBHMOD_SPD_200G_BRCM_FEC_544_1XN_KR4_CR4;
                } else if (fec_type == phymod_fec_RS544_2XN) {
                    *spd_intf = APERTA_TBHMOD_SPD_200G_IEEE_FEC_544_2XN_KR4_CR4;
                } else if (fec_type == phymod_fec_RS272) {
                    *spd_intf = APERTA_TBHMOD_SPD_200G_BRCM_FEC_272_1XN_KR4_CR4;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            default:
                return PHYMOD_E_UNAVAIL;
        }
    } else if (num_lane == 8) {
        if (data_rate == 400000 && (fec_type == phymod_fec_RS544 || 
                    fec_type == phymod_fec_RS544_2XN)) {
            *spd_intf = APERTA_TBHMOD_SPD_400G_BRCM_FEC_544_2XN_X8;
        } else {
            return PHYMOD_E_UNAVAIL;
        }
    } else {
        return PHYMOD_E_UNAVAIL;
    }

    return PHYMOD_E_NONE;
}

/* This function only apply to non-bypass mode */
int plp_aperta_tscbh_phy_speed_config_set(const plp_aperta_phymod_phy_access_t* phy,
                                   const phymod_phy_speed_config_t* speed_config,
                                   const phymod_phy_pll_state_t* old_pll_state,
                                   phymod_phy_pll_state_t* new_pll_state)
{
    uint32_t ovco_is_pwrdn;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    uint32_t lane_mask_backup;
    uint32_t  tvco_pll_index, ovco_pll_index, tvco_pll_div = 0, ovco_pll_div = 0, request_pll_div = 0, pll_index = 0;
    uint32_t loss_in_db;
    int i, start_lane, num_lane, mapped_speed_id, ilkn_set;
    phymod_firmware_lane_config_t firmware_lane_config;
    plp_aperta_phymod_firmware_core_config_t firmware_core_config;
    aperta_tbhmod_spd_intfc_type_t spd_intf = 0;
    aperta_tbhmod_refclk_t ref_clk;
    uint32_t is_pam4, osr_mode;
    uint32_t *tscbh_spd_id_entry_26;
    uint32_t *tscbh_spd_id_entry_25;
    uint32_t *tscbh_spd_id_entry_20;


    tscbh_spd_id_entry_26 = plp_aperta_tscbh_spd_id_entry_26_get();
    tscbh_spd_id_entry_25 = plp_aperta_tscbh_spd_id_entry_25_get();
    tscbh_spd_id_entry_20 = plp_aperta_tscbh_spd_id_entry_20_get();

    PHYMOD_MEMSET(&firmware_lane_config, 0, sizeof(phymod_firmware_lane_config_t));
    firmware_lane_config = speed_config->pmd_lane_config;
    /*first make sure that tvco pll index is valid */
    if (phy->access.tvco_pll_index > 1) {
        PHYMOD_DEBUG_ERROR(("Unsupported tvco index\n"));
        return PHYMOD_E_UNAVAIL;
    }

    tvco_pll_index = phy->access.tvco_pll_index;
    ovco_pll_index = tvco_pll_index ? 0 : 1;

    PHYMOD_MEMSET(&firmware_core_config, 0x0, sizeof(firmware_core_config));

    /* Copy the PLL state */
    *new_pll_state = *old_pll_state;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    lane_mask_backup = phy->access.lane_mask;
    /* Hold the pcs lane reset */
    pm_phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_disable_set(&pm_phy_copy));
        /* add the pcs disable SW WAR */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_reset_sw_war(phy));
    pm_phy_copy.access.lane_mask = lane_mask_backup;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_set_sc_speed(&pm_phy_copy, 0, 0));
    /* write this port forced speed id entry */
    pm_phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_set_sc_speed(&pm_phy_copy, APERTA_TSCBH_FORCED_SPEED_ID_OFFSET + start_lane, 0));

    /*Hold the per lane PMD soft reset bit*/
    pm_phy_copy.access.lane_mask = lane_mask_backup;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 1));

    /* first check if current lane are in ILKN mode */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_pcs_ilkn_chk(&pm_phy_copy, &ilkn_set));
    /* if previous config is ILKN and reqeust config is ethernet
       need to clear some ILKN config */
    if (ilkn_set && !PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_GET(phy->device_op_mode)) {
        /* Remove pmd_tx_disable_pin_dis it may be asserted because of ILKn */
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_pmd_tx_disable_pin_dis_set(&pm_phy_copy, 0));
        }
        pm_phy_copy.access.lane_mask = lane_mask_backup;
        /*disable PCS ilkn mode */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_ilkn_enable(&pm_phy_copy, 0));
        /* previous config is ethernet and request is ILKN mode */
    } else if (!ilkn_set && PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_GET(phy->device_op_mode)) {
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_pmd_tx_disable_pin_dis_set(&pm_phy_copy, 1));
        }

        /*enable PCS ilkn mode */
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            pm_phy_copy.access.lane_mask = lane_mask_backup;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_pcs_ilkn_enable(&pm_phy_copy, 1));
        }
    }

    /*only update the port mode for ethernet port */
    if (!PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_GET(phy->device_op_mode)) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_update_port_mode(&pm_phy_copy));
    }

    /*for ethernet speed mode config set */
    if (!PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_GET(phy->device_op_mode)) {
        PHYMOD_IF_ERR_RETURN(_plp_aperta_tscbh_phy_speed_id_set(num_lane, speed_config->data_rate,
                    speed_config->fec_type, &spd_intf));

        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_get_mapped_speed(spd_intf, &mapped_speed_id));

        /* set the rs fec CW properly */
        if ((speed_config->fec_type == phymod_fec_RS544) ||
            (speed_config->fec_type == phymod_fec_RS544_2XN)) {
            if (start_lane < 4) {
                pm_phy_copy.access.lane_mask = 1 << 0;
            } else {
                pm_phy_copy.access.lane_mask = 1 << 4;
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_rsfec_cw_type(&pm_phy_copy, 0, 0));
        } else if ((speed_config->fec_type == phymod_fec_RS272)||
                   (speed_config->fec_type == phymod_fec_RS272_2XN)) {
            if (start_lane < 4) {
                pm_phy_copy.access.lane_mask = 1 << 0;
            } else {
                pm_phy_copy.access.lane_mask = 1 << 4;
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_rsfec_cw_type(&pm_phy_copy, 1, 0));
        }
    }
    /* Check if ovco is power down */
    pm_phy_copy.access.pll_idx = ovco_pll_index;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_pll_pwrdn_get(&pm_phy_copy, &ovco_is_pwrdn));

    /* if ovco is NOT pwoer down, then get the ovco div*/
    if (!ovco_is_pwrdn) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&pm_phy_copy, &ovco_pll_div));
    }
    /* Get TVCO because it's not allowed to change during speed set */
    pm_phy_copy.access.pll_idx = tvco_pll_index;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&pm_phy_copy, &tvco_pll_div));

    /* based on the current TVCO PLL div, decide which copy of speed id entry to load */
    /* and this step only applies to (non)ilkn port*/
    if (!PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_GET(phy->device_op_mode)) {
        /* first set the lane mask to be 0x1 */
        pm_phy_copy.access.lane_mask = 1 << 0;
        if ((tvco_pll_div == APERTA_TBHMOD_PLL_MODE_DIV_170) || (tvco_pll_div == APERTA_TBHMOD_PLL_MODE_DIV_85))  {
            /* then load 26G TVCO speed id entry */
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_mem_write(&pm_phy_copy, phymodMemSpeedIdTable, APERTA_TSCBH_FORCED_SPEED_ID_OFFSET + start_lane, 
                (tscbh_spd_id_entry_26 + mapped_speed_id * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE)));
        } else if ((tvco_pll_div == APERTA_TBHMOD_PLL_MODE_DIV_165) || (tvco_pll_div == APERTA_TBHMOD_PLL_MODE_DIV_82P5)) {
            /* then load 25G TVCO speed id entry */
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_mem_write(&pm_phy_copy, phymodMemSpeedIdTable, APERTA_TSCBH_FORCED_SPEED_ID_OFFSET + start_lane, 
                 (tscbh_spd_id_entry_25 + mapped_speed_id * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE)));
        } else {
            /* then load 20G TVCO speed id entry */
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_mem_write(&pm_phy_copy, phymodMemSpeedIdTable, APERTA_TSCBH_FORCED_SPEED_ID_OFFSET + start_lane,
                 (tscbh_spd_id_entry_20 + mapped_speed_id * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE)));
        }
    }

    /* Check the request speed VCO */
    pm_phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_refclk_get(&pm_phy_copy, &ref_clk));

    /* Get requested PLL */
    /*for ethernet speed mode config set */
    if (!PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_GET(phy->device_op_mode)) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_plldiv_lkup_get(&pm_phy_copy, mapped_speed_id, ref_clk, &request_pll_div));
    } else {
        /* Always 156.25MHz*/
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_speed_config_get(speed_config->data_rate, 1, &request_pll_div, &is_pam4, &osr_mode));
    }

    if ((ovco_pll_div == request_pll_div) || (tvco_pll_div == request_pll_div)) {
        if (ovco_pll_div == tvco_pll_div) {
            /* In case synce enabled but PLL not configured*/
            pll_index = tvco_pll_index;
        } else {
            /* First check if pll0 is active and the new speed can be
               supported with existing VCO */
            pll_index = (ovco_pll_div == request_pll_div)? ovco_pll_index : tvco_pll_index;
        }
    } else {
        /*this speed request can not be configured */
        PHYMOD_DEBUG_ERROR(("ERROR :: this speed can not be configured \n"));
        return PHYMOD_E_CONFIG;
    }
    PHYMOD_CRIT_INFO(("Req div:%x tdiv:%x odiv pwrdn:%d Odiv:%x pllidx:%d\n",
                request_pll_div, tvco_pll_div, !ovco_is_pwrdn, ovco_pll_div,pll_index ));

    /* choose the right pll index for the port */
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_pll_selection_set(&pm_phy_copy, pll_index));
    }

    pm_phy_copy.access.lane_mask = lane_mask_backup;
    /* Program OS mode */
    if (!PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_GET(phy->device_op_mode)) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pmd_osmode_set(&pm_phy_copy, mapped_speed_id, ref_clk));
    } else {
        for (i = 0; i < num_lane; i++) {
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_osr_mode_set(&pm_phy_copy, osr_mode));
        }
    }
    if (speed_config->data_rate == 400000) {
        plp_aperta_phymod_phy_access_t temp_phy;
        PHYMOD_MEMCPY(&temp_phy, phy, sizeof(plp_aperta_phymod_phy_access_t));
        temp_phy.port_loc = phymodPortLocSys;
        PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_pcs_rx_ts_update_en(&temp_phy, 0));
        temp_phy.port_loc = phymodPortLocLine;
        PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_pcs_rx_ts_update_en(&temp_phy, 0));

    }
    /*next need to set certain firmware lane config to be zero*/
    firmware_lane_config.LaneConfigFromPCS = 0;
    firmware_lane_config.AnEnabled = 0;

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        pm_phy_copy.access.lane_mask = 0x1 << (start_lane + i);

        PHYMOD_IF_ERR_RETURN
            (_plp_aperta_tscbh_phy_firmware_lane_config_set(&pm_phy_copy, firmware_lane_config));
    }

    /* if the PAM4 mode, need to program the channel loss. In NRZ mode it is zeroed. */
    loss_in_db = firmware_lane_config.ForcePAM4Mode? speed_config->PAM4_channel_loss : 0;
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_channel_loss_set(&pm_phy_copy, loss_in_db));
    }
    /* next need to enable/disable link training based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tscbh_phy_cl72_set(phy, speed_config->linkTraining));

    /*release the lne soft reset bit*/
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 0));

    /* Release the pcs lane reset */
    pm_phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_enable_set(&pm_phy_copy));

    /* first clear the current lane _mask from both */
    new_pll_state->pll1_lanes_bitmap &= ~(phy->access.lane_mask);
    new_pll_state->pll0_lanes_bitmap &= ~(phy->access.lane_mask);

    /* need to update the pll_state */
    if (pll_index) {
        new_pll_state->pll1_lanes_bitmap |= phy->access.lane_mask;
    } else {
        new_pll_state->pll0_lanes_bitmap |= phy->access.lane_mask;
    }

    return PHYMOD_E_NONE;
}

/* This function decode the fec_arch in speed_entry to fec_type.
 */
STATIC
int _plp_aperta_tscbh_fec_arch_decode_get(int fec_arch, phymod_fec_type_t* fec_type)
{
    switch (fec_arch) {
        case 1:
            *fec_type = phymod_fec_CL74;
            break;
        case 2:
            *fec_type = phymod_fec_CL91;
            break;
        case 3:
        case 6:
            *fec_type = phymod_fec_RS272;
            break;
        case 4:
            *fec_type = phymod_fec_RS544;
            break;
        case 5:
            *fec_type = phymod_fec_RS544_2XN;
            break;
        default:
            *fec_type = phymod_fec_None;
            break;
    }

    return PHYMOD_E_NONE;
}


STATIC
int _plp_aperta_tscbh_speed_id_to_speed_config_get(const plp_aperta_phymod_phy_access_t* phy,
                                        int speed_id,
                                        int num_lane,
                                        phymod_phy_speed_config_t* speed_config)
{
    uint32_t pll_div, refclk_in_hz, data_rate_lane;
    uint32_t pll_index;
    int osr_mode;
    aperta_tbhmod_refclk_t ref_clk;
    uint32_t vco_rate;
    plp_aperta_phymod_phy_access_t phy_copy;
    phymod_firmware_lane_config_t firmware_lane_config;
    spd_id_tbl_entry_t speed_config_entry;
    uint32_t packed_entry[5];


    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*first figure out which pll the current port is using */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_pll_selection_get(&phy_copy, &pll_index));

    phy_copy.access.pll_idx = pll_index;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tscbh_phy_firmware_lane_config_get(phy, &firmware_lane_config));

    /* get the PLL div from HW */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &pll_div));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_refclk_get(&phy_copy, &ref_clk));

    if (ref_clk == APERTA_TBHMOD_REF_CLK_312P5MHZ) {
        refclk_in_hz = 312500000;
    } else {
        refclk_in_hz = 156250000;
    }
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_get_vco_from_refclk_div(&phy_copy, refclk_in_hz, pll_div, &vco_rate, 0));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_osr_mode_get(&phy_copy, &osr_mode));
    if (vco_rate != 20625000) {
        data_rate_lane = 25000;
    } else {
        data_rate_lane = 20000;
    }

    /* next check if PAM4 mode enabled */
    if (firmware_lane_config.ForcePAM4Mode) {
        data_rate_lane = data_rate_lane  << 1;
    } else {
        /* to get the over sample value */
        data_rate_lane = data_rate_lane >> osr_mode;
    }

    speed_config->data_rate = data_rate_lane * num_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));
    plp_aperta_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);

    PHYMOD_IF_ERR_RETURN
        ( _plp_aperta_tscbh_fec_arch_decode_get(speed_config_entry.fec_arch, &(speed_config->fec_type)));

    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta_tscbh_speed_table_entry_to_speed_config_get(const plp_aperta_phymod_phy_access_t* phy,
                                        spd_id_tbl_entry_t* speed_config_entry,
                                        phymod_phy_speed_config_t* speed_config)
{
    uint32_t pll_div, refclk_in_hz, data_rate_lane;
    uint32_t pll_index;
    int osr_mode;
    aperta_tbhmod_refclk_t ref_clk;
    uint32_t vco_rate;
    plp_aperta_phymod_phy_access_t phy_copy;
    phymod_firmware_lane_config_t firmware_lane_config;
    int num_lane;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    switch (speed_config_entry->num_lanes) {
        case 0: num_lane = 1;
            break;
        case 1: num_lane = 2;
            break;
        case 2: num_lane = 4;
            break;
        case 3: num_lane = 8;
            break;
        case 4: num_lane = 3;
            break;
        case 5: num_lane = 6;
            break;
        case 6: num_lane = 7;
            break;
        default:
            PHYMOD_DEBUG_ERROR(("Unsupported number of lane \n"));
            return PHYMOD_E_UNAVAIL;
    }

    /*first figure out which pll the current port is using */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_pll_selection_get(&phy_copy, &pll_index));

    phy_copy.access.pll_idx = pll_index;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tscbh_phy_firmware_lane_config_get(phy, &firmware_lane_config));

    /* get the PLL div from HW */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &pll_div));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_refclk_get(&phy_copy, &ref_clk));

    if (ref_clk == APERTA_TBHMOD_REF_CLK_312P5MHZ) {
        refclk_in_hz = 312500000;
    } else {
        refclk_in_hz = 156250000;
    }
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_get_vco_from_refclk_div(&phy_copy, refclk_in_hz, pll_div, &vco_rate, 0));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_osr_mode_get(&phy_copy, &osr_mode));
    /*
     * data_rate_lane = vco_rate / 66 * 64.
     * For 25G VCO case, round down the data_rate_lane to 24000 by convention.
     */
    if (vco_rate != 20625000) {
        data_rate_lane = 25000;
    } else {
        data_rate_lane = 20000;
    }
    /* next check if PAM4 mode enabled */
    if (firmware_lane_config.ForcePAM4Mode) {
        data_rate_lane = data_rate_lane  << 1;
    } else {
        /* to get the over sample value */
        data_rate_lane = data_rate_lane >> osr_mode;
    }

    speed_config->data_rate = data_rate_lane * num_lane;

    PHYMOD_IF_ERR_RETURN
        (_plp_aperta_tscbh_fec_arch_decode_get(speed_config_entry->fec_arch, &(speed_config->fec_type)));

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_speed_config_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_speed_config_t* speed_config)
{
    uint32_t cl72_enable;
    plp_aperta_phymod_phy_access_t phy_copy;
    phymod_firmware_lane_config_t firmware_lane_config;
    int start_lane, num_lane, speed_id;
    uint32_t packed_entry[20];
    spd_id_tbl_entry_t speed_config_entry;
    int an_en, an_done, osr_mode;
    uint32_t pll_div, vco_freq_khz, pll_index;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tscbh_phy_firmware_lane_config_get(phy, &firmware_lane_config));

    speed_config->pmd_lane_config.AnEnabled          = firmware_lane_config.AnEnabled;
    speed_config->pmd_lane_config.Cl72AutoPolEn      = firmware_lane_config.Cl72AutoPolEn;
    speed_config->pmd_lane_config.Cl72RestTO         = firmware_lane_config.Cl72RestTO;
    speed_config->pmd_lane_config.DfeOn              = firmware_lane_config.DfeOn;
    speed_config->pmd_lane_config.ForceBrDfe         = firmware_lane_config.ForceBrDfe;
    speed_config->pmd_lane_config.ForceES  = firmware_lane_config.ForceES;
    speed_config->pmd_lane_config.ForceNS   = firmware_lane_config.ForceNS;
    speed_config->pmd_lane_config.ForceNRZMode       = firmware_lane_config.ForceNRZMode;
    speed_config->pmd_lane_config.ForcePAM4Mode      = firmware_lane_config.ForcePAM4Mode;
    speed_config->pmd_lane_config.LaneConfigFromPCS  = firmware_lane_config.LaneConfigFromPCS;
    speed_config->pmd_lane_config.LpDfeOn            = firmware_lane_config.LpDfeOn;
    speed_config->pmd_lane_config.LinkPartnerPrecoderEn  = firmware_lane_config.LinkPartnerPrecoderEn;
    speed_config->pmd_lane_config.MediaType          = firmware_lane_config.MediaType;
    speed_config->pmd_lane_config.ScramblingDisable  = firmware_lane_config.ScramblingDisable;
    speed_config->pmd_lane_config.UnreliableLos      = firmware_lane_config.UnreliableLos;

    /* for ethernet port */
    if (!PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_GET(phy->device_op_mode)) {
        /* first read speed id from resolved status */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_speed_id_get(&phy_copy, &speed_id));
        /*next need to check if an is enabled or not */
        PHYMOD_IF_ERR_RETURN
           (plp_aperta_tbhmod_autoneg_status_get(&phy_copy, &an_en, &an_done));

        /*read the speed id entry and get the num_lane info */
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));
        PHYMOD_MEMSET(&speed_config_entry,0,sizeof(speed_config_entry));
        /*decode speed entry */
        plp_aperta_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta_tscbh_speed_table_entry_to_speed_config_get(phy, &speed_config_entry, speed_config));


        /* if autoneg enabled, needs to update the FEC_ARCH based on the An resolved status */     
        /* however for 100G 4 lane NO FEC/with FEC, will rely on the speed id entry since the AN
        resolved status is not correct */
        if ((an_en && an_done) && (speed_id != APERTA_SPEED_ID_INDEX_100G_4_LANE_CL73_KR4) &&
            (speed_id != APERTA_SPEED_ID_INDEX_100G_4_LANE_CL73_CR4)) {
            uint8_t fec_arch;
            phy_copy.access.lane_mask = 0x1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_autoneg_fec_status_get(&phy_copy, &fec_arch));
            PHYMOD_IF_ERR_RETURN
                (_plp_aperta_tscbh_fec_arch_decode_get(fec_arch, &(speed_config->fec_type)));
        }
    } else {
        /*first figure out which pll the current port is using */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_pll_selection_get(&phy_copy, &pll_index));

        phy_copy.access.pll_idx = pll_index;
         /* get the PLL div from HW */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &pll_div));

        /* for now only 321.5H ref clock is used for  DNX device */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_get_vco_from_refclk_div(&phy_copy, REF_CLOCK_312P5, pll_div, &vco_freq_khz, 0));

        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_osr_mode_get(&phy_copy, &osr_mode));
        /* next check if PAM4 mode enabled */
        if (firmware_lane_config.ForcePAM4Mode) {
            speed_config->data_rate = (vco_freq_khz  << 1) / 1000;
        } else {
            /* to get the over sample value */
            if (osr_mode == 0) {
                speed_config->data_rate = (vco_freq_khz) / 1000;
            } else if(osr_mode == 1) {
                speed_config->data_rate = (vco_freq_khz  >> 1) / 1000;
            } else if (osr_mode == 2) {
                speed_config->data_rate = (vco_freq_khz  >> 2) / 1000;
            }
        }
    }

    /* next get the cl72 enable status */
    phy_copy.access.lane_mask = 0x1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_clause72_control_get(&phy_copy, &cl72_enable));
    speed_config->linkTraining = cl72_enable;

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_cl72_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t cl72_en)
{
    struct blackhawk_tsc_uc_lane_config_st serdes_firmware_config;
    phymod_firmware_lane_config_t firmware_lane_config;
    int start_lane, num_lane, i;
    uint32_t lane_reset, pcs_lane_enable;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    uint8_t flr_support = 0;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_get_uc_lane_cfg(&pm_phy_copy, &serdes_firmware_config));

    if ((serdes_firmware_config.field.dfe_on == 0) && cl72_en) {
      PHYMOD_DEBUG_ERROR(("ERROR :: DFE is off : Can not start CL72/CL93 with no DFE\n"));
      return PHYMOD_E_CONFIG;
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_is_flr_supported(&pm_phy_copy, &flr_support));
    if (flr_support) {
        PHYMOD_DEBUG_ERROR(("ERROR :: FLR ucode is downloaded: Can not enable CL72/CL93 with FLR ucode\n"));
        return PHYMOD_E_CONFIG;
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset_get(&pm_phy_copy, &lane_reset));

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_enable_get(&pm_phy_copy, &pcs_lane_enable));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        pm_phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_disable_set(&pm_phy_copy));
        /* add the pcs disable SW WAR */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_reset_sw_war(&pm_phy_copy));
    }

    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 1));
    }

    /* next need to clear both force ER and NR config on the firmware lane config side
    if link training enable is set */
    if (cl72_en) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tscbh_phy_firmware_lane_config_get(phy, &firmware_lane_config));

        firmware_lane_config.ForceNS = 0;
        firmware_lane_config.ForceES = 0;

         PHYMOD_IF_ERR_RETURN
            (_plp_aperta_tscbh_phy_firmware_lane_config_set(phy, firmware_lane_config));
    }
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_clause72_control(&pm_phy_copy, cl72_en));
    }

    /* release the ln dp reset */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        pm_phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_enable_set(&pm_phy_copy));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_cl72_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* cl72_en)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_clause72_control_get(&pm_phy_copy, cl72_en));

    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_cl72_status_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_cl72_status_t* status)
{
    int i;
    uint32_t tmp_status;
    int start_lane, num_lane;
    plp_aperta_phymod_phy_access_t phy_copy;


    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    status->locked = 1;

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        tmp_status = 1;
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_cl72_receiver_status(&phy_copy, &tmp_status));
        if (tmp_status == 0) {
            status->locked = 0;
            break;
        }
    }
    PHYMOD_IF_ERR_RETURN(
       plp_aperta_tscbh_phy_cl72_get(phy, &tmp_status));

    status->enabled= tmp_status;

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_loopback_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t enable)
{
    int i;
    int start_lane, num_lane;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    switch (loopback) {
    case phymodLoopbackGlobal :
    case phymodLoopbackGlobalPMD :
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            /*PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_tx_disable(&phy_copy, enable));*/
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_dig_lpbk(&phy_copy, (uint8_t) enable));
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_pmd_force_signal_detect(&phy_copy,  (int) enable, (int) enable));
        }
        /* Speed change toggle required only once for a port*/
        for (i = start_lane; i < APERTA_MAX_LANES; i++) {
            if (phy->access.lane_mask & (1 << i)) {
                phy_copy.access.lane_mask = 1 << i;
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta_tbhmod_disable_set(&phy_copy));
                PHYMOD_USLEEP(500);
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta_tbhmod_enable_set(&phy_copy));
                break;
            }
        }
        break;
    case phymodLoopbackRemotePMD :
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_rmt_lpbk(&phy_copy, (uint8_t)enable));
        }
        /* For remote loopback its required to toggle speed change only during 
         * disable*/
        if (enable == 0) {
            /* Speed change toggle required only once for a port*/
            for (i = start_lane; i < APERTA_MAX_LANES; i++) {
                if (phy->access.lane_mask & (1 << i)) {
                    phy_copy.access.lane_mask = 1 << i;
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta_tbhmod_disable_set(&phy_copy));
                    PHYMOD_USLEEP(500);
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta_tbhmod_enable_set(&phy_copy));
                    break;
                }
            }
        }
        break;
    case phymodLoopbackRemotePCS :
        PHYMOD_DEBUG_ERROR(("ERROR :: this mode is not supported\n"));
        break;
    default :
        break;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_loopback_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t* enable)
{
    int start_lane, num_lane;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    switch (loopback) {
    case phymodLoopbackGlobal :
    case phymodLoopbackGlobalPMD :
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_dig_lpbk_get(&phy_copy, enable));
        break;
    case phymodLoopbackRemotePMD :
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_rmt_lpbk_get(&phy_copy, enable));
        break;
    case phymodLoopbackRemotePCS :
        PHYMOD_DEBUG_ERROR(("ERROR :: this mode is not supported\n"));
        break;
    default :
        break;
    }
    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta_tscbh_pll_to_vco_get(aperta_tbhmod_refclk_t ref_clock, uint32_t pll, uint32_t* vco)
{
    if (ref_clock == APERTA_TBHMOD_REF_CLK_156P25MHZ) {
        switch  (pll) {
            case phymod_APERTA_TSCBH_PLL_DIV170:
                 *vco = APERTA_TBHMOD_VCO_26G;
                 break;
            case phymod_APERTA_TSCBH_PLL_DIV165:
                 *vco = APERTA_TBHMOD_VCO_25G;
                 break;
            case phymod_APERTA_TSCBH_PLL_DIV132:
                 *vco = APERTA_TBHMOD_VCO_20G;
                 break;
            case phymod_APERTA_TSCBH_PLL_DIVNONE:
                 *vco = APERTA_TBHMOD_VCO_NONE;
                 break;
            default:
                 *vco = APERTA_TBHMOD_VCO_PCS_BYPASS;
                 break;
        }
    } else if (ref_clock == APERTA_TBHMOD_REF_CLK_312P5MHZ) {
        switch  (pll) {
            case phymod_APERTA_TSCBH_PLL_DIV85:
                 *vco = APERTA_TBHMOD_VCO_26G;
                 break;
            case phymod_APERTA_TSCBH_PLL_DIV82P5:
                 *vco = APERTA_TBHMOD_VCO_25G;
                 break;
            case phymod_APERTA_TSCBH_PLL_DIV66:
                 *vco = APERTA_TBHMOD_VCO_20G;
                 break;
            case phymod_APERTA_TSCBH_PLL_DIVNONE:
                 *vco = APERTA_TBHMOD_VCO_NONE;
                 break;
            default:
                 *vco = APERTA_TBHMOD_VCO_PCS_BYPASS;
                 break;
        }
    } else {
        PHYMOD_DEBUG_ERROR(("Unsupported reference clock.\n"));
        return PHYMOD_E_UNAVAIL;
    }

    return PHYMOD_E_NONE;
}
#define APERTA_W_FW    1
/* Core initialization
 * (PASS1)
 * 1.  De-assert PMD core and PMD lane reset
 * 2.  Set heartbeat for comclk
 * 3.  Configure PMD lane mapping and PCS lane swap
 * 4.  Micro code load and verify
 * 5.  Start CRC Calculation (opt)
 * (PASS2)
 * 6.  De-assert micro reset
 * 7.  Wait for uc_active = 1
 * 8.  Initialize software information table for the micro
 * 9.  Config PMD polarity
 * 10. AFE/PLL configuration
 * 11. Set core_from_pcs_config
 * 12. Program AN default timer
 * 13. Load sd_id_table, am_table and um_table into TSC memory
 * 14. Release core DP soft reset
 */
int _plp_aperta_tscbh_core_init_pass1(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config, const plp_aperta_phymod_core_status_t* core_status)
{
#ifndef APERTA_W_FW
    uint32_t uc_enable = 0;
    plp_aperta_phymod_polarity_t tmp_pol;
    plp_aperta_phymod_phy_access_t phy_access;
    int lane;
#endif
    uint32_t am_table_load_size, um_table_load_size, i;
    uint32_t *tscbh_am_table_entry;
    uint32_t *tscbh_um_table_entry;
    uint32_t *tscbh_speed_priority_mapping_table;

    plp_aperta_phymod_core_access_t  core_copy;
    plp_aperta_phymod_phy_access_t  phy_copy;

    tscbh_am_table_entry = plp_aperta_tscbh_am_table_entry_get();
    tscbh_um_table_entry = plp_aperta_tscbh_um_table_entry_get();
    tscbh_speed_priority_mapping_table = plp_aperta_tscbh_speed_priority_mapping_table_get();

    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));
    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));
#ifndef APERTA_W_FW
    if (core->port_loc == phymodPortLocLine) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_dload_fw(core, init_config->firmware_load_method));
    }

    APERTA_TSCBH_CORE_TO_PHY_ACCESS(&phy_access, core);
    core_copy.access.lane_mask = 0x1;
    phy_copy.access.lane_mask = 0x1;

    PHYMOD_MEMSET(&tmp_pol, 0x0, sizeof(tmp_pol));

    /* 1. De-assert PMD core power and core data path reset */
    PHYMOD_IF_ERR_RETURN
        (aperta_tbhmod_pmd_reset_seq(&phy_copy, core_status->pmd_active));

    phy_copy.access.lane_mask = 0x1;
    core_copy.access.pll_idx = 1;
    phy_copy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 1));
    core_copy.access.pll_idx = 0;
    phy_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 1));

    /* De-assert PMD lane reset */
    
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        phy_access.access.lane_mask = 1 << lane;
        PHYMOD_IF_ERR_RETURN
          (plp_aperta_tbhmod_pmd_x4_reset(&phy_access));
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_uc_active_get(&phy_access, &uc_enable));
    if (uc_enable) return PHYMOD_E_NONE;

    /* 2. Set the heart beat, default is 156.25M */
    if (init_config->interface_t.ref_clock != phymodRefClk156Mhz) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_refclk_set(&phy_copy, init_config->interface_t.ref_clock));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_refclk_set(&phy_copy, APERTA_TBHMOD_REF_CLK_312P5MHZ));
    }

#else
    if (core->port_loc == phymodPortLocLine && (init_config->firmware_load_method != phymodFirmwareLoadMethodNone)) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_dload_fw(core, init_config));
    }
#endif
    am_table_load_size = APERTA_TSCBH_AM_TABLE_SIZE > APERTA_TSCBH_HW_AM_TABLE_SIZE ? APERTA_TSCBH_HW_AM_TABLE_SIZE : APERTA_TSCBH_AM_TABLE_SIZE;
    um_table_load_size = APERTA_TSCBH_UM_TABLE_SIZE > APERTA_TSCBH_HW_UM_TABLE_SIZE ? APERTA_TSCBH_HW_UM_TABLE_SIZE : APERTA_TSCBH_UM_TABLE_SIZE;
    PHYMOD_CRIT_INFO(("Uploading AM table phy:%d\n", phy_copy.access.addr));
    for (i = 0; i < am_table_load_size; i++) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_mem_write(&phy_copy, phymodMemAMTable, i,
                              (tscbh_am_table_entry + i * APERTA_TSCBH_AM_ENTRY_SIZE)));
    }
    PHYMOD_CRIT_INFO(("Uploading UM table phy:%d\n", phy_copy.access.addr));
    for (i = 0; i < um_table_load_size; i++) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_mem_write(&phy_copy, phymodMemUMTable, i,
                              (tscbh_um_table_entry + i * APERTA_TSCBH_UM_ENTRY_SIZE)));
    }

    /*need to update plp_aperta_speed_priority_mapping_table with correct speed id */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_mem_write(&phy_copy, phymodMemSpeedPriorityMapTable, 0,
                          tscbh_speed_priority_mapping_table));

    return PHYMOD_E_NONE;
}


int _plp_aperta_tscbh_core_init_pass2(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config, const plp_aperta_phymod_core_status_t* core_status)
{
    plp_aperta_phymod_phy_access_t phy_access, phy_access_copy, phy_copy;
    plp_aperta_phymod_core_access_t  core_copy;
    enum blackhawk_tsc_pll_refclk_enum refclk;
    int lane, pll_index;
    uint32_t tvco_rate, speed_id_load_size, i;
    uint8_t tvco_pll_index;
    uint32_t* tscbh_spd_id_entry_26;
    uint32_t* tscbh_spd_id_entry_25;
    uint32_t* tscbh_spd_id_entry_20;

    tscbh_spd_id_entry_26 = plp_aperta_tscbh_spd_id_entry_26_get();
    tscbh_spd_id_entry_25 = plp_aperta_tscbh_spd_id_entry_25_get();
    tscbh_spd_id_entry_20 = plp_aperta_tscbh_spd_id_entry_20_get();

    APERTA_TSCBH_CORE_TO_PHY_ACCESS(&phy_access, core);
    phy_access_copy = phy_access;
    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));
    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));
    core_copy.access.lane_mask = 0x1;
    phy_copy.access.lane_mask = 0x1;
    phy_access_copy = phy_access;
    phy_access_copy.access = core->access;
    phy_access_copy.access.lane_mask = 0x1;
    phy_access_copy.type = core->type;
    tvco_pll_index = core->access.tvco_pll_index;

    phy_copy.access.lane_mask = 0x1;
    core_copy.access.pll_idx = 1;
    phy_copy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 1));
    core_copy.access.pll_idx = 0;
    phy_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 1));


    if (init_config->interface.ref_clock == phymodRefClk156Mhz) {
        refclk = BLACKHAWK_TSC_PLL_REFCLK_156P25MHZ;
        /* first check tvco index */
        if (tvco_pll_index == 1) {
            PHYMOD_IF_ERR_RETURN(
                 _plp_aperta_tscbh_pll_to_vco_get(APERTA_TBHMOD_REF_CLK_156P25MHZ, init_config->pll1_div_init_value, &tvco_rate));
        } else if (tvco_pll_index == 0) {
            PHYMOD_IF_ERR_RETURN(
                 _plp_aperta_tscbh_pll_to_vco_get(APERTA_TBHMOD_REF_CLK_156P25MHZ, init_config->pll0_div_init_value, &tvco_rate));
        } else {
            PHYMOD_DEBUG_ERROR(("Unsupported tvco index\n"));
                return PHYMOD_E_UNAVAIL;
        }
    } else if (init_config->interface.ref_clock == phymodRefClk312Mhz) {
        refclk = BLACKHAWK_TSC_PLL_REFCLK_312P5MHZ;
        /* first check tvco index */
        if (tvco_pll_index == 1) {
            PHYMOD_IF_ERR_RETURN(
                 _plp_aperta_tscbh_pll_to_vco_get(APERTA_TBHMOD_REF_CLK_312P5MHZ, init_config->pll1_div_init_value, &tvco_rate));
        } else if (tvco_pll_index == 0) {
            PHYMOD_IF_ERR_RETURN(
                 _plp_aperta_tscbh_pll_to_vco_get(APERTA_TBHMOD_REF_CLK_312P5MHZ, init_config->pll0_div_init_value, &tvco_rate));
        } else {
            PHYMOD_DEBUG_ERROR(("Unsupported tvco index\n"));
                return PHYMOD_E_UNAVAIL;
        }
    } else {
        PHYMOD_DEBUG_ERROR(("Unsupported reference clock.\n"));
        return PHYMOD_E_UNAVAIL;
    }
#ifdef ATE_PRINT_ENABLED
	PHYMOD_DIAG_OUT(("ATE_GUIDELINES Uploading Speed ID table\n"));
#else
    PHYMOD_CRIT_INFO(("Uploading Speed ID table\n"));
#endif
    /*next need to load speed id table and AM table */
    speed_id_load_size = APERTA_TSCBH_SPEED_ID_TABLE_SIZE > APERTA_TSCBH_HW_SPEED_ID_TABLE_SIZE ? APERTA_TSCBH_HW_SPEED_ID_TABLE_SIZE : APERTA_TSCBH_SPEED_ID_TABLE_SIZE;

    if (tvco_rate == APERTA_TSCBH_VCO_26G) {
        for (i = 0; i < speed_id_load_size; i++) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_mem_write(&phy_copy, phymodMemSpeedIdTable, i,
                                  (tscbh_spd_id_entry_26 + i * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE)));
#ifdef ATE_PRINT_ENABLED
            PHYMOD_DEBUG_INFO(("ATE : phymodMemSpeedIdTable :die=%0d, lane_mask=%0x side=%s i=0x%0x entry=0x%0x",phy_copy.access.addr, phy_copy.access.lane_mask,(phy_copy.port_loc == phymodPortLocLine) ? "LINE": "SYS",i,plp_aperta_spd_id_entry_26[i][0]));
#endif
        }
    } else if (tvco_rate == APERTA_TSCBH_VCO_25G) {
        for (i = 0; i < speed_id_load_size; i++) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_mem_write(&phy_copy, phymodMemSpeedIdTable, i,
                (tscbh_spd_id_entry_25 + i * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE)));
#ifdef ATE_PRINT_ENABLED
            PHYMOD_DEBUG_INFO(("ATE : phymodMemSpeedIdTable :die=%0d, lane_mask=%0x side=%s i=0x%0x entry=0x%0x",phy_copy.access.addr, phy_copy.access.lane_mask,(phy_copy.port_loc == phymodPortLocLine) ? "LINE": "SYS",i,plp_aperta_spd_id_entry_25[i][0]));
#endif

        }
    } else if (tvco_rate == APERTA_TSCBH_VCO_20G) {
        for (i = 0; i < speed_id_load_size; i++) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_mem_write(&phy_copy, phymodMemSpeedIdTable, i,
                 (tscbh_spd_id_entry_20 + i * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE)));
#ifdef ATE_PRINT_ENABLED
            PHYMOD_DEBUG_INFO(("ATE : phymodMemSpeedIdTable :die=%0d, lane_mask=%0x side=%s i=0x%0x entry=0x%0x",phy_copy.access.addr, phy_copy.access.lane_mask,(phy_copy.port_loc == phymodPortLocLine) ? "LINE": "SYS",i,plp_aperta_spd_id_entry_20[i][0]));
#endif

        }
    }

      /*need to update plp_aperta_speed_priority_mapping_table with correct speed id */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_mem_write(&phy_copy, phymodMemSpeedPriorityMapTable, 0,  &plp_aperta_speed_priority_mapping_table[0][0]));

#ifndef APERTA_W_FW
        ucode_info_t ucode;
        ucode.stack_size = 0x9F4;
        ucode.ucode_size = 71660;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_uc_active_set(&phy_copy ,1));

        /* 6. Release uc reset */
        PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_uc_reset_with_info(&phy_copy , 0, ucode));

#endif
    (void)lane;
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        phy_access_copy.access.lane_mask = 1 << lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_pmd_ln_h_rstb_pkill_override(&phy_access_copy, 0x1));
    }
    /* we need to wait at least 10ms for the uc to settle */
    PHYMOD_USLEEP(10000);

    /* 8. Initialize software information table for the macro */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(&phy_copy));
    /* release pmd lane hard reset */
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        phy_access_copy.access.lane_mask = 1 << lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_pmd_ln_h_rstb_pkill_override(&phy_access_copy, 0x0));
    }
    /* 10. AFE/PLL configuration */
    for (pll_index = 0; pll_index < 2; pll_index++) {
        core_copy.access.pll_idx = pll_index;
        phy_copy.access.pll_idx = pll_index;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_afe_pll_reg_set(&phy_copy, &init_config->afe_pll));
    }
#ifdef ATE_PRINT_ENABLED
	PHYMOD_DIAG_OUT(("ATE : Configuring PLL 0 div_val:%x 1 div_val:%x\n",init_config->pll0_div_init_value,init_config->pll1_div_init_value ));
#else
    PHYMOD_CRIT_INFO(("Configuring PLL 0:%x 1:%x\n",init_config->pll0_div_init_value,init_config->pll1_div_init_value ));
#endif    
    /* PLL_DIV config for both PLL0 and PLL1 */
    
    if (init_config->pll0_div_init_value != phymod_APERTA_TSCBH_PLL_DIVNONE) {
        core_copy.access.pll_idx = 0;
        phy_copy.access.pll_idx = 0;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_configure_pll_refclk_div(&phy_copy,
                                                      refclk,
                                                      init_config->pll0_div_init_value));
    }

    if (init_config->pll1_div_init_value != phymod_APERTA_TSCBH_PLL_DIVNONE) {
        core_copy.access.pll_idx = 1;
        phy_copy.access.pll_idx = 1;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_configure_pll_refclk_div(&phy_copy,
                                                      refclk,
                                                      init_config->pll1_div_init_value));
    }

    /* 12. Program AN default timer  for both MMP0 and MMP1*/
    /*core_copy.access.lane_mask = 0x1;
    core_copy.access.pll_idx = 0;*/
    phy_copy.access.lane_mask = 0x1;
    phy_copy.access.pll_idx = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_set_an_timers(&phy_copy, init_config->interface.ref_clock, NULL));

    /*core_copy.access.lane_mask = 0x10;
    core_copy.access.pll_idx = 0;*/
    phy_copy.access.lane_mask = 0x10;
    phy_copy.access.pll_idx = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_set_an_timers(&phy_copy, init_config->interface.ref_clock, NULL));

    /* 13. Load spd_id_table, am_table and um_table into TSC memory */
    

    /* 14. Release core DP soft reset for both PLLs */
    core_copy.access.lane_mask = 0x1;
    core_copy.access.pll_idx = 0;
    phy_copy.access.lane_mask = 0x1;
    phy_copy.access.pll_idx = 0;

    /* next need to config PMD micro clock source, chip defualt is PLL0 and SW driver will overwrite the
    default to use PLL1 */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_micro_clk_source_select(&phy_copy, tvco_pll_index));


    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 0));
    core_copy.access.pll_idx = 1;
    phy_copy.access.pll_idx = 1;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 0));
#ifndef APERTA_W_FW
    if (0) {
         phy_copy.access.pll_idx = tvco_pll_index;
         unsigned int data = 0;
         while (1) {
             PHYMOD_IF_ERR_RETURN(
                 plp_aperta_reg32_read(&phy_copy, 0x1800d148, &data));
             if (data & 0x100) {
                 printf("PLL%d locked side:%d addr:%d\n",tvco_pll_index, phy_copy.port_loc, phy_copy.access.addr);
                 break;
             }
             printf("PLL%d not locked:%x :side :%d\n",tvco_pll_index, data,  phy_copy.port_loc);
             PHYMOD_USLEEP(500000);
         }
     }
#endif
   return PHYMOD_E_NONE;

}

int plp_aperta_tscbh_core_init(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config, const plp_aperta_phymod_core_status_t* core_status)
{
    if ( (!PHYMOD_CORE_INIT_F_EXECUTE_PASS1_GET(init_config) &&
          !PHYMOD_CORE_INIT_F_EXECUTE_PASS2_GET(init_config)) ||
        PHYMOD_CORE_INIT_F_EXECUTE_PASS1_GET(init_config)) {
        return (_plp_aperta_tscbh_core_init_pass1(core, init_config, core_status));
    }

    if ( (!PHYMOD_CORE_INIT_F_EXECUTE_PASS1_GET(init_config) &&
          !PHYMOD_CORE_INIT_F_EXECUTE_PASS2_GET(init_config)) ||
        PHYMOD_CORE_INIT_F_EXECUTE_PASS2_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta_tscbh_core_init_pass2(core, init_config, core_status));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_init(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_init_config_t* init_config)
{
    /*int pll_restart = 0;*/
    const plp_aperta_phymod_access_t *pm_acc = &phy->access;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane, i;
    phymod_firmware_lane_config_t firmware_lane_config;

#ifdef VENDOR_BROADCOM
    PHYMOD_VDBG(DBG_CFG, pm_acc, ("%-22s: p=%p adr=%0"PRIx32" lmask=%0"PRIx32"\n",
                __func__, (void *)pm_acc, pm_acc->addr, pm_acc->lane_mask));
#endif
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_MEMSET(&firmware_lane_config, 0x0, sizeof(firmware_lane_config));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(pm_acc, &start_lane, &num_lane));
    if (0) {
    /* per lane based reset release */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_pmd_x4_reset(&pm_phy_copy));

    /* Put PMD lane into soft reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset(&pm_phy_copy, 1));
    
    /* clearing all the lane config */
    PHYMOD_MEMSET(&firmware_lane_config, 0x0, sizeof(firmware_lane_config));

    
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
             (_plp_aperta_tscbh_phy_firmware_lane_config_set(&pm_phy_copy, firmware_lane_config));
        /* FROM PMD team:
         * The best initial analog calibration is achieved using the information
         * from the PMD RX PRBS checker. Using the FW register value of 3 enables
         * a continuous version that does not rely on PRBS traffic anymore.
         */
#ifndef VIRTUAL_SIM_VAL
        PHYMOD_IF_ERR_RETURN
             (plp_aperta_blackhawk_tsc_lane_cfg_fwapi_data1_set(&pm_phy_copy, 3));
#endif
        }
    }

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_rx_lane_control(&pm_phy_copy, 1));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_tx_lane_control(&pm_phy_copy, 1, 0));         /* TX_LANE_CONTROL */

    return PHYMOD_E_NONE;
}


/* this function gives the PMD_RX_LOCK_STATUS */
int plp_aperta_tscbh_phy_link_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* link_status)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_get_pcs_latched_link_status(&pm_phy_copy, link_status));
    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_rx_pmd_locked_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* pmd_lock)
{
    int start_lane, num_lane, i;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    uint8_t tmp_lock;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    *pmd_lock = 1;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_pmd_lock_status(&pm_phy_copy, &tmp_lock));
        *pmd_lock &= (uint32_t) tmp_lock;
    }
    return PHYMOD_E_NONE;

}

/* this function gives the PMD_RX_LOCK_STATUS */
int plp_aperta_tscbh_phy_rx_signal_detect_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* signal_detect)
{
    int start_lane, num_lane, i;
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    uint32_t tmp_detect;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    *signal_detect = 1;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_signal_detect(&pm_phy_copy, &tmp_detect));
        *signal_detect &= tmp_detect;
    }
    return PHYMOD_E_NONE;

}

int plp_aperta_tscbh_phy_reg_read(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t* val)
{

    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, reg_addr, val));
    return PHYMOD_E_NONE;
}


int plp_aperta_tscbh_phy_reg_write(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t val)
{
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, reg_addr, val));
    return PHYMOD_E_NONE;
}

/* get default tx taps*/
int plp_aperta_tscbh_phy_tx_taps_default_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_signalling_method_t mode, plp_aperta_phymod_tx_t* tx)
{
#if 0
    /*always default to 6-taps mode */
    tx->tap_mode = phymodTxTapMode6Tap;
    tx->sig_method = mode;
    if (mode == phymodSignallingMethodNRZ) {
        tx->pre2 = (int8_t) 0;
        tx->pre =  (int8_t) 0;
        tx->main = (int8_t) 127;
        tx->post = (int8_t) 0;
        tx->post2 = (int8_t) 0;
        tx->post3 = (int8_t) 0;
    } else {
        tx->pre2 = (int8_t) 0;
        tx->pre =  (int8_t) -24;
        tx->main = (int8_t) 132;
        tx->post = (int8_t) -12;
        tx->post2 = (int8_t) 0;
        tx->post3 = (int8_t) 0;
    }
#endif
    return PHYMOD_E_NONE;
}

/* get default tx taps*/
int plp_aperta_tscbh_phy_lane_config_default_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_signalling_method_t mode, phymod_firmware_lane_config_t* lane_config)
{
    /* default always assume backplane as the medium type and with dfe on */
    if (mode == phymodSignallingMethodNRZ) {
        lane_config->ForceNRZMode = 1;
        lane_config->ForcePAM4Mode = 0;
        lane_config->ForceNS  = 0;
    } else {
        lane_config->ForceNRZMode = 0;
        lane_config->ForcePAM4Mode = 1;
        lane_config->ForceNS  = 1;
    }
    lane_config->LaneConfigFromPCS = 0;
    lane_config->AnEnabled = 0;
    lane_config->DfeOn = 1;
    lane_config->LpDfeOn = 0;
    lane_config->ForceBrDfe = 0;
    lane_config->MediaType = 0;
    lane_config->ScramblingDisable = 0;
    lane_config->Cl72AutoPolEn = 0;
    lane_config->Cl72RestTO    = 0;
    lane_config->ForceES = 0;
    lane_config->LinkPartnerPrecoderEn = 0;
    lane_config->UnreliableLos = 0;

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_pll_multiplier_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* core_vco_pll_multiplier)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&pm_phy_copy,  core_vco_pll_multiplier));
    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_firmware_load_info_get(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_load_info_t* info)
{
    info->ucode_ptr = &plp_aperta_blackhawk_ucode[0];
    info->ucode_len = plp_aperta_blackhawk_ucode_len;
    return PHYMOD_E_NONE;
}

int _plp_aperta_tscbh_phy_autoneg_ability_to_vco_get(const phymod_autoneg_advert_abilities_t* an_advert_abilities,
                                          uint16_t* request_vco)
{
    phymod_autoneg_advert_ability_t* an_ability;
    int i;

    an_ability = an_advert_abilities->autoneg_abilities;
    /* We do not validate each abilities here since they are supposed to be validated in portmod. */
    for (i = 0; i < an_advert_abilities->num_abilities; i++) {
        switch (an_ability[i].speed) {
            case 10000:
                /* CL73-10G-1lane */
                *request_vco |= APERTA_TSCBH_VCO_20G;
                break;
            case 20000:
                /* CL73BAM-20G-2lanes */
                *request_vco |= APERTA_TSCBH_VCO_20G;
                break;
            case 25000:
                /* CL73-1lane; CL73BAM-1lane; MSA-1lane */
                *request_vco |= APERTA_TSCBH_VCO_25G;
                break;
            case 40000:
                /* CL73-40G-4lanes */
                /* CL73BAM-40G-2lanes */
                *request_vco |= APERTA_TSCBH_VCO_20G;
                break;
            case 50000:
                if ((an_ability[i].resolved_num_lanes == 1) && ((an_ability[i].fec & (1 << phymod_fec_RS544)) || 
                    (an_ability[i].fec & (1 << phymod_fec_None)))) {
                    /* CL73-50G-1lane */
                    /* CL73BAM-50G-2lanes-RS544 */
                    *request_vco |= APERTA_TSCBH_VCO_26G;
                } else {
                    /* CL73BAM-50G-1lane*/
                    /* CL73BAM-50G-2lanes-Nofec/CL74/RS528 */
                    /* MSA-50G-2lanes */
                    *request_vco |= APERTA_TSCBH_VCO_25G;
                }
                break;
            case 100000:
                if (an_ability[i].resolved_num_lanes == 2 && (an_ability[i].fec & (1 << phymod_fec_None))) {
                    /* CL73-100G-2lanes */
                    /* By default SM enables 544*/
                    *request_vco |= APERTA_TSCBH_VCO_26G;
                } else if (an_ability[i].fec & ( 1 << phymod_fec_RS544)) {
                    /* CL73BAM-100G-4lanes-RS544 */
                    *request_vco |= APERTA_TSCBH_VCO_26G;
                } else {
                    /* CL73-100G-4lanes */
                    /* CL73BAM-100G-2lanes-RS528 */
                    /* CL73BAM-100G-2lanes-Nofec */
                    /* CL73BAM-100G-4lanes-Nofec */
                    *request_vco |= APERTA_TSCBH_VCO_25G;
                }
                break;
            case 200000:
                if ((an_ability[i].fec & (1 << phymod_fec_RS544)) ||(an_ability[i].fec & (1 << phymod_fec_None))) {
                    /* CL73-200G-4lanes */
                    /* CL73BAM-200G-4lanes-RS544 */
                    *request_vco |= APERTA_TSCBH_VCO_26G;
                } else {
                    /* CL73BAM-200G-4lanes-Nofec */
                    *request_vco |= APERTA_TSCBH_VCO_25G;
                }
                break;
            default:
                break;
        }

    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_vco_to_pll_lkup(uint32_t vco, aperta_tbhmod_refclk_t refclk, uint32_t* pll_div)
{
    switch(refclk) {
        case APERTA_TBHMOD_REF_CLK_156P25MHZ:
            if (vco == APERTA_TSCBH_VCO_20G) {
                *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_132;
            } else if (vco == APERTA_TSCBH_VCO_25G) {
                *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_165;
            } else {
                *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_170;
            }
            break;
        case APERTA_TBHMOD_REF_CLK_312P5MHZ:
            if (vco == APERTA_TSCBH_VCO_20G) {
                *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_66;
            } else if (vco == APERTA_TSCBH_VCO_25G) {
                *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_82P5;
            } else {
                *pll_div = APERTA_TBHMOD_PLL_MODE_DIV_85;
            }
            break;
        default:
            break;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_autoneg_advert_ability_set(const plp_aperta_phymod_phy_access_t* phy,
                                         const phymod_autoneg_advert_abilities_t* an_advert_abilities,
                                         const phymod_phy_pll_state_t* old_pll_adv_state,
                                         phymod_phy_pll_state_t* new_pll_adv_state)
{
    uint16_t request_vco = 0;
    int start_lane, num_lane, i = 0, requested_pll_num;
    plp_aperta_phymod_phy_access_t phy_copy;
    aperta_tbhmod_refclk_t ref_clk;
    uint32_t request_pll_div[3]; 
    uint32_t  ovco_pll_div = 0, tvco_pll_div = 0, pll_index = 0, ovco_is_pwrdn = 0;
    uint8_t ovco_pll_index = 0;


    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    ovco_pll_index = phy->access.tvco_pll_index ? 0 : 1;

    /* Program local advert abilitiy registers */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_autoneg_ability_set(&phy_copy, an_advert_abilities));
    PHYMOD_IF_ERR_RETURN
        (_plp_aperta_tscbh_phy_autoneg_ability_to_vco_get(an_advert_abilities, &request_vco));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_refclk_get(&phy_copy, &ref_clk));
    /* Check the request speed VCO */
    if (request_vco & APERTA_TBHMOD_VCO_20G) {
        _plp_aperta_vco_to_pll_lkup(APERTA_TBHMOD_VCO_20G, ref_clk, &request_pll_div[i]);
        i++;
    }
    if (request_vco & APERTA_TBHMOD_VCO_25G) {
        _plp_aperta_vco_to_pll_lkup(APERTA_TBHMOD_VCO_25G, ref_clk, &request_pll_div[i]);
        i++;
    }
    if (request_vco & APERTA_TBHMOD_VCO_26G) {
        _plp_aperta_vco_to_pll_lkup(APERTA_TBHMOD_VCO_26G, ref_clk, &request_pll_div[i]);
        i++;
    }
    if (i > 2) {
        /* More than 2 vcos are needed */
        return PHYMOD_E_PARAM;
    }
    requested_pll_num = i - 1;

    /* Get OVCO info */
    phy_copy.access.pll_idx = ovco_pll_index;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_pll_pwrdn_get(&phy_copy, &ovco_is_pwrdn));
    if (!ovco_is_pwrdn) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &ovco_pll_div));
    }

    /* Get TVCO info */
    phy_copy.access.pll_idx = phy->access.tvco_pll_index;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &tvco_pll_div));

    /* Copy existing PLL lane bitmap */
    *new_pll_adv_state = *old_pll_adv_state;

    /* Clear current lane_map from both PLL lane bitmap */
    new_pll_adv_state->pll1_lanes_bitmap &= ~(phy->access.lane_mask);
    new_pll_adv_state->pll0_lanes_bitmap &= ~(phy->access.lane_mask);

    for (i = requested_pll_num; i >= 0; i--) {
        if (request_pll_div[i] == ovco_pll_div) {
            /* if ovco is using PLL 0 */
            if (!ovco_pll_index) {
            new_pll_adv_state->pll0_lanes_bitmap |= phy->access.lane_mask;
            } else {
            new_pll_adv_state->pll1_lanes_bitmap |= phy->access.lane_mask;
            }
        } else if (request_pll_div[i] == tvco_pll_div) {
            /* if ovco is using PLL 0 */
            if (!ovco_pll_index) {
                new_pll_adv_state->pll1_lanes_bitmap |= phy->access.lane_mask;
            } else {
                new_pll_adv_state->pll0_lanes_bitmap |= phy->access.lane_mask;
            }
        } else {
            /* Need VCO change */
            pll_index = ovco_pll_index;
            phy_copy.access.pll_idx = pll_index;

            if (ovco_is_pwrdn) {
                /* Power up PLL0 */
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_core_pwrdn(&phy_copy, PWR_ON));
            }
            /*toggle core dp reset */
            phy_copy.access.lane_mask = 0x1;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 1));

            /*config the PLL to the requested VCO */
            if (ref_clk == APERTA_TBHMOD_REF_CLK_312P5MHZ) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_configure_pll_refclk_div(&phy_copy,
                                                            BLACKHAWK_TSC_PLL_REFCLK_312P5MHZ,
                                                            request_pll_div[i]));
            } else {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_blackhawk_tsc_configure_pll_refclk_div(&phy_copy,
                                                            BLACKHAWK_TSC_PLL_REFCLK_156P25MHZ,
                                                            request_pll_div[i]));
            }
            /* release core soft reset */
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_tsc_core_dp_reset(&phy_copy, 0));
            ovco_pll_div = request_pll_div[i];
            new_pll_adv_state->pll0_lanes_bitmap |= phy->access.lane_mask;
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_fec_override_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* enable)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1;
    PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_fec_override_get(&phy_copy, enable));

    return PHYMOD_E_NONE;

}



int plp_aperta_tscbh_phy_autoneg_advert_ability_get(const plp_aperta_phymod_phy_access_t* phy,
                                         phymod_autoneg_advert_abilities_t* an_advert_abilities)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;
    uint32_t fec_override = 0;
    phymod_firmware_lane_config_t firmware_lane_config;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_autoneg_ability_get(&phy_copy, an_advert_abilities));

    /* Get Medium type from fw_lane_config */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tscbh_phy_firmware_lane_config_get(phy, &firmware_lane_config));

    for (i = 0; i < an_advert_abilities->num_abilities; i++) {
        an_advert_abilities->autoneg_abilities[i].medium = firmware_lane_config.MediaType;
    }
    /* check if FEC override bit is set  */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tscbh_phy_fec_override_get(phy, &fec_override));
    if (fec_override == 1) {
        for (i = 0; i < an_advert_abilities->num_abilities; i++) {
            if ((an_advert_abilities->autoneg_abilities[i].speed == 100000) &&
                (an_advert_abilities->autoneg_abilities[i].resolved_num_lanes == 4) &&
                (an_advert_abilities->autoneg_abilities[i].fec == phymod_fec_CL91) &&
                (an_advert_abilities->autoneg_abilities[i].an_mode == phymod_AN_MODE_CL73)) {
                an_advert_abilities->autoneg_abilities[i].fec = phymod_fec_None;
            }
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_autoneg_remote_advert_ability_get(const plp_aperta_phymod_phy_access_t* phy,
                                                phymod_autoneg_advert_abilities_t* an_advert_abilities)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i, is_copper = 0;
    uint32_t fec_override = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_autoneg_remote_ability_get(&phy_copy, an_advert_abilities));

    for (i = 0; i < an_advert_abilities->num_abilities; i++) {
        if (an_advert_abilities->autoneg_abilities[i].medium == phymodFirmwareMediaTypeCopperCable) {
            is_copper = 1;
            break;
        }
    }

    for (i = 0; i < an_advert_abilities->num_abilities; i++) {
        if (is_copper) {
            an_advert_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
        } else {
            an_advert_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypePcbTraceBackPlane;
        }
    }

    /* check if FEC override bit is set  */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tscbh_phy_fec_override_get(phy, &fec_override));
    if (fec_override == 1) {
        for (i = 0; i < an_advert_abilities->num_abilities; i++) {
            if ((an_advert_abilities->autoneg_abilities[i].speed == 100000) &&
                (an_advert_abilities->autoneg_abilities[i].resolved_num_lanes == 4) &&
                (an_advert_abilities->autoneg_abilities[i].fec == phymod_fec_CL91) &&
                (an_advert_abilities->autoneg_abilities[i].an_mode == phymod_AN_MODE_CL73)) {
                an_advert_abilities->autoneg_abilities[i].fec = phymod_fec_None;
            }
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_autoneg_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_autoneg_control_t* an)
{
    int num_lane_adv_encoded, mapped_speed_id;
    int start_lane, num_lane;
    int i, do_lane_config_set;
    uint32_t pll_1_div, vco_rate, refclk_in_hz;
    phymod_firmware_lane_config_t firmware_lane_config;
    aperta_tbhmod_an_control_t an_control;
    plp_aperta_phymod_phy_access_t phy_copy;
    aperta_tbhmod_refclk_t ref_clk;
    aperta_tbhmod_spd_intfc_type_t spd_intf = 0;

    PHYMOD_MEMSET(&firmware_lane_config, 0x0, sizeof(firmware_lane_config));

    PHYMOD_MEMSET(&an_control, 0x0, sizeof(an_control));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    if (an->enable) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_disable_set(&phy_copy));
        /*next choose TVCO as the PLL selelction for all the lanes*/
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_lane_pll_selection_set(&phy_copy, phy->access.tvco_pll_index));
        }
    }

    switch (an->num_lane_adv) {
        case 1:
            num_lane_adv_encoded = 0;
            break;
        case 2:
            num_lane_adv_encoded = 1;
            break;
        case 4:
            num_lane_adv_encoded = 2;
            break;
        case 8:
            num_lane_adv_encoded = 3;
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    an_control.num_lane_adv = num_lane_adv_encoded;
    an_control.enable       = an->enable;
    switch (an->an_mode) {
        case phymod_AN_MODE_CL73:
            an_control.an_type = APERTA_TBHMOD_AN_MODE_CL73;
            break;
        case phymod_AN_MODE_CL73BAM:
            an_control.an_type = APERTA_TBHMOD_AN_MODE_CL73_BAM;
            break;
        case phymod_AN_MODE_CL73_MSA:
            an_control.an_type = APERTA_TBHMOD_AN_MODE_CL73_MSA;
            break;
        default:
            return PHYMOD_E_PARAM;
            break;
    }

    /* SW WAR for 400G AN */
    /* if AN is enabled, first needs to disable timer */
    if (num_lane == 8) {
        if (an->enable) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_400g_autoneg_timer_disable(&phy_copy, 1));
        } else {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_400g_autoneg_timer_disable(&phy_copy, 0));
        }
    }

    if (an->enable) {
        /* Set AN port mode */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_set_an_port_mode(&phy_copy, start_lane));

        /* Get TVCO rate (PLL1 for now) */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_refclk_get(&phy_copy, &ref_clk));

        if (ref_clk == APERTA_TBHMOD_REF_CLK_312P5MHZ) {
            refclk_in_hz = 312500000;
        } else {
            refclk_in_hz = 156250000;
        }

        phy_copy.access.pll_idx = 1;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &pll_1_div));

        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_get_vco_from_refclk_div(&phy_copy, refclk_in_hz, pll_1_div, &vco_rate, 0));

        if (vco_rate == 20625000) {
            /* load 20G VCO spd_id */
            spd_intf = APERTA_TBHMOD_SPD_CL73_20G;
        } else if (vco_rate == 25781250) {
            /* load 25G VCO spd_id */
            spd_intf = APERTA_TBHMOD_SPD_CL73_25G;
        } else if (vco_rate == 26562500) {
            /* load 26G VCO spd_id */
            spd_intf = APERTA_TBHMOD_SPD_CL73_26G;
        } else {
            return PHYMOD_E_PARAM;
        }

        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_get_mapped_speed(spd_intf, &mapped_speed_id));

        phy_copy.access.lane_mask = 0x1 << start_lane;

        /* Load 1G speed ID*/ 
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_set_sc_speed(&phy_copy, mapped_speed_id, 0));
    }
    do_lane_config_set = 0;
 
    if (an->enable) {
        /* make sure the firmware config is set to an enabled */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tscbh_phy_firmware_lane_config_get(&phy_copy, &firmware_lane_config));
        /* make sure the firmware config is set to an enabled */
        if (firmware_lane_config.AnEnabled != 1) {
              firmware_lane_config.AnEnabled = 1;
              do_lane_config_set = 1;
        }
        if (firmware_lane_config.LaneConfigFromPCS != 1) {
          firmware_lane_config.LaneConfigFromPCS = 1;
          do_lane_config_set = 1;
        }
        firmware_lane_config.Cl72RestTO = 0;
        firmware_lane_config.ForceNS = 0;
        firmware_lane_config.ForceES = 0;
        firmware_lane_config.ForceNRZMode= 0;
        firmware_lane_config.ForcePAM4Mode = 0;
    }
    if (do_lane_config_set) {
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));
        }
        PHYMOD_USLEEP(1000);
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (_plp_aperta_tscbh_phy_firmware_lane_config_set(&phy_copy, firmware_lane_config));
        }
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));
        }
    }

    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_autoneg_control(&phy_copy, &an_control));

    if (!an->enable) {
        PHYMOD_IF_ERR_RETURN
         (plp_aperta_tbhmod_enable_set(&phy_copy));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_autoneg_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_control_t* an, uint32_t* an_done)
{
    aperta_tbhmod_an_control_t an_control;
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane;
    int an_complete = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_MEMSET(&an_control, 0x0,  sizeof(aperta_tbhmod_an_control_t));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_autoneg_control_get(&phy_copy, &an_control, &an_complete));

    if (an_control.enable) {
        an->enable = 1;
        *an_done = an_complete;
    } else {
        an->enable = 0;
        *an_done = 0;
    }

    switch (an_control.num_lane_adv) {
        case 0:
            an->num_lane_adv = 1;
            break;
        case 1:
            an->num_lane_adv = 2;
            break;
        case 2:
            an->num_lane_adv = 4;
            break;
        case 3:
            an->num_lane_adv = 8;
            break;
        default:
            an->num_lane_adv = 0;
            break;
    }

    switch (an_control.an_type) {
        case APERTA_TBHMOD_AN_MODE_CL73:
            an->an_mode = phymod_AN_MODE_CL73;
            break;
        case APERTA_TBHMOD_AN_MODE_CL73_BAM:
            an->an_mode = phymod_AN_MODE_CL73BAM;
            break;
        case APERTA_TBHMOD_AN_MODE_MSA:
            an->an_mode = phymod_AN_MODE_MSA;
            break;
        case APERTA_TBHMOD_AN_MODE_CL73_MSA:
            an->an_mode = phymod_AN_MODE_CL73_MSA;
            break;
        default:
            an->an_mode = phymod_AN_MODE_NONE;
            break;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_autoneg_status_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_status_t* status)
{
    int an_en, an_done;
    phymod_phy_speed_config_t speed_config;
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane/*, speed_id*/;
   /* uint32_t packed_entry[5];
    spd_id_tbl_entry_t speed_config_entry;*/

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN
       (plp_aperta_tbhmod_autoneg_status_get(&phy_copy, &an_en, &an_done));

    PHYMOD_IF_ERR_RETURN
       (plp_aperta_tscbh_phy_speed_config_get(phy, &speed_config));
#if 0
    if (an_en && an_done) {
        uint32_t an_resolved_mode;
        /* if an resolves and link up */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_speed_id_get(&phy_copy, &speed_id));
        /*read the speed id entry and get the num_lane info */
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));
        plp_aperta_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);
        num_lane = 1 << speed_config_entry.num_lanes;
        /* read the AN final resolved port mode */
        PHYMOD_IF_ERR_RETURN
            (aperta_tbhmod_resolved_port_mode_get(&phy_copy, &an_resolved_mode));
        status->resolved_port_mode = an_resolved_mode;
    }
#endif
    status->enabled   = an_en;
    status->locked    = an_done;
    status->data_rate = speed_config.data_rate;
    /*status->resolved_num_lane = num_lane;*/

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_pll_reconfig(const plp_aperta_phymod_phy_access_t* phy,
                            uint8_t pll_index,
                            uint32_t pll_div, plp_aperta_phymod_ref_clk_t ref_clk1)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;
    aperta_tbhmod_refclk_t ref_clk;
    uint32_t tvco_rate = 0, speed_id_load_size, i, pll_is_pwrdn;
    uint8_t tvco_pll_index, tvco_reconfig = 0;
    enum blackhawk_tsc_pll_refclk_enum refclk;
    uint32_t cnt = 0, pll_lock = 0;
    uint32_t* tscbh_spd_id_entry_26;
    uint32_t* tscbh_spd_id_entry_25;
    uint32_t* tscbh_spd_id_entry_20;

    tscbh_spd_id_entry_26 = plp_aperta_tscbh_spd_id_entry_26_get();
    tscbh_spd_id_entry_25 = plp_aperta_tscbh_spd_id_entry_25_get();
    tscbh_spd_id_entry_20 = plp_aperta_tscbh_spd_id_entry_20_get();

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    tvco_pll_index = phy->access.tvco_pll_index;

    if (pll_index > 1) {
        PHYMOD_DEBUG_ERROR(("Unsupported PLL index\n"));
        return PHYMOD_E_UNAVAIL;
    }

    tvco_reconfig = (tvco_pll_index == pll_index) ? 1 : 0;
    PHYMOD_CRIT_INFO(("####tcvo_index:%d side:%x vcoreconfig:%d\n", tvco_pll_index, phy->port_loc,tvco_reconfig));

    if (pll_index == 0) {
        struct   blackhawk_tsc_uc_core_config_st core_cfg;
        uint32_t pll1_div = 0;
        pm_phy_copy.access.pll_idx = 1;
        pm_phy_copy.access.lane_mask = 1 << 0;

        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_get_uc_core_config(&pm_phy_copy, &core_cfg));
        /* Pll1 cannot be 10G, so checking only for 25G or 26G*/
        if (core_cfg.vco_rate_in_Mhz > 25000 && core_cfg.vco_rate_in_Mhz < 26000) {
            pll1_div = 165;
        } else  {
            pll1_div = 170;
        }
        if (pll_div > pll1_div) {
            PHYMOD_DEBUG_ERROR(("Pll1 cfg:%d Pll0 div:%x. Pll1 divider cannot be less than pll0 divider\n", core_cfg.vco_rate_in_Mhz, pll_div));
            return PHYMOD_E_PARAM;
        }
    }
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane_mask = 1 << 0;
    /* first needs to read the ref clock from main reg*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_refclk_get(&pm_phy_copy, &ref_clk));

    if (ref_clk == APERTA_TBHMOD_REF_CLK_156P25MHZ) {
        refclk = BLACKHAWK_TSC_PLL_REFCLK_156P25MHZ;
    } else if (ref_clk == APERTA_TBHMOD_REF_CLK_312P5MHZ) {
        refclk = BLACKHAWK_TSC_PLL_REFCLK_312P5MHZ;
    } else {
        PHYMOD_DEBUG_ERROR(("Unsupported reference clock.\n"));
        return PHYMOD_E_UNAVAIL;
    }

    if (tvco_reconfig) {
        PHYMOD_CRIT_INFO(("Configuring TVCO rate\n"));
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta_tscbh_pll_to_vco_get(ref_clk, pll_div, &tvco_rate));
    } else {
        PHYMOD_CRIT_INFO(("Configuring OVCO rate\n"));
        tvco_rate = 0;
    }

    /* next disable pcs datapath only if TVCO re-config*/
    if (tvco_reconfig) {
        PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_disable_set(&pm_phy_copy));
        /* add the pcs disable SW WAR */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_reset_sw_war(&pm_phy_copy));
    }

    /*only need to load speed id table for tvco re-config */
    /*then reload the speed id table based on the new tvco */
    if (tvco_reconfig) {
        speed_id_load_size = APERTA_TSCBH_SPEED_ID_TABLE_SIZE > APERTA_TSCBH_HW_SPEED_ID_TABLE_SIZE ? APERTA_TSCBH_HW_SPEED_ID_TABLE_SIZE : APERTA_TSCBH_SPEED_ID_TABLE_SIZE;
        if (tvco_rate == APERTA_TBHMOD_VCO_26G) {
            for (i = 0; i < speed_id_load_size; i++) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_mem_write(&pm_phy_copy, phymodMemSpeedIdTable, i, 
                                      (tscbh_spd_id_entry_26 + i * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE)));
            }
        } else if (tvco_rate == APERTA_TSCBH_VCO_25G) {
            for (i = 0; i < speed_id_load_size; i++) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_mem_write(&pm_phy_copy, phymodMemSpeedIdTable, i, 
                                      (tscbh_spd_id_entry_25 + i * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE)));
            }
        } else if (tvco_rate == APERTA_TBHMOD_VCO_20G){
            for (i = 0; i < speed_id_load_size; i++) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_mem_write(&pm_phy_copy, phymodMemSpeedIdTable, i, 
                                      (tscbh_spd_id_entry_20 + i * APERTA_TSCBH_SPEED_ID_ENTRY_SIZE)));
            }
        }
    }

    /*next check if the PLL is power down or not */
    pm_phy_copy.access.pll_idx = pll_index;
    pm_phy_copy.access.lane_mask = 1 << 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_pll_pwrdn_get(&pm_phy_copy, &pll_is_pwrdn));

    /* if PLL is power down, need to power up first */
    if (pll_is_pwrdn) {
        /* Power up ovco if it's power down */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_core_pwrdn(&pm_phy_copy, PWR_ON));
    }
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&pm_phy_copy, 1));

    /*next re-config pll divider */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_configure_pll_refclk_div(&pm_phy_copy,
                                                      refclk,
                                                      pll_div));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_core_dp_reset(&pm_phy_copy, 0));
    /* need to wait for the PLL lock */
    cnt = 0;
    while (cnt < 500) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_pll_lock_get(&pm_phy_copy, &pll_lock));
        cnt = cnt + 1;
        if(pll_lock) {
            break;
        } else {
            if(cnt == 500) {
                PHYMOD_DEBUG_ERROR(("WARNING :: core 0x%x PLL Index %d is not locked within 5 milli second \n", pm_phy_copy.access.addr, pll_index));
                break;
            }
        }
        PHYMOD_USLEEP(10);
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_tx_pam4_precoder_enable_set(const plp_aperta_phymod_phy_access_t* phy, int enable)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;
    uint32_t lane_reset, pcs_lane_enable;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_soft_reset_get(&phy_copy, &lane_reset));

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_enable_get(&phy_copy, &pcs_lane_enable));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_disable_set(&phy_copy));
        /* add the pcs disable SW WAR */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_reset_sw_war(&phy_copy));
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 1));
    }

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_tx_pam4_precoder_enable_set(&phy_copy, enable));
    }

    /* release the ln dp reset */
    if (!lane_reset) {
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_lane_soft_reset(&phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_enable_set(&phy_copy));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_tx_pam4_precoder_enable_get(const plp_aperta_phymod_phy_access_t* phy, int *enable)
{
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_tx_pam4_precoder_enable_get(&pm_phy_copy, enable));
    return PHYMOD_E_NONE;
}

/*Set/Get timesync enable*/
int plp_aperta_tscbh_timesync_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, is_sfd = 0;
    uint32_t pcs_lane_enable;
    uint32_t fclk_div_mode = 0;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    phy_copy.access.lane_mask = 0x1 << start_lane;

    /* RX timestamping control */
    if (PHYMOD_TS_DIRECTION_RX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_rx_ts_en(&phy_copy, enable, is_sfd));
    }

    /* Core related control:
     *     1. Enable fclk on PMD, with default div_mode.
     */
    /*if (PHYMOD_TIMESYNC_ENABLE_CORE_GET(flags))*/ { 
        phy_copy.access.pll_idx = phy_copy.access.tvco_pll_index;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_blackhawk_tsc_clk4sync_enable_set(&phy_copy, enable, fclk_div_mode));
    }

    /* One-Step Timestamp Pipeline */
    /*if (PHYMOD_TIMESYNC_ENABLE_F_ONE_STEP_PIPELINE_GET(flags))*/
    if (flags & 0x4) {
        /* check if PCS lane is in reset */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_enable_get(&phy_copy, &pcs_lane_enable));

        /* disable pcs lane if pcs lane not in rset */
        if (pcs_lane_enable) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_disable_set(&phy_copy));
            /* add the pcs disable SW WAR */
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_pcs_reset_sw_war(phy));
        }

        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_osts_pipeline(&phy_copy, enable));

        /* re-enable pcs lane if pcs lane not in rset */
        if (pcs_lane_enable) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_enable_set(&phy_copy));
        }
    }
    /* PCS timestamp to MAC */
    /* Enabling it by default*/
    /*if (PHYMOD_TIMESYNC_ENABLE_F_RX_PCS_ENABLE_GET(flags))*/ {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_rx_ts_update_en(&phy_copy, enable));
    }

    return PHYMOD_E_NONE;
}

/* Only one flag can be served each time.
 */
int plp_aperta_tscbh_timesync_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable)
{
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));


    /*if (PHYMOD_TIMESYNC_ENABLE_F_ONE_STEP_PIPELINE_GET(flags))*/
    if (flags & 0x4) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_osts_pipeline_get(&phy_copy, enable));
    } else {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_rx_ts_en_get(&phy_copy, enable));

    }

    return PHYMOD_E_NONE;
}

/*Set timesync adjust*/
int plp_aperta_tscbh_timesync_adjust_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t ts_am_norm_mode)
{
    int start_lane, num_lane, i, is_pam4, is_valid = 0;
    uint8_t fec_arch;
    int speed_id, mapped_speed_id;
    int an_en, an_done;
    plp_aperta_phymod_phy_access_t phy_copy;
    spd_id_tbl_entry_t speed_config_entry;
    uint32_t packed_entry[5];
    phymod_phy_speed_config_t speed_config;
    phymod_firmware_lane_config_t firmware_lane_config;
    aperta_tbhmod_spd_intfc_type_t spd_intf = 0;
    phymod_mem_type_t tx_mem,rx_mem;
    int ts_table_index;
    int osr_mode;
    aperta_ts_table_entry *ts_tx_entry = NULL, *ts_rx_entry = NULL;
    aperta_ts_table_entry ts_update_table;
    uint32_t psll_entry[APERTA_TBHMOD_TS_TABLE_SIZE * APERTA_TBHMOD_TS_PSLL_BASED_ENTRY_SIZE];
    uint32_t psll_entry_size, ts_table_size;
    uint32_t pll_div, pll_index;
    aperta_tbhmod_refclk_t ref_clk;
    uint32_t tvco, current_vco;
    int mem_offset;
    aperta_interrupt_type_t intr_type = phymodIntrNone;
    aperta_tbhmod_intr_status_t intr_status;
    char intr_str[16];
    aperta_ts_table_entry* plp_aperta_ts_table_tx_sop;
    aperta_ts_table_entry* plp_aperta_ts_table_rx_sop;
    int am_processing =1;
    int num_retry = 0;
    int rv = 0;

    plp_aperta_ts_table_tx_sop = plp_aperta_ts_table_tx_sop_get();
    plp_aperta_ts_table_rx_sop = plp_aperta_ts_table_rx_sop_get();

    if (ts_am_norm_mode == 0x2) {
        PHYMOD_DEBUG_ERROR(("TSCBH does not supported Latestlane Mode.\n"));
        return PHYMOD_E_UNAVAIL;
    }

    /* Here starts the sequence to enable Timestamping based on the current speed config */
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_IF_ERR_RETURN
       (plp_aperta_tbhmod_autoneg_status_get(&phy_copy, &an_en, &an_done));
    PHYMOD_IF_ERR_RETURN
       (plp_aperta_tscbh_phy_firmware_lane_config_get(phy, &firmware_lane_config));

    is_pam4 = firmware_lane_config.ForcePAM4Mode;
    if (start_lane < 4) {
        /* MPP0 */
        mem_offset = start_lane * APERTA_TBHMOD_TS_DEFAULT_TABLE_SIZE;
    } else {
        /* MPP1 */
        mem_offset = (start_lane - 4) * APERTA_TBHMOD_TS_DEFAULT_TABLE_SIZE;
    }

    /* 1. Find default 1588 Table */

    /* 1.1 Get current speed id */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_speed_id_get(&phy_copy, &speed_id));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));
    plp_aperta_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);
    /* 1.2 Update num_lane and lane_mask for AN port */
    if (an_en && an_done) {
        num_lane = 1 << speed_config_entry.num_lanes;
        /* Update lane_mask */
        phy_copy.access.lane_mask = 0x0;
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask |= 0x1 << (i + start_lane);
        }
    }
    /* 1.3 Get FEC type */
    PHYMOD_IF_ERR_RETURN
        (_plp_aperta_tscbh_speed_id_to_speed_config_get(phy, speed_id, num_lane, &speed_config));

    if (an_en && an_done) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_autoneg_fec_status_get(&phy_copy, &fec_arch));
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta_tscbh_fec_arch_decode_get(fec_arch, &(speed_config.fec_type)));
    }

    /* Base-R CL74 and RS272 does not support 1588 */
    if ((speed_config.fec_type == phymod_fec_CL74)||
        (speed_config.fec_type == phymod_fec_RS272)) {
        PHYMOD_DEBUG_ERROR(("1588 is not supported in current speed config.\n"));
        return PHYMOD_E_UNAVAIL;
    }

    /* 1.4 Get mapped speed id */
    if (speed_id <= 0x25) {
        /* AN speed IDs */
        mapped_speed_id = speed_id;
    } else {
        /* Customized speed ID, need extra mapping. */
        /* Only applys to FS */
        PHYMOD_IF_ERR_RETURN(_plp_aperta_tscbh_phy_speed_id_set(num_lane, speed_config.data_rate,
                                                     speed_config.fec_type, &spd_intf));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_get_mapped_speed(spd_intf, &mapped_speed_id));
    }

    /* 1.5 Get the table index of the 1588 table */
    PHYMOD_IF_ERR_RETURN
       (plp_aperta_tbhmod_1588_table_index_get(mapped_speed_id, speed_config.fec_type, &ts_table_index, &ts_table_size));

    if (ts_table_index == -1) {
        PHYMOD_DEBUG_ERROR(("1588 is not supported in current speed config.\n"));
        return PHYMOD_E_UNAVAIL;
    }

    psll_entry_size = ts_table_size * APERTA_TBHMOD_TS_PSLL_BASED_ENTRY_SIZE;

    /* 1.6 Find the 1588 table */
    ts_tx_entry = plp_aperta_ts_table_tx_sop + ts_table_index;
    ts_rx_entry = plp_aperta_ts_table_rx_sop + ts_table_index;

    if (num_lane == 8) {
        tx_mem = phymodMemTxLkup1588400G;
        rx_mem = phymodMemRxLkup1588400G;
        intr_type = phymodIntrEccRx1588400g;
    } else if (start_lane < 4) {
        tx_mem = phymodMemTxLkup1588Mpp0;
        rx_mem = phymodMemRxLkup1588Mpp0;
        intr_type = phymodIntrEccRx1588Mpp0;
    } else {
        tx_mem = phymodMemTxLkup1588Mpp1;
        rx_mem = phymodMemRxLkup1588Mpp1;
        intr_type = phymodIntrEccRx1588Mpp1;
    }
    /* Get OS mode */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_osr_mode_get(&phy_copy, &osr_mode));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_refclk_get(&phy_copy, &ref_clk));

    /* Get current used VCO */
     PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_pll_selection_get(&phy_copy, &pll_index));
    phy_copy.access.pll_idx = pll_index;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &pll_div));
    PHYMOD_IF_ERR_RETURN
        (_plp_aperta_tscbh_pll_to_vco_get(ref_clk, pll_div, &current_vco));
    /* Get TVCO */
    phy_copy.access.pll_idx = phy_copy.access.tvco_pll_index;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &pll_div));
    PHYMOD_IF_ERR_RETURN
        (_plp_aperta_tscbh_pll_to_vco_get(ref_clk, pll_div, &tvco));

    /* 2. Program UI */
    
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_pcs_set_1588_ui(&phy_copy, current_vco, tvco, osr_mode, 0, is_pam4));

    /* 3. Program PMD lantency */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_1588_pmd_latency(&phy_copy, current_vco, osr_mode, is_pam4));

    /* 4. Enable rx deskew
     *    Need low to high transition to trigger HW recording the current status.
    */
    if (ts_am_norm_mode == 0x1) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_rx_deskew_en(&phy_copy, 0));
        PHYMOD_USLEEP(10);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_pcs_rx_deskew_en(&phy_copy, 1));
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_pcs_rx_ts_update_en(&phy_copy, 1));

    /* 5. Load 1588 TX table */
    /* Different MPPs have different memory space.
     * Different logical ports have different memory offset.
     */
    for (i = 0; i < (int)ts_table_size; i++) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_mem_write(&phy_copy, tx_mem, (i + mem_offset), &(*ts_tx_entry)[i][0]));
    }

    /* 6. Load 1588 RX table */
    if (ts_am_norm_mode == 0x1) {
        /* RX tbl need to update deskew before writing to mem */

        while (am_processing && (num_retry < APERTA_TSCBH_MAX_NUM_AM_RETRY)) {

            /* 6.1 Check for deskew valid */
            for (i = 0; i < 1000; i++) {
                PHYMOD_USLEEP(10);
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_tbhmod_pcs_ts_deskew_valid(&phy_copy, speed_config_entry.bit_mux_mode, &is_valid));
                if (is_valid) {
                    break;
                }
            }
            if (!is_valid) {
                return PHYMOD_E_TIMEOUT;
            }

            /* 6.2 Update deskew to 1588 table */
            /* 6.2.1 Translate RX table to psuedo logical lane(PSLL) based array */
            for (i = 0; i < (int)ts_table_size; i++) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_tbhmod_tbl_entry_to_psll_entry_map(&(*ts_rx_entry)[i][0], &psll_entry[i * APERTA_TBHMOD_TS_PSLL_BASED_ENTRY_SIZE]));
            }

            /* 6.2.2 Calculate deskew and update time value for each PSLL */
            rv = plp_aperta_tbhmod_pcs_mod_rx_1588_tbl_val(&phy_copy, speed_config_entry.bit_mux_mode,
                                            current_vco, osr_mode, is_pam4, psll_entry_size, psll_entry);

            if (rv == PHYMOD_E_NONE) {
                am_processing = 0;
            } else if (rv == PHYMOD_E_INTERNAL) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_tbhmod_pcs_rx_deskew_en(&phy_copy, 0));
                PHYMOD_USLEEP(10);
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_tbhmod_pcs_rx_deskew_en(&phy_copy, 1));
                num_retry++;
            } else {
                return rv;
            }
        } /* while */

        /*
         * check to see if failed to update deskew.
         */
        if (num_retry == APERTA_TSCBH_MAX_NUM_AM_RETRY) {
            PHYMOD_DEBUG_ERROR((" tscbh_timesync_adjust_set Failed to process AM ...\n"));
            return PHYMOD_E_TIMEOUT;
        }

        /* 6.2.3 Add PM offset to each PSLL */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_ts_offset_rx_set(&phy_copy, current_vco, osr_mode, psll_entry_size, psll_entry));

        /* 6.2.4 Translate PSLL based array back to 1588 entry format */
        for (i = 0; i < (int)ts_table_size; i++) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_psll_entry_to_tbl_entry_map(&psll_entry[i * APERTA_TBHMOD_TS_PSLL_BASED_ENTRY_SIZE], &ts_update_table[i][0]));
        }

        /* 2-stage SW WAR for RX 1588 memory accessing:
         * Stage 1: Before accessing RX 1588 memory.
         * Disable corresponding RX 1588 memory ECC interrupt.
        PHYMOD_IF_ERR_RETURN
            (tbhmod_ecc_error_intr_enable_set(&phy_copy.access, intr_type, 0));
         */

        /* 6.3 Write the updated RX table to memroy */
        for (i = 0; i < (int)ts_table_size; i++) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_mem_write(&phy_copy, rx_mem, (i + mem_offset), &ts_update_table[i][0]));
        }
    } else {
        /* 2-stage SW WAR for RX 1588 memory accessing:
         * Stage 1: Before accessing RX 1588 memory.
         * Disable corresponding RX 1588 memory ECC interrupt.
        PHYMOD_IF_ERR_RETURN
            (tbhmod_ecc_error_intr_enable_set(&phy_copy.access, intr_type, 0));
         */

        /* If deskew update is not required, load default tables. */
        /* 6.1.1 Translate RX table to psuedo logical lane(PSLL) based array */
        for (i = 0; i < (int)ts_table_size; i++) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_tbl_entry_to_psll_entry_map(&(*ts_rx_entry)[i][0], &psll_entry[i * APERTA_TBHMOD_TS_PSLL_BASED_ENTRY_SIZE]));
        }
        /* 6.1.2 Add PM offset to each PSLL. */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_ts_offset_rx_set(&phy_copy, current_vco, osr_mode, psll_entry_size, psll_entry));

        /* 6.1.3 Translate PSLL based array back to 1588 table format. */
        for (i = 0; i < (int)ts_table_size; i++) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_psll_entry_to_tbl_entry_map(&psll_entry[i * APERTA_TBHMOD_TS_PSLL_BASED_ENTRY_SIZE], &ts_update_table[i][0]));
        }
        /* 6.1.4 Load the updated RX table to memory. */
        for (i = 0; i < (int)ts_table_size; i++) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_mem_write(&phy_copy, rx_mem, (i + mem_offset), &ts_update_table[i][0]));
        }
    }

    /* 2-stage SW WAR for RX 1588 memory accessing:
     * Stage 2: After accessing RX 1588 memory.
     * Read the ECC status.
     *      If error address is within the range being written, ignore it.
     *      If error address is outside the modification range, print warning.
     *      ECC interrupt will be flagged during next HW access, which will invoke
     *      interrupt handler to handle the true ECC error.
     * Re-enable ECC interrupt.
     */
    intr_status.type = intr_type;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_intr_status_get(&phy_copy, &intr_status));
    if (intr_status.is_1b_err || intr_status.is_2b_err) {
        if ((intr_status.err_addr < mem_offset) ||
            (intr_status.err_addr >= (mem_offset + ts_table_size))) {
            if (intr_status.is_2b_err) {
                PHYMOD_DEBUG_ERROR(("WARNING :: core 0x%x detects 2-bit %s ECC error at address 0x%x.\n",
                                    phy_copy.access.addr, intr_str, intr_status.err_addr));
            } else {
                PHYMOD_DEBUG_ERROR(("WARNING :: core 0x%x detects 1-bit %s ECC error at address 0x%x.\n",
                                    phy_copy.access.addr, intr_str, intr_status.err_addr));
            }
        }
    }
    /*PHYMOD_IF_ERR_RETURN
        (tbhmod_ecc_error_intr_enable_set(&phy_copy.access, intr_type, 1));*/
    /* 7. Enable SFD/SOP timestamping on tx and rx */
    /* In Gen1, only applys to 10G, 20G and 25G. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_pcs_set_1588_xgmii(&phy_copy, 0, current_vco, osr_mode));

    return PHYMOD_E_NONE;
}


int _plp_aperta_tscbh_flexport_sw_workaround(const plp_aperta_phymod_phy_access_t* phy)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, mapped_speed_id;
    uint32_t pll_div, pll_index;
    uint32_t packed_entry[5];
    aperta_tbhmod_spd_intfc_type_t spd_intf = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* enable PMD lane override */
    phy_copy.access.lane_mask = 1 << start_lane;

    /* Hold the pcs lane reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_disable_set(&phy_copy));

    /* get PLL index */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_lane_pll_selection_get(&phy_copy, &pll_index));

    /* get the PLL div */
    phy_copy.access.lane_mask = 0x1;
    phy_copy.access.pll_idx = pll_index;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(&phy_copy, &pll_div));

    /* for 26G VCO, use speed id 5
       for 25G vco, use speed id 2
       for 20G vco, use speed id 0 */
    if ((pll_div == APERTA_TBHMOD_PLL_MODE_DIV_170) || (pll_div == APERTA_TBHMOD_PLL_MODE_DIV_85))  {
        spd_intf = APERTA_TBHMOD_SPD_50G_IEEE_KR1_CR1;
    } else if ((pll_div == APERTA_TBHMOD_PLL_MODE_DIV_165) || (pll_div == APERTA_TBHMOD_PLL_MODE_DIV_82P5)) {
        spd_intf = APERTA_TBHMOD_SPD_25000_XFI;
    } else {
        spd_intf = APERTA_TBHMOD_SPD_10000_XFI;
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_get_mapped_speed(spd_intf, &mapped_speed_id));

    /*next read the speed id entry and then copy to the right forced speed */
    phy_copy.access.lane_mask = 1 << 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_mem_read(&phy_copy, phymodMemSpeedIdTable, mapped_speed_id, packed_entry));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_mem_write(&phy_copy, phymodMemSpeedIdTable, APERTA_TSCBH_FORCED_SPEED_ID_OFFSET + start_lane, packed_entry));

    /* next update the port_mode */
    phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_update_port_mode(&phy_copy));

    /* add rx lock override */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_pmd_rx_lock_override_enable(&phy_copy, 1));
    /* clear state machine state */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_read_sc_fsm_status(&phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_read_sc_done(&phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_enable_set(&phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_polling_for_sc_done(&phy_copy));
    /* disable pcs again */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_disable_set(&phy_copy));
    PHYMOD_USLEEP(10000);
    /* disable rx lock override */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_tbhmod_pmd_rx_lock_override_enable(&phy_copy, 0));

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_pcs_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t enable)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int start_lane, num_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    if (enable == 1) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_enable_set(&phy_copy));
    } else if (enable == 0) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta_tbhmod_disable_set(&phy_copy));
    } else {
        /* this is the SW WAR for the 16nm flexport HW issue */
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta_tscbh_flexport_sw_workaround(phy));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_phy_rx_ppm_get(const plp_aperta_phymod_phy_access_t* phy, int16_t* rx_ppm)
{
    int start_lane, num_lane;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    pm_phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_rx_ppm(&pm_phy_copy, rx_ppm));

    return PHYMOD_E_NONE;
}

int plp_aperta_tscbh_reset_pcs(const plp_aperta_phymod_phy_access_t* phy, int datarate, int quad) 
{
    MAIN0_SETUPr_t mode_reg_Q0;
    plp_aperta_phymod_phy_access_t phy_copy;
    BCMI_TSCBH_XGXS_SC_X4_CTLr_t  x4_ctrl;
    int side = 0;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    phy_copy.access.lane_mask = 0xF;
    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        phy_copy.port_loc = side;
        PHYMOD_IF_ERR_RETURN(
                READ_MAIN0_SETUPr(&phy_copy, &mode_reg_Q0));
        if (quad & 1) {
            phy_copy.access.lane_mask = 1;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_disable_set(&phy_copy));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_set_sc_speed(&phy_copy, 0, 0));
            phy_copy.access.lane_mask = 0xF;
            MAIN0_SETUPr_PORT_MODE_SELf_SET(mode_reg_Q0, 0);
            PHYMOD_IF_ERR_RETURN(
                    WRITE_MAIN0_SETUPr(&phy_copy, mode_reg_Q0));
            phy_copy.access.lane_mask = 1;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_enable_set(&phy_copy));
        }

        phy_copy.access.lane_mask = 0xF0;
        PHYMOD_IF_ERR_RETURN(
                READ_MAIN0_SETUPr(&phy_copy, &mode_reg_Q0));

        if (quad & 2) {
            int is_q2_disabled = 0;
            /* In case of init with 100G/40G mode
             * clear the speed id*/
            phy_copy.access.lane_mask = 0x10;
            PHYMOD_IF_ERR_RETURN(
                READ_SC_X4_CTLr(&phy_copy, &x4_ctrl));
            if (x4_ctrl.v[0] & 0xFF) {
                x4_ctrl.v[0] &= ~(0xFF);
                PHYMOD_IF_ERR_RETURN(
                    WRITE_SC_X4_CTLr(&phy_copy, x4_ctrl));
            }
            phy_copy.access.lane_mask = 0x40;
            PHYMOD_IF_ERR_RETURN(
                READ_SC_X4_CTLr(&phy_copy, &x4_ctrl));
            if (x4_ctrl.v[0] & 0xFF) {
                x4_ctrl.v[0] &= ~(0xFF);
                PHYMOD_IF_ERR_RETURN(
                    WRITE_SC_X4_CTLr(&phy_copy, x4_ctrl));
            }


            phy_copy.access.lane_mask = 0x10;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta_tbhmod_disable_set(&phy_copy));

            /* Disabled Q2 only if enabled*/  
            if (x4_ctrl.v[0] & 0x100) { 
                phy_copy.access.lane_mask = 0x40;
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_tbhmod_disable_set(&phy_copy));
                is_q2_disabled = 1;
            }

            phy_copy.access.lane_mask = 0xF0;
            MAIN0_SETUPr_PORT_MODE_SELf_SET(mode_reg_Q0, 0);
            PHYMOD_IF_ERR_RETURN(
                    WRITE_MAIN0_SETUPr(&phy_copy, mode_reg_Q0));
            if (datarate != 400000) {
                phy_copy.access.lane_mask = 0x10;
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta_tbhmod_enable_set(&phy_copy));
                if (is_q2_disabled) {
                    /* re-enable Q2*/
                    phy_copy.access.lane_mask = 0x40;
                    PHYMOD_IF_ERR_RETURN
                       (plp_aperta_tbhmod_enable_set(&phy_copy));
                }
            }
        }
    }
    return 0;
}

int plp_aperta_tscbh_timesync_tx_info_get(const plp_aperta_phymod_phy_access_t* phy, tbhmod_ts_tx_info_t* ts_tx_info)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    aperta_tbhmod_ts_tx_info_t local_ts_tx_info;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_MEMSET(&local_ts_tx_info, 0, sizeof(aperta_tbhmod_ts_tx_info_t));

    PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_1588_tx_info_get(&phy_copy, &local_ts_tx_info));

    ts_tx_info->ts_in_fifo_lo = ((uint32_t)(local_ts_tx_info.ts_val_mid << 16)) | ((uint32_t)local_ts_tx_info.ts_val_lo);
    ts_tx_info->ts_in_fifo_hi = (uint32_t)local_ts_tx_info.ts_val_hi;
    ts_tx_info->ts_seq_id = (uint32_t)local_ts_tx_info.ts_seq_id;
    ts_tx_info->ts_sub_nanosec = (uint32_t)local_ts_tx_info.ts_sub_nanosec;

    return PHYMOD_E_NONE;
}

