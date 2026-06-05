#ifndef SE_FW_HELPER_H
#define SE_FW_HELPER_H

#include "se_device.h"
#include "se_fw_config.h"

#include "sdk.h"

const char* se_fw_speed_string(FirmwareSpeed_t speed);
const char* se_fw_config_mode_string(unsigned firmware_mode);
const char* se_fw_port_direction_string(unsigned direction);
unsigned se_fw_speed_kbps(FirmwareSpeed_t speed_index);
unsigned se_fw_translate_speed(unsigned speed);
const char* se_fw_state_mode_string(unsigned state_mode);
const char* se_fw_state_major_string(unsigned state_major);
const char* se_fw_state_minor_string(unsigned state_minor);
CredoError_t se_fw_translate_multilane(unsigned multilane, unsigned* multilane_code);
CredoError_t se_fw_translate_multilane_reverse(unsigned multilane_code, unsigned* multilane);
CredoError_t se_fw_translate_config_mode_reverse(unsigned firmware_mode, CredoPortConnectionMode_t* mode);
CredoError_t se_fw_translate_lane_speed_reverse(FirmwareSpeed_t fw_speed, unsigned* speed);
CredoError_t se_fw_translate_state_reverse(unsigned fw_lt_state, CredoLinkTrainingState_t* lt_state);
CredoError_t se_fw_translate_flags(const SePortInfo_t* info, unsigned* detail2);

#endif
