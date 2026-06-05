/*----------------------------------------------------------------------
 * $Id: peregrine5_pc_cfg_seq.h,v 1.1.2.2 2013/09/17 21:11:10 wniu Exp $
 *
 * Broadcom Corporation
 * Proprietary and Confidential information
 * All rights reserved
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without the
 * prior written consent of Broadcom Corporation.
 *---------------------------------------------------------------------
 * File       : peregrine5_pc_cfg_seq.h
 * Description: c functions implementing Tier1s for Osprey Serdes Driver
 *---------------------------------------------------------------------*/
/*
 *  $Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.$
 *  $Id$
*/


#ifndef PEREGRINE5_PC_CFG_SEQ_H
#define PEREGRINE5_PC_CFG_SEQ_H

#include "common/srds_api_err_code.h"
#include <phymod/phymod_diagnostics.h>
#include "peregrine5_pc_prbs.h"

typedef struct {
  int8_t pll_pwrdn;
  int8_t tx_s_pwrdn;
  int8_t rx_s_pwrdn;
} power_status_t;

typedef struct {
  int8_t revid_model;
  int8_t revid_process;
  int8_t revid_bonding;
  int8_t revid_rev_number;
  int8_t revid_rev_letter;
} peregrine5_pc_rev_id0_t;

typedef struct {
  int8_t revid_eee;
  int8_t revid_llp;
  int8_t revid_pir;
  int8_t revid_cl72;
  int8_t revid_micro;
  int8_t revid_mdio;
  int8_t revid_multiplicity;
} peregrine5_pc_rev_id1_t;

typedef enum {
  TX = 0,
  Rx
} tx_rx_t;

typedef enum {
    PEREGRINE5_PC_PRBS_POLYNOMIAL_7 = 0,
    PEREGRINE5_PC_PRBS_POLYNOMIAL_9,
    PEREGRINE5_PC_PRBS_POLYNOMIAL_11,
    PEREGRINE5_PC_PRBS_POLYNOMIAL_15,
    PEREGRINE5_PC_PRBS_POLYNOMIAL_23,
    PEREGRINE5_PC_PRBS_POLYNOMIAL_31,
    PEREGRINE5_PC_PRBS_POLYNOMIAL_58,
    PEREGRINE5_PC_PRBS_POLYNOMIAL_TYPE_COUNT
} peregrine5_pc_prbs_polynomial_type_t;

#define PATTERN_MAX_SIZE 8


extern err_code_t _peregrine5_pc_pmd_mwr_reg_byte(srds_access_t *pa, uint16_t addr, uint16_t mask, uint8_t lsb, uint8_t val);
extern uint8_t _peregrine5_pc_pmd_rde_field_byte(srds_access_t *pa, uint16_t addr, uint8_t shift_left, uint8_t shift_right, err_code_t *err_code_p);
extern uint16_t _peregrine5_pc_pmd_rde_field(srds_access_t *pa, uint16_t addr, uint8_t shift_left, uint8_t shift_right, err_code_t *err_code_p);
err_code_t plp_aperta2_peregrine5_pc_tx_pi_control_get(srds_access_t *pa,  int16_t *value);
err_code_t plp_aperta2_peregrine5_pc_tx_rx_polarity_set(srds_access_t *pa, uint32_t tx_pol, uint32_t rx_pol);
err_code_t plp_aperta2_peregrine5_pc_tx_rx_polarity_get(srds_access_t *pa, uint32_t *tx_pol, uint32_t *rx_pol);
err_code_t plp_aperta2_peregrine5_pc_uc_active_set(srds_access_t *pa, uint32_t enable);
err_code_t plp_aperta2_peregrine5_pc_uc_active_get(srds_access_t *pa, uint32_t *enable);
err_code_t plp_aperta2_peregrine5_pc_pmd_force_signal_detect(srds_access_t *pa, uint8_t force_en, uint8_t force_val);
err_code_t plp_aperta2_peregrine5_pc_pmd_force_signal_detect_get(srds_access_t *sa__, uint8_t *force_en, uint8_t *force_val);
err_code_t plp_aperta2_peregrine5_pc_dig_lpbk_get(srds_access_t *pa, uint32_t *lpbk);
err_code_t plp_aperta2_peregrine5_pc_rmt_lpbk_get(srds_access_t *pa, uint32_t *lpbk);
err_code_t plp_aperta2_peregrine5_pc_pmd_lane_map_get(srds_access_t *pa, uint32_t *tx_lane_map, uint32_t *rx_lane_map);
err_code_t plp_aperta2_peregrine5_pc_identify(srds_access_t *sa__, peregrine5_pc_rev_id0_t *rev_id0, peregrine5_pc_rev_id1_t *rev_id1);
err_code_t plp_aperta2_peregrine5_pc_pmd_ln_h_rstb_pkill_override(srds_access_t *pa, uint16_t val);
err_code_t plp_aperta2_peregrine5_pc_lane_soft_reset(srds_access_t *pa, uint32_t enable);   /* pmd core soft reset */
err_code_t plp_aperta2_peregrine5_pc_lane_soft_reset_get(srds_access_t *pa, uint32_t *enable);
err_code_t plp_aperta2_peregrine5_pc_lane_hard_soft_reset_release(srds_access_t *pa, uint32_t enable);
err_code_t plp_aperta2_peregrine5_pc_clause72_control(srds_access_t *pc, uint32_t cl_72_en);        /* CLAUSE_72_CONTROL */
err_code_t plp_aperta2_peregrine5_pc_clause72_control_get(srds_access_t *pc, uint32_t *cl_72_en);   /* CLAUSE_72_CONTROL */
err_code_t plp_aperta2_peregrine5_pc_pmd_cl72_receiver_status(srds_access_t *pa, uint32_t *status);
err_code_t plp_aperta2_peregrine5_pc_ucode_init(srds_access_t *pa );
err_code_t plp_aperta2_peregrine5_pc_pram_firmware_enable(srds_access_t *pa, int enable, int wait);
err_code_t plp_aperta2_peregrine5_pc_pmd_reset_seq(srds_access_t *pa, int pmd_touched);
err_code_t plp_aperta2_peregrine5_pc_signal_detect(srds_access_t *pa, uint32_t *signal_detect);
err_code_t plp_aperta2_peregrine5_pc_electrical_idle_set(srds_access_t *pa, uint8_t en);
err_code_t plp_aperta2_peregrine5_pc_electrical_idle_get(srds_access_t *pa, uint8_t *en);
err_code_t plp_aperta2_peregrine5_pc_tx_shared_patt_gen_en_get(srds_access_t *pa, uint8_t *enable);
err_code_t plp_aperta2_peregrine5_pc_config_shared_tx_pattern_idx_get(srds_access_t *pa, uint32_t *pattern_len, uint32_t *pattern);
err_code_t plp_aperta2_peregrine5_pc_tx_disable_get(srds_access_t *pa, uint8_t *enable);
err_code_t plp_aperta2_peregrine5_pc_comclk_set(srds_access_t *pa, plp_aperta2_phymod_ref_clk_t ref_clock);

err_code_t peregrine5_pc_channel_loss_set(srds_access_t *sa__, uint32_t loss_in_db);
err_code_t peregrine5_pc_channel_loss_get(srds_access_t *sa__, uint32_t *loss_in_db);
err_code_t plp_aperta2_peregrine5_pc_tx_tap_mode_get(srds_access_t *sa__, uint8_t *mode);
err_code_t plp_aperta2_peregrine5_pc_pam4_tx_pattern_enable_get(srds_access_t *sa__, plp_aperta2_phymod_pattern_t pattern_type, uint32_t* enable);
err_code_t plp_aperta2_peregrine5_pc_signalling_mode_status_get(srds_access_t *sa__, phymod_phy_signalling_method_t *mode);
err_code_t plp_aperta2_peregrine5_pc_tx_nrz_mode_get(srds_access_t *sa__, uint16_t *tx_nrz_mode);
err_code_t plp_aperta2_peregrine5_pc_tx_pam4_precoder_enable_set(srds_access_t *sa__, int enable);
err_code_t plp_aperta2_peregrine5_pc_tx_pam4_precoder_enable_get(srds_access_t *sa__, int *enable);
err_code_t plp_aperta2_peregrine5_pc_speed_config_get(uint32_t speed, uint32_t *pll_multiplier, uint32_t *is_pam4, uint32_t *osr_mode);

 /* Get the PLL powerdown status */
err_code_t plp_aperta2_peregrine5_pc_pll_pwrdn_get(srds_access_t *sa__, uint32_t *is_pwrdn);
err_code_t plp_aperta2_peregrine5_pc_rx_ppm(srds_access_t *sa__, int16_t *rx_ppm);
/* Set/Get clk4sync_en, clk4sync_div */
err_code_t plp_aperta2_peregrine5_pc_clk4sync_enable_set(srds_access_t *sa__, uint32_t en, uint32_t div);
err_code_t plp_aperta2_peregrine5_pc_clk4sync_enable_get(srds_access_t *sa__, uint32_t *en, uint32_t *div);
err_code_t plp_aperta2_peregrine5_pc_pll_lock_get(srds_access_t *sa__, uint32_t *pll_lock);
err_code_t plp_aperta2_peregrine5_pc_lane_pll_selection_set(srds_access_t *pa, uint32_t pll_index);
err_code_t plp_aperta2_peregrine5_pc_lane_pll_selection_get(srds_access_t *pa, uint32_t *pll_index);
err_code_t plp_aperta2_peregrine5_pc_rx_protect_nrzmux_set(srds_access_t *sa__, uint8_t mux_val);
err_code_t plp_aperta2_peregrine5_pc_rx_pmd_lock_status_get(srds_access_t *sa__, uint32_t *pmd_rx_locked, uint32_t *pmd_lock_changed);
err_code_t plp_aperta2_peregrine5_pc_lane_dp_reset_state_get(srds_access_t *sa__, uint32_t *reset_state);
err_code_t plp_aperta2_peregrine5_pc_osr_mode_set(srds_access_t *sa__, int osr_mode);
err_code_t plp_aperta2_peregrine5_pc_osr_mode_get(srds_access_t *sa__, int *osr_mode);
err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_compute_proj_without_print(srds_access_t *sa__,
        peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config,
        peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status);
err_code_t plp_aperta2_peregrine5_pc_tx_clock_div34_enable_set(srds_access_t *sa__, int enable);
err_code_t plp_aperta2_peregrine5_pc_pmd_ln_h_pwrdn_pkill_override(srds_access_t *sa__, uint16_t val);

err_code_t plp_aperta2_peregrine5_pc_pam4_lp_has_precoder_enable_get(srds_access_t *sa__, int *enable);
#endif /* PEREGRINE5_PC_CFG_SEQ_H */
