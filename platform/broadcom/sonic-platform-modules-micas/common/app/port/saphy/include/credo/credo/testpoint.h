#ifndef CREDO_TESTPOINT_H
#define CREDO_TESTPOINT_H

#include "credo/base.h"

/**
 * @brief Credo test point
 *
 */
typedef struct {
    int lane;        //!< target lane to measure testpoint
    int group;       //!< group of the testpoint
    int mode;        //!< measuring mode
    int index;       //!< index in the group
    bool div2;       //!< Enable divide by 2, useful if testpoint voltage would saturate vsensor
    uint32_t flags;  //!< Custom feature flags
} CredoTestPoint_t;

/**
 * @brief Select testpoint
 *
 * @param[in] slice slice to configure
 * @param[in] testpoint testpoint to set
 * @return Error Code
 */
CREDOAPI CredoError_t cr_testpoint_select(CredoSlice_t* slice, const CredoTestPoint_t* testpoint);

/**
 * @brief Clear selected testpoint
 *
 * @param[in] slice
 * @return Error Code
 */
CREDOAPI CredoError_t cr_testpoint_clear(CredoSlice_t* slice);

/**
 * @brief Read selected testpoint
 *
 * @param[in] slice
 * @param[out] value value of selected testpoint
 * @return Error Code
 */
CREDOAPI CredoError_t cr_testpoint_read(CredoSlice_t* slice, double* value);

#endif
