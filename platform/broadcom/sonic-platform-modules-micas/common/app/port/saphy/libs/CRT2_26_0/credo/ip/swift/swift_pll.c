/*
 * Swift PLL cap codes
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"
#include "swift/swift_serdes.h"

#include "sdk.h"

CredoError_t swift_set_rx_cap(CredoSlice_t* slice, int lane, int rx_cap) {
    ERR_PROP(common_dsp_set_rx_cap(slice, lane, rx_cap));
    return CR_OK;
}

CredoError_t swift_get_rx_cap(CredoSlice_t* slice, int lane, int* rx_cap) {
    ERR_PROP(common_dsp_get_rx_cap(slice, lane, rx_cap));
    return CR_OK;
}

CredoError_t swift_set_tx_cap(CredoSlice_t* slice, int lane, int tx_cap) {
    ERR_PROP(common_dsp_set_tx_cap(slice, lane, tx_cap));
    return CR_OK;
}

CredoError_t swift_get_tx_cap(CredoSlice_t* slice, int lane, int* tx_cap) {
    ERR_PROP(common_dsp_get_tx_cap(slice, lane, tx_cap));
    return CR_OK;
}

CredoError_t swift_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    ERR_PROP(common_dsp_get_lane_speed(slice, lane, speed_kbps));
    return CR_OK;
}
