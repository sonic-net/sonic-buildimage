/*
*  $Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.$
*  $Id$
*/

/*
 *
 * $Id: peregrine5_pc_cfg_seq.c$
 *
 * This program is the proprietary software of Broadcom Corporation
 * and/or its licensors, and may only be used, duplicated, modified
 * or distributed pursuant to the terms and conditions of a separate,
 * written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized
 * License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and
 * Broadcom expressly reserves all rights in and to the Software
 * and all intellectual property rights therein.  IF YOU HAVE
 * NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
 * IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 * ALL USE OF THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1.     This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof,
 * and to use this information only in connection with your use of
 * Broadcom integrated circuit products.
 *
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
 * PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 * OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
 * INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
 * ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
 * TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
 * THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 *
 *
 */


#include <phymod/phymod.h>
#include <phymod/phymod_system.h>
#include <phymod/phymod_util.h>
#include <bcmi_peregrine5_pc_xgxs_defs.h>
#include "peregrine5_pc_cfg_seq.h"
#include "peregrine5_pc_fields.h"
#include "peregrine5_pc_field_access.h"
#include "peregrine5_pc_dependencies.h"
#include "peregrine5_pc_interface.h"
#include "peregrine5_pc_functions.h"
#include "public/peregrine5_api_uc_vars_rdwr_defns_public.h"

err_code_t plp_aperta2_peregrine5_pc_tx_rx_polarity_set(srds_access_t *sa__, uint32_t tx_pol, uint32_t rx_pol)
{
    PHYMOD_EFUN_WR(wr_tx_pmd_dp_invert(tx_pol));
    PHYMOD_EFUN_WR(wr_rx_pmd_dp_invert(rx_pol));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_tx_rx_polarity_get(srds_access_t *sa__, uint32_t *tx_pol, uint32_t *rx_pol)
{
    err_code_t srds_err_code = ERR_CODE_NONE;
    *tx_pol = (uint32_t) rd_tx_pmd_dp_invert();
    *rx_pol = (uint32_t) rd_rx_pmd_dp_invert();

    return (srds_err_code);
}

err_code_t plp_aperta2_peregrine5_pc_uc_active_set(srds_access_t *sa__, uint32_t enable)
{
    PHYMOD_EFUN_WR(wrc_uc_active(enable));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_uc_active_get(srds_access_t *sa__, uint32_t *enable)
{
    err_code_t srds_err_code = ERR_CODE_NONE;
    *enable = (uint32_t) rdc_uc_active();

    return (srds_err_code);
}

err_code_t plp_aperta2_peregrine5_pc_pmd_force_signal_detect(srds_access_t *sa__, uint8_t force_en, uint8_t force_val)
{
    PHYMOD_EFUN_WR(wr_signal_detect_frc(force_en));
    PHYMOD_EFUN_WR(wr_signal_detect_frc_val(force_val));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_pmd_force_signal_detect_get(srds_access_t *sa__, uint8_t *force_en, uint8_t *force_val)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *force_en = rd_signal_detect_frc();
    *force_val = rd_signal_detect_frc_val();

    return (srds_err_code);
}

err_code_t plp_aperta2_peregrine5_pc_dig_lpbk_get(srds_access_t *sa__, uint32_t *lpbk)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *lpbk = rd_dig_lpbk_en();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_rmt_lpbk_get(srds_access_t *sa__, uint32_t *lpbk)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *lpbk = rd_rmt_lpbk_en();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_pmd_cl72_receiver_status(srds_access_t *sa__, uint32_t *status)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *status = rd_linktrn_ieee_receiver_status();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_pram_firmware_enable(srds_access_t *sa__, int enable, int wait)   /* release the pmd core soft reset */
{
    uint8_t micro_orig, num_micros, micro_idx;
    err_code_t srds_err_code = ERR_CODE_NONE;

    if (enable == 1) {
        PHYMOD_EFUN_WR(wrc_micro_pramif_ahb_wraddr_msw(0));
        PHYMOD_EFUN_WR(wrc_micro_pramif_ahb_wraddr_lsw(0));
        PHYMOD_EFUN_WR(wrc_micro_pram_if_rstb(1));
        PHYMOD_EFUN_WR(wrc_micro_pramif_en(1));

        PHYMOD_EFUN_WR(wrc_micro_cr_crc_prtsel(0));
        PHYMOD_EFUN_WR(wrc_micro_cr_crc_init(1));                       /* Initialize the HW CRC calculation */
        PHYMOD_EFUN_WR(wrc_micro_cr_crc_init(0));
        PHYMOD_EFUN_WR(wrc_micro_cr_crc_calc_en(1));

        if (wait) {
          PHYMOD_USLEEP(500);
        }
    } else {
        /* block writing to program RAM */
        PHYMOD_EFUN_WR(wrc_micro_ra_wrdatasize(0x2));                   /* Select 32bit transfers as default */
        PHYMOD_EFUN_WR(wrc_micro_cr_ignore_micro_code_writes(1));
        PHYMOD_EFUN_WR(wrc_micro_pramif_en(0));
        PHYMOD_EFUN_WR(wrc_micro_cr_crc_calc_en(0));

        micro_orig = plp_aperta2_peregrine5_pc_get_micro_idx(sa__);
        num_micros = rdc_micro_num_uc_cores();
        for (micro_idx = 0; micro_idx < num_micros; micro_idx++) {
            srds_err_code += plp_aperta2_peregrine5_pc_set_micro_idx(sa__, micro_idx);
            PHYMOD_EFUN_WR(wrc_micro_core_clk_en(1));
        }
        srds_err_code += plp_aperta2_peregrine5_pc_set_micro_idx(sa__, micro_orig);
    }

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_pmd_lane_map_get(srds_access_t *sa__, uint32_t *tx_lane_map, uint32_t *rx_lane_map) {

    err_code_t srds_err_code = ERR_CODE_NONE;
    uint32_t tmp_tx_lane = 0;
    uint32_t tmp_rx_lane = 0;
    *tx_lane_map = 0;
    *rx_lane_map = 0;

    tmp_tx_lane = rdc_tx_lane_addr_0();
    *tx_lane_map |= tmp_tx_lane ;
    tmp_tx_lane = rdc_tx_lane_addr_1();
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 1);
    tmp_tx_lane = rdc_tx_lane_addr_2();
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 2);
    tmp_tx_lane = rdc_tx_lane_addr_3();
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 3);
    tmp_tx_lane = rdc_tx_lane_addr_4();
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 4);
    tmp_tx_lane = rdc_tx_lane_addr_5();
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 5);
    tmp_tx_lane = rdc_tx_lane_addr_6();
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 6);
    tmp_tx_lane = rdc_tx_lane_addr_7();
    *tx_lane_map |= (tmp_tx_lane & 0xf) << (4 * 7);

    tmp_rx_lane = rdc_rx_lane_addr_0();
    *rx_lane_map |= (tmp_rx_lane & 0xf);
    tmp_rx_lane = rdc_rx_lane_addr_1();
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 1);
    tmp_rx_lane = rdc_rx_lane_addr_2();
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 2);
    tmp_rx_lane = rdc_rx_lane_addr_3();
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 3);
    tmp_rx_lane = rdc_rx_lane_addr_4();
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 4);
    tmp_rx_lane = rdc_rx_lane_addr_5();
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 5);
    tmp_rx_lane = rdc_rx_lane_addr_6();
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 6);
    tmp_rx_lane = rdc_rx_lane_addr_7();
    *rx_lane_map |= (tmp_rx_lane & 0xf) << (4 * 7);

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_tx_pi_control_get(srds_access_t *sa__, int16_t* value)
{
    uint8_t override_enable;
    err_code_t srds_err_code = ERR_CODE_NONE;

    override_enable = rd_tx_pi_freq_override_en();
    if (override_enable) {
        *value = rd_tx_pi_freq_override_val();
    } else {
        *value = 0;
    }

    return srds_err_code;
}

err_code_t
plp_aperta2_peregrine5_pc_identify(srds_access_t *sa__, peregrine5_pc_rev_id0_t *rev_id0, peregrine5_pc_rev_id1_t *rev_id1)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

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

    rev_id0->revid_rev_letter =rdc_revid_rev_letter(); if(srds_err_code) return(srds_err_code);
    rev_id0->revid_rev_number =rdc_revid_rev_number(); if(srds_err_code) return(srds_err_code);
    rev_id0->revid_bonding    =rdc_revid_bonding();    if(srds_err_code) return(srds_err_code);
    rev_id0->revid_process    =rdc_revid_process();    if(srds_err_code) return(srds_err_code);
    rev_id0->revid_model      =rdc_revid_model();      if(srds_err_code) return(srds_err_code);

    rev_id1->revid_multiplicity =rdc_revid_multiplicity(); if(srds_err_code) return(srds_err_code);
    rev_id1->revid_mdio         =rdc_revid_mdio();         if(srds_err_code) return(srds_err_code);
    rev_id1->revid_micro        =rdc_revid_micro();        if(srds_err_code) return(srds_err_code);
    rev_id1->revid_cl72         =rdc_revid_linktrn();      if(srds_err_code) return(srds_err_code);
    rev_id1->revid_pir          =rdc_revid_pir();          if(srds_err_code) return(srds_err_code);
    rev_id1->revid_llp          =rdc_revid_llp();          if(srds_err_code) return(srds_err_code);
    rev_id1->revid_eee          =rdc_revid_eee();          if(srds_err_code) return(srds_err_code);

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_pmd_ln_h_rstb_pkill_override(srds_access_t *sa__, uint16_t val)
{
    /*
    * Work around per Magesh/Justin
    * override input from PCS to allow uc_dsc_ready_for_cmd
    * reg get written by UC
    */
    PHYMOD_EFUN_WR(wr_pmd_ln_h_rstb_pkill(val));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_pmd_ln_h_pwrdn_pkill_override(srds_access_t *sa__, uint16_t val)
{
    /*
    * Work around Peregrine aging issue
    */
    PHYMOD_EFUN_WR(wr_pmd_ln_rx_h_pwrdn_pkill(val));
    PHYMOD_EFUN_WR(wr_pmd_ln_tx_h_pwrdn_pkill(val));

    return ERR_CODE_NONE;
}


err_code_t plp_aperta2_peregrine5_pc_lane_soft_reset(srds_access_t *sa__, uint32_t enable)   /* Assert the pmd lane soft reset */
{
    int i, start_lane, num_lane;
    uint32_t reset_enable;
    srds_access_t phy_access_copy;
    err_code_t srds_err_code = ERR_CODE_NONE;

    PHYMOD_MEMCPY(&phy_access_copy, sa__, sizeof(phy_access_copy));

    if (enable) {
        reset_enable = 0;
    } else {
        reset_enable = 1;
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&sa__->access, &start_lane, &num_lane));
    for (i = 0; i < num_lane; i++) {
        sa__->access.lane_mask = 1 << (start_lane + i);
        if (!PHYMOD_LANEPBMP_MEMBER(sa__->access.lane_mask, start_lane + i)) {
            continue;
        }
        PHYMOD_EFUN_WR(wr_ln_dp_s_rstb(reset_enable));
    }

    /* next restore back the sa__ */
    PHYMOD_MEMCPY(sa__, &phy_access_copy, sizeof(phy_access_copy));

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_lane_soft_reset_get(srds_access_t *sa__, uint32_t *enable)
{
    uint32_t data;
    err_code_t srds_err_code = ERR_CODE_NONE;

    data = rd_ln_dp_s_rstb();
    if (data) {
        *enable = 0;
    } else {
        *enable = 1;
    }

    return srds_err_code;
}


err_code_t plp_aperta2_peregrine5_pc_lane_hard_soft_reset_release(srds_access_t *sa__, uint32_t enable)   /* release the pmd lane soft reset */
{
    PHYMOD_EFUN_WR(wr_ln_s_rstb(enable));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_clause72_control(srds_access_t *sa__, uint32_t cl_72_en)                /* CLAUSE_72_CONTROL */
{
    if (cl_72_en) {
        PHYMOD_EFUN_WR(wr_linktrn_ieee_training_enable(1));
    } else {
        PHYMOD_EFUN_WR(wr_linktrn_ieee_training_enable(0));
    }

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_clause72_control_get(srds_access_t *sa__, uint32_t *cl_72_en)                /* CLAUSE_72_CONTROL */
{
    err_code_t srds_err_code = ERR_CODE_NONE;
    *cl_72_en = rd_linktrn_ieee_training_enable();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_electrical_idle_set(srds_access_t *sa__, uint8_t en)
{
    AMS_TX_TX_CTL2r_t reg;

    AMS_TX_TX_CTL2r_CLR(reg);
    AMS_TX_TX_CTL2r_AMS_TX_ELEC_IDLE_AUXf_SET(reg, en);
    PHYMOD_IF_ERR_RETURN(MODIFY_AMS_TX_TX_CTL2r(sa__, reg));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_electrical_idle_get(srds_access_t *sa__, uint8_t *en)
{
    AMS_TX_TX_CTL2r_t reg;

    PHYMOD_IF_ERR_RETURN(READ_AMS_TX_TX_CTL2r(sa__, &reg));
    *en = AMS_TX_TX_CTL2r_AMS_TX_ELEC_IDLE_AUXf_GET(reg);

    return ERR_CODE_NONE;
}

/***********************************************/
/*  Microcode Init into Program RAM Functions  */
/***********************************************/

/* uCode Load through Register (MDIO) Interface [Return Val = Error_Code (0 = PASS)] */
err_code_t plp_aperta2_peregrine5_pc_ucode_init(srds_access_t *sa__ )
{
    PHYMOD_EFUN_WR(wrc_micro_master_clk_en(0x1)); /* Enable clock to microcontroller subsystem */
    PHYMOD_EFUN_WR(wrc_micro_master_rstb(0x1));  /* De-assert reset to microcontroller sybsystem */
    PHYMOD_EFUN_WR(wrc_micro_master_rstb(0x0)); /* Assert reset to microcontroller sybsystem - Toggling reset*/
    PHYMOD_EFUN_WR(wrc_micro_master_rstb(0x1)); /* De-assert reset to microcontroller sybsystem */
    PHYMOD_EFUN_WR(wrc_micro_cr_access_en(1)); /* allow access to Code RAM */

    PHYMOD_EFUN_WR(wrc_micro_ra_init(0x1)); /* Set initialization command to initialize code RAM */
    PHYMOD_EFUN_WR(plp_aperta2_peregrine5_pc_INTERNAL_poll_micro_ra_initdone(sa__, 250)); /* Poll status of data RAM initialization */
    PHYMOD_EFUN_WR(wrc_micro_ra_init(0x2));  /* Write command for data RAM initialization */
    PHYMOD_EFUN_WR(plp_aperta2_peregrine5_pc_INTERNAL_poll_micro_ra_initdone(sa__, 250)); /* Poll status of data RAM initialization */

    PHYMOD_EFUN_WR(wrc_micro_ra_init(0x0)); /* Clear initialization command */

    PHYMOD_EFUN_WR(wrc_micro_cr_crc_prtsel(0));
    PHYMOD_EFUN_WR(wrc_micro_cr_prif_prtsel(0));
    PHYMOD_EFUN_WR(wrc_micro_cr_crc_init(1)); /* initialize the HW CRC calculation */
    PHYMOD_EFUN_WR(wrc_micro_cr_crc_init(0));
    PHYMOD_EFUN_WR(wrc_micro_cr_ignore_micro_code_writes(0));  /* allow writing to program RAM */

    return (ERR_CODE_NONE);
}

/*
@brief   Init the PMD
@param   pmd_touched If the PMD is already initialized
@returns The value ERR_CODE_NONE upon successful completion
@details Per core PMD resets (both datapath and entire core)
We only intend to use this function if the PMD has never been initialized.
*/
err_code_t plp_aperta2_peregrine5_pc_pmd_reset_seq(srds_access_t *sa__, int pmd_touched)
{
    err_code_t __err;
    __err = ERR_CODE_NONE;

   if (pmd_touched == 0) {
         __err = wrc_core_s_rstb(1); if(__err) return(__err);
  }
    return (ERR_CODE_NONE);
}

/**
@brief   Reag signal detect
*/
err_code_t plp_aperta2_peregrine5_pc_signal_detect(srds_access_t *sa__, uint32_t *signal_detect)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *signal_detect = rd_signal_detect();

    return srds_err_code;
}

/* Get Enable/Disable Shared TX pattern generator */
err_code_t plp_aperta2_peregrine5_pc_tx_shared_patt_gen_en_get(srds_access_t *sa__, uint8_t *enable) {
    err_code_t srds_err_code = ERR_CODE_NONE;

    *enable = rd_patt_gen_en();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_config_shared_tx_pattern_idx_get(srds_access_t *sa__, uint32_t *pattern_len, uint32_t *pattern) {
    uint8_t mode_sel;
    uint32_t lsb = 0, msb = 0;
    uint16_t temp_lsb, temp_msb;
    int pattern_idx;
    err_code_t srds_err_code = ERR_CODE_NONE;

    mode_sel = rd_patt_gen_start_pos();
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

    for (pattern_idx = 0; pattern_idx < PATTERN_MAX_SIZE; pattern_idx++) {
        switch (pattern_idx) {
        case 0:
          msb=rd_tx_patt_gen_seq_14();
          lsb=rd_tx_patt_gen_seq_13();
          break;
        case 1:
          msb=rd_tx_patt_gen_seq_12();
          lsb=rd_tx_patt_gen_seq_11();
          break;
        case 2:
          msb=rd_tx_patt_gen_seq_10();
          lsb=rd_tx_patt_gen_seq_9();
          break;
        case 3:
          msb=rd_tx_patt_gen_seq_8();
          lsb=rd_tx_patt_gen_seq_7();
          break;
        case 4:
          msb=rd_tx_patt_gen_seq_6();
          lsb=rd_tx_patt_gen_seq_5();
          break;
        case 5:
          msb=rd_tx_patt_gen_seq_4();
          lsb=rd_tx_patt_gen_seq_3();
          break;
        case 6:
          msb=rd_tx_patt_gen_seq_2();
          lsb=rd_tx_patt_gen_seq_1();
          break;
        case 7:
          msb=rd_tx_patt_gen_seq_0();
          lsb=0;
          break;
        /* Its a dead default and cause coverity defect
        default:
              PHYMOD_DEBUG_ERROR(("Wrong index value : Should be between 0 and 7\n"));
          return ERR_CODE_DIAG;
          break;*/
      }
      plp_aperta2_phymod_swap_bit(lsb, &temp_lsb);
      plp_aperta2_phymod_swap_bit(msb, &temp_msb);

      pattern[pattern_idx] = temp_lsb << 16 | temp_msb  ;
    }

    return srds_err_code;
}

/**********************************/
/*  Serdes TX disable/RX Restart  */
/**********************************/

err_code_t plp_aperta2_peregrine5_pc_tx_disable_get(srds_access_t *sa__, uint8_t *enable)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *enable = (uint8_t) rd_sdk_tx_disable();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_comclk_set(srds_access_t *sa__, plp_aperta2_phymod_ref_clk_t ref_clock)
{
    switch (ref_clock) {
        case phymodRefClk156Mhz:
            PHYMOD_EFUN_WR(wrc_heartbeat_count_1us(0x271));
            break;
        case phymodRefClk125Mhz:
            PHYMOD_EFUN_WR(wrc_heartbeat_count_1us(0x1f4));
            break;
        case phymodRefClk312Mhz:
            PHYMOD_EFUN_WR(wrc_heartbeat_count_1us(0x4e2));
            break;
        default:
            PHYMOD_EFUN_WR(wrc_heartbeat_count_1us(0x4e2));
            break;
    }

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_tx_nrz_mode_get(srds_access_t *sa__, uint16_t *tx_nrz_mode)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *tx_nrz_mode =  rd_txfir_nrz_tap_range_sel();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_signalling_mode_status_get(srds_access_t *sa__, phymod_phy_signalling_method_t *mode)
{
    uint16_t temp_data = 0;
    err_code_t srds_err_code = ERR_CODE_NONE;

    temp_data = rd_pam4_mode();

    if (temp_data) {
        *mode = phymodSignallingMethodPAM4;
    } else {
        *mode = phymodSignallingMethodNRZ;
    }

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_tx_tap_mode_get(srds_access_t *sa__, uint8_t *mode)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    /* if pre3_tap enable is 0, mean 3tap mode, otherwise 6tap mode */
    *mode = (uint8_t) rd_txfir_pre3_tap_en();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_pam4_tx_pattern_enable_get(srds_access_t *sa__, plp_aperta2_phymod_pattern_t pattern_type, uint32_t* enable)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *enable = (uint8_t) rd_patt_gen_en();
#if 0
    if (*enable) {
        switch (pattern_type) {
        case phymod_PAM4TxPattern_JP03B:
            *enable = rd_pam4_tx_jp03b_patt_en();
             break;
        case phymod_PAM4TxPattern_Linear:
            *enable = rd_pam4_tx_linearity_patt_en();
             break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("unsupported PAM4 tx pattern  %d"), pattern_type));
        }
    }
#endif
    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_tx_pam4_precoder_enable_set(srds_access_t *sa__, int enable)
{
    PHYMOD_EFUN_WR(wr_pam4_precoder_en(enable));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_tx_pam4_precoder_enable_get(srds_access_t *sa__, int *enable)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *enable = rd_pam4_precoder_en();

    return srds_err_code;
}

/* Get the PLL powerdown status */
err_code_t plp_aperta2_peregrine5_pc_pll_pwrdn_get(srds_access_t *sa__, uint32_t *is_pwrdn)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    /* Either afs_s_pll_pwrdn or ams_pll_pwrdn is set, PLL is in power down state */
    *is_pwrdn = rdc_afe_s_pll_pwrdn();
    *is_pwrdn |= rdc_ams_pll_pwrdn();

    return srds_err_code;
}

/* Get the PLL lock status */
err_code_t plp_aperta2_peregrine5_pc_pll_lock_get(srds_access_t *sa__, uint32_t *pll_lock)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    /* read PLL lock status */
    *pll_lock = rdc_pll_lock();
    *pll_lock = rdc_pll_lock();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_speed_config_get(uint32_t speed,
                                          uint32_t *pll_multiplier,
                                          uint32_t *is_pam4,
                                          uint32_t *osr_mode)
{
    switch (speed) {
    case 53125:
       *pll_multiplier = PEREGRINE5_PC_PLL_DIV_170;
       *is_pam4 = 1;
       *osr_mode = 1;
       break;
    case 106250:
       *pll_multiplier = PEREGRINE5_PC_PLL_DIV_170;
       *is_pam4 = 1;
       *osr_mode = 0;
       break;
    default:
        return PHYMOD_E_CONFIG;
        break;
    }

     return PHYMOD_E_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_rx_ppm(srds_access_t *sa__, int16_t *rx_ppm)
{
    err_code_t srds_err_code = ERR_CODE_NONE;
    *rx_ppm = rd_cdr_integ_reg() / 84;

    return srds_err_code;
}

/* Set/Get clk4sync_en, clk4sync_div */
err_code_t plp_aperta2_peregrine5_pc_clk4sync_enable_set(srds_access_t *sa__, uint32_t en, uint32_t srds_div)
{
    PHYMOD_EFUN_WR(wrc_ams_pll_en_clk4sync(en));
    PHYMOD_EFUN_WR(wrc_ams_pll_clk4sync_div8_6b(srds_div));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_clk4sync_enable_get(srds_access_t *sa__, uint32_t *en, uint32_t *srds_div)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *en = rdc_ams_pll_en_clk4sync();
    *srds_div = rdc_ams_pll_clk4sync_div8_6b();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_lane_pll_selection_set( srds_access_t *sa__, uint32_t pll_index)
{
    /* PHYMOD_EFUN_WR(wr_pll_select(pll_index)); */

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_lane_pll_selection_get( srds_access_t *sa__, uint32_t *pll_index)
{
/*    
    err_code_t srds_err_code = ERR_CODE_NONE;
    *pll_index = rd_pll_select();
*/
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_rx_protect_nrzmux_set(srds_access_t *sa__, uint8_t mux_val)
{
    /* PHYMOD_EFUN_WR(wr_ams_rx_protect_nrzmux_enb(mux_val)); */

    return ERR_CODE_NONE;
}

err_code_t
plp_aperta2_peregrine5_pc_rx_pmd_lock_status_get(srds_access_t *sa__, uint32_t *pmd_lock, uint32_t *pmd_lock_change)
{
    int start_lane, num_lane, lane_offset;
    srds_access_t phy_access_copy;
    TLB_RX_PMD_RX_LOCK_STSr_t pmd_lock_status_reg;

    PHYMOD_MEMCPY(&phy_access_copy, sa__, sizeof(phy_access_copy));
    *pmd_lock_change = 0;
    *pmd_lock        = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&sa__->access, &start_lane, &num_lane));

    for (lane_offset = 0; lane_offset < num_lane; lane_offset++) {
        if (!PHYMOD_LANEPBMP_MEMBER(sa__->access.lane_mask, start_lane + lane_offset)) {
            continue;
        }
        phy_access_copy.access.lane_mask = 1 << (start_lane + lane_offset);
        PHYMOD_IF_ERR_RETURN(
            READ_TLB_RX_PMD_RX_LOCK_STSr(&phy_access_copy, &pmd_lock_status_reg));
        *pmd_lock_change |=  TLB_RX_PMD_RX_LOCK_STSr_PMD_RX_LOCK_CHANGEf_GET(pmd_lock_status_reg)?
                                 phy_access_copy.access.lane_mask:0;
        *pmd_lock        |=  TLB_RX_PMD_RX_LOCK_STSr_PMD_RX_LOCKf_GET(pmd_lock_status_reg)?
                                 phy_access_copy.access.lane_mask:0;
    }
    *pmd_lock_change    >>= start_lane;
    *pmd_lock           >>= start_lane;
    return PHYMOD_E_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_lane_dp_reset_state_get(srds_access_t *sa__, uint32_t *reset_state)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *reset_state = rd_lane_dp_reset_state();

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_osr_mode_set(srds_access_t *sa__, int osr_mode)
{
    PHYMOD_EFUN_WR(wr_osr_mode_frc_val(osr_mode));
    PHYMOD_EFUN_WR(wr_osr_mode_frc(1));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_osr_mode_get(srds_access_t *sa__, int *osr_mode)
{
    err_code_t srds_err_code = ERR_CODE_NONE;
    int osr_forced;

    *osr_mode = 0;
    osr_forced = rd_osr_mode_frc();

    if(osr_forced) {
        *osr_mode = rd_osr_mode_frc_val();
    }

    return srds_err_code;
}

err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_compute_proj_without_print(srds_access_t *sa__, 
        peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config,
        peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status) {
#ifdef SERDES_API_FLOATING_POINT
    INIT_SRDS_ERR_CODE
    peregrine5_pc_prbs_err_analyzer_report_st err_analyzer_rept;

    USR_MEMSET(&err_analyzer_rept, 0, sizeof(peregrine5_pc_prbs_err_analyzer_report_st));

    if (err_analyzer_config == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (err_analyzer_status == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_report_proj(sa__, err_analyzer_config, err_analyzer_status, &err_analyzer_rept));
/*
    EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_report_display(sa__, err_analyzer_config, &err_analyzer_rept));
*/

#else
    UNUSED(err_analyzer_status);
    UNUSED(err_analyzer_config);
#endif /* SERDES_API_FLOATING_POINT */

    return (ERR_CODE_NONE);
}
err_code_t plp_aperta2_peregrine5_pc_tx_clock_div34_enable_set(srds_access_t *sa__, int enable)
{
    PHYMOD_EFUN_WR(wrc_ams_pll_pd_clkd33_d34(0));
    PHYMOD_EFUN_WR(wrc_ams_pll_en_clkd34(enable));

    return ERR_CODE_NONE;
}
err_code_t plp_aperta2_peregrine5_pc_pam4_lp_has_precoder_enable_get(srds_access_t *sa__, int *enable)
{
    err_code_t srds_err_code = ERR_CODE_NONE;

    *enable = rd_rx_pam4er_prec_disabled() ? 0 : 1;

    return srds_err_code;
}
