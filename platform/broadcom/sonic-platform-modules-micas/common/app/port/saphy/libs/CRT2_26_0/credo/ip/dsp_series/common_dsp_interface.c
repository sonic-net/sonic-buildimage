/*
 * Common DSP interface functions
 *
 * RX Gray code set/get
 * RX precoding set/get
 * RX MSB/LSB swapping set/get
 */

#include "project.h"

#include "dsp_series/common_dsp_functions.h"

#include "sdk.h"

CredoError_t common_dsp_set_rx_gray_code(CredoSlice_t* slice, int lane, int rx_gc) {
    return writeRegLane(slice, lane, REG_RX_GRAY_EN, rx_gc);
}

CredoError_t common_dsp_get_rx_gray_code(CredoSlice_t* slice, int lane, int* rx_gc) {
    return readRegLane(slice, lane, REG_RX_GRAY_EN, (unsigned*)rx_gc);
}

CredoError_t common_dsp_set_tx_gray_code(CredoSlice_t* slice, int lane, int tx_gc) {
    return writeRegLane(slice, lane, REG_TX_GRAY_EN, tx_gc);
}

CredoError_t common_dsp_get_tx_gray_code(CredoSlice_t* slice, int lane, int* tx_gc) {
    return readRegLane(slice, lane, REG_TX_GRAY_EN, (unsigned*)tx_gc);
}

CredoError_t common_dsp_set_rx_precoder(CredoSlice_t* slice, int lane, int rx_pc) {
    return writeRegLane(slice, lane, REG_RX_PRECODER_EN, rx_pc);
}

CredoError_t common_dsp_get_rx_precoder(CredoSlice_t* slice, int lane, int* rx_pc) {
    return readRegLane(slice, lane, REG_RX_PRECODER_EN, (unsigned*)rx_pc);
}

CredoError_t common_dsp_set_tx_precoder(CredoSlice_t* slice, int lane, int tx_pc) {
    return writeRegLane(slice, lane, REG_TX_PRECODER_EN, tx_pc);
}

CredoError_t common_dsp_get_tx_precoder(CredoSlice_t* slice, int lane, int* tx_pc) {
    return readRegLane(slice, lane, REG_TX_PRECODER_EN, (unsigned*)tx_pc);
}

CredoError_t common_dsp_set_rx_msb(CredoSlice_t* slice, int lane, int rx_msb) {
    return writeRegLane(slice, lane, REG_RX_SWAP_MSB_LSB, rx_msb);
}

CredoError_t common_dsp_get_rx_msb(CredoSlice_t* slice, int lane, int* rx_msb) {
    return readRegLane(slice, lane, REG_RX_SWAP_MSB_LSB, (unsigned*)rx_msb);
}

CredoError_t common_dsp_set_tx_msb(CredoSlice_t* slice, int lane, int tx_msb) {
    return writeRegLane(slice, lane, REG_TX_MSB_SWAP, tx_msb);
}

CredoError_t common_dsp_get_tx_msb(CredoSlice_t* slice, int lane, int* tx_msb) {
    return readRegLane(slice, lane, REG_TX_MSB_SWAP, (unsigned*)tx_msb);
}
