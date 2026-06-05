#include "project.h"

#include "fec_analyzer/fec_analyzer.h"

#include "utility.h"
#include "sdk.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>

#if HAL_SUPPORT_FEC_ANA

// convert prbs pattern to fec index
static int convert_prbs_pattern_mode_to_fec_index(CredoSlice_t* slice, CredoLanePrbsPattern_t pattern) {
    switch (pattern) {
        case CR_PRBS31:
            return 3;
        case CR_PRBS15:
            return 2;
        case CR_PRBS13:
            return 1;
        case CR_PRBS9:
            return 0;
        default:
            LOGS_INFO("[fec analyzer] prbs type unknown, assuming prbs31");
            return 3;
    }
}

CredoError_t fec_analyzer_set_mux(CredoSlice_t* slice, int lane, int input_src) {
    return writeRegLane(slice, lane, REG_FECANA_SRC_SEL, input_src);
}

CredoError_t fec_analyzer_get_mux(CredoSlice_t* slice, int lane, int* input_src) {
    return readRegLane(slice, lane, REG_FECANA_SRC_SEL, (unsigned*)input_src);
}

CredoError_t fec_analyzer_set_config(CredoSlice_t* slice, int lane, int enable, CredoFecAnalyzerConfig_t* config) {
    if (!enable) {
        return writeRegLane(slice, lane, REG_FECANA_CONTROL, FECANA_CONTROL_OFF);
    }

    // check what the prbs pattern is for the lane
    int prbs_enable;
    CredoLanePrbsPattern_t pattern;
    ERR_PROPS(hal_get_rx_prbs(slice, lane, &prbs_enable, &pattern));

    if (!prbs_enable) {
        pattern = CR_PRBS_UNKNOWN;
    }

    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CLK_EN, 0x1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_EN, 0x0));

    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_SETUP, FECANA_SETUP_START1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_SETUP, FECANA_SETUP_START2));

    ERR_PROP_SLICE(
        slice, writeRegLane(slice, lane, REG_FECANA_PRBS_MODE, convert_prbs_pattern_mode_to_fec_index(slice, pattern)));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_SYMBOL_SIZE, config->symbol_size));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CODEWORD_SIZE, config->codeword_size));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_THRES1, config->threshold));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_THRES2, config->threshold));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_TEI_TYPE, config->error_type));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_TEO_TYPE, config->error_type));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_AUTO_SYNC_EN, 1));

    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CNT_CLR, 0x1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CNT_CLR, 0x0));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_RESET_HIS, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_RESET_HIS, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_EN, 0x1));

    get_time(&slice->data->prbs_fec_timer[lane]);
    return CR_OK;
}

CredoError_t fec_analyzer_get_config(CredoSlice_t* slice, int lane, int* enable, CredoFecAnalyzerConfig_t* config) {
    unsigned reg;
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_CONTROL, &reg));

    *enable = ((reg & FECANA_CONTROL_RUN) != FECANA_CONTROL_RUN) ? 0 : 1;

    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_SYMBOL_SIZE, &reg));
    if (config) config->symbol_size = reg;
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_CODEWORD_SIZE, &reg));
    if (config) config->codeword_size = reg;
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_THRES1, &reg));
    if (config) config->threshold = reg;
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_TEI_TYPE, &reg));
    if (config) config->error_type = reg;
    return CR_OK;
}

CredoError_t fec_analyzer_reset(CredoSlice_t* slice, int lane) {
    int fec_enable = 0;
    ERR_PROPS(hal_get_fec_analyzer(slice, lane, &fec_enable, NULL));
    if (!fec_enable) return CR_OK;

    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_AUTO_SYNC_EN, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CNT_CLR, 0x1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_CNT_CLR, 0x0));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_AUTO_SYNC_EN, 0));

    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_EN, 0x0));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_RESET_HIS, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_RESET_HIS, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_EN, 0x1));

    get_time(&slice->data->prbs_fec_timer[lane]);
    return CR_OK;
}

CredoError_t fec_analyzer_get_autosync(CredoSlice_t* slice, int lane, bool* enable) {
    unsigned auto_sync;
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_AUTO_SYNC_EN, &auto_sync));
    *enable = (auto_sync != 0);
    return CR_OK;
}

CredoError_t fec_analyzer_get_read_counter(CredoSlice_t* slice, int lane, int counter_sel, unsigned* counter) {
    unsigned lsb, msb;
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_READ_SEL, counter_sel));
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_READ_DATA, &lsb));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_READ_SEL, counter_sel + 1));
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_READ_DATA, &msb));
    *counter = (msb << 16) + lsb;
    return CR_OK;
}

CredoError_t fec_analyzer_get_counter(CredoSlice_t* slice, int lane, unsigned* pre_fec, unsigned* post_fec) {
    ERR_PROPS(hal_get_fec_analyzer_read_counter(slice, lane, FECANA_COUNTER_TEI, pre_fec));
    ERR_PROPS(hal_get_fec_analyzer_read_counter(slice, lane, FECANA_COUNTER_TEO, post_fec));
    return CR_OK;
}

CredoError_t fec_analyzer_set_hist_group(CredoSlice_t* slice, int lane, int group) {
    // toggle fecana_en will reset tei, teo counter too.
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_EN, 0x0));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_N_HIS, group));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_RESET_HIS, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_RESET_HIS, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_FECANA_EN, 0x1));
    return CR_OK;
}
CredoError_t fec_analyzer_get_hist_group(CredoSlice_t* slice, int lane, int* group) {
    // toggle fecana_en will reset tei, teo counter too.
    ERR_PROPS(readRegLane(slice, lane, REG_FECANA_N_HIS, (unsigned*)group));
    return CR_OK;
}

CredoError_t fec_analyzer_get_hist_counter(CredoSlice_t* slice, int lane, unsigned hist_data[4]) {
    for (int i = 0; i < 4; i++) {
        ERR_PROPS(hal_get_fec_analyzer_read_counter(slice, lane, FECANA_COUNTER_HIST(i), hist_data + i));
    }
    return CR_OK;
}

CredoError_t fec_analyzer_get_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms) {
    if (duration_ms == NULL || lane < 0 || lane >= MAX_CREDO_LANES) return CR_INVALID_ARGS;
    *duration_ms = us_passed(&slice->data->prbs_fec_timer[lane]) / 1000;
    return CR_OK;
}

CredoError_t fec_analyzer_get_error_rate(CredoSlice_t* slice, int lane, int counter_sel, int duration_ms,
                                         double* error_rate) {
    unsigned start_count = 0;
    unsigned long duration_ms_l = duration_ms;

    CredoFecAnalyzerConfig_t fec_config;
    int enable;
    ERR_PROPS(hal_get_fec_analyzer(slice, lane, &enable, &fec_config));

    if (!enable) {
        LOGS_ERROR("Lane %d fec analyzer is not configured", lane);
        return CR_FAIL;
    }

    // 0 ms duration means use global timer
    if (duration_ms_l > 0) {
        ERR_PROPS(hal_get_fec_analyzer_read_counter(slice, lane, counter_sel, &start_count));
        sleep_ms(duration_ms_l);
    } else {
        ERR_PROPS(hal_get_fec_analyzer_duration(slice, lane, &duration_ms_l));
    }

    unsigned end_count = 0;
    ERR_PROPS(hal_get_fec_analyzer_read_counter(slice, lane, counter_sel, &end_count));

    ERR_PROPS(
        fec_analyzer_calculate_error_rate(slice, lane, counter_sel, duration_ms_l, error_rate, start_count, end_count));
    return CR_OK;
}

// helper function that can be used in places such as rx_monitor to perform the error rate calculation
CredoError_t fec_analyzer_calculate_error_rate(CredoSlice_t* slice, int lane, int counter_sel,
                                               unsigned long duration_ms, double* error_rate, unsigned start_count,
                                               unsigned end_count) {
    uint32_t lane_speed_kbps;

    CredoFecAnalyzerConfig_t fec_config = {0};
    int enable;
    ERR_PROPS(hal_get_fec_analyzer(slice, lane, &enable, &fec_config));

    if (!enable) {
        *error_rate = NAN;
        return CR_OK;
    }

    ERR_PROPS(hal_fw_get_lane_speed(slice, lane, &lane_speed_kbps));

    if (lane_speed_kbps == 0) {
        *error_rate = NAN;
        return CR_OK;
    }
    unsigned group_size = 1;
    CredoFecErrorType_t error_type;

    if (counter_sel == FECANA_COUNTER_SEI) {
        group_size = fec_config.symbol_size;
    } else if (counter_sel == FECANA_COUNTER_BEI) {
        group_size = 1;
    } else {
        // check the error_type being used
        ERR_PROPS(readRegLane(slice, lane, REG_FECANA_TEI_TYPE, &error_type));

        switch (error_type) {
            case CR_FEC_ERROR_FRAME:
                group_size = fec_config.codeword_size;
                break;
            case CR_FEC_ERROR_SYMBOL:
                group_size = fec_config.symbol_size;
            case CR_FEC_ERROR_BIT:
                group_size = 1;
            default:
                break;
        }
    }

    if (group_size == 0) {
        LOGS_WARN("Invalid symbol/codeword size");
        *error_rate = NAN;
        return CR_OK;
    }

    // prevent divide by zero errors
    if (duration_ms == 0) {
        duration_ms = 1;
    }

    if (end_count < start_count) {
        *error_rate =
            (double)(UINT_MAX - start_count + end_count + 1) * group_size / ((double)duration_ms * lane_speed_kbps);
    } else {
        *error_rate = (double)(end_count - start_count) * group_size / ((double)duration_ms * lane_speed_kbps);
    }
    return CR_OK;
}

#endif  // HAL_SUPPORT_FEC_ANA
