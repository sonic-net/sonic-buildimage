#ifndef BE_FW_HELPER_H
#define BE_FW_HELPER_H

#include "be_fw_config.h"

#include "sdk.h"

const char* be_fw_speed_string(FirmwareSpeed_t speed);
unsigned be_fw_speed_kbps(FirmwareSpeed_t speed_idx);
const char* be_fw_config_mode_string(unsigned firmware_mode);
CredoError_t be_fw_translate_config_mode_reverse(unsigned firmware_mode, CredoPortConnectionMode_t* mode);
CredoError_t be_fw_translate_lane_speed(unsigned speed, FirmwareSpeed_t* fw_speed);
CredoError_t be_fw_translate_lane_speed_reverse(FirmwareSpeed_t fw_speed, unsigned* speed);

#endif
