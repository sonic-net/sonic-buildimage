/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/***********************************************************************************
 ***********************************************************************************
 *  File Name     :  peregrine5_pc_config.c                                           *
 *  Created On    :  03 Nov 2015                                                   *
 *  Created By    :  Brent Roberts                                                 *
 *  Description   :  API config functions Serdes IPs                               *
 *  Revision      :   *
 *                                                                                 *
 ***********************************************************************************
 ***********************************************************************************/


#include <phymod/phymod_system.h>
#include "peregrine5_pc_config.h"
#include "peregrine5_pc_access.h"
#include "peregrine5_pc_common.h"
#include "peregrine5_pc_functions.h"
#include "peregrine5_pc_internal.h"
#include "peregrine5_pc_internal_error.h"
#include "peregrine5_pc_select_defns.h"
#include "peregrine5_pc_enum.h"
#include "peregrine5_pc_dependencies.h"
#include "peregrine5_pc_version.h"
#include "peregrine5_pc_diag.h"

/** @file
 *
 */




/* User specific implementation for creating syncronization object, used for critical section access in initialization routine below */
/*USR_CREATE_LOCK; */

#ifndef SMALL_FOOTPRINT
/*******************************/
/*  Stop/Resume RX Adaptation  */
/*******************************/

err_code_t plp_aperta2_peregrine5_pc_stop_rx_adaptation(srds_access_t *sa__, uint8_t enable) {
  if (enable) {
      err_code_t err_code;
      err_code = plp_aperta2_peregrine5_pc_pmd_uc_control(sa__, CMD_UC_CTRL_STOP_GRACEFULLY,GRACEFUL_STOP_TIME);
      if(err_code) {
          uint8_t temp=0;
          USR_PRINTF(("Warning Graceful stop request returned error %d; Requesting a forceful stop\n",err_code));
          temp = plp_aperta2_peregrine5_pc_INTERNAL_stop_micro(sa__,0,&err_code);
          /* return value from plp_aperta2_peregrine5_pc_INTERNAL_stop_micro - immediate stop is not required */
          /* void casting to avoid compiler warning. */
          UNUSED(temp);
      }
      return(err_code);
  }
  else {
    return(plp_aperta2_peregrine5_pc_pmd_uc_control(sa__, CMD_UC_CTRL_RESUME,50));
  }
}

err_code_t plp_aperta2_peregrine5_pc_request_stop_rx_adaptation(srds_access_t *sa__) {

  return(plp_aperta2_peregrine5_pc_pmd_uc_control_return_immediate(sa__, CMD_UC_CTRL_STOP_GRACEFULLY));
}

/**********************/
/*  uCode CRC Verify  */
/**********************/

err_code_t plp_aperta2_peregrine5_pc_ucode_crc_verify(srds_access_t *sa__, uint32_t ucode_len, uint16_t expected_crc_value) {
    INIT_SRDS_ERR_CODE
    uint16_t calc_crc;

    UNUSED(ucode_len);
    ESTM(calc_crc = rdc_micro_cr_crc_checksum());

    if(calc_crc != expected_crc_value) {
        EFUN_PRINTF(("Microcode CRC did not match expected=%04x : calculated=%04x\n",expected_crc_value, calc_crc));
        return(peregrine5_pc_error(sa__, ERR_CODE_UC_CRC_NOT_MATCH));
    }

    return(ERR_CODE_NONE);
}

#endif /* SMALL_FOOTPRINT */

/**************************************************************/
/*  APIs to Write Lane Config and User variables into uC RAM  */
/**************************************************************/

err_code_t plp_aperta2_peregrine5_pc_set_uc_lane_cfg(srds_access_t *sa__, struct peregrine5_pc_uc_lane_config_st struct_val) {
    INIT_SRDS_ERR_CODE
  uint8_t reset_state;
  ESTM(reset_state = rd_rx_lane_dp_reset_state());
  if(reset_state < 7) {
      EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_set_uc_lane_cfg(..) called without ln_dp_s_rstb=0\n"));
      return (peregrine5_pc_error(sa__, ERR_CODE_LANE_DP_NOT_RESET));
  }
  EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_uc_lane_config_word(&struct_val));
  return(reg_wr_RX_CKRST_CTRL_LANE_UC_CONFIG(struct_val.word));
}

err_code_t plp_aperta2_peregrine5_pc_get_force_lane_cdr_mode(srds_access_t *sa__, enum peregrine5_pc_force_cdr_mode_enum *mode) {
    INIT_SRDS_ERR_CODE
    uint8_t os_mode = 0;
    uint8_t br_mode = 0;
    uint8_t cdr_mode = 0;

    if(mode == NULL) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }
    ESTM(cdr_mode = rd_lane_uc_config_force_cdr_mode());
    os_mode = (cdr_mode == 0x1);
    br_mode = (cdr_mode == 0x2);

    if(os_mode) {
        *mode = PEREGRINE5_PC_OSCDR_FORCE_ENABLE;
    }
    else if(br_mode) {
        *mode = PEREGRINE5_PC_BRCDR_FORCE_ENABLE;
    }
    else {
        *mode = PEREGRINE5_PC_CDR_FORCE_DISABLE;
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_force_lane_cdr_mode(srds_access_t *sa__, enum peregrine5_pc_force_cdr_mode_enum mode) {
    INIT_SRDS_ERR_CODE
    switch(mode) {
        case PEREGRINE5_PC_OSCDR_FORCE_ENABLE:
            EFUN(wr_lane_uc_config_force_cdr_mode(0x1));
            break;
        case PEREGRINE5_PC_OSCDR_FORCE_DISABLE:
            EFUN(wr_lane_uc_config_force_cdr_mode(0x0));
            break;
        case PEREGRINE5_PC_BRCDR_FORCE_ENABLE:
            EFUN(wr_lane_uc_config_force_cdr_mode(0x2));
            break;
        case PEREGRINE5_PC_BRCDR_FORCE_DISABLE:
            EFUN(wr_lane_uc_config_force_cdr_mode(0x0));
            break;
        case PEREGRINE5_PC_CDR_FORCE_DISABLE:
            EFUN(wr_lane_uc_config_force_cdr_mode(0x0));
            break;
        default:
            return peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    return ERR_CODE_NONE;
}

/******************************************************************/
/*  APIs to Read Core/Lane Config and User variables from uC RAM  */
/******************************************************************/

err_code_t plp_aperta2_peregrine5_pc_get_uc_core_config(srds_access_t *sa__, struct peregrine5_pc_uc_core_config_st *get_val) {
    INIT_SRDS_ERR_CODE
    uint8_t pll_orig;
    ESTM(pll_orig = plp_aperta2_peregrine5_pc_acc_get_pll_idx(sa__));
    EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, 0));
    /* TODO: convert to fail-safe form where default return is error case */
    if(!get_val) {
        return peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
        ESTM(get_val->word = reg_rdc_CORE_PLL_COM_PLL_UC_CORE_CONFIG());
    EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, pll_orig));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_uc_core_config_st(get_val));
    return ERR_CODE_NONE;
}
err_code_t plp_aperta2_peregrine5_pc_get_physical_tx_addr(srds_access_t *sa__, uint8_t lane, uint8_t *physical_tx_lane) {
    INIT_SRDS_ERR_CODE
    uint8_t logical_tx_lane = 0, phy_lane = 0;
    const uint8_t num_lanes = 8;
    for(phy_lane = 0; phy_lane < num_lanes; phy_lane++) {
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_logical_tx_lane_addr(sa__, phy_lane, &logical_tx_lane));
        if(logical_tx_lane == lane) {
            *physical_tx_lane = phy_lane;
            return (ERR_CODE_NONE);
        }
    }
    *physical_tx_lane = 255;
    return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
}

err_code_t plp_aperta2_peregrine5_pc_get_physical_rx_addr(srds_access_t *sa__, uint8_t lane, uint8_t *physical_rx_lane) {
    INIT_SRDS_ERR_CODE
    uint8_t logical_rx_lane = 0, phy_lane = 0;
    const uint8_t num_lanes = 8;
    for(phy_lane = 0; phy_lane < num_lanes; phy_lane++) {
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_logical_rx_lane_addr(sa__, phy_lane, &logical_rx_lane));
        if(logical_rx_lane == lane) {
            *physical_rx_lane = phy_lane;
            return (ERR_CODE_NONE);
        }
    }
    *physical_rx_lane = 255;
    return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
}
#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_get_uc_lane_cfg(srds_access_t *sa__, struct peregrine5_pc_uc_lane_config_st *get_val) {
    INIT_SRDS_ERR_CODE

  if(!get_val) {
     return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
  ESTM(get_val->word = reg_rd_RX_CKRST_CTRL_LANE_UC_CONFIG());
  EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_uc_lane_config_st(get_val));
  return (ERR_CODE_NONE);
}
#endif /* SMALL_FOOTPRINT */

/*---------------------------------*/
/*  APIs to Reset Lane to Default  */
/*---------------------------------*/

err_code_t plp_aperta2_peregrine5_pc_reset_tx_lane_to_default(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    EFUN(wr_tx_ln_dp_s_rstb(0x0));
    EFUN(USR_DELAY_US(1));
    EFUN(wr_tx_ln_s_rstb(0x0));
    EFUN(USR_DELAY_US(1));
    EFUN(wr_tx_ln_s_rstb(0x1));
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_reset_rx_lane_to_default(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    EFUN(wr_rx_ln_dp_s_rstb(0x0));
    EFUN(USR_DELAY_US(1));
    EFUN(wr_rx_ln_s_rstb(0x0));
    EFUN(USR_DELAY_US(1));
    EFUN(wr_rx_ln_s_rstb(0x1));
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_reset_lane_to_default(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    EFUN(wr_ln_dp_s_rstb(0x0));
    EFUN(USR_DELAY_US(1));
    EFUN(wr_ln_s_rstb(0x0));
    EFUN(USR_DELAY_US(1));
    EFUN(wr_ln_s_rstb(0x1));
    return (ERR_CODE_NONE);
}

/*--------------------------------------------*/
/*  APIs to Enable or Disable datapath reset  */
/*--------------------------------------------*/


err_code_t plp_aperta2_peregrine5_pc_tx_dp_reset(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    if (enable) {
        EFUN(wr_tx_ln_dp_s_rstb(0x0));
    } else {
        EFUN(wr_tx_ln_dp_s_rstb(0x1));
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_rx_dp_reset(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    if (enable) {
        EFUN(wr_rx_ln_dp_s_rstb(0x0));
    } else {
        EFUN(wr_rx_ln_dp_s_rstb(0x1));
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_core_dp_reset(srds_access_t *sa__, uint8_t enable){
    INIT_SRDS_ERR_CODE
    if (enable) {
        EFUN(wrc_core_dp_s_rstb(0x0));
    } else {
        EFUN(wrc_core_dp_s_rstb(0x1));
    }
    return ERR_CODE_NONE;
}
err_code_t plp_aperta2_peregrine5_pc_ln_dp_reset(srds_access_t *sa__, uint8_t enable){
    INIT_SRDS_ERR_CODE
    if (enable) {
        EFUN(wr_ln_dp_s_rstb(0x0));
    } else {
        EFUN(wr_ln_dp_s_rstb(0x1));
    }
    return ERR_CODE_NONE;
}

/*-----------------------------------------*/
/*  APIs for setting and getting OSR mode  */
/*-----------------------------------------*/

err_code_t plp_aperta2_peregrine5_pc_set_use_osr_mode_pins_only(srds_access_t *sa__, uint8_t use_pins_only) {
    INIT_SRDS_ERR_CODE

    EFUN(plp_aperta2_peregrine5_pc_set_use_tx_osr_mode_pins_only(sa__, use_pins_only));
    EFUN(plp_aperta2_peregrine5_pc_set_use_rx_osr_mode_pins_only(sa__, use_pins_only));

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_get_use_osr_mode_pins_only(srds_access_t *sa__, uint8_t *tx_use_pins_only, uint8_t *rx_use_pins_only) {
    INIT_SRDS_ERR_CODE

    if(tx_use_pins_only == NULL || rx_use_pins_only == NULL) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

    EFUN(plp_aperta2_peregrine5_pc_get_use_tx_osr_mode_pins_only(sa__, tx_use_pins_only));
    EFUN(plp_aperta2_peregrine5_pc_get_use_rx_osr_mode_pins_only(sa__, rx_use_pins_only));

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_set_use_rx_osr_mode_pins_only(srds_access_t *sa__, uint8_t rx_use_pins_only) {
    INIT_SRDS_ERR_CODE
    uint8_t reset_state;
    ESTM(reset_state = rd_rx_lane_dp_reset_state());
    if(reset_state < 7) {
        EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_set_use_rx_osr_mode_pins_only(..) called without ln_dp_s_rstb=0\n"));
        return (peregrine5_pc_error(sa__, ERR_CODE_LANE_DP_NOT_RESET));
    }
    EFUN(wrv_usr_rx_osr_ctrl_byte((uint8_t)((rx_use_pins_only << 7) | PEREGRINE5_PC_OSR_UNINITIALIZED)));

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_get_use_rx_osr_mode_pins_only(srds_access_t *sa__, uint8_t *rx_use_pins_only) {
    INIT_SRDS_ERR_CODE

    if(rx_use_pins_only == NULL) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }
    ESTM(*rx_use_pins_only = rdv_usr_rx_osr_ctrl_byte() >> 7);

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_set_use_tx_osr_mode_pins_only(srds_access_t *sa__, uint8_t tx_use_pins_only) {
    INIT_SRDS_ERR_CODE
    uint8_t reset_state;
    ESTM(reset_state = rd_tx_lane_dp_reset_state());
    if(reset_state < 7) {
        EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_set_use_tx_osr_mode_pins_only(..) called without ln_dp_s_rstb=0\n"));
        return (peregrine5_pc_error(sa__, ERR_CODE_LANE_DP_NOT_RESET));
    }
    EFUN(wrv_usr_tx_osr_ctrl_byte((uint8_t)((tx_use_pins_only << 7) | PEREGRINE5_PC_OSR_UNINITIALIZED)));

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_get_use_tx_osr_mode_pins_only(srds_access_t *sa__, uint8_t *tx_use_pins_only) {
    INIT_SRDS_ERR_CODE

    if(tx_use_pins_only == NULL) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }
    ESTM(*tx_use_pins_only = rdv_usr_tx_osr_ctrl_byte() >> 7);

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_set_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum osr_mode) {
    INIT_SRDS_ERR_CODE

    EFUN(plp_aperta2_peregrine5_pc_set_tx_osr_mode(sa__, osr_mode));
    EFUN(plp_aperta2_peregrine5_pc_set_rx_osr_mode(sa__, osr_mode));

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_get_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum *tx_osr_mode,  enum peregrine5_pc_osr_mode_enum *rx_osr_mode) {
    INIT_SRDS_ERR_CODE

    if(tx_osr_mode == NULL || rx_osr_mode == NULL) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

    EFUN(plp_aperta2_peregrine5_pc_get_tx_osr_mode(sa__, tx_osr_mode));
    EFUN(plp_aperta2_peregrine5_pc_get_rx_osr_mode(sa__, rx_osr_mode));

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_set_rx_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum rx_osr_mode) {
    INIT_SRDS_ERR_CODE
    uint8_t reset_state;
    ESTM(reset_state = rd_rx_lane_dp_reset_state());
    if(reset_state < 7) {
        EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_set_rx_osr_mode(..) called without ln_dp_s_rstb=0\n"));
        return (peregrine5_pc_error(sa__, ERR_CODE_LANE_DP_NOT_RESET));
    }
    EFUN(wrv_usr_rx_osr_ctrl_byte((uint8_t)rx_osr_mode));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_get_rx_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum *rx_osr_mode) {
    INIT_SRDS_ERR_CODE
    uint8_t rx_osr_ctrl;

    if(rx_osr_mode == NULL) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

    ESTM(rx_osr_ctrl = rdv_usr_rx_osr_ctrl_byte());
    if(!(rx_osr_ctrl & (1 << 7))) {
        *rx_osr_mode = (enum peregrine5_pc_osr_mode_enum)(rx_osr_ctrl & 0x3F);
    }
    else {
        ESTM(*rx_osr_mode = (enum peregrine5_pc_osr_mode_enum)rd_rx_osr_mode_pin());
    }

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_set_tx_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum tx_osr_mode) {
    INIT_SRDS_ERR_CODE
    uint8_t reset_state;
    ESTM(reset_state = rd_tx_lane_dp_reset_state());
    if(reset_state < 7) {
        EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_set_tx_osr_mode(..) called without ln_dp_s_rstb=0\n"));
        return (peregrine5_pc_error(sa__, ERR_CODE_LANE_DP_NOT_RESET));
    }
    EFUN(wrv_usr_tx_osr_ctrl_byte((uint8_t)tx_osr_mode));

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_get_tx_osr_mode(srds_access_t *sa__, enum peregrine5_pc_osr_mode_enum *tx_osr_mode) {
    INIT_SRDS_ERR_CODE
    uint8_t tx_osr_ctrl;

    if(tx_osr_mode == NULL) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

    ESTM(tx_osr_ctrl = rdv_usr_tx_osr_ctrl_byte());
    if(!(tx_osr_ctrl & (1 << 7))) {
        *tx_osr_mode = (enum peregrine5_pc_osr_mode_enum)(tx_osr_ctrl & 0x3F);
    }
    else {
        ESTM(*tx_osr_mode = (enum peregrine5_pc_osr_mode_enum)rd_tx_osr_mode_pin());
    }

    return(ERR_CODE_NONE);
}

/********************************/
/*  Loading ucode through PRAM  */
/********************************/
#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_ucode_pram_load(srds_access_t *sa__, char const * ucode_image, uint32_t ucode_len) {
    INIT_SRDS_ERR_CODE
    uint16_t ucode_len_remainder = (uint16_t)(((ucode_len + 3) & 0xFFFFFFFC) - ucode_len);

    if(!ucode_image) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    EFUN(plp_aperta2_peregrine5_pc_ucode_load_init(sa__, 1));

    /* Wait 8 pram clocks */
    EFUN(USR_DELAY_US(1));

    /* write ucode into pram */
    while (ucode_len > 0) {
        EFUN(plp_aperta2_peregrine5_pc_acc_wr_pram(sa__, (uint8_t)*ucode_image));
        --ucode_len;
        ++ucode_image;
    }

    /* Pad to 32 bits */
    while (ucode_len_remainder > 0) {
        EFUN(plp_aperta2_peregrine5_pc_acc_wr_pram(sa__, 0));
        --ucode_len_remainder;
    }

    /* Wait 8 pram clocks */
    EFUN(USR_DELAY_US(1));

    EFUN(plp_aperta2_peregrine5_pc_ucode_load_close(sa__, 1));
    return(ERR_CODE_NONE);
}

/************************************/
/*  Accessing core_config_from_pcs  */
/************************************/

err_code_t plp_aperta2_peregrine5_pc_set_core_config_from_pcs(srds_access_t *sa__, uint8_t core_cfg_from_pcs) {
    INIT_SRDS_ERR_CODE
    struct peregrine5_pc_uc_core_config_st core_config = UC_CORE_CONFIG_INIT;
    EFUN(plp_aperta2_peregrine5_pc_get_uc_core_config(sa__, &core_config));
    core_config.field.core_cfg_from_pcs = core_cfg_from_pcs;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_uc_core_config(sa__, core_config));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_set_osr_5_en(srds_access_t *sa__, uint8_t osr_5_en) {
    INIT_SRDS_ERR_CODE
    struct peregrine5_pc_uc_core_config_st core_config = UC_CORE_CONFIG_INIT;
    EFUN(plp_aperta2_peregrine5_pc_get_uc_core_config(sa__, &core_config));
    core_config.field.osr_5_en = osr_5_en;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_uc_core_config(sa__, core_config));
    return (ERR_CODE_NONE);
}
/******************/
/*  Lane Mapping  */
/******************/

static err_code_t _plp_aperta2_peregrine5_pc_map_lanes(srds_access_t *sa__, const uint8_t num_lanes, uint8_t const *tx_lane_map, uint8_t const *rx_lane_map);
static err_code_t _plp_aperta2_peregrine5_pc_map_lanes(srds_access_t *sa__, const uint8_t num_lanes, uint8_t const *tx_lane_map, uint8_t const *rx_lane_map) {
    INIT_SRDS_ERR_CODE
    uint8_t index1, index2;
    uint8_t lp_message_printed = 0;

    /* Verify that tx_lane_map and rx_lane_map are valid. */
    for (index1=0; index1<num_lanes; ++index1) {

        /* Verify that a lane map is not to an invalid lane. */
        if ((tx_lane_map[index1] >= num_lanes)
            || (rx_lane_map[index1] >= num_lanes)){
            return (ERR_CODE_BAD_LANE);
        }

        /* Warn if an RX lane mapping is not the same as TX. */
        if ((tx_lane_map[index1] != rx_lane_map[index1])
            && !lp_message_printed) {
            ESTM_PRINTF(("Warning:  In core %d, TX lane %d is mapped to %d, while RX lane %d is mapped to %d.\n          Digital and remote loopback will not operate as expected.\n          Further warnings are suppressed.\n", plp_aperta2_peregrine5_pc_acc_get_core(sa__), index1, tx_lane_map[index1], index1, rx_lane_map[index1]));
            lp_message_printed = 1;
        }

        /* Verify that a lane map is not used twice. */
        for (index2=(uint8_t)(index1+1); index2<num_lanes; ++index2) {
            if ((tx_lane_map[index1] == tx_lane_map[index2])
                || (rx_lane_map[index1] == rx_lane_map[index2])) {
                return (ERR_CODE_BAD_LANE);
            }
        }
    }

    /* Write the map bitfields.
     * Support up to 8 lanes.
     */
    EFUN(wrc_tx_lane_addr_0(*(tx_lane_map++))); EFUN(wrc_rx_lane_addr_0(*(rx_lane_map++)));
    EFUN(wrc_tx_lane_addr_1(*(tx_lane_map++))); EFUN(wrc_rx_lane_addr_1(*(rx_lane_map++)));
    EFUN(wrc_tx_lane_addr_2(*(tx_lane_map++))); EFUN(wrc_rx_lane_addr_2(*(rx_lane_map++)));
    EFUN(wrc_tx_lane_addr_3(*(tx_lane_map++))); EFUN(wrc_rx_lane_addr_3(*(rx_lane_map++)));
    EFUN(wrc_tx_lane_addr_4(*(tx_lane_map++))); EFUN(wrc_rx_lane_addr_4(*(rx_lane_map++)));
    EFUN(wrc_tx_lane_addr_5(*(tx_lane_map++))); EFUN(wrc_rx_lane_addr_5(*(rx_lane_map++)));
    EFUN(wrc_tx_lane_addr_6(*(tx_lane_map++))); EFUN(wrc_rx_lane_addr_6(*(rx_lane_map++)));
    EFUN(wrc_tx_lane_addr_7(*(tx_lane_map++))); EFUN(wrc_rx_lane_addr_7(*(rx_lane_map++)));
    return ERR_CODE_NONE;
}
err_code_t plp_aperta2_peregrine5_pc_map_lanes(srds_access_t *sa__, const uint8_t num_lanes, uint8_t const *tx_lane_map, uint8_t const *rx_lane_map) {
    INIT_SRDS_ERR_CODE
    uint8_t rd_val = 0;
    /* Verify that the core data path is held in reset. */
    ESTM(rd_val = rdc_core_dp_s_rstb());
    if (rd_val != 0) {
        EFUN_PRINTF(("ERROR: core data path reset is not asserted\n"));
        return (ERR_CODE_UC_NOT_RESET);
    }

    /* Verify that all micros are held in reset. */

    {
        uint8_t micro_orig, num_micros, micro_idx;
        ESTM(micro_orig = plp_aperta2_peregrine5_pc_acc_get_micro_idx(sa__));
        EFUN(plp_aperta2_peregrine5_pc_get_micro_num_uc_cores(sa__, &num_micros));
        for (micro_idx = 0; micro_idx < num_micros; micro_idx++) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_micro_idx(sa__, micro_idx));
            ESTM(rd_val |= rdc_micro_core_rstb());
        }
        EFUN(plp_aperta2_peregrine5_pc_acc_set_micro_idx(sa__, micro_orig));
    }
    if (rd_val != 0) {
        return (ERR_CODE_UC_NOT_RESET);
    }

    /* Verify that the num_lanes parameter is correct. */
    ESTM(rd_val = rdc_revid_multiplicity());
    if (rd_val != num_lanes) {
        return (ERR_CODE_BAD_LANE_COUNT);
    }

    EFUN(_plp_aperta2_peregrine5_pc_map_lanes(sa__, num_lanes, tx_lane_map, rx_lane_map));

    return (ERR_CODE_NONE);
            }

err_code_t plp_aperta2_peregrine5_pc_get_lane_map(srds_access_t *sa__, const uint8_t num_lanes, uint8_t *tx_lane_map, uint8_t *rx_lane_map) {
    INIT_SRDS_ERR_CODE
    uint8_t rd_val = 0;

    /* Verify that the num_lanes parameter is correct. */
    ESTM(rd_val = rdc_revid_multiplicity());
    if (rd_val != num_lanes) {
        return (ERR_CODE_BAD_LANE_COUNT);
            }
    ESTM(tx_lane_map[0] = rdc_tx_lane_addr_0()); ESTM(rx_lane_map[0] = rdc_rx_lane_addr_0());
    ESTM(tx_lane_map[1] = rdc_tx_lane_addr_1()); ESTM(rx_lane_map[1] = rdc_rx_lane_addr_1());
    ESTM(tx_lane_map[2] = rdc_tx_lane_addr_2()); ESTM(rx_lane_map[2] = rdc_rx_lane_addr_2());
    ESTM(tx_lane_map[3] = rdc_tx_lane_addr_3()); ESTM(rx_lane_map[3] = rdc_rx_lane_addr_3());
    ESTM(tx_lane_map[4] = rdc_tx_lane_addr_4()); ESTM(rx_lane_map[4] = rdc_rx_lane_addr_4());
    ESTM(tx_lane_map[5] = rdc_tx_lane_addr_5()); ESTM(rx_lane_map[5] = rdc_rx_lane_addr_5());
    ESTM(tx_lane_map[6] = rdc_tx_lane_addr_6()); ESTM(rx_lane_map[6] = rdc_rx_lane_addr_6());
    ESTM(tx_lane_map[7] = rdc_tx_lane_addr_7()); ESTM(rx_lane_map[7] = rdc_rx_lane_addr_7());
    return ERR_CODE_NONE;
                }
err_code_t plp_aperta2_peregrine5_pc_remap_lanes(srds_access_t *sa__, const uint8_t num_lanes, uint8_t const *tx_lane_map, uint8_t const *rx_lane_map) {
    INIT_SRDS_ERR_CODE
    uint8_t rd_val = 0;
    uint8_t lane = 0, lane_orig = 0;
    uint8_t lane_not_in_reset = 0;
    uint8_t actual_tx_lane_map[8] = {0};
    uint8_t actual_rx_lane_map[8] = {0};

    /* Verify that the num_lanes parameter is correct. */
    ESTM(rd_val = rdc_revid_multiplicity());
    if (rd_val != num_lanes) {
        return (ERR_CODE_BAD_LANE_COUNT);
            }
    /* Get the current lane mapping */
    EFUN(plp_aperta2_peregrine5_pc_get_lane_map(sa__, num_lanes, actual_tx_lane_map, actual_rx_lane_map));

    /* Verify that the TX and/or RX lane data paths are held in reset only for the lanes which need to be remapped */
    ESTM(lane_orig = plp_aperta2_peregrine5_pc_acc_get_lane(sa__));
    for(lane = 0; lane < num_lanes; lane++) {
        EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane));
        if((tx_lane_map[lane] != actual_tx_lane_map[lane])) {
            ESTM(rd_val = rd_tx_lane_dp_reset_state());
            if (rd_val != 7) {
                EFUN_PRINTF(("ERROR: lane %d has been selected for TX remapping, but TX lane data path reset is not asserted on the lane.\n", lane));
                lane_not_in_reset = 1;
        }
    }
        if((rx_lane_map[lane] != actual_rx_lane_map[lane])) {
            ESTM(rd_val = rd_rx_lane_dp_reset_state());
            if (rd_val != 7) {
                EFUN_PRINTF(("ERROR: lane %d has been selected for RX remapping, but RX lane data path reset is not asserted on the lane.\n", lane));
                lane_not_in_reset = 1;
            }
        }
    }
    EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane_orig));

    if(lane_not_in_reset) {
        return (ERR_CODE_LANE_DP_NOT_RESET);
    }

    EFUN(_plp_aperta2_peregrine5_pc_map_lanes(sa__, num_lanes, tx_lane_map, rx_lane_map));
    return (ERR_CODE_NONE);
}

/************************/
/*  Serdes API Version  */
/************************/

err_code_t plp_aperta2_peregrine5_pc_version(srds_access_t *sa__, uint32_t *api_version) {
    if(!api_version) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    *api_version = ((API_MAJOR_VERSION << 8) | API_MINOR_VERSION);
    return (ERR_CODE_NONE);
}
#endif /* SMALL_FOOTPRINT */

/*****************/
/*  PMD_RX_LOCK  */
/*****************/

err_code_t plp_aperta2_peregrine5_pc_pmd_lock_status(srds_access_t *sa__, uint8_t *pmd_rx_lock) {
    INIT_SRDS_ERR_CODE
    if(!pmd_rx_lock) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    ESTM(*pmd_rx_lock = rd_pmd_rx_lock());
    return (ERR_CODE_NONE);
}

/**********************************/
/*  Serdes TX disable/RX Restart  */
/**********************************/

err_code_t plp_aperta2_peregrine5_pc_tx_disable(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE

    if (enable) {
        EFUN(wr_sdk_tx_disable(0x1));
    }
    else {
        EFUN(wr_sdk_tx_disable(0x0));
    }
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_rx_restart(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE

    EFUN(wr_rx_restart_pmd_hold(enable));
    return(ERR_CODE_NONE);
}

#if !defined(SMALL_FOOTPRINT) 
/******************************************************/
/*  Single function to set/get all RX AFE parameters  */
/******************************************************/

err_code_t plp_aperta2_peregrine5_pc_write_rx_afe(srds_access_t *sa__, enum plp_aperta2_srds_rx_afe_settings_enum param, int8_t val) {
  /* Assumes the micro is not actively tuning */

    switch(param) {
    case RX_AFE_PF:
        return(plp_aperta2_peregrine5_pc_INTERNAL_set_rx_pf_main(sa__, (uint8_t)val));

    case RX_AFE_PF2:
        return(plp_aperta2_peregrine5_pc_INTERNAL_set_rx_pf2(sa__, (uint8_t)val));

    case RX_AFE_VGA:
      return(plp_aperta2_peregrine5_pc_INTERNAL_set_rx_pga(sa__, (uint8_t)val));

    case RX_AFE_PF3:
        return(plp_aperta2_peregrine5_pc_INTERNAL_set_rx_pf3(sa__, (uint8_t)val));

    default:
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
}
#endif /* !SMALL_FOOTPRINT */

#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_read_rx_afe(srds_access_t *sa__, enum plp_aperta2_srds_rx_afe_settings_enum param, int8_t *val) {
    INIT_SRDS_ERR_CODE
    /* Assumes the micro is not actively tuning */

    uint8_t rd_val;

    if(!val) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    switch(param) {
    case RX_AFE_PF:
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pf_main(sa__, &rd_val));
        *val = (int8_t)rd_val;
        break;
    case RX_AFE_PF2:
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pf2(sa__, &rd_val));
        *val = (int8_t)rd_val;
        break;
    case RX_AFE_VGA:
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pga(sa__, &rd_val));
        *val = (int8_t)rd_val;
      break;
    case RX_AFE_DFE1:
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_dfe1(sa__, val));
        break;
    case RX_AFE_DFE2:
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_dfe2(sa__, val));
      break;
    case RX_AFE_PF3:
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pf3(sa__, &rd_val));
        *val = (int8_t)rd_val;
        break;
    default:
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    UNUSED(rd_val);
    return(ERR_CODE_NONE);
}
#endif /* SMALL_FOOTPRINT */

static err_code_t _plp_aperta2_peregrine5_pc_get_txfir_st(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2, peregrine5_pc_txfir_st *txfir);
static err_code_t _plp_aperta2_peregrine5_pc_get_txfir_st(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2, peregrine5_pc_txfir_st *txfir) {
    INIT_SRDS_ERR_CODE

    ENULL_MEMSET(txfir, 0, sizeof(peregrine5_pc_txfir_st));
    if ((enable_taps == PEREGRINE5_PC_NRZ_6TAP) || (enable_taps == PEREGRINE5_PC_PAM4_6TAP)) {
        txfir->pre3 = pre3;
        txfir->pre2 = pre2;
        txfir->pre1 = pre1;
        txfir->main = main;
        txfir->post1 = post1;
        txfir->post2 = post2;
    }
    else {
        txfir->pre1 = pre1;
        txfir->main = main;
        txfir->post1 = post1;
    }
    return ERR_CODE_NONE;
}

static err_code_t _plp_aperta2_peregrine5_pc_validate_individual_taps(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2);
static err_code_t _plp_aperta2_peregrine5_pc_validate_individual_taps(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2) {
    if((enable_taps == PEREGRINE5_PC_PAM4_LP_3TAP) || (enable_taps == PEREGRINE5_PC_NRZ_LP_3TAP)) {
        if ((pre3 != 0) || (pre2 != 0) || (post2 != 0)) {
            EFUN_PRINTF(("ERROR: Selected 3 TAPs option, but other TAP inputs (pre3, pre2, post2) have non-zero value\n"));
            return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
        }
    }

    if (main < 0  || post1 > 0 || pre1 > 0) {
        EFUN_PRINTF(("ERROR: main TAP is expected to be positive and pre1 and post1 TAPs are expected to be negative for normal use cases."));
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (main < SRDS_ABS(pre1) || main < pre2 || main < SRDS_ABS(pre3) || main < SRDS_ABS(post1) || main < post2) {
        EFUN_PRINTF(("ERROR: main TAP is expected to be greater than all other TAPs."));
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if(pre3 > 0 || pre3 < -12) {
        EFUN_PRINTF(("ERROR: pre3 TAP is expected to be within [-12,0]."));
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_validate_txfir_cfg(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2) {
    INIT_SRDS_ERR_CODE
    peregrine5_pc_txfir_st txfir;

    EFUN(_plp_aperta2_peregrine5_pc_validate_individual_taps(sa__, enable_taps, pre3, pre2, pre1, main, post1, post2));
    EFUN(_plp_aperta2_peregrine5_pc_get_txfir_st(sa__, enable_taps, pre3, pre2, pre1, main, post1, post2, &txfir));

    return plp_aperta2_peregrine5_pc_INTERNAL_validate_full_txfir_cfg(sa__, enable_taps, &txfir);
}

#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_write_tx_afe(srds_access_t *sa__, enum peregrine5_pc_tx_afe_settings_enum param, int16_t val) {
    INIT_SRDS_ERR_CODE
    uint8_t  in_nrz_range;

    ESTM(in_nrz_range = rd_txfir_nrz_tap_range_sel());

    if (((!in_nrz_range) && ((val < TXFIR_PAM4_SW_TAP_MIN) || (val > TXFIR_PAM4_SW_TAP_MAX))) ||
        ((in_nrz_range)  && ((val <     TXFIR_NRZ_TAP_MIN) || (val >     TXFIR_NRZ_TAP_MAX)))) {
        return(peregrine5_pc_error(sa__, ERR_CODE_TXFIR_MAIN_INVALID));
    }
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_tx_tap(sa__, param, val));

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_load_txfir_taps(sa__));
    return(ERR_CODE_NONE);
}
#endif /* SMALL_FOOTPRINT */

err_code_t plp_aperta2_peregrine5_pc_read_tx_afe(srds_access_t *sa__, enum peregrine5_pc_tx_afe_settings_enum param, int16_t *val) {
    INIT_SRDS_ERR_CODE
    if(!val) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_tx_tap(sa__, param, val));

    return(ERR_CODE_NONE);
}

#if !defined(SMALL_FOOTPRINT) 
static err_code_t _plp_aperta2_peregrine5_pc_log_api_txfir_event(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps);
static err_code_t _plp_aperta2_peregrine5_pc_log_api_txfir_event(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps) {
    INIT_SRDS_ERR_CODE
    struct peregrine5_pc_event_st api_event;
    if ((enable_taps == PEREGRINE5_PC_NRZ_6TAP) || (enable_taps == PEREGRINE5_PC_PAM4_6TAP)) {
        api_event.data1 = 6;
    }
    else {
        api_event.data1 = 3;
    }
    api_event.event_id = 1;
    api_event.data2 = 0;
    {
        uint8_t reset_state = 0;
        uint8_t revid2;
        ESTM(revid2 = rdc_revid2());
        ESTM(reset_state = rd_lane_dp_reset_state());
        if((reset_state == 0x0) || (revid2 >= 0x2))
        {
            EFUN(plp_aperta2_peregrine5_pc_log_api_event(sa__, api_event));
        }
    }
    return ERR_CODE_NONE;
}
#endif /* !defined(SMALL_FOOTPRINT) && !defined(ATE_LOG) */

err_code_t plp_aperta2_peregrine5_pc_apply_txfir_cfg(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2) {
    INIT_SRDS_ERR_CODE
    peregrine5_pc_txfir_st txfir;

    EFUN(plp_aperta2_peregrine5_pc_validate_txfir_cfg(sa__, enable_taps, pre3, pre2, pre1, main, post1, post2));
    EFUN(_plp_aperta2_peregrine5_pc_get_txfir_st(sa__, enable_taps, pre3, pre2, pre1, main, post1, post2, &txfir));

#if !defined(SMALL_FOOTPRINT) 
    EFUN(_plp_aperta2_peregrine5_pc_log_api_txfir_event(sa__, enable_taps));
#endif

    return plp_aperta2_peregrine5_pc_INTERNAL_apply_full_txfir_cfg(sa__, enable_taps, &txfir);
}

static err_code_t _plp_aperta2_peregrine5_pc_get_txfir_nlc_st(srds_access_t *sa__, int8_t nlc_upper_eye_pct, int8_t nlc_lower_eye_pct, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2, peregrine5_pc_txfir_st *txfir_p3, peregrine5_pc_txfir_st *txfir_m3);
static err_code_t _plp_aperta2_peregrine5_pc_get_txfir_nlc_st(srds_access_t *sa__, int8_t nlc_upper_eye_pct, int8_t nlc_lower_eye_pct, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2, peregrine5_pc_txfir_st *txfir_p3, peregrine5_pc_txfir_st *txfir_m3) {
    INIT_SRDS_ERR_CODE

    ENULL_MEMSET(txfir_p3, 0, sizeof(peregrine5_pc_txfir_st));
    ENULL_MEMSET(txfir_m3, 0, sizeof(peregrine5_pc_txfir_st));

    txfir_p3->pre3  = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(2 * pre3  * nlc_upper_eye_pct, 100));
    txfir_p3->pre2  = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(2 * pre2  * nlc_upper_eye_pct, 100));
    txfir_p3->pre1  = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(2 * pre1  * nlc_upper_eye_pct, 100));
    txfir_p3->main  = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(2 * main  * nlc_upper_eye_pct, 100));
    txfir_p3->post1 = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(2 * post1 * nlc_upper_eye_pct, 100));
    txfir_p3->post2 = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(2 * post2 * nlc_upper_eye_pct, 100));

    txfir_m3->pre3  = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(-2 * pre3  * nlc_lower_eye_pct, 100));
    txfir_m3->pre2  = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(-2 * pre2  * nlc_lower_eye_pct, 100));
    txfir_m3->pre1  = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(-2 * pre1  * nlc_lower_eye_pct, 100));
    txfir_m3->main  = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(-2 * main  * nlc_lower_eye_pct, 100));
    txfir_m3->post1 = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(-2 * post1 * nlc_lower_eye_pct, 100));
    txfir_m3->post2 = (int16_t)(plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(-2 * post2 * nlc_lower_eye_pct, 100));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_validate_txfir_cfg_with_nlc(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int8_t nlc_upper_eye_pct, int8_t nlc_lower_eye_pct, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2) {
    INIT_SRDS_ERR_CODE
    peregrine5_pc_txfir_st txfir, txfir_p3, txfir_m3;

    EFUN(_plp_aperta2_peregrine5_pc_validate_individual_taps(sa__, enable_taps, pre3, pre2, pre1, main, post1, post2));
    EFUN(_plp_aperta2_peregrine5_pc_get_txfir_st(sa__, enable_taps, pre3, pre2, pre1, main, post1, post2, &txfir));
    EFUN(_plp_aperta2_peregrine5_pc_get_txfir_nlc_st(sa__, nlc_upper_eye_pct, nlc_lower_eye_pct, pre3, pre2, pre1, main, post1, post2, &txfir_p3, &txfir_m3));

    return (plp_aperta2_peregrine5_pc_INTERNAL_validate_full_nlc_txfir_cfg(sa__, enable_taps, nlc_upper_eye_pct, nlc_lower_eye_pct, &txfir, &txfir_p3, &txfir_m3));
        }
err_code_t plp_aperta2_peregrine5_pc_apply_txfir_cfg_with_nlc(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int8_t nlc_pct, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2) {
    INIT_SRDS_ERR_CODE

    EFUN(plp_aperta2_peregrine5_pc_apply_txfir_cfg_with_asymm_nlc(sa__, enable_taps, nlc_pct, nlc_pct, pre3, pre2, pre1, main, post1, post2));
    return ERR_CODE_NONE;
    }
err_code_t plp_aperta2_peregrine5_pc_apply_txfir_cfg_with_asymm_nlc(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int8_t nlc_upper_eye_pct, int8_t nlc_lower_eye_pct, int16_t pre3, int16_t pre2, int16_t pre1, int16_t main, int16_t post1, int16_t post2) {
    INIT_SRDS_ERR_CODE
    peregrine5_pc_txfir_st txfir, txfir_p3, txfir_m3;
    int16_t dc_adjust = 0;

    EFUN(plp_aperta2_peregrine5_pc_validate_txfir_cfg_with_nlc(sa__, enable_taps, nlc_upper_eye_pct, nlc_lower_eye_pct, pre3, pre2, pre1, main, post1, post2));
    EFUN(_plp_aperta2_peregrine5_pc_get_txfir_st(sa__, enable_taps, pre3, pre2, pre1, main, post1, post2, &txfir));

    EFUN(_plp_aperta2_peregrine5_pc_get_txfir_nlc_st(sa__, nlc_upper_eye_pct, nlc_lower_eye_pct, pre3, pre2, pre1, main, post1, post2, &txfir_p3, &txfir_m3));

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_nlc_txfir_dc_adjust(sa__, &txfir, &txfir_p3, &txfir_m3, &dc_adjust));
#if !defined(SMALL_FOOTPRINT) 
    EFUN(_plp_aperta2_peregrine5_pc_log_api_txfir_event(sa__, enable_taps));
#endif
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_apply_nlc_txfir_cfg(sa__, &txfir_p3, &txfir_m3, dc_adjust));
    return plp_aperta2_peregrine5_pc_INTERNAL_apply_full_txfir_cfg(sa__, enable_taps, &txfir);
}

err_code_t plp_aperta2_peregrine5_pc_get_tx_nlc_pct(srds_access_t *sa__, int8_t *nlc_pct) {
    INIT_SRDS_ERR_CODE
    int8_t nlc_pct_m = 0;

    EFUN(plp_aperta2_peregrine5_pc_get_tx_asymm_nlc_pct(sa__, nlc_pct, &nlc_pct_m));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_get_tx_asymm_nlc_pct(srds_access_t *sa__, int8_t *nlc_upper_eye_pct, int8_t *nlc_lower_eye_pct) {
    INIT_SRDS_ERR_CODE
    int16_t main_tap, main_p3_lvl, main_m3_lvl;

    if((nlc_upper_eye_pct == NULL) || (nlc_lower_eye_pct == NULL)) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

    ESTM(main_tap = rd_txfir_main_tap());
    ESTM(main_p3_lvl = rd_txfir_main_tap_p3_lvl_adjust());
    if(main_p3_lvl == 0) {
        *nlc_upper_eye_pct = 0;
    }
    else {
    if(main_tap == 0) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

        *nlc_upper_eye_pct = (int8_t)plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div((50 * main_p3_lvl), main_tap);
    }

    ESTM(main_m3_lvl = rd_txfir_main_tap_m3_lvl_adjust());
    if(main_m3_lvl == 0) {
        *nlc_lower_eye_pct = 0;
        return ERR_CODE_NONE;
    }

    if(main_tap == 0) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

    *nlc_lower_eye_pct = (int8_t)plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div((-50 * main_m3_lvl), main_tap);

    return ERR_CODE_NONE;
}

/**********************************************/
/*  Loopback and Ultra-Low Latency Functions  */
/**********************************************/
/* Locks TX_PI to Loop timing from any Rx to any Tx*/
err_code_t plp_aperta2_peregrine5_pc_asymmetric_loop_timing(srds_access_t *sa__, uint8_t timing_src_lane, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    uint8_t current_lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    peregrine5_pc_osr_mode_st osr_mode;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_osr_mode(sa__, &osr_mode));

    if (enable) {
        uint8_t physical_rx_lane;
        /* switch to rx_lane and enable phase_sum_val_logic */
        EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, timing_src_lane));
        /* remember the physical rx lane number for forcing loop timing selection in TX PI */
        ESTM(physical_rx_lane = peregrine5_pc_acc_get_physical_lane(sa__));
        EFUN(wr_tx_pi_loop_timing_src_sel(0x1)); /* RX phase_sum_val_logic enable */
        EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, current_lane));
        if (osr_mode.tx == PEREGRINE5_PC_OSX5) {
            EFUN(wr_tx_pi_sampler_max_cnt(9));
        }
        /* Configure TX PI to do loop timing */
        /* force asymmetric loop timing selection */
        EFUN(wr_tx_pi_loop_timing_sel_frc_val(physical_rx_lane));
        EFUN(wr_tx_pi_loop_timing_sel_frc(1));
        EFUN(wr_tx_pi_en(0x1));                  /* TX_PI enable: 0 = disabled, 1 = enabled */
        EFUN(wr_tx_pi_jitter_filter_en(0x1));    /* Jitter filter enable to lock freq: 0 = disabled, 1 = enabled */
        EFUN(USR_DELAY_US(25));               /* Wait for tclk to lock to CDR */
    } else {
        EFUN(wr_tx_pi_loop_timing_sel_frc_val(0));
        EFUN(wr_tx_pi_loop_timing_sel_frc(0));
        EFUN(wr_tx_pi_jitter_filter_en(0x0));    /* Jitter filter enable to lock freq: 0 = disabled, 1 = enabled */
        EFUN(wr_tx_pi_en(0x0));                  /* TX_PI enable: 0 = disabled, 1 = enabled */
        if (osr_mode.tx == PEREGRINE5_PC_OSX5) {
            EFUN(wr_tx_pi_sampler_max_cnt(7));
        }
        /* switch to rx_lane and disable phase_sum_val_logic */
        EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, timing_src_lane));
        EFUN(wr_tx_pi_loop_timing_src_sel(0x0)); /* RX phase_sum_val_logic disable */
    }
    /*restore to original lane */
    EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, current_lane));
    return (ERR_CODE_NONE);
}

/* Locks TX_PI to Loop timing */
err_code_t plp_aperta2_peregrine5_pc_loop_timing(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    peregrine5_pc_osr_mode_st osr_mode;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_osr_mode(sa__, &osr_mode));

    if (enable) {
        EFUN(wr_tx_pi_loop_timing_src_sel(0x1)); /* RX phase_sum_val_logic enable */
        if (osr_mode.tx == PEREGRINE5_PC_OSX5) {
            EFUN(wr_tx_pi_sampler_max_cnt(9));
        }
        EFUN(wr_tx_pi_en(0x1));                  /* TX_PI enable: 0 = disabled, 1 = enabled */
        EFUN(wr_tx_pi_jitter_filter_en(0x1));    /* Jitter filter enable to lock freq: 0 = disabled, 1 = enabled */
        EFUN(USR_DELAY_US(25));               /* Wait for tclk to lock to CDR */
    }
    else {
        EFUN(wr_tx_pi_jitter_filter_en(0x0));    /* Jitter filter enable to lock freq: 0 = disabled, 1 = enabled */
        EFUN(wr_tx_pi_en(0x0));                  /* TX_PI enable: 0 = disabled, 1 = enabled */
        if (osr_mode.tx == PEREGRINE5_PC_OSX5) {
            EFUN(wr_tx_pi_sampler_max_cnt(7));
        }
        EFUN(wr_tx_pi_loop_timing_src_sel(0x0)); /* RX phase_sum_val_logic enable */
    }
    return (ERR_CODE_NONE);
}

/* Setup Remote Loopback mode  */
err_code_t plp_aperta2_peregrine5_pc_rmt_lpbk(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    if (enable) {
        err_code_t lane_map_error = plp_aperta2_peregrine5_pc_INTERNAL_lpbk_lane_map_check(sa__);
        if(lane_map_error == ERR_CODE_RX_TX_LANE_MISMATCH) {
            EFUN_PRINTF(("WARNING: TX and RX are not mapped to the same physical lane. Remote loopback not performed.\n"));
            return ERR_CODE_NONE;
        }
        else if(lane_map_error != ERR_CODE_NONE) {
            return lane_map_error;
        }
        EFUN(plp_aperta2_peregrine5_pc_loop_timing(sa__, enable));
        EFUN(wr_tx_pi_ext_ctrl_en(0x1));  /* PD path enable: 0 = disabled, 1 = enabled */
        EFUN(wr_rmt_lpbk_en(0x1));        /* Remote Loopback Enable: 0 = disabled, 1 = enable  */
        EFUN(USR_DELAY_US(50));        /* Wait for rclk and tclk phase lock before expecing good data from rmt loopback */
    } else {                              /* Might require longer wait time for smaller values of tx_pi_ext_phase_bwsel_integ */
        EFUN(wr_rmt_lpbk_en(0x0));        /* Remote Loopback Enable: 0 = disabled, 1 = enable  */
        EFUN(wr_tx_pi_ext_ctrl_en(0x0));  /* PD path enable: 0 = disabled, 1 = enabled */
        EFUN(plp_aperta2_peregrine5_pc_loop_timing(sa__, enable));
    }
    return (ERR_CODE_NONE);
}


/********************************/
/*  TX_PI Fixed Frequency Mode  */
/********************************/
#ifndef SMALL_FOOTPRINT
/* TX_PI Frequency Override (Fixed Frequency Mode) */
err_code_t plp_aperta2_peregrine5_pc_tx_pi_freq_override(srds_access_t *sa__, uint8_t enable, int16_t freq_override_val) {
    INIT_SRDS_ERR_CODE
    if (enable) {
        EFUN(wr_tx_pi_en(0x1));                              /* TX_PI enable :            0 = disabled, 1 = enabled */
        EFUN(wr_tx_pi_freq_override_val((uint16_t)freq_override_val)); /* Fixed Freq Override Value (+/-8192) */
        EFUN(wr_tx_pi_freq_override_en(0x1));                /* Fixed freq mode enable:   0 = disabled, 1 = enabled */
    }
    else {
        EFUN(wr_tx_pi_freq_override_en(0x0));                /* Fixed freq mode enable:   0 = disabled, 1 = enabled */
        EFUN(wr_tx_pi_freq_override_val(0));                 /* Fixed Freq Override Value to 0 */
        EFUN(wr_tx_pi_en(0x0));                              /* TX_PI enable :            0 = disabled, 1 = enabled */
    }
  return (ERR_CODE_NONE);
}
#endif /* SMALL_FOOTPRINT */

/*********************************/
/*  uc_active and uc_reset APIs  */
/*********************************/
err_code_t plp_aperta2_peregrine5_pc_get_micro_num_uc_cores(srds_access_t *sa__, uint8_t *num_micros) {
    INIT_SRDS_ERR_CODE
    UNUSED(sa__);
    ESTM(*num_micros = rdc_micro_num_uc_cores());
    return (ERR_CODE_NONE);
}

/* Toggle core reset */
err_code_t plp_aperta2_peregrine5_pc_core_reset(srds_access_t *sa__, ucode_info_t ucode_info) {
    INIT_SRDS_ERR_CODE
    uint8_t i, num_lanes;
    uint8_t orig_pll = plp_aperta2_peregrine5_pc_acc_get_pll_idx(sa__);
    uint8_t orig_lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    EFUN(plp_aperta2_peregrine5_pc_uc_reset(sa__, 1, ucode_info));   /* Assert uC reset */
    EFUN(wrc_micro_clk_s_comclk_sel(0x1));        /* Select com_clk as micro_clk */
    EFUN(wrc_mdio_brcst_port_addr(0x1f));         /* mdio_brcst_port_addr is supposed to be reset to default 0x1f, but can't in fact. So has to be set explicitly.*/
    ESTM(num_lanes = rdc_revid_multiplicity());
    for(i = 0; i < num_lanes; i++) {
        EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, i));
        EFUN(plp_aperta2_peregrine5_pc_ln_dp_reset(sa__, 1));
    }
    EFUN(USR_DELAY_US(1));
    for (i = 0; i < NUM_PLLS; i++) {
        EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, i));
        EFUN(plp_aperta2_peregrine5_pc_core_dp_reset(sa__, 1));
    }
    EFUN(USR_DELAY_US(1));
    EFUN(wrc_core_s_rstb(0x0));                   /* Assert core reset */
    EFUN(wrc_core_s_rstb(0x1));                   /* Deassert core reset */
    EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, orig_lane));
    EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, orig_pll));
    return (ERR_CODE_NONE);
}

/* Enable or Disable the uC reset; Dummy function to maintain compatibility with BHK16 APIs */
err_code_t plp_aperta2_peregrine5_pc_uc_reset_with_info(srds_access_t *sa__, uint8_t enable, ucode_info_t ucode_info) {
    return plp_aperta2_peregrine5_pc_uc_reset(sa__, enable, ucode_info);
}

/* Enable or Disable the uC reset */
err_code_t plp_aperta2_peregrine5_pc_uc_reset(srds_access_t *sa__, uint8_t enable, ucode_info_t ucode_info) {
    INIT_SRDS_ERR_CODE
  if (enable) {
    EFUN(wrc_micro_micro_s_rstb(0x0));                    /* Toggle micro_reset to reset all micro registers to default value */
    EFUN(wrc_micro_micro_s_rstb(0x1));
    EFUN(wrc_micro_cr_crc_calc_en(0x0));                  /* Default value of crc calc en is 1. Set to 0 to stop crc calculation. */

    {
      uint16_t stack_size;
      if (ucode_info.stack_size != 0) {
         stack_size = ucode_info.stack_size;
         EFUN(wrc_micro_core_stack_size(stack_size));
         EFUN(wrc_micro_core_stack_en(1));
      } else {
          EFUN_PRINTF(("ERROR:  Stack Size in ucode_info is Zero.\n"));
          return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
      }
    }
    if(ucode_info.ucode_dr_code_size_kb != 0) {
        EFUN(wrc_micro_dr_code_size(ucode_info.ucode_dr_code_size_kb));
    }
    EFUN(wrc_uc_active(0));  /* This register is only cleared by POR */
  }
  else {
    EFUN(wrc_micro_micro_s_rstb(0x1));                    /* De-assert micro reset */
    /* De-assert micro reset - Start executing code */
    EFUN(wrc_micro_master_clk_en (0x1));                  /* Enable clock to microcontroller subsystem */
    EFUN(wrc_micro_master_rstb   (0x1));                  /* De-assert reset to microcontroller sybsystem */
    EFUN(wrc_micro_cr_access_en  (0x1));
    EFUN(wrc_micro_ra_init(0x2));                         /* Write command for data RAM initialization */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_poll_micro_ra_initdone(sa__, 5));/* Poll status of data RAM initialization */
    EFUN(wrc_micro_ra_init(0x0));                         /* Clear command for data RAM initialization */
    {
        uint8_t micro_orig, num_micros = 0;
        int8_t micro_idx;
        ESTM(micro_orig = plp_aperta2_peregrine5_pc_acc_get_micro_idx(sa__));
        EFUN(plp_aperta2_peregrine5_pc_get_micro_num_uc_cores(sa__, &num_micros));
        for (micro_idx = (int8_t)(num_micros-1); micro_idx >= 0; micro_idx--) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_micro_idx(sa__, (uint8_t)micro_idx));
            EFUN(wrc_micro_core_clk_en(0x1));          /* Enable clock to micro core */
            EFUN(wrc_micro_m0p_gclk_frc(0));
            EFUN(wrc_micro_core_rstb(0x1));
        }
        EFUN(plp_aperta2_peregrine5_pc_acc_set_micro_idx(sa__, micro_orig));
    }
    UNUSED(ucode_info);
  }
  return (ERR_CODE_NONE);
}

/* Wait for uC to become active */
err_code_t plp_aperta2_peregrine5_pc_wait_uc_active(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    uint16_t loop;
    for (loop = 0; loop < 10000; loop++) {
        uint16_t rddata;
        uint8_t core_uc_active=0;
        ESTM(core_uc_active = rdc_uc_active());
        ESTM(rddata = (reg_rdc_MICRO_B_COM_RMI_MICRO_SDK_STATUS0() & 0xF));
        if ((rddata == 0xF) && (core_uc_active == 1)) {
            return (ERR_CODE_NONE);
        }
        if (loop>10) {
            EFUN(USR_DELAY_US(1));
        }
    }
    return (peregrine5_pc_error(sa__, ERR_CODE_MICRO_INIT_NOT_DONE));
}

static err_code_t _plp_aperta2_peregrine5_pc_get_silicon_version(srds_access_t *sa__, uint8_t *silicon_version);
static err_code_t _plp_aperta2_peregrine5_pc_get_silicon_version(srds_access_t *sa__, uint8_t *silicon_version) {
    INIT_SRDS_ERR_CODE
    uint8_t revid2;
    ESTM(revid2 = rdc_revid2());
    if(revid2 == PEREGRINE5_PC_REVID2_DEFAULT_A0_1) {
        *silicon_version = PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_1;
    }
    else if(revid2 == PEREGRINE5_PC_REVID2_DEFAULT_A0_2) {
        *silicon_version = PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_2;
    }
    else {
        *silicon_version = PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_6;
    }
    return ERR_CODE_NONE;
}

static err_code_t _plp_aperta2_peregrine5_pc_ucode_core_ip_check(srds_access_t *sa__);
err_code_t _plp_aperta2_peregrine5_pc_ucode_core_ip_check(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    uint8_t status_byte;
    /* Check to make sure that ucode IP matches core IP */
    ESTM(status_byte = rdcv_status_byte());
    if (status_byte & 0x20) {
        EFUN_PRINTF(("Microcode IP did not match core! (status_byte = 0x%x)\n", status_byte));
        return ERR_CODE_UCODE_IP_DOES_NOT_MATCH_CORE;
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_init_peregrine5_pc_info(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    srds_info_t * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr(sa__); /* Never put a check here */
    uint32_t info_table[INFO_TABLE_END / sizeof(uint32_t)];
    uint32_t info_table_signature;
    uint8_t  info_table_version;
    err_code_t err_code = ERR_CODE_NONE;

    ENULL_MEMSET(info_table, 0, sizeof(info_table));

    USR_ACQUIRE_LOCK;

    if (peregrine5_pc_info_ptr == NULL) {
        EFUN_PRINTF(("ERROR:  Info table pointer is null.\n"));
        RELEASE_LOCK_AND_RETURN (peregrine5_pc_error(sa__, ERR_CODE_INFO_TABLE_ERROR));
    }
        if (peregrine5_pc_info_ptr->signature == SRDS_INFO_SIGNATURE) {
            /*Already initalized and so check for microcode version and exit with proper status*/
            err_code = plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr);

            if (err_code == ERR_CODE_NONE) {
                /* ucode version match */
                RELEASE_LOCK_AND_RETURN (ERR_CODE_NONE);
            } else {
                /* ucode version mismatch */
                RELEASE_LOCK_AND_RETURN (peregrine5_pc_error(sa__, ERR_CODE_MICRO_INIT_NOT_DONE));
            }
        }
        /*Never initialized so far, continue with initialization */

        err_code = plp_aperta2_peregrine5_pc_rdblk_uc_prog_ram(sa__, (uint8_t *)info_table, INFO_TABLE_RAM_BASE, INFO_TABLE_END);
        /* Release lock before returning from the call to avoid deadlock */
        if (ERR_CODE_NONE != err_code) {
           RELEASE_LOCK_AND_RETURN(peregrine5_pc_error(sa__, err_code));
        }

        info_table_signature = info_table[INFO_TABLE_OFFS_SIGNATURE / sizeof(uint32_t)];
        info_table_version = (uint8_t)(info_table_signature >> 24);
    if (((info_table_signature & 0x00FFFFFF) != (INFO_TABLE_SIGNATURE  & 0x00FFFFFF))
            || (!(   ((info_table_version >= 0x32) && (info_table_version <= 0x39))
                  || ((info_table_version >= 0x41) && (info_table_version <= 0x5A))))) {
            RELEASE_LOCK_AND_RETURN (peregrine5_pc_error(sa__, ERR_CODE_INFO_TABLE_ERROR));
        }

        peregrine5_pc_info_ptr->lane_count = info_table[INFO_TABLE_OFFS_OTHER_SIZE / sizeof(uint32_t)] & 0xFF;
        peregrine5_pc_info_ptr->trace_memory_descending_writes = ((info_table[INFO_TABLE_OFFS_OTHER_SIZE / sizeof(uint32_t)] & 0x1000000) != 0);

        peregrine5_pc_info_ptr->core_var_ram_size = (info_table[INFO_TABLE_OFFS_OTHER_SIZE / sizeof(uint32_t)] >> 8) & 0xFF;

        peregrine5_pc_info_ptr->lane_var_ram_size = (info_table[INFO_TABLE_OFFS_TRACE_LANE_MEM_SIZE / sizeof(uint32_t)] >> 16) & 0xFFFF;

        peregrine5_pc_info_ptr->diag_mem_ram_size = (info_table[INFO_TABLE_OFFS_DIAG_LANE_MEM_SIZE / sizeof(uint32_t)] & 0xFFFF);
        peregrine5_pc_info_ptr->diag_mem_ram_base = info_table[INFO_TABLE_OFFS_DIAG_MEM_PTR / sizeof(uint32_t)];

        peregrine5_pc_info_ptr->trace_mem_ram_size = (info_table[INFO_TABLE_OFFS_TRACE_LANE_MEM_SIZE / sizeof(uint32_t)] & 0xFFFF);
        peregrine5_pc_info_ptr->trace_mem_ram_base = info_table[INFO_TABLE_OFFS_TRACE_MEM_PTR / sizeof(uint32_t)];

        peregrine5_pc_info_ptr->core_var_ram_base = info_table[INFO_TABLE_OFFS_CORE_MEM_PTR / sizeof(uint32_t)];

        peregrine5_pc_info_ptr->micro_var_ram_base = info_table[INFO_TABLE_OFFS_MICRO_MEM_PTR / sizeof(uint32_t)];
        peregrine5_pc_info_ptr->micro_var_ram_size = (info_table[INFO_TABLE_OFFS_OTHER_SIZE_2 / sizeof(uint32_t)] >> 4) & 0xFF;

        peregrine5_pc_info_ptr->lane_var_ram_base = info_table[INFO_TABLE_OFFS_LANE_MEM_PTR / sizeof(uint32_t)];
            peregrine5_pc_info_ptr->lane_static_var_ram_base = info_table[INFO_TABLE_OFFS_LANE_STATIC_MEM_PTR  / sizeof(uint32_t)];
            peregrine5_pc_info_ptr->lane_static_var_ram_size = info_table[INFO_TABLE_OFFS_LANE_STATIC_MEM_SIZE / sizeof(uint32_t)];
        peregrine5_pc_info_ptr->common_block_mem_ram_base = info_table[INFO_TABLE_OFFS_COMMON_BLOCK_MEM_PTR / sizeof(uint32_t)];
        peregrine5_pc_info_ptr->common_block_mem_ram_size = info_table[INFO_TABLE_OFFS_COMMON_BLOCK_MEM_SIZE / sizeof(uint32_t)];
        peregrine5_pc_info_ptr->debug_block_mem_ram_base = info_table[INFO_TABLE_OFFS_DEBUG_BLOCK_MEM_PTR / sizeof(uint32_t)];
        peregrine5_pc_info_ptr->debug_block_mem_ram_size = info_table[INFO_TABLE_OFFS_DEBUG_BLOCK_MEM_SIZE / sizeof(uint32_t)];


        peregrine5_pc_info_ptr->micro_count = info_table[INFO_TABLE_OFFS_OTHER_SIZE_2 / sizeof(uint32_t)] & 0xF;
        peregrine5_pc_info_ptr->grp_ram_size = info_table[INFO_TABLE_OFFS_GROUP_RAM_SIZE / sizeof(uint32_t)] & 0xFFFF;
        peregrine5_pc_info_ptr->ucode_version = info_table[INFO_TABLE_OFFS_UC_VERSION /sizeof(uint32_t)];
        peregrine5_pc_info_ptr->info_table_version = info_table_version;
        peregrine5_pc_info_ptr->is_big_endian = plp_aperta2_peregrine5_pc_INTERNAL_is_big_endian();
        err_code = _plp_aperta2_peregrine5_pc_get_silicon_version(sa__, &peregrine5_pc_info_ptr->silicon_version);
        /* Release lock before returning from the call to avoid deadlock */
        if (ERR_CODE_NONE != err_code) {
            RELEASE_LOCK_AND_RETURN (peregrine5_pc_error(sa__, err_code));
        }

        /* The info table signature should be the last info table member which is populated */
        peregrine5_pc_info_ptr->signature = SRDS_INFO_SIGNATURE;

        err_code = _plp_aperta2_peregrine5_pc_ucode_core_ip_check(sa__);
        if (ERR_CODE_NONE != err_code) {
            RELEASE_LOCK_AND_RETURN (peregrine5_pc_error(sa__, err_code));
        }		
    RELEASE_LOCK_AND_RETURN (ERR_CODE_NONE);
}


err_code_t plp_aperta2_peregrine5_pc_clear_peregrine5_pc_info(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    srds_info_t * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr(sa__); /* Never put a check here */

    USR_ACQUIRE_LOCK;

    if (peregrine5_pc_info_ptr == NULL) {
        EFUN_PRINTF(("ERROR:  Info table pointer is null.\n"));
        RELEASE_LOCK_AND_RETURN (peregrine5_pc_error(sa__, ERR_CODE_INFO_TABLE_ERROR));
    }

    ENULL_MEMSET(peregrine5_pc_info_ptr, 0, sizeof(srds_info_t));

    RELEASE_LOCK_AND_RETURN (ERR_CODE_NONE);
}

#ifndef SMALL_FOOTPRINT
/*-----------------*/
/*  Configure PLL  */
/*-----------------*/

err_code_t plp_aperta2_peregrine5_pc_configure_pll_powerdown(srds_access_t *sa__) {
    return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, PEREGRINE5_PC_PLL_REFCLK_UNKNOWN, PEREGRINE5_PC_PLL_DIV_UNKNOWN, 0, PEREGRINE5_PC_PLL_OPTION_POWERDOWN);
}

err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_div(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           enum peregrine5_pc_pll_div_enum srds_div) {
    return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, refclk, srds_div, 0, PEREGRINE5_PC_PLL_OPTION_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_vco(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           uint32_t vco_freq_khz) {
    return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, refclk, PEREGRINE5_PC_PLL_DIV_UNKNOWN, vco_freq_khz, PEREGRINE5_PC_PLL_OPTION_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_configure_pll_div_vco(srds_access_t *sa__,
                                        enum peregrine5_pc_pll_div_enum srds_div,
                                        uint32_t vco_freq_khz) {
    return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, PEREGRINE5_PC_PLL_REFCLK_UNKNOWN, srds_div, vco_freq_khz, PEREGRINE5_PC_PLL_OPTION_NONE);
}

/* following routines divide refclk input by 2 to prevent fracn mode */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_div_div2refclk(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           enum peregrine5_pc_pll_div_enum srds_div) {
   return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, refclk, srds_div, 0, PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV2_EN);
}

err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_vco_div2refclk(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           uint32_t vco_freq_khz) {
    return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, refclk, PEREGRINE5_PC_PLL_DIV_UNKNOWN, vco_freq_khz, PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV2_EN);
}

err_code_t plp_aperta2_peregrine5_pc_configure_pll_div_vco_div2refclk(srds_access_t *sa__,
                                        enum peregrine5_pc_pll_div_enum srds_div,
                                        uint32_t vco_freq_khz) {
    return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, PEREGRINE5_PC_PLL_REFCLK_UNKNOWN, srds_div, vco_freq_khz, PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV2_EN);
}
/* following routines divide refclk input by 4 to prevent fracn mode */
err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_div_div4refclk(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           enum peregrine5_pc_pll_div_enum srds_div) {
   return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, refclk, srds_div, 0, PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV4_EN);
}

err_code_t plp_aperta2_peregrine5_pc_configure_pll_refclk_vco_div4refclk(srds_access_t *sa__,
                                           enum peregrine5_pc_pll_refclk_enum refclk,
                                           uint32_t vco_freq_khz) {
    return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, refclk, PEREGRINE5_PC_PLL_DIV_UNKNOWN, vco_freq_khz, PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV4_EN);
}

err_code_t plp_aperta2_peregrine5_pc_configure_pll_div_vco_div4refclk(srds_access_t *sa__,
                                        enum peregrine5_pc_pll_div_enum srds_div,
                                        uint32_t vco_freq_khz) {
    return plp_aperta2_peregrine5_pc_INTERNAL_configure_pll(sa__, PEREGRINE5_PC_PLL_REFCLK_UNKNOWN, srds_div, vco_freq_khz, PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV4_EN);
}


err_code_t plp_aperta2_peregrine5_pc_get_vco_from_refclk_div(srds_access_t *sa__, uint32_t refclk_freq_hz, enum peregrine5_pc_pll_div_enum srds_div, uint32_t *vco_freq_khz, enum peregrine5_pc_pll_option_enum pll_option) {
    INIT_SRDS_ERR_CODE
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_vco_from_refclk_div(sa__, refclk_freq_hz, srds_div, vco_freq_khz, pll_option));
    return (ERR_CODE_NONE);
}

/***********************************************/
/*  Microcode Load into Program RAM Functions  */
/***********************************************/
err_code_t plp_aperta2_peregrine5_pc_ucode_load_init(srds_access_t *sa__, uint8_t pram) {
    INIT_SRDS_ERR_CODE
    IF_API_ID_CODES_DONT_MATCH_CORE_ID_CODES
       return (ERR_CODE_API_IP_DOES_NOT_MATCH_CORE);
    }
    EFUN(wrc_micro_master_clk_en(0x1));                     /* Enable clock to microcontroller subsystem */
    EFUN(wrc_micro_master_rstb(0x1));                       /* De-assert reset to microcontroller sybsystem */
    EFUN(wrc_micro_master_rstb(0x0));                       /* Assert reset to microcontroller sybsystem - Toggling reset*/
    EFUN(wrc_micro_master_rstb(0x1));                       /* De-assert reset to microcontroller sybsystem */
    EFUN(wrc_micro_cr_access_en(1));                        /* Allow access to Code RAM */
    EFUN(wrc_micro_ra_init(0x1));                           /* Set initialization command to initialize code RAM */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_poll_micro_ra_initdone(sa__, 5));  /* Poll for micro_ra_initdone = 1 to indicate initialization done */
    EFUN(wrc_micro_ra_init(0x0));

    EFUN(wrc_micro_cr_crc_prtsel(0));
    if(pram) {
        EFUN(wrc_micro_cr_prif_prtsel(0));
    }
    EFUN(wrc_micro_cr_crc_init(1));                         /* Initialize the HW CRC calculation */
    EFUN(wrc_micro_cr_crc_init(0));
    EFUN(wrc_micro_cr_crc_calc_en(1));
    EFUN(wrc_micro_cr_ignore_micro_code_writes(0));         /* Allow writing to program RAM */
    if(pram) {
        EFUN(wrc_micro_pramif_ahb_wraddr_msw(0x0000));
        EFUN(wrc_micro_pramif_ahb_wraddr_lsw(0x0000));
        EFUN(wrc_micro_pram_if_rstb(1));
        EFUN(wrc_micro_pramif_en(1));
    }
    else {
        /* Code to Load microcode */
        EFUN(wrc_micro_autoinc_wraddr_en(0x1));             /* To auto increment RAM write address */
        EFUN(wrc_micro_ra_wrdatasize(0x1));                 /* Select 16bit transfers */
        EFUN(wrc_micro_ra_wraddr_msw(0x0));                 /* Upper 16bits of start address of Program RAM where the ucode is to be loaded */
        EFUN(wrc_micro_ra_wraddr_lsw(0x0));                 /* Lower 16bits of start address of Program RAM where the ucode is to be loaded */
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_ucode_load_write(srds_access_t *sa__, uint8_t *ucode_image, uint32_t ucode_len) {
    INIT_SRDS_ERR_CODE
    uint32_t   ucode_len_padded, count = 0;
    uint16_t   wrdata_lsw;
    uint8_t    wrdata_lsb;

    if(!ucode_image) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ucode_len_padded = ((ucode_len + 3) & 0xFFFFFFFC);    /* Aligning ucode size to 4-byte boundary */

    do {                                                  /* ucode_image loaded 16bits at a time */
        wrdata_lsb = (count < ucode_len) ? ucode_image[count] : 0x0; /* wrdata_lsb read from ucode_image; zero padded to 4byte boundary */
        count++;
        wrdata_lsw = (count < ucode_len) ? ucode_image[count] : 0x0; /* wrdata_msb read from ucode_image; zero padded to 4byte boundary */
        count++;
        wrdata_lsw = (uint16_t)((wrdata_lsw << 8) | wrdata_lsb);               /* 16bit wrdata_lsw formed from 8bit msb and lsb values read from ucode_image */
        EFUN(wrc_micro_ra_wrdata_lsw(wrdata_lsw));                   /* Program RAM lower 16bits write data */
    }   while (count < ucode_len_padded);                 /* Loop repeated till entire image loaded (upto the 4byte boundary) */

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_ucode_load_close(srds_access_t *sa__, uint8_t pram) {
    INIT_SRDS_ERR_CODE

    if(!pram) {
        EFUN(wrc_micro_ra_wrdatasize(0x2));               /* Select 32bit transfers as default */
    }
    EFUN(wrc_micro_cr_ignore_micro_code_writes(1));       /* block writing to program RAM */
    EFUN(wrc_micro_cr_crc_calc_en(0));

    {
        uint8_t micro_orig, num_micros = 0, micro_idx;
        ESTM(micro_orig = plp_aperta2_peregrine5_pc_acc_get_micro_idx(sa__));
        EFUN(plp_aperta2_peregrine5_pc_get_micro_num_uc_cores(sa__, &num_micros));
        for (micro_idx = 0; micro_idx < num_micros; micro_idx++) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_micro_idx(sa__, micro_idx));
            EFUN(wrc_micro_core_clk_en(0x1));             /* Enable clock to micro core */
        }
        EFUN(plp_aperta2_peregrine5_pc_acc_set_micro_idx(sa__, micro_orig));
    }

    return (ERR_CODE_NONE);                               /* NO Errors while loading microcode (uCode Load PASS) */

}

/* uCode Load through Register (MDIO) Interface [Return Val = Error_Code (0 = PASS)] */
err_code_t plp_aperta2_peregrine5_pc_ucode_load(srds_access_t *sa__, uint8_t *ucode_image, uint32_t ucode_len) {
    INIT_SRDS_ERR_CODE

    EFUN(plp_aperta2_peregrine5_pc_ucode_load_init(sa__, 0));
    EFUN(plp_aperta2_peregrine5_pc_ucode_load_write(sa__, ucode_image, ucode_len));
    EFUN(plp_aperta2_peregrine5_pc_ucode_load_close(sa__, 0));

    return (ERR_CODE_NONE);
}


/* Read-back uCode from Program RAM and verify against ucode_image [Return Val = Error_Code (0 = PASS)]  */
err_code_t plp_aperta2_peregrine5_pc_ucode_load_verify(srds_access_t *sa__, uint8_t *ucode_image, uint32_t ucode_len) {
    INIT_SRDS_ERR_CODE

    uint32_t ucode_len_padded, count = 0;
    uint16_t rddata_lsw;
    uint16_t data_lsw;
    uint8_t  rddata_lsb;

    if(!ucode_image) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ucode_len_padded = ((ucode_len + 3) & 0xFFFFFFFC);    /* Aligning ucode size to 4-byte boundary */

    EFUN(wrc_micro_autoinc_rdaddr_en(0x1));               /* To auto increment RAM read address */
    EFUN(wrc_micro_ra_rddatasize(0x1));                   /* Select 16bit transfers */
    EFUN(wrc_micro_ra_rdaddr_msw(0x0));                   /* Upper 16bits of start address of Program RAM from where to read ucode */
    EFUN(wrc_micro_ra_rdaddr_lsw(0x0));                   /* Lower 16bits of start address of Program RAM from where to read ucode */

    do {                                                  /* ucode_image read 16bits at a time */
        rddata_lsb = (count < ucode_len) ? ucode_image[count] : 0x0; /* rddata_lsb read from ucode_image; zero padded to 4byte boundary */
        count++;
        rddata_lsw = (count < ucode_len) ? ucode_image[count] : 0x0; /* rddata_msb read from ucode_image; zero padded to 4byte boundary */
        count++;
        rddata_lsw = (uint16_t)((rddata_lsw << 8) | rddata_lsb);               /* 16bit rddata_lsw formed from 8bit msb and lsb values read from ucode_image */
                                                                     /* Compare Program RAM ucode to ucode_image (Read to micro_ra_rddata_lsw reg auto-increments the ram_address) */
        ESTM(data_lsw = rdc_micro_ra_rddata_lsw());
        if (data_lsw != rddata_lsw) {
            EFUN_PRINTF(("Ucode_Load_Verify_FAIL: Addr = 0x%x: Read_data = 0x%x :  Expected_data = 0x%x \n",(count-2),data_lsw,rddata_lsw));
            return (peregrine5_pc_error(sa__, ERR_CODE_UCODE_VERIFY_FAIL));             /* Verify uCode FAIL */
        }

    } while (count < ucode_len_padded);                   /* Loop repeated till entire image loaded (upto the 4byte boundary) */

    EFUN(wrc_micro_ra_rddatasize(0x2));                   /* Select 32bit transfers ad default */
    return (ERR_CODE_NONE);                               /* Verify uCode PASS */
}

#endif /* SMALL_FOOTPRINT */

/************************/
/*  Fast Link Recovery  */
/************************/
err_code_t plp_aperta2_peregrine5_pc_configure_fast_link_recovery(srds_access_t *sa__, uint8_t enable, uint8_t enable_timeout, uint32_t timeout_in_ms) {
    INIT_SRDS_ERR_CODE
    uint8_t flr_support = 0;

    uint8_t reset_state;
    ESTM(reset_state = rd_rx_lane_dp_reset_state());
    if(reset_state < 7) {
        EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_configure_fast_link_recovery(..) called without ln_dp_s_rstb=0 Lane=%d reset_state=%d\n",plp_aperta2_peregrine5_pc_acc_get_lane(sa__),reset_state));
        return (peregrine5_pc_error(sa__, ERR_CODE_LANE_DP_NOT_RESET));
    }
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_is_flr_supported(sa__, &flr_support));
    if(!flr_support) {
        EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_configure_fast_link_recovery(..) unsupported with currently loaded microcode (Core=%d Lane=%d).\n", plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));
        return (peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    if (timeout_in_ms > 511) {
        EFUN_PRINTF(("timeout_time value of %d mS for plp_aperta2_peregrine5_pc_configure_fast_link_recovery() is too large. timeout_time must be <= 511 mS.\n", timeout_in_ms));
        return (peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    EFUN(wr_flr_los_timeout((uint16_t)timeout_in_ms));
    if(enable_timeout) {
        EFUN(wr_flr_timer_done_frc(0x0));
    }
    else {
        EFUN(wr_flr_timer_done_frc_val(0x0));
        EFUN(wr_flr_timer_done_frc(0x1));
    }
    EFUN(wr_flr_en((enable ? 1 : 0)));

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_get_configure_fast_link_recovery(srds_access_t *sa__, uint8_t *enable, uint8_t *enable_timeout, uint32_t *timeout_in_ms) {
    INIT_SRDS_ERR_CODE
    uint8_t flr_support = 0;

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_is_flr_supported(sa__, &flr_support));
    if(!flr_support) {
        EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_get_configure_fast_link_recovery(..) unsupported with currently loaded microcode (Core=%d Lane=%d).\n", plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));
        return (peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if((enable == NULL) || (enable_timeout == NULL) ||( timeout_in_ms == NULL)) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    ESTM(*enable = rd_flr_en());
    ESTM(*enable_timeout = (rd_flr_timer_done_frc() == 0));
    ESTM(*timeout_in_ms = rd_flr_los_timeout());
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta2_peregrine5_pc_get_flr_ready_status(srds_access_t *sa__, uint8_t *ready) {
    INIT_SRDS_ERR_CODE
    if(ready == NULL) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }
    ESTM(*ready = rd_rx_tuning_steady_state());
    return(ERR_CODE_NONE);
}

