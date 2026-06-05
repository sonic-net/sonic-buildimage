#ifndef BE_OPTION_H
#define BE_OPTION_H

#include "sdk.h"

CredoError_t be_option_get_low_vaa(CredoSlice_t* slice, const char* name, int* value);
CredoError_t be_option_set_low_vaa(CredoSlice_t* slice, const char* name, int value);

CredoError_t be_lane_option_get_fast_recover_timeout(CredoSlice_t* slice, int lane, const char* name, int* value);
CredoError_t be_lane_option_set_fast_recover_timeout(CredoSlice_t* slice, int lane, const char* name, int value);
CredoError_t be_lane_option_get_sd_delay(CredoSlice_t* slice, int lane, const char* name, int* value);
CredoError_t be_lane_option_set_sd_delay(CredoSlice_t* slice, int lane, const char* name, int value);

#endif
