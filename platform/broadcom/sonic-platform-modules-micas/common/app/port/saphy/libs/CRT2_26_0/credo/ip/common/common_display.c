#include "project.h"

#include "common/common_display.h"
#include "common/common_firmware.h"

#include "fort.h"
#include "parsing.h"
#include "sbs.h"
#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

#if HAL_SUPPORT_FEC_ANA
#include "fec_analyzer/fec_analyzer.h"
#endif

static CredoError_t common_rx_monitor_inner(CredoSlice_t* slice, int* lane_list, int show_tx,
                                            unsigned long milliseconds, const DisplayState_t* D) {
#if HAL_SUPPORT_FEC_ANA
    CredoFecAnalyzerConfig_t fec_config[CHIP_LANES] = {{0}};
    int fec_enabled[CHIP_LANES] = {0};
    unsigned tei_pre[CHIP_LANES] = {0};
    unsigned teo_pre[CHIP_LANES] = {0};
    unsigned tei[CHIP_LANES] = {0};
    unsigned teo[CHIP_LANES] = {0};
    bool fecana_auto_sync[CHIP_LANES] = {0};
#endif
    CredoError_t ret = CR_OK;
    CredoLaneMode_t lane_mode[CHIP_LANES];
    int eyes[CHIP_LANES][3];
    bool rdy[CHIP_LANES] = {0};
    int prbs_enabled[CHIP_LANES] = {0};
    unsigned prbs_err_cnt_pre[CHIP_LANES] = {0};
    unsigned prbs_err_cnt[CHIP_LANES] = {0};
    bool prbs_auto_sync[CHIP_LANES] = {0};
    CredoPrbsLockStatus_t prbs_lock_status[CHIP_LANES] = {0};
    unsigned speed_kbps[CHIP_LANES] = {0};
    int lane;
    unsigned fw_status;
#define FW_RUNNING (1)

#if HAL_SUPPORT_TX_PRBS_CHECKER
    CredoLaneMode_t tx_lane_mode[CHIP_LANES];
    int tx_prbs_enabled[CHIP_LANES] = {0};
    unsigned tx_prbs_err_cnt_pre[CHIP_LANES][2] = {{0}};
    unsigned tx_prbs_err_cnt[CHIP_LANES][2] = {{0}};
    unsigned tx_speed_kbps[CHIP_LANES] = {0};
#endif

    ERR_PROPS(hal_fw_get_status(slice, &fw_status));
    if (fw_status != FW_RUNNING) DISPF(D, "Firmware is NOT RUNNING or FREEZE!\n");

    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        ERR_PROPS(hal_update_lane_mode(slice, lane));
        ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode[lane]));
        if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane]) && lane_mode[lane] != CR_LMODE_PAM3) continue;
        CredoLanePrbsPattern_t prbs_patt;
        ERR_PROPS(hal_get_rx_prbs(slice, lane, &prbs_enabled[lane], &prbs_patt));

        if (CHECK_HAL(slice, hal_get_rx_prbs_lock)) {
            ERR_PROPS(hal_get_rx_prbs_lock(slice, lane, &prbs_lock_status[lane]));
        }

        if (CHECK_HAL(slice, hal_prbs_get_rx_autosync)) {
            ERR_PROPS(hal_prbs_get_rx_autosync(slice, lane, &prbs_auto_sync[lane]));
        }

        unsigned phy_rdy = 0;
        int sd = 0, lane_rdy = 0;
        ERR_PROPS(hal_get_signal_detect(slice, lane, &sd));
        ERR_PROPS(hal_get_lane_ready(slice, lane, &lane_rdy));
        ERR_PROPS(hal_fw_phy_lane_ready(slice, lane, &phy_rdy));
        if (fw_status == FW_RUNNING) {
            rdy[lane] = sd && lane_rdy && phy_rdy;
        } else {
            rdy[lane] = sd;  // don't care ready status when firmware not running
        }

        ERR_PROPS(hal_fw_get_lane_speed(slice, lane, &speed_kbps[lane]));

        if (fw_status == FW_RUNNING) {
            ret = hal_fw_get_eye(slice, lane, eyes[lane]);
        }

        if (ret != CR_OK || fw_status != FW_RUNNING) {
            ERR_PROPS(hal_get_eye(slice, lane, eyes[lane]));
        }
    }

#if HAL_SUPPORT_FEC_ANA
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane])) continue;
        ERR_PROPS(hal_get_fec_analyzer(slice, lane, &fec_enabled[lane], &fec_config[lane]));
        if (fec_enabled[lane] && CHECK_HAL(slice, hal_fecana_get_autosync)) {
            ERR_PROPS(hal_fecana_get_autosync(slice, lane, &fecana_auto_sync[lane]));
        }
    }
#endif

#if HAL_SUPPORT_TX_PRBS_CHECKER
    if (show_tx) {
        for (int i = 0; i < slice->desc->lane_count; i++) {
            if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
            ERR_PROPS(hal_lane_get_tx_mode(slice, lane, &tx_lane_mode[lane]));
            if (!IS_LANE_MODE_PAM4_OR_NRZ(tx_lane_mode[lane])) continue;
            CredoPrbsPattern_t patt;
            ERR_PROPS(hal_prbs_get_tx_checker(slice, lane, &tx_prbs_enabled[lane], &patt));
            ERR_PROPS(hal_lane_get_tx_speed(slice, lane, &tx_speed_kbps[lane]));
        }
    }
#endif

    if (milliseconds != 0) {
        for (int i = 0; i < slice->desc->lane_count; i++) {
            if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
            if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane]) && lane_mode[lane] != CR_LMODE_PAM3) continue;
#if HAL_SUPPORT_FEC_ANA
            if (fec_enabled[lane]) ERR_PROPS(hal_get_fec_analyzer_counter(slice, lane, &tei_pre[lane], &teo_pre[lane]));
#endif
            ERR_PROPS(hal_get_rx_prbs_count(slice, lane, &prbs_err_cnt_pre[lane]));
        }
#if HAL_SUPPORT_TX_PRBS_CHECKER
        if (show_tx) {
            for (int i = 0; i < slice->desc->lane_count; i++) {
                if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
                if (!tx_prbs_enabled[lane]) continue;
                ERR_PROPS(hal_prbs_get_tx_count(slice, lane, tx_prbs_err_cnt_pre[lane]));
            }
        }
#endif
        sleep_ms(milliseconds);
    }

    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane]) && lane_mode[lane] != CR_LMODE_PAM3) continue;
#if HAL_SUPPORT_FEC_ANA
        if (fec_enabled[lane]) {
            ERR_PROPS(hal_get_fec_analyzer_counter(slice, lane, &tei[lane], &teo[lane]));
            if (tei[lane] >= tei_pre[lane]) {
                tei[lane] -= tei_pre[lane];
            } else {
                tei[lane] = UINT_MAX - tei_pre[lane] + tei[lane] + 1;
            }
            if (teo[lane] >= teo_pre[lane]) {
                teo[lane] -= teo_pre[lane];
            } else {
                teo[lane] = UINT_MAX - teo_pre[lane] + teo[lane] + 1;
            }
        }
#endif
        ERR_PROPS(hal_get_rx_prbs_count(slice, lane, &prbs_err_cnt[lane]));
        if (prbs_err_cnt[lane] >= prbs_err_cnt_pre[lane]) {
            prbs_err_cnt[lane] -= prbs_err_cnt_pre[lane];
        } else {
            prbs_err_cnt[lane] = UINT_MAX - prbs_err_cnt_pre[lane] + prbs_err_cnt[lane] + 1;
        }
    }

#if HAL_SUPPORT_TX_PRBS_CHECKER
    if (show_tx) {
        for (int i = 0; i < slice->desc->lane_count; i++) {
            if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
            if (!tx_prbs_enabled[lane]) continue;
            ERR_PROPS(hal_prbs_get_tx_count(slice, lane, tx_prbs_err_cnt[lane]));
            for (int bin = 0; bin < 2; bin++) {
                if (tx_prbs_err_cnt[lane][bin] >= tx_prbs_err_cnt_pre[lane][bin]) {
                    tx_prbs_err_cnt[lane][bin] -= tx_prbs_err_cnt_pre[lane][bin];
                } else {
                    tx_prbs_err_cnt[lane][bin] =
                        UINT_MAX - tx_prbs_err_cnt_pre[lane][bin] + tx_prbs_err_cnt[lane][bin] + 1;
                }
            }
        }
    }
#endif

    // display
    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_set_cell_prop(table, FT_ANY_ROW, 0, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_LEFT);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_RIGHT_PADDING, 0);

    // create lane # header
    ft_printf(table, "Lane");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        ft_printf(table, "%s(%d)", stringify_lane_id(slice, lane), lane);
    }
    ft_ln(table);  // ends creating the header

    // add mode row
    ft_printf(table, "Mode");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        ft_printf(table, "%s", lane_mode_to_string(lane_mode[lane]));
    }
    ft_ln(table);
    ft_add_separator(table);

    // add link status row
    ft_printf(table, "Link Status");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        ft_printf(table, "%s", rdy[lane] == 0 ? "NOT-RDY" : "RDY");
    }
    ft_ln(table);

    // add eye-height 1 row
    ft_printf(table, "Eye 1 (mV)");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        if (rdy[lane]) {
            ft_printf(table, "%d", eyes[lane][0]);
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);

    // add eye-height 2 row
    ft_printf(table, "Eye 2 (mV)");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        if (rdy[lane] && (lane_mode[lane] == CR_LMODE_PAM4 || lane_mode[lane] == CR_LMODE_PAM3)) {
            ft_printf(table, "%d", eyes[lane][1]);
        } else {
            ft_printf(table, "-");
        }
    }
    ft_ln(table);

    // add eye-height 3 row
    ft_printf(table, "Eye 3 (mV)");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        if (rdy[lane] && lane_mode[lane] == CR_LMODE_PAM4) {
            ft_printf(table, "%d", eyes[lane][2]);
        } else {
            ft_printf(table, "-");
        }
    }
    ft_ln(table);
    ft_add_separator(table);

    // write prbs timer row
    ft_write(table, "PRBS Timer (sec)");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        if (rdy[lane] && prbs_enabled[lane]) {
            unsigned long prbs_milliseconds = milliseconds;
            if (prbs_milliseconds == 0) ERR_PROPS(hal_get_rx_prbs_duration(slice, lane, &prbs_milliseconds));
            ft_printf(table, "%.1f", (double)prbs_milliseconds / 1000);
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);

    // not all chips support prbs locking
    if (CHECK_HAL(slice, hal_get_rx_prbs_lock)) {
        ft_printf(table, "PRBS Lock");
        for (int i = 0; i < slice->desc->lane_count; i++) {
            if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
            if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
            if (rdy[lane] && prbs_enabled[lane] && prbs_lock_status[lane] != CR_PRBS_LOCK_INVALID) {
                ft_printf(table, "%s", (prbs_lock_status[lane] == CR_PRBS_LOCK_YES) ? "locked" : "not locked");
            } else {
                ft_write(table, "-");
            }
        }
        ft_ln(table);
    }

    // add prbs count row
    ft_printf(table, "PRBS Count");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        if (rdy[lane] && prbs_enabled[lane]) {
            const char* async_char = prbs_auto_sync[lane] ? "*" : "";
            ft_printf(table, "%s%u%s", async_char, prbs_err_cnt[lane], async_char);
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);

    // add prbs ber row
    ft_printf(table, "PRBS BER");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        unsigned long prbs_milliseconds = milliseconds;
        if (prbs_milliseconds == 0) {
            ERR_PROPS(hal_get_rx_prbs_duration(slice, lane, &prbs_milliseconds));
            if (prbs_milliseconds == 0) prbs_milliseconds = 1;  // avoid ber calculated incorrect
        }
        if (rdy[lane] && speed_kbps[lane] != 0 && prbs_enabled[lane]) {
            ft_printf(table, "%.2e", prbs_err_cnt[lane] / ((double)prbs_milliseconds * speed_kbps[lane]));
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);

#if HAL_SUPPORT_TX_PRBS_CHECKER
    if (!show_tx) {
        goto end_tx_prbs_checker;
    }
    ft_add_separator(table);
    ft_printf(table, "Tx PRBS Timer (s)");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (!tx_prbs_enabled[lane]) {
            ft_printf(table, "-");
        } else {
            unsigned long prbs_milliseconds = milliseconds;
            if (prbs_milliseconds == 0) ERR_PROPS(hal_prbs_get_tx_duration(slice, lane, &prbs_milliseconds));
            ft_printf(table, "%.1f", (double)prbs_milliseconds / 1000);
        }
    }
    ft_ln(table);
    ft_printf(table, "Tx PRBS Count Bin1");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (!tx_prbs_enabled[lane]) {
            ft_printf(table, "-");
        } else {
            ft_printf(table, "%u", tx_prbs_err_cnt[lane][0]);
        }
    }
    ft_ln(table);
    ft_printf(table, "              Bin2");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (!tx_prbs_enabled[lane]) {
            ft_printf(table, "-");
        } else {
            ft_printf(table, "%u", tx_prbs_err_cnt[lane][1]);
        }
    }
    ft_ln(table);
    ft_printf(table, "Tx PRBS BER Bin1");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (tx_prbs_enabled[lane] && tx_speed_kbps[lane] != 0) {
            unsigned long prbs_milliseconds = milliseconds;
            if (prbs_milliseconds == 0) {
                ERR_PROPS(hal_prbs_get_tx_duration(slice, lane, &prbs_milliseconds));
                if (prbs_milliseconds == 0) prbs_milliseconds = 1;  // avoid ber calculated incorrect
            }
            ft_printf(table, "%.2e", tx_prbs_err_cnt[lane][0] / ((double)prbs_milliseconds * tx_speed_kbps[lane] * 2));
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);
    ft_printf(table, "            Bin2");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (tx_prbs_enabled[lane] && tx_speed_kbps[lane] != 0) {
            unsigned long prbs_milliseconds = milliseconds;
            if (prbs_milliseconds == 0) {
                ERR_PROPS(hal_prbs_get_tx_duration(slice, lane, &prbs_milliseconds));
                if (prbs_milliseconds == 0) prbs_milliseconds = 1;  // avoid ber calculated incorrect
            }
            ft_printf(table, "%.2e", tx_prbs_err_cnt[lane][1] / ((double)prbs_milliseconds * tx_speed_kbps[lane] * 2));
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);
end_tx_prbs_checker:
#endif

#if HAL_SUPPORT_FEC_ANA
    ft_add_separator(table);

    // write prbs timer row
    ft_write(table, "FEC Timer (sec)");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        if (fec_enabled[lane]) {
            unsigned long fec_milliseconds = milliseconds;
            if (fec_milliseconds == 0) ERR_PROPS(hal_get_fec_analyzer_duration(slice, lane, &fec_milliseconds));
            ft_printf(table, "%.1f", (double)fec_milliseconds / 1000);
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);

    // add fec row
    ft_printf(table, "FEC Type");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        if (fec_enabled[lane]) {
            switch (fec_config[lane].error_type) {
                case CR_FEC_ERROR_BIT:
                    ft_printf(table, "bit");
                    break;
                case CR_FEC_ERROR_SYMBOL:
                    ft_printf(table, "symbol");
                    break;
                case CR_FEC_ERROR_FRAME:
                    ft_printf(table, "frame");
                    break;
                default:
                    ft_printf(table, "???");
            }

        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);

    // add pre-fec count row
    ft_printf(table, "Pre-FEC Count");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        if (rdy[lane] && fec_enabled[lane]) {
            const char* async_char = fecana_auto_sync[lane] ? "*" : "";
            ft_printf(table, "%s%u%s", async_char, tei[lane], async_char);
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);

    // add post-fec count row
    ft_printf(table, "Post-FEC Count");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        if (rdy[lane] && fec_enabled[lane]) {
            ft_printf(table, "%u", teo[lane]);
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);

    // add pre-fec ber row
    ft_printf(table, "Pre-FEC Err Rt");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        unsigned long fec_milliseconds = milliseconds;
        if (fec_milliseconds == 0) {
            ERR_PROPS(hal_get_fec_analyzer_duration(slice, lane, &fec_milliseconds));
            if (fec_milliseconds == 0) fec_milliseconds = 1;  // avoid ber calculated incorrect
        }
        if (rdy[lane] && fec_enabled[lane] && speed_kbps[lane] != 0) {
            double fec_error_rate;
            ERR_PROPS(fec_analyzer_calculate_error_rate(slice, lane, FECANA_COUNTER_TEI, fec_milliseconds,
                                                        &fec_error_rate, 0, tei[lane]));
            ft_printf(table, "%.2e", fec_error_rate);
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);

    // add post-fec ber row
    ft_printf(table, "Post-FEC Err Rt");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (IS_LANE_MODE_DISABLE(lane_mode[lane])) continue;
        unsigned long fec_milliseconds = milliseconds;
        if (fec_milliseconds == 0) {
            ERR_PROPS(hal_get_fec_analyzer_duration(slice, lane, &fec_milliseconds));
            if (fec_milliseconds == 0) fec_milliseconds = 1;  // avoid ber calculated incorrect
        }
        if (rdy[lane] && fec_enabled[lane] && speed_kbps[lane] != 0) {
            double fec_error_rate;
            ERR_PROPS(fec_analyzer_calculate_error_rate(slice, lane, FECANA_COUNTER_TEO, fec_milliseconds,
                                                        &fec_error_rate, 0, teo[lane]));
            ft_printf(table, "%.2e", fec_error_rate);
        } else {
            ft_write(table, "-");
        }
    }
    ft_ln(table);
#endif

    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

CredoError_t common_slice_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    const char* sdk_rev = cr_sdk_version_str();
    char fw_version[32] = {0};
    unsigned fw_hash = 0x0;
    unsigned fw_ver = 0;
    ERR_CATCH(hal_fw_ver(slice, &fw_ver), fw_ver = 0);
    stringify_firmware_version(fw_ver, fw_version);
    ERR_CATCH(hal_fw_hash(slice, &fw_hash), fw_hash = 0x0);
    double temp = 0.0;
    ERR_CATCH(hal_fw_get_slice_temp(slice, &temp), temp = -1.0);

    const char* dev_name = cr_device_get_type_name(slice->device);

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_INVALID_ARGS;

    ft_printf_ln(table, "SDK Version|%s", sdk_rev);
    ft_printf_ln(table, "Device Type|%s", dev_name);
    ft_printf_ln(table, "Firmware Version|%s (0x%X)", fw_version, fw_hash);
    ft_printf_ln(table, "Slice Temp|%.1fC", temp);

    DISPF(D, "%s", ft_to_string(table));

    ft_destroy_table(table);
    return CR_OK;
}

static CredoError_t common_rx_monitor_init(CredoSlice_t* slice, int* lane_list, int show_tx) {
    int lane;
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        CredoLaneMode_t lane_mode;
        ERR_PROPS(hal_update_lane_mode(slice, lane));
        ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
        if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode) && lane_mode != CR_LMODE_PAM3) continue;

        // only perform operations if user has rx prbs checker enabled
        CredoLanePrbsPattern_t prbs_pattern;
        int prbs_enable;
        ERR_PROPS(hal_get_rx_prbs(slice, lane, &prbs_enable, &prbs_pattern));
        if (!prbs_enable) continue;

        ERR_PROPS(hal_reset_rx_prbs_count(slice, lane));

#if HAL_SUPPORT_TX_PRBS_CHECKER
        if (show_tx) {
            CredoPrbsPattern_t tx_pattern;
            int tx_prbs_enable;
            ERR_PROPS(hal_prbs_get_tx_checker(slice, lane, &tx_prbs_enable, &tx_pattern));
            if (!tx_prbs_enable) {
                ERR_PROPS(hal_prbs_set_tx_checker(slice, lane, true, prbs_pattern));  // set to rx pattern if not conf
            }
            ERR_PROPS(hal_prbs_reset_tx_count(slice, lane));
        }
#endif

#if HAL_SUPPORT_FEC_ANA
        CredoFecAnalyzerConfig_t fec_config;
        int enable;
        ERR_PROPS(hal_get_fec_analyzer(slice, lane, &enable, &fec_config));
        bool reset_supported = CHECK_HAL(slice, hal_fecana_reset);
        if (enable && reset_supported) {
            ERR_PROPS(hal_fecana_reset(slice, lane));
        } else {
            // configure fec analyzer to basic defaults
            if (lane_mode == CR_LMODE_PAM4) {
                fec_config.codeword_size = 5440;
                fec_config.symbol_size = 10;
                fec_config.threshold = 15;
                fec_config.error_type = CR_FEC_ERROR_FRAME;
            } else {
                fec_config.codeword_size = 5280;
                fec_config.symbol_size = 10;
                fec_config.threshold = 7;
                fec_config.error_type = CR_FEC_ERROR_FRAME;
            }
            if (hal_set_fec_analyzer(slice, lane, true, &fec_config) == CR_OK) {
                if (reset_supported) {
                    ERR_PROPS(hal_fecana_reset(slice, lane));
                }
            }
        }
#endif
    }
    return CR_OK;
}

static CredoError_t common_rx_monitor_reset(CredoSlice_t* slice, int* lane_list, int show_tx) {
    int lane;
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        CredoLaneMode_t lane_mode;
        ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
        if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode) && lane_mode != CR_LMODE_PAM3) continue;

        // only perform operations if user has rx prbs checker enabled
        CredoLanePrbsPattern_t prbs_pattern;
        int prbs_enable;
        ERR_PROPS(hal_get_rx_prbs(slice, lane, &prbs_enable, &prbs_pattern));
        if (!prbs_enable) continue;

        ERR_PROPS(hal_reset_rx_prbs_count(slice, lane));

#if HAL_SUPPORT_TX_PRBS_CHECKER
        if (show_tx) {
            CredoPrbsPattern_t tx_pattern;
            int tx_prbs_enable;
            ERR_PROPS(hal_prbs_get_tx_checker(slice, lane, &tx_prbs_enable, &tx_pattern));
            if (tx_prbs_enable) {
                ERR_PROPS(hal_prbs_reset_tx_count(slice, lane));
            }
        }
#endif

#if HAL_SUPPORT_FEC_ANA
        CredoFecAnalyzerConfig_t fec_config;
        int enable;
        ERR_PROPS(hal_get_fec_analyzer(slice, lane, &enable, &fec_config));
        bool reset_supported = CHECK_HAL(slice, hal_fecana_reset);
        if (enable && reset_supported) {
            ERR_PROPS(hal_fecana_reset(slice, lane));
        }
#endif
    }
    return CR_OK;
}

CredoError_t common_rx_monitor(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    int init = 0;
    int reset = 0;
    int show_tx = 0;
    unsigned milliseconds = 0;
    struct argparse_option options[] = {
        OPT_BOOLEAN('i', "init", &init, "same as reset, but also configures fec analyzer if unconfigured"),
        OPT_BOOLEAN('r', "reset", &reset, "reset prbs counter and fecana counter"),
        OPT_UINTEGER('d', "duration", &milliseconds, "set duration to capture prbs"),
        OPT_BOOLEAN('t', "tx", &show_tx, "show tx prbs checker data"),
        OPT_HELP(),
        OPT_END(),
    };

    DISPLAY_PARSE_ARGS(slice, argc, argv, options, D);

    int lane_list[CHIP_LANES] = {CHIP_LANES};
    const char* lane_list_str = "all";
    // keep for backwards compatibility, but dont document
    ARGCOUNT_CHECK(argc <= 3);

    if (argc > 0 && strcmp(argv[0], "init") == 0) {
        init = 1;
        if (argc == 2) lane_list_str = argv[1];
    } else if (argc > 0 && strcmp(argv[0], "reset") == 0) {
        reset = 1;
        if (argc == 2) lane_list_str = argv[1];
    } else if (argc >= 1) {
        lane_list_str = argv[0];
    }
    parsing_string_to_lane_list(lane_list_str, lane_list, slice->desc->lane_count);

    if (init) {
        return common_rx_monitor_init(slice, lane_list, show_tx);
    }
    if (reset) {
        return common_rx_monitor_reset(slice, lane_list, show_tx);
    }
    // no init or reset
    if (argc == 2) {
        ARGPARSE_STRTOUL(1, milliseconds);
    }
    return common_rx_monitor_inner(slice, lane_list, show_tx, milliseconds, D);
}

CredoError_t common_serdes_param_parser(CredoSlice_t* slice, serdes_param_func_t func, int argc, const char* argv[],
                                        const DisplayState_t* D) {
    split_display_t split_display = NON_SPLIT_DISPLAY;
    int lane_list[CHIP_LANES] = {CHIP_LANES};
    const char* lane_list_str = "all";

    if (argc > 2) return CR_INVALID_ARGS;

    if (argc > 0) {
        const char* string = argv[0];

        // Split display check
        if (string[0] == 'S') {
            split_display = SPLIT_DISPLAY;
            ++string;
        }

        if (strlen(string) > 0) {
            lane_list_str = string;
        } else if (argc > 1) {
            lane_list_str = argv[1];
        }
    }

    common_slice_info(slice, 0, NULL, D);
    parsing_string_to_lane_list(lane_list_str, lane_list, slice->desc->lane_count);
    return func(slice, lane_list, split_display, D);
}

CredoError_t common_mac_show_statistics(CredoSlice_t* slice, int port_num, int port_list[][2],
                                        CredoMACStatistics_t stats[][2], const DisplayState_t* D) {
    bool configured = false;

    for (uint32_t port_id = 0; port_id < port_num; port_id++) {
        if (port_list[port_id][CR_SIDE_HOST] == 1 || port_list[port_id][CR_SIDE_LINE] == 1) {
            configured = true;
            break;
        }
    }

    if (configured != true) {
        DISPF(D, "No port is configured.\n");
        return CR_OK;
    }

    ft_table_t* table = ft_create_table();
    if (!table) return CR_OUT_OF_MEMORY;

    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_set_cell_prop(table, FT_ANY_ROW, 0, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_LEFT);
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, 1, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);

    ft_printf(table, "PortId");
    for (uint32_t port_id = 0, col = 0; port_id < port_num; port_id++) {
        if (port_list[port_id][CR_SIDE_HOST] == 0 && port_list[port_id][CR_SIDE_LINE] == 0) continue;
        ft_printf(table, "%d|", port_id);
        ft_set_cell_span(table, 0, 2 * col + 1, 2);
        col++;
    }
    ft_ln(table);
    ft_add_separator(table);

    ft_printf(table, "Side");
    for (uint32_t port_id = 0; port_id < port_num; port_id++) {
        if (port_list[port_id][CR_SIDE_HOST] == 1 || port_list[port_id][CR_SIDE_LINE] == 1)
            ft_printf(table, "Sys|Line");
    }
    ft_ln(table);

#define IF_PORT_SIDE_EXIST_SHOW_ELEMENT(port_id, side, element, fmt) \
    do {                                                             \
        if (port_list[port_id][side] == 1)                           \
            ft_printf(table, fmt, stats[port_id][side].element);     \
        else                                                         \
            ft_printf(table, "-");                                   \
    } while (0)
#define SHOW_MAC_STATS_ELEMENT_BASIC(name, element, format)                                               \
    do {                                                                                                  \
        const char* fmt = format;                                                                         \
        ft_printf(table, "%s", name);                                                                     \
        for (uint32_t port_id = 0; port_id < port_num; port_id++) {                                       \
            if (port_list[port_id][CR_SIDE_HOST] == 0 && port_list[port_id][CR_SIDE_LINE] == 0) continue; \
            IF_PORT_SIDE_EXIST_SHOW_ELEMENT(port_id, CR_SIDE_HOST, element, fmt);                         \
            IF_PORT_SIDE_EXIST_SHOW_ELEMENT(port_id, CR_SIDE_LINE, element, fmt);                         \
        }                                                                                                 \
        ft_ln(table);                                                                                     \
    } while (0)
#define SHOW_MAC_STATS_ELEMENT(element) SHOW_MAC_STATS_ELEMENT_BASIC(#element, element, "%" PRIu64)

    ft_add_separator(table);
    // RX statistic counters
    SHOW_MAC_STATS_ELEMENT(etherStatsRxOctets);
    SHOW_MAC_STATS_ELEMENT(OctetsReceivedOK);
    SHOW_MAC_STATS_ELEMENT(aAlignmentErrors);
    SHOW_MAC_STATS_ELEMENT(aPAUSEMACCtrlFramesReceived);
    SHOW_MAC_STATS_ELEMENT(aFrameTooLongErrors);
    SHOW_MAC_STATS_ELEMENT(aInRangeLengthErrors);
    SHOW_MAC_STATS_ELEMENT(aFramesReceivedOK);
    SHOW_MAC_STATS_ELEMENT(aFrameCheckSequenceErrors);
    SHOW_MAC_STATS_ELEMENT(VLANReceivedOK);
    SHOW_MAC_STATS_ELEMENT(ifInErrors);
    SHOW_MAC_STATS_ELEMENT(ifInUcastPkts);
    SHOW_MAC_STATS_ELEMENT(ifInMulticastPkts);
    SHOW_MAC_STATS_ELEMENT(ifInBroadcastPkts);
    SHOW_MAC_STATS_ELEMENT(etherStatsDropEvents);
    SHOW_MAC_STATS_ELEMENT(etherStatsRxPkts);
    SHOW_MAC_STATS_ELEMENT(etherStatsUndersizePkts);
    SHOW_MAC_STATS_ELEMENT(etherStatsRxPkts64Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsRxPkts65to127Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsRxPkts128to255Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsRxPkts256to511Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsRxPkts512to1023Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsRxPkts1024to1518Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsRxPkts1519toMaxOctets);
    SHOW_MAC_STATS_ELEMENT(etherStatsOversizePkts);
    SHOW_MAC_STATS_ELEMENT(etherStatsJabbers);
    SHOW_MAC_STATS_ELEMENT(etherStatsFragments);
#if 0  // hidden temporarily
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesReceived_0);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesReceived_1);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesReceived_2);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesReceived_3);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesReceived_4);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesReceived_5);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesReceived_6);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesReceived_7);
#endif
    SHOW_MAC_STATS_ELEMENT(aMACControlFramesReceived);

    ft_add_separator(table);
    // TX statistic counters
    SHOW_MAC_STATS_ELEMENT(etherStatsTxOctets);
    SHOW_MAC_STATS_ELEMENT(OctetsTransmittedOK);
    SHOW_MAC_STATS_ELEMENT(aPAUSEMACCtrlFramesTransmitted);
    SHOW_MAC_STATS_ELEMENT(aFramesTransmittedOK);
    SHOW_MAC_STATS_ELEMENT(VLANTransmittedOK);
    SHOW_MAC_STATS_ELEMENT(ifOutErrors);
    SHOW_MAC_STATS_ELEMENT(ifOutUcastPkts);
    SHOW_MAC_STATS_ELEMENT(ifOutMulticastPkts);
    SHOW_MAC_STATS_ELEMENT(ifOutBroadcastPkts);
    SHOW_MAC_STATS_ELEMENT(etherStatsTxPkts64Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsTxPkts65to127Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsTxPkts128to255Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsTxPkts256to511Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsTxPkts512to1023Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsTxPkts1024to1518Octets);
    SHOW_MAC_STATS_ELEMENT(etherStatsTxPkts1519toMaxOctets);
#if 0  // hidden temporarily
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesTransmitted_0);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesTransmitted_1);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesTransmitted_2);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesTransmitted_3);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesTransmitted_4);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesTransmitted_5);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesTransmitted_6);
    SHOW_MAC_STATS_ELEMENT(aCBFCPAUSEFramesTransmitted_7);
#endif
    SHOW_MAC_STATS_ELEMENT(aMACControlFramesTransmitted);
    SHOW_MAC_STATS_ELEMENT(etherStatsTxPkts);

    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

CredoError_t common_dump_fw_reg(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    CredoError_t ret = CR_OK;
    unsigned section_count[16] = {0};
    unsigned search_count[16] = {0};
    unsigned total_count_except_section0 = 0;
    bool section_style = false;
    ft_table_t* table = ft_create_table();
    if (!table) return CR_OUT_OF_MEMORY;

    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);

    for (unsigned section = 0; section < 16; section++) {
        unsigned addr = 0;
        unsigned upper = 0x10000, lower = 0;

        if (section != 0) {
            upper = section_count[0];
        }

        // get section count with bisection method to reduce access count
        do {
            unsigned value = FW_CMD_LOG_SILENT;
            ++search_count[section];
            if ((ret = hal_fw_reg_rd(slice, addr, section, &value)) != CR_OK) {
                upper = addr;
                addr = (addr + lower) / 2;
            } else {
                lower = addr;
                addr = (addr + upper) / 2;
            }
        } while (upper - lower > 1);
        // if (upper == 0) break;

        section_count[section] = upper;

        if (section != 0) {
            total_count_except_section0 += upper;
            if (!section_style) section_style = section_count[section] >= section_count[section - 1] ? true : false;
        }
    }

    if (section_count[0] <= total_count_except_section0 && section_style == false) {
        // firmware_internal()
        for (unsigned section = 0; section_count[section] != 0 && section < 16 - 1; section++) {
            section_count[section] -= section_count[section + 1];
        }
    } else {
        // firmware_internal_section()
        section_count[0] -= total_count_except_section0;
    }

    for (unsigned section = 0; section < 16; section++) {
        int addr;

        if (section_count[section] <= 0) continue;

        ft_add_separator(table);

        ft_printf_ln(table, "section %d", section);
        ft_printf(table, "addr");
        for (addr = 0; addr < 10; addr++) ft_printf(table, "%02d", addr);
        ft_ln(table);
        ft_add_separator(table);

        for (addr = 0; addr < section_count[section]; addr++) {
            unsigned value = FW_CMD_LOG_SILENT;
            if ((ret = hal_fw_reg_rd(slice, (unsigned)addr, section, &value)) != CR_OK) break;
            if (addr % 10 == 0) {
                ft_printf(table, "%05d", addr);
            }
            ft_printf(table, "0x%04X", value);
            if (addr % 10 == 9) {
                ft_ln(table);
            }
        }
        if (addr % 10 != 0) ft_ln(table);
    }

    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

CredoError_t common_fw_dump_exit_codes(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
#if defined(ALL_EXIT_CODES_START) && defined(ALL_EXIT_CODES_END) && defined(GET_FW_EXIT_CODES)
    int host_lanes = 0, line_lanes = 0;
    ERR_PROPS(hal_get_lane_count(slice, &host_lanes, &line_lanes));

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_INVALID_ARGS;
    CredoError_t err = CR_FAIL;
    ft_printf(table, "Lane\\Index");
    for (unsigned idx = ALL_EXIT_CODES_START; idx < ALL_EXIT_CODES_END; idx++) {
        ft_printf(table, "%d", idx);
    }
    ft_ln(table);
    ft_add_separator(table);

    for (unsigned lane = 0; lane < slice->desc->lane_count; lane++) {
        CredoLaneMode_t mode;
        if (lane == host_lanes) ft_add_separator(table);
        ERR_CATCH(hal_get_lane_mode(slice, lane, &mode), goto cleanup);
        if (mode == CR_LMODE_DISABLE) continue;
        ft_printf(table, "%s(%d)", stringify_lane_id(slice, lane), lane);
        for (unsigned index = ALL_EXIT_CODES_START; index < ALL_EXIT_CODES_END; index++) {
            unsigned value;
            ERR_CATCH(GET_FW_EXIT_CODES(slice, lane, index, &value), goto cleanup);
            ft_printf(table, "0x%X", value);
        }
        ft_ln(table);
    }
    DISPF(D, "%s", ft_to_string(table));
    err = CR_OK;
cleanup:
    ft_destroy_table(table);
    return err;
#else
    return CR_NOTIMPLEMENTED;
#endif
}

CredoError_t common_fw_dump_error_codes(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
#if defined(ALL_ERROR_CODES_START) && defined(ALL_ERROR_CODES_END) && defined(GET_FW_ERROR_CODES)
    int host_lanes = 0, line_lanes = 0;
    ERR_PROPS(hal_get_lane_count(slice, &host_lanes, &line_lanes));

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_INVALID_ARGS;
    CredoError_t err = CR_FAIL;
    ft_printf(table, "Lane\\Index");
    for (unsigned idx = ALL_ERROR_CODES_START; idx < ALL_ERROR_CODES_END; idx++) {
        ft_printf(table, "%d", idx);
    }
    ft_ln(table);
    ft_add_separator(table);

    for (unsigned lane = 0; lane < slice->desc->lane_count; lane++) {
        CredoLaneMode_t mode = CR_LMODE_OFF;
        if (lane == host_lanes) ft_add_separator(table);
        ERR_CATCH(hal_get_lane_mode(slice, lane, &mode), goto cleanup);
        if (mode == CR_LMODE_DISABLE) continue;
        ft_printf(table, "%s(%d)", stringify_lane_id(slice, lane), lane);
        for (unsigned index = ALL_ERROR_CODES_START; index < ALL_ERROR_CODES_END; index++) {
            unsigned value = 0;
            ERR_CATCH(GET_FW_ERROR_CODES(slice, lane, index, &value), goto cleanup);
            ft_printf(table, "%d", (value & 0x8000) ? (int)value - 0x10000 : (int)value);
        }
        ft_ln(table);
    }
    DISPF(D, "%s", ft_to_string(table));
    err = CR_OK;
cleanup:
    ft_destroy_table(table);
    return err;
#else
    return CR_NOTIMPLEMENTED;
#endif
}

CredoError_t common_fw_serdes_control(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    CredoError_t ret = CR_FAIL;
    sbs* tx_prbs_string = SBS128("");
    sbs* rx_prbs_string = SBS128("");
    sbs* lmode_string = SBS128("");
    bool has_tx_mode = CHECK_HAL(slice, hal_get_tx_lane_mode);
    bool has_tx_state = CHECK_HAL(slice, hal_lane_tx_status);
    bool has_lb_mode = CHECK_HAL(slice, hal_get_lane_loopback_mode);
    bool has_hw_tx_pc = CHECK_HAL(slice, hal_get_hw_tx_precoder);
    bool has_tx_taps = CHECK_HAL(slice, hal_get_tx_ffe_range) && CHECK_HAL(slice, hal_get_tx_taps);
    bool has_prbs = CHECK_HAL(slice, hal_get_tx_prbs) && CHECK_HAL(slice, hal_get_rx_prbs);
    int tx_taps_range = 0;

    unsigned fw_status = 0;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));
    if (fw_status != FW_RUNNING) DISPF(D, "Firmware is NOT RUNNING or FREEZE!\n");

    for (int i = argc - 1; i >= 0; i--) {
        const char* b = argv[i];
        bool has = true;

        if (b[0] == '-') {
            has = false;
            ++b;
        } else if (b[0] == '+') {
            has = true;
            ++b;
        }

        if (!strcmp(b, "?")) {
            DISPF(D, "%s",
                  "append below arguments to choose specific category\n"
                  "    [-][+]tx_state   : TX State\n"
                  "    [-][+]lb         : Loopback Mode\n"
                  "    [-][+]tx_taps    : TX Taps\n"
                  "    [-][+]prbs       : PRBS\n"
                  "\nexample: serdes_control -prbs");
            return CR_OK;
        } else if (!strcmp(b, "tx_state")) {
            has_tx_state = has;
        } else if (!strcmp(b, "lb")) {
            has_lb_mode = has;
        } else if (!strcmp(b, "tx_taps")) {
            has_tx_taps = has;
        } else if (!strcmp(b, "prbs")) {
            has_prbs = has;
        } else {
            DISPF(D, "Unknown argument: %s\n", argv[i]);
        }
    }

    int host_lanes = 0, line_lanes = 0;
    ERR_PROPS(hal_get_lane_count(slice, &host_lanes, &line_lanes));

    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_RIGHT_PADDING, 0);

    if (has_tx_taps) hal_get_tx_ffe_range(slice, 0, &tx_taps_range, NULL);

    const char* hdr_tx_taps_range = tx_taps_range == 7   ? "-3 -2 -1 M +1 +2 +3"
                                    : tx_taps_range == 5 ? "-2 -1 M +1 +2"
                                    : tx_taps_range == 4 ? "-1 M +1 +2"
                                                         : " ";

    struct {
        bool has;
        const char* hdr1;
        const char* hdr2;
    } hdr_list[] = {
#define ADD_HEADER(has, hdr1, hdr2) {has, hdr1, hdr2}
        ADD_HEADER(true, " ", "Ln"),
        ADD_HEADER(true, (has_tx_mode ? "Mode" : " "), (has_tx_mode ? "TX,RX" : "Mode")),
        ADD_HEADER(true, (has_tx_mode ? "Speed" : " "), (has_tx_mode ? "TX,RX" : "Speed")),
        ADD_HEADER(has_tx_state, "TX", "State"),
        ADD_HEADER(has_lb_mode, "Loopback", "Mode"),
        ADD_HEADER(true, "Polarity", "Tx,Rx"),
        ADD_HEADER(true, "Gray Code", "Tx,Rx"),
        ADD_HEADER(true, "Precoder", has_hw_tx_pc ? "fwTx,Tx,Rx" : "Tx,Rx"),
        ADD_HEADER(true, "MSB Swap", "Tx,Rx"),
        ADD_HEADER(true, "Input", "Mode"),
        ADD_HEADER(has_tx_taps && tx_taps_range, "TX Taps", hdr_tx_taps_range),
        ADD_HEADER(has_prbs, "PRBS", "Tx,Rx"),
    };

    for (int i = 0; i < sizeof(hdr_list) / sizeof(*hdr_list); i++) {
        if (hdr_list[i].has) ft_printf(table, "%s", hdr_list[i].hdr1);
    }
    ft_ln(table);
    for (int i = 0; i < sizeof(hdr_list) / sizeof(*hdr_list); i++) {
        if (hdr_list[i].has) ft_printf(table, "%s", hdr_list[i].hdr2);
    }
    ft_ln(table);
    ft_add_separator(table);

    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        if (lane == host_lanes) {
            ft_add_separator(table);
            if (has_tx_taps) hal_get_tx_ffe_range(slice, lane, &tx_taps_range, NULL);
        }

        CredoLaneMode_t tx_mode = CR_LMODE_DISABLE, rx_mode;
        uint32_t tx_speed_kbps, rx_speed_kbps;
        CredoLanePrbsPattern_t tx_prbs, rx_prbs;
        CredoLaneTxState_t tx_state;
        CredoLaneCoupling_t input_mode;
        CredoLaneLoopbackMode_t lb_mode;
        int tx_pol, rx_pol, tx_gc, rx_gc, tx_pc, hw_tx_pc, rx_pc, tx_msb, rx_msb, tx_taps[tx_taps_range];
        int tx_prbs_en, rx_prbs_en;

        if (has_tx_mode) ERR_CATCH(hal_get_tx_lane_mode(slice, lane, &tx_mode), goto error_cleanup);
        ERR_CATCH(hal_get_lane_mode(slice, lane, &rx_mode), goto error_cleanup);
        if (rx_mode == CR_LMODE_DISABLE) continue;

        if (has_tx_mode) ERR_CATCH(hal_lane_get_tx_speed(slice, lane, &tx_speed_kbps), goto error_cleanup);
        ERR_CATCH(hal_fw_get_lane_speed(slice, lane, &rx_speed_kbps), goto error_cleanup);

        ERR_CATCH(hal_get_tx_polarity(slice, lane, &tx_pol), goto error_cleanup);
        ERR_CATCH(hal_get_rx_polarity(slice, lane, &rx_pol), goto error_cleanup);
        ERR_CATCH(hal_get_tx_gray_code(slice, lane, &tx_gc), goto error_cleanup);
        ERR_CATCH(hal_get_rx_gray_code(slice, lane, &rx_gc), goto error_cleanup);
        ERR_CATCH(hal_get_tx_precoder(slice, lane, &tx_pc), goto error_cleanup);
        if (has_hw_tx_pc) ERR_CATCH(hal_get_hw_tx_precoder(slice, lane, &hw_tx_pc), goto error_cleanup);
        ERR_CATCH(hal_get_rx_precoder(slice, lane, &rx_pc), goto error_cleanup);
        ERR_CATCH(hal_get_tx_msb(slice, lane, &tx_msb), goto error_cleanup);
        ERR_CATCH(hal_get_rx_msb(slice, lane, &rx_msb), goto error_cleanup);
        ERR_CATCH(hal_get_rx_input_mode(slice, lane, &input_mode), goto error_cleanup);
        if (has_tx_taps && tx_taps_range) ERR_CATCH(hal_get_tx_taps(slice, lane, tx_taps), goto error_cleanup);
        if (has_lb_mode) ERR_CATCH(hal_get_lane_loopback_mode(slice, lane, &lb_mode), goto error_cleanup);
        if (has_tx_state) ERR_CATCH(hal_lane_tx_status(slice, lane, &tx_state), goto error_cleanup);
        if (has_prbs) {
            sbsclear(tx_prbs_string);
            if ((has_tx_mode && (tx_mode == CR_LMODE_PAM4 || tx_mode == CR_LMODE_NRZ || tx_mode == CR_LMODE_PAM3)) ||
                (!has_tx_mode && (rx_mode == CR_LMODE_PAM4 || rx_mode == CR_LMODE_NRZ || rx_mode == CR_LMODE_PAM3))) {
                ERR_CATCH(hal_get_tx_prbs(slice, lane, &tx_prbs_en, &tx_prbs), goto error_cleanup);
                sbscatfmt(tx_prbs_string, tx_prbs_en == 0 ? "OFF  " : prbs_pattern_to_string(tx_prbs));
                sbscatfmt(tx_prbs_string, tx_prbs_en && tx_gc ? "Q" : "");
            }
            sbsclear(rx_prbs_string);
            if (rx_mode == CR_LMODE_PAM4 || rx_mode == CR_LMODE_NRZ || rx_mode == CR_LMODE_PAM3) {
                ERR_CATCH(hal_get_rx_prbs(slice, lane, &rx_prbs_en, &rx_prbs), goto error_cleanup);
                sbscatfmt(rx_prbs_string, rx_prbs_en == 0 ? "OFF  " : prbs_pattern_to_string(rx_prbs));
                sbscatfmt(rx_prbs_string, rx_prbs_en && rx_gc ? "Q" : "");
            }
        }

        ft_printf(table, "%s(%2d)", stringify_lane_id(slice, lane), lane);

        sbsclear(lmode_string);
        if (has_tx_mode) sbscatfmt(lmode_string, "%s,", tx_mode == CR_LMODE_OFF ? " " : lane_mode_to_string(tx_mode));
        sbscatfmt(lmode_string, "%s", rx_mode == CR_LMODE_OFF ? " " : lane_mode_to_string(rx_mode));
        ft_printf(table, "%s", sbsstr(lmode_string));

        if (has_tx_mode) {
            ft_printf(table, "%.3fG,%.3fG", tx_speed_kbps / 1e6, rx_speed_kbps / 1e6);
        } else {
            ft_printf(table, "%.3fG", rx_speed_kbps / 1e6);
        }

        if (has_tx_state) ft_printf(table, "%s", stringify_tx_state(tx_state));
        if (has_lb_mode) ft_printf(table, "%s", lb_mode == CR_LB_DISABLED ? " " : stringify_tx_loopback_mode(lb_mode));

        ft_printf(table, "%2d,%2d|%2d,%2d", tx_pol, rx_pol, tx_gc, rx_gc);
        if (has_hw_tx_pc)
            ft_printf(table, "%2d,%2d,%2d", tx_pc, hw_tx_pc, rx_pc);
        else
            ft_printf(table, "%2d,%2d", tx_pc, rx_pc);
        ft_printf(table, "%2d,%2d|%s", tx_msb, rx_msb, (input_mode == CR_COUPLING_DC) ? "DC" : "AC");

        if (has_tx_taps && tx_taps_range) {
            sbs* tx_taps_string = SBS1024("");

            for (int i = 0; i < tx_taps_range; i++) {
                sbscatfmt(tx_taps_string, "%s%i", i ? ", " : "", tx_taps[i]);
            }
            ft_printf(table, "%s", sbsstr(tx_taps_string));
        }

        if (has_prbs) {
            ft_printf(table, "%7s %7s", sbsstr(tx_prbs_string), sbsstr(rx_prbs_string));
        }

        ft_ln(table);
    }

    DISPF(D, "%s", ft_to_string(table));
    ret = CR_OK;

error_cleanup:
    ft_destroy_table(table);
    return ret;
}

static void list_commands(DisplayState_t* D, const DisplayCommand_t commands[], size_t command_len) {
    for (size_t i = 0; i < command_len; i++) {
        DISPF(D, "- %s\n", commands[i].name);
    }
}

static void display_old_command_help(DisplayState_t* D) {
    struct argparse argparser;
    struct argparse_option options[] = {OPT_HELP(), OPT_END()};
    argparse_init(&argparser, options, D->command->usages, 0, D->writer, D->ud);
    argparse_describe(&argparser, D->command->description, D->command->epilog);
    argparse_help_cb_no_exit(&argparser, options);
}

CredoError_t display_info(CredoSlice_t* slice, size_t argc, const char* argv[], DisplayState_t* D,
                          const DisplayCommand_t commands[], size_t command_len) {
    const DisplayCommand_t* command = NULL;

    if (argc == 0) {
        return CR_INVALID_ARGS;
    }

    const char* command_name = argv[0];

    // special help command
    if (strcmp(command_name, "help") == 0) {
        list_commands(D, commands, command_len);
        return CR_OK;
    }

    // find the command
    for (size_t i = 0; i < command_len; i++) {
        if (strcmp(commands[i].name, command_name) == 0) {
            command = &commands[i];
            break;
        }
    }
    if (command == NULL) {
        DISPF(D, "Invalid command '%s'\n", command_name);
        list_commands(D, commands, command_len);
        return CR_INVALID_ARGS;
    }
    D->command = command;

    // old style command
    if (command->description == NULL) {
        if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
            display_old_command_help(D);
            return CR_OK;
        }
        // old commands do not pass the command name as part of the arguments
        CredoError_t err = command->cb(slice, (int)argc - 1, argv + 1, D);
        if (err == CR_INVALID_ARGS) {
            display_old_command_help(D);
        }
        return err;
    }
    // new style command
    return command->cb(slice, (int)argc, argv, D);
}

enum argparse_exit display_parse_args(CredoSlice_t* slice, int argc, const char* argv[],
                                      struct argparse_option* options, const DisplayState_t* D) {
    struct argparse argparser;
    argparse_init(&argparser, options, D->command->usages, 0, D->writer, D->ud);
    argparse_describe(&argparser, D->command->description, D->command->epilog);
    return argparse_parse(&argparser, argc, argv);
}
