/*
 * Swift SerDes interface functions:
 *
 * RX Gray code set/get
 * RX precoding set/get
 * RX MSB/LSB swapping set/get
 */

#include "project.h"

#include "dsp_series/common_dsp_functions.h"
#include "swift/swift_serdes.h"

#include "sdk.h"

CredoError_t swift_set_rx_gray_code(CredoSlice_t* slice, int lane, int rx_gc) {
    ERR_PROP(common_dsp_set_rx_gray_code(slice, lane, rx_gc));
    return CR_OK;
}

CredoError_t swift_get_rx_gray_code(CredoSlice_t* slice, int lane, int* rx_gc) {
    ERR_PROP(common_dsp_get_rx_gray_code(slice, lane, rx_gc));
    return CR_OK;
}

CredoError_t swift_set_tx_gray_code(CredoSlice_t* slice, int lane, int tx_gc) {
    ERR_PROP(common_dsp_set_tx_gray_code(slice, lane, tx_gc));
    return CR_OK;
}

CredoError_t swift_get_tx_gray_code(CredoSlice_t* slice, int lane, int* tx_gc) {
    ERR_PROP(common_dsp_get_tx_gray_code(slice, lane, tx_gc));
    return CR_OK;
}

CredoError_t swift_set_rx_precoder(CredoSlice_t* slice, int lane, int rx_pc) {
    ERR_PROP(common_dsp_set_rx_precoder(slice, lane, rx_pc));
    return CR_OK;
}

CredoError_t swift_get_rx_precoder(CredoSlice_t* slice, int lane, int* rx_pc) {
    ERR_PROP(common_dsp_get_rx_precoder(slice, lane, rx_pc));
    return CR_OK;
}

CredoError_t swift_set_tx_precoder(CredoSlice_t* slice, int lane, int tx_pc) {
    ERR_PROP(common_dsp_set_tx_precoder(slice, lane, tx_pc));
    return CR_OK;
}

CredoError_t swift_get_tx_precoder(CredoSlice_t* slice, int lane, int* tx_pc) {
    ERR_PROP(common_dsp_get_tx_precoder(slice, lane, tx_pc));
    return CR_OK;
}

CredoError_t swift_set_rx_msb(CredoSlice_t* slice, int lane, int rx_msb) {
    ERR_PROP(common_dsp_set_rx_msb(slice, lane, rx_msb));
    return CR_OK;
}

CredoError_t swift_get_rx_msb(CredoSlice_t* slice, int lane, int* rx_msb) {
    ERR_PROP(common_dsp_get_rx_msb(slice, lane, rx_msb));
    return CR_OK;
}

CredoError_t swift_set_tx_msb(CredoSlice_t* slice, int lane, int tx_msb) {
    ERR_PROP(common_dsp_set_tx_msb(slice, lane, tx_msb));
    return CR_OK;
}

CredoError_t swift_get_tx_msb(CredoSlice_t* slice, int lane, int* tx_msb) {
    ERR_PROP(common_dsp_get_tx_msb(slice, lane, tx_msb));
    return CR_OK;
}
