#ifndef COMMON_EFUSE_H
#define COMMON_EFUSE_H

#include "project.h"

#include "utility.h"
#include "sdk.h"

#ifdef EFUSE_BASE_REG
CredoError_t common_efuse_read_bank(CredoSlice_t *slice, int bank, uint32_t *value);
CredoError_t common_efuse_read_ecid(CredoSlice_t *slice, uint32_t ecid[2]);

#endif

#endif
