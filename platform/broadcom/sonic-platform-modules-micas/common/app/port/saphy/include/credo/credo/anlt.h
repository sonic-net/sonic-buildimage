#ifndef CREDO_ANLT_H
#define CREDO_ANLT_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lane link training status
 */
typedef enum {
    CR_LT_STATUS_OFF = 0,
    CR_LT_STATUS_IDLE = 1,
    CR_LT_STATUS_TRAINING = 2,
    CR_LT_STATUS_FINISHED = 3,
    CR_LT_STATUS_FAILED = 4,
} CredoLinkTrainingStatus_t;

/**
 * @brief Lane link training state
 */
typedef enum {
    CR_LT_STATE_UNKNOWN = -2,
    CR_LT_STATE_EXIT = -1,
    CR_LT_STATE_OFF = 0,
    CR_LT_STATE_IDLE = 1,
    CR_LT_STATE_LINK_UP = 2,

    CR_LT_STATE_START = 0x10,
    CR_LT_STATE_PAM2_WAIT_FRAME_LOCK = 0x11,
    CR_LT_STATE_PAM2_WAIT_REMOTE_LOCK = 0x12,
    CR_LT_STATE_PAM2_WAIT_INITIAL_CMD = 0x13,
    CR_LT_STATE_PAM2_TX_ADJUST = 0x14,
    CR_LT_STATE_PAM2_TX_ADJUST_DONE = 0x15,

    CR_LT_STATE_PAM4_START = 0x100,
    CR_LT_STATE_PAM4_WAIT_FRAME_LOCK = 0x101,
    CR_LT_STATE_PAM4_WAIT_PAM4_FRAME = 0x102,
    CR_LT_STATE_PAM4_TX_ADJUST = 0x103,
    CR_LT_STATE_PAM4_TX_ADJUST_DONE = 0x104,
} CredoLinkTrainingState_t;

/**
 * @brief Auto-Negotiation State
 */
typedef enum {
    CR_AUTONEG_STATE_UNKNOWN = -1,
    CR_AUTONEG_STATE_OFF = 0,
    CR_AUTONEG_STATE_ENABLE = 1,
    CR_AUTONEG_STATE_TX_DISABLE = 2,
    CR_AUTONEG_STATE_ABILITY_DETECT = 3,
    CR_AUTONEG_STATE_ACK_DETECT = 4,
    CR_AUTONEG_STATE_COMPLETE_ACK = 5,
    CR_AUTONEG_STATE_NP_WAIT = 6,
    CR_AUTONEG_STATE_GOOD_CHECK = 7,
    CR_AUTONEG_STATE_LINK_STATUS_CHECK = 8,
    CR_AUTONEG_STATE_PARALLEL_DET_FAULT = 9,
    CR_AUTONEG_STATE_GOOD = 10
} CredoAutoNegState_t;

/**
 * @brief Get Auto-negotiation IEEE state
 * @param[in] slice
 * @param[in] lane
 * @param[out] state
 * @return Error Code
 */
CREDOAPI CredoError_t cr_autoneg_get_state(CredoSlice_t* slice, int lane, CredoAutoNegState_t* state);

/**
 * @brief Get Auto-negotiation restart counter
 * @param[in] slice
 * @param[in] lane
 * @param[out] count
 * @return Error Cde
 */
CREDOAPI CredoError_t cr_autoneg_get_restart_count(CredoSlice_t* slice, int lane, unsigned* count);

/**
 * @brief set Auto Neg page
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[in] pageId page ID to be set
 * @param[in] page page value to be set
 * @return Error Code
 */
CREDOAPI CredoError_t cr_autoneg_set_pages(CredoSlice_t* slice, int lane, int pageId, uint64_t page);

/**
 * @brief Get Auto Neg transmitted/received page
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] page_count the number of transmitted/received pages
 * @param[out] transmitted_pages the values of transmitted pages
 * @param[out] received_pages the value of received pages
 * @return Error Code
 */
CREDOAPI CredoError_t cr_autoneg_get_exchanged_pages(CredoSlice_t* slice, int lane, int* page_count,
                                                     uint64_t transmitted_pages[9], uint64_t received_pages[9]);

/**
 * @brief Get target lane link traning status
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] lt_status
 * @return Error Code
 */
CREDOAPI CredoError_t cr_link_training_get_status(CredoSlice_t* slice, int lane, CredoLinkTrainingStatus_t* lt_status);

/**
 * @brief Get target lane link traning detail state
 * @param[in] slice slice handle
 * @param[in] lane
 * @param[out] lt_state
 * @return Error Code
 */
CREDOAPI CredoError_t cr_link_training_get_state(CredoSlice_t* slice, int lane, CredoLinkTrainingState_t* lt_state);

/**
 * @brief Get target lane link training restart count
 * @param[in] slice
 * @param[in] lane
 * @param[out] count
 * @return Error Code
 */
CREDOAPI CredoError_t cr_link_training_get_restart_count(CredoSlice_t* slice, int lane, unsigned* count);

#ifdef __cplusplus
}
#endif

#endif
