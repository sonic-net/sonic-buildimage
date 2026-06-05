/*
 *
 * $Id:$
 *
 * $Copyright: (c) 2016 Broadcom Corp. All Rights Reserved.$
 *
 *
 */
#include <include/dcccport.h>
#include <include/bcm_aperta2_dc3mac_defs.h>
#include <include/pm8x100_gen2.h>
#include <include/pm8x100_gen2_shared.h>
#include <include/portmod_internal.h>
#include <include/portmod.h>
#include <include/portmod_dispatch.h>
#include <include/plp_soc_portmod.h>
#include <phymod/phymod.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_dispatch.h>
#include <phymod/phymod_diagnostics_dispatch.h>
#include <aperta2_pm_seq.h>



/* Warmboot variable defines - start */

#define aperta2_PM8x100_GEN2_WB_BUFFER_VERSION        (1)

#define aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_SET(unit, pm_info, is_core_initialized) \
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, isCoreInitialized, is_core_initialized);    

#define aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_GET(unit, pm_info, is_core_initialized) \
     plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, isCoreInitialized, &is_core_initialized);

#define aperta2_PM8x100_GEN2_IS_ACTIVE_SET(unit, pm_info, is_active)   \
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, isActive, is_active);    
#define aperta2_PM8x100_GEN2_IS_ACTIVE_GET(unit, pm_info, is_active)    \
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, isActive, &is_active);

#define aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, tvco_pll_active_lane_bitmap)\
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, tvcopllActiveLaneBitmap, tvco_pll_active_lane_bitmap);

#define aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_active_lane_bitmap)\
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, tvcopllActiveLaneBitmap, &tvco_pll_active_lane_bitmap);

#define aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, tvco_pll_adv_lane_bitmap)\
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, tvcopllAdvLaneBitmap, tvco_pll_adv_lane_bitmap);
#define aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, tvco_pll_adv_lane_bitmap)\
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, tvcopllAdvLaneBitmap, &tvco_pll_adv_lane_bitmap);

#define aperta2_PM8x100_GEN2_LANE2PORT_SET(unit, pm_info, lane, port)                  \
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, lane2portMap, port);   
#define aperta2_PM8x100_GEN2_LANE2PORT_GET(unit, pm_info, lane, port)                 \
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, lane2portMap, &port);
    
#define aperta2_PM8x100_GEN2_AN_MODE_SET(unit, pm_info, an_mode, port_index)              \
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, anMode, an_mode);   
#define aperta2_PM8x100_GEN2_AN_MODE_GET(unit, pm_info, an_mode, port_index)             \
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, anMode, &an_mode);

#define aperta2_PM8x100_GEN2_LANE2FEC_SET(unit, pm_info, lane, fec)                   \
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, lane2fecMap, fec);   
#define aperta2_PM8x100_GEN2_LANE2FEC_GET(unit, pm_info, lane, fec)                   \
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, lane2fecMap, &fec);

#define aperta2_PM8x100_GEN2_PORT_IS_PCS_BYPASSED_SET(unit, pm_info, is_bypassed, port_index)             \
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, portIsPcsBypassed, is_bypassed);   
#define aperta2_PM8x100_GEN2_PORT_IS_PCS_BYPASSED_GET(unit, pm_info, is_bypassed, port_index)             \
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, portIsPcsBypassed, &is_bypassed);

#define aperta2_PM8x100_GEN2_TS_ENABLE_PORT_COUNT_SET(unit, pm_info, ts_enable_port_count)   \
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, tsEnablePortCount, ts_enable_port_count);   
#define aperta2_PM8x100_GEN2_TS_ENABLE_PORT_COUNT_GET(unit, pm_info, ts_enable_port_count)    \
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, tsEnablePortCount, &ts_enable_port_count);

#define aperta2_PM8x100_GEN2_TIMESYNC_CONFIG_SET(unit, pm_info, timesync_config, port_index) \
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, timesyncEnable, timesync_config);   
#define aperta2_PM8x100_GEN2_TIMESYNC_CONFIG_GET(unit, pm_info, timesync_config, port_index) \
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, timesyncEnable, &timesync_config);

#define aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_SET(unit, pm_info, speed_id_table_status)   \
    plp_aperta2_plp_aperta2_update_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, speedIdTableStatus, speed_id_table_status );
#define aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_GET(unit, pm_info, speed_id_table_status)    \
    plp_aperta2_plp_aperta2_get_wb_pm_info(PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.addr, speedIdTableStatus, &speed_id_table_status);

typedef enum aperta2_pm8x100_gen2_wb_vars{
    isCoreInitialized,
    isActive,
    tvcopllActiveLaneBitmap,
    tvcopllAdvLaneBitmap,
    lane2portMap,
    anMode,
    lane2fecMap,
    tsEnablePortCount,
    timesyncEnable,
    portIsPcsBypassed,
    speedIdTableStatus
}aperta2_pm8x100_gen2_wb_vars_t;

/* Warmboot variable defines - end */

/* This macro set devad to reg_addr if it is PMD phy address. */
#define aperta2_PM8x100_GEN2_PHY_REG_SET(reg_addr) \
    do { \
        uint32_t phy_reg_addr; \
        phy_reg_addr = reg_addr & 0xffff; \
        if ((0x000 <= phy_reg_addr && phy_reg_addr <= 0x01ff) || \
            (0x1000 <= phy_reg_addr && phy_reg_addr <= 0xdfff) || \
            (0xffd0 <= phy_reg_addr && phy_reg_addr <= 0xffdf)) { \
            reg_addr |= 1 << 16; \
        } \
    } while (0)


#define aperta2_PM8x100_GEN2_MAX_NUM_PLLS (1)

#define aperta2_PM8x100_GEN2_VCO_ALL (aperta2_PM8x100_GEN2_VCO_ETH_51P5625G | aperta2_PM8x100_GEN2_VCO_ETH_53P125G)

#define aperta2_PM8x100_GEN2_VCO_BMP_EMPTY(bmp) \
        ( (bmp & aperta2_PM8x100_GEN2_VCO_ALL) == 0 )

/* FIXME, need to confirm what ILKN speeds need to be supported in pm8x100 gen2 */
/* Highest NRZ per lane speed is 28.125G, supported in ILKN mode */
#define aperta2_PM8x100_GEN2_NRZ_LANE_SPEED_MAX 28125
#define aperta2_PM8x100_GEN2_50G_PAM4_LANE_SPEED_MAX 50000

#define aperta2_PM8x100_GEN2_AN_ABILITY_TABLE_SIZE 30
#define aperta2_PM8x100_GEN2_FS_ABILITY_TABLE_SIZE 32
#define aperta2_PM8x100_GEN2_MAX_AN_ABILITY 20


/*
 * Entries of the force speed ability table; each entry specifies a
 * unique FS port speed ability and its associated VCO rate
 */
typedef struct aperta2_pm8x100_gen2_fs_ability_table_entry_s {
    uint32_t speed; /* port speed in Mbps */
    uint32_t num_lanes; /* number of lanes */
    portmod_fec_t fec_type; /* FEC type */
    portmod_vco_type_t vco; /* associated VCO rate of the ability */
} aperta2_pm8x100_gen2_fs_ability_table_entry_t;

/*
 * Entries of the autoneg ability table; each entry specifies a unique
 * AN speed ability and its associated VCO rate
 */
typedef struct aperta2_pm8x100_gen2_an_ability_table_entry_s {
    uint32_t speed; /* port speed in Mbps */
    uint32_t num_lanes; /* number of lanes */
    portmod_fec_t fec_type; /* FEC type */
    portmod_port_phy_control_autoneg_mode_t an_mode; /* autoneg mode such as cl73, bam or msa */
    portmod_vco_type_t vco; /* associated VCO rate of the ability */
} aperta2_pm8x100_gen2_an_ability_table_entry_t;

/* a comprehensive list of pm8x100 gen2 forced speed abilities and their VCO rates */
const aperta2_pm8x100_gen2_fs_ability_table_entry_t plp_aperta2_pm8x100_gen2_fs_ability_table[aperta2_PM8x100_GEN2_FS_ABILITY_TABLE_SIZE] =
{
    /* port_speed, num_lanes, fec_type, vco */
    { 10000, 1, PORTMOD_PORT_PHY_FEC_NONE,       portmodVCO51P5625G },
    { 25000, 1, PORTMOD_PORT_PHY_FEC_NONE,       portmodVCO51P5625G },
    { 25000, 1, PORTMOD_PORT_PHY_FEC_RS_FEC,     portmodVCO51P5625G },
    { 50000, 1, PORTMOD_PORT_PHY_FEC_RS_FEC,     portmodVCO51P5625G },
    { 50000, 2, PORTMOD_PORT_PHY_FEC_NONE,       portmodVCO51P5625G },
    { 50000, 2, PORTMOD_PORT_PHY_FEC_RS_FEC,     portmodVCO51P5625G },
    {100000, 2, PORTMOD_PORT_PHY_FEC_NONE,       portmodVCO51P5625G },
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_FEC,     portmodVCO51P5625G },
    {100000, 4, PORTMOD_PORT_PHY_FEC_NONE,       portmodVCO51P5625G },
    {100000, 4, PORTMOD_PORT_PHY_FEC_RS_FEC,     portmodVCO51P5625G },
    {200000, 4, PORTMOD_PORT_PHY_FEC_NONE,       portmodVCO51P5625G },
    { 50000, 1, PORTMOD_PORT_PHY_FEC_RS_544,     portmodVCO53P125G },
    { 50000, 1, PORTMOD_PORT_PHY_FEC_RS_272,     portmodVCO53P125G },
    { 50000, 2, PORTMOD_PORT_PHY_FEC_RS_544,     portmodVCO53P125G },
    {100000, 1, PORTMOD_PORT_PHY_FEC_RS_544,     portmodVCO53P125G },
    {100000, 1, PORTMOD_PORT_PHY_FEC_RS_544_2XN, portmodVCO53P125G },
    {100000, 1, PORTMOD_PORT_PHY_FEC_RS_272,     portmodVCO53P125G },
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_544,     portmodVCO53P125G },
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_272,     portmodVCO53P125G },
    {200000, 2, PORTMOD_PORT_PHY_FEC_RS_544,     portmodVCO53P125G },
    {200000, 2, PORTMOD_PORT_PHY_FEC_RS_544_2XN, portmodVCO53P125G },
    {200000, 2, PORTMOD_PORT_PHY_FEC_RS_272,     portmodVCO53P125G },
    {200000, 2, PORTMOD_PORT_PHY_FEC_RS_272_2XN, portmodVCO53P125G },
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_544,     portmodVCO53P125G },
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_544_2XN, portmodVCO53P125G },
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_272,     portmodVCO53P125G },
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_272_2XN, portmodVCO53P125G },
    {400000, 4, PORTMOD_PORT_PHY_FEC_RS_544_2XN, portmodVCO53P125G },
    {400000, 4, PORTMOD_PORT_PHY_FEC_RS_272_2XN, portmodVCO53P125G },
    {400000, 8, PORTMOD_PORT_PHY_FEC_RS_544_2XN, portmodVCO53P125G },
    {400000, 8, PORTMOD_PORT_PHY_FEC_RS_272_2XN, portmodVCO53P125G },
    {800000, 8, PORTMOD_PORT_PHY_FEC_RS_544_2XN, portmodVCO53P125G }
};


/* a comprehensive list of pm8x100 autoneg abilities and their VCO rates */
const aperta2_pm8x100_gen2_an_ability_table_entry_t plp_aperta2_pm8x100_gen2_an_ability_table[aperta2_PM8x100_GEN2_AN_ABILITY_TABLE_SIZE] =
{
    /* port_speed, num_lanes, fec_type, autoneg_mode, vco */
    {25000,  1, PORTMOD_PORT_PHY_FEC_NONE,       PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO51P5625G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO51P5625G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_NONE,       PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO51P5625G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO51P5625G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_NONE,       PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO51P5625G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO51P5625G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO51P5625G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_NONE,       PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO51P5625G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_RS_FEC,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO51P5625G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_NONE,       PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO51P5625G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_RS_FEC,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO51P5625G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_NONE,       PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO51P5625G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_FEC,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO51P5625G},
    {100000, 4, PORTMOD_PORT_PHY_FEC_NONE,       PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO51P5625G},
    {100000, 4, PORTMOD_PORT_PHY_FEC_RS_FEC,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO51P5625G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_NONE,       PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO51P5625G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_RS_544,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO53P125G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_RS_272,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO53P125G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_RS_544,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO53P125G},
    {100000, 1, PORTMOD_PORT_PHY_FEC_RS_544_2XN, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO53P125G},
    {100000, 1, PORTMOD_PORT_PHY_FEC_RS_544,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO53P125G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_544,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO53P125G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_272,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO53P125G},
    {200000, 2, PORTMOD_PORT_PHY_FEC_RS_544_2XN, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO53P125G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_544_2XN, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO53P125G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_544,     PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO53P125G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_272_2XN, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO53P125G},
    {400000, 4, PORTMOD_PORT_PHY_FEC_RS_544_2XN, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO53P125G},
    {400000, 8, PORTMOD_PORT_PHY_FEC_RS_544_2XN, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO53P125G},
    {800000, 8, PORTMOD_PORT_PHY_FEC_RS_544_2XN, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO53P125G}
};


/*
 * given the port speed, number of lanes, and FEC type, return the VCO specified in
 * plp_aperta2_pm8x100_gen2_fs_ability_table in the associated entry; VCO should be portmodVCOInvalid
 * if the ability is invalid
 */
STATIC
int _plp_aperta2_pm8x100_gen2_fs_ability_table_vco_get(uint32_t speed, uint32_t num_lanes, portmod_fec_t fec_type,
                                           portmod_vco_type_t* vco)
{
    int i;

    *vco = portmodVCOInvalid;
    for (i = 0; i < aperta2_PM8x100_GEN2_FS_ABILITY_TABLE_SIZE; i++) {
        if (plp_aperta2_pm8x100_gen2_fs_ability_table[i].speed == speed &&
            plp_aperta2_pm8x100_gen2_fs_ability_table[i].num_lanes == num_lanes &&
            plp_aperta2_pm8x100_gen2_fs_ability_table[i].fec_type == fec_type)
        {
            *vco = plp_aperta2_pm8x100_gen2_fs_ability_table[i].vco;
            break;
        }
    }

    return PHYMOD_E_NONE;
}

/*
 * given the port speed, number of lanes, FEC type, and autoneg mode, return the VCO specified in
 * plp_aperta2_pm8x100_gen2_an_ability_table; VCO should be portmodVCOInvalid if the ability is invalid.
 * this function is temporarily masked out because there is no caller. advert_ability_get() will
 * need to call this function to verify the VCO requirement
 */
STATIC
int _plp_aperta2_pm8x100_gen2_an_ability_table_vco_get(uint32_t speed, uint32_t num_lanes, portmod_fec_t fec_type,
                                           portmod_port_phy_control_autoneg_mode_t an_mode,
                                           portmod_vco_type_t* vco)
{
    int i;

    *vco = portmodVCOInvalid;
    for (i = 0; i < aperta2_PM8x100_GEN2_AN_ABILITY_TABLE_SIZE; i++) {
        if (plp_aperta2_pm8x100_gen2_an_ability_table[i].speed == speed &&
            plp_aperta2_pm8x100_gen2_an_ability_table[i].num_lanes == num_lanes &&
            plp_aperta2_pm8x100_gen2_an_ability_table[i].fec_type == fec_type &&
            plp_aperta2_pm8x100_gen2_an_ability_table[i].an_mode == an_mode)
        {
            *vco = plp_aperta2_pm8x100_gen2_an_ability_table[i].vco;
            break;
        }
    }

    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta2_pm8x100_gen2_port_index_get(int unit, int port, pm_info_t pm_info,
                                 int *first_index, uint32_t *bitmap)
{
   int i, tmp_port = 0;

   *first_index = -1;
   *bitmap = 0;

   for( i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++){
       aperta2_PM8x100_GEN2_LANE2PORT_GET(unit, pm_info, i, tmp_port);
       if(tmp_port == port){
           *first_index = (*first_index == -1 ? i : *first_index);
           SHR_BITSET(*bitmap, i);
       }
   }

   if(*first_index == -1) {
       APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_INTERNAL,
              (SOC_MSG("port was not found in internal DB %d"), port));
   }


    return PHYMOD_E_NONE;
}

STATIC int
_plp_aperta2_pm8x100_gen2_phy_access_get(int unit,
                             int port,
                             pm_info_t pm_info,
                             plp_aperta2_phymod_phy_access_t *phy_acc)
{
    PHYMOD_MEMCPY(phy_acc, &PM_8x100_GEN2_INFO(pm_info)->int_phy_access, sizeof(plp_aperta2_phymod_phy_access_t)); 
    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta2_pm8x100_gen2_port_interrupt_all_enable_set(int unit, int port, int enable)
{
    int i = 0;
    portmod_intr_type_t intrs[] = {
                                    portmodIntrTypeTxPfcFifoOverflow,
                                    portmodIntrTypeRxPfcFifoOverflow,
                                    portmodIntrTypeFdrInterrupt,
                                    portmodIntrTypeTxPktUnderflow,
                                    portmodIntrTypeTxPktOverflow,
                                    portmodIntrTypeTxCdcSingleBitErr,
                                    portmodIntrTypeTxCdcDoubleBitErr,
                                    portmodIntrTypeMibMemSingleBitErr,
                                    portmodIntrTypeMibMemDoubleBitErr,
                                    portmodIntrTypeMibMemMultipleBitErr
                                  };
    

    for (i = 0; i < sizeof(intrs)/sizeof(intrs[0]); i++) {
        PHYMOD_IF_ERR_RETURN(
        plp_aperta2_portmod_port_interrupt_enable_set(unit, port, intrs[i], enable));
    }


    return PHYMOD_E_NONE;
}

/*
 * Get PM active Ethernet port lanes bitmap.
 *
 *    This API doesn't consider the ports whose speeds
 *    are not configured.
 *
 * Inputs:
 *     unit:          unit number;
 *     pm_info:       pm_info data structure;
 *
 * Output:
 *     eth_active_lanes:   active ethernet lane bitmap
 */

STATIC
int _plp_aperta2_pm8x100_gen2_pm_active_eth_lanes_get(int unit,
                                          pm_info_t pm_info,
                                          uint8_t * eth_active_lanes)
{
    int tmp_port, i, is_pcs_bypassed = 0;
    int tvco_pll_active_lane_bitmap;
    int tvco_pll_adv_lane_bitmap;
    int all_active_lane_bitmap;

    

    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_active_lane_bitmap);
    aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, tvco_pll_adv_lane_bitmap);

    all_active_lane_bitmap = (tvco_pll_active_lane_bitmap | tvco_pll_adv_lane_bitmap);

    *eth_active_lanes = 0;
    for(i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
        /* plp_aperta2_Exclude the ports whose speeds are not configured */
        if (all_active_lane_bitmap & (1 << i)) {
            aperta2_PM8x100_GEN2_LANE2PORT_GET(unit, pm_info, i, tmp_port);
            if (tmp_port >= 0) {
                aperta2_PM8x100_GEN2_PORT_IS_PCS_BYPASSED_GET(unit, pm_info, is_pcs_bypassed, i);
                if (!is_pcs_bypassed) {
                    *eth_active_lanes |= (1 << i);
                }
            }
        }
    }

    return PHYMOD_E_NONE;
}

/*Get whether the inerface type is supported by PM */
int plp_aperta2_pm8x100_gen2_pm_interface_type_is_supported(int unit,
                                                soc_port_if_t interface,
                                                int* is_supported)
{

    switch(interface){
        default:
            *is_supported = TRUE;
    }

    return (PHYMOD_E_NONE);
}


STATIC
int
plp_aperta2_pm8x100_gen2_default_bus_write(void* user_acc, uint32_t core_addr, uint32_t reg_addr,
                               uint32_t val)
{
    return (PHYMOD_E_UNAVAIL);
}

STATIC
int
plp_aperta2_pm8x100_gen2_default_bus_read(void* user_acc, uint32_t core_addr, uint32_t reg_addr,
                              uint32_t *val)
{
    return (PHYMOD_E_UNAVAIL);
}

/*
 * Function:
 *      plp_aperta2_portmod_aperta2_pm8x100_gen2_wb_upgrade_func
 * Purpose:
 *      This function will take care of the warmboot variable manipulation
 *      in case of upgrade case, when variable definition got change or
 *      unspoorted varaible from previous version.
 * Parameters:
 *      unit              -(IN) Device unit number .
 *      arg               -(IN) Generic pointer for a specific module to be used
 *      recovered_version -(IN) Warmboot version of the existing data.
 *      new_version       -(IN) Warmboot version of new data.
 * Returns:
 *      Status
 */

int plp_aperta2_portmod_aperta2_pm8x100_gen2_wb_upgrade_func(int unit, void *arg, int recovered_version,
                                         int new_version)
{
    return PHYMOD_E_NONE;

}
#ifdef APERTA2_PM8x100_WB_SUPPORT
int aperta2_pm8x100_gen2_pm_wb_debug_log(int unit, pm_info_t pm_info)
{
    int i, rv = 0;
    int tmp_port;
    int ts_enable_port_count, speed_id_table_status;
    uint8_t tvco_pll_active_lane_bm;
    uint8_t tvco_pll_adv_lane_bm;
    uint32_t is_core_initialized, is_active, is_bypassed, timesync_enable;
    plp_aperta2_phymod_an_mode_type_t an_mode = 0;
    portmod_fec_t fec;

    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$BEGIN:portmod_aperta2_pm8x100_gen2\n")));

    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$BEGIN:portmod_aperta2_pm8x100_gen2:{wb_buffer_index}\n")));
    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "{%d}\n"), pm_info->wb_buffer_id));
    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$END:portmod_aperta2_pm8x100_gen2:{wb_buffer_index}\n")));

    rv = aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_GET(unit, pm_info, is_core_initialized);
    if (PHYMOD_E_NONE == rv) {
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$BEGIN:portmod_aperta2_pm8x100_gen2:{is_initialized}\n")));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "{%d}\n"), is_core_initialized));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$END:portmod_aperta2_pm8x100_gen2:{is_initialized}\n")));
    }

    rv = aperta2_PM8x100_GEN2_IS_ACTIVE_GET(unit, pm_info, is_active);
    if (PHYMOD_E_NONE == rv) {
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$BEGIN:portmod_aperta2_pm8x100_gen2:{is_active}\n")));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "{%d}\n"), is_active));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$END:portmod_aperta2_pm8x100_gen2:{is_active}\n")));
    }

    rv = aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_active_lane_bm);
    if (PHYMOD_E_NONE == rv) {
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$BEGIN:portmod_aperta2_pm8x100_gen2:{tvco_pll_active_lane_bitmap}\n")));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "{0x%x}\n"), tvco_pll_active_lane_bm));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$END:portmod_aperta2_pm8x100_gen2:{tvco_pll_active_lane_bitmap}\n")));
    }

    rv = aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, tvco_pll_adv_lane_bm);
    if (PHYMOD_E_NONE == rv) {
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$BEGIN:portmod_aperta2_pm8x100_gen2:{tvco_pll_adv_lane_bitmap}\n")));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "{0x%x}\n"), tvco_pll_adv_lane_bm));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$END:portmod_aperta2_pm8x100_gen2:{tvco_pll_adv_lane_bitmap}\n")));
    }

    rv = aperta2_PM8x100_GEN2_TS_ENABLE_PORT_COUNT_GET(unit, pm_info, ts_enable_port_count);
    if (PHYMOD_E_NONE == rv) {
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$BEGIN:portmod_aperta2_pm8x100_gen2:{ts_enable_port_count}\n")));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "{%d}\n"), ts_enable_port_count));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$END:portmod_aperta2_pm8x100_gen2:{ts_enable_port_count}\n")));
    }

    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$BEGIN:portmod_aperta2_pm8x100_gen2:{lane,lane2port_map}\n")));
    for (i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {

        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit, "\n")));
        rv = aperta2_PM8x100_GEN2_LANE2PORT_GET(unit, pm_info, i, tmp_port);
        if (PHYMOD_E_NONE == rv) {
            LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                            "{%d,%d}\n"), i, tmp_port));
        }
    }
    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$END:portmod_aperta2_pm8x100_gen2:{lane,lane2port_map}\n")));

    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$BEGIN:portmod_aperta2_pm8x100_gen2:{port,an_mode}\n")));
    for (i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
        rv = aperta2_PM8x100_GEN2_AN_MODE_GET(unit, pm_info, an_mode, i);
        if (PHYMOD_E_NONE == rv) {
            LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                            "{%d,%d}\n"), i, an_mode));
        }
    }
    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$END:portmod_aperta2_pm8x100_gen2:{port,an_mode}\n")));

    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$BEGIN:portmod_aperta2_pm8x100_gen2:{lane,lane2fec_map}\n")));
    for (i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
        rv = aperta2_PM8x100_GEN2_LANE2FEC_GET(unit, pm_info, i, fec);
        if (PHYMOD_E_NONE == rv) {
            LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                            "{%d,%d}\n"), i, fec));
        }
    }
    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$END:portmod_aperta2_pm8x100_gen2:{lane,lane2fec_map}\n")));

    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$BEGIN:portmod_aperta2_pm8x100_gen2:{port,port_is_pcs_bypassed}\n")));
    for (i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
        rv = aperta2_PM8x100_GEN2_PORT_IS_PCS_BYPASSED_GET(unit, pm_info, is_bypassed, i);
        if (PHYMOD_E_NONE == rv) {
            LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                            "{%d,%d}\n"), i, is_bypassed));
        }
    }
    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$END:portmod_aperta2_pm8x100_gen2:{port,port_is_pcs_bypassed}\n")));

    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$BEGIN:portmod_aperta2_pm8x100_gen2:{port,timesync_enable}\n")));
    for (i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
        rv = aperta2_PM8x100_GEN2_TIMESYNC_CONFIG_GET(unit, pm_info, timesync_enable, i);
        if (PHYMOD_E_NONE == rv) {
            LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                            "{%d,%d}\n"), i, timesync_enable));
        }
    }
    LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                    "S$END:portmod_aperta2_pm8x100_gen2:{port,timesync_enable}\n")));

    rv = aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_GET(unit, pm_info, speed_id_table_status);
    if (PHYMOD_E_NONE == rv) {
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$BEGIN:portmod_aperta2_pm8x100_gen2:{speed_id_table_status}\n")));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "{%d}\n"), speed_id_table_status));
        LOG_VERBOSE(BSL_LS_SHARED_SCACHE, (BSL_META_U(unit,
                        "S$END:portmod_aperta2_pm8x100_gen2:{speed_id_table_status}\n")));
    }

    return PHYMOD_E_NONE;
}

/*
 * Initialize the buffer to support warmboot
 * The state of warmboot is store in the variables like
 * isInitialized, isActive, portIsPcsBypassed, ports.. etc.,
 * All of these variables need to be added to warmboot
 * any variables added to save the state of warmboot should be
 * included here.
 */
STATIC
int aperta2_pm8x100_gen2_wb_buffer_init(int unit, int wb_buffer_index, pm_info_t pm_info)
{
    /* Declare the common variables needed for warmboot */
    WB_ENGINE_INIT_TABLES_DEFS;
    int wb_var_id, rv = PHYMOD_E_NONE;
    int buffer_id = wb_buffer_index; /*required by SOC_WB_ENGINE_ADD_ Macros*/
    

    COMPILER_REFERENCE(buffer_is_dynamic);

    SOC_WB_ENGINE_ADD_BUFF(SOC_WB_ENGINE_PORTMOD, wb_buffer_index, "aperta2_pm8x100_gen2",
                           plp_aperta2_portmod_aperta2_pm8x100_gen2_wb_upgrade_func, pm_info,
                           aperta2_PM8x100_GEN2_WB_BUFFER_VERSION, 1,
                           SOC_WB_ENGINE_PRE_RELEASE);
    PHYMOD_IF_ERR_RETURN(rv);

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id,
                         "is_core_initialized", wb_buffer_index, sizeof(int),
                          NULL, MAX_PORTS_PER_PM8X100_GEN2, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[isCoreInitialized] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "is_active",
                          wb_buffer_index, sizeof(uint32_t), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[isActive] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "tvco_pll_active_lane_bitmap",
                          wb_buffer_index, sizeof(uint8_t), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[tvcopllActiveLaneBitmap] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "tvco_pll_adv_lane_bitmap",
                          wb_buffer_index, sizeof(uint8_t), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[tvcopllAdvLaneBitmap] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "lane2portMap",
                          wb_buffer_index, sizeof(int), NULL, MAX_PORTS_PER_PM8X100_GEN2, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[lane2portMap] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "an_mode",
                          wb_buffer_index, sizeof(int), NULL, MAX_PORTS_PER_PM8X100_GEN2, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[anMode] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "lane2fecMap",
                          wb_buffer_index, sizeof(int), NULL, MAX_PORTS_PER_PM8X100_GEN2, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[lane2fecMap] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));

    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "ts_enable_port_count",
                          wb_buffer_index, sizeof(int), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[tsEnablePortCount] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "timesync_enable",
                           wb_buffer_index, sizeof(uint32_t), NULL, MAX_PORTS_PER_PM8X100_GEN2, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[timesyncEnable] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "port_is_pcs_bypassed",
                          wb_buffer_index, sizeof(uint32_t), NULL, MAX_PORTS_PER_PM8X100_GEN2, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[portIsPcsBypassed] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "speed_id_table_status",
                          wb_buffer_index, sizeof(int), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[speedIdTableStatus] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(soc_wb_engine_init_buffer(unit, SOC_WB_ENGINE_PORTMOD,
                                               wb_buffer_index, FALSE));


    return PHYMOD_E_NONE;
}
#endif
plp_aperta2_phymod_bus_t plp_aperta2_pm8x100_gen2_default_bus = {
    "aperta2_PM8x100_gen2 Bus",
    plp_aperta2_pm8x100_gen2_default_bus_read,
    plp_aperta2_pm8x100_gen2_default_bus_write,
    NULL,
    NULL,
    NULL,
    PHYMOD_BUS_CAP_WR_MODIFY | PHYMOD_BUS_CAP_LANE_CTRL
};

STATIC
int _plp_aperta2_pm8x100_gen2_pll_to_vco_get(uint32_t pll, portmod_vco_type_t* vco)
{
    int rv = PHYMOD_E_NONE;

    switch (pll) {
        case 0xA5 :
            *vco = portmodVCO51P5625G;
            break;
        case 0xAA:
            *vco = portmodVCO53P125G;
            break;
        default:
            *vco = portmodVCOInvalid;
            break;
    }

    return rv;
}

int plp_aperta2_pm8x100_gen2_port_mac_link_get(int unit, int port, pm_info_t pm_info, int* link)
{
    uint32_t bitmap;
    int port_index = 0;
    plp_aperta2_phymod_phy_access_t phy_acc;
    
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_MEMSET(&phy_acc, 0 , sizeof(phy_acc));

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, &bitmap));

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_dc3port_link_status_get(&phy_acc, port_index, link));


    return PHYMOD_E_NONE;
}


/*Add new pm.*/
int plp_aperta2_pm8x100_gen2_pm_init(int unit,
                         const portmod_pm_create_info_internal_t* pm_add_info,
                         int wb_buffer_index,
                         pm_info_t pm_info)
{
    const portmod_pm8x100_gen2_create_info_t *info =
                &pm_add_info->pm_specific_info.pm8x100_gen2;
    pm8x100_gen2_t aperta2_pm8x100_gen2_data = NULL;
    int i;
    int pm_is_active, ts_enable_port_count, speed_id_table_status;
    int is_core_initialized, fec, invalid_port;
    uint8_t tvco_pll_adv_lane_bitmap, tvco_pll_active_lane_bitmap;

    pm_info->type = pm_add_info->type;
    pm_info->unit = unit;
    pm_info->wb_buffer_id = wb_buffer_index;

    /* PM8x100 gen2 specific info */
    aperta2_pm8x100_gen2_data = PHYMOD_MALLOC(sizeof(struct aperta2_pm8x100_gen2_s), "specific_db");
    PHYMOD_NULL_CHECK(aperta2_pm8x100_gen2_data);
    pm_info->pm_data.aperta2_pm8x100_gen2_db = aperta2_pm8x100_gen2_data;

    aperta2_pm8x100_gen2_data->int_core_access.type = phymodDispatchTypeCount;
        PM_8x100_GEN2_INFO(pm_info)->first_phy = -1;
    aperta2_pm8x100_gen2_data->warmboot_skip_db_restore = TRUE;
    aperta2_pm8x100_gen2_data->portmod_mac_soft_reset = info->portmod_mac_soft_reset;

    /* init intertnal SerDes core access */
    plp_aperta2_phymod_core_access_t_init(&aperta2_pm8x100_gen2_data->int_core_access);

    PHYMOD_MEMCPY(&aperta2_pm8x100_gen2_data->polarity, &info->polarity,
               sizeof(plp_aperta2_phymod_polarity_t));
    PHYMOD_MEMCPY(&(aperta2_pm8x100_gen2_data->int_core_access.access), &info->access.access,
                sizeof(plp_aperta2_phymod_access_t));
    PHYMOD_MEMCPY(&(aperta2_pm8x100_gen2_data->int_phy_access), &(aperta2_pm8x100_gen2_data->int_core_access),
                sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_MEMCPY(&aperta2_pm8x100_gen2_data->lane_map, &info->lane_map,
                sizeof(aperta2_pm8x100_gen2_data->lane_map));
    aperta2_pm8x100_gen2_data->ref_clk = info->ref_clk;
    aperta2_pm8x100_gen2_data->fw_load_method = info->fw_load_method;
    aperta2_pm8x100_gen2_data->external_fw_loader = info->external_fw_loader;
    aperta2_pm8x100_gen2_data->tvco = info->tvco;
    aperta2_pm8x100_gen2_data->pm_offset = info->pm_offset;


    if (info->access.access.bus == NULL) {
        /* if null - use default */
        aperta2_pm8x100_gen2_data->int_core_access.access.bus = &plp_aperta2_pm8x100_gen2_default_bus;
    } else {
        /* check null for mem_read/mem_write */
    }

#ifdef APERTA2_PM8x100_WB_SUPPORT
    /*init wb buffer
    PHYMOD_IF_ERR_RETURN(aperta2_pm8x100_gen2_wb_buffer_init(unit, wb_buffer_index,  pm_info));*/
#endif


#ifdef APERTA2_PM8x100_WB_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        /* For warmboot, probe Serdes driver type for active PMs. */
        is_core_initialized = 0;

        rv = aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_GET(unit, pm_info, is_core_initialized);
        PHYMOD_IF_ERR_RETURN(rv);

        /* print scache information for debugging */
        aperta2_pm8x100_gen2_pm_wb_debug_log(unit, pm_info);

        if (is_core_initialized) {
            PHYMOD_IF_ERR_RETURN(portmod_common_serdes_probe(aperta2_pm8x100_gen2_serdes_list, &aperta2_pm8x100_gen2_data->int_core_access, &probe));
            if (!probe) {
                PHYMOD_DEBUG_ERROR(
                      (BSL_META_U(unit, "ERROR: serdes probe failed type=%d\n"), aperta2_pm8x100_gen2_data->int_core_access.type));
            }

            /* Get VCO rates from HW */
            PHYMOD_MEMCPY(&phy_access, &(aperta2_pm8x100_gen2_data->int_core_access),
                       sizeof(plp_aperta2_phymod_phy_access_t));

            /* 1. Get TVCO rate :
             *    a. If TVCO is not free, get TVCO rate from HW.
             *    b. If TVCO is free, TVCO is powerdown.
             */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_pll_powerdown_get(&phy_access, phy_access.access.tvco_pll_index, &is_pwrdn));
            if (is_pwrdn)
            {
                aperta2_pm8x100_gen2_data->tvco = portmodVCOInvalid;
            }
            else
            {
                phy_access.access.pll_idx = phy_access.access.tvco_pll_index;
                PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_pll_multiplier_get(&phy_access, &pll_div));
                PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_pll_to_vco_get(pll_div, &vco));
                aperta2_pm8x100_gen2_data->tvco = vco;
            }
        }
    } else {
#endif
        /* For coldboot, initialized warmboot variables for the PM. */
        is_core_initialized = 0;
        aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_SET(unit, pm_info,
                                            is_core_initialized);
        pm_is_active = 0;
        aperta2_PM8x100_GEN2_IS_ACTIVE_SET(unit, pm_info, pm_is_active);

        tvco_pll_adv_lane_bitmap = 0;
        aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, tvco_pll_adv_lane_bitmap);

        tvco_pll_active_lane_bitmap = 0;
        aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, tvco_pll_active_lane_bitmap);

        fec = 0;
        for (i = 0; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
            aperta2_PM8x100_GEN2_LANE2FEC_SET(unit, pm_info, i, fec);
        }

        invalid_port = -1;
        for (i = 0; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
            aperta2_PM8x100_GEN2_LANE2PORT_SET(unit, pm_info, i, invalid_port);
        }

        ts_enable_port_count = 0;
        aperta2_PM8x100_GEN2_TS_ENABLE_PORT_COUNT_SET(unit, pm_info, ts_enable_port_count);

        speed_id_table_status = 0;
        aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_SET(unit, pm_info, speed_id_table_status);
#ifdef APERTA2_PM8x100_WB_SUPPORT
    }
#endif

    return PHYMOD_E_NONE;

}

/*Release PM resources*/
int plp_aperta2_pm8x100_gen2_pm_destroy(int unit, pm_info_t pm_info)
{
    

    if (pm_info->pm_data.aperta2_pm8x100_gen2_db != NULL) {
        PHYMOD_FREE(pm_info->pm_data.aperta2_pm8x100_gen2_db);
        pm_info->pm_data.aperta2_pm8x100_gen2_db = NULL;
    }

    return PHYMOD_E_NONE;
}

/*
 * Get the VCO rates from given PCS bypassed port speed
 */
/* FIXME, to be updated with the supported PCS bypass port speed */
int _plp_aperta2_pm8x100_gen2_pcs_bypassed_vco_get(int speed, portmod_vco_type_t* vco)
{
    

    switch (speed) {
        default:
            *vco = portmodVCOInvalid;
            break;
    }

    return PHYMOD_E_NONE;
}

/* This function will check the speed config is valid or not, and return required vco for valid input */
/* The supported speed config in this function is based on aperta2_PM8x100_gen2 Portmod Spec */
STATIC
int _plp_aperta2_pm8x100_gen2_port_speed_config_to_vco_get(const portmod_speed_config_t* speed_config,
                                               int speed_for_pcs_bypass_port,
                                               portmod_vco_type_t* vco)
{
    

    if (!speed_for_pcs_bypass_port) {
        PHYMOD_IF_ERR_RETURN(
                _plp_aperta2_pm8x100_gen2_fs_ability_table_vco_get(speed_config->speed,
                                                      speed_config->num_lane,
                                                      speed_config->fec,
                                                      vco));
    } else {
        PHYMOD_IF_ERR_RETURN(
                _plp_aperta2_pm8x100_gen2_pcs_bypassed_vco_get(speed_config->speed, vco));
    }
    /*
     * when *vco == portmodVCOInvalid, it means the entered combination of
     * port speed, number of lanes, and FEC type is not supported.
     */
    if (*vco == portmodVCOInvalid) {
        return PHYMOD_E_CONFIG;
    }


    return PHYMOD_E_NONE;
}


STATIC
int _plp_aperta2_pm8x100_gen2_lanebitmap_set(int starting_lane, int num_lane, uint8_t *bitmap)
{
    int i;

    for (i = 0; i < num_lane; i++) {
        *bitmap |= 1 << (starting_lane + i);
    }

    return PHYMOD_E_NONE;
}

/*
 * This function is to validate the FEC settings on each PM.
 * For each MPP, we can not support both RS544 and RS272.
 */
STATIC
int _plp_aperta2_pm8x100_gen2_fec_validate(int unit,
                               uint8_t rs544_bitmap,
                               uint8_t rs272_bitmap,
                               uint8_t *affected_lane_bitmap)
{
    uint8_t rs544_mpp0_01, rs544_mpp0_23, rs544_mpp1_01, rs544_mpp1_23;
    uint8_t rs272_mpp0_01, rs272_mpp0_23, rs272_mpp1_01, rs272_mpp1_23;

    

    /* get the proper fec bitmap properly */
    rs544_mpp0_01 = rs544_bitmap & 0x3;
    rs544_mpp0_23 = rs544_bitmap & 0xc;
    rs544_mpp1_01 = rs544_bitmap & 0x30;
    rs544_mpp1_23 = rs544_bitmap & 0xc0;

    rs272_mpp0_01 = rs272_bitmap & 0x3;
    rs272_mpp0_23 = rs272_bitmap & 0xc;
    rs272_mpp1_01 = rs272_bitmap & 0x30;
    rs272_mpp1_23 = rs272_bitmap & 0xc0;

    *affected_lane_bitmap = 0;
    if (rs544_mpp0_01 && rs272_mpp0_01) {
        *affected_lane_bitmap = rs544_mpp0_01 | rs272_mpp0_01;
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("Can not accommodate FEC settings on MPP0_01.")));
    } else if (rs544_mpp0_23 && rs272_mpp0_23) {
        *affected_lane_bitmap = rs544_mpp0_23 | rs272_mpp0_23;
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("Can not accommodate FEC settings on MPP0_23.")));
    } else if (rs544_mpp1_01 && rs272_mpp1_01) {
        *affected_lane_bitmap = rs544_mpp1_01 | rs272_mpp1_01;
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("Can not accommodate FEC settings on MPP1_01.")));
    } else if (rs544_mpp1_23 && rs272_mpp1_23) {
        *affected_lane_bitmap = rs544_mpp1_23 | rs272_mpp1_23;
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("Can not accommodate FEC settings on MPP1_23.")));
    }


    return PHYMOD_E_NONE;
}

/*
 * This function will return the lane_mask for a given
 * port index and number of lanes of the port.
 */
static void
_plp_aperta2_pm8x100_gen2_lane_mask_get(int port_index, int num_lane, uint32_t *lane_mask)
{
    int i;

    *lane_mask = 0;

    for (i = 0; i < num_lane; i++) {
        *lane_mask |= 1 << (port_index + i);
    }

}

static int
_plp_aperta2_pm8x100_gen2_pm_port_lane_map_validate(int unit, uint32_t lane_map)
{
    int valid = 0;
    int idx;
    uint32_t lmap_lane[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                            0x3, 0xc, 0x30, 0xc0,
                            0xf, 0xf0, 0xff,0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000,0x300, 0xc00, 0x3000, 0xc000, 0xf00, 0xf000, 0xff00};

    /* Check for lane map. */
    for (idx = 0; idx < APERTA2_COUNTOF(lmap_lane); idx++) {
        if (lane_map == lmap_lane[idx]) {
            valid = 1;
            break;
        }
    }

    if (!valid) {
        PHYMOD_DEBUG_ERROR(("Invalid lanemap\n"));
        return (PHYMOD_E_PARAM);
    }

    return PHYMOD_E_NONE;
}

/*Get the suggested VCO values based on the speed config list*/
int plp_aperta2_pm8x100_gen2_pm_vcos_get(int unit,
                             portmod_dispatch_type_t pm_type,
                             uint32_t flags,
                             portmod_pm_vco_setting_t* vco_select)
{
    portmod_vco_type_t current_vco = portmodVCOInvalid, req_vco;
    portmod_speed_config_t *current_speed_config = NULL;
    uint8_t rs544_bm = 0, rs272_bm = 0, affected_bm = 0;
    int i, rv = 0, is_pcs_bypass;
    uint32_t lane_mask = 0;

    

    if(vco_select == NULL){
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_MEMORY,
                          (SOC_MSG("vco_select NULL paramaeter")));
    }

    for (i = 0; i < vco_select->num_speeds; i++) {
        current_speed_config = &vco_select->speed_config_list[i];

        if (vco_select->port_starting_lane_list != NULL) {
            _plp_aperta2_pm8x100_gen2_lane_mask_get(vco_select->port_starting_lane_list[i],
                                        current_speed_config->num_lane, &lane_mask);

            if (vco_select->port_starting_lane_list != NULL) {
                /* 1. Check lane_map. */
                rv = _plp_aperta2_pm8x100_gen2_pm_port_lane_map_validate(unit, lane_mask);
                if (rv != PHYMOD_E_NONE) {
                    APERTA2_SOC_EXIT_WITH_ERR(rv,
                              (SOC_MSG("lane_map validate failed\n")));
                }
            }
        }
        /* 2. Validate force speed ability. */
        is_pcs_bypass = 0;
        req_vco = portmodVCOInvalid;
        rv = _plp_aperta2_pm8x100_gen2_port_speed_config_to_vco_get(current_speed_config,
                                                        is_pcs_bypass,
                                                        &req_vco);
        if (rv != PHYMOD_E_NONE) {
            APERTA2_SOC_EXIT_WITH_ERR(rv,
                      (SOC_MSG("Speed config is not supported")));
        }

        if (current_vco == portmodVCOInvalid) {
            current_vco = req_vco;
        } else if (req_vco != current_vco) {
            APERTA2_SOC_EXIT_WITH_ERR(rv,
                      (SOC_MSG("request vco is unavailable\n")));
        }

        if ((current_speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_272) ||
            (current_speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_272_2XN)) {
            rs272_bm |= lane_mask;
        } else if ((current_speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_544) ||
                   (current_speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_544_2XN)) {
            rs544_bm |= lane_mask;
        }
    }

    /* Validate FEC settings for RS544 and RS272. */
    PHYMOD_IF_ERR_RETURN(
        _plp_aperta2_pm8x100_gen2_fec_validate(unit, rs544_bm, rs272_bm, &affected_bm));

    /* Set output. */
    vco_select->tvco = current_vco;
    vco_select->is_tvco_new = (current_vco == portmodVCOInvalid) ? 0 : 1;


    return PHYMOD_E_NONE;
}

/*Enable port macro.*/
/* This function contains 3 parts:
 * 1. Bring Serdes out of hard reset.
 * 2. Bring 2 DC3MACs out of hard reset.
 */
int plp_aperta2_pm8x100_gen2_pm_enable(int unit,
                           int pm_id,
                           pm_info_t pm_info,
                           int flags,
                           int enable)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    uint32_t is_reset;
    
    /* Get the first physical port of the pm core */
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, 0, pm_info, &phy_acc));

        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3port_dc3mac_control_set(&phy_acc, 1));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3port_dc3mac_control_get(&phy_acc, &is_reset));
    if (enable && is_reset) {
        /* Bring Serdes OOR */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3port_tsc_ctrl_set(&phy_acc, 1/*tsc_pwr_on*/));
        /* Do not take MAC out of reset until PLL lock happens 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3port_dc3mac_control_set(&phy_acc, 0));*/
    } else if ((!enable) && (!is_reset)){ /* disable */
        /* put MAC in reset */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3port_dc3mac_control_set(&phy_acc, 1));
        /* If current PM is not master PM, put Serdes into reset*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3port_tsc_ctrl_set(&phy_acc, 0/*tsc_pwr_off*/));
    }
    return PHYMOD_E_NONE;
}
extern __phymod__dispatch__t__ plp_aperta2_phymod_tscp_driver;
extern __phymod_diagnostics__dispatch__t__ plp_aperta2_phymod_diagnostics_tscp_diagnostics_driver ; 

#define APERTA_PM8x100_GEN2_CORE_ACCESS_GET(unit, port, pm_info, phy_acc) \
    do { \
        PHYMOD_MEMCPY(&phy_acc,&PM_8x100_GEN2_INFO(pm_info)->int_core_access,sizeof(plp_aperta2_phymod_core_access_t));\
    } while (0)


STATIC
int _plp_aperta2_pm8x100_gen2_pm_core_probe(int unit, pm_info_t pm_info, const portmod_port_add_info_t* add_info)
{
    uint32_t probe =0;
    plp_aperta2_phymod_core_access_t core_acc;
    APERTA_PM8x100_GEN2_CORE_ACCESS_GET(unit, 0, pm_info, core_acc);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_phymod_tscp_driver.f_phymod_core_identify(&core_acc, 0, &probe));

    if (probe) {
       return PHYMOD_E_NONE;
    } else {
        PHYMOD_DEBUG_ERROR(("Aperta2 probe Failed\n"));
        return PHYMOD_E_UNAVAIL;
    }
    return PHYMOD_E_NONE;
}

/* This function will return required pll based on the ref_clk and vco */
int _plp_aperta2_pm8x100_gen2_vco_to_pll_get(portmod_vco_type_t vco, uint32_t* pll)
{
    int rv = PHYMOD_E_NONE;
    switch (vco) {
        case portmodVCO51P5625G:
            *pll = 0x000000a5 ;
            break;
        case portmodVCO53P125G:
            *pll = 0x000000AA ;
            break;
        case portmodVCOInvalid:
            *pll = 0;
            break;
        default:
            rv = PHYMOD_E_PARAM;
            break;
    }
    return rv;
}

STATIC
int _plp_aperta2_pm8x100_gen2_pm_serdes_core_init(int unit, pm_info_t pm_info, const portmod_port_add_info_t* add_info)
{
    portmod_vco_type_t init_vco = portmodVCOInvalid;
    int init_flags = 0;
    plp_aperta2_phymod_core_init_config_t core_config;
    plp_aperta2_phymod_core_status_t core_status;
    int core_is_initialized;
    int octal = APERTA2_OCTAL0;

    aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_GET(unit, pm_info, core_is_initialized)

    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_core_init_config_t_init(&core_config));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_core_status_t_init(&core_status));
    core_status.pmd_active = 0;

    if (PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask & 0xFF00) {
        octal = APERTA2_OCTAL1;
    }

    if (octal == APERTA2_OCTAL0) {
        /* Get requeseted VCO based on user input */
        if ( PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc == phymodPortLocSys) {
            init_vco = PM_8x100_GEN2_INFO(pm_info)->sys_tvco;
        } else if ( PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc == phymodPortLocLine) {
            init_vco = PM_8x100_GEN2_INFO(pm_info)->tvco;
        }
    } else {
        /* Get requeseted VCO based on user input */
        if ( PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc == phymodPortLocSys) {
            init_vco = PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco;
        } else if ( PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc == phymodPortLocLine) {
            init_vco = PM_8x100_GEN2_INFO(pm_info)->oc1_tvco;
        }
    }

    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_pm8x100_gen2_vco_to_pll_get(init_vco, &(core_config.pll0_div_init_value)));

    /* Add lane map config */
    core_config.firmware_load_method = PM_8x100_GEN2_INFO(pm_info)->fw_load_method;
    core_config.firmware_loader = PM_8x100_GEN2_INFO(pm_info)->external_fw_loader;
    core_config.polarity_map = PM_8x100_GEN2_INFO(pm_info)->polarity;

    if ( PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc == phymodPortLocSys) {
        PHYMOD_MEMCPY(&core_config.lane_map, &PM_8x100_GEN2_INFO(pm_info)->sys_lane_map, 
                sizeof(plp_aperta2_phymod_lane_map_t));
    } else {
        PHYMOD_MEMCPY(&core_config.lane_map, &PM_8x100_GEN2_INFO(pm_info)->lane_map, 
                sizeof(plp_aperta2_phymod_lane_map_t));
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_inf_config_t_init(&core_config.interface));
    core_config.interface.ref_clock = PM_8x100_GEN2_INFO(pm_info)->ref_clk;

    init_flags = PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info) | PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info);


    if (PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info)) {
            PHYMOD_CORE_INIT_F_EXECUTE_PASS1_SET(&core_config);
            PHYMOD_CORE_INIT_F_EXECUTE_PASS2_CLR(&core_config);
    }

    if (PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info)) {
        PHYMOD_CORE_INIT_F_EXECUTE_PASS1_CLR(&core_config);
        PHYMOD_CORE_INIT_F_EXECUTE_PASS2_SET(&core_config);
    }

    if (!PORTMOD_CORE_INIT_FLAG_INITIALZIED_GET(core_is_initialized)) {
        /* firmware load will happen after pass 1 */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_core_init(&PM_8x100_GEN2_INFO(pm_info)->int_core_access,
                    &core_config, &core_status));
        if (PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info) || init_flags == 0) {
            if ((PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc == phymodPortLocSys) ||
                    PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc == phymodPortLocDC) {
                aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_SET(unit, pm_info, core_is_initialized);
            }
        }
        if (PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info) || init_flags == 0) {
            if ((PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc == phymodPortLocSys) ||
                    PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc == phymodPortLocDC) {
                if (octal == 1) {
                    PORTMOD_CORE_INIT_FLAG_INITIALZIED_SET(core_is_initialized);
                }
                aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_SET(unit, pm_info, core_is_initialized);
            }
        }
    }

    return PHYMOD_E_NONE;
}

/*PM Core init routine*/
/* PM core init has 3 phase:
 * 1. Core probe: Probe serdes dispatch type.
 * 2. Pass1: Download FW, program ref_clk, polarity, lane map, etc.
 * 3. Pass2: PLL configuration, program UC core config, load PCS tables etc.
 */
int plp_aperta2_pm8x100_gen2_pm_serdes_core_init(int unit,
                                     int pm_id,
                                     pm_info_t pm_info,
                                     const portmod_port_add_info_t* add_info)
{
    int init_all = 0;
    int rv = PHYMOD_E_NONE;

    

    /* Update per core based database */
    init_all = (!PORTMOD_PORT_ADD_F_INIT_CORE_PROBE_GET(add_info) &&
                !PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info) &&
                !PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info)) ? 1 : 0;

    /* probe serdes core */
    if (PORTMOD_PORT_ADD_F_INIT_CORE_PROBE_GET(add_info) || init_all) {
        rv = _plp_aperta2_pm8x100_gen2_pm_core_probe(unit, pm_info, add_info);
        PHYMOD_IF_ERR_RETURN(rv);
    }

    /* Return here if caller only request Core Probe */
    if (!(PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info)) &&
        (PORTMOD_PORT_ADD_F_INIT_CORE_PROBE_GET(add_info))) {
        return (rv);
    }

    /* core config for internal serdes. */
    rv = _plp_aperta2_pm8x100_gen2_pm_serdes_core_init(unit, pm_info, add_info);
    PHYMOD_IF_ERR_RETURN(rv);


    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta2_pm8x100_gen2_pm_port_init(int unit,
                               int port,
                               pm_info_t pm_info,
                               int internal_port,
                               const portmod_port_add_info_t* add_info,
                               int enable)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    uint32_t rsv_mask;

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    if (enable) {
        /* RSV Mask */
        rsv_mask = 0;
        rsv_mask |= (1 << 3); /* Receive terminate/code error */
        rsv_mask |= (1 << 4); /* CRC error */
        rsv_mask |= (1 << 6); /* Truncated/Frame out of Range */
        rsv_mask |= (1 << 17); /* RUNT detected*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_mask_set(&phy_acc, rsv_mask));

        /* Init MAC */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_port_init(&phy_acc, 1));
        /* LSS */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3port_port_fault_link_status_set(&phy_acc, 1));

        /* Counter MAX size */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_cntmaxsize_set(unit, 0, pm_info, 0x3FFF));
    }


    return PHYMOD_E_NONE;
}

/*
 * Perfom MAC soft reset, including drain cells procedure.
 *
 * Supported reset modes: IN, OUT, IN-OUT
 *
 *    IN - set MAC in soft reset. 
 *         When reset_mode=IN the parameters saved_rx_enable, saved_mac_ctrl are outputs, 
 *         and should be sent back to this function when called with OUT.
 *    OUT - get MAC out of soft reset.
 *         When reset_mode=OUT the parameters saved_rx_enable, saved_mac_ctrl are inputs,
 *         the values that should be passed in are the values returned from IN.
 *         Example sequnece: 
 *          int saved_rx_enablel uint64_t saved_mac_ctrl;
 *          plp_aperta2_pm8x100_gen2_port_soft_reset(unit, port, pinfo, portmodMacSoftResetModeIn, &saved_rx_enablel, &saved_mac_ctrl);
 *          ... here the MAC is in reset ...
 *          plp_aperta2_pm8x100_gen2_port_soft_reset(unit, port, pinfo, portmodMacSoftResetModeOut, &saved_rx_enablel, &saved_mac_ctrl);
 *     IN-OUT - get MAC IN reset and then out of reset. 
 *          When  reset_mode=INOUT, saved_rx_enable, saved_mac_ctrl can be set to NULL.
 */
int plp_aperta2_pm8x100_gen2_port_soft_reset(int unit,
                                 int port,
                                 pm_info_t pm_info,
                                 portmod_mac_soft_reset_mode_t reset_mode,
                                 int *saved_rx_enable,
                                 uint64_t *saved_mac_ctrl)
{

    int rv, rx_enable = 0, retry_cnt = 1000;
    portmod_drain_cells_t drain_cells;
    uint32_t cell_count;
    uint64_t mac_ctrl;
    plp_aperta2_phymod_phy_access_t phy_acc;
    uint32_t soft_reset_enable;

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    

    if (portmodMacSoftResetModeOut != reset_mode) {
        /* reset_mode is portmodMacSoftResetModeIn or portmodMacSoftResetModeInOut */

        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_dc3mac_egress_queue_drain_get(&phy_acc, &mac_ctrl, &rx_enable));

        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_reset_get(&phy_acc, &soft_reset_enable));
        if (!soft_reset_enable) {
            PHYMOD_IF_ERR_RETURN( /*Drain cells */
                plp_aperta2_dc3mac_drain_cell_get(&phy_acc, &drain_cells));

            /* Start TX FIFO draining */
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_dc3mac_drain_cell_start(&phy_acc));

            /* De-assert SOFT_RESET to let the drain start */
            PHYMOD_IF_ERR_RETURN(
               plp_aperta2_dc3mac_reset_set(&phy_acc, 0));

            /* Wait until TX fifo cell count is 0 */
            for (;;) {
                rv = plp_aperta2_dc3mac_txfifo_cell_cnt_get(&phy_acc, &cell_count);
                PHYMOD_IF_ERR_RETURN(rv);
                if (cell_count == 0) {
                    break;
                }
                retry_cnt--;
                if (retry_cnt == 0) {
                    APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_INTERNAL, (SOC_MSG("ERROR: u=%d p=%d timeout draining TX FIFO (%d cells remain)\n"),
                                unit, port, cell_count));
                }
            }

            /* Stop TX FIFO draining */
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_dc3mac_drain_cell_stop(&phy_acc, &drain_cells));

            /* Put port into SOFT_RESET */
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_dc3mac_reset_set(&phy_acc, 1));
        }
    }

    if(portmodMacSoftResetModeIn != reset_mode) {
        /* reset_mode is portmodMacSoftResetModeOut or portmodMacSoftResetModeInOut */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_reset_get(&phy_acc, &soft_reset_enable));
        if (soft_reset_enable)
        {
            /* 
             * reset credits only if 
             * soft reset would toggle
             */
            /*rv = PM_8x100_GEN2_INFO(pm_info)->portmod_mac_soft_reset(unit, port, portmodCallBackActionTypeDuring);
            PHYMOD_IF_ERR_RETURN(rv);*/
        }

        /* For out of reset only operation, use MAC state from input */
        if (portmodMacSoftResetModeOut == reset_mode) {
            PHYMOD_NULL_CHECK(saved_rx_enable);
            PHYMOD_NULL_CHECK(saved_mac_ctrl);
            rx_enable = *saved_rx_enable;
            COMPILER_64_COPY(mac_ctrl, *saved_mac_ctrl);
        }
        {
            BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;
            
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(&phy_acc, &rval));
            BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_SOFT_RESETf_SET(rval, 0);

            /* Restore aperta2_dc3mac_CTRL to original value */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_mac_ctrl_set(&phy_acc, rval.v[0]));
        }
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta2_pm8x100_gen2_port_mac_drain_soft_reset(const plp_aperta2_phymod_phy_access_t *phy)
{
    int retry_cnt=1000;
    portmod_drain_cells_t drain_cells;
    uint32_t cell_count;

    /* Drain cells */   
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_dc3mac_drain_cell_get(phy, &drain_cells));

    /* Start TX FIFO draining */
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_dc3mac_drain_cell_start(phy));

    /* De-assert SOFT_RESET to let the drain start */
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_dc3mac_reset_set(phy, 0));
    
    /* FIXME:: Workaround for SDK-125372 */
    /* Wait until TX fifo cell count is 0 */
    for (;;) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_dc3mac_txfifo_cell_cnt_get(phy, &cell_count));
        if (cell_count == 0) {
            break;
        }
        retry_cnt--;
        if (retry_cnt == 0) {
            PHYMOD_DEBUG_ERROR(("ERROR: phy=%x timeout draining TX FIFO (%d cells remain)\n", phy->access.addr, cell_count));
            return PHYMOD_E_INTERNAL;
        }
    }
    /* Stop TX FIFO draining */
    PHYMOD_IF_ERR_RETURN( plp_aperta2_dc3mac_drain_cell_stop(phy, &drain_cells));

    /* Put port into SOFT_RESET */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_reset_set(phy, 1));
    return PHYMOD_E_NONE;
}

int _plp_aperta2_pm8x100_gen2_port_rx_restore_mac_out_of_reset(const plp_aperta2_phymod_phy_access_t *phy, int rx_enable)
{
    /* Enable RX, de-assert SOFT_RESET */
    return plp_aperta2_dc3mac_egress_queue_drain_rx_en(phy, rx_enable);
}

/*Add new port*/
int plp_aperta2_pm8x100_gen2_port_attach(int unit, int port, pm_info_t pm_info,
                             const portmod_port_add_info_t* add_info)
{
    int port_index = -1, tvco = 0;
    int nof_phys, side = 0, octal = 0;
    plp_aperta2_phymod_phy_init_config_t init_config;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    portmod_speed_config_t speed_config;
    uint8_t temp_octal = 0;

#ifdef APERTA2_OCTAL_NO_BCAST
    for (temp_octal = add_info->octal_start; temp_octal <= add_info->octal_end; temp_octal++) {
#else
    for (temp_octal = 1; temp_octal < 2; temp_octal++) {
#endif
        octal = (temp_octal - 1); /*changing to Port attach convention*/
        PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.lane_mask = 0xFF << (octal*8);
        PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask = 0xFF << (octal*8);
#ifdef APERTA2_OCTAL_NO_BCAST
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
#else
        for (side = phymodPortLocLine; side < phymodPortLocSys; side++) {
#endif
            PM_8x100_GEN2_INFO(pm_info)->int_phy_access.port_loc = side;
            PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc = side;

            PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_init_config_t_init(&init_config));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                        &params, 1, &phy_access, &nof_phys, NULL));
            /* Do not initalize port until MAC out of reset
            PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_pm_port_init(unit, port, pm_info, port_index, add_info, 1));*/


            init_config.an_en = add_info->autoneg_en;
            init_config.cl72_en = add_info->link_training_en;
            init_config.op_mode = add_info->interface_config.port_op_mode;

            /*Get PLL LOCK*/
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_phymod_tscp_driver.f_phymod_phy_init(&phy_access, &init_config));
        }
#ifdef APERTA2_OCTAL_NO_BCAST
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
#else
        for (side = phymodPortLocLine; side < phymodPortLocSys; side++) {
#endif

            PM_8x100_GEN2_INFO(pm_info)->int_phy_access.port_loc = side;
            PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc = side;

            PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                        &params, 1, &phy_access, &nof_phys, NULL));

            phy_access.access.lane_mask = (octal == 0) ? 0x1:0x100;
            /* Remove MAC out of reset*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3port_dc3mac_control_set(&phy_access, 0));
        }
#ifdef APERTA2_OCTAL_NO_BCAST
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
#else
        for (side = phymodPortLocLine; side < phymodPortLocSys; side++) {
#endif
            PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.lane_mask = 0xFF << (octal*8);
            PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask = 0xFF << (octal*8);
            PM_8x100_GEN2_INFO(pm_info)->int_phy_access.port_loc = side;
            PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc = side;

            /* initalize port */
            PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_pm_port_init(unit, port, pm_info, port_index, add_info, 1));
        }
        /* Initialize PHY*/
#ifdef APERTA2_OCTAL_NO_BCAST
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
#else 
        for (side = phymodPortLocLine; side < phymodPortLocSys; side++) {
#endif
            PM_8x100_GEN2_INFO(pm_info)->int_phy_access.port_loc = side;
            PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc = side;

            /* initialize phy */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                        &params, 1, &phy_access, &nof_phys, NULL));
            if (side == phymodPortLocLine) {
                if (octal ==0) {
                    tvco = PM_8x100_GEN2_INFO(pm_info)->tvco;
                } else {
                    tvco = PM_8x100_GEN2_INFO(pm_info)->oc1_tvco;
                }
            } else {
#ifdef APERTA2_OCTAL_NO_BCAST
                if (octal ==0) {
                    tvco = PM_8x100_GEN2_INFO(pm_info)->sys_tvco;
                } else {
                    tvco = PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco;
                }
#endif
            }
            if (tvco == portmodVCO51P5625G) {
                speed_config.speed =  APERTA2_SPEED_25G;
                speed_config.num_lane = 1;
                speed_config.fec = PORTMOD_PORT_PHY_FEC_NONE;
                speed_config.link_training = 0;
                speed_config.lane_config = 0; /*Init laneconfig*/
                speed_config.modulation= bcmAperta2ModulationNRZ;
                PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.lane_mask = 0x1<< (octal*8);
                PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask = 0x1<< (octal*8);

            } else {
                speed_config.speed =  APERTA2_SPEED_100G;
                speed_config.num_lane = 1;
                speed_config.fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN ;
                speed_config.link_training = 0;
                speed_config.lane_config = 0 ; /*Init laneconfig*/
                speed_config.modulation= bcmAperta2ModulationPAM4;
                PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.lane_mask = 0x1<< (octal*8);
                PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask = 0x1<< (octal*8);
            }
            PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_speed_config_set(unit, port, pm_info, &speed_config));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_soft_reset(unit, port, pm_info, portmodMacSoftResetModeIn_Out, NULL, NULL));

            if (add_info->init_config.parity_enable) {
                _plp_aperta2_pm8x100_gen2_port_interrupt_all_enable_set(unit, port, 1);

            }
        }
    }
    return PHYMOD_E_NONE;
}

/*get port cores' phymod access*/
int plp_aperta2_pm8x100_gen2_pm_core_info_get(int unit, pm_info_t pm_info, int phyn,
                                  portmod_pm_core_info_t* core_info)
{
    

    if(phyn < 0) {
        phyn = APERTA2_PM8X100_GEN2_MAX_NUM_PHYS;
    }

    core_info->ref_clk = PM_8x100_GEN2_INFO(pm_info)->ref_clk;
    PHYMOD_MEMCPY(&core_info->lane_map, &(PM_8x100_GEN2_INFO(pm_info)->lane_map),
               sizeof(plp_aperta2_phymod_lane_map_t));

    return PHYMOD_E_NONE;
}

/*Get PM phys.*/
int plp_aperta2_pm8x100_gen2_pm_phys_get(int unit, pm_info_t pm_info, portmod_pbmp_t* phys)
{
    
    return PHYMOD_E_NONE;
}


/*Port remove in PM level*/
int plp_aperta2_pm8x100_gen2_port_detach(int unit, int port, pm_info_t pm_info)
{
#if 0 /* Not needed, will add based on the need*/
    portmod_phy_timesync_config_t ts_config;
    int port_index = -1;
    uint32_t port_lane_mask;
    int invalid_port = -1, tmp_port, i = 0, pm_id;
    int is_last_one = 1, fec_null = 0;
    const uint32_t is_active = 0;
    uint8_t tvco_pll_active_lane_bm;
    uint8_t tvco_pll_adv_lane_bm;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    plp_aperta2_phymod_an_mode_type_t an_mode;
    uint32_t is_bypassed, is_other_bypassed = 0;
    int other_port_index;
    uint32_t other_bitmap;
    int is_last_non_bypassed, speed_id_table_status = 0;

    

    PHYMOD_IF_ERR_RETURN(portmod_port_periodic_callback_unregister(unit, port));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, &port_lane_mask));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_pm8x100_gen2_port_enable_set(unit, port, pm_info,
         PORTMOD_PORT_ENABLE_PHY | PORTMOD_PORT_ENABLE_MAC, 0));

    /* Disable 1588 on the port. */
    ts_config.flags = 0;
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_pm8x100_gen2_port_timesync_config_set(unit, port,
                                               pm_info, &ts_config));
    /* Clean up WB info for the port. */
    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, &tvco_pll_active_lane_bm));
    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, &tvco_pll_adv_lane_bm));

    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_PORT_IS_PCS_BYPASSED_GET(unit, pm_info, &is_bypassed, port_index));

    is_last_non_bypassed = !is_bypassed;

    for (i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
        PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_LANE2PORT_GET(unit, pm_info, i, &tmp_port));
        if (tmp_port == port) {
            port_index = (port_index == -1 ? i : port_index);
            PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_LANE2PORT_SET(unit, pm_info, i, invalid_port));
            PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_LANE2FEC_SET(unit, pm_info, i, fec_null));
            tvco_pll_active_lane_bm &= ~(1 << i);
            tvco_pll_adv_lane_bm &= ~(1 << i);
        } else if (tmp_port != invalid_port) {
            is_last_one = 0;
            if (!is_bypassed) {
                /* for DNX only - check if this is last non bypassed (not ILKN) port in order to reset MAC before powering down its clock */
                PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, tmp_port, pm_info, &other_port_index, &other_bitmap));
                PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_PORT_IS_PCS_BYPASSED_GET(unit, pm_info, &is_other_bypassed, other_port_index));
                if (!is_other_bypassed) {
                    is_last_non_bypassed = 0;
                }
            }
        }
    }

    if (port_index == -1)
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PORT, (SOC_MSG("Port %d wasn't found"), port));

    /* Power off PM if no port is configured on it. */
    if ((!tvco_pll_active_lane_bm) && (!tvco_pll_adv_lane_bm)) {
        /* Power down TVCO if it's not in use */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_pll_pwrdn(&phy_access, 0, 1));
        PM_8x100_GEN2_INFO(pm_info)->tvco = portmodVCOInvalid;

        /* FIXME, check if need to power down pm here */
        speed_id_table_status = aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_NOT_LOADED;
        PHYMOD_IF_ERR_RETURN
            (aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_SET(unit, pm_info, speed_id_table_status));
    }

    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, tvco_pll_active_lane_bm));
    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, tvco_pll_adv_lane_bm));

    an_mode = phymod_AN_MODE_NONE;
    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_AN_MODE_SET(unit, pm_info, an_mode, port_index));

    if (is_last_one) {
        PHYMOD_IF_ERR_RETURN(portmod_port_pm_id_get(unit, port, &pm_id));
        PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_IS_ACTIVE_SET(unit, pm_info, is_active));
        PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_SET(unit, pm_info, is_active));

        PM_8x100_GEN2_INFO(pm_info)->int_core_access.type = phymodDispatchTypeInvalid;
    }

    if (!is_last_one && is_last_non_bypassed) {
        /* DNX only - reset MAC if last NON bypassed port is removed */  
        int pm_enable_flags = 0;
        PORTMOD_PM_ENABLE_MAC_ONLY_SET(pm_enable_flags);
        PHYMOD_IF_ERR_RETURN(portmod_port_pm_id_get(unit, port, &pm_id));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_pm_enable(unit, pm_id, pm_info, pm_enable_flags, 0));

    }
#endif
    return PHYMOD_E_NONE;
}

/*Port replace in PM level*/
int plp_aperta2_pm8x100_gen2_port_replace(int unit, int port, pm_info_t pm_info, int new_port)
{
    return PHYMOD_E_NONE;
}


/*Port enable*/
int plp_aperta2_pm8x100_gen2_port_enable_set(int unit, int port, pm_info_t pm_info,
                                 int flags, int enable)
{
    int actual_flags = flags;
    int port_index;
    uint32_t  bitmap[1];
    int is_bypassed = 0;
    plp_aperta2_phymod_phy_access_t phy_access;
    portmod_access_get_params_t params;
    int nof_phys = 0;
    int rx_enable;
    uint64_t mac_ctrl;
    uint32_t soft_reset_enable;

    /* If no RX\TX flags - set both*/
    if((!PORTMOD_PORT_ENABLE_TX_GET(flags)) &&
       (!PORTMOD_PORT_ENABLE_RX_GET(flags))) {
        PORTMOD_PORT_ENABLE_RX_SET(actual_flags);
        PORTMOD_PORT_ENABLE_TX_SET(actual_flags);
    }

    /* if no MAC\Phy flags - set both*/
    if((!PORTMOD_PORT_ENABLE_PHY_GET(flags)) &&
       (!PORTMOD_PORT_ENABLE_MAC_GET(flags))) {
        PORTMOD_PORT_ENABLE_PHY_SET(actual_flags);
        PORTMOD_PORT_ENABLE_MAC_SET(actual_flags);
    }

    /*
     * if MAC is set and having only either RX or TX set
     * is invalid combination
     */
    if((PORTMOD_PORT_ENABLE_MAC_GET(actual_flags)) &&
       (!PORTMOD_PORT_ENABLE_PHY_GET(actual_flags))) {

        if((!PORTMOD_PORT_ENABLE_TX_GET(actual_flags)) ||
           (!PORTMOD_PORT_ENABLE_RX_GET(actual_flags))) {
           APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                      (SOC_MSG("MAC RX and TX can't be enabled separately")));
        }
    }

    /* For all phy supported. */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port,
                                     pm_info, &params, 1, &phy_access,
                                     &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, bitmap));
    aperta2_PM8x100_GEN2_PORT_IS_PCS_BYPASSED_GET(unit, pm_info, is_bypassed, port_index);

    if(enable){
        if((PORTMOD_PORT_ENABLE_MAC_GET(actual_flags)) && (!is_bypassed)) {

            if(!PM_8x100_GEN2_INFO(pm_info)->portmod_mac_soft_reset)
            {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_port_enable_set(&phy_access, 1));
            }
            else
            {
                /* Enable DC3MAC RX,TX, skip SOFT_RESET */
                PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_tx_enable_set(&phy_access, 1));
                PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rx_enable_set(&phy_access, 1));
                /* Get MAC CTRL */
                PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_egress_queue_drain_get(&phy_access, &mac_ctrl, &rx_enable));
                
                PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_soft_reset(unit, port, pm_info, portmodMacSoftResetModeOut,
                                                              &rx_enable, &mac_ctrl));
            }
        }
    }

    if (PORTMOD_PORT_ENABLE_PHY_GET(actual_flags)) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_phymod_port_enable_set(&phy_access, enable));
    } else {/* disable */
        /* disable PMD RX/TX */
        if (PORTMOD_PORT_ENABLE_PHY_GET(actual_flags)) {
            if (PORTMOD_PORT_ENABLE_TX_GET(actual_flags)) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_phymod_phy_tx_lane_control_set(&phy_access,
                                                   phymodTxSquelchOn));
            }

            if (PORTMOD_PORT_ENABLE_RX_GET(actual_flags)) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_phymod_phy_rx_lane_control_set(&phy_access,
                                                   phymodRxSquelchOn));
            }
        }

        /* Drain cells and reset MAC */
        if((PORTMOD_PORT_ENABLE_MAC_GET(actual_flags))  && (!is_bypassed)) {
            if(!PM_8x100_GEN2_INFO(pm_info)->portmod_mac_soft_reset)
            {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_port_enable_set(&phy_access, 0));
            }
            else
            {
                /* Disable DC3MAC RX,TX, skip SOFT_RESET */
               PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_reset_get(&phy_access, &soft_reset_enable));
                if (!soft_reset_enable)
                {
                    /* not in Soft reset yet */
                 PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_tx_enable_set (&phy_access, 1));
                 PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rx_enable_set (&phy_access, 0));
                 PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_soft_reset(unit,port, pm_info, portmodMacSoftResetModeIn,
                                                               &rx_enable, &mac_ctrl));
                }
            }
        }
        /* disable phy port */
        if (PORTMOD_PORT_ENABLE_PHY_GET(actual_flags)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_port_enable_set(&phy_access, enable));
        } 
    }


    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_enable_get(int unit, int port, pm_info_t pm_info,
                                 int flags, int* enable)
{
    int nof_phys = 0, port_index;
    uint32_t phy_enable = 1, mac_enable = 1;
    uint32_t bitmap[1];
    int is_bypassed = 0, actual_flags = flags;
    plp_aperta2_phymod_phy_access_t phy_access;
    uint32_t mac_reset = 0;

    
    PHYMOD_NULL_CHECK(pm_info);

    /* If no RX\TX flags - set both*/
    if((!PORTMOD_PORT_ENABLE_TX_GET(flags)) &&
       (!PORTMOD_PORT_ENABLE_RX_GET(flags))) {
        PORTMOD_PORT_ENABLE_RX_SET(actual_flags);
        PORTMOD_PORT_ENABLE_TX_SET(actual_flags);
    }

    /* if no MAC\Phy flags - set both*/
    if((!PORTMOD_PORT_ENABLE_PHY_GET(flags)) &&
       (!PORTMOD_PORT_ENABLE_MAC_GET(flags))) {
        PORTMOD_PORT_ENABLE_PHY_SET(actual_flags);
        PORTMOD_PORT_ENABLE_MAC_SET(actual_flags);
    }

    /* PHY access for all PHY supported. */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port,
                                     pm_info, NULL, 1, &phy_access,
                                     &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, bitmap));
    aperta2_PM8x100_GEN2_PORT_IS_PCS_BYPASSED_GET(unit, pm_info, is_bypassed, port_index);




    if (PORTMOD_PORT_ENABLE_PHY_GET(actual_flags)) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_phymod_tscp_driver.f_phymod_port_enable_get(&phy_access, &phy_enable));
        
    }
    if ((PORTMOD_PORT_ENABLE_MAC_GET(actual_flags)) && (!is_bypassed)) {
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_dc3mac_reset_get(&phy_access, &mac_reset));
        if (mac_reset) {
            mac_enable = 0;
        } else {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_dc3mac_rx_enable_get(&phy_access, &mac_enable));
        }
    }
    *enable = (mac_enable && phy_enable);


    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta2_pm8x100_gen2_fec_lanebitmap_get(int unit,
                                     pm_info_t pm_info,
                                     uint8_t *rs544_bitmap,
                                     uint8_t *rs272_bitmap)
{
    int i;
    int tmp_fec;

    *rs544_bitmap = 0;
    *rs272_bitmap = 0;
    /* Get RS528, RS544, RS272 usage bitmap from WB */
    for (i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++){
        aperta2_PM8x100_GEN2_LANE2FEC_GET(unit, pm_info, i, tmp_fec);
        if ((tmp_fec == PORTMOD_PORT_PHY_FEC_RS_544) ||
                   (tmp_fec == PORTMOD_PORT_PHY_FEC_RS_544_2XN)) {
            *rs544_bitmap |= 1 << i;
        } else if ((tmp_fec == PORTMOD_PORT_PHY_FEC_RS_272) ||
                   (tmp_fec == PORTMOD_PORT_PHY_FEC_RS_272_2XN)) {
            *rs272_bitmap |= 1 << i;
        }
    }

    return PHYMOD_E_NONE;
}

/*
 * Function:
 *_plp_aperta2_pm8x100_gen2_vco_setting_validate
 *
 * Purpose:
 *     VCOs setting validation.
 *
 * Inputs:
 *     unit:               unit number;
 *     pm_info:            pm_info data structure;
 *     ports:              Logical port bitmaps which reqiring the input vcos;
 *     required_vco:       Required vco;
 *     flag:               PLL switch flag;
 * Output:
 *     vco_setting:    If validation pass, vco_setting will indicating
 *                     what's the new vco rate and whether it is new or not;
 *                     vco_setting.num_speeds and vco_setting.speed_config_list
 *                     are no care in this function.
 */
int _plp_aperta2_pm8x100_gen2_vco_setting_validate(int unit,
                                       pm_info_t pm_info,
                                       const portmod_pbmp_t* ports,
                                       portmod_vco_type_t required_vco,
                                       int flag,
                                       portmod_pm_vco_setting_t* vco_setting)
{
    int tvco_pll_active_lane_bitmap, tvco_pll_adv_lane_bitmap;
    int is_tvco_in_use;
    portmod_vco_type_t current_tvco;
    int nof_phys = 0;
    uint8_t active_eth_lanes = 0;
    plp_aperta2_phymod_phy_access_t phy_access;
    int octal = 0;

    /*
     * Get lane mask for the input ports
     */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, 0,
                                     pm_info, NULL, 1, &phy_access,
                                     &nof_phys, NULL));
    
    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_active_lane_bitmap);
    aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, tvco_pll_adv_lane_bitmap);
    PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_pm8x100_gen2_pm_active_eth_lanes_get(unit, pm_info, &active_eth_lanes));

    /* Remove input lanes from tvco and check if it is in use */
    tvco_pll_active_lane_bitmap &= ~phy_access.access.lane_mask;
    tvco_pll_adv_lane_bitmap &= ~phy_access.access.lane_mask;
    active_eth_lanes &= ~phy_access.access.lane_mask;
    is_tvco_in_use = ((tvco_pll_active_lane_bitmap | tvco_pll_adv_lane_bitmap | active_eth_lanes) == 0) ? 0 : 1;
    if (is_tvco_in_use) {
        /* tvco in use (not refering to the ports that are currently requested) */
        octal = APERTA2_GET_OCTAL( phy_access.access.lane_mask);
        if (octal == APERTA2_PM_OCTAL1) { /* Octal 1*/
            if (phy_access.port_loc == phymodPortLocLine) {
                current_tvco = PM_8x100_GEN2_INFO(pm_info)->tvco;
            } else {
                current_tvco = PM_8x100_GEN2_INFO(pm_info)->sys_tvco;
            }
        } else {          /* Octal 2*/
            if (phy_access.port_loc == phymodPortLocLine) {
                current_tvco = PM_8x100_GEN2_INFO(pm_info)->oc1_tvco;
            } else {
                current_tvco = PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco;
            }
        }
        if (required_vco != current_tvco) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                (SOC_MSG("Can not accommodate VCO settings.\n request [VCO %d, state [TVCO %d]\n"), required_vco, current_tvco));
        }
    }
    else {
        current_tvco = required_vco;
    }

    /* Set output */
    vco_setting->tvco = current_tvco;
    vco_setting->is_tvco_new = (PM_8x100_GEN2_INFO(pm_info)->tvco == current_tvco) ? 0 : 1;


    return PHYMOD_E_NONE;
}

/*
 * Validate if a set of speed can be configured for given PORTMACRO without
 * affecting the other active ports.
 *
 * Inputs:
 *     unit:           unit number;
 *     pm_id           portmacro ID;
 *     pm_info:        pm_info data structure;
 *     ports:          Logical port bitmaps which reqiring the input vcos;
 *     flag:           PLL switch flag.
 *
 * Output:
 *     vco_setting:    If validation pass, vco_setting will indicating
 *                     what's the new vco rates and whether they are new or not;
 *
 */
int plp_aperta2_pm8x100_gen2_pm_speed_config_validate(int unit,
                                          int pm_id,
                                          pm_info_t pm_info,
                                          const portmod_pbmp_t* ports,
                                          int flag,
                                          portmod_pm_vco_setting_t* vco_setting)
{
    int i, rv;
    portmod_vco_type_t vco, required_vco = portmodVCOInvalid;
    uint8_t rs528_bm = 0, rs544_bm = 0, rs272_bm = 0, affected_bm = 0;
    int nof_phys;
    plp_aperta2_phymod_phy_access_t phy_access;

    

    /* Get RS FEC usage bitmaps from WB */
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_fec_lanebitmap_get(unit, pm_info,&rs544_bm, &rs272_bm));

    /* Remove the input ports from FEC usage bitmaps*/
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, 0, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));
    rs528_bm &= ~phy_access.access.lane_mask;
    rs544_bm &= ~phy_access.access.lane_mask;
    rs272_bm &= ~phy_access.access.lane_mask;

    /* Get the VCOs required by the input speed config list */
    for (i = 0; i < vco_setting->num_speeds; i++) {
        vco = portmodVCOInvalid;
        rv = _plp_aperta2_pm8x100_gen2_port_speed_config_to_vco_get(&(vco_setting->speed_config_list[i]),
                0,
                &vco);
        if (rv == PHYMOD_E_CONFIG) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_CONFIG,
                    (SOC_MSG("Can not support speed: %d num_lane: %d fec: %d"),
                     vco_setting->speed_config_list[i].speed,
                     vco_setting->speed_config_list[i].num_lane,
                     vco_setting->speed_config_list[i].fec));
        }
        /* Can not accommodate more than one VCO */
        if (required_vco == portmodVCOInvalid) {
            required_vco = vco;
        } else if (required_vco != vco) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                    (SOC_MSG("Can not accommodate more than one VCO.\n")));
        }

        if ((vco_setting->speed_config_list[i].fec == PORTMOD_PORT_PHY_FEC_RS_272) ||
                (vco_setting->speed_config_list[i].fec == PORTMOD_PORT_PHY_FEC_RS_272_2XN)) {
            PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_lanebitmap_set(vco_setting->port_starting_lane_list[i],
                        vco_setting->speed_config_list[i].num_lane,
                        &rs272_bm));
        } else if ((vco_setting->speed_config_list[i].fec == PORTMOD_PORT_PHY_FEC_RS_544) ||
                (vco_setting->speed_config_list[i].fec == PORTMOD_PORT_PHY_FEC_RS_544_2XN)) {
            PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_lanebitmap_set(vco_setting->port_starting_lane_list[i],
                        vco_setting->speed_config_list[i].num_lane,
                        &rs544_bm));
        } else if (vco_setting->speed_config_list[i].fec == PORTMOD_PORT_PHY_FEC_RS_FEC) {
            PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_lanebitmap_set(vco_setting->port_starting_lane_list[i],
                        vco_setting->speed_config_list[i].num_lane,
                        &rs528_bm));
        }
    }

    /* Validate FEC settings for RS528, RS544 and RS272 */
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_fec_validate(unit, rs544_bm, rs272_bm, &affected_bm));

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_vco_setting_validate(unit, pm_info, ports, required_vco, flag, vco_setting));


    return PHYMOD_E_NONE;
}

/*Port speed validation.*/
int plp_aperta2_pm8x100_gen2_port_speed_config_validate(int unit,
                                            int port,
                                            pm_info_t pm_info,
                                            const portmod_speed_config_t* speed_config,
                                            portmod_pbmp_t* affected_pbmp)
{
    portmod_vco_type_t cur_vco = portmodVCOInvalid, req_vco = portmodVCOInvalid;
    uint8_t rs544_req = 0, rs272_req = 0, vco_change_allowed = 0;
    uint8_t rs544_bm = 0, rs272_bm = 0;
    uint32_t pll_div = 0, is_pll_pwrdn = 1;
    int nof_phys, pm_id = 0;
    int rv = 0;
    int tvco_pll_active_lane_bitmap;
    uint32_t port_lane_mask[1];
    uint8_t affected_lane_bitmap = 0;
    plp_aperta2_phymod_phy_access_t phy_access, phy_copy;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                         NULL, 1, &phy_access, &nof_phys, NULL));
    port_lane_mask[0] = phy_access.access.lane_mask;
    
    /* 1. Check lane_map. */
    rv = _plp_aperta2_pm8x100_gen2_pm_port_lane_map_validate(unit, port_lane_mask[0]);
    if (rv != PHYMOD_E_NONE) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("Invalid lane map request on exising logical port.")));
    }

    /*
     * 2. Validate force speed ability.
     * Check if port speed configuration is one of the entries of
     * force_speed_ability table.
     */
    rv = _plp_aperta2_pm8x100_gen2_port_speed_config_to_vco_get(speed_config, 0, &req_vco);
    if (rv == PHYMOD_E_CONFIG) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_CONFIG,
                  (SOC_MSG("Speed config is not supported")));
    }

    /* 3. check 10G speed with link training enabled */
    if ((speed_config->link_training) && (speed_config->speed == 10000)) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("Link training is not supported for this speed %"PRIu32".\n"), speed_config->speed));
    }

    /* 4. Validate FEC settings on each MPP. */
    if ((speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_544) ||
        (speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_544_2XN)) {
        rs544_req = 1;
    } else if ((speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_272) ||
        (speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_272_2XN)) {
        rs272_req = 1;
    }

    if (rs544_req || rs272_req) {
        /* Get RS544, RS272 usage bitmap from WB */
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_fec_lanebitmap_get(unit, pm_info, &rs544_bm, &rs272_bm));

        rs544_bm &= ~port_lane_mask[0];
        rs272_bm &= ~port_lane_mask[0];
        if (rs544_req) {
            rs544_bm |= port_lane_mask[0];
        }
        if (rs272_req) {
            rs272_bm |= port_lane_mask[0];
        }
        rv = _plp_aperta2_pm8x100_gen2_fec_validate(unit, rs544_bm, rs272_bm, &affected_lane_bitmap);
        if (rv != PHYMOD_E_NONE) {
            /* If FEC validate fails, return affected port bit map */
            return rv;
        }
    }

    /* 5. Check requested VCO is valid on the PM. */
    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_active_lane_bitmap);
    if ((tvco_pll_active_lane_bitmap == port_lane_mask[0]) ||
        (tvco_pll_active_lane_bitmap == 0x0)) {
        vco_change_allowed = 1;
    }

    /* Use the first lane. */
    PHYMOD_MEMCPY(&phy_copy, &phy_access, sizeof(phy_access));
    phy_copy.access.lane_mask = 0x1;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_phymod_tscp_driver.f_phymod_phy_pll_powerdown_get(&phy_copy, 0, &is_pll_pwrdn));
    if (is_pll_pwrdn) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("The VCO is powered off on port %"PRIu32".\n"), port));
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_pll_multiplier_get(&phy_copy, &pll_div));
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_pll_to_vco_get(pll_div, &cur_vco));
    }

    if (req_vco != cur_vco) {
        if (vco_change_allowed) {
            /*
             * PM reconfigure the VCO based on the new speed once the
             * validation passes.
             */
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_pm8x100_gen2_pm_vco_reconfig(unit, pm_id, pm_info, &req_vco));
        } else {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (SOC_MSG("Requested VCO %"PRIu32" for speed "
                                 "%"PRIu32" cannot be configured on current "
                                 "settings VCO %"PRIu32"\n"),
                                 req_vco, speed_config->speed, cur_vco));

        }
   }


    return PHYMOD_E_NONE;
}

/*
 * This API reconfig the VCO rates of the PM.
 *
 * 'plp_aperta2_pm8x100_gen2_pm_speed_config_validate' need to be called before this function.
 * Only when plp_aperta2_pm8x100_gen2_pm_speed_config_validate reuturns E_NONE and indicates
 * VCO change requirement, user can call this API to change VCO.
 *
 * vco[0]: TVCO.
 */
int plp_aperta2_pm8x100_gen2_pm_vco_reconfig(int unit,
                                 int pm_id,
                                 pm_info_t pm_info,
                                 const portmod_vco_type_t* vco)
{
    uint8_t zero = 0, active_eth_lanes = 0;
    portmod_vco_type_t cur_tvco = PM_8x100_GEN2_INFO(pm_info)->tvco;
    uint32_t pll_div;
    plp_aperta2_phymod_phy_access_t phy_access;
    int fec_null = 0, i;
    int tvco_pll_active_lane_bitmap, tvco_pll_adv_lane_bitmap;
    int speed_id_table_status = 0;
    int octal = 0;

    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_active_lane_bitmap);
    aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, tvco_pll_adv_lane_bitmap);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_pm_active_eth_lanes_get(unit, pm_info, &active_eth_lanes));

    PHYMOD_MEMCPY(&phy_access, &(PM_8x100_GEN2_INFO(pm_info)->int_core_access),
                   sizeof(plp_aperta2_phymod_phy_access_t));

    octal = APERTA2_GET_OCTAL( phy_access.access.lane_mask);
    if (octal == APERTA2_PM_OCTAL1) { /* Octal 1*/
        if (phy_access.port_loc == phymodPortLocLine) {
            cur_tvco = PM_8x100_GEN2_INFO(pm_info)->tvco;
        } else {
            cur_tvco = PM_8x100_GEN2_INFO(pm_info)->sys_tvco;
        }
    } else {          /* Octal 2*/
        if (phy_access.port_loc == phymodPortLocLine) {
            cur_tvco = PM_8x100_GEN2_INFO(pm_info)->oc1_tvco;
        } else {
            cur_tvco = PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco;
        }
    }

    if (cur_tvco != vco[0]) {
        /* TVCO need to be changed */
        PM_8x100_GEN2_INFO(pm_info)->tvco = vco[0];
        /* next get the TVCO's PLL divider */
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_vco_to_pll_get(vco[0], &pll_div));

        if (pll_div != 0) {
            if (active_eth_lanes == (tvco_pll_adv_lane_bitmap | tvco_pll_active_lane_bitmap)) {
                /*
                 * Set lane_mask to 0xFF for the following two cases:
                 *
                 * 1. If active_eth_lanes != 0, meaning there are only ethernet ports
                 *    in the PM, it is safe to set lane_mask to 0xFF.
                 * 2. If active_eth_lanes == 0, meaning there is no active ports in
                 *    the PM, it is safe to set lane_mask to 0xFF.
                 */
                phy_access.access.lane_mask = 0xFF;
            } else {
                /*
                 * There are ILKN ports in the PM, lane_mask should include:
                 *
                 * 1. All ethernet lanes
                 * 2. ILKN lanes using TVCO.
                 */
                phy_access.access.lane_mask = active_eth_lanes | tvco_pll_active_lane_bitmap;
            }
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_phymod_tscp_driver.f_phymod_phy_pll_reconfig(&phy_access, 0, pll_div, (PM_8x100_GEN2_INFO(pm_info)->ref_clk)));
        } else {
             /* will pwoer down this PLL */
             PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_pll_pwrdn(&phy_access, 0, 1));
        }
        /* For Ethernet mode, TVCO change will reset the whole serdes core.
         * So we will clear:
         *     1. the lane bitmaps for both PLLs.
         *     2. FEC type for all lanes.
         */
        aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, zero);
        aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, zero);
        for (i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
            aperta2_PM8x100_GEN2_LANE2FEC_SET(unit, pm_info, i, fec_null);
        }
        speed_id_table_status = aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_NOT_LOADED;
        aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_SET(unit, pm_info, speed_id_table_status);
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_portmod_common_pmd_lane_config_decode(uint32_t lane_config, portmod_pmd_lane_config_t* portmod_lane_config)
{
    portmod_lane_config->pmd_firmware_lane_config.DfeOn   = PORTMOD_PORT_PHY_LANE_CONFIG_DFE_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.LpDfeOn = PORTMOD_PORT_PHY_LANE_CONFIG_LP_DFE_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.ForceBrDfe = PORTMOD_PORT_PHY_LANE_CONFIG_BR_DFE_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.MediaType = PORTMOD_PORT_PHY_LANE_CONFIG_MEDIUM_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.UnreliableLos = PORTMOD_PORT_PHY_LANE_CONFIG_UNRELIABLE_LOS_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.ScramblingDisable = PORTMOD_PORT_PHY_LANE_CONFIG_SCRAMBLING_DISABLE_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.Cl72AutoPolEn = PORTMOD_PORT_PHY_LANE_CONFIG_CL72_POLARITY_AUTO_EN_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.Cl72RestTO = PORTMOD_PORT_PHY_LANE_CONFIG_CL72_RESTART_TIMEOUT_EN_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.ForceES = PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_ES_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.ForceNS = PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NS_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.LinkPartnerPrecoderEn = PORTMOD_PORT_PHY_LANE_CONFIG_LP_PREC_EN_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.ForcePAM4Mode = PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_PAM4_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.ForceNRZMode = PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NRZ_GET(lane_config);
    portmod_lane_config->pmd_firmware_lane_config.RxLowPower = PORTMOD_PORT_PHY_LANE_CONFIG_RX_LOW_POWER_GET(lane_config);
    portmod_lane_config->pam4_channel_loss = PORTMOD_PORT_PHY_LANE_CONFIG_PAM4_CHANNEL_LOSS_GET(lane_config);

    return 0;
}


/* set/get the speed config for the specified port.*/
int plp_aperta2_pm8x100_gen2_port_speed_config_set(int unit, int port, pm_info_t pm_info, const portmod_speed_config_t* speed_config)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    phymod_phy_signalling_method_t signalling_mode;
    int nof_phys, i, port_is_pcs_bypassed =0 ;
    portmod_pbmp_t affected_pbmp;
    portmod_pmd_lane_config_t portmod_lane_config;
    phymod_phy_speed_config_t phy_speed_config;
    phymod_phy_pll_state_t old_pll_state, new_pll_state;
    uint32_t lane_config, port_lane_mask;
    int tvco_pll_lanes_bitmap;
    portmod_vco_type_t required_vco;
    int port_index = 0, flags = 0;
    plp_aperta2_phymod_an_mode_type_t an_mode = phymod_AN_MODE_NONE;
    portmod_encap_t encap;
    int octal = 0, configured_vco = 0;

    PHYMOD_MEMSET(&portmod_lane_config, 0,sizeof(portmod_lane_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_phy_speed_config_t_init(&phy_speed_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_phy_pll_state_t_init(&old_pll_state));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_phy_pll_state_t_init(&new_pll_state));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));
#if 0 /*Having it for reference*/
    COMPILER_64_ZERO(mac_ctrl);
    if(!PM_8x100_GEN2_INFO(pm_info)->portmod_mac_soft_reset)
    {
        /* Get original MAC CTRL */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_egress_queue_drain_get(&phy_access, &mac_ctrl, &rx_enable));

        /* Disable MAC RX, drain MAC FIFO, assert MAC SOFT_RESET */
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_mac_drain_soft_reset(&phy_access));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_soft_reset(unit, port, pm_info, portmodMacSoftResetModeIn, &rx_enable, &mac_ctrl));
    }
#endif
    /* Validate speed_config */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_speed_config_validate(unit, port, pm_info, speed_config, &affected_pbmp));

    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_lanes_bitmap);
    old_pll_state.pll0_lanes_bitmap = tvco_pll_lanes_bitmap;
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_speed_config_to_vco_get(speed_config, port_is_pcs_bypassed, &required_vco));

    octal = APERTA2_GET_OCTAL( phy_access.access.lane_mask);
    port_lane_mask = phy_access.access.lane_mask;

    if (octal == APERTA2_PM_OCTAL1) { /* Octal 1*/
        if (phy_access.port_loc == phymodPortLocLine) {
            configured_vco = PM_8x100_GEN2_INFO(pm_info)->tvco;
        } else {
            configured_vco = PM_8x100_GEN2_INFO(pm_info)->sys_tvco;
        }
    } else {          /* Octal 2*/
        if (phy_access.port_loc == phymodPortLocLine) {
            configured_vco = PM_8x100_GEN2_INFO(pm_info)->oc1_tvco;
        } else {
            configured_vco = PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco;
        }
    }

    if (required_vco == configured_vco) {
        tvco_pll_lanes_bitmap |= port_lane_mask;
    } else {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                           (SOC_MSG("VCO need to be reconfigured before changing speed.")));
    }
    /* Decode the lane_config in speed_config */
    lane_config = speed_config->lane_config;
    /* Retrieve the default lane config */
    signalling_mode = speed_config->modulation;

    if (lane_config == -1) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_phymod_phy_lane_config_default_get(&phy_access, signalling_mode, &portmod_lane_config.pmd_firmware_lane_config));
        portmod_lane_config.pam4_channel_loss = 0;
    } else {
        if (signalling_mode != phymodSignallingMethodNRZ) {
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_PAM4_SET(lane_config);
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NRZ_CLEAR(lane_config);
        } else {
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NRZ_SET(lane_config);
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_PAM4_CLEAR(lane_config);
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_ES_CLEAR(lane_config);
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NS_CLEAR(lane_config);
        }
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_portmod_common_pmd_lane_config_decode(lane_config, &portmod_lane_config));
    }

    if (speed_config->fec == PORTMOD_PORT_PHY_FEC_NONE) {
        phy_speed_config.fec_type = phymod_fec_None;
    } else if (speed_config->fec ==  PORTMOD_PORT_PHY_FEC_RS_FEC) {
        phy_speed_config.fec_type = phymod_fec_CL91;
    } else if (speed_config->fec ==  PORTMOD_PORT_PHY_FEC_RS_544) {
        phy_speed_config.fec_type = phymod_fec_RS544;
    } else if (speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_272) {
        phy_speed_config.fec_type = phymod_fec_RS272;
    } else if (speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_544_2XN) {
        phy_speed_config.fec_type = phymod_fec_RS544_2XN;
    } else if (speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_272_2XN) {
        phy_speed_config.fec_type = phymod_fec_RS272_2XN;
    }

    phy_speed_config.data_rate = speed_config->speed;
    phy_speed_config.linkTraining = speed_config->link_training;
    phy_speed_config.PAM4_channel_loss = portmod_lane_config.pam4_channel_loss;
    phy_speed_config.pmd_lane_config = portmod_lane_config.pmd_firmware_lane_config;
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_pm_port_init(unit, port, pm_info, 0, NULL, 1));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_dc3port_port_mode_set(&phy_access, flags, phy_access.access.lane_mask));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_tscp_driver.f_phymod_phy_speed_config_set(&phy_access, &phy_speed_config, &old_pll_state,
                                     &new_pll_state));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_dc3mac_encap_get(&phy_access, &encap));

#if 0 /*Having it for reference*/
    if(!PM_8x100_GEN2_INFO(pm_info)->portmod_mac_soft_reset)
    {
        /* Enable MAC RX, De-assert MAC SOFT_RESET */
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_rx_restore_mac_out_of_reset(&phy_access, rx_enable));
        /* Restore original MAC CONTROL */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_mac_ctrl_set(&phy_access, mac_ctrl));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_soft_reset(unit, port, pm_info, portmodMacSoftResetModeOut, &rx_enable, &mac_ctrl));
    }
#endif
    /* Update PLL active lane bitmaps */
    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, tvco_pll_lanes_bitmap);

    /* Clear the adv lane bitmap for the port because it's configured as a force speed port */
    aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, tvco_pll_lanes_bitmap);
    port_lane_mask = phy_access.access.lane_mask;
    tvco_pll_lanes_bitmap &= ~port_lane_mask;
    aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, tvco_pll_lanes_bitmap);

    /* Clear an_mode for the port to force user set new abilities before enable AN later on */
    aperta2_PM8x100_GEN2_AN_MODE_SET(unit, pm_info, an_mode, port_index);
    (void)port_index;
    /* Update FEC usage in WB */
    for (i = 0 ; i < MAX_PORTS_PER_PM8X100_GEN2; i++) {
        if ((port_lane_mask >> i) & 0x1) {
            aperta2_PM8x100_GEN2_LANE2FEC_SET(unit, pm_info, i, speed_config->fec);
        }
    }


    return PHYMOD_E_NONE;
}

int plp_aperta2_portmod_common_pmd_lane_config_encode(portmod_pmd_lane_config_t* portmod_lane_config, uint32_t* lane_config)
{
    *lane_config = 0;
    if (portmod_lane_config->pmd_firmware_lane_config.DfeOn) {
        PORTMOD_PORT_PHY_LANE_CONFIG_DFE_SET(*lane_config);
    }

    if (portmod_lane_config->pmd_firmware_lane_config.LpDfeOn) {
        PORTMOD_PORT_PHY_LANE_CONFIG_LP_DFE_SET(*lane_config);
    }

    if (portmod_lane_config->pmd_firmware_lane_config.ForceBrDfe) {
        PORTMOD_PORT_PHY_LANE_CONFIG_BR_DFE_SET(*lane_config);
    }

    PORTMOD_PORT_PHY_LANE_CONFIG_MEDIUM_SET(*lane_config,
         portmod_lane_config->pmd_firmware_lane_config.MediaType);

    if (portmod_lane_config->pmd_firmware_lane_config.UnreliableLos) {
        PORTMOD_PORT_PHY_LANE_CONFIG_UNRELIABLE_LOS_SET(*lane_config);
    }

    if (portmod_lane_config->pmd_firmware_lane_config.ScramblingDisable) {
        PORTMOD_PORT_PHY_LANE_CONFIG_SCRAMBLING_DISABLE_SET(*lane_config);
    }
    if (portmod_lane_config->pmd_firmware_lane_config.Cl72AutoPolEn) {
        PORTMOD_PORT_PHY_LANE_CONFIG_CL72_POLARITY_AUTO_EN_SET(*lane_config);
    }
    if (portmod_lane_config->pmd_firmware_lane_config.Cl72RestTO) {
        PORTMOD_PORT_PHY_LANE_CONFIG_CL72_RESTART_TIMEOUT_EN_SET(*lane_config);
    }
    if (portmod_lane_config->pmd_firmware_lane_config.ForceES) {
        PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_ES_SET(*lane_config);
    }
    if (portmod_lane_config->pmd_firmware_lane_config.ForceNS) {
        PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NS_SET(*lane_config);
    }
    if (portmod_lane_config->pmd_firmware_lane_config.LinkPartnerPrecoderEn) {
        PORTMOD_PORT_PHY_LANE_CONFIG_LP_PREC_EN_SET(*lane_config);
    }
    if (portmod_lane_config->pmd_firmware_lane_config.ForcePAM4Mode) {
        PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_PAM4_SET(*lane_config);
    }
    if (portmod_lane_config->pmd_firmware_lane_config.ForceNRZMode) {
        PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NRZ_SET(*lane_config);
    }

    PORTMOD_PORT_PHY_LANE_CONFIG_PAM4_CHANNEL_LOSS_SET(*lane_config,
        portmod_lane_config->pam4_channel_loss);

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_speed_config_get(int unit, int port, pm_info_t pm_info, portmod_speed_config_t* speed_config)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys, port_num_lanes;
    phymod_phy_speed_config_t phy_speed_config;
    plp_aperta2_phymod_autoneg_status_t an_status;
    portmod_pmd_lane_config_t portmod_lane_config;
    uint32_t lane_config = 0;

    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));
    port_num_lanes = plp_aperta2_phymod_count_set_bits(phy_access.access.lane_mask);

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_autoneg_status_get(unit, port, pm_info, &an_status));

    speed_config->num_lane = port_num_lanes;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_speed_config_t_init(&phy_speed_config));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_speed_config_get(&phy_access, &phy_speed_config));
    portmod_lane_config.pam4_channel_loss = phy_speed_config.PAM4_channel_loss;
    portmod_lane_config.pmd_firmware_lane_config = phy_speed_config.pmd_lane_config;
    /* Encode the lane_config in speed_config */
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_portmod_common_pmd_lane_config_encode(&portmod_lane_config, &lane_config));

    if (phy_speed_config.fec_type == phymod_fec_None) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_NONE;
    } else if (phy_speed_config.fec_type == phymod_fec_CL91) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_RS_FEC;
    } else if (phy_speed_config.fec_type == phymod_fec_RS544) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_RS_544;
    } else if (phy_speed_config.fec_type == phymod_fec_RS272) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_RS_272;
    } else if (phy_speed_config.fec_type == phymod_fec_RS544_2XN) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN;
    } else if (phy_speed_config.fec_type == phymod_fec_RS272_2XN) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_RS_272_2XN;
    }

    speed_config->speed = phy_speed_config.data_rate;
    speed_config->link_training = phy_speed_config.linkTraining;
    speed_config->lane_config = lane_config;


    return PHYMOD_E_NONE;
}

/*Port cl72 set\get*/
int plp_aperta2_pm8x100_gen2_port_cl72_set(int unit, int port, pm_info_t pm_info, uint32_t enable)
{
    int nof_phys;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_MEMSET(&params , 0, sizeof(portmod_access_get_params_t));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_tscp_driver.f_phymod_phy_cl72_set(&phy_access, enable));


    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_cl72_get(int unit, int port, pm_info_t pm_info, uint32_t* enable)
{
    int nof_phys;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_MEMSET(&params , 0, sizeof(portmod_access_get_params_t));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_phymod_tscp_driver.f_phymod_phy_cl72_get(&phy_access, enable));


    return PHYMOD_E_NONE;
}

/*Get port cl72 status*/
int plp_aperta2_pm8x100_gen2_port_cl72_status_get(int unit, int port, pm_info_t pm_info,
                                      plp_aperta2_phymod_cl72_status_t* status)
{
    int nof_phys;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_MEMSET(&params , 0, sizeof(portmod_access_get_params_t));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN
            (plp_aperta2_phymod_tscp_driver.f_phymod_phy_cl72_status_get(&phy_access, status));


    return PHYMOD_E_NONE;
}

int plp_aperta2_portmod_commmon_portmod_to_phymod_loopback_type(int unit, portmod_loopback_mode_t loopback_type, plp_aperta2_phymod_loopback_mode_t *phymod_lb_type)
{
    switch(loopback_type){
    case portmodLoopbackPhyGloopPCS:
        *phymod_lb_type = phymodLoopbackGlobal;
        break;
    case portmodLoopbackPhyGloopPMD:
        *phymod_lb_type = phymodLoopbackGlobalPMD;
        break;
    case portmodLoopbackPhyRloopPCS:
        *phymod_lb_type = phymodLoopbackRemotePCS;
        break;
    case portmodLoopbackPhyRloopPMD:
        *phymod_lb_type = phymodLoopbackRemotePMD;
        break;
    default:
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, ("unsupported loopback type %d", loopback_type));
    }
    return PHYMOD_E_NONE;
}



/*Port loopback set\get*/
int plp_aperta2_pm8x100_gen2_port_loopback_set(int unit, int port, pm_info_t pm_info,
                                   portmod_loopback_mode_t loopback_type, int enable)
{
    int nof_phys;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    uint32_t phymod_lb_type=0;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_MEMSET(&params , 0, sizeof(portmod_access_get_params_t));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                             &params, 1, &phy_access, &nof_phys, NULL));
   

    switch (loopback_type) {
        case portmodLoopbackPhyGloopPMD:
        case portmodLoopbackPhyRloopPMD:
        case phymodLoopbackGlobalPMD:
        case phymodLoopbackRemotePMD:
        case phymodLoopbackRemotePCS:
            if (loopback_type == portmodLoopbackPhyRloopPCS || loopback_type == portmodLoopbackPhyRloopPMD
                    || loopback_type == portmodLoopbackPhyGloopPMD) {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_commmon_portmod_to_phymod_loopback_type(unit,
                             loopback_type, &phymod_lb_type));
            } else {
                /* coverity[mixed_enums] */
                phymod_lb_type = loopback_type;
            }
            PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_loopback_set(&phy_access, phymod_lb_type, enable));

            PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_loopback_set(&phy_access, phymodLoopbackRemotePMD, enable));
           break;
        case portmodLoopbackMacOuter:
        default:
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_UNAVAIL, (
                      SOC_MSG("unsupported loopback type %d"), loopback_type));
           break;
    }


    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_loopback_get(int unit, int port, pm_info_t pm_info,
                                   portmod_loopback_mode_t loopback_type,
                                   int* enable)
{
    portmod_access_get_params_t params;
    int nof_phys;
    plp_aperta2_phymod_phy_access_t phy_access;
    uint32_t lpbk_en = 0, phymod_lb_type = 0;

    PHYMOD_MEMSET(&params , 0, sizeof(portmod_access_get_params_t));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                             &params, 1, &phy_access, &nof_phys, NULL));
    switch (loopback_type) {
        case portmodLoopbackPhyRloopPCS:
        case portmodLoopbackPhyRloopPMD:
        case portmodLoopbackPhyGloopPMD:
        case phymodLoopbackGlobalPMD:
        case phymodLoopbackRemotePMD:
        case phymodLoopbackRemotePCS:
            if (loopback_type == portmodLoopbackPhyRloopPCS || loopback_type == portmodLoopbackPhyRloopPMD
                    || loopback_type == portmodLoopbackPhyGloopPMD) {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_commmon_portmod_to_phymod_loopback_type(unit,
                            loopback_type, &phymod_lb_type));
            } else {
                /* coverity[mixed_enums] */
                phymod_lb_type = loopback_type;
            }

            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_phymod_tscp_driver.f_phymod_phy_loopback_get(&phy_access, phymod_lb_type, &lpbk_en));
        default:
            break;
    }
    *enable = lpbk_en;
    return PHYMOD_E_NONE;
}

/*Port RX MAC ENABLE set\get*/
int plp_aperta2_pm8x100_gen2_port_rx_mac_enable_set(int unit, int port,
                                        pm_info_t pm_info, int enable)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys=0;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                             NULL, 1, &phy_access, &nof_phys, NULL));
   

    return  plp_aperta2_dc3mac_rx_enable_set(&phy_access, enable);
}

int plp_aperta2_pm8x100_gen2_port_rx_mac_enable_get(int unit, int port,
                                        pm_info_t pm_info, int* enable)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys=0;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                             NULL, 1, &phy_access, &nof_phys, NULL));
    return plp_aperta2_dc3mac_rx_enable_get(&phy_access, (uint32_t *)enable);
}

/*Port TX MAC ENABLE set\get*/
int plp_aperta2_pm8x100_gen2_port_tx_mac_enable_set(int unit, int port,
                                        pm_info_t pm_info,
                                        int enable)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys=0;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                             NULL, 1, &phy_access, &nof_phys, NULL));

    return plp_aperta2_dc3mac_tx_enable_set(&phy_access, enable);
}

int plp_aperta2_pm8x100_gen2_port_tx_mac_enable_get(int unit, int port,
                                        pm_info_t pm_info,
                                        int* enable)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys=0;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                             NULL, 1, &phy_access, &nof_phys, NULL));

    return plp_aperta2_dc3mac_tx_enable_get(&phy_access, (uint32_t *)enable);
}

/*
 * This API looks at the current port's number of lanes and max speed,
 * returns the complete list of force-speed abilities with the same number
 * of lanes and speed that is <= max speed, and the complete list of autoneg
 * abilities with number of lanes <= current port's number of lanes and
 * speed <= max speed.
 */
int plp_aperta2_pm8x100_gen2_port_speed_ability_local_get(int unit, int port, pm_info_t pm_info,
                                              int max_num_abilities,
                                              portmod_port_speed_ability_t* abilities,
                                              int* num_abilities)
{
    /* not used for aperta2*/

    return PHYMOD_E_NONE;
}

/* Translate portmod_port_speed_ability_t to phymod_autoneg_advert_abilities_t */
int _plp_aperta2_pm8x100_gen2_port_port_to_phy_ability(int num_abilities,
                                           const portmod_port_speed_ability_t* abilities,
                                           phymod_autoneg_advert_abilities_t* an_advert_abilities)
{
    int i;

    an_advert_abilities->num_abilities = num_abilities;
    for (i = 0; i < num_abilities; i++) {
        an_advert_abilities->autoneg_abilities[i].speed = abilities[i].speed;
        an_advert_abilities->autoneg_abilities[i].resolved_num_lanes = abilities[i].num_lanes;
        switch (abilities[i].fec_type) {
            case PORTMOD_PORT_PHY_FEC_NONE:
                an_advert_abilities->autoneg_abilities[i].fec = phymod_fec_None;
                break;
            case PORTMOD_PORT_PHY_FEC_RS_FEC:
                an_advert_abilities->autoneg_abilities[i].fec = phymod_fec_CL91;
                break;
            case PORTMOD_PORT_PHY_FEC_RS_544:
                an_advert_abilities->autoneg_abilities[i].fec = phymod_fec_RS544;
                break;
            case PORTMOD_PORT_PHY_FEC_RS_544_2XN:
                an_advert_abilities->autoneg_abilities[i].fec = phymod_fec_RS544_2XN;
                break;
            case PORTMOD_PORT_PHY_FEC_RS_272:
                an_advert_abilities->autoneg_abilities[i].fec = phymod_fec_RS272;
                break;
            case PORTMOD_PORT_PHY_FEC_RS_272_2XN:
                an_advert_abilities->autoneg_abilities[i].fec = phymod_fec_RS272_2XN;
                break;
            default:
                 return PHYMOD_E_CONFIG;
        }
        switch (abilities[i].pause) {
            case PORTMOD_PORT_PHY_PAUSE_NONE:
                an_advert_abilities->autoneg_abilities[i].pause = phymod_pause_none;
                break;
            case PORTMOD_PORT_PHY_PAUSE_TX:
                an_advert_abilities->autoneg_abilities[i].pause = phymod_pause_asym;
                break;
            case PORTMOD_PORT_PHY_PAUSE_RX:
                an_advert_abilities->autoneg_abilities[i].pause = phymod_pause_asym_symm;
                break;
            case PORTMOD_PORT_PHY_PAUSE_SYMM:
                an_advert_abilities->autoneg_abilities[i].pause = phymod_pause_symm;
                break;
            default:
                return PHYMOD_E_CONFIG;
        }
        switch (abilities[i].an_mode) {
            case PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73:
                an_advert_abilities->autoneg_abilities[i].an_mode = phymod_AN_MODE_CL73;
                break;
            case PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM:
                an_advert_abilities->autoneg_abilities[i].an_mode = phymod_AN_MODE_CL73BAM;
                break;
            case PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA:
                an_advert_abilities->autoneg_abilities[i].an_mode = phymod_AN_MODE_MSA;
                break;
            default:
                return PHYMOD_E_CONFIG;
        }
        if (abilities[i].medium == PORTMOD_PORT_PHY_MEDIUM_COPPER) {
            an_advert_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
        } else {
            an_advert_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypePcbTraceBackPlane;
        }
        if (abilities[i].channel == PORTMOD_PORT_PHY_CHANNEL_SHORT) {
            an_advert_abilities->autoneg_abilities[i].channel = phymod_channel_short;
        } else {
            an_advert_abilities->autoneg_abilities[i].channel = phymod_channel_long;
        }
    }

    return PHYMOD_E_NONE;
}

/* Translate phymod_autoneg_advert_abilities_t to portmod_port_speed_ability_t */
int _plp_aperta2_pm8x100_gen2_port_phy_to_port_ability(phymod_autoneg_advert_abilities_t* an_advert_abilities,
                                           int* num_abilities,
                                           portmod_port_speed_ability_t* abilities)
{
    int i;

    *num_abilities = an_advert_abilities->num_abilities;
    for (i = 0; i < *num_abilities; i++) {
        abilities[i].speed = an_advert_abilities->autoneg_abilities[i].speed;
        abilities[i].num_lanes = an_advert_abilities->autoneg_abilities[i].resolved_num_lanes;
        switch (an_advert_abilities->autoneg_abilities[i].fec) {
            case phymod_fec_None:
                abilities[i].fec_type = PORTMOD_PORT_PHY_FEC_NONE;
                break;
            case phymod_fec_CL91:
                abilities[i].fec_type = PORTMOD_PORT_PHY_FEC_RS_FEC;
                break;
            case phymod_fec_RS544:
                abilities[i].fec_type = PORTMOD_PORT_PHY_FEC_RS_544;
                break;
            case phymod_fec_RS544_2XN:
                abilities[i].fec_type = PORTMOD_PORT_PHY_FEC_RS_544_2XN;
                break;
            case phymod_fec_RS272:
                abilities[i].fec_type = PORTMOD_PORT_PHY_FEC_RS_272;
                break;
            case phymod_fec_RS272_2XN:
                abilities[i].fec_type = PORTMOD_PORT_PHY_FEC_RS_272_2XN;
                break;
            default:
                 return PHYMOD_E_CONFIG;
        }
        switch (an_advert_abilities->autoneg_abilities[i].pause) {
            case phymod_pause_none:
                abilities[i].pause = PORTMOD_PORT_PHY_PAUSE_NONE;
                break;
            case phymod_pause_asym:
                abilities[i].pause = PORTMOD_PORT_PHY_PAUSE_TX;
                break;
            case phymod_pause_asym_symm:
                abilities[i].pause = PORTMOD_PORT_PHY_PAUSE_RX;
                break;
            case phymod_pause_symm:
                abilities[i].pause = PORTMOD_PORT_PHY_PAUSE_SYMM;
                break;
            default:
                return PHYMOD_E_CONFIG;
        }
        switch (an_advert_abilities->autoneg_abilities[i].an_mode) {
            case phymod_AN_MODE_CL73:
                abilities[i].an_mode = PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73;
                break;
            case phymod_AN_MODE_CL73BAM:
                abilities[i].an_mode = PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM;
                break;
            case phymod_AN_MODE_MSA:
                abilities[i].an_mode = PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA;
                break;
            default:
                return PHYMOD_E_CONFIG;
        }
        if (an_advert_abilities->autoneg_abilities[i].medium == phymodFirmwareMediaTypeCopperCable) {
            abilities[i].medium = PORTMOD_PORT_PHY_MEDIUM_COPPER;
        } else {
            abilities[i].medium = PORTMOD_PORT_PHY_MEDIUM_BACKPLANE;
        }
        if (an_advert_abilities->autoneg_abilities[i].channel == phymod_channel_short) {
            abilities[i].channel = PORTMOD_PORT_PHY_CHANNEL_SHORT;
        } else {
            abilities[i].channel = PORTMOD_PORT_PHY_CHANNEL_LONG;
        }
    }

    return PHYMOD_E_NONE;
}

static int
_plp_aperta2_pm8x100_gen2_an_encap_mode_get(int unit, int port, pm_info_t pm_info,
                                int *encap)
{
    portmod_encap_t tmp_encap;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys=0;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                             NULL, 1, &phy_access, &nof_phys, NULL));
   

    /*
     * PM8x100 auto-negotiation(AN) only support two encap mode in AN:
     * PM_ENCAP_IEEE or PM_ENCAP_HG3.
     * Within each PM, HiGig3 and IEEE mode can not co-exist in AN.
     * If there's any AN port using HiGig3 encap mode, PM is in HiGig3 AN mode.
     * If there's any AN port using IEEE encap mode, PM is in IEEE AN mode.
     * If no AN port within the PML, user can choose AN encap mode freely.
     */
    /* Get the first physical port of the pm core */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_encap_get(&phy_access, &tmp_encap));
    if (tmp_encap == 0/*DC3MAC_HDR_MODE_IEEE*/) {
        *encap = 0/*DC3MAC_HDR_MODE_IEEE*/;
        return PHYMOD_E_NONE;
    }
    *encap = -1;
    return PHYMOD_E_NONE;
}

/*
 * Function:
 *   plp_aperta2_pm8x100_gen2_port_advert_abilities_validate
 * Purpose:
 *   Validate AN advertisement based on:
 *      1. PM support list.
 *      2. Protocol limitation.
 *      3. VCO limitation.
 *      4. FEC arch restriction.
 *      5. Encap support.
 *   If validation passes, return requested
 *   an_mode, FEC arch and VCO.
 */
int
plp_aperta2_pm8x100_gen2_port_advert_abilities_validate(int unit, int port, pm_info_t pm_info,
                                           int num_abilities,
                                           const portmod_port_speed_ability_t *abilities,
                                           plp_aperta2_phymod_an_mode_type_t *an_mode,
                                           portmod_fec_t *rs_fec_req)  /* portmod fec type or phymod fec type */
{
    int idx, is_dual_50g, port_num_lanes;
    int rs544_req = 0, rs272_req = 0;
    int is_bam = 0, is_msa = 0;
    uint8_t rs544_bm = 0, rs272_bm = 0, affected_bm = 0;
    int ieee_100G_single_lane_request = 0;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys, i, port_index, mpp_index;
    uint32_t bitmap;
    int tvco_pll_active_lane_bm, tvco_pll_adv_lane_bm;
    portmod_vco_type_t vco, current_vco;
    portmod_fec_t  fec_cl73bam_msa;
    int current_media_type;
    phymod_firmware_lane_config_t fw_lane_config;
    portmod_encap_t port_encap;
    int pm_encap_mode;
    int cur_an_mode, octal = 0;

    

    if (num_abilities == 0) {
        return PHYMOD_E_PARAM;
    }

    fec_cl73bam_msa = PORTMOD_PORT_PHY_FEC_INVALID;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    port_num_lanes = plp_aperta2_phymod_count_set_bits(phy_access.access.lane_mask);

    current_media_type = phymodFirmwareMediaTypeCount;
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_phymod_phy_firmware_lane_config_get(&phy_access, &fw_lane_config));

    if (fw_lane_config.MediaType == phymodFirmwareMediaTypePcbTraceBackPlane) {
        current_media_type = PORTMOD_PORT_PHY_MEDIUM_BACKPLANE;
    } else if (fw_lane_config.MediaType == phymodFirmwareMediaTypeCopperCable) {
        current_media_type = PORTMOD_PORT_PHY_MEDIUM_COPPER;
    }

    /*
     * If the port is the only active port on the PM, and the port is not
     * running AN, there's no VCO restriction.
     * Otherwise, current VCO can not be changed.
     */
    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_active_lane_bm);
    aperta2_PM8x100_GEN2_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info,tvco_pll_adv_lane_bm);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, &bitmap));

    if ((tvco_pll_active_lane_bm == phy_access.access.lane_mask)
         && !(tvco_pll_adv_lane_bm & phy_access.access.lane_mask)) {
        current_vco = portmodVCOInvalid;
    } else {
        octal = APERTA2_GET_OCTAL( phy_access.access.lane_mask);
        if (octal == APERTA2_PM_OCTAL1) { /* Octal 1*/
            if (phy_access.port_loc == phymodPortLocLine) {
                current_vco = PM_8x100_GEN2_INFO(pm_info)->tvco;
            } else {
                current_vco = PM_8x100_GEN2_INFO(pm_info)->sys_tvco;
            }
        } else {          /* Octal 2*/
            if (phy_access.port_loc == phymodPortLocLine) {
                current_vco = PM_8x100_GEN2_INFO(pm_info)->oc1_tvco;
            } else {
                current_vco = PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco;
            }
        }
    }

    for (idx = 0; idx < num_abilities; idx++) {
        is_dual_50g = 0;

        /* 1. AN ability is supported by HW. */
        /* 1.1 AN ability is in the port's support list. */
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_pm8x100_gen2_an_ability_table_vco_get(abilities[idx].speed, abilities[idx].num_lanes,
                                                   abilities[idx].fec_type, abilities[idx].an_mode, &vco));

        /* 1.2 AN ability is supported by current VCO. */
        if (current_vco == portmodVCOInvalid) {
            current_vco = vco;
        } else if (current_vco != vco) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                (SOC_MSG("Can not accommodate VCO settings.\n request VCO %d, current VCO %d\n"),
                 vco, current_vco));
        }

        /*
         * 1.3 Number of lane request in the abilies does not exceed the
         * number of physical lanes of the logical port.
         */
        if (abilities[idx].num_lanes > (uint32_t)port_num_lanes) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                (SOC_MSG("port %d: Number of lane request in the abilies does not exceed the number of physical lanes of the port."), port));
        }

        if ((abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_RS_544) ||
            (abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_RS_544_2XN)) {
            rs544_req = 1;
        } else if ((abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_RS_272) ||
                   (abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_RS_272_2XN)) {
            rs272_req = 1;
        }

        /* 2. AN abilities do not conflict with each other. */
        /* 2.1 Pause, medium do not conflict among abilities. */
        if (abilities[idx].pause != abilities[0].pause) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                               (SOC_MSG("port %d: Pause conflicts among abilities."), port));
        }
        if (abilities[idx].medium != current_media_type) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                               (SOC_MSG("port %d: Medium conflicts with current media type."), port));
        }

        if ((abilities[idx].speed == 50000) &&
            (abilities[idx].num_lanes == 2) &&
            (abilities[idx].fec_type != PORTMOD_PORT_PHY_FEC_RS_544)) {
            /* Set flag indicating current ability is dual lane 50G. */
            is_dual_50g = 1;
        }

        /* both 100G 1 lane 544 and 544 2XN can not co-exists */
        if ((abilities[idx].speed == 100000) &&
            (abilities[idx].num_lanes == 1)) {
            if (ieee_100G_single_lane_request == 0) {
                ieee_100G_single_lane_request = 1;
            } else {
                return (PHYMOD_E_FAIL);
            }
        }

        /* 2.2 Check FEC conflicts for each AN mode. */
        switch (abilities[idx].an_mode) {
            case PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM:
                is_bam = 1;

                /*
                 * Speeds 50G-2lane share the same RS528 FEC
                 * bit in BAM. Need to verify if there's any conflict among
                 * these abilities.
                 */
                if (is_dual_50g) {
                    if ((fec_cl73bam_msa == PORTMOD_PORT_PHY_FEC_NONE) &&
                        (abilities[idx].fec_type != PORTMOD_PORT_PHY_FEC_NONE)) {
                        /*
                         * Previous abilities request FEC_NONE, while current
                         * ability request other FEC type.
                         */
                        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                                           (SOC_MSG("port %d: FEC conflicts among abilities."), port));
                    } else if ((abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_NONE) &&
                               (fec_cl73bam_msa == PORTMOD_PORT_PHY_FEC_RS_FEC)) {
                        /*
                         * Current ability request FEC_NONE, while previous
                         * abilities request other FEC type.
                         */
                        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                                           (SOC_MSG("port %d: FEC conflicts among abilities."), port));
                    } else {
                        /* No FEC conflicts. Record current FEC request. */
                        fec_cl73bam_msa = abilities[idx].fec_type;
                    }
                }
                break;
            case PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA:
                is_msa = 1;
                if (is_dual_50g) {
                    if ((fec_cl73bam_msa == PORTMOD_PORT_PHY_FEC_NONE) &&
                        (abilities[idx].fec_type != PORTMOD_PORT_PHY_FEC_NONE)) {
                        /*
                         * Previous abilities request FEC_NONE, while current
                         * ability request other FEC type.
                         */
                        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                                           (SOC_MSG("port %d: FEC conflicts among abilities."), port));
                    } else if ((abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_NONE) &&
                               (fec_cl73bam_msa == PORTMOD_PORT_PHY_FEC_RS_FEC)) {
                        /*
                         * Current ability request FEC_NONE, while previous
                         * abilities request other FEC type.
                         */
                        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                                           (SOC_MSG("port %d: FEC conflicts among abilities."), port));
                    } else {
                        /* No FEC conflicts. Record current FEC request. */
                        fec_cl73bam_msa = abilities[idx].fec_type;
                    }
                }
                break;
            case PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73:
                break;
            default:
                APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                                   (SOC_MSG("port %d: Invalid AN mode %d."), port, abilities[idx].an_mode));
        }
    }

    /*
     * 2.3 Verify an_mode do not conflicts with each other.
     * CL73BAM and MSA can not be advertised at the same time.
     */
    if (is_bam && is_msa) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                           (SOC_MSG("port %d: an_mode conflicts among abilities."
                                     "Cannot advertise CL73MSA and CL73BAM at the same time."), port));
    } else if (is_bam) {
        *an_mode = phymod_AN_MODE_CL73BAM;
    } else if (is_msa) {
        *an_mode = phymod_AN_MODE_CL73_MSA;
    } else {
        *an_mode = phymod_AN_MODE_CL73;
    }

    /*
     * 2.4 Verify an_mode that CL73BAM and MSA does not co-exist
     * on the same MPP.
     */
    mpp_index = (int)(port_index / 4);
    for (i = (0 + (mpp_index * 4)); i < (4 + (mpp_index*4)); i++) {
        if (i == port_index) {
            continue;
        }
        aperta2_PM8x100_GEN2_AN_MODE_GET(unit, pm_info, cur_an_mode, i);
        if ((*an_mode == phymod_AN_MODE_CL73_MSA) &&
            (cur_an_mode == phymod_AN_MODE_CL73BAM)) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                (SOC_MSG("port %d: CL73_MSA cannot be advertised when current MPP is in CL73_BAM."), port));
        }
        if ((*an_mode == phymod_AN_MODE_CL73BAM) &&
            (cur_an_mode == phymod_AN_MODE_CL73_MSA)) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                (SOC_MSG("port %d: CL73_BAM cannot be advertised when current MPP is in CL73_MSA."), port));
        }
    }

    /* 3. FEC arch can be supported within each MPP. */
    if (rs544_req || rs272_req) {
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_fec_lanebitmap_get(unit, pm_info, &rs544_bm, &rs272_bm));
        rs544_bm &= ~phy_access.access.lane_mask;
        rs272_bm &= ~phy_access.access.lane_mask;
        if (rs544_req) {
            rs544_bm |= phy_access.access.lane_mask;
        }
        if (rs272_req) {
            rs272_bm |= phy_access.access.lane_mask;
        }
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_fec_validate(unit, rs544_bm, rs272_bm, &affected_bm));
        if (rs544_req) {
            *rs_fec_req = PORTMOD_PORT_PHY_FEC_RS_544;
        } else if (rs272_req) {
            *rs_fec_req = PORTMOD_PORT_PHY_FEC_RS_272;
        }
    } else {
        *rs_fec_req = PORTMOD_PORT_PHY_FEC_NONE;
    }

    /*
     *  4. PM encap mode can support current port's request.
     *  If the given port has AN enabled with a different encap mode,
     *  This function will reject the encap mode change. User need to
     *  disable AN on the port before they switch the encap mode.
     */
    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_pm8x100_gen2_an_encap_mode_get(unit, port, pm_info,
                            &pm_encap_mode));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_dc3mac_encap_get(&phy_access, &port_encap));
    if ((pm_encap_mode != -1) && (pm_encap_mode != (int)port_encap)) {
        PHYMOD_IF_ERR_RETURN(PHYMOD_E_FAIL);
    }


    return PHYMOD_E_NONE;
}

/*Set/get port auto negotiation ability*/
int plp_aperta2_pm8x100_gen2_port_autoneg_ability_advert_set(int unit, int port, pm_info_t pm_info,
                                                 int num_abilities,
                                                 const portmod_port_speed_ability_t* abilities)
{
    plp_aperta2_phymod_an_mode_type_t an_mode = phymod_AN_MODE_NONE;
    phymod_autoneg_advert_ability_t
                                  autoneg_abilities[aperta2_PM8x100_GEN2_MAX_AN_ABILITY];
    phymod_autoneg_advert_abilities_t an_advert_abilities;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    phymod_phy_pll_state_t old_pll_state, new_pll_state;
    int port_index, idx;
    portmod_fec_t rs_fec_req = PORTMOD_PORT_PHY_FEC_NONE;
    uint32_t bitmap;

    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_MEMSET(autoneg_abilities, 0,
          aperta2_PM8x100_GEN2_MAX_AN_ABILITY * sizeof(phymod_autoneg_advert_ability_t));

    /* 1. Validate abilities. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_pm8x100_gen2_port_advert_abilities_validate(unit, port,
                                                pm_info, num_abilities,
                                                abilities, &an_mode,
                                                &rs_fec_req));
    an_advert_abilities.autoneg_abilities = autoneg_abilities;
    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_pm8x100_gen2_port_port_to_phy_ability(num_abilities, abilities, &an_advert_abilities));

    /* 2. Set AN abilities in HW. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_tscp_driver.f_phymod_phy_autoneg_advert_ability_set(&phy_access,
                                               &an_advert_abilities,
                                               &old_pll_state,
                                               &new_pll_state));

    /* 3. Update DB. */
    /* 3.1 Update an_mode. */
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, &bitmap));
    aperta2_PM8x100_GEN2_AN_MODE_SET(unit, pm_info, an_mode, port_index);

    /* 3.2 Update FEC type. */
    for (idx = 0 ; idx < APERTA2_PM8X100_GEN2_LANES_PER_CORE; idx++) {
        if ((phy_access.access.lane_mask >> idx) & 0x1) {
            aperta2_PM8x100_GEN2_LANE2FEC_SET(unit, pm_info, idx, rs_fec_req);
        }
    }


    return PHYMOD_E_NONE;

}

int plp_aperta2_pm8x100_gen2_port_autoneg_ability_advert_get(int unit, int port, pm_info_t pm_info,
                                                 int max_num_abilities,
                                                 portmod_port_speed_ability_t* abilities,
                                                 int* actual_num_abilities)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys, i;
    phymod_autoneg_advert_ability_t autoneg_abilities[aperta2_PM8x100_GEN2_MAX_AN_ABILITY];
    phymod_autoneg_advert_abilities_t an_advert_abilities;

    

    for (i = 0; i < aperta2_PM8x100_GEN2_MAX_AN_ABILITY; i++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_autoneg_advert_ability_t_init(&autoneg_abilities[i]));
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    an_advert_abilities.autoneg_abilities = autoneg_abilities;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_autoneg_advert_ability_get(&phy_access, &an_advert_abilities));

    if (an_advert_abilities.num_abilities > max_num_abilities) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("port %d: There are %d AN abilities. Larger array is needed."),
                           port, an_advert_abilities.num_abilities));
    } else {
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_phy_to_port_ability(&an_advert_abilities, actual_num_abilities, abilities));
    }


    return PHYMOD_E_NONE;
}

/*Port ability remote Adv get*/
int plp_aperta2_pm8x100_gen2_port_autoneg_ability_remote_get(int unit, int port, pm_info_t pm_info,
                                                 int max_num_abilities,
                                                 portmod_port_speed_ability_t* abilities,
                                                 int* actual_num_abilities)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys, i;
    phymod_autoneg_advert_ability_t autoneg_abilities[aperta2_PM8x100_GEN2_MAX_AN_ABILITY];
    phymod_autoneg_advert_abilities_t an_advert_abilities;

    

    for (i = 0; i < aperta2_PM8x100_GEN2_MAX_AN_ABILITY; i++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_autoneg_advert_ability_t_init(&autoneg_abilities[i]));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    an_advert_abilities.autoneg_abilities = autoneg_abilities;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_autoneg_remote_advert_ability_get(&phy_access, &an_advert_abilities));

    if (an_advert_abilities.num_abilities > max_num_abilities) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("port %d: There are %d AN abilities. Larger array is needed."),
                           port, an_advert_abilities.num_abilities));
    } else {
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_phy_to_port_ability(&an_advert_abilities, actual_num_abilities, abilities));
    }


    return PHYMOD_E_NONE;
}

/*Set\Get autoneg*/
int plp_aperta2_pm8x100_gen2_port_autoneg_set(int unit, int port, pm_info_t pm_info,
                                  uint32_t phy_flags,
                                  const plp_aperta2_phymod_autoneg_control_t* an)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys, port_num_lanes, speed_id_table_status = 0;

    plp_aperta2_phymod_autoneg_control_t an_control;
    portmod_port_speed_ability_t abilities[aperta2_PM8x100_GEN2_MAX_AN_ABILITY];
    int port_index, flags = 0, actual_num_abilities, pm_id = 0;
    uint32_t idx, lane_mask_backup, bitmap;
    uint8_t rs544_req = 0, rs272_req = 0;
    portmod_vco_type_t req_vco = portmodVCOInvalid, current_vco = portmodVCOInvalid;
    portmod_encap_t encap;
    int pll_lanes_bitmap;
    int octal = 0;

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, &bitmap));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    port_num_lanes = plp_aperta2_phymod_count_set_bits(phy_access.access.lane_mask);
    lane_mask_backup = phy_access.access.lane_mask;
    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
    an_control = *an;

    an_control.num_lane_adv = port_num_lanes;

    if (an_control.an_mode == phymod_AN_MODE_NONE) {
        /* Return error only when user want to enable AN without any AN abilities being advertised. */
        if (!an_control.enable) {
            an_control.an_mode = phymod_AN_MODE_CL73;
        } else {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_CONFIG,
                               (SOC_MSG("port %d: There's no valid AN advertisement for the port."), port));
        }
    }


    octal = APERTA2_GET_OCTAL( phy_access.access.lane_mask);
    if (an_control.enable) {
        /* 1. Check if VCO change is needed. */
        /* 1.1 Get current VCO rate. */
        if (octal == APERTA2_PM_OCTAL1) { /* Octal 1*/
            if (phy_access.port_loc == phymodPortLocLine) {
                current_vco = PM_8x100_GEN2_INFO(pm_info)->tvco;
            } else {
                current_vco = PM_8x100_GEN2_INFO(pm_info)->sys_tvco;
            }
        } else {          /* Octal 2*/
            if (phy_access.port_loc == phymodPortLocLine) {
                current_vco = PM_8x100_GEN2_INFO(pm_info)->oc1_tvco;
            } else {
                current_vco = PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco;
            }
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_pm8x100_gen2_port_autoneg_ability_advert_get(unit, port, pm_info,
                                                 aperta2_PM8x100_GEN2_MAX_AN_ABILITY,
                                                 abilities,
                                                 &actual_num_abilities));

        /* 1.2 Get the VCO requirement for AN advertisement. */
        if (actual_num_abilities == 0) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_CONFIG,
                               (SOC_MSG("port %d: There's no valid ability for the port."), port));
        } else {
            /*
             * All the AN advertisement will use the same VCO rate.
             * So break once we get a valid request VCO value.
             */
            PHYMOD_IF_ERR_RETURN
                (_plp_aperta2_pm8x100_gen2_an_ability_table_vco_get(abilities[0].speed,
                                                                abilities[0].num_lanes,
                                                                abilities[0].fec_type,
                                                                abilities[0].an_mode, &req_vco));
        }

        /* 1.3 Change VCO if needed. */
        if (current_vco != req_vco) {
            if (pll_lanes_bitmap == lane_mask_backup) {
                /* The current port is the only active port on the PM. */
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_pm8x100_gen2_pm_vco_reconfig(unit, pm_id,
                                                 pm_info, &req_vco));
            } else {
                APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_CONFIG,
                               (SOC_MSG("vco rate %d is not available.\n"), req_vco));
            }
        }

        /* 2. Reload SPEED ID table if needed. */
        /*
         * Encap validation is done in aperta2_pm8x100_gen2_port_ability_advert_set.
         * Here just need to make sure the SPEED ID table loaded can serve
         * the given port.
         */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_pm8x100_gen2_port_encap_get(unit, port, pm_info, &flags, &encap));

        aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_GET(unit, pm_info, speed_id_table_status);
        if (encap == 0/*DC3MAC_HDR_MODE_IEEE*/) {
                    /* The port's encap mode is IEEE. */
            if (speed_id_table_status !=
                aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_NOT_LOADED) {
                /*
                 * Reload SPEED ID table for IEEE mode.
                 * Update table loading status in HA info.
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_phymod_tscp_driver.f_phymod_phy_autoneg_speed_id_table_reload(&phy_access, 0));

                 */
                speed_id_table_status = aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_IEEE_MODE_LOADED;
                aperta2_PM8x100_GEN2_SPEED_ID_TABLE_STATUS_SET(unit, pm_info, speed_id_table_status);
            }
        }

        /* 3. Set FEC request flag for Phymod. */
        for (idx = 0; idx < actual_num_abilities; idx++) {
            /* Check FEC requests. */
            if ((abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_RS_544) ||
                (abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_RS_544_2XN)) {
                rs544_req = 1;
            } else if ((abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_RS_272) ||
                       (abilities[idx].fec_type == PORTMOD_PORT_PHY_FEC_RS_272_2XN)) {
                rs272_req = 1;
            }
        }

        if (rs544_req) {
            /*PHYMOD_AN_F_FEC_RS272_CLR_SET(&an_control);*/
        } else if (rs272_req) {
            /*PHYMOD_AN_F_FEC_RS272_REQ_SET(&an_control);*/
        }

    }

    phy_access.access.lane_mask = lane_mask_backup;
    /* Enable AN in Serdes. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_tscp_driver.f_phymod_phy_autoneg_set(&phy_access, &an_control));

    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
    if (an_control.enable) {
        pll_lanes_bitmap |= phy_access.access.lane_mask;
    } else {
        pll_lanes_bitmap &= ~phy_access.access.lane_mask;
    }
    aperta2_PM8x100_GEN2_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);


    return PHYMOD_E_NONE;

}

int plp_aperta2_pm8x100_gen2_port_autoneg_get(int unit, int port, pm_info_t pm_info,
                                  uint32_t phy_flags,
                                  plp_aperta2_phymod_autoneg_control_t* an)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    uint32_t an_done;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_autoneg_get(&phy_access, an, &an_done));
    return PHYMOD_E_NONE;
}

/*Get autoneg status*/
int plp_aperta2_pm8x100_gen2_port_autoneg_status_get(int unit, int port, pm_info_t pm_info,
                                         plp_aperta2_phymod_autoneg_status_t* an_status)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;

    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_autoneg_status_get(&phy_access, an_status));
    if (!(an_status->enabled && an_status->locked)) {
        /* upper layer should not rely on the
         * data rate if the AN is not locked
         */
        an_status->data_rate = 0;
        an_status->interface = phymodInterfaceBypass;
    }



    return PHYMOD_E_NONE;
}
#if 0 /* Will add it when needed*/
/* link up event */
/* 1. AN ports: Update port mode based on reslved speed mode.
 *              Program resolved Pause settings.
 * 2. 1588 ports: timesync adjust.
 * 3. register callback function if BER WAR is applicable
 */
int aperta2_pm8x100_gen2_port_phy_link_up_event(int unit, int port, pm_info_t pm_info)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access, phy_copy;
    int nof_phys, port_index = -1;
    uint32_t timesync_config, bitmap;
    int rx_pause, tx_pause, flags = 0;
    uint32_t idx;
    plp_aperta2_phymod_autoneg_status_t an_status;
    int num_advert, num_remote;
    portmod_port_speed_ability_t advert_ability[aperta2_PM8x100_GEN2_MAX_AN_ABILITY];
    portmod_port_speed_ability_t remote_ability[aperta2_PM8x100_GEN2_MAX_AN_ABILITY];
    portmod_port_phy_pause_t pause_local, pause_remote;
    portmod_pause_control_t pause_ctrl;
    portmod_encap_t port_encap;
    phymod_timesync_adjust_config_info_t timesync_adjust_config;

    

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, &bitmap));

    /* 1. 1588 ports: Timesync adjust. */
    PHYMOD_IF_ERR_RETURN
        (aperta2_PM8x100_GEN2_TIMESYNC_CONFIG_GET(unit, pm_info, &timesync_config, port_index));
    if (PORTMOD_PORT_TIMESYNC_CONFIG_ENABLE_GET(timesync_config)) {
        /* Set flags to disable TS_UPDATE_EN. */
        flags = 0;
        PHYMOD_TIMESYNC_ENABLE_F_RX_SET(flags);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_phymod_timesync_enable_set(&phy_access, flags, 0));

        PHYMOD_IF_ERR_RETURN
                (phymod_timesync_adjust_config_info_t_init(&timesync_adjust_config));

        /*
         * If 1588 is enabled on the port, need to adjust timesync during
         * link up event.
         * rx_ts_update will be enabled at the end of adjust function.
         */
        if (PORTMOD_PORT_TIMESYNC_CONFIG_COMP_MODE_LATEST_LANE_GET(timesync_config)){
            timesync_adjust_config.am_norm_mode = phymodTimesyncCompensationModeLatestlane;
        } else {
            timesync_adjust_config.am_norm_mode = phymodTimesyncCompensationModeEarliestLane;
        }

        /*
         * TODO : 1588_IMPLEMENTATION: phymod_timesync_adjust_set
         */

        flags = 0;
        PHYMOD_TIMESYNC_ENABLE_F_RX_SET(flags);
        /* Set flags for timestamp adjust. */
        if (PORTMOD_PORT_TIMESYNC_CONFIG_SFD_GET(timesync_config)) {
            PHYMOD_TIMESYNC_F_SOP_SET(flags);
        }
        if (PORTMOD_PORT_TIMESYNC_CONFIG_MAC_DA_GET(timesync_config)) {
            PHYMOD_TIMESYNC_F_MAC_DA_SET(flags);
        }

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_pm8x100_gen2_port_encap_get(unit, port, pm_info, &flags, &port_encap));

        /*
         * Both SOC_ENCAP_HIGIG3 and BCMPMAC_ENCAP_IEEE_REDUCED_IPG
         * set the same mode in DC3MAC_MODRr.HDR_MODE.
         * Checking ENCAP_HG3 type here, which means REDUCED_PREAMBLE mode is set.
         */
        if (port_encap == SOC_ENCAP_HIGIG3) {
            PHYMOD_TIMESYNC_F_REDUCED_PREAMBLE_MODE_SET(flags);
        }

        PHYMOD_IF_ERR_RETURN
            (phymod_timesync_adjust_set(&phy_access, flags, &timesync_adjust_config));

        /* Set flags to enable TS_UPDATE_EN. */
        flags = 0;
        PHYMOD_TIMESYNC_ENABLE_F_RX_SET(flags);
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_phymod_timesync_enable_set(&phy_access, flags, 1));
        /* Wait for one AM spacing time. */
        PHYMOD_USLEEP(500);
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_autoneg_status_get(unit, port, pm_info, &an_status));
    if ((an_status.enabled && an_status.locked)) {
        /* 2.1 AN ports: Port mode update. */
        sal_memcpy(&phy_copy, &phy_access, sizeof(phy_copy));
        phy_copy.access.lane_mask = 0;
        /* Update lane mask based on resovled number of lanes. */
        for (idx = 0; idx < an_status.resolved_num_lane; idx++) {
            phy_copy.access.lane_mask |= 0x1 << (port_index + idx);
        }
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_dc3port_port_mode_set(unit, port, flags, phy_copy.access.lane_mask));

        /* 2.2 AN ports: Pause update. */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_pm8x100_gen2_port_autoneg_ability_advert_get(unit, port, pm_info,
                                                 aperta2_PM8x100_GEN2_MAX_AN_ABILITY,
                                                 advert_ability, &num_advert));
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_pm8x100_gen2_port_autoneg_ability_remote_get(unit, port, pm_info,
                                                 aperta2_PM8x100_GEN2_MAX_AN_ABILITY,
                                                 remote_ability, &num_remote));
        if (num_advert && num_remote) {
            pause_local = advert_ability[0].pause;
            pause_remote = remote_ability[0].pause;
            if (
                ((pause_local == PORTMOD_PORT_PHY_PAUSE_RX) &&
                 (pause_remote == PORTMOD_PORT_PHY_PAUSE_TX)) ||
                ((pause_local == PORTMOD_PORT_PHY_PAUSE_RX ||
                  pause_local == PORTMOD_PORT_PHY_PAUSE_SYMM) &&
                 (pause_remote == PORTMOD_PORT_PHY_PAUSE_RX ||
                  pause_remote == PORTMOD_PORT_PHY_PAUSE_SYMM))
               ) {
                rx_pause = 1;
            } else {
                rx_pause = 0;
            }
            if (
                ((pause_local == PORTMOD_PORT_PHY_PAUSE_RX ||
                  pause_local == PORTMOD_PORT_PHY_PAUSE_SYMM) &&
                 (pause_remote == PORTMOD_PORT_PHY_PAUSE_RX ||
                  pause_remote == PORTMOD_PORT_PHY_PAUSE_SYMM)) ||
                ((pause_local == PORTMOD_PORT_PHY_PAUSE_TX) &&
                 (pause_remote == PORTMOD_PORT_PHY_PAUSE_RX))
               ) {
                tx_pause = 1;
            } else {
                tx_pause = 0;
            }
        } else {
            tx_pause = 0;
            rx_pause = 0;
        }

        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_pm8x100_gen2_port_pause_control_get(unit, port, pm_info,
                                                 &pause_ctrl));
        if ((pause_ctrl.rx_enable != rx_pause) ||
            (pause_ctrl.tx_enable != tx_pause)) {
            pause_ctrl.rx_enable = rx_pause;
            pause_ctrl.tx_enable = tx_pause;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_pm8x100_gen2_port_pause_control_set(unit, port, pm_info,
                                                     &pause_ctrl));
        }
    }

    /* Need to do the MAC link down recovery sequence. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_dc3mac_rx_enable_set(unit, port, 0));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_dc3mac_discard_set(unit, port, 1));

    PHYMOD_USLEEP(10000);

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_dc3mac_rx_enable_set(unit, port, 1));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_dc3mac_discard_set(unit, port, 0));


    return PHYMOD_E_NONE;
}
#endif
/*PRBS configuration set/get*/
int plp_aperta2_pm8x100_gen2_port_prbs_config_set(int unit, int port, pm_info_t pm_info,
                                      portmod_prbs_mode_t mode, int flags,
                                      const plp_aperta2_phymod_prbs_t* config)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    /* MAC */
    if(mode == 1) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_diagnostics_tscp_diagnostics_driver.f_phymod_phy_prbs_config_set(&phy_access, flags, config));


    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_prbs_config_get(int unit, int port, pm_info_t pm_info,
                                      portmod_prbs_mode_t mode, int flags,
                                      plp_aperta2_phymod_prbs_t* config)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    /* MAC */
    if(mode == 1) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_diagnostics_tscp_diagnostics_driver.f_phymod_phy_prbs_config_get(&phy_access, flags, config));


    return PHYMOD_E_NONE;
}

/*PRBS enable set/get*/
int plp_aperta2_pm8x100_gen2_port_prbs_enable_set(int unit, int port, pm_info_t pm_info,
                                      portmod_prbs_mode_t mode, int flags,
                                      int enable)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    /* MAC */
    if(mode == 1) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_diagnostics_tscp_diagnostics_driver.f_phymod_phy_prbs_enable_set(&phy_access, flags, enable));


    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_prbs_enable_get(int unit, int port, pm_info_t pm_info,
                                      portmod_prbs_mode_t mode, int flags,
                                      int* enable)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    /* MAC */
    if(mode == 1) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_diagnostics_tscp_diagnostics_driver.f_phymod_phy_prbs_enable_get(&phy_access, flags,
                                                (uint32_t *) enable));


    return PHYMOD_E_NONE;
}
/*PRBS status get*/
int plp_aperta2_pm8x100_gen2_port_prbs_status_get(int unit, int port, pm_info_t pm_info,
                                      portmod_prbs_mode_t mode, int flags,
                                      plp_aperta2_phymod_prbs_status_t* status)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    /* MAC */
    if(mode == 1) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_diagnostics_tscp_diagnostics_driver.f_phymod_phy_prbs_status_get(&phy_access, flags, status));


    return PHYMOD_E_NONE;
}

/*Port tx taps set\get*/
int plp_aperta2_pm8x100_gen2_port_tx_set(int unit, int port, pm_info_t pm_info, const plp_aperta2_phymod_tx_t* tx)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_tx_set(&phy_access, tx));


    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_tx_get(int unit, int port, pm_info_t pm_info, plp_aperta2_phymod_tx_t* tx)
{
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_tx_get(&phy_access, tx));


    return PHYMOD_E_NONE;
}

/*Number of lanes get*/
int plp_aperta2_pm8x100_gen2_port_nof_lanes_get(int unit, int port, pm_info_t pm_info,
                                    int* nof_lanes)
{
    int port_index;
    uint32_t bitmap, bcnt = 0;

    

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, &bitmap));

    while (bitmap) {
        if (bitmap & 0x1) bcnt++;
        bitmap >>= 1;
    }
    *nof_lanes = bcnt;


    return PHYMOD_E_NONE;
}

/*Filter packets smaller than the specified threshold*/
int plp_aperta2_pm8x100_gen2_port_runt_threshold_set(int unit, int port,
                                         pm_info_t pm_info,
                                         int value)
{
    return (PHYMOD_E_UNAVAIL);
}

int plp_aperta2_pm8x100_gen2_port_runt_threshold_get(int unit, int port, pm_info_t pm_info,
                                         int* value)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_runt_threshold_get(&phy_access, (uint32_t *)value));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_tx_threshold_set(int unit, int port,
                                       pm_info_t pm_info,
                                       int value)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_tx_threshold_set(&phy_access, value));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_tx_threshold_get(int unit, int port, pm_info_t pm_info,
                                       int* value)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_tx_threshold_get(&phy_access, (uint32_t *)value));

    return PHYMOD_E_NONE;
}

/*Filter packets bigger than the specified value*/
int plp_aperta2_pm8x100_gen2_port_max_packet_size_set(int unit, int port, pm_info_t pm_info,
                                          int value)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_frame_max_set(&phy_access, value));

    return PHYMOD_E_NONE;
}
int plp_aperta2_pm8x100_gen2_port_max_packet_size_get(int unit, int port, pm_info_t pm_info,
                                          int* value)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_frame_max_get(&phy_access, (uint32_t *)value));

    return PHYMOD_E_NONE;
}

/*set/get the MAC source address that will be sent in case of Pause/LLFC*/
int plp_aperta2_pm8x100_gen2_port_tx_mac_sa_set(int unit, int port, pm_info_t pm_info,
                                    pm_mac_addr_t mac_sa)
{
   
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_tx_mac_sa_set(&phy_access, mac_sa));


    return PHYMOD_E_NONE;
}
int plp_aperta2_pm8x100_gen2_port_tx_mac_sa_get(int unit, int port, pm_info_t pm_info,
                                    pm_mac_addr_t mac_sa)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));


    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_tx_mac_sa_get(&phy_access, mac_sa));


    return PHYMOD_E_NONE;
}

/* set/get SA recognized for MAC control packets
 * in addition to the standard 0x0180C2000001
 */
int plp_aperta2_pm8x100_gen2_port_rx_mac_sa_set(int unit, int port, pm_info_t pm_info,
                                    pm_mac_addr_t mac_sa)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rx_mac_sa_set(&phy_access, mac_sa));
    return PHYMOD_E_NONE;
}
int plp_aperta2_pm8x100_gen2_port_rx_mac_sa_get(int unit, int port, pm_info_t pm_info,
                                    pm_mac_addr_t mac_sa)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rx_mac_sa_get(&phy_access, mac_sa));


    return PHYMOD_E_NONE;
}

/*set/get Average inter-packet gap*/
int plp_aperta2_pm8x100_gen2_port_tx_average_ipg_set(int unit, int port, pm_info_t pm_info,
                                         int value)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_avg_ipg_set(&phy_access, value));


    return PHYMOD_E_NONE;


}
int plp_aperta2_pm8x100_gen2_port_tx_average_ipg_get(int unit, int port, pm_info_t pm_info,
                                         int* value)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_avg_ipg_get(&phy_access, (uint8_t *)value));
    return PHYMOD_E_NONE;

}

/*local fault set/get*/
int plp_aperta2_pm8x100_gen2_port_local_fault_control_set(int unit, int port, pm_info_t pm_info,
                                              const portmod_local_fault_control_t* control)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_local_fault_disable_set(&phy_access, control));


    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_local_fault_control_get(int unit, int port, pm_info_t pm_info,
                                              portmod_local_fault_control_t* control)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_local_fault_disable_get(&phy_access, control));


    return PHYMOD_E_NONE;

}

int plp_aperta2_pm8x100_gen2_port_local_fault_force_set(int unit, int port, pm_info_t pm_info, int enable)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_link_fault_os_set(&phy_access, 0, enable));


    return PHYMOD_E_NONE;
}
int plp_aperta2_pm8x100_gen2_port_local_fault_force_get(int unit, int port, pm_info_t pm_info, int * enable)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_link_fault_os_get(&phy_access, 0, (uint32_t *)enable));


    return PHYMOD_E_NONE;
}

/*remote fault set/get*/
int plp_aperta2_pm8x100_gen2_port_remote_fault_control_set(int unit, int port, pm_info_t pm_info,
                                               const portmod_remote_fault_control_t* control)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_remote_fault_disable_set(&phy_access, control));


    return PHYMOD_E_NONE;

}
int plp_aperta2_pm8x100_gen2_port_remote_fault_control_get(int unit, int port, pm_info_t pm_info,
                                               portmod_remote_fault_control_t* control)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_remote_fault_disable_get(&phy_access, control));


    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_remote_fault_force_set(int unit, int port, pm_info_t pm_info, int enable)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_link_fault_os_set(&phy_access, 1, enable));


    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_remote_fault_force_get(int unit, int port, pm_info_t pm_info, int * enable)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_link_fault_os_get(&phy_access, 1, (uint32_t *)enable));


    return PHYMOD_E_NONE;
}

/*local fault steatus get*/
int plp_aperta2_pm8x100_gen2_port_local_fault_status_get(int unit, int port, pm_info_t pm_info,
                                             int* value)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_local_fault_status_get(&phy_access, value));


    return PHYMOD_E_NONE;
}

/*remote fault status get*/
int plp_aperta2_pm8x100_gen2_port_remote_fault_status_get(int unit, int port, pm_info_t pm_info,
                                              int* value)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_remote_fault_status_get(&phy_access, value));


    return PHYMOD_E_NONE;
}

/*local fault steatus clear, there are no clear register for DC3MAC, just clear by read*/
int plp_aperta2_pm8x100_gen2_port_local_fault_status_clear(int unit, int port, pm_info_t pm_info)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int value;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_local_fault_status_get(&phy_access, &value));


    return PHYMOD_E_NONE;
}

/*remote fault status clear, there are no clear register for DC3MAC, just clear by read*/
int plp_aperta2_pm8x100_gen2_port_remote_fault_status_clear(int unit, int port,
                                                pm_info_t pm_info)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int value;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_remote_fault_status_get(&phy_access, &value));
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_pause_control_set(int unit, int port, pm_info_t pm_info,
                                        const portmod_pause_control_t* control)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));
   
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pause_set(&phy_access, control));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_pause_control_get(int unit, int port, pm_info_t pm_info,
                                        portmod_pause_control_t* control)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pause_get(&phy_access, control));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_pfc_control_set(int unit, int port, pm_info_t pm_info,
                                      const portmod_pfc_control_t* control)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pfc_control_set(&phy_access, control));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_pfc_control_get(int unit, int port, pm_info_t pm_info,
                                portmod_pfc_control_t* control)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pfc_control_get(&phy_access, control));

    return PHYMOD_E_NONE;
}

/*get port cores' phymod access*/
int plp_aperta2_pm8x100_gen2_port_core_access_get(int unit, int port, pm_info_t pm_info,
                                      int phyn, int max_cores,
                                      plp_aperta2_phymod_core_access_t* core_access_arr,
                                      int* nof_cores, int* is_most_ext)
{
    

    /* There are only internal phys(1) on aperta2_PM8x100_GEN2, setting phyn to 0 */
    if(phyn < 0)
    {
        phyn = 0;
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_core_access_t_init(&core_access_arr[0]));

    if( phyn == 0 ){
        PHYMOD_MEMCPY(&core_access_arr[0],
                   &(PM_8x100_GEN2_INFO(pm_info)->int_core_access),
                   sizeof(plp_aperta2_phymod_core_access_t));
        *nof_cores = 1;
    }

    if (is_most_ext) {
            *is_most_ext = 1;
    }


    return PHYMOD_E_NONE;
}

/*Get lane phymod access structure. can be used for per lane operations*/
int plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(int unit, int port, pm_info_t pm_info,
                                          const portmod_access_get_params_t* params,
                                          int max_phys,
                                          plp_aperta2_phymod_phy_access_t* phy_access,
                                          int* nof_phys, int* is_most_ext)
{
    int i;

    if(max_phys > MAX_NUM_CORES)
    {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (SOC_MSG("max_phys parameter exceeded the "
                          "MAX value. max_phys=%d, max allowed %d."),
                          max_phys, MAX_NUM_CORES));
    }

    for( i = 0 ; i < max_phys; i++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_access_t_init(&phy_access[i]));
    }

    *nof_phys = 1;

    /* internal core */
    PHYMOD_MEMCPY (&phy_access[0], &(PM_8x100_GEN2_INFO(pm_info)->int_core_access),
                sizeof(plp_aperta2_phymod_phy_access_t));
    if (phy_access[0].access.lane_mask == 0x0) {
            PHYMOD_DEBUG_ERROR(("%s :: Lane mask is 0\n", __func__));
            return PHYMOD_E_INTERNAL;
    }
    if (phy_access[0].port_loc == phymodPortLocSys) {
        phy_access[0].port_loc = phymodPortLocSys;
    } else {
        phy_access[0].port_loc = phymodPortLocLine;
    }

    if (is_most_ext) {
        *is_most_ext = 1;
    }


    return PHYMOD_E_NONE;
}


/*Port PHY Control register read*/
int plp_aperta2_pm8x100_gen2_port_phy_reg_read(int unit, int port, pm_info_t pm_info, int lane,
                                   int flags, int reg_addr, uint32_t* value)
{
    return PHYMOD_E_UNAVAIL;
}

/*Port PHY Control register write*/
int plp_aperta2_pm8x100_gen2_port_phy_reg_write(int unit, int port, pm_info_t pm_info,
                                    int lane, int flags, int reg_addr, uint32_t value)
{
    return PHYMOD_E_UNAVAIL;
}

/*Port Reset set\get*/
int plp_aperta2_pm8x100_gen2_port_reset_set(int unit, int port,
                                pm_info_t pm_info, int mode,
                                int opcode, int value)
{
    return PHYMOD_E_UNAVAIL;
}
int plp_aperta2_pm8x100_gen2_port_reset_get(int unit, int port,
                                pm_info_t pm_info, int mode,
                                int opcode, int* value)
{
    return PHYMOD_E_UNAVAIL;
}

/*Drv Name Get*/
int plp_aperta2_pm8x100_gen2_port_drv_name_get(int unit, int port,
                             pm_info_t pm_info,
                             char* name, int len)
{
    PHYMOD_STRNCPY(name, "aperta2_PM8x100_GEN2 Driver", len);
    return (PHYMOD_E_NONE);
}

/*set/get port fec enable according to local/remote FEC ability*/
int plp_aperta2_pm8x100_gen2_port_fec_enable_set(int unit, int port, pm_info_t pm_info,
                                     uint32_t phy_flags, uint32_t enable)
{
    return PHYMOD_E_UNAVAIL;
}
int plp_aperta2_pm8x100_gen2_port_fec_enable_get(int unit, int port, pm_info_t pm_info,
                                     uint32_t phy_flags, uint32_t* enable)
{
    return PHYMOD_E_UNAVAIL;
}


/*set/get pass control frames.*/
int plp_aperta2_pm8x100_gen2_port_rx_control_set(int unit, int port, pm_info_t pm_info,
                                     const portmod_rx_control_t* rx_ctrl)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    if (rx_ctrl->flags & PORTMOD_MAC_PASS_CONTROL_FRAME) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_selective_mask_set(&phy_access,
                                                    APERTA2_DC3MAC_RSV_MASK_PFC_FRAME,
                                                    rx_ctrl->pass_control_frames));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pass_control_frame_set(&phy_access, rx_ctrl->pass_control_frames));
    }

    if (rx_ctrl->flags & PORTMOD_MAC_PASS_PFC_FRAME) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_selective_mask_set(&phy_access,
                                                    APERTA2_DC3MAC_RSV_MASK_PFC_FRAME,
                                                    rx_ctrl->pass_pfc_frames));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pass_pfc_frame_set(&phy_access, rx_ctrl->pass_pfc_frames));
    }

    if (rx_ctrl->flags & PORTMOD_MAC_PASS_PAUSE_FRAME) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_selective_mask_set(&phy_access,
                                                    APERTA2_DC3MAC_RSV_MASK_PAUSE_FRAME,
                                                    rx_ctrl->pass_pause_frames));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pass_pause_frame_set(&phy_access, rx_ctrl->pass_pause_frames));
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_rx_control_get(int unit, int port, pm_info_t pm_info,
                                     portmod_rx_control_t* rx_ctrl)
{
    uint32_t rx_pass_control_frames = 0, rx_pass_pfc_frames = 0, rx_pass_pause_frames = 0;
    uint32_t rsv_pass_control_frames = 0, rsv_pass_pfc_frames = 0, rsv_pass_pause_frames = 0;
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pass_control_frame_get(&phy_access, &rx_pass_control_frames));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pass_pfc_frame_get(&phy_access, &rx_pass_pfc_frames));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pass_pause_frame_get(&phy_access, &rx_pass_pause_frames));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_selective_mask_get(&phy_access, APERTA2_DC3MAC_RSV_MASK_PFC_FRAME,
                                                   &rsv_pass_control_frames));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_selective_mask_get(&phy_access, APERTA2_DC3MAC_RSV_MASK_PFC_FRAME,
                                                   &rsv_pass_pfc_frames));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_selective_mask_get(&phy_access, APERTA2_DC3MAC_RSV_MASK_PAUSE_FRAME,
                                                   &rsv_pass_pause_frames));

    rx_ctrl->pass_control_frames = rx_pass_control_frames & rsv_pass_control_frames;
    rx_ctrl->pass_pfc_frames = rx_pass_pfc_frames & rsv_pass_pfc_frames;
    rx_ctrl->pass_pause_frames = rx_pass_pause_frames & rsv_pass_pause_frames;

    return PHYMOD_E_NONE;
}

/*set PFC config registers.*/
int plp_aperta2_pm8x100_gen2_port_pfc_config_set(int unit, int port, pm_info_t pm_info,
                                     const portmod_pfc_config_t* pfc_cfg)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    if (pfc_cfg->classes != 8) {
        return PHYMOD_E_PARAM;
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pfc_config_set(&phy_access, pfc_cfg));
    /* set the pfc frame control in aperta2_dc3mac rsv */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_selective_mask_set(&phy_access,
                                                   APERTA2_DC3MAC_RSV_MASK_PFC_FRAME,
                                                   pfc_cfg->rxpass));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_pfc_config_get(int unit, int port, pm_info_t pm_info,
                                     portmod_pfc_config_t* pfc_cfg)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));
    pfc_cfg->classes = 8;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_pfc_config_get(&phy_access, pfc_cfg));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_selective_mask_get(&phy_access,
                                                   APERTA2_DC3MAC_RSV_MASK_PFC_FRAME,
                                                   &pfc_cfg->rxpass));
    return PHYMOD_E_NONE;
}

/*set Vlan Inner/Outer tag.*/
int plp_aperta2_pm8x100_gen2_port_vlan_tag_set(int unit, int port, pm_info_t pm_info,
                                   const portmod_vlan_tag_t* vlan_tag)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));
   
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_vlan_tag_set(&phy_access,
                                         vlan_tag->outer_vlan_tag,
                                         vlan_tag->inner_vlan_tag));

    return PHYMOD_E_NONE;
}
int plp_aperta2_pm8x100_gen2_port_vlan_tag_get(int unit, int port, pm_info_t pm_info,
                                   portmod_vlan_tag_t* vlan_tag)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_vlan_tag_get(&phy_access,
                                         (int*)&vlan_tag->outer_vlan_tag,
                                         (int*)&vlan_tag->inner_vlan_tag));

    return PHYMOD_E_NONE;
}

/*set modid field.*/
int plp_aperta2_pm8x100_gen2_port_mode_set(int unit, int port, pm_info_t pm_info,
                               const portmod_port_mode_info_t* mode)
{

    return (PHYMOD_E_NONE);
}


STATIC
int _plp_aperta2_pm8x100_gen2_dc3port_mode_get(int unit, int port, pm_info_t pm_info,
                                  int first_phy_index, portmod_port_mode_info_t *p_mode)
{
    BCM_APERTA2_DC3MAC_DC3PORT_MODEr_t reg_val;
    plp_aperta2_phymod_phy_access_t phy_acc;

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(READ_DC3PORT_MODEr(&phy_acc, &reg_val));

    switch (reg_val.v[0]) {
        case 0:
            p_mode->cur_mode = portmodPortModeQuad;
            p_mode->lanes = 1;
            p_mode->port_index = first_phy_index;
            break;
        case 1:
            p_mode->cur_mode = portmodPortModeTri012;
            if ((first_phy_index == 0)||(first_phy_index == 1)) {
                p_mode->lanes = 1;
            } else {
                p_mode->lanes = 2;
            }
            p_mode->port_index = first_phy_index;
            break;
        case 2:
            p_mode->cur_mode = portmodPortModeTri023;
            if ((first_phy_index == 2)||(first_phy_index == 3)) {
                p_mode->lanes = 1;
            } else {
                p_mode->lanes = 2;
            }
            p_mode->port_index = first_phy_index;
            break;
        case 3:
            p_mode->cur_mode = portmodPortModeDual;
            p_mode->lanes = 2;
            p_mode->port_index = first_phy_index;
            break;
        case 4:
            p_mode->cur_mode = portmodPortModeSingle;
            p_mode->lanes = 4;
            p_mode->port_index = first_phy_index;
            break;
        case 8:
            p_mode->cur_mode = portmodPortModeSingle_8;
            p_mode->lanes = 4;
            p_mode->port_index = 0;
            break;
        default:
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM, (SOC_MSG("Invalid port mode")));
            break;
    }


    return PHYMOD_E_NONE;

}

int plp_aperta2_pm8x100_gen2_port_mode_get(int unit, int port, pm_info_t pm_info,
                               portmod_port_mode_info_t* mode)
{
    int port_index;
    uint32_t bitmap;
    plp_aperta2_phymod_phy_access_t phy_acc;

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get (unit, port, pm_info, &port_index, &bitmap));
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_dc3port_mode_get(unit,port, pm_info, port_index, mode));


    return PHYMOD_E_NONE;
}

/*set port encap.*/
int plp_aperta2_pm8x100_gen2_port_encap_set(int unit, int port, pm_info_t pm_info,
                                int flags, portmod_encap_t encap)
{
    plp_aperta2_phymod_phy_access_t phy_acc;

    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_encap_set(&phy_acc, encap));

    return PHYMOD_E_NONE;
}
int plp_aperta2_pm8x100_gen2_port_encap_get(int unit, int port, pm_info_t pm_info,
                                int* flags, portmod_encap_t* encap)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_encap_get(&phy_acc, encap));

    return PHYMOD_E_NONE;
}

/*set/get hwfailover for trident.*/
int plp_aperta2_pm8x100_gen2_port_diag_ctrl(int unit, int port, pm_info_t pm_info,
                                uint32_t inst, int op_type, int op_cmd,
                                const void* arg)
{
    return PHYMOD_E_UNAVAIL;

}

int plp_aperta2_pm8x100_gen2_port_cntmaxsize_set(int unit, int port, pm_info_t pm_info, int val)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    return (plp_aperta2_dc3mac_mib_oversize_set(&phy_acc, val));
}

int plp_aperta2_pm8x100_gen2_port_cntmaxsize_get(int unit, int port, pm_info_t pm_info, int* val)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    return (plp_aperta2_dc3mac_mib_oversize_get(&phy_acc, (uint32_t *)val));
}

/*Get Info needed to restore after drain cells.*/
int plp_aperta2_pm8x100_gen2_port_drain_cell_get(int unit, int port, pm_info_t pm_info,
                                     portmod_drain_cells_t* drain_cells)
{
    
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_drain_cell_get(&phy_acc, drain_cells));


    return PHYMOD_E_NONE;
}

/*Restore informaation after drain cells.*/
int plp_aperta2_pm8x100_gen2_port_drain_cell_stop(int unit, int port, pm_info_t pm_info,
                                      const portmod_drain_cells_t* drain_cells)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));


    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_drain_cell_stop(&phy_acc, drain_cells));


    return PHYMOD_E_NONE;
}

/*Restore informaation after drain cells.*/
int plp_aperta2_pm8x100_gen2_port_drain_cell_start(int unit, int port, pm_info_t pm_info)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_drain_cell_start(&phy_acc));


    return PHYMOD_E_NONE;
}

/**/
int plp_aperta2_pm8x100_gen2_port_drain_cells_rx_enable(int unit, int port,
                                            pm_info_t pm_info,
                                            int rx_en)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_drain_cells_rx_enable(&phy_acc, rx_en));
    return PHYMOD_E_NONE;
}

/**/
int plp_aperta2_pm8x100_gen2_port_egress_queue_drain_rx_en(int unit, int port,
                                               pm_info_t pm_info,
                                               int rx_en)
{
     plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_egress_queue_drain_rx_en(&phy_acc, rx_en));
    return PHYMOD_E_NONE;
}

/**/
int plp_aperta2_pm8x100_gen2_port_mac_ctrl_set(int unit, int port,
                                   pm_info_t pm_info,
                                   uint64_t ctrl)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_mac_ctrl_set(&phy_acc, ctrl));
    return PHYMOD_E_NONE;
}

/**/
int plp_aperta2_pm8x100_gen2_port_txfifo_cell_cnt_get(int unit, int port,
                                          pm_info_t pm_info,
                                          uint32_t* cnt)
{
       plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_txfifo_cell_cnt_get(&phy_acc, cnt));
    return PHYMOD_E_NONE;
}

/**/
int plp_aperta2_pm8x100_gen2_port_egress_queue_drain_get(int unit, int port,
                                             pm_info_t pm_info,
                                             uint64_t* ctrl, int* rxen)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_egress_queue_drain_get(&phy_acc, ctrl, rxen));
    return PHYMOD_E_NONE;
}

/**/
int plp_aperta2_pm8x100_gen2_port_mac_reset_set(int unit, int port, pm_info_t pm_info, int val)
{
        plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_reset_set(&phy_acc, val));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_mac_reset_get(int unit, int port, pm_info_t pm_info, int* val)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_reset_get(&phy_acc, (uint32_t *)val));
    return PHYMOD_E_NONE;
}

/*Check if MAC needs to be reset.*/
int plp_aperta2_pm8x100_gen2_port_mac_reset_check(int unit, int port,
                                      pm_info_t pm_info,
                                      int enable, int* reset)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    return (plp_aperta2_dc3mac_reset_check(&phy_acc, enable, reset));
}


/*get the speed for the specified port*/
int plp_aperta2_pm8x100_gen2_port_speed_get(int unit, int port, pm_info_t pm_info, int* speed)
{
    phymod_phy_speed_config_t speed_config;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;

    
    *speed = 0;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_speed_config_t_init(&speed_config));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_tscp_driver.f_phymod_phy_speed_config_get(&phy_access, &speed_config));

    *speed = speed_config.data_rate;

    return PHYMOD_E_NONE;
}

/*Port discard set*/
int plp_aperta2_pm8x100_gen2_port_discard_set(int unit, int port, pm_info_t pm_info, int discard)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_discard_set(&phy_acc, discard));


    return PHYMOD_E_NONE;
}

/*Port tx_en=0 and softreset mac*/
int plp_aperta2_pm8x100_gen2_port_tx_down(int unit, int port, pm_info_t pm_info)
{
    plp_aperta2_phymod_phy_access_t phy_acc;
    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
   
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_tx_enable_set(&phy_acc, 0));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_discard_set(&phy_acc, 0));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_reset_set(&phy_acc, 1));


    return PHYMOD_E_NONE;
}

/*port control phy timesync config set/get*/
int plp_aperta2_pm8x100_gen2_port_control_phy_timesync_set(int unit,
                                               int port, pm_info_t pm_info,
                                               portmod_port_control_phy_timesync_t config,
                                               uint64_t value)
{
#ifdef APERTA2_PM_TS_ENABLE
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    uint32_t flags = 0;

    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    if (config == SOC_PORT_CONTROL_PHY_TIMESYNC_TIMESTAMP_ADJUST) {
        PHYMOD_TIMESYNC_ENABLE_F_RX_SET(flags);
        PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_timesync_enable_set(&phy_access, flags, 0));

    }
    PHYMOD_IF_ERR_RETURN(portmod_common_control_phy_timesync_set(&phy_access, config, value));

    if (config == SOC_PORT_CONTROL_PHY_TIMESYNC_TIMESTAMP_ADJUST) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_timesync_enable_set(&phy_access, flags, 1));
        /* Wait for one AM spacing time */
        PHYMOD_USLEEP(500);
    }
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_control_phy_timesync_get(int unit,
                                         int port, pm_info_t pm_info,
                                         portmod_port_control_phy_timesync_t config,
                                         uint64_t* value)
{
#ifdef APERTA2_PM_TS_ENABLE
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;

    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN(portmod_common_control_phy_timesync_get(&phy_access, config, value));

#endif
    return PHYMOD_E_NONE;
}

/*"port timesync config set/get"*/
int plp_aperta2_pm8x100_gen2_port_timesync_config_set(int unit, int port, pm_info_t pm_info,
                                          const portmod_phy_timesync_config_t* config)
{
#ifdef APERTA2_PM_TS_ENABLE
    uint32_t mac_da_timestamp_en = 0;
    int port_index, /*port_ts_status, */ts_is_enable, ts_enable_port_count;
    uint32_t bitmap, one_step_req, is_one_step = 0, flags = 0;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys, enable;
    uint32_t timesync_config = 0;

    

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, &bitmap));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    enable = (config->flags & SOC_PORT_PHY_TIMESYNC_ENABLE) ? 1 : 0;

    aperta2_PM8x100_GEN2_TIMESYNC_CONFIG_GET(unit, pm_info, timesync_config, port_index);
    ts_is_enable = PORTMOD_PORT_TIMESYNC_CONFIG_ENABLE_GET(timesync_config);
    aperta2_PM8x100_GEN2_TS_ENABLE_PORT_COUNT_GET(unit, pm_info, ts_enable_port_count);

    /* Clear port timestamp status. */
    timesync_config = 0;

    if (enable) {
        /* Enable 1588. */
        if (ts_is_enable) {
            /* Clear current 1588 configs if the port has 1588 enabled. */
            PHYMOD_TIMESYNC_ENABLE_F_RX_SET(flags);
            PHYMOD_TIMESYNC_ENABLE_F_ONE_STEP_PIPELINE_SET(flags);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_phymod_timesync_enable_set(&phy_access, flags, 0));
            flags = 0;
        } else {
            /* 1588 is not enabled on the port. */
            if (ts_enable_port_count == 0) {
                /* This is the first 1588 port on the PM. */
                PHYMOD_TIMESYNC_ENABLE_F_CORE_SET(flags);
            }
            ts_enable_port_count++;
        }
        one_step_req = (config->flags & SOC_PORT_PHY_TIMESYNC_ONE_STEP_ENABLE) ? 1 : 0;
        if (one_step_req) {
            PHYMOD_TIMESYNC_ENABLE_F_ONE_STEP_PIPELINE_SET(flags);
        }
        /* Rx enable will be done on link up as per programming requirement. */
        PHYMOD_IF_ERR_RETURN
                (plp_aperta2_phymod_timesync_enable_set(&phy_access, flags, 1));

        /* Update port timestamp status. */
        PORTMOD_PORT_TIMESYNC_CONFIG_ENABLE_SET(timesync_config);

        mac_da_timestamp_en = 0;

        if (config->flags & SOC_PORT_PHY_TIMESYNC_SELECT_SFD) {
            PORTMOD_PORT_TIMESYNC_CONFIG_SFD_SET(timesync_config);
        }
        if (config->flags & SOC_PORT_PHY_TIMESYNC_SELECT_MAC_DA) {
            PORTMOD_PORT_TIMESYNC_CONFIG_MAC_DA_SET(timesync_config);
            mac_da_timestamp_en = 1;
        }
        if (config->flags & SOC_PORT_PHY_TIMESYNC_COMP_MODE_ON) {
            PORTMOD_PORT_TIMESYNC_CONFIG_COMP_MODE_ON_SET(timesync_config);
            if (config->flags & SOC_PORT_PHY_TIMESYNC_COMP_MODE_LATEST_LANE){
                PORTMOD_PORT_TIMESYNC_CONFIG_COMP_MODE_LATEST_LANE_SET(timesync_config);
            } else {
                PORTMOD_PORT_TIMESYNC_CONFIG_COMP_MODE_LATEST_LANE_CLR(timesync_config);
            }
        } else {
            PORTMOD_PORT_TIMESYNC_CONFIG_COMP_MODE_ON_CLR(timesync_config);
            PORTMOD_PORT_TIMESYNC_CONFIG_COMP_MODE_LATEST_LANE_CLR(timesync_config);
        }

        /* program aperta2_dc3mac da_timestamp_en. */
        PHYMOD_IF_ERR_RETURN
            (plp_aperta2_dc3mac_rx_da_timestmap_enable_set(unit, port, mac_da_timestamp_en));
    } else {
        /* Disable 1588. */
        if (ts_is_enable) {
            /* If 1588 is currently enabled on the port. */
            /* 1. Disable One Stpe pipeline if it is enabled. */
            PHYMOD_TIMESYNC_ENABLE_F_ONE_STEP_PIPELINE_SET(flags);
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_phymod_timesync_enable_get(&phy_access, flags, &is_one_step));
            if (!is_one_step) {
                flags = 0;
            }
            /* 2. Disable RX timestamp. */
            PHYMOD_TIMESYNC_ENABLE_F_RX_SET(flags);

            ts_enable_port_count--;
            /* 3. Disable FCLK if no port will using 1588 on the PM. */
            if (ts_enable_port_count == 0) {
                PHYMOD_TIMESYNC_ENABLE_F_CORE_SET(flags);
            }

            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_phymod_timesync_enable_set(&phy_access, flags, 0));
        }
    }

    /* Update Timesync Enable Status in WB */
    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_TS_ENABLE_PORT_COUNT_SET(unit, pm_info, ts_enable_port_count));
    if (enable) {
        PORTMOD_PORT_TIMESYNC_CONFIG_ENABLE_SET(timesync_config);
    } else {
        PORTMOD_PORT_TIMESYNC_CONFIG_ENABLE_CLR(timesync_config);
    }
    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_TIMESYNC_CONFIG_SET(unit, pm_info, timesync_config, port_index));

#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_timesync_config_get(int unit, int port, pm_info_t pm_info,
                                    portmod_phy_timesync_config_t* config)
{
#ifdef APERTA2_PM_TS_ENABLE
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys, timesync_config, timesync_enable = 0;
    int port_index;
    uint32_t bitmap, is_one_step, flags = 0;
    

    PHYMOD_MEMSET(config, 0,sizeof(portmod_phy_timesync_config_t));

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, &bitmap));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_TIMESYNC_CONFIG_GET(unit, pm_info, &timesync_config, port_index));
    PHYMOD_TIMESYNC_ENABLE_F_ONE_STEP_PIPELINE_SET(flags);
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_timesync_enable_get(&phy_access, flags, &is_one_step));
    timesync_enable = PORTMOD_PORT_TIMESYNC_CONFIG_ENABLE_GET(timesync_config);
    config->flags |= timesync_enable ? (is_one_step? SOC_PORT_PHY_TIMESYNC_ONE_STEP_ENABLE : SOC_PORT_PHY_TIMESYNC_TWO_STEP_ENABLE) : 0;
    config->flags |= PORTMOD_PORT_TIMESYNC_CONFIG_ENABLE_GET(timesync_config)?
                         SOC_PORT_PHY_TIMESYNC_ENABLE : 0;
    config->flags |= PORTMOD_PORT_TIMESYNC_CONFIG_SFD_GET(timesync_config)?
                         SOC_PORT_PHY_TIMESYNC_SELECT_SFD: 0;
    config->flags |= PORTMOD_PORT_TIMESYNC_CONFIG_MAC_DA_GET(timesync_config)?
                         SOC_PORT_PHY_TIMESYNC_SELECT_MAC_DA: 0;
    config->flags |= PORTMOD_PORT_TIMESYNC_CONFIG_COMP_MODE_ON_GET(timesync_config)?
                         SOC_PORT_PHY_TIMESYNC_COMP_MODE_ON: 0;
    config->flags |= PORTMOD_PORT_TIMESYNC_CONFIG_COMP_MODE_LATEST_LANE_GET(timesync_config) ?
                         SOC_PORT_PHY_TIMESYNC_COMP_MODE_LATEST_LANE: 0;

#endif
    return PHYMOD_E_NONE;
}

/*set/get interrupt enable value. */
int plp_aperta2_pm8x100_gen2_port_interrupt_enable_set(int unit, int port,
                                           pm_info_t pm_info,
                                           int intr_type, uint32_t val)
{
#ifdef APERTA2_PM_INT_ENABLE
    uint32_t reg_val;
    plp_aperta2_phymod_phy_access_t phy_acc;
    

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    PHYMOD_IF_ERR_RETURN(READ_DC3PORT_PORT_INTR_ENABLEr(unit, phy_acc, &reg_val));

    switch(intr_type) {
        case portmodIntrTypeLinkdown:
            soc_reg_field_set(unit, DC3PORT_PORT_INTR_ENABLEr, &reg_val, LINK_DOWNf, val);
            break;
        case portmodIntrTypeLinkup:
            soc_reg_field_set(unit, DC3PORT_PORT_INTR_ENABLEr, &reg_val, LINK_UPf, val);
            break;
        case portmodIntrTypeFcReqFull:
            soc_reg_field_set(unit, DC3PORT_PORT_INTR_ENABLEr, &reg_val, FLOWCONTROL_REQ_FULLf, val);
            break;
        case portmodIntrTypeTxPktUnderflow:
        case portmodIntrTypeTxPktOverflow:
        case portmodIntrTypeTxCdcSingleBitErr:
        case portmodIntrTypeTxCdcDoubleBitErr:
        case portmodIntrTypeLocalFaultStatus:
        case portmodIntrTypeRemoteFaultStatus:
        case portmodIntrTypeMibMemSingleBitErr:
        case portmodIntrTypeMibMemDoubleBitErr:
        case portmodIntrTypeMibMemMultipleBitErr:
        case portmodIntrTypeRxPfcFifoOverflow:
        case portmodIntrTypeTxPfcFifoOverflow:
        case portmodIntrTypeFdrInterrupt:
            PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_interrupt_enable_set(unit, port, intr_type, val));
            break;
        default:
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM, (SOC_MSG("Invalid interrupt type")));
            break;
    }
    PHYMOD_IF_ERR_RETURN(WRITE_DC3PORT_PORT_INTR_ENABLEr(unit, phy_acc, reg_val));
#endif

    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta2_pm8x100_gen2_phy_timesync_tx_info_get(int unit, int port, pm_info_t pm_info, portmod_fifo_status_t* tx_info)
{
#ifdef APERTA2_PM_TS_ENABLE
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    portmod_access_get_params_t params;
    phymod_ts_fifo_status_t phy_ts_tx_info;
    

    PHYMOD_IF_ERR_RETURN(phymod_ts_fifo_status_t_init(&phy_ts_tx_info));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN(phymod_timesync_tx_info_get(&phy_access, &phy_ts_tx_info));

    tx_info->timestamps_in_fifo = phy_ts_tx_info.ts_in_fifo_lo;
    tx_info->timestamps_in_fifo_hi = phy_ts_tx_info.ts_in_fifo_hi;
    tx_info->sequence_id = phy_ts_tx_info.ts_seq_id;
    tx_info->timestamp_sub_nanosec = phy_ts_tx_info.ts_sub_nanosec;

#endif
    return PHYMOD_E_NONE;
}

/*!
 * plp_aperta2_pm8x100_gen2_port_timesync_tx_info_get
 *
 * @brief get port timestamps in fifo
 *
 * @param [in]  unit            - unit id
 * @param [in]  port            - logical port
 * @param [inout]  tx_info         - timestamp and seq id form fifo
 */

int plp_aperta2_pm8x100_gen2_port_timesync_tx_info_get(int unit, int port, pm_info_t pm_info, portmod_fifo_status_t* tx_info)
{
    

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_timesync_tx_info_get(unit, port, pm_info, tx_info));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_interrupt_enable_get(int unit, int port,
                                           pm_info_t pm_info,
                                           int intr_type, uint32_t* val)
{
#ifdef APERTA2_PM_INT_ENABLE
    uint32_t reg_val;
    plp_aperta2_phymod_phy_access_t phy_acc;
    

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    PHYMOD_IF_ERR_RETURN(READ_DC3PORT_PORT_INTR_ENABLEr(unit, phy_acc, &reg_val));

    switch(intr_type) {
        case portmodIntrTypeLinkdown:
            *val = soc_reg_field_get(unit, DC3PORT_PORT_INTR_ENABLEr, reg_val, LINK_DOWNf);
            break;
        case portmodIntrTypeLinkup:
            *val = soc_reg_field_get(unit, DC3PORT_PORT_INTR_ENABLEr, reg_val, LINK_UPf);
            break;
        case portmodIntrTypeFcReqFull:
            *val = soc_reg_field_get(unit, DC3PORT_PORT_INTR_ENABLEr, reg_val, FLOWCONTROL_REQ_FULLf);
            break;
        case portmodIntrTypeTxPktUnderflow:
        case portmodIntrTypeTxPktOverflow:
        case portmodIntrTypeTxCdcSingleBitErr:
        case portmodIntrTypeTxCdcDoubleBitErr:
        case portmodIntrTypeLocalFaultStatus:
        case portmodIntrTypeRemoteFaultStatus:
        case portmodIntrTypeMibMemSingleBitErr:
        case portmodIntrTypeMibMemDoubleBitErr:
        case portmodIntrTypeMibMemMultipleBitErr:
        case portmodIntrTypeRxPfcFifoOverflow:
        case portmodIntrTypeTxPfcFifoOverflow:
        case portmodIntrTypeFdrInterrupt:
            PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_interrupt_enable_get(unit, port, intr_type, val));
            break;
        default:
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM, (SOC_MSG("Invalid interrupt type")));
            break;
    }
#endif

    return PHYMOD_E_NONE;
}

/*get interrupt status value. */
int plp_aperta2_pm8x100_gen2_port_interrupt_get(int unit, int port, pm_info_t pm_info, int intr_type, uint32_t* val)
{
#ifdef APERTA2_PM_INT_ENABLE
    plp_aperta2_phymod_phy_access_t phy_acc;
    

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    switch(intr_type) {
        case portmodIntrTypeTxPktUnderflow:
        case portmodIntrTypeTxPktOverflow:
        case portmodIntrTypeTxCdcSingleBitErr:
        case portmodIntrTypeTxCdcDoubleBitErr:
        case portmodIntrTypeLocalFaultStatus:
        case portmodIntrTypeRemoteFaultStatus:
        case portmodIntrTypeMibMemSingleBitErr:
        case portmodIntrTypeMibMemDoubleBitErr:
        case portmodIntrTypeMibMemMultipleBitErr:
        case portmodIntrTypeRxPfcFifoOverflow:
        case portmodIntrTypeTxPfcFifoOverflow:
        case portmodIntrTypeFdrInterrupt:
            PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_interrupt_status_get(unit, port, intr_type, val));
            break;
        default:
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM, (SOC_MSG("Invalid interrupt type")));
            break;
    }
#endif

    return PHYMOD_E_NONE;
}

/*get interrupt value array. */
int plp_aperta2_pm8x100_gen2_port_interrupts_get(int unit, int port, pm_info_t pm_info,
                                     int arr_max_size, uint32_t* intr_arr,
                                     uint32_t* size)
{
#ifdef APERTA2_PM_INT_ENABLE
    uint32_t cnt=0;
    plp_aperta2_phymod_phy_access_t phy_acc;
    uint32_t mac_intr_cnt = 0;
    

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_interrupts_status_get(unit, port, (arr_max_size - cnt), (intr_arr + cnt), &mac_intr_cnt));
    cnt += mac_intr_cnt;
    *size = cnt;
#endif

    return PHYMOD_E_NONE;
}

/* portmod port rsv mask set*/
int plp_aperta2_pm8x100_gen2_port_mac_rsv_mask_set(int unit, int port,
                                       pm_info_t pm_info,
                                       uint32_t rsv_mask)
{
    plp_aperta2_phymod_phy_access_t phy_acc;

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_mask_set(&phy_acc, rsv_mask));


    return PHYMOD_E_NONE;
}

/*Returns if the PortMacro associated with the port is initialized or not*/
int plp_aperta2_pm8x100_gen2_pm_is_initialized(int unit, int pm_id, pm_info_t pm_info, int* is_initialized)
{
    int is_core_initialized = 0;
    aperta2_PM8x100_GEN2_IS_CORE_INITIALIZED_GET(unit, pm_info, is_core_initialized);
    *is_initialized = is_core_initialized;

    return PHYMOD_E_NONE;
}

/* get the logical port bitmap of the current PM */
int plp_aperta2_pm8x100_gen2_pm_logical_pbmp_get(int unit, int pm_id, pm_info_t pm_info,
                                     portmod_pbmp_t* logical_pbmp)
{
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_core_add(int unit, int pm_id, pm_info_t pm_info, int flags, const portmod_port_add_info_t* add_info)
{
    int side = 0;
#ifdef APERTA2_OCTAL_NO_BCAST
    unsigned int lane_mask =0;
#endif

    if (add_info->octal_start == APERTA2_OCTAL0) { 
        if (PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info)) {
#ifdef APERTA2_OCTAL_NO_BCAST
            for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
#else
            for (side = phymodPortLocLine; side < phymodPortLocSys; side++) {
#endif
                PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.lane_mask = 0x1;
                PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask = 0x1;

                PM_8x100_GEN2_INFO(pm_info)->int_phy_access.port_loc = side;
                PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc = side;
                PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_pm_enable(unit, pm_id, pm_info, 0, 1));
            }
        }
        /* Octal 0*/
#ifdef APERTA2_OCTAL_NO_BCAST
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
#else
        for (side = phymodPortLocLine; side < phymodPortLocSys; side++) {
#endif
            PM_8x100_GEN2_INFO(pm_info)->int_phy_access.port_loc = side;
            PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc = side;
            if (PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info)) {
                PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.lane_mask  = 0xFF;
                PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask = 0xFF;
            }

            PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_pm_serdes_core_init(unit, pm_id, pm_info, add_info));
        }
    }
    if (add_info->octal_end == APERTA2_OCTAL1) { 
#ifdef APERTA2_OCTAL_NO_BCAST
        lane_mask = PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask;
        /* Octal 1*/
        if (PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info)) {
            for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
                PM_8x100_GEN2_INFO(pm_info)->int_phy_access.port_loc = side;
                PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc = side;
                PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.lane_mask  = 0x100;
                PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask = 0x100;
                PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_pm_enable(unit, pm_id, pm_info, 0, 1));
            }
        }
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            if (PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info)) {
                PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.lane_mask  = 0xFF00;
                PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask = 0xFF00;
            }
            PM_8x100_GEN2_INFO(pm_info)->int_phy_access.port_loc = side;
            PM_8x100_GEN2_INFO(pm_info)->int_core_access.port_loc = side;

            PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_pm_serdes_core_init(unit, pm_id, pm_info, add_info));
        }
        PM_8x100_GEN2_INFO(pm_info)->int_phy_access.access.lane_mask = PM_8x100_GEN2_INFO(pm_info)->int_core_access.access.lane_mask = lane_mask;
#endif
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_lane_map_set(int unit, int port, pm_info_t pm_info, uint32_t flags, const plp_aperta2_phymod_lane_map_t* lane_map)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta2_pm8x100_gen2_port_lane_map_get(int unit, int port, pm_info_t pm_info, uint32_t flags, plp_aperta2_phymod_lane_map_t* lane_map)
{
    

    *lane_map = PM_8x100_GEN2_INFO(pm_info)->lane_map;

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_synce_clk_ctrl_set(int unit, int port, pm_info_t pm_info,
                                         const portmod_port_synce_clk_ctrl_t* cfg)
{
#ifdef APERTA2_PM_SYNCE
    phymod_synce_clk_ctrl_t phy_synce_cfg;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    int config_valid = 0;

    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    phymod_synce_clk_ctrl_t_init(&phy_synce_cfg);
    phy_synce_cfg.stg0_mode = cfg->stg0_mode;
    phy_synce_cfg.stg1_mode = cfg->stg1_mode;
    phy_synce_cfg.sdm_val = cfg->sdm_val;

    /* next validate the stage0/1 config */
    /* first check legacy mode */
    if (cfg->stg0_mode == 0x0) {
        config_valid = 1;
    } else if ((cfg->stg0_mode == 0x1) && (cfg->stg1_mode == 0x2)) {
        config_valid = 1;
    } else if ((cfg->stg0_mode == 0x2) && (cfg->stg1_mode == 0x0)) {
        config_valid = 1;
    }

    if (config_valid) {
        PHYMOD_IF_ERR_RETURN(phymod_phy_synce_clk_ctrl_set(&phy_access,
                                                       phy_synce_cfg));
    } else {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_CONFIG,
                  (SOC_MSG("SyncE config is not valid \n")));
    }

#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_synce_clk_ctrl_get(int unit, int port, pm_info_t pm_info,
                                         portmod_port_synce_clk_ctrl_t* cfg)
{
#ifdef APERTA2_PM_SYNCE
    phymod_synce_clk_ctrl_t phy_synce_cfg;
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;

    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    phymod_synce_clk_ctrl_t_init(&phy_synce_cfg);

    PHYMOD_IF_ERR_RETURN(phymod_phy_synce_clk_ctrl_get(&phy_access,
                                                    &phy_synce_cfg));

    cfg->stg0_mode = phy_synce_cfg.stg0_mode;
    cfg->stg1_mode = phy_synce_cfg.stg1_mode;
    cfg->sdm_val = phy_synce_cfg.sdm_val;

#endif
    return PHYMOD_E_NONE;
}


int plp_aperta2_pm8x100_gen2_pm_interrupt_process(int unit, int pm_id, pm_info_t pm_info,
                                      portmod_ecc_intr_info_t *ecc_info)
{
/* FIXME, to be added */
    return PHYMOD_E_NONE;
}


int plp_aperta2_pm8x100_gen2_port_mac_enable_set(int unit,int port, pm_info_t pm_info,
                                     int enable)
{
    plp_aperta2_phymod_phy_access_t phy_acc;

    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_port_enable_set(&phy_acc, enable));

    return PHYMOD_E_NONE;
} 

/* This function collects the reasons for local_faults that might have 
 * happened before it's invocation. Returns a bit-map with all those
 * reasons
 */
int plp_aperta2_pm8x100_gen2_port_local_fault_reasons_get(int unit, int port, pm_info_t pm_info,
                                              uint32_t* local_fault_reasons)
{
#ifdef APERTA2_NO_SUPPORT
    int no_local_fault = FALSE;
    uint32_t an_done;
    int nof_phys;
    int port_index;
    uint32_t bitmap[1], is_bypassed = 0;
    plp_aperta2_phymod_phy_access_t phy_access;
    portmod_access_get_params_t params;
    portmod_speed_config_t speed_config;
    plp_aperta2_phymod_autoneg_control_t an_control;
    phymod_phy_local_fault_info_t local_fault_info;

    
    PHYMOD_NULL_CHECK(pm_info);

    *local_fault_reasons = 0;
    PHYMOD_MEMSET(&speed_config, 0, sizeof(portmod_speed_config_t));
    PHYMOD_MEMSET(&local_fault_info, 0, sizeof(phymod_phy_local_fault_info_t));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                &params, 1, &phy_access, &nof_phys, NULL));


    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_port_index_get(unit, port, pm_info, &port_index, bitmap));
    PHYMOD_IF_ERR_RETURN(aperta2_PM8x100_GEN2_PORT_IS_PCS_BYPASSED_GET(unit, pm_info, is_bypassed, port_index));

    if (is_bypassed) {
        PORTMOD_PORT_LOCAL_FAULT_REASON_UNKNOWN_SET(*local_fault_reasons);
        return PHYMOD_E_NONE;
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_autoneg_get(&phy_access, &an_control, &an_done));

    if (an_control.enable) {
        PORTMOD_PORT_LOCAL_FAULT_REASON_UNKNOWN_SET(*local_fault_reasons);
        return PHYMOD_E_NONE;
    }

    PHYMOD_IF_ERR_RETURN
        (phymod_phy_local_fault_info_get(&phy_access, &local_fault_info));

    if (!local_fault_info.pcs_latched_local_fault && 
        local_fault_info.pcs_link_status_live) {
        no_local_fault = TRUE;
    }

    if (local_fault_info.pcs_latched_deskew_low) {
        PORTMOD_PORT_LOCAL_FAULT_REASON_NO_DESKEW_SET(*local_fault_reasons);
    }

    if (local_fault_info.pmd_rx_locked && !local_fault_info.pmd_rx_lock_change) {
        /* PMD is locked and there was no change. 
         * Don't need to report PMD no lock reason
         */
    } else {
        /* For all other cases, report PMD no lock reason */
        PORTMOD_PORT_LOCAL_FAULT_REASON_PMD_NO_LOCK_SET(*local_fault_reasons);
    }

    if (local_fault_info.am_lock_latched_low) {
        PORTMOD_PORT_LOCAL_FAULT_REASON_AM_NO_LOCK_SET(*local_fault_reasons);
    }

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_pm8x100_gen2_port_speed_config_get(unit, port, pm_info, &speed_config));

    if (speed_config.fec != PORTMOD_PORT_PHY_FEC_NONE) {
        if (local_fault_info.fec_align_latched_low) {
            PORTMOD_PORT_LOCAL_FAULT_REASON_FEC_NO_ALIGN_SET(*local_fault_reasons);
        }
    }

    if (!(*local_fault_reasons)) {
        if (no_local_fault) {
            PORTMOD_PORT_LOCAL_FAULT_REASON_NONE_SET(*local_fault_reasons);
        } else {
            PORTMOD_PORT_LOCAL_FAULT_REASON_UNKNOWN_SET(*local_fault_reasons);
        }
    } 
#endif

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_fec_error_inject_set(int unit, int port, pm_info_t pm_info,
                                           uint16_t error_control_map,
                                           portmod_fec_error_mask_t bit_error_mask)
{
#ifdef APERTA2_NO_SUPPORT
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN
        (phymod_phy_fec_error_inject_set(&phy_access, error_control_map, bit_error_mask));
#endif

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm8x100_gen2_port_fec_error_inject_get(int unit, int port, pm_info_t pm_info,
                                           uint16_t* error_control_map,
                                           portmod_fec_error_mask_t* bit_error_mask)
{
#ifdef APERTA2_NO_SUPPORT
    portmod_access_get_params_t params;
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN
        (phymod_phy_fec_error_inject_get(&phy_access, error_control_map, bit_error_mask));
#endif
    return PHYMOD_E_NONE;
}

/*local and remote fault status get*/
int plp_aperta2_pm8x100_gen2_port_faults_status_get(int unit, int port, pm_info_t pm_info,
                                        portmod_port_fault_status_t* faults)
{
    plp_aperta2_phymod_phy_access_t phy_acc;

    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_acc));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_remote_fault_status_get(&phy_acc, &(faults->remote)));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_local_fault_status_get(&phy_acc, &(faults->local)));


    return PHYMOD_E_NONE;
}
int plp_aperta2_pm8x100_gen2_port_pmd_rx_lock_status_get(int unit, int port, pm_info_t pm_info, uint32_t* pmd_lock, uint32_t* pmd_lock_change)
{
    plp_aperta2_phymod_phy_access_t phy_access;
    int nof_phys;
    uint32_t rx_pmd_locked = 0;

    PHYMOD_NULL_CHECK(pm_info);
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_pm8x100_gen2_phy_access_get(unit, port, pm_info, &phy_access));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm8x100_gen2_port_phy_lane_access_get(unit, port, pm_info,
                                NULL, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_tscp_driver.f_phymod_phy_rx_pmd_locked_get(&phy_access, &rx_pmd_locked));

    *pmd_lock = rx_pmd_locked;
    *pmd_lock_change = 0;


    return PHYMOD_E_NONE;
}

