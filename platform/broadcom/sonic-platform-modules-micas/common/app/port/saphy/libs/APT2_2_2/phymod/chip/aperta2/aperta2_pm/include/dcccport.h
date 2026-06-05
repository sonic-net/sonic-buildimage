/*! \file bcmpmac_dcccport.h
 *
 * CDPORT_GEN4 driver.
 *
 * A DC3PORT contains 1 DC3PORT and 1 DC3MACs, and supports 8 ports at most.
 *
 * In this driver, we always use the port number 0~7 to access the CDPORT
 * and DC3MAC per-port registers
 *
 */
/*
 * $Copyright:.$
 */

#ifndef APERTA2_DCCCPORT_H
#define APERTA2_DCCCPORT_H

#include <include/portmod.h>


#define APERTA2_DC3MAC_RSV_MASK_INCORR_SA            (0x1 << (0))
#define APERTA2_DC3MAC_RSV_MASK_STACK_VLAN_TAG_PKT   (0x1 << (1))
#define APERTA2_DC3MAC_RSV_MASK_INCORR_PAUSE_PFC_DA  (0x1 << (2))
#define APERTA2_DC3MAC_RSV_MASK_RX_TERM_ERR          (0x1 << (3))
#define APERTA2_DC3MAC_RSV_MASK_CRC_ERR              (0x1 << (4))
#define APERTA2_DC3MAC_RSV_MASK_FRAME_LEN_ERR   (0x1 << (5)) /* IEEE length check fail */
#define APERTA2_DC3MAC_RSV_MASK_LONG_PKT        (0x1 << (6)) /* truncated/out of range */
#define APERTA2_DC3MAC_RSV_MASK_GOOD_PKT             (0x1 << (7))
#define APERTA2_DC3MAC_RSV_MASK_MCAST_FRAME          (0x1 << (8))
#define APERTA2_DC3MAC_RSV_MASK_BCAST_FRAME          (0x1 << (9))
#define APERTA2_DC3MAC_RSV_MASK_PROMISCUOUS_FRAME    (0x1 << (10))
#define APERTA2_DC3MAC_RSV_MASK_CONTROL_FRAME        (0x1 << (11))
#define APERTA2_DC3MAC_RSV_MASK_PAUSE_FRAME          (0x1 << (12))
#define APERTA2_DC3MAC_RSV_MASK_OPCODE_ERR           (0x1 << (13))
#define APERTA2_DC3MAC_RSV_MASK_VLAN_TAG_DETECT      (0x1 << (14))
#define APERTA2_DC3MAC_RSV_MASK_UCAST_FRAME          (0x1 << (15))
#define APERTA2_DC3MAC_RSV_MASK_RESERVED_0           (0x1 << (16))
#define APERTA2_DC3MAC_RSV_MASK_RESERVED_1           (0x1 << (17))
#define APERTA2_DC3MAC_RSV_MASK_PFC_FRAME            (0x1 << (18))

#define APERTA2_DC3MAC_RSV_MASK_MIN           APERTA2_DC3MAC_RSV_MASK_INCORR_SA
#define APERTA2_DC3MAC_RSV_MASK_MAX           APERTA2_DC3MAC_RSV_MASK_PFC_FRAME
#define APERTA2_DC3MAC_RSV_MASK_ALL           ((APERTA2_DC3MAC_RSV_MASK_PFC_FRAME) | \
                                     ((APERTA2_DC3MAC_RSV_MASK_PFC_FRAME) - 1))

/******************************************************************************
 * Private functions
 ******************************************************************************/

int
plp_aperta2_dc3port_port_reset_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t reset);

int
plp_aperta2_dc3port_port_reset_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *reset);

int
plp_aperta2_dc3port_port_enable_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t enable);

int
plp_aperta2_dc3port_port_enable_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *enable);

int
plp_aperta2_dc3mac_reset_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t reset);

int
plp_aperta2_dc3mac_reset_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *reset);

int
plp_aperta2_dc3mac_rx_enable_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t enable);

int
plp_aperta2_dc3mac_rx_enable_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *enable);

int
plp_aperta2_dc3mac_tx_enable_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t enable);

int
plp_aperta2_dc3mac_tx_enable_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *enable);

int
plp_aperta2_dc3mac_encap_set(const plp_aperta2_phymod_phy_access_t *phy, portmod_encap_t encap);

int
plp_aperta2_dc3mac_encap_get(const plp_aperta2_phymod_phy_access_t *phy, portmod_encap_t *encap);

int
plp_aperta2_dc3port_lpbk_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t en);

int
plp_aperta2_dc3port_lpbk_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *en);

int
plp_aperta2_dc3mac_pause_set(const plp_aperta2_phymod_phy_access_t *phy,
                 const portmod_pause_control_t *ctrl);

int
plp_aperta2_dc3mac_pause_get(const plp_aperta2_phymod_phy_access_t *phy,
                 portmod_pause_control_t *ctrl);


int plp_aperta2_dc3mac_rx_mac_sa_get(const plp_aperta2_phymod_phy_access_t *phy, uint8_t mac[6]);
int plp_aperta2_dc3mac_rx_mac_sa_set(const plp_aperta2_phymod_phy_access_t *phy, uint8_t mac[6]);
int plp_aperta2_dc3mac_tx_mac_sa_get(const plp_aperta2_phymod_phy_access_t *phy, uint8_t mac[6]);
int plp_aperta2_dc3mac_tx_mac_sa_set(const plp_aperta2_phymod_phy_access_t *phy, uint8_t mac[6]);
int
plp_aperta2_dc3mac_frame_max_set(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t size);

int
plp_aperta2_dc3mac_frame_max_get(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t *size);

int
plp_aperta2_dc3mac_remote_fault_status_get(const plp_aperta2_phymod_phy_access_t *phy,
                               int *status);

int
plp_aperta2_dc3mac_local_fault_status_get(const plp_aperta2_phymod_phy_access_t *phy,
                              int *status);

int
plp_aperta2_dc3mac_pfc_control_set(const plp_aperta2_phymod_phy_access_t *phy,
                       const portmod_pfc_control_t  *cfg);

int
plp_aperta2_dc3mac_pfc_control_get(const plp_aperta2_phymod_phy_access_t *phy,
                       portmod_pfc_control_t  *cfg);

int
plp_aperta2_dc3mac_pfc_config_set(const plp_aperta2_phymod_phy_access_t *phy,
                      const portmod_pfc_config_t *cfg);

int
plp_aperta2_dc3mac_pfc_config_get(const plp_aperta2_phymod_phy_access_t *phy,
                      portmod_pfc_config_t *cfg);

int
plp_aperta2_dc3mac_local_fault_disable_set(const plp_aperta2_phymod_phy_access_t *phy,
                               const portmod_local_fault_control_t *st);

int
plp_aperta2_dc3mac_local_fault_disable_get(const plp_aperta2_phymod_phy_access_t *phy,
                               portmod_local_fault_control_t *st);

int
plp_aperta2_dc3mac_remote_fault_disable_set(const plp_aperta2_phymod_phy_access_t *phy,
                                const portmod_remote_fault_control_t *st);

int
plp_aperta2_dc3mac_remote_fault_disable_get(const plp_aperta2_phymod_phy_access_t *phy,
                                portmod_remote_fault_control_t *st);

int
plp_aperta2_dc3mac_avg_ipg_set(const plp_aperta2_phymod_phy_access_t *phy,
                   uint8_t ipg_size);

int
plp_aperta2_dc3mac_avg_ipg_get(const plp_aperta2_phymod_phy_access_t *phy,
                   uint8_t *ipg_size);

int plp_aperta2_dc3mac_interrupt_enable_get(const plp_aperta2_phymod_phy_access_t *phy, int intr_type, uint32 *value);

int plp_aperta2_dc3mac_interrupt_enable_set(const plp_aperta2_phymod_phy_access_t *phy, int intr_type, uint32 value);

int plp_aperta2_dc3mac_interrupt_status_get(const plp_aperta2_phymod_phy_access_t *phy, int intr_type, uint32 *value);

int plp_aperta2_dc3mac_interrupts_status_get(const plp_aperta2_phymod_phy_access_t *phy, int arr_max_size,
                                 uint32* intr_arr, uint32* size);
int
plp_aperta2_dc3mac_mib_counter_control_set(const plp_aperta2_phymod_phy_access_t *phy,
                               int enable, int clear);

int
plp_aperta2_dc3mac_mib_oversize_set(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t size);

int
plp_aperta2_dc3mac_mib_oversize_get(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t *size);

int
plp_aperta2_dc3mac_pass_control_frame_set(const plp_aperta2_phymod_phy_access_t *phy,
                              uint32_t enable);
int
plp_aperta2_dc3mac_pass_control_frame_get(const plp_aperta2_phymod_phy_access_t *phy,
                              uint32_t *enable);
int
plp_aperta2_dc3mac_pass_pfc_frame_set(const plp_aperta2_phymod_phy_access_t *phy,
                          uint32_t enable);
int
plp_aperta2_dc3mac_pass_pfc_frame_get(const plp_aperta2_phymod_phy_access_t *phy,
                          uint32_t *enable);
int
plp_aperta2_dc3mac_pass_pause_frame_set(const plp_aperta2_phymod_phy_access_t *phy,
                            uint32_t enable);
int
plp_aperta2_dc3mac_pass_pause_frame_get(const plp_aperta2_phymod_phy_access_t *phy,
                            uint32_t *enable);

int
plp_aperta2_dc3mac_discard_set(const plp_aperta2_phymod_phy_access_t *phy,
                   uint32_t discard);

int
plp_aperta2_dc3mac_discard_get(const plp_aperta2_phymod_phy_access_t *phy,
                   uint32_t *discard);

int
plp_aperta2_dc3mac_stall_tx_enable_get(const plp_aperta2_phymod_phy_access_t *phy,
                           int *enable);

int
plp_aperta2_dc3mac_stall_tx_enable_set(const plp_aperta2_phymod_phy_access_t *phy,
                           int enable);

int
plp_aperta2_dc3mac_runt_threshold_get(const plp_aperta2_phymod_phy_access_t *phy,
                          uint32_t *value);

int
plp_aperta2_dc3mac_port_init(const plp_aperta2_phymod_phy_access_t *phy,
                 uint32_t init);

int
plp_aperta2_dc3mac_force_pfc_xon_set(const plp_aperta2_phymod_phy_access_t *phy,
                         uint32_t value);

int
plp_aperta2_dc3mac_force_pfc_xon_get(const plp_aperta2_phymod_phy_access_t *phy,
                         uint32_t *value);

int
plp_aperta2_dc3mac_rsv_mask_set(const plp_aperta2_phymod_phy_access_t *phy,
                    uint32_t rsv_mask);

int
plp_aperta2_dc3mac_rsv_mask_get(const plp_aperta2_phymod_phy_access_t *phy,
                    uint32_t *rsv_mask);

/*
 * This function controls which RSV(Receive statistics vector) event
 * causes a purge event that triggers RXERR to be set for the packet
 * sent by the MAC to the IP. These bits are used to mask RSV[34:16]
 * for DC3MAC; bit[18] of MASK maps to bit[34] of RSV, bit[0] of MASK
 * maps to bit[16] of RSV.
 * Enable : Set 0. Go through
 * Disable: Set 1. Purged.
 * bit[18] --> PFC frame detected
 * bit[17] --> Reserved
 * bit[16] --> Reserved
 * bit[15] --> Unicast detected
 * bit[14] --> VLAN tag detected
 * bit[13] --> Unsupported opcode detected
 * bit[12] --> Pause frame received
 * bit[11] --> Control frame received
 * bit[10] --> Promiscuous packet detected
 * bit[ 9] --> Broadcast detected
 * bit[ 8] --> Multicast detected
 * bit[ 7] --> Receive OK
 * bit[ 6] --> Truncated/Frame out of Range
 * bit[ 5] --> Frame length not out of range, but incorrect -
 *             IEEE length check failed
 * bit[ 4] --> CRC error
 * bit[ 3] --> Receive terminate/code error
 * bit[ 2] --> Unsupported DA for pause/PFC packets detected
 * bit[ 1] --> Stack VLAN detected
 * bit[ 0] --> Wrong SA
 */
int
plp_aperta2_dc3mac_rsv_selective_mask_set(const plp_aperta2_phymod_phy_access_t *phy,
                              uint32_t flags, uint32_t value);

int
plp_aperta2_dc3mac_rsv_selective_mask_get(const plp_aperta2_phymod_phy_access_t *phy,
                              uint32_t flags, uint32_t *value);

int
plp_aperta2_dc3mac_strip_crc_get(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t *enable);

int
plp_aperta2_dc3mac_strip_crc_set(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t enable);

int
plp_aperta2_dc3mac_tx_crc_mode_get(const plp_aperta2_phymod_phy_access_t *phy,
                       uint32_t *crc_mode);

int
plp_aperta2_dc3mac_tx_crc_mode_set(const plp_aperta2_phymod_phy_access_t *phy,
                       uint32_t crc_mode);

int
plp_aperta2_dc3mac_tx_threshold_get(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t *threshold);

int
plp_aperta2_dc3mac_tx_threshold_set(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t threshold);

int
plp_aperta2_dc3port_port_mode_set(const plp_aperta2_phymod_phy_access_t *phy,
                      uint32_t flags, uint32_t lane_mask);

int
plp_aperta2_dc3port_dc3mac_control_set(const plp_aperta2_phymod_phy_access_t *phy,
                           uint32_t reset);

int
plp_aperta2_dc3port_dc3mac_control_get(const plp_aperta2_phymod_phy_access_t *phy,
                           uint32_t *reset);

int
plp_aperta2_dc3port_tsc_ctrl_get(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t *tsc_rstb, uint32_t *tsc_pwrdwn);

int
plp_aperta2_dc3port_tsc_ctrl_set(const plp_aperta2_phymod_phy_access_t *phy,
                     int tsc_pwr_on);

int
plp_aperta2_dc3port_link_status_get(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t start_lane, int* link);

int
plp_aperta2_dc3mac_port_enable_set(const plp_aperta2_phymod_phy_access_t *phy,
                       uint32_t enable);

int
plp_aperta2_dc3mac_port_fdr_symbol_error_window_size_set(const plp_aperta2_phymod_phy_access_t *phy,
                                             uint32_t window_size);

int
plp_aperta2_dc3mac_port_fdr_symbol_error_window_size_get(const plp_aperta2_phymod_phy_access_t *phy,
                                             uint32_t* window_size);

int
plp_aperta2_dc3mac_port_fdr_symbol_error_count_threshold_set(const plp_aperta2_phymod_phy_access_t *phy,
                       uint32_t threshold);

int
plp_aperta2_dc3mac_port_fdr_symbol_error_count_threshold_get(const plp_aperta2_phymod_phy_access_t *phy,
                                                 uint32_t* threshold);

int
plp_aperta2_dc3mac_port_mac_link_down_seq_enable_set(const plp_aperta2_phymod_phy_access_t *phy,
                                         uint32_t enable);

int
plp_aperta2_dc3mac_rx_da_timestmap_enable_get(const plp_aperta2_phymod_phy_access_t *phy,
                                  uint32_t *enable);

int
plp_aperta2_dc3mac_rx_da_timestmap_enable_set(const plp_aperta2_phymod_phy_access_t *phy,
                                  uint32_t enable);

int
plp_aperta2_dc3mac_reset_check(const plp_aperta2_phymod_phy_access_t *phy, int enable, int *reset);

int
plp_aperta2_dc3mac_txfifo_cell_cnt_get(const plp_aperta2_phymod_phy_access_t *phy, uint32* val);

int
plp_aperta2_dc3mac_mac_ctrl_set(const plp_aperta2_phymod_phy_access_t *phy, uint64_t ctrl);

int
plp_aperta2_dc3port_port_fault_link_status_set(const plp_aperta2_phymod_phy_access_t *phy, int enable);

int plp_aperta2_dc3mac_vlan_tag_set(const plp_aperta2_phymod_phy_access_t *phy, int outer_vlan_tag,
                        int inner_vlan_tag);

int plp_aperta2_dc3mac_vlan_tag_get(const plp_aperta2_phymod_phy_access_t *phy, int *outer_vlan_tag,
                        int *inner_vlan_tag);

int plp_aperta2_dc3mac_drain_cell_get(const plp_aperta2_phymod_phy_access_t *phy,
                          portmod_drain_cells_t *drain_cells);

int plp_aperta2_dc3mac_drain_cell_stop(const plp_aperta2_phymod_phy_access_t *phy,
                           const portmod_drain_cells_t *drain_cells);

int plp_aperta2_dc3mac_drain_cell_start(const plp_aperta2_phymod_phy_access_t *phy);

int plp_aperta2_dc3mac_drain_cells_rx_enable(const plp_aperta2_phymod_phy_access_t *phy, int rx_en);

int plp_aperta2_dc3mac_egress_queue_drain_rx_en(const plp_aperta2_phymod_phy_access_t *phy, int rx_en);

int plp_aperta2_dc3mac_egress_queue_drain_get(const plp_aperta2_phymod_phy_access_t *phy, uint64_t *mac_ctrl,
                                  int *rx_en);

int plp_aperta2_dc3mac_link_fault_os_set(const plp_aperta2_phymod_phy_access_t *phy, int is_remote, uint32 enable);

int plp_aperta2_dc3mac_link_fault_os_get(const plp_aperta2_phymod_phy_access_t *phy, int is_remote, uint32 *enable);

int plp_aperta2_dc3mac_reg64_read(const plp_aperta2_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data);
int plp_aperta2_dc3mac_reg64_write(const plp_aperta2_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data);
#endif /*_PORTMOD_CDMAC_H_*/
