#include "dii.h"

CredoError_t cr_prbs_get_tx_generator(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_tx_prbs(slice, lane, enable, mode));
}

CredoError_t cr_prbs_get_rx_checker(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_prbs(slice, lane, enable, mode));
}

CredoError_t cr_prbs_set_tx_generator(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d, mode %d", lane, enable, mode);
    CALL_HAL(slice, hal_set_tx_prbs(slice, lane, enable, mode));
}
CredoError_t cr_prbs_set_rx_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d, mode %d", lane, enable, mode);
    CALL_HAL(slice, hal_set_rx_prbs_nrz(slice, lane, enable, mode));
}

CredoError_t cr_prbs_set_rx_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d, mode %d", lane, enable, mode);
    CALL_HAL(slice, hal_set_rx_prbs_pam4(slice, lane, enable, mode));
}
CredoError_t cr_prbs_set_tx_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d, mode %d", lane, enable, mode);
    CALL_HAL(slice, hal_set_tx_prbs_nrz(slice, lane, enable, mode));
}
CredoError_t cr_prbs_set_tx_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d, mode %d", lane, enable, mode);
    CALL_HAL(slice, hal_set_tx_prbs_pam4(slice, lane, enable, mode));
}

CredoError_t cr_prbs_set_rx_checker(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d, mode %d", lane, enable, mode);
    CALL_HAL(slice, hal_set_rx_prbs(slice, lane, enable, mode));
}
CredoError_t cr_prbs_get_rx_count(CredoSlice_t* slice, int lane, uint32_t* count) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_prbs_count(slice, lane, count));
}

CredoError_t cr_prbs_get_rx_lock(CredoSlice_t* slice, int lane, CredoPrbsLockStatus_t* is_locked) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_prbs_lock(slice, lane, is_locked));
}

CredoError_t cr_prbs_get_rx_ber(CredoSlice_t* slice, int lane, int time_ms, double* ber) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, time_ms %d", lane, time_ms);
    CALL_HAL(slice, hal_get_rx_prbs_ber(slice, lane, time_ms, ber));
}

CredoError_t cr_prbs_get_rx_ber_all(CredoSlice_t* slice, const int lanes[], int time_ms, double ber[], unsigned count) {
    LOGS_API();
    CALL_HAL(slice, hal_prbs_get_rx_ber_all(slice, lanes, time_ms, ber, count));
}

CredoError_t cr_prbs_get_rx_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_rx_prbs_duration(slice, lane, duration_ms));
}

CredoError_t cr_prbs_reset_rx_count(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_reset_rx_prbs_count(slice, lane));
}

CredoError_t cr_prbs_generate_tx_error(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_generate_tx_prbs_error(slice, lane));
}

CredoError_t cr_fecana_get_autosync(CredoSlice_t* slice, int lane, bool* enabled) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fecana_get_autosync(slice, lane, enabled));
}

CredoError_t cr_prbs_get_rx_autosync(CredoSlice_t* slice, int lane, bool* enabled) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_prbs_get_rx_autosync(slice, lane, enabled));
}

CredoError_t cr_fecana_configure(CredoSlice_t* slice, int lane, int enable, CredoFecAnalyzerConfig_t* config) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d", lane, enable);
    CALL_HAL(slice, hal_set_fec_analyzer(slice, lane, enable, config));
}

CredoError_t cr_fecana_query(CredoSlice_t* slice, int lane, int* enable, CredoFecAnalyzerConfig_t* config) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_fec_analyzer(slice, lane, enable, config));
}

CredoError_t cr_fecana_reset(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fecana_reset(slice, lane));
}

CredoError_t cr_fecana_get_raw_counter(CredoSlice_t* slice, int lane, int counter_sel, unsigned* counter) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, counter_sel %d", lane, counter_sel);
    CALL_HAL(slice, hal_get_fec_analyzer_read_counter(slice, lane, counter_sel, counter));
}

CredoError_t cr_fecana_get_counter(CredoSlice_t* slice, int lane, unsigned* pre_fec, unsigned* post_fec) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_fec_analyzer_counter(slice, lane, pre_fec, post_fec));
}

CredoError_t cr_fecana_set_hist_group(CredoSlice_t* slice, int lane, int group) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, group %d", lane, group);
    CALL_HAL(slice, hal_set_fec_analyzer_hist_group(slice, lane, group));
}

CredoError_t cr_fecana_get_hist_group(CredoSlice_t* slice, int lane, int* group) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_fecana_get_hist_group(slice, lane, group));
}

CredoError_t cr_fecana_get_hist_counter(CredoSlice_t* slice, int lane, unsigned hist_data[4]) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_fec_analyzer_hist_counter(slice, lane, hist_data));
}

CredoError_t cr_fecana_get_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_get_fec_analyzer_duration(slice, lane, duration_ms));
}

CredoError_t cr_fecana_get_error_rate(CredoSlice_t* slice, int lane, int counter_sel, int duration_ms,
                                      double* error_rate) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, counter_sel %d, duration_ms %d", lane, counter_sel, duration_ms);
    CALL_HAL(slice, hal_get_fec_analyzer_error_rate(slice, lane, counter_sel, duration_ms, error_rate));
}

CredoError_t cr_prbs_training_rx_get_status(CredoSlice_t* slice, int lane, CredoPrbsTrainingStatus_t* status) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_prbs_training_rx_get_status(slice, lane, status));
}

CredoError_t cr_prbs_training_rx_relink(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_prbs_training_rx_relink(slice, lane));
}

CredoError_t cr_prbs_training_tx_enable(CredoSlice_t* slice, int lane, bool enable) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d, enable %d", lane, enable);
    CALL_HAL(slice, hal_prbs_training_tx_enable(slice, lane, enable));
}

CredoError_t cr_prbs_training_tx_is_enabled(CredoSlice_t* slice, int lane, bool* enable) {
    CHECK_LANE_VALID(slice, lane);
    LOGS_API("lane %d", lane);
    CALL_HAL(slice, hal_prbs_training_tx_is_enabled(slice, lane, enable));
}

CredoError_t cr_prbs_pattchecker_set_symbol(CredoSlice_t* slice, int lane, bool en, unsigned symbol) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_pattern_rx_set_prev(slice, lane, en, symbol));
}
CredoError_t cr_prbs_pattchecker_reset_count(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_pattern_rx_reset_count(slice, lane));
}
CredoError_t cr_prbs_pattchecker_get_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_pattern_rx_get_count(slice, lane, pattern_count));
}
CredoError_t cr_prbs_pattchecker_set_phase(CredoSlice_t* slice, int lane, unsigned phase) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_pattern_rx_set_phase(slice, lane, phase));
}

CredoError_t cr_prbs_get_tx_checker(CredoSlice_t* slice, int lane, int* enable, CredoPrbsPattern_t* mode) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_get_tx_checker(slice, lane, enable, mode));
}

CredoError_t cr_prbs_set_tx_checker(CredoSlice_t* slice, int lane, int enable, CredoPrbsPattern_t mode) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_set_tx_checker(slice, lane, enable, mode));
}

CredoError_t cr_prbs_get_tx_count(CredoSlice_t* slice, int lane, uint32_t count[2]) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_get_tx_count(slice, lane, count));
}

CredoError_t cr_prbs_get_tx_ber(CredoSlice_t* slice, int lane, int time_ms, double ber[2]) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_get_tx_ber(slice, lane, time_ms, ber));
}

CredoError_t cr_prbs_get_tx_ber_all(CredoSlice_t* slice, const int lanes[], int time_ms, double ber[][2],
                                    unsigned count) {
    for (size_t i = 0; i < count; i++) {
        CHECK_LANE_VALID(slice, lanes[i]);
    }
    CALL_HAL(slice, hal_prbs_get_tx_ber_all(slice, lanes, time_ms, ber, count));
}

CredoError_t cr_prbs_get_tx_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_get_tx_duration(slice, lane, duration_ms));
}

CredoError_t cr_prbs_reset_tx_count(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_prbs_reset_tx_count(slice, lane));
}
