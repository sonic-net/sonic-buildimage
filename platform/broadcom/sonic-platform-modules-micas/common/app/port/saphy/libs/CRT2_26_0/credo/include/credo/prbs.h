#ifndef CREDO_PRBS_H
#define CREDO_PRBS_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prbs pattern of Lane
 */
typedef enum {
    CR_PRBS7 = 0,
    CR_PRBS9 = 1,
    CR_PRBS11 = 2,
    CR_PRBS13 = 3,
    CR_PRBS15 = 4,
    CR_PRBS23 = 5,
    CR_PRBS31 = 6,
    CR_PRBS19 = 7,
    CR_PRBS_UNKNOWN = 100
} CredoPrbsPattern_t;

/**
 * @brief backwards compatiblity alias
 */
typedef CredoPrbsPattern_t CredoLanePrbsPattern_t;

/**
 * @brief prbs lock status
 */
typedef enum { CR_PRBS_LOCK_NO, CR_PRBS_LOCK_YES, CR_PRBS_LOCK_INVALID } CredoPrbsLockStatus_t;

/**
 * @brief Get target lane tx prbs enable status and prbs pattern
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] enable 1 = enable, 0 = disable
 * @param[out] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_tx_generator(CredoSlice_t* slice, int lane, int* enable, CredoPrbsPattern_t* mode);

/**
 * @brief Get target lane rx prbs enable status and prbs pattern
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] enable 1 = enable, 0 = disable
 * @param[out] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_rx_checker(CredoSlice_t* slice, int lane, int* enable, CredoPrbsPattern_t* mode);

/**
 * @brief Set target lane tx prbs enable status and prbs pattern
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable 1 = enable, 0 = disable
 * @param[in] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_set_tx_generator(CredoSlice_t* slice, int lane, int enable, CredoPrbsPattern_t mode);

/**
 * @brief Set target lane rx prbs enable status and prbs pattern
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable 1 = enable, 0 = disable
 * @param[in] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_set_rx_checker(CredoSlice_t* slice, int lane, int enable, CredoPrbsPattern_t mode);

/**
 * @brief Gets if the prbs checker is locked to a pattern
 * @param slice slice handle
 * @param lane lane to check
 * @param is_locked is the rx checker locked
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_rx_lock(CredoSlice_t* slice, int lane, CredoPrbsLockStatus_t* is_locked);

/**
 * @brief Get target lane rx prbs error counter
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] count
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_rx_count(CredoSlice_t* slice, int lane, uint32_t* count);

/**
 * @brief Get target lane rx ber
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] time_ms 0 to collect ber global, otherwise time in ms to collect ber
 * without reset
 * @param[out] ber
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_rx_ber(CredoSlice_t* slice, int lane, int time_ms, double* ber);

/**
 * @brief Get target lanes rx ber
 * @param[in] slice slice
 * @param[in] lanes lanes to capture ber
 * @param[in] time_ms 0 to collect ber global otherwise windowed ber collection over given time
 * @param[out] ber
 * @param[in] count
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_rx_ber_all(CredoSlice_t* slice, const int lanes[], int time_ms, double ber[],
                                             unsigned count);
/**
 * @brief Get prbs timer duration
 * @param[in] slice slice handle
 * @param[in] lane lane to get timer
 * @param[out] duration_ms milliseconds prbs timer is running
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_rx_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms);

/**
 * @brief Reset target lane prbs error counter
 *
 * Also clears slice prbs timer.
 *
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_reset_rx_count(CredoSlice_t* slice, int lane);

/**
 * @brief Generate 1 bit error to target lane
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_generate_tx_error(CredoSlice_t* slice, int lane);

/**
 * @brief Check if PRBS auto sync is enabled
 *
 * @note If auto sync is enabled, then the PRBS may miss some bursty errors.
 * To disable auto sync, use cr_prbs_reset_rx_count
 *
 * @param[in] slice
 * @param[in] lane
 * @param[out] enabled is auto sync enabled
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_rx_autosync(CredoSlice_t* slice, int lane, bool* enabled);

/**
 * @brief Prbs Pattern checker set symbol type
 *
 * @param[in] slice
 * @param[in] lane
 * @param[in] en
 * @param[in] symbol
 * @return Error Code
 */

CREDOAPI CredoError_t cr_prbs_pattchecker_set_symbol(CredoSlice_t* slice, int lane, bool en, unsigned symbol);
/**
 * @brief Clear prbs pattern checker count
 *
 * @param[in] slice
 * @param[in] lane
 * @return Error Code
 */

CREDOAPI CredoError_t cr_prbs_pattchecker_reset_count(CredoSlice_t* slice, int lane);
/**
 * @brief Get prbs pattern checker error count for different transitions
 *
 * @param[in] slice
 * @param[in] lane
 * @param[out] pattern_count
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_pattchecker_get_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]);
/**
 * @brief Set the phase position to measure pattern checker values
 *
 * @param[in] slice
 * @param[in] lane
 * @param[in] phase
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_pattchecker_set_phase(CredoSlice_t* slice, int lane, unsigned phase);

// TX Checker

/**
 * @brief Get target lane rx prbs enable status and prbs pattern
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] enable 1 = enable, 0 = disable
 * @param[out] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_tx_checker(CredoSlice_t* slice, int lane, int* enable, CredoPrbsPattern_t* mode);

/**
 * @brief Set target lane rx prbs enable status and prbs pattern
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable 1 = enable, 0 = disable
 * @param[in] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_set_tx_checker(CredoSlice_t* slice, int lane, int enable, CredoPrbsPattern_t mode);

/**
 * @brief Get target lane rx prbs error counter
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] count
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_tx_count(CredoSlice_t* slice, int lane, uint32_t count[2]);

/**
 * @brief Get target lane rx ber
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] time_ms 0 to collect ber global, otherwise time in ms to collect ber
 * without reset
 * @param[out] ber
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_tx_ber(CredoSlice_t* slice, int lane, int time_ms, double ber[2]);

/**
 * @brief Get target lanes rx ber
 * @param[in] slice slice
 * @param[in] lanes lanes to capture ber
 * @param[in] time_ms 0 to collect ber global otherwise windowed ber collection over given time
 * @param[out] ber
 * @param[in] count
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_tx_ber_all(CredoSlice_t* slice, const int lanes[], int time_ms, double ber[][2],
                                             unsigned count);
/**
 * @brief Get prbs timer duration
 * @param[in] slice slice handle
 * @param[in] lane lane to get timer
 * @param[out] duration_ms milliseconds prbs timer is running
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_get_tx_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms);

/**
 * @brief Reset target lane prbs error counter
 *
 * Also clears slice prbs timer.
 *
 * @param[in] slice slice handle
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_reset_tx_count(CredoSlice_t* slice, int lane);

#ifdef __cplusplus
}
#endif

#endif
