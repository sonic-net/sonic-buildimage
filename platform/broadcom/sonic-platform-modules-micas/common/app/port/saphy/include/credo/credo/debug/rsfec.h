#ifndef CR_DBG_RSFEC_H
#define CR_DBG_RSFEC_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/* RS-FEC register (32/64-bit) read/write. Address=fec_base_addr(port, side)+offset*4.
 * For 64-bit registers, MSB are in register DATA_HI. */

/**
 * @brief
 * @param[in] slice
 * @param[in] portId
 * @param[in] side
 * @param[in] offset
 * @param[out] val
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_rsfec_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                    unsigned* val);

/**
 * @brief
 * @param[in] slice
 * @param[in] portId
 * @param[in] side
 * @param[in] offset
 * @param[in] val
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_rsfec_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                     unsigned val);

#ifdef __cplusplus
}
#endif

#endif
