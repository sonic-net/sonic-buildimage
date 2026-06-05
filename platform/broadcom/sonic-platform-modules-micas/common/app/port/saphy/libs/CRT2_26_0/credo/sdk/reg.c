#include "dii.h"

#include "sdk.h"

#include <stdlib.h>

static CredoError_t cr_reg_verify_inner(CredoSlice_t* slice, const CredoRegVerifyConfig_t* config,
                                        CredoRegVerifyStats_t* stats) {
    unsigned base_address = 0;
    if (config->flags & CR_REG_VERIFY_OVERRIDE_ADDRESS) {
        base_address = config->address;
    } else {
        ERR_PROPS(hal_reg_get_notepad(slice, &base_address));
    }
    bool is_burst_test = config->flags & CR_REG_VERIFY_BURST;
    unsigned burst_width = 0;
    if (is_burst_test) {
        burst_width = config->burst_width;
    } else {
        burst_width = 1;  // not really a burst, but use the same to make it easy
    }
    if (burst_width == 0) {
        burst_width = 14;
    }
    if (burst_width > 16) {
        LOGS_ERROR("Burst can only support burst up to 16 registers");
        return CR_INVALID_ARGS;
    }
    bool is_duration_test = config->flags & CR_REG_VERIFY_USE_DURATION;
    CredoTime_t start_time = {0};
    get_time(&start_time);
    unsigned test_count = 0;
    unsigned reg_write_values[16] = {0};
    unsigned reg_read_values[16] = {0};

#define REG_TEST_IS_NOT_FINISHED()                                             \
    ((is_duration_test && (sec_passed(&start_time) < config->duration_sec)) || \
     (!is_duration_test && test_count < config->test_count))

    stats->duration_sec = 0;
    stats->fail_count = 0;
    stats->reg_count = 0;
    while (REG_TEST_IS_NOT_FINISHED()) {
        test_count += 1;
        for (size_t i = 0; i < burst_width; i++) {
            reg_write_values[i] = rand() & 0xFFFF;  // NOLINT(cert-msc30-c,cert-msc50-cpp)
        }
        CredoError_t err = cr_slice_burst_write(slice, base_address, reg_write_values, burst_width);
        stats->reg_count += burst_width;
        if (err != CR_OK) {  // driver code indicates error
            stats->fail_count += burst_width;
            continue;
        }
        err = cr_slice_burst_read(slice, base_address, reg_read_values, burst_width);
        if (err != CR_OK) {  // driver code indicates error
            stats->fail_count += burst_width;
            continue;
        }
        for (size_t i = 0; i < burst_width; i++) {
            if (reg_read_values[i] != reg_write_values[i]) {
                stats->fail_count += 1;
            }
        }
    }
    stats->duration_sec = sec_passed(&start_time);
    return CR_OK;
}

CredoError_t cr_reg_verify(CredoSlice_t* slice, const CredoRegVerifyConfig_t* config, CredoRegVerifyStats_t* stats) {
    SLICE_LOCK_GUARD(slice);
    CredoError_t err = cr_reg_verify_inner(slice, config, stats);
    SLICE_UNLOCK(slice);
    return err;
}
