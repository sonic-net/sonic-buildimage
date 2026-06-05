#ifndef CREDO_PCS_H
#define CREDO_PCS_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PCS register (16-bit) read/write. Address=pcs_base_addr(port, side)+offset*2. */

/**
 * @brief This reads any PCS register of specified port/side.
 *
 * The PCS register map conforms to IEEE 802.3 Clause 45, section 45.2.3.
 *
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Port side selection
 * @param[in] offset PCS register offset
 * @param[out] val Holds value to be returned
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_pcs_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                  unsigned* val);

/**
 * @brief This writes any PCS register of specified port/side.
 *
 * The PCS register map conforms to IEEE 802.3 Clause 45, section 45.2.3.
 *
 * @param[in] slice Slice handle
 * @param[in] portId Port ID
 * @param[in] side Port side selection
 * @param[in] offset PCS register offset
 * @param[in] val Value to be written
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_pcs_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                   unsigned val);
/* PCS status read. Equivalent to read PCS offset 1 (address 0x02). */

/**
 * @brief This reads PCS register 1 (PCS status 1) of specified port/side.
 *
 * See IEEE 802.3 section 45.2.3.2 for PCS status 1. Note the link status (bit 2) is latching low.
 *
 * @param[in] slice
 * @param[in] portId
 * @param[in] side
 * @param[out] pcs_status
 * @return  CredoError_t
 */
CREDOAPI CredoError_t cr_pcs_status_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned* pcs_status);

#ifdef __cplusplus
}
#endif

#endif
