#ifndef CONDOR_LP_SERDES_H
#define CONDOR_LP_SERDES_H

#include "sdk.h"

/* Condor LP capability */
CredoError_t condor_lp_get_rx_ffe_range(CredoSlice_t*, int lane, int* taps_len, int* sum_len);
CredoError_t condor_lp_get_rx_ffe_weighting_table_range(CredoSlice_t*, int lane, int* row, int* col);
CredoError_t condor_lp_get_tx_ffe_range(CredoSlice_t*, int lane, int* length, int* extended_length);
CredoError_t condor_lp_get_rx_dfe_range(CredoSlice_t*, int lane, int* length);
CredoError_t condor_lp_get_rx_isi_range(CredoSlice_t*, int lane, int* length);

/* Condor LP serdes interface */
CredoError_t condor_lp_set_tx_gray_code(CredoSlice_t*, int lane, int tx_gc);
CredoError_t condor_lp_get_tx_gray_code(CredoSlice_t*, int lane, int* tx_gc);
CredoError_t condor_lp_set_rx_gray_code(CredoSlice_t*, int lane, int rx_gc);
CredoError_t condor_lp_get_rx_gray_code(CredoSlice_t*, int lane, int* rx_gc);
CredoError_t condor_lp_set_tx_precoder(CredoSlice_t*, int lane, int tx_pc);
CredoError_t condor_lp_get_tx_precoder(CredoSlice_t*, int lane, int* tx_pc);
CredoError_t condor_lp_set_rx_precoder(CredoSlice_t*, int lane, int rx_pc);
CredoError_t condor_lp_get_rx_precoder(CredoSlice_t*, int lane, int* rx_pc);

CredoError_t condor_lp_set_tx_msb(CredoSlice_t*, int lane, int tx_msb);
CredoError_t condor_lp_get_tx_msb(CredoSlice_t*, int lane, int* tx_msb);
CredoError_t condor_lp_set_rx_msb(CredoSlice_t*, int lane, int rx_msb);
CredoError_t condor_lp_get_rx_msb(CredoSlice_t*, int lane, int* rx_msb);

/* Condor LP serdes environment */
CredoError_t condor_lp_set_tx_polarity(CredoSlice_t*, int lane, int tx_pol);
CredoError_t condor_lp_get_tx_polarity(CredoSlice_t*, int lane, int* tx_pol);
CredoError_t condor_lp_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol);
CredoError_t condor_lp_set_rx_polarity_nrz(CredoSlice_t* slice, int lane, int rx_pol);
CredoError_t condor_lp_set_rx_polarity_pam4(CredoSlice_t* slice, int lane, int rx_pol);
CredoError_t condor_lp_get_rx_polarity(CredoSlice_t*, int lane, int* rx_pol);
CredoError_t condor_lp_get_rx_polarity_nrz(CredoSlice_t* slice, int lane, int* rx_pol);
CredoError_t condor_lp_get_rx_polarity_pam4(CredoSlice_t* slice, int lane, int* rx_pol);
CredoError_t condor_lp_set_rx_input_mode(CredoSlice_t*, int lane, CredoLaneCoupling_t input_mode);
CredoError_t condor_lp_get_rx_input_mode(CredoSlice_t*, int lane, CredoLaneCoupling_t* input_mode);

/* Condor LP per-lane PLL */
CredoError_t condor_lp_set_tx_cap(CredoSlice_t*, int lane, int tx_cap);
CredoError_t condor_lp_set_rx_cap(CredoSlice_t*, int lane, int rx_cap);
CredoError_t condor_lp_get_tx_cap(CredoSlice_t*, int lane, int* tx_cap);
CredoError_t condor_lp_get_rx_cap(CredoSlice_t*, int lane, int* rx_cap);

CredoError_t condor_lp_set_tx_pll(CredoSlice_t* slice, int lane, int tx_pll, int vcocap);
CredoError_t condor_lp_set_rx_pll(CredoSlice_t* slice, int lane, int rx_pll, int vcocap);
CredoError_t condor_lp_get_tx_pll(CredoSlice_t* slice, int lane, int* tx_pll, int* vcocap);
CredoError_t condor_lp_get_rx_pll(CredoSlice_t* slice, int lane, int* rx_pll, int* vcocap);
CredoError_t condor_lp_set_tx_pll_frac(CredoSlice_t* slice, int lane, int pll_n, int smen, int sdm_order, int mmdiv);
CredoError_t condor_lp_get_tx_pll_frac(CredoSlice_t* slice, int lane, int* pll_n, int* smen, int* sdm_order,
                                       int* mmdiv);
CredoError_t condor_lp_set_rx_pll_frac(CredoSlice_t* slice, int lane, int pll_n, int smen, int sdm_order, int mmdiv);
CredoError_t condor_lp_get_rx_pll_frac(CredoSlice_t* slice, int lane, int* pll_n, int* smen, int* sdm_order,
                                       int* mmdiv);

/* Condor LP RX */
CredoError_t condor_lp_get_version_id(CredoSlice_t* slice, int lane, uint64_t* version);
CredoError_t condor_lp_get_rx_skef(CredoSlice_t* slice, int lane, int* enable, int* degen, int* addcap, int* skef_gain);
CredoError_t condor_lp_get_rx_skef_enable(CredoSlice_t* slice, int lane, int* skef_en);
CredoError_t condor_lp_get_rx_skef_degen(CredoSlice_t* slice, int lane, int* skef_degen);
CredoError_t condor_lp_get_rx_skef_addcap(CredoSlice_t* slice, int lane, int* skef_addcap);
CredoError_t condor_lp_get_rx_skef_gain(CredoSlice_t* slice, int lane, int* skef_gain);
CredoError_t condor_lp_get_rx_dac(CredoSlice_t* slice, int lane, int* rx_dac);
CredoError_t condor_lp_get_rx_dac_nrz(CredoSlice_t* slice, int lane, int* rx_dac);
CredoError_t condor_lp_get_rx_dac_pam4(CredoSlice_t* slice, int lane, int* rx_dac);
CredoError_t condor_lp_get_f1over3(CredoSlice_t* slice, int lane, int* value);
CredoError_t condor_lp_get_rx_delta(CredoSlice_t* slice, int lane, int* delta);
CredoError_t condor_lp_get_rx_agcgain_count(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t condor_lp_get_agcgain(CredoSlice_t*, int lane, unsigned agcgain[]);
CredoError_t condor_lp_get_rx_ctle_count(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t condor_lp_get_rx_ctle(CredoSlice_t*, int lane, unsigned ctle[]);
CredoError_t condor_lp_get_edge(CredoSlice_t*, int lane, unsigned* value);
CredoError_t condor_lp_get_rx_ths(CredoSlice_t* slice, int lane, int ths[]);
CredoError_t condor_lp_get_rx_dfe(CredoSlice_t* slice, int lane, double dfe_taps[]);
CredoError_t condor_lp_get_rx_dfe_nrz(CredoSlice_t* slice, int lane, double dfe_taps[]);
CredoError_t condor_lp_get_rx_dfe_pam4(CredoSlice_t* slice, int lane, double dfe_taps[]);
CredoError_t condor_lp_get_rx_eye(CredoSlice_t* slice, int lane, int eyes[3]);
CredoError_t condor_lp_get_rx_eye_nrz(CredoSlice_t* slice, int lane, int* eye);
CredoError_t condor_lp_get_rx_eye_pam4(CredoSlice_t* slice, int lane, int eyes[3]);
CredoError_t condor_lp_get_rx_lane_ready(CredoSlice_t*, int lane, int* ready);
CredoError_t condor_lp_get_rx_lane_ready_nrz(CredoSlice_t*, int lane, int* ready);
CredoError_t condor_lp_get_rx_lane_ready_pam4(CredoSlice_t*, int lane, int* ready);
CredoError_t condor_lp_get_rx_sd(CredoSlice_t*, int lane, int* sd);
CredoError_t condor_lp_get_rx_sd_nrz(CredoSlice_t*, int lane, int* sd);
CredoError_t condor_lp_get_rx_sd_pam4(CredoSlice_t*, int lane, int* sd);
CredoError_t condor_lp_updn_control(CredoSlice_t* slice, int lane, int en, int theta[], int flip);
CredoError_t condor_lp_set_rx_updown_mode(CredoSlice_t* slice, int lane, int en, int t2, int t3, int t4, int flip);
CredoError_t condor_lp_get_rx_kp(CredoSlice_t* slice, int lane, int* kp);
CredoError_t condor_lp_set_rx_kp(CredoSlice_t* slice, int lane, int kp);
CredoError_t condor_lp_get_rx_kp_nrz(CredoSlice_t* slice, int lane, int* kp);
CredoError_t condor_lp_set_rx_kp_nrz(CredoSlice_t* slice, int lane, int kp);
CredoError_t condor_lp_get_rx_kp_pam4(CredoSlice_t* slice, int lane, int* kp);
CredoError_t condor_lp_set_rx_kp_pam4(CredoSlice_t* slice, int lane, int kp);
CredoError_t condor_lp_get_rx_kf(CredoSlice_t* slice, int lane, int* kf);
CredoError_t condor_lp_set_rx_kf(CredoSlice_t* slice, int lane, int kf);
CredoError_t condor_lp_get_rx_kf_nrz(CredoSlice_t* slice, int lane, int* kf);
CredoError_t condor_lp_set_rx_kf_nrz(CredoSlice_t* slice, int lane, int kf);
CredoError_t condor_lp_get_rx_kf_pam4(CredoSlice_t* slice, int lane, int* kf);
CredoError_t condor_lp_set_rx_kf_pam4(CredoSlice_t* slice, int lane, int kf);
CredoError_t condor_lp_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm);
CredoError_t condor_lp_get_rx_ppm_nrz(CredoSlice_t* slice, int lane, int* ppm);
CredoError_t condor_lp_get_rx_ppm_pam4(CredoSlice_t* slice, int lane, int* ppm);
CredoError_t condor_lp_set_rx_tia1_bias(CredoSlice_t* slice, int lane, unsigned tia_bias);
CredoError_t condor_lp_get_rx_tia1_bias(CredoSlice_t* slice, int lane, unsigned* tia_bias);

/* Condor LP RX attenuation */
CredoError_t condor_lp_set_rx_atten(CredoSlice_t* slice, int lane, int passive, int gain, int termtune);
CredoError_t condor_lp_get_rx_atten(CredoSlice_t* slice, int lane, int* passive, int* gain, int* termtune);
CredoError_t condor_lp_set_rx_atten_en3db(CredoSlice_t* slice, int lane, int en3db);
CredoError_t condor_lp_get_rx_atten_en3db(CredoSlice_t* slice, int lane, int* en3db);
CredoError_t condor_lp_set_rx_atten_termtune(CredoSlice_t* slice, int lane, int termtune);
CredoError_t condor_lp_get_rx_atten_termtune(CredoSlice_t* slice, int lane, int* termtune);
CredoError_t condor_lp_set_rx_atten_agcgain(CredoSlice_t* slice, int lane, int agcgain);
CredoError_t condor_lp_get_rx_atten_agcgain(CredoSlice_t* slice, int lane, int* agcgain);

/* Condor LP RX debug */
CredoError_t condor_lp_get_rx_ffe_stage_count(CredoSlice_t* slice, int lane, int* stages);
CredoError_t condor_lp_get_rx_ffe_tap_count(CredoSlice_t* slice, int lane, int* k_taps, int* s_taps);
CredoError_t condor_lp_get_rx_ffe_powerup(CredoSlice_t* slice, int lane, int stages[]);
CredoError_t condor_lp_set_rx_ffe_powerup(CredoSlice_t* slice, int lane, int stages[]);
CredoError_t condor_lp_get_rx_ffe_polarity(CredoSlice_t* slice, int lane, int ktaps[]);
CredoError_t condor_lp_set_rx_ffe_polarity(CredoSlice_t* slice, int lane, int ktaps[]);
CredoError_t condor_lp_get_rx_ffe(CredoSlice_t* slice, int lane, int taps[]);
CredoError_t condor_lp_set_rx_ffe(CredoSlice_t* slice, int lane, const int taps[]);
CredoError_t condor_lp_get_rx_ffe_fine(CredoSlice_t* slice, int lane, int taps[]);
CredoError_t condor_lp_set_rx_ffe_fine(CredoSlice_t* slice, int lane, const int taps[]);
CredoError_t condor_lp_set_rx_skef(CredoSlice_t* slice, int lane, int enable, int degen, int addcap, int skef_gain);
CredoError_t condor_lp_set_rx_skef_enable(CredoSlice_t* slice, int lane, int skef_en);
CredoError_t condor_lp_set_rx_skef_degen(CredoSlice_t* slice, int lane, int skef_degen);
CredoError_t condor_lp_set_rx_skef_addcap(CredoSlice_t* slice, int lane, int skef_addcap);
CredoError_t condor_lp_set_rx_skef_gain(CredoSlice_t* slice, int lane, int skef_gain);
CredoError_t condor_lp_set_rx_dac(CredoSlice_t* slice, int lane, int rx_dac);
CredoError_t condor_lp_set_rx_dac_nrz(CredoSlice_t* slice, int lane, int rx_dac);
CredoError_t condor_lp_set_rx_dac_pam4(CredoSlice_t* slice, int lane, int rx_dac);
CredoError_t condor_lp_set_f1over3(CredoSlice_t* slice, int lane, int value);
CredoError_t condor_lp_set_agcgain(CredoSlice_t*, int lane, unsigned value[]);
CredoError_t condor_lp_set_rx_ctle(CredoSlice_t*, int lane, unsigned ctle[]);
CredoError_t condor_lp_set_edge(CredoSlice_t*, int lane, unsigned value);
CredoError_t condor_lp_set_rx_delta(CredoSlice_t* slice, int lane, int delta);
CredoError_t condor_lp_set_rx_sm_bp1_nrz(CredoSlice_t* slice, int lane, int enable, int state);
CredoError_t condor_lp_set_rx_sm_bp1_pam4(CredoSlice_t* slice, int lane, int enable, int state);
CredoError_t condor_lp_get_rx_sm_bp1_nrz(CredoSlice_t* slice, int lane, int* enable, int* state, int* reached);
CredoError_t condor_lp_get_rx_sm_bp1_pam4(CredoSlice_t* slice, int lane, int* enable, int* state, int* reached);
CredoError_t condor_lp_set_rx_sm_bp2_nrz(CredoSlice_t* slice, int lane, int enable, int state);
CredoError_t condor_lp_set_rx_sm_bp2_pam4(CredoSlice_t* slice, int lane, int enable, int state);
CredoError_t condor_lp_get_rx_sm_bp2_nrz(CredoSlice_t* slice, int lane, int* enable, int* state, int* reached);
CredoError_t condor_lp_get_rx_sm_bp2_pam4(CredoSlice_t* slice, int lane, int* enable, int* state, int* reached);
CredoError_t condor_lp_get_rx_sm_state_nrz(CredoSlice_t* slice, int lane, int* state);
CredoError_t condor_lp_get_rx_sm_state_pam4(CredoSlice_t* slice, int lane, int* state);
CredoError_t condor_lp_continue_rx_sm_nrz(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_continue_rx_sm_pam4(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_set_rx_counter_target(CredoSlice_t* slice, int lane, int target);
CredoError_t condor_lp_set_rx_sm_counter_target_nrz(CredoSlice_t* slice, int lane, int target);
CredoError_t condor_lp_get_rx_sm_counter_target_nrz(CredoSlice_t* slice, int lane, int* target);

CredoError_t condor_lp_get_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);
CredoError_t condor_lp_set_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode);

/* Condor LP TX taps */
CredoError_t condor_lp_set_tx_taps_scale(CredoSlice_t* slice, int lane, const unsigned taps_scale[]);
CredoError_t condor_lp_get_tx_taps_scale(CredoSlice_t* slice, int lane, unsigned taps_scale[]);
CredoError_t condor_lp_set_tx_taps(CredoSlice_t*, int lane, const int taps[]);
CredoError_t condor_lp_get_tx_taps(CredoSlice_t*, int lane, int taps[]);
CredoError_t condor_lp_set_tx_taps_extended(CredoSlice_t* slice, int lane, const int taps_extended[]);
CredoError_t condor_lp_get_tx_taps_extended(CredoSlice_t*, int lane, int taps_extended[]);
CredoError_t condor_lp_reset_tx_taps(CredoSlice_t*, int lane);

/* Condor LP PRBS */
CredoError_t condor_lp_get_rx_prbs_enable(CredoSlice_t* slice, int lane, int* enable);
CredoError_t condor_lp_get_rx_prbs_enable_nrz(CredoSlice_t* slice, int lane, int* enable);
CredoError_t condor_lp_get_rx_prbs_enable_pam4(CredoSlice_t* slice, int lane, int* enable);
CredoError_t condor_lp_set_rx_prbs_enable_nrz(CredoSlice_t* slice, int lane, int enable);
CredoError_t condor_lp_set_rx_prbs_enable_pam4(CredoSlice_t* slice, int lane, int enable);
CredoError_t condor_lp_get_tx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode);
CredoError_t condor_lp_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t condor_lp_get_rx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode);
CredoError_t condor_lp_get_rx_prbs_nrz(CredoSlice_t* slice, int lane, CredoLanePrbsPattern_t* mode);
CredoError_t condor_lp_get_rx_prbs_pam4(CredoSlice_t* slice, int lane, CredoLanePrbsPattern_t* mode);
CredoError_t condor_lp_get_rx_prbs_lock(CredoSlice_t* slice, int lane, CredoPrbsLockStatus_t* status);
CredoError_t condor_lp_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t condor_lp_set_rx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t condor_lp_set_rx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t condor_lp_get_rx_prbs_error_count(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t condor_lp_get_rx_prbs_error_count_nrz(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t condor_lp_get_rx_prbs_error_count_pam4(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t condor_lp_rx_prbs_rst(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_rx_prbs_rst_nrz(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_rx_prbs_rst_pam4(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_get_rx_prbs_checker_enable_nrz(CredoSlice_t* slice, int lane, int* enable);
CredoError_t condor_lp_set_rx_prbs_checker_enable_nrz(CredoSlice_t* slice, int lane, int enable);
CredoError_t condor_lp_get_rx_prbs_checker_enable_pam4(CredoSlice_t* slice, int lane, int* enable);
CredoError_t condor_lp_set_rx_prbs_checker_enable_pam4(CredoSlice_t* slice, int lane, int enable);
CredoError_t condor_lp_prbs_gen_1error(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_prbs_gen_1error_nrz(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_prbs_gen_1error_pam4(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_set_rx_prbs_prev(CredoSlice_t* slice, int lane, bool en, unsigned prev);
CredoError_t condor_lp_set_rx_prbs_pattern_phase(CredoSlice_t* slice, int lane, unsigned phase);
CredoError_t condor_lp_reset_rx_prbs_pattern_count(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_get_rx_prbs_pattern_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]);
CredoError_t condor_lp_get_prbs_rx_autosync(CredoSlice_t* slice, int lane, bool* enabled);

CredoError_t condor_lp_lane_rx_reset(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_lane_rx_reset_nrz(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_lane_rx_reset_pam4(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_reset_lane_fast_nrz(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_reset_lane_fast_pam4(CredoSlice_t* slice, int lane);
CredoError_t condor_lp_reset_lane_simple_pam4(CredoSlice_t* slice, int lane);

/* Condor LP TX Asym */
CredoError_t condor_lp_set_tx_asym_enable(CredoSlice_t* slice, int lane, int enable);
CredoError_t condor_lp_get_tx_asym_enable(CredoSlice_t* slice, int lane, int* enable);
CredoError_t condor_lp_set_tx_asym_level(CredoSlice_t* slice, int lane, int level[4]);
CredoError_t condor_lp_get_tx_asym_level(CredoSlice_t* slice, int lane, int level[4]);

/* Condor LP TX Test Pattern */
CredoError_t condor_lp_get_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool* enable);
CredoError_t condor_lp_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable);
CredoError_t condor_lp_get_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode* mode);
CredoError_t condor_lp_set_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode mode);
CredoError_t condor_lp_get_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t* pattern);
CredoError_t condor_lp_set_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t pattern);

/* Internal functions */
#define CONDOR_LP_PRBS_INVALID ((unsigned)-1)
unsigned credo_to_condor_lp(CredoLanePrbsPattern_t prbs_mode, CredoLaneMode_t mode);

#endif
