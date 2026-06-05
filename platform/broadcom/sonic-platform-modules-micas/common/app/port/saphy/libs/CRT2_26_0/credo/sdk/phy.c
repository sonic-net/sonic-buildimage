#include "dii.h"

CredoError_t cr_phy_configure(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed,
                              uint32_t flags) {
    LOGS_API("lane %d, lane_mode %d, speed %u", lane, lane_mode, speed);
    if (slice->hal->hal_fw_config_phy != NULL) {
        CALL_HAL(slice, hal_fw_config_phy(slice, lane, lane_mode, speed, flags));
    } else {
        return cr_lane_configure_mode(slice, lane, lane_mode, speed);
    }
}

CredoError_t cr_phy_configure_shallow_retimer(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed,
                                              uint32_t flags) {
    LOGS_API("lane %d, lane_mode %d, speed %u, flags 0x%08X", lane, lane_mode, speed, flags);
    if (slice->hal->hal_fw_config_phy != NULL) {
        CALL_HAL(slice, hal_fw_config_phy(slice, lane, lane_mode, speed, flags | CR_LFLAG_LOOPBACK));
    } else {
        return cr_lane_configure_mode_loopback(slice, lane, speed);
    }
}

CredoError_t cr_phy_destroy(CredoSlice_t* slice, int lane) {
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fw_deconfig_lane(slice, lane));
}

CredoError_t cr_lane_configure_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed) {
    LOGS_API("lane %d, lane_mode %d, speed %u", lane, lane_mode, speed);
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_fw_config_lane(slice, lane, lane_mode, speed));
}

CredoError_t cr_lane_configure_mode_loopback(CredoSlice_t* slice, int lane, uint32_t speed) {
    LOGS_API("lane %d, speed %u", lane, speed);
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_fw_config_lane_loopback(slice, lane, speed));
}

CredoError_t cr_lane_destroy_mode(CredoSlice_t* slice, int lane) {
    LOGS_API("lane %d", lane);
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_fw_deconfig_lane(slice, lane));
}

CredoError_t cr_phy_is_link_up(CredoSlice_t* slice, int lane, bool* up) {
    unsigned ready = 0;
    CredoError_t err = cr_serdes_get_phy_ready(slice, lane, &ready);
    *up = ready;
    return err;
}
