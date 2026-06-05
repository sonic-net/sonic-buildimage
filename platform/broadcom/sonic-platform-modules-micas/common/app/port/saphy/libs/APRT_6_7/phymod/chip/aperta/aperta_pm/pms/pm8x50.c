/*
 *
 * $Id:$
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */


#include <include/cdmac.h>
#include <include/bcmi_aperta_cdmac_defs.h>
#include <include/pm8x50.h>
#include <include/pm8x50_shared.h>
#include <include/portmod_internal.h>
#include <include/portmod.h>
#include <include/portmod_dispatch.h>
#include <include/plp_soc_portmod.h>
#include <aperta_tscbh.h>
#include <phymod/phymod.h>
#include <phymod/phymod_dispatch.h>
#include <phymod/phymod_diagnostics_dispatch.h>
#include <tier1/aperta_pm_seq.h>

#define PM_8x50_INFO(pm_info) ((pm_info)->pm_data.pm8x50_db)

/* Warmboot variable defines - start */

#define PM8x50_WB_BUFFER_VERSION        (1)
#define PM8x50_IS_CORE_INITIALIZED_SET(unit, pm_info, is_core_initialized) \
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, isCoreInitialized, is_core_initialized);
#define PM8x50_IS_CORE_INITIALIZED_GET(unit, pm_info, is_core_initialized) \
     plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, isCoreInitialized, &is_core_initialized);
#define PM8x50_IS_ACTIVE_SET(unit, pm_info, is_active)   \
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, isActive, is_active);
#define PM8x50_IS_ACTIVE_GET(unit, pm_info, is_active)    \
     plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, isActive, &is_active);

#define PM8x50_IS_BYPASSED_SET(unit, pm_info, is_bypassed)  \
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, isBypassed, is_bypassed);
#define PM8x50_IS_BYPASSED_GET(unit, pm_info, is_bypassed)  \
     plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, isBypassed, &is_bypassed);

#define PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll0_active_lane_bitmap)\
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, pll0ActiveLaneBitmap, pll0_active_lane_bitmap);
#define PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll0_active_lane_bitmap)\
    plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr,pll0ActiveLaneBitmap, &pll0_active_lane_bitmap);

#define PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, sys_pll0_active_lane_bitmap)\
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, syspll0ActiveLaneBitmap, sys_pll0_active_lane_bitmap);
#define PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, sys_pll0_active_lane_bitmap)\
    plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr,syspll0ActiveLaneBitmap, &sys_pll0_active_lane_bitmap);

#define PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll1_active_lane_bitmap)\
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, pll1ActiveLaneBitmap, pll1_active_lane_bitmap);
#define PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll1_active_lane_bitmap)\
    plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, pll1ActiveLaneBitmap, &pll1_active_lane_bitmap);

#define PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, sys_pll1_active_lane_bitmap)\
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, syspll1ActiveLaneBitmap, sys_pll1_active_lane_bitmap);
#define PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, sys_pll1_active_lane_bitmap)\
    plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, syspll1ActiveLaneBitmap, &sys_pll1_active_lane_bitmap);

#define PM8x50_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll0_adv_lane_bitmap)\
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, pll0AdvLaneBitmap, pll0_adv_lane_bitmap);
#define PM8x50_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll0_adv_lane_bitmap)\
    plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, pll0AdvLaneBitmap, &pll0_adv_lane_bitmap);

#define PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, sys_pll0_adv_lane_bitmap)\
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, syspll0AdvLaneBitmap, sys_pll0_adv_lane_bitmap);
#define PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, sys_pll0_adv_lane_bitmap)\
    plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, syspll0AdvLaneBitmap, &sys_pll0_adv_lane_bitmap);

#define PM8x50_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll1_adv_lane_bitmap)\
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, pll1AdvLaneBitmap, pll1_adv_lane_bitmap);
#define PM8x50_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll1_adv_lane_bitmap)\
    plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, pll1AdvLaneBitmap, &pll1_adv_lane_bitmap);

#define PM8x50_SYS_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, sys_pll1_adv_lane_bitmap)\
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, syspll1AdvLaneBitmap, sys_pll1_adv_lane_bitmap);
#define PM8x50_SYS_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, sys_pll1_adv_lane_bitmap)\
    plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, syspll1AdvLaneBitmap, &sys_pll1_adv_lane_bitmap);

#define PM8x50_LANE2PORT_SET(unit, pm_info, lane, port)                  \
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, lane2portMap, port);
#define PM8x50_LANE2PORT_GET(unit, pm_info, lane, port)                 \
     plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, lane2portMap, &port);

#define PM8x50_MAX_SPEED_SET(unit, pm_info, max_speed, port_index)             \
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, maxSpeed, max_speed);
#define PM8x50_MAX_SPEED_GET(unit, pm_info, max_speed, port_index)             \
     plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, maxSpeed, &max_speed);

#define PM8x50_AN_MODE_SET(unit, pm_info, an_mode, port_index)              \
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, anMode, an_mode);
#define PM8x50_AN_MODE_GET(unit, pm_info, an_mode, port_index)             \
     plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, anMode, &an_mode);

#define PM8x50_AN_FEC_SET(unit, pm_info, an_fec, port_index)              \
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, anFec, an_fec);
#define PM8x50_AN_FEC_GET(unit, pm_info, an_fec, port_index)             \
     plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, anFec, &an_fec);

#define PM8x50_FS_CL72_SET(unit, pm_info, fs_cl72, port_index)              \
    plp_aperta_plp_aperta_update_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, fsCl72, fs_cl72);
#define PM8x50_FS_CL72_GET(unit, pm_info, fs_cl72, port_index) \
          plp_aperta_plp_aperta_get_wb_pm_info(PM_8x50_INFO(pm_info)->int_phy_access.access.addr, fsCl72, &fs_cl72);

#define PM8X50_TVCO_PLL_INDEX_GET(pm_info) PM_8x50_INFO(pm_info)->int_core_access.access.tvco_pll_index
typedef enum aperta_pm8x50_wb_vars{
    isCoreInitialized,
    isActive,
    isBypassed,
    pll0ActiveLaneBitmap,
    pll1ActiveLaneBitmap,
    syspll0ActiveLaneBitmap,
    syspll1ActiveLaneBitmap,
    pll0AdvLaneBitmap,
    pll1AdvLaneBitmap,
    syspll0AdvLaneBitmap,
    syspll1AdvLaneBitmap,
    lane2portMap,
    maxSpeed,
    anMode,
    anFec,
    fsCl72,
}aperta_pm8x50_wb_vars_t;

/* Warmboot variable defines - end */

#define PORTMOD_VCO_20G 0x1 /**< Require 20G VCO */
#define PORTMOD_VCO_25G 0x2 /**< Require 25G VCO */
#define PORTMOD_VCO_26G 0x4 /**< Require 26G VCO */

#define PORTMOD_VCO_20G_SET(flags) ((flags) |= PORTMOD_VCO_20G)
#define PORTMOD_VCO_25G_SET(flags) ((flags) |= PORTMOD_VCO_25G)
#define PORTMOD_VCO_26G_SET(flags) ((flags) |= PORTMOD_VCO_26G)

#define PORTMOD_VCO_20G_CLR(flags) ((flags) &= ~PORTMOD_VCO_20G)
#define PORTMOD_VCO_25G_CLR(flags) ((flags) &= ~PORTMOD_VCO_25G)
#define PORTMOD_VCO_26G_CLR(flags) ((flags) &= ~PORTMOD_VCO_26G)

#define PORTMOD_VCO_20G_GET(flags) ((flags) & PORTMOD_VCO_20G ? 1 : 0)
#define PORTMOD_VCO_25G_GET(flags) ((flags) & PORTMOD_VCO_25G ? 1 : 0)
#define PORTMOD_VCO_26G_GET(flags) ((flags) & PORTMOD_VCO_26G ? 1 : 0)

/* Highest NRZ per lane speed is 28.125G, supported in ILKN mode */
#define NRZ_MAX_PER_LANE_SPEED 28125
#define PORTMOD_PHY_SIGNALLING_MODE_GET(speed_config) \
    (speed_config->speed /speed_config->num_lane) > NRZ_MAX_PER_LANE_SPEED ? phymodSignallingMethodPAM4 : phymodSignallingMethodNRZ

#define PM8x50_FS_ABILITY_TABLE_SIZE 29
#define PM8x50_AN_ABILITY_TABLE_SIZE 33
#define PM8x50_MAX_AN_ABILITY 20
#define APERTA_PM8x50_WB_SUPPORT 0


#define APERTA_PM8x50_CORE_ACCESS_GET(unit, port, pm_info, phy_acc) \
    do { \
        PHYMOD_MEMCPY(&phy_acc,&PM_8x50_INFO(pm_info)->int_core_access,sizeof(plp_aperta_phymod_core_access_t));\
    } while (0)

#define APERTA_PM8x50_PHY_ACCESS_GET(unit, port, pm_info, phy_acc) \
    do { \
        PHYMOD_MEMCPY(&phy_acc,&PM_8x50_INFO(pm_info)->int_phy_access,sizeof(plp_aperta_phymod_phy_access_t));\
    } while (0)

/*
 * Entries of the force speed ability table; each entry specifies a
 * unique FS port speed ability and its associated VCO rate
 */
typedef struct aperta_pm8x50_fs_ability_table_entry_s {
    uint32_t speed; /* port speed in Mbps */
    uint32_t num_lanes; /* number of lanes */
    portmod_fec_t fec_type; /* FEC type */
    portmod_vco_type_t vco; /* associated VCO rate of the ability */
} aperta_pm8x50_fs_ability_table_entry_t;

extern __phymod__dispatch__t__ plp_aperta_phymod_aperta_tscbh_driver;
extern __phymod_diagnostics__dispatch__t__ plp_aperta_phymod_diagnostics_tscbh_diagnostics_driver;
/*
 * Entries of the autoneg ability table; each entry specifies a unique
 * AN speed ability and its associated VCO rate
 */
typedef struct aperta_pm8x50_an_ability_table_entry_s {
    uint32_t speed; /* port speed in Mbps */
    uint32_t num_lanes; /* number of lanes */
    portmod_fec_t fec_type; /* FEC type */
    portmod_port_phy_control_autoneg_mode_t an_mode; /* autoneg mode such as cl73, bam or msa */
    portmod_vco_type_t vco; /* associated VCO rate of the ability */
} aperta_pm8x50_an_ability_table_entry_t;

/* a comprehensive list of pm8x50 forced speed abilities and their VCO rates */
const aperta_pm8x50_fs_ability_table_entry_t plp_aperta_pm8x50_fs_ability_table[PM8x50_FS_ABILITY_TABLE_SIZE] =
{
    /* port_speed, num_lanes, fec_type, vco */
    {10000,  1, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO20P625G},
    {10000,  1, PORTMOD_PORT_PHY_FEC_BASE_R, portmodVCO20P625G},
    {20000,  1, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO20P625G},
    {20000,  1, PORTMOD_PORT_PHY_FEC_BASE_R, portmodVCO20P625G},
    {40000,  4, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO20P625G},
    {40000,  4, PORTMOD_PORT_PHY_FEC_BASE_R, portmodVCO20P625G},
    {40000,  2, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO20P625G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_BASE_R, portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC, portmodVCO25P781G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO25P781G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC, portmodVCO25P781G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO25P781G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_RS_FEC, portmodVCO25P781G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO25P781G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_FEC, portmodVCO25P781G},
    {100000, 4, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO25P781G},
    {100000, 4, PORTMOD_PORT_PHY_FEC_RS_FEC, portmodVCO25P781G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_NONE,   portmodVCO25P781G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_RS_544, portmodVCO26P562G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_RS_272, portmodVCO26P562G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_RS_544, portmodVCO26P562G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_544, portmodVCO26P562G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_272, portmodVCO26P562G},
    {100000, 4, PORTMOD_PORT_PHY_FEC_RS_544, portmodVCO26P562G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_272, portmodVCO26P562G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_544, portmodVCO26P562G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_544_2XN, portmodVCO26P562G},
    {400000, 8, PORTMOD_PORT_PHY_FEC_RS_544_2XN, portmodVCO26P562G}
};

/* a comprehensive list of pm8x50 autoneg abilities and their VCO rates */
const aperta_pm8x50_an_ability_table_entry_t plp_aperta_pm8x50_an_ability_table[PM8x50_AN_ABILITY_TABLE_SIZE] =
{
    /* port_speed, num_lanes, fec_type, autoneg_mode, vco */
    {10000,  1, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO20P625G},
    {10000,  1, PORTMOD_PORT_PHY_FEC_BASE_R, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO20P625G},
    {20000,  1, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO20P625G},
    {20000,  1, PORTMOD_PORT_PHY_FEC_BASE_R, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO20P625G},
    {40000,  2, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO20P625G},
    {40000,  4, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO20P625G},
    {40000,  4, PORTMOD_PORT_PHY_FEC_BASE_R, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO20P625G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_BASE_R, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_BASE_R, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_BASE_R, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO25P781G},
    {25000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO25P781G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73, portmodVCO26P562G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_RS_FEC, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO25P781G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO25P781G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_RS_FEC, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA, portmodVCO25P781G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO25P781G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_RS_FEC, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO25P781G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73, portmodVCO26P562G}, /* By dafaults 544 is enabled*/
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_FEC, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO25P781G},
    {100000, 4, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73,     portmodVCO25P781G},
    {100000, 4, PORTMOD_PORT_PHY_FEC_RS_FEC, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO25P781G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_NONE,   PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73, portmodVCO26P562G},
    {50000,  1, PORTMOD_PORT_PHY_FEC_RS_544, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO26P562G},
    {50000,  2, PORTMOD_PORT_PHY_FEC_RS_544, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO26P562G},
    {100000, 2, PORTMOD_PORT_PHY_FEC_RS_544, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO26P562G},
    {100000, 4, PORTMOD_PORT_PHY_FEC_RS_544, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO26P562G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_544_2XN, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73    , portmodVCO26P562G},
    {200000, 4, PORTMOD_PORT_PHY_FEC_RS_544, PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM, portmodVCO26P562G}
};

/*
 * given the port speed, number of lanes, and FEC type, return the VCO specified in
 * plp_aperta_pm8x50_fs_ability_table in the associated entry; VCO should be portmodVCOInvalid
 * if the ability is invalid
 */
STATIC
int _plp_aperta_pm8x50_fs_ability_table_vco_get(uint32_t speed, uint32_t num_lanes, portmod_fec_t fec_type,
                                     portmod_vco_type_t* vco)
{
    int i;

    *vco = portmodVCOInvalid;
    for (i = 0; i < PM8x50_FS_ABILITY_TABLE_SIZE; i++) {
        if (plp_aperta_pm8x50_fs_ability_table[i].speed == speed &&
            plp_aperta_pm8x50_fs_ability_table[i].num_lanes == num_lanes &&
            plp_aperta_pm8x50_fs_ability_table[i].fec_type == fec_type) {
            *vco = plp_aperta_pm8x50_fs_ability_table[i].vco;
            break;
        }
    }
    if (*vco == portmodVCOInvalid) {
        PHYMOD_DEBUG_ERROR(("Invalid VCO for Speed:%d Fec:%s\n", speed,
                    (fec_type == _SHR_PORT_PHY_FEC_NONE) ? "NONE" :
                    (fec_type == _SHR_PORT_PHY_FEC_BASE_R) ? "BASE_R" :
                    (fec_type == _SHR_PORT_PHY_FEC_RS_FEC) ? "RSFEC" :
                    (fec_type == _SHR_PORT_PHY_FEC_RS_544) ? "RS544" :
                    (fec_type == _SHR_PORT_PHY_FEC_RS_272) ? "RS272" :
                    (fec_type == _SHR_PORT_PHY_FEC_RS_544_2XN) ? "RS544_2X": "NONE"));
    }

    return PHYMOD_E_NONE;
}

/*
 * given the port speed, number of lanes, FEC type, and autoneg mode, return the VCO specified in
 * plp_aperta_pm8x50_an_ability_table; VCO should be portmodVCOInvalid if the ability is invalid.
 * this function is temporarily masked out because there is no caller. advert_ability_get() will
 * need to call this function to verify the VCO requirement
 */
STATIC
int _plp_aperta_pm8x50_an_ability_table_vco_get(uint32_t speed, uint32_t num_lanes, portmod_fec_t fec_type,
                                     portmod_port_phy_control_autoneg_mode_t an_mode,
                                     portmod_vco_type_t* vco)
{
    int i;

    *vco = portmodVCOInvalid;
    for (i = 0; i < PM8x50_AN_ABILITY_TABLE_SIZE; i++) {
        if (plp_aperta_pm8x50_an_ability_table[i].speed == speed &&
            plp_aperta_pm8x50_an_ability_table[i].num_lanes == num_lanes &&
            plp_aperta_pm8x50_an_ability_table[i].fec_type == fec_type &&
            plp_aperta_pm8x50_an_ability_table[i].an_mode == an_mode)
        {
            *vco = plp_aperta_pm8x50_an_ability_table[i].vco;
            break;
        }
    }

    return PHYMOD_E_NONE;
}
/* Not used as local get is not req*/
#if 0
/* read an entry of plp_aperta_pm8x50_fs_ability_table[] */
STATIC
int _aperta_pm8x50_fs_ability_table_read_entry(int index, aperta_pm8x50_fs_ability_table_entry_t *table_entry)
{
    if (index < 0 || index >= PM8x50_FS_ABILITY_TABLE_SIZE) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, ("index to read in plp_aperta_pm8x50_fs_ability_table is out of boundary"));
    }

    *table_entry = plp_aperta_pm8x50_fs_ability_table[index];
    return PHYMOD_E_NONE;
}

/* read an entry of plp_aperta_pm8x50_an_ability_table[] */
STATIC
int _aperta_pm8x50_an_ability_table_read_entry(int index, aperta_pm8x50_an_ability_table_entry_t *table_entry)
{
    if (index < 0 || index >= PM8x50_AN_ABILITY_TABLE_SIZE) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, ("index to read in plp_aperta_pm8x50_an_ability_table is out of boundary"));
    }

    *table_entry = plp_aperta_pm8x50_an_ability_table[index];

    return PHYMOD_E_NONE;
}
#endif
STATIC
int _plp_aperta_pm8x50_port_index_get(int unit, int port, pm_info_t pm_info,
                           int *first_index, uint32_t *bitmap)
{
   int i, tmp_port = 0;

   *first_index = -1;
   *bitmap = 0;

   for( i = 0 ; i < MAX_PORTS_PER_PM8X50; i++){
       PM8x50_LANE2PORT_GET(unit, pm_info, i, tmp_port);

       if(tmp_port == port){
           *first_index = (*first_index == -1 ? i : *first_index);
           SHR_BITSET(*bitmap, i);
       }
   }

   if(*first_index == -1) {
       PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL,
              (APERTA_SOC_MSG("port was not found in internal DB %d"), port));
   }


    return PHYMOD_E_NONE;
}

/*Get whether the inerface type is supported by PM */
int plp_aperta_pm8x50_pm_interface_type_is_supported(int unit,
                                          soc_port_if_t interface_t,
                                          int* is_supported)
{

    *is_supported = TRUE;
    return (PHYMOD_E_NONE);
}


/*
 * Function:
 *      plp_aperta_portmod_aperta_pm8x50_wb_upgrade_func
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

int plp_aperta_portmod_aperta_pm8x50_wb_upgrade_func(int unit, void *arg, int recovered_version,
                                   int new_version)
{
    return (PHYMOD_E_NONE);
}

/*
 * Initialize the buffer to support warmboot
 * The state of warmboot is store in the variables like
 * isInitialized, isActive, isBypassed, ports.. etc.,
 * All of these variables need to be added to warmboot
 * any variables added to save the state of warmboot should be
 * included here.
 */
#if 0
STATIC
int aperta_pm8x50_wb_buffer_init(int unit, int wb_buffer_index, pm_info_t pm_info)
{
    /* Declare the common variables needed for warmboot */
    WB_ENGINE_INIT_TABLES_DEFS;
    int wb_var_id, rv;
    int buffer_id = wb_buffer_index; /*required by SOC_WB_ENGINE_ADD_ Macros*/


    COMPILER_REFERENCE(buffer_is_dynamic);

    SOC_WB_ENGINE_ADD_BUFF(SOC_WB_ENGINE_PORTMOD, wb_buffer_index, "pm8x50",
                           plp_aperta_portmod_aperta_pm8x50_wb_upgrade_func, pm_info,
                           PM8x50_WB_BUFFER_VERSION, 1,
                           SOC_WB_ENGINE_PRE_RELEASE);
    PHYMOD_IF_ERR_RETURN(rv);

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id,
                         "is_core_initialized", wb_buffer_index, sizeof(int),
                          NULL, MAX_PORTS_PER_PM8X50, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[isCoreInitialized] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "is_active",
                          wb_buffer_index, sizeof(uint32_t), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[isActive] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "is_bypassed",
                          wb_buffer_index, sizeof(uint32_t), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[isBypassed] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "pll0_active_lane_bitmap",
                          wb_buffer_index, sizeof(uint8_t), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[pll0ActiveLaneBitmap] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "pll1_active_lane_bitmap",
                          wb_buffer_index, sizeof(uint8_t), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[pll1ActiveLaneBitmap] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "pll0_adv_lane_bitmap",
                          wb_buffer_index, sizeof(uint8_t), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[pll0AdvLaneBitmap] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_VAR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "pll1_adv_lane_bitmap",
                          wb_buffer_index, sizeof(uint8_t), NULL, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[pll1AdvLaneBitmap] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "lane2portMap",
                          wb_buffer_index, sizeof(int), NULL, MAX_PORTS_PER_PM8X50, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[lane2portMap] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "max_speed",
                          wb_buffer_index, sizeof(uint32_t), NULL, MAX_PORTS_PER_PM8X50, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[maxSpeed] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "an_mode",
                          wb_buffer_index, sizeof(int), NULL, MAX_PORTS_PER_PM8X50, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[anMode] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "an_fec",
                          wb_buffer_index, sizeof(uint32_t), NULL, MAX_PORTS_PER_PM8X50, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[anFec] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(portmod_next_wb_var_id_get(unit, &wb_var_id));
    SOC_WB_ENGINE_ADD_ARR(SOC_WB_ENGINE_PORTMOD, wb_var_id, "fs_cl72",
                          wb_buffer_index, sizeof(uint32_t), NULL, MAX_PORTS_PER_PM8X50, VERSION(1));
    PHYMOD_IF_ERR_RETURN(rv);
    pm_info->wb_vars_ids[fsCl72] = wb_var_id;

    PHYMOD_IF_ERR_RETURN(soc_wb_engine_init_buffer(unit, SOC_WB_ENGINE_PORTMOD,
                                               wb_buffer_index, FALSE));

    return PHYMOD_E_NONE;
}
#endif

plp_aperta_phymod_bus_t plp_aperta_pm8x50_default_bus = {
    "PM8x50 Bus",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    PHYMOD_BUS_CAP_WR_MODIFY | PHYMOD_BUS_CAP_LANE_CTRL
};

/*Add new pm.*/
int plp_aperta_pm8x50_pm_init(int unit,
                   const portmod_pm_create_info_internal_t* pm_add_info,
                   int wb_buffer_index,
                   pm_info_t pm_info)
{
    const portmod_pm8x50_create_info_internal_t *info =
                &pm_add_info->pm_specific_info.pm8x50;
    pm8x50_t aperta_pm8x50_data = NULL;
    int bypass_enable;
    int is_core_initialized;
    uint8_t pll0_adv_lane_bitmap, pll1_adv_lane_bitmap;
    uint8_t pll0_active_lane_bitmap, pll1_active_lane_bitmap;
#if APERTA_PM8x50_WB_SUPPORT
    int probe= 0;
#endif

    pm_info->type = pm_add_info->type;
    pm_info->unit = unit;
    pm_info->wb_buffer_id = wb_buffer_index;

    /* PM8x50 specific info */
    aperta_pm8x50_data = PHYMOD_MALLOC(sizeof(struct pm8x50_s), "specific_db");
    PHYMOD_NULL_CHECK(aperta_pm8x50_data);
    pm_info->pm_data.pm8x50_db = aperta_pm8x50_data;

    aperta_pm8x50_data->int_core_access.type = phymodDispatchTypeCount;
    aperta_pm8x50_data->first_phy = -1;
    aperta_pm8x50_data->warmboot_skip_db_restore = TRUE;
    aperta_pm8x50_data->rescal = info->rescal;
    
    aperta_pm8x50_data->portmod_mac_soft_reset = info->portmod_mac_soft_reset;

    /* init intertnal SerDes core access */
    plp_aperta_phymod_core_access_t_init(&aperta_pm8x50_data->int_core_access);

    PHYMOD_MEMCPY(&aperta_pm8x50_data->polarity, &info->polarity,
               sizeof(plp_aperta_phymod_polarity_t));
    PHYMOD_MEMCPY(&(aperta_pm8x50_data->int_core_access.access), &info->access.access,
                sizeof(plp_aperta_phymod_access_t));
    PHYMOD_MEMCPY(&(aperta_pm8x50_data->int_phy_access), &(aperta_pm8x50_data->int_core_access),
                sizeof(plp_aperta_phymod_phy_access_t));

    PHYMOD_MEMCPY(&aperta_pm8x50_data->lane_map, &info->lane_map,
                sizeof(aperta_pm8x50_data->lane_map));
    aperta_pm8x50_data->ref_clk = info->ref_clk;
    aperta_pm8x50_data->fw_load_method = info->fw_load_method;
    aperta_pm8x50_data->external_fw_loader = info->external_fw_loader;
    aperta_pm8x50_data->tvco = info->tvco;
    aperta_pm8x50_data->ovco = info->ovco;
    aperta_pm8x50_data->core_num  = info->core_num;
    aperta_pm8x50_data->afe_pll = info->afe_pll;

    if(info->access.access.bus == NULL) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, ("Bus cannot be NULL for APERTA\n"));
    }

    /*init wb buffer
    PHYMOD_IF_ERR_RETURN(aperta_pm8x50_wb_buffer_init(unit, wb_buffer_index,  pm_info));*/
#if APERTA_PM8x50_WB_SUPPORT
    if(SOC_WARM_BOOT(unit)){
        PHYMOD_IF_ERR_RETURN(portmod_common_serdes_probe(aperta_pm8x50_serdes_list, &aperta_pm8x50_data->int_core_access, &probe));
        if (!probe) {
            LOG_ERROR(BSL_LS_SOC_PORT,
                  (BSL_META_U(unit, "ERROR: serdes probe failed type=%d\n"), aperta_pm8x50_data->int_core_access.type));
        }
    }
#endif
#if APERTA_PM8x50_WB_SUPPORT
    if(!SOC_WARM_BOOT(unit)){
#endif
        is_core_initialized = 0;
        PM8x50_IS_CORE_INITIALIZED_SET(unit, pm_info,
                                            is_core_initialized);
        bypass_enable = 0;
        PM8x50_IS_BYPASSED_SET(unit, pm_info, bypass_enable);

        pll0_adv_lane_bitmap = 0;
        PM8x50_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll0_adv_lane_bitmap);
        PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll0_adv_lane_bitmap);

        pll1_adv_lane_bitmap = 0;
        PM8x50_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll1_adv_lane_bitmap);
        PM8x50_SYS_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll1_adv_lane_bitmap);

        pll0_active_lane_bitmap = 0;
        PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll0_active_lane_bitmap);
        PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll0_active_lane_bitmap);

        pll1_active_lane_bitmap = 0;
        PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll1_active_lane_bitmap);
        PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll1_active_lane_bitmap);
#if APERTA_PM8x50_WB_SUPPORT
    }
#endif

    return PHYMOD_E_NONE;
}

/*Release PM resources*/
int plp_aperta_pm8x50_pm_destroy(int unit, pm_info_t pm_info)
{
    if(pm_info->pm_data.pm8x50_db != NULL){
        PHYMOD_FREE(pm_info->pm_data.pm8x50_db);
        pm_info->pm_data.pm8x50_db = NULL;
    }

    return PHYMOD_E_NONE;
}

/* This function will check the speed config is valid or not, and return required vco for valid input */
/* The supported speed config in this function is based on PM8x50 Portmod Spec */
STATIC
int _plp_aperta_pm8x50_port_speed_config_to_vco_get(const portmod_speed_config_t* speed_config,
                                         portmod_vco_type_t* vco)
{
    PHYMOD_IF_ERR_RETURN(
        _plp_aperta_pm8x50_fs_ability_table_vco_get(speed_config->speed,
                                             speed_config->num_lane,
                                             speed_config->fec,
                                             vco));
    /*
     * when *vco == portmodVCOInvalid, it means the entered combination of
     * port speed, number of lanes, and FEC type is not supported.
     */
    if (*vco == portmodVCOInvalid) {
        return PHYMOD_E_CONFIG;
    }
    return PHYMOD_E_NONE;
}

/*Get the suggested VCO values based on the speed config list*/
int plp_aperta_pm8x50_pm_vcos_get(int unit, portmod_dispatch_type_t pm_type, portmod_pm_vco_setting_t* vco_select)
{
    uint8_t vcos = 0;
    portmod_vco_type_t vco;
    int i, rv = 0;

    if(vco_select == NULL){
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("vco_select NULL parameter")));
    }

    for (i = 0; i < vco_select->num_speeds; i++) {
        vco = portmodVCOInvalid;
        rv = _plp_aperta_pm8x50_port_speed_config_to_vco_get(&(vco_select->speed_config_list[i]), &vco);
        if (rv == PHYMOD_E_CONFIG) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG,
                      (APERTA_SOC_MSG("Speed config is not supported")));
        }
        if (vco == portmodVCO20P625G) {
            PORTMOD_VCO_20G_SET(vcos);
        } else if (vco == portmodVCO25P781G) {
            PORTMOD_VCO_25G_SET(vcos);
        } else if (vco == portmodVCO26P562G) {
            PORTMOD_VCO_26G_SET(vcos);
        }
    }

    vco_select->tvco = portmodVCOInvalid;
    vco_select->ovco = portmodVCOInvalid;
    vco_select->is_tvco_new = 0;
    vco_select->is_ovco_new = 0;

    if (PORTMOD_VCO_26G_GET(vcos)) {
        vco_select->tvco = portmodVCO26P562G;
        vco_select->is_tvco_new = 1;
        if (PORTMOD_VCO_25G_GET(vcos)) {
            vco_select->ovco = portmodVCO25P781G;
            vco_select->is_ovco_new = 1;
        } else if (PORTMOD_VCO_20G_GET(vcos)) {
            vco_select->ovco = portmodVCO20P625G;
            vco_select->is_ovco_new = 1;
        }
    } else if (PORTMOD_VCO_25G_GET(vcos)) {
        vco_select->tvco = portmodVCO25P781G;
        vco_select->is_tvco_new = 1;
        if (PORTMOD_VCO_20G_GET(vcos)) {
            vco_select->ovco = portmodVCO20P625G;
            vco_select->is_ovco_new = 1;
        }
    } else if (PORTMOD_VCO_20G_GET(vcos)) {
        vco_select->tvco = portmodVCO20P625G;
        vco_select->is_tvco_new = 1;
    }

    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta_pm8x50_port_tsc_reset_set(int unit, plp_aperta_phymod_phy_access_t *phy_acc, int in_reset)
{
    BCMI_APERTA_CDMAC_CDPORT_XGXS0_CTRLr_t cdport_ctrl;

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_CDMAC_READ_CDPORT_XGXS0_CTRLr(phy_acc, &cdport_ctrl));

    /* Bring Internal Phy OOR */
    BCMI_APERTA_CDMAC_CDPORT_XGXS0_CTRLr_TSC_RSTBf_SET(cdport_ctrl, in_reset ? 0 : 1);

    /* as a default for PM8x50, always use PLL1 as the tsc clock source */
    BCMI_APERTA_CDMAC_CDPORT_XGXS0_CTRLr_TSC_CLK_SELf_SET(cdport_ctrl, 1);
    BCMI_APERTA_CDMAC_CDPORT_XGXS0_CTRLr_TSC_PWRDWNf_SET(cdport_ctrl,  in_reset ? 1 : 0);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_CDMAC_WRITE_CDPORT_XGXS0_CTRLr(phy_acc, cdport_ctrl));

    /* Based on the feedback from SJ pmd support team, ~10-15usecs would be sufficient
     * for the PLL/AFE to settle down out of IDDQ reset.
     */
    PHYMOD_USLEEP(10);


    return PHYMOD_E_NONE;
}


/*Enable port macro.*/
/* This function contains 3 parts:
 * 1. Bring Serdes out of hard reset.
 * 2. Bring 2 CDMACs out of hard reset.
 */
int plp_aperta_pm8x50_pm_enable(int unit,
                     int pm_id,
                     pm_info_t pm_info,
                     int enable)
{
    BCMI_APERTA_CDMAC_CDPORT_MAC_CONTROLr_t reg_val;
    plp_aperta_phymod_phy_access_t phy_acc;
    int is_reset;

    APERTA_PM8x50_PHY_ACCESS_GET(unit, 0, pm_info, phy_acc);

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_CDMAC_READ_CDPORT_MAC_CONTROLr(&phy_acc, &reg_val));
    is_reset = BCMI_APERTA_CDMAC_CDPORT_MAC_CONTROLr_CDMAC0_RESETf_GET(reg_val);

    if(enable && is_reset) {
        /* Bring Serdes OOR */
        PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_port_tsc_reset_set(unit, &phy_acc, 0));

        /* Bring MAC OOR */
        BCMI_APERTA_CDMAC_CDPORT_MAC_CONTROLr_CDMAC0_RESETf_SET(reg_val, 0);
        BCMI_APERTA_CDMAC_CDPORT_MAC_CONTROLr_CDMAC1_RESETf_SET(reg_val, 0);
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_CDMAC_WRITE_CDPORT_MAC_CONTROLr(&phy_acc, reg_val));
    } else if ((!enable) && (!is_reset)){ /* disable */
        /* put MAC in reset */
        BCMI_APERTA_CDMAC_CDPORT_MAC_CONTROLr_CDMAC0_RESETf_SET(reg_val, 1);
        BCMI_APERTA_CDMAC_CDPORT_MAC_CONTROLr_CDMAC1_RESETf_SET(reg_val, 1);
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_CDMAC_WRITE_CDPORT_MAC_CONTROLr(&phy_acc, reg_val));

        /* Put Serdes in reset*/
        PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_port_tsc_reset_set(unit, &phy_acc, 1));
    }


    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta_pm8x50_pm_core_probe(int unit, pm_info_t pm_info, const portmod_port_add_info_t* add_info)
{
    uint32_t probe =0;
    plp_aperta_phymod_core_access_t core_acc;
    APERTA_PM8x50_CORE_ACCESS_GET(unit, 0, pm_info, core_acc);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_phymod_aperta_tscbh_driver.f_phymod_core_identify(&core_acc, 0, &probe));
    if (probe) {
       return PHYMOD_E_NONE;
    } else {
        PHYMOD_DEBUG_ERROR(("Aperta probe Failed\n"));
       return PHYMOD_E_UNAVAIL;
    }
    return PHYMOD_E_NONE;
}

/* This function will return required pll based on the ref_clk and vco */
/* For now, assume PM8x50 only support:
   VCO: 20.625G, 25.78125G,26.5625G;
   Ref_CLK: 156.25M, 312.5M
  For any other input, this function will return E_PARAM
*/
STATIC
int _plp_aperta_pm8x50_vco_to_pll_get(plp_aperta_phymod_ref_clk_t ref_clock, portmod_vco_type_t vco, uint32_t* pll)
{
    int rv = PHYMOD_E_NONE;


    switch (vco) {
        case portmodVCO20P625G:
            if (ref_clock == phymodRefClk156Mhz) {
                *pll = phymod_APERTA_TSCBH_PLL_DIV132;
            } else if (ref_clock == phymodRefClk312Mhz) {
                *pll = phymod_APERTA_TSCBH_PLL_DIV66;
            } else {
                rv = PHYMOD_E_PARAM;
            }
            break;
        case portmodVCO25P781G:
            if (ref_clock == phymodRefClk156Mhz) {
                *pll = phymod_APERTA_TSCBH_PLL_DIV165;
            } else if (ref_clock == phymodRefClk312Mhz) {
                *pll = phymod_APERTA_TSCBH_PLL_DIV82P5;
            } else {
                rv = PHYMOD_E_PARAM;
            }
            break;
        case portmodVCO26P562G:
            if (ref_clock == phymodRefClk156Mhz) {
                *pll = phymod_APERTA_TSCBH_PLL_DIV170;
            } else if (ref_clock == phymodRefClk312Mhz) {
                *pll = phymod_APERTA_TSCBH_PLL_DIV85;
            } else {
                rv = PHYMOD_E_PARAM;
            }
            break;
       case portmodVCOInvalid:
            *pll = phymod_APERTA_TSCBH_PLL_DIVNONE;
            break;
       default:
            rv = PHYMOD_E_PARAM;
            break;
    }
    PHYMOD_IF_ERR_RETURN(rv);


     return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta_pm8x50_pm_serdes_core_init(int unit, pm_info_t pm_info, const portmod_port_add_info_t* add_info)
{
    plp_aperta_phymod_core_init_config_t core_conf;
    plp_aperta_phymod_core_status_t core_status;
    int core_is_initialized;
    uint32_t init_flags = 0;
    uint32_t pll0 = 0, pll1 = 0;
    portmod_vco_type_t ovco = 0, tvco = 0;

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_core_init_config_t_init(&core_conf));

    
    core_conf.firmware_load_method = PM_8x50_INFO(pm_info)->fw_load_method;
    core_conf.firmware_loader = PM_8x50_INFO(pm_info)->external_fw_loader;
    core_conf.lane_map = PM_8x50_INFO(pm_info)->lane_map;
    core_conf.polarity_map = PM_8x50_INFO(pm_info)->polarity;

    core_conf.afe_pll.afe_pll_change_default = PM_8x50_INFO(pm_info)->afe_pll.afe_pll_change_default;
    core_conf.afe_pll.ams_pll_iqp = PM_8x50_INFO(pm_info)->afe_pll.ams_pll_iqp;
    core_conf.afe_pll.ams_pll_en_hrz = PM_8x50_INFO(pm_info)->afe_pll.ams_pll_en_hrz;

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_phy_inf_config_t_init(&core_conf.interface));
    core_conf.interface.ref_clock = PM_8x50_INFO(pm_info)->ref_clk;

    if ((PM_8x50_INFO(pm_info)->ovco == portmodVCOInvalid) && (PM_8x50_INFO(pm_info)->tvco == portmodVCOInvalid)) {
        ovco = add_info->ovco;
        tvco = add_info->tvco;
        PM_8x50_INFO(pm_info)->ovco = ovco;
        PM_8x50_INFO(pm_info)->tvco = tvco;
        PM_8x50_INFO(pm_info)->sys_ovco = ovco;
        PM_8x50_INFO(pm_info)->sys_tvco = tvco;
    } else {
        ovco = PM_8x50_INFO(pm_info)->ovco;
        tvco = PM_8x50_INFO(pm_info)->tvco;
    }
    if ((tvco < portmodVCOInvalid) || (tvco >= portmodVCOCount)) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (APERTA_SOC_MSG("TVCO configuration is invalid.")));
    }
    if (PM_8x50_INFO(pm_info)->int_core_access.access.tvco_pll_index) {
    PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_vco_to_pll_get(core_conf.interface.ref_clock, ovco, &pll0));
    PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_vco_to_pll_get(core_conf.interface.ref_clock, tvco, &pll1));
    } else {
        PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_vco_to_pll_get(core_conf.interface.ref_clock, ovco, &pll1));
        PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_vco_to_pll_get(core_conf.interface.ref_clock, tvco, &pll0));
    }
    core_conf.pll0_div_init_value = pll0;
    core_conf.pll1_div_init_value = pll1;

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_core_status_t_init(&core_status));
    core_status.pmd_active = 0;
    init_flags = PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info) | PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info);

    if (PORTMOD_PORT_ADD_F_FIRMWARE_LOAD_VERIFY_GET(add_info)) {
        PHYMOD_CORE_INIT_F_FIRMWARE_LOAD_VERIFY_SET(&core_conf);
    }
    if (PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info)) {
        PHYMOD_CORE_INIT_F_EXECUTE_PASS1_SET(&core_conf);
    }
    if (PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info)) {
        PHYMOD_CORE_INIT_F_EXECUTE_PASS2_SET(&core_conf);
    }
    if (PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info)) {
        PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_SET(&core_conf);
    }

    PM8x50_IS_CORE_INITIALIZED_GET(unit, pm_info, core_is_initialized)

    if (!PORTMOD_CORE_INIT_FLAG_INITIALZIED_GET(core_is_initialized)) {
         /* firmware load will happen after pass 1 */
         if (!PORTMOD_CORE_INIT_FLAG_FIRMWARE_LOADED_GET(core_is_initialized) ||
             PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info)) {
             PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_core_init(&PM_8x50_INFO(pm_info)->int_core_access,
                                               &core_conf,
                                               &core_status));
            if (PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info) || init_flags == 0) {
                 if ((PM_8x50_INFO(pm_info)->int_core_access.port_loc == phymodPortLocSys) ||
                    PM_8x50_INFO(pm_info)->int_core_access.port_loc == phymodPortLocDC) {
                PORTMOD_CORE_INIT_FLAG_FIRMWARE_LOADED_SET(core_is_initialized);
                PM8x50_IS_CORE_INITIALIZED_SET(unit, pm_info, core_is_initialized)
            }
             }
            if (PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info) || init_flags == 0) {
                if ((PM_8x50_INFO(pm_info)->int_core_access.port_loc == phymodPortLocSys) ||
                    PM_8x50_INFO(pm_info)->int_core_access.port_loc == phymodPortLocDC) {
                    PORTMOD_CORE_INIT_FLAG_INITIALZIED_SET(core_is_initialized);
                    PM8x50_IS_CORE_INITIALIZED_SET(unit, pm_info, core_is_initialized)
                }
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
int plp_aperta_pm8x50_pm_serdes_core_init(int unit,
                               int pm_id,
                               pm_info_t pm_info,
                               const portmod_port_add_info_t* add_info)
{
    int init_all = 0;
    int rv = PHYMOD_E_NONE;

    init_all = (!PORTMOD_PORT_ADD_F_INIT_CORE_PROBE_GET(add_info) &&
                !PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info) &&
                !PORTMOD_PORT_ADD_F_INIT_PASS2_GET(add_info)) ? 1 : 0;

    /* probe serdes core */
    if (PORTMOD_PORT_ADD_F_INIT_CORE_PROBE_GET(add_info) || init_all) {
        rv = _plp_aperta_pm8x50_pm_core_probe(unit, pm_info, add_info);
        PHYMOD_IF_ERR_RETURN(rv);
    }

    /* Return here if caller only request Core Probe */
    if (!(PORTMOD_PORT_ADD_F_INIT_PASS1_GET(add_info)) &&
        (PORTMOD_PORT_ADD_F_INIT_CORE_PROBE_GET(add_info))) {
        return (rv);
    }

    /* core config for internal serdes. */
    rv = _plp_aperta_pm8x50_pm_serdes_core_init(unit, pm_info, add_info);
    PHYMOD_IF_ERR_RETURN(rv);


    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta_pm8x50_pm_port_init(int unit,
                         int port,
                         pm_info_t pm_info,
                         int internal_port,
                         const portmod_port_add_info_t* add_info,
                         int enable)
{
    int rv;
    BCMI_APERTA_CDMAC_CDPORT_FAULT_LINK_STATUSr_t reg_val;
    uint32_t flags;
    uint32_t rsv_mask;
    plp_aperta_phymod_phy_access_t phy_acc;
    int lane = 0, mac_index = 0, act_lm = 0;

    APERTA_PM8x50_PHY_ACCESS_GET(unit, 0, pm_info, phy_acc);

    if (internal_port == -1)  {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (APERTA_SOC_MSG("Invalid internal Port %d"),
                                internal_port));
    }

    if (enable) {
        /* RSV Mask */
        
        rsv_mask = 0;
        rsv_mask |= (1 << 3); /* Receive terminate/code error */
        rsv_mask |= (1 << 4); /* CRC error */
        rsv_mask |= (1 << 6); /* Truncated/Frame out of Range */
        rsv_mask |= (1 << 17); /* RUNT detected*/
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_mac_rsv_mask_set(unit, port, pm_info, rsv_mask));

        /* Init MAC */
        flags = 0;
        flags |= CDMAC_INIT_F_RX_STRIP_CRC;
        flags |= CDMAC_INIT_F_TX_APPEND_CRC;
         act_lm = phy_acc.access.lane_mask;
        if (plp_aperta_count_no_bits(phy_acc.access.lane_mask) == 2) {
            flags = add_info->speed_config.speed ;
            mac_index = (phy_acc.access.lane_mask & 0xF) ? 0 : 1;
            for (lane = 0; lane <2;lane++) {
                phy_acc.access.lane_mask = 3 << ((lane*2) + (mac_index *4));
                /* Update new speed*/
                PHYMOD_IF_ERR_RETURN(
                      plp_aperta_pm_info_speed_set(&phy_acc, add_info->speed_config.speed,unit));
                rv = plp_aperta_cdmac_init(&phy_acc, flags);
                PHYMOD_IF_ERR_RETURN(rv);
                /* LSS */
                PHYMOD_IF_ERR_RETURN(READ_CDPORT_FAULT_LINK_STATUSr(&phy_acc, &reg_val));
                BCMI_APERTA_CDMAC_CDPORT_FAULT_LINK_STATUSr_REMOTE_FAULTf_SET(reg_val, 1);
                BCMI_APERTA_CDMAC_CDPORT_FAULT_LINK_STATUSr_LOCAL_FAULTf_SET(reg_val, 1);
                PHYMOD_IF_ERR_RETURN(WRITE_CDPORT_FAULT_LINK_STATUSr(&phy_acc, reg_val));
            }
        } else {
            if (add_info->speed_config.speed == 10000 || 
                add_info->speed_config.speed == 25000 ||
                add_info->speed_config.speed == 50000) {
                 flags = add_info->speed_config.speed ;
                 mac_index = (phy_acc.access.lane_mask & 0xF) ? 0 : 1;
                 for (lane = 0; lane <4;lane++) {
                     phy_acc.access.lane_mask = 1 << (lane + (mac_index *4));
                     if (act_lm &  phy_acc.access.lane_mask) {
                         /* Update new speed*/
                         PHYMOD_IF_ERR_RETURN(
                               plp_aperta_pm_info_speed_set(&phy_acc, add_info->speed_config.speed,unit));
                         rv = plp_aperta_cdmac_init(&phy_acc, flags);
                         PHYMOD_IF_ERR_RETURN(rv);
                         /* LSS */
                         PHYMOD_IF_ERR_RETURN(READ_CDPORT_FAULT_LINK_STATUSr(&phy_acc, &reg_val));
                         BCMI_APERTA_CDMAC_CDPORT_FAULT_LINK_STATUSr_REMOTE_FAULTf_SET(reg_val, 1);
                         BCMI_APERTA_CDMAC_CDPORT_FAULT_LINK_STATUSr_LOCAL_FAULTf_SET(reg_val, 1);
                         PHYMOD_IF_ERR_RETURN(WRITE_CDPORT_FAULT_LINK_STATUSr(&phy_acc, reg_val));
                     }
                 }
            } else {
                 flags = add_info->speed_config.speed ;
                 rv = plp_aperta_cdmac_init(&phy_acc, flags);
                 PHYMOD_IF_ERR_RETURN(rv);

                 /* LSS */
                 PHYMOD_IF_ERR_RETURN(READ_CDPORT_FAULT_LINK_STATUSr(&phy_acc, &reg_val));
                 BCMI_APERTA_CDMAC_CDPORT_FAULT_LINK_STATUSr_REMOTE_FAULTf_SET(reg_val, 1);
                 BCMI_APERTA_CDMAC_CDPORT_FAULT_LINK_STATUSr_LOCAL_FAULTf_SET(reg_val, 1);
                 PHYMOD_IF_ERR_RETURN(WRITE_CDPORT_FAULT_LINK_STATUSr(&phy_acc, reg_val));
            }
        }

        /* Reset MIB counters */
        
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_mib_reset_toggle(unit, port, pm_info, 0));
    }
    return PHYMOD_E_NONE;
}

#define CDMAC_NUM_LANES 4

#define aperta_soc_reg_field_set(PHY, REGNAME, REG_INS, FIELD, VALUE)   BCMI_APERTA_CDMAC_##REGNAME##_##FIELD##_SET(REG_INS, VALUE)
#define aperta_soc_reg_field_get(PHY, REGNAME, REG_INS, FIELD)          BCMI_APERTA_CDMAC_##REGNAME##_##FIELD##_GET(REG_INS)

STATIC
int _plp_aperta_pm8x50_pm_port_mode_update(int unit,
                                int port,
                                pm_info_t pm_info,
                                int first_phy_index,
                                int num_lanes)
{
    BCMI_APERTA_CDMAC_CDPORT_MODEr_t reg_val;
    uint32_t mac_port_mode, mac_new_port_mode = 0;
    int first_lane_local = 0, mac_stage_id = 0;
    plp_aperta_phymod_phy_access_t phy_acc;

    APERTA_PM8x50_PHY_ACCESS_GET(unit, port, pm_info, phy_acc);
    PHYMOD_MEMSET(&reg_val, 0, sizeof(BCMI_APERTA_CDMAC_CDPORT_MODEr_t));

    mac_stage_id = first_phy_index / CDMAC_NUM_LANES;
    first_lane_local = first_phy_index % CDMAC_NUM_LANES;
    PHYMOD_IF_ERR_RETURN(READ_CDPORT_MODEr(&phy_acc, &reg_val));

    if (mac_stage_id) {
        /* MAC1 need to update port mode */
        mac_port_mode = BCMI_APERTA_CDMAC_CDPORT_MODEr_MAC1_PORT_MODEf_GET(reg_val);
    } else {
        /* MAC0 need to update port mode */
        mac_port_mode = BCMI_APERTA_CDMAC_CDPORT_MODEr_MAC0_PORT_MODEf_GET(reg_val);
    }
    if (num_lanes == 8) {
        /* 400G single port mode */
        aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, SINGLE_PORT_MODE_SPEED_400Gf, 1);
        aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC1_PORT_MODEf, CDMAC_4_LANES_TOGETHER);
        aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC0_PORT_MODEf, CDMAC_4_LANES_TOGETHER);
    } else if (num_lanes == 4) {
        if (aperta_soc_reg_field_get(unit, CDPORT_MODEr, reg_val, SINGLE_PORT_MODE_SPEED_400Gf) == 1) {
            aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, SINGLE_PORT_MODE_SPEED_400Gf, 0);
            aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC1_PORT_MODEf, 0);
            aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC0_PORT_MODEf, 0);
        }
        if (mac_stage_id) {
            /* MAC1 is single-port mode */
            aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC1_PORT_MODEf, CDMAC_4_LANES_TOGETHER);
        } else {
            /* MAC0 is single-port mode */
            aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC0_PORT_MODEf, CDMAC_4_LANES_TOGETHER);
        }
    } else if (num_lanes == 2) {
        aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, SINGLE_PORT_MODE_SPEED_400Gf, 0);
        switch (mac_port_mode) {
            case CDMAC_4_LANES_SEPARATE:
                if (first_lane_local == 0) {
                    mac_new_port_mode = CDMAC_3_TRI_0_0_2_3;
                } else{
                    mac_new_port_mode = CDMAC_3_TRI_0_1_2_2;
                }
                break;
            case CDMAC_3_TRI_0_1_2_2:
                if (first_lane_local == 0) {
                    mac_new_port_mode = CDMAC_2_LANES_DUAL;
                } else {
                    mac_new_port_mode = CDMAC_3_TRI_0_1_2_2;
                }
                break;
            case CDMAC_3_TRI_0_0_2_3:
                if (first_lane_local == 0) {
                    mac_new_port_mode = CDMAC_3_TRI_0_0_2_3;
                } else {
                    mac_new_port_mode = CDMAC_2_LANES_DUAL;
                }
                break;
            case CDMAC_2_LANES_DUAL:
                mac_new_port_mode = CDMAC_2_LANES_DUAL;
                break;
            case CDMAC_4_LANES_TOGETHER:
                mac_new_port_mode = CDMAC_2_LANES_DUAL;
                break;
            default:
                break;
        }
        if (mac_stage_id) {
            aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC1_PORT_MODEf, mac_new_port_mode);
        } else {
            aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC0_PORT_MODEf, mac_new_port_mode);
        }
    } else if (num_lanes == 1) {
        aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, SINGLE_PORT_MODE_SPEED_400Gf, 0);
        switch (mac_port_mode) {
            case CDMAC_4_LANES_SEPARATE:
                mac_new_port_mode = CDMAC_4_LANES_SEPARATE;
                break;
            case CDMAC_3_TRI_0_1_2_2:
                if ((first_lane_local == 0) || (first_lane_local == 1)) {
                    mac_new_port_mode = CDMAC_3_TRI_0_1_2_2;
                } else if ((first_lane_local == 2) || (first_lane_local == 3)) {
                    mac_new_port_mode = CDMAC_4_LANES_SEPARATE;
                }
                break;
            case CDMAC_3_TRI_0_0_2_3:
                if ((first_lane_local == 0) || (first_lane_local == 1)) {
                    mac_new_port_mode = CDMAC_4_LANES_SEPARATE;
                } else if ((first_lane_local == 2) || (first_lane_local == 3)) {
                    mac_new_port_mode = CDMAC_3_TRI_0_0_2_3;
                }
                break;
            case CDMAC_2_LANES_DUAL:
                if ((first_lane_local == 0) || (first_lane_local == 1)) {
                    mac_new_port_mode = CDMAC_3_TRI_0_1_2_2;
                } else if ((first_lane_local == 2) || (first_lane_local == 3)){
                    mac_new_port_mode = CDMAC_3_TRI_0_0_2_3;
                }
                break;
            case CDMAC_4_LANES_TOGETHER:
                mac_new_port_mode = CDMAC_4_LANES_SEPARATE;
                break;
            default:
                break;
        }
        if (mac_stage_id) {
            aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC1_PORT_MODEf, mac_new_port_mode);
        } else {
            aperta_soc_reg_field_set(unit, CDPORT_MODEr, reg_val, MAC0_PORT_MODEf, mac_new_port_mode);
        }
    }
    PHYMOD_IF_ERR_RETURN(WRITE_CDPORT_MODEr(&phy_acc, reg_val));


    return PHYMOD_E_NONE;

}

STATIC
int _plp_aperta_pm8x50_port_mac_drain_soft_reset(const plp_aperta_phymod_phy_access_t *phy_acc)
{
    int rv = 0, retry_cnt =40;
    portmod_drain_cells_t drain_cells;
    uint32_t cell_count;


    /* Drain cells */
    rv = plp_aperta_cdmac_drain_cell_get(phy_acc, &drain_cells);
    PHYMOD_IF_ERR_RETURN(rv);

    /* Start TX FIFO draining */
    if (0) {
    rv = plp_aperta_cdmac_drain_cell_start(phy_acc);
    PHYMOD_IF_ERR_RETURN(rv);

    /* De-assert SOFT_RESET to let the drain start */
    rv = plp_aperta_cdmac_soft_reset_set(phy_acc, 0);
    PHYMOD_IF_ERR_RETURN(rv);
    }
    
    /* Wait until TX fifo cell count is 0 */
    for (;;) {
        rv = plp_aperta_cdmac_txfifo_cell_cnt_get(phy_acc, &cell_count);
        PHYMOD_IF_ERR_RETURN(rv);
        if (cell_count == 0) {
            break;
        }
        retry_cnt--;
        if (retry_cnt < 0) {
            PHYMOD_DEBUG_ERROR(("Error In _plp_aperta_pm8x50_port_mac_drain_soft_reset\n"));
            return PHYMOD_E_INTERNAL;
        }
    }
    if (0) {
    /* Stop TX FIFO draining */
    rv = plp_aperta_cdmac_drain_cell_stop(phy_acc, &drain_cells);
    PHYMOD_IF_ERR_RETURN(rv);

    /* Put port into SOFT_RESET */
    rv = plp_aperta_cdmac_soft_reset_set(phy_acc, 1);
    PHYMOD_IF_ERR_RETURN(rv);
    }


    return PHYMOD_E_NONE;
}

STATIC
int _plp_aperta_pm8x50_port_rx_restore_mac_out_of_reset(const plp_aperta_phymod_phy_access_t *phy_acc, int rx_enable)
{
    int rv;


    /* Enable RX, de-assert SOFT_RESET */
    rv = plp_aperta_cdmac_egress_queue_drain_rx_en(phy_acc, rx_enable);
    PHYMOD_IF_ERR_RETURN(rv);


    return PHYMOD_E_NONE;
}

/*Add new port*/
int plp_aperta_pm8x50_port_attach(int unit, int port, pm_info_t pm_info,
                       const portmod_port_add_info_t* add_info)
{
    int port_index = 0, pm_is_active = 0;
    int rv = 0, side  = 0;
    int nof_phys;
    plp_aperta_phymod_phy_init_config_t init_config;
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    const portmod_speed_config_t *speed_config = &add_info->speed_config;

    /* Update WB DB */
    PM8x50_IS_ACTIVE_GET(unit, pm_info, pm_is_active);
    if (!pm_is_active) {
        pm_is_active = 1;
        PM8x50_IS_ACTIVE_SET(unit, pm_info, pm_is_active);
    }
    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        PM_8x50_INFO(pm_info)->int_phy_access.port_loc = side;
        PM_8x50_INFO(pm_info)->int_core_access.port_loc = side;
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                    &params, 1, &phy_access, &nof_phys, NULL));


    rv = _plp_aperta_pm8x50_pm_port_init(unit, port, pm_info, port_index, add_info, 1);
    PHYMOD_IF_ERR_RETURN(rv);

        /* initalize port */
         for (port_index =0; port_index<8; port_index ++) {
              if (phy_access.access.lane_mask & (1 << port_index)){
                   break;
              }
         }
        PHYMOD_DEBUG_INFO(("Port mode update ...\n"));
    /* Update port mode */
        rv = _plp_aperta_pm8x50_pm_port_mode_update(unit, port, pm_info, port_index,  plp_aperta_count_no_bits(phy_access.access.lane_mask));
    PHYMOD_IF_ERR_RETURN(rv);


    /* initialze phy */
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_phy_init_config_t_init(&init_config));
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));

    
    init_config.an_en = add_info->autoneg_en;
    init_config.cl72_en = add_info->link_training_en;
    init_config.op_mode = add_info->interface_config.port_op_mode;

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_init(&phy_access, &init_config));
    /* MAC SOFT_RESET will be released within plp_aperta_pm8x50_port_speed_config_set */
    rv = plp_aperta_pm8x50_port_speed_config_set(unit, port, pm_info, speed_config);
    PHYMOD_IF_ERR_RETURN(rv);
   }


    return PHYMOD_E_NONE;
}

/*Set PM in bypass mode. should be called in the aggregator code.*/
int plp_aperta_pm8x50_pm_bypass_set(int unit, pm_info_t pm_info, int enable)
{

    return (PHYMOD_E_NONE);
}

/*get port cores' phymod access*/
int plp_aperta_pm8x50_pm_core_info_get(int unit, pm_info_t pm_info, int phyn,
                            portmod_pm_core_info_t* core_info)
{


    if(phyn < 0) {
        phyn = PM8X50_MAX_NUM_PHYS;
    }

    core_info->ref_clk = PM_8x50_INFO(pm_info)->ref_clk;
    PHYMOD_MEMCPY(&core_info->lane_map, &(PM_8x50_INFO(pm_info)->lane_map),
               sizeof(plp_aperta_phymod_lane_map_t));

    return PHYMOD_E_NONE;
}

/*Get PM phys.*/
int plp_aperta_pm8x50_pm_phys_get(int unit, pm_info_t pm_info, int* phys)
{

    *phys =  PM_8x50_INFO(pm_info)->phys;
    return PHYMOD_E_NONE;
}

/*Port remove in PM level*/
int plp_aperta_pm8x50_port_detach(int unit, int port, pm_info_t pm_info)
{
#if 0  /* Can add this whenever needed*/
    int enable, invalid_port = -1, tmp_port, i = 0, flags = 0, pm_id;
    int is_last_one = TRUE, port_index = -1;
    const uint32_t is_active = 0;
    uint8_t pll0_active_lane_bm, pll1_active_lane_bm, pll0_adv_lane_bm, pll1_adv_lane_bm;
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;


    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_enable_get(unit, port, pm_info, flags, &enable));
    if (enable)
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (APERTA_SOC_MSG("can't detach active port %d"), port));

    PHYMOD_IF_ERR_RETURN(PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, &pll0_active_lane_bm));
    PHYMOD_IF_ERR_RETURN(PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, &pll1_active_lane_bm));
    PHYMOD_IF_ERR_RETURN(PM8x50_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, &pll0_adv_lane_bm));
    PHYMOD_IF_ERR_RETURN(PM8x50_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, &pll1_adv_lane_bm));

    for (i = 0 ; i < MAX_PORTS_PER_PM8X50; i++) {
        PHYMOD_IF_ERR_RETURN(PM8x50_LANE2PORT_GET(unit, pm_info, i, &tmp_port));
        if (tmp_port == port) {
            port_index = (port_index == -1 ? i : port_index);
            PHYMOD_IF_ERR_RETURN(PM8x50_LANE2PORT_SET(unit, pm_info, i, invalid_port));

            pll0_active_lane_bm &= ~(1 << i);
            pll1_active_lane_bm &= ~(1 << i);
            pll0_adv_lane_bm &= ~(1 << i);
            pll1_adv_lane_bm &= ~(1 << i);
        } else if (tmp_port != invalid_port) {
            is_last_one = FALSE;
        }
    }

    if (port_index == -1)
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PORT, (APERTA_SOC_MSG("Port %d wasn't found"), port));

    if ((!pll0_active_lane_bm) && (!pll0_adv_lane_bm)) {
        /* Power down PLL0 if it's not in use */
        PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_pll_pwrdn(&phy_access, 0, 1));
    }

    PHYMOD_IF_ERR_RETURN(PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll0_active_lane_bm));
    PHYMOD_IF_ERR_RETURN(PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll1_active_lane_bm));
    PHYMOD_IF_ERR_RETURN(PM8x50_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll0_adv_lane_bm));
    PHYMOD_IF_ERR_RETURN(PM8x50_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll1_adv_lane_bm));

    /*deinit PM in case of last one*/
    if (is_last_one) {
        PHYMOD_IF_ERR_RETURN(portmod_port_pm_id_get(unit, port, &pm_id));
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_pm_enable(unit, pm_id, pm_info, 0));
        PHYMOD_DEVICE_OP_MODE_PCS_BYPASS_CLR(PM_8x50_INFO(pm_info)->int_core_access.device_op_mode);
        PHYMOD_IF_ERR_RETURN(PM8x50_IS_ACTIVE_SET(unit, pm_info, is_active));
        PHYMOD_IF_ERR_RETURN(PM8x50_IS_CORE_INITIALIZED_SET(unit, pm_info, is_active));
        PHYMOD_IF_ERR_RETURN(PM8x50_IS_BYPASSED_SET(unit, pm_info, is_active));

        PM_8x50_INFO(pm_info)->int_core_access.type = phymodDispatchTypeInvalid;
        PM_8x50_INFO(pm_info)->tvco = portmodVCOInvalid;
        PM_8x50_INFO(pm_info)->ovco = portmodVCOInvalid;
    }
#endif

    return PHYMOD_E_NONE;
}

/*Port replace in PM level*/
int plp_aperta_pm8x50_port_replace(int unit, int port, pm_info_t pm_info, int new_port)
{

    return (PHYMOD_E_NONE);
}

/*Port enable*/
int plp_aperta_pm8x50_port_enable_set(int unit, int port, pm_info_t pm_info,
                           int flags, int enable)
{
    int actual_flags = flags;
    int aperta_cdmac_flags = 0;
    int is_bypassed = 0;
    plp_aperta_phymod_phy_access_t phy_access;
    portmod_access_get_params_t params;
    int nof_phys = 0;


    PHYMOD_MEMSET(&phy_access, 0, sizeof(phy_access));
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
           PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                      (APERTA_SOC_MSG("MAC RX and TX can't be enabled separately")));
        }
    }

    if(PORTMOD_PORT_ENABLE_PHY_GET(actual_flags)) {
        /* Only internal phy supported. */
        PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port,
                                     pm_info, &params, 1, &phy_access,
                                     &nof_phys, NULL));
    }


    PM8x50_IS_BYPASSED_GET(unit, pm_info, is_bypassed);

    if(enable){
        if((PORTMOD_PORT_ENABLE_MAC_GET(actual_flags)) && (!is_bypassed)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_enable_set(&phy_access, aperta_cdmac_flags, 1));
        }

        if(PORTMOD_PORT_ENABLE_PHY_GET(actual_flags)) {
            if (PORTMOD_PORT_ENABLE_TX_GET(actual_flags)) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_tx_lane_control_set(&phy_access,
                                                   phymodTxSquelchOff));
            }
            if (PORTMOD_PORT_ENABLE_RX_GET(actual_flags)) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_rx_lane_control_set(&phy_access,
                                                   phymodRxSquelchOff));
            }
        } /* PORTMOD_PORT_ENABLE_PHY_GET */
    } else {/* disable */
        if (PORTMOD_PORT_ENABLE_PHY_GET(actual_flags)) {

            if (PORTMOD_PORT_ENABLE_TX_GET(actual_flags)) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_tx_lane_control_set(&phy_access,
                                                   phymodTxSquelchOn));
            }

            if (PORTMOD_PORT_ENABLE_RX_GET(actual_flags)) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_rx_lane_control_set(&phy_access,
                                                   phymodRxSquelchOn));
            }
        }

        if((PORTMOD_PORT_ENABLE_MAC_GET(actual_flags))  && (!is_bypassed)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_enable_set(&phy_access, aperta_cdmac_flags, 0));
        }
    }


    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_enable_get(int unit, int port, pm_info_t pm_info,
                           int flags, int* enable)
{
    int nof_phys = 0;
    int phy_enable = 1, mac_enable = 1;
    int mac_rx_enable = 0, mac_tx_enable = 0;
    int is_bypassed = 0;
    int actual_flags = flags;
    plp_aperta_phymod_phy_access_t phy_access;
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_tx_lane_control_t tx_control = phymodTxSquelchOn;
    plp_aperta_phymod_phy_rx_lane_control_t rx_control = phymodRxSquelchOn;

    PHYMOD_NULL_CHECK(pm_info);

    PHYMOD_MEMSET(&phy_access, 0, sizeof(phy_access));
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

    if(PORTMOD_PORT_ENABLE_PHY_GET(actual_flags)) {
        /* Only internal phy supported. */
        PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port,
                                     pm_info, &params, 1, &phy_access,
                                     &nof_phys, NULL));
    }

    PM8x50_IS_BYPASSED_GET(unit, pm_info, is_bypassed);

    if (PORTMOD_PORT_ENABLE_PHY_GET(actual_flags)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_tx_lane_control_get(&phy_access,
                                                        &tx_control));
        PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_rx_lane_control_get(&phy_access,
                                                        &rx_control));

        phy_enable =  0;

        if (PORTMOD_PORT_ENABLE_RX_GET(actual_flags)) {
            phy_enable |= (rx_control == phymodRxSquelchOn) ? 0 : 1;
        }
        if (PORTMOD_PORT_ENABLE_TX_GET(actual_flags)) {
            phy_enable |= (tx_control == phymodTxSquelchOn) ? 0 : 1;
        }
    }

    if ((PORTMOD_PORT_ENABLE_MAC_GET(actual_flags)) && (!is_bypassed)) {
        mac_enable = 0;

        if (PORTMOD_PORT_ENABLE_RX_GET(actual_flags)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_enable_get(&phy_access,
                                              CDMAC_ENABLE_SET_FLAGS_RX_EN,
                                              &mac_rx_enable));
            mac_enable |= (mac_rx_enable)? 1: 0;
        }

        if (PORTMOD_PORT_ENABLE_TX_GET(actual_flags)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_enable_get(&phy_access,
                                              CDMAC_ENABLE_SET_FLAGS_TX_EN,
                                              &mac_tx_enable));
            mac_enable |= (mac_tx_enable)? 1: 0;
        }
    }

    *enable = (mac_enable && phy_enable);


    return PHYMOD_E_NONE;
}

/* VCOs setting validation.
 * Inputs:
 *     unit:           unit number;
 *     pm_info:        pm_info data structure;
 *     ports:          Logical port bitmaps which reqiring the input vcos;
 *     required_vcos:  Required vcos bitmap;
 *     flag:           PLL switch flag;
 * Output:
 *     vco_setting:    If validation pass, vco_setting will indicating
 *                     what's the new vco rates and whether they are new or not;
 *                     vco_setting.num_speeds and vco_setting.speed_config_list
 *                     are no care in this function.
 */
int _plp_aperta_pm8x50_vcos_setting_validate(int unit,
                                  pm_info_t pm_info,
                                  const int* ports,
                                  uint8_t required_vcos,
                                  int flag,
                                  portmod_pm_vco_setting_t* vco_setting)
{
#if 1
    int required_vco_count = 0, ovco_is_free = 0;
    portmod_vco_type_t required_vco_0, required_vco_1;
    int ovco_pll_active_lane_bitmap, tvco_pll_active_lane_bitmap, ovco_pll_adv_lane_bitmap, tvco_pll_adv_lane_bitmap;
    uint32_t port_lane_mask = 0;
    int nof_phys, port = 0;
    plp_aperta_phymod_phy_access_t phy_access;
    portmod_access_get_params_t params;
    uint8_t configured_ovco = 0, configured_tvco = 0;



    /* Get the required VCO count by the input speed_config list */
    required_vco_count = plp_aperta_count_no_bits(required_vcos);

    if (required_vco_count > 2) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                           (APERTA_SOC_MSG("Can not accommodate VCO settings.")));
    }

    required_vco_0 = portmodVCOInvalid;
    required_vco_1 = portmodVCOInvalid;


    /* Get lane mask for the input ports */
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    port_lane_mask |= phy_access.access.lane_mask;
    if (phy_access.port_loc == phymodPortLocLine) {
        PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, ovco_pll_active_lane_bitmap);
        PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_active_lane_bitmap);
        PM8x50_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, ovco_pll_adv_lane_bitmap);
        PM8x50_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, tvco_pll_adv_lane_bitmap);
    } else {
        PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, ovco_pll_active_lane_bitmap);
        PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, tvco_pll_active_lane_bitmap);
        PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, ovco_pll_adv_lane_bitmap);
        PM8x50_SYS_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, tvco_pll_adv_lane_bitmap);
    }

    if (phy_access.port_loc == phymodPortLocSys) {
        configured_ovco = PM_8x50_INFO(pm_info)->sys_ovco; 
        configured_tvco = PM_8x50_INFO(pm_info)->sys_tvco;
    } else {
        configured_ovco = PM_8x50_INFO(pm_info)->ovco; 
        configured_tvco = PM_8x50_INFO(pm_info)->tvco;
    }
    /*
     * When flag is default and the input port list is the complete port list
     * of the entire PM, the flag should be treated as "two PLL change allowed".
     * If the input port list is not complete, the flag should be treated as
     * "one PLL change allowed"
     */
    if (flag == PORTMOD_PM_SPEED_VALIDATE_F_PLL_SWITCH_DEFAULT) {
        if ((ovco_pll_active_lane_bitmap | tvco_pll_active_lane_bitmap |
             ovco_pll_adv_lane_bitmap | tvco_pll_adv_lane_bitmap) & (~port_lane_mask)) {
            flag = PORTMOD_PM_SPEED_VALIDATE_F_ONE_PLL_SWITCH_ALLOWED;
        } else {
            flag = PORTMOD_PM_SPEED_VALIDATE_F_TWO_PLL_SWITCH_ALLOWED;
        }
    }

    /* Remove input lanes from ovco_pll_active_lane_bitmap */
    ovco_pll_active_lane_bitmap &= ~port_lane_mask;
    ovco_pll_adv_lane_bitmap &= ~port_lane_mask;
    ovco_is_free = !(ovco_pll_active_lane_bitmap || ovco_pll_adv_lane_bitmap);
    /* Get the required VCO */
    /* Assign higher VCO to required_vco_0 */
    if (PORTMOD_VCO_26G_GET(required_vcos)) {
        required_vco_0 = portmodVCO26P562G;
        if (PORTMOD_VCO_25G_GET(required_vcos)) {
            required_vco_1 = portmodVCO25P781G;
        } else if (PORTMOD_VCO_20G_GET(required_vcos)) {
            required_vco_1 = portmodVCO20P625G;
        }
    } else if (PORTMOD_VCO_25G_GET(required_vcos)) {
        required_vco_0 = portmodVCO25P781G;
        if (PORTMOD_VCO_20G_GET(required_vcos)) {
            required_vco_1 = portmodVCO20P625G;
        }
    } else if (PORTMOD_VCO_20G_GET(required_vcos)) {
        required_vco_0 = portmodVCO20P625G;
    }

    if (required_vco_count == 1) {
        if (flag == PORTMOD_PM_SPEED_VALIDATE_F_TWO_PLL_SWITCH_ALLOWED) {
            /* We are allowed to change the tvco */
            if (/*PM8X50_TVCO_AVOID_20G &&*/ (required_vco_0 == portmodVCO20P625G)) {
                /* If PM8X50_TVCO_AVOID_20G == 1, we need to avoid TVCO being set at 20G.
                 * In this case, even though only 20G VCO is required, we will bring up
                 * TVCO @ 25G, OVCO @ 20G.
                 */
                vco_setting->tvco = portmodVCO25P781G;
                vco_setting->ovco = portmodVCO20P625G;
            } else {
            vco_setting->tvco = required_vco_0;
                vco_setting->ovco = portmodVCOInvalid;
            }

            if (vco_setting->tvco != configured_tvco) {
                /* Need to reset the whole core */
                vco_setting->is_tvco_new = 1;
                vco_setting->is_ovco_new = 1;
            } else if (vco_setting->ovco != configured_ovco) {
                vco_setting->is_tvco_new = 0;
                vco_setting->is_ovco_new = 1;
            } else {
                vco_setting->is_tvco_new = 0;
                vco_setting->is_ovco_new = 0;
            }
        } else if (flag == PORTMOD_PM_SPEED_VALIDATE_F_PLL_SWITCH_NOT_ALLOWED) {
            if (required_vco_0 == configured_tvco
                || required_vco_0 == configured_ovco) {
            vco_setting->tvco = configured_tvco;
            vco_setting->ovco = configured_ovco;
            vco_setting->is_tvco_new = 0;
            vco_setting->is_ovco_new = 0;
            } else {
                 PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                                    (APERTA_SOC_MSG("Can not accommodate VCO settings.")));
            }
        } else if (required_vco_0 == configured_tvco) {
            /* We can use the existing TVCO, no VCO change is needed */
            vco_setting->tvco = configured_tvco;
            vco_setting->is_tvco_new = 0;
            if (ovco_is_free &&
                (configured_ovco != portmodVCOInvalid)) {
                /* ovco becomes free, can be powerdown */
                vco_setting->ovco = portmodVCOInvalid;
                vco_setting->is_ovco_new = 1;
            } else {
                /* 1. ovco is not free, can not be powerdown.
                 * 2. ovco is free and is already in powerdown state. */
                vco_setting->ovco = configured_ovco;
                vco_setting->is_ovco_new = 0;
            }
        } else if (required_vco_0 == configured_ovco) {
            /* We can use existing OVCO, no VCO change is needed */
            vco_setting->tvco = configured_tvco;
            vco_setting->ovco = configured_ovco;
            vco_setting->is_tvco_new = 0;
            vco_setting->is_ovco_new = 0;
        } else if (ovco_is_free) {
            /* Need vco change and OVCO is free to change */
            vco_setting->tvco = configured_tvco;
            vco_setting->ovco = required_vco_0;
            vco_setting->is_tvco_new = 0;
            vco_setting->is_ovco_new = 1;
        } else {
            /* Input need ovco change while ovco is not free to change */
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                              (APERTA_SOC_MSG("Can not accommodate VCO settings. ovco not free")));
        }
    } else if (required_vco_count == 2) {
        if (flag == PORTMOD_PM_SPEED_VALIDATE_F_TWO_PLL_SWITCH_ALLOWED) {
            /* Both VCOs are allowed to change regardless of the current using status */
            vco_setting->tvco = required_vco_0;
            vco_setting->ovco = required_vco_1;
            if (required_vco_0 == configured_tvco) {
                vco_setting->is_tvco_new = 0;
                if (required_vco_1 == configured_ovco) {
                    vco_setting->is_ovco_new = 0;
                } else {
                    vco_setting->is_ovco_new = 1;
                }
            } else {
                /* Need to reset the whole core */
                vco_setting->is_tvco_new = 1;
                vco_setting->is_ovco_new = 1;
            }
        } else if ((required_vco_0 == configured_tvco && required_vco_1 == configured_ovco) ||
                   (required_vco_0 == configured_ovco && required_vco_1 == configured_tvco)) {
            /* No VCO change is needed */
            vco_setting->tvco = configured_tvco;
            vco_setting->ovco = configured_ovco;
            vco_setting->is_tvco_new = 0;
            vco_setting->is_ovco_new = 0;
        } else if (required_vco_0 == configured_tvco || required_vco_1 == configured_tvco) {
            /* One of the new vco equal to current tvco, only ovco change is needed */
            if (flag == PORTMOD_PM_SPEED_VALIDATE_F_PLL_SWITCH_NOT_ALLOWED) {
                 PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                                    (APERTA_SOC_MSG("Can not accommodate VCO settings.")));
            }
            if (ovco_is_free) {
                /* ovco is free to change */
                vco_setting->tvco = configured_tvco;
                if (required_vco_0 == configured_tvco) {
                    vco_setting->ovco = required_vco_1;
                } else {
                    vco_setting->ovco = required_vco_0;
                }
                vco_setting->is_tvco_new = 0;
                vco_setting->is_ovco_new = 1;
            } else {
                /* ovco is in use */
                PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                                  (APERTA_SOC_MSG("Can not accommodate VCO settings.")));
            }
        } else {
            /* We need to change tvco to satisfy the new speed config list, while flag is not set */
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                              (APERTA_SOC_MSG("Can not accommodate VCO settings.")));
        }
    }
#endif
    return PHYMOD_E_NONE;
}

/*Validate a set of speed config within a port macro.*/
int plp_aperta_pm8x50_pm_speed_config_validate(int unit,
                                    int pm_id,
                                    pm_info_t pm_info,
                                    const int* ports,
                                    int flag,
                                    portmod_pm_vco_setting_t* vco_setting)
{
    int i, rv;
    portmod_vco_type_t vco;
    uint8_t required_vcos = 0;

    /* Get the VCOs required by the input speed config list */
    for (i = 0; i < vco_setting->num_speeds; i++) {
        vco = portmodVCOInvalid;
        rv = _plp_aperta_pm8x50_port_speed_config_to_vco_get(&(vco_setting->speed_config_list[i]), &vco);
        if (rv == PHYMOD_E_CONFIG) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG,
                      (APERTA_SOC_MSG("Speed config is not supported")));
        }
        if (vco == portmodVCO20P625G) {
            PORTMOD_VCO_20G_SET(required_vcos);
        } else if (vco == portmodVCO25P781G) {
            PORTMOD_VCO_25G_SET(required_vcos);
        } else if (vco == portmodVCO26P562G) {
            PORTMOD_VCO_26G_SET(required_vcos);
        }
    }

    PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_vcos_setting_validate(unit, pm_info, ports, required_vcos, flag, vco_setting));


    return PHYMOD_E_NONE;
}

/*Port speed validation.*/
int plp_aperta_pm8x50_port_speed_config_validate(int unit,
                                      int port,
                                      pm_info_t pm_info,
                                      const portmod_speed_config_t* speed_config,
                                      int* affected_pbmp)
{
    int rv, i, tmp_port, port_num_lanes, nof_phys;
    plp_aperta_phymod_phy_access_t phy_access;
    portmod_access_get_params_t params;
    portmod_vco_type_t required_vco;
    int pll0_active_lane_bitmap, pll1_active_lane_bitmap, pll0_adv_lane_bitmap, pll1_adv_lane_bitmap;
    uint32_t port_lane_mask;
    uint8_t affected_lane_bitmap = 0, configured_ovco = 0, configured_tvco = 0;

    /* Get lane_mask and number of lanes for the port from DB */
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    port_lane_mask = phy_access.access.lane_mask;
    if (phy_access.port_loc == phymodPortLocSys) {
        configured_ovco = PM_8x50_INFO(pm_info)->sys_ovco; 
        configured_tvco = PM_8x50_INFO(pm_info)->sys_tvco;
    } else {
        configured_ovco = PM_8x50_INFO(pm_info)->ovco; 
        configured_tvco = PM_8x50_INFO(pm_info)->tvco;
    }

    port_num_lanes = plp_aperta_count_no_bits(port_lane_mask);
    /* Check speed_config->lane_num against lane number stored in DB */
    if (port_num_lanes != speed_config->num_lane) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("Invalid lane number request on exising logical port.")));
    }

    /* Verify and get the required VCO */
    rv = _plp_aperta_pm8x50_port_speed_config_to_vco_get(speed_config, &required_vco);
    if (rv == PHYMOD_E_CONFIG) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG,
                  (APERTA_SOC_MSG("Speed config is not supported")));
    }

    if (phy_access.port_loc == phymodPortLocLine) {
        PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll0_active_lane_bitmap);
        PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll1_active_lane_bitmap);
        PM8x50_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll0_adv_lane_bitmap);
        PM8x50_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll1_adv_lane_bitmap);
    } else {
        PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll0_active_lane_bitmap);
        PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll1_active_lane_bitmap);
        PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll0_adv_lane_bitmap);
        PM8x50_SYS_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll1_adv_lane_bitmap);
    }

    /* Remove port lane mask from pll0_active_lane_bitmap */
    pll0_active_lane_bitmap &= ~port_lane_mask;
    

    if (required_vco == configured_tvco
        || required_vco == configured_ovco
        || (pll0_active_lane_bitmap == 0 && pll0_adv_lane_bitmap == 0)) {
        /* 1. no vco change required
         * 2. OR, ovco is free to change */
        return PHYMOD_E_NONE;
    } else if (pll0_active_lane_bitmap & pll0_adv_lane_bitmap) {
        /* PLL0 is used by resolved AN port */
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("Can not accommodate VCO settings due to resolved AN port(s).")));
    } else if (pll0_active_lane_bitmap || pll0_adv_lane_bitmap) {
        /* PLL0 is used by FS port or is advertiesd by AN port */
        /* affected_lane_bitmap in this case should include ports that are actively using ovco or advertising the ovco */
        affected_lane_bitmap = pll0_active_lane_bitmap | pll0_adv_lane_bitmap;
        for( i = 0 ; i < MAX_PORTS_PER_PM8X50; i++){
            if ((affected_lane_bitmap >> i) & 1) {
                PM8x50_LANE2PORT_GET(unit, pm_info, i, tmp_port);
                *affected_pbmp |= tmp_port;
            }
        }
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("Can not accommodate VCO settings.")));
    } else {
        /* coverity[dead_error_line] */
        return PHYMOD_E_PARAM;
    }


    return PHYMOD_E_NONE;
}

/*
 * This API reconfig the VCO rates of the PM. Two VCO values are always
 * required by this API regardless of whether the caller intends to change
 * one or two VCO rates.
 */
int plp_aperta_pm8x50_pm_vco_reconfig(int unit,
                           int pm_id,
                           pm_info_t pm_info,
                           const portmod_vco_type_t* vco)
{
    uint8_t zero = 0;
    portmod_vco_type_t cur_tvco = PM_8x50_INFO(pm_info)->tvco;
    portmod_vco_type_t cur_ovco = PM_8x50_INFO(pm_info)->ovco;
    uint32_t pll_div;
    int tvco_pll_index, ovco_pll_index;
    plp_aperta_phymod_phy_access_t phy_access;
    int is_synce_enabled = 0;

    tvco_pll_index = PM8X50_TVCO_PLL_INDEX_GET(pm_info);
    ovco_pll_index = (tvco_pll_index == 1)? 0: 1;
    PHYMOD_MEMCPY(&phy_access, &(PM_8x50_INFO(pm_info)->int_core_access),
                    sizeof(plp_aperta_phymod_phy_access_t));
    if (phy_access.port_loc == phymodPortLocLine) {
        cur_tvco = PM_8x50_INFO(pm_info)->tvco;
        cur_ovco = PM_8x50_INFO(pm_info)->ovco;
    } else {
        cur_tvco = PM_8x50_INFO(pm_info)->sys_tvco;
        cur_ovco = PM_8x50_INFO(pm_info)->sys_ovco;
    }

    PHYMOD_CRIT_INFO(("Cur_tvco : %d vco[0]:%d cur_ovco:%d vco[1]:%d\n", cur_tvco, vco[0], cur_ovco, vco[1]));
    if (cur_tvco != vco[0]) {
        /* Get PLL_DIV */
        PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_vco_to_pll_get(PM_8x50_INFO(pm_info)->ref_clk, vco[0], &pll_div));

        /* TVCO need to be changed */
        if (phy_access.port_loc == phymodPortLocLine) {
            PM_8x50_INFO(pm_info)->tvco = vco[0];
        } else {
            PM_8x50_INFO(pm_info)->sys_tvco = vco[0];
        }
        PHYMOD_IF_ERR_RETURN (
                plp_aperta_update_vco(&phy_access, tvco_pll_index, vco[0]));
        phy_access.access.pll_idx = 0;

        /* in this case, set the phy_access lane mask to be 0xff
        need to re-visit once ILKN support is added*/
        if (pll_div !=phymod_APERTA_TSCBH_PLL_DIVNONE) {
            phy_access.access.lane_mask = 0xff;
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_pll_reconfig(&phy_access, tvco_pll_index, pll_div, phymodRefClk156Mhz));
        } else {
            PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_synce_enabled(&phy_access, tvco_pll_index, &is_synce_enabled));
            if (!is_synce_enabled) {
                /* will power down this PLL */
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_pll_pwrdn(&phy_access, tvco_pll_index, 1));
            }
        }

        /* For Ethernet mode, TVCO change will reset the whole serdes core.
         * So we will clear the lane bitmaps for both PLLs.
         */
        if (phy_access.port_loc == phymodPortLocLine) {
            PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, zero);
            PM8x50_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, zero);
            PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, zero);
            PM8x50_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, zero);
        } else {
            PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, zero);
            PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, zero);
            PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, zero);
            PM8x50_SYS_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, zero);
        }
    }

    if (cur_ovco != vco[1]) {
        /* next get the OVCO's PLL divider */
        PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_vco_to_pll_get(PM_8x50_INFO(pm_info)->ref_clk, vco[1], &pll_div));

        /* OVCO need to be changed */
        if (phy_access.port_loc == phymodPortLocLine) {
            PM_8x50_INFO(pm_info)->ovco = vco[1];
        } else {
            PM_8x50_INFO(pm_info)->sys_ovco = vco[1];
        }
        PHYMOD_IF_ERR_RETURN (
                plp_aperta_update_vco(&phy_access, ovco_pll_index, vco[1]));

        phy_access.access.pll_idx = 1;
        /*once ilkn support is added, the phy_access lane mask needs to be adjusted */
        if (pll_div !=phymod_APERTA_TSCBH_PLL_DIVNONE) {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_pll_reconfig(&phy_access, ovco_pll_index, pll_div, phymodRefClk156Mhz));
        } else {
            /* Check whether PLLx is needed to be powered up for SyncE */
            PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_synce_enabled(&phy_access, ovco_pll_index, &is_synce_enabled));
            if (!is_synce_enabled) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_pll_pwrdn(&phy_access, ovco_pll_index, 1));
                if (phy_access.port_loc == phymodPortLocLine) {
                    PM_8x50_INFO(pm_info)->ovco = portmodVCOInvalid;
                } else {
                    PM_8x50_INFO(pm_info)->sys_ovco = portmodVCOInvalid;
                }
                PHYMOD_IF_ERR_RETURN (
                  plp_aperta_update_vco(&phy_access, 0, portmodVCOInvalid));
            }
        }

        if (phy_access.port_loc == phymodPortLocLine) {
            PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, zero);
            PM8x50_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, zero);
        } else {
            PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, zero);
            PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, zero);

        }
    }

    return PHYMOD_E_NONE;
}

/*!
 * plp_aperta_portmod_common_pmd_lane_config_decode
 *
 * @brief decode the lane_config passed in from multi_resource_set
 *
 * @param [in]  unit     - lane_config
 * @param [out] name     - portmod_lane_config.
 */
int plp_aperta_portmod_common_pmd_lane_config_decode(uint32_t lane_config, portmod_pmd_lane_config_t* portmod_lane_config)
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
    portmod_lane_config->pam4_channel_loss = PORTMOD_PORT_PHY_LANE_CONFIG_PAM4_CHANNEL_LOSS_GET(lane_config);

    return PHYMOD_E_NONE;

}

/* set/get the speed config for the specified port.*/
int plp_aperta_pm8x50_port_speed_config_set(int unit, int port, pm_info_t pm_info, const portmod_speed_config_t* speed_config)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    uint64_t mac_ctrl = 0;
    phymod_phy_signalling_method_t signalling_mode;
    int nof_phys, rx_enable = 0;
    int affected_pbmp;
    portmod_pmd_lane_config_t portmod_lane_config;
    phymod_phy_speed_config_t phy_speed_config;
    phymod_phy_pll_state_t old_pll_state, new_pll_state;
    uint32_t lane_config;
    int pll_lanes_bitmap;
    int mac_speed = 0;
    int is_synce_enabled = 0;

    PHYMOD_MEMSET(&portmod_lane_config, 0, sizeof(portmod_pmd_lane_config_t));
    PHYMOD_MEMSET(&params, 0, sizeof(portmod_access_get_params_t));

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_phy_speed_config_t_init(&phy_speed_config));

    PHYMOD_IF_ERR_RETURN(
         plp_aperta_cdmac_speed_get(&phy_access, &mac_speed,  speed_config->speed));

    if (mac_speed != speed_config->speed) {
        int port_index = 0, temp_side, side;
        portmod_port_add_info_t add_info;
        PHYMOD_MEMSET(&add_info, 0, sizeof(portmod_port_add_info_t));
        PHYMOD_MEMCPY(&add_info.speed_config, speed_config, sizeof(portmod_speed_config_t));
        temp_side = PM_8x50_INFO(pm_info)->int_phy_access.port_loc;
        PHYMOD_CRIT_INFO(("Reconfiguring CDMAC side:%s\n", (temp_side == phymodPortLocLine) ? "LINE" : "SYS" ));
        /*for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            PM_8x50_INFO(pm_info)->int_phy_access.port_loc = side;
            PM_8x50_INFO(pm_info)->int_core_access.port_loc = side;*/
            PM_8x50_INFO(pm_info)->int_phy_access.port_loc = temp_side;
            PM_8x50_INFO(pm_info)->int_core_access.port_loc = temp_side;

            /* initalize port */
            for (port_index =0; port_index<8; port_index ++) {
                 if (phy_access.access.lane_mask & (1 << port_index)){
                      break;
                 }
            }
            /* Update port mode */
            PHYMOD_IF_ERR_RETURN(
                _plp_aperta_pm8x50_pm_port_mode_update(unit, port, pm_info,
                    port_index,  plp_aperta_count_no_bits(phy_access.access.lane_mask)));


            PHYMOD_IF_ERR_RETURN(
                _plp_aperta_pm8x50_pm_port_init(unit, port, pm_info, port_index, &add_info, 1));
        /*}*/
        PM_8x50_INFO(pm_info)->int_phy_access.port_loc = temp_side;
        PM_8x50_INFO(pm_info)->int_core_access.port_loc = temp_side;
        (void)side;
    }



    /* Get original MAC CTRL */
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_egress_queue_drain_get(&phy_access, &mac_ctrl, &rx_enable));

    
    /* Disable MAC RX, drain MAC FIFO, assert MAC SOFT_RESET */
    PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_port_mac_drain_soft_reset(&phy_access));

    /* Validate speed_config */
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_speed_config_validate(unit, port, pm_info, speed_config, &affected_pbmp));

    if (phy_access.port_loc == phymodPortLocLine) { 
        PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
        old_pll_state.pll0_lanes_bitmap = pll_lanes_bitmap;

        PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
        old_pll_state.pll1_lanes_bitmap = pll_lanes_bitmap;
    } else {
        PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
        old_pll_state.pll0_lanes_bitmap = pll_lanes_bitmap;

        PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
        old_pll_state.pll1_lanes_bitmap = pll_lanes_bitmap;
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));

    /* Decode the lane_config in speed_config */
    lane_config = speed_config->lane_config;
    /* Retrieve the default lane config */
    signalling_mode = PORTMOD_PHY_SIGNALLING_MODE_GET(speed_config);
    if (lane_config == -1) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_lane_config_default_get(&phy_access, signalling_mode, &portmod_lane_config.pmd_firmware_lane_config));
        portmod_lane_config.pam4_channel_loss = 0;
    } else {
        if (signalling_mode == phymodSignallingMethodPAM4) {
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_PAM4_SET(lane_config);
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NRZ_CLEAR(lane_config);
        } else {
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NRZ_SET(lane_config);
            PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_PAM4_CLEAR(lane_config);
        }
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_portmod_common_pmd_lane_config_decode(lane_config, &portmod_lane_config));
    }

    if (speed_config->fec == PORTMOD_PORT_PHY_FEC_NONE) {
        phy_speed_config.fec_type = phymod_fec_None;
    } else if (speed_config->fec == PORTMOD_PORT_PHY_FEC_BASE_R) {
        phy_speed_config.fec_type = phymod_fec_CL74;
    } else if (speed_config->fec ==  PORTMOD_PORT_PHY_FEC_RS_FEC) {
        phy_speed_config.fec_type = phymod_fec_CL91;
    } else if (speed_config->fec ==  PORTMOD_PORT_PHY_FEC_RS_544) {
        phy_speed_config.fec_type = phymod_fec_RS544;
    } else if (speed_config->fec ==  PORTMOD_PORT_PHY_FEC_RS_544_2XN) {
        phy_speed_config.fec_type = phymod_fec_RS544_2XN;
    } else if (speed_config->fec == PORTMOD_PORT_PHY_FEC_RS_272) {
        phy_speed_config.fec_type = phymod_fec_RS272;
    }
    phy_speed_config.data_rate = speed_config->speed;
    phy_speed_config.linkTraining = speed_config->link_training;
    phy_speed_config.PAM4_channel_loss = portmod_lane_config.pam4_channel_loss;
    phy_speed_config.pmd_lane_config = portmod_lane_config.pmd_firmware_lane_config;

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_speed_config_set(&phy_access, &phy_speed_config, &old_pll_state, &new_pll_state));

    if (phy_access.port_loc == phymodPortLocLine) {
        PM8x50_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
	    pll_lanes_bitmap &= ~phy_access.access.lane_mask;
        PM8x50_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
    } else {
        PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
    	pll_lanes_bitmap &= ~phy_access.access.lane_mask;
        PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
    }

    /* Power down PLL0 if it is not being used and if SyncE is not enabled */
    if (!(pll_lanes_bitmap || new_pll_state.pll0_lanes_bitmap)) {
        /* Check whether PLL0 is needed to be powered up for SyncE */
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_synce_enabled(&phy_access, 0, &is_synce_enabled));
        if (!is_synce_enabled) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_pll_pwrdn(&phy_access, 0, 1));
            if (phy_access.port_loc == phymodPortLocLine) {
                PM_8x50_INFO(pm_info)->ovco = portmodVCOInvalid;
            } else {
                PM_8x50_INFO(pm_info)->sys_ovco = portmodVCOInvalid;
            }
            PHYMOD_IF_ERR_RETURN (
                  plp_aperta_update_vco(&phy_access, 0, portmodVCOInvalid));
        }
    }
    if (phy_access.port_loc == phymodPortLocLine) {
        pll_lanes_bitmap = (uint8_t) new_pll_state.pll0_lanes_bitmap;
        PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
        pll_lanes_bitmap = (uint8_t) new_pll_state.pll1_lanes_bitmap;
        PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
    } else {
        pll_lanes_bitmap = (uint8_t) new_pll_state.pll0_lanes_bitmap;
        PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
        pll_lanes_bitmap = (uint8_t) new_pll_state.pll1_lanes_bitmap;
        PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
    }

    /* Enable MAC RX, De-assert MAC SOFT_RESET */
    /* Restore original MAC CONTROL*/
    if (0) {
    PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_port_rx_restore_mac_out_of_reset(&phy_access, rx_enable));
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_mac_ctrl_set(&phy_access, mac_ctrl));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_portmod_common_pmd_lane_config_encode(portmod_pmd_lane_config_t* portmod_lane_config, uint32_t* lane_config)
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

int plp_aperta_pm8x50_port_speed_config_get(int unit, int port, pm_info_t pm_info, portmod_speed_config_t* speed_config)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys, port_num_lanes;
    uint32_t lane_config;
    portmod_pmd_lane_config_t portmod_lane_config;
    phymod_phy_speed_config_t phy_speed_config;



    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    port_num_lanes = plp_aperta_count_no_bits(phy_access.access.lane_mask);
    speed_config->num_lane = port_num_lanes;

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_speed_config_get(&phy_access, &phy_speed_config));

    portmod_lane_config.pam4_channel_loss = phy_speed_config.PAM4_channel_loss;
    portmod_lane_config.pmd_firmware_lane_config = phy_speed_config.pmd_lane_config;
    /* Encode the lane_config in speed_config */
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_portmod_common_pmd_lane_config_encode(&portmod_lane_config, &lane_config));

    if (phy_speed_config.fec_type == phymod_fec_None) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_NONE;
    } else if (phy_speed_config.fec_type == phymod_fec_CL74) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_BASE_R;
    } else if (phy_speed_config.fec_type == phymod_fec_CL91) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_RS_FEC;
    } else if (phy_speed_config.fec_type == phymod_fec_RS544) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_RS_544;
    } else if (phy_speed_config.fec_type == phymod_fec_RS544_2XN) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN;
    } else if (phy_speed_config.fec_type == phymod_fec_RS272) {
        speed_config->fec = PORTMOD_PORT_PHY_FEC_RS_272;
    }

    speed_config->speed = phy_speed_config.data_rate;
    speed_config->link_training = phy_speed_config.linkTraining;
    speed_config->lane_config = lane_config;


    return PHYMOD_E_NONE;
}

/* set/get the interface_t, speed and encapsulation for the specified port. */
int plp_aperta_pm8x50_port_interface_config_set(int unit, int port, pm_info_t pm_info,
                                     const portmod_port_interface_config_t* config,
                                     int phy_init_flags)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_interface_config_get(int unit, int port, pm_info_t pm_info,
                                     portmod_port_interface_config_t* config,
                                     int phy_init_flags)
{

    return (PHYMOD_E_NONE);
}

/*Port cl72 set\get*/
int plp_aperta_pm8x50_port_cl72_set(int unit, int port, pm_info_t pm_info, uint32_t enable)
{
    int nof_phys;
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;


    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN
         (plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_cl72_set(&phy_access, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_cl72_get(int unit, int port, pm_info_t pm_info, uint32_t* enable)
{
    int nof_phys;
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;


    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN
       (plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_cl72_get(&phy_access, enable));


    return PHYMOD_E_NONE;
}

/*Get port cl72 status*/
int plp_aperta_pm8x50_port_cl72_status_get(int unit, int port, pm_info_t pm_info,
                                plp_aperta_phymod_cl72_status_t* status)
{
    int nof_phys;
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;


    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_cl72_status_get(&phy_access, status));


    return PHYMOD_E_NONE;
}

int plp_aperta_portmod_commmon_portmod_to_phymod_loopback_type(int unit, portmod_loopback_mode_t loopback_type, plp_aperta_phymod_loopback_mode_t *phymod_lb_type)
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
int plp_aperta_pm8x50_port_loopback_set(int unit, int port, pm_info_t pm_info,
                             portmod_loopback_mode_t loopback_type, int enable)
{
    plp_aperta_phymod_loopback_mode_t phymod_lb_type;
    int nof_phys;
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    PHYMOD_MEMSET(&params, 0, sizeof(portmod_access_get_params_t));

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                             &params, 1, &phy_access, &nof_phys, NULL));

    /* loopback type validation*/
    switch(loopback_type){
        case portmodLoopbackMacOuter:
            PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_loopback_set(&phy_access, loopback_type, enable));
            break;
        case portmodLoopbackPhyRloopPCS:
        case portmodLoopbackPhyRloopPMD:
        case portmodLoopbackPhyGloopPMD:
        case phymodLoopbackGlobalPMD:
        case phymodLoopbackRemotePMD:
        case phymodLoopbackRemotePCS:
            if (enable) {
                PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_loopback_set(&phy_access, portmodLoopbackMacOuter, 0));
            }
            if (loopback_type == portmodLoopbackPhyRloopPCS || loopback_type == portmodLoopbackPhyRloopPMD
                    || loopback_type == portmodLoopbackPhyGloopPMD) {
                PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_commmon_portmod_to_phymod_loopback_type(unit,
                             loopback_type, &phymod_lb_type));
            } else {
                /* coverity[mixed_enums] */
                phymod_lb_type = loopback_type;
            }
            PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_loopback_set(&phy_access, phymod_lb_type, enable));
            break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_UNAVAIL, (
                      APERTA_SOC_MSG("unsupported loopback type %d"), loopback_type));
            break;
    }


    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_loopback_get(int unit, int port, pm_info_t pm_info,
                             portmod_loopback_mode_t loopback_type,
                             int* enable)
{
    plp_aperta_phymod_loopback_mode_t phymod_lb_type;
    portmod_access_get_params_t params;
    int nof_phys;
    plp_aperta_phymod_phy_access_t phy_access;
    uint32_t tmp_enable = 0;
    int rv = PHYMOD_E_NONE;
    PHYMOD_MEMSET(&params, 0, sizeof(portmod_access_get_params_t));

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                             &params, 1, &phy_access, &nof_phys, NULL));


    switch(loopback_type){
        case portmodLoopbackMacOuter:
            PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_loopback_get(&phy_access, loopback_type, enable));
            break;
        case portmodLoopbackPhyRloopPCS:
        case portmodLoopbackPhyRloopPMD:
        case portmodLoopbackPhyGloopPMD:
        case phymodLoopbackGlobalPMD:
        case phymodLoopbackRemotePMD:
        case phymodLoopbackRemotePCS:
            if (loopback_type == portmodLoopbackPhyRloopPCS || loopback_type == portmodLoopbackPhyRloopPMD
                    || loopback_type == portmodLoopbackPhyGloopPMD) {
                PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_commmon_portmod_to_phymod_loopback_type(unit,
                             loopback_type, &phymod_lb_type));
            } else {
                /* coverity[mixed_enums] */
                phymod_lb_type = loopback_type;
            }

            rv = plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_loopback_get(&phy_access, phymod_lb_type, &tmp_enable);
            if (rv == PHYMOD_E_UNAVAIL) {
                rv = PHYMOD_E_NONE;
                tmp_enable = 0;
            }
            PHYMOD_IF_ERR_RETURN(rv);
            *enable = tmp_enable;
            break;
        default:
            (*enable) = 0; /* not supported --> no loopback */
            break;
    }
    return PHYMOD_E_NONE;
}

#define APERTA_PHY_ACCESS_GET(_PM)   \
    plp_aperta_phymod_phy_access_t phy_access;       \
    PHYMOD_NULL_CHECK(_PM);               \
    APERTA_PM8x50_PHY_ACCESS_GET(unit, port, _PM, phy_access);


/*Port RX MAC ENABLE set\get*/
int plp_aperta_pm8x50_port_rx_mac_enable_set(int unit, int port,
                                  pm_info_t pm_info, int enable)
{
    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rx_enable_set(&phy_access, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_rx_mac_enable_get(int unit, int port,
                                  pm_info_t pm_info, int* enable)
{
    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rx_enable_get(&phy_access, enable));

    return PHYMOD_E_NONE;
}

/*Port TX MAC ENABLE set\get*/
int plp_aperta_pm8x50_port_tx_mac_enable_set(int unit, int port,
                                  pm_info_t pm_info,
                                  int enable)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_tx_enable_set(&phy_access, enable));


    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_tx_mac_enable_get(int unit, int port,
                                  pm_info_t pm_info,
                                  int* enable)
{


    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_tx_enable_get(&phy_access, enable));


    return PHYMOD_E_NONE;
}

/*
 * This API looks at the current port's number of lanes and max speed,
 * returns the complete list of force-speed abilities with the same number
 * of lanes and speed that is <= max speed, and the complete list of autoneg
 * abilities with number of lanes <= current port's number of lanes and
 * speed <= max speed.
 */
int plp_aperta_pm8x50_port_speed_ability_local_get(int unit, int port,
                                        int max_num_abilities,
                                        portmod_port_speed_ability_t* abilities,
                                        int* num_abilities)
{
    /* NOT USED in APERTA*/
    return PHYMOD_E_UNAVAIL;
}

/* Translate portmod_port_speed_ability_t to phymod_autoneg_advert_abilities_t */
int _plp_aperta_pm8x50_port_port_to_phy_ability(int num_abilities,
                                     const portmod_port_speed_ability_t* abilities,
                                     phymod_autoneg_advert_abilities_t* an_advert_abilities)
{
    int i;

    an_advert_abilities->num_abilities = num_abilities;
    for (i = 0; i < num_abilities; i++) {
        an_advert_abilities->autoneg_abilities[i].speed = abilities[i].speed;
        an_advert_abilities->autoneg_abilities[i].resolved_num_lanes = abilities[i].num_lanes;
        an_advert_abilities->autoneg_abilities[i].fec = phymod_fec_None;
        
        if (abilities[i].fec_type & (1 << PORTMOD_PORT_PHY_FEC_NONE)) {
            an_advert_abilities->autoneg_abilities[i].fec |= (1 << phymod_fec_None);
        }
        if (abilities[i].fec_type & (1 << PORTMOD_PORT_PHY_FEC_BASE_R)) {
            an_advert_abilities->autoneg_abilities[i].fec |= (1 << phymod_fec_CL74);
        }
        if (abilities[i].fec_type & (1 <<PORTMOD_PORT_PHY_FEC_RS_FEC)) {
            an_advert_abilities->autoneg_abilities[i].fec |= (1 << phymod_fec_CL91);
        }
        if (abilities[i].fec_type & (1 <<PORTMOD_PORT_PHY_FEC_RS_544)) {
            an_advert_abilities->autoneg_abilities[i].fec |= (1 << phymod_fec_RS544);
        }
        if (abilities[i].fec_type & (1 <<PORTMOD_PORT_PHY_FEC_RS_544_2XN)) {
            an_advert_abilities->autoneg_abilities[i].fec |= (1 << phymod_fec_RS544_2XN);
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
                PHYMOD_DEBUG_ERROR(("Invalid Pause config\n"));
                return PHYMOD_E_CONFIG;
        }
        if (abilities[i].an_mode == (PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73)) {
            an_advert_abilities->autoneg_abilities[i].an_mode = phymod_AN_MODE_CL73;
        }
        if (abilities[i].an_mode == (PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM)) {
            an_advert_abilities->autoneg_abilities[i].an_mode = phymod_AN_MODE_CL73BAM;
        }
        if (abilities[i].an_mode == (PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA)) {
            an_advert_abilities->autoneg_abilities[i].an_mode = phymod_AN_MODE_MSA;
        }
        if (abilities[i].medium == PORTMOD_PORT_PHY_MEDIUM_COPPER) {
            an_advert_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
        } else {
            an_advert_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypePcbTraceBackPlane;
        }
        if (abilities[i].channel == (1 << PORTMOD_PORT_PHY_CHANNEL_SHORT)) {
            an_advert_abilities->autoneg_abilities[i].channel = phymod_channel_short;
        } else {
            an_advert_abilities->autoneg_abilities[i].channel = phymod_channel_long;
        }
    }

    return PHYMOD_E_NONE;
}

/* Translate phymod_autoneg_advert_abilities_t to portmod_port_speed_ability_t */
int _plp_aperta_pm8x50_port_phy_to_port_ability(phymod_autoneg_advert_abilities_t* an_advert_abilities,
                                     int* num_abilities,
                                     portmod_port_speed_ability_t* abilities)
{
    int i;

    *num_abilities = an_advert_abilities->num_abilities;
    for (i = 0; i < *num_abilities; i++) {
        abilities[i].speed = an_advert_abilities->autoneg_abilities[i].speed;
        abilities[i].num_lanes = an_advert_abilities->autoneg_abilities[i].resolved_num_lanes;
        if (an_advert_abilities->autoneg_abilities[i].fec & (1<<phymod_fec_None)) {
            abilities[i].fec_type |= (1 << PORTMOD_PORT_PHY_FEC_NONE);
        }
        if (an_advert_abilities->autoneg_abilities[i].fec & (1<<phymod_fec_CL74)) {
            abilities[i].fec_type |= (1 << PORTMOD_PORT_PHY_FEC_BASE_R);
        }
        if (an_advert_abilities->autoneg_abilities[i].fec & (1 << phymod_fec_CL91)) {
            abilities[i].fec_type |= (1 << PORTMOD_PORT_PHY_FEC_RS_FEC);
        }
        if (an_advert_abilities->autoneg_abilities[i].fec & (1 << phymod_fec_RS544)) {
            abilities[i].fec_type |= (1 << PORTMOD_PORT_PHY_FEC_RS_544);
        }
        if (an_advert_abilities->autoneg_abilities[i].fec & (1 << phymod_fec_RS544_2XN)) {
            abilities[i].fec_type |= (1 << PORTMOD_PORT_PHY_FEC_RS_544_2XN);
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
                PHYMOD_DEBUG_ERROR(("Invalid Pause config\n"));
                return PHYMOD_E_CONFIG;
        }
        if (an_advert_abilities->autoneg_abilities[i].an_mode == phymod_AN_MODE_CL73) {
            abilities[i].an_mode |= (PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73);
        }
        if (an_advert_abilities->autoneg_abilities[i].an_mode == phymod_AN_MODE_CL73BAM) {
            abilities[i].an_mode |= (PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM);
        }
        if (an_advert_abilities->autoneg_abilities[i].an_mode == phymod_AN_MODE_MSA) {
            abilities[i].an_mode |= (PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA);
        }
        if (an_advert_abilities->autoneg_abilities[i].medium == phymodFirmwareMediaTypeCopperCable) {
            abilities[i].medium = PORTMOD_PORT_PHY_MEDIUM_COPPER;
        } else {
            abilities[i].medium = PORTMOD_PORT_PHY_MEDIUM_BACKPLANE;
        }
        if (an_advert_abilities->autoneg_abilities[i].channel == phymod_channel_short) {
            abilities[i].channel |= (1 << PORTMOD_PORT_PHY_CHANNEL_SHORT);
        } 
        if (an_advert_abilities->autoneg_abilities[i].channel == phymod_channel_long) {
            abilities[i].channel |= (1 << PORTMOD_PORT_PHY_CHANNEL_LONG);
        }
    }

    return PHYMOD_E_NONE;
}

static uint8_t _plp_aperta_pm_log2n(uint32_t n) 
{
    return ((n > 1) ? (1 + _plp_aperta_pm_log2n(n / 2)) : 0);
}


/*Set/get port auto negotiation ability*/
int plp_aperta_pm8x50_port_autoneg_ability_advert_set(int unit, int port, pm_info_t pm_info,
                                           int num_abilities,
                                           const portmod_port_speed_ability_t* abilities)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys, i;
    portmod_vco_type_t request_vco = 0;
    uint8_t vcos = 0;
    portmod_pm_vco_setting_t vco_setting;
    int ports;
    phymod_autoneg_advert_ability_t autoneg_abilities[PM8x50_MAX_AN_ABILITY];
    phymod_autoneg_advert_abilities_t an_advert_abilities;
    int is_msa = 0, is_bam = 0;
    plp_aperta_phymod_an_mode_type_t an_mode = 0;
    int channel_25g = 0;
    int fec_cl73, fec_cl73_25g, fec_cl73bam_msa;
    phymod_phy_pll_state_t old_pll_state, new_pll_state;
    int pll_lanes_bitmap;
    int is_synce_enabled = 0;
    int pm_fec_type = 0;

    fec_cl73 = PORTMOD_PORT_PHY_FEC_INVALID;
    fec_cl73_25g = PORTMOD_PORT_PHY_FEC_INVALID;
    fec_cl73bam_msa = PORTMOD_PORT_PHY_FEC_INVALID;
    
    /* Validate abilities */
    for (i = 0; i < num_abilities; i++) {
        if ((abilities[i].fec_type & (1 << PORTMOD_PORT_PHY_FEC_BASE_R)) &&
            (abilities[i].fec_type & (1 << PORTMOD_PORT_PHY_FEC_RS_FEC))) {
            PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_an_ability_table_vco_get(abilities[i].speed, abilities[i].num_lanes,
                         PORTMOD_PORT_PHY_FEC_BASE_R, abilities[i].an_mode, &request_vco));
        } else {
            pm_fec_type = _plp_aperta_pm_log2n(abilities[i].fec_type);
            PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_an_ability_table_vco_get(abilities[i].speed, abilities[i].num_lanes,
                         pm_fec_type, abilities[i].an_mode, &request_vco));
        }
        /* 1. AN ability is supported by HW */
        if (request_vco == portmodVCOInvalid) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG,
                               (APERTA_SOC_MSG("port %d: abilities[%d] is not supported.VCO"), port, i));
        }
        /* 2. AN abilities do not conflict with each other */
        if (abilities[i].pause != abilities[0].pause) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                               (APERTA_SOC_MSG("port %d: Pause conflicts among abilities."), port));
        }
        if (abilities[i].medium != abilities[0].medium) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                               (APERTA_SOC_MSG("port %d: Medium conflicts among abilities."), port));
        }
        if (abilities[i].an_mode == PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_BAM) {
            is_bam = 1;
            if (abilities[i].speed < 50000 || (abilities[i].speed == 50000 && abilities[i].num_lanes == 2)) {
                if ((fec_cl73bam_msa == PORTMOD_PORT_PHY_FEC_NONE)
                    && (abilities[i].fec_type != PORTMOD_PORT_PHY_FEC_NONE)) {
                    PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                                       (APERTA_SOC_MSG("port %d: FEC conflicts among abilities."), port));
                } else {
                    fec_cl73bam_msa = abilities[i].fec_type;
                }
            }
        } else if (abilities[i].an_mode == PORTMOD_PORT_PHY_CONTROL_AUTONEG_MODE_CL73_MSA) {
            is_msa = 1;
            if ((fec_cl73bam_msa == PORTMOD_PORT_PHY_FEC_NONE)
                && (abilities[i].fec_type != PORTMOD_PORT_PHY_FEC_NONE)) {
                PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                                   (APERTA_SOC_MSG("port %d: FEC conflicts among abilities."), port));
            } else {
                fec_cl73bam_msa = abilities[i].fec_type;
            }
        } else {
            /* CL73 */
            if (abilities[i].speed == 40000 || abilities[i].speed == 10000) {
                if ((fec_cl73 == PORTMOD_PORT_PHY_FEC_NONE)
                    && (abilities[i].fec_type != PORTMOD_PORT_PHY_FEC_NONE)) {
                    PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                                       (APERTA_SOC_MSG("port %d: FEC conflicts among abilities."), port));
                } else {
                    fec_cl73 = abilities[i].fec_type;
                }
                if (abilities[i].speed == 10000) {
                    if (abilities[i].medium != PORTMOD_PORT_PHY_MEDIUM_BACKPLANE) {
                        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG,
                                            (APERTA_SOC_MSG("port %d: abilities[%d] is not supported. Not BP"), port, i));
                    }
                }
            } else if (abilities[i].speed == 25000) {
                if (!channel_25g) {
                    channel_25g = abilities[i].channel;
                } else {
                    if (channel_25g != abilities[i].channel) {
                        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                                           (APERTA_SOC_MSG("port %d: Channel conflicts among abilities."), port));
                    }
                }
                if ((abilities[i].channel == PORTMOD_PORT_PHY_CHANNEL_SHORT) &&
                    (abilities[i].fec_type == PORTMOD_PORT_PHY_FEC_RS_FEC)) {
                    PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG,
                                       (APERTA_SOC_MSG("port %d: abilities[%d] is not supported."), port, i));
                }
                if ((fec_cl73_25g == PORTMOD_PORT_PHY_FEC_NONE)
                    && (abilities[i].fec_type != PORTMOD_PORT_PHY_FEC_NONE)) {
                    PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                                       (APERTA_SOC_MSG("port %d: FEC conflicts among abilities."), port));
                } else {
                    fec_cl73_25g = abilities[i].fec_type;
                }
            }
        }
        if (request_vco == portmodVCO20P625G) {
            PORTMOD_VCO_20G_SET(vcos);
        } else if (request_vco == portmodVCO25P781G) {
            PORTMOD_VCO_25G_SET(vcos);
        } else if (request_vco == portmodVCO26P562G) {
            PORTMOD_VCO_26G_SET(vcos);
        }
    }

    if (is_bam && is_msa) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                           (APERTA_SOC_MSG("port %d: an_mode conflicts among abilities."
                                     "Cannot advertise CL73MSA and CL73BAM at the same time."), port));
    } else if (is_bam) {
        an_mode = phymod_AN_MODE_CL73BAM;
    } else if (is_msa) {
        an_mode = phymod_AN_MODE_CL73_MSA;
    } else {
        an_mode = phymod_AN_MODE_CL73;
    }

    /* 3. AN ability can be supported by current VCOs */
    ports = 1 << port;
    PHYMOD_IF_ERR_RETURN(
       _plp_aperta_pm8x50_vcos_setting_validate(unit, pm_info, &ports, vcos, PORTMOD_PM_SPEED_VALIDATE_F_ONE_PLL_SWITCH_ALLOWED, &vco_setting));

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    an_advert_abilities.autoneg_abilities = autoneg_abilities;
    PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_port_port_to_phy_ability(num_abilities, abilities, &an_advert_abilities));

    /* Read current PLL adv bitmap */
    if (phy_access.port_loc == phymodPortLocLine) {
        PM8x50_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
        old_pll_state.pll0_lanes_bitmap = pll_lanes_bitmap;

        PM8x50_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
        old_pll_state.pll1_lanes_bitmap = pll_lanes_bitmap;
    } else {
        PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
        old_pll_state.pll0_lanes_bitmap = pll_lanes_bitmap;

        PM8x50_SYS_TVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
        old_pll_state.pll1_lanes_bitmap = pll_lanes_bitmap;

    }

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_autoneg_advert_ability_set(&phy_access, &an_advert_abilities, &old_pll_state, &new_pll_state));
    /* Update DB and WB */
    if (vco_setting.is_ovco_new) {
        if (phy_access.port_loc == phymodPortLocLine) {
            PM_8x50_INFO(pm_info)->ovco = vco_setting.ovco;
        } else {
            PM_8x50_INFO(pm_info)->sys_ovco = vco_setting.ovco;
        }
    }

    /* Update an_mode in WB */
    /*PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_port_index_get (unit, port, pm_info, &port_index, &bitmap));*/
    PM8x50_AN_MODE_SET(unit, pm_info, an_mode, 0);
    if (phy_access.port_loc == phymodPortLocLine) { 
        PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
    } else {
        PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
    }
    /* Power down PLL0 if it's not being used and if SyncE is disabled */
    if (!(pll_lanes_bitmap || new_pll_state.pll0_lanes_bitmap)) {
        /* Check whether PLL0 is needed to be powered up for SyncE */
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_synce_enabled(&phy_access, 0, &is_synce_enabled));
        if (!is_synce_enabled) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_pll_pwrdn(&phy_access, 0, 1));
            if (phy_access.port_loc == phymodPortLocLine) {
                PM_8x50_INFO(pm_info)->ovco = portmodVCOInvalid;
            } else {
                PM_8x50_INFO(pm_info)->sys_ovco = portmodVCOInvalid;
            }
            PHYMOD_IF_ERR_RETURN (
                  plp_aperta_update_vco(&phy_access, 0, portmodVCOInvalid));

        }
    }
    if (phy_access.port_loc == phymodPortLocLine) { 
        pll_lanes_bitmap = (uint8_t) new_pll_state.pll0_lanes_bitmap;
        PM8x50_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
        pll_lanes_bitmap = (uint8_t) new_pll_state.pll1_lanes_bitmap;
        PM8x50_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
    } else {
        pll_lanes_bitmap = (uint8_t) new_pll_state.pll0_lanes_bitmap;
        PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
        pll_lanes_bitmap = (uint8_t) new_pll_state.pll1_lanes_bitmap;
        PM8x50_SYS_TVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
    }


    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_autoneg_ability_advert_get(int unit, int port, pm_info_t pm_info,
                                           int max_num_abilities,
                                           portmod_port_speed_ability_t* abilities,
                                           int* actual_num_abilities)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys, i;
    phymod_autoneg_advert_ability_t autoneg_abilities[PM8x50_MAX_AN_ABILITY];
    phymod_autoneg_advert_abilities_t an_advert_abilities;



    for (i = 0; i < PM8x50_MAX_AN_ABILITY; i++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_autoneg_advert_ability_t_init(&autoneg_abilities[i]));
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    an_advert_abilities.autoneg_abilities = autoneg_abilities;
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_autoneg_advert_ability_get(&phy_access, &an_advert_abilities));
    PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_port_phy_to_port_ability(&an_advert_abilities, actual_num_abilities, abilities));


    return PHYMOD_E_NONE;
}

/*Port ability remote Adv get*/
int plp_aperta_pm8x50_port_autoneg_ability_remote_get(int unit, int port, pm_info_t pm_info,
                                           int max_num_abilities,
                                           portmod_port_speed_ability_t* abilities,
                                           int* actual_num_abilities)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys, i;
    phymod_autoneg_advert_ability_t autoneg_abilities[PM8x50_MAX_AN_ABILITY];
    phymod_autoneg_advert_abilities_t an_advert_abilities;



    for (i = 0; i < PM8x50_MAX_AN_ABILITY; i++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_autoneg_advert_ability_t_init(&autoneg_abilities[i]));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    an_advert_abilities.autoneg_abilities = autoneg_abilities;

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_autoneg_remote_advert_ability_get(&phy_access, &an_advert_abilities));

    if (an_advert_abilities.num_abilities > max_num_abilities) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("port %d: There are %d AN abilities. Larger array is needed."),
                           port, an_advert_abilities.num_abilities));
    } else {
        PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_port_phy_to_port_ability(&an_advert_abilities, actual_num_abilities, abilities));
    }


    return PHYMOD_E_NONE;
}

/*Set\Get autoneg*/
int plp_aperta_pm8x50_port_autoneg_set(int unit, int port, pm_info_t pm_info,
                            uint32_t phy_flags,
                            const plp_aperta_phymod_autoneg_control_t* an)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys, port_num_lanes;
    int port_index;
    uint32_t bitmap;
    plp_aperta_phymod_autoneg_control_t an_control;
    int pll_lanes_bitmap, is_synce_enabled = 0;



    PHYMOD_IF_ERR_RETURN(_plp_aperta_pm8x50_port_index_get(unit, port, pm_info, &port_index, &bitmap));
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    port_num_lanes = plp_aperta_count_no_bits(phy_access.access.lane_mask);

    an_control = *an;

    an_control.num_lane_adv = port_num_lanes;
#if APERTA_PM8x50_WB_SUPPORT
    /* Get an_mode from WB */
    PHYMOD_IF_ERR_RETURN(PM8x50_AN_MODE_GET(unit, pm_info, &an_control.an_mode, port_index));
    if (an_control.an_mode == phymod_AN_MODE_NONE) {
        /* Return error only when user want to enable AN without any AN abilities being advertised. */
        if (!an_control.enable) {
            an_control.an_mode = phymod_AN_MODE_CL73;
        } else {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG,
                               (APERTA_SOC_MSG("port %d: There's no AN advertisement for the port."), port));
        }
    }
#endif
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_autoneg_set(&phy_access, &an_control));

    if (an_control.enable) {
        /* When AN is enable on a port, always connect that port to TVCO */
        if (phy_access.port_loc == phymodPortLocLine) { 
            PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
            /*pll_lanes_bitmap &= ~phy_access.access.lane_mask;
            PM8x50_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);*/
        } else {
            PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
            /*pll_lanes_bitmap &= ~phy_access.access.lane_mask;
            PM8x50_SYS_OVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);*/
        }

        /* Power down PLL0 if its not being used and if SyncE is disabled */
        if (!pll_lanes_bitmap) {
            if (phy_access.port_loc == phymodPortLocLine) {
                PM8x50_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
            } else {
                PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
            }
            /* Check whether PLL0 is needed to be powered up for SyncE */
            PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_synce_enabled(&phy_access, 0, &is_synce_enabled));
            if (!pll_lanes_bitmap && !is_synce_enabled) {
                PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_pll_pwrdn(&phy_access, 0, 1));
                if (phy_access.port_loc == phymodPortLocLine) {
                    PM_8x50_INFO(pm_info)->ovco = portmodVCOInvalid;
                } else {
                    PM_8x50_INFO(pm_info)->sys_ovco = portmodVCOInvalid;
                }
                PHYMOD_IF_ERR_RETURN (
                      plp_aperta_update_vco(&phy_access, 0, portmodVCOInvalid));

            }
        }

        if (phy_access.port_loc == phymodPortLocLine) { 
#if 0 /* Needed for debug purpose*/
            PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
            pll_lanes_bitmap |= phy_access.access.lane_mask;
            PM8x50_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
            /*PLL ADV*/
    	    PM8x50_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
	        pll_lanes_bitmap &= ~(phy_access.access.lane_mask);
            PM8x50_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
#endif

        } else {
#if 0 /* Needed for debug purpose*/
            PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
            pll_lanes_bitmap |= phy_access.access.lane_mask;
            PM8x50_SYS_TVCO_PLL_ACTIVE_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
            /*PLL ADV*/
    	    PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
            pll_lanes_bitmap |= phy_access.access.lane_mask;
            PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
#endif

        }
    } else {
        if (phy_access.port_loc == phymodPortLocLine) {
            PM8x50_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
	    pll_lanes_bitmap &= ~(phy_access.access.lane_mask);
            PM8x50_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
	} else {
            PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_GET(unit, pm_info, pll_lanes_bitmap);
	    pll_lanes_bitmap &= ~(phy_access.access.lane_mask);
            PM8x50_SYS_OVCO_PLL_ADV_LANE_BITMAP_SET(unit, pm_info, pll_lanes_bitmap);
	}
    }


    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_autoneg_get(int unit, int port, pm_info_t pm_info,
                            uint32_t phy_flags,
                            plp_aperta_phymod_autoneg_control_t* an)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;
    uint32_t an_done;

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_autoneg_get(&phy_access, an, &an_done));


    return PHYMOD_E_NONE;
}

/*Get autoneg status*/
int plp_aperta_pm8x50_port_autoneg_status_get(int unit, int port, pm_info_t pm_info,
                                   plp_aperta_phymod_autoneg_status_t* an_status)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;



    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_autoneg_status_get(&phy_access, an_status));
    
    if (!(an_status->enabled && an_status->locked)) {
        /* upper layer should not rely on the
         * data rate if the AN is not locked
         */
        an_status->data_rate = 0;
        an_status->interface = 0;
    }


    return PHYMOD_E_NONE;
}

/*get link status*/
int plp_aperta_pm8x50_port_link_get(int unit, int port, pm_info_t pm_info,
                         int flags, int* link)
{
    portmod_speed_config_t speed_config;
    int latch_val = 0;
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;

    PHYMOD_MEMSET(&params, 0, sizeof(portmod_access_get_params_t));

    BCMI_APERTA_CDMAC_CDPORT_XGXS0_LN0_STATUSr_t  ln0_status;


    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                    &params, 1, &phy_access, &nof_phys, NULL));

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_speed_config_get(unit, port, pm_info, &speed_config));
    if (speed_config.speed == 400000) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm8x50_port_link_latch_down_get(unit, port, pm_info, PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR, &latch_val));
        if (latch_val) {
            *link = 0;
        } else {
            PHYMOD_IF_ERR_RETURN(
                READ_CDPORT_XGXS0_LN0_STATUSr(&phy_access, &ln0_status));
            *link = BCMI_APERTA_CDMAC_CDPORT_XGXS0_LN0_STATUSr_LINK_STATUSf_GET(ln0_status);
        }
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));

        PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_link_status_get(&phy_access,
                                                   (uint32_t *) link));
    }

    return PHYMOD_E_NONE;
}

/*get latch down link status (whether link was ever down since last clear)*/
int plp_aperta_pm8x50_port_link_latch_down_get(int unit, int port,
                                    pm_info_t pm_info,
                                    uint32_t flags, int* link)
{
    int first_index;
    uint32_t bitmap;
    BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_t rval;

    APERTA_PHY_ACCESS_GET(pm_info);


    *link = 0;

    PHYMOD_IF_ERR_RETURN(
        _plp_aperta_pm8x50_port_index_get(unit, port, pm_info, &first_index, &bitmap));

    PHYMOD_IF_ERR_RETURN(READ_CDPORT_LINKSTATUS_DOWNr(&phy_access, &rval));
    switch(first_index) {
        case 0:
            *link = BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT0_LINKSTATUSf_GET(rval);
            if (PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR & flags) {
                BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT0_LINKSTATUSf_SET(rval, 1);
            }
            break;
        case 1:
            *link = BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT1_LINKSTATUSf_GET(rval);
            if (PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR & flags) {
                BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT1_LINKSTATUSf_SET(rval, 1);
            }
            break;
        case 2:
            *link = BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT2_LINKSTATUSf_GET(rval);
            if (PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR & flags) {
                BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT2_LINKSTATUSf_SET(rval, 1);
            }
            break;
        case 3:
            *link = BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT3_LINKSTATUSf_GET(rval);
            if (PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR & flags) {
                BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT3_LINKSTATUSf_SET(rval, 1);
            }
            break;
        case 4:
            *link = BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT4_LINKSTATUSf_GET(rval);
            if (PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR & flags) {
                BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT4_LINKSTATUSf_SET(rval, 1);
            }
            break;
        case 5:
            *link = BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT5_LINKSTATUSf_GET(rval);
            if (PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR & flags) {
                BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT5_LINKSTATUSf_SET(rval, 1);
            }
            break;
        case 6:
            *link = BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT6_LINKSTATUSf_GET(rval);
            if (PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR & flags) {
                BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT6_LINKSTATUSf_SET(rval, 1);
            }
            break;
        case 7:
            *link = BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT7_LINKSTATUSf_GET(rval);
            if (PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR & flags) {
                BCMI_APERTA_CDMAC_CDPORT_LINKSTATUS_DOWNr_PORT7_LINKSTATUSf_SET(rval, 1);
            }
            break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL,
                     (APERTA_SOC_MSG("Port %d, failed to get port index"), port));
    }


    if (PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR & flags) {
        /* CDPORT_LINKSTATUS_DOWN is sticky register.
         * Need to write 1b'1 to clear each field.
         */
       PHYMOD_IF_ERR_RETURN(
               WRITE_CDPORT_LINKSTATUS_DOWNr(&phy_access, rval));
    }


    return PHYMOD_E_NONE;
}
/* link up event */
int plp_aperta_pm8x50_port_phy_link_up_event(int unit, int port, pm_info_t pm_info)
{

    return (PHYMOD_E_NONE);
}

/* link down event */
int plp_aperta_pm8x50_port_phy_link_down_event(int unit, int port, pm_info_t pm_info)
{

    return (PHYMOD_E_NONE);
}

/*PRBS configuration set/get*/
int plp_aperta_pm8x50_port_prbs_config_set(int unit, int port, pm_info_t pm_info,
                                portmod_prbs_mode_t mode, int flags,
                                const plp_aperta_phymod_prbs_t* config)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;

    /* MAC */
    if(mode == 1) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                          &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_diagnostics_tscbh_diagnostics_driver.f_phymod_phy_prbs_config_set(&phy_access, flags, config));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_prbs_config_get(int unit, int port, pm_info_t pm_info,
                                portmod_prbs_mode_t mode, int flags,
                                plp_aperta_phymod_prbs_t* config)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;

    /* MAC */
    if(mode == 1) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_diagnostics_tscbh_diagnostics_driver.f_phymod_phy_prbs_config_get(&phy_access, flags, config));


    return PHYMOD_E_NONE;
}

/*PRBS enable set/get*/
int plp_aperta_pm8x50_port_prbs_enable_set(int unit, int port, pm_info_t pm_info,
                                portmod_prbs_mode_t mode, int flags,
                                int enable)
{

    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;

    /* MAC */
    if(mode == 1) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_diagnostics_tscbh_diagnostics_driver.f_phymod_phy_prbs_enable_set(&phy_access, flags, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_prbs_enable_get(int unit, int port, pm_info_t pm_info,
                                portmod_prbs_mode_t mode, int flags,
                                int* enable)
{
     portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;

    /* MAC */
    if(mode == 1) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_diagnostics_tscbh_diagnostics_driver.f_phymod_phy_prbs_enable_get(&phy_access, flags,
                                                (uint32_t *) enable));



    return PHYMOD_E_NONE;
}

/*PRBS status get*/
int plp_aperta_pm8x50_port_prbs_status_get(int unit, int port, pm_info_t pm_info,
                                portmod_prbs_mode_t mode, int flags,
                                plp_aperta_phymod_prbs_status_t* status)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;

    /* MAC */
    if(mode == 1) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("MAC PRBS is not supported")));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_diagnostics_tscbh_diagnostics_driver.f_phymod_phy_prbs_status_get(&phy_access, flags, status));


    return PHYMOD_E_NONE;
}

/*Port tx taps set\get*/
int plp_aperta_pm8x50_port_tx_set(int unit, int port, pm_info_t pm_info, const plp_aperta_phymod_tx_t* tx)
{
   /* Will use TSCBH directly*/
    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_tx_get(int unit, int port, pm_info_t pm_info, plp_aperta_phymod_tx_t* tx)
{
   /* Will use TSCBH directly*/


    return PHYMOD_E_NONE;
}

/*Number of lanes get*/
int plp_aperta_pm8x50_port_nof_lanes_get(int unit, int port,
                              int* nof_lanes)
{

    return (PHYMOD_E_NONE);
}

/*Set port PHYs' firmware mode*/
int plp_aperta_pm8x50_port_firmware_mode_set(int unit, int port, pm_info_t pm_info,
                                  phymod_firmware_mode_t fw_mode)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_firmware_mode_get(int unit, int port, pm_info_t pm_info,
                                  phymod_firmware_mode_t* fw_mode)
{

    return (PHYMOD_E_NONE);
}

/*Filter packets smaller than the specified threshold*/
int plp_aperta_pm8x50_port_runt_threshold_set(int unit, int port,
                                   pm_info_t pm_info,
                                   int value)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_runt_threshold_set(&phy_access, value));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_runt_threshold_get(int unit, int port, pm_info_t pm_info,
                                   int* value)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_runt_threshold_get(&phy_access, value));

    return PHYMOD_E_NONE;
}

/*Filter packets bigger than the specified value*/
int plp_aperta_pm8x50_port_max_packet_size_set(int unit, int port, pm_info_t pm_info,
                                    int value)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rx_max_size_set(&phy_access , value));
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_cntmaxsize_set(&phy_access, value));

    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_max_packet_size_get(int unit, int port, pm_info_t pm_info,
                                    int* value)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rx_max_size_get(&phy_access, value));

    return PHYMOD_E_NONE;
}

/*
 * TX pad packets to the specified size.
 * values smaller than 17 means pad is disabled.
 */
int plp_aperta_pm8x50_port_pad_size_set(int unit, int port, pm_info_t pm_info, int value)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pad_size_set(&phy_access, value));


    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_pad_size_get(int unit, int port, pm_info_t pm_info, int* value)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pad_size_get(&phy_access, value));


    return PHYMOD_E_NONE;
}

/*set/get the MAC source address that will be sent in case of Pause/LLFC*/
int plp_aperta_pm8x50_port_tx_mac_sa_set(int unit, int port, pm_info_t pm_info,
                              pm_mac_addr_t mac_sa)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_tx_mac_sa_set(&phy_access, mac_sa));


    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_tx_mac_sa_get(int unit, int port, pm_info_t pm_info,
                              pm_mac_addr_t mac_sa)
{
    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_tx_mac_sa_get(&phy_access, mac_sa));


    return PHYMOD_E_NONE;
}

/* set/get SA recognized for MAC control packets
 * in addition to the standard 0x0180C2000001
 */
int plp_aperta_pm8x50_port_rx_mac_sa_set(int unit, int port, pm_info_t pm_info,
                              pm_mac_addr_t mac_sa)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rx_mac_sa_set(&phy_access, mac_sa));


    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_rx_mac_sa_get(int unit, int port, pm_info_t pm_info,
                              pm_mac_addr_t mac_sa)
{
    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rx_mac_sa_get(&phy_access, mac_sa));


    return PHYMOD_E_NONE;
}

/*set/get Average inter-packet gap*/
int plp_aperta_pm8x50_port_tx_average_ipg_set(int unit, int port, pm_info_t pm_info,
                                   int value)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_tx_average_ipg_set(&phy_access, value));


    return PHYMOD_E_NONE;


}
int plp_aperta_pm8x50_port_tx_average_ipg_get(int unit, int port, pm_info_t pm_info,
                                   int* value)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_tx_average_ipg_get(&phy_access, value));


    return PHYMOD_E_NONE;

}

/*local fault set/get*/
int plp_aperta_pm8x50_port_local_fault_control_set(int unit, int port, pm_info_t pm_info,
                                 const portmod_local_fault_control_t* control)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_local_fault_control_set(&phy_access, control));


    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_local_fault_control_get(int unit, int port, pm_info_t pm_info,
                                      portmod_local_fault_control_t* control)
{
    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_local_fault_control_get(&phy_access, control));


    return PHYMOD_E_NONE;

}

/*remote fault set/get*/
int plp_aperta_pm8x50_port_remote_fault_control_set(int unit, int port, pm_info_t pm_info,
                                 const portmod_remote_fault_control_t* control)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_remote_fault_control_set(&phy_access, control));


    return PHYMOD_E_NONE;

}
int plp_aperta_pm8x50_port_remote_fault_control_get(int unit, int port, pm_info_t pm_info,
                                     portmod_remote_fault_control_t* control)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_remote_fault_control_get(&phy_access, control));


    return PHYMOD_E_NONE;
}

/*local fault steatus get*/
int plp_aperta_pm8x50_port_local_fault_status_get(int unit, int port, pm_info_t pm_info,
                                       int* value)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_local_fault_status_get(&phy_access, value));


    return PHYMOD_E_NONE;
}

/*remote fault status get*/
int plp_aperta_pm8x50_port_remote_fault_status_get(int unit, int port, pm_info_t pm_info,
                                        int* value)
{
    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_remote_fault_status_get(&phy_access, value));


    return PHYMOD_E_NONE;
}

/*local fault steatus clear*/
int plp_aperta_pm8x50_port_local_fault_status_clear(int unit, int port, pm_info_t pm_info)
{

    return (PHYMOD_E_NONE);
}

/*remote fault status clear*/
int plp_aperta_pm8x50_port_remote_fault_status_clear(int unit, int port,
                                          pm_info_t pm_info)
{

    return (PHYMOD_E_NONE);
}

int plp_aperta_pm8x50_port_pause_control_set(int unit, int port, pm_info_t pm_info,
                                  const portmod_pause_control_t* control)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pause_control_set(&phy_access, control));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_pause_control_get(int unit, int port, pm_info_t pm_info,
                                  portmod_pause_control_t* control)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pause_control_get(&phy_access, control));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_pfc_control_set(int unit, int port, pm_info_t pm_info,
                                const portmod_pfc_control_t* control)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pfc_control_set(&phy_access, control));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_pfc_control_get(int unit, int port, pm_info_t pm_info,
                                portmod_pfc_control_t* control)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pfc_control_get(&phy_access, control));

    return PHYMOD_E_NONE;
}

/*Routine for MAC\PHY sync.*/
int plp_aperta_pm8x50_port_update(int unit, int port, pm_info_t pm_info,
                       const portmod_port_update_control_t* update_control)
{

    return (PHYMOD_E_NONE);
}

/*get port cores' phymod access*/
int plp_aperta_pm8x50_port_core_access_get(int unit, int port, pm_info_t pm_info,
                                int phyn, int max_cores,
                                plp_aperta_phymod_core_access_t* core_access_arr,
                                int* nof_cores, int* is_most_ext)
{

    /* There are only internal phys(1) on PM8x50, setting phyn to 0 */
    if(phyn < 0)
    {
        phyn = 0;
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_core_access_t_init(&core_access_arr[0]));

    if( phyn == 0 ){
        PHYMOD_MEMCPY(&core_access_arr[0],
                   &(PM_8x50_INFO(pm_info)->int_core_access),
                   sizeof(plp_aperta_phymod_core_access_t));
        *nof_cores = 1;
    }

    *is_most_ext = 1;


    return PHYMOD_E_NONE;
}

/*Get lane phymod access structure. can be used for per lane operations*/
int plp_aperta_pm8x50_port_phy_lane_access_get(int unit, int port, pm_info_t pm_info,
                                    const portmod_access_get_params_t* params,
                                    int max_phys,
                                    plp_aperta_phymod_phy_access_t* phy_access,
                                    int* nof_phys, int* is_most_ext)
{
    int i;

    if(max_phys > MAX_NUM_CORES)
    {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("max_phys parameter exceeded the "
                          "MAX value. max_phys=%d, max allowed %d."),
                          max_phys, MAX_NUM_CORES));
    }

    for( i = 0 ; i < max_phys; i++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_phy_access_t_init(&phy_access[i]));
    }

    *nof_phys = 1;

    /* internal core */
    PHYMOD_MEMCPY (&phy_access[0], &(PM_8x50_INFO(pm_info)->int_core_access),
                sizeof(plp_aperta_phymod_phy_access_t));
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

/*Port duplex set\get*/
int plp_aperta_pm8x50_port_duplex_set(int unit, int port, pm_info_t pm_info, int enable)
{
    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_duplex_set(&phy_access, enable));
    return PHYMOD_E_NONE;

}

int plp_aperta_pm8x50_port_duplex_get(int unit, int port, pm_info_t pm_info, int* enable)
{
    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_duplex_get(&phy_access, enable));

    return (PHYMOD_E_NONE);
}

/*Port PHY Control register read*/
int plp_aperta_pm8x50_port_phy_reg_read(int unit, int port, pm_info_t pm_info, int flags,
                             int reg_addr, uint32_t* value)
{

    return (PHYMOD_E_NONE);
}

/*Port PHY Control register write*/
int plp_aperta_pm8x50_port_phy_reg_write(int unit, int port, pm_info_t pm_info,
                              int flags, int reg_addr, uint32_t value)
{

    return (PHYMOD_E_NONE);
}

/*Port Reset set\get*/
int plp_aperta_pm8x50_port_reset_set(int unit, int port,
                          pm_info_t pm_info, int mode,
                          int opcode, int value)
{

    /* Not req, use tshbh directely*/
    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_reset_get(int unit, int port,
                          pm_info_t pm_info, int mode,
                          int opcode, int* value)
{

    /* Not req, use tshbh directely*/
    return (PHYMOD_E_NONE);

}

/*Drv Name Get*/
int plp_aperta_pm8x50_port_drv_name_get(int unit, int port,
                             pm_info_t pm_info,
                             char* name, int len)
{
    /* Not req, use tshbh directely*/
    return (PHYMOD_E_NONE);
}

/*set/get port fec enable according to local/remote FEC ability*/
int plp_aperta_pm8x50_port_fec_enable_set(int unit, int port, pm_info_t pm_info,
                               uint32_t enable)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_fec_enable_set(&phy_access, enable));


    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_fec_enable_get(int unit, int port, pm_info_t pm_info,
                               uint32_t* enable)
{
    portmod_access_get_params_t params;
    plp_aperta_phymod_phy_access_t phy_access;
    int nof_phys;

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                                &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_fec_enable_get(&phy_access, enable));


    return PHYMOD_E_NONE;
}

/*get port auto negotiation local ability*/
int plp_aperta_pm8x50_port_ability_advert_set(int unit, int port, pm_info_t pm_info,
                                   portmod_port_ability_t* ability)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_ability_advert_get(int unit, int port, pm_info_t pm_info,
                                   portmod_port_ability_t* ability)
{

    return (PHYMOD_E_NONE);
}

/*Port ability remote Adv get*/
int plp_aperta_pm8x50_port_ability_remote_get(int unit, int port, pm_info_t pm_info,
                                   portmod_port_ability_t* ability)
{

    return (PHYMOD_E_NONE);
}

/*Port Mac Control Spacing Stretch*/
int plp_aperta_pm8x50_port_frame_spacing_stretch_set(int unit, int port,
                                          pm_info_t pm_info, int spacing)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_frame_spacing_stretch_get(int unit, int port,
                                          pm_info_t pm_info,
                                          const int* spacing)
{

    return (PHYMOD_E_NONE);
}

/*get port timestamps in fifo*/
int plp_aperta_pm8x50_port_diag_fifo_status_get(int unit, int port, pm_info_t pm_info,
                                     const portmod_fifo_status_t* diag_info)
{

    return (PHYMOD_E_NONE);
}

/*set/get pass control frames.*/
int plp_aperta_pm8x50_port_rx_control_set(int unit, int port, pm_info_t pm_info,
                               const portmod_rx_control_t* rx_ctrl)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_rx_control_get(int unit, int port, pm_info_t pm_info,
                               portmod_rx_control_t* rx_ctrl)
{

    return (PHYMOD_E_NONE);
}

/*set PFC config registers.*/
int plp_aperta_pm8x50_port_pfc_config_set(int unit, int port, pm_info_t pm_info,
                               const portmod_pfc_config_t* pfc_cfg)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    if (pfc_cfg->classes != 8) {
        return PHYMOD_E_PARAM;
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pfc_config_set(&phy_access, pfc_cfg));
    /* set the pfc frame control in cdmac rsv */
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rsv_mask_control_set(&phy_access,
                                                CDMAC_RSV_MASK_PFC_FRAME,
                                                pfc_cfg->rxpass));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_pfc_config_get(int unit, int port, pm_info_t pm_info,
                               portmod_pfc_config_t* pfc_cfg)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    pfc_cfg->classes = 8;
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pfc_config_get(&phy_access, pfc_cfg));
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rsv_mask_control_get(&phy_access,
                                                CDMAC_RSV_MASK_PFC_FRAME,
                                                &pfc_cfg->rxpass));
    return PHYMOD_E_NONE;
}

/*set EEE Config.*/
int plp_aperta_pm8x50_port_eee_set(int unit, int port, pm_info_t pm_info,
                        const portmod_eee_t* eee)
{

    return (PHYMOD_E_UNAVAIL);
}
int plp_aperta_pm8x50_port_eee_get(int unit, int port, pm_info_t pm_info,
                        portmod_eee_t* eee)
{

    return (PHYMOD_E_UNAVAIL);
}

/*set EEE Config.*/
int plp_aperta_pm8x50_port_eee_clock_set(int unit, int port, pm_info_t pm_info,
                              const portmod_eee_clock_t* eee_clk)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_eee_clock_get(int unit, int port, pm_info_t pm_info,
                              portmod_eee_clock_t* eee_clk)
{

    return (PHYMOD_E_NONE);
}

/*set Vlan Inner/Outer tag.*/
int plp_aperta_pm8x50_port_vlan_tag_set(int unit, int port, pm_info_t pm_info,
                             const portmod_vlan_tag_t* vlan_tag)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rx_vlan_tag_set (&phy_access,
                                            vlan_tag->outer_vlan_tag,
                                            vlan_tag->inner_vlan_tag));

    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_vlan_tag_get(int unit, int port, pm_info_t pm_info,
                             portmod_vlan_tag_t* vlan_tag)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rx_vlan_tag_get (&phy_access,
                                   (int*)&vlan_tag->outer_vlan_tag,
                                   (int*)&vlan_tag->inner_vlan_tag));

    return PHYMOD_E_NONE;
}

/*set modid field.*/
int plp_aperta_pm8x50_port_modid_set(int unit, int port, pm_info_t pm_info, int value)
{

    return (PHYMOD_E_NONE);
}

/*set modid field.*/
int plp_aperta_pm8x50_port_led_chain_config(int unit, int port, pm_info_t pm_info,
                                 int value)
{

    return (PHYMOD_E_NONE);
}

/*set modid field.*/
int plp_aperta_pm8x50_port_clear_rx_lss_status_set(int unit, int port, pm_info_t pm_info,
                                        int lcl_fault, int rmt_fault)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_clear_rx_lss_status_get(int unit, int port, pm_info_t pm_info,
                                        int* lcl_fault, int* rmt_fault)
{

    return (PHYMOD_E_NONE);
}

/*Attaches an external phy lane to a specific port macro*/
int plp_aperta_pm8x50_xphy_lane_attach_to_pm(int unit,
                         pm_info_t pm_info, int iphy, int phyn,
                         const portmod_xphy_lane_connection_t* lane_connection)
{

    return (PHYMOD_E_NONE);
}

/*Attaches an external phy lane to a specific port macro*/
int plp_aperta_pm8x50_xphy_lane_detach_from_pm(int unit,
                               pm_info_t pm_info, int iphy, int phyn,
                               portmod_xphy_lane_connection_t* lane_connection)
{

    return (PHYMOD_E_NONE);
}

/*Toggle Lag Failover Status.*/
int plp_aperta_pm8x50_port_lag_failover_status_toggle(int unit, int port,
                                           pm_info_t pm_info)
{

    return (PHYMOD_E_NONE);
}

/*Toggle Lag Failover loopback set / get.*/
int plp_aperta_pm8x50_port_lag_failover_loopback_set(int unit, int port,
                                          pm_info_t pm_info,
                                          int value)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_failover_loopback_set(&phy_access, value));

    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_lag_failover_loopback_get(int unit, int port,
                                          pm_info_t pm_info,
                                          int* value)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_failover_loopback_get(&phy_access, value));

    return PHYMOD_E_NONE;
}

/*set modid field.*/
int plp_aperta_pm8x50_port_mode_set(int unit, int port, pm_info_t pm_info,
                         const portmod_port_mode_info_t* mode)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_mode_get(int unit, int port, pm_info_t pm_info,
                         portmod_port_mode_info_t* mode)
{

    return (PHYMOD_E_NONE);
}

/*set port encap.*/
int plp_aperta_pm8x50_port_encap_set(int unit, int port, pm_info_t pm_info,
                          int flags, portmod_encap_t encap)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_encap_set(&phy_access, flags, encap));

    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_encap_get(int unit, int port, pm_info_t pm_info,
                          int* flags, portmod_encap_t* encap)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_encap_get(&phy_access, flags, encap));

    return PHYMOD_E_NONE;
}

/*set port register higig field.*/
int plp_aperta_pm8x50_port_higig_mode_set(int unit, int port,
                               pm_info_t pm_info,
                               int mode)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_higig_mode_get(int unit, int port,
                               pm_info_t pm_info,
                               int* mode)
{

    return (PHYMOD_E_NONE);
}

/*set port register higig field.*/
int plp_aperta_pm8x50_port_higig2_mode_set(int unit, int port,
                                pm_info_t pm_info,
                                int mode)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_higig2_mode_get(int unit, int port,
                                pm_info_t pm_info,
                                int* mode)
{

    return (PHYMOD_E_NONE);
}

/*set port register port type field.*/
int plp_aperta_pm8x50_port_config_port_type_set(int unit, int port,
                                     pm_info_t pm_info,
                                     int type)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_config_port_type_get(int unit, int port,
                                     pm_info_t pm_info,
                                     int* type)
{

    return (PHYMOD_E_NONE);
}

/*set/get hwfailover for trident.*/
int plp_aperta_pm8x50_port_trunk_hwfailover_config_set(int unit, int port,
                                            pm_info_t pm_info,
                                            int hw_count)
{
    int old_failover_en=0, new_failover_en = 0;
    int old_link_status_sel=0, new_link_status_sel=0;
    int old_reset_flow_control=0, new_reset_flow_control = 0;
    int lag_failover_lpbk;
    APERTA_PHY_ACCESS_GET(pm_info);

    if (hw_count) {
        new_failover_en        = 1;
        new_link_status_sel    = 1;
        new_reset_flow_control = 1;
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_failover_loopback_get(&phy_access,
                                                     &lag_failover_lpbk));

    if (lag_failover_lpbk) {
        return (PHYMOD_E_NONE);
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_failover_en_get(&phy_access,
                                               &old_failover_en));
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_sw_link_status_select_get(&phy_access,
                                                 &old_link_status_sel));

    if (old_failover_en     != new_failover_en ||
        old_link_status_sel != new_link_status_sel) {

        PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_sw_link_status_select_set(&phy_access,
                                                    new_link_status_sel));
        PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_failover_en_set(&phy_access,
                                                   new_failover_en));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_reset_fc_timers_on_link_dn_get(&phy_access, &old_reset_flow_control));
    if (old_reset_flow_control != new_reset_flow_control) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_reset_fc_timers_on_link_dn_set(&phy_access, new_reset_flow_control));
    }


    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_trunk_hwfailover_config_get(int unit, int port,
                                            pm_info_t pm_info,
                                            int* enable)
{
    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_failover_en_get(&phy_access, enable));

    return PHYMOD_E_NONE;
}

/*set/get hwfailover for trident.*/
int plp_aperta_pm8x50_port_trunk_hwfailover_status_get(int unit, int port,
                                            pm_info_t pm_info,
                                            int* loopback)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_failover_loopback_get(&phy_access,
                                                     loopback));

    return PHYMOD_E_NONE;
}

/*set/get hwfailover for trident.*/
int plp_aperta_pm8x50_port_diag_ctrl(int unit, int port, pm_info_t pm_info,
                          uint32_t inst, int op_type, int op_cmd,
                          const void* arg)
{
   /* Not required for aperta*/

    return PHYMOD_E_NONE;

}

/*Get/Set InterFrameGap Setting. */
int plp_aperta_pm8x50_port_ifg_set(int unit, int port, pm_info_t pm_info, int speed,
                        soc_port_duplex_t duplex, int ifg, int* real_ifg)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_ifg_get(int unit, int port, pm_info_t pm_info, int speed,
                        soc_port_duplex_t duplex, int* ifg)
{

    return (PHYMOD_E_NONE);
}

/*Get the reference clock value 156 or 125.*/
int plp_aperta_pm8x50_port_ref_clk_get(int unit, int port,
                            pm_info_t pm_info,
                            int* ref_clk)
{

    return (PHYMOD_E_NONE);
}

/*Disable lag failover.*/
int plp_aperta_pm8x50_port_lag_failover_disable(int unit, int port, pm_info_t pm_info)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_failover_disable(&phy_access));

    return PHYMOD_E_NONE;
}

/*Disable lag failover.*/
int plp_aperta_pm8x50_port_lag_remove_failover_lpbk_set(int unit, int port,
                                             pm_info_t pm_info, int val)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_remove_failover_lpbk_set(&phy_access, val));


    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_lag_remove_failover_lpbk_get(int unit, int port,
                                             pm_info_t pm_info, int* val)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_lag_remove_failover_lpbk_get(&phy_access, val));


    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_port_cntmaxsize_set(int unit, int port, pm_info_t pm_info, int val)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    return (plp_aperta_cdmac_cntmaxsize_set(&phy_access, val));
}

int plp_aperta_pm8x50_port_cntmaxsize_get(int unit, int port, pm_info_t pm_info, int* val)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    return (plp_aperta_cdmac_cntmaxsize_get(&phy_access, val));
}

/*Get Info needed to restore after drain cells.*/
int plp_aperta_pm8x50_port_drain_cell_get(int unit, int port, pm_info_t pm_info,
                               portmod_drain_cells_t* drain_cells)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_drain_cell_get(&phy_access, drain_cells));


    return PHYMOD_E_NONE;
}

/*Restore informaation after drain cells.*/
int plp_aperta_pm8x50_port_drain_cell_stop(int unit, int port, pm_info_t pm_info,
                                const portmod_drain_cells_t* drain_cells)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_drain_cell_stop(&phy_access, drain_cells));


    return PHYMOD_E_NONE;
}

/*Restore informaation after drain cells.*/
int plp_aperta_pm8x50_port_drain_cell_start(int unit, int port, pm_info_t pm_info)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_drain_cell_start(&phy_access));


    return PHYMOD_E_NONE;
}

/**/
int plp_aperta_pm8x50_port_drain_cells_rx_enable(int unit, int port,
                                      pm_info_t pm_info,
                                      int rx_en)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_drain_cells_rx_enable(&phy_access, rx_en));
    return PHYMOD_E_NONE;
}

/**/
int plp_aperta_pm8x50_port_egress_queue_drain_rx_en(int unit, int port,
                                         pm_info_t pm_info,
                                         int rx_en)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_egress_queue_drain_rx_en(&phy_access, rx_en));


    return PHYMOD_E_NONE;
}

/**/
int plp_aperta_pm8x50_port_mac_ctrl_set(int unit, int port,
                             pm_info_t pm_info,
                             uint64_t ctrl)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_mac_ctrl_set(&phy_access, ctrl));


    return PHYMOD_E_NONE;
}

/**/
int plp_aperta_pm8x50_port_txfifo_cell_cnt_get(int unit, int port,
                                    pm_info_t pm_info,
                                    uint32_t* cnt)
{

    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_txfifo_cell_cnt_get(&phy_access, cnt));


    return PHYMOD_E_NONE;
}

/**/
int plp_aperta_pm8x50_port_egress_queue_drain_get(int unit, int port,
                                       pm_info_t pm_info,
                                       uint64_t* ctrl, int* rxen)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_egress_queue_drain_get(&phy_access, ctrl, rxen));

    return PHYMOD_E_NONE;
}

/**/
int plp_aperta_pm8x50_port_mac_reset_set(int unit, int port, pm_info_t pm_info, int val)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_soft_reset_set(&phy_access, val));

    return PHYMOD_E_NONE;
}

/**/
int plp_aperta_pm8x50_port_soft_reset_toggle(int unit, int port, pm_info_t pm_info, int idx)
{

    return (PHYMOD_E_NONE);
}

/*Check if MAC needs to be reset.*/
int plp_aperta_pm8x50_port_mac_reset_check(int unit, int port,
                                pm_info_t pm_info,
                                int enable, int* reset)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    return (plp_aperta_cdmac_reset_check(&phy_access, enable, reset));
}

/**/
int plp_aperta_pm8x50_port_core_num_get(int unit, int port,
                             pm_info_t pm_info,
                             int* core_num)
{
    *core_num = PM_8x50_INFO(pm_info)->core_num;
    return (PHYMOD_E_NONE);
}

/**/
int plp_aperta_pm8x50_port_e2ecc_hdr_set(int unit, int port, pm_info_t pm_info,
                              const portmod_port_higig_e2ecc_hdr_t* e2ecc_hdr)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_e2ecc_hdr_get(int unit, int port, pm_info_t pm_info,
                              portmod_port_higig_e2ecc_hdr_t* e2ecc_hdr)
{

    return (PHYMOD_E_NONE);
}

/**/
int plp_aperta_pm8x50_port_e2e_enable_set(int unit, int port,
                               pm_info_t pm_info,
                               int enable)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_e2e_enable_get(int unit, int port,
                               pm_info_t pm_info,
                               int* enable)
{

    return (PHYMOD_E_NONE);
}

/*get the speed for the specified port*/
int plp_aperta_pm8x50_port_speed_get(int unit, int port, pm_info_t pm_info, int* speed)
{
    phymod_phy_speed_config_t speed_config;
    plp_aperta_phymod_phy_access_t phy_access;
    portmod_access_get_params_t params;
    int nof_phys;

    *speed = 0;

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_access_get_params_t_init(unit, &params));

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_port_phy_lane_access_get(unit, port, pm_info,
                   &params, 1, &phy_access, &nof_phys, NULL));
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_aperta_tscbh_driver.f_phymod_phy_speed_config_get(&phy_access, &speed_config));

    *speed = speed_config.data_rate;

    return PHYMOD_E_NONE;
}

/*TSC refere clock input and output set/get*/
int plp_aperta_pm8x50_port_tsc_refclock_set(int unit, int port,
                                 pm_info_t pm_info,
                                 int ref_in, int ref_out)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_tsc_refclock_get(int unit, int port,
                                 pm_info_t pm_info,
                                 int* ref_in, int* ref_out)
{

    return (PHYMOD_E_NONE);
}

/*Port discard set*/
int plp_aperta_pm8x50_port_discard_set(int unit, int port, pm_info_t pm_info, int discard)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_discard_set(&phy_access, discard));


    return PHYMOD_E_NONE;
}

/*Port soft reset set set*/
int plp_aperta_pm8x50_port_soft_reset_set(int unit, int port, pm_info_t pm_info,
                               int idx, int val, int flags)
{

    return (PHYMOD_E_NONE);
}

/*Port tx_en=0 and softreset mac*/
int plp_aperta_pm8x50_port_tx_down(int unit, int port, pm_info_t pm_info)
{

    APERTA_PHY_ACCESS_GET(pm_info);

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_tx_enable_set(&phy_access, 0));
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_discard_set(&phy_access, 0));
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_soft_reset_set(&phy_access, 1));


    return PHYMOD_E_NONE;
}

/*reconfig pgw.*/
int plp_aperta_pm8x50_port_pgw_reconfig(int unit, int port, pm_info_t pm_info,
                             const portmod_port_mode_info_t* pmode,
                             int phy_port, int flags)
{

    return (PHYMOD_E_NONE);
}

/*Routine to notify internal phy of external phy link state.*/
int plp_aperta_pm8x50_port_notify(int unit, int port, pm_info_t pm_info, int link)
{

    return (PHYMOD_E_NONE);
}

/*"port control phy timesync config set/get"*/
int plp_aperta_pm8x50_port_control_phy_timesync_set(int unit,
                               int port, pm_info_t pm_info,
                               portmod_port_control_phy_timesync_t config,
                               uint64_t value)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_control_phy_timesync_get(int unit,
                              int port, pm_info_t pm_info,
                              portmod_port_control_phy_timesync_t config,
                              uint64_t* value)
{

    return (PHYMOD_E_NONE);
}

/*"port timesync config set/get"*/
int plp_aperta_pm8x50_port_timesync_config_set(int unit, int port, pm_info_t pm_info,
                              const portmod_phy_timesync_config_t* config)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_timesync_config_get(int unit, int port, pm_info_t pm_info,
                                    portmod_phy_timesync_config_t* config)
{

    return (PHYMOD_E_NONE);
}

/*"port timesync enable set/get"*/
int plp_aperta_pm8x50_port_timesync_enable_set(int unit, int port,
                                    pm_info_t pm_info,
                                    uint32_t enable)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_timesync_enable_get(int unit, int port,
                                    pm_info_t pm_info,
                                     uint32_t* enable)
{

    return (PHYMOD_E_NONE);
}

/*"port timesync nco addend  set/get"*/
int plp_aperta_pm8x50_port_timesync_nco_addend_set(int unit, int port,
                                        pm_info_t pm_info,
                                        uint32_t freq_step)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_timesync_nco_addend_get(int unit, int port,
                                        pm_info_t pm_info,
                                        uint32_t* freq_step)
{

    return (PHYMOD_E_NONE);
}

/*"port timesync framesync info  set/get"*/
int plp_aperta_pm8x50_port_timesync_framesync_mode_set(int unit,
                                int port, pm_info_t pm_info,
                                const portmod_timesync_framesync_t* framesync)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_timesync_framesync_mode_get(int unit,
                                     int port, pm_info_t pm_info,
                                     portmod_timesync_framesync_t* framesync)
{

    return (PHYMOD_E_NONE);
}

/*"port timesync local time  set/get"*/
int plp_aperta_pm8x50_port_timesync_local_time_set(int unit, int port,
                                        pm_info_t pm_info,
                                        uint64_t local_time)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_timesync_local_time_get(int unit, int port,
                                        pm_info_t pm_info,
                                        uint64_t* local_time)
{

    return (PHYMOD_E_NONE);
}

/*"port timesync framesync info  set/get"*/
int plp_aperta_pm8x50_port_timesync_load_ctrl_set(int unit, int port, pm_info_t pm_info,
                                       uint32_t load_once, uint32_t load_always)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_timesync_load_ctrl_get(int unit, int port, pm_info_t pm_info,
                                       uint32_t* load_once, uint32_t* load_always)
{

    return (PHYMOD_E_NONE);
}

/*"port timesync tx timestamp offset set/get"*/
int plp_aperta_pm8x50_port_timesync_tx_timestamp_offset_set(int unit, int port,
                                                 pm_info_t pm_info,
                                                 uint32_t ts_offset)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_timesync_tx_timestamp_offset_get(int unit, int port,
                                                 pm_info_t pm_info,
                                                 uint32_t* ts_offset)
{

    return (PHYMOD_E_NONE);
}

/*"port timesync rx timestamp offset set/get"*/
int plp_aperta_pm8x50_port_timesync_rx_timestamp_offset_set(int unit, int port,
                                                 pm_info_t pm_info,
                                                 uint32_t ts_offset)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_timesync_rx_timestamp_offset_get(int unit, int port,
                                                 pm_info_t pm_info,
                                                 uint32_t* ts_offset)
{

    return (PHYMOD_E_NONE);
}

/*set/get interrupt enable value. */
int plp_aperta_pm8x50_port_interrupt_enable_set(int unit, int port,
                                     pm_info_t pm_info,
                                     int intr_type, uint32_t val)
{
    /* Aperta uses top level interrupt*/
    return PHYMOD_E_NONE;
}
int plp_aperta_pm8x50_port_interrupt_enable_get(int unit, int port,
                                     pm_info_t pm_info,
                                     int intr_type, uint32_t* val)
{

    /* Aperta uses top level interrupt*/
    return PHYMOD_E_NONE;
}


/*get interrupt status value. */
int plp_aperta_pm8x50_port_interrupt_get(int unit, int port, pm_info_t pm_info, int intr_type, uint32_t* val)
{
    return PHYMOD_E_NONE;
}

/*get interrupt value array. */
int plp_aperta_pm8x50_port_interrupts_get(int unit, int port, pm_info_t pm_info,
                               int arr_max_size, uint32_t* intr_arr,
                               uint32_t* size)
{
    return PHYMOD_E_NONE;
}

/* portmod check if external phy is legacy*/
int plp_aperta_pm8x50_port_check_legacy_phy(int unit, int port,
                                 pm_info_t pm_info,
                                 int* legacy_phy)
{

    return (PHYMOD_E_NONE);
}

/* portmod phy failover mode*/
int plp_aperta_pm8x50_port_failover_mode_set(int unit, int port,
                                  pm_info_t pm_info,
                                  plp_aperta_phymod_failover_mode_t failover)
{

    return (PHYMOD_E_NONE);
}
int plp_aperta_pm8x50_port_failover_mode_get(int unit, int port,
                                  pm_info_t pm_info,
                                  plp_aperta_phymod_failover_mode_t* failover)
{

    return (PHYMOD_E_NONE);
}

/* portmod port rsv mask set*/
int plp_aperta_pm8x50_port_mac_rsv_mask_set(int unit, int port,
                                 pm_info_t pm_info,
                                 uint32_t rsv_mask)
{
    APERTA_PHY_ACCESS_GET(pm_info);
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rsv_mask_set(&phy_access, rsv_mask));


    return PHYMOD_E_NONE;
}

/* portmod port mib reset toggle*/
int plp_aperta_pm8x50_port_mib_reset_toggle(int unit, int port,
                                 pm_info_t pm_info,
                                 int port_index)
{
    /* Use CDMAC_MIB_COUNTER_CTRL [4] to clear MIB counter*/

    return (PHYMOD_E_NONE);
}

/* portmod restore information after warmboot*/
int plp_aperta_pm8x50_port_warmboot_db_restore(int unit, int port, pm_info_t pm_info,
                        const portmod_port_interface_config_t* intf_config,
                        const portmod_port_init_config_t* init_config,
                        plp_aperta_phymod_operation_mode_t phy_op_mode)
{

    return (PHYMOD_E_NONE);
}

/* portmod port flow control config*/
int plp_aperta_pm8x50_port_flow_control_set(int unit, int port,
                                 pm_info_t pm_info,
                                 int merge_mode_en,
                                 int parallel_fc_en)
{

    return (PHYMOD_E_NONE);
}

/*Portmod state for any logical port dynamixc settings*/
int plp_aperta_pm8x50_port_update_dynamic_state(int unit, int port,
                                     pm_info_t pm_info,
                                     uint32_t port_dynamic_state)
{

    return (PHYMOD_E_NONE);
}

/*get phy operation mode. */
int plp_aperta_pm8x50_port_phy_op_mode_get(int unit, int port, pm_info_t pm_info,
                                plp_aperta_phymod_operation_mode_t* val)
{

    return (PHYMOD_E_NONE);
}

/*Returns if the PortMacro associated with the port is initialized or not*/
int plp_aperta_pm8x50_pm_is_initialized(int unit, int pm_id, pm_info_t pm_info, int* is_initialized)
{
    int is_core_initialized = 0;
     PM8x50_IS_CORE_INITIALIZED_GET(unit, pm_info, is_core_initialized);
     *is_initialized = is_core_initialized;

    return PHYMOD_E_NONE;
}

/* get the logical port bitmap of the current PM */
int plp_aperta_pm8x50_pm_logical_pbmp_get(int unit, int pm_id, pm_info_t pm_info,
                               int* logical_pbmp)
{
    return PHYMOD_E_NONE;
}

int plp_aperta_pm8x50_core_add(int unit, int pm_id, pm_info_t pm_info, const portmod_port_add_info_t* add_info)
{
    int is_initialized = 0;
    int side = 0;

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_pm_is_initialized(unit, pm_id, pm_info, &is_initialized));

    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {

        PM_8x50_INFO(pm_info)->int_phy_access.port_loc = side;
        PM_8x50_INFO(pm_info)->int_core_access.port_loc = side;

        PHYMOD_IF_ERR_RETURN(plp_aperta_pm8x50_pm_serdes_core_init(unit, pm_id, pm_info, add_info));

    }

    return PHYMOD_E_NONE;
}

