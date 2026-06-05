#ifndef BH_FUNCTIONS_H
#define BH_FUNCTIONS_H

#include "bh_device.h"
#include "bh_fw_helper.h"

CredoError_t bh_slice_enable_clock_output(CredoSlice_t* slice, unsigned clock_output, unsigned lane, unsigned divider);
CredoError_t bh_slice_disable_all_clock_output(CredoSlice_t* slice);
CredoError_t bh_slice_disable_clock_output(CredoSlice_t* slice, unsigned clock_output);
CredoError_t bh_slice_check_clock_output(CredoSlice_t* slice, unsigned clock_output, unsigned* state, unsigned* lane,
                                         unsigned* divider);

CredoError_t bh_get_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);
CredoError_t bh_get_top_pll_cap(CredoSlice_t* slice, unsigned* caps);

CredoError_t bh_top_set_fec_analyzer(CredoSlice_t* slice, int lane, int enable);
CredoError_t bh_fec_analyzer_set_config(CredoSlice_t* slice, int lane, int enable, CredoFecAnalyzerConfig_t* config);
CredoError_t bh_fec_analyzer_get_config(CredoSlice_t* slice, int lane, int* enable, CredoFecAnalyzerConfig_t* config);
CredoError_t bh_get_lane_count(CredoSlice_t* slice, int* host_lane, int* line_lane);
unsigned bh_is_valid_lane(CredoSlice_t* slice, int lane);
// Info functions
CredoError_t bh_display_slice_info(CredoSlice_t* slice, const char* argv[], size_t argc, CredoDisplayWriter_t writer,
                                   void* ud);

// Firmware functions
CredoError_t bh_fw_get_feature(CredoSlice_t* slice, unsigned* feature);
CredoError_t bh_fw_deconfig_cmd(CredoSlice_t* slice, FirmwareMode_t fw_mode, unsigned detail1, unsigned detail2);
CredoError_t bh_fw_download(CredoSlice_t* slice, const char* image_file);
CredoError_t bh_fw_force_loopback(CredoSlice_t* slice, unsigned lane, unsigned force);
CredoError_t bh_fw_clock_outout_notify(CredoSlice_t* slice, unsigned lane, unsigned clk_output, bool en);
CredoError_t bh_fw_clock_outout_ctrl(CredoSlice_t* slice, unsigned clk_output, bool en);
CredoError_t bh_fw_tx_control(CredoSlice_t* slice, unsigned lane, FwTxSource_t source);
CredoError_t bh_fw_rx_control(CredoSlice_t* slice, unsigned lane, FwRxControl_t rx_control);
CredoError_t bh_fw_TRF_control(CredoSlice_t* slice, unsigned lane, TRF_t source);
CredoError_t bh_fw_PLL_source_control(CredoSlice_t* slice, unsigned lane, int source);
CredoError_t bh_fw_port_config_mask(CredoSlice_t* slice, unsigned* config_mask);
CredoError_t bh_fw_phy_ready(CredoSlice_t* slice, unsigned* rdy);
CredoError_t bh_fw_phy_lane_ready(CredoSlice_t* slice, int lane, unsigned* rdy);
CredoError_t bh_fw_config_phy(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed, uint32_t flags);
CredoError_t bh_fw_config_lane(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed);
CredoError_t bh_fw_config_lane_loopback(CredoSlice_t* slice, int lane, unsigned speed);
CredoError_t bh_fw_deconfig_lane(CredoSlice_t* slice, int lane);
CredoError_t bh_fw_get_func_ver(CredoSlice_t* slice, unsigned* ver);
CredoError_t bh_fw_get_status(CredoSlice_t* slice, unsigned* status);

CredoError_t bh_fw_lane_disable(CredoSlice_t* slice, int lane);
CredoError_t bh_fw_atomic_read(CredoSlice_t* slice, unsigned addr, int len, unsigned* data);

CredoError_t bh_fw_optical_mode(CredoSlice_t* slice, unsigned* optical_mode);
CredoError_t bh_fw_set_optical_mode(CredoSlice_t* slice, unsigned first_lane, unsigned lane_count, int is_optical);
CredoError_t bh_fw_get_top_option(CredoSlice_t* slice, unsigned option, bool* en);
CredoError_t bh_fw_set_top_option(CredoSlice_t* slice, unsigned option, bool en);
CredoError_t bh_fw_set_top_sm_enalbe(CredoSlice_t* slice, int lane, bool en);
CredoError_t bh_fw_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);
CredoError_t bh_fw_get_opt_mode(CredoSlice_t* slice, int lane, unsigned* opt_mode);
CredoError_t bh_fw_get_retimer_debug(CredoSlice_t* slice, int lane, int index, unsigned* val);
CredoError_t bh_fw_get_retimer_state(CredoSlice_t* slice, int lane, unsigned* state);
CredoError_t bh_fw_get_bitmux_debug(CredoSlice_t* slice, int lane, int index, unsigned* val);
CredoError_t bh_fw_get_bitmux_state(CredoSlice_t* slice, int lane, unsigned* state);
CredoError_t bh_fw_get_bitmux_fifo(CredoSlice_t* slice, unsigned fifo_idx, unsigned buffer_type, unsigned* fifo);
CredoError_t bh_fw_get_gearbox_debug(CredoSlice_t* slice, int lane, int index, unsigned* val);
CredoError_t bh_fw_get_gearbox_state(CredoSlice_t* slice, int lane, unsigned* state);
CredoError_t bh_fw_set_sd_delay(CredoSlice_t* slice, int lane, int value);
CredoError_t bh_fw_get_sd_delay(CredoSlice_t* slice, int lane, int* value);
CredoError_t bh_fw_set_fast_recover_timeout(CredoSlice_t* slice, int lane, int value);
CredoError_t bh_fw_get_fast_recover_timeout(CredoSlice_t* slice, int lane, int* value);
CredoError_t bh_fw_get_anlt(CredoSlice_t* slice, int lane, unsigned* an_on, unsigned* lt_on);
CredoError_t bh_fw_get_an_mode(CredoSlice_t* slice, int lane, unsigned* an_mode);
CredoError_t bh_fw_get_an_state(CredoSlice_t* slice, int lane, unsigned* an_state);
CredoError_t bh_fw_get_an_pages(CredoSlice_t* slice, int lane, uint64_t* tx_pages, uint64_t* rx_pages);

CredoError_t bh_fw_get_rx_ffe_weighting_table(CredoSlice_t* slice, int lane, double** wt_table);

CredoError_t bh_fw_get_rsfec_index(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int* index);
CredoError_t bh_fw_get_rsfec_fifo(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                  CredoRSFECFifo_t* rsfec_fifo);
CredoError_t bh_fw_get_gearbox_info(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                    fw_gearbox_info_t* gb_info);
CredoError_t bh_fw_get_gearbox_error_info(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                          fw_gearbox_error_info_t* gb_err_info);
CredoError_t bh_fw_get_rsfec_total_codewords(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                             uint64_t* total_bits);
CredoError_t bh_fw_get_rsfec_uncorrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                  unsigned* uncorr_cw);
CredoError_t bh_fw_set_rsfec_count_freeze(CredoSlice_t* slice, int lane, bool en);

CredoError_t bh_fw_eye_monitor_start(CredoSlice_t* slice, int lane, int ber_exp, int flag);
CredoError_t bh_fw_eye_monitor_stop(CredoSlice_t* slice, int lane);
CredoError_t bh_fw_eye_monitor_range(CredoSlice_t* slice, int lane, int* vstep_side, int* hstep_side);
CredoError_t bh_fw_get_raw_cmd_address(CredoSlice_t* slice, unsigned* addr);
CredoError_t bh_port_link_state_internal(CredoSlice_t* slice, const CredoPortConfig_t* port_config, unsigned phy_rdy,
                                         CredoSide_t side, int* link);
CredoError_t bh_port_link_state(CredoSlice_t* slice, unsigned port_id, int link[]);
CredoError_t bh_port_link_state_host(CredoSlice_t* slice, unsigned port_id, int* link);
CredoError_t bh_port_link_state_line(CredoSlice_t* slice, unsigned port_id, int* link);

// Port functions
CredoError_t bh_port_config(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force);
CredoError_t bh_port_teardown(CredoSlice_t* slice, unsigned port_id);
CredoError_t bh_port_teardown_all(CredoSlice_t* slice);
CredoError_t bh_port_query(CredoSlice_t* slice, unsigned port_id, CredoPortConfig_t* port_config);
CredoError_t bh_port_info(CredoSlice_t* slice, unsigned port_id, CredoPortConfig_t* port_config, bool update);
CredoError_t bh_port_is_link_up(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* up);

// Serdes functions
CredoError_t bh_set_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode);
CredoError_t bh_get_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode);

CredoError_t bh_set_tx_precoder(CredoSlice_t* slice, int lane, int pc);
CredoError_t bh_get_tx_precoder(CredoSlice_t* slice, int lane, int* pc);
CredoError_t bh_set_tx_taps_internal(CredoSlice_t* slice, int lane);
CredoError_t bh_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]);
CredoError_t bh_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]);
CredoError_t bh_tx_disable(CredoSlice_t* slice, int lane);
CredoError_t bh_tx_no_disable(CredoSlice_t* slice, int lane);
CredoError_t bh_lane_tx_status(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status);
CredoError_t bh_rx_disable(CredoSlice_t* slice, int lane);
CredoError_t bh_rx_no_disable(CredoSlice_t* slice, int lane);
CredoError_t bh_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode);
CredoError_t bh_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode);
CredoError_t bh_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable);
CredoError_t bh_set_autoneg_pages(CredoSlice_t* slice, int lane, int page_id, uint64_t page);
CredoError_t bh_get_autoneg_exchanged_pages(CredoSlice_t* slice, int lane, int* page_count,
                                            uint64_t transmitted_pages[9], uint64_t received_pages[9]);
CredoError_t bh_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm);
CredoError_t bh_get_rx_ths(CredoSlice_t* slice, int lane, int ths[]);

CredoError_t bh_rsfec_get_total_codewords(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                          uint64_t* total_bits);
CredoError_t bh_rsfec_get_uncorrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                               unsigned* uncorr_cw);
CredoError_t bh_rsfec_set_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool enable);
CredoError_t bh_rsfec_get_fifo(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, CredoRSFECFifo_t* rsfec_fifo);
/* Lane operation mode */
CredoError_t bh_get_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);
CredoError_t bh_set_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode);
CredoError_t bh_update_lane_mode_ex(CredoSlice_t* slice, int lane, bool warm);
CredoError_t bh_update_lane_mode(CredoSlice_t* slice, int lane);
CredoError_t bh_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);

CredoError_t bh_reg_reset_lane(CredoSlice_t* slice, int lane);
CredoError_t bh_logic_reset_lane(CredoSlice_t* slice, int lane);
CredoError_t bh_serdes_preset_tx_taps(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc);

// data capture
CredoError_t bh_data_capture(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[], size_t argc,
                             CredoDataWriter_t writer, void* ud);
void* bh_data_get_commands(CredoSlice_t* slice, size_t* command_count);

CredoError_t bh_slice_warm_init(CredoSlice_t* slice);
CredoError_t bh_fw_get_acfg_status(CredoSlice_t* slice, int* state, int* error_code);
CredoError_t bh_fw_ready_post_actions(CredoSlice_t* slice);

#endif
