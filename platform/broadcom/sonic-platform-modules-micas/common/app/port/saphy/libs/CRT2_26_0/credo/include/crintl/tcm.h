#ifndef CRI_TCM_H
#define CRI_TCM_H

#include "crintl/driver.h"

typedef enum {
    CrIntlTCMBurst16Bit = 0,
    CrIntlTCMBurst32Bit = 1,
    CrIntlTCMBurst48Bit = 2,
    CrIntlTCMBurst64Bit = 3,
    CrIntlTCMBurstUnknown = 0xFF
} CrIntlTCMBurstWidth_t;

#ifdef __cplusplus
extern "C" {
#endif

CREDOAPI CredoError_t cri_tcm_get_base_address(CredoSlice_t* slice, const CrIntlRegHive_t* reghive,
                                               unsigned* base_addr);
CREDOAPI CredoError_t cri_tcm_get_burst_width(CredoSlice_t* slice, unsigned address, CrIntlTCMBurstWidth_t* width);

#ifdef __cplusplus
}
#endif

#endif  // CRI_TCM_H
