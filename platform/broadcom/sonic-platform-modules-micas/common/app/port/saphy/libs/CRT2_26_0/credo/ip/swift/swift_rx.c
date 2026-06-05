/*
 * Swift RX information and debugging
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"
#include "swift/swift_serdes.h"

#include "utility.h"
#include "sdk.h"

#include <math.h>
#include <string.h>

#define SDT_AMP_TH 7
#define SDT_CNT_TH 0
#define SDT_N_SEL  1

CredoError_t swift_get_dtl_bb_en(CredoSlice_t* slice, int lane, bool* en) {
    ERR_PROPS(common_dsp_get_dtl_bb_en(slice, lane, en));
    return CR_OK;
}

CredoError_t swift_get_rx_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    ERR_PROP(common_dsp_get_rx_eye(slice, lane, eyes));
    return CR_OK;
}

CredoError_t swift_get_version_id(CredoSlice_t* slice, int lane, uint64_t* version) {
    ERR_PROP(common_dsp_get_version_id(slice, lane, version));
    return CR_OK;
}

CredoError_t swift_get_rx_sdt(CredoSlice_t* slice, int lane, int amp, int cnt, int n, unsigned* sdt_p, unsigned* sdt_n,
                              unsigned* cnt_p, unsigned* cnt_n) {
    ERR_PROP(common_dsp_get_rx_sdt(slice, lane, amp, cnt, n, sdt_p, sdt_n, cnt_p, cnt_n));
    return CR_OK;
}

CredoError_t swift_get_rx_signal_detect(CredoSlice_t* slice, int lane, int* sd) {
    ERR_PROP(common_dsp_get_rx_signal_detect(slice, lane, sd));
    return CR_OK;
}

CredoError_t swift_get_rx_envelope(CredoSlice_t* slice, int lane, int mode, unsigned amp[]) {
    ERR_PROP(common_dsp_get_rx_envelope(slice, lane, mode, amp));
    return CR_OK;
}

CredoError_t swift_get_rx_ctle_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (count == NULL) return CR_INVALID_ARGS;
    *count = 2;
    return CR_OK;
}

CredoError_t swift_set_rx_ctle(CredoSlice_t* slice, int lane, unsigned ctle[]) {
    ERR_PROP(common_dsp_set_rx_ctle(slice, lane, ctle));
    return CR_OK;
}

CredoError_t swift_get_rx_ctle(CredoSlice_t* slice, int lane, unsigned ctle[]) {
    ERR_PROP(common_dsp_get_rx_ctle(slice, lane, ctle));
    return CR_OK;
}

CredoError_t swift_set_rx_ctle_cs(CredoSlice_t* slice, int lane, unsigned ctle_cs[]) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_ADJ_CS1, ctle_cs[0]));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_ADJ_CS2, ctle_cs[1]));
    return CR_OK;
}

CredoError_t swift_get_rx_ctle_cs(CredoSlice_t* slice, int lane, unsigned ctle_cs[]) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_ADJ_CS1, ctle_cs + 0));
    ERR_PROP(readRegLane(slice, lane, REG_RX_ADJ_CS2, ctle_cs + 1));
    return CR_OK;
}

CredoError_t swift_get_rx_ctle_vs(CredoSlice_t* slice, int lane, unsigned* ctle_vs) {
    ERR_PROP(common_dsp_get_rx_ctle_vs(slice, lane, ctle_vs));
    return CR_OK;
}

CredoError_t swift_get_rx_ctle_ictrl(CredoSlice_t* slice, int lane, unsigned ctle_ictrl[]) {
    ERR_PROP(common_dsp_get_rx_ctle_ictrl(slice, lane, ctle_ictrl));
    return CR_OK;
}

CredoError_t swift_set_rx_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    ERR_PROP(common_dsp_set_rx_agcgain(slice, lane, agcgain));
    return CR_OK;
}

CredoError_t swift_get_rx_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    ERR_PROP(common_dsp_get_rx_agcgain(slice, lane, agcgain));
    return CR_OK;
}

CredoError_t swift_set_rx_ind(CredoSlice_t* slice, int lane, unsigned ind[]) {
    ERR_PROP(common_dsp_set_rx_ind(slice, lane, ind));
    return CR_OK;
}

CredoError_t swift_get_rx_ind(CredoSlice_t* slice, int lane, unsigned ind[]) {
    ERR_PROP(common_dsp_get_rx_ind(slice, lane, ind));
    return CR_OK;
}

CredoError_t swift_set_rx_skef_en(CredoSlice_t* slice, int lane, unsigned enable) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_EN_SKC, enable));
    return CR_OK;
}

CredoError_t swift_get_rx_skef_en(CredoSlice_t* slice, int lane, unsigned* enable) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_EN_SKC, enable));
    return CR_OK;
}

CredoError_t swift_set_rx_skef_degen(CredoSlice_t* slice, int lane, unsigned degen) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_SKC_DEGEN, degen));
    return CR_OK;
}

CredoError_t swift_get_rx_skef_degen(CredoSlice_t* slice, int lane, unsigned* degen) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_SKC_DEGEN, degen));
    return CR_OK;
}

CredoError_t swift_set_rx_skef_cap(CredoSlice_t* slice, int lane, unsigned cap) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CAP_SKC, cap));
    return CR_OK;
}

CredoError_t swift_get_rx_skef_cap(CredoSlice_t* slice, int lane, unsigned* cap) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_CAP_SKC, cap));
    return CR_OK;
}

// XXX, skef interface not compatible with old API, internal use only
CredoError_t swift_set_rx_skef(CredoSlice_t* slice, int lane, unsigned enable, unsigned degen, unsigned cap) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_EN_SKC, enable));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_SKC_DEGEN, degen));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_CAP_SKC, cap));
    return CR_OK;
}

CredoError_t swift_get_rx_skef(CredoSlice_t* slice, int lane, unsigned* enable, unsigned* degen, unsigned* cap) {
    ERR_PROP(readRegLane(slice, lane, REG_RX_EN_SKC, enable));
    ERR_PROP(readRegLane(slice, lane, REG_RX_SKC_DEGEN, degen));
    ERR_PROP(readRegLane(slice, lane, REG_RX_CAP_SKC, cap));
    return CR_OK;
}

CredoError_t swift_set_rx_dac(CredoSlice_t* slice, int lane, int rx_dac) {
    // ERR_PROP(writeRegLane(slice, lane, REG_RX_VADCSW, rx_dac));
    return CR_NOTIMPLEMENTED;
}

CredoError_t swift_get_rx_dac(CredoSlice_t* slice, int lane, int* rx_dac) {
    // ERR_PROP(readRegLane(slice, lane, REG_RX_VADCSW, (unsigned*)rx_dac));
    return CR_NOTIMPLEMENTED;
}

CredoError_t swift_set_rx_degen_dac(CredoSlice_t* slice, int lane, unsigned rx_degen_dac[]) {
    // ERR_PROPS(writeRegLane(slice, lane, REG_RX_DEGENDAC_S1, gray_bin(rx_degen_dac[0])));
    // ERR_PROPS(writeRegLane(slice, lane, REG_RX_DEGENDAC_S2, gray_bin(rx_degen_dac[1])));
    return CR_NOTIMPLEMENTED;
}

CredoError_t swift_get_rx_degen_dac(CredoSlice_t* slice, int lane, unsigned rx_degen_dac[]) {
    // ERR_PROPS(readRegLane(slice, lane, REG_RX_DEGENDAC_S1, rx_degen_dac + 0));
    // ERR_PROPS(readRegLane(slice, lane, REG_RX_DEGENDAC_S2, rx_degen_dac + 1));
    // rx_degen_dac[0] = bin_gray(rx_degen_dac[0]);
    // rx_degen_dac[1] = bin_gray(rx_degen_dac[1]);
    return CR_NOTIMPLEMENTED;
}

CredoError_t swift_get_rx_adc_ref_ctrl(CredoSlice_t* slice, int lane, unsigned* ref_ctrl) {
    ERR_PROP(common_dsp_get_rx_adc_ref_ctrl(slice, lane, ref_ctrl));
    return CR_OK;
}

/* It should not be used when firmware is running */
CredoError_t swift_get_rx_dfe_f0(CredoSlice_t* slice, int lane, unsigned* f0) {
    ERR_PROP(common_dsp_get_rx_dfe_f0(slice, lane, f0));
    return CR_OK;
}

CredoError_t swift_set_rx_dfe_f0(CredoSlice_t* slice, int lane, unsigned f0) {
    ERR_PROP(common_dsp_set_rx_dfe_f0(slice, lane, f0));
    return CR_OK;
}

CredoError_t swift_get_rx_dfe_f1(CredoSlice_t* slice, int lane, int* f1) {
    ERR_PROP(common_dsp_get_rx_dfe_f1(slice, lane, f1));
    return CR_OK;
}

CredoError_t swift_set_rx_dfe_f1(CredoSlice_t* slice, int lane, int f1) {
    ERR_PROP(common_dsp_set_rx_dfe_f1(slice, lane, f1));
    return CR_OK;
}

CredoError_t swift_get_rx_dfe(CredoSlice_t* slice, int lane, int dfe_taps[]) {
    ERR_PROP(common_dsp_get_rx_dfe(slice, lane, dfe_taps));
    return CR_OK;
}

CredoError_t swift_set_rx_dfe(CredoSlice_t* slice, int lane, int dfe_taps[]) {
    ERR_PROP(common_dsp_set_rx_dfe(slice, lane, dfe_taps));
    return CR_OK;
}

CredoError_t swift_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm) {
    ERR_PROP(common_dsp_get_rx_ppm(slice, lane, ppm));
    return CR_OK;
}

CredoError_t swift_set_rx_ppm(CredoSlice_t* slice, int lane, int ppm) {
    ERR_PROP(common_dsp_set_rx_ppm(slice, lane, ppm));
    return CR_OK;
}

CredoError_t swift_get_rx_agc_attn(CredoSlice_t* slice, int lane, unsigned* attn) {
    ERR_PROP(common_dsp_get_rx_agc_attn(slice, lane, attn));
    return CR_OK;
}

CredoError_t swift_set_rx_agc_attn(CredoSlice_t* slice, int lane, unsigned attn) {
    ERR_PROP(common_dsp_set_rx_agc_attn(slice, lane, attn));
    return CR_OK;
}

CredoError_t swift_get_rx_ffe_all(CredoSlice_t* slice, int lane, int taps[]) {
    ERR_PROP(common_dsp_get_rx_ffe_all(slice, lane, taps));
    return CR_OK;
}

CredoError_t swift_get_rx_ffe(CredoSlice_t* slice, int lane, int phase, int taps[]) {
    ERR_PROP(common_dsp_get_rx_ffe(slice, lane, phase, taps));
    return CR_OK;
}

CredoError_t swift_set_rx_ffe(CredoSlice_t* slice, int lane, const int taps[]) {
    ERR_PROP(common_dsp_set_rx_ffe(slice, lane, taps));
    return CR_OK;
}

CredoError_t swift_get_rx_ffe_cm1(CredoSlice_t* slice, int lane, int* cm1) {
    ERR_PROP(common_dsp_get_rx_ffe_cm1(slice, lane, cm1));
    return CR_OK;
}

CredoError_t swift_set_rx_ffe_cm1(CredoSlice_t* slice, int lane, int cm1) {
    ERR_PROP(common_dsp_set_rx_ffe_cm1(slice, lane, cm1));
    return CR_OK;
}

CredoError_t swift_get_rx_lane_ready(CredoSlice_t* slice, int lane, int* ready) {
    ERR_PROP(common_dsp_get_rx_lane_ready(slice, lane, ready));
    return CR_OK;
}

CredoError_t swift_get_rx_dfe_nonlinear_mode(CredoSlice_t* slice, int lane, int* en) {
    ERR_PROP(common_dsp_get_rx_dfe_nonlinear_mode(slice, lane, en));
    return CR_OK;
}

CredoError_t swift_set_rx_dfe_nonlinear_mode(CredoSlice_t* slice, int lane, int en) {
    ERR_PROP(common_dsp_set_rx_dfe_nonlinear_mode(slice, lane, en));
    return CR_OK;
}

CredoError_t swift_set_fec_clk(CredoSlice_t* slice, int lane, bool enable) {
    ERR_PROP(common_dsp_set_fec_clk(slice, lane, enable));
    return CR_OK;
}

CredoError_t swift_get_fec_clk(CredoSlice_t* slice, int lane, bool* enable) {
    ERR_PROP(common_dsp_get_fec_clk(slice, lane, enable));
    return CR_OK;
}

CredoError_t swift_set_lms_clk(CredoSlice_t* slice, int lane, bool enable) {
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, enable));
    return CR_OK;
}

CredoError_t swift_get_lms_clk(CredoSlice_t* slice, int lane, bool* enable) {
    ERR_PROP(common_dsp_get_lms_clk(slice, lane, enable));
    return CR_OK;
}

CredoError_t swift_get_rx_kp(CredoSlice_t* slice, int lane, unsigned* kp) {
    ERR_PROP(common_dsp_get_rx_kp(slice, lane, kp));
    return CR_OK;
}

CredoError_t swift_set_rx_kp(CredoSlice_t* slice, int lane, unsigned kp) {
    ERR_PROP(common_dsp_set_rx_kp(slice, lane, kp));
    return CR_OK;
}

CredoError_t swift_get_rx_kf(CredoSlice_t* slice, int lane, unsigned* kf) {
    ERR_PROP(common_dsp_get_rx_kf(slice, lane, kf));
    return CR_OK;
}

CredoError_t swift_set_rx_kf(CredoSlice_t* slice, int lane, unsigned kf) {
    ERR_PROP(common_dsp_set_rx_kf(slice, lane, kf));
    return CR_OK;
}

CredoError_t swift_get_rx_th_ud_ph_enable(CredoSlice_t* slice, int lane, unsigned* th_ud_ph_en) {
    ERR_PROP(common_dsp_get_rx_th_ud_ph_enable(slice, lane, th_ud_ph_en));
    return CR_OK;
}

CredoError_t swift_get_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]) {
    ERR_PROP(common_dsp_get_rx_flt_sel(slice, lane, flt_sel));
    return CR_OK;
}

CredoError_t swift_get_rx_flt_location_by_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[], unsigned flt_loc[]) {
    ERR_PROP(common_dsp_get_rx_flt_location_by_sel(slice, lane, flt_sel, flt_loc));
    return CR_OK;
}

CredoError_t swift_get_rx_flt_location(CredoSlice_t* slice, int lane, unsigned flt_loc[]) {
    ERR_PROP(common_dsp_get_rx_flt_location(slice, lane, flt_loc));
    return CR_OK;
}

CredoError_t swift_get_rx_thdly(CredoSlice_t* slice, int lane, unsigned thdly[]) {
    ERR_PROP(common_dsp_get_rx_thdly(slice, lane, thdly));
    return CR_OK;
}

CredoError_t swift_get_rx_th_ktheta(CredoSlice_t* slice, int lane, unsigned* ktheta) {
    ERR_PROP(common_dsp_get_rx_th_ktheta(slice, lane, ktheta));
    return CR_OK;
}

CredoError_t swift_get_rx_ktheta(CredoSlice_t* slice, int lane, unsigned kt[]) {
    ERR_PROP(common_dsp_get_rx_ktheta(slice, lane, kt));
    return CR_OK;
}

CredoError_t swift_get_rx_ktheta_flip(CredoSlice_t* slice, int lane, unsigned kflip[]) {
    ERR_PROP(common_dsp_get_rx_ktheta_flip(slice, lane, kflip));
    return CR_OK;
}

CredoError_t swift_get_rx_clk_comp_flip(CredoSlice_t* slice, int lane, unsigned* clk_comp_flip) {
    ERR_PROP(common_dsp_get_rx_clk_comp_flip(slice, lane, clk_comp_flip));
    return CR_OK;
}

CredoError_t swift_get_rx_thdly_acc_in_sel(CredoSlice_t* slice, int lane, unsigned* thdly_acc_in_sel) {
    ERR_PROP(common_dsp_get_rx_thdly_acc_in_sel(slice, lane, thdly_acc_in_sel));
    return CR_OK;
}

CredoError_t swift_get_rx_phase_fast_rotate(CredoSlice_t* slice, int lane, unsigned* ph_fast_rotate) {
    ERR_PROP(common_dsp_get_rx_phase_fast_rotate(slice, lane, ph_fast_rotate));
    return CR_OK;
}

CredoError_t swift_get_rx_ted_slope_decision(CredoSlice_t* slice, int lane, unsigned* ted_slope_decision) {
    ERR_PROP(common_dsp_get_rx_ted_slope_decision(slice, lane, ted_slope_decision));
    return CR_OK;
}

CredoError_t swift_get_rx_delayloop_freeze(CredoSlice_t* slice, int lane, unsigned* delayloop_freeze) {
    ERR_PROP(common_dsp_get_rx_delayloop_freeze(slice, lane, delayloop_freeze));
    return CR_OK;
}

CredoError_t swift_get_rx_dtl_phase0(CredoSlice_t* slice, int lane, unsigned* dtl_phase0) {
    ERR_PROP(common_dsp_get_rx_dtl_phase0(slice, lane, dtl_phase0));
    return CR_OK;
}

CredoError_t swift_set_rx_dtl_phase0(CredoSlice_t* slice, int lane, unsigned dtl_phase0) {
    ERR_PROP(common_dsp_set_rx_dtl_phase0(slice, lane, dtl_phase0));
    return CR_OK;
}

CredoError_t swift_get_rx_dtl_theta(CredoSlice_t* slice, int lane, int dtl_theta[]) {
    ERR_PROP(common_dsp_get_rx_dtl_theta(slice, lane, dtl_theta));
    return CR_OK;
}

CredoError_t swift_get_rx_vga(CredoSlice_t* slice, int lane, unsigned* vga) {
    ERR_PROP(common_dsp_get_rx_vga(slice, lane, vga));
    return CR_OK;
}

CredoError_t swift_set_rx_vga(CredoSlice_t* slice, int lane, unsigned vga) {
    ERR_PROP(common_dsp_set_rx_vga(slice, lane, vga));
    return CR_OK;
}

CredoError_t swift_get_rx_vga_mode(CredoSlice_t* slice, int lane, unsigned vga_mode[]) {
    ERR_PROP(common_dsp_get_rx_vga_mode(slice, lane, vga_mode));
    return CR_OK;
}

CredoError_t swift_get_rx_dc_cmn(CredoSlice_t* slice, int lane, int* dc_cmn) {
    ERR_PROP(common_dsp_get_rx_dc_cmn(slice, lane, dc_cmn));
    return CR_OK;
}

CredoError_t swift_get_rx_dc_sar(CredoSlice_t* slice, int lane, int* dc_sar) {
    ERR_PROP(common_dsp_get_rx_dc_sar(slice, lane, dc_sar));
    return CR_OK;
}

CredoError_t swift_get_rx_gain_sar(CredoSlice_t* slice, int lane, unsigned* gain_sar) {
    ERR_PROP(common_dsp_get_rx_gain_sar(slice, lane, gain_sar));
    return CR_OK;
}

CredoError_t swift_get_rx_halfrate_en(CredoSlice_t* slice, int lane, bool* en) {
    ERR_PROP(common_dsp_get_rx_halfrate_en(slice, lane, en));
    return CR_OK;
}

// before calling this function, it needs freeze firmware ans save current lms clock status
CredoError_t swift_get_rx_dsp_config(CredoSlice_t* slice, int lane, SwiftDspConfig_t* dsp_config) {
    bool lms_clk_orig = false;
    ERR_PROP(common_dsp_get_lms_clk(slice, lane, &lms_clk_orig));

    ERR_PROP(common_dsp_get_rx_vga_mode(slice, lane, dsp_config->vga_mode));

    ERR_PROP(common_dsp_set_lms_clk(slice, lane, false));
    ERR_PROP(readRegLane(slice, lane, REG_RX_DC_ADAPT_CMN_EN, &dsp_config->dc_adapt_cmn_en));
    ERR_PROP(readRegLane(slice, lane, REG_RX_DC_ADAPT_SAR_EN, &dsp_config->dc_adapt_sar_en));
    ERR_PROP(readRegLane(slice, lane, REG_RX_GAIN_ADAPT_CMN_EN, &dsp_config->gain_adapt_cmn_en));
    ERR_PROP(readRegLane(slice, lane, REG_RX_GAIN_ADAPT_EN, &dsp_config->gain_adapt_en));
    ERR_PROP(readRegLane(slice, lane, REG_RX_VGA_ADAPT_EN, &dsp_config->vga_adapt_en));
    ERR_PROP(readRegLane(slice, lane, REG_RX_DFE_ADAPT_F0_EN, &dsp_config->dfe_adapt_f0_en));
    ERR_PROP(readRegLane(slice, lane, REG_RX_DFE_ADAPT_F1_EN, &dsp_config->dfe_adapt_f1_en));
    ERR_PROP(readRegLane(slice, lane, REG_RX_FFE_ADAPT_PRE_EN, &dsp_config->ffe_adapt_pre_en));
    ERR_PROP(readRegLane(slice, lane, REG_RX_FFE_ADAPT_PST_EN, &dsp_config->ffe_adapt_pst_en));
    ERR_PROP(readRegLane(slice, lane, REG_RX_FFE_ADAPT_FLT_EN, &dsp_config->ffe_adapt_flt_en));

    ERR_PROP(readRegLane(slice, lane, REG_RX_DC_ADAPT_CMN_MU, &dsp_config->dc_adapt_cmn_mu));
    ERR_PROP(readRegLane(slice, lane, REG_RX_DC_ADAPT_SAR_MU, &dsp_config->dc_adapt_sar_mu));
    ERR_PROP(readRegLane(slice, lane, REG_RX_GAIN_ADAPT_MU, &dsp_config->gain_adapt_mu));
    ERR_PROP(readRegLane(slice, lane, REG_RX_VGA_ADAPT_MU, &dsp_config->vga_adapt_mu));
    ERR_PROP(readRegLane(slice, lane, REG_RX_DFE_ADAPT_F0_MU, &dsp_config->dfe_adapt_f0_mu));
    ERR_PROP(readRegLane(slice, lane, REG_RX_DFE_ADAPT_F1_MU, &dsp_config->dfe_adapt_f1_mu));
    ERR_PROP(readRegLane(slice, lane, REG_RX_FFE_ADAPT_PRE1_MU, &dsp_config->ffe_adapt_pre1_mu));
    ERR_PROP(readRegLane(slice, lane, REG_RX_FFE_ADAPT_MU, &dsp_config->ffe_adapt_mu));

    ERR_PROP(readRegLane(slice, lane, REG_RX_DC_ADAPT_IN_SEL, &dsp_config->dc_adapt_in_sel));
    ERR_PROP(readRegLane(slice, lane, REG_RX_GAIN_ADAPT_PATH_PERIOD, &dsp_config->gain_adapt_path_period));
    ERR_PROP(readRegLane(slice, lane, REG_RX_GAIN_ADAPT_PATH_ANCHOR, &dsp_config->gain_adapt_path_anchor));

    ERR_PROP(readRegLane(slice, lane, REG_RX_FFE_ADAPT_PHASE_PERIOD, &dsp_config->ffe_adapt_phase_period));
    ERR_PROP(readRegLane(slice, lane, REG_RX_FFE_ADAPT_PH_CMN, &dsp_config->ffe_adapt_phase_cmn));
    ERR_PROP(common_dsp_set_lms_clk(slice, lane, lms_clk_orig));

    return CR_OK;
}

CredoError_t swift_get_rx_ictrl(CredoSlice_t* slice, int lane, unsigned ictrl[2]) {
    ERR_PROP(common_dsp_get_rx_ictrl(slice, lane, ictrl));
    return CR_OK;
}
