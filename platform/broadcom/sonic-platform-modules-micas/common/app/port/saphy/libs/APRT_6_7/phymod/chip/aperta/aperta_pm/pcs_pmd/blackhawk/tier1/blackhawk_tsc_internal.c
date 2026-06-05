/***********************************************************************************
 ***********************************************************************************
 *  File Name     :  blackhawk_tsc_internal.c                                         *
 *  Created On    :  13/02/2014                                                    *
 *  Created By    :  Justin Gaither                                                *
 *  Description   :  APIs for Serdes IPs                                           *
  * $Copyright: (c) 2020 Broadcom.                                               *
   * Broadcom Proprietary and Confidential. All rights reserved.$                 *
 *                                                                                 *
 ***********************************************************************************
 ***********************************************************************************/

/** @file blackhawk_tsc_internal.c
 * Implementation of API functions
 */
#include <phymod/phymod_system.h>
#include "blackhawk_tsc_internal.h"
#include "blackhawk_tsc_internal_error.h"
#include "blackhawk_tsc_access.h"
#include "blackhawk_tsc_common.h"
#include "blackhawk_tsc_config.h"
#include "blackhawk_tsc_functions.h"
#include "blackhawk_tsc_select_defns.h"
#include "blackhawk_tsc_prbs.h"
#include "blackhawk_tsc_debug_functions.h"



/* If SERDES_EVAL is defined, then is_ate_log_enabled() is queried to *\
\* know whether to log ATE.  blackhawk_tsc_access.h provides that function.  */
uint32_t plp_aperta_blackhawk_tsc_INTERNAL_mult_with_overflow_check(uint32_t a, uint32_t b, uint8_t *of) {
    uint16_t c,d;
    uint32_t r,s;
    if (a > b) return plp_aperta_blackhawk_tsc_INTERNAL_mult_with_overflow_check(b, a, of);
    *of = 0;
    c = (uint16_t)(b >> 16);
    d = UINT16_MAX & b;
    r = a * c;
    s = a * d;
    if (r > UINT16_MAX) *of = 1;
    r <<= 16;
    return (s + r);
}

#ifndef PHYMOD_SUPPORT
srds_info_t plp_aperta_blackhawk_tsc_info[NUM_SERDES_INFO_TABLES];
#endif


srds_info_t *plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr(srds_access_t *sa__) {    
#if defined(SERDES_EXTERNAL_INFO_TABLE_EN)
    return blackhawk_tsc_get_info_table_address(sa__);
#else
    uint8_t index = 0;
#if defined(SERDES_MULTI_INFO_TABLE_EN)
    err_code_t __err = ERR_CODE_NONE;
    UNUSED(__err);
    if (plp_aperta_blackhawk_tsc_INTERNAL_ip_version_check(sa__, &index) != ERR_CODE_NONE) {
        EFUN_PRINTF(("WARNING: Unable to figure out IP version in %s; Assuming A0\n", API_FUNCTION_NAME));
    }
#else
    index = 0;
#endif    
    return &plp_aperta_blackhawk_tsc_info[index];
#endif
}

srds_info_t *plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(srds_access_t *sa__) {
    err_code_t err_code = ERR_CODE_NONE;
    srds_info_t * blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr(sa__);
    CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, 0);
#ifdef PHYMOD_SUPPORT
    if ( blackhawk_tsc_info_ptr == NULL ) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    } else if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    }
#endif /* PHYMOD_SUPPORT */
    if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        err_code = plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
        if (err_code != ERR_CODE_NONE){
            EFUN_PRINTF(("ERROR:  %s Failed: 0x%04X \n", API_FUNCTION_NAME, err_code));
            return 0;
        }
    } 
    return blackhawk_tsc_info_ptr;
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_match_ucode_from_info(srds_access_t *sa__) {
    uint16_t ucode_version_major;
    uint8_t ucode_version_minor;
    uint32_t ucode_version;

    srds_info_t * blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr(sa__);
    CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, ERR_CODE_INVALID_INFO_TABLE_ADDR);

    ESTM(ucode_version_major = rdcv_common_ucode_version());
    ESTM(ucode_version_minor = rdcv_common_ucode_minor_version());
    ucode_version = (uint32_t)((ucode_version_major << 8) | ucode_version_minor);
    if (ucode_version == blackhawk_tsc_info_ptr->ucode_version) {
        return(ERR_CODE_NONE);
    } else {
        EFUN_PRINTF(("ERROR:  ucode version of the current thread not matching with stored plp_aperta_blackhawk_tsc_info->ucode_version, Expected 0x%08X, but received 0x%08X.\n",
                    blackhawk_tsc_info_ptr->ucode_version, ucode_version));
        return(ERR_CODE_UCODE_VERIFY_FAIL);
    }
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_verify_blackhawk_tsc_info(srds_info_t *blackhawk_tsc_info_ptr, srds_access_t *sa__)
{
    err_code_t err_code = ERR_CODE_NONE;

    CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, ERR_CODE_INVALID_INFO_TABLE_ADDR);

    if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        EFUN_PRINTF(("ERROR:  Mismatch in plp_aperta_blackhawk_tsc_info signature.  Expected 0x%08X, but received 0x%08X.\n",
                     SRDS_INFO_SIGNATURE, blackhawk_tsc_info_ptr->signature));
        return (blackhawk_tsc_error(sa__, ERR_CODE_INFO_TABLE_ERROR));
    }
    err_code = plp_aperta_blackhawk_tsc_INTERNAL_match_ucode_from_info(sa__);
    if (err_code != ERR_CODE_NONE) {
        /* ucode version mismatch */
        return(ERR_CODE_MICRO_INIT_NOT_DONE);
    }
    return (ERR_CODE_NONE);
}


int plp_aperta_blackhawk_tsc_osr_mode_enum_to_int_x1000(uint8_t osr_mode) {
    switch(osr_mode) {
    case BLACKHAWK_TSC_OSX1:      return 1000;
    case BLACKHAWK_TSC_OSX2:      return 2000;
    case BLACKHAWK_TSC_OSX4:      return 4000;
    case BLACKHAWK_TSC_OSX8:      return 8000;
    case BLACKHAWK_TSC_OSX16P5:   return 16500;
    case BLACKHAWK_TSC_OSX20P625: return 20625;
    case BLACKHAWK_TSC_OSX16:     return 16000;
    case BLACKHAWK_TSC_OSX32:     return 32000;
    case BLACKHAWK_TSC_OSX21P25:  return 21250;
#ifdef wr_ams_rx_rxclk_div2p5
    case BLACKHAWK_TSC_OSX2P5:    return 2500;
#endif
    default:                    return 1000;
    }
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_num_bits_per_ms(srds_access_t *sa__, uint32_t *num_bits_per_ms) {
    uint8_t osr_mode = 0;
#if defined(rd_rx_pam4_mode)
    uint8_t pam4_mode = 0;
#endif
    uint8_t pll_select = 0, prev_pll = 0;
    struct blackhawk_tsc_uc_core_config_st core_config = {{0}};

    { 
       blackhawk_tsc_osr_mode_st osr_mode_st;
       ENULL_MEMSET(&osr_mode_st, 0, sizeof(blackhawk_tsc_osr_mode_st));
       EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_osr_mode(sa__, &osr_mode_st));
       osr_mode = osr_mode_st.rx;
    }

#if defined(rd_rx_pam4_mode)
    ESTM(pam4_mode = rd_rx_pam4_mode());
#endif

#if defined(rd_rx_pll_select)
    ESTM(prev_pll = plp_aperta_blackhawk_tsc_get_pll_idx(sa__));
    ESTM(pll_select = rd_rx_pll_select());
    EFUN(plp_aperta_blackhawk_tsc_set_pll_idx(sa__,pll_select));
#endif
        EFUN(plp_aperta_blackhawk_tsc_get_uc_core_config(sa__, &core_config));
        *num_bits_per_ms = (uint32_t)((((uint32_t)core_config.vco_rate_in_Mhz * 100000UL) / (uint32_t)(plp_aperta_blackhawk_tsc_osr_mode_enum_to_int_x1000(osr_mode))*10));

#if defined(rd_rx_pam4_mode)
    if(pam4_mode > 0) *num_bits_per_ms <<= 1; 
#endif
    EFUN(plp_aperta_blackhawk_tsc_set_pll_idx(sa__,prev_pll));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_display_BER(srds_access_t *sa__, uint16_t time_ms) {
    char string[32];
      EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_BER_string(sa__,time_ms,string));
      USR_PRINTF(("%s",string));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_BER_string(srds_access_t *sa__, uint16_t time_ms, char *string) {
    char string2[3];
    struct blackhawk_tsc_ber_data_st ber_data_local = {0, 0, 0, 0};

    if(string == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_BER_data(sa__, time_ms, &ber_data_local, USE_HW_TIMERS));

    if(ber_data_local.prbs_chk_en == 0) {
        USR_STRCPY(string,"");
        return(ERR_CODE_NONE);
    } else {
        USR_STRCPY(string2," ");
    }

    if((COMPILER_64_HI(ber_data_local.num_errs) == 0) &&
       (COMPILER_64_LO(ber_data_local.num_errs) < 3) &&
       (ber_data_local.lcklost == 0)) {   /* lcklost = 0 */
        USR_STRNCAT(string2,"<",2);
        COMPILER_64_SET(ber_data_local.num_errs, 0, 3);
    } else {
        USR_STRNCAT(string2," ",2);
    }
    if(ber_data_local.lcklost == 1) {    /* lcklost = 1 */
        USR_STRCPY(string,"  !Lock ");
    } else {                    /* lcklost = 0 */
        uint16_t x=0,y=0,z=0,srds_div;

        if(COMPILER_64_GE(ber_data_local.num_errs, ber_data_local.num_bits)) {
            x = 1;y=0;z=0;
        }else {
            uint64_t temp_dividend;
            uint64_t temp_divisor;
            while(1) {
                COMPILER_64_COPY(temp_dividend, ber_data_local.num_errs);
                COMPILER_64_SHL(temp_dividend, 1);
                COMPILER_64_ADD_64(temp_dividend, ber_data_local.num_bits);

                COMPILER_64_COPY(temp_divisor, ber_data_local.num_bits);
                COMPILER_64_SHL(temp_divisor, 1);

                COMPILER_64_UDIV_64(temp_dividend, temp_divisor);

                srds_div = (uint16_t) COMPILER_64_LO( temp_dividend );

                if(srds_div>=10) {
                    break;
                }

                COMPILER_64_UMUL_32(ber_data_local.num_errs, 10);
                z++;
            }
            if(srds_div>=100) {
                srds_div = srds_div/10;
                /* coverity[overflow_const] */
                z--;
            }
            x=srds_div/10;
            y = (uint16_t)(srds_div - 10*x);
            /* coverity[overflow_const] */
            z--;
        }
        USR_SPRINTF(string,"%s%d.%1de-%02d",string2,x,y,z);

    }
    return(ERR_CODE_NONE);

}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_BER_data(srds_access_t *sa__, uint16_t time_ms, struct blackhawk_tsc_ber_data_st *ber_data, enum blackhawk_tsc_prbs_chk_timer_selection_enum timer_sel) {
    uint8_t lcklost = 0;
    uint32_t err_cnt= 0;
    const uint8_t ITER_MAX=10;
    uint8_t iter=0 ;
    

    if(ber_data == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    ESTM(ber_data->prbs_chk_en = rd_prbs_chk_en());
    if(ber_data->prbs_chk_en == 0)
        return(ERR_CODE_NONE);

    /* hardware timer's limitation - 448ms Max */
    if((time_ms <= PRBS_MAX_HW_TIMER_TIMEOUT) && (timer_sel == USE_HW_TIMERS)) {
        uint8_t prbs_chk_mode_bak = 0;
        uint32_t num_bits_per_ms=0;
        uint16_t time_ms_adjusted = 0;
        uint8_t prbs_chk_lock = 0;
        struct blackhawk_tsc_prbs_chk_hw_timer_ctrl_st prbs_chk_hw_timer_ctrl_bak;
        USR_MEMSET(&prbs_chk_hw_timer_ctrl_bak, 0, sizeof(struct blackhawk_tsc_prbs_chk_hw_timer_ctrl_st));
        /* STEP 1: save prbs chk mode */
        ESTM(prbs_chk_mode_bak = rd_prbs_chk_mode());
        
        /* STEP 2: force prbs mode 2, initial seed without hysteresis */ /* ??????????????????? */
        EFUN(wr_prbs_chk_mode(PRBS_INITIAL_SEED_NO_HYSTERESIS));
        
        /* STEP 3: save prbs hardware timer config registers */
        EFUN(plp_aperta_blackhawk_tsc_get_prbs_chk_hw_timer_ctrl(sa__, &prbs_chk_hw_timer_ctrl_bak));
        
        /* STEP 4: Configure hardware timer to count prbs errors */
        EFUN(plp_aperta_blackhawk_tsc_config_prbs_chk_hw_timer(sa__, time_ms, &time_ms_adjusted));
        
        /* STEP 5: Clear prbs error counters and lock lost status */
        EFUN(plp_aperta_blackhawk_tsc_prbs_err_count_state(sa__, &err_cnt,&lcklost)); /* clear error counters */
        
        /* STEP 6: toggle checker enable to start hardware timers and error counters */
        EFUN(plp_aperta_blackhawk_tsc_prbs_chk_en_toggle(sa__));
        
        /* STEP 7: Confirm that prbs lock was achieved */
        ESTM(prbs_chk_lock = rd_prbs_chk_lock());
        if(!prbs_chk_lock) {
            /* If prbs lock was not achieved, set lcklost = 1 and skip the wait for error accumulation */
            ber_data->lcklost = 1;            
        } else {
            /* STEP 8: wait time_ms for hardware timeout */
            EFUN(USR_DELAY_MS(time_ms_adjusted));
            /* STEP 9: Poll prbs_chk_lock to make sure timers expired */
            do {
                EFUN(USR_DELAY_MS(1));
                ESTM(prbs_chk_lock = rd_prbs_chk_lock());
                iter++;
            } while(prbs_chk_lock && iter<ITER_MAX);
            
            if(prbs_chk_lock) {
                /* timer hasn't expired or it expired and lock didn't go away */
                ber_data->lcklost = 1;
                return ERR_CODE_PRBS_CHK_HW_TIMERS_NOT_EXPIRED;
            }
            
            /* STEP 10: Read error counters */
            EFUN(plp_aperta_blackhawk_tsc_prbs_err_count_state(sa__, &err_cnt,&lcklost));
            
            /* STEP 11: Update the BER data structure */
            EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_num_bits_per_ms(sa__,&num_bits_per_ms));
            COMPILER_64_SET(ber_data->num_bits, 0, num_bits_per_ms);
            COMPILER_64_UMUL_32(ber_data->num_bits, time_ms_adjusted);
            COMPILER_64_SET(ber_data->num_errs, 0, err_cnt);
/*
            ber_data->num_bits = (uint64_t)num_bits_per_ms*(uint64_t)time_ms_adjusted;
            ber_data->num_errs = err_cnt;
*/
            ber_data->lcklost  = 0;
        }
        /* STEP 12: restore prbs hardware timer config registers */
        EFUN(plp_aperta_blackhawk_tsc_set_prbs_chk_hw_timer_ctrl(sa__, &prbs_chk_hw_timer_ctrl_bak));
        /* STEP 13: Restore prbs check mode */
        EFUN(wr_prbs_chk_mode(prbs_chk_mode_bak));        
    } else
    {
        UNUSED(timer_sel);
        EFUN(USR_DELAY_MS(time_ms));
        EFUN(plp_aperta_blackhawk_tsc_prbs_err_count_state(sa__, &err_cnt,&lcklost));
        ber_data->lcklost = lcklost;
        if(ber_data->lcklost == 0) {
            uint32_t num_bits_per_ms=0;
            EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_num_bits_per_ms(sa__,&num_bits_per_ms));
            COMPILER_64_SET(ber_data->num_bits, 0, num_bits_per_ms);
            COMPILER_64_UMUL_32(ber_data->num_bits, time_ms);
            COMPILER_64_SET(ber_data->num_errs, 0, err_cnt);
/*
            ber_data->num_bits = (uint64_t)num_bits_per_ms*(uint64_t)time_ms;
            ber_data->num_errs = err_cnt;
*/
            ESTM(lcklost = rd_prbs_chk_lock_lost_lh());

        }
    }



    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_prbs_timeout_count_from_time(uint16_t time_ms, uint16_t * time_ms_adjusted, struct blackhawk_tsc_prbs_chk_hw_timer_ctrl_st * const prbs_chk_hw_timer_ctrl ) {
    uint8_t timeout_value;
    uint8_t iter = 0;
    if(time_ms > PRBS_HW_LUT_MAX_MINUS_1) {
        UNUSED(timeout_value);
        *time_ms_adjusted = PRBS_MAX_HW_TIMER_TIMEOUT;
        return ERR_CODE_NONE;
    }

    *time_ms_adjusted = time_ms;

    if(time_ms < 8) {   /* Linear */
        timeout_value = (uint8_t)time_ms;
    } else {
        for(iter=8; iter>2; iter--) {
            if((time_ms>>iter)&0x1)
                break;
        }

        timeout_value = (uint8_t)(((iter-1)<<2) + ((time_ms>>(iter-2))&(0x3)));

        if((time_ms-((time_ms>>(iter-2))<<(iter-2)))>0) {   /* time_ms is not found in the map, using a possible value larger than provided */
            timeout_value++;
            *time_ms_adjusted = (uint16_t)((1<<(((timeout_value)>>2)+1)) + (((timeout_value)&3)*(1<<(((timeout_value)>>2)-1))));
        }
    }
    prbs_chk_hw_timer_ctrl->prbs_chk_en_timeout = timeout_value;
    prbs_chk_hw_timer_ctrl->prbs_chk_en_timer_mode = 0x3;   /* 1ms unit */    
    return ERR_CODE_NONE;
}



int16_t plp_aperta_blackhawk_tsc_INTERNAL_ladder_setting_to_mV(srds_access_t *sa__, int8_t ctrl, uint8_t range_250) {
    uint16_t absv;                                    /* Absolute value of ctrl */
    int16_t nlmv;                                     /* Non-linear value */
    
    /* Get absolute value */
    absv = (uint16_t)(SRDS_ABS(ctrl));

    {
       UNUSED(range_250);

       nlmv = (int16_t)(absv*263/127);
    }
    /* Add sign and return */
    return( (ctrl>=0) ? nlmv : (int16_t)(-nlmv));
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_pam4_to_bin(srds_access_t *sa__, char var, char bin[]) {
    if(!bin) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

  switch (var) {
    case '0':  ENULL_STRCPY(bin,"00");
               break;
    case '1':  ENULL_STRCPY(bin,"01");
               break;
    case '2':  ENULL_STRCPY(bin,"11");  /* To account for PAM4 Gray coding */
               break;
    case '3':  ENULL_STRCPY(bin,"10");  /* To account for PAM4 Gray coding */
               break;
    case '_':  ENULL_STRCPY(bin,"");
               break;
    default :  ENULL_STRCPY(bin,"");
               EFUN_PRINTF(("ERROR: Invalid PAM4 format Pattern\n"));
               return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_INVALID_PAM4));
  }
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_compute_bin(srds_access_t *sa__, char var, char bin[]) {
    if(!bin) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

  switch (var) {
    case '0':  ENULL_STRCPY(bin,"0000");
               break;
    case '1':  ENULL_STRCPY(bin,"0001");
               break;
    case '2':  ENULL_STRCPY(bin,"0010");
               break;
    case '3':  ENULL_STRCPY(bin,"0011");
               break;
    case '4':  ENULL_STRCPY(bin,"0100");
               break;
    case '5':  ENULL_STRCPY(bin,"0101");
               break;
    case '6':  ENULL_STRCPY(bin,"0110");
               break;
    case '7':  ENULL_STRCPY(bin,"0111");
               break;
    case '8':  ENULL_STRCPY(bin,"1000");
               break;
    case '9':  ENULL_STRCPY(bin,"1001");
               break;
    case 'a':
    case 'A':  ENULL_STRCPY(bin,"1010");
               break;
    case 'b':
    case 'B':  ENULL_STRCPY(bin,"1011");
               break;
    case 'c':
    case 'C':  ENULL_STRCPY(bin,"1100");
               break;
    case 'd':
    case 'D':  ENULL_STRCPY(bin,"1101");
               break;
    case 'e':
    case 'E':  ENULL_STRCPY(bin,"1110");
               break;
    case 'f':
    case 'F':  ENULL_STRCPY(bin,"1111");
               break;
    case '_':  ENULL_STRCPY(bin,"");
               break;
    default :  ENULL_STRCPY(bin,"");
               EFUN_PRINTF(("ERROR: Invalid Hexadecimal Pattern\n"));
               return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_INVALID_HEX));
  }
  return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_compute_hex(srds_access_t *sa__, char bin[],uint8_t *hex) {
  if(!hex) {
    return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
 
  if (!USR_STRCMP(bin,"0000")) {
    *hex = 0x0;
  } 
  else if (!USR_STRCMP(bin,"0001")) {
    *hex = 0x1;
  } 
  else if (!USR_STRCMP(bin,"0010")) {
    *hex = 0x2;
  } 
  else if (!USR_STRCMP(bin,"0011")) {
    *hex = 0x3;
  } 
  else if (!USR_STRCMP(bin,"0100")) {
    *hex = 0x4;
  } 
  else if (!USR_STRCMP(bin,"0101")) {
    *hex = 0x5;
  } 
  else if (!USR_STRCMP(bin,"0110")) {
    *hex = 0x6;
  } 
  else if (!USR_STRCMP(bin,"0111")) {
    *hex = 0x7;
  } 
  else if (!USR_STRCMP(bin,"1000")) {
    *hex = 0x8;
  } 
  else if (!USR_STRCMP(bin,"1001")) {
    *hex = 0x9;
  } 
  else if (!USR_STRCMP(bin,"1010")) {
    *hex = 0xA;
  } 
  else if (!USR_STRCMP(bin,"1011")) {
    *hex = 0xB;
  } 
  else if (!USR_STRCMP(bin,"1100")) {
    *hex = 0xC;
  } 
  else if (!USR_STRCMP(bin,"1101")) {
    *hex = 0xD;
  } 
  else if (!USR_STRCMP(bin,"1110")) {
    *hex = 0xE;
  } 
  else if (!USR_STRCMP(bin,"1111")) {
    *hex = 0xF;
  } 
  else {
    EFUN_PRINTF(("ERROR: Invalid Binary to Hex Conversion\n"));
    *hex = 0x0;
    return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_INVALID_BIN2HEX));
  } 
  return (ERR_CODE_NONE);
}

uint8_t plp_aperta_blackhawk_tsc_INTERNAL_stop_micro(srds_access_t *sa__, uint8_t graceful, err_code_t *err_code_p) {
   uint8_t stop_state = 0;

   if(!err_code_p) {
       return(0xFF);
   }

   /* Log current micro stop status */
   EPSTM2((stop_state = rdv_usr_sts_micro_stopped()),0xFF);

   /* Stop micro only if micro is not stopped currently */
   if (!(stop_state & 0x7F)) {
       if (graceful) {
           EPFUN2((plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__, 1)),0xFF);
       }
       else {
           EPFUN2((plp_aperta_blackhawk_tsc_pmd_uc_control(sa__, CMD_UC_CTRL_STOP_IMMEDIATE,GRACEFUL_STOP_TIME)),0xFF);
       }
   }

   /* Return the previous micro stop status */
   return(stop_state);
}


  

/********************************************************/
/*  Global RAM access through Micro Register Interface  */
/********************************************************/
/* Micro Global RAM Byte Read */
uint8_t plp_aperta_blackhawk_tsc_INTERNAL_rdb_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
    uint8_t rddata;

    if(!err_code_p) {
        return(0);
    }
    EPSTM(rddata = plp_aperta_blackhawk_tsc_rdb_uc_ram(sa__, err_code_p, addr)); /* Use Micro register interface for reading RAM */
    return (rddata);
}

/* Micro Global RAM Word Read */
uint16_t plp_aperta_blackhawk_tsc_INTERNAL_rdw_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
  uint16_t rddata;

  if(!err_code_p) {
      return(0);
  }
  if (addr%2 != 0) {                                         /* Validate even address */
      *err_code_p = ERR_CODE_INVALID_RAM_ADDR;
      USR_PRINTF(("Error incorrect addr x%04x\n",addr));
      return (0);
  }
  EPSTM(rddata = plp_aperta_blackhawk_tsc_rdw_uc_ram(sa__, err_code_p, (addr))); /* Use Micro register interface for reading RAM */
  return (rddata);
}

/* Micro Global RAM Byte Write  */
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_wrb_uc_var(srds_access_t *sa__, uint16_t addr, uint8_t wr_val) {

    return (plp_aperta_blackhawk_tsc_wrb_uc_ram(sa__, addr, wr_val));          /* Use Micro register interface for writing RAM */
}

/* Micro Global RAM Word Write  */
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_wrw_uc_var(srds_access_t *sa__, uint16_t addr, uint16_t wr_val) {
    if (addr%2 != 0) {                                       /* Validate even address */
      USR_PRINTF(("Error incorrect addr x%04x\n",addr));
        return (blackhawk_tsc_error(sa__, ERR_CODE_INVALID_RAM_ADDR));
    }
    return (plp_aperta_blackhawk_tsc_wrw_uc_ram(sa__, addr, wr_val));          /* Use Micro register interface for writing RAM */
}


/***********************/
/*  Event Log Display  */
/***********************/
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_event_log_dump_callback(void *arg, uint8_t byte_count, uint16_t data) {
    blackhawk_tsc_INTERNAL_event_log_dump_state_t * const state_ptr = (blackhawk_tsc_INTERNAL_event_log_dump_state_t *)arg;
    const uint8_t bytes_per_row=16;

    if (byte_count == 0) {
        if (state_ptr->line_start_index != state_ptr->index) {
            EFUN_PRINTF(("%*s    %d\n", 4*(bytes_per_row - state_ptr->index + state_ptr->line_start_index), "", state_ptr->line_start_index));
        }
        return (ERR_CODE_NONE);
    }
    if (byte_count == 1) {
        /* There is a trailing byte in the event log.
         * The simplest way to handle it is to print out a whole word, but mask
         * the invalid upper byte.
         */
        data &= 0xFF;
    }
    EFUN_PRINTF(("  0x%04x", ((data & 0xFF) << 8) | (data >> 8)));
    state_ptr->index = (uint16_t)(state_ptr->index + 2);
    if (state_ptr->index % bytes_per_row == 0) {
        EFUN_PRINTF(("    %d\n", state_ptr->line_start_index));
        state_ptr->line_start_index = state_ptr->index;
    }

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_read_event_log_with_callback(srds_access_t *sa__,
                                                        uint8_t micro_num,
                                                        uint8_t bypass_micro,
                                                        void *arg,
                                                        err_code_t (*callback)(void *, uint8_t, uint16_t)) {
    uint16_t rd_idx;
    uint8_t current_lane;

    srds_info_t * blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__);
    CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, ERR_CODE_INVALID_INFO_TABLE_ADDR);


#ifdef PHYMOD_SUPPORT
    if ( blackhawk_tsc_info_ptr == NULL ) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    } else if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    }
#endif /* PHYMOD_SUPPORT */

    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_verify_blackhawk_tsc_info(blackhawk_tsc_info_ptr, sa__));

    if (micro_num >= blackhawk_tsc_info_ptr->micro_count) {
        return (blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    /* Read Current lane so that it can be restored at the end of function */
    current_lane = plp_aperta_blackhawk_tsc_get_lane(sa__);
    /* Blackhawk has 2 lanes per micro, set lane appropriate for the desired micro */
    EFUN(blackhawk_tsc_set_physical_lane(sa__, (uint8_t)(micro_num<<1)));
    EFUN_PRINTF(("\n\n********************************************\n"));
    EFUN_PRINTF((    "**** SERDES UC TRACE MEMORY DUMP ***********\n"));
    EFUN_PRINTF((    "********************************************\n"));

    if (bypass_micro) {
        ESTM(rd_idx = rducv_trace_mem_wr_idx());
        if (blackhawk_tsc_info_ptr->trace_memory_descending_writes) {
            ++rd_idx;
            if (rd_idx >= blackhawk_tsc_info_ptr->trace_mem_ram_size) {
                rd_idx = 0;
            }
        } else {
            if (rd_idx == 0) {
                rd_idx = (uint16_t)blackhawk_tsc_info_ptr->trace_mem_ram_size;
            }
            --rd_idx;
        }
    } else {
        /* Start Read to stop uC logging and read the word at last event written by uC */
        EFUN(plp_aperta_blackhawk_tsc_pmd_uc_cmd(sa__, CMD_EVENT_LOG_READ, CMD_EVENT_LOG_READ_START, GRACEFUL_STOP_TIME));
        ESTM(rd_idx = rducv_trace_mem_rd_idx());
    }
    
    EFUN_PRINTF(( "\n  DEBUG INFO: trace memory read index = 0x%04x\n", rd_idx));
    
    EFUN_PRINTF(("  DEBUG INFO: trace memory size = 0x%04x\n\n", blackhawk_tsc_info_ptr->trace_mem_ram_size));

    if (blackhawk_tsc_info_ptr->trace_memory_descending_writes) {
        /* Micro writes trace memory using descending addresses.
         * So read using ascending addresses using block read
         */
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_rdblk_uc_generic_ram(sa__,
                                                  blackhawk_tsc_info_ptr->trace_mem_ram_base + (uint32_t)(blackhawk_tsc_info_ptr->grp_ram_size*micro_num),
                                                  (uint16_t)blackhawk_tsc_info_ptr->trace_mem_ram_size,
                                                  rd_idx,
                                                  (uint16_t)blackhawk_tsc_info_ptr->trace_mem_ram_size,
                                                  arg,
                                                  callback));
    } else {
        /* Micro writes trace memory using descending addresses.
         * So read using ascending addresses using block read
         */
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_rdblk_uc_generic_ram_descending(sa__,
                                                             blackhawk_tsc_info_ptr->trace_mem_ram_base + (uint32_t)(blackhawk_tsc_info_ptr->grp_ram_size*micro_num),
                                                             (uint16_t)blackhawk_tsc_info_ptr->trace_mem_ram_size,
                                                             rd_idx,
                                                             (uint16_t)blackhawk_tsc_info_ptr->trace_mem_ram_size,
                                                             arg,
                                                             callback));
    }

    if (!bypass_micro) {
        /* Read Done to resume logging  */
        EFUN(plp_aperta_blackhawk_tsc_pmd_uc_cmd(sa__, CMD_EVENT_LOG_READ, CMD_EVENT_LOG_READ_DONE, 50));
    }
    EFUN(plp_aperta_blackhawk_tsc_set_lane(sa__,current_lane));
    return(ERR_CODE_NONE);
}

#ifdef TO_FLOATS
/* convert uint32_t to float8 */
float8_t blackhawk_tsc_INTERNAL_int32_to_float8(uint32_t input) {
    int8_t cnt;
    uint8_t output;

    if(input == 0) {
      return(0);
    } else if(input == 1) {
      return(0xe0);
    } else {
      cnt = 0;
      input = input & 0x7FFFFFFF; /* get rid of MSB which may be lock indicator */
      do {
        input = input << 1;
        cnt++;
      } while ((input & 0x80000000) == 0);

      output = (uint8_t)((input & 0x70000000)>>23)+(31 - cnt%32);
      return(output);
    }
}
#endif

/* convert float8 to uint32_t */
uint32_t plp_aperta_blackhawk_tsc_INTERNAL_float8_to_int32(float8_t input) {
    uint32_t x;
    if(input == 0) return(0);
    x = (uint32_t)((input>>5) + 8);
    if((input & 0x1F) < 3) {
      return(x>>(3-(input & 0x1f)));
    } 
    return(x<<((input & 0x1F)-3));
}

/* Convert uint8 to 8-bit gray code */
uint8_t plp_aperta_blackhawk_tsc_INTERNAL_uint8_to_gray(uint8_t input) {
    return input ^ (input >> 1);
}

/* Convert 8-bit gray code to uint8 */
uint8_t plp_aperta_blackhawk_tsc_INTERNAL_gray_to_uint8(uint8_t input) {
    input = input ^ (input >> 4);
    input = input ^ (input >> 2);
    input = input ^ (input >> 1);
    return input;
}

/* Convert seconds to hr:min:sec */
uint8_t plp_aperta_blackhawk_tsc_INTERNAL_seconds_to_displayformat(uint32_t seconds, uint8_t *hrs, uint8_t *mins, uint8_t *secs) {
    *hrs    = (uint8_t)(seconds / 3600);
    seconds = (seconds % 3600);
    *mins   = (uint8_t)(seconds / 60);
    *secs   = (uint8_t)(seconds % 60);
    return(ERR_CODE_NONE);
}

/* convert float12 to uint32 */
uint32_t plp_aperta_blackhawk_tsc_INTERNAL_float12_to_uint32(uint8_t input, uint8_t multi) {

    return(((uint32_t)input)<<multi);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_pf_main(srds_access_t *sa__, uint8_t val) {
    if (val > 31) 
    {
      return (blackhawk_tsc_error(sa__, ERR_CODE_PF_INVALID));
    }
    EFUN(WR_RX_PF_CTRL(val));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pf_main(srds_access_t *sa__, uint8_t *val) {
    ESTM(*val = RD_RX_PF_CTRL());
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_pf2(srds_access_t *sa__, uint8_t val) {
    if (val > 7) {
      return (blackhawk_tsc_error(sa__, ERR_CODE_PF_INVALID));
    }
    EFUN(WR_RX_PF2_CTRL(val));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pf2(srds_access_t *sa__, uint8_t *val) {
    ESTM(*val = RD_RX_PF2_CTRL());
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_pf3(srds_access_t *sa__, uint8_t val) {
    if (val > 15) {
      return (blackhawk_tsc_error(sa__, ERR_CODE_PF_INVALID));
    }
    EFUN(wr_ams_rx_pf3_ctrl(plp_aperta_blackhawk_tsc_INTERNAL_uint8_to_gray(val)));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pf3(srds_access_t *sa__, uint8_t *val) {
    ESTM(*val = plp_aperta_blackhawk_tsc_INTERNAL_gray_to_uint8(rd_ams_rx_pf3_ctrl()));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_vga(srds_access_t *sa__, uint8_t val) {

    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_check_uc_lane_stopped(sa__));  /* make sure uC is stopped to avoid race conditions */

    if (val > RX_VGA_CTRL_VAL_MAX) {
      return (blackhawk_tsc_error(sa__, ERR_CODE_VGA_INVALID));
    }
#if defined(RX_VGA_CTRL_VAL_MIN) && (RX_VGA_CTRL_VAL_MIN > 0)
    if (val < RX_VGA_CTRL_VAL_MIN) {
      return (blackhawk_tsc_error(sa__, ERR_CODE_VGA_INVALID));
    } 
#endif
    
    EFUN(wr_rx_vga_val(val));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_vga(srds_access_t *sa__, uint8_t *val) {
    ESTM(*val = (uint8_t)rd_rx_vga_status());
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe1(srds_access_t *sa__, int8_t val) {
    enum blackhawk_tsc_rx_pam_mode_enum pam_mode = NRZ;
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));

    if(pam_mode != NRZ) {
        EFUN_PRINTF(("ERROR: RX DFE1 is only available in NRZ mode\n"));
        return(ERR_CODE_INVALID_RX_PAM_MODE);
    }
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_check_uc_lane_stopped(sa__));  /* make sure uC is stopped to avoid race conditions */

    EFUN(wr_rx_afe_override_sel(1));                     /* Select data23 slicer */
    EFUN(wr_rx_afe_override_val((uint8_t)val));
    EFUN(wr_rx_afe_override_write(1));
    return(ERR_CODE_NONE); 
}
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe1(srds_access_t *sa__, int8_t *val) {
    {
        enum blackhawk_tsc_rx_pam_mode_enum pam_mode = NRZ;
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));

        if(pam_mode != NRZ) {
            EFUN_PRINTF(("ERROR: RX DFE1 is only available in NRZ mode (core %d lane %d)\n",plp_aperta_blackhawk_tsc_get_core(sa__),plp_aperta_blackhawk_tsc_get_lane(sa__)));
            return(ERR_CODE_INVALID_RX_PAM_MODE);
        }
    }
    ESTM(*val = (int8_t)rd_rx_data23_status());
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe2(srds_access_t *sa__, int8_t val) {
    if ((val > 15) || (val < -15)) 
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 15, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rx_afe_override_sel(6));
    EFUN(wr_rxa_dfe_tap2_val((uint8_t)val));
    EFUN(wr_rxa_dfe_tap2_write(1));
    EFUN(wr_rxb_dfe_tap2_val((uint8_t)val));
    EFUN(wr_rxb_dfe_tap2_write(1));
    EFUN(wr_rxc_dfe_tap2_val((uint8_t)val));
    EFUN(wr_rxc_dfe_tap2_write(1));
    EFUN(wr_rxd_dfe_tap2_val((uint8_t)val));
    EFUN(wr_rxd_dfe_tap2_write(1));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe2(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap2_status() + rd_rxb_dfe_tap2_status() + rd_rxc_dfe_tap2_status() + rd_rxd_dfe_tap2_status())>>2));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe3(srds_access_t *sa__, int8_t val) {
    if ((val > 15) || (val < -15))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 15, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rx_afe_override_sel(6));
    EFUN(wr_rxa_dfe_tap3_val((uint8_t)val));
    EFUN(wr_rxa_dfe_tap3_write(1));
    EFUN(wr_rxb_dfe_tap3_val((uint8_t)val));
    EFUN(wr_rxb_dfe_tap3_write(1));
    EFUN(wr_rxc_dfe_tap3_val((uint8_t)val));
    EFUN(wr_rxc_dfe_tap3_write(1));
    EFUN(wr_rxd_dfe_tap3_val((uint8_t)val));
    EFUN(wr_rxd_dfe_tap3_write(1));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe3(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap3_status() + rd_rxb_dfe_tap3_status() + rd_rxc_dfe_tap3_status() + rd_rxd_dfe_tap3_status())>>2));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe4(srds_access_t *sa__, int8_t val) {
    if ((val > 15) || (val < -15))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 15, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap4((uint8_t)val));
    EFUN(wr_rxb_dfe_tap4((uint8_t)val));
    EFUN(wr_rxc_dfe_tap4((uint8_t)val));
    EFUN(wr_rxd_dfe_tap4((uint8_t)val));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe4(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap4() + rd_rxb_dfe_tap4() + rd_rxc_dfe_tap4() + rd_rxd_dfe_tap4())>>2));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe5(srds_access_t *sa__, int8_t val) {
    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap5((uint8_t)val));
    EFUN(wr_rxb_dfe_tap5((uint8_t)val));
    EFUN(wr_rxc_dfe_tap5((uint8_t)val));
    EFUN(wr_rxd_dfe_tap5((uint8_t)val));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe5(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap5() + rd_rxb_dfe_tap5() + rd_rxc_dfe_tap5() + rd_rxd_dfe_tap5())>>2));
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe6(srds_access_t *sa__, int8_t val) {
    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap6((uint8_t)val));
    EFUN(wr_rxb_dfe_tap6((uint8_t)val));
    EFUN(wr_rxc_dfe_tap6((uint8_t)val));
    EFUN(wr_rxd_dfe_tap6((uint8_t)val));
    return(ERR_CODE_NONE); 
}
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe6(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap6() + rd_rxb_dfe_tap6() + rd_rxc_dfe_tap6() + rd_rxd_dfe_tap6())>>2));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe7(srds_access_t *sa__, int8_t val) {
    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap7((uint8_t)val));
    EFUN(wr_rxb_dfe_tap7((uint8_t)val));
    EFUN(wr_rxc_dfe_tap7((uint8_t)val));
    EFUN(wr_rxd_dfe_tap7((uint8_t)val));
    return(ERR_CODE_NONE); 
}
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe7(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap7() + rd_rxb_dfe_tap7() + rd_rxc_dfe_tap7() + rd_rxd_dfe_tap7())>>2));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe8(srds_access_t *sa__, int8_t val) {
    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap8((uint8_t)val));
    EFUN(wr_rxb_dfe_tap8((uint8_t)val));
    EFUN(wr_rxc_dfe_tap8((uint8_t)val));
    EFUN(wr_rxd_dfe_tap8((uint8_t)val));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe8(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap8() + rd_rxb_dfe_tap8() + rd_rxc_dfe_tap8() + rd_rxd_dfe_tap8())>>2));
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe9(srds_access_t *sa__, int8_t val) {
    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap9((uint8_t)val));
    EFUN(wr_rxb_dfe_tap9((uint8_t)val));
    EFUN(wr_rxc_dfe_tap9((uint8_t)val));
    EFUN(wr_rxd_dfe_tap9((uint8_t)val));
    return(ERR_CODE_NONE); 
}
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe9(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap9() + rd_rxb_dfe_tap9() + rd_rxc_dfe_tap9() + rd_rxd_dfe_tap9())>>2));
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe10(srds_access_t *sa__, int8_t val) {
    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap10((uint8_t)val));
    EFUN(wr_rxb_dfe_tap10((uint8_t)val));
    EFUN(wr_rxc_dfe_tap10((uint8_t)val));
    EFUN(wr_rxd_dfe_tap10((uint8_t)val));
    return(ERR_CODE_NONE); 
}
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe10(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap10() + rd_rxb_dfe_tap10() + rd_rxc_dfe_tap10() + rd_rxd_dfe_tap10())>>2));
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe11(srds_access_t *sa__, int8_t val) {
    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap11((uint8_t)val));
    EFUN(wr_rxb_dfe_tap11((uint8_t)val));
    EFUN(wr_rxc_dfe_tap11((uint8_t)val));
    EFUN(wr_rxd_dfe_tap11((uint8_t)val));
    return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe11(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap11() + rd_rxb_dfe_tap11() + rd_rxc_dfe_tap11() + rd_rxd_dfe_tap11())>>2));
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe12(srds_access_t *sa__, int8_t val) {
    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap12((uint8_t)val));
    EFUN(wr_rxb_dfe_tap12((uint8_t)val));
    EFUN(wr_rxc_dfe_tap12((uint8_t)val));
    EFUN(wr_rxd_dfe_tap12((uint8_t)val));
    return(ERR_CODE_NONE); 
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe12(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap12() + rd_rxb_dfe_tap12() + rd_rxc_dfe_tap12() + rd_rxd_dfe_tap12())>>2));
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe13(srds_access_t *sa__, int8_t val) {
    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap13((uint8_t)val));
    EFUN(wr_rxb_dfe_tap13((uint8_t)val));
    EFUN(wr_rxc_dfe_tap13((uint8_t)val));
    EFUN(wr_rxd_dfe_tap13((uint8_t)val));
    return(ERR_CODE_NONE); 
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe13(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap13() + rd_rxb_dfe_tap13() + rd_rxc_dfe_tap13() + rd_rxd_dfe_tap13())>>2));
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_rx_dfe14(srds_access_t *sa__, int8_t val) {

    if ((val > 7) || (val < -7))
    {
        USR_PRINTF(("Exceeded range of DFE tap limit = 7, request %d\n",val));
        return (blackhawk_tsc_error(sa__, ERR_CODE_DFE_TAP));  
    }
    EFUN(wr_rxa_dfe_tap14((uint8_t)val));
    EFUN(wr_rxb_dfe_tap14((uint8_t)val));
    EFUN(wr_rxc_dfe_tap14((uint8_t)val));
    EFUN(wr_rxd_dfe_tap14((uint8_t)val));
    return(ERR_CODE_NONE); 
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe14(srds_access_t *sa__, int8_t *val) {
    ESTM(*val = (int8_t)((rd_rxa_dfe_tap14() + rd_rxb_dfe_tap14() + rd_rxc_dfe_tap14() + rd_rxd_dfe_tap14())>>2));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_ip_version_check(srds_access_t *sa__, uint8_t *i_am_b0) {
        uint8_t ams_tx_version_id = 0;


        ESTM(ams_tx_version_id = rd_ams_tx_version_id());
        *i_am_b0 = (ams_tx_version_id & 0xF0) >= 0xC0;

    return ERR_CODE_NONE;
}



err_code_t plp_aperta_blackhawk_tsc_INTERNAL_load_txfir_taps(srds_access_t *sa__){
  EFUN(wr_txfir_tap_load(1));  /* Load the tap coefficients into TXFIR. */

  return(ERR_CODE_NONE); 
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_tx_tap(srds_access_t *sa__, uint8_t tap_num, int16_t val) {


    switch (tap_num) {
    case  0: EFUN(wr_txfir_tap0_coeff ((uint16_t)val)); break;
    case  1: EFUN(wr_txfir_tap1_coeff ((uint16_t)val)); break;
    case  2: EFUN(wr_txfir_tap2_coeff ((uint16_t)val)); break;
    case  3: EFUN(wr_txfir_tap3_coeff ((uint16_t)val)); break;
    case  4: EFUN(wr_txfir_tap4_coeff ((uint16_t)val)); break;
    case  5: EFUN(wr_txfir_tap5_coeff ((uint16_t)val)); break;
    case  6: EFUN(wr_txfir_tap6_coeff ((uint8_t)val)); break;
    case  7: EFUN(wr_txfir_tap7_coeff ((uint8_t)val)); break;
    case  8: EFUN(wr_txfir_tap8_coeff ((uint8_t)val)); break;
    case  9: EFUN(wr_txfir_tap9_coeff ((uint8_t)val)); break;
    case 10: EFUN(wr_txfir_tap10_coeff((uint8_t)val)); break;
    case 11: EFUN(wr_txfir_tap11_coeff((uint8_t)val)); break;
    default:
        return (blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_tx_tap(srds_access_t *sa__, uint8_t tap_num, int16_t *val) {

    switch (tap_num) {
    case  0: ESTM(*val = rd_txfir_tap0_coeff ()); break;
    case  1: ESTM(*val = rd_txfir_tap1_coeff ()); break;
    case  2: ESTM(*val = rd_txfir_tap2_coeff ()); break;
    case  3: ESTM(*val = rd_txfir_tap3_coeff ()); break;
    case  4: ESTM(*val = rd_txfir_tap4_coeff ()); break;
    case  5: ESTM(*val = rd_txfir_tap5_coeff ()); break;
    case  6: ESTM(*val = rd_txfir_tap6_coeff ()); break;
    case  7: ESTM(*val = rd_txfir_tap7_coeff ()); break;
    case  8: ESTM(*val = rd_txfir_tap8_coeff ()); break;
    case  9: ESTM(*val = rd_txfir_tap9_coeff ()); break;
    case 10: ESTM(*val = rd_txfir_tap10_coeff()); break;
    case 11: ESTM(*val = rd_txfir_tap11_coeff()); break;
    default:
        return (blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }


    return(ERR_CODE_NONE);
}




err_code_t plp_aperta_blackhawk_tsc_INTERNAL_core_clkgate(srds_access_t *sa__, uint8_t enable) {
  UNUSED(sa__);

  if (enable) {
  }
  else {
  }
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_lane_clkgate(srds_access_t *sa__, uint8_t enable) {

  if (enable) {
    /* Use frc/frc_val to force all RX and TX clk_vld signals to 0 */
        EFUN(wr_pmd_rx_clk_vld_frc_val(0x0));
        EFUN(wr_pmd_rx_clk_vld_frc(0x1));
        EFUN(wr_pmd_tx_clk_vld_frc_val(0x0));
        EFUN(wr_pmd_tx_clk_vld_frc(0x1));

    /* Use frc_on to force clkgate */
        EFUN(wr_ln_rx_s_clkgate_frc_on(0x1));

        EFUN(wr_ln_tx_s_clkgate_frc_on(0x1));

  }
  else {
        EFUN(wr_pmd_rx_clk_vld_frc_val(0x0));
        EFUN(wr_pmd_rx_clk_vld_frc(0x0));
        EFUN(wr_pmd_tx_clk_vld_frc_val(0x0));
        EFUN(wr_pmd_tx_clk_vld_frc(0x0));

        EFUN(wr_ln_rx_s_clkgate_frc_on(0x0));

        EFUN(wr_ln_tx_s_clkgate_frc_on(0x0));
  }
  return (ERR_CODE_NONE);
}

/*-----------------------------------------------*/
/*  Get dynamic eye margin estimation values     */
/*-----------------------------------------------*/
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_eye_margin_est(srds_access_t *sa__, uint16_t *left_eye_mUI, uint16_t *right_eye_mUI, uint16_t  *upper_eye_mV, uint16_t *lower_eye_mV) {
  uint8_t ladder_range = 0;
  ESTM(*left_eye_mUI  = plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mUI(sa__, rdv_usr_sts_heye_left()));
  ESTM(*right_eye_mUI = plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mUI(sa__, rdv_usr_sts_heye_right()));
  ESTM(*upper_eye_mV = plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mV(sa__, rdv_usr_sts_veye_upper(), ladder_range));
  ESTM(*lower_eye_mV = plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mV(sa__, rdv_usr_sts_veye_lower(), ladder_range));

  return (ERR_CODE_NONE);
}

/*----------------------------------------------------*/
/*  Get dynamic eye margin estimation values (PAM)    */
/*----------------------------------------------------*/
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_pam_eye_margin_est(srds_access_t *sa__, blackhawk_tsc_eye_margin_t *eye_margin) {
  uint8_t ladder_range = 0;
  uint8_t i;
  for(i=0; i<NUM_PAM_EYES; i++) {
      ESTM(eye_margin->left_eye_mUI [i] = plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mUI(sa__,  rdv_usr_sts_pam_heye_left(i)));
      ESTM(eye_margin->right_eye_mUI[i] = plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mUI(sa__, rdv_usr_sts_pam_heye_right(i)));
      ESTM(eye_margin->upper_eye_mV [i] = plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mV (sa__, rdv_usr_sts_pam_veye_upper(i), ladder_range));
      ESTM(eye_margin->lower_eye_mV [i] = plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mV (sa__, rdv_usr_sts_pam_veye_lower(i), ladder_range));
  }
  return (ERR_CODE_NONE);
}

/*------------------------------------------------------*/
/*  Print dynamic eye margin estimation values (PAM)    */
/*------------------------------------------------------*/
err_code_t plp_aperta_blackhawk_tsc_display_pam_eye_margin_est(srds_access_t *sa__) {
  uint8_t i;
  blackhawk_tsc_eye_margin_t eye_margin;

  ESTM(plp_aperta_blackhawk_tsc_INTERNAL_get_pam_eye_margin_est(sa__, &eye_margin));

  EFUN_PRINTF(("Eye margin: L(mUI),R(mUI),U(mV),D(mV)\n"));
  for(i=NUM_PAM_EYES; i>0; i--) {
      EFUN_PRINTF(("%s eye: ", (i-1)==2?"Upper ":(i-1)==1?"Middle":"Lower "));
      EFUN_PRINTF(("%6d,%6d,%5d,%5d\n", eye_margin.left_eye_mUI[i-1], eye_margin.right_eye_mUI[i-1], eye_margin.upper_eye_mV[i-1], eye_margin.lower_eye_mV[i-1]));
  }
  return (ERR_CODE_NONE);
}

uint16_t plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mUI(srds_access_t *sa__, uint8_t var) 
{   
    UNUSED(sa__);
    /* var is in units of 1/512 th UI, so need to multiply by 1000/512 */
    return (uint16_t)(((uint16_t)var)*125/64);
}


uint16_t plp_aperta_blackhawk_tsc_INTERNAL_eye_to_mV(srds_access_t *sa__, uint8_t var, uint8_t ladder_range) 
{       
    /* find nearest two vertical levels */
    uint16_t vl = (uint16_t)(plp_aperta_blackhawk_tsc_INTERNAL_ladder_setting_to_mV(sa__, (int8_t)var, ladder_range));    
    return (vl);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_osr_mode(srds_access_t *sa__, blackhawk_tsc_osr_mode_st *imode) {
    blackhawk_tsc_osr_mode_st mode;

     ENULL_MEMSET(&mode, 0, sizeof(blackhawk_tsc_osr_mode_st));

    if(!imode) 
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));

  ESTM(mode.tx = rd_tx_osr_mode());
  ESTM(mode.rx = rd_rx_osr_mode());
  mode.tx_rx = 255;
#ifdef wr_ams_rx_rxclk_div2p5    /* Analog 2p5 OSR mode */
  {
      uint8_t osr_2p5, osr_2p5_pwrup;
      ESTM(osr_2p5       = rd_ams_tx_txclk_div2p5());
      ESTM(osr_2p5_pwrup = rd_ams_tx_txclk_div2p5_pwrup());
      if ((mode.tx == BLACKHAWK_TSC_OSX1) && (osr_2p5 == 1)  && (osr_2p5_pwrup == 1)) {
          mode.tx = BLACKHAWK_TSC_OSX2P5;
      }
      ESTM(osr_2p5       = rd_ams_rx_rxclk_div2p5());
      ESTM(osr_2p5_pwrup = rd_ams_rx_rxclk_div2p5_pwrup());
      if ((mode.rx == BLACKHAWK_TSC_OSX1) && (osr_2p5 == 1)  && (osr_2p5_pwrup == 1)) {
          mode.rx = BLACKHAWK_TSC_OSX2P5;
      }
  }
#endif
  *imode = mode;
  return (ERR_CODE_NONE);

}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pam_mode(srds_access_t *sa__, enum blackhawk_tsc_rx_pam_mode_enum *pmode) {
    uint8_t rx_pam4_mode, rx_pam4_es_mode;

    ESTM(rx_pam4_mode    = rd_rx_pam4_mode());
    ESTM(rx_pam4_es_mode = rd_rx_pam4_er_mode());
    if (rx_pam4_mode == 0) {
        *pmode = NRZ;
    }
    else if (rx_pam4_mode == 1) {
        *pmode = (rx_pam4_es_mode) ? PAM4_ES : PAM4_NS;
    }
    else {
        return(blackhawk_tsc_error(sa__, ERR_CODE_INVALID_RX_PAM_MODE));
    }

  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_decode_br_os_mode(srds_access_t *sa__, uint8_t *br_pd_en){
    enum blackhawk_tsc_rx_pam_mode_enum pam_mode = NRZ;
    uint8_t os_pattern_enhanced=0, os_all_edges=0;

    ESTM(os_pattern_enhanced = rd_os_pattern_enhanced());
    ESTM(os_all_edges = rd_os_all_edges());
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
    if(pam_mode == NRZ) {
        ESTM(*br_pd_en = rd_br_pd_en());
    } else {
        *br_pd_en = (uint8_t)(!((!os_pattern_enhanced) & os_all_edges));
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_pmd_lock_status(srds_access_t *sa__, uint8_t *pmd_lock, uint8_t *pmd_lock_chg) {
    uint16_t rddata;
#if defined(reg_rd_TLB_RX_RXDBG_PMD_RX_LOCK_STATUS)
    ESTM(rddata = reg_rd_TLB_RX_RXDBG_PMD_RX_LOCK_STATUS());
    ESTM(*pmd_lock = ex_TLB_RX_RXDBG_PMD_RX_LOCK_STATUS__dbg_pmd_rx_lock(rddata));
    ESTM(*pmd_lock_chg = (uint8_t)(ex_TLB_RX_RXDBG_PMD_RX_LOCK_STATUS__dbg_pmd_rx_lock_change(rddata)));
#elif defined(reg_rd_TLB_RX_DBG_PMD_RX_LOCK_STATUS)
    ESTM(rddata = reg_rd_TLB_RX_DBG_PMD_RX_LOCK_STATUS());
    ESTM(*pmd_lock = ex_TLB_RX_DBG_PMD_RX_LOCK_STATUS__dbg_pmd_rx_lock(rddata));
    ESTM(*pmd_lock_chg = (uint8_t)(ex_TLB_RX_DBG_PMD_RX_LOCK_STATUS__dbg_pmd_rx_lock_change(rddata)));
#else
#error "Unknown core for plp_aperta_blackhawk_tsc_INTERNAL_pmd_lock_status()"    
#endif
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_sigdet_status(srds_access_t *sa__, uint8_t *sig_det, uint8_t *sig_det_chg) {
    uint16_t rddata;

    ESTM(rddata = reg_rd_SIGDET_SDSTATUS_0());
    ESTM(*sig_det = ex_SIGDET_SDSTATUS_0__signal_detect(rddata));
    ESTM(*sig_det_chg = (uint8_t)(ex_SIGDET_SDSTATUS_0__signal_detect_change(rddata)));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_pll_lock_status(srds_access_t *sa__, uint8_t *pll_lock, uint8_t *pll_lock_chg) {

    uint16_t rddata;
    ESTM(rddata = reg_rdc_PLL_CAL_COM_CTL_STATUS_0());
    ESTM(*pll_lock = (uint8_t)(exc_PLL_CAL_COM_CTL_STATUS_0__pll_lock(rddata)));
    ESTM(*pll_lock_chg = exc_PLL_CAL_COM_CTL_STATUS_0__pll_lock_lh_ll(rddata));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_read_lane_state(srds_access_t *sa__, blackhawk_tsc_lane_state_st *istate) {
       
  blackhawk_tsc_lane_state_st state;

  ENULL_MEMSET(&state, 0, sizeof(blackhawk_tsc_lane_state_st));

  if(!istate) 
    return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));

  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_pmd_lock_status(sa__,&state.rx_lock, &state.rx_lock_chg));
  {
      err_code_t err_code = 0;
      state.stop_state = plp_aperta_blackhawk_tsc_INTERNAL_stop_micro(sa__,state.rx_lock,&err_code);
      if(err_code) USR_PRINTF(("WARNING: Unable to stop microcontroller,  following data is suspect\n"));
  }

  {  blackhawk_tsc_osr_mode_st osr_mode;
  ENULL_MEMSET(&osr_mode, 0, sizeof(blackhawk_tsc_osr_mode_st));
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_osr_mode(sa__, &osr_mode));
  state.osr_mode = osr_mode;
  }
  {
     enum blackhawk_tsc_rx_pam_mode_enum pam_mode = NRZ;
     EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
     state.rx_pam_mode = (uint8_t)pam_mode;
  }
  {
      struct blackhawk_tsc_uc_lane_config_st lane_cfg;
      EFUN(plp_aperta_blackhawk_tsc_get_uc_lane_cfg(sa__, &lane_cfg));
      state.ucv_config = lane_cfg.word;
  }
  ESTM(state.ucv_status = rdv_status_byte());
  ESTM(state.ucv_status_ext = rdv_ext_status_word());
  ESTM(state.reset_state    = rd_rx_lane_dp_reset_state());
  ESTM(state.tx_reset_state = rd_tx_lane_dp_reset_state());
  ESTM(state.tx_pll_select = rd_tx_pll_select());
  ESTM(state.rx_pll_select = rd_rx_pll_select());
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_sigdet_status(sa__,&state.sig_det, &state.sig_det_chg));
  ESTM(state.rx_ppm = rd_cdr_integ_reg()/84);
  {
      int8_t reg_data;
      ESTM(reg_data = rdv_usr_sts_phase_hoffset());
      state.clk90 = reg_data;
  }
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_decode_br_os_mode(sa__, &state.br_pd_en));
  /* drop the MSB, the result is actually valid modulo 128 */
  /* Also flip the sign to account for d-m1, versus m1-d */
  
  EFUN(plp_aperta_blackhawk_tsc_read_rx_afe(sa__, RX_AFE_PF, &state.pf_main));
  state.pf_hiz = 0;
  EFUN(plp_aperta_blackhawk_tsc_read_rx_afe(sa__, RX_AFE_PF2, &state.pf2_ctrl));
  EFUN(plp_aperta_blackhawk_tsc_read_rx_afe(sa__, RX_AFE_PF3, &state.pf3_ctrl));
  ESTM(state.bwr_ctrl = (int8_t)rdv_afe_bw_adjust());
  EFUN(plp_aperta_blackhawk_tsc_read_rx_afe(sa__, RX_AFE_VGA, &state.vga));

  ESTM(state.dc_offset = rd_dc_offset_bin());
  ESTM(state.p1_lvl = rdv_usr_main_tap_est()/32);
  state.p1_lvl = plp_aperta_blackhawk_tsc_INTERNAL_ladder_setting_to_mV(sa__, (int8_t)state.p1_lvl, 0);
  if (state.rx_pam_mode == NRZ) {
      ESTM(state.m1_lvl = rd_rx_phase02_status());
      state.m1_lvl = plp_aperta_blackhawk_tsc_INTERNAL_ladder_setting_to_mV(sa__, (int8_t)state.m1_lvl, 0);
      state.usr_status_eq_debug1 = 0;
      state.afe_bw = 0;
  }
  else {
      ESTM(state.m1_lvl = rdv_usr_ctrl_pam4_chn_loss());

      ESTM(state.usr_status_eq_debug1 = rdv_usr_status_eq_debug1());
      {
          uint16_t ucode_version_major;
          uint8_t ucode_version_minor;
          /*Extracting the major version number*/
          ESTM(ucode_version_major = rdcv_common_ucode_version() & 0xFFF);
          ESTM(ucode_version_minor = rdcv_common_ucode_minor_version());
          if ((ucode_version_major == 0x2) && (ucode_version_minor >= 0x3) && (ucode_version_minor < 0x8)) {
              uint8_t afe_bw_dbg0, afe_bw_dbg1;
              uint8_t afe_bw[4] = {1, 2, 0, 3};
              ESTM(afe_bw_dbg0 = rdv_afe_bw_dbg_0());
              ESTM(afe_bw_dbg1 = rdv_afe_bw_dbg_1());

              afe_bw_dbg0 &= 0x01;
              afe_bw_dbg1 &= 0x01;
              state.afe_bw = afe_bw[((afe_bw_dbg0 << 1) | afe_bw_dbg1)];
          } else if (((ucode_version_major == 0x2) && (ucode_version_minor >= 0x8)) || (ucode_version_major > 0x2)) {
              /*tag D002_08 has the changes to afe_bw calculation along with trunk before branching for tag D002_08*/
              ESTM(state.afe_bw = rdv_afe_bw_dbg_0());
          } else {
              state.afe_bw = 0xFF; /* assign to a invalid value. */
          }
      }
  }

  ESTM(state.blind_ctle_ctrl = rdv_usr_status_eq_debug3());
  ESTM(state.blind_ctle_trnsum = rdv_usr_status_eq_debug14());
  if ((state.blind_ctle_ctrl == 0) && (state.blind_ctle_trnsum < 0)) {
      state.afe_bw_metric = state.blind_ctle_trnsum;
  } else {
      state.afe_bw_metric = state.blind_ctle_ctrl;
  } 
  ESTM(state.tp_metric_prec1 = rdv_usr_status_tp_metric_1());

  if (state.rx_pam_mode == NRZ) {
      EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe1(sa__, &state.dfe1));
  }
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe2(sa__, &state.dfe2));  
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe3(sa__, &state.dfe3));  
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe4(sa__, &state.dfe4));
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe5(sa__, &state.dfe5));

  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_dfe6(sa__, &state.dfe6));

  /* tx_ppm = register/10.84 */
  ESTM(state.tx_ppm = (int16_t)(((int32_t)(rd_tx_pi_integ2_reg()))*3125/32768));

  ESTM(state.tx_disable_status = rd_tx_disable_status());
  ESTM(state.tx_prec_en = rd_pam4_precoder_en());
  ESTM(state.linktrn_en = rd_linktrn_ieee_training_enable());
  ESTM(state.txfir_use_pam4_range = rd_txfir_nrz_tap_range_sel() ? 0 : 1);
  {
      uint8_t tap_num;
      for (tap_num=0; tap_num<12; ++tap_num) {
          EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_tx_tap(sa__, tap_num, &state.txfir.tap[tap_num]));
      }
  }
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_eye_margin_est(sa__, &state.heye_left, &state.heye_right, &state.veye_upper, &state.veye_lower));
/* TODO: KD, For Blackhawk PAM4 mode, Eye margin display should show minimum value for each of the above 4 variables among the 3 eyes */ 
  ESTM(state.link_time = (uint16_t)((((uint32_t)rdv_usr_sts_link_time())*8)/10));     /* convert from 80us to 0.1 ms units */
  {
      /* read lock status at end and combine them to handle transient cases */
      uint8_t rx_lock_at_end=0, rx_lock_chg_at_end=0;
      EFUN(plp_aperta_blackhawk_tsc_INTERNAL_pmd_lock_status(sa__,&rx_lock_at_end, &rx_lock_chg_at_end));
      if (state.rx_lock != rx_lock_at_end) {
          USR_PRINTF(("rx_lock has changed while reading the lane state rx_lock_start=%d, rx_lock_chg_start=%d, rx_lock_at_end=%d, rx_lock_chg_at_end=%d\n",
                      state.rx_lock,
                      state.rx_lock_chg,
                      rx_lock_at_end,
                      rx_lock_chg_at_end));
      }
      state.rx_lock &= rx_lock_at_end;
      state.rx_lock_chg |= rx_lock_chg_at_end;
  }

  if (!state.stop_state || (state.stop_state == 0xFF)) {
      /* manually check error code instead of EFUN*/
      {
          err_code_t resume_status = ERR_CODE_NONE;
          resume_status = plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__, 0);
          if (resume_status) {
              USR_PRINTF(("WARNING: Resuming micro after lane state capture failed \n"));
          }
      }
  } 

  *istate = state;


  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_display_lane_state_no_newline(srds_access_t *sa__) {     
  uint16_t lane_idx;
  blackhawk_tsc_lane_state_st state;

  const char* e2s_osr_mode_enum[14] = {
    "x1   ",
    "x2   ",
    "x4   ",
    "ERR  ",
    "x21.2",
    "x8   ",
    "ERR  ",
    "ERR  ",
    "x16.5",
    "x16  ",
    "ERR  ",
    "ERR  ",
    "x20.6",
    "x32  "
  };

  const char* e2s_tx_osr_mode_enum[14] = {
    "x1",
    "x2",
    "x4",
    "ER",
    "xL",
    "x8",
    "ER",
    "ER",
    "xH",
    "xG",
    "ER",
    "ER",
    "xK",
    "xW"
  }; 

  const char* plp_aperta_e2s_rx_pam_mode_enum[3] = {
    "NRZ",
    "P4N",
    "P4E"
  };  

  ENULL_MEMSET(&state, 0, sizeof(blackhawk_tsc_lane_state_st));

  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_read_lane_state(sa__, &state));
 
  lane_idx = plp_aperta_blackhawk_tsc_get_lane(sa__); 
  EFUN_PRINTF(("%2d ", lane_idx));
  if(state.osr_mode.tx_rx != 255) {
      char *s;
      s = (char *)e2s_osr_mode_enum[state.osr_mode.tx_rx];
      EFUN_PRINTF(("(%2s%s, 0x%04x, 0x%02x,  0x%04x,   ", (state.br_pd_en) ? "BR" : "OS", s, state.ucv_config,state.ucv_status,state.ucv_status_ext));
  } else {
      char *s;
      char *r;
      s = (char *)e2s_tx_osr_mode_enum[state.osr_mode.tx];
      r = (char *)e2s_tx_osr_mode_enum[state.osr_mode.rx];
      {
          char *t;
          uint8_t tx, rx;
          t = (char *)plp_aperta_e2s_rx_pam_mode_enum[state.rx_pam_mode];
          ESTM(tx = rd_tx_pam4_mode()); 
          ESTM(rx = rd_rx_pam4_mode());
          EFUN_PRINTF(("(%3s%s,%2s%s:%s, 0x%04x, 0x%02x_%04x,", t,(tx == rx) ? " ": "~", (state.br_pd_en) ? "BR" : "OS", s,r, state.ucv_config,state.ucv_status,state.ucv_status_ext));
      }
  }
  EFUN_PRINTF((" %01x,%01x, %02x, %1d,%1d)",state.tx_reset_state,state.reset_state,state.stop_state,state.tx_pll_select,state.rx_pll_select));
  EFUN_PRINTF((" %1d%s", state.sig_det, state.sig_det_chg ? "*" : " "));
  EFUN_PRINTF((" %1d%s ", state.rx_lock, state.rx_lock_chg ? "*" : " "));
  EFUN_PRINTF(("%4d ", state.rx_ppm));
  EFUN_PRINTF(("  %3d ", state.clk90));
  EFUN_PRINTF(("  (%2d,%1d,%2d,%2d)", state.pf_main, state.pf2_ctrl, state.pf3_ctrl, state.bwr_ctrl));
  EFUN_PRINTF((" %2d ", state.vga));
  EFUN_PRINTF(("%3d  ", state.dc_offset));
  EFUN_PRINTF(("%3d ", state.p1_lvl));
  if (state.afe_bw == 0xFF) {
      /* afe_bw calculation is not supported in this microcode version and so invalid value 0xFF*/
      EFUN_PRINTF(("(%4d,%+5d, x,%3d,%3d) ", 
                state.m1_lvl, /* pam4 _chn_loss */
                state.afe_bw_metric,
                state.usr_status_eq_debug1,
                state.tp_metric_prec1));
  } else {
      EFUN_PRINTF(("(%4d,%+5d,%2d,%3d,%3d) ", 
                state.m1_lvl, /* pam4 _chn_loss */
                state.afe_bw_metric,
                state.afe_bw,
                state.usr_status_eq_debug1,
                state.tp_metric_prec1));
  }
  if (state.rx_pam_mode == NRZ) {
      EFUN_PRINTF(("(%3d,%3d,%3d,%3d,%3d,%3d)", state.dfe1, state.dfe2, state.dfe3, state.dfe4, state.dfe5, state.dfe6));
  }
  else {
      EFUN_PRINTF(("( x ,%3d,%3d,%3d,%3d,%3d)", state.dfe2, state.dfe3, state.dfe4, state.dfe5, state.dfe6));
  }
  EFUN_PRINTF((" %4d ", state.tx_ppm));
  {
      uint8_t enable_6taps;
      if(state.tx_disable_status) {
          EFUN_PRINTF((" D "));
      }
      else {
          if (state.tx_prec_en) {
              EFUN_PRINTF((" P"));
          }
          else {
              EFUN_PRINTF(("  "));
          }
          if(state.linktrn_en) {
              EFUN_PRINTF(("T"));
          }
          else {
              EFUN_PRINTF((" "));
          }
      }
      ESTM(enable_6taps = rd_txfir_tap_en());
      if (!enable_6taps) {
          EFUN_PRINTF(("( x ,%3d,%3d,%3d, x , x ) ", state.txfir.tap[0], state.txfir.tap[1], state.txfir.tap[2]));
      }
      else {
          EFUN_PRINTF(("(%3d,%3d,%3d,%3d,%3d,%3d) ",
                       state.txfir.tap[0], state.txfir.tap[1], state.txfir.tap[2],
                       state.txfir.tap[3], state.txfir.tap[4], state.txfir.tap[5]));
      }
  }
  EFUN_PRINTF(("(%3d,%3d,%3d,%3d) ", state.heye_left, state.heye_right, state.veye_upper, state.veye_lower));
  /* Check to see if link_time is max value after 80us to 0.1msec unit conversion */
  if (state.link_time == 0xCCCC) {
      EFUN_PRINTF((" >%4d.%01d", state.link_time/10, state.link_time%10));
      EFUN_PRINTF((" "));  
  } else {
      EFUN_PRINTF((" %4d.%01d", state.link_time/10, state.link_time%10));
      EFUN_PRINTF(("  "));  
  }
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_display_BER(sa__,100));

  
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_read_core_state(srds_access_t *sa__, blackhawk_tsc_core_state_st *istate) {  
  blackhawk_tsc_core_state_st state;
  struct blackhawk_tsc_uc_core_config_st core_cfg;

  ENULL_MEMSET(&state, 0, sizeof(blackhawk_tsc_core_state_st));
  ENULL_MEMSET(&core_cfg, 0, sizeof(struct blackhawk_tsc_uc_core_config_st));

  if(!istate) 
    return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  EFUN(plp_aperta_blackhawk_tsc_get_uc_core_config(sa__, &core_cfg));
  /* BLACKHAWK TODO:  This is copied from Falcon16. */
  {
      uint8_t rescal_frc;
      ESTM(rescal_frc = rdc_rescal_frc());
      if (1 == rescal_frc) {
          /* rescal_frc_val is read, since the rescal value is overriden. */
          ESTM(state.rescal = rdc_rescal_frc_val());
      } else {
          ESTM(state.rescal = rd_ams_tx_ana_rescal());
      }
  }
  ESTM(state.core_reset           = rdc_core_dp_reset_state());
  ESTM(state.uc_active            = rdc_uc_active());
  ESTM(state.comclk_mhz           = rdc_heartbeat_count_1us());   
  ESTM(state.pll_pwrdn            = rdc_pll_pwrdn_or());
  ESTM(state.ucode_version        = rdcv_common_ucode_version()); 
  EFUN(plp_aperta_blackhawk_tsc_version(sa__, &state.api_version));
  ESTM(state.ucode_minor_version  = rdcv_common_ucode_minor_version());
  ESTM(state.afe_hardware_version = rdcv_afe_hardware_version());
  ESTM(state.temp_idx             = rdcv_temp_idx());
  {  int16_t                           die_temp = 0;
     EFUN(plp_aperta_blackhawk_tsc_read_die_temperature(sa__, &die_temp));
     state.die_temp               =    die_temp;
  }
  ESTM(state.avg_tmon             = (int16_t)(_bin_to_degC(rdcv_avg_tmon_reg13bit()>>3)));
  state.vco_rate_mhz            = (uint16_t)core_cfg.vco_rate_in_Mhz;
  ESTM(state.analog_vco_range     = rdc_ams_pll_range());
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_measure_pll_comp_thresh(sa__, &state.pll_comp_thresh));
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_read_pll_div(sa__, &state.pll_div));
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_pll_lock_status(sa__, &state.pll_lock, &state.pll_lock_chg));
  ESTM(state.core_status          = rdcv_status_byte());
  ESTM(state.refclk_doubler       = rdc_ams_pll_refclk_doubler());

  *istate = state;
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_measure_pll_comp_thresh(srds_access_t *sa__, int16_t *thresh) {
    uint8_t pll_comp_thresh=0;
    uint8_t pll_low=0;
    uint8_t pll_comp_thresh_bak;

    if(!thresh) 
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));

    ESTM(pll_comp_thresh_bak = rdc_ams_pll_comp_thresh());
    
    for(pll_comp_thresh = 0; pll_comp_thresh < 8; pll_comp_thresh++) {
        EFUN(wrc_ams_pll_comp_thresh(pll_comp_thresh));
        ESTM(pll_low = rdc_ams_pll_low());
        if(pll_low == 1) break;
    }
    
    EFUN(wrc_ams_pll_comp_thresh(pll_comp_thresh_bak));

    *thresh = ((pll_comp_thresh==0) ? 0 : (int16_t)(-50*pll_comp_thresh - 50));

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_display_core_state_no_newline(srds_access_t *sa__) {
  blackhawk_tsc_core_state_st state;
    blackhawk_tsc_core_state_st state_pll1;

    ENULL_MEMSET(&state_pll1, 0, sizeof(state_pll1));
    ENULL_MEMSET(&state     , 0, sizeof(state     ));
    {
        uint8_t pll_orig;
        ESTM(pll_orig = plp_aperta_blackhawk_tsc_get_pll_idx(sa__));
        EFUN(plp_aperta_blackhawk_tsc_set_pll_idx(sa__, 0));
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_read_core_state(sa__, &state));
        EFUN(plp_aperta_blackhawk_tsc_set_pll_idx(sa__, 1));
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_read_core_state(sa__, &state_pll1));
        EFUN(plp_aperta_blackhawk_tsc_set_pll_idx(sa__, pll_orig));
    }

    if ((state.avg_tmon<-50)||(state.avg_tmon>135)) {
      EFUN_PRINTF(("\n*** WARNING: Core die temperature (LIVE_TEMP) out of bounds -50C to 130C\n"));
    }
    if ((state.rescal < 6) || (state.rescal > 13)) {
      EFUN_PRINTF(("\n*** WARNING: RESCAL value is out of bounds 6 to 13\n"));
    }
    if ((state_pll1.rescal < 6) || (state_pll1.rescal > 13)) {
      EFUN_PRINTF(("\n*** WARNING: RESCAL PLL1 value is out of bounds 6 to 13\n"));
    }

    EFUN_PRINTF((" %02d "              ,  plp_aperta_blackhawk_tsc_get_core(sa__)));
    EFUN_PRINTF((" %x,%x,%02x "        ,  state.core_reset, state_pll1.core_reset, state.core_status));
    EFUN_PRINTF(("   %1d,%1d    "      ,  state.pll_pwrdn,state_pll1.pll_pwrdn));
    EFUN_PRINTF(("   %1d    "          ,  state.uc_active));
    EFUN_PRINTF((" %3d.%2dMHz"         , (state.comclk_mhz/4),((state.comclk_mhz%4)*25)));    /* comclk in Mhz = heartbeat_count_1us / 4 */
    EFUN_PRINTF(("   %4X_%02X "        ,  state.ucode_version, state.ucode_minor_version));
    EFUN_PRINTF(("  %06X "             , state.api_version));
    EFUN_PRINTF(("    0x%02x   "       ,  state.afe_hardware_version));
    EFUN_PRINTF(("   %3dC   "          ,  state.die_temp));
    EFUN_PRINTF(("   (%02d)%3dC "      ,  state.temp_idx, state.avg_tmon));
    EFUN_PRINTF(("   0x%02x  "         ,  state.rescal));
    EFUN_PRINTF((" %2d.%03d,%2d.%03dGHz ", (state     .vco_rate_mhz / 1000),
                                           (state     .vco_rate_mhz % 1000),
                                           (state_pll1.vco_rate_mhz / 1000),
                                           (state_pll1.vco_rate_mhz % 1000)));
    EFUN_PRINTF(("  %03d,%03d     "    ,  state     .analog_vco_range,
                                          state_pll1.analog_vco_range));
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_display_pll_to_divider(sa__, state .pll_div));
    EFUN_PRINTF((","));
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_display_pll_to_divider(sa__, state_pll1.pll_div));
    EFUN_PRINTF(("   %01d%s,"         ,  state.pll_lock, state.pll_lock_chg ? "*" : " "));
    EFUN_PRINTF((" %01d%s"             ,  state_pll1.pll_lock, state_pll1.pll_lock_chg ? "*" : " "));
    EFUN_PRINTF(("  %+4dmV,%+4dmV "       ,  state.pll_comp_thresh,state_pll1.pll_comp_thresh));


    return (ERR_CODE_NONE);
  }



/* returns 000111 (7 = 8-1), for n = 3  */
#define BFMASK(n) ((1<<((n)))-1)

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_update_uc_lane_config_st(struct  blackhawk_tsc_uc_lane_config_st *st) {
  uint16_t in = st->word;
  st->field.lane_cfg_from_pcs = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.an_enabled = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.dfe_on = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.dfe_lp_mode = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_brdfe_on = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.media_type = in & BFMASK(2); in = (uint16_t)(in >> 2);
  st->field.unreliable_los = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.scrambling_dis = in & BFMASK(1); in = (uint16_t)(in >> 1);  
  st->field.cl72_auto_polarity_en = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.cl72_restart_timeout_en = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_es = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_ns = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.lp_has_prec_en = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_pam4_mode = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_nrz_mode = in & BFMASK(1);
  return(ERR_CODE_NONE);
}
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_functions_st(struct blackhawk_tsc_usr_ctrl_disable_functions_st *st) {
  uint16_t in = st->word;
  st->field.pf_adaptation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.pf2_adaptation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.pf3_adaptation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.dc_adaptation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.vga_adaptation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.slicer_voffset_tuning = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.slicer_hoffset_tuning = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.phase_offset_adaptation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.eye_adaptation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.all_adaptation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.deq_adaptation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.pam_eye_margin_estimation = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.reserved = in & BFMASK(4);
  return ERR_CODE_NONE;
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_dfe_functions_st(struct blackhawk_tsc_usr_ctrl_disable_dfe_functions_st *st) {
  uint8_t in = st->byte;
  st->field.dfe_tap1_adaptation = in & BFMASK(1); in = (uint8_t)(in >> 1);
  st->field.dfe_fx_taps_adaptation = in & BFMASK(1); in = (uint8_t)(in >> 1);
  st->field.dfe_fl_taps_adaptation = in & BFMASK(1); in = (uint8_t)(in >> 1);
  st->field.dfe_dcd_adaptation = in & BFMASK(1);
  return ERR_CODE_NONE;
}
/* word to fields, for display */
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_update_uc_core_config_st(struct blackhawk_tsc_uc_core_config_st *st) {
  uint16_t in = st->word;
  st->field.vco_rate = (uint8_t)(in & BFMASK(8)); in = (uint16_t)(in >> 8);
  st->field.core_cfg_from_pcs = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.reserved = in & BFMASK(7);
  st->vco_rate_in_Mhz = VCO_RATE_TO_MHZ(st->field.vco_rate);
  return ERR_CODE_NONE;
}

/* fields to word, to write into uC RAM */
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_update_uc_core_config_word(struct blackhawk_tsc_uc_core_config_st *st) {
  uint16_t in = 0;
  in = (uint16_t)(in << 7); in |= 0/*st->field.reserved*/ & BFMASK(7);
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.core_cfg_from_pcs & BFMASK(1)));
  in = (uint16_t)(in << 8); in = (uint16_t)(in | (st->field.vco_rate & BFMASK(8)));
  st->word = in;
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_update_uc_lane_config_word(struct blackhawk_tsc_uc_lane_config_st *st) {
  uint16_t in = 0;
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.force_nrz_mode & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.force_pam4_mode & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.lp_has_prec_en & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.force_ns & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.force_es & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.cl72_restart_timeout_en & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.cl72_auto_polarity_en & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.scrambling_dis & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.unreliable_los & BFMASK(1)));
  in = (uint16_t)(in << 2); in = (uint16_t)(in | (st->field.media_type & BFMASK(2)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.force_brdfe_on & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.dfe_lp_mode & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.dfe_on & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.an_enabled & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.lane_cfg_from_pcs & BFMASK(1)));
  st->word = in;
  return ERR_CODE_NONE;
}
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_functions_byte(struct blackhawk_tsc_usr_ctrl_disable_functions_st *st) {
  uint16_t in = 0;
  in = (uint16_t)(in << 5); in |= 0/*st->field.reserved*/ & BFMASK(4);
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.pam_eye_margin_estimation & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.deq_adaptation & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.all_adaptation & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.eye_adaptation & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.phase_offset_adaptation & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.slicer_hoffset_tuning & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.slicer_voffset_tuning & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.vga_adaptation & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.dc_adaptation & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.pf3_adaptation & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.pf2_adaptation & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.pf_adaptation & BFMASK(1)));
  st->word = in;
  return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_dfe_functions_byte(struct  blackhawk_tsc_usr_ctrl_disable_dfe_functions_st *st) {
  uint8_t in = 0;
  in = (uint8_t)(in << 1); in = (uint8_t)(in | (st->field.dfe_dcd_adaptation & BFMASK(1)));
  in = (uint8_t)(in << 1); in = (uint8_t)(in | (st->field.dfe_fl_taps_adaptation & BFMASK(1)));
  in = (uint8_t)(in << 1); in = (uint8_t)(in | (st->field.dfe_fx_taps_adaptation & BFMASK(1)));
  in = (uint8_t)(in << 1); in = (uint8_t)(in | (st->field.dfe_tap1_adaptation & BFMASK(1)));
  st->byte = in;
  return ERR_CODE_NONE;
}

#undef BFMASK

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_check_uc_lane_stopped(srds_access_t *sa__) {

  uint8_t is_micro_stopped;
  ESTM(is_micro_stopped = rdv_usr_sts_micro_stopped() || (rd_rx_lane_dp_reset_state() > 0));
  if (!is_micro_stopped) {
      return(blackhawk_tsc_error(sa__, ERR_CODE_UC_NOT_STOPPED));
  } else {
      return(ERR_CODE_NONE);
  }
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_calc_patt_gen_mode_sel(srds_access_t *sa__, uint8_t *mode_sel, uint8_t *zero_pad_len, uint8_t patt_length) {

  if(!mode_sel) {
    return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
  if(!zero_pad_len) {
    return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  /* Select the correct Pattern generator Mode depending on Pattern length */
  if (!(140 % patt_length)) {
    *mode_sel = 6;
    *zero_pad_len = 100;
  }
  else if (!(160 % patt_length)) {
    *mode_sel = 5;
    *zero_pad_len = 80;
  }
  else if (!(180 % patt_length)) {
    *mode_sel = 4;
    *zero_pad_len = 60;
  }
  else if (!(200 % patt_length)) {
    *mode_sel = 3;
    *zero_pad_len = 40;
  }
  else if (!(220 % patt_length)) {
    *mode_sel = 2;
    *zero_pad_len = 20;
  }
  else if (!(240 % patt_length)) {
    *mode_sel = 1;
    *zero_pad_len = 0;
  } else {
    EFUN_PRINTF(("ERROR: Unsupported Pattern Length\n"));
    return (blackhawk_tsc_error(sa__, ERR_CODE_CFG_PATT_INVALID_PATT_LENGTH));
  }
  return(ERR_CODE_NONE);
}

/*-----------------------------------------*/
/*  Write Core Config variables to uC RAM  */
/*-----------------------------------------*/

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_set_uc_core_config(srds_access_t *sa__, struct blackhawk_tsc_uc_core_config_st struct_val) {
    {   uint8_t reset_state;
        ESTM(reset_state = rdc_core_dp_reset_state());
        if(reset_state < 7) {
            EFUN_PRINTF(("ERROR: plp_aperta_blackhawk_tsc_INTERNAL_set_uc_core_config(..) called without core_dp_s_rstb=0\n"));
            return (blackhawk_tsc_error(sa__, ERR_CODE_CORE_DP_NOT_RESET));
        }
    }
    if(struct_val.vco_rate_in_Mhz > 0) {
        struct_val.field.vco_rate = MHZ_TO_VCO_RATE(struct_val.vco_rate_in_Mhz);
    }
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_update_uc_core_config_word(&struct_val));
    switch (plp_aperta_blackhawk_tsc_get_pll_idx(sa__)) {
    case 0:
        EFUN(reg_wrc_CORE_PLL_COM_PLL_UC_CORE_CONFIG(struct_val.word));
        break;
    case 1:
        EFUN(reg_wrc_CORE_PLL_COM_PLL_UC_CORE_CONFIG(struct_val.word));
        break;
    default:
        EFUN(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
        break;
    }
    return (ERR_CODE_NONE);
}

/*---------------------*/
/*  PLL Configuration  */
/*---------------------*/
/** Extract the refclk frequency in Hz, based on a blackhawk_tsc_pll_refclk_enum value. */
err_code_t _plp_aperta_blackhawk_tsc_get_refclk_in_hz(srds_access_t *sa__, enum blackhawk_tsc_pll_refclk_enum refclk, uint32_t *refclk_in_hz) {
    switch (refclk) {
        case BLACKHAWK_TSC_PLL_REFCLK_100MHZ:          *refclk_in_hz = 100000000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_106P25MHZ:       *refclk_in_hz = 106250000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_122P88MHZ:       *refclk_in_hz = 122880000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_125MHZ:          *refclk_in_hz = 125000000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_145P947802MHZ:   *refclk_in_hz = 145947802; break;
        case BLACKHAWK_TSC_PLL_REFCLK_155P52MHZ:       *refclk_in_hz = 155520000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_156P25MHZ:       *refclk_in_hz = 156250000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_159P375MHZ:      *refclk_in_hz = 159375000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_161P1328125MHZ:  *refclk_in_hz = 161132813; break;
        case BLACKHAWK_TSC_PLL_REFCLK_166P67MHZ:       *refclk_in_hz = 166670000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_166P7974882MHZ:  *refclk_in_hz = 166797488; break;
        case BLACKHAWK_TSC_PLL_REFCLK_167P38MHZ:       *refclk_in_hz = 167380000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_167P4107143MHZ:  *refclk_in_hz = 167410714; break;
        case BLACKHAWK_TSC_PLL_REFCLK_174P703125MHZ:   *refclk_in_hz = 174703125; break;
        case BLACKHAWK_TSC_PLL_REFCLK_212P5MHZ:        *refclk_in_hz = 212500000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_311p04:          *refclk_in_hz = 311040000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_312P5MHZ:        *refclk_in_hz = 312500000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_425MHZ:          *refclk_in_hz = 425000000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_534MHZ:          *refclk_in_hz = 534000000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_705MHZ:          *refclk_in_hz = 705000000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_175MHZ:          *refclk_in_hz = 175000000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_400MHZ:          *refclk_in_hz = 400000000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_266P67MHZ:       *refclk_in_hz = 266670000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_156P637MHZ:      *refclk_in_hz = 156637000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_157P844MHZ:      *refclk_in_hz = 157844000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_158P51MHZ:       *refclk_in_hz = 158510000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_162P9485MHZ:     *refclk_in_hz = 162948500; break;
        case BLACKHAWK_TSC_PLL_REFCLK_167P331MHZ:      *refclk_in_hz = 167331000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_168P04MHZ:       *refclk_in_hz = 168040000; break;
        case BLACKHAWK_TSC_PLL_REFCLK_169P40905086MHZ: *refclk_in_hz = 169409051; break;
        case BLACKHAWK_TSC_PLL_REFCLK_173P37MHZ:       *refclk_in_hz = 173370000; break;
        default:
            EFUN_PRINTF(("ERROR: Unknown refclk frequency:  0x%08X\n", (uint32_t)refclk));
            *refclk_in_hz = 0xFFFFFFFF;
            return (blackhawk_tsc_error(sa__, ERR_CODE_REFCLK_FREQUENCY_INVALID));
    }
    return (ERR_CODE_NONE);
}

/** Identify the ratio:
 *
 *     (numerator / denominator) = (1000 / divisor)
 *
 * such that this has as little rounding error as possible:
 *
 *     refclk_freq_hz = numerator * round(vco_freq_khz / denominator)
 *
 * This will yield the most accurate refclk_freq_hz.
 * Common values of vco_freq_khz are considered in this.
 */
static err_code_t _plp_aperta_blackhawk_tsc_get_divisor_ratio(srds_access_t *sa__, enum blackhawk_tsc_pll_div_enum srds_div, uint16_t *numerator, uint16_t *denominator)
{
    switch (srds_div) {
        case BLACKHAWK_TSC_PLL_DIV_66:         *denominator =  33;  *numerator =  500; break;
        case BLACKHAWK_TSC_PLL_DIV_67:         *denominator =  67;  *numerator = 1000; break;
        case BLACKHAWK_TSC_PLL_DIV_70:         *denominator =   7;  *numerator =  100; break;
        case BLACKHAWK_TSC_PLL_DIV_72:         *denominator =   9;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_76:         *denominator =  19;  *numerator =  250; break;
        case BLACKHAWK_TSC_PLL_DIV_73P6:       *denominator =  46;  *numerator =  625; break;
        case BLACKHAWK_TSC_PLL_DIV_79P2:       *denominator =  99;  *numerator = 1250; break;
        case BLACKHAWK_TSC_PLL_DIV_80:         *denominator =   2;  *numerator =   25; break;
        case BLACKHAWK_TSC_PLL_DIV_82P5:       *denominator =  33;  *numerator =  400; break;
        case BLACKHAWK_TSC_PLL_DIV_84:         *denominator =  21;  *numerator =  250; break;
        case BLACKHAWK_TSC_PLL_DIV_85:         *denominator =  17;  *numerator =  200; break;
        case BLACKHAWK_TSC_PLL_DIV_87P5:       *denominator =   7;  *numerator =   80; break;
        case BLACKHAWK_TSC_PLL_DIV_89P6:       *denominator =  56;  *numerator =  625; break;
        case BLACKHAWK_TSC_PLL_DIV_90:         *denominator =   9;  *numerator =  100; break;
        case BLACKHAWK_TSC_PLL_DIV_96:         *denominator =  12;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_100:        *denominator =   1;  *numerator =   10; break;
        case BLACKHAWK_TSC_PLL_DIV_120:        *denominator =   3;  *numerator =   25; break;
        case BLACKHAWK_TSC_PLL_DIV_127P401984: *denominator = 179;  *numerator = 1405; break;
        case BLACKHAWK_TSC_PLL_DIV_128:        *denominator =  16;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_132:        *denominator =  33;  *numerator =  250; break;
        case BLACKHAWK_TSC_PLL_DIV_134:        *denominator =  67;  *numerator =  500; break;
        case BLACKHAWK_TSC_PLL_DIV_140:        *denominator =   7;  *numerator =   50; break;
        case BLACKHAWK_TSC_PLL_DIV_144:        *denominator =  18;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_147P2:      *denominator =  92;  *numerator =  625; break;
        case BLACKHAWK_TSC_PLL_DIV_148:        *denominator =  37;  *numerator =  250; break;
        case BLACKHAWK_TSC_PLL_DIV_152:        *denominator =  19;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_158P4:      *denominator =  99;  *numerator =  625; break;
        case BLACKHAWK_TSC_PLL_DIV_160:        *denominator =   4;  *numerator =   25; break;
        case BLACKHAWK_TSC_PLL_DIV_165:        *denominator =  33;  *numerator =  200; break;
        case BLACKHAWK_TSC_PLL_DIV_168:        *denominator =  21;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_170:        *denominator =  17;  *numerator =  100; break;
        case BLACKHAWK_TSC_PLL_DIV_175:        *denominator =   7;  *numerator =   40; break;
        case BLACKHAWK_TSC_PLL_DIV_178P895958: *denominator = 178;  *numerator =  995; break;
        case BLACKHAWK_TSC_PLL_DIV_180:        *denominator =   9;  *numerator =   50; break;
        case BLACKHAWK_TSC_PLL_DIV_184:        *denominator =  23;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_192:        *denominator =  24;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_198:        *denominator =  99;  *numerator =  500; break;
        case BLACKHAWK_TSC_PLL_DIV_200:        *denominator =   1;  *numerator =    5; break;
        case BLACKHAWK_TSC_PLL_DIV_224:        *denominator =  28;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_240:        *denominator =   6;  *numerator =   25; break;
        case BLACKHAWK_TSC_PLL_DIV_264:        *denominator =  33;  *numerator =  125; break;
        case BLACKHAWK_TSC_PLL_DIV_280:        *denominator =   7;  *numerator =   25; break;
        case BLACKHAWK_TSC_PLL_DIV_330:        *denominator =  33;  *numerator =  100; break;
        case BLACKHAWK_TSC_PLL_DIV_350:        *denominator =   7;  *numerator =   20; break;
#if defined (SERDES_PLP8LP2)
        case BLACKHAWK_TSC_PLL_DIV_108:         *denominator =    27;  *numerator =   250; break;
        case BLACKHAWK_TSC_PLL_DIV_110:         *denominator =    11;  *numerator =   100; break;
        case BLACKHAWK_TSC_PLL_DIV_114:         *denominator =    57;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_115:         *denominator =    23;  *numerator =   200; break;
        case BLACKHAWK_TSC_PLL_DIV_116:         *denominator =    29;  *numerator =   250; break;
        case BLACKHAWK_TSC_PLL_DIV_118:         *denominator =    59;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_122:         *denominator =    61;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_124:         *denominator =    31;  *numerator =   250; break;
        case BLACKHAWK_TSC_PLL_DIV_125:         *denominator =     1;  *numerator =     8; break;
        case BLACKHAWK_TSC_PLL_DIV_126:         *denominator =    63;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_130:         *denominator =    13;  *numerator =   100; break;
        case BLACKHAWK_TSC_PLL_DIV_135:         *denominator =    27;  *numerator =   200; break;
        case BLACKHAWK_TSC_PLL_DIV_136:         *denominator =    17;  *numerator =   125; break;
        case BLACKHAWK_TSC_PLL_DIV_138:         *denominator =    69;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_142:         *denominator =    71;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_145:         *denominator =    29;  *numerator =   200; break;
        case BLACKHAWK_TSC_PLL_DIV_146:         *denominator =    73;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_150:         *denominator =     3;  *numerator =    20; break;
        case BLACKHAWK_TSC_PLL_DIV_154:         *denominator =    77;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_155:         *denominator =    31;  *numerator =   200; break;
        case BLACKHAWK_TSC_PLL_DIV_156:         *denominator =    39;  *numerator =   250; break;
        case BLACKHAWK_TSC_PLL_DIV_156P566176:  *denominator =  1957;  *numerator = 12500; break;
        case BLACKHAWK_TSC_PLL_DIV_158:         *denominator =    79;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_159P25:      *denominator =     5;  *numerator =    32; break;
        case BLACKHAWK_TSC_PLL_DIV_162:         *denominator =    81;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_164:         *denominator =    41;  *numerator =   250; break;
        case BLACKHAWK_TSC_PLL_DIV_166:         *denominator =    83;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_167:         *denominator =   167;  *numerator =  1000; break;
        case BLACKHAWK_TSC_PLL_DIV_172:         *denominator =    43;  *numerator =   250; break;
        case BLACKHAWK_TSC_PLL_DIV_174:         *denominator =    87;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_176:         *denominator =    22;  *numerator =   125; break;
        case BLACKHAWK_TSC_PLL_DIV_178:         *denominator =    89;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_182:         *denominator =    91;  *numerator =   500; break;
        case BLACKHAWK_TSC_PLL_DIV_176P1381476: *denominator =  8807;  *numerator = 50000; break;
        case BLACKHAWK_TSC_PLL_DIV_178P896:     *denominator = 11181;  *numerator = 62500; break;
        case BLACKHAWK_TSC_PLL_DIV_181P4756672: *denominator =  4537;  *numerator = 25000; break;
        case BLACKHAWK_TSC_PLL_DIV_172P8021978: *denominator =   108;  *numerator =   625; break;
        case BLACKHAWK_TSC_PLL_DIV_154P566176:  *denominator =   483;  *numerator =  3125; break;
#endif
        default:
            EFUN_PRINTF(("ERROR: Unknown divider value:  0x%08X\n", (uint32_t)srds_div));
            *numerator = 0;
            *denominator = 0;
            return (blackhawk_tsc_error(sa__, ERR_CODE_PLL_DIV_INVALID));
    }
    return (ERR_CODE_NONE);
}

/** Get the Refclk frequency in Hz, based on the blackhawk_tsc_pll_div_enum value and VCO frequency. */
static err_code_t _plp_aperta_blackhawk_tsc_get_refclk_from_div_vco(srds_access_t *sa__, uint32_t *refclk_freq_hz, enum blackhawk_tsc_pll_div_enum srds_div, uint32_t vco_freq_khz, enum blackhawk_tsc_pll_option_enum pll_option) {
    uint16_t numerator, denominator;
    EFUN(_plp_aperta_blackhawk_tsc_get_divisor_ratio(sa__, srds_div, &numerator, &denominator));
    *refclk_freq_hz = ((vco_freq_khz+(denominator>>1)) / denominator) * numerator;
    if (pll_option == BLACKHAWK_TSC_PLL_OPTION_REFCLK_DOUBLER_EN) *refclk_freq_hz /= 2;
    if (pll_option == BLACKHAWK_TSC_PLL_OPTION_REFCLK_DIV2_EN)    *refclk_freq_hz *= 2;
    if (pll_option == BLACKHAWK_TSC_PLL_OPTION_REFCLK_DIV4_EN)    *refclk_freq_hz *= 4;
    return (ERR_CODE_NONE);
}

/** Get the VCO frequency in kHz, based on the reference clock frequency and blackhawk_tsc_pll_div_enum value. */
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_vco_from_refclk_div(srds_access_t *sa__, uint32_t refclk_freq_hz, enum blackhawk_tsc_pll_div_enum srds_div, uint32_t *vco_freq_khz, enum blackhawk_tsc_pll_option_enum pll_option) {
    uint16_t numerator, denominator;
    EFUN(_plp_aperta_blackhawk_tsc_get_divisor_ratio(sa__, srds_div, &numerator, &denominator));
    if (pll_option == BLACKHAWK_TSC_PLL_OPTION_REFCLK_DOUBLER_EN) refclk_freq_hz *= 2;
    if (pll_option == BLACKHAWK_TSC_PLL_OPTION_REFCLK_DIV2_EN)    refclk_freq_hz /= 2;
    if (pll_option == BLACKHAWK_TSC_PLL_OPTION_REFCLK_DIV4_EN)    refclk_freq_hz /= 4;
    *vco_freq_khz = ((refclk_freq_hz + (numerator>>1)) / numerator) * denominator;
    return (ERR_CODE_NONE);
}

/* Boundaries for allowed VCO frequency */
#    define SERDES_VCO_FREQ_KHZ_MIN (15000000)
#    define SERDES_VCO_FREQ_KHZ_MAX (30000000)

/* Allowed tolerance in resultant VCO frequency when auto-determining divider value */
#    define SERDES_VCO_FREQ_KHZ_TOLERANCE (2000)

/* The allowed PLL divider values */
static const enum blackhawk_tsc_pll_div_enum _plp_aperta_blackhawk_tsc_div_candidates[] = {
    BLACKHAWK_TSC_PLL_DIV_66,
    BLACKHAWK_TSC_PLL_DIV_67,
    BLACKHAWK_TSC_PLL_DIV_70,
    BLACKHAWK_TSC_PLL_DIV_72,
    BLACKHAWK_TSC_PLL_DIV_76,
    BLACKHAWK_TSC_PLL_DIV_73P6,
    BLACKHAWK_TSC_PLL_DIV_79P2,
    BLACKHAWK_TSC_PLL_DIV_80,
    BLACKHAWK_TSC_PLL_DIV_82P5,
    BLACKHAWK_TSC_PLL_DIV_84,
    BLACKHAWK_TSC_PLL_DIV_85,
    BLACKHAWK_TSC_PLL_DIV_87P5,
    BLACKHAWK_TSC_PLL_DIV_89P6,
    BLACKHAWK_TSC_PLL_DIV_90,
    BLACKHAWK_TSC_PLL_DIV_96,
    BLACKHAWK_TSC_PLL_DIV_100,
    BLACKHAWK_TSC_PLL_DIV_120,
    BLACKHAWK_TSC_PLL_DIV_127P401984,
    BLACKHAWK_TSC_PLL_DIV_128,
    BLACKHAWK_TSC_PLL_DIV_132,
    BLACKHAWK_TSC_PLL_DIV_134,
    BLACKHAWK_TSC_PLL_DIV_140,
    BLACKHAWK_TSC_PLL_DIV_144,
    BLACKHAWK_TSC_PLL_DIV_147P2,
    BLACKHAWK_TSC_PLL_DIV_148,
    BLACKHAWK_TSC_PLL_DIV_152,
    BLACKHAWK_TSC_PLL_DIV_158P4,
    BLACKHAWK_TSC_PLL_DIV_160,
    BLACKHAWK_TSC_PLL_DIV_165,
    BLACKHAWK_TSC_PLL_DIV_168,
    BLACKHAWK_TSC_PLL_DIV_170,
    BLACKHAWK_TSC_PLL_DIV_175,
    BLACKHAWK_TSC_PLL_DIV_178P895958,
    BLACKHAWK_TSC_PLL_DIV_180,
    BLACKHAWK_TSC_PLL_DIV_184,
    BLACKHAWK_TSC_PLL_DIV_192,
    BLACKHAWK_TSC_PLL_DIV_198,
    BLACKHAWK_TSC_PLL_DIV_200,
    BLACKHAWK_TSC_PLL_DIV_224,
    BLACKHAWK_TSC_PLL_DIV_240,
    BLACKHAWK_TSC_PLL_DIV_264,
    BLACKHAWK_TSC_PLL_DIV_280,
    BLACKHAWK_TSC_PLL_DIV_330,
    BLACKHAWK_TSC_PLL_DIV_350
};

static const uint8_t _plp_aperta_blackhawk_tsc_div_candidates_count = sizeof(_plp_aperta_blackhawk_tsc_div_candidates) / sizeof(_plp_aperta_blackhawk_tsc_div_candidates[0]);

static err_code_t _plp_aperta_blackhawk_tsc_check_div(srds_access_t *sa__, enum blackhawk_tsc_pll_div_enum srds_div) {
    uint8_t i, found = 0;
    for (i=0; i<_plp_aperta_blackhawk_tsc_div_candidates_count; i++) {
        found = (uint8_t)(found | (srds_div == _plp_aperta_blackhawk_tsc_div_candidates[i]));
    }
    if (!found) {
        EFUN_PRINTF(("ERROR: Invalid divider value:  0x%08X\n", (uint32_t)srds_div));
        return (blackhawk_tsc_error(sa__, ERR_CODE_PLL_DIV_INVALID));
    }
    return (ERR_CODE_NONE);
}
    
static err_code_t _plp_aperta_blackhawk_tsc_check_vco_freq_khz(srds_access_t *sa__, uint32_t vco_freq_khz) {
    if (vco_freq_khz < SERDES_VCO_FREQ_KHZ_MIN - SERDES_VCO_FREQ_KHZ_TOLERANCE) {
        EFUN_PRINTF(("ERROR: VCO frequency too low:  %d kHz is lower than minimum (%d kHz)\n", vco_freq_khz, SERDES_VCO_FREQ_KHZ_MIN));
        return (blackhawk_tsc_error(sa__, ERR_CODE_VCO_FREQUENCY_INVALID));
    }
    if (vco_freq_khz > SERDES_VCO_FREQ_KHZ_MAX + SERDES_VCO_FREQ_KHZ_TOLERANCE) {
        EFUN_PRINTF(("ERROR: VCO frequency too high:  %d kHz is higher than maximum (%d kHz)\n", vco_freq_khz, SERDES_VCO_FREQ_KHZ_MAX));
        return (blackhawk_tsc_error(sa__, ERR_CODE_VCO_FREQUENCY_INVALID));
    }
    return (ERR_CODE_NONE);
}
    
/** Find the entry out of _plp_aperta_blackhawk_tsc_div_candidates that is closest to matching refclk_freq_hz and vco_freq_khz. */
static err_code_t _plp_aperta_blackhawk_tsc_get_div(srds_access_t *sa__, uint32_t refclk_freq_hz, uint32_t vco_freq_khz, enum blackhawk_tsc_pll_div_enum *srds_div, enum blackhawk_tsc_pll_option_enum pll_option) {
    int32_t vco_freq_khz_min_error = 0x7FFFFFFF;
    uint8_t i;
    for (i=0; i<_plp_aperta_blackhawk_tsc_div_candidates_count; i++) {
        uint32_t actual_vco_freq_khz = 0;
        int32_t  vco_freq_khz_error;
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_vco_from_refclk_div(sa__, refclk_freq_hz, _plp_aperta_blackhawk_tsc_div_candidates[i], &actual_vco_freq_khz, pll_option));
        vco_freq_khz_error = (int32_t)(vco_freq_khz - actual_vco_freq_khz);
        vco_freq_khz_error = SRDS_ABS(vco_freq_khz_error);
        if (vco_freq_khz_min_error > vco_freq_khz_error) {
            vco_freq_khz_min_error = vco_freq_khz_error;
            *srds_div = _plp_aperta_blackhawk_tsc_div_candidates[i];
        }
    }
    if (vco_freq_khz_min_error == (int32_t)0xFFFFFFFF) {
      return (blackhawk_tsc_error(sa__, ERR_CODE_CONFLICTING_PARAMETERS));
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_resolve_pll_parameters(srds_access_t *sa__,
                                                  enum blackhawk_tsc_pll_refclk_enum refclk,
                                                  uint32_t *refclk_freq_hz,
                                                  enum blackhawk_tsc_pll_div_enum *srds_div,
                                                  uint32_t *vco_freq_khz,
                                                  enum blackhawk_tsc_pll_option_enum pll_option) {

    /* Parameter value and consistency checks */
    const uint8_t given_param_count = (uint8_t)(((refclk == BLACKHAWK_TSC_PLL_REFCLK_UNKNOWN) ? 0 : 1)
                                       + ((*srds_div == BLACKHAWK_TSC_PLL_DIV_UNKNOWN) ? 0 : 1)
                                       + ((*vco_freq_khz == 0) ? 0 : 1));
    const uint32_t original_vco_freq_khz = *vco_freq_khz;
    enum blackhawk_tsc_pll_div_enum auto_div = BLACKHAWK_TSC_PLL_DIV_UNKNOWN;
    const char*pll_option_e2s[] = {"no", "refclk_x2", "refclk_div2", "refclk_div4"};
    COMPILER_REFERENCE(pll_option_e2s);

    /* Verify that at least two of the three parameters is given. */
    if (given_param_count < 2) {
        return (blackhawk_tsc_error(sa__, ERR_CODE_INSUFFICIENT_PARAMETERS));
    }

    /* Skip verification if option is selected. Error if all parameters not given. */
    if ((pll_option & BLACKHAWK_TSC_PLL_OPTION_DISABLE_VERIFY) == BLACKHAWK_TSC_PLL_OPTION_DISABLE_VERIFY) {
        if (given_param_count < 3) {
            return (blackhawk_tsc_error(sa__, ERR_CODE_INSUFFICIENT_PARAMETERS));
        } else {
            EFUN(_plp_aperta_blackhawk_tsc_get_refclk_in_hz(sa__, refclk, refclk_freq_hz));
            return(ERR_CODE_NONE);
        }
    }
    pll_option = (enum blackhawk_tsc_pll_option_enum)(pll_option & BLACKHAWK_TSC_PLL_OPTION_REFCLK_MASK);

    /* The refclk value is checked in various functions below. */

    /* Verify that the requested div value is allowed. */
    if (*srds_div != BLACKHAWK_TSC_PLL_DIV_UNKNOWN) {
        EFUN(_plp_aperta_blackhawk_tsc_check_div(sa__, *srds_div));
    }

    /* Verify that the requested VCO frequency is allowed. */
    if (*vco_freq_khz != 0) {
        EFUN(_plp_aperta_blackhawk_tsc_check_vco_freq_khz(sa__, *vco_freq_khz));
    }

    if (refclk == BLACKHAWK_TSC_PLL_REFCLK_UNKNOWN) {
        /* Determine refclk from vco frequency and div */
        EFUN(_plp_aperta_blackhawk_tsc_get_refclk_from_div_vco(sa__, refclk_freq_hz, *srds_div, *vco_freq_khz, pll_option));
    } else {
        EFUN(_plp_aperta_blackhawk_tsc_get_refclk_in_hz(sa__, refclk, refclk_freq_hz));
    }

    if (*vco_freq_khz == 0) {
        /* Determine VCO frequency from refclk and divider */
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_vco_from_refclk_div(sa__, *refclk_freq_hz, *srds_div, vco_freq_khz, pll_option));
    }

    /* Determine divider from vco frequency and refclk.
     * This is done even if the div was provided, because if it is,
     * we still want to check whether the parameters are conflicting.
     */
    EFUN(_plp_aperta_blackhawk_tsc_get_div(sa__, *refclk_freq_hz, *vco_freq_khz, &auto_div, pll_option));
    if (*srds_div == BLACKHAWK_TSC_PLL_DIV_UNKNOWN) {
        /* Use the auto-determined divider value, since the divider was not supplied. */
        *srds_div = auto_div;

        /* Determine resultant VCO frequency from refclk and divider */
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_vco_from_refclk_div(sa__, *refclk_freq_hz, *srds_div, vco_freq_khz, pll_option));
    }

    /* Verify the resultant VCO frequency */
    EFUN(_plp_aperta_blackhawk_tsc_check_vco_freq_khz(sa__, *vco_freq_khz));

    /* Verify that the requested VCO frequency is delivered. */
    if ((original_vco_freq_khz != 0)
        && ((*vco_freq_khz < original_vco_freq_khz - SERDES_VCO_FREQ_KHZ_TOLERANCE)
            || (*vco_freq_khz > original_vco_freq_khz + SERDES_VCO_FREQ_KHZ_TOLERANCE))) {
        EFUN_PRINTF(("ERROR: Could not achieve requested VCO frequency of %d kHz.\n       Refclk is %d Hz, %s option enabled, and auto-determined divider is 0x%08X, yielding a VCO frequency of %d kHz.\n",
                     original_vco_freq_khz, *refclk_freq_hz, pll_option_e2s[pll_option], *srds_div, *vco_freq_khz));
        return (blackhawk_tsc_error(sa__, ERR_CODE_VCO_FREQUENCY_INVALID));
    }
    
    /* Verify the auto-determined divider value. */
    if (auto_div != *srds_div) {
        EFUN_PRINTF(("ERROR: Conflicting PLL parameters:  refclk is %d Hz, %s option enabled, divider is 0x%08X, and VCO frequency is %d kHz.\n       Expected divider is 0x%08X\n",
                     *refclk_freq_hz, pll_option_e2s[pll_option], *srds_div, *vco_freq_khz, auto_div));
        return (blackhawk_tsc_error(sa__, ERR_CODE_CONFLICTING_PARAMETERS));
    }

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_display_pll_to_divider(srds_access_t *sa__, uint32_t srds_div) {
    /* Adjust these to increase or decrease the number of digits to the right
     * of the decimal point.
     */
    const uint8_t decimal_digits = 4;
    const uint32_t ten_to_the_decimal_digits = 10000; /* 10**decimal_digits */

    /* Not a const, because of carry logic below. */
    uint16_t div_integer = SRDS_INTERNAL_GET_PLL_DIV_INTEGER(srds_div);

    if (SRDS_INTERNAL_IS_PLL_DIV_FRACTIONAL(srds_div)) {
        /* fraction_num will have this many bits (ending at bit 0). */
        const uint8_t  fraction_num_width = 28;
        const uint32_t fraction_num =(uint32_t)(SRDS_INTERNAL_GET_PLL_DIV_FRACTION_NUM(srds_div, fraction_num_width));

        /* Identify the number that, when printed with left-padded zeros,
         * becomes the digits to the right of the decimal point.
         *
         * This value can be obtained by dividing fraction_num by:
         *
         *     (2^fraction_num_width)/(10^decimal_digits)
         */
        const uint32_t divisor = (((1U << fraction_num_width) + (ten_to_the_decimal_digits>>1))
                                  / ten_to_the_decimal_digits);
        uint32_t fraction_as_int = (fraction_num + (divisor>>1)) / divisor;

        /* In case the rounding above caused the fractional portion to overflow
         * (e.g. 4.9999999999 becomes 5.0000), implement the carry into the
         * integer portion.
         */
        if (fraction_as_int >= ten_to_the_decimal_digits) {
            fraction_as_int -= ten_to_the_decimal_digits;
            ++div_integer;
        }

        EFUN_PRINTF(("%3d.%0*d", div_integer, decimal_digits, fraction_as_int));
    } else {
        const uint8_t left_spaces = (decimal_digits + 1) >> 1;
        COMPILER_REFERENCE(left_spaces);
        EFUN_PRINTF(("%*s%3d%*s", left_spaces, "", div_integer, decimal_digits - left_spaces + 1, ""));
    }
    return(ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_print_uc_dsc_error(srds_access_t *sa__, enum plp_aperta_srds_pmd_uc_cmd_enum cmd) {
    uint32_t supp_info;

    ESTM(supp_info = (rd_uc_dsc_supp_info()) & 0xFF);
    switch (SUPP_INFO_TO_ERROR_CODE(supp_info)) {
      case UC_CMD_ERROR_INVALID_COMMAND:
        ESTM_PRINTF(("ERROR : UC reported invalid command %d.  (other_info = 0x%X)\n",
                     cmd, SUPP_INFO_TO_OTHER_INFO(supp_info)));
        break;
      case UC_CMD_ERROR_BUSY:
        ESTM_PRINTF(("ERROR : UC reported busy for command %d.  (other_info = 0x%X)\n",
                     cmd, SUPP_INFO_TO_OTHER_INFO(supp_info)));
        break;
      case UC_CMD_ERROR_GET_EYE_SAMPLE_ERROR:
        ESTM_PRINTF(("ERROR : UC reported error in getting eye sample.  (command %d, other_info = 0x%X)\n",
                     cmd, SUPP_INFO_TO_OTHER_INFO(supp_info)));
        break;
      case UC_CMD_ERROR_PRBS_NOT_LOCKED:
        ESTM_PRINTF(("ERROR : UC reported PRBS not locked.  (command %d, other_info = 0x%X)\n",
                     cmd, SUPP_INFO_TO_OTHER_INFO(supp_info)));
        break;
      case UC_CMD_ERROR_COMMAND_IN_PROGRESS:
        ESTM_PRINTF(("ERROR : UC reported command already in progress.  (command %d, other_info = 0x%X)\n",
                     cmd, SUPP_INFO_TO_OTHER_INFO(supp_info)));
        break;
      case UC_CMD_ERROR_INVALID_MODE:
        ESTM_PRINTF(("ERROR : UC reported invalid mode for command %d.  (other_info = 0x%X)\n",
                     cmd, SUPP_INFO_TO_OTHER_INFO(supp_info)));
        break;
      default:
        ESTM_PRINTF(("ERROR : UC reported unknown error 0x%X for command %d.  (other_info = 0x%X)\n",
                     SUPP_INFO_TO_ERROR_CODE(supp_info), cmd, SUPP_INFO_TO_OTHER_INFO(supp_info)));
    }
    /* Cleanup cmd register */
    EFUN(reg_wr_DSC_A_DSC_UC_CTRL(0x80));
    EFUN(wr_uc_dsc_data(0));

    return(ERR_CODE_NONE);
}
 
/******************************************/
/*  Serdes Register field Poll functions  */
/******************************************/

#ifndef CUSTOM_REG_POLLING
/* poll for microcontroller to populate the dsc_data register */
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_poll_diag_done(srds_access_t *sa__, uint16_t *status, uint32_t timeout_ms) {
 uint8_t loop;

 if(!status) {
     return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

 for(loop=0;loop < 100; loop++) {
     ESTM(*status=rdv_usr_diag_status());

     if((*status & 0x8000) > 0) {
         return(ERR_CODE_NONE);
     }
     if(loop>10) {
         EFUN(USR_DELAY_US(10*timeout_ms));
     }
 }
 return(blackhawk_tsc_error(sa__, ERR_CODE_DIAG_TIMEOUT));
}
/** Poll for field "uc_dsc_ready_for_cmd" = 1 [Return Val => Error_code (0 = Polling Pass)] */
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_poll_uc_dsc_ready_for_cmd_equals_1(srds_access_t *sa__, uint32_t timeout_ms, enum plp_aperta_srds_pmd_uc_cmd_enum cmd) {
    /* read quickly for 10 tries */
    uint16_t loop;
    uint16_t reset_state;

    for (loop = 0; loop < 1000; loop++) {
        uint16_t rddata;
        ESTM(rddata = reg_rd_DSC_A_DSC_UC_CTRL());
        if (rddata & 0x0080) {    /* bit 7 is uc_dsc_ready_for_cmd */
            if (rddata & 0x0040) {  /* bit 6 is uc_dsc_error_found   */
                EFUN(plp_aperta_blackhawk_tsc_INTERNAL_print_uc_dsc_error(sa__, cmd));
                return(blackhawk_tsc_error(sa__, ERR_CODE_UC_CMD_RETURN_ERROR));
            }
            return (ERR_CODE_NONE);
        }
        if(loop>10) {
            EFUN(USR_DELAY_US(10*timeout_ms));
        }
    }
    /* Check if ln_s_rstb is asserted by looking at the reset state, the give a warning and display the commands*/
    ESTM(reset_state = rd_lane_dp_reset_state());
    if (reset_state & 0x0007) {
        EFUN_PRINTF(("DSC ready for command is not working; SerDes is probably reset!\n"));
        return (ERR_CODE_NONE);
    }
    {
        EFUN_PRINTF(("%s ERROR : DSC ready for command is not working, applying workaround and getting debug info !\n", API_FUNCTION_NAME));
        /* print the triage info and reset the cmd interface */
        plp_aperta_blackhawk_tsc_INTERNAL_print_triage_info(sa__, ERR_CODE_UC_CMD_POLLING_TIMEOUT, 1, 1, __LINE__);
        /* artifically terminate the command to re-enable the command interface */
        EFUN(wr_uc_dsc_ready_for_cmd(0x1));
    }
    return (ERR_CODE_UC_CMD_POLLING_TIMEOUT);
}

/* Poll for field "dsc_state" = DSC_STATE_UC_TUNE [Return Val => Error_code (0 = Polling Pass)] */
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_poll_dsc_state_equals_uc_tune(srds_access_t *sa__, uint32_t timeout_ms) {
  uint16_t loop;
  uint16_t dsc_state;
  /* poll 10 times to avoid longer delays later */
  for (loop = 0; loop < 100; loop++) {
    ESTM(dsc_state = rd_dsc_state());
    if (dsc_state == DSC_STATE_UC_TUNE) {
      return (ERR_CODE_NONE);
    }
    if(loop>10) {
        EFUN(USR_DELAY_US(10*timeout_ms));
    }
  }
  ESTM_PRINTF(("DSC_STATE = %d\n", rd_dsc_state()));
  return (blackhawk_tsc_error(sa__, ERR_CODE_POLLING_TIMEOUT));          /* Error Code for polling timeout */
}


/* Poll for field "micro_ra_initdone" = 1 [Return Val => Error_code (0 = Polling Pass)] */
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_poll_micro_ra_initdone(srds_access_t *sa__, uint32_t timeout_ms) {
  uint16_t loop;

  uint8_t result;
  for (loop = 0; loop <= 100; loop++) {
    ESTM(result = rdc_micro_ra_initdone());
    if (result) {
      return (ERR_CODE_NONE);
    }
    EFUN(USR_DELAY_US(10*timeout_ms));
  }
  return (blackhawk_tsc_error(sa__, ERR_CODE_POLLING_TIMEOUT));          /* Error Code for polling timeout */
}

#endif /* CUSTOM_REG_POLLING */

int8_t plp_aperta_blackhawk_tsc_INTERNAL_afe_slicer_offset_mapping(int8_t x) {
    if (x>=0)
        return((int8_t)(x^((x&2)>>1)));   /* XOR bit 0 with bit 1 */
    else
        return((int8_t)(-plp_aperta_blackhawk_tsc_INTERNAL_afe_slicer_offset_mapping((int8_t)(-x))));
}

uint8_t plp_aperta_blackhawk_tsc_INTERNAL_is_big_endian(void)
{
    uint32_t one_u32 = 0x01000000;
    char * ptr = (char *)(&one_u32);
    const uint8_t big_endian = (ptr[0] == 1);
    return big_endian;
}


int plp_aperta_blackhawk_tsc_INTERNAL_get_endian_offset(uint8_t *addr) {
    const int endian_const = 1;
    return ((*(char*)&endian_const) == 0) ?
        (
        (((USR_UINTPTR)(addr))%4 == 0) ?  3 :
        (((USR_UINTPTR)(addr))%4 == 1) ?  1 :
        (((USR_UINTPTR)(addr))%4 == 2) ? -1 :
        (((USR_UINTPTR)(addr))%4 == 3) ? -3 :
        0)
        : 0;
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_rdblk_callback(void *arg, uint8_t byte_count, uint16_t data) {
    srds_access_t *sa__;
    blackhawk_tsc_INTERNAL_rdblk_callback_arg_t * const cast_arg = (blackhawk_tsc_INTERNAL_rdblk_callback_arg_t *)arg;
    int endian_offset = 0;
    sa__ = cast_arg->sa;

    ESTM(endian_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_endian_offset(cast_arg->mem_ptr));
    *(cast_arg->mem_ptr + endian_offset) = (uint8_t)(data & 0xFF);
    cast_arg->mem_ptr++;
    if (byte_count == 2) {
        ESTM(endian_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_endian_offset(cast_arg->mem_ptr));
        *(cast_arg->mem_ptr + endian_offset) = (uint8_t)(data >> 8);
        cast_arg->mem_ptr++;
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_rdblk_uc_generic_ram(srds_access_t *sa__,
                                                uint32_t block_addr,
                                                uint16_t block_size,
                                                uint16_t start_offset,
                                                uint16_t cnt,
                                                void *arg,
                                                err_code_t (*callback)(void *, uint8_t, uint16_t)) {
    uint32_t read_val = 0;
    uint8_t defecit = 0;
    uint32_t addr = block_addr + start_offset;
    
    if (cnt == 0) {
        return (ERR_CODE_NONE);
    }

    /* Check for bad start offset and block size. */
    if (start_offset >= block_size) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    while (cnt > 0) {
        /* Determine how many bytes to read before wrapping back to start of block. */
        uint16_t block_cnt = (uint16_t)(SRDS_MIN(cnt, block_addr + block_size - addr));
        cnt = (uint16_t)(cnt - block_cnt);

            /* Set up the word reads. */
            EFUN(wrc_micro_autoinc_rdaddr_en(1));
            EFUN(wrc_micro_ra_rddatasize(0x1));                     /* Select 16bit read datasize */
            EFUN(wrc_micro_ra_rdaddr_msw((uint16_t)(addr >> 16)));              /* Upper 16bits of RAM address to be read */
            EFUN(wrc_micro_ra_rdaddr_lsw(addr & 0xFFFE));           /* Lower 16bits of RAM address to be read */

            /* Read the leading byte, if starting at an odd address. */
            if ((addr & 1) == 1) {
                ESTM(read_val |= (uint32_t)((rdc_micro_ra_rddata_lsw() >> 8) << defecit));
                if (defecit == 8) {
                    EFUN(callback(arg, 2, (uint16_t)read_val));
                    read_val = 0;
                }
                /* We just read a byte.  This toggles the defecit from 0 to 8 or from 8 to 0. */
                defecit ^= 8;
                --block_cnt;
            }

            /* Read the whole words, and call the callback with two bytes at a time. */
            while (block_cnt >= 2) {
                ESTM(read_val |= (uint32_t)(rdc_micro_ra_rddata_lsw() << defecit));
                EFUN(callback(arg, 2, (uint16_t)read_val));
                read_val >>= 16;
                /* We just read two bytes.  This preserves whatever defecit (8 or 0) is there. */
                block_cnt = (uint16_t)(block_cnt - 2);
            }

            /* Read the trailing byte, if leftover after reading whole words. */
            if (block_cnt > 0) {
                ESTM(read_val |= (uint32_t)((rdc_micro_ra_rddata_lsw() & 0xFF) << defecit));
                if (defecit == 8) {
                    EFUN(callback(arg, 2, (uint16_t)read_val));
                    read_val = 0;
                }
                /* We just read a byte.  This toggles the defecit from 0 to 8 or from 8 to 0. */
                defecit ^= 8;
            }
        addr = block_addr;
    }

    /* If a final byte is left behind, then call the callback with it. */
    if (defecit > 0) {
        EFUN(callback(arg, 1, (uint16_t)read_val));
    }

    return(ERR_CODE_NONE);
}
err_code_t plp_aperta_blackhawk_tsc_INTERNAL_rdblk_uc_generic_ram_descending(srds_access_t *sa__,
                                                           uint32_t block_addr,
                                                           uint16_t block_size,
                                                           uint16_t start_offset,
                                                           uint16_t cnt,
                                                           void *arg,
                                                           err_code_t (*callback)(void *, uint8_t, uint16_t)) {
    uint32_t read_val = 0;
    uint8_t defecit = 0;
    uint32_t addr = block_addr + start_offset;
    uint16_t configured_addr_msw = (uint16_t)((addr >> 16) + 1);

    if (cnt == 0) {
        return (ERR_CODE_NONE);
    }

    /* Check for bad start offset and block size. */
    if (start_offset >= block_size) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    EFUN(wrc_micro_autoinc_rdaddr_en(0));
    EFUN(wrc_micro_ra_rddatasize(0x1));                         /* Select 16bit read datasize */
    
    while (cnt > 0) {
        /* Determine how many bytes to read before wrapping back to end of block. */
        uint16_t block_cnt = (uint16_t)(SRDS_MIN(cnt, start_offset+1));
        cnt = (uint16_t)(cnt - block_cnt);

        while (block_cnt > 0) {
            const uint16_t addr_msw = (uint16_t)(addr >> 16);
            uint16_t read_val2;
            if (addr_msw != configured_addr_msw) {
                EFUN(wrc_micro_ra_rdaddr_msw(addr_msw));        /* Upper 16bits of RAM address to be read */
                configured_addr_msw = addr_msw;
            }
            
            EFUN(wrc_micro_ra_rdaddr_lsw(addr & 0xFFFE));       /* Lower 16bits of RAM address to be read */
            ESTM(read_val2 = rdc_micro_ra_rddata_lsw());

            if (((addr & 1) == 1) && (block_cnt >= 2)) {
                /* Reading two bytes.  Since we're reading in descending address order, they
                 * will be reversed before they are sent out. */
                read_val = read_val | (uint32_t)((((read_val2 & 0xFF) << 8) | (read_val2 >> 8)) << defecit);
                EFUN(callback(arg, 2, (uint16_t)read_val));
                read_val >>= 16;
                /* We just read two bytes.  This preserves whatever defecit (8 or 0) is there. */
                block_cnt = (uint16_t)(block_cnt - 2);
                addr -= 2;
            } else {
                if ((addr & 1) == 1) {
                    /* Reading upper byte of word. */
                    read_val = read_val | (uint32_t)((read_val2 >> 8) << defecit);
                } else {
                    /* Reading lower byte of word. */
                    read_val = read_val | (uint32_t)((read_val2 & 0xFF) << defecit);
                }
                if (defecit == 8) {
                    EFUN(callback(arg, 2, (uint16_t)read_val));
                    read_val = 0;
                }
                /* We just read a byte.  This toggles the defecit from 0 to 8 or from 8 to 0. */
                defecit ^= 8;
                --block_cnt;
                --addr;
            }
        }
        
        addr = block_addr + block_size - 1;
        start_offset = (uint16_t)(block_size - 1);
    }

    /* If a final byte is left behind, then call the callback with it. */
    if (defecit > 0) {
        EFUN(callback(arg, 1, (uint16_t)read_val));
    }

    return(ERR_CODE_NONE);
}

uint8_t plp_aperta_blackhawk_tsc_INTERNAL_grp_idx_from_lane(uint8_t lane) {
    return(lane>>1);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_get_lms_thresh_bin(srds_access_t *sa__, int8_t *lms_thresh_bin) {
    ESTM(*lms_thresh_bin = (int8_t)rd_afe_lms_thresh_bin());
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_poll_dbgfb_idx_lock(srds_access_t *sa__, uint8_t timeout_ms) {
  uint32_t loop, max_loop = 10;
  uint8_t idx_lock = 0;

  /* poll for first 10 times and then wait for timeout_ms before polling */
  for (loop = 0; loop < max_loop; loop++) {
      ESTM(idx_lock = rd_dbg_fdbk_idx_lock());
      if (idx_lock) {
          return (ERR_CODE_NONE);
      }
  }
  /* polled for 10 times, wait for timeout_ms right here before polling the register */
  EFUN(plp_aperta_blackhawk_tsc_delay_us((uint32_t)(timeout_ms * 1000)));
  ESTM(idx_lock = rd_dbg_fdbk_idx_lock());
  if (idx_lock) {
      return (ERR_CODE_NONE);
  } else {
      ESTM_PRINTF(("!!!!ERROR!!!! polling for dbgfb idx lock timedout after %d ms \n", timeout_ms));
      /* Error Code for polling timeout */
      return (blackhawk_tsc_error(sa__, ERR_CODE_POLLING_TIMEOUT));
  }
}

err_code_t plp_aperta_blackhawk_tsc_get_dbgfb_summary(srds_access_t *sa__, blackhawk_tsc_dbgfb_cfg_st *dbgfb_cfg) {
    uint32_t d_0_n1, d_0_0, d_0_p1, d_1_n1, d_1_0, d_1_p1;
    uint32_t pdet, ndet;
    int32_t tdet;
    enum blackhawk_tsc_rx_pam_mode_enum pam_mode = NRZ;

    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
    if (pam_mode == NRZ) {
        EFUN_PRINTF(("!!!!ERROR!!!! rx_pam_mode is set to NRZ , plp_aperta_blackhawk_tsc_get_dbgfb_summary cannot continue\n"));
        return(ERR_CODE_INVALID_RX_PAM_MODE);
    }

    EFUN(wr_dbg_fdbk_en(1));
    EFUN(wr_fdbk_counting_mode(0));
    EFUN(wr_dbg_fdbk_data_threshold(dbgfb_cfg->y));
    EFUN(wr_dbg_fdbk_data_sel(1));
    if (dbgfb_cfg->x) {
        EFUN(wr_inlv_idx(78));
        EFUN(wr_dbg_fdbk_idx((uint8_t)(dbgfb_cfg->x -1)));
        EFUN(wr_frc_index_diffprec(1));
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_poll_dbgfb_idx_lock(sa__, 100));
        EFUN(wr_frc_index_diffprec(0));
        EFUN(wr_dbg_fdbk_idx_monitor_en(0));
        EFUN(wr_dbg_fdbk_idx_monitor_en(1));
    } else {
        EFUN(wr_dbg_fdbk_idx(1));
        EFUN(wr_dbg_fdbk_idx(0));
        EFUN(wr_dbg_fdbk_idx_monitor_en(0));
    }
    /*TODO User HW timer for better precision */
    EFUN(USR_DELAY_US(dbgfb_cfg->time_in_us));

    EFUN(wr_dbg_fdbk_en(0));
    ESTM(d_1_n1 = rd_dbg_fdbk_counter_0());

    ESTM(d_1_0  = rd_dbg_fdbk_counter_1());
    ESTM(d_1_p1 = rd_dbg_fdbk_counter_2());
    ESTM(d_0_n1 = rd_dbg_fdbk_counter_3());
    ESTM(d_0_0  = rd_dbg_fdbk_counter_4());
    ESTM(d_0_p1 = rd_dbg_fdbk_counter_5());

    tdet  = (int32_t)(d_1_n1 + d_0_p1-d_0_n1 - d_1_p1);
    pdet  = d_1_n1 + d_1_0 + d_1_p1;
    ndet  = d_0_n1 + d_0_0 + d_0_p1; 
    dbgfb_cfg->data2 = pdet;
    dbgfb_cfg->data3 = ndet;
    dbgfb_cfg->data1 = tdet;
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_collect_dbgfb_stats(srds_access_t *sa__, uint32_t time_in_us, blackhawk_tsc_dbgfb_stats_st *dbgfb_stats) {
    uint8_t y, x;
    uint8_t max_y;
    enum blackhawk_tsc_rx_pam_mode_enum pam_mode = NRZ;
    uint8_t pam4es_en = 0;
    blackhawk_tsc_dbgfb_cfg_st dbgfb_cfg = {0};

    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
    if (pam_mode == NRZ) {
        EFUN_PRINTF(("!!!!ERROR!!!! rx_pam_mode is set to NRZ , plp_aperta_blackhawk_tsc_collect_dbgfb_stats cannot continue\n"));
        return(ERR_CODE_INVALID_RX_PAM_MODE);
    }
    pam4es_en = (pam_mode==PAM4_ES);
    max_y = pam4es_en ? 6 : 3;
    /* backup the measure time and pam4es_en info */
    dbgfb_stats->time_in_us = time_in_us;
    dbgfb_stats->pam4es_en  = pam4es_en; 


    for (y=0; y < max_y; y++){
        for (x = 0; x < _X_; x++){
            dbgfb_cfg.y = y;
            dbgfb_cfg.x = x;
            dbgfb_cfg.time_in_us = time_in_us;
            dbgfb_cfg.data1 = 0;
            dbgfb_cfg.data2 = 0;
            dbgfb_cfg.data3 = 0;
            EFUN(plp_aperta_blackhawk_tsc_get_dbgfb_summary(sa__, &dbgfb_cfg));
            dbgfb_stats->time_in_us = time_in_us;
            dbgfb_stats->data.data1[x][y] = dbgfb_cfg.data1;
            dbgfb_stats->data.data2[x][y] = dbgfb_cfg.data2;
            dbgfb_stats->data.data3[x][y] = dbgfb_cfg.data3;
            dbgfb_stats->data.data4[x][y] = (int32_t)(dbgfb_stats->data.data2[x][y] - dbgfb_stats->data.data3[x][y]);

        }
    }
    return ERR_CODE_NONE;
}


err_code_t plp_aperta_blackhawk_tsc_INTERNAL_display_info_table (srds_access_t *sa__) {
    srds_info_t * const blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__);
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_verify_blackhawk_tsc_info(blackhawk_tsc_info_ptr, sa__));

    EFUN_PRINTF(("\n********** SERDES INFO TABLE DUMP **********\n"));
    EFUN_PRINTF(("signature                 = 0x%X\n", blackhawk_tsc_info_ptr->signature));
    EFUN_PRINTF(("diag_mem_ram_base         = 0x%X\n", blackhawk_tsc_info_ptr->diag_mem_ram_base));
    EFUN_PRINTF(("diag_mem_ram_size         = 0x%X\n", blackhawk_tsc_info_ptr->diag_mem_ram_size));
    EFUN_PRINTF(("core_var_ram_base         = 0x%X\n", blackhawk_tsc_info_ptr->core_var_ram_base));
    EFUN_PRINTF(("core_var_ram_size         = 0x%X\n", blackhawk_tsc_info_ptr->core_var_ram_size));
    EFUN_PRINTF(("lane_var_ram_base         = 0x%X\n", blackhawk_tsc_info_ptr->lane_var_ram_base));
    EFUN_PRINTF(("lane_var_ram_size         = 0x%X\n", blackhawk_tsc_info_ptr->lane_var_ram_size));
    EFUN_PRINTF(("trace_mem_ram_base        = 0x%X\n", blackhawk_tsc_info_ptr->trace_mem_ram_base));
    EFUN_PRINTF(("trace_mem_ram_size        = 0x%X\n", blackhawk_tsc_info_ptr->trace_mem_ram_size));
    EFUN_PRINTF(("micro_var_ram_base        = 0x%X\n", blackhawk_tsc_info_ptr->micro_var_ram_base));
    EFUN_PRINTF(("lane_count                = 0x%X\n", blackhawk_tsc_info_ptr->lane_count));
    EFUN_PRINTF(("trace_memory_descending_writes = 0x%X\n", blackhawk_tsc_info_ptr->trace_memory_descending_writes));
    EFUN_PRINTF(("micro_count               = 0x%X\n", blackhawk_tsc_info_ptr->micro_count));
    EFUN_PRINTF(("micro_var_ram_size        = 0x%X\n", blackhawk_tsc_info_ptr->micro_var_ram_size));
    EFUN_PRINTF(("grp_ram_size              = 0x%X\n", blackhawk_tsc_info_ptr->grp_ram_size));
    EFUN_PRINTF(("ucode_version             = 0x%X\n", blackhawk_tsc_info_ptr->ucode_version));

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_INTERNAL_is_flr_supported(srds_access_t *sa__, uint8_t *flr_support) {
    srds_info_t * const blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__);
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_verify_blackhawk_tsc_info(blackhawk_tsc_info_ptr, sa__));

    *flr_support = (blackhawk_tsc_info_ptr->lane_var_ram_size >= 0x900);
    return ERR_CODE_NONE;
}
