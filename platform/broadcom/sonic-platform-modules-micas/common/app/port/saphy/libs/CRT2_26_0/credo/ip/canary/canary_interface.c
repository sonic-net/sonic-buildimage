/*
 * Canary SerDes interface functions:
 *
 * TX/RX Gray code set/get
 * TX/RX precoding set/get
 * TX/RX MSB/LSB swapping set/get
 */

#include "project.h"

#include "sdk.h"

/* Modes */
CredoError_t canary_set_tx_gray_code(CredoSlice_t* slice, int lane, int tx_gc) {
    CredoError_t ret = CR_OK;

    ERR_PROPS(writeRegLane(slice, lane, REG_TX_GRAY_EN, tx_gc));
    return ret;
}

CredoError_t canary_set_rx_gray_code(CredoSlice_t* slice, int lane, int rx_gc) {
    CredoError_t ret = CR_OK;

    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_GRAY_EN, rx_gc));
    return ret;
}

CredoError_t canary_get_tx_gray_code(CredoSlice_t* slice, int lane, int* tx_gc) {
    CredoError_t ret = CR_OK;
    unsigned gc;

    ERR_PROPS(readRegLane(slice, lane, REG_TX_GRAY_EN, &gc));
    *tx_gc = gc;
    return ret;
}

CredoError_t canary_get_rx_gray_code(CredoSlice_t* slice, int lane, int* rx_gc) {
    CredoError_t ret = CR_OK;
    unsigned gc;

    ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_GRAY_EN, &gc));
    *rx_gc = gc;
    return ret;
}

CredoError_t canary_set_tx_precoder(CredoSlice_t* slice, int lane, int tx_pc) {
    CredoError_t ret = CR_OK;

    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRECODER_EN, tx_pc));
    return ret;
}

CredoError_t canary_set_rx_precoder(CredoSlice_t* slice, int lane, int rx_pc) {
    CredoError_t ret = CR_OK;

    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRECODER_EN, rx_pc));
    return ret;
}

CredoError_t canary_get_tx_precoder(CredoSlice_t* slice, int lane, int* tx_pc) {
    CredoError_t ret = CR_OK;
    unsigned pc;

    ERR_PROPS(readRegLane(slice, lane, REG_TX_PRECODER_EN, &pc));
    *tx_pc = pc;
    return ret;
}

CredoError_t canary_get_rx_precoder(CredoSlice_t* slice, int lane, int* rx_pc) {
    CredoError_t ret = CR_OK;
    unsigned pc;

    ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_PRECODER_EN, &pc));
    *rx_pc = pc;
    return ret;
}

CredoError_t canary_set_tx_msb(CredoSlice_t* slice, int lane, int tx_msb) {
    CredoError_t ret = CR_OK;

    ERR_PROPS(writeRegLane(slice, lane, REG_TX_MSB_SWAP, tx_msb));
    return ret;
}

CredoError_t canary_set_rx_msb(CredoSlice_t* slice, int lane, int rx_msb) {
    CredoError_t ret = CR_OK;

    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_MSB_SWAP, rx_msb));
    return ret;
}

CredoError_t canary_get_tx_msb(CredoSlice_t* slice, int lane, int* tx_msb) {
    CredoError_t ret = CR_OK;
    unsigned msb;

    ERR_PROPS(readRegLane(slice, lane, REG_TX_MSB_SWAP, &msb));
    *tx_msb = msb;
    return ret;
}

CredoError_t canary_get_rx_msb(CredoSlice_t* slice, int lane, int* rx_msb) {
    CredoError_t ret = CR_OK;
    unsigned msb;

    ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_MSB_SWAP, &msb));
    *rx_msb = msb;
    return ret;
}
