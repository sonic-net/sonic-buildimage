/***********************************************************************************
 ***********************************************************************************
 *  File Name     :  blackhawk_tsc_debug_functions.c                               *
 *  Created On    :  03 Nov 2015                                                   *
 *  Created By    :  Brent Roberts                                                 *
 *  Description   :  APIs for Serdes IPs                                           *
 *  Revision      :                                                                *
 *                                                                                 *
 * $Copyright: (c) 2020 Broadcom.                                                  *
 * Broadcom Proprietary and Confidential. All rights reserved.$                    *
 *                                                                                 *
 ***********************************************************************************
 ***********************************************************************************/

/** @file blackhawk_tsc_debug_functions.c
 * Implementation of API debug functions
 */
#include <phymod/phymod_system.h>
#include "blackhawk_tsc_debug_functions.h"
#include "blackhawk_tsc_access.h"
#include "blackhawk_tsc_common.h"
#include "blackhawk_tsc_config.h"
#include "blackhawk_tsc_functions.h"
#include "blackhawk_tsc_internal.h"
#include "blackhawk_tsc_internal_error.h"
#include "blackhawk_tsc_prbs.h"
#include "blackhawk_tsc_select_defns.h"
#include "blackhawk_tsc_reg_dump.h"


static err_code_t _plp_aperta_blackhawk_tsc_reg_print_no_buf(srds_access_t *sa__, uint8_t reg_section);
static err_code_t _plp_aperta_blackhawk_tsc_reg_print_with_buf(srds_access_t *sa__, uint8_t reg_section, char reg_buffer[][250], uint8_t *buf_index);
static err_code_t _plp_aperta_blackhawk_tsc_reg_print(srds_access_t *sa__, uint8_t reg_section, char reg_buffer[][250], uint8_t *buf_index);


/*************************/
/*  Stop/Resume uC Lane  */
/*************************/

err_code_t plp_aperta_blackhawk_tsc_stop_uc_lane(srds_access_t *sa__, uint8_t enable) {

    return(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__,enable));
}


err_code_t plp_aperta_blackhawk_tsc_stop_uc_lane_status(srds_access_t *sa__, uint8_t *uc_lane_stopped) {

  if(!uc_lane_stopped) {
      return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*uc_lane_stopped = rdv_usr_sts_micro_stopped());

  return (ERR_CODE_NONE);
}

/*******************************************************************/
/*  APIs to Write Core/Lane Config and User variables into uC RAM  */
/*******************************************************************/

err_code_t plp_aperta_blackhawk_tsc_set_usr_ctrl_core_event_log_level(srds_access_t *sa__, uint8_t core_event_log_level) {
  return(wrcv_usr_ctrl_core_event_log_level(core_event_log_level));
}

err_code_t plp_aperta_blackhawk_tsc_set_usr_ctrl_lane_event_log_level(srds_access_t *sa__, uint8_t lane_event_log_level) {
  return(wrv_usr_ctrl_lane_event_log_level(lane_event_log_level));
}

err_code_t plp_aperta_blackhawk_tsc_set_usr_ctrl_disable_startup(srds_access_t *sa__, struct blackhawk_tsc_usr_ctrl_disable_functions_st set_val) {
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_functions_byte(&set_val));
  /* BLACKHAWK TODO:  This is copied from Falcon16. */
  return(wrv_usr_ctrl_disable_startup_functions_word(set_val.word));
}

err_code_t plp_aperta_blackhawk_tsc_set_usr_ctrl_disable_startup_dfe(srds_access_t *sa__, struct blackhawk_tsc_usr_ctrl_disable_dfe_functions_st set_val) {
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_dfe_functions_byte(&set_val));
  return(wrv_usr_ctrl_disable_startup_dfe_functions_byte(set_val.byte));
}

err_code_t plp_aperta_blackhawk_tsc_set_usr_ctrl_disable_steady_state(srds_access_t *sa__, struct blackhawk_tsc_usr_ctrl_disable_functions_st set_val) {
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_check_uc_lane_stopped(sa__));  /* make sure uC is stopped to avoid race conditions */
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_functions_byte(&set_val));
  /* BLACKHAWK TODO:  This is copied from Falcon16. */
  return(wrv_usr_ctrl_disable_steady_state_functions_word(set_val.word));
}

err_code_t plp_aperta_blackhawk_tsc_set_usr_ctrl_disable_steady_state_dfe(srds_access_t *sa__, struct blackhawk_tsc_usr_ctrl_disable_dfe_functions_st set_val) {
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_dfe_functions_byte(&set_val));
  return(wrv_usr_ctrl_disable_steady_state_dfe_functions_byte(set_val.byte));
}

/******************************************************************/
/*  APIs to Read Core/Lane Config and User variables from uC RAM  */
/******************************************************************/

err_code_t plp_aperta_blackhawk_tsc_get_usr_ctrl_core_event_log_level(srds_access_t *sa__, uint8_t *core_event_log_level) {

  if(!core_event_log_level) {
     return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*core_event_log_level = rdcv_usr_ctrl_core_event_log_level());

  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_get_usr_ctrl_lane_event_log_level(srds_access_t *sa__, uint8_t *lane_event_log_level) {

  if(!lane_event_log_level) {
     return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*lane_event_log_level = rdv_usr_ctrl_lane_event_log_level());
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_get_usr_ctrl_disable_startup(srds_access_t *sa__, struct blackhawk_tsc_usr_ctrl_disable_functions_st *get_val) {

  if(!get_val) {
     return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  /* BLACKHAWK TODO:  This is copied from Falcon16 */
  ESTM(get_val->word = rdv_usr_ctrl_disable_startup_functions_word());
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_functions_st(get_val));
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_get_usr_ctrl_disable_startup_dfe(srds_access_t *sa__, struct blackhawk_tsc_usr_ctrl_disable_dfe_functions_st *get_val) {

  if(!get_val) {
     return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->byte = rdv_usr_ctrl_disable_startup_dfe_functions_byte());
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_dfe_functions_st(get_val));
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_get_usr_ctrl_disable_steady_state(srds_access_t *sa__, struct blackhawk_tsc_usr_ctrl_disable_functions_st *get_val) {

  if(!get_val) {
     return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  /* BLACKHAWK TODO:  This is copied from Falcon16. */
  ESTM(get_val->word = rdv_usr_ctrl_disable_steady_state_functions_word());
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_functions_st(get_val));
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_get_usr_ctrl_disable_steady_state_dfe(srds_access_t *sa__, struct blackhawk_tsc_usr_ctrl_disable_dfe_functions_st *get_val) {

  if(!get_val) {
     return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->byte = rdv_usr_ctrl_disable_steady_state_dfe_functions_byte());
  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_update_usr_ctrl_disable_dfe_functions_st(get_val));
  return (ERR_CODE_NONE);
}

/********************************************************************************/
/* Helper print function for Serdes Register/Variable Dump using no buffer      */
/********************************************************************************/
static err_code_t _plp_aperta_blackhawk_tsc_reg_print_no_buf(srds_access_t *sa__, uint8_t reg_section)
{
    uint16_t addr, rddata = 0;

    for (addr = plp_aperta_blackhawk_tsc_reg_dump_arr[reg_section][0]; addr < plp_aperta_blackhawk_tsc_reg_dump_arr[reg_section][1]; addr++) 
    {
        if (!(addr & 0x000F))  
        {
            EFUN_PRINTF(("\n%04x ",addr));
        }
        EFUN(plp_aperta_blackhawk_tsc_acc_rdt_reg(sa__, addr,&rddata));
        EFUN_PRINTF(("%04x ",rddata));
    }
    return (ERR_CODE_NONE);
}
/********************************************************************************/
/* Helper print function for Serdes Register/Variable Dump using a buffer       */
/********************************************************************************/
static err_code_t _plp_aperta_blackhawk_tsc_reg_print_with_buf(srds_access_t *sa__, uint8_t reg_section, char reg_buffer[][250], uint8_t *buf_index)
{
    uint16_t addr, rddata = 0;
	uint8_t first = 0;
    char temp_buf[6];
	
	if(reg_section == 0) first = 1;

    for (addr = plp_aperta_blackhawk_tsc_reg_dump_arr[reg_section][0]; addr < plp_aperta_blackhawk_tsc_reg_dump_arr[reg_section][1]; addr++) 
    {
        if (!(addr & 0x000F))  
        {
            if(first) first = 0;
            else (*buf_index)++; /* don't increment buf_index for first section */
            USR_SPRINTF(temp_buf,"%04x ", addr);
            USR_STRCPY(reg_buffer[*buf_index], temp_buf);
        }
        EFUN(plp_aperta_blackhawk_tsc_acc_rdt_reg(sa__, addr,&rddata));
        USR_SPRINTF(temp_buf,"%04x ", rddata);
        USR_STRNCAT(reg_buffer[*buf_index], temp_buf, 6);
    }
    return (ERR_CODE_NONE);
}
/********************************************************************************/
/* Helper print function for Serdes Register/Variable Dump                      */
/********************************************************************************/
static err_code_t _plp_aperta_blackhawk_tsc_reg_print(srds_access_t *sa__, uint8_t reg_section, char reg_buffer[][250], uint8_t *buf_index)
{
    if(reg_buffer == NULL)
    {
         EFUN(_plp_aperta_blackhawk_tsc_reg_print_no_buf(sa__, reg_section));
    }
    else
    {
        EFUN(_plp_aperta_blackhawk_tsc_reg_print_with_buf(sa__, reg_section, reg_buffer, buf_index));
    }
    return (ERR_CODE_NONE);
}
err_code_t plp_aperta_blackhawk_tsc_reg_dump(srds_access_t *sa__, uint8_t core, uint8_t lane, char reg_buffer[][250])
{    
    uint8_t reg_section = 0;
    uint8_t buf_index = 0;

    if (reg_buffer == NULL) 
    {
        EFUN_PRINTF(("\n****  SERDES REGISTER CORE %d LANE %d DUMP    ****", core, lane));
    }
    else
    {
        EFUN_PRINTF(("\n****  SERDES REGISTER CORE %d LANE %d DECODED    ****\n", core, lane));
    }
    
    while(reg_section < BLACKHAWK_TSC_REG_DUMP_SECTIONS) 
    {
        if(plp_aperta_blackhawk_tsc_reg_dump_arr[reg_section][0] == BLACKHAWK_TSC_REG_PLL_START)
        {
            uint8_t pll_orig, pll_idx;
            uint8_t pll_start_section = reg_section;

            ESTM(pll_orig = plp_aperta_blackhawk_tsc_get_pll_idx(sa__));
            for (pll_idx = 0; pll_idx < DUAL_PLL_NUM_PLLS; pll_idx++)
            {
                reg_section = pll_start_section;
                EFUN(plp_aperta_blackhawk_tsc_set_pll_idx(sa__,pll_idx));
                while( reg_section < BLACKHAWK_TSC_REG_DUMP_SECTIONS && plp_aperta_blackhawk_tsc_reg_dump_arr[reg_section][0] != BLACKHAWK_TSC_REG_UC_START)
                {
                    EFUN(_plp_aperta_blackhawk_tsc_reg_print(sa__, reg_section, reg_buffer, &buf_index));
                    reg_section++;
                }
            }
            EFUN(plp_aperta_blackhawk_tsc_set_pll_idx(sa__,pll_orig));
        }
        else if (plp_aperta_blackhawk_tsc_reg_dump_arr[reg_section][0] == BLACKHAWK_TSC_REG_UC_START)
        {
#ifdef SERDES_MULTI_MICROS
            uint8_t micro_orig, micro_idx, num_micros;
            uint8_t uc_start_section = reg_section;

            ESTM(num_micros = rdc_micro_num_uc_cores());
            ESTM(micro_orig = plp_aperta_blackhawk_tsc_get_micro_idx(sa__));
            for(micro_idx = 0; micro_idx < num_micros; micro_idx++)
            {
                reg_section = uc_start_section;
                EFUN(plp_aperta_blackhawk_tsc_set_micro_idx(sa__,micro_idx));
#endif
                while(reg_section < BLACKHAWK_TSC_REG_DUMP_SECTIONS)
                {
                    EFUN(_plp_aperta_blackhawk_tsc_reg_print(sa__, reg_section, reg_buffer, &buf_index));
                    reg_section++;
                }
#ifdef SERDES_MULTI_MICROS
            }
            EFUN(plp_aperta_blackhawk_tsc_set_micro_idx(sa__,micro_orig));
#endif
        }
        else
        {
            EFUN(_plp_aperta_blackhawk_tsc_reg_print(sa__, reg_section, reg_buffer, &buf_index));
            reg_section++;
        }
    } 

    if (reg_buffer != NULL)
    {
        reg_buffer[++buf_index][0] = 0xA;
        reg_buffer[++buf_index][0] = 0;
    }

    return (ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_uc_core_var_dump(srds_access_t *sa__, uint8_t core, char ram_buffer[][250]) {
  uint8_t  addr, rddata, core_var_ram_size;

    srds_info_t *blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__);
    CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, 0);

#ifdef PHYMOD_SUPPORT
    if ( blackhawk_tsc_info_ptr == NULL ) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    } else if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    }
#endif /* PHYMOD_SUPPORT */
    core_var_ram_size = (uint8_t)blackhawk_tsc_info_ptr->core_var_ram_size;
    
  if (ram_buffer == NULL) {
      EFUN_PRINTF(("\n**** SERDES UC CORE %d RAM VARIABLE DUMP ****", core));
      for (addr = 0x0; addr < core_var_ram_size; addr++) {
          if (!(addr % 26))  {
              EFUN_PRINTF(("\n%04x ",addr));
          }
          ESTM_PRINTF(("%02x ", plp_aperta_blackhawk_tsc_rdbc_uc_var(sa__, __ERR, addr)));
      }
  }
  else {
      uint8_t index=0;
      char tempBuf[6];

      EFUN_PRINTF(("\n**** SERDES UC CORE %d RAM VARIABLE DECODED ****\n", core));
      for (addr = 0x0; addr < core_var_ram_size; addr++) {
          if (!(addr % 26))  {
             if (addr != 0) {
                index++;
             }
             USR_SPRINTF(tempBuf,"%04x ", addr);
             USR_STRCPY(ram_buffer[index], tempBuf);
          }
          ESTM(rddata = plp_aperta_blackhawk_tsc_rdbc_uc_var(sa__, __ERR, addr));
          USR_SPRINTF(tempBuf,"%02x ", rddata);
          USR_STRNCAT(ram_buffer[index], tempBuf, 6);
      }
      ram_buffer[index+1][0] = 0xA;
      ram_buffer[index+2][0] = 0;
  }
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_uc_micro_var_dump(srds_access_t *sa__, uint8_t core, uint8_t lane, char ram_buffer[][250]) {
  uint8_t addr;

  if (ram_buffer == NULL) {
      EFUN_PRINTF(("\n**** SERDES UC CORE %d MICRO %d RAM VARIABLE DUMP ****", core,lane/2));

      for (addr = 0x0; addr < 0x10; addr++) {
         if (!(addr % 26))  {
            EFUN_PRINTF(("\n%04x ",addr));
         }
         ESTM_PRINTF(("%02x ", plp_aperta_blackhawk_tsc_rdbuc_uc_var(sa__, __ERR, addr)));
      }
  }
  else
  {
      uint8_t rddata, index=0;
      char tempBuf[6];

      EFUN_PRINTF(("\n**** SERDES UC CORE %d MICRO %d RAM VARIABLE DECODED ****\n", core,(blackhawk_tsc_get_physical_lane(sa__)/2)));

      for (addr = 0x0; addr < 0x10; addr++) {
         if (!(addr % 26))  {
            /* if (addr != 0) { index++; } */
            USR_SPRINTF(tempBuf,"%04x ", addr);
            USR_STRCPY(ram_buffer[index], tempBuf);
         }
         ESTM(rddata = plp_aperta_blackhawk_tsc_rdbuc_uc_var(sa__, __ERR, addr));
         USR_SPRINTF(tempBuf,"%02x ", rddata);
         USR_STRNCAT(ram_buffer[index], tempBuf, 6);
     }
     ram_buffer[index+1][0] = 0xA;
     ram_buffer[index+2][0] = 0;
  }
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_uc_lane_var_dump(srds_access_t *sa__, uint8_t core, uint8_t lane, char ram_buffer[][250]) {
  uint8_t     rx_lock, uc_stopped = 0;
  uint16_t    addr, lane_var_ram_size;

  srds_info_t *blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__);
  CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, 0);
  lane_var_ram_size = (uint16_t)blackhawk_tsc_info_ptr->lane_var_ram_size;

#ifdef PHYMOD_SUPPORT
    if ( blackhawk_tsc_info_ptr == NULL ) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    } else if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    }
#endif /* PHYMOD_SUPPORT */

  ESTM(rx_lock = rd_pmd_rx_lock());

  {
      err_code_t err_code=ERR_CODE_NONE;
      uc_stopped = plp_aperta_blackhawk_tsc_INTERNAL_stop_micro(sa__,rx_lock,&err_code);
      if(err_code) USR_PRINTF(("Unable to stop microcontroller,  following data is suspect\n"));
  }

  if (ram_buffer == NULL) {
      EFUN_PRINTF(("\n**** SERDES UC CORE %d LANE %d RAM VARIABLE DUMP ****", core,lane));
      for (addr = 0x0; addr < lane_var_ram_size; addr++) {
         if (!(addr % 26))  {
            EFUN_PRINTF(("\n%04x ",addr));
         }
         ESTM_PRINTF(("%02x ", plp_aperta_blackhawk_tsc_rdbl_uc_var(sa__, __ERR, addr)));
      }
  }
  else {
      uint8_t rddata, index = 0;
      char tempBuf[6];
      EFUN_PRINTF(("\n**** SERDES UC CORE %d LANE %d RAM VARIABLE DECODED ****\n", core,lane));

      for (addr = 0x0; addr < lane_var_ram_size; addr++) {
         if (!(addr % 26))  {
            if (addr != 0) {
               index++;
            }
            USR_SPRINTF(tempBuf,"%04x ", addr);
            USR_STRCPY(ram_buffer[index], tempBuf);
         }
         ESTM(rddata = plp_aperta_blackhawk_tsc_rdbl_uc_var(sa__, __ERR, addr));
         USR_SPRINTF(tempBuf,"%02x ", rddata);
         USR_STRNCAT(ram_buffer[index], tempBuf, 6);
      }
      ram_buffer[index+1][0] = 0xA;
      ram_buffer[index+2][0] = 0;
  }

  if (rx_lock == 1) {
      if (!uc_stopped) {
          EFUN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__, 0));
      }
  } else {
      EFUN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__, 0));
  }

  return (ERR_CODE_NONE);
}

/***************************************/
/*  API Function to Read Event Logger  */
/***************************************/

err_code_t plp_aperta_blackhawk_tsc_read_event_log(srds_access_t *sa__) {
    blackhawk_tsc_INTERNAL_event_log_dump_state_t state;
    uint8_t micro_num = 0;
    srds_info_t *blackhawk_tsc_info_ptr;
    ESTM(blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__));
    CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, ERR_CODE_INVALID_INFO_TABLE_ADDR);
#ifdef PHYMOD_SUPPORT
    if ( blackhawk_tsc_info_ptr == NULL ) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    } else if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    }
#endif /* PHYMOD_SUPPORT */
    for (; micro_num<blackhawk_tsc_info_ptr->micro_count; ++micro_num)
    {
        state.index = 0;
        state.line_start_index = 0;
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_read_event_log_with_callback(sa__, micro_num, 0, &state, plp_aperta_blackhawk_tsc_INTERNAL_event_log_dump_callback));
        EFUN(plp_aperta_blackhawk_tsc_INTERNAL_event_log_dump_callback(&state, 0, 0));
    }
    return(ERR_CODE_NONE);
}

/**********************************************/
/*  Loopback and Ultra-Low Latency Functions  */
/**********************************************/

/* Enable/Disable Digital Loopback */
err_code_t plp_aperta_blackhawk_tsc_dig_lpbk(srds_access_t *sa__, uint8_t enable) {
    /* setting/clearing prbs_chk_en_auto_mode while coming out of/going in to dig lpbk */
    EFUN(wr_prbs_chk_en_auto_mode(!enable));
    EFUN(wr_dig_lpbk_en(enable));                         /* 0 = disabled, 1 = enabled */

    /* Disabling Link Training as it's an invalid mode in digital loopback */
    if(enable) {
        uint8_t link_training_enable = 0;
        ESTM(link_training_enable = rd_linktrn_ieee_training_enable());
        if(link_training_enable) {
            EFUN_PRINTF(("Warning: Core: %d, Lane %d: Link Training mode is on in digital loopback.\n", plp_aperta_blackhawk_tsc_get_core(sa__), plp_aperta_blackhawk_tsc_get_lane(sa__)));
        }
    }
  return (ERR_CODE_NONE);
}


/**********************************/
/*  TX_PI Jitter Generation APIs  */
/**********************************/

/* TX_PI Sinusoidal or Spread-Spectrum (SSC) Jitter Generation  */
err_code_t plp_aperta_blackhawk_tsc_tx_pi_jitt_gen(srds_access_t *sa__, uint8_t enable, int16_t freq_override_val, enum plp_aperta_srds_tx_pi_freq_jit_gen_enum jit_type, uint8_t tx_pi_jit_freq_idx, uint8_t tx_pi_jit_amp) {
    /* Added a limiting for the jitter amplitude index, per freq_idx */
    uint8_t max_amp_idx_r20_os1[] = {37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 48, 33, 47, 37, 33, 37, 37};

    /* Irrespective of the osr_mode, txpi runs @ os1. Thus the max amp idx values remain the same. */
    if (jit_type == TX_PI_SJ) {
        if (tx_pi_jit_amp > max_amp_idx_r20_os1[tx_pi_jit_freq_idx]) {
            tx_pi_jit_amp = max_amp_idx_r20_os1[tx_pi_jit_freq_idx];
        }
    }

    EFUN(plp_aperta_blackhawk_tsc_tx_pi_freq_override(sa__, enable, freq_override_val));

    if (enable) {
        EFUN(wr_tx_pi_jit_freq_idx(tx_pi_jit_freq_idx));
        EFUN(wr_tx_pi_jit_amp(tx_pi_jit_amp));

        if (jit_type == TX_PI_SSC_HIGH_FREQ) {
            EFUN(wr_tx_pi_jit_ssc_freq_mode(0x1));        /* SSC_FREQ_MODE:             0 = 6G SSC mode, 1 = 10G SSC mode */
            EFUN(wr_tx_pi_ssc_gen_en(0x1));               /* SSC jitter enable:         0 = disabled,    1 = enabled */
        }
        else if (jit_type == TX_PI_SSC_LOW_FREQ) {
            EFUN(wr_tx_pi_jit_ssc_freq_mode(0x0));        /* SSC_FREQ_MODE:             0 = 6G SSC mode, 1 = 10G SSC mode */
            EFUN(wr_tx_pi_ssc_gen_en(0x1));               /* SSC jitter enable:         0 = disabled,    1 = enabled */
        }
        else if (jit_type == TX_PI_SJ) {
            EFUN(wr_tx_pi_sj_gen_en(0x1));                /* Sinusoidal jitter enable:  0 = disabled,    1 = enabled */
        }
    }
    else {
        EFUN(wr_tx_pi_ssc_gen_en(0x0));                   /* SSC jitter enable:         0 = disabled,    1 = enabled */
        EFUN(wr_tx_pi_sj_gen_en(0x0));                    /* Sinusoidal jitter enable:  0 = disabled,    1 = enabled */
    }
  return (ERR_CODE_NONE);
}


/*******************************/
/*  Isolate Serdes Input Pins  */
/*******************************/

err_code_t plp_aperta_blackhawk_tsc_isolate_ctrl_pins(srds_access_t *sa__, uint8_t enable) {
    uint8_t lane, lane_orig, num_lanes;
#ifdef PHYMOD_SUPPORT
    srds_info_t *blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__);
#endif /* PHYMOD_SUPPORT */
    uint8_t pll, pll_orig;

#ifdef PHYMOD_SUPPORT
    if ( blackhawk_tsc_info_ptr == NULL ) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    } else if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    }
#endif /* PHYMOD_SUPPORT */

    ESTM(pll_orig = plp_aperta_blackhawk_tsc_get_pll_idx(sa__));

    for(pll = 0; pll < DUAL_PLL_NUM_PLLS; pll++) {
        EFUN(plp_aperta_blackhawk_tsc_set_pll_idx(sa__, pll));
        EFUN(plp_aperta_blackhawk_tsc_isolate_core_ctrl_pins(sa__, enable));
    }
    EFUN(plp_aperta_blackhawk_tsc_set_pll_idx(sa__, pll_orig));

    ESTM(lane_orig = plp_aperta_blackhawk_tsc_get_lane(sa__));
    /* read num lanes per core directly from register */
    ESTM(num_lanes = rdc_revid_multiplicity());
    for(lane = 0; lane < num_lanes; lane++) {
        EFUN(plp_aperta_blackhawk_tsc_set_lane(sa__, lane));
        EFUN(plp_aperta_blackhawk_tsc_isolate_lane_ctrl_pins(sa__, enable));
    }
    EFUN(plp_aperta_blackhawk_tsc_set_lane(sa__, lane_orig));

  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_isolate_lane_ctrl_pins(srds_access_t *sa__, uint8_t enable) {

  if (enable) {
    EFUN(wr_pmd_ln_tx_h_pwrdn_pkill(0x1));
    EFUN(wr_pmd_ln_rx_h_pwrdn_pkill(0x1));
    EFUN(wr_pmd_ln_dp_h_rstb_pkill(0x1));
    EFUN(wr_pmd_ln_h_rstb_pkill(0x1));
    EFUN(wr_pmd_tx_disable_pkill(0x1));
  }
  else {
    EFUN(wr_pmd_ln_tx_h_pwrdn_pkill(0x0));
    EFUN(wr_pmd_ln_rx_h_pwrdn_pkill(0x0));
    EFUN(wr_pmd_ln_dp_h_rstb_pkill(0x0));
    EFUN(wr_pmd_ln_h_rstb_pkill(0x0));
    EFUN(wr_pmd_tx_disable_pkill(0x0));
  }
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_isolate_core_ctrl_pins(srds_access_t *sa__, uint8_t enable) {

  if (enable) {
    EFUN(wrc_pmd_core_dp_h_rstb_pkill(0x1));
  }
  else {
    EFUN(wrc_pmd_core_dp_h_rstb_pkill(0x0));
  }
  return (ERR_CODE_NONE);
}

/* BLACKHAWK TODO:  Differentify from Falcon16. */


err_code_t plp_aperta_blackhawk_tsc_log_full_pmd_state_noPRBS (srds_access_t *sa__, struct blackhawk_tsc_detailed_lane_status_st *lane_st) {
    uint16_t reg_data, reg_data1, reg_data2, reg_data3, reg_data4;
    int8_t   tmp;
    uint8_t  tmp1;
    const uint8_t big_endian = plp_aperta_blackhawk_tsc_INTERNAL_is_big_endian();

    if(!lane_st)
      return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));

    ESTM(lane_st->pmd_lock = rd_pmd_rx_lock());
    {
        err_code_t err_code=ERR_CODE_NONE;
        lane_st->stop_state = plp_aperta_blackhawk_tsc_INTERNAL_stop_micro(sa__,lane_st->pmd_lock,&err_code);
        if(err_code) USR_PRINTF(("Unable to stop microcontroller,  following data is suspect\n"));
    }

    ESTM(lane_st->reset_state = rd_lane_dp_reset_state());
    /*populate the big endian info */
    lane_st->big_endian = big_endian;

    /* Checking to make sure that micro hasn't kept rx_pi_disp_msb_status = 1 */
  {
    uint8_t disp_msb_status=0;
    ESTM(disp_msb_status = rd_rx_pi_disp_msb_status());
    if(disp_msb_status == 1){
        USR_PRINTF(("rx_pi_disp_msb_status read value = 1\n"));
        return ERR_CODE_RX_PI_DISP_MSB_STATUS_IS_1;
    }
  }
  {
     enum blackhawk_tsc_rx_pam_mode_enum pam_mode = NRZ;
     EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
     lane_st->rx_pam_mode = (uint8_t)pam_mode;
  }

    ESTM(lane_st->pmd_tx_pll_select = rd_tx_pll_select());
    ESTM(lane_st->pmd_rx_pll_select = rd_rx_pll_select());

    ESTM(lane_st->restart_count = rdv_usr_sts_restart_counter());
    ESTM(lane_st->reset_count = rdv_usr_sts_reset_counter());
    ESTM(lane_st->pmd_lock_count = rdv_usr_sts_pmd_lock_counter());

    ESTM(lane_st->temp_idx = rdcv_temp_idx());
    ESTM(lane_st->ams_tx_drv_hv_disable = rd_ams_tx_drv_hv_disable());
    ESTM(lane_st->ams_tx_ana_rescal = rd_ams_tx_ana_rescal());
    ESTM(lane_st->tx_pam_mode = rd_tx_pam4_mode());
    ESTM(lane_st->tx_prec_en  = rd_pam4_precoder_en());
    ESTM(lane_st->txfir_use_pam4_range = rd_txfir_nrz_tap_range_sel() ? 0 : 1);
    {
        uint8_t tap_num;
        for (tap_num=0; tap_num<12; ++tap_num) {
            EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_tx_tap(sa__, tap_num, &(lane_st->txfir.tap[tap_num])));
        }
        ESTM(tap_num = rd_txfir_tap_en());
        lane_st->num_txfir_taps = (tap_num == 0) ? 3 : ((tap_num == 1) ? 6 : ((tap_num == 2) ? 9 : 12));
    }
    ESTM(lane_st->sigdet = rd_signal_detect());
    ESTM(lane_st->dsc_sm[0] = rd_dsc_state_one_hot());
    ESTM(lane_st->dsc_sm[1] = rd_dsc_state_one_hot());
    ESTM(lane_st->ppm = rd_cdr_integ_reg());
    ESTM(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_vga(sa__, &tmp1)); lane_st->vga = tmp1;
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pf_main(sa__, &tmp1)); lane_st->pf = tmp1;
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pf2(sa__, &tmp1)); lane_st->pf2 = tmp1;
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_rx_pf3(sa__, &tmp1)); lane_st->pf3 = tmp1;
    ESTM(lane_st->main_tap_est = rdv_usr_main_tap_est());
    ESTM(lane_st->data23_thresh  = rd_rx_data23_status());
    ESTM(lane_st->data14_thresh  = rd_rx_data14_status());
    ESTM(lane_st->data05_thresh  = rd_rx_data05_status());
    ESTM(lane_st->phase1_thresh  = rd_rx_phase1_status());
    ESTM(lane_st->phase02_thresh = rd_rx_phase02_status());
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_lms_thresh_bin(sa__, &tmp)); lane_st->lms_thresh = tmp;
    ESTM(lane_st->rx_pi_cnt_perr     = rd_rx_pi_cnt_bin_d_perr());
    ESTM(lane_st->clk90     = rdv_usr_sts_phase_hoffset());
    ESTM(reg_data = reg_rd_DSC_E_RX_PI_CNT_BIN_D());
    ESTM(reg_data1 = reg_rd_DSC_E_RX_PI_CNT_BIN_P());
    ESTM(reg_data2 = reg_rd_DSC_E_RX_PI_CNT_BIN_L());
    ESTM(reg_data3 = reg_rd_DSC_E_RX_PI_CNT_BIN_PD());
    ESTM(reg_data4 = reg_rd_DSC_E_RX_PI_CNT_BIN_LD());
    lane_st->ddq_hoffset = (int8_t)((reg_data&0xFF) - (((reg_data>>8)&0xFF) + (int8_t)(-128)));
    lane_st->ppq_hoffset = (int8_t)((reg_data1&0xFF) - (((reg_data1>>8)&0xFF) + (int8_t)(-128)));
    lane_st->llq_hoffset = (int8_t)((reg_data2&0xFF) - (((reg_data2>>8)&0xFF) + (int8_t)(-128)));
    lane_st->dp_hoffset = (int8_t)(((reg_data3>>8)&0xFF) - (reg_data3&0xFF));
    lane_st->dl_hoffset = (int8_t)(((reg_data4>>8)&0xFF) - (reg_data4&0xFF));
    ESTM(lane_st->lms_dac4ck[0] = (int8_t)rd_ams_rx_dac4ck_lms_i());
    ESTM(lane_st->lms_dac4ck[1] = (int8_t)rd_ams_rx_dac4ck_lms_q());
    /* Convert from sign+gray to sign+bin */
    lane_st->lms_dac4ck[0] = (int8_t)((lane_st->lms_dac4ck[0]&0x8)|plp_aperta_blackhawk_tsc_INTERNAL_gray_to_uint8(lane_st->lms_dac4ck[0]&0x07));
    lane_st->lms_dac4ck[1] = (int8_t)((lane_st->lms_dac4ck[1]&0x8)|plp_aperta_blackhawk_tsc_INTERNAL_gray_to_uint8(lane_st->lms_dac4ck[1]&0x07));
    if (lane_st->lms_dac4ck[0]>7) lane_st->lms_dac4ck[0] = (int8_t)(-(lane_st->lms_dac4ck[0]&0x07));
    if (lane_st->lms_dac4ck[1]>7) lane_st->lms_dac4ck[1] = (int8_t)(-(lane_st->lms_dac4ck[1]&0x07));
    ESTM(lane_st->phase_dac4ck[0] = (int8_t)rd_ams_rx_dac4ck_phs_i());
    ESTM(lane_st->phase_dac4ck[1] = (int8_t)rd_ams_rx_dac4ck_phs_q());
    /* Convert from sign+gray to sign+bin */
    lane_st->phase_dac4ck[0] = (int8_t)((lane_st->phase_dac4ck[0]&0x8)|plp_aperta_blackhawk_tsc_INTERNAL_gray_to_uint8(lane_st->phase_dac4ck[0]&0x07));
    lane_st->phase_dac4ck[1] = (int8_t)((lane_st->phase_dac4ck[1]&0x8)|plp_aperta_blackhawk_tsc_INTERNAL_gray_to_uint8(lane_st->phase_dac4ck[1]&0x07));
    if (lane_st->phase_dac4ck[0]>7) lane_st->phase_dac4ck[0] = (int8_t)(-(lane_st->phase_dac4ck[0]&0x07));
    if (lane_st->phase_dac4ck[1]>7) lane_st->phase_dac4ck[1] = (int8_t)(-(lane_st->phase_dac4ck[1]&0x07));
    ESTM(lane_st->data_dac4ck[0] = (int8_t)rd_ams_rx_dac4ck_dat_i());
    ESTM(lane_st->data_dac4ck[1] = (int8_t)rd_ams_rx_dac4ck_dat_q());
    /* Convert from sign+gray to sign+bin */
    lane_st->data_dac4ck[0] = (int8_t)((lane_st->data_dac4ck[0]&0x8)|plp_aperta_blackhawk_tsc_INTERNAL_gray_to_uint8(lane_st->data_dac4ck[0]&0x07));
    lane_st->data_dac4ck[1] = (int8_t)((lane_st->data_dac4ck[1]&0x8)|plp_aperta_blackhawk_tsc_INTERNAL_gray_to_uint8(lane_st->data_dac4ck[1]&0x07));
    if (lane_st->data_dac4ck[0]>7) lane_st->data_dac4ck[0] = (int8_t)(-(lane_st->data_dac4ck[0]&0x07));
    if (lane_st->data_dac4ck[1]>7) lane_st->data_dac4ck[1] = (int8_t)(-(lane_st->data_dac4ck[1]&0x07));
    ESTM(lane_st->dc_offset = rd_dc_offset_bin());
    ESTM(lane_st->dfe[1][0] = rd_rxa_dfe_tap2_status());
    ESTM(lane_st->dfe[1][1] = rd_rxb_dfe_tap2_status());
    ESTM(lane_st->dfe[1][2] = rd_rxc_dfe_tap2_status());
    ESTM(lane_st->dfe[1][3] = rd_rxd_dfe_tap2_status());
    ESTM(lane_st->dfe[2][0] = rd_rxa_dfe_tap3_status());
    ESTM(lane_st->dfe[2][1] = rd_rxb_dfe_tap3_status());
    ESTM(lane_st->dfe[2][2] = rd_rxc_dfe_tap3_status());
    ESTM(lane_st->dfe[2][3] = rd_rxd_dfe_tap3_status());
    ESTM(lane_st->dfe[3][0] = rd_rxa_dfe_tap4());
    ESTM(lane_st->dfe[3][1] = rd_rxb_dfe_tap4());
    ESTM(lane_st->dfe[3][2] = rd_rxc_dfe_tap4());
    ESTM(lane_st->dfe[3][3] = rd_rxd_dfe_tap4());
    ESTM(lane_st->dfe[4][0] = rd_rxa_dfe_tap5());
    ESTM(lane_st->dfe[4][1] = rd_rxb_dfe_tap5());
    ESTM(lane_st->dfe[4][2] = rd_rxc_dfe_tap5());
    ESTM(lane_st->dfe[4][3] = rd_rxd_dfe_tap5());
    ESTM(lane_st->dfe[5][0] = rd_rxa_dfe_tap6());
    ESTM(lane_st->dfe[5][1] = rd_rxb_dfe_tap6());
    ESTM(lane_st->dfe[5][2] = rd_rxc_dfe_tap6());
    ESTM(lane_st->dfe[5][3] = rd_rxd_dfe_tap6());
    ESTM(lane_st->dfe[6][0] = ((rd_rxa_dfe_tap7_mux()==0)?rd_rxa_dfe_tap7():0));
    ESTM(lane_st->dfe[6][1] = ((rd_rxb_dfe_tap7_mux()==0)?rd_rxb_dfe_tap7():0));
    ESTM(lane_st->dfe[6][2] = ((rd_rxc_dfe_tap7_mux()==0)?rd_rxc_dfe_tap7():0));
    ESTM(lane_st->dfe[6][3] = ((rd_rxd_dfe_tap7_mux()==0)?rd_rxd_dfe_tap7():0));
    ESTM(lane_st->dfe[7][0] = ((rd_rxa_dfe_tap8_mux()==0)?rd_rxa_dfe_tap8():0));
    ESTM(lane_st->dfe[7][1] = ((rd_rxb_dfe_tap8_mux()==0)?rd_rxb_dfe_tap8():0));
    ESTM(lane_st->dfe[7][2] = ((rd_rxc_dfe_tap8_mux()==0)?rd_rxc_dfe_tap8():0));
    ESTM(lane_st->dfe[7][3] = ((rd_rxd_dfe_tap8_mux()==0)?rd_rxd_dfe_tap8():0));
    ESTM(lane_st->dfe[8][0] = ((rd_rxa_dfe_tap9_mux()==0)?rd_rxa_dfe_tap9():0));
    ESTM(lane_st->dfe[8][1] = ((rd_rxb_dfe_tap9_mux()==0)?rd_rxb_dfe_tap9():0));
    ESTM(lane_st->dfe[8][2] = ((rd_rxc_dfe_tap9_mux()==0)?rd_rxc_dfe_tap9():0));
    ESTM(lane_st->dfe[8][3] = ((rd_rxd_dfe_tap9_mux()==0)?rd_rxd_dfe_tap9():0));
    ESTM(lane_st->dfe[9][0] = ((rd_rxa_dfe_tap10_mux()==0)?rd_rxa_dfe_tap10():0));
    ESTM(lane_st->dfe[9][1] = ((rd_rxb_dfe_tap10_mux()==0)?rd_rxb_dfe_tap10():0));
    ESTM(lane_st->dfe[9][2] = ((rd_rxc_dfe_tap10_mux()==0)?rd_rxc_dfe_tap10():0));
    ESTM(lane_st->dfe[9][3] = ((rd_rxd_dfe_tap10_mux()==0)?rd_rxd_dfe_tap10():0));
    ESTM(lane_st->dfe[10][0] = ((rd_rxa_dfe_tap7_mux()==1)?rd_rxa_dfe_tap7():(rd_rxa_dfe_tap11_mux()==0)?rd_rxa_dfe_tap11():0));
    ESTM(lane_st->dfe[10][1] = ((rd_rxb_dfe_tap7_mux()==1)?rd_rxb_dfe_tap7():(rd_rxb_dfe_tap11_mux()==0)?rd_rxb_dfe_tap11():0));
    ESTM(lane_st->dfe[10][2] = ((rd_rxc_dfe_tap7_mux()==1)?rd_rxc_dfe_tap7():(rd_rxc_dfe_tap11_mux()==0)?rd_rxc_dfe_tap11():0));
    ESTM(lane_st->dfe[10][3] = ((rd_rxd_dfe_tap7_mux()==1)?rd_rxd_dfe_tap7():(rd_rxd_dfe_tap11_mux()==0)?rd_rxd_dfe_tap11():0));
    ESTM(lane_st->dfe[11][0] = ((rd_rxa_dfe_tap8_mux()==1)?rd_rxa_dfe_tap8():(rd_rxa_dfe_tap12_mux()==0)?rd_rxa_dfe_tap12():0));
    ESTM(lane_st->dfe[11][1] = ((rd_rxb_dfe_tap8_mux()==1)?rd_rxb_dfe_tap8():(rd_rxb_dfe_tap12_mux()==0)?rd_rxb_dfe_tap12():0));
    ESTM(lane_st->dfe[11][2] = ((rd_rxc_dfe_tap8_mux()==1)?rd_rxc_dfe_tap8():(rd_rxc_dfe_tap12_mux()==0)?rd_rxc_dfe_tap12():0));
    ESTM(lane_st->dfe[11][3] = ((rd_rxd_dfe_tap8_mux()==1)?rd_rxd_dfe_tap8():(rd_rxd_dfe_tap12_mux()==0)?rd_rxd_dfe_tap12():0));
    ESTM(lane_st->dfe[12][0] = ((rd_rxa_dfe_tap9_mux()==1)?rd_rxa_dfe_tap9():(rd_rxa_dfe_tap13_mux()==0)?rd_rxa_dfe_tap13():0));
    ESTM(lane_st->dfe[12][1] = ((rd_rxb_dfe_tap9_mux()==1)?rd_rxb_dfe_tap9():(rd_rxb_dfe_tap13_mux()==0)?rd_rxb_dfe_tap13():0));
    ESTM(lane_st->dfe[12][2] = ((rd_rxc_dfe_tap9_mux()==1)?rd_rxc_dfe_tap9():(rd_rxc_dfe_tap13_mux()==0)?rd_rxc_dfe_tap13():0));
    ESTM(lane_st->dfe[12][3] = ((rd_rxd_dfe_tap9_mux()==1)?rd_rxd_dfe_tap9():(rd_rxd_dfe_tap13_mux()==0)?rd_rxd_dfe_tap13():0));
    ESTM(lane_st->dfe[13][0] = ((rd_rxa_dfe_tap10_mux()==1)?rd_rxa_dfe_tap10():(rd_rxa_dfe_tap14_mux()==0)?rd_rxa_dfe_tap14():0));
    ESTM(lane_st->dfe[13][1] = ((rd_rxb_dfe_tap10_mux()==1)?rd_rxb_dfe_tap10():(rd_rxb_dfe_tap14_mux()==0)?rd_rxb_dfe_tap14():0));
    ESTM(lane_st->dfe[13][2] = ((rd_rxc_dfe_tap10_mux()==1)?rd_rxc_dfe_tap10():(rd_rxc_dfe_tap14_mux()==0)?rd_rxc_dfe_tap14():0));
    ESTM(lane_st->dfe[13][3] = ((rd_rxd_dfe_tap10_mux()==1)?rd_rxd_dfe_tap10():(rd_rxd_dfe_tap14_mux()==0)?rd_rxd_dfe_tap14():0));
    ESTM(lane_st->dfe[14][0] = ((rd_rxa_dfe_tap7_mux()==2)?rd_rxa_dfe_tap7():(rd_rxa_dfe_tap11_mux()==1)?rd_rxa_dfe_tap11():0));
    ESTM(lane_st->dfe[14][1] = ((rd_rxb_dfe_tap7_mux()==2)?rd_rxb_dfe_tap7():(rd_rxb_dfe_tap11_mux()==1)?rd_rxb_dfe_tap11():0));
    ESTM(lane_st->dfe[14][2] = ((rd_rxc_dfe_tap7_mux()==2)?rd_rxc_dfe_tap7():(rd_rxc_dfe_tap11_mux()==1)?rd_rxc_dfe_tap11():0));
    ESTM(lane_st->dfe[14][3] = ((rd_rxd_dfe_tap7_mux()==2)?rd_rxd_dfe_tap7():(rd_rxd_dfe_tap11_mux()==1)?rd_rxd_dfe_tap11():0));
    ESTM(lane_st->dfe[15][0] = ((rd_rxa_dfe_tap8_mux()==2)?rd_rxa_dfe_tap8():(rd_rxa_dfe_tap12_mux()==1)?rd_rxa_dfe_tap12():0));
    ESTM(lane_st->dfe[15][1] = ((rd_rxb_dfe_tap8_mux()==2)?rd_rxb_dfe_tap8():(rd_rxb_dfe_tap12_mux()==1)?rd_rxb_dfe_tap12():0));
    ESTM(lane_st->dfe[15][2] = ((rd_rxc_dfe_tap8_mux()==2)?rd_rxc_dfe_tap8():(rd_rxc_dfe_tap12_mux()==1)?rd_rxc_dfe_tap12():0));
    ESTM(lane_st->dfe[15][3] = ((rd_rxd_dfe_tap8_mux()==2)?rd_rxd_dfe_tap8():(rd_rxd_dfe_tap12_mux()==1)?rd_rxd_dfe_tap12():0));
    ESTM(lane_st->dfe[16][0] = ((rd_rxa_dfe_tap9_mux()==2)?rd_rxa_dfe_tap9():(rd_rxa_dfe_tap13_mux()==1)?rd_rxa_dfe_tap13():0));
    ESTM(lane_st->dfe[16][1] = ((rd_rxb_dfe_tap9_mux()==2)?rd_rxb_dfe_tap9():(rd_rxb_dfe_tap13_mux()==1)?rd_rxb_dfe_tap13():0));
    ESTM(lane_st->dfe[16][2] = ((rd_rxc_dfe_tap9_mux()==2)?rd_rxc_dfe_tap9():(rd_rxc_dfe_tap13_mux()==1)?rd_rxc_dfe_tap13():0));
    ESTM(lane_st->dfe[16][3] = ((rd_rxd_dfe_tap9_mux()==2)?rd_rxd_dfe_tap9():(rd_rxd_dfe_tap13_mux()==1)?rd_rxd_dfe_tap13():0));
    ESTM(lane_st->dfe[17][0] = ((rd_rxa_dfe_tap10_mux()==2)?rd_rxa_dfe_tap10():(rd_rxa_dfe_tap14_mux()==1)?rd_rxa_dfe_tap14():0));
    ESTM(lane_st->dfe[17][1] = ((rd_rxb_dfe_tap10_mux()==2)?rd_rxb_dfe_tap10():(rd_rxb_dfe_tap14_mux()==1)?rd_rxb_dfe_tap14():0));
    ESTM(lane_st->dfe[17][2] = ((rd_rxc_dfe_tap10_mux()==2)?rd_rxc_dfe_tap10():(rd_rxc_dfe_tap14_mux()==1)?rd_rxc_dfe_tap14():0));
    ESTM(lane_st->dfe[17][3] = ((rd_rxd_dfe_tap10_mux()==2)?rd_rxd_dfe_tap10():(rd_rxd_dfe_tap14_mux()==1)?rd_rxd_dfe_tap14():0));
    ESTM(lane_st->dfe[18][0] = ((rd_rxa_dfe_tap7_mux()==3)?rd_rxa_dfe_tap7():(rd_rxa_dfe_tap11_mux()==2)?rd_rxa_dfe_tap11():0));
    ESTM(lane_st->dfe[18][1] = ((rd_rxb_dfe_tap7_mux()==3)?rd_rxb_dfe_tap7():(rd_rxb_dfe_tap11_mux()==2)?rd_rxb_dfe_tap11():0));
    ESTM(lane_st->dfe[18][2] = ((rd_rxc_dfe_tap7_mux()==3)?rd_rxc_dfe_tap7():(rd_rxc_dfe_tap11_mux()==2)?rd_rxc_dfe_tap11():0));
    ESTM(lane_st->dfe[18][3] = ((rd_rxd_dfe_tap7_mux()==3)?rd_rxd_dfe_tap7():(rd_rxd_dfe_tap11_mux()==2)?rd_rxd_dfe_tap11():0));
    ESTM(lane_st->dfe[19][0] = ((rd_rxa_dfe_tap8_mux()==3)?rd_rxa_dfe_tap8():(rd_rxa_dfe_tap12_mux()==2)?rd_rxa_dfe_tap12():0));
    ESTM(lane_st->dfe[19][1] = ((rd_rxb_dfe_tap8_mux()==3)?rd_rxb_dfe_tap8():(rd_rxb_dfe_tap12_mux()==2)?rd_rxb_dfe_tap12():0));
    ESTM(lane_st->dfe[19][2] = ((rd_rxc_dfe_tap8_mux()==3)?rd_rxc_dfe_tap8():(rd_rxc_dfe_tap12_mux()==2)?rd_rxc_dfe_tap12():0));
    ESTM(lane_st->dfe[19][3] = ((rd_rxd_dfe_tap8_mux()==3)?rd_rxd_dfe_tap8():(rd_rxd_dfe_tap12_mux()==2)?rd_rxd_dfe_tap12():0));
    ESTM(lane_st->dfe[20][0] = ((rd_rxa_dfe_tap9_mux()==3)?rd_rxa_dfe_tap9():(rd_rxa_dfe_tap13_mux()==2)?rd_rxa_dfe_tap13():0));
    ESTM(lane_st->dfe[20][1] = ((rd_rxb_dfe_tap9_mux()==3)?rd_rxb_dfe_tap9():(rd_rxb_dfe_tap13_mux()==2)?rd_rxb_dfe_tap13():0));
    ESTM(lane_st->dfe[20][2] = ((rd_rxc_dfe_tap9_mux()==3)?rd_rxc_dfe_tap9():(rd_rxc_dfe_tap13_mux()==2)?rd_rxc_dfe_tap13():0));
    ESTM(lane_st->dfe[20][3] = ((rd_rxd_dfe_tap9_mux()==3)?rd_rxd_dfe_tap9():(rd_rxd_dfe_tap13_mux()==2)?rd_rxd_dfe_tap13():0));
    ESTM(lane_st->dfe[21][0] = ((rd_rxa_dfe_tap10_mux()==3)?rd_rxa_dfe_tap10():(rd_rxa_dfe_tap14_mux()==2)?rd_rxa_dfe_tap14():0));
    ESTM(lane_st->dfe[21][1] = ((rd_rxb_dfe_tap10_mux()==3)?rd_rxb_dfe_tap10():(rd_rxb_dfe_tap14_mux()==2)?rd_rxb_dfe_tap14():0));
    ESTM(lane_st->dfe[21][2] = ((rd_rxc_dfe_tap10_mux()==3)?rd_rxc_dfe_tap10():(rd_rxc_dfe_tap14_mux()==2)?rd_rxc_dfe_tap14():0));
    ESTM(lane_st->dfe[21][3] = ((rd_rxd_dfe_tap10_mux()==3)?rd_rxd_dfe_tap10():(rd_rxd_dfe_tap14_mux()==2)?rd_rxd_dfe_tap14():0));
    ESTM(lane_st->dfe[22][0] = ((rd_rxa_dfe_tap11_mux()==3)?rd_rxa_dfe_tap11():0));
    ESTM(lane_st->dfe[22][1] = ((rd_rxb_dfe_tap11_mux()==3)?rd_rxb_dfe_tap11():0));
    ESTM(lane_st->dfe[22][2] = ((rd_rxc_dfe_tap11_mux()==3)?rd_rxc_dfe_tap11():0));
    ESTM(lane_st->dfe[22][3] = ((rd_rxd_dfe_tap11_mux()==3)?rd_rxd_dfe_tap11():0));
    ESTM(lane_st->dfe[23][0] = ((rd_rxa_dfe_tap12_mux()==3)?rd_rxa_dfe_tap12():0));
    ESTM(lane_st->dfe[23][1] = ((rd_rxb_dfe_tap12_mux()==3)?rd_rxb_dfe_tap12():0));
    ESTM(lane_st->dfe[23][2] = ((rd_rxc_dfe_tap12_mux()==3)?rd_rxc_dfe_tap12():0));
    ESTM(lane_st->dfe[23][3] = ((rd_rxd_dfe_tap12_mux()==3)?rd_rxd_dfe_tap12():0));
    ESTM(lane_st->dfe[24][0] = ((rd_rxa_dfe_tap13_mux()==3)?rd_rxa_dfe_tap13():0));
    ESTM(lane_st->dfe[24][1] = ((rd_rxb_dfe_tap13_mux()==3)?rd_rxb_dfe_tap13():0));
    ESTM(lane_st->dfe[24][2] = ((rd_rxc_dfe_tap13_mux()==3)?rd_rxc_dfe_tap13():0));
    ESTM(lane_st->dfe[24][3] = ((rd_rxd_dfe_tap13_mux()==3)?rd_rxd_dfe_tap13():0));
    ESTM(lane_st->dfe[25][0] = ((rd_rxa_dfe_tap14_mux()==3)?rd_rxa_dfe_tap14():0));
    ESTM(lane_st->dfe[25][1] = ((rd_rxb_dfe_tap14_mux()==3)?rd_rxb_dfe_tap14():0));
    ESTM(lane_st->dfe[25][2] = ((rd_rxc_dfe_tap14_mux()==3)?rd_rxc_dfe_tap14():0));
    ESTM(lane_st->dfe[25][3] = ((rd_rxd_dfe_tap14_mux()==3)?rd_rxd_dfe_tap14():0));
    ESTM(lane_st->thctrl_d0[0] = rd_rxa_slicer_offset_adj_cal_d0());
    ESTM(lane_st->thctrl_d0[1] = rd_rxb_slicer_offset_adj_cal_d0());
    ESTM(lane_st->thctrl_d0[2] = rd_rxc_slicer_offset_adj_cal_d0());
    ESTM(lane_st->thctrl_d0[3] = rd_rxd_slicer_offset_adj_cal_d0());
    ESTM(lane_st->thctrl_d1[0] = rd_rxa_slicer_offset_adj_cal_d1());
    ESTM(lane_st->thctrl_d1[1] = rd_rxb_slicer_offset_adj_cal_d1());
    ESTM(lane_st->thctrl_d1[2] = rd_rxc_slicer_offset_adj_cal_d1());
    ESTM(lane_st->thctrl_d1[3] = rd_rxd_slicer_offset_adj_cal_d1());
    ESTM(lane_st->thctrl_d2[0] = rd_rxa_slicer_offset_adj_cal_d2());
    ESTM(lane_st->thctrl_d2[1] = rd_rxb_slicer_offset_adj_cal_d2());
    ESTM(lane_st->thctrl_d2[2] = rd_rxc_slicer_offset_adj_cal_d2());
    ESTM(lane_st->thctrl_d2[3] = rd_rxd_slicer_offset_adj_cal_d2());
    ESTM(lane_st->thctrl_d3[0] = rd_rxa_slicer_offset_adj_cal_d3());
    ESTM(lane_st->thctrl_d3[1] = rd_rxb_slicer_offset_adj_cal_d3());
    ESTM(lane_st->thctrl_d3[2] = rd_rxc_slicer_offset_adj_cal_d3());
    ESTM(lane_st->thctrl_d3[3] = rd_rxd_slicer_offset_adj_cal_d3());
    ESTM(lane_st->thctrl_d4[0] = rd_rxa_slicer_offset_adj_cal_d4());
    ESTM(lane_st->thctrl_d4[1] = rd_rxb_slicer_offset_adj_cal_d4());
    ESTM(lane_st->thctrl_d4[2] = rd_rxc_slicer_offset_adj_cal_d4());
    ESTM(lane_st->thctrl_d4[3] = rd_rxd_slicer_offset_adj_cal_d4());
    ESTM(lane_st->thctrl_d5[0] = rd_rxa_slicer_offset_adj_cal_d5());
    ESTM(lane_st->thctrl_d5[1] = rd_rxb_slicer_offset_adj_cal_d5());
    ESTM(lane_st->thctrl_d5[2] = rd_rxc_slicer_offset_adj_cal_d5());
    ESTM(lane_st->thctrl_d5[3] = rd_rxd_slicer_offset_adj_cal_d5());
    ESTM(lane_st->thctrl_p0[0] = rd_rxa_slicer_offset_adj_cal_p0());
    ESTM(lane_st->thctrl_p0[1] = rd_rxb_slicer_offset_adj_cal_p0());
    ESTM(lane_st->thctrl_p0[2] = rd_rxc_slicer_offset_adj_cal_p0());
    ESTM(lane_st->thctrl_p0[3] = rd_rxd_slicer_offset_adj_cal_p0());
    ESTM(lane_st->thctrl_p1[0] = rd_rxa_slicer_offset_adj_cal_p1());
    ESTM(lane_st->thctrl_p1[1] = rd_rxb_slicer_offset_adj_cal_p1());
    ESTM(lane_st->thctrl_p1[2] = rd_rxc_slicer_offset_adj_cal_p1());
    ESTM(lane_st->thctrl_p1[3] = rd_rxd_slicer_offset_adj_cal_p1());
    ESTM(lane_st->thctrl_p2[0] = rd_rxa_slicer_offset_adj_cal_p2());
    ESTM(lane_st->thctrl_p2[1] = rd_rxb_slicer_offset_adj_cal_p2());
    ESTM(lane_st->thctrl_p2[2] = rd_rxc_slicer_offset_adj_cal_p2());
    ESTM(lane_st->thctrl_p2[3] = rd_rxd_slicer_offset_adj_cal_p2());
    ESTM(lane_st->thctrl_l[0]  = rd_rxa_slicer_offset_adj_cal_lms());
    ESTM(lane_st->thctrl_l[1]  = rd_rxb_slicer_offset_adj_cal_lms());
    ESTM(lane_st->thctrl_l[2]  = rd_rxc_slicer_offset_adj_cal_lms());
    ESTM(lane_st->thctrl_l[3]  = rd_rxd_slicer_offset_adj_cal_lms());

    ESTM(lane_st->ams_rx_sd_cal_pos = rd_ams_rx_sd_cal_pos());
    ESTM(lane_st->ams_rx_sd_cal_neg = rd_ams_rx_sd_cal_neg());
    ESTM(lane_st->pam4_chn_loss = rdv_usr_ctrl_pam4_chn_loss());
    ESTM(lane_st->ams_rx_vga01_bw_ctrl_cap = rd_ams_rx_vga01_bw_ctrl_cap());
    ESTM(lane_st->ams_rx_vga2_bw_ctrl_cap  = rd_ams_rx_vga2_bw_ctrl_cap());
    ESTM(lane_st->ams_rx_vga_bw_ctrl_ind = rd_ams_rx_vga_bw_ctrl_ind());
    ESTM(lane_st->ams_rx_vga3_ind = rd_ams_rx_vga3_ind());
    ESTM(lane_st->ams_rx_pf3_max_bw = rd_ams_rx_pf3_max_bw());
    ESTM(lane_st->ams_rx_pf_max_bw = rd_ams_rx_pf_max_bw());
    ESTM(lane_st->ams_rx_vga0_rescal_mux = rd_ams_rx_vga0_rescal_mux());
    ESTM(lane_st->ams_rx_vga1_rescal_mux = rd_ams_rx_vga1_rescal_mux());
    ESTM(lane_st->ams_rx_vga2_rescal_mux = rd_ams_rx_vga2_rescal_mux());
    ESTM(lane_st->blw_gain = rd_blw_gain());
    ESTM(lane_st->dummy4 = rdv_usr_status_eq_debug1());
    ESTM(lane_st->dummy5 = rdv_usr_status_eq_debug2());
    ESTM(lane_st->dummy6 = rdv_usr_status_eq_debug3());

    ESTM(lane_st->dummy7[0][0] = rdv_lhc_sts_0_0());
    ESTM(lane_st->dummy7[0][1] = rdv_lhc_sts_0_1());
    ESTM(lane_st->dummy7[0][2] = rdv_lhc_sts_0_2());
    ESTM(lane_st->dummy7[0][3] = rdv_lhc_sts_0_3());
    ESTM(lane_st->dummy7[1][0] = rdv_lhc_sts_1_0());
    ESTM(lane_st->dummy7[1][1] = rdv_lhc_sts_1_1());
    ESTM(lane_st->dummy7[1][2] = rdv_lhc_sts_1_2());
    ESTM(lane_st->dummy7[1][3] = rdv_lhc_sts_1_3());
    ESTM(lane_st->dummy7[2][0] = rdv_lhc_sts_2_0());
    ESTM(lane_st->dummy7[2][1] = rdv_lhc_sts_2_1());
    ESTM(lane_st->dummy7[2][2] = rdv_lhc_sts_2_2());
    ESTM(lane_st->dummy7[2][3] = rdv_lhc_sts_2_3());
    ESTM(lane_st->dummy7[3][0] = rdv_lhc_sts_3_0());
    ESTM(lane_st->dummy7[3][1] = rdv_lhc_sts_3_1());
    ESTM(lane_st->dummy7[3][2] = rdv_lhc_sts_3_2());
    ESTM(lane_st->dummy7[3][3] = rdv_lhc_sts_3_3());
    ESTM(lane_st->dummy7[4][0] = rdv_lhc_sts_4_0());
    ESTM(lane_st->dummy7[4][1] = rdv_lhc_sts_4_1());
    ESTM(lane_st->dummy7[4][2] = rdv_lhc_sts_4_2());
    ESTM(lane_st->dummy7[4][3] = rdv_lhc_sts_4_3());
    ESTM(lane_st->dummy7[5][0] = rdv_lhc_sts_5_0());
    ESTM(lane_st->dummy7[5][1] = rdv_lhc_sts_5_1());
    ESTM(lane_st->dummy7[5][2] = rdv_lhc_sts_5_2());
    ESTM(lane_st->dummy7[5][3] = rdv_lhc_sts_5_3());
    ESTM(lane_st->dummy8[0][0] = rdv_lvc_sts_0_0());
    ESTM(lane_st->dummy8[0][1] = rdv_lvc_sts_0_1());
    ESTM(lane_st->dummy8[0][2] = rdv_lvc_sts_0_2());
    ESTM(lane_st->dummy8[0][3] = rdv_lvc_sts_0_3());
    ESTM(lane_st->dummy8[1][0] = rdv_lvc_sts_1_0());
    ESTM(lane_st->dummy8[1][1] = rdv_lvc_sts_1_1());
    ESTM(lane_st->dummy8[1][2] = rdv_lvc_sts_1_2());
    ESTM(lane_st->dummy8[1][3] = rdv_lvc_sts_1_3());
    ESTM(lane_st->dummy8[2][0] = rdv_lvc_sts_2_0());
    ESTM(lane_st->dummy8[2][1] = rdv_lvc_sts_2_1());
    ESTM(lane_st->dummy8[2][2] = rdv_lvc_sts_2_2());
    ESTM(lane_st->dummy8[2][3] = rdv_lvc_sts_2_3());
    ESTM(lane_st->dummy8[3][0] = rdv_lvc_sts_3_0());
    ESTM(lane_st->dummy8[3][1] = rdv_lvc_sts_3_1());
    ESTM(lane_st->dummy8[3][2] = rdv_lvc_sts_3_2());
    ESTM(lane_st->dummy8[3][3] = rdv_lvc_sts_3_3());
    ESTM(lane_st->dummy8[4][0] = rdv_lvc_sts_4_0());
    ESTM(lane_st->dummy8[4][1] = rdv_lvc_sts_4_1());
    ESTM(lane_st->dummy8[4][2] = rdv_lvc_sts_4_2());
    ESTM(lane_st->dummy8[4][3] = rdv_lvc_sts_4_3());
    ESTM(lane_st->dummy8[5][0] = rdv_lvc_sts_5_0());
    ESTM(lane_st->dummy8[5][1] = rdv_lvc_sts_5_1());
    ESTM(lane_st->dummy8[5][2] = rdv_lvc_sts_5_2());
    ESTM(lane_st->dummy8[5][3] = rdv_lvc_sts_5_3());
    ESTM(lane_st->dummy9[0] = rdv_psv_sts_byte());
    ESTM(lane_st->dummy9[1] = rdv_psv_ctl_byte());
    ESTM(lane_st->dummy9[2] = rdv_psv_slic_sts_0_byte());
    ESTM(lane_st->dummy9[3] = rdv_psv_slic_sts_1_byte());
    ESTM(lane_st->dummy9[4] = rdv_psv_slic_sts_2_byte());
    ESTM(lane_st->dummy9[5] = rdv_psv_slic_sts_3_byte());
    ESTM(lane_st->dummy9[6] = rdv_psv_slic_sts_4_byte());
    ESTM(lane_st->dummy9[7] = rdv_psv_slic_sts_5_byte());
    ESTM(lane_st->dummy9[8] = rdv_lth_ctl_0_byte());
    ESTM(lane_st->dummy9[9] = rdv_lth_ctl_1_byte());
    ESTM(lane_st->dummy9[10] = rdv_lth_ctl_2_byte());
    ESTM(lane_st->dummy9[11] = rdv_lth_ctl_3_byte());    
    ESTM(lane_st->dummy10[0][0] = rdv_lc_sts_0_0_byte());
    ESTM(lane_st->dummy10[0][1] = rdv_lc_sts_0_1_byte());
    ESTM(lane_st->dummy10[0][2] = rdv_lc_sts_0_2_byte());
    ESTM(lane_st->dummy10[0][3] = rdv_lc_sts_0_3_byte());
    ESTM(lane_st->dummy10[1][0] = rdv_lc_sts_1_0_byte());
    ESTM(lane_st->dummy10[1][1] = rdv_lc_sts_1_1_byte());
    ESTM(lane_st->dummy10[1][2] = rdv_lc_sts_1_2_byte());
    ESTM(lane_st->dummy10[1][3] = rdv_lc_sts_1_3_byte());
    ESTM(lane_st->dummy10[2][0] = rdv_lc_sts_2_0_byte());
    ESTM(lane_st->dummy10[2][1] = rdv_lc_sts_2_1_byte());
    ESTM(lane_st->dummy10[2][2] = rdv_lc_sts_2_2_byte());
    ESTM(lane_st->dummy10[2][3] = rdv_lc_sts_2_3_byte());
    ESTM(lane_st->dummy10[3][0] = rdv_lc_sts_3_0_byte());
    ESTM(lane_st->dummy10[3][1] = rdv_lc_sts_3_1_byte());
    ESTM(lane_st->dummy10[3][2] = rdv_lc_sts_3_2_byte());
    ESTM(lane_st->dummy10[3][3] = rdv_lc_sts_3_3_byte());
    ESTM(lane_st->dummy10[4][0] = rdv_lc_sts_4_0_byte());
    ESTM(lane_st->dummy10[4][1] = rdv_lc_sts_4_1_byte());
    ESTM(lane_st->dummy10[4][2] = rdv_lc_sts_4_2_byte());
    ESTM(lane_st->dummy10[4][3] = rdv_lc_sts_4_3_byte());
    ESTM(lane_st->dummy10[5][0] = rdv_lc_sts_5_0_byte());
    ESTM(lane_st->dummy10[5][1] = rdv_lc_sts_5_1_byte());
    ESTM(lane_st->dummy10[5][2] = rdv_lc_sts_5_2_byte());
    ESTM(lane_st->dummy10[5][3] = rdv_lc_sts_5_3_byte());
    ESTM(lane_st->dummy11[0][0] = rdv_lhr_sts_0_0());
    ESTM(lane_st->dummy11[0][1] = rdv_lhr_sts_0_1());
    ESTM(lane_st->dummy11[0][2] = rdv_lhr_sts_0_2());
    ESTM(lane_st->dummy11[0][3] = rdv_lhr_sts_0_3());
    ESTM(lane_st->dummy11[1][0] = rdv_lhr_sts_1_0());
    ESTM(lane_st->dummy11[1][1] = rdv_lhr_sts_1_1());
    ESTM(lane_st->dummy11[1][2] = rdv_lhr_sts_1_2());
    ESTM(lane_st->dummy11[1][3] = rdv_lhr_sts_1_3());
    ESTM(lane_st->dummy11[2][0] = rdv_lhr_sts_2_0());
    ESTM(lane_st->dummy11[2][1] = rdv_lhr_sts_2_1());
    ESTM(lane_st->dummy11[2][2] = rdv_lhr_sts_2_2());
    ESTM(lane_st->dummy11[2][3] = rdv_lhr_sts_2_3());
    ESTM(lane_st->dummy11[3][0] = rdv_lhr_sts_3_0());
    ESTM(lane_st->dummy11[3][1] = rdv_lhr_sts_3_1());
    ESTM(lane_st->dummy11[3][2] = rdv_lhr_sts_3_2());
    ESTM(lane_st->dummy11[3][3] = rdv_lhr_sts_3_3());
    ESTM(lane_st->dummy11[4][0] = rdv_lhr_sts_4_0());
    ESTM(lane_st->dummy11[4][1] = rdv_lhr_sts_4_1());
    ESTM(lane_st->dummy11[4][2] = rdv_lhr_sts_4_2());
    ESTM(lane_st->dummy11[4][3] = rdv_lhr_sts_4_3());
    ESTM(lane_st->dummy11[5][0] = rdv_lhr_sts_5_0());
    ESTM(lane_st->dummy11[5][1] = rdv_lhr_sts_5_1());
    ESTM(lane_st->dummy11[5][2] = rdv_lhr_sts_5_2());
    ESTM(lane_st->dummy11[5][3] = rdv_lhr_sts_5_3());

    ESTM(lane_st->dummy12[0][0] = rdv_lvr_sts_0_0());
    ESTM(lane_st->dummy12[0][1] = rdv_lvr_sts_0_1());
    ESTM(lane_st->dummy12[0][2] = rdv_lvr_sts_0_2());
    ESTM(lane_st->dummy12[0][3] = rdv_lvr_sts_0_3());
    ESTM(lane_st->dummy12[1][0] = rdv_lvr_sts_1_0());
    ESTM(lane_st->dummy12[1][1] = rdv_lvr_sts_1_1());
    ESTM(lane_st->dummy12[1][2] = rdv_lvr_sts_1_2());
    ESTM(lane_st->dummy12[1][3] = rdv_lvr_sts_1_3());
    ESTM(lane_st->dummy12[2][0] = rdv_lvr_sts_2_0());
    ESTM(lane_st->dummy12[2][1] = rdv_lvr_sts_2_1());
    ESTM(lane_st->dummy12[2][2] = rdv_lvr_sts_2_2());
    ESTM(lane_st->dummy12[2][3] = rdv_lvr_sts_2_3());
    ESTM(lane_st->dummy12[3][0] = rdv_lvr_sts_3_0());
    ESTM(lane_st->dummy12[3][1] = rdv_lvr_sts_3_1());
    ESTM(lane_st->dummy12[3][2] = rdv_lvr_sts_3_2());
    ESTM(lane_st->dummy12[3][3] = rdv_lvr_sts_3_3());
    ESTM(lane_st->dummy12[4][0] = rdv_lvr_sts_4_0());
    ESTM(lane_st->dummy12[4][1] = rdv_lvr_sts_4_1());
    ESTM(lane_st->dummy12[4][2] = rdv_lvr_sts_4_2());
    ESTM(lane_st->dummy12[4][3] = rdv_lvr_sts_4_3());
    ESTM(lane_st->dummy12[5][0] = rdv_lvr_sts_5_0());
    ESTM(lane_st->dummy12[5][1] = rdv_lvr_sts_5_1());
    ESTM(lane_st->dummy12[5][2] = rdv_lvr_sts_5_2());
    ESTM(lane_st->dummy12[5][3] = rdv_lvr_sts_5_3());
    ESTM(lane_st->dummy13 = rdv_usr_status_eq_debug4());

    ESTM(lane_st->dummy14    = rdv_usr_status_eq_debug10());
    ESTM(lane_st->dummy15[0] = rdv_usr_status_eq_debug9());
    ESTM(lane_st->dummy15[1] = rdv_usr_status_eq_debug8());
    ESTM(lane_st->dummy16    = rdv_usr_status_eq_debug7());
    ESTM(lane_st->dummy17[0] = rdv_usr_status_eq_debug6());
    ESTM(lane_st->dummy17[1] = rdv_usr_status_eq_debug5());
    ESTM(lane_st->dummy18[0] = rdv_usr_status_tp_metric_1());
    ESTM(lane_st->dummy18[1] = rdv_usr_status_tp_metric_2());
    ESTM(lane_st->dummy18[2] = rdv_usr_status_tp_metric_3());
    ESTM(lane_st->dummy19     = rdv_usr_status_eq_debug14());
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_eye_margin_est(sa__, &lane_st->heye_left, &lane_st->heye_right, &lane_st->veye_upper, &lane_st->veye_lower));
    ESTM(lane_st->link_time = (uint16_t)((((uint32_t)rdv_usr_sts_link_time())*8)/10));

    if (lane_st->pmd_lock == 1) {
      if (!lane_st->stop_state) {
        EFUN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__, 0));
      }
    } else {
        EFUN(plp_aperta_blackhawk_tsc_stop_rx_adaptation(sa__, 0));
    }

    return(ERR_CODE_NONE);
}

    const char* plp_aperta_e2s_rx_pam_mode_enum[3] = {
      "    NRZ",
      "PAM4_NS",
      "PAM4_ES"
    };

err_code_t plp_aperta_blackhawk_tsc_log_full_pmd_state (srds_access_t *sa__, struct blackhawk_tsc_detailed_lane_status_st *lane_st) {
    enum plp_aperta_srds_prbs_polynomial_enum prbs_poly_mode = PRBS_7;
    enum plp_aperta_srds_prbs_checker_mode_enum prbs_checker_mode = PRBS_SELF_SYNC_HYSTERESIS;

    if(!lane_st)
      return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));

    ENULL_MEMSET(lane_st, 0, sizeof(struct blackhawk_tsc_detailed_lane_status_st));
    EFUN(plp_aperta_blackhawk_tsc_log_full_pmd_state_noPRBS(sa__, lane_st));

    ESTM(lane_st->prbs_chk_en = rd_prbs_chk_en());
    EFUN(plp_aperta_blackhawk_tsc_get_rx_prbs_config(sa__, &prbs_poly_mode, &prbs_checker_mode, &lane_st->prbs_chk_inv));
    lane_st->prbs_chk_order = (uint8_t)prbs_poly_mode;

    EFUN(plp_aperta_blackhawk_tsc_prbs_chk_lock_state(sa__, &lane_st->prbs_chk_lock));
    EFUN(plp_aperta_blackhawk_tsc_prbs_err_count_ll(sa__, &lane_st->prbs_chk_errcnt));
    if (lane_st->prbs_chk_lock && (lane_st->rx_pam_mode != NRZ)) {
       EFUN(plp_aperta_blackhawk_tsc_collect_dbgfb_stats(sa__, 8000, &lane_st->dbgfb_stats));
    }
    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_get_BER_string(sa__,100,&lane_st->ber_string[0]));

    return(ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_disp_full_pmd_state (srds_access_t *sa__, struct blackhawk_tsc_detailed_lane_status_st const * const lane_st, uint8_t num_lanes) {
    const uint8_t num_bytes_each_line = 26;
    uint32_t i;
    uint32_t size_of_lane_st = 0;

    const uint8_t big_endian = plp_aperta_blackhawk_tsc_INTERNAL_is_big_endian();


    if(lane_st == NULL) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    if ((num_lanes != 4) && (num_lanes != 8) && (num_lanes != 1)) {
      return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));  /* Number of lanes has to be 1, 4 or 8 */
    }

    size_of_lane_st = sizeof(struct blackhawk_tsc_detailed_lane_status_st);

    EFUN_PRINTF(("\n**** SERDES EXTENDED LANE DIAG DATA CORE %d LANE %d DUMP ****", plp_aperta_blackhawk_tsc_get_core(sa__), plp_aperta_blackhawk_tsc_get_lane(sa__)));
    EFUN_PRINTF(("\n0000 "));
    ESTM_PRINTF(("%02x ", num_lanes));
    ESTM_PRINTF(("%02x ", big_endian));
    ESTM_PRINTF(("%02x ", (uint8_t)sizeof(struct blackhawk_tsc_check_platform_info_st)));
    /* For some customer builds, ESTM_PRINTF is an empty macro.
       In such cases, some compilers might generates an error of "unused variable" for big_endian.
       The void usage of big_endian down below - solves this compiler error */
    UNUSED(big_endian);
    for (i = 3; i < num_lanes*size_of_lane_st+3; i++) {
      if (!(i % num_bytes_each_line))  {
        EFUN_PRINTF(("\n%04x ", i));
      }
      ESTM_PRINTF(("%02x ", *(((uint8_t*)lane_st)+i-3)));
    }

    EFUN_PRINTF(("\n**** END OF DATA DUMP ****\n"));

    EFUN_PRINTF(("\n========== End of SERDES EXTENDED LANE DIAG DATA ==================\n"));

    return (ERR_CODE_NONE);
}


void plp_aperta_blackhawk_tsc_INTERNAL_print_triage_info(srds_access_t *sa__, err_code_t err_code, uint8_t print_header, uint8_t print_data, uint16_t line)
{
/*  Note: No EFUNs or ESTMs should be used in this function as this print routine is called by _error() handler. */
    blackhawk_tsc_triage_info info;
    err_code_t error;
    uint8_t error_seen = 0;
    err_code_t __err;
    uint16_t   ucode_version_major;
    uint8_t    ucode_version_minor;
    UNUSED(error);
    if (ERR_CODE_SRDS_REG_ACCESS_FAIL == err_code) {  /* Early return to prevent error handling recursion on Access Errors! */
        return;
    }
    PHYMOD_MEMSET(&info, 0, sizeof(blackhawk_tsc_triage_info));
    info.error = err_code;
    info.line = line;

    if (print_header) {
        USR_PRINTF(("Triage Info Below:\n"));
        if ((err_code == ERR_CODE_UC_CMD_POLLING_TIMEOUT) || (err_code == ERR_CODE_UC_NOT_STOPPED)) {
            USR_PRINTF(("Lane, Core,  API_VER, UCODE_VER, micro_stop_status, exception(sw,hw), stack_ovflw, cmd_info, pmd_lock, sigdet, dsc_one_hot(0,1), Error\n"));
        } else {
            USR_PRINTF(("Lane, Core,  API_VER, UCODE_VER, Error\n"));
        }
    }
    error = plp_aperta_blackhawk_tsc_version(sa__, &info.api_ver);
    if (error) {
        /* Unable to read api version */
        /*USR_PRINTF(("Unable to capture api version and so setting invalid value in the print\n"));*/
        info.api_ver = 0xFFFFFFFF;
        error_seen = 1;
    }
    CHECK_ERR(ucode_version_major = rdcv_common_ucode_version());
    CHECK_ERR(ucode_version_minor = rdcv_common_ucode_minor_version());
    info.ucode_ver = (uint32_t)((ucode_version_major << 8) | ucode_version_minor);
    CHECK_ERR(info.stop_status = rdv_usr_sts_micro_stopped());
        /* Collect exception and overflow information */
    {
        CHECK_ERR(info.stack_overflow   = rdc_micro_status_stack_overflowed());
        CHECK_ERR(info.overflow_lane_id = rdc_micro_status_stack_overflowed_laneID());
        CHECK_ERR(info.sw_exception     = rdc_micro_status_sw_exception_occurred());
        CHECK_ERR(info.hw_exception     = rdc_micro_status_hw_exception_occurred());
    }
    if(plp_aperta_blackhawk_tsc_INTERNAL_sigdet_status(sa__, &info.sig_det, &info.sig_det_chg)) {
        error_seen = 1;
    }
    if (plp_aperta_blackhawk_tsc_INTERNAL_pmd_lock_status(sa__, &info.pmd_lock, &info.pmd_lock_chg)) {
        error_seen = 1;
    }

    CHECK_ERR(info.dsc_one_hot[0] = rd_dsc_state_one_hot());
    CHECK_ERR(info.dsc_one_hot[1] = rd_dsc_state_one_hot());
    CHECK_ERR(info.cmd_info = reg_rd_DSC_A_DSC_UC_CTRL());
    info.core = plp_aperta_blackhawk_tsc_get_core(sa__);
    info.lane = plp_aperta_blackhawk_tsc_get_lane(sa__);
    if (print_data) {
        if ((err_code == ERR_CODE_UC_CMD_POLLING_TIMEOUT) || (err_code == ERR_CODE_UC_NOT_STOPPED)) {
            USR_PRINTF(("%4d, %4d,  %X_%X,    %X_%X, %17d, %7d,%d       , %11d,   0x%04x, %8d, %6d,    0x%x,0x%x   , %s\n",
                        info.lane,
                        info.core,
                        info.api_ver>>8,
                        info.api_ver & 0xFF,
                        info.ucode_ver >> 8,
                        info.ucode_ver & 0xFF,
                        info.stop_status,
                        info.sw_exception,
                        info.hw_exception,
                        info.stack_overflow,
                        info.cmd_info,
                        info.pmd_lock,
                        info.sig_det,
                        info.dsc_one_hot[0],
                        info.dsc_one_hot[1],
                        plp_aperta_blackhawk_tsc_INTERNAL_e2s_err_code(info.error) ));
        } else {
            USR_PRINTF(("%4d, %4d,  %X_%X,    %X_%X, %s\n",
                        info.lane,
                        info.core,
                        info.api_ver>>8,
                        info.api_ver & 0xFF,
                        info.ucode_ver >> 8,
                        info.ucode_ver & 0xFF,
                        plp_aperta_blackhawk_tsc_INTERNAL_e2s_err_code(info.error) ));
        }
    }
    if (error_seen) {
        USR_PRINTF(("WARNING: There were some errors seen while collecting triage info and so the debug data above may not be all accurate\n"));
    }
    return;
}


err_code_t plp_aperta_blackhawk_tsc_set_pam_eye_margin_usr_ctrl(srds_access_t *sa__, uint8_t enable) {
    struct blackhawk_tsc_usr_ctrl_disable_functions_st val;

    /* get current usr ctrl disable value */
    ESTM(plp_aperta_blackhawk_tsc_get_usr_ctrl_disable_steady_state(sa__, &val));

    /* Set/reset pam eye margin control */
    if(enable == 0) {
        val.field.pam_eye_margin_estimation = 1;
    } else {
        val.field.pam_eye_margin_estimation = 0;
    }
    EFUN(plp_aperta_blackhawk_tsc_set_usr_ctrl_disable_steady_state(sa__, val));
    return ERR_CODE_NONE;
}
