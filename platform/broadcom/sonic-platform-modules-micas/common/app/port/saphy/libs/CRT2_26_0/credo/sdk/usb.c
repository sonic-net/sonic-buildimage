#include "dii.h"

CredoError_t cr_usb_phy_configure(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed,
                                  uint32_t flags) {
    LOGS_API("lane %d, lane_mode %d, speed %d", lane, lane_mode, speed);
    // firmware can support other actions when lane is equal to MAX_LANE_PER_SLICE
    CALL_HAL(slice, hal_fw_config_phy(slice, lane, lane_mode, speed, flags));
}

CredoError_t cr_usb_phy_destroy(CredoSlice_t* slice, int lane, uint32_t flags) {
    LOGS_API("lane %d, flags 0x%08X", lane, flags);
    // firmware can support other actions when lane is equal to MAX_LANE_PER_SLICE
    CALL_HAL(slice, hal_fw_deconfig_phy(slice, lane, flags));
}
