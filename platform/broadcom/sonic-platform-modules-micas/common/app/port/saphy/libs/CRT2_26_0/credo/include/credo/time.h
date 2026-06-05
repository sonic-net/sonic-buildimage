#ifndef CREDO_TIME_H
#define CREDO_TIME_H

#include "credo/base.h"

#include <float.h>
#include <math.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the timestamp that firmware started running
 * @param[in] slice
 * @param[out] unix_time timestamp since epoch
 */
CREDOAPI CredoError_t cr_time_start(CredoSlice_t* slice, double* unix_time);

/**
 * @brief Get the system timedelta since firmware started running
 * @param[in] slice
 * @param[out] timedelta time difference since firmware started running (measured by internal hardware on slice)
 * @return Error Code
 */
CREDOAPI CredoError_t cr_time_system(CredoSlice_t* slice, double* timedelta);

#ifdef __cplusplus
}
#endif

#endif
