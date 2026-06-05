/*
 * Canary TX information and setup
 */
#include "project.h"

#include "utility.h"
#include "sdk.h"

#include <stdlib.h>

CredoError_t canary_set_tx_taps_scale(CredoSlice_t* slice, int lane, const unsigned taps_scale[]) {
    unsigned scale = 0;
    unsigned i;

    for (i = 0; i < 5; i++) {
        scale |= ((taps_scale[i] & 1) << i);
    }
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAPS_HF, scale));
    return CR_OK;
}

CredoError_t canary_get_tx_taps_scale(CredoSlice_t* slice, int lane, unsigned taps_scale[]) {
    unsigned scale = 0;
    unsigned i;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_TAPS_HF, &scale));

    for (i = 0; i < 5; i++) {
        taps_scale[i] = (scale >> i) & 1;
    }

    return CR_OK;
}

CredoError_t canary_tx_taps_rule_check(CredoSlice_t* slice, int lane, const int taps[]) {
    unsigned i, hf;
    int tx_taps_sum = 0;

    ERR_PROPS(readRegLane(slice, lane, REG_TX_TAPS_HF, &hf));

    for (i = 0; i < 5; i++) {
        tx_taps_sum += abs(taps[i]) << ((hf >> i) & 1);  // if it's 1, then *2
    }

    if (tx_taps_sum > REG_TX_TAPS_SUM_LIMIT * 2) {
        LOGS_ERROR("[TX taps rule check] tx_taps sum is %f over limit %f !!", tx_taps_sum / 2.0, REG_TX_TAPS_SUM_LIMIT);
        return CR_FAIL;
    }

    return CR_OK;
}

CredoError_t canary_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    ERR_PROPS(canary_tx_taps_rule_check(slice, lane, taps));

    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP1, taps[0]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP2, taps[1]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP3, taps[2]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP4, taps[3]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP5, taps[4]));
    return CR_OK;
}

CredoError_t canary_set_tx_taps_extended(CredoSlice_t* slice, int lane, const int taps_extended[]) {
    ERR_PROPS(canary_set_tx_taps(slice, lane, taps_extended));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP6, taps_extended[5]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP7, taps_extended[6]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP8, taps_extended[7]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP9, taps_extended[8]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP10, taps_extended[9]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP11, taps_extended[10]));
    return CR_OK;
}

CredoError_t canary_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]) {
    /* Check whether it's in training mode first */
    unsigned training;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_COEF_KR_TRAINING, &training));

    if (training) {
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_KR_TAP1, taps + 0));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_KR_TAP2, taps + 1));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_KR_TAP3, taps + 2));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_KR_TAP4, taps + 3));
    } else {
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP1, taps + 0));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP2, taps + 1));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP3, taps + 2));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP4, taps + 3));
    }
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP5, taps + 4));
    return CR_OK;
}

CredoError_t canary_get_tx_taps_extended(CredoSlice_t* slice, int lane, int taps_extended[]) {
    ERR_PROPS(canary_get_tx_taps(slice, lane, taps_extended));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP6, taps_extended + 5));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP7, taps_extended + 6));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP8, taps_extended + 7));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP9, taps_extended + 8));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP10, taps_extended + 9));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP11, taps_extended + 10));
    return CR_OK;
}

CredoError_t canary_reset_tx_taps(CredoSlice_t* slice, int lane) {
    static const int taps[] = {0, 0, (int)REG_TX_TAPS_SUM_LIMIT, 0, 0};
    return canary_set_tx_taps(slice, lane, taps);
}

CredoError_t canary_get_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool* enable) {
    unsigned prbs_en, test_patt_en, prbs_clk_en, test_patt_sc;
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_EN, &prbs_en));
            ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, &prbs_clk_en));
            break;
        case CR_LMODE_DISABLE:  //
        case CR_LMODE_OFF:      //  treat as NRZ
        case CR_LMODE_AN:       //
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, &prbs_en));
            ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_CLK_EN_NRZ, &prbs_clk_en));
            break;
        default:
            *enable = 0;
            LOGS_WARN("[GET TX TEST Pattern] Lane %d unknown mode %d", lane, mode);
            return CR_FAIL;
    }

    ERR_PROPS(readRegLane(slice, lane, REG_TX_TEST_PATT_EN, &test_patt_en));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PATTERN, &test_patt_sc));
    *enable = (bool)(prbs_en & test_patt_en & prbs_clk_en & (1 - test_patt_sc));
    return CR_OK;
}

CredoError_t canary_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable) {
    if (enable) {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, 1));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PATTERN, 0));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, 0));
    }
    return CR_OK;
}

CredoError_t canary_get_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode* mode) {
    return readRegLane(slice, lane, REG_TX_PATT_MODE, (unsigned*)mode);
}

CredoError_t canary_set_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode mode) {
    return writeRegLane(slice, lane, REG_TX_PATT_MODE, (unsigned)mode);
}

CredoError_t canary_get_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t* pattern) {
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

CredoError_t canary_set_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t pattern) {
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PATTERN_MEM_63_48, (unsigned)(pattern >> 48)));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PATTERN_MEM_47_32, (unsigned)(pattern >> 32)));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PATTERN_MEM_31_16, (unsigned)(pattern >> 16)));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PATTERN_MEM_15_00, (unsigned)pattern));
    return CR_OK;
}
