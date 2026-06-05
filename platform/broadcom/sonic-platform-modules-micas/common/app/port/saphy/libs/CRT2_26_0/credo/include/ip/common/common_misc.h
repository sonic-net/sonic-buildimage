#ifndef COMMON_MISC_H
#define COMMON_MISC_H

#include "sdk.h"

CredoError_t common_set_lane_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config);
CredoError_t common_get_lane_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config);

CredoError_t common_slice_get_oui(CredoSlice_t* slice, unsigned* oui);
CredoError_t common_slice_get_model_number(CredoSlice_t* slice, unsigned* model_number);
CredoError_t common_slice_get_revision_number(CredoSlice_t* slice, unsigned* revision_number);

CredoError_t common_slice_load_setup(CredoSlice_t* slice, const char* file_path);
CredoError_t common_slice_get_reghive(CredoSlice_t* slice, const char* hivename, const RegHive_t** reghive);
CredoError_t common_slice_get_reghive_count(CredoSlice_t* slice, unsigned* count);
CredoError_t common_slice_index_reghive(CredoSlice_t* slice, int index, const RegHive_t** reghive);

CredoError_t common_slice_get_sram_status(CredoSlice_t* slice, CredoSramStatus_t* sram_status);
CredoError_t common_slice_generate_sram_error(CredoSlice_t* slice, CredoSramStatus_t sram_status);
CredoError_t common_slice_get_slice_id(CredoSlice_t* slice, int dummy, int* value);

CredoError_t common_option_get_fw_freeze(CredoSlice_t* slice, const char* name, int* value);
CredoError_t common_option_set_fw_freeze(CredoSlice_t* slice, const char* name, int value);
CredoError_t common_option_get_fw_cmd_timeout(CredoSlice_t* slice, const char* name, int* value);
CredoError_t common_option_set_fw_cmd_timeout(CredoSlice_t* slice, const char* name, int value);
CredoError_t common_option_get_fw_unload_timeout(CredoSlice_t* slice, const char* name, int* value);
CredoError_t common_option_set_fw_unload_timeout(CredoSlice_t* slice, const char* name, int value);
CredoError_t common_get_fw_cmd_timeout(CredoSlice_t* slice, int dummy, int* value);
CredoError_t common_set_fw_cmd_timeout(CredoSlice_t* slice, int dummy, int value);

CredoError_t common_option_get_top_cal_timeout(CredoSlice_t* slice, const char* name, int* value);
CredoError_t common_option_set_top_cal_timeout(CredoSlice_t* slice, const char* name, int value);
CredoError_t common_get_top_cal_timeout(CredoSlice_t* slice, int dummy, int* value);
CredoError_t common_set_top_cal_timeout(CredoSlice_t* slice, int dummy, int value);

CredoError_t common_option_get_fw_spiflash_load_timeout(CredoSlice_t* slice, const char* name, int* value);
CredoError_t common_option_set_fw_spiflash_load_timeout(CredoSlice_t* slice, const char* name, int value);
CredoError_t common_get_fw_spiflash_load_timeout(CredoSlice_t* slice, int dummy, int* value);
CredoError_t common_set_fw_spiflash_load_timeout(CredoSlice_t* slice, int dummy, int value);

const char* common_addr_stringify(CredoSlice_t* slice, int address);
#endif  // COMMON_MISC_H
