/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/**************************************************************************************
 **************************************************************************************
 *  File Name     :  blackhawk_barchetta_pwr_mgt.c                                             *
 *  Created On    :  04 Nov 2015                                                      *
 *  Created By    :  Brent Roberts                                                    *
 *  Description   :  APIs for Serdes IPs                                              *
 *  Revision      :      *
 *                                                                                    *
 **************************************************************************************
 **************************************************************************************/

/** @file blackhawk_barchetta_pwr_mgt.c
 * Implementation of API power management functions
 */

#include "blackhawk_barchetta_pwr_mgt.h"
#include "blackhawk_barchetta_common.h"
#include "blackhawk_barchetta_functions.h"
#include "blackhawk_barchetta_internal.h"
#include "blackhawk_barchetta_internal_error.h"
#include "blackhawk_barchetta_select_defns.h"
#include "blackhawk_barchetta_config.h"



/***************************/
/*  Configure Serdes IDDQ  */
/***************************/

err_code_t plp_barchetta_blackhawk_barchetta_core_config_for_iddq(srds_access_t *sa__) {
  UNUSED(sa__);

  return (ERR_CODE_NONE);
}


err_code_t plp_barchetta_blackhawk_barchetta_lane_config_for_iddq(srds_access_t *sa__) {

  /* Use frc/frc_val to force all RX and TX clk_vld signals to 0 */
  EFUN(wr_pmd_rx_clk_vld_frc_val(0x0));
  EFUN(wr_pmd_rx_clk_vld_frc(0x1));
      EFUN(wr_pmd_tx_clk_vld_frc_val(0x0));
      EFUN(wr_pmd_tx_clk_vld_frc(0x1));

  /* Use frc/frc_val to force all pmd_rx_lock signals to 0 */
  EFUN(wr_rx_dsc_lock_frc_val(0x0));
  EFUN(wr_rx_dsc_lock_frc(0x1));

  /* Switch all the lane clocks to comclk by writing to RX/TX comclk_sel registers */
  EFUN(wr_ln_rx_s_comclk_sel(0x1));
    EFUN(wr_ln_tx_s_comclk_sel(0x1));

  /* Assert all the AFE pwrdn/reset pins using frc/frc_val to make sure AFE is in lowest possible power mode */
  EFUN(wr_afe_tx_pwrdn_frc_val(0x1));
  EFUN(wr_afe_tx_pwrdn_frc(0x1));
  EFUN(wr_afe_rx_pwrdn_frc_val(0x1));
  EFUN(wr_afe_rx_pwrdn_frc(0x1));
  EFUN(wr_afe_tx_reset_frc_val(0x1));
  EFUN(wr_afe_tx_reset_frc(0x1));
  EFUN(wr_afe_rx_reset_frc_val(0x1));
  EFUN(wr_afe_rx_reset_frc(0x1));

  /* Set pmd_iddq pin to enable IDDQ */
  return (ERR_CODE_NONE);
}

/****************************************************/
/*  Serdes Powerdown, ClockGate and Deep_Powerdown  */
/****************************************************/

err_code_t plp_barchetta_blackhawk_barchetta_core_pwrdn(srds_access_t *sa__, enum plp_barchetta_srds_core_pwrdn_mode_enum mode) {
    switch(mode) {
    case PWR_ON:
        EFUN(plp_barchetta_blackhawk_barchetta_INTERNAL_core_clkgate(sa__, 0));
        EFUN(wrc_ams_pll_pwrdn(0x0));
        EFUN(wrc_afe_s_pll_pwrdn(0x0));
        EFUN(plp_barchetta_blackhawk_barchetta_core_dp_reset(sa__, 0x0));
        break;
    case PWRDN:
        EFUN(plp_barchetta_blackhawk_barchetta_core_dp_reset(sa__, 1));
        EFUN(USR_DELAY_NS(500)); /* wait >50 comclk cycles  */
        EFUN(wrc_afe_s_pll_pwrdn(0x1));
        EFUN(USR_DELAY_NS(500)); /* wait >50 comclk cycles  */
        EFUN(wrc_ams_pll_pwrdn(0x1));
        break;
    case PWRDN_DEEP:
        {
            uint8_t lane_orig = plp_barchetta_blackhawk_barchetta_get_lane(sa__);
            uint8_t lane;
            uint8_t lanes_per_core;
            ESTM(lanes_per_core = rdc_revid_multiplicity());

            for (lane = 0; lane < lanes_per_core; ++lane) {
                EFUN(plp_barchetta_blackhawk_barchetta_set_lane(sa__,   lane));
                EFUN(wr_ln_rx_s_pwrdn(  0x1));
                EFUN(wr_ln_tx_s_pwrdn(  0x1));
                EFUN(plp_barchetta_blackhawk_barchetta_INTERNAL_lane_clkgate(sa__, 1));
                EFUN(wr_ln_dp_s_rstb(   0x0));
            }
            EFUN(plp_barchetta_blackhawk_barchetta_set_lane(sa__, lane_orig));
        }
        EFUN(plp_barchetta_blackhawk_barchetta_core_dp_reset(sa__, 1));
        EFUN(USR_DELAY_NS(500)); /* wait >50 comclk cycles  */
        {
            uint8_t pll_orig = plp_barchetta_blackhawk_barchetta_get_pll_idx(sa__);
            uint8_t pll;
            for (pll = 0; pll < DUAL_PLL_NUM_PLLS; ++pll) {
                EFUN(plp_barchetta_blackhawk_barchetta_set_pll_idx(sa__, pll));
                EFUN(wrc_afe_s_pll_pwrdn(0x1));
                EFUN(USR_DELAY_NS(500)); /* wait >50 comclk cycles  */
                EFUN(wrc_ams_pll_pwrdn(0x1));
            }
            EFUN(plp_barchetta_blackhawk_barchetta_set_pll_idx(sa__, pll_orig));
        }
        EFUN(plp_barchetta_blackhawk_barchetta_INTERNAL_core_clkgate(sa__, 1));
        break;
    default:
        EFUN(blackhawk_barchetta_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
        break;
    }
    return ERR_CODE_NONE;
}

err_code_t plp_barchetta_blackhawk_barchetta_lane_pwrdn(srds_access_t *sa__, enum plp_barchetta_srds_core_pwrdn_mode_enum mode) {

    switch(mode) {
    case PWR_ON:
        EFUN(wr_ln_tx_s_pwrdn(0x0));
        EFUN(wr_ln_rx_s_pwrdn(0x0));
        EFUN(plp_barchetta_blackhawk_barchetta_INTERNAL_lane_clkgate(sa__, 0));
        break;
    case PWRDN:
        /* do the RX first, since that is what is most users care about */
        EFUN(wr_ln_rx_s_pwrdn(0x1));
        EFUN(wr_ln_tx_s_pwrdn(0x1));
        break;
    case PWRDN_DEEP:
        /* do the RX first, since that is what is most users care about */
        EFUN(wr_ln_rx_s_pwrdn(0x1));
        EFUN(wr_ln_tx_s_pwrdn(0x1));
        EFUN(plp_barchetta_blackhawk_barchetta_INTERNAL_lane_clkgate(sa__, 1));
        EFUN(wr_ln_dp_s_rstb(0x0));
        break;
    case PWRDN_TX:
        EFUN(wr_ln_tx_s_pwrdn(0x1));
        break;
    case PWRDN_RX:
        EFUN(wr_ln_rx_s_pwrdn(0x1));
        break;
    default :
        return(blackhawk_barchetta_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
  return (ERR_CODE_NONE);
}
