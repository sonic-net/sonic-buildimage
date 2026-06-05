/*
 *
 * $Id: phymod.xml,v 1.1.2.5 Broadcom SDK $
 *
 *  *
 *  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */

#include <phymod/phymod.h>
#include <phymod/phymod_dispatch.h>

#ifdef PHYMOD_APERTA_SUPPORT
#include "../aperta_pm/include/portmod_internal.h"
#include <tier1/aperta_pm_seq.h>
#include <tier1/aperta_cfg_seq.h>
#include <tier1/aperta_reg_access.h>
#include <include/bcmi_aperta_tscbh_xgxs_defs.h>
#include <include/aperta_tscbh.h>
#include <include/pm8x50.h>
#include <tier1/aperta_msg_tasks.h>
#include <include/portmod.h>
#include <phymod/phymod_util.h>

#ifdef PHYMOD_TIMESYNC_SUPPORT
#include "timesync.h"
#endif

#define APERTA_TSCBH 0
extern aperta_pm_info_t _plp_aperta_pm_info[APERTA_MAX_PM_INFO];


int plp_aperta_core_disable_81385_die_b(const plp_aperta_phymod_core_access_t *core)
{
    plp_aperta_phymod_core_access_t core_die_b;
    BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_t pll_ctrl12;
    BCMI_APERTA_D_CTRL_RESET_CTRLr_t reset_ctrl;

    PHYMOD_MEMCPY(&core_die_b, core, sizeof(plp_aperta_phymod_core_access_t));
    PHYMOD_MEMSET(&pll_ctrl12, 0, sizeof(BCMI_APERTA_D_ANA_PLPPLL_CTRL12r_t));
    PHYMOD_MEMSET(&reset_ctrl, 0, sizeof(BCMI_APERTA_D_CTRL_RESET_CTRLr_t));

    core_die_b.access.addr += 1;

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_CTRL_RESET_CTRLr(&core_die_b.access, &reset_ctrl));
    /* Configure Die B's reset control register to reset values*/
    BCMI_APERTA_D_CTRL_RESET_CTRLr_SET(reset_ctrl, 0x0080);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_CTRL_RESET_CTRLr(&core_die_b.access, reset_ctrl));

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_ANA_PLPPLL_CTRL12r(&core_die_b.access, &pll_ctrl12));

    /* Disable differential ref-clock outputs*/
    pll_ctrl12.v[0] |= (1 << 15) | ( 1 << 14);

    /* Turn off analog PLL 1G and 25G clock output*/
    pll_ctrl12.v[0] &= ~(1 << 12) & ~(1 << 10);

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_ANA_PLPPLL_CTRL12r(&core_die_b.access, pll_ctrl12));

    return PHYMOD_E_NONE;
}

int plp_aperta_core_identify(const plp_aperta_phymod_core_access_t* core, uint32_t core_id, uint32_t* is_identified)
{
    int chip_id = 0;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_get_chip_id(&core->access, &chip_id));
    if (chip_id == APERTA_CHIP_81343 || chip_id == APERTA_CHIP_81394 ||
        chip_id == APERTA_CHIP_81384 || chip_id == APERTA_CHIP_81398 ||
        chip_id == APERTA_CHIP_81388 || chip_id == APERTA_CHIP_81392 ||
        (chip_id == APERTA_CHIP_81385 && !(core->access.addr & 0x1))) {

        *is_identified = 1;
        /* Adding Bcast Suport*/
        *is_identified |= 0x80000000;
        if (chip_id == APERTA_CHIP_81385) {
            if (!(core->access.flags & BCM_PLP_WARM_BOOT)) {
                PHYMOD_IF_ERR_RETURN(plp_aperta_core_disable_81385_die_b(core));
            }
        }
    } else {
        *is_identified = 0;
    }

    return PHYMOD_E_NONE;

}


int plp_aperta_core_info_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_core_info_t* info)
{
   /* plp_aperta_phymod_phy_access_t phy ;
    uint32_t rev_number;
    BCMI_TSCBH_XGXS_MAIN0_SERDESIDr_t MAIN0_SERDESIDr_reg;

    PHYMOD_MEMCPY(&phy, core, sizeof(phy));
    phy.access.lane_mask = 0xF;
    PHYMOD_IF_ERR_RETURN(BCMI_TSCBH_XGXS_READ_MAIN0_SERDESIDr(&phy, &MAIN0_SERDESIDr_reg));
    PHYMOD_DIAG_OUT(("Core info:%x\n", MAIN0_SERDESIDr_reg._main0_serdesid));

    info->serdes_id = BCMI_TSCBH_XGXS_MAIN0_SERDESIDr_GET(MAIN0_SERDESIDr_reg);
    rev_number = BCMI_TSCBH_XGXS_MAIN0_SERDESIDr_REV_LETTERf_GET(MAIN0_SERDESIDr_reg);
    if (rev_number == 1)*/ {
            info->core_version = phymodCoreVersionAperta;
        }

    /* coverity[buffer_size] */
    PHYMOD_STRNCPY(info->name, "APRT", 5);
    return PHYMOD_E_NONE;

}


int plp_aperta_core_reset_set(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_reset_mode_t reset_mode, plp_aperta_phymod_reset_direction_t direction)
{

    return _plp_aperta_core_reset_set(core, reset_mode, direction);
}

int plp_aperta_core_reset_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_reset_mode_t reset_mode, plp_aperta_phymod_reset_direction_t* direction)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}


int plp_aperta_core_firmware_info_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_core_firmware_info_t* fw_info)
{

    return _plp_aperta_core_firmware_info_get(&core->access, fw_info);
}


int plp_aperta_phy_firmware_core_config_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_firmware_core_config_t fw_core_config)
{
    return plp_aperta_tscbh_phy_firmware_core_config_set(phy, fw_core_config);
}

int plp_aperta_phy_firmware_core_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_firmware_core_config_t* fw_core_config)
{
    return plp_aperta_tscbh_phy_firmware_core_config_get(phy, fw_core_config);
}


int plp_aperta_phy_firmware_lane_config_set(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_lane_config)
{
    return plp_aperta_tscbh_phy_firmware_lane_config_set(phy, fw_lane_config);

}

int plp_aperta_phy_firmware_lane_config_get(const plp_aperta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t* fw_lane_config)
{
    return plp_aperta_tscbh_phy_firmware_lane_config_get(phy, fw_lane_config);

}


int plp_aperta_core_pll_sequencer_restart(const plp_aperta_phymod_core_access_t* core, uint32_t flags, plp_aperta_phymod_sequencer_operation_t operation)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}


int plp_aperta_phy_rx_restart(const plp_aperta_phymod_phy_access_t* phy)
{
    return plp_aperta_tscbh_phy_rx_restart(phy);
}


int plp_aperta_phy_polarity_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_polarity_t* polarity)
{
    unsigned int lane_index = 0;
    plp_aperta_phymod_polarity_t polarity_temp;
    plp_aperta_phymod_phy_access_t phy_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_MEMCPY(&polarity_temp, polarity, sizeof(plp_aperta_phymod_polarity_t));

    for (lane_index = 0; lane_index < 8; lane_index++) {
        if (phy->access.lane_mask & (1<<lane_index)) {
            phy_temp.access.lane_mask = (1<<lane_index);
            if (polarity->tx_polarity != 0xFFFF) {
                polarity_temp.tx_polarity = (polarity->tx_polarity & (1 << lane_index)) >> lane_index;
            }
            if (polarity->rx_polarity != 0xFFFF) {
                polarity_temp.rx_polarity = (polarity->rx_polarity & (1 << lane_index)) >> lane_index;
            }
            PHYMOD_IF_ERR_RETURN(
                    plp_aperta_tscbh_phy_polarity_set(&phy_temp, &polarity_temp));
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_phy_polarity_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_polarity_t* polarity)
{
    unsigned int lane_index = 0;
    plp_aperta_phymod_phy_access_t phy_temp;
    plp_aperta_phymod_polarity_t polarity_temp;

    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_aperta_phymod_phy_access_t));
    PHYMOD_MEMSET(polarity, 0, sizeof(plp_aperta_phymod_polarity_t));
    for (lane_index = 0; lane_index < 8; lane_index++) {
        phy_temp.access.lane_mask = (1<<lane_index);
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_tscbh_phy_polarity_get(&phy_temp, &polarity_temp));
        polarity->tx_polarity |= (((polarity_temp.tx_polarity) ? 1 : 0) << lane_index);
        polarity->rx_polarity |= (((polarity_temp.rx_polarity) ? 1 : 0) << lane_index);
    }
    return PHYMOD_E_NONE;
}


int plp_aperta_phy_pam4_tx_set(const plp_aperta_phymod_phy_access_t* phy, const phymod_pam4_tx_t* tx)
{
    return plp_aperta_tscbh_phy_pam4_tx_set(phy, tx);

}

int plp_aperta_phy_pam4_tx_get(const plp_aperta_phymod_phy_access_t* phy, phymod_pam4_tx_t* tx)
{
    return plp_aperta_tscbh_phy_pam4_tx_get(phy, tx);

}


int plp_aperta_phy_media_type_tx_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_media_typed_t media, plp_aperta_phymod_tx_t* tx)
{


    /* Place your code here */


    return PHYMOD_E_NONE;

}


int plp_aperta_phy_rx_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_rx_t* rx)
{

    return plp_aperta_tscbh_phy_rx_set(phy, rx);

}

int plp_aperta_phy_rx_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_rx_t* rx)
{
    PHYMOD_MEMSET(rx,  0, sizeof(plp_aperta_phymod_rx_t));
    return plp_aperta_tscbh_phy_rx_get(phy, rx);

}


int plp_aperta_phy_rx_adaptation_resume(const plp_aperta_phymod_phy_access_t* phy)
{

    return plp_aperta_tscbh_phy_rx_adaptation_resume(phy);
}


int plp_aperta_phy_reset_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_reset_t* reset)
{

    return PHYMOD_E_UNAVAIL;

}

int plp_aperta_phy_reset_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_reset_t* reset)
{
    return PHYMOD_E_UNAVAIL;

}


int plp_aperta_phy_power_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_power_t* power)
{
    return plp_aperta_tscbh_phy_power_set(phy, power);

}

int plp_aperta_phy_power_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_power_t* power)
{
    return plp_aperta_tscbh_phy_power_get(phy, power);
}


int plp_aperta_phy_tx_lane_control_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_tx_lane_control_t tx_control)
{
    return plp_aperta_tscbh_phy_tx_lane_control_set(phy, tx_control);

}

int plp_aperta_phy_tx_lane_control_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_tx_lane_control_t* tx_control)
{
    return plp_aperta_tscbh_phy_tx_lane_control_get(phy, tx_control);
}


int plp_aperta_phy_rx_lane_control_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_rx_lane_control_t rx_control)
{
    return plp_aperta_tscbh_phy_rx_lane_control_set(phy, rx_control);
}

int plp_aperta_phy_rx_lane_control_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_rx_lane_control_t* rx_control)
{
    return plp_aperta_tscbh_phy_rx_lane_control_get(phy, rx_control);
}


int plp_aperta_phy_fec_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t enable)
{
    PHYMOD_DEBUG_ERROR(("Please use bcm_plp_mode_config_set to configure FEC\n"));
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_phy_fec_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* enable)
{
    PHYMOD_DEBUG_ERROR(("Please use bcm_plp_mode_config_get to get configured FEC type\n"));
    return PHYMOD_E_UNAVAIL;
}


int plp_aperta_phy_interface_config_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta_phymod_phy_inf_config_t* config)
{
    plp_aperta_phymod_phy_inf_config_t config_temp;
    int pif;
    aperta_device_aux_modes_t auxmode;
    if (!config->device_aux_modes) {
        PHYMOD_DEBUG_ERROR(("Aux mode cannot be NULL\n"));
        return PHYMOD_E_PARAM;
    }
    auxmode = *(aperta_device_aux_modes_t*)config->device_aux_modes;

    PHYMOD_MEMCPY(&config_temp, config, sizeof(plp_aperta_phymod_phy_inf_config_t));

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
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_interface_config_set(phy, flags, &config_temp));
    PHYMOD_IF_ERR_RETURN(plp_aperta_sw_intf_set(phy, config->interface_type));
    if (auxmode.failover_config.lane_map != 0) {
        plp_aperta_phymod_phy_access_t phy_cpy;
        PHYMOD_MEMCPY(&phy_cpy, phy, sizeof(plp_aperta_phymod_phy_access_t));
        phy_cpy.access.lane_mask = auxmode.failover_config.lane_map; 
        PHYMOD_IF_ERR_RETURN(plp_aperta_sw_intf_set(&phy_cpy, config->interface_type));
    }

    return PHYMOD_E_NONE;

}

int plp_aperta_phy_interface_config_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_ref_clk_t ref_clock, plp_aperta_phymod_phy_inf_config_t* config)
{
    uint8_t lane_index  =0;

    /* PM configuration*/
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_interface_config_get(phy, flags, config));

    for (lane_index =0; lane_index< APERTA_MAX_LANES; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_sw_intf_get(phy, lane_index, &config->interface_type));
            return PHYMOD_E_NONE;
        }
    }

    return PHYMOD_E_NONE;

}


int plp_aperta_phy_cl72_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t cl72_en)
{
    return plp_aperta_tscbh_phy_cl72_set(phy, cl72_en);

}

int plp_aperta_phy_cl72_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* cl72_en)
{
    return plp_aperta_tscbh_phy_cl72_get(phy, cl72_en);

}


int plp_aperta_phy_cl72_status_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_cl72_status_t* status)
{
    return plp_aperta_tscbh_phy_cl72_status_get(phy, status);
}

int plp_aperta_phy_autoneg_ability_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_autoneg_ability_t* an_ability_set_type)
{
    phymod_autoneg_advert_abilities_t loc_phy_ability;
    phymod_autoneg_advert_ability_t advert_ability;
    portmod_port_speed_ability_t port_abilities;
    plp_aperta_phymod_phy_inf_config_t config;
    int num_abilities = 0;
    aperta_device_aux_modes_t auxmode;
    int port = 0, pm_ability = 0, pll1_div = 0;
    BCMI_APERTA_D_CTRL_SWGPREG05r_t gpreg5;
    int number_of_ability = 0, capability = 0;
    BCMI_APERTA_D_SWS_SWREG_000r_t sw_an_mode;
 
    PHYMOD_MEMSET(&gpreg5, 0, sizeof(BCMI_APERTA_D_CTRL_SWGPREG05r_t));
    PHYMOD_MEMSET(&auxmode, 0, sizeof(aperta_device_aux_modes_t));
    APERTA_UPDATE_PM_INFO(phy->access.addr, phy);
    if (an_ability_set_type->an_cl72 == 0) {
        PHYMOD_DEBUG_ERROR(("an_cl72 must be 1\n"));
        return PHYMOD_E_PARAM;
    }
    config.device_aux_modes = &auxmode;
    loc_phy_ability.autoneg_abilities = &advert_ability;
    loc_phy_ability.num_abilities = 1;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_phy_interface_config_get(phy, 0, phymodRefClk156Mhz, &config));
 
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_get_pll1_div (phy, &pll1_div));

    /* All 53P125G lanes need PLL1 with 26G VCO*/ 
    if ((auxmode.lane_data_rate == bcmplpApertaLaneDataRate_53P125G) && (pll1_div != APERTA_26GVCO_VALUE)) {
        PHYMOD_DEBUG_ERROR(("PLL1(0x%x) need to be configured with 26G VCO \n", pll1_div));
        return PHYMOD_E_PARAM;
    }

    number_of_ability = plp_aperta_count_no_bits(an_ability_set_type->an_cap);
    capability = an_ability_set_type->an_cap;
    PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_SWS_SWREG_000r(&phy->access, &sw_an_mode));
    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
    while (number_of_ability) {
        PHYMOD_MEMSET(&port_abilities, 0, sizeof(port_abilities));
        PHYMOD_MEMSET(&advert_ability, 0, sizeof(phymod_autoneg_advert_ability_t));
        pm_ability = 0;
        advert_ability.speed = config.data_rate ;
#ifdef APERTA_MULTI_SPEED_AN_SUPPORT
        if(PHYMOD_AN_CAP_40G_KR4_GET(capability)|| PHYMOD_AN_CAP_40G_CR4_GET(capability) ) {
            advert_ability.speed = 40000 ;
        }
        if(PHYMOD_AN_CAP_100G_KR4_GET(capability) || PHYMOD_AN_CAP_100G_CR4_GET(capability)) {
            advert_ability.speed = 100000 ;
        }
#endif
        advert_ability.resolved_num_lanes = plp_aperta_count_no_bits(phy->access.lane_mask);
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
        if (PHYMOD_AN_CAP_25G_KR1_GET    (capability) ||
                PHYMOD_AN_CAP_25G_CR1_GET(capability) ||
                PHYMOD_AN_CAP_50G_CR2_GET(capability) ||
                PHYMOD_AN_CAP_50G_KR2_GET(capability)) {
            advert_ability.an_mode = phymod_AN_MODE_MSA;
            /* Set bit 1 for MSA*/
            sw_an_mode.v[0] |= (1 << (port *2));
            /* Clearing to check other ability*/
            PHYMOD_AN_CAP_25G_KR1_CLR(capability);    
            PHYMOD_AN_CAP_25G_CR1_CLR(capability);
            PHYMOD_AN_CAP_50G_CR2_CLR(capability);
            PHYMOD_AN_CAP_50G_KR2_CLR(capability);

        } else {
            /* Set bit 2 for IEEE*/
            sw_an_mode.v[0] |= (2 << (port *2));
            advert_ability.an_mode = phymod_AN_MODE_CL73;
        }
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_SWS_SWREG_000r(&phy->access, sw_an_mode));
        if (advert_ability.speed == APERTA_SPEED_25G) {
            PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_CTRL_SWGPREG05r(&phy->access, &gpreg5));
            if (advert_ability.an_mode == phymod_AN_MODE_CL73) {
                /* 0 for CL73*/
                gpreg5.v[0] &= ~(3 << (port*2));
                if(advert_ability.channel == PORTMOD_PORT_PHY_CHANNEL_SHORT) {
                    /* 2 for KRS*/
                    if(PHYMOD_AN_CAP_25G_KRS1_GET(an_ability_set_type->an_cap)) {
                        gpreg5.v[0] |= (2 << (port *2));
                    }
                }
            } else {
                /* 1 for BAM/MSA*/
                gpreg5.v[0] |= (1 << (port *2));
            }
            PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_CTRL_SWGPREG05r(&phy->access, gpreg5));
        }
        if (an_ability_set_type->an_fec == 0) {
            advert_ability.fec = 1 << phymod_fec_None;
        } else if(an_ability_set_type->an_fec == 1 || an_ability_set_type->an_fec == 2) {
            advert_ability.fec = 1 << phymod_fec_CL74;
        } else if(an_ability_set_type->an_fec == 4 || an_ability_set_type->an_fec == 8) {
            advert_ability.fec = 1 << phymod_fec_CL91;
        } else {
            advert_ability.fec = 1 << phymod_fec_None;
        }
        PHYMOD_IF_ERR_RETURN(
                _plp_aperta_pm8x50_port_phy_to_port_ability(&loc_phy_ability, &num_abilities, &port_abilities));
        if (an_ability_set_type->an_fec == 0) {
            pm_ability = (1 << PORTMOD_PORT_PHY_FEC_NONE);
        } 
        if ((an_ability_set_type->an_fec & 1) || (an_ability_set_type->an_fec & 2)) {
            pm_ability |= (1 << PORTMOD_PORT_PHY_FEC_BASE_R);
        }
        if ((an_ability_set_type->an_fec & 4) || (an_ability_set_type->an_fec & 8)) {
            pm_ability |= (1 << PORTMOD_PORT_PHY_FEC_RS_FEC);
        }
        if (pm_ability == 0) {
            pm_ability = (1 << PORTMOD_PORT_PHY_FEC_NONE);
        }
        port_abilities.fec_type = pm_ability;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_portmod_port_autoneg_ability_advert_set(0, (phy->access.addr << 8), 1, &port_abilities));
    }
    return 0;
}

int plp_aperta_phy_autoneg_ability_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_ability_t* an_ability_get_type)
{
    portmod_port_speed_ability_t port_abilities[APERTA_MAX_NO_ABILITY];
    phymod_autoneg_advert_abilities_t loc_phy_ability;
    phymod_autoneg_advert_ability_t advert_ability[APERTA_MAX_NO_ABILITY];
    int max_ability = 0, port = 0, an_mode =0, index = 0;
    BCMI_APERTA_D_CTRL_SWGPREG05r_t gpreg5;
 
    PHYMOD_MEMSET(&gpreg5, 0, sizeof(BCMI_APERTA_D_CTRL_SWGPREG05r_t));
    PHYMOD_MEMSET(advert_ability, 0 , sizeof(phymod_autoneg_advert_ability_t)*APERTA_MAX_NO_ABILITY);
    APERTA_UPDATE_PM_INFO(phy->access.addr, phy);
    loc_phy_ability.autoneg_abilities = advert_ability;
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_portmod_port_autoneg_ability_advert_get(0, (phy->access.addr << 8), 2, port_abilities, &max_ability));

    PHYMOD_IF_ERR_RETURN(
        _plp_aperta_pm8x50_port_port_to_phy_ability(max_ability, port_abilities, &loc_phy_ability));
    /* Both IEEE/MSA An mode is set*/
    if (port_abilities[0].an_mode == 0xb) {
        an_mode = 1;
    }

    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_CTRL_SWGPREG05r(&phy->access, &gpreg5));
    PHYMOD_DEBUG_INFO(("Number of abilities:%d\n", ((max_ability > APERTA_MAX_NO_ABILITY) ? APERTA_MAX_NO_ABILITY : max_ability)));
    for (index = 0; index < ((max_ability > APERTA_MAX_NO_ABILITY) ? APERTA_MAX_NO_ABILITY : max_ability) ; index++) {
        if (loc_phy_ability.autoneg_abilities[index].speed == 10000) {
            PHYMOD_AN_CAP_10G_KR_SET(an_ability_get_type->an_cap);
        }
        if (loc_phy_ability.autoneg_abilities[index].speed == 25000) {
            APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
            if (an_mode || loc_phy_ability.autoneg_abilities[index].an_mode == phymod_AN_MODE_CL73) {
                if (loc_phy_ability.autoneg_abilities[index].channel == phymod_channel_short) {
                    if (gpreg5.v[0] & (3 << (port*2))) {
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
                PHYMOD_AN_CAP_50G_CR_KR_SET(an_ability_get_type->an_cap);
            }
        }
        if (loc_phy_ability.autoneg_abilities[index].speed == 100000) {
            if (loc_phy_ability.autoneg_abilities[index].resolved_num_lanes == 4) {
                if (loc_phy_ability.autoneg_abilities[index].medium == phymodFirmwareMediaTypeCopperCable) {
                    PHYMOD_AN_CAP_100G_CR4_SET(an_ability_get_type->an_cap);
                } else {
                    PHYMOD_AN_CAP_100G_KR4_SET(an_ability_get_type->an_cap);
                }
            } else {
                PHYMOD_AN_CAP_100G_CR2_KR2_SET(an_ability_get_type->an_cap);
            }
        }
        if (loc_phy_ability.autoneg_abilities[index].speed == 200000) {
            PHYMOD_AN_CAP_200G_CR4_KR4_SET(an_ability_get_type->an_cap);
        }
        if (loc_phy_ability.autoneg_abilities[index].speed == 40000) {
            if (loc_phy_ability.autoneg_abilities[index].medium == phymodFirmwareMediaTypeCopperCable) {
                PHYMOD_AN_CAP_40G_CR4_SET(an_ability_get_type->an_cap);
            } else {
                PHYMOD_AN_CAP_40G_KR4_SET(an_ability_get_type->an_cap);
            }
        }
        if (loc_phy_ability.autoneg_abilities[index].fec == phymod_fec_None) {
            an_ability_get_type->an_fec = 0;
        }
        if (loc_phy_ability.autoneg_abilities[index].fec & (1<< phymod_fec_CL74)) {
            an_ability_get_type->an_fec |= 2;
        } 
        if (loc_phy_ability.autoneg_abilities[index].fec & (1 << phymod_fec_CL91)) {
            an_ability_get_type->an_fec |= 8;
        }
        an_ability_get_type->capabilities |= (advert_ability[index].pause) << 6;
    }
    an_ability_get_type->an_cl72=1;

    return PHYMOD_E_NONE;

}


int plp_aperta_phy_autoneg_remote_ability_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_ability_t* an_ability_get_type)
{
    return _plp_aperta_phy_autoneg_remote_ability_get(phy,an_ability_get_type);
}

int plp_aperta_phy_autoneg_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_autoneg_control_t* an)
{
    plp_aperta_phymod_autoneg_control_t an_loc;
    int speed = 0, port = 0;
    BCMI_APERTA_D_CTRL_SWGPREG05r_t gpreg5;
    BCMI_APERTA_D_SWS_SWREG_000r_t sw_an_mode;
 
    PHYMOD_MEMSET(&gpreg5, 0, sizeof(BCMI_APERTA_D_CTRL_SWGPREG05r_t));
    PHYMOD_MEMCPY(&an_loc, an, sizeof(plp_aperta_phymod_autoneg_control_t));
    PHYMOD_MEMSET(&sw_an_mode, 0, sizeof(BCMI_APERTA_D_SWS_SWREG_000r_t));
    APERTA_UPDATE_PM_INFO(phy->access.addr, phy);
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_info_speed_get(phy, &speed, NULL));

    if (speed == 25000) {
        an_loc.an_mode = phymod_AN_MODE_CL73;
        APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, port);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_CTRL_SWGPREG05r(&phy->access, &gpreg5));
        if ((gpreg5.v[0] & (3 << (port *2))) == (1 << port*2)) {
            an_loc.an_mode = phymod_AN_MODE_CL73_MSA;
        }
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_READ_SWS_SWREG_000r(&phy->access, &sw_an_mode));
        if ((sw_an_mode.v[0] & (3 << (port*2))) == (3 << (port*2))) {
            an_loc.an_mode = phymod_AN_MODE_CL73_MSA;
        }
    } else {
        an_loc.an_mode = phymod_AN_MODE_CL73;
    }
    
    if (speed == 10000 || speed == 25000 || speed == 50000) {
        if (plp_aperta_count_no_bits(phy->access.lane_mask) == 0x2) {
            an_loc.an_mode = phymod_AN_MODE_CL73_MSA;
        }
        an_loc.num_lane_adv = 1;
    } else {
        an_loc.num_lane_adv = 4;
    }
    an_loc.enable = an->enable;

    return plp_aperta_portmod_port_autoneg_set(0, (phy->access.addr << 8), 0, &an_loc);
}

int plp_aperta_phy_autoneg_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_control_t* an, uint32_t* an_done)
{
    APERTA_UPDATE_PM_INFO(phy->access.addr, phy);
    return plp_aperta_tscbh_phy_autoneg_get(phy, an, an_done);
}

int plp_aperta_phy_autoneg_status_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_status_t* status)
{
    
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_phy_init(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_init_config_t* init_config)
{
    plp_aperta_phymod_phy_init_config_t init_config_cpy;
    aperta_config_phy_t config_phy;
    BCMI_APERTA_D_GPIO_MUX_CNTRL_GPIO_INPUT_SEL_1r_t gpio_input_sel1;
    BCMI_APERTA_D_CTRL_SWGPREG1Fr_t gpreg1f;
    BCMI_APERTA_D_SWS_SWREG_002r_t  sw_reg02;
    aperta_fw_init_t *fw_init_param = (aperta_fw_init_t*)(init_config->interface.device_aux_modes);

    PHYMOD_MEMSET(&sw_reg02, 0, sizeof(sw_reg02));
    PHYMOD_MEMCPY(&init_config_cpy, init_config, sizeof(plp_aperta_phymod_phy_init_config_t));
    PHYMOD_MEMSET(&gpreg1f, 0, sizeof(BCMI_APERTA_D_CTRL_SWGPREG1Fr_t));
    if ((fw_init_param != NULL) && (fw_init_param->pll1_vco_rate == bcmplpapertaVco20p625G)) {
        PHYMOD_DEBUG_ERROR(("PLL1 cannot be 20.625G\n"));
        return PHYMOD_E_PARAM;
    }

    if ((fw_init_param != NULL) && (fw_init_param->pll1_vco_rate == bcmplpapertaVco26p562G)) { 
        init_config_cpy.interface.data_rate = 400000;
    } else if ((fw_init_param != NULL) && (fw_init_param->pll1_vco_rate == bcmplpapertaVco25p781G)) {
        init_config_cpy.interface.data_rate = 100000;
    } else {
        init_config_cpy.interface.data_rate = 400000;
    }
    init_config_cpy.interface.interface_type = phymodInterfaceLR4;

    /*Download FW and Add Enable tscbh_aperta*/
    PHYMOD_IF_ERR_RETURN(plp_aperta_port_attach(phy, &init_config_cpy));
    
    if ((fw_init_param != NULL) && (fw_init_param->macsec_static_bypass == 1)) {
        /* Put macsec bypass*/
        config_phy.MACsecOpt = 0x3;
        sw_reg02.v[0] = 1;
    } else {
        /* No macsec Bypass*/
        config_phy.MACsecOpt = 0x0;
        sw_reg02.v[0] = 0;
    }
    /* Default IO signaling should be 1.8V. 3.3V IO signaling enables open-drain.*/
    config_phy.IOOpt = 0x0;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_send_fw_msg(phy, 0, NULL,APERTA_FW_MSG_CONFIG_PHY, &config_phy, 0));

    /* To retreive macsec mode during soft reset*/
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_SWS_SWREG_002r(&phy->access, sw_reg02)); 

    /* Undo PCS*/
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_tscbh_reset_pcs(phy,init_config_cpy.interface.data_rate, 3));

    /* GPIO setting for module*/
    gpio_input_sel1.v[0] = 0x402;
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_GPIO_MUX_CNTRL_GPIO_INPUT_SEL_1r(&phy->access, gpio_input_sel1));

    BCMI_APERTA_D_CTRL_SWGPREG1Fr_SET(gpreg1f, 0xFFFF);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_CTRL_SWGPREG1Fr(&phy->access, gpreg1f));

    return PHYMOD_E_NONE;
}

static int plp_aperta_core_dload(const plp_aperta_phymod_core_access_t *core, int pass, int fw_method, const plp_aperta_phymod_core_status_t* core_status)
{
    plp_aperta_phymod_phy_init_config_t init_config1;

    PHYMOD_MEMSET(&init_config1, 0, sizeof(plp_aperta_phymod_phy_init_config_t));
    init_config1.ext_phy_tx_params_user_flag[0] = pass;
    init_config1.ext_phy_tx_params_user_flag[1] = fw_method;
    {
        const plp_aperta_phymod_phy_init_config_t *init_config = &init_config1;
        PHYMOD_IF_ERR_RETURN(plp_aperta_core_add(core, init_config, core_status));
    }

    return PHYMOD_E_NONE;

}
static int plp_aperta_eeprom_put_serboot_low(const plp_aperta_phymod_core_access_t* core,
        BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t boot_por)
{
    int retry_cnt = APERTA_MICRO_RETRY_COUNT;
    BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t boot;
    uint32_t data1 = 0;
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_t gen_ctrl1;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t msg_out;

    /* This delay is needed here because after hard reset, 
     * Boot loader set serboot busy as "0", 
     * HW needs some time to set serboot busy*/
    PHYMOD_USLEEP(12000);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(&core->access, &msg_out));
    /* Serboot Busy will not get cleared, if there is a download
       error*/
    if (APERTA_MSGOUT_HDR_ERR != msg_out.v[0]) {
        /* Wait till serboot busy is clear*/
        do {
            PHYMOD_USLEEP(1000);
            PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(&core->access, &boot));
            data1 = BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        } while((data1 != 0) && (--retry_cnt));
        if ((retry_cnt <= 0) && (data1 != 0)) {
            PHYMOD_DEBUG_ERROR(("Serboot Busy is not 0 \n"));
            return PHYMOD_E_INTERNAL;
        }
    }
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por,0);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET(boot_por, 0);
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET(boot_por, 0);
    PHYMOD_IF_ERR_RETURN(
         BCMI_APERTA_D_WRITE_MICRO_BOOT_BOOT_PORr(&core->access, boot_por));

    /* Apply soft reset*/
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_GEN_CNTRLS_GEN_CONTROL1r(&core->access, &gen_ctrl1));
    BCMI_APERTA_D_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(gen_ctrl1, 0);
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, gen_ctrl1));

    return PHYMOD_E_NONE;
}
#ifdef VIRTUAL_SIM_VAL
extern int kill_sim;
extern int skip_chip_init;
#endif

int plp_aperta_core_init(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config, const plp_aperta_phymod_core_status_t* core_status)
{
    BCMI_APERTA_D_CTRL_RESET_CTRLr_t reset_ctrl;
    BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t  bcast_enable;
    BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_t gpreg1;
    BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t boot_por;
    BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t boot;
    uint32_t data1 = 0;
    int retry_cnt = APERTA_MICRO_RETRY_COUNT;
    BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr_t msg_out;

    PHYMOD_MEMSET(&reset_ctrl, 0, sizeof(BCMI_APERTA_D_CTRL_RESET_CTRLr_t));
    PHYMOD_MEMSET(&bcast_enable, 0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t));
    PHYMOD_MEMSET(&gpreg1, 0 , sizeof(BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r_t));
    PHYMOD_MEMSET(&boot_por, 0, sizeof(BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_t));
    PHYMOD_MEMSET(&boot, 0, sizeof(BCMI_APERTA_D_GEN_CNTRLS_BOOTr_t));

    if (init_config->firmware_load_method == phymodFirmwareLoadMethodNone) {
        plp_aperta_phymod_core_firmware_info_t fw_info;
        /* This delay is needed here because after hard reset, 
         * Boot loader set serboot busy as "0", 
         * HW needs some time to set serboot busy*/
        PHYMOD_USLEEP(12000);
        do {
            PHYMOD_USLEEP(1000);
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(&core->access, &msg_out));
            if (APERTA_MSGOUT_HDR_ERR == msg_out.v[0]) {
                PHYMOD_DEBUG_INFO(("PHY-%d: Bad image header from SPIROM...\n", core->access.addr));
                break ;
            }
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(&core->access, &boot));
            data1 = BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        } while((data1 != 0) && (--retry_cnt));
        if ((retry_cnt <= 0) && (data1 != 0)) {
            PHYMOD_DEBUG_ERROR(("Serboot Busy is not 0 \n"));
            return PHYMOD_E_INTERNAL;
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_init(core, init_config, core_status));
        PHYMOD_IF_ERR_RETURN(_plp_aperta_check_fw_download_status(core, init_config->firmware_load_method));
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta_core_firmware_info_get(&core->access, &fw_info));

        PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_is_fw_dloaded_set(core->access.addr, 1));

        PHYMOD_CRIT_INFO(("PHY:0x%x FW version:0x%x\n", core->access.addr, fw_info.fw_version));

        PHYMOD_IF_ERR_RETURN(
                plp_aperta_core_dload(core, PORTMOD_PORT_ADD_F_INIT_PASS1,
                                         init_config->firmware_load_method, core_status));

        PHYMOD_IF_ERR_RETURN(
                plp_aperta_core_dload(core, PORTMOD_PORT_ADD_F_INIT_PASS2,
                                         init_config->firmware_load_method, core_status));
    }

    if (PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_GET(init_config)) {
        /* When SERBOOT is HIGH, if user uses MDIO download method
         * for DIE1 followed by DIE0, hard reset of DIE0 affects DIE1.
         * To avoid this, program ext_uc_rstb_in_frc and ext_uc_rstb_in_frcval to 1 in DIE1*/
        plp_aperta_phymod_core_access_t temp_core;
        if (((core->access.addr & 1) == 0) &&
             (init_config->firmware_load_method == phymodFirmwareLoadMethodInternal)) {  /*Doing this for Die1 alone*/
            BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_t misc_ctrl;
            BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_t pad_status;
            PHYMOD_MEMCPY(&temp_core, core, sizeof(temp_core));
            temp_core.access.addr |= 1;
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_PAD_CNTRL_SERBOOT_STATUSr(&temp_core.access, &pad_status));
            if (BCMI_APERTA_D_PAD_CNTRL_SERBOOT_STATUSr_SERBOOT_DIN_RAWf_GET(pad_status) == 1) {
                PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_READ_CTRL_MISC_CONTROL_TYPEr(&temp_core.access, &misc_ctrl));
                BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCVALf_SET(misc_ctrl, 1);
                BCMI_APERTA_D_CTRL_MISC_CONTROL_TYPEr_EXT_UC_RSTB_IN_FRCf_SET(misc_ctrl, 1);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_WRITE_CTRL_MISC_CONTROL_TYPEr(&temp_core.access, misc_ctrl));
            }
        }
        PHYMOD_IF_ERR_RETURN( _plp_aperta_core_reset_set(core, phymodResetModeHard, 0));
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodProgEEPROM) {
            PHYMOD_IF_ERR_RETURN(
                 BCMI_APERTA_D_READ_MICRO_BOOT_BOOT_PORr(&core->access, &boot_por));
            if(BCMI_APERTA_D_MICRO_BOOT_BOOT_PORr_SERBOOTf_GET(boot_por)) {
                PHYMOD_IF_ERR_RETURN(
                    plp_aperta_eeprom_put_serboot_low(core, boot_por));
            }
        }
        /* This delay is needed here because after hard reset, 
         * Boot loader set serboot busy as "0", 
         * HW needs some time to set serboot busy*/
        PHYMOD_USLEEP(12000);

        do {
            PHYMOD_USLEEP(1000);
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_MST_MSGOUTr(&core->access, &msg_out));
            if (APERTA_MSGOUT_HDR_ERR == msg_out.v[0]) {
                PHYMOD_DEBUG_INFO(("PHY-%d: Bad image header from SPIROM...\n", core->access.addr));
                break ;
            }
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_BOOTr(&core->access, &boot));
            data1 = BCMI_APERTA_D_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot);
        } while((data1 != 0) && (--retry_cnt));
        if ((retry_cnt <= 0) && (data1 != 0)) {
            PHYMOD_DEBUG_ERROR(("Serboot Busy is not 0 \n"));
            return PHYMOD_E_INTERNAL;
        }
        /* Adding this to make sure that FW init completed*/
        PHYMOD_USLEEP(5000);
        /*Initlize PM*/
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_init(core, init_config, core_status));
    } else if (PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_GET(init_config)) {
        /* Bcast enable here*/
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, &bcast_enable));
        BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET(bcast_enable, 1);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, bcast_enable));
    } else if (PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_core_dload(core, PORTMOD_PORT_ADD_F_INIT_PASS1,
                                         init_config->firmware_load_method, core_status));

        /* Bcast disable here*/
        BCMI_APERTA_D_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET(bcast_enable, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, bcast_enable));

    } else if (PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_GET(init_config)) {
        plp_aperta_phymod_core_firmware_info_t fw_info;
        PHYMOD_IF_ERR_RETURN(_plp_aperta_check_fw_download_status(core, init_config->firmware_load_method));
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta_core_firmware_info_get(&core->access, &fw_info));

        PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_is_fw_dloaded_set(core->access.addr, 1));

        PHYMOD_CRIT_INFO(("PHY:0x%x FW version:0x%x\n", core->access.addr, fw_info.fw_version));

        PHYMOD_IF_ERR_RETURN(
                plp_aperta_core_dload(core, PORTMOD_PORT_ADD_F_INIT_PASS2,
                                         init_config->firmware_load_method, core_status));
    } else if(PHYMOD_CORE_INIT_F_FW_LOAD_END_GET(init_config)){
        /* Not required for now*/
    }
    return PHYMOD_E_NONE;

}


int plp_aperta_phy_loopback_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t enable)
{
    uint32_t get_sts = 0;

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_loopback_get(phy, loopback, &get_sts));
    /* Dont call loopback set if it is already enabled or disabled*/
    if (get_sts != enable) {
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_loopback_set(phy, loopback, enable));
    }

    return PHYMOD_E_NONE;

}

int plp_aperta_phy_loopback_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t* enable)
{

    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_loopback_get(phy, loopback, enable));

    return PHYMOD_E_NONE;

}


int plp_aperta_phy_rx_pmd_locked_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* rx_pmd_locked)
{
    return plp_aperta_tscbh_phy_rx_pmd_locked_get(phy, rx_pmd_locked);

}


int plp_aperta_phy_link_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* link_status)
{

    PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_phy_link_status_get(phy, link_status));

    return PHYMOD_E_NONE;

}


int plp_aperta_phy_status_dump(const plp_aperta_phymod_phy_access_t* phy)
{
    return _plp_aperta_phy_status_dump(phy);
}


int plp_aperta_phy_reg_read(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t* val)
{
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(phy, reg_addr, val));

    return PHYMOD_E_NONE;

}


int plp_aperta_phy_reg_write(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t val)
{
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_write(phy, reg_addr, val));

    return PHYMOD_E_NONE;

}


int plp_aperta_phy_rev_id(const plp_aperta_phymod_phy_access_t* phy, uint32_t* rev_id)
{

    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, rev_id));

    return PHYMOD_E_NONE;

}

/*! \brief Enable interupt
 *  This API is used to enable/disable specified interrupt and also
 *  enables/disables HW notification
 *
 *  @param phy         Represents PHY access\n
 *  @param intr_type_enable      Specifies interrupt type\n
 *                      aperta supports following interrupt type
 *                      1 : link down interrupt
*/
int plp_aperta_phy_intr_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t intr_type_enable)
{
#ifdef PHYMOD_TIMESYNC_SUPPORT
    if ( intr_type_enable & APERTA_INTR_LEVEL1_TIMESYNC ) {    /* IEEE-1588 interrupts */
        return _plp_aperta_timesync_phy_intr_enable_set(phy, intr_type_enable);
    }
#endif
    return _plp_aperta_phy_intr_enable_set(phy, intr_type_enable);

}

int plp_aperta_phy_intr_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* enable)
{
#ifdef PHYMOD_TIMESYNC_SUPPORT
    if ( *enable & APERTA_INTR_LEVEL1_TIMESYNC ) {             /* IEEE-1588 interrupts */
        return _plp_aperta_timesync_phy_intr_enable_get(phy, enable);
    }
#endif
    return _plp_aperta_phy_intr_enable_get(phy, enable);
}


int plp_aperta_phy_intr_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* intr_status)
{
#ifdef PHYMOD_TIMESYNC_SUPPORT
    if ( *intr_status & APERTA_INTR_LEVEL1_TIMESYNC ) {        /* IEEE-1588 interrupts */
        return _plp_aperta_timesync_phy_intr_status_get(phy, intr_status);
    }
#endif
    return _plp_aperta_phy_intr_status_get(phy, intr_status);
}


int plp_aperta_phy_intr_status_clear(const plp_aperta_phymod_phy_access_t* phy, uint32_t intr_clr)
{
    return _plp_aperta_phy_intr_status_clear(phy, intr_clr);
}


int plp_aperta_phy_i2c_read(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t addr, uint32_t offset, uint32_t size, uint8_t* data)
{
    return plp_aperta_module_read(phy, addr, offset, size, data);
}


int plp_aperta_phy_i2c_write(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t addr, uint32_t offset, uint32_t size, const uint8_t* data)
{
    return plp_aperta_module_write(phy, addr, offset, size, data);
}

/*! \brief Config set for GPIO pins
 *  Config set the GPIO pins
 *  This API is used to set the GPIO pins pull up or pull down
 *  This is generic API provided to user to configure GPIOs based on user need
 *  User has to just Pass Register address and direction & Pull function
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param pin_no   GPIO pin number (0-4)
 *  @param gpio_mode   Configuration direction
 *
 * <table>
 * <caption id="multi_row">I/O pin control mapping table</caption>
 * <tr><th>CHIP I/O Name                 <th>API pin map number
 * </tr>
 * <tr>
 * <td> GPIO_0 </td>
 * <td> gpio_pin_number = 0 </td>
 * </tr>
 * <tr>
 * <td> GPIO_1 </td>
 * <td> gpio_pin_number = 1 </td>
 * </tr>
 * <tr>
 * <td> GPIO_2 </td>
 * <td> gpio_pin_number = 2 </td>
 * </tr>
 * <tr>
 * <td> GPIO_3  </td>
 * <td> gpio_pin_number = 3 </td>
 * </tr>
 * <tr>
 * <td> GPIO_4  </td>
 * <td> gpio_pin_number = 4 </td>
 * </tr>
 * <tr>
 * <td> MOD_ABSENT </td>
 * <td> gpio_pin_number = 5 </td>
 * </tr>
 * <tr>
 * <td> MOD_RXLOS </td>
 * <td> gpio_pin_number = 6 </td>
 * </tr>
 * </table>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int plp_aperta_phy_gpio_config_set(const plp_aperta_phymod_phy_access_t* phy, int pin_no, plp_aperta_phymod_gpio_mode_t gpio_mode)
{
    return _plp_aperta_phy_gpio_config_set(phy, pin_no, gpio_mode);
}

int plp_aperta_phy_gpio_config_get(const plp_aperta_phymod_phy_access_t* phy, int pin_no, plp_aperta_phymod_gpio_mode_t* gpio_mode)
{
    return _plp_aperta_phy_gpio_config_get(phy, pin_no, gpio_mode);
}

/*! \brief Config set for GPIO pins
 *  Config set the GPIO pins
 *  This API is used to set the GPIO pins pull up or pull down
 *  This is generic API provided to user to configure GPIOs based on user need
 *  User has to just Pass Register address and direction & Pull function
 *
 *  @param phy_info <pre> Represents PHY access\n
 *  @param pin_no   GPIO pin number (0-4)
 *  @param value       Configure the pin value
 *                         1 -  High driven on Pin in o/p mode
 *                         0 -  Low driven on Pin in o/p mode </pre>
 *
 * <table>
 * <caption id="multi_row">I/O pin control mapping table</caption>
 * <tr><th>CHIP I/O Name                 <th>API pin map number
 * </tr>
 * <tr>
 * <td> GPIO_0 </td>
 * <td> gpio_pin_number = 0 </td>
 * </tr>
 * <tr>
 * <td> GPIO_1 </td>
 * <td> gpio_pin_number = 1 </td>
 * </tr>
 * <tr>
 * <td> GPIO_2 </td>
 * <td> gpio_pin_number = 2 </td>
 * </tr>
 * <tr>
 * <td> GPIO_3  </td>
 * <td> gpio_pin_number = 3 </td>
 * </tr>
 * <tr>
 * <td> GPIO_4  </td>
 * <td> gpio_pin_number = 4 </td>
 * </tr>
 * <tr>
 * <td> MOD_ABSENT </td>
 * <td> gpio_pin_number = 5 </td>
 * </tr>
 * <tr>
 * <td> MOD_RXLOS </td>
 * <td> gpio_pin_number = 6 </td>
 * </tr>
 * </table>
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int plp_aperta_phy_gpio_pin_value_set(const plp_aperta_phymod_phy_access_t* phy, int pin_no, int value)
{
    return _plp_aperta_phy_gpio_pin_value_set(phy, pin_no, value);
}

int plp_aperta_phy_gpio_pin_value_get(const plp_aperta_phymod_phy_access_t* phy, int pin_no, int* value)
{
    return _plp_aperta_phy_gpio_pin_value_get(phy, pin_no, value);
}


/* IEEE-1588 PTP TimeSync */

#ifdef PHYMOD_TIMESYNC_SUPPORT /*---------------------------------------------------------*/

#define  APERTA_GPIO_INPUT_SEL_4                0x01008adc
#define  APERTA_GPIO_INPUT_SEL_4_HARDCODE       0x0E03
#define  APERTA_GPIO_INPUT_SEL_4_RESET          0x1010
#define  APERTA_GPIO_INPUT_SEL_5                0x01008add
#define  APERTA_GPIO_INPUT_SEL_5_HARDCODE       0x300F
#define  APERTA_GPIO_INPUT_SEL_5_RESET          0x3010
#define  APERTA_GPIO_14_CONTROL_1               0x01008a99
#define  APERTA_GPIO_14_CONTROL_1_HARDCODE      0x0003
#define  APERTA_GPIO_14_CONTROL_1_RESET         0x0103
#define  APERTA_GPIO_15_CONTROL_1               0x01008a9d
#define  APERTA_GPIO_15_CONTROL_1_HARDCODE      0x0003
#define  APERTA_GPIO_15_CONTROL_1_RESET         0x0000

/* New for Aperta:  Only since GPIO is muxed with SyncIn/Out */
int _plp_aperta_ptp_sync_enable(const plp_aperta_phymod_phy_access_t* phy, int enable) {
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_write(phy, APERTA_GPIO_INPUT_SEL_4 ,
                         (enable) ? APERTA_GPIO_INPUT_SEL_4_HARDCODE
                                  : APERTA_GPIO_INPUT_SEL_4_RESET   ) );
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_write(phy, APERTA_GPIO_INPUT_SEL_5 ,
                         (enable) ? APERTA_GPIO_INPUT_SEL_5_HARDCODE
                                  : APERTA_GPIO_INPUT_SEL_5_RESET   ) );
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_write(phy, APERTA_GPIO_14_CONTROL_1,
                         (enable) ? APERTA_GPIO_14_CONTROL_1_HARDCODE
                                  : APERTA_GPIO_14_CONTROL_1_RESET  ) );
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_write(phy, APERTA_GPIO_15_CONTROL_1,
                         (enable) ? APERTA_GPIO_15_CONTROL_1_HARDCODE
                                  : APERTA_GPIO_15_CONTROL_1_RESET  ) );
    return PHYMOD_E_NONE;
}

int plp_aperta_timesync_config_set(const plp_aperta_phymod_phy_access_t* phy,
                               const plp_aperta_phymod_timesync_config_t* config)
{
    uint32_t flags=0, port = 0, unused = 0 ,rev_id = 0;
    plp_aperta_phymod_phy_inf_config_t port_config;
    aperta_ptp_config_t ptp_cfg;
    aperta_device_aux_modes_t auxmode;
    uint8_t port_option = 0;
    int rv = 0;
    plp_aperta_phymod_phy_access_t fw_access;
    unsigned int is_fo_enabled = 0, primary_lm = 0;

    PHYMOD_MEMSET(&port_config, 0, sizeof(plp_aperta_phymod_phy_inf_config_t));
    PHYMOD_MEMSET(&ptp_cfg, 0, sizeof(ptp_cfg));
    PHYMOD_MEMCPY(&fw_access, phy, sizeof(fw_access));
    port_config.device_aux_modes = &auxmode;

    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_interface_config_get(phy, flags, &port_config));
    if (phy->access.lane_mask == auxmode.failover_config.lane_map) {
        /* Fo lane map: use Primary lanemap to get port*/
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_fo_enabled(phy, &port_config, &is_fo_enabled,
                    &port, &primary_lm));
        (void)is_fo_enabled;
        fw_access.access.lane_mask = primary_lm;
    } else {
        /* Use Primary lanemap to get port*/
        APERTA_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, unused);
    }
    PHYMOD_DIAG_OUT(("ts_config_Set:port:%d DataR:%d ldr:%d lanemask:%x failover:%x \n",port,  port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, auxmode.failover_config.lane_map));
    ptp_cfg.PortNum = port;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_fw_config_ptp_read (&fw_access, &ptp_cfg));
    if ((config->flags >> APERTA_TS_MODE_FLAG_SHIFT) & 0x1FF) {
        port_option = plp_aperta_log2n((config->flags >> APERTA_TS_MODE_FLAG_SHIFT) & 0x1FF);
        if (rev_id == APERTA_REV_B0) { 
            /* Timestamp config bit 3 needs to be stored in bit 3 
             * of port option and Bit 0-2 will be updated in bit 4-6 of port options*/
            if (port_option >= 5) { /* Move the value to B0 rev*/
                port_option += 3;
                if (!(ptp_cfg.PortOptions & 0x80)) {
                    PHYMOD_DEBUG_ERROR(("It is required to enable fixed latency, Use bcm_plp_aperta_update_port_config API\n"));
                    return PHYMOD_E_PARAM;
                }
            }
            ptp_cfg.PortOptions &= ~(0xF);
            ptp_cfg.PortOptions |= (port_option & 0x7);
            ptp_cfg.PortOptions |= (port_option & 0x8);
        } else {
            ptp_cfg.PortOptions &= ~(0xF);
            ptp_cfg.PortOptions |= ((port_option) & 0xF);
        }
    }
    /* Pause port */
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode,
                   APERTA_FW_MSG_PAUSE_PORT, NULL, 0));

    /* Flush port */
    rv = plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode,
                   APERTA_FW_MSG_FLUSH_PORT, NULL, 0);
    APERTA_TS_PORT_RESUME(rv, "FLUSH", port);

    if ((config->flags >> APERTA_TS_MODE_FLAG_SHIFT) & 0x1FF) {
        rv = plp_aperta_fw_config_ptp_write (&fw_access, &ptp_cfg);
        APERTA_TS_PORT_RESUME(rv, "CONFIG_PTP", port);
    }
    (void) unused;
    
    rv =  _plp_aperta_ptp_sync_enable(phy, TRUE);
    APERTA_TS_PORT_RESUME(rv, "_plp_aperta_ptp_sync_enable", port);
    rv = _plp_aperta_timesync_config_set(phy, config);
    APERTA_TS_PORT_RESUME(rv, "_plp_aperta_timesync_config_set", port);

    return plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode,
                   APERTA_FW_MSG_RESUME_PORT, NULL, 0);

}

int plp_aperta_timesync_config_get(const plp_aperta_phymod_phy_access_t* phy,
                                     plp_aperta_phymod_timesync_config_t* config)
{
    uint32_t flags=0, port = 0, unused = 0, rev_id = 0;
    plp_aperta_phymod_phy_inf_config_t port_config;
    aperta_ptp_config_t ptp_cfg;
    aperta_device_aux_modes_t auxmode;
    uint8_t port_option = 0;
    plp_aperta_phymod_phy_access_t fw_access;
    unsigned int is_fo_enabled = 0, primary_lm = 0;

    PHYMOD_MEMSET(&port_config, 0, sizeof(plp_aperta_phymod_phy_inf_config_t));
    PHYMOD_MEMSET(&ptp_cfg, 0, sizeof(ptp_cfg));
    port_config.device_aux_modes = &auxmode;

    PHYMOD_MEMCPY(&fw_access, phy, sizeof(fw_access));
    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_interface_config_get(phy, flags, &port_config));
    if (phy->access.lane_mask == auxmode.failover_config.lane_map) {
        /* Fo lane map: use Primary lanemap to get port*/
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_fo_enabled(phy, &port_config, &is_fo_enabled,
                    &port, &primary_lm));
        (void)is_fo_enabled;
        fw_access.access.lane_mask = primary_lm;
    } else {
        /* Use Primary lanemap to get port*/
        APERTA_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, unused);
    }

    PHYMOD_DIAG_OUT(("ts_config_get:port:%d DataR:%d ldr:%d lanemask:%x failover:%x \n",port, port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, auxmode.failover_config.lane_map));
    APERTA_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, unused);
    ptp_cfg.PortNum = port;
    PHYMOD_IF_ERR_RETURN( _plp_aperta_timesync_config_get(phy, config));
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_fw_config_ptp_read (&fw_access, &ptp_cfg));

    port_option = (ptp_cfg.PortOptions & 0x7);
    if (rev_id == APERTA_REV_B0) {
        port_option |= (ptp_cfg.PortOptions & 8);
    }
    if (port_option >= 8 ) {
        config->flags |= (1 << ((port_option-3) + APERTA_TS_MODE_FLAG_SHIFT));
    } else if ((port_option >= 1) && (port_option <=4)) {
        config->flags |= (1 << ((port_option) + APERTA_TS_MODE_FLAG_SHIFT));
    } else { /* Disable Bit*/
        config->flags |= (1 << APERTA_TS_MODE_FLAG_SHIFT);
    }
    (void)unused;
    return PHYMOD_E_NONE;
}

int plp_aperta_timesync_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable)
{
    PHYMOD_IF_ERR_RETURN( _plp_aperta_ptp_sync_enable(phy, enable) );
    return _plp_aperta_timesync_enable_set(phy, flags, enable);
}

int plp_aperta_timesync_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable)
{
    return _plp_aperta_timesync_enable_get(phy, flags, enable);
}

int plp_aperta_timesync_nco_addend_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t freq_step)
{
    return _plp_aperta_timesync_nco_addend_set(phy, freq_step);
}

int plp_aperta_timesync_nco_addend_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* freq_step)
{
    return _plp_aperta_timesync_nco_addend_get(phy, freq_step);
}

int plp_aperta_timesync_framesync_mode_set(const plp_aperta_phymod_phy_access_t* phy,
                                       const plp_aperta_phymod_timesync_framesync_t* framesync)
{
    return _plp_aperta_timesync_framesync_mode_set(phy, framesync);
}

int plp_aperta_timesync_framesync_mode_get(const plp_aperta_phymod_phy_access_t* phy,
                                             plp_aperta_phymod_timesync_framesync_t* framesync)
{
    return _plp_aperta_timesync_framesync_mode_get(phy, framesync);
}

int plp_aperta_timesync_local_time_set(const plp_aperta_phymod_phy_access_t* phy, uint64_t local_time)
{
    return _plp_aperta_timesync_local_time_set(phy, local_time);
}

int plp_aperta_timesync_local_time_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* local_time)
{
    return _plp_aperta_timesync_local_time_get(phy, local_time);
}

int plp_aperta_timesync_load_ctrl_set(const plp_aperta_phymod_phy_access_t* phy,
                                  uint32_t load_once, uint32_t load_always)
{
    return _plp_aperta_timesync_load_ctrl_set(phy, load_once, load_always);
}

int plp_aperta_timesync_load_ctrl_get(const plp_aperta_phymod_phy_access_t* phy,
                                  uint32_t* load_once, uint32_t* load_always)
{
    return _plp_aperta_timesync_load_ctrl_get(phy, load_once, load_always);
}

int plp_aperta_timesync_timing_control_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t regaddr, 
                                       uint64_t value64, uint64_t value_hi, 
                                       uint64_t dpll_value, uint64_t dpll_value_hi)
{
    int index = 0, flags = 0, unused = 0;
    aperta_ptp_tod_t ptp_tod;
    plp_aperta_phymod_phy_access_t fw_access;
    plp_aperta_phymod_phy_inf_config_t port_config;
    aperta_device_aux_modes_t auxmode;
    unsigned int is_fo_enabled = 0, primary_lm = 0, port = 0;

    PHYMOD_MEMSET(&port_config, 0, sizeof(plp_aperta_phymod_phy_inf_config_t));
    PHYMOD_MEMCPY(&fw_access, phy, sizeof(fw_access));
    PHYMOD_MEMSET(&ptp_tod, 0, sizeof(aperta_ptp_tod_t));
    port_config.device_aux_modes = &auxmode;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_interface_config_get(phy, flags, &port_config));
    if (phy->access.lane_mask == auxmode.failover_config.lane_map) {
        /* Fo lane map: use Primary lanemap to get port*/
        PHYMOD_IF_ERR_RETURN(plp_aperta_pm_is_fo_enabled(phy, &port_config, &is_fo_enabled,
                    &port, &primary_lm));
        (void)is_fo_enabled;
        fw_access.access.lane_mask = primary_lm;
    } else {
        /* Use Primary lanemap to get port*/
        APERTA_GET_PORT_FROM_LM_SP(port_config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port, unused);
    }

    (void) unused;
    if (regaddr == bcmplpTimesyncTimerTypeTimeOfDayTSDpll80) {
        ptp_tod.PortNum = port;
        APERTA_TOD_SET (ptp_tod.TOD_TS, 10, value64, value_hi);
        if (dpll_value == 0 && dpll_value_hi == 0) {
            PHYMOD_MEMCPY(ptp_tod.TOD_DPLL, ptp_tod.TOD_TS, sizeof(ptp_tod.TOD_TS));
        } else {
            APERTA_TOD_SET (ptp_tod.TOD_DPLL, 10, dpll_value, dpll_value_hi);
        }
        return plp_aperta_fw_tod_config(&fw_access, APERTA_PTP_TOD_TYPE_80BIT , &ptp_tod);
    }  else if (regaddr == bcmplpTimesyncTimerTypeTimeOfDayTSDpll48) {
        ptp_tod.PortNum = port;
        APERTA_TOD_SET (ptp_tod.TOD_TS, 6, value64, value_hi);
        if (dpll_value == 0 && dpll_value_hi == 0) {
            PHYMOD_MEMCPY(ptp_tod.TOD_DPLL, ptp_tod.TOD_TS, (sizeof(ptp_tod.TOD_TS[0])*6));
        } else {
            APERTA_TOD_SET (ptp_tod.TOD_DPLL, 6, dpll_value, dpll_value_hi);
        }
        return plp_aperta_fw_tod_config(&fw_access, APERTA_PTP_TOD_TYPE_48BIT , &ptp_tod);
    } else {
        return _plp_aperta_timesync_timing_control_set(phy, regaddr, value64);
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_timesync_timing_control_get(const plp_aperta_phymod_phy_access_t* phy,
                                       uint32_t regaddr, uint64_t *value64,
                                       uint64_t* value_hi, uint64_t* dpll_value, uint64_t* dpll_value_hi)
{
    if ((regaddr == bcmplpTimesyncTimerTypeTimeOfDayTSDpll80) || (regaddr == bcmplpTimesyncTimerTypeTimeOfDayTSDpll48)) {
        PHYMOD_DEBUG_ERROR(("TOD get not supported for bcmplpTimesyncTimerTypeTimeOfDayTSDpll80 and \
                              bcmplpTimesyncTimerTypeTimeOfDayTSDpll48\n"));
        return PHYMOD_E_PARAM;
    } else {
        return _plp_aperta_timesync_timing_control_get(phy, regaddr, value64);
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_timesync_link_delay_set(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                        uint32_t  operator, uint32_t  msg_type, uint64_t  linkdelay)
{
    return _plp_aperta_timesync_link_delay_set(phy, operator, msg_type, COMPILER_64_LO(linkdelay));
}

int plp_aperta_timesync_link_delay_get(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                        uint32_t *operator, uint32_t *msg_type, uint64_t *linkdelay)
{
    uint32_t  ldelay32 = 0;
    PHYMOD_IF_ERR_RETURN( _plp_aperta_timesync_link_delay_get(phy, operator, msg_type, &ldelay32) );

    COMPILER_64_SET(*linkdelay, 0, ldelay32);
    return PHYMOD_E_NONE;
}

int plp_aperta_timesync_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                        uint32_t  operator, uint32_t  msg_type, uint64_t  ts_offset)
{
    return _plp_aperta_timesync_timestamp_offset_set(phy, txrx, operator, msg_type, COMPILER_64_LO(ts_offset));
}

int plp_aperta_timesync_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                        uint32_t *operator, uint32_t *msg_type, uint64_t *ts_offset)
{
    uint32_t  tsoffset32 = 0;
    PHYMOD_IF_ERR_RETURN( _plp_aperta_timesync_timestamp_offset_get(phy, txrx,operator,
                                                         msg_type, &tsoffset32) );
    COMPILER_64_SET(*ts_offset, 0, tsoffset32);
    return PHYMOD_E_NONE;
}

int plp_aperta_timesync_time_code_set(const plp_aperta_phymod_phy_access_t  *phy, uint32_t flags,
                                  const plp_aperta_phymod_timesync_timespec_t *timecode)
{
    return _plp_aperta_timesync_time_code_set(phy, timecode);
}

int plp_aperta_timesync_time_code_get(const plp_aperta_phymod_phy_access_t  *phy, uint32_t flags,
                                  plp_aperta_phymod_timesync_timespec_t *timecode)
{
    return _plp_aperta_timesync_time_code_get(phy, flags, timecode);
}

int plp_aperta_timesync_sopmem_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t flags,
                                 int index, phymod_timesync_sopmem_t  *sopmem)
{
    return _plp_aperta_timesync_sopmem_get(phy, flags, index, sopmem);
}

int plp_aperta_timesync_capture_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* cap_ts)
{
    return _plp_aperta_timesync_capture_timestamp_get(phy, cap_ts);
}

int plp_aperta_timesync_heartbeat_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* hb_ts)
{
    return _plp_aperta_timesync_heartbeat_timestamp_get(phy, hb_ts);
}

int plp_aperta_timesync_mpls_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                               const plp_aperta_phymod_timesync_mpls_ctrl_t *config)
{
    return _plp_aperta_timesync_mpls_set(phy, flags, config);
}

int plp_aperta_timesync_mpls_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                               plp_aperta_phymod_timesync_mpls_ctrl_t *config)
{
    return _plp_aperta_timesync_mpls_get(phy, flags, config);
}

int plp_aperta_timesync_inband_filter_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                                      int index, phymod_timesync_inband_filter_ctrl_t *config)
{
    return _plp_aperta_timesync_inband_filter_set(phy, flags, index, config);
}
int plp_aperta_timesync_inband_filter_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                                      int index, phymod_timesync_inband_filter_ctrl_t *config)
{
    return _plp_aperta_timesync_inband_filter_get(phy, flags, index, config);
}


#else  /* ! PHYMOD_TIMESYNC_SUPPORT */ /*---------------------------------------------------------*/

int plp_aperta_timesync_config_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_timesync_config_t* config)
{
    return _aperta_timesync_config_set(phy, config);

}

int plp_aperta_timesync_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_timesync_config_t* config)
{
    return _aperta_timesync_config_get(phy, config);
}


int plp_aperta_timesync_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable)
{

    return _aperta_timesync_enable_set(phy, flags, enable);
}

int plp_aperta_timesync_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable)
{
    return _aperta_timesync_enable_get(phy, flags, enable);

}


int plp_aperta_timesync_nco_addend_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t freq_step)
{

    return _aperta_timesync_nco_addend_set(phy, freq_step);

}

int plp_aperta_timesync_nco_addend_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* freq_step)
{

    return _aperta_timesync_nco_addend_get(phy, freq_step);

}


int plp_aperta_timesync_framesync_mode_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_timesync_framesync_t* framesync)
{

    return _aperta_timesync_framesync_mode_set(phy, framesync);

}

int plp_aperta_timesync_framesync_mode_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_timesync_framesync_t* framesync)
{

    return _aperta_timesync_framesync_mode_get(phy, framesync);

}


int plp_aperta_timesync_local_time_set(const plp_aperta_phymod_phy_access_t* phy, uint64_t local_time)
{

    return _aperta_timesync_local_time_set(phy, local_time);

}

int plp_aperta_timesync_local_time_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* local_time)
{

    return _aperta_timesync_local_time_get(phy, local_time);

}


int plp_aperta_timesync_load_ctrl_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t load_once, uint32_t load_always)
{

    return _aperta_timesync_load_ctrl_set(phy, load_once, load_always);

}

int plp_aperta_timesync_load_ctrl_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* load_once, uint32_t* load_always)
{

    return _aperta_timesync_load_ctrl_get(phy, load_once, load_always);

}


int aperta_timesync_tx_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t ts_offset)
{

    return _aperta_timesync_tx_timestamp_offset_set(phy, ts_offset);

}

int aperta_timesync_tx_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* ts_offset)
{

    return _aperta_timesync_tx_timestamp_offset_get(phy, ts_offset);

}


int aperta_timesync_rx_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t ts_offset)
{

    return _aperta_timesync_rx_timestamp_offset_set(phy, ts_offset);

}

int aperta_timesync_rx_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* ts_offset)
{

    return _aperta_timesync_rx_timestamp_offset_get(phy, ts_offset);
}


int plp_aperta_timesync_capture_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* cap_ts)
{

    return _aperta_timesync_capture_timestamp_get(phy, cap_ts);

}


int plp_aperta_timesync_heartbeat_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* hb_ts)
{

    return _aperta_timesync_heartbeat_timestamp_get(phy, hb_ts);

}
int plp_aperta_timesync_timing_control_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t regaddr, 
                                       uint64_t value64, uint64_t value_hi, 
                                       uint64_t dpll_value, uint64_t dpll_value_hi)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_timing_control_get(const plp_aperta_phymod_phy_access_t* phy,
                                       uint32_t regaddr, uint64_t *value64,
                                       uint64_t* value_hi, uint64_t* dpll_value, uint64_t* dpll_value_hi)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_link_delay_set(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                        uint32_t  operator, uint32_t  msg_type, uint64_t  linkdelay)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_link_delay_get(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                        uint32_t *operator, uint32_t *msg_type, uint64_t *linkdelay)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                        uint32_t  operator, uint32_t  msg_type, uint64_t  ts_offset)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                        uint32_t *operator, uint32_t *msg_type, uint64_t *ts_offset)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_time_code_set(const plp_aperta_phymod_phy_access_t  *phy, uint32_t flags,
                                  const plp_aperta_phymod_timesync_timespec_t *timecode)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_time_code_get(const plp_aperta_phymod_phy_access_t  *phy, uint32_t flags,
                                        plp_aperta_phymod_timesync_timespec_t *timecode)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_sopmem_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t flags,
                                 int index, phymod_timesync_sopmem_t  *sopmem)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_mpls_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                              bcm_plp_timesync_mpls_ctrl_t *config)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_mpls_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                              bcm_plp_timesync_mpls_ctrl_t *config)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_timesync_inband_filter_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                                      int index, phymod_timesync_inband_filter_ctrl_t *config)
{
    return PHYMOD_E_UNAVAIL;
}
int plp_aperta_timesync_inband_filter_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                                      int index, phymod_timesync_inband_filter_ctrl_t *config)
{
    return PHYMOD_E_UNAVAIL;
}


#endif /* PHYMOD_TIMESYNC_SUPPORT */ /*---------------------------------------------------------*/

int plp_aperta_core_lane_map_set(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_lane_map_t* lane_map)
{
    plp_aperta_phymod_lane_map_t loc_lane_map;
    unsigned int lane_index = 0;
    PHYMOD_MEMSET(&loc_lane_map, 0, sizeof(loc_lane_map));
    loc_lane_map.num_of_lanes = lane_map->num_of_lanes;

    for (lane_index = 0; lane_index<APERTA_MAX_LANES; lane_index++) {
        if ( lane_map->lane_map_rx[lane_index] <=3) {
            loc_lane_map.lane_map_rx[lane_index] = 3 - lane_map->lane_map_rx[lane_index];
        } else {
            loc_lane_map.lane_map_rx[lane_index] = lane_map->lane_map_rx[lane_index];
        }

        if ( lane_map->lane_map_tx[lane_index] <=3) {
            loc_lane_map.lane_map_tx[lane_index] = 3 - lane_map->lane_map_tx[lane_index];
        } else {
            loc_lane_map.lane_map_tx[lane_index] = lane_map->lane_map_tx[lane_index];
        }
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_core_lane_map_set(core, &loc_lane_map));
    return PHYMOD_E_NONE;
}

int plp_aperta_core_lane_map_get(const plp_aperta_phymod_core_access_t* core, plp_aperta_phymod_lane_map_t* lane_map)
{
    plp_aperta_phymod_lane_map_t loc_lane_map;
    unsigned int lane_index = 0;

    PHYMOD_MEMSET(&loc_lane_map, 0, sizeof(loc_lane_map));
    PHYMOD_IF_ERR_RETURN(plp_aperta_tscbh_core_lane_map_get(core, &loc_lane_map));

    /*Compensating the default laneswap*/
    for (lane_index = 0; lane_index<APERTA_MAX_LANES; lane_index++) {
        if (loc_lane_map.lane_map_rx[lane_index] <=3) {
            lane_map->lane_map_rx[lane_index] = 3 - loc_lane_map.lane_map_rx[lane_index];
        } else {
            lane_map->lane_map_rx[lane_index] = loc_lane_map.lane_map_rx[lane_index];
        }
        if (loc_lane_map.lane_map_tx[lane_index] <=3) {
            lane_map->lane_map_tx[lane_index] = 3 - loc_lane_map.lane_map_tx[lane_index];
        } else {
            lane_map->lane_map_tx[lane_index] = loc_lane_map.lane_map_tx[lane_index];
        }
    }
    lane_map->num_of_lanes = loc_lane_map.num_of_lanes;
    return PHYMOD_E_NONE;
}

int plp_aperta_synce_config_set(const plp_aperta_phymod_phy_access_t* phy, const phymod_synce_cfg_t* synce_cfg)
{
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_synce_config_set(phy, synce_cfg));
    return PHYMOD_E_NONE;
}

int plp_aperta_synce_config_get(const plp_aperta_phymod_phy_access_t* phy, phymod_synce_cfg_t* synce_cfg)
{
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_synce_config_get(phy, synce_cfg));
    return PHYMOD_E_NONE;
}
#define APERTA_PORT_RET_NOERR   1
int plp_aperta_failover_mode_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_failover_mode_t failover_mode)
{
#if APERTA_PORT_RET_NOERR
    int port_number = 0;
    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask,port_number);
#else
    int port_number = 0, unused;
    plp_aperta_phymod_phy_inf_config_t config;
    aperta_device_aux_modes_t auxmode;
    plp_aperta_phymod_phy_access_t phy_loc;

    config.device_aux_modes = &auxmode;
    PHYMOD_MEMCPY(&phy_loc, phy, sizeof(plp_aperta_phymod_phy_access_t));

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_phy_interface_config_get(&phy_loc, 0/*Flags*/, 0 /*156.25 MHz*/, &config));

    APERTA_GET_PORT_FROM_LM_SP(config.data_rate, auxmode.lane_data_rate, phy->access.lane_mask, port_number, unused);
    (void) unused;
#endif    
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_fw_switch_mux (phy, port_number));

    return PHYMOD_E_NONE;
}

int plp_aperta_failover_mode_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_failover_mode_t* failover_mode)
{
    return PHYMOD_E_UNAVAIL;
}

int plp_aperta_phy_pai_info_get(const plp_aperta_phymod_phy_access_t* phy, phymod_pai_phy_op_t operation, phymod_pai_phy_config_t* pai_phy_config, void* epdm_data)
{
    int no_of_lanes = 0, ldr = 0;
    if (pai_phy_config->config_type == phymodConfigTypeFw_init) {
        phymod_pai_fw_init_config_t *pai_fw_init_config = (phymod_pai_fw_init_config_t*)(pai_phy_config->config_data);
        aperta_fw_init_t *phy_fw_init_param = (aperta_fw_init_t*)(epdm_data);
        if (operation != phymodOperationPai_to_phy) { 
            return PHYMOD_E_NONE;
        }
        phy_fw_init_param->macsec_static_bypass = pai_fw_init_config->macsec_static_bypass;
        phy_fw_init_param->pll1_vco_rate = pai_fw_init_config->pll1_vco;
        phy_fw_init_param->tx_drv_supply = pai_fw_init_config->tx_drv_supply;
    } else if (pai_phy_config->config_type == phymodConfigTypePort_config) {
        phymod_pai_port_config_t *port_cfg = (phymod_pai_port_config_t*)(pai_phy_config->config_data);
        aperta_device_aux_modes_t *aperta_aux_mode = (aperta_device_aux_modes_t*) epdm_data;
        if (operation == phymodOperationPai_to_phy) { 
            PHYMOD_MEMSET(aperta_aux_mode, 0, sizeof(aperta_device_aux_modes_t));
            no_of_lanes = plp_aperta_phymod_count_set_bits(phy->access.lane_mask);
            ldr = (port_cfg->speed/no_of_lanes);
            aperta_aux_mode->failover_config.lane_map = port_cfg->failover_lane_map;
            aperta_aux_mode->failover_config.mux_location = 0;
            if (ldr == APERTA_SPEED_10G) {
                aperta_aux_mode->lane_data_rate = bcmplpApertaLaneDataRate_10P3125G; 
                aperta_aux_mode->modulation_mode = bcmplpApertaModulationNRZ; 
            } else if (ldr == APERTA_SPEED_20G) {
                aperta_aux_mode->lane_data_rate = bcmplpApertaLaneDataRate_20P625G;
                aperta_aux_mode->modulation_mode = bcmplpApertaModulationNRZ; 
            } else if (ldr == APERTA_SPEED_25G) {
                aperta_aux_mode->lane_data_rate = bcmplpApertaLaneDataRate_25P78125G;
                aperta_aux_mode->modulation_mode = bcmplpApertaModulationNRZ; 
            } else if (ldr == APERTA_SPEED_50G) {
                aperta_aux_mode->lane_data_rate = bcmplpApertaLaneDataRate_53P125G; 
                aperta_aux_mode->modulation_mode = bcmplpApertaModulationPAM4; 
            } else {
                PHYMOD_DEBUG_ERROR(("Invalid lane data rate : %d number of lane:%d speed:%d\n", 
                            ldr,no_of_lanes, port_cfg->speed));
                return PHYMOD_E_PARAM;
            }
            if ((port_cfg->fec_mode >> 8) & 0xFF) {
                if ((port_cfg->fec_mode & 0xFF) == phymodPaiFecModeExtendedRS528)  {
                    aperta_aux_mode->fec_mode_sel =  bcmplpapertaRSFEC;
                } else if ((port_cfg->fec_mode & 0xFF) == phymodPaiFecModeExtendedRS544) {
                    aperta_aux_mode->fec_mode_sel =  bcmplpapertaRS544;
                } else if ((port_cfg->fec_mode & 0xFF) == phymodPaiFecModeExtendedRS544_INTERLEAVED) {
                    aperta_aux_mode->fec_mode_sel =  bcmplpapertaRS544_2XN;
                } else if ((port_cfg->fec_mode & 0xFF) == phymodPaiFecModeExtendedFC) {
                    aperta_aux_mode->fec_mode_sel =  bcmplpapertaBaseR;
                } else {
                    aperta_aux_mode->fec_mode_sel =  bcmplpapertaNoFEC;
                }
            } else {
                if (port_cfg->fec_mode == phymodPaiFecModeRS)  {
                    if (port_cfg->speed == APERTA_SPEED_400G) {
                        aperta_aux_mode->fec_mode_sel =  bcmplpapertaRS544_2XN;
                    } else if (port_cfg->speed == APERTA_SPEED_200G) {
                        aperta_aux_mode->fec_mode_sel =  bcmplpapertaRS544;
                    } else if (port_cfg->speed == APERTA_SPEED_25G) {
                        aperta_aux_mode->fec_mode_sel =  bcmplpapertaRSFEC;
                    } else {
                        if (aperta_aux_mode->modulation_mode == bcmplpApertaModulationPAM4) {
                            aperta_aux_mode->fec_mode_sel =  bcmplpapertaRS544;
                        } else {
                            aperta_aux_mode->fec_mode_sel =  bcmplpapertaRSFEC;
                        }
                    }
                } else if (port_cfg->fec_mode == phymodPaiFecModeFC) {
                    aperta_aux_mode->fec_mode_sel =  bcmplpapertaBaseR;
                } else {
                    aperta_aux_mode->fec_mode_sel =  bcmplpapertaNoFEC;
                }
            }
        } else {
            if ((port_cfg->fec_mode >> 8) & 0xFF) {
                if ((aperta_aux_mode->fec_mode_sel == bcmplpApertaRSFEC)) {
                    port_cfg->fec_mode = phymodPaiFecModeExtendedRS528;
                } else if (aperta_aux_mode->fec_mode_sel == bcmplpApertaRS544) {
                    port_cfg->fec_mode = phymodPaiFecModeExtendedRS544;
                } else if (aperta_aux_mode->fec_mode_sel == bcmplpApertaRS544_2XN) {
                    port_cfg->fec_mode = phymodPaiFecModeExtendedRS544_INTERLEAVED;
                } else if (aperta_aux_mode->fec_mode_sel == bcmplpApertaBaseR) {
                    port_cfg->fec_mode = phymodPaiFecModeExtendedFC;
                } else {
                    port_cfg->fec_mode = phymodPaiFecModeExtendedNone;
                }
            } else {
                if ((aperta_aux_mode->fec_mode_sel == bcmplpApertaRS544_2XN) ||
                    (aperta_aux_mode->fec_mode_sel == bcmplpApertaRS544) || 
                    (aperta_aux_mode->fec_mode_sel == bcmplpApertaRSFEC)) {
                    port_cfg->fec_mode = phymodPaiFecModeRS;
                } else if (aperta_aux_mode->fec_mode_sel == bcmplpApertaBaseR) {
                    port_cfg->fec_mode = phymodPaiFecModeFC;
                } else {
                    port_cfg->fec_mode = phymodPaiFecModeNone;
                }
                port_cfg->failover_lane_map = aperta_aux_mode->failover_config.lane_map;
            }
        }
    } else if (pai_phy_config->config_type == phymodConfigTypeTraining) {
        phymod_pai_tx_training_t *pai_training_enable = (phymod_pai_tx_training_t*)(pai_phy_config->config_data);
        uint32_t *aperta_training_enable = (uint32_t*)(epdm_data);
        if (operation == phymodOperationPai_to_phy) { 
            if (pai_training_enable->training_enable) {
                *aperta_training_enable = 1;
            } else {
                *aperta_training_enable = 0;
            }
        } else {
            if (*aperta_training_enable) {
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

int plp_aperta_phy_pam4_fec_status_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_fec_dump_status_t* fec_sts)
{
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_fec_status_get(phy, fec_sts));
    return PHYMOD_E_NONE;
}

int plp_aperta_phy_pcs_status_get(const plp_aperta_phymod_phy_access_t* phy, phymod_pcs_status_t* pcs_status)
{
    return _plp_aperta_pcs_status_get(phy, pcs_status);
}

#endif /* PHYMOD_APERTA_SUPPORT */
