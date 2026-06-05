#include "bh_device.h"
#include "bh_functions.h"
#include "blackhawk.h"

#include "anlt/anlt.h"
#include "common/common_display.h"
#include "common/common_firmware.h"
#include "common/common_fw_dump.h"
#include "common/common_prbs.h"
#include "condor_lp/condor_lp_serdes.h"
#include "fec_analyzer/fec_analyzer.h"
#include "rsfec/rsfec.h"

#include "fort.h"
#include "parsing.h"
#include "sbs.h"
#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local definition

typedef enum {
    SEP_NON_SPLIT,
    SEP_SPLIT_TOP,
    SEP_SPLIT_MIDDLE,
    SEP_SPLIT_BOTTOM,
} sep_enum_t;

static const char* FLAG_star = "* ";

const char* nrz_timer_name[] = {"# timer start",
                                "SD",
                                "atten search",
                                "agcgain search0",
                                "CA",
                                "agcgain search1",
                                "coarse_up",
                                "wait link",
                                "ctle search",
                                "",
                                "",
                                "",
                                "",
                                "",
                                "",
                                "",
                                "",
                                "",
                                "",
                                ""};

const char* pam4_timer_name[] = {"# timer start",
                                 "attenuation search",
                                 "agcgain search0",
                                 "CA",
                                 "agcgain search and channel tap analyzer",
                                 "f13 search",
                                 "ctle search",
                                 "skef search and agcgain search",
                                 "dac search",
                                 "channel tap analyzer and f13 search",
                                 "ppm check and cntr1",
                                 "delta search",
                                 "eye check",
                                 "ffe dearch",
                                 "cntr2 and link up and eye check",
                                 "",
                                 "",
                                 "",
                                 "",
                                 ""};

static dump_info_t nrz_dump_list[] = {
    DUMP_UNSIGNED_HEX("rx_state", NRZ_DEBUG, RX_STATE, 1),
    // DUMP_UNSIGNED("attn_of", NRZ_DEBUG, ATTN_OF, 5),
    DUMP_UNSIGNED("dc_search_cnt", NRZ_DEBUG, DC_SEARCH_CNT, 16),
    DUMP_UNSIGNED("of_cnt", NRZ_DEBUG, CA_OF_CNT, 8),
    DUMP_UNSIGNED("hf_cnt", NRZ_DEBUG, CA_HF_CNT, 8),
    DUMP_UNSIGNED("final_of", NRZ_DEBUG, RX_OF, 1),
    DUMP_UNSIGNED("final_hf", NRZ_DEBUG, RX_HF, 1),
    DUMP_FLOAT("ratio", NRZ_DEBUG, RX_RATIO, 1, 8),
    DUMP_UNSIGNED("ctle_em", NRZ_DEBUG, CTLE_EM, 4),
    DUMP_MATRIX(SIGNED, "dfe", NRZ_DEBUG, DFE, 9, 3),
    // DUMP_TIMERS("timers", NRZ_DEBUG, TIMERS, 20, &nrz_timer_name[0]),
    DUMP_UNSIGNED("time_crashed", NRZ_DEBUG, TIME_CRASHED, 1),
    DUMP_UNSIGNED("time_signal_recover", NRZ_DEBUG, TIME_SIGNAL_RECOVER, 1),
    DUMP_UNSIGNED("time_link_back", NRZ_DEBUG, TIME_LINK_BACK, 1),
    DUMP_UNSIGNED("recover_count", NRZ_DEBUG, RECOVER_COUNT, 1),
    DUMP_SIGNED("recover_delta", NRZ_DEBUG, RECOVER_DELTA, 1),
    DUMP_SIGNED("recover_eye", NRZ_DEBUG, RECOVER_EYE_RAW, 1),
    DUMP_SIGNED("recover_ths", NRZ_DEBUG, RECOVER_THS, 8),
};

static int nrz_dump_len = sizeof(nrz_dump_list) / sizeof(dump_info_t);

static dump_info_t pam4_dump_list[] = {
    DUMP_UNSIGNED_HEX("rx_state", PAM4_DEBUG, RX_STATE, 1),
    DUMP_UNSIGNED("attn_of", PAM4_DEBUG, ATTN_OF, 5),
    DUMP_UNSIGNED("CA hf_cnt", PAM4_DEBUG, CA_HF_CNT, 8),
    DUMP_UNSIGNED("CA cnt3", PAM4_DEBUG, CA_CNT3, 8),
    DUMP_UNSIGNED("CA cnt0", PAM4_DEBUG, CA_CNT0, 8),
    DUMP_UNSIGNED("final_of", PAM4_DEBUG, RX_OF, 1),
    DUMP_UNSIGNED("final_hf", PAM4_DEBUG, RX_HF, 1),
    DUMP_FLOAT("ratio", PAM4_DEBUG, RX_RATIO, 1, 8),
    DUMP_UNSIGNED("f13_init", PAM4_DEBUG, F13_INIT, 1),
    DUMP_UNSIGNED("skef_isi2", PAM4_DEBUG, SKEF_ISI2, 20),
    DUMP_UNSIGNED_SPLIT("skef record", PAM4_DEBUG, SKEF_RECORD, 20, 4, true),
    DUMP_UNSIGNED_SPLIT("skef best", PAM4_DEBUG, SKEF_BEST, 1, 4, true),
    DUMP_SIGNED_DIV("isi ctle srch1", PAM4_DEBUG, ISI_CTLE_SEARCH1, 4, 3, false),
    DUMP_SIGNED_DIV("isi ctle srch2", PAM4_DEBUG, ISI_CTLE_SEARCH2, 4, 3, false),
    DUMP_UNSIGNED_SPLIT("ctle search", PAM4_DEBUG, CTLE_SEARCH, 3, 8, false),
    DUMP_SIGNED_DIV("delta fm1", PAM4_DEBUG, DELTA_FM1, 10, 5, false),
    DUMP_SIGNED("delta", PAM4_DEBUG, DELTA_DELTA, 10),
    DUMP_TIMERS("timers", PAM4_DEBUG, TIMERS, 20, &pam4_timer_name[0]),
#if 0  // DEBUG LF
    DUMP_MATRIX(SIGNED,"smart_check_ths", PAM4_DEBUG, SMART_CHECK_THS, 60, 5),
    DUMP_MATRIX(SIGNED, "force_ths", PAM4_DEBUG, FORCE_THS, 60, 5),
    DUMP_MATRIX(SIGNED, "plus_margin", PAM4_DEBUG, PLUS_MARGIN, 60, 5),
    DUMP_MATRIX(SIGNED, "minus_margin", PAM4_DEBUG, MINUS_MARGIN, 60, 5),
    DUMP_MATRIX(UNSIGNED_HEX, "lf_result", PAM4_DEBUG, LF_RESULT, 60, 5),
    DUMP_MATRIX(SIGNED, "em_debug", PAM4_DEBUG, EM_DEBUG, 60, 5),
#endif
};

static int pam4_dump_len = sizeof(pam4_dump_list) / sizeof(dump_info_t);

static CredoError_t bh_fw_port_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    static const char* link[] = {"Down", " Up "};

    unsigned rdy = 0, port_config_mask = 0;
    ERR_PROPS(bh_fw_phy_ready(slice, &rdy));
    ERR_PROPS(bh_fw_port_config_mask(slice, &port_config_mask));

    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, 0, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, 0, FT_CPROP_RIGHT_PADDING, 0);
    ft_printf_ln(table, "|||Link|SerDes Speed|Lane Group|FEC|");
    ft_printf_ln(table, "Port|MODE|Speed|Host Line|Host Line|Host Line|Host Line|Flags");
    ft_add_separator(table);

    char host_lane_str[32], line_lane_str[32];
    for (int port_id = 0; port_id < BH_MAX_PORT; port_id++) {
        ft_printf(table, "%2d", port_id);
        CredoPortConfig_t port_config = {.port_id = CR_PORT_UNCONFIGURED};
        if (port_config_mask & (1 << port_id)) {
            ERR_PROPS(bh_port_query(slice, port_id, &port_config));
        }

        if (port_config.port_id != CR_PORT_UNCONFIGURED) {
            unsigned mode, lane, speed_A, speed_B;
            lane = port_config.host_start_lane;
            ERR_CATCH(common_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_CONFIG_SEL, &mode), goto cleanup_error);
            ft_printf(table, "%s", bh_fw_config_mode_string(mode));

            ft_printf(table, "%d", port_config.speed);

            int port_link[2] = {0};
            ERR_CATCH(bh_port_link_state_internal(slice, &port_config, rdy, CR_SIDE_HOST, port_link + 0),
                      goto cleanup_error);
            ERR_CATCH(bh_port_link_state_internal(slice, &port_config, rdy, CR_SIDE_LINE, port_link + 1),
                      goto cleanup_error);
            ft_printf(table, "%s %s", link[port_link[0]], link[port_link[1]]);

            unsigned start_A, start_B, end_A, end_B;
            start_A = port_config.host_start_lane;
            end_A = port_config.host_start_lane + port_config.host_no_of_lanes - 1;
            start_B = port_config.line_start_lane;
            end_B = port_config.line_start_lane + port_config.line_no_of_lanes - 1;

            ERR_CATCH(common_fw_get_speed_index(slice, start_A, &speed_A), goto cleanup_error);
            ERR_CATCH(common_fw_get_speed_index(slice, start_B, &speed_B), goto cleanup_error);
            ft_printf(table, "%s %s", bh_fw_speed_string(speed_A), bh_fw_speed_string(speed_B));

            if (start_A == end_A)
                snprintf(host_lane_str, 32, "%s(%d)", stringify_lane_id(slice, start_A), start_A);
            else
                snprintf(host_lane_str, 32, "%s-%s(%d-%-d)", stringify_lane_id(slice, start_A),
                         stringify_lane_id(slice, end_A), start_A, end_A);

            if (start_B == end_B)
                snprintf(line_lane_str, 32, "%s(%d)", stringify_lane_id(slice, start_B), start_B);
            else
                snprintf(line_lane_str, 32, "%s-%s(%2d-%-2d)", stringify_lane_id(slice, start_B),
                         stringify_lane_id(slice, end_B), start_B, end_B);

            ft_printf(table, "%s %s", host_lane_str, line_lane_str);

            ft_printf(table, "%s %s", fec_to_string(port_config.host_fec_type),
                      fec_to_string(port_config.line_fec_type));

            char flag_str[128] = {0};
            ERR_CATCH(port_flags_to_string(port_config.flags, flag_str, 128), goto cleanup_error);
            ft_printf(table, "%s", flag_str);
        }
        ft_ln(table);
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
cleanup_error:
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_FAIL;
}

static CredoError_t bh_fw_serdes_control(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    return common_fw_serdes_control(slice, argc, argv, D);
}

static CredoError_t bh_fw_serdes_param(CredoSlice_t* slice, const int* lane_list, const split_display_t split_display,
                                       const DisplayState_t* D) {
    CredoLaneMode_t lane_mode;
    unsigned top_pll_cap = 0;
    int laneId;
    unsigned speed_index;
    const char* speed;
    int sd, rdy;
    unsigned adapt_done_all, adapt_done;
    unsigned optical_mode_all, optical_mode;
    int ppm = 0, tx_cap, rx_cap;
    char ppm_str[8] = {0};
    unsigned ctle_val[2];
    unsigned agcgain[2];
    unsigned tia1_bias;
    int skef_en, skef_val, skef_addcap, skef_gain;
    int rx_dac;
    int eyes[3];
    double dfe_taps[3];
    int f13_val = 0, delta_val = 0;
    int atten_en = 0, atten_agcgain = 0;
    unsigned int edge;
    int ffe_taps[8];
    unsigned fw_status;
#define FW_IS_RUNNING            (1)
#define CHK_FW_RUNNING_THEN(...) (fw_status == FW_IS_RUNNING) && (__VA_ARGS__)

    ft_table_t* table = ft_create_table();
    ft_table_t* table2 = ft_create_table();

    ft_table_t* secondary_table = table;

    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table2, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    if (split_display == NON_SPLIT_DISPLAY) {
        ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_LEFT_PADDING, 0);
        ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_RIGHT_PADDING, 0);
    }

    bh_fw_get_status(slice, &fw_status);
    if (fw_status != FW_IS_RUNNING) DISPF(D, "WARNING:Firmware is NOT RUNNING or FREEZE!\n");

    ERR_CATCH(bh_get_top_pll_cap(slice, &top_pll_cap), goto error_cleanup);

    ERR_CATCH(bh_get_lane_mode(slice, 0, &lane_mode), goto error_cleanup);

    if (split_display == NON_SPLIT_DISPLAY) {
        ft_printf_ln(table, "|||COUNTERS|SD,RDY,|FRQ|PLL Cal|CHANNEL|CTLE|Skef||Atten|EYE MARGIN|DFE|TIMING|FFE Taps");
        // print shared second topic
        ft_printf(table,
                  "Ln|Mode|Flg|Adp,ReAdp,LLost|AdpDone|PPM|%5d|Est,OF,HF|Peaking, G1, G2|degen,gain|DAC|3db,Agc,tia1",
                  top_pll_cap);
        if (lane_mode == CR_LMODE_PAM4) {
            ft_printf_ln(table, "1 , 2 , 3|F0, F1, F1/F0, F13|Del,Edge|K1, K2, K3, K4, K5, S0, S1, S2");
        } else {
            ft_printf_ln(table, "1 , 2 , 3|F1, F2, F3|Del,Edge|K1, K2, K3, K4, K5, S0, S1, S2");
        }
        ft_add_separator(table);  // signify end of header
    } else {
        ft_printf_ln(table, "|||COUNTERS|SD,RDY,|FRQ|PLL Cal|CHANNEL|CTLE|Skef||Atten");
        // print shared second topic
        ft_printf_ln(
            table, "Ln|Mode|Flg|Adp,ReAdp,LLost|AdpDone|PPM|%5d|Est,OF,HF|Peaking, G1, G2|degen,gain|DAC|3db,Agc,tia1",
            top_pll_cap);

        ft_printf_ln(table2, "Ln|EYE MARGIN|DFE|TIMING|FFE Taps");
        if (lane_mode == CR_LMODE_PAM4) {
            ft_printf_ln(table2, "|1 , 2 , 3|F0, F1, F1/F0, F13|Del,Edge|K1, K2, K3, K4, K5, S0, S1, S2");
        } else {
            ft_printf_ln(table2, "|1 , 2 , 3|F1, F2, F3|Del,Edge|K1, K2, K3, K4, K5, S0, S1, S2");
        }
        ft_add_separator(table);   // signify end of header
        ft_add_separator(table2);  // signify end of header

        secondary_table = table2;  // specify to print to table 2 instead
    }

    ERR_CATCH(bh_fw_phy_ready(slice, &adapt_done_all), goto error_cleanup);
    ERR_CATCH(bh_fw_optical_mode(slice, &optical_mode_all), goto error_cleanup);

    int host_lane, line_lane;
    int separator_index = 0;

    // find separator_index between host and line side
    ERR_CATCH(bh_get_lane_count(slice, &host_lane, &line_lane), goto error_cleanup);
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
        ft_set_cell_prop(table, FT_CUR_ROW, 1, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
        if ((laneId = lane_list[i]) >= slice->desc->lane_count) break;
        if (separator_index != 0 && separator_index == i) {
            ft_add_separator(table);
            ft_add_separator(table2);
        }
        ERR_CATCH(bh_get_lane_mode(slice, laneId, &lane_mode), goto error_cleanup);

        speed_index = SPEED_UNKNOWN;
        if (CHK_FW_RUNNING_THEN(common_fw_get_speed_index(slice, laneId, &speed_index) != CR_OK)) goto error_cleanup;

        if (lane_mode == CR_LMODE_OFF) speed_index = SPEED_OFF;
        speed = bh_fw_speed_string(speed_index);

        if (lane_mode == CR_LMODE_OFF) {
            /* print empty line and continue */
            if (split_display == NON_SPLIT_DISPLAY) {
                ft_printf_ln(table, "%s(%2d)", stringify_lane_id(slice, laneId), laneId);
            } else {
                ft_printf_ln(table, "%s(%2d)", stringify_lane_id(slice, laneId), laneId);
                ft_printf_ln(table2, "%s(%2d)", stringify_lane_id(slice, laneId), laneId);
            }
            if (i == 7) {
                ft_add_separator(table);
                ft_add_separator(table2);
            }
            continue;
        }

        unsigned an_on = 0, lt_on = 0;
        ERR_PROP(bh_fw_get_anlt(slice, laneId, &an_on, &lt_on));

        unsigned adapt_cnt = 0;
        if (CHK_FW_RUNNING_THEN(common_fw_get_adapt_count(slice, laneId, &adapt_cnt) != CR_OK)) goto error_cleanup;

        unsigned readapt_cnt = 0;
        if (CHK_FW_RUNNING_THEN(common_fw_get_readapt_count(slice, laneId, &readapt_cnt) != CR_OK)) goto error_cleanup;

        unsigned linklost_cnt = 0;
        if (CHK_FW_RUNNING_THEN(common_fw_get_link_lost_count(slice, laneId, &linklost_cnt) != CR_OK))
            goto error_cleanup;

        ERR_CATCH(condor_lp_get_rx_sd(slice, laneId, &sd), goto error_cleanup);
        ERR_CATCH(condor_lp_get_rx_lane_ready(slice, laneId, &rdy), goto error_cleanup);

        adapt_done = (adapt_done_all >> laneId) & 1;
        optical_mode = (optical_mode_all >> laneId) & 1;

        // tx_cap, rx_cap
        ERR_CATCH(condor_lp_get_tx_cap(slice, laneId, &tx_cap), goto error_cleanup);
        ERR_CATCH(condor_lp_get_rx_cap(slice, laneId, &rx_cap), goto error_cleanup);

        // chan_est,of,hf
        double chan_est;
        unsigned of = 0, hf = 0;
        if (CHK_FW_RUNNING_THEN((common_fw_get_channel_estimate(slice, laneId, &chan_est) != CR_OK) ||
                                (common_fw_get_of(slice, laneId, &of) != CR_OK) ||
                                (common_fw_get_hf(slice, laneId, &hf) != CR_OK)))
            goto error_cleanup;
        if (of > 63) of = 99;
        if (hf > 63) hf = 99;

        ERR_CATCH(condor_lp_get_rx_ctle(slice, laneId, ctle_val), goto error_cleanup);
        ERR_CATCH(condor_lp_get_agcgain(slice, laneId, agcgain), goto error_cleanup);
        ERR_CATCH(condor_lp_get_rx_skef(slice, laneId, &skef_en, &skef_val, &skef_addcap, &skef_gain),
                  goto error_cleanup);
        ERR_CATCH(condor_lp_get_rx_dac(slice, laneId, &rx_dac), goto error_cleanup);

        ERR_CATCH(condor_lp_get_f1over3(slice, laneId, &f13_val), goto error_cleanup);
        ERR_CATCH(condor_lp_get_rx_ffe(slice, laneId, ffe_taps), goto error_cleanup);

        ERR_CATCH(condor_lp_get_rx_delta(slice, laneId, &delta_val), goto error_cleanup);
        ERR_CATCH(condor_lp_get_edge(slice, laneId, &edge), goto error_cleanup);

        ERR_CATCH(condor_lp_get_rx_atten_en3db(slice, laneId, &atten_en), goto error_cleanup);
        ERR_CATCH(condor_lp_get_rx_atten_agcgain(slice, laneId, &atten_agcgain), goto error_cleanup);
        ERR_CATCH(condor_lp_get_rx_tia1_bias(slice, laneId, &tia1_bias), goto error_cleanup);

        ERR_CATCH(bh_get_rx_ppm(slice, laneId, &ppm), goto error_cleanup);
        ppm_to_format_string(ppm, ppm_str);
        sbs* lane_flag = SBS128("");
        sbscatfmt(lane_flag, "%s%s%s", an_on ? "AN," : "", lt_on ? "LT," : "", optical_mode ? "OPT," : "");
        sbsrange(lane_flag, 0, -2);  // remove last comma

        ft_printf(table, "%s(%2d)|%3s|%s|%5d,%5d,%4d|%c%d,%d,%d%c|%4s|%3d,%3d", stringify_lane_id(slice, laneId),
                  laneId, speed, sbsstr(lane_flag), adapt_cnt, readapt_cnt, linklost_cnt, FLAG_star[sd], sd, rdy,
                  adapt_done, FLAG_star[rdy], ppm_str, tx_cap, rx_cap);

        ft_printf(table, "%5.2f,%2d,%2d", chan_est, of, hf);
        ft_printf(table, "(%2d,%-2d),%3d,%3d", ctle_val[0], ctle_val[1], agcgain[0], agcgain[1]);

        if (skef_en)
            ft_printf(table, "%5d,%4d", skef_val, skef_gain);
        else
            ft_write(table, "");

        ft_printf(table, "%2d", rx_dac);
        ft_printf(table, "%3d,%3d,%2d", atten_en, atten_agcgain, tia1_bias);

        if (split_display == SPLIT_DISPLAY) {
            ft_printf(table2, "%2d", laneId);
        }

        if (lane_mode == CR_LMODE_PAM4) {
            if (fw_status == FW_IS_RUNNING) {
                ERR_CATCH(common_fw_get_eye(slice, laneId, eyes), goto error_cleanup);
                ERR_CATCH(common_fw_get_dfe(slice, laneId, dfe_taps), goto error_cleanup);
            } else {
                ERR_CATCH(condor_lp_get_rx_eye_pam4(slice, laneId, eyes), goto error_cleanup);
                ERR_CATCH(condor_lp_get_rx_dfe_pam4(slice, laneId, dfe_taps), goto error_cleanup);
            }
            ft_printf(secondary_table, "%4d,%3d,%3d", eyes[0], eyes[1], eyes[2]);
            ft_printf(secondary_table, "%4.2f,%5.2f,%5.2f,%2d", dfe_taps[0], dfe_taps[1], dfe_taps[2], f13_val);
            ft_printf(secondary_table, "%3d,%4X", delta_val, edge);

            sbs* ffe_buffer = SBS128("");
            for (int i = 0; i < 8; i++) {
                sbscatprintf(ffe_buffer, "%c%02X,", ffe_taps[i] >= 0 ? ' ' : '-',
                             ffe_taps[i] >= 0 ? ffe_taps[i] : abs(ffe_taps[i]) - 1);
            }
            sbsrange(ffe_buffer, 0, -2);
            ft_printf(secondary_table, "%s", sbsstr(ffe_buffer));
        } else {
            ERR_CATCH(condor_lp_get_rx_eye(slice, laneId, eyes), goto error_cleanup);
            ERR_CATCH(condor_lp_get_rx_dfe_nrz(slice, laneId, dfe_taps), goto error_cleanup);
            ft_printf(secondary_table, "%4d", eyes[0]);
            ft_printf(secondary_table, "%4.0f,%5.0f,%5.0f", dfe_taps[0], dfe_taps[1], dfe_taps[2]);
            ft_printf(secondary_table, "%3d,%4X", delta_val, edge);
            ft_write(secondary_table, "");
        }

        ft_ln(table);
        ft_ln(table2);
    }
    DISPF(D, "%s", ft_to_string(table));
    if (split_display == SPLIT_DISPLAY) {
        DISPF(D, "%s", ft_to_string(table2));
    }
    ft_destroy_table(table);
    ft_destroy_table(table2);
    return CR_OK;
error_cleanup:
    DISPF(D, "%s", ft_to_string(table));
    if (split_display == SPLIT_DISPLAY) {
        DISPF(D, "%s", ft_to_string(table));
    }
    ft_destroy_table(table);
    ft_destroy_table(table2);
    return CR_FAIL;
}

static CredoError_t bh_fw_serdes_param_parser(CredoSlice_t* slice, int argc, const char* argv[],
                                              const DisplayState_t* D) {
    return common_serdes_param_parser(slice, bh_fw_serdes_param, argc, argv, D);
}

static CredoError_t bh_fw_ffe_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc == 1);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);
    int data[8] = {0};

    ft_table_t* table = ft_create_table();
    ft_set_border_style(table, FT_BASIC2_STYLE);  // lines after every row

    // add header
    ft_printf_ln(table, "|K1|K2|K3|K4|K5|S0|S1|S2");
    ft_add_separator(table);

    // add ffe row
    ERR_CATCH(common_fw_get_rx_ffe(slice, lane, data), goto error_cleanup);
    ft_write(table, "FFE");
    for (int i = 0; i < 8; i++) {
        char sign;
        if (data[i] < 0) {
            data[i] = abs(data[i]) - 1;
            sign = '-';
        } else {
            sign = ' ';
        }

        if (i < 5) {
            ft_printf(table, "%*c%c%02X", 8, ' ', sign, data[i]);
        } else {
            ft_printf(table, "%3X", data[i]);
        }
    }
    ft_ln(table);

    ERR_CATCH(common_fw_get_rx_ffe_nbias(slice, lane, data), goto error_cleanup);
    ft_write(table, "NBIAS");
    for (int i = 0; i < 8; i++) {
        if (i < 5) {
            ft_printf(table, "%*c%X", 10, ' ', data[i]);
        } else {
            ft_printf(table, "%3X", data[i]);
        }
    }
    ft_ln(table);

    ERR_CATCH(common_fw_get_rx_ffe_flip_counter(slice, lane, data), goto error_cleanup);
    ft_write(table, "JUMP");
    for (int i = 0; i < 5; i++) {
        if (i < 5) {
            ft_printf(table, "%11u", data[i]);
        } else {
            ft_printf(table, "%3u", data[i]);
        }
    }
    ft_ln(table);

    double kaccu[8] = {0};
    ERR_CATCH(common_fw_get_rx_ffe_kaccu(slice, lane, kaccu), goto error_cleanup);
    ft_write(table, "KACCU");
    for (int i = 0; i < 5; i++) {
        ft_printf(table, "%11.6f", kaccu[i]);
    }
    ft_ln(table);

    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
error_cleanup:
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_FAIL;
}

static CredoError_t bh_fw_retimer_status(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    CredoError_t ret = CR_FAIL;

    unsigned healing_en = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &healing_en));
    healing_en = (healing_en >> 4) & 0x1;

    ft_table_t* table_status = ft_create_table();
    if (table_status == NULL) return CR_OUT_OF_MEMORY;
    ft_set_cell_prop(table_status, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table_status, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);

    ft_printf(table_status, "Healing: %s", healing_en ? "On" : "Off");
    ft_printf_ln(table_status, "0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15");
    ft_printf(table_status, "Status");
    for (unsigned lane = 0; lane < slice->desc->lane_count; lane++) {
        unsigned state = 0;
        ERR_CATCH((ret = bh_fw_get_retimer_state(slice, lane, &state)), goto cleanup_2);
        ft_printf(table_status, "%X", state);
    }
    ft_ln(table_status);

    if (healing_en) {
        ft_printf(table_status, "Heal Cnt");
        for (unsigned lane = 0; lane < slice->desc->lane_count; lane++) {
            unsigned cnt = 0;
            ERR_CATCH((ret = bh_fw_get_retimer_debug(slice, lane, RETIMER_DEBUG_FIFO_HEAL_COUNT, &cnt)),
                      goto cleanup_2);
            ft_printf(table_status, "%u", cnt);
        }
        ft_ln(table_status);
    }
    DISPF(D, "%s", ft_to_string(table_status));

    unsigned port_config_mask = 0;
    ERR_PROPS(bh_fw_port_config_mask(slice, &port_config_mask));

    unsigned four_lane_cross = 0;
    ERR_PROPS(readReg(slice, REG_FOUR_LANE_CROSS, &four_lane_cross));

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_OUT_OF_MEMORY;
    ft_set_cell_prop(table, FT_ANY_ROW, 1, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_set_cell_prop(table, FT_ANY_ROW, 2, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_printf_ln(table, "Port|Host|Retimer Status|Line");
    ft_add_separator(table);
    for (int port_id = 0; port_id < BH_MAX_PORT; port_id++) {
        CredoPortConfig_t port_config = {.port_id = CR_PORT_UNCONFIGURED};
        if (port_config_mask & (1 << port_id)) {
            ERR_CATCH((ret = bh_port_query(slice, port_id, &port_config)), goto cleanup);
        }

        if (port_config.port_id == CR_PORT_UNCONFIGURED || port_config.connection_mode != CR_PORT_RETIMER) continue;

        unsigned fifo1, fifo2, fifo3, fifo4;
        for (int i = 0; i < port_config.line_no_of_lanes; i++) {
            int A = port_config.host_start_lane + i;
            int B = port_config.line_start_lane + i;

            int fifo_A = (four_lane_cross & (1 << A)) ? B : A;
            int fifo_B = (four_lane_cross & (1 << B)) ? A : B;
            ERR_CATCH((ret = readReg(slice, REG_ADR_DIFF_A0(fifo_A), &fifo1)), goto cleanup);
            ERR_CATCH((ret = readReg(slice, REG_ADR_DIFF_A1(fifo_A), &fifo2)), goto cleanup);
            ERR_CATCH((ret = readReg(slice, REG_ADR_DIFF_B0(fifo_B), &fifo3)), goto cleanup);
            ERR_CATCH((ret = readReg(slice, REG_ADR_DIFF_B1(fifo_B), &fifo4)), goto cleanup);
            if (i == 0) {
                ft_printf(table, "%d", port_config.port_id);
            } else {
                ft_printf(table, "%s", "");
            }
            ft_printf_ln(table, "%s(L%d) Rx|-->[%d,%d]-->|Tx %s(L%d)", stringify_lane_id(slice, A), A, gray_bin(fifo1),
                         gray_bin(fifo2), stringify_lane_id(slice, B), B);
            ft_printf_ln(table, "|Tx|<--[%d,%d]<--|Rx", gray_bin(fifo3), gray_bin(fifo4));
        }
        ft_add_separator(table);
    }

    DISPF(D, "%s", ft_to_string(table));
cleanup:
    ft_destroy_table(table);
cleanup_2:
    ft_destroy_table(table_status);
    return ret;
}

static CredoError_t bh_fw_sw_gearbox_error_info(CredoSlice_t* slice, const DisplayState_t* D) {
    DISPF(D, "Gearbox failing counters\n");
    unsigned port_config_mask = 0;
    ERR_PROPS(bh_fw_port_config_mask(slice, &port_config_mask));

    ft_table_t* table = ft_create_table();
    ft_printf_ln(table, "|||RX PDIFF||||TX PDIFF||||RX/TX||First RX PDIFF||||First TX PDIFF||||First RX/TX|First");
    ft_printf_ln(
        table,
        "Port|Dir|Error Code|Cur|Min|Max|Dev|Cur|Min|Max|Dev|ncw_avg|bip|Cur|Min|Max|Dev|Cur|Min|Max|Dev|ncw_avg|bip");
    ft_add_separator(table);
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        if (!(port_config_mask & (1 << port_id))) {
            continue;
        }
        ft_printf(table, "%u", port_id);

        for (int i = 0; i < 2; i++) {
            CredoSide_t side = (i == 0) ? CR_SIDE_HOST : CR_SIDE_LINE;
            if (i == 0) {
                ft_printf(table, "A->B");
            } else {
                ft_printf(table, "|B->A");
            }

            fw_gearbox_error_info_t info = {0};
            ERR_PROPS(bh_fw_get_gearbox_error_info(slice, port_id, side, &info));
            ft_printf(table, "%s(%X)", bh_fw_gearbox_error_string(info.gb_error_code), info.gb_error_code);
            ft_printf(table, "%u|%u|%u|%u", info.fail_pdiff_rx, info.fail_pdiff_rx_min, info.fail_pdiff_rx_max,
                      info.fail_pdiff_rx_dev);
            ft_printf(table, "%u|%u|%u|%u", info.fail_pdiff_tx, info.fail_pdiff_tx_min, info.fail_pdiff_tx_max,
                      info.fail_pdiff_tx_dev);
            ft_printf(table, "%u, %u", info.fail_ncw_avg_rx, info.fail_ncw_avg_tx);
            ft_printf(table, "%u", info.fail_bip);
            ft_printf(table, "%u|%u|%u|%u", info.first_fail_pdiff_rx, info.first_fail_pdiff_rx_min,
                      info.first_fail_pdiff_rx_max, info.first_fail_pdiff_rx_dev);
            ft_printf(table, "%u|%u|%u|%u", info.first_fail_pdiff_tx, info.first_fail_pdiff_tx_min,
                      info.first_fail_pdiff_tx_max, info.first_fail_pdiff_tx_dev);
            ft_printf(table, "%u, %u", info.first_fail_ncw_avg_rx, info.first_fail_ncw_avg_tx);
            ft_printf(table, "%u", info.first_fail_bip);
            ft_ln(table);
        }
        ft_add_separator(table);
    }

    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, 1, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_span(table, 0, 3, 4);
    ft_set_cell_span(table, 0, 7, 4);
    ft_set_cell_span(table, 0, 13, 4);
    ft_set_cell_span(table, 0, 17, 4);
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

static CredoError_t bh_fw_sw_gearbox_status(CredoSlice_t* slice, const DisplayState_t* D) {
    unsigned port_config_mask = 0;
    ERR_PROPS(bh_fw_port_config_mask(slice, &port_config_mask));

    ft_table_t* table = ft_create_table();
    ft_printf_ln(table, "||||RX / TX|");
    ft_printf_ln(table, "Port|Dir|RX FIFO|TX FIFO|PDIFF CFG|PDIFF CFG HW|PDIFF DEV|ncw_avg|bip_sum");
    ft_add_separator(table);
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        if (!(port_config_mask & (1 << port_id))) {
            continue;
        }
        ft_printf(table, "%u", port_id);

        for (int i = 0; i < 2; i++) {
            CredoSide_t side = (i == 0) ? CR_SIDE_HOST : CR_SIDE_LINE;
            if (i == 0) {
                ft_printf(table, "A->B");
            } else {
                ft_printf(table, "|B->A");
            }

            fw_gearbox_info_t info = {{0}};
            ERR_PROPS(bh_fw_get_gearbox_info(slice, port_id, side, &info));
            ft_printf(table, "%4u, %4u, %4u|%4u, %4u, %4u", info.fifo.rx_min, info.fifo.rx_cur, info.fifo.rx_max,
                      info.fifo.tx_min, info.fifo.tx_cur, info.fifo.tx_max);
            ft_printf(table, "%u, %u", info.cfg_pdiff_rx, info.cfg_pdiff_tx);
            ft_printf(table, "%u/%u, %u/%u", info.cfg_pdiff_hw_rx_min, info.cfg_pdiff_hw_rx_max,
                      info.cfg_pdiff_hw_tx_min, info.cfg_pdiff_hw_tx_max);
            ft_printf(table, "%u, %u", info.cfg_pdiff_dev_rx, info.cfg_pdiff_dev_tx);
            ft_printf(table, "%u, %u", info.ncw_avg_rx, info.ncw_avg_tx);

            if (i == 1) {
                ft_printf_ln(table, "%u", info.bip_sum_rx);
            } else {
                ft_ln(table);
            }
        }
        ft_add_separator(table);
    }

    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, 1, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_span(table, 0, 4, 4);
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

static CredoError_t bh_fw_gearbox_status(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    bool active_lane_mask[CHIP_LANES] = {false};
    uint64_t corr_cw[FEC_MAX_NUM] = {0};
    unsigned uncorr_cw[FEC_MAX_NUM] = {0};
    unsigned fec_align[FEC_MAX_NUM] = {0};
    unsigned pcs_align[FEC_MAX_NUM] = {0};
    unsigned in_buf_ptr[FEC_MAX_NUM] = {0};
    unsigned out_buf_ptr[FEC_MAX_NUM] = {0};
    CredoRSFECFifo_t fifo[FEC_MAX_NUM] = {{0}};

    unsigned fw_info_ver = 0;
    ERR_PROPS(bh_fw_get_func_ver(slice, &fw_info_ver));

    unsigned port_config_mask = 0;
    ERR_PROPS(bh_fw_port_config_mask(slice, &port_config_mask));
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        CredoPortConfig_t port_config = {.port_id = CR_PORT_UNCONFIGURED};
        if (!(port_config_mask & (1 << port_id))) {
            continue;
        }

        ERR_PROPS(bh_port_query(slice, port_id, &port_config));
        active_lane_mask[port_config.host_start_lane] = true;
        active_lane_mask[port_config.line_start_lane] = true;
        for (int i = 0; i < 2; i++) {
            CredoSide_t side = (i == 0) ? CR_SIDE_HOST : CR_SIDE_LINE;
            int fec_idx = port_config.line_start_lane / 2;
            if (side == CR_SIDE_HOST) fec_idx -= 4;

            ERR_PROPS(rsfec_get_fec_align_status(slice, port_id, side, &fec_align[fec_idx]));
            ERR_PROPS(rsfec_get_pcs_align_status(slice, port_id, side, &pcs_align[fec_idx]));
            ERR_PROPS(rsfec_get_fifo(slice, port_id, side, &fifo[fec_idx]));
            ERR_PROPS(rsfec_get_corrected_codeword(slice, port_id, side, &corr_cw[fec_idx]));
            ERR_PROPS(rsfec_get_uncorrected_codeword(slice, port_id, side, &uncorr_cw[fec_idx]));
            ERR_PROPS(rsfec_get_buf_pointer(slice, port_id, side, &in_buf_ptr[fec_idx], &out_buf_ptr[fec_idx]));
        }
    }

    unsigned fec_en = 0;
    unsigned adapt_done_all = 0;
    ERR_PROPS(bh_fw_phy_ready(slice, &adapt_done_all));
    ERR_PROPS(readReg(slice, REG_FEC_EN, &fec_en));

    unsigned healing_en = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &healing_en));
    healing_en = (healing_en >> 4) & 0x1;

    ft_table_t* table = ft_create_table();
    ft_table_t* table_adapt = ft_create_table();
    if (table_adapt == NULL) return CR_FAIL;
    if (table == NULL) return CR_FAIL;

    ft_set_cell_prop(table_adapt, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table_adapt, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_write(table_adapt, "");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if (!active_lane_mask[i]) continue;
        ft_printf(table_adapt, "%d", i);
    }
    ft_ln(table_adapt);
    ft_write(table_adapt, "Lane Adapt");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if (!active_lane_mask[i]) continue;
        ft_printf(table_adapt, "%d", adapt_done_all & (1 << i) ? 1 : 0);
    }
    ft_ln(table_adapt);

    ft_write(table_adapt, "Status");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if (!active_lane_mask[i]) continue;
        unsigned state = 0;
        ERR_CATCH(bh_fw_get_gearbox_state(slice, i, &state), goto error_cleanup);
        ft_printf(table_adapt, "%s(%X)", bh_fw_gearbox_state_string(state), state);
    }
    ft_ln(table_adapt);

    if (healing_en) {
        ft_printf(table_adapt, "Heal Cnt");
        for (unsigned lane = 0; lane < slice->desc->lane_count; lane++) {
            if (!active_lane_mask[lane]) continue;
            unsigned cnt = 0;
            CredoError_t err = hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_FIFO_HEAL_COUNT, &cnt);
            // allow it to fail if old firmware (< 1.2.5)
            if (err == CR_OK) {
                ft_printf(table_adapt, "%d", cnt);
            } else {
                ft_printf(table_adapt, "-");
            }
        }
        ft_ln(table_adapt);
    }

    if (fw_info_ver >= 1) {
        ft_write(table_adapt, "Bg Error Code");
        for (unsigned lane = 0; lane < slice->desc->lane_count; lane++) {
            if (!active_lane_mask[lane]) continue;
            unsigned err_code = 0;
            ERR_CATCH(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_LAST_BG_ERROR_CODE, &err_code),
                      goto error_cleanup);
            ft_printf(table_adapt, "%X", err_code);
        }
        ft_ln(table_adapt);
    }

    ft_printf_ln(table, "FEC|FEC ALIGN|PCS ALIGN|CORR ERR|UNCORR ERR|RX FIFO|TX FIFO|In/Out buf ptr");
    for (int i = 0; i < FEC_MAX_NUM; i++) {
        if (i % 4 == 0) ft_add_separator(table);
        char fec_side = (i < FEC_MAX_NUM / 2) ? 'A' : 'B';
        int lane_num = (i < FEC_MAX_NUM / 2) ? i : i - FEC_MAX_NUM / 2;
        ft_printf(table, "%c%d", fec_side, lane_num);
        if (fec_en & (1 << i)) {
            ft_printf_ln(table, "%u|%u|%9" PRIu64 "|%9u|%4u, %4u, %4u|%4u, %4u, %4u|%u, %u", fec_align[i], pcs_align[i],
                         corr_cw[i], uncorr_cw[i],

                         fifo[i].rx_min, fifo[i].rx_cur, fifo[i].rx_max, fifo[i].tx_min, fifo[i].tx_cur, fifo[i].tx_max,
                         in_buf_ptr[i], out_buf_ptr[i]);
        } else {
            ft_printf_ln(table, "OFF|%u", pcs_align[i]);
        }
    }
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, 1, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, 2, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, 7, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);

    DISPF(D, "%s", ft_to_string(table_adapt));
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table_adapt);
    ft_destroy_table(table);

    if (fw_info_ver >= 1) {
        ERR_PROPS(bh_fw_sw_gearbox_status(slice, D));
        if (argc > 0 && strcmp(argv[0], "debug") == 0) {
            ERR_PROPS(bh_fw_sw_gearbox_error_info(slice, D));
        }
    }

    return CR_OK;
error_cleanup:
    DISPF(D, "%s", ft_to_string(table_adapt));
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table_adapt);
    ft_destroy_table(table);
    return CR_FAIL;
}

static CredoError_t bh_fw_bitmux_status(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
#define A_SIDE_FIFO_START_IDX 0
#define B_SIDE_FIFO_START_IDX 4

#define PAM4_SPLIT_FIFO_START_IDX 0
#define PAM4_MERGE_FIFO_START_IDX 2

#define NRZ_SPLIT_FIFO_START_IDX 4
#define NRZ_MERGE_FIFO_START_IDX 6

    unsigned healing_en = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &healing_en));
    healing_en = (healing_en >> 4) & 0x1;

    CredoError_t ret = CR_FAIL;

    ft_table_t* table_status = ft_create_table();
    if (table_status == NULL) return CR_INVALID_ARGS;
    ft_set_cell_prop(table_status, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table_status, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_printf_ln(table_status, "|0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15");
    ft_printf(table_status, "Status");
    for (int i = 0; i < slice->desc->lane_count; i++) {
        unsigned state = 0;
        ERR_CATCH((ret = bh_fw_get_bitmux_state(slice, i, &state)), goto cleanup_2);
        ft_printf(table_status, "%X", state);
    }
    ft_ln(table_status);
    if (healing_en) {
        ft_printf(table_status, "Heal Cnt");
        for (unsigned lane = 0; lane < slice->desc->lane_count; lane++) {
            unsigned cnt = 0;
            // allow it to fail if old firmware (< 1.3.0)
            CredoError_t err = hal_fw_debug_cmd(slice, lane, BITMUX_DEBUG, BITMUX_DEBUG_FIFO_HEAL_COUNT, &cnt);
            if (err == CR_OK) {
                ft_printf(table_status, "%d", cnt);
            } else {
                ft_printf(table_status, "-");
            }
        }
        ft_ln(table_status);
    }
    DISPF(D, "%s", ft_to_string(table_status));

    // Fifo info
    bool read_from_fw = false;
    unsigned fw_feature = 0;
    ERR_PROP(bh_fw_get_feature(slice, &fw_feature));
    if ((fw_feature & FW_FEATURE_BITMUX_FW_PDIFF) == 0) {
        LOGS_DEBUG("[Bitmux] read fifo from HW register");
        read_from_fw = false;
    } else {
        LOGS_DEBUG("[Bitmux] read fifo from firmware");
        read_from_fw = true;
    }

    uint32_t ctrl_val = 0;
    ERR_CATCH((ret = readReg(slice, REG_BM_CTRL_S, &ctrl_val)), goto cleanup_2);

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_INVALID_ARGS;
    ft_printf_ln(table, "FIFO Lane|BitMux Status");
    ft_add_separator(table);
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

        if (read_from_fw == false) {
            // select A or B side fifo
            ERR_CATCH((ret = writeReg(slice, REG_BM_BUF_SEL_LANE, fifo_ab_side_sel + fifo_idx)), goto cleanup);
        }

        char c;
        uint32_t fifo_val, max, min, cur;

        c = (ctrl_idx < 4) ? 'M' : 'S';
        for (int i = 0; i < 2; i++) {
            if (read_from_fw == false) {
                ERR_CATCH((ret = writeReg(slice, REG_BM_BUF_SEL_SLICE, a_fifo_start_idx + i)), goto cleanup);
                ERR_CATCH((ret = readReg(slice, REG_BM_BUF_SEL_READ, &fifo_val)), goto cleanup);
            } else {
                ERR_CATCH(
                    (ret = bh_fw_get_bitmux_fifo(slice, fifo_ab_side_sel + fifo_idx, a_fifo_start_idx + i, &fifo_val)),
                    goto cleanup);
            }
            max = gray_bin((fifo_val >> 4) & 0xf);
            min = gray_bin((fifo_val >> 8) & 0xf);
            cur = gray_bin((fifo_val >> 12) & 0xf);
            if (i == 0) {
                ft_printf_ln(table, "A%u|%c%u -> cur: %u, min: %u, max: %u", fifo_idx, c, i, cur, min, max);
            } else {
                ft_printf_ln(table, "|%c%u -> cur: %u, min: %u, max: %u", c, i, cur, min, max);
            }
        }

        c = (ctrl_idx < 4) ? 'S' : 'M';
        for (int i = 0; i < 2; i++) {
            if (read_from_fw == false) {
                ERR_CATCH((ret = writeReg(slice, REG_BM_BUF_SEL_SLICE, b_fifo_start_idx + i)), goto cleanup);
                ERR_CATCH((ret = readReg(slice, REG_BM_BUF_SEL_READ, &fifo_val)), goto cleanup);
            } else {
                ERR_CATCH(
                    (ret = bh_fw_get_bitmux_fifo(slice, fifo_ab_side_sel + fifo_idx, b_fifo_start_idx + i, &fifo_val)),
                    goto cleanup);
            }
            max = gray_bin((fifo_val >> 4) & 0xf);
            min = gray_bin((fifo_val >> 8) & 0xf);
            cur = gray_bin((fifo_val >> 12) & 0xf);
            if (i == 0) {
                ft_printf_ln(table, "B%u|%c%u -> cur: %u, min: %u, max: %u", fifo_idx, c, i, cur, min, max);
            } else {
                ft_printf_ln(table, "|%c%u -> cur: %u, min: %u, max: %u", c, i, cur, min, max);
            }
        }
        ft_add_separator(table);
    }
    DISPF(D, "%s", ft_to_string(table));

cleanup:
    ft_destroy_table(table);
cleanup_2:
    ft_destroy_table(table_status);
    return ret;
}

CredoError_t bh_fw_dump_isi(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc <= 1);
    int data[16] = {0};
    int lane = 0;
    int lane_list[CHIP_LANES] = {CHIP_LANES};
    CredoError_t ret = CR_OK;

    // convert to lane list
    parsing_string_to_lane_list(argc > 0 ? argv[0] : "all", lane_list, slice->desc->lane_count);

    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_printf_ln(table,
                 "Lane|nrz\npam4|fm1\nfm3|f4\nfm2|f5\nfm1|f6\nf2|f7\nf3|f8\nf4|f9\nf5|f10\nf6|f11\nf7|"
                 "f12\nf8|f13\nf9|f14\nf10|f15\nf11|f16\nf12|f17\nf13|f18\nf14");
    ft_add_separator(table);
    // different sections of the isi
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;
        CredoLaneMode_t lane_mode;
        ERR_CATCH((ret = bh_get_lane_mode(slice, lane, &lane_mode)), goto cleanup);
        if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode)) {
            ft_printf_ln(table, "%s(%d)|-", stringify_lane_id(slice, lane), lane);
            continue;
        }
        // display header if found
        ERR_CATCH((ret = common_fw_get_isi(slice, lane, data)), goto cleanup);
        ft_printf(table, "%s(%d)|%s", stringify_lane_id(slice, lane), lane,
                  (lane_mode == CR_LMODE_NRZ) ? "nrz" : "pam4");
        for (int i = 0; i < 16; i++) {
            ft_printf(table, "%d", data[i]);
        }
        ft_ln(table);
    }

    DISPF(D, "%s", ft_to_string(table));
cleanup:
    ft_destroy_table(table);
    return ret;
}

CredoError_t bh_fec_analyzer_all_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc == 1);
    unsigned seconds = 0;
    ARGPARSE_STRTOUL(0, seconds);

    unsigned hist[CHIP_LANES][16] = {{0}};
    CredoLaneMode_t lane_mode[CHIP_LANES];

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
        ERR_PROPS(bh_get_lane_mode(slice, lane, &lane_mode[lane]));
        if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane])) continue;
        ERR_PROPS(bh_top_set_fec_analyzer(slice, lane, true));
    }

    for (int group = 0; group < 4; group++) {
        for (int lane = 0; lane < slice->desc->lane_count; lane++) {
            if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane])) continue;
            ERR_PROPS(fec_analyzer_set_hist_group(slice, lane, group));
            ERR_PROPS(fec_analyzer_set_config(slice, lane, true,
                                              (lane_mode[lane] == CR_LMODE_NRZ) ? &config_nrz : &config_pam4));
        }

        sleep_ms(seconds * 1000);

        for (int lane = 0; lane < slice->desc->lane_count; lane++) {
            if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane])) continue;
            ERR_PROPS(fec_analyzer_get_hist_counter(slice, lane, &hist[lane][group * 4]));
        }
    }

    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode[lane])) continue;
        ERR_PROPS(fec_analyzer_set_config(slice, lane, false,
                                          (lane_mode[lane] == CR_LMODE_NRZ) ? &config_nrz : &config_pam4));
        ERR_PROPS(bh_top_set_fec_analyzer(slice, lane, false));
    }

    // display table
    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);

    ft_write(table, "Histogram");
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        ft_printf(table, "Lane %s(%2d)", stringify_lane_id(slice, lane), lane);
    }
    ft_ln(table);

    for (int row = 0; row < 16; row++) {
        ft_printf(table, "Sei >= %2d", row);
        for (int lane = 0; lane < slice->desc->lane_count; lane++) {
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

CredoError_t bh_fec_analyzer_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc >= 1 && argc <= 2);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);
    CredoLaneMode_t lane_mode;
    CredoFecAnalyzerConfig_t config = {0};
    unsigned duration_ms = 15000;

    if (argc > 1) {
        ARGPARSE_STRTOUL(1, duration_ms);
    }

    ERR_PROPS(bh_get_lane_mode(slice, lane, &lane_mode));
    switch (lane_mode) {
        case CR_LMODE_NRZ:
            config.codeword_size = 5280;
            config.symbol_size = 10;
            config.threshold = 7;
            break;
        case CR_LMODE_PAM4:
            config.codeword_size = 5440;
            config.symbol_size = 10;
            config.threshold = 15;
            break;
        default:
            DISPF(D, "lane mode %d not NRZ or PAM4\n", lane_mode);
            return CR_FAIL;
    }

    ERR_PROPS(bh_top_set_fec_analyzer(slice, lane, true));

    unsigned hist[16] = {0};
    for (int group = 0; group < 4; group++) {
        ERR_PROPS(fec_analyzer_set_hist_group(slice, lane, group));
        ERR_PROPS(fec_analyzer_set_config(slice, lane, true, &config));

        sleep_ms(duration_ms);

        ERR_PROPS(fec_analyzer_get_hist_counter(slice, lane, &hist[group * 4]));
    }

    ERR_PROPS(fec_analyzer_set_config(slice, lane, false, &config));
    ERR_PROPS(bh_top_set_fec_analyzer(slice, lane, false));

    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_printf_ln(table, "delay = %.3fs|Lane %2d", (double)duration_ms / 1000, lane);

    for (int row = 0; row < 16; row++) {
        ft_printf_ln(table, "Histogram %2d|%10d", row, hist[row]);
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

static CredoError_t bh_display_slice_clock_output(CredoSlice_t* slice, int argc, const char* argv[],
                                                  const DisplayState_t* D) {
    const static char* cko_state_str[] = {"UNCONFIG", "SQUELCHED", "ACTIVE", "INVALID"};
    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);

    ft_printf_ln(table, "id|type|state|lane|divider");

    ft_add_separator(table);
    for (unsigned clock_output = 0; clock_output < 3; clock_output++) {
        const char* clockout_type = (clock_output == 0) ? "DIFF" : "SINGLE";
        ft_printf(table, "%d|%s", clock_output, clockout_type);
        unsigned state = CKO_STATE_INVALID;
        unsigned lane, divider;
        bh_slice_check_clock_output(slice, clock_output, &state, &lane, &divider);
        if (state == CKO_STATE_SQUELCHED || state == CKO_STATE_ACTIVE) {
            ft_printf_ln(table, "%s|%s(%d)|%d", cko_state_str[state], stringify_lane_id(slice, lane), lane, divider);
        } else {
            ft_printf_ln(table, "%s|%c|%c", cko_state_str[state], '-', '-');
        }
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

static CredoError_t bh_bathtub_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc >= 1 && argc <= 2);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);
    int vstep_side, hstep_side, extent_mv;
    int vstep_full, hstep_full;
    int percent = 0, old_percent = -1;
    int ber_exp = 7;
    if (argc > 1) {
        ARGPARSE_STRTOL(1, ber_exp);
    }

    ERR_PROPS(bh_fw_eye_monitor_start(slice, lane, ber_exp, CR_EYE_MONITOR_BATHTUB));

    // allow a 5 second leeway for percentage to increment, otherwise we consider it hung
    CredoTime_t timeout_start;
    get_time(&timeout_start);

    do {
        sleep_ms(500);
        ERR_PROPS(common_fw_em_progress(slice, lane, &percent));
        if (percent != old_percent) {
            get_time(&timeout_start);
            old_percent = percent;
        } else {
            uint64_t duration = ms_passed(&timeout_start);
            if (duration >= 5000) {
                ERR_PROPS(common_fw_em_stop(slice, lane));
                DISPF(D, "ERROR: Bathtub data collection timeout on lane %d\n", lane);
                return CR_FAIL;
            }
        }
    } while (percent < 100);

    ERR_PROPS(bh_fw_eye_monitor_range(slice, lane, &vstep_side, &hstep_side));
    vstep_full = vstep_side * 2 + 1;
    hstep_full = hstep_side * 2 + 1;
    int** data = (int**)malloc(vstep_full * sizeof(int*));
    for (int i = 0; i < vstep_full; i++) {
        data[i] = (int*)malloc(hstep_full * sizeof(int));
    }

    ERR_PROPS(common_fw_em_data(slice, lane, data, &extent_mv));

    ERR_PROPS(common_fw_em_print_bathtub_ascii(slice, data, vstep_side, hstep_side, extent_mv, D));

    for (int i = 0; i < vstep_full; i++) {
        free(data[i]);
    }
    free(data);

    return CR_OK;
}

static CredoError_t bh_eye_monitor_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    // const char* cmd = param->command;
    ARGCOUNT_CHECK(argc <= 3 && argc >= 1);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);
    int vstep_side, hstep_side, extent_mv;
    int vstep_full, hstep_full;
    int percent = 0, old_percent = -1;
    int destructive = 0;
    int flag = 0;
    int ber_exp = 6;
    if (argc > 1) {
        ARGPARSE_STRTOL(1, ber_exp);
    }
    if (argc > 2) {
        ARGPARSE_STRTOL(2, destructive);
        if (destructive) {
            flag = CR_EYE_MONITOR_DESTRUCTIVE;
        }
    }
    ERR_PROPS(bh_fw_eye_monitor_start(slice, lane, ber_exp, flag));

    // allow a leeway for percentage to increment, otherwise we consider it hung
    // use ber_exp to help set such a percentage
    const int timeout_duration_ms = ber_exp * 2500;

    CredoTime_t timeout_start;
    get_time(&timeout_start);

    do {
        sleep_ms(500);
        ERR_PROPS(common_fw_em_progress(slice, lane, &percent));
        if (percent != old_percent) {
            DISPF(D, "Eye monitor lane %d: %d%%\n", lane, percent);
            get_time(&timeout_start);
            old_percent = percent;
        } else {
            uint64_t duration_ms = ms_passed(&timeout_start);
            if (duration_ms >= timeout_duration_ms) {
                ERR_PROPS(common_fw_em_stop(slice, lane));
                DISPF(D, "ERROR: Eye monitor data collection timeout on lane %d\n", lane);
                return CR_FAIL;
            }
        }
    } while (percent < 100);

    ERR_PROPS(bh_fw_eye_monitor_range(slice, lane, &vstep_side, &hstep_side));
    vstep_full = vstep_side * 2 + 1;
    hstep_full = hstep_side * 2 + 1;
    int** data = (int**)malloc(vstep_full * sizeof(int*));
    for (int i = 0; i < vstep_full; i++) {
        data[i] = (int*)malloc(hstep_full * sizeof(int));
    }

    ERR_PROPS(common_fw_em_data(slice, lane, data, &extent_mv));

    ERR_PROPS(common_fw_em_print_eye_monitor_ascii(slice, data, vstep_side, hstep_side, extent_mv, D));

    for (int i = 0; i < vstep_full; i++) {
        free(data[i]);
    }
    free(data);

    return CR_OK;
}

static CredoError_t bh_fw_dump_debug(CredoSlice_t* slice, int lane, CredoLaneMode_t mode, const DisplayState_t* D) {
    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(common_fw_dump_debug(slice, lane, pam4_dump_list, pam4_dump_len, D));
    } else {
        ERR_PROPS(common_fw_dump_debug(slice, lane, nrz_dump_list, nrz_dump_len, D));
    }
    return CR_OK;
}

static CredoError_t bh_fw_dump_debug_pam4(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc == 1);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);
    return bh_fw_dump_debug(slice, lane, CR_LMODE_PAM4, D);
}

static CredoError_t bh_fw_dump_debug_nrz(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc == 1);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);
    return bh_fw_dump_debug(slice, lane, CR_LMODE_NRZ, D);
}

static CredoError_t bh_fw_dump_pll_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc <= 1);

    unsigned lock_criteria = 0;
    if (hal_fw_reg_rd_internal(slice, FWREG_QTOP_VCO_LOCK_CRITERIA, &lock_criteria) != CR_OK) {
        DISPF(D, "WARN: Firmware does not support this feature.");
        return CR_OK;
    }
    DISPF(D, "lock_criteria: %u\n", lock_criteria);

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_OUT_OF_MEMORY;
    if (argc == 0) {
        unsigned vcocap_state = 0;
        ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_VCOCAP_STATE, &vcocap_state));

        ft_printf_ln(table, "|Cnt");
        ft_add_separator(table);
        ft_printf_ln(table, "State|%04X", vcocap_state);
        ft_add_separator(table);
        for (int i = 0; i < 120; i++) {
            unsigned top_cnt = 0;
            ERR_PROPS(hal_fw_debug_cmd(slice, 0, TOP_DEBUG, TOP_DEBUG_VCOCAP_TOP_DEBUG + i, &top_cnt));
            ft_printf_ln(table, "%d|%d", i, top_cnt);
        }
    } else {
        int lane = 0;
        ARGPARSE_STRTOL(0, lane);

        unsigned tx_result = 0, rx_result = 0, cost_time = 0, target_cnt = 0, msb = 0, lsb = 0;
        if (hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_VCOCAP_COST_TIME, &cost_time) != CR_OK) {
            DISPF(D, "WARN: Firmware don't support per lane pll debug info.\n");
            return CR_OK;
        }
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_VCOCAP_TX_RESULT, &tx_result));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_VCOCAP_RX_RESULT, &rx_result));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_VCOCAP_TRGT_CNT_MSB, &msb));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_VCOCAP_TRGT_CNT_LSB, &lsb));
        target_cnt = (msb << 16) | lsb;
        DISPF(D, "cost time: %f, target cnt: %u\n", cost_time * 0.3356, target_cnt);

        ft_printf_ln(table, "|TX|RX");
        ft_add_separator(table);
        ft_printf_ln(table, "Result|%u|%u", tx_result, rx_result);
        ft_add_separator(table);
        ft_printf_ln(table, "Index|HW cnt - target cnt");
        ft_add_separator(table);
        ft_set_cell_span(table, 2, 1, 2);
        for (int i = 0; i < 128; i++) {
            int rx_dbg = 0, tx_dbg = 0;
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_VCOCAP_TX_DEBUG + i, (unsigned*)&tx_dbg));
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_VCOCAP_RX_DEBUG + i, (unsigned*)&rx_dbg));
            tx_dbg = (tx_dbg & 0x8000) ? tx_dbg - 0x10000 : tx_dbg;
            rx_dbg = (rx_dbg & 0x8000) ? rx_dbg - 0x10000 : rx_dbg;
            ft_printf_ln(table, "%d|%d|%d", i, tx_dbg, rx_dbg);
        }
    }

    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

static CredoError_t bh_fw_dump_anlt(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc <= 1);

    bool show_raw = false, show_detail = false;

    if (argc == 0) {
        show_detail = true;
        show_raw = true;
    } else if (strncmp(argv[0], "detail", strlen("detail")) == 0) {
        show_detail = true;
    } else if (strncmp(argv[0], "raw", strlen("raw")) == 0) {
        show_raw = true;
    } else {
        return CR_INVALID_ARGS;
    }

    int lane_idx = 0;
    int lane_list[CHIP_LANES + 1] = {0};

    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        CredoPortConfig_t port_cfg = {0};
        ERR_PROPS(bh_port_query(slice, port_id, &port_cfg));
        if (port_cfg.port_id == CR_PORT_UNCONFIGURED || !(port_cfg.flags & CR_PFLAG_LINE_SIDE_ANLT)) {
            continue;
        }
        lane_list[lane_idx++] = port_cfg.line_start_lane;
    }
    lane_list[lane_idx] = CHIP_LANES;

    unsigned anlt_pwr_saving_en = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &anlt_pwr_saving_en));
    anlt_pwr_saving_en = (anlt_pwr_saving_en >> 6) & 0x1;
    DISPF(D, "Firmware ANLT power saving: %u\n", anlt_pwr_saving_en);

    unsigned an_state[LINE_LANES] = {0};
    uint64_t tx_pages[LINE_LANES][9] = {{0}}, rx_pages[LINE_LANES][9] = {{0}};

    int lane = 0;
    for (int idx = 0; idx < slice->desc->lane_count; idx++) {
        if ((lane = lane_list[idx]) >= slice->desc->lane_count) break;

        ERR_PROP(bh_fw_get_an_state(slice, lane, &an_state[idx]));
        if (anlt_pwr_saving_en) {
            ERR_PROP(bh_fw_get_an_pages(slice, lane, (uint64_t*)tx_pages[idx], (uint64_t*)rx_pages[idx]));
        } else {
            int page_count = 0;  // dummy
            ERR_PROP(common_get_autoneg_exchanged_pages(slice, lane - HOST_LANES, &page_count, tx_pages[idx],
                                                        rx_pages[idx]));
        }
    }

    if (show_raw) {
        ERR_PROP(common_dump_anlt_pages(slice, lane_list, an_state, tx_pages, rx_pages, D));
    }

    if (show_detail) {
        ERR_PROP(common_dump_anlt_detail(slice, lane_list, an_state, tx_pages, rx_pages, D));
    }

    return CR_OK;
}

static CredoError_t bh_display_acfg_status(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    int state, error_code;

    if (bh_fw_get_acfg_status(slice, &state, &error_code) == CR_UNSUPPORTED) {
        LOGS_INFO("Firmware unsupported");
    } else {
        LOGS_INFO("[ACFG] state %d, error_code %d", state, error_code);
    }
    return CR_OK;
}

static const DisplayCommand_t bh_slice_cmds[] = {
    ADD_DISPLAY_CMD("slice_info", common_slice_info, DISPLAY_CMD_DOC("", "Display top level slice information")),
    ADD_DISPLAY_CMD("isi", bh_fw_dump_isi, DISPLAY_CMD_DOC("", "Display Intersymbol interference table for all lanes"),
                    DISPLAY_CMD_DOC("<lane_list>", "... for specific lanes")),
    ADD_DISPLAY_CMD("ffe_info", bh_fw_ffe_info, DISPLAY_CMD_DOC("<lane>", "Display RX ffe taps table")),
    ADD_DISPLAY_CMD("fec_analyzer_all", bh_fec_analyzer_all_info,
                    DISPLAY_CMD_DOC("<seconds>", "Display general prbs fec analzyer information for all lanes")),
    ADD_DISPLAY_CMD(
        "fec_analyzer", bh_fec_analyzer_info,
        DISPLAY_CMD_DOC("<lane> <duration=15_000ms>", "Display specific prbs fec analyzer information for a lane")),
    ADD_DISPLAY_CMD("prbs_phase_error", common_prbs_phase_error,
                    DISPLAY_CMD_DOC("", "Display prbs phase error information, default is lane 0, 1s"),
                    DISPLAY_CMD_DOC("<lane>", "Select lane"),
                    DISPLAY_CMD_DOC("<lane> <seconds>", "Select lane and duration")),
    DISPLAY_ARGP_CMD("rx_monitor", common_rx_monitor, "Display PRBS and FEC Analyzer performance metrics",
                     DISPLAY_ARGP_USES("[options]", "[options] <lanelist>"), NULL),
    ADD_DISPLAY_CMD("bathtub", bh_bathtub_info,
                    DISPLAY_CMD_DOC("<lane> <ber_exp=7>", "Display bathtub graph of eye for a lane")),
    ADD_DISPLAY_CMD(
        "eye_monitor", bh_eye_monitor_info,
        DISPLAY_CMD_DOC("<lane> <ber_exp=6> <destructive=0>",
                        "Display eye diagram for a lane. Enabling Destructive will mean significant data loss.")),
    ADD_DISPLAY_CMD("pam4_debug", bh_fw_dump_debug_pam4,
                    DISPLAY_CMD_DOC("<lane>", "Display pam4 debug information for a lane")),
    ADD_DISPLAY_CMD("nrz_debug", bh_fw_dump_debug_nrz,
                    DISPLAY_CMD_DOC("<lane>", "Display nrz debug information for a lane")),
    ADD_DISPLAY_CMD("pll_info", bh_fw_dump_pll_info, DISPLAY_CMD_DOC("", "Display top information"),
                    DISPLAY_CMD_DOC("<lane>", "Display per lane information")),
    ADD_DISPLAY_CMD("fw_exit_codes", common_fw_dump_exit_codes,
                    DISPLAY_CMD_DOC("", "Display firmware exit codes for all lanes")),
    ADD_DISPLAY_CMD("anlt", bh_fw_dump_anlt, DISPLAY_CMD_DOC("", "Display all ANLT info"),
                    DISPLAY_CMD_DOC("<detail>", "Display ANLT status and parsing result"),
                    DISPLAY_CMD_DOC("<raw>", "Display raw data of each page")),
    ADD_DISPLAY_CMD(
        "serdes_param", bh_fw_serdes_param_parser, DISPLAY_CMD_DOC("", "Display SerDes parameters table for all lanes"),
        DISPLAY_CMD_DOC("<lane_list>", "Display SerDes parameters table for selected lanes"),
        DISPLAY_CMD_DOC("S<lane_list>", "Display SerDes parameters table for selected lanes split to reduce width")),
    ADD_DISPLAY_CMD("serdes_control", bh_fw_serdes_control,
                    DISPLAY_CMD_DOC("", "Display SerDes control parameters (non-firmware controlled) for all lanes")),
    ADD_DISPLAY_CMD("retimer_status", bh_fw_retimer_status,
                    DISPLAY_CMD_DOC("", "Display retimer status for all ports")),
    ADD_DISPLAY_CMD("gearbox_status", bh_fw_gearbox_status, DISPLAY_CMD_DOC("", "Display gearbox status for all ports"),
                    DISPLAY_CMD_DOC("debug", "Show all fw gearbox debug counters")),
    ADD_DISPLAY_CMD("bitmux_status", bh_fw_bitmux_status, DISPLAY_CMD_DOC("", "Display bitmux status for all ports")),
    ADD_DISPLAY_CMD("port_info", bh_fw_port_info,
                    DISPLAY_CMD_DOC("", "Display port configuration information for all ports")),
    // NOTE: this is needed
    ADD_DISPLAY_CMD("clock_output", bh_display_slice_clock_output, DISPLAY_CMD_DOC("", "Display slice clock output")),
    ADD_DISPLAY_CMD("fw_reg", common_dump_fw_reg, DISPLAY_CMD_DOC("", "Display firmware register")),
    ADD_DISPLAY_CMD("acfg_status", bh_display_acfg_status, DISPLAY_CMD_DOC("", "Display auto config status")),
};

// Info functions

CredoError_t bh_display_slice_info(CredoSlice_t* slice, const char* argv[], size_t argc, CredoDisplayWriter_t writer,
                                   void* ud) {
    DisplayState_t D = {.writer = writer, .ud = ud};
    return display_info(slice, argc, argv, &D, bh_slice_cmds, COUNT_OF(bh_slice_cmds));
}
