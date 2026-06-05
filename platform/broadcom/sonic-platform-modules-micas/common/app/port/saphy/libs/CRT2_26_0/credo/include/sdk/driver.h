#ifndef DRIVER_H
#define DRIVER_H

#include "sdk/driver_regs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* device read/write */

CREDOAPI CredoError_t cr_slice_broadcast_burst_write(CredoSlice_t* slice, unsigned first_address,
                                                     const unsigned value[], unsigned count);
CREDOAPI CredoError_t cr_slice_broadcast_write(CredoSlice_t* slice, unsigned reg, const unsigned value);

CREDOAPI CredoError_t cr_write_reg(CredoSlice_t* slice, int lane, const RegHive_t* hive, unsigned offset, unsigned mask,
                                   int lsb, unsigned value);
CREDOAPI unsigned cr_addr_reg(CredoSlice_t* slice, int lane, const RegHive_t* hive, unsigned int offset);
CREDOAPI CredoError_t cr_read_reg(CredoSlice_t* slice, int lane, const RegHive_t* hive, unsigned offset, unsigned mask,
                                  int lsb, unsigned* value);
CREDOAPI CredoError_t cr_read_reg_signed(CredoSlice_t* slice, int lane, const RegHive_t* hive, unsigned offset,
                                         unsigned mask, int msb, int lsb, int* value);

/* TCM read/write */
CREDOAPI CredoError_t cr_read_tcm_reg(CredoSlice_t* slice, const RegHive_t* hive, unsigned offset, unsigned mask,
                                      int lsb, unsigned* value);
CREDOAPI CredoError_t cr_write_tcm_reg(CredoSlice_t* slice, const RegHive_t* hive, unsigned offset, unsigned mask,
                                       int lsb, unsigned value);

#ifdef __cplusplus
}
#endif

#endif
