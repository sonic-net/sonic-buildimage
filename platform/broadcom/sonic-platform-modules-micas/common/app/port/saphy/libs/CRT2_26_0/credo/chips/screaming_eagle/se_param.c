#include "screaming_eagle.h"
#include "se_functions.h"

#include "common/common_misc.h"
#include "common/params.h"
#include "swift/swift_serdes.h"

#include "utility.h"

#define SP_SLICE_SYS_TIME "sys_time", "system monotonic time in seconds (milisecond resolution)", CR_PARAM_TYPE_STATUS
#define SP_SLICE_START_TIME \
    "start_time", "start of firmware based in seconds (milisecond resolution) as wall time", CR_PARAM_TYPE_STATUS

static CredoError_t se_system_time(CredoSlice_t* slice, int index, double* timedelta) {
    return se_time_system(slice, timedelta);
}

static CredoError_t se_start_time(CredoSlice_t* slice, int index, double* start_time) {
    return se_time_start(slice, start_time);
}

const ParamHandler_t param_slice_list[] = {
    PARAM_TOP_INT(SP_FW_CMD_TIMEOUT, common_get_fw_cmd_timeout, common_set_fw_cmd_timeout),
    PARAM_TOP_INT(SP_TOP_CAL_TIMEOUT, common_get_top_cal_timeout, common_set_top_cal_timeout),
    PARAM_TOP_DBL(SP_SLICE_SYS_TIME, se_system_time, NULL),
    PARAM_TOP_DBL(SP_SLICE_START_TIME, se_start_time, NULL),
};
const int param_slice_count = COUNT_OF(param_slice_list);

#define SP_RX_TH_KTHETA          "rx_thdly_ktheta", "", CR_PARAM_TYPE_PARAM
#define SP_RX_THDLY              "rx_thdly", "", CR_PARAM_TYPE_PARAM
#define SP_RX_THDLY_ACC_IN_SEL   "rx_thdly_sel", "", CR_PARAM_TYPE_PARAM
#define SP_RX_THDLY_UD_PH_EN     "rx_thldy_en", "", CR_PARAM_TYPE_PARAM
#define SP_RX_PHASE_FAST_ROTATE  "rx_phase_fast_rotate", "", CR_PARAM_TYPE_PARAM
#define SP_RX_TED_SLOPE_DECISION "rx_ted_slope_decision", "", CR_PARAM_TYPE_PARAM
#define SP_RX_DELAYLOOP_FREEZE   "rx_delayloop_freeze", "", CR_PARAM_TYPE_PARAM
#define SP_RX_DC_CMN             "rx_dc_cmn", "", CR_PARAM_TYPE_PARAM
#define SP_RX_SNR                "rx_snr", "Signal to Noise ratio measured in dB", CR_PARAM_TYPE_PARAM

static CredoError_t se_fw_get_event_time(CredoSlice_t* slice, int lane, unsigned index, double* sec) {
    unsigned msb = 0, lsb = 0;
    ERR_PROPS(hal_fw_debug_cmd_ex(slice, lane, SE_INFO, index, &msb, &lsb));
    *sec = ((msb << 16) | lsb) * FW_TIME_UNIT;
    return CR_OK;
}

static CredoError_t se_get_up_time(CredoSlice_t* slice, int lane, double* up) {
    ERR_PROPS(se_fw_get_event_time(slice, lane, SE_INFO_LANE_UP_TIME, up));
    return CR_OK;
}

static CredoError_t se_get_down_time(CredoSlice_t* slice, int lane, double* down) {
    ERR_PROPS(se_fw_get_event_time(slice, lane, SE_INFO_LANE_DOWN_TIME, down));
    return CR_OK;
}

static CredoError_t se_get_snr(CredoSlice_t* slice, int lane, double* snr) {
    unsigned snr_raw;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_CMIS_SNR, &snr_raw));
    *snr = (double)snr_raw / 256;
    return CR_OK;
}

static CredoError_t se_get_flexspeed_en(CredoSlice_t* slice, int lane, int* en) {
    unsigned flexspeed_lane_map;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_FLEXSPEED_EN_RX, &flexspeed_lane_map));
    *en = ((flexspeed_lane_map & (1 << lane)) != 0);
    return CR_OK;
}

const ParamHandler_t param_serdes_list[] = {
    PARAM_LANE_INT(SP_FW_PRESET_IDX, se_fw_get_preset_index, NULL),
    PARAM_LANE_INT(SP_TX_POL, hal_get_tx_polarity, hal_set_tx_polarity),
    PARAM_LANE_INT(SP_RX_POL, hal_get_rx_polarity, hal_set_rx_polarity),
    PARAM_LANE_INT(SP_RX_INPUT_COUPLING, hal_get_rx_input_mode, hal_set_rx_input_mode),
    PARAM_LANE_INT(SP_TX_GRAYCODE, hal_get_tx_gray_code, hal_set_tx_gray_code),
    PARAM_LANE_INT(SP_RX_GRAYCODE, hal_get_rx_gray_code, hal_set_rx_gray_code),
    PARAM_LANE_INT(SP_TX_PRECODER, hal_get_tx_precoder, hal_set_tx_precoder),
    PARAM_LANE_INT(SP_RX_PRECODER, hal_get_rx_precoder, hal_set_rx_precoder),
    PARAM_LANE_INT(SP_TX_MSBLSB, hal_get_tx_msb, hal_set_tx_msb),
    PARAM_LANE_INT(SP_RX_MSBLSB, hal_get_rx_msb, hal_set_rx_msb),
    PARAM_LANE_INT(SP_TX_PLL_CAP, hal_get_tx_cap, hal_set_tx_cap),
    PARAM_LANE_INT(SP_RX_PLL_CAP, hal_get_rx_cap, hal_set_rx_cap),
    PARAM_LANE_INT(SP_RX_FFE_CM1, se_get_rx_ffe_cm1, se_set_rx_ffe_cm1),
    PARAM_LANE_INT(SP_LMS_CLOCK, swift_get_lms_clk, swift_set_lms_clk),
    PARAM_LANE_INT(SP_RX_PPM, hal_get_rx_ppm, swift_set_rx_ppm),
    PARAM_LANE_INTLIST(SP_RX_SKEF, 3, se_get_skef, se_set_skef),
    PARAM_LANE_INT(SP_RX_SKEF_EN, swift_get_rx_skef_en, swift_set_rx_skef_en),
    PARAM_LANE_INT(SP_RX_SKEF_DEGEN, swift_get_rx_skef_degen, swift_set_rx_skef_degen),
    PARAM_LANE_INT(SP_RX_SKEF_ADDCAP, swift_get_rx_skef_cap, swift_set_rx_skef_cap),
    PARAM_LANE_INTLIST(SP_RX_ENVELOPE, 2, se_get_rx_envelope, NULL),
    PARAM_LANE_INTLIST(SP_RX_FFE_TAPS_ALL, DSP_RX_FFE_COUNT* PHASE_NUM, se_get_rx_ffe_all, NULL),
    PARAM_LANE_INTLIST(SP_RX_FFE_TAPS, DSP_RX_FFE_COUNT, se_get_rx_ffe, NULL),
    PARAM_LANE_VAR_INTLIST_FLG(SP_RX_EYE, 14, se_fw_get_eye_phase0, NULL, se_get_eye_count, NULL, 0x0),
    PARAM_LANE_VAR_INTLIST_FLG(SP_RX_EYE_ALL, 14 * PHASE_NUM, se_fw_get_eye_all, NULL, se_get_eye_all_count, NULL, 0x0),
    PARAM_LANE_VAR_INTLIST_FLG(SP_RX_EYE_HEIGHT, 3, se_fw_get_eye, NULL, se_get_eye_avg_count, NULL, 0x0),
    PARAM_LANE_INTLIST(SP_RX_CTLE, 2, swift_get_rx_ctle, swift_set_rx_ctle),
    PARAM_LANE_INTLIST(SP_RX_CTLE_CS, 2, swift_get_rx_ctle_cs, swift_set_rx_ctle_cs),
    PARAM_LANE_INTLIST(SP_RX_CTLE_IND, 2, swift_get_rx_ind, swift_set_rx_ind),
    PARAM_LANE_INT(SP_RX_ATTEN_GAIN, swift_get_rx_agc_attn, swift_set_rx_agc_attn),
    PARAM_LANE_INT(SP_RX_DFE_NL_MODE, swift_get_rx_dfe_nonlinear_mode, swift_set_rx_dfe_nonlinear_mode),
    PARAM_LANE_INT(SP_RX_DFE_F0, se_get_dfe_f0, se_set_dfe_f0),
    PARAM_LANE_INT(SP_RX_DFE_F1, se_get_dfe_f1, se_set_dfe_f1),
    PARAM_LANE_INTLIST(SP_RX_FLT_SEL, DSP_FLT_COUNT, se_get_rx_flt_sel, se_set_rx_flt_sel),
    PARAM_LANE_INTLIST(SP_RX_FLT_LOC, DSP_FLT_COUNT, se_get_rx_flt_loc, NULL),
    PARAM_LANE_INTLIST(SP_RX_DFE, DSP_DFE_COUNT, se_get_dfe, se_set_dfe),
    PARAM_LANE_INTLIST(SP_RX_DC_SAR, DSP_SAR_COUNT, se_get_dc_sar, NULL),
    PARAM_LANE_INTLIST(SP_RX_GAIN_SAR, DSP_SAR_COUNT, se_get_gain_sar, NULL),
    PARAM_LANE_INT(SP_RX_VGA, se_get_rx_vga, se_set_rx_vga),
    PARAM_LANE_INT(SP_RX_DTL_PHASE0, swift_get_rx_dtl_phase0, swift_set_rx_dtl_phase0),
    PARAM_LANE_INTLIST(SP_RX_AGCGAIN, 4, swift_get_rx_agcgain, se_set_agcgain),
    PARAM_LANE_DBLLIST(SP_RX_ISI, DSP_ISI_COUNT, se_fw_get_isi, NULL),
    PARAM_LANE_DBLLIST(SP_RX_ISI_ALL, DSP_ISI_COUNT* PHASE_NUM, se_fw_get_isi_all, NULL),
    PARAM_LANE_DBLLIST(SP_RX_CHANNEL_EST_PSD, 2, se_fw_get_channel_est_psd, NULL),
    PARAM_LANE_DBL(SP_RX_CHANNEL_EST, hal_fw_get_channel_estimate, NULL),
    PARAM_LANE_INT(SP_RX_KF, swift_get_rx_kf, swift_set_rx_kf),
    PARAM_LANE_INT(SP_RX_KP, swift_get_rx_kp, swift_set_rx_kp),
    PARAM_LANE_INTLIST(SP_TX_TAPS, 7, hal_get_tx_taps, hal_set_tx_taps),
    PARAM_LANE_INT(SP_RX_TH_KTHETA, swift_get_rx_th_ktheta, NULL),
    PARAM_LANE_INTLIST(SP_RX_THDLY, PHASE_NUM, swift_get_rx_thdly, NULL),
    PARAM_LANE_INT(SP_RX_THDLY_ACC_IN_SEL, swift_get_rx_thdly_acc_in_sel, NULL),
    PARAM_LANE_INT(SP_RX_THDLY_UD_PH_EN, swift_get_rx_th_ud_ph_enable, NULL),
    PARAM_LANE_INT(SP_RX_PHASE_FAST_ROTATE, swift_get_rx_phase_fast_rotate, NULL),
    PARAM_LANE_INT(SP_RX_TED_SLOPE_DECISION, swift_get_rx_ted_slope_decision, NULL),
    PARAM_LANE_INT(SP_RX_DELAYLOOP_FREEZE, swift_get_rx_delayloop_freeze, NULL),
    PARAM_LANE_INT(SP_RX_DC_CMN, swift_get_rx_dc_cmn, NULL),
    PARAM_LANE_DBL(SP_UP_TIME, se_get_up_time, NULL),
    PARAM_LANE_DBL(SP_DOWN_TIME, se_get_down_time, NULL),
    PARAM_LANE_DBL(SP_RX_SNR, se_get_snr, NULL),
    PARAM_LANE_INT(SP_FLEXSPEEN_EN, se_get_flexspeed_en, NULL),
};
const int param_serdes_count = COUNT_OF(param_serdes_list);

#define SP_PORT_BITMUX_HOST_STATE  "bitmux_host_state", "state of bitmux on the host side", CR_PARAM_TYPE_STATUS
#define SP_PORT_BITMUX_LINE_STATE  "bitmux_line_state", "state of bitmux on the line side", CR_PARAM_TYPE_STATUS
#define SP_PORT_RETIMER_HOST_STATE "retimer_host_state", "state of retimer on the host side", CR_PARAM_TYPE_STATUS
#define SP_PORT_RETIMER_LINE_STATE "retimer_line_state", "state of retimer on the line side", CR_PARAM_TYPE_STATUS
#define SP_PORT_SWITCHOVER_PRIMED  "switchover_primed", "indicate if each mux group is primed", CR_PARAM_TYPE_STATUS

static CredoError_t se_port_get_bitmux_host_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    SePortInfo_t* port_info = &seslice->port_info[port];
    ERR_PROPS(se_fw_get_bitmux_state(slice, port_info->host_main_lane, state));
    return CR_OK;
}

static CredoError_t se_port_get_bitmux_line_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    SePortInfo_t* port_info = &seslice->port_info[port];
    ERR_PROPS(se_fw_get_bitmux_state(slice, port_info->line_main_lane, state));
    return CR_OK;
}

static CredoError_t se_port_get_retimer_host_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    SePortInfo_t* port_info = &seslice->port_info[port];
    if (port_info->direction == CR_PORT_DIR_LINE_TO_HOST) {
        *state = 0;
    } else {
        ERR_PROPS(se_fw_get_retimer_state(slice, port_info->host_main_lane, state));
    }
    return CR_OK;
}

static CredoError_t se_port_get_retimer_line_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    SePortInfo_t* port_info = &seslice->port_info[port];
    if (port_info->direction == CR_PORT_DIR_HOST_TO_LINE) {
        *state = 0;
    } else {
        ERR_PROPS(se_fw_get_retimer_state(slice, port_info->line_main_lane, state));
    }
    return CR_OK;
}

static CredoError_t se_port_param_get_bitmux_fifo(CredoSlice_t* slice, int lane, unsigned fifo[2]) {
    ERR_PROPS(readRegLane(slice, lane, REG_ADR_DIFF_0, &fifo[0]));
    ERR_PROPS(readRegLane(slice, lane, REG_ADR_DIFF_1, &fifo[1]));
    fifo[0] = gray_bin(fifo[0]);
    fifo[1] = gray_bin(fifo[1]);
    return CR_OK;
}

static CredoError_t se_port_param_get_retimer_fifo(CredoSlice_t* slice, int lane, unsigned fifo[2]) {
    ERR_PROPS(readRegLane(slice, lane, REG_ADR_DIFF_0, &fifo[0]));
    ERR_PROPS(readRegLane(slice, lane, REG_ADR_DIFF_1, &fifo[1]));
    fifo[0] = gray_bin(fifo[0]);
    fifo[1] = gray_bin(fifo[1]);
    return CR_OK;
}

static CredoError_t se_port_param_get_switchover_primed(CredoSlice_t* slice, int port, int primed[2]) {
    unsigned rdy = 0;
    ERR_PROPS(se_fw_phy_ready(slice, &rdy));

    SeSlice_t* seslice = (SeSlice_t*)slice;
    SePortInfo_t* port_info = &seslice->port_info[port];
    CredoPortSetup_t* port_setup = &port_info->setup;

    bool is_aside_broadcast = port_setup->host_count < port_setup->line_count;
    size_t broadcast_count = MIN(port_setup->host_count, port_setup->line_count);

    int* mux_lanes = (is_aside_broadcast) ? port_setup->line_lanes : port_setup->host_lanes;

    primed[0] = 1;
    primed[1] = 1;

    for (size_t i = 0; i < broadcast_count; i++) {
        primed[0] = primed[0] && ((1 << mux_lanes[2 * i]) & rdy);
        primed[1] = primed[1] && ((1 << mux_lanes[(2 * i) + 1]) & rdy);
    }
    return CR_OK;
}

const ParamHandler_t param_port_list[] = {
    PARAM_LANE_INTLIST(SP_PORT_BITMUX_FIFO, 2, se_port_param_get_bitmux_fifo, NULL),
    PARAM_LANE_INTLIST(SP_PORT_RETIMER_FIFO, 2, se_port_param_get_retimer_fifo, NULL),
    PARAM_PORT_INT(SP_PORT_BITMUX_HOST_STATE, se_port_get_bitmux_host_state, NULL),
    PARAM_PORT_INT(SP_PORT_BITMUX_LINE_STATE, se_port_get_bitmux_line_state, NULL),
    PARAM_PORT_INT(SP_PORT_RETIMER_HOST_STATE, se_port_get_retimer_host_state, NULL),
    PARAM_PORT_INT(SP_PORT_RETIMER_LINE_STATE, se_port_get_retimer_line_state, NULL),
    PARAM_PORT_INTLIST(SP_PORT_SWITCHOVER_PRIMED, 2, se_port_param_get_switchover_primed, NULL),
};

const int param_port_count = COUNT_OF(param_port_list);
