/*
 *         
 * $Id: bcm_pm_if_api.h $
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

#ifndef BCM_PM_IF_API_H
#define BCM_PM_IF_API_H

#ifdef USE_EPDM_COMMON_DEFINES
#include "epdm_bcm_common_defines.h"
#else
#include "bcm_common_defines.h"
#endif

#define BCM_LEGACY_PHY_SUPPORT 
#ifdef BCM_LEGACY_PHY_SUPPORT
/* Macro for BCM_PM API's to support existing users*/
#define bcm_pm_if_static_config_set(P_C, P_ID, bcm_static_config)                                                  bcm_plp_aperta_static_config_set((bcm_plp_access_t){P_C, P_ID, 0xFF, 0xFF}, bcm_static_config)

#define bcm_pm_if_static_config_get(P_C, P_ID, bcm_static_config)                                                  bcm_plp_aperta_static_config_get((bcm_plp_access_t){P_C, P_ID, 0xFF, 0xFF}, bcm_static_config)

#define bcm_pm_if_init(P_C, P_ID, READ, WRITE, FW_L_M)                                                             bcm_plp_aperta_init((bcm_plp_access_t){P_C, P_ID, 0xFF, 0xFF}, READ, WRITE, FW_L_M) 

#define bcm_pm_if_cleanup(P_ID)                                                                                    bcm_plp_aperta_cleanup((bcm_plp_access_t){0, P_ID, 0xFF, 0xFF})

#define bcm_pm_if_link_status_get(P_C, P_ID, I_S, L_M, link_status)                                                bcm_plp_aperta_link_status_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, link_status)

#define bcm_pm_if_mode_config_set(P_C, P_ID, I_S, L_M, speed, if_type, ref_clk, interface_mode, device_aux_modes)  bcm_plp_aperta_mode_config_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, speed, if_type, ref_clk, interface_mode, device_aux_modes)

#define bcm_pm_if_mode_config_get(P_C, P_ID, I_S, L_M, speed, if_type, ref_clk, interface_mode, device_aux_modes)  bcm_plp_aperta_mode_config_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, speed, if_type, ref_clk, interface_mode, device_aux_modes)
#define bcm_pm_if_version_get(chip_ver, api_ver, enahan_ver)                                                       bcm_plp_aperta_version_get(chip_ver, api_ver, enahan_ver)
#define bcm_pm_if_prbs_set(P_C, P_ID, I_S, L_M, tx_rx,  poly,  invert,  loopback,  ena_dis)                        bcm_plp_aperta_prbs_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, tx_rx,  poly,  invert,  loopback,  ena_dis)
#define bcm_pm_if_prbs_get(P_C, P_ID, I_S, L_M, tx_rx, poly, invert, loopback, ena_dis)                            bcm_plp_aperta_prbs_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, tx_rx, poly, invert, loopback, ena_dis)
#define bcm_pm_if_prbs_rx_stat(P_C, P_ID, I_S, L_M,  time)                                                         bcm_plp_aperta_prbs_rx_stat((bcm_plp_access_t){P_C, P_ID, I_S, L_M},  time)
#define bcm_pm_if_prbs_clear(P_C, P_ID, I_S, L_M,  tx_rx)                                                          bcm_plp_aperta_prbs_clear((bcm_plp_access_t){P_C, P_ID, I_S, L_M},  tx_rx)
#define bcm_pm_if_prbs_config_get(P_C, P_ID, I_S, L_M,  tx_rx, poly, invert)                                       bcm_plp_aperta_prbs_config_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, tx_rx, poly, invert)
#define bcm_pm_if_prbs_status_get(P_C, P_ID, I_S, L_M, prbs_lock, prbs_lock_loss, error_count)                     bcm_plp_aperta_prbs_status_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, prbs_lock, prbs_lock_loss, error_count)  
#define bcm_pm_if_reg_value_set(P_C, P_ID, devaddr, regaddr,  data)                                                bcm_plp_aperta_reg_value_set((bcm_plp_access_t){P_C, P_ID,0xFF,0xFF}, devaddr, regaddr,  data)
#define bcm_pm_if_reg_value_get(P_C, P_ID, devaddr, regaddr, data)                                                 bcm_plp_aperta_reg_value_get((bcm_plp_access_t){P_C, P_ID, 0xFF, 0xFF}, devaddr, regaddr, data)
#define bcm_pm_if_polarity_set(P_C, P_ID, I_S, L_M, tx_pol,  rx_pol)                                               bcm_plp_aperta_polarity_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, tx_pol,  rx_pol)
#define bcm_pm_if_polarity_get(P_C, P_ID, I_S, L_M, tx_pol, rx_pol)                                                bcm_plp_aperta_polarity_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, tx_pol, rx_pol)
#define bcm_pm_if_power_set(P_C, P_ID, I_S, L_M, power_rx, power_tx)                                               bcm_plp_aperta_power_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, power_rx, power_tx)
#define bcm_pm_if_power_get(P_C, P_ID, I_S, L_M, power_rx, power_tx)                                               bcm_plp_aperta_power_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, power_rx, power_tx)
#define bcm_pm_if_rx_pmd_locked_get(P_C, P_ID, I_S, L_M, rx_pmd_lock)                                              bcm_plp_aperta_rx_pmd_lock_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, rx_pmd_lock)
#define bcm_pm_if_rev_id(P_C, P_ID, rev_id)                                                                       bcm_plp_aperta_rev_id((bcm_plp_access_t){P_C, P_ID, 0xFF, 0xFF}, rev_id)
#define bcm_pm_if_loopback_set(P_C, P_ID, I_S, L_M, lb_mode, enable)                                               bcm_plp_aperta_loopback_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, lb_mode, enable) 
#define bcm_pm_if_loopback_get(P_C, P_ID, I_S, L_M, lb_mode, enable)                                               bcm_plp_aperta_loopback_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, lb_mode, enable)
#define bcm_pm_if_tx_set(P_C, P_ID, I_S, L_M, tx)                                                                  bcm_plp_aperta_tx_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, tx) 
#define bcm_pm_if_tx_get(P_C, P_ID, I_S, L_M, tx)                                                                  bcm_plp_aperta_tx_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, tx)
#define bcm_pm_if_rx_set(P_C, P_ID, I_S, L_M, rx)                                                                  bcm_plp_aperta_rx_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, rx)
#define bcm_pm_if_rx_get(P_C, P_ID, I_S, L_M, rx)                                                                  bcm_plp_aperta_rx_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, rx)
#define bcm_pm_if_reset_set(P_C, P_ID, reset_mode, reset_val)                                                      bcm_plp_aperta_reset_set((bcm_plp_access_t){P_C, P_ID,0xFF,0xFF}, reset_mode, reset_val)
#define bcm_pm_if_phy_lane_reset_set(P_C, P_ID, I_S, L_M, reset)                                                   bcm_plp_aperta_phy_lane_reset_set ((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, reset)
#define bcm_pm_if_phy_lane_reset_get(P_C, P_ID, I_S, L_M, reset)                                                   bcm_plp_aperta_phy_lane_reset_get ((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, reset)
#define bcm_pm_if_tx_lane_control_set(P_C, P_ID, I_S, L_M, tx_control)                                             bcm_plp_aperta_tx_lane_control_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, tx_control) 
#define bcm_pm_if_rx_lane_control_set(P_C, P_ID, I_S, L_M, rx_control)                                             bcm_plp_aperta_rx_lane_control_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, rx_control) 
#define bcm_pm_if_tx_lane_control_get(P_C, P_ID, I_S, L_M, tx_control)                                             bcm_plp_aperta_tx_lane_control_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, tx_control) 
#define bcm_pm_if_rx_lane_control_get(P_C, P_ID, I_S, L_M, rx_control)                                             bcm_plp_aperta_rx_lane_control_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, rx_control) 
#define bcm_pm_if_lane_cross_switch_map_set(P_C, P_ID, I_S, tx_source_array)                                       bcm_plp_aperta_lane_cross_switch_map_set((bcm_plp_access_t){P_C, P_ID, I_S, 0xFF}, tx_source_array) 
#define bcm_pm_if_lane_cross_switch_map_get(P_C, P_ID, L_M,  mapped_to)                                            bcm_plp_aperta_lane_cross_switch_map_get((bcm_plp_access_t){P_C, P_ID, 0xFF, L_M},  mapped_to)
#define bcm_pm_if_force_tx_training_set(P_C, P_ID, I_S, L_M, enable)                                               bcm_plp_aperta_force_tx_training_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, enable)
#define bcm_pm_if_force_tx_training_get(P_C, P_ID, I_S, L_M, enable)                                               bcm_plp_aperta_force_tx_training_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, enable)
#define bcm_pm_if_force_tx_training_status_get(P_C, P_ID, I_S, L_M, enabled, training_failure, trained)            bcm_plp_aperta_force_tx_training_status_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, enabled, training_failure, trained) 
#define bcm_pm_if_cl73_ability_set(P_C, P_ID, L_M, tech_ability, fec_ability, pause_ability, an_config)                       bcm_plp_aperta_cl73_ability_set((bcm_plp_access_t){P_C, P_ID, 0xFF, L_M}, tech_ability, fec_ability, pause_ability, an_config)
#define bcm_pm_if_cl73_ability_get(P_C, P_ID, L_M, tech_ability, fec_ability, pause_ability, an_config)                       bcm_plp_aperta_cl73_ability_get((bcm_plp_access_t){P_C, P_ID, 0xFF, L_M}, tech_ability, fec_ability, pause_ability, an_config)
#define bcm_pm_if_cl73_set(P_C, P_ID, L_M, ena_dis)                                                                bcm_plp_aperta_cl73_set((bcm_plp_access_t){P_C, P_ID, 0xFF, L_M}, ena_dis)
#define bcm_pm_if_cl73_get(P_C, P_ID, L_M, an, an_done)                                                            bcm_plp_aperta_cl73_get((bcm_plp_access_t){P_C, P_ID, 0xFF, L_M}, an, an_done) 
#define bcm_pm_if_display_eye_scan(P_C, P_ID, I_S, L_M)                                                            bcm_plp_aperta_display_eye_scan((bcm_plp_access_t){P_C, P_ID, I_S, L_M})
#define bcm_pm_if_firmware_info_get(P_C, P_ID, fw_version, fw_crc)                                                  bcm_plp_aperta_firmware_info_get((bcm_plp_access_t){P_C, P_ID, 0xFF, 0xFF}, fw_version, fw_crc)
#define bcm_pm_if_pll_sequencer_restart(P_C, P_ID, I_S, flags, operation)                                          bcm_plp_aperta_pll_sequencer_restart((bcm_plp_access_t){P_C, P_ID, I_S, 0xFF}, flags, operation)
#define bcm_pm_if_fec_enable_set(P_C, P_ID, enable)                                                                bcm_plp_aperta_fec_enable_set((bcm_plp_access_t){P_C, P_ID, 0xFF, 0xFF}, enable) 
#define bcm_pm_if_fec_enable_get(P_C, P_ID, enable)                                                                bcm_plp_aperta_fec_enable_get((bcm_plp_access_t){P_C, P_ID, 0xFF, 0xFF}, enable)
#define bcm_pm_if_phy_status_dump(P_C, P_ID, I_S, L_M)                                                             bcm_plp_aperta_phy_status_dump((bcm_plp_access_t){P_C, P_ID, I_S, L_M})
#define bcm_pm_if_phy_diagnostics_get(P_C, P_ID, I_S, L_M, diag)                                                   bcm_plp_aperta_phy_diagnostics_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, diag)
#define bcm_pm_if_intr_status_get(P_C, P_ID, intr_type, intr_status)                                               bcm_plp_aperta_intr_status_get((bcm_plp_access_t){P_C, P_ID,0xFF,0xFF}, intr_type, intr_status)
#define bcm_pm_if_intr_enable_set(P_C, P_ID, intr_type, enable)                                                    bcm_plp_aperta_intr_enable_set((bcm_plp_access_t){P_C, P_ID,0xFF,0xFF}, intr_type, enable)
#define bcm_pm_if_intr_enable_get(P_C, P_ID, intr_type, enable)                                                    bcm_plp_aperta_intr_enable_get((bcm_plp_access_t){P_C, P_ID,0xFF,0xFF}, intr_type, enable)
#define bcm_pm_if_intr_status_clear(P_C, P_ID,  intr_type)                                                         bcm_plp_aperta_intr_status_clear((bcm_plp_access_t){P_C, P_ID, 0xFF, 0xFF},  intr_type) 
#define bcm_pm_if_fc_pcs_chkr_enable_set(P_C, P_ID, I_S, L_M, fcpcs_chkr_mode, enable)                                bcm_plp_aperta_fc_pcs_chkr_enable_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, fcpcs_chkr_mode, enable) 
#define bcm_pm_if_fc_pcs_chkr_enable_get(P_C, P_ID, I_S, L_M, fcpcs_chkr_mode, enable)                                bcm_plp_aperta_fc_pcs_chkr_enable_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, fcpcs_chkr_mode, enable)
#define bcm_pm_if_fc_pcs_chkr_status_get(P_C, P_ID, I_S, L_M, lock_status, lock_lost_lh, error_count)                 bcm_plp_aperta_fc_pcs_chkr_status_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, lock_status, lock_lost_lh, error_count)
#ifdef SERDES_API_FLOATING_POINT 
#define bcm_pm_if_eye_margin_proj(P_C, P_ID, I_S, L_M, rate, ber_scan_mode, timer_control, max_error_control)      bcm_plp_aperta_eye_margin_proj((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, rate, ber_scan_mode, timer_control, max_error_control)  
#else                                                                                                                                                                                                                
#define bcm_pm_if_eye_margin_proj(P_C, P_ID, I_S,  L_M, rate, ber_scan_mode, timer_control, max_error_control)     bcm_plp_aperta_eye_margin_proj((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, rate, ber_scan_mode, timer_control, max_error_control)
#endif                                                                                                                                                                                                                    
#define bcm_pm_if_repeater_mode_get(P_C, P_ID, L_M,  ena_dis)                                                         bcm_repeater_mode_get((bcm_plp_access_t){P_C, P_ID, 0xFF, L_M}, ena_dis) 
#define bcm_pm_if_repeater_mode_set(P_C, P_ID, L_M,  ena_dis)                                                         bcm_plp_aperta_repeater_mode_set((bcm_plp_access_t){P_C, P_ID, 0xFF, L_M}, ena_dis)
#define bcm_pm_if_module_read(P_C, P_ID, L_M,  slv_addr,  start_addr,  no_of_bytes, read_data)                     bcm_plp_aperta_module_read ((bcm_plp_access_t){P_C, P_ID, 0xFF, L_M}, slv_addr, start_addr, no_of_bytes, read_data)
#define bcm_pm_if_module_write(P_C, P_ID, L_M,  slv_addr,  start_addr,  no_of_bytes, write_data)                   bcm_plp_aperta_module_write((bcm_plp_access_t){P_C, P_ID, 0xFF, L_M}, slv_addr, start_addr, no_of_bytes, write_data)
#define bcm_pm_if_cfg_gpio_pin_set(P_C, P_ID,  gpio_pin_number,  cfg_direction,  cfg_pull,  pin_value)             bcm_plp_aperta_cfg_gpio_pin_set((bcm_plp_access_t){P_C, P_ID, 0xFF,0xFF},  gpio_pin_number,  cfg_direction,  cfg_pull,  pin_value)
#define bcm_pm_if_cfg_gpio_pin_get(P_C, P_ID,  gpio_pin_number, cfg_direction, cfg_pull, pin_value)               bcm_plp_aperta_cfg_gpio_pin_get((bcm_plp_access_t){P_C, P_ID, 0xFF,0xFF},  gpio_pin_number, cfg_direction, cfg_pull, pin_value)
#define bcm_pm_if_firmware_lane_config_set(P_C, P_ID, I_S, L_M, firmware_lane_config)                                      bcm_plp_aperta_firmware_lane_config_set((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, firmware_lane_config)
#define bcm_pm_if_firmware_lane_config_get(P_C, P_ID, I_S, L_M, firmware_lane_config)                                      bcm_plp_aperta_firmware_lane_config_get((bcm_plp_access_t){P_C, P_ID, I_S, L_M}, firmware_lane_config)
#endif
/* New API */
int bcm_plp_aperta_static_config_set(bcm_plp_access_t phy_info, void* bcm_static_config);

int bcm_plp_aperta_static_config_get(bcm_plp_access_t phy_info, void* bcm_static_config);
int bcm_plp_aperta_init(bcm_plp_access_t phy_info, int (*read)(void* user_acc,
            unsigned int core_addr, unsigned int reg_addr, unsigned int* val), 
            int (*write)(void* user_acc, unsigned int core_addr, unsigned int reg_addr,
             unsigned int val), bcm_pm_firmware_load_method_t firmware_load_method);

int bcm_plp_aperta_cleanup(bcm_plp_access_t phy_info);

int bcm_plp_aperta_link_status_get(bcm_plp_access_t phy_info, unsigned int *link_status);

int bcm_plp_aperta_mode_config_set(bcm_plp_access_t phy_info, int speed, int if_type,
                            int ref_clk, int interface_mode, void* device_aux_modes);

int bcm_plp_aperta_mode_config_get(bcm_plp_access_t phy_info, int *speed, 
                            int *if_type, int *ref_clk, int *interface_mode,
                            void *device_aux_modes);

void bcm_plp_aperta_version_get(unsigned short *chip_ver, unsigned short *api_ver,
                         unsigned short *enahan_ver);

int bcm_plp_aperta_prbs_set(bcm_plp_access_t phy_info, unsigned int tx_rx, 
                     unsigned int poly, unsigned int invert,
                     unsigned int loopback, unsigned int ena_dis);

int bcm_plp_aperta_prbs_get(bcm_plp_access_t phy_info, unsigned int tx_rx,
                     unsigned int *poly, unsigned int *invert,
                     unsigned int *loopback, unsigned int *ena_dis);

int bcm_plp_aperta_prbs_rx_stat(bcm_plp_access_t phy_info, unsigned int time);

int bcm_plp_aperta_prbs_clear(bcm_plp_access_t phy_info, unsigned int tx_rx);

int bcm_plp_aperta_prbs_config_get(bcm_plp_access_t phy_info, unsigned int tx_rx,
                            unsigned int *poly, unsigned int *invert);

int bcm_plp_aperta_prbs_status_get(bcm_plp_access_t phy_info, unsigned int *prbs_lock,
                            unsigned int *prbs_lock_loss, unsigned int *error_count);  

int bcm_plp_aperta_reg_value_set(bcm_plp_access_t phy_info, unsigned int devaddr,
                          unsigned int regaddr, unsigned int data);

int bcm_plp_aperta_reg_value_get(bcm_plp_access_t phy_info, unsigned int devaddr,
                          unsigned int regaddr, unsigned int *data);

int bcm_plp_aperta_polarity_set(bcm_plp_access_t phy_info, unsigned int tx_pol,
                         unsigned int rx_pol);

int bcm_plp_aperta_polarity_get(bcm_plp_access_t phy_info, unsigned int *tx_pol,
                         unsigned int *rx_pol);

int bcm_plp_aperta_rx_pmd_lock_get(bcm_plp_access_t phy_info, unsigned int* rx_pmd_lock);

int bcm_plp_aperta_rev_id(bcm_plp_access_t phy_info, unsigned int* rev_id);


int bcm_plp_aperta_loopback_set(bcm_plp_access_t phy_info, unsigned int lb_mode,
                         unsigned int enable);

int bcm_plp_aperta_loopback_get(bcm_plp_access_t phy_info, unsigned int lb_mode,
                         unsigned int *enable);

int bcm_plp_aperta_tx_set(bcm_plp_access_t phy_info, bcm_plp_tx_t* tx);

int bcm_plp_aperta_tx_get(bcm_plp_access_t phy_info, bcm_plp_tx_t* tx);

int bcm_plp_aperta_pam4_tx_set(bcm_plp_access_t phy_info, bcm_plp_pam4_tx_t* tx);

int bcm_plp_aperta_pam4_tx_get(bcm_plp_access_t phy_info, bcm_plp_pam4_tx_t* tx);

int bcm_plp_aperta_rx_set(bcm_plp_access_t phy_info, bcm_plp_rx_t* rx);

int bcm_plp_aperta_rx_get(bcm_plp_access_t phy_info, bcm_plp_rx_t* rx);

int bcm_plp_aperta_reset_set(bcm_plp_access_t phy_info, unsigned int reset_mode,
                      unsigned int reset_val);

int bcm_plp_aperta_phy_lane_reset_set(bcm_plp_access_t phy_info, bcm_plp_pm_phy_reset_t* reset);

int bcm_plp_aperta_phy_lane_reset_get(bcm_plp_access_t phy_info, bcm_plp_pm_phy_reset_t* reset);

int bcm_plp_aperta_tx_lane_control_set(bcm_plp_access_t phy_info, bcm_pm_phy_tx_lane_control_t tx_control);

int bcm_plp_aperta_rx_lane_control_set(bcm_plp_access_t phy_info, bcm_pm_phy_rx_lane_control_t rx_control);

int bcm_plp_aperta_tx_lane_control_get(bcm_plp_access_t phy_info, bcm_pm_phy_tx_lane_control_t *tx_control);

int bcm_plp_aperta_rx_lane_control_get(bcm_plp_access_t phy_info, bcm_pm_phy_rx_lane_control_t *rx_control);

int bcm_plp_aperta_lane_cross_switch_map_set(bcm_plp_access_t phy_info, unsigned int* tx_source_array);

int bcm_plp_aperta_lane_cross_switch_map_get(bcm_plp_access_t phy_info, unsigned int *mapped_to);

int bcm_plp_aperta_force_tx_training_set(bcm_plp_access_t phy_info, unsigned int enable);

int bcm_plp_aperta_force_tx_training_get(bcm_plp_access_t phy_info, unsigned int *enable);

int bcm_plp_aperta_force_tx_training_status_get(bcm_plp_access_t phy_info, unsigned int *enabled,
                                         unsigned int *training_failure, unsigned int *trained);
#ifdef BCM_PLP_BASE_T_PHY
int bcm_base_t_autoneg_ability_set(bcm_plp_access_t phy_info,
                                   bcm_plp_base_t_ability_t *ability);
int bcm_base_t_autoneg_ability_get(bcm_plp_access_t phy_info,
                                   bcm_plp_base_t_ability_t *ability);
int bcm_base_t_autoneg_set(bcm_plp_access_t phy_info, int  enable);
int bcm_base_t_autoneg_get(bcm_plp_access_t phy_info, int *enable, int *an_done);
#endif /* BCM_PLP_BASE_T_PHY */

int bcm_plp_aperta_cl73_ability_set(bcm_plp_access_t phy_info, unsigned short tech_ability,
                             unsigned short fec_ability, unsigned short pause_ability, bcm_plp_an_config_t an_config);

int bcm_plp_aperta_cl73_ability_get(bcm_plp_access_t phy_info, unsigned short *tech_ability,
                             unsigned short *fec_ability, unsigned short *pause_ability, bcm_plp_an_config_t *an_config);
int bcm_plp_aperta_cl73_set(bcm_plp_access_t phy_info, unsigned short ena_dis);

int bcm_plp_aperta_cl73_get(bcm_plp_access_t phy_info, unsigned int *an,
                     unsigned int *an_done);

int bcm_plp_aperta_display_eye_scan(bcm_plp_access_t phy_info);

int bcm_plp_aperta_firmware_info_get(bcm_plp_access_t phy_info, unsigned int *fw_version,
                              unsigned int *fw_crc);

int bcm_plp_aperta_firmware_set(bcm_plp_access_t phy_info);

int bcm_plp_aperta_rxtx_laneswap_set(bcm_plp_access_t phy_info, bcm_laneswap_map_t* laneswap_map);

int bcm_plp_aperta_rxtx_laneswap_get(bcm_plp_access_t phy_info, bcm_laneswap_map_t* laneswap_map);

int bcm_plp_aperta_pll_sequencer_restart(bcm_plp_access_t phy_info, unsigned char flags,
                                  bcm_pm_sequencer_operation_t operation);

int bcm_plp_aperta_fec_enable_set(bcm_plp_access_t phy_info, unsigned int enable);

int bcm_plp_aperta_fec_enable_get(bcm_plp_access_t phy_info, unsigned int* enable);

int bcm_plp_aperta_phy_status_dump(bcm_plp_access_t phy_info);

int bcm_plp_aperta_phy_diagnostics_get(bcm_plp_access_t phy_info, bcm_plp_pm_phy_diagnostics_t* diag);

int bcm_plp_aperta_intr_status_get(bcm_plp_access_t phy_info, unsigned int intr_type,
                            unsigned int* intr_status);

int bcm_plp_aperta_intr_enable_set(bcm_plp_access_t phy_info, unsigned int intr_type,
                            unsigned int enable);

int bcm_plp_aperta_intr_enable_get(bcm_plp_access_t phy_info, unsigned int intr_type,
                            unsigned int* enable);

int bcm_plp_aperta_intr_status_clear(bcm_plp_access_t phy_info, unsigned int intr_type);

#ifdef BCM_PLP_BASE_T_PHY
int bcm_base_t_eee_set(bcm_plp_access_t phy_info, bcm_plp_base_t_eee_t *eee_conf);
int bcm_base_t_eee_get(bcm_plp_access_t phy_info, bcm_plp_base_t_eee_t *eee_conf);

int bcm_base_t_power_mode_set(bcm_plp_access_t phy_info, bcm_plp_base_t_power_mode_t  power_mode);
int bcm_base_t_power_mode_get(bcm_plp_access_t phy_info, bcm_plp_base_t_power_mode_t *power_mode);

int bcm_base_t_cable_diag(bcm_plp_access_t phy_info, bcm_plp_base_t_cable_diag_t *cdiag);
int bcm_base_t_state_mfg_diag(bcm_plp_access_t phy_info, unsigned int inst,
                              bcm_plp_base_t_diag_ctrl_type_t op_type,
                              bcm_plp_base_t_diag_ctrl_cmd_t  op_cmd, void *arg);
#endif /* BCM_PLP_BASE_T_PHY */

int bcm_plp_aperta_fc_pcs_chkr_enable_set(bcm_plp_access_t phy_info, unsigned int fcpcs_chkr_mode,
                                   unsigned int enable);

int bcm_plp_aperta_fc_pcs_chkr_enable_get(bcm_plp_access_t phy_info, unsigned int fcpcs_chkr_mode,
                                   unsigned int* enable);

int bcm_plp_aperta_fc_pcs_chkr_status_get(bcm_plp_access_t phy_info, unsigned int *lock_status,
                                   unsigned int* lock_lost_lh, unsigned int* error_count);
#ifdef SERDES_API_FLOATING_POINT
int bcm_plp_aperta_eye_margin_proj(bcm_plp_access_t phy_info, double rate,
                            unsigned char ber_scan_mode, unsigned char timer_control,
                            unsigned char max_error_control);
#else 
int bcm_plp_aperta_eye_margin_proj(bcm_plp_access_t phy_info, int rate,
                            unsigned char ber_scan_mode, unsigned char timer_control,
                            unsigned char max_error_control);
#endif

int bcm_repeater_mode_get(bcm_plp_access_t phy_info, unsigned int *ena_dis);

int bcm_plp_aperta_repeater_mode_set(bcm_plp_access_t phy_info, unsigned int ena_dis);

int bcm_plp_aperta_module_read(bcm_plp_access_t phy_info, unsigned int slv_addr,
                        unsigned int start_addr, unsigned int no_of_bytes,
                        unsigned char *read_data);

int bcm_plp_aperta_module_write(bcm_plp_access_t phy_info, unsigned int slv_addr,
                         unsigned int start_addr, unsigned int no_of_bytes,
                         unsigned char *write_data);

int bcm_plp_aperta_peripheral_device_control(bcm_plp_access_t phy_info,
                                  int peripheral_dev, int peripheral_dev_ctrl,
                                  bcm_plp_peripheral_dev_ctrl_data_t *ctrl_data);

int bcm_plp_aperta_cfg_gpio_pin_set(bcm_plp_access_t phy_info, unsigned int gpio_pin_number,
                             unsigned int cfg_direction, unsigned int cfg_pull,
                             unsigned int pin_value);

int bcm_plp_aperta_cfg_gpio_pin_get(bcm_plp_access_t phy_info, unsigned int gpio_pin_number,
                             unsigned int *cfg_direction, unsigned int *cfg_pull,
                             unsigned int *pin_value);

int bcm_plp_aperta_power_set(bcm_plp_access_t phy_info,  unsigned int power_rx, unsigned int power_tx);

int bcm_plp_aperta_power_get(bcm_plp_access_t phy_info,  unsigned int *power_rx, unsigned int *power_tx);

int bcm_plp_aperta_firmware_lane_config_set(bcm_plp_access_t phy_info, bcm_plp_pm_firmware_lane_config_t* firmware_lane_config);

int bcm_plp_aperta_firmware_lane_config_get(bcm_plp_access_t phy_info, bcm_plp_pm_firmware_lane_config_t* firmware_lane_config);
int
bcm_plp_aperta_init_fw_bcast(bcm_plp_access_t phy_info,
                  int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val),
                  int (*write)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int val),
                  bcm_plp_firmware_load_type_t *firmware_load_type,
                  bcm_pm_firmware_broadcast_method_t broadcast_method);
int bcm_plp_aperta_failover_mode_set(bcm_plp_access_t phy_info,unsigned int failover_mode);
int bcm_plp_aperta_failover_mode_get(bcm_plp_access_t phy_info,unsigned int *failover_mode);

int bcm_plp_aperta_edc_config_set(bcm_plp_access_t phy_info, unsigned int edc_method, unsigned int edc_value );
int bcm_plp_aperta_edc_config_get(bcm_plp_access_t phy_info, unsigned int *edc_method, unsigned int *edc_value );
int bcm_plp_aperta_driver_version_get(bcm_plp_access_t phy_info, char *phy_chip_name, unsigned short *major_ver, unsigned short *minor_ver);

int bcm_plp_aperta_fec_corrected_error_counter(bcm_plp_access_t phy_info, bcm_plp_fec_type_t fec_type, unsigned int *count);

int bcm_plp_aperta_fec_uncorrected_error_counter(bcm_plp_access_t phy_info, bcm_plp_fec_type_t fec_type, unsigned int *count);

int bcm_plp_aperta_event_status_get(bcm_plp_access_t phy_info,  unsigned int event_type, unsigned int* event_status);

int bcm_plp_aperta_timesync_config_set(bcm_plp_access_t phy_info, bcm_plp_timesync_config_t* config);
int bcm_plp_aperta_timesync_config_get(bcm_plp_access_t phy_info, bcm_plp_timesync_config_t* config);
int bcm_plp_aperta_timesync_enable_set(bcm_plp_access_t phy_info, unsigned int flags, unsigned int enable);
int bcm_plp_aperta_timesync_enable_get(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* enable);
int bcm_plp_aperta_timesync_nco_addend_set(bcm_plp_access_t phy_info, unsigned int flags,  unsigned int freq_step);
int bcm_plp_aperta_timesync_nco_addend_get(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* freq_step);
int bcm_plp_aperta_timesync_framesync_mode_set(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_framesync_t* framesync);
int bcm_plp_aperta_timesync_framesync_mode_get(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_framesync_t* framesync);
int bcm_plp_aperta_timesync_local_time_set(bcm_plp_access_t phy_info, unsigned int flags, plp_uint64_t local_time);
int bcm_plp_aperta_timesync_local_time_get(bcm_plp_access_t phy_info, unsigned int flags, plp_uint64_t* local_time);
int bcm_plp_aperta_timesync_load_ctrl_set(bcm_plp_access_t phy_info, unsigned int flags, unsigned int load_once, unsigned int load_always);
int bcm_plp_aperta_timesync_load_ctrl_get(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* load_once, unsigned int* load_always);
int bcm_plp_aperta_timesync_timing_control_set(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *time_ctrl);
int bcm_plp_aperta_timesync_timing_control_get(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *time_ctrl);
int bcm_plp_aperta_timesync_link_delay_set(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *linkdelay);
int bcm_plp_aperta_timesync_link_delay_get(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *linkdelay);
int bcm_plp_aperta_timesync_timestamp_offset_set(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *ts_offset);
int bcm_plp_aperta_timesync_timestamp_offset_get(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_time_value_t *ts_offset);
int bcm_plp_aperta_timesync_time_code_set(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_timespec_t *timecode);
int bcm_plp_aperta_timesync_time_code_get(bcm_plp_access_t phy_info, unsigned int flags, bcm_plp_timesync_timespec_t *timecode);
int bcm_plp_aperta_timesync_tx_timestamp_offset_set(bcm_plp_access_t phy_info, unsigned int flags, unsigned int ts_offset);
int bcm_plp_aperta_timesync_tx_timestamp_offset_get(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* ts_offset);
int bcm_plp_aperta_timesync_rx_timestamp_offset_set(bcm_plp_access_t phy_info, unsigned int flags, unsigned int ts_offset);
int bcm_plp_aperta_timesync_rx_timestamp_offset_get(bcm_plp_access_t phy_info, unsigned int flags, unsigned int* ts_offset);
int bcm_plp_aperta_timesync_capture_timestamp_get(bcm_plp_access_t phy_info, unsigned int flags, plp_uint64_t* cap_ts);
int bcm_plp_aperta_timesync_heartbeat_timestamp_get(bcm_plp_access_t phy_info, unsigned int flags, plp_uint64_t* hb_ts);
int bcm_plp_aperta_timesync_do_sync(bcm_plp_access_t phy_info);
int bcm_plp_aperta_mutex_info_set(bcm_plp_access_t phy_info, bcm_plp_mutex_info_t *mutex_info);
int bcm_plp_aperta_l1_intr_status_get(bcm_plp_access_t phy_info, bcm_plp_l1_intr_status_t *l1_intr_status);
int bcm_plp_aperta_phy_pam4_diagnostics_get(bcm_plp_access_t phy_info,  bcm_plp_pm_phy_pam4_diagnostics_t* diag);
int bcm_plp_aperta_jfec_config_set(bcm_plp_access_t phy_info, bcm_plp_jfec_config_t jfec_config);
int bcm_plp_aperta_fec_status_get(bcm_plp_access_t phy_info, bcm_plp_fec_status_t *fec_status);
int bcm_plp_aperta_kp4_fec_config_get(bcm_plp_access_t phy_info, bcm_plp_kp4_fec_config_t *kp4_fec_config);
int bcm_plp_aperta_kp4_fec_config_set(bcm_plp_access_t phy_info, bcm_plp_kp4_fec_config_t kp4_fec_config);
int bcm_plp_aperta_jfec_config_get(bcm_plp_access_t phy_info, bcm_plp_jfec_config_t *jfec_config);
int bcm_plp_aperta_core_diagnostics_get(bcm_plp_access_t phy_info, bcm_plp_core_diagnostics_t *core_diag);
#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_APERTA2_SUPPORT) || defined(PHYMOD_AGERA2_SUPPORT)
int bcm_avs_config_set(bcm_plp_access_t phy_info, bcm_plp_avs_config_t avs_config);
int bcm_avs_config_get(bcm_plp_access_t phy_info, bcm_plp_avs_config_t* avs_config);
int bcm_avs_status_get(bcm_plp_access_t phy_info, bcm_plp_avs_config_status_t* avs_status);
#endif
#if defined(PHYMOD_ESTOQUE_SUPPORT) || defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT)
int bcm_prbs_error_inject_set(bcm_plp_access_t phy_info, bcm_plp_prbs_error_inject_t prbs_error_inject);
int bcm_plp_aperta_dsrds_firmware_lane_config_set(bcm_plp_access_t phy_info, bcm_plp_dsrds_firmware_lane_config_t firmware_lane_config);
int bcm_plp_aperta_dsrds_firmware_lane_config_get(bcm_plp_access_t phy_info, bcm_plp_dsrds_firmware_lane_config_t* firmware_lane_config);
#endif
int bcm_plp_aperta_logical_lane_set(bcm_plp_access_t phy_info, bcm_plp_logical_lane_map_t logical_lane);
int bcm_plp_aperta_logical_lane_get(bcm_plp_access_t phy_info, bcm_plp_logical_lane_map_t *logical_lane);
int bcm_plp_aperta_prbs_error_analyzer_proj(bcm_plp_access_t phy_info, unsigned short prbs_error_fec_size, unsigned char hist_errcnt_thresh, unsigned int timeout_s);
#ifdef BARCHETTA_DEBUG_SUPPORT
int bcm_get_hwport_number(bcm_plp_access_t phy_info, int *port_number) ;
#endif
int bcm_plp_aperta_synce_config_set(bcm_plp_access_t phy_info, bcm_plp_synce_cfg_t* synce_cfg);
int bcm_plp_aperta_synce_config_get(bcm_plp_access_t phy_info, bcm_plp_synce_cfg_t* synce_cfg);
int
bcm_plp_aperta_autoneg_remote_ability_get(bcm_plp_access_t phy_info, unsigned short *fec_ability, 
                               unsigned short *pause_ability, bcm_plp_an_config_t* an_config);
#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_AGERA2_SUPPORT) || defined (PHYMOD_APERTA_SUPPORT)
int bcm_plp_aperta_pcs_status_get(bcm_plp_access_t phy_info, bcm_plp_pcs_status_t* pcs_status);
#endif
#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_BARCHETTA2_SUPPORT) || defined(PHYMOD_BARCHETTA_SUPPORT) || defined(PHYMOD_APERTA_SUPPORT)
int bcm_plp_aperta_pattern_enable_set(bcm_plp_access_t phy_info, const bcm_plp_pattern_t *pattern);
int bcm_plp_aperta_pattern_enable_get(bcm_plp_access_t phy_info, bcm_plp_pattern_t *pattern);
#endif
int bcm_plp_aperta_fw_init_params_get(bcm_plp_access_t phy_info, void* const fw_init_params);
int bcm_plp_aperta_srds_diag_access_enable(bcm_plp_access_t phy_info, bcm_plp_srds_diag_access_cfg_t *diag_access_cfg);
int bcm_plp_aperta_phy_pai_info_get(bcm_plp_access_t phy_info, bcm_plp_pai_info_t *pai_info);
#endif
