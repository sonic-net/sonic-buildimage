#include "bh_device.h"
#include "bh_functions.h"
#include "bh_option.h"
#include "blackhawk.h"

#include "anlt/anlt.h"
#include "common/common_efuse.h"
#include "common/common_firmware.h"
#include "common/common_misc.h"
#include "common/common_prbs.h"
#include "common/common_reset.h"
#include "common/options.h"
#include "common/params.h"
#include "condor_lp/condor_lp_serdes.h"
#include "fec_analyzer/fec_analyzer.h"
#include "rsfec/rsfec.h"

#include "sdk.h"

/* Register hive definition */
const RegHive_t PHY_CONDORLP[] = {
    {
        .hivename = HIVENAME_SERDES_RX,  // SerDes_Conderlp
        .base_addr = {0x7000},
        .shift = 9,
        .count = 16,
    },
};

const RegHive_t TX_TOP[] = {
    {
        .hivename = HIVENAME_SERDES_TX,  // CondorLP_TX
        .base_addr = {0x7000},
        .shift = 9,
        .count = 16,
    },
};

const RegHive_t Top[] = {
    {
        .hivename = HIVENAME_TOP,  //  BHTop
        .base_addr = {0x9800},
        .shift = 0,
        .count = 1,
    },
};

const RegHive_t TopPLL[] = {
    {
        .hivename = HIVENAME_TOPPLL,  // BHTopPLL
        .base_addr = {0x9500},
        .shift = 0,
        .count = 1,
    },
};

const RegHive_t TopLane[] = {
    {
        .hivename = HIVENAME_TOPLANE,  // BHTopLane
        .base_addr = {0xc000},
        .shift = 8,
        .count = 8,
    },
};

const RegHive_t TopSensor[] = {
    {
        .hivename = HIVENAME_TOPSENSOR,  // BHTopSensor
        .base_addr = {0xb000},
        .shift = 0,
        .count = 1,
    },
};

const RegHive_t FecAnalyzer[] = {
    {
        .hivename = HIVENAME_FECANA,  // FecAnalyzer
        .base_addr = {0x3000, 0x3100, 0x3200, 0x3300, 0x3400, 0x3500, 0x3600, 0x3700, 0x3080, 0x3180, 0x3280, 0x3380,
                      0x3480, 0x3580, 0x3680, 0x3780},
        .shift = BASE_ADDRESS_EXTENDED,
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

const RegHive_t AutoNegIEEE[] = {
    {
        .hivename = HIVENAME_ANI,  // AnI
        .base_addr = {0xD800},
        .shift = 8,
        .count = 8,
    },
};

const RegHive_t AutoNegCustom[] = {
    {
        .hivename = HIVENAME_ANC,  // AnC
        .base_addr = {0xE800},
        .shift = 8,
        .count = 8,
    },
};

const RegHive_t* reghive_array[] = {PHY_CONDORLP, TX_TOP,      TopPLL,        Top,   TopLane, TopSensor,
                                    FecAnalyzer,  AutoNegIEEE, AutoNegCustom, RSFec, NULL};

static const HalFuncTable_t bh_hal_func_table = {
    .hal_allocate_slice = bh_allocate_slice,
    .hal_free_slice = bh_free_slice,
    .hal_slice_init = bh_slice_init,
    .hal_slice_data_init = bh_slice_data_init,
    .hal_slice_get_oui = common_slice_get_oui,
    .hal_slice_get_model_number = common_slice_get_model_number,
    .hal_slice_get_revision_number = common_slice_get_revision_number,
    .hal_slice_get_type = bh_slice_get_type,
    .hal_slice_get_vsensor = bh_slice_get_vsensor,
    .hal_slice_get_sram_status = common_slice_get_sram_status,
    .hal_slice_generate_sram_error = common_slice_generate_sram_error,
    .hal_slice_load_setup = common_slice_load_setup,
    .hal_slice_save_setup = bh_slice_save_setup,
    .hal_slice_get_reghive = common_slice_get_reghive,
    .hal_slice_index_reghive = common_slice_index_reghive,
    .hal_slice_get_reghive_count = common_slice_get_reghive_count,

    // clock output
    .hal_slice_enable_clock_output = bh_slice_enable_clock_output,
    .hal_slice_disable_all_clock_output = bh_slice_disable_all_clock_output,
    .hal_slice_disable_clock_output = bh_slice_disable_clock_output,

    .hal_display_info = bh_display_slice_info,

    /* Firmware itself */
    .hal_fw_unload = common_fw_unload,
    .hal_fw_load = common_fw_load,
    .hal_reg_get_notepad = common_fw_notepad,
    .hal_fw_load_broadcast = common_fw_load_broadcast,
    .hal_fw_load_spi = common_fw_load_spi,
    .hal_fw_ready_post_actions = bh_fw_ready_post_actions,
    .hal_fw_spiflash_status = common_fw_spiflash_status,
    .hal_fw_spiflash_read = common_fw_spiflash_read,
    .hal_fw_spiflash_write = common_fw_spiflash_write,
    .hal_fw_spiflash_erase = common_fw_spiflash_erase,
    .hal_fw_spiflash_display_mbr = common_fw_spiflash_display_mbr,
    .hal_fw_spiflash_format_mbr = common_fw_spiflash_format_mbr,
    .hal_fw_spiflash_read_firmware = common_fw_spiflash_read_firmware,
    .hal_fw_spiflash_write_firmware = common_fw_spiflash_write_firmware,

    .hal_fw_wait_magic_word = common_fw_wait_magic_word,
    .hal_fw_clear_top_pll_cal = common_fw_clear_top_pll_cal,
    .hal_fw_wait_top_pll_cal = common_fw_wait_top_pll_cal,
    .hal_fw_get_status = bh_fw_get_status,

    /* Raw firmware commands */
    .hal_fw_get_raw_cmd_address = bh_fw_get_raw_cmd_address,
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
    .hal_fw_config_port = bh_port_config,
    .hal_fw_teardown_port = bh_port_teardown,
    .hal_fw_phy_ready = bh_fw_phy_ready,
    .hal_fw_phy_lane_ready = bh_fw_phy_lane_ready,
    .hal_fw_config_phy = bh_fw_config_phy,
    .hal_fw_config_lane = bh_fw_config_lane,
    .hal_fw_config_lane_loopback = bh_fw_config_lane_loopback,
    .hal_fw_deconfig_lane = bh_fw_deconfig_lane,
    .hal_fw_query_port = bh_port_query,
    .hal_port_is_link_up = bh_port_is_link_up,
    .hal_fw_clear_all_port = bh_port_teardown_all,

    .hal_fw_get_slice_temp = common_fw_get_slice_temp,
    .hal_fw_atomic_read = bh_fw_atomic_read,

    /* Firmware serdes param */
    .hal_fw_get_adapt_count = common_fw_get_adapt_count,
    .hal_fw_get_readapt_count = common_fw_get_readapt_count,
    .hal_fw_get_link_lost_count = common_fw_get_link_lost_count,
    .hal_fw_get_los_count = common_fw_get_los_count,
    .hal_fw_get_channel_estimate = common_fw_get_channel_estimate,
    .hal_fw_get_of = common_fw_get_of,
    .hal_fw_get_hf = common_fw_get_hf,
    .hal_fw_get_dfe = common_fw_get_dfe,
    .hal_fw_get_eye = common_fw_get_eye,
    .hal_fw_get_isi = common_fw_get_isi,
    .hal_fw_get_rx_ffe = common_fw_get_rx_ffe,
    .hal_fw_get_rx_ffe_nbias = common_fw_get_rx_ffe_nbias,
    .hal_fw_get_rx_ffe_kaccu = common_fw_get_rx_ffe_kaccu,
    .hal_fw_get_rx_ffe_flip_counter = common_fw_get_rx_ffe_flip_counter,
    .hal_fw_get_rx_ffe_weighting_table = bh_fw_get_rx_ffe_weighting_table,
    .hal_fw_get_lane_speed = bh_get_lane_speed,

    /* Firmware eye monitor function */
    .hal_fw_eye_monitor_start = bh_fw_eye_monitor_start,
    .hal_fw_eye_monitor_stop = bh_fw_eye_monitor_stop,
    .hal_fw_eye_monitor_progress = common_fw_em_progress,
    .hal_fw_eye_monitor_data = common_fw_em_data,
    .hal_fw_eye_monitor_range = bh_fw_eye_monitor_range,
    .hal_fw_eye_monitor_separator = common_fw_em_separator,

    /* Top PLL */
    .hal_get_top_pll_cap = bh_get_top_pll_cap,

    /* Capability */
    .hal_get_lane_count = bh_get_lane_count,

    /* Lane operation mode */
    .hal_get_lane_mode = bh_get_lane_mode,
    .hal_set_lane_mode = bh_set_lane_mode,
    .hal_update_lane_mode = bh_update_lane_mode,

    .hal_get_lane_config = common_get_lane_config,
    .hal_set_lane_config = common_set_lane_config,

    .hal_set_lane_loopback_mode = bh_set_lane_loopback_mode,
    .hal_get_lane_loopback_mode = bh_get_lane_loopback_mode,

    /* SerDes capability */
    .hal_get_rx_ffe_range = condor_lp_get_rx_ffe_range,
    .hal_get_rx_ffe_weighting_table_range = condor_lp_get_rx_ffe_weighting_table_range,
    .hal_get_tx_ffe_range = condor_lp_get_tx_ffe_range,
    .hal_get_rx_dfe_range = condor_lp_get_rx_dfe_range,
    .hal_get_rx_isi_range = condor_lp_get_rx_isi_range,

    /* Polarity */
    .hal_set_tx_polarity = condor_lp_set_tx_polarity,
    .hal_set_rx_polarity = condor_lp_set_rx_polarity,
    .hal_get_tx_polarity = condor_lp_get_tx_polarity,
    .hal_get_rx_polarity = condor_lp_get_rx_polarity,

    /* Input Mode */
    .hal_set_rx_input_mode = condor_lp_set_rx_input_mode,
    .hal_get_rx_input_mode = condor_lp_get_rx_input_mode,

    /* Gray code and precoding */
    .hal_set_tx_gray_code = condor_lp_set_tx_gray_code,
    .hal_set_rx_gray_code = condor_lp_set_rx_gray_code,
    .hal_get_tx_gray_code = condor_lp_get_tx_gray_code,
    .hal_get_rx_gray_code = condor_lp_get_rx_gray_code,
    .hal_set_tx_precoder = bh_set_tx_precoder,
    .hal_set_rx_precoder = condor_lp_set_rx_precoder,
    .hal_get_tx_precoder = bh_get_tx_precoder,
    .hal_get_hw_tx_precoder = condor_lp_get_tx_precoder,
    .hal_get_rx_precoder = condor_lp_get_rx_precoder,

    /* MSB/LSB swapping */
    .hal_set_tx_msb = condor_lp_set_tx_msb,
    .hal_set_rx_msb = condor_lp_set_rx_msb,
    .hal_get_tx_msb = condor_lp_get_tx_msb,
    .hal_get_rx_msb = condor_lp_get_rx_msb,

    /* PLL calibration */
    .hal_set_tx_cap = condor_lp_set_tx_cap,
    .hal_set_rx_cap = condor_lp_set_rx_cap,
    .hal_get_tx_cap = condor_lp_get_tx_cap,
    .hal_get_rx_cap = condor_lp_get_rx_cap,

    /* RX detail information */
    .hal_get_rx_ppm = bh_get_rx_ppm,
    .hal_get_rx_skef = condor_lp_get_rx_skef,
    .hal_get_rx_dac = condor_lp_get_rx_dac,
    .hal_get_rx_attenuator = condor_lp_get_rx_atten,
    .hal_get_ffe_taps = condor_lp_get_rx_ffe,
    .hal_get_ffe_taps_fine = condor_lp_get_rx_ffe_fine,
    .hal_get_f1over3 = condor_lp_get_f1over3,
    .hal_get_agcgain_count = condor_lp_get_rx_agcgain_count,
    .hal_get_agcgain = condor_lp_get_agcgain,
    .hal_get_ctle_count = condor_lp_get_rx_ctle_count,
    .hal_get_ctle = condor_lp_get_rx_ctle,
    .hal_get_delta_phase = condor_lp_get_rx_delta,
    .hal_get_edge = condor_lp_get_edge,
    .hal_get_dfe = condor_lp_get_rx_dfe,
    .hal_get_eye = condor_lp_get_rx_eye,
    .hal_get_lane_ready = condor_lp_get_rx_lane_ready,
    .hal_get_signal_detect = condor_lp_get_rx_sd,

    /* RX debugging -- not to be used lightly */
    .hal_set_rx_skef = condor_lp_set_rx_skef,
    .hal_set_rx_dac = condor_lp_set_rx_dac,
    .hal_set_rx_attenuator = condor_lp_set_rx_atten,
    .hal_set_ffe_taps = condor_lp_set_rx_ffe,
    .hal_set_ffe_taps_fine = condor_lp_set_rx_ffe_fine,
    .hal_set_f1over3 = condor_lp_set_f1over3,
    .hal_set_agcgain = condor_lp_set_agcgain,
    .hal_set_ctle = condor_lp_set_rx_ctle,
    .hal_set_delta_phase = condor_lp_set_rx_delta,
    .hal_set_edge = condor_lp_set_edge,

    /* PRBS related functions */
    .hal_get_rx_prbs = condor_lp_get_rx_prbs,
    .hal_get_tx_prbs = condor_lp_get_tx_prbs,
    .hal_set_tx_prbs = bh_set_tx_prbs,
    .hal_set_rx_prbs = bh_set_rx_prbs,

    .hal_get_rx_prbs_count = condor_lp_get_rx_prbs_error_count,
    .hal_get_rx_prbs_ber = common_get_rx_prbs_ber,
    .hal_prbs_get_rx_ber_all = common_get_rx_prbs_ber_all,
    .hal_reset_rx_prbs_count = condor_lp_rx_prbs_rst,
    .hal_prbs_get_rx_autosync = condor_lp_get_prbs_rx_autosync,
    .hal_generate_tx_prbs_error = condor_lp_prbs_gen_1error,
    .hal_get_rx_prbs_duration = common_get_rx_prbs_duration,
    .hal_get_rx_prbs_lock = condor_lp_get_rx_prbs_lock,

    /* TX control */
    .hal_get_tx_test_pattern_enable = condor_lp_get_tx_test_pattern_enable,
    .hal_set_tx_test_pattern_enable = bh_set_tx_test_pattern_enable,
    .hal_get_tx_test_pattern_memory = condor_lp_get_tx_test_pattern_memory,
    .hal_set_tx_test_pattern_memory = condor_lp_set_tx_test_pattern_memory,
    .hal_get_tx_test_pattern_mode = condor_lp_get_tx_test_pattern_mode,
    .hal_set_tx_test_pattern_mode = condor_lp_set_tx_test_pattern_mode,
    .hal_tx_disable = bh_tx_disable,
    .hal_tx_no_disable = bh_tx_no_disable,
    .hal_lane_tx_status = bh_lane_tx_status,

    /* RX control */
    .hal_rx_disable = bh_rx_disable,
    .hal_rx_no_disable = bh_rx_no_disable,

    /* Resets -- be careful */
    .hal_lane_rx_reset = condor_lp_lane_rx_reset,
    .hal_soft_reset = common_soft_reset,
    .hal_logic_reset = common_logic_reset,
    .hal_logic_reset_lane = bh_logic_reset_lane,
    .hal_mcu_reset = common_mcu_reset,
    .hal_mcu_reset_hold = common_mcu_reset_hold,
    .hal_reg_reset = common_reg_reset,
    .hal_reg_reset_lane = bh_reg_reset_lane,

    /* TX taps */
    .hal_set_tx_taps_scale = condor_lp_set_tx_taps_scale,
    .hal_set_tx_taps = bh_set_tx_taps,
    .hal_set_tx_taps_extended = condor_lp_set_tx_taps_extended,
    .hal_get_tx_taps_scale = condor_lp_get_tx_taps_scale,
    .hal_get_tx_taps = bh_get_tx_taps,
    .hal_get_tx_taps_extended = condor_lp_get_tx_taps_extended,
    .hal_reset_tx_taps = condor_lp_reset_tx_taps,

    /* FEC analyzer */
    .hal_set_fec_analyzer = bh_fec_analyzer_set_config,
    .hal_get_fec_analyzer = bh_fec_analyzer_get_config,
    .hal_fecana_get_autosync = fec_analyzer_get_autosync,
    .hal_fecana_reset = fec_analyzer_reset,
    .hal_get_fec_analyzer_read_counter = fec_analyzer_get_read_counter,
    .hal_get_fec_analyzer_counter = fec_analyzer_get_counter,
    .hal_set_fec_analyzer_hist_group = fec_analyzer_set_hist_group,
    .hal_fecana_get_hist_group = fec_analyzer_get_hist_group,
    .hal_get_fec_analyzer_hist_counter = fec_analyzer_get_hist_counter,
    .hal_get_fec_analyzer_duration = fec_analyzer_get_duration,
    .hal_get_fec_analyzer_error_rate = fec_analyzer_get_error_rate,

    /* Auto Neg */
    .hal_set_autoneg_pages = bh_set_autoneg_pages,
    .hal_get_autoneg_exchanged_pages = bh_get_autoneg_exchanged_pages,

    /* RS-FEC */
    .hal_get_rsfec_index = bh_fw_get_rsfec_index,
    .hal_get_rsfec_align_status = rsfec_get_align_status,
    .hal_get_rsfec_fifo = bh_rsfec_get_fifo,
    .hal_get_rsfec_lane_mapping = rsfec_get_lane_mapping,
    .hal_get_rsfec_histogram = rsfec_get_histogram,
    .hal_get_rsfec_corrected_codeword = rsfec_get_corrected_codeword,
    .hal_get_rsfec_uncorrected_codeword = bh_rsfec_get_uncorrected_codeword,
    .hal_get_rsfec_symbol_error = rsfec_get_symbol_error,
    .hal_get_rsfec_total_codeword = bh_rsfec_get_total_codewords,
    .hal_get_rsfec_corrected_bits = rsfec_get_corrected_bits,
    .hal_get_rsfec_count_freeze = rsfec_get_count_freeze,
    .hal_set_rsfec_count_freeze = bh_rsfec_set_count_freeze,
    .hal_reset_rsfec_count = rsfec_reset_count,

    // serdes_param
    .hal_find_param_def = param_find_param_def,
    .hal_param_get_data = param_get_data,
    .hal_param_set_data = param_set_data,
    .hal_get_param_count = param_get_param_count,
    .hal_get_param_val_count = param_get_param_val_count,
    .hal_get_param_val_set_count = param_get_param_val_set_count,
    .hal_index_param_def = param_index_param_def,

    .hal_get_option = option_get_option,
    .hal_set_option = option_set_option,
    .hal_get_option_count = option_get_count,
    .hal_get_option_definition = option_get_def,

    .hal_serdes_preset_tx_taps = bh_serdes_preset_tx_taps,

    .hal_data_capture = bh_data_capture,
    .hal_data_get_commands = bh_data_get_commands,

    .hal_addr_stringify = common_addr_stringify,

    .hal_efuse_read = common_efuse_read_bank,
    .hal_efuse_read_ecid = common_efuse_read_ecid,

    .hal_prbs_pattern_rx_set_prev = condor_lp_set_rx_prbs_prev,
    .hal_prbs_pattern_rx_reset_count = condor_lp_reset_rx_prbs_pattern_count,
    .hal_prbs_pattern_rx_get_count = condor_lp_get_rx_prbs_pattern_count,
    .hal_prbs_pattern_rx_set_phase = condor_lp_set_rx_prbs_pattern_phase,
};

static const SliceOption_t bh_option_list[] = {
    {"fw_freeze", "freeze/un-freeze firmware", common_option_set_fw_freeze, common_option_get_fw_freeze},
    {"fw_cmd_timeout", "default is 10000 us, set 0 to default. This value will not be reset after slice init.",
     common_option_set_fw_cmd_timeout, common_option_get_fw_cmd_timeout},
    {"fw_unload_timeout", "default is 100000 us, set 0 to default. This value will not be reset after slice init.",
     common_option_set_fw_unload_timeout, common_option_get_fw_unload_timeout},
    {"top_cal_timeout", "default is 150000 us, set 0 to default. This value will not be reset after slice init.",
     common_option_set_top_cal_timeout, common_option_get_top_cal_timeout},
    {"sd_delay_optical_nrz",
     "signal delay timeout for nrz optical (unit: milliseconds). This option is persistent across port/lane "
     "reconfigure.",
     bh_option_set_sd_delay, bh_option_get_sd_delay},
    {"sd_delay_nrz",
     "signal delay timeout for nrz electrical (unit: milliseconds). This option is persistent across port/lane "
     "reconfigure.",
     bh_option_set_sd_delay, bh_option_get_sd_delay},
    {"sd_delay_optical_pam4",
     "signal delay timeout for pam4 optical (unit: milliseconds). This option is persistent across port/lane "
     "reconfigure.",
     bh_option_set_sd_delay, bh_option_get_sd_delay},
    {"sd_delay_pam4",
     "signal delay timeout for pam4 electrical (unit: milliseconds). This option is persistent across port/lane "
     "reconfigure.",
     bh_option_set_sd_delay, bh_option_get_sd_delay},
    {"clock_output_squelch",
     "default is 0 (off), only works in port based mode. 0b1 is just clock 0, 0b11 is clock 0,1, 0b111 is clock 0,1,2",
     bh_option_set_clk_output_squelch, bh_option_get_clk_output_squelch},
    {"fw_fifo_healing", "Enable/Disable FIFO healing under background mode", bh_option_set_fifo_auto_recover,
     bh_option_get_fifo_auto_recover},
    {"fw_anlt_power_saving", "Enable/Disable ANLT power saving", bh_option_set_anlt_power_saving,
     bh_option_get_anlt_power_saving},
    {"fw_rsfec_clkgate", "Enable/Disable advanced fec corrected bits reading", bh_option_set_fec_adv_read_info,
     bh_option_get_fec_adv_read_info},
    {"fw_low_latency_1G", "Enable/Disable low latency 1G (20G oversampling), lane bit mask",
     bh_option_set_low_latency_1G, bh_option_get_low_latency_1G},
    {"spare0", "scratch register 0", bh_option_set_scratch, bh_option_get_scratch},
    {"spare1", "scratch register 1", bh_option_set_scratch, bh_option_get_scratch},
    {"spare2", "scratch register 2", bh_option_set_scratch, bh_option_get_scratch},
    {"spare3", "scratch register 3", bh_option_set_scratch, bh_option_get_scratch},
    {"gb_gen_traffic",
     "Enable debug/utility gearbox FEC traffic generation on the A-side lanes. B side must not be connected.",
     bh_option_set_gearbox_traffic_gen, bh_option_get_gearbox_traffic_gen},
    {"fw_uncorr_monitor", "Enable/Disable firmware uncorrectable monitor. Must be set after gearbox port configured.",
     bh_option_set_uncorr_monitor, bh_option_get_uncorr_monitor},
    {"acfg_off", "set 1 to disable auto config after firmware startup", bh_option_set_acfg_off, bh_option_get_acfg_off},
};

static const LaneOption_t bh_lane_option_list[] = {
    {"optical_mode", "This option is used for debug purpose. One should use port flag to assign optical mode.",
     bh_lane_option_set_optical_mode, bh_lane_option_get_optical_mode},
    {"fast_recover_timeout",
     "Maximum fast link recover timeout (unit: milliseconds). If the incoming signal is lost "
     "and recover within this timeout, It will try to recover using the fast path. Otherwise the SerDes will do a full "
     "restart. Each lane in the port should have the same timeout value."
     "Set timeout to 0 to disable. Reset to 0 when port/lane is reconfigured.",
     bh_lane_option_set_generic, bh_lane_option_get_generic},
    {"sd_delay",
     "signal delay timeout (unit: milliseconds), priority higher than slice's sd_delay. This option is persistent "
     "across port/lane reconfigure.",
     bh_lane_option_set_generic, bh_lane_option_get_generic},
};

const SliceCapability_t bh_slice = {
    .slice_type = CREDO_BLACKHAWK,
    .port_count = BH_MAX_PORT,
    .lane_count = 16,
    .vsensor_count = BH_MAX_VSENSOR,
    .hal_func_table = &bh_hal_func_table,
    .option_count = sizeof(bh_option_list) / sizeof(SliceOption_t),
    .option_list = bh_option_list,
    .lane_option_count = sizeof(bh_lane_option_list) / sizeof(LaneOption_t),
    .lane_option_list = bh_lane_option_list,
};

const DeviceCapability_t bh_400 = {
    .device_type = CREDO_BLACKHAWK_400,
    .slice_count = 1,
    /* No disabled lanes */
    .slice_capability = &bh_slice,
};

const DeviceCapability_t bh_800 = {
    .device_type = CREDO_BLACKHAWK_800,
    .slice_count = 2,
    /* No disabled lanes */
    .slice_capability = &bh_slice,
};

void hal_register_sdk(void) {
    cr_register_device(&bh_400);
    cr_register_device(&bh_800);
}
