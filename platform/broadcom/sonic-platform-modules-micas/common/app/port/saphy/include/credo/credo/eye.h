#ifndef CREDO_EYE_H
#define CREDO_EYE_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Eye monitor flags
 *
 */
typedef enum {
    CR_EYE_MONITOR_DESTRUCTIVE = (1 << 0),  //!< destructive mode
    CR_EYE_MONITOR_BATHTUB = (1 << 1),      //!< bathtub mode
} CredoEyeMonitorFlags_t;

/**
 * @brief Start eye monitor
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[in] ber_exp
 * @param[in] flag
 * @return Error Code
 */
CREDOAPI CredoError_t cr_eye_monitor_start(CredoSlice_t* slice, int lane, int ber_exp, int flag);

/**
 * @brief Stop eye monitor
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @return Error Code
 */
CREDOAPI CredoError_t cr_eye_monitor_stop(CredoSlice_t* slice, int lane);

/**
 * @brief Get eye monitor progress
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] percent percent 100 means data ready
 * @return Error Code
 */
CREDOAPI CredoError_t cr_eye_monitor_get_progress(CredoSlice_t* slice, int lane, int* percent);

/**
 * @brief Get eye monitor data
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] data eye monitor data
 * @param[out] extent_mv vertical scale
 * @return Error Code
 */
CREDOAPI CredoError_t cr_eye_monitor_get_data(CredoSlice_t* slice, int lane, int** data, int* extent_mv);

typedef struct {
    int** data;     //!< 2d array of data (single malloc)
    size_t rows;    //!< number of rows
    size_t cols;    //!< number of columns
    int extent_mv;  //!< vertical scale
} CredoEyeData_t;

/**
 * @brief Capture eye data on the heap
 *
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[in,out] data eye data. Must free data
 * @return Error Code
 */
CREDOAPI CredoError_t cr_eye_monitor_get_datah(CredoSlice_t* slice, int lane, CredoEyeData_t* data);

#define CR_EYE_DATA_FREE(d) (free((d).data))

/**
 * @brief Get eye monitor vertical and horizontal range
 * @param[in] slice slice handle
 * @param[in] lane lane to select
 * @param[out] vstep_side vstep_side is defined in firmware
 * @param[out] hstep_side hstep_side is defined in firmware, return 0 if bathtub mode
 * @return Error Code
 */
CREDOAPI CredoError_t cr_eye_monitor_get_range(CredoSlice_t* slice, int lane, int* vstep_side, int* hstep_side);

/**
 * @brief Get eye monitor separator
 * @param[in] slice slice handle
 * @param[out] separator vstep_separator is defined in firmware
 * @return Error Code
 */
CREDOAPI CredoError_t cr_eye_monitor_get_separator(CredoSlice_t* slice, int separator[5]);

#ifdef __cplusplus
}
#endif

#endif
