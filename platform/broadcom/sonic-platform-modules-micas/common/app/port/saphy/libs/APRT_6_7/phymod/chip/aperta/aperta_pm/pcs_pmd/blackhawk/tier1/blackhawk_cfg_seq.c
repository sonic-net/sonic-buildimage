/*
 *
 * $Id: falcon.c,v 1.2.2.26 2013/09/16 12:23:15 hoangngo Exp $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *
 */


#include <phymod/phymod.h>
#include <phymod/phymod_system.h>
#include <bcmi_blackhawk_xgxs_defs.h>
#include <phymod/phymod_util.h>
#include "blackhawk_cfg_seq.h"
#include "blackhawk_tsc_fields.h"
#include "blackhawk_tsc_field_access.h"
#include "blackhawk_tsc_dependencies.h"
#include "blackhawk_tsc_interface.h"
#include "blackhawk_tsc_functions.h"
#include "public/blackhawk_api_uc_vars_rdwr_defns_public.h"
#include <tier1/aperta_reg_access.h>

err_code_t plp_aperta_blackhawk_tx_rx_polarity_set( plp_aperta_phymod_phy_access_t *sa__, uint32_t tx_pol, uint32_t rx_pol)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  if (tx_pol != 0xFFFF) {
      __err = (uint32_t) wr_tx_pmd_dp_invert(tx_pol);
      if(__err) return(__err);
  }
  if (rx_pol != 0xFFFF) {
      __err = (uint32_t) wr_rx_pmd_dp_invert(rx_pol);
      if(__err) return(__err);
  }

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_lane_pll_selection_set( plp_aperta_phymod_phy_access_t *sa__, uint32_t pll_index)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err = (uint32_t) wr_pll_select(pll_index);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_lane_pll_selection_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *pll_index)
{
    err_code_t __err;
    __err=ERR_CODE_NONE;
    *pll_index = (uint32_t) rd_pll_select();
    if(__err) return(__err);

    return ERR_CODE_NONE;
}


err_code_t plp_aperta_blackhawk_tx_rx_polarity_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *tx_pol, uint32_t *rx_pol)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *tx_pol = (uint32_t) rd_tx_pmd_dp_invert();
  if(__err) return(__err);
  *rx_pol = (uint32_t) rd_rx_pmd_dp_invert();
  if(__err) return(__err);
  return ERR_CODE_NONE;

}

err_code_t plp_aperta_blackhawk_uc_active_set( plp_aperta_phymod_phy_access_t *sa__, uint32_t enable)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err=wrc_uc_active(enable);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_uc_active_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *enable)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *enable = (uint32_t) rdc_uc_active();
  if(__err) return(__err);

  return ERR_CODE_NONE;
}


/*
err_code_t blackhawk_uc_reset( plp_aperta_phymod_phy_access_t *sa__, uint32_t enable)
{
  return ERR_CODE_NONE;
}
*/

err_code_t plp_aperta_blackhawk_force_tx_set_rst ( plp_aperta_phymod_phy_access_t *sa__, uint32_t rst)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_force_tx_get_rst ( plp_aperta_phymod_phy_access_t *sa__, uint32_t *rst)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_force_rx_set_rst ( plp_aperta_phymod_phy_access_t *sa__, uint32_t rst)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_force_rx_get_rst ( plp_aperta_phymod_phy_access_t *sa__, uint32_t *rst)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_prbs_tx_inv_data_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *inv_data)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_prbs_rx_inv_data_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *inv_data)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_prbs_tx_poly_get( plp_aperta_phymod_phy_access_t *sa__, blackhawk_prbs_polynomial_type_t *prbs_poly)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_prbs_rx_poly_get( plp_aperta_phymod_phy_access_t *sa__, blackhawk_prbs_polynomial_type_t *prbs_poly)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_prbs_tx_enable_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *enable)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_prbs_rx_enable_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *enable)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pmd_force_signal_detect( plp_aperta_phymod_phy_access_t *sa__, uint8_t force_en, uint8_t force_val)
{
    err_code_t __err;
    __err=ERR_CODE_NONE;
    __err = wr_signal_detect_frc(force_en);
    if(__err) return(__err);
    __err = wr_signal_detect_frc_val(force_val);
    if(__err) return(__err);

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pmd_force_signal_detect_get( plp_aperta_phymod_phy_access_t *sa__, uint8_t *force_en, uint8_t *force_val)
{
    err_code_t __err;
    __err=ERR_CODE_NONE;
    *force_en = rd_signal_detect_frc();
    if(__err) return(__err);
    *force_val = rd_signal_detect_frc_val();
    if(__err) return(__err);

  return ERR_CODE_NONE;
}


err_code_t plp_aperta_blackhawk_pll_mode_set( plp_aperta_phymod_phy_access_t *sa__, int pll_mode)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pll_mode_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *pll_mode)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_afe_pll_reg_set( plp_aperta_phymod_phy_access_t *sa__, const plp_aperta_phymod_afe_pll_t *afe_pll)
{

    AMS_PLL_PLL_CTL2r_t reg;

    AMS_PLL_PLL_CTL2r_CLR(reg);

    if(afe_pll->afe_pll_change_default) {
        AMS_PLL_PLL_CTL2r_AMS_PLL_IQPf_SET(reg, afe_pll->ams_pll_iqp);
        PHYMOD_IF_ERR_RETURN(MODIFY_AMS_PLL_PLL_CTL2r(sa__, reg));
    }

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_afe_pll_reg_get( plp_aperta_phymod_phy_access_t *sa__, plp_aperta_phymod_afe_pll_t *afe_pll)
{
  return ERR_CODE_NONE;
}


err_code_t plp_aperta_blackhawk_osr_mode_set( plp_aperta_phymod_phy_access_t *sa__, int osr_mode)
{
  err_code_t __err;
  __err=ERR_CODE_NONE;
  __err=wr_osr_mode_frc_val(osr_mode);
  if(__err) return(__err);
  __err=wr_osr_mode_frc(1);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_osr_mode_get( plp_aperta_phymod_phy_access_t *sa__, int *osr_mode)
{
  int osr_forced;
  err_code_t __err;
  __err=ERR_CODE_NONE;
  *osr_mode = 0;
  osr_forced = rd_osr_mode_frc();
  if(osr_forced) {
    *osr_mode = rd_osr_mode_frc_val();
    if(__err) return(__err);
  }
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_dig_lpbk_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *lpbk)
{
    err_code_t __err;
    __err=ERR_CODE_NONE;
    *lpbk = rd_dig_lpbk_en();
    if(__err) return(__err);
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_rmt_lpbk_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *lpbk)
{
    err_code_t __err;
    __err=ERR_CODE_NONE;
    *lpbk = rd_rmt_lpbk_en();
    if(__err) return(__err);

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_core_soft_reset( plp_aperta_phymod_phy_access_t *sa__)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_core_soft_reset_release( plp_aperta_phymod_phy_access_t *sa__, uint32_t enable)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_core_soft_reset_read( plp_aperta_phymod_phy_access_t *sa__, uint32_t *enable)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_lane_soft_reset_read( plp_aperta_phymod_phy_access_t *sa__, uint32_t *enable)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pmd_tx_disable_pin_dis_set( plp_aperta_phymod_phy_access_t *sa__, uint32_t enable)
{
   err_code_t __err;
  __err=ERR_CODE_NONE;
  __err=wr_pmd_tx_disable_pkill(enable);
  if(__err) return(__err);

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pmd_tx_disable_pin_dis_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *enable)
{
    err_code_t __err;
    __err=ERR_CODE_NONE;
    *enable = rd_pmd_tx_disable_pkill();
    if(__err) return(__err);

  return ERR_CODE_NONE;
}

/* set powerdown for tx or rx */
/* tx_rx == 1 => disable (enable) power for Tx */
/* tx_rx != 0 => disable (enable) power for Rx */
/* pwrdn == 0 => enable power */
/* pwrdn == 1 => disable power */
err_code_t plp_aperta_blackhawk_tsc_pwrdn_set( plp_aperta_phymod_phy_access_t *sa__, int tx_rx, int pwrdn)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_pwrdn_get( plp_aperta_phymod_phy_access_t *sa__, power_status_t *pwrdn)
{

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pcs_lane_swap_tx( plp_aperta_phymod_phy_access_t *sa__, uint32_t tx_lane_map)
{

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pmd_loopback_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *enable)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pmd_cl72_enable_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *enable)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pmd_cl72_receiver_status( plp_aperta_phymod_phy_access_t *sa__, uint32_t *status)
{
    err_code_t __err;
    __err = ERR_CODE_NONE;
    *status = rd_linktrn_ieee_receiver_status(); if(__err) return(__err);
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pram_firmware_enable( plp_aperta_phymod_phy_access_t *sa__, int enable, int wait)   /* release the pmd core soft reset */
{
    err_code_t __err;
    uint8_t micro_orig, num_micros, micro_idx;

    __err = ERR_CODE_NONE;
    if (enable == 1) {
        __err = wrc_micro_pramif_ahb_wraddr_msw(0); if(__err) return(__err);
        __err = wrc_micro_pramif_ahb_wraddr_lsw(0); if(__err) return(__err);

        __err = wrc_micro_pram_if_rstb(1); if(__err) return(__err);
        __err = wrc_micro_pramif_en(1);    if(__err) return(__err);

        EFUN(wrc_micro_cr_crc_prtsel(0));
        EFUN(wrc_micro_cr_crc_init(1));                       /* Initialize the HW CRC calculation */
        EFUN(wrc_micro_cr_crc_init(0));
        EFUN(wrc_micro_cr_crc_calc_en(1));

        if (wait) {
          PHYMOD_USLEEP(500);
        }
    } else {
        /* block writing to program RAM */
        EFUN(wrc_micro_ra_wrdatasize(0x2));                   /* Select 32bit transfers as default */
        __err = wrc_micro_cr_ignore_micro_code_writes(1);  if(__err) return(__err);
        __err = wrc_micro_pramif_en(0); if(__err) return(__err);

        EFUN(wrc_micro_cr_crc_calc_en(0));

        micro_orig = plp_aperta_blackhawk_tsc_get_micro_idx(sa__);
        num_micros = rdc_micro_num_uc_cores();
        if(__err) return(__err);
        for (micro_idx = 0; micro_idx < num_micros; micro_idx++) {
            __err = plp_aperta_blackhawk_tsc_set_micro_idx(sa__, micro_idx);
            if(__err) return(__err);
            __err = wrc_micro_core_clk_en(1);
            if(__err) return(__err);
        }
        __err = plp_aperta_blackhawk_tsc_set_micro_idx(sa__, micro_orig);
        if(__err) return(__err);
    }
    return ERR_CODE_NONE;
}


err_code_t plp_aperta_blackhawk_pmd_lane_swap ( plp_aperta_phymod_phy_access_t *sa__, uint32_t lane_map) {

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pmd_lane_map_get ( plp_aperta_phymod_phy_access_t *sa__, uint32_t *tx_lane_map, uint32_t *rx_lane_map) {

    err_code_t __err;
    uint32_t tmp_tx_lane = 0;
    uint32_t tmp_rx_lane = 0;
    *tx_lane_map = 0;
    *rx_lane_map = 0;

    __err = ERR_CODE_NONE;
    tmp_tx_lane = rdc_tx_lane_addr_0( );
    if(__err) return(__err);
    *tx_lane_map |= tmp_tx_lane ;
    tmp_tx_lane = rdc_tx_lane_addr_1( );
    if(__err) return(__err);
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 1);
    tmp_tx_lane = rdc_tx_lane_addr_2( );
    if(__err) return(__err);
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 2);
    tmp_tx_lane = rdc_tx_lane_addr_3( );
    if(__err) return(__err);
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 3);
    tmp_tx_lane = rdc_tx_lane_addr_4( );
    if(__err) return(__err);
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 4);
    tmp_tx_lane = rdc_tx_lane_addr_5( );
    if(__err) return(__err);
    *tx_lane_map |= (tmp_tx_lane  & 0xf) << (4 * 5);
    tmp_tx_lane = rdc_tx_lane_addr_6( );
    if(__err) return(__err);
    *tx_lane_map |= (tmp_tx_lane  & 0xf) << (4 * 6);
    tmp_tx_lane = rdc_tx_lane_addr_7( );
    if(__err) return(__err);
    *tx_lane_map |= (tmp_tx_lane  & 0xf) << (4 * 7);

    tmp_rx_lane = rdc_rx_lane_addr_0( );
    if(__err) return(__err);
    *rx_lane_map |= (tmp_rx_lane & 0xf);
    tmp_rx_lane = rdc_rx_lane_addr_1( );
    if(__err) return(__err);
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 1);
    tmp_rx_lane = rdc_rx_lane_addr_2( );
    if(__err) return(__err);
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 2);
    tmp_rx_lane = rdc_rx_lane_addr_3( );
    if(__err) return(__err);
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 3);
    tmp_rx_lane = rdc_rx_lane_addr_4( );
    if(__err) return(__err);
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 4);
    tmp_rx_lane = rdc_rx_lane_addr_5( );
    if(__err) return(__err);
    *rx_lane_map |= (tmp_rx_lane  & 0xf) << (4 * 5);
    tmp_rx_lane = rdc_rx_lane_addr_6( );
    if(__err) return(__err);
    *rx_lane_map |= (tmp_rx_lane  & 0xf) << (4 * 6);
    tmp_rx_lane = rdc_rx_lane_addr_7( );
    if(__err) return(__err);
    *rx_lane_map |= (tmp_rx_lane  & 0xf) << (4 * 7);

    __err = ERR_CODE_NONE;

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tx_pi_control_get( plp_aperta_phymod_phy_access_t *sa__, int16_t* value)
{
    err_code_t __err;
    uint8_t override_enable;

    __err = ERR_CODE_NONE;

    override_enable = rd_tx_pi_freq_override_en();
    if(__err) return(__err);
    if (override_enable) {
        *value = rd_tx_pi_freq_override_val();
        if(__err) return(__err);
    } else {
        *value = 0;
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_identify( plp_aperta_phymod_phy_access_t *sa__, blackhawk_rev_id0_t *rev_id0, blackhawk_rev_id1_t *rev_id1)
{
  err_code_t __err;

  rev_id0->revid_rev_letter =0;
  rev_id0->revid_rev_number =0;
  rev_id0->revid_bonding    =0;
  rev_id0->revid_process    =0;
  rev_id0->revid_model      =0;

  rev_id1->revid_multiplicity =0;
  rev_id1->revid_mdio         =0;
  rev_id1->revid_micro        =0;
  rev_id1->revid_cl72         =0;
  rev_id1->revid_pir          =0;
  rev_id1->revid_llp          =0;
  rev_id1->revid_eee          =0;

  __err=ERR_CODE_NONE;

  rev_id0->revid_rev_letter =rdc_revid_rev_letter(); if(__err) return(__err);
  rev_id0->revid_rev_number =rdc_revid_rev_number(); if(__err) return(__err);
  rev_id0->revid_bonding    =rdc_revid_bonding();    if(__err) return(__err);
  rev_id0->revid_process    =rdc_revid_process();    if(__err) return(__err);
  rev_id0->revid_model      =rdc_revid_model();      if(__err) return(__err);

  rev_id1->revid_multiplicity =rdc_revid_multiplicity(); if(__err) return(__err);
  rev_id1->revid_mdio         =rdc_revid_mdio();         if(__err) return(__err);
  rev_id1->revid_micro        =rdc_revid_micro();        if(__err) return(__err);
  rev_id1->revid_cl72         =rdc_revid_cl72();         if(__err) return(__err);
  rev_id1->revid_pir          =rdc_revid_pir();          if(__err) return(__err);
  rev_id1->revid_llp          =rdc_revid_llp();          if(__err) return(__err);
  rev_id1->revid_eee          =rdc_revid_eee();          if(__err) return(__err);

  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_pmd_ln_h_rstb_pkill_override(  plp_aperta_phymod_phy_access_t *sa__, uint16_t val)
{
    err_code_t __err;
    /*
    * Work around per Magesh/Justin
    * override input from PCS to allow uc_dsc_ready_for_cmd
    * reg get written by UC
    */
    __err = ERR_CODE_NONE;
    __err = wr_pmd_ln_h_rstb_pkill(val);
    if(__err) return(__err);
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_lane_soft_reset( plp_aperta_phymod_phy_access_t *sa__, uint32_t enable)   /* release the pmd core soft reset */
{
    int i, start_lane, num_lane;
    uint32_t reset_enable;
    plp_aperta_phymod_phy_access_t phy_access_copy;
    RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_t reg;

    PHYMOD_MEMCPY(&phy_access_copy, sa__, sizeof(phy_access_copy));
    RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_CLR(reg);

    if (enable) {
        reset_enable = 0;
    } else {
        reset_enable = 1;
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&(sa__->access), &start_lane, &num_lane));
    for (i = 0; i < num_lane; i++) {
        phy_access_copy.access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(sa__->access.lane_mask, start_lane + i)) {
            continue;
        }
        RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr_LN_DP_S_RSTBf_SET(reg, reset_enable);
        PHYMOD_IF_ERR_RETURN(MODIFY_RXTXCOM_LN_CLK_RST_N_PWRDWN_CTLr(&phy_access_copy, reg));
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_lane_soft_reset_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *enable)   /* release the pmd core soft reset */
{
    err_code_t __err;
    uint32_t data;

    __err = ERR_CODE_NONE;
    data = rd_ln_dp_s_rstb();
    if(__err) return(__err);
    if (data) {
        *enable = 0;
    } else {
        *enable = 1;
    }
    return ERR_CODE_NONE;
}


err_code_t plp_aperta_blackhawk_lane_hard_soft_reset_release( plp_aperta_phymod_phy_access_t *sa__, uint32_t enable)   /* release the pmd core soft reset */
{
    err_code_t __err;
    __err = ERR_CODE_NONE;
    __err = wr_ln_s_rstb(enable);
    if(__err) return(__err);

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_clause72_control( plp_aperta_phymod_phy_access_t *sa__, uint32_t cl_72_en)                /* CLAUSE_72_CONTROL */
{
    err_code_t __err;

    __err = ERR_CODE_NONE;

    if (cl_72_en) {
        __err = wr_linktrn_ieee_training_enable(1);
        if(__err) return(__err);
    } else {
        __err = wr_linktrn_ieee_training_enable(0);
        if(__err) return(__err);
    }

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_clause72_control_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *cl_72_en)                /* CLAUSE_72_CONTROL */
{
    err_code_t __err;
    __err = ERR_CODE_NONE;
    *cl_72_en = rd_linktrn_ieee_training_enable();
    if(__err) return(__err);

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_channel_loss_set( plp_aperta_phymod_phy_access_t *sa__, uint32_t loss_in_db)
{
    err_code_t __err;
    __err = ERR_CODE_NONE;
     __err = wrv_blackhawk_tsc_usr_ctrl_pam4_chn_loss(sa__, loss_in_db);
    if(__err) return(__err);
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_channel_loss_get( plp_aperta_phymod_phy_access_t *sa__, uint32_t *loss_in_db)
{
    err_code_t __err;
    __err = ERR_CODE_NONE;
    *loss_in_db = rdv_blackhawk_tsc_usr_ctrl_pam4_chn_loss(sa__);
    if(__err) return(__err);
    return ERR_CODE_NONE;
}



err_code_t plp_aperta_blackhawk_electrical_idle_set( plp_aperta_phymod_phy_access_t *sa__, uint8_t en)
{
    AMS_TX_TX_CTL2r_t reg;

    AMS_TX_TX_CTL2r_CLR(reg);
    AMS_TX_TX_CTL2r_AMS_TX_ELEC_IDLE_AUXf_SET(reg, en);
    PHYMOD_IF_ERR_RETURN(MODIFY_AMS_TX_TX_CTL2r(sa__, reg));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_electrical_idle_get( plp_aperta_phymod_phy_access_t *sa__, uint8_t *en)
{
    AMS_TX_TX_CTL2r_t reg;

    PHYMOD_IF_ERR_RETURN(
       READ_AMS_TX_TX_CTL2r(sa__, &reg));
    *en = AMS_TX_TX_CTL2r_AMS_TX_ELEC_IDLE_AUXf_GET(reg);

    return ERR_CODE_NONE;
}


/***********************************************/
/*  Microcode Init into Program RAM Functions  */
/***********************************************/

/* uCode Load through Register (MDIO) Interface [Return Val = Error_Code (0 = PASS)] */
err_code_t plp_aperta_blackhawk_tsc_ucode_init(  plp_aperta_phymod_phy_access_t *sa__ )
{
    err_code_t __err;
    __err = ERR_CODE_NONE;

    __err = wrc_micro_master_clk_en(0x1); if(__err) return(__err); /* Enable clock to microcontroller subsystem */
    __err = wrc_micro_master_rstb(0x1); if(__err) return(__err); /* De-assert reset to microcontroller sybsystem */
    __err = wrc_micro_master_rstb(0x0); if(__err) return(__err); /* Assert reset to microcontroller sybsystem - Toggling reset*/
    __err = wrc_micro_master_rstb(0x1); if(__err) return(__err); /* De-assert reset to microcontroller sybsystem */
    __err = wrc_micro_cr_access_en(1); if(__err) return(__err); /* allow access to Code RAM */

    __err = wrc_micro_ra_init(0x1); if(__err) return(__err); /* Set initialization command to initialize code RAM */
    __err = plp_aperta_blackhawk_tsc_INTERNAL_poll_micro_ra_initdone(sa__, 250); /* Poll status of data RAM initialization */
    if(__err) return(__err);

    __err = wrc_micro_ra_init(0x2); if(__err) return(__err); /* Write command for data RAM initialization */
    __err = plp_aperta_blackhawk_tsc_INTERNAL_poll_micro_ra_initdone(sa__, 250); /* Poll status of data RAM initialization */
    if(__err) return(__err);

    __err = wrc_micro_ra_init(0x0); if(__err) return(__err); /* Clear initialization command */

    __err = wrc_micro_cr_crc_prtsel(0); if(__err) return(__err);
    __err = wrc_micro_cr_prif_prtsel(0); if(__err) return(__err);
    __err = wrc_micro_cr_crc_init(1); if(__err) return(__err);    /* initialize the HW CRC calculation */
    __err = wrc_micro_cr_crc_init(0); if(__err) return(__err);
    __err = wrc_micro_cr_ignore_micro_code_writes(0); if(__err) return(__err);  /* allow writing to program RAM */


    return (ERR_CODE_NONE);
}

/**
@brief   Init the PMD
@param   pmd_touched If the PMD is already initialized
@returns The value ERR_CODE_NONE upon successful completion
@details Per core PMD resets (both datapath and entire core)
We only intend to use this function if the PMD has never been initialized.
*/
err_code_t plp_aperta_blackhawk_pmd_reset_seq( plp_aperta_phymod_phy_access_t *sa__, int pmd_touched)
{
    err_code_t __err;
    __err = ERR_CODE_NONE;

   if (pmd_touched == 0) {
         __err = wrc_core_s_rstb(1); if(__err) return(__err);
  }
    return (ERR_CODE_NONE);
}


/**
@brief   Enable the pll reset bit
@param   enable Controls whether to reset PLL
@returns The value ERR_CODE_NONE upon successful completion
@details
Resets the PLL
*/
err_code_t plp_aperta_blackhawk_pll_reset_enable_set ( plp_aperta_phymod_phy_access_t *sa__, int enable)
{
    return (ERR_CODE_NONE);
}

/**
@brief   Read PLL range
*/
err_code_t plp_aperta_blackhawk_tsc_read_pll_range ( plp_aperta_phymod_phy_access_t *sa__, uint32_t *pll_range)
{
  return (ERR_CODE_NONE);
}


/**
@brief   Reag signal detect
*/
err_code_t plp_aperta_blackhawk_tsc_signal_detect( plp_aperta_phymod_phy_access_t *sa__, uint32_t *signal_detect)
{
    err_code_t __err;
    __err=ERR_CODE_NONE;
    *signal_detect = rd_signal_detect();
    if(__err) return(__err);
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_ladder_setting_to_mV( plp_aperta_phymod_phy_access_t *sa__, int8_t y, int16_t* level)
{

  return(ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_get_vco ( plp_aperta_phymod_phy_inf_config_t* config, uint32_t *vco_rate, uint32_t *new_pll_div, int16_t *new_os_mode)
{
  return(ERR_CODE_NONE);
}

/* Get Enable/Disable Shared TX pattern generator */
err_code_t plp_aperta_blackhawk_tsc_tx_shared_patt_gen_en_get(  plp_aperta_phymod_phy_access_t *sa__, uint8_t *enable) {
    err_code_t __err;
    __err=ERR_CODE_NONE;
    *enable = rd_patt_gen_en();
    if(__err) return(__err);
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_config_shared_tx_pattern_idx_set(  plp_aperta_phymod_phy_access_t *sa__,  uint32_t *sa__ttern_len)
{
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_config_shared_tx_pattern_idx_get(  plp_aperta_phymod_phy_access_t *sa__, uint32_t *pattern_len, uint32_t *pattern) {
  err_code_t __err;
  uint8_t mode_sel;
  uint32_t lsb = 0, msb = 0;
  uint16_t temp_lsb, temp_msb;
  int pattern_idx;

  __err=ERR_CODE_NONE;

  mode_sel = rd_patt_gen_start_pos(); if(__err) return(__err);

  mode_sel = 12 - mode_sel;

  if(mode_sel == 6) {
    *pattern_len = 140;
  } else if (mode_sel == 5) {
    *pattern_len = 160;
  } else if (mode_sel == 4) {
    *pattern_len = 180;
  } else if (mode_sel == 3) {
    *pattern_len = 200;
  } else if (mode_sel == 2) {
    *pattern_len = 220;
  } else if (mode_sel == 1) {
    *pattern_len = 240;
  } else {
    *pattern_len = 0;
  }

  for (pattern_idx=0;pattern_idx<PATTERN_MAX_SIZE;pattern_idx++) {
      switch (pattern_idx) {
        case 0:
          msb=rdc_patt_gen_seq_14(); if(__err) return(__err);
          lsb=rdc_patt_gen_seq_13(); if(__err) return(__err);
          break;
        case 1:
          msb=rdc_patt_gen_seq_12(); if(__err) return(__err);
          lsb=rdc_patt_gen_seq_11(); if(__err) return(__err);
          break;
        case 2:
          msb=rdc_patt_gen_seq_10(); if(__err) return(__err);
          lsb=rdc_patt_gen_seq_9(); if(__err) return(__err);
          break;
        case 3:
          msb=rdc_patt_gen_seq_8(); if(__err) return(__err);
          lsb=rdc_patt_gen_seq_7(); if(__err) return(__err);
          break;
        case 4:
          msb=rdc_patt_gen_seq_6(); if(__err) return(__err);
          lsb=rdc_patt_gen_seq_5(); if(__err) return(__err);
          break;
        case 5:
          msb=rdc_patt_gen_seq_4(); if(__err) return(__err);
          lsb=rdc_patt_gen_seq_3(); if(__err) return(__err);
          break;
        case 6:
          msb=rdc_patt_gen_seq_2(); if(__err) return(__err);
          lsb=rdc_patt_gen_seq_1(); if(__err) return(__err);
          break;
        case 7:
          msb=rdc_patt_gen_seq_0(); if(__err) return(__err);
          lsb=0;
          break;
        /* Its a dead default and cause coverity defect
        default:
              PHYMOD_DEBUG_ERROR(("Wrong index value : Should be between 0 and 7\n"));
          return ERR_CODE_DIAG;
          break;*/
      }
      plp_aperta_phymod_swap_bit(lsb, &temp_lsb);
      plp_aperta_phymod_swap_bit(msb, &temp_msb);

      pattern[pattern_idx] = temp_lsb << 16 | temp_msb  ;
  }

  return ERR_CODE_NONE;
}

/**********************************/
/*  Serdes TX disable/RX Restart  */
/**********************************/

err_code_t plp_aperta_blackhawk_tsc_tx_disable_get ( plp_aperta_phymod_phy_access_t *sa__, uint8_t *enable)
{
    err_code_t __err;

    __err=ERR_CODE_NONE;
    *enable = (uint8_t) rd_sdk_tx_disable();
    if(__err) return(__err);
     return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_refclk_set( plp_aperta_phymod_phy_access_t *sa__, plp_aperta_phymod_ref_clk_t ref_clock)
{
    DIG_TOP_USER_CTL0r_t dig_top_user_ctrl_reg;
    DIG_TOP_USER_CTL0r_CLR(dig_top_user_ctrl_reg);

    switch (ref_clock) {
        case phymodRefClk156Mhz:
            DIG_TOP_USER_CTL0r_HEARTBEAT_COUNT_1USf_SET(dig_top_user_ctrl_reg, 0x271);
            break;
        case phymodRefClk125Mhz:
            DIG_TOP_USER_CTL0r_HEARTBEAT_COUNT_1USf_SET(dig_top_user_ctrl_reg, 0x1f4);
            break;
        case phymodRefClk312Mhz:
            DIG_TOP_USER_CTL0r_HEARTBEAT_COUNT_1USf_SET(dig_top_user_ctrl_reg, 0x271);
            break;
        default:
            DIG_TOP_USER_CTL0r_HEARTBEAT_COUNT_1USf_SET(dig_top_user_ctrl_reg, 0x271);
            break;
    }
    PHYMOD_IF_ERR_RETURN(MODIFY_DIG_TOP_USER_CTL0r(sa__, dig_top_user_ctrl_reg));

     return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_tx_nrz_mode_get(plp_aperta_phymod_phy_access_t *sa__, uint16_t *tx_nrz_mode)
{
    TXFIR_TAP_CTL0r_t reg;

    PHYMOD_IF_ERR_RETURN(
       READ_TXFIR_TAP_CTL0r(sa__, &reg));
    *tx_nrz_mode = TXFIR_TAP_CTL0r_TXFIR_NRZ_TAP_RANGE_SELf_GET(reg);
     return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_signalling_mode_status_get( plp_aperta_phymod_phy_access_t *sa__, phymod_phy_signalling_method_t *mode)
{
    RXTXCOM_OSR_MODE_STS_MC_MASKr_t reg;
    uint16_t temp_data = 0;

    PHYMOD_IF_ERR_RETURN(
       READ_RXTXCOM_OSR_MODE_STS_MC_MASKr(sa__, &reg));
    temp_data = RXTXCOM_OSR_MODE_STS_MC_MASKr_PAM4_MODEf_GET(reg);
    if (temp_data) {
        *mode = phymodSignallingMethodPAM4;
    } else {
        *mode = phymodSignallingMethodNRZ;
    }
    return ERR_CODE_NONE;
}


err_code_t plp_aperta_blackhawk_tsc_tx_tap_mode_get( plp_aperta_phymod_phy_access_t *sa__, uint8_t *mode)
{
    err_code_t __err;

    __err=ERR_CODE_NONE;
    *mode = (uint8_t) rd_txfir_tap_en();
    if(__err) return(__err);
     return ERR_CODE_NONE;
}

/*
err_code_t blackhawk_tsc_pam4_tx_pattern_enable_get( plp_aperta_phymod_phy_access_t *sa__, phymod_PAM4_tx_pattern_t pattern_type, uint32_t* enable)
{
    err_code_t __err;

    __err=ERR_CODE_NONE;
    *enable = (uint8_t) rd_patt_gen_en();
    if(__err) return(__err);
    if (*enable) {
        switch (pattern_type) {
        case phymod_PAM4TxPattern_JP03B:
            *enable = rd_pam4_tx_jp03b_patt_en();
             if(__err) return(__err);
             break;
        case phymod_PAM4TxPattern_Linear:
            *enable = rd_pam4_tx_linearity_patt_en();
             if(__err) return(__err);
             break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported PAM4 tx pattern  %u"), pattern_type));
        }
    }
     return ERR_CODE_NONE;
}
*/
err_code_t plp_aperta_blackhawk_tsc_tx_pam4_precoder_enable_set(plp_aperta_phymod_phy_access_t *sa__, int enable)
{
    err_code_t __err;

    __err = ERR_CODE_NONE;

    __err = wr_pam4_precoder_en(enable);
    if(__err) return(__err);
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_tx_pam4_precoder_enable_get(plp_aperta_phymod_phy_access_t *sa__, int *enable)
{
    err_code_t __err;
    __err = ERR_CODE_NONE;

    *enable = rd_pam4_precoder_en();
    if(__err) return(__err);
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_micro_clk_source_select( plp_aperta_phymod_phy_access_t *sa__, uint32_t pll_index)
{
    err_code_t __err;
    uint16_t rdval;

    rdval=rdcv_misc_ctrl_byte();
    /* first zero out bit2 which is the micro clk source bit */
    rdval &= ~0x4;
    rdval |= pll_index << 2;
    wrcv_misc_ctrl_byte(rdval);

     return ERR_CODE_NONE;
}

/* Get the PLL powerdown status */
err_code_t plp_aperta_blackhawk_tsc_pll_pwrdn_get(plp_aperta_phymod_phy_access_t *sa__, uint32_t *is_pwrdn)
{
    err_code_t __err;

    __err = ERR_CODE_NONE;
    /* Either afs_s_pll_pwrdn or ams_pll_pwrdn is set, PLL is in power down state */
    *is_pwrdn = rdc_afe_s_pll_pwrdn();
    *is_pwrdn |= rdc_ams_pll_pwrdn();
    if (__err) return (__err);

    return ERR_CODE_NONE;
}
/* Get the PLL lock status */
err_code_t plp_aperta_blackhawk_tsc_pll_lock_get(plp_aperta_phymod_phy_access_t *sa__, uint32_t *pll_lock)
{
    err_code_t __err;

    __err = ERR_CODE_NONE;
    /* read PLL lock status */
    *pll_lock = rdc_pll_lock();
    *pll_lock = rdc_pll_lock();
    if (__err) return (__err);

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_speed_config_get(uint32_t speed, int ref_clk_is_156p25, uint32_t *pll_multiplier, uint32_t *is_pam4, uint32_t *osr_mode)
{
     if (ref_clk_is_156p25) {
         switch (speed) {
         case 10312:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_132;
            *is_pam4 = 0;
            *osr_mode = 1;
            break;
         case 11500:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_147P2;
            *is_pam4 = 0;
            *osr_mode = 1;
            break;
         case 12500:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_160;
            *is_pam4 = 0;
            *osr_mode = 1;
            break;
         case 20625:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_132;
            *is_pam4 = 0;
            *osr_mode = 0;
            break;
         case 23000:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_147P2;
            *is_pam4 = 0;
            *osr_mode = 0;
            break;
         case 25000:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_160;
            *is_pam4 = 0;
            *osr_mode = 0;
            break;
         case 25780:
         case 25781:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_165;
            *osr_mode = 0;
            *is_pam4 = 0;
            break;
         case 27343:
             *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_175;
             *osr_mode = 0;
             *is_pam4 = 0;
             break;
         case 28125:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_180;
            *is_pam4 = 0;
            *osr_mode = 0;
            break;
         case 41250:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_132;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 41875:
             return PHYMOD_E_CONFIG;
             break;
         case 45000:
             *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_144;
             *is_pam4 = 1;
             *osr_mode = 0;
             break;
         case 46000:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_147P2;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 50000:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_160;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 51560:
         case 51562:
         case 51561:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_165;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 53125:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_170;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 56250:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_180;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         default:
            return PHYMOD_E_CONFIG;
            break;
         }
     } else {
         switch (speed) {
         case 10312:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_66;
            *is_pam4 = 0;
            *osr_mode = 1;
            break;
         case 11500:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_73P6;
            *is_pam4 = 0;
            *osr_mode = 1;
            break;
         case 12500:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_80;
            *is_pam4 = 0;
            *osr_mode = 1;
            break;
         case 20625:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_66;
            *is_pam4 = 0;
            *osr_mode = 0;
            break;
         case 23000:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_73P6;
            *is_pam4 = 0;
            *osr_mode = 0;
            break;
         case 25000:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_80;
            *is_pam4 = 0;
            *osr_mode = 0;
            break;
         case 25780:
         case 25781:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_82P5;
            *osr_mode = 0;
            *is_pam4 = 0;
            break;
         case 27343:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_87P5;
            *osr_mode = 0;
            *is_pam4 = 0;
            break;
         case 28125:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_90;
            *is_pam4 = 0;
            *osr_mode = 0;
            break;
         case 41250:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_66;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 41875:
             *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_67;
             *is_pam4 = 1;
             *osr_mode = 0;
             break;
         case 45000:
             *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_72;
             *is_pam4 = 1;
             *osr_mode = 0;
             break;
         case 46000:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_73P6;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 50000:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_80;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 51560:
         case 51562:
         case 51561:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_82P5;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 53125:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_85;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         case 56250:
            *pll_multiplier = BLACKHAWK_TSC_PLL_DIV_90;
            *is_pam4 = 1;
            *osr_mode = 0;
            break;
         default:
            return PHYMOD_E_CONFIG;
            break;
        }
    }

    return PHYMOD_E_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_lane_cfg_fwapi_data1_set( plp_aperta_phymod_phy_access_t *sa__, uint32_t val)
{
    err_code_t __err;

    __err = ERR_CODE_NONE;
    __err = wr_lane_cfg_fwapi_data1(val);
    if(__err) return(__err);

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_enable_pass_through_configuration(plp_aperta_phymod_phy_access_t *sa__, int enable)
{
    err_code_t __err;
    uint16_t write_val;
    __err = ERR_CODE_NONE;

    write_val = enable? 0x9 : 0x0;
    __err = wrv_blackhawk_tsc_usr_ctrl_unused_word(sa__, write_val);
    if(__err) return(__err);

    return ERR_CODE_NONE;
}

/* Locks TX_PI to Loop timing, external CDR */
err_code_t plp_aperta_blackhawk_tsc_ext_loop_timing(plp_aperta_phymod_phy_access_t *sa__, uint8_t enable)
{
    uint8_t is_enable = enable ? 0x1 : 0x0;

    EFUN(wr_tx_pi_repeater_mode_en(is_enable));
    EFUN(wr_tx_pi_jitter_filter_en(is_enable));    /* Jitter filter enable to lock freq */
    EFUN(wr_tx_pi_en(is_enable));
    EFUN(wr_tx_pi_loop_timing_src_sel(is_enable)); /* RX phase_sum_val_logic enable */

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_error_analyzer_status_clear(plp_aperta_phymod_phy_access_t *sa__)
{
    err_code_t __err;

    __err = ERR_CODE_NONE;
    __err = wr_tlb_err_clear_error_analyzer_status(1);
    if(__err) return(__err);

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_rx_ppm(plp_aperta_phymod_phy_access_t *sa__, int16_t *rx_ppm)
{
    err_code_t __err;
    __err=ERR_CODE_NONE;
    *rx_ppm = rd_cdr_integ_reg() / 84;
    if(__err) return(__err);
    return (ERR_CODE_NONE);
}

/* Set/Get clk4sync_en, clk4sync_div */
err_code_t plp_aperta_blackhawk_tsc_clk4sync_enable_set(plp_aperta_phymod_phy_access_t *sa__, uint32_t en, uint32_t div)
{
    err_code_t __err;

    __err = ERR_CODE_NONE;

    __err = wrc_ams_pll_clk4sync_en(en);
    if (__err) return (__err);
    __err = wrc_ams_pll_clk4sync_div(div);
    if (__err) return (__err);

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_clk4sync_enable_get(plp_aperta_phymod_phy_access_t *sa__, uint32_t *en, uint32_t *div)
{
    err_code_t __err;

    __err = ERR_CODE_NONE;

    *en = rdc_ams_pll_clk4sync_en();
    if (__err) return (__err);
    *div = rdc_ams_pll_clk4sync_div();
    if (__err) return (__err);

    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_ams_version_get(plp_aperta_phymod_phy_access_t *sa__, uint32_t *ams_version)
{
    err_code_t __err;

    __err = ERR_CODE_NONE;
    *ams_version = rd_ams_tx_version_id();
    if (__err) return (__err);

    return ERR_CODE_NONE;
}


