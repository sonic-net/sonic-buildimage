/*
 * Common DSP information and setup
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"

#include "utility.h"
#include "sdk.h"

#include <stdlib.h>

CredoError_t common_dsp_get_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    unsigned nrz_en;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_NRZ_MODE, &nrz_en));
    if (nrz_en)
        *mode = CR_LMODE_NRZ;
    else
        *mode = CR_LMODE_PAM4;
    return CR_OK;
}

CredoError_t common_dsp_set_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode) {
    unsigned nrz_en;
    if (mode == CR_LMODE_NRZ)
        nrz_en = 1;
    else
        nrz_en = 0;
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_NRZ_MODE, nrz_en));
    return CR_OK;
}

CredoError_t common_dsp_tx_taps_rule_check(CredoSlice_t* slice, int lane, const int taps[]) {
    int length;
    ERR_PROPS(hal_get_tx_ffe_range(slice, lane, &length, NULL));

    int tx_taps_sum = 0;

    for (int i = 0; i < length; i++) {
        tx_taps_sum += abs(taps[i]);
    }

    if (tx_taps_sum > REG_TX_TAPS_SUM_LIMIT * 2) {
        LOGS_ERROR("[TX taps rule check] tx_taps sum is %f over limit %f !!", tx_taps_sum / 2.0, REG_TX_TAPS_SUM_LIMIT);
        return CR_FAIL;
    }

    return CR_OK;
}

CredoError_t common_dsp_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    ERR_PROPS(common_dsp_tx_taps_rule_check(slice, lane, taps));

    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP1, taps[0]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP2, taps[1]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP3, taps[2]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP4, taps[3]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP5, taps[4]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP6, taps[5]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP7, taps[6]));
    return CR_OK;
}

CredoError_t common_dsp_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]) {
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP1, taps + 0));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP2, taps + 1));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP3, taps + 2));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP4, taps + 3));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP5, taps + 4));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP6, taps + 5));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP7, taps + 6));
    return CR_OK;
}

CredoError_t common_dsp_set_tx_taps_extended(CredoSlice_t* slice, int lane, const int taps_extended[]) {
    ERR_PROPS(common_dsp_set_tx_taps(slice, lane, taps_extended));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP8, taps_extended[7]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP9, taps_extended[8]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP10, taps_extended[9]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP11, taps_extended[10]));
    return CR_OK;
}

CredoError_t common_dsp_get_tx_taps_extended(CredoSlice_t* slice, int lane, int taps_extended[]) {
    ERR_PROPS(common_dsp_get_tx_taps(slice, lane, taps_extended));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP8, taps_extended + 7));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP9, taps_extended + 8));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP10, taps_extended + 9));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP11, taps_extended + 10));

    return CR_OK;
}

CredoError_t common_dsp_reset_tx_taps(CredoSlice_t* slice, int lane) {
    static const int taps[] = {0, 0, 0, (REG_TX_TAPS_SUM_LIMIT * 2), 0, 0, 0};
    return common_dsp_set_tx_taps(slice, lane, taps);
}

CredoError_t common_dsp_get_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool* enable) {
    unsigned prbs_en, test_patt_en, prbs_clk_en, test_patt_sc;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_EN, &prbs_en));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_TEST_PATT_EN, &test_patt_en));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, &prbs_clk_en));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PATTERN, &test_patt_sc));
    *enable = (bool)(prbs_en & test_patt_en & prbs_clk_en & (1 - test_patt_sc));
    return CR_OK;
}

CredoError_t common_dsp_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable) {
    if (enable) {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, 1));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PATTERN, 0));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, 0));
    }
    return CR_OK;
}

CredoError_t common_dsp_get_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode* mode) {
    return readRegLane(slice, lane, REG_TX_PATT_MODE, (unsigned*)mode);
}

CredoError_t common_dsp_set_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode mode) {
    return writeRegLane(slice, lane, REG_TX_PATT_MODE, (unsigned)mode);
}

CredoError_t common_dsp_get_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t* pattern) {
    unsigned value = 0;

    *pattern = 0;
    ERR_PROP(readRegLane(slice, lane, REG_TX_PATTERN_MEM_63_48, &value));
    *pattern |= ((uint64_t)value << 48);
    ERR_PROP(readRegLane(slice, lane, REG_TX_PATTERN_MEM_47_32, &value));
    *pattern |= ((uint64_t)value << 32);
    ERR_PROP(readRegLane(slice, lane, REG_TX_PATTERN_MEM_31_16, &value));
    *pattern |= (value << 16);
    ERR_PROP(readRegLane(slice, lane, REG_TX_PATTERN_MEM_15_00, &value));
    *pattern |= value;

    return CR_OK;
}

CredoError_t common_dsp_set_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t pattern) {
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PATTERN_MEM_63_48, (unsigned)(pattern >> 48)));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PATTERN_MEM_47_32, (unsigned)(pattern >> 32)));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PATTERN_MEM_31_16, (unsigned)(pattern >> 16)));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PATTERN_MEM_15_00, (unsigned)pattern));
    return CR_OK;
}
