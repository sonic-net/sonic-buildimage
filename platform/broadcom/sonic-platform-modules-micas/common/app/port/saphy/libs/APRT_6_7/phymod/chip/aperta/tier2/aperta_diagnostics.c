/*
 *
 *
 *  *
 *  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */

#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diagnostics_dispatch.h>

#ifdef PHYMOD_APERTA_SUPPORT
#include <tier1/aperta_pm_diag_seq.h>
#include <include/aperta_tscbh_diagnostics.h>
#include <include/blackhawk_diagnostics.h>
#include <tier1/aperta_cfg_seq.h>
#include <blackhawk/tier1/blackhawk_tsc_interface.h>
#include <blackhawk/tier1/blackhawk_tsc_functions.h>

int plp_aperta_phy_prbs_config_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , const plp_aperta_phymod_prbs_t* prbs)
{


    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_prbs_config_set(phy, flags, prbs));

    return PHYMOD_E_NONE;

}

int plp_aperta_phy_prbs_config_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , plp_aperta_phymod_prbs_t* prbs)
{

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_prbs_config_get(phy, flags, prbs));
    return PHYMOD_E_NONE;

}


int plp_aperta_phy_prbs_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , uint32_t enable)
{

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_prbs_enable_set(phy,flags ,enable));

    return PHYMOD_E_NONE;

}

int plp_aperta_phy_prbs_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , uint32_t* enable)
{

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_prbs_enable_get(phy, flags, enable));
    return PHYMOD_E_NONE;

}


int plp_aperta_phy_prbs_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_prbs_status_t* prbs_status)
{

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_prbs_status_get(phy, flags,  prbs_status));

    return PHYMOD_E_NONE;

}


int plp_aperta_core_diagnostics_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_core_diagnostics_t* diag)
{
    plp_aperta_phymod_phy_access_t *sa__ = (plp_aperta_phymod_phy_access_t *)core;
    blackhawk_tsc_core_state_st state;
    unsigned int lane_index = 0;

    PHYMOD_MEMSET(&state, 0, sizeof(blackhawk_tsc_core_state_st));
    PHYMOD_MEMSET(diag, 0, sizeof(plp_aperta_phymod_core_diagnostics_t));

    for (lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index++) {
        if (sa__->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__));
            EFUN(plp_aperta_blackhawk_tsc_INTERNAL_read_core_state(sa__, &state));
            diag->temperature = state.die_temp;
            diag->pll_range = state.analog_vco_range;
            break;
        }
    }
    return PHYMOD_E_NONE;
}


int plp_aperta_phy_diagnostics_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_diagnostics_t* diag)
{
    int lane_index = 0,rv = 0;
    plp_aperta_phymod_phy_access_t phy1;
    plp_aperta_phymod_phy_access_t *sa__ = &phy1;
    blackhawk_tsc_lane_state_st state;
    err_code_t __err = 0;

    PHYMOD_MEMCPY(sa__, phy,sizeof(plp_aperta_phymod_phy_access_t));

    for(lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index++) {
        if (((phy->access.lane_mask >> lane_index) & 1) == 0x1) {
           sa__->access.lane_mask = (1 << lane_index);
           PHYMOD_IF_ERR_RETURN(
                   plp_aperta_blackhawk_tsc_INTERNAL_read_lane_state(sa__, &state));
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
    return rv;

}

err_code_t plp_aperta_blackhawk_aperta_get_eye_size (srds_access_t *sa__, int8_t *eye_size) {
    int8_t data;
    uint8_t rx_lock, micro_stop;

    if(!eye_size) { 
      return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    ESTM(rx_lock = rd_pmd_rx_lock());
    if(!rx_lock) {
      *eye_size = 0;
      return (ERR_CODE_NONE);
    }

    /* stop micro so all accesses are consistent */
    {
        err_code_t err_code=ERR_CODE_NONE;
        micro_stop = plp_aperta_blackhawk_tsc_INTERNAL_stop_micro(sa__, rx_lock, &err_code);
        if(err_code) USR_PRINTF(("Unable to stop microcontroller,  following data is suspect\n"));
    }

    /* There is one internal RAM variable which could be used for eye quality.
    We have not characterized it.
    This is for NR mode only.
    use the smallest of these 4 read
        rdv_lvr_sts_1_0()
        rdv_lvr_sts_1_1()
        rdv_lvr_sts_1_2()
        rdv_lvr_sts_1_3()
    this represents the eye opening of the center eye. 
    */
    *eye_size = 127;

    ESTM(data = rdv_lvr_sts_1_0());
    if (data < *eye_size) {
      *eye_size = data;
    }

    ESTM(data = rdv_lvr_sts_1_1());
    if (data < *eye_size) {
      *eye_size = data;
    }

    ESTM(data = rdv_lvr_sts_1_2());
    if (data < *eye_size) {
      *eye_size = data;
    }

    ESTM(data = rdv_lvr_sts_1_3());
    if (data < *eye_size) {
      *eye_size = data;
    }

    /* re-enable micro */
    if (!micro_stop) {
        EFUN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__, 0));
    } 

    return (ERR_CODE_NONE);
}

int plp_aperta_phy_pam4_diagnostics_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_pam4_diagnostics_t* diag)
{
    int lane_index = 0,rv = 0;
    plp_aperta_phymod_phy_access_t phy1;
    plp_aperta_phymod_phy_access_t *sa__ = &phy1;
    blackhawk_tsc_lane_state_st state;
    err_code_t __err = 0;

    PHYMOD_MEMCPY(sa__, phy,sizeof(plp_aperta_phymod_phy_access_t));

    for(lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index++) {
        if (((phy->access.lane_mask >> lane_index) & 1) == 0x1) {
            sa__->access.lane_mask = (1 << lane_index);
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_blackhawk_tsc_INTERNAL_read_lane_state(sa__, &state));
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
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_blackhawk_aperta_get_eye_size (sa__, &diag->eye_size));
            break;
        }
    }
    return rv;

}



int plp_aperta_phy_pmd_info_dump(const plp_aperta_phymod_phy_access_t* phy, char* type)
{
    return plp_aperta_blackhawk_phy_pmd_info_dump(phy, type);
}

#include<aperta_pm_seq.h>

int plp_aperta_phy_eyescan_run(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_eyescan_mode_t mode, const plp_aperta_phymod_phy_eyescan_options_t* eyescan_options)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int lane_index = 0;
    plp_aperta_phymod_phy_eyescan_options_t temp_eye_option;
    plp_aperta_phymod_phy_inf_config_t port_config;
    aperta_device_aux_modes_t auxmode;

    PHYMOD_MEMCPY(&phy_copy, phy,sizeof(plp_aperta_phymod_phy_access_t));
    if (eyescan_options != NULL) {
        PHYMOD_MEMCPY(&temp_eye_option, eyescan_options, sizeof(plp_aperta_phymod_phy_eyescan_options_t));
    }
    port_config.device_aux_modes = &auxmode;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_interface_config_get(phy, 0, &port_config));
    temp_eye_option.linerate_in_khz = auxmode.lane_data_rate * 1000;
    if (mode == phymodEyescanModeFast) {
        flags = 0;
    }
    if (mode == phymodEyescanModeBERProj) {
        PHYMOD_EYESCAN_F_PROCESS_SET(flags);
    }

    for (lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index ++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            phy_copy.access.lane_mask = (1 << lane_index);
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_blackhawk_phy_eyescan_run(&phy_copy, flags, mode, &temp_eye_option));
        }
    }
    return PHYMOD_E_NONE;

}


int plp_aperta_phy_link_mon_enable_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_link_monitor_mode_t link_mon_mode, uint32_t enable)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}

int plp_aperta_phy_link_mon_enable_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_link_monitor_mode_t link_mon_mode, uint32_t* enable)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}


int plp_aperta_phy_link_mon_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* lock_status, uint32_t* lock_lost_lh, uint32_t* error_count)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}

int plp_aperta_phy_fec_correctable_counter_get(const plp_aperta_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{
    return plp_aperta_tscbh_phy_fec_cl91_correctable_counter_get(phy, fec_type, count);
}

int plp_aperta_phy_fec_uncorrectable_counter_get(const plp_aperta_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{
    return plp_aperta_tscbh_phy_fec_cl91_uncorrectable_counter_get(phy, fec_type, count);
}

int plp_aperta_phy_prbs_error_analyzer_proj(const plp_aperta_phymod_phy_access_t* phy, uint16_t prbs_error_fec_size, uint8_t hist_errcnt_thresh, uint32_t timeout_s)
{
    return plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_proj((srds_access_t*)phy, prbs_error_fec_size, hist_errcnt_thresh, timeout_s);
}

int plp_aperta_phy_pattern_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t enable, const plp_aperta_phymod_pattern_t* pattern)
{
    return plp_aperta_blackhawk_phy_pattern_enable_set(phy, enable, pattern);
}

int plp_aperta_phy_pattern_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *enable, plp_aperta_phymod_pattern_t* pattern)
{
    return plp_aperta_blackhawk_phy_pattern_enable_get(phy, enable, pattern);
}

#endif /* PHYMOD_APERTA_SUPPORT */
