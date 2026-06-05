#ifndef CRI_DRIVER_H
#define CRI_DRIVER_H

#include "credo.h"

typedef struct {
#ifdef SWIG
%immutable;
#endif
    const char* hivename;
    unsigned base_addr;
    int base_shift;
    int shift;
    int count;
    const void* handler;
#ifdef SWIG
%mutable;
#endif
} CrIntlRegHive_t;

#ifdef __cplusplus
extern "C" {
#endif

CREDOAPI CredoError_t cri_slice_get_reghive(CredoSlice_t* slice, const char* hivename, CrIntlRegHive_t* reghive);
CREDOAPI unsigned cri_slice_get_addr(CredoSlice_t* slice, int lane, const CrIntlRegHive_t* reghive,
                                     unsigned int offset);

CREDOAPI CredoError_t cri_slice_index_reghive(CredoSlice_t* slice, int index, CrIntlRegHive_t* reghive);

CREDOAPI CredoError_t cri_slice_get_reghive_count(CredoSlice_t* slice, unsigned* count);

CREDOAPI CredoError_t cri_slice_read_directly(CredoSlice_t* slice, unsigned address, unsigned* value);
CREDOAPI CredoError_t cri_slice_write_directly(CredoSlice_t* slice, unsigned address, unsigned value);

#ifdef __cplusplus
}
#endif

#endif  // CRI_DRIVER_H
