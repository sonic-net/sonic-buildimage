/*
 * Canary SerDes capability query
 */
#include "project.h"

#include "sdk.h"

/* SerDes capabilities */
CredoError_t canary_get_rx_ffe_range(CredoSlice_t* slice, int lane, int* taps_len, int* sum_len) {
    if (taps_len) *taps_len = 4;
    if (sum_len) *sum_len = 3;
    return CR_OK;
}

CredoError_t canary_get_tx_ffe_range(CredoSlice_t* slice, int lane, int* length, int* extended_length) {
    if (length) *length = 5;
    if (extended_length) *extended_length = 11;
    return CR_OK;
}

CredoError_t canary_get_rx_dfe_range(CredoSlice_t* slice, int lane, int* length) {
    if (length) *length = 3;
    return CR_OK;
}

CredoError_t canary_get_rx_isi_range(CredoSlice_t* slice, int lane, int* length) {
    if (length) *length = 16;
    return CR_OK;
}
