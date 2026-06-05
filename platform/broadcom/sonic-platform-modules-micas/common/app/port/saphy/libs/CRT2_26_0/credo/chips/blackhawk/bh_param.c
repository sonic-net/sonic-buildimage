#include "bh_functions.h"
#include "blackhawk.h"

#include "common/common_misc.h"
#include "common/params.h"
#include "condor_lp/condor_lp_serdes.h"

#include "utility.h"

static CredoError_t bh_sp_get_top_pll_cap(CredoSlice_t* slice, int index, int* val) {
    return bh_get_top_pll_cap(slice, (unsigned*)val);
}

static CredoError_t bh_sp_get_fec_adv_rd(CredoSlice_t* slice, int index, int* val) {
    bool en = false;
    ERR_PROPS(bh_fw_get_top_option(slice, TOP_OPTION_FEC_ADV_INFO, &en));
    *val = en ? 1 : 0;
    return CR_OK;
}

static CredoError_t bh_sp_set_fec_adv_rd(CredoSlice_t* slice, int index, int val) {
    ERR_PROPS(bh_fw_set_top_option(slice, TOP_OPTION_FEC_ADV_INFO, val));
    return CR_OK;
}

const ParamHandler_t param_slice_list[] = {
    PARAM_TOP_INT(SP_FW_CMD_TIMEOUT, common_get_fw_cmd_timeout, common_set_fw_cmd_timeout),
    PARAM_TOP_INT(SP_FW_UNLOAD_TIMEOUT, common_option_get_fw_unload_timeout, common_option_set_fw_unload_timeout),
    PARAM_TOP_INT(SP_TOP_CAL_TIMEOUT, common_get_top_cal_timeout, common_set_top_cal_timeout),
    PARAM_TOP_INT(SP_TOP_PLL_CAP, bh_sp_get_top_pll_cap, NULL),
    PARAM_TOP_INT(SP_FW_FEC_ADV_RD, bh_sp_get_fec_adv_rd, bh_sp_set_fec_adv_rd),
};
const int param_slice_count = COUNT_OF(param_slice_list);

static CredoError_t bh_sp_get_rx_skef_enable(CredoSlice_t* slice, int lane, int* enable) {
    int degen, addcap, gain;
    return hal_get_rx_skef(slice, lane, enable, &degen, &addcap, &gain);
}
static CredoError_t bh_sp_set_rx_skef_enable(CredoSlice_t* slice, int lane, int enable) {
    int old_enable, degen, addcap, gain;
    ERR_PROPS(hal_get_rx_skef(slice, lane, &old_enable, &degen, &addcap, &gain));
    return hal_set_rx_skef(slice, lane, enable, degen, addcap, gain);
}
static CredoError_t bh_sp_get_rx_skef_degen(CredoSlice_t* slice, int lane, int* degen) {
    int enable, addcap, gain;
    return hal_get_rx_skef(slice, lane, &enable, degen, &addcap, &gain);
}
static CredoError_t bh_sp_set_rx_skef_degen(CredoSlice_t* slice, int lane, int degen) {
    int enable, old_degen, addcap, gain;
    ERR_PROPS(hal_get_rx_skef(slice, lane, &enable, &old_degen, &addcap, &gain));
    return hal_set_rx_skef(slice, lane, enable, degen, addcap, gain);
}
static CredoError_t bh_sp_get_rx_skef_addcap(CredoSlice_t* slice, int lane, int* addcap) {
    int enable, degen, gain;
    return hal_get_rx_skef(slice, lane, &enable, &degen, addcap, &gain);
}
static CredoError_t bh_sp_set_rx_skef_addcap(CredoSlice_t* slice, int lane, int addcap) {
    int enable, degen, old_addcap, gain;
    ERR_PROPS(hal_get_rx_skef(slice, lane, &enable, &degen, &old_addcap, &gain));
    return hal_set_rx_skef(slice, lane, enable, degen, addcap, gain);
}
static CredoError_t bh_sp_get_rx_skef_gain(CredoSlice_t* slice, int lane, int* gain) {
    int enable, degen, addcap;
    return hal_get_rx_skef(slice, lane, &enable, &degen, &addcap, gain);
}
static CredoError_t bh_sp_set_rx_skef_gain(CredoSlice_t* slice, int lane, int gain) {
    int enable, degen, addcap, old_gain;
    ERR_PROPS(hal_get_rx_skef(slice, lane, &enable, &degen, &addcap, &old_gain));
    return hal_set_rx_skef(slice, lane, enable, degen, addcap, gain);
}

static CredoError_t bh_sp_get_rx_atten_passive(CredoSlice_t* slice, int lane, int* passive) {
    int gain, termtune;
    return hal_get_rx_attenuator(slice, lane, passive, &gain, &termtune);
}
static CredoError_t bh_sp_set_rx_atten_passive(CredoSlice_t* slice, int lane, int passive) {
    int old_passive, gain, termtune;
    ERR_PROPS(hal_get_rx_attenuator(slice, lane, &old_passive, &gain, &termtune));
    return hal_set_rx_attenuator(slice, lane, passive, gain, termtune);
}
static CredoError_t bh_sp_get_rx_atten_gain(CredoSlice_t* slice, int lane, int* gain) {
    int passive, termtune;
    return hal_get_rx_attenuator(slice, lane, &passive, gain, &termtune);
}
static CredoError_t bh_sp_set_rx_atten_gain(CredoSlice_t* slice, int lane, int gain) {
    int passive, old_gain, termtune;
    ERR_PROPS(hal_get_rx_attenuator(slice, lane, &passive, &old_gain, &termtune));
    return hal_set_rx_attenuator(slice, lane, passive, gain, termtune);
}
static CredoError_t bh_sp_get_rx_atten_termtune(CredoSlice_t* slice, int lane, int* termtune) {
    int passive, gain;
    return hal_get_rx_attenuator(slice, lane, &passive, &gain, termtune);
}
static CredoError_t bh_sp_set_rx_atten_termtune(CredoSlice_t* slice, int lane, int termtune) {
    int passive, gain, old_termtune;
    ERR_PROPS(hal_get_rx_attenuator(slice, lane, &passive, &gain, &old_termtune));
    return hal_set_rx_attenuator(slice, lane, passive, gain, termtune);
}

static CredoError_t bh_sp_get_rx_ffe_weighting_table(CredoSlice_t* slice, int lane, double vals[]) {
    double raw_data[5][9] = {{0}};
    double* data_row[5] = {raw_data[0], raw_data[1], raw_data[2], raw_data[3], raw_data[4]};
    double** data = data_row;
    ERR_PROPS(hal_fw_get_rx_ffe_weighting_table(slice, lane, data));

    for (int i = 0; i < 45; i++) {
        vals[i] = data[i / 9][i % 9];
    }
    return CR_OK;
}

static CredoError_t bh_sp_get_eye_height(CredoSlice_t* slice, int lane, int eyes[3]) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    if (mode == CR_LMODE_PAM4) {
        return hal_fw_get_eye(slice, lane, eyes);
    } else {
        return hal_get_eye(slice, lane, eyes);
    }
}

static CredoError_t bh_sp_get_eye_height_count(CredoSlice_t* slice, int lane, int* count) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    if (mode == CR_LMODE_PAM4) {
        *count = 3;
        return CR_OK;
    } else if (mode == CR_LMODE_NRZ) {
        *count = 1;
        return CR_OK;
    }
    // not sure so just default to max size
    *count = 3;
    return CR_OK;
}

#define SP_PORT_BITMUX_HOST_STATE  "bitmux_host_state", "state of bitmux on the host side", CR_PARAM_TYPE_STATUS
#define SP_PORT_BITMUX_LINE_STATE  "bitmux_line_state", "state of bitmux on the line side", CR_PARAM_TYPE_STATUS
#define SP_PORT_RETIMER_HOST_STATE "retimer_host_state", "state of retimer on the host side", CR_PARAM_TYPE_STATUS
#define SP_PORT_RETIMER_LINE_STATE "retimer_line_state", "state of retimer on the line side", CR_PARAM_TYPE_STATUS
#define SP_PORT_GEARBOX_HOST_STATE "gearbox_host_state", "state of gearbox on the host side", CR_PARAM_TYPE_STATUS
#define SP_PORT_GEARBOX_LINE_STATE "gearbox_line_state", "state of gearbox on the line side", CR_PARAM_TYPE_STATUS

static CredoError_t bh_port_get_bitmux_host_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    BhSlice_t* bhslice = (BhSlice_t*)slice;
    BhPortInfo_t* port_info = &bhslice->port_info[port];
    CredoPortConfig_t* port_config = &port_info->port_config;
    ERR_PROPS(bh_fw_get_bitmux_state(slice, port_config->host_start_lane, state));
    return CR_OK;
}

static CredoError_t bh_port_get_bitmux_line_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    BhSlice_t* bhslice = (BhSlice_t*)slice;
    BhPortInfo_t* port_info = &bhslice->port_info[port];
    CredoPortConfig_t* port_config = &port_info->port_config;
    ERR_PROPS(bh_fw_get_bitmux_state(slice, port_config->line_start_lane, state));
    return CR_OK;
}

static CredoError_t bh_port_get_retimer_host_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    BhSlice_t* bhslice = (BhSlice_t*)slice;
    BhPortInfo_t* port_info = &bhslice->port_info[port];
    CredoPortConfig_t* port_config = &port_info->port_config;
    ERR_PROPS(bh_fw_get_retimer_state(slice, port_config->host_start_lane, state));
    return CR_OK;
}

static CredoError_t bh_port_get_retimer_line_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    BhSlice_t* bhslice = (BhSlice_t*)slice;
    BhPortInfo_t* port_info = &bhslice->port_info[port];
    CredoPortConfig_t* port_config = &port_info->port_config;
    ERR_PROPS(bh_fw_get_retimer_state(slice, port_config->line_start_lane, state));
    return CR_OK;
}

static CredoError_t bh_port_get_gearbox_host_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    BhSlice_t* bhslice = (BhSlice_t*)slice;
    BhPortInfo_t* port_info = &bhslice->port_info[port];
    CredoPortConfig_t* port_config = &port_info->port_config;
    ERR_PROPS(bh_fw_get_gearbox_state(slice, port_config->host_start_lane, state));
    return CR_OK;
}

static CredoError_t bh_port_get_gearbox_line_state(CredoSlice_t* slice, uint32_t port, unsigned* state) {
    BhSlice_t* bhslice = (BhSlice_t*)slice;
    BhPortInfo_t* port_info = &bhslice->port_info[port];
    CredoPortConfig_t* port_config = &port_info->port_config;
    ERR_PROPS(bh_fw_get_gearbox_state(slice, port_config->line_start_lane, state));
    return CR_OK;
}

static CredoError_t bh_sp_get_tx_taps_raw(CredoSlice_t* slice, int lane, int vals[]) {
    ERR_PROPS(hal_get_tx_taps_extended(slice, lane, vals));
    ERR_PROPS(condor_lp_get_tx_taps(slice, lane, vals));
    return CR_OK;
}

static CredoError_t bh_sp_set_tx_taps_raw(CredoSlice_t* slice, int lane, int vals[]) {
    ERR_PROPS(hal_set_tx_taps_extended(slice, lane, vals));
    ERR_PROPS(condor_lp_set_tx_taps(slice, lane, vals));
    return CR_OK;
}

// make sure to use SP_ name for the parameter, and add it if it doesnt exist. This helps to keep the names
// standardized across chips.
const ParamHandler_t param_serdes_list[] = {

    PARAM_LANE_INTLIST(SP_TX_TAPS, 7, hal_get_tx_taps, hal_set_tx_taps),
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
    PARAM_LANE_INT(SP_RX_PPM, hal_get_rx_ppm, NULL),
    PARAM_LANE_INT(SP_RX_DAC, hal_get_rx_dac, hal_set_rx_dac),
    PARAM_LANE_INTLIST(SP_RX_FFE_TAPS, 8, hal_get_ffe_taps, hal_set_ffe_taps),
    PARAM_LANE_INTLIST(SP_RX_FFE_TAPS_FINE, 8, hal_get_ffe_taps_fine, hal_set_ffe_taps_fine),
    PARAM_LANE_INTLIST(SP_RX_FFE_NBIAS, 8, hal_fw_get_rx_ffe_nbias, NULL),
    PARAM_LANE_DBLLIST(SP_RX_FFE_KACCU, 5, hal_fw_get_rx_ffe_kaccu, NULL),
    PARAM_LANE_INTLIST(SP_RX_FFE_FLIP_COUNTER, 5, hal_fw_get_rx_ffe_flip_counter, NULL),
    PARAM_LANE_DBLLIST(SP_RX_FFE_WEIGHTING_TABLE, 45, bh_sp_get_rx_ffe_weighting_table, NULL),
    PARAM_LANE_INT(SP_RX_F1OVER3, hal_get_f1over3, hal_set_f1over3),
    PARAM_LANE_INTLIST(SP_RX_AGCGAIN, 2, hal_get_agcgain, hal_set_agcgain),
    PARAM_LANE_INTLIST(SP_RX_CTLE, 2, hal_get_ctle, hal_set_ctle),
    PARAM_LANE_INT(SP_RX_TIA1_BIAS, condor_lp_get_rx_tia1_bias, condor_lp_set_rx_tia1_bias),
    PARAM_LANE_INT(SP_RX_DELTA, hal_get_delta_phase, hal_set_delta_phase),
    PARAM_LANE_INT(SP_RX_EDGE, hal_get_edge, hal_set_edge),
    PARAM_LANE_INTLIST(SP_RX_THS, 12, bh_get_rx_ths, NULL),
    PARAM_LANE_DBLLIST(SP_RX_DFE, 3, hal_get_dfe, NULL),
    PARAM_LANE_VAR_INTLIST_FLG(SP_RX_EYE_HEIGHT, 3, bh_sp_get_eye_height, NULL, bh_sp_get_eye_height_count, NULL, 0x0),
    PARAM_LANE_INT(SP_RX_READY, hal_get_lane_ready, NULL),
    PARAM_LANE_INT(SP_RX_SIGNAL_DETECT, hal_get_signal_detect, NULL),
    PARAM_LANE_INT(SP_RX_SKEF_EN, bh_sp_get_rx_skef_enable, bh_sp_set_rx_skef_enable),
    PARAM_LANE_INT(SP_RX_SKEF_DEGEN, bh_sp_get_rx_skef_degen, bh_sp_set_rx_skef_degen),
    PARAM_LANE_INT(SP_RX_SKEF_ADDCAP, bh_sp_get_rx_skef_addcap, bh_sp_set_rx_skef_addcap),
    PARAM_LANE_INT(SP_RX_SKEF_GAIN, bh_sp_get_rx_skef_gain, bh_sp_set_rx_skef_gain),
    PARAM_LANE_INT(SP_RX_ATTEN_PASSIVE, bh_sp_get_rx_atten_passive, bh_sp_set_rx_atten_passive),
    PARAM_LANE_INT(SP_RX_ATTEN_GAIN, bh_sp_get_rx_atten_gain, bh_sp_set_rx_atten_gain),
    PARAM_LANE_INT(SP_RX_ATTEN_TERMTUNE, bh_sp_get_rx_atten_termtune, bh_sp_set_rx_atten_termtune),
    PARAM_LANE_INT(SP_RX_ADAPT, hal_fw_get_adapt_count, NULL),
    PARAM_LANE_INT(SP_RX_READAPT, hal_fw_get_readapt_count, NULL),
    PARAM_LANE_INT(SP_RX_LINKLOST, hal_fw_get_link_lost_count, NULL),
    PARAM_LANE_DBL(SP_RX_CHANNEL_EST, hal_fw_get_channel_estimate, NULL),
    PARAM_LANE_INT(SP_RX_CHANNEL_OF, hal_fw_get_of, NULL),
    PARAM_LANE_INT(SP_RX_CHANNEL_HF, hal_fw_get_hf, NULL),
    PARAM_LANE_INT(SP_RX_KP, condor_lp_get_rx_kp, condor_lp_set_rx_kp),
    PARAM_LANE_INT(SP_RX_KF, condor_lp_get_rx_kf, condor_lp_set_rx_kf),
    PARAM_LANE_INTLIST(SP_TX_TAPS_RAW, 24, bh_sp_get_tx_taps_raw, bh_sp_set_tx_taps_raw),
};
const int param_serdes_count = COUNT_OF(param_serdes_list);

const ParamHandler_t param_port_list[] = {
    PARAM_PORT_INTLIST(SP_PORT_LINK, 2, bh_port_link_state, NULL),
    PARAM_PORT_INT(SP_PORT_LINK_HOST, bh_port_link_state_host, NULL),
    PARAM_PORT_INT(SP_PORT_LINK_LINE, bh_port_link_state_line, NULL),
    PARAM_LANE_INT(SP_PORT_BITMUX_HOST_STATE, bh_port_get_bitmux_host_state, NULL),
    PARAM_LANE_INT(SP_PORT_BITMUX_LINE_STATE, bh_port_get_bitmux_line_state, NULL),
    PARAM_LANE_INT(SP_PORT_RETIMER_HOST_STATE, bh_port_get_retimer_host_state, NULL),
    PARAM_LANE_INT(SP_PORT_RETIMER_LINE_STATE, bh_port_get_retimer_line_state, NULL),
    PARAM_LANE_INT(SP_PORT_GEARBOX_HOST_STATE, bh_port_get_gearbox_host_state, NULL),
    PARAM_LANE_INT(SP_PORT_GEARBOX_LINE_STATE, bh_port_get_gearbox_line_state, NULL),
};
const int param_port_count = COUNT_OF(param_port_list);

static CredoError_t bh_lane_param_get_state(CredoSlice_t* slice, int lane, int* state) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, PAM4_DEBUG, PAM4_DEBUG_RX_STATE, (unsigned*)state));
    } else if (mode == CR_LMODE_NRZ) {
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, NRZ_DEBUG, NRZ_DEBUG_RX_STATE, (unsigned*)state));
    } else {
        // not in any mode, let state be 0
        *state = 0;
    }
    return CR_OK;
}

const ParamHandler_t param_lane_list[] = {
    PARAM_LANE_INT(SP_LANE_STATE, bh_lane_param_get_state, NULL),
};
const int param_lane_count = COUNT_OF(param_lane_list);
