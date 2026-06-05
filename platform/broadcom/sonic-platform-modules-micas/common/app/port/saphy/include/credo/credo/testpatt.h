#ifndef CREDO_TESTPATT_H
#define CREDO_TESTPATT_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief State of the transmitter
 */
typedef enum {
    CR_TESTPATT_CUSTOM,  //!< Use custom bit pattern
    CR_TESTPATT_JP03A,
    CR_TESTPATT_JP03B,
    CR_TESTPATT_LINEAR,
    CR_TESTPATT_UNKNOWN = 100
} CredoTestPatternMode_t;

/**
 * @brief alias for backwards compatibility
 *
 */
typedef CredoTestPatternMode_t CredoLaneTxTestPatternMode;

/**
 * @brief Set target lane test pattern enable state
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable Tx test pattern enable state
 * @return Error Code
 */
CREDOAPI CredoError_t cr_testpatt_set_tx_enable(CredoSlice_t* slice, int lane, bool enable);

/**
 * @brief Get target lane test pattern enable state
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] enable Tx test pattern enable state
 * @return Error Code
 */
CREDOAPI CredoError_t cr_testpatt_get_tx_enable(CredoSlice_t* slice, int lane, bool* enable);

/**
 * @brief Set target lane test pattern memory
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] pattern Tx test pattern memory
 * @return Error Code
 */
CREDOAPI CredoError_t cr_testpatt_set_tx_memory(CredoSlice_t* slice, int lane, uint64_t pattern);

/**
 * @brief Get target lane test pattern memory
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] pattern Tx test pattern memory
 * @return Error Code
 */
CREDOAPI CredoError_t cr_testpatt_get_tx_memory(CredoSlice_t* slice, int lane, uint64_t* pattern);

/**
 * @brief Set target lane test pattern mode, only valid when test pattern enabled
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] mode Tx test pattern mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_testpatt_set_tx_mode(CredoSlice_t* slice, int lane, CredoTestPatternMode_t mode);

/**
 * @brief Get target lane test pattern mode, only valid when test pattern enabled
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] mode Tx test pattern mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_testpatt_get_tx_mode(CredoSlice_t* slice, int lane, CredoTestPatternMode_t* mode);

#ifdef __cplusplus
}
#endif

#endif
