#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"
#include "se_fw_state.h"
#include "se_option.h"
#include "strify.h"

#include "anlt/anlt.h"
#include "common/common_display.h"
#include "common/common_firmware.h"
#include "common/common_prbs.h"
#include "swift/swift_serdes.h"

#include "fort.h"
#include "sbs.h"
#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local definition
#define BSIZE 1024

static CredoError_t se_port_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    static const char* link[] = {"Down", " Up "};
    int active_lane_list[32] = {0};
    char host_lane_str[LANE_LIST_STR_MAX_LEN] = {0};
    char line_lane_str[LANE_LIST_STR_MAX_LEN] = {0};
    char active_lane_str[LANE_LIST_STR_MAX_LEN] = {0};
    unsigned rdy = 0;
    int isc_slice_id = 0xFFFF;
    bool switchover_enable = false;
    sbs* isc_slice_id_str = SBS64("");
    ERR_PROPS(se_fw_phy_ready(slice, &rdy));
    ERR_PROPS(se_option_get_isc_slice_id(slice, NULL, &isc_slice_id));
    ERR_PROPS(se_port_get_switchover_configured(slice, &switchover_enable));
    if (isc_slice_id != 0xFFFF) sbscatprintf(isc_slice_id_str, "ISC slice_id %u", isc_slice_id);

    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, 0, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, 0, FT_CPROP_RIGHT_PADDING, 0);
    ft_printf(table, "|||Link|SerDes Speed|Lane Group|Main Lane");
    if (switchover_enable) {
        ft_printf(table, " ");
    }
    ft_printf_ln(table, "%s", sbsstr(isc_slice_id_str));
    ft_printf(table, "Port|MODE|Speed|Host Line|Host Line|Host Line|Host Line");
    if (switchover_enable) {
        ft_printf(table, "Active Lanes");
    }
    ft_printf_ln(table, "Flags");
    ft_add_separator(table);

    SeSlice_t* se_slice = (SeSlice_t*)slice;
    for (int i = 0; i < slice->desc->port_count; i++) {
        ft_printf(table, "%2d", i);
        SePortInfo_t* port_info = &se_slice->port_info[i];
        CredoPortSetup_t* port_setup = &port_info->setup;
        if (port_info->started) {
            unsigned mode, lane, speed_A, speed_B;
            lane = (port_info->direction == CR_PORT_DIR_LINE_TO_HOST) ? port_setup->line_lanes[0]
                                                                      : port_setup->host_lanes[0];

            ERR_CATCH(se_fw_get_config_mode(slice, lane, &mode), goto error_cleanup);
            ft_printf(table, "%s", se_fw_config_mode_string(mode));
            ft_printf(table, "%d", port_setup->speed);

            int port_link[2] = {0};
            ERR_CATCH(se_port_link_internal(slice, i, rdy, CR_SIDE_HOST, port_link + 0), goto error_cleanup);
            ERR_CATCH(se_port_link_internal(slice, i, rdy, CR_SIDE_LINE, port_link + 1), goto error_cleanup);
            ft_printf(table, "%s %s", link[port_link[0]], link[port_link[1]]);

            unsigned start_A, start_B;
            start_A = port_setup->host_lanes[0];
            start_B = port_setup->line_lanes[0];

            ERR_CATCH(common_fw_get_speed_index(slice, start_A, &speed_A), goto error_cleanup);
            ERR_CATCH(common_fw_get_speed_index(slice, start_B, &speed_B), goto error_cleanup);
            ft_printf(table, "%s-%s", se_fw_speed_string(speed_A & 0xf), se_fw_speed_string(speed_B & 0xf));

            stringify_lane_list(port_setup->host_lanes, port_setup->host_count, host_lane_str);
            stringify_lane_list(port_setup->line_lanes, port_setup->line_count, line_lane_str);

            ft_printf(table, "%12s %s %-16s", host_lane_str, se_fw_port_direction_string(port_info->direction),
                      line_lane_str);

            ft_printf(table, "%d, %d", port_info->host_main_lane, port_info->line_main_lane);

            if (port_info->setup.mode == CR_PORT_SWITCHOVER_RETIMER) {
                ERR_CATCH(se_port_get_active_lane_list(slice, i, active_lane_list), goto error_cleanup);
                stringify_lane_list(active_lane_list, 32, active_lane_str);
                ft_printf(table, "%s", active_lane_str);
            }

            char flag_str[128] = {0};
            se_port_options_to_string(slice, port_info, flag_str, 128);
            int isc_enable = 0;
            ERR_CATCH(se_port_get_isc_enable(slice, i, &isc_enable), goto error_cleanup);
            ft_printf(table, "%s%s%s", flag_str, strlen(flag_str) ? "," : "", isc_enable ? "ISC" : "");
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

static CredoError_t se_retimer_status(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    CredoErrorCodes_t ret = CR_OK;

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_OUT_OF_MEMORY;
    ft_set_cell_prop(table, FT_ANY_ROW, 1, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_set_cell_prop(table, FT_ANY_ROW, 2, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_printf_ln(table, "Port|Host|Retimer Status|Line");
    ft_add_separator(table);
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        SePortInfo_t* port_info = &se_slice->port_info[port_id];
        CredoPortSetup_t* port_setup = &port_info->setup;

        if (port_info->started == false) continue;
        if (port_setup->mode != CR_PORT_RETIMER) continue;

        unsigned fifo1, fifo2, fifo3, fifo4;
        for (int i = 0; i < port_setup->line_count; i++) {
            unsigned direction = port_info->direction;
            int A = port_setup->host_lanes[i];
            int B = port_setup->line_lanes[i];
            ERR_CATCH((ret = readRegLane(slice, A, REG_ADR_DIFF_0, &fifo1)), goto cleanup);
            ERR_CATCH((ret = readRegLane(slice, A, REG_ADR_DIFF_1, &fifo2)), goto cleanup);
            ERR_CATCH((ret = readRegLane(slice, B, REG_ADR_DIFF_0, &fifo3)), goto cleanup);
            ERR_CATCH((ret = readRegLane(slice, B, REG_ADR_DIFF_1, &fifo4)), goto cleanup);
            if (i == 0) {
                ft_printf(table, "%d", port_id);
            } else {
                ft_printf(table, "%s", "");
            }

            ft_printf(table, "L%d Rx", A);
            if (direction == CR_PORT_DIR_LINE_TO_HOST) {
                ft_printf_ln(table, "-----X---->|Tx L%d", B);
            } else {
                ft_printf_ln(table, "-->[%d,%d]-->|Tx L%d", gray_bin(fifo1), gray_bin(fifo2), B);
            }

            ft_printf(table, "|Tx");
            if (direction == CR_PORT_DIR_HOST_TO_LINE) {
                ft_printf_ln(table, "<----X-----|Rx");
            } else {
                ft_printf_ln(table, "<--[%d,%d]<--|Rx", gray_bin(fifo3), gray_bin(fifo4));
            }
        }
        ft_add_separator(table);
    }

    DISPF(D, "%s", ft_to_string(table));
cleanup:
    ft_destroy_table(table);
    return ret;
}

static CredoErrorCodes_t se_bitmux_status(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    SeSlice_t* h_slice = (SeSlice_t*)slice;
    CredoErrorCodes_t ret = CR_OK;

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_OUT_OF_MEMORY;
    ft_set_cell_prop(table, FT_ANY_ROW, 1, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_set_cell_prop(table, FT_ANY_ROW, 2, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_printf_ln(table, "Port|Ln|Fifo");
    ft_add_separator(table);
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        SePortInfo_t* port_info = &h_slice->port_info[port_id];
        CredoPortSetup_t* port_setup = &port_info->setup;

        if (port_info->started == false) continue;
        if (port_setup->mode != CR_PORT_BITMUX) continue;

        unsigned fifo1, fifo2;
        for (int i = 0; i < port_setup->host_count; i++) {
            int lane = port_setup->host_lanes[i];
            ERR_CATCH((ret = readRegLane(slice, lane, REG_ADR_DIFF_0, &fifo1)), goto cleanup);
            ERR_CATCH((ret = readRegLane(slice, lane, REG_ADR_DIFF_1, &fifo2)), goto cleanup);
            if (i == 0) {
                ft_printf_ln(table, "%d|%d|%d, %d", port_id, lane, gray_bin(fifo1), gray_bin(fifo2));
            } else {
                ft_printf_ln(table, "|%d|%d, %d", lane, gray_bin(fifo1), gray_bin(fifo2));
            }
        }
        for (int i = 0; i < port_setup->line_count; i++) {
            int lane = port_setup->line_lanes[i];
            ERR_CATCH((ret = readRegLane(slice, lane, REG_ADR_DIFF_0, &fifo1)), goto cleanup);
            ERR_CATCH((ret = readRegLane(slice, lane, REG_ADR_DIFF_1, &fifo2)), goto cleanup);
            ft_printf_ln(table, "|%d|%d, %d", lane, gray_bin(fifo1), gray_bin(fifo2));
        }
        ft_add_separator(table);
    }

    DISPF(D, "%s", ft_to_string(table));
cleanup:
    ft_destroy_table(table);
    return ret;
}

static CredoError_t se_fw_serdes_control(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    return common_fw_serdes_control(slice, argc, argv, D);
}

static CredoError_t se_analog_info(CredoSlice_t* slice, int lane, const DisplayState_t* D) {
    unsigned agcgain[4] = {0};
    unsigned agc_attn = 0;
    unsigned ctle[2] = {0};
    unsigned ctle_cs[2] = {0};
    unsigned ctle_ind[2] = {0};
    unsigned ctle_ictrl[2] = {0};
    unsigned skef_en = 0, skef_degen = 0, skef_cap = 0;
    unsigned adc_ref_ctrl = 0;

    ERR_PROPS(swift_get_rx_agcgain(slice, lane, agcgain));
    ERR_PROPS(swift_get_rx_ctle(slice, lane, ctle));
    ERR_PROPS(swift_get_rx_ctle_cs(slice, lane, ctle_cs));
    ERR_PROPS(swift_get_rx_ctle_ictrl(slice, lane, ctle_ictrl));
    ERR_PROPS(swift_get_rx_agc_attn(slice, lane, &agc_attn));
    ERR_PROPS(swift_get_rx_ind(slice, lane, ctle_ind));
    ERR_PROPS(swift_get_rx_skef(slice, lane, &skef_en, &skef_degen, &skef_cap));
    ERR_PROPS(swift_get_rx_adc_ref_ctrl(slice, lane, &adc_ref_ctrl));

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_OUT_OF_MEMORY;
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_RIGHT_PADDING, 0);

    ft_printf_ln(table, "SKEF EN/FREQ/CAP = %u / %u / %u", skef_en, skef_degen, skef_cap);
    ft_printf_ln(table, "CTLE CS1/CS2/PK1/PK2/IND1/IND2/ICTRL1/ICTRL2 = %u/%u/%u/%u/%u/%u/%u/%u", ctle_cs[0],
                 ctle_cs[1], ctle[0], ctle[1], ctle_ind[0], ctle_ind[1], ctle_ictrl[0], ctle_ictrl[1]);
    ft_printf_ln(table, "ADC reference voltage control = %u", adc_ref_ctrl);
    ft_printf_ln(table, "AGCGAIN = %u, %u, %u, %u", agcgain[0], agcgain[1], agcgain[2], agcgain[3]);
    ft_printf_ln(table, "AGCATT = %u", agc_attn);

    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);

    return CR_OK;
}

static CredoError_t se_dtl_config_info(CredoSlice_t* slice, int lane, const DisplayState_t* D) {
    unsigned kf = 0, kp = 0, phase_fast_rotate = 0, ted_slope_decision = 0, delayloop_freeze = 0;
    unsigned th_ktheta = 0, thdly_acc_in_sel = 0, thdly_en = 0;
    unsigned thdly[PHASE_NUM] = {0};
    // unsigned clk_comp_flip = 0;
    // unsigned ktheta[3] = {0};
    // unsigned kflip[3] = {0};

    ERR_PROPS(swift_get_rx_kf(slice, lane, &kf));
    ERR_PROPS(swift_get_rx_kp(slice, lane, &kp));
    ERR_PROPS(swift_get_rx_th_ktheta(slice, lane, &th_ktheta));
    ERR_PROPS(swift_get_rx_thdly(slice, lane, thdly));
    // ERR_PROPS(swift_get_rx_ktheta(slice, lane, ktheta));
    // ERR_PROPS(swift_get_rx_ktheta_flip(slice, lane, kflip));
    // ERR_PROPS(swift_get_rx_clk_comp_flip(slice, lane, &clk_comp_flip));
    ERR_PROPS(swift_get_rx_thdly_acc_in_sel(slice, lane, &thdly_acc_in_sel));
    ERR_PROPS(swift_get_rx_th_ud_ph_enable(slice, lane, &thdly_en));
    ERR_PROPS(swift_get_rx_phase_fast_rotate(slice, lane, &phase_fast_rotate));
    ERR_PROPS(swift_get_rx_ted_slope_decision(slice, lane, &ted_slope_decision));
    ERR_PROPS(swift_get_rx_delayloop_freeze(slice, lane, &delayloop_freeze));

    char thdly_str[128] = {0};
    char* b = thdly_str;
    for (int i = 0; i < PHASE_NUM; i++) {
        b += sprintf(b, "%u ", thdly[i]);
    }

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_OUT_OF_MEMORY;

    ft_printf_ln(table, "KF|KP|PHASE_FAST_ROTATE|TED_SLOP_DEC|DLOOP_FREEZE");
    ft_printf_ln(table, "%u|%u|%u|%u|%u", kf, kp, phase_fast_rotate, ted_slope_decision, delayloop_freeze);
    ft_add_separator(table);
    // ft_printf_ln(table, "4PHASE  K=%u%u%u FLIP=%u%u%u * %u", ktheta[0], ktheta[1], ktheta[2], kflip[0], kflip[1],
    //             kflip[2], clk_comp_flip);
    ft_printf_ln(table, "THDLY    K=%u SEL=%u EN=0x%04x", th_ktheta, thdly_acc_in_sel, thdly_en);
    ft_printf_ln(table, "ph_trim  %s", thdly_str);

    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, 1, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_RIGHT_PADDING, 0);
    ft_set_cell_span(table, 2, 0, 5);
    ft_set_cell_span(table, 3, 0, 5);
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);

    return CR_OK;
}

static CredoError_t se_dsp_config_info(CredoSlice_t* slice, int lane, const DisplayState_t* D) {
    SwiftDspConfig_t dsp_cfg = {0};
    ERR_PROPS(common_fw_cmd(slice, FW_CMD_FREEZE, 0x0, NULL, NULL));
    sleep_ms(10);  // IMPORTANT: need this to give time for the firmware to complete freezing
    ERR_PROPS(swift_get_rx_dsp_config(slice, lane, &dsp_cfg));
    ERR_PROPS(common_fw_cmd(slice, FW_CMD_UNFREEZE, 0x0, NULL, NULL));

    unsigned flt_sel[DSP_FLT_COUNT] = {0};
    unsigned flt_loc[DSP_FLT_COUNT] = {0};
    ERR_PROPS(se_get_rx_flt_sel(slice, lane, flt_sel));
    ERR_PROPS(swift_get_rx_flt_location_by_sel(slice, lane, flt_sel, flt_loc));

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_OUT_OF_MEMORY;
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, 4, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_LEFT);

    ft_printf_ln(table, "|DC|GAIN|VGA|F0|F1|PRE|PST|FLT");
    ft_add_separator(table);
    ft_printf_ln(table, "EN|%u %u|%u,%u|%u|%u|%u|%x|%x|%x", dsp_cfg.dc_adapt_cmn_en, dsp_cfg.dc_adapt_sar_en,
                 dsp_cfg.gain_adapt_cmn_en, dsp_cfg.gain_adapt_en, dsp_cfg.vga_adapt_en, dsp_cfg.dfe_adapt_f0_en,
                 dsp_cfg.dfe_adapt_f1_en, dsp_cfg.ffe_adapt_pre_en, dsp_cfg.ffe_adapt_pst_en, dsp_cfg.ffe_adapt_flt_en);
    ft_printf_ln(table, "MU|%u %u|%u|%u|%u|%u|%u|%u|%u", dsp_cfg.dc_adapt_cmn_mu, dsp_cfg.dc_adapt_sar_mu,
                 dsp_cfg.gain_adapt_mu, dsp_cfg.vga_adapt_mu, dsp_cfg.dfe_adapt_f0_mu, dsp_cfg.dfe_adapt_f1_mu,
                 dsp_cfg.ffe_adapt_pre1_mu, dsp_cfg.ffe_adapt_mu, dsp_cfg.ffe_adapt_mu);
    ft_printf_ln(table, "|in%u|p%2ua%d|%u %u %u %u|||period%d cmn%d", dsp_cfg.dc_adapt_in_sel,
                 dsp_cfg.gain_adapt_path_period, dsp_cfg.gain_adapt_path_anchor, dsp_cfg.vga_mode[0],
                 dsp_cfg.vga_mode[1], dsp_cfg.vga_mode[2], dsp_cfg.vga_mode[3], dsp_cfg.ffe_adapt_phase_period,
                 dsp_cfg.ffe_adapt_phase_cmn);
    ft_printf_ln(table, "FltTaps @  [%d, %d, %d, %d] [%d, %d, %d, %d]", flt_loc[0], flt_loc[1], flt_loc[2], flt_loc[3],
                 flt_loc[4], flt_loc[5], flt_loc[6], flt_loc[7]);

    ft_set_cell_span(table, 3, 6, 3);
    ft_set_cell_span(table, 4, 0, 9);
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);

    int dtl_freq = 0;
    unsigned dtl_phase0 = 0;
    unsigned env[2] = {0};
    ERR_PROPS(swift_get_rx_ppm(slice, lane, &dtl_freq));
    for (int i = 0; i < 100; i++) {
        ERR_PROPS(swift_get_rx_dtl_phase0(slice, lane, &dtl_phase0));
        // phase0_sum += dtl_phase0;
    }
    // phase0_sum /= 100;
    ERR_PROPS(se_get_rx_envelope(slice, lane, env));
    DISPF(D, "Freq = %5d Phase0 = %4d ENV = %2d,%2d\n", dtl_freq, dtl_phase0, env[0], env[1]);

    return CR_OK;
}

static CredoError_t se_coe_info(CredoSlice_t* slice, int lane, const DisplayState_t* D) {
    unsigned vga_coe = 0;
    int dc_cmn = 0;
    ERR_PROPS(se_get_rx_vga(slice, lane, &vga_coe));
    ERR_PROPS(se_get_dc_cmn(slice, lane, &dc_cmn));
    DISPF(D, "VGA = %u, DC_CMN = %d\n", vga_coe, dc_cmn);

    /* DC SAR, GAIN SAR, DFE */
    int dc_sar[DSP_SAR_COUNT] = {0};
    unsigned gain_sar[DSP_SAR_COUNT] = {0};
    int dfe[DSP_DFE_COUNT] = {0};
    ERR_PROPS(se_get_dc_sar(slice, lane, dc_sar));
    ERR_PROPS(se_get_gain_sar(slice, lane, gain_sar));
    ERR_PROPS(se_get_dfe(slice, lane, dfe));

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_OUT_OF_MEMORY;
    ft_write_ln(table, "DC_SAR");
    for (int r = 0; r < DSP_SAR_COUNT / PHASE_NUM; r++) {
        for (int i = 0; i < PHASE_NUM; i++) {
            ft_printf(table, "%d", dc_sar[r * PHASE_NUM + i]);
        }
        ft_ln(table);
    }
    ft_write_ln(table, "GAIN_SAR");
    for (int r = 0; r < DSP_SAR_COUNT / PHASE_NUM; r++) {
        for (int i = 0; i < PHASE_NUM; i++) {
            ft_printf(table, "%d", gain_sar[r * PHASE_NUM + i]);
        }
        ft_ln(table);
    }
    ft_printf_ln(table, "DFE F0 = %d  F1 = %d", dfe[0], dfe[1]);

    int header_row[] = {0, 5, 10};
    for (int i = 0; i < 3; i++) {
        ft_set_cell_prop(table, header_row[i], FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
        ft_set_cell_prop(table, header_row[i], FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
        ft_set_cell_span(table, header_row[i], 0, 16);
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);

    /* FFE */
    int ffe[DSP_RX_FFE_COUNT * PHASE_NUM] = {0};
    ERR_PROPS(se_get_rx_ffe_all(slice, lane, ffe));

    ft_table_t* ffe_table = ft_create_table();
    if (ffe_table == NULL) return CR_OUT_OF_MEMORY;
    ft_set_cell_prop(ffe_table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);

    for (int i = 4; i > 0; i--) {
        ft_printf(ffe_table, "CM%d", i);
    }
    for (int i = 1; i < 9; i++) {
        ft_printf(ffe_table, "CP%d", i);
    }
    for (int i = 0; i < 8; i++) {
        ft_printf(ffe_table, "CF%d", i);
    }
    ft_ln(ffe_table);
    ft_add_separator(ffe_table);

    for (int ph = 0; ph < PHASE_NUM; ph++) {
        for (int i = 0; i < 20; i++) {
            ft_printf(ffe_table, "%d", ffe[i + 20 * ph]);
        }
        ft_ln(ffe_table);
    }

    DISPF(D, "%s", ft_to_string(ffe_table));
    ft_destroy_table(ffe_table);
    return CR_OK;
}

static int cmpfunc(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

static CredoError_t se_eye_isi_info(CredoSlice_t* slice, int lane, const DisplayState_t* D) {
    CredoLaneMode_t lane_mode;
    ERR_PROP(hal_get_lane_mode(slice, lane, &lane_mode));

    if (lane_mode == CR_LMODE_PAM4) {
        double isi[DSP_ISI_COUNT] = {0};
        ERR_PROPS(se_fw_get_isi(slice, lane, isi));
        sbs* isi_str = SBS512("");
        for (int i = 7; i > 3; i--) {
            sbscatprintf(isi_str, "%.2f%% ", isi[i]);
        }
        DISPF(D, "RESISI[-1:-4] = %s\n", sbsstr(isi_str));
        sbsclear(isi_str);
        for (int i = 9; i < 15; i++) {
            sbscatprintf(isi_str, "%.2f%% ", isi[i]);
        }
        DISPF(D, "RESISI[ 2: 8] = %s\n", sbsstr(isi_str));
    }

    int eye_all[14 * PHASE_NUM] = {1};
    unsigned eye_cnt = 0;
    ERR_PROPS(se_fw_get_eye_all(slice, lane, eye_all));
    ERR_PROPS(se_get_eye_count(slice, lane, &eye_cnt));

    ft_table_t* table = ft_create_table();
    if (table == NULL) return CR_OUT_OF_MEMORY;
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);

    sbs* eye_str = SBS2048("");
    for (int ph_stage = 0; ph_stage < 4; ph_stage++) {
        for (int ph = 0; ph < 4; ph++) {
            ft_printf(table, "Phase %d", ph_stage * 4 + ph);
        }
        ft_ln(table);
        ft_add_separator(table);

        if (lane_mode == CR_LMODE_PAM4) {
            for (int ph = 0; ph < 4; ph++) {
                sbsclear(eye_str);
                for (int j = 0; j < 5; j++) {
                    sbscatprintf(eye_str, "%4d ", eye_all[8 + eye_cnt * (ph_stage * 4 + ph) + j]);
                }
                ft_printf(table, "%s", sbsstr(eye_str));
            }
            ft_ln(table);
            for (int ph = 0; ph < 4; ph++) {
                sbsclear(eye_str);
                for (int j = 0; j < 4; j++) {
                    sbscatprintf(eye_str, "%4d ", eye_all[4 + eye_cnt * (ph_stage * 4 + ph) + j]);
                }
                sbscat(eye_str, "     ");
                ft_printf(table, "%s", sbsstr(eye_str));
            }
            ft_ln(table);
            for (int ph = 0; ph < 4; ph++) {
                sbsclear(eye_str);
                for (int j = 0; j < 4; j++) {
                    sbscatprintf(eye_str, "%4d ", eye_all[eye_cnt * (ph_stage * 4 + ph) + j]);
                }
                sbscatprintf(eye_str, "%4d ", eye_all[eye_cnt * (ph_stage * 4 + ph) + 13]);
                ft_printf(table, "%s", sbsstr(eye_str));
            }
        } else {
            for (int ph = 0; ph < 4; ph++) {
                ft_printf(table, "%4d %4d", eye_all[(ph_stage * 4 + ph) * 2], eye_all[(ph_stage * 4 + ph) * 2 + 1]);
            }
        }
        ft_ln(table);
        ft_add_separator(table);
    }

    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);

    int nl_mode = 0;
    double ber = 0;
    ERR_PROPS(swift_get_rx_dfe_nonlinear_mode(slice, lane, &nl_mode));
    ERR_PROPS(hal_get_rx_prbs_ber(slice, lane, 1000, &ber));

    if (lane_mode == CR_LMODE_PAM4) {
        qsort(eye_all, eye_cnt * PHASE_NUM, sizeof(int), cmpfunc);
        DISPF(D, "EYE = (%3d,%3d,%3d)  BER = %.2e %s\n", eye_all[0], eye_all[(eye_cnt * PHASE_NUM) / 2],
              eye_all[eye_cnt * PHASE_NUM - 1], ber, (nl_mode) ? "NL" : "LI");
    } else if (lane_mode == CR_LMODE_NRZ) {
        DISPF(D, "EYE = (%3d,%3d)  BER = %.2e %s\n", eye_all[0], eye_all[1], ber, (nl_mode) ? "NL" : "LI");
    }
    return CR_OK;
}

static CredoError_t se_serdes_diag(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc == 1);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);

    ERR_PROPS(se_analog_info(slice, lane, D));
    ERR_PROPS(se_dtl_config_info(slice, lane, D));
    ERR_PROPS(se_dsp_config_info(slice, lane, D));
    ERR_PROPS(se_coe_info(slice, lane, D));

    unsigned phy_ready = 0;
    ERR_PROPS(se_fw_phy_lane_ready(slice, lane, &phy_ready));
    if (phy_ready == 1) {
        ERR_PROPS(se_eye_isi_info(slice, lane, D));
    } else {
        DISPF(D, "Lane %d phy not ready\n", lane);
    }

    return CR_OK;
}

static CredoError_t se_serdes_param_inner(CredoSlice_t* slice, const int* lane_list,
                                          const split_display_t split_display, const DisplayState_t* D,
                                          ft_table_t* table) {
#define FW_IS_RUNNING            (1)
#define CHK_FW_RUNNING_THEN(...) (fw_status == FW_IS_RUNNING) && (__VA_ARGS__)
    unsigned fw_status = 0;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));
    if (fw_status != FW_IS_RUNNING) DISPF(D, "WARN: Firmware is NOT RUNNING or FREEZE!\n");

    unsigned top_pll_cap = 0;
    ERR_PROPS(hal_get_top_pll_cap(slice, &top_pll_cap));

    unsigned adapt_done_all;
    ERR_PROPS(hal_fw_phy_ready(slice, &adapt_done_all));

    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_RIGHT_PADDING, 0);

    ft_printf_ln(table,
                 "||Last Time|Counters|Sd,Rdy,|PLL %u||REQ|PSD0,PSD1,||SKEF||GAIN|CTLE|||DFE|FFE.PRE|FFE.PST|FFE.FLT",
                 top_pll_cap);
    ft_printf_ln(table,
                 "Ln|Mode|Up Down|Adp,ReAdp,LL,LS|AdpDone|Tx "
                 "Rx|CDR|PPM|ChEst,Preset|ENV|EN,Degen,Cap|EYE|1,2,3,4|CS;PK|ATTN|VGA|F0,F1|CM4-CM1|CP1-CP8|CF0-CF7");
    ft_add_separator(table);

    int dfe_taps[DSP_DFE_COUNT];
    int ffe_taps[DSP_RX_FFE_COUNT];
    int lane = 0;
    CredoLaneMode_t lane_mode;
    for (int i = 0; i < slice->desc->lane_count; i++) {
        if ((lane = lane_list[i]) >= slice->desc->lane_count) break;

        ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
        if (lane_mode == CR_LMODE_DISABLE) continue;  // do not print disabled lanes at all

        if (lane_mode == CR_LMODE_OFF) {
            ft_printf_ln(table, "%s(%2d)", stringify_lane_id(slice, lane), lane);
            continue;
        }

        unsigned down_time = 0, up_time = 0;
        char down_time_str[16] = {0}, up_time_str[16] = {0};
        unsigned speed_idx = SPEED_UNKNOWN;
        unsigned flexspeed_lane_map = 0;
        unsigned fw_feature = 0;
        unsigned adapt_cnt = 0, readapt_cnt = 0, linklost_cnt = 0, los_cnt = 0;
        if (fw_status == FW_IS_RUNNING) {
            ERR_PROPS(se_fw_get_feature(slice, &fw_feature));
            ERR_PROPS(se_fw_get_link_time(slice, lane, &up_time, &down_time));
            ERR_PROPS(common_fw_get_speed_index(slice, lane, &speed_idx));
            ERR_PROPS(common_fw_get_adapt_count(slice, lane, &adapt_cnt));
            ERR_PROPS(common_fw_get_readapt_count(slice, lane, &readapt_cnt));
            ERR_PROPS(common_fw_get_link_lost_count(slice, lane, &linklost_cnt));
            ERR_PROPS(common_fw_get_los_count(slice, lane, &los_cnt));
            if (fw_feature & FW_FEATURE_SUPPORT_FLEXSPEED) {
                ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_FLEXSPEED_EN_RX, &flexspeed_lane_map));
            }
        }
        seconds_to_format_string(up_time, up_time_str);
        seconds_to_format_string(down_time, down_time_str);

        int ppm = 0;
        bool rx_cdr_bb_mode = false;
        char ppm_str[8] = {0};
        ERR_PROPS(hal_get_rx_ppm(slice, lane, &ppm));
        ERR_PROPS(swift_get_dtl_bb_en(slice, lane, &rx_cdr_bb_mode));
        ppm_to_format_string(ppm, ppm_str);

        unsigned agcgain[4] = {0};
        unsigned agc_attn = 0;
        unsigned ctle[2] = {0};
        unsigned ctle_cs[2] = {0};
        unsigned skef_en = 0, skef_degen = 0, skef_cap = 0;
        ERR_PROP(swift_get_rx_agcgain(slice, lane, agcgain));
        ERR_PROP(swift_get_rx_agc_attn(slice, lane, &agc_attn));
        ERR_PROP(swift_get_rx_ctle(slice, lane, ctle));
        ERR_PROP(swift_get_rx_ctle_cs(slice, lane, ctle_cs));
        ERR_PROP(swift_get_rx_skef(slice, lane, &skef_en, &skef_degen, &skef_cap));

        int tx_cap = 0, rx_cap = 0;
        ERR_PROPS(swift_get_tx_cap(slice, lane, &tx_cap));
        ERR_PROPS(swift_get_rx_cap(slice, lane, &rx_cap));

        int sd = 0, rdy = 0;
        int eyes[3] = {0};
        unsigned envelope[2] = {0};
        unsigned vga_coe = 0, preset_idx = 0;
        double chan_est_psd[2] = {0};
        double chan_est = 0.0f;
        ERR_PROPS(hal_get_lane_ready(slice, lane, &rdy));
        if (fw_status == FW_IS_RUNNING) {
            ERR_PROPS(se_fw_get_sd(slice, lane, &sd));
            ERR_PROPS(se_fw_get_rx_ffe(slice, lane, 0, ffe_taps));
            ERR_PROPS(se_fw_get_dfe(slice, lane, dfe_taps));
            ERR_PROPS(se_fw_get_rx_vga(slice, lane, &vga_coe));
            ERR_PROPS(se_fw_get_amp(slice, lane, envelope));
            ERR_PROPS(se_fw_get_channel_est_psd(slice, lane, chan_est_psd));
            ERR_PROPS(se_fw_get_channel_est(slice, lane, &chan_est));
            ERR_PROPS(se_fw_get_preset_index(slice, lane, &preset_idx));
            ERR_PROPS(se_fw_get_eye(slice, lane, eyes));
        } else {
            ERR_PROPS(swift_get_rx_signal_detect(slice, lane, &sd));
            ERR_PROPS(swift_get_rx_ffe(slice, lane, 0, ffe_taps));
            ERR_PROPS(swift_get_rx_dfe(slice, lane, dfe_taps));
            ERR_PROPS(swift_get_rx_vga(slice, lane, &vga_coe));
            ERR_PROPS(swift_get_rx_envelope(slice, lane, 0, envelope));
            if (rdy) {
                ERR_PROPS(swift_get_rx_eye(slice, lane, eyes));
            }
        }

        if ((flexspeed_lane_map & (1 << lane)) != 0) {
            uint32_t full_rate = 0;
            ERR_PROPS(hal_fw_get_lane_speed(slice, lane, &full_rate));
            ft_printf(table, "%s(%2d)|%s(%3.3fG)", stringify_lane_id(slice, lane), lane, se_fw_speed_string(speed_idx),
                      full_rate / 1e6);
        } else {
            ft_printf(table, "%s(%2d)|%s", stringify_lane_id(slice, lane), lane, se_fw_speed_string(speed_idx));
        }

        ft_printf(table, "%s,%s", up_time_str, down_time_str);
        ft_printf(table, "%d,%d,%d,%d", adapt_cnt, readapt_cnt, linklost_cnt, los_cnt);
        ft_printf(table, "%d,%d,%d", sd, rdy, ((adapt_done_all >> lane) & 0x1));
        ft_printf(table, "%d,%d", tx_cap, rx_cap);
        ft_printf(table, "%2s|%s", rx_cdr_bb_mode ? "BB" : "MM", ppm_str);
        ft_printf(table, "%0.2lf,%0.2lf,%0.3lf,%u", chan_est_psd[0], chan_est_psd[1], chan_est, preset_idx);
        ft_printf(table, "%u,%u", envelope[0], envelope[1]);
        ft_printf(table, "%u,%u,%u", skef_en, skef_degen, skef_cap);
        ft_printf(table, "%4d,%3d,%3d", eyes[0], eyes[1], eyes[2]);
        ft_printf(table, "%2d,%2d,%2d,%2d", agcgain[0], agcgain[1], agcgain[2], agcgain[3]);
        ft_printf(table, "%2u,%-2u;%2u,%-2u|%u|%u", ctle_cs[0], ctle_cs[1], ctle[0], ctle[1], agc_attn, vga_coe);
        ft_printf(table, "%d,%d", dfe_taps[0], dfe_taps[1]);

        sbs* b = SBS256("");
        for (int i = 0; i < 4; i++) {
            sbscatprintf(b, "%3d,", ffe_taps[i]);
        }
        sbsrange(b, 0, -2);
        ft_printf(table, "%s", sbsstr(b));
        sbsclear(b);
        for (int i = 4; i < 12; i++) {
            sbscatprintf(b, "%3d,", ffe_taps[i]);
        }
        sbsrange(b, 0, -2);
        ft_printf(table, "%s", sbsstr(b));

        sbsclear(b);
        for (int i = 12; i < 20; i++) {
            sbscatprintf(b, "%2d,", ffe_taps[i]);
        }
        sbsrange(b, 0, -2);
        ft_printf(table, "%s", sbsstr(b));
        ft_ln(table);
    }
    DISPF(D, "%s", ft_to_string(table));
    return CR_OK;
}

static CredoError_t se_serdes_param(CredoSlice_t* slice, const int* lane_list, const split_display_t split_display,
                                    const DisplayState_t* D) {
    ft_table_t* table = ft_create_table();
    if (table == NULL) {
        return CR_OUT_OF_MEMORY;
    }
    CredoError_t err = se_serdes_param_inner(slice, lane_list, split_display, D, table);
    ft_destroy_table(table);
    return err;
}

static CredoError_t se_serdes_param_parser(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    return common_serdes_param_parser(slice, se_serdes_param, argc, argv, D);
}

static CredoError_t se_fw_cmd_log(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    unsigned start_idx, cnt = 0;
    ERR_PROPS(hal_fw_debug_cmd(slice, 0, SE_INFO, SE_INFO_CMD_LOG_CNT, &cnt));
    start_idx = (cnt >> 8) & 0xFF;
    cnt &= 0x7F;
    if (cnt == 0) {
        DISPF(D, "Firmware cmd log empty!\n");
        return CR_OK;
    }
    start_idx = (start_idx != cnt) ? start_idx : 0;

    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_LEFT_PADDING, 0);
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_RIGHT_PADDING, 0);
    ft_printf_ln(table, "id|cmd|detail 1|detail 2|ext 0|ext 1|ext 2|ext 3");
    ft_add_separator(table);

    for (int i = 0; i < cnt; i++) {
        unsigned cmd[7] = {0};
        ft_printf(table, "%d", i);
        ERR_PROPS(se_fw_get_cmd_log(slice, (i + start_idx) % cnt, cmd));
        for (int j = 0; j < 7; j++) {
            ft_printf(table, "%04X", cmd[j]);
        }
        ft_ln(table);
    }
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

static CredoError_t se_fw_state_timer(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc >= 1 && argc <= 2);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);

    unsigned cnt = 0, max_cnt = 0;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_CNT, &cnt));
    if (cnt == 0) {
        DISPF(D, "Firmware state timer empty!");
        return CR_OK;
    }

    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_MAX_CNT, &max_cnt));
    if (cnt > max_cnt) {
        DISPF(D, "WARN: Firmware return state count over size, force to %d!", max_cnt);
    }

    unsigned* timestamp = (unsigned*)malloc(cnt * sizeof(unsigned));
    if (timestamp == NULL) return CR_FAIL;
    ERR_CATCH(se_fw_get_state_timestamp(slice, lane, cnt, timestamp), goto err_exit2);

    ft_table_t* table = ft_create_table();
    ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_set_cell_prop(table, 0, 0, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    ft_printf_ln(table, "Lane %d State|||Time|Total|Time stamp", lane);
    ft_printf_ln(table, "mode|major|minor|(ms)|(ms)|(tick)");
    ft_add_separator(table);

    double diff = 0, total = 0;
    for (int i = 0; i < cnt; i++) {
        FirmwareState_t state = {0};
        ERR_CATCH(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_REC + i, &state.state), goto err_exit1);
        ft_printf(table, "%s(%X)|%s(%X)|%s(%02X)", se_fw_state_mode_string(state.s.mode), state.s.mode,
                  se_fw_state_major_string(state.s.maj_state), state.s.maj_state,
                  se_fw_state_minor_string(state.s.min_state), state.s.min_state);

        diff = 0;
        if (cnt > 1 && i < (cnt - 1)) {
            diff = (double)(timestamp[i + 1] - timestamp[i]) * FW_STATE_TIME_UNIT;
        }
        total += diff;
        ft_printf(table, "%0.2lf|%0.2lf|%u", diff, total, timestamp[i]);
        ft_ln(table);
    }
    ft_set_cell_span(table, 0, 0, 3);
    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;

err_exit1:
    ft_destroy_table(table);
err_exit2:
    free(timestamp);
    return CR_FAIL;
}

static CredoError_t se_bathtub_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc >= 1 && argc <= 3);
    int lane = 0;
    ARGPARSE_STRTOL(0, lane);
    int vstep_side, hstep_side, extent_mv;
    int vstep_full, hstep_full;
    int percent = 0, old_percent = -1;
    int ber_exp = 7, timeout = 5000;

    if (argc >= 2) {
        ARGPARSE_STRTOL(1, ber_exp);
    }
    if (argc >= 3) {
        ARGPARSE_STRTOL(2, timeout);
    }

    DISPF(D, "Bathtub lane %d ber_exp %d timeout %d ms\n", lane, ber_exp, timeout);
    ERR_PROPS(se_fw_eye_monitor_start(slice, lane, ber_exp, CR_EYE_MONITOR_BATHTUB));

    CredoTime_t timeout_start;
    get_time(&timeout_start);

    do {
        sleep_ms(500);
        ERR_PROPS(se_fw_eye_monitor_progress(slice, lane, &percent));
        DISPF(D, "percent %d\n", percent);
        if (percent != old_percent) {
            get_time(&timeout_start);
            old_percent = percent;
        } else {
            uint64_t duration = ms_passed(&timeout_start);
            if (duration >= timeout) {
                ERR_PROPS(se_fw_eye_monitor_stop(slice, lane));
                DISPF(D, "Bathtub data collection timeout on lane %d\n", lane);
                return CR_FAIL;
            }
        }
    } while (percent < 100);

    ERR_PROPS(se_fw_eye_monitor_range(slice, lane, &vstep_side, &hstep_side));
    vstep_full = vstep_side * 2 + 1;
    hstep_full = hstep_side * 2 + 1;
    int** data = (int**)malloc(vstep_full * sizeof(int*));
    for (int i = 0; i < vstep_full; i++) {
        data[i] = (int*)malloc(hstep_full * sizeof(int));
    }

    ERR_PROPS(se_fw_eye_monitor_data(slice, lane, data, &extent_mv));

    ERR_PROPS(common_fw_em_print_bathtub_ascii(slice, data, vstep_side, hstep_side, extent_mv, D));

    for (int i = 0; i < vstep_full; i++) {
        free(data[i]);
    }
    free(data);

    return CR_OK;
}

static CredoError_t se_fw_dump_anlt(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    int lane_idx = 0;
    int lane_list[CHIP_LANES + 1] = {0};

    SeSlice_t* se_slice = (SeSlice_t*)slice;
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        SePortInfo_t* port_info = &se_slice->port_info[port_id];

        if (port_info->started == false) {
            continue;
        }
        if (port_info->line_an || port_info->line_lt) {
            lane_list[lane_idx++] = port_info->line_main_lane;
        }
    }
    lane_list[lane_idx] = CHIP_LANES;

    unsigned an_state[LINE_LANES] = {0};
    uint64_t tx_pages[LINE_LANES][9] = {{0}}, rx_pages[LINE_LANES][9] = {{0}};

    int lane = 0;
    for (int idx = 0; idx < slice->desc->lane_count; idx++) {
        if ((lane = lane_list[idx]) >= slice->desc->lane_count) break;
        ERR_PROP(se_fw_get_an_state(slice, lane, &an_state[idx]));
        ERR_PROP(se_fw_get_an_pages(slice, lane, (uint64_t*)tx_pages[idx], (uint64_t*)rx_pages[idx]));
    }

    ERR_PROP(common_dump_anlt_pages(slice, lane_list, an_state, tx_pages, rx_pages, D));
    ERR_PROP(common_dump_anlt_detail(slice, lane_list, an_state, tx_pages, rx_pages, D));

    return CR_OK;
}

static CredoError_t se_clockout_display(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
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
        se_slice_check_clock_output(slice, clock_output, &state, &lane, &divider);
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

static CredoError_t se_anlt_timer(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    enum {
        DUMP_AN_STATE = 0,
        DUMP_ANLT_STATE = 1,
        DUMP_AN_MODE = 2,
        DUMP_ARB_SM = 3,
        DUMP_ANLT_STARTED_MASK = 4,
        DUMP_ANLT_LT_DONE = 5,
        DUMP_AN_LFIT_TIME = 6,
        DUMP_AN_LANE = 7,
        DUMP_HCD_CONSORT_RAW = 8,
        DUMP_HCD_IEEE_RAW = 9,
        DUMP_HCD_MODE = 10,
        DUMP_HCD_LANES = 11,
        DUMP_HCD_SPEED = 12,
        DUMP_ANLT_DATA_READY = 13,
        DUMP_AN_RESTART_CNT = 14,
        DUMP_LT_RESTART_CNT = 15,
        DUMP_LT_STATE = 16,
        DUMP_LT_START_TIME_UPPER = 18,
        DUMP_LT_START_TIME = 19,
        DUMP_LT_FRAME_LOCK_TIME = 20,
        DUMP_LT_LOCAL_READY_TIME = 21,
        DUMP_LT_REMOTE_READY_TIME = 22,
        DUMP_AN_DONE_TIME = 23,
        DUMP_AN_TIMER_OFF_TIME = 25,
        DUMP_AN_ON_HCD_RES_TIME = 26,
        DUMP_AN_DIFF_TIME = 27,
    };
#define DISP_COUNTER 2
#define DISP_TIME    1
#define DISP_FLOAT   3

#define ADD_ANLT_INFO_BASIC(name, section, index, c, t) \
    { name, section, index, c }
#define ADD_ANLT_INFO(name, section, index)         ADD_ANLT_INFO_BASIC(name, section, index, 1.0, DISP_TIME)
#define ADD_ANLT_INFO_COUNTER(name, section, index) ADD_ANLT_INFO_BASIC(name, section, index, 2.0, DISP_COUNTER)

    typedef struct {
        const char* name;
        int section;
        int index;
        int type;
        double c;
    } anlt_info_t;
    anlt_info_t anlt_info_fw_reg[] = {
        ADD_ANLT_INFO("link_fail_inhibit_timer_CL72", FW_REG_SECTION_AUTONEG, 4),
        ADD_ANLT_INFO("link_fail_inhibit_timer_CL136", FW_REG_SECTION_AUTONEG, 5),
        ADD_ANLT_INFO("link_fail_inhibit_timer_CL136F", FW_REG_SECTION_AUTONEG, 6),
        ADD_ANLT_INFO("holdoff_timer", FW_REG_SECTION_AUTONEG, 8),
        ADD_ANLT_INFO("max_wait_timer", FW_REG_SECTION_AUTONEG, 9),
        ADD_ANLT_INFO("wait_timer", FW_REG_SECTION_AUTONEG, 10),
        ADD_ANLT_INFO("link_fail_inhibit_time", FW_REG_SECTION_AUTONEG, 11),
        ADD_ANLT_INFO(NULL, 0, 0),
    };
    anlt_info_t an_info[] = {
        ADD_ANLT_INFO_BASIC("an total time (usec)", ANLT_DEBUG, DUMP_AN_DIFF_TIME, 1.0 / 3.0, DISP_FLOAT),
        ADD_ANLT_INFO("an_done", ANLT_DEBUG, DUMP_AN_DONE_TIME),
        ADD_ANLT_INFO("an_resolved", ANLT_DEBUG, DUMP_AN_ON_HCD_RES_TIME),
        ADD_ANLT_INFO(NULL, 0, 0),
    };

    anlt_info_t lt_info[] = {
        ADD_ANLT_INFO("lt_frame_lock", ANLT_DEBUG, DUMP_LT_FRAME_LOCK_TIME),
        ADD_ANLT_INFO("lt_local_ready", ANLT_DEBUG, DUMP_LT_LOCAL_READY_TIME),
        ADD_ANLT_INFO("lt_remote_ready", ANLT_DEBUG, DUMP_LT_REMOTE_READY_TIME),
        ADD_ANLT_INFO_COUNTER("lt_restart_cnt", ANLT_DEBUG, DUMP_LT_RESTART_CNT),
        ADD_ANLT_INFO(NULL, 0, 0),
    };
    bool is_lane_lt[CHIP_LANES] = {false};
    ft_table_t* table = ft_create_table();
    ft_table_t* table2 = ft_create_table();

    double start_time;
    ERR_PROPS(hal_time_start(slice, &start_time));

    for (int i = 0; anlt_info_fw_reg[i].name != NULL; i++) {
        unsigned value = FW_CMD_LOG_SILENT;
        ft_printf(table, "%s", anlt_info_fw_reg[i].name);
        if (hal_fw_reg_rd(slice, anlt_info_fw_reg[i].index, anlt_info_fw_reg[i].section, &value) != CR_OK) {
            ft_printf_ln(table, "-");
        } else {
            ft_printf_ln(table, "%d", value);
        }
    }

    ft_printf(table2, "Lane");
    for (int lane = 0; lane < CHIP_LANES; lane++) {
        unsigned value = FW_CMD_LOG_SILENT;
        ft_printf(table2, "%d", lane);
        if (hal_fw_debug_cmd(slice, lane, SE_INFO, SE_INFO_ANLT_ON, &value) == CR_OK) {
            // LT on
            if (value & (1 << 1)) is_lane_lt[lane] = true;
        }
    }
    ft_ln(table2);

    ft_add_separator(table2);

    for (int i = 0; an_info[i].name != NULL; i++) {
        ft_printf(table2, "%s", an_info[i].name);
        for (int lane = 0; lane < CHIP_LANES; lane++) {
            unsigned value = FW_CMD_LOG_SILENT;
            if (is_lane_lt[lane] == false) {
                ft_printf(table2, " ");
            } else {
                if (hal_fw_debug_cmd(slice, lane, an_info[i].section, an_info[i].index, &value) != CR_OK) {
                    ft_printf(table2, "-");
                } else if (an_info[i].type == DISP_TIME) {
                    double timedelta = (double)value * FW_STATE_TIME_UNIT / 1000;
                    ft_printf(table2, "%s", STRIFY_TIMEDELTA(timedelta, true));
                } else if (an_info[i].type == DISP_COUNTER) {
                    ft_printf(table2, "%d", value);
                } else {
                    ft_printf(table2, "%.3lf", (double)value * an_info[i].c);
                }
            }
        }
        ft_ln(table2);
    }

    unsigned lt_start_time[CHIP_LANES] = {0};
    bool show_timestamp = true;

    ft_add_separator(table2);

    ft_printf(table2, "lt_start_time");
    for (int lane = 0; lane < CHIP_LANES; lane++) {
        if (is_lane_lt[lane] == false) {
            ft_printf(table2, " ");
            continue;
        }
        unsigned lt_start_time_upper = FW_CMD_LOG_SILENT, lt_start_time_lower;
        CredoError_t err = hal_fw_debug_cmd(slice, lane, ANLT_DEBUG, DUMP_LT_START_TIME_UPPER, &lt_start_time_upper);
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, ANLT_DEBUG, DUMP_LT_START_TIME, &lt_start_time_lower));
        if (err != CR_OK) {
            show_timestamp = false;
            lt_start_time[lane] = lt_start_time_lower;
            ft_printf(table2, "%s", STRIFY_TIMEDELTA((double)lt_start_time_lower * FW_STATE_TIME_UNIT / 1000, true));
            continue;
        }
        lt_start_time[lane] = (lt_start_time_upper << 16) | lt_start_time_lower;
        if (lt_start_time[lane] == 0) {
            ft_printf(table2, "-");
        } else {
            ft_printf(table2, "%s", STRIFY_TIME_ISO8601(lt_start_time[lane] * FW_STATE_TIME_UNIT / 1000 + start_time));
        }
    }
    ft_ln(table2);

    for (int i = 0; lt_info[i].name != NULL; i++) {
        ft_printf(table2, "%s", lt_info[i].name);
        for (int lane = 0; lane < CHIP_LANES; lane++) {
            unsigned value = FW_CMD_LOG_SILENT;
            if (is_lane_lt[lane] == false) {
                ft_printf(table2, " ");
            } else {
                if (hal_fw_debug_cmd(slice, lane, lt_info[i].section, lt_info[i].index, &value) != CR_OK) {
                    ft_printf(table2, "-");
                } else if (lt_info[i].type == DISP_TIME) {
                    if (value == 0) {
                        ft_printf(table2, "0");
                        continue;
                    }
                    unsigned lt_start_time_lower = lt_start_time[lane] & 0xFFFF;
                    unsigned lt_start_time_upper = lt_start_time[lane] & 0xFFFF0000;
                    if (value < lt_start_time_lower) {
                        value += 0x10000;
                    }
                    if (!show_timestamp) {
                        ft_printf(table2, "%s", STRIFY_TIMEDELTA((double)value * FW_STATE_TIME_UNIT / 1000, true));
                        continue;
                    }
                    double timestamp = (double)(value + lt_start_time_upper) * FW_STATE_TIME_UNIT / 1000 + start_time;
                    ft_printf(table2, "%s", STRIFY_TIME_ISO8601(timestamp));
                } else if (lt_info[i].type == DISP_COUNTER) {
                    ft_printf(table2, "%d", value);
                } else {
                    ft_printf(table2, "%.3lf", (double)value * lt_info[i].c);
                }
            }
        }
        ft_ln(table2);
    }

    DISPF(D, "%s", ft_to_string(table));
    DISPF(D, "%s", ft_to_string(table2));

    ft_destroy_table(table);
    ft_destroy_table(table2);
    return CR_OK;
}

static CredoError_t se_fw_dump_pll_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D) {
    ARGCOUNT_CHECK(argc <= 1);

    unsigned lock_criteria = 0;
    if (hal_fw_reg_rd_internal(slice, FWREG_TOP_VCO_LOCK_CRITERIA, &lock_criteria) != CR_OK) {
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
        for (int i = 0; i < SE_TOP_VCOCAP_LENGTH; i++) {
            unsigned top_cnt = 0;
            ERR_PROPS(hal_fw_debug_cmd(slice, 0, SE_INFO, SE_INFO_TOP_PLL_VCOCAP_TOP_DEBUG + i, &top_cnt));
            ft_printf_ln(table, "%d|%d", i, top_cnt);
        }
    } else {
        int lane = 0;
        ARGPARSE_STRTOL(0, lane);

        unsigned tx_final = 0, rx_final = 0, tx_min = 0, rx_min = 0, tx_margin_cnt = 0, rx_margin_cnt = 0;
        unsigned tx_result = 0, rx_result = 0, rx_target_cnt = 0, tx_target_cnt = 0, msb = 0, lsb = 0;
        unsigned tx_range_min = 0, tx_range_max = 0, rx_range_min = 0, rx_range_max = 0;
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_TX_PLL_SEARCH_RANGE, &tx_range_min));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_TX_PLL_SEARCH_RANGE + 1, &tx_range_max));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_RX_PLL_SEARCH_RANGE, &rx_range_min));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_RX_PLL_SEARCH_RANGE + 1, &rx_range_max));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_TX_FINAL, &tx_final));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_RX_FINAL, &rx_final));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_TX_FINAL, &tx_final));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_RX_FINAL, &rx_final));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_TX_MIN, &tx_min));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_RX_MIN, &rx_min));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_TX_MARGIN_CNT, &tx_margin_cnt));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_RX_MARGIN_CNT, &rx_margin_cnt));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_TX_RESULT, &tx_result));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_RX_RESULT, &rx_result));
        ERR_PROPS(hal_fw_debug_cmd_ex(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_RX_TRGT_CNT, &lsb, &msb));
        rx_target_cnt = (msb << 16) | lsb;
        ERR_PROPS(hal_fw_debug_cmd_ex(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_TX_TRGT_CNT, &lsb, &msb));
        tx_target_cnt = (msb << 16) | lsb;

        ft_printf_ln(table, "|TX|RX");
        ft_add_separator(table);
        ft_printf_ln(table, "Search range|%u ~ %u|%u ~ %u", tx_range_min, tx_range_max, rx_range_min, rx_range_max);
        ft_printf_ln(table, "Final|%u|%u", tx_final, rx_final);
        ft_printf_ln(table, "Min|%u|%u", tx_min, rx_min);
        ft_printf_ln(table, "Margin cnt|%u|%u", tx_margin_cnt, rx_margin_cnt);
        ft_printf_ln(table, "Target cnt|%u|%u", tx_target_cnt, rx_target_cnt);
        ft_printf_ln(table, "Result|%u|%u", tx_result, rx_result);
        ft_add_separator(table);
        ft_printf_ln(table, "Index|HW cnt - target cnt");
        ft_add_separator(table);
        ft_set_cell_span(table, 7, 1, 2);

        unsigned max_range = (tx_range_max > rx_range_max) ? tx_range_max : rx_range_max;
        for (int i = 0; i < max_range; i++) {
            int rx_dbg = 0, tx_dbg = 0;
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_TX_DEBUG + i, (unsigned*)&tx_dbg));
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VCOCAP_RX_DEBUG + i, (unsigned*)&rx_dbg));
            tx_dbg = (tx_dbg & 0x8000) ? tx_dbg - 0x10000 : tx_dbg;
            rx_dbg = (rx_dbg & 0x8000) ? rx_dbg - 0x10000 : rx_dbg;
            ft_printf_ln(table, "%d|%d|%d", i, tx_dbg, rx_dbg);
        }
    }

    DISPF(D, "%s", ft_to_string(table));
    ft_destroy_table(table);
    return CR_OK;
}

static const DisplayCommand_t se_slice_cmds[] = {
    ADD_DISPLAY_CMD("slice_info", common_slice_info, DISPLAY_CMD_DOC("", "Display top level slice information")),
    DISPLAY_ARGP_CMD("rx_monitor", common_rx_monitor, "Display PRBS and FEC Analyzer performance metrics",
                     DISPLAY_ARGP_USES("[options]", "[options] <lanelist>"), NULL),
    ADD_DISPLAY_CMD("serdes_param", se_serdes_param_parser,
                    DISPLAY_CMD_DOC("", "Display SerDes parameters table for all lanes"),
                    DISPLAY_CMD_DOC("<lane_list>", "Display SerDes parameters table for selected lanes")),
    ADD_DISPLAY_CMD("serdes_control", se_fw_serdes_control,
                    DISPLAY_CMD_DOC("", "Display SerDes control parameters (non-firmware controlled) for all lanes")),
    ADD_DISPLAY_CMD("serdes_diag", se_serdes_diag, DISPLAY_CMD_DOC("<lane>", "Display SerDes diag info for a lane")),
    ADD_DISPLAY_CMD("port_info", se_port_info,
                    DISPLAY_CMD_DOC("", "Display port configuration information for all ports")),
    ADD_DISPLAY_CMD("retimer_status", se_retimer_status, DISPLAY_CMD_DOC("", "Display retimer status for all ports")),
    ADD_DISPLAY_CMD("bitmux_status", se_bitmux_status, DISPLAY_CMD_DOC("", "Display bitmux status for all ports")),
    ADD_DISPLAY_CMD("prbs_phase_error", common_prbs_phase_error,
                    DISPLAY_CMD_DOC("", "Display prbs phase error information, default is lane 0, 1s"),
                    DISPLAY_CMD_DOC("<lane>", "Select lane"),
                    DISPLAY_CMD_DOC("<lane> <seconds>", "Select lane and duration")),
    ADD_DISPLAY_CMD("bathtub", se_bathtub_info,
                    DISPLAY_CMD_DOC("<lane> <ber_exp=7> <timeout=5s>", "Display bathtub graph of eye for a lane")),
    ADD_DISPLAY_CMD("anlt", se_fw_dump_anlt, DISPLAY_CMD_DOC("", "Display all ANLT info")),
    ADD_DISPLAY_CMD("pll_info", se_fw_dump_pll_info, DISPLAY_CMD_DOC("", "Display top information"),
                    DISPLAY_CMD_DOC("<lane>", "Display per lane information")),
    ADD_DISPLAY_CMD("fw_reg", common_dump_fw_reg, DISPLAY_CMD_DOC("", "Display firmware register")),
    ADD_DISPLAY_CMD("fw_exit_codes", common_fw_dump_exit_codes,
                    DISPLAY_CMD_DOC("", "Display firmware exit codes for all lanes")),
    ADD_DISPLAY_CMD("fw_error_codes", common_fw_dump_error_codes,
                    DISPLAY_CMD_DOC("", "Display firmware error codes for all lanes")),
    ADD_DISPLAY_CMD("fw_cmd_log", se_fw_cmd_log, DISPLAY_CMD_DOC("", "Display firmware command history")),
    ADD_DISPLAY_CMD("fw_state_timer", se_fw_state_timer, DISPLAY_CMD_DOC("<lane>", "Display firmware state timer")),
    ADD_DISPLAY_CMD("clock_output", se_clockout_display, DISPLAY_CMD_DOC("", "Display slice clock output")),
    ADD_DISPLAY_CMD("anlt_timer", se_anlt_timer, DISPLAY_CMD_DOC("", "Display anlt timer")),
};

CredoError_t se_display_info(CredoSlice_t* slice, const char* argv[], size_t argc, CredoDisplayWriter_t writer,
                             void* ud) {
    DisplayState_t D = {.writer = writer, .ud = ud};
    return display_info(slice, argc, argv, &D, se_slice_cmds, COUNT_OF(se_slice_cmds));
}
