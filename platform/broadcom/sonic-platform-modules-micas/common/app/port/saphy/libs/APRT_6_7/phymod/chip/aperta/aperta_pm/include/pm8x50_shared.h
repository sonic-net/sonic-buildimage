
/*
 *
 * $Id:$
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */

#ifndef _PM8X50_SHARED_H_
#define _PM8X50_SHARED_H_

#define PM8X50_LANES_PER_CORE (8)
#define MAX_PORTS_PER_PM8X50 (8)
#define PM8X50_MAX_NUM_PHYS  (1)

struct pm8x50_s{
    int phys;
    int first_phy;
    plp_aperta_phymod_ref_clk_t ref_clk;
    plp_aperta_phymod_polarity_t polarity;
    plp_aperta_phymod_lane_map_t lane_map;
    plp_aperta_phymod_firmware_load_method_t fw_load_method;
    phymod_firmware_loader_f external_fw_loader;
    plp_aperta_phymod_core_access_t int_core_access;
    plp_aperta_phymod_phy_access_t int_phy_access;
    uint8_t core_num;
    portmod_mac_soft_reset_f portmod_mac_soft_reset;
    plp_aperta_phymod_afe_pll_t afe_pll;
    int warmboot_skip_db_restore;
    uint8_t tvco;
    uint8_t ovco;
    uint8_t sys_tvco;
    uint8_t sys_ovco;

    int rescal;
};

#endif /*_PM8X50_SHARED_H_*/
