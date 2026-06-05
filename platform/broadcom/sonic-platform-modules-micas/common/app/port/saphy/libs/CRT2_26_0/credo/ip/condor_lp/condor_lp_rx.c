/*
 * Condor RX information and debugging
 */
#include "project.h"

#include "condor_lp/condor_lp_regmap.h"
#include "condor_lp/condor_lp_serdes.h"

#include "utility.h"
#include "sdk.h"

#include <stdlib.h>

CredoError_t condor_lp_get_version_id(CredoSlice_t* slice, int lane, uint64_t* version) {
    unsigned ip_id = 0, revision = 0, subrevision = 0;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_VER_ID_47_32, &ip_id));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VER_ID_31_16, &revision));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_VER_ID_15_00, &subrevision));
    *version = 0;
    *version = ((uint64_t)ip_id << 32) | (revision << 16) | subrevision;
    return CR_OK;
}

CredoError_t condor_lp_set_rx_sm_bp1_nrz(CredoSlice_t* slice, int lane, int enable, int state) {
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_BREAKPOINT_1_EN, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_BREAKPOINT_1_STATE, state));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_sm_bp1_pam4(CredoSlice_t* slice, int lane, int enable, int state) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_BREAKPOINT_1_EN, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_BREAKPOINT_1_STATE, state));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_sm_bp1_nrz(CredoSlice_t* slice, int lane, int* enable, int* state, int* reached) {
    ERR_PROPS(readRegLane(slice, lane, REG_NRZ_BREAKPOINT_1_EN, (unsigned*)enable));
    ERR_PROPS(readRegLane(slice, lane, REG_NRZ_BREAKPOINT_1_STATE, (unsigned*)state));
    ERR_PROPS(readRegLane(slice, lane, REG_NRZ_BREAKPOINT_1_REACHED, (unsigned*)reached));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_sm_bp1_pam4(CredoSlice_t* slice, int lane, int* enable, int* state, int* reached) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_BREAKPOINT_1_EN, (unsigned*)enable));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_BREAKPOINT_1_STATE, (unsigned*)state));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_BREAKPOINT_1_REACHED, (unsigned*)reached));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_sm_bp2_nrz(CredoSlice_t* slice, int lane, int enable, int state) {
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_BREAKPOINT_2_EN, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_BREAKPOINT_2_STATE, state));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_sm_bp2_pam4(CredoSlice_t* slice, int lane, int enable, int state) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_BREAKPOINT_2_EN, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_BREAKPOINT_2_STATE, state));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_sm_bp2_nrz(CredoSlice_t* slice, int lane, int* enable, int* state, int* reached) {
    ERR_PROPS(readRegLane(slice, lane, REG_NRZ_BREAKPOINT_2_EN, (unsigned*)enable));
    ERR_PROPS(readRegLane(slice, lane, REG_NRZ_BREAKPOINT_2_STATE, (unsigned*)state));
    ERR_PROPS(readRegLane(slice, lane, REG_NRZ_BREAKPOINT_2_REACHED, (unsigned*)reached));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_sm_bp2_pam4(CredoSlice_t* slice, int lane, int* enable, int* state, int* reached) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_BREAKPOINT_2_EN, (unsigned*)enable));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_BREAKPOINT_2_STATE, (unsigned*)state));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_BREAKPOINT_2_REACHED, (unsigned*)reached));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_sm_state_nrz(CredoSlice_t* slice, int lane, int* state) {
    return readRegLane(slice, lane, REG_NRZ_READ_STATE_NUMBER_Q, (unsigned*)state);
}

CredoError_t condor_lp_get_rx_sm_state_pam4(CredoSlice_t* slice, int lane, int* state) {
    return readRegLane(slice, lane, REG_RX_READ_STATE_NUMBER_Q, (unsigned*)state);
}

CredoError_t condor_lp_continue_rx_sm_nrz(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_STATEMACHINE_CONTINUE, 1));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_STATEMACHINE_CONTINUE, 0));
    return CR_OK;
}

CredoError_t condor_lp_continue_rx_sm_pam4(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STATEMACHINE_CONTINUE, 1));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STATEMACHINE_CONTINUE, 0));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_sm_counter_target_nrz(CredoSlice_t* slice, int lane, int target) {
    return writeRegLane(slice, lane, REG_NRZ_TOSM_CNTR_TARGET, target);
}

CredoError_t condor_lp_get_rx_sm_counter_target_nrz(CredoSlice_t* slice, int lane, int* target) {
    return readRegLane(slice, lane, REG_NRZ_TOSM_CNTR_TARGET, (unsigned*)target);
}

CredoError_t condor_lp_get_rx_ffe_stage_count(CredoSlice_t* slice, int lane, int* stages) {
    *stages = 4;
    return CR_OK;
}

CredoError_t condor_lp_get_rx_ffe_tap_count(CredoSlice_t* slice, int lane, int* k_taps, int* s_taps) {
    *k_taps = 5;
    *s_taps = 3;
    return CR_OK;
}

CredoError_t condor_lp_get_rx_ffe_powerup(CredoSlice_t* slice, int lane, int stages[]) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_PU_SUM_0, (unsigned*)stages));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_PU_SUM_1, (unsigned*)stages + 1));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_PU_SUM_2, (unsigned*)stages + 2));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_PU_SUM_3, (unsigned*)stages + 3));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_ffe_powerup(CredoSlice_t* slice, int lane, int stages[]) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PU_SUM_0, stages[0]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PU_SUM_1, stages[1]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PU_SUM_2, stages[2]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_PU_SUM_3, stages[3]));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_ffe_polarity(CredoSlice_t* slice, int lane, int ktaps[]) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL1, (unsigned*)ktaps));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL2, (unsigned*)ktaps + 1));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL3, (unsigned*)ktaps + 2));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL4, (unsigned*)ktaps + 3));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL5, (unsigned*)ktaps + 4));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_ffe_polarity(CredoSlice_t* slice, int lane, int ktaps[]) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL1, ktaps[0]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL2, ktaps[1]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL3, ktaps[2]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL4, ktaps[3]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL5, ktaps[4]));
    return CR_OK;
}

/* Condor LP FFE: polarity=1 -> positive. */
static inline int combine_ffe_taps(unsigned pol, unsigned gray_msb, unsigned gray_lsb) {
    int val = (gray_bin(gray_msb) << 4) + (gray_bin(gray_lsb));
    return (pol == 0) ? (-val - 1) : val;
}

static inline void separate_ffe_taps(int value, unsigned* pol, unsigned* gray_msb, unsigned* gray_lsb) {
    if (value < 0) {
        value = -value - 1;
        *pol = 0;
    } else {
        *pol = 1;
    }
    *gray_msb = bin_gray((value >> 4) & 0xf);
    *gray_lsb = bin_gray(value & 0xf);
}

CredoError_t condor_lp_get_rx_ffe(CredoSlice_t* slice, int lane, int taps[]) {
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

    // K5
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_POL5, &pol));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K5_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_K5_LSB, &lsb));
    taps[4] = combine_ffe_taps(pol, msb, lsb);

    // S0
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S0_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S0_LSB, &lsb));
    taps[5] = combine_ffe_taps(1, msb, lsb);

    // S1
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S1_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S1_LSB, &lsb));
    taps[6] = combine_ffe_taps(1, msb, lsb);

    // S2
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S2_MSB, &msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_S2_LSB, &lsb));
    taps[7] = combine_ffe_taps(1, msb, lsb);

    return CR_OK;
}

CredoError_t condor_lp_set_rx_ffe(CredoSlice_t* slice, int lane, const int taps[]) {
    unsigned int pol = 0;
    unsigned int msb = 0;
    unsigned int lsb = 0;

    // K1
    separate_ffe_taps(taps[0], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K1_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K1_LSB, lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL1, pol));

    // K2
    separate_ffe_taps(taps[1], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K2_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K2_LSB, lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL2, pol));

    // K3
    separate_ffe_taps(taps[2], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K3_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K3_LSB, lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL3, pol));

    // K4
    separate_ffe_taps(taps[3], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K4_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K4_LSB, lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL4, pol));

    // K5
    separate_ffe_taps(taps[4], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K5_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_K5_LSB, lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_POL5, pol));

    // S0
    separate_ffe_taps(taps[5], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S0_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S0_LSB, lsb));

    // S1
    separate_ffe_taps(taps[6], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S1_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S1_LSB, lsb));

    // S2
    separate_ffe_taps(taps[7], &pol, &msb, &lsb);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S2_MSB, msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_S2_LSB, lsb));

    return CR_OK;
}

CredoError_t condor_lp_get_rx_ffe_fine(CredoSlice_t* slice, int lane, int taps[]) {
    unsigned int val = 0;

    // K1
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_MAIN1, &val));
    taps[0] = gray_bin(val);

    // K2
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_MAIN2, &val));
    taps[1] = gray_bin(val);

    // K3
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_MAIN3, &val));
    taps[2] = gray_bin(val);

    // K4
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_MAIN4, &val));
    taps[3] = gray_bin(val);

    // K5
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_SUM4, &val));
    taps[4] = gray_bin(val);

    // S0
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_SUM1, &val));
    taps[5] = gray_bin(val);

    // S1
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_SUM2, &val));
    taps[6] = gray_bin(val);

    // S2
    ERR_PROPS(readRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_SUM3, &val));
    taps[7] = gray_bin(val);

    return CR_OK;
}

CredoError_t condor_lp_set_rx_ffe_fine(CredoSlice_t* slice, int lane, const int taps[]) {
    unsigned int val = 0;

    // K1
    val = bin_gray(taps[0]);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_MAIN1, val));

    // K2
    val = bin_gray(taps[1]);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_MAIN2, val));

    // K3
    val = bin_gray(taps[2]);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_MAIN3, val));

    // K4
    val = bin_gray(taps[3]);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_MAIN4, val));

    // K5
    val = bin_gray(taps[4]);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_SUM4, val));

    // S0
    val = bin_gray(taps[5]);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_SUM1, val));

    // S1
    val = bin_gray(taps[6]);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_SUM2, val));

    // S2
    val = bin_gray(taps[7]);
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_FFE_VGAVDSAT_SUM3, val));

    return CR_OK;
}

CredoError_t condor_lp_set_rx_skef(CredoSlice_t* slice, int lane, int enable, int degen, int addcap, int skef_gain) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_DEGEN, degen));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_EN, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_ADDCAP, addcap));

#ifdef CONDOR_LP_1P2
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_GAIN_LSB, reverse_bits(skef_gain & 0x3, 2)));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_GAIN_MSB, skef_gain >> 2));
#else
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_GAIN, skef_gain & 0x7));
#endif
    return CR_OK;
}

CredoError_t condor_lp_get_rx_skef(CredoSlice_t* slice, int lane, int* enable, int* degen, int* addcap,
                                   int* skef_gain) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_EN, (unsigned*)enable));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_DEGEN, (unsigned*)degen));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_ADDCAP, (unsigned*)addcap));

#ifdef CONDOR_LP_1P2
    unsigned skef_gain_lsb, skef_gain_msb;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_GAIN_LSB, &skef_gain_lsb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_GAIN_MSB, &skef_gain_msb));

    *skef_gain = (skef_gain_msb << 2) | reverse_bits(skef_gain_lsb, 2);
#else
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_GAIN, (unsigned*)skef_gain));
#endif
    return CR_OK;
}

CredoError_t condor_lp_set_rx_skef_enable(CredoSlice_t* slice, int lane, int skef_en) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_EN, skef_en));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_skef_enable(CredoSlice_t* slice, int lane, int* skef_en) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_EN, (unsigned*)skef_en));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_skef_degen(CredoSlice_t* slice, int lane, int skef_degen) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_DEGEN, skef_degen));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_skef_degen(CredoSlice_t* slice, int lane, int* skef_degen) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_DEGEN, (unsigned*)skef_degen));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_skef_addcap(CredoSlice_t* slice, int lane, int skef_addcap) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_ADDCAP, skef_addcap));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_skef_addcap(CredoSlice_t* slice, int lane, int* skef_addcap) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_ADDCAP, (unsigned*)skef_addcap));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_skef_gain(CredoSlice_t* slice, int lane, int skef_gain) {
#ifdef CONDOR_LP_1P2
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_GAIN_LSB, reverse_bits(skef_gain & 0x3, 2)));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_GAIN_MSB, skef_gain >> 2));
#else
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SKEF_GAIN, skef_gain & 0x7));
#endif
    return CR_OK;
}

CredoError_t condor_lp_get_rx_skef_gain(CredoSlice_t* slice, int lane, int* skef_gain) {
#ifdef CONDOR_LP_1P2
    unsigned skef_gain_lsb, skef_gain_msb;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_GAIN_LSB, &skef_gain_lsb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_GAIN_MSB, &skef_gain_msb));

    *skef_gain = (skef_gain_msb << 2) | reverse_bits(skef_gain_lsb, 2);

#else
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SKEF_GAIN, (unsigned*)skef_gain));
#endif
    return CR_OK;
}

CredoError_t condor_lp_set_rx_ctle(CredoSlice_t* slice, int lane, unsigned ctle[]) {
    unsigned int ctle1 = ctle[0] & 0x1f;
    unsigned int ctle2 = ctle[1] & 0x1f;
    unsigned int ctle_val = (ctle2 << 5) | ctle1;

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGC_DEGEN, ctle_val));

    return CR_OK;
}

CredoError_t condor_lp_get_rx_ctle_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (count == NULL) return CR_INVALID_ARGS;
    *count = 2;
    return CR_OK;
}

CredoError_t condor_lp_get_rx_ctle(CredoSlice_t* slice, int lane, unsigned ctle[]) {
    unsigned int ctle_val;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGC_DEGEN, &ctle_val));

    ctle[0] = ctle_val & 0x1f;
    ctle[1] = ctle_val >> 5;

    return CR_OK;
}

CredoError_t condor_lp_set_f1over3(CredoSlice_t* slice, int lane, int value) {
    return writeRegLane(slice, lane, REG_RX_TOSM_F1OVER3_INIT, value);
}

CredoError_t condor_lp_set_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
#ifdef CONDOR_LP_1P2
    unsigned int agcgain1 = 0;
    unsigned int agcgain2 = 0;

    agcgain1 = bin_gray(agcgain[0]);
    agcgain2 = bin_gray(agcgain[1]);

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGCGAIN1_LSB, (agcgain1 & 0x1f)));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGCGAIN1_MSB, reverse_bits(agcgain1 >> 5, 2)));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGCGAIN2, reverse_bits(agcgain2, 7)));
#else
    unsigned int agcgain1 = 0;
    unsigned int agcgain2 = 0;

    agcgain1 = bin_gray(agcgain[0]);
    agcgain2 = bin_gray(agcgain[1]);

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGCGAIN1_LSB, (agcgain1 & 0x1f)));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGCGAIN1_MSB, agcgain1 >> 5));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_AGCGAIN2, agcgain2));
#endif
    return CR_OK;
}

CredoError_t condor_lp_get_f1over3(CredoSlice_t* slice, int lane, int* value) {
    return readRegSignedLane(slice, lane, REG_RX_TOSM_F1OVER3_INIT, value);
}

CredoError_t condor_lp_get_rx_delta(CredoSlice_t* slice, int lane, int* delta) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(readRegSignedLane(slice, lane, REG_NRZ_READ_DELTA_Q, delta));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_READ_DELTA_Q, delta));
            break;
        case CR_LMODE_DISABLE:
            *delta = 0;
            break;
        default:
            LOGS_WARN("[Get RX DELTA] Unknown mode %d, return 0", mode);
            *delta = 0;
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_agcgain_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (count == NULL) return CR_INVALID_ARGS;
    *count = 2;
    return CR_OK;
}

CredoError_t condor_lp_get_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
#ifdef CONDOR_LP_1P2
    unsigned int agcgain1_lsb = 0;
    unsigned int agcgain1_msb = 0;
    unsigned int agcgain2 = 0;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGCGAIN1_LSB, &agcgain1_lsb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGCGAIN1_MSB, &agcgain1_msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGCGAIN2, &agcgain2));

    agcgain1_msb = reverse_bits(agcgain1_msb, 2);
    agcgain2 = reverse_bits(agcgain2, 7);

    agcgain[0] = gray_bin((agcgain1_msb << 5) | (agcgain1_lsb));
    agcgain[1] = gray_bin(agcgain2);
#else
    unsigned int agcgain1_lsb = 0;
    unsigned int agcgain1_msb = 0;
    unsigned int agcgain2 = 0;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGCGAIN1_LSB, &agcgain1_lsb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGCGAIN1_MSB, &agcgain1_msb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_AGCGAIN2, &agcgain2));

    agcgain[0] = gray_bin((agcgain1_msb << 5) | (agcgain1_lsb));
    agcgain[1] = gray_bin(agcgain2);
#endif
    return CR_OK;
}

CredoError_t condor_lp_set_rx_atten(CredoSlice_t* slice, int lane, int passive, int gain, int termtune) {
    if (condor_lp_set_rx_atten_en3db(slice, lane, passive)) return CR_FAIL;
    if (condor_lp_set_rx_atten_agcgain(slice, lane, gain)) return CR_FAIL;

    // skip, always set it to 2 inside attenuation function for all scenarios, by Kevin
    // if (condor_lp_set_rx_atten_termtune(slice, lane, termtune)) return CR_FAIL;
    return CR_OK;
}

CredoError_t condor_lp_get_rx_atten(CredoSlice_t* slice, int lane, int* passive, int* gain, int* termtune) {
    if (passive) {
        if (condor_lp_get_rx_atten_en3db(slice, lane, passive)) return CR_FAIL;
    }
    if (gain) {
        if (condor_lp_get_rx_atten_agcgain(slice, lane, gain)) return CR_FAIL;
    }
    if (termtune) {
        if (condor_lp_get_rx_atten_termtune(slice, lane, termtune)) return CR_FAIL;
    }
    return CR_OK;
}

CredoError_t condor_lp_set_rx_atten_en3db(CredoSlice_t* slice, int lane, int en3db) {
    return writeRegLane(slice, lane, REG_RX_3DB_ATTEN_EN, en3db);
}

CredoError_t condor_lp_get_rx_atten_en3db(CredoSlice_t* slice, int lane, int* en3db) {
    return readRegLane(slice, lane, REG_RX_3DB_ATTEN_EN, (unsigned*)en3db);
}

CredoError_t condor_lp_set_rx_atten_termtune(CredoSlice_t* slice, int lane, int termtune) {
    return writeRegLane(slice, lane, REG_RX_TERM_TUNE, termtune);
}

CredoError_t condor_lp_get_rx_atten_termtune(CredoSlice_t* slice, int lane, int* termtune) {
    return readRegLane(slice, lane, REG_RX_TERM_TUNE, (unsigned*)termtune);
}

CredoError_t condor_lp_set_rx_atten_agcgain(CredoSlice_t* slice, int lane, int agcgain) {
    return writeRegLane(slice, lane, REG_RX_AGCGAIN_ATTN, agcgain);
}

CredoError_t condor_lp_get_rx_atten_agcgain(CredoSlice_t* slice, int lane, int* agcgain) {
    return readRegLane(slice, lane, REG_RX_AGCGAIN_ATTN, (unsigned*)agcgain);
}

static const unsigned edgeindex[14][4] = {
    {0, 0, 0, 0},   // 15.6
    {1, 0, 0, 1},   // 16.94
    {2, 0, 0, 2},   // 18.34
    {3, 0, 0, 3},   // 19.75
    {4, 0, 2, 3},   // 20.01
    {5, 0, 1, 3},   // 20.64
    {6, 0, 3, 3},   // 22.15
    {7, 1, 0, 0},   // 23.28
    {8, 1, 0, 1},   // 24.43
    {9, 1, 0, 2},   // 25.67
    {10, 1, 0, 3},  // 26.95
    {11, 1, 2, 3},  // 27.09
    {12, 1, 1, 3},  // 27.44
    {13, 1, 3, 3},  // 28.43
};

static const float edge_delay[14] = {15.6,  16.94, 18.34, 19.75, 20.01, 20.64, 22.15,
                                     23.28, 24.43, 25.67, 26.95, 27.09, 27.44, 28.43};

CredoError_t condor_lp_set_edge(CredoSlice_t* slice, int lane, const unsigned edge) {
    if (edge > 14) {
        LOGS_ERROR("[Set Rx Edge] Wrong index");
    }
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_EDGE, edgeindex[edge][3] * 0x1111));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_EDGE_SEL, edgeindex[edge][1]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_EDGE_CLK_GM_SEL, edgeindex[edge][2] * 0x1111));
    LOGS_INFO("[Set Rx Edge] Using IDX:%d\nedge_sel=%d\nedge_clk_gm_sel=%d\naddcap=%d\ndelay=%.2f", edgeindex[edge][0],
              edgeindex[edge][1], edgeindex[edge][2], edgeindex[edge][3], edge_delay[edge]);
    return CR_OK;
}

CredoError_t condor_lp_get_edge(CredoSlice_t* slice, int lane, unsigned* value) {
    unsigned edge, edge_sel, edge_clk_gm_sel;
    int i;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_EDGE, &edge));
    edge = edge & 0xF;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_EDGE_SEL, &edge_sel));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_EDGE_CLK_GM_SEL, &edge_clk_gm_sel));
    for (i = 0; i < 14; i++) {
        if (edge_sel == edgeindex[i][1] && edge_clk_gm_sel == edgeindex[i][2] && edge == edgeindex[i][3]) {
            value[0] = edgeindex[i][0];
            break;
        }
    }
    if (i == 14) {
        value[0] = i;
        LOGS_WARN("[Get Rx Edge] Values aren't match with table");
    }

    // value[1] = edge_sel;
    // value[2] = edge_clk_gm_sel;
    // value[3] = edge;

    return CR_OK;
}

CredoError_t condor_lp_set_rx_delta(CredoSlice_t* slice, int lane, int delta) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_DELTA_Q, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OW_DELTA_Q, delta));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_dac(CredoSlice_t* slice, int lane, int rx_dac) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_set_rx_dac_nrz(slice, lane, rx_dac));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_set_rx_dac_pam4(slice, lane, rx_dac));
            break;
        case CR_LMODE_DISABLE:
            // do nothing
            break;
        default:
            LOGS_WARN("[Set RX DAC] Unknown mode %d, setting NRZ DAC", mode);
            ERR_PROPS(condor_lp_set_rx_dac_nrz(slice, lane, rx_dac));
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_set_rx_dac_nrz(CredoSlice_t* slice, int lane, int rx_dac) {
    // TODO: add true writing ability with get enable bit
    ERR_PROPS(writeRegLane(slice, lane, REG_OWEN_FROM_SM_DAC_SEL, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_OW_FROM_SM_DAC_SEL, rx_dac));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_dac_pam4(CredoSlice_t* slice, int lane, int rx_dac) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_DAC_OW, rx_dac));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_DAC_OWEN, 1));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_dac(CredoSlice_t* slice, int lane, int* rx_dac) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_get_rx_dac_nrz(slice, lane, rx_dac));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_get_rx_dac_pam4(slice, lane, rx_dac));
            break;
        case CR_LMODE_DISABLE:
            *rx_dac = 0;
            break;
        default:
            LOGS_WARN("[Get RX DAC] Unknown mode %d, return 0", mode);
            *rx_dac = 0;
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_dac_nrz(CredoSlice_t* slice, int lane, int* rx_dac) {
    unsigned owen = 0;
    ERR_PROPS(readRegLane(slice, lane, REG_OWEN_FROM_SM_DAC_SEL, &owen));
    return (owen == 1) ? readRegLane(slice, lane, REG_OW_FROM_SM_DAC_SEL, (unsigned*)rx_dac)
                       : readRegLane(slice, lane, REG_READ_DAC_SEL_Q, (unsigned*)rx_dac);
}

CredoError_t condor_lp_get_rx_dac_pam4(CredoSlice_t* slice, int lane, int* rx_dac) {
    unsigned owen = 0;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_DAC_OWEN, &owen));
    return (owen == 1) ? readRegLane(slice, lane, REG_RX_DAC_OW, (unsigned*)rx_dac)
                       : readRegLane(slice, lane, REG_RX_DAC, (unsigned*)rx_dac);
}

CredoError_t condor_lp_get_rx_ths(CredoSlice_t* slice, int lane, int ths[]) {
    for (int i = 0; i < 12; i++) {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_PAM4_DFE_SEL, i));
        ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_PAM4_DFE_RD, ths + i));
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_dfe(CredoSlice_t* slice, int lane, double dfe_taps[]) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_get_rx_dfe_nrz(slice, lane, dfe_taps));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_get_rx_dfe_pam4(slice, lane, dfe_taps));
            break;
        case CR_LMODE_DISABLE:
            dfe_taps[0] = dfe_taps[1] = dfe_taps[2] = 0;
            break;
        default:
            LOGS_WARN("[Get RX dfe] Unknown mode %d, returning all 0", mode);
            dfe_taps[0] = dfe_taps[1] = dfe_taps[2] = 0;
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_dfe_nrz(CredoSlice_t* slice, int lane, double dfe_taps[]) {
    int dfe[3];
    ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_F1_NRZ, dfe + 0));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_F2_NRZ, dfe + 1));
    ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_F3_NRZ, dfe + 2));
    dfe_taps[0] = dfe[0];
    dfe_taps[1] = dfe[1];
    dfe_taps[2] = dfe[2];
    return CR_OK;
}

/* For PAM4, you should really call firmware version. */
CredoError_t condor_lp_get_rx_dfe_pam4(CredoSlice_t* slice, int lane, double dfe_taps[]) {
    int ths_list[12] = {0};
    ERR_PROPS(condor_lp_get_rx_ths(slice, lane, ths_list));

    dfe_taps[0] = (-3.0f / 16) * ((ths_list[0] - ths_list[2]) + (ths_list[3] - ths_list[5]) +
                                  (ths_list[6] - ths_list[8]) + (ths_list[9] - ths_list[11]));
    dfe_taps[1] = (-3.0f / 20) *
                  ((ths_list[0] + ths_list[1] + ths_list[2] - ths_list[9] - ths_list[10] - ths_list[11]) +
                   (1.0f / 3) * (ths_list[3] + ths_list[4] + ths_list[5] - ths_list[6] - ths_list[7] - ths_list[8]));
    dfe_taps[0] /= 2048.0;
    dfe_taps[1] /= 2048.0;
    dfe_taps[2] = (dfe_taps[0] == 0) ? 0 : dfe_taps[1] / dfe_taps[0];

    return CR_OK;
}

CredoError_t condor_lp_get_rx_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    eyes[0] = eyes[1] = eyes[2] = 0;
    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_get_rx_eye_nrz(slice, lane, &eyes[0]));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_get_rx_eye_pam4(slice, lane, eyes));
            break;
        case CR_LMODE_DISABLE:
            break;
        default:
            LOGS_WARN("[Read eye] Unknown mode %d, return 0", mode);
            break;
    }

    return CR_OK;
}

CredoError_t condor_lp_get_rx_eye_nrz(CredoSlice_t* slice, int lane, int* eye) {
    int dac_val;
    unsigned int eye_reg_val, eye_reg_val2;
    int diff;
    int i;

    ERR_PROPS(condor_lp_get_rx_dac(slice, lane, &dac_val));
    for (i = 0; i < 100; i++) {
        ERR_PROPS(readRegLane(slice, lane, REG_RX_EM, &eye_reg_val));
        ERR_PROPS(readRegLane(slice, lane, REG_RX_EM, &eye_reg_val2));
        diff = eye_reg_val - eye_reg_val2;
        if (diff < 50 && diff > -50) break;
    }
    *eye = (eye_reg_val / 2048.0) * (200 + 50.0 * dac_val);

    return CR_OK;
}

/* If firmware is running, PAM4 eye might be inaccurate. Prefer to use firmware eye read. */
CredoError_t condor_lp_get_rx_eye_pam4(CredoSlice_t* slice, int lane, int eyes[3]) {
    int dac_val;
    int i, j;
    int sel;
    int plus_margin, minus_margin, diff;
    unsigned minus_margin_L;
    int minus_margin_H;
    int result1;

    ERR_PROPS(condor_lp_get_rx_dac(slice, lane, &dac_val));
    for (i = 0; i < 3; i++) {
        result1 = 0x10000;
        for (j = 0; j < 4; j++) {
            sel = 3 * j + i;
            ERR_PROPS(writeRegLane(slice, lane, REG_RX_PLUS_MARGIN_SEL, sel));
            ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_PLUS_MARGIN, &plus_margin));

            ERR_PROPS(writeRegLane(slice, lane, REG_RX_MINUS_MARGIN_SEL, sel));
            ERR_PROPS(readRegLane(slice, lane, REG_RX_MINUS_MARGIN_L, &minus_margin_L));
            ERR_PROPS(readRegSignedLane(slice, lane, REG_RX_MINUS_MARGIN_H, &minus_margin_H));
            minus_margin = (minus_margin_H << 8) | (minus_margin_L);

            diff = plus_margin - minus_margin;

            if (diff < result1) result1 = diff;
        }
        eyes[i] = (result1 / 2048.0) * (100 + 50.0 * dac_val);
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_lane_ready(CredoSlice_t* slice, int lane, int* ready) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_get_rx_lane_ready_nrz(slice, lane, ready));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_get_rx_lane_ready_pam4(slice, lane, ready));
            break;
        case CR_LMODE_DISABLE:
            *ready = 0;
            break;
        default:
            LOGS_WARN("[Get lane ready] Unknown mode %d, returning not ready", mode);
            *ready = 0;
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_lane_ready_nrz(CredoSlice_t* slice, int lane, int* ready) {
    unsigned value = 0;
    unsigned state;

    ERR_PROPS(readRegLane(slice, lane, REG_NRZ_READ_STATE_NUMBER_Q, &state));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_RDY_NRZ, &value));
    // if (state > 5) value |= (1 << 8);
    *ready = value;

    return CR_OK;
}

CredoError_t condor_lp_get_rx_lane_ready_pam4(CredoSlice_t* slice, int lane, int* ready) {
    unsigned value = 0;
    unsigned state;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_STATE_NUMBER_Q, &state));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_RDY, &value));
    // if (state > 5) value |= (1 << 8);
    *ready = value;
    return CR_OK;
}

CredoError_t condor_lp_get_rx_sd(CredoSlice_t* slice, int lane, int* sd) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_get_rx_sd_nrz(slice, lane, sd));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_get_rx_sd_pam4(slice, lane, sd));
            break;
        case CR_LMODE_DISABLE:
            *sd = 0;
            break;
        default:
            LOGS_WARN("[Get signal detect] Unknown mode %d, returning not detect", mode);
            *sd = 0;
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_sd_nrz(CredoSlice_t* slice, int lane, int* sd) {
    return readRegLane(slice, lane, REG_RX_SD_NRZ, (unsigned*)sd);
}

CredoError_t condor_lp_get_rx_sd_pam4(CredoSlice_t* slice, int lane, int* sd) {
    return readRegLane(slice, lane, REG_RX_SD, (unsigned*)sd);
}

CredoError_t condor_lp_lane_rx_reset(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_lane_rx_reset_nrz(slice, lane));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_lane_rx_reset_pam4(slice, lane));
            break;
        case CR_LMODE_DISABLE:
            break;
        default:
            LOGS_WARN("[RX lane reset] Unknown mode %d, assuming NRZ", mode);
            ERR_PROPS(condor_lp_lane_rx_reset_nrz(slice, lane));
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_lane_rx_reset_nrz(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_LANE_RST_NRZ, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_STATEMACHINE_CONTINUE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_STATEMACHINE_CONTINUE, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_LANE_RST_NRZ, 0));
    return CR_OK;
}

CredoError_t condor_lp_lane_rx_reset_pam4(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_LANE_RST, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STATEMACHINE_CONTINUE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STATEMACHINE_CONTINUE, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_LANE_RST, 0));
    return CR_OK;
}

CredoError_t condor_lp_reset_lane_fast_nrz(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_TOSM_STATE_RESET, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_ADC_CAL_START, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OW_ADC_CAL_START, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_OWEN_TOSM_CAL_DONE, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_OW_TOSM_CAL_DONE, 0));

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_TOSM_STATE_RESET, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_NRZ_OWEN_TOSM_CAL_DONE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_ADC_CAL_START, 0));

    return CR_OK;
}

CredoError_t condor_lp_reset_lane_fast_pam4(CredoSlice_t* slice, int lane) {
    int theta[3] = {2, 3, 0};
    int i;
    unsigned state_number;
    unsigned updn_org;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_THETA2_UPDATE_MODE, &updn_org));
    ERR_PROPS(condor_lp_updn_control(slice, lane, 0, theta, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SM_RST, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STATEMACHINE_CONTINUE, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_ADC_CAL_START_Q, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OW_ADC_CAL_START_Q, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_TOSM_CAL_DONE, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OW_TOSM_CAL_DONE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STATEMACHINE_CONTINUE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SM_RST, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_TOSM_CAL_DONE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_ADC_CAL_START_Q, 0));

    for (i = 0; i < 5000; i++) {
        ERR_PROPS(readRegLane(slice, lane, REG_RX_READ_STATE_NUMBER_Q, &state_number));
        if (state_number >= 0x12) break;
        sleep_ms(1);
    }
    ERR_PROPS(condor_lp_updn_control(slice, lane, updn_org, theta, 1));

    return CR_OK;
}

CredoError_t condor_lp_reset_lane_simple_pam4(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SM_RST, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STATEMACHINE_CONTINUE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STATEMACHINE_CONTINUE, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STEP_STATE_MACHINE_EN, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STEP_STATE_MACHINE, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STEP_STATE_MACHINE, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_SM_RST, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_STEP_STATE_MACHINE_EN, 0));

    return CR_OK;
}

CredoError_t condor_lp_updn_control(CredoSlice_t* slice, int lane, int en, int theta[], int flip) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_UPDN_PHASE2_MUX, theta[0]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_UPDN_PHASE3_MUX, theta[1]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_UPDN_PHASE4_MUX, theta[2]));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_CNTR6_ENABLE, flip));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_CNTR7_ENABLE, flip));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_CNTR8_ENABLE, flip));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_THETA2_UPDATE_MODE, en));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_THETA3_UPDATE_MODE, en));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_THETA4_UPDATE_MODE, en));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_THETA4_ACC, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_THETA3_ACC, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_THETA2_ACC, 0));

    return CR_OK;
}

CredoError_t condor_lp_set_rx_updown_mode(CredoSlice_t* slice, int lane, int en, int t2, int t3, int t4, int flip) {
    int theta[3];

    theta[0] = t2;
    theta[1] = t3;
    theta[2] = t4;

    return condor_lp_updn_control(slice, lane, en, theta, flip);
}

CredoError_t condor_lp_get_rx_kp(CredoSlice_t* slice, int lane, int* kp) {
    CredoLaneMode_t mode;

    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            if (condor_lp_get_rx_kp_nrz(slice, lane, kp)) return CR_FAIL;
            break;
        case CR_LMODE_PAM4:
            if (condor_lp_get_rx_kp_pam4(slice, lane, kp)) return CR_FAIL;
            break;
        case CR_LMODE_DISABLE:
            *kp = 0;
            break;
        default:
            LOGS_WARN("[Get RX KP] Unknown mode %d, returning 0", mode);
            *kp = 0;
            break;
    }

    return CR_OK;
}

CredoError_t condor_lp_set_rx_kp(CredoSlice_t* slice, int lane, int kp) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_set_rx_kp_nrz(slice, lane, kp));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_set_rx_kp_pam4(slice, lane, kp));
            break;
        case CR_LMODE_DISABLE:
            // do nothing
            break;
        default:
            LOGS_WARN("[Set RX KP] Unknown mode %d, setting NRZ KP", mode);
            ERR_PROPS(condor_lp_set_rx_kp_nrz(slice, lane, kp));
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_kp_nrz(CredoSlice_t* slice, int lane, int* kp) {
    return readRegLane(slice, lane, REG_RX_TOSM_KP_S6_NRZ, (unsigned*)kp);
}

CredoError_t condor_lp_set_rx_kp_nrz(CredoSlice_t* slice, int lane, int kp) {
    return writeRegLane(slice, lane, REG_RX_TOSM_KP_S6_NRZ, kp);
}

CredoError_t condor_lp_get_rx_kp_pam4(CredoSlice_t* slice, int lane, int* kp) {
    return readRegLane(slice, lane, REG_RX_READ_KP_Q, (unsigned*)kp);
}

CredoError_t condor_lp_set_rx_kp_pam4(CredoSlice_t* slice, int lane, int kp) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_KP_Q, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OW_KP_Q, kp));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_kf(CredoSlice_t* slice, int lane, int* kf) {
    CredoLaneMode_t mode;

    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            if (condor_lp_get_rx_kf_nrz(slice, lane, kf)) return CR_FAIL;
            break;
        case CR_LMODE_PAM4:
            if (condor_lp_get_rx_kf_pam4(slice, lane, kf)) return CR_FAIL;
            break;
        case CR_LMODE_DISABLE:
            *kf = 0;
            break;
        default:
            LOGS_WARN("[Get RX KF] Unknown mode %d, returning 0", mode);
            *kf = 0;
            break;
    }

    return CR_OK;
}

CredoError_t condor_lp_set_rx_kf(CredoSlice_t* slice, int lane, int kf) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_set_rx_kf_nrz(slice, lane, kf));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_set_rx_kf_pam4(slice, lane, kf));
            break;
        case CR_LMODE_DISABLE:
            // do nothing
            break;
        default:
            LOGS_WARN("[Set RX KF] Unknown mode %d, setting NRZ KF", mode);
            ERR_PROPS(condor_lp_set_rx_kf_nrz(slice, lane, kf));
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_kf_nrz(CredoSlice_t* slice, int lane, int* kf) {
    return readRegLane(slice, lane, REG_RX_TOSM_KF_S6_NRZ, (unsigned*)kf);
}

CredoError_t condor_lp_set_rx_kf_nrz(CredoSlice_t* slice, int lane, int kf) {
    return writeRegLane(slice, lane, REG_RX_TOSM_KF_S6_NRZ, kf);
}

CredoError_t condor_lp_get_rx_kf_pam4(CredoSlice_t* slice, int lane, int* kf) {
    return readRegLane(slice, lane, REG_RX_READ_KF_Q, (unsigned*)kf);
}

CredoError_t condor_lp_set_rx_kf_pam4(CredoSlice_t* slice, int lane, int kf) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OWEN_KF_Q, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_OW_KF_Q, kf));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm) {
    CredoLaneMode_t mode;

    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            if (condor_lp_get_rx_ppm_nrz(slice, lane, ppm)) return CR_FAIL;
            break;
        case CR_LMODE_PAM4:
            if (condor_lp_get_rx_ppm_pam4(slice, lane, ppm)) return CR_FAIL;
            break;
        case CR_LMODE_DISABLE:
            *ppm = 0;
            break;
        default:
            LOGS_WARN("[Get RX PPM] Unknown mode %d, returning 0", mode);
            *ppm = 0;
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_ppm_nrz(CredoSlice_t* slice, int lane, int* ppm) {
    return readRegSignedLane(slice, lane, REG_RX_READ_FREQ_ACC_NRZ, ppm);
}

CredoError_t condor_lp_get_rx_ppm_pam4(CredoSlice_t* slice, int lane, int* ppm) {
    return readRegSignedLane(slice, lane, REG_RX_READ_FREQ_ACC, ppm);
}

CredoError_t condor_lp_set_rx_tia1_bias(CredoSlice_t* slice, int lane, unsigned tia_bias) {
#ifdef CONDOR_LP_1P2
    return writeRegLane(slice, lane, REG_RX_TIA1_BIAS, reverse_bits(tia_bias, 3));
#else
    return writeRegLane(slice, lane, REG_RX_TIA1_BIAS, tia_bias);
#endif
}

CredoError_t condor_lp_get_rx_tia1_bias(CredoSlice_t* slice, int lane, unsigned* tia_bias) {
#ifdef CONDOR_LP_1P2
    ERR_PROPS(readRegLane(slice, lane, REG_RX_TIA1_BIAS, tia_bias));
    *tia_bias = reverse_bits(*tia_bias, 3);
    return CR_OK;
#else
    return readRegLane(slice, lane, REG_RX_TIA1_BIAS, tia_bias);
#endif
}
