#ifndef HAL_FUNC_TABLE_H
#define HAL_FUNC_TABLE_H

/* This table is mandatory. */

#include "credo/debug/eip.h"
#include "credo/debug/lane.h"
#include "credo/debug/sram.h"
#include "sdk/types.h"

#include "crintl/driver.h"
#include "crintl/serdes.h"
#include "crintl/tcm.h"

#include <stdio.h>

// NOTE: Please add additional hal functions to end
struct HalFuncTable {
    // Device & Slice Management
    CredoError_t (*hal_allocate_slice)(CredoSdk_t* sdk, CredoSlice_t** slice_handle);
    void (*hal_free_slice)(CredoSlice_t* slice);
    CredoError_t (*hal_get_base_address)(CredoSlice_t* slice, const RegHive_t* hive, unsigned* base_addr);
    CredoError_t (*hal_get_hive_index)(CredoSlice_t* slice, unsigned* hive_index);
    CredoError_t (*hal_slice_init)(CredoSlice_t* slice, const CredoSliceConfig_t* config);

    // Slice Info
    CredoError_t (*hal_slice_get_oui)(CredoSlice_t* slice, unsigned* oui);
    CredoError_t (*hal_slice_get_model_number)(CredoSlice_t* slice, unsigned* model_number);
    CredoError_t (*hal_slice_get_revision_number)(CredoSlice_t* slice, unsigned* revision_number);
    CredoError_t (*hal_slice_get_type)(CredoSlice_t* slice, CredoSliceType_t* slice_type);
    CredoError_t (*hal_slice_get_vsensor)(CredoSlice_t* slice, double* vsensor);
    CredoError_t (*hal_slice_get_vsensor_ex)(CredoSlice_t* slice, unsigned sel_vin, double* vsensor);
    CredoError_t (*hal_slice_get_sram_status)(CredoSlice_t* slice, CredoSramStatus_t* sram_status);
    CredoError_t (*hal_slice_generate_sram_error)(CredoSlice_t* slice, CredoSramStatus_t sram_status);

    // Slice Operations
    CredoError_t (*hal_slice_load_setup)(CredoSlice_t* slice, const char* file_path);
    CredoError_t (*hal_slice_save_setup)(CredoSlice_t* slice, const char* file_path);

    CredoError_t (*hal_slice_get_reghive)(CredoSlice_t* slice, const char* hivename, const RegHive_t** reghive);

    // Clock Output
    CredoError_t (*hal_slice_enable_clock_output)(CredoSlice_t* slice, unsigned clock_output, unsigned lane,
                                                  unsigned divider);
    CredoError_t (*hal_slice_disable_all_clock_output)(CredoSlice_t* slice);
    CredoError_t (*hal_slice_disable_clock_output)(CredoSlice_t* slice, unsigned clock_output);

    // Information
    CredoError_t (*hal_display_slice_info)(CredoSlice_t* slice, int argc, const char* argv[]);

    // Firmware Management
    CredoError_t (*hal_fw_unload)(CredoSlice_t* slice);
    CredoError_t (*hal_fw_load)(CredoSlice_t* slice, FILE* file);
    CredoError_t (*hal_fw_load_broadcast)(CredoSlice_t* slices[], int slice_count, FILE* file, unsigned delay_time_us);
    // Firmware SPI Operations
    CredoError_t (*hal_fw_load_spi)(CredoSlice_t* slice, int partition_num);
    CredoError_t (*hal_fw_spiflash_status)(CredoSlice_t* slice, unsigned* response);
    CredoError_t (*hal_fw_spiflash_read)(CredoSlice_t* slice, unsigned addr, unsigned* buffer, unsigned count);
    CredoError_t (*hal_fw_spiflash_write)(CredoSlice_t* slice, unsigned addr, const unsigned* buffer, unsigned count);
    CredoError_t (*hal_fw_spiflash_erase)(CredoSlice_t* slice, unsigned addr);
    CredoError_t (*hal_fw_spiflash_display_mbr)(CredoSlice_t* slice);
    CredoError_t (*hal_fw_spiflash_format_mbr)(CredoSlice_t* slice, unsigned flash_kb_size, int force);
    CredoError_t (*hal_fw_spiflash_read_firmware)(CredoSlice_t* slice, const char* fwname, int partition_num);
    CredoError_t (*hal_fw_spiflash_write_firmware)(CredoSlice_t* slice, const char* fwname, int partition_num,
                                                   int force);

    CredoError_t (*hal_fw_wait_magic_word)(CredoSlice_t* slice, unsigned timeout);
    CredoError_t (*hal_fw_wait_top_pll_cal)(CredoSlice_t* slice, unsigned timeout);
    CredoError_t (*hal_fw_get_status)(CredoSlice_t* slice, unsigned* status);

    // Firmware Information
    CredoError_t (*hal_fw_magic)(CredoSlice_t* slice, unsigned* magic);
    CredoError_t (*hal_fw_ver)(CredoSlice_t* slice, unsigned* version);
    CredoError_t (*hal_fw_hash)(CredoSlice_t* slice, unsigned* hash);
    CredoError_t (*hal_fw_crc)(CredoSlice_t* slice, unsigned* crc);
    CredoError_t (*hal_fw_date)(CredoSlice_t* slice, unsigned* date);

    // Firmware Commands
    CredoError_t (*hal_fw_cmd)(CredoSlice_t* slice, unsigned cmd, unsigned params, unsigned* response,
                               unsigned* response_param);
    CredoError_t (*hal_fw_cmd_ex)(CredoSlice_t* slice, unsigned cmd, unsigned param1, unsigned param2,
                                  unsigned* response, unsigned* response_param1, unsigned* response_param2);
    CredoError_t (*hal_fw_debug_cmd)(CredoSlice_t* slice, unsigned lane, unsigned section, unsigned index,
                                     unsigned* value);
    CredoError_t (*hal_fw_debug_cmd_ex)(CredoSlice_t* slice, unsigned lane, unsigned section, unsigned index,
                                        unsigned* response1, unsigned* response2);

    // Firmware Registers
    CredoError_t (*hal_fw_reg_rd)(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned* value);
    CredoError_t (*hal_fw_reg_wr)(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned value);
    CredoError_t (*hal_fw_reg_rd_internal)(CredoSlice_t* slice, unsigned combined_addr, unsigned* value);
    CredoError_t (*hal_fw_reg_wr_internal)(CredoSlice_t* slice, unsigned combined_addr, unsigned value);

    // Port Operations
    CredoError_t (*hal_fw_config_port)(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force);
    CredoError_t (*hal_fw_teardown_port)(CredoSlice_t* slice, uint32_t port_id);
    CredoError_t (*hal_fw_query_port)(CredoSlice_t* slice, uint32_t portId, CredoPortConfig_t* port_config);
    CredoError_t (*hal_fw_clear_all_port)(CredoSlice_t* slice);

    // Slice Operations
    CredoError_t (*hal_fw_get_slice_temp)(CredoSlice_t* slice, double* temp);

    // Lane Operations
    CredoError_t (*hal_fw_config_lane)(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed);
    CredoError_t (*hal_fw_config_lane_loopback)(CredoSlice_t* slice, int lane, uint32_t speed);
    CredoError_t (*hal_fw_deconfig_lane)(CredoSlice_t* slice, int lane);

    // Serdes Operations
    CredoError_t (*hal_fw_phy_ready)(CredoSlice_t* slice, unsigned* rdy);
    CredoError_t (*hal_fw_phy_lane_ready)(CredoSlice_t* slice, int lane, unsigned* rdy);
    CredoError_t (*hal_fw_get_adapt_count)(CredoSlice_t* slice, int lane, unsigned* count);
    CredoError_t (*hal_fw_get_readapt_count)(CredoSlice_t* slice, int lane, unsigned* count);
    CredoError_t (*hal_fw_get_link_lost_count)(CredoSlice_t* slice, int lane, unsigned* count);
    CredoError_t (*hal_fw_get_los_count)(CredoSlice_t* slice, int lane, unsigned* count);
    CredoError_t (*hal_fw_get_channel_estimate)(CredoSlice_t* slice, int lane, double* chan_est);
    CredoError_t (*hal_fw_get_of)(CredoSlice_t* slice, int lane, unsigned* of);
    CredoError_t (*hal_fw_get_hf)(CredoSlice_t* slice, int lane, unsigned* hf);
    CredoError_t (*hal_fw_get_eye)(CredoSlice_t* slice, int lane, int eyes[3]);
    CredoError_t (*hal_fw_get_isi)(CredoSlice_t* slice, int lane, int isi[]);
    CredoError_t (*hal_fw_get_rx_ffe)(CredoSlice_t* slice, int lane, int taps[]);
    CredoError_t (*hal_fw_get_rx_ffe_nbias)(CredoSlice_t* slice, int lane, int nbias[]);
    CredoError_t (*hal_fw_get_rx_ffe_kaccu)(CredoSlice_t* slice, int lane, double kaccu[]);
    CredoError_t (*hal_fw_get_rx_ffe_weighting_table)(CredoSlice_t* slice, int lane, double** wt_table);
    CredoError_t (*hal_fw_get_rx_ffe_flip_counter)(CredoSlice_t* slice, int lane, int flip_counter[]);
    CredoError_t (*hal_fw_get_lane_speed)(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);
    CredoError_t (*hal_fw_eye_monitor_start)(CredoSlice_t* slice, int lane, int ber_exp, int flag);
    CredoError_t (*hal_fw_eye_monitor_stop)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_fw_eye_monitor_progress)(CredoSlice_t* slice, int lane, int* percent);
    CredoError_t (*hal_fw_eye_monitor_data)(CredoSlice_t* slice, int lane, int** data, int* extent_mv);
    CredoError_t (*hal_fw_eye_monitor_range)(CredoSlice_t* slice, int lane, int* vstep_full, int* hstep_full);
    CredoError_t (*hal_fw_eye_monitor_separator)(CredoSlice_t* slice, int separator[5]);

    // Top Level
    CredoError_t (*hal_set_lane_config)(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config);
    CredoError_t (*hal_get_lane_config)(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config);

    // Lane Loopback
    CredoError_t (*hal_set_lane_loopback_mode)(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode);

    // Lane Mode
    CredoError_t (*hal_get_lane_mode)(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);
    CredoError_t (*hal_set_lane_mode)(CredoSlice_t* slice, int lane, CredoLaneMode_t mode);
    CredoError_t (*hal_update_lane_mode)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_disable_lane)(CredoSlice_t* slice, int lane);

    // Top PLL
    CredoError_t (*hal_top_pll_cal)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_top_pll_init)(CredoSlice_t* slice);
    CredoError_t (*hal_get_top_pll_cap)(CredoSlice_t* slice, unsigned* cap);

    // Capability
    CredoError_t (*hal_get_lane_count)(CredoSlice_t* slice, int* host_lane, int* line_lane);
    CredoError_t (*hal_get_rx_ffe_range)(CredoSlice_t* slice, int lane, int* taps_len, int* sum_len);
    CredoError_t (*hal_get_rx_ffe_weighting_table_range)(CredoSlice_t* slice, int lane, int* row, int* col);
    CredoError_t (*hal_get_tx_ffe_range)(CredoSlice_t* slice, int lane, int* length, int* extended_length);
    CredoError_t (*hal_get_rx_dfe_range)(CredoSlice_t* slice, int lane, int* length);
    CredoError_t (*hal_get_rx_isi_range)(CredoSlice_t* slice, int lane, int* length);

    // Polarity
    CredoError_t (*hal_set_rx_polarity)(CredoSlice_t* slice, int lane, int rx_pol);
    CredoError_t (*hal_set_tx_polarity)(CredoSlice_t* slice, int lane, int tx_pol);
    CredoError_t (*hal_get_rx_polarity)(CredoSlice_t* slice, int lane, int* rx_pol);
    CredoError_t (*hal_get_tx_polarity)(CredoSlice_t* slice, int lane, int* tx_pol);

    // Input Mode
    CredoError_t (*hal_set_rx_input_mode)(CredoSlice_t* slice, int lane, CredoLaneCoupling_t input_mode);
    CredoError_t (*hal_get_rx_input_mode)(CredoSlice_t* slice, int lane, CredoLaneCoupling_t* input_mode);

    // Gray code and precoding
    CredoError_t (*hal_set_tx_gray_code)(CredoSlice_t* slice, int lane, int tx_gc);
    CredoError_t (*hal_set_rx_gray_code)(CredoSlice_t* slice, int lane, int rx_gc);
    CredoError_t (*hal_get_tx_gray_code)(CredoSlice_t* slice, int lane, int* tx_gc);
    CredoError_t (*hal_get_rx_gray_code)(CredoSlice_t* slice, int lane, int* rx_gc);
    CredoError_t (*hal_set_tx_precoder)(CredoSlice_t* slice, int lane, int tx_pc);
    CredoError_t (*hal_set_rx_precoder)(CredoSlice_t* slice, int lane, int rx_pc);
    CredoError_t (*hal_get_tx_precoder)(CredoSlice_t* slice, int lane, int* tx_pc);
    CredoError_t (*hal_get_rx_precoder)(CredoSlice_t* slice, int lane, int* rx_pc);

    // Bit Swapping
    CredoError_t (*hal_set_tx_msb)(CredoSlice_t* slice, int lane, int tx_msb);
    CredoError_t (*hal_set_rx_msb)(CredoSlice_t* slice, int lane, int rx_msb);
    CredoError_t (*hal_get_tx_msb)(CredoSlice_t* slice, int lane, int* tx_msb);
    CredoError_t (*hal_get_rx_msb)(CredoSlice_t* slice, int lane, int* rx_msb);

    // Lane PLL
    CredoError_t (*hal_set_tx_cap)(CredoSlice_t* slice, int lane, int tx_cap);
    CredoError_t (*hal_set_rx_cap)(CredoSlice_t* slice, int lane, int rx_cap);
    CredoError_t (*hal_get_tx_cap)(CredoSlice_t* slice, int lane, int* tx_cap);
    CredoError_t (*hal_get_rx_cap)(CredoSlice_t* slice, int lane, int* rx_cap);

    // RX detail
    CredoError_t (*hal_get_rx_ppm)(CredoSlice_t* slice, int lane, int* ppm);
    CredoError_t (*hal_get_rx_skef)(CredoSlice_t* slice, int lane, int* enable, int* degen, int* addcap,
                                    int* skef_gain);
    CredoError_t (*hal_get_rx_dac)(CredoSlice_t* slice, int lane, int* rx_dac);
    CredoError_t (*hal_get_rx_attenuator)(CredoSlice_t* slice, int lane, int* passive, int* gain, int* termtune);
    CredoError_t (*hal_get_ffe_taps)(CredoSlice_t* slice, int lane, int taps[]);
    CredoError_t (*hal_get_ffe_taps_fine)(CredoSlice_t* slice, int lane, int taps[]);
    CredoError_t (*hal_get_f1over3)(CredoSlice_t* slice, int lane, int* value);
    CredoError_t (*hal_get_agcgain_count)(CredoSlice_t* slice, int lane, unsigned* count);
    CredoError_t (*hal_get_agcgain)(CredoSlice_t* slice, int lane, unsigned agcgain[]);
    CredoError_t (*hal_get_ctle_count)(CredoSlice_t* slice, int lane, unsigned* count);
    CredoError_t (*hal_get_ctle)(CredoSlice_t* slice, int lane, unsigned value[]);
    CredoError_t (*hal_get_delta_phase)(CredoSlice_t* slice, int lane, int* value);
    CredoError_t (*hal_get_edge)(CredoSlice_t* slice, int lane, unsigned* value);
    CredoError_t (*hal_get_dfe)(CredoSlice_t* slice, int lane, double dfe_taps[]);
    CredoError_t (*hal_get_eye)(CredoSlice_t* slice, int lane, int eyes[3]);
    CredoError_t (*hal_get_lane_ready)(CredoSlice_t* slice, int lane, int* ready);
    CredoError_t (*hal_get_signal_detect)(CredoSlice_t* slice, int lane, int* sd);

    // RX debugging
    CredoError_t (*hal_set_rx_skef)(CredoSlice_t* slice, int lane, int enable, int degen, int addcap, int skef_gain);
    CredoError_t (*hal_set_rx_dac)(CredoSlice_t* slice, int lane, int rx_dac);
    CredoError_t (*hal_set_rx_attenuator)(CredoSlice_t* slice, int lane, int passive, int gain, int termtune);
    CredoError_t (*hal_set_ffe_taps)(CredoSlice_t* slice, int lane, const int taps[]);
    CredoError_t (*hal_set_ffe_taps_fine)(CredoSlice_t* slice, int lane, const int taps[]);
    CredoError_t (*hal_set_f1over3)(CredoSlice_t* slice, int lane, int value);
    CredoError_t (*hal_set_agcgain)(CredoSlice_t* slice, int lane, unsigned value[]);
    CredoError_t (*hal_set_ctle)(CredoSlice_t* slice, int lane, unsigned ctle[]);
    CredoError_t (*hal_set_delta_phase)(CredoSlice_t* slice, int lane, int value);
    CredoError_t (*hal_set_edge)(CredoSlice_t* slice, int lane, unsigned value);

    // PRBS
    CredoError_t (*hal_get_tx_prbs)(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode);
    CredoError_t (*hal_get_rx_prbs)(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode);
    CredoError_t (*hal_set_tx_prbs)(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
    CredoError_t (*hal_set_rx_prbs_nrz)(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
    CredoError_t (*hal_set_rx_prbs_pam4)(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
    CredoError_t (*hal_set_tx_prbs_nrz)(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
    CredoError_t (*hal_set_tx_prbs_pam4)(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
    CredoError_t (*hal_set_rx_prbs)(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
    CredoError_t (*hal_get_rx_prbs_count)(CredoSlice_t* slice, int lane, uint32_t* count);
    CredoError_t (*hal_get_rx_prbs_ber)(CredoSlice_t* slice, int lane, int time_ms, double* ber);
    CredoError_t (*hal_reset_rx_prbs_count)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_generate_tx_prbs_error)(CredoSlice_t* slice, int lane);

    // Test pattern
    CredoError_t (*hal_get_tx_test_pattern_enable)(CredoSlice_t* slice, int lane, bool* enable);
    CredoError_t (*hal_set_tx_test_pattern_enable)(CredoSlice_t* slice, int lane, bool enable);
    CredoError_t (*hal_get_tx_test_pattern_memory)(CredoSlice_t* slice, int lane, uint64_t* pattern);
    CredoError_t (*hal_set_tx_test_pattern_memory)(CredoSlice_t* slice, int lane, uint64_t pattern);
    CredoError_t (*hal_get_tx_test_pattern_mode)(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode* mode);
    CredoError_t (*hal_set_tx_test_pattern_mode)(CredoSlice_t* slice, int lane, CredoLaneTxTestPatternMode mode);

    // TX control
    CredoError_t (*hal_tx_disable)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_tx_no_disable)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_lane_tx_status)(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status);

    // RX control
    CredoError_t (*hal_rx_disable)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_rx_no_disable)(CredoSlice_t* slice, int lane);

    // Resets -- be careful
    CredoError_t (*hal_lane_rx_reset)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_soft_reset)(CredoSlice_t* slice);
    CredoError_t (*hal_logic_reset)(CredoSlice_t* slice);
    CredoError_t (*hal_logic_reset_lane)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_mcu_reset)(CredoSlice_t* slice);
    CredoError_t (*hal_mcu_reset_hold)(CredoSlice_t* slice);
    CredoError_t (*hal_reg_reset)(CredoSlice_t* slice);
    CredoError_t (*hal_reg_reset_lane)(CredoSlice_t* slice, int lane);

    // TX taps
    CredoError_t (*hal_set_tx_taps_scale)(CredoSlice_t* slice, int lane, const unsigned taps_scale[]);
    CredoError_t (*hal_get_tx_taps_scale)(CredoSlice_t* slice, int lane, unsigned taps_scale[]);
    CredoError_t (*hal_set_tx_taps)(CredoSlice_t* slice, int lane, const int taps[]);
    CredoError_t (*hal_set_tx_taps_extended)(CredoSlice_t* slice, int lane, const int taps_extended[]);
    CredoError_t (*hal_get_tx_taps)(CredoSlice_t* slice, int lane, int taps[]);
    CredoError_t (*hal_get_tx_taps_extended)(CredoSlice_t* slice, int lane, int taps_extended[]);
    CredoError_t (*hal_reset_tx_taps)(CredoSlice_t* slice, int lane);

    // FEC analyzer
    CredoError_t (*hal_set_fec_analyzer)(CredoSlice_t* slice, int lane, int enable, CredoFecAnalyzerConfig_t* config);
    CredoError_t (*hal_get_fec_analyzer)(CredoSlice_t* slice, int lane, int* enable, CredoFecAnalyzerConfig_t* config);
    CredoError_t (*hal_get_fec_analyzer_read_counter)(CredoSlice_t* slice, int lane, int counter_sel,
                                                      unsigned* counter);
    CredoError_t (*hal_get_fec_analyzer_counter)(CredoSlice_t* slice, int lane, unsigned* pre_fec, unsigned* post_fec);
    CredoError_t (*hal_set_fec_analyzer_hist_group)(CredoSlice_t* slice, int lane, int group);
    CredoError_t (*hal_get_fec_analyzer_hist_counter)(CredoSlice_t* slice, int lane, unsigned hist_data[4]);
    CredoError_t (*hal_get_fec_analyzer_duration)(CredoSlice_t* slice, int lane, unsigned long* duration_ms);
    CredoError_t (*hal_get_fec_analyzer_error_rate)(CredoSlice_t* slice, int lane, int counter_sel, int duration_ms,
                                                    double* error_rate);

    // RS-FEC
    CredoError_t (*hal_get_rsfec_index)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int* index);
    CredoError_t (*hal_get_rsfec_align_status)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                               CredoRSFECStatus_t* rsfec_status);
    CredoError_t (*hal_get_rsfec_fifo)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                       CredoRSFECFifo_t* rsfec_fifo);
    CredoError_t (*hal_get_rsfec_lane_mapping)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                               unsigned* lane_mapping);
    CredoError_t (*hal_get_rsfec_histogram)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int hist_bin,
                                            uint64_t* hist);
    CredoError_t (*hal_get_rsfec_corrected_codeword)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                     uint64_t* corr_cw);
    CredoError_t (*hal_get_rsfec_uncorrected_codeword)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                       unsigned* uncorr_cw);
    CredoError_t (*hal_get_rsfec_symbol_error)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int fec_lane,
                                               unsigned* symbol_error);
    CredoError_t (*hal_get_rsfec_total_codeword)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                 uint64_t* total_bits);
    CredoError_t (*hal_get_rsfec_corrected_bits)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                 uint64_t* corrected_bits);
    CredoError_t (*hal_set_rsfec_count_freeze)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool enable);
    CredoError_t (*hal_get_rsfec_count_freeze)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* enable);
    CredoError_t (*hal_reset_rsfec_count)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side);

    // AutoNeg
    CredoError_t (*hal_set_autoneg_pages)(CredoSlice_t* slice, int lane, int pageId, uint64_t page);
    CredoError_t (*hal_get_autoneg_exchanged_pages)(CredoSlice_t* slice, int lane, int* page_count,
                                                    uint64_t transmitted_pages[9], uint64_t received_pages[9]);

    // TCM
    CredoError_t (*hal_get_tcm_burst_width)(CredoSlice_t* slice, unsigned address, TCMBurstWidth_t* width);
    CredoError_t (*hal_tcm_get_base_address)(CredoSlice_t* slice, const RegHive_t* hive, unsigned* base_addr);

    CredoError_t (*hal_tcm_read)(CredoSlice_t* slice, unsigned addr, unsigned* data);
    CredoError_t (*hal_tcm_write)(CredoSlice_t* slice, unsigned addr, unsigned data);

    CredoError_t (*hal_tcm_burst_read)(CredoSlice_t* slice, unsigned first_address, uint64_t val[], unsigned count);

    CredoError_t (*hal_tcm_burst_write)(CredoSlice_t* slice, unsigned first_address, const uint64_t val[],
                                        unsigned count);

    CredoError_t (*hal_pcs_read)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                 unsigned* val);

    CredoError_t (*hal_pcs_write)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                  unsigned val);
    CredoError_t (*hal_pcs_status_read)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned* pcs_status);

    CredoError_t (*hal_rs_fec_read)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                    unsigned* val);
    CredoError_t (*hal_rs_fec_write)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                     unsigned val);
    CredoError_t (*hal_mac_read)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                 unsigned* val);
    CredoError_t (*hal_mac_write)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                  unsigned val);

    CredoError_t (*hal_mac_status_read)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned* mac_status);
    CredoError_t (*hal_mac_stats_read)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                       unsigned* val);
    CredoError_t (*hal_mac_stats_write)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                        unsigned val);

    CredoError_t (*hal_mac_stats_read_counters)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                                CredoMACStatistics_t* stats);
    CredoError_t (*hal_mac_stats_clear_counters)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side);

    CredoError_t (*hal_mac_stop)(CredoSlice_t* slice, uint32_t portId);
    CredoError_t (*hal_eip_stop)(CredoSlice_t* slice, uint32_t portId);

    CredoError_t (*hal_configure_eip)(CredoSlice_t* slice, uint32_t portId);
    CredoError_t (*hal_configure_mac)(CredoSlice_t* slice, uint32_t portId);

    CredoError_t (*hal_display_topology_types)(CredoSlice_t* slice);

    CredoError_t (*hal_set_topology)(CredoSlice_t* slice, const char* topology);
    CredoError_t (*hal_get_topology)(CredoSlice_t* slice, char* topology, uint32_t len);

    CredoError_t (*hal_fault_propagation_control)(CredoSlice_t* slice, uint32_t portId, CredoFaultPropagation_t enable);
    CredoError_t (*hal_fault_propagation_status)(CredoSlice_t* slice, uint32_t portId, CredoFaultPropagation_t* enable);

    CredoError_t (*hal_enable_low_latency_bypass)(CredoSlice_t* slice, uint32_t port_id,
                                                  CredoMACsecDirection_t direction, uint8_t enable);

    CredoError_t (*hal_low_latency_bypass_status)(CredoSlice_t* slice, uint32_t port_id,
                                                  CredoMACsecDirection_t direction, uint8_t* enable);

    CredoError_t (*hal_get_client_id)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, uint32_t* sys_client);
    CredoError_t (*hal_get_channel_id)(CredoSlice_t* slice, uint32_t port_id, uint32_t* channel_id);

    CredoError_t (*hal_get_macsec_datapath)(CredoSlice_t* slice, CredoMACsecDirection_t direction,
                                            CredoMACsecDataPath_t* datapath);
    // any below this must remove 1 from hal_padding when adding a new hal function
    // this keeps the ability to use older hal with new sdk
    CredoError_t (*hal_get_rx_prbs_duration)(CredoSlice_t* slice, int lane, unsigned long* duration_ms);
    CredoError_t (*hal_slice_set_mdio_mode)(CredoSlice_t* slice, bool is_push_pull);
    CredoError_t (*hal_get_rx_prbs_lock)(CredoSlice_t* slice, int lane, CredoPrbsLockStatus_t* status);
    CredoError_t (*hal_packet_inject)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                      CredoPacketInjectConfig_t* pkt_config);

    void* hal_set_paramlisti;
    void* hal_get_paramlisti;
    void* hal_set_paramlistf;
    void* hal_get_paramlistf;

    CredoError_t (*hal_get_param_count)(CredoSlice_t* slice, ParamDomain_t domain, int* count);
    CredoError_t (*hal_index_param_def)(CredoSlice_t* slice, ParamDomain_t domain, int param_index,
                                        CredoParam_t* param);
    CredoError_t (*hal_find_param_def)(CredoSlice_t* slice, ParamDomain_t domain, const char* name, bool* found,
                                       CredoParam_t* param);
    CredoError_t (*hal_get_param_val_count)(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                            int* count);

    CredoError_t (*hal_get_lane_loopback_mode)(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode);
    CredoError_t (*hal_fw_get_dfe)(CredoSlice_t* slice, int lane, double dfe_taps[]);
    CredoError_t (*hal_fw_get_raw_cmd_address)(CredoSlice_t* slice, unsigned* addr);

    CredoError_t (*hal_get_tx_lane_mode)(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);
    CredoError_t (*hal_set_tx_lane_mode)(CredoSlice_t* slice, int lane, CredoLaneMode_t mode);
    CredoError_t (*hal_fw_get_tx_lane_speed)(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);
    CredoError_t (*hal_fw_atomic_read)(CredoSlice_t* slice, unsigned addr, int len, unsigned* data);

    CredoError_t (*hal_set_option)(CredoSlice_t* slice, OptionDomain_t type, int index, const char* option, int value);
    CredoError_t (*hal_get_option)(CredoSlice_t* slice, OptionDomain_t type, int index, const char* option, int* value);
    CredoError_t (*hal_get_option_count)(CredoSlice_t* slice, OptionDomain_t type, int* count);
    CredoError_t (*hal_get_option_definition)(CredoSlice_t* slice, OptionDomain_t type, int index,
                                              CredoOption_t* option);
    // DO NOT USE hal_fw_info_port
    CredoError_t (*hal_fw_info_port)(CredoSlice_t* slice, uint32_t portId, CredoPortConfig_t* port_config, bool update);
    CredoError_t (*hal_port_is_link_up)(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, bool* up);
    CredoError_t (*hal_get_hw_tx_precoder)(CredoSlice_t* slice, int lane, int* hw_tx_pc);
    CredoError_t (*hal_slice_data_init)(CredoSlice_t* slice);

    CredoError_t (*hal_port_build)(CredoSlice_t* slice, uint32_t port_id, const CredoPortSetup_t* setup);
    CredoError_t (*hal_port_start)(CredoSlice_t* slice, uint32_t port_id, bool force);
    CredoError_t (*hal_port_get_setup)(CredoSlice_t* slice, uint32_t port_id, bool* launched, CredoPortSetup_t* setup);
    CredoError_t (*hal_port_assign_id)(CredoSlice_t* slice, uint32_t* port_id);

    CredoError_t (*hal_serdes_set_tx_taps_preset)(CredoSlice_t* slice, int lane, const char* preset);
    CredoError_t (*hal_tcm_memset)(CredoSlice_t* slice, const CredoTCMBurstIOCtrl_t* io_ctrl, unsigned value);
    CredoError_t (*hal_tcm_range_program)(CredoSlice_t* slice, int index,
                                          const CredoTCMBurstIORangeProgram_t* range_param);
    CredoError_t (*hal_tcm_multi_slice_range_read)(CredoSlice_t* slices[], int slice_count,
                                                   const CredoTCMBurstIORange_t* io_range[],
                                                   CredoTCMBurstIOData_t* data[]);
    CredoError_t (*hal_fw_config_phy)(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed,
                                      uint32_t flags);
    CredoError_t (*hal_fw_deconfig_phy)(CredoSlice_t* slice, int lane, uint32_t flags);
    CredoError_t (*hal_tcm_get_device_id)(CredoSlice_t* slice, uint32_t port_id, TCMCoreId_t core, CredoSide_t side,
                                          CredoTCMDevice_t* tcm_device);
    CredoError_t (*hal_tcm_get_reg_space)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                          CredoTCMRegSpace_t* reg_space);
    CredoError_t (*hal_serdes_preset_tx_taps)(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc);
    CredoError_t (*hal_setup_capture_packet)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                             CredoPacketCaptureConfig_t* pkt_config);
    CredoError_t (*hal_stop_capture_packet)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side);
    CredoError_t (*hal_get_capture_packet_buffer_status)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                         CredoPacketCaptureBufferStatus_t* status);
    CredoError_t (*hal_clear_capture_packet_buffer)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side);
    CredoError_t (*hal_get_capture_packet_info)(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                CredoPacketCaptureInfo_t* pkt_info);
    CredoError_t (*hal_prbs_training_rx_get_status)(CredoSlice_t* slice, int lane, CredoPrbsTrainingStatus_t* status);
    CredoError_t (*hal_prbs_training_rx_relink)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_prbs_training_tx_enable)(CredoSlice_t* slice, int lane, bool enable);
    CredoError_t (*hal_prbs_training_tx_is_enabled)(CredoSlice_t* slice, int lane, bool* enabled);

    CredoError_t (*hal_slice_index_reghive)(CredoSlice_t* slice, int index, const RegHive_t** reghive);
    CredoError_t (*hal_slice_get_reghive_count)(CredoSlice_t* slice, unsigned* count);

    CredoError_t (*hal_prbs_get_rx_ber_all)(CredoSlice_t* slice, const int lanes[], unsigned time_ms, double ber[],
                                            unsigned count);
    CredoError_t (*hal_link_training_get_status)(CredoSlice_t* slice, int lane, CredoLinkTrainingStatus_t* lt_status);
    CredoError_t (*hal_link_training_get_state)(CredoSlice_t* slice, int lane, CredoLinkTrainingState_t* lt_state);

    CredoError_t (*hal_fw_clear_top_pll_cal)(CredoSlice_t* slice);

    CredoError_t (*hal_get_param_val_set_count)(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                                int* count);
    CredoError_t (*hal_autoneg_get_state)(CredoSlice_t* slice, int lane, CredoAutoNegState_t* state);
    CredoError_t (*hal_autoneg_get_restart_count)(CredoSlice_t* slice, int lane, unsigned* count);
    CredoError_t (*hal_link_training_get_restart_count)(CredoSlice_t* slice, int lane, unsigned* count);
    void*(ignore_this_lmap1);
    void*(ignore_this_lmap2);
    CredoError_t (*hal_data_capture)(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[],
                                     size_t argc, CredoDataWriter_t writer, void* ud);
    void* (*hal_data_get_commands)(CredoSlice_t* slice, size_t* command_count);
    CredoError_t (*hal_display_info)(CredoSlice_t* slice, const char* argv[], size_t argc, CredoDisplayWriter_t writer,
                                     void* ud);

    CredoError_t (*hal_frecov_configure)(CredoSlice_t* slice, int lane, unsigned timeout_ms);
    CredoError_t (*hal_frecov_get_status)(CredoSlice_t* slice, int lane, unsigned* timeout_ms,
                                          CredoFastRecoveryStatus_t* status);
    CredoError_t (*hal_frecov_get_recover_count)(CredoSlice_t* slice, int lane, unsigned* count);
    CredoError_t (*hal_fecana_reset)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_fecana_get_autosync)(CredoSlice_t* slice, int lane, bool* enable);
    CredoError_t (*hal_prbs_get_rx_autosync)(CredoSlice_t* slice, int lane, bool* enable);

    CredoError_t (*hal_fw_spiflash_set_partition)(CredoSlice_t* slice, int partition_num);

    CredoError_t (*hal_param_get_data)(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                       CredoParamData_t* data);
    CredoError_t (*hal_param_set_data)(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                       const CredoParamData_t* data);
    const char* (*hal_addr_stringify)(CredoSlice_t* slice, int address);

    CredoError_t (*hal_testpoint_select)(CredoSlice_t* slice, const CredoTestPoint_t* testpoint);
    CredoError_t (*hal_testpoint_clear)(CredoSlice_t* slice);
    CredoError_t (*hal_testpoint_read)(CredoSlice_t* slice, double* value);

    CredoError_t (*hal_enable_lane)(CredoSlice_t* slice, int lane);

    CredoError_t (*hal_time_start)(CredoSlice_t* slice, double* unix_time);
    CredoError_t (*hal_time_system)(CredoSlice_t* slice, double* timedelta);

    CredoError_t (*hal_fecana_get_hist_group)(CredoSlice_t* slice, int lane, int* group);

    CredoError_t (*hal_efuse_read)(CredoSlice_t* slice, int bank, uint32_t* val);
    CredoError_t (*hal_efuse_read_ecid)(CredoSlice_t* slice, uint32_t ecid[2]);

    CredoError_t (*hal_prbs_pattern_rx_set_prev)(CredoSlice_t* slice, int lane, bool en, unsigned prev);
    CredoError_t (*hal_prbs_pattern_rx_reset_count)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_prbs_pattern_rx_get_count)(CredoSlice_t* slice, int lane, unsigned pattern_count[12]);
    CredoError_t (*hal_prbs_pattern_rx_set_phase)(CredoSlice_t* slice, int lane, unsigned phase);

    CredoError_t (*hal_prbs_set_tx_checker)(CredoSlice_t* slice, int lane, int en, CredoPrbsPattern_t pattern);
    CredoError_t (*hal_prbs_get_tx_checker)(CredoSlice_t* slice, int lane, int* en, CredoPrbsPattern_t* pattern);
    CredoError_t (*hal_prbs_reset_tx_count)(CredoSlice_t* slice, int lane);
    CredoError_t (*hal_prbs_get_tx_count)(CredoSlice_t* slice, int lane, unsigned tx_count[2]);
    CredoError_t (*hal_prbs_get_tx_duration)(CredoSlice_t* slice, int lane, unsigned long* tx_duration);
    CredoError_t (*hal_prbs_get_tx_ber)(CredoSlice_t* slice, int lane, unsigned time_ms, double tx_ber[2]);
    CredoError_t (*hal_prbs_get_tx_ber_all)(CredoSlice_t* slice, const int lanes[], unsigned time_ms,
                                            double tx_ber[][2], unsigned count);

    CredoError_t (*hal_fw_ready_post_actions)(CredoSlice_t* slice);

    CredoError_t (*hal_reg_get_notepad)(CredoSlice_t* slice, unsigned* notepad_register);

    void* hal_padding[426];
};

#endif
