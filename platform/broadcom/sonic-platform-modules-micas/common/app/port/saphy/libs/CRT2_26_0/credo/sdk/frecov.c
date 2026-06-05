#include "dii.h"

CredoError_t cr_frecov_configure(CredoSlice_t* slice, int lane, unsigned timeout_ms) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_frecov_configure(slice, lane, timeout_ms));
}

CredoError_t cr_frecov_get_status(CredoSlice_t* slice, int lane, unsigned* timeout_ms,
                                  CredoFastRecoveryStatus_t* status) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_frecov_get_status(slice, lane, timeout_ms, status));
}

CredoError_t cr_frecov_get_recover_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_frecov_get_recover_count(slice, lane, count));
}
