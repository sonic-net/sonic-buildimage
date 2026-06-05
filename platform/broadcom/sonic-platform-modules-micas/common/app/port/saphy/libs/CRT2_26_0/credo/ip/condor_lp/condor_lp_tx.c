/*
 * Condor LP TX information and setup
 */
#include "project.h"

#include "condor_lp/condor_lp_regmap.h"

#include "utility.h"
#include "sdk.h"

#include <stdlib.h>

CredoError_t condor_lp_set_tx_taps_scale(CredoSlice_t* slice, int lane, const unsigned taps_scale[]) {
    LOGS_WARN("[Condor LP] Don't support set tx taps scale, do nothing!");
    return CR_OK;
}

CredoError_t condor_lp_get_tx_taps_scale(CredoSlice_t* slice, int lane, unsigned taps_scale[]) {
    int length;
    ERR_PROPS(hal_get_tx_ffe_range(slice, lane, &length, NULL));

    for (int i = 0; i < length; i++) {
        taps_scale[i] = 1;
    }

    return CR_OK;
}

CredoError_t condor_lp_get_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    unsigned nrz_en;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_NRZ_MODE, &nrz_en));
    if (nrz_en)
        *mode = CR_LMODE_NRZ;
    else
        *mode = CR_LMODE_PAM4;
    return CR_OK;
}

CredoError_t condor_lp_set_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode) {
    unsigned nrz_en;
    if (mode == CR_LMODE_NRZ)
        nrz_en = 1;
    else
        nrz_en = 0;
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_NRZ_MODE, nrz_en));
    return CR_OK;
}

CredoError_t condor_lp_tx_taps_rule_check(CredoSlice_t* slice, int lane, const int taps[]) {
    int tx_taps_sum = 0;

    for (int i = 0; i < 7; i++) {
        tx_taps_sum += abs(taps[i]);
    }

    if (tx_taps_sum > REG_TX_TAPS_SUM_LIMIT * 2) {
        LOGS_ERROR("[TX taps rule check] tx_taps sum is %f over limit %f !!", tx_taps_sum / 2.0, REG_TX_TAPS_SUM_LIMIT);
        return CR_FAIL;
    }

    return CR_OK;
}

CredoError_t condor_lp_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    ERR_PROPS(condor_lp_tx_taps_rule_check(slice, lane, taps));

    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP1, taps[0]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP2, taps[1]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP3, taps[2]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP4, taps[3]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP5, taps[4]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP6, taps[5]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP7, taps[6]));
    return CR_OK;
}

CredoError_t condor_lp_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]) {
    /* Check whether it's in training mode first
     * REG_TX_COEF_KR_TRAINING always be 0 in condor lp
     * */
    unsigned training;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_COEF_KR_TRAINING, &training));

    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP1, taps + 0));
    if (training) {
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_KR_TAP2, taps + 1));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_KR_TAP3, taps + 2));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_KR_TAP4, taps + 3));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_KR_TAP5, taps + 4));
        for (int i = 1; i < 5; i++) {
            taps[i] <<= 2;
        }
    } else {
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP2, taps + 1));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP3, taps + 2));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP4, taps + 3));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP5, taps + 4));
    }
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP6, taps + 5));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP7, taps + 6));
    return CR_OK;
}

CredoError_t condor_lp_reset_tx_taps(CredoSlice_t* slice, int lane) {
    static const int taps[] = {0, 0, 0, (REG_TX_TAPS_SUM_LIMIT * 2), 0, 0, 0};
    ERR_PROPS(hal_set_tx_taps(slice, lane, taps));
    return CR_OK;
}

CredoError_t condor_lp_set_tx_taps_extended(CredoSlice_t* slice, int lane, const int taps_extended[]) {
    ERR_PROPS(hal_set_tx_taps(slice, lane, taps_extended));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP8, taps_extended[7]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP9, taps_extended[8]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP10, taps_extended[9]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP11, taps_extended[10]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP12, taps_extended[11]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP13, taps_extended[12]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP14, taps_extended[13]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP15, taps_extended[14]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP16, taps_extended[15]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP17, taps_extended[16]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP18, taps_extended[17]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP19, taps_extended[18]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP20, taps_extended[19]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP21, taps_extended[20]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP22, taps_extended[21]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP23, taps_extended[22]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_TAP24, taps_extended[23]));
    return CR_OK;
}

CredoError_t condor_lp_get_tx_taps_extended(CredoSlice_t* slice, int lane, int taps_extended[]) {
    ERR_PROPS(hal_get_tx_taps(slice, lane, taps_extended));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP8, taps_extended + 7));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP9, taps_extended + 8));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP10, taps_extended + 9));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP11, taps_extended + 10));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP12, taps_extended + 11));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP13, taps_extended + 12));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP14, taps_extended + 13));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP15, taps_extended + 14));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP16, taps_extended + 15));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP17, taps_extended + 16));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP18, taps_extended + 17));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP19, taps_extended + 18));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP20, taps_extended + 19));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP21, taps_extended + 20));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP22, taps_extended + 21));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP23, taps_extended + 22));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_TAP24, taps_extended + 23));

    return CR_OK;
}

CredoError_t condor_lp_set_tx_asym_enable(CredoSlice_t* slice, int lane, int enable) {
    int level1;

    if (enable) {
        ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_COEF_0, &level1));
        level1 = level1 - ((level1 << 1) & (1 << 7));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0_BAR, level1 * (-1)));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0_3, level1 * 3));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0_BAR_3, level1 * (-3)));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_AUTO_SEL_0, 0));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_AUTO_SEL_0, 1));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0_BAR, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0_3, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0_BAR_3, 0));
    }
    return CR_OK;
}

CredoError_t condor_lp_get_tx_asym_enable(CredoSlice_t* slice, int lane, int* enable) {
    ERR_PROPS(readRegLane(slice, lane, REG_TX_COEF_AUTO_SEL_0, (unsigned*)enable));
    *enable ^= 1;
    return CR_OK;
}

CredoError_t condor_lp_set_tx_asym_level(CredoSlice_t* slice, int lane, int level[4]) {
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0, level[1]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0_BAR, level[2]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0_3, level[0]));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_COEF_0_BAR_3, level[3]));
    return CR_OK;
}
CredoError_t condor_lp_get_tx_asym_level(CredoSlice_t* slice, int lane, int level[4]) {
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_COEF_0, &level[1]));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_COEF_0_BAR, &level[2]));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_COEF_0_3, &level[0]));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_TX_COEF_0_BAR_3, &level[3]));
    // level[0] = level[0] - ((level[0] << 1) & (1 << 7));
    // level[1] = level[1] - ((level[1] << 1) & (1 << 7));
    // level[2] = level[2] - ((level[2] << 1) & (1 << 9));
    // level[3] = level[3] - ((level[3] << 1) & (1 << 9));
    return CR_OK;
}

CredoError_t condor_lp_get_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool* enable) {
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
            ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN_NRZ, &prbs_clk_en));
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

CredoError_t condor_lp_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable) {
    if (enable) {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, 1));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PATTERN, 0));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, 0));
    }
    return CR_OK;
}

CredoError_t condor_lp_get_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode* mode) {
    return readRegLane(slice, lane, REG_TX_PATT_MODE, (unsigned*)mode);
}

CredoError_t condor_lp_set_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode mode) {
    return writeRegLane(slice, lane, REG_TX_PATT_MODE, (unsigned)mode);
}

CredoError_t condor_lp_get_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t* pattern) {
    unsigned value = 0;

    *pattern = 0;
    ERR_PROP(readRegLane(slice, lane, REG_PATTERN_MEM_63_48, &value));
    *pattern |= ((uint64_t)value << 48);
    ERR_PROP(readRegLane(slice, lane, REG_PATTERN_MEM_47_32, &value));
    *pattern |= ((uint64_t)value << 32);
    ERR_PROP(readRegLane(slice, lane, REG_PATTERN_MEM_32_16, &value));
    *pattern |= (value << 16);
    ERR_PROP(readRegLane(slice, lane, REG_PATTERN_MEM_15_00, &value));
    *pattern |= value;

    return CR_OK;
}

CredoError_t condor_lp_set_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t pattern) {
    ERR_PROP(writeRegLane(slice, lane, REG_PATTERN_MEM_63_48, (unsigned)(pattern >> 48)));
    ERR_PROP(writeRegLane(slice, lane, REG_PATTERN_MEM_47_32, (unsigned)(pattern >> 32)));
    ERR_PROP(writeRegLane(slice, lane, REG_PATTERN_MEM_32_16, (unsigned)(pattern >> 16)));
    ERR_PROP(writeRegLane(slice, lane, REG_PATTERN_MEM_15_00, (unsigned)pattern));
    return CR_OK;
}
