#ifndef CREDO_FRECOV_H
#define CREDO_FRECOV_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure fast recovery for a lane
 * @param[in] slice
 * @param[in] lane
 * @param[in] timeout_ms timeout duration before fast recovery gives up
 * @return Error Code
 */
CREDOAPI CredoError_t cr_frecov_configure(CredoSlice_t* slice, int lane, unsigned timeout_ms);

/**
 * @brief Fast Recovery Status
 */
typedef enum {
    CR_FRECOV_DISABLED = 0,
    CR_FRECOV_ENABLED = 1,  //!< fast recovery is enabled but not ready
    CR_FRECOV_ARMED = 2     //!< fast recovery is enabled and ready in the event of a link down
} CredoFastRecoveryStatus_t;

/**
 * @brief Get the fast recovery status
 * @param[in] slice
 * @param[in] lane
 * @param[out] timeout_ms how long fast recovery timeout
 * @param[out] status get the status of fast recovery for the lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_frecov_get_status(CredoSlice_t* slice, int lane, unsigned* timeout_ms,
                                           CredoFastRecoveryStatus_t* status);

/**
 * @brief Get the number of fast recoveries that occured for the lane
 * @param[in] slice
 * @param[in] lane
 * @param[out] count
 * @return Error Code
 */
CREDOAPI CredoError_t cr_frecov_get_recover_count(CredoSlice_t* slice, int lane, unsigned* count);

#ifdef __cplusplus
}
#endif

#endif
