/*
 * Condor LP SerDes capability query
 */
#include "project.h"

#include "sdk.h"

/* SerDes capabilities */
CredoError_t condor_lp_get_rx_ffe_range(CredoSlice_t* slice, int lane, int* taps_len, int* sum_len) {
    if (taps_len) *taps_len = 5;
    if (sum_len) *sum_len = 3;
    return CR_OK;
}

CredoError_t condor_lp_get_rx_ffe_weighting_table_range(CredoSlice_t* slice, int lane, int* row, int* col) {
    if (row) *row = FW_FFE_WT_ROW;
    if (col) *col = FW_FFE_WT_COL;
    return CR_OK;
}

CredoError_t condor_lp_get_tx_ffe_range(CredoSlice_t* slice, int lane, int* length, int* extended_length) {
    if (length) *length = 7;
    if (extended_length) *extended_length = 24;
    return CR_OK;
}

CredoError_t condor_lp_get_rx_dfe_range(CredoSlice_t* slice, int lane, int* length) {
    if (length) *length = 3;
    return CR_OK;
}

// TODO, functions below need update
CredoError_t condor_lp_get_rx_isi_range(CredoSlice_t* slice, int lane, int* length) {
    if (length) *length = 18;
    return CR_OK;
}
