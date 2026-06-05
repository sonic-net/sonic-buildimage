/*
 * Canary PLL cap codes
 */
#include "project.h"

#include "sdk.h"

CredoError_t canary_set_tx_cap(CredoSlice_t* slice, int lane, int tx_cap) {
    return writeRegLane(slice, lane, REG_TX_PLL_LVCOCAP, tx_cap);
}

CredoError_t canary_set_rx_cap(CredoSlice_t* slice, int lane, int rx_cap) {
    return writeRegLane(slice, lane, REG_RX_PLL_LVCOCAP, rx_cap);
}

CredoError_t canary_get_tx_cap(CredoSlice_t* slice, int lane, int* tx_cap) {
    unsigned cap;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PLL_LVCOCAP, &cap));
    *tx_cap = cap;
    return CR_OK;
}

CredoError_t canary_get_rx_cap(CredoSlice_t* slice, int lane, int* rx_cap) {
    unsigned cap;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_PLL_LVCOCAP, &cap));
    *rx_cap = cap;
    return CR_OK;
}

CredoError_t canary_set_low_vaa(CredoSlice_t* slice, int lane, int enable) {
    if (enable) {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_ACOMPVREG, 1));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_ACOMPNBS1, 1));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_ACOMPVREG, 3));
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_ACOMPNBS1, 3));
    }
    return CR_OK;
}

CredoError_t canary_get_low_vaa(CredoSlice_t* slice, int lane, int* enable) {
    unsigned val = 0;

    *enable = 0;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_ACOMPVREG, &val));
    if (val != 1) return CR_OK;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_ACOMPNBS1, &val));
    if (val != 1) return CR_OK;

    *enable = 1;
    return CR_OK;
}
