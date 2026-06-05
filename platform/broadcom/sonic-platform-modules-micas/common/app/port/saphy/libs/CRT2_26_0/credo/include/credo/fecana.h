#ifndef CREDO_FECANA_H
#define CREDO_FECANA_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fec analyzer error type
 */
typedef enum { CR_FEC_ERROR_FRAME = 0, CR_FEC_ERROR_SYMBOL = 1, CR_FEC_ERROR_BIT = 2 } CredoFecErrorType_t;

/**
 * @brief Fec analyzer Configuration Structure
 */
typedef struct {
    uint16_t codeword_size;  //!< in bits, for 802.3 typically 5440 (PAM4) or 5280 (NRZ)
    uint16_t symbol_size;    //!< for 802.3 typically 10
    uint16_t threshold;      //!< for 802.3 typically 15 (PAM4) or 7 (NRZ)
    CredoFecErrorType_t error_type;
} CredoFecAnalyzerConfig_t;

/**
 * @brief Set target lane FEC analyzer
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable
 * @param[in] config
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_configure(CredoSlice_t* slice, int lane, int enable, CredoFecAnalyzerConfig_t* config);

/**
 * @brief Get target lane FEC analyzer
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] enable
 * @param[out] config
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_query(CredoSlice_t* slice, int lane, int* enable, CredoFecAnalyzerConfig_t* config);

/**
 * @brief Reset the FEC analyzer counter
 * - Auto sync to PRBS traffic
 * - Reset couners
 * - Update lane fecana timer
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_reset(CredoSlice_t* slice, int lane);

/**
 * @brief Get target lane FEC analyzer counter by counter selection
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] counter_sel
 * @param[out] counter
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_get_raw_counter(CredoSlice_t* slice, int lane, int counter_sel, unsigned* counter);

/**
 * @brief Get target lane FEC analyzer counter
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] pre_fec
 * @param[out] post_fec
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_get_counter(CredoSlice_t* slice, int lane, unsigned* pre_fec, unsigned* post_fec);

/**
 * @brief Set target lane FEC analyzer histogram group
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] group
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_set_hist_group(CredoSlice_t* slice, int lane, int group);

/**
 * @brief Get target lane FEC analyzer histogram group
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] group
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_get_hist_group(CredoSlice_t* slice, int lane, int* group);

/**
 * @brief Get target lane FEC analyzer histogram data
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] hist_data
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_get_hist_counter(CredoSlice_t* slice, int lane, unsigned hist_data[4]);

/**
 * @brief Get the fec analyzer duration for lane counters
 * @param[in] slice slice handle
 * @param[in] lane lane to get duration
 * @param[out] duration_ms duration in milliseconds
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_get_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms);

/**
 * @brief Get the fec analyzer error rate for a lane
 *
 * @note If you provide duration_ms=0, the fec analyzer will use the lane global duration
 * @param[in] slice slice
 * @param[in] lane lane to use
 * @param[in] counter_sel what counter to get error rate
 * @param[in] duration_ms duration in milliseconds to record
 * @param[out] error_rate error rate for the counter
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_get_error_rate(CredoSlice_t* slice, int lane, int counter_sel, int duration_ms,
                                               double* error_rate);

/**
 * @brief Get auto sync status for fec analyzer
 *
 * @note if enabled then bursty errors may be missed
 *
 * To disable auto sync, use cr_fecana_reset
 * @param[in] slice
 * @param[in] lane
 * @param[out] enabled
 * @return Error Code
 */
CREDOAPI CredoError_t cr_fecana_get_autosync(CredoSlice_t* slice, int lane, bool* enabled);

#ifdef __cplusplus
}
#endif

#endif
