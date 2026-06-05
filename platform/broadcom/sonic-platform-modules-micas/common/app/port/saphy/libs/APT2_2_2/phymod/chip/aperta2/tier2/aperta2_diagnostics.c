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
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diagnostics_dispatch.h>

#ifdef PHYMOD_APERTA2_SUPPORT
#include <tier1/aperta2_pm_diag_seq.h>
#include <tscp_diagnostics.h>
#include <peregrine5_pc_diagnostics.h>
#include <aperta2_cfg_seq.h>
#include <aperta2_sdk_intf.h>
#include <peregrine5_pc_interface.h>
#include <peregrine5_pc_functions.h>
#include <aperta2_pm_seq.h>
#include <aperta2_reg_access.h>

int plp_aperta2_phy_prbs_config_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , const plp_aperta2_phymod_prbs_t* prbs)
{        
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_prbs_config_set(phy, flags, prbs));
        
    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_prbs_config_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , plp_aperta2_phymod_prbs_t* prbs)
{        
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_prbs_config_get(phy, flags, prbs));
        
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_prbs_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , uint32_t enable)
{        
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_prbs_enable_set(phy, flags, enable));
    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_prbs_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , uint32_t* enable)
{        
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_prbs_enable_get(phy, flags, enable));
        
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_prbs_status_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_prbs_status_t* prbs_status)
{        
   
     PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_prbs_status_get(phy, flags,  prbs_status));
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_pattern_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t enable, const plp_aperta2_phymod_pattern_t* pattern)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}

int plp_aperta2_phy_pattern_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* enable, const plp_aperta2_phymod_pattern_t* pattern)
{        
    
    /* Place your code here */

        
    return PHYMOD_E_UNAVAIL;
    
}


int plp_aperta2_core_diagnostics_get(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_core_diagnostics_t* diag)
{       
#ifdef SERDES_API_FLOATING_POINT
    uint32_t data = 0;
    plp_aperta2_phymod_phy_access_t phy;
    float voltage = 0;
    if (core->access.lane_mask == 0x0) {
        PHYMOD_DEBUG_ERROR(("Invalid Lane mask\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_MEMCPY(&phy, core, sizeof(phy));

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_reg32_read(&phy, 0x5200601c, &data));
    /* Sensor data 0-10*/
    data = data & 0x7FF;
    diag->temperature =  ((-0.28833 * data) + 445.41 );
    diag->pll_range = 0xDEAD;

    /* Read voltage from one of the sensors - Reading VTMON_vmon_1v_0 */
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_reg32_read(&phy, 0x52006044, &data));

    /* Voltage sensor data is 0 to 10 bits */
    voltage = data & 0x7FF;

    /* Calculate millivolt value from the register value */
    diag->voltage = voltage = (voltage/2048) * 1002 /* Correction factor */;
    return PHYMOD_E_NONE;
#else
    return PHYMOD_E_UNAVAIL;
#endif
    
}


int plp_aperta2_phy_diagnostics_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_phy_diagnostics_t* diag)
{        
    
    return PHYMOD_E_UNAVAIL;

}


int plp_aperta2_phy_pam4_diagnostics_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_phy_pam4_diagnostics_t* diag)
{        
    int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy1;
    srds_access_t *sa__ = &phy1;
    peregrine5_pc_lane_state_st_t state;

    PHYMOD_MEMCPY(sa__, phy,sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&state, 0, sizeof(peregrine5_pc_lane_state_st_t));

    for(lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (((phy->access.lane_mask >> lane_index) & 1) == 0x1) {
            sa__->access.lane_mask = (1 << lane_index);
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_peregrine5_pc_INTERNAL_read_lane_state(sa__, &state));
            diag->signal_detect = state.sig_det;
            if (state.osr_mode.tx == 0) {
                diag->osr_mode = phymodOversampleMode1;
            } else if (state.osr_mode.tx == 1) {
                diag->osr_mode = phymodOversampleMode2;
            } else if (state.osr_mode.tx == 3) {
                diag->osr_mode = phymodOversampleMode5;
            }
            diag->rx_lock = state.pmd_lock;
            diag->tx_ppm = state.tx_ppm;
            diag->rx_ppm = state.ppm;
            diag->link_time = state.link_time;
            diag->pf_main   = state.PF_M;
            diag->pf2_ctrl  = state.PF_L;
            diag->pf3_ctrl  = state.PF_H;
            diag->vga = state.VGA;
            diag->dc_offset = state.DCO;
            diag->dfe1 = state.dfe_taps[0];
            diag->dfe2 = state.dfe_taps[1];
            diag->lane_reset_state = state.rx_rst;
            diag->tx_reset_state   = state.tx_rst;
            diag->uc_stop_state    = state.stop_state;
            diag->ucv_lane_status  = state.UC_STS;
            diag->tx_pll_select = state.tx_pll_select;
            diag->rx_pll_select = state.rx_pll_select;
            diag->txfir_pre3 =state.txfir.pre3;
            diag->txfir_pre2 =state.txfir.pre2;
            diag->txfir_pre  =state.txfir.pre1 ;
            diag->txfir_main =state.txfir.main;
            diag->txfir_post1 =state.txfir.post1;
            diag->txfir_post2=state.txfir.post2;
            diag->laneid        = state.laneid ;
            diag->rx_pam_mode   = state.rx_pam_mode;
            diag->CDR           = state.CDR ;
            PHYMOD_STRCPY(diag->tx_osr_mode_str, state.tx_osr_mode_str);
            PHYMOD_STRCPY(diag->rx_osr_mode_str, state.rx_osr_mode_str);
            diag->UC_CFG        = state.UC_CFG;
            diag->UC_STS_EXT    = state.UC_STS_EXT;
            diag->sig_det_chg   = state.sig_det_chg;
            diag->pmd_lock_chg  = state.pmd_lock_chg;
            diag->TP0           = state.TP0 ;
            diag->TP1           = state.TP1 ;
            diag->TP2           = state.TP2 ;
            diag->ffe_n3        = state.RXFFE_n3;
            diag->ffe_n2        = state.RXFFE_n2;
            diag->ffe_n1        = state.RXFFE_n1;
            diag->ffe_m         = state.RXFFE_m;
            diag->ffe_p1        = state.RXFFE_p1;
            diag->ffe_p2        = state.RXFFE_p2;
            diag->FLT_M         = state.FLT_M ;
            diag->FLT_S         = state.FLT_S ;
            diag->linktrn_en    = state.linktrn_en ;
            PHYMOD_STRCPY(diag->snr_str, state.snr_dfe);
            diag->tx_prec_en    = state.tx_prec_en ;
            diag->EYE_U = state.EYE_U;
            diag->EYE_M = state.EYE_M;
            diag->EYE_L = state.EYE_L;
            PHYMOD_STRCPY(diag->ber_str, state.BER);
            diag->rx_tuning_done = state.rx_tuning_done ;
            diag->enable_6taps   = state.enable_6taps   ;
            diag->disable_eye_display  = state.disable_eye_display  ;
            diag->tx_pam_mode          = state.tx_pam_mode          ;
            diag->tx_disable_status    = state.tx_disable_status    ;
            /*diag->txfir_nlc_upper_pct  = state.txfir_nlc_upper_pct  ;
            diag->txfir_nlc_lower_pct  = state.txfir_nlc_lower_pct  ;*/
            diag->txfir_use_pam4_range = state.txfir_use_pam4_range ;
            diag->tx_pmd_dp_invert     = state.tx_pmd_dp_invert     ;
            diag->rx_pmd_dp_invert     = state.rx_pmd_dp_invert     ;
            diag->rmt_lpbk_en          = state.rmt_lpbk_en          ;
            diag->dig_lpbk_en          = state.dig_lpbk_en          ;
            diag->ilb_en               = state.ilb_en               ;
            diag->tx_pi_en             = state.tx_pi_en             ;
            PHYMOD_STRCPY(diag->uc_sts_decoded, state.uc_sts_decoded);
            PHYMOD_STRCPY(diag->uc_sts_ext_decoded, state.uc_sts_ext_decoded);

            break;
        }
    }

    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_pmd_info_dump(const plp_aperta2_phymod_phy_access_t* phy, char* type)
{        
    
    return plp_aperta2_peregrine5_pc_phy_pmd_info_dump(phy, type);
}


int plp_aperta2_phy_eyescan_run(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_eyescan_mode_t mode, const plp_aperta2_phymod_phy_eyescan_options_t* eyescan_options)
{        
    plp_aperta2_phymod_phy_access_t phy_copy;
    int lane_index = 0;
    PHYMOD_MEMCPY(&phy_copy, phy,sizeof(plp_aperta2_phymod_phy_access_t));

    if (mode == phymodEyescanModeFast) {
        flags = 0;
        for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index ++) {
            if (phy->access.lane_mask & (1 << lane_index)) {
                phy_copy.access.lane_mask = (1 << lane_index);
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_peregrine5_pc_phy_eyescan_run(&phy_copy, flags, mode, eyescan_options));
            }
        }
        return PHYMOD_E_NONE;
    }
    if (mode == phymodEyescanModeBERProj) {
        plp_aperta2_phymod_phy_inf_config_t config;
        aperta2_device_aux_modes_t auxmode;
        PHYMOD_MEMSET(&auxmode, 0, sizeof(aperta2_device_aux_modes_t));
        config.device_aux_modes = &auxmode;
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_interface_config_get(phy, 0, &config));
        for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index ++) {
            if (phy->access.lane_mask & (1 << lane_index)) {
                phy_copy.access.lane_mask = (1 << lane_index);
                PHYMOD_IF_ERR_RETURN(plp_aperta2_peregrine5_pc_eye_margin_proj(&phy_copy, (auxmode.lane_data_rate * 1000 * 1000), 
                     eyescan_options->ber_proj_scan_mode, eyescan_options->ber_proj_timer_cnt,  eyescan_options->ber_proj_err_cnt));
            }
        }
        return PHYMOD_E_NONE;
    }

        
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_link_mon_enable_set(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_link_monitor_mode_t link_mon_mode, uint32_t enable)
{        
    
    /* Place your code here */

        
    return PHYMOD_E_UNAVAIL;
    
}

int plp_aperta2_phy_link_mon_enable_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_link_monitor_mode_t link_mon_mode, uint32_t* enable)
{        
    
    /* Place your code here */

        
    return PHYMOD_E_UNAVAIL;
    
}


int plp_aperta2_phy_link_mon_status_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* lock_status, uint32_t* lock_lost_lh, uint32_t* error_count)
{        
    
    /* Place your code here */

        
    return PHYMOD_E_UNAVAIL;
    
}


int plp_aperta2_phy_fec_correctable_counter_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{        
    
    return  plp_aperta2_tscp_phy_fec_cl91_correctable_counter_get(phy, fec_type, count);
    
}


int plp_aperta2_phy_fec_uncorrectable_counter_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{        
    
    return  plp_aperta2_tscp_phy_fec_cl91_uncorrectable_counter_get(phy, fec_type, count);
    
}


int plp_aperta2_phy_prbs_error_analyzer_proj(const plp_aperta2_phymod_phy_access_t* phy, uint16_t prbs_error_fec_size, uint8_t hist_errcnt_thresh, uint32_t timeout_s)
{        
   
    return plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_proj((srds_access_t*)phy, 0 /*peregrine5_pc_RS_544_514_10*/, timeout_s);
}


#endif /* PHYMOD_APERTA2_SUPPORT */
