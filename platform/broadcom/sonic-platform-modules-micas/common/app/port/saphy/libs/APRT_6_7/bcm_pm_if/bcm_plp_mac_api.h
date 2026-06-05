/*
 *         
 * $Id: bcm_plp_mac_api.h $
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */
#ifndef BCM_PLP_MAC_API_H
#define BCM_PLP_MAC_API_H

#ifdef BCM_PLP_MAC_SUPPORT
#include "bcm_pm_if.h"
#include "bcm_pm_if_api.h"


#define BCM_PLP_MAC_GET_P_CTXT(_PI, IDX, E_P)                              \
    do{                                                                    \
        _bcm_plp_aperta_pm_if_get_phy_id_idx(_PI.phy_addr, &IDX, &E_P);               \
        if (E_P != 1) {                                                    \
            _plp_aperta_phyid_list[_PI.phy_addr].valid = 0;                           \
            PHYMOD_DEBUG_ERROR(("PHY does not exist\n"));                  \
            rv = BCM_PM_IF_PHY_NA;                                         \
            goto ERR;                                                      \
        }                                                                  \
        if (IDX == BCM_PM_IF_MAX_PHY) {                                    \
            PHYMOD_DEBUG_ERROR(("MAX PHY reached\n"));                     \
            rv = BCM_PM_IF_INTERNAL;                                       \
            goto ERR;                                                      \
        }                                                                  \
        if (_PI.platform_ctxt == NULL) {                                   \
            _PI.platform_ctxt = plp_aperta_phy_ctrl.phy[IDX]->pm_phy.access.user_acc; \
        }                                                                  \
    }while(0);

#define BCM_PLP_MAC_FILL_PHY_ACCESS(_PI, IDX)                         \
    do {                                                              \
        if (_PI.platform_ctxt != NULL) {                              \
            plp_aperta_phy_ctrl.phy[IDX]->pm_phy.access.user_acc = _PI.platform_ctxt;\
            plp_aperta_phy_ctrl.phy[IDX]->core->pm_core.access.user_acc = _PI.platform_ctxt;\
        }                                                              \
        plp_aperta_phy_ctrl.phy[IDX]->pm_phy.access.lane_mask = _PI.lane_map;     \
        (_PI.if_side == 0) ? (plp_aperta_phy_ctrl.phy[IDX]->pm_phy.port_loc = phymodPortLocLine) : (plp_aperta_phy_ctrl.phy[IDX]->pm_phy.port_loc = phymodPortLocSys);\
    } while (0);


#define BCM_PLP_MAC_API_UNAVAIL_CHECK(rv)             \
    if (rv == BCM_PM_IF_UNAVAIL) {                    \
        PHYMOD_DEBUG_ERROR(("API not available\n"));  \
    }                                                 \

#define BCM_PLP_MAC_GET_PORT(LM, port)                                 \
    if ((LM == 0xFF)|| (LM == 0xF) || (LM == 0x3) || (LM == 0x1)) {    \
        port = 0;                                                      \
    } else if (LM == 0x2) {                                            \
        port = 1;                                                      \
    } else if ((LM == 0x4) || (LM == 0xC)) {                           \
        port = 2;                                                      \
    } else if(LM == 0x8) {                                             \
        port = 3;                                                      \
    } else if((LM == 0x10) || (LM == 0x30) ||  (LM == 0xF0)) {         \
        port = 4;                                                      \
    } else if(LM == 0x20) {                                            \
        port = 5;                                                      \
    } else if((LM == 0x40) || (LM == 0xC0)) {                          \
        port = 6;                                                      \
    } else if(LM == 0x80) {                                            \
        port = 7;                                                      \
    } else {                                                           \
        port = 0;                                                      \
    }

#define BCM_PLP_MAC_IF_ERR_GOTO_ERR(A) \
    do {   \
        int loc_err ; \
        if ((loc_err = (A)) != PHYMOD_E_NONE) \
        {  goto ERR ; } \
    } while (0)


int bcm_plp_aperta_mac_loopback_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_lb_type_t lb_type, unsigned int enable);
int bcm_plp_aperta_mac_loopback_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_lb_type_t lb_type, unsigned int *enable);
int bcm_plp_aperta_mac_fault_option_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_fault_option_t fault_option);
int bcm_plp_aperta_mac_fault_option_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_fault_option_t *fault_option);
int bcm_plp_aperta_mac_flow_control_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_flow_control_t flow_option);
int bcm_plp_aperta_mac_flow_control_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_flow_control_t *flow_option);
int bcm_plp_aperta_mac_store_and_forward_mode_set(bcm_plp_mac_access_t mac_info, int enable);
int bcm_plp_aperta_mac_store_and_forward_mode_get(bcm_plp_mac_access_t mac_info, int *is_enable);
int bcm_plp_aperta_reg64_value_set(bcm_plp_access_t phy_info, unsigned int devaddr, unsigned int regaddr, plp_uint64_t data);
int bcm_plp_aperta_reg64_value_get(bcm_plp_access_t phy_info, unsigned int devaddr, unsigned int regaddr, plp_uint64_t *data);
int bcm_plp_aperta_mac_cleanup(bcm_plp_mac_access_t mac_info);

int bcm_plp_aperta_mac_max_packet_size_set(bcm_plp_mac_access_t mac_info, int pkt_size);
int bcm_plp_aperta_mac_max_packet_size_get(bcm_plp_mac_access_t mac_info, int *pkt_size);

int bcm_plp_aperta_mac_runt_threshold_set(bcm_plp_mac_access_t mac_info, int threshold);
int bcm_plp_aperta_mac_runt_threshold_get(bcm_plp_mac_access_t mac_info, int *threshold);

int bcm_plp_aperta_mac_pad_size_set(bcm_plp_mac_access_t mac_info, int pad_size);
int bcm_plp_aperta_mac_pad_size_get(bcm_plp_mac_access_t mac_info, int *pad_size);

int bcm_plp_aperta_tx_mac_sa_set(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6]);
int bcm_plp_aperta_tx_mac_sa_get(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6]);

int bcm_plp_aperta_rx_mac_sa_set(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6]);
int bcm_plp_aperta_rx_mac_sa_get(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6]);

int bcm_plp_aperta_mac_tx_avg_ipg_set(bcm_plp_mac_access_t mac_info, int avg_ipg);
int bcm_plp_aperta_mac_tx_avg_ipg_get(bcm_plp_mac_access_t mac_info, int *avg_ipg);

int bcm_plp_aperta_mac_tx_preamble_length_set(bcm_plp_mac_access_t mac_info, int preamble_length);
int bcm_plp_aperta_mac_tx_preamble_length_get(bcm_plp_mac_access_t mac_info, int *preamble_length);

int bcm_plp_aperta_mac_configure_frame_drop_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_frame_select_t frame, unsigned int  enable);
int bcm_plp_aperta_mac_configure_frame_drop_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_frame_select_t frame, unsigned int  *enable);

int bcm_plp_aperta_mac_pause_control_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pause_control_t pause_control);
int bcm_plp_aperta_mac_pause_control_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pause_control_t *pause_control);

int bcm_plp_aperta_mac_pfc_control_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pfc_control_t pfc_ctrl);
int bcm_plp_aperta_mac_pfc_control_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pfc_control_t *pfc_ctrl);

int bcm_plp_aperta_mac_diagnostic_dump(bcm_plp_mac_access_t mac_info);

int bcm_plp_aperta_warmboot_init(bcm_plp_mac_access_t mac_info);
int bcm_plp_aperta_datapath_flush(bcm_plp_mac_access_t mac_info);
int bcm_plp_aperta_port_enable_set(bcm_plp_mac_access_t mac_info, int tx_rx, int enable_disable);
int bcm_plp_aperta_port_enable_get(bcm_plp_mac_access_t mac_info, int tx_rx, int *enable_disable);
int bcm_plp_aperta_pcs_diagnostic_dump(bcm_plp_mac_access_t mac_info);
int bcm_plp_aperta_pm_timesync_enable_get(bcm_plp_mac_access_t mac_info, unsigned int flags, unsigned int *enable);
int bcm_plp_aperta_pm_timesync_tx_info_get(bcm_plp_mac_access_t mac_info, bcm_plp_pm_ts_tx_info_t* ts_tx_info);
int bcm_plp_aperta_pm_timesync_enable_set(bcm_plp_mac_access_t mac_info, unsigned int flags, unsigned int enable);
int bcm_plp_aperta_mac_mib_stat_get(bcm_plp_mac_access_t mac_info, bcm_plp_mib_stat_type_t stat_type, plp_uint64_t *count);
int bcm_plp_aperta_update_port_config(bcm_plp_mac_access_t mac_info, void *prt_cfg);
#endif
#endif
