#ifndef CANARY_SERDES_H
#define CANARY_SERDES_H

#include "sdk.h"

/* Canary capability */
CredoError_t canary_get_rx_ffe_range(CredoSlice_t*, int lane, int* taps_len, int* sum_len);
CredoError_t canary_get_tx_ffe_range(CredoSlice_t*, int lane, int* length, int* extended_length);
CredoError_t canary_get_rx_dfe_range(CredoSlice_t*, int lane, int* length);
CredoError_t canary_get_rx_isi_range(CredoSlice_t*, int lane, int* length);

/* Canary serdes interface */
CredoError_t canary_set_tx_gray_code(CredoSlice_t*, int lane, int tx_gc);
CredoError_t canary_set_rx_gray_code(CredoSlice_t*, int lane, int rx_gc);
CredoError_t canary_get_tx_gray_code(CredoSlice_t*, int lane, int* tx_gc);
CredoError_t canary_get_rx_gray_code(CredoSlice_t*, int lane, int* rx_gc);
CredoError_t canary_set_tx_precoder(CredoSlice_t*, int lane, int tx_pc);
CredoError_t canary_set_rx_precoder(CredoSlice_t*, int lane, int rx_pc);
CredoError_t canary_get_tx_precoder(CredoSlice_t*, int lane, int* tx_pc);
CredoError_t canary_get_rx_precoder(CredoSlice_t*, int lane, int* rx_pc);

CredoError_t canary_set_tx_msb(CredoSlice_t*, int lane, int tx_msb);
CredoError_t canary_set_rx_msb(CredoSlice_t*, int lane, int rx_msb);
CredoError_t canary_get_tx_msb(CredoSlice_t*, int lane, int* tx_msb);
CredoError_t canary_get_rx_msb(CredoSlice_t*, int lane, int* rx_msb);

/* Canary serdes environment */
CredoError_t canary_set_tx_polarity(CredoSlice_t*, int lane, int tx_pol);
CredoError_t canary_set_rx_polarity(CredoSlice_t*, int lane, int rx_pol);
CredoError_t canary_get_tx_polarity(CredoSlice_t*, int lane, int* tx_pol);
CredoError_t canary_get_rx_polarity(CredoSlice_t*, int lane, int* rx_pol);
CredoError_t canary_set_rx_input_mode(CredoSlice_t*, int lane, CredoLaneCoupling_t input_mode);
CredoError_t canary_get_rx_input_mode(CredoSlice_t*, int lane, CredoLaneCoupling_t* input_mode);

/* Canary per-lane PLL */
CredoError_t canary_set_tx_cap(CredoSlice_t*, int lane, int tx_cap);
CredoError_t canary_set_rx_cap(CredoSlice_t*, int lane, int rx_cap);
CredoError_t canary_get_tx_cap(CredoSlice_t*, int lane, int* tx_cap);
CredoError_t canary_get_rx_cap(CredoSlice_t*, int lane, int* rx_cap);
CredoError_t canary_set_low_vaa(CredoSlice_t* slice, int lane, int enable);
CredoError_t canary_get_low_vaa(CredoSlice_t* slice, int lane, int* enable);

/* Canary RX */
CredoError_t canary_get_version_id(CredoSlice_t* slice, int lane, uint64_t* version);
CredoError_t canary_get_rx_ppm(CredoSlice_t*, int lane, int* ppm);
CredoError_t canary_get_rx_skef(CredoSlice_t*, int lane, int* enable, int* degen, int* addcap, int* gain);
CredoError_t canary_get_rx_dac(CredoSlice_t*, int lane, int* dac);
CredoError_t canary_get_ffe_taps(CredoSlice_t*, int lane, int taps[]);
CredoError_t canary_get_ffe_nbias(CredoSlice_t* slice, int lane, int nbias[]);
CredoError_t canary_get_f1over3(CredoSlice_t*, int lane, int* value);
CredoError_t canary_get_agcgain_count(CredoSlice_t*, int lane, unsigned* count);
CredoError_t canary_get_agcgain(CredoSlice_t*, int lane, unsigned agcgain[]);
CredoError_t canary_get_ctle_count(CredoSlice_t*, int lane, unsigned* count);
CredoError_t canary_get_ctle_index(CredoSlice_t* slice, int lane, unsigned* ctle_index);
CredoError_t canary_get_ctle(CredoSlice_t*, int lane, unsigned ctle[]);
CredoError_t canary_get_delta_phase(CredoSlice_t*, int lane, int* value);
CredoError_t canary_get_edge(CredoSlice_t*, int lane, unsigned* value);
CredoError_t canary_get_dfe(CredoSlice_t*, int lane, double dfe_taps[]);
CredoError_t canary_get_eye(CredoSlice_t*, int lane, int eyes[3]);
CredoError_t canary_get_lane_ready(CredoSlice_t*, int lane, int* ready);
CredoError_t canary_get_signal_detect(CredoSlice_t*, int lane, int* sd);

/* Canary RX debug */
CredoError_t canary_set_rx_skef(CredoSlice_t*, int lane, int enable, int degen, int addcap, int gain);
CredoError_t canary_set_rx_dac(CredoSlice_t*, int lane, int rx_dac);
CredoError_t canary_set_ffe_taps(CredoSlice_t*, int lane, const int taps[]);
CredoError_t canary_set_f1over3(CredoSlice_t*, int lane, int value);
CredoError_t canary_set_agcgain(CredoSlice_t*, int lane, unsigned value[]);
CredoError_t canary_set_ctle(CredoSlice_t*, int lane, unsigned ctle[]);
CredoError_t canary_set_delta_phase(CredoSlice_t*, int lane, int value);
CredoError_t canary_set_edge(CredoSlice_t*, int lane, unsigned value);

/* Canary TX taps */
CredoError_t canary_set_tx_taps_scale(CredoSlice_t*, int lane, const unsigned taps_scale[]);
CredoError_t canary_get_tx_taps_scale(CredoSlice_t*, int lane, unsigned taps_scale[]);
CredoError_t canary_set_tx_taps(CredoSlice_t*, int lane, const int taps[]);
CredoError_t canary_set_tx_taps_extended(CredoSlice_t*, int lane, const int taps_extended[]);
CredoError_t canary_get_tx_taps(CredoSlice_t*, int lane, int taps[]);
CredoError_t canary_get_tx_taps_extended(CredoSlice_t*, int lane, int taps_extended[]);
CredoError_t canary_reset_tx_taps(CredoSlice_t*, int lane);

/* Canary PRBS */
CredoError_t canary_get_tx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode);
CredoError_t canary_get_rx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode);
CredoError_t canary_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t canary_set_rx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t canary_set_rx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t canary_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t canary_get_rx_prbs_count(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t canary_reset_rx_prbs_count_nrz(CredoSlice_t* slice, int lane);
CredoError_t canary_reset_rx_prbs_count_pam4(CredoSlice_t* slice, int lane);
CredoError_t canary_reset_rx_prbs_count(CredoSlice_t* slice, int lane);
CredoError_t canary_prbs_gen_1error(CredoSlice_t* slice, int lane);
CredoError_t canary_get_prbs_rx_autosync(CredoSlice_t* slice, int lane, bool* enabled);
CredoError_t canary_lane_rx_reset(CredoSlice_t* slice, int lane);
CredoError_t canary_set_rx_prbs_prev(CredoSlice_t* slice, int lane, bool en, unsigned prev);
CredoError_t canary_set_rx_prbs_pattern_phase(CredoSlice_t* slice, int lane, unsigned phase);
CredoError_t canary_reset_rx_prbs_pattern_count(CredoSlice_t* slice, int lane);
CredoError_t canary_get_rx_prbs_pattern_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]);

/* Canary TX Test Pattern */
CredoError_t canary_get_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool* enable);
CredoError_t canary_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable);
CredoError_t canary_get_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode* mode);
CredoError_t canary_set_tx_test_pattern_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode mode);
CredoError_t canary_get_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t* pattern);
CredoError_t canary_set_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t pattern);

/* Internal functions */
#define CANARY_PRBS_INVALID ((unsigned)-1)
unsigned credo_to_canary(CredoLanePrbsPattern_t prbs_mode, CredoLaneMode_t mode);

#endif
