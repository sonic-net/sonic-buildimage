#ifndef BE_FUNCTIONS_H
#define BE_FUNCTIONS_H

#include "be_device.h"
#include "be_fw_helper.h"

CredoError_t be_top_pll_init(CredoSlice_t* slice);
CredoError_t be_top_pll_cal(CredoSlice_t* slice, int lane);
CredoError_t be_get_top_pll_cap(CredoSlice_t* slice, unsigned* caps);

CredoError_t be_get_lane_count(CredoSlice_t* slice, int* host_lane, int* line_lane);
unsigned be_is_valid_lane(CredoSlice_t* slice, int lane);
// Info functions

CredoError_t be_display_info(CredoSlice_t* slice, const char* argv[], size_t argc, CredoDisplayWriter_t writer,
                             void* ud);
unsigned be_fw_speed_kbps(FirmwareSpeed_t speed_index);

CredoError_t be_fw_translate_lane_speed(unsigned speed, FirmwareSpeed_t* fw_speed);
CredoError_t be_fw_translate_lane_speed_reverse(FirmwareSpeed_t fw_speed, unsigned* speed);
// Firmware functions

CredoError_t be_fw_deconfig_cmd(CredoSlice_t* slice, unsigned config_mode);
CredoError_t be_fw_phy_ready(CredoSlice_t* slice, unsigned* rdy);
CredoError_t be_fw_phy_lane_ready(CredoSlice_t* slice, int lane, unsigned* rdy);
CredoError_t be_fw_config_phy(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed, uint32_t flags);
CredoError_t be_fw_config_lane(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed);
CredoError_t be_fw_config_lane_loopback(CredoSlice_t* slice, int lane, uint32_t speed);
CredoError_t be_fw_deconfig_lane(CredoSlice_t* slice, int lane);
CredoError_t be_fw_set_nrz_optical_mode(CredoSlice_t* slice, unsigned first_lane, unsigned lane_count, int is_optical);
CredoError_t be_fw_get_nrz_optical_mode(CredoSlice_t* slice, unsigned* optical_mode);
CredoError_t be_fw_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);
CredoError_t be_fw_get_opt_mode(CredoSlice_t* slice, int lane, unsigned* opt_mode);
CredoError_t be_fw_get_an_state(CredoSlice_t* slice, int lane, unsigned* an_state);
CredoError_t be_fw_get_lane_link_training(CredoSlice_t* slice, int lane, unsigned* lt_on);
CredoError_t be_fw_get_status(CredoSlice_t* slice, unsigned* status);
CredoError_t be_fw_download_from_file(CredoSlice_t* slice, const char* image_file);
CredoError_t be_set_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode);
CredoError_t be_get_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode);

CredoError_t be_fw_get_rsfec_index(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int* index);

// Port functions
CredoError_t be_port_config(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force);
CredoError_t be_port_teardown(CredoSlice_t* slice, unsigned port_id);
CredoError_t be_port_teardown_all(CredoSlice_t* slice);
CredoError_t be_port_query(CredoSlice_t* slice, unsigned port_id, CredoPortConfig_t* port_config);

/* TX control */
CredoError_t be_tx_disable(CredoSlice_t* slice, int lane);
CredoError_t be_tx_no_disable(CredoSlice_t* slice, int lane);
CredoError_t be_lane_tx_status(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status);
CredoError_t be_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t mode);
CredoError_t be_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable);

/* RX control */
CredoError_t be_rx_disable(CredoSlice_t* slice, int lane);
CredoError_t be_rx_no_disable(CredoSlice_t* slice, int lane);

/* Lane operation mode */
CredoError_t be_get_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode);
CredoError_t be_set_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode);
CredoError_t be_update_lane_mode(CredoSlice_t* slice, int lane);
CredoError_t be_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps);
CredoError_t be_disable_lane(CredoSlice_t* slice, int lane);

CredoError_t be_reg_reset_lane(CredoSlice_t* slice, int lane);
CredoError_t be_logic_reset_lane(CredoSlice_t* slice, int lane);

CredoError_t be_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm);

CredoError_t be_serdes_preset_tx_taps(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc);

#endif
