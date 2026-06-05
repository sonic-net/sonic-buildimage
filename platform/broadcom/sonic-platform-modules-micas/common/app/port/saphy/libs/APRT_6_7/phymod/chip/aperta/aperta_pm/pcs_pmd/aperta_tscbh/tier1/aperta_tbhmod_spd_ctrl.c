/*------------------------------------------------------------------------------
 * $Id: aperta_tbhmod_spd_ctrl.c $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *------------------------------------------------------------------------------
 *  Description: Functons for Speed Table, AM Table, and UM Table access.
 *----------------------------------------------------------------------------*/
#ifndef _APERTA_TBHMOD_SPD_CTRL_C_
#define _APERTA_TBHMOD_SPD_CTRL_C_

#include "aperta_tbhmod_spd_ctrl_defines.h"
#include "aperta_tbhmod_spd_ctrl.h"

/*
 * plp_aperta_spd_ctrl_unpack_spd_id_tbl_entry() : Unpacks a char array into a spd_id_tbl_entry_t
 */
void plp_aperta_spd_ctrl_unpack_spd_id_tbl_entry(uint32_t *packed_entry, spd_id_tbl_entry_t *entry)
{
    int word_size = 32;
    int word_idx;
    int bit_idx;
    int high_bits;
    int low_bits;
    int high_mask;
    int low_mask;
   
    /* AM_TABLE_INDEX */
    word_idx = SPD_ID_TBL_AM_TABLE_INDEX_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_AM_TABLE_INDEX_OFFSET % word_size;
    entry->am_table_index = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_AM_TABLE_INDEX_MASK;

    /* FEC_ARCH */
    word_idx = SPD_ID_TBL_FEC_ARCH_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_FEC_ARCH_OFFSET % word_size;
    entry->fec_arch = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_FEC_ARCH_MASK;

    /* SYMBOL_INTERLEAVE */
    word_idx = SPD_ID_TBL_SYMBOL_INTERLEAVE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_SYMBOL_INTERLEAVE_OFFSET % word_size;
    entry->symbol_interleave = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_SYMBOL_INTERLEAVE_MASK;

    /* TC_XOR_CONTROL */
    word_idx = SPD_ID_TBL_TC_XOR_CONTROL_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_TC_XOR_CONTROL_OFFSET % word_size;
    entry->tc_xor_control = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_TC_XOR_CONTROL_MASK;

    /* RS_FEC_SYNC_HEADER_MODE */
    word_idx = SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_OFFSET % word_size;
    entry->rs_fec_sync_header_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_MASK;

    /* RS_FEC_CWM_NIBBLE_MATCH_COUNT */
    word_idx = SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_OFFSET % word_size;
    entry->rs_fec_cwm_nibble_match_count = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_MASK;

    /* BASE_R_FEC_SYNC_HEADER_MODE */
    word_idx = SPD_ID_TBL_BASE_R_FEC_SYNC_HEADER_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BASE_R_FEC_SYNC_HEADER_MODE_OFFSET % word_size;
    entry->base_r_fec_sync_header_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_BASE_R_FEC_SYNC_HEADER_MODE_MASK;

    /* RS_FEC_SYMBOL_ERROR_WINDOW_MODE */
    word_idx = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_OFFSET % word_size;
    entry->rs_fec_symbol_error_window_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_MASK;

    /* RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD */
    word_idx = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_OFFSET % word_size;
    low_bits = word_size - bit_idx;
    low_mask = (1 << low_bits)  -1;
    high_bits = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_WIDTH - low_bits;
    high_mask = (1 << high_bits) - 1;
    entry->rs_fec_symbol_error_count_threshold = (packed_entry[word_idx] >> bit_idx) & low_mask;
    entry->rs_fec_symbol_error_count_threshold |= (packed_entry[word_idx + 1] & high_mask) << low_bits;

    /* DESKEW_FORWARDING_THRESHOLD_2XN */
    word_idx = SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_OFFSET % word_size;
    entry->deskew_forwarding_threshold_2xn = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_MASK;

    /* DESKEW_FORWARDING_THRESHOLD_1XN */
    word_idx = SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_OFFSET % word_size;
    entry->deskew_forwarding_threshold_1xn = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_MASK;

    /* BIT_MUX_MODE */
    word_idx = SPD_ID_TBL_BIT_MUX_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BIT_MUX_MODE_OFFSET % word_size;
    entry->bit_mux_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_BIT_MUX_MODE_MASK;

    /* NUM_LANES */
    word_idx = SPD_ID_TBL_NUM_LANES_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_NUM_LANES_OFFSET % word_size;
    entry->num_lanes = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_NUM_LANES_MASK;

    /* DESKEW_WINDOW_WITHOUT_RS_FEC */
    word_idx = SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_OFFSET % word_size;
    entry->deskew_window_without_rs_fec = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_MASK;

    /* DESKEW_WINDOW_WITH_RS_FEC */
    word_idx = SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_OFFSET % word_size;
    entry->deskew_window_with_rs_fec = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_MASK;

    /* BER_FSM_DISABLE_WITH_RS_FEC */
    word_idx = SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_OFFSET % word_size;
    entry->ber_fsm_disable_with_rs_fec = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_MASK;

    /* BER_FSM_DISABLE_WITHOUT_RS_FEC */
    word_idx = SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_OFFSET % word_size;
    entry->ber_fsm_disable_without_rs_fec = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_MASK;

    /* BER_WINDOW_MODE */
    word_idx = SPD_ID_TBL_BER_WINDOW_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BER_WINDOW_MODE_OFFSET % word_size;
    entry->ber_window_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_BER_WINDOW_MODE_MASK;

    /* BER_TRIGGER_COUNT */
    word_idx = SPD_ID_TBL_BER_TRIGGER_COUNT_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BER_TRIGGER_COUNT_OFFSET % word_size;
    entry->ber_trigger_count = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_BER_TRIGGER_COUNT_MASK;

    /* CREDIT_QUOTIENT */
    word_idx = SPD_ID_TBL_CREDIT_QUOTIENT_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_CREDIT_QUOTIENT_OFFSET % word_size;
    entry->credit_quotient = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_CREDIT_QUOTIENT_MASK;

    /* CREDIT_REMAINDER */
    word_idx = SPD_ID_TBL_CREDIT_REMAINDER_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_CREDIT_REMAINDER_OFFSET % word_size;
    entry->credit_remainder = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_CREDIT_REMAINDER_MASK;

    /* CREDIT_DIVISOR */
    word_idx = SPD_ID_TBL_CREDIT_DIVISOR_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_CREDIT_DIVISOR_OFFSET % word_size;
    low_bits = word_size - bit_idx;
    low_mask = (1 << low_bits)  -1;
    high_bits = SPD_ID_TBL_CREDIT_DIVISOR_WIDTH - low_bits;
    high_mask = (1 << high_bits) - 1;
    entry->credit_divisor = (packed_entry[word_idx] >> bit_idx) & low_mask;
    entry->credit_divisor |= (packed_entry[word_idx + 1] & high_mask) << low_bits;

    /* USE_CL49_BLOCK_SYNC */
    word_idx = SPD_ID_TBL_USE_CL49_BLOCK_SYNC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_USE_CL49_BLOCK_SYNC_OFFSET % word_size;
    entry->use_cl49_block_sync = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_USE_CL49_BLOCK_SYNC_MASK;

    /* SCR_MODE */
    word_idx = SPD_ID_TBL_SCR_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_SCR_MODE_OFFSET % word_size;
    entry->scr_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_SCR_MODE_MASK;

    /* CODEC_MODE */
    word_idx = SPD_ID_TBL_CODEC_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_CODEC_MODE_OFFSET % word_size;
    entry->codec_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_CODEC_MODE_MASK;

    /* TRAINING_EN */
    word_idx = SPD_ID_TBL_TRAINING_EN_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_TRAINING_EN_OFFSET % word_size;
    entry->training_en = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_TRAINING_EN_MASK;

    /* AN_TIMER_SELECT */
    word_idx = SPD_ID_TBL_AN_TIMER_SELECT_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_AN_TIMER_SELECT_OFFSET % word_size;
    entry->an_timer_select = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_AN_TIMER_SELECT_MASK;

    /* PMD_PAM4_MODE */
    word_idx = SPD_ID_TBL_PMD_PAM4_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_PMD_PAM4_MODE_OFFSET % word_size;
    entry->pmd_pam4_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_PMD_PAM4_MODE_MASK;

    /* PMD_OSR_MODE */
    word_idx = SPD_ID_TBL_PMD_OSR_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_PMD_OSR_MODE_OFFSET % word_size;
    entry->pmd_osr_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_PMD_OSR_MODE_MASK;

    /* T_PMA_WATERMARK */
    word_idx = SPD_ID_TBL_T_PMA_WATERMARK_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_T_PMA_WATERMARK_OFFSET % word_size;
    entry->t_pma_watermark = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_T_PMA_WATERMARK_MASK;

    /* L_TPMA_WATERMARK */
    word_idx = SPD_ID_TBL_L_TPMA_WATERMARK_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_L_TPMA_WATERMARK_OFFSET % word_size;
    entry->l_tpma_watermark = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_L_TPMA_WATERMARK_MASK;

    /* LOOP_BLOCK_COUNT_FOR_TS */
    word_idx = SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_OFFSET % word_size;
    low_bits = word_size - bit_idx;
    low_mask = (1 << low_bits)  -1;
    high_bits = SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_WIDTH - low_bits;
    high_mask = (1 << high_bits) - 1;
    entry->loop_block_count_for_ts = (packed_entry[word_idx] >> bit_idx) & low_mask;
    entry->loop_block_count_for_ts |= (packed_entry[word_idx + 1] & high_mask) << low_bits;

    /* LOOP_BIT_COUNT_FOR_TS */
    word_idx = SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_OFFSET % word_size;
    entry->loop_bit_count_for_ts = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_MASK;

    /* AM_LOCK_FSM_MODE */
    word_idx = SPD_ID_TBL_AM_LOCK_FSM_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_AM_LOCK_FSM_MODE_OFFSET % word_size;
    entry->am_lock_fsm_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_AM_LOCK_FSM_MODE_MASK;

    /* T_PMA_START_MODE */
    word_idx = SPD_ID_TBL_T_PMA_START_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_T_PMA_START_MODE_OFFSET % word_size;
    entry->t_pma_start_mode = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_T_PMA_START_MODE_MASK;

    /* HI_SER_ENABLE */
    word_idx = SPD_ID_TBL_HI_SER_ENABLE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_HI_SER_ENABLE_OFFSET % word_size;
    entry->hi_ser_enable = (packed_entry[word_idx] >> bit_idx) & SPD_ID_TBL_HI_SER_ENABLE_MASK;

    return;
}

/*
 * plp_aperta_spd_ctrl_pack_spd_id_tbl_entry() : Pack a spd_id_tbl_entry_t into an array of 32-bit blocks
 */
void plp_aperta_spd_ctrl_pack_spd_id_tbl_entry(spd_id_tbl_entry_t *entry, uint32_t *packed_entry)
{
    int num_words = 5;
    int word_size = 32;
    int word_idx = 0;
    int bit_idx = 0;
    int high_bits;
    int low_bits;
    int high_mask;
    int low_mask;
    int i;

    /* Initialize the packed_entry array to 0's */
    for (i = 0; i < num_words; i++) {
        packed_entry[i] = 0;
    }

    /* AM_TABLE_INDEX */
    word_idx = SPD_ID_TBL_AM_TABLE_INDEX_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_AM_TABLE_INDEX_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->am_table_index & SPD_ID_TBL_AM_TABLE_INDEX_MASK) << bit_idx;

    /* FEC_ARCH */
    word_idx = SPD_ID_TBL_FEC_ARCH_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_FEC_ARCH_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->fec_arch & SPD_ID_TBL_FEC_ARCH_MASK) << bit_idx;

    /* SYMBOL_INTERLEAVE */
    word_idx = SPD_ID_TBL_SYMBOL_INTERLEAVE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_SYMBOL_INTERLEAVE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->symbol_interleave & SPD_ID_TBL_SYMBOL_INTERLEAVE_MASK) << bit_idx;

    /* TC_XOR_CONTROL */
    word_idx = SPD_ID_TBL_TC_XOR_CONTROL_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_TC_XOR_CONTROL_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->tc_xor_control & SPD_ID_TBL_TC_XOR_CONTROL_MASK) << bit_idx;

    /* RS_FEC_SYNC_HEADER_MODE */
    word_idx = SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->rs_fec_sync_header_mode & SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_MASK) << bit_idx;

    /* RS_FEC_CWM_NIBBLE_MATCH_COUNT */
    word_idx = SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->rs_fec_cwm_nibble_match_count & SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_MASK) << bit_idx;

    /* BASE_R_FEC_SYNC_HEADER_MODE */
    word_idx = SPD_ID_TBL_BASE_R_FEC_SYNC_HEADER_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BASE_R_FEC_SYNC_HEADER_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->base_r_fec_sync_header_mode & SPD_ID_TBL_BASE_R_FEC_SYNC_HEADER_MODE_MASK) << bit_idx;

    /* RS_FEC_SYMBOL_ERROR_WINDOW_MODE */
    word_idx = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->rs_fec_symbol_error_window_mode & SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_MASK) << bit_idx;

    /* RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD */
    word_idx = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_OFFSET % word_size;
    low_bits = word_size - bit_idx;
    low_mask = (1 << low_bits)  -1;
    high_bits = SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_WIDTH - low_bits;
    high_mask = (1 << high_bits) - 1;
    packed_entry[word_idx] |= (entry->rs_fec_symbol_error_count_threshold & low_mask) << bit_idx;
    packed_entry[word_idx + 1] |= (entry->rs_fec_symbol_error_count_threshold >> low_bits) & high_mask;

    /* DESKEW_FORWARDING_THRESHOLD_2XN */
    word_idx = SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->deskew_forwarding_threshold_2xn & SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_MASK) << bit_idx;

    /* DESKEW_FORWARDING_THRESHOLD_1XN */
    word_idx = SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->deskew_forwarding_threshold_1xn & SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_MASK) << bit_idx;

    /* BIT_MUX_MODE */
    word_idx = SPD_ID_TBL_BIT_MUX_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BIT_MUX_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->bit_mux_mode & SPD_ID_TBL_BIT_MUX_MODE_MASK) << bit_idx;

    /* NUM_LANES */
    word_idx = SPD_ID_TBL_NUM_LANES_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_NUM_LANES_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->num_lanes & SPD_ID_TBL_NUM_LANES_MASK) << bit_idx;

    /* DESKEW_WINDOW_WITHOUT_RS_FEC */
    word_idx = SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->deskew_window_without_rs_fec & SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_MASK) << bit_idx;

    /* DESKEW_WINDOW_WITH_RS_FEC */
    word_idx = SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->deskew_window_with_rs_fec & SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_MASK) << bit_idx;

    /* BER_FSM_DISABLE_WITH_RS_FEC */
    word_idx = SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->ber_fsm_disable_with_rs_fec & SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_MASK) << bit_idx;

    /* BER_FSM_DISABLE_WITHOUT_RS_FEC */
    word_idx = SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->ber_fsm_disable_without_rs_fec & SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_MASK) << bit_idx;

    /* BER_WINDOW_MODE */
    word_idx = SPD_ID_TBL_BER_WINDOW_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BER_WINDOW_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->ber_window_mode & SPD_ID_TBL_BER_WINDOW_MODE_MASK) << bit_idx;

    /* BER_TRIGGER_COUNT */
    word_idx = SPD_ID_TBL_BER_TRIGGER_COUNT_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_BER_TRIGGER_COUNT_OFFSET % word_size;
        packed_entry[word_idx] |= (entry->ber_trigger_count & SPD_ID_TBL_BER_TRIGGER_COUNT_MASK) << bit_idx;

    /* CREDIT_QUOTIENT */
    word_idx = SPD_ID_TBL_CREDIT_QUOTIENT_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_CREDIT_QUOTIENT_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->credit_quotient & SPD_ID_TBL_CREDIT_QUOTIENT_MASK) << bit_idx;

    /* CREDIT_REMAINDER */
    word_idx = SPD_ID_TBL_CREDIT_REMAINDER_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_CREDIT_REMAINDER_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->credit_remainder & SPD_ID_TBL_CREDIT_REMAINDER_MASK) << bit_idx;

    /* CREDIT_DIVISOR */
    word_idx = SPD_ID_TBL_CREDIT_DIVISOR_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_CREDIT_DIVISOR_OFFSET % word_size;
    low_bits = word_size - bit_idx;
    low_mask = (1 << low_bits)  -1;
    high_bits = SPD_ID_TBL_CREDIT_DIVISOR_WIDTH - low_bits;
    high_mask = (1 << high_bits) - 1;
    packed_entry[word_idx] |= (entry->credit_divisor & low_mask) << bit_idx;
    packed_entry[word_idx + 1] |= (entry->credit_divisor >> low_bits) & high_mask;

    /* USE_CL49_BLOCK_SYNC */
    word_idx = SPD_ID_TBL_USE_CL49_BLOCK_SYNC_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_USE_CL49_BLOCK_SYNC_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->use_cl49_block_sync & SPD_ID_TBL_USE_CL49_BLOCK_SYNC_MASK) << bit_idx;

    /* SCR_MODE */
    word_idx = SPD_ID_TBL_SCR_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_SCR_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->scr_mode & SPD_ID_TBL_SCR_MODE_MASK) << bit_idx;

    /* CODEC_MODE */
    word_idx = SPD_ID_TBL_CODEC_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_CODEC_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->codec_mode & SPD_ID_TBL_CODEC_MODE_MASK) << bit_idx;

    /* TRAINING_EN */
    word_idx = SPD_ID_TBL_TRAINING_EN_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_TRAINING_EN_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->training_en & SPD_ID_TBL_TRAINING_EN_MASK) << bit_idx;

    /* AN_TIMER_SELECT */
    word_idx = SPD_ID_TBL_AN_TIMER_SELECT_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_AN_TIMER_SELECT_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->an_timer_select & SPD_ID_TBL_AN_TIMER_SELECT_MASK) << bit_idx;

    /* PMD_PAM4_MODE */
    word_idx = SPD_ID_TBL_PMD_PAM4_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_PMD_PAM4_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->pmd_pam4_mode & SPD_ID_TBL_PMD_PAM4_MODE_MASK) << bit_idx;

    /* PMD_OSR_MODE */
    word_idx = SPD_ID_TBL_PMD_OSR_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_PMD_OSR_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->pmd_osr_mode & SPD_ID_TBL_PMD_OSR_MODE_MASK) << bit_idx;

    /* T_PMA_WATERMARK */
    word_idx = SPD_ID_TBL_T_PMA_WATERMARK_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_T_PMA_WATERMARK_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->t_pma_watermark & SPD_ID_TBL_T_PMA_WATERMARK_MASK) << bit_idx;

    /* L_TPMA_WATERMARK */
    word_idx = SPD_ID_TBL_L_TPMA_WATERMARK_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_L_TPMA_WATERMARK_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->l_tpma_watermark & SPD_ID_TBL_L_TPMA_WATERMARK_MASK) << bit_idx;

    /* LOOP_BLOCK_COUNT_FOR_TS */
    word_idx = SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_OFFSET % word_size;
    low_bits = word_size - bit_idx;
    low_mask = (1 << low_bits)  -1;
    high_bits = SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_WIDTH - low_bits;
    high_mask = (1 << high_bits) - 1;
    packed_entry[word_idx] |= (entry->loop_block_count_for_ts & low_mask) << bit_idx;
    packed_entry[word_idx + 1] |= (entry->loop_block_count_for_ts >> low_bits) & high_mask;

    /* LOOP_BIT_COUNT_FOR_TS */
    word_idx = SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->loop_bit_count_for_ts & SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_MASK) << bit_idx;

    /* AM_LOCK_FSM_MODE */
    word_idx = SPD_ID_TBL_AM_LOCK_FSM_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_AM_LOCK_FSM_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->am_lock_fsm_mode & SPD_ID_TBL_AM_LOCK_FSM_MODE_MASK) << bit_idx;

    /* T_PMA_START_MODE */
    word_idx = SPD_ID_TBL_T_PMA_START_MODE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_T_PMA_START_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->t_pma_start_mode & SPD_ID_TBL_T_PMA_START_MODE_MASK) << bit_idx;

    /* HI_SER_ENABLE */
    word_idx = SPD_ID_TBL_HI_SER_ENABLE_OFFSET / word_size;
    bit_idx  = SPD_ID_TBL_HI_SER_ENABLE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->hi_ser_enable & SPD_ID_TBL_HI_SER_ENABLE_MASK) << bit_idx;

    return;
}


#ifdef _DV_TB_
/*
 * spd_ctrl_print_spd_id_tbl_entry() : Print the contents of a UM Table Entry
 */
void spd_ctrl_print_spd_id_tbl_entry(spd_id_tbl_entry_t *entry)
{
    printf("AM_TABLE_INDEX                      : 0x%0x\n", entry->am_table_index);
    printf("FEC_ARCH                            : 0x%0x\n", entry->fec_arch);
    printf("SYMBOL_INTERLEAVE                   : 0x%0x\n", entry->symbol_interleave);
    printf("TC_XOR_CONTROL                      : 0x%0x\n", entry->tc_xor_control);
    printf("RS_FEC_SYNC_HEADER_MODE             : 0x%0x\n", entry->rs_fec_sync_header_mode);
    printf("RS_FEC_CWM_NIBBLE_MATCH_COUNT       : 0x%0x\n", entry->rs_fec_cwm_nibble_match_count);
    printf("BASE_R_FEC_SYNC_HEADER_MODE         : 0x%0x\n", entry->base_r_fec_sync_header_mode);
    printf("RS_FEC_SYMBOL_ERROR_WINDOW_MODE     : 0x%0x\n", entry->rs_fec_symbol_error_window_mode);
    printf("RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD : 0x%0x\n", entry->rs_fec_symbol_error_count_threshold);
    printf("DESKEW_FORWARDING_THRESHOLD_2XN     : 0x%0x\n", entry->deskew_forwarding_threshold_2xn);
    printf("DESKEW_FORWARDING_THRESHOLD_1XN     : 0x%0x\n", entry->deskew_forwarding_threshold_1xn);
    printf("BIT_MUX_MODE                        : 0x%0x\n", entry->bit_mux_mode);
    printf("NUM_LANES                           : 0x%0x\n", entry->num_lanes);
    printf("DESKEW_WINDOW_WITHOUT_RS_FEC        : 0x%0x\n", entry->deskew_window_without_rs_fec);
    printf("DESKEW_WINDOW_WITH_RS_FEC           : 0x%0x\n", entry->deskew_window_with_rs_fec);
    printf("BER_FSM_DISABLE_WITH_RS_FEC         : 0x%0x\n", entry->ber_fsm_disable_with_rs_fec);
    printf("BER_FSM_DISABLE_WITHOUT_RS_FEC      : 0x%0x\n", entry->ber_fsm_disable_without_rs_fec);
    printf("BER_WINDOW_MODE                     : 0x%0x\n", entry->ber_window_mode);
    printf("BER_TRIGGER_COUNT                   : 0x%0x\n", entry->ber_trigger_count);
    printf("CREDIT_QUOTIENT                     : 0x%0x\n", entry->credit_quotient);
    printf("CREDIT_REMAINDER                    : 0x%0x\n", entry->credit_remainder);
    printf("CREDIT_DIVISOR                      : 0x%0x\n", entry->credit_divisor);
    printf("USE_CL49_BLOCK_SYNC                 : 0x%0x\n", entry->use_cl49_block_sync);
    printf("SCR_MODE                            : 0x%0x\n", entry->scr_mode);
    printf("CODEC_MODE                          : 0x%0x\n", entry->codec_mode);
    printf("TRAINING_EN                         : 0x%0x\n", entry->training_en);
    printf("AN_TIMER_SELECT                     : 0x%0x\n", entry->an_timer_select);
    printf("PMD_PAM4_MODE                       : 0x%0x\n", entry->pmd_pam4_mode);
    printf("PMD_OSR_MODE                        : 0x%0x\n", entry->pmd_osr_mode);
    printf("T_PMA_WATERMARK                     : 0x%0x\n", entry->t_pma_watermark);
    printf("L_TPMA_WATERMARK                    : 0x%0x\n", entry->l_tpma_watermark);
    printf("LOOP_BLOCK_COUNT_FOR_TS             : 0x%0x\n", entry->loop_block_count_for_ts);
    printf("LOOP_BIT_COUNT_FOR_TS               : 0x%0x\n", entry->loop_bit_count_for_ts);
    printf("AM_LOCK_FSM_MODE                    : 0x%0x\n", entry->am_lock_fsm_mode);
    printf("T_PMA_START_MODE                    : 0x%0x\n", entry->t_pma_start_mode);

    return;
}
#endif

/*
 * plp_aperta_spd_ctrl_unpack_am_tbl_entry() : Unpacks a char array into an am_tbl_entry_t
 */
void plp_aperta_spd_ctrl_unpack_am_tbl_entry(uint32_t *packed_entry, am_tbl_entry_t *entry)
{
    int word_size = 32;
    int word_idx;
    int bit_idx;
    int high_bits;
    int low_bits;
    int high_mask;
    int low_mask;

    /* AM_COMPRESSION_MODE */
    word_idx = AM_TBL_AM_COMPRESSION_MODE_OFFSET / word_size;
    bit_idx  = AM_TBL_AM_COMPRESSION_MODE_OFFSET % word_size;
    entry->am_compression_mode = (packed_entry[word_idx] >> bit_idx) & AM_TBL_AM_COMPRESSION_MODE_MASK;

    /* AM_SIZE */
    word_idx = AM_TBL_AM_SIZE_OFFSET / word_size;
    bit_idx  = AM_TBL_AM_SIZE_OFFSET % word_size;
    entry->am_size = (packed_entry[word_idx] >> bit_idx) & AM_TBL_AM_SIZE_MASK;

    /* PAD_MODE */
    word_idx = AM_TBL_PAD_MODE_OFFSET / word_size;
    bit_idx  = AM_TBL_PAD_MODE_OFFSET % word_size;
    entry->pad_mode = (packed_entry[word_idx] >> bit_idx) & AM_TBL_PAD_MODE_MASK;

    /* UM_TABLE_COUNT */
    word_idx = AM_TBL_UM_TABLE_COUNT_OFFSET / word_size;
    bit_idx  = AM_TBL_UM_TABLE_COUNT_OFFSET % word_size;
    entry->um_table_count = (packed_entry[word_idx] >> bit_idx) & AM_TBL_UM_TABLE_COUNT_MASK;

    /* UM_TABLE_INDEX */
    word_idx = AM_TBL_UM_TABLE_INDEX_OFFSET / word_size;
    bit_idx  = AM_TBL_UM_TABLE_INDEX_OFFSET % word_size;
    entry->um_table_index = (packed_entry[word_idx] >> bit_idx) & AM_TBL_UM_TABLE_INDEX_MASK;

    /* CM */
    word_idx = AM_TBL_CM_OFFSET / word_size;
    bit_idx  = AM_TBL_CM_OFFSET % word_size;
    low_bits = word_size - bit_idx;
    low_mask = (1 << low_bits)  -1;
    high_bits = AM_TBL_CM_WIDTH - low_bits;
    high_mask = (1 << high_bits) - 1;
    entry->cm = (packed_entry[word_idx] >> bit_idx) & low_mask;
    entry->cm |= (packed_entry[word_idx + 1] & high_mask) << low_bits;

    /* USE_FIXED */
    word_idx = AM_TBL_USE_FIXED_OFFSET / word_size;
    bit_idx  = AM_TBL_USE_FIXED_OFFSET % word_size;
    entry->use_fixed = (packed_entry[word_idx] >> bit_idx) & AM_TBL_USE_FIXED_MASK;

    /* AM_SPACING_RS */
    word_idx = AM_TBL_AM_SPACING_RS_OFFSET / word_size;
    bit_idx  = AM_TBL_AM_SPACING_RS_OFFSET % word_size;
    entry->am_spacing_rs = (packed_entry[word_idx] >> bit_idx) & AM_TBL_AM_SPACING_RS_MASK;

    /* AM_SPACING_NO_RS */
    word_idx = AM_TBL_AM_SPACING_NO_RS_OFFSET / word_size;
    bit_idx  = AM_TBL_AM_SPACING_NO_RS_OFFSET % word_size;
    low_bits = word_size - bit_idx;
    low_mask = (1 << low_bits)  -1;
    high_bits = AM_TBL_AM_SPACING_NO_RS_WIDTH - low_bits;
    high_mask = (1 << high_bits) - 1;
    entry->am_spacing_no_rs = (packed_entry[word_idx] >> bit_idx) & low_mask;
    entry->am_spacing_no_rs |= (packed_entry[word_idx + 1] & high_mask) << low_bits;

    /* AM_SPACING_CREDITS */
    word_idx = AM_TBL_AM_SPACING_CREDITS_OFFSET / word_size;
    bit_idx  = AM_TBL_AM_SPACING_CREDITS_OFFSET % word_size;
    entry->am_spacing_credits = (packed_entry[word_idx] >> bit_idx) & AM_TBL_AM_SPACING_CREDITS_MASK;

    return;
}


/*
 * plp_aperta_spd_ctrl_pack_am_tbl_entry() : Packet an am_tbl_entry_t into an array of 32-bit blocks
 */
void plp_aperta_spd_ctrl_pack_am_tbl_entry(am_tbl_entry_t *entry, uint32_t *packed_entry)
{
    int num_words = 3;
    int word_size = 32;
    int word_idx = 0;
    int bit_idx = 0;
    int high_bits;
    int low_bits;
    int high_mask;
    int low_mask;
    int i;

    /* Initialize the packed_entry array to 0's */
    for (i = 0; i < num_words; i++) {
        packed_entry[i] = 0;
    }

    /* AM_COMPRESSION_MODE */
    word_idx = AM_TBL_AM_COMPRESSION_MODE_OFFSET / word_size;    
    bit_idx  = AM_TBL_AM_COMPRESSION_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->am_compression_mode & AM_TBL_AM_COMPRESSION_MODE_MASK) << bit_idx;

    /* AM_SIZE */
    word_idx = AM_TBL_AM_SIZE_OFFSET / word_size;    
    bit_idx  = AM_TBL_AM_SIZE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->am_size & AM_TBL_AM_SIZE_MASK) << bit_idx;

    /* PAD_MODE */
    word_idx = AM_TBL_PAD_MODE_OFFSET / word_size;    
    bit_idx  = AM_TBL_PAD_MODE_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->pad_mode & AM_TBL_PAD_MODE_MASK) << bit_idx;

    /* UM_TABLE_COUNT */
    word_idx = AM_TBL_UM_TABLE_COUNT_OFFSET / word_size;    
    bit_idx  = AM_TBL_UM_TABLE_COUNT_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->um_table_count & AM_TBL_UM_TABLE_COUNT_MASK) << bit_idx;

    /* UM_TABLE_INDEX */
    word_idx = AM_TBL_UM_TABLE_INDEX_OFFSET / word_size;    
    bit_idx  = AM_TBL_UM_TABLE_INDEX_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->um_table_index & AM_TBL_UM_TABLE_INDEX_MASK) << bit_idx;

    /* CM */
    word_idx = AM_TBL_CM_OFFSET / word_size;    
    bit_idx  = AM_TBL_CM_OFFSET % word_size;
    low_bits  = 32 - bit_idx;
    low_mask  = (1 << low_bits) - 1;
    high_bits = AM_TBL_CM_WIDTH - low_bits;
    high_mask  = (1 << high_bits) - 1;
    packed_entry[word_idx] |= (entry->cm & low_mask) << bit_idx;
    packed_entry[word_idx + 1] |= (entry->cm & high_mask) >> low_bits;

    /* USE_FIXED */
    word_idx = AM_TBL_USE_FIXED_OFFSET / word_size;    
    bit_idx  = AM_TBL_USE_FIXED_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->use_fixed & AM_TBL_USE_FIXED_MASK) << bit_idx;

    /* AM_SPACING_RS */
    word_idx = AM_TBL_AM_SPACING_RS_OFFSET / word_size;    
    bit_idx  = AM_TBL_AM_SPACING_RS_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->am_spacing_rs & AM_TBL_AM_SPACING_RS_MASK) << bit_idx;

    /* AM_SPACING_NO_RS */
    word_idx = AM_TBL_AM_SPACING_NO_RS_OFFSET / word_size;    
    bit_idx  = AM_TBL_AM_SPACING_NO_RS_OFFSET % word_size;
    low_bits  = 32 - bit_idx;
    low_mask  = (1 << low_bits) - 1;
    high_bits = AM_TBL_AM_SPACING_NO_RS_WIDTH - low_bits;
    high_mask  = (1 << high_bits) - 1;
    packed_entry[word_idx] |= (entry->am_spacing_no_rs & low_mask) << bit_idx;
    packed_entry[word_idx + 1] |= (entry->am_spacing_no_rs & high_mask) >> low_bits;

    /* AM_SPACING_CREDITS */
    word_idx = AM_TBL_AM_SPACING_CREDITS_OFFSET / word_size;    
    bit_idx  = AM_TBL_AM_SPACING_CREDITS_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->am_spacing_credits & AM_TBL_AM_SPACING_CREDITS_MASK) << bit_idx;

    return;
}

#ifdef _DV_TB_
/*
 * spd_ctrl_print_am_tbl_entry() : Print the contents of a UM Table Entry
 */
void spd_ctrl_print_am_tbl_entry(am_tbl_entry_t *entry)
{
   printf("AM_SPACING_CREDITS  : 0x%0x\n", entry->am_spacing_credits);
   printf("AM_SPACING_NO_RS    : 0x%0x\n", entry->am_spacing_no_rs);
   printf("AM_SPACING_RS       : 0x%0x\n", entry->am_spacing_rs);
   printf("USE_FIXED           : 0x%0x\n", entry->use_fixed);
   printf("CM                  : 0x%0x\n", entry->cm);
   printf("UM_TABLE_INDEX      : 0x%0x\n", entry->um_table_index);
   printf("UM_TABLE_COUNT      : 0x%0x\n", entry->um_table_count);
   printf("PAD_MODE            : 0x%0x\n", entry->pad_mode);
   printf("AM_SIZE             : 0x%0x\n", entry->am_size);
   printf("AM_COMPRESSION_MODE : 0x%0x\n", entry->am_compression_mode);
   return;
}
#endif

/*
 * plp_aperta_spd_ctrl_unpack_um_tbl_entry() : Unpacks a uint8_t array into an um_tbl_entry_t
 */
void plp_aperta_spd_ctrl_unpack_um_tbl_entry(uint32_t *packed_entry, um_tbl_entry_t *entry)
{
    int word_size = 32;
    int word_idx;
    int bit_idx;
    int high_bits;
    int low_bits;
    int high_mask;
    int low_mask;

    /* CM_CONTROL */
    word_idx = UM_TBL_CM_CONTROL_OFFSET / word_size;
    bit_idx  = UM_TBL_CM_CONTROL_OFFSET % word_size;
    entry->cm_control = (packed_entry[word_idx] >> bit_idx) & UM_TBL_CM_CONTROL_MASK;
    
    /* UM_PAD */
    word_idx = UM_TBL_UM_PAD_OFFSET / word_size;
    bit_idx  = UM_TBL_UM_PAD_OFFSET % word_size;
    entry->um_pad = (packed_entry[word_idx] >> bit_idx) & UM_TBL_UM_PAD_MASK;

    /* CM_PAD */
    word_idx = UM_TBL_CM_PAD_OFFSET / word_size;
    bit_idx  = UM_TBL_CM_PAD_OFFSET % word_size;
        entry->cm_pad = (packed_entry[word_idx] >> bit_idx) & UM_TBL_CM_PAD_MASK;

    /* UM */
    word_idx = UM_TBL_UM_OFFSET / word_size;
    bit_idx  = UM_TBL_UM_OFFSET % word_size;
    low_bits = word_size - bit_idx;
    low_mask = (1 << low_bits)  -1;
    high_bits = UM_TBL_UM_WIDTH - low_bits;
    high_mask =  (1 << high_bits) - 1;
    entry->um = (packed_entry[word_idx] >> bit_idx) & low_mask;
    entry->um |= (packed_entry[word_idx + 1] & high_mask) << low_bits;

    return;
}

/*
 * plp_aperta_spd_ctrl_pack_um_tbl_entry() : Packet an am_tbl_entry_t into an array of 32-bit blocks
 */
void plp_aperta_spd_ctrl_pack_um_tbl_entry(um_tbl_entry_t *entry, uint32_t *packed_entry)
{
    int num_words = 2;
    int word_size = 32;
    int word_idx = 0;
    int bit_idx = 0;
    int high_bits;
    int low_bits;
    int high_mask;
    int low_mask;
    int i;

    /* Initialize the packed_entry array to 0's */
    for (i = 0; i < num_words; i++) {
        packed_entry[i] = 0;
    }

    /* CM_CONTROL */
    word_idx = UM_TBL_CM_CONTROL_OFFSET / word_size;    
    bit_idx  = UM_TBL_CM_CONTROL_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->cm_control & UM_TBL_CM_CONTROL_MASK) << bit_idx;

    /* UM_PAD */
    word_idx = UM_TBL_UM_PAD_OFFSET / word_size;    
    bit_idx  = UM_TBL_UM_PAD_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->um_pad & UM_TBL_UM_PAD_MASK) << bit_idx;

    /* CM_PAD */
    word_idx = UM_TBL_CM_PAD_OFFSET / word_size;    
    bit_idx  = UM_TBL_CM_PAD_OFFSET % word_size;
    packed_entry[word_idx] |= (entry->cm_pad & UM_TBL_CM_PAD_MASK) << bit_idx;

    /* UM */
    word_idx = UM_TBL_UM_OFFSET / word_size;    
    bit_idx  = UM_TBL_UM_OFFSET % word_size;
    low_bits  = word_size - bit_idx;
    low_mask  = (1 << low_bits) - 1;
    high_bits = UM_TBL_UM_WIDTH - low_bits;
    high_mask  = (1 << high_bits) - 1;
    packed_entry[word_idx] |= (entry->um & low_mask) << bit_idx;
    packed_entry[word_idx + 1] |= (entry->um >> low_bits) & high_mask;

    return;
}

#ifdef _DV_TB_
/*
 * spd_ctrl_print_um_tbl_entry() : Print the contents of a UM Table Entry
 */
void spd_ctrl_print_um_tbl_entry(um_tbl_entry_t *entry) 
{
   printf("UM         : 0x%0x\n", entry->um);
   printf("CM_PAD     : 0x%0x\n", entry->cm_pad);
   printf("UM_PAD     : 0x%0x\n", entry->um_pad);
   printf("CM_CONTROL : 0x%0x\n", entry->cm_control);
   return;
}
#endif
#endif  /* _APERTA_TBHMOD_SPD_CTRL_C_ */
