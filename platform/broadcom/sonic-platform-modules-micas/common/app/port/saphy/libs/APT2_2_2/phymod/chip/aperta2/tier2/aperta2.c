/*
 *         
 * $Id: phymod.xml,v 1.1.2.5 Broadcom SDK $
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *         
 *     
 */

#include <phymod/phymod.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_dispatch.h>

#ifdef PHYMOD_APERTA2_SUPPORT
#include <tier1/aperta2_reg_access.h>
#include <tier1/aperta2_cfg_seq.h>
#include <tier1/aperta2_pm_seq.h>
#include <tier1/bcm_aperta2_direct_defs.h>
#include <tscp.h>
#include <peregrine5_pc.h>
#include <include/pm8x100_gen2.h>
#include <phymod/chip/aperta2.h>

#ifdef PHYMOD_TIMESYNC_SUPPORT
#include "timesync.h"
#endif

aperta2_pm_info_t _plp_aperta2_pm_info[APERTA2_MAX_PM_INFO];
aperta2_sys_port_config_t _plp_aperta2_sys_port_config[APERTA2_MAX_PHY];
extern __phymod__dispatch__t__ plp_aperta2_phymod_tscp_driver;

int plp_aperta2_core_identify(const plp_aperta2_phymod_core_access_t* core, uint32_t core_id, uint32_t* is_identified)
{        
    
    int chip_id = 0;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_chip_id(&core->access, &chip_id));

    if (chip_id == APERTA2_CHIP_85343 ||
        chip_id == APERTA2_CHIP_85344 ||
        chip_id == APERTA2_AGERA2_COMP_59611) {
        *is_identified = 1;
        /* Adding Bcast Suport*/
        *is_identified |= 0x80000000;
    } else {
        *is_identified = 0;
    }
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_core_info_get(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_core_info_t* info)
{
    /* coverity[buffer_size] */
    PHYMOD_STRNCPY(info->name, "APT2", 5);
    return PHYMOD_E_NONE;
}


int plp_aperta2_core_lane_map_set(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_lane_map_t* lane_map)
{        
    return PHYMOD_E_UNAVAIL;
    
}

int plp_aperta2_core_lane_map_get(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_lane_map_t* lane_map)
{        
    int lane_index = 0;
    aperta2_config_lane_t lane_cfg;

    
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_fw_config_lanes((plp_aperta2_phymod_phy_access_t*)core, &lane_cfg, APERTA2_OP_READ));
 
    for (lane_index = 0; lane_index < APERTA2_LANES_PER_OCTAL*2;lane_index ++) {
        if (core->port_loc == phymodPortLocSys) {
            lane_map->lane_map_tx[lane_index] = lane_map->lane_map_rx[lane_index] = lane_cfg.sys_lane_list[lane_index];
        } else {
            lane_map->lane_map_tx[lane_index] = lane_map->lane_map_rx[lane_index] = lane_cfg.line_lane_list[lane_index];
        }
    }

    lane_map->num_of_lanes = 16;
   
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_core_reset_set(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_reset_mode_t reset_mode, plp_aperta2_phymod_reset_direction_t direction)
{        
    
    return _plp_aperta2_core_reset_set(core, reset_mode, direction);
}

int plp_aperta2_core_reset_get(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_reset_mode_t reset_mode, plp_aperta2_phymod_reset_direction_t* direction)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}


int plp_aperta2_core_firmware_info_get(const plp_aperta2_phymod_core_access_t* core, plp_aperta2_phymod_core_firmware_info_t* fw_info)
{        
    
    return _plp_aperta2_core_firmware_info_get(&core->access, fw_info);
}


int plp_aperta2_phy_firmware_core_config_set(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_firmware_core_config_t fw_core_config)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}

int plp_aperta2_phy_firmware_core_config_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_firmware_core_config_t* fw_core_config)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}


int plp_aperta2_phy_firmware_lane_config_set(const plp_aperta2_phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_lane_config)
{        
    return plp_aperta2_tscp_phy_firmware_lane_config_set(phy, fw_lane_config);
    
}

int plp_aperta2_phy_firmware_lane_config_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_firmware_lane_config_t* fw_lane_config)
{        
    
    return plp_aperta2_tscp_phy_firmware_lane_config_get(phy, fw_lane_config);
}


int plp_aperta2_core_pll_sequencer_restart(const plp_aperta2_phymod_core_access_t* core, uint32_t flags, plp_aperta2_phymod_sequencer_operation_t operation)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}


int plp_aperta2_phy_rx_restart(const plp_aperta2_phymod_phy_access_t* phy)
{        
    
    return PHYMOD_E_UNAVAIL;
}


int plp_aperta2_phy_polarity_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_polarity_t* polarity)
{        
    unsigned int lane_index = 0;
    plp_aperta2_phymod_polarity_t polarity_temp;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&polarity_temp, polarity, sizeof(plp_aperta2_phymod_polarity_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            if (polarity->tx_polarity != 0xFFFFFFFF) {
                polarity_temp.tx_polarity = (polarity->tx_polarity & (1 << lane_index)) >> lane_index;
            }
            if (polarity->rx_polarity != 0xFFFFFFFF) {
                polarity_temp.rx_polarity = (polarity->rx_polarity & (1 << lane_index)) >> lane_index;
            }
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_tscp_phy_polarity_set(&phy_temp, &polarity_temp));
        }
    }

    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_polarity_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_polarity_t* polarity)
{        
    unsigned int lane_index = 0;
    plp_aperta2_phymod_polarity_t polarity_temp;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    polarity->rx_polarity = 0;
    polarity->tx_polarity = 0;

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_MEMSET(&polarity_temp, 0, sizeof(polarity_temp));
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_tscp_phy_polarity_get(&phy_temp, &polarity_temp));
            polarity->rx_polarity |= ((polarity_temp.rx_polarity ? 1 : 0) << lane_index);
            polarity->tx_polarity |= ((polarity_temp.tx_polarity ? 1 : 0) << lane_index);
        }
    }
        
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_pam4_tx_set(const plp_aperta2_phymod_phy_access_t* phy, const phymod_pam4_tx_t* tx)
{       
    unsigned int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_IF_ERR_RETURN(plp_aperta2_tscp_phy_tx_set(&phy_temp, tx));
        }
    }
    return PHYMOD_E_NONE;

}

int plp_aperta2_phy_pam4_tx_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_pam4_tx_t* tx)
{        
    unsigned int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_tscp_phy_tx_get(&phy_temp, tx));
            break;
        }
    }
    return PHYMOD_E_NONE;
   
}


int plp_aperta2_phy_media_type_tx_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_media_typed_t media, plp_aperta2_phymod_tx_t* tx)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}


int plp_aperta2_phy_rx_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_rx_t* rx)
{        
    unsigned int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_peregrine5_pc_phy_rx_set(&phy_temp, rx));
        }
    }
    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_rx_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_rx_t* rx)
{        
    unsigned int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_peregrine5_pc_phy_rx_get(&phy_temp, rx));
            break;
        }
    }
   
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_rx_adaptation_resume(const plp_aperta2_phymod_phy_access_t* phy)
{        
    unsigned int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_peregrine5_pc_phy_rx_adaptation_resume(&phy_temp));
        }
    }
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_reset_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_phy_reset_t* reset)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}

int plp_aperta2_phy_reset_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_phy_reset_t* reset)
{        
    
    return PHYMOD_E_UNAVAIL;
}


int plp_aperta2_phy_power_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_phy_power_t* power)
{       
    if (power->rx == phymodPowerOff && power->tx == phymodPowerOff) {
         PHYMOD_IF_ERR_RETURN(
             plp_aperta2_phymod_tscp_driver.f_phymod_port_enable_set(phy,0));
    } else if (power->rx == phymodPowerOn && power->tx == phymodPowerOn){
         PHYMOD_IF_ERR_RETURN(
             plp_aperta2_phymod_tscp_driver.f_phymod_port_enable_set(phy,1));
    } else {
        return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_phy_power_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_phy_power_t* power)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}


int plp_aperta2_phy_tx_lane_control_set(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_phy_tx_lane_control_t tx_control)
{        
    unsigned int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_tscp_phy_tx_lane_control_set(&phy_temp, tx_control));
        }
    }

    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_tx_lane_control_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_phy_tx_lane_control_t* tx_control)
{        
    unsigned int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_tscp_phy_tx_lane_control_get(&phy_temp, tx_control));
            break;
        }
    }
   
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_rx_lane_control_set(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_phy_rx_lane_control_t rx_control)
{        
    unsigned int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_tscp_phy_rx_lane_control_set(&phy_temp, rx_control));
        }
    }

    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_rx_lane_control_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_phy_rx_lane_control_t* rx_control)
{        
     unsigned int lane_index = 0;
    plp_aperta2_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (lane_index = 0; lane_index < APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_tscp_phy_rx_lane_control_get(&phy_temp, rx_control));
            break;
        }
    }
    
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_fec_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t enable)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}

int plp_aperta2_phy_fec_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* enable)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}


int plp_aperta2_phy_interface_config_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta2_phymod_phy_inf_config_t* config)
{        
    plp_aperta2_phymod_phy_inf_config_t config_temp;
    int pif, side = 0;
    aperta2_device_aux_modes_t auxmode;
    plp_aperta2_phymod_phy_access_t temp_access;

    if (!config->device_aux_modes) {
        PHYMOD_DEBUG_ERROR(("Aux mode cannot be NULL\n"));
        return PHYMOD_E_PARAM;
    }

    if (APERTA2_IS_SYSTEM_SIDE(phy)) {
        if (phy->access.addr < APERTA2_MAX_PHY) {
            PHYMOD_MEMCPY(&_plp_aperta2_sys_port_config[phy->access.addr].sys_port_config, config, sizeof(plp_aperta2_phymod_phy_inf_config_t));
            PHYMOD_MEMCPY(&_plp_aperta2_sys_port_config[phy->access.addr].sys_aux_mode, config->device_aux_modes, sizeof(aperta2_device_aux_modes_t));
            _plp_aperta2_sys_port_config[phy->access.addr].lane_mask = phy->access.lane_mask;
            return PHYMOD_E_NONE;
        }
        PHYMOD_DEBUG_ERROR(("Invalid PHY:%x\n", phy->access.addr ));
        return PHYMOD_E_RESOURCE;
    } else {
        for (side = phymodPortLocSys; side >= phymodPortLocLine; side --) {
            if (side == phymodPortLocSys) {
                PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
                temp_access.port_loc = phymodPortLocSys;
                temp_access.access.lane_mask = _plp_aperta2_sys_port_config[phy->access.addr].lane_mask;
                PHYMOD_MEMCPY(&auxmode, &_plp_aperta2_sys_port_config[phy->access.addr].sys_aux_mode, sizeof(aperta2_device_aux_modes_t));
                PHYMOD_MEMCPY(&config_temp, &_plp_aperta2_sys_port_config[phy->access.addr].sys_port_config , sizeof(plp_aperta2_phymod_phy_inf_config_t));
                config_temp.device_aux_modes = (void*)&auxmode;
            } else {
                PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
                auxmode = *(aperta2_device_aux_modes_t*)config->device_aux_modes;
                PHYMOD_MEMCPY(&config_temp, config, sizeof(plp_aperta2_phymod_phy_inf_config_t));
            }
            pif = config_temp.interface_type;
            if ((pif == phymodInterfaceSFI) ||
                    (pif == phymodInterfaceSR4) ||
                    (pif == phymodInterfaceLR4) ||
                    (pif == phymodInterfaceLR) ||
                    (pif == phymodInterfaceER4) ||
                    (pif == phymodInterfaceER) ||
                    (pif == phymodInterfaceSR) ||
                    (pif == phymodInterfaceLR2) ||
                    (pif == phymodInterfaceER2) ||
                    (pif == phymodInterfaceSR2)) {
                PHYMOD_INTF_MODES_FIBER_SET(&config_temp);
            } else if (pif == phymodInterfaceCR ||
                    pif == phymodInterfaceCR2 ||
                    pif == phymodInterfaceCR4 ) {
                PHYMOD_INTF_MODES_COPPER_SET(&config_temp);
            } else {
                PHYMOD_INTF_MODES_BACKPLANE_SET(&config_temp);
            }
            /* PM configuration*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_interface_config_set(&temp_access, flags, &config_temp, phy->access.lane_mask, config));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_sw_intf_set(&temp_access, config_temp.interface_type));
            if (auxmode.failover_config.lane_map != 0) {
                plp_aperta2_phymod_phy_access_t phy_cpy;
                PHYMOD_MEMCPY(&phy_cpy, &temp_access, sizeof(plp_aperta2_phymod_phy_access_t));
                phy_cpy.access.lane_mask = auxmode.failover_config.lane_map; 
                PHYMOD_IF_ERR_RETURN(plp_aperta2_sw_intf_set(&phy_cpy, config_temp.interface_type));
            }
        }
    }

    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_interface_config_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_ref_clk_t ref_clock, plp_aperta2_phymod_phy_inf_config_t* config)
{        
    uint8_t lane_index  =0;

    /* PM configuration*/
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_interface_config_get(phy, flags, config));

    for (lane_index =0; lane_index< APERTA2_MAX_NUM_LANES; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_sw_intf_get(phy, lane_index, &config->interface_type));
            return PHYMOD_E_NONE;
        }
    }
        
    return PHYMOD_E_NONE;
}


int plp_aperta2_phy_cl72_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t cl72_en)
{        
    
    return plp_aperta2_tscp_phy_cl72_set(phy, cl72_en);
    
}

int plp_aperta2_phy_cl72_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* cl72_en)
{        
    
    return plp_aperta2_tscp_phy_cl72_get(phy, cl72_en);
}

int plp_aperta2_phy_cl72_status_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_cl72_status_t* status)
{        
    
    return plp_aperta2_tscp_phy_cl72_status_get(phy, status);
    
}


int plp_aperta2_phy_autoneg_ability_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_autoneg_ability_t* an_ability_set_type)
{        
    phymod_autoneg_advert_abilities_t loc_phy_ability;
    phymod_autoneg_advert_ability_t advert_ability;
    portmod_port_speed_ability_t port_abilities;
    plp_aperta2_phymod_phy_inf_config_t config;
    int num_abilities = 0;
    aperta2_device_aux_modes_t auxmode;
    int port = 0, new_port = 0, pm_ability = 0;
    uint32_t gpreg13_14 = 0/*KRS/CRS*/  /* SWGPREG 13&14*/, reg_addr = 0;
    int number_of_ability = 0, capability = 0, capability_ext = 0 ;
    uint32_t sw_an_mode = 0; /*SWGPREG 15*/
 
    PHYMOD_MEMSET(&auxmode, 0, sizeof(aperta2_device_aux_modes_t));
    APERTA2_UPDATE_PM_INFO(phy->access.addr, phy);
    if (an_ability_set_type->an_cl72 == 0) {
        PHYMOD_DEBUG_ERROR(("an_cl72 must be 1\n"));
        return PHYMOD_E_PARAM;
    }
    config.device_aux_modes = &auxmode;
    loc_phy_ability.autoneg_abilities = &advert_ability;
    loc_phy_ability.num_abilities = 1;
    number_of_ability = plp_aperta2_phymod_count_set_bits(an_ability_set_type->an_cap);
    number_of_ability += plp_aperta2_phymod_count_set_bits(an_ability_set_type->an_cap_ext);
    capability = an_ability_set_type->an_cap;
    capability_ext = an_ability_set_type->an_cap_ext;
 
    /* Check for invalid combination*/
    /* MSA Capability with IEEE FEC*/
    if((PHYMOD_AN_CAP1_MSA_200G_CR4_KR4_GET(capability_ext) && (an_ability_set_type->an_fec == 0x80)) ||
       (PHYMOD_AN_CAP1_MSA_100G_CR2_KR2_GET(capability_ext) && (an_ability_set_type->an_fec == 0x20)) ||
       /* coverity[copy_paste_error] */
       (PHYMOD_AN_CAP_50G_CR1_KR1_GET(capability) && (an_ability_set_type->an_fec == 0x20))) {
        PHYMOD_DEBUG_ERROR(("Invalid FEC and Capability\n"));
        return PHYMOD_E_PARAM;
    }
 
    /* IEEE Capability with MSA FEC*/
    if((PHYMOD_AN_CAP_200G_CR4_KR4_GET(capability) && (an_ability_set_type->an_fec == 0x100)) ||
       (PHYMOD_AN_CAP_100G_CR2_KR2_GET(capability) && (an_ability_set_type->an_fec == 0x40)) ||
       (PHYMOD_AN_CAP_50G_CR_KR_GET(capability) && (an_ability_set_type->an_fec == 0x40))) {
        PHYMOD_DEBUG_ERROR(("Invalid FEC and Capability\n"));
        return PHYMOD_E_PARAM;
    }

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_phy_interface_config_get(phy, 0, phymodRefClk312Mhz, &config));
   
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_CTRL_SWGPREG15r, &sw_an_mode));

    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    while (number_of_ability) {
        PHYMOD_MEMSET(&port_abilities, 0, sizeof(port_abilities));
        PHYMOD_MEMSET(&advert_ability, 0, sizeof(phymod_autoneg_advert_ability_t));
        pm_ability = 0;
        advert_ability.speed = config.data_rate ;
        advert_ability.resolved_num_lanes = plp_aperta2_phymod_count_set_bits(phy->access.lane_mask);
        /* Mark FEC as none and update it later*/
        advert_ability.fec = an_ability_set_type->an_fec;
        advert_ability.pause = (an_ability_set_type->capabilities >> 6);
        if (config.interface_type == phymodInterfaceCR || config.interface_type == phymodInterfaceCR2 ||
                config.interface_type == phymodInterfaceCR4) {
            advert_ability.medium = phymodFirmwareMediaTypeCopperCable; 
        } else {
            advert_ability.medium = phymodFirmwareMediaTypePcbTraceBackPlane; 
        }
        number_of_ability--;
        if (PHYMOD_AN_CAP_25G_KRS1_GET(capability) ||
            PHYMOD_AN_CAP_25G_CRS1_GET(capability)) {
            advert_ability.channel = PORTMOD_PORT_PHY_CHANNEL_SHORT;
        } else {
            advert_ability.channel = PORTMOD_PORT_PHY_CHANNEL_LONG;
        }
        if (PHYMOD_AN_CAP_25G_KR1_GET    (capability) || PHYMOD_AN_CAP_25G_CR1_GET(capability) ||
             PHYMOD_AN_CAP_50G_CR2_GET(capability) || PHYMOD_AN_CAP_50G_KR2_GET(capability) ||
            (PHYMOD_AN_CAP_400G_CR8_KR8_GET(capability) && advert_ability.resolved_num_lanes == 8) ||
            (PHYMOD_AN_CAP1_MSA_200G_CR4_KR4_GET(capability_ext) && (an_ability_set_type->an_fec == 0x100)) ||
            (PHYMOD_AN_CAP1_MSA_100G_CR2_KR2_GET(capability_ext) && (an_ability_set_type->an_fec == 0x40)) ||
            (PHYMOD_AN_CAP_50G_CR1_KR1_GET(capability) && (an_ability_set_type->an_fec == 0x40))) {
            advert_ability.an_mode = phymod_AN_MODE_MSA;
            /* Set 1 for MSA*/
            sw_an_mode |= (1 << (port));
            /* Clearing to check other ability*/
            PHYMOD_AN_CAP_25G_KR1_CLR(capability);    
            PHYMOD_AN_CAP_25G_CR1_CLR(capability);
            PHYMOD_AN_CAP_50G_CR2_CLR(capability);
            PHYMOD_AN_CAP_50G_KR2_CLR(capability);

        } else {
            /* Set 0 for IEEE*/
            sw_an_mode &= ~(1 << port);
            advert_ability.an_mode = phymod_AN_MODE_CL73;
        }
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_CTRL_SWGPREG15r, sw_an_mode));

        if (advert_ability.speed == APERTA2_SPEED_25G) {
            if (port < 8) {
                reg_addr = BCM_APERTA2_DIRECT_CTRL_SWGPREG13r;
                new_port = port;
            } else {
                reg_addr = BCM_APERTA2_DIRECT_CTRL_SWGPREG14r;
                new_port = port-8;
            }
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_direct_reg_read (phy, reg_addr, &gpreg13_14));
            if (advert_ability.an_mode == phymod_AN_MODE_CL73) {
                /* 0 for CL73*/
                gpreg13_14 &= ~(3 << (new_port*2));
                if(advert_ability.channel == PORTMOD_PORT_PHY_CHANNEL_SHORT) {
                    /* 2 for KRS*/
                    if(PHYMOD_AN_CAP_25G_KRS1_GET(an_ability_set_type->an_cap)) {
                        gpreg13_14 |= (2 << (new_port *2));
                    }
                }
                if (PHYMOD_AN_CAP_25G_KR_GET(capability) && an_ability_set_type->an_fec == 0x8000) {
                    gpreg13_14 |= (2 << (new_port *2));
                } 
            } else {
                /* 1 for BAM/MSA*/
                gpreg13_14 |= (1 << (new_port *2));
            }
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_direct_reg_write (phy, reg_addr, gpreg13_14));
        }
        if (an_ability_set_type->an_fec == 0) {
            advert_ability.fec = 0x0;
        } else if(an_ability_set_type->an_fec == 1 || an_ability_set_type->an_fec == 2) {
            advert_ability.fec =  phymod_fec_CL74;
        } else if(an_ability_set_type->an_fec == 4 || an_ability_set_type->an_fec == 8) {
            advert_ability.fec =  phymod_fec_CL91;
        } else if(an_ability_set_type->an_fec == 0x80) {
            advert_ability.fec =  phymod_fec_RS544_2XN;
        } else if(an_ability_set_type->an_fec == 0x20) {
            advert_ability.fec =  phymod_fec_RS544;
        } else if(an_ability_set_type->an_fec == 0x40) {
            advert_ability.fec =  phymod_fec_RS272;
        } else if(an_ability_set_type->an_fec == 0x100) {
            advert_ability.fec =  phymod_fec_RS272_2XN;
        } else {
            advert_ability.fec = phymod_fec_None;
        }
        (void)pm_ability;
        PHYMOD_IF_ERR_RETURN(
                _plp_aperta2_pm8x100_gen2_port_phy_to_port_ability
                 (&loc_phy_ability, &num_abilities, &port_abilities));
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_portmod_port_autoneg_ability_advert_set(0, (phy->access.addr << 16), 1, &port_abilities));
    }
        
    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_autoneg_ability_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_autoneg_ability_t* an_ability_get_type)
{        
    
    portmod_port_speed_ability_t port_abilities[APERTA2_MAX_NO_ABILITY];
    phymod_autoneg_advert_abilities_t loc_phy_ability;
    phymod_autoneg_advert_ability_t advert_ability[APERTA2_MAX_NO_ABILITY];
    int max_ability = 0, port = 0, new_port = 0, an_mode =0, index = 0;
    unsigned int gpreg13_14 = 0, reg_addr = 0;
 
    PHYMOD_MEMSET(advert_ability, 0 , sizeof(phymod_autoneg_advert_ability_t)*APERTA2_MAX_NO_ABILITY);
    APERTA2_UPDATE_PM_INFO(phy->access.addr, phy);
    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    loc_phy_ability.autoneg_abilities = advert_ability;
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_portmod_port_autoneg_ability_advert_get(0, (phy->access.addr << 16), 2, port_abilities, &max_ability));

    PHYMOD_IF_ERR_RETURN(
        _plp_aperta2_pm8x100_gen2_port_port_to_phy_ability(max_ability, port_abilities, &loc_phy_ability));
    /* Both IEEE/MSA An mode is set*/
    if (port_abilities[0].an_mode == 0xb) {
        an_mode = 1;
    }
    if (port < 8) {
        reg_addr = BCM_APERTA2_DIRECT_CTRL_SWGPREG13r;
        new_port = port;
    } else {
        reg_addr = BCM_APERTA2_DIRECT_CTRL_SWGPREG14r;
        new_port = port-8;
    }

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_direct_reg_read(phy, reg_addr , &gpreg13_14));
    PHYMOD_DEBUG_INFO(("Number of abilities:%d\n", ((max_ability > APERTA2_MAX_NO_ABILITY) ? APERTA2_MAX_NO_ABILITY : max_ability)));
    for (index = 0; index < ((max_ability > APERTA2_MAX_NO_ABILITY) ? APERTA2_MAX_NO_ABILITY : max_ability) ; index++) {
        if (loc_phy_ability.autoneg_abilities[index].speed == 10000) {
            PHYMOD_AN_CAP_10G_KR_SET(an_ability_get_type->an_cap);
        }
        if (loc_phy_ability.autoneg_abilities[index].speed == 25000) {
            if (an_mode || loc_phy_ability.autoneg_abilities[index].an_mode == phymod_AN_MODE_CL73) {
                if (loc_phy_ability.autoneg_abilities[index].channel == phymod_channel_short) {
                    if (gpreg13_14 & (3 << (new_port*2))) {
                        PHYMOD_AN_CAP_25G_KRS1_SET(an_ability_get_type->an_cap);
                    } else {
                        PHYMOD_AN_CAP_25G_CRS1_SET(an_ability_get_type->an_cap);
                    }
                } else {
                    if (loc_phy_ability.autoneg_abilities[index].medium == phymodFirmwareMediaTypeCopperCable) {
                        PHYMOD_AN_CAP_25G_CR_SET(an_ability_get_type->an_cap);
                    } else {
                        PHYMOD_AN_CAP_25G_KR_SET(an_ability_get_type->an_cap);
                    }
                }
            }
            if (an_mode || loc_phy_ability.autoneg_abilities[index].an_mode == phymod_AN_MODE_MSA) {
                if (loc_phy_ability.autoneg_abilities[index].medium == phymodFirmwareMediaTypeCopperCable) {
                    PHYMOD_AN_CAP_25G_CR1_SET(an_ability_get_type->an_cap);
                } else {
                    PHYMOD_AN_CAP_25G_KR1_SET(an_ability_get_type->an_cap);
                }
            }
        }
        if (loc_phy_ability.autoneg_abilities[index].speed == 50000) {
            if (loc_phy_ability.autoneg_abilities[index].resolved_num_lanes == 2) {
                if (loc_phy_ability.autoneg_abilities[index].medium == phymodFirmwareMediaTypeCopperCable) {
                    PHYMOD_AN_CAP_50G_CR2_SET(an_ability_get_type->an_cap);
                } else {
                    PHYMOD_AN_CAP_50G_KR2_SET(an_ability_get_type->an_cap);
                }
            } else {
                if (loc_phy_ability.autoneg_abilities[index].fec == phymod_fec_RS272) {
                    PHYMOD_AN_CAP_50G_CR1_KR1_SET(an_ability_get_type->an_cap);
                } else {
                    PHYMOD_AN_CAP_50G_CR_KR_SET(an_ability_get_type->an_cap);
                }
            }
        }
        if (loc_phy_ability.autoneg_abilities[index].speed == 100000) {
            if (loc_phy_ability.autoneg_abilities[index].resolved_num_lanes == 4) {
                if (loc_phy_ability.autoneg_abilities[index].medium == phymodFirmwareMediaTypeCopperCable) {
                    PHYMOD_AN_CAP_100G_CR4_SET(an_ability_get_type->an_cap);
                } else {
                    PHYMOD_AN_CAP_100G_KR4_SET(an_ability_get_type->an_cap);
                }
            } else if (loc_phy_ability.autoneg_abilities[index].resolved_num_lanes == 2){
                if (loc_phy_ability.autoneg_abilities[index].fec  ==  phymod_fec_RS272) {
                    PHYMOD_AN_CAP1_MSA_100G_CR2_KR2_SET(an_ability_get_type->an_cap_ext);
                } else {
                    PHYMOD_AN_CAP_100G_CR2_KR2_SET(an_ability_get_type->an_cap);
                }
            } else {
                PHYMOD_AN_CAP1_100G_CR1_KR1_SET(an_ability_get_type->an_cap_ext);
            }
        }
        if (loc_phy_ability.autoneg_abilities[index].speed == 200000) {
            if (loc_phy_ability.autoneg_abilities[index].resolved_num_lanes == 4) {
                if (loc_phy_ability.autoneg_abilities[index].fec  ==  phymod_fec_RS272_2XN) {
                    PHYMOD_AN_CAP1_MSA_200G_CR4_KR4_SET(an_ability_get_type->an_cap_ext);
                } else {
                    PHYMOD_AN_CAP_200G_CR4_KR4_SET(an_ability_get_type->an_cap);
                }
            } else {
                PHYMOD_AN_CAP1_200G_CR2_KR2_SET(an_ability_get_type->an_cap_ext);
            }
        }
        if (loc_phy_ability.autoneg_abilities[index].speed == 400000) {
            if (loc_phy_ability.autoneg_abilities[index].resolved_num_lanes == 8) {
                PHYMOD_AN_CAP_400G_CR8_KR8_SET(an_ability_get_type->an_cap);
            } else {
                PHYMOD_AN_CAP1_400G_CR4_KR4_SET(an_ability_get_type->an_cap_ext);
            }
        }

        if (loc_phy_ability.autoneg_abilities[index].speed == 40000) {
            if (loc_phy_ability.autoneg_abilities[index].medium == phymodFirmwareMediaTypeCopperCable) {
                PHYMOD_AN_CAP_40G_CR4_SET(an_ability_get_type->an_cap);
            } else {
                PHYMOD_AN_CAP_40G_KR4_SET(an_ability_get_type->an_cap);
            }
        }
        if (loc_phy_ability.autoneg_abilities[index].fec == phymod_fec_None) {
            an_ability_get_type->an_fec = 0x8000;
        }
        if (loc_phy_ability.autoneg_abilities[index].fec  == phymod_fec_CL74) {
            an_ability_get_type->an_fec |= 2;
        } 
        if (loc_phy_ability.autoneg_abilities[index].fec  ==  phymod_fec_CL91) {
            an_ability_get_type->an_fec |= 8;
        }
        if (loc_phy_ability.autoneg_abilities[index].fec  ==  phymod_fec_RS544) {
            an_ability_get_type->an_fec |= 0x20;
        }
        if (loc_phy_ability.autoneg_abilities[index].fec  ==  phymod_fec_RS272) {
            an_ability_get_type->an_fec |= 0x40;
        }
        if (loc_phy_ability.autoneg_abilities[index].fec  ==  phymod_fec_RS544_2XN) {
            an_ability_get_type->an_fec |= 0x80;
        }
        if (loc_phy_ability.autoneg_abilities[index].fec  ==  phymod_fec_RS272_2XN) {
            an_ability_get_type->an_fec |= 0x100;
        }
        an_ability_get_type->capabilities |= (advert_ability[index].pause) << 6;
    }
    an_ability_get_type->an_cl72=1;

    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_autoneg_remote_ability_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_autoneg_ability_t* an_ability_get_type)
{        

    return _plp_aperta2_phy_autoneg_remote_ability_get(phy, an_ability_get_type) ;
}


int plp_aperta2_phy_autoneg_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_autoneg_control_t* an)
{        
    plp_aperta2_phymod_autoneg_control_t an_loc;
    int speed = 0, port = 0, new_port = 0;
    unsigned int gpreg13_14 = 0;
    uint32_t sw_an_mode = 0; /*SWGPREG 15*/
    int lane_map = 0; 
    unsigned int reg_addr = 0, num_lanes = plp_aperta2_phymod_count_set_bits(phy->access.lane_mask);

 
    PHYMOD_MEMCPY(&an_loc, an, sizeof(plp_aperta2_phymod_autoneg_control_t));
    APERTA2_UPDATE_PM_INFO(phy->access.addr, phy);
    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_info_port_speed_get(phy, port, &speed, &lane_map));
    if (port < 8) {
        reg_addr = BCM_APERTA2_DIRECT_CTRL_SWGPREG13r;
        new_port = port;
    } else {
        reg_addr = BCM_APERTA2_DIRECT_CTRL_SWGPREG14r;
        new_port = port-8;
    }

    if (speed == 25000 || speed == 400000 || 
        (speed == 200000 && (num_lanes == 4)) ||
        (speed == 100000 && (num_lanes == 2)) ||
        (speed == 50000 && (num_lanes == 1))) {
        an_loc.an_mode = phymod_AN_MODE_CL73;
        PHYMOD_IF_ERR_RETURN(
               plp_aperta2_direct_reg_read(phy, reg_addr, &gpreg13_14));
        if ((gpreg13_14 & (3 << (new_port *2))) == (1 << new_port*2)) {
            an_loc.an_mode = phymod_AN_MODE_CL73_MSA;
        }
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_CTRL_SWGPREG15r, &sw_an_mode));
        if ((sw_an_mode & (1 << port)) == (1 << port)) {
            an_loc.an_mode = phymod_AN_MODE_CL73_MSA;
        }
    } else {
        an_loc.an_mode = phymod_AN_MODE_CL73;
    }
    
    if (speed == 10000 || speed == 25000 || speed == 50000) {
        if (plp_aperta2_phymod_count_set_bits(phy->access.lane_mask) == 0x2) {
            an_loc.an_mode = phymod_AN_MODE_CL73_MSA;
        }
        an_loc.num_lane_adv = 1;
    } else {
        an_loc.num_lane_adv = plp_aperta2_phymod_count_set_bits(phy->access.lane_mask);
    }
    an_loc.enable = an->enable;

    return plp_aperta2_portmod_port_autoneg_set(0, (phy->access.addr << 16), 0, &an_loc);
}

int plp_aperta2_phy_autoneg_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_autoneg_control_t* an, uint32_t* an_done)
{
    APERTA2_UPDATE_PM_INFO(phy->access.addr, phy);
    return plp_aperta2_tscp_phy_autoneg_get(phy, an, an_done);
} 

int plp_aperta2_phy_autoneg_status_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_autoneg_status_t* status)
{        
    
    return PHYMOD_E_UNAVAIL;
    
}

#ifdef APERTA2_FRC_SERBOOT_LOW
static int aperta2_eeprom_put_serboot_low(const plp_aperta2_phymod_core_access_t* core,
        BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_t boot_por)
{
    int retry_cnt = APERTA2_MICRO_RETRY_COUNT;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_t boot;
    uint32_t data1 = 0;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_t gen_ctrl1;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGOUTr_t msg_out;

    /* This delay is needed here because after hard reset, 
     * Boot loader set serboot busy as "0", 
     * HW needs some time to set serboot busy*/
    PHYMOD_USLEEP(12000);
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_MST_MSGOUTr(&core->access, &msg_out));
    /* Serboot Busy will not get cleared, if there is a download
       error*/
    if (APERTA2_MSGOUT_HDR_ERR == msg_out.v[0]) {
        /* Wait till serboot busy is clear*/
        do {
            PHYMOD_USLEEP(1000);
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_BOOTr(&core->access, &boot));
            data1 = BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        } while((data1 != 0) && (--retry_cnt));
        if ((retry_cnt <= 0) && (data1 != 0)) {
            PHYMOD_DEBUG_ERROR(("Serboot Busy is not 0 \n"));
            return PHYMOD_E_INTERNAL;
        }
    }
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por,0);
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET(boot_por,1);
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET(boot_por,0);
    PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_MICRO_BOOT_BOOT_PORr(&core->access, boot_por));

    /* Apply soft reset*/
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GEN_CONTROL1r(&core->access, &gen_ctrl1));
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(gen_ctrl1, 0);
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, gen_ctrl1));

    return PHYMOD_E_NONE;
}
#endif

int plp_aperta2_core_init(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config, const plp_aperta2_phymod_core_status_t* core_status)
{        
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t  bcast_enable;
    BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_t boot_por;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_t boot;
    uint32_t data1 = 0;
    int retry_cnt = APERTA2_MICRO_RETRY_COUNT;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGOUTr_t msg_out;

    PHYMOD_MEMSET(&bcast_enable, 0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t));
    PHYMOD_MEMSET(&boot_por, 0, sizeof(BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_t));
    PHYMOD_MEMSET(&boot, 0, sizeof(BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_t));

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_init_db(core, init_config));
    if (init_config->firmware_load_method == phymodFirmwareLoadMethodNone) {
        plp_aperta2_phymod_core_firmware_info_t fw_info;
        /* This delay is needed here because after hard reset, 
         * Boot loader set serboot busy as "0", 
         * HW needs some time to set serboot busy*/
        PHYMOD_USLEEP(15000);
#if 0
        do {
            PHYMOD_USLEEP(1000);
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_MST_MSGOUTr(&core->access, &msg_out));
            if (APERTA2_MSGOUT_HDR_ERR == msg_out.v[0]) {
                PHYMOD_DEBUG_INFO(("PHY-%d: Bad image header from SPIROM...\n", core->access.addr));
                break ;
            }
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_BOOTr(&core->access, &boot));
            data1 = BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        } while((data1 != 0) && (--retry_cnt));
        if ((retry_cnt <= 0) && (data1 != 0)) {
            PHYMOD_DEBUG_ERROR(("Serboot Busy is not 0 \n"));
            return PHYMOD_E_INTERNAL;
        }
#endif
        PHYMOD_IF_ERR_RETURN(plp_aperta2_ded_wka(core, init_config));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_init(core, init_config, core_status));
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_check_fw_download_status(core, init_config->firmware_load_method));
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_core_firmware_info_get(&core->access, &fw_info));

        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_is_fw_dloaded_set(core->access.addr, 1));

        PHYMOD_CRIT_INFO(("PHY:0x%x FW version:0x%x\n", core->access.addr, fw_info.fw_version));
    }

    if (PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN( _plp_aperta2_core_reset_set(core, phymodResetModeHard, 0));
#ifdef APERTA2_FRC_SERBOOT_LOW
        /*Below code is not needed as plp_aperta2_ded_wka, perform serboot BUSY clear*/
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodProgEEPROM) {
            PHYMOD_IF_ERR_RETURN(
                 BCM_APERTA2_DIRECT_READ_MICRO_BOOT_BOOT_PORr(&core->access, &boot_por));
            if(BCM_APERTA2_DIRECT_MICRO_BOOT_BOOT_PORr_SERBOOTf_GET(boot_por)) {
                PHYMOD_IF_ERR_RETURN(
                    aperta2_eeprom_put_serboot_low(core, boot_por));
            }
        }
#endif
        PHYMOD_IF_ERR_RETURN(plp_aperta2_ded_wka(core, init_config));
        /* This delay is needed here because after hard reset, 
         * Boot loader set serboot busy as "0", 
         * HW needs some time to set serboot busy*/
        PHYMOD_USLEEP(12000);

        do {
            PHYMOD_USLEEP(7000);
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_MST_MSGOUTr(&core->access, &msg_out));
            if (APERTA2_MSGOUT_HDR_ERR == msg_out.v[0]) {
                PHYMOD_DEBUG_INFO(("PHY-%d: Bad image header from SPIROM...\n", core->access.addr));
                break ;
            }
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_BOOTr(&core->access, &boot));
            data1 = BCM_APERTA2_DIRECT_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        } while((data1 != 0) && (--retry_cnt));
        if ((retry_cnt <= 0) && (data1 != 0)) {
            PHYMOD_DEBUG_ERROR(("Serboot Busy is not 0 \n"));
            return PHYMOD_E_INTERNAL;
        }
        /* Adding this to make sure that FW init completed*/
        PHYMOD_USLEEP(5000);
        /*Initlize PM*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_init(core, init_config, core_status));
    } else if (PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_GET(init_config)) {
        /* Bcast enable here*/
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, &bcast_enable));
        BCM_APERTA2_DIRECT_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET(bcast_enable, 1);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, bcast_enable));
    } else if (PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD_GET(init_config)) {
        /* Download FW first*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dload_fw(core, init_config));
         
        /* Bcast disable here*/
        BCM_APERTA2_DIRECT_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET(bcast_enable, 0);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, bcast_enable));
    } else if (PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_GET(init_config)) {
        plp_aperta2_phymod_core_firmware_info_t fw_info;
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_check_fw_download_status(core, init_config->firmware_load_method));
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_core_firmware_info_get(&core->access, &fw_info));

        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_is_fw_dloaded_set(core->access.addr, 1));

        PHYMOD_CRIT_INFO(("PHY:0x%x FW version:0x%x CRC:0x%x\n", core->access.addr, fw_info.fw_version, fw_info.fw_crc));

    } else if(PHYMOD_CORE_INIT_F_FW_LOAD_END_GET(init_config)){
        /* Not required for now*/
    }
    return PHYMOD_E_NONE;

}

int plp_aperta2_phy_init(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_phy_init_config_t* init_config)
{        
    aperta2_config_phy_t config_phy;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG00r_t  sw_reg00;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG12r_t  sw_reg12;

    unsigned int octal = 0;
    aperta2_switch_dpclk_t switch_dp_clk;
    aperta2_fw_init_t *fw_init_param = (aperta2_fw_init_t*)(init_config->interface.device_aux_modes);

    if (fw_init_param == NULL) {
        PHYMOD_DEBUG_ERROR(("FW init param cannot be NULL\n"));
        return PHYMOD_E_PARAM;
    }

    PHYMOD_MEMSET(&sw_reg00, 0, sizeof(sw_reg00));

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_0Fr , 1)); 


    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_lane_swap_set(phy, &fw_init_param->sys_lane_map , &fw_init_param->line_lane_map));
    
    /* Put macsec bypass*/
    config_phy.macsec_option = fw_init_param->macsec_option;
    config_phy.ptp_option = fw_init_param->ptp_option;
    sw_reg00.v[0] = (fw_init_param->ptp_option << 4) | fw_init_param->macsec_option;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_send_fw_msg(phy, 0, NULL, APERTA2_MSG_CONFIG_PHY, &config_phy, 0));

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_configure_octal(phy, APERTA2_OCTAL0, APERTA2_OCTAL1, init_config));

    /* Store init DP clk in SW reg*/
    for (octal = APERTA2_OCTAL0; octal <= APERTA2_OCTAL1; octal++) {
        switch_dp_clk.OctalSel = octal;
        if (octal == APERTA2_OCTAL0) {
            if (fw_init_param->octal0.line_vco > fw_init_param->octal0.sys_vco) {
                switch_dp_clk.ClkSel = APERTA2_LINE_DP_CLK;
            } else {
                switch_dp_clk.ClkSel = APERTA2_SYS_DP_CLK;
            }
        } else {
            if (fw_init_param->octal1.line_vco > fw_init_param->octal1.sys_vco) {
                switch_dp_clk.ClkSel = APERTA2_LINE_DP_CLK;
            } else {
                switch_dp_clk.ClkSel = APERTA2_SYS_DP_CLK;
            }
        }
        sw_reg00.v[0] |= ((switch_dp_clk.ClkSel) << (8 + (octal*4)));
    }
    /* To retreive macsec option, PTP and Clock Sel option during soft reset and Warmboot*/
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG00r(&phy->access, sw_reg00)); 

    /* Setting watermark for accounting purposes */
    BCM_APERTA2_DIRECT_CTRL_SWGPREG12r_SET(sw_reg12, 0xFFFF);
    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG12r(&phy->access, sw_reg12));

    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_loopback_set(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_loopback_mode_t loopback, uint32_t enable)
{        
    
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_tscp_phy_loopback_set(phy, loopback, enable));
        
    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_loopback_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_loopback_mode_t loopback, uint32_t* enable)
{        
    
   PHYMOD_IF_ERR_RETURN(
        plp_aperta2_tscp_phy_loopback_get(phy, loopback, enable));
       
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_rx_pmd_locked_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* rx_pmd_locked)
{        
    
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_peregrine5_pc_phy_rx_pmd_locked_get(phy, rx_pmd_locked));
    
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_link_status_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* link_status)
{        
    
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_tscp_phy_link_status_get(phy, link_status));
        
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_status_dump(const plp_aperta2_phymod_phy_access_t* phy)
{        
    
    return _plp_aperta2_phy_status_dump(phy);
    
}


int plp_aperta2_phy_reg_read(const plp_aperta2_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t* val)
{        
    
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_reg32_read(phy, reg_addr, val));

    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_reg_write(const plp_aperta2_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t val)
{        
    
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_reg32_write(phy, reg_addr, val));

        
    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_rev_id(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* rev_id)
{        
    return plp_aperta2_get_chip_rev (&phy->access, rev_id);
}


int plp_aperta2_phy_intr_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t enable)
{        
    int speed = 0, configured_lane = 0, port = 0;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_lane_speed_get(phy, &speed, &configured_lane, &port));
    (void) speed;
    (void) configured_lane;

    return _plp_aperta2_intr_enable_set(phy, port, enable);
    
}

int plp_aperta2_phy_intr_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* enable)
{        
    int speed = 0, configured_lane = 0, port = 0;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_lane_speed_get(phy, &speed, &configured_lane, &port));
    (void) speed;
    (void) configured_lane;

    return _plp_aperta2_intr_enable_get(phy, port, enable);
    
}


int plp_aperta2_phy_intr_status_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* intr_status)
{        
    int speed = 0, configured_lane = 0, port = 0;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_lane_speed_get(phy, &speed, &configured_lane, &port));
    (void) speed;
    (void) configured_lane;

    return _plp_aperta2_phy_intr_status_get(phy, port, intr_status);
    
}


int plp_aperta2_phy_intr_status_clear(const plp_aperta2_phymod_phy_access_t* phy, uint32_t intr_clr)
{        
    int speed = 0, configured_lane = 0, port = 0;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_lane_speed_get(phy, &speed, &configured_lane, &port));
    (void) speed;
    (void) configured_lane;

    return _plp_aperta2_phy_intr_status_clear(phy, port, intr_clr);
    
}


int plp_aperta2_phy_i2c_read(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t addr, uint32_t offset, uint32_t size, uint8_t* data)
{        
    (void) flags; 
    return _plp_aperta2_phy_i2c_read(phy, addr, offset, size, data);
    
}


int plp_aperta2_phy_i2c_write(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t addr, uint32_t offset, uint32_t size, const uint8_t* data)
{        
        
    (void) flags; 
    return _plp_aperta2_phy_i2c_write(phy, addr, offset, size, data);
}


int plp_aperta2_phy_gpio_config_set(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, plp_aperta2_phymod_gpio_mode_t gpio_mode)
{        
    
    return _plp_aperta2_phy_gpio_config_set(phy, pin_no, gpio_mode);
}

int plp_aperta2_phy_gpio_config_get(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, plp_aperta2_phymod_gpio_mode_t* gpio_mode)
{        
    
    return _plp_aperta2_phy_gpio_config_get(phy, pin_no, gpio_mode);
    
}


int plp_aperta2_phy_gpio_pin_value_set(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, int value)
{        
    
    return _plp_aperta2_phy_gpio_pin_value_set(phy, pin_no, value);
    
}

int plp_aperta2_phy_gpio_pin_value_get(const plp_aperta2_phymod_phy_access_t* phy, int pin_no, int* value)
{        
    return _plp_aperta2_phy_gpio_pin_value_get(phy, pin_no, value);
        
}

int _plp_aperta2_ptp_sync_enable(const plp_aperta2_phymod_phy_access_t* phy, int enable)
{

    return PHYMOD_E_NONE;
}

int plp_aperta2_timesync_config_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_timesync_config_t* config)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    aperta2_device_aux_modes_t auxmode;
    plp_aperta2_phymod_phy_inf_config_t    port_config;
    plp_aperta2_phymod_phy_access_t        fw_access;
    aperta2_ptp_config_t       ptp_cfg;
    uint32_t flags=0, port = 0, lane = 0, oct = 0;
    unsigned int is_fo_enabled = 0, primary_lm = 0;
    uint8_t port_option = 0;
    int rv = 0;

    (void) lane;
    (void) oct;

    PHYMOD_MEMSET(&port_config, 0,   sizeof(plp_aperta2_phymod_phy_inf_config_t));
    PHYMOD_MEMSET(&ptp_cfg,     0,   sizeof(aperta2_ptp_config_t));
    PHYMOD_MEMCPY(&fw_access,   phy, sizeof(plp_aperta2_phymod_phy_access_t));
    port_config.device_aux_modes = &auxmode;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_interface_config_get(phy, flags, &port_config));

    if (phy->access.lane_mask == auxmode.failover_config.lane_map) {
        /* Fo lane map: use Primary lanemap to get port*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_is_fo_enabled(phy, &port_config, &is_fo_enabled, &port, &primary_lm));
        (void)is_fo_enabled;
        fw_access.access.lane_mask = primary_lm;
    } else {
        /* Use Primary lanemap to get port*/
        APERTA2_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, lane, oct);
    }
    PHYMOD_DEBUG_INFO(("ts_config_Set:port:%d DataR:%d ldr:%d lanemask:%x failover:%x \n",port,  port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, auxmode.failover_config.lane_map));
    ptp_cfg.PortNum = port;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_fw_config_ptp_read (&fw_access, &ptp_cfg));

    if ((config->flags >> APERTA2_TS_MODE_FLAG_SHIFT) & 0x1FF) {
        port_option = plp_aperta2_phymod_log2n((config->flags >> APERTA2_TS_MODE_FLAG_SHIFT) & 0x1FF);
        if (port_option > 4) {
            if(port_option < 7) {
                port_option = 7 ;
                PHYMOD_DEBUG_ERROR(("ERROR: Invalid port option\n"));
                return PHYMOD_E_PARAM;
            } else {
                port_option += 3;
                if(port_option >= 10) {
                    port_option -= 5 ;
                }
            }
        }
        ptp_cfg.PortOptions &= ~(0x7 << 4);
        ptp_cfg.PortOptions |= ((port_option & 0x7) << 4);
    }
    /* Pause port */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_send_fw_msg(&fw_access, port_config.data_rate , &auxmode, APERTA2_MSG_PAUSE_PORT, NULL, 0));

    /* Flush port */
    rv = plp_aperta2_send_fw_msg(&fw_access, port_config.data_rate , &auxmode, APERTA2_MSG_FLUSH_PORT, NULL, 0);
    APERTA2_TS_PORT_RESUME(rv, "FLUSH", port);

    if ((config->flags >> APERTA2_TS_MODE_FLAG_SHIFT) & 0x1FF) {
        rv = plp_aperta2_fw_config_ptp_write (&fw_access, &ptp_cfg);
        APERTA2_TS_PORT_RESUME(rv, "CONFIG_PTP", port);
    }

    rv =  _plp_aperta2_ptp_sync_enable(phy, TRUE);
    APERTA2_TS_PORT_RESUME(rv, "_aperta_ptp_sync_enable", port);
    rv = _plp_aperta2_timesync_config_set(phy, config);
    APERTA2_TS_PORT_RESUME(rv, "_plp_aperta2_timesync_config_set", port);

    return plp_aperta2_send_fw_msg(&fw_access, port_config.data_rate , &auxmode, APERTA2_MSG_RESUME_PORT, NULL, 0);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_config_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_timesync_config_t* config)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    aperta2_device_aux_modes_t auxmode;
    plp_aperta2_phymod_phy_inf_config_t    port_config;
    plp_aperta2_phymod_phy_access_t        fw_access;
    aperta2_ptp_config_t       ptp_cfg;
    uint32_t flags=0, port = 0, lane = 0, oct = 0;
    unsigned int is_fo_enabled = 0, primary_lm = 0;
    uint8_t port_option = 0;

    (void) lane;
    (void) oct;

    PHYMOD_MEMSET(&port_config, 0,   sizeof(plp_aperta2_phymod_phy_inf_config_t));
    PHYMOD_MEMSET(&ptp_cfg,     0,   sizeof(aperta2_ptp_config_t));
    PHYMOD_MEMCPY(&fw_access,   phy, sizeof(plp_aperta2_phymod_phy_access_t));
    port_config.device_aux_modes = &auxmode;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_interface_config_get(phy, flags, &port_config));

    if (phy->access.lane_mask == auxmode.failover_config.lane_map) {
        /* Fo lane map: use Primary lanemap to get port*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_is_fo_enabled(phy, &port_config, &is_fo_enabled, &port, &primary_lm));
        (void)is_fo_enabled;
        fw_access.access.lane_mask = primary_lm;
    } else {
        /* Use Primary lanemap to get port*/
        APERTA2_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, lane, oct);
    }
    
    PHYMOD_DEBUG_INFO(("ts_config_get:port:%d DataR:%d ldr:%d lanemask:%x failover:%x \n",port, port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, auxmode.failover_config.lane_map));
    APERTA2_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, lane, oct);
    ptp_cfg.PortNum = port;
    PHYMOD_IF_ERR_RETURN( _plp_aperta2_timesync_config_get(phy, config));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_fw_config_ptp_read (&fw_access, &ptp_cfg));

    port_option = ((ptp_cfg.PortOptions >> 4) & 0x7);
    if(port_option < 5) {
    	config->flags |= (1 << ((port_option) + APERTA2_TS_MODE_FLAG_SHIFT));
    } else if(port_option < 7) {
    	config->flags |= (1 << ((port_option + 2) + APERTA2_TS_MODE_FLAG_SHIFT));
    }
    
    return PHYMOD_E_NONE;
#else
    return PHYMOD_E_UNAVAIL;
#endif
}


int plp_aperta2_timesync_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    PHYMOD_IF_ERR_RETURN( _plp_aperta2_ptp_sync_enable(phy, enable) );
    return _plp_aperta2_timesync_enable_set(phy, flags, enable);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_enable_get(phy, flags, enable);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}


int plp_aperta2_timesync_nco_addend_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t freq_step)
{        
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return PHYMOD_E_UNAVAIL;
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_nco_addend_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t* freq_step)
{        
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return PHYMOD_E_UNAVAIL;
#else
    return PHYMOD_E_UNAVAIL;
#endif
}


int plp_aperta2_timesync_framesync_mode_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_timesync_framesync_t* framesync)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_framesync_mode_set(phy, framesync);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_framesync_mode_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_timesync_framesync_t* framesync)
{        
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_framesync_mode_get(phy, framesync);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_capture_timestamp_get(const plp_aperta2_phymod_phy_access_t* phy, uint64_t* cap_ts)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return PHYMOD_E_UNAVAIL;
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_failover_mode_set(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_failover_mode_t failover_mode)
{        
    int port_number = 0, cur_active_port = 0;
    plp_aperta2_phymod_failover_mode_t failover_mode_get;

    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask,port_number);
    PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_fo_get(phy, port_number, &failover_mode_get));
    cur_active_port = (failover_mode_get & 0xF0);
    if (failover_mode_get & 1) {
        if (cur_active_port !=  (failover_mode & 0xF0)) {
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_fw_switch_mux (phy, port_number));
        }
    } else {
        PHYMOD_DEBUG_ERROR(("Port LM: %x side:%d is not a failover port\n", phy->access.lane_mask, phy->port_loc));
        return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
    
}

int plp_aperta2_failover_mode_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_failover_mode_t* failover_mode)
{        
    
    int port_number = 0;
    
    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask,port_number);
    PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_fo_get(phy, port_number, failover_mode));

    return PHYMOD_E_NONE;
    
}


int plp_aperta2_phy_pam4_fec_status_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_phy_fec_dump_status_t* fec_sts)
{        
    
    return _plp_aperta2_phy_pam4_fec_status_get(phy, fec_sts);
    
}

/* AVS helper function to set or get AVS configuration */
int _plp_aperta2_setget_avs_config(const plp_aperta2_phymod_phy_access_t* phy,
    phymod_avs_config_t *avs_config, APERTA2_ACTION_TYPE_T action_type)
{
    BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_t avs_config_type;
    BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_t avs_status_type;
    BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_t regulator0_config_type;
    BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_t regulator1_config_type;
    BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_t i2c_addr0_type;
    BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_t i2c_addr1_type;
    BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_t vout_ctrl_type;
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_t gpreg_1;
    uint16_t retry_cnt = 25;
    uint32_t debug_data = 0;

    BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_CLR(avs_config_type);
    BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_CLR(avs_status_type);
    BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_CLR(regulator0_config_type);
    BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_CLR(regulator1_config_type);
    BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_CLR(i2c_addr0_type);
    BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_CLR(i2c_addr1_type);
    BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_CLR(vout_ctrl_type);
    BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_CLR(gpreg_1);

    if (!phy->access.lane_mask) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, ("Invalid lane mask: 0x0\n"));
    }
    if (!avs_config) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_MEMORY, ("AVS config structure pointer is NULL\n"));
    }
    if (action_type == APERTA2_ACTION_TYPE_SET) {
        BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_CLR(avs_config_type);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_CLR(regulator0_config_type);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_CLR(regulator1_config_type);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_CLR(i2c_addr0_type);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_CLR(i2c_addr1_type);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_CLR(vout_ctrl_type);

        /* Aperta2 only supports Regulators 4678 (SW/Eval board) and 4677 (DVT board) */
        if (!((avs_config->avs_regulator == phymodAvsRegulator4678) || (avs_config->avs_regulator == phymodAvsRegulator4677))) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, ("Unsupported AVS regulator:0x%x\n", avs_config->avs_regulator));
        }
        /* Check AVS state and clear AVS init done bit only when AVS is disabled */
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A0r, &avs_config_type._fws_fwreg_3a0));


        if (BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_EN_GET(avs_config_type) == 0) {
            /* Return success if user requests to disable AVS and if its already disabled */
            if (avs_config->enable == 0) {
                PHYMOD_DEBUG_INFO(("AVS is already disabled\n"));
                return PHYMOD_E_NONE;
            }
            /* Inform FW by clearing AVS init done bit. FW will set this bit after AVS init is done */
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GPREG_01r(&phy->access, &gpreg_1));
            BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_AVS_INIT_DONE_SET(gpreg_1, 0);
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_GEN_CNTRLS_GPREG_01r(&phy->access, gpreg_1));
        }
        if (avs_config->regulator_config[0].regulator_addr != 0) {
            BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_ADDR_SET(regulator0_config_type, avs_config->regulator_config[0].regulator_addr);
            BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_TYPE_SET(regulator0_config_type, avs_config->regulator_config[0].regulator_type);
            BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_CH0_INFO_SET(regulator0_config_type,
                (avs_config->regulator_config[0].channel_info[0] == 1) ? 0x3 : 0x2);
            BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_CH1_INFO_SET(regulator0_config_type,
                (avs_config->regulator_config[0].channel_info[1] == 1) ? 0x3 : 0x2);
            /* Setting higher I2C speed to 400 KHz by default (1), other I2C supported speed is 100KHz (0) */
            BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_I2C_SPEED_SET(regulator0_config_type, 1);

            /* Write 0x3A2 register */
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A2r, regulator0_config_type._fws_fwreg_3a2));
        }
        /* Configure 2nd Regulator only if regulator is 4677 (DVT board) */
        if ((avs_config->regulator_config[1].regulator_addr != 0) &&
            (avs_config->regulator_config[1].regulator_type == phymodAvsRegulator4677))
        {
            BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_ADDR_SET(regulator1_config_type, avs_config->regulator_config[1].regulator_addr);
            BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_TYPE_SET(regulator1_config_type, avs_config->regulator_config[1].regulator_type);
            BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_CH0_INFO_SET(regulator1_config_type,
                (avs_config->regulator_config[1].channel_info[0] == 1) ? 0x3 : 0x1);
            BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_CH1_INFO_SET(regulator1_config_type,
                (avs_config->regulator_config[1].channel_info[1] == 1) ? 0x3 : 0x1);
            /* Setting higher I2C speed to 400 KHz by default */
            BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_I2C_SPEED_SET(regulator1_config_type, 1);

            /* Write 0x3A3 register */
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A3r, regulator1_config_type._fws_fwreg_3a3));
        }
        /* Not supported in FW */
        BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_AVS_RESPONDER0_ADDR_SET(i2c_addr0_type, avs_config->i2c_slave_address[0]);
        /* Configure common DVDD power rail I2C address */
        BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_AVS_DVDD_RAIL_ADDR_SET(i2c_addr0_type, avs_config->regulator_i2c_address);

        /* Write 0x3A4 register */
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A4r, i2c_addr0_type._fws_fwreg_3a4));

        /* Update 0x3A5 register - Only supported for external regulators (not supported in FW) */
        BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_AVS_RESPONDER1_ADDR_SET(i2c_addr1_type, avs_config->i2c_slave_address[1]);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_AVS_RESPONDER2_ADDR_SET(i2c_addr1_type, avs_config->i2c_slave_address[2]);
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A5r, i2c_addr1_type._fws_fwreg_3a5));

        /* Update 0x3A6 reg to configure DC margin value. External control step is not supported in FW */
        BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_AVS_BRD_DC_MARGIN_SET(vout_ctrl_type, avs_config->avs_dc_margin);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_AVS_EXT_CTRL_STEP_SET(vout_ctrl_type, avs_config->external_ctrl_step);
        /* Write 3A6 reg */
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A6r, vout_ctrl_type._fws_fwreg_3a6));

        /* AVS status might be set to phymodAvsStatusInitError due to I2C mis-configuration and unreliability during previous boot cycle,
         * it should be cleared before AVS can be enabled again
         * */
        PHYMOD_IF_ERR_RETURN(
           plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A1r, &avs_status_type._fws_fwreg_3a1));
        if (BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_STS_GET(avs_status_type) == phymodAvsStatusInitError) {
            BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_STS_SET(avs_status_type, phymodAvsStatusNotStarted);
            /* Clear AVS status if it is set. FW sets it but does not clear it */
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A1r, avs_status_type._fws_fwreg_3a1));
        }

        /* Write 3A0 reg to enable or disable AVS and configure master vs slave */
        BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_EN_SET(avs_config_type, avs_config->enable);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_IS_PRI_SET(avs_config_type, avs_config->internal_master_slave_package);
        /* Following options are not supported in FW */
        BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_DIS_TYPE_SET(avs_config_type, avs_config->avs_disable_type);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_CTRL_SET(avs_config_type, avs_config->avs_ctrl);
        BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_PKG_SHARE_SET(avs_config_type, avs_config->avs_pkg_share);
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A0r, avs_config_type._fws_fwreg_3a0));
        /* Wait for FW to complete AVS operation */
        do {
            PHYMOD_USLEEP(200000);
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_GEN_CNTRLS_GPREG_01r(&phy->access, &gpreg_1));
            /* Check for AVS init done (bit 4)*/
            if (BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_01r_AVS_INIT_DONE_GET(gpreg_1) == 1) {
                /* FW sets AVS status to 0 when AVS is disabled */
                if (avs_config->enable == 0) {
                    break;
                } else {
                    if(avs_config->avs_ctrl == 1) {
                        if(!avs_config->i2c_slave_address[0]) {
                            PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_GEN_CNTRLS_GPREG_19r, &debug_data));
                            PHYMOD_DEBUG_INFO(("MDIO:AVS_VOUT_REQ : %04X\n", debug_data));
                        } else {
                            PHYMOD_IF_ERR_RETURN(plp_aperta2_reg32_read(phy, (APERTA2_VTMON_AVS_I2C_PMON_BASEADR | 0x2008) , &debug_data));
                            PHYMOD_DEBUG_INFO(("I2C:AVS_VOUT_REQ : %04X\n", debug_data));
                        }
                    }
                    PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A1r, &avs_status_type._fws_fwreg_3a1));
                    /* Exit immediately if FW detects this error state */
                    if (BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_STS_GET(avs_status_type) == phymodAvsStatusInitError) {
                        PHYMOD_DEBUG_ERROR(("Invalid I2C configuration or I2C communication is not reliable\n"));
                        return PHYMOD_E_FAIL;
                    }
                    /* Exit the timeout loop only when AVS is enabled successfully (init done is set and avs status is success) */
                    if (BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_STS_GET(avs_status_type) == phymodAvsStatusAliveAndInitSuccess) {
                        break;
                    }
                }
            }
        } while (--retry_cnt);
        if(retry_cnt == 0) {
            PHYMOD_DEBUG_ERROR(("Timeout in getting AVS init done\n"));
            return PHYMOD_E_TIMEOUT;
        }
    } else if (action_type == APERTA2_ACTION_TYPE_GET) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A0r, &avs_config_type._fws_fwreg_3a0));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A2r, &regulator0_config_type._fws_fwreg_3a2));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A3r, &regulator1_config_type._fws_fwreg_3a3));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A4r, &i2c_addr0_type._fws_fwreg_3a4));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A5r, &i2c_addr1_type._fws_fwreg_3a5));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A6r, &vout_ctrl_type._fws_fwreg_3a6));

        avs_config->enable = BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_EN_GET(avs_config_type);
        avs_config->avs_disable_type = BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_DIS_TYPE_GET(avs_config_type);
        avs_config->avs_ctrl = BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_CTRL_GET(avs_config_type);
        avs_config->avs_pkg_share = BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_PKG_SHARE_GET(avs_config_type);
        avs_config->internal_master_slave_package = BCM_APERTA2_DIRECT_FWS_FWREG_3A0r_AVS_IS_PRI_GET(avs_config_type);
        avs_config->regulator_config[0].regulator_type = BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_TYPE_GET(regulator0_config_type);
        avs_config->regulator_config[0].regulator_addr = BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_ADDR_GET(regulator0_config_type);
        avs_config->regulator_config[0].channel_info[0] = (BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_CH0_INFO_GET(regulator0_config_type) == 0x3) ? 1 : 0;
        avs_config->regulator_config[0].channel_info[1] = (BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_REGULATOR_CH1_INFO_GET(regulator0_config_type) == 0x3) ? 1 : 0;

        avs_config->regulator_config[1].regulator_type = BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_TYPE_GET(regulator1_config_type);
        if (avs_config->regulator_config[1].regulator_type == phymodAvsRegulator4677) {
            avs_config->regulator_config[1].regulator_addr = BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_ADDR_GET(regulator1_config_type);
            avs_config->regulator_config[1].channel_info[0] = (BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_CH0_INFO_GET(regulator1_config_type) == 0x3) ? 1 : 0;
            avs_config->regulator_config[1].channel_info[1] = (BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_REGULATOR_CH1_INFO_GET(regulator1_config_type) == 0x3) ? 1 : 0;
        }
        avs_config->avs_dc_margin = BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_AVS_BRD_DC_MARGIN_GET(vout_ctrl_type);
        avs_config->external_ctrl_step = BCM_APERTA2_DIRECT_FWS_FWREG_3A6r_AVS_EXT_CTRL_STEP_GET(vout_ctrl_type);

        avs_config->i2c_slave_address[0] = BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_AVS_RESPONDER0_ADDR_GET(i2c_addr0_type);
        avs_config->regulator_i2c_address = BCM_APERTA2_DIRECT_FWS_FWREG_3A4r_AVS_DVDD_RAIL_ADDR_GET(i2c_addr0_type);
        avs_config->i2c_slave_address[1] = BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_AVS_RESPONDER1_ADDR_GET(i2c_addr1_type);
        avs_config->i2c_slave_address[2] = BCM_APERTA2_DIRECT_FWS_FWREG_3A5r_AVS_RESPONDER2_ADDR_GET(i2c_addr1_type);

        PHYMOD_DEBUG_INFO(("avs_config.enable = %d\n", avs_config->enable));
        PHYMOD_DEBUG_INFO(("avs_config.avs_ctrl = %d\n", avs_config->avs_ctrl));
        PHYMOD_DEBUG_INFO(("avs_config.avs_disable_type = %d\n", avs_config->avs_disable_type));
        PHYMOD_DEBUG_INFO(("avs_config.avs_pkg_share = %d\n", avs_config->avs_pkg_share));
        PHYMOD_DEBUG_INFO(("avs_config.avs_dc_margin = %d\n", avs_config->avs_dc_margin));
        PHYMOD_DEBUG_INFO(("avs_config.avs_regulator = %d\n", avs_config->avs_regulator));
        PHYMOD_DEBUG_INFO(("avs_config.external_ctrl_step = %d\n", avs_config->external_ctrl_step));
        PHYMOD_DEBUG_INFO(("avs_config.i2c_slave_address[0] = %d\n", avs_config->i2c_slave_address[0]));
        PHYMOD_DEBUG_INFO(("avs_config.i2c_slave_address[1] = %d\n", avs_config->i2c_slave_address[1]));
        PHYMOD_DEBUG_INFO(("avs_config.i2c_slave_address[2] = %d\n", avs_config->i2c_slave_address[2]));
        PHYMOD_DEBUG_INFO(("avs_config.regulator_i2c_address = %d\n", avs_config->regulator_i2c_address));
        PHYMOD_DEBUG_INFO(("avs_config.regulator_legs = %d\n", avs_config->regulator_legs));
        PHYMOD_DEBUG_INFO(("avs_config.internal_master_slave_package = %d\n", avs_config->internal_master_slave_package));

        PHYMOD_DEBUG_INFO(("avs_config->regulator_config[0].regulator_type = %d\n", avs_config->regulator_config[0].regulator_type));
        PHYMOD_DEBUG_INFO(("avs_config->regulator_config[0].regulator_addr = %d\n", avs_config->regulator_config[0].regulator_addr));
        PHYMOD_DEBUG_INFO(("avs_config->regulator_config[0].channel_info[0] = %d\n", avs_config->regulator_config[0].channel_info[0]));
        PHYMOD_DEBUG_INFO(("avs_config->regulator_config[0].channel_info[1] = %d\n", avs_config->regulator_config[0].channel_info[1]));
        PHYMOD_DEBUG_INFO(("3A2r_AVS_I2C_SPEED_GET = %d\n", BCM_APERTA2_DIRECT_FWS_FWREG_3A2r_AVS_I2C_SPEED_GET(regulator0_config_type)));

        PHYMOD_DEBUG_INFO(("avs_config->regulator_config[1].regulator_type = %d\n", avs_config->regulator_config[1].regulator_type));
        PHYMOD_DEBUG_INFO(("avs_config->regulator_config[1].regulator_addr = %d\n", avs_config->regulator_config[1].regulator_addr));
        PHYMOD_DEBUG_INFO(("avs_config->regulator_config[1].channel_info[0] = %d\n", avs_config->regulator_config[1].channel_info[0]));
        PHYMOD_DEBUG_INFO(("avs_config->regulator_config[1].channel_info[1] = %d\n", avs_config->regulator_config[1].channel_info[1]));
        PHYMOD_DEBUG_INFO(("3A3r_AVS_I2C_SPEED_GET = %d\n", BCM_APERTA2_DIRECT_FWS_FWREG_3A3r_AVS_I2C_SPEED_GET(regulator1_config_type)));
    } else {
        return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;
}
/* Set AVS configurations */
int plp_aperta2_phy_avs_config_set(const plp_aperta2_phymod_phy_access_t* phy, phymod_avs_config_t avs_config)
{
    phymod_avs_config_t avs_config_get;

    PHYMOD_MEMSET(&avs_config_get, 0, sizeof(phymod_avs_config_t));
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_phy_avs_config_get(phy, &avs_config_get));

    PHYMOD_DEBUG_INFO(("avs_config.enable = %d\n", avs_config.enable));
    PHYMOD_DEBUG_INFO(("avs_config.avs_ctrl = %d\n", avs_config.avs_ctrl));
    PHYMOD_DEBUG_INFO(("avs_config.avs_disable_type = %d\n", avs_config.avs_disable_type));
    PHYMOD_DEBUG_INFO(("avs_config.avs_pkg_share = %d\n", avs_config.avs_pkg_share));
    PHYMOD_DEBUG_INFO(("avs_config.avs_dc_margin = %d\n", avs_config.avs_dc_margin));
    PHYMOD_DEBUG_INFO(("avs_config.avs_regulator = %d\n", avs_config.avs_regulator));
    PHYMOD_DEBUG_INFO(("avs_config.external_ctrl_step = %d\n", avs_config.external_ctrl_step));
    PHYMOD_DEBUG_INFO(("avs_config.i2c_slave_address[0] = %d\n", avs_config.i2c_slave_address[0]));
    PHYMOD_DEBUG_INFO(("avs_config.i2c_slave_address[1] = %d\n", avs_config.i2c_slave_address[1]));
    PHYMOD_DEBUG_INFO(("avs_config.i2c_slave_address[2] = %d\n", avs_config.i2c_slave_address[2]));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_i2c_address = %d\n", avs_config.regulator_i2c_address));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_legs = %d\n", avs_config.regulator_legs));
    PHYMOD_DEBUG_INFO(("avs_config.internal_master_slave_package = %d\n", avs_config.internal_master_slave_package));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_config[0].regulator_type = %d\n", avs_config.regulator_config[0].regulator_type));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_config[0].regulator_addr = %d\n", avs_config.regulator_config[0].regulator_addr));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_config[0].channel_info[0] = %d\n", avs_config.regulator_config[0].channel_info[0]));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_config[0].channel_info[1] = %d\n", avs_config.regulator_config[0].channel_info[1]));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_config[1].regulator_type = %d\n", avs_config.regulator_config[1].regulator_type));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_config[1].regulator_addr = %d\n", avs_config.regulator_config[1].regulator_addr));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_config[1].channel_info[0] = %d\n", avs_config.regulator_config[1].channel_info[0]));
    PHYMOD_DEBUG_INFO(("avs_config.regulator_config[1].channel_info[1] = %d\n", avs_config.regulator_config[1].channel_info[1]));
    if (avs_config.enable != avs_config_get.enable) { /* Don't disable AVS if it is already in disabled state*/ 
        PHYMOD_IF_ERR_RETURN(_plp_aperta2_setget_avs_config(phy, &avs_config, APERTA2_ACTION_TYPE_SET));
    }
    return PHYMOD_E_NONE;
}
/* Get AVS configuration */
int plp_aperta2_phy_avs_config_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_avs_config_t* avs_config)
{
    PHYMOD_IF_ERR_RETURN(_plp_aperta2_setget_avs_config(phy, avs_config, APERTA2_ACTION_TYPE_GET));
    return PHYMOD_E_NONE;
}
/* Get AVS state */
int plp_aperta2_phy_avs_status_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_avs_config_status_t* avs_status)
{
    BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_t avs_fw_status_type;
    BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_CLR(avs_fw_status_type);

    PHYMOD_MEMSET(avs_status, 0, sizeof(phymod_avs_config_status_t));
    PHYMOD_IF_ERR_RETURN(
       plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_FWS_FWREG_3A1r, &avs_fw_status_type._fws_fwreg_3a1));

    avs_status->enable = BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_ENABLED_GET(avs_fw_status_type);
    avs_status->avs_track_status = BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_TRACK_STS_GET(avs_fw_status_type);
    avs_status->avs_status = BCM_APERTA2_DIRECT_FWS_FWREG_3A1r_AVS_STS_GET(avs_fw_status_type);

    return PHYMOD_E_NONE;
}

int plp_aperta2_synce_config_set(const plp_aperta2_phymod_phy_access_t* phy, const phymod_synce_cfg_t* synce_cfg)
{       
    return plp_aperta2_pm_synce_config_set(phy, synce_cfg);
}

int plp_aperta2_synce_config_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_synce_cfg_t* synce_cfg)
{        
    
    return plp_aperta2_pm_synce_config_get(phy, synce_cfg);
}


int plp_aperta2_timesync_time_code_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta2_phymod_timesync_timespec_t* timecode)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_time_code_set(phy, flags, timecode);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_time_code_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_timesync_timespec_t* timecode)
{        
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_time_code_get(phy, flags, timecode);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_timestamp_offset_set(const plp_aperta2_phymod_phy_access_t* phy, int txrx, uint32_t op, uint32_t msg_type, uint64_t ts_offset)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_timestamp_offset_set(phy, txrx, op, msg_type, COMPILER_64_LO(ts_offset));
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_timestamp_offset_get(const plp_aperta2_phymod_phy_access_t* phy, int txrx, uint32_t* op, uint32_t* msg_type, uint64_t* ts_offset)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    uint32_t  tsoffset32 = 0;
    PHYMOD_IF_ERR_RETURN( _plp_aperta2_timesync_timestamp_offset_get(phy, txrx, op, msg_type, &tsoffset32) );
    COMPILER_64_SET(*ts_offset, 0, tsoffset32);
    return PHYMOD_E_NONE;
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_sopmem_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, int index, phymod_timesync_sopmem_t* sopmem)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_sopmem_get(phy, flags, index, sopmem);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_mpls_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta2_phymod_timesync_mpls_ctrl_t* config)
{        
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_mpls_set(phy, flags, config);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_mpls_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_timesync_mpls_ctrl_t* config)
{
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_mpls_get(phy, flags, config);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_inband_filter_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t index, const phymod_timesync_inband_filter_ctrl_t* config)
{        
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_inband_filter_set(phy, flags, index, config);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_timesync_inband_filter_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t index, phymod_timesync_inband_filter_ctrl_t* config)
{        
#if defined (PHYMOD_TIMESYNC_SUPPORT)
    return _plp_aperta2_timesync_inband_filter_get(phy, flags, index, config);
#else
    return PHYMOD_E_UNAVAIL;
#endif
}

int plp_aperta2_phy_pai_info_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_pai_phy_op_t operation, phymod_pai_phy_config_t* pai_phy_config, void* epdm_data)
{        
    
    int no_of_lanes = 0, ldr = 0;
    if (pai_phy_config->config_type == phymodConfigTypeFw_init) {
        phymod_pai_fw_init_config_t *pai_fw_init_config = (phymod_pai_fw_init_config_t*)(pai_phy_config->config_data);
        aperta2_fw_init_t *aperta2_fw_init_param = (aperta2_fw_init_t *)(epdm_data);
        if (operation != phymodOperationPai_to_phy) {
            return PHYMOD_E_NONE;
        }
        aperta2_fw_init_param->macsec_option = pai_fw_init_config-> macsec_static_bypass;
        aperta2_fw_init_param->ptp_option    = pai_fw_init_config-> ptp_option ;

        aperta2_fw_init_param->octal0.sys_vco        = pai_fw_init_config->octal0.sys_vco;
        aperta2_fw_init_param->octal0.line_vco       = pai_fw_init_config->octal0.line_vco;
        aperta2_fw_init_param->octal1.sys_vco        = pai_fw_init_config->octal1.sys_vco;
        aperta2_fw_init_param->octal1.line_vco       = pai_fw_init_config->octal1.line_vco;
        
        PHYMOD_MEMCPY(&aperta2_fw_init_param->sys_lane_map, &pai_fw_init_config->sys_lane_map, sizeof(plp_aperta2_phymod_lane_map_t));
        PHYMOD_MEMCPY(&aperta2_fw_init_param->line_lane_map, &pai_fw_init_config->line_lane_map, sizeof(plp_aperta2_phymod_lane_map_t));
    } else if (pai_phy_config->config_type == phymodConfigTypePort_config) {
        phymod_pai_port_config_t *port_cfg = (phymod_pai_port_config_t*)(pai_phy_config->config_data);
        aperta2_device_aux_modes_t *aperta2_aux_mode = (aperta2_device_aux_modes_t*) epdm_data;
        if (operation == phymodOperationPai_to_phy) {
            PHYMOD_MEMSET(aperta2_aux_mode, 0, sizeof(aperta2_device_aux_modes_t));
            no_of_lanes = plp_aperta2_phymod_count_set_bits(phy->access.lane_mask);
            ldr = (port_cfg->speed/no_of_lanes);
            aperta2_aux_mode->failover_config.lane_map = port_cfg->failover_lane_map;
            aperta2_aux_mode->failover_config.tx_back_pressure_mode= 0;
            if (ldr == APERTA2_SPEED_10G) {
                aperta2_aux_mode->lane_data_rate = bcmAperta2LaneDataRate_10P3125G;
                aperta2_aux_mode->modulation_mode = bcmAperta2ModulationNRZ;
            } else if (ldr == APERTA2_SPEED_25G) {
                aperta2_aux_mode->lane_data_rate = bcmAperta2LaneDataRate_25P78125G;
                aperta2_aux_mode->modulation_mode = bcmAperta2ModulationNRZ;
            } else if (ldr == APERTA2_SPEED_50G) {
                aperta2_aux_mode->lane_data_rate = bcmAperta2LaneDataRate_53P125G;
                aperta2_aux_mode->modulation_mode = bcmAperta2ModulationPAM4;
            } else if (ldr == APERTA2_SPEED_100G) {
                aperta2_aux_mode->lane_data_rate = bcmAperta2LaneDataRate_106P25G;
                aperta2_aux_mode->modulation_mode = bcmAperta2ModulationPAM4;
            } else {
                PHYMOD_DEBUG_ERROR(("Invalid lane data rate : %d number of lane:%d speed:%d\n",
                            ldr,no_of_lanes, port_cfg->speed));
                return PHYMOD_E_PARAM;
            }
            if ((port_cfg->fec_mode >> 8) & 1) { /* Use extended FEC selected */
                if ((port_cfg->fec_mode & 0xFF) == phymodPaiFecModeExtendedRS528)  {
                    aperta2_aux_mode->fec_mode_sel = bcmAperta2RSFEC;
                } else if ((port_cfg->fec_mode & 0xFF) == phymodPaiFecModeExtendedRS544) {
                    aperta2_aux_mode->fec_mode_sel = bcmAperta2RS544;
                } else if ((port_cfg->fec_mode & 0xFF) == phymodPaiFecModeExtendedRS544_INTERLEAVED) {
                    aperta2_aux_mode->fec_mode_sel = bcmAperta2RS544_2XN;
                } else if ((port_cfg->fec_mode & 0xFF) == phymodPaiFecModeExtendedNone){
                    aperta2_aux_mode->fec_mode_sel = bcmAperta2NoFEC;
                } else {
                    return PHYMOD_E_PARAM;
                }
            } else {
                if (port_cfg->fec_mode == phymodPaiFecModeRS)  {
                    if (port_cfg->speed == APERTA2_SPEED_800G || port_cfg->speed == APERTA2_SPEED_400G ||
                         port_cfg->speed == APERTA2_SPEED_200G) {
                        aperta2_aux_mode->fec_mode_sel =  bcmAperta2RS544_2XN;
                    } else if (port_cfg->speed == APERTA2_SPEED_100G && (no_of_lanes == 4 || no_of_lanes == 2)) {
                        aperta2_aux_mode->fec_mode_sel =  bcmAperta2RSFEC;
                    } else if (port_cfg->speed == APERTA2_SPEED_100G && no_of_lanes == 1) {
                        aperta2_aux_mode->fec_mode_sel =  bcmAperta2RS544;
                    } else if (port_cfg->speed == APERTA2_SPEED_50G && (no_of_lanes == 1 || no_of_lanes == 2)) {
                        aperta2_aux_mode->fec_mode_sel =  bcmAperta2RSFEC;
                    } else if (port_cfg->speed == APERTA2_SPEED_25G) {
                        aperta2_aux_mode->fec_mode_sel =  bcmAperta2RSFEC;
                    } else {
                        PHYMOD_DEBUG_ERROR(("Invalid lane data rate : %d number of lane:%d speed:%d fec:%d\n",
                            ldr,no_of_lanes, port_cfg->speed, port_cfg->fec_mode));
                        return PHYMOD_E_PARAM;
                    }
                } else {
                    aperta2_aux_mode->fec_mode_sel =  bcmplpapertaNoFEC;
                }
            }
            if (port_cfg->octal_crossing == bcmAperta2NoOctalCrossing) {
                aperta2_aux_mode->octal_crossing = bcmAperta2NoOctalCrossing;
            } else if (port_cfg->octal_crossing == bcmAperta2SysOctalCrossing) {
                aperta2_aux_mode->octal_crossing = bcmAperta2SysOctalCrossing;
            } else if (port_cfg->octal_crossing == bcmAperta2LineOctalCrossing) {
                aperta2_aux_mode->octal_crossing = bcmAperta2LineOctalCrossing;
            } else {
                PHYMOD_DEBUG_ERROR(("Invalid Octal Crossing %d \n",port_cfg->octal_crossing));
                return PHYMOD_E_PARAM;
            } 
        } else {
            if ((port_cfg->fec_mode >> 8) & 0x1) {
                if (aperta2_aux_mode->fec_mode_sel == bcmAperta2RSFEC) {
                    port_cfg->fec_mode = phymodPaiFecModeExtendedRS528;
                } else if (aperta2_aux_mode->fec_mode_sel == bcmAperta2RS544) {
                    port_cfg->fec_mode = phymodPaiFecModeExtendedRS544;
                } else if (aperta2_aux_mode->fec_mode_sel == bcmAperta2RS544_2XN) {
                    port_cfg->fec_mode = phymodPaiFecModeExtendedRS544_INTERLEAVED;
                } else {
                    port_cfg->fec_mode = phymodPaiFecModeExtendedNone;
                }
            } else {
                port_cfg->fec_mode = phymodPaiFecModeExtendedNone;
            }
        }
    } else if (pai_phy_config->config_type == phymodConfigTypeTraining) {
        phymod_pai_tx_training_t *pai_training_enable = (phymod_pai_tx_training_t*)(pai_phy_config->config_data);
        uint32_t *training_enable = (uint32_t*)(epdm_data);
        if (operation == phymodOperationPai_to_phy) {
            if (pai_training_enable->training_enable) {
                *training_enable = 1;
            } else {
                *training_enable = 0;
            }
        } else {
            if (*training_enable) {
                pai_training_enable->training_enable = 1;
            } else {
                pai_training_enable->training_enable = 0;
            }
        }
    } else {
        return PHYMOD_E_NONE;
    }

    return PHYMOD_E_NONE;
    
}

int plp_aperta2_phy_tx_override_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_tx_override_t* tx_override)
{        
    
    return plp_aperta2_phymod_tscp_driver.f_phymod_phy_tx_override_set(phy, tx_override);
}


int plp_aperta2_phy_tx_override_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_tx_override_t* tx_override)
{        
    
    return plp_aperta2_phymod_tscp_driver.f_phymod_phy_tx_override_get(phy, tx_override);
}


int plp_aperta2_get_port_information(const plp_aperta2_phymod_phy_access_t* phy, phymod_phy_port_information_t* port_information)
{
    int speed = 0, configured_lane = 0, port_num = 0;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG16r_t swgpreg_16;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG17r_t swgpreg_17;
    plp_aperta2_phymod_phy_access_t temp_access;
    uint32_t lane_map = 0;

    PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_lane_speed_get(phy, &speed, &configured_lane, &port_num));
    (void) speed;
    (void) configured_lane;

    if (port_num == 0xFF) {
        PHYMOD_DEBUG_ERROR(("Invalid lanemap\n"));
        return PHYMOD_E_PARAM;
    }
    /* Logical port number, System Identifier and line Identifier*/
    port_information->logical_port = port_num;
    temp_access.port_loc = phymodPortLocSys;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_port_lane_map_get (&temp_access, port_num, &lane_map));
    APERTA2_GET_PORT_FROM_LM(lane_map, port_information->sys_id);
    temp_access.port_loc = phymodPortLocLine;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_port_lane_map_get (&temp_access, port_num, &lane_map));
    APERTA2_GET_PORT_FROM_LM(lane_map, port_information->line_id);

    /* Octal Crossing state*/
    if (port_num < 8) {
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG16r(&phy->access, &swgpreg_16));
        port_information->crossing = ((swgpreg_16.v[0] & (3 << (port_num *2))) >> (port_num *2));
    } else {
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG17r(&phy->access, &swgpreg_17));
        port_information->crossing = ((swgpreg_17.v[0] & (3 << ((port_num-8) *2))) >> ((port_num-8) *2));
    }
    return PHYMOD_E_NONE;
}

#endif /* PHYMOD_APERTA2_SUPPORT */
