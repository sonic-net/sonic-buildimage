/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/***********************************************************************************
 ***********************************************************************************
 *  File Name     :  peregrine5_pc_prbs.c                                             *
 *  Created On    :  04 Nov 2015                                                   *
 *  Created By    :  Brent Roberts                                                 *
 *  Description   :  Serdes PRBS test APIs for Serdes IPs                          *
 *  Revision      :     *
 *                                                                                 *
 ***********************************************************************************
 ***********************************************************************************/
#ifdef NON_SDK
#include <stdio.h>
#include <math.h>
#endif

#include <phymod/phymod_system.h>
#include "peregrine5_pc_prbs.h"
#include "peregrine5_pc_common.h"
#include "peregrine5_pc_config.h"
#include "peregrine5_pc_dependencies.h"
#include "peregrine5_pc_functions.h"
#include "peregrine5_pc_internal.h"
#include "peregrine5_pc_internal_error.h"
#include "peregrine5_pc_select_defns.h"
#include "peregrine5_pc_access.h"


/*! @file
 *
 */

#ifndef SMALL_FOOTPRINT


static err_code_t _plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_prbs_chk_lock(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer, uint8_t *prbs_chk_lock);

/******************************/
/*  TX Pattern Generator APIs */
/******************************/

/* Configure shared TX Pattern (Return Val = 0:PASS, 1-6:FAIL (reports 6 possible error scenarios if failed)) */
err_code_t plp_aperta2_peregrine5_pc_config_shared_tx_pattern(srds_access_t *sa__, uint8_t patt_length_bits, const char pattern[]) {
    INIT_SRDS_ERR_CODE

  char       patt_final[246] = "";
  char       patt_mod[246]   = "", bin[5] = "";
  size_t     final_count = 0, mod_count = 0;
  int32_t    bytes_written = 0;
  uint8_t    str_len, i, k, j = 0;
  uint8_t    offset_len, actual_patt_len = 0, hex = 0;
  uint8_t    zero_pad_len    = 0; /* suppress warnings, changed by plp_aperta2_peregrine5_pc_INTERNAL_calc_patt_gen_mode_sel() */
  uint16_t   patt_gen_wr_val = 0;
  uint8_t    mode_sel        = 0; /* suppress warnings, changed by plp_aperta2_peregrine5_pc_INTERNAL_calc_patt_gen_mode_sel() */

  EFUN(plp_aperta2_peregrine5_pc_INTERNAL_calc_patt_gen_mode_sel(sa__, &mode_sel,&zero_pad_len,patt_length_bits));

  /* Generating the appropriate write value to patt_gen_seq registers */
  str_len = (uint8_t)USR_STRLEN(pattern);

  if ((str_len > 2) && ((USR_STRNCMP(pattern, "0x", 2)) == 0)) {
    /* Hexadecimal Pattern */
    for (i=2; i < str_len; i++) {
      if (pattern[i] != '_') {
        actual_patt_len = (uint8_t)(actual_patt_len + 4);
        if (actual_patt_len > 240) {
          EFUN_PRINTF(("ERROR: Pattern bigger than max pattern length\n"));
          return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN));
        }
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_compute_bin(sa__, pattern[i],bin, sizeof(bin)));
        bytes_written = USR_SNPRINTF(patt_mod + mod_count, sizeof(patt_mod) - mod_count, "%s",  bin);
        mod_count += (size_t)bytes_written;
        if(( bytes_written < 0) || (mod_count >= sizeof(patt_mod))) {
          return ERR_CODE_REACHED_BUF_SIZE_LIMIT;
        }
      }
    }

    offset_len = (uint8_t)(actual_patt_len - patt_length_bits);
    if ((offset_len > 3)  || (actual_patt_len < patt_length_bits)) {
      EFUN_PRINTF(("ERROR: Pattern length provided does not match the hexadecimal pattern provided\n"));
      return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
    }
    else if (offset_len) {
      for (i=0; i < offset_len; i++) {
        if (patt_mod[i] != '0') {
          EFUN_PRINTF(("ERROR: Pattern length provided does not match the hexadecimal pattern provided\n"));
          return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
        }
      }
      for (i=offset_len; i <= actual_patt_len; i++) {
        patt_mod[i - offset_len] = patt_mod[i];
      }
    }
  }
  else if ((str_len > 1) && ((USR_STRNCMP(pattern, "p", 1)) == 0)) {
    /* PAM4 Pattern */
    for (i=1; i < str_len; i++) {
      if (pattern[i] != '_') {
        actual_patt_len = (uint8_t)(actual_patt_len + 2);
        if (actual_patt_len > 240) {
          EFUN_PRINTF(("ERROR: Pattern bigger than max pattern length\n"));
          return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN));
        }
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_pam4_to_bin(sa__, pattern[i], bin, sizeof(bin)));
        bytes_written = USR_SNPRINTF(patt_mod + mod_count, sizeof(patt_mod) - mod_count, "%s",  bin);
        mod_count += (size_t)bytes_written;
        if(( bytes_written < 0) || (mod_count >= sizeof(patt_mod))) {
          return ERR_CODE_REACHED_BUF_SIZE_LIMIT;
        }

      }
    }

    offset_len = (uint8_t)(actual_patt_len - patt_length_bits);
    if ((offset_len > 3)  || (actual_patt_len < patt_length_bits)) {
      EFUN_PRINTF(("ERROR: Pattern length provided does not match the PAM4 pattern provided\n"));
      return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
    }
    else if (offset_len) {
      for (i=0; i < offset_len; i++) {
        if (patt_mod[i] != '0') {
          EFUN_PRINTF(("ERROR: Pattern length provided does not match the PAM4 pattern provided\n"));
          return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
        }
      }
      for (i=offset_len; i <= actual_patt_len; i++) {
        patt_mod[i - offset_len] = patt_mod[i];
      }
    }
  }
  else {
    /* Binary Pattern */
    for (i=0; i < str_len; i++) {
      if ((pattern[i] == '0') || (pattern[i] == '1')) {
        actual_patt_len++;
        if (actual_patt_len > 240) {
          EFUN_PRINTF(("ERROR: Pattern bigger than max pattern length\n"));
          return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN));
        }
        bin[0] = pattern[i];
        bin[1] = '\0';
        bytes_written = USR_SNPRINTF(patt_mod + mod_count, sizeof(patt_mod) - mod_count, "%s",  bin);
        mod_count += (size_t)bytes_written;
        if(( bytes_written < 0) || (mod_count >= sizeof(patt_mod))) {
          return ERR_CODE_REACHED_BUF_SIZE_LIMIT;
        }
      }
      else if (pattern[i] != '_') {
        EFUN_PRINTF(("ERROR: Invalid input Pattern\n"));
        return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_INVALID_PATTERN));
      }
    }

    if (actual_patt_len != patt_length_bits) {
      EFUN_PRINTF(("ERROR: Pattern length provided does not match the binary pattern provided\n"));
      return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
    }
  }

  /* Zero padding upper bits and concatenating patt_mod to form patt_final */
  if(zero_pad_len) {
    bytes_written = USR_SNPRINTF(patt_final + final_count, sizeof(patt_final) - final_count, "%0*d", zero_pad_len, 0);
    final_count += (size_t)bytes_written;
    if(( bytes_written < 0) || (final_count >= sizeof(patt_final))) {
      return ERR_CODE_REACHED_BUF_SIZE_LIMIT;
    }
  }
  for (i=zero_pad_len; i + patt_length_bits < 241; i = (uint8_t)(i + patt_length_bits)) {
    bytes_written = USR_SNPRINTF(patt_final + final_count, sizeof(patt_final) - final_count, "%s", patt_mod);
    final_count += (size_t)bytes_written;
    if(( bytes_written < 0) || (final_count >= sizeof(patt_final))) {
      return ERR_CODE_REACHED_BUF_SIZE_LIMIT;
    }
  }

  /* EFUN_PRINTF(("\nFinal Pattern = %s\n\n",patt_final)); */

  for (i=0; i < 15; i++) {

    for (j=0; j < 4; j++) {
      k = (uint8_t)(i*16 + j*4);
      bin[0] = patt_final[k];
      bin[1] = patt_final[k+1];
      bin[2] = patt_final[k+2];
      bin[3] = patt_final[k+3];
      bin[4] = '\0';
      EFUN(plp_aperta2_peregrine5_pc_INTERNAL_compute_hex(sa__,bin, &hex));
      patt_gen_wr_val = (uint16_t)((patt_gen_wr_val << 4) | hex);
    }
    /* EFUN_PRINTF(("patt_gen_wr_val[%d] = 0x%x\n",(14-i),patt_gen_wr_val)); */

    /* Writing to appropriate patt_gen_seq Registers */
    switch (i) {
      case 0:  EFUN(wr_tx_patt_gen_seq_14(patt_gen_wr_val));
               break;
      case 1:  EFUN(wr_tx_patt_gen_seq_13(patt_gen_wr_val));
               break;
      case 2:  EFUN(wr_tx_patt_gen_seq_12(patt_gen_wr_val));
               break;
      case 3:  EFUN(wr_tx_patt_gen_seq_11(patt_gen_wr_val));
               break;
      case 4:  EFUN(wr_tx_patt_gen_seq_10(patt_gen_wr_val));
               break;
      case 5:  EFUN(wr_tx_patt_gen_seq_9(patt_gen_wr_val));
               break;
      case 6:  EFUN(wr_tx_patt_gen_seq_8(patt_gen_wr_val));
               break;
      case 7:  EFUN(wr_tx_patt_gen_seq_7(patt_gen_wr_val));
               break;
      case 8:  EFUN(wr_tx_patt_gen_seq_6(patt_gen_wr_val));
               break;
      case 9:  EFUN(wr_tx_patt_gen_seq_5(patt_gen_wr_val));
               break;
      case 10: EFUN(wr_tx_patt_gen_seq_4(patt_gen_wr_val));
               break;
      case 11: EFUN(wr_tx_patt_gen_seq_3(patt_gen_wr_val));
               break;
      case 12: EFUN(wr_tx_patt_gen_seq_2(patt_gen_wr_val));
               break;
      case 13: EFUN(wr_tx_patt_gen_seq_1(patt_gen_wr_val));
               break;
      case 14: EFUN(wr_tx_patt_gen_seq_0(patt_gen_wr_val));
               break;
      default: EFUN_PRINTF(("ERROR: Invalid write to patt_gen_seq register\n"));
               return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_INVALID_SEQ_WRITE));
    }
  }

  /* Pattern Generator Mode Select */
  /* EFUN(wr_patt_gen_mode_sel(mode_sel)); */
  /* EFUN_PRINTF(("Pattern gen Mode = %d\n",mode));    */

  /* Enable Fixed pattern Generation */
  /* EFUN(wr_patt_gen_en(0x1)); */
  return(ERR_CODE_NONE);
}


/* Enable/Disable Shared TX pattern generator */
err_code_t plp_aperta2_peregrine5_pc_tx_shared_patt_gen_en(srds_access_t *sa__, uint8_t enable, uint8_t patt_length_bits) {
    INIT_SRDS_ERR_CODE
  uint8_t zero_pad_len = 0; /* suppress warnings, changed by plp_aperta2_peregrine5_pc_INTERNAL_calc_patt_gen_mode_sel() */
  uint8_t mode_sel     = 0; /* suppress warnings, changed by plp_aperta2_peregrine5_pc_INTERNAL_calc_patt_gen_mode_sel() */

  EFUN(plp_aperta2_peregrine5_pc_INTERNAL_calc_patt_gen_mode_sel(sa__, &mode_sel,&zero_pad_len,patt_length_bits));

  if (enable) {
    if ((mode_sel < 1) || (mode_sel > 6)) {
      return (peregrine5_pc_error(sa__, ERR_CODE_PATT_GEN_INVALID_MODE_SEL));
    }
    mode_sel = (uint8_t)(12 - mode_sel);
    EFUN(wr_patt_gen_start_pos(mode_sel));            /* Start position for pattern */
    EFUN(wr_patt_gen_stop_pos(0x0));                  /* Stop position for pattern */
    EFUN(wr_patt_gen_en(0x1));                        /* Enable Fixed pattern Generation  */
  }
  else {
    EFUN(wr_patt_gen_en(0x0));                        /* Disable Fixed pattern Generation  */
  }
  return(ERR_CODE_NONE);
}

/* Configure JP03B Pattern */
err_code_t plp_aperta2_peregrine5_pc_config_tx_jp03b_pattern(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE

  if (enable) {
    /* JP03B Pattern - "p0330" => 8'b_0011_1100 or 8'h3C w/o  Gray coding */
    /*                         => 8'b_0010_1000 or 8'h28 with Gray coding */
    EFUN(wr_tx_patt_gen_seq_1(0x2800));
    EFUN(wr_pam4_precoder_en(0x0));
    EFUN(wr_pam4_gray_enc_en(0x0));
    EFUN(wr_pam4_tx_jp03b_patt_en(0x1));
    EFUN(wr_patt_gen_en(0x1));
  }
  else {
    EFUN(wr_patt_gen_en(0x0));
    EFUN(wr_pam4_tx_jp03b_patt_en(0x0));
  }
  return(ERR_CODE_NONE);
}


/* Configure TX Linearity Pattern */
err_code_t plp_aperta2_peregrine5_pc_config_tx_linearity_pattern(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE

  if (enable) {
    /* Linearity Pattern - "p0123_0303_21" => 20'b_0001_1011_0011_0011_1001 or 20'h1B339 w/o  Gray coding */
    /*                                     => 20'b_0001_1110_0010_0010_1101 or 20'h1E22D with Gray coding */
    EFUN(wr_tx_patt_gen_seq_0(0xE22D));
    EFUN(wr_tx_patt_gen_seq_1(0x0001));
    EFUN(wr_pam4_precoder_en(0x0));
    EFUN(wr_pam4_gray_enc_en(0x0));
    EFUN(wr_pam4_tx_jp03b_patt_en(0x0));
    EFUN(wr_pam4_tx_linearity_patt_en(0x1));
    EFUN(wr_patt_gen_en(0x1));
  }
  else {
    EFUN(wr_patt_gen_en(0x0));
    EFUN(wr_pam4_tx_linearity_patt_en(0x0));
  }
  return(ERR_CODE_NONE);
}

/**************************************/
/*  PRBS Generator/Checker Functions  */
/**************************************/

/* Configure PRBS Generator */
err_code_t plp_aperta2_peregrine5_pc_config_tx_prbs(srds_access_t *sa__, enum plp_aperta2_srds_prbs_polynomial_enum prbs_poly_mode, uint8_t prbs_inv) {
    INIT_SRDS_ERR_CODE
    if(prbs_poly_mode >= PCS_PRBS_7) {
      return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    EFUN(wr_prbs_gen_cy_cnt_en(0x0)); /* Disable tx_patt_gen PRBS generator mode if enabled */
    {
    EFUN(wr_prbs_gen_mode_select((uint8_t)prbs_poly_mode));  /* PRBS Generator mode sel */
    }
    EFUN(wr_prbs_gen_inv(prbs_inv));                      /* PRBS Invert Enable/Disable */
    /* To enable PRBS Generator */
    /* EFUN(wr_prbs_gen_en(0x1)); */
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_get_tx_prbs_config(srds_access_t *sa__, enum plp_aperta2_srds_prbs_polynomial_enum *prbs_poly_mode, uint8_t *prbs_inv) {
    INIT_SRDS_ERR_CODE
    uint8_t val;

    ESTM(val = rd_prbs_gen_mode_select());                /* PRBS Generator mode sel */
    *prbs_poly_mode = (enum plp_aperta2_srds_prbs_polynomial_enum)val;
    ESTM(val = rd_prbs_gen_inv());                        /* PRBS Invert Enable/Disable */
    *prbs_inv = val;
  return (ERR_CODE_NONE);
}

/* PRBS Generator Enable/Disable */
err_code_t plp_aperta2_peregrine5_pc_tx_prbs_en(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    if (enable) {
        EFUN(wr_prbs_gen_en(0x1));                          /* Enable PRBS Generator */
    }
    else {
        EFUN(wr_prbs_gen_en(0x0));                          /* Disable PRBS Generator */
    }
    return (ERR_CODE_NONE);
}
/* Get PRBS Generator Enable/Disable */
err_code_t plp_aperta2_peregrine5_pc_get_tx_prbs_en(srds_access_t *sa__, uint8_t *enable) {
    INIT_SRDS_ERR_CODE
    if(!enable) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    ESTM(*enable = rd_prbs_gen_en());
    return (ERR_CODE_NONE);
}

/* PRBS 1-bit error injection */
err_code_t plp_aperta2_peregrine5_pc_tx_prbs_err_inject(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
  /* PRBS Error Insert.
     0 to 1 transition on this signal will insert single bit error in the MSB bit of the data bus.
     Reset value is 0x0.
  */
    if(enable) {
      EFUN(wr_prbs_gen_err_ins(0x1));
    }
    EFUN(wr_prbs_gen_err_ins(0));
  return (ERR_CODE_NONE);
}

/* Configure PRBS Checker */
err_code_t plp_aperta2_peregrine5_pc_config_rx_prbs(srds_access_t *sa__, enum plp_aperta2_srds_prbs_polynomial_enum prbs_poly_mode, enum plp_aperta2_srds_prbs_checker_mode_enum prbs_checker_mode, uint8_t prbs_inv) {
    INIT_SRDS_ERR_CODE
  {
    uint8_t dig_lpbk = 0;
  if(prbs_poly_mode >= PCS_PRBS_7) {
    return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
  }
  EFUN(wr_prbs_chk_cy_cnt_en(0x0));  /* Disable tx_patt_gen PRBS checker mode if enabled */
  if (prbs_poly_mode == PRBS_AUTO_DETECT) {
    EFUN(wr_prbs_chk_auto_detect_en(0x1));                 /* Enable PRBS checker Auto-Detect */
    EFUN(wr_prbs_chk_mode(PRBS_INITIAL_SEED_HYSTERESIS));  /* PRBS Checker mode sel (PRBS LOCK state machine select) */
  }
  else if (prbs_poly_mode == USER_40_BIT_REPEAT) {
    EFUN(wr_prbs_chk_mode_select(USER_40_BIT_REPEAT));    /* PRBS Checker Polynomial mode sel to USER_40_BIT_REPEAT */
    EFUN(wr_prbs_chk_mode((uint8_t)prbs_checker_mode));  /* PRBS Checker mode sel (PRBS LOCK state machine select) */
  }
    else {
      EFUN(wr_prbs_chk_mode_select((uint8_t)prbs_poly_mode)); /* PRBS Checker Polynomial mode sel  */
      EFUN(wr_prbs_chk_mode((uint8_t)prbs_checker_mode));  /* PRBS Checker mode sel (PRBS LOCK state machine select) */
      ESTM(dig_lpbk = rd_dig_lpbk_en());
      if(dig_lpbk == 0) {
        /* Only enable auto mode in non-digital loop-back mode */
        EFUN(wr_prbs_chk_en_auto_mode(0x1));             /* PRBS Checker enable control - rx_dsc_lock & prbs_chk_en */
      }
      EFUN(wr_prbs_chk_inv(prbs_inv));                     /* PRBS Invert Enable/Disable */
      EFUN(wr_prbs_chk_auto_detect_en(0x0));               /* Disable PRBS checker Auto-Detect */
    }
  }
  /* To enable PRBS Checker */
  /* EFUN(wr_prbs_chk_en(0x1)); */
  return (ERR_CODE_NONE);
}

/* get PRBS Checker */
err_code_t plp_aperta2_peregrine5_pc_get_rx_prbs_config(srds_access_t *sa__, enum plp_aperta2_srds_prbs_polynomial_enum *prbs_poly_mode, enum plp_aperta2_srds_prbs_checker_mode_enum *prbs_checker_mode, uint8_t *prbs_inv) {
    INIT_SRDS_ERR_CODE
    uint8_t val;

    uint8_t auto_det_en;
    ESTM(auto_det_en = rd_prbs_chk_auto_detect_en());       /* PRBS checker Auto-Detect Enable */
    if (auto_det_en) {
      uint8_t auto_det_lock;
      ESTM(auto_det_lock = rd_prbs_chk_auto_detect_lock()); /* PRBS checker Auto-Detect Lock */
      if (auto_det_lock) {
        ESTM(val = rd_prbs_chk_mode_select_auto_detect());  /* PRBS Auto-Detect Checker Polynomial mode sel  */
        *prbs_poly_mode = (enum plp_aperta2_srds_prbs_polynomial_enum)val;

        ESTM(val = rd_prbs_chk_mode());                     /* PRBS Checker mode sel (PRBS LOCK state machine select) */
        *prbs_checker_mode = (enum plp_aperta2_srds_prbs_checker_mode_enum)val;
        ESTM(val = rd_prbs_chk_inv_auto_detect());          /* PRBS Invert Enable/Disable */
        *prbs_inv = val;
      }
      else {
        *prbs_poly_mode = PRBS_UNKNOWN;
        *prbs_inv = 0;
        ESTM(val = rd_prbs_chk_mode());                     /* PRBS Checker mode sel (PRBS LOCK state machine select) */
        *prbs_checker_mode = (enum plp_aperta2_srds_prbs_checker_mode_enum)val;
      }
    }
    else {
      ESTM(val = rd_prbs_chk_mode_select());              /* PRBS Checker Polynomial mode sel  */
      *prbs_poly_mode = (enum plp_aperta2_srds_prbs_polynomial_enum)val;

      ESTM(val = rd_prbs_chk_mode());                     /* PRBS Checker mode sel (PRBS LOCK state machine select) */
      *prbs_checker_mode = (enum plp_aperta2_srds_prbs_checker_mode_enum)val;
      ESTM(val = rd_prbs_chk_inv());                      /* PRBS Invert Enable/Disable */
      *prbs_inv = val;
    }
  return (ERR_CODE_NONE);
}

/* PRBS Checker Enable/Disable */
err_code_t plp_aperta2_peregrine5_pc_rx_prbs_en(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    if (enable) {
        EFUN(wr_prbs_chk_en(0x1));                          /* Enable PRBS Checker */
    }
    else {
        EFUN(wr_prbs_chk_en(0x0));                          /* Disable PRBS Checker */
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_get_rx_prbs_en(srds_access_t *sa__, uint8_t *enable) {
    INIT_SRDS_ERR_CODE
    if(!enable) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    ESTM(*enable = rd_prbs_chk_en());
    return (ERR_CODE_NONE);
}


/* PRBS Checker Lock State */
err_code_t plp_aperta2_peregrine5_pc_prbs_chk_lock_state(srds_access_t *sa__, uint8_t *chk_lock) {
    INIT_SRDS_ERR_CODE
  if(!chk_lock) {
    return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
    ESTM(*chk_lock = rd_prbs_chk_lock());                  /* PRBS Checker Lock Indication 1 = Locked, 0 = Out of Lock */
  return (ERR_CODE_NONE);
}

/* PRBS Error Count and Lock Lost (bit 31 in lock lost) */
err_code_t plp_aperta2_peregrine5_pc_prbs_err_count_ll(srds_access_t *sa__, uint32_t *prbs_err_cnt) {
    INIT_SRDS_ERR_CODE
  uint16_t rddata;

  if(!prbs_err_cnt) {
    return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
    ESTM(rddata = reg_rd_TLB_RX_PRBS_CHK_ERR_CNT_MSB_STATUS());
    *prbs_err_cnt = ((uint32_t) rddata)<<16;
    ESTM(*prbs_err_cnt = (*prbs_err_cnt | rd_prbs_chk_err_cnt_lsb()));
  return (ERR_CODE_NONE);
}

/* PRBS Error Count State  */
err_code_t plp_aperta2_peregrine5_pc_prbs_err_count_state(srds_access_t *sa__, uint32_t *prbs_err_cnt, uint8_t *lock_lost) {
    INIT_SRDS_ERR_CODE
  uint8_t dig_lpbk_enable = 0;
  uint8_t link_training_enable = 0;
  uint8_t hw_timer_mode = 0;
  if(!prbs_err_cnt || !lock_lost) {
    return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
    EFUN(plp_aperta2_peregrine5_pc_prbs_err_count_ll(sa__, prbs_err_cnt));
    ESTM(hw_timer_mode = rd_prbs_chk_en_timer_unit_sel());
    if(hw_timer_mode != 0) {
      *lock_lost = 0;
    } else
    {
      *lock_lost = (uint8_t)(*prbs_err_cnt >> 31);
    }
    *prbs_err_cnt = (*prbs_err_cnt & 0x7FFFFFFF);

    /* Check if Digital Loopback and LinkTrainig both are enabled */
    ESTM(dig_lpbk_enable = rd_dig_lpbk_en());
    ESTM(link_training_enable = rd_linktrn_ieee_training_enable());

    if(dig_lpbk_enable && link_training_enable) {
      EFUN_PRINTF(("WARNING: PRBS Check Lock - Digital Loopback and Link Training both are enabled\n"));
    }
  return (ERR_CODE_NONE);
}

/* Save prbs hardware timer config registers */
err_code_t plp_aperta2_peregrine5_pc_get_prbs_chk_hw_timer_ctrl(srds_access_t *sa__, struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st * const prbs_chk_hw_timer_ctrl_bak) {
    INIT_SRDS_ERR_CODE
    ESTM(prbs_chk_hw_timer_ctrl_bak->prbs_chk_burst_err_cnt_en = rd_prbs_chk_burst_err_cnt_en());
    ESTM(prbs_chk_hw_timer_ctrl_bak->prbs_chk_en_timer_unit_sel = rd_prbs_chk_en_timer_unit_sel());
    ESTM(prbs_chk_hw_timer_ctrl_bak->prbs_chk_en_timer_unit_timeout = rd_prbs_chk_en_timer_unit_timeout());
    ESTM(prbs_chk_hw_timer_ctrl_bak->prbs_chk_new_timer_mode = rd_prbs_chk_new_timer_mode());
    return ERR_CODE_NONE;
}

/* Restore prbs hardware timer config registers */
err_code_t plp_aperta2_peregrine5_pc_set_prbs_chk_hw_timer_ctrl(srds_access_t *sa__, struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st const * const prbs_chk_hw_timer_ctrl_bak) {
    INIT_SRDS_ERR_CODE
    EFUN(wr_prbs_chk_burst_err_cnt_en(prbs_chk_hw_timer_ctrl_bak->prbs_chk_burst_err_cnt_en));
    EFUN(wr_prbs_chk_en_timer_unit_sel(prbs_chk_hw_timer_ctrl_bak->prbs_chk_en_timer_unit_sel));
    EFUN(wr_prbs_chk_en_timer_unit_timeout(prbs_chk_hw_timer_ctrl_bak->prbs_chk_en_timer_unit_timeout));
    EFUN(wr_prbs_chk_new_timer_mode(prbs_chk_hw_timer_ctrl_bak->prbs_chk_new_timer_mode));
    return ERR_CODE_NONE;
}

/* Configure hardware timer to count prbs errors */
err_code_t plp_aperta2_peregrine5_pc_config_prbs_chk_hw_timer(srds_access_t *sa__, uint16_t time_ms, uint16_t *time_ms_adjusted) {
    INIT_SRDS_ERR_CODE
    struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st prbs_chk_hw_timer_ctrl;
    ENULL_MEMSET(&prbs_chk_hw_timer_ctrl, 0, sizeof(struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st));
    EFUN(wr_prbs_chk_burst_err_cnt_en(0));

    /* Figure out timer mode and timer count */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_prbs_timeout_count_from_time(time_ms, time_ms_adjusted, &prbs_chk_hw_timer_ctrl));
    EFUN(wr_prbs_chk_en_timer_unit_sel(prbs_chk_hw_timer_ctrl.prbs_chk_en_timer_unit_sel));
    EFUN(wr_prbs_chk_en_timer_unit_timeout(prbs_chk_hw_timer_ctrl.prbs_chk_en_timer_unit_timeout));
    EFUN(wr_prbs_chk_new_timer_mode(1));
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_BER_setup_measurement(srds_access_t *sa__, uint16_t time_ms, struct peregrine5_pc_ber_data_st *ber_data, struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st *prbs_chk_hw_timer_ctrl) { 
    uint8_t lcklost = 0;
    uint32_t err_cnt= 0;
    uint8_t clk_gate = 0;
    uint8_t prbs_chk_lock_state = 0;

    INIT_SRDS_ERR_CODE

    /* Check arguments */
    if ((ber_data == NULL) || (prbs_chk_hw_timer_ctrl == NULL)) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    /* clear BER data before capture */
    /*ber_data->num_errs       = 0;*/
    /*ber_data->num_bits       = 0;*/
    COMPILER_64_SET(ber_data->num_errs, 0, 0);
    COMPILER_64_SET(ber_data->num_bits, 0, 0);
    ber_data->lcklost        = 0;
    ber_data->prbs_chk_en    = 0;
    ber_data->cdrlcklost     = 0;
    ber_data->prbs_lck_state = 0;

    /* Test for PRBS error checking availablity */
    ESTM(ber_data->prbs_chk_en = rd_prbs_chk_en());
    ESTM(clk_gate = rd_ln_rx_s_clkgate_frc_on());
    if(ber_data->prbs_chk_en == 0) {
        return(ERR_CODE_PRBS_CHK_DISABLED);
    }
    else if (clk_gate == 1) {
        return (ERR_CODE_RX_CLKGATE_FRC_ON);
    }

    ESTM(prbs_chk_lock_state = rd_prbs_chk_lock_state()); /* Do a preemptive read to flush out lock state history */
    UNUSED(prbs_chk_lock_state);
    ENULL_MEMSET(prbs_chk_hw_timer_ctrl, 0, sizeof(struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st));

    /* Check if CDR lost lock */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_prbs_chk_cdr_lock_lost(sa__, &ber_data->cdrlcklost));

    /* save prbs hardware timer config registers and prbs mode */
    EFUN(plp_aperta2_peregrine5_pc_get_prbs_chk_hw_timer_ctrl(sa__, prbs_chk_hw_timer_ctrl));

    /* Configure hardware timer to count prbs errors */
    EFUN(plp_aperta2_peregrine5_pc_config_prbs_chk_hw_timer(sa__, time_ms, &prbs_chk_hw_timer_ctrl->time_ms_adjusted_bak));
    EFUN(plp_aperta2_peregrine5_pc_prbs_err_count_state(sa__, &err_cnt, &lcklost)); /* Clear prbs error counters and start timer */
    /* Extra delay to account for the synchronization delay in prbs checker */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_hw_timer_sync_delay(sa__));

    return(ERR_CODE_NONE);
} /* plp_aperta2_peregrine5_pc_BER_setup_measurement() */

err_code_t plp_aperta2_peregrine5_pc_BER_poll_measurement(srds_access_t *sa__, struct peregrine5_pc_ber_data_st *ber_data, struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st *prbs_chk_hw_timer_ctrl) { 
    const uint16_t ITER_MAX=10;
    uint16_t iter=0 ;
    uint8_t prbs_chk_timeout = 0;
    uint8_t prbs_chk_lock_state = 0;
    INIT_SRDS_ERR_CODE

    /* Check arguments */
    if (ber_data == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    else if(ber_data->prbs_chk_en == 0) {
        return(ERR_CODE_PRBS_CHK_DISABLED);
    }

    /* Confirm that prbs lock was achieved */
    ESTM(prbs_chk_lock_state = rd_prbs_chk_lock_state());
    if (prbs_chk_lock_state == PRBS_CHECKER_NOT_ENABLED) {
        /* If prbs lock was not achieved, set lcklost = 1 and skip the wait for error accumulation */
        ber_data->lcklost = 1;
    }
    /* Poll prbs_chk_lock until done or timeout has expired */
    else {
        do {
            ESTM(prbs_chk_timeout = rd_prbs_chk_timer_done_lh());
            /* Once PRBS timer expires, break out */
            if (prbs_chk_timeout) {
                break;
            }

            /* loop delay */
            EFUN(USR_DELAY_MS(1));

            /* Increment safety exit check */ 
            iter++;
        } while (iter < (ITER_MAX + (prbs_chk_hw_timer_ctrl->time_ms_adjusted_bak)));

        /* If PRBS timer did not expire, or it expired and lock didn't go away */
        if(!prbs_chk_timeout) {
            ber_data->lcklost = 1;  /* set based on timeout */
            return ERR_CODE_PRBS_CHK_HW_TIMERS_NOT_EXPIRED;
        }

        ESTM(ber_data->prbs_lck_state = rd_prbs_chk_lock_state());
    }

    return(ERR_CODE_NONE);

} /* BER poll measurement */

err_code_t plp_aperta2_peregrine5_pc_BER_get_measurement(srds_access_t *sa__, struct peregrine5_pc_ber_data_st *ber_data, struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st *prbs_chk_hw_timer_ctrl) { 
    uint8_t  lcklost = 0;
    uint16_t time_ms = 0, unit_sel = 0, unit_count = 0;
    uint32_t err_cnt = 0;
    uint32_t num_bits_per_ms = 0;
    uint8_t lock_state = 0;
    INIT_SRDS_ERR_CODE

    /* Check arguments */
    if (ber_data == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    ESTM(lock_state = rd_prbs_chk_lock_state());

    /* Capture and Post process data provided lock was maintained */
    if (lock_state != PRBS_CHECKER_LOCKED)
    {
            ber_data->lcklost = 1;
    }
    else
    {
            /* Read error counters */
            EFUN(plp_aperta2_peregrine5_pc_prbs_err_count_state(sa__, &err_cnt, &lcklost));

            ber_data->lcklost = lcklost;
            
            
            /* Update the BER data structure, bits/mS * mS run time */
            EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_num_bits_per_ms(sa__, 1, &num_bits_per_ms));

            /* For Snapshot usage, get data out of registers, not the data structures  */
            ESTM(unit_sel = rd_prbs_chk_en_timer_unit_sel());
            ESTM(unit_count = rd_prbs_chk_en_timer_unit_timeout());

            /* Calculate absolute mS time */
            if (unit_sel == PRBS_CHK_EN_TIMER_UNIT_SEL_1MS) {
                time_ms = unit_count;
            }     
            else if (unit_sel == PRBS_CHK_EN_TIMER_UNIT_SEL_10MS) {
                time_ms = (uint16_t)(unit_count * 10);
            }     
            else if (unit_sel == PRBS_CHK_EN_TIMER_UNIT_SEL_100MS) {
                time_ms = (uint16_t)(unit_count * 100);
            }     
            else {
                EFUN_PRINTF(("ERROR: PRBS timer UNIT SEL read is out of range.\n"));
                return(ERR_CODE_INVALID_VALUE);
            }     
            /*
             * ber_data->num_bits = (uint64_t)num_bits_per_ms * (uint64_t)time_ms;
             * ber_data->num_errs = (uint64_t)err_cnt;
             */
            COMPILER_64_SET(ber_data->num_bits, 0, num_bits_per_ms);
            COMPILER_64_UMUL_32(ber_data->num_bits, time_ms);
            COMPILER_64_SET(ber_data->num_errs, 0, err_cnt);
    }

    /* restore prbs hardware timer config registers */   
    EFUN(plp_aperta2_peregrine5_pc_set_prbs_chk_hw_timer_ctrl(sa__, prbs_chk_hw_timer_ctrl));

    return(ERR_CODE_NONE);
} /* plp_aperta2_peregrine5_pc_BER_get_measurement() */

err_code_t plp_aperta2_peregrine5_pc_display_detailed_prbs_state_hdr(srds_access_t *sa__) {
    EFUN_PRINTF(("\nPRBS DETAILED DISPLAY :\n"));

/* Comment out display of PRBS burst error if !MERLIN7 */
    EFUN_PRINTF((" LN TX-Mode TX-PRBS-Inv TX-PMD-Inv RX-Mode RX-PRBS-Inv RX-PMD-Inv Lck LL PRBS-Err-Cnt"));
    EFUN_PRINTF(("     BER\n"));
    return (ERR_CODE_NONE);
}

const char* plp_aperta2_peregrine5_pc_get_e2s_prbs_mode_enum(enum plp_aperta2_srds_prbs_polynomial_enum prbs_poly_mode) {
    const char* peregrine5_pc_e2s_prbs_mode_enum[22] = {
    " PRBS_7",
    " PRBS_9",
    "PRBS_11",
    "PRBS_15",
    "PRBS_23",
    "PRBS_31",
    "PRBS_58",
    "PRBS_49",
    "PRBS_10",
    "PRBS_20",
    "PRBS_13",
    "  ERR  ",
    "  ERR  ",
    "  ERR  ",
    "  ERR  ",
    "USER_40",
    "  ERR  ",
    "  ERR  ",
    "  ERR  ",
    "  ERR  ",
    "AUTO_DT",
    "UNKNOWN"
    };
    if(prbs_poly_mode >= sizeof(peregrine5_pc_e2s_prbs_mode_enum)/sizeof(peregrine5_pc_e2s_prbs_mode_enum[0])) {
        return "UNKNOWN";
    }
    return peregrine5_pc_e2s_prbs_mode_enum[prbs_poly_mode];
}

err_code_t plp_aperta2_peregrine5_pc_display_all_detailed_prbs_state(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    uint8_t lane = 0, num_lanes = 0;
    uint8_t lane_orig = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);

    ESTM(num_lanes = rdc_revid_multiplicity());
    EFUN(plp_aperta2_peregrine5_pc_display_detailed_prbs_state_hdr(sa__));
    for(lane = 0; lane < num_lanes; lane++) {
        EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane));
        EFUN(plp_aperta2_peregrine5_pc_display_detailed_prbs_state(sa__));
    }
    EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane_orig));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_display_detailed_prbs_state(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    uint32_t err_cnt = 0;
    uint8_t lock_lost = 0;
    uint8_t enabled;
    char BER[SRDS_MAX_BER_STR_LEN];

    ESTM_PRINTF(("  %d ",plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));

    ESTM(enabled = rd_prbs_gen_en());
    if(enabled) {
        enum plp_aperta2_srds_prbs_polynomial_enum prbs_poly_mode = PRBS_7;
        uint8_t prbs_inv = 0;
        const char *prbs_string;
        EFUN(plp_aperta2_peregrine5_pc_get_tx_prbs_config(sa__, &prbs_poly_mode, &prbs_inv));
        if(prbs_poly_mode == PRBS_UNKNOWN) {
            prbs_string = "UNKNOWN";
        }
        else {
            prbs_string =  plp_aperta2_peregrine5_pc_get_e2s_prbs_mode_enum(prbs_poly_mode);
        }
        ESTM_PRINTF(("%s",prbs_string));
        ESTM_PRINTF(("      %1d     ",prbs_inv));
    } else {
        EFUN_PRINTF(("  OFF  "));
        ESTM_PRINTF(("      -     "));
    }
    ESTM_PRINTF(("     %1d     ",rd_tx_pmd_dp_invert()));

    ESTM(enabled = rd_prbs_chk_en());
    if(enabled) {
        enum plp_aperta2_srds_prbs_polynomial_enum prbs_poly_mode = PRBS_7;
        enum plp_aperta2_srds_prbs_checker_mode_enum prbs_checker_mode;
        uint8_t prbs_inv = 0;
        const char *prbs_string;
        EFUN(plp_aperta2_peregrine5_pc_get_rx_prbs_config(sa__, &prbs_poly_mode, &prbs_checker_mode, &prbs_inv));
        if(prbs_poly_mode == PRBS_UNKNOWN) {
            prbs_string = "UNKNOWN";
        }
        else {
            prbs_string =  plp_aperta2_peregrine5_pc_get_e2s_prbs_mode_enum(prbs_poly_mode);
        }
        ESTM_PRINTF((" %s",prbs_string));
        ESTM_PRINTF(("      %1d     ",prbs_inv));
    } else {
        EFUN_PRINTF(("   OFF  "));
        ESTM_PRINTF(("      -     "));
    }

    ESTM_PRINTF(("     %1d     ",rd_rx_pmd_dp_invert()));
    ESTM_PRINTF(("  %d ",rd_prbs_chk_lock()));
    {
        struct peregrine5_pc_ber_data_st ber_data = BER_DATA_ST_INIT;
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_BER_data_and_string(sa__, SRDS_BER_MEASURE_TIME_MS, &ber_data, BER, sizeof(BER)));
        err_cnt = (uint32_t)ber_data.num_errs;
        lock_lost = ber_data.lcklost;
    }
    EFUN_PRINTF(("  %d  %010u ",lock_lost,err_cnt));
    USR_PRINTF(("%s",BER));
    EFUN_PRINTF(("\n"));

    return (ERR_CODE_NONE);
}


/***********************************/
/*  PRBS Error Analyzer Functions  */
/***********************************/
const char* plp_aperta2_peregrine5_pc_get_e2s_prbs_aggregate_mode_enum(enum peregrine5_pc_prbs_error_analyzer_aggregate_mode_enum prbs_aggregate_mode) {
    const char* peregrine5_pc_e2s_prbs_aggregate_mode_enum[ ] = {
        "PEREGRINE5_PC_FEC_100GE       ",
        "PEREGRINE5_PC_FEC_200GE       ",
        "PEREGRINE5_PC_FEC_CUSTOM      "
    };

    if(prbs_aggregate_mode >= sizeof(peregrine5_pc_e2s_prbs_aggregate_mode_enum)/sizeof(peregrine5_pc_e2s_prbs_aggregate_mode_enum[0])) {
         return "PEREGRINE5_PC_UNKNOWN              ";
    }

    return peregrine5_pc_e2s_prbs_aggregate_mode_enum[prbs_aggregate_mode];
}
/* Reset PRBS Error Analyzer counters all lanes including aggregate lane */

err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_reset(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_common_config_st *err_analyzer_common_config) {
    INIT_SRDS_ERR_CODE

    if(!err_analyzer_common_config) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    EFUN(wrc_tlb_err_analyze_en(0x0)); 
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_clear_error_analyzer_dac_pattern_memory(sa__));  

    return (ERR_CODE_NONE);
}

/* Configure PRBS Error Analyzer common */
err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_common_config(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_common_config_st *err_analyzer) {
    INIT_SRDS_ERR_CODE

    if(!err_analyzer) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    EFUN(wrc_tlb_err_aggr_start_error_aggregation(0x0));
    switch (err_analyzer->prbs_err_aggregate_mode) {
        case PEREGRINE5_PC_FEC_100GE:              
           err_analyzer->num_of_aggregate_lanes = 1;
           break;
        case PEREGRINE5_PC_FEC_200GE:              
           err_analyzer->num_of_aggregate_lanes = 2;
           break;
        case PEREGRINE5_PC_FEC_CUSTOM:
           break;
        default:
           EFUN_PRINTF(("\n << WARNING: prbs_err_aggregate_mode needs to be a valid number\n"));
           return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    /* select 35bits counter saturation */
    EFUN(wrc_tlb_err_analyze_cnt_sat(0x1));

    /* set compressed format */
    if (err_analyzer->use_compressed_format) {
        EFUN(wrc_tlb_err_analyze_rd_float(0x1));
    }
    else {
        EFUN(wrc_tlb_err_analyze_rd_float(0x0));
    }

    /* counters in memory get cleared when registers are updated from it */
    if (err_analyzer->cnt_clear_on_update) {
        EFUN(wrc_tlb_err_analyze_cnt_clr(0x1));
    }
    else {
        EFUN(wrc_tlb_err_analyze_cnt_clr(0x0));
    }

    /* Enable all error counters */
    EFUN(wrc_tlb_err_analyze_counter_en(0xffff)); 

    /* Enable counters >15 errors */
    EFUN(wrc_tlb_err_analyze_counter_more_than_15_en(0x1)); 

    EFUN(wrc_tlb_err_analyze_en(0x1)); 

    return (ERR_CODE_NONE);
}

/* Configure PRBS Error Analyzer on the lane */

err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_lane_config(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_lane_config) {
    INIT_SRDS_ERR_CODE
    uint8_t  fec_size = 0;
    uint8_t  sizep5 = 0;
    uint8_t  msb_lsb_group = 1;
    uint8_t  msb_en = 1;
    uint8_t  lsb_en = 1;
    uint8_t  l_en = 1;
    uint8_t  r_en = 1;
    uint8_t  swap_en = 1;
    uint8_t  fec_4_1_bit_mux_en = 1;
    uint8_t  fec_4cw_ilv_en = 1;
    uint8_t  fec_4cw_bit = 1;
    uint16_t fec_size_bits = 0;

    peregrine5_pc_prbs_err_analyzer_common_config_st *err_analyzer_common_config = NULL;

    if(!err_analyzer_lane_config) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    err_analyzer_common_config = err_analyzer_lane_config->common_config_ptr;
    if(!err_analyzer_common_config) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    /* validate the fec frame size, throw an error and exit if not compliant */
    if ((err_analyzer_lane_config->prbs_err_fec_size < PEREGRINE5_PC_PRBS_ERR_ANALYZER_FEC_SIZE_MIN) ||
        (err_analyzer_lane_config->prbs_err_fec_size > PEREGRINE5_PC_PRBS_ERR_ANALYZER_FEC_SIZE_MAX)) {
        return (peregrine5_pc_error(sa__, ERR_CODE_INVALID_PRBS_ERR_ANALYZER_FEC_SIZE));
    }

    /* program fec frame size, return with error if size not compliant */
    fec_size = (uint8_t)(err_analyzer_lane_config->prbs_err_fec_size / PEREGRINE5_PC_PRBS_FEC_FRAME_SIZE_MULTIPLIER);

    fec_size_bits = (uint16_t)(fec_size * PEREGRINE5_PC_PRBS_FEC_FRAME_SIZE_MULTIPLIER);
    
    /* validate fec frame size */
    if (err_analyzer_lane_config->prbs_err_fec_size != fec_size_bits) {
        if (err_analyzer_lane_config->prbs_err_fec_size - fec_size_bits == PEREGRINE5_PC_PRBS_FEC_FRAME_SIZE_MULTIPLIER/2) {
            sizep5 = 1;
            EFUN(wr_tlb_err_fec_sizep5(sizep5));
        }
        else {
            EFUN_PRINTF(("\n << ERROR: FEC Frame size of %d bits NOT programmable. >>\n", err_analyzer_lane_config->prbs_err_fec_size));
            return (peregrine5_pc_error(sa__, ERR_CODE_INVALID_PRBS_ERR_ANALYZER_FEC_SIZE));
        }
    }
    else {
        EFUN(wr_tlb_err_fec_sizep5(sizep5));
    }

    /* program fec_size */
    EFUN(wr_tlb_err_fec_size(fec_size));

    /* Stop Error Analyzer counters */
    EFUN(wr_tlb_err_start_error_analyzer(0x0));

    /* enable storing error counters from this lane  */
    EFUN(wr_tlb_err_analyze_lanes_active(0x1)); 


    switch (err_analyzer_common_config->prbs_err_aggregate_mode) {
        case PEREGRINE5_PC_FEC_100GE:              /* 100G w/ Bit Muxing 4:1 and no Code Word Interleaving 100GE 112G Peregrine */
           err_analyzer_lane_config->fec_size_frac = PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_8;
           swap_en = 0x0;
           fec_4cw_ilv_en = 0x0;
           break;
        case PEREGRINE5_PC_FEC_200GE:
           err_analyzer_lane_config->fec_size_frac = PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_8;
           fec_size = fec_size * 2;
           if (sizep5) {
               fec_size = fec_size + 1;
               EFUN(wr_tlb_err_fec_sizep5(0));
           }
           EFUN(wr_tlb_err_fec_size(fec_size));
           lsb_en = 0x0;
           fec_4cw_ilv_en = 0x0;
           break;

        case PEREGRINE5_PC_FEC_CUSTOM:
           break;
        default:
           EFUN_PRINTF(("\n << WARNING: prbs_err_aggregate_mode needs to be a valid number\n"));
           return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    /* Set fraction to appropriate value */
    EFUN(wr_tlb_err_fec_size_frac(err_analyzer_lane_config->fec_size_frac));    
    EFUN(wr_tlb_err_fec_4_1_bit_mux_en(fec_4_1_bit_mux_en));
    EFUN(wr_tlb_err_fec_4cw_ilv_en(fec_4cw_ilv_en));
    EFUN(wr_tlb_err_fec_4cw_bit(fec_4cw_bit));
    EFUN(wr_tlb_err_symbol_msb_lsb_group(msb_lsb_group));
    EFUN(wr_tlb_err_fec_msb_en(msb_en));
    EFUN(wr_tlb_err_fec_lsb_en(lsb_en));
    EFUN(wr_tlb_err_fec_l_en(l_en));
    EFUN(wr_tlb_err_fec_r_en(r_en));
    EFUN(wr_tlb_err_fec_rscode_swap_en(swap_en));


    return (ERR_CODE_NONE);
}


/* Start single lane PRBS Error Analyzer */
err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_start(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer) {
    INIT_SRDS_ERR_CODE
    uint32_t prbs_bit_errcnt = 0;
    uint8_t lcklost = 0;

    if(!err_analyzer) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    /* Start Error Analyzer per lane and clear internal counters per lane */
    EFUN(wr_tlb_err_start_error_analyzer(0x1));
    /* Reset raw PRBS error counters and start counting */
    EFUN(plp_aperta2_peregrine5_pc_prbs_err_count_state(sa__, &prbs_bit_errcnt, &lcklost));
    return (ERR_CODE_NONE);
}

/* Stop single lane PRBS Error Analyzer */
err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_stop(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer) {
    INIT_SRDS_ERR_CODE

    if(!err_analyzer) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    /* Stop Error Analyzer per lane and clear internal counters per lane */
    EFUN(wr_tlb_err_start_error_analyzer(0));
    return (ERR_CODE_NONE);
}


/* Get PRBS Error Analyzer Config */

err_code_t plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_config(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer) {
    INIT_SRDS_ERR_CODE

    uint16_t prbs_err_fec_size;
    uint8_t  prbs_err_fec_size_frac;

    if(!err_analyzer) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    ESTM(prbs_err_fec_size  = rd_tlb_err_fec_size());
    prbs_err_fec_size = prbs_err_fec_size * PEREGRINE5_PC_PRBS_FEC_FRAME_SIZE_MULTIPLIER;
    {
        uint8_t  sizep5 = 0;
        ESTM(sizep5 = rd_tlb_err_fec_sizep5()); 
        if ( sizep5 ) {
            prbs_err_fec_size  += PEREGRINE5_PC_PRBS_FEC_FRAME_SIZE_MULTIPLIER/2;
        }
    }
    err_analyzer->prbs_err_fec_size = prbs_err_fec_size;
    ESTM(prbs_err_fec_size_frac  = rd_tlb_err_fec_size_frac());
    err_analyzer->fec_size_frac = prbs_err_fec_size_frac;
    return (ERR_CODE_NONE);
}


/* Display PRBS Error Analyzer Config */

err_code_t plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_config(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer) {
    uint32_t timeout_s;
    peregrine5_pc_prbs_err_analyzer_common_config_st *err_analyzer_common;

    if (!err_analyzer) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    err_analyzer_common = err_analyzer->common_config_ptr;
    if (!err_analyzer_common) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    timeout_s = err_analyzer->timeout_s;

    if ( ! err_analyzer_common->encrypt_mode ) {
        EFUN_PRINTF(("\n ****************************************************************************** \n"));
        EFUN_PRINTF(("  PRBS Error Analyzer Config:\n"));
        EFUN_PRINTF((" ------------------------------------------------------------- \n"));
        EFUN_PRINTF(("    FEC Mode                     = %s\n",plp_aperta2_peregrine5_pc_get_e2s_prbs_aggregate_mode_enum(err_analyzer_common->prbs_err_aggregate_mode)));
        EFUN_PRINTF(("    FEC Frame Size               = %4d bits\n", err_analyzer->prbs_err_fec_size));
        EFUN_PRINTF(("    Time Duration of Analysis    = %4d seconds\n", timeout_s));
        EFUN_PRINTF((" -------------------------------------------------------------\n"));
    }

    return (ERR_CODE_NONE);
}




static err_code_t _plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_prbs_chk_lock(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer, uint8_t *prbs_chk_lock)  {
    INIT_SRDS_ERR_CODE
    uint8_t i;

    ESTM(*prbs_chk_lock = rd_prbs_chk_lock_state());

    if (*prbs_chk_lock != PRBS_CHECKER_LOCKED) {
        EFUN_PRINTF(("\nERROR : PRBS Checker is not locked for Lane %d\n", plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));

        for (i=0; i<PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
            err_analyzer->prbs_errcnt[i] = 0xFFFFFFFF;
        }
        err_analyzer->prbs_frames_all = 0xFFFFFFFF;
#ifdef SERDES_API_FLOATING_POINT
        err_analyzer->prbs_bit_errcnt = 0xFFFFFFFFFFFFF;
#endif
    }

    return (ERR_CODE_NONE);
}


/* Get PRBS Error Analyzer Error Counts from aggregate lane or lane n, where n can be 0..num_of_lanes */
/* (eg. for 8 lanes, you can do 0..8 where lanes 0..7 are lane-specific and lane 8 gives aggregate err count) */


/* Get PRBS Error Analyzer Lane Error Counts */
err_code_t plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_err_count(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config,
        peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status) {
    INIT_SRDS_ERR_CODE

    uint8_t  lane;
    uint8_t  prbs_chk_lock_state;

    if(!err_analyzer_config) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if(!err_analyzer_status) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    /* Need these runtime info for post processing */
    ESTM(err_analyzer_status->pp_field2 = plp_aperta2_peregrine5_pc_acc_get_lane(sa__));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_num_bits_per_ms(sa__, 1, &err_analyzer_status->pp_field1));

    EFUN(_plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_prbs_chk_lock(sa__, err_analyzer_status, &prbs_chk_lock_state));
    if (prbs_chk_lock_state != PRBS_CHECKER_LOCKED) {
        return (ERR_CODE_NONE);
    }

    ESTM(lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__));

    ESTM(err_analyzer_status->fec_size_frac_programmed = rd_tlb_err_fec_size_frac());

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_prbs_error_analyzer_lane_err_count(sa__, err_analyzer_status, lane));

    return (ERR_CODE_NONE);
}

/* Get PRBS Error Analyzer Aggregate Error Counts */
err_code_t plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_aggregate_err_count(srds_access_t *sa__,  peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config,
        peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status) {
    INIT_SRDS_ERR_CODE

    uint8_t num_lanes, lane;
    uint8_t  prbs_chk_lock_state;
    peregrine5_pc_prbs_err_analyzer_common_config_st * common_config;

    if(!err_analyzer_config) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if(!err_analyzer_status) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    common_config = err_analyzer_config->common_config_ptr;
    if(!common_config) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    /* read the number of lanes from register */
    ESTM(num_lanes = rdc_revid_multiplicity());

    for(lane = 0;lane<num_lanes;lane++) {
        if ((common_config->lane_mask_aggregate >> lane) & 0x1) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__,lane));
            
            EFUN(_plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_prbs_chk_lock(sa__, err_analyzer_status, &prbs_chk_lock_state));
            if (prbs_chk_lock_state != PRBS_CHECKER_LOCKED) {
                return (ERR_CODE_NONE);
            }
        }
    }

    /* Stop Error Analyzer aggregation */
    EFUN(wrc_tlb_err_aggr_start_error_aggregation(0x0));

    /* For aggregate counters, just pass the num_lanes to point to the correct memory location in the DAC */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_prbs_error_analyzer_lane_err_count(sa__, err_analyzer_status, num_lanes)); 

    return (ERR_CODE_NONE);
}


/* PRBS Error Analyzer Display */

err_code_t plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_err_count(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status,
        uint8_t encrypt_mode) {
    uint8_t i;

    if (!err_analyzer_status) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (encrypt_mode) {
        for (i=0; i<(PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1); i++) {
            EFUN_PRINTF(("%08x,", err_analyzer_status->prbs_errcnt[i]));
        }
        EFUN_PRINTF(("%08x\n", err_analyzer_status->prbs_errcnt[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1]));
        EFUN_PRINTF(("%08x\n", err_analyzer_status->prbs_frames_all));
#if defined(SERDES_API_FLOATING_POINT)
        EFUN_PRINTF(("%f\n", err_analyzer_status->prbs_bit_errcnt));
#endif
    }
    else {
        /* Display PRBS Error Analyzer Err_Counts */
        EFUN_PRINTF(("\n -------------------------------------------------------------\n"));
        EFUN_PRINTF(("  PRBS Error Analyzer Error_Counts for Lane %d:\n", err_analyzer_status->pp_field2));
        EFUN_PRINTF((" -------------------------------------------------------------\n"));
        for (i=0; i<PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
            if (err_analyzer_status->prbs_errcnt[i] == 0xFFFFFFFF) {
                EFUN_PRINTF(("    (%d,%d) FEC Frames with > %2d Errors (t=%2d) =        MAX \n", 
                            plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__), i, i));
            }
            else {
                EFUN_PRINTF(("    (%d,%d) FEC Frames with > %2d Errors (t=%2d) = %10u \n", 
                            plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__), i, i, err_analyzer_status->prbs_errcnt[i]));
            }
        }
        if (PRBS_VERBOSE > 2) {
            if (err_analyzer_status->prbs_frames_all == 0xFFFFFFFF) {
                EFUN_PRINTF(("                      (%d,%d) Total FEC Frames = MAX\n",
                            plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));
            }
            else {
                EFUN_PRINTF(("                      (%d,%d) Total FEC Frames = %10u\n",
                            plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__),err_analyzer_status->prbs_frames_all));
            }
        }
        EFUN_PRINTF((" -------------------------------------------------------------\n"));
    }

    return (ERR_CODE_NONE);
}

/* PRBS Error Analyzer Aggregate error count Display */

err_code_t plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_aggregate_err_count(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status, 
        uint8_t encrypt_mode) {
    uint8_t i;

    if (!err_analyzer_status) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    /* Display PRBS Error Analyzer Aggregate Err_Counts */
    if (encrypt_mode) {
        for (i=0; i<(PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1); i++) {
            EFUN_PRINTF(("%08x,", err_analyzer_status->prbs_errcnt[i]));
        }
        EFUN_PRINTF(("%08x\n", err_analyzer_status->prbs_errcnt[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1]));
        EFUN_PRINTF(("%08x\n", err_analyzer_status->prbs_frames_all));
    }
    else {
        EFUN_PRINTF(("\n -------------------------------------------------------------\n"));
        EFUN_PRINTF(("  PRBS Error Analyzer Aggregate Error_Counts\n"));
        EFUN_PRINTF((" -------------------------------------------------------------\n"));

        for (i=0; i<PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
            if (err_analyzer_status->prbs_errcnt[i] == 0xFFFFFFFF) {
                EFUN_PRINTF(("    FEC Frames with > %2d Errors (t=%2d) =        MAX \n", i, i));
            }
            else {
                EFUN_PRINTF(("    FEC Frames with > %2d Errors (t=%2d) = %10u \n", i, i, err_analyzer_status->prbs_errcnt[i]));
            }
        }
        EFUN_PRINTF((" -------------------------------------------------------------\n"));
    }

    return (ERR_CODE_NONE);
}




/* Display PRBS Error Analyzer Config and Errors for diagnostic dump*/

err_code_t plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_struct(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config,
        peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status) {
    INIT_SRDS_ERR_CODE
    peregrine5_pc_prbs_err_analyzer_common_config_st *err_analyzer_common;
    uint32_t timeout_s;

    if(!err_analyzer_config) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if(!err_analyzer_status) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    err_analyzer_common = err_analyzer_config->common_config_ptr;
    if(!err_analyzer_common) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    timeout_s = err_analyzer_config->timeout_s;
    if (err_analyzer_common->encrypt_mode)
    {
        /* Start of data dump */
        EFUN_PRINTF(("SERDES PRBS ERR ANALYZER DATA DUMP START\n"));
        EFUN_PRINTF(("%u %hu %hhu %u %hhu %hhu %hhu %08x %08x\n",   timeout_s, err_analyzer_config->prbs_err_fec_size,
                    err_analyzer_config->fec_size_frac, err_analyzer_common->prbs_err_aggregate_mode, 
                    err_analyzer_common->lane_mask_aggregate, err_analyzer_common->encrypt_mode, plp_aperta2_peregrine5_pc_acc_get_lane(sa__),
                    err_analyzer_status->pp_field2, err_analyzer_status->pp_field1));
    }

    /* Display PRBS Error Analyzer Config */
    EFUN(plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_config(sa__, err_analyzer_config));


    /* Display PRBS Error Analyzer Err_Counts */
    EFUN(plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_err_count(sa__, err_analyzer_status, err_analyzer_common->encrypt_mode));

    if (err_analyzer_common->encrypt_mode)
    {
        /* End of data dump */
        EFUN_PRINTF(("SERDES PRBS ERR ANALYZER DATA DUMP END\n"));
    }

    return (ERR_CODE_NONE);
}

/* Configure PRBS Error Analyzer for aggregate mode */

err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_aggregate_config(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_lane_config) {
    INIT_SRDS_ERR_CODE
    uint8_t num_lanes, lane;
    peregrine5_pc_prbs_err_analyzer_common_config_st *err_analyzer_common_config;

    if(!err_analyzer_lane_config) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    err_analyzer_common_config = err_analyzer_lane_config->common_config_ptr;
    if (!err_analyzer_common_config) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }


    /* Configure PRBS Error Analyzer common part */
    EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_common_config(sa__, err_analyzer_common_config));  

    /* read the number of lanes from register */
    ESTM(num_lanes = rdc_revid_multiplicity());

    for(lane = 0;lane<num_lanes;lane++) {
        if ((err_analyzer_common_config->lane_mask_aggregate >> lane) & 0x1) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__,lane));
            /* Configure PRBS Error Analyzer for single lane */
            EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_lane_config(sa__, err_analyzer_lane_config));  
            /* Enable selected lane for aggregation */
            EFUN(wr_tlb_err_aggr_lanes_active(0x1)); 
        }
    }

    EFUN(wrc_tlb_err_aggr_en_force(0x1));

    if (err_analyzer_common_config->num_of_aggregate_lanes == 8)
       EFUN(wrc_tlb_err_aggr_mode(0x3));
    if (err_analyzer_common_config->num_of_aggregate_lanes == 4)
       EFUN(wrc_tlb_err_aggr_mode(0x2));
    if (err_analyzer_common_config->num_of_aggregate_lanes == 2)
       EFUN(wrc_tlb_err_aggr_mode(0x1));
    else
       EFUN(wrc_tlb_err_aggr_mode(0x0));

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_display_all_prbs_error_analyzer_proj(srds_access_t *sa__, enum peregrine5_pc_fec_code_type_enum fec_code_type, uint32_t timeout_s) {
    INIT_SRDS_ERR_CODE
    uint8_t lane_orig = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);

    /* Call the aggregate function to display error analyzer projections */
    EFUN(plp_aperta2_peregrine5_pc_display_aggregate_prbs_error_analyzer_proj(sa__, fec_code_type, PEREGRINE5_PC_DEFAULT_FEC_RATE, 0xFF, timeout_s));

    EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane_orig));

    return ERR_CODE_NONE;
}

/* Display PRBS Error Analyzer BER Projection */
err_code_t plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_proj(srds_access_t *sa__, enum peregrine5_pc_fec_code_type_enum fec_code_type, uint32_t timeout_s) {
    INIT_SRDS_ERR_CODE
    uint8_t lane_mask_aggregate;

    if (timeout_s == 0) {
        EFUN_PRINTF(("\nERROR: timeout_s value cannot be 0 for Lane %d >>\n", plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    ESTM(lane_mask_aggregate = (uint8_t)(0x1 << (plp_aperta2_peregrine5_pc_acc_get_lane(sa__))));

    /* Call the aggregate function to display error analyzer projections */
    EFUN(plp_aperta2_peregrine5_pc_display_aggregate_prbs_error_analyzer_proj(sa__, fec_code_type, PEREGRINE5_PC_DEFAULT_FEC_RATE, lane_mask_aggregate, timeout_s));
    return (ERR_CODE_NONE);
}


/* Display Aggregate PRBS Error Analyzer Projection */

err_code_t plp_aperta2_peregrine5_pc_display_aggregate_prbs_error_analyzer_proj(srds_access_t *sa__, enum peregrine5_pc_fec_code_type_enum fec_code_type, enum peregrine5_pc_prbs_error_analyzer_aggregate_mode_enum prbs_err_aggregate_mode, uint8_t lane_mask_aggregate, uint32_t timeout_s) {
    INIT_SRDS_ERR_CODE
    EFUN(plp_aperta2_peregrine5_pc_display_aggregate_prbs_error_analyzer_proj_with_encrypted_support(sa__, fec_code_type, prbs_err_aggregate_mode, lane_mask_aggregate, timeout_s, 0)); 
    return (ERR_CODE_NONE);
 
}

err_code_t plp_aperta2_peregrine5_pc_display_aggregate_prbs_error_analyzer_proj_with_encrypted_support(srds_access_t *sa__, enum peregrine5_pc_fec_code_type_enum fec_code_type, enum peregrine5_pc_prbs_error_analyzer_aggregate_mode_enum prbs_err_aggregate_mode, uint8_t lane_mask_aggregate, uint32_t timeout_s, uint8_t encrypt) {
    INIT_SRDS_ERR_CODE
    uint8_t hrs, mins, secs;
    uint8_t num_lanes, lane;
    uint8_t prbs_chk_lock_state;
    uint8_t i;
    enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;
    uint16_t prbs_err_fec_size;
    peregrine5_pc_prbs_err_analyzer_common_config_st err_analyzer_common_config;
    peregrine5_pc_prbs_err_analyzer_lane_config_st err_analyzer_lane_config;
    peregrine5_pc_prbs_err_analyzer_lane_status_st err_analyzer_lane_status;

    if (timeout_s == 0) {
        EFUN_PRINTF(("\nERROR: timeout_s value cannot be 0 for Lane %d >>\n", plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }


    /* read the number of lanes from register */
    ESTM(num_lanes = rdc_revid_multiplicity());

    /* Start out by checking for PRBS lock on all the lanes selected for aggregation */
    for(lane = 0;lane<num_lanes;lane++) {
        if ((lane_mask_aggregate >> lane) & 0x1) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__,lane));
            ESTM(prbs_chk_lock_state = rd_prbs_chk_lock_state());
            if (prbs_chk_lock_state != PRBS_CHECKER_LOCKED) {
                EFUN_PRINTF(("\nERROR : PRBS Checker is not locked[%d] for RX core %d, lane %d. Cannot run aggregation.\n",
                            prbs_chk_lock_state, plp_aperta2_peregrine5_pc_acc_get_core(sa__), lane));
                return (ERR_CODE_NONE);
            }
            /* fec error analyzer works at PAM4 mode only */
            EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
            if ( pam_mode != PAM4_NR && pam_mode != PAM4_ER ) {

                EFUN_PRINTF(("Error: FEC error analyzer only supported in PAM4 mode\n"));
                return(ERR_CODE_NONE);
            }
        }
    }

    ENULL_MEMSET(&err_analyzer_common_config, 0, sizeof(peregrine5_pc_prbs_err_analyzer_common_config_st));
    ENULL_MEMSET(&err_analyzer_lane_config, 0, sizeof(peregrine5_pc_prbs_err_analyzer_lane_config_st));
    ENULL_MEMSET(&err_analyzer_lane_status, 0, sizeof(peregrine5_pc_prbs_err_analyzer_lane_status_st));

    err_analyzer_common_config.lane_mask_aggregate = lane_mask_aggregate;
    err_analyzer_common_config.encrypt_mode = encrypt;

    err_analyzer_common_config.prbs_err_aggregate_mode = prbs_err_aggregate_mode;

    err_analyzer_lane_config.common_config_ptr = &err_analyzer_common_config;
    err_analyzer_lane_config.timeout_s = timeout_s;
    err_analyzer_lane_config.fec_code_type = fec_code_type;

    switch (fec_code_type) {
        case PEREGRINE5_PC_RS_544_514_10:
            prbs_err_fec_size = PEREGRINE5_PC_PRBS_FEC_RS_544_FRAME_SIZE;
            break;
        case PEREGRINE5_PC_RS_528_514_10:
            prbs_err_fec_size = PEREGRINE5_PC_PRBS_FEC_RS_528_FRAME_SIZE;
            break;
        default:
            EFUN_PRINTF(("\nERROR: FEC code type for Lane %d is not supported.\n", plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));
            return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    err_analyzer_lane_config.prbs_err_fec_size = prbs_err_fec_size;



    EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_reset(sa__, &err_analyzer_common_config));  
    /* Configure PRBS Error Analyzer with aggregation Mode*/
    EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_aggregate_config(sa__, &err_analyzer_lane_config));  

    /* Start PRBS Error Analyzer aggregate counters before the lane counters */
    EFUN(wrc_tlb_err_aggr_start_error_aggregation(0x1));   

    for(lane = 0;lane<num_lanes;lane++) {
        if ((err_analyzer_common_config.lane_mask_aggregate >> lane) & 0x1) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__,lane));
            /* Start PRBS Error Analyzer */
            EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_start(sa__, &err_analyzer_lane_config));  
        }
    }

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_seconds_to_displayformat(timeout_s, &hrs, &mins, &secs));
    EFUN_PRINTF((" \n Waiting for PRBS Error Analyzer measurement: time approx %d seconds (%d hr:%d mins: %ds) \n",timeout_s,hrs,mins,secs));
    EFUN_PRINTF(("\n"));

    /* Wait for required duration and accumulate errors */
    EFUN(USR_DELAY_MS(((uint32_t)timeout_s) * 1000));

    /* Stop PRBS Error Analyzer on all lanes */
    for(lane = 0;lane<num_lanes;lane++) {
        if ((err_analyzer_common_config.lane_mask_aggregate >> lane) & 0x1) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__,lane));
            EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_stop(sa__, &err_analyzer_lane_config));  
        }
    }
    if (err_analyzer_common_config.num_of_aggregate_lanes > 1)
    {
        /* Stop Error Analyzer aggregation */
        EFUN(wrc_tlb_err_aggr_start_error_aggregation(0x0));
    }

    /* Display PRBS Error Analyzer Err_Counts and projections */
    for(lane = 0;lane<num_lanes;lane++) {
        if ((err_analyzer_common_config.lane_mask_aggregate >> lane) & 0x1) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__,lane));
            /* Clear out prbs err counts from previous lane */
            for (i = 0; i < PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
               err_analyzer_lane_status.prbs_errcnt[i] = 0;
            }
            err_analyzer_lane_status.prbs_frames_all = 0;
#ifdef SERDES_API_FLOATING_POINT
            err_analyzer_lane_status.prbs_bit_errcnt = 0;
#endif
            /* Display PRBS Error Analyzer Config */
            EFUN(plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_err_count(sa__, &err_analyzer_lane_config, &err_analyzer_lane_status));
            EFUN(plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_struct(sa__, &err_analyzer_lane_config, &err_analyzer_lane_status));
            EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_compute_proj(sa__, &err_analyzer_lane_config, &err_analyzer_lane_status));
        }
    }

    /* For aggregated lane */
    if (err_analyzer_common_config.num_of_aggregate_lanes > 1)
    {
        /* Clear out prbs err counts from previous lane */
        for (i = 0; i < PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
          err_analyzer_lane_status.prbs_errcnt[i] = 0;
        }
        err_analyzer_lane_status.prbs_frames_all = 0;
#ifdef SERDES_API_FLOATING_POINT
        err_analyzer_lane_status.prbs_bit_errcnt = 0;
#endif
        EFUN(plp_aperta2_peregrine5_pc_get_prbs_error_analyzer_aggregate_err_count(sa__, &err_analyzer_lane_config, &err_analyzer_lane_status));
        EFUN(plp_aperta2_peregrine5_pc_display_prbs_error_analyzer_aggregate_err_count(sa__, &err_analyzer_lane_status, err_analyzer_common_config.encrypt_mode));
        EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_compute_proj(sa__, &err_analyzer_lane_config, &err_analyzer_lane_status));
    }

    return (ERR_CODE_NONE);
}


#ifdef SERDES_API_FLOATING_POINT
static USR_DOUBLE _plp_aperta2_peregrine5_pc_get_ber_from_fec_table(srds_access_t *sa__, USR_DOUBLE i_ber) {
    uint32_t i=0;
    USR_DOUBLE i_log = 0.0, o_log = 0.0;
    USR_DOUBLE diff_x = 0.0, diff_y = 0.0;
    static USR_DOUBLE fec_array[][2] = {
        {-4.00, -18.5702},
        {-4.25, -21.5670},
        {-4.50, -24.2549},
        {-4.75, -26.5850},
        {-5.00, -28.5482},
        {-5.25, -30.1630},
        {-5.50, -31.4698},
        {-5.75, -32.5143},
        {-6.00, -33.3458},
        {-6.25, -34.0061},
        {-6.50, -34.5361},
        {-6.75, -34.9666},
        {-7.00, -35.3316},
        {-7.25, -35.6498},
        {-7.50, -35.9393},
        {-7.75, -36.2140},
        {-8.00, -36.4776},
        {-8.25, -36.7352},
        {-8.50, -36.9914},
        {-8.75, -37.2418},
        {-9.00, -37.4935},
        {-9.25, -37.7447},
        {-9.50, -37.9957},
        {-9.75, -38.2449},
        {-10.00, -38.4949}
    };
    uint32_t fec_array_len = sizeof(fec_array)/sizeof(fec_array[0]);

    /* Convert i_ber to log */
    i_log = log10(i_ber);

    /* Linear search */
    for(i=0; i<fec_array_len;i++) {
        if(i_log > fec_array[i][0]) {
            if(i>1) {
                diff_x = fec_array[i-1][0] - i_log;
                diff_y = (fec_array[i-1][1] - fec_array[i][1])*(diff_x/(fec_array[i-1][0]-fec_array[i][0]));
                o_log = fec_array[i-1][1] - diff_y;
            } else {
                o_log = fec_array[i][1];
            }
            break;
        } else
            o_log = fec_array[i][1];
    }
    if (PRBS_VERBOSE > 1) {
        EFUN_PRINTF(("Raw BER: %.1f, %d:%.4f\n", i_log, i, o_log));
    }

    return pow(10.0,o_log);
}

#endif /* SERDES_API_FLOATING_POINT */

/*----------------------------------------------------------------------------------------------------------------*/
/* Following calculations are used for projecting the Bit Error Rate (BER) from the PRBS Error Analyzer result    */
/*  - test_time_in_bits     = test_time_ms * num_bits_per_ms                                                      */
/*  - num_frames_in_test    = test_time_in_bits / (num_bits_per_frame * frame_overlap_ratio)                      */
/*  - frame_err_rate  (FER) = num_frame_errors / num_frames_in_test                                               */
/*  - bit_err_rate    (BER) = FER / num_bits_per_frame                                                            */
/*                  => BER  = (num_frame_errors * frame_overlap_ratio) / (test_time_ms * num_bits_per_ms)         */
/*                            where, frame_overlap_ratio = 1 / frame_overlap_factor                               */
/*                                                                                                                */
/* Following guidelines are used while computing projected values -                                               */
/*   i) Only "frame_errors" that are in between "10 and 65535" are considered as valid data points for projection */
/*  ii) Need at least 2 valid data points to generate projected data                                              */
/*----------------------------------------------------------------------------------------------------------------*/
#ifdef SERDES_API_FLOATING_POINT
static void _plp_aperta2_linear_fit_extrapolate_ber(srds_access_t *sa__, uint8_t start_idx, uint8_t stop_idx, uint8_t t, USR_DOUBLE meas_fec_ber[], USR_DOUBLE proj_ber[]){
    uint8_t     i;
    uint8_t    delta_n;
    USR_DOUBLE Exy = 0.0;
    USR_DOUBLE Eyy = 0.0;
    USR_DOUBLE Exx = 0.0;
    USR_DOUBLE Ey  = 0.0;
    USR_DOUBLE Ex  = 0.0;
    USR_DOUBLE alpha = 0.0, alpha2 = 0.0, sq_r = 0.0;
    USR_DOUBLE beta = 0.0;

    delta_n = (uint8_t)(stop_idx - start_idx);       /* Number of MEASURED points available for extrapolation */
    if (delta_n < 2) {
        EFUN_PRINTF(("\t Number of measured points not enough for extrapolation %d - %d = %d\n", stop_idx, start_idx, delta_n));
        for (i=0; i<PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1; i++) {
            proj_ber[i] = 0.0;
        }
        return;
    }

    /* Compute covariance and mean for extrapolation */
    for(i = start_idx; i < stop_idx; i++) {
        USR_DOUBLE x, y;
        x    = (USR_DOUBLE) (i);
        y    = pow((-log10(meas_fec_ber[i])),PEREGRINE5_PC_PRBS_ERR_ANALYZER_LINEARITY_ADJUST);
        if (PRBS_VERBOSE > 3) {
            EFUN_PRINTF(("[%d], x: %f, y: %f\n", i, x, y));
        }

        Exy += ((x * y)/(USR_DOUBLE)delta_n);
        Eyy += ((y * y)/(USR_DOUBLE)delta_n);
        Exx += ((x * x)/(USR_DOUBLE)delta_n);
        Ey  += ((  y  )/(USR_DOUBLE)delta_n);
        Ex  += ((  x  )/(USR_DOUBLE)delta_n);
    }

    /* Compute fit slope and offset: ber = alpha*frame_err_thresh + beta */
    alpha = (Exy - Ey*Ex) / (Exx - Ex*Ex);
    beta  = Ey - Ex*alpha;
    /* Compute alpha2: slope of regression: frame_err_thresh = alpha2*ber + beta2 */
    alpha2 = (Exy - Ey*Ex) / (Eyy - Ey*Ey);
    /* Compute correlation index sq_r */
    sq_r = alpha*alpha2;

    if (PRBS_VERBOSE > 2) {
        EFUN_PRINTF(("\n\t << Computing linear fit from Measured Equivalent BER points >>\n"));
        EFUN_PRINTF(("\t Number of measured points used for extrapolation %d - %d = %d\n", stop_idx, start_idx, delta_n));
        EFUN_PRINTF(("\t Exy=%lf, Eyy=%lf, Exx=%lf, Ey=%lf, Ex=%lf\n",Exy,Eyy,Exx,Ey,Ex));
        EFUN_PRINTF(("\t alpha=%lf, beta=%lf\n",alpha,beta));
        EFUN_PRINTF(("\t sq_r=%lf\n",sq_r));
        EFUN_PRINTF(("\n\t=====> DEBUG INFO (end)\n\n"));
    }

    proj_ber[t] = ((alpha * t) + beta);
    proj_ber[t] = pow(10,-proj_ber[t]);
}
#endif /* SERDES_API_FLOATING_POINT */

err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_report_proj(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config,
        peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status, peregrine5_pc_prbs_err_analyzer_report_st *err_analyzer_report) {
#ifdef SERDES_API_FLOATING_POINT
    INIT_SRDS_ERR_CODE
    uint8_t    delta_n;
    uint8_t     i;
    uint8_t    overlap_factor;
    uint8_t    start_idx, stop_idx;
    uint32_t   num_bits_per_ms;
    uint32_t   timeout_s;
    USR_DOUBLE meas_fec_ber[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS];
    USR_DOUBLE proj_ber[16] = {0.0};
    USR_DOUBLE raw_ber = 0.0; 

    if (err_analyzer_config == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (err_analyzer_status == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    timeout_s = err_analyzer_config->timeout_s;
    if (timeout_s == 0) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    if (err_analyzer_report == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ENULL_MEMSET(err_analyzer_report, 0, sizeof(peregrine5_pc_prbs_err_analyzer_report_st));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_num_bits_per_ms(sa__, 1, &num_bits_per_ms));

    if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_8)
        overlap_factor = 8;
    else if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_4)
        overlap_factor = 4;
    else if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_2)
        overlap_factor = 2;
    else
        overlap_factor = 1;

    if (PRBS_VERBOSE > 2) {
        EFUN_PRINTF(("\n\t=====> DEBUG INFO (start)\n"));
        EFUN_PRINTF(("\n\t << Measured Equivalent BER for specific error correcting FEC >>\n"));
    }


    /* Loop to calculate the measured BER based on above mentioned calculations */
    for(i=0; i < PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
        if (num_bits_per_ms !=0) {
            meas_fec_ber[i] = ((((USR_DOUBLE)(err_analyzer_status->prbs_errcnt[i]) / (USR_DOUBLE) overlap_factor) / (USR_DOUBLE)(timeout_s * 1000)) / (USR_DOUBLE) num_bits_per_ms);
        }
        else {
            meas_fec_ber[i] = (USR_DOUBLE)0xFFFFFFFF;
        }
        if (PRBS_VERBOSE > 2) {
            EFUN_PRINTF(("\t Measured Equivalent BER at 't=%2d' (((frameErr/overlap_factor)/time_ms)/rate) = (((%u/%u)/%d)/%d): %0.3e \n",i, err_analyzer_status->prbs_errcnt[i],
                                                                                overlap_factor, timeout_s*1000, num_bits_per_ms, meas_fec_ber[i]));
        }
    }

    /* Calculating number of MEASURED points available for extrapolation */
    /* start_idx - first non-max FEC frame error value; stop_idx - index where FEC frame errors < 10 errors */
    start_idx = 0;
    stop_idx  = 0;
    if (err_analyzer_status->prbs_errcnt[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1] != 0xFFFFFFFF) {
        stop_idx  = PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1;
    }
    for (i=PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i > 0; i--) {
        if (err_analyzer_status->prbs_errcnt[i - 1] != 0xFFFFFFFF) {
            start_idx = (uint8_t)(i - 1);
        }
        if (err_analyzer_status->prbs_errcnt[i- 1] < ((uint32_t)(10*overlap_factor))) { /* "10*overlap_factor" means "10 error events" */
            stop_idx = (uint8_t)(i - 1);
        }
    }
    delta_n = (uint8_t)(stop_idx - start_idx);                                                  /* Number of MEASURED points available for extrapolation */

    /* Calulate Projected BER => Equivalent projected post-FEC BER for t=15 or t=7 */
    if (err_analyzer_config->fec_code_type == PEREGRINE5_PC_RS_528_514_10) {
        _plp_aperta2_linear_fit_extrapolate_ber(sa__, start_idx, stop_idx, 7, meas_fec_ber, proj_ber);
    }
    else {
        _plp_aperta2_linear_fit_extrapolate_ber(sa__, start_idx, stop_idx, 15, meas_fec_ber, proj_ber);
        }

    /* Calculate raw BER */
    if (err_analyzer_status->prbs_bit_errcnt < 3.0) {
         raw_ber = 3.0 / (USR_DOUBLE)(timeout_s * 1000) / (USR_DOUBLE) (num_bits_per_ms);
    }
    else
    {    if (err_analyzer_status->prbs_bit_errcnt >= 0xFFFFFFFFFFFFF)
              raw_ber = 1.0;
         else
              raw_ber = (err_analyzer_status->prbs_bit_errcnt) / (USR_DOUBLE) (timeout_s * 1000) / (USR_DOUBLE) (num_bits_per_ms);
    }
    if (err_analyzer_config->fec_code_type == PEREGRINE5_PC_RS_528_514_10) {
        err_analyzer_report->proj_ber  = proj_ber[7];
    }
    else {
        err_analyzer_report->proj_ber  = proj_ber[15];
    }

    if (err_analyzer_report->proj_ber == 0) {
        /* Get expected BER from FEC table */
        err_analyzer_report->proj_ber = _plp_aperta2_peregrine5_pc_get_ber_from_fec_table(sa__, raw_ber);   
    }


    /* To be populated: 0 -> valid, +1 -> BER greater than; -1 -> BER less than */
    err_analyzer_report->ber_proj_invalid  = 0;       
    err_analyzer_report->delta_n   = delta_n;
        if (delta_n >= 2) {
        err_analyzer_report->report_ber = SRDS_REPT_BER_EQUAL_TO;
        }
        else {
        err_analyzer_report->report_ber = SRDS_REPT_BER_LESS_AND_EQUAL;
        }
    return (ERR_CODE_NONE);
#else
    UNUSED(err_analyzer_config);
    UNUSED(err_analyzer_status);
    UNUSED(err_analyzer_report);
    USR_PRINTF(("%s : This function needs SERDES_API_FLOATING_POINT define to operate \n", API_FUNCTION_NAME));
    return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
#endif /* SERDES_API_FLOATING_POINT */
}

#if defined(SERDES_API_FLOATING_POINT)
static err_code_t _plp_aperta2_peregrine5_pc_prbs_error_analyzer_is_low_confidence(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config, peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status, uint8_t *low_confidence) {
    INIT_SRDS_ERR_CODE
    uint8_t nr_ecd_en = 0;
    uint8_t er_ecd_en = 0;
    uint8_t overlap_factor;
    const uint32_t THRESHOLD_COUNT = 10;
    uint32_t factored_threshold;

    if (err_analyzer_config == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (err_analyzer_status == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    *low_confidence = 0;
    if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_8)
        overlap_factor = 8;
    else if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_4)
        overlap_factor = 4;
    else if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_2)
        overlap_factor = 2;
    else
        overlap_factor = 1;

    ESTM(er_ecd_en = rd_er_ecd_en());
    ESTM(nr_ecd_en = rd_nr_ecd_en());
    factored_threshold = overlap_factor * THRESHOLD_COUNT;

    if ( er_ecd_en || nr_ecd_en ) {
        if (err_analyzer_status->prbs_errcnt[0] > factored_threshold && 
            err_analyzer_status->prbs_errcnt[1] > factored_threshold &&
            err_analyzer_status->prbs_errcnt[2] < factored_threshold) {
                if (PRBS_VERBOSE > 1) {
                    EFUN_PRINTF(("\t projecting using 90%% confidence with t=2 datapoint!\n"));
                }
                *low_confidence = 1;
        }
    } 

    return (ERR_CODE_NONE);
}
#endif /* defined(SERDES_USE_LINEAR_FIT) && defined(SERDES_API_FLOATING_POINT) */

#ifdef SERDES_API_FLOATING_POINT
static err_code_t _plp_aperta2_peregrine5_pc_prbs_error_analyzer_low_confidence_proj(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config, \
        peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status, peregrine5_pc_prbs_err_analyzer_report_st *err_analyzer_report) {
    INIT_SRDS_ERR_CODE
    uint8_t    delta_n;
    uint8_t     i;
    uint8_t    overlap_factor;
    uint8_t    start_idx, stop_idx;
    uint32_t   num_bits_per_ms;
    uint32_t   timeout_s;
    USR_DOUBLE meas_fec_ber[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS] = {0.0};
    USR_DOUBLE proj_ber[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS] = {0.0};
    USR_DOUBLE raw_ber = 0.0; 
    peregrine5_pc_prbs_err_analyzer_lane_status_st err_analyzer_status_adjusted;
    peregrine5_pc_prbs_err_analyzer_report_st err_analyzer_rept_adjusted;


    if (err_analyzer_config == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (err_analyzer_status == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    timeout_s = err_analyzer_config->timeout_s;
    if (timeout_s == 0) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    if (err_analyzer_report == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    err_analyzer_status_adjusted = *err_analyzer_status;
    err_analyzer_rept_adjusted = *err_analyzer_report;

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_num_bits_per_ms(sa__, 1, &num_bits_per_ms));

    if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_8)
        overlap_factor = 8;
    else if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_4)
        overlap_factor = 4;
    else if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_2)
        overlap_factor = 2;
    else
        overlap_factor = 1;

    err_analyzer_status_adjusted.prbs_errcnt[2] = (uint32_t)(overlap_factor * 2.3026);
    err_analyzer_status_adjusted.prbs_errcnt[1] = (uint32_t)pow(10, (log10(err_analyzer_status_adjusted.prbs_errcnt[0]) + log10(err_analyzer_status_adjusted.prbs_errcnt[2]))/2);

    if (PRBS_VERBOSE > 1) {
        /* Display Adjusted PRBS Error Analyzer Err_Counts */
        EFUN_PRINTF(("\n -------------------------------------------------------------\n"));
        EFUN_PRINTF(("  Adjusted PRBS Error Analyzer Error_Counts for Lane %d:\n", err_analyzer_status_adjusted.pp_field2));
        EFUN_PRINTF((" -------------------------------------------------------------\n"));
        for (i=0; i<PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
            if (err_analyzer_status_adjusted.prbs_errcnt[i] == 0xFFFFFFFF) {
                EFUN_PRINTF(("    (%d,%d) FEC Frames with > %2d Errors (t=%2d) =        MAX \n", 
                            plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__), i, i));
    }
    else {
                EFUN_PRINTF(("    (%d,%d) FEC Frames with > %2d Errors (t=%2d) = %10u \n", 
                            plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__), i, i, err_analyzer_status_adjusted.prbs_errcnt[i]));
            }
        }
    }

    if (PRBS_VERBOSE > 2) {
        EFUN_PRINTF(("\n\t=====> DEBUG INFO (start)\n"));
        EFUN_PRINTF(("\n\t << Adjusted Equivalent BER for specific error correcting FEC >>\n"));
    }


    /* Loop to calculate the measured BER based on above mentioned calculations */
    for(i=0; i < PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
        if (num_bits_per_ms !=0) {
            meas_fec_ber[i] = ((((USR_DOUBLE)(err_analyzer_status_adjusted.prbs_errcnt[i]) / (USR_DOUBLE) overlap_factor) / (USR_DOUBLE)(timeout_s * 1000)) / (USR_DOUBLE) num_bits_per_ms);
        }
        else {
            meas_fec_ber[i] = (USR_DOUBLE)0xFFFFFFFF;
        }
        if (PRBS_VERBOSE > 2) {
            EFUN_PRINTF(("\t Adjusted Equivalent BER at 't=%2d' (((frameErr/overlap_factor)/time_ms)/rate) = (((%u/%u)/%d)/%d): %0.3e \n",i, err_analyzer_status_adjusted.prbs_errcnt[i],
                                                                                overlap_factor, timeout_s*1000, num_bits_per_ms, meas_fec_ber[i]));
        }
        }

    /* if t=0 BER is < 1.0e-11, just report projected BER as <= 1.0e-24 */
    if ( meas_fec_ber[0] < 1.0e-11) {
        err_analyzer_report->report_ber = SRDS_REPT_BER_LESS_AND_EQUAL;
        err_analyzer_report->proj_ber = 1.0e-24;

        return (ERR_CODE_NONE);
    }

    /* Calculating number of MEASURED points available for extrapolation */
    /* start_idx - first non-max FEC frame error value; stop_idx - index where FEC frame errors < 1 errors */
    start_idx = 0;
    stop_idx  = 0;
    if (err_analyzer_status_adjusted.prbs_errcnt[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1] != 0xFFFFFFFF) {
        stop_idx  = PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1;
    }
    for (i=PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i > 0; i--) {
        if (err_analyzer_status_adjusted.prbs_errcnt[i - 1] != 0xFFFFFFFF) {
            start_idx = (uint8_t)(i - 1);
        }
        if (err_analyzer_status_adjusted.prbs_errcnt[i- 1] < ((uint32_t)(1*overlap_factor))) { /* "1*overlap_factor" means "1 error events" */
            stop_idx = (uint8_t)(i - 1);
        }
    }
    delta_n = (uint8_t)(stop_idx - start_idx);       /* Number of MEASURED points available for extrapolation */


    /* Calulate Projected BER => Equivalent projected post-FEC BER for t=15 or t=7 */
    if (err_analyzer_config->fec_code_type == PEREGRINE5_PC_RS_528_514_10) {
        _plp_aperta2_linear_fit_extrapolate_ber(sa__, start_idx, stop_idx, 7, meas_fec_ber, proj_ber);
    }
    else {
        _plp_aperta2_linear_fit_extrapolate_ber(sa__, start_idx, stop_idx, 15, meas_fec_ber, proj_ber);
    }
    /* Calculate raw BER */
    if (err_analyzer_status_adjusted.prbs_bit_errcnt < 3.0) {
         raw_ber = 3.0 / (USR_DOUBLE)(timeout_s * 1000) / (USR_DOUBLE) (num_bits_per_ms);
    }
    else
    {    if (err_analyzer_status_adjusted.prbs_bit_errcnt >= 0xFFFFFFFFFFFFF)
              raw_ber = 1.0;
         else
              raw_ber = (err_analyzer_status_adjusted.prbs_bit_errcnt) / (USR_DOUBLE) (timeout_s * 1000) / (USR_DOUBLE) (num_bits_per_ms);
    }


    /* Populate output structure */
    if (err_analyzer_config->fec_code_type == PEREGRINE5_PC_RS_528_514_10) {
        err_analyzer_rept_adjusted.proj_ber  = proj_ber[7];
    }
    else {
        err_analyzer_rept_adjusted.proj_ber  = proj_ber[15];
    }

    if (err_analyzer_rept_adjusted.proj_ber == 0) {
        /* Get expected BER from FEC table */
        err_analyzer_rept_adjusted.proj_ber = _plp_aperta2_peregrine5_pc_get_ber_from_fec_table(sa__, raw_ber);   
    }

    /* To be populated: 0 -> valid, +1 -> BER greater than; -1 -> BER less than */
    err_analyzer_rept_adjusted.ber_proj_invalid  = 0;       
    err_analyzer_rept_adjusted.delta_n   = delta_n;
    err_analyzer_rept_adjusted.report_ber = SRDS_REPT_BER_LESS_THAN;

    if ( err_analyzer_report->proj_ber > err_analyzer_rept_adjusted.proj_ber ) {
        if (PRBS_VERBOSE > 1) {
            EFUN_PRINTF(("BER reported: %.3e\n", err_analyzer_report->proj_ber));
            EFUN_PRINTF(("BER adjusted : %.3e\n", err_analyzer_rept_adjusted.proj_ber));
        }
        *err_analyzer_report = err_analyzer_rept_adjusted;
    }
    return (ERR_CODE_NONE);
}
#endif /* SERDES_API_FLOATING_POINT */

err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_report_proj2(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config,
        peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status, peregrine5_pc_prbs_err_analyzer_report_st *err_analyzer_report) {
#ifdef SERDES_API_FLOATING_POINT
    INIT_SRDS_ERR_CODE
    uint8_t    delta_n;
    uint8_t    i, proj_err_bin;
    uint8_t    overlap_factor;
    uint8_t    start_idx, stop_idx;
    uint32_t   num_bits_per_ms;
    uint32_t   timeout_s;
    USR_DOUBLE meas_fec_ber[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS];
    USR_DOUBLE proj_ber[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS] = { 0.0 };
    USR_DOUBLE a[3] = { 0.0 }; /* coefficients */
    USR_DOUBLE raw_ber = 0.0;

    if (err_analyzer_config == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (err_analyzer_status == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    timeout_s = err_analyzer_config->timeout_s;
    if (timeout_s == 0) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    if (err_analyzer_report == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ENULL_MEMSET(err_analyzer_report, 0, sizeof(peregrine5_pc_prbs_err_analyzer_report_st));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_num_bits_per_ms(sa__, 1, &num_bits_per_ms));

    if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_8)
        overlap_factor = 8;
    else if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_4)
        overlap_factor = 4;
    else if (err_analyzer_status->fec_size_frac_programmed == PEREGRINE5_PC_PRBS_FEC_SIZE_FRAC_1_2)
        overlap_factor = 2;
    else
        overlap_factor = 1;

    if (PRBS_VERBOSE > 2) {
        EFUN_PRINTF(("\n\t=====> DEBUG INFO (start)\n"));
        EFUN_PRINTF(("\n\t << Measured Equivalent BER for specific error correcting FEC >>\n"));
    }


    /* Loop to calculate the measured BER based on above mentioned calculations */
    for(i=0; i < PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
        if (num_bits_per_ms !=0) {
            meas_fec_ber[i] = ((((USR_DOUBLE)(err_analyzer_status->prbs_errcnt[i]) / (USR_DOUBLE) overlap_factor) / (USR_DOUBLE)(timeout_s * 1000)) / (USR_DOUBLE) num_bits_per_ms);
        }
        else {
            meas_fec_ber[i] = (USR_DOUBLE)0xFFFFFFFF;
        }
        if (PRBS_VERBOSE > 2) {
            EFUN_PRINTF(("\t Measured Equivalent BER at 't=%2d' (((frameErr/overlap_factor)/time_ms)/rate) = (((%u/%u)/%d)/%d): %0.3e \n",i, err_analyzer_status->prbs_errcnt[i],
                                                                                overlap_factor, timeout_s*1000, num_bits_per_ms, meas_fec_ber[i]));
        }
    }

    /* Calculating number of MEASURED points available for extrapolation */
    /* start_idx - first non-max FEC frame error value; stop_idx - index where FEC frame errors < 10 errors */
    start_idx = 0;
    stop_idx  = 0;
    if (err_analyzer_status->prbs_errcnt[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1] != 0xFFFFFFFF) {
        stop_idx  = PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1;
    }
    for (i=PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i > 0; i--) {
        if (err_analyzer_status->prbs_errcnt[i - 1] != 0xFFFFFFFF) {
            start_idx = (uint8_t)(i - 1);
        }
        if (err_analyzer_status->prbs_errcnt[i- 1] < ((uint32_t)(10*overlap_factor))) { /* "10*overlap_factor" means "10 error events" */
            stop_idx = (uint8_t)(i - 1);
        }
    }
    
    /* Number of MEASURED points available for extrapolation */
    delta_n = (uint8_t)(stop_idx - start_idx);                                                  
    if (delta_n >= 3) { /* There need at least 3 points to trace a 2nd degree fit for extrapolation */
        USR_DOUBLE x[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1] = { 0.0 };
        USR_DOUBLE y[PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1] = { 0.0 };
        for(i = start_idx; i < stop_idx; i++) {
            x[i-start_idx]    = (USR_DOUBLE) i;
            y[i-start_idx]    = -log10(meas_fec_ber[i]);
            if (PRBS_VERBOSE > 3) {
                EFUN_PRINTF(("[%d], x: %f, y: %f\n", i-start_idx, x[i-start_idx], y[i-start_idx]));
            }
        }

        /* Compute coeffients for extrapolation using Least Square polynomia fitting
         * polynomia : f(x) = a[2]x^2 + a[1]x^1 + a[0]
         */
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_fit_second_order(sa__, delta_n, x, y, a));
    }

    if (PRBS_VERBOSE > 2) {
        EFUN_PRINTF(("\n\t << Computing second order fit from Measured Equivalent BER points >>\n"));
        EFUN_PRINTF(("\t Number of measured points used for extrapolation %d - %d= %d\n", stop_idx, start_idx, delta_n));
        EFUN_PRINTF(("\t %6.3lfx^2 + %6.3lfx + %6.3lf\n", a[2], a[1], a[0]));
        EFUN_PRINTF(("\n\t=====> DEBUG INFO (end)\n\n"));
    }


    /* Calulate Projected BER => Equivalent projected post-FEC BER for t=15 */
    if (err_analyzer_config->fec_code_type == PEREGRINE5_PC_RS_528_514_10) {
        proj_err_bin = 7;
    }
    else {
        proj_err_bin = 15;
    }
    if (delta_n >= 3) {
        proj_ber[proj_err_bin] = a[2] * pow(proj_err_bin, 2) + a[1] * (proj_err_bin) + a[0];
        proj_ber[proj_err_bin] = pow(10,-proj_ber[proj_err_bin]);
        err_analyzer_report->report_ber = SRDS_REPT_BER_EQUAL_TO;
    }
    else {
        proj_ber[proj_err_bin] = 0.0;
        err_analyzer_report->report_ber = SRDS_REPT_BER_LESS_AND_EQUAL;
    }

#ifdef SERDES_API_FLOATING_POINT
    /* Calculate raw BER */
    if (err_analyzer_status->prbs_bit_errcnt < 3.0)
         raw_ber = 3.0 / (USR_DOUBLE)(timeout_s * 1000) / (USR_DOUBLE) (num_bits_per_ms);
    else
    {    if (err_analyzer_status->prbs_bit_errcnt >= 0xFFFFFFFFFFFFF)
              raw_ber = 1.0;
         else
              raw_ber = (err_analyzer_status->prbs_bit_errcnt) / (USR_DOUBLE) (timeout_s * 1000) / (USR_DOUBLE) (num_bits_per_ms);
    }
#endif



    /* Populate output structure */
    err_analyzer_report->proj_ber  = proj_ber[proj_err_bin];

    if (err_analyzer_report->proj_ber == 0) {
        /* Get expected BER from FEC table */
        err_analyzer_report->proj_ber = _plp_aperta2_peregrine5_pc_get_ber_from_fec_table(sa__, raw_ber);   
    }

    /* To be populated: 0 -> valid, +1 -> BER greater than; -1 -> BER less than */
    err_analyzer_report->ber_proj_invalid  = 0;       
    err_analyzer_report->delta_n   = delta_n;

    return (ERR_CODE_NONE);
#else
    UNUSED(err_analyzer_config);
    UNUSED(err_analyzer_status);
    UNUSED(err_analyzer_report);
    USR_PRINTF(("%s : This function needs SERDES_API_FLOATING_POINT define to operate \n", API_FUNCTION_NAME));
    return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
#endif /* SERDES_API_FLOATING_POINT */
}




err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_compute_proj(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config,
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
    {
        uint8_t low_confidence = 0;
        EFUN(_plp_aperta2_peregrine5_pc_prbs_error_analyzer_is_low_confidence(sa__, err_analyzer_config, err_analyzer_status, &low_confidence));
        if ( low_confidence ) {
            EFUN(_plp_aperta2_peregrine5_pc_prbs_error_analyzer_low_confidence_proj(sa__, err_analyzer_config, err_analyzer_status, &err_analyzer_rept));       
        }
    }

    EFUN(plp_aperta2_peregrine5_pc_prbs_error_analyzer_report_display(sa__, err_analyzer_config, &err_analyzer_rept));


#else
    UNUSED(err_analyzer_status);
    UNUSED(err_analyzer_config);
#endif /* SERDES_API_FLOATING_POINT */

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_prbs_error_analyzer_report_display(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_config_st *err_analyzer_config,
        peregrine5_pc_prbs_err_analyzer_report_st *err_analyzer_rept) {
#ifdef SERDES_API_FLOATING_POINT
    uint8_t err_bin = 15;

    if (err_analyzer_config == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    if (err_analyzer_rept == NULL){
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }


    if (err_analyzer_config->fec_code_type == PEREGRINE5_PC_RS_528_514_10) {
        err_bin = 7;
    }
    else {
        err_bin = 15;
    }

    if (err_analyzer_rept->report_ber == SRDS_REPT_BER_LESS_AND_EQUAL) {
        EFUN_PRINTF(("\n  PRBS Error Analyzer Projected BER (Equivalent projected post-FEC BER for t=%d) for Lane %d <= %0.3e\n\n", err_bin, plp_aperta2_peregrine5_pc_acc_get_lane(sa__), err_analyzer_rept->proj_ber));
    }
    else {

        /* Displaying Projected BER => Equivalent projected post-FEC BER for t=15 */
        if (err_analyzer_rept->report_ber == SRDS_REPT_BER_EQUAL_TO) {
        EFUN_PRINTF(("\n  PRBS Error Analyzer Projected BER (Equivalent projected post-FEC BER for t=%d) for Lane %d = %0.3e\n\n", err_bin, plp_aperta2_peregrine5_pc_acc_get_lane(sa__), err_analyzer_rept->proj_ber));
    }
    else {
            EFUN_PRINTF(("\n  PRBS Error Analyzer Projected BER (Equivalent projected post-FEC BER for t=%d) for Lane %d < %0.3e\n\n", err_bin, plp_aperta2_peregrine5_pc_acc_get_lane(sa__), err_analyzer_rept->proj_ber));
        }
    }


#else /* not #defined(SERDES_API_FLOATING_POINT) */
    UNUSED(err_analyzer_config);
    UNUSED(err_analyzer_rept);
#endif 

    return (ERR_CODE_NONE);
}


err_code_t plp_aperta2_peregrine5_pc_get_rx_tuning_status(srds_access_t *sa__, uint8_t *status) {
    INIT_SRDS_ERR_CODE

    if(status == NULL) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }
    {
        uint8_t flr_en = 0;
        ESTM(flr_en = rd_flr_en());
        if(flr_en) {
            EFUN(plp_aperta2_peregrine5_pc_get_flr_ready_status(sa__, status));
        }
        else {
            ESTM(*status = (rdv_rx_init_tuning_status() == SSA_MODE));
        }
    }
    return(ERR_CODE_NONE);
}

#endif /* SMALL_FOOTPRINT */



