#ifndef CR_DBG_SRAM_H
#define CR_DBG_SRAM_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sram status
 */
typedef enum {
    CR_SRAM_NO_ERROR,  //!< Sram ECC no error
    CR_SRAM_CORR_ERROR,
    CR_SRAM_UNCORR_ERROR
} CredoSramStatus_t;

/**
 * @brief Get slice sram ecc status.
 * @param[in] slice slice handle
 * @param[out] sram_status slice sram status
 * @return Error Code
 */
CREDOAPI CredoError_t cr_sram_get_status(CredoSlice_t* slice, CredoSramStatus_t* sram_status);

/**
 * @brief Inject slice sram ecc error.
 * @param[in] slice slice handle
 * @param[in] sram_status slice sram error type
 * @return Error Code
 */
CREDOAPI CredoError_t cr_sram_generate_error(CredoSlice_t* slice, CredoSramStatus_t sram_status);

#ifdef __cplusplus
}
#endif

#endif
