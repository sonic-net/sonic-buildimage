#ifndef COMMON_DSP_FUNCTIONS_H
#define COMMON_DSP_FUNCTIONS_H

#include "sdk.h"

#define DSP_EYE_PAM4_COUNT 14
#define DSP_EYE_NRZ_COUNT  2
#define DSP_RX_FFE_COUNT   20
#define DSP_AGCGAIN_COUNT  4
#define DSP_ISI_COUNT      67  // 8 + 58 + 1
#define DSP_FLT_COUNT      8
#define DSP_DFE_COUNT      2
#define DSP_SAR_COUNT      64

#define DSP_SYS_CDFL_CW 11
#define DSP_SYS_CDFL_CF 7
#define DSP_SYS_F1_CF   5
#define DSP_CDFL_TH_MAX 1023
#define DSP_CDFL_TH_MIN -1023

/* Serdes interface */
CredoError_t common_dsp_set_tx_gray_code(CredoSlice_t*, int lane, int tx_gc);
CredoError_t common_dsp_get_tx_gray_code(CredoSlice_t*, int lane, int* tx_gc);
CredoError_t common_dsp_set_rx_gray_code(CredoSlice_t*, int lane, int rx_gc);
CredoError_t common_dsp_get_rx_gray_code(CredoSlice_t*, int lane, int* rx_gc);
CredoError_t common_dsp_set_tx_precoder(CredoSlice_t*, int lane, int tx_pc);
CredoError_t common_dsp_get_tx_precoder(CredoSlice_t*, int lane, int* tx_pc);
CredoError_t common_dsp_set_rx_precoder(CredoSlice_t*, int lane, int rx_pc);
CredoError_t common_dsp_get_rx_precoder(CredoSlice_t*, int lane, int* rx_pc);
CredoError_t common_dsp_set_tx_msb(CredoSlice_t*, int lane, int tx_msb);
CredoError_t common_dsp_get_tx_msb(CredoSlice_t*, int lane, int* tx_msb);
CredoError_t common_dsp_set_rx_msb(CredoSlice_t*, int lane, int rx_msb);
CredoError_t common_dsp_get_rx_msb(CredoSlice_t*, int lane, int* rx_msb);

/* Serdes environment */
CredoError_t common_dsp_set_tx_polarity(CredoSlice_t*, int lane, int tx_pol);
CredoError_t common_dsp_set_rx_polarity(CredoSlice_t*, int lane, int rx_pol);
CredoError_t common_dsp_get_tx_polarity(CredoSlice_t*, int lane, int* tx_pol);
CredoError_t common_dsp_get_rx_polarity(CredoSlice_t*, int lane, int* rx_pol);
CredoError_t common_dsp_set_rx_input_mode(CredoSlice_t*, int lane, CredoLaneCoupling_t input_mode);
CredoError_t common_dsp_get_rx_input_mode(CredoSlice_t*, int lane, CredoLaneCoupling_t* input_mode);

/* Per lane PLL */
CredoError_t common_dsp_set_tx_cap(CredoSlice_t*, int lane, int tx_cap);
CredoError_t common_dsp_get_tx_cap(CredoSlice_t*, int lane, int* tx_cap);
CredoError_t common_dsp_set_rx_cap(CredoSlice_t*, int lane, int rx_cap);
CredoError_t common_dsp_get_rx_cap(CredoSlice_t*, int lane, int* rx_cap);
CredoError_t common_dsp_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);

/* RX */
CredoError_t common_dsp_get_dtl_bb_en(CredoSlice_t* slice, int lane, bool* en);
CredoError_t common_dsp_get_version_id(CredoSlice_t* slice, int lane, uint64_t* version);
CredoError_t common_dsp_get_rx_sdt(CredoSlice_t* slice, int lane, int amp, int cnt, int n, unsigned* sdt_p,
                                   unsigned* sdt_n, unsigned* cnt_p, unsigned* cnt_n);
CredoError_t common_dsp_get_rx_signal_detect(CredoSlice_t*, int lane, int* sd);
CredoError_t common_dsp_get_rx_envelope(CredoSlice_t* slice, int lane, int mode, unsigned amp[]);
CredoError_t common_dsp_get_rx_lane_ready(CredoSlice_t* slice, int lane, int* ready);
CredoError_t common_dsp_get_rx_cdfl_eye(CredoSlice_t* slice, int lane, int eyes[]);
CredoError_t common_dsp_get_rx_eye(CredoSlice_t* slice, int lane, int eyes[3]);
CredoError_t common_dsp_get_rx_ctle(CredoSlice_t*, int lane, unsigned ctle[]);
CredoError_t common_dsp_get_rx_ctle_vs(CredoSlice_t* slice, int lane, unsigned* ctle_vs);
CredoError_t common_dsp_get_rx_ctle_ictrl(CredoSlice_t* slice, int lane, unsigned ctle_ictrl[]);
CredoError_t common_dsp_get_rx_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]);
CredoError_t common_dsp_get_rx_ind(CredoSlice_t* slice, int lane, unsigned ind[]);
CredoError_t common_dsp_set_rx_ind(CredoSlice_t* slice, int lane, unsigned ind[]);
CredoError_t common_dsp_get_rx_ffe_all(CredoSlice_t* slice, int lane, int taps[]);
CredoError_t common_dsp_get_rx_ffe(CredoSlice_t* slice, int lane, int phase, int taps[]);
CredoError_t common_dsp_get_rx_ffe_cm1(CredoSlice_t* slice, int lane, int* cm1);
CredoError_t common_dsp_get_rx_skef_en(CredoSlice_t* slice, int lane, unsigned* enable);
CredoError_t common_dsp_get_rx_skef_degen(CredoSlice_t* slice, int lane, unsigned* degen);
CredoError_t common_dsp_get_rx_skef_cap(CredoSlice_t* slice, int lane, unsigned* cap);
CredoError_t common_dsp_get_rx_adc_ref_ctrl(CredoSlice_t* slice, int lane, unsigned* ref_ctrl);
CredoError_t common_dsp_get_rx_dfe_nonlinear_mode(CredoSlice_t* slice, int lane, int* en);
CredoError_t common_dsp_get_rx_dfe_f0(CredoSlice_t* slice, int lane, unsigned* f0);
CredoError_t common_dsp_set_rx_dfe_f0(CredoSlice_t* slice, int lane, unsigned f0);
CredoError_t common_dsp_get_rx_dfe_f1(CredoSlice_t* slice, int lane, int* f1);
CredoError_t common_dsp_set_rx_dfe_f1(CredoSlice_t* slice, int lane, int f1);
CredoError_t common_dsp_get_rx_dfe(CredoSlice_t* slice, int lane, int dfe_taps[]);
CredoError_t common_dsp_set_rx_dfe(CredoSlice_t* slice, int lane, int dfe_taps[]);
CredoError_t common_dsp_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm);
CredoError_t common_dsp_get_rx_agc_attn(CredoSlice_t* slice, int lane, unsigned* attn);
CredoError_t common_dsp_set_fec_clk(CredoSlice_t* slice, int lane, bool enable);
CredoError_t common_dsp_get_fec_clk(CredoSlice_t* slice, int lane, bool* enable);
CredoError_t common_dsp_set_lms_clk(CredoSlice_t* slice, int lane, bool enable);
CredoError_t common_dsp_get_lms_clk(CredoSlice_t* slice, int lane, bool* enable);
CredoError_t common_dsp_get_rx_kp(CredoSlice_t* slice, int lane, unsigned* kp);
CredoError_t common_dsp_get_rx_kf(CredoSlice_t* slice, int lane, unsigned* kf);
CredoError_t common_dsp_get_rx_th_ud_ph_enable(CredoSlice_t* slice, int lane, unsigned* th_ud_ph_en);
CredoError_t common_dsp_get_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]);
CredoError_t common_dsp_get_rx_flt_location(CredoSlice_t* slice, int lane, unsigned flt_loc[]);
CredoError_t common_dsp_get_rx_flt_location_by_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[],
                                                   unsigned flt_loc[]);
CredoError_t common_dsp_get_rx_thdly(CredoSlice_t* slice, int lane, unsigned thdly[]);
CredoError_t common_dsp_get_rx_th_ktheta(CredoSlice_t* slice, int lane, unsigned* ktheta);
CredoError_t common_dsp_get_rx_ktheta(CredoSlice_t* slice, int lane, unsigned kt[]);
CredoError_t common_dsp_get_rx_ktheta_flip(CredoSlice_t* slice, int lane, unsigned kflip[]);
CredoError_t common_dsp_get_rx_clk_comp_flip(CredoSlice_t* slice, int lane, unsigned* clk_comp_flip);
CredoError_t common_dsp_get_rx_thdly_acc_in_sel(CredoSlice_t* slice, int lane, unsigned* thdly_acc_in_sel);
CredoError_t common_dsp_get_rx_phase_fast_rotate(CredoSlice_t* slice, int lane, unsigned* ph_fast_rotate);
CredoError_t common_dsp_get_rx_ted_slope_decision(CredoSlice_t* slice, int lane, unsigned* ted_slope_decision);
CredoError_t common_dsp_get_rx_delayloop_freeze(CredoSlice_t* slice, int lane, unsigned* delayloop_freeze);
CredoError_t common_dsp_set_rx_dtl_phase0(CredoSlice_t* slice, int lane, unsigned dtl_phase0);
CredoError_t common_dsp_get_rx_dtl_phase0(CredoSlice_t* slice, int lane, unsigned* dtl_phase0);
CredoError_t common_dsp_get_rx_dtl_theta(CredoSlice_t* slice, int lane, int dtl_theta[]);
CredoError_t common_dsp_get_rx_halfrate_en(CredoSlice_t* slice, int lane, bool* en);
CredoError_t common_dsp_get_rx_vga(CredoSlice_t* slice, int lane, unsigned* vga_coe);
CredoError_t common_dsp_set_rx_vga(CredoSlice_t* slice, int lane, unsigned vga_coe);
CredoError_t common_dsp_get_rx_vga_mode(CredoSlice_t* slice, int lane, unsigned* vga_mode);
CredoError_t common_dsp_get_rx_dc_cmn(CredoSlice_t* slice, int lane, int* dc_cmn);
CredoError_t common_dsp_get_rx_dc_sar(CredoSlice_t* slice, int lane, int* dc_sar);
CredoError_t common_dsp_get_rx_gain_sar(CredoSlice_t* slice, int lane, unsigned* gain_sar);
CredoError_t common_dsp_get_rx_ictrl(CredoSlice_t* slice, int lane, unsigned ictrl[2]);

/* RX debug */
CredoError_t common_dsp_set_rx_ffe(CredoSlice_t* slice, int lane, const int taps[]);
CredoError_t common_dsp_set_rx_ffe_cm1(CredoSlice_t* slice, int lane, int cm1);
CredoError_t common_dsp_set_rx_dfe_nonlinear_mode(CredoSlice_t* slice, int lane, int en);
CredoError_t common_dsp_set_rx_ppm(CredoSlice_t* slice, int lane, int ppm);
CredoError_t common_dsp_set_rx_ctle(CredoSlice_t*, int lane, unsigned ctle[]);
CredoError_t common_dsp_set_rx_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]);
CredoError_t common_dsp_set_rx_agc_attn(CredoSlice_t* slice, int lane, unsigned attn);
CredoError_t common_dsp_set_rx_skef_en(CredoSlice_t* slice, int lane, unsigned enable);
CredoError_t common_dsp_set_rx_skef_degen(CredoSlice_t* slice, int lane, unsigned degen);
CredoError_t common_dsp_set_rx_skef_cap(CredoSlice_t* slice, int lane, unsigned cap);
CredoError_t common_dsp_set_rx_kp(CredoSlice_t* slice, int lane, unsigned kp);
CredoError_t common_dsp_set_rx_kf(CredoSlice_t* slice, int lane, unsigned kf);

/* TX taps */
CredoError_t common_dsp_get_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);
CredoError_t common_dsp_set_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode);
CredoError_t common_dsp_set_tx_taps(CredoSlice_t*, int lane, const int taps[]);
CredoError_t common_dsp_get_tx_taps(CredoSlice_t*, int lane, int taps[]);
CredoError_t common_dsp_set_tx_taps_extended(CredoSlice_t* slice, int lane, const int taps_extended[]);
CredoError_t common_dsp_get_tx_taps_extended(CredoSlice_t*, int lane, int taps_extended[]);
CredoError_t common_dsp_reset_tx_taps(CredoSlice_t*, int lane);

/* PRBS */
#define DSP_PRBS_INVALID 0xF
CredoLanePrbsPattern_t common_dsp_rx_prbs_type_hw_to_credo(unsigned prbs_mode);
unsigned common_dsp_rx_prbs_type_credo_to_hw(CredoLanePrbsPattern_t prbs_mode);
CredoLanePrbsPattern_t common_dsp_tx_prbs_type_hw_to_credo(unsigned tx_prbs_mode, CredoLaneMode_t mode);
unsigned common_dsp_tx_prbs_type_credo_to_hw(CredoLanePrbsPattern_t prbs_mode, CredoLaneMode_t mode);
CredoError_t common_dsp_set_tx_prbs_mode(CredoSlice_t* slice, int lane, CredoLanePrbsPattern_t prbs_mode);
CredoError_t common_dsp_get_tx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode);
CredoError_t common_dsp_get_rx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode);
CredoError_t common_dsp_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t common_dsp_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t common_dsp_set_rx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t common_dsp_set_rx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t common_dsp_set_tx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t common_dsp_set_tx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t common_dsp_get_rx_prbs_error_count(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t common_dsp_rx_prbs_rst_nrz(CredoSlice_t* slice, int lane);
CredoError_t common_dsp_rx_prbs_rst_pam4(CredoSlice_t* slice, int lane);
CredoError_t common_dsp_rx_prbs_rst(CredoSlice_t* slice, int lane);
CredoError_t common_dsp_get_rx_prbs_lock(CredoSlice_t* slice, int lane, CredoPrbsLockStatus_t* lock);
CredoError_t common_dsp_prbs_gen_1error(CredoSlice_t* slice, int lane);
CredoError_t common_dsp_prbs_gen_1error_nrz(CredoSlice_t* slice, int lane);
CredoError_t common_dsp_prbs_gen_1error_pam4(CredoSlice_t* slice, int lane);
CredoError_t common_dsp_set_rx_prbs_prev(CredoSlice_t* slice, int lane, bool en, unsigned prev);
CredoError_t common_dsp_set_rx_prbs_pattern_phase(CredoSlice_t* slice, int lane, unsigned phase);
CredoError_t common_dsp_get_rx_prbs_pattern_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]);
CredoError_t common_dsp_reset_rx_prbs_pattern_count(CredoSlice_t* slice, int lane);
CredoError_t common_dsp_prbs_get_rx_autosync(CredoSlice_t* slice, int lane, bool* enabled);

/* TX Test Pattern */
CredoError_t common_dsp_get_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool* enable);
CredoError_t common_dsp_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable);
CredoError_t common_dsp_get_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode* mode);
CredoError_t common_dsp_set_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode mode);
CredoError_t common_dsp_get_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t* pattern);
CredoError_t common_dsp_set_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t pattern);

#endif
