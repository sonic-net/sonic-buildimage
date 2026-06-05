#include "dii.h"

#include <stdlib.h>
#include <string.h>
// Top PLL

CredoError_t cr_serdes_cal_top_pll(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_top_pll_cal(slice, lane));
}

CredoError_t cr_serdes_init_top_pll(CredoSlice_t* slice) {
    LOGS_API();
    CALL_HAL(slice, hal_top_pll_init(slice));
}

CredoError_t cr_serdes_get_top_pll_cap(CredoSlice_t* slice, unsigned* cap) {
    LOGS_API();
    CALL_HAL(slice, hal_get_top_pll_cap(slice, cap));
}
// Capability

CredoError_t cr_serdes_get_rx_ffe_range(CredoSlice_t* slice, int lane, int* taps_len, int* sum_len) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_ffe_range(slice, lane, taps_len, sum_len));
}

CredoError_t cr_serdes_get_rx_ffe_weighting_table_range(CredoSlice_t* slice, int lane, int* row, int* col) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_ffe_weighting_table_range(slice, lane, row, col));
}

CredoError_t cr_serdes_get_tx_ffe_range(CredoSlice_t* slice, int lane, int* length, int* extended_length) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_ffe_range(slice, lane, length, extended_length));
}

CredoError_t cr_serdes_get_rx_dfe_range(CredoSlice_t* slice, int lane, int* length) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_dfe_range(slice, lane, length));
}

CredoError_t cr_serdes_get_rx_isi_range(CredoSlice_t* slice, int lane, int* length) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_isi_range(slice, lane, length));
}

// Polarity

CredoError_t cr_serdes_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, rx_pol %d", lane, rx_pol);
    CALL_HAL(slice, hal_set_rx_polarity(slice, lane, rx_pol));
}

CredoError_t cr_serdes_set_tx_polarity(CredoSlice_t* slice, int lane, int tx_pol) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, tx_pol %d", lane, tx_pol);
    CALL_HAL(slice, hal_set_tx_polarity(slice, lane, tx_pol));
}

CredoError_t cr_serdes_get_rx_polarity(CredoSlice_t* slice, int lane, int* rx_pol) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_polarity(slice, lane, rx_pol));
}

CredoError_t cr_serdes_get_tx_polarity(CredoSlice_t* slice, int lane, int* tx_pol) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_polarity(slice, lane, tx_pol));
}

// Input Mode

CredoError_t cr_serdes_set_rx_coupling(CredoSlice_t* slice, int lane, CredoLaneCoupling_t input_mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, input_mode %d", lane, input_mode);
    CALL_HAL(slice, hal_set_rx_input_mode(slice, lane, input_mode));
}

CredoError_t cr_serdes_get_rx_coupling(CredoSlice_t* slice, int lane, CredoLaneCoupling_t* input_mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_input_mode(slice, lane, input_mode));
}

// Gray & Pre Coding

CredoError_t cr_serdes_set_tx_gray_code(CredoSlice_t* slice, int lane, int tx_gc) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, tx_gc %d", lane, tx_gc);
    CALL_HAL(slice, hal_set_tx_gray_code(slice, lane, tx_gc));
}

CredoError_t cr_serdes_set_rx_gray_code(CredoSlice_t* slice, int lane, int rx_gc) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, rx_gc %d", lane, rx_gc);
    CALL_HAL(slice, hal_set_rx_gray_code(slice, lane, rx_gc));
}

CredoError_t cr_serdes_get_tx_gray_code(CredoSlice_t* slice, int lane, int* tx_gc) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_gray_code(slice, lane, tx_gc));
}

CredoError_t cr_serdes_get_rx_gray_code(CredoSlice_t* slice, int lane, int* rx_gc) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_gray_code(slice, lane, rx_gc));
}

CredoError_t cr_serdes_set_tx_precoder(CredoSlice_t* slice, int lane, int tx_pc) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, tx_pc %d", lane, tx_pc);
    CALL_HAL(slice, hal_set_tx_precoder(slice, lane, tx_pc));
}

CredoError_t cr_serdes_set_rx_precoder(CredoSlice_t* slice, int lane, int rx_pc) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, rx_pc %d", lane, rx_pc);
    CALL_HAL(slice, hal_set_rx_precoder(slice, lane, rx_pc));
}

CredoError_t cr_serdes_get_tx_precoder(CredoSlice_t* slice, int lane, int* tx_pc) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_precoder(slice, lane, tx_pc));
}

CredoError_t cr_serdes_get_rx_precoder(CredoSlice_t* slice, int lane, int* rx_pc) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_precoder(slice, lane, rx_pc));
}

// Bit swapping
CredoError_t cr_serdes_set_tx_msb(CredoSlice_t* slice, int lane, int tx_msb) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, tx_msb %d", lane, tx_msb);
    CALL_HAL(slice, hal_set_tx_msb(slice, lane, tx_msb));
}

CredoError_t cr_serdes_set_rx_msb(CredoSlice_t* slice, int lane, int rx_msb) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, rx_msb %d", lane, rx_msb);
    CALL_HAL(slice, hal_set_rx_msb(slice, lane, rx_msb));
}

CredoError_t cr_serdes_get_tx_msb(CredoSlice_t* slice, int lane, int* tx_msb) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_msb(slice, lane, tx_msb));
}

CredoError_t cr_serdes_get_rx_msb(CredoSlice_t* slice, int lane, int* rx_msb) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_msb(slice, lane, rx_msb));
}

// Lane PLL
CredoError_t cr_serdes_set_tx_cap(CredoSlice_t* slice, int lane, int tx_cap) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, tx_cap %d", lane, tx_cap);
    CALL_HAL(slice, hal_set_tx_cap(slice, lane, tx_cap));
}

CredoError_t cr_serdes_set_rx_cap(CredoSlice_t* slice, int lane, int rx_cap) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, rx_cap %d", lane, rx_cap);
    CALL_HAL(slice, hal_set_rx_cap(slice, lane, rx_cap));
}

CredoError_t cr_serdes_get_tx_cap(CredoSlice_t* slice, int lane, int* tx_cap) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_cap(slice, lane, tx_cap));
}

CredoError_t cr_serdes_get_rx_cap(CredoSlice_t* slice, int lane, int* rx_cap) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_cap(slice, lane, rx_cap));
}

// RX Detail
CredoError_t cr_serdes_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_ppm(slice, lane, ppm));
}

CredoError_t cr_serdes_get_rx_skef(CredoSlice_t* slice, int lane, int* enable, int* degen, int* addcap, int* gain) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_skef(slice, lane, enable, degen, addcap, gain));
}

CredoError_t cr_serdes_get_rx_dac(CredoSlice_t* slice, int lane, int* rx_dac) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_dac(slice, lane, rx_dac));
}

CredoError_t cr_serdes_get_rx_attenuator(CredoSlice_t* slice, int lane, int* passive, int* gain, int* termtune) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_attenuator(slice, lane, passive, gain, termtune));
}

CredoError_t cr_serdes_get_ffe_taps(CredoSlice_t* slice, int lane, int taps[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_ffe_taps(slice, lane, taps));
}

CredoError_t cr_serdes_get_ffe_taps_fine(CredoSlice_t* slice, int lane, int taps[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_ffe_taps_fine(slice, lane, taps));
}

CredoError_t cr_serdes_get_f1over3(CredoSlice_t* slice, int lane, int* value) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_f1over3(slice, lane, value));
}

CredoError_t cr_serdes_get_agcgain_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_agcgain_count(slice, lane, count));
}

CredoError_t cr_serdes_get_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_agcgain(slice, lane, agcgain));
}

CredoError_t cr_serdes_get_ctle_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_ctle_count(slice, lane, count));
}

CredoError_t cr_serdes_get_ctle(CredoSlice_t* slice, int lane, unsigned* value) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_ctle(slice, lane, value));
}

CredoError_t cr_serdes_get_delta_phase(CredoSlice_t* slice, int lane, int* value) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_delta_phase(slice, lane, value));
}

CredoError_t cr_serdes_get_edge(CredoSlice_t* slice, int lane, unsigned* value) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_edge(slice, lane, value));
}

CredoError_t cr_serdes_get_dfe(CredoSlice_t* slice, int lane, double dfe_taps[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    SLICE_LOCK_GUARD(slice);

    CredoError_t ret = CR_FAIL;
    unsigned status = 0;

    ret = hal_fw_get_status(slice, &status);
    if (ret != CR_OK) {
        SLICE_UNLOCK(slice);
        return ret;
    }

    if (status == 1) {
        ret = hal_fw_get_dfe(slice, lane, dfe_taps);
    }

    /* if firmware unsupport or fail, read from register */
    if (ret != CR_OK || status == 0) {
        ret = hal_get_dfe(slice, lane, dfe_taps);
    }

    SLICE_UNLOCK(slice);
    return ret;
}

CredoError_t cr_serdes_get_raw_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);

    SLICE_LOCK_GUARD(slice);

    CredoError_t ret = CR_FAIL;
    unsigned status = 0;

    ret = hal_fw_get_status(slice, &status);
    if (ret != CR_OK) {
        SLICE_UNLOCK(slice);
        return ret;
    }

    if (status == 1) {
        ret = hal_fw_get_eye(slice, lane, eyes);
    }

    /* if firmware unsupport or fail, read from register */
    if (ret != CR_OK || status == 0) {
        ret = hal_get_eye(slice, lane, eyes);
    }

    SLICE_UNLOCK(slice);
    return ret;
}

CredoError_t cr_serdes_get_rx_ready(CredoSlice_t* slice, int lane, int* ready) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_lane_ready(slice, lane, ready));
}

CredoError_t cr_serdes_get_rx_signal_detect(CredoSlice_t* slice, int lane, int* sd) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_signal_detect(slice, lane, sd));
}

// RX debugging

CredoError_t cr_serdes_set_rx_skef(CredoSlice_t* slice, int lane, int enable, int degen, int addcap, int gain) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d, degen %d, addcap %d, gain %d", lane, enable, degen, addcap, gain);
    CALL_HAL(slice, hal_set_rx_skef(slice, lane, enable, degen, addcap, gain));
}

CredoError_t cr_serdes_set_rx_dac(CredoSlice_t* slice, int lane, int rx_dac) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, rx_dac %d", lane, rx_dac);
    CALL_HAL(slice, hal_set_rx_dac(slice, lane, rx_dac));
}

CredoError_t cr_serdes_set_rx_attenuator(CredoSlice_t* slice, int lane, int passive, int gain, int termtune) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, passive %d, gain %d, termtune %d", lane, passive, gain, termtune);
    CALL_HAL(slice, hal_set_rx_attenuator(slice, lane, passive, gain, termtune));
}

CredoError_t cr_serdes_set_ffe_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_set_ffe_taps(slice, lane, taps));
}

CredoError_t cr_serdes_set_ffe_taps_fine(CredoSlice_t* slice, int lane, const int taps[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_set_ffe_taps_fine(slice, lane, taps));
}

CredoError_t cr_serdes_set_f1over3(CredoSlice_t* slice, int lane, int value) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, f1over3 %d", lane, value);
    CALL_HAL(slice, hal_set_f1over3(slice, lane, value));
}

CredoError_t cr_serdes_set_agcgain(CredoSlice_t* slice, int lane, unsigned value[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_set_agcgain(slice, lane, value));
}

CredoError_t cr_serdes_set_ctle(CredoSlice_t* slice, int lane, unsigned value[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_set_ctle(slice, lane, value));
}

CredoError_t cr_serdes_set_delta_phase(CredoSlice_t* slice, int lane, int value) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, delta phase %d", lane, value);
    CALL_HAL(slice, hal_set_delta_phase(slice, lane, value));
}

CredoError_t cr_serdes_set_edge(CredoSlice_t* slice, int lane, unsigned value) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, edge %d", lane, value);
    CALL_HAL(slice, hal_set_edge(slice, lane, value));
}

// TX Taps

CredoError_t cr_serdes_set_tx_taps_scale(CredoSlice_t* slice, int lane, const unsigned taps_scale[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_set_tx_taps_scale(slice, lane, taps_scale));
}

CredoError_t cr_serdes_get_tx_taps_scale(CredoSlice_t* slice, int lane, unsigned taps_scale[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_taps_scale(slice, lane, taps_scale));
}

CredoError_t cr_serdes_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_set_tx_taps(slice, lane, taps));
}

CredoError_t cr_serdes_preset_tx_taps(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_serdes_preset_tx_taps(slice, lane, desc));
}

CredoError_t cr_serdes_set_tx_taps_extended(CredoSlice_t* slice, int lane, const int taps_extended[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_set_tx_taps_extended(slice, lane, taps_extended));
}

CredoError_t cr_serdes_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_taps(slice, lane, taps));
}

CredoError_t cr_serdes_get_tx_taps_extended(CredoSlice_t* slice, int lane, int taps_extended[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_taps_extended(slice, lane, taps_extended));
}

CredoError_t cr_serdes_get_paramh(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data) {
    return cr_param_get_paramh(slice, PARAM_DOMAIN_SERDES, name, index, data);
}

CredoError_t cr_serdes_get_param(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data) {
    return cr_param_get_param(slice, PARAM_DOMAIN_SERDES, name, index, data);
}

CredoError_t cr_serdes_set_param(CredoSlice_t* slice, const char* name, int index, const CredoParamData_t* data) {
    return cr_param_set_param(slice, PARAM_DOMAIN_SERDES, name, index, data);
}

CredoError_t cri_serdes_reset_tx_taps(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_reset_tx_taps(slice, lane));
}

// SerDes Information

CredoError_t cr_serdes_get_all_phy_ready(CredoSlice_t* slice, unsigned* rdy) {
    LOGS_API();
    CALL_HAL(slice, hal_fw_phy_ready(slice, rdy));
}

CredoError_t cr_serdes_get_phy_ready(CredoSlice_t* slice, int lane, unsigned* rdy) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_phy_lane_ready(slice, lane, rdy));
}

CredoError_t cr_serdes_get_adapt_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_adapt_count(slice, lane, count));
}

CredoError_t cr_serdes_get_readapt_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_readapt_count(slice, lane, count));
}

CredoError_t cr_serdes_get_link_lost_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_link_lost_count(slice, lane, count));
}

CredoError_t cr_serdes_get_los_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_los_count(slice, lane, count));
}

CredoError_t cr_serdes_get_channel_estimate(CredoSlice_t* slice, int lane, double* chan_est) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_channel_estimate(slice, lane, chan_est));
}

CredoError_t cr_serdes_get_of(CredoSlice_t* slice, int lane, unsigned* of) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_of(slice, lane, of));
}

CredoError_t cr_serdes_get_hf(CredoSlice_t* slice, int lane, unsigned* hf) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_hf(slice, lane, hf));
}

CredoError_t cr_serdes_get_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_eye(slice, lane, eyes));
}

CredoError_t cr_serdes_get_isi(CredoSlice_t* slice, int lane, int isi[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_isi(slice, lane, isi));
}

CredoError_t cr_serdes_get_rx_ffe(CredoSlice_t* slice, int lane, int taps[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_rx_ffe(slice, lane, taps));
}

CredoError_t cr_serdes_get_rx_ffe_nbias(CredoSlice_t* slice, int lane, int nbias[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_rx_ffe_nbias(slice, lane, nbias));
}

CredoError_t cr_serdes_get_rx_ffe_kaccu(CredoSlice_t* slice, int lane, double kaccu[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_rx_ffe_kaccu(slice, lane, kaccu));
}

CredoError_t cr_serdes_get_rx_ffe_weighting_table(CredoSlice_t* slice, int lane, double** wt_table) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_rx_ffe_weighting_table(slice, lane, wt_table));
}

CredoError_t cr_serdes_get_rx_ffe_flip_counter(CredoSlice_t* slice, int lane, int flip_counter[]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_get_rx_ffe_flip_counter(slice, lane, flip_counter));
}
