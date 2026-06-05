#ifndef CRI_FIRMWARE_H
#define CRI_FIRMWARE_H

#include "credo.h"

CREDOAPI CredoError_t cri_spiflash_read(CredoSlice_t* slice, unsigned addr, unsigned* buffer, unsigned count);
CREDOAPI CredoError_t cri_spiflash_write(CredoSlice_t* slice, unsigned addr, const unsigned* buffer, unsigned count);
CREDOAPI CredoError_t cri_spiflash_erase(CredoSlice_t* slice, unsigned addr);

#ifdef __cplusplus
}
#endif

#endif  // CRI_DRIVER_H
