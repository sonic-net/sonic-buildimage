/*
 *
 * $Id: epdm.h $
 *
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */

#ifndef EPDM_H
#define EPDM_H

#include "epdm_bcm_common_defines.h"
#include "epdm_custom_config.h"
#define __PLP_NOT_USED ((void*)0)

typedef int (*__bcm_plp_mutex_info_set_f) (bcm_plp_access_t phy_info, bcm_plp_mutex_info_t *mutex_info);
typedef int (*__bcm_plp_static_config_set_f)(bcm_plp_access_t phy_info, void* bcm_static_config);
typedef int (*__bcm_plp_static_config_get_f)(bcm_plp_access_t phy_info, void* bcm_static_config);
typedef int (*__bcm_plp_init_f)(bcm_plp_access_t phy_info, int (*read)(void* user_acc,
            unsigned int core_addr, unsigned int reg_addr, unsigned int* val),
            int (*write)(void* user_acc, unsigned int core_addr, unsigned int reg_addr,
             unsigned int val), bcm_pm_firmware_load_method_t firmware_load_method);

typedef int (*__bcm_plp_cleanup_f)(bcm_plp_access_t phy_info);

typedef int (*__bcm_plp_link_status_get_f)(bcm_plp_access_t phy_info, unsigned int *link_status);

typedef int (*__bcm_plp_mode_config_set_f)(bcm_plp_access_t phy_info, int speed, int if_type,
                            int ref_clk, int interface_mode, void* device_aux_modes);

typedef int (*__bcm_plp_mode_config_get_f)(bcm_plp_access_t phy_info, int *speed,
                            int *if_type, int *ref_clk, int *interface_mode,
                            void *device_aux_modes);

typedef int (*__bcm_plp_driver_version_get_f)(bcm_plp_access_t phy_info, char *phy_chip_name, unsigned short *major_ver,
                         unsigned short *minor_ver);

typedef int (*__bcm_plp_prbs_set_f)(bcm_plp_access_t phy_info, unsigned int tx_rx,
                     unsigned int poly, unsigned int invert,
                     unsigned int loopback, unsigned int ena_dis);

typedef int (*__bcm_plp_prbs_get_f)(bcm_plp_access_t phy_info, unsigned int tx_rx,
                     unsigned int *poly, unsigned int *invert,
                     unsigned int *loopback, unsigned int *ena_dis);

typedef int (*__bcm_plp_prbs_rx_stat_f)(bcm_plp_access_t phy_info, unsigned int time);

typedef int (*__bcm_plp_prbs_clear_f)(bcm_plp_access_t phy_info, unsigned int tx_rx);

typedef int (*__bcm_plp_prbs_config_get_f)(bcm_plp_access_t phy_info, unsigned int tx_rx,
                            unsigned int *poly, unsigned int *invert);

typedef int (*__bcm_plp_prbs_status_get_f)(bcm_plp_access_t phy_info, unsigned int *prbs_lock,
                            unsigned int *prbs_lock_loss, unsigned int *error_count);

typedef int (*__bcm_plp_reg_value_set_f)(bcm_plp_access_t phy_info, unsigned int devaddr,
                          unsigned int regaddr, unsigned int data);

typedef int (*__bcm_plp_reg_value_get_f)(bcm_plp_access_t phy_info, unsigned int devaddr,
                          unsigned int regaddr, unsigned int *data);

typedef int (*__bcm_plp_polarity_set_f)(bcm_plp_access_t phy_info, unsigned int tx_pol,
                         unsigned int rx_pol);

typedef int (*__bcm_plp_polarity_get_f)(bcm_plp_access_t phy_info, unsigned int *tx_pol,
                         unsigned int *rx_pol);

typedef int (*__bcm_plp_rx_pmd_lock_get_f)(bcm_plp_access_t phy_info, unsigned int* rx_pmd_lock);

typedef int (*__bcm_plp_rev_id_f)(bcm_plp_access_t phy_info, unsigned int* rev_id);


typedef int (*__bcm_plp_loopback_set_f)(bcm_plp_access_t phy_info, unsigned int lb_mode,
                         unsigned int enable);

typedef int (*__bcm_plp_loopback_get_f)(bcm_plp_access_t phy_info, unsigned int lb_mode,
                         unsigned int *enable);

typedef int (*__bcm_plp_tx_set_f)(bcm_plp_access_t phy_info, bcm_plp_tx_t* tx);

typedef int (*__bcm_plp_tx_get_f)(bcm_plp_access_t phy_info, bcm_plp_tx_t* tx);

typedef int (*__bcm_plp_rx_set_f)(bcm_plp_access_t phy_info, bcm_plp_rx_t* rx);

typedef int (*__bcm_plp_rx_get_f)(bcm_plp_access_t phy_info, bcm_plp_rx_t* rx);

typedef int (*__bcm_plp_reset_set_f)(bcm_plp_access_t phy_info, unsigned int reset_mode,
                      unsigned int reset_val);

typedef int (*__bcm_plp_phy_lane_reset_set_f)(bcm_plp_access_t phy_info, bcm_plp_pm_phy_reset_t* reset);

typedef int (*__bcm_plp_phy_lane_reset_get_f)(bcm_plp_access_t phy_info, bcm_plp_pm_phy_reset_t* reset);

typedef int (*__bcm_plp_tx_lane_control_set_f)(bcm_plp_access_t phy_info, bcm_pm_phy_tx_lane_control_t tx_control);

typedef int (*__bcm_plp_rx_lane_control_set_f)(bcm_plp_access_t phy_info, bcm_pm_phy_rx_lane_control_t rx_control);

typedef int (*__bcm_plp_tx_lane_control_get_f)(bcm_plp_access_t phy_info, bcm_pm_phy_tx_lane_control_t *tx_control);

typedef int (*__bcm_plp_rx_lane_control_get_f)(bcm_plp_access_t phy_info, bcm_pm_phy_rx_lane_control_t *rx_control);

typedef int (*__bcm_plp_lane_cross_switch_map_set_f)(bcm_plp_access_t phy_info, unsigned int* tx_source_array);

typedef int (*__bcm_plp_lane_cross_switch_map_get_f)(bcm_plp_access_t phy_info, unsigned int *mapped_to);

typedef int (*__bcm_plp_force_tx_training_set_f)(bcm_plp_access_t phy_info, unsigned int enable);

typedef int (*__bcm_plp_force_tx_training_get_f)(bcm_plp_access_t phy_info, unsigned int *enable);

typedef int (*__bcm_plp_force_tx_training_status_get_f)(bcm_plp_access_t phy_info, unsigned int *enabled,
                                         unsigned int *training_failure, unsigned int *trained);

typedef int (*__bcm_plp_cl73_ability_set_f)(bcm_plp_access_t phy_info, unsigned short tech_ability,
                             unsigned short fec_ability, unsigned short pause_ability, bcm_plp_an_config_t an_config);

typedef int (*__bcm_plp_cl73_ability_get_f)(bcm_plp_access_t phy_info, unsigned short *tech_ability,
                             unsigned short *fec_ability, unsigned short *pause_ability, bcm_plp_an_config_t* an_config);
typedef int (*__bcm_plp_cl73_set_f)(bcm_plp_access_t phy_info, unsigned short ena_dis);

typedef int (*__bcm_plp_cl73_get_f)(bcm_plp_access_t phy_info, unsigned int *an,
                     unsigned int *an_done);

typedef int (*__bcm_plp_display_eye_scan_f)(bcm_plp_access_t phy_info);

typedef int (*__bcm_plp_firmware_info_get_f)(bcm_plp_access_t phy_info, unsigned int *fw_version,
                              unsigned int *fw_crc);

typedef int (*__bcm_plp_firmware_set_f)(bcm_plp_access_t phy_info);

typedef int (*__bcm_plp_rxtx_laneswap_set_f)(bcm_plp_access_t phy_info, bcm_plp_laneswap_map_t* laneswap_map);

typedef int (*__bcm_plp_rxtx_laneswap_get_f)(bcm_plp_access_t phy_info, bcm_plp_laneswap_map_t* laneswap_map);

typedef int (*__bcm_plp_pll_sequencer_restart_f)(bcm_plp_access_t phy_info, unsigned char flags,
                                  bcm_pm_sequencer_operation_t operation);

typedef int (*__bcm_plp_fec_enable_set_f)(bcm_plp_access_t phy_info, unsigned int enable);

typedef int (*__bcm_plp_fec_enable_get_f)(bcm_plp_access_t phy_info, unsigned int* enable);

typedef int (*__bcm_plp_phy_status_dump_f)(bcm_plp_access_t phy_info);

typedef int (*__bcm_plp_phy_diagnostics_get_f)(bcm_plp_access_t phy_info, bcm_plp_pm_phy_diagnostics_t* diag);

typedef int (*__bcm_plp_intr_status_get_f)(bcm_plp_access_t phy_info, unsigned int intr_type,
                            unsigned int* intr_status);

typedef int (*__bcm_plp_intr_enable_set_f)(bcm_plp_access_t phy_info, unsigned int intr_type,
                            unsigned int enable);

typedef int (*__bcm_plp_intr_enable_get_f)(bcm_plp_access_t phy_info, unsigned int intr_type,
                            unsigned int* enable);

typedef int (*__bcm_plp_intr_status_clear_f)(bcm_plp_access_t phy_info, unsigned int intr_type);

typedef int (*__bcm_plp_fc_pcs_chkr_enable_set_f)(bcm_plp_access_t phy_info, unsigned int fcpcs_chkr_mode,
                                   unsigned int enable);

typedef int (*__bcm_plp_fc_pcs_chkr_enable_get_f)(bcm_plp_access_t phy_info, unsigned int fcpcs_chkr_mode,
                                   unsigned int* enable);

typedef int (*__bcm_plp_fc_pcs_chkr_status_get_f)(bcm_plp_access_t phy_info, unsigned int *lock_status,
                                   unsigned int* lock_lost_lh, unsigned int* error_count);
#ifdef SERDES_API_FLOATING_POINT
typedef int (*__bcm_plp_eye_margin_proj_f)(bcm_plp_access_t phy_info, double rate,
                            unsigned char ber_scan_mode, unsigned char timer_control,
                            unsigned char max_error_control);
#else
typedef int (*__bcm_plp_eye_margin_proj_f)(bcm_plp_access_t phy_info, int rate,
                            unsigned char ber_scan_mode, unsigned char timer_control,
                            unsigned char max_error_control);
#endif

typedef int (*__bcm_plp_repeater_mode_get_f)(bcm_plp_access_t phy_info, unsigned int *ena_dis);

typedef int (*__bcm_plp_repeater_mode_set_f)(bcm_plp_access_t phy_info, unsigned int ena_dis);

typedef int (*__bcm_plp_module_read_f)(bcm_plp_access_t phy_info, unsigned int slv_addr,
                        unsigned int start_addr, unsigned int no_of_bytes,
                        unsigned char *read_data);

typedef int (*__bcm_plp_module_write_f)(bcm_plp_access_t phy_info, unsigned int slv_addr,
                         unsigned int start_addr, unsigned int no_of_bytes,
                         unsigned char *write_data);

typedef int (*__bcm_plp_peripheral_device_control_f)(bcm_plp_access_t phy_info,
                                                     int peripheral_dev, int peripheral_dev_ctl,
                                                     bcm_plp_peripheral_dev_ctrl_data_t *ctrl_data);

typedef int (*__bcm_plp_cfg_gpio_pin_set_f)(bcm_plp_access_t phy_info, unsigned int gpio_pin_number,
                             unsigned int cfg_direction, unsigned int cfg_pull,
                             unsigned int pin_value);

typedef int (*__bcm_plp_cfg_gpio_pin_get_f)(bcm_plp_access_t phy_info, unsigned int gpio_pin_number,
                             unsigned int *cfg_direction, unsigned int *cfg_pull,
                             unsigned int *pin_value);

typedef int (*__bcm_plp_power_set_f)(bcm_plp_access_t phy_info,  unsigned int power_rx, unsigned int power_tx);

typedef int (*__bcm_plp_power_get_f)(bcm_plp_access_t phy_info,  unsigned int *power_rx, unsigned int *power_tx);

typedef int (*__bcm_plp_firmware_lane_config_set_f)(bcm_plp_access_t phy_info, bcm_plp_pm_firmware_lane_config_t* firmware_lane_config);

typedef int (*__bcm_plp_firmware_lane_config_get_f)(bcm_plp_access_t phy_info, bcm_plp_pm_firmware_lane_config_t* firmware_lane_config);
typedef int (*__bcm_plp_init_fw_bcast_f)(bcm_plp_access_t phy_info,
                  int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val),
                  int (*write)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int val),
                  bcm_plp_firmware_load_type_t *firmware_load_type,
                  bcm_plp_firmware_broadcast_method_t broadcast_method);
typedef int (*__bcm_plp_failover_mode_set_f)(bcm_plp_access_t phy_info,unsigned int failover_mode);
typedef int (*__bcm_plp_failover_mode_get_f)(bcm_plp_access_t phy_info,unsigned int *failover_mode);

typedef int (*__bcm_plp_edc_config_set_f)(bcm_plp_access_t phy_info, unsigned int edc_method, unsigned int edc_value );
typedef int (*__bcm_plp_edc_config_get_f)(bcm_plp_access_t phy_info, unsigned int *edc_method, unsigned int *edc_value );

typedef int (*__bcm_plp_fec_corrected_error_counter_f)(bcm_plp_access_t phy_info, bcm_plp_fec_type_t fec_type, unsigned int *count);
typedef int (*__bcm_plp_fec_uncorrected_error_counter_f)(bcm_plp_access_t phy_info, bcm_plp_fec_type_t fec_type, unsigned int *count);

typedef int (*__bcm_plp_event_status_get)(bcm_plp_access_t phy_info, unsigned int event_type, unsigned int *event_status);

typedef int (*__bcm_plp_base_t_autoneg_ability_set_f)(bcm_plp_access_t phy_info, bcm_plp_base_t_ability_t *ability);
typedef int (*__bcm_plp_base_t_autoneg_ability_get_f)(bcm_plp_access_t phy_info, bcm_plp_base_t_ability_t *ability);
typedef int (*__bcm_plp_base_t_autoneg_set_f)(bcm_plp_access_t phy_info, int  enable);
typedef int (*__bcm_plp_base_t_autoneg_get_f)(bcm_plp_access_t phy_info, int *enable, int *an_done);
typedef int (*__bcm_plp_base_t_eee_set_f)(bcm_plp_access_t phy_info, bcm_plp_base_t_eee_t *eee_conf);
typedef int (*__bcm_plp_base_t_eee_get_f)(bcm_plp_access_t phy_info, bcm_plp_base_t_eee_t *eee_conf);
typedef int (*__bcm_plp_base_t_power_mode_set_f)(bcm_plp_access_t phy_info, bcm_plp_base_t_power_mode_t  power_mode);
typedef int (*__bcm_plp_base_t_power_mode_get_f)(bcm_plp_access_t phy_info, bcm_plp_base_t_power_mode_t *power_mode);
typedef int (*__bcm_plp_base_t_cable_diag_f)(bcm_plp_access_t phy_info, bcm_plp_base_t_cable_diag_t *cdiag);
typedef int (*__bcm_plp_base_t_state_mfg_diag_f)(bcm_plp_access_t phy_info, unsigned int inst,
                                                 bcm_plp_base_t_diag_ctrl_type_t op_type,
                                                 bcm_plp_base_t_diag_ctrl_cmd_t  op_cmd, void *arg);
typedef int (*__bcm_plp_l1_intr_status_get_f)(bcm_plp_access_t phy_info, bcm_plp_l1_intr_status_t *l1_intr_status);

typedef int (*__bcm_plp_timesync_config_set)(bcm_plp_access_t phy_info, bcm_plp_timesync_config_t* config);
typedef int (*__bcm_plp_timesync_config_get)(bcm_plp_access_t phy_info, bcm_plp_timesync_config_t* config);
typedef int (*__bcm_plp_timesync_enable_set)(bcm_plp_access_t phy_info, unsigned int flags, unsigned int enable);
typedef int (*__bcm_plp_timesync_enable_get)(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* enable);
typedef int (*__bcm_plp_timesync_mpls_set)(bcm_plp_access_t phy_info, unsigned int flags, int  enable, bcm_plp_timesync_mpls_ctrl_t *mpls_ctrl);
typedef int (*__bcm_plp_timesync_mpls_get)(bcm_plp_access_t phy_info, unsigned int flags, int* enable, bcm_plp_timesync_mpls_ctrl_t *mpls_ctrl);
typedef int (*__bcm_plp_timesync_pch_set)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_pch_t *pch);
typedef int (*__bcm_plp_timesync_pch_get)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_pch_t *pch);

typedef int (*__bcm_plp_timesync_inband_filter_set)(bcm_plp_access_t phy_info, unsigned int flags, int index, bcm_plp_timesync_inband_filter_ctrl_t *config);
typedef int (*__bcm_plp_timesync_inband_filter_get)(bcm_plp_access_t phy_info, unsigned int flags, int index, bcm_plp_timesync_inband_filter_ctrl_t *config);
typedef int (*__bcm_plp_timesync_nco_addend_set)(bcm_plp_access_t phy_info, unsigned int flags,  unsigned int freq_step);
typedef int (*__bcm_plp_timesync_nco_addend_get)(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* freq_step);
typedef int (*__bcm_plp_timesync_framesync_mode_set)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_framesync_t* framesync);
typedef int (*__bcm_plp_timesync_framesync_mode_get)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_framesync_t* framesync);
typedef int (*__bcm_plp_timesync_local_time_set)(bcm_plp_access_t phy_info, unsigned int flags, plp_uint64_t local_time);
typedef int (*__bcm_plp_timesync_local_time_get)(bcm_plp_access_t phy_info, unsigned int flags, plp_uint64_t* local_time);
typedef int (*__bcm_plp_timesync_load_ctrl_set)(bcm_plp_access_t phy_info, unsigned int flags, unsigned int load_once, unsigned int load_always);
typedef int (*__bcm_plp_timesync_load_ctrl_get)(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* load_once, unsigned int* load_always);
typedef int (*__bcm_plp_timesync_timing_control_set)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *time_ctrl);
typedef int (*__bcm_plp_timesync_timing_control_get)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *time_ctrl);
typedef int (*__bcm_plp_timesync_link_delay_set)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *linkdelay);
typedef int (*__bcm_plp_timesync_link_delay_get)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *linkdelay);
typedef int (*__bcm_plp_timesync_timestamp_offset_set)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *ts_offset);
typedef int (*__bcm_plp_timesync_timestamp_offset_get)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *ts_offset);
typedef int (*__bcm_plp_timesync_tx_timestamp_offset_set)(bcm_plp_access_t phy_info, unsigned int flags, unsigned int ts_offset);
typedef int (*__bcm_plp_timesync_tx_timestamp_offset_get)(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* ts_offset);
typedef int (*__bcm_plp_timesync_rx_timestamp_offset_set)(bcm_plp_access_t phy_info, unsigned int flags, unsigned int ts_offset);
typedef int (*__bcm_plp_timesync_rx_timestamp_offset_get)(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* ts_offset);
typedef int (*__bcm_plp_timesync_time_code_set)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_timespec_t *timecode);
typedef int (*__bcm_plp_timesync_time_code_get)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_timespec_t *timecode);
typedef int (*__bcm_plp_timesync_sopmem_set)(bcm_plp_access_t phy_info, unsigned int flags, int index, bcm_plp_timesync_sopmem_t *sopentry);
typedef int (*__bcm_plp_timesync_sopmem_get)(bcm_plp_access_t phy_info, unsigned int flags, int index, bcm_plp_timesync_sopmem_t *sopentry);
typedef int (*__bcm_plp_timestamp_univ_set)(bcm_plp_access_t phy_info, unsigned int flags, int type, bcm_plp_timestamp_univ_t *timeuniv);
typedef int (*__bcm_plp_timestamp_univ_get)(bcm_plp_access_t phy_info, unsigned int flags, int type, bcm_plp_timestamp_univ_t *timeuniv);
typedef int (*__bcm_plp_timesync_ptp_action_set)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_txrx_t  direction, bcm_plp_timesync_ptp_action_mode_t  mode, void *user_def);
typedef int (*__bcm_plp_timesync_ptp_action_get)(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_txrx_t  direction, bcm_plp_timesync_ptp_action_mode_t *mode, void *user_def);
typedef int (*__bcm_plp_timesync_capture_timestamp_get)(bcm_plp_access_t phy_info, unsigned int flags, plp_uint64_t* cap_ts);
typedef int (*__bcm_plp_timesync_heartbeat_timestamp_get)(bcm_plp_access_t phy_info, unsigned int flags, plp_uint64_t* hb_ts);
typedef int (*__bcm_plp_timesync_do_sync)(bcm_plp_access_t phy_info);
typedef int (*__bcm_plp_pam4_tx_set_f)(bcm_plp_access_t phy_info, bcm_plp_pam4_tx_t* tx);
typedef int (*__bcm_plp_pam4_tx_get_f)(bcm_plp_access_t phy_info, bcm_plp_pam4_tx_t* tx);
typedef int (*__bcm_plp_phy_pam4_diagnostics_get)(bcm_plp_access_t phy_info,  bcm_plp_pm_phy_pam4_diagnostics_t* diag);
typedef int (*__bcm_plp_jfec_config_set)(bcm_plp_access_t phy_info, bcm_plp_jfec_config_t jfec_config);
typedef int (*__bcm_plp_fec_status_get)(bcm_plp_access_t phy_info, bcm_plp_fec_status_t *fec_status);
typedef int (*__bcm_plp_kp4_fec_config_get)(bcm_plp_access_t phy_info, bcm_plp_kp4_fec_config_t *kp4_fec_config);
typedef int (*__bcm_plp_kp4_fec_config_set)(bcm_plp_access_t phy_info, bcm_plp_kp4_fec_config_t kp4_fec_config);
typedef int (*__bcm_plp_jfec_config_get)(bcm_plp_access_t phy_info, bcm_plp_jfec_config_t *jfec_config);
typedef int (*__bcm_plp_core_diagnostics_get)(bcm_plp_access_t phy_info, bcm_plp_core_diagnostics_t *core_diag);
typedef int (*__bcm_plp_avs_config_set)(bcm_plp_access_t phy_info, bcm_plp_avs_config_t avs_config);
typedef int (*__bcm_plp_avs_config_get)(bcm_plp_access_t phy_info, bcm_plp_avs_config_t* avs_config);
typedef int (*__bcm_plp_avs_status_get)(bcm_plp_access_t phy_info, bcm_plp_avs_config_status_t* avs_status);
typedef int (*__bcm_plp_prbs_error_inject_set)(bcm_plp_access_t phy_info, bcm_plp_prbs_error_inject_t prbs_error_inject);

typedef int (*__bcm_plp_logical_lane_set_f)(bcm_plp_access_t phy_info, bcm_plp_logical_lane_map_t  logical_lane);
typedef int (*__bcm_plp_logical_lane_get_f)(bcm_plp_access_t phy_info, bcm_plp_logical_lane_map_t* logical_lane);
typedef int (*__bcm_plp_prbs_error_analyzer_proj_f)(bcm_plp_access_t phy_info, unsigned short prbs_error_fec_size, unsigned char hist_errcnt_thresh, unsigned int timeout_s);
typedef int (*__bcm_plp_dsrds_firmware_lane_config_set_f)(bcm_plp_access_t phy_info, bcm_plp_dsrds_firmware_lane_config_t firmware_lane_config);
typedef int (*__bcm_plp_dsrds_firmware_lane_config_get_f)(bcm_plp_access_t phy_info, bcm_plp_dsrds_firmware_lane_config_t* firmware_lane_config);
typedef int (*__bcm_plp_synce_config_set)(bcm_plp_access_t phy_info, bcm_plp_synce_cfg_t* synce_cfg);
typedef int (*__bcm_plp_synce_config_get)(bcm_plp_access_t phy_info, bcm_plp_synce_cfg_t* synce_cfg);
typedef int (*__bcm_autoneg_remote_ability_get_f)(bcm_plp_access_t phy_info, unsigned short *fec_ability, 
                                                  unsigned short *pause_ability, bcm_plp_an_config_t* an_config);
typedef int (*__bcm_plp_pcs_status_get_f)(bcm_plp_access_t phy_info, bcm_plp_pcs_status_t* pcs_status);
typedef int (*__bcm_plp_pattern_enable_set_f)(bcm_plp_access_t phy_info, const bcm_plp_pattern_t* pattern);
typedef int (*__bcm_plp_pattern_enable_get_f)(bcm_plp_access_t phy_info, bcm_plp_pattern_t* pattern);
typedef int (*__bcm_plp_fw_init_params_get_f)(bcm_plp_access_t phy_info, void* const fw_init_params);
typedef int (*__bcm_plp_srds_diag_access_enable_f)(bcm_plp_access_t phy_info, bcm_plp_srds_diag_access_cfg_t *diag_access_cfg);
typedef int (*__bcm_plp_phy_pai_info_get_f)(bcm_plp_access_t phy_info, bcm_plp_pai_info_t *epdm_pai_info);
typedef int (*__bcm_plp_failover_config_set_f)(bcm_plp_access_t phy_info, bcm_plp_failover_config_t failover_config);
typedef int (*__bcm_plp_failover_config_get_f)(bcm_plp_access_t phy_info, bcm_plp_failover_config_t *failover_config);
typedef int (*__bcm_plp_phy_stats_f)(bcm_plp_access_t phy_info, bcm_plp_mib_stat_operation_t op_type, unsigned int * stat_id, unsigned int stats_count, plp_uint64_t *stats_p);

typedef struct __plp__dispatch__s__ {
    __bcm_plp_static_config_set_f __static_config_set;
    __bcm_plp_static_config_get_f __static_config_get;
    __bcm_plp_init_f __init;
    __bcm_plp_cleanup_f __cleanup;
    __bcm_plp_link_status_get_f __link_status_get;
    __bcm_plp_mode_config_set_f __mode_config_set;
    __bcm_plp_mode_config_get_f __mode_config_get;
    __bcm_plp_driver_version_get_f __driver_version_get;
    __bcm_plp_prbs_set_f __prbs_set;
    __bcm_plp_prbs_get_f __prbs_get;
    __bcm_plp_prbs_rx_stat_f __prbs_rx_stat;
    __bcm_plp_prbs_clear_f __prbs_clear;
    __bcm_plp_prbs_config_get_f __prbs_config_get;
    __bcm_plp_prbs_status_get_f __prbs_status_get;
    __bcm_plp_reg_value_set_f __reg_value_set;
    __bcm_plp_reg_value_get_f __reg_value_get;
    __bcm_plp_polarity_set_f __polarity_set;
    __bcm_plp_polarity_get_f __polarity_get;
    __bcm_plp_rx_pmd_lock_get_f __rx_pmd_lock_get;
    __bcm_plp_rev_id_f __rev_id;
    __bcm_plp_loopback_set_f __loopback_set;
    __bcm_plp_loopback_get_f __loopback_get;
    __bcm_plp_tx_set_f __tx_set;
    __bcm_plp_tx_get_f __tx_get;
    __bcm_plp_rx_set_f __rx_set;
    __bcm_plp_rx_get_f __rx_get;
    __bcm_plp_reset_set_f __reset_set;
    __bcm_plp_phy_lane_reset_set_f __phy_lane_reset_set;
    __bcm_plp_phy_lane_reset_get_f __phy_lane_reset_get;
    __bcm_plp_tx_lane_control_set_f __tx_lane_control_set;
    __bcm_plp_rx_lane_control_set_f __rx_lane_control_set;
    __bcm_plp_tx_lane_control_get_f __tx_lane_control_get;
    __bcm_plp_rx_lane_control_get_f __rx_lane_control_get;
    __bcm_plp_lane_cross_switch_map_set_f __lane_cross_switch_map_set;
    __bcm_plp_lane_cross_switch_map_get_f __lane_cross_switch_map_get;
    __bcm_plp_force_tx_training_set_f __force_tx_training_set;
    __bcm_plp_force_tx_training_get_f __force_tx_training_get;
    __bcm_plp_force_tx_training_status_get_f __force_tx_training_status_get;
    __bcm_plp_cl73_ability_set_f __cl73_ability_set;
    __bcm_plp_cl73_ability_get_f __cl73_ability_get;
    __bcm_plp_cl73_set_f __cl73_set;
    __bcm_plp_cl73_get_f __cl73_get;
    __bcm_plp_display_eye_scan_f __display_eye_scan;
    __bcm_plp_firmware_info_get_f __firmware_info_get;
    __bcm_plp_firmware_set_f __firmware_set;
    __bcm_plp_rxtx_laneswap_set_f __rxtx_laneswap_set;
    __bcm_plp_rxtx_laneswap_get_f __rxtx_laneswap_get;
    __bcm_plp_pll_sequencer_restart_f __pll_sequencer_restart;
    __bcm_plp_fec_enable_set_f __fec_enable_set;
    __bcm_plp_fec_enable_get_f __fec_enable_get;
    __bcm_plp_phy_status_dump_f __phy_status_dump;
    __bcm_plp_phy_diagnostics_get_f __phy_diagnostics_get;
    __bcm_plp_intr_status_get_f __intr_status_get;
    __bcm_plp_intr_enable_set_f __intr_enable_set;
    __bcm_plp_intr_enable_get_f __intr_enable_get;
    __bcm_plp_intr_status_clear_f __intr_status_clear;
    __bcm_plp_fc_pcs_chkr_enable_set_f __fc_pcs_chkr_enable_set;
    __bcm_plp_fc_pcs_chkr_enable_get_f __fc_pcs_chkr_enable_get;
    __bcm_plp_fc_pcs_chkr_status_get_f __fc_pcs_chkr_status_get;
    __bcm_plp_eye_margin_proj_f __eye_margin_proj;
    __bcm_plp_repeater_mode_get_f __repeater_mode_get;
    __bcm_plp_repeater_mode_set_f __repeater_mode_set;
    __bcm_plp_module_read_f __module_read;
    __bcm_plp_module_write_f __module_write;
    __bcm_plp_cfg_gpio_pin_set_f __cfg_gpio_pin_set;
    __bcm_plp_cfg_gpio_pin_get_f __cfg_gpio_pin_get;
    __bcm_plp_power_set_f __power_set;
    __bcm_plp_power_get_f __power_get;
    __bcm_plp_firmware_lane_config_set_f __firmware_lane_config_set;
    __bcm_plp_firmware_lane_config_get_f __firmware_lane_config_get;
    __bcm_plp_init_fw_bcast_f __init_fw_bcast;
    __bcm_plp_failover_mode_set_f __failover_mode_set;
    __bcm_plp_failover_mode_get_f __failover_mode_get;
    __bcm_plp_edc_config_set_f __edc_config_set;
    __bcm_plp_edc_config_get_f __edc_config_get;
    __bcm_plp_fec_corrected_error_counter_f __fec_corrected_counter_get;
    __bcm_plp_fec_uncorrected_error_counter_f __fec_uncorrected_counter_get;
    __bcm_plp_event_status_get __event_status_get;
    __bcm_plp_base_t_autoneg_ability_set_f  __base_t_autoneg_ability_set;
    __bcm_plp_base_t_autoneg_ability_get_f  __base_t_autoneg_ability_get;
    __bcm_plp_base_t_autoneg_set_f  __base_t_autoneg_set;
    __bcm_plp_base_t_autoneg_get_f  __base_t_autoneg_get;
    __bcm_plp_base_t_eee_set_f  __base_t_eee_set;
    __bcm_plp_base_t_eee_get_f  __base_t_eee_get;
    __bcm_plp_base_t_power_mode_set_f  __base_t_power_mode_set;
    __bcm_plp_base_t_power_mode_get_f  __base_t_power_mode_get;
    __bcm_plp_timesync_config_set __timesync_config_set;
    __bcm_plp_timesync_config_get __timesync_config_get;
    __bcm_plp_timesync_enable_set __timesync_enable_set;
    __bcm_plp_timesync_enable_get __timesync_enable_get;
    __bcm_plp_timesync_nco_addend_set __timesync_nco_addend_set;
    __bcm_plp_timesync_nco_addend_get __timesync_nco_addend_get;
    __bcm_plp_timesync_framesync_mode_set  __timesync_framesync_mode_set;
    __bcm_plp_timesync_framesync_mode_get  __timesync_framesync_mode_get;
    __bcm_plp_timesync_local_time_set   __timesync_local_time_set;
    __bcm_plp_timesync_local_time_get   __timesync_local_time_get;
    __bcm_plp_timesync_load_ctrl_set    __timesync_load_ctrl_set;
    __bcm_plp_timesync_load_ctrl_get    __timesync_load_ctrl_get;
    __bcm_plp_timesync_tx_timestamp_offset_set  __timesync_tx_timestamp_offset_set;
    __bcm_plp_timesync_tx_timestamp_offset_get  __timesync_tx_timestamp_offset_get;
    __bcm_plp_timesync_rx_timestamp_offset_set  __timesync_rx_timestamp_offset_set;
    __bcm_plp_timesync_rx_timestamp_offset_get  __timesync_rx_timestamp_offset_get;
    __bcm_plp_timesync_capture_timestamp_get    __timesync_capture_timestamp_get;
    __bcm_plp_timesync_heartbeat_timestamp_get  __timesync_heartbeat_timestamp_get;
    __bcm_plp_timesync_do_sync __timesync_do_sync;
    __bcm_plp_mutex_info_set_f __mutex_info_set;
    __bcm_plp_l1_intr_status_get_f __l1_intr_status_get;
    __bcm_plp_base_t_cable_diag_f  __base_t_cable_diag;
    __bcm_plp_base_t_state_mfg_diag_f  __base_t_state_mfg_diag;
    __bcm_plp_pam4_tx_set_f __pam4_tx_set;
    __bcm_plp_pam4_tx_get_f __pam4_tx_get;
    __bcm_plp_phy_pam4_diagnostics_get __pam4_diag_get;
    __bcm_plp_jfec_config_set     __jfec_config_set;
    __bcm_plp_jfec_config_get     __jfec_config_get;
    __bcm_plp_fec_status_get      __fec_status;
    __bcm_plp_kp4_fec_config_set  __kp4_fec_config_set;
    __bcm_plp_kp4_fec_config_get  __kp4_fec_config_get;
    __bcm_plp_core_diagnostics_get __core_diagnostics_get;
    __bcm_plp_avs_config_set __avs_config_set;
    __bcm_plp_avs_config_get __avs_config_get;
    __bcm_plp_avs_status_get __avs_status_get;
    __bcm_plp_prbs_error_inject_set __prbs_error_inject_set;
    __bcm_plp_logical_lane_set_f    __logical_lane_set ;
    __bcm_plp_logical_lane_get_f    __logical_lane_get ;
    __bcm_plp_prbs_error_analyzer_proj_f __prbs_err_proj;
    __bcm_plp_dsrds_firmware_lane_config_set_f __dsrds_firmware_lane_config_set;
    __bcm_plp_dsrds_firmware_lane_config_get_f __dsrds_firmware_lane_config_get;
    __bcm_plp_synce_config_set __synce_config_set;
    __bcm_plp_synce_config_get __synce_config_get;
    __bcm_autoneg_remote_ability_get_f __remote_ability_get;
    __bcm_plp_timesync_time_code_set  __timesync_time_code_set;
    __bcm_plp_timesync_time_code_get  __timesync_time_code_get;
    __bcm_plp_timesync_link_delay_set  __timesync_link_delay_set;
    __bcm_plp_timesync_link_delay_get  __timesync_link_delay_get;
    __bcm_plp_timesync_timestamp_offset_set  __timesync_timestamp_offset_set;
    __bcm_plp_timesync_timestamp_offset_get  __timesync_timestamp_offset_get;
    __bcm_plp_timesync_timing_control_set  __timesync_timing_control_set;
    __bcm_plp_timesync_timing_control_get  __timesync_timing_control_get;
    __bcm_plp_pcs_status_get_f __pcs_status_get;
    __bcm_plp_timesync_sopmem_get  __timesync_sopmem_get;
    __bcm_plp_pattern_enable_set_f __pattern_enable_set;
    __bcm_plp_pattern_enable_get_f __pattern_enable_get;
    __bcm_plp_fw_init_params_get_f __fw_init_params_get;
    __bcm_plp_srds_diag_access_enable_f __srds_diag_access_enable;
    __bcm_plp_timesync_mpls_set    __timesync_mpls_set;
    __bcm_plp_timesync_mpls_get    __timesync_mpls_get;
    __bcm_plp_timesync_inband_filter_set    __timesync_inband_filter_set;
    __bcm_plp_timesync_inband_filter_get    __timesync_inband_filter_get;
    __bcm_plp_phy_pai_info_get_f __phy_pai_info_get;
    __bcm_plp_failover_config_set_f __failover_config_set;
    __bcm_plp_failover_config_get_f __failover_config_get;
    __bcm_plp_timesync_sopmem_set  __timesync_sopmem_set;
    __bcm_plp_timesync_pch_set  __timesync_pch_set;
    __bcm_plp_timesync_pch_get  __timesync_pch_get;
    __bcm_plp_timestamp_univ_set  __timestamp_univ_set;
    __bcm_plp_timestamp_univ_get  __timestamp_univ_get;
    __bcm_plp_peripheral_device_control_f __peripheral_device_control;
    __bcm_plp_phy_stats_f __phy_stats;
    __bcm_plp_timesync_ptp_action_set  __timesync_ptp_action_set;
    __bcm_plp_timesync_ptp_action_get  __timesync_ptp_action_get;
} __plp__dispatch__t__;

#define bcm_plp_l1_intr_status_get(_pd, _a, _b) \
    PLP_CALL((_pd), __l1_intr_status_get, ((_a), (_b)))
#define bcm_plp_mutex_info_set(_pd, _a, _b) \
    PLP_CALL((_pd), __mutex_info_set, ((_a), (_b)))
#define bcm_plp_static_config_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __static_config_set, ((_a), (_b)))
#define bcm_plp_static_config_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __static_config_get, ((_a), (_b)))
#define bcm_plp_init(_pd, _a, _b, _c, _d ) \
    PLP_CALL((_pd), __init, ((_a), (_b), (_c), (_d)))
#define bcm_plp_cleanup(_pd, _a ) \
    PLP_CALL((_pd), __cleanup, ((_a)))
#define bcm_plp_link_status_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __link_status_get, ((_a), (_b)))
#define bcm_plp_mode_config_set(_pd, _a, _b, _c, _d, _e, _f ) \
    PLP_CALL((_pd), __mode_config_set, ((_a), (_b), (_c), (_d), (_e), (_f)))
#define bcm_plp_mode_config_get(_pd, _a, _b, _c, _d, _e, _f  ) \
    PLP_CALL((_pd), __mode_config_get, ((_a), (_b), (_c), (_d), (_e), (_f)))
#define bcm_plp_driver_version_get(_pd, _a, _b, _c, _d) \
    PLP_CALL((_pd), __driver_version_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_prbs_set(_pd, _a, _b, _c, _d, _e, _f ) \
    PLP_CALL((_pd), __prbs_set, ((_a), (_b), (_c), (_d), (_e), (_f)))
#define bcm_plp_prbs_get(_pd, _a, _b, _c, _d, _e, _f ) \
    PLP_CALL((_pd), __prbs_get, ((_a), (_b), (_c), (_d), (_e), (_f)))
#define bcm_plp_prbs_rx_stat(_pd, _a, _b ) \
    PLP_CALL((_pd), __prbs_rx_stat, ((_a), (_b)))
#define bcm_plp_prbs_clear(_pd, _a, _b ) \
    PLP_CALL((_pd), __prbs_clear, ((_a), (_b)))
#define bcm_plp_prbs_config_get(_pd, _a, _b, _c, _d ) \
    PLP_CALL((_pd), __prbs_config_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_prbs_status_get(_pd, _a, _b, _c, _d ) \
    PLP_CALL((_pd), __prbs_status_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_reg_value_set(_pd, _a, _b, _c, _d ) \
    PLP_CALL((_pd), __reg_value_set, ((_a), (_b), (_c), (_d)))
#define bcm_plp_reg_value_get(_pd, _a, _b, _c, _d ) \
    PLP_CALL((_pd), __reg_value_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_polarity_set(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __polarity_set, ((_a), (_b), (_c)))
#define bcm_plp_polarity_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __polarity_get, ((_a), (_b), (_c)))
#define bcm_plp_rx_pmd_lock_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __rx_pmd_lock_get, ((_a), (_b)))
#define bcm_plp_rev_id(_pd, _a, _b ) \
    PLP_CALL((_pd), __rev_id, ((_a), (_b)))
#define bcm_plp_loopback_set(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __loopback_set, ((_a), (_b), (_c)))
#define bcm_plp_loopback_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __loopback_get, ((_a), (_b), (_c)))
#define bcm_plp_tx_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __tx_set, ((_a), (_b)))
#define bcm_plp_tx_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __tx_get, ((_a), (_b)))
#define bcm_plp_pam4_tx_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __pam4_tx_set, ((_a), (_b)))
#define bcm_plp_pam4_tx_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __pam4_tx_get, ((_a), (_b)))

#define bcm_plp_pam4_diag_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __pam4_diag_get, ((_a), (_b)))
#define bcm_plp_jfec_config_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __jfec_config_set, ((_a), (_b)))
#define bcm_plp_jfec_config_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __jfec_config_get, ((_a), (_b)))
#define bcm_plp_fec_status(_pd, _a, _b ) \
    PLP_CALL((_pd), __fec_status, ((_a), (_b)))
#define bcm_plp_kp4_fec_config_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __kp4_fec_config_get, ((_a), (_b)))
#define bcm_plp_kp4_fec_config_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __kp4_fec_config_set, ((_a), (_b)))

#define bcm_plp_core_diagnostics_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __core_diagnostics_get, ((_a), (_b)))
#define bcm_plp_avs_config_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __avs_config_set, ((_a), (_b)))
#define bcm_plp_avs_config_get(_pd, _a, _b ) \
   PLP_CALL((_pd), __avs_config_get, ((_a), (_b)))
#define bcm_plp_avs_status_get(_pd, _a, _b ) \
   PLP_CALL((_pd), __avs_status_get, ((_a), (_b)))
#define bcm_plp_prbs_error_inject_set(_pd, _a, _b ) \
   PLP_CALL((_pd), __prbs_error_inject_set, ((_a), (_b)))

#define bcm_plp_rx_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __rx_set, ((_a), (_b)))
#define bcm_plp_rx_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __rx_get, ((_a), (_b)))
#define bcm_plp_reset_set(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __reset_set, ((_a), (_b), (_c)))
#define bcm_plp_phy_lane_reset_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __phy_lane_reset_set, ((_a), (_b)))
#define bcm_plp_phy_lane_reset_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __phy_lane_reset_get, ((_a), (_b)))
#define bcm_plp_tx_lane_control_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __tx_lane_control_set, ((_a), (_b)))
#define bcm_plp_rx_lane_control_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __rx_lane_control_set, ((_a), (_b)))
#define bcm_plp_tx_lane_control_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __tx_lane_control_get, ((_a), (_b)))
#define bcm_plp_rx_lane_control_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __rx_lane_control_get, ((_a), (_b)))
#define bcm_plp_lane_cross_switch_map_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __lane_cross_switch_map_set, ((_a), (_b)))
#define bcm_plp_lane_cross_switch_map_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __lane_cross_switch_map_get, ((_a), (_b)))
#define bcm_plp_force_tx_training_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __force_tx_training_set, ((_a), (_b)))
#define bcm_plp_force_tx_training_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __force_tx_training_get, ((_a), (_b)))
#define bcm_plp_force_tx_training_status_get(_pd, _a, _b, _c, _d ) \
    PLP_CALL((_pd), __force_tx_training_status_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_cl73_ability_set(_pd, _a, _b, _c, _d, _e ) \
    PLP_CALL((_pd), __cl73_ability_set, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_cl73_ability_get(_pd, _a, _b, _c, _d, _e ) \
    PLP_CALL((_pd), __cl73_ability_get, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_cl73_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __cl73_set, ((_a), (_b)))
#define bcm_plp_cl73_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __cl73_get, ((_a), (_b), (_c)))
#define bcm_plp_display_eye_scan(_pd, _a ) \
    PLP_CALL((_pd), __display_eye_scan, ((_a)))
#define bcm_plp_firmware_info_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __firmware_info_get, ((_a), (_b), (_c)))
#define bcm_plp_firmware_set(_pd, _a) \
    PLP_CALL((_pd), __firmware_set, ((_a)))
#define bcm_plp_rxtx_laneswap_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __rxtx_laneswap_set, ((_a), (_b)))
#define bcm_plp_rxtx_laneswap_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __rxtx_laneswap_get, ((_a), (_b)))
#define bcm_plp_pll_sequencer_restart(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __pll_sequencer_restart, ((_a), (_b), (_c)))
#define bcm_plp_fec_enable_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __fec_enable_set, ((_a), (_b)))
#define bcm_plp_fec_enable_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __fec_enable_get, ((_a), (_b)))
#define bcm_plp_phy_status_dump(_pd, _a ) \
    PLP_CALL((_pd), __phy_status_dump, ((_a)))
#define bcm_plp_phy_diagnostics_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __phy_diagnostics_get, ((_a), (_b)))
#define bcm_plp_intr_status_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __intr_status_get, ((_a), (_b), (_c)))
#define bcm_plp_intr_enable_set(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __intr_enable_set, ((_a), (_b), (_c)))
#define bcm_plp_intr_enable_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __intr_enable_get, ((_a), (_b), (_c)))
#define bcm_plp_intr_status_clear(_pd, _a, _b ) \
    PLP_CALL((_pd), __intr_status_clear, ((_a), (_b)))
#define bcm_plp_fc_pcs_chkr_enable_set(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __fc_pcs_chkr_enable_set, ((_a), (_b), (_c)))
#define bcm_plp_fc_pcs_chkr_enable_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __fc_pcs_chkr_enable_get, ((_a), (_b), (_c)))
#define bcm_plp_fc_pcs_chkr_status_get(_pd, _a, _b, _c, _d ) \
    PLP_CALL((_pd), __fc_pcs_chkr_status_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_eye_margin_proj(_pd, _a, _b, _c, _d, _e ) \
    PLP_CALL((_pd), __eye_margin_proj, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_repeater_mode_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __repeater_mode_get, ((_a), (_b)))
#define bcm_plp_repeater_mode_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __repeater_mode_set, ((_a), (_b)))
#define bcm_plp_module_read(_pd, _a, _b, _c, _d, _e ) \
    PLP_CALL((_pd), __module_read, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_module_write(_pd, _a, _b, _c, _d, _e ) \
    PLP_CALL((_pd), __module_write, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_peripheral_device_control(_pd, _a, _b, _c, _d ) \
    PLP_CALL((_pd), __peripheral_device_control, ((_a), (_b), (_c), (_d)))
#define bcm_plp_cfg_gpio_pin_set(_pd, _a, _b, _c, _d, _e ) \
    PLP_CALL((_pd), __cfg_gpio_pin_set, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_cfg_gpio_pin_get(_pd, _a, _b, _c, _d, _e ) \
    PLP_CALL((_pd), __cfg_gpio_pin_get, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_power_set(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __power_set, ((_a), (_b), (_c)))
#define bcm_plp_power_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __power_get, ((_a), (_b), (_c)))
#define bcm_plp_firmware_lane_config_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __firmware_lane_config_set, ((_a), (_b)))
#define bcm_plp_firmware_lane_config_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __firmware_lane_config_get, ((_a), (_b)))
#define bcm_plp_init_fw_bcast(_pd, _a, _b, _c, _d, _e ) \
    PLP_CALL((_pd), __init_fw_bcast, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_failover_mode_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __failover_mode_set, ((_a), (_b)))
#define bcm_plp_failover_mode_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __failover_mode_get, ((_a), (_b)))
#define bcm_plp_edc_config_set(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __edc_config_set, ((_a), (_b), (_c)))
#define bcm_plp_edc_config_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __edc_config_get, ((_a), (_b), (_c)))
#define bcm_plp_fec_corrected_error_counter(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __fec_corrected_counter_get, ((_a), (_b),(_c)))
#define bcm_plp_fec_uncorrected_error_counter(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __fec_uncorrected_counter_get, ((_a), (_b), (_c)))
#define bcm_plp_event_status_get(_pd, _a, _b , _c) \
    PLP_CALL((_pd), __event_status_get, ((_a), (_b), (_c)))

#ifdef BCM_PLP_BASE_T_PHY
#define bcm_plp_base_t_autoneg_ability_set(_pd, _a, _b) \
    PLP_CALL((_pd), __base_t_autoneg_ability_set, ((_a), (_b)))
#define bcm_plp_base_t_autoneg_ability_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __base_t_autoneg_ability_get, ((_a), (_b)))
#define bcm_plp_base_t_autoneg_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __base_t_autoneg_set, ((_a), (_b)))
#define bcm_plp_base_t_autoneg_get(_pd, _a, _b, _c ) \
    PLP_CALL((_pd), __base_t_autoneg_get, ((_a), (_b), (_c)))
#define bcm_plp_base_t_eee_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __base_t_eee_set, ((_a), (_b)))
#define bcm_plp_base_t_eee_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __base_t_eee_get, ((_a), (_b)))
#define bcm_plp_base_t_power_mode_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __base_t_power_mode_set, ((_a), (_b)))
#define bcm_plp_base_t_power_mode_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __base_t_power_mode_get, ((_a), (_b)))
#define bcm_plp_base_t_cable_diag(_pd, _a, _b) \
     PLP_CALL((_pd), __base_t_cable_diag, ((_a), (_b)))
#define bcm_plp_base_t_state_mfg_diag(_pd, _a, _b, _c, _d, _e) \
     PLP_CALL((_pd), __base_t_state_mfg_diag, ((_a), (_b), (_c), (_d), (_e)))
#endif /* BCM_PLP_BASE_T_PHY */

#define bcm_plp_timesync_config_set(_pd, _a, _b) \
    PLP_CALL((_pd), __timesync_config_set, ((_a), (_b)))
#define bcm_plp_timesync_config_get(_pd, _a, _b) \
    PLP_CALL((_pd), __timesync_config_get, ((_a), (_b)))
#define bcm_plp_timesync_enable_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_enable_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_enable_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_enable_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_pch_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_pch_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_pch_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_pch_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_mpls_set(_pd,  _a, _b, _c, _d) \
    PLP_CALL((_pd), __timesync_mpls_set, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_mpls_get(_pd,  _a, _b, _c, _d) \
    PLP_CALL((_pd), __timesync_mpls_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_inband_filter_set(_pd,  _a, _b, _c, _d) \
    PLP_CALL((_pd), __timesync_inband_filter_set, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_inband_filter_get(_pd,  _a, _b, _c, _d) \
    PLP_CALL((_pd), __timesync_inband_filter_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_nco_addend_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_nco_addend_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_nco_addend_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_nco_addend_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_framesync_mode_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_framesync_mode_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_framesync_mode_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_framesync_mode_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_local_time_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_local_time_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_local_time_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_local_time_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_load_ctrl_set(_pd,  _a, _b, _c, _d) \
    PLP_CALL((_pd), __timesync_load_ctrl_set, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_load_ctrl_get(_pd,  _a, _b, _c, _d) \
    PLP_CALL((_pd), __timesync_load_ctrl_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_tx_timestamp_offset_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_tx_timestamp_offset_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_tx_timestamp_offset_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_tx_timestamp_offset_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_rx_timestamp_offset_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_rx_timestamp_offset_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_rx_timestamp_offset_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_rx_timestamp_offset_get, ((_a), (_b), (_c)))
#define bcm_plp_timestamp_univ_set(_pd, _a, _b, _c, _d) \
    PLP_CALL((_pd), __timestamp_univ_set, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timestamp_univ_get(_pd, _a, _b, _c, _d) \
    PLP_CALL((_pd), __timestamp_univ_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_sopmem_set(_pd, _a, _b, _c, _d) \
    PLP_CALL((_pd), __timesync_sopmem_set, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_sopmem_get(_pd, _a, _b, _c, _d) \
    PLP_CALL((_pd), __timesync_sopmem_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_ptp_action_set(_pd, _a, _b, _c, _d, _e) \
    PLP_CALL((_pd), __timesync_ptp_action_set, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_timesync_ptp_action_get(_pd, _a, _b, _c, _d, _e) \
    PLP_CALL((_pd), __timesync_ptp_action_get, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_timesync_capture_timestamp_get(_pd,   _a, _b, _c) \
    PLP_CALL((_pd), __timesync_capture_timestamp_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_heartbeat_timestamp_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_heartbeat_timestamp_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_do_sync(_pd, _a) \
    PLP_CALL((_pd), __timesync_do_sync, (_a))
#define bcm_plp_logical_lane_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __logical_lane_set, ((_a), (_b)))
#define bcm_plp_logical_lane_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __logical_lane_get, ((_a), (_b)))
#define bcm_plp_prbs_error_analyzer_proj(_pd, _a, _b, _c, _d) \
    PLP_CALL((_pd), __prbs_err_proj, ((_a), (_b), (_c), (_d)))
#define bcm_plp_dsrds_firmware_lane_config_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __dsrds_firmware_lane_config_set, ((_a), (_b)))
#define bcm_plp_dsrds_firmware_lane_config_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __dsrds_firmware_lane_config_get, ((_a), (_b)))
#define bcm_plp_synce_config_set(_pd, _a, _b) \
    PLP_CALL((_pd), __synce_config_set, ((_a), (_b)))
#define bcm_plp_synce_config_get(_pd, _a, _b) \
    PLP_CALL((_pd), __synce_config_get, ((_a), (_b)))
#define bcm_plp_autoneg_remote_ability_get(_pd, _a, _b, _c, _d ) \
    PLP_CALL((_pd), __remote_ability_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_timesync_timing_control_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_timing_control_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_timing_control_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_timing_control_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_link_delay_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_link_delay_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_link_delay_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_link_delay_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_timestamp_offset_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_timestamp_offset_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_timestamp_offset_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_timestamp_offset_get, ((_a), (_b), (_c)))
#define bcm_plp_timesync_time_code_set(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_time_code_set, ((_a), (_b), (_c)))
#define bcm_plp_timesync_time_code_get(_pd, _a, _b, _c) \
    PLP_CALL((_pd), __timesync_time_code_get, ((_a), (_b), (_c)))
#define bcm_plp_pcs_status_get(_pd, _a, _b) \
    PLP_CALL((_pd), __pcs_status_get, ((_a), (_b)))
#define bcm_plp_pattern_enable_set(_pd, _a, _b) \
    PLP_CALL((_pd), __pattern_enable_set, ((_a), (_b)))
#define bcm_plp_pattern_enable_get(_pd, _a, _b) \
    PLP_CALL((_pd), __pattern_enable_get, ((_a), (_b)))
#define bcm_plp_fw_init_params_get(_pd, _a, _b) \
    PLP_CALL((_pd), __fw_init_params_get, ((_a), (_b)))
#define bcm_plp_srds_diag_access_enable(_pd, _a, _b) \
    PLP_CALL((_pd), __srds_diag_access_enable, ((_a), (_b)))
#define bcm_plp_phy_pai_info_get(_pd, _a, _b) \
    PLP_CALL((_pd), __phy_pai_info_get, ((_a), (_b)))
#define bcm_plp_failover_config_set(_pd, _a, _b ) \
    PLP_CALL((_pd), __failover_config_set, ((_a), (_b)))
#define bcm_plp_failover_config_get(_pd, _a, _b ) \
    PLP_CALL((_pd), __failover_config_get, ((_a), (_b)))
#define bcm_plp_phy_stats(_pd, _a, _b, _c, _d, _e) \
    PLP_CALL((_pd), __phy_stats, ((_a), (_b), (_c), (_d), (_e)))

extern __plp__dispatch__t__ plp_evora_dispatch;
extern __plp__dispatch__t__ plp_aperta_dispatch;
extern __plp__dispatch__t__ plp_europa_dispatch;
extern __plp__dispatch__t__ plp_quadra28_dispatch;
extern __plp__dispatch__t__ plp_gallardo28_dispatch;
extern __plp__dispatch__t__ plp_dino_dispatch;
extern __plp__dispatch__t__ plp_koi_dispatch;
extern __plp__dispatch__t__ plp_orca_dispatch;
extern __plp__dispatch__t__ plp_shortfin_dispatch;
extern __plp__dispatch__t__ plp_estoque_dispatch;
extern __plp__dispatch__t__ plp_estoquej_dispatch;
extern __plp__dispatch__t__ plp_millenio_dispatch;
extern __plp__dispatch__t__ plp_miura_dispatch;
extern __plp__dispatch__t__ plp_seahawks_dispatch;
extern __plp__dispatch__t__ plp_barchetta_dispatch;
extern __plp__dispatch__t__ plp_broncos_dispatch;
extern __plp__dispatch__t__ plp_ravens_dispatch;
extern __plp__dispatch__t__ plp_giants_dispatch;
extern __plp__dispatch__t__ plp_blackfin_dispatch;
extern __plp__dispatch__t__ plp_longfin_dispatch;
extern __plp__dispatch__t__ plp_broadfin_dispatch;
extern __plp__dispatch__t__ plp_whitetip_dispatch;
extern __plp__dispatch__t__ plp_milleniob_dispatch;
extern __plp__dispatch__t__ plp_rams_dispatch;
extern __plp__dispatch__t__ plp_cowboys_dispatch;
extern __plp__dispatch__t__ plp_barchetta2_dispatch;
extern __plp__dispatch__t__ plp_agera_dispatch;
extern __plp__dispatch__t__ plp_ageralite_dispatch;
extern __plp__dispatch__t__ plp_aperta2_dispatch;
extern __plp__dispatch__t__ plp_agera2_dispatch;
extern __plp__dispatch__t__ plp_kauai_dispatch;
extern __plp__dispatch__t__ plp_lanai_dispatch;

#define _PLP_FALSE (0)

#define PLP_CHECK_PF(__pf, __pa) ((__pf == __PLP_NOT_USED) ? _PLP_API_UNAVAILABLE : (__pf __pa))

#ifdef PLP_EVORA_SUPPORT
    #define PLP_EVORA_COMPARE(_N, _pf, _pa) ((strcmp(_N, "evora")) == 0) ? PLP_CHECK_PF(plp_evora_dispatch._pf, _pa)
#else
    #define PLP_EVORA_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_EUROPA_SUPPORT
    #define PLP_EUROPA_COMPARE(_N, _pf, _pa) ((strcmp(_N, "europa")) == 0) ? PLP_CHECK_PF(plp_europa_dispatch._pf, _pa)
#else
    #define PLP_EUROPA_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_MIURA_SUPPORT
    #define PLP_MIURA_COMPARE(_N, _pf, _pa) ((strcmp(_N, "miura")) == 0) ? PLP_CHECK_PF(plp_miura_dispatch._pf, _pa)
#else
    #define PLP_MIURA_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_DINO_SUPPORT
    #define PLP_DINO_COMPARE(_N, _pf, _pa) ((strcmp(_N, "dino")) == 0) ? PLP_CHECK_PF(plp_dino_dispatch._pf, _pa)
#else
    #define PLP_DINO_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_QUADRA28_SUPPORT
    #define PLP_QUADRA28_COMPARE(_N, _pf, _pa) ((strcmp(_N, "quadra28")) == 0) ? PLP_CHECK_PF(plp_quadra28_dispatch._pf, _pa)
#else
    #define PLP_QUADRA28_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_GALLARDO28_SUPPORT
    #define PLP_GALLARDO28_COMPARE(_N, _pf, _pa) ((strcmp(_N, "gallardo28")) == 0) ? PLP_CHECK_PF(plp_gallardo28_dispatch._pf, _pa)
#else
    #define PLP_GALLARDO28_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_KOI_SUPPORT
    #define PLP_KOI_COMPARE(_N, _pf, _pa) ((strcmp(_N, "koi")) == 0) ? PLP_CHECK_PF(plp_koi_dispatch._pf, _pa)
#else
    #define PLP_KOI_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_ORCA_SUPPORT
    #define PLP_ORCA_COMPARE(_N, _pf, _pa) ((strcmp(_N, "orca")) == 0) ? PLP_CHECK_PF(plp_orca_dispatch._pf, _pa)
#else
    #define PLP_ORCA_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_SHORTFIN_SUPPORT
    #define PLP_SHORTFIN_COMPARE(_N, _pf, _pa) ((strcmp(_N, "shortfin")) == 0) ? PLP_CHECK_PF(plp_shortfin_dispatch._pf, _pa)
#else
    #define PLP_SHORTFIN_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_ESTOQUE_SUPPORT
    #define PLP_ESTOQUE_COMPARE(_N, _pf, _pa) ((strcmp(_N, "estoque")) == 0) ? PLP_CHECK_PF(plp_estoque_dispatch._pf, _pa)
#else
    #define PLP_ESTOQUE_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_ESTOQUEJ_SUPPORT
    #define PLP_ESTOQUEJ_COMPARE(_N, _pf, _pa) ((strcmp(_N, "estoquej")) == 0) ? PLP_CHECK_PF(plp_estoquej_dispatch._pf, _pa)
#else
    #define PLP_ESTOQUEJ_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_MILLENIO_SUPPORT
    #define PLP_MILLENIO_COMPARE(_N, _pf, _pa) ((strcmp(_N, "millenio")) == 0) ? PLP_CHECK_PF(plp_millenio_dispatch._pf, _pa)
#else
    #define PLP_MILLENIO_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_SEAHAWKS_SUPPORT
    #define PLP_SEAHAWKS_COMPARE(_N, _pf, _pa) ((strcmp(_N, "seahawks")) == 0) ? PLP_CHECK_PF(plp_seahawks_dispatch._pf, _pa)
#else
    #define PLP_SEAHAWKS_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_BARCHETTA_SUPPORT
    #define PLP_BARCHETTA_COMPARE(_N, _pf, _pa) ((strcmp(_N, "barchetta")) == 0) ? PLP_CHECK_PF(plp_barchetta_dispatch._pf, _pa)
#else
    #define PLP_BARCHETTA_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_BARCHETTA2_SUPPORT
    #define PLP_BARCHETTA2_COMPARE(_N, _pf, _pa) ((strcmp(_N, "barchetta2")) == 0) ? PLP_CHECK_PF(plp_barchetta2_dispatch._pf, _pa)
#else
    #define PLP_BARCHETTA2_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_BRONCOS_SUPPORT
    #define PLP_BRONCOS_COMPARE(_N, _pf, _pa) ((strcmp(_N, "broncos")) == 0) ? PLP_CHECK_PF(plp_broncos_dispatch._pf, _pa)
#else
    #define PLP_BRONCOS_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_RAVENS_SUPPORT
    #define PLP_RAVENS_COMPARE(_N, _pf, _pa) ((strcmp(_N, "ravens")) == 0) ? PLP_CHECK_PF(plp_ravens_dispatch._pf, _pa)
#else
    #define PLP_RAVENS_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_GIANTS_SUPPORT
    #define PLP_GIANTS_COMPARE(_N, _pf, _pa) ((strcmp(_N, "giants")) == 0) ? PLP_CHECK_PF(plp_giants_dispatch._pf, _pa)
#else
    #define PLP_GIANTS_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_BLACKFIN_SUPPORT
    #define PLP_BLACKFIN_COMPARE(_N, _pf, _pa) ((strcmp(_N, "blackfin")) == 0) ? PLP_CHECK_PF(plp_blackfin_dispatch._pf, _pa)
#else
    #define PLP_BLACKFIN_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_APERTA_SUPPORT
    #define PLP_APERTA_COMPARE(_N, _pf, _pa) ((strcmp(_N, "aperta")) == 0) ? PLP_CHECK_PF(plp_aperta_dispatch._pf, _pa)
#else
    #define PLP_APERTA_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_LONGFIN_SUPPORT
    #define PLP_LONGFIN_COMPARE(_N, _pf, _pa) ((strcmp(_N, "longfin")) == 0) ? PLP_CHECK_PF(plp_longfin_dispatch._pf, _pa)
#else
    #define PLP_LONGFIN_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_BROADFIN_SUPPORT
    #define PLP_BROADFIN_COMPARE(_N, _pf, _pa) ((strcmp(_N, "broadfin")) == 0) ? PLP_CHECK_PF(plp_broadfin_dispatch._pf, _pa)
#else
    #define PLP_BROADFIN_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_WHITETIP_SUPPORT
    #define PLP_WHITETIP_COMPARE(_N, _pf, _pa) ((strcmp(_N, "whitetip")) == 0) ? PLP_CHECK_PF(plp_whitetip_dispatch._pf, _pa)
#else
    #define PLP_WHITETIP_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_KAUAI_SUPPORT
    #define PLP_KAUAI_COMPARE(_N, _pf, _pa) ((strcmp(_N, "kauai")) == 0) ? PLP_CHECK_PF(plp_kauai_dispatch._pf, _pa)
#else
    #define PLP_KAUAI_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_LANAI_SUPPORT
    #define PLP_LANAI_COMPARE(_N, _pf, _pa) ((strcmp(_N, "lanai")) == 0) ? PLP_CHECK_PF(plp_lanai_dispatch._pf, _pa)
#else
    #define PLP_LANAI_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_MILLENIOB_SUPPORT
    #define PLP_MILLENIOB_COMPARE(_N, _pf, _pa) ((strcmp(_N, "milleniob")) == 0) ? PLP_CHECK_PF(plp_milleniob_dispatch._pf, _pa)
#else
    #define PLP_MILLENIOB_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_RAMS_SUPPORT
    #define PLP_RAMS_COMPARE(_N, _pf, _pa) ((strcmp(_N, "rams")) == 0) ? PLP_CHECK_PF(plp_rams_dispatch._pf, _pa)
#else
    #define PLP_RAMS_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_COWBOYS_SUPPORT
    #define PLP_COWBOYS_COMPARE(_N, _pf, _pa) ((strcmp(_N, "cowboys")) == 0) ? PLP_CHECK_PF(plp_cowboys_dispatch._pf, _pa)
#else
    #define PLP_COWBOYS_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#ifdef PLP_AGERA_SUPPORT
    #define PLP_AGERA_COMPARE(_N, _pf, _pa) ((strcmp(_N, "agera")) == 0) ? PLP_CHECK_PF(plp_agera_dispatch._pf, _pa)
#else
    #define PLP_AGERA_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif
#ifdef PLP_AGERALITE_SUPPORT
    #define PLP_AGERALITE_COMPARE(_N, _pf, _pa) ((strcmp(_N, "ageralite")) == 0) ? PLP_CHECK_PF(plp_ageralite_dispatch._pf, _pa)
#else
    #define PLP_AGERALITE_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif
#ifdef PLP_APERTA2_SUPPORT
    #define PLP_APERTA2_COMPARE(_N, _pf, _pa) ((strcmp(_N, "aperta2")) == 0) ? PLP_CHECK_PF(plp_aperta2_dispatch._pf, _pa)
#else
    #define PLP_APERTA2_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif
#ifdef PLP_AGERA2_SUPPORT
    #define PLP_AGERA2_COMPARE(_N, _pf, _pa) ((strcmp(_N, "agera2")) == 0) ? PLP_CHECK_PF(plp_agera2_dispatch._pf, _pa)
#else
    #define PLP_AGERA2_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#define PLP_CALL(_N, _pf, _pa) (PLP_DINO_COMPARE(_N, _pf, _pa)  :\
                               (PLP_EVORA_COMPARE(_N, _pf, _pa) :\
                               (PLP_EUROPA_COMPARE(_N, _pf, _pa) :\
                               (PLP_QUADRA28_COMPARE(_N, _pf, _pa):\
                               (PLP_GALLARDO28_COMPARE(_N, _pf, _pa):\
                               (PLP_KOI_COMPARE(_N, _pf, _pa):\
                               (PLP_ORCA_COMPARE(_N, _pf, _pa):\
                               (PLP_SHORTFIN_COMPARE(_N, _pf, _pa):\
                               (PLP_ESTOQUE_COMPARE(_N, _pf, _pa):\
                               (PLP_ESTOQUEJ_COMPARE(_N, _pf, _pa):\
                               (PLP_MILLENIO_COMPARE(_N, _pf, _pa):\
                               (PLP_MIURA_COMPARE(_N, _pf, _pa) :\
                               (PLP_SEAHAWKS_COMPARE(_N, _pf, _pa):\
                               (PLP_BARCHETTA_COMPARE(_N, _pf, _pa):\
                               (PLP_BRONCOS_COMPARE(_N, _pf, _pa):\
                               (PLP_RAVENS_COMPARE(_N, _pf, _pa):\
                               (PLP_GIANTS_COMPARE(_N, _pf, _pa):\
                               (PLP_BLACKFIN_COMPARE(_N, _pf, _pa):\
                               (PLP_APERTA_COMPARE(_N, _pf, _pa):\
                               (PLP_LONGFIN_COMPARE(_N, _pf, _pa):\
                               (PLP_BROADFIN_COMPARE(_N, _pf, _pa):\
                               (PLP_WHITETIP_COMPARE(_N, _pf, _pa):\
                               (PLP_KAUAI_COMPARE(_N, _pf, _pa):\
                               (PLP_LANAI_COMPARE(_N, _pf, _pa):\
                               (PLP_MILLENIOB_COMPARE(_N, _pf, _pa):\
                               (PLP_RAMS_COMPARE(_N, _pf, _pa):\
                               (PLP_COWBOYS_COMPARE(_N, _pf, _pa):\
                               (PLP_BARCHETTA2_COMPARE(_N, _pf, _pa):\
                               (PLP_AGERA_COMPARE(_N, _pf, _pa):\
                               (PLP_AGERALITE_COMPARE(_N, _pf, _pa):\
                               (PLP_APERTA2_COMPARE(_N, _pf, _pa):\
                               (PLP_AGERA2_COMPARE(_N, _pf, _pa):\
                                _PLP_API_UNAVAILABLE))))))))))))))))))))))))))))))))

#endif /* EPDM_H */
