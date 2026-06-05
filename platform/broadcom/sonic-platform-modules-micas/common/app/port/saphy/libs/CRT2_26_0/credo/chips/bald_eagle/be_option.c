#include "project.h"
#include "be_device.h"
#include "be_option.h"

#include "canary/canary_serdes.h"

#include <string.h>

CredoError_t be_option_get_low_vaa(CredoSlice_t* slice, const char* name, int* value) {
    int en = 0;

    *value = 0;
    for (int ln = 0; ln < slice->desc->lane_count; ln++) {
        ERR_PROPS(canary_get_low_vaa(slice, ln, &en));
        if (en != 0) return CR_OK;
    }

    *value = 1;
    return CR_OK;
}

CredoError_t be_option_set_low_vaa(CredoSlice_t* slice, const char* name, int value) {
    for (int ln = 0; ln < slice->desc->lane_count; ln++) {
        ERR_PROPS(canary_set_low_vaa(slice, ln, value));
    }
    return CR_OK;
}

CredoError_t be_lane_option_get_fast_recover_timeout(CredoSlice_t* slice, int lane, const char* name, int* value) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_INFO, TOP_INFO_SD_TIMEOUT, (unsigned*)value));
    return CR_OK;
}

CredoError_t be_lane_option_set_fast_recover_timeout(CredoSlice_t* slice, int lane, const char* name, int value) {
    ERR_PROP(writeReg(slice, REG_FW_REG_VALUE, value));
    ERR_PROPS(hal_fw_cmd(slice, FW_CMD_RECOVER_TIMEOUT, 1 << lane, NULL, NULL));
    return CR_OK;
}

CredoError_t be_lane_option_get_sd_delay(CredoSlice_t* slice, int lane, const char* name, int* value) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_INFO, TOP_INFO_SD_DELAY, (unsigned*)value));
    return CR_OK;
}

CredoError_t be_lane_option_set_sd_delay(CredoSlice_t* slice, int lane, const char* name, int value) {
    ERR_PROP(writeReg(slice, REG_FW_REG_VALUE, value));
    ERR_PROPS(hal_fw_cmd(slice, FW_CMD_SD_DELAY, 1 << lane, NULL, NULL));
    return CR_OK;
}
