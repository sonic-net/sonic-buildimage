#ifndef BH_OPTION_H
#define BH_OPTION_H

#include "sdk.h"

CredoError_t bh_option_get_scratch(CredoSlice_t* slice, const char* name, int* value);
CredoError_t bh_option_set_scratch(CredoSlice_t* slice, const char* name, int value);
CredoError_t bh_option_get_sd_delay(CredoSlice_t* slice, const char* option_name, int* value);
CredoError_t bh_option_set_sd_delay(CredoSlice_t* slice, const char* option_name, int value);
CredoError_t bh_option_get_fifo_auto_recover(CredoSlice_t* slice, const char* name, int* value);
CredoError_t bh_option_set_fifo_auto_recover(CredoSlice_t* slice, const char* name, int value);
CredoError_t bh_option_get_low_latency_1G(CredoSlice_t* slice, const char* name, int* value);
CredoError_t bh_option_set_low_latency_1G(CredoSlice_t* slice, const char* name, int value);
CredoError_t bh_option_get_anlt_power_saving(CredoSlice_t* slice, const char* name, int* value);
CredoError_t bh_option_set_anlt_power_saving(CredoSlice_t* slice, const char* name, int value);
CredoError_t bh_option_get_fec_adv_read_info(CredoSlice_t* slice, const char* name, int* value);
CredoError_t bh_option_set_fec_adv_read_info(CredoSlice_t* slice, const char* name, int value);
CredoError_t bh_option_get_clk_output_squelch(CredoSlice_t* slice, const char* option_name, int* value);
CredoError_t bh_option_set_clk_output_squelch(CredoSlice_t* slice, const char* option_name, int value);
CredoError_t bh_option_get_uncorr_monitor(CredoSlice_t* slice, const char* option_name, int* value);
CredoError_t bh_option_set_uncorr_monitor(CredoSlice_t* slice, const char* option_name, int value);
CredoError_t bh_option_set_gearbox_traffic_gen(CredoSlice_t* slice, const char* option_name, int value);
CredoError_t bh_option_get_gearbox_traffic_gen(CredoSlice_t* slice, const char* option_name, int* value);
CredoError_t bh_lane_option_get_optical_mode(CredoSlice_t* slice, int lane, const char* name, int* value);
CredoError_t bh_lane_option_set_optical_mode(CredoSlice_t* slice, int lane, const char* name, int value);
CredoError_t bh_lane_option_get_generic(CredoSlice_t* slice, int lane, const char* option_name, int* value);
CredoError_t bh_lane_option_set_generic(CredoSlice_t* slice, int lane, const char* option_name, int value);
CredoError_t bh_option_set_acfg_off(CredoSlice_t* slice, const char* option_name, int value);
CredoError_t bh_option_get_acfg_off(CredoSlice_t* slice, const char* option_name, int* value);

#endif
