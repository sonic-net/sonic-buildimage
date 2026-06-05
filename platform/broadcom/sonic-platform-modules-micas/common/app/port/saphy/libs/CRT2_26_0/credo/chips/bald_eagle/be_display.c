#include "bald_eagle.h"
#include "be_device.h"
#include "be_functions.h"

#include "anlt/anlt.h"
#include "canary/canary_serdes.h"
#include "common/common_display.h"
#include "common/common_firmware.h"
#include "common/common_prbs.h"
#include "fec_analyzer/fec_analyzer.h"

#include "fort.h"
#include "parsing.h"
#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* FLAG_star = "* ";

static CredoError_t be_fw_port_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    static const char* link[] = {"Down", " Up "};
    unsigned rdy = 0;
    ERR_PROPS(be_fw_phy_ready(slice, &rdy));

    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, 0, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, 0, FT_CPROP_RIGHT_PADDING, 0);
    ft_printf_ln(table, "|||Link|SerDes Speed|Lane Group|");
    ft_printf_ln(table, "Port|MODE|Speed|Host Line|Host Line|Host Line|Flags");
    ft_add_separator(table);

    BeSlice_t* be_slice = (BeSlice_t*)slice;
    char host_lane_str[6], line_lane_str[6];
    for (int i = 0; i < BE_MAX_PORT; i++) {
        ft_printf(table, "%2d", i);
        BePortInfo_t* port_info = be_slice->port_info + i;
        CredoPortConfig_t* port_config = &port_info->port_config;
        if (port_info->configured) {
            unsigned mode, lane, speed_A, speed_B;
            lane = port_config->host_start_lane;
            ERR_CATCH(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_CONFIG_SEL, &mode), goto error_cleanup);
            ft_printf(table, "%s", be_fw_config_mode_string((mode >> 4) & 0xf));
            ft_printf(table, "%d", port_config->speed);

            int host_link = 1, line_link = 1;
            for (int i = 0; i < port_config->host_no_of_lanes; i++) {
                if (((rdy >> (port_config->host_start_lane + i)) & 0x1) == 0) {
                    host_link = 0;
                }
            }
            for (int i = 0; i < port_config->line_no_of_lanes; i++) {
                if (((rdy >> (port_config->line_start_lane + i)) & 0x1) == 0) {
                    line_link = 0;
                }
            }
            ft_printf(table, "%s %s", link[host_link], link[line_link]);

            unsigned start_A, start_B, end_A, end_B;
            start_A = port_config->host_start_lane;
            end_A = port_config->host_start_lane + port_config->host_no_of_lanes - 1;
            start_B = port_config->line_start_lane;
            end_B = port_config->line_start_lane + port_config->line_no_of_lanes - 1;

            ERR_CATCH(common_fw_get_speed_index(slice, start_A, &speed_A), goto error_cleanup);
            ERR_CATCH(common_fw_get_speed_index(slice, start_B, &speed_B), goto error_cleanup);
            ft_printf(table, "%s-%s", be_fw_speed_string(speed_A & 0xf), be_fw_speed_string(speed_B & 0xf));

            if (start_A == end_A)
                snprintf(host_lane_str, 6, "%d", start_A);
            else
                snprintf(host_lane_str, 6, "%2d-%-2d", start_A, end_A);

            if (start_B == end_B)
                snprintf(line_lane_str, 6, "%d", start_B);
            else
                snprintf(line_lane_str, 6, "%2d-%-2d", start_B, end_B);

            ft_printf(table, "%s %s", host_lane_str, line_lane_str);

            char flag_str[128] = {0};
            ERR_CATCH(port_flags_to_string(port_config->flags, flag_str, 128), goto error_cleanup);
            ft_printf(table, "%s", flag_str);
        }
        ft_ln(table);
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
error_cleanup:
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_FAIL;
}

static CredoError_t be_fw_serdes_control(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    // argv[argc++] = "-tx_state";  // omit tx_state as default
    // argv[argc++] = "-lb";        // omit loopback as default
    // argv[argc++] = "-prbs";      // omit prbs as default
    return common_fw_serdes_control(slice, argc, argv, D);
}

static CredoError_t be_fw_serdes_param(CredoSlice_t* slice, const int* lane_list, const split_display_t split_display,
                                       const DisplayState_t* D) {
    int laneId;
    unsigned top_pll_A_cap = 0, top_pll_B_cap = 0, top_pll_cap = 0;
    unsigned speed_index;
    const char* speed;
    int sd, rdy;
    unsigned adapt_done_all, adapt_done;
    int ppm = 0, tx_cap, rx_cap;
    char ppm_str[8] = {0};
    double chan_est;
    unsigned of, hf;
    unsigned ctle_val[2];
    unsigned agcgain[2];
    int skef_en, skef_val;
    int rx_dac;
    int eyes[3];
    double dfe_taps[3];
    int f13_val;
    int delta_val;
    unsigned int edge;
    int ffe_taps_extended[7];
    CredoLaneMode_t opt_mode;
    static const char* header_nrz =
        "Ln|Mode|Adp, ReAdp, LLost|AdpDone|PPM|%3d,%-3d|Est, OF, HF|Peaking, G1, G2|SK|DAC|1, 2, 3|F1, F2, F3|Del, "
        "Edge|K1, K2, K3, K4, S1,S2,Sf";
    static const char* header_pam4 =
        "Ln|Mode|Adp, ReAdp, LLost|AdpDone|PPM|%3d,%-3d|Est, OF, HF|Peaking, G1, G2|SK|DAC|1, 2, 3|F0, F1, F1/F0, "
        "F13|Del,Edge|K1, K2, K3, K4, S1,S2,Sf";

    unsigned fw_status;
#define FW_IS_RUNNING            (1)
#define CHK_FW_RUNNING_THEN(...) (fw_status == FW_IS_RUNNING) && (__VA_ARGS__)

    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);

    be_fw_get_status(slice, &fw_status);
    if (fw_status != FW_IS_RUNNING) DISPF(D, "Firmware is not running!\n");

    ERR_CATCH(be_get_top_pll_cap(slice, &top_pll_cap), goto error_cleanup);

    top_pll_A_cap = top_pll_cap >> 8;
    top_pll_B_cap = top_pll_cap & 0xFF;

    ERR_CATCH(be_fw_phy_ready(slice, &adapt_done_all), goto error_cleanup);

    int host_lane, line_lane;
    int separator_index = 0;

    // find separator_index between host and line side
    ERR_CATCH(be_get_lane_count(slice, &host_lane, &line_lane), goto error_cleanup);
    for (int i = 0, host_flag, pre_flag = lane_list[0] < host_lane; i < slice->desc->lane_count; i++) {
        if ((laneId = lane_list[i]) >= slice->desc->lane_count) break;

        host_flag = laneId < host_lane;
        if (pre_flag != host_flag) {
            if (separator_index == 0) {
                separator_index = i;
            } else {
                separator_index = 0;
                break;
            }
        }
        pre_flag = host_flag;
    }

    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((laneId = lane_list[i]) >= slice->desc->lane_count) break;
        if (separator_index != 0 && separator_index == i) ft_add_separator(table);
        if (fw_status == FW_IS_RUNNING) {
            ERR_CATCH(be_update_lane_mode(slice, laneId), goto error_cleanup);
        } else {
            unsigned pam4_en;
            ERR_CATCH(readRegLane(slice, laneId, REG_FLR_RX_PAM4_EN, &pam4_en), goto error_cleanup);
            ERR_CATCH(be_set_lane_mode(slice, laneId, (pam4_en == 1) ? CR_LMODE_PAM4 : CR_LMODE_NRZ),
                      goto error_cleanup);
        }
        ERR_CATCH(be_get_lane_mode(slice, laneId, &opt_mode), goto error_cleanup);

        if (i == 0) {
            /* print header. It's either NRZ or PAM4, even there could be other modes. */
            if (opt_mode == CR_LMODE_PAM4) {
                ft_printf_ln(table, header_pam4, top_pll_A_cap, top_pll_B_cap);
            } else {
                ft_printf_ln(table, header_nrz, top_pll_A_cap, top_pll_B_cap);
            }
        }
        if (opt_mode == CR_LMODE_DISABLE) {
            ft_printf_ln(table, "%02d|%s", laneId, " DIS");
            continue;
        }
        if (opt_mode != CR_LMODE_PAM4 && opt_mode != CR_LMODE_NRZ)
            opt_mode = CR_LMODE_OFF;  // keep between pam4, nrz, or off

        speed_index = SPEED_UNKNOWN;
        if (CHK_FW_RUNNING_THEN(common_fw_get_speed_index(slice, laneId, &speed_index) != CR_OK)) goto error_cleanup;

        speed = be_fw_speed_string(speed_index);

        if (opt_mode == CR_LMODE_OFF || speed_index == SPEED_OFF) {
            /* print empty line and continue */
            ft_printf_ln(table, "%02d|%s", laneId, " OFF");
            continue;
        }

        unsigned adapt_cnt = 0;
        if (CHK_FW_RUNNING_THEN(common_fw_get_adapt_count(slice, laneId, &adapt_cnt) != CR_OK)) goto error_cleanup;

        unsigned readapt_cnt = 0;
        if (CHK_FW_RUNNING_THEN(common_fw_get_readapt_count(slice, laneId, &readapt_cnt) != CR_OK)) goto error_cleanup;

        unsigned linklost_cnt = 0;
        if (CHK_FW_RUNNING_THEN(common_fw_get_link_lost_count(slice, laneId, &linklost_cnt) != CR_OK))
            goto error_cleanup;

        unsigned lt_on = 0;
        if (CHK_FW_RUNNING_THEN(be_fw_get_lane_link_training(slice, laneId, &lt_on) != CR_OK)) goto error_cleanup;
        char LT = (lt_on != 0) ? 'L' : ' ';

        ERR_CATCH(canary_get_signal_detect(slice, laneId, &sd), goto error_cleanup);
        ERR_CATCH(canary_get_lane_ready(slice, laneId, &rdy), goto error_cleanup);

        adapt_done = (adapt_done_all >> laneId) & 1;

        // tx_cap, rx_cap
        ERR_CATCH(canary_get_tx_cap(slice, laneId, &tx_cap), goto error_cleanup);
        ERR_CATCH(canary_get_rx_cap(slice, laneId, &rx_cap), goto error_cleanup);

        // chan_est,of,hf
        chan_est = of = hf = 0;
        if (CHK_FW_RUNNING_THEN((common_fw_get_channel_estimate(slice, laneId, &chan_est) != CR_OK) ||
                                (common_fw_get_of(slice, laneId, &of) != CR_OK) ||
                                (common_fw_get_hf(slice, laneId, &hf) != CR_OK)))
            goto error_cleanup;

        ERR_CATCH(canary_get_ctle(slice, laneId, ctle_val), goto error_cleanup);
        ERR_CATCH(canary_get_agcgain(slice, laneId, agcgain), goto error_cleanup);
        ERR_CATCH(canary_get_rx_skef(slice, laneId, &skef_en, &skef_val, NULL, NULL), goto error_cleanup);
        ERR_CATCH(canary_get_rx_dac(slice, laneId, &rx_dac), goto error_cleanup);

        ERR_CATCH(canary_get_f1over3(slice, laneId, &f13_val), goto error_cleanup);
        ERR_CATCH(canary_get_ffe_taps(slice, laneId, ffe_taps_extended),
                  goto error_cleanup);  // TODO: implement extended here

        ERR_CATCH(canary_get_delta_phase(slice, laneId, &delta_val), goto error_cleanup);
        ERR_CATCH(canary_get_edge(slice, laneId, &edge), goto error_cleanup);

        ERR_CATCH(hal_get_rx_ppm(slice, laneId, &ppm), goto error_cleanup);
        ppm_to_format_string(ppm, ppm_str);

        ft_printf(table, "%02d|%c%3s|%5d,%5d,%4d|%c%d,%d,%d%c|%4s|%3d,%-3d", laneId, LT, speed, adapt_cnt, readapt_cnt,
                  linklost_cnt, FLAG_star[sd], sd, rdy, adapt_done, FLAG_star[rdy], ppm_str, tx_cap, rx_cap);

        ft_printf(table, "%5.2f,%2d,%2d", chan_est, of, hf);
        ft_printf(table, "(%d,%-2d),%3d,%2d ", ctle_val[0], ctle_val[1], agcgain[0], agcgain[1]);
        ft_printf(table, "%d|%2d", skef_val, rx_dac);

        if (opt_mode == CR_LMODE_PAM4) {
            if (fw_status == FW_IS_RUNNING) {
                ERR_CATCH(common_fw_get_eye(slice, laneId, eyes), goto error_cleanup);
                ERR_CATCH(common_fw_get_dfe(slice, laneId, dfe_taps), goto error_cleanup);
            } else {
                ERR_CATCH(canary_get_eye(slice, laneId, eyes), goto error_cleanup);
                ERR_CATCH(canary_get_dfe(slice, laneId, dfe_taps), goto error_cleanup);
            }
            ft_printf(table, "%3d,%3d,%3d", eyes[0], eyes[1], eyes[2]);
            ft_printf(table, "%4.2f,%4.2f,%5.2f,%2d", dfe_taps[0], dfe_taps[1], dfe_taps[2], f13_val);
            ft_printf(table, "%3d,%4X", delta_val, edge);
            ft_printf(table, "%4d,%4d,%4d,%4d,%02X,%02X,%02X", ffe_taps_extended[0], ffe_taps_extended[1],
                      ffe_taps_extended[2], ffe_taps_extended[3], ffe_taps_extended[4], ffe_taps_extended[5],
                      ffe_taps_extended[6]);
        } else {
            ERR_CATCH(canary_get_eye(slice, laneId, eyes), goto error_cleanup);
            ERR_CATCH(canary_get_dfe(slice, laneId, dfe_taps), goto error_cleanup);
            ft_printf(table, "%4d", eyes[0]);
            ft_printf(table, "%4.0f,%4.0f, %4.0f", dfe_taps[0], dfe_taps[1], dfe_taps[2]);
            ft_printf(table, "%3d,%4X", delta_val, edge);
        }
        ft_ln(table);
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
error_cleanup:
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_FAIL;
}

static CredoError_t be_fw_serdes_param_parser(CredoSlice_t* slice, int argc, const char* argv[],
                                              const DisplayState_t* D) {
    return common_serdes_param_parser(slice, be_fw_serdes_param, argc, argv, D);
}

static CredoError_t be_fw_retimer_status(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    BeSlice_t* be_slice = (BeSlice_t*)slice;

    for (int port_id = 0; port_id < BE_MAX_PORT; port_id++) {
        BePortInfo_t* port_info = be_slice->port_info + port_id;
        CredoPortConfig_t* port_config = &port_info->port_config;

        if (port_info->configured == false) continue;

        unsigned mode;
        ERR_PROPS(hal_fw_debug_cmd(slice, port_config->host_start_lane, TOP_DEBUG, TOP_DEBUG_CONFIG_SEL, &mode));
        mode >>= 4;
        if (mode != MODE_RETIMER_NRZ && mode != MODE_RETIMER_PAM4 && mode != MODE_RETIMER_CROSS_NRZ &&
            mode != MODE_RETIMER_CROSS_PAM4)
            continue;

        unsigned fifo1, fifo2, fifo3, fifo4;
        for (int i = 0; i < port_config->line_no_of_lanes; i++) {
            int A = port_config->host_start_lane + i;
            int B = port_config->line_start_lane + i;
            ERR_PROPS(readReg(slice, REG_ADR_DIFF_A0(A), &fifo1));
            ERR_PROPS(readReg(slice, REG_ADR_DIFF_A1(A), &fifo2));
            ERR_PROPS(readReg(slice, REG_ADR_DIFF_B0(B), &fifo3));
            ERR_PROPS(readReg(slice, REG_ADR_DIFF_B1(B), &fifo4));
            DISPF(D, "L%02d Rx -->[%d,%d]--> Tx L%02d\n", A, gray_bin(fifo1), gray_bin(fifo2), B);
            DISPF(D, "    Tx <--[%d,%d]<-- Rx\n", gray_bin(fifo3), gray_bin(fifo4));
        }
    }
    return CR_OK;
}

CredoError_t be_fw_gearbox_status(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    unsigned fec_en;
    unsigned adapt_done_all;

    ft_table_t* table = ft_create_table();
    ft_table_t* adapt_table = ft_create_table();

    ERR_CATCH(readRegLane(slice, 0, REG_FEC_EN, &fec_en), goto error_cleanup);
    ERR_CATCH(be_fw_phy_ready(slice, &adapt_done_all), goto error_cleanup);

    // create lane adapt table
    ft_printf_ln(adapt_table, "Lane Adapt|0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15");
    ft_write(adapt_table, "");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        ft_printf(adapt_table, "%d", adapt_done_all & (1 << i) ? 1 : 0);
    }

    ft_printf_ln(table, "FEC|ALIGN|CORR ERR|UNCORR ERR|RX FIFO|TX FIFO");
    for (int i = 0; i < FEC_MAX_NUM; i++) {
        unsigned rx_aligned, corr_error_lower, corr_error_upper, uncorr_error_lower, uncorr_error_upper, fifo_rx_min,
            fifo_rx_cur, fifo_rx_max, fifo_tx_min, fifo_tx_cur, fifo_tx_max;
        char fec_side;
        int lane_num;
        if (i % 4 == 0) ft_add_separator(table);
        if (i < FEC_MAX_NUM / 2) {
            lane_num = i;
            fec_side = 'A';
        } else {
            lane_num = i - FEC_MAX_NUM / 2;
            fec_side = 'B';
        }
        ft_printf(table, "FEC %c%d", fec_side, lane_num);
        if (fec_en & (1 << i)) {
            if (i < FEC_MAX_NUM / 2) {
                lane_num = i;
                fec_side = 'A';
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_RX_ALIGNED_A, &rx_aligned), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_CORR_ERROR_LOWER_A, &corr_error_lower),
                          goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_CORR_ERROR_UPPER_A, &corr_error_upper),
                          goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_UNCORR_ERROR_LOWER_A, &uncorr_error_lower),
                          goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_UNCORR_ERROR_UPPER_A, &uncorr_error_upper),
                          goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_RX_FIFO_MIN_A, &fifo_rx_min), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_RX_FIFO_CUR_A, &fifo_rx_cur), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_RX_FIFO_MAX_A, &fifo_rx_max), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_TX_FIFO_MIN_A, &fifo_tx_min), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_TX_FIFO_CUR_A, &fifo_tx_cur), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_TX_FIFO_MAX_A, &fifo_tx_max), goto error_cleanup);
            } else {
                lane_num = i - (FEC_MAX_NUM / 2);
                fec_side = 'B';
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_RX_ALIGNED_B, &rx_aligned), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_CORR_ERROR_LOWER_B, &corr_error_lower),
                          goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_CORR_ERROR_UPPER_B, &corr_error_upper),
                          goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_UNCORR_ERROR_LOWER_B, &uncorr_error_lower),
                          goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_UNCORR_ERROR_UPPER_B, &uncorr_error_upper),
                          goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_RX_FIFO_MIN_B, &fifo_rx_min), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_RX_FIFO_CUR_B, &fifo_rx_cur), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_RX_FIFO_MAX_B, &fifo_rx_max), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_TX_FIFO_MIN_B, &fifo_tx_min), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_TX_FIFO_CUR_B, &fifo_tx_cur), goto error_cleanup);
                ERR_CATCH(readRegLane(slice, lane_num, REG_FEC_TX_FIFO_MAX_B, &fifo_tx_max), goto error_cleanup);
            }

            ft_printf(table, "%u|%9u|%9u|%4u, %4u, %4u|%4u, %4u, %4u", rx_aligned,
                      corr_error_lower + (corr_error_upper << 16), uncorr_error_lower + (uncorr_error_upper << 16),
                      fifo_rx_min, fifo_rx_cur, fifo_rx_max, fifo_tx_min, fifo_tx_cur, fifo_tx_max);
        }
        ft_ln(table);
    }
    DISPF(D, "%s", ft_to_string(adapt_table));
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(adapt_table);
    ft_destroy_table(table);
    return CR_OK;
error_cleanup:
    DISPF(D, "%s", ft_to_string(adapt_table));
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(adapt_table);
    ft_destroy_table(table);
    return CR_FAIL;
}

static CredoError_t be_fw_bitmux_status(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
#define A_SIDE_FIFO_START_IDX 0
#define B_SIDE_FIFO_START_IDX 4

#define PAM4_SPLIT_FIFO_START_IDX 0
#define PAM4_MERGE_FIFO_START_IDX 2

#define NRZ_SPLIT_FIFO_START_IDX 4
#define NRZ_MERGE_FIFO_START_IDX 6
    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_INVALID_ARGS;
    ft_printf_ln(table, "FIFO Lane|BitMux Status");
    ft_add_separator(table);

    uint32_t ctrl_val = 0;
    ERR_PROP(readReg(slice, REG_BM_CTRL_S, &ctrl_val));

    for (int ctrl_idx = 0; ctrl_idx < 8; ctrl_idx++) {
        if (((1 << ctrl_idx) & ctrl_val) == 0) continue;

        uint32_t fifo_idx = ctrl_idx % 4;
        uint32_t pam4_nrz_fifo_sel = 1 << (fifo_idx + 8);
        uint32_t fifo_ab_side_sel = (ctrl_idx < 4) ? B_SIDE_FIFO_START_IDX : A_SIDE_FIFO_START_IDX;
        uint32_t a_fifo_start_idx, b_fifo_start_idx;
        if (ctrl_idx < 4) {
            // 2A1B
            a_fifo_start_idx = (pam4_nrz_fifo_sel & ctrl_val) ? NRZ_MERGE_FIFO_START_IDX : PAM4_MERGE_FIFO_START_IDX;
            b_fifo_start_idx = (pam4_nrz_fifo_sel & ctrl_val) ? NRZ_SPLIT_FIFO_START_IDX : PAM4_SPLIT_FIFO_START_IDX;
        } else {
            // 1A2B
            a_fifo_start_idx = (pam4_nrz_fifo_sel & ctrl_val) ? NRZ_SPLIT_FIFO_START_IDX : PAM4_SPLIT_FIFO_START_IDX;
            b_fifo_start_idx = (pam4_nrz_fifo_sel & ctrl_val) ? NRZ_MERGE_FIFO_START_IDX : PAM4_MERGE_FIFO_START_IDX;
        }

        // select A or B side fifo
        ERR_PROP(writeReg(slice, REG_BM_BUF_SEL_LANE, fifo_ab_side_sel + fifo_idx));

        char c;
        uint32_t fifo_val, max, min, cur;

        c = (ctrl_idx < 4) ? 'M' : 'S';
        for (int i = 0; i < 2; i++) {
            ERR_PROP(writeReg(slice, REG_BM_BUF_SEL_SLICE, a_fifo_start_idx + i));
            ERR_PROP(readReg(slice, REG_BM_BUF_SEL_READ, &fifo_val));
            max = gray_bin(fifo_val & 0xf);
            min = gray_bin((fifo_val >> 4) & 0xf);
            cur = gray_bin((fifo_val >> 8) & 0xf);

            if (i == 0) {
                ft_printf_ln(table, "A%u|%c%u -> cur: %u, min: %u, max: %u", fifo_idx, c, i, cur, min, max);
            } else {
                ft_printf_ln(table, "|%c%u -> cur: %u, min: %u, max: %u", c, i, cur, min, max);
            }
        }

        c = (fifo_idx < 4) ? 'S' : 'M';
        for (int i = 0; i < 2; i++) {
            ERR_PROP(writeReg(slice, REG_BM_BUF_SEL_SLICE, b_fifo_start_idx + i));
            ERR_PROP(readReg(slice, REG_BM_BUF_SEL_READ, &fifo_val));
            max = gray_bin(fifo_val & 0xf);
            min = gray_bin((fifo_val >> 4) & 0xf);
            cur = gray_bin((fifo_val >> 8) & 0xf);
            if (i == 0) {
                ft_printf_ln(table, "B%u|%c%u -> cur: %u, min: %u, max: %u", fifo_idx, c, i, cur, min, max);
            } else {
                ft_printf_ln(table, "|%c%u -> cur: %u, min: %u, max: %u", c, i, cur, min, max);
            }
        }
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

static CredoError_t be_fw_dump_anlt(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    int lane_list[CHIP_LANES + 1] = {CHIP_LANES};
    int lane = 0, idx = 0;
    for (lane = HOST_LANES; lane < slice->desc->lane_count; lane++) {
        unsigned lane_mode = CR_LMODE_OFF;
        ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
        if (lane_mode == CR_LMODE_DISABLE) continue;

        unsigned lt_on = 0;
        ERR_PROPS(be_fw_get_lane_link_training(slice, lane, &lt_on));
        if (lt_on) {
            lane_list[idx++] = lane;
        }
    }
    lane_list[idx] = CHIP_LANES;

    unsigned an_state[CHIP_LANES] = {0};
    uint64_t tx_pages[CHIP_LANES][9] = {{0}}, rx_pages[CHIP_LANES][9] = {{0}};

    int page_count = 0;  // dummy
    for (int idx = 0; idx < slice->desc->lane_count; idx++) {
        if ((lane = lane_list[idx]) >= slice->desc->lane_count) break;

        ERR_PROP(be_fw_get_an_state(slice, lane, &an_state[idx]));
        ERR_PROP(common_get_autoneg_exchanged_pages(slice, lane, &page_count, tx_pages[idx], rx_pages[idx]));
    }

    ERR_PROP(common_dump_anlt_pages(slice, lane_list, an_state, tx_pages, rx_pages, D));
    ERR_PROP(common_dump_anlt_detail(slice, lane_list, an_state, tx_pages, rx_pages, D));
    return CR_OK;
}

CredoError_t be_fw_dump_isi(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc == 1);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);
    int data[16] = {0};
    ERR_PROPS(common_fw_get_isi(slice, lane, data));

    ft_table_t* table = ft_create_table();
    ft_printf_ln(table, "fm2|fm1|f2|f3|f4|f5|f6|f7|f8|f9|f10|f11|f12|f13|f14|f15");
    for (int i = 0; i < 16; i++) {
        ft_printf(table, "%4d", data[i]);
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

CredoError_t be_fec_analyzer_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc >= 1);
    unsigned seconds = 0;
    ARGPARSE_STRTOUL(0, seconds);

    int lane_list[CHIP_LANES] = {CHIP_LANES};

    parsing_string_to_lane_list(argc > 1 ? argv[1] : "all", lane_list, slice->desc->lane_count);

    unsigned hist[CHIP_LANES][16] = {{0}};
    CredoLaneMode_t lane_mode[CHIP_LANES] = {CR_LMODE_OFF};

    CredoFecAnalyzerConfig_t config_nrz = {
        .codeword_size = 5280,
        .symbol_size = 10,
        .threshold = 7,
    };

    CredoFecAnalyzerConfig_t config_pam4 = {
        .codeword_size = 5440,
        .symbol_size = 10,
        .threshold = 15,
    };

    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        ERR_PROPS(be_get_lane_mode(slice, lane, &lane_mode[lane]));
    }

    int lane = 0;
    for (int group = 0; group < 4; group++) {
        for (int i = 0; i < slice->desc->lane_count; i++) {
            if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
            if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane])) continue;
            ERR_PROPS(fec_analyzer_set_hist_group(slice, lane, group));
            ERR_PROPS(fec_analyzer_set_config(slice, lane, true,
                                              (lane_mode[lane] == CR_LMODE_NRZ) ? &config_nrz : &config_pam4));
        }

        sleep_ms(seconds * 1000);

        for (int i = 0; i < slice->desc->lane_count; i++) {
            if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
            if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane])) continue;
            ERR_PROPS(fec_analyzer_get_hist_counter(slice, lane, &hist[lane][group * 4]));
        }
    }

    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane])) continue;
        ERR_PROPS(fec_analyzer_set_config(slice, lane, false,
                                          (lane_mode[lane] == CR_LMODE_NRZ) ? &config_nrz : &config_pam4));
    }

    // display table
    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);

    ft_write(table, "Histogram");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        ft_printf(table, "Lane %2d", lane);
    }
    ft_ln(table);

    for (int row = 0; row < 16; row++) {
        ft_printf(table, "Sei >= %2d", row);
        for (int i = 0; i < slice->desc->lane_count; i++) {
            if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
            if (IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane])) {
                ft_printf(table, "%10u", hist[lane][row]);
            } else {
                ft_printf(table, "-");
            }
        }
        ft_ln(table);
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

static const DisplayCommand_t be_slice_cmds[] = {
    ADD_DISPLAY_CMD("slice_info", common_slice_info, DISPLAY_CMD_DOC("", "Display top level slice information")),
    ADD_DISPLAY_CMD("isi", be_fw_dump_isi, DISPLAY_CMD_DOC("<lane>", "Display Intersymbol interference table")),
    ADD_DISPLAY_CMD(
        "fec_analyzer_all", be_fec_analyzer_info,
        DISPLAY_CMD_DOC("<seconds>", "Display general prbs fec analzyer information for all lanes"),
        DISPLAY_CMD_DOC("<seconds> <lane_list>", "Display general prbs fec analzyer information for selected lanes")),
    DISPLAY_ARGP_CMD("rx_monitor", common_rx_monitor, "Display PRBS and FEC Analyzer performance metrics",
                     DISPLAY_ARGP_USES("[options]", "[options] <lanelist>"), NULL),
    ADD_DISPLAY_CMD("prbs_phase_error", common_prbs_phase_error,
                    DISPLAY_CMD_DOC("", "Display prbs phase error information, default is lane 0, 1s"),
                    DISPLAY_CMD_DOC("<lane>", "Select lane"),
                    DISPLAY_CMD_DOC("<lane> <seconds>", "Select lane and duration")),
    ADD_DISPLAY_CMD("anlt", be_fw_dump_anlt, DISPLAY_CMD_DOC("", "Display ANLT status and parsing result")),
    ADD_DISPLAY_CMD("fw_exit_codes", common_fw_dump_exit_codes,
                    DISPLAY_CMD_DOC("", "Display firmware exit codes for all lanes")),

    ADD_DISPLAY_CMD("serdes_param", be_fw_serdes_param_parser,
                    DISPLAY_CMD_DOC("", "Display SerDes parameters table for all lanes"),
                    DISPLAY_CMD_DOC("<lane_list>", "Display SerDes parameters table for selected lanes")),
    ADD_DISPLAY_CMD("serdes_control", be_fw_serdes_control,
                    DISPLAY_CMD_DOC("", "Display SerDes control parameters (non-firmware controlled) for all lanes")),
    ADD_DISPLAY_CMD("retimer_status", be_fw_retimer_status,
                    DISPLAY_CMD_DOC("", "Display retimer status for all ports")),
    ADD_DISPLAY_CMD("gearbox_status", be_fw_gearbox_status,
                    DISPLAY_CMD_DOC("", "Display gearbox status for all ports")),
    ADD_DISPLAY_CMD("bitmux_status", be_fw_bitmux_status, DISPLAY_CMD_DOC("", "Display bitmux status for all ports")),
    ADD_DISPLAY_CMD("port_info", be_fw_port_info,
                    DISPLAY_CMD_DOC("", "Display port configuration information for all ports")),
    ADD_DISPLAY_CMD("fw_reg", common_dump_fw_reg, DISPLAY_CMD_DOC("", "Display firmware register")),
};

CredoError_t be_display_info(CredoSlice_t* slice, const char* argv[], size_t argc, CredoDisplayWriter_t writer,
                             void* ud) {
    DisplayState_t D = {.writer = writer, .ud = ud};
    return display_info(slice, argc, argv, &D, be_slice_cmds, COUNT_OF(be_slice_cmds));
}
