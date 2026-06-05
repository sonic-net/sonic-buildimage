
/*
 *         
 * $Id:$
 * 
 * $Copyright: (c) 2013 Broadcom Corp. All Rights Reserved.$
 *         
 *
 */

#ifndef _APERTA2_PM8X100_GEN2_SHARED_H_
#define _APERTA2_PM8X100_GEN2_SHARED_H_

#define APERTA2_PM8X100_GEN2_LANES_PER_CORE (8)
#define MAX_PORTS_PER_PM8X100_GEN2  (8)
#define APERTA2_PM8X100_GEN2_MAX_NUM_PHYS   (1)
#define PMD_INFO_DATA_STRUCTURE_SIZE     128     /* in bytes */


/*! PM Timesync Table Entry Size. */
#define APERTA2_PM8X100_GEN2_TS_ENTRY_SIZE                 3

/*! PM Tx Timesync Table Size. */
#define APERTA2_PM8X100_GEN2_TS_TX_MPP_MEM_SIZE          160

/*! PM Rx Timesync Table Size for U0. */
#define APERTA2_PM8X100_GEN2_TS_U0_RX_MPP_MEM_SIZE       160

/*! PM Rx Timesync Table Size for U1. */
#define APERTA2_PM8X100_GEN2_TS_U1_RX_MPP_MEM_SIZE        80

/*! PM Timesync Table Entry. */
typedef uint32_t pm_ts_table_entry[APERTA2_PM8X100_GEN2_TS_ENTRY_SIZE];

/*! Structure for storing PM specific 1588 table shawdow memory. */
typedef struct aperta2_pm8x100_gen2_1588_lookup_memory_s {
    /*! U0 TX_LKUP_1588_MEM_MPP0m/MPPm */
    pm_ts_table_entry tx_lkup_1588_mem_mpp0[APERTA2_PM8X100_GEN2_TS_TX_MPP_MEM_SIZE];

    /*! U0 TX_LKUP_1588_MEM_MPP1m */
    pm_ts_table_entry tx_lkup_1588_mem_mpp1[APERTA2_PM8X100_GEN2_TS_TX_MPP_MEM_SIZE];

    /*! U0 RX_LKUP_1588_MEM_MPP0m */
    pm_ts_table_entry rx_lkup_1588_mem_mpp0[APERTA2_PM8X100_GEN2_TS_U0_RX_MPP_MEM_SIZE];

    /*! U0 RX_LKUP_1588_MEM_MPP1m */
    pm_ts_table_entry rx_lkup_1588_mem_mpp1[APERTA2_PM8X100_GEN2_TS_U0_RX_MPP_MEM_SIZE];

    /*! U1 TX_LKUP_1588_MEM_MPPm */
    pm_ts_table_entry tx_lkup_1588_mem_mpp_u1[APERTA2_PM8X100_GEN2_TS_TX_MPP_MEM_SIZE];

    /*! U1 RX_LKUP_1588_MEM_MPP0m */
    pm_ts_table_entry rx_lkup_1588_mem_mpp0_u1[APERTA2_PM8X100_GEN2_TS_U1_RX_MPP_MEM_SIZE];

    /*! U1 RX_LKUP_1588_MEM_MPP1m */
    pm_ts_table_entry rx_lkup_1588_mem_mpp1_u1[APERTA2_PM8X100_GEN2_TS_U1_RX_MPP_MEM_SIZE];

} aperta2_pm8x100_gen2_1588_lookup_memory_t;

struct aperta2_pm8x100_gen2_s{
    portmod_pbmp_t phys;
    int first_phy;
    plp_aperta2_phymod_ref_clk_t ref_clk;
    plp_aperta2_phymod_polarity_t polarity;
    plp_aperta2_phymod_lane_map_t lane_map;
    plp_aperta2_phymod_lane_map_t sys_lane_map;
    plp_aperta2_phymod_firmware_load_method_t fw_load_method;
    phymod_firmware_loader_f external_fw_loader;
    plp_aperta2_phymod_core_access_t int_core_access;
    plp_aperta2_phymod_phy_access_t int_phy_access;
    uint8_t core_num;
    portmod_mac_soft_reset_f portmod_mac_soft_reset;
    int warmboot_skip_db_restore;
    uint8_t tvco;
    uint8_t sys_tvco;
    uint8_t oc1_tvco;
    uint8_t oc1_sys_tvco;
    int is_master_pm;
    void* pmd_info;
    /* The delay from CMIC to PCS in nanoseconds. */
    uint32_t pm_offset;
    aperta2_pm8x100_gen2_1588_lookup_memory_t pm_1588_lookup_shadow_memory;
};

#endif /*_APERTA2_PM8X100_GEN2_SHARED_H_*/
