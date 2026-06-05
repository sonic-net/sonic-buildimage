/*
 *
 * $Id: epdm_base_t_sec.h $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */

#ifndef EPDM_BASE_T_SEC_H
#define EPDM_BASE_T_SEC_H

#include "epdm_bcm_base_t_common_sec_defines.h"
#include "epdm_bcm_common_defines.h"
#include "epdm_custom_config.h"
#define __PLP_NOT_USED ((void*)0)

/* SECY APIs */
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_init_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_settings_t * const settings_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_uninit_f)(bcm_plp_base_t_sec_phy_access_t *pa);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_limits_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 unsigned int * const max_vport_count_p,
                                 unsigned int * const max_rule_count_p,
                                 unsigned int * const max_sa_count_p,
                                 unsigned int * const max_sc_count_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_vport_add_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 bcm_plp_base_t_secy_vport_handle_t *vport_handle_p);


typedef unsigned char (*__bcm_plp_base_t_secy_vport_handle_issame_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_vport_handle_t * const handle1_p,
                                 const bcm_plp_base_t_secy_vport_handle_t * const handle2_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_vport_remove_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_vport_handle_t vport_handle);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_add_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                 bcm_plp_base_t_secy_sa_handle_t * const sa_handle_p,
                                 const bcm_plp_base_t_secy_sa_t * const sa_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_update_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                 const bcm_plp_base_t_secy_sa_t * const sa_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_read_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                 const unsigned int word_offset,
                                 const unsigned int word_count,
                                 unsigned int * transform_p);

typedef unsigned char (*__bcm_plp_base_t_secy_sa_handle_issame_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_sa_handle_t * const handle1_p,
                                 const bcm_plp_base_t_secy_sa_handle_t * const handle2_p);

typedef unsigned char (*__bcm_plp_base_t_secy_sa_handle_sa_index_issame_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                 const unsigned int sa_index);


typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_remove_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_sa_handle_t sa_handle);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_nextpn_update_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                 const unsigned int nextpn_lo,
                                 const unsigned int nextpn_hi,
                                 unsigned char * const fnextpn_written_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_chain_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_sa_handle_t active_sa_handle,
                                 bcm_plp_base_t_secy_sa_handle_t * const new_sa_handle_p,
                                 const bcm_plp_base_t_secy_sa_t * const new_sa_p);


typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_switch_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_sa_handle_t active_sa_handle,
                                 const bcm_plp_base_t_secy_sa_handle_t new_sa_handle,
                                 const bcm_plp_base_t_secy_sa_t * const new_sa_p);


typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_add_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                 bcm_plp_base_t_secy_rule_handle_t *rule_handle_p,
                                 const bcm_plp_base_t_secy_rule_t *rule_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_remove_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_rule_handle_t rule_handle);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_update_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_rule_handle_t rule_handle,
                                 const bcm_plp_base_t_secy_rule_t *rule_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_enable_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_rule_handle_t rule_handle,
                                 const unsigned char fsync);

typedef unsigned char (*__bcm_plp_base_t_secy_rule_handle_issame_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_rule_handle_t * const handle1_p,
                                 const bcm_plp_base_t_secy_rule_handle_t * const handle2_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_disable_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_rule_handle_t rule_handle,
                                 const unsigned char fsync);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_enable_disable_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_rule_handle_t rule_handle_disable,
                                 const bcm_plp_base_t_secy_rule_handle_t rule_handle_enable,
                                 const unsigned char enable_all,
                                 const unsigned char disable_all,
                                 const unsigned char fsync);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_index_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                 unsigned int * const sa_index_p,
                                 unsigned int * const sc_index_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_index_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_rule_handle_t rule_handle,
                                 unsigned int * const rule_index_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_vport_index_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                 unsigned int * const vport_index_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_handle_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const unsigned int sa_index,
                                 bcm_plp_base_t_secy_sa_handle_t * const sa_handle_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_handle_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const unsigned int rule_index,
                                 bcm_plp_base_t_secy_rule_handle_t * const rule_handle_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_vport_handle_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                 const unsigned int vport_index,
                                 bcm_plp_base_t_secy_vport_handle_t * const vport_handle_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_update_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                              const bcm_plp_base_t_secy_device_params_t  * const device_p );

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_crypt_auth_bypass_len_update_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                              const unsigned int bypass_length);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rules_mtu_check_update_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                    const unsigned int sci_index,
                                                                    const bcm_plp_base_t_secy_sc_rule_mtu_check_t * const mtucheck_rule_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_tcam_statistics_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                const bcm_plp_base_t_secy_rule_handle_t rule_handle,
                                                                                bcm_plp_base_t_secy_tcam_stat_t * const stat_p,
                                                                                const unsigned char fsync);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rxcam_statistics_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                 const unsigned int scindex,
                                                                                 bcm_plp_base_t_secy_rxcam_stat_t* const stats_p,
                                                                                 const unsigned char fsync);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_secy_statistics_i_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                 bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                                                                 bcm_plp_base_t_secy_secy_stat_i_t * const stats_p,
                                                                                 const unsigned char fsync);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_secy_statistics_e_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                 bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                                                                 bcm_plp_base_t_secy_secy_stat_e_t * const stats_p,
                                                                                 const unsigned char fsync);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_statistics_i_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                           const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                                                           bcm_plp_base_t_secy_sa_stat_i_t * const stats_p,
                                                                           const unsigned char fsync);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_statistics_e_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                            const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                                                            bcm_plp_base_t_secy_sa_stat_e_t * const stats_p,
                                                                            const unsigned char fsync);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_global_statistics_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                 bcm_plp_base_t_secy_global_stat_t * const stat_p,
                                                                                 const unsigned char fsync);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_ifc_statistics_e_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                    bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                                                    bcm_plp_base_t_secy_ifc_stat_e_t * const stats_p,
                                                                    const unsigned char fsync);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_ifc_statistics_i_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                        bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                                                        bcm_plp_base_t_secy_ifc_stat_i_t * const stats_p,
                                                                        const unsigned char fsync);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_count_summary_pifc_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                                           bcm_plp_base_t_secy_vport_handle_t ** const vport_handle_pp,
                                                                                                           unsigned int * const num_ifc_indexes_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_count_summary_prxcam_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                                         unsigned int ** const rx_cam_indexes_pp,
                                                                                                         unsigned int * const num_rx_cam_indexes_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_count_summary_psa_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                           bcm_plp_base_t_secy_sa_handle_t ** const sa_handle_pp,
                                                                                           unsigned int * const num_sa_indexes_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_count_summary_psecy_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                                                bcm_plp_base_t_secy_vport_handle_t ** const vport_handle_pp,
                                                                                                                unsigned int * const nm_secy_indexes_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_count_summary_ptcam_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                 bcm_plp_base_t_secy_rule_handle_t ** const rule_handle_pp,
                                                                                 unsigned int * const num_tcam_indexes_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_count_summary_sa_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                    bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                                                                    unsigned int * const count_summary_sa_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_count_summary_secy_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                                 bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                                                                                 unsigned int * const count_summary_secy_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_count_summary_tcam_global_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                                          unsigned int * const count_summary_tcam_global_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_count_summary_ifc_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                           bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                                                                           unsigned int * const count_summary_ifc_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_expired_summary_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                      bcm_plp_base_t_secy_sa_handle_t ** const sa_handle_pp,
                                                                                      unsigned int * const num_sa_indexes_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_pnthr_summary_checkandclear_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                       bcm_plp_base_t_secy_sa_handle_t ** const sa_handle_pp,
                                                                                       unsigned int * const num_sa_indexes_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_intr_enable_set_f)(bcm_plp_base_t_sec_phy_access_t *pa, const bcm_plp_base_t_secy_intr_t *secy_intr_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_intr_enable_get_f)(bcm_plp_base_t_sec_phy_access_t *pa, bcm_plp_base_t_secy_intr_t *secy_intr_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_intr_status_get_f)(bcm_plp_base_t_sec_phy_access_t *pa, bcm_plp_base_t_secy_intr_t *secy_intr_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_intr_status_clear_f)(bcm_plp_base_t_sec_phy_access_t *pa, const bcm_plp_base_t_secy_intr_t *secy_intr_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_event_status_get_f)(bcm_plp_base_t_sec_phy_access_t *pa, bcm_plp_base_t_secy_intr_t *secy_intr_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_intr_set_f)(bcm_plp_base_t_sec_phy_access_t *pa, const bcm_plp_base_t_secy_intr_t *secy_intr, const unsigned int enable);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_intr_get_f)(bcm_plp_base_t_sec_phy_access_t *pa, bcm_plp_base_t_secy_intr_t *secy_intr, unsigned int *enable);
typedef bcm_plp_base_t_sa_builder_status_t (*__bcm_plp_base_t_secy_build_transform_record_f)(bcm_plp_base_t_sec_phy_access_t *pa, bcm_plp_base_t_sa_builder_params_t *params, unsigned int *sa_buffer_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_active_e_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                  const unsigned int vport,
                                  bcm_plp_base_t_secy_sa_handle_t * const active_sa_handle_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_active_i_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                  const unsigned int vport,
                                  const unsigned char * const sci_p,
                                  bcm_plp_base_t_secy_sa_handle_t * const active_sa_handle_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_window_size_update_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                                      const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                                                                      const unsigned int window_size);

typedef int (*__bcm_plp_base_t_imacsec_version_get_f)(bcm_plp_base_t_sec_phy_access_t *pa, bcm_plp_base_t_version_t* version_info);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_bypass_set_f)(bcm_plp_base_t_sec_phy_access_t *pa, const unsigned char fbypass);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_bypass_get_f)(bcm_plp_base_t_sec_phy_access_t *pa, unsigned char *fbypass);


typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_vport_params_get_f)( bcm_plp_base_t_sec_phy_access_t *pa,
                                                                    const bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                                                    bcm_plp_base_t_secy_sa_t * const sa_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_params_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                           const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                           bcm_plp_base_t_secy_sa_t * const sa_p,
                                           unsigned char * const sci_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_next_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                        const bcm_plp_base_t_secy_sa_handle_t current_sa_handle,
                                        bcm_plp_base_t_secy_sa_handle_t * const next_sa_handle_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_window_size_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                         const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                         unsigned int * const  window_size_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_next_pn_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                            const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                            unsigned int * const next_pn_lo_p,
                                            unsigned int * const next_pn_hi_p,
                                            unsigned char * const fext_pn_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_vport_next_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                              const bcm_plp_base_t_secy_vport_handle_t current_vport_handle,
                                              bcm_plp_base_t_secy_vport_handle_t * const next_vport_handle_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                             const bcm_plp_base_t_secy_rule_handle_t rule_handle,
                                             bcm_plp_base_t_secy_rule_t * const rule_p,
                                             unsigned char * const f_enabled_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_next_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                    const bcm_plp_base_t_secy_rule_handle_t current_rule_handle,
                                    bcm_plp_base_t_secy_rule_handle_t * const next_rule_handle_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rule_vport_next_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                        const bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                        const bcm_plp_base_t_secy_rule_handle_t current_rule_handle,
                                        bcm_plp_base_t_secy_rule_handle_t * const next_rule_handle_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_chained_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                            const bcm_plp_base_t_secy_sa_handle_t current_sa_handle,
                                            bcm_plp_base_t_secy_sa_handle_t * const next_sa_handle_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_rules_mtu_check_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                       unsigned int sci_index,
                                                                       bcm_plp_base_t_secy_sc_rule_mtu_check_t * const mtu_check_rule_p);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_diag_vport_dump_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                        const bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                        const unsigned char f_all_vports,
                                        const unsigned char f_include_sa);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_diag_sa_dump_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                                                const unsigned char f_all_sas);

typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_diag_rule_dump_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                      const bcm_plp_base_t_secy_rule_handle_t rule_handle,
                                                      const unsigned char f_all_rules);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sa_vport_handle_get)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                        const bcm_plp_base_t_secy_sa_handle_t sa_handle,
                                                                        bcm_plp_base_t_secy_vport_handle_t * const vport_handle_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_sci_next_get)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                const bcm_plp_base_t_secy_vport_handle_t vport_handle,
                                                                const void ** tmp_handle_p,
                                                                unsigned char * const next_sci_p,
                                                                unsigned int * const sci_index_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_diag_device_dump)(bcm_plp_base_t_sec_phy_access_t *pa);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_config_get)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                       bcm_plp_base_t_secy_device_params_t * const control_p);
typedef bcm_plp_base_t_secy_status_t (*__bcm_plp_base_t_secy_device_role_get)(bcm_plp_base_t_sec_access_t *pa,
                            bcm_plp_base_t_secy_role_t * const role_p);
/* UNIMAC APIs */
typedef int (*__bcm_plp_base_t_unimac_reset_f)(bcm_plp_mac_access_t mac_info, int reset);
typedef int (*__bcm_plp_base_t_unimac_pause_set_f)(bcm_plp_mac_access_t mac_info, int pause);
typedef int (*__bcm_plp_base_t_unimac_datapath_flush_f)(bcm_plp_mac_access_t mac_info, int enable_disable);
typedef int (*__bcm_plp_base_t_unimac_port_enable_f)(bcm_plp_mac_access_t mac_info, int tx_rx, int enable_disable);


/* 64-bit Register set & get */
typedef int (*__bcm_plp_base_t_reg64_value_set_f)(bcm_plp_access_t phy_info, unsigned int devaddr, unsigned int regaddr, plp_uint64_t data);
typedef int (*__bcm_plp_base_t_reg64_value_get_f)(bcm_plp_access_t phy_info, unsigned int devaddr, unsigned int regaddr, plp_uint64_t *data);

/* XLMAC APIs */
typedef int (*__bcm_plp_base_t_mac_loopback_set_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_lb_type_t lb_type, unsigned int enable);
typedef int (*__bcm_plp_base_t_mac_loopback_get_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_lb_type_t lb_type, unsigned int *enable);
typedef int (*__bcm_plp_base_t_mac_fault_option_set_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_fault_option_t fault_option);
typedef int (*__bcm_plp_base_t_mac_fault_option_get_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_fault_option_t *fault_option);
typedef int (*__bcm_plp_base_t_mac_flow_control_set_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_flow_control_t flow_option);
typedef int (*__bcm_plp_base_t_mac_flow_control_get_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_flow_control_t *flow_option);
typedef int (*__bcm_plp_base_t_mac_store_and_forward_mode_set_f)(bcm_plp_mac_access_t mac_info, int enable);
typedef int (*__bcm_plp_base_t_mac_store_and_forward_mode_get_f)(bcm_plp_mac_access_t mac_info, int *is_enable);
typedef int (*__bcm_plp_base_t_mac_cleanup_f)(bcm_plp_mac_access_t mac_info);
typedef int (*__bcm_plp_base_t_mac_max_packet_size_set_f)(bcm_plp_mac_access_t mac_info, int pkt_size);
typedef int (*__bcm_plp_base_t_mac_max_packet_size_get_f)(bcm_plp_mac_access_t mac_info, int *pkt_size);
typedef int (*__bcm_plp_base_t_mac_runt_threshold_set_f)(bcm_plp_mac_access_t mac_info, int threshold);
typedef int (*__bcm_plp_base_t_mac_runt_threshold_get_f)(bcm_plp_mac_access_t mac_info, int *threshold);
typedef int (*__bcm_plp_base_t_mac_pad_size_set_f)(bcm_plp_mac_access_t mac_info, int pad_size);
typedef int (*__bcm_plp_base_t_mac_pad_size_get_f)(bcm_plp_mac_access_t mac_info, int *pad_size);
typedef int (*__bcm_plp_base_t_mac_tx_sa_set_f)(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6]);
typedef int (*__bcm_plp_base_t_mac_tx_sa_get_f)(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6]);
typedef int (*__bcm_plp_base_t_mac_rx_sa_set_f)(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6]);
typedef int (*__bcm_plp_base_t_mac_rx_sa_get_f)(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6]);
typedef int (*__bcm_plp_base_t_mac_tx_avg_ipg_set_f)(bcm_plp_mac_access_t mac_info, int avg_ipg);
typedef int (*__bcm_plp_base_t_mac_tx_avg_ipg_get_f)(bcm_plp_mac_access_t mac_info, int *avg_ipg);
typedef int (*__bcm_plp_base_t_mac_tx_preamble_length_set_f)(bcm_plp_mac_access_t mac_info, int preamble_length);
typedef int (*__bcm_plp_base_t_mac_tx_preamble_length_get_f)(bcm_plp_mac_access_t mac_info, int *preamble_length);
typedef int (*__bcm_plp_base_t_mac_pause_control_set_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pause_control_t pause_control);
typedef int (*__bcm_plp_base_t_mac_pause_control_get_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pause_control_t *pause_control);
typedef int (*__bcm_plp_base_t_mac_configure_frame_drop_set_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_frame_select_t frame, unsigned int enable);
typedef int (*__bcm_plp_base_t_mac_configure_frame_drop_get_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_frame_select_t frame, unsigned int* enable);
typedef int (*__bcm_plp_base_t_mac_pfc_control_set_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pfc_control_t pfc_ctrl);
typedef int (*__bcm_plp_base_t_mac_pfc_control_get_f)(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pfc_control_t *pfc_ctrl);
typedef int (*__bcm_plp_base_t_mac_diagnostic_dump_f)(bcm_plp_mac_access_t mac_info);
typedef int (*__bcm_plp_base_t_mac_datapath_flush_f)(bcm_plp_mac_access_t mac_info);
typedef int (*__bcm_plp_base_t_mac_port_enable_set_f)(bcm_plp_mac_access_t mac_info, int tx_rx, int enable_disable);
typedef int (*__bcm_plp_base_t_mac_port_enable_get_f)(bcm_plp_mac_access_t mac_info, int tx_rx, int *enable_disable);

typedef int (*__bcm_plp_base_t_mac_reset_set_f)(bcm_plp_mac_access_t mac_info, int  reset);
typedef int (*__bcm_plp_base_t_mac_reset_get_f)(bcm_plp_mac_access_t mac_info, int *reset);

typedef bcm_plp_base_t_warmboot_status_t (*__bcm_plp_base_t_macsec_warmboot_register_f)(bcm_plp_base_t_sec_phy_access_t *pa, bcm_plp_base_t_macsec_warmboot_callbacks_t *ops);
typedef bcm_plp_base_t_warmboot_status_t (*__bcm_plp_base_t_macsec_warmboot_shutdown_f)(bcm_plp_base_t_sec_phy_access_t *pa, const unsigned int areaid);
typedef bcm_plp_base_t_warmboot_status_t (*__bcm_plp_base_t_macsec_warmboot_restore_f)(bcm_plp_base_t_sec_phy_access_t *pa, const unsigned int areaid);
typedef bcm_plp_base_t_warmboot_status_t (*__bcm_plp_base_t_macsec_warmboot_maxsize_get_f)(bcm_plp_base_t_sec_phy_access_t *pa,
                                                                             const bcm_plp_base_t_macsec_warmboot_device_type_t device_type,
                                                                             unsigned int * const maxbyte_count_p);

typedef struct __plp__base__t__sec__dispatch__s__ {
    __bcm_plp_base_t_secy_device_init_f __secy_device_init;
    __bcm_plp_base_t_secy_device_uninit_f __secy_device_uninit;
    __bcm_plp_base_t_secy_device_limits_f __secy_device_limits;
    __bcm_plp_base_t_secy_vport_add_f __secy_vport_add;
    __bcm_plp_base_t_secy_vport_handle_issame_f __secy_vport_handle_issame;
    __bcm_plp_base_t_secy_vport_remove_f __secy_vport_remove;
    __bcm_plp_base_t_secy_sa_add_f __secy_sa_add;
    __bcm_plp_base_t_secy_sa_update_f __secy_sa_update;
    __bcm_plp_base_t_secy_sa_read_f __secy_sa_read;
    __bcm_plp_base_t_secy_sa_handle_issame_f __secy_sa_handle_issame;
    __bcm_plp_base_t_secy_sa_handle_sa_index_issame_f __secy_sa_handle_sa_index_issame;
    __bcm_plp_base_t_secy_sa_remove_f __secy_sa_remove;
    __bcm_plp_base_t_secy_sa_nextpn_update_f __secy_sa_nextpn_update;
    __bcm_plp_base_t_secy_sa_chain_f __secy_sa_chain;
    __bcm_plp_base_t_secy_sa_switch_f __secy_sa_switch;
    __bcm_plp_base_t_secy_rule_add_f __secy_rule_add;
    __bcm_plp_base_t_secy_rule_update_f __secy_rule_update;
    __bcm_plp_base_t_secy_rule_enable_f __secy_rule_enable;
    __bcm_plp_base_t_secy_rule_enable_disable_f __secy_rule_enable_disable;
    __bcm_plp_base_t_secy_rule_handle_issame_f __secy_rule_handle_issame;
    __bcm_plp_base_t_secy_rule_disable_f __secy_rule_disable;
    __bcm_plp_base_t_secy_rule_remove_f __secy_rule_remove;
    __bcm_plp_base_t_secy_sa_index_get_f __secy_sa_index_get;
    __bcm_plp_base_t_secy_rule_index_get_f __secy_rule_index_get;
    __bcm_plp_base_t_secy_vport_index_get_f __secy_vport_index_get;
    __bcm_plp_base_t_secy_sa_handle_get_f __secy_sa_handle_get;
    __bcm_plp_base_t_secy_rule_handle_get_f __secy_rule_handle_get;
    __bcm_plp_base_t_secy_vport_handle_get_f __secy_vport_handle_get;
    __bcm_plp_base_t_secy_device_update_f __secy_device_update;
    __bcm_plp_base_t_secy_crypt_auth_bypass_len_update_f __secy_crypt_auth_bypass_len_update;
    __bcm_plp_base_t_secy_rules_mtu_check_update_f __secy_rules_mtu_check_update;
    __bcm_plp_base_t_secy_tcam_statistics_get_f __secy_tcam_statistics_get;
    __bcm_plp_base_t_secy_rxcam_statistics_get_f __secy_rxcam_statistics_get;
    __bcm_plp_base_t_secy_secy_statistics_i_get_f __secy_secy_statistics_i_get;
    __bcm_plp_base_t_secy_secy_statistics_e_get_f __secy_secy_statistics_e_get;
    __bcm_plp_base_t_secy_sa_statistics_i_get_f __secy_sa_statistics_i_get;
    __bcm_plp_base_t_secy_sa_statistics_e_get_f __secy_sa_statistics_e_get;
    __bcm_plp_base_t_secy_global_statistics_get_f __secy_global_statistics_get;
    __bcm_plp_base_t_secy_ifc_statistics_e_get_f __secy_ifc_statistics_e_get;
    __bcm_plp_base_t_secy_ifc_statistics_i_get_f __secy_ifc_statistics_i_get;
    __bcm_plp_base_t_secy_device_count_summary_pifc_checkandclear_f __secy_device_count_summary_pifc_checkandclear;
    __bcm_plp_base_t_secy_device_count_summary_prxcam_checkandclear_f __secy_device_count_summary_prxcam_checkandclear;
    __bcm_plp_base_t_secy_device_count_summary_psa_checkandclear_f __secy_device_count_summary_psa_checkandclear;
    __bcm_plp_base_t_secy_device_count_summary_psecy_checkandclear_f __secy_device_count_summary_psecy_checkandclear;
    __bcm_plp_base_t_secy_device_count_summary_ptcam_checkandclear_f __secy_device_count_summary_ptcam_checkandclear;
    __bcm_plp_base_t_secy_device_count_summary_sa_checkandclear_f __secy_device_count_summary_sa_checkandclear;
    __bcm_plp_base_t_secy_device_count_summary_secy_checkandclear_f __secy_device_count_summary_secy_checkandclear;
    __bcm_plp_base_t_secy_device_count_summary_tcam_global_checkandclear_f __secy_device_count_summary_tcam_global_checkandclear;
    __bcm_plp_base_t_secy_device_count_summary_ifc_checkandclear_f __secy_device_count_summary_ifc_checkandclear;
    __bcm_plp_base_t_secy_sa_expired_summary_checkandclear_f __secy_sa_expired_summary_checkandclear;
    __bcm_plp_base_t_secy_sa_pnthr_summary_checkandclear_f __secy_sa_pnthr_summary_checkandclear;
    __bcm_plp_base_t_secy_intr_enable_set_f __secy_intr_enable_set;
    __bcm_plp_base_t_secy_intr_enable_get_f __secy_intr_enable_get;
    __bcm_plp_base_t_secy_intr_status_get_f __secy_intr_status_get;
    __bcm_plp_base_t_secy_intr_status_clear_f __secy_intr_status_clear;
    __bcm_plp_base_t_secy_event_status_get_f __secy_event_status_get;
    __bcm_plp_base_t_secy_intr_set_f             __secy_intr_set;
    __bcm_plp_base_t_secy_intr_get_f             __secy_intr_get;
    __bcm_plp_base_t_secy_build_transform_record_f __secy_build_transform_record;
    __bcm_plp_base_t_secy_sa_active_e_get_f __secy_sa_active_e_get;
    __bcm_plp_base_t_secy_sa_active_i_get_f __secy_sa_active_i_get;
    __bcm_plp_base_t_imacsec_version_get_f __imacsec_version_get;
    __bcm_plp_base_t_secy_sa_window_size_update_f __secy_sa_window_size_update;
    __bcm_plp_base_t_secy_bypass_set_f __secy_bypass_set;
    __bcm_plp_base_t_secy_bypass_get_f __secy_bypass_get;
    __bcm_plp_base_t_secy_vport_params_get_f __secy_vport_params_get;
    __bcm_plp_base_t_secy_sa_params_get_f __secy_sa_params_get;
    __bcm_plp_base_t_secy_sa_next_get_f __secy_sa_next_get;
    __bcm_plp_base_t_secy_sa_window_size_get_f __secy_sa_window_size_get;
    __bcm_plp_base_t_secy_sa_next_pn_get_f __secy_sa_next_pn_get;
    __bcm_plp_base_t_secy_vport_next_get_f __secy_vport_next_get;
    __bcm_plp_base_t_secy_rule_get_f __secy_rule_get;
    __bcm_plp_base_t_secy_rule_next_get_f __secy_rule_next_get;
    __bcm_plp_base_t_secy_rule_vport_next_get_f __secy_rule_vport_next_get;
    __bcm_plp_base_t_secy_sa_chained_get_f __secy_sa_chained_get;
    __bcm_plp_base_t_secy_rules_mtu_check_get_f __secy_rules_mtu_check_get;
    __bcm_plp_base_t_secy_diag_vport_dump_f __secy_diag_vport_dump;
    __bcm_plp_base_t_secy_diag_sa_dump_f __secy_diag_sa_dump;
    __bcm_plp_base_t_secy_diag_rule_dump_f __secy_diag_rule_dump;
    __bcm_plp_base_t_secy_sa_vport_handle_get __secy_sa_vport_handle_get;
    __bcm_plp_base_t_secy_sci_next_get __secy_sci_next_get;
    __bcm_plp_base_t_secy_diag_device_dump __secy_diag_device_dump;
    __bcm_plp_base_t_secy_device_config_get __secy_device_config_get;
    __bcm_plp_base_t_secy_device_role_get __secy_device_role_get;
    void* reserved16;
    void* reserved17;
    void* reserved18;
    void* reserved19;
    void* reserved20;
    __bcm_plp_base_t_unimac_reset_f __mac_reset;
    __bcm_plp_base_t_unimac_pause_set_f __mac_pause_set;
    __bcm_plp_base_t_unimac_datapath_flush_f __mac_datapath_flush;
    __bcm_plp_base_t_unimac_port_enable_f __mac_port_enable;
    __bcm_plp_base_t_mac_loopback_set_f __mac_loopback_set;
    __bcm_plp_base_t_mac_loopback_get_f __mac_loopback_get;
    __bcm_plp_base_t_mac_fault_option_set_f __mac_fault_option_set;
    __bcm_plp_base_t_mac_fault_option_get_f __mac_fault_option_get;
    __bcm_plp_base_t_mac_flow_control_set_f __mac_flow_control_set;
    __bcm_plp_base_t_mac_flow_control_get_f __mac_flow_control_get;
    __bcm_plp_base_t_mac_store_and_forward_mode_set_f __mac_store_and_forward_mode_set;
    __bcm_plp_base_t_mac_store_and_forward_mode_get_f __mac_store_and_forward_mode_get;
    __bcm_plp_base_t_mac_cleanup_f __mac_cleanup;
    __bcm_plp_base_t_mac_max_packet_size_set_f __mac_max_packet_size_set;
    __bcm_plp_base_t_mac_max_packet_size_get_f __mac_max_packet_size_get;
    __bcm_plp_base_t_mac_runt_threshold_set_f __mac_runt_threshold_set;
    __bcm_plp_base_t_mac_runt_threshold_get_f __mac_runt_threshold_get;
    __bcm_plp_base_t_mac_pad_size_set_f __mac_pad_size_set;
    __bcm_plp_base_t_mac_pad_size_get_f __mac_pad_size_get;
    __bcm_plp_base_t_mac_tx_sa_set_f __tx_mac_sa_set;
    __bcm_plp_base_t_mac_tx_sa_get_f __tx_mac_sa_get;
    __bcm_plp_base_t_mac_rx_sa_set_f __rx_mac_sa_set;
    __bcm_plp_base_t_mac_rx_sa_get_f __rx_mac_sa_get;
    __bcm_plp_base_t_mac_tx_avg_ipg_set_f __tx_avg_ipg_set;
    __bcm_plp_base_t_mac_tx_avg_ipg_get_f __tx_avg_ipg_get;
    __bcm_plp_base_t_mac_tx_preamble_length_set_f __mac_tx_preamble_length_set;
    __bcm_plp_base_t_mac_tx_preamble_length_get_f __mac_tx_preamble_length_get;
    __bcm_plp_base_t_mac_pause_control_set_f __mac_pause_control_set;
    __bcm_plp_base_t_mac_pause_control_get_f __mac_pause_control_get;
    __bcm_plp_base_t_mac_pfc_control_set_f __mac_pfc_control_set;
    __bcm_plp_base_t_mac_pfc_control_get_f __mac_pfc_control_get;
    __bcm_plp_base_t_reg64_value_set_f __reg64_value_set;
    __bcm_plp_base_t_reg64_value_get_f __reg64_value_get;
    __bcm_plp_base_t_mac_diagnostic_dump_f __mac_diag_dump;
    __bcm_plp_base_t_mac_datapath_flush_f __datapath_flush;
    __bcm_plp_base_t_mac_port_enable_set_f __port_enable_set;
    __bcm_plp_base_t_mac_port_enable_get_f __port_enable_get;
    __bcm_plp_base_t_mac_configure_frame_drop_set_f __mac_configure_frame_drop_set;
    __bcm_plp_base_t_mac_configure_frame_drop_get_f __mac_configure_frame_drop_get;
    __bcm_plp_base_t_mac_reset_set_f __mac_reset_set;
    __bcm_plp_base_t_mac_reset_get_f __mac_reset_get;
    __bcm_plp_base_t_macsec_warmboot_register_f __macsec_warmboot_register;
    __bcm_plp_base_t_macsec_warmboot_shutdown_f __macsec_warmboot_shutdown;
    __bcm_plp_base_t_macsec_warmboot_restore_f __macsec_warmboot_restore;
    __bcm_plp_base_t_macsec_warmboot_maxsize_get_f __macsec_warmboot_maxsize_get;
} __plp__base__t__sec__dispatch__t__;

/* SECY */
#define  bcm_plp_base_t_secy_device_init(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_init, ((_a), (_b)))

#define  bcm_plp_base_t_secy_device_uninit(_pd, _a) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_uninit, ((_a)))

#define  bcm_plp_base_t_secy_device_limits(_pd, _a, _b, _c, _d, _e) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_limits, ((_a), (_b), (_c), (_d), (_e)))

#define bcm_plp_base_t_secy_vport_add(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_vport_add, ((_a), (_b)))

#define bcm_plp_base_t_secy_vport_handle_issame(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_vport_handle_issame, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_vport_remove(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_vport_remove, ((_a), (_b)))

#define  bcm_plp_base_t_secy_sa_add(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_add, ((_a), (_b), (_c), (_d)))

#define  bcm_plp_base_t_secy_sa_update(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_update, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_sa_read(_pd, _a, _b, _c, _d, _e) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_read, ((_a), (_b), (_c), (_d), (_e)))
#define  bcm_plp_base_t_secy_sa_handle_issame(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_handle_issame, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_sa_handle_sa_index_issame(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_handle_sa_index_issame, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_sa_remove(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd),  __secy_sa_remove, ((_a), (_b)))
#define bcm_plp_base_t_secy_sa_nextpn_update(_pd, _a, _b, _c, _d, _e) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_nextpn_update, ((_a), (_b), (_c), (_d), (_e)))

#define  bcm_plp_base_t_secy_sa_chain(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_chain, ((_a), (_b), (_c), (_d)))

#define  bcm_plp_base_t_secy_sa_switch(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_switch, ((_a), (_b), (_c), (_d)))

#define bcm_plp_base_t_secy_rule_add(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_add, ((_a), (_b), (_c), (_d)))

#define bcm_plp_base_t_secy_rule_update(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_update, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_rule_enable(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_enable, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_rule_enable_disable(_pd, _a, _b, _c, _d, _e, _f) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_enable_disable, ((_a), (_b), (_c), (_d), (_e), (_f)))
#define bcm_plp_base_t_secy_rule_handle_issame(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_handle_issame, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_rule_disable(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_disable, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_rule_remove(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_remove, ((_a), (_b)))

#define bcm_plp_base_t_secy_sa_index_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_index_get, ((_a), (_b), (_c), (_d)))

#define bcm_plp_base_t_secy_rule_index_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_index_get, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_vport_index_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_vport_index_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_sa_handle_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_handle_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_rule_handle_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_handle_get, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_vport_handle_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_vport_handle_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_device_update(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_update, ((_a), (_b)))

#define  bcm_plp_base_t_secy_crypt_auth_bypass_len_update(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_crypt_auth_bypass_len_update, ((_a), (_b)))

#define bcm_plp_base_t_secy_rules_mtu_check_update(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_rules_mtu_check_update, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_tcam_statistics_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_tcam_statistics_get, ((_a), (_b), (_c), (_d)))

#define  bcm_plp_base_t_secy_rxcam_statistics_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_rxcam_statistics_get, ((_a), (_b), (_c), (_d)))

#define  bcm_plp_base_t_secy_secy_statistics_i_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_secy_statistics_i_get, ((_a), (_b), (_c), (_d)))

#define  bcm_plp_base_t_secy_secy_statistics_e_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_secy_statistics_e_get, ((_a), (_b), (_c), (_d)))

#define  bcm_plp_base_t_secy_sa_statistics_i_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_statistics_i_get, ((_a), (_b), (_c), (_d)))

#define  bcm_plp_base_t_secy_sa_statistics_e_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_statistics_e_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_base_t_secy_global_statistics_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_global_statistics_get, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_ifc_statistics_e_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_ifc_statistics_e_get, ((_a), (_b), (_c), (_d)))

#define  bcm_plp_base_t_secy_ifc_statistics_i_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_ifc_statistics_i_get, ((_a), (_b), (_c), (_d)))


#define  bcm_plp_base_t_secy_device_count_summary_secy_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_count_summary_secy_checkandclear, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_device_count_summary_pifc_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_count_summary_pifc_checkandclear, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_device_count_summary_ifc_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_count_summary_ifc_checkandclear, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_device_count_summary_prxcam_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_count_summary_prxcam_checkandclear, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_device_count_summary_ptcam_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_count_summary_ptcam_checkandclear, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_device_count_summary_psa_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_count_summary_psa_checkandclear, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_device_count_summary_sa_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_count_summary_sa_checkandclear, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_sa_pnthr_summary_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_pnthr_summary_checkandclear, ((_a), (_b), (_c)))

#define  bcm_plp_base_t_secy_sa_expired_summary_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_expired_summary_checkandclear, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_device_count_summary_tcam_global_checkandclear(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_count_summary_tcam_global_checkandclear, ((_a), (_b)))

#define  bcm_plp_base_t_secy_device_count_summary_psecy_checkandclear(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_count_summary_psecy_checkandclear, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_intr_enable_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_intr_enable_set, ((_a), (_b)))

#define bcm_plp_base_t_secy_intr_enable_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_intr_enable_get, ((_a), (_b)))

#define bcm_plp_base_t_secy_intr_status_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_intr_status_get, ((_a), (_b)))

#define bcm_plp_base_t_secy_intr_status_clear(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_intr_status_clear, ((_a), (_b)))

#define bcm_plp_base_t_secy_event_status_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_event_status_get, ((_a), (_b)))

#define  bcm_plp_base_t_secy_intr_set(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_intr_set, ((_a), (_b), (_c)))
#define  bcm_plp_base_t_secy_intr_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_intr_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_build_transform_record(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_build_transform_record, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_sa_active_e_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_active_e_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_sa_active_i_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_active_i_get, ((_a), (_b), (_c), (_d)))
#define bcm_plp_base_t_secy_sa_window_size_update(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_window_size_update, ((_a), (_b), (_c)))
#define bcm_plp_base_t_imacsec_version_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __imacsec_version_get, ((_a), (_b)))
#define bcm_plp_base_t_secy_bypass_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_bypass_set, ((_a), (_b)))
#define bcm_plp_base_t_secy_bypass_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_bypass_get, ((_a), (_b)))

#define bcm_plp_base_t_secy_vport_params_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_vport_params_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_sa_params_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_params_get, ((_a), (_b), (_c), (_d)))

#define bcm_plp_base_t_secy_sa_next_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_next_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_sa_window_size_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_window_size_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_sa_next_pn_get(_pd, _a, _b, _c, _d, _e) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_next_pn_get, ((_a), (_b), (_c), (_d), (_e)))

#define bcm_plp_base_t_secy_vport_next_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_vport_next_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_rule_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_get, ((_a), (_b), (_c), (_d)))

#define bcm_plp_base_t_secy_rule_next_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_next_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_rule_vport_next_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_rule_vport_next_get, ((_a), (_b), (_c), (_d)))

#define bcm_plp_base_t_secy_sa_chained_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_chained_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_rules_mtu_check_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_rules_mtu_check_get, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_diag_vport_dump(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __secy_diag_vport_dump, ((_a), (_b), (_c), (_d)))

#define bcm_plp_base_t_secy_diag_sa_dump(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_diag_sa_dump, ((_a), (_b), (_c)))

#define bcm_plp_base_t_secy_diag_rule_dump(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_diag_rule_dump, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_sa_vport_handle_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __secy_sa_vport_handle_get, ((_a), (_b), (_c)))
#define bcm_plp_base_t_secy_sci_next_get(_pd, _a, _b, _c, _d, _e) \
    PLP_BASET_SEC_CALL((_pd), __secy_sci_next_get, ((_a), (_b), (_c), (_d), (_e)))
#define bcm_plp_base_t_secy_diag_device_dump(_pd, _a) \
    PLP_BASET_SEC_CALL((_pd), __secy_diag_device_dump, ((_a)))
#define bcm_plp_base_t_secy_device_config_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_config_get, ((_a), (_b)))
#define bcm_plp_base_t_secy_device_role_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __secy_device_role_get, ((_a), (_b)))
/* UNIMAC */
#define bcm_plp_base_t_mac_reset(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_reset, ((_a), (_b)))
#define bcm_plp_base_t_mac_pause_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_pause_set, ((_a), (_b)))
#define bcm_plp_base_t_mac_datapath_flush(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_datapath_flush, ((_a), (_b)))
#define bcm_plp_base_t_mac_port_enable(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __mac_port_enable, ((_a), (_b), (_c)))

#if !defined (BASET_HSIP_MACSEC_SUPPORT)
/* XLMAC */
#define bcm_plp_mac_loopback_set(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __mac_loopback_set, ((_a), (_b), (_c)))

#define bcm_plp_mac_loopback_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __mac_loopback_get, ((_a), (_b), (_c)))

#define bcm_plp_mac_fault_option_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_fault_option_set, ((_a), (_b)))

#define bcm_plp_mac_fault_option_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_fault_option_get, ((_a), (_b)))

#define bcm_plp_mac_flow_control_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_flow_control_set, ((_a), (_b)))

#define bcm_plp_mac_flow_control_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_flow_control_get, ((_a), (_b)))

#define bcm_plp_mac_store_and_forward_mode_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_store_and_forward_mode_set, ((_a), (_b)))

#define bcm_plp_mac_store_and_forward_mode_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_store_and_forward_mode_get, ((_a), (_b)))

#define bcm_plp_reg64_value_set(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __reg64_value_set, ((_a), (_b), (_c), (_d)))

#define bcm_plp_reg64_value_get(_pd, _a, _b, _c, _d) \
    PLP_BASET_SEC_CALL((_pd), __reg64_value_get, ((_a), (_b), (_c), (_d)))

#define bcm_plp_mac_cleanup(_pd, _a) \
    PLP_BASET_SEC_CALL((_pd), __mac_cleanup, ((_a)))

#define bcm_plp_mac_max_packet_size_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_max_packet_size_set, ((_a), (_b)))

#define bcm_plp_mac_max_packet_size_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_max_packet_size_get, ((_a), (_b)))

#define bcm_plp_mac_runt_threshold_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_runt_threshold_set, ((_a), (_b)))

#define bcm_plp_mac_runt_threshold_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_runt_threshold_get, ((_a), (_b)))

#define bcm_plp_mac_pad_size_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_pad_size_set, ((_a), (_b)))

#define bcm_plp_mac_pad_size_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_pad_size_get, ((_a), (_b)))

#define bcm_plp_tx_mac_sa_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __tx_mac_sa_set, ((_a), (_b)))

#define bcm_plp_tx_mac_sa_get(_pd, _a, _b)\
    PLP_BASET_SEC_CALL((_pd), __tx_mac_sa_get, ((_a), (_b)))

#define bcm_plp_rx_mac_sa_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __rx_mac_sa_set, ((_a), (_b)))

#define bcm_plp_rx_mac_sa_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __rx_mac_sa_get, ((_a), (_b)))

#define bcm_plp_mac_tx_avg_ipg_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __tx_avg_ipg_set, ((_a), (_b)))

#define bcm_plp_mac_tx_avg_ipg_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __tx_avg_ipg_get, ((_a), (_b)))

#define bcm_plp_mac_tx_preamble_length_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_tx_preamble_length_set, ((_a), (_b)))

#define bcm_plp_mac_tx_preamble_length_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_tx_preamble_length_get, ((_a), (_b)))

#define bcm_plp_mac_pause_control_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_pause_control_set, ((_a), (_b)))

#define bcm_plp_mac_pause_control_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_pause_control_get, ((_a), (_b)))

#define bcm_plp_mac_configure_frame_drop_set(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __mac_configure_frame_drop_set, ((_a), (_b), (_c)))

#define bcm_plp_mac_configure_frame_drop_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __mac_configure_frame_drop_get, ((_a), (_b), (_c)))

#define bcm_plp_mac_pfc_control_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_pfc_control_set, ((_a), (_b)))

#define bcm_plp_mac_pfc_control_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_pfc_control_get, ((_a), (_b)))

#define bcm_plp_mac_diagnostic_dump(_pd, _a) \
    PLP_BASET_SEC_CALL((_pd), __mac_diag_dump, (_a))

#define bcm_plp_datapath_flush(_pd, _a) \
    PLP_BASET_SEC_CALL((_pd), __datapath_flush, (_a))

#define bcm_plp_port_enable_set(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __port_enable_set, ((_a), (_b), (_c)))

#define bcm_plp_port_enable_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __port_enable_get, ((_a), (_b), (_c)))
#endif /* BASET_HSIP_MACSEC_SUPPORT */

#define bcm_plp_mac_reset_set(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_reset_set, ((_a), (_b)))

#define bcm_plp_mac_reset_get(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __mac_reset_get, ((_a), (_b)))

#define bcm_plp_base_t_macsec_warmboot_register(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __macsec_warmboot_register, ((_a), (_b)))

#define bcm_plp_base_t_macsec_warmboot_shutdown(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __macsec_warmboot_shutdown, ((_a), (_b)))

#define bcm_plp_base_t_macsec_warmboot_restore(_pd, _a, _b) \
    PLP_BASET_SEC_CALL((_pd), __macsec_warmboot_restore, ((_a), (_b)))

#define bcm_plp_base_t_macsec_warmboot_maxsize_get(_pd, _a, _b, _c) \
    PLP_BASET_SEC_CALL((_pd), __macsec_warmboot_maxsize_get, ((_a), (_b), (_c)))

extern __plp__base__t__sec__dispatch__t__ plp_seahawks_sec_dispatch;
extern __plp__base__t__sec__dispatch__t__ plp_longfin_sec_dispatch;
extern __plp__base__t__sec__dispatch__t__ plp_broadfin_sec_dispatch;
extern __plp__base__t__sec__dispatch__t__ plp_whitetip_sec_dispatch;

#define _PLP_FALSE (0)

#define PLP_CHECK_PF(__pf, __pa) ((__pf == __PLP_NOT_USED) ? _PLP_API_UNAVAILABLE : (__pf __pa))

#if (defined(PLP_MACSEC_SUPPORT)) && (defined(PLP_SEAHAWKS_SUPPORT))
    #define PLP_SEAHAWKS_SEC_COMPARE(_N, _pf, _pa) (strcmp(_N, "seahawks")) == 0 ? PLP_CHECK_PF(plp_seahawks_sec_dispatch._pf, _pa)
#else
    #define PLP_SEAHAWKS_SEC_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#if (defined(PLP_MACSEC_SUPPORT)) && (defined(PLP_LONGFIN_SUPPORT))
    #define PLP_LONGFIN_SEC_COMPARE(_N, _pf, _pa) (strcmp(_N, "longfin")) == 0 ? PLP_CHECK_PF(plp_longfin_sec_dispatch._pf, _pa)
#else
    #define PLP_LONGFIN_SEC_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#if (defined(PLP_MACSEC_SUPPORT)) && (defined(PLP_BROADFIN_SUPPORT))
    #define PLP_BROADFIN_SEC_COMPARE(_N, _pf, _pa) (strcmp(_N, "broadfin")) == 0 ? PLP_CHECK_PF(plp_broadfin_sec_dispatch._pf, _pa)
#else
    #define PLP_BROADFIN_SEC_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#if (defined(PLP_MACSEC_SUPPORT)) && (defined(PLP_WHITETIP_SUPPORT))
    #define PLP_WHITETIP_SEC_COMPARE(_N, _pf, _pa) (strcmp(_N, "whitetip")) == 0 ? PLP_CHECK_PF(plp_whitetip_sec_dispatch._pf, _pa)
#else
    #define PLP_WHITETIP_SEC_COMPARE(_N, _pf, _pa) _PLP_FALSE ? _PLP_FALSE
#endif

#define PLP_BASET_SEC_CALL(_N, _pf, _pa) (PLP_SEAHAWKS_SEC_COMPARE(_N, _pf, _pa) : \
                                   (PLP_LONGFIN_SEC_COMPARE(_N, _pf, _pa)  : \
                                   (PLP_BROADFIN_SEC_COMPARE(_N, _pf, _pa)  : \
                                   (PLP_WHITETIP_SEC_COMPARE(_N, _pf, _pa)  : \
                                    _PLP_API_UNAVAILABLE))))

#endif /* EPDM_BASE_T_SEC_H */
