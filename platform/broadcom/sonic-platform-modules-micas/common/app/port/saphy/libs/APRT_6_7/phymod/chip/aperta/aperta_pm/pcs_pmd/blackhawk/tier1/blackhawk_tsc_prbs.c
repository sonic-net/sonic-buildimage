/**************************************************************************************
 **************************************************************************************
 *  File Name     :  blackhawk_tsc_prbs.c                                                *
 *  Created On    :  04 Nov 2015                                                      *
 *  Created By    :  Brent Roberts                                                    *
 *  Description   :  APIs for Serdes IPs                                              *
  * $Copyright: (c) 2020 Broadcom.                                               *
   * Broadcom Proprietary and Confidential. All rights reserved.$                 *
 *                                                                                    *
 **************************************************************************************
 **************************************************************************************/

/** @file blackhawk_tsc_prbs.c
 * Implementation of API PRBS functions
 */
#include <phymod/phymod_system.h>
#include "blackhawk_tsc_prbs.h"
#include "blackhawk_tsc_common.h"
#include "blackhawk_tsc_dependencies.h"
#include "blackhawk_tsc_functions.h"
#include "blackhawk_tsc_internal.h"
#include "blackhawk_tsc_internal_error.h"
#include "blackhawk_tsc_select_defns.h"
#include "blackhawk_tsc_access.h"

#ifdef SERDES_API_FLOATING_POINT
#include <math.h>
#endif

/******************************/
/*  TX Pattern Generator APIs */
/******************************/

/* Cofigure shared TX Pattern (Return Val = 0:PASS, 1-6:FAIL (reports 6 possible error scenarios if failed)) */
err_code_t plp_aperta_blackhawk_tsc_config_shared_tx_pattern(srds_access_t *sa__, uint8_t patt_length, const char pattern[]) {

  char       patt_final[245] = "";
  char       patt_mod[245]   = "", bin[5] = "";
  uint8_t    str_len, i, k, j = 0;
  uint8_t    offset_len, actual_patt_len = 0, hex = 0;
  uint8_t    zero_pad_len    = 0; /* suppress warnings, changed by plp_aperta_blackhawk_tsc_INTERNAL_calc_patt_gen_mode_sel() */
  uint16_t   patt_gen_wr_val = 0;
  uint8_t    mode_sel        = 0; /* suppress warnings, changed by plp_aperta_blackhawk_tsc_INTERNAL_calc_patt_gen_mode_sel() */

  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_calc_patt_gen_mode_sel(sa__, &mode_sel,&zero_pad_len,patt_length));

  /* Generating the appropriate write value to patt_gen_seq registers */
  str_len = (uint8_t)USR_STRLEN(pattern);

  if ((str_len > 2) && ((USR_STRNCMP(pattern, "0x", 2)) == 0)) {
    /* Hexadecimal Pattern */
    for (i=2; i < str_len; i++) {
      if (pattern[i] != '_') {
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_compute_bin(sa__, pattern[i],bin));
        ENULL_STRNCAT(patt_mod, bin, 5);
        actual_patt_len = (uint8_t)(actual_patt_len + 4);
        if (actual_patt_len > 240) {
          EFUN_PRINTF(("ERROR: Pattern bigger than max pattern length\n"));
          return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN));
        }
      }
    }

    offset_len = (uint8_t)(actual_patt_len - patt_length);
    if ((offset_len > 3)  || (actual_patt_len < patt_length)) {
      EFUN_PRINTF(("ERROR: Pattern length provided does not match the hexadecimal pattern provided\n"));
      return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
    }
    else if (offset_len) {
      for (i=0; i < offset_len; i++) {
        if (patt_mod[i] != '0') {
          EFUN_PRINTF(("ERROR: Pattern length provided does not match the hexadecimal pattern provided\n"));
          return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
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
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_pam4_to_bin(sa__, pattern[i],bin));
        ENULL_STRNCAT(patt_mod, bin, 5);
        actual_patt_len = (uint8_t)(actual_patt_len + 2);
        if (actual_patt_len > 240) {
          EFUN_PRINTF(("ERROR: Pattern bigger than max pattern length\n"));
          return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN));
        }
      }
    }

    offset_len = (uint8_t)(actual_patt_len - patt_length);
    if ((offset_len > 3)  || (actual_patt_len < patt_length)) {
      EFUN_PRINTF(("ERROR: Pattern length provided does not match the PAM4 pattern provided\n"));
      return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
    }
    else if (offset_len) {
      for (i=0; i < offset_len; i++) {
        if (patt_mod[i] != '0') {
          EFUN_PRINTF(("ERROR: Pattern length provided does not match the PAM4 pattern provided\n"));
          return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
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
        bin[0] = pattern[i];
        bin[1] = '\0';
        ENULL_STRNCAT(patt_mod, bin, 5);
        actual_patt_len++;
        if (actual_patt_len > 240) {
          EFUN_PRINTF(("ERROR: Pattern bigger than max pattern length\n"));
          return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN));
        }
      }
      else if (pattern[i] != '_') {
        EFUN_PRINTF(("ERROR: Invalid input Pattern\n"));
        return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_INVALID_PATTERN));
      }
    }

    if (actual_patt_len != patt_length) {
      EFUN_PRINTF(("ERROR: Pattern length provided does not match the binary pattern provided\n"));
      return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_LEN_MISMATCH));
    }
  }

  /* Zero padding upper bits and concatinating patt_mod to form patt_final */
  for (i=0; i < zero_pad_len; i++) {
    ENULL_STRNCAT(patt_final, "0", 2);
    j++;
  }
  for (i=zero_pad_len; i + patt_length < 241; i = (uint8_t)(i + patt_length)) {
    ENULL_STRNCAT(patt_final, patt_mod, patt_length);
    j++;
  }

  /* EFUN_PRINTF(("\nFinal Pattern = %s\n\n",patt_final));    */

  for (i=0; i < 15; i++) {

    for (j=0; j < 4; j++) {
      k = (uint8_t)(i*16 + j*4);
      bin[0] = patt_final[k];
      bin[1] = patt_final[k+1];
      bin[2] = patt_final[k+2];
      bin[3] = patt_final[k+3];
      bin[4] = '\0';
      EFUN(plp_aperta_blackhawk_tsc_INTERNAL_compute_hex(sa__, bin, &hex));
      patt_gen_wr_val = (uint16_t)((patt_gen_wr_val << 4) | hex);
    }
    /* EFUN_PRINTF(("patt_gen_wr_val[%d] = 0x%x\n",(14-i),patt_gen_wr_val));    */

    /* Writing to apprpriate patt_gen_seq Registers */
    switch (i) {
      case 0:  EFUN(wrc_patt_gen_seq_14(patt_gen_wr_val));
               break;
      case 1:  EFUN(wrc_patt_gen_seq_13(patt_gen_wr_val));
               break;
      case 2:  EFUN(wrc_patt_gen_seq_12(patt_gen_wr_val));
               break;
      case 3:  EFUN(wrc_patt_gen_seq_11(patt_gen_wr_val));
               break;
      case 4:  EFUN(wrc_patt_gen_seq_10(patt_gen_wr_val));
               break;
      case 5:  EFUN(wrc_patt_gen_seq_9(patt_gen_wr_val));
               break;
      case 6:  EFUN(wrc_patt_gen_seq_8(patt_gen_wr_val));
               break;
      case 7:  EFUN(wrc_patt_gen_seq_7(patt_gen_wr_val));
               break;
      case 8:  EFUN(wrc_patt_gen_seq_6(patt_gen_wr_val));
               break;
      case 9:  EFUN(wrc_patt_gen_seq_5(patt_gen_wr_val));
               break;
      case 10: EFUN(wrc_patt_gen_seq_4(patt_gen_wr_val));
               break;
      case 11: EFUN(wrc_patt_gen_seq_3(patt_gen_wr_val));
               break;
      case 12: EFUN(wrc_patt_gen_seq_2(patt_gen_wr_val));
               break;
      case 13: EFUN(wrc_patt_gen_seq_1(patt_gen_wr_val));
               break;
      case 14: EFUN(wrc_patt_gen_seq_0(patt_gen_wr_val));
               break;
      default: EFUN_PRINTF(("ERROR: Invalid write to patt_gen_seq register\n"));
               return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_INVALID_SEQ_WRITE));
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
err_code_t plp_aperta_blackhawk_tsc_tx_shared_patt_gen_en(srds_access_t *sa__, uint8_t enable, uint8_t patt_length) {
  uint8_t zero_pad_len = 0; /* suppress warnings, changed by plp_aperta_blackhawk_tsc_INTERNAL_calc_patt_gen_mode_sel() */
  uint8_t mode_sel     = 0; /* suppress warnings, changed by plp_aperta_blackhawk_tsc_INTERNAL_calc_patt_gen_mode_sel() */

  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_calc_patt_gen_mode_sel(sa__, &mode_sel,&zero_pad_len,patt_length));

  if (enable) {
    if ((mode_sel < 1) || (mode_sel > 6)) {
      return (blackhawk_tsc_error(sa__, ERR_CODE_PATT_GEN_INVALID_MODE_SEL));
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
err_code_t plp_aperta_blackhawk_tsc_config_tx_jp03b_pattern(srds_access_t *sa__, uint8_t enable) {

  if (enable) {
    /* JP03B Pattern - "p0330" => 8'b_0011_1100 or 8'h3C w/o  Gray coding */
    /*                         => 8'b_0010_1000 or 8'h28 with Gray coding */   
    EFUN(wrc_patt_gen_seq_1(0x2800));     
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
err_code_t plp_aperta_blackhawk_tsc_config_tx_linearity_pattern(srds_access_t *sa__, uint8_t enable) {

  if (enable) {
    /* Linearity Pattern - "p0123_0303_21" => 20'b_0001_1011_0011_0011_1001 or 20'h1B339 w/o  Gray coding */
    /*                                     => 20'b_0001_1110_0010_0010_1101 or 20'h1E22D with Gray coding */   
    EFUN(wrc_patt_gen_seq_0(0xE22D));   
    EFUN(wrc_patt_gen_seq_1(0x0001));
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
err_code_t plp_aperta_blackhawk_tsc_config_tx_prbs(srds_access_t *sa__, enum plp_aperta_srds_prbs_polynomial_enum prbs_poly_mode, uint8_t prbs_inv) {
    uint8_t poly_mode_lsb;
    poly_mode_lsb = prbs_poly_mode & 0x7;

  EFUN(wr_prbs_gen_mode_sel((uint8_t)poly_mode_lsb));   /* PRBS Generator mode sel */
  EFUN(wr_prbs_gen_mode_sel_msb(prbs_poly_mode > 7));   /* Set MSB for new Poly mode selections */
  EFUN(wr_prbs_gen_inv(prbs_inv));                      /* PRBS Invert Enable/Disable */
  /* To enable PRBS Generator */
  /* EFUN(wr_prbs_gen_en(0x1)); */
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_get_tx_prbs_config(srds_access_t *sa__, enum plp_aperta_srds_prbs_polynomial_enum *prbs_poly_mode, uint8_t *prbs_inv) {
    uint8_t val;

  ESTM(val = rd_prbs_gen_mode_sel());                   /* PRBS Generator mode sel */
  ESTM(val = (uint8_t)(val + (rd_prbs_gen_mode_sel_msb()?8:0)));        /* Add the MSB for mode_sel */
  *prbs_poly_mode = (enum plp_aperta_srds_prbs_polynomial_enum)val;
  ESTM(val = rd_prbs_gen_inv());                        /* PRBS Invert Enable/Disable */
  *prbs_inv = val;

  return (ERR_CODE_NONE);
}

/* PRBS Generator Enable/Disable */
err_code_t plp_aperta_blackhawk_tsc_tx_prbs_en(srds_access_t *sa__, uint8_t enable) {

  if (enable) {
    EFUN(wr_prbs_gen_en(0x1));                          /* Enable PRBS Generator */
  }
  else {
    EFUN(wr_prbs_gen_en(0x0));                          /* Disable PRBS Generator */
  }
  return (ERR_CODE_NONE);
}

/* Get PRBS Generator Enable/Disable */
err_code_t plp_aperta_blackhawk_tsc_get_tx_prbs_en(srds_access_t *sa__, uint8_t *enable) {

  ESTM(*enable = rd_prbs_gen_en());

  return (ERR_CODE_NONE);
}

/* PRBS 1-bit error injection */
err_code_t plp_aperta_blackhawk_tsc_tx_prbs_err_inject(srds_access_t *sa__, uint8_t enable) {
  /* PRBS Error Insert.
     0 to 1 transition on this signal will insert single bit error in the MSB bit of the data bus.
     Reset value is 0x0.
  */
  if(enable)
    EFUN(wr_prbs_gen_err_ins(0x1));
  EFUN(wr_prbs_gen_err_ins(0));
  return (ERR_CODE_NONE);
}

/* Configure PRBS Checker */
err_code_t plp_aperta_blackhawk_tsc_config_rx_prbs(srds_access_t *sa__, enum plp_aperta_srds_prbs_polynomial_enum prbs_poly_mode, enum plp_aperta_srds_prbs_checker_mode_enum prbs_checker_mode, uint8_t prbs_inv) {
    uint8_t dig_lpbk = 0;
  if (prbs_poly_mode == PRBS_AUTO_DETECT) {
    EFUN(wr_prbs_chk_auto_detect_en(0x1));               /* Enable PRBS checker Auto-Detect */
    EFUN(wr_prbs_chk_mode(PRBS_INITIAL_SEED_HYSTERESIS));  /* PRBS Checker mode sel (PRBS LOCK state machine select) */
  }
  else {
    uint8_t poly_mode_lsb;
    poly_mode_lsb = prbs_poly_mode & 0x7;

    EFUN(wr_prbs_chk_mode_sel((uint8_t)poly_mode_lsb));  /* PRBS Checker Polynomial mode sel  */
    EFUN(wr_prbs_chk_mode((uint8_t)prbs_checker_mode));  /* PRBS Checker mode sel (PRBS LOCK state machine select) */
    ESTM(dig_lpbk = rd_dig_lpbk_en());
    if(dig_lpbk == 0) {
        /* Only enable auto mode in non-digital loop-back mode */
        EFUN(wr_prbs_chk_en_auto_mode(0x1));                 /* PRBS Checker enable control - rx_dsc_lock & prbs_chk_en */
    }
    EFUN(wr_prbs_chk_inv(prbs_inv));                     /* PRBS Invert Enable/Disable */
    EFUN(wr_prbs_chk_mode_sel_msb(prbs_poly_mode > 7));  /* Set MSB for new Poly mode selections */
    EFUN(wr_prbs_chk_auto_detect_en(0x0));               /* Disable PRBS checker Auto-Detect */
  }
  /* To enable PRBS Checker */
  /* EFUN(wr_prbs_chk_en(0x1)); */
  return (ERR_CODE_NONE);
}

/* get PRBS Checker */
err_code_t plp_aperta_blackhawk_tsc_get_rx_prbs_config(srds_access_t *sa__, enum plp_aperta_srds_prbs_polynomial_enum *prbs_poly_mode, enum plp_aperta_srds_prbs_checker_mode_enum *prbs_checker_mode, uint8_t *prbs_inv) {
  uint8_t val;

  uint8_t auto_det_en;
  ESTM(auto_det_en = rd_prbs_chk_auto_detect_en());       /* PRBS checker Auto-Detect Enable */
  if (auto_det_en) {
    uint8_t auto_det_lock;
    ESTM(auto_det_lock = rd_prbs_chk_auto_detect_lock()); /* PRBS checker Auto-Detect Lock */
    if (auto_det_lock) {
      ESTM(val = rd_prbs_chk_mode_sel_auto_detect());     /* PRBS Auto-Detect Checker Polynomial mode sel  */
      ESTM(val = (uint8_t)(val + (rd_prbs_chk_mode_sel_msb_auto_detect()? 8:0)));  /* Add the MSB for Auto-Detect mode_sel */
      *prbs_poly_mode = (enum plp_aperta_srds_prbs_polynomial_enum)val;

      ESTM(val = rd_prbs_chk_mode());                     /* PRBS Checker mode sel (PRBS LOCK state machine select) */
      *prbs_checker_mode = (enum plp_aperta_srds_prbs_checker_mode_enum)val;
      ESTM(val = rd_prbs_chk_inv_auto_detect());          /* PRBS Invert Enable/Disable */
      *prbs_inv = val;
    }
    else {
      *prbs_poly_mode = PRBS_UNKNOWN;
      *prbs_inv = 0;
      ESTM(val = rd_prbs_chk_mode());                     /* PRBS Checker mode sel (PRBS LOCK state machine select) */
      *prbs_checker_mode = (enum plp_aperta_srds_prbs_checker_mode_enum)val;      
    }
  }
  else {
    ESTM(val = rd_prbs_chk_mode_sel());                 /* PRBS Checker Polynomial mode sel  */
    ESTM(val = (uint8_t)(val + (rd_prbs_chk_mode_sel_msb()? 8:0)));     /* Add the MSB for mode_sel */
    *prbs_poly_mode = (enum plp_aperta_srds_prbs_polynomial_enum)val;

    ESTM(val = rd_prbs_chk_mode());                     /* PRBS Checker mode sel (PRBS LOCK state machine select) */
    *prbs_checker_mode = (enum plp_aperta_srds_prbs_checker_mode_enum)val;
    ESTM(val = rd_prbs_chk_inv());                      /* PRBS Invert Enable/Disable */
    *prbs_inv = val;
  }
  return (ERR_CODE_NONE);
}

/* PRBS Checker Enable/Disable */
err_code_t plp_aperta_blackhawk_tsc_rx_prbs_en(srds_access_t *sa__, uint8_t enable) {

    if (enable) {
        EFUN(wr_prbs_chk_en(0x1));                          /* Enable PRBS Checker */
    }
    else {
        EFUN(wr_prbs_chk_en(0x0));                          /* Disable PRBS Checker */
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_get_rx_prbs_en(srds_access_t *sa__, uint8_t *enable) {

    ESTM(*enable = rd_prbs_chk_en());
    return (ERR_CODE_NONE);
}


/* PRBS Checker Lock State */
err_code_t plp_aperta_blackhawk_tsc_prbs_chk_lock_state(srds_access_t *sa__, uint8_t *chk_lock) {
    if(!chk_lock) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ESTM(*chk_lock = rd_prbs_chk_lock());                  /* PRBS Checker Lock Indication 1 = Locked, 0 = Out of Lock */
    return (ERR_CODE_NONE);
}

/* PRBS Error Count and Lock Lost (bit 31 in lock lost) */
err_code_t plp_aperta_blackhawk_tsc_prbs_err_count_ll(srds_access_t *sa__, uint32_t *prbs_err_cnt) {
    uint16_t rddata;

    if(!prbs_err_cnt) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    ESTM(rddata = REG_RD_TLB_RX_PRBS_CHK_ERR_CNT_MSB_STATUS());
    *prbs_err_cnt = ((uint32_t) rddata)<<16;
    ESTM(*prbs_err_cnt = (*prbs_err_cnt | rd_prbs_chk_err_cnt_lsb()));


    return (ERR_CODE_NONE);
}

/* PRBS Error Count State  */
err_code_t plp_aperta_blackhawk_tsc_prbs_err_count_state(srds_access_t *sa__, uint32_t *prbs_err_cnt, uint8_t *lock_lost) {
    uint8_t dig_lpbk_enable = 0;
    uint8_t link_training_enable = 0;
    if(!prbs_err_cnt || !lock_lost) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    EFUN(plp_aperta_blackhawk_tsc_prbs_err_count_ll(sa__, prbs_err_cnt));
    *lock_lost    = (uint8_t)(*prbs_err_cnt >> 31);
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
err_code_t plp_aperta_blackhawk_tsc_get_prbs_chk_hw_timer_ctrl(srds_access_t *sa__, struct blackhawk_tsc_prbs_chk_hw_timer_ctrl_st * const prbs_chk_hw_timer_ctrl_bak) {
    ESTM(prbs_chk_hw_timer_ctrl_bak->prbs_chk_burst_err_cnt_en = rd_prbs_chk_burst_err_cnt_en());
    ESTM(prbs_chk_hw_timer_ctrl_bak->prbs_chk_en_timer_mode = rd_prbs_chk_en_timer_mode());
    ESTM(prbs_chk_hw_timer_ctrl_bak->prbs_chk_en_timeout = rd_prbs_chk_en_timeout());
    return ERR_CODE_NONE;
}

/* Restore prbs hardware timer config registers */
err_code_t plp_aperta_blackhawk_tsc_set_prbs_chk_hw_timer_ctrl(srds_access_t *sa__, struct blackhawk_tsc_prbs_chk_hw_timer_ctrl_st const * const prbs_chk_hw_timer_ctrl_bak) {
    EFUN(wr_prbs_chk_burst_err_cnt_en(prbs_chk_hw_timer_ctrl_bak->prbs_chk_burst_err_cnt_en));
    EFUN(wr_prbs_chk_en_timer_mode(prbs_chk_hw_timer_ctrl_bak->prbs_chk_en_timer_mode));
    EFUN(wr_prbs_chk_en_timeout(prbs_chk_hw_timer_ctrl_bak->prbs_chk_en_timeout));
    return ERR_CODE_NONE;
}

/* Configure hardware timer to count prbs errors */
err_code_t plp_aperta_blackhawk_tsc_config_prbs_chk_hw_timer(srds_access_t *sa__, uint16_t time_ms, uint16_t *time_ms_adjusted) {

    struct blackhawk_tsc_prbs_chk_hw_timer_ctrl_st prbs_chk_hw_timer_ctrl;
    USR_MEMSET(&prbs_chk_hw_timer_ctrl, 0, sizeof(struct blackhawk_tsc_prbs_chk_hw_timer_ctrl_st));

    EFUN(wr_prbs_chk_burst_err_cnt_en(0));

    /* Figure out timer mode and timer count */
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_prbs_timeout_count_from_time(time_ms, time_ms_adjusted, &prbs_chk_hw_timer_ctrl));
        
    EFUN(wr_prbs_chk_en_timer_mode(prbs_chk_hw_timer_ctrl.prbs_chk_en_timer_mode));
    EFUN(wr_prbs_chk_en_timeout(prbs_chk_hw_timer_ctrl.prbs_chk_en_timeout));

    return ERR_CODE_NONE;
}

/* toggle checker enable to start hardware timers and error counters */
err_code_t plp_aperta_blackhawk_tsc_prbs_chk_en_toggle(srds_access_t *sa__) {
    EFUN(wr_prbs_chk_en(0));
    EFUN(wr_prbs_chk_en(1));
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_display_detailed_prbs_state_hdr(void) {
    EFUN_PRINTF(("\nPRBS DETAILED DISPLAY :\n"));

/* Comment out display of PRBS burst error if BLACKHAWK */
    EFUN_PRINTF((" LN TX-Mode TX-PRBS-Inv TX-PMD-Inv RX-Mode RX-PRBS-Inv RX-PMD-Inv Lck LL PRBS-Err-Cnt"));
    EFUN_PRINTF(("     BER\n"));
    return (ERR_CODE_NONE);
}

const char* plp_aperta_blackhawk_tsc_e2s_prbs_mode_enum[17] = {
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
    "  ERR  ",
    "UNKNOWN"
  };

err_code_t plp_aperta_blackhawk_tsc_display_detailed_prbs_state(srds_access_t *sa__) {
    uint32_t err_cnt = 0;
    uint8_t lock_lost = 0;
    uint8_t enabled;

    ESTM_PRINTF(("  %d ",plp_aperta_blackhawk_tsc_get_lane(sa__)));

    ESTM(enabled = rd_prbs_gen_en());
    if(enabled) {
        enum plp_aperta_srds_prbs_polynomial_enum prbs_poly_mode = PRBS_7;
        uint8_t prbs_inv = 0;
        char *prbs_string;
        EFUN(plp_aperta_blackhawk_tsc_get_tx_prbs_config(sa__, &prbs_poly_mode, &prbs_inv));
        prbs_string = (char *)plp_aperta_blackhawk_tsc_e2s_prbs_mode_enum[prbs_poly_mode];
        ESTM_PRINTF(("%s",prbs_string));
        ESTM_PRINTF(("      %1d     ",prbs_inv));
    } else {
        EFUN_PRINTF(("  OFF  "));
        ESTM_PRINTF(("      -     "));
    }
    ESTM_PRINTF(("     %1d     ",rd_tx_pmd_dp_invert()));

    ESTM(enabled = rd_prbs_chk_en());
    if(enabled) {
        enum plp_aperta_srds_prbs_polynomial_enum prbs_poly_mode = PRBS_7;
        enum plp_aperta_srds_prbs_checker_mode_enum prbs_checker_mode;
        uint8_t prbs_inv = 0;
        char *prbs_string;
        EFUN(plp_aperta_blackhawk_tsc_get_rx_prbs_config(sa__, &prbs_poly_mode, &prbs_checker_mode, &prbs_inv));
        prbs_string = (char *)plp_aperta_blackhawk_tsc_e2s_prbs_mode_enum[prbs_poly_mode];
        ESTM_PRINTF((" %s",prbs_string));
        ESTM_PRINTF(("      %1d     ",prbs_inv));
    } else {
        EFUN_PRINTF(("   OFF  "));
        ESTM_PRINTF(("      -     "));
    }

    ESTM_PRINTF(("     %1d     ",rd_rx_pmd_dp_invert()));
    ESTM_PRINTF(("  %d ",rd_prbs_chk_lock()));
    EFUN(plp_aperta_blackhawk_tsc_prbs_err_count_state(sa__,&err_cnt,&lock_lost));
    EFUN_PRINTF(("  %d  %010d ",lock_lost,err_cnt));
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_display_BER(sa__,100));
    EFUN_PRINTF(("\n"));

    return (ERR_CODE_NONE);
}


/***********************************/
/*  PRBS Error Analyzer Functions  */
/***********************************/

/* Configure PRBS Error Analyzer */ 
err_code_t plp_aperta_blackhawk_tsc_prbs_error_analyzer_config(srds_access_t *sa__, uint16_t prbs_err_fec_size, uint8_t prbs_errcnt_thresh, uint8_t hist_errcnt_thresh) {
    uint8_t  fec_size;
    uint16_t fec_size_bits;

    if ((prbs_err_fec_size < BLACKHAWK_TSC_PRBS_ERR_ANALYZER_FEC_SIZE_MIN) || (prbs_err_fec_size > BLACKHAWK_TSC_PRBS_ERR_ANALYZER_FEC_SIZE_MAX)) { 
        return (blackhawk_tsc_error(sa__, ERR_CODE_INVALID_PRBS_ERR_ANALYZER_FEC_SIZE));
    }
    if ((prbs_errcnt_thresh < BLACKHAWK_TSC_PRBS_ERR_ANALYZER_ERR_THRESH_MIN) || (prbs_errcnt_thresh > BLACKHAWK_TSC_PRBS_ERR_ANALYZER_ERR_THRESH_MAX)) {
        return (blackhawk_tsc_error(sa__, ERR_CODE_INVALID_PRBS_ERR_ANALYZER_ERR_THRESH));
    }
    if ((hist_errcnt_thresh < BLACKHAWK_TSC_PRBS_ERR_ANALYZER_HIST_ERR_THRESH_MIN) || (hist_errcnt_thresh > BLACKHAWK_TSC_PRBS_ERR_ANALYZER_HIST_ERR_THRESH_MAX)) {
        return (blackhawk_tsc_error(sa__, ERR_CODE_INVALID_PRBS_ERR_ANALYZER_HIST_ERR_THRESH));
    }

    fec_size = (uint8_t)((prbs_err_fec_size + BLACKHAWK_TSC_PRBS_FEC_FRAME_SIZE_ROUND_UP) / BLACKHAWK_TSC_PRBS_FEC_FRAME_SIZE_MULTIPLIER);

    fec_size_bits = (uint16_t)(fec_size * BLACKHAWK_TSC_PRBS_FEC_FRAME_SIZE_MULTIPLIER);
    if (prbs_err_fec_size != fec_size_bits) {
        EFUN_PRINTF(("\n << WARNING: FEC Frame size of %d bits NOT programmable. Instead setting frame size to %d bits for Lane %d >>\n", prbs_err_fec_size, fec_size_bits, plp_aperta_blackhawk_tsc_get_lane(sa__)));
    }

    EFUN(wr_tlb_err_fec_size(fec_size));
    EFUN(wr_tlb_err_error_count_thresh(prbs_errcnt_thresh));
    EFUN(wr_tlb_err_histogram_error_thresh(hist_errcnt_thresh));
    EFUN(wr_tlb_err_clear_error_analyzer_status(1));                                         /* Clear all the error analyzer status registers */
    return (ERR_CODE_NONE);
}

/* Get PRBS Error Analyzer Config */
err_code_t plp_aperta_blackhawk_tsc_get_prbs_error_analyzer_config(srds_access_t *sa__, uint16_t *prbs_err_fec_size, uint8_t *prbs_errcnt_thresh, uint8_t *hist_errcnt_thresh) {

    if(!prbs_err_fec_size || !prbs_errcnt_thresh || !hist_errcnt_thresh) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ESTM(*prbs_err_fec_size  = rd_tlb_err_fec_size());
    *prbs_err_fec_size       = (uint16_t)(*prbs_err_fec_size * BLACKHAWK_TSC_PRBS_FEC_FRAME_SIZE_MULTIPLIER);
    ESTM(*prbs_errcnt_thresh = rd_tlb_err_error_count_thresh());
    ESTM(*hist_errcnt_thresh = rd_tlb_err_histogram_error_thresh());
    return (ERR_CODE_NONE);
}

/* Display PRBS Error Analyzer Config */
err_code_t plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_config(srds_access_t *sa__, uint8_t prbs_errcnt_thresh, uint8_t hist_errcnt_thresh, uint32_t timeout_s) {

    uint8_t  fec_size;
    uint16_t fec_size_bits;

    ESTM(fec_size = (uint8_t)rd_tlb_err_fec_size());
    fec_size_bits = (uint16_t)(fec_size * BLACKHAWK_TSC_PRBS_FEC_FRAME_SIZE_MULTIPLIER);

    EFUN_PRINTF(("\n ****************************************************************************** \n"));
    EFUN_PRINTF(("  PRBS Error Analyzer Config:\n"));
    EFUN_PRINTF((" ------------------------------------------------------------- \n"));
    EFUN_PRINTF(("    FEC Frame Size               = %4d bits\n",fec_size_bits));
    EFUN_PRINTF(("    PRBS Max Err Threshold (Max) = %4d errors per frame\n",prbs_errcnt_thresh));
    EFUN_PRINTF(("    PRBS Histogram Start (E)     = %4d errors per frame\n",hist_errcnt_thresh));
    EFUN_PRINTF(("    Time Duration of Analysis    = %4d seconds\n",timeout_s));
    EFUN_PRINTF((" -------------------------------------------------------------\n"));

    return (ERR_CODE_NONE);
}

/* Get PRBS Error Analyzer Error Count */
err_code_t plp_aperta_blackhawk_tsc_prbs_error_analyzer_err_count(srds_access_t *sa__, blackhawk_tsc_prbs_err_analyzer_errcnt_st *err_analyzer) {
    uint16_t err_cnt;
    uint8_t  prbs_lock;
    uint8_t   i;

    if(!err_analyzer) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    
    ESTM(err_analyzer->hist_errcnt_thresh = rd_tlb_err_histogram_error_thresh());
    ESTM(err_analyzer->prbs_errcnt_thresh = rd_tlb_err_error_count_thresh());

    ESTM(prbs_lock = rd_prbs_chk_lock());
    if(!prbs_lock) {
        USR_PRINTF(("\nERROR : PRBS Checker is not locked for core %d Lane %d\n",plp_aperta_blackhawk_tsc_get_core(sa__),plp_aperta_blackhawk_tsc_get_lane(sa__)));
        for (i=0; i<9; i++) {
            err_analyzer->prbs_errcnt[i] = 0xFFFFFFFF;
        }
    } 
    else {
        uint32_t frame_errors[9]; 
        uint32_t temp32;

        /* Reading raw data from FEC err_count registers */
        ESTM(frame_errors[8] = rd_tlb_err_count_error_lo());
        ESTM(err_cnt = rd_tlb_err_count_error_hi());
        frame_errors[8] = (frame_errors[8] | (((uint32_t) err_cnt)<<16));

        ESTM(frame_errors[7] = rd_tlb_err_count_histogram_error_offset_7());
        ESTM(frame_errors[6] = rd_tlb_err_count_histogram_error_offset_6());
        ESTM(frame_errors[5] = rd_tlb_err_count_histogram_error_offset_5());
        ESTM(frame_errors[4] = rd_tlb_err_count_histogram_error_offset_4());
        ESTM(frame_errors[3] = rd_tlb_err_count_histogram_error_offset_3());
        ESTM(frame_errors[2] = rd_tlb_err_count_histogram_error_offset_2());
        ESTM(frame_errors[1] = rd_tlb_err_count_histogram_error_offset_1());
        ESTM(frame_errors[0] = rd_tlb_err_count_histogram_error_offset_0());

        if (PRBS_VERBOSE > 2) {
            EFUN_PRINTF(("\n\n\t=====> DEBUG INFO (start) [Core = %d, Lane = %d]\n", plp_aperta_blackhawk_tsc_get_core(sa__), plp_aperta_blackhawk_tsc_get_lane(sa__)));
            EFUN_PRINTF(("\n\t << Number of Frame Errors as read from the register >>\n"));
            for (i=0; i<9; i++) {
                if (i < 8) {
                    EFUN_PRINTF(("\t FEC frames with   %2d Errors = %u\n",(i+err_analyzer->hist_errcnt_thresh),frame_errors[i]));
                }
                else {
                    EFUN_PRINTF(("\t FEC frames with > %2d Errors = %u\n",err_analyzer->prbs_errcnt_thresh-1,frame_errors[i]));
                }
            }
        }

        /* Calculating cumulative error count for respective values of "t" */ 
        for (i=9; i > 0; i--) {
            if ((frame_errors[i-1] != 0xFFFF) && (frame_errors[i-1] != 0xFFFFF)) {
                if ((i-1)!=8) {
                    if (frame_errors[i] == 0xFFFFFFFF) {
                        frame_errors[i-1] = 0xFFFFFFFF;
                    }
                    else {
                        frame_errors[i-1] = (frame_errors[i-1] + frame_errors[i]);
                    }
                }
            }
            else {

                if (PRBS_VERBOSE > 2) {
                    EFUN_PRINTF(("\t frame_errors[%d] == MAX\n", i-1));
                }

                frame_errors[i-1] = 0xFFFFFFFF;
            }
        }

        if (PRBS_VERBOSE > 2) {
            EFUN_PRINTF(("\n\t=====> DEBUG INFO (end)\n\n"));
        }

        /* Accumulating error counts and assigning to struct */
        for (i=0; i<9; i++) {
            temp32 = 0xFFFFFFFF - frame_errors[i];
            if ((frame_errors[i] == 0xFFFFFFFF) || (temp32 <= err_analyzer->prbs_errcnt[i]))
                err_analyzer->prbs_errcnt[i] = 0xFFFFFFFF;
            else
                err_analyzer->prbs_errcnt[i] = (frame_errors[i] + err_analyzer->prbs_errcnt[i]);
        }
    }
    return (ERR_CODE_NONE);
}

/* Get PRBS Error Analyzer Accumulate Error Count and Clear error counters */
err_code_t plp_aperta_blackhawk_tsc_prbs_error_analyzer_accumulate_err_count(srds_access_t *sa__, blackhawk_tsc_prbs_err_analyzer_errcnt_st *err_analyzer) {

    EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_err_count(sa__, err_analyzer));
    return (ERR_CODE_NONE);
}

static err_code_t _plp_aperta_blackhawk_tsc_display_fec_frame_error(uint8_t errs_per_frame, uint32_t num_frame_errs) {
    if (num_frame_errs == 0xFFFFFFFF) {
        EFUN_PRINTF(("    FEC Frames with > %2d Errors (t=%2d) =        MAX \n",errs_per_frame,errs_per_frame));
    }
    else {
        EFUN_PRINTF(("    FEC Frames with > %2d Errors (t=%2d) = %10u \n",errs_per_frame,errs_per_frame,num_frame_errs));
    }
    return (ERR_CODE_NONE);
}

/* Optimize histogram error count threshold for PRBS Error Analyzer */
err_code_t plp_aperta_blackhawk_tsc_optimize_hist_errcnt_thresh(srds_access_t *sa__, blackhawk_tsc_prbs_err_analyzer_errcnt_st *err_analyzer, uint32_t timeout_check, uint32_t timeout_s) {
    uint8_t  i = 0;
    uint8_t errcnt_sat = 0;
#ifdef PHYMOD_SUPPORT
    uint64_t tmp_err, tmp_sat;
#endif /* PHYMOD_SUPPORT */

    if((timeout_check == 0) || timeout_s == 0) {
        USR_PRINTF(("\nERROR : timeout value cannot be 0 for RX lane %d\n",plp_aperta_blackhawk_tsc_get_lane(sa__)));
    }
    if(timeout_check > timeout_s) {
        USR_PRINTF(("\nERROR : timeout_check value has to be <= timeout_s value for RX lane %d\n",plp_aperta_blackhawk_tsc_get_lane(sa__)));
    }

    /* Read the current histogram settings*/
    ESTM(err_analyzer->hist_errcnt_thresh = rd_tlb_err_histogram_error_thresh());

    /* Extrapolate and check for saturation */
#ifdef PHYMOD_SUPPORT
    for(i = BLACKHAWK_TSC_PRBS_ERR_ANALYZER_NUM_COUNTERS; i>0 ; i--) {
        /*
         * 64-bit operations use COMPILER_64_XXX macros
         */
        /* tmp_err = (uint64_t)timeout_s * err_analyzer->prbs_errcnt[i] */
        COMPILER_64_SET(tmp_err, 0, (uint32_t)1);
        COMPILER_64_UMUL_32(tmp_err, timeout_s);
        COMPILER_64_UMUL_32(tmp_err, err_analyzer->prbs_errcnt[i-1]);

        /* tmp_sat = (uint64_t)timeout_check * 0xFFFF */
        COMPILER_64_SET(tmp_sat, 0, (uint32_t)1);
        COMPILER_64_UMUL_32(tmp_sat, timeout_check);
        COMPILER_64_UMUL_32(tmp_sat, 0xFFFF);

        if (COMPILER_64_GE(tmp_err, tmp_sat)) {
            errcnt_sat = 1;
            break;
        }
    }
#else
    for(i = BLACKHAWK_TSC_PRBS_ERR_ANALYZER_NUM_COUNTERS; i>0 ; i--) {
        if((uint64_t)timeout_s * err_analyzer->prbs_errcnt[i-1] > (uint64_t)timeout_check * (0xFFFF)) {
            errcnt_sat = 1;
            break;
        }
    }
#endif /* PHYMOD_SUPPORT */

    if((errcnt_sat == 1) && ((i-1) != 0)) {
        if(((i-1)+err_analyzer->hist_errcnt_thresh) >= BLACKHAWK_TSC_PRBS_ERR_ANALYZER_HIST_ERR_THRESH_MAX) {
            err_analyzer->hist_errcnt_thresh = BLACKHAWK_TSC_PRBS_ERR_ANALYZER_HIST_ERR_THRESH_MAX-1;
            err_analyzer->prbs_errcnt_thresh = BLACKHAWK_TSC_PRBS_ERR_ANALYZER_ERR_THRESH_MAX;
            EFUN_PRINTF(("\tLane %d: Setting the histogram error count threshold to maximum value = %d and PRBS error count threshold = %d\n \n ", plp_aperta_blackhawk_tsc_get_lane(sa__), err_analyzer->hist_errcnt_thresh, BLACKHAWK_TSC_PRBS_ERR_ANALYZER_ERR_THRESH_MAX));
            
        } else {
            err_analyzer->hist_errcnt_thresh = (uint8_t)(err_analyzer->hist_errcnt_thresh + (i-1));
            err_analyzer->prbs_errcnt_thresh = (uint8_t)(err_analyzer->hist_errcnt_thresh + 8);
            EFUN_PRINTF(("\tLane %d: Optimum histogram error count threshold found\n", plp_aperta_blackhawk_tsc_get_lane(sa__)));
            USR_PRINTF(("           Changing histogram error count threshold to %d and PRBS error count threshold to %d\n \n ", err_analyzer->hist_errcnt_thresh, err_analyzer->prbs_errcnt_thresh));
        }
        /* Reconfigure */
        EFUN(wr_tlb_err_histogram_error_thresh(err_analyzer->hist_errcnt_thresh));
        EFUN(wr_tlb_err_error_count_thresh(err_analyzer->prbs_errcnt_thresh));
    } else if ((err_analyzer->prbs_errcnt[0] == 0) && (err_analyzer->hist_errcnt_thresh != BLACKHAWK_TSC_PRBS_ERR_ANALYZER_ERR_THRESH_MIN)) {
        EFUN_PRINTF(("\tLane %d: WARNING: Error counters are all 0 after %ds. Try reducing hist_errcnt_thresh or increasing timeout_check and rerun the projection\n \n ", plp_aperta_blackhawk_tsc_get_lane(sa__), timeout_check));
    } else if ((err_analyzer->prbs_errcnt[0] == 0) && (err_analyzer->hist_errcnt_thresh == BLACKHAWK_TSC_PRBS_ERR_ANALYZER_ERR_THRESH_MIN)) {
        EFUN_PRINTF(("\tLane %d: WARNING: Error counters are all 0 after %ds. Try increasing timeout_check and/or timeout and rerun the projection\n \n ", plp_aperta_blackhawk_tsc_get_lane(sa__), timeout_check));
    } else {
        EFUN_PRINTF(("\tLane %d: Current hist_errcnt_thresh is the optimized value. Value = %d is unchanged. \n \n ", plp_aperta_blackhawk_tsc_get_lane(sa__), err_analyzer->hist_errcnt_thresh));
    }

    /* Clear all the error analyzer status registers */
    EFUN(wr_tlb_err_clear_error_analyzer_status(1));

    return (ERR_CODE_NONE);
}

/* PRBS Error Analyzer Display */
err_code_t plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_err_count(srds_access_t *sa__, blackhawk_tsc_prbs_err_analyzer_errcnt_st err_analyzer) {
    uint8_t t, i;  

    t = (uint8_t)(err_analyzer.hist_errcnt_thresh - 1);                                      /* number of FEC correctable errors */

    /* Display PRBS Error Analyzer Err_Counts */
    EFUN_PRINTF(("\n -------------------------------------------------------------\n"));
    EFUN_PRINTF(("  PRBS Error Analyzer Error_Counts:\n"));
    EFUN_PRINTF((" -------------------------------------------------------------\n"));
    for (i=0; i<8; i++) {
        EFUN(_plp_aperta_blackhawk_tsc_display_fec_frame_error((uint8_t)(t+i), err_analyzer.prbs_errcnt[i]));
    }
    EFUN(_plp_aperta_blackhawk_tsc_display_fec_frame_error((uint8_t)(err_analyzer.prbs_errcnt_thresh-1), err_analyzer.prbs_errcnt[8]));
    EFUN_PRINTF((" -------------------------------------------------------------\n"));    

    return (ERR_CODE_NONE);
}


/* Display PRBS Error Analyzer Projection */
err_code_t plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_proj(srds_access_t *sa__, uint16_t prbs_err_fec_size, uint8_t hist_errcnt_thresh, uint32_t timeout_s) {
    uint8_t  hrs, mins, secs;
    uint8_t  prbs_lock;
    uint32_t time_remaining_s;
    uint8_t  i = 0;
    blackhawk_tsc_prbs_err_analyzer_errcnt_st err_analyzer;

    if (timeout_s == 0) {
        EFUN_PRINTF(("\nERROR: timeout_s value cannot be 0 for Lane %d >>\n", plp_aperta_blackhawk_tsc_get_lane(sa__)));
    }

    ESTM(prbs_lock = rd_prbs_chk_lock());
    if(!prbs_lock) {
        USR_PRINTF(("\nERROR : PRBS Checker is not locked for RX lane %d\n",plp_aperta_blackhawk_tsc_get_lane(sa__)));
    }
    else {
    USR_MEMSET(&err_analyzer, 0, sizeof(blackhawk_tsc_prbs_err_analyzer_errcnt_st));
        err_analyzer.prbs_errcnt_thresh = (uint8_t)(hist_errcnt_thresh + 8);     /* Since 20bit err_cnt register counts Frames with errors >= prbs_errcnt_thresh errors */
        if (err_analyzer.prbs_errcnt_thresh > 15) {
            err_analyzer.prbs_errcnt_thresh = 15;
        }
        err_analyzer.hist_errcnt_thresh = hist_errcnt_thresh;
        EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_config(sa__, prbs_err_fec_size, err_analyzer.prbs_errcnt_thresh, err_analyzer.hist_errcnt_thresh));  /* Configure PRBS Error Analyzer */
        EFUN(wr_tlb_err_clear_status_on_read(1));        /* Clear error status registers on read (TODO: verify functionality in production cores) */ 
        /* Clear the error counters for a fresh start */
        for (i = 0; i <= BLACKHAWK_TSC_PRBS_ERR_ANALYZER_NUM_COUNTERS; i++) {
           err_analyzer.prbs_errcnt[i] = 0;
        }

        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_seconds_to_displayformat(timeout_s, &hrs, &mins, &secs)); 
        EFUN_PRINTF((" \n Waiting for PRBS Error Analyzer measurement: time approx %d seconds (%d hr:%d mins: %ds) ",timeout_s,hrs,mins,secs));

        /* Wait for required duration and accumulate errors */
        time_remaining_s = timeout_s;
        while (time_remaining_s > 0) {
            if (time_remaining_s > 5) {
                EFUN(USR_DELAY_MS(5000));
                EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_accumulate_err_count(sa__, &err_analyzer));
                EFUN_PRINTF(("."));
                time_remaining_s = time_remaining_s - 5;
            }
            else {
                EFUN(USR_DELAY_MS((((uint32_t)time_remaining_s) * 1000)));                                                      
                EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_accumulate_err_count(sa__, &err_analyzer));
                EFUN_PRINTF(("\n"));
                time_remaining_s = 0;
            }
        }
                                                    
        /* Display PRBS Error Analyzer Config */
        EFUN(plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_config(sa__, err_analyzer.prbs_errcnt_thresh, err_analyzer.hist_errcnt_thresh, timeout_s));   

        /* Display PRBS Error Analyzer Err_Counts */
        EFUN(plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_err_count(sa__, err_analyzer));

        /* Compute and display PRBS Error Analyzer Projection */
        EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_compute_proj(sa__, err_analyzer, timeout_s));
    }
    return (ERR_CODE_NONE);
}

/* Display PRBS Error Analyzer Projection with automatic threshold adjusting */
err_code_t plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_proj_auto_threshold(srds_access_t *sa__, uint16_t prbs_err_fec_size, uint32_t timeout_s) {
    uint8_t  hrs, mins, secs;
    uint8_t  prbs_lock;
    uint32_t time_remaining_s, timeout_frac;
    uint8_t  i = 0;
    blackhawk_tsc_prbs_err_analyzer_errcnt_st err_analyzer;

    if (timeout_s == 0) {
        EFUN_PRINTF(("\nERROR: timeout_s value cannot be 0 for Lane %d >>\n", plp_aperta_blackhawk_tsc_get_lane(sa__)));
    }

    ESTM(prbs_lock = rd_prbs_chk_lock());
    if(!prbs_lock) {
        USR_PRINTF(("\nERROR : PRBS Checker is not locked for RX lane %d\n",plp_aperta_blackhawk_tsc_get_lane(sa__)));
    }
    else {
        USR_MEMSET(&err_analyzer, 0, sizeof(blackhawk_tsc_prbs_err_analyzer_errcnt_st));
        err_analyzer.hist_errcnt_thresh = BLACKHAWK_TSC_PRBS_ERR_ANALYZER_HIST_ERR_THRESH_MIN;
        err_analyzer.prbs_errcnt_thresh = (uint8_t)(err_analyzer.hist_errcnt_thresh + 8);     /* Since 20bit err_cnt register counts Frames with errors >= prbs_errcnt_thresh errors */

        EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_config(sa__, prbs_err_fec_size, err_analyzer.prbs_errcnt_thresh, err_analyzer.hist_errcnt_thresh));  /* Configure PRBS Error Analyzer */

        EFUN(wr_tlb_err_clear_status_on_read(1));        /* Clear error status registers on read (TODO: verify functionality in production cores) */ 

        timeout_frac = (5*timeout_s+99)/100; /* 99 is used to generate CEIL function of 5% of timeout_s */
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_seconds_to_displayformat(timeout_frac, &hrs, &mins, &secs));
        EFUN_PRINTF(("\n \n Running a quick PRBS Error Analyzer measurement : time approx %d seconds (%d hr:%d mins: %ds) to check if any error counters would saturate\n \n ",timeout_frac,hrs,mins,secs));

        /* Run a quick test (5% of timeout_s) and optimize the hist_errcnt_thresh */
        EFUN(USR_DELAY_MS(1000*timeout_frac));
        EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_accumulate_err_count(sa__, &err_analyzer));
        EFUN(plp_aperta_blackhawk_tsc_optimize_hist_errcnt_thresh(sa__, &err_analyzer, timeout_frac, timeout_s));

        /* Clear the error counters for a fresh start */
        for (i = 0; i <= BLACKHAWK_TSC_PRBS_ERR_ANALYZER_NUM_COUNTERS; i++) {
           err_analyzer.prbs_errcnt[i] = 0;
        }

        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_seconds_to_displayformat(timeout_s, &hrs, &mins, &secs)); 
        EFUN_PRINTF((" \n Waiting for PRBS Error Analyzer measurement: time approx %d seconds (%d hr:%d mins: %ds) ",timeout_s,hrs,mins,secs));

        /* Wait for required duration and accumulate errors */
        time_remaining_s = timeout_s;
        while (time_remaining_s > 0) {
            if (time_remaining_s > 5) {
                EFUN(USR_DELAY_MS(5000));
                EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_accumulate_err_count(sa__, &err_analyzer));
                EFUN_PRINTF(("."));
                time_remaining_s = time_remaining_s - 5;
            }
            else {
                EFUN(USR_DELAY_MS(((uint32_t)time_remaining_s) * 1000));                                                      
                EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_accumulate_err_count(sa__, &err_analyzer));
                EFUN_PRINTF(("\n"));
                time_remaining_s = 0;
            }
        }
                                                    
        /* Display PRBS Error Analyzer Config */
        EFUN(plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_config(sa__, err_analyzer.prbs_errcnt_thresh, err_analyzer.hist_errcnt_thresh, timeout_s));   

        /* Display PRBS Error Analyzer Err_Counts */
        EFUN(plp_aperta_blackhawk_tsc_display_prbs_error_analyzer_err_count(sa__, err_analyzer));

        /* Compute and display PRBS Error Analyzer Projection */
        EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_compute_proj(sa__, err_analyzer, timeout_s));
    }
    return (ERR_CODE_NONE);
}


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
/*  ii) Need atleast 2 valid data points to generated projected data                                              */
/*----------------------------------------------------------------------------------------------------------------*/
err_code_t plp_aperta_blackhawk_tsc_prbs_error_analyzer_report_proj(srds_access_t *sa__, blackhawk_tsc_prbs_err_analyzer_errcnt_st err_analyzer, uint32_t timeout_s, blackhawk_tsc_prbs_err_analyzer_report_st *err_analyzer_report) {
#ifdef SERDES_API_FLOATING_POINT
    uint8_t    t, delta_n;
    uint8_t     i;
    uint8_t    start_idx, stop_idx;
    uint32_t   num_bits_per_ms;
    USR_DOUBLE meas_fec_ber[9];
    USR_DOUBLE proj_ber[16];
    USR_DOUBLE x, y;
    USR_DOUBLE Exy = 0.0;
    USR_DOUBLE Eyy = 0.0;
    USR_DOUBLE Exx = 0.0;
    USR_DOUBLE Ey  = 0.0;
    USR_DOUBLE Ex  = 0.0;
    USR_DOUBLE alpha = 0.0, alpha2 = 0.0, sq_r = 0.0;
    USR_DOUBLE beta = 0.0;

    if (timeout_s == 0) {
        EFUN_PRINTF(("\nERROR: timeout_s value cannot be 0 for Lane %d >>\n", plp_aperta_blackhawk_tsc_get_lane(sa__)));
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    USR_MEMSET(err_analyzer_report, 0, sizeof(blackhawk_tsc_prbs_err_analyzer_report_st));
    t = (uint8_t)(err_analyzer.hist_errcnt_thresh - 1);                                                       /* Number of FEC correctable errors */ 
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_num_bits_per_ms(sa__, &num_bits_per_ms));

    if (num_bits_per_ms == 0) {
        EFUN_PRINTF(("\nERROR: num_bits_per_ms value cannot be 0 for Lane %d >>\n", plp_aperta_blackhawk_tsc_get_lane(sa__)));
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    if (PRBS_VERBOSE > 2) {
        EFUN_PRINTF(("\n\t=====> DEBUG INFO (start)\n"));
        EFUN_PRINTF(("\n\t << Measured Equivalent BER for specific error correcting FEC >>\n"));
    }

    /* Loop to calculate the measured BER based on above mentioned calculations */

    for(i=0; i < 9; i++) {
        meas_fec_ber[i] = ((((USR_DOUBLE)(err_analyzer.prbs_errcnt[i]) / (USR_DOUBLE) BLACKHAWK_TSC_PRBS_ERR_ANALYZER_FRAME_OVERLAP_FACTOR) / (USR_DOUBLE)(timeout_s * 1000)) / (USR_DOUBLE) num_bits_per_ms);
        if (PRBS_VERBOSE > 2) {
            EFUN_PRINTF(("\t Measured Equivalent BER at 't=%2d' (((frameErr/overlap_factor)/time_ms)/rate) = (((%u/%d)/%d)/%d): %0.3e \n",(t+i), err_analyzer.prbs_errcnt[i], 
                                                                                BLACKHAWK_TSC_PRBS_ERR_ANALYZER_FRAME_OVERLAP_FACTOR, timeout_s*1000, num_bits_per_ms, meas_fec_ber[i]));
        }
    }

    /* Calculating number of MEASURED points available for extrapolation */
    /* start_idx - first non-max FEC frame error value; stop_idx - index where FEC frame errors < 10 errors */
    start_idx = 0;
    stop_idx  = 0;
    if (err_analyzer.prbs_errcnt[8] != 0xFFFFFFFF) {
        stop_idx  = 9;
    }
    for (i=9; i > 0; i--) {
        if (err_analyzer.prbs_errcnt[i-1] != 0xFFFFFFFF) {
            start_idx = (uint8_t)(i - 1);
        }
        if (err_analyzer.prbs_errcnt[i-1] < 10*BLACKHAWK_TSC_PRBS_ERR_ANALYZER_FRAME_OVERLAP_FACTOR) {  /* "10*BLACKHAWK_TSC_PRBS_ERR_ANALYZER_FRAME_OVERLAP_FACTOR"  means  "10 error events" */
            stop_idx = (uint8_t)(i - 1);
        }
    }
    delta_n = (uint8_t)(stop_idx - start_idx);                                                  /* Number of MEASURED points available for extrapolation */

    /* Compute covariance and mean for extrapolation */
    if (delta_n >= 2) {                                                              /* There are at least 2 points to trace a line for extrapolation */
        for(i=start_idx; i < stop_idx; i++) {
            x    = (USR_DOUBLE) (t + i);
            y    = pow((-log10(meas_fec_ber[i])),BLACKHAWK_TSC_PRBS_ERR_ANALYZER_LINEARITY_ADJUST);
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
    }

    if (PRBS_VERBOSE > 2) {
        EFUN_PRINTF(("\n\t << Computing linear fit from Measured Equivalent BER points >>\n"));
        EFUN_PRINTF(("\t Number of measured points used for extrapolation = %d\n",delta_n));
        EFUN_PRINTF(("\t Exy=%lf, Eyy=%lf, Exx=%lf, Ey=%lf, Ex=%lf\n",Exy,Eyy,Exx,Ey,Ex));    
        EFUN_PRINTF(("\t alpha=%lf, beta=%lf\n",alpha,beta));    
        EFUN_PRINTF(("\t sq_r=%lf\n",sq_r));    
        EFUN_PRINTF(("\n\t=====> DEBUG INFO (end)\n\n"));
    }


    /* Calulate Projected BER => Equivalent projected post-FEC BER for t=15 */
    if (delta_n >= 2) {
        proj_ber[15] = ((alpha * 15) + beta);
        proj_ber[15] = pow(proj_ber[15],1.0/BLACKHAWK_TSC_PRBS_ERR_ANALYZER_LINEARITY_ADJUST);
        proj_ber[15] = pow(10,-proj_ber[15]);
    }
    else {
        proj_ber[15] = 0.0;
    }



    /* Populate output structure */ 
    err_analyzer_report->proj_ber  = proj_ber[15];
    err_analyzer_report->ber_proj_invalid  = 0;       /* To be populated: 0 -> valid, +1 -> BER greater than; -1 -> BER less than */
    err_analyzer_report->delta_n   = delta_n;
    return (ERR_CODE_NONE);
#else
    UNUSED(err_analyzer);
    UNUSED(timeout_s);
    UNUSED(*err_analyzer_report);
    EFUN_PRINTF(("This function needs SERDES_API_FLOATING_POINT define to operate \n"));
    return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
#endif /* SERDES_API_FLOATING_POINT */
}

err_code_t plp_aperta_blackhawk_tsc_prbs_error_analyzer_compute_proj(srds_access_t *sa__, blackhawk_tsc_prbs_err_analyzer_errcnt_st err_analyzer, uint32_t timeout_s) {

#ifdef SERDES_API_FLOATING_POINT
    blackhawk_tsc_prbs_err_analyzer_report_st err_analyzer_rep;

    if (timeout_s == 0) {
        EFUN_PRINTF(("\nERROR: timeout_s value cannot be 0 for Lane %d >>\n", plp_aperta_blackhawk_tsc_get_lane(sa__)));
    }

    EFUN(plp_aperta_blackhawk_tsc_prbs_error_analyzer_report_proj(sa__, err_analyzer, timeout_s,  &err_analyzer_rep));


    /* Displaying Projected BER => Equivalent projected post-FEC BER for t=15 */
    if (err_analyzer_rep.delta_n >= 2) {
        EFUN_PRINTF(("\n  PRBS Error Analyzer Projected BER (Equivalent projected post-FEC BER for t=15) for Lane %d = %0.3e\n\n",plp_aperta_blackhawk_tsc_get_lane(sa__), err_analyzer_rep.proj_ber));
    }
    else {
        EFUN_PRINTF(("\n << WARNING: Not enough valid measured points available for BER Projection for Lane %d ! >>\n\n",plp_aperta_blackhawk_tsc_get_lane(sa__)));
    }

#else
    UNUSED(err_analyzer);
    UNUSED(timeout_s);
#endif /* SERDES_API_FLOATING_POINT */


    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_get_prbsfb_status(srds_access_t *sa__, uint8_t *done_once) {
    /* D003_XX has this register read macro defined*/
    uint16_t ucode_version_major;
    /*Extracting the major version number*/
    ESTM(ucode_version_major = rdcv_common_ucode_version() & 0xFFF);
    if (ucode_version_major >= 0x3) {
        ESTM((*done_once = rdv_opt_err_pam4_prbsfb_byte_byte() & 0x40));
    } else {
        *done_once = 1;
        EFUN_PRINTF(("ERROR: Need microcode version D003_xx for prbsfb status read to work correctly, setting done_once to 1 \n"));
    }
    return(ERR_CODE_NONE);
}



