/*
 *
 * $Id: barchetta_cfg_seq.c,  $
 *
 *  *
 *  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
 *  *
 *  *
 *
 */

#include "barchetta_diag_seq.h"
#include "bcmi_barchetta_defs.h"
#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diagnostics_dispatch.h>
#include "barchetta_cfg_seq.h"
#include "blackhawk_barchetta_interface.h"

#define BARCHETTA_IS_ON              1
#define BARCHETTA_SSPRQ_PATTERN      0
#define BARCHETTA_QPRBS13_PATTERN    1
#define BARCHETTA_SQUAREWAVE_PATTERN 2
#define BARCHETTA_PCSSCRIDLE_PATTERN 3
#define BARCHETTA_KP4PRBS_PATTERN    4

#define BARCHETTA_ONE_2_CONSECUTIVE  0
#define BARCHETTA_ONE_4_CONSECUTIVE  1
#define BARCHETTA_ONE_8_CONSECUTIVE  2
#define BARCHETTA_ONE_16_CONSECUTIVE 3

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
STATIC
int _plp_barchetta_prbs_poly_phymod_to_bhawk(plp_barchetta_phymod_prbs_poly_t phymod_poly,
        enum plp_barchetta_srds_prbs_polynomial_enum *poly) {
    switch (phymod_poly) {
    case phymodPrbsPoly7:
        *poly = PRBS_7;
        break;
    case phymodPrbsPoly9:
        *poly = PRBS_9;
        break;
    case phymodPrbsPoly11:
        *poly = PRBS_11;
        break;
    case phymodPrbsPoly15:
        *poly = PRBS_15;
        break;
    case phymodPrbsPoly23:
        *poly = PRBS_23;
        break;
    case phymodPrbsPoly31:
        *poly = PRBS_31;
        break;
    case phymodPrbsPoly58:
        *poly = PRBS_58;
        break;
    case phymodPrbsPoly49:
        *poly = PRBS_49;
        break;
    case phymodPrbsPoly10:
        *poly = PRBS_10;
         break;
    case phymodPrbsPoly20:
        *poly = PRBS_20;
         break;
    case phymodPrbsPoly13:
        *poly = PRBS_13;
         break;

    default:
            PHYMOD_RETURN_WITH_ERR
                (PHYMOD_E_PARAM,\
                (_PHYMOD_MSG("unsupported polynomial ")));
        break;
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
STATIC
int _plp_barchetta_prbs_poly_bhawk_to_phymod(enum plp_barchetta_srds_prbs_polynomial_enum poly,
        plp_barchetta_phymod_prbs_poly_t *phymod_poly) {
    switch (poly) {
    case PRBS_7:
        *phymod_poly = phymodPrbsPoly7;
        break;
    case PRBS_9:
        *phymod_poly = phymodPrbsPoly9;
        break;
    case PRBS_11:
        *phymod_poly = phymodPrbsPoly11;
        break;
    case PRBS_15:
        *phymod_poly = phymodPrbsPoly15;
        break;
    case PRBS_23:
        *phymod_poly = phymodPrbsPoly23;
        break;
    case PRBS_31:
        *phymod_poly = phymodPrbsPoly31;
        break;
    case PRBS_58:
        *phymod_poly = phymodPrbsPoly58;
        break;
    case PRBS_49:
        *phymod_poly = phymodPrbsPoly49;
        break;
    case PRBS_10:
        *phymod_poly = phymodPrbsPoly10;
        break;
    case PRBS_20:
        *phymod_poly = phymodPrbsPoly20;
        break;
    case PRBS_13:
        *phymod_poly = phymodPrbsPoly13;
        break;

    default:
            PHYMOD_RETURN_WITH_ERR
                (PHYMOD_E_INTERNAL,\
                (_PHYMOD_MSG("unknown polymial")));
        break;
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_prbs_config_set(const plp_barchetta_phymod_phy_access_t* phy,
        uint32_t flags, const plp_barchetta_phymod_prbs_t* prbs) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    enum plp_barchetta_srds_prbs_polynomial_enum serdes_poly;
    uint8_t num_of_max_lanes = 0;
    int checker_mode = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_prbs_poly_phymod_to_bhawk(prbs->poly,&serdes_poly));
            if (PHYMOD_PRBS_DIRECTION_RX_GET(flags) || (flags ==0)) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

                /* Defaults to HYSTERESIS, May need to change */
                if (_plp_barchetta_is_pam4_mode(phy)) {
                    checker_mode =  PRBS_INITIAL_SEED_NO_HYSTERESIS;
                } else {
                    checker_mode =  PRBS_INITIAL_SEED_HYSTERESIS;
                }
                PHYMOD_IF_ERR_RETURN(
                      plp_barchetta_blackhawk_barchetta_config_rx_prbs(&phy->access,serdes_poly, checker_mode, prbs->invert));
            }
            if (PHYMOD_PRBS_DIRECTION_TX_GET(flags) || (flags ==0)) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_config_tx_prbs(&phy->access, serdes_poly, prbs->invert));
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_prbs_config_get(const plp_barchetta_phymod_phy_access_t* phy,
        uint32_t flags, plp_barchetta_phymod_prbs_t* prbs) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    uint8_t prbs_inv = 0;
    enum plp_barchetta_srds_prbs_polynomial_enum serdes_poly = 0;
    enum plp_barchetta_srds_prbs_checker_mode_enum prbs_checker_mode = 0;
    uint8_t num_of_max_lanes = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            if (PHYMOD_PRBS_DIRECTION_RX_GET(flags) || (flags ==0)) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_get_rx_prbs_config(&phy->access, &serdes_poly, &prbs_checker_mode, &prbs_inv));
            }
            if (PHYMOD_PRBS_DIRECTION_TX_GET(flags) || (flags ==0)) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_get_tx_prbs_config(&phy->access, &serdes_poly, &prbs_inv));
            }
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_prbs_poly_bhawk_to_phymod(serdes_poly, &prbs->poly));
            prbs->invert = prbs_inv;
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_prbs_enable_set(const plp_barchetta_phymod_phy_access_t* phy,
        uint32_t flags, uint32_t enable) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    uint8_t num_of_max_lanes = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            if (PHYMOD_PRBS_DIRECTION_RX_GET(flags) || (flags ==0)) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_rx_prbs_en(&phy->access, enable));
            }
            if (PHYMOD_PRBS_DIRECTION_TX_GET(flags) || (flags ==0)) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_tx_prbs_en(&phy->access, enable));

            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_prbs_enable_get(const plp_barchetta_phymod_phy_access_t* phy,
        uint32_t flags, uint32_t* enable) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    uint8_t num_of_max_lanes = 0, bh_enable = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            if (PHYMOD_PRBS_DIRECTION_RX_GET(flags) || (flags ==0)) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_get_rx_prbs_en(&phy->access, &bh_enable));
                *enable = bh_enable;
            }
            if (PHYMOD_PRBS_DIRECTION_TX_GET(flags) || (flags ==0)) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_get_tx_prbs_en(&phy->access, &bh_enable));
                *enable = bh_enable;
            }
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_prbs_status_get(const plp_barchetta_phymod_phy_access_t* phy,
        uint32_t flags, plp_barchetta_phymod_prbs_status_t* prbs_status) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t logical_lane  = 0;
    uint8_t lane_index    = 0;
    uint8_t prbs_lock     = 0;
    uint8_t lock_lost     = 0;
    uint32_t prbs_err_cnt = 0;
    uint8_t num_of_max_lanes  = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    prbs_status->prbs_lock = 0xFFFF;
    prbs_status->prbs_lock_loss = 0;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_prbs_chk_lock_state(&phy->access, &prbs_lock));
            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_prbs_err_count_state(&phy->access, &prbs_err_cnt, &lock_lost));
            prbs_status->prbs_lock &= prbs_lock;
            prbs_status->error_count |= prbs_err_cnt;
            prbs_status->prbs_lock_loss |= lock_lost;
        }
    }
    return PHYMOD_E_NONE;
}


/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_diagnostics_get(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_phy_diagnostics_t* diag) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    blackhawk_barchetta_lane_state_st state;
    err_code_t __err = 0;
    uint8_t num_of_max_lanes = 0;

    PHYMOD_MEMSET(&state,    0, sizeof(blackhawk_barchetta_lane_state_st));
    PHYMOD_MEMSET(diag,      0, sizeof(plp_barchetta_phymod_phy_diagnostics_t));
    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_init_blackhawk_barchetta_info(&phy->access));

            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_INTERNAL_read_lane_state(&phy->access, &state));
            diag->signal_detect = state.sig_det;
            diag->osr_mode = state.osr_mode.tx_rx;
            diag->rx_lock = state.rx_lock;
            diag->tx_ppm = state.tx_ppm;
            diag->clk90_offset = state.clk90;
            diag->clkp1_offset = state.clkp1;
            diag->p1_lvl = state.p1_lvl;
            diag->dfe1_dcd = state.dfe1_dcd;
            diag->dfe2_dcd = state.dfe2_dcd;
            diag->eyescan.heye_left = state.heye_left;
            diag->eyescan.heye_right = state.heye_right;
            diag->eyescan.veye_upper = state.veye_upper;
            diag->eyescan.veye_lower = state.veye_lower;
            diag->link_time = state.link_time;
            diag->pf_main = state.pf_main;
            diag->pf_hiz = state.pf_hiz;
            diag->pf2_ctrl = state.pf2_ctrl;
            diag->vga = state.vga;
            diag->dc_offset = state.dc_offset;
            diag->p1_lvl_ctrl = state.p1_lvl_ctrl;
            diag->dfe1 = state.dfe1;
            diag->dfe2 = state.dfe2;
            diag->dfe3 = state.dfe3;
            diag->dfe4 = state.dfe4;
            diag->dfe5 = state.dfe5;
            diag->dfe6 = state.dfe6;
            diag->br_pd_en = state.br_pd_en;
            diag->pf3_ctrl = state.pf3_ctrl;
            diag->lane_reset_state = state.reset_state;
            diag->tx_reset_state = state.tx_reset_state;
            diag->uc_stop_state = state.stop_state;
            diag->ucv_lane_status = state.ucv_status;
            diag->tx_pll_select = state.tx_pll_select;
            diag->rx_pll_select = state.rx_pll_select;

            if (!rd_txfir_tap_en()) {
                diag->txfir_pre = state.txfir.tap[0];
                diag->txfir_main = state.txfir.tap[1];
                diag->txfir_post1 = state.txfir.tap[2];
                diag->txfir_pre2 = 0;
                diag->txfir_post2 = 0;
                diag->txfir_post3 = 0;
            } else {
                diag->txfir_pre2 = state.txfir.tap[0];
                diag->txfir_pre = state.txfir.tap[1];
                diag->txfir_main = state.txfir.tap[2];
                diag->txfir_post1 = state.txfir.tap[3];
                diag->txfir_post2 = state.txfir.tap[4];
                diag->txfir_post3 = state.txfir.tap[5];
            }
            /* Not Suported By BARCH Bhawk*/
            /*diag->ffe_enable;
             diag->ffe1;
             diag->ffe2;
             diag->tx_amp_ctrl = state.tx_amp;*/
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_eyescan_run(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags,
        plp_barchetta_phymod_eyescan_mode_t mode,
        const plp_barchetta_phymod_phy_eyescan_options_t* eyescan_options) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t logical_lane = 0;
    uint8_t lane_index   = 0;
    uint8_t num_of_max_lanes = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_init_blackhawk_barchetta_info(&phy->access));
            if (mode == phymodEyescanModeFast) {
                PHYMOD_DIAG_OUT(("EYE PLOT for PHY:%d lane:%d side:%s\n", phy->access.addr, logical_lane, (phy->port_loc == phymodPortLocSys) ? "SYS" : "LINE"));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_display_eye_scan(&phy->access));
            } else if (mode == phymodEyescanModeBERProj) {
                barchetta_lane_data_rate_t lane_data_rate;
                USR_DOUBLE d_lane_rate;
                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_restore_lane_data_rate(phy, lane_index, &lane_data_rate));
                d_lane_rate = lane_data_rate ;
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_eye_margin_proj(&phy->access, d_lane_rate*1000*1000,
                                eyescan_options->ber_proj_scan_mode,
                                eyescan_options->ber_proj_timer_cnt,
                                eyescan_options->ber_proj_err_cnt));
                (void)lane_data_rate;
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_link_mon_enable_set(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_link_monitor_mode_t link_mon_mode, uint32_t enable) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    int port_number = 0xFF, port_lane = 0;
    int lane_index = 0;
    uint8_t num_of_max_lanes = 0;
    BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_t pcs_main_ctrl;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    PHYMOD_MEMSET(&pcs_main_ctrl, 0, sizeof(BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_t));

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("Error : Invalid port number as 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
               _plp_barchetta_get_port_lane(phy, num_of_max_lanes, (1 << lane_index), port_number, &port_lane));

            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice (phy, port_number, (1 << port_lane), BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, 0xFFFF));
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_PCS_MON_LANE_MAIN_CONTROLr(&phy->access, &pcs_main_ctrl));

            switch (link_mon_mode) {
            case phymodLinkMonPCS49_1x10G:
                BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_MOD_MODEf_SET(pcs_main_ctrl, 0);
                break;
            case phymodLinkMonPCS82_4x10G:
                BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_MOD_MODEf_SET(pcs_main_ctrl, 1);
                break;
            case phymodLinkMonPCS82_2x25G:
                BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_MOD_MODEf_SET(pcs_main_ctrl, 1);
                BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_MODE_2LOGICALLANES_MUXEDf_SET(pcs_main_ctrl, 1);
                break;
            case phymodLinkMonPCS82_4x25G:
                BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_MOD_MODEf_SET(pcs_main_ctrl, 1);
                BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_MODE_100Gf_SET(pcs_main_ctrl, 1);
                break;
            default:
                PHYMOD_DEBUG_ERROR(("Error : Invalid port link mon mode\n"));
                return PHYMOD_E_UNAVAIL;
            }
            if (enable == 1) {
                BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_MOD_ENAf_SET(pcs_main_ctrl, 1);
            } else {
                BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_MOD_ENAf_SET(pcs_main_ctrl, 0);
            }
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_WRITE_PCS_MON_LANE_MAIN_CONTROLr(&phy->access, pcs_main_ctrl));
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_link_mon_enable_get(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_link_monitor_mode_t link_mon_mode, uint32_t* enable) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    int port_number = 0xFF, port_lane = 0;
    int lane_index = 0;
    uint8_t num_of_max_lanes = 0;
    BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_t pcs_main_ctrl;
    PHYMOD_MEMSET(&pcs_main_ctrl, 0, sizeof(BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_t));

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("Error : Invalid port number as 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
               _plp_barchetta_get_port_lane(phy, num_of_max_lanes, (1 << lane_index), port_number, &port_lane));

            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice (phy, port_number, (1 << port_lane), BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, 0xFFFF));

            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_PCS_MON_LANE_MAIN_CONTROLr(&phy->access, &pcs_main_ctrl));
            *enable = BCMI_BARCHETTA_PCS_MON_LANE_MAIN_CONTROLr_MOD_ENAf_GET(pcs_main_ctrl);
            break;
        }
    }

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_link_mon_status_get(const plp_barchetta_phymod_phy_access_t* phy,
        uint32_t* lock_status, uint32_t* lock_lost_lh, uint32_t* error_count) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    int port_number = 0xFF, port_lane = 0;
    int lane_index = 0;
    uint8_t num_of_max_lanes = 0;
    BCMI_BARCHETTA_PCS_MON_LANE_MAIN_STATUSr_t main_sts;
    BCMI_BARCHETTA_PCS_MON_LANE_CL49_BER_STATUSr_t cl49_ber_sts;
    BCMI_BARCHETTA_PCS_MON_LANE_BER_CNT_MSBr_t ber_msb;
    BCMI_BARCHETTA_PCS_MON_LANE_LIVE_STATUSr_t live_sts;
    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("Error : Invalid port number as 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
               _plp_barchetta_get_port_lane(phy, num_of_max_lanes, (1 << lane_index), port_number, &port_lane));

            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice (phy, port_number, (1 << port_lane), BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, 0xFFFF));

            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_PCS_MON_LANE_MAIN_STATUSr (&phy->access, &main_sts));
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_PCS_MON_LANE_CL49_BER_STATUSr(&phy->access, &cl49_ber_sts));
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_PCS_MON_LANE_BER_CNT_MSBr(&phy->access, &ber_msb));
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_PCS_MON_LANE_LIVE_STATUSr(&phy->access, &live_sts));

            *lock_status = BCMI_BARCHETTA_PCS_MON_LANE_LIVE_STATUSr_BLOCK_LOCKf_GET(live_sts);
            *lock_lost_lh = BCMI_BARCHETTA_PCS_MON_LANE_MAIN_STATUSr_BLOCK_LOCK_LLf_GET(main_sts);
            *error_count = (( BCMI_BARCHETTA_PCS_MON_LANE_BER_CNT_MSBr_BER_COUNT_MSBf_GET(ber_msb) << 6 ) |
                             BCMI_BARCHETTA_PCS_MON_LANE_CL49_BER_STATUSr_CL49IEEE_BERf_GET(cl49_ber_sts));
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: To display PRBS Error Analyzer Projection

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_prbs_error_analyzer_proj(const plp_barchetta_phymod_phy_access_t* phy, uint16_t prbs_error_fec_size, uint8_t hist_errcnt_thresh, uint32_t timeout_s)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t logical_lane = 0;
    uint8_t lane_index   = 0;
    uint8_t num_of_max_lanes = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_display_prbs_error_analyzer_proj(&phy->access, prbs_error_fec_size, hist_errcnt_thresh, timeout_s));
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: To retrieve FEC corrected code word counter

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_fec_correctable_counter_get(const plp_barchetta_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{
    BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_CORR_BLK_CNT_UPPERr_t cl74_fec_corr_upper;
    BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_CORR_BLK_CNT_LOWERr_t cl74_fec_corr_lower;
    BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_CORR_CW_CNT_UPPERr_t   cl91_fec_corr_upper;
    BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_CORR_CW_CNT_LOWERr_t   cl91_fec_corr_lower;
    barchetta_package_info_t pkg_info;
    int32_t  port_number      = 0xFF;
    int32_t  port_lane        = 0;
    uint32_t count_per_lane   = 0;
    uint8_t  num_of_max_lanes = 0;
    uint8_t  lane_index       = 0;
    *count = 0;

    PHYMOD_MEMSET(&cl91_fec_corr_upper, 0, sizeof(BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_CORR_CW_CNT_UPPERr_t));
    PHYMOD_MEMSET(&cl91_fec_corr_lower, 0, sizeof(BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_CORR_CW_CNT_LOWERr_t));
    PHYMOD_MEMSET(&cl74_fec_corr_upper, 0, sizeof(BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_CORR_BLK_CNT_UPPERr_t));
    PHYMOD_MEMSET(&cl74_fec_corr_lower, 0, sizeof(BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_CORR_BLK_CNT_LOWERr_t));
    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));

    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("Error : Invalid port number as 0xFF\n"));
        return PHYMOD_E_PARAM;
    }

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_port_lane(phy, num_of_max_lanes, (1 << lane_index), port_number, &port_lane));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_set_slice(phy, port_number, (1 << port_lane), BarchettaRegisterSelectIngress, BarchettaRegisterTypePMD, 0xFFFF));

            if((fec_type == phymod_fec_CL91) || (fec_type == phymod_fec_CL108)) {
                PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_IEEE_CL91_RX_REGS_FEC_CORR_CW_CNT_UPPERr(&phy->access, &cl91_fec_corr_upper));
                PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_IEEE_CL91_RX_REGS_FEC_CORR_CW_CNT_LOWERr(&phy->access, &cl91_fec_corr_lower));
                count_per_lane = ( (BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_CORR_CW_CNT_UPPERr_FEC_CORR_CW_CNT_UPPERf_GET(cl91_fec_corr_upper) << 16) |
                                   (BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_CORR_CW_CNT_LOWERr_FEC_CORR_CW_CNT_LOWERf_GET(cl91_fec_corr_lower) & 0xFFFF)
                                 );
            } else if(fec_type == phymod_fec_CL74) {
                PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_IEEE_CL74_FEC_REGS_FEC_CORR_BLK_CNT_UPPERr(&phy->access, &cl74_fec_corr_upper));
                PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_IEEE_CL74_FEC_REGS_FEC_CORR_BLK_CNT_LOWERr(&phy->access, &cl74_fec_corr_lower));
                count_per_lane = ( (BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_CORR_BLK_CNT_UPPERr_FEC_CORR_BLK_CNT_UPPERf_GET(cl74_fec_corr_upper) << 16) |
                                   (BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_CORR_BLK_CNT_LOWERr_FEC_CORR_BLK_CNT_LOWERf_GET(cl74_fec_corr_lower) & 0xFFFF)
                                 );
            }
            if(count_per_lane > 0xffffffff - *count) {
                *count = 0xffffffff ;
            } else {
                *count += count_per_lane;
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: To retrieve FEC uncorrected code word counter.

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_phy_fec_uncorrectable_counter_get(const plp_barchetta_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{
    BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_UNCORR_BLK_CNT_UPPERr_t cl74_fec_uncorr_upper;
    BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_UNCORR_BLK_CNT_LOWERr_t cl74_fec_uncorr_lower;
    BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_UNCORR_CW_CNT_UPPERr_t   cl91_fec_uncorr_upper;
    BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_UNCORR_CW_CNT_LOWERr_t   cl91_fec_uncorr_lower;
    barchetta_package_info_t pkg_info;
    int32_t  port_number      = 0xFF;
    int32_t  port_lane        = 0;
    uint32_t count_per_lane   = 0;
    uint8_t  num_of_max_lanes = 0;
    uint8_t  lane_index       = 0;
    *count = 0;

    PHYMOD_MEMSET(&cl91_fec_uncorr_upper, 0, sizeof(BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_UNCORR_CW_CNT_UPPERr_t));
    PHYMOD_MEMSET(&cl91_fec_uncorr_lower, 0, sizeof(BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_UNCORR_CW_CNT_LOWERr_t));
    PHYMOD_MEMSET(&cl74_fec_uncorr_upper, 0, sizeof(BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_UNCORR_BLK_CNT_UPPERr_t));
    PHYMOD_MEMSET(&cl74_fec_uncorr_lower, 0, sizeof(BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_UNCORR_BLK_CNT_LOWERr_t));
    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("Error : Invalid port number as 0xFF\n"));
        return PHYMOD_E_PARAM;
    }

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_port_lane(phy, num_of_max_lanes, (1 << lane_index), port_number, &port_lane));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_set_slice(phy, port_number, (1 << port_lane), BarchettaRegisterSelectIngress, BarchettaRegisterTypePMD, 0xFFFF));

            if((fec_type == phymod_fec_CL91) || (fec_type == phymod_fec_CL108)) {
                PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_IEEE_CL91_RX_REGS_FEC_UNCORR_CW_CNT_UPPERr(&phy->access, &cl91_fec_uncorr_upper));
                PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_IEEE_CL91_RX_REGS_FEC_UNCORR_CW_CNT_LOWERr(&phy->access, &cl91_fec_uncorr_lower));
                count_per_lane = ( (BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_UNCORR_CW_CNT_UPPERr_FEC_UNCORR_CW_CNT_UPPERf_GET(cl91_fec_uncorr_upper) << 16) |
                                   (BCMI_BARCHETTA_IEEE_CL91_RX_REGS_FEC_UNCORR_CW_CNT_LOWERr_FEC_UNCORR_CW_CNT_LOWERf_GET(cl91_fec_uncorr_lower) & 0xFFFF)
                                 );
            } else if(fec_type == phymod_fec_CL74) {
                PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_IEEE_CL74_FEC_REGS_FEC_UNCORR_BLK_CNT_UPPERr(&phy->access, &cl74_fec_uncorr_upper));
                PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_IEEE_CL74_FEC_REGS_FEC_UNCORR_BLK_CNT_LOWERr(&phy->access, &cl74_fec_uncorr_lower));
                count_per_lane = ( (BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_UNCORR_BLK_CNT_UPPERr_FEC_UNCORR_BLK_CNT_UPPERf_GET(cl74_fec_uncorr_upper) << 16) |
                                   (BCMI_BARCHETTA_IEEE_CL74_FEC_REGS_FEC_UNCORR_BLK_CNT_LOWERr_FEC_UNCORR_BLK_CNT_LOWERf_GET(cl74_fec_uncorr_lower) & 0xFFFF)
                                 );
            }
            if(count_per_lane > 0xffffffff - *count) {
                *count = 0xffffffff ;
            } else {
                *count += count_per_lane;
            }
        }
    }
    return PHYMOD_E_NONE;
}

int _plp_barchetta_phy_pattern_enable_set(const plp_barchetta_phymod_phy_access_t* phy,
                                      uint32_t enable,
                                      const plp_barchetta_phymod_pattern_t* patt)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t patt_len;
    unsigned int *patt_type;
    uint8_t patt_switch;
    uint8_t patt_mode_len;
    uint8_t lane_index = 0;
    uint8_t logical_lane = 0;
    uint8_t num_of_max_lanes = 0;
    char *hexpatt = "0xFF00";

    if ((patt == NULL) || (patt->pattern == NULL)) {
        PHYMOD_RETURN_WITH_ERR
                (PHYMOD_E_PARAM,\
                (_PHYMOD_MSG("User passed NULL parameter")));
    }
    patt_type = patt->pattern;
    patt_switch = enable;
    patt_mode_len = patt->pattern_len;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy), 
                                                      BarchettaRegisterTypePMD, logical_lane));
            patt_len = 16;
            if (patt_switch == BARCHETTA_IS_ON) {
                switch(*patt_type) {
                    case BARCHETTA_SQUAREWAVE_PATTERN:
                        switch(patt_mode_len) {
                            case BARCHETTA_ONE_2_CONSECUTIVE:
                                hexpatt = "0xCCCC";
                                break;
                            case BARCHETTA_ONE_4_CONSECUTIVE:
                                hexpatt = "0xF0F0";
                                break;
                            case BARCHETTA_ONE_8_CONSECUTIVE:
                                hexpatt = "0xFF00";
                                break;
                            case BARCHETTA_ONE_16_CONSECUTIVE:
                                hexpatt = "0xFFFF0000";
                                patt_len = 32;
                                break;
                        }
                        break;

                    default:
                        PHYMOD_DEBUG_ERROR(("Error : Only Square Wave Pattern supported for now\n"));
                        return PHYMOD_E_PARAM;
                }
                PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_config_shared_tx_pattern(pa, patt_len, hexpatt));
            }
            PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_tx_shared_patt_gen_en(pa, patt_switch, patt_len));
        }
    }
    return PHYMOD_E_NONE;
}

int _plp_barchetta_phy_pattern_enable_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t *enable, 
                                      plp_barchetta_phymod_pattern_t* pattern)
{
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    unsigned int *patt_type;
    uint8_t lane_index = 0;
    uint8_t logical_lane = 0;
    uint8_t num_of_max_lanes = 0;
    uint32_t patt_hw = 0;
    uint16_t msb,lsb = 0;

    if ((pattern == NULL) || (pattern->pattern == NULL)) {
        PHYMOD_RETURN_WITH_ERR
                (PHYMOD_E_PARAM,\
                (_PHYMOD_MSG("User passed NULL parameter")));
    }
    patt_type = pattern->pattern;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy), 
                                                      BarchettaRegisterTypePMD, logical_lane));
            switch(*patt_type) {
                case BARCHETTA_SQUAREWAVE_PATTERN:
                    break;

                default:
                    PHYMOD_DEBUG_ERROR(("Error : Only Square Wave Pattern supported for now\n"));
                    return PHYMOD_E_PARAM;
            }

            ESTM(*enable = rd_patt_gen_en());
            ESTM(lsb = rdc_patt_gen_seq_0());
            ESTM(msb = rdc_patt_gen_seq_1());
            patt_hw = (msb << 16) | lsb;

            if((msb == 0xFF00) || (msb == 0x00FF)) {
                pattern->pattern_len = BARCHETTA_ONE_8_CONSECUTIVE;
            } else if((patt_hw == 0xFFFF0000) || (patt_hw == 0x0000FFFF)){
                pattern->pattern_len = BARCHETTA_ONE_16_CONSECUTIVE;
            } else if((msb == 0xF0F0) || (msb == 0x0F0F)){
                pattern->pattern_len = BARCHETTA_ONE_4_CONSECUTIVE;
            } else if(msb == 0xCCCC){
                pattern->pattern_len = BARCHETTA_ONE_2_CONSECUTIVE;
            } else {
                pattern->pattern_len = 0;
            }
            break;
        }
    }
    return PHYMOD_E_NONE;
}
