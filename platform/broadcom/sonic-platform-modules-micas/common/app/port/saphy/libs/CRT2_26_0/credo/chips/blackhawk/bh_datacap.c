#include "bh_functions.h"
#include "blackhawk.h"
#include "strify.h"

#include "common/common_efuse.h"
#include "common/common_firmware.h"
#include "common/datacap.h"
#include "fec_analyzer/fec_analyzer.h"
#include "rsfec/rsfec.h"

#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <inttypes.h>
#include <string.h>
typedef struct {
    int header, lane, metadata, serdes_control, serdes_param, prbs, exit_code, port_info, isi, fecana, fecana_histgroup,
        fecana_hist, fecana_hist_duration, retimer, bitmux, gearbox, rsfec, debug_mode;
} LaneDataArgTable_t;

const LaneDataArgTable_t lane_data_default_args = {
    .header = 0,
    .lane = 0,
    .metadata = 1,
    .serdes_control = 1,
    .serdes_param = 1,
    .prbs = 1,

    .exit_code = 1,
    .isi = 0,

    .fecana = 1,
    .fecana_histgroup = 1,
    .fecana_hist = 0,
    .fecana_hist_duration = 250,

    .port_info = 1,
    .retimer = 1,
    .bitmux = 1,
    .gearbox = 1,

    .debug_mode = 0,

    .rsfec = 1,
};

typedef struct {
    // metadata
    char timestamp[64];
    uint32_t ecid_raw[2];
    // unsigned chip_rev;
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
    int rx_ppm;
    int rx_dac;
    // double rx_psd[2];
    double rx_channel_est;
    int rx_channel_of;
    int rx_channel_hf;
    // double rx_snr;
    // int fw_preset_idx;
    // int rx_envelope[2];
    int rx_skef_addcap;
    int rx_skef_degen;
    int rx_skef_en;
    int rx_skef_gain;
    int rx_eye_height[3];
    int rx_agcgain[2];
    int rx_ctle[2];

    int rx_delta;
    int rx_edge;
    int rx_atten_passive;
    int rx_atten_gain;
    int rx_atten_termtune;
    int rx_vga;
    double rx_dfe[3];
    int rx_ths[12];
    // int rx_dfe_nl;
    int rx_ffe_taps[8];
    int rx_ffe_taps_fine[8];
    int rx_ffe_nbias[8];
    double rx_ffe_kaccu[5];
    int rx_ffe_jump[5];
    int rx_f1over3;
    int rx_tia1_bias;
    // serdes diag analog (might just add this to serdes_param)

    // serdes diag

    int rx_kf;
    int rx_kp;

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
    CredoPortConnectionMode_t port_connection_mode;
    CredoPortMode_t port_mode;
    uint32_t port_speed;
    bool port_host_up;
    bool port_line_up;
    unsigned port_host_lanes;
    unsigned port_line_lanes;

    // retimer
    unsigned retimer_host_state;
    unsigned retimer_line_state;
    unsigned retimer_healing_en;
    unsigned retimer_heal_cnt;
    unsigned retimer_fifo[4];
    unsigned retimer_four_lane_cross;

    // bitmux
    unsigned bitmux_host_state;
    unsigned bitmux_line_state;
    unsigned bitmux_healing_en;
    unsigned bitmux_heal_cnt;
    unsigned bitmux_fifo_max[2];
    unsigned bitmux_fifo_min[2];
    unsigned bitmux_fifo_cur[2];

    // gearbox
    unsigned gearbox_host_state;
    unsigned gearbox_line_state;
    unsigned gearbox_fifo[2];
    unsigned gearbox_healing_en;
    unsigned gearbox_heal_cnt;
    fw_gearbox_info_t gb_info;

    // rsfec
    unsigned rsfec_fec_align;
    unsigned rsfec_pcs_align;
    CredoRSFECFifo_t rsfec_fifo;
    unsigned rsfec_in_buf_ptr;
    unsigned rsfec_out_buf_ptr;
    uint64_t rsfec_corr_cw;
    unsigned rsfec_uncorr_cw;
    uint64_t rsfec_total_cw;
    double rsfec_corr_err_rate;
    uint64_t rsfec_hist[16];

    // isi
    int isi[16];
    // exit_code
    unsigned exit_code[16];
} LaneData_t;

static CredoError_t bh_lane_data_capture_metadata(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                  LaneData_t* data) {
    strify_timestamp_now(data->timestamp);
    ERR_PROPS(cr_firmware_version_str(slice, data->firmware_version));
    ERR_PROPS(cr_firmware_hash(slice, &data->firmware_hash));

    ERR_PROPS(common_efuse_read_ecid(slice, data->ecid_raw));

    data->sdk_version = cr_sdk_version_str();
    data->slice_id = slice->slice_id;
    ERR_PROPS(cr_slice_get_temperature(slice, &data->temp));
    data->lane = lane;
    data->lane_id = stringify_lane_id(slice, lane);
    return CR_OK;
}

static CredoError_t bh_lane_data_capture_serdes_control(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
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

static CredoError_t bh_lane_data_capture_serdes_param(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                      LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    ERR_PROPS(cr_serdes_get_adapt_count(slice, lane, &data->adapt_count));
    ERR_PROPS(cr_serdes_get_readapt_count(slice, lane, &data->readapt_count));
    ERR_PROPS(cr_serdes_get_link_lost_count(slice, lane, &data->link_loss_count));
    ERR_PROPS(cr_serdes_get_rx_signal_detect(slice, lane, &data->signal_detect));
    ERR_PROPS(cr_serdes_get_rx_ready(slice, lane, &data->rx_ready));
    ERR_PROPS(cr_serdes_get_phy_ready(slice, lane, &data->phy_ready));

    ERR_PROPS(cr_serdes_get_rx_ppm(slice, lane, &data->rx_ppm));
    ERR_PROPS(cr_serdes_get_rx_dac(slice, lane, &data->rx_dac));

    ERR_PROPS(cr_serdes_get_top_pll_cap(slice, &data->top_pll));
    ERR_PROPS(cr_serdes_get_tx_cap(slice, lane, &data->tx_pll));
    ERR_PROPS(cr_serdes_get_rx_cap(slice, lane, &data->rx_pll));

    // ERR_PROPS(hal_serdes_get_param(slice, "rx_snr", lane, CR_PDATA_F(&data->rx_snr, 1)));
    // ERR_PROPS(hal_serdes_get_param(slice, "rx_channel_est_psd", lane, CR_PDATA_F(data->rx_psd, 2)));
    ERR_PROPS(cr_serdes_get_channel_estimate(slice, lane, &data->rx_channel_est));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_channel_of", lane, CR_PDATA_I(&data->rx_channel_of, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_channel_hf", lane, CR_PDATA_I(&data->rx_channel_hf, 1)));

    // ERR_PROPS(hal_serdes_get_param(slice, "fw_preset_idx", lane, CR_PDATA_I(&data->fw_preset_idx, 1)));

    // ERR_PROPS(hal_serdes_get_param(slice, "rx_envelope", lane, CR_PDATA_I(data->rx_envelope, 2)));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_skef_en", lane, CR_PDATA_I(&data->rx_skef_en, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_skef_degen", lane, CR_PDATA_I(&data->rx_skef_degen, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_skef_addcap", lane, CR_PDATA_I(&data->rx_skef_addcap, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_skef_gain", lane, CR_PDATA_I(&data->rx_skef_gain, 1)));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_eye_height", lane, CR_PDATA_I(data->rx_eye_height, 3)));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_agcgain", lane, CR_PDATA_I(data->rx_agcgain, 2)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_ctle", lane, CR_PDATA_I(data->rx_ctle, 2)));
    // ERR_PROPS(hal_serdes_get_param(slice, "rx_ctle_cs", lane, CR_PDATA_I(data->rx_ctle_cs, 2)));
    // ERR_PROPS(swift_get_rx_ind(slice, lane, data->rx_ctle_ind));
    // ERR_PROPS(swift_get_rx_ctle_ictrl(slice, lane, data->rx_ctle_ictrl));
    // ERR_PROPS(swift_get_rx_adc_ref_ctrl(slice, lane, &data->rx_adc_ref_vctrl));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_delta", lane, CR_PDATA_I(&data->rx_delta, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_edge", lane, CR_PDATA_I(&data->rx_edge, 1)));

    ERR_PROPS(hal_serdes_get_param(slice, "rx_atten_passive", lane, CR_PDATA_I(&data->rx_atten_passive, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_atten_gain", lane, CR_PDATA_I(&data->rx_atten_gain, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_atten_termtune", lane, CR_PDATA_I(&data->rx_atten_termtune, 1)));
    // ERR_PROPS(hal_serdes_get_param(slice, "rx_vga", lane, CR_PDATA_I(&data->rx_vga, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_dfe", lane, CR_PDATA_F(data->rx_dfe, 3)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_ths", lane, CR_PDATA_I(data->rx_ths, 12)));
    // ERR_PROPS(swift_get_rx_dfe_nonlinear_mode(slice, lane, &data->rx_dfe_nl));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_ffe_taps", lane, CR_PDATA_I(data->rx_ffe_taps, 8)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_ffe_taps_fine", lane, CR_PDATA_I(data->rx_ffe_taps_fine, 8)));

    if (data->lane_mode != CR_LMODE_NRZ) {
        ERR_PROPS(hal_serdes_get_param(slice, "rx_ffe_nbias", lane, CR_PDATA_I(data->rx_ffe_nbias, 8)));
        ERR_PROPS(hal_serdes_get_param(slice, "rx_ffe_kaccu", lane, CR_PDATA_F(data->rx_ffe_kaccu, 5)));
        ERR_PROPS(hal_serdes_get_param(slice, "rx_ffe_flip_counter", lane, CR_PDATA_I(data->rx_ffe_jump, 5)));
    }

    ERR_PROPS(hal_serdes_get_param(slice, "rx_f1over3", lane, CR_PDATA_I(&data->rx_f1over3, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_tia1_bias", lane, CR_PDATA_I(&data->rx_tia1_bias, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_kp", lane, CR_PDATA_I(&data->rx_kp, 1)));
    ERR_PROPS(hal_serdes_get_param(slice, "rx_kf", lane, CR_PDATA_I(&data->rx_kf, 1)));

    return CR_OK;
}

static CredoError_t bh_lane_data_capture_isi(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                             LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    ERR_PROPS(common_fw_get_isi(slice, lane, data->isi));
    return CR_OK;
}

static CredoError_t bh_lane_data_capture_prbs(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
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

static CredoError_t bh_lane_data_capture_fecana(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
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

static CredoError_t bh_lane_data_capture_fecana_hist(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
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

static CredoError_t bh_lane_data_capture_port_info(CredoSlice_t* slice, int lane, uint32_t port_id,
                                                   const LaneDataArgTable_t* argt, LaneData_t* data) {
    if (!data->port_exists) {
        return CR_OK;
    }
    BhSlice_t* bhslice = (BhSlice_t*)slice;
    BhPortInfo_t* info = &bhslice->port_info[port_id];

    data->port_speed = info->port_config.speed;
    data->port_mode = info->port_config.port_mode;
    data->port_connection_mode = info->port_config.connection_mode;
    data->port_host_lanes = info->port_config.host_no_of_lanes;
    data->port_line_lanes = info->port_config.line_no_of_lanes;

    ERR_PROPS(cr_port_is_link_up(slice, port_id, CR_SIDE_HOST, &data->port_host_up));
    ERR_PROPS(cr_port_is_link_up(slice, port_id, CR_SIDE_LINE, &data->port_line_up));

    return CR_OK;
}

CredoError_t bh_lane_data_capture_bitmux(CredoSlice_t* slice, int lane, uint32_t port_id,
                                         const LaneDataArgTable_t* argt, LaneData_t* data) {
    if (!data->port_exists || data->port_connection_mode != CR_PORT_BITMUX) {
        return CR_OK;
    }
#define A_SIDE_FIFO_START_IDX 0
#define B_SIDE_FIFO_START_IDX 4

#define PAM4_SPLIT_FIFO_START_IDX 0
#define PAM4_MERGE_FIFO_START_IDX 2

#define NRZ_SPLIT_FIFO_START_IDX 4
#define NRZ_MERGE_FIFO_START_IDX 6

    ERR_PROPS(cr_port_get_param(slice, "bitmux_host_state", port_id, CR_PDATA_U(&data->bitmux_host_state, 1)));
    ERR_PROPS(cr_port_get_param(slice, "bitmux_line_state", port_id, CR_PDATA_U(&data->bitmux_line_state, 1)));
    unsigned healing_en = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &healing_en));
    data->bitmux_healing_en = (healing_en >> 4) & 0x1;
    ERR_PROPS(bh_fw_get_bitmux_debug(slice, lane, BITMUX_DEBUG_FIFO_HEAL_COUNT, &data->bitmux_heal_cnt));

    bool read_from_fw = false;
    unsigned fw_feature = 0;
    ERR_PROP(bh_fw_get_feature(slice, &fw_feature));
    if ((fw_feature & FW_FEATURE_BITMUX_FW_PDIFF) == 0) {
        read_from_fw = false;
    } else {
        read_from_fw = true;
    }

    BhSlice_t* bhslice = (BhSlice_t*)slice;
    BhPortInfo_t* info = &bhslice->port_info[port_id];
    CredoPortConfig_t* port_config = &info->port_config;

    uint32_t ctrl_val = 0;
    ERR_PROPS(readReg(slice, REG_BM_CTRL_S, &ctrl_val));

    unsigned lane_side_index = lane % 8;

    // bool a_side_merge = ((1 << fifo_idx) & ctrl_val);
    bool a_side_merge = port_config->host_no_of_lanes > port_config->line_no_of_lanes;
    bool merge = (lane < 8) ? a_side_merge : !a_side_merge;
    unsigned fifo_idx;
    if (merge) {
        fifo_idx = lane_side_index / 2;
    } else {
        fifo_idx = (lane_side_index < 4) ? lane_side_index : lane_side_index - 2;
    }

    unsigned fifo_ab_side_sel = (lane < 8) ? A_SIDE_FIFO_START_IDX : B_SIDE_FIFO_START_IDX;
    if (read_from_fw == false) {
        ERR_PROPS(writeReg(slice, REG_BM_BUF_SEL_LANE, fifo_ab_side_sel + fifo_idx));
    }
    bool nrz = ctrl_val & ((1 << fifo_idx) << 8);

    unsigned fifo_start_idx;
    if (merge) {
        fifo_start_idx = (nrz) ? NRZ_MERGE_FIFO_START_IDX : PAM4_MERGE_FIFO_START_IDX;
    } else {
        fifo_start_idx = (nrz) ? NRZ_SPLIT_FIFO_START_IDX : PAM4_SPLIT_FIFO_START_IDX;
    }

    unsigned fifo_val;
    for (int i = 0; i < 2; i++) {
        if (read_from_fw == false) {
            ERR_PROPS(writeReg(slice, REG_BM_BUF_SEL_SLICE, fifo_start_idx + i));
            ERR_PROPS(readReg(slice, REG_BM_BUF_SEL_READ, &fifo_val));
            fifo_val = fifo_val << 4;
        } else {
            ERR_PROPS(bh_fw_get_bitmux_fifo(slice, fifo_ab_side_sel + fifo_idx, fifo_start_idx + i, &fifo_val));
        }
        data->bitmux_fifo_max[i] = gray_bin((fifo_val >> 4) & 0xf);
        data->bitmux_fifo_min[i] = gray_bin((fifo_val >> 8) & 0xf);
        data->bitmux_fifo_cur[i] = gray_bin((fifo_val >> 12) & 0xf);
    }

    return CR_OK;
}

CredoError_t bh_lane_data_capture_retimer(CredoSlice_t* slice, int lane, uint32_t port_id,
                                          const LaneDataArgTable_t* argt, LaneData_t* data) {
    if (!data->port_exists || data->port_connection_mode != CR_PORT_RETIMER) {
        return CR_OK;
    }

    ERR_PROPS(cr_port_get_param(slice, "retimer_host_state", port_id, CR_PDATA_U(&data->retimer_host_state, 1)));
    ERR_PROPS(cr_port_get_param(slice, "retimer_line_state", port_id, CR_PDATA_U(&data->retimer_line_state, 1)));

    unsigned healing_en = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &healing_en));
    data->retimer_healing_en = (healing_en >> 4) & 0x1;
    ERR_PROPS(bh_fw_get_retimer_debug(slice, lane, RETIMER_DEBUG_FIFO_HEAL_COUNT, &data->retimer_heal_cnt));

    BhSlice_t* bhslice = (BhSlice_t*)slice;
    BhPortInfo_t* info = &bhslice->port_info[port_id];
    CredoPortConfig_t* port_config = &info->port_config;

    data->retimer_four_lane_cross = 0;
    ERR_PROPS(readReg(slice, REG_FOUR_LANE_CROSS, &data->retimer_four_lane_cross));

    int lane_offset = lane - ((lane < 8) ? port_config->host_start_lane : port_config->line_start_lane);
    int A = port_config->host_start_lane + lane_offset;
    int B = port_config->line_start_lane + lane_offset;

    int fifo_A = (data->retimer_four_lane_cross & (1 << A)) ? B : A;
    int fifo_B = (data->retimer_four_lane_cross & (1 << B)) ? A : B;

    ERR_PROPS(readReg(slice, REG_ADR_DIFF_A0(fifo_A), &data->retimer_fifo[0]));
    ERR_PROPS(readReg(slice, REG_ADR_DIFF_A1(fifo_A), &data->retimer_fifo[1]));
    ERR_PROPS(readReg(slice, REG_ADR_DIFF_B0(fifo_B), &data->retimer_fifo[2]));
    ERR_PROPS(readReg(slice, REG_ADR_DIFF_B1(fifo_B), &data->retimer_fifo[3]));

    for (int i = 0; i < 4; i++) {
        data->retimer_fifo[i] = gray_bin(data->retimer_fifo[i]);
    }

    return CR_OK;
}

CredoError_t bh_lane_data_capture_gearbox(CredoSlice_t* slice, int lane, uint32_t port_id,
                                          const LaneDataArgTable_t* argt, LaneData_t* data) {
    if (!data->port_exists || data->port_connection_mode != CR_PORT_GEARBOX) {
        return CR_OK;
    }

    ERR_PROPS(cr_port_get_param(slice, "gearbox_host_state", port_id, CR_PDATA_U(&data->gearbox_host_state, 1)));
    ERR_PROPS(cr_port_get_param(slice, "gearbox_line_state", port_id, CR_PDATA_U(&data->gearbox_line_state, 1)));

    unsigned healing_en = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &healing_en));
    data->gearbox_healing_en = (healing_en >> 4) & 0x1;
    ERR_PROPS(bh_fw_get_gearbox_debug(slice, lane, GEARBOX_DEBUG_FIFO_HEAL_COUNT, &data->gearbox_heal_cnt));
    ERR_PROPS(
        bh_fw_get_gearbox_info(slice, port_id, (lane < HOST_LANES) ? CR_SIDE_HOST : CR_SIDE_LINE, &data->gb_info));
    return CR_OK;
}

static CredoError_t bh_lane_data_capture_rsfec(CredoSlice_t* slice, int lane, uint32_t port_id,
                                               const LaneDataArgTable_t* argt, LaneData_t* data) {
    if (!data->port_exists || data->port_connection_mode != CR_PORT_GEARBOX) {
        return CR_OK;
    }
    CredoSide_t side = (lane < 8) ? CR_SIDE_HOST : CR_SIDE_LINE;
    ERR_PROPS(rsfec_get_fec_align_status(slice, port_id, side, &data->rsfec_fec_align));
    ERR_PROPS(rsfec_get_pcs_align_status(slice, port_id, side, &data->rsfec_pcs_align));
    ERR_PROPS(rsfec_get_fifo(slice, port_id, side, &data->rsfec_fifo));
    ERR_PROPS(rsfec_get_corrected_codeword(slice, port_id, side, &data->rsfec_corr_cw));
    ERR_PROPS(rsfec_get_uncorrected_codeword(slice, port_id, side, &data->rsfec_uncorr_cw));
    ERR_PROPS(rsfec_get_total_codewords(slice, port_id, side, &data->rsfec_total_cw));
    ERR_PROPS(rsfec_get_buf_pointer(slice, port_id, side, &data->rsfec_in_buf_ptr, &data->rsfec_out_buf_ptr));
    data->rsfec_corr_err_rate = (double)data->rsfec_corr_cw / (double)data->rsfec_total_cw;

    for (int bin = 0; bin < 16; bin++) {
        ERR_PROPS(rsfec_get_histogram(slice, port_id, side, bin, &data->rsfec_hist[bin]));
    }

    return CR_OK;
}

static CredoError_t bh_lane_data_capture_exit_codes(CredoSlice_t* slice, int lane, const LaneDataArgTable_t* argt,
                                                    LaneData_t* data) {
    if (data->lane_mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    for (unsigned index = 0; index < 16; index++) {
        ERR_PROPS(GET_FW_EXIT_CODES(slice, lane, ALL_EXIT_CODES_START + index, &data->exit_code[index]));
    }
    return CR_OK;
}

static bool bh_compute_lane_port(CredoSlice_t* slice, int lane, uint32_t* lane_port_id) {
    BhSlice_t* bhslice = (BhSlice_t*)slice;
    for (uint32_t port_id = 0; port_id < BH_MAX_PORT; port_id++) {
        BhPortInfo_t* info = &bhslice->port_info[port_id];
        if (!info->configured) {
            continue;
        }
        CredoPortConfig_t port_config = info->port_config;
        if ((port_config.host_start_lane <= lane &&
             lane < port_config.host_start_lane + port_config.host_no_of_lanes) ||
            (port_config.line_start_lane <= lane &&
             lane < port_config.line_start_lane + port_config.line_no_of_lanes)) {
            *lane_port_id = port_id;
            return true;
        }
    }
    return false;
}

static CredoError_t bh_lane_data_capture_data(CredoSlice_t* slice, const LaneDataArgTable_t* argt, LaneData_t* data) {
    // compute the lane's port (if it exists)
    data->port_exists = bh_compute_lane_port(slice, argt->lane, &data->port);
    int lane = argt->lane;
    ERR_PROPS(cr_lane_get_mode(slice, lane, &data->lane_mode));
    if (argt->metadata) {
        ERR_PROPS(bh_lane_data_capture_metadata(slice, argt->lane, argt, data));
    }
    if (argt->serdes_control) {
        ERR_PROPS(bh_lane_data_capture_serdes_control(slice, argt->lane, argt, data));
    }
    if (argt->serdes_param) {
        ERR_PROPS(bh_lane_data_capture_serdes_param(slice, argt->lane, argt, data));
    }
    if (argt->prbs) {
        ERR_PROPS(bh_lane_data_capture_prbs(slice, lane, argt, data));
    }
    if (argt->fecana) {
        ERR_PROPS(bh_lane_data_capture_fecana(slice, lane, argt, data));
    }
    if (argt->fecana_hist) {
        ERR_PROPS(bh_lane_data_capture_fecana_hist(slice, lane, argt, data));
    }
    if (argt->port_info) {
        ERR_PROPS(bh_lane_data_capture_port_info(slice, lane, data->port, argt, data));
    }
    if (argt->retimer) {
        ERR_PROPS(bh_lane_data_capture_retimer(slice, lane, data->port, argt, data));
    }

    if (argt->bitmux) {
        ERR_PROPS(bh_lane_data_capture_bitmux(slice, lane, data->port, argt, data));
    }

    if (argt->gearbox) {
        ERR_PROPS(bh_lane_data_capture_gearbox(slice, lane, data->port, argt, data));
    }

    if (argt->rsfec) {
        ERR_PROPS(bh_lane_data_capture_rsfec(slice, lane, data->port, argt, data));
    }
    if (argt->isi) {
        ERR_PROPS(bh_lane_data_capture_isi(slice, lane, argt, data));
    }

    if (argt->exit_code) {
        ERR_PROPS(bh_lane_data_capture_exit_codes(slice, lane, argt, data));
    }

    return CR_OK;
}

static int bh_lane_data_write_header(const DataCaptureState_t* D, const LaneDataArgTable_t* argt) {
    bool add_start_comma = false;
    if (argt->metadata) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D,
                  "metadata,timestamp,ecid_raw,firmware_version,firmware_hash,sdk_version,"
                  "slice,temp,"
                  "port,lane,lane_id");
    }
    if (argt->serdes_control) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D,
                  "serdes_control,lane_mode,lane_speed,tx_state,loopback_mode,tx_pol,rx_pol,tx_gc,rx_gc,tx_pc,rx_pc,tx_"
                  "msblsb,rx_msblsb,rx_coupling,tx_tap_pre3,tx_tap_pre2,tx_tap_pre1,tx_tap_main,tx_tap_post1,tx_tap_"
                  "post2,tx_tap_post3");
    }
    if (argt->serdes_param) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(
            D,
            "serdes_param,adapt_count,readapt_count,link_loss_count,loss_signal_count,signal_detect,"
            "rx_ready,adapt_done,top_pll_cap,tx_pll_cap,rx_pll_cap,rx_ppm,rx_dac,"
            "rx_channel_est,rx_channel_of,rx_channel_hf,"
            //"fw_preset_idx,"
            "rx_skef_addcap,rx_skef_degen,rx_skef_en,rx_eye_height0,rx_eye_height1,rx_eye_"
            "height2,rx_agcgain0,rx_agcgain1,rx_ctle0,rx_ctle1,"

            "rx_delta,rx_edge,"
            "rx_atten_passive,rx_atten_gain,rx_atten_termtune,rx_vga,rx_dfe0,rx_dfe1,rx_dfe2,"
            "rx_ths0,rx_ths1,rx_ths2,rx_ths3,rx_ths4,rx_ths5,rx_ths6,rx_ths7,rx_ths8,rx_ths9,rx_ths10,rx_ths11,"

            "rx_ffe_k1,rx_ffe_k2,rx_ffe_k3,rx_ffe_k4,rx_ffe_k5,rx_ffe_s0,rx_ffe_s1,rx_ffe_s2,"
            "rx_ffe_fine_k1,rx_ffe_fine_k2,rx_ffe_fine_k3,rx_ffe_fine_k4,rx_ffe_fine_k5,rx_ffe_fine_s0,rx_ffe_fine_s1,"
            "rx_ffe_fine_s2,"
            "rx_ffe_nbias_k1,rx_ffe_nbias_k2,rx_ffe_nbias_k3,rx_ffe_nbias_k4,rx_ffe_nbias_k5,rx_ffe_nbias_s0,rx_ffe_"
            "nbias_s1,rx_ffe_nbias_s2,"
            "rx_ffe_jump_k1,rx_ffe_jump_k2,rx_ffe_jump_k3,rx_ffe_jump_k4,rx_ffe_jump_k5,"
            "rx_ffe_kaccu_k1,rx_ffe_kaccu_k2,rx_ffe_kaccu_k3,rx_ffe_kaccu_k4,rx_ffe_kaccu_k5,"

            "rx_f1over3,rx_tia1_bias,rx_kp,rx_kf");
    }

    if (argt->isi) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, "isi,isi0,isi1,isi2,isi3,isi4,isi5,isi6,isi7,isi8,isi9,isi10,isi11,isi12,isi13,isi14,isi15");
    }

    if (argt->prbs) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, "prbs,prbs_enabled,prbs_autosync,prbs_pattern,prbs_count,prbs_lock,prbs_ber,prbs_duration");
    }
    if (argt->fecana) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D,
                  "fecana,fec_enabled,fec_autosync,fec_error_type,fec_threshold,fec_pre_count,fec_post_count,"
                  "fec_pre_error_rate,fec_post_error_rate,fec_duration");
    }
    if (argt->fecana_histgroup || argt->fecana_hist) {  // show histogram
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(
            D,
            "fecana_hist,fec_hist0,fec_hist1,fec_hist2,fec_hist3,fec_hist4,fec_hist5,fec_hist6,fec_hist7,fec_hist8,fec_"
            "hist9,fec_hist10,fec_hist11,fec_hist12,fec_hist13,fec_hist14,fec_hist15,fec_hist_duration");
    }
    if (argt->port_info) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D,
                  "port_info,port_mode,port_connection_mode,port_speed,port_host_up,port_line_up,port_host_lanes,port_"
                  "line_lanes");
    }
    if (argt->retimer) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D,
                  "retimer,retimer_host_state,retimer_line_state,retimer_healing_en,retimer_heal_cnt,"
                  "retimer_four_lane_cross,retimer_fifo0,retimer_fifo1,retimer_fifo2,retimer_fifo3");
    }

    if (argt->bitmux) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D,
                  "bitmux,bitmux_host_state,bitmux_line_state,bitmux_healing_en,bitmux_heal_cnt,"
                  "bitmux_fifo0_min,bitmux_fifo0_cur,bitmux_fifo0_max,"
                  "bitmux_fifo1_min,bitmux_fifo1_cur,bitmux_fifo1_max");
    }

    if (argt->gearbox) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(
            D,
            "gearbox,gb_host_state,gb_line_state,gb_healing_en,gb_heal_cnt,"
            "gb_fifo_tx_min,gb_fifo_tx_cur,gb_fifo_tx_max,gb_fifo_rx_min,gb_fifo_rx_cur,gb_fifo_rx_max,"
            "cfg_pdiff_rx,cdf_pdiff_tx,cdf_pdiff_hw_rx_min,cfg_pdiff_hw_rx_max,cfg_pdiff_hw_tx_min,cfg_pdiff_hw_tx_max,"
            "cfg_pdiff_dev_rx,cfg_pdiff_dev_tx,ncw_avg_rx,ncw_avg_tx,bip_sum_rx");
    }

    if (argt->rsfec) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(
            D,
            "rsfec,rsfec_fec_align,rsfec_pcs_align,rsfec_fifo_rx_min,rsfec_fifo_rx_cur,rsfec_fifo_rx_max,"
            "rsfec_fifo_tx_min,rsfec_fifo_tx_cur,rsfec_fifo_tx_max,rsfec_in_buf_ptr,rsfec_out_buf_ptr,"
            "rsfec_corr_cw,rsfec_uncorr_cw,rsfec_total_cw,rsfec_corr_err_rate"
            "rsfec_hist0,rsfec_hist1,rsfec_hist2,rsfec_hist3,rsfec_hist4,rsfec_hist5,rsfec_hist6,rsfec_hist7,"
            "rsfec_hist8,rsfec_hist9,rsfec_hist10,rsfec_hist11,rsfec_hist12,rsfec_hist13,rsfec_hist14,rsfec_hist15");
    }
    if (argt->exit_code) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D,
                  "exit_code,exit_code0,exit_code1,exit_code2,exit_code3,exit_code4,exit_code5,exit_code6,exit_code7,"
                  "exit_code8,exit_code9,exit_code10,exit_code11,exit_code12,exit_code13,exit_code14,exit_code15");
    }

    DC_PRINTF(D, "\n");
    return 0;
}

static int bh_lane_data_write(const DataCaptureState_t* D, const LaneDataArgTable_t* argt, const LaneData_t* data) {
    bool add_start_comma = false;
    if (argt->metadata) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        DC_PRINTF(D, "%s,", data->timestamp);
        DC_PRINTF(D, "0x%08X%08X,", data->ecid_raw[0], data->ecid_raw[1]);
        // DC_PRINTF(D, "rev%X,", data->chip_rev);
        DC_PRINTF(D, "%s,", data->firmware_version);
        DC_PRINTF(D, "0x%06X,", data->firmware_hash);
        DC_PRINTF(D, "%s,", data->sdk_version);
        DC_PRINTF(D, "%d,", data->slice_id);
        DC_PRINTF(D, "%.2f,", data->temp);
        if (data->port_exists) {
            DC_PRINTF(D, "%u,", data->port);
        } else {
            DC_PRINTF(D, ",");
        }
        DC_PRINTF(D, "%d,", data->lane);
        DC_PRINTF(D, "%s", data->lane_id);
    }
    if (argt->serdes_control) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        DC_PRINTF(D, "%s,", lane_mode_to_string(data->lane_mode));
        if (data->lane_mode == CR_LMODE_OFF) {
            for (size_t i = 0; i < 18; i++) {
                DC_PRINTF(D, ",");
            }
        } else {
            DC_PRINTF(D, "%.3fG,", (double)data->lane_speed / 1.0e6);
            DC_PRINTF(D, "%s,", stringify_tx_state(data->tx_state));
            DC_PRINTF(D, "%s,", stringify_tx_loopback_mode(data->loopback_mode));
            DC_PRINTF(D, "%d,", data->tx_pol);
            DC_PRINTF(D, "%d,", data->rx_pol);
            DC_PRINTF(D, "%d,", data->tx_gc);
            DC_PRINTF(D, "%d,", data->rx_gc);
            DC_PRINTF(D, "%d,", data->tx_pc);
            DC_PRINTF(D, "%d,", data->rx_pc);
            DC_PRINTF(D, "%d,", data->tx_msblsb);
            DC_PRINTF(D, "%d,", data->rx_msblsb);
            DC_PRINTF(D, "%s,", data->rx_coupling ? "AC" : "DC");
            DC_PRINTF(D, "%d,%d,%d,%d,%d,%d,%d", data->tx_taps[0], data->tx_taps[1], data->tx_taps[2], data->tx_taps[3],
                      data->tx_taps[4], data->tx_taps[5], data->tx_taps[6]);
        }
    }
    if (argt->serdes_param) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        if (data->lane_mode == CR_LMODE_OFF) {
            for (size_t i = 0; i < 83; i++) {
                DC_PRINTF(D, ",");
            }
        } else {
            DC_PRINTF(D, "%u,", data->adapt_count);
            DC_PRINTF(D, "%u,", data->readapt_count);
            DC_PRINTF(D, "%u,", data->link_loss_count);
            DC_PRINTF(D, "%u,", data->loss_signal_count);
            DC_PRINTF(D, "%d,", data->signal_detect);
            DC_PRINTF(D, "%d,", data->rx_ready);
            DC_PRINTF(D, "%u,", data->phy_ready);
            DC_PRINTF(D, "%u,", data->top_pll);
            DC_PRINTF(D, "%u,", data->tx_pll);
            DC_PRINTF(D, "%u,", data->rx_pll);
            DC_PRINTF(D, "%d,", data->rx_ppm);
            DC_PRINTF(D, "%d,", data->rx_dac);
            DC_PRINTF(D, "%.3f,", data->rx_channel_est);
            DC_PRINTF(D, "%d,", data->rx_channel_of);
            DC_PRINTF(D, "%d,", data->rx_channel_hf);
            DC_PRINTF(D, "%d,", data->rx_skef_addcap);
            DC_PRINTF(D, "%d,", data->rx_skef_degen);
            DC_PRINTF(D, "%d,", data->rx_skef_en);
            if (data->lane_mode == CR_LMODE_PAM4) {
                DC_PRINTF(D, "%d,%d,%d,", data->rx_eye_height[0], data->rx_eye_height[1], data->rx_eye_height[2]);
            } else {
                DC_PRINTF(D, "%d,,,", data->rx_eye_height[0]);
            }
            DC_PRINTF(D, "%d,%d,", data->rx_agcgain[0], data->rx_agcgain[1]);
            DC_PRINTF(D, "%d,%d,", data->rx_ctle[0], data->rx_ctle[1]);
            DC_PRINTF(D, "%d,", data->rx_delta);
            DC_PRINTF(D, "%d,", data->rx_edge);
            DC_PRINTF(D, "%d,", data->rx_atten_passive);
            DC_PRINTF(D, "%d,", data->rx_atten_gain);
            DC_PRINTF(D, "%d,", data->rx_atten_termtune);
            DC_PRINTF(D, "%d,", data->rx_vga);
            DC_PRINTF(D, "%.2f,%.2f,%.2f", data->rx_dfe[0], data->rx_dfe[1], data->rx_dfe[2]);
            for (size_t i = 0; i < 12; i++) {
                DC_PRINTF(D, ",%d", data->rx_ths[i]);
            }
            for (size_t i = 0; i < 8; i++) {
                DC_PRINTF(D, ",%d", data->rx_ffe_taps[i]);
            }
            for (size_t i = 0; i < 8; i++) {
                DC_PRINTF(D, ",%d", data->rx_ffe_taps_fine[i]);
            }
            if (data->lane_mode != CR_LMODE_NRZ) {
                for (size_t i = 0; i < 8; i++) {
                    DC_PRINTF(D, ",%d", data->rx_ffe_nbias[i]);
                }
                for (size_t i = 0; i < 5; i++) {
                    DC_PRINTF(D, ",%d", data->rx_ffe_jump[i]);
                }
                for (size_t i = 0; i < 5; i++) {
                    DC_PRINTF(D, ",%.6f", data->rx_ffe_kaccu[i]);
                }
            } else {
                for (size_t i = 0; i < 18; i++) {
                    DC_PRINTF(D, ",");
                }
            }
            DC_PRINTF(D, "%d,", data->rx_f1over3);
            DC_PRINTF(D, "%d,", data->rx_tia1_bias);
            DC_PRINTF(D, "%d,", data->rx_kp);
            DC_PRINTF(D, "%d,", data->rx_kf);
        }
    }

    if (argt->isi) {
        DC_START_COMMA(D, add_start_comma);
        // data group column covered by print out method
        if (data->lane_mode == CR_LMODE_OFF) {
            for (size_t i = 0; i < 16; i++) {
                DC_PRINTF(D, ",");
            }
        } else {
            for (size_t i = 0; i < 16; i++) {
                DC_PRINTF(D, ",%d", data->isi[i]);
            }
        }
    }

    if (argt->prbs) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        DC_PRINTF(D, "%s,", data->prbs_enabled ? "true" : "false");
        if (!data->prbs_enabled) {
            DC_PRINTF(D, ",,,,,");
        } else {
            DC_PRINTF(D, "%s,", data->prbs_autosync ? "enabled" : "disabled");
            DC_PRINTF(D, "%s,", prbs_pattern_to_string(data->prbs_pattern));
            DC_PRINTF(D, "%u,", data->prbs_count);
            DC_PRINTF(D, "%s,", data->prbs_lock == CR_PRBS_LOCK_YES ? "true" : "false");
            DC_PRINTF(D, "%.3e,", data->prbs_ber);
            DC_PRINTF(D, "%s", STRIFY_TIMEDELTA(data->prbs_duration, true));
        }
    }
    if (argt->fecana) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        DC_PRINTF(D, "%s,", data->fec_enabled ? "true" : "false");
        if (!data->fec_enabled) {
            DC_PRINTF(D, ",,,,,,,");
        } else {
            DC_PRINTF(D, "%s,", data->prbs_autosync ? "enabled" : "disabled");
            DC_PRINTF(D, "%s,", strify_fec_error_type(data->fec_error_type));
            DC_PRINTF(D, "%u,", data->fec_config.threshold);
            DC_PRINTF(D, "%u,", data->fec_pre_count);
            DC_PRINTF(D, "%u,", data->fec_post_count);
            DC_PRINTF(D, "%.3e,", data->fec_pre_error_rate);
            DC_PRINTF(D, "%.3e,", data->fec_post_error_rate);
            DC_PRINTF(D, "%s", STRIFY_TIMEDELTA(data->fec_duration, true));
        }
    }
    if (argt->fecana_hist) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        if (!data->fec_hist_enabled) {
            for (size_t i = 0; i < 16; i++) {
                DC_PRINTF(D, ",");
            }
        } else {
            for (size_t i = 0; i < 16; i++) {
                DC_PRINTF(D, "%u,", data->fec_hist[i]);
            }
            DC_PRINTF(D, "%s", STRIFY_TIMEDELTA(data->fec_hist_duration, true));
        }
    } else if (argt->fecana_histgroup) {  // print just hist group if user doesnt enable full histogram
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        if (!data->fec_hist_enabled) {
            for (size_t i = 0; i < 16; i++) {
                DC_PRINTF(D, ",");
            }
        } else {
            for (size_t i = 0; i < 16; i++) {
                if (i / 4 != data->fec_histgroup) {
                    DC_PRINTF(D, ",");
                } else {
                    DC_PRINTF(D, "%u,", data->fec_histgroup_bins[i % 4]);
                }
            }
            DC_PRINTF(D, "%s", STRIFY_TIMEDELTA(data->fec_duration, true));
        }
    }
    if (argt->port_info) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        if (!data->port_exists) {
            DC_PRINTF(D, ",,,,,,");
        } else {
            DC_PRINTF(D, "%s,", strify_port_mode(data->port_mode));
            DC_PRINTF(D, "%s,", strify_port_connection_mode(data->port_connection_mode));
            DC_PRINTF(D, "%.1fG,", (double)data->port_speed / 1000);
            DC_PRINTF(D, "%s,", (data->port_host_up) ? "true" : "false");
            DC_PRINTF(D, "%s,", (data->port_line_up) ? "true" : "false");
            DC_PRINTF(D, "%u,", data->port_host_lanes);
            DC_PRINTF(D, "%u", data->port_line_lanes);
        }
    }
    if (argt->retimer) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        if (!data->port_exists || data->port_connection_mode != CR_PORT_RETIMER) {
            DC_PRINTF(D, ",,,,,,,,");
        } else {
            DC_PRINTF(D, "%u,", data->retimer_host_state);
            DC_PRINTF(D, "%u,", data->retimer_line_state);
            DC_PRINTF(D, "%u,", data->retimer_healing_en);
            DC_PRINTF(D, "%u,", data->retimer_heal_cnt);
            DC_PRINTF(D, "%u,", data->retimer_four_lane_cross);
            DC_PRINTF(D, "%u,%u,%u,%u", data->retimer_fifo[0], data->retimer_fifo[1], data->retimer_fifo[2],
                      data->retimer_fifo[3]);
        }
    }

    if (argt->bitmux) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        if (!data->port_exists || data->port_connection_mode != CR_PORT_BITMUX) {
            DC_PRINTF(D, ",,,,,,,,,");
        } else {
            DC_PRINTF(D, "%u,", data->bitmux_host_state);
            DC_PRINTF(D, "%u,", data->bitmux_line_state);
            DC_PRINTF(D, "%u,", data->bitmux_healing_en);
            DC_PRINTF(D, "%u,", data->bitmux_heal_cnt);
            DC_PRINTF(D, "%u,%u,%u,", data->bitmux_fifo_min[0], data->bitmux_fifo_cur[0], data->bitmux_fifo_max[0]);
            DC_PRINTF(D, "%u,%u,%u", data->bitmux_fifo_min[1], data->bitmux_fifo_cur[1], data->bitmux_fifo_max[1]);
        }
    }

    if (argt->gearbox) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        if (!data->port_exists || data->port_connection_mode != CR_PORT_GEARBOX) {
            DC_PRINTF(D, ",,,,,,,,,,,,,,,,,,,,");
        } else {
            DC_PRINTF(D, "%u,", data->gearbox_host_state);
            DC_PRINTF(D, "%u,", data->gearbox_line_state);
            DC_PRINTF(D, "%u,", data->gearbox_healing_en);
            DC_PRINTF(D, "%u,", data->gearbox_heal_cnt);
            DC_PRINTF(D, "%u,%u,%u,", data->gb_info.fifo.tx_min, data->gb_info.fifo.tx_cur, data->gb_info.fifo.tx_max);
            DC_PRINTF(D, "%u,%u,%u,", data->gb_info.fifo.rx_min, data->gb_info.fifo.rx_cur, data->gb_info.fifo.rx_max);
            DC_PRINTF(D, "%u,%u,", data->gb_info.cfg_pdiff_rx, data->gb_info.cfg_pdiff_tx);
            DC_PRINTF(D, "%u,%u,", data->gb_info.cfg_pdiff_hw_rx_min, data->gb_info.cfg_pdiff_hw_rx_max);
            DC_PRINTF(D, "%u,%u,", data->gb_info.cfg_pdiff_hw_tx_min, data->gb_info.cfg_pdiff_hw_tx_max);
            DC_PRINTF(D, "%u,%u,", data->gb_info.cfg_pdiff_dev_rx, data->gb_info.cfg_pdiff_dev_tx);
            DC_PRINTF(D, "%u,%u,%u", data->gb_info.ncw_avg_rx, data->gb_info.ncw_avg_tx, data->gb_info.bip_sum_rx);
        }
    }

    if (argt->rsfec) {
        DC_START_COMMA(D, add_start_comma);
        DC_PRINTF(D, ",");  // add comma for data-group column spacer
        if (!data->port_exists || data->port_connection_mode != CR_PORT_GEARBOX) {
            for (size_t i = 0; i < 28; i++) {
                DC_PRINTF(D, ",");
            }
        } else {
            DC_PRINTF(D, "%u,%u,", data->rsfec_fec_align, data->rsfec_pcs_align);
            DC_PRINTF(D, "%u,%u,%u,", data->rsfec_fifo.rx_min, data->rsfec_fifo.rx_cur, data->rsfec_fifo.rx_max);
            DC_PRINTF(D, "%u,%u,%u,", data->rsfec_fifo.tx_min, data->rsfec_fifo.tx_cur, data->rsfec_fifo.tx_max);
            DC_PRINTF(D, "%u,%u,", data->rsfec_in_buf_ptr, data->rsfec_out_buf_ptr);
            DC_PRINTF(D, "%" PRIu64 ",%u,%" PRIu64 ",%f", data->rsfec_corr_cw, data->rsfec_uncorr_cw,
                      data->rsfec_total_cw, data->rsfec_corr_err_rate);
            for (size_t i = 0; i < 16; i++) {
                DC_PRINTF(D, ",%" PRIu64, data->rsfec_hist[i]);
            }
        }
    }

    if (argt->exit_code) {
        DC_START_COMMA(D, add_start_comma);
        // spacer comma added by data print
        if (data->lane_mode == CR_LMODE_OFF) {
            for (size_t i = 0; i < 16; i++) {
                DC_PRINTF(D, ",");
            }
        } else {
            for (size_t i = 0; i < 16; i++) {
                DC_PRINTF(D, ",0x%X", data->exit_code[i]);
            }
        }
    }

    DC_PRINTF(D, "\n");
    return 0;
}

CredoError_t bh_lane_data_capture(CredoSlice_t* slice, const CredoDataCaptureArg_t argv[], size_t argc,
                                  DataCaptureState_t* D) {
    LaneDataArgTable_t argt = lane_data_default_args;
    DataCaptureArgMap_t map[] = {
        {"header", &argt.header},
        {"metadata", &argt.metadata},
        {"lane", &argt.lane},
        {"serdes_control", &argt.serdes_control},
        {"serdes_param", &argt.serdes_param},
        {"prbs", &argt.prbs},
        {"exit_code", &argt.exit_code},
        {"port_info", &argt.port_info},
        {"isi", &argt.isi},

        {"fecana", &argt.fecana},
        {"fecana_histgroup", &argt.fecana_histgroup},
        {"fecana_hist", &argt.fecana_hist},
        {"fecana_hist_duration", &argt.fecana_hist_duration},
        {"retimer", &argt.retimer},
        {"bitmux", &argt.bitmux},
        {"gearbox", &argt.gearbox},
        {"rsfec", &argt.rsfec},
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
        argt.fecana_hist = 1;
        argt.isi = 1;
        // restore user selected lane
        argt.lane = lane;
        argt.header = header;
    }

    // print header as its own thing
    if (argt.header) {
        bh_lane_data_write_header(D, &argt);
        return CR_OK;
    }

    // capture data
    LaneData_t data = {{0}};
    ERR_PROPS(bh_lane_data_capture_data(slice, &argt, &data));
    // write data
    int err = bh_lane_data_write(D, &argt, &data);
    if (err != 0) {
        LOGS_ERROR("Failed to write lane_data (%d)", err);
        return CR_FAIL;
    }
    return CR_OK;
}

const DataCaptureCommand_t bh_data_capture_list[] = {
    {
        .name = "lane_data",
        .description = "Capture all of the lane data into a CSV format",
        .params =
            {
                {"header", 0, "Print header in the output"},
                {"lane", 0, "Lane to capture data"},
                {"metadata", 1, "Metadata information about the slice, lane, port, and environment"},
                {"serdes_control", 1, ""},
                {"serdes_param", 1, ""},
                {"prbs", 1, ""},
                {"port_info", 1, ""},
                {"exit_code", 1, ""},
                {"isi", 0, ""},

                {"fecana", 1, ""},
                {"fecana_histgroup", 1, "Show current fecana histogram group."},
                {"fecana_hist", 0, "Show full histogram instead of current hist group."},
                {"fecana_hist_duration", 250, ""},
                {"retimer", 1, ""},
                {"bitmux", 1, ""},
                {"gearbox", 1, ""},
                {"rsfec", 1, ""},
                {"debug_mode", 0, "If enabled, this will force domains to a preset for tons of debug information."},

                {NULL},
            },
        .runner = bh_lane_data_capture,
    },
    {NULL},
};

CredoError_t bh_data_capture(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[], size_t argc,
                             CredoDataWriter_t writer, void* ud) {
    const DataCaptureCommand_t* def = datacap_find_command(bh_data_capture_list, command);
    if (def == NULL) {
        LOGS_ERROR("Invalid command: '%s'", command);
        return CR_INVALID_ARGS;
    }
    DataCaptureState_t D = {.ud = ud, .writer = writer};
    return def->runner(slice, argv, argc, &D);
}

// utility for documentation generation
void* bh_data_get_commands(CredoSlice_t* slice, size_t* command_count) {
    *command_count = COUNT_OF(bh_data_capture_list) - 1;  // remove NULL from count
    return (void*)bh_data_capture_list;
}
