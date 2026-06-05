/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/***********************************************************************************
 ***********************************************************************************
 *  File Name     :  peregrine5_pc_debug_functions.c                                  *
 *  Created On    :  03 Nov 2015                                                   *
 *  Created By    :  Brent Roberts                                                 *
 *  Description   :  Debug APIs for Serdes IPs                                     *
 *  Revision      :                                                            *
 ***********************************************************************************
 ***********************************************************************************/

#include <phymod/phymod_system.h>
#include "peregrine5_pc_debug_functions.h"
#include "peregrine5_pc_access.h"
#include "peregrine5_pc_common.h"
#include "peregrine5_pc_config.h"
#include "peregrine5_pc_functions.h"
#include "peregrine5_pc_internal.h"
#include "peregrine5_pc_internal_error.h"
#include "peregrine5_pc_prbs.h"
#include "peregrine5_pc_select_defns.h"
#include "peregrine5_pc_reg_dump.h"


/** @file
 *
 */




#ifndef SMALL_FOOTPRINT
static err_code_t _plp_aperta2_peregrine5_pc_reg_print_no_buf(srds_access_t *sa__, uint16_t start_addr, uint16_t valid_addr);
static err_code_t _plp_aperta2_peregrine5_pc_reg_print_with_buf(srds_access_t *sa__, uint16_t start_addr, uint16_t valid_addr, char reg_buffer[SRDS_DUMP_BUF_SIZE]);
static err_code_t _plp_aperta2_peregrine5_pc_reg_print(srds_access_t *sa__, uint8_t reg_start_index, uint8_t reg_end_index, char reg_buffer[][SRDS_DUMP_BUF_SIZE], uint8_t *buf_index);
#endif /* SMALL_FOOTPRINT */


#ifndef SMALL_FOOTPRINT

/*************************/
/*  Stop/Resume uC Lane  */
/*************************/

err_code_t plp_aperta2_peregrine5_pc_stop_uc_lane(srds_access_t *sa__, uint8_t enable) {

    return(plp_aperta2_peregrine5_pc_stop_rx_adaptation(sa__,enable));
}

err_code_t plp_aperta2_peregrine5_pc_stop_uc_lane_status(srds_access_t *sa__, uint8_t *uc_lane_stopped) {
    INIT_SRDS_ERR_CODE

  if(!uc_lane_stopped) {
      return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*uc_lane_stopped = rdv_usr_sts_micro_stopped());

  return (ERR_CODE_NONE);
}

/*******************************************************************/
/*  APIs to Write Core/Lane Config and User variables into uC RAM  */
/*******************************************************************/
err_code_t plp_aperta2_peregrine5_pc_get_usr_event_log_group_mask(srds_access_t *sa__, uint32_t *event_group_mask) {
    INIT_SRDS_ERR_CODE

    if(event_group_mask == NULL) {
       return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

        ESTM(*event_group_mask = rdv_usr_event_log_group_mask());
        return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_set_usr_event_log_group_mask(srds_access_t *sa__, uint32_t event_group_mask) {
    INIT_SRDS_ERR_CODE

        EFUN(wrv_usr_event_log_group_mask(event_group_mask));
        return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_add_usr_event_log_group_mask(srds_access_t *sa__, uint32_t event_group_mask) {
    INIT_SRDS_ERR_CODE
    uint32_t mask;

        ESTM(mask = rdv_usr_event_log_group_mask());
        EFUN(wrv_usr_event_log_group_mask((mask | event_group_mask)));
        return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_delete_usr_event_log_group_mask(srds_access_t *sa__, uint32_t event_group_mask) {
    INIT_SRDS_ERR_CODE
    uint32_t mask;

        ESTM(mask = rdv_usr_event_log_group_mask());
        EFUN(wrv_usr_event_log_group_mask((mask & ~event_group_mask)));
        return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_set_usr_ctrl_core_event_log_level(srds_access_t *sa__, uint8_t core_event_log_level) {
    return(wrcv_usr_ctrl_core_event_log_level(core_event_log_level));
}

err_code_t plp_aperta2_peregrine5_pc_set_usr_ctrl_lane_event_log_level(srds_access_t *sa__, uint8_t lane_event_log_level) {
    INIT_SRDS_ERR_CODE

        if (lane_event_log_level > 4 )
            EFUN(plp_aperta2_peregrine5_pc_set_usr_event_log_group_mask(sa__, EVENT_GROUP_PRIORITY_5));
        else if (lane_event_log_level ==4 )
            EFUN(plp_aperta2_peregrine5_pc_set_usr_event_log_group_mask(sa__, EVENT_GROUP_PRIORITY_4));
        else if (lane_event_log_level ==3 )
            EFUN(plp_aperta2_peregrine5_pc_set_usr_event_log_group_mask(sa__, EVENT_GROUP_PRIORITY_3));
        else if (lane_event_log_level ==2 )
            EFUN(plp_aperta2_peregrine5_pc_set_usr_event_log_group_mask(sa__, EVENT_GROUP_PRIORITY_2));
        else if (lane_event_log_level ==1 )
            EFUN(plp_aperta2_peregrine5_pc_set_usr_event_log_group_mask(sa__, EVENT_GROUP_PRIORITY_1));
        else /* == 0  */
            EFUN(plp_aperta2_peregrine5_pc_set_usr_event_log_group_mask(sa__, EVENT_GROUP_PRIORITY_0));
        USR_PRINTF(("WARNING: Please use 'event_log_group_mask' feature with this build of uCode.\n"));
        /* allow to fall through and return legacy variable */
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_set_usr_ctrl_disable_startup(srds_access_t *sa__, struct peregrine5_pc_usr_ctrl_disable_functions_st set_val) {
    INIT_SRDS_ERR_CODE
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_usr_ctrl_disable_functions_byte(&set_val));
    return(wrv_usr_ctrl_disable_startup_functions_dword(set_val.dword));
}


err_code_t plp_aperta2_peregrine5_pc_set_usr_ctrl_disable_steady_state(srds_access_t *sa__, struct peregrine5_pc_usr_ctrl_disable_functions_st set_val) {
    INIT_SRDS_ERR_CODE
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_check_uc_lane_stopped(sa__));  /* make sure uC is stopped to avoid race conditions */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_usr_ctrl_disable_functions_byte(&set_val));
    return(wrv_usr_ctrl_disable_steady_state_functions_dword(set_val.dword));
}


err_code_t plp_aperta2_peregrine5_pc_set_usr_ctrl_disable_blind(srds_access_t *sa__, struct peregrine5_pc_usr_ctrl_disable_functions_st set_val) {
    INIT_SRDS_ERR_CODE
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_usr_ctrl_disable_functions_byte(&set_val));
    return(wrv_usr_ctrl_disable_blind_functions_dword(set_val.dword));
}

/******************************************************************/
/*  APIs to Read Core/Lane Config and User variables from uC RAM  */
/******************************************************************/

err_code_t plp_aperta2_peregrine5_pc_get_usr_ctrl_core_event_log_level(srds_access_t *sa__, uint8_t *core_event_log_level) {
    INIT_SRDS_ERR_CODE
    if(!core_event_log_level) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    ESTM(*core_event_log_level = rdcv_usr_ctrl_core_event_log_level());
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta2_peregrine5_pc_get_usr_ctrl_disable_startup(srds_access_t *sa__, struct peregrine5_pc_usr_ctrl_disable_functions_st *get_val) {
    INIT_SRDS_ERR_CODE

    if(!get_val) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ESTM(get_val->dword = rdv_usr_ctrl_disable_startup_functions_dword());
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_usr_ctrl_disable_functions_st(get_val));
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta2_peregrine5_pc_get_usr_ctrl_disable_steady_state(srds_access_t *sa__, struct peregrine5_pc_usr_ctrl_disable_functions_st *get_val) {
    INIT_SRDS_ERR_CODE

    if(!get_val) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ESTM(get_val->dword = rdv_usr_ctrl_disable_steady_state_functions_dword());
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_usr_ctrl_disable_functions_st(get_val));
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta2_peregrine5_pc_get_usr_ctrl_disable_blind(srds_access_t *sa__, struct peregrine5_pc_usr_ctrl_disable_functions_st *get_val) {
    INIT_SRDS_ERR_CODE

    if(!get_val) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ESTM(get_val->dword = rdv_usr_ctrl_disable_blind_functions_dword());
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_usr_ctrl_disable_functions_st(get_val));
    return (ERR_CODE_NONE);
}

/********************************************************************************/
/* Helper print function for Serdes Register/Variable Dump using no buffer      */
/********************************************************************************/
static err_code_t _plp_aperta2_peregrine5_pc_reg_print_no_buf(srds_access_t *sa__, uint16_t start_addr, uint16_t valid_addr) {
    INIT_SRDS_ERR_CODE
    uint16_t offset = 0, rddata = 0;

    EFUN_PRINTF(("\n%04x ", start_addr));
    for (offset = 0; offset < 0x10; offset++) {
        if(valid_addr & (0x1 << offset)) {
            EFUN(plp_aperta2_peregrine5_pc_acc_rdt_reg(sa__, (uint16_t)(start_addr + offset), &rddata));
        }
        else {
            rddata = 0x0;
        }
        EFUN_PRINTF(("%04x ",rddata));
    }
    return (ERR_CODE_NONE);
}

/********************************************************************************/
/* Helper print function for Serdes Register/Variable Dump using a buffer       */
/********************************************************************************/
static err_code_t _plp_aperta2_peregrine5_pc_reg_print_with_buf(srds_access_t *sa__, uint16_t start_addr, uint16_t valid_addr, char reg_buffer[SRDS_DUMP_BUF_SIZE]) {
    INIT_SRDS_ERR_CODE
    int32_t count = 0;
    uint16_t offset = 0, rddata = 0;

    count += USR_SNPRINTF(reg_buffer + count, (size_t)(SRDS_DUMP_BUF_SIZE - count),"%04x ", start_addr);

    for (offset = 0; offset < 0x10; offset++) {
        if(valid_addr & (0x1 << offset)) {
            EFUN(plp_aperta2_peregrine5_pc_acc_rdt_reg(sa__, (uint16_t)(start_addr + offset), &rddata));
        }
        else {
            rddata = 0x0;
        }
        count += USR_SNPRINTF(reg_buffer + count, (size_t)(SRDS_DUMP_BUF_SIZE - count), "%04x ", rddata);
    }
    return (ERR_CODE_NONE);
}

/********************************************************************************/
/* Helper print function for Serdes Register/Variable Dump                      */
/********************************************************************************/
static err_code_t _plp_aperta2_peregrine5_pc_reg_print(srds_access_t *sa__, uint8_t reg_start_index, uint8_t reg_end_index, char reg_buffer[][SRDS_DUMP_BUF_SIZE], uint8_t *buf_index) {
    INIT_SRDS_ERR_CODE
    uint8_t reg_section;
    peregrine5_pc_reg_dump_arr_t *peregrine5_pc_reg_dump_arr = plp_aperta2_peregrine5_pc_get_reg_dump_arr();
    if(reg_buffer == NULL) {
        for(reg_section = reg_start_index; reg_section < reg_end_index; reg_section++) {
            EFUN(_plp_aperta2_peregrine5_pc_reg_print_no_buf(sa__, peregrine5_pc_reg_dump_arr[reg_section][0], peregrine5_pc_reg_dump_arr[reg_section][1]));
        }
    }
    else {
        for(reg_section = reg_start_index; reg_section < reg_end_index; reg_section++) {
            EFUN(_plp_aperta2_peregrine5_pc_reg_print_with_buf(sa__, peregrine5_pc_reg_dump_arr[reg_section][0], peregrine5_pc_reg_dump_arr[reg_section][1], reg_buffer[*buf_index]));
            (*buf_index)++;
        }
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_reg_dump(srds_access_t *sa__, char reg_buffer[][SRDS_DUMP_BUF_SIZE]) {
    INIT_SRDS_ERR_CODE
    EFUN(plp_aperta2_peregrine5_pc_reg_select_dump(sa__, SRDS_LANE_AND_CORE_REG, reg_buffer));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_reg_select_dump(srds_access_t *sa__, uint8_t reg_dump_control, char reg_buffer[][SRDS_DUMP_BUF_SIZE]) {
    INIT_SRDS_ERR_CODE
    srds_core_t core = plp_aperta2_peregrine5_pc_acc_get_core(sa__);
    uint8_t lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    uint8_t buf_index = 0;
    uint8_t pll_orig, pll_idx;
    uint8_t micro_orig, micro_idx, num_micros;

    if (reg_buffer == NULL) {
        if(reg_dump_control == SRDS_CORE_REG_ONLY) {
            EFUN_PRINTF(("\n****  SERDES REGISTER CORE %d DUMP    ****", core));
        }
        else {
            EFUN_PRINTF(("\n****  SERDES REGISTER CORE %d LANE %d DUMP    ****", core, lane));
        }
    }
    else {
        if(reg_dump_control == SRDS_CORE_REG_ONLY) {
            EFUN_PRINTF(("\n****  SERDES REGISTER CORE %d DECODED    ****\n", core));
        }
        else {
            EFUN_PRINTF(("\n****  SERDES REGISTER CORE %d LANE %d DECODED    ****\n", core, lane));
        }
    }

    if (reg_dump_control & SRDS_LANE_REG_ONLY) {
        /* Lane Registers */
        EFUN(_plp_aperta2_peregrine5_pc_reg_print(sa__, 0, PEREGRINE5_PC_REG_CORE_START_INDEX, reg_buffer, &buf_index));
    }
    if (reg_dump_control & SRDS_CORE_REG_ONLY) {
        /* Core Registers */
        EFUN(_plp_aperta2_peregrine5_pc_reg_print(sa__, PEREGRINE5_PC_REG_CORE_START_INDEX, PEREGRINE5_PC_REG_PLL_START_INDEX, reg_buffer, &buf_index));

        /* Core PLL Registers */
        ESTM(pll_orig = plp_aperta2_peregrine5_pc_acc_get_pll_idx(sa__));
        for (pll_idx = 0; pll_idx < NUM_PLLS; pll_idx++) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__,pll_idx));
            EFUN(_plp_aperta2_peregrine5_pc_reg_print(sa__, PEREGRINE5_PC_REG_PLL_START_INDEX, PEREGRINE5_PC_REG_UC_START_INDEX, reg_buffer, &buf_index));
        }
        EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__,pll_orig));

        /* Core Micro Registers */
        ESTM(micro_orig = plp_aperta2_peregrine5_pc_acc_get_micro_idx(sa__));
        EFUN(plp_aperta2_peregrine5_pc_get_micro_num_uc_cores(sa__, &num_micros));
        for(micro_idx = 0; micro_idx < num_micros; micro_idx++) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_micro_idx(sa__,micro_idx));
            EFUN(_plp_aperta2_peregrine5_pc_reg_print(sa__, PEREGRINE5_PC_REG_UC_START_INDEX, PEREGRINE5_PC_REG_DUMP_SECTIONS, reg_buffer, &buf_index));
        }
        EFUN(plp_aperta2_peregrine5_pc_acc_set_micro_idx(sa__,micro_orig));
    }

    if (reg_buffer != NULL) {
        reg_buffer[buf_index++][0] = 0xA;
        reg_buffer[buf_index][0] = 0;
    }

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_uc_core_var_blk_dump(srds_access_t *sa__,srds_core_t core, uint16_t data_buff_size,uint8_t *data_buffer) {
      INIT_SRDS_ERR_CODE

      srds_info_t * peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
      peregrine5_pc_INTERNAL_rdblk_ram_arg_t rdblk_state;
      peregrine5_pc_INTERNAL_ram_dump_state_t state;
      uint32_t core_var_ram_size = 0;
      EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));
      core_var_ram_size = (uint32_t)peregrine5_pc_info_ptr->core_var_ram_size;
      state.index = 0;
      state.line_start_index = 0;
      state.ram_idx = 0;
      state.count = 0;
      rdblk_state.mem_ptr = data_buffer;
      rdblk_state.dump_state_ptr = &state;
      if(data_buffer == NULL) {
      EFUN_PRINTF(("\n**** SERDES UC CORE %d RAM VARIABLE DUMP ****", core));
      } else {
      EFUN_PRINTF(("\n***** SERDES UC CORE %d RAM VARIABLE DECODED ****\n", core));
      }
      if(data_buffer != NULL) {
      if(data_buff_size != core_var_ram_size) {
         EFUN_PRINTF(("***ERROR: UC CORE VAR BLOCK DUMP BUFFER SIZE IS INSUFFICIENT\n"));
         return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
      }
      }
      EFUN(plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram(sa__,
                                                      peregrine5_pc_info_ptr->core_var_ram_base,
                                                      peregrine5_pc_info_ptr->core_var_ram_size,
                                                      0,
                                                      peregrine5_pc_info_ptr->core_var_ram_size,
                                                      &rdblk_state,
                                                      plp_aperta2_peregrine5_pc_INTERNAL_rdblk_ram_read_callback));


      return (ERR_CODE_NONE);

 }

err_code_t plp_aperta2_peregrine5_pc_uc_micro_var_blk_dump(srds_access_t *sa__,srds_core_t core,uint8_t lane,uint16_t data_buff_size,uint8_t *data_buffer) {

      INIT_SRDS_ERR_CODE
      uint16_t  micro_var_ram_size;
      peregrine5_pc_INTERNAL_rdblk_ram_arg_t rdblk_state;
      peregrine5_pc_INTERNAL_ram_dump_state_t state;
      uint32_t uc_addr_offset = 0;
      srds_info_t * peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
      EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));
      micro_var_ram_size = peregrine5_pc_info_ptr->micro_var_ram_size;

      state.index = 0;
      state.line_start_index = 0;
      state.ram_idx = 0;
      state.count = 0;
      rdblk_state.mem_ptr = data_buffer;
      rdblk_state.ram_size = micro_var_ram_size;
      rdblk_state.dump_state_ptr = &state;

      if(data_buffer != NULL) {
      if(data_buff_size != micro_var_ram_size) {
         EFUN_PRINTF(("***ERROR: UC CORE MICRO VAR BLOCK DUMP BUFFER SIZE IS INSUFFICIENT\n"));
         return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
      }
      }

      if(data_buffer == NULL) {
      EFUN_PRINTF(("\n**** SERDES UC CORE %d MICRO %d RAM VARIABLE DUMP ****", core,lane/2));
      } else {
      EFUN_PRINTF(("\n***** SERDES UC CORE %d MICRO %d RAM VARIABLE DECODED ****\n", core,lane/2));
      }


          lane = peregrine5_pc_acc_get_physical_lane(sa__);
          uc_addr_offset = _plp_aperta2_peregrine5_pc_INTERNAL_get_uc_addr_from_lane(sa__, 0,lane);

      EFUN(plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram(sa__,
                                                  uc_addr_offset,
                                                  peregrine5_pc_info_ptr->micro_var_ram_size,
                                                   0,
                                                  peregrine5_pc_info_ptr->micro_var_ram_size,
                                                  &rdblk_state,
                                                  plp_aperta2_peregrine5_pc_INTERNAL_rdblk_ram_read_callback));


      return (ERR_CODE_NONE);

}

err_code_t plp_aperta2_peregrine5_pc_uc_lane_static_var_blk_dump(srds_access_t *sa__, srds_core_t core, uint8_t lane, uint32_t data_buff_size,uint8_t *data_buffer) {
    INIT_SRDS_ERR_CODE
    uint8_t     rx_lock, uc_stopped = 0;
    uint32_t     lane_static_var_ram_size;
    srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    uint32_t lane_addr_offset;

    peregrine5_pc_INTERNAL_rdblk_ram_arg_t rdblk_state;
    peregrine5_pc_INTERNAL_ram_dump_state_t state;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));
    lane_static_var_ram_size = peregrine5_pc_info_ptr->lane_static_var_ram_size;

    EFUN(plp_aperta2_peregrine5_pc_pmd_lock_status(sa__, &rx_lock));

    {
        err_code_t err_code=ERR_CODE_NONE;
        uc_stopped = plp_aperta2_peregrine5_pc_INTERNAL_stop_micro(sa__,rx_lock,&err_code);
        if(err_code) USR_PRINTF(("Unable to stop microcontroller,  following data is suspect\n"));
    }
    state.index = 0;
    state.line_start_index = 0;
    state.ram_idx = 0;
    state.count = 0;
    rdblk_state.mem_ptr = data_buffer;
    rdblk_state.ram_size = lane_static_var_ram_size;
    rdblk_state.dump_state_ptr = &state;

    if(data_buffer != NULL){
        if(data_buff_size != lane_static_var_ram_size) {
            EFUN_PRINTF(("\n***ERROR: UC LANE STATIC VAR BLOCK DUMP BUFFER SIZE IS INSUFFICIENT"));
            return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
        }
    }
    if(data_buffer == NULL) {
        EFUN_PRINTF(("\n**** SERDES UC CORE %d LANE %d STATIC RAM VARIABLE DUMP ****", core,lane));
    } else {
        EFUN_PRINTF(("\n**** SERDES UC CORE %d LANE %d STATIC RAM VARIABLE DECODED****\n", core,lane));
    }


    lane = peregrine5_pc_acc_get_physical_lane(sa__);
    lane_addr_offset = _plp_aperta2_peregrine5_pc_INTERNAL_get_static_addr_from_lane(sa__,0,lane);

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram(sa__,
            lane_addr_offset,
            peregrine5_pc_info_ptr->lane_static_var_ram_size,
            0,
            peregrine5_pc_info_ptr->lane_static_var_ram_size,
            &rdblk_state,
            plp_aperta2_peregrine5_pc_INTERNAL_rdblk_ram_read_callback));

    if (rx_lock == 1) {
        if (!uc_stopped) {
            EFUN(plp_aperta2_peregrine5_pc_stop_rx_adaptation(sa__, 0));
        }
    } else {
        EFUN(plp_aperta2_peregrine5_pc_stop_rx_adaptation(sa__, 0));
    }

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_uc_lane_var_blk_dump(srds_access_t *sa__, srds_core_t core, uint8_t lane, uint32_t data_buff_size,uint8_t *data_buffer) {
    INIT_SRDS_ERR_CODE
    uint8_t     rx_lock, uc_stopped = 0;
    uint32_t    lane_var_ram_size;
    srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    peregrine5_pc_INTERNAL_rdblk_ram_arg_t rdblk_state;
    peregrine5_pc_INTERNAL_ram_dump_state_t state;
    uint32_t lane_addr_offset = 0;

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));
    lane_var_ram_size = (uint16_t)peregrine5_pc_info_ptr->lane_var_ram_size;

    EFUN(plp_aperta2_peregrine5_pc_pmd_lock_status(sa__, &rx_lock));

    {
        err_code_t err_code=ERR_CODE_NONE;
        uc_stopped = plp_aperta2_peregrine5_pc_INTERNAL_stop_micro(sa__,rx_lock,&err_code);
        if(err_code) USR_PRINTF(("Unable to stop microcontroller,  following data is suspect\n"));
    }

    state.index = 0;
    state.line_start_index = 0;
    state.count = 0;
    state.ram_idx = 0;
    rdblk_state.mem_ptr = data_buffer;
    rdblk_state.ram_size = lane_var_ram_size;
    rdblk_state.dump_state_ptr = &state;

    if(data_buffer != NULL) {
        if(data_buff_size != lane_var_ram_size) {
            EFUN_PRINTF(("\n***ERROR: UC LANE VAR BLOCK DUMP BUFFER SIZE IS INSUFFICIENT"));
            return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
        }
    }


    if(data_buffer == NULL) {
        EFUN_PRINTF(("\n**** SERDES UC CORE %d LANE %d RAM VARIABLE DUMP ****", core,lane));
    } else {
        EFUN_PRINTF(("\n***** SERDES UC CORE %d LANE %d RAM VARIABLE DECODED ****\n", core,lane));
    }


    lane = peregrine5_pc_acc_get_physical_lane(sa__);
    lane_addr_offset = _plp_aperta2_peregrine5_pc_INTERNAL_get_addr_from_lane(sa__, 0 ,lane);


    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram(sa__,
            lane_addr_offset,
            peregrine5_pc_info_ptr->lane_var_ram_size,
            0,
            peregrine5_pc_info_ptr->lane_var_ram_size,
            &rdblk_state,
            plp_aperta2_peregrine5_pc_INTERNAL_rdblk_ram_read_callback));

    if (rx_lock == 1) {
        if (!uc_stopped) {
            EFUN(plp_aperta2_peregrine5_pc_stop_rx_adaptation(sa__, 0));
        }
    } else {
        EFUN(plp_aperta2_peregrine5_pc_stop_rx_adaptation(sa__, 0));
    }

    return (ERR_CODE_NONE);
}

static void  _plp_aperta2_peregrine5_pc_print_common_block_header(srds_access_t *sa__, srds_core_t core, uint8_t *data_buffer);
static void  _plp_aperta2_peregrine5_pc_print_common_block_header(srds_access_t *sa__, srds_core_t core, uint8_t *data_buffer) {
    uint8_t lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    if(data_buffer == NULL) {
        EFUN_PRINTF(("\n**** SERDES COMMON BLOCK CORE %d LANE %d RAM VARIABLE DUMP ****", core, lane));
    }
    else {
        EFUN_PRINTF(("\n***** SERDES COMMON BLOCK CORE %d LANE %d RAM VARIABLE DECODED ****\n", core, lane));
    }
}

err_code_t plp_aperta2_peregrine5_pc_common_block_mem_ram_blk_dump(srds_access_t *sa__,srds_core_t core, uint32_t data_buff_size,uint8_t *data_buffer) {
    INIT_SRDS_ERR_CODE

    _plp_aperta2_peregrine5_pc_print_common_block_header(sa__, core, data_buffer);
    EFUN(plp_aperta2_peregrine5_pc_common_block_mem_ram_blk_dump_no_header(sa__, data_buff_size, data_buffer));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_common_block_mem_ram_blk_dump_no_header(srds_access_t *sa__, uint32_t data_buff_size,uint8_t *data_buffer) {
    INIT_SRDS_ERR_CODE

    srds_info_t * peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    peregrine5_pc_INTERNAL_rdblk_ram_arg_t rdblk_state;
    peregrine5_pc_INTERNAL_ram_dump_state_t state;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));
    state.index = 0;
    state.line_start_index = 0;
    state.ram_idx = 0;
    state.count = 0;
    rdblk_state.mem_ptr = data_buffer;
    rdblk_state.ram_size = peregrine5_pc_info_ptr->common_block_mem_ram_size;
    rdblk_state.dump_state_ptr = &state;
    if(data_buffer != NULL && data_buff_size != peregrine5_pc_info_ptr->common_block_mem_ram_size) {
        EFUN_PRINTF(("***ERROR: COMMON BLOCK MEM RAM DUMP BUFFER SIZE IS INSUFFICIENT\n"));
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram(sa__,
                                                    peregrine5_pc_info_ptr->common_block_mem_ram_base,
                                                    peregrine5_pc_info_ptr->common_block_mem_ram_size,
                                                    0,
                                                    peregrine5_pc_info_ptr->common_block_mem_ram_size,
                                                    &rdblk_state,
                                                    plp_aperta2_peregrine5_pc_INTERNAL_rdblk_ram_read_callback));

    return (ERR_CODE_NONE);
}
/*
static void _peregrine5_pc_print_debug_block_mem_ram_blk_dump_footer(srds_access_t *sa__);
static void _peregrine5_pc_print_debug_block_mem_ram_blk_dump_footer(srds_access_t *sa__) {
    uint8_t is_big_endian = plp_aperta2_peregrine5_pc_INTERNAL_is_big_endian();
    srds_info_t * peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    EFUN_PRINTF(("%02x ", (is_big_endian ? (peregrine5_pc_info_ptr->lane_count >> 8) : (uint8_t)(peregrine5_pc_info_ptr->lane_count & 0xFF))));
    EFUN_PRINTF(("%02x ", (is_big_endian ? (uint8_t)(peregrine5_pc_info_ptr->lane_count & 0xFF) : (peregrine5_pc_info_ptr->lane_count >> 8))));
    EFUN_PRINTF(("%02x ", (is_big_endian ? (peregrine5_pc_info_ptr->grp_ram_size >> 8) : (uint8_t)(peregrine5_pc_info_ptr->grp_ram_size & 0xFF))));
    EFUN_PRINTF(("%02x ", (is_big_endian ? (uint8_t)(peregrine5_pc_info_ptr->grp_ram_size & 0xFF) : (peregrine5_pc_info_ptr->grp_ram_size >> 8))));
}
*/
err_code_t plp_aperta2_peregrine5_pc_debug_block_mem_ram_blk_dump(srds_access_t *sa__,srds_core_t core, uint8_t lane, uint32_t data_buff_size,uint8_t *data_buffer) {
    INIT_SRDS_ERR_CODE

    uint8_t     rx_lock, uc_stopped = 0;
    srds_info_t * peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    peregrine5_pc_INTERNAL_rdblk_ram_arg_t rdblk_state;
    peregrine5_pc_INTERNAL_ram_dump_state_t state;
    uint32_t debug_mem_offset = 0;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));

    EFUN(plp_aperta2_peregrine5_pc_pmd_lock_status(sa__, &rx_lock));

    {
        err_code_t err_code=ERR_CODE_NONE;
        uc_stopped = plp_aperta2_peregrine5_pc_INTERNAL_stop_micro(sa__,rx_lock,&err_code);
        if(err_code) USR_PRINTF(("Unable to stop microcontroller,  following data is suspect\n"));
    }

    state.index = 0;
    state.line_start_index = 0;
    state.ram_idx = 0;
    state.count = 0;
    rdblk_state.mem_ptr = data_buffer;
    rdblk_state.ram_size = peregrine5_pc_info_ptr->debug_block_mem_ram_size;
    rdblk_state.dump_state_ptr = &state;
    if(data_buffer == NULL) {
        EFUN_PRINTF(("\n**** SERDES DEBUG BLOCK CORE %d LANE %d RAM VARIABLE DUMP ****", core, lane));
    }
    else {
        EFUN_PRINTF(("\n***** SERDES DEBUG BLOCK CORE %d LANE %d RAM VARIABLE DECODED ****\n", core, lane));
    }
    if(data_buffer != NULL && data_buff_size != peregrine5_pc_info_ptr->debug_block_mem_ram_size) {
        EFUN_PRINTF(("***ERROR: DEBUG BLOCK MEM RAM DUMP BUFFER SIZE IS INSUFFICIENT\n"));
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    lane = peregrine5_pc_acc_get_physical_lane(sa__);
    debug_mem_offset = plp_aperta2_peregrine5_pc_INTERNAL_get_debug_mem_addr_from_lane(sa__,lane);

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram(sa__,
                                                    debug_mem_offset,
                                                    peregrine5_pc_info_ptr->debug_block_mem_ram_size,
                                                    0,
                                                    peregrine5_pc_info_ptr->debug_block_mem_ram_size,
                                                    &rdblk_state,
                                                    plp_aperta2_peregrine5_pc_INTERNAL_rdblk_ram_read_callback));
/*
    if(data_buffer == NULL) {
        _peregrine5_pc_print_debug_block_mem_ram_blk_dump_footer(sa__);
    }
#if defined(SERDES_EVAL)
    else if (is_tee_external_log_valid()){
            tee_to_external_log_only();
            _peregrine5_pc_print_debug_block_mem_ram_blk_dump_footer(sa__);
            tee_to_main_log_only();
        }
#endif
*/
    if ((rx_lock == 1 && !uc_stopped) || (rx_lock == 0)) {
        EFUN(plp_aperta2_peregrine5_pc_stop_rx_adaptation(sa__, 0));
    }

    return (ERR_CODE_NONE);
}

static err_code_t _plp_aperta2_peregrine5_pc_print_indirect_access_blk_dump_header(srds_access_t *sa__, uint16_t *byte_count);
static err_code_t _plp_aperta2_peregrine5_pc_print_indirect_access_blk_dump_header(srds_access_t *sa__, uint16_t *byte_count) {
    srds_info_t * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    uint8_t is_big_endian;
    
    SRDS_INFO_PTR_NULL_CHECK;
    
    is_big_endian = peregrine5_pc_info_ptr->is_big_endian;
    EFUN_PRINTF(("\n%04x ", *byte_count));
    EFUN_PRINTF(("%02x ", (is_big_endian ? (SRDS_NUM_INDIRECT_ACCESS_BLKS >> 8) : (uint8_t)(SRDS_NUM_INDIRECT_ACCESS_BLKS & 0xFF))));
    EFUN_PRINTF(("%02x ", (is_big_endian ? (uint8_t)(SRDS_NUM_INDIRECT_ACCESS_BLKS & 0xFF) : (SRDS_NUM_INDIRECT_ACCESS_BLKS >> 8))));
    *byte_count += 2;
    EFUN_PRINTF(("%02x ", (is_big_endian ? (SRDS_INDIRECT_ACCESS_BLK1_SIZE_BYTES >> 8) : (uint8_t)(SRDS_INDIRECT_ACCESS_BLK1_SIZE_BYTES & 0xFF))));
    EFUN_PRINTF(("%02x ", (is_big_endian ? (uint8_t)(SRDS_INDIRECT_ACCESS_BLK1_SIZE_BYTES & 0xFF) : (SRDS_INDIRECT_ACCESS_BLK1_SIZE_BYTES >> 8))));
    *byte_count += 2;
    EFUN_PRINTF(("%02x ", (is_big_endian ? (SRDS_INDIRECT_ACCESS_BLK2_SIZE_BYTES >> 8) : (uint8_t)(SRDS_INDIRECT_ACCESS_BLK2_SIZE_BYTES & 0xFF))));
    EFUN_PRINTF(("%02x ", (is_big_endian ? (uint8_t)(SRDS_INDIRECT_ACCESS_BLK2_SIZE_BYTES & 0xFF) : (SRDS_INDIRECT_ACCESS_BLK2_SIZE_BYTES >> 8))));
    *byte_count += 2;
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_indirect_access_blk_dump(srds_access_t *sa__, srds_core_t core, uint8_t lane, uint32_t data_buff_size, uint8_t *data_buffer) {
    INIT_SRDS_ERR_CODE
    uint16_t byte_count = 0;
    UNUSED(data_buff_size);

    if(data_buffer == NULL) {
        EFUN_PRINTF(("\n**** SERDES INDIRECT ACCESS CORE %d LANE %d DATA DUMP ****", core, lane));
        EFUN(_plp_aperta2_peregrine5_pc_print_indirect_access_blk_dump_header(sa__, &byte_count));
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_ffe_offsets(sa__, NULL, SRDS_NUM_ADCS, &byte_count));
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_ffe_taps(sa__, NULL, SRDS_NUM_FFE_ILVS * SRDS_NUM_FFE_TAPS, &byte_count));
    }
    else {
        EFUN_PRINTF(("\n**** SERDES INDIRECT ACCESS CORE %d LANE %d DATA DECODED ****", core, lane));
    }
    return ERR_CODE_NONE;
}

/***************************************/
/*  API Function to Read Event Logger  */
/***************************************/

err_code_t plp_aperta2_peregrine5_pc_read_micro_event_log(srds_access_t *sa__, uint8_t micro_idx) {
    INIT_SRDS_ERR_CODE
    peregrine5_pc_INTERNAL_event_log_dump_state_t state;
    srds_info_t * peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    uint8_t micro_num = 0;
    uint8_t micro_start = 0;
    uint8_t micro_end = 0;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));

    if (micro_idx < peregrine5_pc_info_ptr->micro_count) {
        micro_start = micro_idx;
        micro_end = micro_start + 1;
    }
    else {
        micro_start = 0;
        micro_end = peregrine5_pc_info_ptr->micro_count;
    }


    for (micro_num = micro_start; micro_num < micro_end; ++micro_num)
    {
        state.index = 0;
        state.line_start_index = 0;
        state.zero_cnt = 0;

        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_read_event_log_with_callback(sa__, micro_num, 0, &state, plp_aperta2_peregrine5_pc_INTERNAL_event_log_dump_callback));
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_event_log_dump_callback(sa__, &state, 0, 0));
    }
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_read_event_log(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE

    /* Print CORE for the standalone diag decoder usage with Version 2 event code in customer logs */
    EFUN_PRINTF(("\n**** Starting Event Log Decode for CORE: %d ****\n", plp_aperta2_peregrine5_pc_acc_get_core(sa__)));

    EFUN(plp_aperta2_peregrine5_pc_read_micro_event_log(sa__, 0xff)); /* 0xff: all micros */

    return(ERR_CODE_NONE);
}

/*******************************/
/*  MSE per Lvl Diag routines  */
/*******************************/

/*! Callback function argument used for mse_per_lvl APIs
 *
 */
typedef struct {
  uint16_t  word_count;
  uint32_t *buffer_ptr;
} mse_per_lvl_callback_arg_t;

/** This function is used as part of the micro data reading process it stuffs the 16bit data into uint32_t 
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[in,out] arg is a mse_per_lvl_callback_arg_t pointer for holding state.
 * @param[in] byte_count is the number of bytes to dump.
 *        -# This function is called repeatedly with a byte count of 2 with U16 values to dump.
 *        -# This function may then be called with a byte count of 1 if the event log has an odd number of bytes.
 *        -# Finally, this function must be called with a byte count of 0 to finish up.
 * @param[in] data is the event log data to dump.
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors).
*/
static err_code_t _plp_aperta2_peregrine5_pc_get_diag_memory_uint32_callback(srds_access_t *sa__, void *arg, uint8_t byte_count, uint16_t data);

/** Get Event Log from uC, and call callback for every two bytes.
 * @param[in] sa__ is an opaque state vector passed through to device access functions.
 * @param[out] vec is a pointer to array of cnt - uint32_t
 * @param[in] cnt is number of elements in the elements in array
 * @param[out] status is diag status word returned from uc_cmd
 * @param[in] callback is called with all of the data read, two bytes at a time.
 *                  The last call of callback may have one byte; in that case, the upper byte is undefined.
 *                  The call is in the form:  callback(arg, byte_count, data).
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors).
 */
static err_code_t _plp_aperta2_peregrine5_pc_read_mse_per_lvl_data(srds_access_t *sa__, uint32_t *vec, uint16_t cnt, uint16_t *status, err_code_t (*callback)(srds_access_t *, void *, uint8_t, uint16_t));

static err_code_t _plp_aperta2_peregrine5_pc_read_mse_per_lvl_data(srds_access_t *sa__, uint32_t *vec, uint16_t cnt, uint16_t *status, err_code_t (*callback)(srds_access_t *, void *, uint8_t, uint16_t))
{
    INIT_SRDS_ERR_CODE
    peregrine5_pc_read_diag_data_st   read_diag_state;
    srds_info_t const *const   peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    uint16_t                   diag_rd_ptr=0;
    mse_per_lvl_callback_arg_t arg;

    SRDS_INFO_PTR_NULL_CHECK;

    arg.buffer_ptr              = vec;
    arg.word_count = 0;
    read_diag_state.status      = status;
    read_diag_state.arg         = &arg;
    read_diag_state.callback    = callback;
    read_diag_state.diag_rd_ptr = &diag_rd_ptr;
    read_diag_state.byte_count  = cnt*sizeof(uint32_t);

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_read_diag_data(sa__, peregrine5_pc_info_ptr, &read_diag_state));
    return(ERR_CODE_NONE);
}

static err_code_t _plp_aperta2_peregrine5_pc_get_diag_memory_uint32_callback(srds_access_t *sa__, void *arg, uint8_t byte_count, uint16_t data)
{
/*     srds_info_t * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    uint8_t is_big_endian = peregrine5_pc_info_ptr->is_big_endian;
 */
    mse_per_lvl_callback_arg_t *const cast_arg    = (mse_per_lvl_callback_arg_t *)arg;
    UNUSED(sa__);

    if (byte_count != 2) {
        return (ERR_CODE_DATA_NOTAVAIL);
    }
    /* TODO : handle endian ness?  */
    *(cast_arg->buffer_ptr) = (*(cast_arg->buffer_ptr) >> 16) | (data << 16);
    if (cast_arg->word_count % 2 == 1) {
        cast_arg->buffer_ptr++;
    }

    ++(cast_arg->word_count);
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_measure_mse_per_lvl(srds_access_t *sa__, uint32_t *vec)
{
    INIT_SRDS_ERR_CODE
    uint16_t status = 0;
    err_code_t error = ERR_CODE_NONE;
    uint8_t pmd_lock = 0, pmd_lock_change = 0,stopped = 0;
    enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;

    if(vec == NULL) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_pmd_lock_status(sa__,&pmd_lock, &pmd_lock_change));

    if(pmd_lock==0) {
        ESTM(stopped = rdv_usr_dbstopped());
        if(stopped == 0) {
            return(ERR_CODE_UC_NOT_STOPPED);
        }
    }
    
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));

    if(pam_mode==PAM4_NR) {
            error = plp_aperta2_peregrine5_pc_pmd_uc_cmd(sa__, CMD_MEASURE_MSE_PER_LVL, 0, GRACEFUL_STOP_TIME);
        if (error == ERR_CODE_NONE) {
            EFUN(_plp_aperta2_peregrine5_pc_read_mse_per_lvl_data(sa__, vec, 4, &status, _plp_aperta2_peregrine5_pc_get_diag_memory_uint32_callback));
            return (ERR_CODE_NONE);
        } 
    } else {
        return(ERR_CODE_INVALID_MODE);
    }
    return(ERR_CODE_UC_CMD_RETURN_ERROR);
}

err_code_t plp_aperta2_peregrine5_pc_display_snr_per_lvl(srds_access_t *sa__)
{
    INIT_SRDS_ERR_CODE
    uint32_t mse[4] = {0};
    int8_t i=0;

    USR_PRINTF(("Core = %d Lane = %d : ",plp_aperta2_peregrine5_pc_acc_get_core(sa__),plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));

    srds_err_code = plp_aperta2_peregrine5_pc_measure_mse_per_lvl(sa__, mse);
    if(srds_err_code == ERR_CODE_NONE) {
        for(i = 3;i >= 0; i--) {
#ifdef SERDES_API_FLOATING_POINT
            USR_PRINTF(("MSE:SNR[%d] = %d : %.4f dB ", i,mse[i],SERDES_MSE_TO_SNR(mse[i])));
#else
            USR_PRINTF(("MSE[%d] = %d ", i,mse[i]));
#endif
        }
    } else if(srds_err_code == ERR_CODE_INVALID_MODE) {
        USR_PRINTF(("Not valid Mode %s",API_FUNCTION_NAME));
        srds_err_code = ERR_CODE_NONE;
    }
    USR_PRINTF(("\n"));
    return (peregrine5_pc_error(sa__, srds_err_code));
}
/**********************************************/
/*  Loopback and Ultra-Low Latency Functions  */
/**********************************************/

/* Enable/Disable internal Loopback */
err_code_t plp_aperta2_peregrine5_pc_internal_lpbk(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    peregrine5_pc_osr_mode_st osr_mode;
    uint8_t reset_state;
    err_code_t lane_map_error = plp_aperta2_peregrine5_pc_INTERNAL_lpbk_lane_map_check(sa__);
    if(lane_map_error == ERR_CODE_RX_TX_LANE_MISMATCH) {
        EFUN_PRINTF(("WARNING: TX and RX are not mapped to the same physical lane. ILB not performed.\n"));
        return ERR_CODE_NONE;
    }
    else if(lane_map_error != ERR_CODE_NONE) {
        return lane_map_error;
    }
    {
        if ((enable == 1) || (enable > 3)) {
            USR_PRINTF(("ERROR: bad input argument value, 'enable' can not be 1 or greater than 3\n"));
            return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
        }
    }
    {
        uint8_t use_rx_osr_pin = 0;
        EFUN(plp_aperta2_peregrine5_pc_get_use_rx_osr_mode_pins_only(sa__, &use_rx_osr_pin));
        if(use_rx_osr_pin) {
            ESTM(osr_mode.rx = rd_rx_osr_mode_pin());
        }
        else {
            enum peregrine5_pc_osr_mode_enum rx_osr_mode = PEREGRINE5_PC_OSR_UNINITIALIZED;
            EFUN(plp_aperta2_peregrine5_pc_get_rx_osr_mode(sa__, &rx_osr_mode));
            osr_mode.rx = rx_osr_mode;
        }
    }
    if ((osr_mode.rx != PEREGRINE5_PC_OSX1) && (osr_mode.rx != PEREGRINE5_PC_OSX2)) {
        EFUN_PRINTF(("Error: Internal loopback only supported in OSx1 and OSx2\n"));
        return(peregrine5_pc_error(sa__, ERR_CODE_CONFLICTING_PARAMETERS));
    }

    {
        enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;
            EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
            if (pam_mode == PAM4_ER) {
                EFUN_PRINTF(("Error: Internal loopback not supported in PAM4 ER mode\n"));
                return(peregrine5_pc_error(sa__, ERR_CODE_INVALID_RX_PAM_MODE));
            }
    }

    if(enable) {
        ESTM(reset_state = rd_rx_lane_dp_reset_state());
        if(reset_state < 7) {
            EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_internal_lpbk(..) called without ln_dp_s_rstb=0 Lane=%d reset_state=%d\n",plp_aperta2_peregrine5_pc_acc_get_lane(sa__),reset_state));
            return (peregrine5_pc_error(sa__, ERR_CODE_LANE_DP_NOT_RESET));
        }
        EFUN_PRINTF((" Core: %d, Lane %d: Starting internal loopback mode...\n", plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));
        /* Apply TX/RX settings */
        EFUN(plp_aperta2_peregrine5_pc_apply_txfir_cfg(sa__, PEREGRINE5_PC_PAM4_6TAP, 0, 0, 0, 170, 0, 0));
        EFUN(wr_lane_uc_config_force_cdr_mode(1));                            /* Force OS-PD */
        EFUN(wr_lane_uc_config_rx_low_power(1));                              /* RX low power */
        EFUN(wr_ilb_en(enable));
        EFUN(wr_signal_detect_frc_val(1)); EFUN(wr_signal_detect_frc(1));     /* Force SD on */
    }
    else {
        EFUN(wr_signal_detect_frc(0));                                           /* Disable force sigdet */
        EFUN(wr_ilb_en(0));                                                      /* Disable ILB mode */
        EFUN(plp_aperta2_peregrine5_pc_reset_lane_to_default(sa__));                                /* Toggle lane soft reset */

        EFUN_PRINTF(("WARNING: Core: %d, Lane %d: Exiting internal loopback mode by toggling ln_s_rstb, please re-configure the lane!\n", plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));
    }
    return (ERR_CODE_NONE);
}

/* Enable/Disable Digital Loopback */
err_code_t plp_aperta2_peregrine5_pc_dig_lpbk(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    srds_info_t * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));
    if(enable) {
        err_code_t lane_map_error = plp_aperta2_peregrine5_pc_INTERNAL_lpbk_lane_map_check(sa__);
        if(lane_map_error == ERR_CODE_RX_TX_LANE_MISMATCH) {
            EFUN_PRINTF(("WARNING: TX and RX are not mapped to the same physical lane. Digital loopback not performed.\n"));
            return ERR_CODE_NONE;
        }
        else if(lane_map_error != ERR_CODE_NONE) {
            return lane_map_error;
        }
        {
        uint8_t link_training_enable = 0;
        ESTM(link_training_enable = rd_linktrn_ieee_training_enable());
        if(link_training_enable) {
                EFUN_PRINTF(("WARNING: Core: %d, Lane %d: Link Training mode is on in digital loopback. Digital loopback not performed.\n", plp_aperta2_peregrine5_pc_acc_get_core(sa__), plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));
                return ERR_CODE_NONE;
        }
        }
    }
    /* setting/clearing prbs_chk_en_auto_mode while coming out of/going in to dig lpbk */
    EFUN(wr_prbs_chk_en_auto_mode(!enable));
    EFUN(wr_dig_lpbk_en(enable));                         /* 0 = disabled, 1 = enabled */
    if(peregrine5_pc_info_ptr->silicon_version <= PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_2) {
        EFUN(wr_rx_restart_pmd_hold(enable)); /* Need to force micro to leave active restart state */
    }

  return (ERR_CODE_NONE);
}


/**********************************/
/*  TX_PI Jitter Generation APIs  */
/**********************************/

/* TX_PI Sinusoidal or Spread-Spectrum (SSC) Jitter Generation  */
err_code_t plp_aperta2_peregrine5_pc_tx_pi_jitt_gen(srds_access_t *sa__, uint8_t enable, int16_t freq_override_val, enum plp_aperta2_srds_tx_pi_freq_jit_gen_enum jit_type, uint8_t tx_pi_jit_freq_idx, uint8_t tx_pi_jit_amp) {
    INIT_SRDS_ERR_CODE
    /* Added a limiting for the jitter amplitude index, per freq_idx */
    uint8_t max_amp_idx_r20_os1[] = {37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 42, 48, 56, 33, 39, 47, 58, 37, 48, 33, 47, 37, 33, 37, 37};

    /* Irrespective of the osr_mode, txpi runs @ os1. Thus the max amp idx values remain the same. */
    if (jit_type == TX_PI_SJ) {
        if (tx_pi_jit_amp > max_amp_idx_r20_os1[tx_pi_jit_freq_idx]) {
            tx_pi_jit_amp = max_amp_idx_r20_os1[tx_pi_jit_freq_idx];
        }
    }

    EFUN(plp_aperta2_peregrine5_pc_tx_pi_freq_override(sa__, enable, freq_override_val));

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

err_code_t plp_aperta2_peregrine5_pc_isolate_ctrl_pins(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    uint8_t lane, lane_orig, num_lanes;
    uint8_t pll, pll_orig;
    ESTM(pll_orig = plp_aperta2_peregrine5_pc_acc_get_pll_idx(sa__));

    for(pll = 0; pll < NUM_PLLS; pll++) {
        EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, pll));
        EFUN(plp_aperta2_peregrine5_pc_isolate_core_ctrl_pins(sa__, enable));
    }
    EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, pll_orig));

    ESTM(lane_orig = plp_aperta2_peregrine5_pc_acc_get_lane(sa__));
    /* read num lanes per core directly from register */
    ESTM(num_lanes = rdc_revid_multiplicity());
    for(lane = 0; lane < num_lanes; lane++) {
        EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane));
        EFUN(plp_aperta2_peregrine5_pc_isolate_lane_ctrl_pins(sa__, enable));
    }
    EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane_orig));

  return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_isolate_lane_ctrl_pins(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE

    EFUN(plp_aperta2_peregrine5_pc_isolate_lane_ctrl_tx_pins(sa__, enable));
    EFUN(plp_aperta2_peregrine5_pc_isolate_lane_ctrl_rx_pins(sa__, enable));
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_isolate_lane_ctrl_tx_pins(srds_access_t *sa__, uint8_t enable){

    INIT_SRDS_ERR_CODE

    if (enable) {
        EFUN(wr_pmd_ln_tx_h_pwrdn_pkill(0x1));
        EFUN(wr_pmd_ln_dp_h_rstb_pkill(0x1));
        EFUN(wr_pmd_ln_h_rstb_pkill(0x1));
        EFUN(wr_pmd_tx_disable_pkill(0x1));
    }
    else {
        EFUN(wr_pmd_ln_tx_h_pwrdn_pkill(0x0));
        EFUN(wr_pmd_ln_dp_h_rstb_pkill(0x0));
        EFUN(wr_pmd_ln_h_rstb_pkill(0x0));
        EFUN(wr_pmd_tx_disable_pkill(0x0));
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_isolate_lane_ctrl_rx_pins(srds_access_t *sa__, uint8_t enable){

    INIT_SRDS_ERR_CODE

    if (enable) {
      EFUN(wr_pmd_ln_rx_h_pwrdn_pkill(0x1));
      EFUN(wr_pmd_ln_dp_h_rstb_pkill(0x1));
      EFUN(wr_pmd_ln_h_rstb_pkill(0x1));

    }
    else {
      EFUN(wr_pmd_ln_rx_h_pwrdn_pkill(0x0));
      EFUN(wr_pmd_ln_dp_h_rstb_pkill(0x0));
      EFUN(wr_pmd_ln_h_rstb_pkill(0x0));

    }
    return (ERR_CODE_NONE);
}


err_code_t plp_aperta2_peregrine5_pc_isolate_core_ctrl_pins(srds_access_t *sa__, uint8_t enable) {
  INIT_SRDS_ERR_CODE

  if (enable) {
    EFUN(wrc_pmd_core_dp_h_rstb_pkill(0x1));
  }
  else {
    EFUN(wrc_pmd_core_dp_h_rstb_pkill(0x0));
  }
  return (ERR_CODE_NONE);
}




#endif /* ! SMALL_FOOTPRINT */

/*
 * for backtrace() support
 */


void plp_aperta2_peregrine5_pc_INTERNAL_print_triage_info(srds_access_t *sa__, err_code_t err_code, uint8_t print_header, uint8_t print_data, uint16_t line)
{
    /*  Note: No EFUNs or ESTMs should be used in this function as this print routine is called by _error() handler. */
#if defined(SMALL_FOOTPRINT)
    return;
#else
    peregrine5_pc_triage_info info;
    INIT_SRDS_ERR_CODE
    uint16_t   ucode_version_major;
    uint8_t    ucode_version_minor, error_seen = 0;

    if (ERR_CODE_SRDS_REG_ACCESS_FAIL == err_code) {  /* Early return to prevent error handling recursion on Access Errors! */
        return;
    }

    USR_MEMSET(&info, 0, sizeof(struct peregrine5_pc_triage_info_st));
    info.error = err_code;
    info.line = line;

    if (print_header) {
        USR_PRINTF(("Triage Info Below:\n"));
        if ((err_code == ERR_CODE_UC_CMD_POLLING_TIMEOUT) || (err_code == ERR_CODE_UC_NOT_STOPPED)) {
            USR_PRINTF(("Lane,       Core,  API_VER, UCODE_VER, micro_stop_status, exception(sw,hw), stack_ovflw, cmd_info, pmd_lock, sigdet, dsc_one_hot(0,1), Error\n"));
        } else {
            USR_PRINTF(("Lane,       Core,  API_VER, UCODE_VER, Error\n"));
        }
    }

    if (plp_aperta2_peregrine5_pc_version(sa__, &info.api_ver)) {  /* Unable to read api version */
        info.api_ver = 0xFFFFFFFF;
        error_seen = 1;
    }
    CHECK_ERR(ucode_version_major = rdcv_common_ucode_version());
    CHECK_ERR(ucode_version_minor = rdcv_common_ucode_minor_version());
    info.ucode_ver = (uint32_t)((ucode_version_major << 8) | ucode_version_minor);
    CHECK_ERR(info.stop_status = rdv_usr_sts_micro_stopped());

    /* Collect exception and overflow information */
    CHECK_ERR(info.stack_overflow   = rdc_micro_status_stack_overflowed());
    CHECK_ERR(info.overflow_lane_id = rdc_micro_status_stack_overflowed_laneID());
    CHECK_ERR(info.sw_exception     = rdc_micro_status_sw_exception_occurred());
    CHECK_ERR(info.hw_exception     = rdc_micro_status_hw_exception_occurred());
    if(plp_aperta2_peregrine5_pc_INTERNAL_sigdet_status(sa__, &info.sig_det, &info.sig_det_chg)) {
        error_seen = 1;
    }
    if (plp_aperta2_peregrine5_pc_pmd_lock_status(sa__, &info.pmd_lock)) {
        error_seen = 1;
    }

    CHECK_ERR(info.dsc_one_hot[0] = rd_dsc_state_one_hot());
    CHECK_ERR(info.dsc_one_hot[1] = rd_dsc_state_one_hot());
    CHECK_ERR(info.cmd_info = reg_rd_DSC_MISC_DSC_UC_CTRL());
    info.core = plp_aperta2_peregrine5_pc_acc_get_core(sa__);
    info.lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    if (print_data) {
        if ((err_code == ERR_CODE_UC_CMD_POLLING_TIMEOUT) || (err_code == ERR_CODE_UC_NOT_STOPPED)) {
            USR_PRINTF(("%4d, %10d,  %X_%X,    %X_%X, %17d, %7d,%d       , %11d,   0x%04x, %8d, %6d,    0x%x,0x%x   , %s\n",
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
                        plp_aperta2_peregrine5_pc_INTERNAL_e2s_err_code(info.error) ));
        } else {
            USR_PRINTF(("%4d, %10d,  %X_%X,     %X_%X, %s\n",
                        info.lane,
                        info.core,
                        info.api_ver>>8,
                        info.api_ver & 0xFF,
                        info.ucode_ver >> 8,
                        info.ucode_ver & 0xFF,
                        plp_aperta2_peregrine5_pc_INTERNAL_e2s_err_code(info.error) ));
        }
    }
    if (error_seen) {
        USR_PRINTF(("WARNING: There were some errors seen while collecting triage info and so the debug data above may not be all accurate\n"));
    }
    return;
#endif /* SMALL_FOOTPRINT */
}


