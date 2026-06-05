/*
 *
 * $Id:$
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 * File:    cdmac.h
 * Purpose:
 *
 *
 *
 */

#ifndef _PORTMOD_CDMAC_H_
#define _PORTMOD_CDMAC_H_

#include <include/portmod.h>

#define CDMAC_NULL_FLAGS (0)

#define CDMAC_ENABLE_RX  (1)
#define CDMAC_ENABLE_TX  (2)

#define CDMAC_ENABLE_SET_FLAGS_SOFT_RESET         0x1
#define CDMAC_ENABLE_SET_FLAGS_TX_EN              0x2
#define CDMAC_ENABLE_SET_FLAGS_RX_EN              0x4

#define CDMAC_RUNT_THRESHOLD_MIN      64
#define CDMAC_RUNT_THRESHOLD_MAX      96
#define CDMAC_RUNT_THRESHOLD_DEFAULT  64

#define CDMAC_JUMBO_MAXSZ         0x3FFF

#define CDMAC_PAD_THRESHOLD_SIZE_MIN       64
#define CDMAC_PAD_THRESHOLD_SIZE_MAX       96
#define CDMAC_PAD_THRESHOLD_SIZE_DEFAULT   64

#define CDMAC_AVERAGE_IPG_DEFAULT  12

#define CDMAC_INIT_F_RX_STRIP_CRC               0x1
#define CDMAC_INIT_F_TX_APPEND_CRC              0x2
#define CDMAC_INIT_F_TX_REPLACE_CRC             0x4
#define CDMAC_INIT_F_TX_PASS_THROUGH_CRC_MODE   0x8

#define CDMAC_RSV_MASK_WRONG_SA             (0x1 << (0))
#define CDMAC_RSV_MASK_STACK_VLAN_DETECT    (0x1 << (1))
#define CDMAC_RSV_MASK_PFC_DA_ERR           (0x1 << (2)) /* unsupported PFC DA*/
#define CDMAC_RSV_MASK_PAUSE_DA_ERR         (0x1 << (2)) /* same bit as PFC */
#define CDMAC_RSV_MASK_RCV_TERM_OR_CODE_ERR (0x1 << (3))
#define CDMAC_RSV_MASK_CRC_ERR              (0x1 << (4))
#define CDMAC_RSV_MASK_FRAME_LEN_ERR   (0x1 << (5)) /* IEEE length check fail */
#define CDMAC_RSV_MASK_TRUNCATED_FRAME (0x1 << (6)) /* truncated/out of range */
#define CDMAC_RSV_MASK_FRAME_RCV_OK    (0x1 << (7))
#define CDMAC_RSV_MASK_MCAST_FRAME          (0x1 << (8))
#define CDMAC_RSV_MASK_BCAST_FRAME          (0x1 << (9))
#define CDMAC_RSV_MASK_PROMISCUOUS_FRAME    (0x1 << (10))
#define CDMAC_RSV_MASK_CONTROL_FRAME        (0x1 << (11))
#define CDMAC_RSV_MASK_PAUSE_FRAME          (0x1 << (12))
#define CDMAC_RSV_MASK_OPCODE_ERR           (0x1 << (13))
#define CDMAC_RSV_MASK_VLAN_TAG_DETECT      (0x1 << (14))
#define CDMAC_RSV_MASK_UCAST_FRAME          (0x1 << (15))
#define CDMAC_RSV_MASK_RESERVED_0           (0x1 << (16))
#define CDMAC_RSV_MASK_RESERVED_1           (0x1 << (17))
#define CDMAC_RSV_MASK_PFC_FRAME            (0x1 << (18))

#define CDMAC_RSV_MASK_MIN           CDMAC_RSV_MASK_WRONG_SA
#define CDMAC_RSV_MASK_MAX           CDMAC_RSV_MASK_PFC_FRAME
#define CDMAC_RSV_MASK_ALL           ((CDMAC_RSV_MASK_PFC_FRAME) | \
                                     ((CDMAC_RSV_MASK_PFC_FRAME) - 1))

typedef enum aperta_cdmac_port_mode_e{
    CDMAC_4_LANES_SEPARATE  = 0,
    CDMAC_3_TRI_0_1_2_2     = 1,
    CDMAC_3_TRI_0_0_2_3     = 2,
    CDMAC_2_LANES_DUAL      = 3,
    CDMAC_4_LANES_TOGETHER  = 4
} aperta_cdmac_port_mode_t;
int plp_aperta_cdmac_init(const plp_aperta_phymod_phy_access_t *phy, uint32_t init_flags);
int plp_aperta_cdmac_speed_set(const plp_aperta_phymod_phy_access_t *phy, int flags, int speed);
int plp_aperta_cdmac_speed_get     (const plp_aperta_phymod_phy_access_t *phy, int *speed, int speed_to_configure);
int plp_aperta_cdmac_encap_set(const plp_aperta_phymod_phy_access_t *phy, int flags,
                    portmod_encap_t encap);
int plp_aperta_cdmac_encap_get(const plp_aperta_phymod_phy_access_t *phy, int *flags,
                    portmod_encap_t *encap);
int plp_aperta_cdmac_enable_set(const plp_aperta_phymod_phy_access_t *phy, int flags, int enable);
int plp_aperta_cdmac_enable_get(const plp_aperta_phymod_phy_access_t *phy, int flags, int *enable);
int plp_aperta_cdmac_duplex_set(const plp_aperta_phymod_phy_access_t *phy, int duplex);
int plp_aperta_cdmac_duplex_get(const plp_aperta_phymod_phy_access_t *phy, int *duplex);

int plp_aperta_cdmac_loopback_set(const plp_aperta_phymod_phy_access_t *phy, portmod_loopback_mode_t lb,
                       int enable);
int plp_aperta_cdmac_loopback_get(const plp_aperta_phymod_phy_access_t *phy, portmod_loopback_mode_t lb,
                       int *enable);
int plp_aperta_cdmac_discard_set   (const plp_aperta_phymod_phy_access_t *phy, int discard);
int plp_aperta_cdmac_tx_enable_set (const plp_aperta_phymod_phy_access_t *phy, int enable);
int plp_aperta_cdmac_tx_enable_get (const plp_aperta_phymod_phy_access_t *phy, int *enable);
int plp_aperta_cdmac_rx_enable_set (const plp_aperta_phymod_phy_access_t *phy, int enable);
int plp_aperta_cdmac_rx_enable_get (const plp_aperta_phymod_phy_access_t *phy, int *enable);
int plp_aperta_cdmac_tx_mac_sa_set(const plp_aperta_phymod_phy_access_t *phy, pm_mac_addr_t mac);
int plp_aperta_cdmac_tx_mac_sa_get(const plp_aperta_phymod_phy_access_t *phy, pm_mac_addr_t mac);
int plp_aperta_cdmac_rx_mac_sa_set(const plp_aperta_phymod_phy_access_t *phy, pm_mac_addr_t mac);
int plp_aperta_cdmac_rx_mac_sa_get(const plp_aperta_phymod_phy_access_t *phy, pm_mac_addr_t mac);
int plp_aperta_cdmac_soft_reset_set(const plp_aperta_phymod_phy_access_t *phy, int enable);
int plp_aperta_cdmac_soft_reset_get(const plp_aperta_phymod_phy_access_t *phy, int *enable);
int plp_aperta_cdmac_rx_vlan_tag_set(const plp_aperta_phymod_phy_access_t *phy, int outer_vlan_tag,
                          int inner_vlan_tag);
int plp_aperta_cdmac_rx_vlan_tag_get(const plp_aperta_phymod_phy_access_t *phy, int *outer_vlan_tag,
                          int *inner_vlan_tag);
int plp_aperta_cdmac_rx_max_size_set(const plp_aperta_phymod_phy_access_t *phy, int value);
int plp_aperta_cdmac_rx_max_size_get(const plp_aperta_phymod_phy_access_t *phy, int *value);
int plp_aperta_cdmac_pad_size_set(const plp_aperta_phymod_phy_access_t *phy, int value);
int plp_aperta_cdmac_pad_size_get(const plp_aperta_phymod_phy_access_t *phy, int *value);
int plp_aperta_cdmac_tx_average_ipg_set(const plp_aperta_phymod_phy_access_t *phy, int val);
int plp_aperta_cdmac_tx_average_ipg_get(const plp_aperta_phymod_phy_access_t *phy, int *val);
int plp_aperta_cdmac_runt_threshold_set(const plp_aperta_phymod_phy_access_t *phy, int value);
int plp_aperta_cdmac_runt_threshold_get(const plp_aperta_phymod_phy_access_t *phy, int *value);
int plp_aperta_cdmac_remote_fault_control_get(const plp_aperta_phymod_phy_access_t *phy,
                            portmod_remote_fault_control_t *control);
int plp_aperta_cdmac_remote_fault_control_set(const plp_aperta_phymod_phy_access_t *phy,
                            const portmod_remote_fault_control_t *control);
int plp_aperta_cdmac_local_fault_control_get(const plp_aperta_phymod_phy_access_t *phy,
                            portmod_local_fault_control_t *control);
int plp_aperta_cdmac_local_fault_control_set(const plp_aperta_phymod_phy_access_t *phy,
                             const portmod_local_fault_control_t *control);

int plp_aperta_cdmac_local_fault_status_get  (const plp_aperta_phymod_phy_access_t *phy, int *status);
int plp_aperta_cdmac_remote_fault_status_get (const plp_aperta_phymod_phy_access_t *phy, int *status);

int plp_aperta_cdmac_pfc_control_set(const plp_aperta_phymod_phy_access_t *phy,
                          const portmod_pfc_control_t *control);
int plp_aperta_cdmac_pfc_control_get(const plp_aperta_phymod_phy_access_t *phy,
                          portmod_pfc_control_t *control);
int plp_aperta_cdmac_pause_control_set(const plp_aperta_phymod_phy_access_t *phy,
                            const portmod_pause_control_t *control);
int plp_aperta_cdmac_pause_control_get(const plp_aperta_phymod_phy_access_t *phy,
                            portmod_pause_control_t *control);

int plp_aperta_cdmac_pfc_config_set(const plp_aperta_phymod_phy_access_t *phy,
                         const portmod_pfc_config_t* pfc_cfg);
int plp_aperta_cdmac_pfc_config_get(const plp_aperta_phymod_phy_access_t *phy,
                         portmod_pfc_config_t* pfc_cfg);

int plp_aperta_cdmac_pass_control_frame_set(const plp_aperta_phymod_phy_access_t *phy, int value);
int plp_aperta_cdmac_pass_control_frame_get(const plp_aperta_phymod_phy_access_t *phy, int *value);

int plp_aperta_cdmac_pass_pfc_frame_set(const plp_aperta_phymod_phy_access_t *phy, int value);
int plp_aperta_cdmac_pass_pfc_frame_get(const plp_aperta_phymod_phy_access_t *phy, int *value);

int plp_aperta_cdmac_pass_pause_frame_set(const plp_aperta_phymod_phy_access_t *phy, int value);
int plp_aperta_cdmac_pass_pause_frame_get(const plp_aperta_phymod_phy_access_t *phy, int *value);

int plp_aperta_cdmac_lag_failover_loopback_set(const plp_aperta_phymod_phy_access_t *phy, int val);
int plp_aperta_cdmac_lag_failover_loopback_get(const plp_aperta_phymod_phy_access_t *phy, int *val);
int plp_aperta_cdmac_lag_failover_disable(const plp_aperta_phymod_phy_access_t *phy);

int plp_aperta_cdmac_lag_failover_en_get(const plp_aperta_phymod_phy_access_t *phy, int *val);
int plp_aperta_cdmac_lag_failover_en_set(const plp_aperta_phymod_phy_access_t *phy, int val);

int plp_aperta_cdmac_reset_fc_timers_on_link_dn_get (const plp_aperta_phymod_phy_access_t *phy, int *val);
int plp_aperta_cdmac_reset_fc_timers_on_link_dn_set (const plp_aperta_phymod_phy_access_t *phy, int val);

int plp_aperta_cdmac_lag_remove_failover_lpbk_get(const plp_aperta_phymod_phy_access_t *phy, int *val);
int plp_aperta_cdmac_lag_remove_failover_lpbk_set(const plp_aperta_phymod_phy_access_t *phy, int val);

int plp_aperta_cdmac_mac_ctrl_set(const plp_aperta_phymod_phy_access_t *phy, uint64_t ctrl);
int plp_aperta_cdmac_drain_cell_get(const plp_aperta_phymod_phy_access_t *phy,
                           portmod_drain_cells_t *drain_cells);
int plp_aperta_cdmac_drain_cell_stop(const plp_aperta_phymod_phy_access_t *phy,
                          const portmod_drain_cells_t *drain_cells);
int plp_aperta_cdmac_drain_cell_start(const plp_aperta_phymod_phy_access_t *phy);

int plp_aperta_cdmac_txfifo_cell_cnt_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t* val);
int plp_aperta_cdmac_egress_queue_drain_get(const plp_aperta_phymod_phy_access_t *phy, uint64_t *mac_ctrl,
                                 int *rx_en);

int plp_aperta_cdmac_drain_cells_rx_enable(const plp_aperta_phymod_phy_access_t *phy, int rx_en);
int plp_aperta_cdmac_egress_queue_drain_rx_en(const plp_aperta_phymod_phy_access_t *phy, int rx_en);
int plp_aperta_cdmac_reset_check(const plp_aperta_phymod_phy_access_t *phy, int enable, int *reset);

int plp_aperta_cdmac_sw_link_status_select_set(const plp_aperta_phymod_phy_access_t *phy, int enable);
int plp_aperta_cdmac_sw_link_status_select_get(const plp_aperta_phymod_phy_access_t *phy, int *enable);
int plp_aperta_cdmac_sw_link_status_set (const plp_aperta_phymod_phy_access_t *phy, int link);
int plp_aperta_cdmac_sw_link_status_get (const plp_aperta_phymod_phy_access_t *phy, int *link);

int plp_aperta_cdmac_rsv_mask_control_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t flags, uint32_t value);
int plp_aperta_cdmac_rsv_mask_control_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t flags, uint32_t *value);
int plp_aperta_cdmac_rsv_mask_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t rsv_mask);
int plp_aperta_cdmac_rsv_mask_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t *rsv_mask);

int plp_aperta_cdmac_mib_counter_control_set(const plp_aperta_phymod_phy_access_t *phy,
                                  int enable, int clear);
int plp_aperta_cdmac_cntmaxsize_set(const plp_aperta_phymod_phy_access_t *phy, int val);
int plp_aperta_cdmac_cntmaxsize_get(const plp_aperta_phymod_phy_access_t *phy, int *val);

int plp_aperta_cdmac_link_down_sequence_enable_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t value);
int plp_aperta_cdmac_link_down_sequence_enable_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t *value);

int plp_aperta_cdmac_interrupt_enable_set(const plp_aperta_phymod_phy_access_t *phy, int intr_type, uint32_t value);
int plp_aperta_cdmac_interrupt_enable_get(const plp_aperta_phymod_phy_access_t *phy, int intr_type, uint32_t *value);
int plp_aperta_cdmac_interrupt_status_get(const plp_aperta_phymod_phy_access_t *phy, int intr_type, uint32_t *value);
int plp_aperta_cdmac_interrupts_status_get(const plp_aperta_phymod_phy_access_t *phy, int arr_max_size,
                                uint32_t* intr_arr, uint32_t* size);

int plp_aperta_cdmac_reg64_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data);
int plp_aperta_cdmac_reg64_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data);
#endif /*_PORTMOD_CDMAC_H_*/
