/*
 * Canary RX information and debugging
 */
#include "project.h"

#include "utility.h"
#include "sdk.h"

#include <stdlib.h>

CredoError_t canary_get_version_id(CredoSlice_t* slice, int lane, uint64_t* version) {
    unsigned ip_id = 0, revision = 0, subrevision = 0;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_VER_ID_47_32, &ip_id));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VER_ID_31_16, &revision));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VER_ID_15_00, &subrevision));
    *version = 0;
    *version = ((uint64_t)ip_id << 32) | (revision << 16) | subrevision;
    return CR_OK;
}

CredoError_t canary_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegSignedLane(slice, lane, REG_EAGLE_RX_FREQ_ACCUM, ppm));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegSignedLane(slice, lane, REG_FLR_RX_FREQ_ACCUM, ppm));
            break;
        default:
            LOGS_WARN("[Get RX PPM] Unknown mode %d, returning 0", mode);
            *ppm = 0;
            break;
    }
    return CR_OK;
}

CredoError_t canary_get_rx_skef(CredoSlice_t* slice, int lane, int* enable, int* degen, int* addcap, int* skef_gain) {
    CredoError_t ret = CR_OK;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_EN, (unsigned*)enable));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_DEGEN, (unsigned*)degen));
    return ret;
}

CredoError_t canary_get_rx_dac(CredoSlice_t* slice, int lane, int* rx_dac) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    unsigned dac = 0, owen = 0;

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_DAC_OWEN, &owen));
            if (owen) {
                ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_DAC_OW, &dac));
            } else {
                ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_DAC, &dac));
            }
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_DAC_OWEN, &owen));
            if (owen) {
                ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_DAC_OW, &dac));
            } else {
                ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_DAC, &dac));
            }
            break;
        default:
            LOGS_WARN("[Get RX DAC] Unknown mode %d, returning 0", mode);
            dac = 0;
            break;
    }
    *rx_dac = dac;
    return CR_OK;
}

/* Canary FFE: polarity=0 -> positive. Returns 2-compliment */
static inline int combine_ffe_taps(unsigned pol, unsigned gray_msb, unsigned gray_lsb) {
    int abs_val = (gray_bin(gray_msb) << 4) + (gray_bin(gray_lsb));
    return pol ? -abs_val - 1 : abs_val;
}

static inline void separate_ffe_taps(int value, unsigned* pol, unsigned* gray_msb, unsigned* gray_lsb) {
    if (value < 0) {
        value = -value - 1;
        *pol = 1;
    } else {
        *pol = 0;
    }
    *gray_msb = bin_gray(value >> 4);
    *gray_lsb = bin_gray(value & 0xf);
}

CredoError_t canary_get_ffe_taps(CredoSlice_t* slice, int lane, int taps[]) {
    unsigned int pol = 0;
    unsigned int msb = 0;
    unsigned int lsb = 0;

    // K1
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL1, &pol));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K1_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K1_LSB, &lsb));
    taps[0] = combine_ffe_taps(pol, msb, lsb);

    // K2
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL2, &pol));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K2_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K2_LSB, &lsb));
    taps[1] = combine_ffe_taps(pol, msb, lsb);

    // K3
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL3, &pol));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K3_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K3_LSB, &lsb));
    taps[2] = combine_ffe_taps(pol, msb, lsb);

    // K4
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL4, &pol));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K4_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K4_LSB, &lsb));
    taps[3] = combine_ffe_taps(pol, msb, lsb);

    // s1
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S1_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S1_LSB, &lsb));
    taps[4] = combine_ffe_taps(0, msb, lsb);

    // s2
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S2_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S2_LSB, &lsb));
    taps[5] = combine_ffe_taps(0, msb, lsb);

    // sf
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_SF_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_SF_LSB, &lsb));
    taps[6] = combine_ffe_taps(0, msb, lsb);

    return CR_OK;
}

CredoError_t canary_get_ffe_nbias(CredoSlice_t* slice, int lane, int nbias[]) {
    unsigned nbias_main = 0, nbias_sum = 0;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_NBIAS_MAIN, &nbias_main));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_NBIAS_SUM, &nbias_sum));
    nbias[0] = gray_bin(nbias_main);
    nbias[1] = gray_bin(nbias_sum);
    return CR_OK;
}

CredoError_t canary_get_f1over3(CredoSlice_t* slice, int lane, int* value) {
    return readRegSignedLane(slice, lane, REG_FLR_RX_F1OVER3, value);
}

CredoError_t canary_get_agcgain_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (count == NULL) return CR_INVALID_ARGS;
    *count = 2;
    return CR_OK;
}

CredoError_t canary_get_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    unsigned int agcgain1 = 0;
    unsigned int agcgain2 = 0;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGCGAIN1, &agcgain1));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGCGAIN2, &agcgain2));

    agcgain[0] = gray_bin(agcgain1);
    agcgain[1] = gray_bin(agcgain2);
    //*agcgain = (agcgain[0]<<8) | (agcgain[1]);
    return CR_OK;
}

CredoError_t canary_get_ctle_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (count == NULL) return CR_INVALID_ARGS;
    *count = 2;
    return CR_OK;
}

CredoError_t canary_get_ctle_index(CredoSlice_t* slice, int lane, unsigned* ctle_index) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_AGC_OW, ctle_index));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_AGC_OW, ctle_index));
            break;
        default:
            *ctle_index = 0;
            break;
    }
    return CR_OK;
}

CredoError_t canary_get_ctle(CredoSlice_t* slice, int lane, unsigned value[]) {
    unsigned ctle_index = 0, ctle, ctle_msb;
    unsigned maps[3];
    uint64_t map;

    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode != CR_LMODE_NRZ && mode != CR_LMODE_PAM4) {
        LOGS_WARN("[Get CTLE] Unknown mode %d, using NRZ mode", mode);
        mode = CR_LMODE_NRZ;
    }

    /* Read CTLE map */
    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_AGC_OW, &ctle_index));
            ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_AGC_MAP0, maps + 0));
            ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_AGC_MAP1, maps + 1));
            ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_AGC_MAP2, maps + 2));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_AGC_OW, &ctle_index));
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_AGC_MAP0, maps + 0));
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_AGC_MAP1, maps + 1));
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_AGC_MAP2, maps + 2));
            break;
        default:;  // won't happen
    }

    /* Calculate AGC from map */
    map = ((uint64_t)maps[0] << 32) + ((uint64_t)maps[1] << 16) + (maps[2]);
    ctle = (map >> ((7 - ctle_index) * 6)) & 0x3F;

    /* Read CTLE MSB */
    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGC_DEGEN, &ctle_msb));

    value[0] = ((ctle >> 3) & 0x7) + (((ctle_msb >> 1) & 1) << 3);
    value[1] = (ctle & 7) + ((ctle_msb & 1) << 3);

    return CR_OK;
}

CredoError_t canary_get_delta_phase(CredoSlice_t* slice, int lane, int* value) {
    CredoLaneMode_t mode;
    // ERR_PROPS( get_lane_mode(slice, lane, &mode));
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegSignedLane(slice, lane, REG_EAGLE_RX_DELTA, value));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegSignedLane(slice, lane, REG_FLR_RX_DELTA_OW, value));
            break;
        default:
            LOGS_WARN("[Get delta phase] Unknown mode %d, returning 0", mode);
            *value = 0;
            break;
    }
    return CR_OK;
}

CredoError_t canary_get_edge(CredoSlice_t* slice, int lane, unsigned* value) {
    return readRegLane(slice, lane, REG_RX_EDGE, value);
}

CredoError_t canary_get_dfe(CredoSlice_t* slice, int lane, double dfe_taps[3]) {
    CredoLaneMode_t mode;
    // ERR_PROPS( get_lane_mode(slice, lane, &mode));
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    int i = 0;

    switch (mode) {
        case CR_LMODE_NRZ: {
            int dfe[3];
            ERR_PROPS(readRegSignedLane(slice, lane, REG_EAGLE_RX_F1, dfe + 0));
            ERR_PROPS(readRegSignedLane(slice, lane, REG_EAGLE_RX_F2, dfe + 1));
            ERR_PROPS(readRegSignedLane(slice, lane, REG_EAGLE_RX_F3, dfe + 2));

            dfe_taps[0] = dfe[0];
            dfe_taps[1] = dfe[1];
            dfe_taps[2] = dfe[2];
        } break;
        case CR_LMODE_PAM4: {
            /* For PAM4, you should really call firmware version. */
            int ths_list[12];
            for (i = 0; i < 12; i++) {
                ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PAM4_DFE_SEL, i));
                ERR_PROPS(readRegSignedLane(slice, lane, REG_FLR_RX_PAM4_DFE_RD, ths_list + i));
            }

            dfe_taps[0] = (-3.0f / 16) * ((ths_list[0] - ths_list[2]) + (ths_list[3] - ths_list[5]) +
                                          (ths_list[6] - ths_list[8]) + (ths_list[9] - ths_list[11]));
            dfe_taps[1] =
                (-3.0f / 20) *
                ((ths_list[0] + ths_list[1] + ths_list[2] - ths_list[9] - ths_list[10] - ths_list[11]) +
                 (1.0f / 3) * (ths_list[3] + ths_list[4] + ths_list[5] - ths_list[6] - ths_list[7] - ths_list[8]));
            dfe_taps[0] /= 2048.0;
            dfe_taps[1] /= 2048.0;
            if (dfe_taps[0] == 0)
                dfe_taps[2] = 0;
            else
                dfe_taps[2] = dfe_taps[1] / dfe_taps[0];
        } break;
        default:
            LOGS_WARN("[Get DFE] Unknown mode %d, returning all 0", mode);
            dfe_taps[0] = 0;
            dfe_taps[1] = 0;
            dfe_taps[2] = 0;
            break;
    }

    return CR_OK;
}

/* If firmware is running, PAM4 eye might be inaccurate. Prefer to use firmware eye read. */
CredoError_t canary_get_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    int dac_val;
    unsigned int eye_reg_val;
    int i, j;
    int sel;
    int plus_margin, minus_margin, diff;
    unsigned minus_margin_L;
    int minus_margin_H;
    int result1;

    CredoLaneMode_t mode;
    // ERR_PROPS( get_lane_mode(slice, lane, &mode));
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    ERR_PROPS(canary_get_rx_dac(slice, lane, &dac_val));
    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_EM, &eye_reg_val));
            eyes[0] = (eye_reg_val / 2048.0) * (200 + 50.0 * dac_val);
            eyes[1] = 0;
            eyes[2] = 0;
            break;
        case CR_LMODE_PAM4:
            for (i = 0; i < 3; i++) {
                result1 = 0x10000;
                for (j = 0; j < 4; j++) {
                    sel = 3 * j + i;
                    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PLUS_MARGIN_SEL, sel));
                    ERR_PROPS(readRegSignedLane(slice, lane, REG_FLR_RX_PLUS_MARGIN, &plus_margin));

                    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_MINUS_MARGIN_SEL, sel));
                    ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_MINUS_MARGIN_L, &minus_margin_L));
                    ERR_PROPS(readRegSignedLane(slice, lane, REG_FLR_RX_MINUS_MARGIN_H, &minus_margin_H));
                    minus_margin = (minus_margin_H << 8) | (minus_margin_L);

                    diff = plus_margin - minus_margin;

                    if (diff < result1) result1 = diff;
                }
                eyes[i] = (result1 / 2048.0) * (100 + 50.0 * dac_val);
            }

            break;
        default:
            LOGS_WARN("[Read eye] Unknown mode %d, return 0", mode);
            eyes[0] = 0;
            eyes[1] = 0;
            eyes[2] = 0;
            break;
    }
    return CR_OK;
}

CredoError_t canary_get_lane_ready(CredoSlice_t* slice, int lane, int* ready) {
    CredoLaneMode_t mode;
    // ERR_PROPS( get_lane_mode(slice, lane, &mode));
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    unsigned value = 0;

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_RDY, &value));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_RDY, &value));
            break;
        default:
            LOGS_WARN("[Get lane ready] Unknown mode %d, returning not ready", mode);
            break;
    }
    *ready = value;
    return CR_OK;
}

CredoError_t canary_get_signal_detect(CredoSlice_t* slice, int lane, int* sd) {
    unsigned value = 0;
    CredoLaneMode_t mode;
    // ERR_PROPS( get_lane_mode(slice, lane, &mode));
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_SD, &value));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_SD, &value));
            break;
        default:
            LOGS_WARN("[Get signal detect] Unknown mode %d, returning not detect", mode);
            break;
    }
    *sd = value;
    return CR_OK;
}

/********************************/
CredoError_t canary_set_rx_skef(CredoSlice_t* slice, int lane, int enable, int degen, int addcap, int skef_gain) {
    CredoError_t ret = CR_OK;

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_DEGEN, degen));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_EN, enable));
    return ret;
}

CredoError_t canary_set_rx_dac(CredoSlice_t* slice, int lane, int rx_dac) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_DAC_OW, rx_dac));
        ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_DAC_OWEN, 1));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_DAC_OW, rx_dac));
        ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_DAC_OWEN, 1));
        if (mode != CR_LMODE_NRZ) {
            LOGS_WARN("[Set RX DAC] Unknown mode %d, setting NRZ DAC", mode);
        }
    }
    return CR_OK;
}

CredoError_t canary_set_ffe_taps(CredoSlice_t* slice, int lane, const int taps[7]) {
    unsigned pol;
    unsigned msb;
    unsigned lsb;

    // k1
    separate_ffe_taps(taps[0], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K1_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K1_LSB, lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL1, pol));

    // k2
    separate_ffe_taps(taps[1], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K2_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K2_LSB, lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL2, pol));

    // k3
    separate_ffe_taps(taps[2], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K3_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K3_LSB, lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL3, pol));

    // k4
    separate_ffe_taps(taps[3], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K4_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K4_LSB, lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL4, pol));

    // s1
    separate_ffe_taps(taps[4], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S1_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S1_LSB, lsb));

    // s2
    separate_ffe_taps(taps[5], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S2_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S2_LSB, lsb));

    // sf
    separate_ffe_taps(taps[6], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_SF_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_SF_LSB, lsb));
    return CR_OK;
}

CredoError_t canary_set_f1over3(CredoSlice_t* slice, int lane, int value) {
    return writeRegLane(slice, lane, REG_FLR_RX_F1OVER3, value);
}

CredoError_t canary_set_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    unsigned int agcgain1 = 0;
    unsigned int agcgain2 = 0;

    agcgain1 = bin_gray(agcgain[0]);
    agcgain2 = bin_gray(agcgain[1]);

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGCGAIN1, agcgain1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGCGAIN2, agcgain2));

    return CR_OK;
}

CredoError_t canary_set_ctle(CredoSlice_t* slice, int lane, unsigned ctle[]) {
    /* Always use entry 7 */
    unsigned ctle1 = ctle[0] & 0xf;
    unsigned ctle2 = ctle[1] & 0xf;
    unsigned ctle_msb = (((ctle1 >> 3) & 1) << 1) + ((ctle2 >> 3) & 1);
    unsigned ctle_combined = ((ctle1 & 0x7) << 3) + (ctle2 & 0x7);

    CredoLaneMode_t mode;
    // ERR_PROPS( get_lane_mode(slice, lane, &mode));
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_AGC_ENTRY7, ctle_combined));
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_AGC_OW, 7));
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_AGC_OWEN, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGC_DEGEN, ctle_msb));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_AGC_ENTRY7, ctle_combined));
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_AGC_OW, 7));
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_AGC_OWEN, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGC_DEGEN, ctle_msb));
            break;
        default:
            LOGS_WARN("[Set CTLE] Unknown mode %d, skipped", mode);
            break;
    }
    return CR_OK;
}

CredoError_t canary_set_delta_phase(CredoSlice_t* slice, int lane, int value) {
    CredoLaneMode_t mode;
    // ERR_PROPS( get_lane_mode(slice, lane, &mode));
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_DELTA_OW, value));
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_DELTA_OWEN, 1));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_DELTA_OW, value));
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_DELTA_OWEN, 1));
            break;
        default:
            LOGS_WARN("[Set delta phase] Unknown mode %d, skipped", mode);
            break;
    }

    return CR_OK;
}

CredoError_t canary_set_edge(CredoSlice_t* slice, int lane, const unsigned edge) {
    return writeRegLane(slice, lane, REG_RX_EDGE, edge);
}

CredoError_t canary_lane_rx_reset_nrz(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_LANE_RST, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_SM_CONT, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_SM_CONT, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_LANE_RST, 0));
    return CR_OK;
}

CredoError_t canary_lane_rx_reset_pam4(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_LANE_RST, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_SM_CONT, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_SM_CONT, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_LANE_RST, 0));
    return CR_OK;
}

CredoError_t canary_lane_rx_reset(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    if (mode == CR_LMODE_PAM4) {
        return canary_lane_rx_reset_pam4(slice, lane);
    } else {
        if (mode != CR_LMODE_NRZ) LOGS_WARN("[RX lane reset] Unknown mode %d, assuming NRZ", mode);
        return canary_lane_rx_reset_nrz(slice, lane);
    }
}
