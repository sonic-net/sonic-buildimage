/*
*
*  *
*  *
  * $Copyright: (c) 2022 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
*  *
*
*/

/*
 * Includes
 */
#include <phymod/phymod.h>
#include <phymod/phymod_util.h>
#include <tier1/aperta2_pm_seq.h>
#include <tier1/aperta2_reg_access.h>
#include <include/bcm_aperta2_dc3mac_defs.h>
#include <bcm_aperta2_id_defs.h>
#include <bcm_aperta2_direct_defs.h>
#include <peregrine5_pc_types.h>
#include <peregrine5_pc_internal.h>
#include <phymod/phymod.h>
#include <phymod/phymod_system.h>
#include <phymod/phymod.h>
#include <phymod/phymod_dispatch.h>
#include <tscp.h>
#include <tscpmod.h>
#include <tscp_diagnostics.h>
#include <phymod/chip/aperta2.h>

extern __phymod__dispatch__t__ plp_aperta2_phymod_tscp_driver;
extern aperta2_pm_info_t _plp_aperta2_pm_info[APERTA2_MAX_PM_INFO];

int plp_aperta2_convert_speed_to_bits(uint32_t speed)
{
    uint32_t speed_in_bits=0;
    switch(speed)
    {
        case APERTA2_SPEED_800G:
            speed_in_bits = 1;
            break;
        case APERTA2_SPEED_400G:
            speed_in_bits = 2;
            break;
        case APERTA2_SPEED_200G:
            speed_in_bits = 3;
            break;
        case APERTA2_SPEED_100G:
            speed_in_bits = 4;
            break;
        case APERTA2_SPEED_10G:
            speed_in_bits = 5;
            break;
        case APERTA2_SPEED_25G:
            speed_in_bits = 6;
            break;
        case APERTA2_SPEED_50G:
            speed_in_bits = 7;
            break;
       default:
            speed_in_bits = 1; /*set default to 800G */
            break;
    }
    return speed_in_bits;
}

int plp_aperta2_get_speed_from_bits(uint32_t speed_in_bits)
{
    uint32_t speed=0;
    switch(speed_in_bits)
    {
        case 1:
            speed = APERTA2_SPEED_800G;
            break;
        case 2:
            speed = APERTA2_SPEED_400G;
            break;
        case 3:
            speed = APERTA2_SPEED_200G;
            break;
        case 4:
            speed = APERTA2_SPEED_100G;
            break;
        case 5:
            speed = APERTA2_SPEED_10G;
            break;
        case 6:
            speed = APERTA2_SPEED_25G;
            break;
        case 7:
            speed = APERTA2_SPEED_50G;
            break;
       default:
            speed = 1; /*set default to 800G */
            break;

    }
    return speed;
}

int plp_aperta2_write_warmboot_reg(int phy_id , unsigned int type , int value, int lane_index, int lane_map)
{
    
    BCM_APERTA2_DIRECT_CTRL_SWGPREG18r_t  swgpreg_18;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG19r_t  swgpreg_19;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG1Ar_t  swgpreg_1A;
    uint32_t swreg_0 = 0, swreg_1 = 0; /* Per port BCM_APERTA2_DIRECT_SWS_SWREG_000r to
                                          BCM_APERTA2_DIRECT_SWS_SWREG_063r registers, 4 per port*/
    plp_aperta2_phymod_phy_access_t phy_access;
    unsigned char temp = 0;
    
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_phyinfo_from_pminfo(phy_id, &phy_access));

    switch (type) {
        case APERTA2_ISCOREINITIALIZED:
            PHYMOD_IF_ERR_RETURN(
               BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG18r(&phy_access.access, &swgpreg_18)); 
            swgpreg_18.v[0] &= ~1;
            swgpreg_18.v[0] |= value & 1;
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG18r(&phy_access.access, swgpreg_18)); 
        break;
        case APERTA2_ISACTIVE:
            PHYMOD_IF_ERR_RETURN(
               BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG18r(&phy_access.access, &swgpreg_18)); 
            swgpreg_18.v[0] &= ~(1 << APERTA2_ISACTIVE_SHIFT);
            swgpreg_18.v[0] |= (value & 1) << APERTA2_ISACTIVE_SHIFT;
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG18r(&phy_access.access, swgpreg_18)); 
        break;
        case APERTA2_SPEEDIDTABLESTATUS:
            PHYMOD_IF_ERR_RETURN(
               BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG18r(&phy_access.access, &swgpreg_18)); 
            swgpreg_18.v[0] &= ~0xC0;
            swgpreg_18.v[0] |= (value & 3) << APERTA2_SPEEDIDTABLESTATUS_SHIFT;
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG18r(&phy_access.access, swgpreg_18)); 
        break;
        case APERTA2_FWDLOAD:
            PHYMOD_IF_ERR_RETURN(
               BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG18r(&phy_access.access, &swgpreg_18)); 
            swgpreg_18.v[0] &= ~(0x3 << APERTA2_FWDLOAD_SHIFT);
            swgpreg_18.v[0] |= ((value & 3) << APERTA2_FWDLOAD_SHIFT);
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG18r(&phy_access.access, swgpreg_18));
        break;
        case APERTA2_FW_INIT_STATE:
            PHYMOD_IF_ERR_RETURN(
               BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG18r(&phy_access.access, &swgpreg_18)); 
            swgpreg_18.v[0] &= ~(0x3 << APERTA2_FWINITSTATE_SHIFT);
            swgpreg_18.v[0] |= ((value & 3) << APERTA2_FWINITSTATE_SHIFT);
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG18r(&phy_access.access, swgpreg_18));
        break;
        case APERTA2_TVCOPLLACTIVELANEBITMAP:
            swgpreg_19.v[0] = value ;
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG19r(&phy_access.access, swgpreg_19)); 
        break;
        case APERTA2_TVCOPLLADVLANEBITMAP:
            swgpreg_1A.v[0] = value ;
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG1Ar(&phy_access.access, swgpreg_1A)); 
        break;
        case APERTA2_PORT_SYS_FO:
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_read(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_000r + (lane_index*5), &swreg_0));
            swreg_0 = value ;
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_000r+ (lane_index*5), swreg_0));
        break;
        case APERTA2_PORT_LINE_FO:
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_read(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_004r+ (lane_index*5), &swreg_0));
            swreg_0 = (value );
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_004r+ (lane_index*5), swreg_0));
        break;
        case APERTA2_PORT_SYS_SPEED:
            temp = plp_aperta2_convert_speed_to_bits(value);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_read(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_001r+ (lane_index*5), &swreg_1));
            swreg_1 &= ~0xFF;
            swreg_1 |= temp;
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_001r+ (lane_index*5), swreg_1));
        break;
        case APERTA2_PORT_LINE_SPEED:
            temp = plp_aperta2_convert_speed_to_bits(value);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_read(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_001r+ (lane_index*5), &swreg_1));
            swreg_1 &= ~(0xFF << APERTA2_LINE_SPD_SHIFT);
            swreg_1 |= (temp << APERTA2_LINE_SPD_SHIFT);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_001r+ (lane_index*5), swreg_1));
        break;
        case APERTA2_PORT_SYS_LM :
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_002r+ (lane_index*5), value));
        break;
        case APERTA2_PORT_LINE_LM:  
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(&phy_access, BCM_APERTA2_DIRECT_SWS_SWREG_003r+ (lane_index*5), value));
        break;
    }
    return PHYMOD_E_NONE;

}

int plp_aperta2_get_phyinfo_from_pminfo(int phy_id, plp_aperta2_phymod_phy_access_t *phy)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy_id) {
            PHYMOD_MEMCPY(phy, &(_plp_aperta2_pm_info[cnt].pm_info->pm_data.aperta2_pm8x100_gen2_db->int_core_access), sizeof(plp_aperta2_phymod_access_t));
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in getting Phy access\n"));
    return PHYMOD_E_INTERNAL;
}
/*!
 *  Function to update PM Warmboot information
 *
 *  @param phy_id          phy ID
 *  @param wb_idx          Warmboot identifier
 *  @param val             Value to be updated
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
void plp_aperta2_plp_aperta2_update_pm_info(int phy_id, uint32_t wb_idx, int val)
{
     int cnt = 0;
    for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy_id) {
            /* coverity[check_return] */
            plp_aperta2_write_warmboot_reg(phy_id, wb_idx, val, 0 , 0);
        }
    }
}

/*!
 *  Function to Get PM Warmboot information
 *
 *  @param phy_id          phy ID
 *  @param wb_idx          Warmboot identifier
 *  @param val             Value of the identifier
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

void plp_aperta2_plp_aperta2_get_wb_pm_info(int phy_id, uint32_t wb_idx, int *val)
{
    BCM_APERTA2_DIRECT_CTRL_SWGPREG18r_t  swgpreg_18;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG19r_t  swgpreg_19;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG1Ar_t  swgpreg_1A;
    plp_aperta2_phymod_phy_access_t phy_access;
    
    plp_aperta2_get_phyinfo_from_pminfo(phy_id, &phy_access);
    switch (wb_idx) {
        case APERTA2_ISCOREINITIALIZED:
            /*BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG18r(&phy_access.access, &swgpreg_18); 
            *val = swgpreg_18.v[0] & 1;*/
            *val = 0;
        break;
        case APERTA2_ISACTIVE:
            /* coverity[check_return] */
            BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG18r(&phy_access.access, &swgpreg_18);
            *val = (swgpreg_18.v[0] >> APERTA2_ISACTIVE_SHIFT) & 1;
        break;
        case APERTA2_TVCOPLLACTIVELANEBITMAP:
            /* coverity[check_return] */
            BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG19r(&phy_access.access, &swgpreg_19); 
            *val = swgpreg_19.v[0];
        break;
        case APERTA2_TVCOPLLADVLANEBITMAP:
            /* coverity[check_return] */
            BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG1Ar(&phy_access.access, &swgpreg_1A); 
            *val = swgpreg_1A.v[0];
        break;
        case APERTA2_SPEEDIDTABLESTATUS:
            /* coverity[check_return] */
            BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG18r(&phy_access.access, &swgpreg_18); 
            *val = ((swgpreg_18.v[0] >> APERTA2_SPEEDIDTABLESTATUS_SHIFT) & 0x3);
        break;
        default:
           *val = 0;
    }
}

/*!
 *  Function to Perform FW message
 *
 *  @param phy              Phymod access configuration
 *  @param data_rate        port datarate
 *  @param aux              port aux structure 
 *  @param msg              FW message to be performed 
 *  @param data             data to be read/write
 *  @param read             1 to perform read 0 to write
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_send_fw_msg(const plp_aperta2_phymod_phy_access_t *phy, 
                       int data_rate, aperta2_device_aux_modes_t *aux,
                       int msg, void *data, int read)
{
    int port_num = 0, lane = 0, oct = 0;
    int rv = 0;

    (void) lane;
    (void) oct;
    switch (msg) {
        case APERTA2_MSG_CONFIG_PHY: {
            aperta2_config_phy_t *config_phy = (aperta2_config_phy_t*)data;
            if (read == 1) {
                config_phy->port_operation = APERTA2_OP_READ; 
            } else {
                config_phy->port_operation = APERTA2_OP_WRITE; 
            }
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_config_phy(phy, config_phy));
        }
        break;
        case APERTA2_MSG_CLOCK_GEN:         
        break;
        case APERTA2_MSG_CONFIG_PORT:
        {
           aperta2_config_port_t *port_cfg = (aperta2_config_port_t*)data;
           if (read == 1) {
                port_cfg->port_operation = APERTA2_OP_READ; 
           } else {
                port_cfg->port_operation = APERTA2_OP_WRITE; 
           }
            PHYMOD_IF_ERR_RETURN(plp_aperta2_fw_config_port(phy, port_cfg));
        }
        break;
        case APERTA2_MSG_PAUSE_PORT:
            APERTA2_GET_PORT_FROM_LM_SP(data_rate, aux->lane_data_rate, phy->access.lane_mask, port_num, lane, oct);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_port_op (phy, port_num, APERTA2_FUNC_PAUSE_PORT));
        break;
        case APERTA2_MSG_RESUME_PORT:
            APERTA2_GET_PORT_FROM_LM_SP(data_rate, aux->lane_data_rate, phy->access.lane_mask, port_num, lane, oct);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_port_op (phy, port_num, APERTA2_FUNC_RESUME_PORT));
        break;
        case APERTA2_MSG_ENABLE_PORT:
            /*APERTA2_GET_PORT_FROM_LM_SP(data_rate, aux->lane_data_rate, phy->access.lane_mask, port_num, lane, oct);*/
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_port_op (phy, *(uint8_t*)data, APERTA2_FUNC_ENABLE_PORT));
        break;
        case APERTA2_MSG_DISABLE_PORT:
            port_num = *(uint32_t*)data;
            if ((port_num & 0xF0000) == APERTA2_PORT_DISABLE_PH1) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_fw_port_op (phy, (port_num &0xFF), APERTA2_FUNC_PAUSE_PORT));
                rv = plp_aperta2_fw_port_op (phy, (port_num &0xFF), APERTA2_FUNC_FLUSH_PORT);
                if (rv != PHYMOD_E_NONE) {
                    PHYMOD_CRIT_INFO(("Retrying Flush...\n"));
                    rv = plp_aperta2_fw_port_op (phy, (port_num &0xFF), APERTA2_FUNC_FLUSH_PORT);
                    if (rv != PHYMOD_E_NONE) {
                        PHYMOD_CRIT_INFO(("Flush Fails... Ignoring error as Disable port will clear all the Errors\n"));
                    }
                }
            }
            if ((port_num & 0xF0000) == APERTA2_PORT_DISABLE_PH2) {
                uint32_t line_lane_map = 0, sys_lane_map = 0;
                plp_aperta2_phymod_phy_access_t temp_access;

                PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_fw_port_op (phy, (port_num & 0xFF) , APERTA2_FUNC_DISABLE_PORT));

                temp_access.port_loc = phymodPortLocLine;
                PHYMOD_IF_ERR_RETURN(  
                     plp_aperta2_pm_info_port_lane_map_get(&temp_access, (port_num & 0xFF), &line_lane_map));
                
                temp_access.port_loc = phymodPortLocSys;
                PHYMOD_IF_ERR_RETURN(  
                     plp_aperta2_pm_info_port_lane_map_get(&temp_access, (port_num & 0xFF), &sys_lane_map));
                if (line_lane_map != 0) {
                    /* Disable Line PCS*/
                    temp_access.port_loc = phymodPortLocLine;
                    temp_access.access.lane_mask = line_lane_map;
                    plp_aperta2_phymod_tscp_driver.f_phymod_port_enable_set(&temp_access,0);
                }

                if (sys_lane_map != 0) {
                    /* Disable Sys PCS*/
                    temp_access.port_loc = phymodPortLocSys;
                    temp_access.access.lane_mask = sys_lane_map;
                    plp_aperta2_phymod_tscp_driver.f_phymod_port_enable_set(&temp_access,0);
                }

                if (phy->access.lane_mask == 0xFF) {
                    /* call disable post here*/
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_pm_tx_rx_enable_post(phy, 0/*Disable*/, 1/*NON single*/, 0, 0));
                } else {
                    PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
                    temp_access.access.lane_mask = sys_lane_map;
                    /* call disable post here*/
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_pm_tx_rx_enable_post(&temp_access, 0/*Disable*/, 0/*NON single*/, 0, line_lane_map));
                }
                /*Invalidating lane map*/
                PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
                temp_access.access.lane_mask = 0;
                PHYMOD_IF_ERR_RETURN(  
                        plp_aperta2_pm_info_port_speed_set(&temp_access, 0, (port_num&0xFF), 0/*FO lanemap*/, NULL));
                temp_access.port_loc = phymodPortLocLine;
                temp_access.access.lane_mask = 0;
                PHYMOD_IF_ERR_RETURN(  
                        plp_aperta2_pm_info_port_speed_set(&temp_access, 0, (port_num&0xFF), 0 /*FO lanemap*/, NULL));

            }
        break;
        case APERTA2_MSG_FLUSH_PORT:
            APERTA2_GET_PORT_FROM_LM_SP(data_rate, aux->lane_data_rate, phy->access.lane_mask, port_num, lane, oct);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_port_op (phy, port_num, APERTA2_FUNC_FLUSH_PORT));
        break;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_is_fo_enabled(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_phy_inf_config_t* config,
                            unsigned int *is_fo_enabled, unsigned int *pri_port, 
                            unsigned int *primary_lm)
{
    unsigned int num_lanes = 0, num_ports = 0, basic_lane_map = 0x3 ;
    unsigned int port_index = 0, pri_lane_map = 0, lane_map = 0, primary_port = 0 ;
    int rv = 0;
    aperta2_config_port_t temp_port_cfg ;
    aperta2_device_aux_modes_t *auxmode = (aperta2_device_aux_modes_t*)config->device_aux_modes;

    if (phy->access.lane_mask != 0xFFFF) {
        num_lanes = plp_aperta2_phymod_count_set_bits(phy->access.lane_mask);  /* Get num lane from lane mask*/
        num_ports = (16/num_lanes);   /* Get num ports*/

        /* Get the basic lanemap based on number of lanes*/
        if (num_lanes == 1) {
            basic_lane_map =0x1;
        } else if (num_lanes == 2) {
            basic_lane_map =0x3;
        } else if (num_lanes == 4) {
            basic_lane_map =0xF;
        }  else if (num_lanes == 8) {
            basic_lane_map =0xFF;
        }
        for (port_index = 0 ; port_index < num_ports; port_index++) {
            pri_lane_map = (basic_lane_map << (num_lanes * port_index));
            APERTA2_GET_PORT_FROM_LM(pri_lane_map, primary_port);
            temp_port_cfg.PortNum = primary_port;
            rv = plp_aperta2_send_fw_msg(phy, 0, NULL, APERTA2_MSG_CONFIG_PORT, &temp_port_cfg, 1);
            if (rv == PHYMOD_E_NONE) {
                if (temp_port_cfg.PortMode == 1) { /*FO enabled for the port*/
                    lane_map = 0;
                    APERTA2_GET_LM_FROM_PORT(config->data_rate, auxmode->lane_data_rate, temp_port_cfg.FOPortID, lane_map);
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

/*!
 *  Function to initialize PM DB
 *
 *  @param phy_id            Phy id 
 *  @param pm_info           portmacro information to be stored
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_port_dp_init(aperta2_pm_info_t *_laperta2_pm_info, pm_info_t pm_info, int octal_start, int octal_end) 
{
    int port_index = 0, init_speed = 0;
    if (octal_start == APERTA2_OCTAL0) {
        /* Initializing to default speed*/
        /* Line Speed Octal0*/
        if (PM_8x100_GEN2_INFO(pm_info)->tvco == portmodVCO53P125G) {
            init_speed = APERTA2_LINE_INIT_SPEED;
        } else {
            init_speed = APERTA2_SPEED_25G;
        }
        for (port_index = 0; port_index < APERTA2_LANES_PER_OCTAL ; port_index++) {
            _laperta2_pm_info->port_info[port_index].line_speed = init_speed;
            _laperta2_pm_info->port_info[port_index].line_lane_map = 1 << port_index;
            _laperta2_pm_info->port_info[port_index].line_fo_map = 0;
        }
        /* System Speed Octal0*/
        if (PM_8x100_GEN2_INFO(pm_info)->sys_tvco == portmodVCO53P125G) {
            init_speed = APERTA2_LINE_INIT_SPEED;
        } else {
            init_speed = APERTA2_SPEED_25G;
        }
        for (port_index = 0; port_index < APERTA2_LANES_PER_OCTAL ; port_index++) {
            _laperta2_pm_info->port_info[port_index].sys_speed = init_speed;
            _laperta2_pm_info->port_info[port_index].sys_lane_map = 1 << port_index;
            _laperta2_pm_info->port_info[port_index].sys_fo_map = 0;
        }
    }
    if (octal_end == APERTA2_OCTAL1) {
        /* Line Speed Octal1*/
        if (PM_8x100_GEN2_INFO(pm_info)->oc1_tvco == portmodVCO53P125G) {
            init_speed = APERTA2_LINE_INIT_SPEED;
        } else {
            init_speed = APERTA2_SPEED_25G;
        }
        for (port_index = APERTA2_LANES_PER_OCTAL; port_index < (APERTA2_LANES_PER_OCTAL * APERTA2_MAX_OCTAL) ; port_index++) {
            _laperta2_pm_info->port_info[port_index].line_speed = init_speed;
            _laperta2_pm_info->port_info[port_index].line_lane_map = 1 << port_index;
            _laperta2_pm_info->port_info[port_index].line_fo_map = 0;
        }
        /* System Speed Octal1*/
        if (PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco == portmodVCO53P125G) {
            init_speed = APERTA2_LINE_INIT_SPEED;
        } else {
            init_speed = APERTA2_SPEED_25G;
        }
        for (port_index = APERTA2_LANES_PER_OCTAL; port_index < (APERTA2_LANES_PER_OCTAL * APERTA2_MAX_OCTAL) ; port_index++) {
            _laperta2_pm_info->port_info[port_index].sys_speed = init_speed;
            _laperta2_pm_info->port_info[port_index].sys_lane_map = 1 << port_index;
            _laperta2_pm_info->port_info[port_index].sys_fo_map = 0;
        }
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to add PM information
 *
 *  @param phy_id            Phy id 
 *  @param pm_info           portmacro information to be stored
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_add_pm_info(int phy_id, pm_info_t pm_info)
{
    unsigned short cnt = 0;

    for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy_id )
        {
            PHYMOD_DEBUG_ERROR(("PHY already initialized\n"));
            return PHYMOD_E_FAIL;
        }
        if (_plp_aperta2_pm_info[cnt].phy_id == APERTA2_UNINIT_PHYS) {
            _plp_aperta2_pm_info[cnt].phy_id = phy_id;
            _plp_aperta2_pm_info[cnt].pm_info = (pm_info_t)PHYMOD_MALLOC(sizeof(struct pm_info_s), "PM ALLOC");
            PHYMOD_MEMCPY(_plp_aperta2_pm_info[cnt].pm_info, pm_info, sizeof(struct pm_info_s));
            PHYMOD_IF_ERR_RETURN(
                      plp_aperta2_port_dp_init(&_plp_aperta2_pm_info[cnt], pm_info, APERTA2_OCTAL0, APERTA2_OCTAL1));
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in adding PM info\n"));
    return PHYMOD_E_RESOURCE;
}

/*!
 *  Function to remove PM information
 *
 *  @param phy_id            Phy id 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_remove_pm_info(int phy_id)
{
    unsigned short cnt = 0, port_index = 0;
    for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy_id) {
            _plp_aperta2_pm_info[cnt].phy_id = APERTA2_UNINIT_PHYS;
            _plp_aperta2_pm_info[cnt].is_fw_dloaded = 0;
            /* This memory has been allocated in pm_init() */
            PHYMOD_FREE(_plp_aperta2_pm_info[cnt].pm_info->pm_data.aperta2_pm8x100_gen2_db);
            PHYMOD_FREE(_plp_aperta2_pm_info[cnt].pm_info);
            _plp_aperta2_pm_info[cnt].pm_info = NULL ;
            _plp_aperta2_pm_info[cnt].init_state = 0;
            /* Revert back to default speed*/
            for (port_index = 0; port_index < APERTA2_MAX_PORT ; port_index++) {
                _plp_aperta2_pm_info[cnt].port_info[port_index].line_speed = APERTA2_LINE_INIT_SPEED;
                _plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map = 1 << port_index;
                _plp_aperta2_pm_info[cnt].port_info[port_index].sys_speed = APERTA2_SYS_INIT_SPEED;
                _plp_aperta2_pm_info[cnt].port_info[port_index].sys_lane_map = 1 << port_index;
            }
            return PHYMOD_E_NONE;
        }
    }

    PHYMOD_DEBUG_ERROR(("Error in removing PM info\n"));
    return PHYMOD_E_RESOURCE;
}

/*!
 *  Function to get PM information
 *
 *  @param phy_id            Phy id 
 *  @param pm_info           portmacro information to be retrived
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_get_pm_info(int phy_id, pm_info_t pm_info)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy_id) {
            PHYMOD_MEMCPY(pm_info, _plp_aperta2_pm_info[cnt].pm_info, sizeof(struct pm_info_s));
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in getting PM info\n"));
    return PHYMOD_E_INTERNAL;
}

/*!
 *  Function to get FW download status 
 *
 *  @param phy_id            Phy id 
 *  @param active            FW download status
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_pm_is_fw_dloaded_get(int phy_id, uint32_t *active)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy_id) {
            *active = _plp_aperta2_pm_info[cnt].is_fw_dloaded;
            return PHYMOD_E_NONE;
        }
    }
    *active = 0;
    return PHYMOD_E_NONE;
}

/*!
 *  Function to store FW download status 
 *
 *  @param phy_id            Phy id 
 *  @param active            FW download status
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_pm_is_fw_dloaded_set(int phy_id, uint32_t active)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt <APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy_id) {
            _plp_aperta2_pm_info[cnt].is_fw_dloaded = active ;
            PHYMOD_IF_ERR_RETURN(plp_aperta2_write_warmboot_reg( phy_id , APERTA2_FWDLOAD, active, 0, 0 )) ;
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in setting FW Dload\n"));
    return PHYMOD_E_INTERNAL;
}

/*!
 *  Function to update VCO value  
 *
 *  @param phy_id            Phy id 
 *  @param octal             octal0/octal1
 *  @param val               vco value
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_update_vco(plp_aperta2_phymod_phy_access_t *phy, int octal, int val) 
{
    BCM_APERTA2_DIRECT_CTRL_SWGPREG0Cr_t swgpreg_0c;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG0Dr_t swgpreg_0d;
    if (octal == APERTA2_OCTAL0) {
        PHYMOD_IF_ERR_RETURN(
               BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Cr(&phy->access, &swgpreg_0c));
        if (phy->port_loc == phymodPortLocLine) {
            swgpreg_0c.v[0] &= ~0xFF;
            swgpreg_0c.v[0] |= val & 0xFF;
        } else {
            swgpreg_0c.v[0] &= ~0xFF00;
            swgpreg_0c.v[0] |= ((val & 0xFF) << 8);
        }
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Cr(&phy->access, swgpreg_0c));
    } else {
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Dr(&phy->access, &swgpreg_0d));
        if (phy->port_loc == phymodPortLocLine) {
            swgpreg_0d.v[0] &= ~0xFF;
            swgpreg_0d.v[0] |= val & 0xFF;
        } else {
            swgpreg_0d.v[0] &= ~0xFF00;
            swgpreg_0d.v[0] |= ((val & 0xFF) << 8);
        }
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Dr(&phy->access, swgpreg_0d));
    }

    return PHYMOD_E_NONE;
}

/*!
 *  Function to initialize PM 
 *
 *  @param core 
 *  @param init_config
 *  @param core_status
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_pm_init(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config, const plp_aperta2_phymod_core_status_t* core_status)
{
    int unit = 0, index = 0, octal = 0, side = 0;
    portmod_pm_create_info_t pm_create_info;
    portmod_pm_create_info_internal_t pm_internal_info;
    pm_info_t init_pm_info;
    struct pm_info_s init_pm_info_1;
    portmod_pm_vco_setting_t vco_select;
    portmod_speed_config_t speed_config_list[4]; /*Octal0: Sys line Octal 1 : sys line*/
    int start_lane_number; /*Octal0: Sys line Octal 1 : sys line*/
    plp_aperta2_phymod_phy_access_t phy_access, phy_temp;
    aperta2_fw_init_t *fw_init_param = (aperta2_fw_init_t*)(init_config->interface.device_aux_modes);

    if (fw_init_param == NULL) {
        PHYMOD_DEBUG_ERROR(("Invalid Init param\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_MEMSET(speed_config_list, 0, sizeof(portmod_speed_config_t)*4);

    PHYMOD_MEMSET(&init_pm_info_1, 0, sizeof(init_pm_info_1));
    init_pm_info = &init_pm_info_1;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_pm_create_info_t_init(unit, &pm_create_info));
    pm_create_info.phys = 1;
#ifdef PHYMOD_APERTA2_SUPPORT
    pm_create_info.type = portmodDispatchTypePm8x100_gen2;
#endif
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_access_t_init(&pm_create_info.pm_specific_info.pm8x100_gen2.access));

    /* Moving access, MDIO address, bus etc*/
    PHYMOD_MEMCPY(&pm_create_info.pm_specific_info.pm8x100_gen2.access, core, sizeof(plp_aperta2_phymod_phy_access_t));

    pm_create_info.pm_specific_info.pm8x100_gen2.fw_load_method = init_config->firmware_load_method ;
    pm_create_info.pm_specific_info.pm8x100_gen2.fw_load_method &= 0xff;

    /* Use default external loader if NULL */
    pm_create_info.pm_specific_info.pm8x100_gen2.external_fw_loader = NULL;

    /* Polarity */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_polarity_t_init (&(pm_create_info.pm_specific_info.pm8x100_gen2.polarity)));

    /* Lane map */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_lane_map_t_init(&(pm_create_info.pm_specific_info.pm8x100_gen2.lane_map)));


    pm_create_info.pm_specific_info.pm8x100_gen2.lane_map.num_of_lanes = APERTA2_MAX_NUM_LANES;

    vco_select.port_starting_lane_list = &start_lane_number;
    vco_select.num_speeds = 1; /*Sys oc0, ln oc0, sy oc1, ln oc1*/

    if (fw_init_param != NULL) {

        /* line octal 0*/
        if (fw_init_param->octal0.line_vco == aperta2Vco53G) { 
            speed_config_list[0].speed = APERTA2_SPEED_800G;
            speed_config_list[0].num_lane = 8; 
            speed_config_list[0].fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN;
        } else {
            speed_config_list[0].speed = APERTA2_SPEED_100G;
            speed_config_list[0].num_lane = 4; 
            speed_config_list[0].fec = PORTMOD_PORT_PHY_FEC_NONE; 
        }
        /* sys octal 0*/
        if (fw_init_param->octal0.sys_vco == aperta2Vco53G) { 
            speed_config_list[1].speed = APERTA2_SPEED_800G;
            speed_config_list[1].num_lane = 8; 
            speed_config_list[1].fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN; 
        } else {
            speed_config_list[1].speed = APERTA2_SPEED_100G;
            speed_config_list[1].num_lane = 4; 
            speed_config_list[1].fec = PORTMOD_PORT_PHY_FEC_NONE; 
        }
        /* line octal 1*/
        if (fw_init_param->octal1.line_vco == aperta2Vco53G) { 
            speed_config_list[2].speed = APERTA2_SPEED_800G;
            speed_config_list[2].num_lane = 8; 
            speed_config_list[2].fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN; 
        } else {
            speed_config_list[2].speed = APERTA2_SPEED_100G;
            speed_config_list[2].num_lane = 4; 
            speed_config_list[2].fec = PORTMOD_PORT_PHY_FEC_NONE; 
        }
        /*sys octal 1*/
        if (fw_init_param->octal1.sys_vco == aperta2Vco53G) { 
            speed_config_list[3].speed = APERTA2_SPEED_800G;
            speed_config_list[3].num_lane = 8; 
            speed_config_list[3].fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN; 
        } else {
            speed_config_list[3].speed = APERTA2_SPEED_100G;
            speed_config_list[3].num_lane = 4; 
            speed_config_list[3].fec = PORTMOD_PORT_PHY_FEC_NONE; 
        }
    }

    PHYMOD_MEMCPY(&phy_temp, core, sizeof(plp_aperta2_phymod_phy_access_t));
    for (index = 0; index < APERTA2_MAX_PM_INFO; index++) {
        if(APERTA2_UNINIT_PHYS == _plp_aperta2_pm_info[index].phy_id) {
            /* Octal 0 - Sys speed & LM */
            for (octal = APERTA2_OCTAL0; octal <= APERTA2_OCTAL1; octal++) {
                for (side = phymodPortLocLine; side <= phymodPortLocSys; side ++)  {
                    phy_temp.port_loc = side;
                    if (speed_config_list[(side-1)+((octal-1)*2)].speed == APERTA2_SPEED_800G) {
                        phy_temp.access.lane_mask = (0xFF << ((octal == APERTA2_OCTAL1) ? 8 : 0)) ;
                        PHYMOD_IF_ERR_RETURN(
                             plp_aperta2_pm_info_port_speed_set(&phy_temp, APERTA2_SPEED_800G, 0, 0/*Fo lane map*/, NULL));
                    } else if (speed_config_list[(side-1)+((octal-1)*2)].speed == APERTA2_SPEED_400G) {
                        phy_temp.access.lane_mask = (0xF << ((octal == APERTA2_OCTAL1) ? 8 : 0)) ;
                        PHYMOD_IF_ERR_RETURN(
                             plp_aperta2_pm_info_port_speed_set(&phy_temp, APERTA2_SPEED_100G, 0, 0 /*Fo lane map*/, NULL));
                    }
                }
            }
        }
    }
   
    vco_select.speed_config_list= &speed_config_list[0];
    vco_select.speed_config_list->link_training = 0;
    vco_select.speed_config_list->lane_config = 4;
    vco_select.tvco = portmodVCOInvalid;
    vco_select.ovco = portmodVCOInvalid;
    vco_select.is_tvco_new = 0;

    /* Defaults to 312MHZ*/
    pm_create_info.pm_specific_info.pm8x100_gen2.ref_clk = phymodRefClk312Mhz;

    PHYMOD_MEMCPY(&pm_internal_info, &pm_create_info, sizeof(portmod_pm_create_info_internal_t));

    /* Add PM to PortMod */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_pm_init(unit, &pm_internal_info, 0, init_pm_info));

#ifdef PHYMOD_APERTA2_SUPPORT
    init_pm_info->pm_data.aperta2_pm8x100_gen2_db->int_core_access.type = phymodDispatchTypeAperta2;
    PM_8x100_GEN2_INFO(init_pm_info)->int_core_access.type = phymodDispatchTypeAperta2;
    PM_8x100_GEN2_INFO(init_pm_info)->int_core_access.access.tvco_pll_index = 0;
    PM_8x100_GEN2_INFO(init_pm_info)->fw_load_method = init_config->firmware_load_method ;
    PM_8x100_GEN2_INFO(init_pm_info)->external_fw_loader = init_config->firmware_loader ;
    PHYMOD_MEMCPY(&PM_8x100_GEN2_INFO(init_pm_info)->int_phy_access, &PM_8x100_GEN2_INFO(init_pm_info)->int_core_access,
        sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&PM_8x100_GEN2_INFO(init_pm_info)->lane_map, &fw_init_param->line_lane_map, sizeof(plp_aperta2_phymod_lane_map_t));
    PHYMOD_MEMCPY(&PM_8x100_GEN2_INFO(init_pm_info)->sys_lane_map, &fw_init_param->sys_lane_map, sizeof(plp_aperta2_phymod_lane_map_t));

#endif
    PHYMOD_MEMCPY(&phy_access, core, sizeof(plp_aperta2_phymod_phy_access_t));

#ifdef PHYMOD_APERTA2_SUPPORT
    for (octal = APERTA2_OCTAL0; octal <= APERTA2_OCTAL1; octal ++) {
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side ++)  {
            vco_select.num_speeds = 1; /*Sys oc0, ln oc0, sy oc1, ln oc1*/
            vco_select.speed_config_list = &speed_config_list[(side-1) + ((octal-1) *2)];
            start_lane_number = (octal == APERTA2_OCTAL1)  ? 8 : 0; /** for octal 1 and 0 for octal 0*/ 
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_portmod_pm_vcos_get(unit, portmodDispatchTypePm8x100_gen2, 0, &vco_select));
            if (octal == APERTA2_OCTAL0) {
                if (side == phymodPortLocLine) {
                    PM_8x100_GEN2_INFO(init_pm_info)->tvco = vco_select.tvco;
                } else {
                    PM_8x100_GEN2_INFO(init_pm_info)->sys_tvco = vco_select.tvco;
                }
            } else {
                if (side == phymodPortLocLine) {
                    PM_8x100_GEN2_INFO(init_pm_info)->oc1_tvco = vco_select.tvco;
                } else {
                    PM_8x100_GEN2_INFO(init_pm_info)->oc1_sys_tvco = vco_select.tvco;
                }
            }
            phy_access.port_loc = side;
            PHYMOD_IF_ERR_RETURN(
                  plp_aperta2_update_vco(&phy_access, octal, vco_select.tvco)); 
        }
    }
#endif
    PHYMOD_ACC_F_CLAUSE45_SET(&(PM_8x100_GEN2_INFO(init_pm_info)->int_core_access.access));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_add_pm_info(core->access.addr, init_pm_info));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to add core 
 *
 *  @param core 
 *  @param init_config
 *  @param core_status
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_core_add(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_phy_init_config_t* init_config,
                    const plp_aperta2_phymod_core_status_t* core_status, uint8_t octal_start, uint8_t octal_end)
{
    int port = 0; /* This has info about PHY and port of a PM */
    portmod_port_add_info_t add_info;
    int unit = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_portmod_port_add_info_t_init(unit, &add_info));

    /* Clear the load verify flag to speed up the boot time */
    /* Get Pass flag*/
    add_info.flags= init_config->ext_phy_tx_params_user_flag[0] ;
    /* Get Load method*/
    add_info.ilkn_oob_cal_len_tx = init_config->ext_phy_tx_params_user_flag[1]  ;

    /* Initialize both interface_t config and init config */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_portmod_port_init_config_t_init(unit, &add_info.init_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_portmod_port_interface_config_t_init(unit, &add_info.interface_config));

    /* Can Add init config on demand*/
    add_info.autoneg_en = init_config->an_en ;
    add_info.link_training_en = init_config->cl72_en;

    /*Update octal information*/
    add_info.octal_start = octal_start;
    add_info.octal_end = octal_end;

    add_info.interface_config.interface = init_config->interface.interface_type;
    add_info.interface_config.line_interface = init_config->interface.interface_type;
    add_info.interface_config.serdes_interface = init_config->interface.interface_type;
    add_info.interface_config.interface_modes = init_config->interface.interface_modes;
    add_info.interface_config.flags = 0;
    add_info.interface_config.port_refclk_int = init_config->interface.ref_clock;
    add_info.interface_config.port_op_mode = init_config->op_mode;
    add_info.interface_config.speed = init_config->interface.data_rate;
    add_info.interface_config.max_speed = init_config->interface.data_rate;
    add_info.interface_config.encap_mode = _SHR_PORT_ENCAP_IEEE;

    /* it is not used in TSCP, so setting it to 0*/
    add_info.interface_config.pll_divider_req = 0;
    if (add_info.interface_config.speed == 0) {
        add_info.interface_config.speed = APERTA2_SPEED_800G;
        add_info.interface_config.interface = phymodInterfaceCAUI;
        add_info.interface_config.port_num_lanes = 4;
    }

    /* Setting init flags*/
     add_info.interface_config.flags |= (1 << 31);
    /* Considering only for one port of PM*/
    port = APERTA2_PORT_CONSTRUCTION(core->access.addr, 0);
    APERTA2_UPDATE_LM(core->access.addr, 0xFF);
    add_info.phy_op_datapath = phymodDatapathNormal;
     /* Initialize port 0 of a core*/
    PHYMOD_IF_ERR_RETURN(
         plp_aperta2_portmod_core_add(unit, port, 0, &add_info));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to get PM information
 *
 *  @param unit
 *  @param port           port address(added with PHY)
 *  @param pm_info        PM information 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_portmod_pm_info_get(int unit, int port, pm_info_t* pm_info)
{
    int phy_id = APERTA2_GET_PHYID_FROM_PM_PORT(port);
    PHYMOD_IF_ERR_RETURN(plp_aperta2_get_pm_info(phy_id, *pm_info));
    return PHYMOD_E_NONE;
}

/*!
 *  Function to get PM type
 *
 *  @param unit
 *  @param port           port address(added with PHY)
 *  @param type           Get PM type 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_portmod_port_pm_type_get(int unit, int port, portmod_dispatch_type_t* type)
{
    struct pm_info_s pm_info_1;
    pm_info_t pm_info;

    pm_info = &pm_info_1;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_pm_info_get(unit, port, &pm_info));
    *type = pm_info_1.type;

    return PHYMOD_E_NONE;
}

/*!
 *  Function to select tsc clock
 *
 *  @param phy            phy configuration
 *  @param switch_dp_clk  Octal and datapath clock information
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_tsc_clock_sel(const plp_aperta2_phymod_phy_access_t* phy,  aperta2_switch_dpclk_t switch_dp_clk)
{
    PHYMOD_IF_ERR_RETURN(
         plp_aperta2_switch_dp_clock(phy, APERTA2_OP_START, switch_dp_clk));
    
    PHYMOD_IF_ERR_RETURN(
         plp_aperta2_switch_dp_clock(phy, APERTA2_OP_START_RESULT,  switch_dp_clk));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to reset the port active information
 *
 *  @param phy            phy configuration
 *  @param port           port number to de-activate
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_port_active_reset(const plp_aperta2_phymod_phy_access_t* phy, int port)
{
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0;

    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA2_PORT_ACTIVE_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to set port active information
 *
 *  @param phy            phy configuration
 *  @param data_rate      data_rate of the port
 *  @param lane_data_rate  lane_data_rate of the port
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_port_active_set(const plp_aperta2_phymod_phy_access_t* phy, int data_rate, int lane_data_rate)
{
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0, port = 0, lane = 0, octal = 0;
    if (phy->access.lane_mask == 0x0) {
        return PHYMOD_E_PARAM;
    }
#ifdef APERTA2_USE_LINE_SIDE_PORT
    lane_data_rate = (int)data_rate/plp_aperta2_phymod_count_set_bits(phy->access.lane_mask);
    APERTA2_GET_PORT_FROM_LM_SP(data_rate, lane_data_rate, phy->access.lane_mask, port, lane, octal);
#endif
    port = data_rate;
    (void) lane;
    (void) octal;
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA2_PORT_ACTIVE_SHIFT);
    data |= (1 << APERTA2_PORT_ACTIVE_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to set port speed information
 *
 *  @param phy            phy configuration
 *  @param data_rate      data_rate of the port
 *  @param lane_data_rate  lane_data_rate of the port
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_port_speed_set(const plp_aperta2_phymod_phy_access_t* phy, int data_rate, int lane_data_rate)
{
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0, port = 0, lane = 0, prt_speed = 0, octal= 0;

    APERTA2_GET_PORT_FROM_LM_SP(data_rate, lane_data_rate, phy->access.lane_mask, port, lane, octal);
    (void) lane;
    (void) octal;

    APERTA2_PORT_SPEED(data_rate,lane_data_rate,prt_speed);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(0xF << APERTA2_PORT_SPD_SHIFT);
    data |= (prt_speed << APERTA2_PORT_SPD_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to set fault option 
 *
 *  @param phy            phy configuration
 *  @param fault_option    fault option to set on the port
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_pm_fault_option_set(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_fault_option_t fault_option)
{
    int port = 0;
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0;

    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA2_PORT_FAULT_SHIFT);
    data |= (fault_option << APERTA2_PORT_FAULT_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_fault_option_get(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_fault_option_t *fault_option)
{
    int port = 0;
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0;

    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    *fault_option = (data >>APERTA2_PORT_FAULT_SHIFT) & 1;

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_flow_control_set(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_flow_control_t flow_control)
{
    int port = 0;
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0;

    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA2_PORT_FC_SHIFT);
    data |= (flow_control << APERTA2_PORT_FC_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

   return PHYMOD_E_NONE;
}

int plp_aperta2_pm_flow_control_get(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_flow_control_t *flow_control)
{
    int port = 0;
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0;

    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    *flow_control = (data >> APERTA2_PORT_FC_SHIFT) & 1;

   return PHYMOD_E_NONE;
}

int plp_aperta2_pm_store_and_forward_mode_set(const plp_aperta2_phymod_phy_access_t* phy, int enable)
{
    int port = 0;
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0;

    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    data &= ~(1 << APERTA2_PORT_SF_SHIFT);
    data |= (enable << APERTA2_PORT_SF_SHIFT);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(&phy->access, addr[port], data));

   return PHYMOD_E_NONE;
}

int plp_aperta2_pm_store_and_forward_mode_get(const plp_aperta2_phymod_phy_access_t* phy, int *is_enable)
{
    int port = 0;
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0;

    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, addr[port], &data));
    *is_enable = (data >> APERTA2_PORT_SF_SHIFT) & 1;

   return PHYMOD_E_NONE;

}

int plp_aperta2_pm_max_pkt_size_set(const plp_aperta2_phymod_phy_access_t* phy, int size)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_max_packet_size_set(unit, port, size));

    return PHYMOD_E_NONE;
}
int plp_aperta2_pm_max_pkt_size_get(const plp_aperta2_phymod_phy_access_t* phy, int *size)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_max_packet_size_get(unit, port, size));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_runt_threshold_set(const plp_aperta2_phymod_phy_access_t* phy, int threshold)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_runt_threshold_set(unit, port, threshold));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_runt_threshold_get(const plp_aperta2_phymod_phy_access_t* phy, int *threshold)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_runt_threshold_get(unit, port, threshold));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_pad_size_set(const plp_aperta2_phymod_phy_access_t* phy, int pad_size)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_pad_size_set(unit, port, pad_size));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_pad_size_get(const plp_aperta2_phymod_phy_access_t* phy, int *pad_size)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_pad_size_get(unit, port, pad_size));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_tx_mac_sa_set(const plp_aperta2_phymod_phy_access_t* phy, unsigned char mac_sa[6])
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_tx_mac_sa_set(unit, port, mac_sa));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_tx_mac_sa_get(const plp_aperta2_phymod_phy_access_t* phy, unsigned char mac_sa[6])
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_tx_mac_sa_get(unit, port, mac_sa));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_rx_mac_sa_set(const plp_aperta2_phymod_phy_access_t* phy, unsigned char mac_sa[6])
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_rx_mac_sa_set(unit, port, mac_sa));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_rx_mac_sa_get(const plp_aperta2_phymod_phy_access_t* phy, unsigned char mac_sa[6])
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_rx_mac_sa_get(unit, port, mac_sa));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_tx_avg_ipg_set(const plp_aperta2_phymod_phy_access_t* phy, int avg_ipg)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_tx_average_ipg_set(unit, port, avg_ipg));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_tx_avg_ipg_get(const plp_aperta2_phymod_phy_access_t* phy, int *avg_ipg)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_tx_average_ipg_get(unit, port, avg_ipg));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_tx_preamble_length_set(const plp_aperta2_phymod_phy_access_t* phy, int preamble_length)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta2_pm_tx_preamble_length_get(const plp_aperta2_phymod_phy_access_t* phy, int *preamble_length)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta2_pm_pause_control_set(const plp_aperta2_phymod_phy_access_t* phy, portmod_pause_control_t *pause_control)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_pause_control_set(unit, port, pause_control));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_pause_control_get(const plp_aperta2_phymod_phy_access_t* phy, portmod_pause_control_t *pause_control)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_pause_control_get(unit, port, pause_control));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_pfc_control_set(const plp_aperta2_phymod_phy_access_t* phy, portmod_pfc_control_t *pfc_ctrl)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_pfc_control_set(unit, port, pfc_ctrl));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_pfc_control_get(const plp_aperta2_phymod_phy_access_t* phy, portmod_pfc_control_t *pfc_ctrl)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_pfc_control_get(unit, port, pfc_ctrl));

    return PHYMOD_E_NONE;
}


int plp_aperta2_pm_llfc_control_set(const plp_aperta2_phymod_phy_access_t* phy, portmod_llfc_control_t *llfc_ctrl)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_llfc_control_set(unit, port, llfc_ctrl));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_llfc_control_get(const plp_aperta2_phymod_phy_access_t* phy, portmod_llfc_control_t *llfc_ctrl)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_llfc_control_get(unit, port, llfc_ctrl));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_pfc_config_set(const plp_aperta2_phymod_phy_access_t* phy, portmod_pfc_config_t *pfc_config)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_pfc_config_set(unit, port, pfc_config));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_pfc_config_get(const plp_aperta2_phymod_phy_access_t* phy, portmod_pfc_config_t *pfc_config)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_pfc_config_get(unit, port, pfc_config));

    return PHYMOD_E_NONE;
}

int plp_aperta2_rx_mac_enable_set(const plp_aperta2_phymod_phy_access_t* phy, int enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_rx_mac_enable_set(unit, port, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tx_mac_enable_set(const plp_aperta2_phymod_phy_access_t* phy, int enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_tx_mac_enable_set(unit, port, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta2_rx_mac_enable_get(const plp_aperta2_phymod_phy_access_t* phy, int *enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_rx_mac_enable_get(unit, port, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tx_mac_enable_get(const plp_aperta2_phymod_phy_access_t* phy, int *enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_CALL_PM_API(phy, plp_aperta2_portmod_port_tx_mac_enable_get(unit, port, enable));

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_info_port_speed_set(const plp_aperta2_phymod_phy_access_t *phy, int speed, int port_number, int fo_lanemap,
                                   aperta2_device_aux_modes_t *aux_mode)
{
    int cnt = 0, port_status = 0;
    int port_index = 0;
    int octal_crossing_lane_map = 0;

    for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy->access.addr) { 
            if (APERTA2_IS_LINE_SIDE(phy)) {
                _plp_aperta2_pm_info[cnt].port_info[port_number].line_speed = speed;
                _plp_aperta2_pm_info[cnt].port_info[port_number].line_lane_map = phy->access.lane_mask;
                _plp_aperta2_pm_info[cnt].port_info[port_number].line_fo_map = fo_lanemap;
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_LINE_FO, fo_lanemap, port_number, 0));
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_LINE_LM, phy->access.lane_mask, port_number, 0));
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_LINE_SPEED, speed, port_number, 0));
            } else {
                _plp_aperta2_pm_info[cnt].port_info[port_number].sys_speed = speed;
                _plp_aperta2_pm_info[cnt].port_info[port_number].sys_lane_map = phy->access.lane_mask;
                _plp_aperta2_pm_info[cnt].port_info[port_number].sys_fo_map = fo_lanemap;
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_SYS_FO, fo_lanemap, port_number, 0));
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_SYS_LM, phy->access.lane_mask, port_number, 0));
                PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_SYS_SPEED, speed, port_number, 0));
            }
           /* if (fo_lanemap)*/ 
           {
                plp_aperta2_phymod_phy_access_t temp_access;
                unsigned int temp_lanemap = 0;
                PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
                if (fo_lanemap) {
                    temp_lanemap = temp_access.access.lane_mask;
                    temp_access.access.lane_mask = fo_lanemap;
                    PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_tx_rx_status(&temp_access, &port_status));
                } else {
                    temp_lanemap = fo_lanemap;
                    PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_tx_rx_status(&temp_access, &port_status));
                }
                if (aux_mode) {
                    if ((aux_mode->octal_crossing == bcmAperta2LineOctalCrossing) && (phy->port_loc == phymodPortLocLine)) {
                        octal_crossing_lane_map = (phy->access.lane_mask & 0xFF) ? (phy->access.lane_mask << 8) : (phy->access.lane_mask >> 8); 
                    } else if ((aux_mode->octal_crossing == bcmAperta2SysOctalCrossing) && (phy->port_loc == phymodPortLocSys)) {
                        octal_crossing_lane_map = (phy->access.lane_mask & 0xFF) ? (phy->access.lane_mask << 8) : (phy->access.lane_mask >> 8); 
                    } else {
                        octal_crossing_lane_map = 0;
                    }
                }
                if (!port_status) { /* Do this only for not enabled ports*/
                    for (port_index = 0; port_index < APERTA2_MAX_PORT; port_index ++) {
                        if (port_index == port_number) {
                            continue;
                        } 
                        if (APERTA2_IS_LINE_SIDE(phy)) {
                            if (((temp_access.access.lane_mask | octal_crossing_lane_map | temp_lanemap) & _plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map) ||
                                ((temp_access.access.lane_mask | octal_crossing_lane_map | temp_lanemap) & _plp_aperta2_pm_info[cnt].port_info[port_index].line_fo_map)) {
                                _plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map = 0;
                                _plp_aperta2_pm_info[cnt].port_info[port_index].line_fo_map = 0;
                                PHYMOD_IF_ERR_RETURN(
                                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_LINE_FO, 0, port_index, 0));
                                PHYMOD_IF_ERR_RETURN(
                                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_LINE_LM, 0, port_index, 0));
                            } else {
                                continue;
                            }
                        } else {
                            if (((temp_access.access.lane_mask | octal_crossing_lane_map | temp_lanemap) & _plp_aperta2_pm_info[cnt].port_info[port_index].sys_lane_map) ||
                                ((temp_access.access.lane_mask | octal_crossing_lane_map | temp_lanemap) & _plp_aperta2_pm_info[cnt].port_info[port_index].sys_fo_map))   {
                                _plp_aperta2_pm_info[cnt].port_info[port_index].sys_lane_map = 0;
                                _plp_aperta2_pm_info[cnt].port_info[port_index].sys_fo_map = 0;
                                /*Do for line side as it affect disable port((0xf-0xf0),(0xF0-F00))*/
                                _plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map = 0;
                                _plp_aperta2_pm_info[cnt].port_info[port_index].line_fo_map = 0;
                                PHYMOD_IF_ERR_RETURN(
                                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_LINE_FO, 0, port_index, 0));
                                PHYMOD_IF_ERR_RETURN(
                                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_LINE_LM, 0, port_index, 0));
                                PHYMOD_IF_ERR_RETURN(
                                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_SYS_FO, 0, port_index, 0));
                                PHYMOD_IF_ERR_RETURN(
                                        plp_aperta2_write_warmboot_reg(phy->access.addr, APERTA2_PORT_SYS_LM, 0, port_index, 0));
                            } else {
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_info_port_speed_get(const plp_aperta2_phymod_phy_access_t *phy, int port_number, int *speed, int *lane_map)
{
    int cnt = 0, port_index = 0;

    for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy->access.addr) { 
            for (port_index = 0; port_index < APERTA2_MAX_PORT; port_index ++) {
                if (APERTA2_IS_LINE_SIDE(phy)) {
                    if ((phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map) ||
                         (phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].line_fo_map)) {
                        *speed = _plp_aperta2_pm_info[cnt].port_info[port_index].line_speed;
                      
                        if (phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map) {
                            *lane_map = _plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map;
                        } else {
                            *lane_map = _plp_aperta2_pm_info[cnt].port_info[port_index].line_fo_map;
                        }
                        return PHYMOD_E_NONE;
                    } else {
                        continue;
                    }
                } else {
                    if ((phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].sys_lane_map) ||
                         (phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].sys_fo_map))   {
                        *speed = _plp_aperta2_pm_info[cnt].port_info[port_index].sys_speed;
                        if (phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].sys_lane_map) {
                            *lane_map = _plp_aperta2_pm_info[cnt].port_info[port_index].sys_lane_map;
                        } else {
                            *lane_map = _plp_aperta2_pm_info[cnt].port_info[port_index].sys_fo_map;
                        }
                        return PHYMOD_E_NONE;
                    } else {
                        continue;
                    }
                }
            }
        }
    }
    (void) port_number;
    return PHYMOD_E_NONE;
}
/*!
 *  Function to get lanemap from port 
 *
 *  @param phy            phy configuration
 *  @param port_number    port_number to acces
 *  @param lane_map       output, lane map of a port
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_pm_info_port_lane_map_get(const plp_aperta2_phymod_phy_access_t *phy, int port_number, uint32_t *lane_map)
{
    int cnt = 0;
    for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy->access.addr) { 
            if (APERTA2_IS_LINE_SIDE(phy)) {
                *lane_map = _plp_aperta2_pm_info[cnt].port_info[port_number].line_lane_map;
                return PHYMOD_E_NONE;
            } else {
                *lane_map = _plp_aperta2_pm_info[cnt].port_info[port_number].sys_lane_map;
                return PHYMOD_E_NONE;
            }
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_info_lane_speed_get(const plp_aperta2_phymod_phy_access_t *phy, int *speed, int *configured_lane, int *port_num)
{
    int cnt = 0, port_index = 0;
    for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy->access.addr) { 
            for (port_index = 0; port_index < APERTA2_MAX_PORT; port_index ++) {
                if (APERTA2_IS_LINE_SIDE(phy)) {
                    if ((_plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map & phy->access.lane_mask) ||  
                         (phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].line_fo_map)) {
                        if (phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map) {
                            *configured_lane = _plp_aperta2_pm_info[cnt].port_info[port_index].line_lane_map;
                        } else {
                            *configured_lane = _plp_aperta2_pm_info[cnt].port_info[port_index].line_fo_map;
                        }
                        *speed = _plp_aperta2_pm_info[cnt].port_info[port_index].line_speed;
                        *port_num = port_index;
                        return PHYMOD_E_NONE;
                    }
                }
                if (APERTA2_IS_SYSTEM_SIDE(phy)) {
                    if ((_plp_aperta2_pm_info[cnt].port_info[port_index].sys_lane_map & phy->access.lane_mask)  ||  
                         (phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].sys_fo_map)) {
                        *speed = _plp_aperta2_pm_info[cnt].port_info[port_index].sys_speed;
                        *port_num = port_index;
                        if (phy->access.lane_mask & _plp_aperta2_pm_info[cnt].port_info[port_index].sys_lane_map) {
                            *configured_lane = _plp_aperta2_pm_info[cnt].port_info[port_index].sys_lane_map;
                        } else {
                            *configured_lane = _plp_aperta2_pm_info[cnt].port_info[port_index].sys_fo_map;
                        }
                        return PHYMOD_E_NONE;
                    }
                }
            }
            /*Unallocated port*/
            *speed = 0;
            *port_num = 0xff;
            return PHYMOD_E_NONE;
        }
    }
    *speed = 100000;
    PHYMOD_DEBUG_ERROR(("PM info speed get is not correct\n"));
    return PHYMOD_E_CONFIG;
}

int
_plp_aperta2_get_lane_config_word(const plp_aperta2_phymod_phy_inf_config_t *config,
        int *lane_config_word) {
    aperta2_device_aux_modes_t *aux_mode = (aperta2_device_aux_modes_t *) config->device_aux_modes;
    *lane_config_word = 0;
    if (config->interface_type == phymodInterfaceSFI) {
        PORTMOD_PORT_PHY_LANE_CONFIG_MEDIUM_SET(*lane_config_word, phymodFirmwareMediaTypeOptics);
    } else if ((config->interface_type == phymodInterfaceCR) ||
               (config->interface_type == phymodInterfaceCR2)||
               (config->interface_type == phymodInterfaceCR4)) {
        PORTMOD_PORT_PHY_LANE_CONFIG_MEDIUM_SET(*lane_config_word, phymodFirmwareMediaTypeCopperCable);
    } else {
        /*C2M, C2C, KR,MR,ER,LR,VSR, XFI, SR*/
        PORTMOD_PORT_PHY_LANE_CONFIG_MEDIUM_SET(*lane_config_word, phymodFirmwareMediaTypePcbTraceBackPlane);
    }

    if (aux_mode->modulation_mode == bcmAperta2ModulationPAM4){
        PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_PAM4_SET(*lane_config_word);
        PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NS_SET(*lane_config_word);
        if (PORTMOD_PORT_PHY_LANE_CONFIG_MEDIUM_GET(*lane_config_word) ==  phymodFirmwareMediaTypeOptics) {
            PHYMOD_DEBUG_ERROR(("Optical interface is not supported in PAM4\n"));
            return PHYMOD_E_PARAM;
        }
    } else {
        PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_NRZ_SET(*lane_config_word);
    }
    if (config->interface_type == phymodInterfaceCAUI4_C2M ||
        config->interface_type == phymodInterfaceAUI_C2M ||
        config->interface_type == phymodInterfaceVSR   || 
        config->interface_type == phymodInterfaceCEIMR) {
        /* Rx low power*/
        PORTMOD_PORT_PHY_LANE_CONFIG_LP_DFE_SET(*lane_config_word);
    }

    return PHYMOD_E_NONE;
}

/*!
 *  Function to get FW port speed from user datarate and lane datarate
 *
 *  @param phy            phy configuration
 *  @param auxmode        aux parameter
 *  @param *fw_speed      output, Firmware port speed
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int _plp_aperta2_get_fw_speed (uint32_t port_data_rate, aperta2_device_aux_modes_t auxmode, uint8_t *fw_speed) {

    if (port_data_rate == APERTA2_SPEED_10G) {
        *fw_speed = APERTA2_SPEED_10G_1x10G;
    } else if (port_data_rate == APERTA2_SPEED_25G) {
        *fw_speed = APERTA2_SPEED_25G_1x25G;
    } else if (port_data_rate == APERTA2_SPEED_50G &&
               auxmode.modulation_mode == bcmAperta2ModulationNRZ) {
        *fw_speed = APERTA2_SPEED_50G_2x25G;
    } else if (port_data_rate == APERTA2_SPEED_50G &&
            auxmode.modulation_mode == bcmAperta2ModulationPAM4) {
        *fw_speed = APERTA2_SPEED_50G_1x50G;
    } else if (port_data_rate == APERTA2_SPEED_100G &&
            auxmode.modulation_mode == bcmAperta2ModulationNRZ) {
        *fw_speed = APERTA2_SPEED_100G_4x25G;
    } else if (port_data_rate == APERTA2_SPEED_100G &&
               auxmode.lane_data_rate == bcmAperta2LaneDataRate_53P125G) {
        *fw_speed = APERTA2_SPEED_100G_2x50G;
    } else if (port_data_rate == APERTA2_SPEED_100G &&
               auxmode.lane_data_rate == bcmAperta2LaneDataRate_106P25G) {
        *fw_speed = APERTA2_SPEED_100G_1x100G;
    } else if (port_data_rate == APERTA2_SPEED_200G &&
              auxmode.lane_data_rate == bcmAperta2LaneDataRate_53P125G) {
        *fw_speed = APERTA2_SPEED_200G_4x50G;
    } else if (port_data_rate == APERTA2_SPEED_200G &&
              auxmode.lane_data_rate == bcmAperta2LaneDataRate_106P25G) {
        *fw_speed = APERTA2_SPEED_200G_2x100G;
    } else if (port_data_rate == APERTA2_SPEED_400G &&
              auxmode.lane_data_rate == bcmAperta2LaneDataRate_53P125G) {
        *fw_speed = APERTA2_SPEED_400G_8x50G;
    } else if (port_data_rate == APERTA2_SPEED_400G &&
              auxmode.lane_data_rate == bcmAperta2LaneDataRate_106P25G) {
        *fw_speed = APERTA2_SPEED_400G_4x100G;
    }else if (port_data_rate == APERTA2_SPEED_800G ) {
        *fw_speed = APERTA2_SPEED_800G_8x100G;
    } else {
        PHYMOD_DEBUG_ERROR(("Incorrect datarate\n"));
        return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;

}
/*!
 *  Function to get user port speed and lane data rate from FW speed
 *
 *  @param fw_speed       FW speed
 *  @param data_rate          output user speed
 *  @param ldr            output lane data rate
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int _plp_aperta2_get_dr_ldr_from_fw_speed (uint8_t fw_speed, uint32_t *data_rate, uint32_t *ldr) {

    if (fw_speed == APERTA2_SPEED_10G_1x10G) {
        *data_rate = APERTA2_SPEED_10G;
        *ldr = bcmAperta2LaneDataRate_10P3125G;
    } else if (fw_speed == APERTA2_SPEED_25G_1x25G) { 
        *data_rate = APERTA2_SPEED_25G;
        *ldr = bcmAperta2LaneDataRate_25P78125G;
    } else if (fw_speed == APERTA2_SPEED_50G_2x25G) { 
        *data_rate = APERTA2_SPEED_50G;
        *ldr = bcmAperta2LaneDataRate_25P78125G;
    } else if (fw_speed == APERTA2_SPEED_50G_1x50G) { 
        *data_rate = APERTA2_SPEED_50G;
        *ldr = bcmAperta2LaneDataRate_53P125G;
    } else if (fw_speed == APERTA2_SPEED_100G_4x25G) { 
        *data_rate = APERTA2_SPEED_100G;
        *ldr = bcmAperta2LaneDataRate_25P78125G;
    } else if (fw_speed == APERTA2_SPEED_100G_2x50G) { 
        *data_rate = APERTA2_SPEED_100G;
        *ldr = bcmAperta2LaneDataRate_53P125G;
    } else if (fw_speed == APERTA2_SPEED_100G_1x100G) { 
        *data_rate = APERTA2_SPEED_100G;
        *ldr = bcmAperta2LaneDataRate_106P25G;
    } else if (fw_speed == APERTA2_SPEED_200G_4x50G) { 
        *data_rate = APERTA2_SPEED_200G;
        *ldr = bcmAperta2LaneDataRate_53P125G;
    } else if (fw_speed == APERTA2_SPEED_200G_2x100G) { 
        *data_rate = APERTA2_SPEED_200G;
        *ldr = bcmAperta2LaneDataRate_106P25G;
    } else if (fw_speed == APERTA2_SPEED_400G_8x50G) { 
        *data_rate = APERTA2_SPEED_400G;
        *ldr = bcmAperta2LaneDataRate_53P125G;
    } else if (fw_speed == APERTA2_SPEED_400G_4x100G) { 
        *data_rate = APERTA2_SPEED_400G;
        *ldr = bcmAperta2LaneDataRate_106P25G;
    }else if (fw_speed == APERTA2_SPEED_800G_8x100G) { 
        *data_rate = APERTA2_SPEED_800G;
        *ldr = bcmAperta2LaneDataRate_106P25G;

    } else {
        PHYMOD_DEBUG_ERROR(("Incorrect datarate\n"));
        return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;

}
int _plp_aperta2_fill_port_cfg(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_phy_inf_config_t* config, aperta2_config_port_t *fw_port_config)
{
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t data = 0, line_no_lanes = 0, lane = 0, port = 0;
    uint32_t sys_no_lanes = 0, octal = 0;
    aperta2_device_aux_modes_t auxmode = *(aperta2_device_aux_modes_t*)config->device_aux_modes;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG0Er_t swgpreg_0e;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG10r_t swgpreg_10;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG16r_t swgpreg_16;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG17r_t swgpreg_17;

    APERTA2_GET_PORT_FROM_LM_SP(config->data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, lane, octal);
    (void)lane;
    (void)octal;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, APERTA2_SYSTEM_SIDE_NO_OF_LANES, &sys_no_lanes));

    line_no_lanes = plp_aperta2_phymod_count_set_bits(phy->access.lane_mask);
    fw_port_config->PortNum = fw_port_config->SPMPortID = (sys_no_lanes & 0xF0) >> 4;
    fw_port_config->LPMPortID =  port;
    fw_port_config->PortType = ((sys_no_lanes & 0xF) == line_no_lanes) ? bcmAperta2PortTypePassthrough :
                           ((sys_no_lanes & 0xF) > line_no_lanes) ? bcmAperta2PortTypeGearBox :
                            bcmAperta2PortTypeReverseGearBox;
    fw_port_config->PortMode =  0;    

    /* Get the system port speed from GPREG*/
    fw_port_config->SysPortSpeed = (sys_no_lanes >> 8) & 0xFF;
    PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_get_fw_speed (config->data_rate, auxmode, &fw_port_config->LinePortSpeed));

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_direct_reg_read(phy, addr[fw_port_config->PortNum], &data));
    if (data & (1 << APERTA2_PORT_SF_SHIFT)) {
        fw_port_config->PortOptions |= 1;
    }
    if (data & (1 << APERTA2_PORT_FC_SHIFT)) {
        fw_port_config->PortOptions |= 2;
    }
    if (data & (1 << APERTA2_PORT_FAULT_SHIFT)) {
        fw_port_config->PortOptions |= 4;
    }
    if (auxmode.ing_fixed_latency.enable) {
        fw_port_config->PortOptions |= 0x10;
        fw_port_config->IngFixedLatency = auxmode.ing_fixed_latency.dp_ck_cycles;
    }
    if (auxmode.ing_fixed_latency.start_point) {
        fw_port_config->PortOptions |= 0x20;
    }
    if (auxmode.egr_fixed_latency.enable) {
        fw_port_config->PortOptions |= 0x40;
        fw_port_config->EgrFixedLatency = auxmode.egr_fixed_latency.dp_ck_cycles;
    }
    if (auxmode.egr_fixed_latency.start_point) {
        fw_port_config->PortOptions |= 0x80;
    }

    fw_port_config->PortOptions |= (auxmode.ts_config) << 8;

    PHYMOD_IF_ERR_RETURN(
       BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Er(&phy->access, &swgpreg_0e));
    PHYMOD_IF_ERR_RETURN(
       BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG10r(&phy->access, &swgpreg_10));
    
    if (swgpreg_0e.v[0]) {
        /*Enable system side F0*/
        if ((swgpreg_10.v[0]) & 1) { /* Tx back pressure or'ed*/
            fw_port_config->FOOptions = 0x2;
        } else { /* just for this port*/
            fw_port_config->FOOptions = 0x0;
        }
        fw_port_config->PortMode =  1;
        /* FO on sustem side*/
        fw_port_config->FOOptions &= ~(0x1);
        APERTA2_GET_PORT_FROM_LM((swgpreg_0e.v[0]), fw_port_config->FOPortID);
        if (auxmode.failover_config.lane_map != 0) {
            PHYMOD_DEBUG_ERROR(("Failover cannot be configured for both line and System side\n"));
            return PHYMOD_E_PARAM;
        }
    }
    /* Line side Failover*/
    if (auxmode.failover_config.lane_map != 0) {
        /*Enable Line side FO*/
        fw_port_config->FOOptions = 0x1;
        if (auxmode.failover_config.tx_back_pressure_mode & 1) { /* Back pressure ORed from active port*/
            fw_port_config->FOOptions |= 0x2;
        } else {   /* Just for this port*/
            /* nothing needed default to it */
        }
        fw_port_config->PortMode =  1;
        APERTA2_GET_PORT_FROM_LM(auxmode.failover_config.lane_map, fw_port_config->FOPortID);
    }
    if (fw_port_config->PortNum < 8) {
        PHYMOD_IF_ERR_RETURN(
           BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG16r(&phy->access, &swgpreg_16));
        swgpreg_16.v[0] &= ~(3 << (fw_port_config->PortNum *2));
        swgpreg_16.v[0] |= (auxmode.octal_crossing << (fw_port_config->PortNum *2));
        PHYMOD_IF_ERR_RETURN(
           BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG16r(&phy->access, swgpreg_16));
    } else {
        PHYMOD_IF_ERR_RETURN(
           BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG17r(&phy->access, &swgpreg_17));
        swgpreg_17.v[0] &= ~(3 << ((fw_port_config->PortNum-8) *2));
        swgpreg_17.v[0] |= (auxmode.octal_crossing << ((fw_port_config->PortNum-8) *2));
        PHYMOD_IF_ERR_RETURN(
           BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG17r(&phy->access, swgpreg_17));
    }

    if (auxmode.octal_crossing == bcmAperta2LineOctalCrossing) {
        /* Configure FW octal Crossing*/
        fw_port_config->PortOptions |= (1 << 11);
    } else {
        fw_port_config->PortOptions &= ~(1 << 11);
    }

    fw_port_config->port_operation = APERTA2_OP_WRITE;

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_interface_config_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta2_phymod_phy_inf_config_t* config, 
                                    uint32_t line_lane_map, const plp_aperta2_phymod_phy_inf_config_t *line_config)
{
    int int_port = 0, unused = 0, octal = 0, lane = 0;
    unsigned int port = 0;
    int prev_speed = 0, prev_lanemap = 0, prev_port = 0;
    portmod_speed_config_t speed_config, line_speed_config;
    struct pm_info_s pm_info_temp;
    aperta2_config_port_t fw_port_config ;
    pm_info_t pm_info = &pm_info_temp;
    aperta2_device_aux_modes_t auxmode = *(aperta2_device_aux_modes_t*)config->device_aux_modes;
    aperta2_device_aux_modes_t line_auxmode = *(aperta2_device_aux_modes_t*)line_config->device_aux_modes;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG0Er_t swgpreg_0e;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG10r_t swgpreg_10;
    plp_aperta2_phymod_phy_access_t fo_phy, *pfo_phy;
    plp_aperta2_phymod_phy_access_t temp_acc;
    pfo_phy = &fo_phy;
 
    PHYMOD_MEMCPY(pfo_phy, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&fw_port_config, 0, sizeof(aperta2_config_port_t));
    PHYMOD_MEMSET(&pm_info_temp, 0, sizeof(struct pm_info_s));
    PHYMOD_MEMSET(&speed_config, 0, sizeof(portmod_speed_config_t));
    PHYMOD_MEMSET(&line_speed_config, 0, sizeof(portmod_speed_config_t));
    PHYMOD_MEMSET(&swgpreg_0e, 0, sizeof(BCM_APERTA2_DIRECT_CTRL_SWGPREG0Er_t));
    PHYMOD_MEMSET(&swgpreg_10, 0, sizeof(BCM_APERTA2_DIRECT_CTRL_SWGPREG10r_t));
    PHYMOD_MEMCPY(&temp_acc, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);
    speed_config.speed = config->data_rate;
    speed_config.num_lane = plp_aperta2_phymod_count_set_bits(phy->access.lane_mask);
    speed_config.link_training = 0;
    line_speed_config.speed = line_config->data_rate;
    line_speed_config.num_lane = plp_aperta2_phymod_count_set_bits(line_lane_map);
    line_speed_config.link_training = 0;


   APERTA2_GET_PORTMODE_FEC(auxmode.fec_mode_sel, speed_config.fec);
   APERTA2_GET_PORTMODE_FEC(line_auxmode.fec_mode_sel, line_speed_config.fec);
   speed_config.modulation = auxmode.modulation_mode;
   line_speed_config.modulation = line_auxmode.modulation_mode;

    if (APERTA2_IS_LINE_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Er(&phy->access, &swgpreg_0e));
    } else {
        /* System side if failover enabled*/
        if (auxmode.failover_config.lane_map != 0) {
            swgpreg_0e.v[0] = auxmode.failover_config.lane_map ;
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Er(&phy->access, swgpreg_0e));
            swgpreg_10.v[0] = auxmode.failover_config.tx_back_pressure_mode ;
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG10r(&phy->access, swgpreg_10));
        }
    }

    /* Disable previously allocated port*/
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_pm_info_lane_speed_get(phy, &prev_speed, &prev_lanemap, &prev_port));
    if (prev_speed == 0) { /* Does not match system side. Check line side MAP */
        PHYMOD_MEMCPY(&temp_acc, phy, sizeof(plp_aperta2_phymod_phy_access_t));
        temp_acc.port_loc = phymodPortLocLine;
        temp_acc.access.lane_mask = line_lane_map;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_lane_speed_get(&temp_acc, &prev_speed, &prev_lanemap, &prev_port));
        if (prev_speed == 0 && ((auxmode.failover_config.lane_map != 0) ||
                                (line_auxmode.failover_config.lane_map != 0))) {
            temp_acc.port_loc = (auxmode.failover_config.lane_map != 0) ? phymodPortLocSys: phymodPortLocLine;
            temp_acc.access.lane_mask = (auxmode.failover_config.lane_map != 0) ? auxmode.failover_config.lane_map : 
                                         line_auxmode.failover_config.lane_map;
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_pm_info_lane_speed_get(&temp_acc, &prev_speed, &prev_lanemap, &prev_port));
        }
    }

    PHYMOD_CRIT_INFO(("Prev speed :%d prev_port:%d prev_lanemap:%x\n", prev_speed, prev_port, prev_lanemap));
    /* Handling Special cases in octal crossing with
     * unused lanes */
    if (APERTA2_IS_SYSTEM_SIDE(phy) && (prev_speed == 0)) {
        if (auxmode.octal_crossing != bcmAperta2NoOctalCrossing) {
            PHYMOD_IF_ERR_RETURN(
               plp_aperta2_disable_port(phy, config, prev_port, APERTA2_PORT_DISABLE_PH1, line_lane_map, line_config));
            PHYMOD_IF_ERR_RETURN(
               plp_aperta2_disable_port(phy, config, prev_port, APERTA2_PORT_DISABLE_PH2, line_lane_map, line_config));
        } else {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_disable_octal_crossed_port (phy, config, line_lane_map, line_config));
        }
    }
    /* Creating Disable in phase wise to support aperta kind of disable needed 
     * in future*/
    if (APERTA2_IS_SYSTEM_SIDE(phy) && (prev_speed != 0)) {
        /* Phase1 Disable*/
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_disable_port(phy, config, prev_port, APERTA2_PORT_DISABLE_PH1, line_lane_map, line_config));
    }
    if (APERTA2_IS_SYSTEM_SIDE(phy)) {
        if (prev_speed != 0) {
            /* Phase2 Disable for a port*/
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_disable_port(phy, config, prev_port, APERTA2_PORT_DISABLE_PH2, line_lane_map, line_config));
        }
        /* Update new speed*/
        APERTA2_GET_PORT_FROM_LM_SP(config->data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, lane, octal);
        (void)lane;
    } else {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_read(phy, APERTA2_SYSTEM_SIDE_NO_OF_LANES, &port ));
        /* Get system port number to update the lane map on line side*/
        port = (port >> 4) & 0xF;
    }
    
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_port_speed_set(phy, config->data_rate, port, auxmode.failover_config.lane_map, &auxmode));
   
    PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_get_lane_config_word(config, &speed_config.lane_config));
    
    if (APERTA2_IS_SYSTEM_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_reconfigure_octal_pll(phy, &speed_config, &line_speed_config));
        /* Update port only if it is initialized by reconfig*/
        if (speed_config.flags == 0xF) {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_pm_info_port_speed_set(phy, config->data_rate, port, auxmode.failover_config.lane_map, &auxmode));
            speed_config.flags = 0;
        }

        /* Need to update here again as re-configure PLL will change the access lane*/
        APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);
    }
    if (APERTA2_IS_LINE_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
           _plp_aperta2_fill_port_cfg(phy, config, &fw_port_config));
        PHYMOD_CRIT_INFO(("port:%d port_typ:%d port_mode:%d, sysspeed:%d linespeed:%d spmid:%d lpmid:%d port_opt:%d Ilat:%d  eglat:%x foopt:%d, fp_pnu:%d \n",
        fw_port_config.PortNum, fw_port_config.PortType, fw_port_config.PortMode, fw_port_config.SysPortSpeed, fw_port_config.LinePortSpeed, fw_port_config.SPMPortID,   fw_port_config.LPMPortID,
        fw_port_config.PortOptions, fw_port_config.IngFixedLatency,fw_port_config.EgrFixedLatency, fw_port_config.FOOptions, fw_port_config.FOPortID));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_send_fw_msg(phy, config->data_rate, &auxmode,
                       APERTA2_MSG_CONFIG_PORT, &fw_port_config, 0/*Write/read*/));
    } else {
        int system_port = 0;
        uint8_t fw_speed = 0;
        APERTA2_GET_PORT_FROM_LM_SP(config->data_rate,auxmode.lane_data_rate, phy->access.lane_mask, system_port,unused, octal);
        (void) unused;
        (void) octal;
        system_port = system_port << 4;
        system_port |=  speed_config.num_lane & 0xF;
        /* Store System FW port speed*/
        PHYMOD_IF_ERR_RETURN( 
                _plp_aperta2_get_fw_speed(config->data_rate, auxmode, &fw_speed));
        system_port |= (fw_speed << 8);
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(phy, APERTA2_SYSTEM_SIDE_NO_OF_LANES, system_port ));
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_write(phy, BCM_APERTA2_DIRECT_CTRL_SWGPREG11r, phy->access.lane_mask));
    }
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_pm_info(phy->access.addr, pm_info));

    if ((auxmode.octal_crossing == bcmAperta2LineOctalCrossing) && (phy->port_loc == phymodPortLocLine)) {
        plp_aperta2_phymod_phy_access_t octal_access;
        PHYMOD_MEMCPY(&octal_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
        /* Configuring other octal portmode*/
        octal_access.access.lane_mask = (phy->access.lane_mask & 0xFF) ? (phy->access.lane_mask << 8) : (phy->access.lane_mask >> 8); 
        APERTA2_OCT_CROSS_CLEAR_USED_LM(octal_access) ;
        if (octal_access.access.lane_mask != 0) {
            PHYMOD_IF_ERR_RETURN
            (plp_aperta2_dc3port_port_mode_set(&octal_access, 0/*flags*/, octal_access.access.lane_mask));
        }
    } else if ((auxmode.octal_crossing == bcmAperta2SysOctalCrossing) && (phy->port_loc == phymodPortLocSys)) {
        plp_aperta2_phymod_phy_access_t octal_access;
        PHYMOD_MEMCPY(&octal_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
        /* Configuring other octal portmode*/
        octal_access.access.lane_mask = (phy->access.lane_mask & 0xFF) ? (phy->access.lane_mask << 8) : (phy->access.lane_mask >> 8); 
        APERTA2_OCT_CROSS_CLEAR_USED_LM(octal_access) ;
        if (octal_access.access.lane_mask != 0) {
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_dc3port_port_mode_set(&octal_access, 0 /*flags*/, octal_access.access.lane_mask));
        }
    } else {
        /* Do Nothing*/
    }

    port = APERTA2_PORT_CONSTRUCTION(phy->access.addr, int_port);
    /* Using Unit as port type, to avoid lot of changes */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_port_speed_config_set(auxmode.port_type, port, &speed_config));
    plp_aperta2_phymod_tscp_driver.f_phymod_port_enable_set(phy,1);

    PHYMOD_IF_ERR_RETURN(
        BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Er(&phy->access, &swgpreg_0e));
    if (swgpreg_0e.v[0] && (phy->port_loc == phymodPortLocSys)) {
        pfo_phy->access.lane_mask = swgpreg_0e.v[0] ;
        pfo_phy->port_loc = phymodPortLocSys;
        APERTA2_GET_PORT_UPDATE_PM_INFO_LM(pfo_phy, port);
        /* Using Unit as port type, to avoid lot of changes */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_port_speed_config_set(auxmode.port_type, port, &speed_config));
        plp_aperta2_phymod_tscp_driver.f_phymod_port_enable_set(pfo_phy,1);
        APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    }
    if (auxmode.failover_config.lane_map != 0 && (phy->port_loc == phymodPortLocLine)) {
        pfo_phy->access.lane_mask = auxmode.failover_config.lane_map;
        pfo_phy->port_loc = phymodPortLocLine;
        APERTA2_GET_PORT_UPDATE_PM_INFO_LM(pfo_phy, port);
        /* Using Unit as port type, to avoid lot of changes */
        PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_port_speed_config_set(auxmode.port_type, port, &speed_config));
        plp_aperta2_phymod_tscp_driver.f_phymod_port_enable_set(pfo_phy,1);
        APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    }

    if (APERTA2_IS_LINE_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
            /*plp_aperta2_port_active_set(phy, config->data_rate, auxmode.lane_data_rate));*/
            plp_aperta2_port_active_set(phy, fw_port_config.PortNum, auxmode.lane_data_rate));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_send_fw_msg(phy,config->data_rate, &auxmode,
                       APERTA2_MSG_ENABLE_PORT, &fw_port_config.PortNum, 0/*Read/write*/));
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_tx_rx_enable( phy, 1/*Enable*/, ((config->data_rate == APERTA2_SPEED_800G) ? 1 : 0), 0/*Failover disabled*/));
        /* Enable Failover lane*/
        if (swgpreg_0e.v[0]) {
            pfo_phy->access.lane_mask = swgpreg_0e.v[0];
            pfo_phy->port_loc = phymodPortLocSys;
        }
        if (auxmode.failover_config.lane_map != 0) {
            pfo_phy->access.lane_mask = auxmode.failover_config.lane_map;
            pfo_phy->port_loc = phymodPortLocLine;
        }
        if ((auxmode.failover_config.lane_map != 0) || (swgpreg_0e.v[0])) {
            APERTA2_GET_PORT_UPDATE_PM_INFO_LM(pfo_phy, port);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_pm_tx_rx_enable(pfo_phy, 1/*Enable*/, 0/*Single port*/, 1/*Failover Enable*/));
            APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);
        }
        swgpreg_0e.v[0] = 0;
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Er(&phy->access, swgpreg_0e));

    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_get_enabled_port(const plp_aperta2_phymod_phy_access_t *phy, uint8_t *enabled_port_list) 
{
    uint32_t addr[APERTA2_MAX_PORT] = APERTA2_PORT_CTRL_REG;
    uint32_t port = 0, data = 0;

    for (port =0; port < APERTA2_MAX_PORT; port++) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_reg32_read(phy, addr[port], &data));
        enabled_port_list[port] = ((data >> 15) & 1) ? 1 : 0;
    }
 
    return PHYMOD_E_NONE;
}
int plp_aperta2_disable_octal_crossed_port (const plp_aperta2_phymod_phy_access_t *phy, const plp_aperta2_phymod_phy_inf_config_t* config, 
                                        uint32_t line_lane_map, const plp_aperta2_phymod_phy_inf_config_t *line_config) 
{
    uint32_t port_list = 0, list_cnt = 0, index = 0;
    uint8_t enabled_port_list[APERTA2_MAX_PORT], octal_crossing = 0, side = 0;
    unsigned int swapped_lane_mask = 0;
    uint32_t port_num = 0, dp_port_num = 0, cnt = 0;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG16r_t swgpreg_16;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG17r_t swgpreg_17;
    plp_aperta2_phymod_phy_access_t temp_access;
    aperta2_device_aux_modes_t sys_auxmode = *(aperta2_device_aux_modes_t*)config->device_aux_modes;
    aperta2_device_aux_modes_t line_auxmode = *(aperta2_device_aux_modes_t*)line_config->device_aux_modes;

    PHYMOD_MEMCPY(&temp_access, phy, sizeof(temp_access));

    for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
        if (side == phymodPortLocLine) {
            int prev_speed = 0, prev_lanemap = 0, oc_port = 0, retry = 0;
            swapped_lane_mask = (line_lane_map & 0xFF) ? (line_lane_map << 8) : (line_lane_map >> 8);
            if (line_auxmode.failover_config.lane_map) {
                swapped_lane_mask |= (line_auxmode.failover_config.lane_map & 0xFF) ? (line_auxmode.failover_config.lane_map << 8) : (line_auxmode.failover_config.lane_map >> 8);
            }
            temp_access.port_loc = side;
            temp_access.access.lane_mask = swapped_lane_mask;
            while (temp_access.access.lane_mask) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_pm_info_lane_speed_get(&temp_access, &prev_speed, &prev_lanemap, &oc_port));
                if (oc_port != 0xFF) {
                    temp_access.access.lane_mask &= ~prev_lanemap; 
                    port_list &= ~(0xF << (list_cnt*4));
                    port_list |= (oc_port << (list_cnt*4));
                    list_cnt ++;
                }
                if (retry++ > APERTA2_MAX_PORT) {
                    break;
                }
            }
        } else {
            swapped_lane_mask = (phy->access.lane_mask & 0xFF) ? (phy->access.lane_mask << 8) : (phy->access.lane_mask >> 8);
            if (sys_auxmode.failover_config.lane_map) {
                swapped_lane_mask |= (sys_auxmode.failover_config.lane_map & 0xFF) ? (sys_auxmode.failover_config.lane_map << 8) : (sys_auxmode.failover_config.lane_map >> 8);
            }
            APERTA2_LM_TO_POSSIBLE_PORT_LIST(swapped_lane_mask, port_list, list_cnt);
        }
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_enabled_port(phy, enabled_port_list));
        for (index = 0; index < list_cnt; index ++) {
            port_num = (port_list >> (4 * index)) & 0xF;
            if (port_num < 8) {
                PHYMOD_IF_ERR_RETURN(
                        BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG16r(&phy->access, &swgpreg_16));
                octal_crossing = ((swgpreg_16.v[0] & (3 << (port_num * 2))) >> (port_num * 2));
            } else {
                PHYMOD_IF_ERR_RETURN(
                        BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG17r(&phy->access, &swgpreg_17));
                octal_crossing = ((swgpreg_17.v[0] & (3 << ((port_num - 8) *2))) >> ((port_num - 8) *2));
            }
            if (octal_crossing != bcmAperta2NoOctalCrossing) {
                if (octal_crossing == bcmAperta2SysOctalCrossing && (side == phymodPortLocSys) ) {
                    if (enabled_port_list[port_num]) {
                        PHYMOD_DIAG_OUT(("##Sys Disable port:%d\n", port_num));
                        dp_port_num = APERTA2_PORT_DISABLE_PH1 | port_num;
                        PHYMOD_IF_ERR_RETURN( plp_aperta2_send_fw_msg(phy, 0, NULL,
                                    APERTA2_MSG_DISABLE_PORT, &dp_port_num, 0));
                        dp_port_num = APERTA2_PORT_DISABLE_PH2 | port_num;
                        PHYMOD_IF_ERR_RETURN( plp_aperta2_send_fw_msg(phy, 0, NULL,
                                    APERTA2_MSG_DISABLE_PORT, &dp_port_num, 0));
                        PHYMOD_IF_ERR_RETURN(
                                plp_aperta2_port_active_reset(phy, port_num));
                    }
                } 
                if (octal_crossing == bcmAperta2LineOctalCrossing && (side == phymodPortLocLine)) {
                    if (enabled_port_list[port_num]) {
                        PHYMOD_DIAG_OUT(("##Line Disable port:%d\n", port_num));
                        dp_port_num = APERTA2_PORT_DISABLE_PH1 | port_num;
                        PHYMOD_IF_ERR_RETURN( plp_aperta2_send_fw_msg(phy, 0, NULL,
                                    APERTA2_MSG_DISABLE_PORT, &dp_port_num, 0));
                        dp_port_num = APERTA2_PORT_DISABLE_PH2 | port_num;
                        PHYMOD_IF_ERR_RETURN( plp_aperta2_send_fw_msg(phy, 0, NULL,
                                    APERTA2_MSG_DISABLE_PORT, &dp_port_num, 0));
                        PHYMOD_IF_ERR_RETURN(
                                plp_aperta2_port_active_reset(phy, port_num));
                    }
                }
            }
        }
    }


    return PHYMOD_E_NONE;
}

int plp_aperta2_disable_port(const plp_aperta2_phymod_phy_access_t *phy, const plp_aperta2_phymod_phy_inf_config_t* config, 
                         int prev_port, int phase, uint32_t line_lane_map, const plp_aperta2_phymod_phy_inf_config_t *line_config) {
    int port_index = 0;
    uint8_t enabled_port_list[APERTA2_MAX_PORT], max_ports = 0;
    uint64_t possible_port_list = 0;
    int temp = 0 , cnt = 0;
    int phase_single_port = 0, single_port_start_index = 0;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG0Fr_t disable_port_phase;  /* Added to makesure PH1 disable is not called again*/
    aperta2_device_aux_modes_t *aux_mode = (aperta2_device_aux_modes_t *) config->device_aux_modes;
    aperta2_device_aux_modes_t *line_auxmode = (aperta2_device_aux_modes_t*)line_config->device_aux_modes;
    int port_map = 0;
    plp_aperta2_phymod_phy_access_t phy_acc;
    plp_aperta2_phymod_phy_access_t *temp_acc = &phy_acc;

    PHYMOD_MEMSET(&disable_port_phase, 0, sizeof(BCM_APERTA2_DIRECT_CTRL_SWGPREG0Fr_t));

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_enabled_port(phy, enabled_port_list));
    if (config->data_rate == APERTA2_SPEED_800G ||
        ((config->data_rate == APERTA2_SPEED_400G) && (aux_mode->lane_data_rate == APERTA2_LD_50G))) {
        if (aux_mode->octal_crossing != bcmAperta2NoOctalCrossing) {
            APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(phy->access.lane_mask, phymodPortLocSys);
            APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(line_lane_map, phymodPortLocLine);
            if (line_auxmode->failover_config.lane_map) {
                APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(line_auxmode->failover_config.lane_map, phymodPortLocLine);
            }
            if (aux_mode->failover_config.lane_map) {
                APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(aux_mode->failover_config.lane_map, phymodPortLocSys);
            }
            single_port_start_index = 0;
            max_ports = 0;
        } else if (APERTA2_GET_OCTAL(phy->access.lane_mask) == APERTA2_PM_OCTAL1) {
            single_port_start_index = 0;
            max_ports = 8;
        } else {
            single_port_start_index = 8;
            max_ports = APERTA2_MAX_PORT;
        }
        for (port_index = single_port_start_index; port_index < max_ports; port_index++) {
            if (enabled_port_list[port_index] == 1) {
                if(phase & APERTA2_PORT_DISABLE_PH1) {
                     PHYMOD_IF_ERR_RETURN(
                         BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Fr(&phy->access, &disable_port_phase));
                     phase_single_port = (APERTA2_PORT_DISABLE_PH1 | port_index);
                     PHYMOD_CRIT_INFO(("##Single Disable port:%d##%x \n",  port_index, phase_single_port));
                     PHYMOD_IF_ERR_RETURN(
                        plp_aperta2_send_fw_msg(phy, 0, NULL,
                           APERTA2_MSG_DISABLE_PORT, &phase_single_port, 0));
                    disable_port_phase.v[0] &= ~(1 << (port_index & 0xF));
                    disable_port_phase.v[0] |= ( 1 << (port_index & 0xF));
                    PHYMOD_IF_ERR_RETURN(
                         BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Fr(&phy->access, disable_port_phase));
                } else if(phase & APERTA2_PORT_DISABLE_PH2) {
                    PHYMOD_IF_ERR_RETURN(
                         BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Fr(&phy->access, &disable_port_phase));

                    phase_single_port = (APERTA2_PORT_DISABLE_PH2 | port_index);
                    PHYMOD_CRIT_INFO(("##Single Disable port:%d##%x \n",  port_index, phase_single_port));
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_send_fw_msg(phy, 0, NULL,
                                APERTA2_MSG_DISABLE_PORT, &phase_single_port, 0));
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_port_active_reset(phy, port_index));
                    disable_port_phase.v[0] &= ~(1 << (port_index & 0xF));
                    PHYMOD_IF_ERR_RETURN(
                         BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Fr(&phy->access, disable_port_phase));

                }
            }
        }
        /* If previous port is octal crossing*/
        APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(phy->access.lane_mask, phymodPortLocSys);
        APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(phy->access.lane_mask, phymodPortLocLine);
    } else {
        /* Disable existing port associated with the line lane map*/

        if ((prev_port <= 0xF) && enabled_port_list[prev_port]) {
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Fr(&phy->access, &disable_port_phase));
            max_ports = 1;
    	    COMPILER_64_SET(possible_port_list, 0, prev_port); 
            APERTA2_DISABLE_PORT(phy);
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Fr(&phy->access, disable_port_phase));
        }
        APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(phy->access.lane_mask, phymodPortLocSys);

        /* Disable possible ports that are associated with current System side lane map */
        APERTA2_LM_TO_POSSIBLE_PORT_LIST(phy->access.lane_mask, possible_port_list, max_ports);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Fr(&phy->access, &disable_port_phase));
        APERTA2_DISABLE_PORT(phy)
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Fr(&phy->access, disable_port_phase));

        /* Take care of System Side FO lanemap*/
        if (aux_mode->failover_config.lane_map) {
            APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(aux_mode->failover_config.lane_map, phymodPortLocSys);
            PHYMOD_MEMCPY(temp_acc, phy, sizeof(plp_aperta2_phymod_phy_access_t));
            temp_acc->access.lane_mask = aux_mode->failover_config.lane_map;
            APERTA2_LM_TO_POSSIBLE_PORT_LIST(aux_mode->failover_config.lane_map, possible_port_list, max_ports);
            PHYMOD_IF_ERR_RETURN(
                    BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Fr(&phy->access, &disable_port_phase));
            /* Disable other possible port if it was already part of curent lane*/
            APERTA2_DISABLE_PORT(temp_acc)
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Fr(&phy->access, disable_port_phase));
        }

        /* Disable existing port associated with the line lane map*/
        APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(line_lane_map, phymodPortLocLine);
#if 0 /* This is not needed as we use system lane as logical port*/
        /* Disable possible ports that are associated with current System side lane map */
        port_map = (phy->access.lane_mask ^ line_lane_map) & line_lane_map;
        PHYMOD_MEMCPY(temp_acc, phy, sizeof(plp_aperta2_phymod_phy_access_t));
        APERTA2_LM_TO_POSSIBLE_PORT_LIST(port_map, possible_port_list, max_ports);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Fr(&phy->access, &disable_port_phase));
        phy_acc.access.lane_mask = port_map;
        /* Disable other port if it was already part of current lane*/
        APERTA2_DISABLE_PORT(temp_acc);
        PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Fr(&phy->access, disable_port_phase));
#endif
        /* Take care of Line Side FO lanemap*/
        if (line_auxmode->failover_config.lane_map) {
            APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(line_auxmode->failover_config.lane_map, phymodPortLocLine);
            PHYMOD_MEMCPY(temp_acc, phy, sizeof(plp_aperta2_phymod_phy_access_t));
            temp_acc->access.lane_mask = line_auxmode->failover_config.lane_map;
            temp_acc->port_loc = phymodPortLocLine;
            APERTA2_LM_TO_POSSIBLE_PORT_LIST(line_auxmode->failover_config.lane_map, possible_port_list, max_ports);
            PHYMOD_IF_ERR_RETURN(
                    BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Fr(&temp_acc->access, &disable_port_phase));
            /* Disable other possible port if it was already part of curent lane*/
            APERTA2_DISABLE_PORT(temp_acc)
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Fr(&temp_acc->access, disable_port_phase));
        }
        /* Disable port that was created by FW for octal crossing*/ 
        PHYMOD_IF_ERR_RETURN(
           plp_aperta2_get_octal_crossing_port(phy, aux_mode, line_lane_map, &port_map, &max_ports));
        if ((max_ports > 0) && (max_ports != 0xFF)) { 
            COMPILER_64_SET(possible_port_list, 0, port_map); 
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Fr(&phy->access, &disable_port_phase));
            /* Disable other port if it was already part of current lane*/
            APERTA2_DISABLE_PORT(phy);
            PHYMOD_IF_ERR_RETURN(
                BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Fr(&phy->access, disable_port_phase));
        }
        if (phase & APERTA2_PORT_DISABLE_PH2) {
            /* Special case handling for previous octal crossing*/
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_disable_octal_crossed_port (phy, config, line_lane_map, line_config));
        }

    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_tx_rx_enable(const plp_aperta2_phymod_phy_access_t* phy, int enable, int single_port, int failover)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t tx_ctrl;
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t dc3mac_ctrl;
    int side = 0, count = 0 , prev_state;
    uint32_t temp = 0, system_lane_map;
    plp_aperta2_phymod_phy_access_t temp_access;
    plp_aperta2_phymod_phy_access_t phy1;
    PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&phy1, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_IF_ERR_RETURN(
                plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_CTRL_SWGPREG11r, &system_lane_map ));
    if (!enable) {
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
            temp_access.port_loc = side;
            for (count = 0; count < APERTA2_MAX_NUM_LANES; count ++) {
                if ((phy->access.lane_mask & (1 << count))) {
                    temp_access.access.lane_mask = 1 << count;
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_reg32_read(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, &temp));
                    if (temp & 0x2) {
                        temp &= ~0x02;
                        PHYMOD_IF_ERR_RETURN(
                                plp_aperta2_reg32_write(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, temp));
                    }
                    prev_state = side;
                    side = (prev_state == phymodPortLocLine) ? phymodPortLocSys : phymodPortLocLine;
                    temp_access.port_loc = side;
                    PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_reg32_read(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr, &temp));
                    if ((temp & 0x4) != 0x4) {
                        temp |= 0x4 ;
                        PHYMOD_IF_ERR_RETURN(
                                plp_aperta2_reg32_write(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr, temp));
                    }
                    side = prev_state ;
                    temp_access.port_loc = side;
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
            for (count = 0; count < APERTA2_MAX_NUM_LANES; count ++) {
                if ((phy1.access.lane_mask & (1 << count))) {
                    temp_access.access.lane_mask = 1 << count;
                    PHYMOD_IF_ERR_RETURN(
                         BCM_APERTA2_DC3MAC_READ_DC3MAC_TX_CTRLr(&temp_access, &tx_ctrl));
                    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_DISCARDf_SET(tx_ctrl, 0);
                    PHYMOD_IF_ERR_RETURN(
                         BCM_APERTA2_DC3MAC_WRITE_DC3MAC_TX_CTRLr(&temp_access, tx_ctrl));
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
            for (count = 0; count < APERTA2_MAX_NUM_LANES; count ++) {
                if ((phy1.access.lane_mask & (1 << count))) {
                    temp_access.access.lane_mask = 1 << count;

                    PHYMOD_IF_ERR_RETURN(
                        BCM_APERTA2_DC3MAC_READ_DC3MAC_CTRLr(&temp_access, &dc3mac_ctrl));
                    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_SOFT_RESETf_SET(dc3mac_ctrl, 0);
                    PHYMOD_IF_ERR_RETURN(
                        BCM_APERTA2_DC3MAC_WRITE_DC3MAC_CTRLr(&temp_access, dc3mac_ctrl));
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
            for (count = 0; count < APERTA2_MAX_NUM_LANES; count ++) {
                if ((phy1.access.lane_mask & (1 << count))) {
                    temp_access.access.lane_mask = 1 << count;
                    PHYMOD_IF_ERR_RETURN(
                        BCM_APERTA2_DC3MAC_READ_DC3MAC_CTRLr(&temp_access, &dc3mac_ctrl));
                    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_TX_ENf_SET(dc3mac_ctrl, 1);
                    PHYMOD_IF_ERR_RETURN(
                        BCM_APERTA2_DC3MAC_WRITE_DC3MAC_CTRLr(&temp_access, dc3mac_ctrl));
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
            for (count = 0; count < APERTA2_MAX_NUM_LANES; count ++) {
                if ((phy1.access.lane_mask & (1 << count))) {
                    temp_access.access.lane_mask = 1 << count;
                    PHYMOD_IF_ERR_RETURN(
                        BCM_APERTA2_DC3MAC_READ_DC3MAC_CTRLr(&temp_access, &dc3mac_ctrl));
                    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_RX_ENf_SET(dc3mac_ctrl, 1);
                    PHYMOD_IF_ERR_RETURN(
                        BCM_APERTA2_DC3MAC_WRITE_DC3MAC_CTRLr(&temp_access, dc3mac_ctrl));
                }
            }
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_pm_tx_rx_enable_post(const plp_aperta2_phymod_phy_access_t* phy, int enable, int single_port, uint16_t fo_side_lm, uint32_t line_mask)
{
    int side = 0, count = 0;
    uint32_t temp = 0, lane_index = 0 ;
    plp_aperta2_phymod_phy_access_t temp_access, temp_access1;
    PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&temp_access1, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    if (!enable) {
        for (side = phymodPortLocLine; side <= phymodPortLocSys; side++) {
           temp_access.port_loc = side;
           if (!single_port) {
               temp_access.access.lane_mask = (side == phymodPortLocLine) ? line_mask : phy->access.lane_mask;
               temp_access1.access.lane_mask = (side == phymodPortLocLine) ? line_mask : phy->access.lane_mask;
           }
           if (single_port) {
                for (count = 0; count < APERTA2_MAX_NUM_LANES; count ++) {
                    if ((phy->access.lane_mask & (1 << count))) {
                        temp_access.access.lane_mask = 1 << count;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_reg32_read(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, &temp));
                        temp &= ~0x01;
                        PHYMOD_IF_ERR_RETURN(
                           plp_aperta2_reg32_write(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, temp));
                        temp |= 0x4;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_reg32_write(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, temp));
                    }
                }
            } else {
                for (count = 0; count < APERTA2_MAX_NUM_LANES; count ++) {
                    if ((temp_access1.access.lane_mask & (1 << count))) {
                        temp_access.access.lane_mask = 1 << count;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_reg32_read(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, &temp));
                        temp &= ~0x01;
                        PHYMOD_IF_ERR_RETURN(
                           plp_aperta2_reg32_write(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, temp));
                        temp |= 0x4;
                        PHYMOD_IF_ERR_RETURN(
                            plp_aperta2_reg32_write(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, temp));
                        if ((fo_side_lm & 0xFF) && (side == ((((fo_side_lm >> 8) & 0xF) == phymodPortLocSys) ? phymodPortLocSys :
                               phymodPortLocLine))) {
                            for (lane_index = 0 ; lane_index <8; lane_index ++) {
                                 if (fo_side_lm & ( 1 << lane_index)) {
                                     temp_access.access.lane_mask = 1 << lane_index;
                                     PHYMOD_IF_ERR_RETURN(
                                         plp_aperta2_reg32_read(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, &temp));
                                     temp &= ~0x01;
                                     PHYMOD_IF_ERR_RETURN(
                                        plp_aperta2_reg32_write(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, temp));
                                     temp |= 0x4;
                                     PHYMOD_IF_ERR_RETURN(
                                         plp_aperta2_reg32_write(&temp_access, BCM_APERTA2_DC3MAC_DC3MAC_CTRLr, temp));
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

int plp_aperta2_pm_interface_config_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_phy_inf_config_t* config)
{
    int unit = 0, int_port = 0, port = 0;
    portmod_speed_config_t speed_config;
    aperta2_config_port_t port_cfg ;
    aperta2_device_aux_modes_t *auxmode = (aperta2_device_aux_modes_t*)config->device_aux_modes;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG16r_t swgpreg_16;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG17r_t swgpreg_17;
    int speed = 0, configured_lane = 0, port_num = 0; 


    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);
    if(auxmode == NULL) {
        PHYMOD_DEBUG_ERROR(("Aux mode cannot be NULL\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_MEMSET(auxmode, 0, sizeof(aperta2_device_aux_modes_t));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_port_speed_config_get(unit, port, &speed_config));

    config->interface_modes =  0; /* Aperta supports only IEEE*/
    config->ref_clock = phymodRefClk312Mhz; /* Aperta supports only 156*/
    config->data_rate = speed_config.speed;
    if (speed_config.fec == PORTMOD_PORT_PHY_FEC_NONE) {
        auxmode->fec_mode_sel = bcmAperta2NoFEC;
    } else if(speed_config.fec == PORTMOD_PORT_PHY_FEC_RS_544) {
        auxmode->fec_mode_sel = bcmAperta2RS544;
    } else if (speed_config.fec == PORTMOD_PORT_PHY_FEC_RS_272) {
        auxmode->fec_mode_sel = bcmAperta2RS272;
    } else if (speed_config.fec == PORTMOD_PORT_PHY_FEC_RS_544_2XN) {
        auxmode->fec_mode_sel = bcmAperta2RS544_2XN;
    } else if (speed_config.fec == PORTMOD_PORT_PHY_FEC_RS_FEC) {
        auxmode->fec_mode_sel = bcmAperta2RSFEC;
    } else if (speed_config.fec == PORTMOD_PORT_PHY_FEC_RS_272_2XN) {
        auxmode->fec_mode_sel = bcmAperta2RS272_2XN;
    } else {
        auxmode->fec_mode_sel = bcmAperta2NoFEC;
    }
    if (config->data_rate == APERTA2_SPEED_800G) { 
        auxmode->lane_data_rate = APERTA2_LD_100G;
        auxmode->modulation_mode = bcmAperta2ModulationPAM4;
    } else if (config->data_rate == APERTA2_SPEED_400G) {
        auxmode->modulation_mode = bcmAperta2ModulationPAM4;
        if (speed_config.num_lane == 4) {
            auxmode->lane_data_rate = APERTA2_LD_100G;
        } else {
            auxmode->lane_data_rate = APERTA2_LD_50G;
        }
    } else if (config->data_rate == APERTA2_SPEED_200G) {
        auxmode->modulation_mode = bcmAperta2ModulationPAM4;
        if (speed_config.num_lane == 2) {
            auxmode->lane_data_rate = APERTA2_LD_100G;
        } else {
            auxmode->lane_data_rate = APERTA2_LD_50G;
        }
    } else if (config->data_rate == APERTA2_SPEED_100G) {
        if (PORTMOD_PORT_PHY_LANE_CONFIG_FORCE_PAM4_GET(speed_config.lane_config)) {
            auxmode->modulation_mode = bcmAperta2ModulationPAM4;
            if (speed_config.num_lane == 1) {
                auxmode->lane_data_rate = APERTA2_LD_100G;
            } else if (speed_config.num_lane == 2) {
                auxmode->lane_data_rate = APERTA2_LD_50G;
            }
        } else {
            auxmode->lane_data_rate = APERTA2_LD_25G;
            auxmode->modulation_mode = bcmAperta2ModulationNRZ;
        }
    } else if (config->data_rate == APERTA2_SPEED_50G) { 
        if (speed_config.num_lane == 1)  {
            auxmode->lane_data_rate = APERTA2_LD_50G;
            auxmode->modulation_mode = bcmAperta2ModulationPAM4;
        } else {
            auxmode->lane_data_rate = APERTA2_LD_25G;
            auxmode->modulation_mode = bcmAperta2ModulationNRZ;
        }
    } else if (config->data_rate == APERTA2_SPEED_25G) {
        auxmode->lane_data_rate = APERTA2_LD_25G;
        auxmode->modulation_mode = bcmAperta2ModulationNRZ;
    } else if (config->data_rate == APERTA2_SPEED_10G) {
        auxmode->lane_data_rate = APERTA2_LD_10G;
        auxmode->modulation_mode = bcmAperta2ModulationNRZ;
    } else {
        auxmode->lane_data_rate = APERTA2_LD_25G;
        auxmode->modulation_mode = bcmAperta2ModulationNRZ;
    }
    /* To check given lane is Failover*/
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_pm_info_lane_speed_get(phy, &speed, &configured_lane, &port_num));
    (void) speed;
    (void) configured_lane;
    /*PHYMOD_IF_ERR_RETURN(
            APERTA2_pm_is_fo_enabled(phy, config,
                &fo_port, &pri_port, &primary_lm));*/
    port_cfg.PortNum = port_num & 0xF;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_send_fw_msg(phy, 0, NULL,
                       APERTA2_MSG_CONFIG_PORT, &port_cfg, 1));
    if (port_cfg.PortMode == 1) {
        if (port_cfg.FOOptions & 1){
            if (APERTA2_IS_LINE_SIDE(phy))  {/* Line side*/
                APERTA2_GET_LM_FROM_PORT(config->data_rate, auxmode->lane_data_rate, port_cfg.FOPortID, auxmode->failover_config.lane_map);
                auxmode->failover_config.tx_back_pressure_mode = (port_cfg.FOOptions & 2) >> 1;
            } else {
                auxmode->failover_config.lane_map = 0;
                auxmode->failover_config.tx_back_pressure_mode = 0;
            }
        } else { /* Sys side*/
            if (APERTA2_IS_SYSTEM_SIDE(phy))  {
                APERTA2_GET_LM_FROM_PORT(config->data_rate, auxmode->lane_data_rate, port_cfg.FOPortID, auxmode->failover_config.lane_map);
                auxmode->failover_config.tx_back_pressure_mode = (port_cfg.FOOptions & 2) >> 1;
            } else {
                auxmode->failover_config.lane_map = 0;
                auxmode->failover_config.tx_back_pressure_mode = 0;
            }
        }
    } else {
        auxmode->failover_config.lane_map = 0;
        auxmode->failover_config.tx_back_pressure_mode = 0;
    }
    auxmode->ing_fixed_latency.enable = (port_cfg.PortOptions & 0x10) ? 1 : 0;
    auxmode->ing_fixed_latency.start_point = (port_cfg.PortOptions & 0x20) ? 1 : 0;
    auxmode->egr_fixed_latency.enable = (port_cfg.PortOptions & 0x40) ? 1 : 0;
    auxmode->egr_fixed_latency.start_point = (port_cfg.PortOptions & 0x80) ? 1 : 0;
    auxmode->ts_config = (port_cfg.PortOptions >> 8) & 0x7;
    if (port_cfg.PortNum < 8) {
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG16r(&phy->access, &swgpreg_16));
        auxmode->octal_crossing = ((swgpreg_16.v[0] & (3 << (port_cfg.PortNum *2))) >> (port_cfg.PortNum *2));
    } else {
        PHYMOD_IF_ERR_RETURN(
            BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG17r(&phy->access, &swgpreg_17));
        auxmode->octal_crossing = ((swgpreg_17.v[0] & (3 << ((port_cfg.PortNum-8) *2))) >> ((port_cfg.PortNum-8) *2));
    }
    if (auxmode->ing_fixed_latency.enable) {
        auxmode->ing_fixed_latency.dp_ck_cycles = port_cfg.IngFixedLatency;
    } else {
        auxmode->ing_fixed_latency.dp_ck_cycles = 0;
    }
    if (auxmode->egr_fixed_latency.enable) {
        auxmode->egr_fixed_latency.dp_ck_cycles = port_cfg.EgrFixedLatency;
    } else {
        auxmode->egr_fixed_latency.dp_ck_cycles = 0;
    }
 
    return PHYMOD_E_NONE;
}

int plp_aperta2_flush_port(const plp_aperta2_phymod_phy_access_t *phy)
{
    uint32_t flags=0;
    plp_aperta2_phymod_phy_inf_config_t config;
    aperta2_device_aux_modes_t auxmode;
    int rv = 0;

    PHYMOD_MEMSET(&config, 0, sizeof(plp_aperta2_phymod_phy_inf_config_t));
    config.device_aux_modes = &auxmode;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_interface_config_get(phy, flags, &config));
    
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_send_fw_msg(phy, config.data_rate , &auxmode,
                       APERTA2_MSG_PAUSE_PORT, NULL, 0));
    rv = plp_aperta2_send_fw_msg(phy, config.data_rate , &auxmode,
                       APERTA2_MSG_FLUSH_PORT, NULL, 0);
    if (rv != PHYMOD_E_NONE) {
        PHYMOD_CRIT_INFO(("FLUSH FAILS. Retrying ...\n"));
        rv = plp_aperta2_send_fw_msg(phy, config.data_rate , &auxmode,
                       APERTA2_MSG_FLUSH_PORT, NULL, 0);
        if (rv != PHYMOD_E_NONE) {
            PHYMOD_IF_ERR_RETURN(
                 plp_aperta2_send_fw_msg(phy, config.data_rate , &auxmode,
                       APERTA2_MSG_RESUME_PORT, NULL, 0));
            return rv ;
        }
    }
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_send_fw_msg(phy, config.data_rate , &auxmode,
                       APERTA2_MSG_RESUME_PORT, NULL, 0));

    return PHYMOD_E_NONE;

}
int plp_aperta2_print_pm_mem(const plp_aperta2_phymod_phy_access_t* phy, int port_num, int side, phymod_mem_type_t mem_type, int first_idx, int last_idx)
{
    plp_aperta2_phymod_phy_access_t phy_temp;
    int side_idx = 0;
    int idx;
    int side_list[3] = { APERTA2_SW_LINE_SIDE, APERTA2_SW_SYS_SIDE, APERTA2_SW_NO_SIDE };
    char mem_name[8] = "DCCC";
    char side_sfx[6] = "     ";
    char port_sfx[3] = ".";
    unsigned int port =  port_num > 7 ? (port_num-8) : port_num;

#ifdef APERTA2_REG_DUMP_INTO_FILE
    PHYMOD_FILE *fp = PHYMOD_FOPEN("./RegDump.txt", "a");
    if (!fp) {
        PHYMOD_DEBUG_ERROR(("Invalid File pointer\n"));
        return PHYMOD_E_FAIL;
    }

#endif
    for (side_idx = 0; side_idx < 3; side_idx++) {
        if (side != APERTA2_SW_BOTH_SIDE && side_list[side_idx] != side) {
            continue;
        }
        if (side == APERTA2_SW_BOTH_SIDE && side_list[side_idx] == APERTA2_SW_NO_SIDE) {
            continue;
        }

        (side_list[side_idx] == APERTA2_SW_NO_SIDE) ? (PHYMOD_STRCPY(side_sfx, "     ")) : ((side_list[side_idx] == APERTA2_SW_LINE_SIDE) ? (PHYMOD_STRCPY(side_sfx, "  (L)")) : (PHYMOD_STRCPY(side_sfx, "  (S)")));

        PHYMOD_SPRINTF(port_sfx+1,"%x", port_num % 16);

        /* update pm memory access params */
        switch (mem_type) {
            case phymodMemPmMib:
                PHYMOD_STRCPY(mem_name, "DCCCMIB");
                break;
            case phymodMemSpeedIdTable:
                PHYMOD_STRCPY(mem_name, "SPDID");
                break;
            default: /* same as CDMIB */
                PHYMOD_STRCPY(mem_name, "DCCCMIB");
                break;
        }


        /* Read and print the memory index */
        for (idx = first_idx; idx <= last_idx; idx++) {
            uint32_t mac_counter_data[16];
            char     spc_sfx[2] = "";
            int      i;
            PHYMOD_MEMSET(mac_counter_data, 0, sizeof(mac_counter_data));
            PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));
            if (port_num > 7) {
                phy_temp.access.lane_mask = 0x100;
            } else {
                phy_temp.access.lane_mask = 0x1;
            }
            phy_temp.port_loc = (side_list[side_idx] == APERTA2_SW_SYS_SIDE) ? phymodPortLocSys : phymodPortLocLine;

            APERTA2_PM_IF_ERR_RETURN(
                plp_aperta2_mem_read(&phy_temp, mem_type, idx + (port*16) /*Index*/, mac_counter_data/*data*/));

            APERTA2_PCS_INFO_PRINT("%d.%s[%0X]%s %s:", phy->access.addr, mem_name,idx + (port_num*16) , port_sfx, side_sfx);

            for (i = 0; i < 8; i++) {
                PHYMOD_STRCPY(spc_sfx, "");
                APERTA2_PCS_INFO_PRINT(" 0x");
                
                /*LSB*/
                APERTA2_PCS_INFO_PRINT("%s%04x", spc_sfx, mac_counter_data[(i*2)]);
                PHYMOD_STRCPY(spc_sfx, "-");

                /*MSB*/
                APERTA2_PCS_INFO_PRINT("%s%04x", spc_sfx, mac_counter_data[(i*2)+1]);
                PHYMOD_STRCPY(spc_sfx, "-");
                
            }
            APERTA2_PCS_INFO_PRINT("\n");
        }
    }
APERTA2_ERR:
#ifdef APERTA2_REG_DUMP_INTO_FILE
    PHYMOD_FCLOSE(fp);
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_print_regs(const plp_aperta2_phymod_phy_access_t* phy, int port_num, int side, int first_addr, int last_addr) {
    aperta2_config_port_t port_cfg ;
    plp_aperta2_phymod_phy_access_t phy_temp;
    uint32_t   rddata[16] = { 0 };
    uint32_t   lane_map = 0;
    uint8_t    enabled_port_list[APERTA2_MAX_PORT];
    char       portid_sfx[12] = "pm_portid"; /* #dp_port */
    int        portid_list[3] = { 0 };       /* {LINE/EGR, SYS/ING, common} */
    int        portid;
    int        addr, addr_inc;
    int        idx = 0;
    int        side_idx;
    int        side_ii;
    int        side_list[3] = { APERTA2_SW_LINE_SIDE, APERTA2_SW_SYS_SIDE, APERTA2_SW_NO_SIDE };
    uint32_t        speed = 0, ldr = 0;
#ifdef APERTA2_REG_DUMP_INTO_FILE
    PHYMOD_FILE *fp = PHYMOD_FOPEN("./RegDump.txt", "a");
    if (!fp) {
        PHYMOD_DEBUG_ERROR(("Invalid File pointer\n"));
        return PHYMOD_E_FAIL;
    }

#endif

    PHYMOD_MEMSET(&port_cfg, 0, sizeof(aperta2_config_port_t));
    APERTA2_PM_IF_ERR_RETURN(plp_aperta2_get_enabled_port(phy, enabled_port_list));

    if (port_num != APERTA2_PORT_NONE) {
        if(enabled_port_list[port_num]){
            port_cfg.PortNum = port_num;
            port_cfg.port_operation = APERTA2_OP_READ; 
            APERTA2_PM_IF_ERR_RETURN(
                    plp_aperta2_fw_config_port (phy,  &port_cfg));
        }
    }

    if ((first_addr & 0xF0000000) == 0xA0000000) { /* PM */
        addr_inc = 1;
        portid_list[APERTA2_SW_LINE_SIDE] = port_cfg.LPMPortID;
        portid_list[APERTA2_SW_SYS_SIDE]  = port_cfg.SPMPortID;
        portid_list[2] = -1; /* ERROR scenario */
        PHYMOD_STRCPY(portid_sfx, "pm_portid");
    }  else if ((first_addr & 0xFF000000) == 0xAA000000) { /* CHIP_IND */
        addr_inc = 1;
        portid_list[APERTA2_SW_LINE_SIDE] = port_num;  /* ING    */
        portid_list[APERTA2_SW_SYS_SIDE] = port_num;   /* EGR    */
        portid_list[2] = port_num;          /* Common */
        PHYMOD_STRCPY(portid_sfx, "dp_port");
    } else if (((first_addr & 0xFFF00000) >= 0x50100000) & ((first_addr & 0xFFF00000) <= 0x50700000)) { /* MACsec registers */
        addr_inc = 4;
        portid_list[APERTA2_SW_LINE_SIDE] = port_num;
        portid_list[APERTA2_SW_SYS_SIDE] = port_num;
        portid_list[2] = port_num;
        PHYMOD_STRCPY(portid_sfx, "dp_port");
    } else {
        addr_inc = 1;
        portid_list[APERTA2_SW_LINE_SIDE] = port_num;
        portid_list[APERTA2_SW_SYS_SIDE] = port_num;
        portid_list[2] = port_num;
        PHYMOD_STRCPY(portid_sfx, "dp_port");
    }
    for (side_idx = 0; side_idx < 3; side_idx++) {
        if (side != APERTA2_SW_BOTH_SIDE && side_list[side_idx] != side) {
            continue;
        }
        /* For APERTA2_SW_BOTH_SIDE registers, don't loop thru APERTA2_SW_NO_SIDE option which is for common regs. */
        if (side == APERTA2_SW_BOTH_SIDE && side_list[side_idx] == APERTA2_SW_NO_SIDE) {
            continue;
        }
        side_ii = side_list[side_idx];
        portid  = portid_list[side_idx];
        for (idx = 0, addr = first_addr; addr <= last_addr; addr = addr + addr_inc) {
            char side_sfx[4] = "   ";
            char port_sfx[3] = ".";

            if (((first_addr & 0xFF00FF00) == 0xAA00F000) || ((first_addr & 0xFF00FF00) == 0xAA00F100) || ((first_addr & 0xFF00F000) == 0xAA00D000) || ((first_addr & 0xFFFF0000) == 0x50300000)|| ((first_addr & 0xFF00F000) == 0xAA00B000) || ((first_addr & 0xFF00F000) == 0xAA00D000)) {
                PHYMOD_STRCPY(side_sfx, "(E)");
            } else if (((first_addr & 0xFF00FF00) == 0xAA00E000) || ((first_addr & 0xFF00FF00) == 0xAA00E100) || ((first_addr & 0xFF00F000) == 0xAA00C000) || ((first_addr & 0xFFFF0000) == 0x50700000) || ((first_addr & 0xFF00F000) == 0xAA00A000) || ((first_addr & 0xFF00F000) == 0xAA00C000)) {
                PHYMOD_STRCPY(side_sfx, "(I)");
            } else {
                (side_ii == APERTA2_SW_NO_SIDE) ? (PHYMOD_STRCPY(side_sfx, "   ")) : ((side_ii == APERTA2_SW_LINE_SIDE) ? (PHYMOD_STRCPY(side_sfx, "(L)")) : (PHYMOD_STRCPY(side_sfx, "(S)")));
            }
            if(port_num == APERTA2_PORT_NONE) {
                PHYMOD_STRCPY(port_sfx, "");
            } else {
                PHYMOD_SPRINTF(port_sfx+1,"%x", port_num % 16);
            }
            if (idx % 16 == 0) {
                APERTA2_PCS_INFO_PRINT("%0d.0x%08X%2s %3s: ", phy->access.addr, addr, port_sfx, side_sfx);
            }

            if (port_num == APERTA2_PORT_NONE) {
                APERTA2_PM_IF_ERR_RETURN(plp_aperta2_reg32_read(phy, addr, &rddata[idx]));idx += addr_inc;
            } else {
                if (APERTA2_IS_LINE_SIDE(phy)) {
                    APERTA2_PM_IF_ERR_RETURN(
                        _plp_aperta2_get_dr_ldr_from_fw_speed (port_cfg.LinePortSpeed, &speed, &ldr));
                } else {
                    APERTA2_PM_IF_ERR_RETURN(
                        _plp_aperta2_get_dr_ldr_from_fw_speed (port_cfg.SysPortSpeed, &speed, &ldr));
                }
                APERTA2_GET_LM_FROM_PORT(speed, ldr, port_cfg.PortNum, lane_map);
                PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta2_phymod_phy_access_t));
                /* For the slaves with no side information we need logical port number based on 
                   system side */
                if (side == APERTA2_SW_NO_SIDE) {
                    phy_temp.port_loc = phymodPortLocSys; 
                } else {
                phy_temp.port_loc = (side_list[side_idx] == APERTA2_SW_SYS_SIDE) ? phymodPortLocSys : phymodPortLocLine;
                }
                phy_temp.access.lane_mask = lane_map;
                APERTA2_PM_IF_ERR_RETURN(plp_aperta2_reg32_read(&phy_temp, addr, &rddata[idx]));
                idx += addr_inc;
            }
            if (idx % 16 == 0 || addr == last_addr) {
                int _idx;
                for (_idx = 0; _idx < idx; _idx = _idx + addr_inc) {
                    if (addr_inc == 1) {
                        APERTA2_PCS_INFO_PRINT("0x%-4x ", rddata[_idx]);
                    } else {
                        APERTA2_PCS_INFO_PRINT("0x%-18x ", rddata[_idx]);
                    }
                }
                APERTA2_PCS_INFO_PRINT(" // %s = %d\n", portid_sfx, portid);
                idx = 0;
            }
        }
    }
APERTA2_ERR:
#ifdef APERTA2_REG_DUMP_INTO_FILE
    PHYMOD_FCLOSE(fp);
#endif
    return PHYMOD_E_NONE;
}

int plp_aperta2_dump_macsec_regs(const plp_aperta2_phymod_phy_access_t* phy) {
    int temp = 0, ch = 0;
    plp_aperta2_phymod_phy_access_t oc_access;
    PHYMOD_FILE *fp=NULL;

    PHYMOD_MEMCPY(&oc_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    for (temp = APERTA2_OCTAL0; temp <= APERTA2_OCTAL1; temp++) {
#ifdef APERTA2_REG_DUMP_INTO_FILE
        fp = PHYMOD_FOPEN("./RegDump.txt", "a");
        if (!fp) {
            PHYMOD_DEBUG_ERROR(("Invalid File pointer\n"));
            return PHYMOD_E_FAIL;
        }
#endif
        APERTA2_PCS_INFO_PRINT("\n=================================================================================================================================\n");
        APERTA2_PCS_INFO_PRINT("Dumping MACSEC Common Registers for OCTAL_%s\n", (temp == APERTA2_OCTAL0) ? "0" : "1");
        APERTA2_PCS_INFO_PRINT("===================================================================================================================================\n");
#ifdef APERTA2_REG_DUMP_INTO_FILE
        PHYMOD_FCLOSE(fp);
#endif
        if (temp == APERTA2_OCTAL0) {
            oc_access.access.lane_mask = 0x1; /* Octal 0 lane map*/
        } else {
            oc_access.access.lane_mask = 0x100; /* Octal 1 lane map*/
        }
        /*EIP 163*/
        /* Egress - skip 4*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5010FE00, 0x5010FE1C)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5010FF00, 0x5010FF28)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5010FF60, 0x5010FF64)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5010FFEC, 0x5010FFEC)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5010FFF0, 0x5010FFF8)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50108100, 0x50108104)); 
        /* Ingress - skip 4*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5050FE00, 0x5050FE1C)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5050FF00, 0x5050FF28)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5050FF60, 0x5050FF64)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5050FFEC, 0x5050FFEC)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5050FFF0, 0x5050FFF8)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50508100, 0x50508104)); 

        /*Per Channel - EGR*/
        for (ch=0; ch <8; ch +=1) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, (0x5010a000 + 0x40 * ch), (0x5010a00C + 0x40 * ch)));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, (0x5010a014 + 0x40 * ch), (0x5010a014 + 0x40 * ch)));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, (0x5010a01C + 0x40 * ch), (0x5010a01C + 0x40 * ch)));
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50100000, 0x50107FF0)); /* All TCAM MASK_0*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50110000, 0x50117FF0)); /* All TCAM MASK_1*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50108800, 0x50108FFC)); /* All TCAM POLICY_0*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50118800, 0x50118FFC)); /* All TCAM POLICY_1*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50109000, 0x501090FC)); /* All VPORT POLICY*/
        /*Per Channel - IGR*/
        for (ch=0; ch <8; ch +=1) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, (0x5050a000 + 0x40 * ch), (0x5050a00C + 0x40 * ch)));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, (0x5050a014 + 0x40 * ch), (0x5050a014 + 0x40 * ch)));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, (0x5050a01C + 0x40 * ch), (0x5050a01C + 0x40 * ch)));
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50500000, 0x50507FF0)); /* All TCAM MASK_0*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50510000, 0x50517FF0)); /* All TCAM MASK_1*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50508800, 0x50508FFC)); /* All TCAM POLICY_0*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50518800, 0x50518FFC)); /* All TCAM POLICY_1*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x50509000, 0x505090FC)); /* All VPORT POLICY*/
        /* Stat Per ch : EGR*/
        for (ch=0; ch <8; ch +=1) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, (0x5010E000 + 0x40 * ch), (0x5010E034 + 0x40 * ch)));
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5010EC10, 0x5010EC10));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5010FF00, 0x5010FF28));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5010FFE8, 0x5010FFEC));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5010FFF0, 0x5010FFF0));
        /* Stat Per ch : IGR*/
        for (ch=0; ch <8; ch +=1) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, (0x5050E000 + 0x40 * ch), (0x5050E034 + 0x40 * ch)));
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5050EC10, 0x5050EC10));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5050FF00, 0x5050FF28));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5050FFE8, 0x5050FFEC));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5050FFF0, 0x5050FFF0));
        /*EIP 164*/
        /* Egress - skip 4*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5020FE00, 0x5010FE1C)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5020FF00, 0x5010FF44)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5020FF60, 0x5010FF64)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5020FFE0, 0x5010FFE0)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5020FFE8, 0x5010FFF0)); 
        /* Ingress - skip 4*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5060FE00, 0x5060FE1C)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5060FF00, 0x5060FF44)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5060FF60, 0x5060FF64)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5060FFE0, 0x5060FFE0)); 
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(&oc_access, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, 0x5060FFE8, 0x5060FFF0)); 
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_dump_port_regs(const plp_aperta2_phymod_phy_access_t* phy)
{
    uint32_t  rddata[2] = { 0 };
    uint32_t  port_type = 0, fo_port=0, fo_side=0, temp = 0;
    uint8_t enabled_port_list[APERTA2_MAX_PORT], fo_port_list[APERTA2_MAX_PORT];
    int  port_ii, port_side = APERTA2_SW_BOTH_SIDE;
    int  dp_port, spm_portid, lpm_portid, ing_port =0, egr_port =0;
#ifdef APERTA2_REG_DUMP_INTO_FILE
    PHYMOD_FILE *fp = NULL;
#endif
    PHYMOD_MEMSET(fo_port_list, 0, sizeof(uint8_t) * APERTA2_MAX_PORT);
    PHYMOD_IF_ERR_RETURN(plp_aperta2_get_enabled_port(phy, enabled_port_list));


    for(port_ii = 0; port_ii < APERTA2_MAX_PORT; port_ii++) {
        if(enabled_port_list[port_ii]) {
            if (!(fo_port_list[port_ii] & 0xF)) {
                /* Get dp_port, spm_portid, lpm_portid associated with Logical #Port */
                PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, 0x0100a002 + 16 * port_ii, &rddata[0])); /* dp_portid */
                PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, 0x0100a007 + 16 * port_ii, &rddata[1])); /* lpm_portid, spm_portid */
                PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, 0x0100a003 + 16 * port_ii, &port_type)); /* port_type */
                PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, 0x0100a005 + 16 * port_ii, &temp)); /* ingress port and egress port */
                ing_port = temp & 0xFF;
                egr_port = (temp & 0xFF00) >> 8;
                dp_port    = (rddata[0] & 0x00FF);
                spm_portid = (rddata[1] & 0x00FF);
                lpm_portid = (rddata[1] & 0xFF00) >> 8;

                if (port_type & 0xFF) { /*If failover enabled*/
                    PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, 0x0100a004 + 16 * port_ii, &fo_port)); /* FO Port & side*/
                    fo_side = fo_port & 1;
                    fo_port >>= 8;
                    fo_side = fo_side ? APERTA2_SW_LINE_SIDE : APERTA2_SW_SYS_SIDE;
                    fo_port_list[fo_port] = (1 | (fo_side << 4));
                    enabled_port_list[fo_port] = 1; /*Store it for Future iteration*/
                }
                port_side = APERTA2_SW_BOTH_SIDE;
            } else {
                /* FO port*/
                dp_port    = port_ii;
                spm_portid = port_ii;
                lpm_portid = port_ii;
                ing_port = port_ii;
                egr_port = port_ii;
                port_side = (fo_port_list[port_ii]) >> 4;
                enabled_port_list[port_ii] = 0; 
                fo_port_list[port_ii] = 0;
            }


#ifdef APERTA2_REG_DUMP_INTO_FILE
            fp = PHYMOD_FOPEN("./RegDump.txt", "a");
            if (!fp) {
                PHYMOD_DEBUG_ERROR(("Invalid File pointer\n"));
                return PHYMOD_E_FAIL;
            }
#endif
            APERTA2_PCS_INFO_PRINT("Logical Port %x dp_portid=%d spm_portid=%x lpm_portid=%x\n", port_ii, dp_port, spm_portid, lpm_portid);
            APERTA2_PCS_INFO_PRINT("rddata[0]=%x rdata[1]=%x\n\n", rddata[0], rddata[1]);

            APERTA2_PCS_INFO_PRINT("\n=================================================================================================================================\n");
            APERTA2_PCS_INFO_PRINT("Dumping port-level registers: PORT-%0d (dp_port=%x spm_portid=%x lpm_portid=%x)\n", port_ii, dp_port, spm_portid, lpm_portid);
            APERTA2_PCS_INFO_PRINT("===================================================================================================================================\n");
            APERTA2_PCS_INFO_PRINT("                  : +0     +1     +2     +3     +4     +5     +6     +7     +8     +9     +A     +B     +C     +D     +E     +F\n");
#ifdef APERTA2_REG_DUMP_INTO_FILE
            PHYMOD_FCLOSE(fp);
#endif
            /* ingress RX port ID*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00e000), (0xAA00E004)));
            /* Egress RX port ID*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00F000), (0xAA00F004)));

            /* ingress TX port ID*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00e100), (0xAA00E101)));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00e104), (0xAA00E104)));
            /* Egress TX port ID*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00F100), (0xAA00F101)));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00F104), (0xAA00F104)));

            /*PM2MACSEC - Ing*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00C001 + 0x100 * ing_port), (0xAA00C00D + 0x100 * ing_port))); 
            /*PM2MACSEC - Egress*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00D001 + 0x100 * egr_port), (0xAA00D00D + 0x100 * egr_port))); 
            /*P2M RC - Ing == skip 4*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0x50700008 + 0x100 * ing_port), (0x50700008 + 0x100 * ing_port))); 
            /*P2M RC - Egress == skip 4*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0x50300008 + 0x100 * egr_port), (0x50300008 + 0x100 * egr_port))); 
            /*INGRESS FC */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00A000 + 0x100 * ing_port), (0xAA00A008 + 0x100 * ing_port))); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00A800 + 0x100 * ing_port), (0xAA00A801 + 0x100 * ing_port))); 
            /*EGRESS FC */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00B000 + 0x100 * egr_port), (0xAA00B006 + 0x100 * egr_port))); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00B800 + 0x100 * egr_port), (0xAA00B801 + 0x100 *egr_port))); 

            /* PTP Bypass : Common*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA005000), (0xAA005000))); 
            /* PTP Bypass */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA006000 + 0x100 * dp_port), (0xAA006000 + 0x100 * dp_port))); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA007000 + 0x100 * dp_port), (0xAA007000 + 0x100 * dp_port))); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA006080 + 0x100 * dp_port), (0xAA006080 + 0x100 * dp_port))); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA007080 + 0x100 * dp_port), (0xAA007080 + 0x100 * dp_port))); 
            /* SF == EGRESS*/ 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA009000 + 0x100 * egr_port), (0xAA009009 + 0x100 * egr_port))); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA009800 + 0x100 * egr_port), (0xAA009800 + 0x100 * egr_port))); 
            /*M2P - Ingress*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00C021 + 0x100 * ing_port), (0xAA00C028 + 0x100 * ing_port))); 
            /*M2P - Egress*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, APERTA2_SW_NO_SIDE, (0xAA00D021 + 0x100 * egr_port), (0xAA00D028 + 0x100 * egr_port))); 

            /* TSCP registers - MainCtrl*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029000, 0xA0029000)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029200, 0xA0029200)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029225, 0xA0029225)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029223, 0xA0029223)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029010, 0xA0029010)); 
            /* TSCP registers - ECC*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029009, 0xA0029009)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029231, 0xA0029231)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029233, 0xA0029233)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029235, 0xA0029239)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002923C, 0xA002923C)); 
            /* TSCP Per Port*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C010, 0xA002C013)); 
            /* TSCP - PCS*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C070, 0xA002C070)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C111, 0xA002C111)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C132, 0xA002C132)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C121, 0xA002C121)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C123, 0xA002C123)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C150, 0xA002C150)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C154, 0xA002C155)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C157, 0xA002C15B)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C160, 0xA002C160)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C167, 0xA002C169)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C170, 0xA002C174)); 
            /* TSCP - AN*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029250, 0xA0029251)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029254, 0xA0029256)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA0029259, 0xA002925B)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C1C0, 0xA002C1C6)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C1D0, 0xA002C1D2)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C1E6, 0xA002C1E6)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C1E8, 0xA002C1E8)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C1E9, 0xA002C1E9)); 
            /* TSCP - FEC*/
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C113, 0xA002C113)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C131, 0xA002C131)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C130, 0xA002C130)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C17A, 0xA002C17B)); 
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA002C351, 0xA002C357)); 

            /* DCCC PORT */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xa0000001, 0xa0000001)); /* LINE+SYS CDPORT 16B */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xa0020006, 0xa0020011)); /* LINE+SYS CDPORT 16B */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xa0020015, 0xa0020015)); /* LINE+SYS CDPORT 16B */

            /* DCCC MAC */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA400010A, 0xA400010B));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA400010F, 0xA400010F));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA400010E, 0xA400010E));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA4000110, 0xA4000110));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA4000112, 0xA4000113));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA4000115, 0xA4000116));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA4000118, 0xA4000118));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA400011A, 0xA400011B));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA400011D, 0xA400011D));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA4020102, 0xA4020103));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA4020109, 0xA4020109));
            
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA500010C, 0xA500010C));	
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA500011C, 0xA500011C));	
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA5000119, 0xA5000119));	
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA6000114, 0xA6000114));	
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, port_ii, port_side, 0xA6000117, 0xA6000117));	
            
            /* MEM RDs */
            PHYMOD_IF_ERR_RETURN(plp_aperta2_print_pm_mem(phy, port_ii, port_side, phymodMemPmMib, 0, 15));
            


        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_dump_comm_regs(const plp_aperta2_phymod_phy_access_t* phy)
{
    uint32_t common_reg_start[APERTA2_MAX_CMN_REG] = APERTA2_COMMON_REGISTER_START_LIST ;
    uint32_t common_reg_end[APERTA2_MAX_CMN_REG]   = APERTA2_COMMON_REGISTER_END_LIST ;
    uint8_t i = 0;
#ifdef APERTA2_REG_DUMP_INTO_FILE
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
    APERTA2_PCS_INFO_PRINT("===================================================================================================================================\n");
    APERTA2_PCS_INFO_PRINT("Dumping Chip-level registers for DIE_%s \n", (phy->access.addr & 0x01) ? "B" : "A");
    APERTA2_PCS_INFO_PRINT("===================================================================================================================================\n");
    APERTA2_PCS_INFO_PRINT("                  : +0     +1     +2     +3     +4     +5     +6     +7     +8     +9     +A     +B     +C     +D     +E     +F\n");
#ifdef APERTA2_REG_DUMP_INTO_FILE
    PHYMOD_FCLOSE(fp);
#endif
    for(i=0; i<APERTA2_MAX_CMN_REG; i++) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_print_regs(phy, APERTA2_PORT_NONE, APERTA2_SW_NO_SIDE, common_reg_start[i], common_reg_end[i]));
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_pcs_info_dump(const plp_aperta2_phymod_phy_access_t *phy)
{
    int macsec_bypass = 0;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG00r_t sw_reg00;
#ifdef APERTA2_REG_DUMP_INTO_FILE
    char verbosity[4];
    PHYMOD_MEMSET(verbosity, 0, sizeof(verbosity));
#endif   
    PHYMOD_IF_ERR_RETURN(
       BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG00r(&phy->access, &sw_reg00)); 
    macsec_bypass = sw_reg00.v[0] & 0xF;
 
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dump_comm_regs(phy));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dump_port_regs(phy));
    if (!macsec_bypass) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_dump_macsec_regs(phy));
    }
#ifdef APERTA2_REG_DUMP_INTO_FILE
    if(phy->access.flags & BCM_PLP_PCS_DIAG_DUMP_L1) {
        PHYMOD_STRNCPY(verbosity, "L1", 3);
    } else {
        PHYMOD_STRNCPY(verbosity, "L2", 3);
    }
   /* PHYMOD_IF_ERR_RETURN(phymod_convert_dump_to_txt("RegDump.txt", (phy->access.addr&1) ? "B":"A", verbosity)); */
#endif
    return PHYMOD_E_NONE;

}

int plp_aperta2_pm_mac_mib_stat_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t stat_type, plp_uint64_t *count)
{
    int port = 0;
    uint32_t mac_counter_data[16];
    uint32_t mac_mem_index = (stat_type/8); /* Each memory row has 512 bytes which corresponds to 8 MIB entries per row */
    uint16_t stat_index =(stat_type%8)*2; 

    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    port = port > 7 ? (port-8) : port;
    if (stat_type == 0xFFFF) {  /* Clear the stat */
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_dc3mac_mib_counter_control_set(phy, 1 /*enable*/, 1 /*Clear*/));
    } else {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_mem_read(phy, phymodMemPmMib/*Mem type*/, (mac_mem_index + (16*port))/*offset*/, mac_counter_data/*data*/));
        COMPILER_64_SET(*count,  (mac_counter_data[stat_index+1] & 0xFF), mac_counter_data[stat_index]);
    }

    return PHYMOD_E_NONE;
}

int  plp_aperta2_pm_diagnostic_dump(const plp_aperta2_phymod_phy_access_t *phy)
{
    return plp_aperta2_pcs_info_dump(phy);
}

/*!
 *  Function to update Initialization State information
 *
 *  @param phy_id          phy ID
 *  @param val             Value to be updated
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_set_init_state(int phy_id, int val)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy_id) {
            _plp_aperta2_pm_info[cnt].init_state = val ;
            PHYMOD_IF_ERR_RETURN(plp_aperta2_write_warmboot_reg( phy_id , APERTA2_FW_INIT_STATE, val, 0, 0 )) ;
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in setting Init State\n"));
    return PHYMOD_E_INTERNAL;
}

/*!
 *  Function to get Initialization State information
 *
 *  @param phy_id          phy ID
 *  @param val             Output, Initialization state 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_get_init_state(int phy_id, int *val)
{
    unsigned short cnt = 0;
    for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy_id) {
            *val = _plp_aperta2_pm_info[cnt].init_state;
            /*PHYMOD_IF_ERR_RETURN(plp_aperta2_write_warmboot_reg( phy_id , APERTA2_FW_INIT_STATE, active, 0, 0 )) ;*/
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_DEBUG_ERROR(("Error in getting Init State\n"));
    return PHYMOD_E_INTERNAL;
}

/*!
 *  Function to set lane map set
 *  
 *  @param phy             phy access information
 *  @param lane_map        TX/RX lane map information 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_lane_swap_set(const plp_aperta2_phymod_phy_access_t *phy, const plp_aperta2_phymod_lane_map_t *sys_lane_map, const plp_aperta2_phymod_lane_map_t *line_lane_map)
{
    int lane_index = 0;
    aperta2_config_lane_t lane_cfg;

    if (sys_lane_map->num_of_lanes < (APERTA2_LANES_PER_OCTAL * APERTA2_PM_OCTAL2)) {
        return PHYMOD_E_PARAM;
    }
    if (line_lane_map->num_of_lanes < (APERTA2_LANES_PER_OCTAL * APERTA2_PM_OCTAL2)) {
        return PHYMOD_E_PARAM;
    }
    lane_cfg.num_sys_lane = 16;
    lane_cfg.num_line_lane = 16;
    for (lane_index = 0; lane_index < (APERTA2_LANES_PER_OCTAL * APERTA2_PM_OCTAL2); lane_index++) {
        if (sys_lane_map->lane_map_rx[lane_index] != sys_lane_map->lane_map_tx[lane_index]) {
            PHYMOD_DEBUG_ERROR(("System side Asymentric lane swaps are not supported\n"));
            return PHYMOD_E_PARAM;
        }
        if (line_lane_map->lane_map_rx[lane_index] != line_lane_map->lane_map_tx[lane_index]) {
            PHYMOD_DEBUG_ERROR(("Line side Asymentric lane swaps are not supported\n"));
            return PHYMOD_E_PARAM;
        }
        lane_cfg.sys_lane_list[lane_index] = sys_lane_map->lane_map_rx[lane_index];
        lane_cfg.line_lane_list[lane_index] = line_lane_map->lane_map_rx[lane_index];
    }
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_fw_config_lanes(phy, &lane_cfg, APERTA2_OP_WRITE));
     
    return PHYMOD_E_NONE;
}
/*!
 *  Function to get remote ability
 *  
 *  @param phy                             phy access information
 *  @param plp_aperta2_phymod_autoneg_ability_t        remote partner abilities 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int _plp_aperta2_phy_autoneg_remote_ability_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_autoneg_ability_t* an_ability_get_type)
{        
    phymod_autoneg_advert_abilities_t remote_ability;
    phymod_autoneg_advert_ability_t ability[APERTA2_MAX_NO_ABILITY];
    int index = 0;

    PHYMOD_MEMSET(an_ability_get_type, 0, sizeof(plp_aperta2_phymod_autoneg_ability_t));
    remote_ability.autoneg_abilities = ability;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_tscp_phy_autoneg_remote_advert_ability_get(phy, &remote_ability));

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
            } else if (ability[index].resolved_num_lanes == 2){
                PHYMOD_AN_CAP_100G_CR2_KR2_SET(an_ability_get_type->an_cap);
            } else {
                PHYMOD_AN_CAP1_100G_CR1_KR1_SET(an_ability_get_type->an_cap_ext);
            }
        }
        if (ability[index].speed == 200000) {
            if (ability[index].resolved_num_lanes == 4) {
                PHYMOD_AN_CAP_200G_CR4_KR4_SET(an_ability_get_type->an_cap);
            } else {
                PHYMOD_AN_CAP1_200G_CR2_KR2_SET(an_ability_get_type->an_cap_ext);
            }
        }
        if (ability[index].speed == 400000) {
            if (ability[index].resolved_num_lanes == 8) {
                PHYMOD_AN_CAP_400G_CR8_KR8_SET(an_ability_get_type->an_cap);
            } else {
                PHYMOD_AN_CAP1_400G_CR4_KR4_SET(an_ability_get_type->an_cap_ext);
            }
        }

        if (ability[index].speed == 40000) {
            if (ability[index].medium == phymodFirmwareMediaTypeCopperCable) {
                PHYMOD_AN_CAP_40G_CR4_SET(an_ability_get_type->an_cap);
            } else {
                PHYMOD_AN_CAP_40G_KR4_SET(an_ability_get_type->an_cap);
            }
        }
        if (ability[index].fec == phymod_fec_None) {
            an_ability_get_type->an_fec = 0x8000;
        }
        if (ability[index].fec  == phymod_fec_CL74) {
            an_ability_get_type->an_fec |= 2;
        } 
        if (ability[index].fec  ==  phymod_fec_CL91) {
            an_ability_get_type->an_fec |= 8;
        }
        if (ability[index].fec  ==  phymod_fec_RS544) {
            an_ability_get_type->an_fec |= 0x20;
        }
        if (ability[index].fec  ==  phymod_fec_RS272) {
            an_ability_get_type->an_fec |= 0x40;
        }
        if (ability[index].fec  ==  phymod_fec_RS544_2XN) {
            an_ability_get_type->an_fec |= 0x80;
        }
        if (ability[index].fec  ==  phymod_fec_RS272_2XN) {
            an_ability_get_type->an_fec |= 0x100;
        }
        an_ability_get_type->capabilities |= (ability[index].pause) << 6;
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to perform synce configuration
 *  
 *  @param phy               phy access information
 *  @param synce_cfg         synce configuration
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_pm_synce_config_set(const plp_aperta2_phymod_phy_access_t* phy, const phymod_synce_cfg_t* synce_cfg)
{
    phymod_synce_cfg_t synce_cfg_get;
    aperta2_clock_gen_t clk_cfg;
    int speed = 0, configured_lane = 0, port_num = 0;
    plp_aperta2_phymod_phy_access_t temp_access;

    PHYMOD_MEMSET(&clk_cfg, 0, sizeof(aperta2_clock_gen_t));
    PHYMOD_MEMSET(&synce_cfg_get, 0, sizeof(phymod_synce_cfg_t));
    PHYMOD_MEMCPY(&temp_access, phy, sizeof(temp_access));

    if ((synce_cfg->clkGenSquelchCfg != phymodClkGenSquelchDisable) &&
        ((synce_cfg->squelchMonitorLanemap <= 0) || (synce_cfg->squelchMonitorLanemap > 0xFFFF))) {
        PHYMOD_DEBUG_ERROR(("portLane out of range :0x%0x, valid values: 0x1 to 0xFF\n",
            synce_cfg->squelchMonitorLanemap));
        return PHYMOD_E_PARAM;
    }

    temp_access.access.lane_mask = synce_cfg->squelchMonitorLanemap;
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_pm_info_lane_speed_get(&temp_access, &speed, &configured_lane, &port_num));
    clk_cfg.ClkPortNum = port_num;
    
    /*  Query the current SyncE configuration to make sure all conditions met before applying new configuration:
     *  1. FW message fails if synce is already disabled and user is still trying to disable it.
     *  2. Update to SyncE configuration on the same rclk_out_pin_sel is not allowed. User needs to disable SyncE and re-enable it.
     *  3. rclk_out_pin_sel is shared between die0 and die1 and can only be enabled on one die at any given time.
     *     This restriction is only applicable for single ended outputs (RCLK1 and RCLK2).
     *     RCLK0 can be enabled on both die0 and die1 at the same time.
     */
    synce_cfg_get.rclk_out_pin_sel = synce_cfg->rclk_out_pin_sel;

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_pm_synce_config_get(phy, &synce_cfg_get));

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
            clk_cfg.ClkGenEn = APERTA2_CLKGEN_DIS;
            clk_cfg.RClkNum = synce_cfg->rclk_out_pin_sel;
        } else {
            /* All conditions met, it is now safe to apply new settings */
            clk_cfg.ClkGenEn = APERTA2_CLKGEN_ENA;
            clk_cfg.ClkLane = synce_cfg->recoveredClkLane;
            clk_cfg.RClkNum = synce_cfg->rclk_out_pin_sel;

            /* Reference code defines LINE_SIDE as 0 and SYS_SIDE as 1 */
            /* FW Message defines LINE_SIDE as 1 and SYS_SIDE as 0 */
            if (synce_cfg->rclk_if_side == 1) {
                clk_cfg.ClkSide = APERTA2_CLKGEN_CLKSIDE_SYS;
            } else if (synce_cfg->rclk_if_side == 0) {
                clk_cfg.ClkSide = APERTA2_CLKGEN_CLKSIDE_LINE;
            } else {
                PHYMOD_DEBUG_ERROR(("%s(), Invalid if side: 0x%x\n", __func__, synce_cfg->rclk_if_side));
                return PHYMOD_E_UNAVAIL;
            }
            switch(synce_cfg->clkGenSquelchCfg) {
                case phymodClkGenSquelchNone:
                    clk_cfg.SquelchMode = APERTA2_CLKGEN_NO_SQUELCH; /* No Squelch Needed.*/
                    break;
                case phymodClkGenSquelchLOS:
                    clk_cfg.SquelchMode = APERTA2_CLKGEN_SQUELCH_LOS; /* Squelch Clock on Loss of Signal.*/
                    break;
                case phymodClkGenSquelchLOL:
                    clk_cfg.SquelchMode = APERTA2_CLKGEN_SQUELCH_LOL; /* Squelch Clock on Loss of Line.*/
                    break;
                default:
                    PHYMOD_DEBUG_ERROR(("%s(), Invalid Squelch Config: 0x%x\n", __func__, synce_cfg->clkGenSquelchCfg));
                    return PHYMOD_E_UNAVAIL;
            }
            if (clk_cfg.RClkNum == APERTA2_CLKGEN_OCT0_RCLKNUM_0 ||
                 clk_cfg.RClkNum == APERTA2_CLKGEN_OCT1_RCLKNUM_0 ) {  /* Differential Clock Output*/
                switch(synce_cfg->divider) {
                    case phymodDivider20:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_20;
                        break;
                    case phymodDivider40:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_40;
                        break;
                    case phymodDivider160:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_160;
                        break;
                    case phymodDivider320:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_320;
                        break;
                    case phymodDivider640:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_640;
                        break;
                    case phymodDivider80:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_80;
                        break;
                    default:
                        PHYMOD_DEBUG_ERROR(("%s(), Invalid divider value:0x%x for rclk output: 0x%x\n", __func__, synce_cfg->divider, clk_cfg.RClkNum));
                        return PHYMOD_E_UNAVAIL;
                }
            } else if ((clk_cfg.RClkNum == APERTA2_CLKGEN_OCT0_RCLKNUM_1) || (clk_cfg.RClkNum == APERTA2_CLKGEN_OCT0_RCLKNUM_2) ||
                       (clk_cfg.RClkNum == APERTA2_CLKGEN_OCT1_RCLKNUM_1) || (clk_cfg.RClkNum == APERTA2_CLKGEN_OCT1_RCLKNUM_2)) { /* Single Ended Clock output */
                switch(synce_cfg->divider) {
                    case phymodDivider80:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_80;
                        break;
                    case phymodDivider120:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_120;
                        break;
                    case phymodDivider240:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_240;
                        break;
                    case phymodDivider480:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_480;
                        break;
                    case phymodDivider520:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_520;
                        break;
                    case phymodDivider960:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_960;
                        break;
                    case phymodDivider1000:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_1000;
                        break;
                    case phymodDivider1040:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_1040;
                        break;
                    case phymodDivider2000:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_2000;
                        break;
                    case phymodDivider2040:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_2040;
                        break;
                    case phymodDivider2080:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_2080;
                        break;
                    case phymodDivider4000:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_4000;
                        break;
                    case phymodDivider4080:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_4080;
                        break;
                    case phymodDivider8160:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_8160;
                        break;
                    case phymodDivider16320:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_16320;
                        break;
                    case phymodDivider32640:
                        clk_cfg.Divider = APERTA2_CLKGEN_DIVIDER_32640;
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
        plp_aperta2_fw_clock_gen (phy, APERTA2_OP_WRITE, &clk_cfg));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to get synce configuration
 *  
 *  @param phy               phy access information
 *  @param synce_cfg         synce configuration
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_pm_synce_config_get(const plp_aperta2_phymod_phy_access_t* phy,  phymod_synce_cfg_t* synce_cfg)
{
    aperta2_clock_gen_t clk_cfg;

    PHYMOD_MEMSET(&clk_cfg, 0, sizeof(aperta2_clock_gen_t));
    clk_cfg.RClkNum = synce_cfg->rclk_out_pin_sel;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_fw_clock_gen (phy, APERTA2_OP_READ, &clk_cfg));

    if (clk_cfg.ClkGenEn == APERTA2_CLKGEN_DIS) {
        synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchDisable;
    } else {
        switch(clk_cfg.SquelchMode) {
            case APERTA2_CLKGEN_NO_SQUELCH:
                synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchNone; /* No Squelch Needed.*/
                break;
            case APERTA2_CLKGEN_SQUELCH_LOS:
                synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchLOS; /* Squelch Clock on Loss of Signal.*/
                break;
            case APERTA2_CLKGEN_SQUELCH_LOL:
                synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchLOL; /* Squelch Clock on Loss of Line.*/
                break;
            default:
                PHYMOD_DEBUG_ERROR(("%s(), Invalid Squelch Config: 0x%x\n", __func__, synce_cfg->clkGenSquelchCfg));
                return PHYMOD_E_UNAVAIL;
        }
        /* Convert this to lanemap*/
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_port_lane_map_get(phy, clk_cfg.ClkPortNum, &synce_cfg->squelchMonitorLanemap));
        synce_cfg->recoveredClkLane = clk_cfg.ClkLane;
        synce_cfg->rclk_out_pin_sel = clk_cfg.RClkNum;
        if (clk_cfg.RClkNum == APERTA2_CLKGEN_OCT0_RCLKNUM_0 ||
            clk_cfg.RClkNum == APERTA2_CLKGEN_OCT1_RCLKNUM_0) {  /* Differential Clock Output*/
            switch(clk_cfg.Divider) {
                case APERTA2_CLKGEN_DIVIDER_20:
                    synce_cfg->divider = phymodDivider20;
                    break;
                case APERTA2_CLKGEN_DIVIDER_40:
                    synce_cfg->divider = phymodDivider40;
                    break;
                case APERTA2_CLKGEN_DIVIDER_160:
                    synce_cfg->divider = phymodDivider160;
                    break;
                case APERTA2_CLKGEN_DIVIDER_320:
                    synce_cfg->divider = phymodDivider320;
                    break;
                case APERTA2_CLKGEN_DIVIDER_640:
                    synce_cfg->divider = phymodDivider640;
                    break;
                case APERTA2_CLKGEN_DIVIDER_80:
                    synce_cfg->divider = phymodDivider80;
                    break;
                default:
                    PHYMOD_DEBUG_ERROR(("%s(), Invalid divider value:0x%x for rclk output: 0x%x\n", __func__, clk_cfg.Divider, clk_cfg.RClkNum));
                    return PHYMOD_E_UNAVAIL;
            }
        } else if ((clk_cfg.RClkNum == APERTA2_CLKGEN_OCT0_RCLKNUM_1) || (clk_cfg.RClkNum == APERTA2_CLKGEN_OCT0_RCLKNUM_2) ||
                (clk_cfg.RClkNum == APERTA2_CLKGEN_OCT1_RCLKNUM_1) || (clk_cfg.RClkNum == APERTA2_CLKGEN_OCT1_RCLKNUM_2)) { /* Single Ended Clock output */
            switch(clk_cfg.Divider) {
                case APERTA2_CLKGEN_DIVIDER_80:
                    synce_cfg->divider = phymodDivider80;
                    break;
                case APERTA2_CLKGEN_DIVIDER_120:
                    synce_cfg->divider = phymodDivider120;
                    break;
                case APERTA2_CLKGEN_DIVIDER_240:
                    synce_cfg->divider = phymodDivider240 ;
                    break;
                case APERTA2_CLKGEN_DIVIDER_480:
                    synce_cfg->divider = phymodDivider480;
                    break;
                case APERTA2_CLKGEN_DIVIDER_520:
                    synce_cfg->divider = phymodDivider520;
                    break;
                case APERTA2_CLKGEN_DIVIDER_960:
                    synce_cfg->divider = phymodDivider960;
                    break;
                case APERTA2_CLKGEN_DIVIDER_1000:
                    synce_cfg->divider = phymodDivider1000;
                    break;
                case APERTA2_CLKGEN_DIVIDER_1040:
                    synce_cfg->divider = phymodDivider1040;
                    break;
                case APERTA2_CLKGEN_DIVIDER_2000:
                    synce_cfg->divider = phymodDivider2000;
                    break;
                case APERTA2_CLKGEN_DIVIDER_2040:
                    synce_cfg->divider = phymodDivider2040;
                    break;
                case APERTA2_CLKGEN_DIVIDER_2080:
                    synce_cfg->divider = phymodDivider2080;
                    break;
                case APERTA2_CLKGEN_DIVIDER_4000:
                    synce_cfg->divider = phymodDivider4000;
                    break;
                case APERTA2_CLKGEN_DIVIDER_4080:
                    synce_cfg->divider =  phymodDivider4080;
                    break;
                case APERTA2_CLKGEN_DIVIDER_8160:
                    synce_cfg->divider = phymodDivider8160;
                    break;
                case APERTA2_CLKGEN_DIVIDER_16320:
                    synce_cfg->divider = phymodDivider16320;
                    break;
                case APERTA2_CLKGEN_DIVIDER_32640:
                    synce_cfg->divider = phymodDivider32640;
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
        if (clk_cfg.ClkSide == APERTA2_CLKGEN_CLKSIDE_SYS) {
            synce_cfg->rclk_if_side = 1;
        } else {
            synce_cfg->rclk_if_side = 0;
        }
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to set port macro timesync
 *  
 *  @param phy               phy access information
 *  @param flags             Various supported Flags 
 *                            0x1  : Enable Rx
 *                            0x4  : Enable One Step processing
 *                            0x8  : Enable -  802.3 CX mode. Disabled - Legacy Broadcom mode will be used.
 *                            0x10 : Enable SOP timestamp mode, ignore MAC_DA bit. If disable, check MAC_DA mode bit. 
 *                            0x20 : Enable MAC_DA timestamp mode. Disabled, SFD timestamp mode will be used.
 *                            0x40 : Port is in reduced preamble mode. 
 *  @param enable            Enable/Disable timesync
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_pm_timesync_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable)
{
    aperta2_device_aux_modes_t auxmode;
    plp_aperta2_phymod_phy_inf_config_t    port_config;
    plp_aperta2_phymod_phy_access_t        fw_access;
    plp_aperta2_phymod_phy_access_t        phy_copy;
    aperta2_ptp_config_t       ptp_cfg;
    uint32_t prt_flags=0, port = 0, lane = 0, oct = 0;
    uint32_t is_fo_enabled = 0, primary_lm = 0;
    uint8_t port_option = 0;
    tscp_timesync_adjust_config_info_t config;
    int rv = 0;

    (void) lane;
    (void) oct;

    PHYMOD_MEMSET(&port_config, 0,   sizeof(plp_aperta2_phymod_phy_inf_config_t));
    PHYMOD_MEMSET(&ptp_cfg,     0,   sizeof(aperta2_ptp_config_t));
    PHYMOD_MEMSET(&config,      0,   sizeof(tscp_timesync_adjust_config_info_t));
    PHYMOD_MEMCPY(&fw_access,   phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&phy_copy,    phy, sizeof(plp_aperta2_phymod_phy_access_t));
    port_config.device_aux_modes = &auxmode;

    config.am_norm_mode = 0x2 ;

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_interface_config_get(phy, prt_flags, &port_config));

    if (phy->access.lane_mask == auxmode.failover_config.lane_map) {
        /* FO lane map: use Primary lanemap to get port*/
        PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_is_fo_enabled(phy, &port_config, &is_fo_enabled, &port, &primary_lm));
        (void)is_fo_enabled;
        fw_access.access.lane_mask = primary_lm;
    } else {
        /* Use Primary lanemap to get port*/
        APERTA2_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, lane, oct);
    }
    ptp_cfg.PortNum = port;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_fw_config_ptp_read (&fw_access, &ptp_cfg));

    if ((flags >> 28) & 0xF) {
        port_option = (flags >> 28);
        ptp_cfg.PortOptions &= ~(0x7 << 4);
        ptp_cfg.PortOptions |= ((port_option & 0x7) << 4);
    }

    /* Pause port */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_send_fw_msg(&fw_access, port_config.data_rate , &auxmode, APERTA2_MSG_PAUSE_PORT, NULL, 0));

    /* Flush port */
    rv = plp_aperta2_send_fw_msg(&fw_access, port_config.data_rate , &auxmode, APERTA2_MSG_FLUSH_PORT, NULL, 0);
    APERTA2_TS_PORT_RESUME(rv, "FLUSH", port);

    if (port_option) {
        rv = plp_aperta2_fw_config_ptp_write (&fw_access, &ptp_cfg);
        APERTA2_TS_PORT_RESUME(rv, "CONFIG_PTP", port);
    }

    (void) flags;

    /* Doing TS only on line side as we dont support TS in system side*/
    {
        uint32_t line_lane_map = 0; 
        phy_copy.port_loc = phymodPortLocLine;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_port_lane_map_get(&phy_copy, port, &line_lane_map));
        phy_copy.access.lane_mask = line_lane_map;
    }
    if (enable == 1) {
        rv = plp_aperta2_tscp_timesync_adjust_set(&phy_copy, flags, &config);
        APERTA2_TS_PORT_RESUME(rv, "TSCP_TIMESYNC_ADJUST_SET", port);
    }
    /* Enable PM TS*/
    rv = plp_aperta2_tscp_timesync_enable_set(&phy_copy, flags, enable);
    APERTA2_TS_PORT_RESUME(rv, "TSCP_TIMESYNC_ENABLE_SET", port);

    /* Resume after enable*/
    PHYMOD_IF_ERR_RETURN(plp_aperta2_send_fw_msg(&fw_access, port_config.data_rate , &auxmode, APERTA2_MSG_RESUME_PORT, NULL, 0));

    return PHYMOD_E_NONE;

}

int plp_aperta2_pm_timesync_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable)
{
    plp_aperta2_phymod_phy_access_t        phy_copy;
    uint32_t line_lane_map = 0; 
    int speed = 0, configured_lane = 0, port_num = 0;
    
    PHYMOD_MEMCPY(&phy_copy,    phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_IF_ERR_RETURN(
         plp_aperta2_pm_info_lane_speed_get(phy, &speed, &configured_lane, &port_num));
    if (port_num == 0xFF) {
        PHYMOD_DEBUG_ERROR(("Invalid port map:%x \n", phy->access.lane_mask));
        return PHYMOD_E_PARAM;
    }
    (void) speed;
    (void) configured_lane;
    phy_copy.port_loc = phymodPortLocLine;
    PHYMOD_IF_ERR_RETURN(
         plp_aperta2_pm_info_port_lane_map_get(&phy_copy, port_num, &line_lane_map));

    phy_copy.access.lane_mask = line_lane_map;


    return plp_aperta2_tscp_timesync_enable_get(&phy_copy, flags, enable);
}

int plp_aperta2_pm_timesync_tx_info_get(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_ts_tx_info_t* ts_tx_info)
{
    tscpmod_ts_tx_info_t tx_info;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_tscp_timesync_tx_info_get(phy, &tx_info));
    ts_tx_info->ts_in_fifo_lo = tx_info.ts_in_fifo_lo;
    ts_tx_info->ts_in_fifo_hi = tx_info.ts_in_fifo_hi;
    ts_tx_info->ts_seq_id = tx_info.ts_seq_id;
    ts_tx_info->ts_sub_nanosec =  tx_info.ts_sub_nanosec;

    return PHYMOD_E_NONE;
}

/*!
 *  Function to get RX and Tx  configuration
 *  
 *  @param phy               phy access information
 *  @param data              Tx/Rx configuration 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_tx_rx_status(const plp_aperta2_phymod_phy_access_t* phy, int *data)
{
    aperta2_register_read_t read_param;
    APERTA2_GET_PORT_FROM_LM(phy->access.lane_mask, read_param.port_number);
    read_param.lane_sel = 0;   
    read_param.pll_sel = 0;    
    read_param.micro_sel = 0;
    read_param.reg_addr = BCM_APERTA2_DC3MAC_DC3MAC_CTRLr;   
    read_param.oct_sel = APERTA2_GET_OCTAL(phy->access.lane_mask);
    APERTA2_GET_PM_SEL(phy, read_param.oct_sel, read_param.pm_sel);

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_register_read(phy, &read_param));
    *data = (read_param.reg_data & 1);
    return PHYMOD_E_NONE;
}

/*!
 *  Function to get Octal crossing port
 *  
 *  @param phy               System Phy access information
 *  @param sys_config        System side configuration 
 *  @param line_lane_map     Line side lane map 
 *  @param port              output, port 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_get_octal_crossing_port(const plp_aperta2_phymod_phy_access_t* phy, aperta2_device_aux_modes_t *sys_aux_mode, unsigned int line_lane_map, int *port, uint8_t *max_port)
{
    int octal_lane_mask = 0;
    int cnt = 0, side = 0, retry = 0;
    plp_aperta2_phymod_phy_access_t temp_access;
    *max_port = 0xFF;
    *port = 0;
    PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    if (sys_aux_mode->octal_crossing ==  bcmAperta2SysOctalCrossing) {
        octal_lane_mask = (phy->access.lane_mask & 0xFF) ? (phy->access.lane_mask << 8) : (phy->access.lane_mask >> 8); 
        side = phymodPortLocSys; 
    } else if (sys_aux_mode->octal_crossing ==  bcmAperta2LineOctalCrossing) {
        octal_lane_mask = (line_lane_map & 0xFF) ? (line_lane_map << 8) : (line_lane_map >> 8); 
        side = phymodPortLocLine; 
    } else {
        *max_port = 0xFF;
        return PHYMOD_E_NONE;
    }
    {
        int prev_speed, prev_lanemap, oc_port;
        temp_access.access.lane_mask = octal_lane_mask;
        temp_access.port_loc = side;
        while (temp_access.access.lane_mask) {
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_pm_info_lane_speed_get(&temp_access, &prev_speed, &prev_lanemap, &oc_port));
            if (oc_port != 0xFF) {
                temp_access.access.lane_mask &= ~prev_lanemap; 
                *port &= ~(0xF << (cnt*4));
                *port |= (oc_port << (cnt*4));
                cnt ++;
            }
            if (retry++ > APERTA2_MAX_PORT) {
                break;
            }
        }
    }
    /*APERTA2_LM_TO_POSSIBLE_PORT_LIST(octal_lane_mask, *port, temp);*/
    *max_port = cnt;
    return PHYMOD_E_NONE;
}

/*!
 *  Function to get Ingress Egress access
 *  
 *  @param phy               Phy access information
 *  @param igr_access        Ingress DP Access 
 *  @param egr_access        egress DP access  
 *  @param port              output, port 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_get_igr_egr_access(const plp_aperta2_phymod_phy_access_t* phy,  plp_aperta2_phymod_phy_access_t *igr_access,  plp_aperta2_phymod_phy_access_t *egr_access)
{
    unsigned int lane_map = 0;
    phymod_phy_port_information_t port_info;
    
    PHYMOD_MEMSET(&port_info, 0, sizeof(phymod_phy_port_information_t));
    PHYMOD_MEMCPY(igr_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(egr_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_port_information(phy, &port_info));
    egr_access->port_loc = phymodPortLocSys;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_port_lane_map_get (egr_access, port_info.logical_port, &lane_map));
    egr_access->access.lane_mask = lane_map;
    igr_access->port_loc = phymodPortLocLine;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_port_lane_map_get (igr_access, port_info.logical_port, &lane_map));
    igr_access->access.lane_mask = lane_map;
    /* Returning after updating lane map to support
     *  cross switch*/
    if (port_info.crossing == phymodOctalCrossingNone) {
         return PHYMOD_E_NONE;
    }

    if (port_info.crossing == phymodOctalCrossingSystem) {
        /* Do Nothing for Ingress*
         * Update Egress*/
        APERTA2_SWAP_OCTAL(egr_access->access.lane_mask);
    } else { /* Line Octal Crossing*/
        /* Do Nothing for Egress*
         * Update Ingress*/
        APERTA2_SWAP_OCTAL(igr_access->access.lane_mask);
    }

    return PHYMOD_E_NONE;
}

/*!
 *  Function to update the init pass
 *  
 *  @param phy               Phy access information
 *  @param pass              init pass value 
 *  @param core_status       Status of code, un-used
 *  @param octal_start       Octal Start
 *  @param octal_end         Octal end
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_core_dload(const plp_aperta2_phymod_core_access_t *core, int pass, 
                              const plp_aperta2_phymod_core_status_t* core_status, uint8_t octal_start, uint8_t octal_end)
{
    plp_aperta2_phymod_phy_init_config_t init_config1;

    PHYMOD_MEMSET(&init_config1, 0, sizeof(plp_aperta2_phymod_phy_init_config_t));
    init_config1.ext_phy_tx_params_user_flag[0] = pass;
    init_config1.ext_phy_tx_params_user_flag[1] = phymodFirmwareLoadMethodNone;
    {
        const plp_aperta2_phymod_phy_init_config_t *init_config = &init_config1;
        PHYMOD_IF_ERR_RETURN(plp_aperta2_core_add(core, init_config, core_status, octal_start, octal_end));
    }

    return PHYMOD_E_NONE;

}

/*!
 *  Function to attach default port
 *  
 *  @param phy               Phy access information
 *  @param init_config       Phy init config
 *  @param octal_start       Octal Start address 
 *  @param octal_end         Octal end address
 *  @param init_config       octal initialization config
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_port_attach(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_phy_init_config_t* init_config, 
                        uint8_t octal_start, uint8_t octal_end)
{
    int port = 0; /* This has information about PHY and port of a PM */
    portmod_port_add_info_t add_info;
    plp_aperta2_phymod_phy_access_t phy_copy;
    int unit = 0;

    PHYMOD_MEMSET(&add_info, 0, sizeof(portmod_port_add_info_t));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_portmod_port_add_info_t_init(unit, &add_info));
    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_aperta2_phymod_phy_access_t));


    /* Initialize both interface_t config and init config */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_portmod_port_init_config_t_init(unit, &add_info.init_config));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_portmod_port_interface_config_t_init(unit, &add_info.interface_config));

    /* Can Add init config on demand*/
    add_info.autoneg_en = init_config->an_en ;
    add_info.link_training_en = init_config->cl72_en;

    add_info.interface_config.flags = 0;
    add_info.interface_config.port_refclk_int = init_config->interface.ref_clock;
    add_info.interface_config.port_op_mode = init_config->op_mode;
    add_info.interface_config.speed = init_config->interface.data_rate;
    add_info.interface_config.max_speed = init_config->interface.data_rate;
    add_info.interface_config.encap_mode = _SHR_PORT_ENCAP_IEEE;
    add_info.octal_start = octal_start;
    add_info.octal_end = octal_end;

    /* Not used in TSBH, so setting it to 0*/
    add_info.interface_config.pll_divider_req = 0;
    if (add_info.interface_config.speed == 0) {
        PHYMOD_DEBUG_ERROR(("Init speed cannot be 0 \n"));
        return PHYMOD_E_CONFIG;
    }
    add_info.interface_config.port_num_lanes = APERTA2_EIGHT_LANE_PORT;
    add_info.speed_config.speed =  APERTA2_SPEED_800G;
    add_info.speed_config.num_lane = add_info.interface_config.port_num_lanes;
    add_info.speed_config.fec = PORTMOD_PORT_PHY_FEC_RS_544_2XN ;
    add_info.speed_config.link_training = 0;
    add_info.speed_config.lane_config = APERTA2_INIT_LANE_CONFIG; /*Init laneconfig*/
    add_info.speed_config.modulation= bcmAperta2ModulationPAM4;
    /* Setting init flags*/
    add_info.interface_config.flags |= (1 << 31);
    add_info.phy_op_datapath = phymodDatapathNormal;

    /* Considering port1 of PM*/
    port = APERTA2_PORT_CONSTRUCTION(phy->access.addr, 0);
    APERTA2_UPDATE_LM(phy->access.addr, 0xFF);
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_port_attach(unit, port, &add_info));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_pm_tx_rx_enable(phy, 0 /*Enable*/,0 /*Single port*/,0 /*Failover*/));
    return 0;
}

/*!
 *  Function to configure octal
 *  
 *  @param phy               Phy access information
 *  @param octal_start       Octal Start address 
 *  @param 0ctal_end         Octal end address
 *  @param init_config       octal initialization config
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_configure_octal(const plp_aperta2_phymod_phy_access_t* phy, uint8_t octal_start, uint8_t octal_end,
                            const plp_aperta2_phymod_phy_init_config_t *init_config)
{
    unsigned int octal = 0;
    plp_aperta2_phymod_core_access_t core;
    aperta2_switch_dpclk_t switch_dp_clk;
    plp_aperta2_phymod_core_status_t core_status;
    plp_aperta2_phymod_phy_init_config_t init_config_cpy;
    aperta2_fw_init_t *fw_init_param = (aperta2_fw_init_t*)(init_config->interface.device_aux_modes);

    PHYMOD_MEMCPY(&init_config_cpy, init_config, sizeof(plp_aperta2_phymod_phy_init_config_t));
    PHYMOD_MEMCPY(&core, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    /* Configure DP clk*/
    for (octal = octal_start; octal <= octal_end; octal++) {
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
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_tsc_clock_sel(phy, switch_dp_clk));
    }

#ifndef APERTA2_OCTAL_NO_BCAST
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_set_init_state(phy->access.addr, APERTA2_INIT_STATE_START));
#endif
    /* Will Discuss with change if necessary*/
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_core_dload(&core, PORTMOD_PORT_ADD_F_INIT_PASS1, &core_status, octal_start, octal_end));

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_core_dload(&core, PORTMOD_PORT_ADD_F_INIT_PASS2, &core_status, octal_start, octal_end));
    (void) core_status;
 
    PHYMOD_IF_ERR_RETURN(plp_aperta2_port_attach(phy, &init_config_cpy, octal_start, octal_end));
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_set_init_state(phy->access.addr, APERTA2_INIT_STATE_COMPLETE));

    for (octal = octal_start; octal <= octal_end; octal++) {
        /*1 - system octal0 4 - line octal0
          2 - system octal1 8 - line octal1*/
        /* System Side*/
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_fw_package_polarity_set (phy, octal));
        /* Line Side*/
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_fw_package_polarity_set (phy, (octal*4)));
    }
   
    return PHYMOD_E_NONE;
}

/*!
 *  Function to re-configure PLL
 *  
 *  @param phy               Phy access information
 *  @param speed_config       speed config data
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_reconfigure_octal_pll(const plp_aperta2_phymod_phy_access_t* phy, portmod_speed_config_t *speed_config,
                                  portmod_speed_config_t *line_speed_config)
{
    portmod_pm_vco_setting_t vco_setting, line_vco_setting;
    int lane_list = 0, octal = 0;
    unsigned int port = 0;
    pm_info_t pm_info;
    struct pm_info_s pm_info_1;
    portmod_vco_type_t sys_cur_tvco = portmodVCOInvalid, line_cur_tvco = portmodVCOInvalid;
    aperta2_fw_init_t fw_init_param ;
    plp_aperta2_phymod_phy_access_t temp_access, *ptemp_access = &temp_access, vco_access;
    uint8_t enabled_port_list[APERTA2_MAX_PORT];
    int max_port = 0, port_start_index = 0, cnt = 0;
    plp_aperta2_phymod_polarity_t sys_polarity, line_polarity;
    int speed = 0, configured_lane = 0, port_num = 0, temp = 0;
    int int_port = 0;

    (void)int_port;

    PHYMOD_MEMSET(&vco_setting, 0, sizeof(portmod_pm_vco_setting_t));
    PHYMOD_MEMSET(&line_vco_setting, 0, sizeof(portmod_pm_vco_setting_t));
    PHYMOD_MEMSET(&pm_info_1, 0, sizeof(pm_info_1));
    PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&vco_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_enabled_port(phy, enabled_port_list));
    pm_info = &pm_info_1;
    /* System VCO setting*/
    vco_setting.num_speeds = 1;
    vco_setting.speed_config_list = speed_config;
    vco_setting.port_starting_lane_list= &lane_list;
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_portmod_pm_speed_config_validate(0/*NA*/, (phy->access.addr << 16), &port, 0x4/*One PLL Switch*/, &vco_setting));

    /* Line VCO setting*/
    line_vco_setting.num_speeds = 1;
    line_vco_setting.speed_config_list = line_speed_config;
    line_vco_setting.port_starting_lane_list= &lane_list;
    /* Make sure to check on line side*/
    temp_access.port_loc = phymodPortLocLine;
    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(ptemp_access, port);
    
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_portmod_pm_speed_config_validate(0/*NA*/, (phy->access.addr << 16), &port, 0x4/*One PLL Switch*/, &line_vco_setting));
    
    /* Revert it back*/
    PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(ptemp_access, port);

    PHYMOD_IF_ERR_RETURN(
          plp_aperta2_get_pm_info(phy->access.addr, pm_info));
    octal = APERTA2_GET_OCTAL(phy->access.lane_mask);
    if (octal == APERTA2_PM_OCTAL1) { /* Octal 1*/
        line_cur_tvco = PM_8x100_GEN2_INFO(pm_info)->tvco;
        sys_cur_tvco = PM_8x100_GEN2_INFO(pm_info)->sys_tvco;
        port_start_index = 0;
        max_port = 8;
    } else {          /* Octal 2*/
        line_cur_tvco = PM_8x100_GEN2_INFO(pm_info)->oc1_tvco;
        sys_cur_tvco = PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco;
        port_start_index = 8;
        max_port = 16;
    }
    if ((sys_cur_tvco != vco_setting.tvco) || (line_cur_tvco != line_vco_setting.tvco)) {
        plp_aperta2_phymod_phy_init_config_t init_config;
        unsigned int port = 0, port_index = 0;

        PHYMOD_MEMSET(&init_config, 0, sizeof(plp_aperta2_phymod_phy_init_config_t));
        temp_access.access.lane_mask=0xFFFF;
        temp_access.port_loc = phymodPortLocLine;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_phy_polarity_get(&temp_access, &line_polarity));
        temp_access.port_loc = phymodPortLocSys;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_phy_polarity_get(&temp_access, &sys_polarity));
        PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));

        if (octal == APERTA2_PM_OCTAL1) { /* Octal 1*/
            fw_init_param.octal0.sys_vco = (vco_setting.tvco == portmodVCO51P5625G) ? aperta2Vco51G : aperta2Vco53G ;
            fw_init_param.octal0.line_vco = (line_vco_setting.tvco == portmodVCO51P5625G) ? aperta2Vco51G : aperta2Vco53G ;
            PM_8x100_GEN2_INFO(pm_info)->sys_tvco =  vco_setting.tvco  ;
            PM_8x100_GEN2_INFO(pm_info)->tvco = line_vco_setting.tvco ;
            temp_access.access.lane_mask = 0xFF;
            /*Update VCO for Warmboot*/
            vco_access.port_loc = phymodPortLocSys;
            PHYMOD_IF_ERR_RETURN(
                  plp_aperta2_update_vco(&vco_access, APERTA2_PM_OCTAL1 ,PM_8x100_GEN2_INFO(pm_info)->sys_tvco )); 
            vco_access.port_loc = phymodPortLocLine;
            PHYMOD_IF_ERR_RETURN(
                  plp_aperta2_update_vco(&vco_access, APERTA2_PM_OCTAL1 ,PM_8x100_GEN2_INFO(pm_info)->tvco )); 
        } else { /* Octal 2*/
            fw_init_param.octal1.sys_vco = (vco_setting.tvco == portmodVCO51P5625G) ? aperta2Vco51G : aperta2Vco53G ;
            fw_init_param.octal1.line_vco = (line_vco_setting.tvco == portmodVCO51P5625G) ? aperta2Vco51G : aperta2Vco53G ;
            PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco =  vco_setting.tvco  ;
            PM_8x100_GEN2_INFO(pm_info)->oc1_tvco = line_vco_setting.tvco ;
            temp_access.access.lane_mask = 0xFF00;
            /*Update VCO for Warmboot*/
            vco_access.port_loc = phymodPortLocSys;
            PHYMOD_IF_ERR_RETURN(
                  plp_aperta2_update_vco(&vco_access, APERTA2_PM_OCTAL2,PM_8x100_GEN2_INFO(pm_info)->oc1_sys_tvco )); 
            vco_access.port_loc = phymodPortLocLine;
            PHYMOD_IF_ERR_RETURN(
                  plp_aperta2_update_vco(&vco_access, APERTA2_PM_OCTAL2 ,PM_8x100_GEN2_INFO(pm_info)->oc1_tvco )); 
        }

        for (port_index = port_start_index; port_index < max_port; port_index++) {
             if (enabled_port_list[port_index]) {
                 port = APERTA2_PORT_DISABLE_PH1 | port_index;
                 PHYMOD_IF_ERR_RETURN( plp_aperta2_send_fw_msg(phy, 0, NULL,
                 		       APERTA2_MSG_DISABLE_PORT, &port, 0));
                 port = APERTA2_PORT_DISABLE_PH2 | port_index;
                 PHYMOD_IF_ERR_RETURN( plp_aperta2_send_fw_msg(phy, 0, NULL,
         			       APERTA2_MSG_DISABLE_PORT, &port, 0));
                 PHYMOD_IF_ERR_RETURN(
                      plp_aperta2_port_active_reset(phy, port_index));
             }
        }
        temp = temp_access.access.lane_mask ;
        /*Handle ports that are octal crossed*/
        /*Previous loop disabled the actual ports on the octal. Below loop
         will disable other ports associated to octals*/
        port_start_index = (port_start_index == 0) ? 8 : 0;
        max_port = port_start_index + 8;
        for (port_index = port_start_index; port_index < max_port; port_index++) {
            temp_access.port_loc = phymodPortLocSys;
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_pm_info_lane_speed_get(&temp_access, &speed, &configured_lane, &port_num));
            if ((port_num != 0xFF) && enabled_port_list[port_num]) {
                APERTA2_SIMPLE_DISABLE_PORT(port_num);
                temp_access.access.lane_mask  &= ~configured_lane;
            }
        }
        temp_access.access.lane_mask = temp;
        for (port_index = port_start_index; port_index < max_port; port_index++) {
            temp_access.port_loc = phymodPortLocLine;
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_pm_info_lane_speed_get(&temp_access, &speed, &configured_lane, &port_num));
            if ((port_num != 0xFF) && enabled_port_list[port_num]) {
                APERTA2_SIMPLE_DISABLE_PORT(port_num);
                temp_access.access.lane_mask  &= ~configured_lane;
            }
        }
        (void) speed;
        PHYMOD_MEMCPY(&temp_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));
        temp_access.access.lane_mask = temp;
        for (cnt = 0; cnt < APERTA2_MAX_PM_INFO; cnt++) {
            if (_plp_aperta2_pm_info[cnt].phy_id == phy->access.addr) {
                PHYMOD_IF_ERR_RETURN(
                     plp_aperta2_port_dp_init(&_plp_aperta2_pm_info[cnt], pm_info, octal, octal));
                break;
            }
        }
 
        init_config.interface.data_rate= speed_config->speed ;
        init_config.interface.device_aux_modes = (void*)&fw_init_param;
        PHYMOD_IF_ERR_RETURN(plp_aperta2_configure_octal(&temp_access, octal, octal,
                            &init_config));
        if (octal == APERTA2_PM_OCTAL1) {
            temp_access.access.lane_mask=0xFF;
        } else {
            temp_access.access.lane_mask=0xFF00;
        }
        /* Need to re-configure polarity as FW clears it*/
        temp_access.port_loc = phymodPortLocLine;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_phy_polarity_set(&temp_access, &line_polarity));
        temp_access.port_loc = phymodPortLocSys;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_phy_polarity_set(&temp_access, &sys_polarity));
        /*Speed config updated to initiate re-config happened*/
        speed_config->flags = 0xF;
    }

    return PHYMOD_E_NONE;
}

/*!
 *  Function to get FEC status 
 *  
 *  @param phy                Phy access information
 *  @param fec_sts            FEC status
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int _plp_aperta2_phy_pam4_fec_status_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_phy_fec_dump_status_t* fec_sts)
{
    unsigned int data = 0 , index = 0, cnt_index = 0; 
    int speed = 0, configured_lane = 0, port_num = 0, temp_port = 0;
    uint32_t mem_data[APERTA2_FDR_SIZE*2];
    uint32_t count = 0, ieee_symbols_corr_cnt_fln[8];
    int actual_count = 0;
    int is_line = APERTA2_IS_LINE_SIDE(phy);
    plp_aperta2_phymod_phy_access_t phy1;
    
    PHYMOD_MEMCPY(&phy1, phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_pm_info_lane_speed_get(phy, &speed, &configured_lane, &port_num));
    (void) configured_lane;
    /* Lanemap handles the octal selection*/
    temp_port = (port_num >= 8) ? (port_num - 8) : (port_num);

    if (fec_sts->fec_status_init == phymodFecStatusInitEnable || 
            fec_sts->fec_status_init == phymodFecStatusInitDisable) {
        if (is_line) {
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_reg32_read(phy, BCM_APERTA2_ID_LINE_PMIF_PM_FDR_CTRLr, &data));
            if (fec_sts->fec_status_init == phymodFecStatusInitEnable) { 
                data |= (1 << temp_port);
            } else {
                data &= ~(1 << temp_port);
            }
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_reg32_write(phy,BCM_APERTA2_ID_LINE_PMIF_PM_FDR_CTRLr, data));
        } else {
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_reg32_read(phy, BCM_APERTA2_ID_SYS_PMIF_PM_FDR_CTRLr, &data));
            if (fec_sts->fec_status_init == phymodFecStatusInitEnable) { 
                data |= (1 << temp_port);
            } else {
                data &= ~(1 << temp_port);
            }
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta2_reg32_write(phy,BCM_APERTA2_ID_SYS_PMIF_PM_FDR_CTRLr, data));
        }
        return PHYMOD_E_NONE;
    } else {
        /*Do Nothing*/
    }
    PHYMOD_MEMSET(fec_sts, 0, sizeof(phymod_phy_fec_dump_status_t));
    for (index = 0 ; index < APERTA2_FDR_MAX_ROW; index++) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_mem_read(phy, phymodMemFDR, ((temp_port * 4) + index) , mem_data));
        if (index < 2) {
            for (cnt_index = 0; cnt_index < APERTA2_FDR_SIZE; cnt_index ++) {
                COMPILER_64_SET(fec_sts->fec_err_cnt.total_frames_err_cnt[(index*APERTA2_FDR_SIZE)+cnt_index], mem_data[(cnt_index*2)+1], mem_data[cnt_index*2]);
            }
        } else {
            COMPILER_64_SET(fec_sts->fec_err_cnt.total_symbols_corr_cnt, mem_data[3],mem_data[2]) ;
            COMPILER_64_SET(fec_sts->fec_err_cnt.total_frame_rev_cnt, mem_data[5], mem_data[4]);
            COMPILER_64_SET(fec_sts->fec_err_cnt.total_frame_uncorr_cnt, mem_data[7], mem_data[6]);
        }
    }
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_tscp_phy_rsfec_symbol_error_counter_get(phy, 8/*Max 8 lane*/, &actual_count, ieee_symbols_corr_cnt_fln));
    for (index = 0 ; index < actual_count; index++) {
         fec_sts->fec_dump_status.ieee_fec_sts.ieee_symbols_corr_cnt_fln[index] = ieee_symbols_corr_cnt_fln[index];
    }
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_tscpmod_fec_error_bits_counter_get (&phy1, speed, &count));
    COMPILER_64_SET(fec_sts->fec_err_cnt.total_bits_corr_cnt[0], 0, count);

    return PHYMOD_E_NONE;
}

/*!
 *  Function to get failover port information 
 *  
 *  @param phy                Phy access information
 *  @param port_number        port number
 *  @param failover_mode      failover mode,
 *                         Bit[3:0] :- 0x00 : bcmplpFailovermodeNone\n
 *                         Bit[7:0] :- 0x21 : bcmplpFailovermodeSwitchToPrimary\n
 *                                  :- 0x31 : bcmplpFailovermodeSwitchToSecondary\n
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int _plp_aperta2_fo_get(const plp_aperta2_phymod_phy_access_t* phy, int port_number, plp_aperta2_phymod_failover_mode_t* failover_mode)
{
    unsigned int port_mode = 0, fo_mux_status = 0;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, 0x101a000 | (port_number << 4), &fo_mux_status));
    
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, 0x101a003 | (port_number << 4), &port_mode));

    if (port_mode & 0xFF) {
        *failover_mode = ((fo_mux_status & 0xF000) == 0) ? phymodFailovermodeSwitchToPrimary : phymodFailovermodeSwitchToSecondary;
    } else {
        *failover_mode = phymodFailovermodeNone;
    }
    
    return PHYMOD_E_NONE;
}

/*!
 *  Function to restore PM info for warmboot 
 *  
 *  @param phy                Phy access information
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
#include <peregrine5_pc_config.h>
int plp_aperta2_restore_pm_info(const plp_aperta2_phymod_phy_access_t* phy )
{
    unsigned int cnt = 0, temp = 0, swreg = 0;
    unsigned int speed = 0;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG0Cr_t swgpreg_0c;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG0Dr_t swgpreg_0d;
    BCM_APERTA2_DIRECT_CTRL_SWGPREG18r_t  swgpreg_18;
    for (cnt = 0; cnt <APERTA2_MAX_PM_INFO; cnt++) {
        if (_plp_aperta2_pm_info[cnt].phy_id == phy->access.addr) {
            PHYMOD_IF_ERR_RETURN(
               BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG18r(&phy->access, &swgpreg_18));
            _plp_aperta2_pm_info[cnt].is_fw_dloaded = (swgpreg_18.v[0] >> APERTA2_FWDLOAD_SHIFT) & 0x3;
            _plp_aperta2_pm_info[cnt].init_state = (swgpreg_18.v[0] >> APERTA2_FWINITSTATE_SHIFT) & 0x3;
 
            for (temp = 0; temp < APERTA2_MAX_PORT; temp++) {
                PHYMOD_IF_ERR_RETURN(
                     plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_SWS_SWREG_000r + (temp*5), &swreg));
                _plp_aperta2_pm_info[cnt].port_info[temp].sys_fo_map =  (swreg) ;
                PHYMOD_IF_ERR_RETURN(
                     plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_SWS_SWREG_004r + (temp*5), &swreg));
                _plp_aperta2_pm_info[cnt].port_info[temp].line_fo_map = (swreg) ;
                PHYMOD_IF_ERR_RETURN(
                     plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_SWS_SWREG_002r + (temp*5), &swreg));
                _plp_aperta2_pm_info[cnt].port_info[temp].sys_lane_map =  swreg;
                PHYMOD_IF_ERR_RETURN(
                     plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_SWS_SWREG_003r + (temp*5), &swreg));
                _plp_aperta2_pm_info[cnt].port_info[temp].line_lane_map =  swreg;
                PHYMOD_IF_ERR_RETURN(
                     plp_aperta2_direct_reg_read(phy, BCM_APERTA2_DIRECT_SWS_SWREG_001r + (temp*5), &speed));
                _plp_aperta2_pm_info[cnt].port_info[temp].line_speed = plp_aperta2_get_speed_from_bits(speed >> APERTA2_LINE_SPD_SHIFT);
                _plp_aperta2_pm_info[cnt].port_info[temp].sys_speed = plp_aperta2_get_speed_from_bits(speed & 0xFF);
            }
            PHYMOD_IF_ERR_RETURN(
                   BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Cr(&phy->access, &swgpreg_0c));
            PHYMOD_IF_ERR_RETURN(
                        BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Dr(&phy->access, &swgpreg_0d));
            PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->tvco = swgpreg_0c.v[0] & 0xFF;
            PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->sys_tvco = (swgpreg_0c.v[0] >> 8) & 0xFF;
            PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->oc1_tvco = swgpreg_0d.v[0] & 0xFF;
            PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->oc1_sys_tvco = (swgpreg_0d.v[0] >> 8) & 0xFF;
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_peregrine5_pc_init_peregrine5_pc_info((srds_access_t *)phy));
            return PHYMOD_E_NONE;
        }
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to initialize warmboot 
 *  
 *  @param phy                Phy access information
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_pm_warmboot_init(const plp_aperta2_phymod_phy_access_t* phy)
{
    int unit = 0;
    portmod_pm_create_info_t pm_create_info;
    portmod_pm_create_info_internal_t pm_internal_info;
    pm_info_t init_pm_info;
    struct pm_info_s init_pm_info_1;
    plp_aperta2_phymod_phy_access_t phy_access;
    plp_aperta2_phymod_core_firmware_info_t fw_info;

    PHYMOD_MEMSET(&init_pm_info_1, 0, sizeof(init_pm_info_1));
    init_pm_info = &init_pm_info_1;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_init_db((plp_aperta2_phymod_core_access_t*) phy, NULL));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_pm_create_info_t_init(unit, &pm_create_info));
    pm_create_info.phys = 1;
#ifdef PHYMOD_APERTA2_SUPPORT
    pm_create_info.type = portmodDispatchTypePm8x100_gen2;
#endif
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_phy_access_t_init(&pm_create_info.pm_specific_info.pm8x100_gen2.access));
    /* Moving access, MDIO address, bus etc*/
    PHYMOD_MEMCPY(&pm_create_info.pm_specific_info.pm8x100_gen2.access, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    pm_create_info.pm_specific_info.pm8x100_gen2.fw_load_method &= 0xff;

    /* Use default external loader if NULL */
    pm_create_info.pm_specific_info.pm8x100_gen2.external_fw_loader = NULL;

    /* Polarity */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_polarity_t_init (&(pm_create_info.pm_specific_info.pm8x100_gen2.polarity)));

    /* Lane map */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_phymod_lane_map_t_init(&(pm_create_info.pm_specific_info.pm8x100_gen2.lane_map)));


    pm_create_info.pm_specific_info.pm8x100_gen2.lane_map.num_of_lanes = APERTA2_MAX_NUM_LANES;

    /* Defaults to 312MHZ*/
    pm_create_info.pm_specific_info.pm8x100_gen2.ref_clk = phymodRefClk312Mhz;

    PHYMOD_MEMCPY(&pm_internal_info, &pm_create_info, sizeof(portmod_pm_create_info_internal_t));

    PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_core_firmware_info_get(&phy->access, &fw_info));
    PHYMOD_CRIT_INFO((" In Warmboot ..\n"));
    if( fw_info.fw_version == 0x1)/*when no fw is present*/
    {
        PHYMOD_CRIT_INFO(("Firmware is not present \n Cannot perform warmboot without firmware . "));
        return PHYMOD_E_INTERNAL;
    }
    PHYMOD_CRIT_INFO((" FW version:0x%x\n", fw_info.fw_version));


    /* Add PM to PortMod */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_pm_init(unit, &pm_internal_info, 0, init_pm_info));


#ifdef PHYMOD_APERTA2_SUPPORT
    init_pm_info->pm_data.aperta2_pm8x100_gen2_db->int_core_access.type = phymodDispatchTypeAperta2;
    PM_8x100_GEN2_INFO(init_pm_info)->int_core_access.type = phymodDispatchTypeAperta2;
    PM_8x100_GEN2_INFO(init_pm_info)->int_core_access.access.tvco_pll_index = 0;
    PM_8x100_GEN2_INFO(init_pm_info)->external_fw_loader = NULL;
    PHYMOD_MEMCPY(&PM_8x100_GEN2_INFO(init_pm_info)->int_phy_access, &PM_8x100_GEN2_INFO(init_pm_info)->int_core_access,
        sizeof(plp_aperta2_phymod_phy_access_t));
#endif
    PHYMOD_MEMCPY(&phy_access, phy, sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_ACC_F_CLAUSE45_SET(&(PM_8x100_GEN2_INFO(init_pm_info)->int_core_access.access));
    PHYMOD_IF_ERR_RETURN(plp_aperta2_add_pm_info(phy->access.addr, init_pm_info));
    /* Restore values in global struct array using pm_info from register */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_restore_pm_info(phy ));

    return PHYMOD_E_NONE;
}

