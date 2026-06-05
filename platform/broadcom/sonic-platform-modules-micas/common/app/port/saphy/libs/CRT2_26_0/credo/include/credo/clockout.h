#ifndef CREDO_CLOCKOUT_H
#define CREDO_CLOCKOUT_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

// Clock Output

/**
 * @brief Enable clock output
 *
 * Please refer to chip specs to find out available cloock output. Use a lane clock signal to generate a reference clock
 * output.
 * @param slice
 * @param clock_output index of clock output. chip dependent on what clock outputs are available and if they are
 * single-ended or differential
 * @param lane lane to use to generate clock output
 * @param divider divider to use from lane signal to reduce clock output frequency
 * @return Error Code
 */
CREDOAPI CredoError_t cr_clockout_enable(CredoSlice_t* slice, unsigned clock_output, unsigned lane, unsigned divider);

/**
 * @brief Disable all clock output generation
 * @param slice slice to disable clock output
 * @return Error Code
 */
CREDOAPI CredoError_t cr_clockout_disable_all(CredoSlice_t* slice);

/**
 * @brief Disable clock output generation for one clock
 * @param slice slice to disable clock output
 * @param clock_output clock output to disable
 * @return Error Code
 */
CREDOAPI CredoError_t cr_clockout_disable(CredoSlice_t* slice, unsigned clock_output);

#ifdef __cplusplus
}
#endif

#endif
