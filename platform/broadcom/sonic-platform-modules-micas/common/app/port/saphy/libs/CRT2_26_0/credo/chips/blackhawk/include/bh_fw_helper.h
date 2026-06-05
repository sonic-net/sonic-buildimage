#ifndef BH_FW_HELPER_H
#define BH_FW_HELPER_H

#include "bh_fw_config.h"

#include "sdk.h"

const char* bh_fw_speed_string(FirmwareSpeed_t speed);
unsigned bh_fw_speed_kbps(FirmwareSpeed_t speed_idx);
const char* bh_fw_config_mode_string(unsigned config_sel);
const char* bh_fw_gearbox_error_string(unsigned err_code);
const char* bh_fw_gearbox_state_string(unsigned state);
CredoError_t bh_fw_translate_multilane(unsigned multilane, unsigned* multilane_code);
CredoError_t bh_fw_translate_multilane_reverse(unsigned multilane_code, unsigned* multilane);
CredoError_t bh_fw_translate_lane_speed(unsigned speed, FirmwareSpeed_t* fw_speed);
CredoError_t bh_fw_translate_lane_speed_reverse(FirmwareSpeed_t fw_speed, unsigned* speed);
CredoError_t bh_fw_translate_config_mode_reverse(unsigned firmware_mode, CredoPortConnectionMode_t* mode);
CredoError_t bh_fw_translate_FEC(CredoFecType_t fec, FirmwareFecType_t* fw_fec);
CredoError_t bh_fw_translate_FEC_reverse(FirmwareFecType_t fw_fec, CredoFecType_t* fec);

#endif
