/*------------------------------------------------------------------------------
 * $Id: tscpmod_spd_ctrl.c $
 *
 * $Copyright: (c) 2017 Broadcom Corporation All Rights Reserved.$
 *  Broadcom Corporation
 *  Proprietary and Confidential information
 *  All rights reserved
 *  This source file is the property of Broadcom Corporation, and
 *  may not be copied or distributed in any isomorphic form without the
 *  prior written consent of Broadcom Corporation.
 *------------------------------------------------------------------------------
 *  Description: Functons for Speed Table, AM Table, and UM Table access.
 *----------------------------------------------------------------------------*/
#ifndef _TSCPMOD_SPD_CTRL_C_
#define _TSCPMOD_SPD_CTRL_C_
 
#include "tscpmod_spd_ctrl_defines.h"
#include "tscpmod_spd_ctrl.h"

/*
 * spd_ctrl_unpack_spd_id_tbl_entry() : Unpacks a char array into a spd_id_tbl_entry_t
 */
void plp_aperta2_tscpmod_spd_ctrl_unpack_spd_id_tbl_entry(uint32_t *packed_entry, tscpmod_spd_id_tbl_entry_t *entry)
{
    int word_size = 32;
    int word_idx;
    int bit_idx;
    int high_bits;
    int low_bits;
    int high_mask;
    int low_mask;
   
    /* AM_TABLE_INDEX */
    word_idx = TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->am_table_index = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->am_table_index |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->am_table_index = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_MASK;
    }

    /* FEC_ARCH */
    word_idx = TSCPMOD_SPD_ID_TBL_FEC_ARCH_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_FEC_ARCH_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_FEC_ARCH_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_FEC_ARCH_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->fec_arch = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->fec_arch |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->fec_arch = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_FEC_ARCH_MASK;
    }

    /* TC_XOR_CONTROL */
    word_idx = TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->tc_xor_control = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->tc_xor_control |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->tc_xor_control = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_MASK;
    }

    /* RS_FEC_SYNC_HEADER_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->rs_fec_sync_header_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->rs_fec_sync_header_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->rs_fec_sync_header_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_MASK;
    }

    /* RS_FEC_CWM_NIBBLE_MATCH_COUNT */
    word_idx = TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->rs_fec_cwm_nibble_match_count = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->rs_fec_cwm_nibble_match_count |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->rs_fec_cwm_nibble_match_count = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_MASK;
    }

    /* RS_FEC_SYMBOL_ERROR_WINDOW_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->rs_fec_symbol_error_window_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->rs_fec_symbol_error_window_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->rs_fec_symbol_error_window_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_MASK;
    }

    /* RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD */
    word_idx = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->rs_fec_symbol_error_count_threshold = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->rs_fec_symbol_error_count_threshold |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->rs_fec_symbol_error_count_threshold = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_MASK;
    }

    /* DESKEW_FORWARDING_THRESHOLD_2XN */
    word_idx = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->deskew_forwarding_threshold_2xn = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->deskew_forwarding_threshold_2xn |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->deskew_forwarding_threshold_2xn = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_MASK;
    }

    /* DESKEW_FORWARDING_THRESHOLD_1XN */
    word_idx = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->deskew_forwarding_threshold_1xn = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->deskew_forwarding_threshold_1xn |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->deskew_forwarding_threshold_1xn = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_MASK;
    }

    /* BIT_MUX_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->bit_mux_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->bit_mux_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->bit_mux_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_MASK;
    }

    /* NUM_LANES */
    word_idx = TSCPMOD_SPD_ID_TBL_NUM_LANES_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_NUM_LANES_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_NUM_LANES_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_NUM_LANES_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->num_lanes = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->num_lanes |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->num_lanes = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_NUM_LANES_MASK;
    }

    /* DESKEW_WINDOW_WITHOUT_RS_FEC */
    word_idx = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->deskew_window_without_rs_fec = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->deskew_window_without_rs_fec |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->deskew_window_without_rs_fec = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_MASK;
    }

    /* DESKEW_WINDOW_WITH_RS_FEC */
    word_idx = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->deskew_window_with_rs_fec = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->deskew_window_with_rs_fec |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->deskew_window_with_rs_fec = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_MASK;
    }

    /* BER_FSM_DISABLE_WITH_RS_FEC */
    word_idx = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->ber_fsm_disable_with_rs_fec = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->ber_fsm_disable_with_rs_fec |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->ber_fsm_disable_with_rs_fec = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_MASK;
    }

    /* BER_FSM_DISABLE_WITHOUT_RS_FEC */
    word_idx = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->ber_fsm_disable_without_rs_fec = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->ber_fsm_disable_without_rs_fec |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->ber_fsm_disable_without_rs_fec = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_MASK;
    }

    /* BER_WINDOW_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->ber_window_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->ber_window_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->ber_window_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_MASK;
    }

    /* BER_TRIGGER_COUNT */
    word_idx = TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->ber_trigger_count = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->ber_trigger_count |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->ber_trigger_count = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_MASK;
    }

    /* CREDIT_QUOTIENT */
    word_idx = TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->credit_quotient = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->credit_quotient |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->credit_quotient = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_MASK;
    }

    /* USE_CL49_BLOCK_SYNC */
    word_idx = TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->use_cl49_block_sync = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->use_cl49_block_sync |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->use_cl49_block_sync = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_MASK;
    }

    /* SCR_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_SCR_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_SCR_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_SCR_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_SCR_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->scr_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->scr_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->scr_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_SCR_MODE_MASK;
    }

    /* CODEC_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_CODEC_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_CODEC_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_CODEC_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_CODEC_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->codec_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->codec_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->codec_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_CODEC_MODE_MASK;
    }

    /* TRAINING_EN */
    word_idx = TSCPMOD_SPD_ID_TBL_TRAINING_EN_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_TRAINING_EN_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_TRAINING_EN_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_TRAINING_EN_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->training_en = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->training_en |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->training_en = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_TRAINING_EN_MASK;
    }

    /* SCRAMBLER_DIS */
    word_idx = TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->scrambler_dis = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->scrambler_dis |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->scrambler_dis = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_MASK;
    }

    /* AN_TIMER_SELECT */
    word_idx = TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->an_timer_select = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->an_timer_select |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->an_timer_select = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_MASK;
    }

    /* T_PMA_WATERMARK */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->t_pma_watermark = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->t_pma_watermark |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->t_pma_watermark = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_MASK;
    }

    /* LOOP_BLOCK_COUNT_FOR_TS */
    word_idx = TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->loop_block_count_for_ts = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->loop_block_count_for_ts |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->loop_block_count_for_ts = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_MASK;
    }

    /* LOOP_BIT_COUNT_FOR_TS */
    word_idx = TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->loop_bit_count_for_ts = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->loop_bit_count_for_ts |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->loop_bit_count_for_ts = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_MASK;
    }

    /* AM_LOCK_FSM_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->am_lock_fsm_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->am_lock_fsm_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->am_lock_fsm_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_MASK;
    }

    /* T_PMA_START_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->t_pma_start_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->t_pma_start_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->t_pma_start_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_MASK;
    }

    /* HI_SER_ENABLE */
    word_idx = TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->hi_ser_enable = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->hi_ser_enable |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->hi_ser_enable = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_MASK;
    }

    /* T_PMA_INPUT_WIDTH_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->t_pma_input_width_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->t_pma_input_width_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->t_pma_input_width_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_MASK;
    }

    /* T_PMA_OUTPUT_WIDTH_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->t_pma_output_width_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->t_pma_output_width_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->t_pma_output_width_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_MASK;
    }

    /* HI_SER_2 */
    word_idx = TSCPMOD_SPD_ID_TBL_HI_SER_2_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_HI_SER_2_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_HI_SER_2_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_HI_SER_2_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->hi_ser_2 = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->hi_ser_2 |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->hi_ser_2 = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_HI_SER_2_MASK;
    }

    /* T_PMA_TDM */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_TDM_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_TDM_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_TDM_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_TDM_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->t_pma_tdm = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->t_pma_tdm |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->t_pma_tdm = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_SPD_ID_TBL_T_PMA_TDM_MASK;
    }

    return;
}

/*
 * spd_ctrl_pack_spd_id_tbl_entry() : Pack a spd_id_tbl_entry_t into an array of 32-bit blocks
 */
void plp_aperta2_tscpmod_spd_ctrl_pack_spd_id_tbl_entry(tscpmod_spd_id_tbl_entry_t *entry, uint32_t *packed_entry)
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
    word_idx = TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->am_table_index & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->am_table_index >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->am_table_index & TSCPMOD_SPD_ID_TBL_AM_TABLE_INDEX_MASK) << bit_idx;
    }

    /* FEC_ARCH */
    word_idx = TSCPMOD_SPD_ID_TBL_FEC_ARCH_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_FEC_ARCH_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_FEC_ARCH_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_FEC_ARCH_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->fec_arch & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->fec_arch >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->fec_arch & TSCPMOD_SPD_ID_TBL_FEC_ARCH_MASK) << bit_idx;
    }

    /* TC_XOR_CONTROL */
    word_idx = TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->tc_xor_control & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->tc_xor_control >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->tc_xor_control & TSCPMOD_SPD_ID_TBL_TC_XOR_CONTROL_MASK) << bit_idx;
    }

    /* RS_FEC_SYNC_HEADER_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->rs_fec_sync_header_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->rs_fec_sync_header_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->rs_fec_sync_header_mode & TSCPMOD_SPD_ID_TBL_RS_FEC_SYNC_HEADER_MODE_MASK) << bit_idx;
    }

    /* RS_FEC_CWM_NIBBLE_MATCH_COUNT */
    word_idx = TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->rs_fec_cwm_nibble_match_count & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->rs_fec_cwm_nibble_match_count >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->rs_fec_cwm_nibble_match_count & TSCPMOD_SPD_ID_TBL_RS_FEC_CWM_NIBBLE_MATCH_COUNT_MASK) << bit_idx;
    }

    /* RS_FEC_SYMBOL_ERROR_WINDOW_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->rs_fec_symbol_error_window_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->rs_fec_symbol_error_window_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->rs_fec_symbol_error_window_mode & TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_WINDOW_MODE_MASK) << bit_idx;
    }

    /* RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD */
    word_idx = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->rs_fec_symbol_error_count_threshold & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->rs_fec_symbol_error_count_threshold >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->rs_fec_symbol_error_count_threshold & TSCPMOD_SPD_ID_TBL_RS_FEC_SYMBOL_ERROR_COUNT_THRESHOLD_MASK) << bit_idx;
    }

    /* DESKEW_FORWARDING_THRESHOLD_2XN */
    word_idx = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->deskew_forwarding_threshold_2xn & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->deskew_forwarding_threshold_2xn >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->deskew_forwarding_threshold_2xn & TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_2XN_MASK) << bit_idx;
    }

    /* DESKEW_FORWARDING_THRESHOLD_1XN */
    word_idx = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->deskew_forwarding_threshold_1xn & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->deskew_forwarding_threshold_1xn >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->deskew_forwarding_threshold_1xn & TSCPMOD_SPD_ID_TBL_DESKEW_FORWARDING_THRESHOLD_1XN_MASK) << bit_idx;
    }

    /* BIT_MUX_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->bit_mux_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->bit_mux_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->bit_mux_mode & TSCPMOD_SPD_ID_TBL_BIT_MUX_MODE_MASK) << bit_idx;
    }

    /* NUM_LANES */
    word_idx = TSCPMOD_SPD_ID_TBL_NUM_LANES_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_NUM_LANES_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_NUM_LANES_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_NUM_LANES_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->num_lanes & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->num_lanes >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->num_lanes & TSCPMOD_SPD_ID_TBL_NUM_LANES_MASK) << bit_idx;
    }

    /* DESKEW_WINDOW_WITHOUT_RS_FEC */
    word_idx = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->deskew_window_without_rs_fec & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->deskew_window_without_rs_fec >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->deskew_window_without_rs_fec & TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITHOUT_RS_FEC_MASK) << bit_idx;
    }

    /* DESKEW_WINDOW_WITH_RS_FEC */
    word_idx = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->deskew_window_with_rs_fec & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->deskew_window_with_rs_fec >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->deskew_window_with_rs_fec & TSCPMOD_SPD_ID_TBL_DESKEW_WINDOW_WITH_RS_FEC_MASK) << bit_idx;
    }

    /* BER_FSM_DISABLE_WITH_RS_FEC */
    word_idx = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->ber_fsm_disable_with_rs_fec & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->ber_fsm_disable_with_rs_fec >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->ber_fsm_disable_with_rs_fec & TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITH_RS_FEC_MASK) << bit_idx;
    }

    /* BER_FSM_DISABLE_WITHOUT_RS_FEC */
    word_idx = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->ber_fsm_disable_without_rs_fec & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->ber_fsm_disable_without_rs_fec >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->ber_fsm_disable_without_rs_fec & TSCPMOD_SPD_ID_TBL_BER_FSM_DISABLE_WITHOUT_RS_FEC_MASK) << bit_idx;
    }

    /* BER_WINDOW_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->ber_window_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->ber_window_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->ber_window_mode & TSCPMOD_SPD_ID_TBL_BER_WINDOW_MODE_MASK) << bit_idx;
    }

    /* BER_TRIGGER_COUNT */
    word_idx = TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->ber_trigger_count & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->ber_trigger_count >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->ber_trigger_count & TSCPMOD_SPD_ID_TBL_BER_TRIGGER_COUNT_MASK) << bit_idx;
    }

    /* CREDIT_QUOTIENT */
    word_idx = TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->credit_quotient & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->credit_quotient >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->credit_quotient & TSCPMOD_SPD_ID_TBL_CREDIT_QUOTIENT_MASK) << bit_idx;
    }

    /* USE_CL49_BLOCK_SYNC */
    word_idx = TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->use_cl49_block_sync & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->use_cl49_block_sync >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->use_cl49_block_sync & TSCPMOD_SPD_ID_TBL_USE_CL49_BLOCK_SYNC_MASK) << bit_idx;
    }

    /* SCR_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_SCR_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_SCR_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_SCR_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_SCR_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->scr_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->scr_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->scr_mode & TSCPMOD_SPD_ID_TBL_SCR_MODE_MASK) << bit_idx;
    }

    /* CODEC_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_CODEC_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_CODEC_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_CODEC_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_CODEC_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->codec_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->codec_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->codec_mode & TSCPMOD_SPD_ID_TBL_CODEC_MODE_MASK) << bit_idx;
    }

    /* TRAINING_EN */
    word_idx = TSCPMOD_SPD_ID_TBL_TRAINING_EN_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_TRAINING_EN_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_TRAINING_EN_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_TRAINING_EN_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->training_en & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->training_en >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->training_en & TSCPMOD_SPD_ID_TBL_TRAINING_EN_MASK) << bit_idx;
    }

    /* SCRAMBLER_DIS */
    word_idx = TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->scrambler_dis & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->scrambler_dis >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->scrambler_dis & TSCPMOD_SPD_ID_TBL_SCRAMBLER_DIS_MASK) << bit_idx;
    }

    /* AN_TIMER_SELECT */
    word_idx = TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->an_timer_select & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->an_timer_select >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->an_timer_select & TSCPMOD_SPD_ID_TBL_AN_TIMER_SELECT_MASK) << bit_idx;
    }

    /* T_PMA_WATERMARK */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->t_pma_watermark & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->t_pma_watermark >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->t_pma_watermark & TSCPMOD_SPD_ID_TBL_T_PMA_WATERMARK_MASK) << bit_idx;
    }

    /* LOOP_BLOCK_COUNT_FOR_TS */
    word_idx = TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->loop_block_count_for_ts & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->loop_block_count_for_ts >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->loop_block_count_for_ts & TSCPMOD_SPD_ID_TBL_LOOP_BLOCK_COUNT_FOR_TS_MASK) << bit_idx;
    }

    /* LOOP_BIT_COUNT_FOR_TS */
    word_idx = TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->loop_bit_count_for_ts & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->loop_bit_count_for_ts >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->loop_bit_count_for_ts & TSCPMOD_SPD_ID_TBL_LOOP_BIT_COUNT_FOR_TS_MASK) << bit_idx;
    }

    /* AM_LOCK_FSM_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->am_lock_fsm_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->am_lock_fsm_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->am_lock_fsm_mode & TSCPMOD_SPD_ID_TBL_AM_LOCK_FSM_MODE_MASK) << bit_idx;
    }

    /* T_PMA_START_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->t_pma_start_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->t_pma_start_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->t_pma_start_mode & TSCPMOD_SPD_ID_TBL_T_PMA_START_MODE_MASK) << bit_idx;
    }

    /* HI_SER_ENABLE */
    word_idx = TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->hi_ser_enable & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->hi_ser_enable >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->hi_ser_enable & TSCPMOD_SPD_ID_TBL_HI_SER_ENABLE_MASK) << bit_idx;
    }

    /* T_PMA_INPUT_WIDTH_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->t_pma_input_width_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->t_pma_input_width_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->t_pma_input_width_mode & TSCPMOD_SPD_ID_TBL_T_PMA_INPUT_WIDTH_MODE_MASK) << bit_idx;
    }

    /* T_PMA_OUTPUT_WIDTH_MODE */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->t_pma_output_width_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->t_pma_output_width_mode >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->t_pma_output_width_mode & TSCPMOD_SPD_ID_TBL_T_PMA_OUTPUT_WIDTH_MODE_MASK) << bit_idx;
    }

    /* HI_SER_2 */
    word_idx = TSCPMOD_SPD_ID_TBL_HI_SER_2_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_HI_SER_2_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_HI_SER_2_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_HI_SER_2_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->hi_ser_2 & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->hi_ser_2 >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->hi_ser_2 & TSCPMOD_SPD_ID_TBL_HI_SER_2_MASK) << bit_idx;
    }

    /* T_PMA_TDM */
    word_idx = TSCPMOD_SPD_ID_TBL_T_PMA_TDM_OFFSET / word_size;
    bit_idx  = TSCPMOD_SPD_ID_TBL_T_PMA_TDM_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_SPD_ID_TBL_T_PMA_TDM_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_SPD_ID_TBL_T_PMA_TDM_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->t_pma_tdm & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->t_pma_tdm >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->t_pma_tdm & TSCPMOD_SPD_ID_TBL_T_PMA_TDM_MASK) << bit_idx;
    }

    return;
}



/*
 * spd_ctrl_unpack_am_tbl_entry() : Unpacks a char array into an am_tbl_entry_t
 */
void plp_aperta2_tscpmod_spd_ctrl_unpack_am_tbl_entry(uint32_t *packed_entry, tscpmod_am_tbl_entry_t *entry)
{
    int word_size = 32;
    int word_idx;
    int bit_idx;
    int high_bits;
    int low_bits;
    int high_mask;
    int low_mask;

    /* AM_COMPRESSION_MODE */
    word_idx = TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->am_compression_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->am_compression_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->am_compression_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_MASK;
    }

    /* AM_SIZE */
    word_idx = TSCPMOD_AM_TBL_AM_SIZE_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_AM_SIZE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_SIZE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_AM_SIZE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->am_size = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->am_size |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->am_size = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_AM_SIZE_MASK;
    }

    /* PAD_MODE */
    word_idx = TSCPMOD_AM_TBL_PAD_MODE_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_PAD_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_PAD_MODE_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_PAD_MODE_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->pad_mode = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->pad_mode |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->pad_mode = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_PAD_MODE_MASK;
    }

    /* UM_TABLE_COUNT */
    word_idx = TSCPMOD_AM_TBL_UM_TABLE_COUNT_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_UM_TABLE_COUNT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_UM_TABLE_COUNT_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_UM_TABLE_COUNT_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->um_table_count = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->um_table_count |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->um_table_count = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_UM_TABLE_COUNT_MASK;
    }

    /* UM_TABLE_INDEX */
    word_idx = TSCPMOD_AM_TBL_UM_TABLE_INDEX_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_UM_TABLE_INDEX_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_UM_TABLE_INDEX_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_UM_TABLE_INDEX_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->um_table_index = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->um_table_index |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->um_table_index = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_UM_TABLE_INDEX_MASK;
    }

    /* CM */
    word_idx = TSCPMOD_AM_TBL_CM_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_CM_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_CM_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_CM_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->cm = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->cm |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->cm = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_CM_MASK;
    }

    /* USE_FIXED */
    word_idx = TSCPMOD_AM_TBL_USE_FIXED_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_USE_FIXED_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_USE_FIXED_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_USE_FIXED_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->use_fixed = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->use_fixed |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->use_fixed = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_USE_FIXED_MASK;
    }

    /* AM_SPACING_RS */
    word_idx = TSCPMOD_AM_TBL_AM_SPACING_RS_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_AM_SPACING_RS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_SPACING_RS_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_AM_SPACING_RS_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->am_spacing_rs = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->am_spacing_rs |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->am_spacing_rs = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_AM_SPACING_RS_MASK;
    }

    /* AM_SPACING_NO_RS */
    word_idx = TSCPMOD_AM_TBL_AM_SPACING_NO_RS_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_AM_SPACING_NO_RS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_SPACING_NO_RS_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_AM_SPACING_NO_RS_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->am_spacing_no_rs = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->am_spacing_no_rs |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->am_spacing_no_rs = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_AM_SPACING_NO_RS_MASK;
    }

    /* AM_SPACING_CREDITS */
    word_idx = TSCPMOD_AM_TBL_AM_SPACING_CREDITS_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_AM_SPACING_CREDITS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_SPACING_CREDITS_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_AM_SPACING_CREDITS_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->am_spacing_credits = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->am_spacing_credits |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->am_spacing_credits = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_AM_SPACING_CREDITS_MASK;
    }

    /* CM_AM_SEL */
    word_idx = TSCPMOD_AM_TBL_CM_AM_SEL_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_CM_AM_SEL_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_CM_AM_SEL_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_CM_AM_SEL_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->cm_am_sel = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->cm_am_sel |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->cm_am_sel = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_CM_AM_SEL_MASK;
    }

    /* AM_DROP_CONTROL */
    word_idx = TSCPMOD_AM_TBL_AM_DROP_CONTROL_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_AM_DROP_CONTROL_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_DROP_CONTROL_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_AM_DROP_CONTROL_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->am_drop_control = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->am_drop_control |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->am_drop_control = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_AM_DROP_CONTROL_MASK;
    }

    /* UM_MODIFIER_MPP1 */
    word_idx = TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->um_modifier_mpp1 = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->um_modifier_mpp1 |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->um_modifier_mpp1 = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_MASK;
    }

    /* UM_MODIFIER_MPP0 */
    word_idx = TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_OFFSET / word_size;
    bit_idx  = TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_WIDTH - low_bits;
        high_mask = (1 << high_bits) - 1;
        entry->um_modifier_mpp0 = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->um_modifier_mpp0 |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->um_modifier_mpp0 = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_MASK;
    }

    return;
}


/*
 * spd_ctrl_pack_am_tbl_entry() : Packet an am_tbl_entry_t into an array of 32-bit blocks
 */
void plp_aperta2_tscpmod_spd_ctrl_pack_am_tbl_entry(tscpmod_am_tbl_entry_t *entry, uint32_t *packed_entry)
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
    word_idx = TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->am_compression_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->am_compression_mode & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->am_compression_mode & TSCPMOD_AM_TBL_AM_COMPRESSION_MODE_MASK) << bit_idx;
    }

    /* AM_SIZE */
    word_idx = TSCPMOD_AM_TBL_AM_SIZE_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_AM_SIZE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_SIZE_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_AM_SIZE_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->am_size & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->am_size & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->am_size & TSCPMOD_AM_TBL_AM_SIZE_MASK) << bit_idx;
    }

    /* PAD_MODE */
    word_idx = TSCPMOD_AM_TBL_PAD_MODE_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_PAD_MODE_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_PAD_MODE_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_PAD_MODE_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->pad_mode & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->pad_mode & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->pad_mode & TSCPMOD_AM_TBL_PAD_MODE_MASK) << bit_idx;
    }

    /* UM_TABLE_COUNT */
    word_idx = TSCPMOD_AM_TBL_UM_TABLE_COUNT_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_UM_TABLE_COUNT_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_UM_TABLE_COUNT_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_UM_TABLE_COUNT_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->um_table_count & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->um_table_count & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->um_table_count & TSCPMOD_AM_TBL_UM_TABLE_COUNT_MASK) << bit_idx;
    }

    /* UM_TABLE_INDEX */
    word_idx = TSCPMOD_AM_TBL_UM_TABLE_INDEX_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_UM_TABLE_INDEX_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_UM_TABLE_INDEX_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_UM_TABLE_INDEX_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->um_table_index & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->um_table_index & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->um_table_index & TSCPMOD_AM_TBL_UM_TABLE_INDEX_MASK) << bit_idx;
    }

    /* CM */
    word_idx = TSCPMOD_AM_TBL_CM_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_CM_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_CM_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_CM_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->cm & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->cm & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->cm & TSCPMOD_AM_TBL_CM_MASK) << bit_idx;
    }

    /* USE_FIXED */
    word_idx = TSCPMOD_AM_TBL_USE_FIXED_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_USE_FIXED_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_USE_FIXED_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_USE_FIXED_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->use_fixed & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->use_fixed & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->use_fixed & TSCPMOD_AM_TBL_USE_FIXED_MASK) << bit_idx;
    }

    /* TSCPMOD_AM_SPACING_RS */
    word_idx = TSCPMOD_AM_TBL_AM_SPACING_RS_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_AM_SPACING_RS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_SPACING_RS_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_AM_SPACING_RS_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->am_spacing_rs & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->am_spacing_rs & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->am_spacing_rs & TSCPMOD_AM_TBL_AM_SPACING_RS_MASK) << bit_idx;
    }

    /* AM_SPACING_NO_RS */
    word_idx = TSCPMOD_AM_TBL_AM_SPACING_NO_RS_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_AM_SPACING_NO_RS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_SPACING_NO_RS_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_AM_SPACING_NO_RS_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->am_spacing_no_rs & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->am_spacing_no_rs & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->am_spacing_no_rs & TSCPMOD_AM_TBL_AM_SPACING_NO_RS_MASK) << bit_idx;
    }

    /* TSCPMOD_AM_SPACING_CREDITS */
    word_idx = TSCPMOD_AM_TBL_AM_SPACING_CREDITS_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_AM_SPACING_CREDITS_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_SPACING_CREDITS_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_AM_SPACING_CREDITS_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->am_spacing_credits & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->am_spacing_credits & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->am_spacing_credits & TSCPMOD_AM_TBL_AM_SPACING_CREDITS_MASK) << bit_idx;
    }

    /* CM_AM_SEL */
    word_idx = TSCPMOD_AM_TBL_CM_AM_SEL_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_CM_AM_SEL_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_CM_AM_SEL_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_CM_AM_SEL_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->cm_am_sel & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->cm_am_sel & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->cm_am_sel & TSCPMOD_AM_TBL_CM_AM_SEL_MASK) << bit_idx;
    }

    /* AM_DROP_CONTROL */
    word_idx = TSCPMOD_AM_TBL_AM_DROP_CONTROL_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_AM_DROP_CONTROL_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_AM_DROP_CONTROL_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_AM_DROP_CONTROL_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->am_drop_control & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->am_drop_control & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->am_drop_control & TSCPMOD_AM_TBL_AM_DROP_CONTROL_MASK) << bit_idx;
    }

    /* UM_MODIFIER_MPP1 */
    word_idx = TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->um_modifier_mpp1 & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->um_modifier_mpp1 & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->um_modifier_mpp1 & TSCPMOD_AM_TBL_UM_MODIFIER_MPP1_MASK) << bit_idx;
    }

    /* UM_MODIFIER_MPP0 */
    word_idx = TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_OFFSET / word_size;    
    bit_idx  = TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_WIDTH) > word_size ) {
        low_bits  = 32 - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->um_modifier_mpp0 & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->um_modifier_mpp0 & high_mask) >> low_bits;
    }
    else {
        packed_entry[word_idx] |= (entry->um_modifier_mpp0 & TSCPMOD_AM_TBL_UM_MODIFIER_MPP0_MASK) << bit_idx;
    }

    return;
}



/*
 * spd_ctrl_unpack_um_tbl_entry() : Unpacks a uint8_t array into an um_tbl_entry_t
 */
void plp_aperta2_tscpmod_spd_ctrl_unpack_um_tbl_entry(uint32_t *packed_entry, tscpmod_um_tbl_entry_t *entry)
{
    int word_size = 32;
    int word_idx;
    int bit_idx;
    int high_bits;
    int low_bits;
    int high_mask;
    int low_mask;

    /* CM_CONTROL */
    word_idx = TSCPMOD_UM_TBL_CM_CONTROL_OFFSET / word_size;
    bit_idx  = TSCPMOD_UM_TBL_CM_CONTROL_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_UM_TBL_CM_CONTROL_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_UM_TBL_CM_CONTROL_WIDTH - low_bits;
        high_mask =  (1 << high_bits) - 1;
        entry->cm_control = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->cm_control |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->cm_control = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_UM_TBL_CM_CONTROL_MASK;
    }
    
    /* TSCPMOD_UM_PAD */
    word_idx = TSCPMOD_UM_TBL_UM_PAD_OFFSET / word_size;
    bit_idx  = TSCPMOD_UM_TBL_UM_PAD_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_UM_TBL_UM_PAD_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_UM_TBL_UM_PAD_WIDTH - low_bits;
        high_mask =  (1 << high_bits) - 1;
        entry->um_pad = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->um_pad |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->um_pad = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_UM_TBL_UM_PAD_MASK;
    }

    /* CM_PAD */
    word_idx = TSCPMOD_UM_TBL_CM_PAD_OFFSET / word_size;
    bit_idx  = TSCPMOD_UM_TBL_CM_PAD_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_UM_TBL_CM_PAD_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_UM_TBL_CM_CONTROL_WIDTH - low_bits;
        high_mask =  (1 << high_bits) - 1;
        entry->cm_pad = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->cm_pad |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->cm_pad = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_UM_TBL_CM_PAD_MASK;
    }

    /* UM */
    word_idx = TSCPMOD_UM_TBL_UM_OFFSET / word_size;
    bit_idx  = TSCPMOD_UM_TBL_UM_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_UM_TBL_UM_WIDTH) > word_size) {
        low_bits = word_size - bit_idx;
        low_mask = (1 << low_bits)  -1;
        high_bits = TSCPMOD_UM_TBL_UM_WIDTH - low_bits;
        high_mask =  (1 << high_bits) - 1;
        entry->um = (packed_entry[word_idx] >> bit_idx) & low_mask;
        entry->um |= (packed_entry[word_idx + 1] & high_mask) << low_bits;
    }
    else {
        entry->um = (packed_entry[word_idx] >> bit_idx) & TSCPMOD_UM_TBL_UM_MASK;
    }

    return;
}

/*
 * spd_ctrl_pack_um_tbl_entry() : Packet an am_tbl_entry_t into an array of 32-bit blocks
 */
void plp_aperta2_tscpmod_spd_ctrl_pack_um_tbl_entry(tscpmod_um_tbl_entry_t *entry, uint32_t *packed_entry)
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
    word_idx = TSCPMOD_UM_TBL_CM_CONTROL_OFFSET / word_size;    
    bit_idx  = TSCPMOD_UM_TBL_CM_CONTROL_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_UM_TBL_CM_CONTROL_WIDTH) > word_size ) {
        low_bits  = word_size - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_UM_TBL_CM_CONTROL_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->cm_control & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->cm_control >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->cm_control & TSCPMOD_UM_TBL_CM_CONTROL_MASK) << bit_idx;
    }

    /* UM_PAD */
    word_idx = TSCPMOD_UM_TBL_UM_PAD_OFFSET / word_size;    
    bit_idx  = TSCPMOD_UM_TBL_UM_PAD_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_UM_TBL_UM_PAD_WIDTH) > word_size ) {
        low_bits  = word_size - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_UM_TBL_UM_PAD_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->um_pad & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->um_pad >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->um_pad & TSCPMOD_UM_TBL_UM_PAD_MASK) << bit_idx;
    }

    /* CM_PAD */
    word_idx = TSCPMOD_UM_TBL_CM_PAD_OFFSET / word_size;    
    bit_idx  = TSCPMOD_UM_TBL_CM_PAD_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_UM_TBL_CM_PAD_WIDTH) > word_size ) {
        low_bits  = word_size - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_UM_TBL_CM_PAD_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->cm_pad & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->cm_pad >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->cm_pad & TSCPMOD_UM_TBL_CM_PAD_MASK) << bit_idx;
    }

    /* UM */
    word_idx = TSCPMOD_UM_TBL_UM_OFFSET / word_size;    
    bit_idx  = TSCPMOD_UM_TBL_UM_OFFSET % word_size;
    /* coverity[dead_error_condition:FALSE] */
    if ( (bit_idx + TSCPMOD_UM_TBL_UM_WIDTH) > word_size ) {
        low_bits  = word_size - bit_idx;
        low_mask  = (1 << low_bits) - 1;
        high_bits = TSCPMOD_UM_TBL_UM_WIDTH - low_bits;
        high_mask  = (1 << high_bits) - 1;
        packed_entry[word_idx] |= (entry->um & low_mask) << bit_idx;
        packed_entry[word_idx + 1] |= (entry->um >> low_bits) & high_mask;
    }
    else {
        packed_entry[word_idx] |= (entry->um & TSCPMOD_UM_TBL_UM_MASK) << bit_idx;
    }

    return;
}

#endif  /* _TBHMOD_SPD_CTRL_C_ */
