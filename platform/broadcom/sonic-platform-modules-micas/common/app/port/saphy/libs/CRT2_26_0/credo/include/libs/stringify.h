#ifndef HELP_H
#define HELP_H

#include "sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LANE_LIST_STR_MAX_LEN 32

#define CASE_ENUM_RETURN_STRING_BASIC(enum, string) \
    case enum:                                      \
        return string
#define CASE_ENUM_RETURN_STRING(enum) CASE_ENUM_RETURN_STRING_BASIC(enum, #enum)

const char* errorcodes_to_string(CredoError_t err);
const char* prbs_pattern_to_string(CredoLanePrbsPattern_t mode);
const char* lane_mode_to_string(CredoLaneMode_t mode);
const char* fec_to_string(CredoFecType_t fec);
CredoError_t port_flags_to_string(uint32_t flags, char* buf, uint32_t size);
void ppm_to_format_string(int32_t ppm, char ppm_str[8]);
void seconds_to_format_string(unsigned sec, char time_str[16]);
void stringify_firmware_version(unsigned version, char version_str[32]);
void stringify_lane_list(int* lane_list, unsigned count, char lane_list_str[LANE_LIST_STR_MAX_LEN]);
const char* stringify_tx_state(CredoLaneTxState_t state);
const char* stringify_tx_loopback_mode(CredoLaneLoopbackMode_t mode);
const char* stringify_lane_id(CredoSlice_t* slice, int lane);

#ifdef __cplusplus
}
#endif

#endif  // HELP_H
