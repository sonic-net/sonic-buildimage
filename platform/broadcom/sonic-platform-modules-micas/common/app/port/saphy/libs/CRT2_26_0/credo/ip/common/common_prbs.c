#include "common/common_prbs.h"

#include "fort.h"
#include "utility.h"
#include "sdk.h"

#include <limits.h>
#include <math.h>

int common_prbs_phase_encode(int code, int pol, int gc, int msblsb) {
    int map_pol[4] = {3, 2, 1, 0};
    int map_msblsb[4] = {0, 2, 1, 3};
    int map_gc[4] = {0, 1, 3, 2};
    if (msblsb == 0) code = map_msblsb[code];
    if (gc == 1) code = map_gc[code];
    if (pol == 1) code = map_pol[code];
    return code;
}

int common_prbs_phase_decode(int code, int pol, int gc, int msblsb) {
    int map_pol[4] = {3, 2, 1, 0};
    int map_msblsb[4] = {0, 2, 1, 3};
    int map_gc[4] = {0, 1, 3, 2};
    if (pol == 1) code = map_pol[code];
    if (gc == 1) code = map_gc[code];
    if (msblsb == 0) code = map_msblsb[code];
    return code;
}

CredoError_t common_get_rx_prbs_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms) {
    if (slice == NULL || duration_ms == NULL || lane < 0 || lane >= MAX_CREDO_LANES) return CR_INVALID_ARGS;
    *duration_ms = us_passed(&slice->data->prbs_timer[lane]) / 1000;
    return CR_OK;
}

CredoError_t common_get_tx_prbs_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms) {
    if (slice == NULL || duration_ms == NULL || lane < 0 || lane >= MAX_CREDO_LANES) return CR_INVALID_ARGS;
    *duration_ms = us_passed(&slice->data->tx_prbs_timer[lane]) / 1000;
    return CR_OK;
}

CredoError_t common_get_rx_prbs_ber(CredoSlice_t* slice, int lane, int time_ms, double* ber) {
    CredoLaneMode_t mode = CR_LMODE_OFF;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    *ber = NAN;
    if (mode == CR_LMODE_DISABLE) {
        return CR_OK;
    } else if (mode != CR_LMODE_NRZ && mode != CR_LMODE_PAM4 && mode != CR_LMODE_PAM3) {
        return CR_OK;
    }

    int prbs_enabled = 0;
    CredoLanePrbsPattern_t prbs_patt;
    ERR_PROPS(hal_get_rx_prbs(slice, lane, &prbs_enabled, &prbs_patt));
    if (prbs_enabled == 0) {
        return CR_OK;
    }

    uint32_t err_cnt_pre = 0, err_cnt_post, speed_kbps;

    if (time_ms < 0) return CR_INVALID_ARGS;
    if (time_ms != 0) {
        ERR_PROPS(hal_get_rx_prbs_count(slice, lane, &err_cnt_pre));
        sleep_ms(time_ms);
    } else {
        time_ms = us_passed(&slice->data->prbs_timer[lane]) / 1000;
        if (time_ms == 0) time_ms = 1;  // avoid ber calculated incorrect
    }

    ERR_PROPS(hal_get_rx_prbs_count(slice, lane, &err_cnt_post));
    ERR_PROPS(hal_fw_get_lane_speed(slice, lane, &speed_kbps));
    if (speed_kbps == 0) {
        LOGS_ERROR("The lane %d is not configured.\n", lane);
        return CR_FAIL;
    }
    if (err_cnt_post < err_cnt_pre) {
        *ber = (UINT_MAX - err_cnt_pre + err_cnt_post + 1) / ((double)time_ms * speed_kbps);
    } else {
        *ber = (err_cnt_post - err_cnt_pre) / ((double)time_ms * speed_kbps);
    }

    return CR_OK;
}
CredoError_t common_get_tx_prbs_ber(CredoSlice_t* slice, int lane, unsigned time_ms, double ber[2]) {
    CredoLaneMode_t mode = CR_LMODE_OFF;
    ERR_PROPS(hal_lane_get_tx_mode(slice, lane, &mode));

    ber[0] = NAN;
    ber[1] = NAN;
    if (mode == CR_LMODE_DISABLE) {
        return CR_OK;
    } else if (mode != CR_LMODE_NRZ && mode != CR_LMODE_PAM4 && mode != CR_LMODE_PAM3) {
        return CR_OK;
    }

    int prbs_enabled = 0;
    CredoLanePrbsPattern_t prbs_patt;
    ERR_PROPS(hal_prbs_get_tx_checker(slice, lane, &prbs_enabled, &prbs_patt));
    if (prbs_enabled == 0) {
        return CR_OK;
    }

    uint32_t err_cnt_pre[2] = {0}, err_cnt_post[2], speed_kbps;

    if (time_ms < 0) return CR_INVALID_ARGS;
    if (time_ms != 0) {
        ERR_PROPS(hal_prbs_get_tx_count(slice, lane, err_cnt_pre));
        sleep_ms(time_ms);
    } else {
        unsigned long time_ms_long;
        ERR_PROPS(hal_prbs_get_tx_duration(slice, lane, &time_ms_long));
        time_ms = (int)time_ms_long;
        if (time_ms <= 0) time_ms = 1;  // avoid ber calculated incorrect
    }

    ERR_PROPS(hal_prbs_get_tx_count(slice, lane, err_cnt_post));
    ERR_PROPS(hal_lane_get_tx_speed(slice, lane, &speed_kbps));
    if (speed_kbps == 0) {
        LOGS_ERROR("The lane %d is not configured.\n", lane);
        return CR_FAIL;
    }
    for (int i = 0; i < 2; i++) {
        if (err_cnt_post[i] < err_cnt_pre[i]) {
            ber[i] = (UINT_MAX - err_cnt_pre[i] + err_cnt_post[i] + 1) / ((double)time_ms * speed_kbps * 2);
        } else {
            *ber = (err_cnt_post[i] - err_cnt_pre[i]) / ((double)time_ms * speed_kbps * 2);
        }
    }
    return CR_OK;
}

CredoError_t common_get_rx_prbs_ber_all(CredoSlice_t* slice, const int lanes[], unsigned time_ms, double ber[],
                                        unsigned count) {
    if (count > MAX_LANE_PER_SLICE) {
        return CR_INVALID_ARGS;
    }
    CredoLaneMode_t mode[MAX_LANE_PER_SLICE] = {CR_LMODE_OFF};
    uint32_t err_cnt_pre[MAX_LANE_PER_SLICE] = {0};
    uint32_t speed_kbps[MAX_LANE_PER_SLICE] = {0};
    bool is_global = (time_ms == 0);  // is global ber
    CredoTime_t start[MAX_LANE_PER_SLICE];

    // capture necessary info to compute ber
    for (int i = 0; i < count; i++) {
        int lane = lanes[i];
        ERR_PROPS(hal_get_lane_mode(slice, lane, &mode[i]));
        // short circuit to speed up computation
        if (mode[i] == CR_LMODE_DISABLE || mode[i] == CR_LMODE_OFF) continue;
        int prbs_en;
        CredoLanePrbsPattern_t patt;
        // ensure rx prbs is enabled
        ERR_PROPS(hal_get_rx_prbs(slice, lane, &prbs_en, &patt));
        if (!prbs_en) continue;
        // check fw speed is > 0
        ERR_PROPS(hal_fw_get_lane_speed(slice, lane, &speed_kbps[i]));
        if (speed_kbps[i] == 0) continue;
        if (is_global) continue;
        ERR_PROPS(hal_get_rx_prbs_count(slice, lane, &err_cnt_pre[i]));
        get_time(&start[i]);
    }

    // ensure minimnum delay is reached
    if (!is_global) {  // windowed ber
        sleep_ms(time_ms);
    }

    for (int i = 0; i < count; i++) {
        int lane = lanes[i];
        // skip unconfigured lanes
        if (speed_kbps[i] == 0 || mode[i] == CR_LMODE_DISABLE || mode[i] == CR_LMODE_OFF) {
            ber[i] = NAN;
            continue;
        }
        uint32_t err_cnt_post = 0;
        ERR_PROPS(hal_get_rx_prbs_count(slice, lane, &err_cnt_post));
        unsigned long duration_ms = 0;

        if (is_global) {
            ERR_PROPS(hal_get_rx_prbs_duration(slice, lane, &duration_ms));
            if (duration_ms == 0) {
                duration_ms = 1;
            }
            ber[i] = err_cnt_post / ((double)duration_ms * speed_kbps[i]);
            continue;
        }
        duration_ms = ms_passed(&start[i]);

        if (err_cnt_post < err_cnt_pre[i]) {
            ber[i] = (UINT_MAX - err_cnt_pre[i] + err_cnt_post + 1) / ((double)duration_ms * speed_kbps[i]);
        } else {
            ber[i] = (err_cnt_post - err_cnt_pre[i]) / ((double)duration_ms * speed_kbps[i]);
        }
    }

    return CR_OK;
}

CredoError_t common_get_tx_prbs_ber_all(CredoSlice_t* slice, const int lanes[], unsigned time_ms, double ber[][2],
                                        unsigned count) {
    if (count > MAX_LANE_PER_SLICE) {
        return CR_INVALID_ARGS;
    }
    CredoLaneMode_t mode[MAX_LANE_PER_SLICE] = {CR_LMODE_OFF};
    uint32_t err_cnt_pre[MAX_LANE_PER_SLICE][2] = {{0}};
    uint32_t speed_kbps[MAX_LANE_PER_SLICE] = {0};
    bool is_global = (time_ms == 0);  // is global ber
    CredoTime_t start[MAX_LANE_PER_SLICE];

    // capture necessary info to compute ber
    for (int i = 0; i < count; i++) {
        int lane = lanes[i];
        ERR_PROPS(hal_lane_get_tx_mode(slice, lane, &mode[i]));
        // short circuit to speed up computation
        if (mode[i] == CR_LMODE_DISABLE || mode[i] == CR_LMODE_OFF) continue;
        int prbs_en;
        CredoLanePrbsPattern_t patt;
        // ensure tx prbs is enabled
        ERR_PROPS(hal_prbs_get_tx_checker(slice, lane, &prbs_en, &patt));
        if (!prbs_en) continue;
        // check fw speed is > 0
        ERR_PROPS(hal_lane_get_tx_speed(slice, lane, &speed_kbps[i]));
        if (speed_kbps[i] == 0) continue;
        if (is_global) continue;
        ERR_PROPS(hal_prbs_get_tx_count(slice, lane, err_cnt_pre[i]));
        get_time(&start[i]);
    }

    // ensure minimnum delay is reached
    if (!is_global) {  // windowed ber
        sleep_ms(time_ms);
    }

    for (int i = 0; i < count; i++) {
        int lane = lanes[i];
        // skip unconfigured lanes
        if (speed_kbps[i] == 0 || mode[i] == CR_LMODE_DISABLE || mode[i] == CR_LMODE_OFF) {
            ber[i][0] = NAN;
            ber[i][1] = NAN;
            continue;
        }
        unsigned err_cnt_post[2] = {0};
        ERR_PROPS(hal_prbs_get_tx_count(slice, lane, err_cnt_post));
        unsigned long duration_ms = 0;

        if (is_global) {
            ERR_PROPS(hal_prbs_get_tx_duration(slice, lane, &duration_ms));
            if (duration_ms == 0) {
                duration_ms = 1;
            }
            for (int j = 0; j < 2; j++) {
                ber[i][j] = err_cnt_post[j] / ((double)duration_ms * speed_kbps[i]);
            }

            continue;
        }
        duration_ms = ms_passed(&start[i]);

        for (int j = 0; j < 2; j++) {
            if (err_cnt_post < err_cnt_pre[i]) {
                ber[i][j] =
                    (UINT_MAX - err_cnt_pre[i][j] + err_cnt_post[j] + 1) / ((double)duration_ms * speed_kbps[i] * 2);
            } else {
                ber[i][j] = (err_cnt_post[j] - err_cnt_pre[i][j]) / ((double)duration_ms * speed_kbps[i] * 2);
            }
        }
    }

    return CR_OK;
}

/*
 * This display command needs functions below implemented
 * hal_get_rx_polarity
 * hal_get_rx_precoder
 * hal_get_rx_gray_code
 * hal_get_rx_msb
 * hal_prbs_pattern_rx_set_prev
 * hal_prbs_pattern_rx_reset_count
 * hal_prbs_pattern_rx_set_phase
 * hal_prbs_pattern_rx_get_count
 */
CredoError_t common_prbs_phase_error(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    static const char* bit_str[4] = {"00", "01", "10", "11"};
    static const int prbs_pattern[12] = {1, 2, 3, 4, 6, 7, 8, 9, 11, 12, 13, 14};

    ARGCOUNT_CHECK(argc >= 1 && argc <= 2);
    int lane = 0;
    if (argc > 0) {
        ARGPARSE_STRTOL(0, lane);
    }

    unsigned sec = 1;
    unsigned msec = 1000;
    if (argc > 1) {
        ARGPARSE_STRTOL(1, sec);
        msec = (sec < 20) ? sec * 1000 : sec;
    }
    if (msec == 0) msec = 100;

    int rx_pol = 0, rx_pc = 0, rx_gc = 0, rx_msb = 0;
    ERR_PROPS(hal_get_rx_polarity(slice, lane, &rx_pol));
    ERR_PROPS(hal_get_rx_precoder(slice, lane, &rx_pc));
    ERR_PROPS(hal_get_rx_gray_code(slice, lane, &rx_gc));
    ERR_PROPS(hal_get_rx_msb(slice, lane, &rx_msb));

    LOGS_INFO("Note: 00<-01 -> it means should be 00, mistake as 01");
    LOGS_INFO("      Please take care RX PC, GC, MSBLSB setting and make sure PRBS auto sync disabled");
    LOGS_INFO("      Lane %d pol: %d, pc: %d, gc: %d, msblsb: %d", lane, rx_pol, rx_pc, rx_gc, rx_msb);
    LOGS_INFO("      It will takes %.1f seconds to complete process", (msec * 36) / 1000.0);

    ft_table_t* table = NULL;
    for (int prev = 0; prev < 4; prev++) {
        table = ft_create_table();

        ERR_PROPS(
            hal_prbs_pattern_rx_set_prev(slice, lane, true, common_prbs_phase_decode(prev, rx_pol, rx_gc, rx_msb)));
        ft_printf_ln(table, "Prev %s", bit_str[prev]);
        ft_set_cell_prop(table, FT_CUR_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
        ft_printf(table, "PrbsErrCntr|total");
        ft_set_cell_span(table, 0, 0, 14);
        ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
        ft_set_cell_prop(table, 1, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
        ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);

        for (int i = 0; i < 12; i++) {
            ft_printf(table, "%s<-%s", bit_str[common_prbs_phase_encode(prbs_pattern[i] / 4, rx_pol, rx_gc, rx_msb)],
                      bit_str[common_prbs_phase_encode(prbs_pattern[i] % 4, rx_pol, rx_gc, rx_msb)]);
        }
        ft_ln(table);

        ERR_PROPS(hal_prbs_pattern_rx_reset_count(slice, lane));
        for (int phase = 0; phase < 9; phase++) {
            unsigned pattern_count_start[13] = {0};
            unsigned pattern_count_end[13] = {0};

            ft_set_cell_prop(table, FT_CUR_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
            ERR_PROPS(hal_prbs_pattern_rx_set_phase(slice, lane, phase));
            ERR_PROPS(hal_prbs_pattern_rx_get_count(slice, lane, &pattern_count_start[1]));
            sleep_ms(msec);
            ERR_PROPS(hal_prbs_pattern_rx_get_count(slice, lane, &pattern_count_end[1]));
            for (int i = 1; i < 13; i++) {
                pattern_count_end[i] -= pattern_count_start[i];
                pattern_count_end[0] += pattern_count_end[i];
            }

            if (phase == 0) {
                ft_printf(table, "Phase all");
            } else {
                ft_printf(table, "Phase %3X", phase);
            }
            for (int i = 0; i < 13; i++) {
                ft_printf(table, "%10d", pattern_count_end[i]);
            }
            ft_ln(table);
        }
        DISPF(D, "%s", ft_to_string(table));
        ft_destroy_table(table);
    }

    ERR_PROPS(hal_prbs_pattern_rx_set_prev(slice, lane, false, 0));
    ERR_PROPS(hal_prbs_pattern_rx_set_phase(slice, lane, 0));
    return CR_OK;
}
