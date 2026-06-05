#ifndef SE_FUNCTIONS_H
#define SE_FUNCTIONS_H

#include "se_device.h"
#include "se_fw_helper.h"

CredoError_t se_get_top_pll_cap(CredoSlice_t* slice, unsigned* caps);
CredoError_t se_get_lane_count(CredoSlice_t* slice, int* host_lane, int* line_lane);

/* Lane operation mode */
CredoError_t se_logic_reset_lane(CredoSlice_t* slice, int lane);
CredoError_t se_reg_reset_lane(CredoSlice_t* slice, int lane);

CredoError_t se_get_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);
CredoError_t se_get_lane_tx_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);
CredoError_t se_set_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode);
CredoError_t se_set_lane_mode_uni_retimer(CredoSlice_t* slice, int src_lane, int dst_lane, CredoLaneMode_t mode);
CredoError_t se_update_lane_mode(CredoSlice_t* slice, int lane);
CredoError_t se_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);
CredoError_t se_get_lane_tx_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);

// fec analyzer
CredoError_t se_fec_analyzer_set_config(CredoSlice_t* slice, int lane, int enable, CredoFecAnalyzerConfig_t* config);
CredoError_t se_fec_analyzer_get_config(CredoSlice_t* slice, int lane, int* enable, CredoFecAnalyzerConfig_t* config);
CredoError_t se_fec_analyzer_reset(CredoSlice_t* slice, int lane);

// Info functions
CredoError_t se_display_info(CredoSlice_t* slice, const char* argv[], size_t argc, CredoDisplayWriter_t writer,
                             void* ud);

// Screaming eagle functions
CredoError_t se_get_rx_signal_detect(CredoSlice_t* slice, int lane, int* sd);
CredoError_t se_get_dfe_f0(CredoSlice_t* slice, int lane, unsigned* f0);
CredoError_t se_set_dfe_f0(CredoSlice_t* slice, int lane, unsigned f0);
CredoError_t se_get_dfe_f1(CredoSlice_t* slice, int lane, int* f1);
CredoError_t se_set_dfe_f1(CredoSlice_t* slice, int lane, int f1);
CredoError_t se_get_dfe(CredoSlice_t* slice, int lane, int dfe[]);
CredoError_t se_set_dfe(CredoSlice_t* slice, int lane, int dfe[]);
CredoError_t se_get_rx_ffe_all(CredoSlice_t* slice, int lane, int taps[]);
CredoError_t se_get_rx_ffe(CredoSlice_t* slice, int lane, int taps[]);
CredoError_t se_get_rx_ffe_cm1(CredoSlice_t* slice, int lane, int* cm1);
CredoError_t se_set_rx_ffe_cm1(CredoSlice_t* slice, int lane, int cm1);
CredoError_t se_get_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]);
CredoError_t se_set_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]);
CredoError_t se_get_rx_flt_loc(CredoSlice_t* slice, int lane, unsigned flt_loc[]);
CredoError_t se_get_rx_envelope(CredoSlice_t* slice, int lane, unsigned envelope[]);
CredoError_t se_get_dc_sar(CredoSlice_t* slice, int lane, int dc_sar[]);
CredoError_t se_get_gain_sar(CredoSlice_t* slice, int lane, unsigned gain_sar[]);
CredoError_t se_get_dc_cmn(CredoSlice_t* slice, int lane, int* dc_cmn);
CredoError_t se_get_rx_vga(CredoSlice_t* slice, int lane, unsigned* vga);
CredoError_t se_set_rx_vga(CredoSlice_t* slice, int lane, unsigned vga);

// Firmware functions
CredoError_t se_fw_get_feature(CredoSlice_t* slice, unsigned* feature);
CredoError_t se_fw_get_raw_cmd_address(CredoSlice_t* slice, unsigned* addr);
CredoError_t se_fw_get_toppll_mode(CredoSlice_t* slice, int* mode);
CredoError_t se_fw_set_toppll_mode(CredoSlice_t* slice, int mode);
CredoError_t se_fw_deconfig_cmd(CredoSlice_t* slice, unsigned fw_mode, unsigned detail1);
CredoError_t se_fw_data(CredoSlice_t* slice, int lane, unsigned section, unsigned index, int len, int data[]);
CredoError_t se_fw_data_unsigned(CredoSlice_t* slice, int lane, unsigned section, unsigned index, int len,
                                 unsigned data[]);
CredoError_t se_fw_set_fast_recover_timeout(CredoSlice_t* slice, int lane, int value);
CredoError_t se_fw_get_fast_recover_timeout(CredoSlice_t* slice, int lane, int* value);
CredoError_t se_fw_set_sd_delay(CredoSlice_t* slice, int lane, int value);
CredoError_t se_fw_get_sd_delay(CredoSlice_t* slice, int lane, int* value);
CredoError_t se_fw_set_fec_clk(CredoSlice_t* slice, int lane, int enable);
CredoError_t se_fw_get_status(CredoSlice_t* slice, unsigned* status);
CredoError_t se_fw_get_info_ver(CredoSlice_t* slice, unsigned* ver);
CredoError_t se_fw_get_config_mode(CredoSlice_t* slice, int lane, unsigned* config_mode);
CredoError_t se_fw_get_rate_mode(CredoSlice_t* slice, int lane, unsigned* rate_mode);
CredoError_t se_fw_get_tx_rate_mode(CredoSlice_t* slice, int lane, unsigned* rate_mode);
CredoError_t se_fw_get_cmd_log(CredoSlice_t* slice, unsigned idx, unsigned* cmd);
CredoError_t se_fw_get_state_timestamp(CredoSlice_t* slice, int lane, unsigned cnt, unsigned* timestamp);
CredoError_t se_fw_set_tx_map(CredoSlice_t* slice, unsigned map_a, unsigned map_b);
CredoError_t se_fw_get_tx_map(CredoSlice_t* slice, unsigned* map_a, unsigned* map_b);
CredoError_t se_fw_set_tx_map_isc(CredoSlice_t* slice, unsigned map_a, unsigned map_b);
CredoError_t se_fw_get_tx_map_isc(CredoSlice_t* slice, unsigned* map_a, unsigned* map_b);
CredoError_t se_fw_get_retimer_state(CredoSlice_t* slice, int lane, unsigned* state);
CredoError_t se_fw_get_bitmux_state(CredoSlice_t* slice, int lane, unsigned* state);
CredoError_t se_fw_download(CredoSlice_t* slice, const char* image_file);
CredoError_t se_fw_phy_ready(CredoSlice_t* slice, unsigned* rdy);
CredoError_t se_fw_phy_lane_ready(CredoSlice_t* slice, int lane, unsigned* rdy);
CredoError_t se_fw_get_eye_all(CredoSlice_t* slice, int lane, int eyes[]);
CredoError_t se_fw_get_eye_phase0(CredoSlice_t* slice, int lane, int eyes[]);
CredoError_t se_fw_get_eye(CredoSlice_t* slice, int lane, int eyes[3]);
CredoError_t se_fw_get_dfe(CredoSlice_t* slice, int lane, int dfe[]);
CredoError_t se_fw_set_dfe(CredoSlice_t* slice, int lane, int dfe[]);
CredoError_t se_fw_get_dfe_f0(CredoSlice_t* slice, int lane, unsigned* dfe_f0);
CredoError_t se_fw_set_dfe_f0(CredoSlice_t* slice, int lane, unsigned dfe_f0);
CredoError_t se_fw_get_dfe_f1(CredoSlice_t* slice, int lane, int* dfe_f1);
CredoError_t se_fw_set_dfe_f1(CredoSlice_t* slice, int lane, int dfe_f1);
CredoError_t se_fw_get_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]);
CredoError_t se_fw_set_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]);
CredoError_t se_fw_get_rx_ffe_all(CredoSlice_t* slice, int lane, int taps[]);
CredoError_t se_fw_get_rx_ffe(CredoSlice_t* slice, int lane, int phase, int taps[]);
CredoError_t se_fw_get_rx_ffe_cm1(CredoSlice_t* slice, int lane, int* cm1);
CredoError_t se_fw_set_rx_ffe_cm1(CredoSlice_t* slice, int lane, int cm1);
CredoError_t se_fw_get_isi_all(CredoSlice_t* slice, int lane, double isi[]);
CredoError_t se_fw_get_isi(CredoSlice_t* slice, int lane, double isi[]);
CredoError_t se_fw_get_dc_sar(CredoSlice_t* slice, int lane, int dc_sar[]);
CredoError_t se_fw_get_gain_sar(CredoSlice_t* slice, int lane, unsigned gain_sar[]);
CredoError_t se_fw_get_channel_est_psd(CredoSlice_t* slice, int lane, double psd[]);
CredoError_t se_fw_get_channel_est(CredoSlice_t* slice, int lane, double* chan_est);
CredoError_t se_fw_get_preset_index(CredoSlice_t* slice, int lane, unsigned* preset_index);
CredoError_t se_fw_get_amp(CredoSlice_t* slice, int lane, unsigned amp[]);
CredoError_t se_fw_get_an_state(CredoSlice_t* slice, int lane, unsigned* an_state);
CredoError_t se_fw_get_an_pages(CredoSlice_t* slice, int lane, uint64_t* tx_pages, uint64_t* rx_pages);
CredoError_t se_fw_get_rx_state(CredoSlice_t* slice, int lane, unsigned* rx_state);
CredoError_t se_fw_get_error_state(CredoSlice_t* slice, int lane, unsigned* error_state);
CredoError_t se_fw_get_opt_mode(CredoSlice_t* slice, int lane, unsigned* opt_mode);
CredoError_t se_fw_get_sd(CredoSlice_t* slice, int lane, int* sd);
CredoError_t se_fw_get_agc_att(CredoSlice_t* slice, int lane, int* agc_att);
CredoError_t se_fw_get_agc_gain(CredoSlice_t* slice, int lane, int* agc_gain);
CredoError_t se_fw_get_agc_degen(CredoSlice_t* slice, int lane, int* agc_degen);
CredoError_t se_fw_get_cdfl_env(CredoSlice_t* slice, int lane, int* cdfl_env);
CredoError_t se_fw_get_loss(CredoSlice_t* slice, int lane, int* loss);
CredoError_t se_fw_get_skef(CredoSlice_t* slice, int lane, int* skef);
CredoError_t se_fw_get_dc_cmn(CredoSlice_t* slice, int lane, int* dc_cmn);
CredoError_t se_fw_get_rx_vga(CredoSlice_t* slice, int lane, unsigned* vga);
CredoError_t se_fw_set_rx_vga(CredoSlice_t* slice, int lane, unsigned vga);
CredoError_t se_fw_set_rx_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]);
CredoError_t se_fw_get_tx_ffe(CredoSlice_t* slice, int lane, int* tx_ffe);
CredoError_t se_fw_get_link_time(CredoSlice_t* slice, int lane, unsigned* up, unsigned* down);
CredoError_t se_fw_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);
CredoError_t se_fw_get_anlt(CredoSlice_t* slice, int lane, unsigned* an_on, unsigned* lt_on);
CredoError_t se_fw_get_an_state(CredoSlice_t* slice, int lane, unsigned* an_state);
CredoError_t se_fw_get_lt_state(CredoSlice_t* slice, int lane, unsigned* lt_state);

CredoError_t se_fw_config_phy(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed, uint32_t flags);
CredoError_t se_fw_config_lane(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed);
CredoError_t se_fw_config_lane_loopback(CredoSlice_t* slice, int lane, unsigned speed);
CredoError_t se_fw_deconfig_lane(CredoSlice_t* slice, int lane);

CredoError_t se_fw_tx_control(CredoSlice_t* slice, int lane, FwTxSource_t source);
CredoError_t se_fw_tx_disable(CredoSlice_t* slice, int lane);
CredoError_t se_fw_tx_no_disable(CredoSlice_t* slice, int lane);
CredoError_t se_fw_tx_status(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status);

CredoError_t se_fw_lane_disable(CredoSlice_t* slice, int lane);
CredoError_t se_fw_rx_disable(CredoSlice_t* slice, int lane);
CredoError_t se_fw_rx_no_disable(CredoSlice_t* slice, int lane);
CredoError_t se_fw_rx_reset(CredoSlice_t* slice, int lane);

CredoError_t se_fw_testpoint_read(CredoSlice_t* slice, unsigned* vsensor);

// Serdes
CredoError_t se_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode);
CredoError_t se_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode);
CredoError_t se_set_tx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode);
CredoError_t se_set_tx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode);
CredoError_t se_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable);

CredoError_t se_get_eye_avg_count(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t se_get_eye_count(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t se_get_eye_all_count(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t se_get_agcgain_count(CredoSlice_t* slice, int lane, unsigned* count);
CredoError_t se_get_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]);
CredoError_t se_set_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]);
CredoError_t se_get_ppm(CredoSlice_t* slice, int lane, int* ppm);

CredoError_t se_set_tx_taps_internal(CredoSlice_t* slice, int lane, bool force);
CredoError_t se_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]);
CredoError_t se_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]);
CredoError_t se_get_tx_polarity(CredoSlice_t* slice, int lane, int* tx_pol);
CredoError_t se_set_tx_polarity(CredoSlice_t* slice, int lane, int tx_pol);
CredoError_t se_get_rx_polarity(CredoSlice_t* slice, int lane, int* rx_pol);
CredoError_t se_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol);
CredoError_t se_get_skef(CredoSlice_t* slice, int lane, unsigned skef[]);
CredoError_t se_set_skef(CredoSlice_t* slice, int lane, unsigned skef[]);

CredoError_t se_port_capture_info(CredoSlice_t* slice, unsigned port_id, SePortInfo_t* port_info);
CredoError_t se_port_assign_id(CredoSlice_t* slice, unsigned* port_id);
CredoError_t se_fw_teardown_port(CredoSlice_t* slice, unsigned port_id);
CredoError_t se_fw_teardown_port_all(CredoSlice_t* slice);
CredoError_t se_port_link_internal(CredoSlice_t* slice, unsigned port_id, unsigned phy_rdy, CredoSide_t side,
                                   int* link);
CredoError_t se_port_link(CredoSlice_t* slice, unsigned port_id, int link[]);
CredoError_t se_port_link_host(CredoSlice_t* slice, unsigned port_id, int* link);
CredoError_t se_port_link_line(CredoSlice_t* slice, unsigned port_id, int* link);
CredoError_t se_port_is_link_up(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* up);

CredoError_t se_set_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode);
CredoError_t se_get_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode);
CredoError_t se_fw_TRF_control(CredoSlice_t* slice, unsigned lane, TRF_t source);
CredoError_t se_fw_PLL_source_control(CredoSlice_t* slice, unsigned lane, int force);
CredoError_t se_fw_force_loopback(CredoSlice_t* slice, unsigned lane, unsigned force);
CredoError_t se_fw_t2r_force_loopback(CredoSlice_t* slice, unsigned lane, unsigned force);

CredoError_t se_fw_switchover_switch(CredoSlice_t* slice, unsigned new_active_map, unsigned old_active_map);

CredoError_t se_fw_eye_monitor_range(CredoSlice_t* slice, int lane, int* vstep_side, int* hstep_side);
CredoError_t se_fw_eye_monitor_start(CredoSlice_t* slice, int lane, int ber_exp, int flag);
CredoError_t se_fw_eye_monitor_stop(CredoSlice_t* slice, int lane);
CredoError_t se_fw_eye_monitor_progress(CredoSlice_t* slice, int lane, int* percent);
CredoError_t se_fw_eye_monitor_data(CredoSlice_t* slice, int lane, int** data, int* extent_mv);
CredoError_t se_fw_eye_monitor_separator(CredoSlice_t* slice, int separator[5]);

CredoError_t se_port_build(CredoSlice_t* slice, uint32_t port_id, const CredoPortSetup_t* setup);
CredoError_t se_port_start(CredoSlice_t* slice, uint32_t port_id, bool force);
CredoError_t se_port_get_setup(CredoSlice_t* slice, uint32_t port_id, bool* started, CredoPortSetup_t* setup);
void se_port_options_to_string(CredoSlice_t* slice, const SePortInfo_t* port_info, char* buf, uint32_t size);
CredoError_t se_port_get_isc_enable(CredoSlice_t* slice, uint32_t port_id, int* value);
CredoError_t se_port_get_active_lane_list(CredoSlice_t* slice, uint32_t port_id, int active_lane_list[32]);
CredoError_t se_port_get_switchover_configured(CredoSlice_t* slice, bool* value);
CredoError_t se_port_switchover_switch(CredoSlice_t* slice, uint32_t port_id, int lane);
CredoError_t se_serdes_preset_tx_taps(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc);

CredoError_t se_slice_enable_clock_output(CredoSlice_t* slice, unsigned clock_output, unsigned lane, unsigned divider);
CredoError_t se_slice_disable_all_clock_output(CredoSlice_t* slice);
CredoError_t se_slice_disable_clock_output(CredoSlice_t* slice, unsigned clock_output);
CredoError_t se_slice_check_clock_output(CredoSlice_t* slice, unsigned clock_output, unsigned* state, unsigned* lane,
                                         unsigned* divider);

CredoError_t se_autoneg_get_exchanged_pages(CredoSlice_t* slice, int lane, int* page_count,
                                            uint64_t transmitted_pages[9], uint64_t received_pages[9]);
CredoError_t se_an_get_restart_count(CredoSlice_t* slice, int lane, unsigned* val);
CredoError_t se_lt_get_restart_count(CredoSlice_t* slice, int lane, unsigned* val);
CredoError_t se_an_get_state(CredoSlice_t* slice, int lane, CredoAutoNegState_t* state);
CredoError_t se_get_lt_status(CredoSlice_t* slice, int lane, CredoLinkTrainingStatus_t* lt_status);
CredoError_t se_get_lt_state(CredoSlice_t* slice, int lane, CredoLinkTrainingState_t* lt_state);

CredoError_t se_soft_reset_without_fw_running(CredoSlice_t* slice);

CredoError_t se_fw_spiflash_status(CredoSlice_t* slice, unsigned* status);
CredoError_t se_fw_spiflash_erase(CredoSlice_t* slice, unsigned addr);
CredoError_t se_fw_spiflash_read(CredoSlice_t* slice, unsigned addr, unsigned* buffer, unsigned count);
CredoError_t se_fw_spiflash_write(CredoSlice_t* slice, unsigned addr, const unsigned* buffer, unsigned count);
CredoError_t se_fw_spiflash_format(CredoSlice_t* slice, unsigned flash_kb_size, int force);
CredoError_t se_fw_spiflash_display(CredoSlice_t* slice);
CredoError_t se_fw_spiflash_write_firmware(CredoSlice_t* slice, const char* fwname, int partition_num, int force);
CredoError_t se_fw_spiflash_read_firmware(CredoSlice_t* slice, const char* fwname, int partition_num);
CredoError_t se_fw_spiflash_set_partition(CredoSlice_t* slice, int partition_num);

// data capture
CredoError_t se_data_capture(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[], size_t argc,
                             CredoDataWriter_t writer, void* ud);
void* se_data_get_commands(CredoSlice_t* slice, size_t* command_count);

// fast recovery
CredoError_t se_frecov_configure(CredoSlice_t* slice, int lane, unsigned timeout_ms);
CredoError_t se_frecov_get_status(CredoSlice_t* slice, int lane, unsigned* timeout_ms,
                                  CredoFastRecoveryStatus_t* status);
CredoError_t se_frecov_get_recover_count(CredoSlice_t* slice, int lane, unsigned* count);

CredoError_t se_slice_get_revision(CredoSlice_t* slice, unsigned* revision_number);

CredoError_t se_testpoint_clear(CredoSlice_t* slice);
CredoError_t se_testpoint_select(CredoSlice_t* slice, const CredoTestPoint_t* testpoint);
CredoError_t se_testpoint_read(CredoSlice_t* slice, double* vsensor);

// time

CredoError_t se_time_start(CredoSlice_t* slice, double* unix_time);
CredoError_t se_time_system(CredoSlice_t* slice, double* timedelta);

CredoError_t se_flexspeed_set_registers(CredoSlice_t* slice, CredoSide_t side, const flexspeed_config_t* config,
                                        bool en_tx, bool en_rx);

flexspeed_config_t se_flexspeed_compute_config(double datarate, CredoLaneMode_t mode);
double se_flexspeed_compute_speed(double base_datarate, unsigned pll_n, unsigned pll_n_frac);

#endif
