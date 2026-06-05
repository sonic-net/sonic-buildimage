/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/***********************************************************************************
 ***********************************************************************************
 *  File Name     :  peregrine5_pc_internal.c                                         *
 *  Created On    :  13/02/2014                                                    *
 *  Created By    :  Justin Gaither                                                *
 *  Description   :  Internal APIs for Serdes IPs                                  *
 *                                                                                 *
 ***********************************************************************************
 ***********************************************************************************/

#ifdef NON_SDK
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#endif

#include <phymod/phymod_system.h>
#include "peregrine5_pc_internal.h"
#include "peregrine5_pc_internal_error.h"
#include "peregrine5_pc_access.h"
#include "peregrine5_pc_common.h"
#include "peregrine5_pc_config.h"
#include "peregrine5_pc_functions.h"
#include "peregrine5_pc_select_defns.h"
#include "peregrine5_pc_debug_functions.h"
#include "peregrine5_pc_decode_print.h"
#include "peregrine5_pc_diag.h"


/*! @file
 *
 */



#ifndef SMALL_FOOTPRINT
#ifndef UINT16_MAX
#define UINT16_MAX 0xFFFF
#endif
uint32_t plp_aperta2_peregrine5_pc_INTERNAL_mult_with_overflow_check(uint32_t a, uint32_t b, uint8_t *of) {
    uint16_t c,d;
    uint32_t r,s;
    if (a > b) return plp_aperta2_peregrine5_pc_INTERNAL_mult_with_overflow_check(b, a, of);
    *of = 0;
    c = (uint16_t)(b >> 16);
    d = UINT16_MAX & b;
    r = a * c;
    s = a * d;
    if (r > UINT16_MAX) *of = 1;
    r <<= 16;
    return (s + r);
}
#endif /* SMALL_FOOTPRINT */

#if !defined(SERDES_EXTERNAL_INFO_TABLE_EN)
static srds_info_t plp_aperta2_peregrine5_pc_info;
#endif


srds_info_t *plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr(srds_access_t *sa__) {
#if defined(SERDES_EXTERNAL_INFO_TABLE_EN)
    return peregrine5_pc_acc_get_info_table_address(sa__);
#else
    UNUSED(sa__);
    return &plp_aperta2_peregrine5_pc_info;
#endif
}

srds_info_t *plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(srds_access_t *sa__) {
    err_code_t err_code = ERR_CODE_NONE;
    srds_info_t * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr(sa__);
    if (peregrine5_pc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        err_code = plp_aperta2_peregrine5_pc_init_peregrine5_pc_info(sa__);
    }
    if (err_code != ERR_CODE_NONE) {
        EFUN_PRINTF(("ERROR: Serdes Info pointer not initialized correctly\n"));
        return (srds_info_t *)NULL;
    }
    return peregrine5_pc_info_ptr;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(srds_access_t *sa__, srds_info_t const *peregrine5_pc_info_ptr) {
    INIT_SRDS_ERR_CODE
    uint16_t ucode_version_major;
    uint8_t ucode_version_minor;
    uint32_t ucode_version;

    if (peregrine5_pc_info_ptr == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    ESTM(ucode_version_major = rdcv_common_ucode_version());
    ESTM(ucode_version_minor = rdcv_common_ucode_minor_version());
    ucode_version = (uint32_t)((ucode_version_major << 8) | ucode_version_minor);

    if (ucode_version == peregrine5_pc_info_ptr->ucode_version) {
        return(ERR_CODE_NONE);
    } else {
        EFUN_PRINTF(("ERROR:  ucode version of the current thread not matching with stored plp_aperta2_peregrine5_pc_info->ucode_version, Expected 0x%08X, but received 0x%08X.\n",
                    peregrine5_pc_info_ptr->ucode_version, ucode_version));
        return(ERR_CODE_UCODE_VERIFY_FAIL);
    }
}

/* This function is getting deprecated.
   Please use plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check and plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info instead */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_verify_peregrine5_pc_info(srds_access_t *sa__, srds_info_t const *peregrine5_pc_info_ptr) {
    err_code_t err_code = ERR_CODE_NONE;

    if (peregrine5_pc_info_ptr == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    if (peregrine5_pc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        EFUN_PRINTF(("ERROR:  Mismatch in plp_aperta2_peregrine5_pc_info signature.  Expected 0x%08X, but received 0x%08X.\n",
                    SRDS_INFO_SIGNATURE, peregrine5_pc_info_ptr->signature));
        return (peregrine5_pc_error(sa__, ERR_CODE_INFO_TABLE_ERROR));
    }
    err_code = plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr);

    if (err_code != ERR_CODE_NONE) {
        /* ucode version mismatch */
        return(ERR_CODE_MICRO_INIT_NOT_DONE);
    }
    return (ERR_CODE_NONE);
}

#ifndef SMALL_FOOTPRINT

/* Timestamp check to see if heartbeat timer is programmed correctly for the COMCLK frequency it is running at */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_test_uc_timestamp_with_print_options(srds_access_t *sa__, uint8_t console_print_options) {
#ifdef SERDES_API_FLOATING_POINT
    INIT_SRDS_ERR_CODE
    uint16_t time = 0;
    uint16_t time1 = 0;
    uint16_t time2 = 0;
    USR_DOUBLE resolution;
    const uint16_t test_delay=400;

    const USR_DOUBLE max_resolution=11.0;
    const USR_DOUBLE min_resolution=9.0;

    EFUN(plp_aperta2_peregrine5_pc_pmd_uc_cmd(sa__, CMD_UC_DBG, CMD_UC_DBG_TIMESTAMP, 100));
    ESTM(time1=rd_uc_dsc_data());
    EFUN(USR_DELAY_MS(test_delay));
    EFUN(plp_aperta2_peregrine5_pc_pmd_uc_cmd(sa__, CMD_UC_DBG, CMD_UC_DBG_TIMESTAMP, 100));
    ESTM(time2=rd_uc_dsc_data());
    time=(uint16_t)(time2-time1);
    if(time == 0) {
        EFUN_PRINTF(("%s ERROR : Diag  timestamp fail, start = 0x%04x, end = 0x%04x\n", API_FUNCTION_NAME, time1, time2));
        return (ERR_CODE_DIAG_TIMESTAMP_FAIL);
    }
    resolution=test_delay*1000.0/(double)time;
    if((resolution<min_resolution)||(resolution>max_resolution)){
        if (console_print_options) {
            EFUN_PRINTF(("\nERROR : Lane %i:\tuC timestamp: %i\t\tResolution: %.1fus/count. Passing resolution limits:\tMax: %.1fus/count\tMin: %.1fus/count\n",plp_aperta2_peregrine5_pc_acc_get_lane(sa__),time,resolution,max_resolution,min_resolution));
        }
        return(ERR_CODE_DIAG_TIMESTAMP_FAIL);
    }
    else {
        if (console_print_options) {
            EFUN_PRINTF(("\nPassed timestamp check : Lane %i:\tuC timestamp: %i\t\tResolution: %.1fus/count. Passing resolution limits:\tMax: %.1fus/count\tMin: %.1fus/count\n",plp_aperta2_peregrine5_pc_acc_get_lane(sa__),time,resolution,max_resolution,min_resolution));
        }
   }
#else
    USR_PRINTF(("%s : This function needs SERDES_API_FLOATING_POINT define to operate \n", API_FUNCTION_NAME));
#endif
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_num_bits_per_ms(srds_access_t *sa__, uint8_t select_rx, uint32_t *num_bits_per_ms) {
    INIT_SRDS_ERR_CODE
    uint8_t osr_mode = 0;
    uint8_t pam4_mode = 0;
    uint8_t pll_select = 0, prev_pll = 0;
    struct peregrine5_pc_uc_core_config_st core_config = UC_CORE_CONFIG_INIT;
    peregrine5_pc_osr_mode_st osr_mode_st;
    ENULL_MEMSET(&osr_mode_st, 0, sizeof(peregrine5_pc_osr_mode_st));

    {
        uint8_t use_osr_mode_pin = 0;
        if(select_rx) {
            EFUN(plp_aperta2_peregrine5_pc_get_use_rx_osr_mode_pins_only(sa__, &use_osr_mode_pin));
            if(use_osr_mode_pin) {
                ESTM(osr_mode_st.rx = rd_rx_osr_mode_pin());
            }
            else {
                enum peregrine5_pc_osr_mode_enum rx_osr_mode = PEREGRINE5_PC_OSR_UNINITIALIZED;
                EFUN(plp_aperta2_peregrine5_pc_get_rx_osr_mode(sa__, &rx_osr_mode));
                osr_mode_st.rx = rx_osr_mode;
            }
        }
        else {
            EFUN(plp_aperta2_peregrine5_pc_get_use_tx_osr_mode_pins_only(sa__, &use_osr_mode_pin));
            if(use_osr_mode_pin) {
                ESTM(osr_mode_st.tx = rd_tx_osr_mode_pin());
            }
            else {
                enum peregrine5_pc_osr_mode_enum tx_osr_mode = PEREGRINE5_PC_OSR_UNINITIALIZED;
                EFUN(plp_aperta2_peregrine5_pc_get_tx_osr_mode(sa__, &tx_osr_mode));
                osr_mode_st.tx = tx_osr_mode;
            }
        }
        if (select_rx) {
            osr_mode = osr_mode_st.rx;
        } else {
            osr_mode = osr_mode_st.tx;
        }
    }

    if (select_rx) {
        ESTM(pam4_mode = rd_rx_pam4_mode());
    } else {
        ESTM(pam4_mode = rd_tx_pam4_mode());
    }

    ESTM(prev_pll = plp_aperta2_peregrine5_pc_acc_get_pll_idx(sa__));
    pll_select = 0;
    EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__,pll_select));

        EFUN(plp_aperta2_peregrine5_pc_get_uc_core_config(sa__, &core_config));
        *num_bits_per_ms = (uint32_t)((((uint64_t)core_config.vco_rate_in_Mhz * 100000UL) / plp_aperta2_peregrine5_pc_osr_mode_enum_to_int_x1000(osr_mode))*10);

    if(pam4_mode > 0) *num_bits_per_ms <<= 1;
    EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__,prev_pll));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_display_BER(srds_access_t *sa__, uint16_t time_ms) {
    INIT_SRDS_ERR_CODE
    char string[SRDS_MAX_BER_STR_LEN];
    struct peregrine5_pc_ber_data_st ber_data = BER_DATA_ST_INIT;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_BER_data_and_string(sa__, time_ms, &ber_data, string, sizeof(string)));
      USR_PRINTF(("%s",string));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_BER_data_and_string(srds_access_t *sa__, uint16_t time_ms, struct peregrine5_pc_ber_data_st *ber_data, char *string, uint8_t string_size) {
    INIT_SRDS_ERR_CODE
    err_code_t ber_data_err;
    enum peregrine5_pc_prbs_chk_timer_selection_enum timer_sel = USE_HW_TIMERS;
    ber_data_err = plp_aperta2_peregrine5_pc_INTERNAL_get_BER_data(sa__, time_ms, ber_data, timer_sel);
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_BER_string(sa__, ber_data, ber_data_err, string, string_size));
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_BER_string(srds_access_t *sa__, const struct peregrine5_pc_ber_data_st *ber_data, err_code_t ber_data_err, char *string, uint8_t string_size) {
    char string2[4];
    char string_extra[3] = "";
    struct peregrine5_pc_ber_data_st ber_data_local;

    if(string == NULL || ber_data == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    ber_data_local = *ber_data;

    if(ber_data_err == ERR_CODE_PRBS_CHK_DISABLED) {
        USR_SNPRINTF(string, (size_t)(string_size), " !chk_en ");
        return(ERR_CODE_NONE);
    }
    else if(ber_data_err == ERR_CODE_RX_CLKGATE_FRC_ON) {
        USR_SNPRINTF(string, (size_t)(string_size), "clk_gated");
        return(ERR_CODE_NONE);
    }
    else if(ber_data_err == ERR_CODE_NO_PMD_RX_LOCK) {
        USR_SNPRINTF(string, (size_t)(string_size), "!pmd_lock");
        return(ERR_CODE_NONE);
    }
    else if(ber_data_err == ERR_CODE_PRBS_CHK_HW_TIMERS_NOT_EXPIRED) {
        USR_SNPRINTF(string, (size_t)(string_size), "!time_exp");
        return(ERR_CODE_NONE);
    }
    else if(ber_data_err != ERR_CODE_NONE) {
        return peregrine5_pc_error(sa__, ber_data_err);
    }

    if((ber_data_local.num_errs < 3) && (ber_data_local.lcklost == 0)) {   /* lcklost = 0 */
        USR_SNPRINTF(string2, sizeof(string2), " <");
        ber_data_local.num_errs = 3;
    } else {
        USR_SNPRINTF(string2, sizeof(string2), "  ");
    }
    if(ber_data_local.cdrlcklost) {
        USR_SNPRINTF(string_extra, sizeof(string_extra), "*");
    }
    if(ber_data_local.lcklost == 1) {    /* lcklost = 1 */
        if(ber_data_local.prbs_lck_state == PRBS_CHECKER_NOT_ENABLED) {
            USR_SNPRINTF(string, (size_t)(string_size), "         ");
        }
        else if(ber_data_local.prbs_lck_state == PRBS_CHECKER_LOCK_LOST_AFTER_LOCK) {
            USR_SNPRINTF(string, (size_t)(string_size), " Lck lost");
        }
        else
        {
            USR_SNPRINTF(string, (size_t)(string_size), "  !Lock  ");
        }
    }
    else {                    /* lcklost = 0 */
        uint16_t x=0,y=0,z=0,srds_div;

        if(ber_data_local.num_errs < ber_data_local.num_bits) {
            while(1) {
                srds_div = (uint16_t)(((ber_data_local.num_errs<<1) + ber_data_local.num_bits)/(ber_data_local.num_bits<<1));
                if(srds_div>=10) break;
                ber_data_local.num_errs = ber_data_local.num_errs*10;
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
            USR_SNPRINTF(string, (size_t)(string_size), "%s%d.%1de-%02d%s",string2,x,y,z,string_extra);
        }
        else {
            USR_SNPRINTF(string, (size_t)(string_size), " ERR_OVF ");
        }

    }
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_BER_data(srds_access_t *sa__, uint16_t time_ms, struct peregrine5_pc_ber_data_st *ber_data, enum peregrine5_pc_prbs_chk_timer_selection_enum timer_sel) {
    INIT_SRDS_ERR_CODE
    uint8_t  lcklost = 0;
    uint32_t err_cnt= 0;

    /* Configure, Start, and Poll for end */
    if (timer_sel == USE_HW_TIMERS) {
        /* Create PRBS HW timer ctrl struct */
        struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st prbs_chk_hw_timer_ctrl;
        err_code_t ber_err = ERR_CODE_NONE;

        /* Call functionality step by step */
        ber_err = plp_aperta2_peregrine5_pc_BER_setup_measurement(sa__, time_ms, ber_data, &prbs_chk_hw_timer_ctrl);
        if(ber_err != ERR_CODE_NONE) {
            return ber_err;
        }
        ber_err = plp_aperta2_peregrine5_pc_BER_poll_measurement(sa__, ber_data, &prbs_chk_hw_timer_ctrl);
        if(ber_err != ERR_CODE_NONE) {
            /* restore prbs hardware timer config registers */   
            EFUN(plp_aperta2_peregrine5_pc_set_prbs_chk_hw_timer_ctrl(sa__, &prbs_chk_hw_timer_ctrl));
            return ber_err;
        }
        EFUN(plp_aperta2_peregrine5_pc_BER_get_measurement(sa__, ber_data, &prbs_chk_hw_timer_ctrl));
    }
    /* Else HW timers not available, use SW TIMERS method */
    else
    {
        uint8_t clk_gate = 0;
        UNUSED(timer_sel);
        ESTM(ber_data->prbs_chk_en = rd_prbs_chk_en());
        ESTM(ber_data->prbs_lck_state = rd_prbs_chk_lock_state());
        ESTM(clk_gate = rd_ln_rx_s_clkgate_frc_on());
        if(ber_data->prbs_chk_en == 0) {
            return(ERR_CODE_PRBS_CHK_DISABLED);
        }
        else if (clk_gate == 1) {
            return(ERR_CODE_RX_CLKGATE_FRC_ON);
        }
        EFUN(plp_aperta2_peregrine5_pc_prbs_err_count_state(sa__, &err_cnt, &lcklost)); /* clear error counters */
        EFUN(USR_DELAY_MS(time_ms));
        EFUN(plp_aperta2_peregrine5_pc_prbs_err_count_state(sa__, &err_cnt, &lcklost));
        ber_data->lcklost = lcklost;
        if(ber_data->lcklost == 0) {
            uint32_t num_bits_per_ms=0;
            EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_num_bits_per_ms(sa__, 1, &num_bits_per_ms));
            ber_data->num_bits = (uint64_t)num_bits_per_ms*(uint64_t)time_ms;
            ber_data->num_errs = err_cnt;
            ESTM(lcklost = rd_prbs_chk_lock_lost_lh()); /* Clear sticky bit */
            UNUSED(lcklost);
        }

    }

    return(ERR_CODE_NONE);
}

/* Check CDR lost lock */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_prbs_chk_cdr_lock_lost(srds_access_t *sa__, uint8_t *cdrlcklost) {
    INIT_SRDS_ERR_CODE
    uint32_t err_cnt, num_bits_per_ms = 0, num_bits;
    uint8_t lcklost;
    uint8_t time_ms = 1;
    if(!cdrlcklost) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    EFUN(plp_aperta2_peregrine5_pc_prbs_err_count_state(sa__, &err_cnt,&lcklost)); /* clear error counters */
    EFUN(USR_DELAY_MS(time_ms));
    EFUN(plp_aperta2_peregrine5_pc_prbs_err_count_state(sa__, &err_cnt,&lcklost));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_num_bits_per_ms(sa__, 1, &num_bits_per_ms));
    num_bits = num_bits_per_ms*time_ms;

    /* check for BER>0.25 */
    *cdrlcklost = lcklost || (err_cnt>(num_bits>>2));

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_prbs_timeout_count_from_time(uint16_t time_ms, uint16_t * time_ms_adjusted, struct peregrine5_pc_prbs_chk_hw_timer_ctrl_st * const prbs_chk_hw_timer_ctrl) {
    uint16_t timeout_value=0;
    uint8_t timer_unit_sel;

    if(!time_ms_adjusted || !prbs_chk_hw_timer_ctrl) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    *time_ms_adjusted = time_ms;

    /* Select unit of time for PRBS HW TIMER */
    if (time_ms < 100) {
        timer_unit_sel = PRBS_CHK_EN_TIMER_UNIT_SEL_1MS;
        timeout_value = (uint32_t)time_ms;
    }
    else if (time_ms < 1000) {
        timer_unit_sel = PRBS_CHK_EN_TIMER_UNIT_SEL_10MS;
        timeout_value = (uint32_t)time_ms/10;
    }
    else {
        timer_unit_sel = PRBS_CHK_EN_TIMER_UNIT_SEL_100MS;
        timeout_value = (uint32_t)time_ms/100;
    }

    prbs_chk_hw_timer_ctrl->prbs_chk_en_timer_unit_timeout = timeout_value;
    prbs_chk_hw_timer_ctrl->prbs_chk_en_timer_unit_sel = timer_unit_sel;

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_hw_timer_sync_delay(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    uint8_t i, timer_unit_sel;
    uint8_t ms_delay = 0;
    uint16_t delay_count = 1;
    ESTM(timer_unit_sel = rd_prbs_chk_en_timer_unit_sel());

    /* prbs_chk_en_timer_unit_sel register should have a valid value when calling this function */
    if((timer_unit_sel == PRBS_CHK_EN_TIMER_UNIT_SEL_DISABLED) || (timer_unit_sel == PRBS_CHK_EN_TIMER_UNIT_SEL_RESERVED)) {
        return ERR_CODE_INVALID_VALUE;
    }

    /* Determine the delay to wait for synchronization based on the prbs_chk_en_timer_unit_sel */
    for(i=PRBS_CHK_EN_TIMER_UNIT_SEL_1US; i<=PRBS_CHK_EN_TIMER_UNIT_SEL_100MS; i++) {
        if(i == PRBS_CHK_EN_TIMER_UNIT_SEL_1MS) {
            ms_delay = 1;
            delay_count /= 1000;
        }
        if(i < timer_unit_sel) {
            delay_count = (uint16_t)(delay_count * 10);
        } else {
            break;
        }
    }
    /* Add the appropriate delay */
    if(ms_delay){
        EFUN(USR_DELAY_MS(delay_count));
    } else {
        if(delay_count!=1) EFUN(USR_DELAY_US(delay_count));
    }
    return ERR_CODE_NONE;
}



err_code_t plp_aperta2_peregrine5_pc_INTERNAL_ladder_setting_to_mV(srds_access_t *sa__, int16_t ctrl, uint8_t range_250, afe_override_slicer_sel_t slicer_sel, int16_t *nlmv_val) {
    uint16_t absv;                                    /* Absolute value of ctrl */
    int16_t nlmv=0;                                     /* Non-linear value */
    UNUSED(sa__);
    UNUSED(slicer_sel);
    if (nlmv_val == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    /* Get absolute value */
    absv = (uint16_t)(SRDS_ABS(ctrl));

       UNUSED(range_250);
       nlmv = (int16_t)(absv*1);
    /* Add sign and return */
    *nlmv_val=(int16_t)((ctrl>=0) ? nlmv : -nlmv);

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_pam4_to_bin(srds_access_t *sa__, char var, char bin[], size_t bin_size) {
    if(!bin) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

  switch (var) {
    case '0': USR_SNPRINTF(bin, bin_size, "00");
              break;
    case '1': USR_SNPRINTF(bin, bin_size, "01");
              break;
    case '2': USR_SNPRINTF(bin, bin_size, "11");  /* To account for PAM4 Gray coding */
              break;
    case '3': USR_SNPRINTF(bin, bin_size, "10");  /* To account for PAM4 Gray coding */
              break;
    case '_': bin[0] = '\0';
              break;
    default : bin[0] = '\0';
              EFUN_PRINTF(("ERROR: Invalid PAM4 format Pattern\n"));
              return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_INVALID_PAM4));
  }
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_compute_bin(srds_access_t *sa__, char var, char bin[], size_t bin_size) {
    if(!bin) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

  switch (var) {
    case '0':  USR_SNPRINTF(bin, bin_size, "0000");
               break;
    case '1':  USR_SNPRINTF(bin, bin_size, "0001");
               break;
    case '2':  USR_SNPRINTF(bin, bin_size, "0010");
               break;
    case '3':  USR_SNPRINTF(bin, bin_size, "0011");
               break;
    case '4':  USR_SNPRINTF(bin, bin_size, "0100");
               break;
    case '5':  USR_SNPRINTF(bin, bin_size, "0101");
               break;
    case '6':  USR_SNPRINTF(bin, bin_size, "0110");
               break;
    case '7':  USR_SNPRINTF(bin, bin_size, "0111");
               break;
    case '8':  USR_SNPRINTF(bin, bin_size, "1000");
               break;
    case '9':  USR_SNPRINTF(bin, bin_size, "1001");
               break;
    case 'a':
    case 'A':  USR_SNPRINTF(bin,bin_size,"1010");
               break;
    case 'b':
    case 'B':  USR_SNPRINTF(bin,bin_size,"1011");
               break;
    case 'c':
    case 'C':  USR_SNPRINTF(bin,bin_size,"1100");
               break;
    case 'd':
    case 'D':  USR_SNPRINTF(bin,bin_size,"1101");
               break;
    case 'e':
    case 'E':  USR_SNPRINTF(bin,bin_size,"1110");
               break;
    case 'f':
    case 'F':  USR_SNPRINTF(bin,bin_size,"1111");
               break;
    case '_': bin[0] = '\0';
              break;
    default : bin[0] = '\0';
               USR_PRINTF(("ERROR: Invalid Hexadecimal Pattern\n"));
               return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_INVALID_HEX));
  }
  return (ERR_CODE_NONE);
}


err_code_t plp_aperta2_peregrine5_pc_INTERNAL_compute_hex(srds_access_t *sa__, char bin[],uint8_t *hex) {
  if(!hex || !bin) {
    return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
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
    return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_INVALID_BIN2HEX));
  }
  return (ERR_CODE_NONE);
}

#define MICRO_STOP_BITS  (0x77)


uint8_t plp_aperta2_peregrine5_pc_INTERNAL_stop_micro(srds_access_t *sa__, uint8_t graceful, err_code_t *err_code_p) {
   INIT_SRDS_ERR_CODE
   uint8_t stop_state = 0;

   if(!err_code_p) {
       return(0xFF);
   }

   /* Log current micro stop status */
   EPSTM2((stop_state = rdv_usr_sts_micro_stopped()),0xFF);

   /* Stop micro only if micro is not stopped currently */
   if (!(stop_state & MICRO_STOP_BITS)) {
       if (graceful) {
           EPFUN2((plp_aperta2_peregrine5_pc_stop_rx_adaptation(sa__, 1)),0xFF);
       }
       else {
           EPFUN2((plp_aperta2_peregrine5_pc_pmd_uc_control(sa__, CMD_UC_CTRL_STOP_IMMEDIATE,GRACEFUL_STOP_TIME)),0xFF);
       }
   }

   /* Return the previous micro stop status */
   return(stop_state);
}




/********************************************************/
/*  Global RAM access through Micro Register Interface  */
/********************************************************/
/* Micro Global RAM Byte Read */
uint8_t plp_aperta2_peregrine5_pc_INTERNAL_rdb_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint32_t addr) {
    INIT_SRDS_ERR_CODE
    uint8_t rddata;

    if(!err_code_p) {
        return(0);
    }
    EPSTM(rddata = plp_aperta2_peregrine5_pc_rdb_uc_ram(sa__, err_code_p, addr)); /* Use Micro register interface for reading RAM */
    return (rddata);
}

/* Micro Global RAM Word Read */
uint16_t plp_aperta2_peregrine5_pc_INTERNAL_rdw_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint32_t addr) {
  uint16_t rddata;
  INIT_SRDS_ERR_CODE

  if(!err_code_p) {
      return(0);
  }
  if (addr%2 != 0) {                                         /* Validate even address */
      *err_code_p = ERR_CODE_INVALID_RAM_ADDR;
      USR_PRINTF(("Error incorrect addr x%04x\n",addr));
      return (0);
  }
  EPSTM(rddata = plp_aperta2_peregrine5_pc_rdw_uc_ram(sa__, err_code_p, addr)); /* Use Micro register interface for reading RAM */
  return (rddata);
}

/* Micro Global RAM Byte Write  */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_wrb_uc_var(srds_access_t *sa__, uint32_t addr, uint8_t wr_val) {

    return (plp_aperta2_peregrine5_pc_wrb_uc_ram(sa__, addr, wr_val));          /* Use Micro register interface for writing RAM */
}

/* Micro Global RAM Word Write  */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_wrw_uc_var(srds_access_t *sa__, uint32_t addr, uint16_t wr_val) {
    if (addr%2 != 0) {                                       /* Validate even address */
      USR_PRINTF(("Error incorrect addr x%04x\n",addr));
        return (peregrine5_pc_error(sa__, ERR_CODE_INVALID_RAM_ADDR));
    }
    return (plp_aperta2_peregrine5_pc_wrw_uc_ram(sa__, addr, wr_val));          /* Use Micro register interface for writing RAM */
}


/***********************/
/*  Event Log Display  */
/***********************/
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_event_log_dump_callback(srds_access_t *sa__, void *arg, uint8_t byte_count, uint16_t data) {
    peregrine5_pc_INTERNAL_event_log_dump_state_t * const state_ptr = (peregrine5_pc_INTERNAL_event_log_dump_state_t *)arg;
    const uint8_t bytes_per_row=16;

    /* when detecting more than 256 bytes of consequtive 0 in data,
     * terminate the printing to the log
     */
    if (state_ptr->zero_cnt == 0xffff) {
        return (ERR_CODE_NONE);
    }
    if (data == 0) {
        state_ptr->zero_cnt++;
    }
    else {
        state_ptr->zero_cnt = 0;
    }
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
        /* 128 words/256 bytes is the largest event data section.
         * 0xffff is used as a flag to terminate the printing to the logfile
         */
        if (state_ptr->zero_cnt > 128) {
            state_ptr->zero_cnt = 0xffff;
        }
    }

    return(ERR_CODE_NONE);
}



err_code_t plp_aperta2_peregrine5_pc_INTERNAL_read_event_log_with_callback(srds_access_t *sa__,
                                                        uint8_t micro_num,
                                                        uint8_t bypass_micro,
                                                        void *arg,
                                                        err_code_t (*callback)(srds_access_t *, void *, uint8_t, uint16_t)) {
    INIT_SRDS_ERR_CODE

    srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    uint16_t rd_idx;
    uint8_t current_lane;


    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));

    if (micro_num >= peregrine5_pc_info_ptr->micro_count) {
        return (peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    /* Read Current lane so that it can be restored at the end of function */
    current_lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    /* 2 lanes per micro; Set lane appropriate for the desired micro */
    EFUN(peregrine5_pc_acc_set_physical_lane(sa__,(uint8_t)(micro_num<<1)));
    /* Check info table version for feature inclusion */
    if ((peregrine5_pc_info_ptr->signature >> 24) >= 0x42) {
        ESTM_PRINTF(("\n\nEVENT MASK %u = 0x%08x\n", (uint8_t)(micro_num<<1), rdv_usr_event_log_group_mask()));
        /* 2 lanes per micro; Set to the 2nd lane for the desired micro */
        EFUN(peregrine5_pc_acc_set_physical_lane(sa__,(uint8_t)((micro_num<<1) + 1)));
        ESTM_PRINTF(("EVENT MASK %u = 0x%08x\n", (uint8_t)((micro_num<<1) + 1), rdv_usr_event_log_group_mask()));
        EFUN(peregrine5_pc_acc_set_physical_lane(sa__,(uint8_t)(micro_num<<1)));
    }
    EFUN_PRINTF(("\n\n********************************************\n"));
    EFUN_PRINTF((    "**** SERDES UC TRACE MEMORY DUMP ***********\n"));
    EFUN_PRINTF((    "********************************************\n"));
    if (bypass_micro == 0) {
        uint16_t reset_state;
        ESTM(reset_state = rd_lane_dp_reset_state());
        if (reset_state & 0x0007) {
            EFUN(peregrine5_pc_acc_set_physical_lane(sa__,(uint8_t)((micro_num<<1) + 1)));
            ESTM(reset_state = rd_lane_dp_reset_state());
            if (reset_state & 0x0007) {
                EFUN_PRINTF(("Both lanes of micro %d are in reset. Setting bypass_micro to get event log.\n", micro_num));
                bypass_micro = 1;
            }
        }
    }
    if (bypass_micro) {
        ESTM(rd_idx = rducv_trace_mem_wr_idx());
        if (peregrine5_pc_info_ptr->trace_memory_descending_writes) {
            ++rd_idx;
            if (rd_idx >= peregrine5_pc_info_ptr->trace_mem_ram_size) {
                rd_idx = 0;
            }
        } else {
            if (rd_idx == 0) {
                rd_idx = (uint16_t)peregrine5_pc_info_ptr->trace_mem_ram_size;
            }
            --rd_idx;
        }
    } else {
        /* Start Read to stop uC logging and read the word at last event written by uC */
        EFUN(plp_aperta2_peregrine5_pc_pmd_uc_cmd(sa__, CMD_EVENT_LOG_READ, CMD_EVENT_LOG_READ_START, GRACEFUL_STOP_TIME));
        ESTM(rd_idx = rducv_trace_mem_rd_idx());
    }

    EFUN_PRINTF(( "\n  DEBUG INFO: trace memory read index = 0x%04x\n", rd_idx));
    EFUN_PRINTF(("  DEBUG INFO: trace memory size = 0x%04x\n", peregrine5_pc_info_ptr->trace_mem_ram_size));
    EFUN_PRINTF(("  DEBUG INFO: bypass_micro = 0x%04x\n\n", bypass_micro));

    if (bypass_micro) {
        const uint8_t decoder_version = 0x02;
        peregrine5_pc_INTERNAL_event_log_dump_state_t * const state_ptr = (peregrine5_pc_INTERNAL_event_log_dump_state_t *)arg;
        EFUN_PRINTF(("  0x%02x00  0x00%02x  0x0000", EVENT_SOURCE_VERID, decoder_version));
        state_ptr->index = 6;
    }

    if (peregrine5_pc_info_ptr->trace_memory_descending_writes) {
        /* Micro writes trace memory using descending addresses.
         * So read using ascending addresses using block read
         */

        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram(sa__,
                                                  peregrine5_pc_info_ptr->trace_mem_ram_base + (uint32_t)(peregrine5_pc_info_ptr->grp_ram_size*micro_num),
                                                  (uint16_t)peregrine5_pc_info_ptr->trace_mem_ram_size,
                                                  rd_idx,
                                                  (uint16_t)peregrine5_pc_info_ptr->trace_mem_ram_size,
                                                  arg,
                                                  callback));
    } else {
        /* Micro writes trace memory using descending addresses.
         * So read using ascending addresses using block read
         */
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram_descending(sa__,
                                                             peregrine5_pc_info_ptr->trace_mem_ram_base + (uint32_t)(peregrine5_pc_info_ptr->grp_ram_size*micro_num),
                                                             (uint16_t)peregrine5_pc_info_ptr->trace_mem_ram_size,
                                                             rd_idx,
                                                             (uint16_t)peregrine5_pc_info_ptr->trace_mem_ram_size,
                                                             arg,
                                                             callback));
    }


    if (!bypass_micro) {
        /* Read Done to resume logging  */
        EFUN(plp_aperta2_peregrine5_pc_pmd_uc_cmd(sa__, CMD_EVENT_LOG_READ, CMD_EVENT_LOG_READ_DONE, 50));
    }
    EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__,current_lane));
    return(ERR_CODE_NONE);
}  /* plp_aperta2_peregrine5_pc_INTERNAL_read_event_log_with_callback() */


#ifdef TO_FLOATS
/* convert uint32_t to float8 */
float8_t peregrine5_pc_INTERNAL_int32_to_float8(uint32_t input) {
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
uint32_t plp_aperta2_peregrine5_pc_INTERNAL_float8_to_uint32(float8_t input) {
    uint32_t x;
    if(input == 0) return(0);
    x = (uint32_t)((((uint8_t)(input))>>5) + 8);
    if((input & 0x1F) < 3) {
      return(x>>(3-(input & 0x1f)));
    } else if((input & 0x1F) > 3) {
        x=(x<<1)|1;
        return((x<<((input & 0x1F)-3-1)));
    }else {
        return(x<<((input & 0x1F)-3));
    }
}

/* Convert uint8 to 8-bit gray code */
uint8_t plp_aperta2_peregrine5_pc_INTERNAL_uint8_to_gray(uint8_t input) {
    return input ^ (input >> 1);
}

/* Convert 8-bit gray code to uint8 */
uint8_t plp_aperta2_peregrine5_pc_INTERNAL_gray_to_uint8(uint8_t input) {
    input = input ^ (input >> 4);
    input = input ^ (input >> 2);
    input = input ^ (input >> 1);
    return input;
}

/* Convert seconds to hr:min:sec */
uint8_t plp_aperta2_peregrine5_pc_INTERNAL_seconds_to_displayformat(uint32_t seconds, uint8_t *hrs, uint8_t *mins, uint8_t *secs) {
    if(!hrs || !mins || !secs) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    *hrs    = (uint8_t)(seconds / 3600);
    seconds = (seconds % 3600);
    *mins   = (uint8_t)(seconds / 60);
    *secs   = (uint8_t)(seconds % 60);
    return(ERR_CODE_NONE);
}

/* convert float12 to uint32 */
uint32_t plp_aperta2_peregrine5_pc_INTERNAL_float12_to_uint32(uint8_t input, uint8_t multi) {

    return(((uint32_t)input)<<multi);
}

#endif /* !SMALL_FOOTPRINT */

#if !defined(SMALL_FOOTPRINT) 
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_set_rx_pf_main(srds_access_t *sa__, uint8_t val) {
    INIT_SRDS_ERR_CODE
    uint8_t pf_max;
    pf_max = PF_MAX_VALUE;
    if (val > pf_max) {
       return (peregrine5_pc_error(sa__, ERR_CODE_PF_INVALID));
    }
    EFUN(WR_RX_PF_CTRL(val));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_set_rx_pf2(srds_access_t *sa__, uint8_t val) {
    INIT_SRDS_ERR_CODE
    if (val > PF2_MAX_VALUE) {
      return (peregrine5_pc_error(sa__, ERR_CODE_PF_INVALID));
    }
    EFUN(WR_RX_PF2_CTRL(val));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_set_rx_pf3(srds_access_t *sa__, uint8_t val) {
    INIT_SRDS_ERR_CODE
    uint8_t pf3_max;
    pf3_max = PF3_MAX_VALUE;
    if(val > pf3_max) {
      return (peregrine5_pc_error(sa__, ERR_CODE_PF_INVALID));
    }

    EFUN(WR_RX_PF3_CTRL(val));
    return(ERR_CODE_NONE);
}
#endif /* !SMALL_FOOTPRINT */

#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pf_main(srds_access_t *sa__, uint8_t *val) {
    INIT_SRDS_ERR_CODE
    ESTM(*val = RD_RX_PF_CTRL());
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pf2(srds_access_t *sa__, uint8_t *val) {
    INIT_SRDS_ERR_CODE
    ESTM(*val = RD_RX_PF2_CTRL());
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pf3(srds_access_t *sa__, uint8_t *val) {
    INIT_SRDS_ERR_CODE
    ESTM(*val = RD_RX_PF3_CTRL());
    return (ERR_CODE_NONE);
}

#endif /* !SMALL_FOOTPRINT */

#if !defined(SMALL_FOOTPRINT) 

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pga(srds_access_t *sa__, uint8_t *val) {
    INIT_SRDS_ERR_CODE
    ESTM(*val = (uint8_t)((16-rd_rx_pga_att_val()) + rd_rx_pga_val() + rd_afe_pga_boost_ctrl()));

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_set_rx_pga(srds_access_t *sa__, uint8_t val) {
    INIT_SRDS_ERR_CODE
    uint8_t pga_att_ctrl = 0, pga_ctrl = 0, pga_boost_ctrl = 0;

    if (val <= 16) {
        pga_att_ctrl = (16-val);
        pga_ctrl = 0;
        pga_boost_ctrl = 0;
    }
    else if (val <= (16+45)) {
        pga_att_ctrl = 0;
        pga_ctrl = (val-16);
        pga_boost_ctrl = 0;
    }
    else {
        pga_att_ctrl = 0;
        pga_ctrl = 45;
        pga_boost_ctrl = (val-16-45);
    }

    EFUN(wr_rx_pga_att_val(pga_att_ctrl));
    EFUN(wr_rx_pga_val(pga_ctrl));
    EFUN(wr_afe_pga_boost_ctrl (pga_boost_ctrl));

    return (ERR_CODE_NONE);
}

#endif /* !SMALL_FOOTPRINT */

#ifndef SMALL_FOOTPRINT
#endif /* !SMALL_FOOTPRINT */

#if !defined(SMALL_FOOTPRINT) 
#endif /* !SMALL_FOOTPRINT */

#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_dfe1(srds_access_t *sa__, int8_t *val) {
    INIT_SRDS_ERR_CODE
    ESTM(*val = (int8_t)rd_dfe_alpha());
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_dfe2(srds_access_t *sa__, int8_t *val) {
    INIT_SRDS_ERR_CODE
    ESTM(*val = rd_dfe_beta());
    return (ERR_CODE_NONE);
}
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_ffe_enabled(srds_access_t *sa__, uint8_t *val) {
    UNUSED(sa__);
    *val = 1;
    return (ERR_CODE_NONE);
}
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_intrusive_eye_scan(srds_access_t *sa__, uint32_t *data, int16_t *y_max, int16_t *y_step, uint8_t *lane_mask, uint8_t num_lanes, uint16_t *usr_diag_status) {
    INIT_SRDS_ERR_CODE
    uint32_t x;
    uint8_t lane_orig = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    uint8_t lane, stop_lane, start_lane;
    uint8_t error_mask = 0;
    uint8_t ilb_en;
    uint32_t intrusive_buff_size;
    err_code_t err[NUM_LANES_MAX];
    enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;

    if(!data || !y_max || !y_step || !lane_mask) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    if(num_lanes == 0 || num_lanes > NUM_LANES_MAX) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    ESTM(ilb_en = rd_ilb_en());
    if(ilb_en) {
        EFUN_PRINTF(("ERROR: intrusive eye scan is not supported in internal loopback mode.\n"));
        return (ERR_CODE_INVALID_MODE);
    }
    if(num_lanes == 1) {
        start_lane = lane_orig;
        stop_lane = lane_orig;
    }
    else {
        start_lane = 0;
        stop_lane = (uint8_t)(num_lanes - 1);
    }

    *lane_mask = 0;

    for(lane = start_lane; lane <= stop_lane; lane++) {
        uint8_t y_index;
        err[lane] = ERR_CODE_NONE;
        EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane));
        if(num_lanes == 1) {
            y_index = 0;
        }
        else {
            y_index = lane;
        }
        err[lane] = plp_aperta2_peregrine5_pc_INTERNAL_eye_scan_setup(sa__, 1, &y_max[y_index], &y_step[y_index], (usr_diag_status+(lane-start_lane)));

        if(num_lanes == 1 && (err[lane] != ERR_CODE_NONE)) {
            return err[lane];
        }
        if((err[lane] == ERR_CODE_RX_TUNING_NOT_DONE) || ((enum srds_diag_failcodes)err[lane] == ERR_CODE_DIAG_SCAN_NO_PMD_LOCK)) {
            srds_core_t core;
            ESTM(core = plp_aperta2_peregrine5_pc_acc_get_core(sa__));
            EFUN_PRINTF(("core(%d), lane(%d) ignored for 2D eye scan\n", core, lane));
            err[lane] = ERR_CODE_NONE;
        }
        else if(err[lane] != ERR_CODE_NONE) {
           error_mask |= (uint8_t)(1<<lane);
        }
        else {
            *lane_mask |= (uint8_t)(1<<lane);
        }
    }

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));

    intrusive_buff_size = ((pam_mode == NRZ) ? EYE_SCAN_NRZ_INTRUSIVE_BUF_SIZE : EYE_SCAN_PAM_INTRUSIVE_BUF_SIZE);

    for (x=0; x<intrusive_buff_size; x+=EYE_SCAN_DISPLAY_SRIPE_SIZE) {
        for(lane=start_lane; lane <= stop_lane; lane++) {
            if(*lane_mask & (1<<lane)) {
                uint16_t status = 0;
                uint32_t data_index;

                EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane));
                if(num_lanes == 1) {
                    data_index = x;
                }
                else {
                    data_index = (uint32_t)(intrusive_buff_size * lane) + x;
                }
                err[lane] = plp_aperta2_peregrine5_pc_read_eye_scan_stripe(sa__, &data[data_index], &status);
                if (err[lane]) {
                    err_code_t meas_done_err = plp_aperta2_peregrine5_pc_meas_eye_scan_done(sa__);
                    EFUN_PRINTF(("Error on lane %d during plp_aperta2_peregrine5_pc_read_eye_scan_stripe().\n", lane));
                    if(meas_done_err != ERR_CODE_NONE) {
                        EFUN_PRINTF(("Error on lane %d during plp_aperta2_peregrine5_pc_meas_eye_scan_done().\n", lane));
                    }
                    *lane_mask &=  (uint8_t)~(1<<lane);
                    error_mask |= (uint8_t)(1<<lane);
                }
            }
        }
    }

    /* Stop acquisition */
    for(lane = start_lane; lane <= stop_lane; lane++) {
        if(*lane_mask & (1 << lane)) {
            EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane));
            err[lane] = plp_aperta2_peregrine5_pc_meas_eye_scan_done(sa__);
            if(err[lane] != ERR_CODE_NONE) {
                EFUN_PRINTF(("Error on lane %d during plp_aperta2_peregrine5_pc_meas_eye_scan_done().\n", lane));
                error_mask |= (uint8_t)(1<<lane);
            }
        }
    }

    EFUN(plp_aperta2_peregrine5_pc_acc_set_lane(sa__, lane_orig));

    if(error_mask) {
        for(lane=start_lane;lane <= stop_lane;lane++) {
            if(error_mask & (1<<lane)) {
                EFUN_PRINTF(("The following error occurred on lane %d during plp_aperta2_peregrine5_pc_INTERNAL_get_intrusive_eye_scan():\n", lane));
                peregrine5_pc_error_report(sa__, err[lane]);
            }
        }
    }

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_eye_scan_setup(srds_access_t *sa__, uint8_t intr_pass_b, int16_t *y_max, int16_t *y_step, uint16_t *usr_diag_status) {
    INIT_SRDS_ERR_CODE
    enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;
    uint8_t dig_lpbk_en;
    int16_t val_max,val_step;

    /* init value */
    *usr_diag_status = 0;

    EFUN(wrv_usr_eye_scan_config_word(0));
    ESTM(dig_lpbk_en = rd_dig_lpbk_en());
    if(dig_lpbk_en) {
        EFUN_PRINTF(("Eye scan is not supported while digital loopback is enabled.\n"));
        return (ERR_CODE_INVALID_DIG_LPBK_STATE);
    }
    val_max  = EYE_SCAN_NRZ_VERTICAL_IDX_MAX;
    val_step = EYE_SCAN_NRZ_VERTICAL_STEP;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
    if((pam_mode == PAM4_NR)||(pam_mode == PAM4_ER)) {
        val_max  = EYE_SCAN_PAM_VERTICAL_IDX_MAX;
        val_step = EYE_SCAN_PAM_VERTICAL_STEP;
    }
    EFUN_PRINTF(("\n\n****  SERDES %sEYE SCAN CORE %d LANE %d   ****\n", intr_pass_b?"INTRUSIVE ":"",plp_aperta2_peregrine5_pc_acc_get_core(sa__),plp_aperta2_peregrine5_pc_acc_get_lane(sa__)));

    if(intr_pass_b == 0) {
        uint8_t ffe_enabled;
        ffe_enabled = 1;
        if(ffe_enabled) {   /* Unsupported */
            EFUN_PRINTF(("2-D eye scan is not supported, running eye slice instead of eye scan\n"));
        }
    }
    /* start horizontal acquisition */
    {
        uint8_t direction = EYE_SCAN_SLICE;
        err_code_t err_code;

        if(intr_pass_b) {
            direction = EYE_SCAN_INTR;
        }

        err_code = plp_aperta2_peregrine5_pc_meas_eye_scan_start(sa__, direction);

        if (err_code) {
            if (err_code == ERR_CODE_DIAG_SCAN_NO_PMD_LOCK) {
                return err_code;
            }
            else {
                EFUN(plp_aperta2_peregrine5_pc_meas_eye_scan_done(sa__));
            }
        }
    }

    {
        uint8_t rx_tuning_done=0;
        EFUN(plp_aperta2_peregrine5_pc_get_rx_tuning_status(sa__, &rx_tuning_done));
        if((pam_mode != NRZ) && (!rx_tuning_done)) {
            EFUN_PRINTF(("2-D eye scan is not ready yet, wait for RX tuning status to be set\n"));
            EFUN(plp_aperta2_peregrine5_pc_meas_eye_scan_done(sa__));
            return ERR_CODE_RX_TUNING_NOT_DONE;
        }
    }

    ESTM(*usr_diag_status = rdv_usr_diag_status());
    if((pam_mode != NRZ) && ((*usr_diag_status & DIAG_EYE_SCAN_V_RANGE_MASK) == DIAG_EYE_SCAN_V_HALF)) {
        val_max  = EYE_SCAN_PAM_VERTICAL_IDX_MAX>>1;
        val_step = EYE_SCAN_PAM_VERTICAL_STEP>>1;
    }


    *y_max = val_max;
    *y_step = val_step;

    return ERR_CODE_NONE;
}
#endif /* SMALL_FOOTPRINT */

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_load_txfir_taps(srds_access_t *sa__){
    INIT_SRDS_ERR_CODE
    uint8_t tx_elec_idle_status = 0;
    uint8_t tx_disable_output_sel_orig = 0;
    uint8_t revid2;
    ESTM(revid2 = rdc_revid2());
    ESTM(tx_elec_idle_status = rd_tx_elec_idle_status());
    if((revid2 < 0x8) && (tx_elec_idle_status))
    {
        ESTM(tx_disable_output_sel_orig = rd_tx_disable_output_sel());
        EFUN(wr_ams_tx_pd_dac(1));
        EFUN(wr_tx_disable_output_sel(3)); /* Send zeroes */
    EFUN(wr_txfir_tap_load(1));  /* Load the tap coefficients into TXFIR. */
        EFUN(wr_tx_disable_output_sel(tx_disable_output_sel_orig));
        EFUN(wr_ams_tx_pd_dac(0));
    }
    else
    {
        EFUN(wr_txfir_tap_load(1));  /* Load the tap coefficients into TXFIR. */
    }
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_set_tx_tap(srds_access_t *sa__, enum peregrine5_pc_tx_afe_settings_enum tap_num, int16_t val) {
    INIT_SRDS_ERR_CODE

    switch (tap_num) {
    case  PEREGRINE5_PC_TX_AFE_PRE3:  EFUN(wr_txfir_pre3_tap  ((uint16_t)val)); break;
    case  PEREGRINE5_PC_TX_AFE_PRE2:  EFUN(wr_txfir_pre2_tap  ((uint16_t)val)); break;
    case  PEREGRINE5_PC_TX_AFE_PRE1:  EFUN(wr_txfir_pre1_tap  ((uint16_t)val)); break;
    case  PEREGRINE5_PC_TX_AFE_MAIN:  EFUN(wr_txfir_main_tap  ((uint16_t)val)); break;
    case  PEREGRINE5_PC_TX_AFE_POST1: EFUN(wr_txfir_post1_tap ((uint16_t)val)); break;
    case  PEREGRINE5_PC_TX_AFE_POST2: EFUN(wr_txfir_post2_tap ((uint16_t)val)); break;

    default:
        return (peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_tx_tap(srds_access_t *sa__, enum peregrine5_pc_tx_afe_settings_enum tap_num, int16_t *val) {
    INIT_SRDS_ERR_CODE

    switch (tap_num) {
    case  PEREGRINE5_PC_TX_AFE_PRE3:  ESTM(*val = rd_txfir_pre3_tap  ()); break;
    case  PEREGRINE5_PC_TX_AFE_PRE2:  ESTM(*val = rd_txfir_pre2_tap  ()); break;
    case  PEREGRINE5_PC_TX_AFE_PRE1:  ESTM(*val = rd_txfir_pre1_tap  ()); break;
    case  PEREGRINE5_PC_TX_AFE_MAIN:  ESTM(*val = rd_txfir_main_tap  ()); break;
    case  PEREGRINE5_PC_TX_AFE_POST1: ESTM(*val = rd_txfir_post1_tap ()); break;
    case  PEREGRINE5_PC_TX_AFE_POST2: ESTM(*val = rd_txfir_post2_tap ()); break;
    default:
        return (peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_validate_full_txfir_cfg(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, peregrine5_pc_txfir_st *txfir) {
    uint8_t tap_num;
    err_code_t err_code = ERR_CODE_NONE;
    uint16_t abs_total = 0;
    uint8_t  in_nrz_range;
    int16_t tap_min, tap_max;
    UNUSED(sa__);

    if (txfir == NULL) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    in_nrz_range = ((enable_taps == PEREGRINE5_PC_NRZ_LP_3TAP) || (enable_taps == PEREGRINE5_PC_NRZ_6TAP)) ? 1 : 0;
    if ((enable_taps == PEREGRINE5_PC_NRZ_6TAP) || (enable_taps == PEREGRINE5_PC_PAM4_6TAP)) {
        tap_num=TXFIR_ST_NUM_TAPS;
    }
    else {
        tap_num=3;
    }
    tap_min =  in_nrz_range ? TXFIR_NRZ_TAP_MIN : TXFIR_PAM4_SW_TAP_MIN;
    tap_max =  in_nrz_range ? TXFIR_NRZ_TAP_MAX : TXFIR_PAM4_SW_TAP_MAX;
    if((txfir->pre1  < tap_min) || (txfir->pre1  > tap_max) ||
       (txfir->main  < tap_min) || (txfir->main  > tap_max) ||
       (txfir->post1 < tap_min) || (txfir->post1 > tap_max)) { 
        EFUN_PRINTF(("ERROR: TXFIR main tap invalid\n"));
        err_code |= ERR_CODE_TXFIR_MAIN_INVALID;
    }
    abs_total = (uint16_t)(SRDS_ABS(txfir->pre1) + SRDS_ABS(txfir->main) + SRDS_ABS(txfir->post1));
    if(tap_num == 6) {
        if((txfir->pre3  < tap_min) || (txfir->pre3  > tap_max) ||
           (txfir->pre2  < tap_min) || (txfir->pre2  > tap_max) ||
           (txfir->post2 < tap_min) || (txfir->post2 > tap_max)) { 
            EFUN_PRINTF(("ERROR: TXFIR main tap invalid\n"));
            err_code |= ERR_CODE_TXFIR_MAIN_INVALID;
        }
        abs_total += (uint16_t)(SRDS_ABS(txfir->pre3) + SRDS_ABS(txfir->pre2) + SRDS_ABS(txfir->post2));
    }
    if (abs_total > (in_nrz_range ? TXFIR_NRZ_SUM_LIMIT: TXFIR_PAM4_SW_SUM_LIMIT)) {
        EFUN_PRINTF(("ERROR: TXFIR sum limit failed %d\n",abs_total));
        err_code |= ERR_CODE_TXFIR_SUM_LIMIT;
    }
    return (peregrine5_pc_error(sa__, err_code));
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_apply_full_txfir_cfg(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, peregrine5_pc_txfir_st *txfir) {
    INIT_SRDS_ERR_CODE

    if (txfir == NULL) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    EFUN(wr_txfir_nrz_tap_range_sel(((enable_taps == PEREGRINE5_PC_NRZ_LP_3TAP) || (enable_taps == PEREGRINE5_PC_NRZ_6TAP)) ? 1 : 0));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_PRE1, txfir->pre1));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_MAIN, txfir->main));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_POST1, txfir->post1));
    if ((enable_taps == PEREGRINE5_PC_NRZ_6TAP) || (enable_taps == PEREGRINE5_PC_PAM4_6TAP))
    {
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_PRE3, txfir->pre3));
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_PRE2, txfir->pre2));
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_set_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_POST2, txfir->post2));
    }
    EFUN(wr_txfir_pre1_tap_en(0x1));
    EFUN(wr_txfir_main_tap_en(0x1));
    EFUN(wr_txfir_post1_tap_en(0x1));
    if ((enable_taps == PEREGRINE5_PC_NRZ_6TAP) || (enable_taps == PEREGRINE5_PC_PAM4_6TAP)) {
        EFUN(wr_txfir_pre3_tap_en(0x1));
        EFUN(wr_txfir_pre2_tap_en(0x1));
        EFUN(wr_txfir_post2_tap_en(0x1));
    }
    else {
        EFUN(wr_txfir_pre3_tap_en(0x0));
        EFUN(wr_txfir_pre2_tap_en(0x0));
        EFUN(wr_txfir_post2_tap_en(0x0));
    }
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_load_txfir_taps(sa__));
    return (ERR_CODE_NONE);
}

static err_code_t _plp_aperta2_peregrine5_pc_INTERNAL_get_nlc_txfir_min_and_max(peregrine5_pc_txfir_st *txfir, peregrine5_pc_txfir_st *txfir_p3, peregrine5_pc_txfir_st *txfir_m3, int32_t *min, int32_t *max);
static err_code_t _plp_aperta2_peregrine5_pc_INTERNAL_get_nlc_txfir_min_and_max(peregrine5_pc_txfir_st *txfir, peregrine5_pc_txfir_st *txfir_p3, peregrine5_pc_txfir_st *txfir_m3, int32_t *min, int32_t *max) {
    if(txfir->pre3  > 0) {
        *max =  3 * txfir->pre3 + txfir_p3->pre3;
        *min = -3 * txfir->pre3 + txfir_m3->pre3;
    }
    else {
        *max = -3 * txfir->pre3 + txfir_m3->pre3;
        *min =  3 * txfir->pre3 + txfir_p3->pre3;
    }
    if(txfir->pre2  > 0) {
        *max +=  3 * txfir->pre2 + txfir_p3->pre2;
        *min += -3 * txfir->pre2 + txfir_m3->pre2;
    }
    else {
        *max += -3 * txfir->pre2 + txfir_m3->pre2;
        *min +=  3 * txfir->pre2 + txfir_p3->pre2;
    }
    if(txfir->pre1  > 0) {
        *max +=  3 * txfir->pre1 + txfir_p3->pre1;
        *min += -3 * txfir->pre1 + txfir_m3->pre1;
    }
    else {
        *max += -3 * txfir->pre1 + txfir_m3->pre1;
        *min +=  3 * txfir->pre1 + txfir_p3->pre1;
    }
    if(txfir->main  > 0) {
        *max +=  3 * txfir->main + txfir_p3->main;
        *min += -3 * txfir->main + txfir_m3->main;
    }
    else {
        *max += -3 * txfir->main + txfir_m3->main;
        *min +=  3 * txfir->main + txfir_p3->main;
    }
    if(txfir->post1  > 0) {
        *max +=  3 * txfir->post1 + txfir_p3->post1;
        *min += -3 * txfir->post1 + txfir_m3->post1;
    }
    else {
        *max += -3 * txfir->post1 + txfir_m3->post1;
        *min +=  3 * txfir->post1 + txfir_p3->post1;
    }
    if(txfir->post2  > 0) {
        *max +=  3 * txfir->post2 + txfir_p3->post2;
        *min += -3 * txfir->post2 + txfir_m3->post2;
    }
    else {
        *max += -3 * txfir->post2 + txfir_m3->post2;
        *min +=  3 * txfir->post2 + txfir_p3->post2;
    }

    return ERR_CODE_NONE;
}

static err_code_t _plp_aperta2_peregrine5_pc_INTERNAL_get_dc_adjust_from_min_max(int32_t min_val, int32_t max_val, int16_t *dc_adjust);
static err_code_t _plp_aperta2_peregrine5_pc_INTERNAL_get_dc_adjust_from_min_max(int32_t min_val, int32_t max_val, int16_t *dc_adjust) {
    *dc_adjust = (int16_t)(-((min_val + max_val)>>1));
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_nlc_txfir_dc_adjust(srds_access_t *sa__, peregrine5_pc_txfir_st *txfir, peregrine5_pc_txfir_st *txfir_p3, peregrine5_pc_txfir_st *txfir_m3, int16_t *dc_adjust) {
    INIT_SRDS_ERR_CODE
    int32_t max_val = 0, min_val = 0;

    EFUN(_plp_aperta2_peregrine5_pc_INTERNAL_get_nlc_txfir_min_and_max(txfir, txfir_p3, txfir_m3, &min_val, &max_val));
    EFUN(_plp_aperta2_peregrine5_pc_INTERNAL_get_dc_adjust_from_min_max(min_val, max_val, dc_adjust));
    return ERR_CODE_NONE;
}

#define CHECK_NLC_TAP_MIN_MAX(_tap) ((_tap < TXFIR_NLC_SW_TAP_MIN) || (_tap > TXFIR_NLC_SW_TAP_MAX))

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_validate_full_nlc_txfir_cfg(srds_access_t *sa__, enum peregrine5_pc_txfir_tap_enable_enum enable_taps, int8_t nlc_upper_eye_pct, int8_t nlc_lower_eye_pct, peregrine5_pc_txfir_st *txfir, peregrine5_pc_txfir_st *txfir_p3, peregrine5_pc_txfir_st *txfir_m3) {
    INIT_SRDS_ERR_CODE
    int32_t max_val = 0, min_val = 0;
    int16_t dc_adjust = 0;
    err_code_t err_code = ERR_CODE_NONE;

    if((nlc_upper_eye_pct > 100) || (nlc_upper_eye_pct < -100) || (nlc_lower_eye_pct > 100) || (nlc_lower_eye_pct < -100)) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }
    if((enable_taps !=  PEREGRINE5_PC_PAM4_6TAP)) {
        return ERR_CODE_INVALID_MODE;
    }

    EFUN(_plp_aperta2_peregrine5_pc_INTERNAL_get_nlc_txfir_min_and_max(txfir, txfir_p3, txfir_m3, &min_val, &max_val));
    EFUN(_plp_aperta2_peregrine5_pc_INTERNAL_get_dc_adjust_from_min_max(min_val, max_val, &dc_adjust));

    if ((max_val - min_val) > 1023) {
        EFUN_PRINTF(("ERROR: TXFIR taps too large for NLC: max_val (%d) - min_val (%d) > 1023\n", max_val, min_val));
        err_code |= ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

    if((CHECK_NLC_TAP_MIN_MAX(txfir_p3->pre3)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_p3->pre2)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_p3->pre1)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_p3->main)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_p3->post1)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_p3->post2))) {
        EFUN_PRINTF(("ERROR: TXFIR nlc_upper_eye_pct too large. Taps must be within [%d,%d]. One or more taps are invalid: (%d,%d,%d,%d,%d,%d)\n",
                    TXFIR_NLC_SW_TAP_MIN, TXFIR_NLC_SW_TAP_MAX, txfir_p3->pre3, txfir_p3->pre2, txfir_p3->pre1, txfir_p3->main, txfir_p3->post1, txfir_p3->post2));
        err_code |= ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }


    if((CHECK_NLC_TAP_MIN_MAX(txfir_m3->pre3)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_m3->pre2)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_m3->pre1)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_m3->main)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_m3->post1)) ||
       (CHECK_NLC_TAP_MIN_MAX(txfir_m3->post2))) {
        EFUN_PRINTF(("ERROR: TXFIR nlc_lower_eye_pct too large. Taps must be within [%d,%d]. One or more taps are invalid: (%d,%d,%d,%d,%d,%d)\n",
                    TXFIR_NLC_SW_TAP_MIN, TXFIR_NLC_SW_TAP_MAX, txfir_m3->pre3, txfir_m3->pre2, txfir_m3->pre1, txfir_m3->main, txfir_m3->post1, txfir_m3->post2));
        err_code |= ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }
    if(CHECK_NLC_TAP_MIN_MAX(dc_adjust)) {
        EFUN_PRINTF(("ERROR: TXFIR NLC asymmetry is too large. Must be within [%d,%d]. Actual value:%d\n", TXFIR_NLC_SW_TAP_MIN, TXFIR_NLC_SW_TAP_MAX, dc_adjust));
        err_code |= ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

    return (peregrine5_pc_error(sa__, err_code));
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_apply_nlc_txfir_cfg(srds_access_t *sa__, peregrine5_pc_txfir_st *txfir_p3, peregrine5_pc_txfir_st *txfir_m3, int16_t dc_adjust) {
    INIT_SRDS_ERR_CODE

    EFUN(wr_txfir_pre3_tap_p3_lvl_adjust((uint8_t)txfir_p3->pre3));
    EFUN(wr_txfir_pre2_tap_p3_lvl_adjust((uint8_t)txfir_p3->pre2));
    EFUN(wr_txfir_pre1_tap_p3_lvl_adjust((uint8_t)txfir_p3->pre1));
    EFUN(wr_txfir_main_tap_p3_lvl_adjust((uint8_t)txfir_p3->main));
    EFUN(wr_txfir_post1_tap_p3_lvl_adjust((uint8_t)txfir_p3->post1));
    EFUN(wr_txfir_post2_tap_p3_lvl_adjust((uint8_t)txfir_p3->post2));

    EFUN(wr_txfir_pre3_tap_m3_lvl_adjust((uint8_t)txfir_m3->pre3));
    EFUN(wr_txfir_pre2_tap_m3_lvl_adjust((uint8_t)txfir_m3->pre2));
    EFUN(wr_txfir_pre1_tap_m3_lvl_adjust((uint8_t)txfir_m3->pre1));
    EFUN(wr_txfir_main_tap_m3_lvl_adjust((uint8_t)txfir_m3->main));
    EFUN(wr_txfir_post1_tap_m3_lvl_adjust((uint8_t)txfir_m3->post1));
    EFUN(wr_txfir_post2_tap_m3_lvl_adjust((uint8_t)txfir_m3->post2));
    EFUN(wr_txfir_dc_adjust((uint8_t)dc_adjust));

    return (ERR_CODE_NONE);
}




err_code_t plp_aperta2_peregrine5_pc_INTERNAL_core_clkgate(srds_access_t *sa__, uint8_t enable) {
    UNUSED(sa__);
    UNUSED(enable);
    return (ERR_CODE_NONE);
}
#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_lane_clkgate(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE

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


uint16_t plp_aperta2_peregrine5_pc_INTERNAL_eye_to_mUI(srds_access_t *sa__, uint8_t var) {
    UNUSED(sa__);
    /* var is in units of 1/512 th UI, so need to multiply by 1000/512 */
    return (uint16_t)(((uint16_t)var)*125/64);
}

uint16_t plp_aperta2_peregrine5_pc_INTERNAL_eye_to_mV(srds_access_t *sa__, uint8_t var, uint8_t ladder_range) {
    INIT_SRDS_ERR_CODE
    int16_t nlmv_val = 0;
    uint16_t vl = 0;
    afe_override_slicer_sel_t slicer = INVALID_SLICER;
    /* find nearest two vertical levels */
    slicer = LMS_SLICER;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_ladder_setting_to_mV(sa__, (int16_t)var, ladder_range, slicer, &nlmv_val));
    vl = (uint16_t)nlmv_val;
    return (vl);
}


/*----------------------------------------------------*/
/*  Get dynamic eye margin estimation values (PAM)    */
/*----------------------------------------------------*/
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_pam_eye_margin_est(srds_access_t *sa__, peregrine5_pc_eye_margin_t *eye_margin) {
    INIT_SRDS_ERR_CODE
  uint8_t ladder_range = 0;
  uint8_t pam_eye_val = 0;
  uint8_t i;
  for(i=0; i<NUM_PAM_EYES; i++) {
      eye_margin->left_eye_mUI [i] = 0;
      eye_margin->right_eye_mUI [i] = 0;
      ESTM(pam_eye_val = (uint8_t)rdv_usr_sts_veye_margin(i));
      ESTM(eye_margin->upper_eye_mV [i] = plp_aperta2_peregrine5_pc_INTERNAL_eye_to_mV (sa__, pam_eye_val, ladder_range));
      ESTM(eye_margin->lower_eye_mV [i] = plp_aperta2_peregrine5_pc_INTERNAL_eye_to_mV (sa__, pam_eye_val, ladder_range));
  }
  return (ERR_CODE_NONE);
}

/*------------------------------------------------------*/
/*  Print dynamic eye margin estimation values (PAM)    */
/*------------------------------------------------------*/
err_code_t plp_aperta2_peregrine5_pc_display_pam_eye_margin_est(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    uint8_t i;
    peregrine5_pc_eye_margin_t eye_margin = {{0},{0},{0},{0}};
    enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;
    struct peregrine5_pc_usr_ctrl_disable_functions_st dsu, dss;
    uint8_t disable_display=0;

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
    EFUN(plp_aperta2_peregrine5_pc_get_usr_ctrl_disable_startup(sa__, &dsu));
    EFUN(plp_aperta2_peregrine5_pc_get_usr_ctrl_disable_steady_state(sa__, &dss));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_pam_eye_margin_est(sa__, &eye_margin));
    disable_display = (pam_mode == NRZ) || (dsu.field.eye_margin_estimation) || (dss.field.eye_margin_estimation);

    EFUN_PRINTF(("Eye margin: L(mUI),R(mUI),U(mV),D(mV)\n"));
    for(i=NUM_PAM_EYES; i>0; i--) {
        EFUN_PRINTF(("%s eye: ", (i-1)==2?"Upper ":(i-1)==1?"Middle":"Lower "));
        if (disable_display) {
            EFUN_PRINTF(("  x   ,  x   ,  x  ,  x  \n"));
        }
        else {
            uint8_t ffe_enabled;
            EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_ffe_enabled(sa__, &ffe_enabled));
            if (ffe_enabled) {
                EFUN_PRINTF(("  x   ,  x   ,%5d,%5d\n", eye_margin.upper_eye_mV[i-1], eye_margin.lower_eye_mV[i-1]));
            }
            else {
                EFUN_PRINTF(("%6d,%6d,%5d,%5d\n", eye_margin.left_eye_mUI[i-1], eye_margin.right_eye_mUI[i-1], eye_margin.upper_eye_mV[i-1], eye_margin.lower_eye_mV[i-1]));
            }
        }
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_osr_mode(srds_access_t *sa__, peregrine5_pc_osr_mode_st *imode) {
    INIT_SRDS_ERR_CODE
    peregrine5_pc_osr_mode_st mode;

    ENULL_MEMSET(&mode, 0, sizeof(peregrine5_pc_osr_mode_st));

    if(!imode) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ESTM(mode.tx = rd_tx_osr_mode());
    ESTM(mode.rx = rd_rx_osr_mode());
    mode.tx_rx = 255;
    *imode = mode;
    return (ERR_CODE_NONE);
}


/** A simple decoding structure
 *
 */
struct simpleDecoder
{
    int32_t  name;
    uint32_t val;
};

uint32_t plp_aperta2_peregrine5_pc_osr_mode_enum_to_int_x1000(uint8_t osr_mode) {
    int32_t c;
    struct simpleDecoder dec[] = INIT_OSR_ASSOC_TABLE;

    for(c = 0; c < (int32_t)(sizeof(dec)/sizeof(struct simpleDecoder)); c++)
    {
        if(osr_mode == dec[c].name)
        {
            return(dec[c].val);
        }
    }
    return(1000);

}
const char* plp_aperta2_peregrine5_pc_INTERNAL_get_osr_str(uint8_t osr_val, uint8_t osr_str_length) {
    const char* e2s_long_osr_mode_enum[35] = {
      "x1   ",
      "x2   ",
      "ERR  ",
      "x5   ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "x33  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "x41P2",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "ERR  ",
      "x42P5",
      "ERR  ",
    };

    const char* e2s_short_osr_mode_enum[35] = {
      "x1",
      "x2",
      "ER",
      "x5",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "xU",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "xO",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "ER",
      "xP",
      "ER",
    };
    switch(osr_str_length) {
        case (GET_OSR_STR_LONG):
            return ((osr_val < sizeof(e2s_long_osr_mode_enum)/sizeof(e2s_long_osr_mode_enum[0])) ? e2s_long_osr_mode_enum[osr_val] : "ERR  ");
        case (GET_OSR_STR_SHORT):
            return ((osr_val < sizeof(e2s_short_osr_mode_enum)/sizeof(e2s_short_osr_mode_enum[0])) ? e2s_short_osr_mode_enum[osr_val] : "ER");
        default:
            return "ER";
    }
}
#endif /* SMALL_FOOTPRINT */

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(srds_access_t *sa__, enum peregrine5_pc_rx_pam_mode_enum *pmode) {
    INIT_SRDS_ERR_CODE
    uint8_t rx_pam4_mode;
    uint8_t rx_pam4_er_mode;

    ESTM(rx_pam4_mode    = rd_rx_pam4_mode());
    ESTM(rx_pam4_er_mode = rd_rx_pam4_er_mode());

    if (rx_pam4_mode == 0) {
        *pmode = NRZ;
    }
    else if (rx_pam4_mode == 1) {
        *pmode = (rx_pam4_er_mode) ? PAM4_ER : PAM4_NR;
    }
    else {
        return(peregrine5_pc_error(sa__, ERR_CODE_INVALID_RX_PAM_MODE));
    }

  return (ERR_CODE_NONE);
}

#ifndef SMALL_FOOTPRINT

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_decode_br_os_mode(srds_access_t *sa__, uint8_t *br_pd_en) {
    INIT_SRDS_ERR_CODE
    ESTM(*br_pd_en = (rd_cdr_pd_mode() > 1)? 1:0);
    return (ERR_CODE_NONE);
}

#endif /* SMALL_FOOTPRINT */

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_sigdet_status(srds_access_t *sa__, uint8_t *sig_det, uint8_t *sig_det_chg) {
    INIT_SRDS_ERR_CODE
    uint16_t rddata;

    ESTM(rddata = reg_rd_SIGDET_SDSTATUS_0());
    *sig_det = ex_SIGDET_SDSTATUS_0__signal_detect(rddata);
    *sig_det_chg = (uint8_t)(ex_SIGDET_SDSTATUS_0__signal_detect_change(rddata));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_pll_lock_status(srds_access_t *sa__, uint8_t *pll_lock, uint8_t *pll_lock_chg) {
    INIT_SRDS_ERR_CODE
    uint16_t rddata;
    uint8_t pll_fail;
    ESTM(rddata = reg_rdc_PLL_CAL_COM_STS_8());
    *pll_lock = (uint8_t)(exc_PLL_CAL_COM_STS_8__pll_lock(rddata));
    pll_fail = (uint8_t)(exc_PLL_CAL_COM_STS_8__pll_fail_stky(rddata));
    *pll_lock_chg = (uint8_t)((*pll_lock ^ !exc_PLL_CAL_COM_STS_8__pll_lock_bar_stky(rddata)) | (*pll_lock ^ !pll_fail));
    return(ERR_CODE_NONE);
}
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_poll_pll_lock(srds_access_t *sa__, uint16_t timeout_ms) {
    INIT_SRDS_ERR_CODE
    uint8_t pll_lock = 0;
    uint8_t pll_lock_chg = 0;
    uint8_t loop = 0;

    for(loop=0;loop < 100; loop++) {
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_pll_lock_status(sa__, &pll_lock, &pll_lock_chg));
        if(pll_lock) {
            return ERR_CODE_NONE;
        }
        if(loop>10) {
            EFUN(USR_DELAY_US(10*timeout_ms));
        }
    }
    return ERR_CODE_POLLING_TIMEOUT;
}

#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_pmd_lock_status(srds_access_t *sa__, uint8_t *pmd_lock, uint8_t *pmd_lock_chg) {
    INIT_SRDS_ERR_CODE
    uint16_t rddata;
    ESTM(rddata = reg_rd_TLB_RX_DBG_PMD_RX_LOCK_STATUS());
    *pmd_lock = ex_TLB_RX_DBG_PMD_RX_LOCK_STATUS__dbg_pmd_rx_lock(rddata);
    *pmd_lock_chg = (uint8_t)(ex_TLB_RX_DBG_PMD_RX_LOCK_STATUS__dbg_pmd_rx_lock_change(rddata));
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_tx_ppm(srds_access_t *sa__, int16_t *tx_ppm) {
    INIT_SRDS_ERR_CODE
    /* tx_ppm = register/10.486                     */
    /* 3125/32768 = 1/10.486                        */
    /* tx_ppm = register*3125/32768                 */
    /* Suppose you want x/N, but with rounding then */
    /* tx_ppm rounded = (x+N/2)/N;                  */

    ESTM(*tx_ppm = (int16_t)(((rd_tx_pi_integ2_reg() * 3125) + (32768 / 2)) / 32768));
    return ERR_CODE_NONE;
}


err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_ppm(srds_access_t *sa__, int16_t *rx_ppm) {
    INIT_SRDS_ERR_CODE
    int16_t divisor;
    enum peregrine5_pc_osr_mode_enum rx_osr_mode = PEREGRINE5_PC_OSR_UNINITIALIZED;
    EFUN(plp_aperta2_peregrine5_pc_get_rx_osr_mode(sa__, &rx_osr_mode));
    divisor = ((rx_osr_mode == PEREGRINE5_PC_OSX2) ? 42 : ((rx_osr_mode == PEREGRINE5_PC_OSX5) ? 105 : 84));
    ESTM(*rx_ppm = rd_cdr_integ_reg()/divisor);
    return ERR_CODE_NONE;
}


err_code_t plp_aperta2_peregrine5_pc_INTERNAL_read_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *istate) {
   INIT_SRDS_ERR_CODE

    if(!istate) {
      return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    EFUN(plp_aperta2_peregrine5_pc_gen_collect_peregrine5_pc_lane_state_st(sa__, istate));

    return (ERR_CODE_NONE);
}

/*------------------------------------*/
/*  Read Lane state Helper Functions  */
/*------------------------------------*/
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_resume_micro_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    if (!st->stop_state || (st->stop_state == 0xFF) || (st->stop_state == DB_STOPPED)) {
        /* manually check error code instead of EFUN*/
        err_code_t resume_status = ERR_CODE_NONE;
        resume_status = plp_aperta2_peregrine5_pc_stop_rx_adaptation(sa__, 0);
        if (resume_status) {
            USR_PRINTF(("WARNING: Resuming micro after lane state capture failed \n"));
        }
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_stop_state_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    err_code_t err_code = 0;
    st->stop_state = plp_aperta2_peregrine5_pc_INTERNAL_stop_micro(sa__,st->pmd_lock,&err_code);

    if(err_code) USR_PRINTF(("WARNING: Unable to stop microcontroller,  following data is suspect\n"));
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_pmd_lock_chg_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    INIT_SRDS_ERR_CODE
    uint8_t rx_lock_at_end=0, rx_lock_chg_at_end=0;

    /* read lock status at end and combine them to handle transient cases */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_pmd_lock_status(sa__, &rx_lock_at_end, &rx_lock_chg_at_end));
    if (st->pmd_lock != rx_lock_at_end) {
        USR_PRINTF(("WARNING: Lane %d rx_lock status changed while reading lane state (rx_lock_start=%d, rx_lock_chg_start=%d, rx_lock_at_end=%d, rx_lock_chg_at_end=%d) \n",
                    plp_aperta2_peregrine5_pc_acc_get_lane(sa__), st->pmd_lock, st->pmd_lock_chg, rx_lock_at_end, rx_lock_chg_at_end));
    }
    st->pmd_lock &= rx_lock_at_end;
    st->pmd_lock_chg |= rx_lock_chg_at_end;
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_osr_str_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    const char *tx;
    UNUSED(sa__);

    if(st->osr_mode.tx_rx != 255) {
        tx = plp_aperta2_peregrine5_pc_INTERNAL_get_osr_str(st->osr_mode.tx_rx, GET_OSR_STR_LONG);
        USR_SNPRINTF(st->tx_osr_mode_str, sizeof(st->tx_osr_mode_str), "%s", tx);
    }
    else {
        const char *rx;
        tx = plp_aperta2_peregrine5_pc_INTERNAL_get_osr_str(st->osr_mode.tx, GET_OSR_STR_SHORT);
        rx = plp_aperta2_peregrine5_pc_INTERNAL_get_osr_str(st->osr_mode.rx, GET_OSR_STR_SHORT);
        USR_SNPRINTF(st->tx_osr_mode_str, sizeof(st->tx_osr_mode_str), "%s", tx);
        USR_SNPRINTF(st->rx_osr_mode_str, sizeof(st->rx_osr_mode_str), "%s", rx);
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_uc_cfg_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    INIT_SRDS_ERR_CODE
    struct peregrine5_pc_uc_lane_config_st lane_cfg;

    EFUN(plp_aperta2_peregrine5_pc_get_uc_lane_cfg(sa__, &lane_cfg));
    st->UC_CFG = lane_cfg.word;

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_uc_sts_decoded_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    INIT_SRDS_ERR_CODE
    const char * const prepend = "    ";
    if(st->UC_STS != 0) {
        EFUN(plp_aperta2_peregrine5_pc_decode_uc_sts(sa__, st->UC_STS, st->uc_sts_decoded, sizeof(st->uc_sts_decoded), prepend));
    }
    if(st->UC_STS_EXT != 0) {
        EFUN(plp_aperta2_peregrine5_pc_decode_uc_sts_ext(sa__, st->UC_STS_EXT, st->uc_sts_ext_decoded, sizeof(st->uc_sts_ext_decoded), prepend));
    }
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    INIT_SRDS_ERR_CODE
    enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));
    st->rx_pam_mode = (uint8_t)pam_mode;

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_disable_eye_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    INIT_SRDS_ERR_CODE
    struct peregrine5_pc_usr_ctrl_disable_functions_st dsu, dss;
    EFUN(plp_aperta2_peregrine5_pc_get_usr_ctrl_disable_startup(sa__, &dsu));
    EFUN(plp_aperta2_peregrine5_pc_get_usr_ctrl_disable_steady_state(sa__, &dss));
    st->disable_eye_display = (dsu.field.eye_margin_estimation) || (dss.field.eye_margin_estimation);
    return ERR_CODE_NONE;
}


err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_txfir_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    INIT_SRDS_ERR_CODE
    ESTM(st->txfir_use_pam4_range = rd_txfir_nrz_tap_range_sel() ? 0 : 1);
    ESTM(st->enable_6taps = rd_txfir_pre3_tap_en());
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_PRE1, &st->txfir.pre1));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_MAIN, &st->txfir.main));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_POST1, &st->txfir.post1));
    st->TXEQ_n1 = st->txfir.pre1;
    st->TXEQ_m  = st->txfir.main;
    st->TXEQ_p1 = st->txfir.post1;
    if(st->enable_6taps) {
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_PRE3, &st->txfir.pre3));
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_PRE2, &st->txfir.pre2));
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_tx_tap(sa__, PEREGRINE5_PC_TX_AFE_POST2, &st->txfir.post2));
        st->TXEQ_n3 = st->txfir.pre3;
        st->TXEQ_n2 = st->txfir.pre2;
        st->TXEQ_p2 = st->txfir.post2;
    }
    return ERR_CODE_NONE;
}


err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_rx_ffe_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
    INIT_SRDS_ERR_CODE

    ESTM(st->RXFFE_n3 = rdv_usr_sts_rxffe_taps_0());
    ESTM(st->RXFFE_n2 = rdv_usr_sts_rxffe_taps_1());
    ESTM(st->RXFFE_n1 = rdv_usr_sts_rxffe_taps_2());
    ESTM(st->RXFFE_m =  rdv_usr_sts_rxffe_taps_3());
    ESTM(st->RXFFE_p1 = rdv_usr_sts_rxffe_taps_4());
    ESTM(st->RXFFE_p2 = rdv_usr_sts_rxffe_taps_5());
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_tuningdone_time(srds_access_t *sa__, uint32_t *tuningdone_time) {
    INIT_SRDS_ERR_CODE
    uint32_t usr_sts_tuningdone_time = 0;
    if(tuningdone_time == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    ESTM(usr_sts_tuningdone_time = rdv_usr_sts_tuningdone_time());
    *tuningdone_time = (usr_sts_tuningdone_time<<PEREGRINE5_PC_LANE_TIMER_SHIFT)/10;
    return ERR_CODE_NONE;
}

#ifdef SERDES_API_FLOATING_POINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_snr_dfe(srds_access_t *sa__, USR_DOUBLE *snr) {
    INIT_SRDS_ERR_CODE
    uint32_t mse;
    ESTM(mse = rdv_usr_sts_mse());
    if(mse == 0 ){
        *snr = 999.0;
    }
    else {
        *snr = SERDES_MSE_TO_SNR(mse);
    }
    return ERR_CODE_NONE;
}
#endif
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_snr_dfe_lane_state(srds_access_t *sa__, peregrine5_pc_lane_state_st_t *st) {
#ifdef SERDES_API_FLOATING_POINT
    INIT_SRDS_ERR_CODE
    USR_DOUBLE snr=0.0;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_snr_dfe(sa__, &snr));
    if(snr >900.0 ){
        USR_SNPRINTF(st->snr_dfe, sizeof(st->snr_dfe)," ERR ");
    }
    else {
        USR_SNPRINTF(st->snr_dfe, sizeof(st->snr_dfe), "%5.2f", snr);
    }
#else
    USR_SNPRINTF(st->snr_dfe, sizeof(st->snr_dfe),"NFP  ");
#endif
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_link_time(srds_access_t *sa__, uint32_t *link_time) {
    INIT_SRDS_ERR_CODE
    uint32_t usr_sts_link_time = 0;
    if(link_time == NULL) {
        return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    ESTM(usr_sts_link_time = rdv_usr_sts_link_time());
    *link_time = (usr_sts_link_time<<PEREGRINE5_PC_LANE_TIMER_SHIFT)/10;
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_display_lane_state_no_newline(srds_access_t *sa__) {
  INIT_SRDS_ERR_CODE
  err_code_t Exit_Status = ERR_CODE_NONE;
  peregrine5_pc_lane_state_st_t *state;


  state = (peregrine5_pc_lane_state_st_t *)USR_CALLOC(1, sizeof(peregrine5_pc_lane_state_st_t));
  if(!state) {
      return (ERR_CODE_MEM_ALLOC_FAIL);
  }
  CFUN(plp_aperta2_peregrine5_pc_INTERNAL_read_lane_state(sa__, state));

  EFUN_PRINTF(("%2d ", state->laneid));


  if(state->osr_mode.tx_rx != 255) {
      EFUN_PRINTF(("(%s%s%2s%s, 0x%04x, 0x%02x ,", (state->tx_pmd_dp_invert ? "-" : "+"), (state->rx_pmd_dp_invert ? "-" : "+"),

                                                    ((state->CDR) ? "BR" : "OS"),

                                                    state->tx_osr_mode_str, state->UC_CFG,state->UC_STS));
  } else {
      EFUN_PRINTF(("(%s%s%3s%s,%2s%s:%s, 0x%04x, 0x%02x_%04x,", (state->tx_pmd_dp_invert ? "-" : "+"), (state->rx_pmd_dp_invert ? "-" : "+"),
                    ((state->rx_pam_mode == NRZ) ? "NRZ" : ((state->rx_pam_mode == PAM4_NR) ? "P4N": "P4E")), ((state->tx_pam_mode == (state->rx_pam_mode > 0)) ? " ": "~"),

                    ((state->CDR) ? "BR" : "OS"),

                    state->tx_osr_mode_str, state->rx_osr_mode_str, state->UC_CFG, state->UC_STS, state->UC_STS_EXT));
  }
  EFUN_PRINTF((" %01x,%01x, %02x )",state->tx_rst,state->rx_rst,state->stop_state));
  EFUN_PRINTF((" %1d%s", state->sig_det, state->sig_det_chg ? "*" : " "));

  EFUN_PRINTF((" %1d%s", state->pmd_lock, state->pmd_lock_chg ? "*" : " "));
  EFUN_PRINTF(("%s", state->pmd_lock ? (state->rx_tuning_done ? " " : "+") : " "));
  EFUN_PRINTF(("%4d ", state->ppm));

  EFUN_PRINTF((" (%2d,%2d,%2d)  ", state->PF_M, state->PF_L, state->PF_H));

  EFUN_PRINTF(("%2d ", state->VGA));

  EFUN_PRINTF(("%4d ", state->DCO));

  EFUN_PRINTF(("(%2d,%2d,",
              state->TP0,
              state->TP1
             ));
  if((state->TP2 > 2) && (state->TP2 < 6)) {
      EFUN_PRINTF((" %c)", ((state->TP2 == 3) ? 'L' : ((state->TP2 == 4) ? 'H' : 'Q'))));
  }
  else {
      EFUN_PRINTF(("%2d)", state->TP2));
  }
  EFUN_PRINTF((" (%4d,%4d,%5d,%4d,%5d,%4d)  ", state->RXFFE_n3, state->RXFFE_n2, state->RXFFE_n1, state->RXFFE_m, state->RXFFE_p1, state->RXFFE_p2));
  if (state->dfe_taps[0] == 64) {
      EFUN_PRINTF(("( x,%2d) ", state->dfe_taps[1]));
  }
  else {
      EFUN_PRINTF(("(%2d,%2d) ", state->dfe_taps[0], state->dfe_taps[1]));
  }
  EFUN_PRINTF(("(%3d,%3d)", state->FLT_M, state->FLT_S));
  EFUN_PRINTF((" %4d%c ", state->tx_ppm, state->tx_pi_en ? '*' : ' '));

  if(state->tx_disable_status){
      EFUN_PRINTF((" D"));
  }else{
      EFUN_PRINTF(("  "));
  }

  if(state->tx_prec_en){
      EFUN_PRINTF(("P"));
  }else{
      EFUN_PRINTF((" "));
  }

  if(state->rmt_lpbk_en){
      EFUN_PRINTF(("R"));
  }else if(state->dig_lpbk_en){
      EFUN_PRINTF(("L"));
  }else if(state->ilb_en){
      EFUN_PRINTF(("I"));
  }else if(state->linktrn_en){
      EFUN_PRINTF(("T"));
  }else{
      EFUN_PRINTF((" "));
  }

  if((state->txfir_nlc_upper_pct != 0) || (state->txfir_nlc_lower_pct != 0)){
      EFUN_PRINTF(("N"));
  }else{
      EFUN_PRINTF((" "));
  }

  if (!state->enable_6taps) {
      EFUN_PRINTF(("( x , x ,%3d,%3d,%3d, x ) ", state->txfir.pre1, state->txfir.main, state->txfir.post1));
  }
  else {
      EFUN_PRINTF(("(%3d,%3d,%3d,%3d,%3d,%3d) ",
                   state->txfir.pre3, state->txfir.pre2, state->txfir.pre1,
                   state->txfir.main, state->txfir.post1, state->txfir.post2));

  }
  if (state->disable_eye_display) {
      EFUN_PRINTF(("( x , x , x ) "));
  }
  else {
      if (state->rx_pam_mode == NRZ) {
          EFUN_PRINTF(("(%3d, x , x ) ", state->EYE_L));
      }
      else {
          EFUN_PRINTF(("(%3d,%3d,%3d) ", state->EYE_U, state->EYE_M, state->EYE_L));
      }
  }
  /* Check to see if link_time is max value after 80us to 0.1msec unit conversion */
  if (state->link_time == ((0xFFFFL<<PEREGRINE5_PC_LANE_TIMER_SHIFT)/10)) {
      EFUN_PRINTF((" >%4d.%01d", state->link_time/10, state->link_time%10));
      EFUN_PRINTF(("  "));
  } else {
      EFUN_PRINTF((" %4d.%01d", state->link_time/10, state->link_time%10));
      EFUN_PRINTF(("   "));
  }
  EFUN_PRINTF((" %s ",state->snr_dfe));

  EFUN_PRINTF(("%s",state->BER));


  if(state->UC_STS != 0) {
      EFUN_PRINTF(("\n"));
      EFUN_PRINTF(("%s", state->uc_sts_decoded));
  }

  if(state->UC_STS_EXT != 0) {
      EFUN_PRINTF(("%s", state->uc_sts_ext_decoded));
  }

Exit:
  USR_FREE(state);
  if(Exit_Status != ERR_CODE_NONE) {
      return peregrine5_pc_error_report(sa__, Exit_Status);
  }

  return (ERR_CODE_NONE);
}  /* plp_aperta2_peregrine5_pc_INTERNAL_display_lane_state_no_newline(srds_access_t *sa__) */



err_code_t plp_aperta2_peregrine5_pc_INTERNAL_read_core_state(srds_access_t *sa__, peregrine5_pc_core_state_st *istate) {
  INIT_SRDS_ERR_CODE
  peregrine5_pc_core_state_st state;
  struct peregrine5_pc_uc_core_config_st core_cfg = UC_CORE_CONFIG_INIT;
  uint8_t tmon_num_rshift = 2;

  if(!istate) {
    return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ENULL_MEMSET(&state, 0, sizeof(peregrine5_pc_core_state_st));

  EFUN(plp_aperta2_peregrine5_pc_get_uc_core_config(sa__, &core_cfg));

  {
      uint8_t rescal_frc;
      ESTM(rescal_frc = rdc_rescal_frc());
      if (1 == rescal_frc) {
          /* rescal_frc_val is read, since the rescal value is overriden. */
          ESTM(state.rescal = rdc_rescal_frc_val());
      } else {
          ESTM(state.rescal = rd_tx_sts_ana_rescal());
      }
  }

  ESTM(state.core_reset           = rdc_core_dp_reset_state());
  ESTM(state.uc_active            = rdc_uc_active());
  ESTM(state.comclk_mhz           = rdc_heartbeat_count_1us());
  ESTM(state.pll_pwrdn            = rdc_pll_pwrdn_or());
  ESTM(state.ucode_version        = rdcv_common_ucode_version());
  ESTM(state.ucode_minor_version  = rdcv_common_ucode_minor_version());
  EFUN(plp_aperta2_peregrine5_pc_version(sa__, &state.api_version));
  ESTM(state.afe_hardware_version = rdcv_afe_hardware_version());
  ESTM(state.temp_idx             = rdcv_temp_idx());
  EFUN(plp_aperta2_peregrine5_pc_read_die_temperature(sa__, &state.die_temp));
  if((((state.ucode_version & 0x0FFF) == 0x002) && (state.ucode_minor_version == 0x0)) ||
      ((state.ucode_version & 0x0FFF) < 0x002)) {
    tmon_num_rshift = 3;
  }
  ESTM(state.avg_tmon             = (int16_t)(_bin_to_degC(rdcv_avg_tmon_reg13bit()>>tmon_num_rshift)));
  state.vco_rate_mhz              = (uint16_t)core_cfg.vco_rate_in_Mhz;
  EFUN(plp_aperta2_peregrine5_pc_INTERNAL_read_pll_div(sa__, &state.pll_div));
  EFUN(plp_aperta2_peregrine5_pc_INTERNAL_pll_lock_status(sa__, &state.pll_lock, &state.pll_lock_chg));
  ESTM(state.analog_vco_range     = rdc_pll_range());
  ESTM(state.core_status          = rdcv_status_byte());
  *istate = state;
  return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_display_core_state_no_newline(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
  peregrine5_pc_core_state_st state;
    ENULL_MEMSET(&state     , 0, sizeof(state     ));
    {
        err_code_t Exit_Status = ERR_CODE_NONE;
        uint8_t pll_orig;
        ESTM(pll_orig = plp_aperta2_peregrine5_pc_acc_get_pll_idx(sa__));
        CFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, 0));
        CFUN(plp_aperta2_peregrine5_pc_INTERNAL_read_core_state(sa__, &state));
Exit:
        EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, pll_orig));
        if(Exit_Status != ERR_CODE_NONE) {
            return peregrine5_pc_error_report(sa__, Exit_Status);
        }
    }

    if ((state.avg_tmon<-50)||(state.avg_tmon>135)) {
      EFUN_PRINTF(("\n*** WARNING: Core die temperature (AVG_TMON) out of bounds -50C to 130C\n"));
    }
    if ((state.rescal < RESCAL_MIN) || (state.rescal > RESCAL_MAX)) {
      EFUN_PRINTF(("\n*** WARNING: RESCAL value is out of bounds %d to %d\n",RESCAL_MIN, RESCAL_MAX));
    }

    EFUN_PRINTF((" %02d "              ,  plp_aperta2_peregrine5_pc_acc_get_core(sa__)));
    EFUN_PRINTF(("  %x,%02x  "         ,  state.core_reset, state.core_status));
    EFUN_PRINTF(("    %1d     "        ,  state.pll_pwrdn));
    EFUN_PRINTF(("   %1d    "          ,  state.uc_active));
    EFUN_PRINTF((" %3d.%02dMHz"        , (state.comclk_mhz/4),((state.comclk_mhz%4)*25)));    /* comclk in Mhz = heartbeat_count_1us / 4 */
    EFUN_PRINTF(("   %4X_%02X "        ,  state.ucode_version, state.ucode_minor_version));
    EFUN_PRINTF(("  %06X "             ,  state.api_version));
    EFUN_PRINTF(("    0x%02X   "       ,  state.afe_hardware_version));
    EFUN_PRINTF(("   %3dC   "          ,  state.die_temp));
    EFUN_PRINTF(("   (%02d)%3dC "      ,  state.temp_idx, state.avg_tmon));
    EFUN_PRINTF(("   0x%02x  "         ,  state.rescal));
    EFUN_PRINTF(("  %2d.%03dGHz "      ,  state.vco_rate_mhz/1000, state.vco_rate_mhz % 1000));
    EFUN_PRINTF(("    %03d       "     ,  state.analog_vco_range));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_display_pll_to_divider(sa__, state.pll_div));
    EFUN_PRINTF(("     %01d%s  "       ,  state.pll_lock, state.pll_lock_chg ? "*" : " "));
    if(state.core_status != 0) {
        char decode_buf[512];
        EFUN_PRINTF(("\n"));
        EFUN(plp_aperta2_peregrine5_pc_decode_core_sts(sa__, state.core_status, decode_buf, sizeof(decode_buf), ""));
        EFUN_PRINTF(("%s", decode_buf));
    }
    return (ERR_CODE_NONE);
  }
#endif /* SMALL_FOOTPRINT */


/* returns 000111 (7 = 8-1), for n = 3  */
#define BFMASK(n) ((1<<((n)))-1)

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_update_uc_lane_config_st(struct  peregrine5_pc_uc_lane_config_st *st) {
  uint16_t in = st->word;
  st->field.lane_cfg_from_pcs = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.an_enabled = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.dfe_on = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.rx_low_power = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_cdr_mode = in & BFMASK(2); in = (uint16_t)(in >> 2);
  st->field.media_type = in & BFMASK(2); in = (uint16_t)(in >> 2);
  st->field.unreliable_los = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.linktrn_auto_polarity_en = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.linktrn_restart_timeout_en = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_er = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_nr = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.lp_has_prec_en = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_pam4_mode = in & BFMASK(1); in = (uint16_t)(in >> 1);
  st->field.force_nrz_mode = in & BFMASK(1);
  return(ERR_CODE_NONE);
}
#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_update_usr_ctrl_disable_functions_st(struct peregrine5_pc_usr_ctrl_disable_functions_st *st) {
  uint32_t in = st->dword;
  st->field.all_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.adc_hw_calibration = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.adc_gain_calibration = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.adc_skew_calibration = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.c2d_coupling_calibration = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.pga_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.pfhi_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.pfmid_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.pflow_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.afebw_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.dc_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.blw_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.snr_estimation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.eye_margin_estimation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.ffe_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.ffe_offset_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.rcfir_tap_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.dfe_tap_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.dfe_offset_adaptation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.channel_id_estimation = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.freq_offset = in & BFMASK(1); in = (uint32_t)(in >> 1);
  st->field.generic_disable13 = in & BFMASK(1);
  return ERR_CODE_NONE;
}

#endif /* SMALL_FOOTPRINT */
/* word to fields, for display */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_update_uc_core_config_st(struct peregrine5_pc_uc_core_config_st *st) {
  uint16_t in = st->word;
  st->field.vco_rate = (uint8_t)(in & BFMASK(8)); in = (uint16_t)(in >> 8);
  st->field.core_cfg_from_pcs = (uint8_t)(in & BFMASK(1)); in = (uint16_t)(in >> 1);
  st->field.osr_5_en = (uint8_t)(in & BFMASK(1)); in = (uint16_t)(in >> 1);
  st->field.reserved = in & BFMASK(6);
  st->vco_rate_in_Mhz = VCO_RATE_TO_MHZ(st->field.vco_rate);
  return ERR_CODE_NONE;
}

/* fields to word, to write into uC RAM */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_update_uc_core_config_word(struct peregrine5_pc_uc_core_config_st *st) {
  uint16_t in = 0;
  in = (uint16_t)(in << 6); in |= 0/*st->field.reserved*/ & BFMASK(6);
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.osr_5_en & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.core_cfg_from_pcs & BFMASK(1)));
  in = (uint16_t)(in << 8); in = (uint16_t)(in | (st->field.vco_rate & BFMASK(8)));
  st->word = in;
  return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_update_uc_lane_config_word(struct peregrine5_pc_uc_lane_config_st *st) {
  uint16_t in = 0;
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.force_nrz_mode & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.force_pam4_mode & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.lp_has_prec_en & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.force_nr & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.force_er & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.linktrn_restart_timeout_en & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | ( st->field.linktrn_auto_polarity_en & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | ( st->field.unreliable_los & BFMASK(1)));
  in = (uint16_t)(in << 2); in = (uint16_t)(in | ( st->field.media_type & BFMASK(2)));
  in = (uint16_t)(in << 2); in = (uint16_t)(in | ( st->field.force_cdr_mode & BFMASK(2)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.rx_low_power & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.dfe_on & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.an_enabled & BFMASK(1)));
  in = (uint16_t)(in << 1); in = (uint16_t)(in | (st->field.lane_cfg_from_pcs & BFMASK(1)));
  st->word = in;
  return ERR_CODE_NONE;
}


#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_update_usr_ctrl_disable_functions_byte(struct peregrine5_pc_usr_ctrl_disable_functions_st *st) {
  uint32_t in = 0;
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.generic_disable13 & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.freq_offset & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.channel_id_estimation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.dfe_offset_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.dfe_tap_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.rcfir_tap_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.ffe_offset_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.ffe_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.eye_margin_estimation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.snr_estimation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.blw_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.dc_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.afebw_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.pflow_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.pfmid_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.pfhi_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.pga_adaptation & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.c2d_coupling_calibration & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.adc_skew_calibration & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.adc_gain_calibration & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.adc_hw_calibration & BFMASK(1)));
  in = (uint32_t)(in << 1); in = (uint32_t)(in | (st->field.all_adaptation & BFMASK(1)));
  st->dword = in;
  return ERR_CODE_NONE;
}

#endif /*SMALL_FOOTPRINT */

#undef BFMASK

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_check_uc_lane_stopped(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE

  uint8_t is_micro_stopped;
  ESTM(is_micro_stopped = rdv_usr_sts_micro_stopped() || (rd_rx_lane_dp_reset_state() > 0));
  if (!is_micro_stopped) {
      return(peregrine5_pc_error(sa__, ERR_CODE_UC_NOT_STOPPED));
  } else {
      return(ERR_CODE_NONE);
  }
}
#ifndef SMALL_FOOTPRINT

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_calc_patt_gen_mode_sel(srds_access_t *sa__, uint8_t *mode_sel, uint8_t *zero_pad_len, uint8_t patt_length) {

  if(!mode_sel || !zero_pad_len || !patt_length) {
    return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
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
    return (peregrine5_pc_error(sa__, ERR_CODE_CFG_PATT_INVALID_PATT_LENGTH));
  }
  return(ERR_CODE_NONE);
}
#endif /*SMALL_FOOTPRINT */

/*-----------------------------------------*/
/*  Write Core Config variables to uC RAM  */
/*-----------------------------------------*/

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_set_uc_core_config(srds_access_t *sa__, struct peregrine5_pc_uc_core_config_st struct_val) {
    INIT_SRDS_ERR_CODE
    uint8_t pll_orig;
    ESTM(pll_orig = plp_aperta2_peregrine5_pc_acc_get_pll_idx(sa__));
    EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, 0));
    {   uint8_t reset_state;
        ESTM(reset_state = rdc_core_dp_reset_state());
        if(reset_state < 7) {
            EFUN_PRINTF(("ERROR: plp_aperta2_peregrine5_pc_INTERNAL_set_uc_core_config(..) called without core_dp_s_rstb=0\n"));
            return (peregrine5_pc_error(sa__, ERR_CODE_CORE_DP_NOT_RESET));
        }
    }
    if(struct_val.vco_rate_in_Mhz > 0) {
        struct_val.field.vco_rate = MHZ_TO_VCO_RATE(struct_val.vco_rate_in_Mhz);
    }
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_update_uc_core_config_word(&struct_val));
        EFUN(reg_wrc_CORE_PLL_COM_PLL_UC_CORE_CONFIG(struct_val.word));
    EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, pll_orig));
    return (ERR_CODE_NONE);
}

/*---------------------*/
/*  PLL Configuration  */
/*---------------------*/
/** Extract the refclk frequency in Hz, based on a peregrine5_pc_pll_refclk_enum value. */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_refclk_in_hz(srds_access_t *sa__, enum peregrine5_pc_pll_refclk_enum refclk, uint32_t *refclk_in_hz) {
    switch (refclk) {
        case PEREGRINE5_PC_PLL_REFCLK_156P25MHZ:      *refclk_in_hz = 156250000; break;
        case PEREGRINE5_PC_PLL_REFCLK_161P1328125MHZ: *refclk_in_hz = 161132813; break;
        case PEREGRINE5_PC_PLL_REFCLK_166P67MHZ:      *refclk_in_hz = 166670000; break;
        case PEREGRINE5_PC_PLL_REFCLK_312P5MHZ:       *refclk_in_hz = 312500000; break;
        default:
            EFUN_PRINTF(("ERROR: Unknown refclk frequency:  0x%08X\n", (uint32_t)refclk));
            *refclk_in_hz = 0xFFFFFFFF;
            return (peregrine5_pc_error(sa__, ERR_CODE_REFCLK_FREQUENCY_INVALID));
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
static uint32_t _plp_aperta2_steins_gcd(uint32_t u, uint32_t v) {
    /* simple cases (termination) */
    if (u == v)
        return u;

    if (u == 0)
        return v;

    if (v == 0)
        return u;

    /* look for factors of 2 */
    if (~u & 1) {                                       /* u is even */
        if (v & 1)                                      /* v is odd */
            return _plp_aperta2_steins_gcd(u >> 1, v);
        else                                            /* both u and v are even */
            return _plp_aperta2_steins_gcd(u >> 1, v >> 1) << 1;
    }

    if (~v & 1)                                         /* u is odd, v is even */
        return _plp_aperta2_steins_gcd(u, v >> 1);

    /* reduce larger argument */
    if (u > v)
        return _plp_aperta2_steins_gcd((u - v) >> 1, v);

    return _plp_aperta2_steins_gcd((v - u) >> 1, u);
}

static err_code_t _plp_aperta2_peregrine5_pc_minimize_divisor_ratio(uint32_t md, uint32_t *simple_numerator, uint32_t *simple_denominator) {
    /* continued fraction coefficients */
    uint32_t a, h[3] = { 0, 1, 0 }, k[3] = { 1, 0, 0 };
    uint32_t x, d, n;
    uint8_t i;

    n = *simple_numerator;
    d = *simple_denominator;

    /* continued fraction and check denominator each step */
    for (i = 0; i < 64; i++) {

        a = (n ? d / n : 0);
        if (i && !a) break;

        /* coverity[divide_by_zero] */
        x = d; d = n; n = x % n;

        x = a;
        if (k[1] * a + k[0] >= md) {
            x = (md - k[0]) / k[1];
            if (x * 2 >= a || k[1] >= md)
                i = 65;
            else
                break;
        }

        h[2] = x * h[1] + h[0]; h[0] = h[1]; h[1] = h[2];
        k[2] = x * k[1] + k[0]; k[0] = k[1]; k[1] = k[2];
    }
    *simple_denominator = k[1];
    *simple_numerator   = h[1];
    return (ERR_CODE_NONE);
}

static err_code_t _plp_aperta2_peregrine5_pc_get_divisor_ratio(srds_access_t *sa__, enum peregrine5_pc_pll_div_enum srds_div, uint16_t *simple_numerator, uint16_t *simple_denominator){
    /*
       From the documentation above:
       (numerator / denominator) = (1000 / divisor)
       divisor = div_int + div_decimal where div_decimal = (UPPER_FIVE_HEX_DIGITS_OF_ENUM)/(0x100000).
       This is the same as: (numerator / denominator) =  (1000)(0x100000) / ((div_int)*(0x100000) + UPPER_FIVE_HEX_DIGITS_OF_ENUM)
       numerator = 1000(0x100000) and denominator= div_int*0x100000 + UPPER_FIVE_HEX_DIGITS_OF_ENUM
       If we simplify this fraction, we should end up with the same values as the table above
    */

    uint32_t lowest_numerator, lowest_denominator, gcd;

    if (srds_div == PEREGRINE5_PC_PLL_DIV_UNKNOWN){
        EFUN_PRINTF(("ERROR: Unknown divider value:  0x%08X\n", (uint32_t)srds_div));
        *simple_numerator = 0;
        *simple_denominator = 0;
        return (peregrine5_pc_error(sa__, ERR_CODE_PLL_DIV_INVALID));
    }

    lowest_numerator = (1000)*(0x100000);
    lowest_denominator = ((uint32_t)srds_div & 0xFFF)*0x100000 + (((uint32_t)srds_div & 0xFFFFC000)>>12);
    gcd = _plp_aperta2_steins_gcd(lowest_numerator, lowest_denominator);

    lowest_numerator    = lowest_numerator / gcd;
    lowest_denominator  = lowest_denominator / gcd;

    if (lowest_numerator >= 0x10000){
        _plp_aperta2_peregrine5_pc_minimize_divisor_ratio(0x10000,&lowest_numerator, &lowest_denominator);
        *simple_numerator   = (uint16_t)lowest_denominator;
        *simple_denominator = (uint16_t)lowest_numerator;
        return (ERR_CODE_NONE);
    }
    *simple_numerator   = (uint16_t)lowest_numerator;
    *simple_denominator = (uint16_t)lowest_denominator;

    return (ERR_CODE_NONE);
}


#ifndef SMALL_FOOTPRINT
/** Get the Refclk frequency in Hz, based on the peregrine5_pc_pll_div_enum value and VCO frequency. */
static err_code_t _plp_aperta2_peregrine5_pc_get_refclk_from_div_vco(srds_access_t *sa__, uint32_t *refclk_freq_hz, enum peregrine5_pc_pll_div_enum srds_div, uint32_t vco_freq_khz, enum peregrine5_pc_pll_option_enum pll_option) {
    INIT_SRDS_ERR_CODE
    uint16_t numerator, denominator;
    EFUN(_plp_aperta2_peregrine5_pc_get_divisor_ratio(sa__, srds_div, &numerator, &denominator));
    if (denominator == 0){
        EFUN_PRINTF(("ERROR: Divider returned denominator 0:  0x%08X\n", (uint32_t)srds_div));
        return (peregrine5_pc_error(sa__, ERR_CODE_PLL_DIV_INVALID));
    }
    *refclk_freq_hz = ((vco_freq_khz+(denominator>>1)) / denominator) * numerator;
    if (pll_option == PEREGRINE5_PC_PLL_OPTION_REFCLK_DOUBLER_EN) *refclk_freq_hz /= 2;
    if (pll_option == PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV2_EN)    *refclk_freq_hz *= 2;
    if (pll_option == PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV4_EN)    *refclk_freq_hz *= 4;
    return (ERR_CODE_NONE);
}
#endif /*SMALL_FOOTPRINT */

/** Get the VCO frequency in kHz, based on the reference clock frequency and peregrine5_pc_pll_div_enum value. */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_vco_from_refclk_div(srds_access_t *sa__, uint32_t refclk_freq_hz, enum peregrine5_pc_pll_div_enum srds_div, uint32_t *vco_freq_khz, enum peregrine5_pc_pll_option_enum pll_option) {
    INIT_SRDS_ERR_CODE
    uint16_t numerator, denominator;
    EFUN(_plp_aperta2_peregrine5_pc_get_divisor_ratio(sa__, srds_div, &numerator, &denominator));
    if (pll_option == PEREGRINE5_PC_PLL_OPTION_REFCLK_DOUBLER_EN) refclk_freq_hz *= 2;
    if (pll_option == PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV2_EN)    refclk_freq_hz /= 2;
    if (pll_option == PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV4_EN)    refclk_freq_hz /= 4;
    if (numerator == 0){
        EFUN_PRINTF(("ERROR: Divider returned numerator 0:  0x%08X\n", (uint32_t)srds_div));
        return (peregrine5_pc_error(sa__, ERR_CODE_PLL_DIV_INVALID));
    }
    *vco_freq_khz =(uint32_t) (((uint64_t)refclk_freq_hz * denominator + (numerator >> 1)) / numerator);
    return (ERR_CODE_NONE);
}

/* Boundaries for allowed VCO frequency */
#    define SERDES_VCO_FREQ_KHZ_MIN (41000000)
#    define SERDES_VCO_FREQ_KHZ_MAX (56250000)

/* Allowed tolerance in resultant VCO frequency when auto-determining divider value */
#    define SERDES_VCO_FREQ_KHZ_TOLERANCE (2000)

#ifndef SMALL_FOOTPRINT
/* The allowed PLL divider values */
static const enum peregrine5_pc_pll_div_enum _plp_aperta2_peregrine5_pc_div_candidates[] = {
    PEREGRINE5_PC_PLL_DIV_179P2,
    PEREGRINE5_PC_PLL_DIV_256,
    PEREGRINE5_PC_PLL_DIV_288,
    PEREGRINE5_PC_PLL_DIV_294P4,
    PEREGRINE5_PC_PLL_DIV_316P8,
    PEREGRINE5_PC_PLL_DIV_320,
    PEREGRINE5_PC_PLL_DIV_336,
    PEREGRINE5_PC_PLL_DIV_340,
    PEREGRINE5_PC_PLL_DIV_358,
    PEREGRINE5_PC_PLL_DIV_358P4,
    PEREGRINE5_PC_PLL_DIV_360,
    PEREGRINE5_PC_PLL_DIV_368,
    PEREGRINE5_PC_PLL_DIV_396,
    PEREGRINE5_PC_PLL_DIV_400,
    PEREGRINE5_PC_PLL_DIV_412P5,
    PEREGRINE5_PC_PLL_DIV_448,
    PEREGRINE5_PC_PLL_DIV_480,
    PEREGRINE5_PC_PLL_DIV_528,
    PEREGRINE5_PC_PLL_DIV_560,
    PEREGRINE5_PC_PLL_DIV_132,
    PEREGRINE5_PC_PLL_DIV_147P2,
    PEREGRINE5_PC_PLL_DIV_158P4,
    PEREGRINE5_PC_PLL_DIV_160,
    PEREGRINE5_PC_PLL_DIV_165,
    PEREGRINE5_PC_PLL_DIV_168,
    PEREGRINE5_PC_PLL_DIV_170,
    PEREGRINE5_PC_PLL_DIV_175,
    PEREGRINE5_PC_PLL_DIV_180,
    PEREGRINE5_PC_PLL_DIV_264,
    PEREGRINE5_PC_PLL_DIV_280,
    PEREGRINE5_PC_PLL_DIV_330,
    PEREGRINE5_PC_PLL_DIV_350
};

static const uint8_t _plp_aperta2_peregrine5_pc_div_candidates_count = sizeof(_plp_aperta2_peregrine5_pc_div_candidates) / sizeof(_plp_aperta2_peregrine5_pc_div_candidates[0]);

static err_code_t _plp_aperta2_peregrine5_pc_check_div(srds_access_t *sa__, enum peregrine5_pc_pll_div_enum srds_div) {
    uint8_t i, found = 0;
    for (i=0; i<_plp_aperta2_peregrine5_pc_div_candidates_count; i++) {
        found = (uint8_t)(found | (srds_div == _plp_aperta2_peregrine5_pc_div_candidates[i]));
    }
    if (!found) {
        EFUN_PRINTF(("ERROR: Invalid divider value:  0x%08X\n", (uint32_t)srds_div));
        return (peregrine5_pc_error(sa__, ERR_CODE_PLL_DIV_INVALID));
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_check_vco_freq_khz(srds_access_t *sa__, uint32_t vco_freq_khz) {
    uint32_t min_vco_freq = SERDES_VCO_FREQ_KHZ_MIN;
    if (vco_freq_khz < min_vco_freq - SERDES_VCO_FREQ_KHZ_TOLERANCE) {
        EFUN_PRINTF(("ERROR: VCO frequency too low:  %d kHz is lower than minimum (%d kHz)\n", vco_freq_khz, min_vco_freq));
        return (peregrine5_pc_error(sa__, ERR_CODE_VCO_FREQUENCY_INVALID));
    }
    if (vco_freq_khz > SERDES_VCO_FREQ_KHZ_MAX + SERDES_VCO_FREQ_KHZ_TOLERANCE) {
        EFUN_PRINTF(("ERROR: VCO frequency too high:  %d kHz is higher than maximum (%d kHz)\n", vco_freq_khz, SERDES_VCO_FREQ_KHZ_MAX));
        return (peregrine5_pc_error(sa__, ERR_CODE_VCO_FREQUENCY_INVALID));
    }
    return (ERR_CODE_NONE);
}

/** Find the entry out of _plp_aperta2_peregrine5_pc_div_candidates that is closest to matching refclk_freq_hz and vco_freq_khz. */
static err_code_t _plp_aperta2_peregrine5_pc_get_div(srds_access_t *sa__, uint32_t refclk_freq_hz, uint32_t vco_freq_khz, enum peregrine5_pc_pll_div_enum *srds_div, enum peregrine5_pc_pll_option_enum pll_option) {
    INIT_SRDS_ERR_CODE
    int32_t vco_freq_khz_min_error = 0x7FFFFFFF;
    uint8_t i;
    for (i=0; i<_plp_aperta2_peregrine5_pc_div_candidates_count; i++) {
        uint32_t actual_vco_freq_khz = 0;
        int32_t  vco_freq_khz_error;
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_vco_from_refclk_div(sa__, refclk_freq_hz, _plp_aperta2_peregrine5_pc_div_candidates[i], &actual_vco_freq_khz, pll_option));
        vco_freq_khz_error = (int32_t)(vco_freq_khz - actual_vco_freq_khz);
        vco_freq_khz_error = SRDS_ABS(vco_freq_khz_error);
        if (vco_freq_khz_min_error > vco_freq_khz_error) {
            vco_freq_khz_min_error = vco_freq_khz_error;
            *srds_div = _plp_aperta2_peregrine5_pc_div_candidates[i];
        }
    }
    if (vco_freq_khz_min_error == (int32_t)0xFFFFFFFF) {
      return (peregrine5_pc_error(sa__, ERR_CODE_CONFLICTING_PARAMETERS));
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_resolve_pll_parameters(srds_access_t *sa__,
                                                  enum peregrine5_pc_pll_refclk_enum refclk,
                                                  uint32_t *refclk_freq_hz,
                                                  enum peregrine5_pc_pll_div_enum *srds_div,
                                                  uint32_t *vco_freq_khz,
                                                  enum peregrine5_pc_pll_option_enum pll_option) {
    INIT_SRDS_ERR_CODE

    /* Parameter value and consistency checks */
    const uint8_t given_param_count = (uint8_t)(((refclk == PEREGRINE5_PC_PLL_REFCLK_UNKNOWN) ? 0 : 1)
                                       + ((*srds_div == PEREGRINE5_PC_PLL_DIV_UNKNOWN) ? 0 : 1)
                                       + ((*vco_freq_khz == 0) ? 0 : 1));
    const uint32_t original_vco_freq_khz = *vco_freq_khz;
    enum peregrine5_pc_pll_div_enum auto_div = PEREGRINE5_PC_PLL_DIV_UNKNOWN;
    const char*pll_option_e2s[] = {"no", "refclk_x2", "refclk_div2", "refclk_div4"};

    /* Verify that at least two of the three parameters is given. */
    if (given_param_count < 2) {
        return (peregrine5_pc_error(sa__, ERR_CODE_INSUFFICIENT_PARAMETERS));
    }

    /* Skip verification if option is selected. Error if all parameters not given. */
    if ((pll_option & PEREGRINE5_PC_PLL_OPTION_DISABLE_VERIFY) == PEREGRINE5_PC_PLL_OPTION_DISABLE_VERIFY) {
        if (given_param_count < 3) {
            return (peregrine5_pc_error(sa__, ERR_CODE_INSUFFICIENT_PARAMETERS));
        } else {
            /* Calculate "refclk_freq_hz" from the input "refclk" enum (0xNNNMMMMM = Frequency is MMMMM / NNN MHz) */
            *refclk_freq_hz = (uint32_t)((((uint64_t)(refclk & 0xFFFFF)) * 1000000) / ((((uint32_t)refclk) & 0xFFF00000) >> 20));
            return(ERR_CODE_NONE);
        }
    }
    pll_option = (enum peregrine5_pc_pll_option_enum)(pll_option & PEREGRINE5_PC_PLL_OPTION_REFCLK_MASK);

    /* The refclk value is checked in various functions below. */

    /* Verify that the requested div value is allowed. */
    if (*srds_div != PEREGRINE5_PC_PLL_DIV_UNKNOWN) {
        EFUN(_plp_aperta2_peregrine5_pc_check_div(sa__, *srds_div));
    }

    /* Verify that the requested VCO frequency is allowed. */
    if (*vco_freq_khz != 0) {
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_check_vco_freq_khz(sa__,*vco_freq_khz));
    }

    if (refclk == PEREGRINE5_PC_PLL_REFCLK_UNKNOWN) {
        /* Determine refclk from vco frequency and div */
        EFUN(_plp_aperta2_peregrine5_pc_get_refclk_from_div_vco(sa__, refclk_freq_hz, *srds_div, *vco_freq_khz, pll_option));
    } else {
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_refclk_in_hz(sa__, refclk, refclk_freq_hz));
    }

    if (*vco_freq_khz == 0) {
        /* Determine VCO frequency from refclk and divider */
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_vco_from_refclk_div(sa__, *refclk_freq_hz, *srds_div, vco_freq_khz, pll_option));
    }

    /* Determine divider from vco frequency and refclk.
     * This is done even if the div was provided, because if it is,
     * we still want to check whether the parameters are conflicting.
     */
    EFUN(_plp_aperta2_peregrine5_pc_get_div(sa__, *refclk_freq_hz, *vco_freq_khz, &auto_div, pll_option));
    if (*srds_div == PEREGRINE5_PC_PLL_DIV_UNKNOWN) {
        /* Use the auto-determined divider value, since the divider was not supplied. */
        *srds_div = auto_div;

        /* Determine resultant VCO frequency from refclk and divider */
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_vco_from_refclk_div(sa__, *refclk_freq_hz, *srds_div, vco_freq_khz, pll_option));
    }

    /* Verify the resultant VCO frequency */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_check_vco_freq_khz(sa__, *vco_freq_khz));

    /* Verify that the requested VCO frequency is delivered. */
    if ((original_vco_freq_khz != 0)
        && ((*vco_freq_khz < original_vco_freq_khz - SERDES_VCO_FREQ_KHZ_TOLERANCE)
            || (*vco_freq_khz > original_vco_freq_khz + SERDES_VCO_FREQ_KHZ_TOLERANCE))) {
        EFUN_PRINTF(("ERROR: Could not achieve requested VCO frequency of %d kHz.\n       Refclk is %d Hz, %s option enabled, and auto-determined divider is 0x%08X, yielding a VCO frequency of %d kHz.\n",
                     original_vco_freq_khz, *refclk_freq_hz, pll_option_e2s[pll_option], *srds_div, *vco_freq_khz));
        return (peregrine5_pc_error(sa__, ERR_CODE_VCO_FREQUENCY_INVALID));
    }

    /* Verify the auto-determined divider value. */
    if (auto_div != *srds_div) {
        EFUN_PRINTF(("ERROR: Conflicting PLL parameters:  refclk is %d Hz, %s option enabled, divider is 0x%08X, and VCO frequency is %d kHz.\n       Expected divider is 0x%08X\n",
                     *refclk_freq_hz, pll_option_e2s[pll_option], *srds_div, *vco_freq_khz, auto_div));
        return (peregrine5_pc_error(sa__, ERR_CODE_CONFLICTING_PARAMETERS));
    }

    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_display_pll_to_divider(srds_access_t *sa__, uint32_t srds_div) {
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
        EFUN_PRINTF(("%*s%3d%*s", left_spaces, "", div_integer, decimal_digits - left_spaces + 1, ""));
    }
    return(ERR_CODE_NONE);
}
#endif /* SMALL_FOOTPRINT */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_print_uc_dsc_error(srds_access_t *sa__, enum plp_aperta2_srds_pmd_uc_cmd_enum cmd) {
    INIT_SRDS_ERR_CODE
    uint8_t supp_info;
    uint16_t other_info;

    ESTM(supp_info = rd_uc_dsc_supp_info());
    ESTM(other_info = rd_uc_dsc_data());
    switch (supp_info) {
      case UC_CMD_ERROR_INVALID_COMMAND:
        ESTM_PRINTF(("ERROR : UC reported invalid command %d.  (other_info = 0x%X)\n",
                     cmd, other_info));
        break;
      case UC_CMD_ERROR_BUSY:
        ESTM_PRINTF(("ERROR : UC reported busy for command %d.  (other_info = 0x%X)\n",
                     cmd, other_info));
        break;
      case UC_CMD_ERROR_GET_EYE_SAMPLE_ERROR:
        ESTM_PRINTF(("ERROR : UC reported error in getting eye sample.  (command %d, other_info = 0x%X)\n",
                     cmd, other_info));
        break;
      case UC_CMD_ERROR_PRBS_NOT_LOCKED:
        ESTM_PRINTF(("ERROR : UC reported PRBS not locked.  (command %d, other_info = 0x%X)\n",
                     cmd, other_info));
        break;
      case UC_CMD_ERROR_COMMAND_IN_PROGRESS:
        ESTM_PRINTF(("ERROR : UC reported command already in progress.  (command %d, other_info = 0x%X)\n",
                     cmd, other_info));
        break;
      case UC_CMD_ERROR_INVALID_MODE:
        ESTM_PRINTF(("ERROR : UC reported invalid mode for command %d.  (other_info = 0x%X)\n",
                     cmd, other_info));
        break;
      default:
        ESTM_PRINTF(("ERROR : UC reported unknown error 0x%X for command %d.  (other_info = 0x%X)\n",
                     supp_info, cmd, other_info));
    }
    /* Cleanup cmd register */
    EFUN(reg_wr_DSC_MISC_DSC_UC_CTRL(0x80));
    EFUN(wr_uc_dsc_data(0));

    return(ERR_CODE_NONE);
}
/******************************************/
/*  Serdes Register field Poll functions  */
/******************************************/

#ifndef CUSTOM_REG_POLLING
#ifndef SMALL_FOOTPRINT
/* poll for microcontroller to populate the dsc_data register */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_poll_diag_done(srds_access_t *sa__, uint16_t *status, uint32_t timeout_ms)
{
    INIT_SRDS_ERR_CODE
    uint8_t loop;

    if(!status) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
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
 return(peregrine5_pc_error(sa__, ERR_CODE_DIAG_TIMEOUT));
}
#endif /*SMALL_FOOTPRINT */


/** Poll for field "uc_dsc_ready_for_cmd" = 1 [Return Val => Error_code (0 = Polling Pass)] */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_poll_uc_dsc_ready_for_cmd_equals_1(srds_access_t *sa__, uint32_t timeout_ms, enum plp_aperta2_srds_pmd_uc_cmd_enum cmd)
{
    INIT_SRDS_ERR_CODE
  {
    /* read quickly for 10 tries */
    uint16_t loop = 0;
    uint16_t reset_state;

    for (loop = 0; loop < 100; loop++)
    {
        uint16_t rddata;
        ESTM(rddata = reg_rd_DSC_MISC_DSC_UC_CTRL());
        if (rddata & 0x0080) {    /* bit 7 is uc_dsc_ready_for_cmd */
            if (rddata & 0x0040) {  /* bit 6 is uc_dsc_error_found   */
                EFUN(plp_aperta2_peregrine5_pc_INTERNAL_print_uc_dsc_error(sa__, cmd));
                return(peregrine5_pc_error(sa__, ERR_CODE_UC_CMD_RETURN_ERROR));
            }
            return (ERR_CODE_NONE);
        }
        if(loop>10) {
            EFUN(USR_DELAY_US(10*timeout_ms));
        }
    }
    /* Check if ln_s_rstb is asserted by looking at the reset state, then give a warning and display the commands */
    ESTM(reset_state = rd_lane_dp_reset_state());
    if (reset_state & 0x0007) {
        EFUN_PRINTF(("DSC ready for command is not working; SerDes lane is probably reset!\n"));
        EFUN(reg_wr_DSC_MISC_DSC_UC_CTRL(0x80)); /* Cleanup after failure */
        return (ERR_CODE_NONE);
    }
    {
        EFUN_PRINTF(("%s ERROR : DSC ready for command is not working, applying workaround and getting debug info !\n", API_FUNCTION_NAME));
        /* print the triage info and reset the cmd interface */
        plp_aperta2_peregrine5_pc_INTERNAL_print_triage_info(sa__, ERR_CODE_UC_CMD_POLLING_TIMEOUT, 1, 1, (uint16_t)__LINE__);
        /* artifically terminate the command to re-enable the command interface */
        EFUN(wr_uc_dsc_ready_for_cmd(0x1));
    }
    return (ERR_CODE_UC_CMD_POLLING_TIMEOUT);
  }
}


#ifndef SMALL_FOOTPRINT
/* Poll for field "dsc_state" = DSC_STATE_UC_TUNE [Return Val => Error_code (0 = Polling Pass)] */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_poll_dsc_state_equals_uc_tune(srds_access_t *sa__, uint32_t timeout_ms) {
    INIT_SRDS_ERR_CODE
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
    return (peregrine5_pc_error(sa__, ERR_CODE_POLLING_TIMEOUT));          /* Error Code for polling timeout */
}

#endif /*SMALL_FOOTPRINT */

/* Poll for field "micro_ra_initdone" = 1 [Return Val => Error_code (0 = Polling Pass)] */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_poll_micro_ra_initdone(srds_access_t *sa__, uint32_t timeout_ms)
{
    INIT_SRDS_ERR_CODE
    uint16_t loop, max_count;
    uint8_t result;

    max_count = 333;
    for (loop = 0; loop <= max_count; loop++) {
        ESTM(result = rdc_micro_ra_initdone());
        if (result) {
            return (ERR_CODE_NONE);
        }
        EFUN(USR_DELAY_US(3*timeout_ms));
    }
    return (peregrine5_pc_error(sa__, ERR_CODE_POLLING_TIMEOUT));          /* Error Code for polling timeout */
}

#endif /* CUSTOM_REG_POLLING */

uint8_t plp_aperta2_peregrine5_pc_INTERNAL_is_big_endian(void) {
    uint32_t one_u32 = 0x01000000;
    char * ptr = (char *)(&one_u32);
    const uint8_t big_endian = (ptr[0] == 1);
    return big_endian;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_rdblk_callback(srds_access_t *sa__, void *arg, uint8_t byte_count, uint16_t data) {
    /* Never call plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check() here */

    peregrine5_pc_INTERNAL_rdblk_callback_arg_t * const cast_arg = (peregrine5_pc_INTERNAL_rdblk_callback_arg_t *)arg;
    UNUSED(sa__);
    *(cast_arg->mem_ptr + get_endian_offset(cast_arg->mem_ptr)) = (uint8_t)(data & 0xFF);
    cast_arg->mem_ptr++;
    if (byte_count == 2) {
        *(cast_arg->mem_ptr + get_endian_offset(cast_arg->mem_ptr)) = (uint8_t)(data >> 8);
        cast_arg->mem_ptr++;
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_ram_dump_callback(srds_access_t *sa__, void *arg, uint8_t byte_count, uint16_t data) {
    peregrine5_pc_INTERNAL_ram_dump_state_t * const state_ptr = (peregrine5_pc_INTERNAL_ram_dump_state_t *)arg;
    const uint8_t bytes_per_row=26;

    if (byte_count == 0) {
        if (state_ptr->line_start_index != state_ptr->index) {
            EFUN_PRINTF(("%*s    %d\n", 4*(bytes_per_row - state_ptr->index + state_ptr->line_start_index), "", state_ptr->line_start_index));
        }
        return (ERR_CODE_NONE);
    }
    if (byte_count == 1) {
#if defined(SMALL_FOOTPRINT)
        /* coverity[assigned_value] */
#endif
        data &= 0xFF;
    }

    if (state_ptr->index % bytes_per_row == 0) {
        state_ptr->line_start_index = state_ptr->index;
        EFUN_PRINTF(("\n%04x ", state_ptr->line_start_index));

    }
    EFUN_PRINTF(("%02x ", (uint8_t)(data & 0xFF)));
    EFUN_PRINTF(("%02x ", (data >> 8)));
    state_ptr->index = (uint16_t)(state_ptr->index + 2);

    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_rdblk_ram_callback(srds_access_t *sa__, void *arg, uint8_t byte_count, uint16_t data) {
    INIT_SRDS_ERR_CODE
    srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__); /* Needed for get_endian_offset_info_ptr() */
    peregrine5_pc_INTERNAL_rdblk_ram_arg_t *  cast_arg = (peregrine5_pc_INTERNAL_rdblk_ram_arg_t *)arg;
    peregrine5_pc_INTERNAL_ram_dump_state_t * const state_ptr = (peregrine5_pc_INTERNAL_ram_dump_state_t *)(cast_arg->dump_state_ptr);
    SRDS_INFO_PTR_NULL_CHECK;

    if(cast_arg->ram_buff != NULL) {
        const uint8_t bytes_per_row = 26;
        if (state_ptr->index % bytes_per_row == 0) {
            state_ptr->line_start_index = (state_ptr->index);
            if (state_ptr->index != 0){
                state_ptr->ram_idx++;
                state_ptr->count = 0;
            }

            state_ptr->count += USR_SNPRINTF((cast_arg->ram_buff + get_endian_offset_info_ptr(cast_arg->ram_buff)+(state_ptr->ram_idx * SRDS_DUMP_BUF_SIZE )+ state_ptr->count), (size_t)(SRDS_DUMP_BUF_SIZE - state_ptr->count - 1), "%04x ",(state_ptr->line_start_index) );

        }
        state_ptr->count += USR_SNPRINTF((cast_arg->ram_buff + get_endian_offset_info_ptr(cast_arg->ram_buff)+(state_ptr->ram_idx * SRDS_DUMP_BUF_SIZE )+ state_ptr->count), (size_t)(SRDS_DUMP_BUF_SIZE - state_ptr->count - 1), "%02x ", (uint8_t)(data&0xFF));

        state_ptr->index++;
        if (byte_count == 2) {
            state_ptr->count += USR_SNPRINTF((cast_arg->ram_buff + get_endian_offset_info_ptr(cast_arg->ram_buff)+(state_ptr->ram_idx * SRDS_DUMP_BUF_SIZE)+ state_ptr->count), (size_t)(SRDS_DUMP_BUF_SIZE - state_ptr->count - 1), "%02x ",(uint8_t)(data>>8));
            state_ptr->index++;
        }

        if(state_ptr->index == cast_arg->ram_size){
            state_ptr->count = 0;
            state_ptr->ram_idx++;
            *(cast_arg->ram_buff + get_endian_offset_info_ptr(cast_arg->ram_buff)+(state_ptr->ram_idx * SRDS_DUMP_BUF_SIZE) + state_ptr->count) = 0xA;
            state_ptr->ram_idx++;
            *(cast_arg->ram_buff + get_endian_offset_info_ptr(cast_arg->ram_buff)+(state_ptr->ram_idx * SRDS_DUMP_BUF_SIZE) + state_ptr->count) = 0x0;

        }

    }

    if (cast_arg->ram_buff == NULL) {
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_ram_dump_callback(sa__, cast_arg->dump_state_ptr, byte_count,data));
    }


    return (ERR_CODE_NONE);
}

/************************************************************/
/*      Serdes IP RAM access - Lane RAM Variables           */
/*----------------------------------------------------------*/
/*   - through Micro Register Interface for PMD IPs         */
/*   - through Serdes FW DSC Command Interface for Gallardo */
/************************************************************/

/* This function returns the absolute address of lane RAM variables given the offset address and lane */
uint32_t _plp_aperta2_peregrine5_pc_INTERNAL_get_addr_from_lane(srds_access_t *sa__, uint16_t addr, uint8_t lane) {
    uint32_t lane_var_ram_base=0, lane_var_ram_size=0;
    uint32_t lane_addr_offset = 0;
    uint16_t grp_ram_size=0,lane_count=0;
    srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);

    if (peregrine5_pc_info_ptr != NULL) {
        lane_var_ram_base = peregrine5_pc_info_ptr->lane_var_ram_base;
        lane_var_ram_size = peregrine5_pc_info_ptr->lane_var_ram_size;
        grp_ram_size = peregrine5_pc_info_ptr->grp_ram_size;
        lane_count = peregrine5_pc_info_ptr->lane_count;
        if(!lane_count) {
            return lane_addr_offset; /* To avoid a modulo by 0 exception */
        }
        lane_addr_offset = (uint32_t)(lane_var_ram_base+addr+ ((uint32_t)(lane%lane_count)*lane_var_ram_size) + (uint32_t)(grp_ram_size*plp_aperta2_peregrine5_pc_INTERNAL_grp_idx_from_lane(lane)));
    }
    return(lane_addr_offset);
}
 /******************************************************************************/
 /*      Serdes IP RAM access - Lane Static/Persistent RAM Variables           */
 /*----------------------------------------------------------------------------*/
 /*   - through Micro Register Interface for PMD IPs                           */
 /*   - through Serdes FW DSC Command Interface for Gallardo                   */
 /******************************************************************************/

 /* This function returns the absolute address of static/persistent lane RAM variables given the offset address and lane */
 uint32_t _plp_aperta2_peregrine5_pc_INTERNAL_get_static_addr_from_lane(srds_access_t *sa__, uint16_t addr, uint8_t lane) {
     uint32_t lane_static_var_ram_base=0, lane_static_var_ram_size=0;
     uint32_t lane_static_addr_offset = 0;
     uint16_t grp_ram_size=0,lane_count=0;
     srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);

     if (peregrine5_pc_info_ptr != NULL) {
         lane_static_var_ram_base = peregrine5_pc_info_ptr->lane_static_var_ram_base;
         lane_static_var_ram_size = peregrine5_pc_info_ptr->lane_static_var_ram_size;
         grp_ram_size = peregrine5_pc_info_ptr->grp_ram_size;
         lane_count = peregrine5_pc_info_ptr->lane_count;
         if(!lane_count) {
             return lane_static_addr_offset; /* To avoid a modulo by 0 exception */
         }
         lane_static_addr_offset = (uint32_t)(lane_static_var_ram_base+addr+ ((uint32_t)(lane%lane_count)*lane_static_var_ram_size) + (uint32_t)(grp_ram_size*plp_aperta2_peregrine5_pc_INTERNAL_grp_idx_from_lane(lane)));
     }
     return(lane_static_addr_offset);
 }

 /************************************************************/
 /*      Serdes IP RAM access - Micro RAM Variables           */
 /*----------------------------------------------------------*/
 /*   - through Micro Register Interface for PMD IPs         */
 /************************************************************/

 /* This function returns the absolute address of uc RAM variables given the offset address and lane */
 uint32_t _plp_aperta2_peregrine5_pc_INTERNAL_get_uc_addr_from_lane(srds_access_t *sa__, uint16_t addr, uint8_t lane) {
     uint32_t micro_var_ram_base = 0;
     uint16_t grp_ram_size = 0;
     uint32_t uc_addr_offset = 0;
     srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);

     if (peregrine5_pc_info_ptr != NULL) {
         micro_var_ram_base = peregrine5_pc_info_ptr->micro_var_ram_base;
         grp_ram_size = peregrine5_pc_info_ptr->grp_ram_size;
         uc_addr_offset = (uint32_t)(micro_var_ram_base + addr + (uint32_t)(grp_ram_size*plp_aperta2_peregrine5_pc_INTERNAL_grp_idx_from_lane(lane)));
     }
     return(uc_addr_offset);
 }

 /************************************************************/
 /*      Serdes IP RAM access - Core RAM Variables           */
 /*----------------------------------------------------------*/
 /*   - through Micro Register Interface for PMD IPs         */
 /*   - through Serdes FW DSC Command Interface for Gallardo */
 /************************************************************/

 /* This function returns the absolute address of core RAM variables given the offset address and lane */
 uint32_t _plp_aperta2_peregrine5_pc_INTERNAL_get_addr_from_core(srds_access_t *sa__, uint16_t addr) {
     uint32_t core_var_ram_base=0;
     uint32_t core_addr_offset = 0;
     srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);

     if (peregrine5_pc_info_ptr != NULL) {
         core_var_ram_base = peregrine5_pc_info_ptr->core_var_ram_base;
         core_addr_offset = core_var_ram_base + addr;
     }
     return(core_addr_offset);
}

 /************************************************************/
 /*    Serdes IP RAM access - Debug Block Mem RAM Variables  */
 /*----------------------------------------------------------*/
 /*   - through Micro Register Interface for PMD IPs         */
 /************************************************************/

 /* This function returns the absolute address of debug mem RAM variables given the offset address and lane */
 uint32_t plp_aperta2_peregrine5_pc_INTERNAL_get_debug_mem_addr_from_lane(srds_access_t *sa__, uint8_t lane) {
     uint32_t debug_mem_ram_base = 0;
     uint16_t grp_ram_size = 0;
     uint32_t debug_mem_addr_offset = 0;
     srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);

     if (peregrine5_pc_info_ptr != NULL) {
         debug_mem_ram_base = peregrine5_pc_info_ptr->debug_block_mem_ram_base;
         grp_ram_size = peregrine5_pc_info_ptr->grp_ram_size;
         debug_mem_addr_offset = (uint32_t)(debug_mem_ram_base + (uint32_t)(grp_ram_size*plp_aperta2_peregrine5_pc_INTERNAL_grp_idx_from_lane(lane)));
     }
     return(debug_mem_addr_offset);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_rdblk_ram_read_callback(srds_access_t * sa__,void *arg,uint8_t byte_count,uint16_t data) {
    INIT_SRDS_ERR_CODE
    srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__); /* Needed for get_endian_offset_info_ptr() */
    peregrine5_pc_INTERNAL_rdblk_ram_arg_t * const cast_arg = (peregrine5_pc_INTERNAL_rdblk_ram_arg_t *)arg;
    SRDS_INFO_PTR_NULL_CHECK;
    if(cast_arg->mem_ptr != NULL) {
        *(cast_arg->mem_ptr + get_endian_offset_info_ptr(cast_arg->mem_ptr)) = (uint8_t)(data & 0xFF);
        cast_arg->mem_ptr++;
        if (byte_count == 2) {
            *(cast_arg->mem_ptr + get_endian_offset_info_ptr(cast_arg->mem_ptr)) = (uint8_t)(data >> 8);
            cast_arg->mem_ptr++;
        }
    }
    if (cast_arg->mem_ptr == NULL) {
        EFUN(plp_aperta2_peregrine5_pc_INTERNAL_ram_dump_callback(sa__, cast_arg->dump_state_ptr, byte_count,data));
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram(srds_access_t *sa__,
                                                uint32_t block_addr,
                                                uint32_t block_size,
                                                uint32_t start_offset,
                                                uint32_t cnt,
                                                void *arg,
                                                err_code_t (*callback)(srds_access_t *, void *, uint8_t, uint16_t)) {
    /* Never call plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check() here */

    INIT_SRDS_ERR_CODE
    err_code_t Exit_Status = ERR_CODE_NONE;
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
        uint32_t block_cnt = (uint32_t)(SRDS_MIN(cnt, block_addr + block_size - addr));
        cnt = cnt - block_cnt;

            /* Set up the word reads. */
            CFUN(wrc_micro_autoinc_rdaddr_en(1));
            CFUN(wrc_micro_ra_rddatasize(0x1));                     /* Select 16bit read datasize */
            CFUN(wrc_micro_ra_rdaddr_msw((uint16_t)(addr >> 16)));              /* Upper 16bits of RAM address to be read */
            CFUN(wrc_micro_ra_rdaddr_lsw(addr & 0xFFFE));           /* Lower 16bits of RAM address to be read */

            /* Read the leading byte, if starting at an odd address. */
            if ((addr & 1) == 1) {
                CSTM(read_val |= (uint32_t)((rdc_micro_ra_rddata_lsw() >> 8) << defecit));
                if (defecit == 8) {
                    CFUN(callback(sa__, arg, 2, (uint16_t)read_val));
                    read_val = 0;
                }
                /* We just read a byte.  This toggles the defecit from 0 to 8 or from 8 to 0. */
                defecit ^= 8;
                --block_cnt;
            }

            /* Read the whole words, and call the callback with two bytes at a time. */
            while (block_cnt >= 2) {
                CSTM(read_val |= (uint32_t)(rdc_micro_ra_rddata_lsw() << defecit));
                CFUN(callback(sa__, arg, 2, (uint16_t)read_val));
                read_val >>= 16;
                /* We just read two bytes.  This preserves whatever defecit (8 or 0) is there. */
                block_cnt -= 2;
            }

            /* Read the trailing byte, if leftover after reading whole words. */
            if (block_cnt > 0) {
                CSTM(read_val |= (uint32_t)((rdc_micro_ra_rddata_lsw() & 0xFF) << defecit));
                if (defecit == 8) {
                    CFUN(callback(sa__, arg, 2, (uint16_t)read_val));
                    read_val = 0;
                }
                /* We just read a byte.  This toggles the defecit from 0 to 8 or from 8 to 0. */
                defecit ^= 8;
            }
        addr = block_addr;
    }

    /* If a final byte is left behind, then call the callback with it. */
    if (defecit > 0) {
        CFUN(callback(sa__, arg, 1, (uint16_t)read_val));
    }

Exit:
    return(Exit_Status);
}


#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram_descending(srds_access_t *sa__,
                                                           uint32_t block_addr,
                                                           uint32_t block_size,
                                                           uint32_t start_offset,
                                                           uint32_t cnt,
                                                           void *arg,
                                                           err_code_t (*callback)(srds_access_t *, void *, uint8_t, uint16_t)) {
    INIT_SRDS_ERR_CODE
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
        uint32_t block_cnt = (uint32_t)(SRDS_MIN(cnt, start_offset+1));
        cnt = cnt - block_cnt;

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
                EFUN(callback(sa__, arg, 2, (uint16_t)read_val));
                read_val >>= 16;
                /* We just read two bytes.  This preserves whatever defecit (8 or 0) is there. */
                block_cnt -= 2;
                addr -= 2;
            }
            else {
                if ((addr & 1) == 1) {
                    /* Reading upper byte of word. */
                    read_val = read_val | (uint32_t)((read_val2 >> 8) << defecit);
                } else {
                    /* Reading lower byte of word. */
                    read_val = read_val | (uint32_t)((read_val2 & 0xFF) << defecit);
                }
                if (defecit == 8) {
                    EFUN(callback(sa__, arg, 2, (uint16_t)read_val));
                    read_val = 0;
                }
                /* We just read a byte.  This toggles the defecit from 0 to 8 or from 8 to 0. */
                defecit ^= 8;
                --block_cnt;
                --addr;
            }
        }

        addr = block_addr + block_size - 1;
        start_offset = block_size - 1;
    }

    /* If a final byte is left behind, then call the callback with it. */
    if (defecit > 0) {
        EFUN(callback(sa__, arg, 1, (uint16_t)read_val));
    }

    return(ERR_CODE_NONE);
}
#endif /*SMALL_FOOTPRINT */

uint8_t plp_aperta2_peregrine5_pc_INTERNAL_grp_idx_from_lane(uint8_t lane) {
    return(lane>>1);
}


#ifndef SMALL_FOOTPRINT
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_clk4sync_div2_sequence(srds_access_t *sa__, uint8_t enable) {
    INIT_SRDS_ERR_CODE
    err_code_t Exit_Status = ERR_CODE_NONE;
    uint8_t pll_selected = 0;
    uint8_t pll_orig = 0;
    uint8_t pll_lock = 0;
    uint8_t pll_lock_chg = 0;
    if (enable) {
        /* Check if selected pll is locked */
        ESTM(pll_orig = plp_aperta2_peregrine5_pc_acc_get_pll_idx(sa__));
        pll_selected = 0;
        CFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__,pll_selected));
        CFUN(plp_aperta2_peregrine5_pc_INTERNAL_pll_lock_status(sa__, &pll_lock, &pll_lock_chg));

        if (pll_lock) {
            CFUN(wrc_clk4sync_div2_s_comclk_sel(0x1)); /* Reset clk4sync_div2 clock domain to use comclk (slow clock) */
            CFUN(wrc_clk4sync_div2_en(0x1)); /* 1: div2 enabled */
            CFUN(wrc_clk4sync_div2_s_comclk_sel(0x0)); /* Switch clk4sync_div2 clock domain to use VCO divided clk (fast clock) */
        }
        else {
            EFUN_PRINTF(("Selected PLL not locked for PRBS error analyzer!\n"));
            Exit_Status = ERR_CODE_INVALID_PRBS_ERR_ANALYZER_NO_PLL_LOCK;
        }
Exit:
            EFUN(plp_aperta2_peregrine5_pc_acc_set_pll_idx(sa__, pll_orig));
    }
    else {
        EFUN(wrc_clk4sync_div2_s_comclk_sel(0x1)); /* Switch clk4sync_div2 clock domain to use comclk (slow clock) */
        EFUN(wrc_clk4sync_div2_en(0x0)); /* 0: disable to save power */
    }

    return (Exit_Status);
}
#endif /* SMALL_FOOTPRINT */



#ifndef SMALL_FOOTPRINT





#endif /* SMALL_FOOTPRINT */

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_display_info_table (srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));

    EFUN_PRINTF(("\n********** SERDES INFO TABLE DUMP **********\n"));
    EFUN_PRINTF(("signature                 = 0x%X\n", peregrine5_pc_info_ptr->signature));
    EFUN_PRINTF(("diag_mem_ram_base         = 0x%X\n", peregrine5_pc_info_ptr->diag_mem_ram_base));
    EFUN_PRINTF(("diag_mem_ram_size         = 0x%X\n", peregrine5_pc_info_ptr->diag_mem_ram_size));
    EFUN_PRINTF(("core_var_ram_base         = 0x%X\n", peregrine5_pc_info_ptr->core_var_ram_base));
    EFUN_PRINTF(("core_var_ram_size         = 0x%X\n", peregrine5_pc_info_ptr->core_var_ram_size));
    EFUN_PRINTF(("lane_var_ram_base         = 0x%X\n", peregrine5_pc_info_ptr->lane_var_ram_base));
    EFUN_PRINTF(("lane_var_ram_size         = 0x%X\n", peregrine5_pc_info_ptr->lane_var_ram_size));
        EFUN_PRINTF(("lane_static_var_ram_base  = 0x%X\n", peregrine5_pc_info_ptr->lane_static_var_ram_base));
        EFUN_PRINTF(("lane_static_var_ram_size  = 0x%X\n", peregrine5_pc_info_ptr->lane_static_var_ram_size));
    EFUN_PRINTF(("trace_mem_ram_base        = 0x%X\n", peregrine5_pc_info_ptr->trace_mem_ram_base));
    EFUN_PRINTF(("trace_mem_ram_size        = 0x%X\n", peregrine5_pc_info_ptr->trace_mem_ram_size));
    EFUN_PRINTF(("micro_var_ram_base        = 0x%X\n", peregrine5_pc_info_ptr->micro_var_ram_base));
    EFUN_PRINTF(("common_block_mem_ram_base = 0x%X\n", peregrine5_pc_info_ptr->common_block_mem_ram_base));
    EFUN_PRINTF(("common_block_mem_ram_size = 0x%X\n", peregrine5_pc_info_ptr->common_block_mem_ram_size));
    EFUN_PRINTF(("debug_block_mem_ram_base  = 0x%X\n", peregrine5_pc_info_ptr->debug_block_mem_ram_base));
    EFUN_PRINTF(("debug_block_mem_ram_size  = 0x%X\n", peregrine5_pc_info_ptr->debug_block_mem_ram_size));
    EFUN_PRINTF(("lane_count                = 0x%X\n", peregrine5_pc_info_ptr->lane_count));
    EFUN_PRINTF(("trace_memory_descending_writes = 0x%X\n", peregrine5_pc_info_ptr->trace_memory_descending_writes));
    EFUN_PRINTF(("micro_count               = 0x%X\n", peregrine5_pc_info_ptr->micro_count));
    EFUN_PRINTF(("micro_var_ram_size        = 0x%X\n", peregrine5_pc_info_ptr->micro_var_ram_size));
    EFUN_PRINTF(("grp_ram_size              = 0x%X\n", peregrine5_pc_info_ptr->grp_ram_size));
    EFUN_PRINTF(("ucode_version             = 0x%X\n", peregrine5_pc_info_ptr->ucode_version));
    EFUN_PRINTF(("info_table_version        = 0x%X\n", peregrine5_pc_info_ptr->info_table_version));
    EFUN_PRINTF(("silicon_version           = 0x%X\n", peregrine5_pc_info_ptr->silicon_version));
    EFUN_PRINTF(("is_big_endian             = 0x%X\n", peregrine5_pc_info_ptr->is_big_endian));

    return (ERR_CODE_NONE);
}
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_logical_tx_lane_addr(srds_access_t *sa__, uint8_t physical_tx_lane, uint8_t *logical_tx_lane) {
    INIT_SRDS_ERR_CODE
    if(logical_tx_lane == NULL) return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    switch(physical_tx_lane) {
        case 0:
            ESTM(*logical_tx_lane = rdc_tx_lane_addr_0());
            break;
        case 1:
            ESTM(*logical_tx_lane = rdc_tx_lane_addr_1());
            break;
        case 2:
            ESTM(*logical_tx_lane = rdc_tx_lane_addr_2());
            break;
        case 3:
            ESTM(*logical_tx_lane = rdc_tx_lane_addr_3());
            break;
        case 4:
            ESTM(*logical_tx_lane = rdc_tx_lane_addr_4());
            break;
        case 5:
            ESTM(*logical_tx_lane = rdc_tx_lane_addr_5());
            break;
        case 6:
            ESTM(*logical_tx_lane = rdc_tx_lane_addr_6());
            break;
        case 7:
            ESTM(*logical_tx_lane = rdc_tx_lane_addr_7());
            break;
        default:
            return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    return (ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_logical_rx_lane_addr(srds_access_t *sa__, uint8_t physical_rx_lane, uint8_t *logical_rx_lane) {
    INIT_SRDS_ERR_CODE
    if(logical_rx_lane == NULL) return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    switch(physical_rx_lane) {
        case 0:
            ESTM(*logical_rx_lane = rdc_rx_lane_addr_0());
            break;
        case 1:
            ESTM(*logical_rx_lane = rdc_rx_lane_addr_1());
            break;
        case 2:
            ESTM(*logical_rx_lane = rdc_rx_lane_addr_2());
            break;
        case 3:
            ESTM(*logical_rx_lane = rdc_rx_lane_addr_3());
            break;
        case 4:
            ESTM(*logical_rx_lane = rdc_rx_lane_addr_4());
            break;
        case 5:
            ESTM(*logical_rx_lane = rdc_rx_lane_addr_5());
            break;
        case 6:
            ESTM(*logical_rx_lane = rdc_rx_lane_addr_6());
            break;
        case 7:
            ESTM(*logical_rx_lane = rdc_rx_lane_addr_7());
            break;
        default:
            return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    return (ERR_CODE_NONE);
}

#ifndef SMALL_FOOTPRINT
/****************/
/*  DBstop API  */
/****************/
static err_code_t _plp_aperta2_print_breakpoint_warning(srds_access_t *sa__) {
    EFUN_PRINTF(("*******************************************************************************\n"));
    EFUN_PRINTF(("* WARNING: SRDS_BREAKPOINT functionality is being used.                       *\n"));
    EFUN_PRINTF(("*                                                                             *\n"));
    EFUN_PRINTF(("*          Use of this feature may affect SERDES performance!                 *\n"));
    EFUN_PRINTF(("*******************************************************************************\n"));
    return (ERR_CODE_NONE);
}
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_en_breakpoint(srds_access_t *sa__, uint8_t breakpoint) {
    INIT_SRDS_ERR_CODE
    EFUN(_plp_aperta2_print_breakpoint_warning(sa__));
    EFUN(wrv_usr_dbstop_enable(breakpoint));
    return (ERR_CODE_NONE);
}
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_goto_breakpoint(srds_access_t *sa__, uint8_t breakpoint) {
    INIT_SRDS_ERR_CODE
    uint8_t already_stopped;

    ESTM(already_stopped = rdv_usr_dbstopped());      /* find out if breakpoint is already reached */
    EFUN(wrv_usr_dbstop_enable(breakpoint));
    if (already_stopped) {WRV_USR_DBSTOPPED_TO_0;}    /* if already stopped, let it continue */
    return (ERR_CODE_NONE);
}
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_rd_breakpoint(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    uint8_t val;
    srds_core_t core;
    uint8_t lane;
#ifdef SMALL_FOOTPRINT
    UNUSED(core);
    UNUSED(lane);
#endif
    ESTM(core = plp_aperta2_peregrine5_pc_acc_get_core(sa__));
    ESTM(lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__));
    ESTM(val = rdv_usr_dbstopped());
    if(val > 0) {
        EFUN_PRINTF(("Stopped at Breakpoint %d on Core=%d, Lane=%d\n", val, core, lane));
    }
    else {
        EFUN_PRINTF(("Not currently stopped at a breakpoint.\n"));
    }
    return (ERR_CODE_NONE);
}
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_dis_breakpoint(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    EFUN(wrv_usr_dbstop_enable(0));
    WRV_USR_DBSTOPPED_TO_0;
    return (ERR_CODE_NONE);
}

#endif /* SMALL_FOOTPRINT */



err_code_t plp_aperta2_peregrine5_pc_INTERNAL_fit_second_order(srds_access_t *sa__, uint8_t n, USR_DOUBLE *x, USR_DOUBLE *y, USR_DOUBLE *a) {
#ifdef SERDES_API_FLOATING_POINT
    uint8_t    i, j, k;
    USR_DOUBLE Ex[5]     = { 0.0 }; /* elements of normal matrix */
    USR_DOUBLE Eb[3][4]  = { {0.0} }; /* augmented matrix */
    USR_DOUBLE Ey[3]     = { 0.0 };
    UNUSED(sa__);

    if ( x == NULL || y == NULL || a == NULL ) {
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }
    if ( n < 2 ) {
        EFUN_PRINTF(("Not enough samples.\n"));
        return (ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
    }

    for (i=0; i<=4; i++) {
        Ex[i] = 0;
        for (j=0; j<n; j++) {
            Ex[i] = Ex[i] + pow(x[j], i);
        }
    }

    for (i=0; i<=2; i++) {
        Ey[i] = 0;
        for (j=0; j<n; j++) {
            Ey[i] = Ey[i] + pow(x[j], i) * y[j];
        }
    }

    for (i=0; i<=2; i++) {
        for (j=0; j<=2; j++) {
            Eb[i][j] = Ex[i+j];
        }
        Eb[i][3] = Ey[i];
    }

    /* apply gauss elimination to get the coefficents */
    for (i=0; i<2; i++) {
        /* partial pivoting */
        for (k=(uint8_t)(i+1); k<3; k++) {
            /* swap if diagonal element(absolute) is smaller than any terms below it */
             if (fabs(Eb[i][j]) < fabs(Eb[k][i])) {
                 for (j=0; j<4; j++) {
                     USR_DOUBLE temp;
                     temp = Eb[i][j];
                     Eb[i][j] = Eb[k][j];
                     Eb[k][j] = temp;
                 }
            }
        }
        /* gauss elimination */
        for (k=(uint8_t)(i+1); k<3; k++) {
            USR_DOUBLE term = Eb[k][i]/Eb[i][i];
            for (j=0; j<4; j++) {
                Eb[k][j] = Eb[k][j] - term * Eb[i][j];
            }
        }
    }

    /* back substitution */
    for (i=3; i>0; i--) {
        a[i-1] = Eb[i-1][3];
        for (j=i; j<3; j++) {
            a[i-1] = a[i-1] - Eb[i-1][j] * a[j];
        }
        a[i-1] = a[i-1]/Eb[i-1][i-1];
    }

    return (ERR_CODE_NONE);
#else
    UNUSED(n);
    UNUSED(x);
    UNUSED(y);
    UNUSED(a);
    USR_PRINTF(("%s : This function needs SERDES_API_FLOATING_POINT define to operate \n", API_FUNCTION_NAME));
    return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
#endif /* SERDES_API_FLOATING_POINT */
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_is_flr_supported(srds_access_t *sa__, uint8_t *flr_support) {
    INIT_SRDS_ERR_CODE
    srds_info_t * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_match_ucode_from_info(sa__, peregrine5_pc_info_ptr));

    *flr_support = ((peregrine5_pc_info_ptr->ucode_version & 0x000FFFFF) >= 0x00000203);
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_ffe_offsets(srds_access_t *sa__, int16_t *buf, size_t buf_size, uint16_t *byte_count) {
    INIT_SRDS_ERR_CODE
    srds_info_t * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    uint8_t i = 0;
    int16_t data = 0;
    uint8_t is_big_endian;
    SRDS_INFO_PTR_NULL_CHECK;
    is_big_endian = peregrine5_pc_info_ptr->is_big_endian;

    if(((buf != NULL) && (buf_size < SRDS_NUM_ADCS)) || (byte_count == NULL)) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }


        EFUN(wr_ffe_offset_rd_auto_inc_en(1));
        EFUN(wr_ffe_offset_rd_idx(0));
    for(i = 0; i < SRDS_NUM_ADCS; i++) {
            ESTM(data = rd_ffe_offset_rd_val());

        if(buf == NULL)
        {
            const uint8_t bytes_per_row=26;
            if(!(*byte_count % bytes_per_row)) {
                EFUN_PRINTF(("\n%04x ", *byte_count));
            }
            EFUN_PRINTF(("%02x ", (is_big_endian ? ((data >> 8) & 0xFF) : (uint8_t)(data & 0xFF))));
            EFUN_PRINTF(("%02x ", (is_big_endian ? (uint8_t)(data & 0xFF) : ((data >> 8) & 0xFF))));
        }

        if(buf != NULL) {
            buf[i] = data;
        }
        *byte_count += 2;
    }
    return ERR_CODE_NONE;
}


err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_ffe_taps(srds_access_t *sa__, int16_t *buf, size_t buf_size, uint16_t *byte_count) {
    INIT_SRDS_ERR_CODE
    srds_info_t * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__);
    uint8_t i = 0;
    int16_t data = 0;
    uint8_t is_big_endian;
    SRDS_INFO_PTR_NULL_CHECK;
    is_big_endian = peregrine5_pc_info_ptr->is_big_endian;

    if(((buf != NULL) && (buf_size < (SRDS_NUM_FFE_ILVS * SRDS_NUM_FFE_TAPS))) || (byte_count == NULL)) {
        return ERR_CODE_BAD_PTR_OR_INVALID_INPUT;
    }

    EFUN(wr_ffe_rd_auto_inc_en(1));
    EFUN(wr_ffe_rd_ilv_idx(1));
    EFUN(wr_ffe_rd_tap_idx(0));
    for (i = 0; i < SRDS_NUM_FFE_ILVS * SRDS_NUM_FFE_TAPS; i++) {
        ESTM(data = rd_ffe_rd_tap_val());
        if(buf == NULL)
        {
            const uint8_t bytes_per_row=26;
            if(!(*byte_count % bytes_per_row)) {
                EFUN_PRINTF(("\n%04x ", *byte_count));
            }
            EFUN_PRINTF(("%02x ", (is_big_endian ? ((data >> 8) & 0xFF) : (uint8_t)(data & 0xFF))));
            EFUN_PRINTF(("%02x ", (is_big_endian ? (uint8_t)(data & 0xFF) : ((data >> 8) & 0xFF))));
        }

    if(buf != NULL) {
         buf[i] = data;
    }
        *byte_count += 2;
    }

    return ERR_CODE_NONE;
}


/* Get PRBS Error Analyzer All Error Counts for aggregate lane from register */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_error_analyzer_aggregate_err_counts_register(srds_access_t *sa__, uint32_t *err_counters, uint8_t num_counters) {
    INIT_SRDS_ERR_CODE

    uint8_t  lane;

    if (!err_counters) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (num_counters < 17) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    /* aggregate lane */
    ESTM(lane = rdc_revid_multiplicity());

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_error_analyzer_err_counts_register(sa__, err_counters, num_counters, lane));

    return (ERR_CODE_NONE);
}

/* Get PRBS Error Analyzer All Error Counts for one lane from register */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_error_analyzer_lane_err_counts_register(srds_access_t *sa__, uint32_t *err_counters, uint8_t num_counters) {
    INIT_SRDS_ERR_CODE

    uint8_t  lane;

    if (!err_counters) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (num_counters < 17) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    ESTM(lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__));
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_error_analyzer_err_counts_register(sa__, err_counters, num_counters, lane));


    return (ERR_CODE_NONE);
}

/* Get PRBS Error Analyzer All Error Counts from register */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_error_analyzer_err_counts_register(srds_access_t *sa__, uint32_t *err_counters, uint8_t num_counters, uint8_t lane) {
    INIT_SRDS_ERR_CODE

    uint8_t num_lanes;
    uint8_t use_compressed_format = 0;
    uint8_t upper_3b = 0;
    const int DELAY = 10;
    uint8_t data_ready = 0;
    uint16_t lo = 0, med = 0, hi = 0;
    int loops;

    if (!err_counters) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    if (num_counters < 17) {
       return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    /* read the number of lanes from register */
    ESTM(num_lanes = rdc_revid_multiplicity());
    if (lane > num_lanes) { /* aggregate lane is included */
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    EFUN(wrc_tlb_err_analyze_read_ln(lane));

    EFUN(USR_DELAY_NS(300));

    EFUN(wrc_tlb_err_analyze_rd_strobe(0x1));

    loops = 0;
    do {
        ESTM(data_ready = rdc_tlb_err_analyze_counter_read_stky());
        if (data_ready) {

            ESTM(use_compressed_format = rdc_tlb_err_analyze_rd_float());

            if ( ! use_compressed_format ) {
                /* counter 0 */
                ESTM(hi = rdc_tlb_err_analyze_counter_0_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_0_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_0_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[0] = 0xffffffff;
                }
                else {
                    err_counters[0] = (uint32_t)med<<16 | lo;
                }

                /* counter 1 */
                ESTM(hi = rdc_tlb_err_analyze_counter_1_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_1_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_1_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[1] = 0xffffffff;
                }
                else {
                    err_counters[1] = (uint32_t)med<<16 | lo;
                }

                /* counter 2 */
                ESTM(hi = rdc_tlb_err_analyze_counter_2_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_2_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_2_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[2] = 0xffffffff;
                }
                else {
                    err_counters[2] = (uint32_t)med<<16 | lo;
                }

                /* counter 3 */
                ESTM(hi = rdc_tlb_err_analyze_counter_3_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_3_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_3_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[3] = 0xffffffff;
                }
                else {
                    err_counters[3] = (uint32_t)med<<16 | lo;
                }

                /* counter 4 */
                ESTM(hi = rdc_tlb_err_analyze_counter_4_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_4_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_4_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[4] = 0xffffffff;
                }
                else {
                    err_counters[4] = (uint32_t)med<<16 | lo;
                }

                /* counter 5 */
                ESTM(hi = rdc_tlb_err_analyze_counter_5_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_5_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_5_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[5] = 0xffffffff;
                }
                else {
                    err_counters[5] = (uint32_t)med<<16 | lo;
                }

                /* counter 6 */
                ESTM(hi = rdc_tlb_err_analyze_counter_6_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_6_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_6_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[6] = 0xffffffff;
                }
                else {
                    err_counters[6] = (uint32_t)med<<16 | lo;
                }

                /* counter 7 */
                ESTM(hi = rdc_tlb_err_analyze_counter_7_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_7_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_7_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[7] = 0xffffffff;
                }
                else {
                    err_counters[7] = (uint32_t)med<<16 | lo;
                }

                /* counter 8 */
                ESTM(hi = rdc_tlb_err_analyze_counter_8_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_8_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_8_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[8] = 0xffffffff;
                }
                else {
                    err_counters[8] = (uint32_t)med<<16 | lo;
                }

                /* counter 9 */
                ESTM(hi = rdc_tlb_err_analyze_counter_9_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_9_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_9_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[9] = 0xffffffff;
                }
                else {
                    err_counters[9] = (uint32_t)med<<16 | lo;
                }

                /* counter 10 */
                ESTM(hi = rdc_tlb_err_analyze_counter_10_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_10_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_10_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[10] = 0xffffffff;
                }
                else {
                    err_counters[10] = (uint32_t)med<<16 | lo;
                }

                /* counter 11 */
                ESTM(hi = rdc_tlb_err_analyze_counter_11_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_11_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_11_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[11] = 0xffffffff;
                }
                else {
                    err_counters[11] = (uint32_t)med<<16 | lo;
                }

                /* counter 12 */
                ESTM(hi = rdc_tlb_err_analyze_counter_12_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_12_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_12_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[12] = 0xffffffff;
                }
                else {
                    err_counters[12] = (uint32_t)med<<16 | lo;
                }

                /* counter 13 */
                ESTM(hi = rdc_tlb_err_analyze_counter_13_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_13_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_13_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[13] = 0xffffffff;
                }
                else {
                    err_counters[13] = (uint32_t)med<<16 | lo;
                }

                /* counter 14 */
                ESTM(hi = rdc_tlb_err_analyze_counter_14_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_14_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_14_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[14] = 0xffffffff;
                }
                else {
                    err_counters[14] = (uint32_t)med<<16 | lo;
                }

                /* counter 15 */
                ESTM(hi = rdc_tlb_err_analyze_counter_15_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_15_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_15_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[15] = 0xffffffff;
                }
                else {
                    err_counters[15] = (uint32_t)med<<16 | lo;
                }

                /* counter 16 */
                ESTM(hi = rdc_tlb_err_analyze_counter_16_hi());
                ESTM(med = rdc_tlb_err_analyze_counter_16_med());
                ESTM(lo = rdc_tlb_err_analyze_counter_16_lo());

                upper_3b = (uint8_t)hi;
                if( upper_3b ) {
                    err_counters[16] = 0xffffffff;
                }
                else {
                    err_counters[16] = (uint32_t)med<<16 | lo;
                }
            }
            else { /* use_compressed_format */
                /* counter 0 */
                ESTM(hi = rdc_tlb_err_analyze_counter_0_hi());
                err_counters[0] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 1 */
                ESTM(hi = rdc_tlb_err_analyze_counter_1_hi());
                err_counters[1] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 2 */
                ESTM(hi = rdc_tlb_err_analyze_counter_2_hi());
                err_counters[2] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 3 */
                ESTM(hi = rdc_tlb_err_analyze_counter_3_hi());
                err_counters[3] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 4 */
                ESTM(hi = rdc_tlb_err_analyze_counter_4_hi());
                err_counters[4] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 5 */
                ESTM(hi = rdc_tlb_err_analyze_counter_5_hi());
                err_counters[5] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 6 */
                ESTM(hi = rdc_tlb_err_analyze_counter_6_hi());
                err_counters[6] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 7 */
                ESTM(hi = rdc_tlb_err_analyze_counter_7_hi());
                err_counters[7] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 8 */
                ESTM(hi = rdc_tlb_err_analyze_counter_8_hi());
                err_counters[8] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 9 */
                ESTM(hi = rdc_tlb_err_analyze_counter_9_hi());
                err_counters[9] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 10 */
                ESTM(hi = rdc_tlb_err_analyze_counter_10_hi());
                err_counters[10] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 11 */
                ESTM(hi = rdc_tlb_err_analyze_counter_11_hi());
                err_counters[11] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 12 */
                ESTM(hi = rdc_tlb_err_analyze_counter_12_hi());
                err_counters[12] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 13 */
                ESTM(hi = rdc_tlb_err_analyze_counter_13_hi());
                err_counters[13] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 14 */
                ESTM(hi = rdc_tlb_err_analyze_counter_14_hi());
                err_counters[14] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 15 */
                ESTM(hi = rdc_tlb_err_analyze_counter_15_hi());
                err_counters[15] = (uint32_t)(hi & 0x7ff) << (hi>>11);

                /* counter 16 */
                ESTM(hi = rdc_tlb_err_analyze_counter_16_hi());
                err_counters[16] = (uint32_t)(hi & 0x7ff) << (hi>>11);
            }

            break; /* exit while loop */
        }

        EFUN(USR_DELAY_MS(1));

    } while( loops++ < DELAY );

    if ( loops >= DELAY ) {
        return (ERR_CODE_POLLING_TIMEOUT);
    }

    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_get_prbs_error_analyzer_lane_err_count(srds_access_t *sa__, peregrine5_pc_prbs_err_analyzer_lane_status_st *err_analyzer_status, uint8_t lane) {
    INIT_SRDS_ERR_CODE

    uint8_t  i;
    uint8_t  num_lanes;
    uint8_t  num_counters;
    uint32_t err_counters[17] = {0};
    uint32_t size_to_max = 0;
    uint32_t prbs_bit_errcnt = 0;
    uint8_t  lcklost = 0;
    USR_DOUBLE temp_errs;

    if(!err_analyzer_status) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    /* read the number of lanes from register */
    ESTM(num_lanes = rdc_revid_multiplicity());
    if (lane > num_lanes) { /* aggregate lane is included */
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    if (lane < num_lanes) {
       EFUN(plp_aperta2_peregrine5_pc_prbs_err_count_state(sa__, &prbs_bit_errcnt,&lcklost));
    }
    else {
       prbs_bit_errcnt = 0;
       lcklost = 0;
     }

    num_counters = sizeof(err_counters) / sizeof(uint32_t);
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_error_analyzer_err_counts_register(sa__, err_counters, num_counters, lane));

    for (i=0; i<(PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS + 1); i++) {
        switch ( i ) {
            case 0:
                ESTM(err_analyzer_status->prbs_frames_all = err_counters[i]);
                break;
            default:
                ESTM(err_analyzer_status->prbs_errcnt[i-1] = err_counters[i]);
                break;
        }
    }

    if (PRBS_VERBOSE > 2) {
        EFUN_PRINTF(("\n\n\t==== DEBUG INFO (start) [Core = %d, Lane = %d]\n", plp_aperta2_peregrine5_pc_acc_get_core(sa__), lane));
        EFUN_PRINTF(("\n\t << Frame Errors accumulated in the counter registers >>\n"));
        EFUN_PRINTF(("\t FEC frames with    0 Errors = %u\n",err_analyzer_status->prbs_frames_all));
        for (i=0; i<PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i++) {
            if (i < (PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1)) {
                EFUN_PRINTF(("\t FEC frames with   %2d Errors = %u\n",i+1,err_analyzer_status->prbs_errcnt[i]));
            }
            else {
                EFUN_PRINTF(("\t FEC frames with > %2d Errors = %u\n",(PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS-1),err_analyzer_status->prbs_errcnt[i]));
            }
        }
    }

    /* Calculating cumulative error count for respective values of "t" */
    for (i=PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS; i > 0; i--) {
        if (i!=PEREGRINE5_PC_PRBS_NUM_OF_ERROR_ANALYZER_COUNTERS) {
            if (err_analyzer_status->prbs_errcnt[i] == 0xFFFFFFFF) {
                err_analyzer_status->prbs_errcnt[i-1] = 0xFFFFFFFF;
                if (PRBS_VERBOSE > 2) {
                    EFUN_PRINTF(("\t err_analyzer_status->prbs_errcnt[%d] == MAX\n", (i-1)));
                }
            }
            else {
                /* Check for saturation while accumulating from histogram bins */
                size_to_max = 0xFFFFFFFF - err_analyzer_status->prbs_errcnt[i-1];
                if ((err_analyzer_status->prbs_errcnt[i-1] == 0xFFFFFFFF) || (size_to_max <= err_analyzer_status->prbs_errcnt[i])) {
                      err_analyzer_status->prbs_errcnt[i-1] = 0xFFFFFFFF;
                      if (PRBS_VERBOSE > 2) {
                          EFUN_PRINTF(("\t err_analyzer_status->prbs_errcnt[%d] == MAX\n", (i-1)));
                      }
                }
                else
                      err_analyzer_status->prbs_errcnt[i-1] = (err_analyzer_status->prbs_errcnt[i-1] + err_analyzer_status->prbs_errcnt[i]);
            }
        }
    }

    /* Calculating the number of all frames */
    if (err_analyzer_status->prbs_errcnt[0] == 0xFFFFFFFF) {
        err_analyzer_status->prbs_frames_all = 0xFFFFFFFF;
        if (PRBS_VERBOSE > 2) {
            EFUN_PRINTF(("\t err_analyzer_status->prbs_frames_all == MAX\n"));
        }
    }
    else {
        /* Check for saturation while accumulating from histogram bins */
        size_to_max = 0xFFFFFFFF - err_analyzer_status->prbs_frames_all;
        if ((err_analyzer_status->prbs_frames_all == 0xFFFFFFFF) || (size_to_max <= err_analyzer_status->prbs_errcnt[0])) {
              err_analyzer_status->prbs_frames_all = 0xFFFFFFFF;
              if (PRBS_VERBOSE > 2) {
                  EFUN_PRINTF(("\t err_analyzer_status->prbs_frames_all == MAX\n"));
              }
        }
        else
              err_analyzer_status->prbs_frames_all = (err_analyzer_status->prbs_frames_all + err_analyzer_status->prbs_errcnt[0]);
    }

    if (PRBS_VERBOSE > 2) {
        EFUN_PRINTF(("\n\t==== DEBUG INFO (end)\n\n"));
    }

#ifdef SERDES_API_FLOATING_POINT
    /* Accumulating PRBS errors */
    temp_errs = (USR_DOUBLE) (0xFFFFFFFFFFFFF - prbs_bit_errcnt);
    if ((prbs_bit_errcnt == 0xFFFFFFFF) || (temp_errs <= err_analyzer_status->prbs_bit_errcnt)) {
        err_analyzer_status->prbs_bit_errcnt = 0xFFFFFFFFFFFFF;
    }
    else {
        err_analyzer_status->prbs_bit_errcnt = (prbs_bit_errcnt + err_analyzer_status->prbs_bit_errcnt);
    }
#else
    UNUSED(temp_errs);
#endif

    return (ERR_CODE_NONE);
}



/* Stop error analyzer and Clear DAC pattern memory */
/* for all lanes and aggregated */
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_clear_error_analyzer_dac_pattern_memory(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    uint8_t i;

    EFUN(wrc_tlb_err_aggr_start_error_aggregation(0x0));
    /* Disables error counting into the memory */
    EFUN(wrc_tlb_err_analyze_en(0x0));


    /* Set clock to comclk when writing to SC bit dac_test_mem_write_pulse.
       The following is added for Osprey7 and new Blackhawk7 cores. */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_clk4sync_div2_sequence(sa__, 0));
    EFUN(wrc_dac_test_mem_write_en(0));
    EFUN(wrc_dac_test_mem_write_pulse(0));
    EFUN(wrc_dac_test_readback_en(0));
    EFUN(wrc_dac_test_read_en(0));
    EFUN(wrc_dac_test_test_en(0));
    EFUN(wrc_dac_test_lane_sel_en(0));
    /* Enables tclk0 to be used by the memory for rclk and wclk. Need to set this clock to be the fastest tclk */
    EFUN(wrc_dac_test_lane_sel_en(1));
    EFUN(wrc_dac_test_mem_write_en(1));
    EFUN(wrc_dac_test_test_en(1));

    EFUN(wrc_tlb_err_analyze_reset_mem_data(1));

    /* Write the data into the memory */
    for (i = 0; i < 50; i++) {
        EFUN(wrc_dac_test_mem_write_pulse(1));
    }

    EFUN(wrc_dac_test_mem_write_en(0));
    EFUN(wrc_dac_test_test_en(0));
    EFUN(wrc_tlb_err_analyze_reset_mem_data(0));
    /* Set clock to fast clock(clk4sync_div2) for running PRBS error analyzer.
       This is added for Osprey7 and new Blackhawk7 cores. */
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_clk4sync_div2_sequence(sa__, 1));

    return (ERR_CODE_NONE);
}


#if !defined(SMALL_FOOTPRINT)
err_code_t plp_aperta2_peregrine5_pc_INTERNAL_poll_diag_data(srds_access_t *sa__, const srds_info_t *peregrine5_pc_info_ptr, peregrine5_pc_poll_diag_data_st *poll_diag_state) {
    INIT_SRDS_ERR_CODE
    const uint32_t lane_diag_size = peregrine5_pc_info_ptr->diag_mem_ram_size;
    uint16_t loop;

    if(!poll_diag_state || !poll_diag_state->status || !poll_diag_state->diag_rd_ptr) {
        return(peregrine5_pc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    *poll_diag_state->diag_rd_ptr = 0;

    /** If the byte_count is too high, then there might be problems not updating
     *  the read pointer fast enough.
     */
    if (poll_diag_state->byte_count > (lane_diag_size)) {
        EFUN_PRINTF(("\nERROR : plp_aperta2_peregrine5_pc_INTERNAL_poll_diag_data() has excessive byte count of %d expecting <= %d.\n", poll_diag_state->byte_count, lane_diag_size));
        return (peregrine5_pc_error(sa__, ERR_CODE_DIAG_TIMEOUT));
    }

    ESTM(*poll_diag_state->diag_rd_ptr = rdv_usr_diag_rd_ptr());

    /* Wait until byte_count bytes are available to be read in the diagnostic memory. */
    loop = 0;
    while (1) {
        uint16_t diag_wr_ptr, full_count;

        ESTM(diag_wr_ptr = rdv_usr_diag_wr_ptr());
        if (diag_wr_ptr >= *poll_diag_state->diag_rd_ptr) {
            full_count = diag_wr_ptr - *poll_diag_state->diag_rd_ptr;
        } else {
            full_count = (uint16_t)(diag_wr_ptr + lane_diag_size - *poll_diag_state->diag_rd_ptr);
        }
        if (full_count >= poll_diag_state->byte_count) {
            break;
        }

        ++loop;
        if (loop > 10) {
            EFUN(USR_DELAY_US(10*poll_diag_state->timeout_ms));
        }
        if (loop > 1000) {
            return(peregrine5_pc_error(sa__, ERR_CODE_DIAG_TIMEOUT));
        }
    }
    ESTM(*poll_diag_state->status = rdv_usr_diag_status());
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_read_diag_data(srds_access_t *sa__, const srds_info_t *peregrine5_pc_info_ptr, peregrine5_pc_read_diag_data_st *read_diag_state) {
    INIT_SRDS_ERR_CODE
    uint32_t lane_diag_base;
    const uint8_t lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    uint32_t timeout_ms = (*read_diag_state->status & 0x4000)?300000:DIAG_POLL_TIMEOUT;
    peregrine5_pc_poll_diag_data_st poll_state;
    if((!(peregrine5_pc_info_ptr->lane_count)) || (!(peregrine5_pc_info_ptr->diag_mem_ram_size))) {
        return ERR_CODE_INFO_TABLE_ERROR; /* To avoid a modulo by 0 exception */
    }
    {
        lane_diag_base = peregrine5_pc_info_ptr->diag_mem_ram_base + ((lane%peregrine5_pc_info_ptr->lane_count) * peregrine5_pc_info_ptr->diag_mem_ram_size) +
            (uint32_t)(peregrine5_pc_info_ptr->grp_ram_size*plp_aperta2_peregrine5_pc_INTERNAL_grp_idx_from_lane(peregrine5_pc_acc_get_physical_lane(sa__)));
    }
    poll_state.status = read_diag_state->status;
    poll_state.diag_rd_ptr = read_diag_state->diag_rd_ptr;
    poll_state.timeout_ms = timeout_ms;
    poll_state.byte_count = read_diag_state->byte_count;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_poll_diag_data(sa__, peregrine5_pc_info_ptr, &poll_state));

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_rdblk_uc_generic_ram(sa__,
                                              lane_diag_base,
                                              (uint16_t)peregrine5_pc_info_ptr->diag_mem_ram_size,
                                              *read_diag_state->diag_rd_ptr,
                                              read_diag_state->byte_count,
                                              read_diag_state->arg,
                                              read_diag_state->callback));

    *read_diag_state->diag_rd_ptr = (uint16_t)(((uint32_t)*read_diag_state->diag_rd_ptr + read_diag_state->byte_count) % peregrine5_pc_info_ptr->diag_mem_ram_size);
    EFUN(wrv_usr_diag_rd_ptr(*read_diag_state->diag_rd_ptr));
    read_diag_state->bytes_remaining = (*poll_state.status & 0xFF) - poll_state.byte_count;
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_eye_margin_proj_start(srds_access_t *sa__, uint8_t ber_scan_mode, uint8_t timer_control, uint8_t max_error_control, int16_t *offset_start) {
    INIT_SRDS_ERR_CODE
    uint8_t lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    enum peregrine5_pc_rx_pam_mode_enum pam_mode = NRZ;
    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_get_rx_pam_mode(sa__, &pam_mode));

    if(pam_mode != NRZ) {
        EFUN_PRINTF(("Lane %d ERROR: NRZ eye margin projection is only available in NRZ mode\n", lane));
        return(ERR_CODE_INVALID_RX_PAM_MODE);
    }
    /* Below 'DIAG_VERBOSE' level is intended to be modified only within a debug */
    /* session immediately after a breakpoint, and to retain its state only */
    /* through function exit:  therefore it must be 'volatile' to prevent a */
    /* compiler from eliding code conditioned on it, but NOT 'static'.      */

    if(DIAG_VERBOSE > 2) EFUN_PRINTF(("lane %d: start begin\n", lane));
    EFUN(plp_aperta2_peregrine5_pc_start_ber_scan_test(sa__, ber_scan_mode, timer_control, max_error_control));
    *offset_start = EYE_SCAN_NRZ_VERTICAL_IDX_MAX;
    if(DIAG_VERBOSE > 2) EFUN_PRINTF(("lane %d: start done\n", lane));
    return ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_eye_margin_proj_end(srds_access_t *sa__, USR_DOUBLE rate, uint8_t ber_scan_mode, uint8_t timer_control, int16_t offset_start) {
    INIT_SRDS_ERR_CODE
    uint32_t errs[DIAG_MAX_SAMPLES];
    uint32_t time[DIAG_MAX_SAMPLES];
    uint8_t i,cnt=0;
    uint16_t sts;
    uint8_t lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);

    /* Below 'DIAG_VERBOSE' level is intended to be modified only within a debug */
    /* session immediately after a breakpoint, and to retain its state only */
    /* through function exit:  therefore it must be 'volatile' to prevent a */
    /* compiler from eliding code conditioned on it, but NOT 'static'.      */

    for(i=0;i<DIAG_MAX_SAMPLES;i++) {
        errs[i]=0;
        time[i]=0;
    }

    EFUN(plp_aperta2_peregrine5_pc_INTERNAL_poll_diag_done(sa__, &sts,(uint32_t)(timer_control*2000)));

    if(DIAG_VERBOSE > 2) EFUN_PRINTF(("lane %d: delay done\n", lane));

    EFUN(plp_aperta2_peregrine5_pc_read_ber_scan_data(sa__, &errs[0], &time[0], &cnt, 2000));

    if(DIAG_VERBOSE > 2) EFUN_PRINTF(("lane %d: read done cnt=%d\n",lane, cnt));

    EFUN(plp_aperta2_peregrine5_pc_meas_eye_scan_done(sa__));

    if(DIAG_VERBOSE > 2) EFUN_PRINTF(("lane %d: end function done\n", lane));
 /* if(cnt == 1) {                                                     */
 /*     EFUN_PRINTF(("Not enough points found to extrapolate BER\n")); */
 /*     return(ERR_CODE_NONE);                                         */
 /* }                                                                  */

    EFUN(plp_aperta2_peregrine5_pc_display_ber_scan_data(sa__, rate, ber_scan_mode, &errs[0], &time[0],(uint8_t)SRDS_ABS(offset_start)));


    if(DIAG_VERBOSE > 2) EFUN_PRINTF(("lane %d: display done\n", lane));

    return ERR_CODE_NONE;
}

#endif /* !defined(SMALL_FOOTPRINT) */
#define SRDS_OPP_SIGN(_AS_, _BS_)   (((_AS_) < 0) ^ ((_BS_) < 0))
int32_t  plp_aperta2_peregrine5_pc_INTERNAL_signed_round_div(int32_t a, int32_t b) {
    int32_t  half_b;
    if(b == 0) {
        return 0;
    }
    half_b = b / 2;
    return ((a + (SRDS_OPP_SIGN(a, b) ? (-half_b) : (half_b))) / b);
}

err_code_t plp_aperta2_peregrine5_pc_INTERNAL_lpbk_lane_map_check(srds_access_t *sa__) {
    INIT_SRDS_ERR_CODE
    uint8_t logical_lane, physical_lane, rx_map, tx_map, num_lanes;
    logical_lane = plp_aperta2_peregrine5_pc_acc_get_lane(sa__);
    ESTM(num_lanes = rdc_revid_multiplicity());
    for(physical_lane = 0; physical_lane < num_lanes; physical_lane++) {
        switch (physical_lane) {
            case 0:
                ESTM(rx_map=rdc_rx_lane_addr_0());
                ESTM(tx_map=rdc_tx_lane_addr_0());
                break;
            case 1:
                ESTM(rx_map=rdc_rx_lane_addr_1());
                ESTM(tx_map=rdc_tx_lane_addr_1());
                break;
            case 2:
                ESTM(rx_map=rdc_rx_lane_addr_2());
                ESTM(tx_map=rdc_tx_lane_addr_2());
                break;
            case 3:
                ESTM(rx_map=rdc_rx_lane_addr_3());
                ESTM(tx_map=rdc_tx_lane_addr_3());
                break;
            case 4:
                ESTM(rx_map=rdc_rx_lane_addr_4());
                ESTM(tx_map=rdc_tx_lane_addr_4());
                break;
            case 5:
                ESTM(rx_map=rdc_rx_lane_addr_5());
                ESTM(tx_map=rdc_tx_lane_addr_5());
                break;
            case 6:
                ESTM(rx_map=rdc_rx_lane_addr_6());
                ESTM(tx_map=rdc_tx_lane_addr_6());
                break;
            case 7:
                ESTM(rx_map=rdc_rx_lane_addr_7());
                ESTM(tx_map=rdc_tx_lane_addr_7());
                break;
            default:
                EFUN_PRINTF(("Error: Unsupported lane %d\n", physical_lane));
                return(peregrine5_pc_error(sa__, ERR_CODE_BAD_LANE));
        }
        if((rx_map == logical_lane) || (tx_map == logical_lane)) {
            return ((rx_map != tx_map) ? ERR_CODE_RX_TX_LANE_MISMATCH : ERR_CODE_NONE);
        }
    }
    /* RX and TX of the logical lane are not mapped to any physical lane */
    return ERR_CODE_BAD_LANE;
}
