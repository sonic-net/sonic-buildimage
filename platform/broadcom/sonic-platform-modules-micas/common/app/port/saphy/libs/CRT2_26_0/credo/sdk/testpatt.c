#include "dii.h"

#include <inttypes.h>

// Test pattern
CredoError_t cr_testpatt_set_tx_enable(CredoSlice_t* slice, int lane, bool enable) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d", lane, enable);
    CALL_HAL(slice, hal_set_tx_test_pattern_enable(slice, lane, enable));
}

CredoError_t cr_testpatt_get_tx_enable(CredoSlice_t* slice, int lane, bool* enable) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_test_pattern_enable(slice, lane, enable));
}

CredoError_t cr_testpatt_set_tx_memory(CredoSlice_t* slice, int lane, uint64_t pattern) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, pattern %" PRIu64, lane, pattern);
    CALL_HAL(slice, hal_set_tx_test_pattern_memory(slice, lane, pattern));
}

CredoError_t cr_testpatt_get_tx_memory(CredoSlice_t* slice, int lane, uint64_t* pattern) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_test_pattern_memory(slice, lane, pattern));
}

CredoError_t cr_testpatt_set_tx_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, mode %d", lane, mode);
    CALL_HAL(slice, hal_set_tx_test_pattern_mode(slice, lane, mode));
}

CredoError_t cr_testpatt_get_tx_mode(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode* mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_test_pattern_mode(slice, lane, mode));
}
