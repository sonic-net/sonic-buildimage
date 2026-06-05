#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"

CredoError_t se_frecov_configure(CredoSlice_t* slice, int lane, unsigned timeout_ms) {
    ERR_PROPS(se_fw_set_fast_recover_timeout(slice, lane, (int)timeout_ms));
    return CR_OK;
}

CredoError_t se_frecov_get_status(CredoSlice_t* slice, int lane, unsigned* timeout_ms,
                                  CredoFastRecoveryStatus_t* status) {
    CredoLaneMode_t lane_mode = CR_LMODE_OFF;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));

    if (lane_mode == CR_LMODE_OFF) {
        *status = CR_FRECOV_DISABLED;
        *timeout_ms = 0;
        return CR_OK;
    }
    ERR_PROPS(se_fw_get_fast_recover_timeout(slice, lane, (int*)timeout_ms));

    if (*timeout_ms == 0) {
        *status = CR_FRECOV_DISABLED;
        return CR_OK;
    }
    *status = CR_FRECOV_ENABLED;

    unsigned frecov_armed_map = 0;
    ERR_PROPS(readTop(slice, REG_FRECOV_ARMED, &frecov_armed_map));
    if ((frecov_armed_map & (1 << lane)) != 0) {
        *status = CR_FRECOV_ARMED;
    }
    return CR_OK;
}

CredoError_t se_frecov_get_recover_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CredoLaneMode_t lane_mode = CR_LMODE_OFF;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));

    if (lane_mode == CR_LMODE_OFF) {
        *count = 0;
        return CR_OK;
    }
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_SD_COUNT, count));
    return CR_OK;
}
