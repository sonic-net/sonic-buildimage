#include "bald_eagle.h"
#include "be_device.h"
#include "be_functions.h"
#include "be_option.h"

#include "anlt/anlt.h"
#include "canary/canary_serdes.h"
#include "common/common_efuse.h"
#include "common/common_firmware.h"
#include "common/common_misc.h"
#include "common/common_prbs.h"
#include "common/common_reset.h"
#include "common/params.h"
#include "fec_analyzer/fec_analyzer.h"
#include "rsfec/rsfec.h"

#include "sdk.h"

/* Register hive definition */
const RegHive_t PHY[] = {
    {
        .hivename = HIVENAME_SERDES,  // SerDes
        .base_addr = {0x7000},
        .shift = 9,
        .count = 16,
    },
};

const RegHive_t Top[] = {
    {
        .hivename = HIVENAME_TOP,  // BE2Top
        .base_addr = {0x9800},
        .shift = 0,
        .count = 1,
    },
};

const RegHive_t TopPLL[] = {
    {
        .hivename = HIVENAME_TOPPLL,  // BE2TopPLL
        .base_addr = {0x9500},
        .shift = 8,
        .count = 2,
    },
};

const RegHive_t FecA[] = {
    {
        .hivename = HIVENAME_FECA,  // FecA
        .base_addr = {0x4000},
        .shift = 8,
        .count = 4,
    },
};

const RegHive_t FecB[] = {
    {
        .hivename = HIVENAME_FECB,  // FecB
        .base_addr = {0x5000},
        .shift = 8,
        .count = 4,
    },
};

const RegHive_t AutoNegIEEE[] = {
    {
        .hivename = HIVENAME_ANI,  // AnI
        .base_addr = {0xD000},
        .shift = 8,
        .count = 16,
    },
};

const RegHive_t AutoNegCustom[] = {
    {
        .hivename = HIVENAME_ANC,  // AnC
        .base_addr = {0xE000},
        .shift = 8,
        .count = 16,
    },
};

const RegHive_t FecAnalyzer[] = {
    {
        .hivename = HIVENAME_FECANA,  // FecAnalyzer
        .base_addr = {0x71C0},
        .shift = 9,
        .count = 16,
    },
};

const RegHive_t RSFec[] = {
    {
        .hivename = HIVENAME_RSFEC,  // RSFEC
        .base_addr = {0x4000, 0x4100, 0x4200, 0x4300, 0x5000, 0x5100, 0x5200, 0x5300},
        .shift = BASE_ADDRESS_EXTENDED,
        .count = 8,
    },
};

const RegHive_t TopSensor[] = {
    {
        .hivename = HIVENAME_TOPSENSOR,  // BETopSensor
        .base_addr = {0xB000},
        .shift = 0,
        .count = 1,
    },
};

const RegHive_t* reghive_array[] = {PHY,           Top,         TopPLL, FecA,      FecB, AutoNegIEEE,
                                    AutoNegCustom, FecAnalyzer, RSFec,  TopSensor, NULL};

static const HalFuncTable_t be_hal_func_table = {
    .hal_allocate_slice = be_allocate_slice,
    .hal_free_slice = be_free_slice,
    .hal_slice_init = be_slice_init,
    .hal_slice_data_init = be_slice_data_init,
    .hal_slice_get_oui = common_slice_get_oui,
    .hal_slice_get_model_number = common_slice_get_model_number,
    .hal_slice_get_revision_number = common_slice_get_revision_number,
    .hal_slice_get_vsensor = be_slice_get_vsensor,
    .hal_slice_get_sram_status = common_slice_get_sram_status,
    .hal_slice_generate_sram_error = common_slice_generate_sram_error,
    .hal_slice_load_setup = common_slice_load_setup,
    .hal_slice_save_setup = be_slice_save_setup,
    .hal_slice_get_reghive = common_slice_get_reghive,
    .hal_slice_index_reghive = common_slice_index_reghive,
    .hal_slice_get_reghive_count = common_slice_get_reghive_count,

    .hal_display_info = be_display_info,

    /* Firmware itself */
    .hal_fw_unload = common_fw_unload,
    .hal_fw_load = common_fw_load,
    .hal_reg_get_notepad = common_fw_notepad,
    .hal_fw_load_broadcast = common_fw_load_broadcast,
    .hal_fw_wait_magic_word = common_fw_wait_magic_word,
    .hal_fw_clear_top_pll_cal = common_fw_clear_top_pll_cal,
    .hal_fw_wait_top_pll_cal = common_fw_wait_top_pll_cal,
    .hal_fw_get_status = be_fw_get_status,

    /* Raw firmware commands */
    .hal_fw_cmd = common_fw_cmd,
    .hal_fw_cmd_ex = common_fw_cmd_ex,

    /* Firmware information */
    .hal_fw_magic = common_fw_magic,
    .hal_fw_ver = common_fw_ver,
    .hal_fw_hash = common_fw_hash,
    .hal_fw_crc = common_fw_crc,
    .hal_fw_date = common_fw_date,

    /* Firmware registers */
    .hal_fw_reg_rd = common_fw_reg_rd,
    .hal_fw_reg_wr = common_fw_reg_wr,
    .hal_fw_reg_rd_internal = common_fw_reg_rd_internal,
    .hal_fw_reg_wr_internal = common_fw_reg_wr_internal,

    /* Firmware serdes information/debug */
    .hal_fw_debug_cmd = common_fw_debug_cmd,
    .hal_fw_debug_cmd_ex = common_fw_debug_cmd_ex,

    /* Port configuration */
    .hal_fw_config_port = be_port_config,
    .hal_fw_teardown_port = be_port_teardown,
    .hal_fw_phy_ready = be_fw_phy_ready,
    .hal_fw_phy_lane_ready = be_fw_phy_lane_ready,
    .hal_fw_config_phy = be_fw_config_phy,
    .hal_fw_config_lane = be_fw_config_lane,
    .hal_fw_config_lane_loopback = be_fw_config_lane_loopback,
    .hal_fw_deconfig_lane = be_fw_deconfig_lane,
    .hal_fw_query_port = be_port_query,
    .hal_fw_clear_all_port = be_port_teardown_all,

    .hal_fw_get_slice_temp = common_fw_get_slice_temp,

    /* Firmware serdes param */
    .hal_fw_get_adapt_count = common_fw_get_adapt_count,
    .hal_fw_get_readapt_count = common_fw_get_readapt_count,
    .hal_fw_get_link_lost_count = common_fw_get_link_lost_count,
    .hal_fw_get_channel_estimate = common_fw_get_channel_estimate,
    .hal_fw_get_of = common_fw_get_of,
    .hal_fw_get_hf = common_fw_get_hf,
    .hal_fw_get_dfe = common_fw_get_dfe,
    .hal_fw_get_eye = common_fw_get_eye,
    .hal_fw_get_isi = common_fw_get_isi,
    .hal_fw_get_lane_speed = be_get_lane_speed,

    /* Top PLL */
    .hal_top_pll_cal = be_top_pll_cal,
    .hal_top_pll_init = be_top_pll_init,
    .hal_get_top_pll_cap = be_get_top_pll_cap,

    /* Capability */
    .hal_get_lane_count = be_get_lane_count,  // TODO: fully implement

    /* Lane operation mode */
    .hal_get_lane_mode = be_get_lane_mode,
    .hal_set_lane_mode = be_set_lane_mode,
    .hal_update_lane_mode = be_update_lane_mode,
    .hal_disable_lane = be_disable_lane,

    .hal_get_lane_config = common_get_lane_config,
    .hal_set_lane_config = common_set_lane_config,

    .hal_set_lane_loopback_mode = be_set_lane_loopback_mode,
    .hal_get_lane_loopback_mode = be_get_lane_loopback_mode,
    /* SerDes capability */
    .hal_get_rx_ffe_range = canary_get_rx_ffe_range,
    .hal_get_tx_ffe_range = canary_get_tx_ffe_range,
    .hal_get_rx_dfe_range = canary_get_rx_dfe_range,
    .hal_get_rx_isi_range = canary_get_rx_isi_range,

    /* Polarity */
    .hal_set_tx_polarity = canary_set_tx_polarity,
    .hal_set_rx_polarity = canary_set_rx_polarity,
    .hal_get_tx_polarity = canary_get_tx_polarity,
    .hal_get_rx_polarity = canary_get_rx_polarity,

    /* Input Mode */
    .hal_set_rx_input_mode = canary_set_rx_input_mode,
    .hal_get_rx_input_mode = canary_get_rx_input_mode,

    /* Gray code and precoding */
    .hal_set_tx_gray_code = canary_set_tx_gray_code,
    .hal_set_rx_gray_code = canary_set_rx_gray_code,
    .hal_get_tx_gray_code = canary_get_tx_gray_code,
    .hal_get_rx_gray_code = canary_get_rx_gray_code,
    .hal_set_tx_precoder = canary_set_tx_precoder,
    .hal_set_rx_precoder = canary_set_rx_precoder,
    .hal_get_tx_precoder = canary_get_tx_precoder,
    .hal_get_rx_precoder = canary_get_rx_precoder,

    /* MSB/LSB swapping */
    .hal_set_tx_msb = canary_set_tx_msb,
    .hal_set_rx_msb = canary_set_rx_msb,
    .hal_get_tx_msb = canary_get_tx_msb,
    .hal_get_rx_msb = canary_get_rx_msb,

    /* PLL calibration */
    .hal_set_tx_cap = canary_set_tx_cap,
    .hal_set_rx_cap = canary_set_rx_cap,
    .hal_get_tx_cap = canary_get_tx_cap,
    .hal_get_rx_cap = canary_get_rx_cap,

    /* RX detail information */
    .hal_get_rx_ppm = be_get_rx_ppm,
    .hal_get_rx_skef = canary_get_rx_skef,
    .hal_get_rx_dac = canary_get_rx_dac,
    .hal_get_ffe_taps = canary_get_ffe_taps,
    .hal_get_f1over3 = canary_get_f1over3,
    .hal_get_agcgain_count = canary_get_agcgain_count,
    .hal_get_agcgain = canary_get_agcgain,
    .hal_get_ctle_count = canary_get_ctle_count,
    .hal_get_ctle = canary_get_ctle,
    .hal_get_delta_phase = canary_get_delta_phase,
    .hal_get_edge = canary_get_edge,
    .hal_get_dfe = canary_get_dfe,
    .hal_get_eye = canary_get_eye,
    .hal_get_lane_ready = canary_get_lane_ready,
    .hal_get_signal_detect = canary_get_signal_detect,

    /* RX debugging -- not to be used lightly */
    .hal_set_rx_skef = canary_set_rx_skef,
    .hal_set_rx_dac = canary_set_rx_dac,
    .hal_set_ffe_taps = canary_set_ffe_taps,
    .hal_set_f1over3 = canary_set_f1over3,
    .hal_set_agcgain = canary_set_agcgain,
    .hal_set_ctle = canary_set_ctle,
    .hal_set_delta_phase = canary_set_delta_phase,
    .hal_set_edge = canary_set_edge,

    /* PRBS related functions */
    .hal_get_rx_prbs = canary_get_rx_prbs,
    .hal_get_tx_prbs = canary_get_tx_prbs,
    .hal_set_tx_prbs = be_set_tx_prbs,
    .hal_set_rx_prbs = canary_set_rx_prbs,
    .hal_set_rx_prbs_nrz = canary_set_rx_prbs_nrz,
    .hal_set_rx_prbs_pam4 = canary_set_rx_prbs_pam4,
    .hal_get_rx_prbs_count = canary_get_rx_prbs_count,
    .hal_get_rx_prbs_ber = common_get_rx_prbs_ber,
    .hal_prbs_get_rx_ber_all = common_get_rx_prbs_ber_all,
    .hal_reset_rx_prbs_count = canary_reset_rx_prbs_count,
    .hal_prbs_get_rx_autosync = canary_get_prbs_rx_autosync,
    .hal_generate_tx_prbs_error = canary_prbs_gen_1error,
    .hal_get_rx_prbs_duration = common_get_rx_prbs_duration,

    /* TX control */
    .hal_get_tx_test_pattern_enable = canary_get_tx_test_pattern_enable,
    .hal_set_tx_test_pattern_enable = be_set_tx_test_pattern_enable,
    .hal_get_tx_test_pattern_memory = canary_get_tx_test_pattern_memory,
    .hal_set_tx_test_pattern_memory = canary_set_tx_test_pattern_memory,
    .hal_get_tx_test_pattern_mode = canary_get_tx_test_pattern_mode,
    .hal_set_tx_test_pattern_mode = canary_set_tx_test_pattern_mode,
    .hal_tx_disable = be_tx_disable,
    .hal_tx_no_disable = be_tx_no_disable,
    .hal_lane_tx_status = be_lane_tx_status,

    /* RX control */
    .hal_rx_disable = be_rx_disable,
    .hal_rx_no_disable = be_rx_no_disable,

    /* Resets -- be careful */
    .hal_lane_rx_reset = canary_lane_rx_reset,
    .hal_soft_reset = common_soft_reset,
    .hal_logic_reset = common_logic_reset,
    .hal_logic_reset_lane = be_logic_reset_lane,
    .hal_mcu_reset = common_mcu_reset,
    .hal_mcu_reset_hold = common_mcu_reset_hold,
    .hal_reg_reset = common_reg_reset,
    .hal_reg_reset_lane = be_reg_reset_lane,

    /* TX taps */
    .hal_set_tx_taps_scale = canary_set_tx_taps_scale,
    .hal_get_tx_taps_scale = canary_get_tx_taps_scale,
    .hal_set_tx_taps = canary_set_tx_taps,
    .hal_set_tx_taps_extended = canary_set_tx_taps_extended,
    .hal_get_tx_taps = canary_get_tx_taps,
    .hal_get_tx_taps_extended = canary_get_tx_taps_extended,
    .hal_reset_tx_taps = canary_reset_tx_taps,

    /* FEC analyzer */
    .hal_set_fec_analyzer = fec_analyzer_set_config,
    .hal_get_fec_analyzer = fec_analyzer_get_config,
    .hal_fecana_reset = fec_analyzer_reset,
    .hal_fecana_get_autosync = fec_analyzer_get_autosync,
    .hal_get_fec_analyzer_read_counter = fec_analyzer_get_read_counter,
    .hal_get_fec_analyzer_counter = fec_analyzer_get_counter,
    .hal_set_fec_analyzer_hist_group = fec_analyzer_set_hist_group,
    .hal_fecana_get_hist_group = fec_analyzer_get_hist_group,
    .hal_get_fec_analyzer_hist_counter = fec_analyzer_get_hist_counter,
    .hal_get_fec_analyzer_duration = fec_analyzer_get_duration,
    .hal_get_fec_analyzer_error_rate = fec_analyzer_get_error_rate,

    /* Auto Neg */
    .hal_set_autoneg_pages = common_set_autoneg_pages,
    .hal_get_autoneg_exchanged_pages = common_get_autoneg_exchanged_pages,

    /* RS-FEC */
    .hal_get_rsfec_index = be_fw_get_rsfec_index,
    .hal_get_rsfec_align_status = rsfec_get_align_status,
    .hal_get_rsfec_fifo = rsfec_get_fifo,
    .hal_get_rsfec_lane_mapping = rsfec_get_lane_mapping,
    .hal_get_rsfec_histogram = rsfec_get_histogram,
    .hal_get_rsfec_corrected_codeword = rsfec_get_corrected_codeword,
    .hal_get_rsfec_uncorrected_codeword = rsfec_get_uncorrected_codeword,
    .hal_get_rsfec_symbol_error = rsfec_get_symbol_error,
    .hal_get_rsfec_total_codeword = rsfec_get_total_codewords,
    .hal_get_rsfec_corrected_bits = rsfec_get_corrected_bits,
    .hal_get_rsfec_count_freeze = rsfec_get_count_freeze,
    .hal_set_rsfec_count_freeze = rsfec_set_count_freeze,
    .hal_reset_rsfec_count = rsfec_reset_count,

    // serdes_param
    .hal_find_param_def = param_find_param_def,
    .hal_param_get_data = param_get_data,
    .hal_param_set_data = param_set_data,
    .hal_get_param_count = param_get_param_count,
    .hal_get_param_val_count = param_get_param_val_count,
    .hal_get_param_val_set_count = param_get_param_val_set_count,
    .hal_index_param_def = param_index_param_def,

    .hal_serdes_preset_tx_taps = be_serdes_preset_tx_taps,

    .hal_addr_stringify = common_addr_stringify,

    .hal_efuse_read = common_efuse_read_bank,
    .hal_efuse_read_ecid = common_efuse_read_ecid,

    .hal_prbs_pattern_rx_set_prev = canary_set_rx_prbs_prev,
    .hal_prbs_pattern_rx_reset_count = canary_reset_rx_prbs_pattern_count,
    .hal_prbs_pattern_rx_get_count = canary_get_rx_prbs_pattern_count,
    .hal_prbs_pattern_rx_set_phase = canary_set_rx_prbs_pattern_phase,
};

static const SliceOption_t be_option_list[] = {
    {"fw_freeze", "freeze/un-freeze firmware", common_option_set_fw_freeze, common_option_get_fw_freeze},
    {"fw_cmd_timeout", "default is 50000 us, set 0 to default. This value will not be reset after slice init.",
     common_option_set_fw_cmd_timeout, common_option_get_fw_cmd_timeout},
    {"fw_unload_timeout", "default is 100000 us, set 0 to default. This value will not be reset after slice init.",
     common_option_set_fw_unload_timeout, common_option_get_fw_unload_timeout},
    {"low_VAA",
     "Default is disabled. If powered using low VAA(1.55V), this option should be enabled. Intended to set after slice "
     "initialization.",
     be_option_set_low_vaa, be_option_get_low_vaa},
};

static const LaneOption_t be_lane_option_list[] = {
    {"fast_recover_timeout",
     "Maximum fast link recover timeout (unit: milliseconds). If the line side (NRZ mode only) incoming signal is lost "
     "and recover within this timeout, It will try to recover using the fast path. Otherwise the SerDes will do a full "
     "restart. Only NRZ retimer and 100G gearbox are supported. Each line side lane in the port should have the same "
     "timeout value. Set timeout to 0 to disable.",
     be_lane_option_set_fast_recover_timeout, be_lane_option_get_fast_recover_timeout},
    {"sd_delay", "signal delay timeout (unit: milliseconds), This option is persistent across port/lane reconfigure.",
     be_lane_option_set_sd_delay, be_lane_option_get_sd_delay},
};

const SliceCapability_t be_slice = {
    .slice_type = CREDO_BALDEAGLE,
    .port_count = BE_MAX_PORT,
    .lane_count = 16,
    .vsensor_count = BE_MAX_VSENSOR,
    .hal_func_table = &be_hal_func_table,
    .option_count = sizeof(be_option_list) / sizeof(SliceOption_t),
    .option_list = be_option_list,
    .lane_option_count = sizeof(be_lane_option_list) / sizeof(LaneOption_t),
    .lane_option_list = be_lane_option_list,
};

const DeviceCapability_t be_400 = {
    .device_type = CREDO_BALDEAGLE_400,
    .slice_count = 1,
    /* No disabled lanes */
    .slice_capability = &be_slice,
};

const DeviceCapability_t be_800 = {
    .device_type = CREDO_BALDEAGLE_800,
    .slice_count = 2,
    /* No disabled lanes */
    .slice_capability = &be_slice,
};

void hal_register_sdk(void) {
    cr_register_device(&be_400);
    cr_register_device(&be_800);
}
