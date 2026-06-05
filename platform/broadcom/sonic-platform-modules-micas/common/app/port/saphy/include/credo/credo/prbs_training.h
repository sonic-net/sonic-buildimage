#ifndef CREDO_PRBS_TRAINING_H
#define CREDO_PRBS_TRAINING_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PRBS Training Status
 */
typedef enum {
    CR_PRBS_TRAINING_ERROR = -2,      //!< PRBS Training failed, need to relink
    CR_PRBS_TRAINING_RELINK = -1,     //!< SerDes link was lost, need to relink
    CR_PRBS_TRAINING_OPTIMIZING = 0,  //!< SerDes adaptation is occuring, or it is waiting for signal
    CR_PRBS_TRAINING_LINKED = 1       //!< Link is up and finished
} CredoPrbsTrainingStatus_t;

/**
 * @brief Get the Rx status
 *
 * @param[in] slice
 * @param[in] lane
 * @param[out] status
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_training_rx_get_status(CredoSlice_t* slice, int lane, CredoPrbsTrainingStatus_t* status);
/**
 * @brief Relink the RX
 *
 * Restarts the prbs training. SerDes will go through adaptation sequence
 *
 * The relink will only occur if `training_status < 0` otherwise this function noops.
 *
 * @param[in] slice
 * @param[in] lane
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_training_rx_relink(CredoSlice_t* slice, int lane);
/**
 * @brief Enable/disable PRBS training mode traffic generation
 * @param[in] slice
 * @param[in] lane
 * @param[in] enable enable or disable the PRBS training traffic
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_training_tx_enable(CredoSlice_t* slice, int lane, bool enable);
/**
 * @brief Is the tx sending prbs training traffic
 * @param[in] slice
 * @param[in] lane
 * @param[out] enable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_prbs_training_tx_is_enabled(CredoSlice_t* slice, int lane, bool* enable);

#ifdef __cplusplus
}
#endif

#endif
