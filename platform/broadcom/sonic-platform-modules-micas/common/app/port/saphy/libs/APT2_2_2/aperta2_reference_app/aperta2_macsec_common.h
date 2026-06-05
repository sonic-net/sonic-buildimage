/*
 * $Copyright: (c) 2023 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

/*
 * Header file for MACsec and IPsec data structures and functions
 */

#ifndef __APERTA2_MACSEC_COMMON_H__
#define __APERTA2_MACSEC_COMMON_H__

/* Includes */
#include "aperta2_common.h"

#define EGRESS                  0
#define INGRESS                 1
#define POLICY_BYPASS           1

#define MAX_VPORT               512
#define MAX_SA                  1024
#define MAX_SA_PER_VPORT        4
#define TRANSREC_INGRESS_SIZE   20
#define TRANSREC_EGRESS_SIZE    22

/* Used in Egress transform record and Ingress rule */
#define SPI                 0x40d3d4fa
#define REPLAY_WINDOW_SIZE  0x80
#define WARMBOOT_FILE_PREFIX "/tmp/warmboot"
#define WARMBOOT_MASTER_FILE "/tmp/warmbootmaster"
#define FILENAME_MAX_LENGTH     30

typedef enum {
    POLICY_BYPASS_NONE,
    POLICY_BYPASS_CONTROL_PACKET,
    POLICY_BYPASS_DATA_PACKET,
    POLICY_BYPASS_OPTION_MAX
} macsec_policy_bypass_option_t ;


int macsec_ipsec_initialize(bcm_plp_sec_phy_access_t sec_info, int bypass_enable);
int macsec_ipsec_uninitialize(bcm_plp_sec_phy_access_t sec_info);
int macsec_add_vport(bcm_plp_sec_phy_access_t sec_info);
int macsec_remove_vport(bcm_plp_sec_phy_access_t sec_info);
int ipsec_add_vport(bcm_plp_sec_phy_access_t sec_info);
int macsec_add_sa_with_transform_record(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy);
int macsec_remove_sa_with_transform_record(bcm_plp_sec_phy_access_t sec_info);
int ipsec_add_sa_with_transform_record(bcm_plp_sec_phy_access_t sec_info);
int macsec_add_rule_policy(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy);
int macsec_remove_rule_policy(bcm_plp_sec_phy_access_t sec_info);
int ipsec_add_rule_policy(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy);
int lanemap_to_portindex(unsigned int lane_map);
void* discard_const(const void * Ptr_p);
int ipsec_channel_mode_config_set(bcm_plp_sec_phy_access_t sec_info);
int ipsec_install_parser_settings(bcm_plp_sec_phy_access_t sec_info);
#endif /* __APERTA2_MACSEC_COMMON_H__ */

