/*
 * $Id: phymod.xml,v 1.1.2.5 Broadcom SDK $
 *
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

#include <phymod/phymod.h>
#include <phymod/phymod_dispatch.h>
#include <phymod/phymod_util.h>

#include "../tier1/barchetta_cfg_seq.h"
#ifdef PHYMOD_BARCHETTA_SUPPORT
extern barchetta_sw_db_t plp_barchetta_sw_db[BARCHETTA_NUM_OF_PHY][BARCHETTA_NUM_OF_PORTS]; /* SW database for all barchetta hardware ports */
extern uint8_t plp_barchetta_allocated_port_list[BARCHETTA_NUM_OF_PHY][BARCHETTA_NUM_OF_PORTS];       /* Allocated port list container for multi PHY   */
extern uint8_t plp_barchetta_unallocated_port_list[BARCHETTA_NUM_OF_PHY][BARCHETTA_NUM_OF_PORTS];     /* Unallocated port list container for multi PHY */

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_core_identify(const plp_barchetta_phymod_core_access_t* core, uint32_t core_id, uint32_t* is_identified)
{
    int chip_id = 0;

    *is_identified = 0;
    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_get_chip_id (&core->access, &chip_id));
    if (chip_id == BARCHETTA_CHIP_81337 ||
        chip_id == BARCHETTA_CHIP_81338 ||
        chip_id == BARCHETTA_CHIP_81381 ||
        chip_id == BARCHETTA_CHIP_81321 ||
        chip_id == BARCHETTA_CHIP_81764) {
        *is_identified = 1;
        /* Adding Bcast Suport*/
        *is_identified |= 0x80000000;
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_core_info_get(const plp_barchetta_phymod_core_access_t* core, plp_barchetta_phymod_core_info_t* info)
{
    PHYMOD_STRNCPY(info->name, "BARC", 5);

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_core_reset_set(const plp_barchetta_phymod_core_access_t* core, plp_barchetta_phymod_reset_mode_t reset_mode, plp_barchetta_phymod_reset_direction_t direction)
{
    return _plp_barchetta_core_reset_set(core, reset_mode, direction);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_core_reset_get(const plp_barchetta_phymod_core_access_t* core, plp_barchetta_phymod_reset_mode_t reset_mode, plp_barchetta_phymod_reset_direction_t* direction)
{
    /* Place your code here */
    return PHYMOD_E_UNAVAIL;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_core_firmware_info_get(const plp_barchetta_phymod_core_access_t* core, plp_barchetta_phymod_core_firmware_info_t* fw_info)
{
    return _plp_barchetta_core_firmware_info_get(core, fw_info);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_firmware_lane_config_set(const plp_barchetta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t fw_lane_config)
{
    return  _plp_barchetta_phy_firmware_lane_config_set(phy, fw_lane_config);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_firmware_lane_config_get(const plp_barchetta_phymod_phy_access_t* phy, phymod_firmware_lane_config_t* fw_lane_config)
{
    return  _plp_barchetta_phy_firmware_lane_config_get(phy, fw_lane_config);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_core_pll_sequencer_restart(const plp_barchetta_phymod_core_access_t* core, uint32_t flags, plp_barchetta_phymod_sequencer_operation_t operation)
{
    /* Place your code here */
    return PHYMOD_E_UNAVAIL;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_polarity_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_polarity_t* polarity)
{
    return _plp_barchetta_phy_polarity_set(phy, polarity);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_polarity_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_polarity_t* polarity)
{
    return _plp_barchetta_phy_polarity_get(phy, polarity);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_pam4_tx_set(const plp_barchetta_phymod_phy_access_t* phy, const phymod_pam4_tx_t* tx)
{
    return _plp_barchetta_phy_pam4_tx_set(phy, tx);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_pam4_tx_get(const plp_barchetta_phymod_phy_access_t* phy, phymod_pam4_tx_t* tx)
{
    return _plp_barchetta_phy_pam4_tx_get(phy, tx);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_rx_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_rx_t* rx)
{
    return _plp_barchetta_phy_rx_set(phy, rx);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_rx_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_rx_t* rx)
{
    PHYMOD_MEMSET(rx, 0, sizeof(plp_barchetta_phymod_rx_t));
    return _plp_barchetta_phy_rx_get(phy, rx);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_reset_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_phy_reset_t* reset)
{
    return _plp_barchetta_phy_reset_set(phy, reset);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_reset_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_phy_reset_t* reset)
{
    return _plp_barchetta_phy_reset_get(phy, reset);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_power_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_phy_power_t* power)
{
    return _plp_barchetta_phy_power_set(phy, power);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_power_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_phy_power_t* power)
{
    return _plp_barchetta_phy_power_get(phy, power);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_tx_lane_control_set(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_phy_tx_lane_control_t tx_control)
{
    return _plp_barchetta_phy_tx_lane_control_set(phy, tx_control);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_tx_lane_control_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_phy_tx_lane_control_t* tx_control)
{
    return _plp_barchetta_phy_tx_lane_control_get(phy, tx_control);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_rx_lane_control_set(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_phy_rx_lane_control_t rx_control)
{
    return _plp_barchetta_phy_rx_lane_control_set(phy, rx_control);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_rx_lane_control_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_phy_rx_lane_control_t* rx_control)
{
    return _plp_barchetta_phy_rx_lane_control_get(phy, rx_control);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_interface_config_set(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags, const plp_barchetta_phymod_phy_inf_config_t* config)
{
    int chip_id = 0;
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;
    if (aux_mode->modulation_mode == BARCHETTA_MODULATION_PAM4) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_chip_id(&phy->access, &chip_id));
        if (chip_id == BARCHETTA_CHIP_81381) {
            return PHYMOD_E_PARAM;
        }
    }
    return (_plp_barchetta_phy_interface_config_set(phy, flags, config));
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_interface_config_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags, plp_barchetta_phymod_ref_clk_t ref_clock, plp_barchetta_phymod_phy_inf_config_t* config)
{
    return _plp_barchetta_phy_interface_config_get(phy, flags, config);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_core_init(const plp_barchetta_phymod_core_access_t* core, const plp_barchetta_phymod_core_init_config_t* init_config, const plp_barchetta_phymod_core_status_t* core_status)
{
    BCMI_BARCHETTA_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t bcast;
    PHYMOD_MEMSET(&bcast, 0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_MDIO_PHYAD_CTRLr_t));
    if (init_config->firmware_load_method == phymodFirmwareLoadMethodNone) {
        PHYMOD_IF_ERR_RETURN( _plp_barchetta_core_reset_set(core, phymodResetModeHard, 0));
        PHYMOD_SLEEP(1);
        PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_check_fw_download_status(core, init_config->firmware_load_method));
    }
    if (PHYMOD_CORE_INIT_F_RESET_CORE_FOR_FW_LOAD_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN( _plp_barchetta_core_reset_set(core, phymodResetModeHard, 0));
    } else if (PHYMOD_CORE_INIT_F_UNTIL_FW_LOAD_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, &bcast));
        BCMI_BARCHETTA_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET(bcast, 1);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, bcast));
    } else if (PHYMOD_CORE_INIT_F_EXECUTE_FW_LOAD_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_dload_fw(core, init_config));
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, &bcast));
        BCMI_BARCHETTA_GEN_CNTRLS_MDIO_PHYAD_CTRLr_MDIO_BRDCST_ENf_SET(bcast, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MDIO_PHYAD_CTRLr(&core->access, bcast));
    } else if (PHYMOD_CORE_INIT_F_RESUME_AFTER_FW_LOAD_GET(init_config)) {
        PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_check_fw_download_status(core, init_config->firmware_load_method));
    } else if(PHYMOD_CORE_INIT_F_FW_LOAD_END_GET(init_config)) {
        /* Not required for now*/
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_init(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_phy_init_config_t* init_config)
{
    return _plp_barchetta_phy_init(phy, init_config);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_warmboot_init(const plp_barchetta_phymod_core_access_t* core, void* init_data)
{
    return _plp_barchetta_warmboot_init(core, init_data) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_loopback_set(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_loopback_mode_t loopback, uint32_t enable)
{
    uint32_t enable_state = 0;
    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_phy_loopback_get(phy, loopback, &enable_state));
    if (enable_state != enable){
        PHYMOD_IF_ERR_RETURN(
           _plp_barchetta_phy_loopback_set(phy, loopback, enable));
    }
    
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_loopback_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_loopback_mode_t loopback, uint32_t* enable)
{
    return _plp_barchetta_phy_loopback_get(phy, loopback, enable);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_rx_pmd_locked_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t* rx_pmd_locked)
{
    return _plp_barchetta_phy_rx_pmd_locked_get(phy, rx_pmd_locked);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_link_status_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t* link_status)
{
    return _plp_barchetta_phy_rx_pmd_locked_get(phy, link_status);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_status_dump(const plp_barchetta_phymod_phy_access_t* phy)
{
    return _plp_barchetta_phy_status_dump(phy);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_reg_read(const plp_barchetta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t* val)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t lane_index       = 0;
    uint8_t dp_dir =0;
    int port_number = 0, port_lane = 0;
    BARCHETTA_REGISTER_SELECT_T reg_sel;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;
    dp_dir = (reg_addr >> 28) & 0xF;

    if (dp_dir == 0) {
    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, (lane_index)));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_reg_read(&phy->access, (reg_addr | (phy->access.devad << 16)), val));
            break;
        }
    }
    } else {
        if (dp_dir == 1) {
            reg_sel = BarchettaRegisterSelectEgress;
        } else {
            reg_sel = BarchettaRegisterSelectIngress;
        }
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
        if (port_number == 0xFF) {
            return PHYMOD_E_CONFIG;
        }
        for (lane_index =0; lane_index < num_of_max_lanes; lane_index++) {
            if (phy->access.lane_mask & (1 << lane_index)) {
                PHYMOD_IF_ERR_RETURN(
                   _plp_barchetta_get_port_lane(phy, num_of_max_lanes, (1 << lane_index), port_number, &port_lane)); 

                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice (phy, port_number, (1 << port_lane), reg_sel, 
                        (phy->access.devad == 7) ? BarchettaRegisterTypeAN : BarchettaRegisterTypePMD , 0xFFFF));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_reg_read(&phy->access, (reg_addr | (phy->access.devad << 16)), val));
                break;
            }
        }
    }

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_reg_write(const plp_barchetta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t val)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t lane_index   = 0;
    uint8_t dp_dir =0;
    int port_number = 0, port_lane = 0;
    BARCHETTA_REGISTER_SELECT_T reg_sel;


    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;
    dp_dir = (reg_addr >> 28) & 0xF;

    if (dp_dir == 0) {
    for (lane_index =0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, (lane_index)));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_reg_write(&phy->access, (reg_addr | (phy->access.devad << 16)), val));
        }
    }
    } else {
        if (dp_dir == 1) {
            reg_sel = BarchettaRegisterSelectEgress;
        } else {
            reg_sel = BarchettaRegisterSelectIngress;
        }
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
        if (port_number == 0xFF) {
            return PHYMOD_E_CONFIG;
        }
        for (lane_index =0; lane_index < num_of_max_lanes; lane_index++) {
            if (phy->access.lane_mask & (1 << lane_index)) {
                PHYMOD_IF_ERR_RETURN(
                   _plp_barchetta_get_port_lane(phy, num_of_max_lanes, (1 << lane_index), port_number, &port_lane));

                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice (phy, port_number, (1 << port_lane), reg_sel, 
                        (phy->access.devad == 7) ? BarchettaRegisterTypeAN : BarchettaRegisterTypePMD , 0xFFFF));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_reg_write(&phy->access, (reg_addr | (phy->access.devad << 16)), val));
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_rev_id(const plp_barchetta_phymod_phy_access_t* phy, uint32_t* rev_id)
{
    return _plp_barchetta_get_chip_rev (&phy->access, rev_id);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_logical_lane_set(const plp_barchetta_phymod_core_access_t* core, const phymod_logical_lane_map_t* lane_map)
{
    plp_barchetta_phymod_phy_access_t phy;

    PHYMOD_MEMCPY(&phy, core, sizeof(plp_barchetta_phymod_phy_access_t));
    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_logical_lane_list_set(&phy, lane_map->num_of_lanes, lane_map->tx_lane_list, lane_map->rx_lane_list));

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_logical_lane_get(const plp_barchetta_phymod_core_access_t* core, phymod_logical_lane_map_t* lane_map)
{
    plp_barchetta_phymod_phy_access_t phy;

    PHYMOD_MEMCPY(&phy, core, sizeof(plp_barchetta_phymod_phy_access_t));
    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_logical_lane_list_get(&phy, &lane_map->num_of_lanes, lane_map->tx_lane_list, lane_map->rx_lane_list));

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_rx_adaptation_resume(const plp_barchetta_phymod_phy_access_t* phy)
{
    return PHYMOD_E_UNAVAIL;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_fec_enable_set(const plp_barchetta_phymod_phy_access_t* phy, uint32_t enable)
{
    return _plp_barchetta_phy_fec_enable_set(phy, enable);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_fec_enable_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t* enable)
{
    return _plp_barchetta_phy_fec_enable_get(phy, enable);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_tx_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_tx_t* tx)
{
    PHYMOD_DEBUG_ERROR(("phy_tx_set API is not available, please use PAM4_tx_set for both PAM4/NRZ modes\n"));
    return PHYMOD_E_UNAVAIL;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_tx_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_tx_t* tx)
{
    PHYMOD_DEBUG_ERROR(("phy_tx_get API is not available, please use PAM4_tx_get for both PAM4/NRZ modes\n"));
    return PHYMOD_E_UNAVAIL;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_cl72_set(const plp_barchetta_phymod_phy_access_t* phy, uint32_t cl72_en)
{
    return _plp_barchetta_phy_cl72_set(phy, cl72_en);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_cl72_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t* cl72_en)
{
    return _plp_barchetta_phy_cl72_get(phy, cl72_en);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_cl72_status_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_cl72_status_t* status)
{
    return _plp_barchetta_phy_cl72_status_get(phy, status);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_autoneg_ability_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_autoneg_ability_t* an_ability)
{
    if (plp_barchetta_phymod_count_set_bits(an_ability->an_cap) > 1) {
        PHYMOD_DEBUG_ERROR(("ERROR: Multiple AN capabilities are not allowed\n"));
        return PHYMOD_E_PARAM;
    }
    if ((an_ability->an_cap == PHYMOD_AN_CAP_50G_CR_KR) ||
        (an_ability->an_cap == PHYMOD_AN_CAP_100G_CR2_KR2) ||
        (an_ability->an_cap == PHYMOD_AN_CAP_200G_CR4_KR4)) {
        if(an_ability->an_cl72 == 0) {
            PHYMOD_DEBUG_ERROR(("ERROR: In PAM4 mode AN without training is not supported\n"));
            return PHYMOD_E_PARAM;
        }
    }
    return _plp_barchetta_phy_autoneg_ability_set(phy, an_ability);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_autoneg_ability_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_autoneg_ability_t* an_ability_get_type)
{
    return _plp_barchetta_phy_autoneg_ability_get(phy, an_ability_get_type);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_autoneg_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_autoneg_control_t* an)
{
    return _plp_barchetta_phy_autoneg_set(phy, an);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_autoneg_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_autoneg_control_t* an, uint32_t* an_done)
{
    return _plp_barchetta_phy_autoneg_get(phy, an, an_done);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_autoneg_remote_ability_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_autoneg_ability_t* an_ability_get_type)
{
    return _plp_barchetta_phy_autoneg_remote_ability_get(phy, an_ability_get_type);

}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_intr_enable_set(const plp_barchetta_phymod_phy_access_t* phy, uint32_t enable)
{
    return _plp_barchetta_phy_intr_enable_set(phy, enable) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_intr_enable_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t* enable)
{
    return _plp_barchetta_phy_intr_enable_get(phy, enable) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_intr_status_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t* intr_status)
{
    return _plp_barchetta_phy_intr_status_get(phy, intr_status) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_intr_status_clear(const plp_barchetta_phymod_phy_access_t* phy, uint32_t intr_clr)
{
    return _plp_barchetta_phy_intr_status_clear(phy, intr_clr) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_i2c_read(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags, uint32_t addr, uint32_t offset, uint32_t size, uint8_t* data)
{
    int chip_id = 0 ;
    (void)flags ;
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_chip_id(&phy->access, &chip_id));
    if ((chip_id == BARCHETTA_CHIP_81337) || (chip_id == BARCHETTA_CHIP_81338)) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Does not support module controller")));
    }
    return _plp_barchetta_phy_i2c_read(phy, addr, offset, size, data) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_i2c_write(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags, uint32_t addr, uint32_t offset, uint32_t size, const uint8_t* data)
{
    int chip_id = 0 ;
    (void)flags ;
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_chip_id(&phy->access, &chip_id));
    if ((chip_id == BARCHETTA_CHIP_81337) || (chip_id == BARCHETTA_CHIP_81338)) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Does not support module controller")));
    }
    return _plp_barchetta_phy_i2c_write(phy, addr, offset, size, data) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_gpio_config_set(const plp_barchetta_phymod_phy_access_t* phy, int pin_no, plp_barchetta_phymod_gpio_mode_t gpio_mode)
{
    return _plp_barchetta_phy_gpio_config_set(phy, pin_no, gpio_mode) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_gpio_config_get(const plp_barchetta_phymod_phy_access_t* phy, int pin_no, plp_barchetta_phymod_gpio_mode_t* gpio_mode)
{
    return _plp_barchetta_phy_gpio_config_get(phy, pin_no, gpio_mode) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_gpio_pin_value_set(const plp_barchetta_phymod_phy_access_t* phy, int pin_no, int value)
{
    return _plp_barchetta_phy_gpio_pin_value_set(phy, pin_no, value) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_gpio_pin_value_get(const plp_barchetta_phymod_phy_access_t* phy, int pin_no, int* value)
{
    return _plp_barchetta_phy_gpio_pin_value_get(phy, pin_no, value) ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_failover_mode_set(const plp_barchetta_phymod_phy_access_t* phy, unsigned int failover_mode)
{
    return _plp_barchetta_failover_mode_set(phy, failover_mode) ;
}

int plp_barchetta_failover_mode_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_failover_mode_t* failover_mode)
{
    return _plp_barchetta_failover_mode_get(phy, failover_mode) ;
}

/*******************************************************************************
 PURPOSE: Enable/Disable SyncE with user provided configuration.

 COMMENT:
 *******************************************************************************/
int plp_barchetta_synce_config_set(const plp_barchetta_phymod_phy_access_t* phy, const phymod_synce_cfg_t* synce_cfg)
{
    return _plp_barchetta_synce_config_set(phy, synce_cfg);
}

/*******************************************************************************
 PURPOSE: Retrieve current SyncE Configuration and return to the user.

 COMMENT:
 *******************************************************************************/
int plp_barchetta_synce_config_get(const plp_barchetta_phymod_phy_access_t* phy, phymod_synce_cfg_t* synce_cfg)
{
    return _plp_barchetta_synce_config_get(phy, synce_cfg);
}

/*******************************************************************************
 PURPOSE: Enables serdes diagnostics access for debugging

 COMMENT:
 *******************************************************************************/
int plp_barchetta_srds_diag_access_enable(const plp_barchetta_phymod_phy_access_t* phy, const phymod_srds_diag_access_cfg_t* diag_access_cfg)
{
    return _plp_barchetta_srds_diag_access_enable(phy, diag_access_cfg);
}

/*******************************************************************************
 PURPOSE: PAI support: pai info get

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_pai_info_get(const plp_barchetta_phymod_phy_access_t* phy, phymod_pai_phy_op_t operation, phymod_pai_phy_config_t* pai_phy_config, void* epdm_data)
{
    int no_of_lanes = 0, ldr = 0;

    if (pai_phy_config->config_type == phymodConfigTypePort_config) {
        phymod_pai_port_config_t *port_cfg = (phymod_pai_port_config_t*)(pai_phy_config->config_data);
        barchetta_aux_modes_t *barchetta_aux_mode = (barchetta_aux_modes_t*) epdm_data;
        if (operation == phymodOperationPai_to_phy) {
            PHYMOD_MEMSET(barchetta_aux_mode, 0, sizeof(barchetta_aux_modes_t));
            no_of_lanes = plp_barchetta_phymod_count_set_bits(phy->access.lane_mask);
            barchetta_aux_mode->failover_lane_map = port_cfg->failover_lane_map;
            ldr = (port_cfg->speed/no_of_lanes);
            if (ldr == BARCHETTA_SPEED_10G) {
                barchetta_aux_mode->lane_data_rate = BARCHETTA_LANE_DATA_RATE_10P3125G;
                barchetta_aux_mode->modulation_mode = BARCHETTA_MODULATION_NRZ;
            } else if (ldr == BARCHETTA_SPEED_20G) {
                barchetta_aux_mode->lane_data_rate = BARCHETTA_LANE_DATA_RATE_20P625G;
                barchetta_aux_mode->modulation_mode = BARCHETTA_MODULATION_NRZ;
            } else if (ldr == BARCHETTA_SPEED_25G) {
                barchetta_aux_mode->lane_data_rate = BARCHETTA_LANE_DATA_RATE_25P78125G;
                barchetta_aux_mode->modulation_mode = BARCHETTA_MODULATION_NRZ;
            } else if (ldr == BARCHETTA_SPEED_50G) {
                barchetta_aux_mode->lane_data_rate = BARCHETTA_LANE_DATA_RATE_53P125G;
                barchetta_aux_mode->modulation_mode = BARCHETTA_MODULATION_PAM4;
            } else {
                PHYMOD_DEBUG_ERROR(("Invalid lane data rate : %d number of lane:%d speed:%d\n",
                            ldr,no_of_lanes, port_cfg->speed));
                return PHYMOD_E_PARAM;
            }
            if(port_cfg->tx_drv_supply == phymodPaiTxDrvSupply0P80Volt) {
                barchetta_aux_mode->tx_driver_mode = bcmplptxdrivebarchetta0P8Volt;
            } else if (port_cfg->tx_drv_supply == phymodPaiTxDrvSupply1P20Volt) {
                barchetta_aux_mode->tx_driver_mode = bcmplptxdrivebarchetta1P2Volt;
            } else if ((port_cfg->tx_drv_supply == phymodPaiTxDrvSupply1P00Volt) ||
                       (port_cfg->tx_drv_supply == phymodPaiTxDrvSupply1P25Volt)) {
                PHYMOD_DEBUG_ERROR(("Invalid tx drv supply :%d\n", port_cfg->tx_drv_supply));
                return PHYMOD_E_PARAM;
            } else {
                barchetta_aux_mode->tx_driver_mode = bcmplptxdrivebarchettaHWdefaults;
            }
            if (port_cfg->low_latency_variation == phymodPaiLowLatencyVariationDisable) {
                barchetta_aux_mode->ll_mode = bcmplpLowLatencyVariationDisable;
            } else if (port_cfg->low_latency_variation == phymodPaiLowLatencyVariationEnable) {
                barchetta_aux_mode->ll_mode = bcmplpLowLatencyVariationEnable;
            } else if (port_cfg->low_latency_variation == phymodPaiLowLatencyVariationIngressEnable) {
                barchetta_aux_mode->ll_mode = bcmplpLowLatencyVariationIngressEnable;
            } else if (port_cfg->low_latency_variation == phymodPaiLowLatencyVariationEgressEnable) {
                barchetta_aux_mode->ll_mode = bcmplpLowLatencyVariationEgressEnable;
            } else {
                PHYMOD_DEBUG_ERROR(("Invalid low latency variation :%d\n", port_cfg->low_latency_variation));
                return PHYMOD_E_PARAM;
            }
        } else {
            port_cfg->failover_lane_map = barchetta_aux_mode->failover_lane_map;
            if (barchetta_aux_mode->tx_driver_mode == bcmplptxdrivebarchetta0P8Volt) {
                port_cfg->tx_drv_supply = phymodPaiTxDrvSupply0P80Volt;
            } else if (barchetta_aux_mode->tx_driver_mode == bcmplptxdrivebarchetta1P2Volt) {
                port_cfg->tx_drv_supply = phymodPaiTxDrvSupply1P20Volt;
            } else {
                port_cfg->tx_drv_supply = phymodPaiTxDrvSupplyHWDefault;
            }
            if (barchetta_aux_mode->ll_mode == bcmplpLowLatencyVariationDisable) {
                port_cfg->low_latency_variation = phymodPaiLowLatencyVariationDisable;
            } else if (barchetta_aux_mode->ll_mode == bcmplpLowLatencyVariationEnable) {
                port_cfg->low_latency_variation = phymodPaiLowLatencyVariationEnable;
            } else if (barchetta_aux_mode->ll_mode == bcmplpLowLatencyVariationIngressEnable) {
                port_cfg->low_latency_variation = phymodPaiLowLatencyVariationIngressEnable;
            } else if (barchetta_aux_mode->ll_mode == bcmplpLowLatencyVariationEgressEnable) {
                port_cfg->low_latency_variation = phymodPaiLowLatencyVariationEgressEnable;
            } else {
                PHYMOD_DEBUG_ERROR(("Invalid aux low latency variation :%d\n", barchetta_aux_mode->ll_mode));
                return PHYMOD_E_PARAM;
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

#endif /* PHYMOD_BARCHETTA_SUPPORT */
