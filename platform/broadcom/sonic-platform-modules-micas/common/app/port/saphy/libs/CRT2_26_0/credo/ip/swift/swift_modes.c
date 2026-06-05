/*
 * Swift running environments
 *
 * TX/RX polarity set/get
 * RX input mode (CR_COUPLING_AC/CR_COUPLING_DC)
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"
#include "swift/swift_serdes.h"

#include "sdk.h"

CredoError_t swift_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol) {
    ERR_PROP(common_dsp_set_rx_polarity(slice, lane, rx_pol));
    return CR_OK;
}

CredoError_t swift_get_rx_polarity(CredoSlice_t* slice, int lane, int* rx_pol) {
    ERR_PROP(common_dsp_get_rx_polarity(slice, lane, rx_pol));
    return CR_OK;
}

CredoError_t swift_set_tx_polarity(CredoSlice_t* slice, int lane, int tx_pol) {
    ERR_PROP(common_dsp_set_tx_polarity(slice, lane, tx_pol));
    return CR_OK;
}

CredoError_t swift_get_tx_polarity(CredoSlice_t* slice, int lane, int* tx_pol) {
    ERR_PROP(common_dsp_get_tx_polarity(slice, lane, tx_pol));
    return CR_OK;
}

CredoError_t swift_set_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t input_mode) {
    ERR_PROP(common_dsp_set_rx_input_mode(slice, lane, input_mode));
    return CR_OK;
}

CredoError_t swift_get_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t* input_mode) {
    ERR_PROP(common_dsp_get_rx_input_mode(slice, lane, input_mode));
    return CR_OK;
}
