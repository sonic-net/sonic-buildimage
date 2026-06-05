/*
*
*  *
*  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
*  *
*
*/

/*
 * Includes
 */
#include <phymod/phymod.h>
#include <tier1/aperta_pm_seq.h>
#include <include/pm8x50_shared.h>
#include <include/cdmac.h>
#include <include/portmod.h>
#include <include/portmod_dispatch.h>
#include <tier1/bcmi_aperta_d_defs.h>
#include <include/bcmi_aperta_cdmac_defs.h>
#include <phymod/phymod_acc.h>
#include <tier1/aperta_msg_tasks.h>
#include <include/aperta_tscbh.h>
#include "blackhawk/tier1/blackhawk_cfg_seq.h"
#include <blackhawk.h>
#include <phymod/phymod_util.h>
#include <bcmi_aperta_tscbh_xgxs_defs.h>
#include <aperta_tscbh_diagnostics.h>

extern aperta_pm_info_t _plp_aperta_pm_info[APERTA_MAX_PM_INFO];
const int plp_aperta_port_mode_sys_line_lane[APERTA_MAX_GB_RGB_PORT][APERTA_MAX_GB_RGB_PORT_ENTITY] = {
                                                                                                    /* GB50G, 100G*/
                                                                                                   {APERTA_GB, 0x3,   0x1},
                                                                                                   {APERTA_GB, 0xc,   0x4},
                                                                                                   {APERTA_GB, 0x30,  0x10},
                                                                                                   {APERTA_GB, 0xc,   0x40},
                                                                                                   {APERTA_GB, 0xF,   0x3},
                                                                                                   {APERTA_GB, 0xF0,  0x30},
                                                                                                    /*RGB 50G, 100G,  40G*/
                                                                                                   {APERTA_RGB, 0x1,  0x3},
                                                                                                   {APERTA_RGB, 0x4,  0xc},
                                                                                                   {APERTA_RGB, 0x10, 0x30},
                                                                                                   {APERTA_RGB, 0x40, 0xc0},
                                                                                                   {APERTA_RGB, 0xc,  0x40},
                                                                                                   {APERTA_RGB, 0x3,  0xF},
                                                                                                   {APERTA_RGB, 0x30, 0xF0},
                                                                                                   {APERTA_RGB, 0x3,  0xF},
                                                                                                   {APERTA_RGB, 0x30, 0xF0}};

int plp_aperta_convert_speed_to_bits(uint32_t speed)
{
    uint32_t speed_in_bits=0;
    switch(speed)
    {
        case APERTA_SPEED_400G:
            speed_in_bits = 1;
            break;
        case APERTA_SPEED_200G:
            speed_in_bits = 2;
            break;
        case APERTA_SPEED_100G:
            speed_in_bits = 3;
            break;
        case APERTA_SPEED_10G:
            speed_in_bits = 4;
            break;
        case APERTA_SPEED_25G:
            speed_in_bits = 5;
            break;
        case APERTA_SPEED_20G:
            speed_in_bits = 6;
            break;
        case APERTA_SPEED_40G:
            speed_in_bits = 7;
            break;
        case APERTA_SPEED_50G:
            speed_in_bits = 8;
            break;
       default:
            speed_in_bits = 1; /*set default to 400G */
            break;
    }
    return speed_in_bits;
}

int plp_aperta_get_speed_from_bits(uint32_t speed_in_bits)
{
    uint32_t speed=0;
    switch(speed_in_bits)
    {
        case 1:
            speed = APERTA_SPEED_400G;
            break;
        case 2:
            speed = APERTA_SPEED_200G;
            break;
        case 3:
            speed = APERTA_SPEED_100G;
            break;
        case 4:
            speed = APERTA_SPEED_10G;
            break;
        case 5 :
            speed = APERTA_SPEED_25G;
            break;
        case 6:
            speed = APERTA_SPEED_20G;
            break;
        case 7:
            speed = APERTA_SPEED_40G;
            break;
        case 8:
            speed = APERTA_SPEED_50G;
            break;
       default:
            speed = APERTA_SPEED_PLL1_DEF; /*set default to 400G */
            break;

    }
    return speed;
}



int plp_aperta_restore_pm_info(const plp_aperta_phymod_phy_access_t* phy )
{
    BCMI_APERTA_D_CTRL_SWGPREG0Er_t swgpreg_e;
    BCMI_APERTA_D_CTRL_SWGPREG0Fr_t swgpreg_f;
    BCMI_APERTA_D_CTRL_SWGPREG10r_t swgpreg_10;
    unsigned short cnt = 0, value = 0;
    BCMI_APERTA_D_CTRL_SWGPREG11r_t swgpreg_11;
    BCMI_APERTA_D_CTRL_SWGPREG12r_t swgpreg_12;
    BCMI_APERTA_D_CTRL_SWGPREG13r_t swgpreg_13;
    BCMI_APERTA_D_CTRL_SWGPREG14r_t swgpreg_14;
    BCMI_APERTA_D_CTRL_SWGPREG15r_t swgpreg_15;
    BCMI_APERTA_D_CTRL_SWGPREG16r_t swgpreg_16;
    BCMI_APERTA_D_CTRL_SWGPREG17r_t swgpreg_17;
    BCMI_APERTA_D_CTRL_SWGPREG18r_t swgpreg_18;
    BCMI_APERTA_D_CTRL_SWGPREG1Ar_t swgpreg_1A;
    BCMI_APERTA_D_CTRL_SWGPREG1Br_t swgpreg_1B;
    BCMI_APERTA_D_CTRL_SWGPREG1Cr_t swgpreg_1C;
    BCMI_APERTA_D_CTRL_SWGPREG1Dr_t swgpreg_1D;
    plp_aperta_phymod_phy_access_t phy_copy;
    int lane_index = 0, lmap = 0, lane_num = 0, temp = 0;
    unsigned int pll = 0, lane = 0, speed = 0;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG0Er(&phy->access, &swgpreg_e));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG0Fr(&phy->access, &swgpreg_f));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG10r(&phy->access, &swgpreg_10));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG15r(&phy->access, &swgpreg_15));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG16r(&phy->access, &swgpreg_16));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG17r(&phy->access, &swgpreg_17));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG18r(&phy->access, &swgpreg_18));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG1Ar(&phy->access, &swgpreg_1A));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG1Br(&phy->access, &swgpreg_1B));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG1Cr(&phy->access, &swgpreg_1C));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG1Dr(&phy->access, &swgpreg_1D));
    pll = swgpreg_e.v[0] >> 8;
    if (pll == bcmplpapertaVco20p625G) {
        speed = 40000;
    } else if (pll == bcmplpapertaVco25p781G) {
        speed = 100000;
    } else if (pll == bcmplpapertaVco26p562G) {
        speed = 400000;
    }
    for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy->access.addr) {
            value = swgpreg_e.v[0] & 0x1 ;
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_ISCOREINITIALIZED] = value ; 
            value = (swgpreg_e.v[0] & 0x2) >> 1 ;
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_ISACTIVE] = value ; 
            value = (swgpreg_e.v[0] & 0x4) >> 2 ;
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_ISBYPASSED] = value ; 
            value = (swgpreg_e.v[0] & 0x8) >> 3 ;
            _plp_aperta_pm_info[cnt].is_fw_dloaded = value;
            for (lane=0 ; lane < APERTA_PM_NUM_LANES; lane++) {
                _plp_aperta_pm_info[cnt].speed[lane] = speed;
                _plp_aperta_pm_info[cnt].sys_speed[lane] = speed;
            }
            for (lane_index = 0; lane_index <8; lane_index ++) {
                if (lane_index == 0 || lane_index == 1) {
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_CTRL_SWGPREG11r(&phy->access, &swgpreg_11));
                    temp = (swgpreg_11.v[0] & (0xFF << (lane_index %2)*8)) >> ((lane_index %2)*8);
                } else if (lane_index == 2 || lane_index == 3) {
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_CTRL_SWGPREG12r(&phy->access, &swgpreg_12));
                    temp = (swgpreg_12.v[0] & (0xFF << (lane_index %2)*8)) >> ((lane_index %2)*8);
                } else if (lane_index == 4 || lane_index == 5) {
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_CTRL_SWGPREG13r(&phy->access, &swgpreg_13));
                    temp = (swgpreg_13.v[0] & (0xFF << (lane_index %2)*8)) >> ((lane_index %2)*8);
                } else {
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_CTRL_SWGPREG14r(&phy->access, &swgpreg_14));
                    temp = (swgpreg_14.v[0] & (0xFF << (lane_index %2)*8)) >> ((lane_index %2)*8);
                }
                lane_num = temp >> 4;
                APERTA_NUM_TO_LMAP(lane_num, lmap);
                speed = plp_aperta_get_speed_from_bits(temp & 0xF);
                if (speed != APERTA_SPEED_PLL1_DEF) {
                    _plp_aperta_pm_info[cnt].speed[lane_index] = speed; 
                    _plp_aperta_pm_info[cnt].speed[lane_index] |= lmap << 24; 
                }
            }
            for (lane_index = 0; lane_index <8; lane_index ++) {
                if (lane_index == 0 || lane_index == 1) {
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_CTRL_SWGPREG1Ar(&phy->access, &swgpreg_1A));
                    temp = (swgpreg_1A.v[0] & (0xFF << (lane_index %2)*8)) >> ((lane_index %2)*8);
                } else if (lane_index == 2 || lane_index == 3) {
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_CTRL_SWGPREG1Br(&phy->access, &swgpreg_1B));
                    temp = (swgpreg_1B.v[0] & (0xFF << (lane_index %2)*8)) >> ((lane_index %2)*8);
                } else if (lane_index == 4 || lane_index == 5) {
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_CTRL_SWGPREG1Cr(&phy->access, &swgpreg_1C));
                    temp = (swgpreg_1C.v[0] & (0xFF << (lane_index %2)*8)) >> ((lane_index %2)*8);
                } else {
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_CTRL_SWGPREG1Dr(&phy->access, &swgpreg_1D));
                    temp = (swgpreg_1D.v[0] & (0xFF << (lane_index %2)*8)) >> ((lane_index %2)*8);
                }
                lane_num = temp >> 4;
                APERTA_NUM_TO_LMAP(lane_num, lmap);
                speed = plp_aperta_get_speed_from_bits(temp & 0xF);
                if (speed != APERTA_SPEED_PLL1_DEF) {
                    _plp_aperta_pm_info[cnt].sys_speed[lane_index] = speed; 
                    _plp_aperta_pm_info[cnt].sys_speed[lane_index] |= lmap << 24; 
                }
            }
            value = (swgpreg_f.v[0] & 0xFF);
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_PLL0ACTIVELANEBITMAP] = value ; 
            value = ((swgpreg_f.v[0] & 0xFF00) >> 8);
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_PLL1ACTIVELANEBITMAP] = value ; 
            value = (swgpreg_10.v[0] & 0xFF);
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_PLL0ADVLANEBITMAP] = value ; 
            value = ((swgpreg_10.v[0] & 0xFF00) >> 8);
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_PLL1ADVLANEBITMAP] = value ; 

            value = (swgpreg_17.v[0] & 0xFF);
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_SYS_PLL0ACTIVELANEBITMAP] = value ; 
            value = ((swgpreg_17.v[0] & 0xFF00) >> 8);
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_SYS_PLL1ACTIVELANEBITMAP] = value ; 
            value = (swgpreg_18.v[0] & 0xFF);
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_SYS_PLL0ADVLANEBITMAP] = value ; 
            value = ((swgpreg_18.v[0] & 0xFF00) >> 8);
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[APERTA_SYS_PLL1ADVLANEBITMAP] = value ; 

            PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->ovco = swgpreg_15.v[0] & 0xFF;
            PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->tvco = (swgpreg_15.v[0] >> 8) & 0xFF;
            PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->sys_ovco = swgpreg_16.v[0] & 0xFF;
            PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->sys_tvco = (swgpreg_16.v[0] >> 8) & 0xFF;
            PHYMOD_IF_ERR_RETURN
                  (plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(&phy_copy));
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in restoring PM Info\n"));
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_pm_warmboot_init(const plp_aperta_phymod_phy_access_t* phy)
{
    int unit = 0, lane = 0;
    portmod_pm_create_info_t pm_create_info;
    portmod_pm_create_info_internal_t pm_internal_info;
    pm_info_t init_pm_info;
    struct pm_info_s init_pm_info_1;
    plp_aperta_phymod_core_firmware_info_t fw_info;

    PHYMOD_MEMSET(&init_pm_info_1, 0, sizeof(init_pm_info_1));
    init_pm_info = &init_pm_info_1;
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_pm_create_info_t_init(unit, &pm_create_info));
    pm_create_info.phys = 1;
#ifdef PHYMOD_APERTA_SUPPORT
    pm_create_info.type = portmodDispatchTypePm8x50;
#endif
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_phy_access_t_init(&pm_create_info.pm_specific_info.pm8x50.access));

    /* Moving access, MDIO address, bus etc*/
    PHYMOD_MEMCPY(&pm_create_info.pm_specific_info.pm8x50.access, phy, sizeof(plp_aperta_phymod_phy_access_t));

    /* Use default external loader if NULL */
    pm_create_info.pm_specific_info.pm8x50.external_fw_loader = NULL;

    /* Polarity */
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_polarity_t_init (&(pm_create_info.pm_specific_info.pm8x50.polarity)));

    /* Lane map */
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_lane_map_t_init(&(pm_create_info.pm_specific_info.pm8x50.lane_map)));


    pm_create_info.pm_specific_info.pm8x50.lane_map.num_of_lanes = 8;
    pm_create_info.pm_specific_info.pm8x50.core_num = 1;

    /* Assiginig default lane map i.e. Tx/Rx lane 0 to 0, 1 to 1, etc*/
    for (lane=0 ; lane< APERTA_PM_NUM_LANES; lane++) {
        pm_create_info.pm_specific_info.pm8x50.lane_map.lane_map_tx[lane] =
             (0x76543210 >> (lane * 4)) & 0xF;
        pm_create_info.pm_specific_info.pm8x50.lane_map.lane_map_rx[lane] =
              (0x76543210 >> (lane * 4)) & 0xF;
    }

    /* Defaults to 156MHZ*/
    pm_create_info.pm_specific_info.pm8x50.ref_clk = phymodRefClk156Mhz;

    PHYMOD_MEMCPY(&pm_internal_info, &pm_create_info, sizeof(portmod_pm_create_info_internal_t));

    PHYMOD_IF_ERR_RETURN(
            _plp_aperta_core_firmware_info_get(&phy->access, &fw_info));
    PHYMOD_CRIT_INFO((" In Warmboot ..\n"));
    if( fw_info.fw_version == 0x1)/*when no fw is present*/
    {
        PHYMOD_CRIT_INFO(("Firmware is not present \n Cannot perform warmboot without firmware . "));
        return PHYMOD_E_INTERNAL;
    }
    PHYMOD_CRIT_INFO((" FW version:0x%x\n", fw_info.fw_version));

    /* Add PM to PortMod */
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_pm_init(unit, &pm_internal_info, 0, init_pm_info));

#ifdef PHYMOD_APERTA_SUPPORT
    PM_8x50_INFO(init_pm_info)->int_core_access.type = phymodDispatchTypeAperta;
    PM_8x50_INFO(init_pm_info)->int_core_access.access.tvco_pll_index = APERTA_TVCO_PLL_INDEX;
    PHYMOD_MEMCPY(&PM_8x50_INFO(init_pm_info)->int_phy_access, &PM_8x50_INFO(init_pm_info)->int_core_access,
        sizeof(plp_aperta_phymod_phy_access_t));
#endif
    PHYMOD_ACC_F_CLAUSE45_SET(&(PM_8x50_INFO(init_pm_info)->int_core_access.access));
    PHYMOD_IF_ERR_RETURN(plp_aperta_add_pm_info(phy->access.addr, init_pm_info));

    /* Restore values in global struct array using pm_info from register */
    PHYMOD_IF_ERR_RETURN(plp_aperta_restore_pm_info(phy ));

    return PHYMOD_E_NONE;
}

int plp_aperta_get_phyinfo_from_pminfo(int phy_id, plp_aperta_phymod_access_t *phy)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
            PHYMOD_MEMCPY(phy, &(_plp_aperta_pm_info[cnt].pm_info->pm_data.pm8x50_db->int_core_access.access), sizeof(plp_aperta_phymod_access_t));
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in getting Phy access\n"));
    return PHYMOD_E_INTERNAL;
}

int plp_aperta_write_warmboot_reg(int phy_id , unsigned int type , int value, int lane_index, int lane_map)
{
    BCMI_APERTA_D_CTRL_SWGPREG0Er_t swgpreg_e;
    BCMI_APERTA_D_CTRL_SWGPREG0Fr_t swgpreg_f;
    BCMI_APERTA_D_CTRL_SWGPREG10r_t swgpreg_10;
    BCMI_APERTA_D_CTRL_SWGPREG11r_t swgpreg_11;
    BCMI_APERTA_D_CTRL_SWGPREG12r_t swgpreg_12;
    BCMI_APERTA_D_CTRL_SWGPREG13r_t swgpreg_13;
    BCMI_APERTA_D_CTRL_SWGPREG14r_t swgpreg_14;
    BCMI_APERTA_D_CTRL_SWGPREG17r_t swgpreg_17;
    BCMI_APERTA_D_CTRL_SWGPREG18r_t swgpreg_18;
    BCMI_APERTA_D_CTRL_SWGPREG1Ar_t swgpreg_1A;
    BCMI_APERTA_D_CTRL_SWGPREG1Br_t swgpreg_1B;
    BCMI_APERTA_D_CTRL_SWGPREG1Cr_t swgpreg_1C;
    BCMI_APERTA_D_CTRL_SWGPREG1Dr_t swgpreg_1D;

    plp_aperta_phymod_access_t phy_access;
    unsigned char temp = 0;
  
    plp_aperta_get_phyinfo_from_pminfo(phy_id, &phy_access);
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG0Er(&phy_access, &swgpreg_e));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG0Fr(&phy_access, &swgpreg_f));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG10r(&phy_access, &swgpreg_10));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG17r(&phy_access, &swgpreg_17));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG18r(&phy_access, &swgpreg_18));

    switch (type) {
        case APERTA_ISCOREINITIALIZED:
            swgpreg_e.v[0] &= ~1;
            swgpreg_e.v[0] |= value & 1;
        break;
        case APERTA_ISACTIVE:
            swgpreg_e.v[0] &= ~2;
            swgpreg_e.v[0] |= (value & 1) << 1;
        break;
        case APERTA_ISBYPASSED:
            swgpreg_e.v[0] &= ~4;
            swgpreg_e.v[0]|= (value & 1) << 2;
        break;
        case APERTA_FWDLOAD:
            swgpreg_e.v[0] &= ~8;
            swgpreg_e.v[0] |= (value & 1) << 3;
        break;
        case APERTA_SPEED:
            APERTA_LMAP_TO_NUM(lane_map, temp);
            temp = (temp << 4);
            temp |= (plp_aperta_convert_speed_to_bits(value) & 0xF);
            if (lane_index == 0 || lane_index == 1) {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_CTRL_SWGPREG11r(&phy_access, &swgpreg_11));
                swgpreg_11.v[0] &= ~(0xFF << ((lane_index % 2) * 8));
                swgpreg_11.v[0] |= (temp << ((lane_index % 2) * 8));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_CTRL_SWGPREG11r(&phy_access, swgpreg_11));
            } else if (lane_index == 2 || lane_index == 3) {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_CTRL_SWGPREG12r(&phy_access, &swgpreg_12));
                swgpreg_12.v[0] &= ~(0xFF << ((lane_index % 2) * 8));
                swgpreg_12.v[0] |= (temp << ((lane_index % 2) * 8));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_CTRL_SWGPREG12r(&phy_access, swgpreg_12));
            } else if (lane_index == 4 || lane_index == 5) {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_CTRL_SWGPREG13r(&phy_access, &swgpreg_13));
                swgpreg_13.v[0] &= ~(0xFF << ((lane_index % 2) * 8));
                swgpreg_13.v[0] |= (temp << ((lane_index % 2) * 8));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_CTRL_SWGPREG13r(&phy_access, swgpreg_13));
            } else if (lane_index == 6 || lane_index == 7) {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_CTRL_SWGPREG14r(&phy_access, &swgpreg_14));
                swgpreg_14.v[0] &= ~(0xFF << ((lane_index % 2) * 8));
                swgpreg_14.v[0] |= (temp << ((lane_index % 2) * 8));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_CTRL_SWGPREG14r(&phy_access, swgpreg_14));
            }
            return PHYMOD_E_NONE;
        case APERTA_SYS_SPEED:
            APERTA_LMAP_TO_NUM(lane_map, temp);
            temp = (temp << 4);
            temp |= (plp_aperta_convert_speed_to_bits(value) & 0xF);
            if (lane_index == 0 || lane_index == 1) {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_CTRL_SWGPREG1Ar(&phy_access, &swgpreg_1A));
                swgpreg_1A.v[0] &= ~(0xFF << ((lane_index % 2) * 8));
                swgpreg_1A.v[0] |= (temp << ((lane_index % 2) * 8));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_CTRL_SWGPREG1Ar(&phy_access, swgpreg_1A));
            } else if (lane_index == 2 || lane_index == 3) {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_CTRL_SWGPREG1Br(&phy_access, &swgpreg_1B));
                swgpreg_1B.v[0] &= ~(0xFF << ((lane_index % 2) * 8));
                swgpreg_1B.v[0] |= (temp << ((lane_index % 2) * 8));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_CTRL_SWGPREG1Br(&phy_access, swgpreg_1B));
            } else if (lane_index == 4 || lane_index == 5) {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_CTRL_SWGPREG1Cr(&phy_access, &swgpreg_1C));
                swgpreg_1C.v[0] &= ~(0xFF << ((lane_index % 2) * 8));
                swgpreg_1C.v[0] |= (temp << ((lane_index % 2) * 8));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_CTRL_SWGPREG1Cr(&phy_access, swgpreg_1C));
            } else if (lane_index == 6 || lane_index == 7) {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_CTRL_SWGPREG1Dr(&phy_access, &swgpreg_1D));
                swgpreg_1D.v[0] &= ~(0xFF << ((lane_index % 2) * 8));
                swgpreg_1D.v[0] |= (temp << ((lane_index % 2) * 8));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_CTRL_SWGPREG1Dr(&phy_access, swgpreg_1D));
            }
            return PHYMOD_E_NONE;

        case APERTA_PLL0ACTIVELANEBITMAP:
            swgpreg_f.v[0] &= ~(0xFF);
            swgpreg_f.v[0] |= value;
        break;
        case APERTA_PLL1ACTIVELANEBITMAP:
            swgpreg_f.v[0] &= ~(0xFF00);
            swgpreg_f.v[0] |= (value<<8);
        break;
        case APERTA_PLL0ADVLANEBITMAP:
            swgpreg_10.v[0] &= ~(0xFF);
            swgpreg_10.v[0] |= value;
        break;
        case APERTA_PLL1ADVLANEBITMAP:
            swgpreg_10.v[0] &= ~(0xFF00);
            swgpreg_10.v[0] |= (value<<8);
        break;
        case APERTA_SYS_PLL0ACTIVELANEBITMAP:
            swgpreg_17.v[0] &= ~(0xFF);
            swgpreg_17.v[0] |= value;
        break;
        case APERTA_SYS_PLL1ACTIVELANEBITMAP:
            swgpreg_17.v[0] &= ~(0xFF00);
            swgpreg_17.v[0] |= (value<<8);
        break;
        case APERTA_SYS_PLL0ADVLANEBITMAP:
            swgpreg_18.v[0] &= ~(0xFF);
            swgpreg_18.v[0] |= value;
        break;
        case APERTA_SYS_PLL1ADVLANEBITMAP:
            swgpreg_18.v[0] &= ~(0xFF00);
            swgpreg_18.v[0] |= (value<<8);
        break;

    }
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_CTRL_SWGPREG0Er(&phy_access, swgpreg_e));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_CTRL_SWGPREG0Fr(&phy_access, swgpreg_f));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_CTRL_SWGPREG10r(&phy_access, swgpreg_10));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_CTRL_SWGPREG17r(&phy_access, swgpreg_17));
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_CTRL_SWGPREG18r(&phy_access, swgpreg_18));

    return PHYMOD_E_NONE;
}


int plp_aperta_add_pm_info(int phy_id, pm_info_t pm_info)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt < APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
            PHYMOD_DEBUG_ERROR(("PHY already initialized\n"));
            return PHYMOD_E_FAIL;
        }
        if (_plp_aperta_pm_info[cnt].phy_id == APERTA_UNINIT_PHYS) {
            _plp_aperta_pm_info[cnt].phy_id = phy_id;
            _plp_aperta_pm_info[cnt].pm_info = (pm_info_t)PHYMOD_MALLOC(sizeof(struct pm_info_s), "PM ALLOC");
            PHYMOD_MEMCPY(_plp_aperta_pm_info[cnt].pm_info, pm_info, sizeof(struct pm_info_s));
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in adding PM info\n"));
    return PHYMOD_E_RESOURCE;
}

int plp_aperta_remove_pm_info(int phy_id)
{
    unsigned short cnt = 0, lane_index = 0;
    for (cnt = 0; cnt < APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
            _plp_aperta_pm_info[cnt].phy_id = APERTA_UNINIT_PHYS;
            _plp_aperta_pm_info[cnt].is_fw_dloaded = 0;
            /* This memory has been allocated in pm_init() */
            PHYMOD_FREE(_plp_aperta_pm_info[cnt].pm_info->pm_data.pm8x50_db);
            PHYMOD_FREE(_plp_aperta_pm_info[cnt].pm_info);
            /* Initializing to default speed*/
            for (lane_index = 0; lane_index < APERTA_PM_NUM_LANES ; lane_index++) {
                _plp_aperta_pm_info[cnt].speed[lane_index] = 100000;
                _plp_aperta_pm_info[cnt].sys_speed[lane_index] = 100000;
            }
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in removing PM info\n"));
    return PHYMOD_E_RESOURCE;
}

int plp_aperta_get_pm_info(int phy_id, pm_info_t pm_info)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
            PHYMOD_MEMCPY(pm_info, _plp_aperta_pm_info[cnt].pm_info, sizeof(struct pm_info_s));
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in getting PM info\n"));
    return PHYMOD_E_INTERNAL;
}

void plp_aperta_plp_aperta_update_pm_info(int phy_id, uint32_t wb_idx, int val)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
            _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[wb_idx] = val;
            if (wb_idx <= APERTA_SYS_PLL1ADVLANEBITMAP) { 
                /* coverity[check_return] */
                plp_aperta_write_warmboot_reg(phy_id, wb_idx, val, 0 , 0);
            }
            break;
        }
    }
}

int plp_aperta_update_vco(plp_aperta_phymod_phy_access_t *phy, int pll, int val) 
{
    BCMI_APERTA_D_CTRL_SWGPREG15r_t swgpreg_15;
    BCMI_APERTA_D_CTRL_SWGPREG16r_t swgpreg_16;
    if (phy->port_loc == phymodPortLocLine) {
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_CTRL_SWGPREG15r(&phy->access, &swgpreg_15));
        if (pll == 0) {
            swgpreg_15.v[0] &= ~0xFF;
            swgpreg_15.v[0] |= val & 0xFF;
        } else {
            swgpreg_15.v[0] &= ~0xFF00;
            swgpreg_15.v[0] |= ((val & 0xFF) << 8);
        }
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_CTRL_SWGPREG15r(&phy->access, swgpreg_15));
    } else {
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_CTRL_SWGPREG16r(&phy->access, &swgpreg_16));
        if (pll == 0) {
            swgpreg_16.v[0] &= ~0xFF;
            swgpreg_16.v[0] |= val & 0xFF;
        } else {
            swgpreg_16.v[0] &= ~0xFF00;
            swgpreg_16.v[0] |= ((val & 0xFF) << 8);
        }
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_CTRL_SWGPREG16r(&phy->access, swgpreg_16));
    }

    return PHYMOD_E_NONE;
}

void plp_aperta_plp_aperta_get_wb_pm_info(int phy_id, uint32_t wb_idx, int *val)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
           *val = _plp_aperta_pm_info[cnt].pm_info->wb_vars_ids[wb_idx];
           break;
        }
    }
}

int plp_aperta_pm_is_fw_dloaded_get(int phy_id, uint32_t *active)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
            *active = _plp_aperta_pm_info[cnt].is_fw_dloaded;
            return PHYMOD_E_NONE;
        }
    }
    *active = 0;
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_is_fw_dloaded_set(int phy_id, uint32_t active)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
            _plp_aperta_pm_info[cnt].is_fw_dloaded = active ;
            PHYMOD_IF_ERR_RETURN(plp_aperta_write_warmboot_reg( phy_id , APERTA_FWDLOAD, active, 0, 0 )) ;
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in setting FW Dload\n"));
    return PHYMOD_E_INTERNAL;
}

int _plp_aperta_pm_info_speed_set(const plp_aperta_phymod_phy_access_t *phy, int lane_index, int *speed, int new_dr, aperta_port_type_t port_type)
{
    unsigned int configured_speed = 0, configured_lane = 0;
	unsigned short reg_updated = 0;
    unsigned int wb_type = (phy->port_loc == phymodPortLocLine) ? APERTA_SPEED : APERTA_SYS_SPEED;
    int temp_index = 0;

    configured_speed = speed[lane_index]  & 0xFFFFFF;
    configured_lane = (speed[lane_index] >> 24) & 0xFF;
    speed[lane_index] = new_dr;
    speed[lane_index] |= ((phy->access.lane_mask & 0xFF) << 24);
    /*Adding it for gearbox mode*/
    if ((new_dr != 400000) && (speed[4] & 0xFFFFFF) == 400000) {
        if (plp_aperta_count_no_bits(phy->access.lane_mask)  == 1) {
            speed[4] = ((0x10) <<24)|new_dr;
            speed[5] = ((0x20) <<24)|new_dr;
            speed[6] = ((0x40) <<24)|new_dr;
            speed[7] = ((0x80) <<24)|new_dr;
        } else if (plp_aperta_count_no_bits(phy->access.lane_mask)  == 2) {
            speed[4] = ((0x30) <<24)|new_dr;
            speed[5] = ((0x30) <<24)|new_dr;
            speed[6] = ((0xc0) <<24)|new_dr;
            speed[7] = ((0xc0) <<24)|new_dr;
        } else {
            speed[4] = ((0xF0) <<24)|new_dr;
            speed[5] = ((0xF0) <<24)|new_dr;
            speed[6] = ((0xF0) <<24)|new_dr;
            speed[7] = ((0xF0) <<24)|new_dr;
        }
        reg_updated = 1;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 4, (speed[4] >> 24 & 0xFF))) ;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 5, (speed[5] >> 24 & 0xFF))) ;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 6, (speed[6] >> 24 & 0xFF))) ;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 7, (speed[7] >> 24 & 0xFF))) ;
    }
    
    if ((new_dr == 100000 || new_dr == 50000 || new_dr == 40000) && (plp_aperta_count_no_bits(phy->access.lane_mask) == 2)) {
        if (port_type > bcmplpApertaPortTypePassthrough) {
            if (new_dr == 50000) {
                if (phy->access.lane_mask & 0x3) {
                    speed[0] = speed[1] = ((APERTA_DEF_GB_Q1_LANEMAP) <<24)|new_dr;
                } else if (phy->access.lane_mask & 0xc) {
                    speed[2] = speed[3] = ((0xC) <<24)|new_dr;
                } else if (phy->access.lane_mask & APERTA_DEF_GB_Q2_LANEMAP) {
                    speed[4] = speed[5] = ((APERTA_DEF_GB_Q2_LANEMAP) <<24)|new_dr;
                } else if (phy->access.lane_mask & 0xC0) {
                    speed[6] = speed[7] = ((0xC0) <<24)|new_dr;
                }
            } else {
                if (phy->access.lane_mask & 0xF) {
                    speed[0] = speed[1] = speed[2] = speed[3] = ((APERTA_DEF_GB_Q1_LANEMAP) <<24)|new_dr;
                } else {
                    speed[4] = speed[5]= speed[6]=speed[7] = ((APERTA_DEF_GB_Q2_LANEMAP) <<24)|new_dr;
                }
            }
            reg_updated = 1;
        } else if (port_type == bcmplpApertaPortTypePassthrough) {
            if (phy->access.lane_mask & 0xF) {
                speed[0] = speed[1] = ((APERTA_DEF_GB_Q1_LANEMAP) <<24)|new_dr;
                speed[2] = speed[3] = ((0xC) <<24)|new_dr;
            } else {
                speed[4] = speed[5] = ((APERTA_DEF_GB_Q2_LANEMAP) <<24)|new_dr;
                speed[6] = speed[7] =((0xC0) <<24)|new_dr;
            }
            reg_updated = 1;
        }
        if (reg_updated) {
            if (phy->access.lane_mask & 0xF) {
                for (temp_index = 0; temp_index < 4; temp_index++) {
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, temp_index, (speed[temp_index] >> 24 & 0xFF))) ;
                }
            } else {
                for (temp_index = 4; temp_index < 8; temp_index++) {
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, temp_index, (speed[temp_index] >> 24 & 0xFF))) ;
                }
            }
        }
    }
    /* Changing speed to single lane for a whole quad*/
    if ((configured_speed == 400000 || 
         (configured_speed == 100000 && (plp_aperta_count_no_bits(configured_lane) != 2)) || configured_speed == 200000 || 
         configured_speed == 40000) &&
            (new_dr == 10000 || new_dr == 25000 || 
            (new_dr == 50000 && plp_aperta_count_no_bits(phy->access.lane_mask)  == 1))) {
        for (temp_index = 0; temp_index < 4; temp_index++) {
            if (phy->access.lane_mask & 0xF) {
                speed[temp_index] = ((1 << temp_index) <<24)|new_dr;
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, temp_index, (1 << temp_index))) ;
                reg_updated = 1;
            } else if (phy->access.lane_mask & 0xF0){
                speed[temp_index+4] = ((0x1 << (temp_index+4)) <<24)|new_dr;
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, (temp_index+4), (1 << (temp_index+4)))) ;
                reg_updated = 1;
            }
        }
    }
    if ( ((new_dr == 50000 || new_dr == 25000 || new_dr == 10000) && plp_aperta_count_no_bits(phy->access.lane_mask)  == 1)) {
        if (port_type > bcmplpApertaPortTypePassthrough) {
            /* In case of GB/RGB lane map needs to be updated for flexing*/
            if (phy->access.lane_mask & APERTA_DEF_GB_Q1_LANEMAP) {
                speed[0] = speed[1] = ((0x1) <<24)|new_dr;
                if (speed[2] >> 24 == 0x3) { /* Clear 100G GB lane map*/
                    speed[2] = ((0x4) <<24)|new_dr;
                    speed[3] = ((0x8) <<24)|new_dr;
                }
            } else if (phy->access.lane_mask & 0xc) {
                speed[2] = speed[3] = ((0x4) <<24)|new_dr;
            } else if (phy->access.lane_mask & APERTA_DEF_GB_Q2_LANEMAP) {
                speed[4] = speed[5] = ((0x10) <<24)|new_dr;
                if (speed[6] >> 24 == 0x30) { /* Clear 100G GB lane map*/
                    speed[6] = ((0x40) <<24)|new_dr;
                    speed[7] = ((0x80) <<24)|new_dr;
                }

            } else if (phy->access.lane_mask & 0xC0) {
                speed[6] = speed[7] = ((0x40) <<24)|new_dr;
            }
            for (temp_index = 0; temp_index < APERTA_PM_NUM_LANES; temp_index++) {
                if (phy->access.lane_mask & (1 << temp_index)) {
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, temp_index, (speed[temp_index] >> 24 & 0xFF))) ;
                    if (temp_index < (APERTA_PM_NUM_LANES-1)) { 
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, (temp_index+1), (speed[temp_index+1] >> 24 & 0xFF))) ;
                    }
                    reg_updated = 1;
                    break;
                }
            }
        } else {
            int temp_index = 0;
            for (temp_index = 0; temp_index < APERTA_PM_NUM_LANES; temp_index++) {
                if ((speed[temp_index] >> 24) & phy->access.lane_mask) {
                    speed[temp_index] = (1 << (temp_index +24)) | new_dr;
                }
            }
            reg_updated =1;
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, lane_index, phy->access.lane_mask)) ;
        }
    }

    if (configured_speed == 400000 && new_dr == 200000) {
        int temp_index = 0;
        if (phy->access.lane_mask & 0xF) {
            speed[4] = (0xF0 <<24)|new_dr;
            speed[5] = (0xF0 <<24)|new_dr;
            speed[6] = (0xF0 <<24)|new_dr;
            speed[7] = (0xF0 <<24)|new_dr;
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 4, (speed[4] >> 24 & 0xFF))) ;
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 5, (speed[5] >> 24 & 0xFF))) ;
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 6, (speed[6] >> 24 & 0xFF))) ;
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 7, (speed[7] >> 24 & 0xFF))) ;
            for (temp_index = 0; temp_index<4; temp_index++) {
                speed[temp_index] = (0xF <<24)|new_dr;
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, temp_index, (speed[temp_index] >> 24 & 0xFF))) ;

            }
            
        } else {
            speed[0] = (0xF <<24)|new_dr;
            speed[1] = (0xF <<24)|new_dr;
            speed[2] = (0xF <<24)|new_dr;
            speed[3] = (0xF <<24)|new_dr;
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 0, (speed[0] >> 24 & 0xFF))) ;
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 1, (speed[1] >> 24 & 0xFF))) ;
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 2, (speed[2] >> 24 & 0xFF))) ;
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, 3, (speed[3] >> 24 & 0xFF))) ;
        }

        reg_updated = 1;
    }
    if ((plp_aperta_count_no_bits(configured_lane) == 2) &&
            (new_dr == 10000 || new_dr == 25000 || 
             (new_dr == 50000 && plp_aperta_count_no_bits(phy->access.lane_mask)  == 1))) {
        int is_gb_rgb_configured = 0, start = 0, end = 0, check=0;
        if (phy->access.lane_mask & 0xF) {
            start = 0;
            end = 4;
            check = APERTA_DEF_GB_Q1_LANEMAP;
        } else {
            start = 4;
            end = 8;
            check = APERTA_DEF_GB_Q2_LANEMAP;
        }
        for (temp_index = start; temp_index < end; temp_index++) {
            if ((speed[temp_index] >> 24) == check) {
                is_gb_rgb_configured ++;
            }
        }
        if (is_gb_rgb_configured >= 3) {
            for (temp_index = start; temp_index < end; temp_index++) {
                speed[temp_index] = ((0x1 << temp_index) << 24 | new_dr);
            }
        }
        for (temp_index = 0; temp_index < 8; temp_index++) {
            if (configured_lane & (1<<temp_index )) {
                speed[temp_index] = ((1 << temp_index) <<24)|new_dr;
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, temp_index, (1<<temp_index))) ;
                reg_updated = 1;
            }
        }
    }
    if (reg_updated == 0) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_write_warmboot_reg(phy->access.addr , wb_type, new_dr, lane_index, phy->access.lane_mask)) ;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_info_speed_set(const plp_aperta_phymod_phy_access_t *phy, int speed, aperta_port_type_t port_type)
{
    unsigned short cnt = 0, lane_index = 0;
    for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy->access.addr) {
            for (lane_index = 0; lane_index < APERTA_PM_NUM_LANES ; lane_index++) {
                if (phy->access.lane_mask & (1 << lane_index)) {
                    if (phy->port_loc == phymodPortLocLine) {
					    PHYMOD_IF_ERR_RETURN(
    					    _plp_aperta_pm_info_speed_set(phy, lane_index, _plp_aperta_pm_info[cnt].speed, speed, port_type));
                    } else {
					   PHYMOD_IF_ERR_RETURN(
    					    _plp_aperta_pm_info_speed_set(phy, lane_index, _plp_aperta_pm_info[cnt].sys_speed, speed, port_type));
                    }
                }
            }
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Invalid speed\n"));
    return PHYMOD_E_INTERNAL;
}

int plp_aperta_pm_info_speed_get(const plp_aperta_phymod_phy_access_t *phy, int *speed, int *configured_lane)
{
    unsigned short cnt = 0, lane_index = 0;
    int data = 0;
    for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy->access.addr) {
            for (lane_index = 0; lane_index < APERTA_PM_NUM_LANES ; lane_index++) {
                if (phy->access.lane_mask & (1 << lane_index)) {
                    if (phy->port_loc == phymodPortLocLine) {
                        data = _plp_aperta_pm_info[cnt].speed[lane_index]; 
                    } else {
                        data = _plp_aperta_pm_info[cnt].sys_speed[lane_index]; 
                    }
                    *speed = (data & 0xFFFFFF);
                    if (configured_lane) {
                        *configured_lane = (data >> 24) & 0xFF;
                        if (*configured_lane == 0x0) {
                            if (phy->access.lane_mask & 0xF) {
                            *configured_lane = 0xF;
                            } else {
                                *configured_lane = 0xF0;
                            }
                        }
                    }
                    return PHYMOD_E_NONE;
                }
            }
        }
    }
    *speed = 100000;
    PHYMOD_DEBUG_ERROR(("PM info speed get is not correct\n"));
    return PHYMOD_E_CONFIG;
}


int plp_aperta_portmod_pm_info_get(int unit, int port, pm_info_t* pm_info)
{
    int phy_id = (port & 0xffff00) >> 8;
    PHYMOD_IF_ERR_RETURN(plp_aperta_get_pm_info(phy_id, *pm_info));
    return PHYMOD_E_NONE;
}

int plp_aperta_portmod_port_pm_type_get(int unit, int port, portmod_dispatch_type_t* type)
{
    struct pm_info_s pm_info_1;
    pm_info_t pm_info;

    pm_info = &pm_info_1;

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_pm_info_get(unit, port, &pm_info));
    *type = pm_info_1.type;

    return PHYMOD_E_NONE;
}

int plp_aperta_portmod_port_chain_phy_access_get(int unit, int port, pm_info_t pm_info, plp_aperta_phymod_phy_access_t* core_access_arr, int max_buf, int* nof_cores)
{
    int rv = 0;
    portmod_access_get_params_t params;
    int core_count, is_most_ext;

    params.phyn = 0;
    rv = __plp_aperta_portmod__dispatch__[pm_info->type]->f_portmod_port_phy_lane_access_get(unit, port, pm_info, &params, max_buf, &(core_access_arr[0]), &core_count, &is_most_ext);
    APERTA_UNUSED_VAR(core_count);
    APERTA_UNUSED_VAR(is_most_ext);

    *nof_cores = 1;

    return rv;
}

int plp_aperta_portmod_port_chain_core_access_get(int unit, int port, pm_info_t pm_info, plp_aperta_phymod_core_access_t* core_access_arr, int max_buf, int* nof_cores)
{
    int rv = 0;
    int core_count, is_most_ext, phyn = 0;

    rv = __plp_aperta_portmod__dispatch__[pm_info->type]->f_portmod_port_core_access_get(unit, port, pm_info, phyn, max_buf, &(core_access_arr[0]), &core_count, &is_most_ext);
    APERTA_UNUSED_VAR(core_count);
    APERTA_UNUSED_VAR(is_most_ext);

    *nof_cores = 1;

    return rv;
}

int plp_aperta_pm_init(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config, const plp_aperta_phymod_core_status_t* core_status)
{
    int unit = 0, lane = 0, index = 0;
    portmod_pm_create_info_t pm_create_info;
    portmod_pm_create_info_internal_t pm_internal_info;
    pm_info_t init_pm_info;
    struct pm_info_s init_pm_info_1;
    portmod_pm_vco_setting_t vco_select;
    portmod_speed_config_t speed_config_list;
    uint32_t reg_data = 0;
    BCMI_APERTA_D_CTRL_SWGPREG0Er_t swgpreg_e;
    plp_aperta_phymod_phy_access_t phy_access;
    aperta_fw_init_t *fw_init_param = (aperta_fw_init_t*)(init_config->interface.device_aux_modes);


    PHYMOD_MEMSET(&init_pm_info_1, 0, sizeof(init_pm_info_1));
    PHYMOD_MEMSET(&swgpreg_e, 0, sizeof(BCMI_APERTA_D_CTRL_SWGPREG0Er_t));
    init_pm_info = &init_pm_info_1;
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_pm_create_info_t_init(unit, &pm_create_info));
    pm_create_info.phys = 1;
#ifdef PHYMOD_APERTA_SUPPORT
    pm_create_info.type = portmodDispatchTypePm8x50;
#endif
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_phy_access_t_init(&pm_create_info.pm_specific_info.pm8x50.access));

    /* Moving access, MDIO address, bus etc*/
    PHYMOD_MEMCPY(&pm_create_info.pm_specific_info.pm8x50.access, core, sizeof(plp_aperta_phymod_phy_access_t));

    pm_create_info.pm_specific_info.pm8x50.fw_load_method = init_config->firmware_load_method ;
    pm_create_info.pm_specific_info.pm8x50.fw_load_method &= 0xff;

    /* Use default external loader if NULL */
    pm_create_info.pm_specific_info.pm8x50.external_fw_loader = NULL;

    /* Polarity */
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_polarity_t_init (&(pm_create_info.pm_specific_info.pm8x50.polarity)));

    /* Lane map */
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_lane_map_t_init(&(pm_create_info.pm_specific_info.pm8x50.lane_map)));


    pm_create_info.pm_specific_info.pm8x50.lane_map.num_of_lanes = 8;
    pm_create_info.pm_specific_info.pm8x50.core_num = 1;

    /* Assiginig default lane map i.e. Tx/Rx lane 0 to 0, 1 to 1, etc*/
    for (lane=0 ; lane< APERTA_PM_NUM_LANES; lane++) {
        pm_create_info.pm_specific_info.pm8x50.lane_map.lane_map_tx[lane] =
             (0x76543210 >> (lane * 4)) & 0xF;
        pm_create_info.pm_specific_info.pm8x50.lane_map.lane_map_rx[lane] =
              (0x76543210 >> (lane * 4)) & 0xF;
    }
    vco_select.speed_config_list = &speed_config_list;
    vco_select.num_speeds = 1;
    vco_select.speed_config_list->num_lane = 4;
    vco_select.speed_config_list->fec = 1; /* NO fec*/

    if ((fw_init_param != NULL) && (fw_init_param->pll1_vco_rate == bcmplpapertaVco26p562G)) { 
        vco_select.speed_config_list->speed = 400000;
        vco_select.speed_config_list->num_lane = 8;
        vco_select.speed_config_list->fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN;
    } else if ((fw_init_param != NULL) && (fw_init_param->pll1_vco_rate == bcmplpapertaVco25p781G)) { 
        vco_select.speed_config_list->speed = 100000;
    } else if ((fw_init_param != NULL) && (fw_init_param->pll1_vco_rate == bcmplpapertaVco20p625G)) { 
        vco_select.speed_config_list->speed = 40000;
    }  else {
        vco_select.speed_config_list->speed = 400000;
        vco_select.speed_config_list->num_lane = 8;
        vco_select.speed_config_list->fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN;
    }
    if (fw_init_param != NULL) {
        swgpreg_e.v[0] = (fw_init_param->pll1_vco_rate << 8);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_CTRL_SWGPREG0Er(&core->access, swgpreg_e));
    }
 
    for (index = 0; index < APERTA_MAX_PM_INFO; index++) {
        if(APERTA_UNINIT_PHYS == _plp_aperta_pm_info[index].phy_id) {
            for (lane=0 ; lane < APERTA_PM_NUM_LANES; lane++) {
                _plp_aperta_pm_info[index].speed[lane] = vco_select.speed_config_list->speed;
                _plp_aperta_pm_info[index].sys_speed[lane] = vco_select.speed_config_list->speed;
            }
        }
    }

    vco_select.speed_config_list->link_training = 0;
    vco_select.speed_config_list->lane_config = 4;
    vco_select.tvco = portmodVCOInvalid;
    vco_select.ovco = portmodVCOInvalid;
    vco_select.is_tvco_new = 0;
    vco_select.is_ovco_new = 0;

#ifdef PHYMOD_APERTA_SUPPORT
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_portmod_pm_vcos_get(unit, portmodDispatchTypePm8x50, &vco_select));
#endif    
    /* Defaults to 156MHZ*/
    pm_create_info.pm_specific_info.pm8x50.ref_clk = phymodRefClk156Mhz;

    PHYMOD_MEMCPY(&pm_internal_info, &pm_create_info, sizeof(portmod_pm_create_info_internal_t));

    /* Add PM to PortMod */
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_pm_init(unit, &pm_internal_info, 0, init_pm_info));

#ifdef PHYMOD_APERTA_SUPPORT
    PM_8x50_INFO(init_pm_info)->int_core_access.type = phymodDispatchTypeAperta;
    PM_8x50_INFO(init_pm_info)->int_core_access.access.tvco_pll_index = APERTA_TVCO_PLL_INDEX;
    PM_8x50_INFO(init_pm_info)->fw_load_method = init_config->firmware_load_method ;
    PM_8x50_INFO(init_pm_info)->external_fw_loader = init_config->firmware_loader ;
    PHYMOD_MEMCPY(&PM_8x50_INFO(init_pm_info)->int_phy_access, &PM_8x50_INFO(init_pm_info)->int_core_access,
        sizeof(plp_aperta_phymod_phy_access_t));
    PM_8x50_INFO(init_pm_info)->tvco = vco_select.tvco;
    PM_8x50_INFO(init_pm_info)->ovco = vco_select.ovco;
    PM_8x50_INFO(init_pm_info)->sys_tvco = vco_select.tvco;
    PM_8x50_INFO(init_pm_info)->sys_ovco = vco_select.ovco;

#endif
    /* Setting tx_driver_supply to 1.25 or 1 V*/
    if (fw_init_param != NULL) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_direct_reg_read(&PM_8x50_INFO(init_pm_info)->int_phy_access, 0x8b51, &reg_data));
        reg_data &= ~(1 << 6);
        reg_data |= (fw_init_param->tx_drv_supply? 1: 0) << 6;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_direct_reg_write(&PM_8x50_INFO(init_pm_info)->int_phy_access, 0x8b51, reg_data));
    }
    PHYMOD_MEMCPY(&phy_access, core, sizeof(plp_aperta_phymod_phy_access_t));
    phy_access.port_loc = phymodPortLocLine;
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_update_vco(&phy_access, 1, vco_select.tvco)); 
    phy_access.port_loc = phymodPortLocSys;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_update_vco(&phy_access, 1, vco_select.tvco)); 

    PHYMOD_ACC_F_CLAUSE45_SET(&(PM_8x50_INFO(init_pm_info)->int_core_access.access));
    PHYMOD_IF_ERR_RETURN(plp_aperta_add_pm_info(core->access.addr, init_pm_info));

    return PHYMOD_E_NONE;
}

int plp_aperta_core_add(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_phy_init_config_t* init_config,
                    const plp_aperta_phymod_core_status_t* core_status)
{
    int port = 0; /* This has info abt PHY and port of a PM */
    portmod_port_add_info_t add_info;
    int unit = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_portmod_port_add_info_t_init(unit, &add_info));

    /* Clear the load verify flag to speed up the boot time */
    /* Get Pass flag*/
    add_info.flags= init_config->ext_phy_tx_params_user_flag[0] ;
    /* Get Load method*/
    add_info.ilkn_oob_cal_len_tx = init_config->ext_phy_tx_params_user_flag[1]  ;

    /* Initialize both interface_t config and init config */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_portmod_port_init_config_t_init(unit, &add_info.init_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_portmod_port_interface_config_t_init(unit, &add_info.interface_config));

    /* Cann Add init config on demand*/
    add_info.autoneg_en = init_config->an_en ;
    add_info.link_training_en = init_config->cl72_en;

    add_info.interface_config.interface_t = init_config->interface.interface_type;
    add_info.interface_config.line_interface = init_config->interface.interface_type;
    add_info.interface_config.serdes_interface = init_config->interface.interface_type;
    add_info.interface_config.interface_modes = init_config->interface.interface_modes;
    add_info.interface_config.flags = 0;
    add_info.interface_config.port_refclk_int = init_config->interface.ref_clock;
    add_info.interface_config.port_op_mode = init_config->op_mode;
    add_info.interface_config.speed = init_config->interface.data_rate;
    add_info.interface_config.max_speed = init_config->interface.data_rate;
    add_info.interface_config.encap_mode = _SHR_PORT_ENCAP_IEEE;

    /* it is not used in TSBH, so setting it to 0*/
    add_info.interface_config.pll_divider_req = 0;
    if (add_info.interface_config.speed == 0) {
        add_info.interface_config.speed = APERTA_SPEED_100G;
        add_info.interface_config.interface_t = phymodInterfaceCAUI;
        add_info.interface_config.port_num_lanes = 4;
    }
    if (init_config->interface.data_rate == APERTA_SPEED_100G) {
        add_info.interface_config.port_num_lanes = 4;
    } else if (init_config->interface.data_rate == APERTA_SPEED_40G) {
        add_info.interface_config.port_num_lanes = 4;
    } else if (init_config->interface.data_rate == APERTA_SPEED_50G) {
        add_info.interface_config.port_num_lanes = 2;
    } else if (init_config->interface.data_rate == APERTA_SPEED_10G) {
        add_info.interface_config.port_num_lanes = 1;
    }
    /* Setting init flags*/
     add_info.interface_config.flags |= (1 << 31);
    /* Considering only For one port of PM*/
    port = APERTA_PORT_CONSTRUCTION(core->access.addr, 0);
    APERTA_UPDATE_LM(core->access.addr, 0xFF);
    add_info.phy_op_datapath = phymodDatapathNormal;
     /* Initialize port 0 of a core*/
    PHYMOD_IF_ERR_RETURN(
         plp_aperta_portmod_core_add(unit, port, &add_info));

    return PHYMOD_E_NONE;
}
int plp_aperta_port_attach(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_init_config_t* init_config)
{
    int port = 0; /* This has info abt PHY and port of a PM */
    portmod_port_add_info_t add_info;
    plp_aperta_phymod_phy_access_t phy_copy;
    int unit = 0;

    PHYMOD_MEMSET(&add_info, 0, sizeof(portmod_port_add_info_t));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_portmod_port_add_info_t_init(unit, &add_info));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));


    /* Initialize both interface_t config and init config */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_portmod_port_init_config_t_init(unit, &add_info.init_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta_portmod_port_interface_config_t_init(unit, &add_info.interface_config));

    /* Cann Add init config on demand*/
    add_info.autoneg_en = init_config->an_en ;
    add_info.link_training_en = init_config->cl72_en;

    add_info.interface_config.flags = 0;
    add_info.interface_config.port_refclk_int = init_config->interface.ref_clock;
    add_info.interface_config.port_op_mode = init_config->op_mode;
    add_info.interface_config.speed = init_config->interface.data_rate;
    add_info.interface_config.max_speed = init_config->interface.data_rate;
    add_info.interface_config.encap_mode = _SHR_PORT_ENCAP_IEEE;

    /* it is not used in TSBH, so setting it to 0*/
    add_info.interface_config.pll_divider_req = 0;
    if (add_info.interface_config.speed == 0) {
        PHYMOD_DEBUG_ERROR(("Init speed cannot be 0 \n"));
        return PHYMOD_E_CONFIG;
    }
    if (init_config->interface.data_rate == APERTA_SPEED_100G) {
        add_info.interface_config.port_num_lanes = 4;
    } else if (init_config->interface.data_rate == APERTA_SPEED_40G) {
        add_info.interface_config.port_num_lanes = 4;
    } else if (init_config->interface.data_rate == APERTA_SPEED_50G) {
        add_info.interface_config.port_num_lanes = 2;
    } else if (init_config->interface.data_rate == APERTA_SPEED_10G) {
        add_info.interface_config.port_num_lanes = 1;
    } else if (init_config->interface.data_rate == APERTA_SPEED_400G) {
        add_info.interface_config.port_num_lanes = 8;
    }

    add_info.speed_config.speed =  add_info.interface_config.speed;
    add_info.speed_config.num_lane = add_info.interface_config.port_num_lanes;
    if (init_config->interface.data_rate == 400000) {
        add_info.speed_config.fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN ;
    } else {
        add_info.speed_config.fec = PORTMOD_PORT_PHY_FEC_NONE;
    }
    add_info.speed_config.link_training = 0;
    add_info.speed_config.lane_config = -1 ;
    /* Setting init flags*/
    add_info.interface_config.flags |= (1 << 31);
    add_info.phy_op_datapath = phymodDatapathNormal;

    if (init_config->interface.data_rate == 400000) {
        /* Considering port1 of PM*/
        port = APERTA_PORT_CONSTRUCTION(phy->access.addr, 0);
        APERTA_UPDATE_LM(phy->access.addr, 0xFF);
        PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_attach(unit, port, &add_info));
    } else {
        /* Considering port1 of PM*/
        port = APERTA_PORT_CONSTRUCTION(phy->access.addr, 0);
        APERTA_UPDATE_LM(phy->access.addr, 0xF);

        /* Initialize port 0 of a core*/
        PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_attach(unit, port, &add_info));

        /* Initialize port 1 of a core*/
        port = APERTA_PORT_CONSTRUCTION(phy->access.addr, 0);
        APERTA_UPDATE_LM(phy->access.addr, 0xF0);
        PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_attach(unit, port, &add_info));
    }

    if (init_config->ext_phy_tx_params_user_flag[0] == 0) {
        /*PHYMOD_IF_ERR_RETURN(plp_aperta_pm_tx_rx_enable(phy, 1));*/

    }
    if (init_config->interface.data_rate == 400000) {
        phy_copy.access.lane_mask = 0xFF;
        PHYMOD_IF_ERR_RETURN(
               plp_aperta_port_speed_set(&phy_copy, APERTA_SPEED_400G, APERTA_LD_50G));
    } else {
        if (init_config->interface.data_rate == 100000) {
            phy_copy.access.lane_mask = 0xF;
            PHYMOD_IF_ERR_RETURN(
                   plp_aperta_port_speed_set(&phy_copy, APERTA_SPEED_100G, APERTA_LD_25G));
            phy_copy.access.lane_mask = 0xF0;
            PHYMOD_IF_ERR_RETURN(
                   plp_aperta_port_speed_set(&phy_copy, APERTA_SPEED_100G, APERTA_LD_25G));
        } else {
            phy_copy.access.lane_mask = 0xF;
            PHYMOD_IF_ERR_RETURN(
                   plp_aperta_port_speed_set(&phy_copy, APERTA_SPEED_40G, APERTA_LD_10G));
            phy_copy.access.lane_mask = 0xF0;
            PHYMOD_IF_ERR_RETURN(
                   plp_aperta_port_speed_set(&phy_copy, APERTA_SPEED_40G, APERTA_LD_10G));
        }
    }
    add_info.interface_config.flags &= ~(1 << 31);

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_tx_rx_enable(const plp_aperta_phymod_phy_access_t* phy, int enable, int single_port, int failover)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t tx_ctrl;
    int side = 0, count = 0 , prev_state;
    uint32_t temp = 0, system_lane_map;
    plp_aperta_phymod_phy_access_t temp_access;
    plp_aperta_phymod_phy_access_t phy1;
    PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_MEMCPY(&phy1, phy, sizeof(plp_aperta_phymod_phy_access_t));

    PHYMOD_IF_ERR_RETURN(
                plp_aperta_direct_reg_read(phy, APERTA_SYSTEM_SIDE_NO_OF_LANES, &system_lane_map ));
    system_lane_map >>= 8;
    if (!enable) {
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
           /*if (side != phy->port_loc) {
               continue;
           }*/
           temp_access.port_loc = side;
            if (single_port) {
                for (count = 0; count < APERTA_PM_NUM_LANES; count ++) {
                    if ((phy->access.lane_mask & (1 << count))) {
                        temp_access.access.lane_mask = 1 << count;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_reg32_read(&temp_access, 0x1400010b, &temp));
                        if (temp & 0x2) {
                           temp &= ~0x02;
                            PHYMOD_IF_ERR_RETURN(
                                plp_aperta_reg32_write(&temp_access, 0x1400010b, temp));
                        }
                        prev_state = side;
                        side = (prev_state == phymodPortLocLine) ? phymodPortLocSys : phymodPortLocLine;
                        temp_access.port_loc = side;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_reg32_read(&temp_access, 0x1500010d, &temp));
                        if ((temp & 0x4) != 0x4) {
                           temp |= 0x4 ;
                            PHYMOD_IF_ERR_RETURN(
                                plp_aperta_reg32_write(&temp_access, 0x1500010d, temp));
                        }
 
                        side = prev_state ;
                        temp_access.port_loc = side;
                    }
                }
            } else {
                for (count = 0; count < APERTA_PM_NUM_LANES; count ++) {
                    if ((phy->access.lane_mask & (1 << count))) {
                        temp_access.access.lane_mask = 1 << count;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_reg32_read(&temp_access, 0x1400010b, &temp));
                        if (temp & 0x2) {
                            temp &= ~0x02;
                            PHYMOD_IF_ERR_RETURN(
                                plp_aperta_reg32_write(&temp_access, 0x1400010b, temp));
                        }
                        prev_state = side;
                        side = (prev_state == phymodPortLocLine) ? phymodPortLocSys : phymodPortLocLine;
                        temp_access.port_loc = side;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_reg32_read(&temp_access, 0x1500010d, &temp));
                        if ((temp & 0x4) != 0x4) {
                            temp |= 0x4 ;
                            PHYMOD_IF_ERR_RETURN(
                                plp_aperta_reg32_write(&temp_access, 0x1500010d, temp));
                        }
                        side = prev_state ;
                        temp_access.port_loc = side;
                    }
                }
            }
        }
    } else {
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = side;
            if (side == phymodPortLocSys) {
                if (!failover) {
                    phy1.access.lane_mask = system_lane_map;
                }
                if (failover && phy->port_loc == phymodPortLocLine) {
                    continue;
                }

            } else {
                if (phy->port_loc == phymodPortLocSys) {
                    continue;
                }
                phy1.access.lane_mask = phy->access.lane_mask;
            }
            for (count = 0; count < APERTA_PM_NUM_LANES; count ++) {
                if ((phy1.access.lane_mask & (1 << count))) {
                    temp_access.access.lane_mask = 1 << count;
                    PHYMOD_IF_ERR_RETURN(
                         BCMI_APERTA_CDMAC_READ_CDMAC_TX_CTRLr(&temp_access, &tx_ctrl));
                    BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_DISCARDf_SET(tx_ctrl, 0);
                    PHYMOD_IF_ERR_RETURN(
                         BCMI_APERTA_CDMAC_WRITE_CDMAC_TX_CTRLr(&temp_access, tx_ctrl));
                }
            }
        }
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = side;
            if (side == phymodPortLocSys) {
                if (!failover) {
                    phy1.access.lane_mask = system_lane_map;
                }
                if (failover && phy->port_loc == phymodPortLocLine) {
                    continue;
                }

            } else {
                if (phy->port_loc == phymodPortLocSys) {
                    continue;
                }
                phy1.access.lane_mask = phy->access.lane_mask;
            }
            /* Clear Softreset*/
            for (count = 0; count < APERTA_PM_NUM_LANES; count ++) {
                if ((phy1.access.lane_mask & (1 << count))) {
                    temp_access.access.lane_mask = 1 << count;

                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_CDMAC_READ_CDMAC_CTRLr(&temp_access, &cdmac_ctrl));
                    BCMI_APERTA_CDMAC_CDMAC_CTRLr_SOFT_RESETf_SET(cdmac_ctrl, 0);
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_CDMAC_WRITE_CDMAC_CTRLr(&temp_access, cdmac_ctrl));
                }
            }
        }
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = side;
            if (side == phymodPortLocSys) {
                if (!failover) {
                    phy1.access.lane_mask = system_lane_map;
                }
                if (failover && phy->port_loc == phymodPortLocLine) {
                    continue;
                }

            } else {
                if (phy->port_loc == phymodPortLocSys) {
                    continue;
                }
                phy1.access.lane_mask = phy->access.lane_mask;
            }
            /* Enable Tx*/
            for (count = 0; count < APERTA_PM_NUM_LANES; count ++) {
                if ((phy1.access.lane_mask & (1 << count))) {
                    temp_access.access.lane_mask = 1 << count;
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_CDMAC_READ_CDMAC_CTRLr(&temp_access, &cdmac_ctrl));
                    BCMI_APERTA_CDMAC_CDMAC_CTRLr_TX_ENf_SET(cdmac_ctrl, 1);
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_CDMAC_WRITE_CDMAC_CTRLr(&temp_access, cdmac_ctrl));
                }
            }
        }
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = side;
            if (side == phymodPortLocSys) {
                if (!failover) {
                   phy1.access.lane_mask = system_lane_map;
                }
                if (failover && phy->port_loc == phymodPortLocLine) {
                    continue;
                }

            } else {
                if (phy->port_loc == phymodPortLocSys) {
                    continue;
                }
                phy1.access.lane_mask = phy->access.lane_mask;
            }
            /* Enable Rx*/
            for (count = 0; count < APERTA_PM_NUM_LANES; count ++) {
                if ((phy1.access.lane_mask & (1 << count))) {
                    temp_access.access.lane_mask = 1 << count;
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_CDMAC_READ_CDMAC_CTRLr(&temp_access, &cdmac_ctrl));
                    BCMI_APERTA_CDMAC_CDMAC_CTRLr_RX_ENf_SET(cdmac_ctrl, 1);
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_CDMAC_WRITE_CDMAC_CTRLr(&temp_access, cdmac_ctrl));
                }
            }
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_tx_rx_enable_post(const plp_aperta_phymod_phy_access_t* phy, int enable, int single_port, uint16_t fo_side_lm)
{
    int side = 0, count = 0;
    uint32_t temp = 0, lane_index = 0 ;
    plp_aperta_phymod_phy_access_t temp_access;
    PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta_phymod_phy_access_t));

    if (!enable) {
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
           temp_access.port_loc = side;
            if (single_port) {
                for (count = 0; count < APERTA_PM_NUM_LANES; count ++) {
                    if ((phy->access.lane_mask & (1 << count))) {
                        temp_access.access.lane_mask = 1 << count;

                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_reg32_read(&temp_access, 0x1400010b, &temp));
                        temp &= ~0x01;
                        PHYMOD_IF_ERR_RETURN(
                           plp_aperta_reg32_write(&temp_access, 0x1400010b, temp));
                        temp |= 0x40;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_reg32_write(&temp_access, 0x1400010b, temp));
                    }
                }
            } else {
                for (count = 0; count < APERTA_PM_NUM_LANES; count ++) {
                    if ((phy->access.lane_mask & (1 << count))) {
                        temp_access.access.lane_mask = 1 << count;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_reg32_read(&temp_access, 0x1400010b, &temp));
                        temp &= ~0x01;
                        PHYMOD_IF_ERR_RETURN(
                           plp_aperta_reg32_write(&temp_access, 0x1400010b, temp));
                        temp |= 0x40;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_reg32_write(&temp_access, 0x1400010b, temp));
                        if ((fo_side_lm & 0xFF) && (side == ((((fo_side_lm >> 8) & 0xF) == phymodPortLocSys) ? phymodPortLocSys :
                               phymodPortLocLine))) {
                            for (lane_index = 0 ; lane_index <8; lane_index ++) {
                                 if (fo_side_lm & ( 1 << lane_index)) {
                                     temp_access.access.lane_mask = 1 << lane_index;
                                     PHYMOD_IF_ERR_RETURN(
                                         plp_aperta_reg32_read(&temp_access, 0x1400010b, &temp));
                                     temp &= ~0x01;
                                     PHYMOD_IF_ERR_RETURN(
                                        plp_aperta_reg32_write(&temp_access, 0x1400010b, temp));
                                     temp |= 0x40;
                                     PHYMOD_IF_ERR_RETURN(
                                         plp_aperta_reg32_write(&temp_access, 0x1400010b, temp));
                                 }
                            }
                            fo_side_lm &= ~(0xFF);
                        }
                    }
                }
            }
        }
    }
    return PHYMOD_E_NONE;
}
#include<blackhawk/tier1/blackhawk_tsc_interface.h>
int
_plp_aperta_get_lane_config_word(const plp_aperta_phymod_phy_inf_config_t *config,
        int *lane_config_word) {
    struct blackhawk_tsc_uc_lane_config_st st;
    aperta_device_aux_modes_t *aux_mode = (aperta_device_aux_modes_t *) config->device_aux_modes;

    PHYMOD_MEMSET(&st, 0, sizeof(struct blackhawk_tsc_uc_lane_config_st));
    if ((config->interface_type == phymodInterfaceSFI)||
        (config->interface_type == phymodInterfaceSR4)||
        (config->interface_type == phymodInterfaceLR4)||
        (config->interface_type == phymodInterfaceLR) ||
        (config->interface_type == phymodInterfaceER4)||
        (config->interface_type == phymodInterfaceER) ||
        (config->interface_type == phymodInterfaceSR) ||
        (config->interface_type == phymodInterfaceLR2)||
        (config->interface_type == phymodInterfaceER2)||
        (config->interface_type == phymodInterfaceXLPPI)||
        (config->interface_type == phymodInterfaceSR2)) {
        st.field.media_type = phymodFirmwareMediaTypeOptics;
    } else if ((config->interface_type == phymodInterfaceCR) ||
               (config->interface_type == phymodInterfaceCR2)||
               (config->interface_type == phymodInterfaceCR4)) {
        st.field.media_type = phymodFirmwareMediaTypeCopperCable;
    } else {
        st.field.media_type = phymodFirmwareMediaTypePcbTraceBackPlane;
    }

    if (((aux_mode->lane_data_rate == bcmplpApertaLaneDataRate_51P5625G)||
        (aux_mode->lane_data_rate ==  bcmplpApertaLaneDataRate_53P125G)||
        (aux_mode->lane_data_rate ==  bcmplpApertaLaneDataRate_56P25G)) &&
        (aux_mode->modulation_mode == bcmplpApertaModulationPAM4)){
        st.field.force_pam4_mode = 1;
        st.field.dfe_on = 1;
        st.field.force_ns = 1;
        if (st.field.media_type == phymodFirmwareMediaTypeOptics) {
            PHYMOD_DEBUG_ERROR(("Optical interface is not supported in PAM4\n"));
            return PHYMOD_E_PARAM;
        }
    } else {
        st.field.force_nrz_mode = 1;
        if ((config->interface_type == phymodInterfaceSR)  ||
            (config->interface_type == phymodInterfaceSR4) ||
            (config->interface_type == phymodInterfaceER)  ||
            (config->interface_type == phymodInterfaceER4) ||
            (config->interface_type == phymodInterfaceLR)  ||
            (config->interface_type == phymodInterfaceLR4) ||
            (config->interface_type == phymodInterfaceKR)  ||
            (config->interface_type == phymodInterfaceKR4) ||
            (config->interface_type == phymodInterfaceCR)  ||
            (config->interface_type == phymodInterfaceCR2)  ||
            (config->interface_type == phymodInterfaceKR2)  ||
            (config->interface_type == phymodInterfaceCR4)) {
            st.field.dfe_on = 1;
        }
        if (config->interface_type == phymodInterfaceCAUI4_C2C || 
            config->interface_type == phymodInterfaceCAUI4_C2M ||
            config->interface_type == phymodInterfaceCEIMR ||
            config->interface_type == phymodInterfaceVSR) {
            st.field.dfe_on = 1;
            st.field.dfe_lp_mode = 1;
        }
        if (config->interface_type == phymodInterfaceCEILR) {
            st.field.dfe_on = 1;
            st.field.dfe_lp_mode = 0;
        }

    }
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_blackhawk_tsc_INTERNAL_update_uc_lane_config_word(&st));
    *lane_config_word = st.word;

    return PHYMOD_E_NONE;
}

int _plp_aperta_fill_port_cfg(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_inf_config_t* config, aperta_config_port_t *fw_port_config)
{
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0, line_no_lanes = 0, lane = 0, port = 0;
    uint32_t sys_no_lanes = 0, rev_id = 0;
    aperta_device_aux_modes_t auxmode = *(aperta_device_aux_modes_t*)config->device_aux_modes;
    BCMI_APERTA_D_CTRL_SWGPREG19r_t swgpreg_19;

    APERTA_GET_PORT_FROM_LM_SP(config->data_rate, auxmode.lane_data_rate, phy->access.lane_mask,port, lane);
    APERTA_UNUSED_VAR(lane);
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_direct_reg_read(phy, APERTA_SYSTEM_SIDE_NO_OF_LANES, &sys_no_lanes));

    line_no_lanes = plp_aperta_count_no_bits(phy->access.lane_mask);
    fw_port_config->SPMPortID = (sys_no_lanes & 0xF0) >> 4;
    fw_port_config->LPMPortID = fw_port_config->PortNum = port;
    fw_port_config->PortType = ((sys_no_lanes & 0xF) == line_no_lanes) ? APERTA_PORT_TYPE_REPEATER :
                           ((sys_no_lanes & 0xF) > line_no_lanes) ? APERTA_PORT_TYPE_GEARBOX :
                            APERTA_PORT_TYPE_R_GEARBOX;
    fw_port_config->PortMode =  0;    
    if (config->data_rate == APERTA_SPEED_10G) {
        fw_port_config->PortSpeed = APERTA_FW_SP_10G;
    } else if (config->data_rate == APERTA_SPEED_25G) {
        fw_port_config->PortSpeed = APERTA_FW_SP_25G;
    } else if (config->data_rate == APERTA_SPEED_40G) {
        fw_port_config->PortSpeed = APERTA_FW_SP_40G;
    } else if (config->data_rate == APERTA_SPEED_50G &&
            auxmode.modulation_mode == bcmplpApertaModulationNRZ) {
        fw_port_config->PortSpeed = APERTA_FW_SP_50G_NRZ;
    } else if (config->data_rate == APERTA_SPEED_50G &&
            auxmode.modulation_mode == bcmplpApertaModulationPAM4) {
        fw_port_config->PortSpeed = APERTA_FW_SP_50G_PAM4;
    } else if (config->data_rate == APERTA_SPEED_100G &&
            auxmode.modulation_mode == bcmplpApertaModulationNRZ) {
        fw_port_config->PortSpeed = APERTA_FW_SP_100G_NRZ;
    } else if (config->data_rate == APERTA_SPEED_100G &&
            auxmode.modulation_mode == bcmplpApertaModulationPAM4) {
        fw_port_config->PortSpeed = APERTA_FW_SP_100G_PAM4;
    } else if (config->data_rate == APERTA_SPEED_200G &&
            auxmode.modulation_mode == bcmplpApertaModulationPAM4) {
        fw_port_config->PortSpeed = APERTA_FW_SP_200G_PAM4;
    } else if (config->data_rate == APERTA_SPEED_400G &&
            auxmode.modulation_mode == bcmplpApertaModulationPAM4) {
        fw_port_config->PortSpeed = APERTA_FW_SP_400G_PAM4;
    } else {
        PHYMOD_DEBUG_ERROR(("Incorrect datarate\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_direct_reg_read(phy, addr[port], &data));
    if (data & (1 << APERTA_S_F_SHIFT)) {
        fw_port_config->PortOptions |= 1;
    }
    if (data & (1 << APERTA_FLOW_CTRL_SHIFT)) {
        fw_port_config->PortOptions |= 2;
    }
    if (data & (1 << APERTA_FAULT_SHIFT)) {
        fw_port_config->PortOptions |= 4;
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));
    if (rev_id == APERTA_REV_B0) {
        /* Timestamp config bit 3 needs to be stored in bit 3 
         * of port option and Bit 0-2 will be updated in bit 4-6 of port options*/
        fw_port_config->PortOptions |= (((auxmode.ts_config >> 3) & 1)  << 3);
        fw_port_config->PortOptions |= (auxmode.ts_config & 7)  << 4;
        fw_port_config->EgrptpFixedLatency =  auxmode.egr_ptp_fixed_latency;
    } else {
        fw_port_config->PortOptions |= (auxmode.ts_config) << 4;
    }
    if (auxmode.fixed_latency_config.enable) {
        fw_port_config->PortOptions |= 0x80;
        fw_port_config->IngFixedLatency = auxmode.fixed_latency_config.igr_dp_ck_cycles;
        fw_port_config->EgrFixedLatency = auxmode.fixed_latency_config.egr_dp_ck_cycles;
    }
    if ((fw_port_config->PortOptions & 0x8) && !(fw_port_config->PortOptions & 0x80)) {
        PHYMOD_DEBUG_ERROR(("Port cannot be configured without enabling fixed latency \n"));
        return PHYMOD_E_NONE;
    }
    /* System side Failover*/
    PHYMOD_IF_ERR_RETURN(
           BCMI_APERTA_D_READ_CTRL_SWGPREG19r(&phy->access, &swgpreg_19));
    if (swgpreg_19.v[0] & 0xFF) {
        /*Enable system side F0*/
        if ((swgpreg_19.v[0] >> 8) & 1) { /* Before MACSEC*/
            fw_port_config->FOOptions = 0x4;
        } else { /* After Macsec*/
            fw_port_config->FOOptions = 0x0;
        }
        fw_port_config->PortMode =  1;
        APERTA_GET_PORT_FROM_LM((swgpreg_19.v[0] & 0xFF), fw_port_config->FOPortNum);
        fw_port_config->FOPortID = fw_port_config->FOPortNum;
        if (auxmode.failover_config.lane_map != 0) {
            PHYMOD_DEBUG_ERROR(("Failover cannot be configured for both line and System side\n"));
            return PHYMOD_E_PARAM;
        }
    }
    /* Line side Failover*/
    if (auxmode.failover_config.lane_map != 0) {
        /*Enable Line side side FO*/
        fw_port_config->FOOptions = 0x1;
        if (auxmode.failover_config.mux_location & 1) { /* Before MACSEC*/
            fw_port_config->FOOptions |= 0x4;
        } else {   /* After macsec*/
            fw_port_config->FOOptions = 0x1;
        }
        fw_port_config->PortMode =  1;
        APERTA_GET_PORT_FROM_LM(auxmode.failover_config.lane_map, fw_port_config->FOPortNum);
        fw_port_config->FOPortID = fw_port_config->FOPortNum;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_reset_pcs(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t* config)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    int port_index = 0;
    uint8_t enabled_port_list[APERTA_MAX_PORT];
    int possible_port_list = 0, max_ports = 0, temp;
    int line_prev_speed = 0, line_prev_lanemap = 0, line_prev_port = 0, line_prev_no_lanes = 0;
    int prev_speed = 0, prev_lanemap = 0, prev_port = 0, prev_no_lanes = 0, unused = 0;
    int side = 0, fo_lane_map = 0;
    aperta_device_aux_modes_t aux_mode = *(aperta_device_aux_modes_t*)config->device_aux_modes;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));
   
    /* Get System side speed and port numbers*/
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_pm_info_speed_get(phy, &prev_speed, &prev_lanemap));

    prev_no_lanes = plp_aperta_count_no_bits(prev_lanemap);
    APERTA_GET_PORT_FROM_LM_SP(prev_speed, (prev_speed/prev_no_lanes), prev_lanemap, prev_port, unused);
    APERTA_UNUSED_VAR(unused);
    APERTA_UNUSED_VAR(prev_port);

    /* Get Line side speed and port numbers*/
    phy_copy.port_loc = phymodPortLocLine;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_info_speed_get(&phy_copy, &line_prev_speed, &line_prev_lanemap));

    line_prev_no_lanes = plp_aperta_count_no_bits(line_prev_lanemap);
    APERTA_GET_PORT_FROM_LM_SP(line_prev_speed, (line_prev_speed/line_prev_no_lanes), line_prev_lanemap, line_prev_port, unused);
    APERTA_UNUSED_VAR(unused);
    APERTA_UNUSED_VAR(line_prev_port);

    /* Revert it back to system for performing other operation*/
    phy_copy.port_loc = phymodPortLocSys;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_get_enabled_port(phy, enabled_port_list));

    if ((config->data_rate == APERTA_SPEED_400G) && enabled_port_list[0]) {
        phy_copy.access.lane_mask = 0xFF;
        APERTA_PCS_UPDATE(&phy_copy, 0xFF);
    } else {
        if (enabled_port_list[line_prev_port]) {
            /* Sys Lane map*/
            phy_copy.access.lane_mask = prev_lanemap;
            APERTA_PCS_UPDATE(&phy_copy, line_prev_lanemap);
        }
        APERTA_LM_TO_POSSIBLE_PORT_LIST(phy->access.lane_mask, possible_port_list, max_ports);
        /* Disable other port if it was already part of curent lane*/
        for (port_index = 0; port_index < max_ports; port_index++) {
            temp = (possible_port_list >> port_index*4) & 0xF;
            if (enabled_port_list[temp] && (temp != line_prev_port)) {
                phy_copy.port_loc = phymodPortLocSys;
                phy_copy.access.lane_mask = (1 << temp);
                line_prev_lanemap = (1 << temp);
                APERTA_PCS_UPDATE(&phy_copy, line_prev_lanemap);
            }
        }
    }
    if (aux_mode.port_type == bcmplpApertaPortTypeGearBox ||  
            (aux_mode.port_type == bcmplpApertaPortTypeReverseGearBox)) {
        int port_map = 0, gb_port = 0;
        for (gb_port = 0; gb_port < APERTA_MAX_GB_RGB_PORT; gb_port ++) {
            if ((aux_mode.port_type == plp_aperta_port_mode_sys_line_lane[gb_port][APERTA_GB_RGB_PORT_TYPE]) && 
                    (phy->access.lane_mask == plp_aperta_port_mode_sys_line_lane[gb_port][APERTA_GB_RGB_PORT_SYS_LANE])) {
                port_map = plp_aperta_port_mode_sys_line_lane[gb_port][APERTA_GB_RGB_PORT_SYS_LANE] ^ plp_aperta_port_mode_sys_line_lane[gb_port][APERTA_GB_RGB_PORT_LINE_LANE];
                APERTA_LM_TO_POSSIBLE_PORT_LIST(port_map, possible_port_list, max_ports);
                /* Disable other port if it was already part of curent lane*/
                for (port_index = 0; port_index < max_ports; port_index++) {
                    temp = (possible_port_list >> port_index*4) & 0xF;
                    if (enabled_port_list[temp] && (temp != line_prev_port)) {
                        phy_copy.port_loc = phymodPortLocSys;
                        phy_copy.access.lane_mask = (1 << temp);
                        line_prev_lanemap = (1 << temp);
                        APERTA_PCS_UPDATE(&phy_copy, line_prev_lanemap);
                    }
                }
            }
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_is_fo_enabled(const plp_aperta_phymod_phy_access_t *phy, int *side)
{
    int port_number = 0, fo_lane_map = 0, speed = 0, ldr = 0;
    aperta_config_port_t port_cfg ;

    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port_number);
    port_cfg.PortNum = port_number;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_send_fw_msg(phy, 0, NULL,
                APERTA_FW_MSG_CONFIG_PORT, &port_cfg, 1));
    if (port_cfg.PortMode == 1) {
        APERTA_FW_DR_USER_DR (port_cfg.PortSpeed, speed, ldr);
        APERTA_GET_LM_FROM_PORT(speed, ldr, port_cfg.FOPortNum, fo_lane_map);
        *side = ((port_cfg.FOOptions &1) ? phymodPortLocLine : phymodPortLocSys);
        /* FO lanemap*/
        return (fo_lane_map);
    }
    /* NoN Failover port*/
    return 0xFFFF;
}

int plp_aperta_pm_interface_config_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta_phymod_phy_inf_config_t* config)
{
    int port = 0, int_port = 0, unused = 0, fo_lane_map = 0, fo_side = 0;
    int prev_speed = 0, prev_lanemap = 0, prev_port = 0, prev_no_lanes = 0;
    portmod_speed_config_t speed_config;
    struct pm_info_s pm_info_temp;
    aperta_config_port_t fw_port_config ;
    portmod_pm_vco_setting_t vco_setting;
    portmod_vco_type_t vco[2];
    pm_info_t pm_info = &pm_info_temp;
    aperta_device_aux_modes_t auxmode = *(aperta_device_aux_modes_t*)config->device_aux_modes;
    BCMI_APERTA_D_CTRL_SWGPREG19r_t swgpreg_19;
    plp_aperta_phymod_phy_access_t fo_phy, *pfo_phy;
    uint16_t fo_side_lm = 0, side = 0;
    plp_aperta_phymod_phy_access_t temp_acc;
    plp_aperta_phymod_phy_power_t power, pwr_get[2];
    int fo_prev_port = 0;
    int pll1_vco = 0;
    pfo_phy = &fo_phy;
 
    PHYMOD_MEMCPY(pfo_phy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_MEMSET(&vco_setting, 0, sizeof(portmod_pm_vco_setting_t));
    PHYMOD_MEMSET(&fw_port_config, 0, sizeof(aperta_config_port_t));
    PHYMOD_MEMSET(&pm_info_temp, 0, sizeof(struct pm_info_s));
    PHYMOD_MEMSET(&speed_config, 0, sizeof(portmod_speed_config_t));
    PHYMOD_MEMSET(&speed_config, 0, sizeof(portmod_speed_config_t));
    PHYMOD_MEMSET(&swgpreg_19, 0, sizeof(BCMI_APERTA_D_CTRL_SWGPREG19r_t));
    PHYMOD_MEMCPY(&temp_acc, phy, sizeof(plp_aperta_phymod_phy_access_t));
    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);
    speed_config.speed = config->data_rate;
    speed_config.num_lane = plp_aperta_count_no_bits(phy->access.lane_mask);
    speed_config.link_training = 0;

    if (auxmode.fec_mode_sel == bcmplpApertaNoFEC) {
        speed_config.fec = PORTMOD_PORT_PHY_FEC_NONE;
    } else if(auxmode.fec_mode_sel == bcmplpApertaRS544) {
       speed_config.fec = PORTMOD_PORT_PHY_FEC_RS_544;
    } else if(auxmode.fec_mode_sel == bcmplpApertaBaseR) {
       speed_config.fec = PORTMOD_PORT_PHY_FEC_BASE_R;
    } else if(auxmode.fec_mode_sel == bcmplpApertaRS272) {
       speed_config.fec = PORTMOD_PORT_PHY_FEC_RS_272;
    } else if(auxmode.fec_mode_sel == bcmplpApertaRSFEC) {
       speed_config.fec = PORTMOD_PORT_PHY_FEC_RS_FEC;
    } else if(auxmode.fec_mode_sel == bcmplpApertaRS544_2XN) {
       speed_config.fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN;
    } else {
        speed_config.fec = PORTMOD_PORT_PHY_FEC_NONE;
    }
    if (auxmode.modulation_mode == bcmplpApertaModulationPAM4) {
         PHYMOD_IF_ERR_RETURN(plp_aperta_get_pll1_div (phy, &pll1_vco));
         if ( pll1_vco != APERTA_26GVCO_VALUE) {
             PHYMOD_DEBUG_ERROR(("To configure PAM4 mode, Pll1 needs to be configured with 26G VCO\n"));
             return PHYMOD_E_PARAM;
         }
    }
    if (APERTA_IS_LINE_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_CTRL_SWGPREG19r(&phy->access, &swgpreg_19));
    } else {
        /* System side if failover enabled*/
        if (auxmode.failover_config.lane_map != 0) {
            swgpreg_19.v[0] = auxmode.failover_config.lane_map ;
            swgpreg_19.v[0] |= (auxmode.failover_config.mux_location) << 8;
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_CTRL_SWGPREG19r(&phy->access, swgpreg_19));
        }
    }
    /* Disable previously allocated port*/
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_pm_info_speed_get(phy, &prev_speed, &prev_lanemap));

    prev_no_lanes = plp_aperta_count_no_bits(prev_lanemap);
    APERTA_GET_PORT_FROM_LM_SP(prev_speed, (prev_speed/prev_no_lanes), prev_lanemap, prev_port, unused);
    APERTA_UNUSED_VAR(unused);
    APERTA_UNUSED_VAR(prev_port);

    PHYMOD_CRIT_INFO(("Prev speed :%d prev_port:%d prev_lanemap:%x\n", prev_speed, prev_port, prev_lanemap));
    /* Since we are flushing the data in FIFO, It is mandatory to enable TX and Rx*/
    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        temp_acc.port_loc = side;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_tscbh_phy_power_get(&temp_acc, &pwr_get[side-1]));
        if ((pwr_get[side-1].tx != phymodPowerOn) ||
                (pwr_get[side-1].rx != phymodPowerOn)) {
            power.tx = 1;
            power.rx = 1;
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_tscbh_phy_power_set(&temp_acc, &power));
        }
    }

   
    if ((prev_speed == APERTA_SPEED_400G) && 
            (phy->access.lane_mask & 0xF || phy->access.lane_mask == 0xFF ||  phy->access.lane_mask & 0xF0 )) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_tx_rx_enable(phy, 0/*Disable*/,  1/*Singleport*/, 0/*Non Failover*/));
        if (auxmode.failover_config.lane_map != 0) {
            pfo_phy->access.lane_mask = auxmode.failover_config.lane_map;
            PHYMOD_IF_ERR_RETURN(plp_aperta_pm_tx_rx_enable(pfo_phy, 0/*Disable*/,  0/*Non Singleport*/, 0/*Non failover*/));

        }

    } 
    if (prev_speed != APERTA_SPEED_400G) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_tx_rx_enable(phy, 0/*Disable*/,  0/*Non Singleport*/, 0/*Non failover*/));
        if (auxmode.failover_config.lane_map != 0) {
            pfo_phy->access.lane_mask = auxmode.failover_config.lane_map;
            PHYMOD_IF_ERR_RETURN(plp_aperta_pm_tx_rx_enable(pfo_phy, 0/*Disable*/,  0/*Non Singleport*/, 0/*Non failover*/));

        }
        fo_lane_map = plp_aperta_is_fo_enabled(phy, &fo_side);
        if (fo_lane_map != 0xFFFF) {
            pfo_phy->access.lane_mask = fo_lane_map;
            if (fo_side == phy->port_loc) {
                PHYMOD_CRIT_INFO(("Disabling FO lanemap:%x side:%d\n", fo_lane_map, pfo_phy->port_loc));
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_pm_tx_rx_enable(pfo_phy, 0/*Disable*/,  0/*Non Singleport*/, 0/*Non failover*/));
            }
        }
    }
    if (APERTA_IS_SYSTEM_SIDE(phy) && (prev_speed != 0)) {
        /* Reading here because we are disabling port */
        fo_lane_map = plp_aperta_is_fo_enabled(phy, &fo_side);
        /* Phase1 Disable*/
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_disable_port(phy, config, prev_port, APERTA_PORT_DISABLE_PH1));
        if (auxmode.failover_config.lane_map != 0 || 
                (swgpreg_19.v[0] & 0xFF) != 0) {
            pfo_phy->access.lane_mask = auxmode.failover_config.lane_map ? 
                auxmode.failover_config.lane_map : (swgpreg_19.v[0] & 0xFF);
    
            /* Disable previously allocated port*/
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_pm_info_speed_get(pfo_phy, &prev_speed, &prev_lanemap));
            prev_no_lanes = plp_aperta_count_no_bits(prev_lanemap);
            APERTA_GET_PORT_FROM_LM_SP(prev_speed, (prev_speed/prev_no_lanes), prev_lanemap, fo_prev_port, unused);
            if ( fo_prev_port != prev_port) { /* Adding this becoz, till here we just done with PH1*/
                /* Phase1 Disable for FO*/
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_disable_port(pfo_phy, config, fo_prev_port, APERTA_PORT_DISABLE_PH1));
            }
        }
    }
    if (APERTA_IS_SYSTEM_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
               plp_aperta_pm_reset_pcs(phy, config));
        if (prev_speed != 0) {
            /* Phase2 Disable for a port*/
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_disable_port(phy, config, prev_port, APERTA_PORT_DISABLE_PH2));
            if (auxmode.failover_config.lane_map != 0 || 
                    (swgpreg_19.v[0] & 0xFF) != 0) {
                /* Phase2 Disable for FO*/
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta_disable_port(pfo_phy, config, fo_prev_port, APERTA_PORT_DISABLE_PH2));
            }
        }
    }

    if ((prev_speed == APERTA_SPEED_400G) && 
            (phy->access.lane_mask & 0xF || phy->access.lane_mask == 0xFF || phy->access.lane_mask & 0xF0)) {
        plp_aperta_phymod_phy_access_t temp_access;
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_tx_rx_enable_post(phy, 0/*Disable*/, 1/*Single port*/, 0));
        /* update remaining lanes PLP-3094*/
        if (config->data_rate == APERTA_SPEED_200G) {
            PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta_phymod_phy_access_t));
            temp_access.access.lane_mask = 0xFF & ~(phy->access.lane_mask);
            PHYMOD_IF_ERR_RETURN(plp_aperta_pm_tx_rx_enable_post(&temp_access, 0/*Disable*/, 1/*Single port*/, 0));
        }

    }
    if (prev_speed != APERTA_SPEED_400G)  {
        fo_side_lm = auxmode.failover_config.lane_map;
        fo_side_lm |= (phy->port_loc << 8);
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_tx_rx_enable_post(phy, 0/*Disable*/, 0/*NON single*/, fo_side_lm));
        if (APERTA_IS_LINE_SIDE(phy)) {
            if (fo_lane_map != 0xFFFF) {
                pfo_phy->access.lane_mask = fo_lane_map;
                pfo_phy->port_loc = fo_side;
                PHYMOD_CRIT_INFO(("Post Disabling FO lanemap:%x side:%d\n", fo_lane_map, pfo_phy->port_loc));
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta_pm_tx_rx_enable_post(pfo_phy, 0/*Disable*/,  0/*Non Singleport*/, 0 ));
            }
        }
    }
    /* FW updates prev FO lane to enable TH, 
     * updating this as lane needs to be in reset 
     * before enabling tx and rx. Fix for FO flexing*/
    if (APERTA_IS_LINE_SIDE(phy) && (swgpreg_19.v[0] & 0xFF)) {
        plp_aperta_phymod_phy_access_t temp_access;
        int lane_index = 0;
        uint32_t temp = 0;
        PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta_phymod_phy_access_t));
        temp_access.port_loc = phymodPortLocSys;
        for (lane_index =0; lane_index<8; lane_index++) {
            if (swgpreg_19.v[0] & (1 << lane_index)) {
                temp_access.access.lane_mask = 1 << lane_index;
                PHYMOD_IF_ERR_RETURN(
                   plp_aperta_reg32_read(&temp_access, 0x1400010b, &temp));
                if (temp == 0x5001) {
                    PHYMOD_IF_ERR_RETURN(
                       plp_aperta_reg32_write(&temp_access, 0x1400010b, 0x5040));
                }
            }
        }
    }


    PHYMOD_MEMCPY(pfo_phy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    /* Update new speed*/
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_info_speed_set(phy, config->data_rate, auxmode.port_type));
    if (auxmode.failover_config.lane_map != 0) {
         pfo_phy->access.lane_mask = auxmode.failover_config.lane_map;
         PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_info_speed_set(pfo_phy, config->data_rate,auxmode.port_type ));
    }

    PHYMOD_IF_ERR_RETURN(
            _plp_aperta_get_lane_config_word(config, &speed_config.lane_config));

    vco_setting.num_speeds = 1;
    vco_setting.speed_config_list = &speed_config;
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_portmod_pm_speed_config_validate(0/*NA*/, 0/*NA*/, &port, 0x4/*One PLL Switch*/, &vco_setting));
    vco[0] = vco_setting.tvco;
    vco[1] = vco_setting.ovco;

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_portmod_pm_vco_reconfig(0, (phy->access.addr << 8), vco));

    if (APERTA_IS_LINE_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
           _plp_aperta_fill_port_cfg(phy, config, &fw_port_config));
        PHYMOD_DEBUG_INFO(("port:%d port_typ:%d port_mode:%d, speed:%d spmid:%d lpmid:%d port_opt:%d Ilat:%d egrlat:%d foopt:%d, fp_pnu:%d fo_port:%d\n",
        fw_port_config.PortNum, fw_port_config.PortType, fw_port_config.PortMode, fw_port_config.PortSpeed, fw_port_config.SPMPortID,   fw_port_config.LPMPortID,
        fw_port_config.PortOptions, fw_port_config.IngFixedLatency,fw_port_config.EgrFixedLatency, fw_port_config.FOOptions, fw_port_config.FOPortNum, fw_port_config.FOPortID));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_send_fw_msg(phy, config->data_rate, &auxmode,
                       APERTA_FW_MSG_CONFIG_PORT, &fw_port_config, 0/*Write/read*/));
    } else {
        int system_port = 0;
        APERTA_GET_PORT_FROM_LM_SP(config->data_rate,auxmode.lane_data_rate, phy->access.lane_mask, system_port,unused);
        system_port = system_port << 4;
        system_port |=  speed_config.num_lane & 0xF;
        system_port |= (phy->access.lane_mask << 8);
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_direct_reg_write(phy, APERTA_SYSTEM_SIDE_NO_OF_LANES, system_port ));
    }

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_get_pm_info(phy->access.addr, pm_info));
    /* Using Unit as port type, to avoid lot of changes */
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_speed_config_set(auxmode.port_type, port, &speed_config));

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_CTRL_SWGPREG19r(&phy->access, &swgpreg_19));
    if (swgpreg_19.v[0] & 0xFF && (phy->port_loc == phymodPortLocSys)) {
        pfo_phy->access.lane_mask = swgpreg_19.v[0] & 0xFF;
        pfo_phy->port_loc = phymodPortLocSys;
        APERTA_GET_PORT_UPDATE_PM_INFO_LM(pfo_phy, port);
        /* Using Unit as port type, to avoid lot of changes */
        PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_speed_config_set(auxmode.port_type, port, &speed_config));
        APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    }
    if (auxmode.failover_config.lane_map != 0 && (phy->port_loc == phymodPortLocLine)) {
        pfo_phy->access.lane_mask = auxmode.failover_config.lane_map & 0xFF;
        pfo_phy->port_loc = phymodPortLocLine;
        APERTA_GET_PORT_UPDATE_PM_INFO_LM(pfo_phy, port);
        /* Using Unit as port type, to avoid lot of changes */
        PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_speed_config_set(auxmode.port_type, port, &speed_config));
        APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    }

    if (APERTA_IS_LINE_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_port_active_set(phy, config->data_rate, 0/*LDR*/));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_send_fw_msg(phy,config->data_rate, &auxmode,
                       APERTA_FW_MSG_ENABLE_PORT, NULL/*No data*/, 0/*Read/write*/));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_tx_rx_enable( phy, 1/*Enable*/, ((config->data_rate == APERTA_SPEED_400G) ? 1 : 0), 0/*Failover disabled*/));
        /* Enable Failover lane*/
        if (swgpreg_19.v[0] & 0xFF) {
            pfo_phy->access.lane_mask = swgpreg_19.v[0] & 0xFF;
            pfo_phy->port_loc = phymodPortLocSys;
        }
        if (auxmode.failover_config.lane_map != 0) {
            pfo_phy->access.lane_mask = auxmode.failover_config.lane_map & 0xFF;
            pfo_phy->port_loc = phymodPortLocLine;
        }
        if ((auxmode.failover_config.lane_map != 0) || (swgpreg_19.v[0] & 0xFF)) {
            APERTA_GET_PORT_UPDATE_PM_INFO_LM(pfo_phy, port);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_pm_tx_rx_enable(pfo_phy, 1/*Enable*/, 0/*Single port*/, 1/*Failover Enable*/));
            APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);
        }
        swgpreg_19.v[0] = 0;
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_CTRL_SWGPREG19r(&phy->access, swgpreg_19));

    }
    /* Putting back user configured value*/
    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        temp_acc.port_loc = side;
        power.tx = pwr_get[side-1].tx;
        power.rx = pwr_get[side-1].rx;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_tscbh_phy_power_set(&temp_acc, &power));
    }   


    return PHYMOD_E_NONE;
}

int plp_aperta_count_no_bits(int data)
{
     /* Take alternalte bits*/
     data = data - ((data >> 1) & 0x55555555);
     /* Take consecutive bits*/
     data = (data & 0x33333333) + ((data >> 2) & 0x33333333);
     /* Get the corner*/
     data = ((data + (data>> 4)) & 0x0F0F0F0F);
     return (data * 0x01010101) >> 24;
}

int plp_aperta_pm_is_fo_enabled(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_inf_config_t* config,
                            unsigned int *is_fo_enabled, unsigned int *pri_port, 
                            unsigned int *primary_lm)
{
    unsigned int num_lanes = 0, num_ports = 0, basic_lane_map = 0x3 ;
    unsigned int port_index = 0, pri_lane_map = 0, lane_map = 0, primary_port = 0 ;
    int rv = 0;
    aperta_config_port_t temp_port_cfg ;
    aperta_device_aux_modes_t *auxmode = (aperta_device_aux_modes_t*)config->device_aux_modes;

    if (phy->access.lane_mask != 0xFF) {
        num_lanes = plp_aperta_count_no_bits(phy->access.lane_mask);  /* Get num lane from lane mask*/
        num_ports = (8/num_lanes);   /* Get num ports*/

        /* Get the basic lanemap based on number of lanes*/
        if (num_lanes == 1) {
            basic_lane_map =0x1;
        } else if (num_lanes == 2) {
            basic_lane_map =0x3;
        } else if (num_lanes == 4) {
            basic_lane_map =0xF;
        } 
        for (port_index = 0 ; port_index < num_ports; port_index++) {
            pri_lane_map = (basic_lane_map << (num_lanes * port_index));
            APERTA_GET_PORT_FROM_LM(pri_lane_map, primary_port);
            temp_port_cfg.PortNum = primary_port;
            rv = plp_aperta_send_fw_msg(phy, 0, NULL, APERTA_FW_MSG_CONFIG_PORT, &temp_port_cfg, 1);
            if (rv == PHYMOD_E_NONE) {
                if (temp_port_cfg.PortMode == 1) { /*FO enabled for the port*/
                    lane_map = 0;
                    APERTA_GET_LM_FROM_PORT(config->data_rate, auxmode->lane_data_rate, temp_port_cfg.FOPortNum, lane_map);
                    if (lane_map == phy->access.lane_mask) { /* F0 Enabled for the Port*/ 
                        *is_fo_enabled = 1;
                        *primary_lm = pri_lane_map;
                        *pri_port = primary_port;
                        break;
                    }
                }
            } else {
                /*Do Nothing*/
            }
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_interface_config_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_phy_inf_config_t* config)
{
    int unit = 0, int_port = 0, port = 0;
    uint32_t rev_id = 0;
    portmod_speed_config_t speed_config;
    aperta_config_port_t port_cfg ;
    unsigned int pri_port = 0, fo_port = 0, primary_lm = 0;
    aperta_device_aux_modes_t *auxmode = (aperta_device_aux_modes_t*)config->device_aux_modes;

    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);
    if(auxmode == NULL) {
        PHYMOD_DEBUG_ERROR(("Aux mode cannot be NULL\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_MEMSET(auxmode, 0, sizeof(aperta_device_aux_modes_t));

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_speed_config_get(unit, port, &speed_config));

    config->interface_modes =  0; /* Aperta supports only IEEE*/
    config->ref_clock = phymodRefClk156Mhz; /* Aperta supports only 156*/
    config->data_rate = speed_config.speed;
    if (speed_config.fec == PORTMOD_PORT_PHY_FEC_NONE) {
        auxmode->fec_mode_sel = bcmplpApertaNoFEC;
    } else if(speed_config.fec == PORTMOD_PORT_PHY_FEC_RS_544) {
        auxmode->fec_mode_sel = bcmplpApertaRS544;
    } else if (speed_config.fec == PORTMOD_PORT_PHY_FEC_BASE_R) {
        auxmode->fec_mode_sel = bcmplpApertaBaseR;
    } else if (speed_config.fec == PORTMOD_PORT_PHY_FEC_RS_272) {
        auxmode->fec_mode_sel = bcmplpApertaRS272;
    } else if (speed_config.fec == PORTMOD_PORT_PHY_FEC_RS_544_2XN) {
        auxmode->fec_mode_sel = bcmplpApertaRS544_2XN;
    } else if (speed_config.fec == PORTMOD_PORT_PHY_FEC_RS_FEC) {
        auxmode->fec_mode_sel = bcmplpApertaRSFEC;
    } else {
        auxmode->fec_mode_sel = bcmplpApertaNoFEC;
    }
    if (config->data_rate == APERTA_SPEED_400G || 
        config->data_rate == APERTA_SPEED_200G) {
        auxmode->lane_data_rate = APERTA_LD_50G;
        auxmode->modulation_mode = bcmplpApertaModulationPAM4;
    } else if (config->data_rate == APERTA_SPEED_50G &&
               speed_config.num_lane == 1) {
        auxmode->lane_data_rate = APERTA_LD_50G;
        auxmode->modulation_mode = bcmplpApertaModulationPAM4;
    } else if (config->data_rate == APERTA_SPEED_10G || 
            (config->data_rate == APERTA_SPEED_40G &&
            speed_config.num_lane == 4)) {
        auxmode->lane_data_rate = APERTA_LD_10G;
        auxmode->modulation_mode = bcmplpApertaModulationNRZ;
    } else if (config->data_rate == APERTA_SPEED_40G &&
               speed_config.num_lane == 2) {
        auxmode->lane_data_rate = APERTA_LD_20G;
        auxmode->modulation_mode = bcmplpApertaModulationNRZ;
    } else if (config->data_rate == APERTA_SPEED_100G &&
               speed_config.num_lane == 2) {
        auxmode->lane_data_rate = APERTA_LD_50G;
        auxmode->modulation_mode = bcmplpApertaModulationPAM4;
    } else if (config->data_rate == APERTA_SPEED_50G &&
               speed_config.num_lane == 2) {
        auxmode->lane_data_rate = APERTA_LD_25G;
        auxmode->modulation_mode = bcmplpApertaModulationNRZ;
    } else {
        auxmode->lane_data_rate = APERTA_LD_25G;
        auxmode->modulation_mode = bcmplpApertaModulationNRZ;
    }
    /* To check given lane is Failover*/

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_is_fo_enabled(phy, config,
                &fo_port, &pri_port, &primary_lm));

    (void)primary_lm;
    if (fo_port == 1) {
        port_cfg.PortNum = pri_port & 0xF;
    } else {
        port_cfg.PortNum = port & 0xF;
    }
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_send_fw_msg(phy, 0, NULL,
                       APERTA_FW_MSG_CONFIG_PORT, &port_cfg, 1));
    if (port_cfg.PortMode == 1) {
        if (port_cfg.FOOptions & 1){
            if (APERTA_IS_LINE_SIDE(phy))  {/* Line side*/
                APERTA_GET_LM_FROM_PORT(config->data_rate, auxmode->lane_data_rate, port_cfg.FOPortNum, auxmode->failover_config.lane_map);
                auxmode->failover_config.mux_location = (port_cfg.FOOptions & 4) >> 2;
            } else {
                auxmode->failover_config.lane_map = 0;
                auxmode->failover_config.mux_location = 0;
            }
        } else { /* Sys side*/
            if (APERTA_IS_SYSTEM_SIDE(phy))  {
                APERTA_GET_LM_FROM_PORT(config->data_rate, auxmode->lane_data_rate, port_cfg.FOPortNum, auxmode->failover_config.lane_map);
                auxmode->failover_config.mux_location = (port_cfg.FOOptions & 4) >> 2;
            } else {
                auxmode->failover_config.lane_map = 0;
                auxmode->failover_config.mux_location = 0;
            }
        }
    } else {
        auxmode->failover_config.lane_map = 0;
        auxmode->failover_config.mux_location = 0;
    }
    auxmode->fixed_latency_config.enable = (port_cfg.PortOptions & 0x80) ? 1 : 0;
    auxmode->ts_config = (port_cfg.PortOptions & 0x70) >> 4;
    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));
    if (rev_id == APERTA_REV_B0) {
        auxmode->ts_config |= (port_cfg.PortOptions & 8) ;
        auxmode->egr_ptp_fixed_latency = port_cfg.EgrptpFixedLatency;
    }
    if (auxmode->fixed_latency_config.enable) {
        auxmode->fixed_latency_config.igr_dp_ck_cycles = port_cfg.IngFixedLatency;
        auxmode->fixed_latency_config.egr_dp_ck_cycles = port_cfg.EgrFixedLatency;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_link_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *link_status)
{
    int unit = 0, int_port = 0, port = 0;
    int status = 0, latch_status = 0;

    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    /* PCS status from TSCF*/
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_link_get(unit, port, 0, &status));

    *link_status = (status & 0xFFFF);

    /* Read latch status from CLPORT and clear the latch status*/
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_link_latch_down_get(unit, port, PORTMOD_PORT_LINK_LATCH_DOWN_F_CLEAR, &latch_status));
    (void) latch_status;

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_loopback_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t enable)
{
    int unit = 0, int_port = 0, port = 0;
    portmod_loopback_mode_t loopback_type;

    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);
    if (loopback == phymodLoopbackGlobal) {
        loopback_type = phymodLoopbackGlobalPMD;
    } else if (loopback == 5){
        loopback_type = portmodLoopbackMacOuter;
    } else {
        /* coverity[mixed_enums] */
        loopback_type = loopback;
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_loopback_set(unit, port, loopback_type, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_loopback_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t *enable)
{
    int unit = 0, int_port = 0, port = 0;
    portmod_loopback_mode_t loopback_type;
    int lb_get;

    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);
    if (loopback == phymodLoopbackGlobal) {
        loopback_type = phymodLoopbackGlobalPMD;
    } else if (loopback == 5) {
        loopback_type = portmodLoopbackMacOuter;
    } else {
        /* coverity[mixed_enums] */
        loopback_type = loopback;
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_loopback_get(unit, port, loopback_type, &lb_get));
    *enable = lb_get ? 1 : 0;

    return PHYMOD_E_NONE;
}


int plp_aperta_port_active_reset(const plp_aperta_phymod_phy_access_t* phy, int port)
{
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0;

    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA_PRT_ACTIVE_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

    return PHYMOD_E_NONE;
}

int plp_aperta_port_active_set(const plp_aperta_phymod_phy_access_t* phy, int data_rate, int lane_data_rate)
{
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0, port = 0, lane = 0;
    if (phy->access.lane_mask == 0x0) {
        return PHYMOD_E_PARAM;
    }
    lane_data_rate = (int)data_rate/plp_aperta_count_no_bits(phy->access.lane_mask);
    APERTA_GET_PORT_FROM_LM_SP(data_rate, lane_data_rate, phy->access.lane_mask, port, lane);
    (void) lane;
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA_PRT_ACTIVE_SHIFT);
    data |= (1 << APERTA_PRT_ACTIVE_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

    return PHYMOD_E_NONE;
}

int plp_aperta_port_speed_set(const plp_aperta_phymod_phy_access_t* phy, int data_rate, int lane_data_rate)
{
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0, port = 0, lane = 0, prt_speed = 0;

    APERTA_GET_PORT_FROM_LM_SP(data_rate, lane_data_rate, phy->access.lane_mask, port, lane);
    (void) lane;

    APERTA_PORT_SPEED(data_rate,lane_data_rate,prt_speed);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(0xF << APERTA_PRT_SPEED_SHIFT);
    data |= (prt_speed << APERTA_PRT_SPEED_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_fault_option_set(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_fault_option_t fault_option)
{
    int port = 0;
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0;

    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA_FAULT_SHIFT);
    data |= (fault_option << APERTA_FAULT_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_fault_option_get(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_fault_option_t *fault_option)
{
    int port = 0;
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0;

    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    *fault_option = (data >>APERTA_FAULT_SHIFT) & 1;

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_flow_control_set(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_flow_control_t flow_control)
{
    int port = 0;
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0;

    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA_FLOW_CTRL_SHIFT);
    data |= (flow_control << APERTA_FLOW_CTRL_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

   return PHYMOD_E_NONE;
}

int plp_aperta_pm_flow_control_get(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_flow_control_t *flow_control)
{
    int port = 0;
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0;

    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    *flow_control = (data >> APERTA_FLOW_CTRL_SHIFT) & 1;

   return PHYMOD_E_NONE;
}

int plp_aperta_pm_store_and_forward_mode_set(const plp_aperta_phymod_phy_access_t* phy, int enable)
{
    int port = 0;
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0;

    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA_S_F_SHIFT);
    data |= (enable << APERTA_S_F_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

   return PHYMOD_E_NONE;
}
int plp_aperta_pm_store_and_forward_mode_get(const plp_aperta_phymod_phy_access_t* phy, int *is_enable)
{
    int port = 0;
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t data = 0;

    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    *is_enable = (data >> APERTA_S_F_SHIFT) & 1;

   return PHYMOD_E_NONE;

}

int plp_aperta_send_fw_msg(const plp_aperta_phymod_phy_access_t *phy,
                       int data_rate, aperta_device_aux_modes_t *aux,
                       int msg, void *data, int read)
{
    int port_num = 0, lane = 0, rv = 0;

    (void) lane;
    switch (msg) {
        case APERTA_FW_MSG_CONFIG_PHY: {
            aperta_config_phy_t *config_phy = (aperta_config_phy_t*)data;
#ifdef ATE_PRINT_ENABLED
            if (read ==1) {
                PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_READ: APERTA_FW_MSG_CONFIG_PHY :die=%0d, lane_mask=0x%0x side=%s",phy->access.addr, phy->access.lane_mask,(phy->port_loc == phymodPortLocLine) ? "LINE": "SYS"));
            } else {
                PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_WRITE: APERTA_FW_MSG_CONFIG_PHY :die=%0d, lane_mask=%0x side=%s config_phy.MACsecOpt=0x%0x config_phy.IOOpt=0x%0x",phy->access.addr, phy->access.lane_mask,(phy->port_loc == phymodPortLocLine) ? "LINE": "SYS",config_phy->MACsecOpt, config_phy->IOOpt));
            }
#endif
            if (read == 1) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_fw_config_phy_get(phy, config_phy));

            } else {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_fw_config_phy_set(phy, config_phy));
            }
        }
        break;
        case APERTA_FW_MSG_CLOCK_GEN: {
            aperta_clock_gen_t *clk_cfg = (aperta_clock_gen_t*) data;
#ifdef ATE_PRINT_ENABLED
            if (read==1) {
                PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_READ: APERTA_FW_MSG_CLK_GEN: die=%0d lane_mask=0x%0x side=%s clk_cfg.RClkNum=0x%0x\n",phy->access.addr, phy->access.lane_mask, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS", clk_cfg->RClkNum));
            } else {
                PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_WRITE: APERTA_FW_MSG_CLOCK_GEN: die=%0d lane_mask=0x%0x side=%s clk_cfg.RClkNum=0x%0x clk_cfg.ClkGenEn=0x%0x clk_cfg.ClkSide=0x%0x clk_cfg.ClkLane=0x%0x clk_cfg.Divider=0x%0x clk_cfg.SquelchMode=0x%0x clk_cfg.PortLanes=0x%0x\n",
                    phy->access.addr, phy->access.lane_mask, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS", clk_cfg->RClkNum,clk_cfg->ClkGenEn,clk_cfg->ClkSide,clk_cfg->ClkLane,clk_cfg->Divider,clk_cfg->SquelchMode,clk_cfg->PortLanes));
            }
#endif
            if (read == 1) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_fw_clock_gen_read (phy,  clk_cfg));
            } else {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_fw_configure_clock_gen (phy,  clk_cfg));
            }
        }
        break;
        case APERTA_FW_MSG_CONFIG_PORT: {
            aperta_config_port_t *port_cfg = (aperta_config_port_t*)data;
#ifdef ATE_PRINT_ENABLED
            if (read==1) {
                PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_READ: APERTA_FW_MSG_CONFIG_PORT: die=%0d lane_mask=0x%0x side=%s",phy->access.addr, phy->access.lane_mask,(phy->port_loc == phymodPortLocLine) ? "LINE": "SYS"));
            } else {
                PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_WRITE: APERTA_FW_MSG_CONFIG_PORT: die=%0d lane_mask=0x%0x side=%s PortNum=0x%0x PortType=0x%0x PortMode=0x%0x PortSpeed=0x%0x SPMPortID=0x%0x LPMPortID=0x%0x PortOptions=0x%0x IngFixedLatency=0x%0x EgrFixedLatency=0x%0x FOOptions=0x%0x FOPortNum=0x%0x FOPortID=0x%0x",
                    phy->access.addr, phy->access.lane_mask,(phy->port_loc == phymodPortLocLine) ? "LINE": "SYS",port_cfg->PortNum,port_cfg->PortType,port_cfg->PortMode,port_cfg->PortSpeed,port_cfg->SPMPortID,port_cfg->LPMPortID,port_cfg->PortOptions,port_cfg->IngFixedLatency,port_cfg->EgrFixedLatency,port_cfg->FOOptions,port_cfg->FOPortNum,port_cfg->FOPortID));
            }
#endif
            if (read == 1) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_fw_config_port_get (phy, port_cfg));
            } else {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_fw_config_port_set (phy, port_cfg));
            }
        }
        break;
        case APERTA_FW_MSG_PAUSE_PORT:
            APERTA_GET_PORT_FROM_LM_SP(data_rate, aux->lane_data_rate, phy->access.lane_mask, port_num, lane);
#ifdef ATE_PRINT_ENABLED
            PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_WRITE: APERTA_FW_MSG_PAUSE_PORT: die=%0d lane_mask=0x%0x side=%s port_num=0x%0x",phy->access.addr, phy->access.lane_mask,(phy->port_loc == phymodPortLocLine) ? "LINE": "SYS",port_num));
#endif
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_fw_port_op (phy, port_num, APERTA_FW_PORT_OP_PAUSE));
            break;
        case APERTA_FW_MSG_RESUME_PORT:
            APERTA_GET_PORT_FROM_LM_SP(data_rate, aux->lane_data_rate, phy->access.lane_mask, port_num, lane);
#ifdef ATE_PRINT_ENABLED
            PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_WRITE: APERTA_FW_MSG_RESUME_PORT: die=%0d lane_mask=0x%0x side=%s port_num=0x%0x",phy->access.addr, phy->access.lane_mask,(phy->port_loc == phymodPortLocLine) ? "LINE": "SYS",port_num));
#endif
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_fw_port_op (phy, port_num, APERTA_FW_PORT_OP_RESUME));
            break;
        case APERTA_FW_MSG_ENABLE_PORT:
            APERTA_GET_PORT_FROM_LM_SP(data_rate, aux->lane_data_rate, phy->access.lane_mask, port_num, lane);
#ifdef ATE_PRINT_ENABLED
            PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_WRITE: APERTA_FW_MSG_ENABLE_PORT: die=%0d lane_mask=0x%0x side=%s port_num=0x%0x",phy->access.addr, phy->access.lane_mask,(phy->port_loc == phymodPortLocLine) ? "LINE": "SYS",port_num));
#endif
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_fw_port_op (phy, port_num, APERTA_FW_PORT_OP_ENABLE));
            break;
        case APERTA_FW_MSG_DISABLE_PORT:
            port_num = *(uint32_t*)data;
#ifdef ATE_PRINT_ENABLED
            PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_WRITE: APERTA_FW_MSG_DISABLE_PORT: die=%0d lane_mask=0x%0x side=%s port_num=0x%0x",phy->access.addr, phy->access.lane_mask,(phy->port_loc == phymodPortLocLine) ? "LINE": "SYS",port_num));
#endif           
            if ((port_num & 0xF0000) == APERTA_PORT_DISABLE_PH1) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_fw_port_op (phy, (port_num &0xFF), APERTA_FW_PORT_OP_PAUSE));
                rv = plp_aperta_fw_port_op (phy, (port_num &0xFF), APERTA_FW_PORT_OP_FLUSH);
                if (rv != 0) {
                    PHYMOD_CRIT_INFO(("Retrying flush port...\n"));
                    rv = plp_aperta_fw_port_op (phy, (port_num &0xFF), APERTA_FW_PORT_OP_FLUSH);
                    if (rv != 0) {
                        PHYMOD_CRIT_INFO(("Flush Fails. Ignoring error as disable port takes care of it ...\n"));
                    }
                }
            }
            
            if ((port_num & 0xF0000) == APERTA_PORT_DISABLE_PH2) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_fw_port_op (phy, (port_num & 0xFF) , APERTA_FW_PORT_OP_DISABLE));
            }
            break;
        case APERTA_FW_MSG_FLUSH_PORT:
            APERTA_GET_PORT_FROM_LM_SP(data_rate, aux->lane_data_rate, phy->access.lane_mask, port_num, lane);
#ifdef ATE_PRINT_ENABLED
            PHYMOD_DEBUG_INFO(("ATE PLP_FW_MESSAGE_WRITE: APERTA_FW_MSG_FLUSH_PORT: die=%0d lane_mask=0x%0x side=%s port_num=0x%0x",phy->access.addr, phy->access.lane_mask,(phy->port_loc == phymodPortLocLine) ? "LINE": "SYS",port_num));
#endif
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_fw_port_op (phy, port_num, APERTA_FW_PORT_OP_FLUSH));
            break;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_max_pkt_size_set(const plp_aperta_phymod_phy_access_t* phy, int size)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_max_packet_size_set(unit, port, size));

    return PHYMOD_E_NONE;
}
int plp_aperta_pm_max_pkt_size_get(const plp_aperta_phymod_phy_access_t* phy, int *size)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_max_packet_size_get(unit, port, size));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_runt_threshold_set(const plp_aperta_phymod_phy_access_t* phy, int threshold)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_runt_threshold_set(unit, port, threshold));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_runt_threshold_get(const plp_aperta_phymod_phy_access_t* phy, int *threshold)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_runt_threshold_get(unit, port, threshold));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_pad_size_set(const plp_aperta_phymod_phy_access_t* phy, int pad_size)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_pad_size_set(unit, port, pad_size));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_pad_size_get(const plp_aperta_phymod_phy_access_t* phy, int *pad_size)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_pad_size_get(unit, port, pad_size));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_tx_mac_sa_set(const plp_aperta_phymod_phy_access_t* phy, unsigned char mac_sa[6])
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_tx_mac_sa_set(unit, port, mac_sa));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_tx_mac_sa_get(const plp_aperta_phymod_phy_access_t* phy, unsigned char mac_sa[6])
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_tx_mac_sa_get(unit, port, mac_sa));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_rx_mac_sa_set(const plp_aperta_phymod_phy_access_t* phy, unsigned char mac_sa[6])
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_rx_mac_sa_set(unit, port, mac_sa));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_rx_mac_sa_get(const plp_aperta_phymod_phy_access_t* phy, unsigned char mac_sa[6])
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_rx_mac_sa_get(unit, port, mac_sa));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_tx_avg_ipg_set(const plp_aperta_phymod_phy_access_t* phy, int avg_ipg)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_tx_average_ipg_set(unit, port, avg_ipg));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_tx_avg_ipg_get(const plp_aperta_phymod_phy_access_t* phy, int *avg_ipg)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_tx_average_ipg_get(unit, port, avg_ipg));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_tx_preamble_length_set(const plp_aperta_phymod_phy_access_t* phy, int preamble_length)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_pm_tx_preamble_length_get(const plp_aperta_phymod_phy_access_t* phy, int *preamble_length)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_pm_pause_control_set(const plp_aperta_phymod_phy_access_t* phy, portmod_pause_control_t *pause_control)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_pause_control_set(unit, port, pause_control));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_pause_control_get(const plp_aperta_phymod_phy_access_t* phy, portmod_pause_control_t *pause_control)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_pause_control_get(unit, port, pause_control));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_pfc_control_set(const plp_aperta_phymod_phy_access_t* phy, portmod_pfc_control_t *pfc_ctrl)
{
    int unit = 0, int_port = 0, port = 0;
    unsigned int temp = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_pfc_control_set(unit, port, pfc_ctrl));

    /* Get port number*/
    port = (int_port & 0xFF);

    if (pfc_ctrl->rx_enable || pfc_ctrl->tx_enable) {
        /* Ingress PM2MACSEC*/
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_read(phy, (0x4900c001 | (1 << (port*8))), &temp));
        temp &= ~(0xc);
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_write(phy, (0x4900c001 | (1 << (port*8))), temp));

        /* Egress PM2MACSEC*/
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_read(phy, (0x4900d001 | (1 << (port*8))), &temp));
        temp &= ~(0xc);
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_write(phy, (0x4900d001 | (1 << (port*8))), temp));
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_pfc_control_get(const plp_aperta_phymod_phy_access_t* phy, portmod_pfc_control_t *pfc_ctrl)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_pfc_control_get(unit, port, pfc_ctrl));

    return PHYMOD_E_NONE;
}


int plp_aperta_pm_llfc_control_set(const plp_aperta_phymod_phy_access_t* phy, portmod_llfc_control_t *llfc_ctrl)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_llfc_control_set(unit, port, llfc_ctrl));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_llfc_control_get(const plp_aperta_phymod_phy_access_t* phy, portmod_llfc_control_t *llfc_ctrl)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_llfc_control_get(unit, port, llfc_ctrl));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_pfc_config_set(const plp_aperta_phymod_phy_access_t* phy, portmod_pfc_config_t *pfc_config)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_pfc_config_set(unit, port, pfc_config));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_pfc_config_get(const plp_aperta_phymod_phy_access_t* phy, portmod_pfc_config_t *pfc_config)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_pfc_config_get(unit, port, pfc_config));

    return PHYMOD_E_NONE;
}

int plp_aperta_rx_mac_enable_set(const plp_aperta_phymod_phy_access_t* phy, int enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_rx_mac_enable_set(unit, port, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta_tx_mac_enable_set(const plp_aperta_phymod_phy_access_t* phy, int enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_tx_mac_enable_set(unit, port, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta_rx_mac_enable_get(const plp_aperta_phymod_phy_access_t* phy, int *enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_rx_mac_enable_get(unit, port, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta_tx_mac_enable_get(const plp_aperta_phymod_phy_access_t* phy, int *enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_CALL_PM_API(phy, plp_aperta_portmod_port_tx_mac_enable_get(unit, port, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_diagnostic_dump(const plp_aperta_phymod_phy_access_t* phy)
{
    int no_of_reg = 0, port = 0;
    uint32_t mac_counter_data[16], mac_mem_index = 0;
    char *mac_counter_name[APERTA_MAC_REG_COUNTER] = { "CDMIB_R64",   "CDMIB_R127",  "CDMIB_R255",  "CDMIB_R511", "CDMIB_R1023",  "CDMIB_R1518",  "CDMIB_RMGV", "CDMIB_R2047",
                                                       "CDMIB_R4095", "CDMIB_R9216", "CDMIB_R16383","CDMIB_RBCA", "CDMIB_RPROG0", "CDMIB_RPROG1", "CDMIB_RPROG2", "CDMIB_RPROG3",
                                                       "CDMIB_RPKT",  "CDMIB_RPOK",  "CDMIB_RUCA",  "CDMIB_RESERVED0", "CDMIB_RMCA",   "CDMIB_RXPF", "CDMIB_RXPP", "CDMIB_RXCF",
                                                       "CDMIB_RFCS",  "CDMIB_RERPKT","CDMIB_RFLR",  "CDMIB_RJBR", "CDMIB_RMTUE",  "CDMIB_ROVR", "CDMIB_RVLN", "CDMIB_RDVLN",
                                                       "CDMIB_RXUO",  "CDMIB_RXUDA", "CDMIB_RXWSA", "CDMIB_RPRM", "CDMIB_RPFC0",  "CDMIB_RPFCOFF0", "CDMIB_RPFC1", "CDMIB_RPFCOFF1",
                                                       "CDMIB_RPFC2", "CDMIB_RPFCOFF2","CDMIB_RPFC3", "CDMIB_RPFCOFF3", "CDMIB_RPFC4 " ,"CDMIB_RPFCOFF4",  "CDMIB_RPFC5", "CDMIB_RPFCOFF5",
                                                       "CDMIB_RPFC6", "CDMIB_RPFCOFF6", "CDMIB_RPFC7", "CDMIB_RPFCOFF7", "CDMIB_RUND", "CDMIB_RFRG", "CDMIB_RRPKT", "CDMIB_RESERVED1",
                                                       "CDMIB_T64  ", "CDMIB_T127", "CDMIB_T255", "CDMIB_T511", "CDMIB_T1023", "CDMIB_T1518", "CDMIB_TMGV",  "CDMIB_T2047",
                                                       "CDMIB_T4095", "CDMIB_T9216",  "CDMIB_T16383",  "CDMIB_TBC", "CDMIB_TPFC0", "CDMIB_TPFCOFF0",  "CDMIB_TPFC1", "CDMIB_TPFCOFF1",
                                                       "CDMIB_TPFC2", "CDMIB_TPFCOFF2", "CDMIB_TPFC3", "CDMIB_TPFCOFF3", "CDMIB_TPFC4", "CDMIB_TPFCOFF4",  "CDMIB_TPFC5", "CDMIB_TPFCOFF5",
                                                       "CDMIB_TPFC6", "CDMIB_TPFCOFF6", "CDMIB_TPFC7", "CDMIB_TPFCOFF7", "CDMIB_TPKT", "CDMIB_TPOK",     "CDMIB_TUCA",  "CDMIB_TUF",
                                                       "CDMIB_TMCA ", "CDMIB_TXPF", "CDMIB_TXPP", "CDMIB_TXCF", "CDMIB_TFCS ", "CDMIB_TERR", "CDMIB_TOVR", "CDMIB_TJBR",
                                                       "CDMIB_TRPKT", "CDMIB_TFRG", "CDMIB_TVLN", "CDMIB_TDVLN","CDMIB_RBYT","CDMIB_RRBYT","CDMIB_TBYT"};
    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_DIAG_OUT(("\t\tMAC Counters\n"));
    PHYMOD_DIAG_OUT(("\t\t------------\n"));
    PHYMOD_DIAG_OUT(("\t\tPHY: %d LM:%d IF Side: %s\n", phy->access.addr, phy->access.lane_mask, (phy->port_loc == phymodPortLocLine) ? "LINE" : "SYS"));
    PHYMOD_DIAG_OUT(("\t\t----------------------------\n"));
    PHYMOD_DIAG_OUT(("\t\t----------\n"));
    PHYMOD_DIAG_OUT(("\t\t| Port %d |\n", port));
    PHYMOD_DIAG_OUT(("\t\t----------\n"));
    if (phy->access.lane_mask & (1 << port)) {
        for (mac_mem_index = 0; mac_mem_index < APERTA_MAC_MEM_IDX_COUNT; mac_mem_index ++) {
            PHYMOD_IF_ERR_RETURN(
             plp_aperta_mem_read(phy, phymodMemPmMib/*Mem type*/, (mac_mem_index + (16*port))/*offset*/, mac_counter_data/*data*/));
            for (no_of_reg = 0; no_of_reg < APERTA_MAC_MEM_INDEX_MAX_COUNT; no_of_reg++) {
                if ((mac_mem_index == (APERTA_MAC_MEM_IDX_COUNT-1)) &&
                        (no_of_reg == APERTA_MAC_MEM_INDEX_MAX_COUNT-1)) {
                    continue;
                }
                PHYMOD_DIAG_OUT(("%s : 0x%08x%08x\n", mac_counter_name[(8*mac_mem_index)+no_of_reg], mac_counter_data[(no_of_reg*2)+1] , mac_counter_data[(no_of_reg*2)]));
               (void)mac_counter_name;
            }
        }
        PHYMOD_DIAG_OUT(("-------------------------------------------------------------------------------\n"));
    }
    PHYMOD_DIAG_OUT(("\n\n"));
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_mac_mib_stat_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t stat_type, uint64_t *count)
{
    int port = 0;
    uint32_t mac_counter_data[16];
    uint32_t mac_mem_index = (stat_type/8); /* Each memory row has 512 bytes which corresponds to 8 MIB entries per row */
    uint16_t stat_index =(stat_type%8)*2; 

    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    if (stat_type == 0xFFFF) {  /* Clear the stat */
        PHYMOD_IF_ERR_RETURN(
             plp_aperta_cdmac_mib_counter_control_set(phy, 1 /*enable*/, 1 /*Clear*/));
    } else {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_mem_read(phy, phymodMemPmMib/*Mem type*/, (mac_mem_index + (16*port))/*offset*/, mac_counter_data/*data*/));
        COMPILER_64_SET(*count,  (mac_counter_data[stat_index+1] & 0xFF), mac_counter_data[stat_index]);
    }
    return PHYMOD_E_NONE;
}

void plp_aperta_get_no_of_port_from_speed(int speed, int *no_of_port){
    switch (speed) {
        case 10000:
        case 25000:
            *no_of_port = 8;
            break;
        case 100000:
        case 40000:
            *no_of_port = 2;
            break;
        case 50000:
            *no_of_port = 4;
            break;
        default :
            *no_of_port = 1;
    }
}

int plp_aperta_print_pm_mem(const plp_aperta_phymod_phy_access_t* phy, int port_num, int side, phymod_mem_type_t mem_type, int first_idx, int last_idx)
{
    plp_aperta_phymod_phy_access_t phy_temp;
    int side_idx, port_idx_offset;
    int idx;
    int side_list[3] = { APERTA_SW_LINE_SIDE, APERTA_SW_SYS_SIDE, APERTA_SW_NO_SIDE };
    char mem_name[6] = "CDMIB";
    char side_sfx[6] = "     ";
    char port_sfx[3] = ".";
    APERTA_PM_MEM_ACCESS_T pm_mem_access;

#ifdef APERTA_REG_DUMP_INTO_FILE
    PHYMOD_FILE *fp = PHYMOD_FOPEN("./RegDump.txt", "a");
    if (!fp) {
        PHYMOD_DEBUG_ERROR(("Invalid File pointer\n"));
        return PHYMOD_E_FAIL;
    }

#endif
    PHYMOD_MEMSET(&pm_mem_access, 0, sizeof(APERTA_PM_MEM_ACCESS_T));
    for (side_idx = 0; side_idx < 3; side_idx++) {
        if (side != APERTA_SW_BOTH_SIDE && side_list[side_idx] != side) {
            continue;
        }
        if (side == APERTA_SW_BOTH_SIDE && side_list[side_idx] == APERTA_SW_NO_SIDE) {
            continue;
        }
        pm_mem_access.pm_mem_sel = (side_list[side_idx] == APERTA_SW_LINE_SIDE) ? APERTA_LINE_SIDE_PM : APERTA_SYS_SIDE_PM;
        pm_mem_access.pm_mem_type = (phymod_mem_type_t)mem_type;
        pm_mem_access.pm_mem_rw = 0; /* read */

        (side_list[side_idx] == APERTA_SW_NO_SIDE) ? (PHYMOD_STRCPY(side_sfx, "     ")) : ((side_list[side_idx] == APERTA_SW_LINE_SIDE) ? (PHYMOD_STRCPY(side_sfx, "  (L)")) : (PHYMOD_STRCPY(side_sfx, "  (S)")));

        PHYMOD_SPRINTF(port_sfx+1,"%x", port_num % 8);

        /* update pm memory access params */
        switch (mem_type) {
            case phymodMemPmMib:
                port_idx_offset = 16*(port_num);
                pm_mem_access.pm_mem_len = 32;
                PHYMOD_STRCPY(mem_name, "CDMIB");
                break;
            case phymodMemSpeedIdTable:
                port_idx_offset = 0; /* TBD */
                pm_mem_access.pm_mem_len = 10;
                PHYMOD_STRCPY(mem_name, "SPDID");
                break;
            default: /* same as CDMIB */
                port_idx_offset = 16*(port_num);
                pm_mem_access.pm_mem_len = 32;
                PHYMOD_STRCPY(mem_name, "CDMIB");
                break;
        }

        /* Read and print the memory index */
        for (idx = first_idx; idx <= last_idx; idx++) {
            uint32_t _data32b;
            char     spc_sfx[2] = "";
            int      i;
            pm_mem_access.pm_mem_addr = port_idx_offset + idx;
            PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta_phymod_phy_access_t));
            phy_temp.port_loc = (side_list[side_idx] == APERTA_SW_SYS_SIDE) ? phymodPortLocSys : phymodPortLocLine;
            /*coverity[overrun-call]*/
            APERTA_PM_IF_ERR_RETURN(plp_aperta_pm_tsc_mem_access(&phy_temp, &pm_mem_access));

            APERTA_PCS_INFO_PRINT("%d.%s[%0X]%s %s:", phy->access.addr, mem_name, pm_mem_access.pm_mem_addr, port_sfx, side_sfx);

            for (i = 0; i < pm_mem_access.pm_mem_len/4; i++) {
                _data32b = 0;
                PHYMOD_STRCPY(spc_sfx, "");
                APERTA_PCS_INFO_PRINT(" 0x");
                
                /*MSB*/
                _data32b = (pm_mem_access.pm_mem_data[(i*2)+1] & 0xFFFF0000)>>16;
                APERTA_PCS_INFO_PRINT("%s%04x", spc_sfx, _data32b);

                PHYMOD_STRCPY(spc_sfx, "-");

                _data32b = pm_mem_access.pm_mem_data[(i*2)+1] & 0xFFFF;
                APERTA_PCS_INFO_PRINT("%s%04x", spc_sfx, _data32b);
                PHYMOD_STRCPY(spc_sfx, "-");
                
                /*LSB*/
                _data32b = (pm_mem_access.pm_mem_data[i*2] & 0xFFFF0000)>>16;
                APERTA_PCS_INFO_PRINT("%s%04x", spc_sfx, _data32b);

                PHYMOD_STRCPY(spc_sfx, "-");

                _data32b = pm_mem_access.pm_mem_data[i*2] & 0xFFFF;
                APERTA_PCS_INFO_PRINT("%s%04x", spc_sfx, _data32b);

                PHYMOD_STRCPY(spc_sfx, "-");
            }
            APERTA_PCS_INFO_PRINT("\n");
        }
    }
APERTA_ERR:
#ifdef APERTA_REG_DUMP_INTO_FILE
    PHYMOD_FCLOSE(fp);
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta_print_regs(const plp_aperta_phymod_phy_access_t* phy, int port_num, int side, int first_addr, int last_addr) {
    aperta_config_port_t port_cfg ;
    plp_aperta_phymod_phy_access_t phy_temp;
    uint32_t   rddata[16] = { 0 };
    uint32_t   lane_map = 0;
    uint8_t    enabled_port_list[APERTA_MAX_PORT];
    char       portid_sfx[12] = "pm_portid"; /* #dp_port */
    int        portid_list[3] = { 0 };       /* {LINE/EGR, SYS/ING, common} */
    int        portid;
    int        addr, addr_inc;
    int        idx = 0;
    int        side_idx;
    int        side_ii;
    int        side_list[3] = { APERTA_SW_LINE_SIDE, APERTA_SW_SYS_SIDE, APERTA_SW_NO_SIDE };
    int        speed = 0, ldr = 0;
#ifdef APERTA_REG_DUMP_INTO_FILE
    PHYMOD_FILE *fp = PHYMOD_FOPEN("./RegDump.txt", "a");
    if (!fp) {
        PHYMOD_DEBUG_ERROR(("Invalid File pointer\n"));
        return PHYMOD_E_FAIL;
    }

#endif

    PHYMOD_MEMSET(&port_cfg, 0, sizeof(aperta_config_port_t));
    APERTA_PM_IF_ERR_RETURN(plp_aperta_get_enabled_port(phy, enabled_port_list));

    if(port_num != APERTA_PORT_NONE) {
        if(enabled_port_list[port_num]){
            port_cfg.PortNum = port_num;
            APERTA_PM_IF_ERR_RETURN(plp_aperta_fw_config_port_get (phy, &port_cfg));
        }
    }

    if ((first_addr & 0xF0000000) == 0x10000000) { /* PM */
        addr_inc = 1;
        portid_list[APERTA_SW_LINE_SIDE] = port_cfg.LPMPortID;
        portid_list[APERTA_SW_SYS_SIDE]  = port_cfg.SPMPortID;
        portid_list[2] = -1; /* ERROR scenario */
        PHYMOD_STRCPY(portid_sfx, "pm_portid");
    } else if ((first_addr & 0xF0000000) == 0x20000000) { /* EIP218 */
        addr_inc = 4;
        portid_list[APERTA_SW_LINE_SIDE] = -1;       /* ERROR scenario */
        portid_list[APERTA_SW_SYS_SIDE]  = -1;       /* ERROR scenario */
        portid_list[2] = port_num;                   /* APERTA_SW_NO_SIDE */
        PHYMOD_STRCPY(portid_sfx, "dp_port");
    } else if ((first_addr & 0xFF000000) == 0x49000000) { /* CHIP_IND */
        addr_inc = 1;
        portid_list[APERTA_SW_LINE_SIDE] = port_num;  /* ING    */
        portid_list[APERTA_SW_SYS_SIDE] = port_num;   /* EGR    */
        portid_list[2] = port_num;          /* Common */
        PHYMOD_STRCPY(portid_sfx, "dp_port");
    } else if (((first_addr & 0xFF000000) >= 0x43000000) & ((first_addr & 0xFF000000) <= 0x46000000)) { /* MACsec registers */
        addr_inc = 4;
        portid_list[APERTA_SW_LINE_SIDE] = port_num;
        portid_list[APERTA_SW_SYS_SIDE] = port_num;
        portid_list[2] = port_num;
        PHYMOD_STRCPY(portid_sfx, "dp_port");
    } else {
        addr_inc = 1;
        portid_list[APERTA_SW_LINE_SIDE] = port_num;
        portid_list[APERTA_SW_SYS_SIDE] = port_num;
        portid_list[2] = port_num;
        PHYMOD_STRCPY(portid_sfx, "dp_port");
    }
    for (side_idx = 0; side_idx < 3; side_idx++) {
        if (side != APERTA_SW_BOTH_SIDE && side_list[side_idx] != side) {
            continue;
        }
        /* For APERTA_SW_BOTH_SIDE registers, don't loop thru APERTA_SW_NO_SIDE option which is for common regs. */
        if (side == APERTA_SW_BOTH_SIDE && side_list[side_idx] == APERTA_SW_NO_SIDE) {
            continue;
        }
        side_ii = side_list[side_idx];
        portid  = portid_list[side_idx];
        for (idx = 0, addr = first_addr; addr <= last_addr; addr = addr + addr_inc) {
            char side_sfx[4] = "   ";
            char port_sfx[3] = ".";

            if (((first_addr & 0xFF000000) == 0x45000000) || ((first_addr & 0xFF000000) == 0x46000000) || ((first_addr & 0xFF00F000) == 0x49004000) || ((first_addr & 0xFF00F000) == 0x49004000)) {
                PHYMOD_STRCPY(side_sfx, "(E)");
            } else if (((first_addr & 0xFF000000) == 0x43000000) || ((first_addr & 0xFF000000) == 0x44000000) || ((first_addr & 0xFF00F000) == 0x49003000) || ((first_addr & 0xFF00F000) == 0x49003000)) {
                PHYMOD_STRCPY(side_sfx, "(I)");
            } else {
                (side_ii == APERTA_SW_NO_SIDE) ? (PHYMOD_STRCPY(side_sfx, "   ")) : ((side_ii == APERTA_SW_LINE_SIDE) ? (PHYMOD_STRCPY(side_sfx, "(L)")) : (PHYMOD_STRCPY(side_sfx, "(S)")));
            }
            if(port_num == APERTA_PORT_NONE) {
                PHYMOD_STRCPY(port_sfx, "");
            } else {
                PHYMOD_SPRINTF(port_sfx+1,"%x", port_num % 8);
            }
            if (idx % 16 == 0) {
                APERTA_PCS_INFO_PRINT("%0d.0x%08X%2s %3s: ", phy->access.addr, addr, port_sfx, side_sfx);
            }

            if (port_num == APERTA_PORT_NONE) {
                APERTA_PM_IF_ERR_RETURN(plp_aperta_reg32_read(phy, addr, &rddata[idx]));idx += addr_inc;
            } else {
                APERTA_FW_DR_USER_DR (port_cfg.PortSpeed, speed, ldr);
                APERTA_GET_LM_FROM_PORT(speed, ldr, port_cfg.PortNum, lane_map);
                PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta_phymod_phy_access_t));
                phy_temp.port_loc = (side_list[side_idx] == APERTA_SW_SYS_SIDE) ? phymodPortLocSys : phymodPortLocLine;
                phy_temp.access.lane_mask = lane_map;
                APERTA_PM_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_temp, addr, &rddata[idx]));
                idx += addr_inc;
            }
            if (idx % 16 == 0 || addr == last_addr) {
                int _idx;
                for (_idx = 0; _idx < idx; _idx = _idx + addr_inc) {
                    if (addr_inc == 1) {
                        APERTA_PCS_INFO_PRINT("0x%-4x ", rddata[_idx]);
                    } else {
                        APERTA_PCS_INFO_PRINT("0x%-18x ", rddata[_idx]);
                    }
                }
                APERTA_PCS_INFO_PRINT(" // %s = %d\n", portid_sfx, portid);
                idx = 0;
            }
        }
    }
APERTA_ERR:
#ifdef APERTA_REG_DUMP_INTO_FILE
    PHYMOD_FCLOSE(fp);
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta_dump_macsec_regs(const plp_aperta_phymod_phy_access_t* phy) {
    int cnt, ch;
#ifdef APERTA_REG_DUMP_INTO_FILE
    PHYMOD_FILE *fp = PHYMOD_FOPEN("./RegDump.txt", "a");
    if (!fp) {
        PHYMOD_DEBUG_ERROR(("Invalid File pointer\n"));
        return PHYMOD_E_FAIL;
    }

#endif
    APERTA_PCS_INFO_PRINT("\n=================================================================================================================================\n");
    APERTA_PCS_INFO_PRINT("Dumping MACSEC Common Registers for DIE_%s\n", (phy->access.addr & 0x01) ? "B" : "A");
    APERTA_PCS_INFO_PRINT("===================================================================================================================================\n");
#ifdef APERTA_REG_DUMP_INTO_FILE
    PHYMOD_FCLOSE(fp);
#endif
    for (cnt = 0; cnt < 11; cnt += 1) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4300FF00 + 0x4 * cnt), (0x4300FF00 + 0x4 * cnt))); /* EIP163_I RAMs */
    }

    /* EIP163_I common */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4300FF60), (0x4300FF60))); /* EIP163_I ECC_CORR_ENABLE       */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4300FF64), (0x4300FF64))); /* EIP163_I ECC_DERR_ENABLE       */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4300FFE8), (0x4300FFE8))); /* EIP163_I FORCE_CLOCK_STATE     */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4300FFEC), (0x4300FFEC))); /* EIP163_I FORCE_CLOCK_ON        */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4300FFF0), (0x4300FFF0))); /* EIP163_I FORCE_CLOCK_OFF       */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4300FFF4), (0x4300FFF4))); /* EIP163_I CONFIG2               */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4300FFF8), (0x4300FFF8))); /* EIP163_I CONFIG                */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x43008100), (0x43008100))); /* EIP163_I ENTRY ENABLE CONTROL  */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x43008104), (0x43008104))); /* EIP163_I ENTRY ENABLE CONTROL  */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4300EC10), (0x4300EC10))); /* EIP163_I CHANNEL COUNT CONTROL */

    /* EIP163_I per_CH */
    for (ch=0; ch <8; ch +=1) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300FE00+ch*4),  (0x4300FE00+ch*4 ))); /* EIP163_I CHANNEL_CTRL          */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E000+ch*64), (0x4300E000+ch*64))); /* EIP163_I TCAMHitMult-Lo        */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E004+ch*64), (0x4300E004+ch*64))); /* EIP163_I -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E008+ch*64), (0x4300E008+ch*64))); /* EIP163_I HeaderPrsedDropped-Lo */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E00C+ch*64), (0x4300E00C+ch*64))); /* EIP163_I -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E010+ch*64), (0x4300E010+ch*64))); /* EIP163_I TCAMMiss Lo           */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E014+ch*64), (0x4300E014+ch*64))); /* EIP163_I -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E018+ch*64), (0x4300E018+ch*64))); /* EIP163_I PktCtrl Lo            */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E01C+ch*64), (0x4300E01C+ch*64))); /* EIP163_I -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E020+ch*64), (0x4300E020+ch*64))); /* EIP163_I PktsData Lo           */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E024+ch*64), (0x4300E024+ch*64))); /* EIP163_I -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E028+ch*64), (0x4300E028+ch*64))); /* EIP163_I PktsDropped Lo        */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E02C+ch*64), (0x4300E02C+ch*64))); /* EIP163_I -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E030+ch*64), (0x4300E030+ch*64))); /* EIP163_I PktsErrIn Lo          */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4300E034+ch*64), (0x4300E034+ch*64))); /* EIP163_I -"-                   */
    }

    /* EIP164_I common */
    for (cnt = 0; cnt < 18; cnt += 1) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4400FF00 + 0x4 * cnt), (0x4400FF00 + 0x4 * cnt))); /* EIP164_I RAMs */
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4400FF60), (0x4400FF60))); /* EIP164_I ECC_CORR_ENABLE */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4400FF64), (0x4400FF64))); /* EIP164_I ECC_DERR_ENABLE */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4400FFE0), (0x4400FFE0))); /* EIP164_I INFLIGHT Status */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4400FFE8), (0x4400FFE8))); /* EIP164_I CLOCK_STATE     */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4400FFEC), (0x4400FFEC))); /* EIP164_I CLOCK_ON        */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4400FFF0), (0x4400FFF0))); /* EIP164_I CLOCK_OFF       */

    /* EIP164_I per_CH */
    for (ch=0; ch <8; ch +=1) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4400FE00+ch*4), (0x4400FE00+ch*4))); /* EIP164_I CHANNEL_CTRL */
    }

    /* EIP163_E common */
    for (cnt = 0; cnt < 11; cnt += 1) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4500FF00 + 0x4 * cnt), (0x4500FF00 + 0x4 * cnt))); /* EIP163_E RAMs */
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4500FF60), (0x4500FF60))); /* EIP163_E ECC_CORR_ENABLE   */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4500FF64), (0x4500FF64))); /* EIP163_E ECC_DERR_ENABLE   */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4500FFE8), (0x4500FFE8))); /* EIP163_E FORCE_CLOCK_STATE */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4500FFEC), (0x4500FFEC))); /* EIP163_E FORCE_CLOCK_ON    */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4500FFF0), (0x4500FFF0))); /* EIP163_E FORCE_CLOCK_OFF   */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4500FFF4), (0x4500FFF4))); /* EIP163_E CONFIG2           */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4500FFF8), (0x4500FFF8))); /* EIP163_E CONFIG2           */

    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x45008100), (0x45008100))); /* EIP163_E ENTRY ENABLE CONTROL  */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x45008104), (0x45008104))); /* EIP163_E ENTRY ENABLE CONTROL  */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4500EC10), (0x4500EC10))); /* EIP163_E CHANNEL COUNT CONTROL */

    /* EIP163_E per_CH */
    for (ch=0; ch <8; ch +=1) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500FE00+ch*4 ), (0x4500FE00+ch*4 ))); /* EIP163_E CHANNEL_CTRL          */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E000+ch*64), (0x4500E000+ch*64))); /* EIP163_E TCAMHitMult-Lo        */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E004+ch*64), (0x4500E004+ch*64))); /* EIP163_E -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E008+ch*64), (0x4500E008+ch*64))); /* EIP163_E HeaderPrsedDropped-Lo */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E00C+ch*64), (0x4500E00C+ch*64))); /* EIP163_E -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E010+ch*64), (0x4500E010+ch*64))); /* EIP163_E TCAMMiss Lo           */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E014+ch*64), (0x4500E014+ch*64))); /* EIP163_E -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E018+ch*64), (0x4500E018+ch*64))); /* EIP163_E PktCtrl Lo            */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E01C+ch*64), (0x4500E01C+ch*64))); /* EIP163_E -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E020+ch*64), (0x4500E020+ch*64))); /* EIP163_E PktsData Lo           */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E024+ch*64), (0x4500E024+ch*64))); /* EIP163_E -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E028+ch*64), (0x4500E028+ch*64))); /* EIP163_E PktsDropped Lo        */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E02C+ch*64), (0x4500E02C+ch*64))); /* EIP163_E -"- Hi                */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E030+ch*64), (0x4500E030+ch*64))); /* EIP163_E PktsErrIn Lo          */
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4500E034+ch*64), (0x4500E034+ch*64))); /* EIP163_E -"-                   */
    }

    /* EIP164_E common */
    for (cnt = 0; cnt < 16; cnt += 1) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4600FF00 + 0x4 * cnt), (0x4600FF00 + 0x4 * cnt))); /* EIP164_E RAMs */
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4600FF60), (0x4600FF60))); /* EIP164_E ECC_CORR_ENABLE */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4600FF64), (0x4600FF64))); /* EIP164_E ECC_DERR_ENABLE */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4600FFE0), (0x4600FFE0))); /* EIP164_E INFLIGHT Status */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4600FFE8), (0x4600FFE8))); /* EIP164_E CLOCK_STATE     */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4600FFEC), (0x4600FFEC))); /* EIP164_E CLOCK_ON        */
    PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, (0x4600FFF0), (0x4600FFF0))); /* EIP164_E CLOCK_OFF       */

    /* EIP164_E per_CH */
    for (ch=0; ch <8; ch +=1) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, ch, APERTA_SW_NO_SIDE, (0x4600FE00+ch*4), (0x4600FE00+ch*4))); /* EIP164_E CHANNEL_CTRL */
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_dump_port_regs(const plp_aperta_phymod_phy_access_t* phy)
{
    uint32_t  rddata[2] = { 0 };
    uint32_t  port_type = 0, fo_port=0, fo_side=0;
    uint8_t enabled_port_list[APERTA_MAX_PORT], fo_port_list[APERTA_MAX_PORT];
    int  port_ii, port_side = APERTA_SW_BOTH_SIDE;
    int  dp_port, spm_portid, lpm_portid;
#ifdef APERTA_REG_DUMP_INTO_FILE
    PHYMOD_FILE *fp = NULL;
#endif
    PHYMOD_MEMSET(fo_port_list, 0, sizeof(uint8_t) * APERTA_MAX_PORT);
    PHYMOD_IF_ERR_RETURN(plp_aperta_get_enabled_port(phy, enabled_port_list));
    for(port_ii = 0; port_ii < APERTA_MAX_PORT; port_ii++) {
        if(enabled_port_list[port_ii]) {
            if (!(fo_port_list[port_ii] & 0xF)) {
                /* Get dp_port, spm_portid, lpm_portid associated with Logical #Port */
                PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, 0x0100a102 + 16 * port_ii, &rddata[0])); /* dp_portid */
                PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, 0x0100a104 + 16 * port_ii, &rddata[1])); /* lpm_portid, spm_portid */
                PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, 0x0100a103 + 16 * port_ii, &port_type)); /* port_type */
                dp_port    = (rddata[0] & 0x00FF);
                spm_portid = (rddata[1] & 0x00FF);
                lpm_portid = (rddata[1] & 0xFF00) >> 8;

                if (port_type & 0xFF) { /*If failover enabled*/
                    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, 0x0100a107 + 16 * port_ii, &fo_port)); /* FO Port*/
                    fo_port &= 0xFF;
                    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, 0x0100a101 + 16 * port_ii, &fo_side)); /* FO Side*/
                    fo_side &= (0x100);
                    fo_side >>= 8;
                    fo_side = fo_side ? APERTA_SW_LINE_SIDE : APERTA_SW_SYS_SIDE;
                    fo_port_list[fo_port] = (1 | (fo_side << 4));
                    enabled_port_list[fo_port] = 1; /*Store it for Future iteration*/
                }
                port_side = APERTA_SW_BOTH_SIDE;
            } else {
                /* FO port*/
                dp_port    = port_ii;
                spm_portid = port_ii;
                lpm_portid = port_ii;
                port_side = (fo_port_list[port_ii]) >> 4;
                enabled_port_list[port_ii] = 0; 
                fo_port_list[port_ii] = 0;
            }


#ifdef APERTA_REG_DUMP_INTO_FILE
            fp = PHYMOD_FOPEN("./RegDump.txt", "a");
            if (!fp) {
                PHYMOD_DEBUG_ERROR(("Invalid File pointer\n"));
                return PHYMOD_E_FAIL;
            }
#endif
            APERTA_PCS_INFO_PRINT("Logical Port %x dp_portid=%d spm_portid=%x lpm_portid=%x\n", port_ii, dp_port, spm_portid, lpm_portid);
            APERTA_PCS_INFO_PRINT("rddata[0]=%x rdata[1]=%x\n\n", rddata[0], rddata[1]);

            APERTA_PCS_INFO_PRINT("\n=================================================================================================================================\n");
            APERTA_PCS_INFO_PRINT("Dumping port-level registers: PORT-%0d (dp_port=%x spm_portid=%x lpm_portid=%x)\n", port_ii, dp_port, spm_portid, lpm_portid);
            APERTA_PCS_INFO_PRINT("===================================================================================================================================\n");
            APERTA_PCS_INFO_PRINT("                  : +0     +1     +2     +3     +4     +5     +6     +7     +8     +9     +A     +B     +C     +D     +E     +F\n");
#ifdef APERTA_REG_DUMP_INTO_FILE
            PHYMOD_FCLOSE(fp);
#endif
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x49007000 + 0x100 * dp_port), (0x4900700F + 0x100 * dp_port))); /* P1588  */

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x49009000 + 0x100 * dp_port), (0x49009080 + 0x100 * dp_port))); /* EGR_SF */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, 0x49009800, 0x49009800));                                         /* EGR_SF */

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4900a000 + 0x100 * dp_port), (0x4900a080 + 0x100 * dp_port))); /* ING_FC */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, 0x4900a800, 0x4900a801));                                         /* ING_FC */

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4900b000 + 0x100 * dp_port), (0x4900b080 + 0x100 * dp_port))); /* EGR_FC */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, 0x4900b800, 0x4900b801));                                         /* EGR_FC */

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4900c000 + 0x100 * dp_port), (0x4900c00c + 0x100 * dp_port))); /* ING_PM2MACSEC */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4900c020 + 0x100 * dp_port), (0x4900c029 + 0x100 * dp_port))); /* ING_MACSEC2PM */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4900d000 + 0x100 * dp_port), (0x4900d00c + 0x100 * dp_port))); /* EGR_PM2MACSEC */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4900d020 + 0x100 * dp_port), (0x4900d029 + 0x100 * dp_port))); /* EGR_MACSEC2PM */

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x21000000 + 0x100 * dp_port), (0x2100000C + 0x100 * dp_port))); /* EIP218_ING_P2M */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x22000000 + 0x100 * dp_port), (0x2200000C + 0x100 * dp_port))); /* EIP218_EGR_P2M */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x22001000 + 0x100 * dp_port), (0x2200100C + 0x100 * dp_port))); /* EIP218_EGR_FC  */

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4300FE00 + 0x4 * dp_port), (0x4300FE00 + 0x4 * dp_port)));     /* EIP163_I_SYSTEM_CONTROL_CHANNEL_CTRL_0..7 */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4300A000 + 64 * dp_port), (0x4300A03C + 64 * dp_port)));       /* EIP163_I CHANNEL SETTINGS */

            /* Ingress Macsec Statistics Register */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4300e000 + 64 * dp_port), (0x4300e034 + 64 * dp_port)));

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4400fe00 + 4 * dp_port), (0x440fe00 + 4 * dp_port)));

            /* == Egress MACSEC */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4500FE00 + 0x4 * dp_port), (0x4500FE00 + 0x4 * dp_port)));     /* EIP163_I_SYSTEM_CONTROL_CHANNEL_CTRL_0..7 */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4500A000 + 64 * dp_port), (0x4500A03C + 64 * dp_port)));       /* EIP163_I CHANNEL SETTINGS */

            /* Ingress Macsec Statistics Register */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4500e000 + 64 * dp_port), (0x4500e034 + 64 * dp_port)));

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, APERTA_SW_NO_SIDE, (0x4600fe00 + 4 * dp_port), (0x460fe00 + 4 * dp_port)));

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1800D11C, 0x1800D11C)); /* LINE+SYS BH - */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1800D147, 0x1800D148)); /* LINE+SYS BH - */

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x10000000, 0x10000004)); /* LINE+SYS CDPORT 16B */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x10020009, 0x10020016)); /* LINE+SYS CDPORT 16B */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x10020019, 0x10020019)); /* LINE+SYS CDPORT 16B */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1002001E, 0x1002001E)); /* LINE+SYS CDPORT 16B */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x11000005, 0x11000008)); /* LINE+SYS CDPORT 16B */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x11020017, 0x11020018)); /* LINE+SYS CDPORT 32B */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1102001A, 0x1102001D)); /* LINE+SYS CDPORT 32B */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1202001F, 0x1202001F)); /* LINE+SYS CDPORT 48B */

            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x14020100, 0x14020100));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x14020103, 0x14020105));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1402010A, 0x1402010A));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1400010B, 0x1400010C));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1400010F, 0x1400010F));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x14000111, 0x14000111));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x14000113, 0x14000114));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x14000116, 0x14000118));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1400011A, 0x14000122));
            
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x15020101, 0x15020102));	
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x15020106, 0x15020109));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1500010D, 0x1500010D));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x15000120, 0x15000120));
            
            
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1600010E, 0x1600010E));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x16000110, 0x16000110));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x16000112, 0x16000112));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x16000115, 0x16000115));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x16000119, 0x16000119));
            
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029000, 0x18029000)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029002, 0x18029004)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029008, 0x18029012)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029030, 0x18029030)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029033, 0x18029033)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029037, 0x1802903E)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029040, 0x1802904C)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029200, 0x1802920D)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029223, 0x18029223)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029225, 0x18029225)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x18029230, 0x1802923E)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C010, 0x1802C018)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C050, 0x1802C05E)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C062, 0x1802C064)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C070, 0x1802C078)); /* LINE+SYS TSCBH */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C123, 0x1802C128)); /* LINE+SYS TSCBH - PCS Status latched            */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C130, 0x1802C134)); /* LINE+SYS TSCBH - RS-PMA controls               */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C140, 0x1802C142)); /* LINE+SYS TSCBH - FEC controls                  */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C150, 0x1802C15C)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C160, 0x1802C16B)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C170, 0x1802C174)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C17A, 0x1802C17B)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C180, 0x1802C180)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C185, 0x1802C185)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C18A, 0x1802C18A)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C190, 0x1802C194)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C1A0, 0x1802C1AB)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C1B0, 0x1802C1B0)); /* LINE+SYS TSCBH - PCS-FEC control & status      */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C1C0, 0x1802C1CD)); /* LINE+SYS TSCBH - AN                            */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C1D0, 0x1802C1DC)); /* LINE+SYS TSCBH - AN                            */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C1E0, 0x1802C1ED)); /* LINE+SYS TSCBH - AN                            */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C211, 0x1802C212)); /* LINE+SYS TSCBH - AN                            */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C330, 0x1802C330)); /* LINE+SYS TSCBH - AN                            */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C340, 0x1802C340)); /* LINE+SYS TSCBH - AN                            */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C351, 0x1802C357)); /* LINE+SYS TSCBH - AN                            */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, port_ii, port_side, 0x1802C360, 0x1802C367)); /* LINE+SYS TSCBH - RS FEC Symbol error counters  */

            /* MEM RDs */
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_pm_mem(phy, port_ii, port_side, phymodMemPmMib, 0, 15));
            PHYMOD_IF_ERR_RETURN(plp_aperta_print_pm_mem(phy, port_ii, port_side, phymodMemSpeedIdTable, 56, 58));
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_dump_comm_regs(const plp_aperta_phymod_phy_access_t* phy)
{
    uint32_t common_reg_start[25] = APERTA_COMMON_REGISTER_START_LIST ;
    uint32_t common_reg_end[25]   = APERTA_COMMON_REGISTER_END_LIST ;
    uint8_t i = 0;
#ifdef APERTA_REG_DUMP_INTO_FILE
    PHYMOD_FILE *fp = NULL;
    if(!(phy->access.addr & 0x01)) {
        fp = PHYMOD_FOPEN("./RegDump.txt", "w");
    } else {
        fp = PHYMOD_FOPEN("./RegDump.txt", "a");
    }
    if (!fp) {
        PHYMOD_DEBUG_ERROR(("Invalid File pointer\n"));
        return PHYMOD_E_FAIL;
    }
#endif
    APERTA_PCS_INFO_PRINT("===================================================================================================================================\n");
    APERTA_PCS_INFO_PRINT("Dumping Chip-level registers for DIE_%s \n", (phy->access.addr & 0x01) ? "B" : "A");
    APERTA_PCS_INFO_PRINT("===================================================================================================================================\n");
    APERTA_PCS_INFO_PRINT("                  : +0     +1     +2     +3     +4     +5     +6     +7     +8     +9     +A     +B     +C     +D     +E     +F\n");
#ifdef APERTA_REG_DUMP_INTO_FILE
    PHYMOD_FCLOSE(fp);
#endif
    for(i=0; i<24; i++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_print_regs(phy, APERTA_PORT_NONE, APERTA_SW_NO_SIDE, common_reg_start[i], common_reg_end[i]));
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_pcs_info_dump(const plp_aperta_phymod_phy_access_t* phy)
{
#ifdef APERTA_REG_DUMP_INTO_FILE
    char verbosity[4];
    PHYMOD_MEMSET(verbosity, 0, sizeof(verbosity));
#endif    
    PHYMOD_IF_ERR_RETURN(plp_aperta_dump_comm_regs(phy));
    PHYMOD_IF_ERR_RETURN(plp_aperta_dump_port_regs(phy));
    PHYMOD_IF_ERR_RETURN(plp_aperta_dump_macsec_regs(phy));
#ifdef APERTA_REG_DUMP_INTO_FILE
    if(phy->access.flags & BCM_PLP_PCS_DIAG_DUMP_L1) {
        PHYMOD_STRNCPY(verbosity, "L1", 3);
    } else {
        PHYMOD_STRNCPY(verbosity, "L2", 3);
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_phymod_convert_dump_to_txt("RegDump.txt", (phy->access.addr&1) ? "B":"A", verbosity)); 
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta_get_enabled_port(const plp_aperta_phymod_phy_access_t *phy, uint8_t *enabled_port_list) 
{
    uint32_t addr[APERTA_MAX_PORT] = {BCMI_APERTA_D_CTRL_PORT0_CONFIGr, BCMI_APERTA_D_CTRL_PORT1_CONFIGr, BCMI_APERTA_D_CTRL_PORT2_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT3_CONFIGr, BCMI_APERTA_D_CTRL_PORT4_CONFIGr, BCMI_APERTA_D_CTRL_PORT5_CONFIGr,
         BCMI_APERTA_D_CTRL_PORT6_CONFIGr, BCMI_APERTA_D_CTRL_PORT7_CONFIGr};
    uint32_t port = 0, data = 0;

    for (port =0; port<APERTA_MAX_PORT; port++) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_read(phy, addr[port], &data));
        enabled_port_list[port] = ((data >> 15) & 1) ? 1 : 0;
    }
 
    return PHYMOD_E_NONE;
}

int plp_aperta_disable_port(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t* config, int prev_port, int phase) {
    int port_index = 0, gb_port = 0;
    uint8_t enabled_port_list[APERTA_MAX_PORT];
    int possible_port_list = 0, max_ports = 0, temp;
    int phase_400G = 0;
    BCMI_APERTA_D_SWS_SWREG_001r_t disable_port_phase;  /* Added to makesure PH1 disable is not called again*/
    aperta_device_aux_modes_t *aux_mode = (aperta_device_aux_modes_t *) config->device_aux_modes;

    PHYMOD_MEMSET(&disable_port_phase, 0, sizeof(BCMI_APERTA_D_SWS_SWREG_001r_t));

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_get_enabled_port(phy, enabled_port_list));
    if (config->data_rate == APERTA_SPEED_400G) {
        for (port_index = 0; port_index < APERTA_MAX_PORT; port_index++) {
            if (enabled_port_list[port_index] == 1) {
                if(phase & APERTA_PORT_DISABLE_PH1) {
                     phase_400G = (APERTA_PORT_DISABLE_PH1 | port_index);
                     PHYMOD_CRIT_INFO(("##400G Disable port:%d##%x \n",  port_index,phase_400G ));
                    PHYMOD_IF_ERR_RETURN(
                        plp_aperta_send_fw_msg(phy, 0, NULL,
                           APERTA_FW_MSG_DISABLE_PORT, &phase_400G, 0));
                } else if(phase & APERTA_PORT_DISABLE_PH2) {
                    PHYMOD_CRIT_INFO(("##400G Disable port:%d##%x \n",  port_index,phase_400G ));
                    phase_400G = (APERTA_PORT_DISABLE_PH2 | port_index);
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta_send_fw_msg(phy, 0, NULL,
                                APERTA_FW_MSG_DISABLE_PORT, &phase_400G, 0));
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta_port_active_reset(phy, port_index));
                }
            }
        }
    } else {
        if (enabled_port_list[prev_port]) {
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_SWS_SWREG_001r(&phy->access, &disable_port_phase));
            phase &= ~(0xF);
            phase |= prev_port &0xF; 
            if ((!(disable_port_phase.v[0] & (1 << (prev_port&0xF)))) &&
                        (phase & APERTA_PORT_DISABLE_PH1)) { 
                /* Phase 1 of Port Disable*/
                PHYMOD_CRIT_INFO(("##Previous ena-dis port:%x## PH1\n", phase));
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_send_fw_msg(phy, 0, NULL, APERTA_FW_MSG_DISABLE_PORT, &phase, 0));
                disable_port_phase.v[0] &= ~(1 << (prev_port&0xF));
                disable_port_phase.v[0] |= ( 1 << (prev_port&0xF));
            } else if ((disable_port_phase.v[0] & (1 << (prev_port&0xF))) &&
                        (phase & APERTA_PORT_DISABLE_PH2)) { 
                PHYMOD_CRIT_INFO(("##Previous ena-dis port:%x## PH2\n", phase));
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_send_fw_msg(phy, 0, NULL, APERTA_FW_MSG_DISABLE_PORT, &phase, 0));
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_port_active_reset(phy, prev_port));
                disable_port_phase.v[0] &= ~(1 << (prev_port&0xF));
            } else {
                /* Nothing Needed*/
            }
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_SWS_SWREG_001r(&phy->access, disable_port_phase));
        }
        APERTA_LM_TO_POSSIBLE_PORT_LIST(phy->access.lane_mask, possible_port_list, max_ports);
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_SWS_SWREG_001r(&phy->access, &disable_port_phase));
        /* Disable other port if it was already part of curent lane*/
        APERTA_DISABLE_PORT
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_SWS_SWREG_001r(&phy->access, disable_port_phase));
       
        /* To take care of gearbox and reverse GB mode*/
        if (aux_mode->port_type == bcmplpApertaPortTypeGearBox ||  
           (aux_mode->port_type == bcmplpApertaPortTypeReverseGearBox)) {
            int port_map = 0;
            for (gb_port = 0; gb_port < APERTA_MAX_GB_RGB_PORT; gb_port ++) {
                if ((aux_mode->port_type == plp_aperta_port_mode_sys_line_lane[gb_port][APERTA_GB_RGB_PORT_TYPE]) && 
                        (phy->access.lane_mask == plp_aperta_port_mode_sys_line_lane[gb_port][APERTA_GB_RGB_PORT_SYS_LANE])) {
                    port_map = plp_aperta_port_mode_sys_line_lane[gb_port][APERTA_GB_RGB_PORT_SYS_LANE] ^ plp_aperta_port_mode_sys_line_lane[gb_port][APERTA_GB_RGB_PORT_LINE_LANE];
                    APERTA_LM_TO_POSSIBLE_PORT_LIST(port_map, possible_port_list, max_ports);
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_SWS_SWREG_001r(&phy->access, &disable_port_phase));
                    /* Disable other port if it was already part of curent lane*/
                    APERTA_DISABLE_PORT
                    PHYMOD_IF_ERR_RETURN(
                         BCMI_APERTA_D_WRITE_SWS_SWREG_001r(&phy->access, disable_port_phase));

                    break;
                }
            }
        } else {
            int line_prev_speed = 0, line_prev_lanemap = 0, rv = 0;
            plp_aperta_phymod_phy_access_t temp_acc;

            PHYMOD_MEMCPY(&temp_acc, phy, sizeof(temp_acc));

            temp_acc.port_loc = phymodPortLocLine;
            rv = plp_aperta_pm_info_speed_get(&temp_acc, &line_prev_speed, &line_prev_lanemap);
            if (rv == 0) {
                APERTA_LM_TO_POSSIBLE_PORT_LIST(line_prev_lanemap, possible_port_list, max_ports);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_SWS_SWREG_001r(&phy->access, &disable_port_phase));
                PHYMOD_DEBUG_INFO(("Possible list:%x\n", possible_port_list));
                /* Disable other port if it was already part of curent lane*/
                APERTA_DISABLE_PORT
                    PHYMOD_IF_ERR_RETURN(
                            BCMI_APERTA_D_WRITE_SWS_SWREG_001r(&phy->access, disable_port_phase));
            }
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_timesync_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable)
{
    uint32_t prt_flags=0, port = 0, unused = 0 ,rev_id = 0;
    plp_aperta_phymod_phy_inf_config_t port_config;
    aperta_ptp_config_t ptp_cfg;
    aperta_device_aux_modes_t auxmode;
    uint8_t port_option = 0;
    int rv = 0;
    plp_aperta_phymod_phy_access_t fw_access, phy_copy;
    unsigned int is_fo_enabled = 0, primary_lm = 0;
   
    /* Removing validation of SYSTEM SIDE
     * to recalculate the port in case of FO
    if (APERTA_IS_SYSTEM_SIDE(phy)) {
        PHYMOD_DEBUG_ERROR(("PM Time sync not supported in system side\n"));
        return PHYMOD_E_PARAM;
    }*/

    PHYMOD_MEMCPY(&fw_access, phy, sizeof(fw_access));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_MEMSET(&port_config, 0, sizeof(plp_aperta_phymod_phy_inf_config_t));
    PHYMOD_MEMSET(&ptp_cfg, 0, sizeof(ptp_cfg));
    port_config.device_aux_modes = &auxmode;

    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_interface_config_get(phy, prt_flags, &port_config));
    if (phy->access.lane_mask == auxmode.failover_config.lane_map) {
        /* Fo lane map: use sec lanemap to get port*/
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_fo_enabled(phy, &port_config, &is_fo_enabled,
                    &port, &primary_lm));
        (void)is_fo_enabled;
        fw_access.access.lane_mask = primary_lm;
    } else {
        /* Use Primary lanemap to get port*/
        APERTA_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, unused);
    }

    ptp_cfg.PortNum = port;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_fw_config_ptp_read (&fw_access, &ptp_cfg));
    if ((flags >> 28) & 0xF) {
        port_option = (flags >> 28);
        if (rev_id == APERTA_REV_B0) { 
            /* Timestamp config bit 3 needs to be stored in bit 3 
             * of port option and Bit 0-2 will be updated in bit 4-6 of port options*/
            if (port_option >= 5) { /* Move the value to B0 rev*/
                port_option += 3;
            }
            ptp_cfg.PortOptions &= ~(0xF);
            ptp_cfg.PortOptions |= (port_option & 0x7);
            ptp_cfg.PortOptions |= (port_option & 8);
        } else {
            ptp_cfg.PortOptions &= ~(0xF);
            ptp_cfg.PortOptions |= (port_option);
        }
    }
    /* Pause port*/
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode,
                   APERTA_FW_MSG_PAUSE_PORT, NULL, 0));
    /* Flush port */
    rv = plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode,
                   APERTA_FW_MSG_FLUSH_PORT, NULL, 0);
    APERTA_TS_PORT_RESUME(rv, "FLUSH", port);

    if (port_option) {
        rv = plp_aperta_fw_config_ptp_write (&fw_access, &ptp_cfg);
        APERTA_TS_PORT_RESUME(rv, "PTP_CONFIG", port);
    }
    (void) unused;
    (void) flags;

    /* Doing TS only on line side as we dont support
     * TS in system side*/
    phy_copy.port_loc = phymodPortLocLine;  
    if (enable == 1) {
        rv = plp_aperta_tscbh_timesync_adjust_set(&phy_copy, 1);
        APERTA_TS_PORT_RESUME(rv, "APERTA_TSCBH_TIMESYNC_ADJUST_SET", port);
    }
    /* Enable PM TS*/
    rv = plp_aperta_tscbh_timesync_enable_set(&phy_copy, flags, enable);
    APERTA_TS_PORT_RESUME(rv, "APERTA_TSCBH_TIMESYNC_ENABLE_SET", port);

    /* Resume after enable*/
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode,
                   APERTA_FW_MSG_RESUME_PORT, NULL, 0));
    return 0;

}

int plp_aperta_pm_timesync_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable) 
{
    if (APERTA_IS_SYSTEM_SIDE(phy)) {
        PHYMOD_DEBUG_ERROR(("PM Time sync not supported in system side\n"));
        return PHYMOD_E_PARAM;
    }
    return plp_aperta_tscbh_timesync_enable_get(phy, flags, enable);
}

int plp_aperta_pm_timesync_tx_info_get(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_ts_tx_info_t* ts_tx_info) 
{
    tbhmod_ts_tx_info_t tx_info;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_tscbh_timesync_tx_info_get(phy, &tx_info));
    ts_tx_info->ts_in_fifo_lo = tx_info.ts_in_fifo_lo;
    ts_tx_info->ts_in_fifo_hi = tx_info.ts_in_fifo_hi;
    ts_tx_info->ts_seq_id = tx_info.ts_seq_id;
    ts_tx_info->ts_sub_nanosec =  tx_info.ts_sub_nanosec;

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_synce_config_get(const plp_aperta_phymod_phy_access_t* phy,
        phymod_synce_cfg_t* synce_cfg)
{
    aperta_clock_gen_t clk_cfg;

    PHYMOD_MEMSET(&clk_cfg, 0, sizeof(aperta_clock_gen_t));
    clk_cfg.RClkNum = synce_cfg->rclk_out_pin_sel;

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_send_fw_msg(phy, 0, NULL,
            APERTA_FW_MSG_CLOCK_GEN, &clk_cfg, 1));

    if (clk_cfg.ClkGenEn == APERTA_CLKGEN_DIS) {
        synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchDisable;
    } else {
        switch(clk_cfg.SquelchMode) {
            case APERTA_CLKGEN_NO_SQUELCH:
                synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchNone; /* No Squelch Needed.*/
                break;
            case APERTA_CLKGEN_SQUELCH_LOS:
                synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchLOS; /* Squelch Clock on Loss of Signal.*/
                break;
            case APERTA_CLKGEN_SQUELCH_LOL:
                synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchLOL; /* Squelch Clock on Loss of Line.*/
                break;
            default:
                PHYMOD_DEBUG_ERROR(("%s(), Invalid Squelch Config: 0x%x\n", __func__, synce_cfg->clkGenSquelchCfg));
                return PHYMOD_E_UNAVAIL;
        }
        synce_cfg->squelchMonitorLanemap = clk_cfg.PortLanes;
        synce_cfg->recoveredClkLane = clk_cfg.ClkLane;
        synce_cfg->rclk_out_pin_sel = clk_cfg.RClkNum;
        if (clk_cfg.RClkNum == APERTA_CLKGEN_RCLKNUM_0) {  /* Differential Clock Output*/
            switch(clk_cfg.Divider) {
                case APERTA_CLKGEN_DIVIDER_32:
                    synce_cfg->divider = phymodDivider32;
                    break;
                case APERTA_CLKGEN_DIVIDER_64:
                    synce_cfg->divider = phymodDivider64;
                    break;
                case APERTA_CLKGEN_DIVIDER_128:
                    synce_cfg->divider = phymodDivider128;
                    break;
                case APERTA_CLKGEN_DIVIDER_256:
                    synce_cfg->divider = phymodDivider256;
                    break;
                case APERTA_CLKGEN_DIVIDER_512:
                    synce_cfg->divider = phymodDivider512;
                    break;
                case APERTA_CLKGEN_DIVIDER_1024:
                    synce_cfg->divider = phymodDivider1024;
                    break;
                case APERTA_CLKGEN_DIVIDER_2048:
                    synce_cfg->divider = phymodDivider2048;
                    break;
                case APERTA_CLKGEN_DIVIDER_4096:
                    synce_cfg->divider = phymodDivider4096;
                    break;
                default:
                    PHYMOD_DEBUG_ERROR(("%s(), Invalid divider value:0x%x for rclk output: 0x%x\n", __func__, clk_cfg.Divider, clk_cfg.RClkNum));
                    return PHYMOD_E_UNAVAIL;
            }
        } else if ((clk_cfg.RClkNum == APERTA_CLKGEN_RCLKNUM_1) ||
                   (clk_cfg.RClkNum == APERTA_CLKGEN_RCLKNUM_2)) { /* Single Ended Clock output */
            switch(clk_cfg.Divider) {
                case APERTA_CLKGEN_DIVIDER_80:
                    synce_cfg->divider = phymodDivider80;
                    break;
                case APERTA_CLKGEN_DIVIDER_120:
                    synce_cfg->divider = phymodDivider120;
                    break;
                case APERTA_CLKGEN_DIVIDER_240:
                    synce_cfg->divider = phymodDivider240;
                    break;
                case APERTA_CLKGEN_DIVIDER_520:
                    synce_cfg->divider = phymodDivider520;
                    break;
                case APERTA_CLKGEN_DIVIDER_1000:
                    synce_cfg->divider = phymodDivider1000;
                    break;
                case APERTA_CLKGEN_DIVIDER_2040:
                    synce_cfg->divider = phymodDivider2040;
                    break;
                case APERTA_CLKGEN_DIVIDER_4080:
                    synce_cfg->divider = phymodDivider4080;
                    break;
                default:
                    PHYMOD_DEBUG_ERROR(("%s(), Invalid divider value:0x%x for rclk output: 0x%x\n", __func__, clk_cfg.Divider, clk_cfg.RClkNum));
                    return PHYMOD_E_UNAVAIL;
            }
        } else {
            PHYMOD_DEBUG_ERROR(("%s(), Invalid rclk output pin: 0x%x\n", __func__, clk_cfg.RClkNum));
            return PHYMOD_E_UNAVAIL;
        }

        /* Reference code defines LINE_SIDE as 0 and SYS_SIDE as 1 */
        /* FW Message defines LINE_SIDE as 1 and SYS_SIDE as 0 */
        if (clk_cfg.ClkSide == APERTA_CLKGEN_CLKSIDE_SYS) {
            synce_cfg->rclk_if_side = 1;
        } else {
            synce_cfg->rclk_if_side = 0;
        }
    }
    return PHYMOD_E_NONE;
}

/* Check whether SyncE is enabled for differential output on requested pll_idx*/
int plp_aperta_pm_is_synce_enabled(plp_aperta_phymod_phy_access_t *phy, unsigned int pll_idx, int *is_synce_enabled)
{
    phymod_synce_cfg_t synce_cfg_get;

    if(!phy || !is_synce_enabled) {
        return PHYMOD_E_MEMORY;
    }
    PHYMOD_MEMSET(&synce_cfg_get, 0, sizeof(phymod_synce_cfg_t));
    /* This is only applicable to Differential clock. Single ended clocks does not use PLL0 or PLL1 */
    synce_cfg_get.rclk_out_pin_sel = 0  /* RCLK0 */;
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_pm_synce_config_get(phy, &synce_cfg_get));

    *is_synce_enabled = 0;
    if (synce_cfg_get.clkGenSquelchCfg > phymodClkGenSquelchDisable) {
        if (((pll_idx == 0) && (synce_cfg_get.recoveredClkLane <= 3)) ||
            ((pll_idx == 1) && (synce_cfg_get.recoveredClkLane >= 4
                            && synce_cfg_get.recoveredClkLane <= 7))) {
            /* FW uses PLL0 for lanes 0 to 3 and PLL1 for lanes 4 to 7*/
            *is_synce_enabled = 1;
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_synce_update_gpio(const plp_aperta_phymod_phy_access_t* phy)
{
    uint32_t  gpio12_ctrl1 = 0, gpio13_ctrl1 = 0;

    PHYMOD_IF_ERR_RETURN(
                plp_aperta_direct_reg_read(phy, APERTA_GPIO_12_CTRL1_BASE_ADDR, &gpio12_ctrl1));
    PHYMOD_IF_ERR_RETURN(
                plp_aperta_direct_reg_read(phy, APERTA_GPIO_13_CTRL1_BASE_ADDR, &gpio13_ctrl1));

    /* Updating gpio_12/13_sel0 (bit1) and gpio_12/13_sel2 (bit3) to
     * program higher drive strength of the output clock */
    gpio12_ctrl1 |= (1 << 3) | (1 << 1);
    gpio13_ctrl1 |= (1 << 3) | (1 << 1);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_direct_reg_write(phy, APERTA_GPIO_12_CTRL1_BASE_ADDR, gpio12_ctrl1));
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_direct_reg_write(phy, APERTA_GPIO_13_CTRL1_BASE_ADDR, gpio13_ctrl1));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_synce_config_set(const plp_aperta_phymod_phy_access_t* phy,
        const phymod_synce_cfg_t* synce_cfg)
{
    phymod_synce_cfg_t synce_cfg_get;
    aperta_clock_gen_t clk_cfg;
    plp_aperta_phymod_phy_access_t phy_for_other_die;

    PHYMOD_MEMSET(&clk_cfg, 0, sizeof(aperta_clock_gen_t));
    PHYMOD_MEMSET(&synce_cfg_get, 0, sizeof(phymod_synce_cfg_t));

    if ((synce_cfg->clkGenSquelchCfg != phymodClkGenSquelchDisable) &&
        ((synce_cfg->squelchMonitorLanemap <= 0) || (synce_cfg->squelchMonitorLanemap > 0xFF))) {
        PHYMOD_DEBUG_ERROR(("portLane out of range :0x%0x, valid values: 0x1 to 0xFF\n",
            synce_cfg->squelchMonitorLanemap));
        return PHYMOD_E_PARAM;
    }

    /*  Query the current SyncE configuration to make sure all conditions met before applying new configuration:
     *  1. FW message fails if synce is already disabled and user is still trying to disable it.
     *  2. Update to SyncE configuration on the same rclk_out_pin_sel is not allowed. User needs to disable SyncE and re-enable it.
     *  3. rclk_out_pin_sel is shared between die0 and die1 and can only be enabled on one die at any given time.
     *     This restriction is only applicable for single ended outputs (RCLK1 and RCLK2).
     *     RCLK0 can be enabled on both die0 and die1 at the same time.
     */
    synce_cfg_get.rclk_out_pin_sel = synce_cfg->rclk_out_pin_sel;
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_pm_synce_config_get(phy, &synce_cfg_get));

    if ((synce_cfg->clkGenSquelchCfg == phymodClkGenSquelchDisable) && (synce_cfg_get.clkGenSquelchCfg == phymodClkGenSquelchDisable)) {
        PHYMOD_DEBUG_ERROR(("%s(), Error: SyncE is already disabled on rclk_out_pin_sel:%d\n", __func__, synce_cfg->rclk_out_pin_sel));
        return PHYMOD_E_UNAVAIL;
    } else if ((synce_cfg->clkGenSquelchCfg != phymodClkGenSquelchDisable) && ((synce_cfg_get.clkGenSquelchCfg >= phymodClkGenSquelchNone)
                && (synce_cfg_get.clkGenSquelchCfg <= phymodClkGenSquelchLOL))) {
        PHYMOD_DEBUG_ERROR(("%s(), Error: SyncE is already enabled on rclk_out_pin_sel:%d, Disable it first and then re-enable it\n",
            __func__, synce_cfg->rclk_out_pin_sel));
        return PHYMOD_E_UNAVAIL;
    } else {
        if (synce_cfg->clkGenSquelchCfg == phymodClkGenSquelchDisable) {
            clk_cfg.ClkGenEn = APERTA_CLKGEN_DIS;
            clk_cfg.RClkNum = synce_cfg->rclk_out_pin_sel;
            {
                plp_aperta_phymod_phy_access_t synce_pll_phy_acc;
                uint32_t pwr_dwn = 0, pll_lock = 0;
                int is_synce_enabled = 0;
                PHYMOD_MEMCPY(&synce_pll_phy_acc, phy, sizeof(plp_aperta_phymod_phy_access_t));
                /* Check whether PLL0 is needed to be powered up for SyncE */
                PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_synce_enabled(&synce_pll_phy_acc, 0, &is_synce_enabled));
                if (!is_synce_enabled) {
                    /* Doing this only for Pll 0*/
                    synce_pll_phy_acc.access.pll_idx = 0;
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_pll_pwrdn_get(&synce_pll_phy_acc, &pwr_dwn));
                    PHYMOD_IF_ERR_RETURN
                        (plp_aperta_blackhawk_tsc_pll_lock_get(&synce_pll_phy_acc, &pll_lock));
                    if (pwr_dwn == 0 && pll_lock == 0) {
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta_blackhawk_phy_pll_pwrdn(&synce_pll_phy_acc, 0, 1));
                    }
                }
            }
        } else {
            /* Check if SyncE is not configured on the other die for RCLK1 and RCLK2 */
            if (synce_cfg->rclk_out_pin_sel != APERTA_CLKGEN_RCLKNUM_0) {
                int chip_id = 0;
                PHYMOD_IF_ERR_RETURN(
                   plp_aperta_get_chip_id(&phy->access, &chip_id));
                if (chip_id != APERTA_CHIP_81385) {
                    /* Check if SyncE is not configured on the other die */
                    PHYMOD_MEMCPY(&phy_for_other_die, phy, sizeof(plp_aperta_phymod_phy_access_t));
                    /* Toggle bit 0 of the phy address to compute other die's address */
                    phy_for_other_die.access.addr ^= (1 << 0);

                    PHYMOD_MEMSET(&synce_cfg_get, 0, sizeof(phymod_synce_cfg_t));
                    synce_cfg_get.rclk_out_pin_sel = synce_cfg->rclk_out_pin_sel;
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta_pm_synce_config_get(&phy_for_other_die, &synce_cfg_get));
                    if ((synce_cfg_get.clkGenSquelchCfg >= phymodClkGenSquelchNone) && 
                            (synce_cfg_get.clkGenSquelchCfg <= phymodClkGenSquelchLOL)) {
                        PHYMOD_DEBUG_ERROR(("%s(), Error: SyncE is already enabled on other die for rclk_out_pin_sel:%d\n",
                                    __func__, synce_cfg->rclk_out_pin_sel));
                        return PHYMOD_E_UNAVAIL;
                    }
                }
            }
            /* All conditions met, it is now safe to apply new settings */
            clk_cfg.ClkGenEn = APERTA_CLKGEN_ENA;
            clk_cfg.PortLanes = synce_cfg->squelchMonitorLanemap;
            clk_cfg.ClkLane = synce_cfg->recoveredClkLane;
            clk_cfg.RClkNum = synce_cfg->rclk_out_pin_sel;

            /* Update clock drive strength to get a cleaner waveform on single ended output */
            if ((clk_cfg.RClkNum == APERTA_CLKGEN_RCLKNUM_1) ||
                       (clk_cfg.RClkNum == APERTA_CLKGEN_RCLKNUM_2)) { /* Single Ended Clock output */
                PHYMOD_IF_ERR_RETURN(plp_aperta_pm_synce_update_gpio(phy));
            }

            /* Reference code defines LINE_SIDE as 0 and SYS_SIDE as 1 */
            /* FW Message defines LINE_SIDE as 1 and SYS_SIDE as 0 */
            if (synce_cfg->rclk_if_side == 1) {
                clk_cfg.ClkSide = APERTA_CLKGEN_CLKSIDE_SYS;
            } else if (synce_cfg->rclk_if_side == 0) {
                clk_cfg.ClkSide = APERTA_CLKGEN_CLKSIDE_LINE;
            } else {
                PHYMOD_DEBUG_ERROR(("%s(), Invalid if side: 0x%x\n", __func__, synce_cfg->rclk_if_side));
                return PHYMOD_E_UNAVAIL;
            }
            switch(synce_cfg->clkGenSquelchCfg) {
                case phymodClkGenSquelchNone:
                    clk_cfg.SquelchMode = APERTA_CLKGEN_NO_SQUELCH; /* No Squelch Needed.*/
                    break;
                case phymodClkGenSquelchLOS:
                    clk_cfg.SquelchMode = APERTA_CLKGEN_SQUELCH_LOS; /* Squelch Clock on Loss of Signal.*/
                    break;
                case phymodClkGenSquelchLOL:
                    clk_cfg.SquelchMode = APERTA_CLKGEN_SQUELCH_LOL; /* Squelch Clock on Loss of Line.*/
                    break;
                default:
                    PHYMOD_DEBUG_ERROR(("%s(), Invalid Squelch Config: 0x%x\n", __func__, synce_cfg->clkGenSquelchCfg));
                    return PHYMOD_E_UNAVAIL;
            }
            if (clk_cfg.RClkNum == APERTA_CLKGEN_RCLKNUM_0) {  /* Differential Clock Output*/
                switch(synce_cfg->divider) {
                    case phymodDivider32:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_32;
                        break;
                    case phymodDivider64:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_64;
                        break;
                    case phymodDivider128:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_128;
                        break;
                    case phymodDivider256:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_256;
                        break;
                    case phymodDivider512:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_512;
                        break;
                    case phymodDivider1024:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_1024;
                        break;
                    case phymodDivider2048:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_2048;
                        break;
                    case phymodDivider4096:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_4096;
                        break;
                    default:
                        PHYMOD_DEBUG_ERROR(("%s(), Invalid divider value:0x%x for rclk output: 0x%x\n", __func__, synce_cfg->divider, clk_cfg.RClkNum));
                        return PHYMOD_E_UNAVAIL;
                }
            } else if ((clk_cfg.RClkNum == APERTA_CLKGEN_RCLKNUM_1) ||
                       (clk_cfg.RClkNum == APERTA_CLKGEN_RCLKNUM_2)) { /* Single Ended Clock output */
                switch(synce_cfg->divider) {
                    case phymodDivider80:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_80;
                        break;
                    case phymodDivider120:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_120;
                        break;
                    case phymodDivider240:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_240;
                        break;
                    case phymodDivider520:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_520;
                        break;
                    case phymodDivider1000:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_1000;
                        break;
                    case phymodDivider2040:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_2040;
                        break;
                    case phymodDivider4080:
                        clk_cfg.Divider = APERTA_CLKGEN_DIVIDER_4080;
                        break;
                    default:
                        PHYMOD_DEBUG_ERROR(("%s(), Invalid divider value:0x%x for rclk output: 0x%x\n", __func__, synce_cfg->divider, clk_cfg.RClkNum));
                        return PHYMOD_E_UNAVAIL;
                }
            } else {
                PHYMOD_DEBUG_ERROR(("%s(), Invalid rclk output pin: 0x%x\n", __func__, clk_cfg.RClkNum));
                return PHYMOD_E_UNAVAIL;
            }
        }
    }
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_send_fw_msg(phy, 0/*datarate*/, NULL/*Aux*/,
            APERTA_FW_MSG_CLOCK_GEN/*Message*/, &clk_cfg/*Message data*/, 0/*Read/write*/));
    return PHYMOD_E_NONE;
}

int plp_aperta_fec_status_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_fec_dump_status_t* fec_sts)
{
    int flags = 0;
    int is_cl74 = 0;
    uint32_t count_lo = 0, count_hi= 0, lane_index = 0, mpp_lane = 0;
    plp_aperta_phymod_phy_inf_config_t config;
    BCMI_TSCBH_XGXS_RX_X4_FEC_BIT_ERR_CTR0r_t fec_bit_error;
    BCMI_TSCBH_XGXS_RX_X4_FEC_BIT_ERR_CTR1r_t fec_bit_error1;
    aperta_device_aux_modes_t auxmode;
    plp_aperta_phymod_phy_access_t phy_copy;
    
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    config.device_aux_modes = &auxmode;
    PHYMOD_MEMSET(fec_sts, 0xFF, sizeof(phymod_phy_fec_dump_status_t));
    PHYMOD_MEMSET(&fec_bit_error, 0, sizeof(fec_bit_error));
    PHYMOD_MEMSET(&fec_bit_error1, 0, sizeof(fec_bit_error1));
    fec_sts->fec_status_clear = 0;
    
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_interface_config_get(phy, flags, &config));
    if (auxmode.fec_mode_sel == bcmplpApertaNoFEC) {
        PHYMOD_DEBUG_ERROR(("FEC not enabled for lane map:0x%x\n", phy->access.lane_mask));
        return PHYMOD_E_PARAM;
    }
    is_cl74 = (auxmode.fec_mode_sel == bcmplpApertaBaseR) ? 1 : 0;
    
    PHYMOD_IF_ERR_RETURN(
         plp_aperta_tscbh_phy_fec_cl91_correctable_counter_get(phy, 
             ((is_cl74) ? phymod_fec_CL74 : phymod_fec_CL91), &count_lo));
    COMPILER_64_SET(fec_sts->fec_err_cnt.total_frame_corr_cnt, 0, count_lo);
    PHYMOD_IF_ERR_RETURN(
         plp_aperta_tscbh_phy_fec_cl91_uncorrectable_counter_get(phy, 
             ((is_cl74) ? phymod_fec_CL74 : phymod_fec_CL91), &count_lo));
    COMPILER_64_SET(fec_sts->fec_err_cnt.total_frame_uncorr_cnt, 0, count_lo);
    
    if (!is_cl74) {
        PHYMOD_IF_ERR_RETURN(
            BCMI_TSCBH_XGXS_READ_RX_X4_FEC_BIT_ERR_CTR1r(phy, &fec_bit_error1));
        PHYMOD_IF_ERR_RETURN(
            BCMI_TSCBH_XGXS_READ_RX_X4_FEC_BIT_ERR_CTR0r(phy, &fec_bit_error));
        fec_sts->fec_dump_status.fec_ber.post_fec_ber = (BCMI_TSCBH_XGXS_RX_X4_FEC_BIT_ERR_CTR1r_GET(fec_bit_error1) << 16) |
                  BCMI_TSCBH_XGXS_RX_X4_FEC_BIT_ERR_CTR0r_GET(fec_bit_error);

        COMPILER_64_SET(fec_sts->fec_err_cnt.total_symbols_corr_cnt, 0, 0);
        for (lane_index =0; lane_index<8; lane_index ++) {
            if (phy->access.lane_mask & (1 << lane_index)) {
                phy_copy.access.lane_mask = ( 1<< lane_index);
                mpp_lane = (lane_index < 4) ? lane_index : (lane_index-4);
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta_reg32_read(&phy_copy, (BCMI_TSCBH_XGXS_RX_X4_RS_FEC_FEC_SYM_ERR_CTR_LOW0r + (mpp_lane*2)), &count_lo));
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta_reg32_read(&phy_copy, (BCMI_TSCBH_XGXS_RX_X4_RS_FEC_FEC_SYM_ERR_CTR_UP0r + (mpp_lane*2)), &count_hi));
                COMPILER_64_SET(fec_sts->fec_err_cnt.total_symbols_corr_cnt, count_hi, count_lo);
            }
        }
        fec_sts->align_lol_sticky = 1;
        fec_sts->align_lock = 1;
        for (lane_index =0; lane_index<8; lane_index ++) {
            if (phy->access.lane_mask & (1 << lane_index)) {
                phy_copy.access.lane_mask = ( 1<< lane_index);
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_reg32_read(&phy_copy, 0x1802C17A, &count_lo));
                    fec_sts->align_lol_sticky &= (count_lo & 2) >> 1;;
                    fec_sts->align_lock &= (count_lo &1);
            }
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_flush_port(const plp_aperta_phymod_phy_access_t* phy) 
{
    uint32_t flags=0;
    plp_aperta_phymod_phy_inf_config_t config;
    aperta_device_aux_modes_t auxmode;
    int rv = 0;

    PHYMOD_MEMSET(&config, 0, sizeof(plp_aperta_phymod_phy_inf_config_t));
    config.device_aux_modes = &auxmode;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_interface_config_get(phy, flags, &config));
    
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_send_fw_msg(phy, config.data_rate , &auxmode,
                       APERTA_FW_MSG_PAUSE_PORT, NULL, 0));
    rv = plp_aperta_send_fw_msg(phy, config.data_rate , &auxmode,
                       APERTA_FW_MSG_FLUSH_PORT, NULL, 0);
    if (rv != 0) {
        PHYMOD_CRIT_INFO(("Retrying flush port...\n"));
        rv = plp_aperta_send_fw_msg(phy, config.data_rate , &auxmode,
                                   APERTA_FW_MSG_FLUSH_PORT, NULL, 0);
        if (rv != 0) {
            PHYMOD_CRIT_INFO(("Flush Fails. Resuming port ...\n"));
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_send_fw_msg(phy, config.data_rate , &auxmode,
                       APERTA_FW_MSG_RESUME_PORT, NULL, 0));
            return rv;
        }
    }
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_send_fw_msg(phy, config.data_rate , &auxmode,
                       APERTA_FW_MSG_RESUME_PORT, NULL, 0));

    return PHYMOD_E_NONE;
}

int plp_aperta_update_port_config(const plp_aperta_phymod_phy_access_t *phy, aperta_update_port_config_t *port_lat_config) 
{
    uint32_t flags=0, port = 0, unused = 0, rev_id = 0;
    plp_aperta_phymod_phy_inf_config_t port_config;
    aperta_ptp_config_t ptp_cfg;
    aperta_device_aux_modes_t auxmode;
    plp_aperta_phymod_phy_access_t fw_access;
    int rv = 0;
    unsigned int is_fo_enabled = 0, primary_lm = 0;

    if (!port_lat_config) {
        PHYMOD_DEBUG_ERROR(("Port config cannot be NULL\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_MEMCPY(&fw_access, phy, sizeof(fw_access));
    PHYMOD_MEMSET(&port_config, 0, sizeof(plp_aperta_phymod_phy_inf_config_t));
    PHYMOD_MEMSET(&ptp_cfg, 0, sizeof(ptp_cfg));
    port_config.device_aux_modes = &auxmode;

    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_interface_config_get(phy, flags, &port_config));
    if (phy->access.lane_mask == auxmode.failover_config.lane_map) {
        /* Fo lane map: use sec lanemap to get port*/
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_fo_enabled(phy, &port_config, &is_fo_enabled,
                    &port, &primary_lm));
        (void)is_fo_enabled;
        fw_access.access.lane_mask = primary_lm;
    } else {
        /* Use Primary lanemap to get port*/
        APERTA_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, unused);
    }

    ptp_cfg.PortNum = port;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_fw_config_ptp_read (&fw_access, &ptp_cfg));
    /* Pause in config port and resume after enable*/
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode,
                  APERTA_FW_MSG_PAUSE_PORT, NULL, 0));
    rv = plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode,
                  APERTA_FW_MSG_FLUSH_PORT, NULL, 0);
    APERTA_TS_PORT_RESUME(rv, "FLUSH", port);

    if (port_lat_config->fixed_latency_config.enable) {
        ptp_cfg.PortOptions |= 0x80;
        ptp_cfg.IngFixedLatency = port_lat_config->fixed_latency_config.igr_dp_ck_cycles; 
        ptp_cfg.EgrFixedLatency = port_lat_config->fixed_latency_config.egr_dp_ck_cycles; 
    }
    if (rev_id == APERTA_REV_B0) { 
        ptp_cfg.EgrPTPFixedLatency = port_lat_config->egr_ptp_fixed_latency; 
    }
    (void)unused;
    rv = plp_aperta_fw_config_ptp_write (&fw_access, &ptp_cfg);
    APERTA_TS_PORT_RESUME(rv, "CONFIG_PTP", port);
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode,
                  APERTA_FW_MSG_RESUME_PORT, NULL, 0));

    return PHYMOD_E_NONE;
}

uint8_t plp_aperta_log2n(uint32_t n) {
    return ((n > 1) ? (1 + plp_aperta_log2n(n / 2)) : 0);
}

int plp_aperta_get_pll1_div (const plp_aperta_phymod_phy_access_t *phy, int *div)
{
    struct   blackhawk_tsc_uc_core_config_st core_cfg;
    plp_aperta_phymod_phy_access_t pm_phy_copy;

    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    pm_phy_copy.access.pll_idx = 1;
    pm_phy_copy.access.lane_mask = 1;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_blackhawk_tsc_get_uc_core_config(&pm_phy_copy, &core_cfg));
    /* Pll1 cannot be 10G, so checking only for 25G or 26G*/
    if (core_cfg.vco_rate_in_Mhz > APERTA_SPEED_25G && core_cfg.vco_rate_in_Mhz < APERTA_SPEED_26G) {
        *div = APERTA_25GVCO_VALUE;
    } else  {
        *div = APERTA_26GVCO_VALUE;
    }

    return PHYMOD_E_NONE;

}

/*!
 *  Function to get remote ability
 *  
 *  @param phy                             phy access information
 *  @param plp_aperta_phymod_autoneg_ability_t        remote partner abilities 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int _plp_aperta_phy_autoneg_remote_ability_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_ability_t* an_ability_get_type)
{        
    phymod_autoneg_advert_abilities_t remote_ability;
    phymod_autoneg_advert_ability_t ability[APERTA_MAX_NO_ABILITY];
    int index = 0;

    PHYMOD_MEMSET(an_ability_get_type, 0, sizeof(plp_aperta_phymod_autoneg_ability_t));
    PHYMOD_MEMSET(ability, 0, sizeof(phymod_autoneg_advert_ability_t)*APERTA_MAX_NO_ABILITY);
    remote_ability.autoneg_abilities = ability;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_tscbh_phy_autoneg_remote_advert_ability_get(phy, &remote_ability));

    for (index = 0 ; index < remote_ability.num_abilities; index++) {
        if (ability[index].speed == 10000) {
            PHYMOD_AN_CAP_10G_KR_SET(an_ability_get_type->an_cap);
        }
        if (ability[index].speed == 25000) {
            if (ability[index].an_mode == phymod_AN_MODE_CL73) {
                if (ability[index].channel == phymod_channel_short) {
                    if (ability[index].medium == phymodFirmwareMediaTypeCopperCable) {
                        PHYMOD_AN_CAP_25G_CRS1_SET(an_ability_get_type->an_cap);
                    } else {
                        PHYMOD_AN_CAP_25G_KRS1_SET(an_ability_get_type->an_cap);
                    }
                } else {
                    if (ability[index].medium == phymodFirmwareMediaTypeCopperCable) {
                        PHYMOD_AN_CAP_25G_CR_SET(an_ability_get_type->an_cap);
                    } else {
                        PHYMOD_AN_CAP_25G_KR_SET(an_ability_get_type->an_cap);
                    }
                }
            }
            if (ability[index].an_mode == phymod_AN_MODE_MSA) {
                if (ability[index].medium == phymodFirmwareMediaTypeCopperCable) {
                    PHYMOD_AN_CAP_25G_CR1_SET(an_ability_get_type->an_cap);
                } else {
                    PHYMOD_AN_CAP_25G_KR1_SET(an_ability_get_type->an_cap);
                }
            }
        }
        if (ability[index].speed == 50000) {
            if (ability[index].resolved_num_lanes == 2) {
                if (ability[index].medium == phymodFirmwareMediaTypeCopperCable) {
                    PHYMOD_AN_CAP_50G_CR2_SET(an_ability_get_type->an_cap);
                } else {
                    PHYMOD_AN_CAP_50G_KR2_SET(an_ability_get_type->an_cap);
                }
            } else {
                PHYMOD_AN_CAP_50G_CR_KR_SET(an_ability_get_type->an_cap);
            }
        }
        if (ability[index].speed == 100000) {
            if (ability[index].resolved_num_lanes == 4) {
                if (ability[index].medium == phymodFirmwareMediaTypeCopperCable) {
                    PHYMOD_AN_CAP_100G_CR4_SET(an_ability_get_type->an_cap);
                } else {
                    PHYMOD_AN_CAP_100G_KR4_SET(an_ability_get_type->an_cap);
                }
            } else {
                PHYMOD_AN_CAP_100G_CR2_KR2_SET(an_ability_get_type->an_cap);
            }
        }
        if (ability[index].speed == 200000) {
            PHYMOD_AN_CAP_200G_CR4_KR4_SET(an_ability_get_type->an_cap);
        }
        if (ability[index].speed == 400000) {
            PHYMOD_AN_CAP_400G_CR8_KR8_SET(an_ability_get_type->an_cap);
        }

        if (ability[index].speed == 40000) {
            if (ability[index].medium == phymodFirmwareMediaTypeCopperCable) {
                PHYMOD_AN_CAP_40G_CR4_SET(an_ability_get_type->an_cap);
            } else {
                PHYMOD_AN_CAP_40G_KR4_SET(an_ability_get_type->an_cap);
            }
        }
        if (ability[index].fec & (1 << phymod_fec_None)) {
            an_ability_get_type->an_fec = 0x0;
        }
        if (ability[index].fec  & (1 << phymod_fec_CL74)) {
            an_ability_get_type->an_fec |= 2;
        } 
        if (ability[index].fec  &  (1 << phymod_fec_CL91)) {
            an_ability_get_type->an_fec |= 8;
        }
        if (ability[index].fec  &  (1 << phymod_fec_RS544)) {
            an_ability_get_type->an_fec |= 0x20;
        }
        if (ability[index].fec  &  (1 << phymod_fec_RS272)) {
            an_ability_get_type->an_fec |= 0x40;
        }
        if (ability[index].fec  &  (1 << phymod_fec_RS544_2XN)) {
            an_ability_get_type->an_fec |= 0x80;
        }
        if (ability[index].fec  &  (1 << phymod_fec_RS272_2XN)) {
            an_ability_get_type->an_fec |= 0x100;
        }
        an_ability_get_type->capabilities |= (ability[index].pause) << 6;
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta_pcs_status_get(const plp_aperta_phymod_phy_access_t* phy, phymod_pcs_status_t* pcs_status)
{
    unsigned int lane_index = 0, temp = 0;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_MEMSET(pcs_status , 0, sizeof(phymod_pcs_status_t));
    for (lane_index = 0; lane_index < 8; lane_index ++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            phy_copy.access.lane_mask = ( 1<< lane_index);

            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_PCS_LATCH_STS1r, &temp));
            pcs_status->ieee_pcs_sts.ieee_baser_pcs_status_1 |= (temp & 0x400) << 2 ; /*PCS live link*/
            pcs_status->ieee_pcs_sts.ieee_baser_pcs_status_1 |= ((temp & 0x10) ? 0 : 1) << 1 ; /*HI BER LL*/
            pcs_status->ieee_pcs_sts.ieee_baser_pcs_status_2 |= ((temp & 0x20) ? 1 : 0) << 14 ; /*HI BER LH*/

            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_CL82_AM_LATCH_STS_PSLL1r, &temp));
            pcs_status->ieee_pcs_sts.ieee_pcs_alignment_status_1 = temp;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_CL82_AM_LATCH_STS_PSLL2r, &temp));
            pcs_status->ieee_pcs_sts.ieee_pcs_alignment_status_2 = temp ;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_CL82_AM_LATCH_STS_PSLL3r, &temp));
            pcs_status->ieee_pcs_sts.ieee_pcs_alignment_status_3 = temp;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_CL82_AM_LATCH_STS_PSLL4r, &temp));
            pcs_status->ieee_pcs_sts.ieee_pcs_alignment_status_4 = temp;

            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_BIPCNT0r, &temp));
            pcs_status->ieee_pcs_sts.ieee_bip_err_count_pcsln[0] = temp & 0xFF;
            pcs_status->ieee_pcs_sts.ieee_bip_err_count_pcsln[1] = (temp >> 8) & 0xFF;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_BIPCNT1r, &temp));
            pcs_status->ieee_pcs_sts.ieee_bip_err_count_pcsln[2] = temp & 0xFF;
            pcs_status->ieee_pcs_sts.ieee_bip_err_count_pcsln[3] = (temp >> 8) & 0xFF;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_BIPCNT2r, &temp));
            pcs_status->ieee_pcs_sts.ieee_bip_err_count_pcsln[4] = temp & 0xFF;

            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_PSLL_TO_VL_MAP0r, &temp));
            pcs_status->ieee_pcs_sts.ieee_pcs_lane_mapping [0] = temp & 0x1F;
            pcs_status->ieee_pcs_sts.ieee_pcs_lane_mapping [1] = (temp >> 5) & 0x1F;
            pcs_status->ieee_pcs_sts.ieee_pcs_lane_mapping [2] = (temp >> 10) & 0x1F;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_PSLL_TO_VL_MAP1r, &temp));
            pcs_status->ieee_pcs_sts.ieee_pcs_lane_mapping[3] =  temp & 0x1F;
            pcs_status->ieee_pcs_sts.ieee_pcs_lane_mapping[4] =  (temp >> 5) & 0x1F;

            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_BLKSYNC_STSr        , &temp));

            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_block_lock_stat = temp & 0x1F;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_BLK_LOCK_LATCH_STSr , &temp));
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_block_lolock_sticky =  (temp &   0x5) &    1  ?    1 : 0;
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_block_lolock_sticky |= (temp &   0x5) &    4  ?    2 : 0;
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_block_lolock_sticky |= (temp & 0x050) &  0x10 ?    4 : 0;
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_block_lolock_sticky |= (temp & 0x050) &  0x40 ?    8 : 0;
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_block_lolock_sticky |= (temp & 0x100) & 0x100 ? 0x10 : 0;

            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_AM_LOCK_LATCH_STSr  , &temp));
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_am_lolock_sticky =  (temp &   0x5) &    1  ?    1 : 0;
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_am_lolock_sticky |= (temp &   0x5) &    4  ?    2 : 0;
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_am_lolock_sticky |= (temp & 0x050) &  0x10 ?    4 : 0;
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_am_lolock_sticky |= (temp & 0x050) &  0x40 ?    8 : 0;
            pcs_status->pcs_sts.pcs_status_phylane[0].pcs_am_lolock_sticky |= (temp & 0x100) & 0x100 ? 0x10 : 0;

            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_RX_LATCH_STSr  ,&temp));
            pcs_status->pcs_sts.pcs_dskw_align_stat = (temp & 2) ? 1 : 0;

            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_PCS_LATCH_STS1r ,&temp));
            pcs_status->pcs_sts.pcs_dskw_align_loss_sticky = temp & 1;
            pcs_status->pcs_sts.pcs_hiber_sticky = (temp & 0x20) ? 1 : 0;

            /* Cl82 BER CNT*/
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_BER_LOr        , &temp));
            pcs_status->pcs_sts.pcs_ber_cnt  = temp & 0xFF;
            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_BER_HOr        ,&temp));
            pcs_status->pcs_sts.pcs_ber_cnt |= (temp & 0x3FFF) << 8;

            PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy_copy, BCMI_TSCBH_XGXS_RX_X4_PCS_LATCH_STS1r,&temp));
            pcs_status->pcs_sts.pcs_link_stat = (temp & 0x400) ? 1 : 0;
            pcs_status->pcs_sts.pcs_link_stat_sticky = (temp & 4) ? 1 : 0;
            break;
        }
    }



    /*Unsupported counter
     ieee_ber_high_order_counter;
     pcs_am_lock_stat;
     pcs_dskw_error_sticky;
     pcs_skew_stat[PHYMOD_PCS_LOGICAL_LANES];
     pcs_bip_err_cnt[PHYMOD_PCS_LOGICAL_LANES];
     pcs_igbox_clsn_sticky;
     pcs_hiber_stat;
     pcs_status->pcs_sts.pcs_lane_mapping[PHYMOD_PCS_LOGICAL_LANES]));
     fcfec_corrected_block_cnt
     fcfec_corrected_block_cnt
     fcfec_corrected_block_cnt
     fcfec_corrected_block_cnt
     pcs_skew_stat_pcsln[PHYMOD_PCS_ENCODED_BIT_STREAMS];*/

     return PHYMOD_E_NONE;

}
