#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"
#include "se_option.h"

#include "anlt/anlt.h"
#include "common/common_efuse.h"
#include "common/common_firmware.h"
#include "common/common_misc.h"
#include "common/common_prbs.h"
#include "common/common_reset.h"
#include "common/options.h"
#include "common/params.h"
#include "fec_analyzer/fec_analyzer.h"
#include "swift/swift_serdes.h"

#include "sdk.h"

/* Register hive definition */
const RegHive_t PHY[] = {
    {
        .hivename = HIVENAME_SERDES_RX,
        .base_addr = {0x1000},
        .shift = 11,
        .count = 16,
    },
};

const RegHive_t TX_TOP[] = {
    {
        .hivename = HIVENAME_SERDES_TX,
        .base_addr = {0x1200},
        .shift = 11,
        .count = 16,
    },
};

const RegHive_t TopAna[] = {{.hivename = HIVENAME_TOP_ANA,
                             .base_addr = {0x1100, 0x1900, 0x2100, 0x2900, 0x3100, 0x3900, 0x4100, 0x4900, 0x5100,
                                           0x5900, 0x6100, 0x6900, 0x7100, 0x7900, 0x8100, 0x8900},
                             .shift = BASE_ADDRESS_EXTENDED,
                             .count = 16}};

const RegHive_t PllTx[] = {{.hivename = HIVENAME_PLL_TX,
                            .base_addr = {0x1300, 0x1B00, 0x2300, 0x2B00, 0x3300, 0x3B00, 0x4300, 0x4B00, 0x5300,
                                          0x5B00, 0x6300, 0x6B00, 0x7300, 0x7B00, 0x8300, 0x8B00},
                            .shift = BASE_ADDRESS_EXTENDED,
                            .count = 16}};

const RegHive_t Top[] = {
    {
        .hivename = HIVENAME_TOP,
        .base_addr = {0xE000},
        .shift = 0,
        .count = 1,
    },
};

// top pll
const RegHive_t TopPLL[] = {
    {
        .hivename = HIVENAME_TOPPLL,
        .base_addr = {0xFA00},
        .shift = 0,
        .count = 1,
    },
};

// reg one lane
const RegHive_t TopOneLane[] = {
    {
        .hivename = HIVENAME_TOPONELANE,
        .base_addr = {0x1700},
        .shift = 11,
        .count = 16,
    },
};

// sensor
const RegHive_t TopSensor[] = {
    {
        .hivename = HIVENAME_TOPSENSOR,
        .base_addr = {0xFD00},
        .shift = 0,
        .count = 1,
    },
};

// fec analyzer
const RegHive_t FecAnalyzer[] = {
    {
        .hivename = HIVENAME_FECANA,
        .base_addr = {0x1180},
        .shift = 11,
        .count = 16,
    },
};

// ecc
const RegHive_t Ecc[] = {
    {
        .hivename = HIVENAME_ECC,
        .base_addr = {0xFFE0},
        .shift = 0,
        .count = 1,
    },
};

const RegHive_t AutoNegIEEE[] = {
    {
        .hivename = HIVENAME_ANI,  // AnI
        .base_addr = {0x1400},
        .shift = 11,
        .count = 16,
    },
};

const RegHive_t AutoNegCustom[] = {
    {
        .hivename = HIVENAME_ANC,  // AnC
        .base_addr = {0x1500},
        .shift = 11,
        .count = 16,
    },
};

const RegHive_t* reghive_array[] = {PHY,    TX_TOP,      Top,         TopPLL,        TopOneLane, TopSensor,
                                    TopAna, FecAnalyzer, AutoNegIEEE, AutoNegCustom, NULL};

static const HalFuncTable_t se_hal_func_table = {
    .hal_allocate_slice = se_allocate_slice,
    .hal_free_slice = se_free_slice,
    .hal_slice_init = se_slice_init,
    .hal_slice_data_init = se_slice_data_init,
    .hal_slice_get_vsensor = se_slice_get_vsensor,
    .hal_slice_get_sram_status = common_slice_get_sram_status,
    .hal_slice_generate_sram_error = common_slice_generate_sram_error,
    .hal_slice_load_setup = common_slice_load_setup,
    .hal_slice_save_setup = se_slice_save_setup,
    .hal_slice_get_reghive = common_slice_get_reghive,
    .hal_slice_get_reghive_count = common_slice_get_reghive_count,
    .hal_slice_index_reghive = common_slice_index_reghive,
    .hal_slice_get_revision_number = se_slice_get_revision,

    .hal_display_info = se_display_info,

    /* Firmware itself */
    .hal_fw_unload = common_fw_unload,
    .hal_fw_load = common_fw_load,
    .hal_reg_get_notepad = common_fw_notepad,
    .hal_fw_load_broadcast = common_fw_load_broadcast,
    .hal_fw_wait_magic_word = common_fw_wait_magic_word,
    .hal_fw_clear_top_pll_cal = common_fw_clear_top_pll_cal,
    .hal_fw_wait_top_pll_cal = common_fw_wait_top_pll_cal,
    .hal_fw_get_status = se_fw_get_status,

    /* Raw firmware commands */
    .hal_fw_get_raw_cmd_address = se_fw_get_raw_cmd_address,
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
    .hal_fw_teardown_port = se_fw_teardown_port,
    .hal_fw_phy_ready = se_fw_phy_ready,
    .hal_fw_phy_lane_ready = se_fw_phy_lane_ready,
    .hal_fw_config_phy = se_fw_config_phy,
    .hal_fw_config_lane = se_fw_config_lane,
    //.hal_fw_config_lane_loopback = se_fw_config_lane_loopback,
    .hal_fw_deconfig_lane = se_fw_deconfig_lane,
    .hal_fw_clear_all_port = se_fw_teardown_port_all,
    .hal_port_is_link_up = se_port_is_link_up,

    .hal_fw_get_slice_temp = common_fw_get_slice_temp,

    /* Firmware serdes param */
    .hal_fw_get_adapt_count = common_fw_get_adapt_count,
    .hal_fw_get_readapt_count = common_fw_get_readapt_count,
    .hal_fw_get_link_lost_count = common_fw_get_link_lost_count,
    .hal_fw_get_los_count = common_fw_get_los_count,
    .hal_fw_get_eye = se_fw_get_eye,
    .hal_fw_get_channel_estimate = se_fw_get_channel_est,
    //.hal_fw_get_dfe = se_fw_get_dfe,
    .hal_fw_get_lane_speed = se_get_lane_speed,
    .hal_fw_get_tx_lane_speed = se_get_lane_tx_speed,

    /* Firmware function */
    .hal_fw_eye_monitor_start = se_fw_eye_monitor_start,
    .hal_fw_eye_monitor_stop = se_fw_eye_monitor_stop,
    .hal_fw_eye_monitor_progress = se_fw_eye_monitor_progress,
    .hal_fw_eye_monitor_data = se_fw_eye_monitor_data,
    .hal_fw_eye_monitor_range = se_fw_eye_monitor_range,
    .hal_fw_eye_monitor_separator = se_fw_eye_monitor_separator,
    .hal_fw_load_spi = common_fw_load_spi,
    .hal_fw_spiflash_status = se_fw_spiflash_status,
    .hal_fw_spiflash_read = se_fw_spiflash_read,
    .hal_fw_spiflash_write = se_fw_spiflash_write,
    .hal_fw_spiflash_erase = se_fw_spiflash_erase,
    .hal_fw_spiflash_display_mbr = se_fw_spiflash_display,
    .hal_fw_spiflash_format_mbr = se_fw_spiflash_format,
    .hal_fw_spiflash_read_firmware = se_fw_spiflash_read_firmware,
    .hal_fw_spiflash_write_firmware = se_fw_spiflash_write_firmware,
    .hal_fw_spiflash_set_partition = se_fw_spiflash_set_partition,

    /* Top PLL */
    .hal_get_top_pll_cap = se_get_top_pll_cap,

    /* Capability */
    .hal_get_lane_count = se_get_lane_count,

    /* Lane operation mode */
    .hal_get_lane_mode = se_get_lane_mode,
    .hal_get_tx_lane_mode = se_get_lane_tx_mode,
    .hal_set_lane_mode = se_set_lane_mode,
    .hal_update_lane_mode = se_update_lane_mode,
    .hal_disable_lane = se_fw_lane_disable,

    .hal_get_lane_config = common_get_lane_config,
    .hal_set_lane_config = common_set_lane_config,

    .hal_set_lane_loopback_mode = se_set_lane_loopback_mode,
    .hal_get_lane_loopback_mode = se_get_lane_loopback_mode,

    /* SerDes capability */
    .hal_get_rx_ffe_range = swift_get_rx_ffe_range,
    .hal_get_tx_ffe_range = swift_get_tx_ffe_range,
    .hal_get_rx_dfe_range = swift_get_rx_dfe_range,
    .hal_get_rx_isi_range = swift_get_rx_isi_range,

    /* Polarity */
    .hal_set_tx_polarity = se_set_tx_polarity,
    .hal_set_rx_polarity = se_set_rx_polarity,
    .hal_get_tx_polarity = se_get_tx_polarity,
    .hal_get_rx_polarity = se_get_rx_polarity,

    /* Input Mode */
    .hal_set_rx_input_mode = swift_set_rx_input_mode,
    .hal_get_rx_input_mode = swift_get_rx_input_mode,

    /* Gray code and precoding */
    .hal_set_tx_gray_code = swift_set_tx_gray_code,
    .hal_set_rx_gray_code = swift_set_rx_gray_code,
    .hal_get_tx_gray_code = swift_get_tx_gray_code,
    .hal_get_rx_gray_code = swift_get_rx_gray_code,
    .hal_set_tx_precoder = swift_set_tx_precoder,
    .hal_set_rx_precoder = swift_set_rx_precoder,
    .hal_get_tx_precoder = swift_get_tx_precoder,
    .hal_get_rx_precoder = swift_get_rx_precoder,

    /* MSB/LSB swapping */
    .hal_set_tx_msb = swift_set_tx_msb,
    .hal_set_rx_msb = swift_set_rx_msb,
    .hal_get_tx_msb = swift_get_tx_msb,
    .hal_get_rx_msb = swift_get_rx_msb,

    /* PLL calibration */
    .hal_set_tx_cap = swift_set_tx_cap,
    .hal_set_rx_cap = swift_set_rx_cap,
    .hal_get_tx_cap = swift_get_tx_cap,
    .hal_get_rx_cap = swift_get_rx_cap,

    /* RX detail information */
    .hal_get_rx_ppm = se_get_ppm,
    //.hal_get_rx_skef = swift_get_rx_skef,
    //.hal_get_rx_dac = swift_get_rx_dac,
    //.hal_get_rx_attenuator = condor_lp_get_rx_atten,
    .hal_get_ffe_taps = se_get_rx_ffe,
    //.hal_get_ffe_taps_fine = condor_lp_get_rx_ffe_fine,
    //.hal_get_f1over3 = condor_lp_get_f1over3,
    .hal_get_agcgain_count = se_get_agcgain_count,
    .hal_get_agcgain = swift_get_rx_agcgain,
    .hal_get_ctle_count = swift_get_rx_ctle_count,
    .hal_get_ctle = swift_get_rx_ctle,
    //.hal_get_delta_phase = condor_lp_get_rx_delta,
    //.hal_get_edge = condor_lp_get_edge,
    //.hal_get_dfe = se_get_dfe,
    .hal_get_eye = swift_get_rx_eye,
    .hal_get_lane_ready = swift_get_rx_lane_ready,
    .hal_get_signal_detect = se_get_rx_signal_detect,

    /* RX debugging -- not to be used lightly */
    //.hal_set_rx_skef = swift_set_rx_skef,
    //.hal_set_rx_dac = swift_set_rx_dac,
    //.hal_set_rx_attenuator = condor_lp_set_rx_atten,
    .hal_set_ffe_taps = swift_set_rx_ffe,
    //.hal_set_ffe_taps_fine = condor_lp_set_rx_ffe_fine,
    //.hal_set_f1over3 = condor_lp_set_f1over3,
    .hal_set_agcgain = swift_set_rx_agcgain,
    .hal_set_ctle = swift_set_rx_ctle,
    //.hal_set_delta_phase = condor_lp_set_rx_delta,
    //.hal_set_edge = condor_lp_set_edge,

    /* PRBS related functions */
    .hal_get_rx_prbs = swift_get_rx_prbs,
    .hal_get_tx_prbs = swift_get_tx_prbs,
    .hal_set_tx_prbs = se_set_tx_prbs,
    .hal_set_rx_prbs = se_set_rx_prbs,
    .hal_set_tx_prbs_pam4 = se_set_tx_prbs_pam4,
    .hal_set_tx_prbs_nrz = se_set_tx_prbs_nrz,

    //.hal_set_rx_prbs_nrz = condor_lp_set_rx_prbs_nrz,
    //.hal_set_rx_prbs_pam4 = condor_lp_set_rx_prbs_pam4,
    .hal_get_rx_prbs_count = swift_get_rx_prbs_error_count,
    .hal_get_rx_prbs_ber = common_get_rx_prbs_ber,
    .hal_prbs_get_rx_ber_all = common_get_rx_prbs_ber_all,
    .hal_reset_rx_prbs_count = swift_rx_prbs_rst,
    .hal_get_rx_prbs_duration = common_get_rx_prbs_duration,
    .hal_generate_tx_prbs_error = swift_prbs_gen_1error,
    .hal_get_rx_prbs_lock = swift_get_rx_prbs_lock,
    .hal_prbs_get_rx_autosync = swift_prbs_get_rx_autosync,

    /* TX control */
    .hal_get_tx_test_pattern_enable = swift_get_tx_test_pattern_enable,
    .hal_set_tx_test_pattern_enable = se_set_tx_test_pattern_enable,
    .hal_get_tx_test_pattern_memory = swift_get_tx_test_pattern_memory,
    .hal_set_tx_test_pattern_memory = swift_set_tx_test_pattern_memory,
    .hal_get_tx_test_pattern_mode = swift_get_tx_test_pattern_mode,
    .hal_set_tx_test_pattern_mode = swift_set_tx_test_pattern_mode,
    .hal_tx_disable = se_fw_tx_disable,
    .hal_tx_no_disable = se_fw_tx_no_disable,
    .hal_lane_tx_status = se_fw_tx_status,

    /* RX control */
    .hal_rx_disable = se_fw_rx_disable,
    .hal_rx_no_disable = se_fw_rx_no_disable,

    /* Resets -- be careful */
    .hal_lane_rx_reset = se_fw_rx_reset,
    .hal_soft_reset = common_soft_reset,
    .hal_logic_reset = common_logic_reset,
    .hal_logic_reset_lane = se_logic_reset_lane,
    .hal_mcu_reset = common_mcu_reset,
    .hal_mcu_reset_hold = common_mcu_reset_hold,
    .hal_reg_reset = common_reg_reset,
    .hal_reg_reset_lane = se_reg_reset_lane,

    /* TX taps */
    .hal_set_tx_taps = se_set_tx_taps,
    .hal_set_tx_taps_extended = swift_set_tx_taps_extended,
    .hal_get_tx_taps = se_get_tx_taps,
    .hal_get_tx_taps_extended = swift_get_tx_taps_extended,
    .hal_reset_tx_taps = swift_reset_tx_taps,

    /* FEC analyzer */
    .hal_set_fec_analyzer = se_fec_analyzer_set_config,
    .hal_get_fec_analyzer = se_fec_analyzer_get_config,
    .hal_get_fec_analyzer_read_counter = fec_analyzer_get_read_counter,
    .hal_get_fec_analyzer_counter = fec_analyzer_get_counter,
    .hal_set_fec_analyzer_hist_group = fec_analyzer_set_hist_group,
    .hal_fecana_get_hist_group = fec_analyzer_get_hist_group,
    .hal_get_fec_analyzer_hist_counter = fec_analyzer_get_hist_counter,
    .hal_get_fec_analyzer_duration = fec_analyzer_get_duration,
    .hal_get_fec_analyzer_error_rate = fec_analyzer_get_error_rate,
    .hal_fecana_reset = se_fec_analyzer_reset,
    .hal_fecana_get_autosync = fec_analyzer_get_autosync,

    // param
    .hal_param_get_data = param_get_data,
    .hal_param_set_data = param_set_data,
    .hal_find_param_def = param_find_param_def,
    .hal_get_param_count = param_get_param_count,
    .hal_get_param_val_count = param_get_param_val_count,
    .hal_get_param_val_set_count = param_get_param_val_set_count,

    .hal_index_param_def = param_index_param_def,

    .hal_get_option = option_get_option,
    .hal_set_option = option_set_option,
    .hal_get_option_count = option_get_count,
    .hal_get_option_definition = option_get_def,

    // port build
    .hal_port_build = se_port_build,
    .hal_port_get_setup = se_port_get_setup,
    .hal_port_start = se_port_start,
    .hal_port_assign_id = se_port_assign_id,

    .hal_serdes_preset_tx_taps = se_serdes_preset_tx_taps,

    // clock output
    .hal_slice_enable_clock_output = se_slice_enable_clock_output,
    .hal_slice_disable_all_clock_output = se_slice_disable_all_clock_output,
    .hal_slice_disable_clock_output = se_slice_disable_clock_output,

    .hal_get_autoneg_exchanged_pages = se_autoneg_get_exchanged_pages,
    .hal_set_autoneg_pages = common_set_autoneg_pages,
    .hal_autoneg_get_state = se_an_get_state,
    .hal_autoneg_get_restart_count = se_an_get_restart_count,
    .hal_link_training_get_restart_count = se_lt_get_restart_count,
    .hal_link_training_get_status = se_get_lt_status,
    .hal_link_training_get_state = se_get_lt_state,

    .hal_data_capture = se_data_capture,
    .hal_data_get_commands = se_data_get_commands,

    // fast recovery
    .hal_frecov_configure = se_frecov_configure,
    .hal_frecov_get_status = se_frecov_get_status,
    .hal_frecov_get_recover_count = se_frecov_get_recover_count,

    .hal_addr_stringify = common_addr_stringify,

    .hal_testpoint_clear = se_testpoint_clear,
    .hal_testpoint_select = se_testpoint_select,
    .hal_testpoint_read = se_testpoint_read,

    .hal_time_start = se_time_start,
    .hal_time_system = se_time_system,

    .hal_efuse_read = common_efuse_read_bank,
    .hal_efuse_read_ecid = common_efuse_read_ecid,

    .hal_prbs_pattern_rx_set_prev = swift_set_rx_prbs_prev,
    .hal_prbs_pattern_rx_reset_count = swift_reset_rx_prbs_pattern_count,
    .hal_prbs_pattern_rx_get_count = swift_get_rx_prbs_pattern_count,
    .hal_prbs_pattern_rx_set_phase = swift_set_rx_prbs_pattern_phase,

    .hal_prbs_set_tx_checker = swift_set_tx_prbs_checker,
    .hal_prbs_get_tx_checker = swift_get_tx_prbs_checker,
    .hal_prbs_get_tx_count = swift_get_tx_prbs_error_count,
    .hal_prbs_reset_tx_count = swift_tx_prbs_rst,
    .hal_prbs_get_tx_ber = common_get_tx_prbs_ber,
    .hal_prbs_get_tx_ber_all = common_get_tx_prbs_ber_all,
    .hal_prbs_get_tx_duration = common_get_tx_prbs_duration,
};

static const SliceOption_t se_option_list[] = {
    {"fw_freeze", "freeze/un-freeze firmware", common_option_set_fw_freeze, common_option_get_fw_freeze},
    {"fw_cmd_timeout", "default is 100000 us, set 0 to default. This value will not be reset after slice init.",
     common_option_set_fw_cmd_timeout, common_option_get_fw_cmd_timeout},
    {"fw_unload_timeout", "default is 100000 us, set 0 to default. This value will not be reset after slice init.",
     common_option_set_fw_unload_timeout, common_option_get_fw_unload_timeout},
    {"fw_isi_timeout",
     "per-phase timeout, default is 20000 ms, set 0 to default. This value will be reset after slice init.",
     se_option_set_fw_isi_timeout, se_option_get_fw_isi_timeout},
    {"top_cal_timeout", "default is 150000 us, set 0 to default. This value will not be reset after slice init.",
     common_option_set_top_cal_timeout, common_option_get_top_cal_timeout},
    {"fw_toppll_mode",
     "0: bypass toppll and div2, 1: bypass toppll and enable div2, 2: enable toppll and bypass div2. default is 0",
     se_option_set_toppll_mode, se_option_get_toppll_mode},
    {"fw_em_vstep_side", "This option is used for debug purpose. firmware eye vstep side value, set non-zero to enable",
     se_option_set_fw_em_vstep_side, se_option_get_fw_em_vstep_side},
    {"fw_spiflash_load_timeout",
     "default is (16 MCU * 500 ms) * 1000, set 0 to put to default. Use microsecond units. This value should be set "
     "before slice init.",
     common_option_set_fw_spiflash_load_timeout, common_option_get_fw_spiflash_load_timeout},
    {"isc_slice_id", "ISC slice id", se_option_set_isc_slice_id, se_option_get_isc_slice_id},
    {"anlt_holdoff_timer", "", se_slice_option_set_lt_timer, se_slice_option_get_lt_timer},
    {"anlt_max_wait_timer", "", se_slice_option_set_lt_timer, se_slice_option_get_lt_timer},
    {"anlt_wait_timer", "", se_slice_option_set_lt_timer, se_slice_option_get_lt_timer},
    {"anlt_link_fail_inhibit_timer",
     "Check duration from AN done to LT remote ready. Default is 1000. Value is multipled by factor determined by lane "
     "speed:\n"
     "- 10g,25g: 0.5\n"
     "- 53g:     1.2\n"
     "- 106g:    3.0",
     se_slice_option_set_lt_timer, se_slice_option_get_lt_timer},
    {"vsensor_resolution",
     "Vsensor output resolution control. Default is 3.\n- 0: 14-bit, 1: 12-bit, 2: 10-bit, 3: 8-bit",
     se_slice_option_set_vsensor_res, se_slice_option_get_vsensor_res},
};

static const LaneOption_t se_lane_option_list[] = {
    {"eye_phase_base", "eye phase base for eye diagrams", se_lane_option_set_phase_base, se_lane_option_get_phase_base},
    {"sd_delay", "signal detect delay value (ms)", se_lane_option_set_sd_delay, se_lane_option_get_sd_delay},
    {"fast_recover_timeout",
     "Maximum fast link recover timeout (unit: milliseconds). If the incoming signal is lost "
     "and recover within this timeout, It will try to recover using the fast path. Otherwise the SerDes will do a full "
     "restart. Each lane in the port should have the same timeout value."
     "Set timeout to 0 to disable. Reset to 0 when port/lane is reconfigured.",
     se_lane_option_set_fast_recover_timeout, se_lane_option_get_fast_recover_timeout},
    {"oneshot",
     "Debug tool to enable oneshot mode. Oneshot mode prevents lane readaptation, allowing you to better debug the "
     "lane in the failing state.",
     se_lane_option_set_oneshot, se_lane_option_get_oneshot},
    {"lt_oneshot", "Debug tool to enable LT oneshot mode. Oneshot mode prevents link training from restarting",
     se_lane_option_set_lt_oneshot, se_lane_option_get_lt_oneshot},
};

const SliceCapability_t se_slice = {
    .slice_type = CREDO_SLC_SCREAMING_EAGLE,
    .port_count = 16,
    .lane_count = 16,
    .vsensor_count = SE_MAX_VSENSOR,
    .hal_func_table = &se_hal_func_table,
    .option_count = sizeof(se_option_list) / sizeof(SliceOption_t),
    .option_list = se_option_list,
    .lane_option_count = sizeof(se_lane_option_list) / sizeof(LaneOption_t),
    .lane_option_list = se_lane_option_list,
};

const DeviceCapability_t screaming_eagle_aec = {
    .device_type = CREDO_SCREAMING_EAGLE_AEC,
    .slice_count = 1,
    .slice_capability = &se_slice,
};

const DeviceCapability_t screaming_eagle = {
    .device_type = CREDO_SCREAMING_EAGLE,
    .slice_count = 2,
    .slice_capability = &se_slice,
};

void hal_register_sdk(void) {
    cr_register_device(&screaming_eagle_aec);
    cr_register_device(&screaming_eagle);
}
