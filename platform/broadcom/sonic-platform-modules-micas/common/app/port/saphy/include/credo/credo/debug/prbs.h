#ifndef CR_DBG_PRBS_H
#define CR_DBG_PRBS_H

#include "credo/base.h"
#include "credo/prbs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set target lane rx prbs to nrz mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable 1 = enable, 0 = disable
 * @param[in] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_set_rx_nrz(CredoSlice_t* slice, int lane, int enable, CredoPrbsPattern_t mode);

/**
 * @brief Set target lane rx prbs to pam4 mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable 1 = enable, 0 = disable
 * @param[in] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_set_rx_pam4(CredoSlice_t* slice, int lane, int enable, CredoPrbsPattern_t mode);

/**
 * @brief Set target lane tx prbs to nrz mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable 1 = enable, 0 = disable
 * @param[in] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_set_tx_nrz(CredoSlice_t* slice, int lane, int enable, CredoPrbsPattern_t mode);

/**
 * @brief Set target lane tx prbs to pam4 mode
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] enable 1 = enable, 0 = disable
 * @param[in] mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_set_tx_pam4(CredoSlice_t* slice, int lane, int enable, CredoPrbsPattern_t mode);

#ifdef __cplusplus
}
#endif

#endif
