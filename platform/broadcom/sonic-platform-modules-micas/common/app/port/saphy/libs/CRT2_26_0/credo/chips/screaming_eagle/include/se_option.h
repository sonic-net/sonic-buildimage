#ifndef SE_OPTION_H
#define SE_OPTION_H

#include "sdk.h"

CredoError_t se_option_get_toppll_mode(CredoSlice_t* slice, const char* name, int* value);
CredoError_t se_option_set_toppll_mode(CredoSlice_t* slice, const char* name, int value);
CredoError_t se_option_get_fw_em_vstep_side(CredoSlice_t* slice, const char* name, int* value);
CredoError_t se_option_set_fw_em_vstep_side(CredoSlice_t* slice, const char* name, int value);
CredoError_t se_option_get_fw_isi_timeout(CredoSlice_t* slice, const char* name, int* value);
CredoError_t se_option_set_fw_isi_timeout(CredoSlice_t* slice, const char* name, int value);
CredoError_t se_option_get_isc_slice_id(CredoSlice_t* slice, const char* name, int* value);
CredoError_t se_option_set_isc_slice_id(CredoSlice_t* slice, const char* name, int value);

CredoError_t se_lane_option_get_phase_base(CredoSlice_t* slice, int lane, const char* name, int* value);
CredoError_t se_lane_option_set_phase_base(CredoSlice_t* slice, int lane, const char* name, int value);
CredoError_t se_lane_option_get_fast_recover_timeout(CredoSlice_t* slice, int lane, const char* name, int* value);
CredoError_t se_lane_option_set_fast_recover_timeout(CredoSlice_t* slice, int lane, const char* name, int value);
CredoError_t se_lane_option_get_sd_delay(CredoSlice_t* slice, int lane, const char* name, int* value);
CredoError_t se_lane_option_set_sd_delay(CredoSlice_t* slice, int lane, const char* name, int value);

CredoError_t se_lane_option_set_oneshot(CredoSlice_t* slice, int lane, const char* name, int value);
CredoError_t se_lane_option_get_oneshot(CredoSlice_t* slice, int lane, const char* name, int* value);

CredoError_t se_lane_option_set_lt_oneshot(CredoSlice_t* slice, int lane, const char* name, int value);
CredoError_t se_lane_option_get_lt_oneshot(CredoSlice_t* slice, int lane, const char* name, int* value);

CredoError_t se_slice_option_set_lt_timer(CredoSlice_t* slice, const char* name, int value);
CredoError_t se_slice_option_get_lt_timer(CredoSlice_t* slice, const char* name, int* value);
CredoError_t se_slice_option_set_vsensor_res(CredoSlice_t* slice, const char* name, int value);
CredoError_t se_slice_option_get_vsensor_res(CredoSlice_t* slice, const char* name, int* value);

#endif
