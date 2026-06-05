#include "screaming_eagle.h"
#include "se_functions.h"
#include "strify.h"

#include "common/common_efuse.h"
#include "common/datacap.h"
#include "fec_analyzer/fec_analyzer.h"
#include "swift/swift_serdes.h"

#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <stdlib.h>
#include <string.h>
typedef struct {
    int header, json, lane, metadata, serdes_control, serdes_param, serdes_diag, pll, isi, prbs, fecana,
        fecana_histgroup, fecana_hist, fecana_hist_duration, port_info, bitmux, retimer, exit_code, debug_mode;
} LaneDataArgTable_t;

const LaneDataArgTable_t lane_data_default_args = {
    .header = 0,
    .json = 0,
    .lane = 0,
    .metadata = 1,
    .serdes_control = 1,
    .serdes_param = 1,
    .serdes_diag = 0,
    .pll = 0,
    .isi = 0,
    .prbs = 1,
    .fecana = 1,
    .fecana_histgroup = 1,
    .fecana_hist = 0,
    .fecana_hist_duration = 250,
    .port_info = 1,
    .bitmux = 1,
    .retimer = 1,
    .exit_code = 1,
    .debug_mode = 0,
};

typedef struct {
    // metadata
    char timestamp[64];
    uint32_t ecid_raw[2];
    unsigned chip_rev;
    double firmware_time;
    char firmware_version[32];
    unsigned firmware_hash;
    const char* sdk_version;
    unsigned slice_id;
    double temp;
    uint32_t port;
    bool port_exists;
    int lane;
    const char* lane_id;
    // serdes control
    CredoLaneMode_t lane_mode;
    unsigned lane_speed;
    CredoLaneTxState_t tx_state;
    CredoLaneLoopbackMode_t loopback_mode;
    int tx_pol;
    int rx_pol;
    int tx_gc;
    int rx_gc;
    int tx_pc;
    int rx_pc;
    int tx_msblsb;
    int rx_msblsb;
    CredoLaneCoupling_t rx_coupling;
    int tx_taps[7];
    // serdes_param
    double up_time;
    double down_time;
    unsigned adapt_count;
    unsigned readapt_count;
    unsigned link_loss_count;
    unsigned loss_signal_count;
    int signal_detect;
    int rx_ready;
    unsigned phy_ready;
    unsigned top_pll;
    int tx_pll;
    int rx_pll;
    bool rx_cdr_bb_mode;
    int rx_ppm;
    double rx_psd[2];
    double rx_channel_est;
    double rx_snr;
    int fw_preset_idx;
    int rx_envelope[2];
    int rx_skef_addcap;
    int rx_skef_degen;
    int rx_skef_en;
    int rx_eye_height[3];
    int rx_agcgain[4];
    int rx_ctle[2];
    int rx_ctle_cs[2];
    unsigned rx_ctle_ind[2];
    unsigned rx_ctle_ictrl[2];
    unsigned rx_adc_ref_vctrl;
    int rx_atten_gain;
    int rx_vga;
    int rx_dfe[2];
    int rx_dfe_nl;
    int rx_ffe_taps[20];
    int rx_ffe_taps_float_loc[8];
    // serdes diag analog (might just add this to serdes_param)

    // serdes diag
    unsigned rx_kf;
    unsigned rx_kp;
    unsigned rx_thdly_ktheta;
    unsigned rx_thdly[PHASE_NUM];
    unsigned rx_thdly_acc_in_sel;
    unsigned rx_thdly_en;
    unsigned rx_phase_fast_rotate;
    unsigned rx_ted_slope_dec;
    unsigned rx_delay_loop_freeze;
    int rx_dc_cmn;
    int rx_dc_sar[DSP_SAR_COUNT];
    unsigned rx_gain_sar[DSP_SAR_COUNT];

    // pll
    unsigned top_pll_min;
    unsigned top_pll_sel;
    unsigned top_pll_max;
    unsigned top_pll_valid_count;
    unsigned tx_pll_min;
    int tx_pll_sel;
    unsigned tx_pll_max;
    unsigned tx_pll_valid_count;
    unsigned rx_pll_min;
    int rx_pll_sel;
    unsigned rx_pll_max;
    unsigned rx_pll_valid_count;

    // prbs
    bool prbs_enabled;
    CredoLanePrbsPattern_t prbs_pattern;
    uint32_t prbs_count;
    CredoPrbsLockStatus_t prbs_lock;
    double prbs_ber;
    double prbs_duration;
    bool prbs_autosync;
    // fecana
    bool fec_enabled;
    CredoFecAnalyzerConfig_t fec_config;
    CredoFecErrorType_t fec_error_type;
    uint32_t fec_pre_count;
    uint32_t fec_post_count;
    double fec_pre_error_rate;
    double fec_post_error_rate;
    double fec_duration;
    bool fec_autosync;
    int fec_histgroup;
    uint32_t fec_histgroup_bins[4];
    // fecana_hist
    bool fec_hist_enabled;
    uint32_t fec_hist[16];
    double fec_hist_duration;
    // port_info
    CredoPortConnectionMode_t port_mode;
    uint32_t port_speed;
    bool port_host_up;
    bool port_line_up;
    unsigned port_host_lanes;
    unsigned port_line_lanes;
    // retimer
    unsigned retimer_host_state;
    unsigned retimer_line_state;
    unsigned retimer_fifo[2];
    // bitmux
    unsigned bitmux_host_state;
    unsigned bitmux_line_state;
    unsigned bitmux_fifo[2];
    // isi
    double isi[67];
    // exit_code
    unsigned exit_code[16];
} LaneData_t;

static CredoError_t se_lane_data_capture_metadata(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                  LaneData_t* data) {
    strify_timestamp_now(data->timestamp);
    ERR_PROPS(cr_slice_get_param(slice, "sys_time", 0, CR_PDATA_F(&data->firmware_time, 1)));
    ERR_PROPS(cr_firmware_version_str(slice, data->firmware_version));
    ERR_PROPS(cr_firmware_hash(slice, &data->firmware_hash));
    ERR_PROPS(se_slice_get_revision(slice, &data->chip_rev));
    ERR_PROPS(common_efuse_read_ecid(slice, data->ecid_raw));

    data->sdk_version = cr_sdk_version_str();
    data->slice_id = slice->slice_id;
    ERR_PROPS(cr_slice_get_temperature(slice, &data->temp));
    data->lane = lane;
    data->lane_id = stringify_lane_id(slice, lane);
    return CR_OK;
}

static CredoError_t se_lane_data_capture_serdes_control(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                        LaneData_t* data) {
    ERR_PROPS(cr_lane_get_speed(slice, lane, &data->lane_speed));
    ERR_PROPS(cr_lane_get_loopback_mode(slice, lane, &data->loopback_mode));
    ERR_PROPS(cr_serdes_get_tx_polarity(slice, lane, &data->tx_pol));
    ERR_PROPS(cr_serdes_get_rx_polarity(slice, lane, &data->rx_pol));
    ERR_PROPS(cr_serdes_get_tx_gray_code(slice, lane, &data->tx_gc));
    ERR_PROPS(cr_serdes_get_rx_gray_code(slice, lane, &data->rx_gc));
    ERR_PROPS(cr_serdes_get_tx_precoder(slice, lane, &data->tx_pc));
    ERR_PROPS(cr_serdes_get_rx_precoder(slice, lane, &data->rx_pc));
    ERR_PROPS(cr_serdes_get_tx_msb(slice, lane, &data->tx_msblsb));
    ERR_PROPS(cr_serdes_get_rx_msb(slice, lane, &data->rx_msblsb));
    ERR_PROPS(cr_serdes_get_rx_coupling(slice, lane, &data->rx_coupling));
    ERR_PROPS(cr_serdes_get_tx_taps(slice, lane, data->tx_taps));
    ERR_PROPS(cr_lane_tx_get_status(slice, lane, &data->tx_state));
    return CR_OK;
}

static CredoError_t se_lane_data_capture_serdes_param(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                      LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    ERR_PROPS(hal_serdes_get_param(slice, "up_time", lane, CR_PDATA_F(&data->up_time, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "down_time", lane, CR_PDATA_F(&data->down_time, 1)));
    ERR_PROPS(cr_serdes_get_adapt_count(slice, lane, &data->adapt_count));
    ERR_PROPS(cr_serdes_get_readapt_count(slice, lane, &data->readapt_count));
    ERR_PROPS(cr_serdes_get_link_lost_count(slice, lane, &data->link_loss_count));
    ERR_PROPS(cr_serdes_get_rx_signal_detect(slice, lane, &data->signal_detect));
    ERR_PROPS(cr_serdes_get_rx_ready(slice, lane, &data->rx_ready));
    ERR_PROPS(cr_serdes_get_phy_ready(slice, lane, &data->phy_ready));

    ERR_PROPS(swift_get_dtl_bb_en(slice, lane, &data->rx_cdr_bb_mode));

    ERR_PROPS(cr_serdes_get_top_pll_cap(slice, &data->top_pll));
    ERR_PROPS(cr_serdes_get_tx_cap(slice, lane, &data->tx_pll));
    ERR_PROPS(cr_serdes_get_rx_cap(slice, lane, &data->rx_pll));

    ERR_PROPS(cr_serdes_get_rx_ppm(slice, lane, &data->rx_ppm));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_snr", lane, CR_PDATA_F(&data->rx_snr, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_channel_est_psd", lane, CR_PDATA_F(data->rx_psd, 2)));
    ERR_PROPS(cr_serdes_get_channel_estimate(slice, lane, &data->rx_channel_est));
    ERR_PROPS(hal_serdes_get_param(slice, "fw_preset_idx", lane, CR_PDATA_I(&data->fw_preset_idx, 1)));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_envelope", lane, CR_PDATA_I(data->rx_envelope, 2)));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_skef_en", lane, CR_PDATA_I(&data->rx_skef_en, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_skef_degen", lane, CR_PDATA_I(&data->rx_skef_degen, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_skef_addcap", lane, CR_PDATA_I(&data->rx_skef_addcap, 1)));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_eye_height", lane, CR_PDATA_I(data->rx_eye_height, 3)));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_agcgain", lane, CR_PDATA_I(data->rx_agcgain, 4)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_ctle", lane, CR_PDATA_I(data->rx_ctle, 2)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_ctle_cs", lane, CR_PDATA_I(data->rx_ctle_cs, 2)));
    ERR_PROPS(swift_get_rx_ind(slice, lane, data->rx_ctle_ind));
    ERR_PROPS(swift_get_rx_ctle_ictrl(slice, lane, data->rx_ctle_ictrl));
    ERR_PROPS(swift_get_rx_adc_ref_ctrl(slice, lane, &data->rx_adc_ref_vctrl));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_atten_gain", lane, CR_PDATA_I(&data->rx_atten_gain, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_vga", lane, CR_PDATA_I(&data->rx_vga, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_dfe", lane, CR_PDATA_I(data->rx_dfe, 2)));
    ERR_PROPS(swift_get_rx_dfe_nonlinear_mode(slice, lane, &data->rx_dfe_nl));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_ffe_taps", lane, CR_PDATA_I(data->rx_ffe_taps, 20)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_flt_loc", lane, CR_PDATA_I(data->rx_ffe_taps_float_loc, 8)));

    return CR_OK;
}

static CredoError_t se_lane_data_capture_serdes_diag(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                     LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    ERR_PROPS(swift_get_rx_kf(slice, lane, &data->rx_kf));
    ERR_PROPS(swift_get_rx_kp(slice, lane, &data->rx_kp));
    ERR_PROPS(swift_get_rx_th_ktheta(slice, lane, &data->rx_thdly_ktheta));
    ERR_PROPS(swift_get_rx_thdly(slice, lane, data->rx_thdly));
    ERR_PROPS(swift_get_rx_thdly_acc_in_sel(slice, lane, &data->rx_thdly_acc_in_sel));
    ERR_PROPS(swift_get_rx_th_ud_ph_enable(slice, lane, &data->rx_thdly_en));
    ERR_PROPS(swift_get_rx_phase_fast_rotate(slice, lane, &data->rx_phase_fast_rotate));
    ERR_PROPS(swift_get_rx_ted_slope_decision(slice, lane, &data->rx_ted_slope_dec));
    ERR_PROPS(swift_get_rx_delayloop_freeze(slice, lane, &data->rx_delay_loop_freeze));
    ERR_PROPS(se_get_dc_cmn(slice, lane, &data->rx_dc_cmn));
    ERR_PROPS(se_get_dc_sar(slice, lane, data->rx_dc_sar));
    ERR_PROPS(se_get_gain_sar(slice, lane, data->rx_gain_sar));
    return CR_OK;
}

static CredoError_t se_lane_data_capture_pll(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                             LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    ERR_PROPS(hal_get_top_pll_cap(slice, &data->top_pll_sel));
    unsigned top_pll_target_margin = 0;
    ERR_PROPS(hal_fw_debug_cmd(slice, 0, SE_INFO, SE_INFO_TOP_PLL_TARGET_MARGIN, &top_pll_target_margin));
    bool top_pll_min_found = false;
    for (unsigned i = 0; i < SE_TOP_VCOCAP_LENGTH; i++) {
        unsigned diff_value = 0;
        ERR_PROPS(hal_fw_debug_cmd(slice, 0, SE_INFO, SE_INFO_TOP_PLL_VCOCAP_TOP_DEBUG + i, &diff_value));
        int diff_value_signed = abs(twos_to_int((int)diff_value, 16));

        if (diff_value_signed != 0 && diff_value < top_pll_target_margin) {
            data->top_pll_valid_count += 1;
            if (!top_pll_min_found) {
                data->top_pll_min = i;
                top_pll_min_found = true;
            }
            data->top_pll_max = i;
        }
    }

    ERR_PROPS(hal_get_tx_cap(slice, lane, &data->tx_pll_sel));
    unsigned tx_pll_target_margin = 0;
    ERR_PROPS(hal_fw_debug_cmd(slice, 0, OPT_DEBUG, OPT_DEBUG_VCOCAP_TX_MARGIN_CNT, &tx_pll_target_margin));
    bool tx_pll_min_found = false;
    for (unsigned i = 0; i < SE_LANE_VCOCAP_LENGTH; i++) {
        unsigned diff_value = 0;
        ERR_PROPS(hal_fw_debug_cmd(slice, 0, OPT_DEBUG, OPT_DEBUG_VCOCAP_TX_DEBUG + i, &diff_value));
        int diff_value_signed = abs(twos_to_int((int)diff_value, 16));

        if (diff_value_signed != 0 && diff_value < tx_pll_target_margin) {
            data->tx_pll_valid_count += 1;
            if (!tx_pll_min_found) {
                data->tx_pll_min = i;
                tx_pll_min_found = true;
            }
            data->tx_pll_max = i;
        }
    }

    ERR_PROPS(hal_get_rx_cap(slice, lane, &data->rx_pll_sel));
    unsigned rx_pll_target_margin = 0;
    ERR_PROPS(hal_fw_debug_cmd(slice, 0, OPT_DEBUG, OPT_DEBUG_VCOCAP_RX_MARGIN_CNT, &rx_pll_target_margin));
    bool rx_pll_min_found = false;
    for (unsigned i = 0; i < SE_LANE_VCOCAP_LENGTH; i++) {
        unsigned diff_value = 0;
        ERR_PROPS(hal_fw_debug_cmd(slice, 0, OPT_DEBUG, OPT_DEBUG_VCOCAP_RX_DEBUG + i, &diff_value));
        int diff_value_signed = abs(twos_to_int((int)diff_value, 16));

        if (diff_value_signed != 0 && diff_value < rx_pll_target_margin) {
            data->rx_pll_valid_count += 1;
            if (!rx_pll_min_found) {
                data->rx_pll_min = i;
                rx_pll_min_found = true;
            }
            data->rx_pll_max = i;
        }
    }

    return CR_OK;
}

static CredoError_t se_lane_data_capture_isi(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                             LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    ERR_PROPS(hal_serdes_get_param(slice, "rx_isi", lane, CR_PDATA_F(data->isi, 67)));
    return CR_OK;
}

static CredoError_t se_lane_data_capture_prbs(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                              LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    int prbs_enable;
    ERR_PROPS(cr_prbs_get_rx_checker(slice, lane, &prbs_enable, &data->prbs_pattern));
    data->prbs_enabled = prbs_enable;

    if (!data->prbs_enabled) {
        return CR_OK;
    }

    unsigned long prbs_duration = 0;
    // global ber, get duration from timestamp
    ERR_PROPS(cr_prbs_get_rx_duration(slice, lane, &prbs_duration));
    data->prbs_duration = (double)prbs_duration / 1000;
    ERR_PROPS(cr_prbs_get_rx_count(slice, lane, &data->prbs_count));
    ERR_PROPS(cr_prbs_get_rx_lock(slice, lane, &data->prbs_lock));
    ERR_PROPS(cr_prbs_get_rx_ber(slice, lane, 0, &data->prbs_ber));
    ERR_PROPS(cr_prbs_get_rx_autosync(slice, lane, &data->prbs_autosync));
    return CR_OK;
}

static CredoError_t se_lane_data_capture_fecana(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    int fecana_enable;
    CredoFecAnalyzerConfig_t fecana_config;
    ERR_PROPS(cr_fecana_query(slice, lane, &fecana_enable, &fecana_config));
    data->fec_enabled = fecana_enable;
    data->fec_config = fecana_config;
    if (!data->fec_enabled) {
        return CR_OK;
    }
    data->fec_error_type = fecana_config.error_type;
    unsigned long fecana_duration;
    ERR_PROPS(cr_fecana_get_duration(slice, lane, &fecana_duration));
    data->fec_duration = (double)fecana_duration / 1000;

#define TEI 4
#define TEO 6
    unsigned tmp;
    // little bit tricky due to getting to error rates
    ERR_PROPS(cr_fecana_get_counter(slice, lane, &data->fec_pre_count, &tmp));
    ERR_PROPS(cr_fecana_get_error_rate(slice, lane, TEI, 0, &data->fec_pre_error_rate));
    ERR_PROPS(cr_fecana_get_autosync(slice, lane, &data->fec_autosync));
    ERR_PROPS(cr_fecana_get_counter(slice, lane, &tmp, &data->fec_post_count));
    ERR_PROPS(cr_fecana_get_error_rate(slice, lane, TEO, 0, &data->fec_post_error_rate));
    ERR_PROPS(fec_analyzer_get_hist_group(slice, lane, &data->fec_histgroup));
    ERR_PROPS(cr_fecana_get_hist_counter(slice, lane, data->fec_histgroup_bins));
    return CR_OK;
}

static CredoError_t se_lane_data_capture_fecana_hist(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                     LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    int fecana_enable;
    CredoFecAnalyzerConfig_t fecana_config;
    ERR_PROPS(cr_fecana_query(slice, lane, &fecana_enable, &fecana_config));
    data->fec_hist_enabled = fecana_enable;
    if (!data->fec_hist_enabled) {
        return CR_OK;
    }
    for (size_t group = 0; group < 4; group++) {
        ERR_PROPS(cr_fecana_set_hist_group(slice, lane, (int)group));
        sleep_ms(argt->fecana_hist_duration);
        ERR_PROPS(cr_fecana_get_hist_counter(slice, lane, (data->fec_hist) + (group * 4)));
    }
    data->fec_hist_duration = (double)argt->fecana_hist_duration / 1000;
    return CR_OK;
}

static CredoError_t se_lane_data_capture_port_info(CredoSlice_t* slice, int lane, uint32_t port_id,
                                                   const LaneDataArgTable_t* argt, LaneData_t* data) {
    if (!data->port_exists) {
        return CR_OK;
    }
    SeSlice_t* seslice = (SeSlice_t*)slice;
    SePortInfo_t* info = &seslice->port_info[port_id];

    data->port_speed = info->setup.speed;
    data->port_mode = info->setup.mode;
    data->port_host_lanes = info->setup.host_count;
    data->port_line_lanes = info->setup.line_count;

    ERR_PROPS(cr_port_is_link_up(slice, port_id, CR_SIDE_HOST, &data->port_host_up));
    ERR_PROPS(cr_port_is_link_up(slice, port_id, CR_SIDE_LINE, &data->port_line_up));

    return CR_OK;
}

CredoError_t se_lane_data_capture_retimer(CredoSlice_t* slice, int lane, uint32_t port_id,
                                          const LaneDataArgTable_t* argt, LaneData_t* data) {
    if (!data->port_exists || data->port_mode != CR_PORT_RETIMER) {
        return CR_OK;
    }

    ERR_PROPS(cr_port_get_param(slice, "retimer_host_state", port_id, CR_PDATA_U(&data->retimer_host_state, 1)));
    ERR_PROPS(cr_port_get_param(slice, "retimer_line_state", port_id, CR_PDATA_U(&data->retimer_line_state, 1)));
    ERR_PROPS(cr_port_get_param(slice, "retimer_fifo", lane, CR_PDATA_U(data->retimer_fifo, 2)));
    return CR_OK;
}

CredoError_t se_lane_data_capture_bitmux(CredoSlice_t* slice, int lane, uint32_t port_id,
                                         const LaneDataArgTable_t* argt, LaneData_t* data) {
    if (!data->port_exists || data->port_mode != CR_PORT_BITMUX) {
        return CR_OK;
    }
    ERR_PROPS(cr_port_get_param(slice, "bitmux_host_state", port_id, CR_PDATA_U(&data->bitmux_host_state, 1)));
    ERR_PROPS(cr_port_get_param(slice, "bitmux_line_state", port_id, CR_PDATA_U(&data->bitmux_line_state, 1)));
    ERR_PROPS(cr_port_get_param(slice, "bitmux_fifo", lane, CR_PDATA_U(data->bitmux_fifo, 2)));
    return CR_OK;
}

static CredoError_t se_lane_data_capture_exit_codes(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                    LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    for (unsigned index = 0; index < 16; index++) {
        ERR_PROPS(GET_FW_EXIT_CODES(slice, lane, ALL_EXIT_CODES_START + index, &data->exit_code[index]));
    }
    return CR_OK;
}

static bool se_compute_lane_port(CredoSlice_t* slice, int lane, uint32_t* lane_port_id) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    for (uint32_t port_id = 0; port_id < SE_MAX_PORT; port_id++) {
        SePortInfo_t* info = &seslice->port_info[port_id];
        if (!info->started) {
            continue;
        }
        CredoPortSetup_t setup = info->setup;
        for (size_t i = 0; i < setup.host_count; i++) {
            if (setup.host_lanes[i] == lane) {
                *lane_port_id = port_id;
                return true;
            };
        }
        for (size_t i = 0; i < setup.line_count; i++) {
            if (setup.line_lanes[i] == lane) {
                *lane_port_id = port_id;
                return true;
            }
        }
    }
    return false;
}

static CredoError_t se_lane_data_capture_data(CredoSlice_t* slice, const LaneDataArgTable_t* argt, LaneData_t* data) {
    // compute the lane's port (if it exists)
    data->port_exists = se_compute_lane_port(slice, argt->lane, &data->port);
    int lane = argt->lane;
    ERR_PROPS(cr_lane_get_mode(slice, lane, &data->lane_mode));
    if (argt->metadata) {
        ERR_PROPS(se_lane_data_capture_metadata(slice, argt->lane, argt, data));
    }
    if (argt->serdes_control) {
        ERR_PROPS(se_lane_data_capture_serdes_control(slice, argt->lane, argt, data));
    }
    if (argt->serdes_param) {
        ERR_PROPS(se_lane_data_capture_serdes_param(slice, argt->lane, argt, data));
    }
    if (argt->serdes_diag) {
        ERR_PROPS(se_lane_data_capture_serdes_diag(slice, argt->lane, argt, data));
    }
    if (argt->prbs) {
        ERR_PROPS(se_lane_data_capture_prbs(slice, lane, argt, data));
    }
    if (argt->fecana) {
        ERR_PROPS(se_lane_data_capture_fecana(slice, lane, argt, data));
    }
    if (argt->fecana_hist) {
        ERR_PROPS(se_lane_data_capture_fecana_hist(slice, lane, argt, data));
    }
    if (argt->port_info) {
        ERR_PROPS(se_lane_data_capture_port_info(slice, lane, data->port, argt, data));
    }
    if (argt->retimer) {
        ERR_PROPS(se_lane_data_capture_retimer(slice, lane, data->port, argt, data));
    }
    if (argt->bitmux) {
        ERR_PROPS(se_lane_data_capture_bitmux(slice, lane, data->port, argt, data));
    }
    if (argt->exit_code) {
        ERR_PROPS(se_lane_data_capture_exit_codes(slice, lane, argt, data));
    }
    if (argt->isi) {
        ERR_PROPS(se_lane_data_capture_isi(slice, lane, argt, data));
    }
    if (argt->pll) {
        ERR_PROPS(se_lane_data_capture_pll(slice, lane, argt, data));
    }
    return CR_OK;
}

static void se_lane_data_write(DataCaptureState_t* D, const LaneDataArgTable_t* argt, const LaneData_t* data) {
    DC_CSV_INHEADER(D, argt->header);
    if (argt->metadata) {
        DC_CSV_PRINTF(D, "metadata", "%s", "");
        DC_CSV_PRINTF(D, "timestamp", "%s", data->timestamp);
        char firmware_time[128];
        strify_timedelta(data->firmware_time, true, firmware_time);
        DC_CSV_PRINTF(D, "firmware_time", "%s", firmware_time);
        DC_CSV_PRINTF(D, "ecid_raw", "0x%08X%08X", data->ecid_raw[0], data->ecid_raw[1]);
        DC_CSV_PRINTF(D, "chip_revision", "rev%X", data->chip_rev);
        DC_CSV_PRINTF(D, "firmware_version", "%s", data->firmware_version);
        DC_CSV_PRINTF(D, "firmware_hash", "0x%06X", data->firmware_hash);
        DC_CSV_PRINTF(D, "sdk_version", "%s", data->sdk_version);
        DC_CSV_PRINTF(D, "slice", "%d", data->slice_id);
        DC_CSV_PRINTF(D, "temp", "%.2f", data->temp);
        if (data->port_exists) {
            DC_CSV_PRINTF(D, "port", "%u", data->port);
        } else {
            DC_CSV_PRINTF(D, "port", "%s", "");
        }
        DC_CSV_PRINTF(D, "lane", "%d", data->lane);
        DC_CSV_PRINTF(D, "lane_id", "%s", data->lane_id);
    }
    if (argt->serdes_control) {
        DC_CSV_PRINTF(D, "serdes_control", "%s", "");
        DC_CSV_PRINTF(D, "lane_mode", "%s", lane_mode_to_string(data->lane_mode));
        {
            DC_CSV_OPT(D, data->lane_mode == CR_LMODE_OFF);
            DC_CSV_PRINTF(D, "lane_speed", "%.3fG", (double)data->lane_speed / 1.0e6);
            DC_CSV_PRINTF(D, "tx_state", "%s", stringify_tx_state(data->tx_state));
            DC_CSV_PRINTF(D, "loopback_mode", "%s", stringify_tx_loopback_mode(data->loopback_mode));
            DC_CSV_PRINTF(D, "tx_pol", "%d", data->tx_pol);
            DC_CSV_PRINTF(D, "rx_pol", "%d", data->rx_pol);
            DC_CSV_PRINTF(D, "tx_gc", "%d", data->tx_gc);
            DC_CSV_PRINTF(D, "rx_gc", "%d", data->rx_gc);
            DC_CSV_PRINTF(D, "tx_pc", "%d", data->tx_pc);
            DC_CSV_PRINTF(D, "rx_pc", "%d", data->rx_pc);
            DC_CSV_PRINTF(D, "tx_msblsb", "%d", data->tx_msblsb);
            DC_CSV_PRINTF(D, "rx_msblsb", "%d", data->rx_msblsb);
            DC_CSV_PRINTF(D, "rx_coupling", "%s", data->rx_coupling ? "AC" : "DC");
            DC_CSV_PRINTF(D, "tx_tap_pre3,tx_tap_pre2,tx_tap_pre1,tx_tap_main,tx_tap_post1,tx_tap_post2,tx_tap_post3",
                          "%d,%d,%d,%d,%d,%d,%d", data->tx_taps[0], data->tx_taps[1], data->tx_taps[2],
                          data->tx_taps[3], data->tx_taps[4], data->tx_taps[5], data->tx_taps[6]);
            DC_CSV_OPTEND(D);
        }
    }

    if (argt->serdes_param) {
        DC_CSV_PRINTF(D, "serdes_param", "%s", "");
        {
            DC_CSV_OPT(D, data->lane_mode == CR_LMODE_OFF);
            char timedelta[64];
            strify_timedelta(data->up_time, true, timedelta);
            DC_CSV_PRINTF(D, "up_time", "%s", timedelta);
            strify_timedelta(data->down_time, true, timedelta);
            DC_CSV_PRINTF(D, "down_time", "%s", timedelta);
            DC_CSV_PRINTF(D, "adapt_count", "%u", data->adapt_count);
            DC_CSV_PRINTF(D, "readapt_count", "%u", data->readapt_count);
            DC_CSV_PRINTF(D, "link_loss_count", "%u", data->link_loss_count);
            DC_CSV_PRINTF(D, "loss_signal_count", "%u", data->loss_signal_count);
            DC_CSV_PRINTF(D, "signal_detect", "%d", data->signal_detect);
            DC_CSV_PRINTF(D, "rx_ready", "%d", data->rx_ready);
            DC_CSV_PRINTF(D, "adapt_done", "%u", data->phy_ready);
            DC_CSV_PRINTF(D, "top_pll_cap", "%u", data->top_pll);
            DC_CSV_PRINTF(D, "tx_pll_cap", "%u", data->tx_pll);
            DC_CSV_PRINTF(D, "rx_pll_cap", "%u", data->rx_pll);
            DC_CSV_PRINTF(D, "rx_cdr", "%s", data->rx_cdr_bb_mode ? "BB" : "MM");
            DC_CSV_PRINTF(D, "rx_ppm", "%d", data->rx_ppm);
            DC_CSV_PRINTF(D, "rx_psd0,rx_psd1", "%.3f,%.3f", data->rx_psd[0], data->rx_psd[1]);
            DC_CSV_PRINTF(D, "rx_channel_est", "%.3f", data->rx_channel_est);
            DC_CSV_PRINTF(D, "rx_snr", "%.2f", data->rx_snr);
            DC_CSV_PRINTF(D, "fw_preset_idx", "%d", data->fw_preset_idx);
            DC_CSV_PRINTF(D, "rx_envelope0,rx_envelope1", "%d,%d", data->rx_envelope[0], data->rx_envelope[1]);
            DC_CSV_PRINTF(D, "rx_skef_addcap", "%d", data->rx_skef_addcap);
            DC_CSV_PRINTF(D, "rx_skef_degen", "%d", data->rx_skef_degen);
            DC_CSV_PRINTF(D, "rx_skef_en", "%d", data->rx_skef_en);
            if (data->lane_mode == CR_LMODE_PAM4) {
                DC_CSV_PRINTF(D, "rx_eye_height0,rx_eye_height1,rx_eye_height2", "%d,%d,%d", data->rx_eye_height[0],
                              data->rx_eye_height[1], data->rx_eye_height[2]);
            } else {
                DC_CSV_PRINTF(D, "rx_eye_height0,rx_eye_height1,rx_eye_height2", "%d,,", data->rx_eye_height[0]);
            }
            DC_CSV_PRINTF(D, "rx_agcgain0,rx_agcgain1,rx_agcgain2,rx_agcgain3", "%d,%d,%d,%d", data->rx_agcgain[0],
                          data->rx_agcgain[1], data->rx_agcgain[2], data->rx_agcgain[3]);
            DC_CSV_PRINTF(D, "rx_ctle0,rx_ctle1", "%d,%d", data->rx_ctle[0], data->rx_ctle[1]);
            DC_CSV_PRINTF(D, "rx_ctle_cs0,rx_ctle_cs1", "%d,%d", data->rx_ctle_cs[0], data->rx_ctle_cs[1]);
            DC_CSV_PRINTF(D, "rx_ctle_ind1,rx_ctle_ind2", "%u,%u", data->rx_ctle_ind[0], data->rx_ctle_ind[1]);
            DC_CSV_PRINTF(D, "rx_ctle_ictrl1,rx_ctle_ictrl2", "%u,%u", data->rx_ctle_ictrl[0], data->rx_ctle_ictrl[1]);
            DC_CSV_PRINTF(D, "rx_adc_ref_vctrl", "%u", data->rx_adc_ref_vctrl);
            DC_CSV_PRINTF(D, "rx_atten_gain", "%d", data->rx_atten_gain);
            DC_CSV_PRINTF(D, "rx_vga", "%d", data->rx_vga);
            DC_CSV_PRINTF(D, "rx_dfe0,rx_dfe1,rx_dfe_nl", "%d,%d,%s", data->rx_dfe[0], data->rx_dfe[1],
                          data->rx_dfe_nl ? "NL" : "LI");
            {
                DC_CSV_PRINTF_HEADER_ARRAY(D, "rx_ffe_pre%d", 0, 4);
                DC_CSV_PRINTF_HEADER_ARRAY(D, "rx_ffe_post%d", 0, 8);
                DC_CSV_PRINTF_HEADER_ARRAY(D, "rx_ffe_float%d", 0, 8);
                DC_CSV_PRINTF_CELL_ARRAY(D, "%d", data->rx_ffe_taps, 20);
            }
            {
                DC_CSV_PRINTF_HEADER_ARRAY(D, "rx_ffe_float_loc%d", 0, 8);
                DC_CSV_PRINTF_CELL_ARRAY(D, "%d", data->rx_ffe_taps_float_loc, 8);
            }

            DC_CSV_OPTEND(D);
        }
    }

    if (argt->serdes_diag) {
        DC_CSV_PRINTF(D, "serdes_diag", "%s", "");
        {
            DC_CSV_OPT(D, data->lane_mode == CR_LMODE_OFF);
            DC_CSV_PRINTF(
                D,
                "rx_kf,rx_kp,rx_phase_fast_rotate,rx_ted_slop_dec,rx_dloop_freeze,rx_thdly_k,rx_thdly_sel,rx_thdly_en",
                "%u,%u,%u,%u,%u,%u,%u,%u", data->rx_kf, data->rx_kp, data->rx_phase_fast_rotate, data->rx_ted_slope_dec,
                data->rx_delay_loop_freeze, data->rx_thdly_ktheta, data->rx_thdly_acc_in_sel, data->rx_thdly_en);
            DC_CSV_PRINTF_HEADER_ARRAY(D, "rx_ph_trim%d", 0, PHASE_NUM);
            DC_CSV_PRINTF_CELL_ARRAY(D, "%u", data->rx_thdly, PHASE_NUM);
            for (int r = 0; r < DSP_SAR_COUNT / PHASE_NUM; r++) {
                for (int i = 0; i < PHASE_NUM; i++) {
                    DC_CSV_PRINTF_HEADER(D, "rx_sar_dc%d_ph%d", r, i);
                }
            }
            DC_CSV_PRINTF_CELL_ARRAY(D, "%d", data->rx_dc_sar, DSP_SAR_COUNT);
            for (int r = 0; r < DSP_SAR_COUNT / PHASE_NUM; r++) {
                for (int i = 0; i < PHASE_NUM; i++) {
                    DC_CSV_PRINTF_HEADER(D, "rx_sar_gain%d_ph%d", r, i);
                }
            }
            DC_CSV_PRINTF_CELL_ARRAY(D, "%d", data->rx_gain_sar, DSP_SAR_COUNT);
            DC_CSV_OPTEND(D);
        }
    }

    if (argt->pll) {
        DC_CSV_PRINTF(D, "pll", "%s", "");
        {
            DC_CSV_OPT(D, data->lane_mode == CR_LMODE_OFF);
            DC_CSV_PRINTF(D, "top_pll_min,top_pll_sel,top_pll_max,top_pll_valid_count", "%u,%u,%u,%u",
                          data->top_pll_min, data->top_pll_sel, data->top_pll_max, data->top_pll_valid_count);
            DC_CSV_PRINTF(D, "tx_pll_min,tx_pll_sel,tx_pll_max,tx_pll_valid_count", "%u,%u,%u,%u", data->tx_pll_min,
                          data->tx_pll_sel, data->tx_pll_max, data->tx_pll_valid_count);
            DC_CSV_PRINTF(D, "rx_pll_min,rx_pll_sel,rx_pll_max,rx_pll_valid_count", "%u,%u,%u,%u", data->rx_pll_min,
                          data->rx_pll_sel, data->rx_pll_max, data->rx_pll_valid_count);
            DC_CSV_OPTEND(D);
        }
    }
    if (argt->isi) {
        DC_CSV_PRINTF(D, "isi", "%s", "");
        {
            DC_CSV_OPT(D, data->lane_mode == CR_LMODE_OFF);
            DC_CSV_PRINTF_HEADER_ARRAY(D, "isi%d", 0, 67);
            DC_CSV_PRINTF_CELL_ARRAY(D, "%.2g", data->isi, 67);
            DC_CSV_OPTEND(D);
        }
    }
    if (argt->prbs) {
        DC_CSV_PRINTF(D, "prbs", "%s", "");
        DC_CSV_PRINTF(D, "prbs_enabled", "%s", data->prbs_enabled ? "true" : "false");
        {
            DC_CSV_OPT(D, !data->prbs_enabled);
            DC_CSV_PRINTF(D, "prbs_autosync", "%s", data->prbs_autosync ? "enabled" : "disabled");
            DC_CSV_PRINTF(D, "prbs_pattern", "%s", prbs_pattern_to_string(data->prbs_pattern));
            DC_CSV_PRINTF(D, "prbs_count", "%u", data->prbs_count);
            DC_CSV_PRINTF(D, "prbs_lock", "%s", data->prbs_lock == CR_PRBS_LOCK_YES ? "true" : "false");
            DC_CSV_PRINTF(D, "prbs_ber", "%.3e", data->prbs_ber);
            DC_CSV_PRINTF(D, "prbs_duration", "%s", STRIFY_TIMEDELTA(data->prbs_duration, true));
            DC_CSV_OPTEND(D);
        }
    }
    if (argt->fecana) {
        DC_CSV_PRINTF(D, "fecana", "%s", "");
        DC_CSV_PRINTF(D, "fec_enabled", "%s", data->fec_enabled ? "true" : "false");
        {
            DC_CSV_OPT(D, !data->fec_enabled);
            DC_CSV_PRINTF(D, "fec_autosync", "%s", data->prbs_autosync ? "enabled" : "disabled");
            DC_CSV_PRINTF(D, "fec_error_type", "%s", strify_fec_error_type(data->fec_error_type));
            DC_CSV_PRINTF(D, "fec_threshold", "%u", data->fec_config.threshold);
            DC_CSV_PRINTF(D, "fec_pre_count", "%u", data->fec_pre_count);
            DC_CSV_PRINTF(D, "fec_post_count", "%u", data->fec_post_count);
            DC_CSV_PRINTF(D, "fec_pre_error_rate", "%.3e", data->fec_pre_error_rate);
            DC_CSV_PRINTF(D, "fec_post_error_rate", "%.3e", data->fec_post_error_rate);
            DC_CSV_PRINTF(D, "fec_duration", "%s", STRIFY_TIMEDELTA(data->fec_duration, true));
            DC_CSV_OPTEND(D);
        }
    }
    if (argt->fecana_hist) {
        DC_CSV_PRINTF(D, "fecana_hist", "%s", "");
        {
            DC_CSV_OPT(D, !data->fec_hist_enabled);
            DC_CSV_PRINTF_HEADER_ARRAY(D, "fec_hist%d", 0, 16);
            DC_CSV_PRINTF_CELL_ARRAY(D, "%u", data->fec_hist, 16);
            DC_CSV_PRINTF(D, "fec_hist_duration", "%s", STRIFY_TIMEDELTA(data->fec_hist_duration, true));
            DC_CSV_OPTEND(D);
        }
    } else if (argt->fecana_histgroup) {  // print just hist group if user doesnt enable full histogram
        DC_CSV_PRINTF(D, "fecana_hist", "%s", "");
        {
            DC_CSV_OPT(D, !data->fec_hist_enabled);
            DC_CSV_PRINTF_HEADER_ARRAY(D, "fec_hist%d", 0, 16);
            for (size_t i = 0; i < 16; i++) {
                if (i / 4 != data->fec_histgroup) {
                    DC_CSV_PRINTF_CELL(D, "%s", "");
                } else {
                    DC_CSV_PRINTF_CELL(D, "%u", data->fec_histgroup_bins[i % 4]);
                }
            }
            DC_CSV_PRINTF(D, "fec_hist_duration", "%s", STRIFY_TIMEDELTA(data->fec_duration, true));
            DC_CSV_OPTEND(D);
        }
    }
    if (argt->port_info) {
        DC_CSV_PRINTF(D, "port_info", "%s", "");
        {
            DC_CSV_OPT(D, !data->port_exists);
            DC_CSV_PRINTF(D, "port_mode", "%s", strify_port_connection_mode(data->port_mode));
            DC_CSV_PRINTF(D, "port_speed", "%.1fG", (double)data->port_speed / 1000);
            DC_CSV_PRINTF(D, "port_host_up", "%s", (data->port_host_up) ? "true" : "false");
            DC_CSV_PRINTF(D, "port_line_up", "%s", (data->port_line_up) ? "true" : "false");
            DC_CSV_PRINTF(D, "port_host_lanes", "%u", data->port_host_lanes);
            DC_CSV_PRINTF(D, "port_line_lanes", "%u", data->port_line_lanes);
            DC_CSV_OPTEND(D);
        }
    }
    if (argt->retimer) {
        DC_CSV_PRINTF(D, "retimer", "%s", "");
        {
            DC_CSV_OPT(D, !data->port_exists || data->port_mode != CR_PORT_RETIMER);
            DC_CSV_PRINTF(D, "retimer_host_state", "%d", data->retimer_host_state);
            DC_CSV_PRINTF(D, "retimer_line_state", "%d", data->retimer_line_state);
            DC_CSV_PRINTF(D, "retimer_fifo0,retimer_fifo1", "%u,%u", data->retimer_fifo[0], data->retimer_fifo[1]);
            DC_CSV_OPTEND(D);
        }
    }
    if (argt->bitmux) {
        DC_CSV_PRINTF(D, "bitmux", "%s", "");
        {
            DC_CSV_OPT(D, !data->port_exists || data->port_mode != CR_PORT_BITMUX);
            DC_CSV_PRINTF(D, "bitmux_host_state", "%d", data->bitmux_host_state);
            DC_CSV_PRINTF(D, "bitmux_line_state", "%d", data->bitmux_line_state);
            DC_CSV_PRINTF(D, "bitmux_fifo0,bitmux_fifo1", "%u,%u", data->bitmux_fifo[0], data->bitmux_fifo[1]);
            DC_CSV_OPTEND(D);
        }
    }
    if (argt->exit_code) {
        DC_CSV_PRINTF(D, "exit_code", "%s", "");
        {
            DC_CSV_OPT(D, data->lane_mode == CR_LMODE_OFF);
            DC_CSV_PRINTF_HEADER_ARRAY(D, "exit_code%d", 0, 16);
            DC_CSV_PRINTF_CELL_ARRAY(D, "0x%X", data->exit_code, 16);
            DC_CSV_OPTEND(D);
        }
    }
    DC_CSV_LN(D);
}

CredoError_t se_lane_data_capture(CredoSlice_t* slice, const CredoDataCaptureArg_t argv[], size_t argc,
                                  DataCaptureState_t* D) {
    LaneDataArgTable_t argt = lane_data_default_args;
    DataCaptureArgMap_t map[] = {
        {"header", &argt.header},
        {"json", &argt.json},
        {"metadata", &argt.metadata},
        {"lane", &argt.lane},
        {"serdes_control", &argt.serdes_control},
        {"serdes_param", &argt.serdes_param},
        {"serdes_diag", &argt.serdes_diag},
        {"pll", &argt.pll},
        {"isi", &argt.isi},
        {"prbs", &argt.prbs},
        {"fecana", &argt.fecana},
        {"fecana_histgroup", &argt.fecana_histgroup},
        {"fecana_hist", &argt.fecana_hist},
        {"fecana_hist_duration", &argt.fecana_hist_duration},
        {"port_info", &argt.port_info},
        {"bitmux", &argt.bitmux},
        {"retimer", &argt.retimer},
        {"exit_code", &argt.exit_code},
        {"debug_mode", &argt.debug_mode},
        {NULL, NULL},
    };
    // extract args passed by user
    size_t parsed_args = datacap_extract_args(argv, argc, map);
    if (parsed_args < argc) {
        LOGS_ERROR("Invalid argument for lane_data command: '%s'", argv[parsed_args].name);
        return CR_INVALID_ARGS;
    }

    if (argt.debug_mode) {
        int lane = argt.lane;
        int header = argt.header;
        argt = lane_data_default_args;
        // turn on all diagnostics
        argt.serdes_diag = 1;
        argt.isi = 1;
        argt.pll = 1;
        // restore user selected lane
        argt.lane = lane;
        argt.header = header;
    }
    // capture data
    LaneData_t data = {{0}};
    if (!argt.header) {
        ERR_PROPS(se_lane_data_capture_data(slice, &argt, &data));
    }
    se_lane_data_write(D, &argt, &data);
    return CR_OK;
}

const DataCaptureCommand_t se_data_capture_list[] = {
    {
        .name = "lane_data",
        .description = "Capture all of the lane data into a CSV format",
        .params =
            {
                {"header", 0, "Print header in the output"},
                {"json", 0, "Produce json instead of csv format output"},
                {"lane", 0, "Lane to capture data"},
                {"metadata", 1, "Metadata information about the slice, lane, port, and environment"},
                {"serdes_control", 1, ""},
                {"serdes_param", 1, ""},
                {"serdes_diag", 0, ""},
                {"pll", 0, ""},
                {"isi", 0, ""},
                {"prbs", 1, ""},
                {"fecana", 1, ""},
                {"fecana_histgroup", 1, "Show current fecana histogram group."},
                {"fecana_hist", 0, "Show full histogram instead of current hist group."},
                {"fecana_hist_duration", 250, ""},
                {"port_info", 1, ""},
                {"bitmux", 1, ""},
                {"retimer", 1, ""},
                {"exit_code", 1, ""},
                {"debug_mode", 0, "If enabled, this will force domains to a preset for tons of debug information."},
                {NULL},
            },
        .runner = se_lane_data_capture,
    },
    {NULL},
};

CredoError_t se_data_capture(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[], size_t argc,
                             CredoDataWriter_t writer, void* ud) {
    const DataCaptureCommand_t* def = datacap_find_command(se_data_capture_list, command);
    if (def == NULL) {
        LOGS_ERROR("Invalid command: '%s'", command);
        return CR_INVALID_ARGS;
    }
    DataCaptureState_t D = {.ud = ud, .writer = writer, .inHeader = false, .optMode = 0, .prefixComma = false};
    return def->runner(slice, argv, argc, &D);
}

// utility for documentation generation
void* se_data_get_commands(CredoSlice_t* slice, size_t* command_count) {
    *command_count = COUNT_OF(se_data_capture_list) - 1;  // remove NULL from count
    return (void*)se_data_capture_list;
}
