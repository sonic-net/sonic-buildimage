/*
 * Common DSP running environments
 *
 * TX/RX polarity set/get
 * RX input mode (CR_COUPLING_AC/CR_COUPLING_DC)
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"

#include "sdk.h"

CredoError_t common_dsp_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol) {
    return writeRegLane(slice, lane, REG_RX_DATA_CUSTOMER_FLIP, rx_pol);
}

CredoError_t common_dsp_get_rx_polarity(CredoSlice_t* slice, int lane, int* rx_pol) {
    return readRegLane(slice, lane, REG_RX_DATA_CUSTOMER_FLIP, (unsigned*)rx_pol);
}

CredoError_t common_dsp_set_tx_polarity(CredoSlice_t* slice, int lane, int tx_pol) {
    return writeRegLane(slice, lane, REG_TX_ANA_OUTPUT_FLIP, !tx_pol);
}

CredoError_t common_dsp_get_tx_polarity(CredoSlice_t* slice, int lane, int* tx_pol) {
    unsigned pol;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_ANA_OUTPUT_FLIP, &pol));
    *tx_pol = !pol;
    return CR_OK;
}

CredoError_t common_dsp_set_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t input_mode) {
    if (input_mode == CR_COUPLING_AC) {
        ERR_PROP(writeRegLane(slice, lane, REG_RX_EN_EXTAC, 1));
    } else {
        ERR_PROP(writeRegLane(slice, lane, REG_RX_EN_EXTAC, 0));
    }
    return CR_OK;
}

CredoError_t common_dsp_get_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t* input_mode) {
    return readRegLane(slice, lane, REG_RX_EN_EXTAC, input_mode);
}
