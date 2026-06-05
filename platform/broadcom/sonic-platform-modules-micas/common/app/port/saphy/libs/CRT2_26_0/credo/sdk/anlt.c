#include "dii.h"

#include <inttypes.h>

CredoError_t cr_autoneg_set_pages(CredoSlice_t* slice, int lane, int pageId, uint64_t page) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, pageId %d, page %" PRIu64, lane, pageId, page);
    CALL_HAL(slice, hal_set_autoneg_pages(slice, lane, pageId, page));
}

CredoError_t cr_autoneg_get_exchanged_pages(CredoSlice_t* slice, int lane, int* page_count,
                                            uint64_t transmitted_pages[9], uint64_t received_pages[9]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_autoneg_exchanged_pages(slice, lane, page_count, transmitted_pages, received_pages));
}

CredoError_t cr_link_training_get_status(CredoSlice_t* slice, int lane, CredoLinkTrainingStatus_t* status) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_link_training_get_status(slice, lane, status));
}

CredoError_t cr_link_training_get_state(CredoSlice_t* slice, int lane, CredoLinkTrainingState_t* state) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_link_training_get_state(slice, lane, state));
}

CredoError_t cr_autoneg_get_state(CredoSlice_t* slice, int lane, CredoAutoNegState_t* state) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_autoneg_get_state(slice, lane, state));
}

CredoError_t cr_autoneg_get_restart_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_autoneg_get_restart_count(slice, lane, count));
}

CredoError_t cr_link_training_get_restart_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_link_training_get_restart_count(slice, lane, count));
}
