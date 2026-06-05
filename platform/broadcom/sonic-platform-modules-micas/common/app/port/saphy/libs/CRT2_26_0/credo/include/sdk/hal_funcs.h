#ifndef HAL_FUNCS_H
#define HAL_FUNCS_H
#include "sdk/device.h"

static inline CredoError_t hal_get_base_address(CredoSlice_t* slice, const RegHive_t* hive, unsigned* base_addr) {
    if (slice->hal->hal_get_base_address == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_base_address(slice, hive, base_addr);
}

static inline CredoError_t hal_get_hive_index(CredoSlice_t* slice, unsigned* hive_index) {
    if (slice->hal->hal_get_hive_index == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_hive_index(slice, hive_index);
}

static inline CredoError_t hal_slice_init(CredoSlice_t* slice, const CredoSliceConfig_t* config) {
    if (slice->hal->hal_slice_init == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_init(slice, config);
}

static inline CredoError_t hal_slice_get_oui(CredoSlice_t* slice, unsigned* oui) {
    if (slice->hal->hal_slice_get_oui == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_get_oui(slice, oui);
}

static inline CredoError_t hal_slice_get_model_number(CredoSlice_t* slice, unsigned* model_number) {
    if (slice->hal->hal_slice_get_model_number == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_get_model_number(slice, model_number);
}

static inline CredoError_t hal_slice_get_revision_number(CredoSlice_t* slice, unsigned* revision_number) {
    if (slice->hal->hal_slice_get_revision_number == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_get_revision_number(slice, revision_number);
}

static inline CredoError_t hal_slice_get_type(CredoSlice_t* slice, CredoSliceType_t* slice_type) {
    if (slice->hal->hal_slice_get_type == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_get_type(slice, slice_type);
}

static inline CredoError_t hal_slice_get_vsensor(CredoSlice_t* slice, double* vsensor) {
    if (slice->hal->hal_slice_get_vsensor == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_get_vsensor(slice, vsensor);
}

static inline CredoError_t hal_slice_get_vsensor_ex(CredoSlice_t* slice, unsigned sel_vin, double* vsensor) {
    if (slice->hal->hal_slice_get_vsensor_ex == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_get_vsensor_ex(slice, sel_vin, vsensor);
}

static inline CredoError_t hal_slice_get_sram_status(CredoSlice_t* slice, CredoSramStatus_t* sram_status) {
    if (slice->hal->hal_slice_get_sram_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_get_sram_status(slice, sram_status);
}

static inline CredoError_t hal_slice_generate_sram_error(CredoSlice_t* slice, CredoSramStatus_t sram_status) {
    if (slice->hal->hal_slice_generate_sram_error == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_generate_sram_error(slice, sram_status);
}

static inline CredoError_t hal_slice_load_setup(CredoSlice_t* slice, const char* file_path) {
    if (slice->hal->hal_slice_load_setup == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_load_setup(slice, file_path);
}

static inline CredoError_t hal_slice_save_setup(CredoSlice_t* slice, const char* file_path) {
    if (slice->hal->hal_slice_save_setup == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_save_setup(slice, file_path);
}

static inline CredoError_t hal_slice_get_reghive(CredoSlice_t* slice, const char* hivename, const RegHive_t** reghive) {
    if (slice->hal->hal_slice_get_reghive == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_get_reghive(slice, hivename, reghive);
}

static inline CredoError_t hal_slice_enable_clock_output(CredoSlice_t* slice, unsigned clock_output, unsigned lane,
                                                         unsigned divider) {
    if (slice->hal->hal_slice_enable_clock_output == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_enable_clock_output(slice, clock_output, lane, divider);
}

static inline CredoError_t hal_slice_disable_all_clock_output(CredoSlice_t* slice) {
    if (slice->hal->hal_slice_disable_all_clock_output == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_disable_all_clock_output(slice);
}

static inline CredoError_t hal_slice_disable_clock_output(CredoSlice_t* slice, unsigned clock_output) {
    if (slice->hal->hal_slice_disable_clock_output == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_disable_clock_output(slice, clock_output);
}

static inline CredoError_t hal_display_slice_info(CredoSlice_t* slice, int argc, const char* argv[]) {
    if (slice->hal->hal_display_slice_info == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_display_slice_info(slice, argc, argv);
}

static inline CredoError_t hal_fw_unload(CredoSlice_t* slice) {
    if (slice->hal->hal_fw_unload == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_unload(slice);
}

static inline CredoError_t hal_fw_load(CredoSlice_t* slice, FILE* file) {
    if (slice->hal->hal_fw_load == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_load(slice, file);
}

static inline CredoError_t hal_fw_load_broadcast(CredoSlice_t* slices[], int slice_count, FILE* file,
                                                 unsigned delay_time_us) {
    if (slices[0]->hal->hal_fw_load_broadcast == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slices[0]->hal->hal_fw_load_broadcast(slices, slice_count, file, delay_time_us);
}

static inline CredoError_t hal_fw_load_spi(CredoSlice_t* slice, int partition_num) {
    if (slice->hal->hal_fw_load_spi == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_load_spi(slice, partition_num);
}

static inline CredoError_t hal_fw_spiflash_status(CredoSlice_t* slice, unsigned* response) {
    if (slice->hal->hal_fw_spiflash_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_spiflash_status(slice, response);
}

static inline CredoError_t hal_fw_spiflash_read(CredoSlice_t* slice, unsigned addr, unsigned* buffer, unsigned count) {
    if (slice->hal->hal_fw_spiflash_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_spiflash_read(slice, addr, buffer, count);
}

static inline CredoError_t hal_fw_spiflash_write(CredoSlice_t* slice, unsigned addr, const unsigned* buffer,
                                                 unsigned count) {
    if (slice->hal->hal_fw_spiflash_write == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_spiflash_write(slice, addr, buffer, count);
}

static inline CredoError_t hal_fw_spiflash_erase(CredoSlice_t* slice, unsigned addr) {
    if (slice->hal->hal_fw_spiflash_erase == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_spiflash_erase(slice, addr);
}

static inline CredoError_t hal_fw_spiflash_display_mbr(CredoSlice_t* slice) {
    if (slice->hal->hal_fw_spiflash_display_mbr == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_spiflash_display_mbr(slice);
}

static inline CredoError_t hal_fw_spiflash_format_mbr(CredoSlice_t* slice, unsigned flash_kb_size, int force) {
    if (slice->hal->hal_fw_spiflash_format_mbr == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_spiflash_format_mbr(slice, flash_kb_size, force);
}

static inline CredoError_t hal_fw_spiflash_read_firmware(CredoSlice_t* slice, const char* fwname, int partition_num) {
    if (slice->hal->hal_fw_spiflash_read_firmware == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_spiflash_read_firmware(slice, fwname, partition_num);
}

static inline CredoError_t hal_fw_spiflash_write_firmware(CredoSlice_t* slice, const char* fwname, int partition_num,
                                                          int force) {
    if (slice->hal->hal_fw_spiflash_write_firmware == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_spiflash_write_firmware(slice, fwname, partition_num, force);
}

static inline CredoError_t hal_fw_wait_magic_word(CredoSlice_t* slice, unsigned timeout) {
    if (slice->hal->hal_fw_wait_magic_word == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_wait_magic_word(slice, timeout);
}

static inline CredoError_t hal_fw_wait_top_pll_cal(CredoSlice_t* slice, unsigned timeout) {
    if (slice->hal->hal_fw_wait_top_pll_cal == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_wait_top_pll_cal(slice, timeout);
}

static inline CredoError_t hal_fw_get_status(CredoSlice_t* slice, unsigned* status) {
    if (slice->hal->hal_fw_get_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_status(slice, status);
}

static inline CredoError_t hal_fw_magic(CredoSlice_t* slice, unsigned* magic) {
    if (slice->hal->hal_fw_magic == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_magic(slice, magic);
}

static inline CredoError_t hal_fw_ver(CredoSlice_t* slice, unsigned* version) {
    if (slice->hal->hal_fw_ver == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_ver(slice, version);
}

static inline CredoError_t hal_fw_hash(CredoSlice_t* slice, unsigned* hash) {
    if (slice->hal->hal_fw_hash == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_hash(slice, hash);
}

static inline CredoError_t hal_fw_crc(CredoSlice_t* slice, unsigned* crc) {
    if (slice->hal->hal_fw_crc == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_crc(slice, crc);
}

static inline CredoError_t hal_fw_date(CredoSlice_t* slice, unsigned* date) {
    if (slice->hal->hal_fw_date == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_date(slice, date);
}

static inline CredoError_t hal_fw_get_raw_cmd_address(CredoSlice_t* slice, unsigned* addr) {
    if (slice->hal->hal_fw_get_raw_cmd_address == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_raw_cmd_address(slice, addr);
}

static inline CredoError_t hal_fw_cmd(CredoSlice_t* slice, unsigned cmd, unsigned params, unsigned* response,
                                      unsigned* response_param) {
    if (slice->hal->hal_fw_cmd == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_cmd(slice, cmd, params, response, response_param);
}

static inline CredoError_t hal_fw_cmd_ex(CredoSlice_t* slice, unsigned cmd, unsigned param1, unsigned param2,
                                         unsigned* response, unsigned* response_param1, unsigned* response_param2) {
    if (slice->hal->hal_fw_cmd_ex == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_cmd_ex(slice, cmd, param1, param2, response, response_param1, response_param2);
}

static inline CredoError_t hal_fw_debug_cmd(CredoSlice_t* slice, unsigned lane, unsigned section, unsigned index,
                                            unsigned* value) {
    if (slice->hal->hal_fw_debug_cmd == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_debug_cmd(slice, lane, section, index, value);
}

static inline CredoError_t hal_fw_debug_cmd_ex(CredoSlice_t* slice, unsigned lane, unsigned section, unsigned index,
                                               unsigned* response1, unsigned* response2) {
    if (slice->hal->hal_fw_debug_cmd_ex == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_debug_cmd_ex(slice, lane, section, index, response1, response2);
}

static inline CredoError_t hal_fw_reg_rd(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned* value) {
    if (slice->hal->hal_fw_reg_rd == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_reg_rd(slice, addr, section, value);
}

static inline CredoError_t hal_fw_reg_wr(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned value) {
    if (slice->hal->hal_fw_reg_wr == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_reg_wr(slice, addr, section, value);
}

static inline CredoError_t hal_fw_reg_rd_internal(CredoSlice_t* slice, unsigned combined_addr, unsigned* value) {
    if (slice->hal->hal_fw_reg_rd_internal == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_reg_rd_internal(slice, combined_addr, value);
}

static inline CredoError_t hal_fw_reg_wr_internal(CredoSlice_t* slice, unsigned combined_addr, unsigned value) {
    if (slice->hal->hal_fw_reg_wr_internal == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_reg_wr_internal(slice, combined_addr, value);
}

static inline CredoError_t hal_fw_config_port(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force) {
    if (slice->hal->hal_fw_config_port == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_config_port(slice, port_config, force);
}

static inline CredoError_t hal_fw_teardown_port(CredoSlice_t* slice, uint32_t port_id) {
    if (slice->hal->hal_fw_teardown_port == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_teardown_port(slice, port_id);
}

static inline CredoError_t hal_fw_query_port(CredoSlice_t* slice, uint32_t portId, CredoPortConfig_t* port_config) {
    if (slice->hal->hal_fw_query_port == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_query_port(slice, portId, port_config);
}

static inline CredoError_t hal_fw_clear_all_port(CredoSlice_t* slice) {
    if (slice->hal->hal_fw_clear_all_port == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_clear_all_port(slice);
}

static inline CredoError_t hal_fw_get_slice_temp(CredoSlice_t* slice, double* temp) {
    if (slice->hal->hal_fw_get_slice_temp == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_slice_temp(slice, temp);
}

static inline CredoError_t hal_fw_config_lane(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode,
                                              uint32_t speed) {
    if (slice->hal->hal_fw_config_lane == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_config_lane(slice, lane, lane_mode, speed);
}

static inline CredoError_t hal_fw_config_lane_loopback(CredoSlice_t* slice, int lane, uint32_t speed) {
    if (slice->hal->hal_fw_config_lane_loopback == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_config_lane_loopback(slice, lane, speed);
}

static inline CredoError_t hal_fw_deconfig_lane(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_fw_deconfig_lane == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_deconfig_lane(slice, lane);
}

static inline CredoError_t hal_fw_phy_ready(CredoSlice_t* slice, unsigned* rdy) {
    if (slice->hal->hal_fw_phy_ready == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_phy_ready(slice, rdy);
}

static inline CredoError_t hal_fw_phy_lane_ready(CredoSlice_t* slice, int lane, unsigned* rdy) {
    if (slice->hal->hal_fw_phy_lane_ready == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_phy_lane_ready(slice, lane, rdy);
}

static inline CredoError_t hal_fw_get_adapt_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (slice->hal->hal_fw_get_adapt_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_adapt_count(slice, lane, count);
}

static inline CredoError_t hal_fw_get_readapt_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (slice->hal->hal_fw_get_readapt_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_readapt_count(slice, lane, count);
}

static inline CredoError_t hal_fw_get_link_lost_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (slice->hal->hal_fw_get_link_lost_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_link_lost_count(slice, lane, count);
}

static inline CredoError_t hal_fw_get_los_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (slice->hal->hal_fw_get_los_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_los_count(slice, lane, count);
}

static inline CredoError_t hal_fw_get_channel_estimate(CredoSlice_t* slice, int lane, double* chan_est) {
    if (slice->hal->hal_fw_get_channel_estimate == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_channel_estimate(slice, lane, chan_est);
}

static inline CredoError_t hal_fw_get_of(CredoSlice_t* slice, int lane, unsigned* of) {
    if (slice->hal->hal_fw_get_of == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_of(slice, lane, of);
}

static inline CredoError_t hal_fw_get_hf(CredoSlice_t* slice, int lane, unsigned* hf) {
    if (slice->hal->hal_fw_get_hf == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_hf(slice, lane, hf);
}

static inline CredoError_t hal_fw_get_dfe(CredoSlice_t* slice, int lane, double dfe_taps[]) {
    if (slice->hal->hal_fw_get_dfe == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_dfe(slice, lane, dfe_taps);
}

static inline CredoError_t hal_fw_get_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    if (slice->hal->hal_fw_get_eye == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_eye(slice, lane, eyes);
}

static inline CredoError_t hal_fw_get_isi(CredoSlice_t* slice, int lane, int isi[]) {
    if (slice->hal->hal_fw_get_isi == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_isi(slice, lane, isi);
}

static inline CredoError_t hal_fw_get_rx_ffe(CredoSlice_t* slice, int lane, int taps[]) {
    if (slice->hal->hal_fw_get_rx_ffe == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_rx_ffe(slice, lane, taps);
}

static inline CredoError_t hal_fw_get_rx_ffe_nbias(CredoSlice_t* slice, int lane, int nbias[]) {
    if (slice->hal->hal_fw_get_rx_ffe_nbias == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_rx_ffe_nbias(slice, lane, nbias);
}

static inline CredoError_t hal_fw_get_rx_ffe_kaccu(CredoSlice_t* slice, int lane, double kaccu[]) {
    if (slice->hal->hal_fw_get_rx_ffe_kaccu == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_rx_ffe_kaccu(slice, lane, kaccu);
}

static inline CredoError_t hal_fw_get_rx_ffe_weighting_table(CredoSlice_t* slice, int lane, double** wt_table) {
    if (slice->hal->hal_fw_get_rx_ffe_weighting_table == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_rx_ffe_weighting_table(slice, lane, wt_table);
}

static inline CredoError_t hal_fw_get_rx_ffe_flip_counter(CredoSlice_t* slice, int lane, int flip_counter[]) {
    if (slice->hal->hal_fw_get_rx_ffe_flip_counter == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_rx_ffe_flip_counter(slice, lane, flip_counter);
}

static inline CredoError_t hal_fw_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    if (slice->hal->hal_fw_get_lane_speed == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_lane_speed(slice, lane, speed_kbps);
}

static inline CredoError_t hal_fw_eye_monitor_start(CredoSlice_t* slice, int lane, int ber_exp, int flag) {
    if (slice->hal->hal_fw_eye_monitor_start == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_eye_monitor_start(slice, lane, ber_exp, flag);
}

static inline CredoError_t hal_fw_eye_monitor_stop(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_fw_eye_monitor_stop == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_eye_monitor_stop(slice, lane);
}

static inline CredoError_t hal_fw_eye_monitor_progress(CredoSlice_t* slice, int lane, int* percent) {
    if (slice->hal->hal_fw_eye_monitor_progress == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_eye_monitor_progress(slice, lane, percent);
}

static inline CredoError_t hal_fw_eye_monitor_data(CredoSlice_t* slice, int lane, int** data, int* extent_mv) {
    if (slice->hal->hal_fw_eye_monitor_data == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_eye_monitor_data(slice, lane, data, extent_mv);
}

static inline CredoError_t hal_fw_eye_monitor_range(CredoSlice_t* slice, int lane, int* vstep_full, int* hstep_full) {
    if (slice->hal->hal_fw_eye_monitor_range == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_eye_monitor_range(slice, lane, vstep_full, hstep_full);
}

static inline CredoError_t hal_fw_eye_monitor_separator(CredoSlice_t* slice, int separator[5]) {
    if (slice->hal->hal_fw_eye_monitor_separator == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_eye_monitor_separator(slice, separator);
}

static inline CredoError_t hal_set_lane_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config) {
    if (slice->hal->hal_set_lane_config == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_lane_config(slice, lane, lane_config);
}

static inline CredoError_t hal_get_lane_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config) {
    if (slice->hal->hal_get_lane_config == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_lane_config(slice, lane, lane_config);
}

static inline CredoError_t hal_set_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode) {
    if (slice->hal->hal_set_lane_loopback_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_lane_loopback_mode(slice, lane, mode);
}

static inline CredoError_t hal_get_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode) {
    if (slice->hal->hal_get_lane_loopback_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_lane_loopback_mode(slice, lane, mode);
}

static inline CredoError_t hal_get_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    if (slice->hal->hal_get_lane_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_lane_mode(slice, lane, mode);
}

static inline CredoError_t hal_set_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode) {
    if (slice->hal->hal_set_lane_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_lane_mode(slice, lane, mode);
}

static inline CredoError_t hal_update_lane_mode(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_update_lane_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_update_lane_mode(slice, lane);
}

static inline CredoError_t hal_enable_lane(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_enable_lane == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_enable_lane(slice, lane);
}

static inline CredoError_t hal_disable_lane(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_disable_lane == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_disable_lane(slice, lane);
}

static inline CredoError_t hal_top_pll_cal(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_top_pll_cal == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_top_pll_cal(slice, lane);
}

static inline CredoError_t hal_top_pll_init(CredoSlice_t* slice) {
    if (slice->hal->hal_top_pll_init == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_top_pll_init(slice);
}

static inline CredoError_t hal_get_top_pll_cap(CredoSlice_t* slice, unsigned* cap) {
    if (slice->hal->hal_get_top_pll_cap == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_top_pll_cap(slice, cap);
}

static inline CredoError_t hal_get_lane_count(CredoSlice_t* slice, int* host_lane, int* line_lane) {
    if (slice->hal->hal_get_lane_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_lane_count(slice, host_lane, line_lane);
}

static inline CredoError_t hal_get_rx_ffe_range(CredoSlice_t* slice, int lane, int* taps_len, int* sum_len) {
    if (slice->hal->hal_get_rx_ffe_range == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_ffe_range(slice, lane, taps_len, sum_len);
}

static inline CredoError_t hal_get_rx_ffe_weighting_table_range(CredoSlice_t* slice, int lane, int* row, int* col) {
    if (slice->hal->hal_get_rx_ffe_weighting_table_range == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_ffe_weighting_table_range(slice, lane, row, col);
}

static inline CredoError_t hal_get_tx_ffe_range(CredoSlice_t* slice, int lane, int* length, int* extended_length) {
    if (slice->hal->hal_get_tx_ffe_range == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_ffe_range(slice, lane, length, extended_length);
}

static inline CredoError_t hal_get_rx_dfe_range(CredoSlice_t* slice, int lane, int* length) {
    if (slice->hal->hal_get_rx_dfe_range == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_dfe_range(slice, lane, length);
}

static inline CredoError_t hal_get_rx_isi_range(CredoSlice_t* slice, int lane, int* length) {
    if (slice->hal->hal_get_rx_isi_range == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_isi_range(slice, lane, length);
}

static inline CredoError_t hal_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol) {
    if (slice->hal->hal_set_rx_polarity == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_polarity(slice, lane, rx_pol);
}

static inline CredoError_t hal_set_tx_polarity(CredoSlice_t* slice, int lane, int tx_pol) {
    if (slice->hal->hal_set_tx_polarity == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_polarity(slice, lane, tx_pol);
}

static inline CredoError_t hal_get_rx_polarity(CredoSlice_t* slice, int lane, int* rx_pol) {
    if (slice->hal->hal_get_rx_polarity == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_polarity(slice, lane, rx_pol);
}

static inline CredoError_t hal_get_tx_polarity(CredoSlice_t* slice, int lane, int* tx_pol) {
    if (slice->hal->hal_get_tx_polarity == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_polarity(slice, lane, tx_pol);
}

static inline CredoError_t hal_set_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t input_mode) {
    if (slice->hal->hal_set_rx_input_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_input_mode(slice, lane, input_mode);
}

static inline CredoError_t hal_get_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t* input_mode) {
    if (slice->hal->hal_get_rx_input_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_input_mode(slice, lane, input_mode);
}

static inline CredoError_t hal_set_tx_gray_code(CredoSlice_t* slice, int lane, int tx_gc) {
    if (slice->hal->hal_set_tx_gray_code == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_gray_code(slice, lane, tx_gc);
}

static inline CredoError_t hal_set_rx_gray_code(CredoSlice_t* slice, int lane, int rx_gc) {
    if (slice->hal->hal_set_rx_gray_code == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_gray_code(slice, lane, rx_gc);
}

static inline CredoError_t hal_get_tx_gray_code(CredoSlice_t* slice, int lane, int* tx_gc) {
    if (slice->hal->hal_get_tx_gray_code == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_gray_code(slice, lane, tx_gc);
}

static inline CredoError_t hal_get_rx_gray_code(CredoSlice_t* slice, int lane, int* rx_gc) {
    if (slice->hal->hal_get_rx_gray_code == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_gray_code(slice, lane, rx_gc);
}

static inline CredoError_t hal_set_tx_precoder(CredoSlice_t* slice, int lane, int tx_pc) {
    if (slice->hal->hal_set_tx_precoder == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_precoder(slice, lane, tx_pc);
}

static inline CredoError_t hal_set_rx_precoder(CredoSlice_t* slice, int lane, int rx_pc) {
    if (slice->hal->hal_set_rx_precoder == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_precoder(slice, lane, rx_pc);
}

static inline CredoError_t hal_get_tx_precoder(CredoSlice_t* slice, int lane, int* tx_pc) {
    if (slice->hal->hal_get_tx_precoder == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_precoder(slice, lane, tx_pc);
}

static inline CredoError_t hal_get_rx_precoder(CredoSlice_t* slice, int lane, int* rx_pc) {
    if (slice->hal->hal_get_rx_precoder == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_precoder(slice, lane, rx_pc);
}

static inline CredoError_t hal_set_tx_msb(CredoSlice_t* slice, int lane, int tx_msb) {
    if (slice->hal->hal_set_tx_msb == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_msb(slice, lane, tx_msb);
}

static inline CredoError_t hal_set_rx_msb(CredoSlice_t* slice, int lane, int rx_msb) {
    if (slice->hal->hal_set_rx_msb == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_msb(slice, lane, rx_msb);
}

static inline CredoError_t hal_get_tx_msb(CredoSlice_t* slice, int lane, int* tx_msb) {
    if (slice->hal->hal_get_tx_msb == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_msb(slice, lane, tx_msb);
}

static inline CredoError_t hal_get_rx_msb(CredoSlice_t* slice, int lane, int* rx_msb) {
    if (slice->hal->hal_get_rx_msb == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_msb(slice, lane, rx_msb);
}

static inline CredoError_t hal_set_tx_cap(CredoSlice_t* slice, int lane, int tx_cap) {
    if (slice->hal->hal_set_tx_cap == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_cap(slice, lane, tx_cap);
}

static inline CredoError_t hal_set_rx_cap(CredoSlice_t* slice, int lane, int rx_cap) {
    if (slice->hal->hal_set_rx_cap == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_cap(slice, lane, rx_cap);
}

static inline CredoError_t hal_get_tx_cap(CredoSlice_t* slice, int lane, int* tx_cap) {
    if (slice->hal->hal_get_tx_cap == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_cap(slice, lane, tx_cap);
}

static inline CredoError_t hal_get_rx_cap(CredoSlice_t* slice, int lane, int* rx_cap) {
    if (slice->hal->hal_get_rx_cap == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_cap(slice, lane, rx_cap);
}

static inline CredoError_t hal_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm) {
    if (slice->hal->hal_get_rx_ppm == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_ppm(slice, lane, ppm);
}

static inline CredoError_t hal_get_rx_skef(CredoSlice_t* slice, int lane, int* enable, int* degen, int* addcap,
                                           int* skef_gain) {
    if (slice->hal->hal_get_rx_skef == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_skef(slice, lane, enable, degen, addcap, skef_gain);
}

static inline CredoError_t hal_get_rx_dac(CredoSlice_t* slice, int lane, int* rx_dac) {
    if (slice->hal->hal_get_rx_dac == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_dac(slice, lane, rx_dac);
}

static inline CredoError_t hal_get_rx_attenuator(CredoSlice_t* slice, int lane, int* passive, int* gain,
                                                 int* termtune) {
    if (slice->hal->hal_get_rx_attenuator == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_attenuator(slice, lane, passive, gain, termtune);
}

static inline CredoError_t hal_get_ffe_taps(CredoSlice_t* slice, int lane, int taps[]) {
    if (slice->hal->hal_get_ffe_taps == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_ffe_taps(slice, lane, taps);
}

static inline CredoError_t hal_get_ffe_taps_fine(CredoSlice_t* slice, int lane, int taps[]) {
    if (slice->hal->hal_get_ffe_taps_fine == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_ffe_taps_fine(slice, lane, taps);
}

static inline CredoError_t hal_get_f1over3(CredoSlice_t* slice, int lane, int* value) {
    if (slice->hal->hal_get_f1over3 == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_f1over3(slice, lane, value);
}

static inline CredoError_t hal_get_agcgain_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (slice->hal->hal_get_agcgain_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_agcgain_count(slice, lane, count);
}

static inline CredoError_t hal_get_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    if (slice->hal->hal_get_agcgain == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_agcgain(slice, lane, agcgain);
}

static inline CredoError_t hal_get_ctle_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (slice->hal->hal_get_ctle_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_ctle_count(slice, lane, count);
}

static inline CredoError_t hal_get_ctle(CredoSlice_t* slice, int lane, unsigned value[]) {
    if (slice->hal->hal_get_ctle == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_ctle(slice, lane, value);
}

static inline CredoError_t hal_get_delta_phase(CredoSlice_t* slice, int lane, int* value) {
    if (slice->hal->hal_get_delta_phase == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_delta_phase(slice, lane, value);
}

static inline CredoError_t hal_get_edge(CredoSlice_t* slice, int lane, unsigned* value) {
    if (slice->hal->hal_get_edge == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_edge(slice, lane, value);
}

static inline CredoError_t hal_get_dfe(CredoSlice_t* slice, int lane, double dfe_taps[]) {
    if (slice->hal->hal_get_dfe == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_dfe(slice, lane, dfe_taps);
}

static inline CredoError_t hal_get_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    if (slice->hal->hal_get_eye == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_eye(slice, lane, eyes);
}

static inline CredoError_t hal_get_lane_ready(CredoSlice_t* slice, int lane, int* ready) {
    if (slice->hal->hal_get_lane_ready == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_lane_ready(slice, lane, ready);
}

static inline CredoError_t hal_get_signal_detect(CredoSlice_t* slice, int lane, int* sd) {
    if (slice->hal->hal_get_signal_detect == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_signal_detect(slice, lane, sd);
}

static inline CredoError_t hal_set_rx_skef(CredoSlice_t* slice, int lane, int enable, int degen, int addcap,
                                           int skef_gain) {
    if (slice->hal->hal_set_rx_skef == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_skef(slice, lane, enable, degen, addcap, skef_gain);
}

static inline CredoError_t hal_set_rx_dac(CredoSlice_t* slice, int lane, int rx_dac) {
    if (slice->hal->hal_set_rx_dac == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_dac(slice, lane, rx_dac);
}

static inline CredoError_t hal_set_rx_attenuator(CredoSlice_t* slice, int lane, int passive, int gain, int termtune) {
    if (slice->hal->hal_set_rx_attenuator == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_attenuator(slice, lane, passive, gain, termtune);
}

static inline CredoError_t hal_set_ffe_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    if (slice->hal->hal_set_ffe_taps == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_ffe_taps(slice, lane, taps);
}

static inline CredoError_t hal_set_ffe_taps_fine(CredoSlice_t* slice, int lane, const int taps[]) {
    if (slice->hal->hal_set_ffe_taps_fine == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_ffe_taps_fine(slice, lane, taps);
}

static inline CredoError_t hal_set_f1over3(CredoSlice_t* slice, int lane, int value) {
    if (slice->hal->hal_set_f1over3 == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_f1over3(slice, lane, value);
}

static inline CredoError_t hal_set_agcgain(CredoSlice_t* slice, int lane, unsigned value[]) {
    if (slice->hal->hal_set_agcgain == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_agcgain(slice, lane, value);
}

static inline CredoError_t hal_set_ctle(CredoSlice_t* slice, int lane, unsigned ctle[]) {
    if (slice->hal->hal_set_ctle == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_ctle(slice, lane, ctle);
}

static inline CredoError_t hal_set_delta_phase(CredoSlice_t* slice, int lane, int value) {
    if (slice->hal->hal_set_delta_phase == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_delta_phase(slice, lane, value);
}

static inline CredoError_t hal_set_edge(CredoSlice_t* slice, int lane, unsigned value) {
    if (slice->hal->hal_set_edge == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_edge(slice, lane, value);
}

static inline CredoError_t hal_get_tx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode) {
    if (slice->hal->hal_get_tx_prbs == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_prbs(slice, lane, enable, mode);
}

static inline CredoError_t hal_get_rx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* mode) {
    if (slice->hal->hal_get_rx_prbs == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_prbs(slice, lane, enable, mode);
}

static inline CredoError_t hal_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    if (slice->hal->hal_set_tx_prbs == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_prbs(slice, lane, enable, mode);
}

static inline CredoError_t hal_set_rx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    if (slice->hal->hal_set_rx_prbs_nrz == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_prbs_nrz(slice, lane, enable, mode);
}

static inline CredoError_t hal_set_rx_prbs_pam4(CredoSlice_t* slice, int lane, int enable,
                                                CredoLanePrbsPattern_t mode) {
    if (slice->hal->hal_set_rx_prbs_pam4 == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_prbs_pam4(slice, lane, enable, mode);
}

static inline CredoError_t hal_set_tx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    if (slice->hal->hal_set_tx_prbs_nrz == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_prbs_nrz(slice, lane, enable, mode);
}

static inline CredoError_t hal_set_tx_prbs_pam4(CredoSlice_t* slice, int lane, int enable,
                                                CredoLanePrbsPattern_t mode) {
    if (slice->hal->hal_set_tx_prbs_pam4 == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_prbs_pam4(slice, lane, enable, mode);
}

static inline CredoError_t hal_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode) {
    if (slice->hal->hal_set_rx_prbs == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rx_prbs(slice, lane, enable, mode);
}

static inline CredoError_t hal_get_rx_prbs_count(CredoSlice_t* slice, int lane, uint32_t* count) {
    if (slice->hal->hal_get_rx_prbs_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_prbs_count(slice, lane, count);
}

static inline CredoError_t hal_get_rx_prbs_ber(CredoSlice_t* slice, int lane, int time_ms, double* ber) {
    if (slice->hal->hal_get_rx_prbs_ber == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_prbs_ber(slice, lane, time_ms, ber);
}

static inline CredoError_t hal_reset_rx_prbs_count(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_reset_rx_prbs_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_reset_rx_prbs_count(slice, lane);
}

static inline CredoError_t hal_generate_tx_prbs_error(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_generate_tx_prbs_error == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_generate_tx_prbs_error(slice, lane);
}

static inline CredoError_t hal_get_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool* enable) {
    if (slice->hal->hal_get_tx_test_pattern_enable == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_test_pattern_enable(slice, lane, enable);
}

static inline CredoError_t hal_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable) {
    if (slice->hal->hal_set_tx_test_pattern_enable == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_test_pattern_enable(slice, lane, enable);
}

static inline CredoError_t hal_get_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t* pattern) {
    if (slice->hal->hal_get_tx_test_pattern_memory == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_test_pattern_memory(slice, lane, pattern);
}

static inline CredoError_t hal_set_tx_test_pattern_memory(CredoSlice_t* slice, int lane, uint64_t pattern) {
    if (slice->hal->hal_set_tx_test_pattern_memory == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_test_pattern_memory(slice, lane, pattern);
}

static inline CredoError_t hal_get_tx_test_pattern_mode(CredoSlice_t* slice, int lane,
                                                        CredoLaneTxTestPatternMode* mode) {
    if (slice->hal->hal_get_tx_test_pattern_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_test_pattern_mode(slice, lane, mode);
}

static inline CredoError_t hal_set_tx_test_pattern_mode(CredoSlice_t* slice, int lane,
                                                        CredoLaneTxTestPatternMode mode) {
    if (slice->hal->hal_set_tx_test_pattern_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_test_pattern_mode(slice, lane, mode);
}

static inline CredoError_t hal_tx_disable(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_tx_disable == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tx_disable(slice, lane);
}

static inline CredoError_t hal_tx_no_disable(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_tx_no_disable == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tx_no_disable(slice, lane);
}

static inline CredoError_t hal_lane_tx_status(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status) {
    if (slice->hal->hal_lane_tx_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_lane_tx_status(slice, lane, status);
}

static inline CredoError_t hal_rx_disable(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_rx_disable == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_rx_disable(slice, lane);
}

static inline CredoError_t hal_rx_no_disable(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_rx_no_disable == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_rx_no_disable(slice, lane);
}

static inline CredoError_t hal_lane_rx_reset(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_lane_rx_reset == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_lane_rx_reset(slice, lane);
}

static inline CredoError_t hal_soft_reset(CredoSlice_t* slice) {
    if (slice->hal->hal_soft_reset == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_soft_reset(slice);
}

static inline CredoError_t hal_logic_reset(CredoSlice_t* slice) {
    if (slice->hal->hal_logic_reset == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_logic_reset(slice);
}

static inline CredoError_t hal_logic_reset_lane(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_logic_reset_lane == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_logic_reset_lane(slice, lane);
}

static inline CredoError_t hal_mcu_reset(CredoSlice_t* slice) {
    if (slice->hal->hal_mcu_reset == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mcu_reset(slice);
}

static inline CredoError_t hal_mcu_reset_hold(CredoSlice_t* slice) {
    if (slice->hal->hal_mcu_reset_hold == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mcu_reset_hold(slice);
}

static inline CredoError_t hal_reg_reset(CredoSlice_t* slice) {
    if (slice->hal->hal_reg_reset == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_reg_reset(slice);
}

static inline CredoError_t hal_reg_reset_lane(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_reg_reset_lane == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_reg_reset_lane(slice, lane);
}

static inline CredoError_t hal_set_tx_taps_scale(CredoSlice_t* slice, int lane, const unsigned taps_scale[]) {
    if (slice->hal->hal_set_tx_taps_scale == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_taps_scale(slice, lane, taps_scale);
}

static inline CredoError_t hal_get_tx_taps_scale(CredoSlice_t* slice, int lane, unsigned taps_scale[]) {
    if (slice->hal->hal_get_tx_taps_scale == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_taps_scale(slice, lane, taps_scale);
}

static inline CredoError_t hal_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    if (slice->hal->hal_set_tx_taps == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_taps(slice, lane, taps);
}

static inline CredoError_t hal_set_tx_taps_extended(CredoSlice_t* slice, int lane, const int taps_extended[]) {
    if (slice->hal->hal_set_tx_taps_extended == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_taps_extended(slice, lane, taps_extended);
}

static inline CredoError_t hal_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]) {
    if (slice->hal->hal_get_tx_taps == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_taps(slice, lane, taps);
}

static inline CredoError_t hal_get_tx_taps_extended(CredoSlice_t* slice, int lane, int taps_extended[]) {
    if (slice->hal->hal_get_tx_taps_extended == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_taps_extended(slice, lane, taps_extended);
}

static inline CredoError_t hal_reset_tx_taps(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_reset_tx_taps == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_reset_tx_taps(slice, lane);
}

static inline CredoError_t hal_set_fec_analyzer(CredoSlice_t* slice, int lane, int enable,
                                                CredoFecAnalyzerConfig_t* config) {
    if (slice->hal->hal_set_fec_analyzer == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_fec_analyzer(slice, lane, enable, config);
}

static inline CredoError_t hal_get_fec_analyzer(CredoSlice_t* slice, int lane, int* enable,
                                                CredoFecAnalyzerConfig_t* config) {
    if (slice->hal->hal_get_fec_analyzer == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_fec_analyzer(slice, lane, enable, config);
}

static inline CredoError_t hal_get_fec_analyzer_read_counter(CredoSlice_t* slice, int lane, int counter_sel,
                                                             unsigned* counter) {
    if (slice->hal->hal_get_fec_analyzer_read_counter == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_fec_analyzer_read_counter(slice, lane, counter_sel, counter);
}

static inline CredoError_t hal_get_fec_analyzer_counter(CredoSlice_t* slice, int lane, unsigned* pre_fec,
                                                        unsigned* post_fec) {
    if (slice->hal->hal_get_fec_analyzer_counter == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_fec_analyzer_counter(slice, lane, pre_fec, post_fec);
}

static inline CredoError_t hal_set_fec_analyzer_hist_group(CredoSlice_t* slice, int lane, int group) {
    if (slice->hal->hal_set_fec_analyzer_hist_group == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_fec_analyzer_hist_group(slice, lane, group);
}

static inline CredoError_t hal_fecana_get_hist_group(CredoSlice_t* slice, int lane, int* group) {
    if (slice->hal->hal_fecana_get_hist_group == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fecana_get_hist_group(slice, lane, group);
}

static inline CredoError_t hal_get_fec_analyzer_hist_counter(CredoSlice_t* slice, int lane, unsigned hist_data[4]) {
    if (slice->hal->hal_get_fec_analyzer_hist_counter == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_fec_analyzer_hist_counter(slice, lane, hist_data);
}

static inline CredoError_t hal_get_fec_analyzer_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms) {
    if (slice->hal->hal_get_fec_analyzer_duration == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_fec_analyzer_duration(slice, lane, duration_ms);
}

static inline CredoError_t hal_get_fec_analyzer_error_rate(CredoSlice_t* slice, int lane, int counter_sel,
                                                           int duration_ms, double* error_rate) {
    if (slice->hal->hal_get_fec_analyzer_error_rate == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_fec_analyzer_error_rate(slice, lane, counter_sel, duration_ms, error_rate);
}

static inline CredoError_t hal_get_rsfec_index(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int* index) {
    if (slice->hal->hal_get_rsfec_index == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_index(slice, port_id, side, index);
}

static inline CredoError_t hal_get_rsfec_align_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                      CredoRSFECStatus_t* rsfec_status) {
    if (slice->hal->hal_get_rsfec_align_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_align_status(slice, port_id, side, rsfec_status);
}

static inline CredoError_t hal_get_rsfec_fifo(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                              CredoRSFECFifo_t* rsfec_fifo) {
    if (slice->hal->hal_get_rsfec_fifo == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_fifo(slice, port_id, side, rsfec_fifo);
}

static inline CredoError_t hal_get_rsfec_lane_mapping(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                      unsigned* lane_mapping) {
    if (slice->hal->hal_get_rsfec_lane_mapping == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_lane_mapping(slice, port_id, side, lane_mapping);
}

static inline CredoError_t hal_get_rsfec_histogram(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                   int hist_bin, uint64_t* hist) {
    if (slice->hal->hal_get_rsfec_histogram == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_histogram(slice, port_id, side, hist_bin, hist);
}

static inline CredoError_t hal_get_rsfec_corrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                            uint64_t* corr_cw) {
    if (slice->hal->hal_get_rsfec_corrected_codeword == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_corrected_codeword(slice, port_id, side, corr_cw);
}

static inline CredoError_t hal_get_rsfec_uncorrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                              unsigned* uncorr_cw) {
    if (slice->hal->hal_get_rsfec_uncorrected_codeword == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_uncorrected_codeword(slice, port_id, side, uncorr_cw);
}

static inline CredoError_t hal_get_rsfec_symbol_error(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                      int fec_lane, unsigned* symbol_error) {
    if (slice->hal->hal_get_rsfec_symbol_error == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_symbol_error(slice, port_id, side, fec_lane, symbol_error);
}

static inline CredoError_t hal_get_rsfec_total_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                        uint64_t* total_cw) {
    if (slice->hal->hal_get_rsfec_total_codeword == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_total_codeword(slice, port_id, side, total_cw);
}

static inline CredoError_t hal_get_rsfec_corrected_bits(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                        uint64_t* corrected_bits) {
    if (slice->hal->hal_get_rsfec_corrected_bits == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_corrected_bits(slice, port_id, side, corrected_bits);
}

static inline CredoError_t hal_set_rsfec_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                      bool enable) {
    if (slice->hal->hal_set_rsfec_count_freeze == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_rsfec_count_freeze(slice, port_id, side, enable);
}

static inline CredoError_t hal_get_rsfec_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                      bool* enable) {
    if (slice->hal->hal_get_rsfec_count_freeze == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rsfec_count_freeze(slice, port_id, side, enable);
}

static inline CredoError_t hal_reset_rsfec_count(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side) {
    if (slice->hal->hal_reset_rsfec_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_reset_rsfec_count(slice, port_id, side);
}

static inline CredoError_t hal_set_autoneg_pages(CredoSlice_t* slice, int lane, int pageId, uint64_t page) {
    if (slice->hal->hal_set_autoneg_pages == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_autoneg_pages(slice, lane, pageId, page);
}

static inline CredoError_t hal_get_autoneg_exchanged_pages(CredoSlice_t* slice, int lane, int* page_count,
                                                           uint64_t transmitted_pages[9], uint64_t received_pages[9]) {
    if (slice->hal->hal_get_autoneg_exchanged_pages == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_autoneg_exchanged_pages(slice, lane, page_count, transmitted_pages, received_pages);
}

static inline CredoError_t hal_get_tcm_burst_width(CredoSlice_t* slice, unsigned address, TCMBurstWidth_t* width) {
    if (slice->hal->hal_get_tcm_burst_width == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tcm_burst_width(slice, address, width);
}

static inline CredoError_t hal_tcm_get_base_address(CredoSlice_t* slice, const RegHive_t* hive, unsigned* base_addr) {
    if (slice->hal->hal_tcm_get_base_address == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tcm_get_base_address(slice, hive, base_addr);
}

static inline CredoError_t hal_tcm_read(CredoSlice_t* slice, unsigned addr, unsigned* data) {
    if (slice->hal->hal_tcm_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tcm_read(slice, addr, data);
}

static inline CredoError_t hal_tcm_write(CredoSlice_t* slice, unsigned addr, unsigned data) {
    if (slice->hal->hal_tcm_write == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tcm_write(slice, addr, data);
}

static inline CredoError_t hal_tcm_burst_read(CredoSlice_t* slice, unsigned first_address, uint64_t val[],
                                              unsigned count) {
    if (slice->hal->hal_tcm_burst_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tcm_burst_read(slice, first_address, val, count);
}

static inline CredoError_t hal_tcm_burst_write(CredoSlice_t* slice, unsigned first_address, const uint64_t val[],
                                               unsigned count) {
    if (slice->hal->hal_tcm_burst_write == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tcm_burst_write(slice, first_address, val, count);
}

static inline CredoError_t hal_pcs_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                        unsigned* val) {
    if (slice->hal->hal_pcs_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_pcs_read(slice, portId, side, offset, val);
}

static inline CredoError_t hal_pcs_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                         unsigned val) {
    if (slice->hal->hal_pcs_write == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_pcs_write(slice, portId, side, offset, val);
}

static inline CredoError_t hal_pcs_status_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                               unsigned* pcs_status) {
    if (slice->hal->hal_pcs_status_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_pcs_status_read(slice, portId, side, pcs_status);
}

static inline CredoError_t hal_rs_fec_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                           unsigned* val) {
    if (slice->hal->hal_rs_fec_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_rs_fec_read(slice, portId, side, offset, val);
}

static inline CredoError_t hal_rs_fec_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                            unsigned val) {
    if (slice->hal->hal_rs_fec_write == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_rs_fec_write(slice, portId, side, offset, val);
}

static inline CredoError_t hal_mac_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                        unsigned* val) {
    if (slice->hal->hal_mac_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mac_read(slice, portId, side, offset, val);
}

static inline CredoError_t hal_mac_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                         unsigned val) {
    if (slice->hal->hal_mac_write == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mac_write(slice, portId, side, offset, val);
}

static inline CredoError_t hal_mac_status_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                               unsigned* mac_status) {
    if (slice->hal->hal_mac_status_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mac_status_read(slice, portId, side, mac_status);
}

static inline CredoError_t hal_mac_stats_read(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                              unsigned* val) {
    if (slice->hal->hal_mac_stats_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mac_stats_read(slice, portId, side, offset, val);
}

static inline CredoError_t hal_mac_stats_write(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, unsigned offset,
                                               unsigned val) {
    if (slice->hal->hal_mac_stats_write == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mac_stats_write(slice, portId, side, offset, val);
}

static inline CredoError_t hal_mac_stats_read_counters(CredoSlice_t* slice, uint32_t portId, CredoSide_t side,
                                                       CredoMACStatistics_t* stats) {
    if (slice->hal->hal_mac_stats_read_counters == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mac_stats_read_counters(slice, portId, side, stats);
}

static inline CredoError_t hal_mac_stats_clear_counters(CredoSlice_t* slice, uint32_t portId, CredoSide_t side) {
    if (slice->hal->hal_mac_stats_clear_counters == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mac_stats_clear_counters(slice, portId, side);
}

static inline CredoError_t hal_mac_stop(CredoSlice_t* slice, uint32_t portId) {
    if (slice->hal->hal_mac_stop == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_mac_stop(slice, portId);
}

static inline CredoError_t hal_eip_stop(CredoSlice_t* slice, uint32_t portId) {
    if (slice->hal->hal_eip_stop == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_eip_stop(slice, portId);
}

static inline CredoError_t hal_configure_eip(CredoSlice_t* slice, uint32_t portId) {
    if (slice->hal->hal_configure_eip == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_configure_eip(slice, portId);
}

static inline CredoError_t hal_configure_mac(CredoSlice_t* slice, uint32_t portId) {
    if (slice->hal->hal_configure_mac == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_configure_mac(slice, portId);
}

static inline CredoError_t hal_display_topology_types(CredoSlice_t* slice) {
    if (slice->hal->hal_display_topology_types == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_display_topology_types(slice);
}

static inline CredoError_t hal_set_topology(CredoSlice_t* slice, const char* topology) {
    if (slice->hal->hal_set_topology == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_topology(slice, topology);
}

static inline CredoError_t hal_get_topology(CredoSlice_t* slice, char* topology, uint32_t len) {
    if (slice->hal->hal_get_topology == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_topology(slice, topology, len);
}

static inline CredoError_t hal_fault_propagation_control(CredoSlice_t* slice, uint32_t portId,
                                                         CredoFaultPropagation_t enable) {
    if (slice->hal->hal_fault_propagation_control == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fault_propagation_control(slice, portId, enable);
}

static inline CredoError_t hal_fault_propagation_status(CredoSlice_t* slice, uint32_t portId,
                                                        CredoFaultPropagation_t* enable) {
    if (slice->hal->hal_fault_propagation_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fault_propagation_status(slice, portId, enable);
}

static inline CredoError_t hal_enable_low_latency_bypass(CredoSlice_t* slice, uint32_t port_id,
                                                         CredoMACsecDirection_t direction, uint8_t enable) {
    if (slice->hal->hal_enable_low_latency_bypass == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_enable_low_latency_bypass(slice, port_id, direction, enable);
}

static inline CredoError_t hal_low_latency_bypass_status(CredoSlice_t* slice, uint32_t port_id,
                                                         CredoMACsecDirection_t direction, uint8_t* enable) {
    if (slice->hal->hal_low_latency_bypass_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_low_latency_bypass_status(slice, port_id, direction, enable);
}

static inline CredoError_t hal_get_client_id(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                             uint32_t* sys_client) {
    if (slice->hal->hal_get_client_id == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_client_id(slice, port_id, side, sys_client);
}

static inline CredoError_t hal_get_channel_id(CredoSlice_t* slice, uint32_t port_id, uint32_t* channel_id) {
    if (slice->hal->hal_get_channel_id == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_channel_id(slice, port_id, channel_id);
}

static inline CredoError_t hal_get_macsec_datapath(CredoSlice_t* slice, CredoMACsecDirection_t direction,
                                                   CredoMACsecDataPath_t* datapath) {
    if (slice->hal->hal_get_macsec_datapath == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_macsec_datapath(slice, direction, datapath);
}

static inline CredoError_t hal_get_rx_prbs_duration(CredoSlice_t* slice, int lane, unsigned long* duration_ms) {
    if (slice->hal->hal_get_rx_prbs_duration == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_prbs_duration(slice, lane, duration_ms);
}

static inline CredoError_t hal_slice_set_mdio_mode(CredoSlice_t* slice, bool is_push_pull) {
    if (slice->hal->hal_slice_set_mdio_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_set_mdio_mode(slice, is_push_pull);
}

static inline CredoError_t hal_get_rx_prbs_lock(CredoSlice_t* slice, int lane, CredoPrbsLockStatus_t* status) {
    if (slice->hal->hal_get_rx_prbs_lock == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_rx_prbs_lock(slice, lane, status);
}

static inline CredoError_t hal_packet_inject(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                             CredoPacketInjectConfig_t* pkt_config) {
    if (slice->hal->hal_packet_inject == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_packet_inject(slice, port_id, side, pkt_config);
}

static inline CredoError_t hal_get_param_val_count(CredoSlice_t* slice, ParamDomain_t domain, const char* name,
                                                   int index, int* count) {
    if (slice->hal->hal_get_param_val_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_param_val_count(slice, domain, name, index, count);
}

static inline CredoError_t hal_get_param_val_set_count(CredoSlice_t* slice, ParamDomain_t domain, const char* name,
                                                       int index, int* count) {
    if (slice->hal->hal_get_param_val_set_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_param_val_set_count(slice, domain, name, index, count);
}

static inline CredoError_t hal_get_param_count(CredoSlice_t* slice, ParamDomain_t domain, int* count) {
    if (slice->hal->hal_get_param_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_param_count(slice, domain, count);
}
static inline CredoError_t hal_index_param_def(CredoSlice_t* slice, ParamDomain_t domain, int param_index,
                                               CredoParam_t* param) {
    if (slice->hal->hal_index_param_def == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_index_param_def(slice, domain, param_index, param);
}
static inline CredoError_t hal_find_param_def(CredoSlice_t* slice, ParamDomain_t domain, const char* name, bool* found,
                                              CredoParam_t* param) {
    if (slice->hal->hal_find_param_def == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_find_param_def(slice, domain, name, found, param);
}

static inline CredoError_t hal_get_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    if (slice->hal->hal_get_tx_lane_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_tx_lane_mode(slice, lane, mode);
}

static inline CredoError_t hal_set_tx_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode) {
    if (slice->hal->hal_set_tx_lane_mode == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_tx_lane_mode(slice, lane, mode);
}

static inline CredoError_t hal_fw_get_tx_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    if (slice->hal->hal_fw_get_tx_lane_speed == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_get_tx_lane_speed(slice, lane, speed_kbps);
}

static inline CredoError_t hal_fw_atomic_read(CredoSlice_t* slice, unsigned addr, int len, unsigned* data) {
    if (slice->hal->hal_fw_atomic_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_atomic_read(slice, addr, len, data);
}

static inline CredoError_t hal_set_option(CredoSlice_t* slice, OptionDomain_t type, int index, const char* option,
                                          int value) {
    if (slice->hal->hal_set_option == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_set_option(slice, type, index, option, value);
}

static inline CredoError_t hal_get_option(CredoSlice_t* slice, OptionDomain_t type, int index, const char* option,
                                          int* value) {
    if (slice->hal->hal_get_option == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_option(slice, type, index, option, value);
}

static inline CredoError_t hal_get_option_count(CredoSlice_t* slice, OptionDomain_t type, int* count) {
    if (slice->hal->hal_get_option == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_option_count(slice, type, count);
}

static inline CredoError_t hal_get_option_definition(CredoSlice_t* slice, OptionDomain_t type, int index,
                                                     CredoOption_t* option) {
    if (slice->hal->hal_get_option_definition == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_option_definition(slice, type, index, option);
}

static inline CredoError_t hal_port_is_link_up(CredoSlice_t* slice, uint32_t portId, CredoSide_t side, bool* up) {
    if (slice->hal->hal_port_is_link_up == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_port_is_link_up(slice, portId, side, up);
}

static inline CredoError_t hal_get_hw_tx_precoder(CredoSlice_t* slice, int lane, int* tx_pc) {
    if (slice->hal->hal_get_hw_tx_precoder == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_hw_tx_precoder(slice, lane, tx_pc);
}

static inline CredoError_t hal_slice_data_init(CredoSlice_t* slice) {
    if (slice->hal->hal_slice_data_init == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_data_init(slice);
}

static inline CredoError_t hal_port_build(CredoSlice_t* slice, uint32_t port_id, const CredoPortSetup_t* setup) {
    if (slice->hal->hal_port_build == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_port_build(slice, port_id, setup);
}

static inline CredoError_t hal_port_start(CredoSlice_t* slice, uint32_t port_id, bool force) {
    if (slice->hal->hal_port_start == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_port_start(slice, port_id, force);
}

static inline CredoError_t hal_port_get_setup(CredoSlice_t* slice, uint32_t port_id, bool* launched,
                                              CredoPortSetup_t* setup) {
    if (slice->hal->hal_port_get_setup == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_port_get_setup(slice, port_id, launched, setup);
}

static inline CredoError_t hal_port_assign_id(CredoSlice_t* slice, uint32_t* port_id) {
    if (slice->hal->hal_port_assign_id == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_port_assign_id(slice, port_id);
}

static inline CredoError_t hal_serdes_set_tx_taps_preset(CredoSlice_t* slice, int lane, const char* preset) {
    if (slice->hal->hal_serdes_set_tx_taps_preset == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_serdes_set_tx_taps_preset(slice, lane, preset);
}

static inline CredoError_t hal_tcm_memset(CredoSlice_t* slice, const CredoTCMBurstIOCtrl_t* io_ctrl, unsigned value) {
    if (slice->hal->hal_tcm_memset == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tcm_memset(slice, io_ctrl, value);
}

static inline CredoError_t hal_tcm_range_program(CredoSlice_t* slice, int index,
                                                 const CredoTCMBurstIORangeProgram_t* range_param) {
    if (slice->hal->hal_tcm_range_program == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tcm_range_program(slice, index, range_param);
}

static inline CredoError_t hal_tcm_multi_slice_range_read(CredoSlice_t* slices[], int slice_count,
                                                          const CredoTCMBurstIORange_t* io_range[],
                                                          CredoTCMBurstIOData_t* data[]) {
    if (slices[0]->hal->hal_tcm_multi_slice_range_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slices[0]->hal->hal_tcm_multi_slice_range_read(slices, slice_count, io_range, data);
}

static inline CredoError_t hal_fw_config_phy(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed,
                                             uint64_t flags) {
    if (slice->hal->hal_fw_config_phy == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_config_phy(slice, lane, lane_mode, speed, flags);
}

static inline CredoError_t hal_fw_deconfig_phy(CredoSlice_t* slice, int lane, uint64_t flags) {
    if (slice->hal->hal_fw_deconfig_phy == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_deconfig_phy(slice, lane, flags);
}

static inline CredoError_t hal_tcm_get_device_id(CredoSlice_t* slice, uint32_t port_id, TCMCoreId_t core,
                                                 CredoSide_t side, CredoTCMDevice_t* tcm_device) {
    if (slice->hal->hal_tcm_get_device_id == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tcm_get_device_id(slice, port_id, core, side, tcm_device);
}

static inline CredoError_t hal_tcm_get_reg_space(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                 CredoTCMRegSpace_t* reg_space) {
    if (slice->hal->hal_tcm_get_reg_space == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_tcm_get_reg_space(slice, port_id, side, reg_space);
}
static inline CredoError_t hal_serdes_preset_tx_taps(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    if (slice->hal->hal_serdes_preset_tx_taps == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_serdes_preset_tx_taps(slice, lane, desc);
}
static inline CredoError_t hal_setup_capture_packet(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                    CredoPacketCaptureConfig_t* pkt_config) {
    if (slice->hal->hal_setup_capture_packet == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_setup_capture_packet(slice, port_id, side, pkt_config);
}

static inline CredoError_t hal_stop_capture_packet(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side) {
    if (slice->hal->hal_stop_capture_packet == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_stop_capture_packet(slice, port_id, side);
}

static inline CredoError_t hal_get_capture_packet_buffer_status(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                                CredoPacketCaptureBufferStatus_t* status) {
    if (slice->hal->hal_get_capture_packet_buffer_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_capture_packet_buffer_status(slice, port_id, side, status);
}

static inline CredoError_t hal_clear_capture_packet_buffer(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side) {
    if (slice->hal->hal_clear_capture_packet_buffer == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_clear_capture_packet_buffer(slice, port_id, side);
}

static inline CredoError_t hal_get_capture_packet_info(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                       CredoPacketCaptureInfo_t* pkt_info) {
    if (slice->hal->hal_get_capture_packet_info == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_get_capture_packet_info(slice, port_id, side, pkt_info);
}

static inline CredoError_t hal_prbs_training_rx_get_status(CredoSlice_t* slice, int lane,
                                                           CredoPrbsTrainingStatus_t* status) {
    if (slice->hal->hal_prbs_training_rx_get_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_training_rx_get_status(slice, lane, status);
}

static inline CredoError_t hal_prbs_training_rx_relink(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_prbs_training_rx_relink == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_training_rx_relink(slice, lane);
}

static inline CredoError_t hal_prbs_training_tx_enable(CredoSlice_t* slice, int lane, bool enable) {
    if (slice->hal->hal_prbs_training_tx_enable == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_training_tx_enable(slice, lane, enable);
}

static inline CredoError_t hal_prbs_training_tx_is_enabled(CredoSlice_t* slice, int lane, bool* enabled) {
    if (slice->hal->hal_prbs_training_tx_is_enabled == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_training_tx_is_enabled(slice, lane, enabled);
}

static inline CredoError_t hal_slice_index_reghive(CredoSlice_t* slice, int index, const RegHive_t** reghive) {
    if (slice->hal->hal_slice_index_reghive == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_index_reghive(slice, index, reghive);
}

static inline CredoError_t hal_slice_get_reghive_count(CredoSlice_t* slice, unsigned* count) {
    if (slice->hal->hal_slice_get_reghive_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_slice_get_reghive_count(slice, count);
}

static inline CredoError_t hal_prbs_get_rx_ber_all(CredoSlice_t* slice, const int lanes[], unsigned time_ms,
                                                   double ber[], unsigned count) {
    if (slice->hal->hal_prbs_get_rx_ber_all == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_get_rx_ber_all(slice, lanes, time_ms, ber, count);
}

static inline CredoError_t hal_link_training_get_status(CredoSlice_t* slice, int lane,
                                                        CredoLinkTrainingStatus_t* lt_status) {
    if (slice->hal->hal_link_training_get_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_link_training_get_status(slice, lane, lt_status);
}

static inline CredoError_t hal_link_training_get_state(CredoSlice_t* slice, int lane,
                                                       CredoLinkTrainingState_t* lt_state) {
    if (slice->hal->hal_link_training_get_state == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_link_training_get_state(slice, lane, lt_state);
}

static inline CredoError_t hal_fw_clear_top_pll_cal(CredoSlice_t* slice) {
    if (slice->hal->hal_fw_clear_top_pll_cal == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_clear_top_pll_cal(slice);
}

static inline CredoError_t hal_autoneg_get_state(CredoSlice_t* slice, int lane, CredoAutoNegState_t* state) {
    if (slice->hal->hal_autoneg_get_state == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_autoneg_get_state(slice, lane, state);
}
static inline CredoError_t hal_autoneg_get_restart_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (slice->hal->hal_autoneg_get_restart_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_autoneg_get_restart_count(slice, lane, count);
}
static inline CredoError_t hal_link_training_get_restart_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (slice->hal->hal_link_training_get_restart_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_link_training_get_restart_count(slice, lane, count);
}

static inline CredoError_t hal_data_capture(CredoSlice_t* slice, const char* command,
                                            const CredoDataCaptureArg_t argv[], size_t argc, CredoDataWriter_t writer,
                                            void* ud) {
    if (slice->hal->hal_data_capture == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_data_capture(slice, command, argv, argc, writer, ud);
}

static inline CredoError_t hal_display_info(CredoSlice_t* slice, const char* argv[], size_t argc,
                                            CredoDisplayWriter_t writer, void* ud) {
    if (slice->hal->hal_display_info == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_display_info(slice, argv, argc, writer, ud);
}

static inline CredoError_t hal_frecov_configure(CredoSlice_t* slice, int lane, unsigned timeout_ms) {
    if (slice->hal->hal_frecov_configure == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_frecov_configure(slice, lane, timeout_ms);
}

static inline CredoError_t hal_frecov_get_status(CredoSlice_t* slice, int lane, unsigned* timeout_ms,
                                                 CredoFastRecoveryStatus_t* status) {
    if (slice->hal->hal_frecov_get_status == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_frecov_get_status(slice, lane, timeout_ms, status);
}

static inline CredoError_t hal_frecov_get_recover_count(CredoSlice_t* slice, int lane, unsigned* count) {
    if (slice->hal->hal_frecov_get_recover_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_frecov_get_recover_count(slice, lane, count);
}

static inline CredoError_t hal_fecana_reset(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_fecana_reset == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fecana_reset(slice, lane);
}

static inline CredoError_t hal_fecana_get_autosync(CredoSlice_t* slice, int lane, bool* enable) {
    if (slice->hal->hal_fecana_get_autosync == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fecana_get_autosync(slice, lane, enable);
}

static inline CredoError_t hal_prbs_get_rx_autosync(CredoSlice_t* slice, int lane, bool* enable) {
    if (slice->hal->hal_prbs_get_rx_autosync == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_get_rx_autosync(slice, lane, enable);
}

static inline CredoError_t hal_fw_spiflash_set_partition(CredoSlice_t* slice, int partition_num) {
    if (slice->hal->hal_fw_spiflash_set_partition == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_spiflash_set_partition(slice, partition_num);
}

static inline CredoError_t hal_param_get_data(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                              CredoParamData_t* data) {
    if (slice->hal->hal_param_get_data == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_param_get_data(slice, domain, name, index, data);
}

static inline CredoError_t hal_param_set_data(CredoSlice_t* slice, ParamDomain_t domain, const char* name, int index,
                                              const CredoParamData_t* data) {
    if (slice->hal->hal_param_set_data == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_param_set_data(slice, domain, name, index, data);
}

static inline CredoError_t hal_serdes_get_param(CredoSlice_t* slice, const char* name, int index,
                                                CredoParamData_t* data) {
    return hal_param_get_data(slice, PARAM_DOMAIN_SERDES, name, index, data);
}

static inline CredoError_t hal_serdes_set_param(CredoSlice_t* slice, const char* name, int index,
                                                const CredoParamData_t* data) {
    return hal_param_set_data(slice, PARAM_DOMAIN_SERDES, name, index, data);
}

static inline CredoError_t hal_lane_get_param(CredoSlice_t* slice, const char* name, int index,
                                              CredoParamData_t* data) {
    return hal_param_get_data(slice, PARAM_DOMAIN_LANE, name, index, data);
}

static inline CredoError_t hal_lane_set_param(CredoSlice_t* slice, const char* name, int index,
                                              const CredoParamData_t* data) {
    return hal_param_set_data(slice, PARAM_DOMAIN_LANE, name, index, data);
}
static inline CredoError_t hal_port_get_param(CredoSlice_t* slice, const char* name, int index,
                                              CredoParamData_t* data) {
    return hal_param_get_data(slice, PARAM_DOMAIN_PORT, name, index, data);
}

static inline CredoError_t hal_port_set_param(CredoSlice_t* slice, const char* name, int index,
                                              const CredoParamData_t* data) {
    return hal_param_set_data(slice, PARAM_DOMAIN_PORT, name, index, data);
}

static inline CredoError_t hal_slice_get_param(CredoSlice_t* slice, const char* name, int index,
                                               CredoParamData_t* data) {
    return hal_param_get_data(slice, PARAM_DOMAIN_SLICE, name, index, data);
}

static inline CredoError_t hal_slice_set_param(CredoSlice_t* slice, const char* name, int index,
                                               const CredoParamData_t* data) {
    return hal_param_set_data(slice, PARAM_DOMAIN_SLICE, name, index, data);
}

static inline const char* hal_addr_stringify(CredoSlice_t* slice, int address) {
    if (slice->hal->hal_addr_stringify == NULL) return NULL;
    return slice->hal->hal_addr_stringify(slice, address);
}

static inline CredoError_t hal_testpoint_select(CredoSlice_t* slice, const CredoTestPoint_t* testpoint) {
    if (slice->hal->hal_testpoint_select == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_testpoint_select(slice, testpoint);
}

static inline CredoError_t hal_testpoint_clear(CredoSlice_t* slice) {
    if (slice->hal->hal_testpoint_clear == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_testpoint_clear(slice);
}

static inline CredoError_t hal_testpoint_read(CredoSlice_t* slice, double* value) {
    if (slice->hal->hal_testpoint_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_testpoint_read(slice, value);
}

static inline CredoError_t hal_time_start(CredoSlice_t* slice, double* unix_time) {
    if (slice->hal->hal_time_start == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_time_start(slice, unix_time);
}

static inline CredoError_t hal_time_system(CredoSlice_t* slice, double* timedelta) {
    if (slice->hal->hal_time_system == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_time_system(slice, timedelta);
}

static inline CredoError_t hal_efuse_read(CredoSlice_t* slice, int bank, uint32_t* val) {
    if (slice->hal->hal_efuse_read == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_efuse_read(slice, bank, val);
}

static inline CredoError_t hal_efuse_read_ecid(CredoSlice_t* slice, uint32_t ecid[2]) {
    if (slice->hal->hal_efuse_read_ecid == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_efuse_read_ecid(slice, ecid);
}

static inline CredoError_t hal_prbs_pattern_rx_set_prev(CredoSlice_t* slice, int lane, bool en, unsigned prev) {
    if (slice->hal->hal_prbs_pattern_rx_set_prev == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_pattern_rx_set_prev(slice, lane, en, prev);
}

static inline CredoError_t hal_prbs_pattern_rx_reset_count(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_prbs_pattern_rx_reset_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_pattern_rx_reset_count(slice, lane);
}

static inline CredoError_t hal_prbs_pattern_rx_get_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]) {
    if (slice->hal->hal_prbs_pattern_rx_get_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_pattern_rx_get_count(slice, lane, pattern_count);
}

static inline CredoError_t hal_prbs_pattern_rx_set_phase(CredoSlice_t* slice, int lane, unsigned phase) {
    if (slice->hal->hal_prbs_pattern_rx_set_phase == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_pattern_rx_set_phase(slice, lane, phase);
}

// get tx speed but assumes tx_speed == rx_speed if custom func doesnt exist
static inline CredoError_t hal_lane_get_tx_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    if (slice->hal->hal_fw_get_tx_lane_speed == NULL) {
        return hal_fw_get_lane_speed(slice, lane, speed_kbps);
    }
    return hal_fw_get_tx_lane_speed(slice, lane, speed_kbps);
}
// get tx mode but assumes tx_mode == rx_mode if custom func doesnt exist
static inline CredoError_t hal_lane_get_tx_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    if (slice->hal->hal_get_tx_lane_mode == NULL) {
        return hal_get_lane_mode(slice, lane, mode);
    }
    return hal_get_tx_lane_mode(slice, lane, mode);
}

static inline CredoError_t hal_prbs_set_tx_checker(CredoSlice_t* slice, int lane, int en, CredoPrbsPattern_t pattern) {
    if (slice->hal->hal_prbs_set_tx_checker == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_set_tx_checker(slice, lane, en, pattern);
}

static inline CredoError_t hal_prbs_get_tx_checker(CredoSlice_t* slice, int lane, int* en,
                                                   CredoPrbsPattern_t* pattern) {
    if (slice->hal->hal_prbs_get_tx_checker == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_get_tx_checker(slice, lane, en, pattern);
}

static inline CredoError_t hal_prbs_reset_tx_count(CredoSlice_t* slice, int lane) {
    if (slice->hal->hal_prbs_reset_tx_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_reset_tx_count(slice, lane);
}

static inline CredoError_t hal_prbs_get_tx_count(CredoSlice_t* slice, int lane, unsigned tx_count[2]) {
    if (slice->hal->hal_prbs_get_tx_count == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_get_tx_count(slice, lane, tx_count);
}

static inline CredoError_t hal_prbs_get_tx_duration(CredoSlice_t* slice, int lane, unsigned long* tx_duration) {
    if (slice->hal->hal_prbs_get_tx_duration == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_get_tx_duration(slice, lane, tx_duration);
}

static inline CredoError_t hal_prbs_get_tx_ber(CredoSlice_t* slice, int lane, unsigned time_ms, double tx_ber[2]) {
    if (slice->hal->hal_prbs_get_tx_ber == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_get_tx_ber(slice, lane, time_ms, tx_ber);
}

static inline CredoError_t hal_prbs_get_tx_ber_all(CredoSlice_t* slice, const int lanes[], unsigned time_ms,
                                                   double tx_ber[][2], unsigned count) {
    if (slice->hal->hal_prbs_get_tx_ber_all == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_prbs_get_tx_ber_all(slice, lanes, time_ms, tx_ber, count);
}

static inline CredoError_t hal_fw_ready_post_actions(CredoSlice_t* slice) {
    if (slice->hal->hal_fw_ready_post_actions == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_fw_ready_post_actions(slice);
}

static inline CredoError_t hal_reg_get_notepad(CredoSlice_t* slice, unsigned* register_notepad) {
    if (slice->hal->hal_reg_get_notepad == NULL) return CR_NOTIMPLEMENTED_HAL;
    return slice->hal->hal_reg_get_notepad(slice, register_notepad);
}

#endif
