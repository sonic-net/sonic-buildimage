/*
 * Swift TX information and setup
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"
#include "swift/swift_serdes.h"

#include "utility.h"
#include "sdk.h"

#include <stdlib.h>

CredoError_t swift_get_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    ERR_PROP(common_dsp_get_tx_lane_mode(slice, lane, mode));
    return CR_OK;
}

CredoError_t swift_set_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode) {
    ERR_PROP(common_dsp_set_tx_lane_mode(slice, lane, mode));
    return CR_OK;
}

CredoError_t swift_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    ERR_PROP(common_dsp_set_tx_taps(slice, lane, taps));
    return CR_OK;
}

CredoError_t swift_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]) {
    ERR_PROP(common_dsp_get_tx_taps(slice, lane, taps));
    return CR_OK;
}

CredoError_t swift_set_tx_taps_extended(CredoSlice_t* slice, int lane, const int taps_extended[]) {
    ERR_PROP(common_dsp_set_tx_taps_extended(slice, lane, taps_extended));
    return CR_OK;
}

CredoError_t swift_get_tx_taps_extended(CredoSlice_t* slice, int lane, int taps_extended[]) {
    ERR_PROP(common_dsp_get_tx_taps_extended(slice, lane, taps_extended));
    return CR_OK;
}

CredoError_t swift_reset_tx_taps(CredoSlice_t* slice, int lane) {
    ERR_PROP(common_dsp_reset_tx_taps(slice, lane));
    return CR_OK;
}

CredoError_t swift_get_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool* enable) {
    ERR_PROP(common_dsp_get_tx_test_pattern_enable(slice, lane, enable));
    return CR_OK;
}

CredoError_t swift_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable) {
    ERR_PROP(common_dsp_set_tx_test_pattern_enable(slice, lane, enable));
    return CR_OK;
}

CredoError_t swift_get_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode* mode) {
    ERR_PROP(common_dsp_get_tx_test_pattern_mode(slice, lane, mode));
    return CR_OK;
}

CredoError_t swift_set_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode mode) {
    ERR_PROP(common_dsp_set_tx_test_pattern_mode(slice, lane, mode));
    return CR_OK;
}

CredoError_t swift_get_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t* pattern) {
    ERR_PROP(common_dsp_get_tx_test_pattern_memory(slice, lane, pattern));
    return CR_OK;
}

CredoError_t swift_set_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t pattern) {
    ERR_PROP(common_dsp_set_tx_test_pattern_memory(slice, lane, pattern));
    return CR_OK;
}
