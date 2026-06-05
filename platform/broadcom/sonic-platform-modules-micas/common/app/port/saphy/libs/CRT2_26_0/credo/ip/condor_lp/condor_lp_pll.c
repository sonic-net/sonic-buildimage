/*
 * Condor LP PLL cap codes
 */
#include "project.h"

#include "condor_lp/condor_lp_regmap.h"

#include "sdk.h"

CredoError_t condor_lp_set_tx_cap(CredoSlice_t* slice, int lane, int tx_cap) {
    return writeRegLane(slice, lane, REG_TX_PLL_LVCOCAP, tx_cap);
}

CredoError_t condor_lp_set_rx_cap(CredoSlice_t* slice, int lane, int rx_cap) {
    return writeRegLane(slice, lane, REG_RX_PLL_LVCOCAP, rx_cap);
}

CredoError_t condor_lp_get_tx_cap(CredoSlice_t* slice, int lane, int* tx_cap) {
    unsigned cap;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PLL_LVCOCAP, &cap));
    *tx_cap = cap;
    return CR_OK;
}

CredoError_t condor_lp_get_rx_cap(CredoSlice_t* slice, int lane, int* rx_cap) {
    unsigned cap;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_PLL_LVCOCAP, &cap));
    *rx_cap = cap;
    return CR_OK;
}

CredoError_t condor_lp_set_tx_pll(CredoSlice_t* slice, int lane, int tx_pll, int vcocap) {
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PLL_N, tx_pll));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PLL_LVCOCAP, vcocap));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_pll(CredoSlice_t* slice, int lane, int rx_pll, int vcocap) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PLL_N, rx_pll));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PLL_LVCOCAP, vcocap));
    return CR_OK;
}

CredoError_t condor_lp_get_tx_pll(CredoSlice_t* slice, int lane, int* tx_pll, int* vcocap) {
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PLL_N, (unsigned*)tx_pll));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PLL_LVCOCAP, (unsigned*)vcocap));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_pll(CredoSlice_t* slice, int lane, int* rx_pll, int* vcocap) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PLL_N, (unsigned*)rx_pll));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PLL_LVCOCAP, (unsigned*)vcocap));
    return CR_OK;
}

CredoError_t condor_lp_set_tx_pll_frac(CredoSlice_t* slice, int lane, int pll_n, int smen, int sdm_order, int mmdiv) {
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PLL_FRAC_N, pll_n));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_SM_EN, smen));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_SAM_ORDER, sdm_order));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_MMDIV_MODE_SEL, mmdiv));
    return CR_OK;
}

CredoError_t condor_lp_get_tx_pll_frac(CredoSlice_t* slice, int lane, int* pll_n, int* smen, int* sdm_order,
                                       int* mmdiv) {
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PLL_FRAC_N, (unsigned*)pll_n));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_SM_EN, (unsigned*)smen));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_SAM_ORDER, (unsigned*)sdm_order));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_MMDIV_MODE_SEL, (unsigned*)mmdiv));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_pll_frac(CredoSlice_t* slice, int lane, int pll_n, int smen, int sdm_order, int mmdiv) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PLL_FRAC_N, pll_n));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SM_EN, smen));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SAM_ORDER, sdm_order));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_MMDIV_MODE_SEL, mmdiv));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_pll_frac(CredoSlice_t* slice, int lane, int* pll_n, int* smen, int* sdm_order,
                                       int* mmdiv) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PLL_FRAC_N, (unsigned*)pll_n));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SM_EN, (unsigned*)smen));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SAM_ORDER, (unsigned*)sdm_order));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_MMDIV_MODE_SEL, (unsigned*)mmdiv));
    return CR_OK;
}
