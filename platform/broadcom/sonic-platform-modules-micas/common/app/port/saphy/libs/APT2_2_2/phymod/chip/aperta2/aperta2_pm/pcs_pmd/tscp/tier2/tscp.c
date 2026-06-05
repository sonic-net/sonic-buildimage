/*
 *
 * $Id: phymod.xml,v 1.1.2.5 2013/09/12 10:43:06 nirf Exp $
 *
 * $Copyright:
 * All Rights Reserved.$
 *
 *
 */

#include <phymod/phymod.h>
#include <phymod/phymod_system.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_dispatch.h>
#include <tscp.h>
#include <bcmi_tscp_xgxs_defs.h>
#include <peregrine5_pc.h>
#include "tscp/tier1/tscpmod.h"
#include "tscp/tier1/tscpmod_sc_lkup_table.h"
#include "tscp/tier1/tscpmod_1588_lkup_table.h"
#include "peregrine5_pc/tier2/peregrine5_pc_ucode.h"
#include <peregrine5_pc_config.h>
#include <peregrine5_pc_cfg_seq.h>
#include <peregrine5_pc_enum.h>
#include <peregrine5_pc_common.h>
#include <peregrine5_pc_interface.h>
#include <peregrine5_pc_dependencies.h>
#include <peregrine5_pc_internal.h>
#include <peregrine5_pc_functions.h>
#include "public/peregrine5_api_uc_vars_rdwr_defns_public.h"
#include "peregrine5_pc_access.h"
#include <aperta2_pm_seq.h>
#include <aperta2_reg_access.h>

#define PMDCODE 1

#define TSCP_SERDES_ID           0x2d /* 0x9008 Main0_serdesID - Serdes ID Register */

#define TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_KR4     0x6
#define TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_CR4     0x7

#define PEREGRINE_AGING_SUPPORT 0


int plp_aperta2_tscp_core_identify(const plp_aperta2_phymod_core_access_t* core, uint32_t core_id,
                       uint32_t* is_identified)
{
    PHYID2r_t id2;
    PHYID3r_t id3;
    MAIN0_SERDESIDr_t serdesid;
    int ioerr = 0;

    *is_identified = 0;
    ioerr += READ_PHYID2r((plp_aperta2_phymod_phy_access_t*)&core, &id2);
    ioerr += READ_PHYID3r((plp_aperta2_phymod_phy_access_t*)&core, &id3);

    if (PHYID2r_REGID1f_GET(id2) == TSCPMOD_ID0 &&
       (PHYID3r_REGID2f_GET(id3) == TSCPMOD_ID1)) {
        /* PHY IDs match - now check PCS model */
        ioerr += READ_MAIN0_SERDESIDr((plp_aperta2_phymod_phy_access_t*)&core, &serdesid);
        if ( (MAIN0_SERDESIDr_MODEL_NUMBERf_GET(serdesid)) == TSCP_SERDES_ID)  {
            *is_identified = 1;
        }
    }
    return ioerr ? PHYMOD_E_IO : PHYMOD_E_NONE;
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_core_info_get(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_core_info_t* info)
{
    int rv = 0;
    MAIN0_SERDESIDr_t serdes_id;
    PHYID2r_t id2;
    PHYID3r_t id3;

    rv = READ_MAIN0_SERDESIDr((plp_aperta2_phymod_phy_access_t*)&core, &serdes_id);

    info->serdes_id = MAIN0_SERDESIDr_GET(serdes_id);
    info->serdes_id = MAIN0_SERDESIDr_GET(serdes_id);
    info->core_version = 0;

    PHYMOD_IF_ERR_RETURN(READ_PHYID2r((plp_aperta2_phymod_phy_access_t*)&core, &id2));
    PHYMOD_IF_ERR_RETURN(READ_PHYID3r((plp_aperta2_phymod_phy_access_t*)&core, &id3));

    info->phy_id0 = (uint16_t) id2.v[0];
    info->phy_id1 = (uint16_t) id3.v[0];

    return rv;
}

int plp_aperta2_tscp_phy_firmware_lane_config_get(const plp_aperta2_phymod_phy_access_t* phy,
                                      phymod_firmware_lane_config_t* fw_config)
{
    /* fixe me later */
#if PMDCODE
    struct peregrine5_pc_uc_lane_config_st lane_config;
    enum peregrine5_pc_force_cdr_mode_enum cdr_mode = 0;
    enum peregrine5_pc_rx_pam_mode_enum pmode = NRZ;
    plp_aperta2_phymod_phy_access_t pm_phy_copy;
    int lp_has_precoder;
    uint32_t cl72_en;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_MEMSET(&lane_config, 0x0, sizeof(lane_config));
    PHYMOD_MEMSET(fw_config, 0, sizeof(*fw_config));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_get_uc_lane_cfg(&pm_phy_copy, &lane_config));

    fw_config->LaneConfigFromPCS     = lane_config.field.lane_cfg_from_pcs;
    fw_config->AnEnabled             = lane_config.field.an_enabled;
    fw_config->DfeOn                 = lane_config.field.dfe_on;
    fw_config->LpDfeOn               = lane_config.field.rx_low_power;
    fw_config->ForceBrDfe            = lane_config.field.force_cdr_mode;
    fw_config->MediaType             = lane_config.field.media_type;
    fw_config->UnreliableLos         = lane_config.field.unreliable_los;
    fw_config->Cl72AutoPolEn         = lane_config.field.linktrn_auto_polarity_en;
    fw_config->Cl72RestTO            = lane_config.field.linktrn_restart_timeout_en;
    fw_config->ForceES     = lane_config.field.force_er;
    fw_config->ForceNS      = lane_config.field.force_nr;
    fw_config->LinkPartnerPrecoderEn     = lane_config.field.lp_has_prec_en;
    fw_config->ForcePAM4Mode         = lane_config.field.force_pam4_mode;
    fw_config->ForceNRZMode          = lane_config.field.force_nrz_mode;

    /* need to check rx PAM4 ER mode */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(&pm_phy_copy, &pmode));
    fw_config->ForceES = (pmode == PAM4_ER) ? 1 : 0;
    fw_config->ForceNS = (pmode == PAM4_NR) ? 1 : 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_get_force_lane_cdr_mode(&pm_phy_copy, &cdr_mode));
    if (cdr_mode == PEREGRINE5_PC_OSCDR_FORCE_ENABLE) {
        fw_config->ForceOsCdr = 1;
        fw_config->ForceBrDfe = 0;
    } else if (cdr_mode == PEREGRINE5_PC_BRCDR_FORCE_ENABLE) {
        fw_config->ForceOsCdr = 0;
        fw_config->ForceBrDfe = 1;
    } else {
        fw_config->ForceOsCdr = 0;
        fw_config->ForceBrDfe = 0;
    }

    /*read lp_has_preocder_en from register when cl72 en*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_clause72_control_get(&pm_phy_copy, &cl72_en));
    if (lane_config.field.force_pam4_mode && cl72_en && (PAM4_ER == pmode)) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_pam4_lp_has_precoder_enable_get(&pm_phy_copy, &lp_has_precoder));
        fw_config->LinkPartnerPrecoderEn = lp_has_precoder;
    }
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_tx_lane_control_set(const plp_aperta2_phymod_phy_access_t* phy,
                                 plp_aperta2_phymod_phy_tx_lane_control_t tx_control)
{
#if PMDCODE
    phymod_firmware_lane_config_t fw_lane_config;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_IF_ERR_RETURN (plp_aperta2_tscp_phy_firmware_lane_config_get(phy, &fw_lane_config));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    switch (tx_control) {
        case phymodTxElectricalIdleEnable:
            /* idle is the same as tx disable */
            if (fw_lane_config.LaneConfigFromPCS == 0) {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable(&phy_copy, 1));
            } else {
                return PHYMOD_E_PARAM;
            }
            break;
        case phymodTxElectricalIdleDisable:
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable(&phy_copy, 0));
            break;
        case phymodTxSquelchOn:
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable(&phy_copy, 1));
            break;
        case phymodTxSquelchOff:
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable(&phy_copy, 0));
            break;
        default:
            return PHYMOD_E_PARAM;
    }
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_tx_lane_control_get(const plp_aperta2_phymod_phy_access_t* phy,
                                 plp_aperta2_phymod_phy_tx_lane_control_t *tx_control)
{
#if PMDCODE
    uint8_t tx_disable;
    uint32_t lb_enable;
    plp_aperta2_phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    /* next program the tx fir taps and driver current based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    pm_phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable_get(&pm_phy_copy, &tx_disable));

    /* next check if PMD loopback is on */
    if (tx_disable) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_dig_lpbk_get(&pm_phy_copy, &lb_enable));
        if (lb_enable) tx_disable = 0;
    }

    if (tx_disable) {
        *tx_control = phymodTxSquelchOn;
    } else {
        *tx_control = phymodTxSquelchOff;
    }
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_rx_lane_control_set(const plp_aperta2_phymod_phy_access_t* phy,
                                 plp_aperta2_phymod_phy_rx_lane_control_t rx_control)
{
#if PMDCODE
    plp_aperta2_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    switch (rx_control) {
        case phymodRxSquelchOn:
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_force_signal_detect(&pm_phy_copy, 1, 0));
            break;
        case phymodRxSquelchOff:
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_force_signal_detect(&pm_phy_copy, 0, 0));
            break;
        default:
            return PHYMOD_E_PARAM;
    }
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_rx_lane_control_get(const plp_aperta2_phymod_phy_access_t* phy,
                                 plp_aperta2_phymod_phy_rx_lane_control_t* rx_control)
{
#if PMDCODE
    int rx_squelch_enable;
    uint32_t lb_enable;
    uint8_t force_en, force_val;
    plp_aperta2_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    /* first get the force enabled bit and forced value */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_force_signal_detect_get(&pm_phy_copy, &force_en, &force_val));

    if (force_en & (!force_val)) {
        rx_squelch_enable = 1;
    } else {
        rx_squelch_enable = 0;
    }

    /* next check if PMD loopback is on */
    if (rx_squelch_enable) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_dig_lpbk_get(&pm_phy_copy, &lb_enable));
        if (lb_enable) rx_squelch_enable = 0;
    }
    if(rx_squelch_enable) {
        *rx_control = phymodRxSquelchOn;
    } else {
        *rx_control = phymodRxSquelchOff;
    }
#endif
    return PHYMOD_E_NONE;
}


#ifdef APERTA2_NO_FW
#if PMDCODE
STATIC
int _tscp_core_firmware_load(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config)
{
    plp_aperta2_phymod_core_access_t  core_copy;
    plp_aperta2_phymod_phy_access_t phy_access;
    unsigned int osprey_ucode_len;
    unsigned char *osprey_ucode;

    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));
    TSCPMOD_CORE_TO_PHY_ACCESS(&phy_access, core);

    phy_access.access.lane_mask = 0x1;
    osprey_ucode = plp_aperta2_peregrine5_pc_ucode_get();
    osprey_ucode_len = PEREGRINE5_PC_UCODE_IMAGE_SIZE;

    switch(init_config->firmware_load_method){
    case phymodFirmwareLoadMethodInternal:
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_ucode_load(&core_copy.access, osprey_ucode, osprey_ucode_len));
        break;
    case phymodFirmwareLoadMethodExternal:
        PHYMOD_NULL_CHECK(init_config->firmware_loader);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_ucode_load_init(&core_copy.access, 1));
        PHYMOD_IF_ERR_RETURN(init_config->firmware_loader(core, osprey_ucode_len, osprey_ucode));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_ucode_load_close(&core_copy.access, 1));
        break;
    case phymodFirmwareLoadMethodNone:
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("illegal fw load method %u"), init_config->firmware_load_method));
    }
    return PHYMOD_E_NONE;
}
#endif
#endif


int plp_aperta2_tscp_phy_tx_set(const plp_aperta2_phymod_phy_access_t* phy, const phymod_pam4_tx_t* tx)
{
#if PMDCODE
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i, port_start_lane, port_num_lane;
    enum peregrine5_pc_txfir_tap_enable_enum enable_taps = PEREGRINE5_PC_NRZ_6TAP;
    uint32_t lane_reset, pcs_lane_enable, port_lane_mask = 0;

    /*
     * Extra bit could be set in phy->access.lane_mask as the special flag.
     * It is used in plp_aperta2_tscpmod_port_start_lane_get() in order to get the correct
     * port_start_lane for 4-lane port or 8-lane port.
     * The extra bit is used in plp_aperta2_tscpmod_port_start_lane_get() only,
     * and it should be cleared before calling other functions.
     */
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* clear the special bit, only bit 0-7 are cared 
    phy_copy.access.lane_mask &= 0xff;*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy_copy.access, &start_lane, &num_lane));

    /*get the start lane of the port lane mask */
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_start_lane_get(&phy_copy, &port_start_lane, &port_num_lane));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_mask_get(port_start_lane, port_num_lane, &port_lane_mask));

    phy_copy.access.lane_mask = 1 << port_start_lane;

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_enable_get(&phy_copy, &pcs_lane_enable));

    /*first check if lane is in reset */
    phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_lane_soft_reset_get(&phy_copy, &lane_reset));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << port_start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_disable_set(&phy_copy));
    }
    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        /* clear the special bit, only bit 0-7 are cared */
        /*phy_copy.access.lane_mask &= 0xff;*/
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 1));
    }

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP ||
                tx->serdes_tx_tap_mode == phymodTxTapModePAM4_LP_3TAP) {
            if (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP) {
                enable_taps = PEREGRINE5_PC_NRZ_LP_3TAP;
            } else {
                enable_taps = PEREGRINE5_PC_PAM4_LP_3TAP;
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_apply_txfir_cfg(&phy_copy,
                                                 enable_taps,
                                                 tx->pre3,
                                                 0,
                                                 tx->pre,
                                                 tx->main,
                                                 tx->post,
                                                 0));
        } else {
            if (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_6TAP) {
                enable_taps= PEREGRINE5_PC_NRZ_6TAP;
            } else {
                enable_taps= PEREGRINE5_PC_PAM4_6TAP;
            }

            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_apply_txfir_cfg(&phy_copy,
                                                 enable_taps,
                                                 tx->pre3,
                                                 tx->pre2,
                                                 tx->pre,
                                                 tx->main,
                                                 tx->post,
                                                 tx->post2));
        }
    }

    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        /* clear the special bit, only bit 0-7 are cared 
        phy_copy.access.lane_mask &= 0xff;*/
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << port_start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_enable_set(&phy_copy));
    }
#endif
    return PHYMOD_E_NONE;
}


int plp_aperta2_tscp_core_lane_map_set(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_lane_map_t* lane_map)
{
    uint32_t lane, pcs_tx_swap = 0, pcs_rx_swap = 0;
    uint8_t pmd_tx_addr[8], pmd_rx_addr[8];
    plp_aperta2_phymod_core_access_t  core_copy;

    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));

    if (lane_map->num_of_lanes != TSCPMOD_NOF_LANES_IN_CORE){
        return PHYMOD_E_CONFIG;
    }

    for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane++){
        if ((lane_map->lane_map_tx[lane] >= TSCPMOD_NOF_LANES_IN_CORE)||
             (lane_map->lane_map_rx[lane] >= TSCPMOD_NOF_LANES_IN_CORE)){
            return PHYMOD_E_CONFIG;
        }
        /*encode each lane as four bits*/
        pcs_tx_swap += lane_map->lane_map_tx[lane]<<(lane*4);
        pcs_rx_swap += lane_map->lane_map_rx[lane]<<(lane*4);
    }
    /* PMD lane addr is based on PCS logical to physical mapping*/
    for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane++){
        pmd_tx_addr[((pcs_tx_swap >> (lane*4)) & 0xf)] = lane;
        pmd_rx_addr[((pcs_rx_swap >> (lane*4)) & 0xf)] = lane;
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pcs_tx_lane_swap((plp_aperta2_phymod_phy_access_t*)&core_copy, pcs_tx_swap));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pcs_rx_lane_swap((plp_aperta2_phymod_phy_access_t*)&core_copy, pcs_rx_swap));

    COMPILER_REFERENCE(pmd_tx_addr);
    COMPILER_REFERENCE(pmd_rx_addr);
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_map_lanes((plp_aperta2_phymod_phy_access_t*)&core_copy, TSCPMOD_NOF_LANES_IN_CORE, pmd_tx_addr, pmd_rx_addr));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_core_lane_map_get(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_lane_map_t* lane_map)
{
#if PMDCODE
    int i = 0, octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(core->access.lane_mask);
    uint32_t tx_lane_map = 0, rx_lane_map = 0;
    plp_aperta2_phymod_core_access_t  core_copy;

    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));
    core_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_pmd_lane_map_get((plp_aperta2_phymod_phy_access_t*)&core_copy, &tx_lane_map, &rx_lane_map));

    /* Get the lane map into serdes api format */
    lane_map->num_of_lanes = TSCPMOD_NOF_LANES_IN_CORE;
    for (i = 0; i < TSCPMOD_NOF_LANES_IN_CORE; i++) {
        lane_map->lane_map_tx[tx_lane_map >> (4 * i) & 0xf] = i;
        lane_map->lane_map_rx[rx_lane_map >> (4 * i) & 0xf] = i;
    }
#endif
    return PHYMOD_E_NONE;
}

static int
_plp_aperta2_tscp_phy_lane_swap_validate(const plp_aperta2_phymod_phy_access_t* phy)
{
    int i;
    plp_aperta2_phymod_lane_map_t lane_map;
    plp_aperta2_phymod_core_access_t core;

    PHYMOD_MEMCPY(&core, phy, sizeof(plp_aperta2_phymod_core_access_t));
    PHYMOD_MEMSET(&lane_map, 0x0, sizeof(lane_map));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscp_core_lane_map_get(&core, &lane_map));

    for (i = 0; i < TSCPMOD_NOF_LANES_IN_CORE; i++) {
         if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, i)) {
             continue;
         }

         if (lane_map.lane_map_tx[i] != lane_map.lane_map_rx[i]) {
             PHYMOD_DIAG_OUT(("Warning: core_addr 0x%x, TX lane %d is mapped to %d, while RX lane %d is mapped to %d!\n",
                                core.access.addr, i, lane_map.lane_map_tx[i], i, lane_map.lane_map_rx[i]));
             return TRUE;
         }
    }

    return FALSE;
}

static 
int _plp_aperta2_tscp_phy_firmware_lane_config_set(const plp_aperta2_phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_config)
{
#if PMDCODE
    uint32_t is_warm_boot;
    struct peregrine5_pc_uc_lane_config_st serdes_firmware_config;
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, i;

    PHYMOD_MEMSET(&serdes_firmware_config, 0x0, sizeof(serdes_firmware_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        serdes_firmware_config.field.lane_cfg_from_pcs      = fw_config.LaneConfigFromPCS;
        serdes_firmware_config.field.an_enabled             = fw_config.AnEnabled;
        serdes_firmware_config.field.dfe_on                 = fw_config.DfeOn;
        serdes_firmware_config.field.force_cdr_mode         = fw_config.ForceBrDfe;
        serdes_firmware_config.field.unreliable_los         = fw_config.UnreliableLos;
        serdes_firmware_config.field.media_type             = fw_config.MediaType;
        serdes_firmware_config.field.rx_low_power            = fw_config.LpDfeOn;
        serdes_firmware_config.field.linktrn_auto_polarity_en  = fw_config.Cl72AutoPolEn;
        serdes_firmware_config.field.linktrn_restart_timeout_en = fw_config.Cl72RestTO;
        serdes_firmware_config.field.force_er               = fw_config.ForceES;
        serdes_firmware_config.field.force_nr               = fw_config.ForceNS;
        serdes_firmware_config.field.force_nrz_mode         = fw_config.ForceNRZMode;
        serdes_firmware_config.field.force_pam4_mode        = fw_config.ForcePAM4Mode;
        serdes_firmware_config.field.lp_has_prec_en         = fw_config.LinkPartnerPrecoderEn;
        PHYMOD_IF_ERR_RETURN(PHYMOD_IS_WRITE_DISABLED(&phy->access, &is_warm_boot));
        if (!is_warm_boot) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_set_uc_lane_cfg(&phy_copy, serdes_firmware_config));
            if (fw_config.ForceBrDfe) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_force_lane_cdr_mode(&phy_copy, PEREGRINE5_PC_BRCDR_FORCE_ENABLE));
            } else if (fw_config.ForceOsCdr) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_force_lane_cdr_mode(&phy_copy, PEREGRINE5_PC_OSCDR_FORCE_ENABLE));
            } else if ((fw_config.ForceOsCdr == 0) && (fw_config.ForceBrDfe == 0)) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_force_lane_cdr_mode(&phy_copy, PEREGRINE5_PC_CDR_FORCE_DISABLE));
            }
        }
    }

#endif
    return PHYMOD_E_NONE;
}


int plp_aperta2_tscp_phy_firmware_lane_config_set(const plp_aperta2_phymod_phy_access_t* phy,
                                      phymod_firmware_lane_config_t fw_config)
{
#if PMDCODE
    plp_aperta2_phymod_phy_access_t phy_copy;
    uint32_t lane_reset, pcs_lane_enable, port_lane_mask = 0;
    int start_lane, num_lane, port_start_lane, port_num_lane;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(phy->access.lane_mask);
    uint32_t reset_state = 0, cnt = 0;

    /*
     * Extra bit could be set in phy->access.lane_mask as the special flag.
     * It is used in plp_aperta2_tscpmod_port_start_lane_get() in order to get the correct
     * port_start_lane for 4-lane port or 8-lane port.
     * The extra bit is used in plp_aperta2_tscpmod_port_start_lane_get() only,
     * and it should be cleared before calling other functions.
     */
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* clear the special bit, only bit 0-7 are cared */
    /*phy_copy.access.lane_mask &= (0xff << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /*get the start lane of the port lane mask */
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_start_lane_get(&phy_copy, &port_start_lane, &port_num_lane));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_mask_get(port_start_lane, port_num_lane, &port_lane_mask));

    /* need to check if both FORCE os/BR cdr bit are set */
    if ((fw_config.ForceBrDfe) && (fw_config.ForceOsCdr)) {
        PHYMOD_DEBUG_ERROR(("force both OS/BR CDR mode is NOT supported\n"));
        return PHYMOD_E_UNAVAIL;
    }

    /*first check if lane is in reset */
    phy_copy.access.lane_mask &= (0xff << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
    /* clear the special bit, only bit 0-7 are cared */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_lane_soft_reset_get(&phy_copy, &lane_reset));

    /*next check if PCS lane is in reset */
    phy_copy.access.lane_mask = 1 << port_start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_enable_get(&phy_copy, &pcs_lane_enable));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_disable_set(&phy_copy));
    }

    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        /* clear the special bit, only bit 0-7 are cared */
        phy_copy.access.lane_mask &= (0xff << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 1));
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* clear the special bit, only bit 0-7 are cared */
    phy_copy.access.lane_mask &= (0xff<< PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
    PHYMOD_IF_ERR_RETURN
         (_plp_aperta2_tscp_phy_firmware_lane_config_set(&phy_copy, fw_config));

    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        /* clear the special bit, only bit 0-7 are cared */
        phy_copy.access.lane_mask &= (0xff<< PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << port_start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_enable_set(&phy_copy));
        while (cnt < 200) {
           PHYMOD_IF_ERR_RETURN
               (plp_aperta2_peregrine5_pc_lane_dp_reset_state_get(&phy_copy, &reset_state));
           if (!reset_state) {
                break;
           }
           cnt++;
           if(cnt == 200) {
               PHYMOD_DEBUG_ERROR(("WARNING :: 0x%x SerDes lane is reset\n", phy_copy.access.addr));
               break;
           }
           PHYMOD_USLEEP(10);
        }
    }

#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_polarity_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_polarity_t* polarity)
{
#if PMDCODE
    plp_aperta2_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_tx_rx_polarity_set(&phy_copy, polarity->tx_polarity, polarity->rx_polarity));
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_polarity_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_polarity_t* polarity)
{
#if PMDCODE
    int start_lane, num_lane, i;
    plp_aperta2_phymod_polarity_t temp_pol;
    plp_aperta2_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

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
            (plp_aperta2_peregrine5_pc_tx_rx_polarity_get(&phy_copy, &temp_pol.tx_polarity, &temp_pol.rx_polarity));
        polarity->tx_polarity |= ((temp_pol.tx_polarity & 0x1) << i);
        polarity->rx_polarity |= ((temp_pol.rx_polarity & 0x1) << i);
    }
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_port_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    uint32_t pcs_enable;
    int start_lane, num_lane, port_an_enable, port_enable;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /* first read port an enable bit */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_an_mode_enable_get(&phy_copy, &port_an_enable));

    /* next read current port enable bit */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_enable_get(&phy_copy, &port_enable));

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_enable_get(&phy_copy, &pcs_enable));

    if ((port_an_enable) || (port_enable && !pcs_enable)) {
        /* cuurent port is in An mode mode */
        if (enable == 1) {
#if PMDCODE 
            /* next release both tx/rx squelch */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable(&phy_copy, 0));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_force_signal_detect(&phy_copy, 0, 0));
#endif
            phy_copy.access.lane_mask = 1 << start_lane;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_port_cl73_enable_set(&phy_copy, enable));
        /*check if enable ==0 */
        if (!enable) {
#if PMDCODE
            /* next set both tx/rx squelch */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable(&phy_copy, 1));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_force_signal_detect(&phy_copy, 1, 0));
#endif
        }

    } else {
        /* cuurent port is in forced speed mode */
        if (enable == 1) {
#if PMDCODE
            /* next release both tx/rx squelch */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable(&phy_copy, 0));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_force_signal_detect(&phy_copy, 0, 0));
#endif
            phy_copy.access.lane_mask = 1 << start_lane;
            /* enable speed control bit */
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_enable_set(&phy_copy));
        } else if (enable == 0) {
            /* disable speed control bit */
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_disable_set(&phy_copy));
            /* next set both tx/rx squelch */
#if PMDCODE
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable(&phy_copy, 1));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_force_signal_detect(&phy_copy, 1, 0));
#endif
        }
    }

    /* next set port enable bit */
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_enable_set(&phy_copy, enable));


    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_port_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int temp_enable;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* first read port an enable bit */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_enable_get(&phy_copy, &temp_enable));

    *enable = (uint32_t) temp_enable;

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_tx_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_pam4_tx_t* tx)
{
#if PMDCODE
#ifdef APERTA2_NO_USER_TAP_MODE
    uint8_t pmd_tx_tap_mode;
    uint16_t tx_tap_nrz_mode = 0;
#endif
    int16_t val;
    plp_aperta2_phymod_phy_access_t phy_copy;
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

#ifdef APERTA2_NO_USER_TAP_MODE
    /* read current tx tap mode */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_tx_tap_mode_get(&phy_copy, &pmd_tx_tap_mode));

    /*read current tx NRZ mode control info */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_tx_nrz_mode_get(&phy_copy, &tx_tap_nrz_mode));

    if (pmd_tx_tap_mode == 0) {
        /* 3 tap mode */
        if (tx_tap_nrz_mode) {/*NRZ*/
            tx->serdes_tx_tap_mode = phymodTxTapModeNRZ_LP_3TAP;
        } else {
            tx->serdes_tx_tap_mode = phymodTxTapModePAM4_LP_3TAP;
        }
    } else {
        if (tx_tap_nrz_mode) {/*NRZ*/
            tx->serdes_tx_tap_mode = phymodTxTapModeNRZ_6TAP;
        } else {
            tx->serdes_tx_tap_mode = phymodTxTapModePAM4_6TAP;
        }
    }
#endif
    /*next check 3 tap mode or 6 tap mode */
    if (tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP ||
          tx->serdes_tx_tap_mode == phymodTxTapModePAM4_LP_3TAP  ) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_read_tx_afe(&phy_copy, PEREGRINE5_PC_TX_AFE_PRE1, &val));
        tx->pre = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_read_tx_afe(&phy_copy, PEREGRINE5_PC_TX_AFE_MAIN, &val));
        tx->main = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_read_tx_afe(&phy_copy, PEREGRINE5_PC_TX_AFE_POST1, &val));
        tx->post = val;
        tx->pre2 = 0;
        tx->post2 = 0;
        tx->post3 = 0;
        tx->pre3 = 0;
    } else {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_read_tx_afe(&phy_copy, PEREGRINE5_PC_TX_AFE_PRE3, &val));
        tx->pre3 = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_read_tx_afe(&phy_copy, PEREGRINE5_PC_TX_AFE_PRE2, &val));
        tx->pre2 = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_read_tx_afe(&phy_copy, PEREGRINE5_PC_TX_AFE_PRE1, &val));
        tx->pre = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_read_tx_afe(&phy_copy, PEREGRINE5_PC_TX_AFE_MAIN, &val));
        tx->main = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_read_tx_afe(&phy_copy, PEREGRINE5_PC_TX_AFE_POST1, &val));
        tx->post = val;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_read_tx_afe(&phy_copy, PEREGRINE5_PC_TX_AFE_POST2, &val));
        tx->post2 = val;
        tx->post3 = 0;
    }
#endif
    return PHYMOD_E_NONE;
}

/* This function based on num_lane, data_rate and fec_type
 * assign force speed SW speed_id.
 */

STATIC
int _plp_aperta2_tscp_phy_speed_id_set(int num_lane,
                            uint32_t data_rate,
                            phymod_fec_type_t fec_type,
                            tscpmod_spd_intfc_type_t* spd_intf)
{
    if (num_lane == 1) {
        switch (data_rate) {
            case 10000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = TSCPMOD_SPD_10G_IEEE_KR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 25000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = TSCPMOD_SPD_25G_BRCM_NO_FEC_KR1_CR1;
                } else if (fec_type == phymod_fec_CL91) {
                    *spd_intf = TSCPMOD_SPD_25G_BRCM_FEC_528_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 50000:
                if (fec_type == phymod_fec_CL91) {
                    *spd_intf = TSCPMOD_SPD_50G_BRCM_FEC_528_CR1_KR1;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = TSCPMOD_SPD_50G_IEEE_KR1_CR1;
                } else if (fec_type == phymod_fec_RS272) {
                    *spd_intf = TSCPMOD_SPD_50G_BRCM_FEC_272_CR1_KR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 100000:
                if (fec_type == phymod_fec_RS544_2XN) {
                    *spd_intf = TSCPMOD_SPD_100G_IEEE_KR1_CR1_OPT;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = TSCPMOD_SPD_100G_BRCM_KR1_CR1;
                } else if (fec_type == phymod_fec_RS272) {
                    *spd_intf = TSCPMOD_SPD_100G_BRCM_FEC_272_KR1_CR1;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            default:
                return PHYMOD_E_UNAVAIL;
        }
    } else if (num_lane == 2) {
        switch (data_rate) {
            case 50000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = TSCPMOD_SPD_50G_BRCM_CR2_KR2_NO_FEC;
                } else if (fec_type == phymod_fec_CL91) {
                    *spd_intf = TSCPMOD_SPD_50G_BRCM_CR2_KR2_RS_FEC;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = TSCPMOD_SPD_50G_BRCM_FEC_544_CR2_KR2;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 100000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = TSCPMOD_SPD_100G_BRCM_NO_FEC_KR2_CR2;
                } else if (fec_type == phymod_fec_CL91) {
                    *spd_intf = TSCPMOD_SPD_100G_BRCM_FEC_528_KR2_CR2;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = TSCPMOD_SPD_100G_IEEE_KR2_CR2;
                } else if (fec_type == phymod_fec_RS272) {
                    *spd_intf = TSCPMOD_SPD_100G_BRCM_FEC_272_CR2_KR2;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 200000:
                if (fec_type == phymod_fec_RS544_2XN) {
                    *spd_intf = TSCPMOD_SPD_200G_IEEE_KR2_CR2;
                } else if (fec_type == phymod_fec_RS272_2XN) {
                    *spd_intf = TSCPMOD_SPD_200G_BRCM_FEC_272_KR2_CR2;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = TSCPMOD_SPD_200G_BRCM_FEC_544_KR2_CR2;
                } else if (fec_type == phymod_fec_RS272) {
                    *spd_intf = TSCPMOD_SPD_200G_BRCM_FEC_272_N2;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            default:
                return PHYMOD_E_UNAVAIL;
        }
    } else if (num_lane == 4) {
        switch (data_rate) {
            case 100000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = TSCPMOD_SPD_100G_BRCM_NO_FEC_X4;
                } else if (fec_type == phymod_fec_CL91) {
                    *spd_intf = TSCPMOD_SPD_100G_IEEE_KR4;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 200000:
                if (fec_type == phymod_fec_None) {
                    *spd_intf = TSCPMOD_SPD_200G_BRCM_NO_FEC_KR4_CR4;
                } else if (fec_type == phymod_fec_RS544) {
                    *spd_intf = TSCPMOD_SPD_200G_BRCM_KR4_CR4;
                } else if (fec_type == phymod_fec_RS544_2XN) {
                    *spd_intf = TSCPMOD_SPD_200G_IEEE_KR4_CR4;
                } else if (fec_type == phymod_fec_RS272) {
                    *spd_intf = TSCPMOD_SPD_200G_BRCM_FEC_272_N4;
                } else if (fec_type == phymod_fec_RS272_2XN) {
                    *spd_intf = TSCPMOD_SPD_200G_BRCM_FEC_272_CR4_KR4;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            case 400000:
                if (fec_type == phymod_fec_RS544_2XN) {
                    *spd_intf = TSCPMOD_SPD_400G_IEEE_KR4_CR4;
                } else if (fec_type == phymod_fec_RS272_2XN) {
                    *spd_intf = TSCPMOD_SPD_400G_BRCM_FEC_272_KR4_CR4;
                } else {
                    return PHYMOD_E_UNAVAIL;
                }
                break;
            default:
                return PHYMOD_E_UNAVAIL;
        }
    } else if (num_lane == 8) {
        if (data_rate == 800000 && fec_type == phymod_fec_RS544_2XN) {
            *spd_intf = TSCPMOD_SPD_800G_BRCM_KR8_CR8;
        } else if (data_rate == 400000 && fec_type == phymod_fec_RS544_2XN) {
            *spd_intf = TSCPMOD_SPD_400G_BRCM_FEC_KR8_CR8;
        } else if (data_rate == 400000 && fec_type == phymod_fec_RS272_2XN) {
            *spd_intf = TSCPMOD_SPD_400G_BRCM_FEC_272_N8;
        } else {
            return PHYMOD_E_UNAVAIL;
        }
    } else {
        return PHYMOD_E_UNAVAIL;
    }

    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta2_tscp_per_lane_data_rate_get(uint32_t vco_rate, int osr_mode, int is_pam4, uint32_t *data_rate)
{
    switch (vco_rate) {
        case 41250000:
            if (osr_mode == TSCPMOD_OS_MODE_2) {
            /* NRZ speed, if OSR is by 2 */
                *data_rate = 20000;
            } else if (osr_mode == TSCPMOD_OS_MODE_33) {
            /* 1G OSR MODE for AUTONEG/DME */
                *data_rate = 1000;
            } else {
                *data_rate = 0;
            }
            break;
        case 51562500:
            if (is_pam4) {
                if (osr_mode == TSCPMOD_OS_MODE_1) {
                /* PAM4 speed 100G per lane */
                    *data_rate = 100000;
                } else if (osr_mode == TSCPMOD_OS_MODE_2) {
                /* PAM4 speed 50G per lane */
                    *data_rate = 50000;
                } else if (osr_mode == TSCPMOD_OS_MODE_41p25) {
                    /* 1G OSR MODE for AUTONEG/DME */
                    *data_rate = 1000;
                } else {
                    *data_rate = 0;
                }
           } else {
                /* NRZ speed */
                if (osr_mode == TSCPMOD_OS_MODE_2) {
                    *data_rate = 25000;
                } else if (osr_mode == TSCPMOD_OS_MODE_5) {
                    *data_rate = 10000;
                } else if (osr_mode == TSCPMOD_OS_MODE_41p25) {
                    /* 1G OSR MODE for AUTONEG/DME */
                    *data_rate = 1000;
                } else {
                    *data_rate = 0;
                }
            }
            break;
        case 53125000:
            if (is_pam4) {
                /* PAM4 speed */
                if (osr_mode == TSCPMOD_OS_MODE_1) {
                /* PAM4 speed 100G per lane */
                    *data_rate = 100000;
                } else if (osr_mode == TSCPMOD_OS_MODE_2) {
                /* PAM4 speed 50G per lane */
                    *data_rate = 50000;
                } else if (osr_mode == TSCPMOD_OS_MODE_42p5) {
                    /* 1G OSR MODE for AUTONEG/DME */
                    *data_rate = 1000;
                } else {
                    *data_rate = 0;
                }
            } else {
                /* NRZ speed */
                if (osr_mode == TSCPMOD_OS_MODE_2) {
                    *data_rate = 25000;
                } else if (osr_mode == TSCPMOD_OS_MODE_42p5) {
                    /* 1G OSR MODE for AUTONEG/DME */
                    *data_rate = 1000;
                } else {
                    *data_rate = 0;
                }
            }
            break;
        default:
            *data_rate = 0;
    }
    if (*data_rate == 0) {
        PHYMOD_DEBUG_ERROR(("Unsupported data_rate with current VCO %d, OSR_MODE %d\n", vco_rate, osr_mode));
            return PHYMOD_E_UNAVAIL;
    }
    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta2_tscp_speed_table_entry_to_speed_config_get(const plp_aperta2_phymod_phy_access_t *phy,
                                        tscpmod_spd_id_tbl_entry_t *speed_config_entry,
                                        phymod_phy_speed_config_t *speed_config)
{
#if PMDCODE
    uint32_t pll_div;
    enum peregrine5_pc_osr_mode_enum tx_osr_mode, rx_osr_mode;
#endif
    uint32_t  refclk_in_hz, data_rate_lane = 0;
    int osr_mode_value = 0;
    tscpmod_refclk_t ref_clk;
    uint32_t vco_rate = 41250000;
    plp_aperta2_phymod_phy_access_t phy_copy;
    phymod_firmware_lane_config_t firmware_lane_config = {0};
    int num_lane, start_lane, lanes;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
       (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &lanes));

    switch (speed_config_entry->num_lanes) {
        case 0: num_lane = 1;
            break;
        case 1: num_lane = 2;
            break;
        case 2: num_lane = 4;
            break;
        case 3: num_lane = 8;
            break;
        default:
            PHYMOD_DEBUG_ERROR(("Unsupported number of lane \n"));
            return PHYMOD_E_UNAVAIL;
    }

    phy_copy.access.pll_idx = 0;
    PHYMOD_MEMSET(&firmware_lane_config,0,sizeof(firmware_lane_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscp_phy_firmware_lane_config_get(phy, &firmware_lane_config));

    /* get the PLL div from HW */
#if PMDCODE
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_INTERNAL_read_pll_div(&phy_copy, &pll_div));
#endif

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_refclk_get(&phy_copy, &ref_clk));

    if (ref_clk == TSCPMOD_REF_CLK_312P5MHZ) {
        refclk_in_hz = 312500000;
    } else {
        refclk_in_hz = 156250000;
    }

    COMPILER_REFERENCE(refclk_in_hz);

#if PMDCODE
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_get_vco_from_refclk_div(&phy_copy, refclk_in_hz, pll_div, &vco_rate, 0));

    /* rx_osr_mode is used as osr_mode for rate calculation, we don't
      * have a case where tx_osr_mode and rx_osr_mode are different.
      */
     phy_copy.access.lane_mask = 1 << start_lane;
     PHYMOD_IF_ERR_RETURN
         (plp_aperta2_peregrine5_pc_get_osr_mode(&phy_copy, &tx_osr_mode, &rx_osr_mode));
     osr_mode_value = (int)rx_osr_mode;

#endif

    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_tscp_per_lane_data_rate_get(vco_rate, osr_mode_value, firmware_lane_config.ForcePAM4Mode, &data_rate_lane));

    speed_config->data_rate = data_rate_lane * num_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_fec_arch_decode_get(speed_config_entry->fec_arch, &(speed_config->fec_type)));

    return PHYMOD_E_NONE;
}



int plp_aperta2_tscp_phy_speed_config_set(const plp_aperta2_phymod_phy_access_t* phy,
                              const phymod_phy_speed_config_t* speed_config,
                              const phymod_phy_pll_state_t* old_pll_state,
                              phymod_phy_pll_state_t* new_pll_state)
{
    plp_aperta2_phymod_phy_access_t pm_phy_copy;
    uint32_t lane_mask_backup;
    uint32_t tvco_pll_div = TSCPMOD_PLL_MODE_DIV_170, request_pll_div = 0;
    int i, start_lane, num_lane, mapped_speed_id;
    phymod_firmware_lane_config_t firmware_lane_config;
    plp_aperta2_phymod_firmware_core_config_t firmware_core_config;
    tscpmod_spd_intfc_type_t spd_intf = 0;
    tscpmod_refclk_t ref_clk;
    int port_enable, osr_mode;
    uint32_t osr5_is_required = 0, reset_state = 0, cnt = 0;
    uint32_t *plp_aperta2_tscp_spd_id_entry;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(phy->access.lane_mask);


    plp_aperta2_tscp_spd_id_entry = plp_aperta2_tscp_spd_id_entry_get();

    firmware_lane_config = speed_config->pmd_lane_config;
    /*first make sure that tvco pll index is valid */
    if (phy->access.tvco_pll_index > 0) {
        PHYMOD_DEBUG_ERROR(("Unsupported tvco index\n"));
        return PHYMOD_E_UNAVAIL;
    }

    PHYMOD_MEMSET(&firmware_core_config, 0x0, sizeof(firmware_core_config));

    /* Copy the PLL state */
    *new_pll_state = *old_pll_state;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    lane_mask_backup = phy->access.lane_mask;

    /* get current port enable bit */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_enable_get(&pm_phy_copy, &port_enable));

    /* then clear the port an mode enable bit */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_an_mode_enable_set(&pm_phy_copy, 0));

    /* Hold the pcs lane reset 
    pm_phy_copy.access.lane_mask = 1 << start_lane;*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_disable_set(&pm_phy_copy));

    /* write this port forced speed id entry */
    pm_phy_copy.access.lane_mask = 1 << start_lane;
    {
        int sc_speed = (start_lane > 7) ? (start_lane-8) : start_lane;
        PHYMOD_IF_ERR_RETURN
           (plp_aperta2_tscpmod_set_sc_speed(&pm_phy_copy, TSCPMOD_FORCED_SPEED_ID_OFFSET + sc_speed, 0));
    }
    
    /*Hold the per lane PMD soft reset bit*/
    pm_phy_copy.access.lane_mask = lane_mask_backup;
#if PMDCODE
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_lane_soft_reset(&pm_phy_copy, 1));
#endif
    /* update the port mode */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_update_port_mode(&pm_phy_copy, speed_config->data_rate));

    /*for speed mode config set */
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_tscp_phy_speed_id_set(num_lane, speed_config->data_rate,
                                                 speed_config->fec_type, &spd_intf));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_get_mapped_speed(spd_intf, &mapped_speed_id));

    /* Get TVCO because it's not allowed to change during speed set */
    pm_phy_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_INTERNAL_read_pll_div(&pm_phy_copy, &tvco_pll_div));

    /* based on the current TVCO PLL div, decide which copy of speed id entry to load */
    pm_phy_copy.access.lane_mask = 1 << (0 + PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
    {
        int mem_idx = (start_lane > 7) ? (start_lane-8) : start_lane;

        PHYMOD_IF_ERR_RETURN
        (plp_aperta2_mem_write(&pm_phy_copy, phymodMemSpeedIdTable, TSCPMOD_FORCED_SPEED_ID_OFFSET + mem_idx,
                          (plp_aperta2_tscp_spd_id_entry + mapped_speed_id * TSCPMOD_SPEED_ID_ENTRY_SIZE)));
    }

    /* Check the request speed VCO */
    pm_phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_refclk_get(&pm_phy_copy, &ref_clk));

    /* Get requested PLL */
    /*for ethernet speed mode config set */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_plldiv_lkup_get(&pm_phy_copy, mapped_speed_id, ref_clk, &request_pll_div));

    /* for the 10G single lane and 40G 4 lane case, either 41G or 51G VCO can work */
    /* first check if requested 40G VCO is present in the current PLL config, if NOT, need
     to try 51G VCO present or not */
    if (request_pll_div != tvco_pll_div) {
        if  ((int)tvco_pll_div == TSCPMOD_PLL_MODE_DIV_165) {
            request_pll_div = tvco_pll_div;
            osr5_is_required = 1;
        }
    }

    /*next need to set certain firmware lane config to be zero*/
    firmware_lane_config.LaneConfigFromPCS = 0;
    firmware_lane_config.AnEnabled = 0;
   if (speed_config->linkTraining) {
       firmware_lane_config.ForceNS = 0;
       firmware_lane_config.ForceES = 0;
   }
    /* Based on chip team Dfe need to be enabled for every Speed 
     * and every interface*/
    firmware_lane_config.DfeOn = 1;

   for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 0x1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
             (_plp_aperta2_tscp_phy_firmware_lane_config_set(&pm_phy_copy, firmware_lane_config));
    }

    pm_phy_copy.access.lane_mask = lane_mask_backup;
    /* Program OS mode */
    /* need to check if osr5 is required */
    /* Set OS mode. */
    if (osr5_is_required) {
        osr_mode = TSCPMOD_OS_MODE_5;
    } else {
        osr_mode = plp_aperta2_tscpmod_mapped_speed_get_osmode(mapped_speed_id);
    }
    /* if PHYMOD sim is enabled use register based, otherwise
     * use ram based ctrl
     */
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_set_osr_mode(&pm_phy_copy, (enum peregrine5_pc_osr_mode_enum)osr_mode));
    }

    /* next need to enable/disable link training based on the input */
#if PMDCODE
    if (speed_config->linkTraining) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscp_phy_cl72_set(phy, speed_config->linkTraining));
    } else {
        /* disable cl72 and avoid overwriting the value from above _firmware_lane_config_set */
        for (i = 0; i < num_lane; i++) {
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_clause72_control(&pm_phy_copy, speed_config->linkTraining));
        }
    }

    /*release the lane soft reset bit*/
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_lane_soft_reset(&pm_phy_copy, 0));
#endif

    /* Release the pcs lane reset */
    if (port_enable) {
        pm_phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_enable_set(&pm_phy_copy));
        while ( cnt < 200) {
           PHYMOD_IF_ERR_RETURN
               (plp_aperta2_peregrine5_pc_lane_dp_reset_state_get(&pm_phy_copy, &reset_state));
           if (!reset_state) {
                break;
           }
           cnt++;
           if(cnt == 200) {
               PHYMOD_DEBUG_ERROR(("WARNING :: 0x%x SerDes lane is reset\n", pm_phy_copy.access.addr));
               break;
           }
           PHYMOD_USLEEP(10);
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_speed_config_get(const plp_aperta2_phymod_phy_access_t* phy,
                              phymod_phy_speed_config_t* speed_config)
{
#if PMDCODE
    uint32_t cl72_enable = 0;
#endif
    plp_aperta2_phymod_phy_access_t phy_copy;
    phymod_firmware_lane_config_t firmware_lane_config = {0};
    int start_lane, num_lane, speed_id;
    uint32_t packed_entry[20];
    tscpmod_spd_id_tbl_entry_t speed_config_entry;
    int an_en, an_done, octal =  PHYMOD_APERTA2_TSCP_GET_OCTAL(phy->access.lane_mask);;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMSET(&firmware_lane_config,0,sizeof(firmware_lane_config));
#if PMDCODE
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscp_phy_firmware_lane_config_get(phy, &firmware_lane_config));

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

#endif
    /* for ethernet port */
    if (!PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_GET(phy->device_op_mode)) {
        /* first read speed id from resolved status */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_speed_id_get(&phy_copy, &speed_id));

        /* next check check if AN enabled */
        PHYMOD_IF_ERR_RETURN
           (plp_aperta2_tscpmod_autoneg_status_get(&phy_copy, &an_en, &an_done));

        /* first read the speed entry and then decode the speed and FEC type */
        phy_copy.access.lane_mask = 1 << (0+PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));

        /*decode speed entry */
        plp_aperta2_tscpmod_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);


        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscp_speed_table_entry_to_speed_config_get(phy, &speed_config_entry, speed_config));

        /* if autoneg enabled, needs to update the FEC_ARCH based on the An resolved status */
        if (an_en && an_done) {
            uint32_t fec_align_live = 1;
            phy_copy.access.lane_mask = 0x1 << start_lane;

            /* For 100G 4 lane NO FEC the AN resolved status is not correct.
             * Hence check if FEC align indeed happened to distinguish NO FEC case*/
            if ((speed_id == TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_KR4) ||
                (speed_id == TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_CR4)) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_tscpmod_fec_align_status_get(&phy_copy, &fec_align_live));
                if (!fec_align_live) {
                    /* Case of 100G 4 LANE with NO FEC */
                    speed_config->fec_type = phymod_fec_None;
                }
            }
        }
    }

    /* next get the cl72 enable status */
#if PMDCODE
    phy_copy.access.lane_mask = 0x1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_clause72_control_get(&phy_copy, &cl72_enable));
    speed_config->linkTraining = cl72_enable;
#endif

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_cl72_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t cl72_en)
{
    struct peregrine5_pc_uc_lane_config_st serdes_firmware_config;
    phymod_firmware_lane_config_t firmware_lane_config;
    int start_lane, num_lane, i, precoder_en;
    uint32_t lane_reset, pcs_lane_enable;
    plp_aperta2_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_get_uc_lane_cfg(&pm_phy_copy, &serdes_firmware_config));

    if ((serdes_firmware_config.field.dfe_on == 0) && cl72_en) {
      PHYMOD_DEBUG_ERROR(("ERROR :: DFE is off : Can not start CL72/CL93 with no DFE\n"));
      return PHYMOD_E_CONFIG;
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_lane_soft_reset_get(&pm_phy_copy, &lane_reset));

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_enable_get(&pm_phy_copy, &pcs_lane_enable));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        pm_phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_disable_set(&pm_phy_copy));
    }

    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&pm_phy_copy, 1));
    }

    /* next need to clear both force ER and NR config on the firmware lane config side
    if link training enable is set */
    if (cl72_en) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscp_phy_firmware_lane_config_get(phy, &firmware_lane_config));

        firmware_lane_config.ForceNS = 0;
        firmware_lane_config.ForceES = 0;

         PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscp_phy_firmware_lane_config_set(phy, firmware_lane_config));
    } else {
        /* disable Tx pre-coding and set Rx in NR mode */
        for (i = 0; i < num_lane; i++) {
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            precoder_en = 0;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscp_phy_tx_pam4_precoder_enable_get(&pm_phy_copy, &precoder_en));
            if (precoder_en) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_tscp_phy_tx_pam4_precoder_enable_set(&pm_phy_copy, 0));
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscp_phy_firmware_lane_config_get(&pm_phy_copy, &firmware_lane_config));
            if (firmware_lane_config.ForcePAM4Mode) {
                firmware_lane_config.ForceNS = 1;
                firmware_lane_config.ForceES = 0;
                PHYMOD_IF_ERR_RETURN
                    (_plp_aperta2_tscp_phy_firmware_lane_config_set(&pm_phy_copy, firmware_lane_config));
            }
        }
    }
    for (i = 0; i < num_lane; i++) {
        pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_clause72_control(&pm_phy_copy, cl72_en));
    }

    /* release the ln dp reset */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&pm_phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        pm_phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_enable_set(&pm_phy_copy));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_cl72_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* cl72_en)
{
    plp_aperta2_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_clause72_control_get(&pm_phy_copy, cl72_en));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_cl72_status_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_cl72_status_t* status)
{
    int i;
    uint32_t tmp_status;
    int start_lane, num_lane;
    plp_aperta2_phymod_phy_access_t phy_copy;
    int an_en, an_done, speed_id;
    tscpmod_spd_id_tbl_entry_t speed_config_entry;
    uint32_t packed_entry[5];

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    status->locked = 1;

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    /* check check if AN enabled */
    PHYMOD_IF_ERR_RETURN
       (plp_aperta2_tscpmod_autoneg_status_get(&phy_copy, &an_en, &an_done));

    if (an_en && an_done) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_speed_id_get(&phy_copy, &speed_id));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));
        plp_aperta2_tscpmod_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);
        /* Update num_lane for AN port */
        num_lane = 1 << speed_config_entry.num_lanes;
    }

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_clause72_control_get(&phy_copy, &status->enabled));

        tmp_status = 1;
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_cl72_receiver_status(&phy_copy, &tmp_status));
        if (tmp_status == 0) {
            status->locked = 0;
            return PHYMOD_E_NONE;
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_loopback_set(const plp_aperta2_phymod_phy_access_t* phy,
                          plp_aperta2_phymod_loopback_mode_t loopback, uint32_t enable)
{
    int i;
    int start_lane, num_lane, port_enable;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* next figure out the lane num and start_lane based on the input */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    if (enable && (num_lane != TSCPMOD_NOF_LANES_IN_CORE)) {
        if (_plp_aperta2_tscp_phy_lane_swap_validate(phy)) {
            PHYMOD_DIAG_OUT(("Warning: Digital and remote loopback will not operate as expected!\n"));
        }
    }

    switch (loopback) {
    case phymodLoopbackGlobal :
    case phymodLoopbackGlobalPMD :
        if (enable) {
            phy_copy.access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_disable_set(&phy_copy));
            /*first squelch rx */
            for (i = 0; i < num_lane; i++) {
                if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                    continue;
                }
                phy_copy.access.lane_mask = 0x1 << (i + start_lane);
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_force_signal_detect(&phy_copy,  (int) enable, (int) 0));
            }
            phy_copy.access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_enable_set(&phy_copy));
        }
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_tx_disable(&phy_copy, enable));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_dig_lpbk(&phy_copy, (uint8_t) enable));
        }
        if (!enable) {
            for (i = 0; i < num_lane; i++) {
                if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                    continue;
                }
                phy_copy.access.lane_mask = 0x1 << (i + start_lane);
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pmd_force_signal_detect(&phy_copy,  (int) enable, (int) 0));
            }
        }
        break;
    case phymodLoopbackRemotePMD :
        /* get current port enable bit */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_port_enable_get(&phy_copy, &port_enable));
        if (port_enable) {
            phy_copy.access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_disable_set(&phy_copy));
        }

        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_rmt_lpbk(&phy_copy, (uint8_t)enable));
        }

        /* Release the pcs lane reset if port is enabled */
        if (port_enable) {
            phy_copy.access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_enable_set(&phy_copy));
        }
        break;
    case phymodLoopbackRemotePCS :
    default :
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_UNAVAIL,
                               (_PHYMOD_MSG("This mode is not supported\n")));
        break;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_loopback_get(const plp_aperta2_phymod_phy_access_t* phy,
                          plp_aperta2_phymod_loopback_mode_t loopback, uint32_t* enable)
{
    int start_lane, num_lane;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*next figure out the lane num and start_lane based on the input*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    switch (loopback) {
    case phymodLoopbackGlobal :
    case phymodLoopbackGlobalPMD :
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_dig_lpbk_get(&phy_copy, enable));
        break;
    case phymodLoopbackRemotePMD :
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_rmt_lpbk_get(&phy_copy, enable));
        break;
    case phymodLoopbackRemotePCS :
    default :
        return PHYMOD_E_UNAVAIL;
    }
    return PHYMOD_E_NONE;
}

/* Core initialization
 * (PASS1)
 * 1.  De-assert PMD core and PMD lane reset
 * 2.  Set heartbeat for comclk
 * 3.  Micro code load and verify
 * (PASS2)
 * 4.  Configure PMD lane mapping and PCS lane swap
 * 5.  De-assert micro reset
 * 6.  Wait for uc_active = 1
 * 7.  Initialize software information table for the micro
 * 8.  Config PMD polarity
 * 9. AFE/PLL configuration
 * 10. Set core_from_pcs_config
 * 11. Program AN default timer
 * 12. Load sd_id_table, am_table and um_table into TSC memory
 * 13. Release core DP soft reset
 */
STATIC
int _plp_aperta2_tscp_core_init_pass1(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config, const plp_aperta2_phymod_core_status_t* core_status)
{
    int lane;
    plp_aperta2_phymod_phy_access_t phy_access;
    plp_aperta2_phymod_core_access_t  core_copy;
    /* need to fix this later */
    uint32_t i;
    uint32_t am_table_load_size, um_table_load_size, speed_id_load_size;
    uint32_t *plp_aperta2_tscp_am_table_entry;
    uint32_t *plp_aperta2_tscp_um_table_entry;
    uint32_t *plp_aperta2_tscp_speed_priority_mapping_table;
    uint32_t *plp_aperta2_tscp_spd_id_entry;
#if PMDCODE
    int  octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(core->access.lane_mask);
    uint32_t init_data[]={0x00,0x00,0x00};
    int idx;
#endif

    plp_aperta2_tscp_spd_id_entry = plp_aperta2_tscp_spd_id_entry_get();
    plp_aperta2_tscp_am_table_entry = plp_aperta2_tscp_am_table_entry_get();
    plp_aperta2_tscp_um_table_entry = plp_aperta2_tscp_um_table_entry_get();
    plp_aperta2_tscp_speed_priority_mapping_table = plp_aperta2_tscp_speed_priority_mapping_table_get();

    TSCPMOD_CORE_TO_PHY_ACCESS(&phy_access, core);
    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));
    if (core->access.lane_mask & 0xFF00) {
        octal = 1;
    }
    core_copy.access.lane_mask = 0x1 << (octal * 8);
    /* 1. De-assert PMD core power and core data path reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pmd_reset_seq((plp_aperta2_phymod_phy_access_t*)&core_copy));

    core_copy.access.pll_idx = 0;
    #if PMDCODE
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_core_dp_reset((plp_aperta2_phymod_phy_access_t*)&core_copy, 1));
    #endif
    /* De-assert PMD lane reset */
    /* FIXME::Here should be hard reset or soft reset? shall we first assert it then de-assert it? */
    for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane++) {
        phy_access.access.lane_mask = 1 << (lane + PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
        PHYMOD_IF_ERR_RETURN
          (plp_aperta2_tscpmod_pmd_x4_reset(&phy_access));
    }
#if PMDCODE
    /* 2. Set the heart beat COM Clock, default is 312.5M */
    if (init_config->interface.ref_clock == phymodRefClk312Mhz) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_comclk_set((plp_aperta2_phymod_phy_access_t*)&core_copy, init_config->interface.ref_clock));
    } else {
        PHYMOD_DEBUG_ERROR(("Unsupported reference clock.\n"));
        return PHYMOD_E_UNAVAIL;
    }
#endif

    PHYMOD_CRIT_INFO(("Initializing Speed ID Table for:0x%x...\n", core->access.addr));
    speed_id_load_size = TSCPMOD_SPEED_ID_TABLE_SIZE > TSCPMOD_HW_SPEED_ID_TABLE_SIZE ? TSCPMOD_HW_SPEED_ID_TABLE_SIZE : TSCPMOD_SPEED_ID_TABLE_SIZE;

    for (i = 0; i < speed_id_load_size; i++) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write((plp_aperta2_phymod_phy_access_t*)&core_copy, phymodMemSpeedIdTable, i,
                              (plp_aperta2_tscp_spd_id_entry + i * TSCPMOD_SPEED_ID_ENTRY_SIZE)));
    }

    /*next need to load UM table and AM table */
    am_table_load_size = TSCPMOD_AM_TABLE_SIZE > TSCPMOD_HW_AM_TABLE_SIZE ? TSCPMOD_HW_AM_TABLE_SIZE : TSCPMOD_AM_TABLE_SIZE;
    um_table_load_size = TSCPMOD_UM_TABLE_SIZE > TSCPMOD_HW_UM_TABLE_SIZE ? TSCPMOD_HW_UM_TABLE_SIZE : TSCPMOD_UM_TABLE_SIZE;

    PHYMOD_CRIT_INFO(("Initializing AM Table for:0x%x...\n", core->access.addr));
    for (i = 0; i < am_table_load_size; i++) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write((plp_aperta2_phymod_phy_access_t*)&core_copy, phymodMemAMTable, i,  (plp_aperta2_tscp_am_table_entry + i * TSCPMOD_AM_ENTRY_SIZE)));
    }

    PHYMOD_CRIT_INFO(("Initializing UM Table for:0x%x...\n", core->access.addr));
    for (i = 0; i < um_table_load_size; i++) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write((plp_aperta2_phymod_phy_access_t*)&core_copy, phymodMemUMTable, i,  (plp_aperta2_tscp_um_table_entry + i * TSCPMOD_UM_ENTRY_SIZE)));
    }
    /*
     * Initialize 1588 look up table. On Rx tables.
     */
    for (idx = 0; idx < TSCPMOD_TS_RX_MPP_MEM_SIZE; idx++) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write((plp_aperta2_phymod_phy_access_t*)&core_copy, phymodMemRxLkup1588Mpp0, idx, init_data));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write((plp_aperta2_phymod_phy_access_t*)&core_copy, phymodMemRxLkup1588Mpp1, idx, init_data));
    }
    PHYMOD_CRIT_INFO(("Initializing Priority Table for:0x%x...\n", core->access.addr));
    /*need to update speed_priority_mapping_table with correct speed id */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_mem_write((plp_aperta2_phymod_phy_access_t*)&core_copy, phymodMemSpeedPriorityMapTable, 0,  plp_aperta2_tscp_speed_priority_mapping_table));

    /* Program AN default timer  for both MMP0 and MMP1*/
    core_copy.access.lane_mask = 0x1 << (octal * 8);
    core_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_set_an_timers((plp_aperta2_phymod_phy_access_t*)&core_copy, init_config->interface.ref_clock, NULL));
    /* enable FEC COBRA as default */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_fec_cobra_enable((plp_aperta2_phymod_phy_access_t*)&core_copy, 1));

    core_copy.access.lane_mask = 0x10 << (octal * 8);
    core_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_set_an_timers((plp_aperta2_phymod_phy_access_t*)&core_copy, init_config->interface.ref_clock, NULL));
    /* enable FEC COBRA as default */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_fec_cobra_enable((plp_aperta2_phymod_phy_access_t*)&core_copy, 1));

    return PHYMOD_E_NONE;
}


STATIC
int _plp_aperta2_tscp_core_init_pass2(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config, const plp_aperta2_phymod_core_status_t* core_status)
{
    plp_aperta2_phymod_phy_access_t phy_access, phy_access_copy;
    plp_aperta2_phymod_core_access_t  core_copy;
    enum peregrine5_pc_pll_refclk_enum refclk;
    plp_aperta2_phymod_polarity_t tmp_pol;
    uint32_t tvco_rate;
    int octal = 0;
    plp_aperta2_phymod_phy_access_t phy_acess_copy_rst;
#if PMDCODE
    uint32_t fclk_div_mode = 1;
    int lane;
#endif
    if (core->access.lane_mask & 0xFF00) {
        octal = 1;
    }

    TSCPMOD_CORE_TO_PHY_ACCESS(&phy_access, core);
    phy_access_copy = phy_access;
    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));

    core_copy.access.lane_mask = 0x1 << (octal * 8);
    phy_access_copy = phy_access;
    phy_access_copy.access = core->access;
    phy_access_copy.access.lane_mask = 0x1 << (octal * 8);
    phy_access_copy.type = core->type;

    PHYMOD_MEMSET(&tmp_pol, 0x0, sizeof(tmp_pol));
    
    PHYMOD_CRIT_INFO(("Setting Up Reference clock for:0x%x pll:%d...\n", core->access.addr, init_config->pll0_div_init_value));
    if (init_config->interface.ref_clock == phymodRefClk312Mhz) {
        refclk = PEREGRINE5_PC_PLL_REFCLK_312P5MHZ;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_refclk_set(&phy_access, TSCPMOD_REF_CLK_312P5MHZ));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_pll_to_vco_get(TSCPMOD_REF_CLK_312P5MHZ, init_config->pll0_div_init_value, &tvco_rate));
    } else {
        PHYMOD_DEBUG_ERROR(("Unsupported reference clock.\n"));
        return PHYMOD_E_UNAVAIL;
    }

    COMPILER_REFERENCE(refclk);
    COMPILER_REFERENCE(phy_access_copy);
    /* 7. Wait for uc_active = 1 */
    PHYMOD_IF_ERR_RETURN
       (plp_aperta2_peregrine5_pc_wait_uc_active(&phy_access));

    for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane++) {
            phy_access_copy.access.lane_mask = 1 << (lane + PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_pmd_ln_h_rstb_pkill_override(&phy_access_copy, 0x1));
        }

        /* 8. Initialize software information table for the macro */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_init_peregrine5_pc_info((plp_aperta2_phymod_phy_access_t*)&core_copy));

       /* release pmd lane hard reset */
        for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane++) {
            phy_access_copy.access.lane_mask = 1 << (lane + PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_pmd_ln_h_rstb_pkill_override(&phy_access_copy, 0x0));
        }
    /* set PCS delay cnt to 4 by default */
    phy_acess_copy_rst  = phy_access_copy;
    phy_acess_copy_rst.access.lane_mask = 0xff << (octal * 8);
    /* set PCS delay cnt to 4 by default */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_tscpmod_pmd_tx_pcs_delay_cnt_set(&phy_acess_copy_rst, 0x4));
    /* Enable PCS clock block on the PMD side */
    core_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pcs_clk_blk_en((plp_aperta2_phymod_phy_access_t*)&core_copy, 1));

    if ((int) (init_config->pll0_div_init_value) != 0xFFFFFFFF ) {
        int tx_clock_div34 = 0;
        core_copy.access.pll_idx = 0;

        /* based on the VCO rate, need to choose tx clock divider */
        /*   for 51G choose div33, for 53G choose div34  */
        if ((int) (init_config->pll0_div_init_value) == TSCPMOD_PLL_MODE_DIV_170) {
            tx_clock_div34 = 1;
        }

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_tx_clock_div34_enable_set((plp_aperta2_phymod_phy_access_t*)&core_copy, tx_clock_div34));
        /* Sending pll0 init value as user defined*/
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_configure_pll_refclk_div((plp_aperta2_phymod_phy_access_t*)&core_copy,
                                                      refclk,
                                                      init_config->pll0_div_init_value));
    }

    /* Peregrine aging support */
#if (PEREGRINE_AGING_SUPPORT)
    for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane++) {
        phy_access_copy.access.lane_mask = 1 << lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_pmd_ln_h_pwrdn_pkill_override(&phy_access_copy, 1));
    }
#endif

    COMPILER_REFERENCE(fclk_div_mode);
    if ((int) (init_config->pll0_div_init_value) != 0xFFFFFFFF) {
        /* Set FCLK period. */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_set_fclk_period((plp_aperta2_phymod_phy_access_t*)&core_copy, tvco_rate, fclk_div_mode));
    }

    /* Set PM timer offset. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pcs_1588_ts_offset_set((plp_aperta2_phymod_phy_access_t*)&core_copy, /*init_config->pm_timer_offset*/0, 0));

    /* set the PMD debug level to be 2 as default */
    /* need to re-enable later */
    /* disable dynamic tracking feature by default*/

    for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane++) {
        phy_access_copy.access.lane_mask = 1 << (lane + (octal * 8));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_set_usr_event_log_group_mask(&phy_access_copy, EVENT_GROUP_PRIORITY_2));
    }

    /* disable aging protection to lower power and/or reduce noise */
    phy_access_copy.access.lane_mask = 0xff << (octal * 8);
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_rx_protect_nrzmux_set(&phy_access_copy, 0x1));

    /* 14. Release core DP soft reset for PLL0 */
    core_copy.access.lane_mask = 0x1 << (octal * 8);
    core_copy.access.pll_idx = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_core_dp_reset((plp_aperta2_phymod_phy_access_t*)&core_copy, 0));

    return PHYMOD_E_NONE;

}


int plp_aperta2_tscp_core_init(const plp_aperta2_phymod_core_access_t* core,
                   const plp_aperta2_phymod_core_init_config_t* init_config,
                   const plp_aperta2_phymod_core_status_t* core_status)
{
    if ( (!PHYMOD_CORE_INIT_F_EXECUTE_PASS1_GET(init_config) &&
          !PHYMOD_CORE_INIT_F_EXECUTE_PASS2_GET(init_config)) ||
        PHYMOD_CORE_INIT_F_EXECUTE_PASS1_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscp_core_init_pass1(core, init_config, core_status));

        if (PHYMOD_CORE_INIT_F_EXECUTE_PASS1_GET(init_config)) {
            return PHYMOD_E_NONE;
        }
    }

    if ( (!PHYMOD_CORE_INIT_F_EXECUTE_PASS1_GET(init_config) &&
          !PHYMOD_CORE_INIT_F_EXECUTE_PASS2_GET(init_config)) ||
        PHYMOD_CORE_INIT_F_EXECUTE_PASS2_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscp_core_init_pass2(core, init_config, core_status));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_init(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_phy_init_config_t* init_config)
{
    plp_aperta2_phymod_phy_access_t pm_phy_copy;
    int start_lane, num_lane, octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(phy->access.lane_mask);
    uint32_t pll_power_down = 0;
    uint32_t pll_div;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
/*     PHYMOD_MEMSET(&firmware_lane_config, 0x0, sizeof(firmware_lane_config)); */

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pm_phy_copy.access, &start_lane, &num_lane));
    /* per lane based reset release */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pmd_x4_reset(&pm_phy_copy));

   /* make sure that power up PLL is locked */
    pm_phy_copy.access.pll_idx = 0;
    pm_phy_copy.access.lane_mask = 1 << (octal *8);
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_pll_pwrdn_get(&pm_phy_copy, &pll_power_down));

    /* next read current TVCO pll divider*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_INTERNAL_read_pll_div(&pm_phy_copy, &pll_div));


    /* need to check pll0 lock if not power up */
    /* put the check here is to save on boot up time */
    /* FIXME need to re-visit once Peregrine api is integrated */
    if (!pll_power_down) {
        uint32_t cnt = 0, pll_lock = 0;
        cnt = 0;
        while (cnt < 5000) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pll_lock_get(&pm_phy_copy, &pll_lock));
            cnt = cnt + 1;
            if (pll_lock) {
                break;
            } else {
                if(cnt == 5000) {
                    PHYMOD_DEBUG_ERROR(("WARNING :: core 0x%x PLL0 is not locked within 50 milli second \n",
                                         pm_phy_copy.access.addr));
                    break;
                }
            }
            PHYMOD_USLEEP(10);
        }
    }
    PHYMOD_CRIT_INFO(("PLL locked for:0x%x...%s\n", phy->access.addr, phy->port_loc == phymodPortLocLine ? "LINE":"SYS"));
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_link_status_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* link_status)
{
    plp_aperta2_phymod_phy_access_t pm_phy_copy;
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_tscpmod_get_pcs_latched_link_status(&pm_phy_copy, link_status));
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_tx_taps_default_get(const plp_aperta2_phymod_phy_access_t* phy,
                                 phymod_phy_signalling_method_t mode,
                                 phymod_pam4_tx_t* tx)
{
    /*always default to 6-taps mode */
    tx->serdes_tx_tap_mode = phymodTxTapModePAM4_6TAP;
    if (mode == phymodSignallingMethodNRZ) {
        tx->pre3 = 0;
        tx->pre2 = 0;
        tx->pre = -12;
        tx->main = 88;
        tx->post = -26;
        tx->post2 = 0;
        tx->post3 = 0;
    } else if (mode == phymodSignallingMethodPAM4) {
        tx->pre3 = -4;
        tx->pre2 = 14;
        tx->pre = -36;
        tx->main = 112;
        tx->post = 0;
        tx->post2 = 0;
        tx->post3 = 0;
    } else {
        tx->pre3 = 0;
        tx->pre2 = 0;
        tx->pre = 0;
        tx->main = 128;
        tx->post = 0;
        tx->post2 = 0;
        tx->post3 = 0;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_lane_config_default_get(const plp_aperta2_phymod_phy_access_t* phy,
                                     phymod_phy_signalling_method_t mode,
                                     phymod_firmware_lane_config_t* lane_config)
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
    lane_config->Cl72RestTO = 0;
    lane_config->ForceES = 0;
    lane_config->LinkPartnerPrecoderEn = 0;
    lane_config->UnreliableLos = 0;
    lane_config->ForceOsCdr = 0;

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_reg_read(const plp_aperta2_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t* val)
{
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_reg_write(const plp_aperta2_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t val)
{
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_firmware_load_info_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_firmware_load_info_t* info)
{
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_autoneg_advert_ability_set(const plp_aperta2_phymod_phy_access_t* phy,
                                        const phymod_autoneg_advert_abilities_t* an_advert_abilities,
                                        const phymod_phy_pll_state_t* old_pll_adv_state,
                                        phymod_phy_pll_state_t* new_pll_adv_state)
{
    int start_lane, num_lane;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    /* Program local advert abilitiy registers */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_autoneg_ability_set(&phy_copy, an_advert_abilities));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_autoneg_advert_ability_get(const plp_aperta2_phymod_phy_access_t* phy,
                                        phymod_autoneg_advert_abilities_t* an_advert_abilities)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane;
    uint32_t i;
    phymod_firmware_lane_config_t firmware_lane_config;

    firmware_lane_config.MediaType = phymodFirmwareMediaTypePcbTraceBackPlane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_autoneg_ability_get(&phy_copy, an_advert_abilities));

    /* Get Medium type from fw_lane_config */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscp_phy_firmware_lane_config_get(phy, &firmware_lane_config));

    for (i = 0; i < an_advert_abilities->num_abilities; i++) {
        an_advert_abilities->autoneg_abilities[i].medium = firmware_lane_config.MediaType;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_autoneg_remote_advert_ability_get(const plp_aperta2_phymod_phy_access_t* phy,
                                               phymod_autoneg_advert_abilities_t* an_advert_abilities)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane;
    uint32_t i;
    phymod_firmware_lane_config_t firmware_lane_config;

    firmware_lane_config.MediaType = phymodFirmwareMediaTypePcbTraceBackPlane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_autoneg_remote_ability_get(&phy_copy, an_advert_abilities));

    /* Get Medium type from fw_lane_config */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscp_phy_firmware_lane_config_get(phy, &firmware_lane_config));

    for (i = 0; i < an_advert_abilities->num_abilities; i++) {
        an_advert_abilities->autoneg_abilities[i].medium = firmware_lane_config.MediaType;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_autoneg_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_autoneg_control_t* an)
{
    int num_lane_adv_encoded, mapped_speed_id;
    int start_lane, num_lane;
    int i, do_lane_config_set;
    uint32_t pll_div, vco_rate, refclk_in_hz;
    phymod_firmware_lane_config_t firmware_lane_config;
    tscpmod_an_control_t an_control;
    plp_aperta2_phymod_phy_access_t phy_copy;
    tscpmod_refclk_t ref_clk;
    tscpmod_spd_intfc_type_t spd_intf = 0;

    PHYMOD_MEMSET(&firmware_lane_config, 0x0, sizeof(firmware_lane_config));

    PHYMOD_MEMSET(&an_control, 0x0, sizeof(an_control));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* first set the port an mode enable bit */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_an_mode_enable_set(&phy_copy, (int) an->enable));

    phy_copy.access.lane_mask = 0x1 << start_lane;

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
            an_control.an_type = TSCPMOD_AN_MODE_CL73;
            break;
        case phymod_AN_MODE_CL73BAM:
            an_control.an_type = TSCPMOD_AN_MODE_CL73_BAM;
            break;
        case phymod_AN_MODE_CL73_MSA:
            an_control.an_type = TSCPMOD_AN_MODE_CL73_MSA;
            break;
        default:
            return PHYMOD_E_PARAM;
            break;
    }

    if (an->enable) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        /* Set AN port mode */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_set_an_port_mode(&phy_copy, start_lane));

        /* Get TVCO rate (PLL1 for now) */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_refclk_get(&phy_copy, &ref_clk));

        if (ref_clk == TSCPMOD_REF_CLK_312P5MHZ) {
            refclk_in_hz = 312500000;
        } else {
            refclk_in_hz = 156250000;
        }

        /* next read current TVCO pll divider*/
        phy_copy.access.pll_idx = 0;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_INTERNAL_read_pll_div(&phy_copy, &pll_div));

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_get_vco_from_refclk_div(&phy_copy, refclk_in_hz, pll_div, &vco_rate, 0));

        if (vco_rate == 41250000) {
            /* load 41G VCO spd_id */
            spd_intf = TSCPMOD_SPD_CL73_IEEE_41G;
        } else if (vco_rate == 51562500) {
            /* load 51G VCO spd_id */
            spd_intf = TSCPMOD_SPD_CL73_IEEE_51G;
        } else if (vco_rate == 53125000) {
            /* load 53G VCO spd_id */
            spd_intf = TSCPMOD_SPD_CL73_IEEE_53G;
        } else {
            return PHYMOD_E_PARAM;
        }

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_get_mapped_speed(spd_intf, &mapped_speed_id));

        phy_copy.access.lane_mask = 0x1 << start_lane;

        /* Load 1G speed ID */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_set_sc_speed(&phy_copy, mapped_speed_id, 0));
    }

    PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscp_phy_firmware_lane_config_get(&phy_copy, &firmware_lane_config));

    if (an->enable) {
        do_lane_config_set = 0;
        /* make sure the firmware config is set to an enabled */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscp_phy_firmware_lane_config_get(&phy_copy, &firmware_lane_config));
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
    } else {
        /* Clear AnEnabled before disabling PCS CL73 autoneg to avoid a rare race condition with PMD */
        firmware_lane_config.AnEnabled = 0;
        firmware_lane_config.LaneConfigFromPCS = 0;
        do_lane_config_set = 1;
    }

    if (do_lane_config_set) {
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 1));
        }
        PHYMOD_USLEEP(1000);
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (_plp_aperta2_tscp_phy_firmware_lane_config_set(&phy_copy, firmware_lane_config));
        }
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 0));
        }
    }

    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_autoneg_control(&phy_copy, &an_control));
    if (!an->enable) {
        /* Disable Tx PAM4 pre-coding. It might be enabled by AN link training. */
        for (i = 0; i < num_lane; i++) {
            if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
                continue;
            }
            phy_copy.access.lane_mask = 0x1 << (i + start_lane);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_tx_pam4_precoder_enable_set(&phy_copy, 0));
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_autoneg_get(const plp_aperta2_phymod_phy_access_t* phy,
                         plp_aperta2_phymod_autoneg_control_t* an, uint32_t* an_done)
{
    tscpmod_an_control_t an_control;
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane;
    int an_complete = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_MEMSET(&an_control, 0x0,  sizeof(tscpmod_an_control_t));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_autoneg_control_get(&phy_copy, &an_control, &an_complete));

    if (an_control.enable) {
        an->enable = 1;
        *an_done = an_complete;
    } else {
        an->enable = 0;
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
        case TSCPMOD_AN_MODE_CL73:
            an->an_mode = phymod_AN_MODE_CL73;
            break;
        case TSCPMOD_AN_MODE_CL73_BAM:
            an->an_mode = phymod_AN_MODE_CL73BAM;
            break;
        case TSCPMOD_AN_MODE_MSA:
            an->an_mode = phymod_AN_MODE_MSA;
            break;
        case TSCPMOD_AN_MODE_CL73_MSA:
            an->an_mode = phymod_AN_MODE_CL73_MSA;
            break;
        default:
            an->an_mode = phymod_AN_MODE_NONE;
            break;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_autoneg_status_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_autoneg_status_t* status)
{
    int an_en, an_done;
    phymod_phy_speed_config_t speed_config;
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, speed_id;
    uint32_t packed_entry[5];
    tscpmod_spd_id_tbl_entry_t speed_config_entry;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << start_lane;

    PHYMOD_IF_ERR_RETURN
       (plp_aperta2_tscpmod_autoneg_status_get(&phy_copy, &an_en, &an_done));

    PHYMOD_IF_ERR_RETURN
       (plp_aperta2_tscp_phy_speed_config_get(phy, &speed_config));

    if (an_en && an_done) {
        uint32_t an_resolved_mode;
        /* if an resolves and link up */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_speed_id_get(&phy_copy, &speed_id));
        /*read the speed id entry and get the num_lane info */
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));
        plp_aperta2_tscpmod_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);
        num_lane = 1 << speed_config_entry.num_lanes;
        /* read the AN final resolved port mode */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_resolved_port_mode_get(&phy_copy, &an_resolved_mode));
        /*status->resolved_port_mode = an_resolved_mode;*/
    }

    status->enabled   = an_en;
    status->locked    = an_done;
    status->data_rate = speed_config.data_rate;
    /*status->resolved_num_lane = num_lane;*/

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_pll_reconfig(const plp_aperta2_phymod_phy_access_t* phy,
                          uint8_t pll_index,
                          uint32_t pll_div,
                          plp_aperta2_phymod_ref_clk_t ref_clk1)
{
    plp_aperta2_phymod_phy_access_t pm_phy_copy;
    tscpmod_refclk_t ref_clk;
    uint32_t tvco_rate = 0;
    enum peregrine5_pc_pll_refclk_enum refclk;
    uint32_t cnt = 0, pll_lock = 0, fclk_div_mode = 1;
    int tx_clock_div34 = 0, octal_shift = 0;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    
    if (pll_index > 0) {
        PHYMOD_DEBUG_ERROR(("Unsupported PLL index\n"));
        return PHYMOD_E_UNAVAIL;
    }
    if (APERTA2_GET_OCTAL(phy->access.lane_mask) == APERTA2_PM_OCTAL2) {
        octal_shift = 1;
    }
    

    pm_phy_copy.access.lane_mask = 1 << (octal_shift * 8);
    /* first needs to read the ref clock from main reg*/
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_refclk_get(&pm_phy_copy, &ref_clk));

    if (ref_clk == TSCPMOD_REF_CLK_312P5MHZ) {
        refclk = PEREGRINE5_PC_PLL_REFCLK_312P5MHZ;
    } else {
        PHYMOD_DEBUG_ERROR(("Unsupported reference clock.\n"));
        return PHYMOD_E_UNAVAIL;
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pll_to_vco_get(ref_clk, pll_div, &tvco_rate));

    /* next disable pcs datapath only if TVCO re-config*/
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_disable_set(&pm_phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_core_dp_reset(&pm_phy_copy, 1));

    /* need to set tx clock div properly based on VCO rate */
    if ((int) pll_div == TSCPMOD_PLL_MODE_DIV_170) {
        tx_clock_div34 = 1;
    }
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_tx_clock_div34_enable_set(&pm_phy_copy, tx_clock_div34));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_configure_pll_refclk_div(&pm_phy_copy,
                                                  refclk,
                                                  pll_div));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_core_dp_reset(&pm_phy_copy, 0));


    COMPILER_REFERENCE(fclk_div_mode);
    /* Configure FCLK period. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_set_fclk_period(&pm_phy_copy, tvco_rate, fclk_div_mode));

    /* need to wait for the PLL lock */
    cnt = 0;
    while (cnt < 1000) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_pll_lock_get(&pm_phy_copy, &pll_lock));
        cnt = cnt + 1;
        if (pll_lock) {
            break;
        } else {
            if (cnt == 1000) {
                PHYMOD_DEBUG_ERROR(("WARNING :: core 0x%x PLL Index %d is not locked within 10 milli second \n", pm_phy_copy.access.addr, pll_index));
                break;
            }
        }
        PHYMOD_USLEEP(10);
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_tx_pam4_precoder_enable_set(const plp_aperta2_phymod_phy_access_t* phy, int enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane, port_start_lane, port_num_lane, i;
    uint32_t lane_reset, pcs_lane_enable;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*get the start lane of the port lane mask */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_port_start_lane_get(&phy_copy, &port_start_lane, &port_num_lane));

    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_lane_soft_reset_get(&phy_copy, &lane_reset));
    phy_copy.access.lane_mask = 1 << port_start_lane;
    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_enable_get(&phy_copy, &pcs_lane_enable));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << port_start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_disable_set(&phy_copy));
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 1));
    }

    for (i = 0; i < num_lane; i++) {
        if (!PHYMOD_LANEPBMP_MEMBER(phy->access.lane_mask, start_lane + i)) {
            continue;
        }
        phy_copy.access.lane_mask = 1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_tx_pam4_precoder_enable_set(&phy_copy, enable));
    }

    /* release the ln dp reset */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << port_start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_enable_set(&phy_copy));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_tx_pam4_precoder_enable_get(const plp_aperta2_phymod_phy_access_t* phy, int *enable)
{
    plp_aperta2_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_tx_pam4_precoder_enable_get(&pm_phy_copy, enable));
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_timesync_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane;
    uint32_t pcs_lane_enable;
    uint32_t fclk_div_mode = 1;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    phy_copy.access.lane_mask = 0x1 << start_lane;

    /* RX timestamping control */
    if (PHYMOD_TS_DIRECTION_RX_GET(flags)) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_pcs_rx_ts_en(&phy_copy, enable));
    }

    /* Core related control:
     * 1. Enable FCLK on PMD, with div_mode = 8T.
     */
    /*if (PHYMOD_TIMESYNC_ENABLE_F_CORE_GET(flags)) */{
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_clk4sync_enable_set(&phy_copy, enable, fclk_div_mode));
    }

    /* 2. One-Step Timestmap Pipeline. */
    /*if (PHYMOD_TIMESYNC_ENABLE_F_ONE_STEP_PIPELINE_GET(flags)) {*/
    if (flags &4) {
        /* Check if PCS lane is in reset. */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_enable_get(&phy_copy, &pcs_lane_enable));
        /* Disable PCS lane if PCS lane is not in reset. */
        if (pcs_lane_enable) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_disable_set(&phy_copy));
        }

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_osts_pipeline(&phy_copy, enable));

        /* Re-enable PCS lane if PCS lane not in reset. */
        if (pcs_lane_enable) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_enable_set(&phy_copy));
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_timesync_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    if (/*PHYMOD_TIMESYNC_ENABLE_F_ONE_STEP_PIPELINE_GET(flags)*/flags & 4) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_osts_pipeline_get(&phy_copy, enable));
    } else {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_pcs_rx_ts_en_get(&phy_copy, enable));
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_timesync_offset_set(const plp_aperta2_phymod_core_access_t* core, uint32_t ts_offset)
{
    uint32_t sub_ns_offset=0;
    plp_aperta2_phymod_core_access_t core_copy;
    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pcs_1588_ts_offset_set((plp_aperta2_phymod_phy_access_t*)&core_copy, ts_offset, sub_ns_offset));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_timesync_offset_get(const plp_aperta2_phymod_core_access_t* core, uint32_t *ts_offset)
{
    uint32_t sub_ns_offset=0;
    plp_aperta2_phymod_core_access_t core_copy;
    PHYMOD_MEMCPY(&core_copy, core, sizeof(core_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pcs_1588_ts_offset_get((plp_aperta2_phymod_phy_access_t*)&core_copy, ts_offset, &sub_ns_offset));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_timesync_adjust_set(const plp_aperta2_phymod_phy_access_t* phy,
                             uint32_t flags,
                             const tscp_timesync_adjust_config_info_t *config)
{

#if PMDCODE
    int start_lane, num_lane, i, is_pam4, is_valid = 0;
    int speed_id, mapped_speed_id;
    phymod_firmware_lane_config_t firmware_lane_config;
    int an_en, an_done, normalize_to_latest = 0;
    plp_aperta2_phymod_phy_access_t phy_copy;
    tscpmod_spd_id_tbl_entry_t speed_config_entry;
    uint32_t packed_entry[5];
    phymod_phy_speed_config_t speed_config;
    tscpmod_spd_intfc_type_t spd_intf;
    phymod_mem_type_t tx_mem, rx_mem;
    int ts_table_index;
    int osr_mode;
    int pma_width_multiplier;
    tscpmod_ts_table_entry *ts_tx_entry = NULL, *ts_rx_entry = NULL;
    uint32_t ts_table_size;
    uint32_t pll_div;
    tscpmod_refclk_t ref_clk;
    uint32_t vco;
    int rx_mem_offset, tx_mem_offset;
    int rx_shadow_mem_offset;
    int tx_max_skew;
    uint32_t clk4sync_en, clk4sync_div;
    uint32_t lane_mask_backup;
    uint32_t rx_max_skew, rx_min_skew, rx_dsl_sel, rx_psll_sel;
    uint32_t skew_per_vl[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM];
    int ts_offset;
    int ts_use_sfd_table;
    int rx_base_addr = 0;
    int tx_base_addr = 0;
    int rx_use_both_tables = 0;
    uint8_t fec_arch = 0;

    tscpmod_ts_table_entry* plp_aperta2_tscpmod_ts_table_tx_sop;
    tscpmod_ts_table_entry* plp_aperta2_tscpmod_ts_table_rx_sop;
    tscpmod_ts_table_entry* plp_aperta2_tscpmod_ts_table_tx_sfd;
    tscpmod_ts_table_entry* plp_aperta2_tscpmod_ts_table_rx_sfd;
    tscpmod_ts_table_entry_mem *rx_shadow_mem;
    tscpmod_ts_table_entry_mem *rx_shadow_mem_mpp1;
    tscpmod_ts_table_entry_mem *tx_shadow_mem;


    plp_aperta2_tscpmod_ts_table_tx_sop = plp_aperta2_tscpmod_ts_table_tx_sop_get(TSCP_TIMESYNC_F_802_3_CX_GET(flags));
    plp_aperta2_tscpmod_ts_table_rx_sop = plp_aperta2_tscpmod_ts_table_rx_sop_get(TSCP_TIMESYNC_F_802_3_CX_GET(flags));
    plp_aperta2_tscpmod_ts_table_tx_sfd = plp_aperta2_tscpmod_ts_table_tx_sfd_get(TSCP_TIMESYNC_F_802_3_CX_GET(flags));
    plp_aperta2_tscpmod_ts_table_rx_sfd = plp_aperta2_tscpmod_ts_table_rx_sfd_get(TSCP_TIMESYNC_F_802_3_CX_GET(flags));

    if (config->am_norm_mode == 0x2) {
        /* phymodTimesyncCompensationModeLatestlane */
        normalize_to_latest = 1;
    }

   /*
    * ----------------------------------
    *   SOP  |   MAC_DA   |   SELECTION
    * ----------------------------------
    *   1    |      x     |   SOP
    *   0    |      1     |   MAC_DA
    *   0    |      0     |   SFD
    */

    ts_use_sfd_table = 0;
    if (TSCP_TIMESYNC_F_SOP_GET(flags)) {
        ts_offset = TSCP_TS_OFFSET_SOP;
    } else {
        if (TSCP_TIMESYNC_F_MAC_DA_GET(flags)) {
            ts_offset = TSCP_TS_OFFSET_MAC_DA;
        } else {
            ts_offset = TSCP_TS_OFFSET_SFD;
            ts_use_sfd_table = 1;
        }
    }

    /*
     * In Reduced Preamble Mode for Non SOP location, subtract the offset.
     */
    if ((TSCP_TIMESYNC_F_REDUCED_PREAMBLE_MODE_GET(flags)) &&
        (ts_offset != TSCP_TS_OFFSET_SOP)){
        ts_offset -= TSCP_TS_RPM_OFFSET;
    }

    PHYMOD_MEMSET(&firmware_lane_config,0,sizeof(firmware_lane_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscp_phy_firmware_lane_config_get(phy, &firmware_lane_config));
    if (firmware_lane_config.ForcePAM4Mode) {
        is_pam4 = 1;
    } else {
        is_pam4 = 0;
    }

    /* Save the lane mask for the logical port. */
    lane_mask_backup = phy->access.lane_mask;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /* Here starts the sequence to enable Timestamping based on the current speed config. */
    tx_mem_offset           = TSCPMOD_MPP_LANE(start_lane) * TSCPMOD_TS_DEFAULT_TABLE_SIZE;
    rx_mem_offset           = TSCPMOD_MPP_LANE(start_lane) * TSCPMOD_TS_DEFAULT_TABLE_SIZE;
    rx_shadow_mem_offset    = rx_mem_offset;

    /*
     * phymod_timesync_adjust_config_info_t memory mapping
     * RX tabke     -> rx_lkup_1588_mem_mpp0/1
     * TX table     -> tx_lkup_1588_mem_mpp0/1
     * RX_OPT table -> rx_lkup_1588_mem_sp
     * TX_OPT table -> tx_lkup_1588_mem_sp
     */
    if (TSCPMOD_MPP_NUM(start_lane) == 0) {
        /* MPP0 */
        rx_mem = phymodMemRxLkup1588Mpp0;
        tx_mem = phymodMemTxLkup1588Mpp0;
        rx_shadow_mem = NULL;
        tx_shadow_mem = NULL;
    } else {
        /* MPP1. */
        rx_mem = phymodMemRxLkup1588Mpp1;
        tx_mem = phymodMemTxLkup1588Mpp1;
        rx_shadow_mem = NULL;
        tx_shadow_mem = NULL;
    }



    /* 1. Find default 1588 Table. */

    phy_copy.access.lane_mask = 0x1 << start_lane;
    /* 1.1 Get current speed id. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_speed_id_get(&phy_copy, &speed_id));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));

    plp_aperta2_tscpmod_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);

    /* 1.2 Update num_lane and lane mask for AN port. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_autoneg_status_get(&phy_copy, &an_en, &an_done));
    if (an_en && an_done) {
        num_lane = 1 << speed_config_entry.num_lanes;
        /* Updaet lane_mask */
        lane_mask_backup = 0x0;
        for (i = 0; i < num_lane; i++) {
            lane_mask_backup |= 0x1 << (i + start_lane);
        }
    }

    /* 1.3 Get FEC type. */
    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_tscp_speed_table_entry_to_speed_config_get(phy, &speed_config_entry, &speed_config));
    if (an_en && an_done) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_autoneg_fec_status_get(&phy_copy, &fec_arch));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_fec_arch_decode_get(fec_arch, &(speed_config.fec_type)));
    }

    /* 1.4 Get mapped speed id. */
    if (speed_id <= TSCPMOD_AUTONEG_SPEED_ID_COUNT) {
        mapped_speed_id = speed_id;
    } else {
        /* Force speed speed IDs. */
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscp_phy_speed_id_set(num_lane, speed_config.data_rate,
                                       speed_config.fec_type, &spd_intf));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_get_mapped_speed(spd_intf, &mapped_speed_id));
    }

    /* 1.5 Get the table index and table size of the 1588 table. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_1588_table_index_get( mapped_speed_id,
                                       &ts_table_index,
                                       &ts_table_size));

    if (ts_table_index == -1) {
        PHYMOD_DEBUG_ERROR(("1588 is not supported in current speed config.\n"));
        return PHYMOD_E_UNAVAIL;
    }

    /*
     * Rx 1588 memory use native HW address, Tx use regular address.
     */
    rx_base_addr = TSCPMOD_TS_RX_MPP_MEM_ADDR_TO_NATIVE_ADDR(rx_mem_offset);
    tx_base_addr = tx_mem_offset;

    /*
     * 100G 1 lane 544_2xN need the special handling of table base address.
     * This uses the space for two lanes (80 entries). This is now
     * need to put in dedicated memory location.
     */
    if (ts_table_index == TSCPMOD_SPEED_100G_IEEE_KR1_CR1_OPT){

        /* Tx use different memory space. */
        tx_mem              = phymodMemTxLkup1588100G_KR1_2XN;
        tx_mem_offset       = TSCP_TS_TX_1588_TABLE_100G_2XN_START_LOCATION;

        /* Rx use same memory, different offset location. */
        rx_mem_offset       = TSCP_TS_RX_1588_TABLE_100G_2XN_START_LOCATION;
        rx_shadow_mem_offset= 0;

        rx_base_addr        = TSCP_TS_RX_1588_TABLE_100G_2XN_BASE_ADDRESS;
        tx_base_addr        = TSCP_TS_TX_1588_TABLE_100G_2XN_BASE_ADDRESS;

        rx_shadow_mem = NULL;
        tx_shadow_mem = NULL;
    }

    /*
     * 800G 8 lanes 544_2xN need to load both MPP0 and MPP1.
     */
    rx_shadow_mem_mpp1 = NULL;
    if (ts_table_index == TSCPMOD_SPEED_800G_BRCM_KR8_CR8){
        rx_use_both_tables = 1;
        rx_shadow_mem_mpp1 = (tscpmod_ts_table_entry_mem *)config->rx_lkup_1588_mem_mpp1;

    }

    /* 1.6 Find the 1588 table. */
    if (ts_use_sfd_table) {
        ts_tx_entry = plp_aperta2_tscpmod_ts_table_tx_sfd + ts_table_index;
        ts_rx_entry = plp_aperta2_tscpmod_ts_table_rx_sfd + ts_table_index;
    } else {
        ts_tx_entry = plp_aperta2_tscpmod_ts_table_tx_sop + ts_table_index;
        ts_rx_entry = plp_aperta2_tscpmod_ts_table_rx_sop + ts_table_index;
    }

   /* Get OS mode. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_osr_mode_get(&phy_copy, &osr_mode));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_refclk_get(&phy_copy, &ref_clk));

    /* Get current VCO. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_phy_pll_multiplier_get(&phy_copy, &pll_div));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pll_to_vco_get(ref_clk, pll_div, &vco));

    /* 2. Load 1588 TX table. */
    for (i = 0; i < (int) ts_table_size; i++) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write(&phy_copy, tx_mem, (i + tx_mem_offset), &(*ts_tx_entry)[i][0]));
        if (tx_shadow_mem != NULL) {
            PHYMOD_MEMCPY( &tx_shadow_mem[i + tx_mem_offset][0],
                        &(*ts_tx_entry)[i][0],
                        sizeof(uint32_t)*TSCPMOD_TS_ENTRY_SIZE);
        }
    }

    /* 3. Load default 1588 RX table. No need to add fix PM offset as already in the table.*/

    /* 3.4 Write updated table to memory. */
    for (i = 0; i < (int)ts_table_size; i++) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write(&phy_copy, rx_mem, (i + rx_mem_offset), &(*ts_rx_entry)[i][0]));
        /*
         * special handling for speed that need to load both MPP0 and MPP1 1588 tables.
         */
        if (rx_use_both_tables) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_mem_write(&phy_copy, phymodMemRxLkup1588Mpp1, (i + rx_mem_offset), &(*ts_rx_entry)[i][0]));
            if (rx_shadow_mem_mpp1 != NULL) {
                PHYMOD_MEMCPY( &rx_shadow_mem_mpp1[i + rx_shadow_mem_offset][0],
                            &(*ts_rx_entry)[i][0],
                            sizeof(uint32_t)*TSCPMOD_TS_ENTRY_SIZE);
            }
        }
        if (rx_shadow_mem != NULL) {
            PHYMOD_MEMCPY( &rx_shadow_mem[i + rx_shadow_mem_offset][0],
                        &(*ts_rx_entry)[i][0],
                        sizeof(uint32_t)*TSCPMOD_TS_ENTRY_SIZE);
        }
    }

    phy_copy.access.lane_mask = 0x1 << start_lane;
    /* 4. Proram UI. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_clk4sync_enable_get(&phy_copy, &clk4sync_en, &clk4sync_div));

    /* Both UI and PMD latency are PMD lane based,
     * so the lane mask need to be the entire logical port's lane_mask.
     */
    phy_copy.access.lane_mask = lane_mask_backup;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pcs_set_1588_ui(&phy_copy, vco, osr_mode, is_pam4));

    /* 5. Program PMD latency. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_tx_pmd_latency_set(&phy_copy, vco, osr_mode, is_pam4));

    /* 6. Update deskew for TX and RX. */
    if (config->am_norm_mode) {
        /* 6.1 Enable rx, tx deskew record.
         *     Need low-to-hight transition to trigger HW recording the current status.
         */
        phy_copy.access.lane_mask = 0x1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_pcs_rx_deskew_en(&phy_copy, 0));
        phy_copy.access.lane_mask = lane_mask_backup;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_pcs_set_tx_lane_skew_capture(&phy_copy, 1));
        PHYMOD_USLEEP(5);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_pcs_set_tx_lane_skew_capture(&phy_copy, 0));
        phy_copy.access.lane_mask = 0x1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_pcs_rx_deskew_en(&phy_copy, 1));

        /* 6.2 Update tx skew. */

        /*
         * T_PMA_OUTPUT_WIDTH_MODE, mode 0 is 40b, 1 and 2 are 80b and 40bx2 respectively.
         * Depending on the width mode, determine the pma width multiplier.
         */
        pma_width_multiplier = (speed_config_entry.t_pma_output_width_mode == 0) ? 1:2;
        phy_copy.access.lane_mask = lane_mask_backup;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_pcs_measure_tx_lane_skew(&phy_copy, vco,
                                              osr_mode, is_pam4,
                                              pma_width_multiplier,
                                              normalize_to_latest, &tx_max_skew));

        /* 6.3 Check for rx deskew valid. */
        for (i = 0; i < 1000; i++) {
            PHYMOD_USLEEP(10);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_chk_rx_ts_deskew_valid(&phy_copy,
                                                    speed_config_entry.bit_mux_mode,
                                                    &is_valid));
            if (is_valid) {
                break;
            }
        }
        if (!is_valid) {
            return PHYMOD_E_TIMEOUT;
        }

        /* 6.4 Update RX deksew. */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_mod_rx_1588_tbl_val(&phy_copy, speed_config_entry.bit_mux_mode,
                                             vco, osr_mode, is_pam4, normalize_to_latest,
                                             &rx_max_skew, &rx_min_skew, skew_per_vl,
                                             &rx_dsl_sel, &rx_psll_sel));
    }
    /* 7. Set SFD/SOP and byte offset. */
    phy_copy.access.lane_mask = 0x1 << start_lane;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_pcs_ts_config(&phy_copy, ts_offset, rx_base_addr, tx_base_addr));

#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_pcs_lane_swap_adjust(const plp_aperta2_phymod_phy_access_t* phy,
                                  uint32_t active_lane_map,
                                  uint32_t original_tx_lane_map,
                                  uint32_t original_rx_lane_map)
{
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_timesync_tx_info_get(const plp_aperta2_phymod_phy_access_t* phy, tscpmod_ts_tx_info_t* ts_tx_info)
{

    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane;
    uint32_t ts_info[3];
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(phy->access.lane_mask);

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    phy_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal) ;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_mem_read(&phy_copy, phymodMemTxTwostep1588Ts, ((start_lane > 7) ? (start_lane-8):start_lane), ts_info));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_tx_ts_info_unpack_tx_ts_tbl_entry(ts_info, ts_tx_info));

    return PHYMOD_E_NONE;
}

/*FIXME once GSH speed id table is ready, will update */
int plp_aperta2_tscp_phy_autoneg_speed_id_table_reload(const plp_aperta2_phymod_phy_access_t* phy, uint32_t gsh_header_enable)
{
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_pcs_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    int start_lane, num_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    if (enable == 1) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_enable_set(&phy_copy));
    } else if (enable == 0) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_disable_set(&phy_copy));
    }

    return PHYMOD_E_NONE;
}
#ifdef APERTA2_PM_UNSUPPORTED_API
int tscp_phy_synce_clk_ctrl_set(const plp_aperta2_phymod_phy_access_t* phy,
                                phymod_synce_clk_ctrl_t cfg)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    uint32_t sdm_val, data_rate_lane = 0;
    int osr_mode;
    uint32_t pll_div;
    phymod_firmware_lane_config_t firmware_lane_config = {0};
    tscpmod_refclk_t ref_clk;
    uint32_t vco_rate = 41250000, refclk_in_hz;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
         (plp_aperta2_tscpmod_synce_mode_set(&phy_copy, cfg.stg0_mode, cfg.stg1_mode));

    /* next check if SDM mode, if yes, needs to figure out the SDM value based on the current */
    if ((cfg.stg0_mode == 0x2) && (cfg.stg1_mode == 0x0)) {
        /* next figure out per lane speed */
        PHYMOD_MEMSET(&firmware_lane_config, 0, sizeof(firmware_lane_config));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscp_phy_firmware_lane_config_get(phy, &firmware_lane_config));

        /* get the PLL div from HW */
        phy_copy.access.pll_idx = 0;
        phy_copy.access.lane_mask = 0x1;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_INTERNAL_read_pll_div(&phy_copy, &pll_div));

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_refclk_get(&phy_copy, &ref_clk));

        if (ref_clk == TSCPMOD_REF_CLK_312P5MHZ) {
            refclk_in_hz = 312500000;
        } else {
            refclk_in_hz = 156250000;
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_get_vco_from_refclk_div(&phy_copy, refclk_in_hz, pll_div, &vco_rate, 0));

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_osr_mode_get(&phy_copy, &osr_mode));

        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscp_per_lane_data_rate_get(vco_rate, osr_mode, firmware_lane_config.ForcePAM4Mode, &data_rate_lane));

        if (data_rate_lane == 10000) {
            sdm_val = TSCPMOD_SYNCE_SDM_DIVISOR_10G_PER_LANE;
        } else if (data_rate_lane == 20000) {
            sdm_val = TSCPMOD_SYNCE_SDM_DIVISOR_20G_PER_LANE;
        } else if (data_rate_lane == 25000) {
            sdm_val = TSCPMOD_SYNCE_SDM_DIVISOR_25G_PER_LANE;
        } else if ((firmware_lane_config.ForcePAM4Mode) && (pll_div == TSCPMOD_PLL_MODE_DIV_170)) {
            sdm_val = TSCPMOD_SYNCE_SDM_DIVISOR_53G_VCO_PAM4;
        } else if ((firmware_lane_config.ForcePAM4Mode) && (pll_div == TSCPMOD_PLL_MODE_DIV_165)) {
            sdm_val = TSCPMOD_SYNCE_SDM_DIVISOR_51G_VCO_PAM4;
        } else {
            PHYMOD_DEBUG_ERROR(("Unsupported speeds\n"));
            return PHYMOD_E_UNAVAIL;
        }

        /* next configure the SDM value */
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_synce_clk_ctrl_set(&phy_copy, sdm_val));
    }

    return PHYMOD_E_NONE;
}

int tscp_phy_synce_clk_ctrl_get(const plp_aperta2_phymod_phy_access_t* phy,
                                phymod_synce_clk_ctrl_t *cfg)
{
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_synce_mode_get(&phy_copy, &(cfg->stg0_mode), &(cfg->stg1_mode)));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_synce_clk_ctrl_get(&phy_copy, &(cfg->sdm_val)));

    return PHYMOD_E_NONE;
}
#endif
/* This function will handle PCS ECC interrupts.
 * 1. Clear interrupt status.
 * 2. Re-load the table entry if error is in UM, AM table.
 * 3. Re-load the 1588 tables and SPEED_ID table entry for 1b error.
 * 4. Return is_handled = 0 if 2b-error happens and Phymod can not recover.
 */
#ifdef APERTA2_PM_UNSUPPORTED_API
int tscp_intr_handler(const plp_aperta2_phymod_phy_access_t* phy,
                      phymod_interrupt_type_t type,
                      uint32_t* is_handled,
                      phymod_intr_info_t *mem_info)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    tscpmod_intr_status_t intr_status;
    uint32_t speed_id_table_entry[TSCPMOD_SPEED_ID_ENTRY_SIZE];
    uint32_t ts_table_entry[TSCPMOD_TS_ENTRY_SIZE];
    int index;
    int native_addr;
    int mem_addr = 0;
    phymod_mem_type_t mem_type=phymodMemCount;

    uint32_t *plp_aperta2_tscp_am_table_entry;
    uint32_t *plp_aperta2_tscp_um_table_entry;

    plp_aperta2_tscp_am_table_entry = plp_aperta2_tscp_am_table_entry_get();
    plp_aperta2_tscp_um_table_entry = plp_aperta2_tscp_um_table_entry_get();

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_MEMSET(&intr_status, 0, sizeof(intr_status));
    intr_status.type = type;

    /* Get and clear interrupt status. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_intr_status_get(&phy_copy, &intr_status));

    phy_copy.access.lane_mask = 0x1;

    if (intr_status.is_2b_err) {
        *is_handled = 0;
    } else {
        *is_handled = 1;
    }

    switch (type) {
        case phymodIntrEccAMTable:
            if (intr_status.is_2b_err || intr_status.is_1b_err) {
                index = intr_status.err_addr;
                if (index >= TSCPMOD_HW_AM_TABLE_SIZE) {
                    return PHYMOD_E_FAIL;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_write(&phy_copy, phymodMemAMTable, index,
                                      (plp_aperta2_tscp_am_table_entry + index * TSCPMOD_AM_ENTRY_SIZE)));
                *is_handled = 1;
            }
            break;
        case phymodIntrEccSpeedTable:
            if (intr_status.is_1b_err) {
                index = intr_status.err_addr;
                if (index >= TSCPMOD_HW_SPEED_ID_TABLE_SIZE) {
                    return PHYMOD_E_FAIL;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_read(&phy_copy, phymodMemSpeedIdTable,
                                     index, speed_id_table_entry));
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_write(&phy_copy, phymodMemSpeedIdTable,
                                      index, speed_id_table_entry));
            }
            break;
        case phymodIntrEccUMTable:
            if (intr_status.is_2b_err || intr_status.is_1b_err) {
                index = intr_status.err_addr;
                if (index >= TSCPMOD_HW_UM_TABLE_SIZE) {
                    return PHYMOD_E_FAIL;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_write(&phy_copy, phymodMemUMTable, index,
                                      (plp_aperta2_tscp_um_table_entry + index * TSCPMOD_UM_ENTRY_SIZE)));
                *is_handled = 1;
            }
            break;
        case phymodIntrEccRx1588Mpp1: /* EVEN */
            if ((intr_status.is_1b_err) || (intr_status.is_2b_err)) {
                native_addr = intr_status.err_addr;

                mem_type = phymodMemRxLkup1588Mpp1;
                mem_addr = TSCPMOD_TS_RX_MPP_EVEN_BANK_NATIVE_TO_ADDR(native_addr) ;
            }
            if (intr_status.is_2b_err) {
                if (mem_addr >= TSCPMOD_TS_RX_MPP_MEM_SIZE) {
                    return PHYMOD_E_FAIL;
                }
                mem_info->mem_type = mem_type;
                mem_info->mem_addr = mem_addr;

            } else if (intr_status.is_1b_err) {
                index = intr_status.err_addr;
                if (index >= TSCPMOD_TS_RX_MPP_MEM_SIZE) {
                    return PHYMOD_E_FAIL;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_read(&phy_copy, mem_type,
                                     index, ts_table_entry));
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_write(&phy_copy, mem_type,
                                      index, ts_table_entry));
            }
            break;
        case phymodIntrEccRx1588Mpp0:
            if (intr_status.is_1b_err) {
                index = intr_status.err_addr;
                if (index >= TSCPMOD_TS_RX_MPP_MEM_SIZE) {
                    return PHYMOD_E_FAIL;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_read(&phy_copy, phymodMemRxLkup1588Mpp0,
                                     index, ts_table_entry));
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_write(&phy_copy, phymodMemRxLkup1588Mpp0,
                                      index, ts_table_entry));
            }
            break;
        case phymodIntrEccTx1588Mpp0:
            /*
             * Tscp only has one TX 1588 memory.
             * Here we use phymodIntrEccTx1588Mpp0 to represent
             * TX 1588 ECC error type.
             */
            if (intr_status.is_1b_err) {
                index = intr_status.err_addr;
                if (index >= TSCPMOD_TS_TX_MEM_SIZE) {
                    return PHYMOD_E_FAIL;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_read(&phy_copy, phymodMemTxLkup1588Mpp0,
                                     index, ts_table_entry));
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_write(&phy_copy, phymodMemTxLkup1588Mpp0,
                                      index, ts_table_entry));
            }
            break;
        default:
            break;
    }

    if (!(*is_handled)) {
        /* Disable the interrupt to avoid continuous firing */
        switch (type) {
            case phymodIntrEccRsFECMpp1:
            case phymodIntrEccRsFECMpp0:
            case phymodIntrEccRsFECRbufMpp1:
            case phymodIntrEccRsFECRbufMpp0:
                PHYMOD_IF_ERR_RETURN(plp_aperta2_tscpmod_interrupt_enable_set(&phy_copy,
                                                                  type, 0));
                break;
            default:
                break;

        }
    }


    return PHYMOD_E_NONE;
}
int tscp_phy_codec_mode_set(const plp_aperta2_phymod_phy_access_t* phy, phymod_phy_codec_mode_t codec_type)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    tscpmod_spd_id_tbl_entry_t speed_config_entry;
    uint32_t packed_entry[5];
    int start_lane, num_lane;
    uint32_t lane_reset, pcs_lane_enable;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_lane_soft_reset_get(&phy_copy, &lane_reset));

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_enable_get(&phy_copy, &pcs_lane_enable));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_disable_set(&phy_copy));
    }

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 1));
    }

    /* first read the current speed id entry for this port */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_mem_read(&phy_copy, phymodMemSpeedIdTable, TSCPMOD_FORCED_SPEED_ID_OFFSET + start_lane, packed_entry));

    plp_aperta2_tscpmod_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);

    /* update the codec field */
    speed_config_entry.codec_mode = codec_type;

    /* next pack the speed config entry into 5 word format */
    plp_aperta2_tscpmod_spd_ctrl_pack_spd_id_tbl_entry(&speed_config_entry, &packed_entry[0]);

    /* write back to the speed id HW table */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_mem_write(&phy_copy, phymodMemSpeedIdTable, TSCPMOD_FORCED_SPEED_ID_OFFSET + start_lane, &packed_entry[0]));

    /* release the ln dp reset */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_enable_set(&phy_copy));
    }

    return PHYMOD_E_NONE;
}

int tscp_phy_codec_mode_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_phy_codec_mode_t* codec_type)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    tscpmod_spd_id_tbl_entry_t speed_config_entry;
    uint32_t packed_entry[5];
    int start_lane, num_lane, speed_id;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    phy_copy.access.lane_mask = 1 << start_lane;

    /* first read speed id from resolved status */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_speed_id_get(&phy_copy, &speed_id));

    /* first read the current speed id entry for this port */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_mem_read(&phy_copy, phymodMemSpeedIdTable, speed_id, packed_entry));

    plp_aperta2_tscpmod_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, &speed_config_entry);

    /* the codeec type */
    *codec_type = speed_config_entry.codec_mode;

    return PHYMOD_E_NONE;
}
#endif

int plp_aperta2_tscp_phy_fec_bypass_indication_set(const plp_aperta2_phymod_phy_access_t* phy,
                                       uint32_t enable)
{
    int start_lane, num_lane;
    plp_aperta2_phymod_phy_access_t phy_copy;
    uint32_t lane_reset, pcs_lane_enable;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    /*first check if lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_lane_soft_reset_get(&phy_copy, &lane_reset));

    /*next check if PCS lane is in reset */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_enable_get(&phy_copy, &pcs_lane_enable));

    /* disable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_disable_set(&phy_copy));
    }

    /* if lane is not in reset, then reset the lane first */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 1));
    }

    phy_copy.access.lane_mask = 1 << start_lane;
    PHYMOD_IF_ERR_RETURN
      (plp_aperta2_tscpmod_fec_bypass_indication_set(&phy_copy, enable));

    /* release the lane soft reset bit */
    if (!lane_reset) {
        PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_peregrine5_pc_lane_soft_reset(&phy_copy, 0));
    }

    /* re-enable pcs lane if pcs lane not in rset */
    if (pcs_lane_enable) {
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_tscpmod_enable_set(&phy_copy));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_fec_bypass_indication_get(const plp_aperta2_phymod_phy_access_t* phy,
                                       uint32_t *enable)
{
    int start_lane, num_lane;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    phy_copy.access.lane_mask = 1 << start_lane;

    PHYMOD_IF_ERR_RETURN
      (plp_aperta2_tscpmod_fec_bypass_indication_get(&phy_copy, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_rs_fec_rxp_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* hi_ser_lh, uint32_t* hi_ser_live)
{
    int start_lane, num_lane;
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane));

    phy_copy.access.lane_mask = 1 << start_lane;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_rs_fec_hi_ser_get(&phy_copy, hi_ser_lh, hi_ser_live));

    return PHYMOD_E_NONE;
}

#ifdef APERTA2_PM_UNSUPPORTED_API
int tscp_phy_pmd_override_enable_set(const plp_aperta2_phymod_phy_access_t* phy,
                                           phymod_override_type_t pmd_override_type,
                                           uint32_t override_enable,
                                           uint32_t override_val)
{
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (tscpmod_pmd_override_enable_set(&phy_copy, pmd_override_type, override_enable, override_val));

    return PHYMOD_E_NONE;
}
#endif
/*
 * Enables/Disables 100G 4 lane FEC override
 *
 * value: 0x1    Enable for NOFEC 100G AN
 * register 0xc05e sc_x2_control_SW_spare1 lane 0 copy  is used
 * for this purpose
 */
int plp_aperta2_tscp_phy_fec_override_set (const plp_aperta2_phymod_phy_access_t* phy, uint32_t enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    uint32_t tvco_pll_div;
    uint32_t *plp_aperta2_tscp_spd_id_entry = plp_aperta2_tscp_spd_id_entry_get();
    uint32_t *plp_aperta2_tscp_spd_id_entry_100g_4lane_no_fec = plp_aperta2_tscp_spd_id_entry_100g_4lane_no_fec_get();

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1;

    if ((enable ==  0) || (enable == 1)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_tscpmod_fec_override_set(&phy_copy, enable));
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR :: Supported input values: 1 to set FEC override, 0 to disable FEC override\n"));
    }

    phy_copy.access.pll_idx = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_peregrine5_pc_INTERNAL_read_pll_div(&phy_copy, &tvco_pll_div));

    /* based on the current TVCO PLL div, decide which copy of speed id entry to load */
    /* first set the lane mask to be 0x1 */
    phy_copy.access.lane_mask = 1 << 0;
    if (enable) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write(&phy_copy, phymodMemSpeedIdTable,
                              TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_KR4,
                              plp_aperta2_tscp_spd_id_entry_100g_4lane_no_fec));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write(&phy_copy, phymodMemSpeedIdTable,
                              TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_CR4,
                              plp_aperta2_tscp_spd_id_entry_100g_4lane_no_fec));
    } else {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write(&phy_copy, phymodMemSpeedIdTable,
                              TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_KR4,
                              (plp_aperta2_tscp_spd_id_entry + (TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_KR4 * TSCPMOD_SPEED_ID_ENTRY_SIZE))));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_mem_write(&phy_copy, phymodMemSpeedIdTable,
                              TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_CR4,
                              (plp_aperta2_tscp_spd_id_entry + (TSCP_SPEED_ID_INDEX_100G_4_LANE_CL73_CR4 * TSCPMOD_SPEED_ID_ENTRY_SIZE))));
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_fec_override_get (const plp_aperta2_phymod_phy_access_t* phy, uint32_t* enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_tscpmod_fec_override_get(&phy_copy, enable));

    return PHYMOD_E_NONE;

}

int plp_aperta2_tscp_phy_interrupt_enable_set(const plp_aperta2_phymod_phy_access_t* phy,
                                        phymod_interrupt_type_t intr_type,
                                        uint32_t enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    tscpmod_intr_status_t intr_status;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    /* Get and clear interrupt status. */
    intr_status.type = intr_type;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_tscpmod_intr_status_get(&phy_copy, &intr_status));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_tscpmod_interrupt_enable_set(&phy_copy,
                                                      intr_type, enable));
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscp_phy_interrupt_enable_get(const plp_aperta2_phymod_phy_access_t* phy,
                                        phymod_interrupt_type_t intr_type,
                                        uint32_t* enable)
{
    plp_aperta2_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_tscpmod_interrupt_enable_get(&phy_copy,
                                                      intr_type, enable));
    return PHYMOD_E_NONE;
}

#ifdef APERTA2_PM_UNSUPPORTED_API
static
int _tscp_phy_power_mode_map(phymod_power_mode_t power_mode, enum plp_aperta2_srds_core_pwrdn_mode_enum *pwrdn_mode)
{
    switch (power_mode) {
    case phymodSerdesPWR_ON:
        *pwrdn_mode = PWR_ON;
        break;
    case phymodSerdesPWRDN:
        *pwrdn_mode = PWRDN;
        break;
    case phymodSerdesPWRDN_DEEP:
        *pwrdn_mode = PWRDN_DEEP;
        break;
    case phymodSerdesPWRDN_TX:
        *pwrdn_mode = PWRDN_TX;
        break;
    case phymodSerdesPWRDN_RX:
        *pwrdn_mode = PWRDN_RX;
        break;
    case phymodSerdesPWROFF_DEEP:
        *pwrdn_mode = PWROFF_DEEP;
        break;
    case phymodSerdesPWRDN_TX_DEEP:
        *pwrdn_mode = PWRDN_TX_DEEP;
        break;
    case phymodSerdesPWRDN_RX_DEEP:
        *pwrdn_mode = PWRDN_RX_DEEP;
        break;
    case phymodSerdesPWR_ON_TX:
        *pwrdn_mode = PWR_ON_TX;
        break;
    case phymodSerdesPWR_ON_RX:
        *pwrdn_mode = PWR_ON_RX;
        break;
    default:
        return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
}

/* Power on/down on core, lane or both*/
int tscp_phy_serdes_power_set(const plp_aperta2_phymod_phy_access_t* phy, phymod_power_option_t power_option, phymod_power_mode_t power_mode)
{
    plp_aperta2_phymod_phy_access_t phy_copy;
    enum plp_aperta2_srds_core_pwrdn_mode_enum pwrdn_mode;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (_tscp_phy_power_mode_map(power_mode, &pwrdn_mode));
    switch (power_option) {
        case phymodSerdesOptionLane:
            break;
        case phymodSerdesOptionCore:
            phy_copy.access.lane_mask = 0x1;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_core_pwrdn(&phy_copy, pwrdn_mode));
            if (power_mode == phymodSerdesPWR_ON) {
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_peregrine5_pc_core_dp_reset(&phy_copy, 0));
            }
            break;
        case phymodSerdesOptionAll:
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_pwrdn_all(&phy_copy, pwrdn_mode));
            break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_UNAVAIL,
                               (_PHYMOD_MSG("This power option is not supported\n")));
            break;
    }

    return PHYMOD_E_NONE;
}


/*The unit of period is ms*/
int tscp_phy_an_timer_set(const plp_aperta2_phymod_phy_access_t* phy, phymod_an_timer_t timer_type, uint32_t period)
{
    plp_aperta2_phymod_phy_access_t  phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    switch (timer_type) {
        case phymodAnTimerLinkFailInhibitLtPam4:
             /* configure the period on MPP0 */
            phy_copy.access.lane_mask = 0x1;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_an_link_fail_inhibit_timer_set(&phy_copy,
                                                        TSCPMOD_AN_FAIL_INHIBIT_TIMER_LT_PAM4,
                                                        period));
            /* configure the period on MPP1 */
            phy_copy.access.lane_mask = 0x10;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_an_link_fail_inhibit_timer_set(&phy_copy,
                                                        TSCPMOD_AN_FAIL_INHIBIT_TIMER_LT_PAM4,
                                                        period));
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
}

int tscp_phy_an_timer_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_an_timer_t timer_type, uint32_t* period)
{
    plp_aperta2_phymod_phy_access_t  phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1 << 0;
    switch (timer_type) {
        case phymodAnTimerLinkFailInhibitLtPam4:
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_tscpmod_an_link_fail_inhibit_timer_get(&phy_copy,
                                                             TSCPMOD_AN_FAIL_INHIBIT_TIMER_LT_PAM4,
                                                       period));
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
}
#endif
