#ifndef CR_DBG_SERDES_H
#define CR_DBG_SERDES_H

#include "credo/base.h"
#include "credo/debug/params.h"

#ifdef __cplusplus
extern "C" {
#endif

// Top PLL
/**
 * @brief Calibration slice top pll
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 *
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_cal_top_pll(CredoSlice_t* slice, int lane);

/**
 * @brief Init slice all pll setting
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_init_top_pll(CredoSlice_t* slice);

/**
 * @brief Get slice top pll vco cap
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[out] cap
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_top_pll_cap(CredoSlice_t* slice, unsigned* cap);

// Capability

/**
 * @brief Get target lane rx ffe capability
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] taps_len taps length
 * @param[out] sum_len summer length
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_ffe_range(CredoSlice_t* slice, int lane, int* taps_len, int* sum_len);

/**
 * @brief Get target lane rx ffe weighting table capability
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] row table row
 * @param[out] col table column
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_ffe_weighting_table_range(CredoSlice_t* slice, int lane, int* row, int* col);

/**
 * @brief Get target lane tx ffe capability
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] length
 * @param[out] extended_length
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_tx_ffe_range(CredoSlice_t* slice, int lane, int* length, int* extended_length);

/**
 * @brief Get target lane used dfe count
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] length
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_dfe_range(CredoSlice_t* slice, int lane, int* length);

/**
 * @brief Get target lane used isi count
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] length
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_isi_range(CredoSlice_t* slice, int lane, int* length);

// Lane PLL

/**
 * @brief Set target lane tx vco cap value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] tx_cap
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_tx_cap(CredoSlice_t* slice, int lane, int tx_cap);

/**
 * @brief Set target lane rx vco cap value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] rx_cap
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_rx_cap(CredoSlice_t* slice, int lane, int rx_cap);

/**
 * @brief Get target lane tx vco cap value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] tx_cap
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_tx_cap(CredoSlice_t* slice, int lane, int* tx_cap);

/**
 * @brief Get target lane rx vco cap value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] rx_cap
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_cap(CredoSlice_t* slice, int lane, int* rx_cap);

// RX Detail

/**
 * @brief Get target lane frequency accumulator value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] ppm
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm);

/**
 * @brief Get target lane rx skin effect enable status and degen value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] enable
 * @param[out] degen
 * @param[out] addcap
 * @param[out] gain
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_skef(CredoSlice_t* slice, int lane, int* enable, int* degen, int* addcap,
                                            int* gain);

/**
 * @brief Get target lane rx dac value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] rx_dac dac value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_dac(CredoSlice_t* slice, int lane, int* rx_dac);

/**
 * @brief Get target lane rx attenuator value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] passive
 * @param[out] gain
 * @param[out] termtune
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_attenuator(CredoSlice_t* slice, int lane, int* passive, int* gain,
                                                  int* termtune);

/**
 * @brief Get target lane ffe taps value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] taps rx taps value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_ffe_taps(CredoSlice_t* slice, int lane, int taps[]);

/**
 * @brief Get target lane ffe fine taps value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] taps rx fine taps value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_ffe_taps_fine(CredoSlice_t* slice, int lane, int taps[]);

/**
 * @brief Get target lane f1over3 value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_f1over3(CredoSlice_t* slice, int lane, int* value);

/**
 * @brief Get the agcgain count
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane id
 * @param[out] count how many rx agcgain taps
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_agcgain_count(CredoSlice_t* slice, int lane, unsigned* count);

/**
 * @brief Get target lane agcgain value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] agcgain gain1 store in agcgain[0], gain2 store in agcgain[1]
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]);

/**
 * @brief Get the ctle count
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane id
 * @param[out] count how many rx ctle values
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_ctle_count(CredoSlice_t* slice, int lane, unsigned* count);

/**
 * @brief Get target lane ctle value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] value ctle value list
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_ctle(CredoSlice_t* slice, int lane, unsigned value[]);

/**
 * @brief Get target lane delta overwrite value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] value delta overwrite value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_delta_phase(CredoSlice_t* slice, int lane, int* value);

/**
 * @brief Get target lane edge value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_edge(CredoSlice_t* slice, int lane, unsigned* value);

/**
 * @brief Get target lane dfe value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] dfe_taps
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_dfe(CredoSlice_t* slice, int lane, double dfe_taps[]);  // may only use 1

/**
 * @brief Get target lane eye value. If target lane is NRZ mode, only eyes[0] is available.
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] eyes
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_raw_eye(CredoSlice_t* slice, int lane, int eyes[3]);

/**
 * @brief Get signal detect status
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] sd signal detect value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_signal_detect(CredoSlice_t* slice, int lane, int* sd);

// RX debugging -- not to be used lightly

/**
 * @brief Set target lane rx skin effect enable status and degen value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable
 * @param[in] degen
 * @param[in] addcap
 * @param[in] gain
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_rx_skef(CredoSlice_t* slice, int lane, int enable, int degen, int addcap, int gain);

/**
 * @brief Set target lane rx dac value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] rx_dac dac value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_rx_dac(CredoSlice_t* slice, int lane, int rx_dac);

/**
 * @brief Set target lane rx attenuator value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] passive
 * @param[in] gain
 * @param[in] termtune
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_rx_attenuator(CredoSlice_t* slice, int lane, int passive, int gain, int termtune);

/**
 * @brief Set target lane rx ffe taps value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] taps rx taps value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_ffe_taps(CredoSlice_t* slice, int lane, const int taps[]);

/**
 * @brief Set target lane rx ffe fine taps value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] taps rx fine taps value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_ffe_taps_fine(CredoSlice_t* slice, int lane, const int taps[]);

/**
 * @brief Set target lane f1over3 value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_f1over3(CredoSlice_t* slice, int lane, int value);

/**
 * @brief Set target lane agcgain value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] value gain value list
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_agcgain(CredoSlice_t* slice, int lane, unsigned value[]);

/**
 * @brief Set target lane ctle value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] ctle ctle value list
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_ctle(CredoSlice_t* slice, int lane, unsigned ctle[]);

/**
 * @brief Set target lane delta overwrite value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] value delta overwrite value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_delta_phase(CredoSlice_t* slice, int lane, int value);

/**
 * @brief Set target lane edge value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_edge(CredoSlice_t* slice, int lane, unsigned value);

/**
 * @brief Get adapt count from firmware
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] count Every time receiver starts to adapt, this counter will increase by 1.
 * Wrap around on overflow, does not clear on read.
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_adapt_count(CredoSlice_t* slice, int lane, unsigned* count);

/**
 * @brief Get readapt count from firmware
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] count Every time receiver starts to adapt, this counter will increase by 1.
 * Saturate on overflow, clear on read.
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_readapt_count(CredoSlice_t* slice, int lane, unsigned* count);

/**
 * @brief Get link lost count from firmware
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] count Every time receiver can not hold the link, and the link is already established, this counter will
 * increase. Saturate on overflow, clear on read.
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_link_lost_count(CredoSlice_t* slice, int lane, unsigned* count);

/**
 * @brief Get link loss of signal count from firmware
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] count Every time receiver detects signal loss, this counter will increase by 1.
 * Saturate on overflow, clear on read. Note: If the receiver does not see signal, the counter will be at least 1.
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_los_count(CredoSlice_t* slice, int lane, unsigned* count);

/**
 * @brief Get channel estimate from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] chan_est channel estimate
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_channel_estimate(CredoSlice_t* slice, int lane, double* chan_est);

/**
 * @brief Get overflow frequency from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] of overflow frequency
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_of(CredoSlice_t* slice, int lane, unsigned* of);

/**
 * @brief Get high frequency from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] hf high frequency
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_hf(CredoSlice_t* slice, int lane, unsigned* hf);

/**
 * @brief Get eyes from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] eyes eye value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_eye(CredoSlice_t* slice, int lane, int eyes[3]);

/**
 * @brief Get isi from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] isi isi value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_isi(CredoSlice_t* slice, int lane, int isi[]);

/**
 * @brief Get ffe taps from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] taps ffe taps value
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_ffe(CredoSlice_t* slice, int lane, int taps[]);

/**
 * @brief Get ffe nbias from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] nbias ffe nbias
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_ffe_nbias(CredoSlice_t* slice, int lane, int nbias[]);

/**
 * @brief Get ffe kaccu from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] kaccu ffe kaccu
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_ffe_kaccu(CredoSlice_t* slice, int lane, double kaccu[]);

/**
 * @brief Get ffe weighting table from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] wt_table ffe weighting table
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_ffe_weighting_table(CredoSlice_t* slice, int lane, double** wt_table);

/**
 * @brief Get ffe polarity flip counter from firmware
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] flip_counter ffe polarity flip counter
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_ffe_flip_counter(CredoSlice_t* slice, int lane, int flip_counter[]);

/**
 * @addtogroup SerdesParam
 * @{
 */
#define CR_SPARAM_UP_TIME                "up_time"
#define CR_SPARAM_DOWN_TIME              "down_time"
#define CR_SPARAM_LMS_CLOCK              "lms_clock"
#define CR_SPARAM_RX_ADAPT               "rx_adapt"
#define CR_SPARAM_RX_AGCGAIN             "rx_agcgain"
#define CR_SPARAM_RX_AGCATTEN            "rx_agcatten"
#define CR_SPARAM_RX_ATTEN_GAIN          "rx_atten_gain"
#define CR_SPARAM_RX_ATTEN_PASSIVE       "rx_atten_passive"
#define CR_SPARAM_RX_ATTEN_TERMTUNE      "rx_atten_termtune"
#define CR_SPARAM_RX_CHANNEL_EST         "rx_channel_est"
#define CR_SPARAM_RX_CHANNEL_HF          "rx_channel_hf"
#define CR_SPARAM_RX_CHANNEL_OF          "rx_channel_of"
#define CR_SPARAM_RX_KF                  "rx_kf"
#define CR_SPARAM_RX_KP                  "rx_kp"
#define CR_SPARAM_RX_ENVELOPE            "rx_envelope"
#define CR_SPARAM_RX_CTLE                "rx_ctle"
#define CR_SPARAM_RX_CTLE_INDEX          "rx_ctle_index"
#define CR_SPARAM_RX_TIA1_BIAS           "rx_tia1_bias"
#define CR_SPARAM_RX_DAC                 "rx_dac"
#define CR_SPARAM_RX_DELTA               "rx_delta"
#define CR_SPARAM_RX_THS                 "rx_ths"
#define CR_SPARAM_RX_DC_SAR              "rx_dc_sar"
#define CR_SPARAM_RX_GAIN_SAR            "rx_gain_sar"
#define CR_SPARAM_RX_DFE_NL_MODE         "rx_dfe_nl_mode"
#define CR_SPARAM_RX_DFE                 "rx_dfe"
#define CR_SPARAM_RX_DFE_F0              "rx_dfe_f0"
#define CR_SPARAM_RX_DFE_F1              "rx_dfe_f1"
#define CR_SPARAM_RX_DFE_ALL             "rx_dfe_all"
#define CR_SPARAM_RX_VGA                 "rx_vga"
#define CR_SPARAM_RX_EDGE                "rx_edge"
#define CR_SPARAM_RX_EYE_HEIGHT          "rx_eye_height"
#define CR_SPARAM_RX_EYE_ALL             "rx_eye_all"
#define CR_SPARAM_RX_EYE                 "rx_eye"
#define CR_SPARAM_RX_F1OVER3             "rx_f1over3"
#define CR_SPARAM_RX_FFE_FLIP_COUNTER    "rx_ffe_flip_counter"
#define CR_SPARAM_RX_FFE_KACCU           "rx_ffe_kaccu"
#define CR_SPARAM_RX_FFE_NBIAS           "rx_ffe_nbias"
#define CR_SPARAM_RX_FFE_TAPS_ALL        "rx_ffe_taps_all"
#define CR_SPARAM_RX_FFE_TAPS            "rx_ffe_taps"
#define CR_SPARAM_RX_FFE_TAPS_FINE       "rx_ffe_taps_fine"
#define CR_SPARAM_RX_FFE_DEGENDL_TIA     "rx_ffe_degendl_tia"
#define CR_SPARAM_RX_FFE_WEIGHTING_TABLE "rx_ffe_weighting_table"
#define CR_SPARAM_RX_GRAYCODE            "rx_graycode"
#define CR_SPARAM_RX_INPUT_COUPLING      "rx_input_coupling"
#define CR_SPARAM_RX_LINKLOST            "rx_linklost"
#define CR_SPARAM_RX_MSBLSB              "rx_msblsb"
#define CR_SPARAM_RX_PLL_CAP             "rx_pll_cap"
#define CR_SPARAM_RX_POL                 "rx_pol"
#define CR_SPARAM_RX_PPM                 "rx_ppm"
#define CR_SPARAM_RX_PRECODER            "rx_precoder"
#define CR_SPARAM_RX_READAPT             "rx_readapt"
#define CR_SPARAM_RX_READY               "rx_ready"
#define CR_SPARAM_RX_SIGNAL_DETECT       "rx_signal_detect"
#define CR_SPARAM_RX_SKEF                "rx_skef"
#define CR_SPARAM_RX_SKEF_ADDCAP         "rx_skef_addcap"
#define CR_SPARAM_RX_SKEF_DEGEN          "rx_skef_degen"
#define CR_SPARAM_RX_SKEF_EN             "rx_skef_en"
#define CR_SPARAM_RX_SKEF_GAIN           "rx_skef_gain"
#define CR_SPARAM_TX_GRAYCODE            "tx_graycode"
#define CR_SPARAM_TX_MSBLSB              "tx_msblsb"
#define CR_SPARAM_TX_PLL_CAP             "tx_pll_cap"
#define CR_SPARAM_TX_POL                 "tx_pol"
#define CR_SPARAM_TX_PRECODER            "tx_precoder"
#define CR_SPARAM_TX_TAPS                "tx_taps"
#define CR_SPARAM_TX_TAPS_RAW            "tx_taps_raw"
#define CR_SPARAM_TX_TAPS_SCALE          "tx_taps_scale"
#define CR_SPARAM_TOP_PLL_CAP            "top_pll_cap"
#define CR_SPARAM_RX_ISI                 "rx_isi"
#define CR_SPARAM_RX_ISI_ALL             "rx_isi_all"
#define CR_SPARAM_TX_TAPS_NONLINEAR      "tx_taps_nl"
#define CR_SPARAM_TX_INTERPLOTATOR       "tx_intp"
#define CR_SPARAM_PLL_LOCK               "pll_lock"
#define CR_SPARAM_TOP_PLL_LOCK           "top_pll_lock"
#define CR_SPARAM_TX_PHASE               "tx_phase"
#define CR_SPARAM_RX_PHASE               "rx_phase"
/** @} */

/**
 * @brief Set target lane extended taps value
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] taps_extended
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_tx_taps_extended(CredoSlice_t* slice, int lane, const int taps_extended[]);

/**
 * @brief Get target lane extended taps value
 * @deprecated replaced by SerdesParam group for more flexibility in v2.1.0
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] taps_extended
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_tx_taps_extended(CredoSlice_t* slice, int lane, int taps_extended[]);

/**
 * @brief Set target lane half amptitude value
 * @param[in] slice slice handle
 * @param[in] lane lane to use
 * @param[in] taps_scale taps scale of lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_set_tx_taps_scale(CredoSlice_t* slice, int lane, const unsigned taps_scale[]);

/**
 * @brief Get target lane half amptitude value
 * @param[in] slice slice handle
 * @param[in] lane lane to use
 * @param[out] taps_scale taps scale of lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_tx_taps_scale(CredoSlice_t* slice, int lane, unsigned taps_scale[]);

/**
 * @brief HW CDR lock "RDY"
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] status
 * @return Error Code
 */
CREDOAPI CredoError_t cr_serdes_get_rx_ready(CredoSlice_t* slice, int lane, int* status);

#ifdef __cplusplus
}
#endif

#endif
