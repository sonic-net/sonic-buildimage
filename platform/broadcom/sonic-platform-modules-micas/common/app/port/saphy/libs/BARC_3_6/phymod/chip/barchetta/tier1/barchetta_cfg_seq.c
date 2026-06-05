/*
 *
 * $Id: barchetta_cfg_seq.c,  $
 *
 *  *
 *  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
 *  *
 *  *
 *
 */

#include "barchetta_cfg_seq.h"
#include <phymod/phymod_acc.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diagnostics_dispatch.h>

extern unsigned char plp_barchetta_plp_barchetta_ucode_bin[];
extern uint32_t plp_barchetta_plp_barchetta_ucode_len;

extern unsigned char plp_barchetta_plp_barchetta_b0_ucode_bin[];
extern uint32_t plp_barchetta_plp_barchetta_b0_ucode_len;

barchetta_sw_db_t plp_barchetta_sw_db[BARCHETTA_NUM_OF_PHY][BARCHETTA_NUM_OF_PORTS]; /* SW database for all barchetta hardware ports  */
uint8_t plp_barchetta_allocated_port_list[BARCHETTA_NUM_OF_PHY][BARCHETTA_NUM_OF_PORTS];       /* Allocated port list array for multi PHY       */
uint8_t plp_barchetta_unallocated_port_list[BARCHETTA_NUM_OF_PHY][BARCHETTA_NUM_OF_PORTS];     /* Unallocated port list array for multi PHY     */

extern const unsigned long plp_barchetta_dr_table[BARCHETTA_MAX_NO_DR][BARCHETTA_MAX_NO_DR_COL];

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_get_chip_id(const plp_barchetta_phymod_access_t *pa, int *chip_id) {
    BCMI_BARCHETTA_CTRL_CHIP_IDr_t chip_id_lsb;
    BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_t chip_id_msb;

    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_CTRL_CHIP_IDr(pa, &chip_id_lsb));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_CTRL_CHIP_REVISIONr(pa, &chip_id_msb));
    *chip_id =(BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_ID_19_16f_GET(chip_id_msb)<< 16)|
               BCMI_BARCHETTA_CTRL_CHIP_IDr_CHIP_ID_15_0f_GET(chip_id_lsb);
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to get the package related information like package type(Duplex/Simplex),
           Package lanes (8/16 lanes), number of dies (single / dual die package)
 \param    pa          [In]  Pointer to phymod access structure
 \param    pkg_info    [Out] Pointer to package information structure
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int
_plp_barchetta_get_package_info(const plp_barchetta_phymod_access_t *pa,
        barchetta_package_info_t *pkg_info) {
    int chip_id;
    PHYMOD_IF_ERR_RETURN(
                 _plp_barchetta_get_chip_id(pa, &chip_id));
    if ((chip_id == BARCHETTA_CHIP_81337) ||
        (chip_id == BARCHETTA_CHIP_81338) ||
        (chip_id == BARCHETTA_CHIP_81381) ||
        (chip_id == BARCHETTA_CHIP_81321)) {
        pkg_info->pkg_type = BARCHETTA_PORT_TYPE_DUPLEX;
        pkg_info->pkg_lanes = BARCHETTA_MAX_NUM_DUPLEX_LANES;
        pkg_info->no_of_dies = BARCHETTA_SINGLE_DIE_PACKAGE;
        pkg_info->no_of_max_ports = BARCHETTA_MAX_NUM_DUPLEX_PORTS ;

    } else if (chip_id == BARCHETTA_CHIP_81764) {
        pkg_info->pkg_type = BARCHETTA_PORT_TYPE_DUPLEX;
        pkg_info->pkg_lanes = BARCHETTA_MAX_NUM_DUPLEX_LANES;
        pkg_info->no_of_dies = BARCHETTA_DUAL_DIE_PACKAGE;
        pkg_info->no_of_max_ports = BARCHETTA_MAX_NUM_DUPLEX_PORTS ;

    } else if (chip_id == 0x1234) { 
        pkg_info->pkg_type = BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT;
        pkg_info->pkg_lanes = BARCHETTA_MAX_NUM_SIMPLEX_LANES;
        pkg_info->no_of_dies = BARCHETTA_SINGLE_DIE_PACKAGE;
        pkg_info->no_of_max_ports = BARCHETTA_MAX_NUM_SIMPLEX_PORTS ;

    } else if (chip_id == 0x5678) { 
        pkg_info->pkg_type = BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST;
        pkg_info->pkg_lanes = BARCHETTA_MAX_NUM_SIMPLEX_LANES;
        pkg_info->no_of_dies = BARCHETTA_SINGLE_DIE_PACKAGE;
        pkg_info->no_of_max_ports = BARCHETTA_MAX_NUM_SIMPLEX_PORTS ;
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to find log2(n)
 \param    n  [In] Input for which the log2 operation will be performed
 \return   log2(n) value
*******************************************************************************/
static uint8_t
__plp_barchetta_log2n(uint32_t n) {
    return (n > 1 ? 1 + __plp_barchetta_log2n(n / 2) : 0);
}

/***************************************************************************//**
 \brief    API to determine the number of set bits in the provided lane map
 \param    lane_map  [In] Lane map
 \return   Number of set bits (number of ones) in the provided lane map
*******************************************************************************/
static uint8_t
__plp_barchetta_count_number_of_set_bits(uint32_t lane_map) {
    uint8_t count = 0;
    while (lane_map) {
        lane_map &= (lane_map - 1);
        count++;
    }
    return (count);
}

/***************************************************************************//**
 \brief    API to convert lane map into lane list and also determines the
           number of lanes from the lane map
 \param    pa           [In]  Pointer to phymod access structure
 \param    pkg_info     [In]  Package information
 \param    lane_map     [In]  Lane map
 \param    no_of_lanes  [Out] Number of lanes in the lane map
 \param    lane_list    [Out] Lane list derived from the lane map
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_convert_lane_map_to_lane_list(const plp_barchetta_phymod_access_t *pa,
         barchetta_package_info_t pkg_info, uint32_t lane_map,
         uint8_t *no_of_lanes, uint8_t *lane_list) {
    uint8_t num_of_active_lanes = 0;
    uint8_t num_of_pkg_lanes = 0;
    uint8_t i = 0, j = 0;
    num_of_pkg_lanes = pkg_info.pkg_lanes ;
    if (lane_map < 0xFFFF) {
        num_of_active_lanes = __plp_barchetta_count_number_of_set_bits(lane_map);
        if (num_of_active_lanes <= num_of_pkg_lanes) {
            for (i = 0; i < num_of_pkg_lanes; i++) {
                if (lane_map & (1 << i)) {
                    lane_list[j++] = i;
                }
            }
            *no_of_lanes = num_of_active_lanes;
        }
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to convert lane list to lane map
 \param    pa                [In]  Pointer to phymod access structure
 \param    lane_list         [In]  Lane list
 \param    no_of_lanes       [In]  Number of lanes in the lane list
 \param    lane_map          [Out] Pointer to lane map
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_convert_lane_list_to_lane_map(const plp_barchetta_phymod_access_t *pa,
         uint8_t *lane_list, uint8_t no_of_lanes, uint32_t *lane_map) {
    uint8_t i = 0;
    *lane_map = 0;
    for(i=0; i<no_of_lanes; i++) {
        *lane_map |= 1 << lane_list[i] ;
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to convert logical lane map to package lane map
 \param    pa  [In]  Pointer to phymod access structure
 \return   Package lane map
*******************************************************************************/
static uint32_t
__plp_barchetta_convert_logical_lanemap_to_package_lanemap(const plp_barchetta_phymod_access_t *pa) {
    uint32_t  logical_lane_map = PHYMOD_ACC_LANE_MASK(pa);
    uint32_t pkg_lane_map = 0 ;
    int chip_id;
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_chip_id(pa, &chip_id));
    if ((chip_id == BARCHETTA_CHIP_81381) || (chip_id == BARCHETTA_CHIP_81321)) {
        pkg_lane_map = logical_lane_map ;
    } else if((chip_id == BARCHETTA_CHIP_81337) || (chip_id == BARCHETTA_CHIP_81338)) {
      
        pkg_lane_map = logical_lane_map ;
    } else {
        pkg_lane_map = logical_lane_map ;
    }
    return(pkg_lane_map) ;
}

/***************************************************************************//**
 \brief    API to retrieve hardware port number from SW database based on phymod access lane mask
 \param    phy         [In]  Pointer to phy access structure
 \param    pkg_info    [In]  Package information
 \param    port_number [Out] Pointer to port number
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int
_plp_barchetta_retrieve_hardware_port_number_from_sw_database(
        const plp_barchetta_phymod_phy_access_t *phy, barchetta_package_info_t pkg_info, int *port_number) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t lane_map = PHYMOD_ACC_LANE_MASK(pa);
    uint8_t port_type = 0;
    uint8_t i         = 0;

    port_type = pkg_info.pkg_type;
    *port_number = 0xFF ;

    if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
            for (i = 0; i < pkg_info.no_of_max_ports; i++) {
                if (plp_barchetta_sw_db[pa->addr][i].port_status == BARCHETTA_PORT_ALLOCATED) {
                    if (plp_barchetta_sw_db[pa->addr][i].lane_map_info.lane_map.duplex_lane_map.sys_lane_map & lane_map) {
                        *port_number = i ;
                    }
                }
            }
        } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
            for (i = 0; i < pkg_info.no_of_max_ports; i++) {
                if (plp_barchetta_sw_db[pa->addr][i].port_status == BARCHETTA_PORT_ALLOCATED) {
                    if (plp_barchetta_sw_db[pa->addr][i].lane_map_info.lane_map.duplex_lane_map.line_lane_map & lane_map) {
                        *port_number = i ;
                    }
                }
            }
        }
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        
    }
    return PHYMOD_E_NONE;
}
/*******************************************************************************
 PURPOSE:  Function is to get logical lane number from lane MAP.

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_get_logical_lane_from_lane_map(const plp_barchetta_phymod_phy_access_t* phy,
        uint8_t lane_map_index, uint8_t *logical_lane) {
    *logical_lane = lane_map_index ;
#ifdef BCM_PLP_RMT_PHY_CAPABILITY
    {
        barchetta_package_info_t pkg_info;
        barchetta_port_config_t port_cfg_rd ;
        int port_number = 0, no_sel_lane =0, lane_index = 0;

        PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
        PHYMOD_MEMSET(&port_cfg_rd, 0, sizeof(barchetta_port_config_t));
        no_sel_lane = __plp_barchetta_count_number_of_set_bits(phy->access.lane_mask);
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
        if (port_number == 0xFF) {
            *logical_lane = lane_map_index ;
            return PHYMOD_E_NONE;
        }
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_msg_config_port_rd(&phy->access, port_number, &port_cfg_rd));
        if ((port_cfg_rd.port_lanes.duplex.num_line_lanes == 1) &&  (no_sel_lane == 2)) {
            /* Selecting only valid lane*/
            for (lane_index = 0; lane_index < pkg_info.pkg_lanes; lane_index++) {
                if (phy->access.lane_mask & (1 << lane_index)) {
                    *logical_lane = lane_index;
                    break;
                }
            }
        }
    }
#endif
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_get_chip_rev(const plp_barchetta_phymod_access_t *pa, uint32_t *chip_rev) {
    BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_t chip_id_msb;

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_CTRL_CHIP_REVISIONr(pa, &chip_id_msb));
    *chip_rev = BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(chip_id_msb);

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: To read the chip revision from internal register

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_get_internal_chip_rev(const plp_barchetta_phymod_access_t *pa, uint32_t *chip_rev) {
    BCMI_BARCHETTA_GEN_CNTRLS_GPREG_0Br_t gpreg_0b;

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_GPREG_0Br(pa, &gpreg_0b));
    *chip_rev = BCMI_BARCHETTA_GEN_CNTRLS_GPREG_0Br_GPREG_0B_DATAf_GET(gpreg_0b);

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_set_slice(const plp_barchetta_phymod_phy_access_t *phy, int port, int port_lane,
        BARCHETTA_REGISTER_SELECT_T reg_sel, BARCHETTA_REGISTER_TYPE_T reg_type,
        int lane_based) {
    uint8_t pll_idx = 0;
    BCMI_BARCHETTA_SLICE_SLICE0r_t slice_0;
    BCMI_BARCHETTA_SLICE_SLICE1r_t slice_1;

    BCMI_BARCHETTA_SLICE_SLICE0r_CLR(slice_0);
    BCMI_BARCHETTA_SLICE_SLICE1r_CLR(slice_1);

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_get_pll_index(phy, &pll_idx));

    if (lane_based == 0xFFFF) {
        if (port != 0xFF) {
            BCMI_BARCHETTA_SLICE_SLICE0r_PORT_SELf_SET(slice_0, port);
        }
        if (port_lane != 0xFFFF) {
            BCMI_BARCHETTA_SLICE_SLICE0r_LANE_SELf_SET(slice_0, port_lane);
        }
        if (reg_sel == BarchettaRegisterSelectIngress) {
            BCMI_BARCHETTA_SLICE_SLICE0r_IN_SELf_SET(slice_0, 1);
            BCMI_BARCHETTA_SLICE_SLICE0r_OUT_SELf_SET(slice_0, 0);
        } else if (reg_sel == BarchettaRegisterSelectEgress) {
            BCMI_BARCHETTA_SLICE_SLICE0r_IN_SELf_SET(slice_0, 0);
            BCMI_BARCHETTA_SLICE_SLICE0r_OUT_SELf_SET(slice_0, 1);
        } else {
            BCMI_BARCHETTA_SLICE_SLICE0r_IN_SELf_SET(slice_0, 0);
            BCMI_BARCHETTA_SLICE_SLICE0r_OUT_SELf_SET(slice_0, 0);
        }
        if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
            BCMI_BARCHETTA_SLICE_SLICE0r_SYS_SELf_SET(slice_0, 1);
            BCMI_BARCHETTA_SLICE_SLICE0r_LIN_SELf_SET(slice_0, 0);
        }
        if (BARCHETTA_IS_LINE_SIDE(phy)) {
            BCMI_BARCHETTA_SLICE_SLICE0r_SYS_SELf_SET(slice_0, 0);
            BCMI_BARCHETTA_SLICE_SLICE0r_LIN_SELf_SET(slice_0, 1);
        }
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));

        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_SLICE_SLICE1r(&phy->access, &slice_1));

        BCMI_BARCHETTA_SLICE_SLICE1r_BH_PORT_MODE_SELf_SET(slice_1, 1);
        if (reg_type == BarchettaRegisterTypeAN) {
            BCMI_BARCHETTA_SLICE_SLICE1r_REG_TYPE_SELf_SET(slice_1, 1);
        } else {
            BCMI_BARCHETTA_SLICE_SLICE1r_REG_TYPE_SELf_SET(slice_1, 0);
        }
        BCMI_BARCHETTA_SLICE_SLICE1r_PLL_MICRO_SELf_SET(slice_1, pll_idx);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_SLICE_SLICE1r(&phy->access, slice_1));
    } else {
        BCMI_BARCHETTA_SLICE_SLICE0r_LANE_SELf_SET(slice_0, (1 << lane_based));

        /* This is dont care, But needed for lane based register access*/
        BCMI_BARCHETTA_SLICE_SLICE0r_IN_SELf_SET(slice_0, 1);
        if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
            BCMI_BARCHETTA_SLICE_SLICE0r_SYS_SELf_SET(slice_0, 1);
            BCMI_BARCHETTA_SLICE_SLICE0r_LIN_SELf_SET(slice_0, 0);
        }
        if (BARCHETTA_IS_LINE_SIDE(phy)) {
            BCMI_BARCHETTA_SLICE_SLICE0r_SYS_SELf_SET(slice_0, 0);
            BCMI_BARCHETTA_SLICE_SLICE0r_LIN_SELf_SET(slice_0, 1);
        }
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));

        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_SLICE_SLICE1r(&phy->access, &slice_1));

        BCMI_BARCHETTA_SLICE_SLICE1r_BH_PORT_MODE_SELf_SET(slice_1, 0);
        BCMI_BARCHETTA_SLICE_SLICE1r_PLL_MICRO_SELf_SET(slice_1, pll_idx);
        if (reg_type == BarchettaRegisterTypeAN) {
            BCMI_BARCHETTA_SLICE_SLICE1r_REG_TYPE_SELf_SET(slice_1, 1);
        } else {
            BCMI_BARCHETTA_SLICE_SLICE1r_REG_TYPE_SELf_SET(slice_1, 0);
        }
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_SLICE_SLICE1r(&phy->access, slice_1));
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_reg_read(const plp_barchetta_phymod_access_t *pa, uint32_t address, uint32_t *data)
{
    uint16_t dev_addr = 0;
    dev_addr = (address >> 16) & 0x1F;
    if (((address & 0xD000) == 0xD000) || (dev_addr == 0)){ /* Adding dev type for PMD*/
        address |= (1 <<16);
    } else if (dev_addr != 1 && dev_addr != 7) {
        /* Invalid Device Address */
        *data = 0xFFFF;
        return PHYMOD_E_NONE;
    }
    
    PHYMOD_IF_ERR_RETURN(PHYMOD_BUS_READ(pa, address, data));
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_reg_write(const plp_barchetta_phymod_access_t *pa, uint32_t address, uint32_t data)
{
    uint16_t dev_addr = 0;
    dev_addr = (address >> 16) & 0x1F;
    /* If the dev address is invalid then ignore it and return success*/
    if (((address & 0xD000) == 0xD000) || (dev_addr == 0)){ /* Adding dev type for PMD*/
        address |= (1 <<16);
    } else if (dev_addr != 1 && dev_addr != 7) {
        /* For invalid Dev_address dont do anything return success*/
        return PHYMOD_E_NONE;
    }

    PHYMOD_IF_ERR_RETURN(PHYMOD_BUS_WRITE(pa, address, data));
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    To convert lane data rate into numeric value
 \param    lane_data_rate   [In]  Pointer to phy access structure
 \return   Return corresponding numeric value for the provided lane_data_rate
 \retval   PHYMOD_E_NONE, PHYMOD_E_PARAM
*******************************************************************************/
int __plp_barchetta_convert_lane_data_rate_to_numeric_value(
        barchetta_lane_data_rate_t lane_data_rate, uint32_t *lane_dr_idx)
{
    int barchetta_lane_datarate_list[BARCHETTA_MAX_NO_DR] = BARCHETTA_LANE_DATARATE_LIST_ELEMENTS;
    int min_index = 0, max_no_dr = 0, max_index = 0, search_dr = 0;
    uint8_t loop_idx = 0 ;
    max_index = max_no_dr = (sizeof(barchetta_lane_datarate_list)/sizeof(barchetta_lane_datarate_list[0])) - 1;

    for(loop_idx = 0; loop_idx < max_no_dr; loop_idx++)  {
        search_dr = barchetta_lane_datarate_list[((min_index + max_index)/2)];
        if (lane_data_rate == search_dr) {
            *lane_dr_idx = ((min_index + max_index)/2);
            break;
        } else if (lane_data_rate < search_dr) {
            max_index = ((min_index + max_index)/2) - 1;
        } else if (lane_data_rate > search_dr) {
            min_index = ((min_index + max_index)/2) + 1;
        }
        if (min_index > max_index) {
            PHYMOD_DEBUG_ERROR(("Invalid datarate%d\n", lane_data_rate ));
            return PHYMOD_E_PARAM;
        }
    }
    return PHYMOD_E_NONE;
}

/*****************************************************************************
 \brief    API to get the serdes config information like PLl Div, OSR etc
 \param    phy      [In]  Pointer to phy access structure
 \param    config   [In]  Pointer to phy interface config structure
 \param    config   [Out] Pointer to serdes config structure
 \return   Status information
 \retval   PHYMOD_E_NONE, PHYMOD_E_PARAM
*******************************************************************************/
int
__plp_barchetta_get_serdes_config_info(const plp_barchetta_phymod_phy_access_t *phy,
        const plp_barchetta_phymod_phy_inf_config_t *config,
        barchetta_serdes_config_info_t *serdes_cfg) {
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;
    uint32_t dr_idx = 0;
    uint32_t ref_clk_idx = 0;

    PHYMOD_IF_ERR_RETURN(
            __plp_barchetta_convert_lane_data_rate_to_numeric_value(aux_mode->lane_data_rate, &dr_idx));
    if (config->ref_clock ==  plp_barchetta_dr_table[dr_idx][BARCHETTA_REF_CLK_IDX]) {
        serdes_cfg->pll_div = plp_barchetta_dr_table[dr_idx][BARCHETTA_PLL_DIV_IDX];
        serdes_cfg->osr     = plp_barchetta_dr_table[dr_idx][BARCHETTA_OSR_IDX];
        serdes_cfg->refclk  = plp_barchetta_dr_table[dr_idx][BARCHETTA_BH_REF_CLK_IDX];
        return PHYMOD_E_NONE;
    } else {
        for (ref_clk_idx = 0; ref_clk_idx < BARCHETTA_MAX_REF_CLK_FOR_DR; ref_clk_idx++) {
            if ((plp_barchetta_dr_table[dr_idx + ref_clk_idx][BARCHETTA_LN_DR_IDX] == aux_mode->lane_data_rate) &&
                (config->ref_clock ==  plp_barchetta_dr_table[dr_idx + ref_clk_idx][BARCHETTA_REF_CLK_IDX])) {
                serdes_cfg->pll_div  = plp_barchetta_dr_table[dr_idx + ref_clk_idx][BARCHETTA_PLL_DIV_IDX];
                serdes_cfg->osr      = plp_barchetta_dr_table[dr_idx + ref_clk_idx][BARCHETTA_OSR_IDX];
                serdes_cfg->refclk   = plp_barchetta_dr_table[dr_idx + ref_clk_idx][BARCHETTA_BH_REF_CLK_IDX];
                return PHYMOD_E_NONE;
            }
        }
        if (ref_clk_idx == BARCHETTA_MAX_REF_CLK_FOR_DR) {
            for (ref_clk_idx = 0; ref_clk_idx < BARCHETTA_MAX_REF_CLK_FOR_DR; ref_clk_idx++) {
                if ((plp_barchetta_dr_table[dr_idx - ref_clk_idx][BARCHETTA_LN_DR_IDX] == aux_mode->lane_data_rate) &&
                    (config->ref_clock ==  plp_barchetta_dr_table[dr_idx - ref_clk_idx][BARCHETTA_REF_CLK_IDX])) {
                    serdes_cfg->pll_div     = plp_barchetta_dr_table[dr_idx - ref_clk_idx][BARCHETTA_PLL_DIV_IDX];
                    serdes_cfg->osr         = plp_barchetta_dr_table[dr_idx - ref_clk_idx ][BARCHETTA_OSR_IDX];
                    serdes_cfg->refclk      = plp_barchetta_dr_table[dr_idx - ref_clk_idx][BARCHETTA_BH_REF_CLK_IDX];
                    return PHYMOD_E_NONE;
                }
            }
        }
        if (ref_clk_idx == BARCHETTA_MAX_REF_CLK_FOR_DR) {
            PHYMOD_DEBUG_ERROR(("ERROR: Invalid ref clk for datarate:%d\n",aux_mode->lane_data_rate));
            return PHYMOD_E_PARAM;
        }
    }

    return PHYMOD_E_NONE;
}

#ifdef BCM_PLP_RMT_PHY_CAPABILITY
/*******************************************************************************
 PURPOSE: Zero initialization of Remote PHY related general control GPREG

 COMMENT:
 *******************************************************************************/
int
_barchetta_clear_remote_phy_registers(const plp_barchetta_phymod_phy_access_t* phy)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t offset = 0;

    /* Zero initialization of registers from address 0x18259 to 0x18266 */
    for(offset=0; offset<= 0x7; offset++) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, (BCMI_BARCHETTA_GEN_CNTRLS_GPREG_09r + offset),  0x0));
    }
    return PHYMOD_E_NONE;
}
#endif

/***************************************************************************//**
 \brief    To perform HARD or SOFT reset functionality
 \param    core        pointer to core access structure
           reset_mode  Reset mode (HARD/SOFT)
           direction   Not used
 \return   PHYMOD_E_NONE, PHYMOD_E_UNAVAIL
*******************************************************************************/
int
_plp_barchetta_core_reset_set(const plp_barchetta_phymod_core_access_t* core,
        plp_barchetta_phymod_reset_mode_t reset_mode, plp_barchetta_phymod_reset_direction_t direction) {
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_t gen_ctrl1;
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_t gen_ctrl2;
    BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_t     boot_por;
    const plp_barchetta_phymod_access_t *pa = &core->access;
    plp_barchetta_phymod_phy_init_config_t init_config ;
    uint32_t temp[9] = {0};
    uint32_t offset = 0;

    PHYMOD_MEMSET(&gen_ctrl1, 0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_t));

    switch (reset_mode) {
    case phymodResetModeHard:
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL1r(&core->access, &gen_ctrl1));
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_RESETBf_SET(gen_ctrl1, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, gen_ctrl1));
        break;
    case phymodResetModeSoft:
        PHYMOD_MEMSET(&boot_por,    0, sizeof(BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_t));
        PHYMOD_MEMSET(&gen_ctrl2,   0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_t));
        PHYMOD_MEMSET(&init_config, 0, sizeof(plp_barchetta_phymod_phy_init_config_t));

        PHYMOD_IF_ERR_RETURN(_plp_barchetta_phy_init((plp_barchetta_phymod_phy_access_t *)core, &init_config));

#ifdef BCM_PLP_RMT_PHY_CAPABILITY
        PHYMOD_IF_ERR_RETURN(_barchetta_clear_remote_phy_registers((plp_barchetta_phymod_phy_access_t *)core));
#endif
        /* Save MICRO_BOOT registers */
        for(offset=0; offset<=7; offset++) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, (BCMI_BARCHETTA_MICRO_BOOT_GPREG0_PORr + offset),  &temp[offset]));
        }
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr,  &temp[8]));

        /* Make serboot 0 */
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
        BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por, 0);
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));

        /* Assert the chip level soft reset */
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL1r(&core->access, &gen_ctrl1));
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(gen_ctrl1, 0);
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL1r(&core->access, gen_ctrl1));

        /* Restore MICRO_BOOT registers */
        for(offset=0; offset<=7; offset++) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, (BCMI_BARCHETTA_MICRO_BOOT_GPREG0_PORr + offset),  temp[offset]));
        }
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr,  temp[8]));

        /* Make serboot to 1 */
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
        BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por, 1);
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));

        /*Put master M0 peripheral and master M0 micro into reset*/
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL2r(&core->access, &gen_ctrl2));
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 0);
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL2r(&core->access, gen_ctrl2));

        /*Take master M0 peripheral out of reset*/
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL2r(&core->access, &gen_ctrl2));
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 1);
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL2r(&core->access, gen_ctrl2));

        /*Take master M0 micro out of reset*/
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL2r(&core->access, &gen_ctrl2));
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 1);
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL2r(&core->access, gen_ctrl2));

        PHYMOD_USLEEP(400000);
        break ;
    default:
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid reset mode\n"));
        return PHYMOD_E_UNAVAIL;
    }

    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    Wait Msg Out from Master Micro

 \param    acc          [In] Pointer to phymod access structure
 \param    exp_message  [In] Expected Message
 \param    flag_error   [In] If 1, flag error if received Msg is different than the expected.
 \param    poll_time    [In] Time between register reads in miliseconds

 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
plp_barchetta_wait_mst_msgout(const plp_barchetta_phymod_access_t *acc,
        uint16_t exp_message, int flag_error, int poll_time) {
    BARCHETTA_MESSAGE_TYPE_T msgout;
    int retry_count = BARCHETTA_MICRO_RETRY_COUNT * 10;
    BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_t msg_out;

    do {
        /* read register*/
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_MST_MSGOUTr(acc, &msg_out));
        msgout = BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET(msg_out);
        if (msgout != 0) {
            if (flag_error && (msgout != exp_message)) {
                PHYMOD_DEBUG_ERROR(
                        ("ERR Recived msgout = (0x%x), exp_message = 0x%x addr:%d)\n", msgout, exp_message, acc->addr));
                return PHYMOD_E_INTERNAL;
            } else {
                /*PHYMOD_DEBUG_INFO(("Recived msgout = (0x%x), exp_message = 0x%x)\n", msgout,  exp_message));*/
            }
        }
        /* wait before reading again */
        if (poll_time != 0) {
#ifdef SIM_VAL
            PHYMOD_USLEEP(5000);
            PHYMOD_DEBUG_INFO(("Retry:%d\n", retry_count));
#else
            PHYMOD_USLEEP(poll_time * 1000);
#endif
        }
    } while ((--retry_count) && (msgout != exp_message));

    if ((!retry_count) && (msgout != exp_message)) {
        PHYMOD_DEBUG_ERROR(
                ("ERROR: Recived msgout = (0x%x), exp_message = 0x%x) -- retry:%d\n", msgout, exp_message, retry_count));
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL,
                (_PHYMOD_MSG("Firmware download failed")));
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
static int
plp_barchetta_get_word_from_buffer(const uint8_t *buf, int index) {
    uint16_t word = buf[(index * 2) + 1];
    word <<= 8;
    return (word | buf[index * 2]);
}

/***************************************************************************//**
 \brief    Download and Fuse firmware.
           This function is used to download the firmware through I2C/MDIO
           and fuse it to SPI EEPROM if prg_eeprom flag is set
 \param    core_access      [In] Pointer to phymod access structure
 \param    barchetta_ucode  [In] Pointer to firmware array
 \param    fw_length        [In] Length of the firmware array
 \param    master_en        [In] Master enable
 \param    mst_boot_addr    [In] Master boot address
 \param    prg_eeprom       [In] program EEPROM
 \param    init_config      [In] Pointer to init_config

 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int
__plp_barchetta_download_prog_eeprom(const plp_barchetta_phymod_core_access_t *core_access,
        uint8_t *barchetta_ucode, uint32_t fw_length, uint16_t master_en,
        uint16_t mst_boot_addr, uint8_t prg_eeprom,
        const plp_barchetta_phymod_core_init_config_t *init_config) {
    int i, size0, size1 = 0, size2 = 0;
    uint16_t next_param = 0x1000;
    int retry_cnt = BARCHETTA_MICRO_RETRY_COUNT, data1 = 0;
    BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_t msg_in;
    BCMI_BARCHETTA_GEN_CNTRLS_SPI_CODE_LOAD_ENr_t spi_code_load_en;
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_t gen_ctrl3;
    BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_t boot_por;
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_t gen_ctrl2;
    BCMI_BARCHETTA_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t start_ptr;
    BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_t msg_out;
    const plp_barchetta_phymod_access_t *pa = &core_access->access;
    unsigned int trans_size = 0;
    bcm_plp_ext_fw_params_t ext_fw_params;
    BCMI_BARCHETTA_GEN_CNTRLS_GPREG_02r_t gpreg2;
    BCMI_BARCHETTA_GEN_CNTRLS_BOOTr_t boot;
    int dload_idx = 0;

    PHYMOD_MEMSET(&ext_fw_params,0,sizeof(ext_fw_params));
    PHYMOD_MEMSET(&boot_por,         0, sizeof(BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_t));
    PHYMOD_MEMSET(&msg_in,           0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_t));
    PHYMOD_MEMSET(&msg_out,          0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_t));
    PHYMOD_MEMSET(&gen_ctrl2,        0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_t));
    PHYMOD_MEMSET(&spi_code_load_en, 0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_SPI_CODE_LOAD_ENr_t));
    PHYMOD_MEMSET(&gen_ctrl3,        0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_t));
    PHYMOD_MEMSET(&start_ptr,        0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_t));
    PHYMOD_MEMSET(&gpreg2,           0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GPREG_02r_t));
    PHYMOD_MEMSET(&boot,             0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_BOOTr_t));

    if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
        PHYMOD_IF_ERR_RETURN(
            init_config->firmware_loader(NULL, &ext_fw_params));
        if (ext_fw_params.transfer_size < 32) {
            PHYMOD_DEBUG_ERROR(("ERROR: Transfer size cannot be less than 32 bytes"));
            return PHYMOD_E_PARAM;
        }
        if ((ext_fw_params.transfer_size % 2) != 0) {
            PHYMOD_DEBUG_ERROR(("ERROR: Transfer size cannot be Odd number\n"));
            return PHYMOD_E_PARAM;
        }

        if (ext_fw_params.firmware_address == NULL) {
            PHYMOD_DEBUG_ERROR(("ERROR: Invalid FW array\n"));
            return PHYMOD_E_PARAM;
        }
        if (ext_fw_params.fw_length == 0) {
            PHYMOD_DEBUG_ERROR(("ERROR: Invalid FW length\n"));
            return PHYMOD_E_PARAM;
        }
        if (ext_fw_params.transfer_size > ext_fw_params.fw_length) {
            PHYMOD_DEBUG_ERROR(("ERROR: Transfer size is greater than FW length\n"));
            return PHYMOD_E_PARAM;
        }
        fw_length = ext_fw_params.fw_length;
        trans_size = ext_fw_params.transfer_size;
    }

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
    if (BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SERBOOTf_GET(boot_por) &&
         BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_GET(boot_por)) {
        PHYMOD_USLEEP(100000);
        do {
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_BOOTr(pa, &boot));
            if (BCMI_BARCHETTA_GEN_CNTRLS_BOOTr_SERBOOT_BUSYf_GET(boot) == 0) {
                _PHYMOD_MSG("SERBOOT_BUSY is cleared.");
                break;
            }
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_MST_MSGOUTr(pa, &msg_out));
            if (BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET(msg_out) == BARCHETTA_PARAM_HDR_ERR) {
                _PHYMOD_MSG("MSGOUT return HDR_ERR while SERBOOT_BUSY is high.");

               // *******  start of additional fix  ******* 
                BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_SET(boot_por, 0);
                PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));
                
                {
                    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_t gen_ctrl1;
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL1r(pa, &gen_ctrl1));
                    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_SOFT_RSTBf_SET(gen_ctrl1, 0);
                    PHYMOD_IF_ERR_RETURN(                
                        BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL1r(pa, gen_ctrl1));
                    goto check_eeprom; // for_EEPROM_flahing_option_at_line_930;  This is needed as all the code in between are unnecessary, and the checks there could still fail.                    
                } 
                // *******  end of additional fix  ******* 		
                break;
            }

            PHYMOD_USLEEP(100000);
        } while (--retry_cnt);

        if (retry_cnt == 0) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_FAIL,
                          (_PHYMOD_MSG("ERR:SERBOOT BUSY BIT SET")));
        }	
    }
    retry_cnt = BARCHETTA_MICRO_RETRY_COUNT;
    /*set to 0 for mdio default download on reset*/
    BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por, 0);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));

    /*Put master M0 peripheral and master M0 micro into reset*/
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 0);
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 0);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    /* Set Download done as '0'*/
    PHYMOD_IF_ERR_RETURN(
         BCMI_BARCHETTA_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
    BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_SET(boot_por, 0);
    BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_SET(boot_por, 0);
    PHYMOD_IF_ERR_RETURN(
         BCMI_BARCHETTA_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));

    /*Take master M0 peripheral out of reset*/
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_UCP_RSTBf_SET(gen_ctrl2, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    /* Fix for FW download while SERBOOT pin=High */
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_MST_MSGOUTr(pa, &msg_out));

    /*Take master M0 micro out of reset*/
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    /* STEP 1: Program master enable, slave enable, broadcast enable bits */
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, &spi_code_load_en));
    BCMI_BARCHETTA_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_SET(spi_code_load_en, 1);
    BCMI_BARCHETTA_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_SET(spi_code_load_en, 0x3);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, spi_code_load_en));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, &spi_code_load_en));
    if ((BCMI_BARCHETTA_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SLV_CODE_DOWNLOAD_ENf_GET(spi_code_load_en)!= 0x3)||
        !(BCMI_BARCHETTA_GEN_CNTRLS_SPI_CODE_LOAD_ENr_MST_CODE_DOWNLOAD_ENf_GET(spi_code_load_en))) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_FAIL,
                (_PHYMOD_MSG("ERR: BROADCAST ENABLE IS NOT SET")));
    }

    if (prg_eeprom) {
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, &spi_code_load_en));
        BCMI_BARCHETTA_GEN_CNTRLS_SPI_CODE_LOAD_ENr_SPI_MST_OEBf_SET(spi_code_load_en, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_SPI_CODE_LOAD_ENr(pa, spi_code_load_en));

        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL3r(pa, &gen_ctrl3));
        /* Select SPI Speed*/
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_UCSPI_SLOWf_SET(gen_ctrl3, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL3r(pa, gen_ctrl3));
    }
#ifdef SIM_VAL
    PHYMOD_IF_ERR_RETURN(
            plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_NOT_DWNLD, 1, 2000));
#else
    PHYMOD_IF_ERR_RETURN(
            plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_NOT_DWNLD, 1, 0));

#endif

    PHYMOD_DEBUG_INFO(("FW Download preparation Completed...\n"));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
    /* Force Master New Download */
    if (BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_GET(boot_por)) {
        PHYMOD_DEBUG_ERROR(("ERROR: MST dload done:%x\n",
                  BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_MST_DWLD_DONEf_GET(boot_por)));
        return PHYMOD_E_INTERNAL;
    }
    if (BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_GET(boot_por)) {
        PHYMOD_DEBUG_ERROR(("ERROR: Slave dload done:%x\n",
                  BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SLV_DWLD_DONEf_GET(boot_por)));
        return PHYMOD_E_INTERNAL;
    }

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
    /*set to 0 for mdio default download on reset*/
    BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SPI_PORT_USEDf_SET(boot_por, 0);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_MICRO_BOOT_BOOT_PORr(pa, &boot_por));
    BCMI_BARCHETTA_MICRO_BOOT_BOOT_PORr_SERBOOTf_SET(boot_por, 1);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_MICRO_BOOT_BOOT_PORr(pa, boot_por));

    BCMI_BARCHETTA_GEN_CNTRLS_GPREG_02r_CLR(gpreg2);
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GPREG_02r(pa, gpreg2));

    /* STEP 1: Put Master under Reset
     BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_CLR(gen_ctrl2);
     PHYMOD_IF_ERR_RETURN(
     BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));*/

    BCMI_BARCHETTA_GEN_CNTRLS_SPI_MST_CODE_START_PTRr_SET(start_ptr, mst_boot_addr);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_GEN_CNTRLS_SPI_MST_CODE_START_PTRr(pa, start_ptr));

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 0);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

#ifdef SIM_VAL
    PHYMOD_USLEEP(300);
#else
    PHYMOD_USLEEP(10000);
#endif

    /* Dummy read needed to MSGOUT register to make sure bootloader clears out 0x101 value
       and set either 0x0 or 0xF1AC depending on the bootloader execution completion
    */
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_MST_MSGOUTr(pa, &msg_out));

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL2r(pa, &gen_ctrl2));
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL2r_MST_RSTBf_SET(gen_ctrl2, 1);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL2r(pa, gen_ctrl2));

    /*EEPROM*/
check_eeprom:    
    if (prg_eeprom) {
        PHYMOD_DEBUG_INFO(("Enabling EEPROM\n"));
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_FLASH, 1, 2000));
#else
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_FLASH, 1, 1));
#endif
        BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 1);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in)); /* Flashing Enable*/

#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_FLASH, 1, 2000));
#else
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_FLASH, 1, 0));
#endif
        /* Send write delay*/
        BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_FLASH, 1, 2000));
#else
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_FLASH, 1, 0));
#endif
        /* Send the debug mode*/
        BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
    } else {
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_FLASH, 1, 2000));
#else
        /* Checking for flash enable*/
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_FLASH, 1, 1));
#endif
        BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_SET(msg_in, 0);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in)); /* Flashing Disable*/
    }
#ifdef SIM_VAL
    PHYMOD_IF_ERR_RETURN(
            plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_HEAD, 1, 2000));
#else

    PHYMOD_IF_ERR_RETURN(
            plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_HEAD, 1, 0));
#endif
    BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_SET(msg_in, mst_boot_addr);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));

    if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
        size0 = (((plp_barchetta_get_word_from_buffer(ext_fw_params.firmware_address, 14) & 0xF) << 16)
                | plp_barchetta_get_word_from_buffer(ext_fw_params.firmware_address, 13));
        size1 = (((plp_barchetta_get_word_from_buffer(ext_fw_params.firmware_address, 20) & 0xF) << 16)
                | plp_barchetta_get_word_from_buffer(ext_fw_params.firmware_address, 19));
        size2 = (((plp_barchetta_get_word_from_buffer(ext_fw_params.firmware_address, 26) & 0xF) << 16)
                | plp_barchetta_get_word_from_buffer(ext_fw_params.firmware_address, 25));
    } else {
    size0 = (((plp_barchetta_get_word_from_buffer(barchetta_ucode, 14) & 0xF) << 16)
            | plp_barchetta_get_word_from_buffer(barchetta_ucode, 13));
    size1 = (((plp_barchetta_get_word_from_buffer(barchetta_ucode, 20) & 0xF) << 16)
            | plp_barchetta_get_word_from_buffer(barchetta_ucode, 19));
    size2 = (((plp_barchetta_get_word_from_buffer(barchetta_ucode, 26) & 0xF) << 16)
            | plp_barchetta_get_word_from_buffer(barchetta_ucode, 25));
    }

    /*adjusted to be multiple of 64B*/
    size0 = (size0 % BARCHETTA_HEADER_WORDS) ?
            ((size0 / BARCHETTA_HEADER_WORDS) + 1) * BARCHETTA_HEADER_WORDS : size0;
    size1 = (size1 % BARCHETTA_HEADER_WORDS) ?
            ((size1 / BARCHETTA_HEADER_WORDS) + 1) * BARCHETTA_HEADER_WORDS : size1;
    size2 = (size2 % BARCHETTA_HEADER_WORDS) ?
            ((size2 / BARCHETTA_HEADER_WORDS) + 1) * BARCHETTA_HEADER_WORDS : size2;

    PHYMOD_DEBUG_INFO(("FW Header Download Started...\n"));
    for (i = 0; i < BARCHETTA_HEADER_WORDS; i++) {
#ifdef SIM_VAL
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_HEAD, 1, 2000));
#else
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_HEAD, 1, 0));
#endif
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
            BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_SET(msg_in,
                    plp_barchetta_get_word_from_buffer(ext_fw_params.firmware_address, dload_idx));
            dload_idx++;
        } else {
            BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_SET(msg_in,
                plp_barchetta_get_word_from_buffer(barchetta_ucode, i));
        }
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
            trans_size -= 2;
            if (trans_size == 0) {
                PHYMOD_IF_ERR_RETURN(
                    init_config->firmware_loader(NULL, &ext_fw_params));
                if (ext_fw_params.transfer_size < 32) {
                    PHYMOD_DEBUG_ERROR(("ERROR: Transfer size cannot be less than 32 bytes"));
                    return PHYMOD_E_PARAM;
                }
                trans_size = ext_fw_params.transfer_size;
                dload_idx =0;
            }
        }
    }
    PHYMOD_DEBUG_INFO(("FW Header Download Completed...\n"));
    do {
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_MST_MSGOUTr(pa, &msg_out));
        if (BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET(msg_out) == BARCHETTA_PARAM_HDR_ERR) {
            PHYMOD_DEBUG_ERROR(("ERROR: PARAM_HDR_ERR occured\n"));
            return PHYMOD_E_INTERNAL;
        }
#ifdef SIM_VAL
        PHYMOD_USLEEP(9000);
#else
        PHYMOD_USLEEP(10000);
#endif
    } while (((BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET(msg_out)!= 0x1000)) &&
               --retry_cnt);
    if (retry_cnt <= 0) {
        PHYMOD_DEBUG_ERROR(("ERROR: NOt received param next\n"));
        return PHYMOD_E_INTERNAL;
    }

    PHYMOD_DEBUG_INFO(("FW MST/SLV Download Started...\n"));
    for (i = BARCHETTA_HEADER_WORDS; i < (fw_length / 2); i++) {
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
            if (trans_size == 0) {
                PHYMOD_IF_ERR_RETURN(
                    init_config->firmware_loader(NULL, &ext_fw_params));
                trans_size = ext_fw_params.transfer_size;
                dload_idx = 0;
            }
            BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_SET(msg_in,
                    plp_barchetta_get_word_from_buffer(ext_fw_params.firmware_address, dload_idx));
            dload_idx ++;
        } else {
        BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGINr_SET(msg_in,
                plp_barchetta_get_word_from_buffer(barchetta_ucode, i));
        }
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MST_MSGINr(pa, msg_in));

        if (i != ((fw_length / 2) - 1)) {
            if ((i - 63) == (size0 / 2)) {
                next_param = 0x2000;
            } else if ((i - 63) == ((size0 + size1) / 2)) {
                next_param = 0x3000;
            } else if (!((i + 1) % 32)) {
                next_param++;
            }
#ifdef SIM_VAL
            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_wait_mst_msgout(pa, next_param, 1, 2000));
#else
            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_wait_mst_msgout(pa, next_param, 1, 0));
#endif
        }
        if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
            trans_size -= 2;
        }
    }
#ifdef SIM_VAL
    PHYMOD_IF_ERR_RETURN(
            plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_DWNLD_DONE, 1, 2000));
#else
    PHYMOD_IF_ERR_RETURN(
            plp_barchetta_wait_mst_msgout(pa, BARCHETTA_PARAM_DWNLD_DONE, 1, 1));
#endif

    PHYMOD_DEBUG_INFO(("FW MST/SLV Download Completed...\n"));

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_check_fw_download_status(const plp_barchetta_phymod_core_access_t *core_access,
        plp_barchetta_phymod_firmware_load_method_t load_method) {
    uint16_t n_img = 0, no_of_img = 0, retry_cnt = BARCHETTA_MICRO_RETRY_COUNT;
    uint32_t data;
    uint32_t live_CRC, precalc_CRC;
    /*uint16_t d_sts =(load_method == phymodFirmwareLoadMethodProgEEPROM) ?
                    0x0404 : 0x0303;
    BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_t msg_out;
    BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_00r_t dload0;*/
    BCMI_BARCHETTA_GEN_CNTRLS_FIRMWARE_VERSIONr_t fw_ver;
    BCMI_BARCHETTA_GEN_CNTRLS_GPREG_01r_t gpreg_1;
    BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_01r_t dwnld_01;
    BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_03r_t dwnld_03;

    PHYMOD_MEMSET(&gpreg_1, 0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GPREG_01r_t));
    PHYMOD_MEMSET(&dwnld_01, 0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_01r_t));
    PHYMOD_MEMSET(&dwnld_03, 0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_03r_t));

#ifndef DUMMY_FW
    /*PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_MST_MSGOUTr(&core_access->access, &msg_out));
    if (BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET(msg_out)!= d_sts) {
        PHYMOD_DEBUG_ERROR(
                ("Unexpected FW Status 0x%x--%x\n",
                  d_sts, BCMI_BARCHETTA_GEN_CNTRLS_MST_MSGOUTr_MST_MSGOUT_VALf_GET(msg_out)));
        return PHYMOD_E_INTERNAL;
    }

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_DWNLD_00r(&core_access->access, &dload0));
    n_img = BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_00r_DOWNLOAD_00f_GET(dload0);
    n_img = plp_barchetta_plp_barchetta_ucode_bin[16]; */
    n_img = 6; /* there could be maximum 6 images in Barchetta */
    for (no_of_img = 0; no_of_img < n_img; ++no_of_img) {
        PHYMOD_IF_ERR_RETURN(
              _plp_barchetta_reg_read(&core_access->access, (BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_00r + (no_of_img*2)), &data));
        if (data == 0) break; /* all images have been checked, leaving loop */
        if (data != 0x600D) {
            PHYMOD_IF_ERR_RETURN(
                  _plp_barchetta_reg_read(&core_access->access, (BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_00r + (no_of_img*2) + 1), &live_CRC));
            PHYMOD_IF_ERR_RETURN(
                  _plp_barchetta_reg_read(&core_access->access, (BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_10r + (no_of_img*2) + 1), &precalc_CRC));
            if (live_CRC == precalc_CRC) continue;  /* This 0xBADD is a false alarm */
            PHYMOD_DEBUG_ERROR(("ERROR: Image Dload status not correct for image:%d\n", no_of_img));
            return PHYMOD_E_INTERNAL;
        } else { /* (data == 0x600D) */
            continue; /* for next image */
        }
    }
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_FIRMWARE_VERSIONr(&core_access->access, &fw_ver));

    do {
        PHYMOD_USLEEP(5000);
        PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_GPREG_01r(&core_access->access, &gpreg_1));

        if (BCMI_BARCHETTA_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1)& 1) {
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_DWNLD_03r(&core_access->access, &dwnld_03));
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_GEN_CNTRLS_DWNLD_01r(&core_access->access, &dwnld_01));
            PHYMOD_CRIT_INFO(
                    ("FW download success. FW ver:0x%x. CRC0:%x CRC1:%x\n",
                      BCMI_BARCHETTA_GEN_CNTRLS_FIRMWARE_VERSIONr_FIRMWARE_VERSION_VALf_GET(fw_ver),
                      BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_01r_DOWNLOAD_01f_GET(dwnld_01),
                      BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_03r_DOWNLOAD_03f_GET(dwnld_03)));
            return PHYMOD_E_NONE;
        }
    } while (--retry_cnt);
#else
    (void)data ;
    (void)live_CRC ;
    (void)precalc_CRC ;
    BCMI_BARCHETTA_GEN_CNTRLS_GPREG_00r_GPREG_00_DATAf_SET(gpreg_0, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GPREG_00r(&core_access->access, gpreg_0));
    do {
        BCMI_BARCHETTA_GEN_CNTRLS_GPREG_01r_t gpreg1;
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GPREG_01r(&core_access->access, &gpreg1));
        if (BCMI_BARCHETTA_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg1) == 0xAAAA) {
            break;
        }
        PHYMOD_USLEEP(5000);
    }while(1);

    BCMI_BARCHETTA_GEN_CNTRLS_GPREG_00r_GPREG_00_DATAf_SET(gpreg_0, 2);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GPREG_00r(&core_access->access, gpreg_0));
    do {
        BCMI_BARCHETTA_GEN_CNTRLS_GPREG_01r_t gpreg1;
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GPREG_01r(&core_access->access, &gpreg1));
        if (BCMI_BARCHETTA_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg1) == 0xBBBB) {
            return PHYMOD_E_NONE;
        }
        PHYMOD_USLEEP(5000);
    }while(1);
#endif
    PHYMOD_DEBUG_ERROR(
            ("ERROR: FW download Failure. Gpreg1:%x\n",
              BCMI_BARCHETTA_GEN_CNTRLS_GPREG_01r_GPREG_01_DATAf_GET(gpreg_1)));

    return PHYMOD_E_INTERNAL;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_dload_fw(const plp_barchetta_phymod_core_access_t* core,
        const plp_barchetta_phymod_core_init_config_t *init_config) {
    int rv = 0;
    int mst_boot_addr = 0, master_en = 1;
    int chip_id = 0;
    uint32_t chip_rev = 0;

    PHYMOD_IF_ERR_RETURN(
             _plp_barchetta_get_chip_id(&core->access, &chip_id));
    if (((chip_id == BARCHETTA_CHIP_81337) || (chip_id == BARCHETTA_CHIP_81338)) &&
            (init_config->firmware_load_method == phymodFirmwareLoadMethodProgEEPROM)) {
        PHYMOD_DEBUG_ERROR(("ERROR: EEPROM load method\n"));
        return PHYMOD_E_PARAM;
    }

    if (init_config->firmware_load_method == phymodFirmwareLoadMethodExternal) {
        rv = __plp_barchetta_download_prog_eeprom(core, NULL,
             0, master_en, mst_boot_addr,
            (init_config->firmware_load_method == phymodFirmwareLoadMethodProgEEPROM) ? 1 : 0,
            init_config);
    } else {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_internal_chip_rev(&core->access, &chip_rev));
        if(chip_rev == BARCHETTA_CHIP_REV_INTERNAL) {
            rv = __plp_barchetta_download_prog_eeprom(core, plp_barchetta_plp_barchetta_b0_ucode_bin,
                 plp_barchetta_plp_barchetta_b0_ucode_len, master_en, mst_boot_addr,
                 (init_config->firmware_load_method == phymodFirmwareLoadMethodProgEEPROM) ? 1 : 0,
                 init_config);
        } else {
            rv = __plp_barchetta_download_prog_eeprom(core, plp_barchetta_plp_barchetta_ucode_bin,
                 plp_barchetta_plp_barchetta_ucode_len, master_en, mst_boot_addr,
                 (init_config->firmware_load_method == phymodFirmwareLoadMethodProgEEPROM) ? 1 : 0,
                 init_config);
        }
    }
    if ((rv != PHYMOD_E_NONE) && (rv != BARCHETTA_FW_ALREADY_DOWNLOADED)) {
        PHYMOD_DEBUG_ERROR(("ERROR: Firmware download failed\n"));
        return PHYMOD_E_FAIL;
    }

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_core_firmware_info_get(const plp_barchetta_phymod_core_access_t* core,
        plp_barchetta_phymod_core_firmware_info_t* fw_info) {
    BCMI_BARCHETTA_GEN_CNTRLS_FIRMWARE_VERSIONr_t fw_ver;
    uint32_t data = 0;

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_reg_read(&core->access, BCMI_BARCHETTA_GEN_CNTRLS_DWNLD_01r, &data));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_GEN_CNTRLS_FIRMWARE_VERSIONr(&core->access, &fw_ver));

    fw_info->fw_crc = data;
    fw_info->fw_version = fw_ver.v[0];

    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to find the lane map of the allocated port
 \param    port_cfg      [In]  Pointer to port config structure
 \param    lane_map_cfg  [Out] Pointer to lane map config structure
 \return   Pointer to the lane map config structure
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_get_lane_map(barchetta_port_config_t *port_cfg,
        barchetta_lane_map_t *lane_map_cfg) {
    uint8_t i;
    uint8_t port_type;

    /* Retrieve port direction from PortType */
    port_type = port_cfg->port_type;

    if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        barchetta_duplex_port_lanes_t *p_duplex_port_lanes = &(port_cfg->port_lanes.duplex);
        lane_map_cfg->lane_map.duplex_lane_map.sys_lane_map = 0;
        lane_map_cfg->lane_map.duplex_lane_map.line_lane_map = 0;
        for (i = 0; i < p_duplex_port_lanes->num_sys_lanes; i++) {
            m_BARCHETTA_SET_BIT((lane_map_cfg->lane_map.duplex_lane_map.sys_lane_map),
                                (p_duplex_port_lanes->sys_lane_list[i]));
        }
        for (i = 0; i < p_duplex_port_lanes->num_line_lanes; i++) {
            m_BARCHETTA_SET_BIT((lane_map_cfg->lane_map.duplex_lane_map.line_lane_map),
                                (p_duplex_port_lanes->line_lane_list[i]));
        }
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief   To save the status information stored in software database into GPREG
 \param    pa        [In] Pointer to phymod access structure
 \param    port_num  [In] Port number
 \param    reg_addr  [In] Register address
 \param    status    [Out] Pointer to the status information
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_save_sw_db_status_info(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num, uint32_t reg_addr, uint8_t status) {
    uint32_t status_all_ports = 0 ;
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, reg_addr, &status_all_ports));
    status_all_ports = ((status_all_ports & (unsigned int)(~(1<<port_num))) | (unsigned int)(status<<port_num)) ;
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, reg_addr, status_all_ports));
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    To restore status information stored in software database
 \param    pa        [In] Pointer to phymod access structure
 \param    port_num  [In] Port number
 \param    reg_addr  [In] Register address
 \param    status    [Out] Pointer to the status information
 \return   Status information
 \retval PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_restore_sw_db_status_info(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num, uint32_t reg_addr, uint8_t *status) {
    uint32_t status_all_ports = 0 ;
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, reg_addr, &status_all_ports));
    *status = ((status_all_ports & (0x01<<port_num)) != 0) ;
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int _plp_barchetta_warmboot_init(const plp_barchetta_phymod_core_access_t* core, void* init_data)
{
    plp_barchetta_phymod_phy_access_t      *phy  = (plp_barchetta_phymod_phy_access_t *)core ;
    const plp_barchetta_phymod_access_t    *pa = &core->access;
    barchetta_pmd_config_t   *p_pmd_config_sys = NULL ;
    barchetta_config_lanes_t *p_config_lanes   = NULL ;
    barchetta_aux_modes_t    *p_aux_mode_sys   = NULL ;
    barchetta_port_config_t  *p_port_config    = NULL ;
    barchetta_lane_map_t     *p_lane_map       = NULL ;
    barchetta_aux_modes_t    *p_aux_modes      = NULL ;
    uint32_t  *p_primary_lane_map_sys          = NULL ;
    uint8_t   *p_lanes_cfg_status_sys          = NULL ;
    uint8_t   *p_port_cfg_aux_status_sys       = NULL ;
    uint8_t   *p_pmd_cfg_status_sys            = NULL ;
    uint8_t   *p_port_status                   = NULL ;

    plp_barchetta_phymod_phy_inf_config_t inf_config ;
    barchetta_config_pmd_t  config_pmd ;
    uint32_t rx_osr   = 0 ;
    uint8_t  port_num = 0 ;
    phy->port_loc = phymodPortLocSys ;

    PHYMOD_MEMSET(&inf_config, 0, sizeof(plp_barchetta_phymod_phy_inf_config_t));
    PHYMOD_MEMSET(&config_pmd, 0, sizeof(barchetta_config_pmd_t));
    PHYMOD_MEMSET(&plp_barchetta_allocated_port_list[pa->addr], 0xFF, sizeof(uint8_t)*BARCHETTA_NUM_OF_PORTS);
    plp_barchetta_unallocated_port_list[pa->addr][0] = 0x0;
    plp_barchetta_unallocated_port_list[pa->addr][1] = 0x1;
    plp_barchetta_unallocated_port_list[pa->addr][2] = 0x2;
    plp_barchetta_unallocated_port_list[pa->addr][3] = 0x3;
    plp_barchetta_unallocated_port_list[pa->addr][4] = 0x4;
    plp_barchetta_unallocated_port_list[pa->addr][5] = 0x5;
    plp_barchetta_unallocated_port_list[pa->addr][6] = 0x6;
    plp_barchetta_unallocated_port_list[pa->addr][7] = 0x7;

    p_config_lanes     = &(plp_barchetta_sw_db[pa->addr][0].lanes_cfg_sys);
    p_pmd_config_sys   = &(plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys);
    p_aux_mode_sys     = &(plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys);
    p_primary_lane_map_sys    = &(plp_barchetta_sw_db[pa->addr][0].sys_primary_port_lane_map);
    p_lanes_cfg_status_sys    = &(plp_barchetta_sw_db[pa->addr][0].lanes_cfg_sys_status);
    p_port_cfg_aux_status_sys = &(plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys_status);
    p_pmd_cfg_status_sys      = &(plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys_status);

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_msg_config_lanes_rd(pa, p_config_lanes));

    for(port_num = 0; port_num < BARCHETTA_NUM_OF_PORTS; port_num++) {
        p_port_config      = &(plp_barchetta_sw_db[pa->addr][port_num].port_cfg_info);
        p_lane_map         = &(plp_barchetta_sw_db[pa->addr][port_num].lane_map_info);
        p_port_status      = &(plp_barchetta_sw_db[pa->addr][port_num].port_status);

        PHYMOD_IF_ERR_RETURN(__plp_barchetta_restore_sw_db_status_info(pa, port_num, BARCHETTA_SW_DB_PORT_STATUS_REG_ADDR, p_port_status));
        if(*p_port_status == BARCHETTA_PORT_ALLOCATED) {
            plp_barchetta_allocated_port_list[pa->addr][port_num] = port_num ;
            plp_barchetta_unallocated_port_list[pa->addr][port_num] = 0xFF ;
            inf_config.device_aux_modes = (void *)p_aux_mode_sys ;
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_phy_interface_config_get(phy, 0, &inf_config));
            p_aux_modes = (barchetta_aux_modes_t *)inf_config.device_aux_modes ;

            PHYMOD_IF_ERR_RETURN(_plp_barchetta_msg_config_pmd_rd(pa, port_num, BARCHETTA_SIDE_SYS, &config_pmd));

            p_pmd_config_sys->clock_mode = p_aux_modes->clock_mode ;
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_pll_index(phy, &(p_pmd_config_sys->pll_sel)));

            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(&phy->access, 0x1d1c0, &rx_osr));

            p_pmd_config_sys->osr              = (rx_osr & 0xFF) ;
            p_pmd_config_sys->an_opt           = config_pmd.an_opt ;
            p_pmd_config_sys->an_use_pcs_mon   = config_pmd.an_use_pcs_mon ;
            p_pmd_config_sys->an_master_lane   = config_pmd.an_master_lane ;
            p_pmd_config_sys->tx_training_opt  = config_pmd.tx_training_opt ;
            p_pmd_config_sys->fec_opt          = config_pmd.fec_opt ;
            p_pmd_config_sys->lane_config_word = config_pmd.lane_config_word ;
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_msg_config_port_rd(pa, port_num, p_port_config));

            PHYMOD_IF_ERR_RETURN(__plp_barchetta_get_lane_map(p_port_config, p_lane_map));

            *p_primary_lane_map_sys = p_lane_map->lane_map.duplex_lane_map.sys_lane_map ;
        }
    }
    PHYMOD_IF_ERR_RETURN(__plp_barchetta_restore_sw_db_status_info(pa, 0, BARCHETTA_SW_DB_LANES_CONFIG_SYS_STATUS_REG_ADDR, p_lanes_cfg_status_sys));
    PHYMOD_IF_ERR_RETURN(__plp_barchetta_restore_sw_db_status_info(pa, 0, BARCHETTA_SW_DB_PORT_CONFIG_SYS_STATUS_REG_ADDR, p_port_cfg_aux_status_sys));
    PHYMOD_IF_ERR_RETURN(__plp_barchetta_restore_sw_db_status_info(pa, 0, BARCHETTA_SW_DB_PMD_CONFIG_SYS_STATUS_REG_ADDR, p_pmd_cfg_status_sys));

    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to deallocate the resources of the specified port and
           clear the software database information of the specified port
 \param    pa         [In]Pointer to phymod access structure
 \param    port       [In]Port which needs to be deallocated and needs the clearing of SW database
 \param    idx        [In]Index required for updating allocated and unallocated port list
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_deallocate_port_and_clear_sw_database(
        const plp_barchetta_phymod_access_t *pa, uint8_t port) {
    uint8_t i;
    uint8_t port_type = 0;

    if(port >= BARCHETTA_NUM_OF_PORTS) {
        PHYMOD_DEBUG_ERROR(("ERROR: Port %d is invalid.\n", port));
        return PHYMOD_E_INTERNAL;
    }

    port_type = (plp_barchetta_sw_db[pa->addr][port].port_cfg_info.port_type);
    if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        barchetta_duplex_port_lanes_t *p_duplex_port_lanes = &(plp_barchetta_sw_db[pa->addr][port].port_cfg_info.port_lanes.duplex);
        barchetta_duplex_lane_map_t *p_duplex_lane_map = &(plp_barchetta_sw_db[pa->addr][port].lane_map_info.lane_map.duplex_lane_map);

        /* Clear the system_lanes_list */
        for (i = 0; i < p_duplex_port_lanes->num_sys_lanes; i++) {
            p_duplex_port_lanes->sys_lane_list[i] = 0xFF;
        }
        /* Clear number of sys lanes */
        p_duplex_port_lanes->num_sys_lanes = 0x0;

        /* Clear the line_lanes_list */
        for (i = 0; i < p_duplex_port_lanes->num_line_lanes; i++) {
            p_duplex_port_lanes->line_lane_list[i] = 0xFF;
        }
        /* Clear number of line lanes */
        p_duplex_port_lanes->num_line_lanes = 0x0;

        /* Clear duplex lane maps */
        p_duplex_lane_map->sys_lane_map = 0x00;
        p_duplex_lane_map->line_lane_map = 0x00;
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        
    }

    /* Clear the port type */
    plp_barchetta_sw_db[pa->addr][port].port_cfg_info.port_type = 0x00;

    /* Clear the port mode */
    plp_barchetta_sw_db[pa->addr][port].port_cfg_info.port_mode = 0x00;

    /* Make port status as unallocated */
    plp_barchetta_sw_db[pa->addr][port].port_status = BARCHETTA_PORT_UNALLOCATED;
    PHYMOD_IF_ERR_RETURN(
           __plp_barchetta_save_sw_db_status_info(pa, port, BARCHETTA_SW_DB_PORT_STATUS_REG_ADDR, BARCHETTA_PORT_UNALLOCATED)) ;

    /* Clear the port_num */
    plp_barchetta_sw_db[pa->addr][port].port_cfg_info.port_num = 0xFF;

    /* Update allocated and unallocated port list */
    plp_barchetta_unallocated_port_list[pa->addr][port] = port;
    plp_barchetta_allocated_port_list[pa->addr][port] = 0xFF;

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
static int
__plp_barchetta_remove_associated_pll_lanes(const plp_barchetta_phymod_phy_access_t *phy, uint8_t port_num)
{
    BCMI_BARCHETTA_CTRL_SWGPREG4r_t pll_0_associated_lanes;
    BCMI_BARCHETTA_CTRL_SWGPREG6r_t pll_1_associated_lanes;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_port_config_t *p_port_config = NULL ;
    barchetta_lane_map_t *p_lane_map = NULL ;
    uint32_t sys_line_lane_map = 0;

    p_port_config = &(plp_barchetta_sw_db[pa->addr][port_num].port_cfg_info);
    p_lane_map = &(plp_barchetta_sw_db[pa->addr][port_num].lane_map_info);

    PHYMOD_MEMSET(&pll_0_associated_lanes, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG4r_t));
    PHYMOD_MEMSET(&pll_1_associated_lanes, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG6r_t));

    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(pa, &pll_0_associated_lanes));
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(pa, &pll_1_associated_lanes));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_msg_config_port_rd(pa, port_num, p_port_config));
    PHYMOD_IF_ERR_RETURN(__plp_barchetta_get_lane_map(p_port_config, p_lane_map));

    sys_line_lane_map = (((p_lane_map->lane_map.duplex_lane_map.sys_lane_map & 0xFF) << 8) | (p_lane_map->lane_map.duplex_lane_map.line_lane_map & 0xFF));

    pll_0_associated_lanes.v[0] &= ~sys_line_lane_map ;
    pll_1_associated_lanes.v[0] &= ~sys_line_lane_map ;

    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(pa, pll_0_associated_lanes));
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(pa, pll_1_associated_lanes));

    return PHYMOD_E_NONE;
}
/***************************************************************************//**
 \brief    API to allocate and configure the port using message interface and
           update the sw database if port configuration successful.
           Also it updates the allocated and unallocated port list
 \param    phy        [In]Pointer to phy access structure
 \param    port_cfg   [In]Pointer to the port configuration structure
 \param    port       [In]Port which is going to be allocated and configured
 \param    idx        [In]Index required for updating allocated and unallocated port list
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_allocate_port_and_update_sw_database(
        const plp_barchetta_phymod_phy_access_t *phy, barchetta_port_config_t *port_cfg,
        const plp_barchetta_phymod_phy_inf_config_t *config, barchetta_serdes_config_info_t serdes_cfg, uint8_t port) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    plp_barchetta_phymod_phy_access_t phy_temp;
    barchetta_port_config_t port_cfg_rd;
    uint32_t chip_rev = 0;
    uint8_t port_type = 0;
    uint8_t i = 0;

    if(port >= BARCHETTA_NUM_OF_PORTS) {
        PHYMOD_DEBUG_ERROR(("ERROR: Port %d is invalid.\n", port));
        return PHYMOD_E_INTERNAL;
    }

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_internal_chip_rev(&phy->access, &chip_rev));

    if(plp_barchetta_sw_db[pa->addr][port].port_status == BARCHETTA_PORT_ALLOCATED) {
        if(chip_rev == BARCHETTA_CHIP_REV_INTERNAL) {
            PHYMOD_IF_ERR_RETURN(__plp_barchetta_remove_associated_pll_lanes(phy, port));
        }
        /* Disable the port through message interface */
        if (BARCHETTA_MSG_IF_RET_SUCCESS == _plp_barchetta_msg_disable_port(pa, port)) {
            /* Clear the SW database for respective port and update the allocated and unallocated port list. */
            PHYMOD_IF_ERR_RETURN(
            __plp_barchetta_deallocate_port_and_clear_sw_database(pa, port));
        } else {
            PHYMOD_DEBUG_ERROR(
                    ("ERROR: Port %d Disable Failed Due to DISABLE_PORT Error.\n", port));
            return PHYMOD_E_INTERNAL;
        }
    }
    port_cfg->port_num = port;

    if ((BARCHETTA_MSG_IF_RET_SUCCESS == _plp_barchetta_msg_config_port_wr(&phy->access, port_cfg))&&
        (plp_barchetta_sw_db[pa->addr][port].port_status != BARCHETTA_PORT_ALLOCATED)) {
        if (BARCHETTA_MSG_IF_RET_SUCCESS == _plp_barchetta_msg_config_port_rd(&phy->access, port,&port_cfg_rd)) {
            barchetta_lane_map_t lane_map_cfg;
            PHYMOD_IF_ERR_RETURN(
                    __plp_barchetta_get_lane_map(&port_cfg_rd, &lane_map_cfg));

            port_type = (port_cfg_rd.port_type);
            if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
                barchetta_duplex_port_lanes_t *p_duplex_port_lanes_sw_db =
                        &(plp_barchetta_sw_db[pa->addr][port].port_cfg_info.port_lanes.duplex);
                barchetta_duplex_lane_map_t *p_duplex_lane_map_sw_db =
                        &(plp_barchetta_sw_db[pa->addr][port].lane_map_info.lane_map.duplex_lane_map);
                barchetta_duplex_port_lanes_t *p_duplex_lane_cfg_rd =
                        &(port_cfg_rd.port_lanes.duplex);
                barchetta_duplex_lane_map_t *p_duplex_lane_map_rd =
                        &(lane_map_cfg.lane_map.duplex_lane_map);

                /* Update number of sys lanes */
                p_duplex_port_lanes_sw_db->num_sys_lanes =
                        p_duplex_lane_cfg_rd->num_sys_lanes;
                /* Update the sys_lanes_list */
                for (i = 0; i < p_duplex_lane_cfg_rd->num_sys_lanes; i++) {
                    p_duplex_port_lanes_sw_db->sys_lane_list[i] =
                            p_duplex_lane_cfg_rd->sys_lane_list[i];
                }

                /* Update number of line lanes */
                p_duplex_port_lanes_sw_db->num_line_lanes =
                        p_duplex_lane_cfg_rd->num_line_lanes;
                /* Update the line_lanes_list */
                for (i = 0; i < p_duplex_lane_cfg_rd->num_line_lanes; i++) {
                    p_duplex_port_lanes_sw_db->line_lane_list[i] =
                            p_duplex_lane_cfg_rd->line_lane_list[i];
                }

                /* Update duplex lane maps */
                p_duplex_lane_map_sw_db->sys_lane_map =
                        p_duplex_lane_map_rd->sys_lane_map;
                p_duplex_lane_map_sw_db->line_lane_map =
                        p_duplex_lane_map_rd->line_lane_map;
                if(chip_rev == BARCHETTA_CHIP_REV_INTERNAL) {
                    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_barchetta_phymod_phy_access_t));
                    phy_temp.port_loc = phymodPortLocSys;
                    phy_temp.access.lane_mask = p_duplex_lane_map_rd->sys_lane_map;
                    PHYMOD_IF_ERR_RETURN(__plp_barchetta_get_serdes_config_info(&phy_temp, config, &serdes_cfg));
                    PHYMOD_IF_ERR_RETURN(_plp_barchetta_configure_pll(&phy_temp, config->ref_clock, serdes_cfg));

                    phy_temp.port_loc = phymodPortLocLine;
                    phy_temp.access.lane_mask = p_duplex_lane_map_rd->line_lane_map;
                    PHYMOD_IF_ERR_RETURN(__plp_barchetta_get_serdes_config_info(&phy_temp, config, &serdes_cfg));
                    PHYMOD_IF_ERR_RETURN(_plp_barchetta_configure_pll(&phy_temp, config->ref_clock, serdes_cfg));
                }
            } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {

            } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {

            }

            /* Update port_type   */
            plp_barchetta_sw_db[pa->addr][port].port_cfg_info.port_type = port_cfg_rd.port_type;

            /* Update port number */
            plp_barchetta_sw_db[pa->addr][port].port_cfg_info.port_num = port_cfg_rd.port_num;

            /* update port mode */
            plp_barchetta_sw_db[pa->addr][port].port_cfg_info.port_mode = port_cfg_rd.port_mode;

            /* Update port status */
            plp_barchetta_sw_db[pa->addr][port].port_status = BARCHETTA_PORT_ALLOCATED;
            PHYMOD_IF_ERR_RETURN(
                  __plp_barchetta_save_sw_db_status_info(pa, port, BARCHETTA_SW_DB_PORT_STATUS_REG_ADDR, BARCHETTA_PORT_ALLOCATED)) ;

            /* Update allocated and unallocated port list */
            plp_barchetta_allocated_port_list[pa->addr][port] = port;
            plp_barchetta_unallocated_port_list[pa->addr][port] = 0xFF;
        } else {
            plp_barchetta_sw_db[pa->addr][port].port_status = BARCHETTA_PORT_UNALLOCATED;
            PHYMOD_IF_ERR_RETURN(
                   __plp_barchetta_save_sw_db_status_info(pa, port, BARCHETTA_SW_DB_PORT_STATUS_REG_ADDR, BARCHETTA_PORT_UNALLOCATED));
            port_cfg->port_num = 0xFF; /* Indicate that port allocation and configuration being failed */
            PHYMOD_DEBUG_ERROR(
                    ("ERROR: Port Allocation Failed Due to CONFIG_PORT.READ() Error\n"));
            return PHYMOD_E_INTERNAL;
        }
    } else {
        PHYMOD_DEBUG_ERROR(
                ("ERROR: Port Allocation Failed Due to CONFIG_PORT.Write() Error. Port:%d sts:%d\n", port,
                 plp_barchetta_sw_db[pa->addr][port].port_status));
        plp_barchetta_sw_db[pa->addr][port].port_status = BARCHETTA_PORT_UNALLOCATED;
        PHYMOD_IF_ERR_RETURN(
               __plp_barchetta_save_sw_db_status_info(pa, port, BARCHETTA_SW_DB_PORT_STATUS_REG_ADDR, BARCHETTA_PORT_UNALLOCATED));
        port_cfg->port_num = 0xFF; /* Indicate that port allocation and configuration being failed */
       return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to find whether the requested lanes are already being used
           by allocated port or not
 \param    phy        [In] Pointer to phy access structure
 \param    port       [In] Already allocated hardware port
 \param    port_cfg   [In] Pointer to port configuration structure
 \return   Status as 1 if requested lanes are already being used by allocated port,
           otherwise it returns 0
*******************************************************************************/
static uint8_t
__plp_barchetta_are_requested_lanes_used_by_allocated_port(
        const plp_barchetta_phymod_phy_access_t *phy, uint8_t port,
        barchetta_port_config_t *port_cfg) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint8_t port_type;
    uint8_t status = 0;
    barchetta_lane_map_t lane_map_cfg;

    PHYMOD_IF_ERR_RETURN(__plp_barchetta_get_lane_map(port_cfg, &lane_map_cfg));

    if(port >= BARCHETTA_NUM_OF_PORTS) {
        PHYMOD_DEBUG_ERROR(("ERROR: Port %d is invalid.\n", port));
        return PHYMOD_E_INTERNAL;
    }

    /* Retrieve port direction from PortType */
    port_type = (port_cfg->port_type);

    if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        barchetta_duplex_lane_map_t *p_duplex_lane_map_requested = &(lane_map_cfg.lane_map.duplex_lane_map);
        barchetta_duplex_lane_map_t *p_duplex_lane_map_allocated = &(plp_barchetta_sw_db[pa->addr][port].lane_map_info.lane_map.duplex_lane_map);
        if ((p_duplex_lane_map_allocated->sys_lane_map  & p_duplex_lane_map_requested->sys_lane_map)||
            (p_duplex_lane_map_allocated->line_lane_map & p_duplex_lane_map_requested->line_lane_map)) {
            status = 1;
        }
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        
    }
    return (status);
}

/***************************************************************************//**
 \brief    API to disable port(s) if lanes are already being used by the port
 \param    phy                 [In] Pointer to phy access structure
 \param    port_list_allocated [In] Pointer to allocated port list
 \param    port_cfg            [In] Pointer to port configuration structure
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_disable_ports_if_lanes_are_already_in_use(
        const plp_barchetta_phymod_phy_access_t *phy, barchetta_package_info_t pkg_info,
        uint8_t *port_list_allocated, barchetta_port_config_t *port_cfg) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t chip_rev = 0;
    uint8_t i;

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_internal_chip_rev(&phy->access, &chip_rev));

    for (i = 0; i < pkg_info.no_of_max_ports; i++) {
        /* ARUN: Before calling this, can we check whether the port is allocated*/
        if ((port_list_allocated[i] != 0xFF) &&
           (__plp_barchetta_are_requested_lanes_used_by_allocated_port(phy, port_list_allocated[i], port_cfg))) {
            if(chip_rev == BARCHETTA_CHIP_REV_INTERNAL) {
                PHYMOD_IF_ERR_RETURN(__plp_barchetta_remove_associated_pll_lanes(phy, port_list_allocated[i]));
            }
            /* Disable the port through message interface */
            if (BARCHETTA_MSG_IF_RET_SUCCESS == _plp_barchetta_msg_disable_port(pa, port_list_allocated[i])) {
                /* Clear the SW database for respective port and update the allocated and unallocated port list. */
                PHYMOD_IF_ERR_RETURN(
                        __plp_barchetta_deallocate_port_and_clear_sw_database(pa, port_list_allocated[i]));
            } else {
                PHYMOD_DEBUG_ERROR(
                        ("ERROR: Port %d Disable Failed Due to DISABLE_PORT Error.\n", port_list_allocated[i]));
                return PHYMOD_E_INTERNAL;
            }
        }
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to find the maximum of both (System and Line) side port lanes
 \param    port_cfg   [In] Pointer to port configuration structure
 \return   Maximum number of lanes
*******************************************************************************/
static uint8_t
__plp_barchetta_get_maximum_of_both_side_port_lanes(
        barchetta_port_config_t *port_cfg) {
    uint8_t port_type;
    uint8_t max_no_of_lanes = 0;
    /* Retrieve port direction from PortType */
    port_type = (port_cfg->port_type);

    if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        barchetta_duplex_port_lanes_t *p_duplex_port_lanes = &(port_cfg->port_lanes.duplex);
        max_no_of_lanes = m_BARCHETTA_MAX(p_duplex_port_lanes->num_sys_lanes, p_duplex_port_lanes->num_line_lanes);

    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        max_no_of_lanes = 0;

    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        max_no_of_lanes = 0;

    }

    if(port_cfg->port_mode == BARCHETTA_PORT_MODE_FAILOVER) {
        max_no_of_lanes /= 2 ;
    }
    return (max_no_of_lanes);
}

/***************************************************************************//**
 \brief    API to determine port mode (Regular/Failover mode)
 \param    sys_primary_lane_map    [In] System side primary lane map
 \param    sys_failover_lane_map   [In] System side failover lane map
 \param    line_primary_lane_map   [In] Line side primary lane map
 \param    line_failover_lane_map  [In] Line side failover lane map
 \return   Status information
 \retval   PHYMOD_E_PARAM
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int
__plp_barchetta_get_port_mode(uint32_t sys_primary_lane_map,
        uint32_t sys_failover_lane_map, uint32_t line_primary_lane_map,
        uint32_t line_failover_lane_map, uint8_t *port_mode) {
    *port_mode = BARCHETTA_PORT_MODE_REGULAR;
    if ((sys_failover_lane_map == 0) && (line_failover_lane_map == 0)) {
        *port_mode = BARCHETTA_PORT_MODE_REGULAR;
         return PHYMOD_E_NONE;
    } else if ((sys_failover_lane_map != 0) && (line_failover_lane_map != 0)) {
        *port_mode = 0xFF;
         return PHYMOD_E_PARAM;
    } else if ((sys_failover_lane_map != 0) || (line_failover_lane_map != 0)) {
        if(sys_primary_lane_map & sys_failover_lane_map) {
            *port_mode = 0xFF;
             return PHYMOD_E_PARAM;
        }
        else if(line_primary_lane_map & line_failover_lane_map) {
            *port_mode = 0xFF;
             return PHYMOD_E_PARAM;
        } else {
            *port_mode = BARCHETTA_PORT_MODE_FAILOVER;
        }
    }
    return PHYMOD_E_NONE ;
}

/***************************************************************************//**
 \brief   Check wheather any allocation possibility found or not
 \param   allocation_possibility_list [In] Pointer to allocation possibility list
 \param   size                        [In] Size of the allocation possibility list
 \return  1 if no allocation possibility found
*******************************************************************************/
static int
__plp_barchetta_no_allocation_possibility_found(uint8_t *allocation_possibility_list, uint8_t size) {
    uint8_t i = 0 ;
    for(i=0; i<size; i++) {
        if(allocation_possibility_list[i] != 0xFF) {
            return 0 ;
        }
    }
    return 1 ;
}
/***************************************************************************//**
 \brief    To check wheather next allocation possible port already allocated or not
 \param    phy                      [In] Pointer to phymod phy access structure
 \param    allocation_possible_port [In] Next allocation possible port
 \return   Next allocation possible port status
*******************************************************************************/
static int
__plp_barchetta_is_next_allocation_possible_port_already_allocated(const plp_barchetta_phymod_phy_access_t *phy,
        uint8_t allocation_possible_port) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    if(allocation_possible_port < BARCHETTA_NUM_OF_PORTS) {
        uint8_t *p_port_status = &(plp_barchetta_sw_db[pa->addr][allocation_possible_port].port_status); ;
        return(*p_port_status);
    } else {
        return BARCHETTA_PORT_UNALLOCATED ;
    }
}

/***************************************************************************//**
 \brief    Modify and re-arrange allocation possibility list
 \param    phy                         [In]     Pointer to phymod phy access structure
 \param    pkg_info                    [In]     Package information
 \param    allocation_possibility_list [In/Out] allocation possibility list
 \return
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_modify_allocation_possibility_list(const plp_barchetta_phymod_phy_access_t *phy,
        barchetta_package_info_t pkg_info, uint8_t *allocation_possibility_list) {
    int i = 0, j = 0;
    for(i=0; i<pkg_info.no_of_max_ports / 2; i++) {
        if(__plp_barchetta_is_next_allocation_possible_port_already_allocated(phy, allocation_possibility_list[i])) {
            allocation_possibility_list[i] = 0xFF ;
        }
    }

    /* Rearrange allocation_possibility_list[] by moving all 0xFF at the end */
    for(i=0; i<pkg_info.no_of_max_ports / 2; i++) {
        if(allocation_possibility_list[i] != 0xFF) {
            allocation_possibility_list[j++] = allocation_possibility_list[i] ;
        }
    }
    while(j < pkg_info.no_of_max_ports / 2) {
        allocation_possibility_list[j++] = 0xFF ;
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to allocate proper hardware port number and configure the same.
 \param    phy              [In]  Pointer to phy access structure
 \param    config           [In]  Pointer to phy interface config structure
 \param    allocated_port   [Out] Pointer to allocated port
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_allocate_and_configure_port(const plp_barchetta_phymod_phy_access_t *phy,
        const plp_barchetta_phymod_phy_inf_config_t *config, barchetta_serdes_config_info_t serdes_cfg, uint8_t *allocated_port) {
    uint8_t allocation_possibility_matrix[4][BARCHETTA_NUM_OF_PORTS] =
    {
      { 0x03, 0x01, 0x02, 0x00, 0x07, 0x05, 0x06, 0x04 }, /* 1 lane  hardware port allocation possibility based on quad */
      { 0x01, 0x02, 0x00, 0xFF, 0x05, 0x06, 0x04, 0xFF }, /* 2 lanes hardware port allocation possibility based on quad */
      { 0x00, 0xFF, 0xFF, 0xFF, 0x04, 0xFF, 0xFF, 0xFF }, /* 4 lanes hardware port allocation possibility based on quad */
      { 0x00, 0xFF, 0xFF, 0xFF, 0x04, 0xFF, 0xFF, 0xFF }  /* 8 lanes hardware port allocation possibility based on quad */
    };
    barchetta_port_config_t port_cfg;
    barchetta_package_info_t pkg_info;
    uint8_t port_cfg_line_status = 0;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    typedef uint8_t (*ptr_to_arr_t)[BARCHETTA_NUM_OF_PORTS];
    uint8_t allocation_possibility_list[BARCHETTA_NUM_OF_PORTS / 2];
    uint8_t i = 0, j = 0;
    uint8_t *pchar = NULL;
    uint8_t num_of_lanes = 0;
    uint8_t num_of_max_lanes = 0 ;
    uint8_t lane_list[BARCHETTA_NUM_OF_LANES] = { 0xFF };
    uint8_t lane_list_failover[BARCHETTA_NUM_OF_LANES] = { 0xFF };
    uint8_t max_num_of_lanes;
    uint8_t port_type;
    uint8_t port_mode;
    uint32_t sys_primary_lane_map   = 0;
    uint32_t line_primary_lane_map  = 0;
    uint32_t sys_failover_lane_map  = 0;
    uint32_t line_failover_lane_map = 0;
    ptr_to_arr_t p_to_arr;
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    port_type = pkg_info.pkg_type;
    num_of_max_lanes = pkg_info.pkg_lanes ;

    PHYMOD_MEMSET(&port_cfg, 0, sizeof(barchetta_port_config_t));
    port_cfg.port_type = port_type;
    *allocated_port = 0xFF;
    if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        barchetta_duplex_port_lanes_t *p_duplex_port_lanes = &(port_cfg.port_lanes.duplex);
        if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
            /* In case of System Side, Save the lane related information in Software Database */
            plp_barchetta_sw_db[pa->addr][0].sys_primary_port_lane_map = PHYMOD_ACC_LANE_MASK(&phy->access);
            plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.failover_lane_map = aux_mode->failover_lane_map;
            /* Indicate that System Side Configuration has been provided */
            plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys_status = 1;
            PHYMOD_IF_ERR_RETURN(__plp_barchetta_save_sw_db_status_info(pa, 0, BARCHETTA_SW_DB_PORT_CONFIG_SYS_STATUS_REG_ADDR, 1)) ;
            return PHYMOD_E_NONE;
        } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
            /* In case of line side, copy the line side lane related information from aux_mode structure */
            line_primary_lane_map = PHYMOD_ACC_LANE_MASK(&phy->access);
            line_failover_lane_map = aux_mode->failover_lane_map;
            /* In case of system side, copy the lane related information from software database */
            sys_primary_lane_map = plp_barchetta_sw_db[pa->addr][0].sys_primary_port_lane_map;
            sys_failover_lane_map = plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.failover_lane_map;
            /* Figure out the port_mode based on system and line side lane maps */
            PHYMOD_IF_ERR_RETURN(__plp_barchetta_get_port_mode(sys_primary_lane_map, sys_failover_lane_map, line_primary_lane_map, line_failover_lane_map, &port_mode));
            port_cfg.port_mode = port_mode;

            /* Figure out the line side primary port lane list and number of lanes */
            PHYMOD_IF_ERR_RETURN(__plp_barchetta_convert_lane_map_to_lane_list(pa, pkg_info, line_primary_lane_map, &num_of_lanes, lane_list));
            p_duplex_port_lanes->num_line_lanes = num_of_lanes;
            for (i = 0; i < p_duplex_port_lanes->num_line_lanes; i++) {
                p_duplex_port_lanes->line_lane_list[i] = lane_list[i];
            }
            if((port_cfg.port_mode == BARCHETTA_PORT_MODE_FAILOVER) && (line_failover_lane_map != 0)) {
                /* Figure out the line side failover port lane list and number of lanes */
                PHYMOD_IF_ERR_RETURN(__plp_barchetta_convert_lane_map_to_lane_list(pa, pkg_info, line_failover_lane_map, &num_of_lanes, lane_list_failover));
                p_duplex_port_lanes->num_line_lanes += num_of_lanes ;
                for (j = 0; j < num_of_lanes; j++, i++) {
                    p_duplex_port_lanes->line_lane_list[i] = lane_list_failover[j];
                }
            }
            port_cfg_line_status = 1;

            /* Reset the num_of_lanes and lane_list[] to initial stage */
            num_of_lanes = 0;
            for (i = 0; i < num_of_max_lanes; i++) {
                lane_list[i] = 0xFF;
            }
            /* Figure out the system side primary port lane list and number of lanes */
            PHYMOD_IF_ERR_RETURN(
                    __plp_barchetta_convert_lane_map_to_lane_list(pa, pkg_info, sys_primary_lane_map, &num_of_lanes, lane_list));
            p_duplex_port_lanes->num_sys_lanes = num_of_lanes;
            for (i = 0; i < p_duplex_port_lanes->num_sys_lanes; i++) {
                p_duplex_port_lanes->sys_lane_list[i] = lane_list[i];
            }
            if((port_cfg.port_mode == BARCHETTA_PORT_MODE_FAILOVER) && (plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.failover_lane_map != 0)) {
                /* Figure out the system side failover port lane list and number of lanes */
                PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_convert_lane_map_to_lane_list(pa, pkg_info, plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.failover_lane_map, &num_of_lanes, lane_list_failover));
                p_duplex_port_lanes->num_sys_lanes += num_of_lanes ;
                for (j = 0; j < num_of_lanes; j++, i++) {
                    p_duplex_port_lanes->sys_lane_list[i] = lane_list_failover[j];
                }
            }
        }
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {

    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {

    }

    if (plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys_status && port_cfg_line_status) {
        max_num_of_lanes = __plp_barchetta_get_maximum_of_both_side_port_lanes(&port_cfg);
        p_to_arr = &allocation_possibility_matrix[__plp_barchetta_log2n(max_num_of_lanes)];
        pchar = (uint8_t *) p_to_arr;

        /* In case of upper quad select right half of the allocation possibility list */
        if(phy->access.lane_mask & 0xF0) {
           pchar += 4;
        }

        for (i = 0; i < pkg_info.no_of_max_ports / 2; i++) {
            allocation_possibility_list[i] = *(pchar + i);
        }
        PHYMOD_DEBUG_INFO(("Before Modification : allocation_possibility_list[] = {0x%x, 0x%x, 0x%x, 0x%x}\n",
        allocation_possibility_list[0], allocation_possibility_list[1], allocation_possibility_list[2], allocation_possibility_list[3]));

        PHYMOD_IF_ERR_RETURN(__plp_barchetta_disable_ports_if_lanes_are_already_in_use(phy, pkg_info, plp_barchetta_allocated_port_list[pa->addr], &port_cfg));

        PHYMOD_IF_ERR_RETURN(__plp_barchetta_modify_allocation_possibility_list(phy, pkg_info, allocation_possibility_list));

        PHYMOD_DEBUG_INFO(("After  Modification : allocation_possibility_list[] = {0x%x, 0x%x, 0x%x, 0x%x}\n",
        allocation_possibility_list[0], allocation_possibility_list[1], allocation_possibility_list[2], allocation_possibility_list[3]));

        /* After modification of allocation_possibility_list[], if there are no allocation possibility
           found in allocation_possibility_list[], then restore the original allocation possibility list */
        if(__plp_barchetta_no_allocation_possibility_found(allocation_possibility_list, BARCHETTA_NUM_OF_PORTS/2)) {
            /* Restore original allocation possibility list */
            for (i = 0; i < pkg_info.no_of_max_ports/2; i++) {
                allocation_possibility_list[i] = *(pchar + i);
            }
        }

        /* Find out wheather there is a match of element in allocation_possibility_list[] with elements in plp_barchetta_unallocated_port_list[].
         If there is a match then pick up that element from allocation_possibility_list[] and grant for allocation.
         If there is no match then pick up the first element from the allocation_possibility_list[] and grant for allocation */
        for (i = 0; i < pkg_info.no_of_max_ports/2; i++) {
            if (allocation_possibility_list[i] != 0xFF) {
                for (j = 0; j < pkg_info.no_of_max_ports; j++) {
                    if (allocation_possibility_list[i] == plp_barchetta_unallocated_port_list[pa->addr][j]) {
                        goto decide_port_grant_for_allocation;
                    }
                }
            }
        }
decide_port_grant_for_allocation:
        if ((i == pkg_info.no_of_max_ports/2)&& (j == pkg_info.no_of_max_ports)) {
            PHYMOD_IF_ERR_RETURN( __plp_barchetta_allocate_port_and_update_sw_database(
                                  phy,
                                  &port_cfg,
                                  config,
                                  serdes_cfg,
                                  allocation_possibility_list[0])
                                );
            *allocated_port = allocation_possibility_list[0];
        } else {
            PHYMOD_IF_ERR_RETURN( __plp_barchetta_allocate_port_and_update_sw_database(
                                  phy,
                                  &port_cfg,
                                  config,
                                  serdes_cfg,
                                  allocation_possibility_list[i])
                                );
            *allocated_port = allocation_possibility_list[i];
        }

        PHYMOD_DEBUG_INFO(("Allocated port = %d\n", *allocated_port));

        plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys_status = 0;
        PHYMOD_IF_ERR_RETURN(__plp_barchetta_save_sw_db_status_info(pa, 0, BARCHETTA_SW_DB_PORT_CONFIG_SYS_STATUS_REG_ADDR, 0)) ;
        PHYMOD_MEMSET(&port_cfg, 0, sizeof(barchetta_port_config_t));
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: Either System or Line side configuration is not completed yet\n"));
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}


/***************************************************************************//**
 \brief    Numeric value to lane data rate conversion
 \param    numeric_val   [In] Numeric value
 \return   Returns the corresponding lane_data_rate for the provided numeric value
*******************************************************************************/
barchetta_lane_data_rate_t __plp_barchetta_convert_numeric_value_to_lane_data_rate(
        uint32_t numeric_val) {
  int barchetta_lane_datarate_list[BARCHETTA_MAX_NO_DR] = BARCHETTA_LANE_DATARATE_LIST_ELEMENTS ;
  if(numeric_val < (sizeof(barchetta_lane_datarate_list)/sizeof(barchetta_lane_datarate_list[0]))) {
      return ((barchetta_lane_data_rate_t)barchetta_lane_datarate_list[numeric_val]) ;
  } else {
      return BARCHETTA_LANE_DATA_RATE_NONE;
  }
}

/***************************************************************************//**
 \brief    API to save lane data rate in SW GP register
 \param    phy      [In] Pointer to phy access structure
 \param    config   [In] Pointer to phy interface config structure
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int
__plp_barchetta_save_lane_data_rate(const plp_barchetta_phymod_phy_access_t *phy,
        barchetta_package_info_t pkg_info, const plp_barchetta_phymod_phy_inf_config_t *config ) {
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t lane_data_rate_numeric_val = 0;
    uint32_t lane_data_rate_rd = 0;
    uint32_t lane_data_rate_wr = 0;
    uint32_t lane_map = 0;
    uint8_t num_of_max_lanes   = 0;
    uint8_t lane_index = 0;
    uint8_t sys_side   = 0 ;

    num_of_max_lanes = pkg_info.pkg_lanes ;
    lane_map = (phy->access.lane_mask | aux_mode->failover_lane_map);

    PHYMOD_IF_ERR_RETURN(
          __plp_barchetta_convert_lane_data_rate_to_numeric_value(aux_mode->lane_data_rate, &lane_data_rate_numeric_val));

    if (BARCHETTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (lane_map & (1 << lane_index)) {
            /* Less optimized version (Easy to understand the underlying idea of the following line) :
             * PHYMOD_IF_ERR_RETURN(
             *       _plp_barchetta_reg_read(pa, ( BARCHETTA_LANE_DATA_RATE_SAVE_GPREG_BASE_ADDR +
             *                               (sys_side * BARCHETTA_LINE_SYS_LANE_DATA_RATE_SAVE_GPREG_OFFSET) +
             *                               (lane_index/2)),
             *                               &lane_data_rate_rd));
             */
            PHYMOD_IF_ERR_RETURN(
                  _plp_barchetta_reg_read(pa, (BARCHETTA_LANE_DATA_RATE_SAVE_GPREG_BASE_ADDR +
                                          (sys_side * BARCHETTA_LINE_SYS_LANE_DATA_RATE_SAVE_GPREG_OFFSET) +
                                          (lane_index >> 1)),
                                           &lane_data_rate_rd));

            /* Less optimized version (Easy to understand the underlying idea of the next two lines) :
             * lane_data_rate_wr  = (lane_data_rate_rd & ~(0xFF << 8*(lane_index % 2)));
             * lane_data_rate_wr |= (lane_data_rate_numeric_val & 0xFF) << 8*(lane_index % 2) ;
             */
            lane_data_rate_wr  = (lane_data_rate_rd & ~(BARCHETTA_LANE_DATA_RATE_STORAGE_MASK << ((lane_index & 0x1) << 0x3)));
            lane_data_rate_wr |= ((lane_data_rate_numeric_val & BARCHETTA_LANE_DATA_RATE_STORAGE_MASK) << ((lane_index & 0x1) << 0x3)) ;
            PHYMOD_IF_ERR_RETURN(
                  _plp_barchetta_reg_write(pa, (BARCHETTA_LANE_DATA_RATE_SAVE_GPREG_BASE_ADDR +
                                           (sys_side * BARCHETTA_LINE_SYS_LANE_DATA_RATE_SAVE_GPREG_OFFSET) +
                                           (lane_index >> 1)),
                                           lane_data_rate_wr));
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:  API to restore lane data rate from SW GP register

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_restore_lane_data_rate(const plp_barchetta_phymod_phy_access_t *phy,
        uint8_t lane_index,
        barchetta_lane_data_rate_t *lane_data_rate ) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t lane_data_rate_rd = 0;
    uint8_t sys_side = 0 ;
    if (BARCHETTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }
    /* Less optimized version (Easy to understand the underlying idea of the following line) :
     * PHYMOD_IF_ERR_RETURN(
     *       _plp_barchetta_reg_read(pa, ( BARCHETTA_LANE_DATA_RATE_SAVE_GPREG_BASE_ADDR +
     *                               (sys_side * BARCHETTA_LINE_SYS_LANE_DATA_RATE_SAVE_GPREG_OFFSET) +
     *                               (lane_index/2)),
     *                               &lane_data_rate_rd));
     */
    PHYMOD_IF_ERR_RETURN(
          _plp_barchetta_reg_read(pa, (BARCHETTA_LANE_DATA_RATE_SAVE_GPREG_BASE_ADDR +
                                  (sys_side * BARCHETTA_LINE_SYS_LANE_DATA_RATE_SAVE_GPREG_OFFSET) +
                                  (lane_index >> 1)),
                                   &lane_data_rate_rd));

    /* Less optimized version (Easy to understand the underlying idea of following line) :
     * *lane_data_rate = __plp_barchetta_convert_numeric_value_to_lane_data_rate((lane_data_rate_rd >> 8*(lane_index % 2)) & 0xFF) ;
     */
    *lane_data_rate = __plp_barchetta_convert_numeric_value_to_lane_data_rate((lane_data_rate_rd >> ((lane_index & 0x1)<<3)) & BARCHETTA_LANE_DATA_RATE_STORAGE_MASK);
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    API to determine the low latency mode
 \param    phy      [In] Pointer to phy access structure
 \param    config   [In] Pointer to phy interface config structure
 \return   Low latency mode
*******************************************************************************/
uint8_t
__plp_barchetta_get_low_latency_mode(const plp_barchetta_phymod_phy_access_t *phy,
        const plp_barchetta_phymod_phy_inf_config_t *config) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;
    uint8_t ll_mode = 0;
    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        if ((aux_mode->modulation_mode == BARCHETTA_MODULATION_PAM4) ||
            (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_1P25G)) {
            /* Set Egress low latency mode */
            plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.ll_mode = 0x02;
        } else {
            plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.ll_mode = (aux_mode->ll_mode & 2);
        }
    } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
        if ((aux_mode->modulation_mode == BARCHETTA_MODULATION_PAM4)||
            (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_1P25G)) {
            /* Set Ingress low latency mode */
            ll_mode = 0x01;
        } else {
            ll_mode = (aux_mode->ll_mode & 1);
        }

        /* Retrieve back the Egress low latency mode from SW database and update the same in ll_mode */
        ll_mode |= plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.ll_mode;
    }
    return (ll_mode);
}

/***************************************************************************//**
 \brief    API to determine the div2en status
 \param    phy      [In] Pointer to phy access structure
 \param    config   [In] Pointer to phy interface config structure
 \return   div2en status
*******************************************************************************/
uint8_t __plp_barchetta_get_div2en_status(const plp_barchetta_phymod_phy_access_t *phy,
        const plp_barchetta_phymod_phy_inf_config_t *config) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint8_t div2en = 0;
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;
    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.lane_data_rate = aux_mode->lane_data_rate;
    } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
        if (((( plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.lane_data_rate == BARCHETTA_LANE_DATA_RATE_20P625G))&&
              (__plp_barchetta_count_number_of_set_bits(plp_barchetta_sw_db[pa->addr][0].sys_primary_port_lane_map)== 0x02))&&
              (((aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_10P3125G) ||
                (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_10P70922G)||
                (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_10P75460G)||
                (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_10P9375G))&&
                (__plp_barchetta_count_number_of_set_bits(phy->access.lane_mask) == 0x04))) {
            div2en = 1;
        }
    }
    return (div2en);
}

/***************************************************************************//**
 \brief    API to get the OSR information
 \param    phy      [In] Pointer to phy access structure
 \param    config   [In] Pointer to phy interface config structure
 \return   OSR information
*******************************************************************************/
uint8_t
__plp_barchetta_get_osr(const plp_barchetta_phymod_phy_access_t *phy,
        const plp_barchetta_phymod_phy_inf_config_t *config) {
    barchetta_serdes_config_info_t serdes_cfg;
    __plp_barchetta_get_serdes_config_info(phy, config, &serdes_cfg);
    return (serdes_cfg.osr);
}

/***************************************************************************//**
 \brief   Compute the lane config word
 \param   config           [In]  Pointer to interface config structure
 \param   lane_config_word [Out] Lane config word
 \retval  PHYMOD_E_NONE, PHYMOD_E_PARAM
*******************************************************************************/
int
__plp_barchetta_get_lane_config_word(const plp_barchetta_phymod_phy_inf_config_t *config,
        uint16_t *lane_config_word) {
    struct blackhawk_barchetta_uc_lane_config_st st;
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;

    PHYMOD_MEMSET(&st, 0, sizeof(struct blackhawk_barchetta_uc_lane_config_st));
    if ((config->interface_type == phymodInterfaceSFI)||
        (config->interface_type == phymodInterfaceSR4)||
        (config->interface_type == phymodInterfaceLR4)||
        (config->interface_type == phymodInterfaceLR) ||
        (config->interface_type == phymodInterfaceER4)||
        (config->interface_type == phymodInterfaceER) ||
        (config->interface_type == phymodInterfaceSR) ||
        (config->interface_type == phymodInterfaceLR2)||
        (config->interface_type == phymodInterfaceER2)||
        (config->interface_type == phymodInterfaceSR2)) {
        st.field.media_type = phymodFirmwareMediaTypeOptics;
    } else if ((config->interface_type == phymodInterfaceCR) ||
               (config->interface_type == phymodInterfaceCR2)||
               (config->interface_type == phymodInterfaceCR4)) {
        st.field.media_type = phymodFirmwareMediaTypeCopperCable;
    } else {
        st.field.media_type = phymodFirmwareMediaTypePcbTraceBackPlane;
    }

    if (((aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_51P5625G)||
        (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_50G) ||
        (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_53P125G)||
        (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_56P1G)||
        (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_32P5G)||
        (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_33P75G)||
        (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_46P25G) ||
        (aux_mode->lane_data_rate == BARCHETTA_LANE_DATA_RATE_56P25G)) &&
        (aux_mode->modulation_mode == BARCHETTA_MODULATION_PAM4)){
        st.field.force_pam4_mode = 1;
        st.field.dfe_on = 1;
        st.field.force_ns = 1;
        if (st.field.media_type == phymodFirmwareMediaTypeOptics) {
            PHYMOD_DEBUG_ERROR(("ERROR: Wrong media type\n"));
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
        if ((config->interface_type == phymodInterfaceCAUI4_C2C) ||
            (config->interface_type == phymodInterfaceCAUI4_C2M) ||
            (config->interface_type == phymodInterfaceCAUI)) {
            st.field.dfe_on = 1;
            st.field.dfe_lp_mode = 1;
        }
    }
    PHYMOD_IF_ERR_RETURN(
            plp_barchetta_blackhawk_barchetta_INTERNAL_update_uc_lane_config_word(&st));
    *lane_config_word = st.word;

    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    To enable the specified port
 \param    phy      [In] Pointer to phy access structure
 \param    port     [In] Port which is going to be enabled
 \param    config   [In] Pointer to phy interface config structure
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int
__plp_barchetta_enable_port(const plp_barchetta_phymod_phy_access_t *phy, uint8_t port,
        const plp_barchetta_phymod_phy_inf_config_t *config) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;
    barchetta_enable_port_t port_en_cfg;
    PHYMOD_MEMSET(&port_en_cfg, 0, sizeof(barchetta_enable_port_t));
    port_en_cfg.port_num = port;

    if (config->interface_modes == BARCHETTA_INTERFACE_MODE_ETHERNET) {
        port_en_cfg.traffic_type = 0;
    } else if (config->interface_modes == BARCHETTA_INTERFACE_MODE_FIBER) {
        port_en_cfg.traffic_type = 1;
    }

    port_en_cfg.ll_mode = __plp_barchetta_get_low_latency_mode(phy, config);
    port_en_cfg.div2en  = __plp_barchetta_get_div2en_status(phy, config);

    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys.clock_mode = aux_mode->clock_mode;
        plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys.osr = __plp_barchetta_get_osr(phy,config);
        PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_get_lane_config_word(config, &plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys.lane_config_word));
        plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys_status = 1;
        PHYMOD_IF_ERR_RETURN(
               __plp_barchetta_save_sw_db_status_info(pa, 0, BARCHETTA_SW_DB_PMD_CONFIG_SYS_STATUS_REG_ADDR, 1)) ;
        return PHYMOD_E_NONE;
    } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
        port_en_cfg.pmd_cfg[1].clock_mode = aux_mode->clock_mode;
#ifdef QUAD_PLL_CONFIG
        /* Hardcoding pll_sel to 0xFF for line side so that FW will configure TX/RX logical lane mapping */
        port_en_cfg.pmd_cfg[1].pll_sel = 0xFF;
#else
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_get_pll_index(phy, &port_en_cfg.pmd_cfg[1].pll_sel));
#endif
        port_en_cfg.pmd_cfg[1].osr        = __plp_barchetta_get_osr(phy, config);
        port_en_cfg.pmd_cfg[1].fec_opt        = 1;
        PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_get_lane_config_word(config, &port_en_cfg.pmd_cfg[1].lane_config_word));

        /* Retrieve System side PMD configuration from Software Database */
        port_en_cfg.pmd_cfg[0].clock_mode = plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys.clock_mode;
        {
            plp_barchetta_phymod_phy_access_t phy_sys;
            PHYMOD_MEMCPY(&phy_sys, phy, sizeof(plp_barchetta_phymod_phy_access_t));
            phy_sys.port_loc = phymodPortLocSys;
            phy_sys.access.lane_mask = plp_barchetta_sw_db[pa->addr][0].sys_primary_port_lane_map;
#ifdef QUAD_PLL_CONFIG
        /* Hardcoding pll_sel to 0xFF for sys side so that FW will configure TX/RX logical lane mapping */
        port_en_cfg.pmd_cfg[0].pll_sel = 0xFF;
#else
            /* Getting PLL for SYS*/
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_get_pll_index(&phy_sys, &port_en_cfg.pmd_cfg[0].pll_sel));
#endif
        }
        port_en_cfg.pmd_cfg[0].fec_opt        = 0;
        port_en_cfg.pmd_cfg[0].osr = plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys.osr;
        port_en_cfg.pmd_cfg[0].lane_config_word = plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys.lane_config_word;
    }

    if (plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys_status) {
        if (BARCHETTA_MSG_IF_RET_SUCCESS == _plp_barchetta_msg_enable_port(&phy->access, &port_en_cfg)) {
            plp_barchetta_sw_db[pa->addr][0].pmd_cfg_sys_status = 0;
            PHYMOD_IF_ERR_RETURN(
                  __plp_barchetta_save_sw_db_status_info(pa, 0, BARCHETTA_SW_DB_PMD_CONFIG_SYS_STATUS_REG_ADDR, 0)) ;
        }
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: System side PMD configuration not done\n"));
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    To convert interface type to numeric value
 \param    if_type   [In]  Interface type
 \return   Return corresponding numeric value for the provided interface type
*******************************************************************************/
uint32_t __plp_barchetta_convert_interface_type_to_numeric_value(
        plp_barchetta_phymod_interface_t if_type) {
  const plp_barchetta_phymod_interface_t barchetta_if_type_list[] = BARCHETTA_IF_TYPE_LIST_ELEMENTS ;
  uint8_t i = 0 ;
  for(i=0; i < (sizeof(barchetta_if_type_list)/sizeof(barchetta_if_type_list[0])); i++) {
     if(barchetta_if_type_list[i] == if_type){
         return(i) ;
     }
  }
  return(0);
}

/***************************************************************************//**
 \brief    To convert numeric value to interface type
 \param    numeric_val   [In] Numeric value
 \return   Returns the corresponding interface type for the provided numeric value
*******************************************************************************/
plp_barchetta_phymod_interface_t __plp_barchetta_convert_numeric_value_to_interface_type(
        uint32_t numeric_val) {
  const plp_barchetta_phymod_interface_t barchetta_if_type_list[] = BARCHETTA_IF_TYPE_LIST_ELEMENTS ;
  if(numeric_val < (sizeof(barchetta_if_type_list)/sizeof(barchetta_if_type_list[0]))) {
      return (barchetta_if_type_list[numeric_val]) ;
  } else {
      return phymodInterfaceBypass;
  }
}

/***************************************************************************//**
 \brief    To configure the interface type
 \param    phy      [In] Pointer to port configuration structure
 \param    config   [In] Pointer to phy interface config structure
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int __plp_barchetta_configure_interface_type(const plp_barchetta_phymod_phy_access_t *phy,
        barchetta_package_info_t pkg_info, const plp_barchetta_phymod_phy_inf_config_t *config) {
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t if_type_numeric_val = 0;
    uint32_t if_type_rd = 0;
    uint32_t if_type_wr = 0;
    uint32_t lane_map   = 0;
    uint8_t lane_index  = 0;
    uint8_t sys_side    = 0;
    uint8_t num_of_max_lanes = 0;

    num_of_max_lanes = pkg_info.pkg_lanes ;
    lane_map = (phy->access.lane_mask | aux_mode->failover_lane_map);

    if_type_numeric_val = __plp_barchetta_convert_interface_type_to_numeric_value(config->interface_type);
    if (BARCHETTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (lane_map & (1 << lane_index)) {
           /* Less optimized version (Easy to understand the underlying idea of the following line) :
            * PHYMOD_IF_ERR_RETURN(
            *       _plp_barchetta_reg_read(pa, (BARCHETTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
            *                               (sys_side * 2) +
            *                               (lane_index/4)),
            *                               &if_type_rd));
            */
            PHYMOD_IF_ERR_RETURN(
                  _plp_barchetta_reg_read(pa, (BARCHETTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                                          (sys_side * BARCHETTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) +
                                          (lane_index >> 2)),
                                          &if_type_rd));

            /* Less optimized version (Easy to understand the underlying idea of the next two lines) :
             * if_type_wr  = (if_type_rd & ~(0xF << 4*(lane_index % 4)));
             * if_type_wr |= (if_type_numeric_val & 0xF) << 4*(lane_index % 4);
             */
            if_type_wr  = (if_type_rd & ~(BARCHETTA_IF_TYPE_PER_LANE_STORAGE_MASK << ((lane_index & 0x3) << 0x2)));
            if_type_wr |= ((if_type_numeric_val & BARCHETTA_IF_TYPE_PER_LANE_STORAGE_MASK) << ((lane_index & 0x3) << 0x2)) ;
            PHYMOD_IF_ERR_RETURN(
                  _plp_barchetta_reg_write(pa, (BARCHETTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                                           (sys_side * BARCHETTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) +
                                           (lane_index >> 2)),
                                           if_type_wr));
        }
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    To get the interface type
 \param    phy        [In]  Pointer to port configuration structure
 \param    if_type    [Out] Pointer to interface type used
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int
__plp_barchetta_interface_type_get(const plp_barchetta_phymod_phy_access_t *phy, uint8_t lane_index, plp_barchetta_phymod_interface_t *if_type) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t if_type_rd = 0;
    uint8_t sys_side    = 0;
    if (BARCHETTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }
    /* Less optimized version (Easy to understand the underlying idea of the following line) :
     * PHYMOD_IF_ERR_RETURN(
     *       _plp_barchetta_reg_read(pa, IF_TYPE_SAVE_SWGPREG_BASE_ADDR + sys_side * LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET + lane_index/4, &if_type_rd));
     */
    PHYMOD_IF_ERR_RETURN(
          _plp_barchetta_reg_read(pa, (BARCHETTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                                  (sys_side * BARCHETTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) +
                                  (lane_index >> 2)),
                                  &if_type_rd));

    /* Less optimized version (Easy to understand the underlying idea of following line) :
     * *if_type = __plp_barchetta_convert_numeric_value_to_interface_type((if_type_rd >> 4*(lane_index % 4)) & IF_TYPE_PER_LANE_STORAGE_MASK) ;
     */
    *if_type = __plp_barchetta_convert_numeric_value_to_interface_type((if_type_rd >> ((lane_index & 0x3)<<2)) & BARCHETTA_IF_TYPE_PER_LANE_STORAGE_MASK) ;
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_logical_lane_list_set(const plp_barchetta_phymod_phy_access_t *phy,
        const uint8_t no_of_lanes, const uint8_t *tx_list,
        const uint8_t *rx_list) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t lanes_cfg_line_status = 0;
    uint8_t port_type;
    uint8_t i;
    barchetta_config_lanes_t lanes_cfg;
    PHYMOD_MEMSET(&lanes_cfg, 0, sizeof(barchetta_config_lanes_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    lanes_cfg.config_type = port_type = pkg_info.pkg_type;

    if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        barchetta_duplex_lanes_t *p_duplex_lanes_sys_sw_db = &(plp_barchetta_sw_db[pa->addr][0].lanes_cfg_sys.lanes.duplex);
        barchetta_duplex_lanes_t *p_duplex_lanes = &(lanes_cfg.lanes.duplex);

        if (no_of_lanes != BARCHETTA_MAX_NUM_DUPLEX_LANES) {
            PHYMOD_DEBUG_ERROR(("ERROR: Invalid number of lanes, It is expected to be 8\n"));
            return PHYMOD_E_PARAM;
        }

        if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
            p_duplex_lanes_sys_sw_db->num_sys_lanes = no_of_lanes;
            for (i = 0; i < no_of_lanes; i++) {
                p_duplex_lanes_sys_sw_db->sr_lane_list[i] = rx_list[i];
                p_duplex_lanes_sys_sw_db->st_lane_list[i] = tx_list[i];
            }
            /* Indicate that system side logical lane configuration has been provided */
            plp_barchetta_sw_db[pa->addr][0].lanes_cfg_sys_status = 1;
            PHYMOD_IF_ERR_RETURN(
                   __plp_barchetta_save_sw_db_status_info(pa, 0, BARCHETTA_SW_DB_LANES_CONFIG_SYS_STATUS_REG_ADDR, 1)) ;
            return PHYMOD_E_NONE;
        } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
            p_duplex_lanes->num_line_lanes = no_of_lanes;
            for (i = 0; i < no_of_lanes; i++) {
                p_duplex_lanes->lr_lane_list[i] = rx_list[i];
                p_duplex_lanes->lt_lane_list[i] = tx_list[i];
            }
            lanes_cfg_line_status = 1;

            /* Retrieve back system side lane configuration */
            p_duplex_lanes->num_sys_lanes =
                    p_duplex_lanes_sys_sw_db->num_sys_lanes;
            for (i = 0; i < p_duplex_lanes->num_sys_lanes; i++) {
                p_duplex_lanes->sr_lane_list[i] = p_duplex_lanes_sys_sw_db->sr_lane_list[i];
                p_duplex_lanes->st_lane_list[i] = p_duplex_lanes_sys_sw_db->st_lane_list[i];
            }
        }
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        
    }

    if (plp_barchetta_sw_db[pa->addr][0].lanes_cfg_sys_status && lanes_cfg_line_status) {
        if (BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_config_lanes_wr(pa, &lanes_cfg)) {
            PHYMOD_DEBUG_ERROR(
                    ("ERROR: Lane configuration Failed Due to CONFIG_LANES.Write() Error\n"));
            return PHYMOD_E_INTERNAL;
        }
    } else {
        PHYMOD_DEBUG_ERROR(
                ("ERROR: Either System or Line side configuration is not completed yet\n"));
        return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_logical_lane_list_get(const plp_barchetta_phymod_phy_access_t* phy,
        uint8_t *no_of_lanes, uint8_t *tx_list, uint8_t *rx_list) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_config_lanes_t lanes_cfg;
    uint8_t i;
    PHYMOD_MEMSET(&lanes_cfg, 0, sizeof(barchetta_config_lanes_t));
    if (BARCHETTA_MSG_IF_RET_SUCCESS == _plp_barchetta_msg_config_lanes_rd(pa, &lanes_cfg)) {
        if (lanes_cfg.config_type == BARCHETTA_PORT_TYPE_DUPLEX) {
            barchetta_duplex_lanes_t *p_duplex_lanes = &(lanes_cfg.lanes.duplex);
            if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
                *no_of_lanes = p_duplex_lanes->num_sys_lanes;
                for (i = 0; i < p_duplex_lanes->num_sys_lanes; i++) {
                    tx_list[i] = p_duplex_lanes->st_lane_list[i];
                    rx_list[i] = p_duplex_lanes->sr_lane_list[i];
                }
            } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
                *no_of_lanes = p_duplex_lanes->num_line_lanes;
                for (i = 0; i < p_duplex_lanes->num_line_lanes; i++) {
                    tx_list[i] = p_duplex_lanes->lt_lane_list[i];
                    rx_list[i] = p_duplex_lanes->lr_lane_list[i];
                }
            }
        } else if (lanes_cfg.config_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
            
        } else if (lanes_cfg.config_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
            
        }
    } else {
        PHYMOD_DEBUG_ERROR(
                ("Lane configuration read Failed Due to CONFIG_LANES.Read() Error\n"));
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_top_out_of_reset(const plp_barchetta_phymod_phy_access_t *phy,
        int allocated_port) {
    int port_lane = 0, lane = 0;
    BCMI_BARCHETTA_BHIF_PER_LANE_REPEATER_STATUS_PER_LANEr_t lane_rpts_sts;
    BCMI_BARCHETTA_DP_PORT_RST_BLK_RSTr_t prt_rst_blk;

    PHYMOD_MEMSET(&prt_rst_blk, 0, sizeof(BCMI_BARCHETTA_DP_PORT_RST_BLK_RSTr_t));
    PHYMOD_MEMSET(&lane_rpts_sts, 0, sizeof(BCMI_BARCHETTA_BHIF_PER_LANE_REPEATER_STATUS_PER_LANEr_t));
    
    for (lane = 0; lane < BARCHETTA_MAX_LANE; lane++) {
        if (phy->access.lane_mask & (1 << lane)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, allocated_port, port_lane,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, 0xFFFF));
            port_lane++;
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_BHIF_PER_LANE_REPEATER_STATUS_PER_LANEr(&phy->access, &lane_rpts_sts));

            if (BCMI_BARCHETTA_BHIF_PER_LANE_REPEATER_STATUS_PER_LANEr_PMD_TX_CLK_VLDf_GET(lane_rpts_sts)) {
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_DP_PORT_RST_BLK_RSTr(&phy->access, &prt_rst_blk));
                BCMI_BARCHETTA_DP_PORT_RST_BLK_RSTr_BLK_RSTB_FRCf_SET(prt_rst_blk, 1);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_DP_PORT_RST_BLK_RSTr(&phy->access, prt_rst_blk));
                PHYMOD_USLEEP(5000);
                BCMI_BARCHETTA_DP_PORT_RST_BLK_RSTr_BLK_RSTB_FRCf_SET(prt_rst_blk, 0);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_DP_PORT_RST_BLK_RSTr(&phy->access, prt_rst_blk));
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_clock_config(const plp_barchetta_phymod_phy_access_t *phy,
                            const plp_barchetta_phymod_phy_inf_config_t *config)
{
    BCMI_BARCHETTA_SLICE_SLICE0r_t slice_0;
    BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_t ack_timer;
    BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_t clock_scaler;
    uint16_t heart_beat;
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_t  rev;

    PHYMOD_MEMSET(&clock_scaler, 0 , sizeof(clock_scaler));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_CTRL_CHIP_REVISIONr(&phy->access, &rev));

    slice_0.v[0] = ((1 << 15) | ((BARCHETTA_IS_LINE_SIDE(phy)) ?
                    (1 << 12) : (1 << 13)));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));
    PHYMOD_IF_ERR_RETURN(
       BCMI_BARCHETTA_READ_CTRL_BH_ACK_TIMERr(&phy->access, &ack_timer));

    if ((config->ref_clock == phymodRefClk156Mhz) || (config->ref_clock == phymodRefClk312Mhz)) {
        if (config->ref_clock == phymodRefClk156Mhz) {
             BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
            if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
                heart_beat = 312;
            } else {
                heart_beat = 625;
            }
        } else {
            BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 1);
            heart_beat = 625;
        }
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x320);
    } else if (config->ref_clock == phymodRefClk156P637Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 313;
        } else {
            heart_beat = 626;
        }
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x321);
    } else if (config->ref_clock == phymodRefClk157P844Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 315;
        } else {
            heart_beat = 631;
        }
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x328);
    } else if (config->ref_clock == phymodRefClk158P51Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 317;
        } else {
            heart_beat = 634;
        }
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x32b);
    } else if ((config->ref_clock == phymodRefClk161Mhz) || (config->ref_clock == phymodRefClk322Mhz)) {
        if (config->ref_clock == phymodRefClk161Mhz) {
            BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
            if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
                heart_beat = 322;
            } else {
                heart_beat = 645;
            }
        } else {
            BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 1);
            heart_beat = 645 ;
        }
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x339);
    } else if (config->ref_clock == phymodRefClk169P409Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x363);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 339;
        } else {
            heart_beat = 678;
        }
    } else if (config->ref_clock == phymodRefClk162P948Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x342);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
        heart_beat = 326;
        } else {
            heart_beat = 652;
        }
    } else if (config->ref_clock == phymodRefClk172Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x373);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 344;
        } else {
            heart_beat = 688;
        }
    } else if (config->ref_clock == phymodRefClk167P331Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x358);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 334;
        } else {
            heart_beat = 669;
        }
    } else if (config->ref_clock == phymodRefClk168Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x35D);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
        heart_beat = 336;
        } else {
            heart_beat = 672;
        }
    } else if (config->ref_clock == phymodRefClk106P25Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x220);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
        heart_beat = 212;
        } else {
            heart_beat = 425;
        }
    } else if (config->ref_clock == phymodRefClk125Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x280);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 250;
        } else {
            heart_beat = 500;
        }
    } else if (config->ref_clock == phymodRefClk122P88Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x274);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 245;
        } else {
            heart_beat = 491;
        }
    } else if (config->ref_clock == phymodRefClk155P52Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x31c);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 311;
        } else {
            heart_beat = 622;
        }
    } else if (config->ref_clock == phymodRefClk159P375Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x32f);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 319;
        } else {
            heart_beat = 637;
        }
    } else if (config->ref_clock == phymodRefClk168P04Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x35c);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 336;
        } else {
            heart_beat = 672;
        }
    } else if (config->ref_clock == phymodRefClk173P37Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x377);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 347;
        } else {
            heart_beat = 693;
        }
    }  else if (config->ref_clock == phymodRefClk174P70Mhz) {
        BCMI_BARCHETTA_CTRL_BH_ACK_TIMERr_COM_CK_DIV2_ENABLEf_SET(ack_timer, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_CLOCK_SCALER_CTRLr_CLOCK_SCALER_RATIOf_SET(clock_scaler, 0x37d);
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            heart_beat = 349;
        } else {
            heart_beat = 699;
        }
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid ref clock\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_IF_ERR_RETURN(
       BCMI_BARCHETTA_WRITE_CTRL_BH_ACK_TIMERr(&phy->access, ack_timer));

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_GEN_CNTRLS_CLOCK_SCALER_CTRLr(&phy->access, clock_scaler));

    PHYMOD_IF_ERR_RETURN(
            wrc_heartbeat_count_1us(heart_beat));

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: To set get the Tx driver mode

 COMMENT:
*******************************************************************************/
int _plp_barchetta_set_get_tx_drv_mode(const plp_barchetta_phymod_phy_access_t *phy, int set_get,
                                   barchetta_package_info_t pkg_info,
                                   void* void_aux_mode)
{
    uint32_t lane = 0;
    uint8_t logical_lane =0;
    BCMI_BARCHETTA_BHIF_COMMON_COMMON_CONTROLr_t common_ctrl;
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) void_aux_mode;

    PHYMOD_MEMSET(&common_ctrl, 0, sizeof(BCMI_BARCHETTA_BHIF_COMMON_COMMON_CONTROLr_t));
    for (lane = 0; lane < pkg_info.pkg_lanes; lane++) {
        if (phy->access.lane_mask & (1 << lane)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                    BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_BHIF_COMMON_COMMON_CONTROLr(&phy->access, &common_ctrl));
            if (set_get == 0) {
                if (aux_mode->tx_driver_mode == bcmplptxdrivebarchettaHWdefaults) {
                    BCMI_BARCHETTA_BHIF_COMMON_COMMON_CONTROLr_TX_DRV_HV_DISABLEf_SET(common_ctrl, 0);
                } else if (aux_mode->tx_driver_mode == bcmplptxdrivebarchetta0P8Volt) {
                    BCMI_BARCHETTA_BHIF_COMMON_COMMON_CONTROLr_TX_DRV_HV_DISABLEf_SET(common_ctrl, 1);
                } else if (aux_mode->tx_driver_mode == bcmplptxdrivebarchetta1P2Volt) {
                    BCMI_BARCHETTA_BHIF_COMMON_COMMON_CONTROLr_TX_DRV_HV_DISABLEf_SET(common_ctrl, 0);
                }
                PHYMOD_IF_ERR_RETURN(
                  BCMI_BARCHETTA_WRITE_BHIF_COMMON_COMMON_CONTROLr(&phy->access, common_ctrl));
            } else {
                if (BCMI_BARCHETTA_BHIF_COMMON_COMMON_CONTROLr_TX_DRV_HV_DISABLEf_GET(common_ctrl) == 0) {
                    aux_mode->tx_driver_mode = bcmplptxdrivebarchetta1P2Volt;
                } else {
                    aux_mode->tx_driver_mode = bcmplptxdrivebarchetta0P8Volt;
                }
                break;
            }
        }
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    To enable clock buffer if the port need clocks crossing the quad,
           otherwise disable clock buffer. (Applicable only for B0)
 \param    phy      [In] Pointer to phy access structure
 \return   PHYMOD_E_NONE
*******************************************************************************/
static int
__plp_barchetta_configure_rptr_clock_buffer(const plp_barchetta_phymod_phy_access_t *phy, uint32_t failover_lanemap)
{
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    BCMI_BARCHETTA_CTRL_SWGPREG4r_t pll_0_associated_lanes;
    BCMI_BARCHETTA_CTRL_SWGPREG6r_t pll_1_associated_lanes;
    uint32_t chip_rev = 0;
    uint32_t pll0_tx_lane_map = 0;
    uint32_t pll0_rx_lane_map = 0;
    uint32_t pll1_tx_lane_map = 0;
    uint32_t pll1_rx_lane_map = 0;
    uint8_t  pll = 0;
    uint8_t  dir = 0;
    uint8_t  i   = 0;
    uint8_t  ams_pll_rx_clk_rptr_pd = 0;
    uint8_t  ams_pll_tx_clk_rptr_pd = 0;
    uint8_t  pll_clk_buf = 0;
    uint8_t  pll_0_lanes = 0;
    uint8_t  pll_1_lanes = 0;
    uint8_t  num_of_lanes = 0;
    uint8_t  rx_lane_list[8] = { 0xFF };
    uint8_t  tx_lane_list[8] = { 0xFF };

    PHYMOD_MEMSET(&pll_0_associated_lanes, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG4r_t));
    PHYMOD_MEMSET(&pll_1_associated_lanes, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG6r_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_internal_chip_rev(&phy->access, &chip_rev));

    /* Applicable only for B0 */
    if(chip_rev == BARCHETTA_CHIP_REV_INTERNAL) {
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));
        if (BARCHETTA_IS_LINE_SIDE(phy)) {
            pll_0_lanes = BCMI_BARCHETTA_CTRL_SWGPREG4r_SWGPREG4_DATAf_GET(pll_0_associated_lanes) & 0xFF;
            pll_1_lanes = BCMI_BARCHETTA_CTRL_SWGPREG6r_SWGPREG6_DATAf_GET(pll_1_associated_lanes) & 0xFF;
        } else {
            pll_0_lanes = ((BCMI_BARCHETTA_CTRL_SWGPREG4r_SWGPREG4_DATAf_GET(pll_0_associated_lanes) & 0xFF00) >> 8);
            pll_1_lanes = ((BCMI_BARCHETTA_CTRL_SWGPREG6r_SWGPREG6_DATAf_GET(pll_1_associated_lanes) & 0xFF00) >> 8);
        }
        /* Consider association of failover lanemap */
        if(pll_0_lanes & phy->access.lane_mask) {
            pll_0_lanes |= failover_lanemap;
        }
        /* Consider association of failover lanemap */
        if(pll_1_lanes & phy->access.lane_mask) {
            pll_1_lanes |= failover_lanemap;
        }
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_logical_lane_list_get(phy, &num_of_lanes, tx_lane_list, rx_lane_list));

        for(i=0; i<num_of_lanes; i++) {
            if(pll_0_lanes & (1 <<i)) {
                pll0_tx_lane_map |= (1 << tx_lane_list[i]) ;
                pll0_rx_lane_map |= (1 << rx_lane_list[i]) ;
            }
            if(pll_1_lanes & (1 <<i)) {
                pll1_tx_lane_map |= (1 << tx_lane_list[i]) ;
                pll1_rx_lane_map |= (1 << rx_lane_list[i]) ;
            }
        }

        pll_clk_buf  = ((pll0_rx_lane_map & 0xF0) ? 1 : 0) ;
        pll_clk_buf |= ((pll0_tx_lane_map & 0xF0) ? 1 : 0) << 1 ;
        pll_clk_buf |= ((pll1_rx_lane_map & 0x0F) ? 1 : 0) << 2 ;
        pll_clk_buf |= ((pll1_tx_lane_map & 0x0F) ? 1 : 0) << 3 ;
        for(pll=0; pll<2; pll++) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_set_slice_pll_index(phy, (int)pll));
            ESTM(ams_pll_rx_clk_rptr_pd = rdc_ams_pll_rx_clk_rptr_pd());
            ESTM(ams_pll_tx_clk_rptr_pd = rdc_ams_pll_tx_clk_rptr_pd());
            for(dir = 0; dir <=1; dir++) {
                if((pll_clk_buf >> (pll * 2)) & (1 << dir)) {
                    if(dir == 0) { /* dir == Rx */
                        if(ams_pll_rx_clk_rptr_pd) {
                            EFUN(wrc_ams_pll_rx_clk_rptr_pd(0));
                        }
                    } else { /* dir == Tx */
                        if(ams_pll_tx_clk_rptr_pd) {
                            EFUN(wrc_ams_pll_tx_clk_rptr_pd(0));
                        }
                    }
                } else {
                    if(dir == 0) { /* dir == Rx */
                        if(!ams_pll_rx_clk_rptr_pd) {
                            EFUN(wrc_ams_pll_rx_clk_rptr_pd(1));
                        }
                    } else { /* dir == Tx */
                        if(!ams_pll_tx_clk_rptr_pd) {
                            EFUN(wrc_ams_pll_tx_clk_rptr_pd(1));
                        }
                    }
                }
            }
            /* Following information is for debug purpose only */
            ESTM(ams_pll_rx_clk_rptr_pd = rdc_ams_pll_rx_clk_rptr_pd());
            ESTM(ams_pll_tx_clk_rptr_pd = rdc_ams_pll_tx_clk_rptr_pd());
            PHYMOD_DEBUG_INFO(("PHYID-0x%2x : %s : PLL-%d : lanes = 0x%2x : Rx buf = %d : Tx buf = %d\n",
            phy->access.addr, BARCHETTA_IS_LINE_SIDE(phy) ? "LIN": "SYS", pll, (pll == 0) ? pll_0_lanes:pll_1_lanes, ams_pll_rx_clk_rptr_pd, ams_pll_tx_clk_rptr_pd));
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_interface_config_set(const plp_barchetta_phymod_phy_access_t *phy,
        uint32_t flags, const plp_barchetta_phymod_phy_inf_config_t *config) {
    barchetta_aux_modes_t *aux_mode = (barchetta_aux_modes_t *) config->device_aux_modes;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    plp_barchetta_phymod_phy_access_t phy_temp;
    uint8_t allocated_port = 0xFF;
    barchetta_serdes_config_info_t serdes_cfg;
    barchetta_package_info_t pkg_info;
#ifdef BCM_PLP_RMT_PHY_CAPABILITY
    uint32_t phy_control = 0, an_control = 0;
#endif
    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_clock_config(phy, config));
    PHYMOD_IF_ERR_RETURN(
         _plp_barchetta_reg_write(&phy->access, GPREG_FOR_REF_CLK_OFFLINE, config->ref_clock));

    PHYMOD_IF_ERR_RETURN(
                  __plp_barchetta_get_serdes_config_info(phy, config, &serdes_cfg));
    PHYMOD_IF_ERR_RETURN(
            __plp_barchetta_save_lane_data_rate(phy, pkg_info, config));
    PHYMOD_IF_ERR_RETURN(
         _plp_barchetta_configure_pll(phy, config->ref_clock,serdes_cfg));

    /* Configure Interface type.*/
    PHYMOD_IF_ERR_RETURN(__plp_barchetta_configure_interface_type(phy, pkg_info, config));

    /* Allocate and configure port */
    PHYMOD_IF_ERR_RETURN(__plp_barchetta_allocate_and_configure_port(phy, config, serdes_cfg, &allocated_port));
#ifdef BCM_PLP_RMT_PHY_CAPABILITY
    if (aux_mode->phy_mode == BARCHETTA_PHY_MODE_LOCAL) {
        phy_control &= ~(0xFF);
        phy_control |= BARCHETTA_PHY_MODE_LOCAL;
    } else if(aux_mode->phy_mode == BARCHETTA_PHY_MODE_REMOTE) {
        phy_control &= ~(0xFF);
        phy_control |= BARCHETTA_PHY_MODE_REMOTE;
    } else {
        phy_control &= ~(0xFF);
    }
    if (config->data_rate == BARCHETTA_PORT_SPEED_10G) {
        phy_control |= BARCHETTA_RPHY_10G << 4 ;
    } else if (config->data_rate == BARCHETTA_PORT_SPEED_25G) {
        phy_control |= BARCHETTA_RPHY_25G << 4 ;
    } else if (config->data_rate == BARCHETTA_PORT_SPEED_50G) {
        phy_control |= BARCHETTA_RPHY_50G << 4 ;
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_write(&phy->access, GPREG_FW_PHY_CONTROL_ADR, phy_control));

    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_read(&phy->access, GPREG_FW_AN_CONTROL_ADR, &an_control));
    if (aux_mode->phy_mode == BARCHETTA_PHY_MODE_REMOTE) {
       an_control |= BARCHETTA_RMT_DISABLE_PRBS_AN;
    } else {
       an_control |= BARCHETTA_LOC_DISABLE_PRBS_AN;
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_write(&phy->access, GPREG_FW_AN_CONTROL_ADR, an_control));
#else
    /* Do nothing*/
    /* May need to update later*/
#endif
    /* Enable port if the port allocation is valid */
    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_enable_port(phy, allocated_port, config));
    } else if ((BARCHETTA_IS_LINE_SIDE(phy))&& (allocated_port != 0xFF)) {
        PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_configure_rptr_clock_buffer(phy, aux_mode->failover_lane_map));
        PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_barchetta_phymod_phy_access_t));
        phy_temp.port_loc = phymodPortLocSys;
        phy_temp.access.lane_mask = plp_barchetta_sw_db[pa->addr][0].sys_primary_port_lane_map;
        PHYMOD_IF_ERR_RETURN(
               __plp_barchetta_configure_rptr_clock_buffer(&phy_temp, plp_barchetta_sw_db[pa->addr][0].port_cfg_aux_sys.failover_lane_map));
        PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_enable_port(phy, allocated_port, config));
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid port:%x\n", allocated_port));
        return PHYMOD_E_INTERNAL;
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_get_tx_drv_mode(phy, 0, pkg_info, config->device_aux_modes));
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    To get low latency mode
 \param    phy        [In]  Pointer to port configuration structure
 \param    ll_mode    [Out] Pointer to low latency mode
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int
__plp_barchetta_low_latency_mode_get(const plp_barchetta_phymod_phy_access_t *phy, int port_number,
        unsigned char *ll_mode) {
    BCMI_BARCHETTA_DP_TX_LANE_GBOX_CFGr_t gbox_cfg;
    plp_barchetta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(plp_barchetta_phymod_phy_access_t));

    *ll_mode = 0;
    PHYMOD_MEMSET(&gbox_cfg, 0, sizeof(BCMI_BARCHETTA_DP_TX_LANE_GBOX_CFGr_t));
    phy_copy.port_loc = phymodPortLocSys;
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(&phy_copy, port_number, 1 , BarchettaRegisterSelectIngress,
            BarchettaRegisterTypePMD, 0xFFFF));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_DP_TX_LANE_GBOX_CFGr(&phy->access, &gbox_cfg));
    if (BCMI_BARCHETTA_DP_TX_LANE_GBOX_CFGr_MAPf_GET(gbox_cfg)) {
        *ll_mode = 1;
    }
    phy_copy.port_loc = phymodPortLocLine;
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(&phy_copy, port_number,1 , BarchettaRegisterSelectEgress,
            BarchettaRegisterTypePMD, 0xFFFF));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_DP_TX_LANE_GBOX_CFGr(&phy->access, &gbox_cfg));

    if (BCMI_BARCHETTA_DP_TX_LANE_GBOX_CFGr_MAPf_GET(gbox_cfg)) {
        *ll_mode |= (1 << 1);
    }
    return PHYMOD_E_NONE;
}

/***************************************************************************//**
 \brief    To get the port config information
 \param    phy       [In]  Pointer to port configuration structure
 \param    if_type   [Out] Pointer to reference clock used
 \return   Status information
 \retval   PHYMOD_E_NONE
*******************************************************************************/
int
__plp_barchetta_port_config_get(const plp_barchetta_phymod_phy_access_t *phy,
         barchetta_package_info_t pkg_info,
        plp_barchetta_phymod_phy_inf_config_t *config) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    struct blackhawk_barchetta_uc_core_config_st core_config_get ;
    struct blackhawk_barchetta_uc_lane_config_st lane_config_get ;
    barchetta_port_config_t port_cfg_rd ;
    int port_number  ;
    uint32_t vco_rate_in_Mhz = 25312;
    barchetta_aux_modes_t device_aux_mode;
    barchetta_lane_data_rate_t   lane_data_rate_from_gp_reg ;
    int lane_data_rate_from_bhwk_reg ;
    uint32_t rx_osr = 0, osr_mode = 1;
    uint8_t lane = 0 ;
    uint8_t logical_lane=0, num_of_max_lanes = 0;
    uint8_t num_of_lanes = 0;
#ifdef BCM_PLP_RMT_PHY_CAPABILITY
    uint32_t phy_control = 0 ;
#endif

    PHYMOD_MEMSET(&device_aux_mode, 0, sizeof(barchetta_aux_modes_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane = 0; lane < num_of_max_lanes; lane++) {
        if (phy->access.lane_mask & (1 << lane)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_get_uc_core_config(pa, &core_config_get));

            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_get_uc_lane_cfg(pa, &lane_config_get));

            vco_rate_in_Mhz = (uint32_t)(core_config_get.vco_rate_in_Mhz /1000)*1000;

            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_restore_lane_data_rate(phy, lane, &lane_data_rate_from_gp_reg));

            PHYMOD_IF_ERR_RETURN(
                        __plp_barchetta_interface_type_get(phy, lane, &(config->interface_type)));

            PHYMOD_IF_ERR_RETURN(
                     _plp_barchetta_reg_read(&phy->access, 0x1d1c0 ,&rx_osr));

             if ((rx_osr & 0x4000) && ((rx_osr & 0x70) == 0x10)) {
                 lane_data_rate_from_bhwk_reg = (int)core_config_get.vco_rate_in_Mhz;
                 if(lane_config_get.field.force_pam4_mode) {
                     lane_data_rate_from_bhwk_reg <<=  1 ;
                 }
             } else {
                 if ((rx_osr & 0xF) == 0) {
                     osr_mode=1;
                 } else if ((rx_osr & 0xF) == 1) {
                     osr_mode=2;
                 } else if ((rx_osr & 0xF) == 2) {
                     osr_mode=4;
                 } else if ((rx_osr & 0xF) == 4) {
                     osr_mode=21;
                 } else if ((rx_osr & 0xF) == 8) {
                     osr_mode=16; /* Its suppose to 16.5, But no problem as we do div but 1000*/
                 } else if ((rx_osr & 0xF) == 12) {
                     osr_mode=20; /* Its suppose to 20.625, But no problem as we do div but 1000*/
                 } else if ((rx_osr & 0xF) == 5) {
                     osr_mode=8;
                 } else if ((rx_osr & 0xF) == 9) {
                     osr_mode=16;
                 } else if ((rx_osr & 0xF) == 13) {
                     osr_mode=32;
                 } else {
                     osr_mode=1;
                 }
                lane_data_rate_from_bhwk_reg = (int)(core_config_get.vco_rate_in_Mhz/osr_mode);
             }
            device_aux_mode.lane_data_rate = ((lane_data_rate_from_gp_reg/1000) == (lane_data_rate_from_bhwk_reg/1000)) ?
                                             lane_data_rate_from_gp_reg :
                                             (barchetta_lane_data_rate_t)lane_data_rate_from_bhwk_reg ;

            device_aux_mode.modulation_mode = lane_config_get.field.force_pam4_mode? BARCHETTA_MODULATION_PAM4:BARCHETTA_MODULATION_NRZ;
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(&phy->access, GPREG_FOR_REF_CLK_OFFLINE, &config->ref_clock));
            break;
        }
    }
    config->interface_modes = 0 ;
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));

    if(BARCHETTA_MSG_IF_RET_SUCCESS == _plp_barchetta_msg_config_port_rd(&phy->access, port_number, &port_cfg_rd)) {
        if(port_cfg_rd.port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
            if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
                num_of_lanes = port_cfg_rd.port_lanes.duplex.num_sys_lanes ;
                if((port_cfg_rd.port_mode == BARCHETTA_PORT_MODE_FAILOVER) &&
                   (port_cfg_rd.port_lanes.duplex.num_sys_lanes == 2*port_cfg_rd.port_lanes.duplex.num_line_lanes)) {
                    PHYMOD_IF_ERR_RETURN(
                    __plp_barchetta_convert_lane_list_to_lane_map(pa,
                            (port_cfg_rd.port_lanes.duplex.sys_lane_list + (port_cfg_rd.port_lanes.duplex.num_sys_lanes/2)),
                            (port_cfg_rd.port_lanes.duplex.num_sys_lanes/2), &device_aux_mode.failover_lane_map));
                } else {
                    device_aux_mode.failover_lane_map = 0;
                }
            } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
                num_of_lanes = port_cfg_rd.port_lanes.duplex.num_line_lanes ;
                if((port_cfg_rd.port_mode == BARCHETTA_PORT_MODE_FAILOVER) &&
                   (port_cfg_rd.port_lanes.duplex.num_line_lanes == 2*port_cfg_rd.port_lanes.duplex.num_sys_lanes)) {
                    PHYMOD_IF_ERR_RETURN(
                    __plp_barchetta_convert_lane_list_to_lane_map (pa,
                            (port_cfg_rd.port_lanes.duplex.line_lane_list + (port_cfg_rd.port_lanes.duplex.num_line_lanes/2)),
                            (port_cfg_rd.port_lanes.duplex.num_line_lanes/2), &device_aux_mode.failover_lane_map));
                } else {
                    device_aux_mode.failover_lane_map = 0;
                }
            }

            if (device_aux_mode.lane_data_rate >= 50000) {
                config->data_rate = num_of_lanes * 50000 ;
            } else if((device_aux_mode.lane_data_rate >= 25000) && (device_aux_mode.lane_data_rate < 32500)) {
                config->data_rate = num_of_lanes * 25000 ;
            } else {
                config->data_rate = num_of_lanes * (vco_rate_in_Mhz/osr_mode) ;
                /* Doing this to make sure data rate should not exceed lane datarate*/
                if ((config->data_rate > device_aux_mode.lane_data_rate) && (num_of_lanes == 1)) {
                    config->data_rate = device_aux_mode.lane_data_rate;
                }
                if(device_aux_mode.modulation_mode == BARCHETTA_MODULATION_PAM4) {
                    config->data_rate <<= 1 ;
                }
            }
            /* FC Data Rates */
            if((device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_1P0625G)   ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_2P125G)    ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_4P25G)     ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_8P5G)      ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_9P95328G)  ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_10P51875G) ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_14P025G)   ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_28P05G)    ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_56P1G)) {
                config->data_rate = device_aux_mode.lane_data_rate * num_of_lanes ;
                config->interface_modes = PHYMOD_INTF_MODES_FIBER ;
            }
            /* OTN Data rates */
            else if((device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_10P70922G) ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_10P75460G) ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_11P04908G) ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_11P09568G) ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_11P181G)   ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_27P9525G)) {
                config->data_rate = device_aux_mode.lane_data_rate * num_of_lanes ;
                config->interface_modes = PHYMOD_INTF_MODES_OTN ;
            }
            /* HIGIG Data rates */
            else if((device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_10P9375G) ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_21P875G)  ||
               (device_aux_mode.lane_data_rate == BARCHETTA_LANE_DATA_RATE_27P34375G)) {
                config->interface_modes = PHYMOD_INTF_MODES_HIGIG ;
                if(device_aux_mode.lane_data_rate != BARCHETTA_LANE_DATA_RATE_27P34375G) {
                    config->data_rate = ((config->data_rate / 10000) * 10000) ;
                }
            }
            /* Any other data rate */
            else {
                config->data_rate = ((config->data_rate / 1000) * 1000) ;
            }

            if((port_cfg_rd.port_mode == BARCHETTA_PORT_MODE_FAILOVER) && (device_aux_mode.failover_lane_map != 0)){
                config->data_rate >>= 1 ;
            }

            PHYMOD_IF_ERR_RETURN(__plp_barchetta_low_latency_mode_get(phy, port_number, &device_aux_mode.ll_mode));
            device_aux_mode.clock_mode = 0;
#ifdef BCM_PLP_RMT_PHY_CAPABILITY
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(&phy->access, GPREG_FW_PHY_CONTROL_ADR, &phy_control));
            if((phy_control & 0xF) == BARCHETTA_PHY_MODE_LOCAL) {
                device_aux_mode.phy_mode = BARCHETTA_PHY_MODE_LOCAL ;
            } else if((phy_control & 0xF) == BARCHETTA_PHY_MODE_REMOTE) {
                device_aux_mode.phy_mode = BARCHETTA_PHY_MODE_REMOTE ;
            } else if((phy_control & 0xF) == BARCHETTA_PHY_MODE_GENERAL) {
                device_aux_mode.phy_mode = BARCHETTA_PHY_MODE_GENERAL ;
            }
#endif
            PHYMOD_MEMCPY(config->device_aux_modes, &device_aux_mode, sizeof(barchetta_aux_modes_t));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_set_get_tx_drv_mode(phy, 1, pkg_info, config->device_aux_modes));

        } else if(port_cfg_rd.port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
            
        } else if(port_cfg_rd.port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
            
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_interface_config_get(const plp_barchetta_phymod_phy_access_t *phy,
        uint32_t flags, plp_barchetta_phymod_phy_inf_config_t *config) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));

    PHYMOD_IF_ERR_RETURN(
            __plp_barchetta_port_config_get(phy, pkg_info, config));

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_rx_pmd_locked_get(const plp_barchetta_phymod_phy_access_t* phy,
        uint32_t* rx_pmd_locked) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t serdes_pmd_lock = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;
    *rx_pmd_locked = 0xFFFF;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_pmd_lock_status(&phy->access,&serdes_pmd_lock));
            *rx_pmd_locked &= serdes_pmd_lock;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_pam4_tx_set(const plp_barchetta_phymod_phy_access_t* phy,
        const phymod_pam4_tx_t* tx) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_apply_txfir_cfg(&phy->access,
                        (enum blackhawk_barchetta_txfir_tap_enable_enum )(tx->serdes_tx_tap_mode- 1),
                    tx->pre2, tx->pre, tx->main, tx->post,tx->post2, tx->post3));
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_pam4_tx_get(const plp_barchetta_phymod_phy_access_t* phy,
        phymod_pam4_tx_t* tx) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    int16_t tap_val[6] = { 0 };
    int tap_index = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            for (tap_index = 0; tap_index < 6; tap_index++) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_read_tx_afe(&phy->access, tap_index, &tap_val[tap_index]));
            }
            if ((tx->serdes_tx_tap_mode == phymodTxTapModeNRZ_LP_3TAP) ||
                (tx->serdes_tx_tap_mode == phymodTxTapModePAM4_LP_3TAP)) {
                tx->pre = tap_val[0];
                tx->main = tap_val[1];
                tx->post = tap_val[2];
                tx->pre2 = 0;
                tx->post2 = 0;
                tx->post3 = 0;
            } else {
                tx->pre2 = tap_val[0];
                tx->pre = tap_val[1];
                tx->main = tap_val[2];
                tx->post = tap_val[3];
                tx->post2 = tap_val[4];
                tx->post3 = tap_val[5];
            }
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_rx_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_rx_t* rx) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    uint8_t idx = 0;
    int port_number = 0xFF;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));

    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid Port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1,
            BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, 0xffff));
    PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_init_blackhawk_barchetta_info(&phy->access));

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

            if (rx->vga.enable) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_stop_rx_adaptation(&phy->access, 1));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_write_rx_afe(&phy->access, RX_AFE_VGA, rx->vga.value));
            }
            for (idx = 0; idx < rx->num_of_dfe_taps; idx++) {
                if (idx == 0) {
                    uint8_t rx_pam4_mode = 0;
                    ESTM(rx_pam4_mode    = rd_rx_pam4_mode());
                    if (rx_pam4_mode != 0) {
                        continue;
                    }
                }
                if (rx->dfe[idx].enable) {
                    PHYMOD_IF_ERR_RETURN(
                            plp_barchetta_blackhawk_barchetta_stop_rx_adaptation(&phy->access, 1));
                    PHYMOD_IF_ERR_RETURN(
                            plp_barchetta_blackhawk_barchetta_write_rx_afe(&phy->access, RX_AFE_DFE1 + idx, rx->dfe[idx].value));
                }
            }
            if (rx->peaking_filter.enable) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_stop_rx_adaptation(&phy->access, 1));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_write_rx_afe(&phy->access, RX_AFE_PF, rx->peaking_filter.value));
            }
            if (rx->low_freq_peaking_filter.enable) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_stop_rx_adaptation(&phy->access, 1));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_write_rx_afe(&phy->access, RX_AFE_PF2, rx->low_freq_peaking_filter.value));
            }
            if (rx->high_freq_peaking_filter.enable) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_stop_rx_adaptation(&phy->access, 1));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_write_rx_afe(&phy->access, RX_AFE_PF3, rx->high_freq_peaking_filter.value));
            }
            if (rx->peaking_filter.enable) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_stop_rx_adaptation(&phy->access, 1));
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_write_rx_afe(&phy->access, RX_AFE_PF, rx->peaking_filter.value));
            }
            /* Black Hawk Dont support it*/
            /*if (rx->ffe1.enable) {
             PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_stop_rx_adaptation(&phy->access, 1));
             PHYMOD_IF_ERR_RETURN(
             plp_barchetta_blackhawk_barchetta_write_rx_afe(&phy->access, RX_FFE_1, rx->ffe1.value));
             }
             if (rx->ffe2.enable) {
             PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_stop_rx_adaptation(&phy->access, 1));
             PHYMOD_IF_ERR_RETURN(
             plp_barchetta_blackhawk_barchetta_write_rx_afe(&phy->access, RX_FFE_2, rx->ffe2.value));
             }*/
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_rx_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_rx_t* rx) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    uint8_t idx = 0;
    int8_t vga = 0, peaking_filter = 0, low_freq = 0, high_freq = 0, dfe = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_read_rx_afe(&phy->access, RX_AFE_VGA, &vga));
            rx->vga.value = vga;
            rx->vga.enable = 0;
            for (idx = 0; idx < 14; idx++) {
                if (idx == 0) {
                    uint8_t rx_pam4_mode = 0;
                    ESTM(rx_pam4_mode    = rd_rx_pam4_mode());
                    if (rx_pam4_mode != 0) {
                        rx->dfe[idx].value = 0xDEAD;
                        continue;
                    }
                }
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_read_rx_afe(&phy->access, RX_AFE_DFE1 + idx, &dfe));
                rx->dfe[idx].value = dfe;
                rx->dfe[idx].enable = 0;
            }
            rx->num_of_dfe_taps = 14;
            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_read_rx_afe(&phy->access, RX_AFE_PF, &peaking_filter));
            rx->peaking_filter.enable = 0;
            rx->peaking_filter.value = peaking_filter;
            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_read_rx_afe(&phy->access, RX_AFE_PF2, &low_freq));
            rx->low_freq_peaking_filter.enable = 0;
            rx->low_freq_peaking_filter.value = low_freq;
            PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_read_rx_afe(&phy->access, RX_AFE_PF3, &high_freq));
            rx->high_freq_peaking_filter.value=high_freq;
            rx->high_freq_peaking_filter.enable = 0;
            /* Black Hawk Dont support it*/
            /*PHYMOD_IF_ERR_RETURN(
             plp_barchetta_blackhawk_barchetta_read_rx_afe(&phy->access, RX_FFE_1, (int8_t*)&rx->ffe1.value));
             rx->ffe1.enable=0;
             PHYMOD_IF_ERR_RETURN(
             plp_barchetta_blackhawk_barchetta_read_rx_afe(&phy->access, RX_FFE_2, (int8_t*)&rx->ffe2.value));
             rx->ffe2.enable=0;*/
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_polarity_set(const plp_barchetta_phymod_phy_access_t* phy,
        const plp_barchetta_phymod_polarity_t* polarity) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    barchetta_config_polarity_swap_t polarity_cfg_set;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    uint8_t configured_polarity = 0;
    err_code_t __err = 0;
    plp_barchetta_phymod_polarity_t pol_get;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&pol_get, 0, sizeof(plp_barchetta_phymod_polarity_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    PHYMOD_MEMSET(&polarity_cfg_set, 0,
            sizeof(barchetta_config_polarity_swap_t));

    PHYMOD_IF_ERR_RETURN(
         _plp_barchetta_phy_polarity_get(phy, &pol_get));
    pol_get.tx_polarity ^= polarity->tx_polarity;
    pol_get.rx_polarity ^= polarity->rx_polarity;

    pol_get.tx_polarity &= phy->access.lane_mask;
    pol_get.rx_polarity &= phy->access.lane_mask;

    /* Write the effective polarity to Blackhawk register */
    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            if (polarity->tx_polarity != 0xFFFF) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                            BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                configured_polarity = rd_tx_pmd_dp_invert();
                if (__err != 0) {
                    PHYMOD_DEBUG_ERROR(("Polarity failed\n"));
                    return __err;
                }
                if ((pol_get.tx_polarity >> lane_index) & 1) {
                    PHYMOD_IF_ERR_RETURN(
                           wr_tx_pmd_dp_invert(!configured_polarity));
                }
            }
            if (polarity->rx_polarity != 0xFFFF) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                            BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                configured_polarity = rd_rx_pmd_dp_invert();
                if (__err != 0) {
                    PHYMOD_DEBUG_ERROR(("Polarity failed\n"));
                    return __err;
                }

                if ((pol_get.rx_polarity >> lane_index) & 1) {
                PHYMOD_IF_ERR_RETURN(
                        wr_rx_pmd_dp_invert(!configured_polarity));
                }
            }
        }
    }

    if (pkg_info.pkg_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        barchetta_duplex_polarity_swap_t *p_duplex_polarity_swap_set = &(polarity_cfg_set.polarity_swap.duplex);

        if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
            p_duplex_polarity_swap_set->num_sys_lanes    = num_of_max_lanes;
            if (polarity->rx_polarity != 0xFFFF) {
                p_duplex_polarity_swap_set->sr_polarity_swap = ((pol_get.rx_polarity) & 0xFF);
            }
            if (polarity->tx_polarity != 0xFFFF) {
                p_duplex_polarity_swap_set->st_polarity_swap = ((pol_get.tx_polarity) & 0xFF);
            }
            p_duplex_polarity_swap_set->num_line_lanes   = num_of_max_lanes;
            p_duplex_polarity_swap_set->lr_polarity_swap = 0;
            p_duplex_polarity_swap_set->lt_polarity_swap = 0;
        } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
            p_duplex_polarity_swap_set->num_line_lanes   = num_of_max_lanes;
            if (polarity->rx_polarity != 0xFFFF) {
                p_duplex_polarity_swap_set->lr_polarity_swap = ((pol_get.rx_polarity) & 0xFF);
            }
            if (polarity->tx_polarity != 0xFFFF) {
                p_duplex_polarity_swap_set->lt_polarity_swap = ((pol_get.tx_polarity) & 0xFF);
            }
            p_duplex_polarity_swap_set->num_sys_lanes    = num_of_max_lanes;
            p_duplex_polarity_swap_set->sr_polarity_swap = 0;
            p_duplex_polarity_swap_set->st_polarity_swap = 0;
        }
    } else if (polarity_cfg_set.config_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        
    } else if (polarity_cfg_set.config_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        
    }

    if (BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_config_polarity_swap_wr(sa__, &polarity_cfg_set)) {
        PHYMOD_DEBUG_ERROR(
                ("Polarity configuration Failed Due to CONFIG_PLOLARITY_SWAP.Write() Error\n"));
        return PHYMOD_E_INTERNAL;
    }

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_polarity_get(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_polarity_t* polarity) {
    barchetta_config_polarity_swap_t polarity_cfg;
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    PHYMOD_MEMSET(&polarity_cfg, 0, sizeof(barchetta_config_polarity_swap_t));
    if (BARCHETTA_MSG_IF_RET_SUCCESS == _plp_barchetta_msg_config_polarity_swap_rd(sa__, &polarity_cfg)) {
        if (polarity_cfg.config_type == BARCHETTA_PORT_TYPE_DUPLEX) {
            barchetta_duplex_polarity_swap_t *p_duplex_polarity_swap = &(polarity_cfg.polarity_swap.duplex);
            if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
                polarity->rx_polarity = (p_duplex_polarity_swap->sr_polarity_swap & phy->access.lane_mask);
                polarity->tx_polarity = (p_duplex_polarity_swap->st_polarity_swap & phy->access.lane_mask);
            } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
                polarity->rx_polarity = (p_duplex_polarity_swap->lr_polarity_swap & phy->access.lane_mask);
                polarity->tx_polarity = (p_duplex_polarity_swap->lt_polarity_swap & phy->access.lane_mask);
            }
        } else if (polarity_cfg.config_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
            
        } else if (polarity_cfg.config_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
            
        }
    } else {
        PHYMOD_DEBUG_ERROR(
                ("Polarity configuration read Failed Due to CONFIG_PLOLARITY_SWAP.Read() Error\n"));
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_reset_set(const plp_barchetta_phymod_phy_access_t* phy,
        const plp_barchetta_phymod_phy_reset_t* reset) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

            if (reset->tx == phymodResetDirectionIn) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_tx_dp_reset(&phy->access, 1));
            }
            if (reset->tx == phymodResetDirectionOut) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_tx_dp_reset(&phy->access, 0));
            }
            if (reset->tx == phymodResetDirectionInOut) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_tx_dp_reset(&phy->access, 1));
                PHYMOD_USLEEP(10);
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_tx_dp_reset(&phy->access, 0));
            }
            /* Perform Rx resets*/
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            if (reset->rx == phymodResetDirectionIn) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_rx_dp_reset(&phy->access, 1));
            }
            if (reset->rx == phymodResetDirectionOut) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_rx_dp_reset(&phy->access, 0));
            }
            if (reset->rx == phymodResetDirectionInOut) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_rx_dp_reset(&phy->access, 1));
                PHYMOD_USLEEP(10);
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_rx_dp_reset(&phy->access, 0));
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_reset_get(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_phy_reset_t* reset) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    err_code_t __err = 0;
    int ln_reset = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            ln_reset = rd_tx_ln_dp_s_rstb();
            if (__err != 0) {
                PHYMOD_DEBUG_ERROR(("Read Tx lane reset failed\n"));
                return __err;
            }

            reset->tx = (ln_reset == 0) ? phymodResetDirectionIn : phymodResetDirectionOut;

            /* Get Rx resets*/
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            ln_reset = rd_rx_ln_dp_s_rstb();
            if (__err != 0) {
                PHYMOD_DEBUG_ERROR(("Read Rx lane reset failed\n"));
                return __err;
            }

            reset->rx = (ln_reset == 0) ? phymodResetDirectionIn : phymodResetDirectionOut;
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_power_set(const plp_barchetta_phymod_phy_access_t* phy,
        const plp_barchetta_phymod_phy_power_t* power) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            if (power->tx != phymodPowerNoChange) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                            BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                if (power->tx == phymodPowerOff || power->tx == phymodPowerOn) {
                    PHYMOD_IF_ERR_RETURN(
                            (wr_ln_tx_s_pwrdn((power->tx == phymodPowerOff) ? 1 : 0)));
                }
                if (power->tx == phymodPowerOffOn) {
                    PHYMOD_IF_ERR_RETURN((wr_ln_tx_s_pwrdn(1)));
                    PHYMOD_USLEEP(10);
                    PHYMOD_IF_ERR_RETURN((wr_ln_tx_s_pwrdn(0)));
                }
            }
            if (power->rx != phymodPowerNoChange) {
                /*Rx Power*/
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                            BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
                if (power->rx == phymodPowerOff || power->rx == phymodPowerOn) {
                    PHYMOD_IF_ERR_RETURN(
                            (wr_ln_rx_s_pwrdn((power->rx == phymodPowerOff) ? 1 : 0)));
                }
                if (power->rx == phymodPowerOffOn) {
                    PHYMOD_IF_ERR_RETURN((wr_ln_rx_s_pwrdn(1)));
                    PHYMOD_USLEEP(10);
                    PHYMOD_IF_ERR_RETURN((wr_ln_rx_s_pwrdn(0)));
                }
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_power_get(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_phy_power_t* power) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes  = 0;
    uint32_t serdes_power = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    err_code_t __err = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            serdes_power = rd_ln_tx_s_pwrdn();
            if (__err != 0) {
                PHYMOD_DEBUG_ERROR(("Read lane Tx pwrdn failed\n"));
                return __err;
            }
            power->tx = (serdes_power ? phymodPowerOff : phymodPowerOn);

            /*Rx Power*/
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            serdes_power = rd_ln_rx_s_pwrdn();
            if (__err != 0) {
                PHYMOD_DEBUG_ERROR(("Read lane Rx pwrdn failed\n"));
                return __err;
            }
            power->rx = (serdes_power ? phymodPowerOff : phymodPowerOn);
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_tx_lane_control_set(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_phy_tx_lane_control_t tx_control) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            if (tx_control == phymodTxSquelchOn || tx_control == phymodTxSquelchOff) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_tx_disable(&phy->access, (tx_control == phymodTxSquelchOn) ? 1 : 0));
            } else if (tx_control == phymodTxReset) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_tx_dp_reset(&phy->access, 1));
                PHYMOD_USLEEP(5);
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_tx_dp_reset(&phy->access, 0));
            } else {
                PHYMOD_DEBUG_ERROR(("ERROR: Invalid tx_lane_control option\n"));
                return PHYMOD_E_UNAVAIL;
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_tx_lane_control_get(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_phy_tx_lane_control_t* tx_control) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    err_code_t __err = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

            *tx_control = ( rd_sdk_tx_disable() ? phymodTxSquelchOn : phymodTxSquelchOff ) ;
            if (__err != 0) {
                PHYMOD_DEBUG_ERROR(("Read lane Tx disable failed\n"));
                return __err;
            }

            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_rx_lane_control_set(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_phy_rx_lane_control_t rx_control) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            if (rx_control == phymodRxSquelchOn || rx_control == phymodRxSquelchOff) {
                PHYMOD_IF_ERR_RETURN(
                        wr_ln_rx_s_pwrdn((rx_control == phymodRxSquelchOn) ? 1 : 0));
            } else if (rx_control == phymodRxReset) {
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_rx_dp_reset(&phy->access, 1));
                PHYMOD_USLEEP(5);
                PHYMOD_IF_ERR_RETURN(
                        plp_barchetta_blackhawk_barchetta_rx_dp_reset(&phy->access, 0));
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_rx_lane_control_get(const plp_barchetta_phymod_phy_access_t *phy,
        plp_barchetta_phymod_phy_rx_lane_control_t* rx_control) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes  = 0;
    uint32_t serdes_power = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    err_code_t __err = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            serdes_power = rd_ln_rx_s_pwrdn();
            if (__err != 0) {
                PHYMOD_DEBUG_ERROR(("Read lane Rx powerdown failed\n"));
                return __err;
            }
            *rx_control = (serdes_power ? phymodRxSquelchOn : phymodRxSquelchOff);
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_lpbk_control(const plp_barchetta_phymod_phy_access_t *phy, plp_barchetta_phymod_loopback_mode_t loopback, uint32_t enable)
{
    const uint8_t sys_tx_die_lane_81338[8] = {2,1,3,0,4,7,5,6};
    const uint8_t sys_rx_die_lane_81338[8] = {0,1,2,3,7,4,6,5};
    const uint8_t line_tx_die_lane_81338[8] = {0,2,1,3,7,4,6,5};
    const uint8_t line_rx_die_lane_81338[8] = {2,1,3,0,4,7,6,5};
    const uint8_t die_lane_81381[8] = {0,1,2,3,7,6,5,4};
    uint8_t num_of_max_lanes  = 0;
    barchetta_package_info_t pkg_info;
    uint8_t no_of_lanes = 0;
    uint8_t tx_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];
    uint8_t rx_list[BARCHETTA_MAX_NUM_SIMPLEX_LANES];
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    int chip_id = 0, lane_index = 0;
    uint8_t logical_lane=0, pkg_lane = 0, die_lane = 0;
    plp_barchetta_phymod_phy_access_t phy1;

    PHYMOD_MEMCPY(&phy1, phy, sizeof(plp_barchetta_phymod_phy_access_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;
    PHYMOD_IF_ERR_RETURN(
             _plp_barchetta_get_chip_id(&phy->access, &chip_id));
    /* Got package lane*/
    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_logical_lane_list_get(&phy1, &no_of_lanes, tx_list, rx_list));
    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            if (loopback == phymodLoopbackRemotePMD) {
                /* Get package lane from virtual lane*/
                pkg_lane = rx_list[lane_index];
                if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
                    /*Convert pkg to die*/
                    if ((chip_id == BARCHETTA_CHIP_81381) || (chip_id == BARCHETTA_CHIP_81321)) {
                        die_lane = die_lane_81381[pkg_lane] ;
                    } else {
                        die_lane = sys_rx_die_lane_81338[pkg_lane] ;
                    }
                    PHYMOD_IF_ERR_RETURN(wr_rlpbk_sel_frc_val(die_lane));
                } else {
                    /*Convert pkg to die*/
                    if ((chip_id == BARCHETTA_CHIP_81381) || (chip_id == BARCHETTA_CHIP_81321)) {
                        die_lane = die_lane_81381[pkg_lane] ;
                    } else {
                        die_lane = line_rx_die_lane_81338[pkg_lane] ;
                    }
                    PHYMOD_IF_ERR_RETURN(
                        wr_rlpbk_sel_frc_val(die_lane));
                }
                PHYMOD_IF_ERR_RETURN(
                    wr_rlpbk_sel_frc(enable));
            } else {
                /* Get package lane from virtual lane*/
                pkg_lane = tx_list[lane_index];
                if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
                    /*Convert pkg to die*/
                    if ((chip_id == BARCHETTA_CHIP_81381) || (chip_id == BARCHETTA_CHIP_81321)) {
                        die_lane = die_lane_81381[pkg_lane] ;
                    } else {
                        die_lane = sys_tx_die_lane_81338[pkg_lane] ;
                    }
                    PHYMOD_IF_ERR_RETURN(
                        wr_dlpbk_sel_frc_val(die_lane));
                } else {
                    /*Convert pkg to die*/
                    if ((chip_id == BARCHETTA_CHIP_81381) || (chip_id == BARCHETTA_CHIP_81321)) {
                        die_lane = die_lane_81381[pkg_lane] ;
                    } else {
                        die_lane = line_tx_die_lane_81338[pkg_lane] ;
                    }
                    PHYMOD_IF_ERR_RETURN(
                        wr_dlpbk_sel_frc_val(die_lane));
                }
                PHYMOD_IF_ERR_RETURN(
                    wr_dlpbk_sel_frc(enable));
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_rmt_loop_back(const plp_barchetta_phymod_phy_access_t *phy, uint32_t enable)
{
    barchetta_package_info_t pkg_info;
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    int lane_index = 0;
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    for (lane_index = 0; lane_index < pkg_info.pkg_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
          PHYMOD_IF_ERR_RETURN(
               _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                       BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, lane_index));
          EFUN(wr_tx_pi_loop_timing_src_sel(enable));
          PHYMOD_IF_ERR_RETURN(
             plp_barchetta_blackhawk_barchetta_rmt_lpbk(&phy->access, enable));
       }
   }
   return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_loopback_set(const plp_barchetta_phymod_phy_access_t *phy,
        plp_barchetta_phymod_loopback_mode_t loopback, uint32_t enable) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    plp_barchetta_phymod_phy_access_t phy_temp;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    int port_number = 0xFF;
    uint8_t msg_if_status = BARCHETTA_MSG_IF_RET_SUCCESS ;
    uint8_t msg_port_num = 0;
    uint8_t checker_auto_mode = 0 ;
    uint8_t num_of_lanes_lpbk_side  = 0 ;
    uint8_t num_of_lanes_other_side = 0 ;
    err_code_t __err = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_barchetta_phymod_phy_access_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
       PHYMOD_DEBUG_ERROR(("ERROR: Invalid port number 0xFF\n"));
       return PHYMOD_E_PARAM;
    }
    msg_port_num = (port_number & 0xFF);

    if(enable) {
        msg_if_status = _plp_barchetta_msg_pause_port_wr(sa__, msg_port_num) ;
        if(msg_if_status != BARCHETTA_MSG_IF_RET_SUCCESS) {
            PHYMOD_DEBUG_ERROR(("Pausing a port Failed Due to PAUSE_PORT.Write() Error\n"));
            return PHYMOD_E_INTERNAL;
        }
    }

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_lpbk_control(phy, loopback, enable));

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, logical_lane));

            if (loopback == phymodLoopbackRemotePMD) {
                PHYMOD_IF_ERR_RETURN(_plp_barchetta_rmt_loop_back(phy, enable));
            } else {
                PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_dig_lpbk_rptr(&phy->access, enable, DIG_LPBK_SIDE));

                checker_auto_mode = rd_prbs_chk_en_auto_mode();
                if (__err != 0) {
                    PHYMOD_DEBUG_ERROR(("Reading checker auto mode failed\n"));
                    return PHYMOD_E_CONFIG;
                }
                if ((checker_auto_mode == 1) && (enable == 1)) {
                    EFUN(wr_prbs_chk_en_auto_mode(0x0));
                } else {
                    EFUN(wr_prbs_chk_en_auto_mode(0x1));
                }
                if(BARCHETTA_IS_SYSTEM_SIDE(phy)) {
                    num_of_lanes_lpbk_side  = plp_barchetta_sw_db[sa__->addr][msg_port_num].port_cfg_info.port_lanes.duplex.num_sys_lanes  ;
                    num_of_lanes_other_side = plp_barchetta_sw_db[sa__->addr][msg_port_num].port_cfg_info.port_lanes.duplex.num_line_lanes ;
                } else if(BARCHETTA_IS_LINE_SIDE(phy)) {
                    num_of_lanes_lpbk_side  = plp_barchetta_sw_db[sa__->addr][msg_port_num].port_cfg_info.port_lanes.duplex.num_line_lanes ;
                    num_of_lanes_other_side = plp_barchetta_sw_db[sa__->addr][msg_port_num].port_cfg_info.port_lanes.duplex.num_sys_lanes  ;
                }

                if(num_of_lanes_lpbk_side > num_of_lanes_other_side) {
                    EFUN(wr_tlb_rx_nrz_ll_mode_en(!enable));
                }
                if(num_of_lanes_lpbk_side < num_of_lanes_other_side) {
                   EFUN(_plp_barchetta_blackhawk_barchetta_pmd_mwr_reg_byte(sa__, 0xd052, 0x3800, 11, enable));
                }
            }
        }
    }

    if(BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        phy_temp.port_loc = phymodPortLocLine;
        phy_temp.access.lane_mask = plp_barchetta_sw_db[sa__->addr][msg_port_num].lane_map_info.lane_map.duplex_lane_map.line_lane_map ;
    } else if(BARCHETTA_IS_LINE_SIDE(phy)) {
        phy_temp.port_loc = phymodPortLocSys;
        phy_temp.access.lane_mask = plp_barchetta_sw_db[sa__->addr][msg_port_num].lane_map_info.lane_map.duplex_lane_map.sys_lane_map ;
    }

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy_temp.access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_logical_lane_from_lane_map(&phy_temp, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_set_slice(&phy_temp, 0xFFFF, 0xFFFF, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, logical_lane));

            if (loopback != phymodLoopbackRemotePMD) {
                PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_dig_lpbk_rptr(&(phy_temp.access), enable, DATA_IN_SIDE));
            }
        }
    }

    if(!enable) {
        if(BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_resume_port_wr(sa__, msg_port_num)) {
            PHYMOD_DEBUG_ERROR(("Resuming port Failed Due to RESUME_PORT.Write() Error\n"));
            return PHYMOD_E_INTERNAL;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_loopback_get(const plp_barchetta_phymod_phy_access_t *phy,
        plp_barchetta_phymod_loopback_mode_t loopback, uint32_t *enable) {
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    err_code_t __err = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;


    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            if (loopback == phymodLoopbackRemotePMD) {
                *enable = rd_rmt_lpbk_en();
                if (__err != 0) {
                    PHYMOD_DEBUG_ERROR(("Read remote loopback failed\n"));
                    return __err;
                }
            } else {
                *enable = rd_dig_lpbk_en();
                if (__err != 0) {
                    PHYMOD_DEBUG_ERROR(("Read remote loopback failed\n"));
                    return __err;
                }
            }
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_status_dump(const plp_barchetta_phymod_phy_access_t *phy) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint32_t lane_index = 0;
    char *debug_mod_name[BARC_REG_DUMP_MOD_CNT] = {"CHIP INFO", "FW VER", "DLOAD REG", "GPR",
                                          "PORT CFG REG", "LANE CFG REG", "DIAG_REG"};
    uint32_t debug_mod_reg_cnt[BARC_REG_DUMP_MOD_CNT] = {2, 1, 4, 32, 256, 64, 16};
    uint32_t debug_reg_start_addr[BARC_REG_DUMP_MOD_CNT] = {0x8500, 0x8215, 0x8230, 0x8250,
                                                            0x8c00, 0x8e00, 0x8ef0};
    uint32_t loop_index = 0, data = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;
    if ((phy->access.flags & PHY_INTERNAL_DUMP) || (phy->access.flags & PHY_DUMP_L1) ||
        (phy->access.flags & PHY_DUMP_L2) || (phy->access.flags & PHY_DUMP_L3)) {
        for(loop_index = 0; loop_index < BARC_REG_DUMP_MOD_CNT; loop_index++) {
            PHYMOD_DIAG_OUT((" ***************************************\n"));
            PHYMOD_DIAG_OUT(("        %s           \n", debug_mod_name[loop_index] ));
            PHYMOD_DIAG_OUT((" ***************************************\n"));
            for (lane_index = debug_reg_start_addr[loop_index];
                    lane_index < (debug_reg_start_addr[loop_index] + debug_mod_reg_cnt[loop_index]); lane_index++) {
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_reg_read(&phy->access, (lane_index | (1 << 16)), &data));
                PHYMOD_DIAG_OUT((" REG ADDR:0x%X Data:0x%X\n", (lane_index | (1 << 16)), data));
            }
        }
    }
    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            PHYMOD_DIAG_OUT((" ***************************************\n"));
            PHYMOD_DIAG_OUT(
                    (" ******* PHY status dump for Barchetta PHY ID:0x%x ********\n", phy->access.addr));
            PHYMOD_DIAG_OUT((" ***************************************\n"));
            PHYMOD_DIAG_OUT((" ***************************************\n"));
            PHYMOD_DIAG_OUT(
                    (" ******* PHY status dump for side:%s Logical Lane:%d ********\n", (BARCHETTA_IS_SYSTEM_SIDE(phy)) ? "SYS" : "LINE", logical_lane));
            PHYMOD_DIAG_OUT((" ***************************************\n"));
            PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_init_blackhawk_barchetta_info(&phy->access));
            PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_display_state(&phy->access));
            if (phy->access.flags & PHY_INTERNAL_DUMP) {
                PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_display_diag_data(&phy->access, SRDS_INTERNAL_DUMP));
            } else if (phy->access.flags & PHY_DUMP_L1) {
                PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_display_diag_data(&phy->access, SRDS_DUMP_L1));
            } else if (phy->access.flags & PHY_DUMP_L2) {
                PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_display_diag_data(&phy->access, SRDS_DUMP_L2));
            } else if (phy->access.flags & PHY_DUMP_L3) {
                PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_display_diag_data(&phy->access, SRDS_DUMP_L3));
            } else {
                PHYMOD_IF_ERR_RETURN(
                    plp_barchetta_blackhawk_barchetta_display_diag_data(&phy->access, SRDS_MIN_DUMP));
            }
            PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_display_lane_config(&phy->access));
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_fec_enable_set(const plp_barchetta_phymod_phy_access_t* phy,
        uint32_t enable) {
    BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_t dec_top_ctrl;
    BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_t enc_top_ctrl;
    BCMI_BARCHETTA_SLICE_SLICE0r_t slice_0;
    BCMI_BARCHETTA_SLICE_SLICE1r_t slice_1;
    plp_barchetta_phymod_phy_inf_config_t config;
    int port_number = 0;
    barchetta_package_info_t pkg_info;
    barchetta_aux_modes_t loc_aux_mode;

    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        /* FEC NOT supported in sys*/
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid side for FEC as system side\n"));
        return PHYMOD_E_PARAM;
    }

    PHYMOD_MEMSET(&dec_top_ctrl, 0,
            sizeof(BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_t));
    PHYMOD_MEMSET(&enc_top_ctrl, 0,
            sizeof(BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_t));
    PHYMOD_MEMSET(&config, 0, sizeof(plp_barchetta_phymod_phy_inf_config_t));
    config.device_aux_modes = &loc_aux_mode;

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
    PHYMOD_IF_ERR_RETURN(
            __plp_barchetta_port_config_get(phy, pkg_info, &config));
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));

    slice_0.v[0] = (1 << 15) | (port_number << 8);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));
    slice_1.v[0] = 1 << 4;
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_SLICE_SLICE1r(&phy->access, slice_1));

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r(&phy->access, &dec_top_ctrl));

    slice_0.v[0] = (1 << 14)| (port_number << 8);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r(&phy->access, &enc_top_ctrl));

    if(PHYMOD_FEC_CL108_GET(enable)) { /* Is CL108 Selected */
        /* Is CL108 needs to be enabled */
        if(enable & 0x1) {
            if(config.data_rate == 25000) {
                BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_DP_MODEf_SET(dec_top_ctrl, 4);
                BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_DP_MODEf_SET(enc_top_ctrl, 4);
            } else {
                PHYMOD_DEBUG_ERROR(("ERROR: Invalid data rate for Cl108\n"));
                return PHYMOD_E_UNAVAIL;
            }
        }
     } else if (PHYMOD_FEC_CL91_GET(enable)) { /* Is CL91 selected */
        if(enable & 0x1) {
            if (config.data_rate == 100000) {
                BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_DP_MODEf_SET(dec_top_ctrl, 12);
                BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_DP_MODEf_SET(enc_top_ctrl, 12);
        } else if (config.data_rate == 25000) {
                BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_DP_MODEf_SET(dec_top_ctrl, 6);
                BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_DP_MODEf_SET(enc_top_ctrl, 6);
        } else if (config.data_rate == 50000) {
                BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_DP_MODEf_SET(dec_top_ctrl, 10);
                BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_DP_MODEf_SET(enc_top_ctrl, 10);
        } else {
                PHYMOD_DEBUG_ERROR(("ERROR: Invalid data rate for CL91\n"));
            return PHYMOD_E_UNAVAIL;
        }
    }
    } else if (!PHYMOD_FEC_CL91_GET(enable)) { /* Is CL74 selected */
        if(enable & 0x1) {
            if (config.data_rate == 10000) {
                BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_DP_MODEf_SET(dec_top_ctrl, 3);
                BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_DP_MODEf_SET(enc_top_ctrl, 3);
            } else if (config.data_rate == 25000) {
                BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_DP_MODEf_SET(dec_top_ctrl, 5);
                BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_DP_MODEf_SET(enc_top_ctrl, 5);
            } else if (config.data_rate == 40000) {
                BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_DP_MODEf_SET(dec_top_ctrl, 9);
                BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_DP_MODEf_SET(enc_top_ctrl, 9);
            } else if (config.data_rate == 50000) {
                BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_DP_MODEf_SET(dec_top_ctrl, 11);
                BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_DP_MODEf_SET(enc_top_ctrl, 11);
            } else {
                PHYMOD_DEBUG_ERROR(("ERROR: Invalid data rate for CL91\n"));
                return PHYMOD_E_UNAVAIL;
            }
        }
    }

    if (enable & 0x1) {
        BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_TOP_DP_BYPASS_ENf_SET(
                dec_top_ctrl, 0);
        BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_TOP_DP_BYPASS_ENf_SET(
                enc_top_ctrl, 0);
    } else {
        BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_TOP_DP_BYPASS_ENf_SET(
                dec_top_ctrl, 1);
        BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_FEC_ENC_TOP_DP_BYPASS_ENf_SET(
                enc_top_ctrl, 1);
    }
    slice_0.v[0] = (1 << 15)| (port_number << 8);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r(&phy->access, dec_top_ctrl));

    slice_0.v[0] = (1 << 14)| (port_number << 8);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r(&phy->access, enc_top_ctrl));

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_fec_enable_get(const plp_barchetta_phymod_phy_access_t* phy,
        uint32_t *enable) {
    int fec_val = 0;
    BCMI_BARCHETTA_SLICE_SLICE0r_t slice_0;
    BCMI_BARCHETTA_SLICE_SLICE1r_t slice_1;
    int port_number = 0;
    barchetta_package_info_t pkg_info;
    BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_t dec_top_ctrl;
    BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_t enc_top_ctrl;

    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        /* FEC NOT supported in sys*/
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid side for FEC set\n"));
        return PHYMOD_E_PARAM;
    }

    PHYMOD_MEMSET(&dec_top_ctrl, 0,
            sizeof(BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_t));
    PHYMOD_MEMSET(&enc_top_ctrl, 0,
            sizeof(BCMI_BARCHETTA_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));

    slice_0.v[0] = (1 << 15) | (port_number << 8);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));
    slice_1.v[0] = 1 << 4;
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_SLICE_SLICE1r(&phy->access, slice_1));

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r(&phy->access, &dec_top_ctrl));

    slice_0.v[0] = (1 << 14)| (port_number << 8);
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_USER_FEC_ENC_TOPS_FEC_ENC_TOP_CTRL_0r(&phy->access, &enc_top_ctrl));

    fec_val = BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_DP_MODEf_GET(dec_top_ctrl);
    if (PHYMOD_FEC_CL108_GET(*enable)) {
        if (fec_val == 4) {
            *enable = !BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_TOP_DP_BYPASS_ENf_GET(dec_top_ctrl);
        } else {
            *enable = 0 ;
        }
    } else if (PHYMOD_FEC_CL91_GET(*enable)) {
        if (fec_val == 12 || fec_val == 6 || fec_val == 10) {
            *enable = !BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_TOP_DP_BYPASS_ENf_GET(dec_top_ctrl);
    } else {
            *enable = 0 ;
        }
    } else if (!PHYMOD_FEC_CL91_GET(*enable)){
        if (fec_val == 3 || fec_val == 5 || fec_val == 7 || fec_val == 9 || fec_val == 11 ) {
            *enable = !BCMI_BARCHETTA_USER_FEC_DEC_TOPS_FEC_DEC_TOP_CTRL_0r_FEC_DEC_TOP_DP_BYPASS_ENf_GET(dec_top_ctrl);
        } else {
            *enable = 0 ;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:  API to choose the PLL selection

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_get_bhawk_pll_to_gpreg_pll(enum blackhawk_barchetta_pll_div_enum pll_div, int *gpreg_val)
{
    switch (pll_div) {
        case BLACKHAWK_BARCHETTA_PLL_DIV_66:
           *gpreg_val = BARCHETTA_PLL_DIV_66;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_70:
           *gpreg_val = BARCHETTA_PLL_DIV_70;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_80:
           *gpreg_val = BARCHETTA_PLL_DIV_80;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_82P5:
           *gpreg_val = BARCHETTA_PLL_DIV_82P5;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_85:
           *gpreg_val = BARCHETTA_PLL_DIV_85;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_87P5:
           *gpreg_val = BARCHETTA_PLL_DIV_87P5;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_90:
            *gpreg_val = BARCHETTA_PLL_DIV_90;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_120:
            *gpreg_val = BARCHETTA_PLL_DIV_120;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_128:
            *gpreg_val = BARCHETTA_PLL_DIV_128;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_132:
            *gpreg_val = BARCHETTA_PLL_DIV_132;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_140:
            *gpreg_val = BARCHETTA_PLL_DIV_140;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_148:
            *gpreg_val = BARCHETTA_PLL_DIV_148;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_160:
            *gpreg_val = BARCHETTA_PLL_DIV_160;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_165:
            *gpreg_val = BARCHETTA_PLL_DIV_165;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_170:
            *gpreg_val = BARCHETTA_PLL_DIV_170;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_175:
            *gpreg_val = BARCHETTA_PLL_DIV_175;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_180:
           *gpreg_val = BARCHETTA_PLL_DIV_180;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_264:
           *gpreg_val = BARCHETTA_PLL_DIV_264;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_280:
           *gpreg_val = BARCHETTA_PLL_DIV_280;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_330:
           *gpreg_val = BARCHETTA_PLL_DIV_330;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_350:
           *gpreg_val = BARCHETTA_PLL_DIV_350;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_96:
           *gpreg_val = BARCHETTA_PLL_DIV_96;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_144:
           *gpreg_val = BARCHETTA_PLL_DIV_144;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_168:
           *gpreg_val = BARCHETTA_PLL_DIV_168;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_184:
           *gpreg_val = BARCHETTA_PLL_DIV_184;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_198:
           *gpreg_val = BARCHETTA_PLL_DIV_198;
            break;
        case BLACKHAWK_BARCHETTA_PLL_DIV_200:
           *gpreg_val = BARCHETTA_PLL_DIV_200;
            break;

        default :
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid PLL value\n"));
        return PHYMOD_E_CONFIG;
    }

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: To Convert from GPREG PLL div numeric value to corresponding
          blackhwak barchetta PLL div value
 COMMENT:
*******************************************************************************/
int
_plp_barchetta_get_bhawk_pll_from_gpreg_pll(int gpreg_pll, enum blackhawk_barchetta_pll_div_enum *bhawk_pll)
{
    switch (gpreg_pll) {
        case BARCHETTA_PLL_DIV_66:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_66;
            break;
        case BARCHETTA_PLL_DIV_70:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_70;
            break;
        case BARCHETTA_PLL_DIV_80:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_80;
            break;
        case BARCHETTA_PLL_DIV_82P5:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_82P5;
            break;
        case BARCHETTA_PLL_DIV_85:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_85;
            break;
        case BARCHETTA_PLL_DIV_87P5:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_87P5;
            break;
        case BARCHETTA_PLL_DIV_90:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_90;
            break;
        case BARCHETTA_PLL_DIV_120:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_120;
            break;
        case BARCHETTA_PLL_DIV_128:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_128;
            break;
        case BARCHETTA_PLL_DIV_132:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_132;
            break;
        case BARCHETTA_PLL_DIV_140:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_140;
            break;
        case BARCHETTA_PLL_DIV_148:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_148;
            break;
        case BARCHETTA_PLL_DIV_160:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_160;
            break;
        case BARCHETTA_PLL_DIV_165:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_165;
            break;
        case BARCHETTA_PLL_DIV_170:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_170;
            break;
        case BARCHETTA_PLL_DIV_175:
            *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_175;
            break;
        case BARCHETTA_PLL_DIV_180:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_180;
            break;
        case BARCHETTA_PLL_DIV_264:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_264;
            break;
        case BARCHETTA_PLL_DIV_280:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_280;
            break;
        case BARCHETTA_PLL_DIV_330:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_330;
            break;
        case BARCHETTA_PLL_DIV_350:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_350;
            break;
        case BARCHETTA_PLL_DIV_96:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_96;
            break;
        case BARCHETTA_PLL_DIV_144:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_144;
            break;
        case BARCHETTA_PLL_DIV_168:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_168;
            break;
        case BARCHETTA_PLL_DIV_184:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_184;
            break;
        case BARCHETTA_PLL_DIV_198:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_198;
            break;
        case BARCHETTA_PLL_DIV_200:
           *bhawk_pll = BLACKHAWK_BARCHETTA_PLL_DIV_200;
            break;
        default :
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid PLL value\n"));
        return PHYMOD_E_CONFIG;
    }

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:  API to choose the PLL selection

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_get_pll_index(const plp_barchetta_phymod_phy_access_t* phy, uint8_t *pll_index)
{
    BCMI_BARCHETTA_CTRL_SWGPREG4r_t pll_0_associated_lanes;
    BCMI_BARCHETTA_CTRL_SWGPREG6r_t pll_1_associated_lanes;
    PHYMOD_MEMSET(&pll_0_associated_lanes, 0 ,sizeof(BCMI_BARCHETTA_CTRL_SWGPREG4r_t));
    PHYMOD_MEMSET(&pll_1_associated_lanes, 0 ,sizeof(BCMI_BARCHETTA_CTRL_SWGPREG6r_t));
    PHYMOD_IF_ERR_RETURN(
         BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));
    PHYMOD_IF_ERR_RETURN(
         BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));

    if (BARCHETTA_IS_LINE_SIDE(phy)) {
    if (pll_1_associated_lanes.v[0] & (phy->access.lane_mask)) {
        *pll_index = 1;
    } else {
        *pll_index = 0;
        }
    } else {
        if (pll_1_associated_lanes.v[0] & (phy->access.lane_mask << 8)) {
            *pll_index = 1;
        } else {
            *pll_index = 0;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_set_slice_pll_index(const plp_barchetta_phymod_phy_access_t* phy, int pll_index)
{
    BCMI_BARCHETTA_SLICE_SLICE0r_t slice_0;
    BCMI_BARCHETTA_SLICE_SLICE1r_t slice_1;

    slice_0.v[0] = ((1 << 15) | ((BARCHETTA_IS_LINE_SIDE(phy)) ?
                    (1 << 12) : (1 << 13)));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_SLICE_SLICE0r(&phy->access, slice_0));
    slice_1.v[0] = pll_index;
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_SLICE_SLICE1r(&phy->access, slice_1));
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_configure_bhawk_pll(const plp_barchetta_phymod_phy_access_t* phy,
        barchetta_serdes_config_info_t serdes_cfg, int pll_index, int force) {
    uint8_t pll_lock=0, pll_lock_chg = 0;
    int retry_cnt = 1000;
    uint32_t configured_div = 0;

    PHYMOD_IF_ERR_RETURN(
        plp_barchetta_blackhawk_barchetta_INTERNAL_read_pll_div(&phy->access, &configured_div));
    if (configured_div == serdes_cfg.pll_div && (!force)) {
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_INTERNAL_pll_lock_status(&phy->access,&pll_lock, &pll_lock_chg));
        if (pll_lock) {
            PHYMOD_DEBUG_ERROR(("Not configuring PLL as its was configured already\n"));
            return PHYMOD_E_NONE;
        }
    }
    PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_core_dp_reset(&phy->access, 1));
    PHYMOD_IF_ERR_RETURN(
            plp_barchetta_blackhawk_barchetta_configure_pll_refclk_div(&phy->access, serdes_cfg.refclk, serdes_cfg.pll_div));
    PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_core_dp_reset(&phy->access, 0));
    if (pll_index == BARCHETTA_PLL_INDEX_1) {
        BCMI_BARCHETTA_SLICE_SLICE1r_t slice_1, prev_config;
        BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_t  rev;
        uint32_t ams_pll = 0;
        PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_CTRL_CHIP_REVISIONr(&phy->access, &rev));
        if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_READ_SLICE_SLICE1r(&phy->access, &prev_config));
            slice_1.v[0] = 0;
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_SLICE_SLICE1r(&phy->access, slice_1));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_read(&phy->access, 0x1d11a, &ams_pll));
            ams_pll |= 0x101;
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_write(&phy->access, 0x1d11a, ams_pll));
            slice_1.v[0] = 1;
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_SLICE_SLICE1r(&phy->access, slice_1));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_read(&phy->access, 0x1d11a, &ams_pll));
            ams_pll |= 0x101;
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_write(&phy->access, 0x1d11a, ams_pll));
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_SLICE_SLICE1r(&phy->access, prev_config));
        }
    }
    do {
        PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_INTERNAL_pll_lock_status(&phy->access,&pll_lock, &pll_lock_chg));
        PHYMOD_USLEEP(100);
    } while ((pll_lock != 1) && (--retry_cnt));
    (void) pll_lock_chg;
    if (retry_cnt == 0) {
        PHYMOD_DEBUG_ERROR(("ERROR: PLL not locked\n"));
        return PHYMOD_E_TIMEOUT;
    }
    PHYMOD_DEBUG_INFO(("PLL :%d Locked\n", pll_index ));
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_invalidate_pll_and_lane_associated(const plp_barchetta_phymod_phy_access_t* phy,
        int pll_index, int lanemap_associated) {
    BCMI_BARCHETTA_CTRL_SWGPREG4r_t pll_0_associated_lanes;
    BCMI_BARCHETTA_CTRL_SWGPREG6r_t pll_1_associated_lanes;
    BCMI_BARCHETTA_CTRL_SWGPREG5r_t line_pll_0_pll_1_div;
    BCMI_BARCHETTA_CTRL_SWGPREG7r_t sys_pll_0_pll_1_div;

    if (pll_index == BARCHETTA_PLL_INDEX_0) {
        PHYMOD_MEMSET(&pll_0_associated_lanes, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG4r_t));
        PHYMOD_IF_ERR_RETURN(
              BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(&phy->access, pll_0_associated_lanes));
        PHYMOD_IF_ERR_RETURN(
             BCMI_BARCHETTA_READ_CTRL_SWGPREG5r(&phy->access, &line_pll_0_pll_1_div));
        PHYMOD_IF_ERR_RETURN(
             BCMI_BARCHETTA_READ_CTRL_SWGPREG7r(&phy->access, &sys_pll_0_pll_1_div));
        line_pll_0_pll_1_div.v[0] &= (~0xff);
        sys_pll_0_pll_1_div.v[0] &= (~0xff);
        PHYMOD_IF_ERR_RETURN(
             BCMI_BARCHETTA_WRITE_CTRL_SWGPREG5r(&phy->access, line_pll_0_pll_1_div));
        PHYMOD_IF_ERR_RETURN(
             BCMI_BARCHETTA_WRITE_CTRL_SWGPREG7r(&phy->access, sys_pll_0_pll_1_div));
     } else {
         PHYMOD_MEMSET(&pll_1_associated_lanes, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG6r_t));
         PHYMOD_IF_ERR_RETURN(
             BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(&phy->access, pll_1_associated_lanes));
         PHYMOD_IF_ERR_RETURN(
             BCMI_BARCHETTA_READ_CTRL_SWGPREG5r(&phy->access, &line_pll_0_pll_1_div));
         PHYMOD_IF_ERR_RETURN(
             BCMI_BARCHETTA_READ_CTRL_SWGPREG7r(&phy->access, &sys_pll_0_pll_1_div));
         line_pll_0_pll_1_div.v[0] &= (0x00FF);
         sys_pll_0_pll_1_div.v[0] &= (0x00FF);
         PHYMOD_IF_ERR_RETURN(
             BCMI_BARCHETTA_WRITE_CTRL_SWGPREG5r(&phy->access, line_pll_0_pll_1_div));
         PHYMOD_IF_ERR_RETURN(
             BCMI_BARCHETTA_WRITE_CTRL_SWGPREG7r(&phy->access, sys_pll_0_pll_1_div));
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_configure_pll(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_ref_clk_t ref_clock,
                             barchetta_serdes_config_info_t serdes_cfg)
{
    BCMI_BARCHETTA_CTRL_SWGPREG4r_t pll_0_associated_lanes;
    BCMI_BARCHETTA_CTRL_SWGPREG5r_t line_pll_0_pll_1_div;
    BCMI_BARCHETTA_CTRL_SWGPREG6r_t pll_1_associated_lanes;
    BCMI_BARCHETTA_CTRL_SWGPREG7r_t sys_pll_0_pll_1_div;
    int gpreg_pll = 0, pll_0 = 0, pll_1 = 0;
    int rev_based_lane_map = 0;
    BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_t  rev;

    PHYMOD_MEMSET(&pll_0_associated_lanes, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG4r_t));
    PHYMOD_MEMSET(&line_pll_0_pll_1_div, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG5r_t));
    PHYMOD_MEMSET(&pll_1_associated_lanes, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG6r_t));
    PHYMOD_MEMSET(&sys_pll_0_pll_1_div, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREG7r_t));

    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_CTRL_CHIP_REVISIONr(&phy->access, &rev));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_CTRL_SWGPREG5r(&phy->access, &line_pll_0_pll_1_div));
    PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_CTRL_SWGPREG7r(&phy->access, &sys_pll_0_pll_1_div));

    if (BARCHETTA_IS_LINE_SIDE(phy)) {
        pll_0 = BCMI_BARCHETTA_CTRL_SWGPREG5r_SWGPREG5_DATAf_GET(line_pll_0_pll_1_div) & 0xFF;
        pll_1 = ((BCMI_BARCHETTA_CTRL_SWGPREG5r_SWGPREG5_DATAf_GET(line_pll_0_pll_1_div) & 0xFF00) >> 8);
    } else {
        pll_0 = BCMI_BARCHETTA_CTRL_SWGPREG7r_SWGPREG7_DATAf_GET(sys_pll_0_pll_1_div) & 0xFF;
        pll_1 = ((BCMI_BARCHETTA_CTRL_SWGPREG7r_SWGPREG7_DATAf_GET(sys_pll_0_pll_1_div) & 0xFF00) >> 8);
    }
    PHYMOD_IF_ERR_RETURN(
         _plp_barchetta_get_bhawk_pll_to_gpreg_pll(serdes_cfg.pll_div, &gpreg_pll));

    if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
        rev_based_lane_map = phy->access.lane_mask;
    } else {
        /* Dont care, based on other condition it will work*/
        rev_based_lane_map = 0xFF;
    }
#ifdef QUAD_PLL_CONFIG
    if ((rev_based_lane_map & 0xF) && (phy->access.lane_mask & 0x0F) && (pll_0 != gpreg_pll)) {
#else
    if ((rev_based_lane_map & 0xF) && ((pll_0 & 0xFF) == 0)) {
#endif
        PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_set_slice_pll_index(phy, 0));
        PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_configure_bhawk_pll(phy, serdes_cfg, BARCHETTA_PLL_INDEX_0, 0));
        if ((pll_1 != 0) && BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
            barchetta_serdes_config_info_t serdes_cfg_pll1;
            PHYMOD_MEMCPY(&serdes_cfg_pll1, &serdes_cfg, sizeof(barchetta_serdes_config_info_t));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_get_bhawk_pll_from_gpreg_pll(pll_1, &serdes_cfg_pll1.pll_div));
            PHYMOD_DEBUG_INFO(("Re-config PLL1 CONFIG:%d\n", serdes_cfg_pll1.pll_div));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_set_slice_pll_index(phy, 1));

            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_configure_bhawk_pll(phy, serdes_cfg_pll1, BARCHETTA_PLL_INDEX_1, 1));
        }

        /* Configure PLL0*/
        if (BARCHETTA_IS_LINE_SIDE(phy)) {
            PHYMOD_DEBUG_INFO(("####Line PLL0 CONFIG:%x\n", serdes_cfg.pll_div ));
            /* Use LSB for PLL0 */
            BCMI_BARCHETTA_CTRL_SWGPREG5r_SWGPREG5_DATAf_SET(line_pll_0_pll_1_div, (gpreg_pll & 0xFF));
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_WRITE_CTRL_SWGPREG5r(&phy->access, line_pll_0_pll_1_div));

            /* Store line lane map associated with pll0*/
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));

            pll_0_associated_lanes.v[0] |= (phy->access.lane_mask);
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(&phy->access, pll_0_associated_lanes));
            /*Remove lanes if it already used in pll1*/
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));
            if (pll_1_associated_lanes.v[0] & phy->access.lane_mask) {
                pll_1_associated_lanes.v[0] &= (~phy->access.lane_mask);
                PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(&phy->access, pll_1_associated_lanes));
            }
        } else {
            PHYMOD_DEBUG_INFO(("####SYS PLL0 CONFIG\n"));
            /* Use LSB for PLL0 */
            BCMI_BARCHETTA_CTRL_SWGPREG7r_SWGPREG7_DATAf_SET(sys_pll_0_pll_1_div, (gpreg_pll & 0xFF));
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_WRITE_CTRL_SWGPREG7r(&phy->access, sys_pll_0_pll_1_div));

            /* Store system side lane map associated with pll0*/
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));

            pll_0_associated_lanes.v[0] |= ((phy->access.lane_mask & 0xFF)<< 8);
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(&phy->access, pll_0_associated_lanes));
            /*Remove lanes if it already used in pll1*/
            PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));
            if (pll_1_associated_lanes.v[0] & (phy->access.lane_mask << 8)) {
                pll_1_associated_lanes.v[0] &= (~(phy->access.lane_mask << 8));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(&phy->access, pll_1_associated_lanes));
            }
        }
        return PHYMOD_E_NONE;
    } else {
#ifdef QUAD_PLL_CONFIG
        if ((gpreg_pll == pll_0) && ((rev_based_lane_map & 0xF)) && (phy->access.lane_mask & 0xF)) {
#else
        if ((gpreg_pll == pll_0) && ((rev_based_lane_map & 0xF))) {
#endif
            if (BARCHETTA_IS_LINE_SIDE(phy)) {
                PHYMOD_DEBUG_INFO(("####Adding lane:%x to Pll0 line\n", phy->access.lane_mask));
                /* Remove the lanes if it is used in PLL1*/
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));
                if (pll_1_associated_lanes.v[0] & phy->access.lane_mask) {
                    pll_1_associated_lanes.v[0] &= ~(phy->access.lane_mask);
                    PHYMOD_IF_ERR_RETURN(
                            BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(&phy->access, pll_1_associated_lanes));
                }
                /* New port Pll same as configured PLL Store Port number associated*/
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));
                pll_0_associated_lanes.v[0] |= (phy->access.lane_mask);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(&phy->access, pll_0_associated_lanes));
                return PHYMOD_E_NONE;
            } else {
                /* Remove the sys lanes if it is used in PLL1*/
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));
                if (pll_1_associated_lanes.v[0] & (phy->access.lane_mask << 8)) {
                    pll_1_associated_lanes.v[0] &= ~(phy->access.lane_mask << 8);
                    PHYMOD_IF_ERR_RETURN(
                            BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(&phy->access, pll_1_associated_lanes));
                }

                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));
                pll_0_associated_lanes.v[0] |= ((phy->access.lane_mask & 0xFF) << 8);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(&phy->access, pll_0_associated_lanes));

                return PHYMOD_E_NONE;
            }
        } else if ((rev_based_lane_map & 0xF0) && (pll_1 == 0)) {
            if (BCMI_BARCHETTA_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(rev) == BARCHETTA_CHIP_REV_A0) {
                if (pll_0 == 0) {
                    PHYMOD_IF_ERR_RETURN(
                            _plp_barchetta_set_slice_pll_index(phy, 0));
                    PHYMOD_IF_ERR_RETURN(
                            _plp_barchetta_configure_bhawk_pll(phy, serdes_cfg, BARCHETTA_PLL_INDEX_0, 0));
                    /* Configure PLL0*/
                    if (BARCHETTA_IS_LINE_SIDE(phy)) {
                        PHYMOD_DEBUG_INFO(("####Line PLL0 CONFIG:%x\n", serdes_cfg.pll_div ));
                        /* Use LSB for PLL0 */
                        BCMI_BARCHETTA_CTRL_SWGPREG5r_SWGPREG5_DATAf_SET(line_pll_0_pll_1_div, (gpreg_pll & 0xFF));
                        PHYMOD_IF_ERR_RETURN(
                                BCMI_BARCHETTA_WRITE_CTRL_SWGPREG5r(&phy->access, line_pll_0_pll_1_div));
                    } else {
                        PHYMOD_DEBUG_INFO(("####SYS PLL0 CONFIG\n"));
                        /* Use LSB for PLL0 */
                        BCMI_BARCHETTA_CTRL_SWGPREG7r_SWGPREG7_DATAf_SET(sys_pll_0_pll_1_div, (gpreg_pll & 0xFF));
                        PHYMOD_IF_ERR_RETURN(
                                BCMI_BARCHETTA_WRITE_CTRL_SWGPREG7r(&phy->access, sys_pll_0_pll_1_div));
                    }
                }
            }
            /*Configure Pll1*/
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice_pll_index(phy, 1));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_configure_bhawk_pll(phy, serdes_cfg, BARCHETTA_PLL_INDEX_1, 0));
            if (BARCHETTA_IS_LINE_SIDE(phy)) {
                PHYMOD_DEBUG_INFO(("####Line PLL1 CONFIG:%x\n",  serdes_cfg.pll_div));
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG5r(&phy->access, &line_pll_0_pll_1_div));
                line_pll_0_pll_1_div.v[0] |= ((gpreg_pll & 0xFF) << 8);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG5r(&phy->access, line_pll_0_pll_1_div));

                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));
                pll_1_associated_lanes.v[0] |= phy->access.lane_mask;
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(&phy->access, pll_1_associated_lanes));
                /*Remove lanes if it already used in pll0*/
                PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));
                if (pll_0_associated_lanes.v[0] & phy->access.lane_mask) {
                    pll_0_associated_lanes.v[0] &= (~phy->access.lane_mask);
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(&phy->access, pll_0_associated_lanes));
                }
            } else {
                PHYMOD_DEBUG_INFO(("####Sys PLL1 CONFIG:%x\n",  serdes_cfg.pll_div));
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG7r(&phy->access, sys_pll_0_pll_1_div));
                sys_pll_0_pll_1_div.v[0] |= ((gpreg_pll & 0xFF) << 8);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG7r(&phy->access, sys_pll_0_pll_1_div));

                /* Store SYS side PLL*/
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));

                pll_1_associated_lanes.v[0] |= (phy->access.lane_mask & 0xFF) << 8;
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(&phy->access, pll_1_associated_lanes));

                /*Remove lanes if it already used in pll0*/
                PHYMOD_IF_ERR_RETURN(
                    BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));
                if (pll_0_associated_lanes.v[0] & (phy->access.lane_mask << 8)) {
                    pll_0_associated_lanes.v[0] &= (~(phy->access.lane_mask<<8));
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(&phy->access, pll_0_associated_lanes));
                }
            }
            return PHYMOD_E_NONE;
        } else if ((rev_based_lane_map & 0xF0) && (pll_1 == gpreg_pll)) {
            if (BARCHETTA_IS_LINE_SIDE(phy)) {
                PHYMOD_DEBUG_INFO(("####Adding lanemap:%x to Pll1 line side\n", phy->access.lane_mask));
                /* Remove the lanes if it is used in PLL0*/
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));
                if (pll_0_associated_lanes.v[0] & (phy->access.lane_mask)) {
                    pll_0_associated_lanes.v[0] &= ~(phy->access.lane_mask);
                    PHYMOD_IF_ERR_RETURN(
                            BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(&phy->access, pll_0_associated_lanes));
                }

                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));
                pll_1_associated_lanes.v[0] |= (phy->access.lane_mask);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(&phy->access, pll_1_associated_lanes));
                return PHYMOD_E_NONE;
            } else {
                /* Remove the Sys lanes if it is used in PLL0*/
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG4r(&phy->access, &pll_0_associated_lanes));
                if (pll_0_associated_lanes.v[0] & (phy->access.lane_mask << 8)) {
                    pll_0_associated_lanes.v[0] &= ~(phy->access.lane_mask << 8);
                    PHYMOD_IF_ERR_RETURN(
                            BCMI_BARCHETTA_WRITE_CTRL_SWGPREG4r(&phy->access, pll_0_associated_lanes));
                }
                /* Add sys PLL1 lane map*/
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_READ_CTRL_SWGPREG6r(&phy->access, &pll_1_associated_lanes));
                pll_1_associated_lanes.v[0] |= ((phy->access.lane_mask & 0xFF) << 8);
                PHYMOD_IF_ERR_RETURN(
                        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG6r(&phy->access, pll_1_associated_lanes));

                return PHYMOD_E_NONE;
            }
        } else if ((pll_0 != 0) && (rev_based_lane_map & 0xF)) {
            PHYMOD_DEBUG_INFO(("PLL 0 is in use. PLLs associated with lane map:%x will be removed\n", phy->access.lane_mask));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_invalidate_pll_and_lane_associated(phy, BARCHETTA_PLL_INDEX_0, pll_0_associated_lanes.v[0]));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_configure_pll(phy, ref_clock, serdes_cfg));
        } else if ((pll_1 != 0) && (rev_based_lane_map & 0xF0)) {
            PHYMOD_DEBUG_INFO(("PLL 1 is in use. PLLs associated with lane map:%x will be removed\n", phy->access.lane_mask));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_invalidate_pll_and_lane_associated(phy, BARCHETTA_PLL_INDEX_1, pll_1_associated_lanes.v[0]));
                PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_configure_pll(phy, ref_clock, serdes_cfg));
            } else {
                PHYMOD_DEBUG_ERROR(("Error matching PLL found\n"));
                return PHYMOD_E_CONFIG;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_cl72_set(const plp_barchetta_phymod_phy_access_t* phy, uint32_t cl72_en)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info    ;
    barchetta_modify_port_t  port_modify ;
    barchetta_config_pmd_t   pmd_cfg_rd  ;
    int port_number = 0xFF;
    uint8_t msg_if_status = BARCHETTA_MSG_IF_RET_ERROR ;
    uint8_t msg_port_num = 0;

    PHYMOD_MEMSET(&pkg_info,    0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&port_modify, 0, sizeof(barchetta_modify_port_t));
    PHYMOD_MEMSET(&pmd_cfg_rd,  0, sizeof(barchetta_config_pmd_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));

    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Wrong Port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
    msg_port_num = port_number;

    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        msg_if_status = _plp_barchetta_msg_config_pmd_rd(pa, msg_port_num, BARCHETTA_SIDE_SYS, &pmd_cfg_rd);

    } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
        msg_if_status = _plp_barchetta_msg_config_pmd_rd(pa, msg_port_num, BARCHETTA_SIDE_LINE, &pmd_cfg_rd);
    }

    if(msg_if_status == BARCHETTA_MSG_IF_RET_SUCCESS) {
       pmd_cfg_rd.tx_training_opt = (cl72_en & 0x1);
       PHYMOD_MEMCPY(&port_modify, &pmd_cfg_rd, sizeof(barchetta_config_pmd_t));
       if(BARCHETTA_MSG_IF_RET_ERROR ==_plp_barchetta_msg_modify_port(pa, &port_modify)) {
           PHYMOD_DEBUG_ERROR(("ERROR: Port modification Failed Due to MODIFY_PORT.Start() Error\n"));
           return PHYMOD_E_INTERNAL;
       }
    }
    else {
        PHYMOD_DEBUG_ERROR(("ERROR: Reading PMD configuration Failed Due to CONFIG_PMD.Read() Error\n"));
        return PHYMOD_E_INTERNAL;
    }

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_cl72_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t* cl72_en)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info    ;
    barchetta_config_pmd_t   pmd_cfg_rd  ;
    int port_number = 0xFF;
    uint8_t msg_if_status = BARCHETTA_MSG_IF_RET_ERROR ;
    uint8_t msg_port_num = 0;
    *cl72_en = 0xFFFF;
    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&pmd_cfg_rd, 0, sizeof(barchetta_config_pmd_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Wrong Port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
    msg_port_num = (port_number & 0xFF);
    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        msg_if_status = _plp_barchetta_msg_config_pmd_rd(pa, msg_port_num, BARCHETTA_SIDE_SYS, &pmd_cfg_rd);

    } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
        msg_if_status = _plp_barchetta_msg_config_pmd_rd(pa, msg_port_num, BARCHETTA_SIDE_LINE, &pmd_cfg_rd);
    }

    if(msg_if_status == BARCHETTA_MSG_IF_RET_SUCCESS) {
        *cl72_en &= pmd_cfg_rd.tx_training_opt;
       } else {
        *cl72_en &= 0;
        PHYMOD_DEBUG_ERROR(("ERROR: Reading PMD configuration Failed Due to CONFIG_PMD.Read() Error\n"));
        return PHYMOD_E_INTERNAL;
    }

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_cl72_status_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_cl72_status_t* status)
{
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0;
    uint8_t logical_lane = 0;
    uint8_t lane_index = 0;
    err_code_t __err = 0;
    status->enabled = 0xFFFF ;
    status->locked  = 0xFFFF ;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes ;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            status->enabled &= rd_linktrn_ieee_training_enable();
            if (__err != 0) {
                PHYMOD_DEBUG_ERROR(("ERROR: Read Training enable failed\n"));
                return __err;
            }
            if ((status->enabled) && (rd_linktrn_ieee_receiver_status())) {
                status->locked  &= rd_pmd_rx_lock();
                if (__err != 0) {
                    PHYMOD_DEBUG_ERROR(("ERROR: Read Training locked(rx_pmd) failed\n"));
                    return __err;
                }
            } else {
                status->locked = 0;
            }
        }
    }

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_autoneg_block_config(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_autoneg_ability_t* an_ability, uint32_t enable)
{
    barchetta_package_info_t pkg_info;
    int port_number = 0, no_of_lanes = 0;
    barchetta_port_config_t port_cfg;
    BCMI_BARCHETTA_AN_CONFIG_CTRL_AN_CONFIGr_t an_config;
    BCMI_BARCHETTA_AN_CONFIG_BLK_RSTr_t blk_rst;
    BCMI_BARCHETTA_DP_PORT_PORT_CFGr_t dp_port_cfg;
    BCMI_BARCHETTA_CL73_USER_BLK0_CL73_UCTRL2r_t uctrl2;

    PHYMOD_MEMSET(&uctrl2, 0, sizeof(BCMI_BARCHETTA_CL73_USER_BLK0_CL73_UCTRL2r_t));
    PHYMOD_MEMSET(&dp_port_cfg, 0, sizeof(BCMI_BARCHETTA_DP_PORT_PORT_CFGr_t));
    PHYMOD_MEMSET(&an_config, 0, sizeof(BCMI_BARCHETTA_AN_CONFIG_CTRL_AN_CONFIGr_t));
    PHYMOD_MEMSET(&port_cfg, 0, sizeof(barchetta_port_config_t));
    PHYMOD_MEMSET(&blk_rst, 0, sizeof(BCMI_BARCHETTA_AN_CONFIG_BLK_RSTr_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));

    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_msg_config_port_rd(&phy->access, port_number, &port_cfg));

    no_of_lanes = (BARCHETTA_IS_SYSTEM_SIDE(phy)) ?
                  (port_cfg.port_lanes.duplex.num_sys_lanes):
                  (port_cfg.port_lanes.duplex.num_line_lanes);
    if (no_of_lanes == 1) {
        BCMI_BARCHETTA_AN_CONFIG_CTRL_AN_CONFIGr_AN_PORT_TYPEf_SET(an_config, 0);
    } else if (no_of_lanes == 2) {
        BCMI_BARCHETTA_AN_CONFIG_CTRL_AN_CONFIGr_AN_PORT_TYPEf_SET(an_config, 1);
    } else if (no_of_lanes == 4) {
        BCMI_BARCHETTA_AN_CONFIG_CTRL_AN_CONFIGr_AN_PORT_TYPEf_SET(an_config, 2);
    } else{
        BCMI_BARCHETTA_AN_CONFIG_CTRL_AN_CONFIGr_AN_PORT_TYPEf_SET(an_config, 3);
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1, BarchettaRegisterSelectIngress, BarchettaRegisterTypeAN, 0xffff));

    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_AN_CONFIG_CTRL_AN_CONFIGr(&phy->access, an_config));

    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_AN_CONFIG_BLK_RSTr(&phy->access, &blk_rst));
    if (enable) {
        BCMI_BARCHETTA_AN_CONFIG_BLK_RSTr_RG_AN_BLK_ENf_SET(blk_rst, 1);
    } else {
        BCMI_BARCHETTA_AN_CONFIG_BLK_RSTr_RG_AN_BLK_ENf_SET(blk_rst, 0);
    }
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_AN_CONFIG_BLK_RSTr(&phy->access, blk_rst));

    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, 0xffff));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_DP_PORT_PORT_CFGr(&phy->access, &dp_port_cfg));

    if (an_ability) {
        BCMI_BARCHETTA_DP_PORT_PORT_CFGr_RX_DME_MAPf_SET(dp_port_cfg, (1 << an_ability->an_master_lane));
    } else {
        /* UPdate reset value*/
        BCMI_BARCHETTA_DP_PORT_PORT_CFGr_RX_DME_MAPf_SET(dp_port_cfg, 0);
    }
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_DP_PORT_PORT_CFGr(&phy->access, dp_port_cfg));


    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1, BarchettaRegisterSelectIngress, BarchettaRegisterTypeAN, 0xffff));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_CL73_USER_BLK0_CL73_UCTRL2r(&phy->access, &uctrl2));
    if (enable) {
        BCMI_BARCHETTA_CL73_USER_BLK0_CL73_UCTRL2r_DME_RXSEQDONE_WAIT_DISf_SET(uctrl2, 1);
    } else {
        BCMI_BARCHETTA_CL73_USER_BLK0_CL73_UCTRL2r_DME_RXSEQDONE_WAIT_DISf_SET(uctrl2, 0);
    }
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_CL73_USER_BLK0_CL73_UCTRL2r(&phy->access, uctrl2));


    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: To set autoneg ability configuration

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_autoneg_ability_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_autoneg_ability_t* an_ability)
{
    BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_t an_adv_1;
    BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_t an_adv_2;
    BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_t an_adv_3;
    BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_t con_up_31_16;
    BCMI_BARCHETTA_CTRL_SWGPREG8r_t cl72_en;
    BCMI_BARCHETTA_CTRL_SWGPREGFr_t ieee_ability_25G;
    barchetta_package_info_t pkg_info;
    int port_number = 0;
    BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_t can_ctrl;
    BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_t an_con_fec;
    plp_barchetta_phymod_phy_inf_config_t config;
    barchetta_aux_modes_t loc_aux_mode;

    PHYMOD_MEMSET(&an_adv_1, 0, sizeof(BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_t));
    PHYMOD_MEMSET(&an_adv_2, 0, sizeof(BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_t));
    PHYMOD_MEMSET(&an_adv_3, 0, sizeof(BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_t));
    PHYMOD_MEMSET(&con_up_31_16, 0, sizeof(BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_t));
    PHYMOD_MEMSET(&cl72_en, 0, sizeof(cl72_en));
    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&can_ctrl, 0, sizeof(BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_t));
    PHYMOD_MEMSET(&ieee_ability_25G, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREGFr_t));
    PHYMOD_MEMSET(&an_con_fec, 0, sizeof(BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_t));
    PHYMOD_MEMSET(&config, 0, sizeof(plp_barchetta_phymod_phy_inf_config_t));
    config.device_aux_modes = &loc_aux_mode;

    if (BARCHETTA_IS_SYSTEM_SIDE(phy) && (an_ability->an_fec != 0)) {
        /* FEC NOT supported in sys*/
        PHYMOD_DEBUG_ERROR(("ERROR: FEC not supported on SYS side\n"));
        return PHYMOD_E_PARAM;
    }


    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
    PHYMOD_IF_ERR_RETURN(
            __plp_barchetta_port_config_get(phy, pkg_info, &config));

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid Port number as 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
        PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_CTRL_SWGPREGFr(&phy->access, &ieee_ability_25G));

    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_READ_CTRL_SWGPREG8r(&phy->access, &cl72_en));

    cl72_en.v[0] &= ~(1 << port_number);
    cl72_en.v[0] |= ((an_ability->an_cl72 & 1) << port_number);

    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_CTRL_SWGPREG8r(&phy->access, cl72_en));
    } else {
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_reg_read(&phy->access, GPREG_AN_CL72_ENA_STS, &cl72_en.v[0]));

        cl72_en.v[0] &= ~(1 << port_number);
        cl72_en.v[0] |= ((an_ability->an_cl72 & 1) << port_number);

        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_reg_write(&phy->access, GPREG_AN_CL72_ENA_STS, cl72_en.v[0]));
    }

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_phy_autoneg_block_config(phy, an_ability, 1));

    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1, BarchettaRegisterSelectIngress, BarchettaRegisterTypeAN, 0xffff));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r(&phy->access, &an_adv_1));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r(&phy->access, &an_adv_2));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r(&phy->access, &an_adv_3));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_CONSORTIUM_AN_CAN_CTRLr(&phy->access, &can_ctrl));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_CONSORTIUM_AN_CAN_UP_47_32r(&phy->access, &an_con_fec));

    if (PHYMOD_AN_CAP_25G_KR1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CR1_GET (an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_NEXT_PAGEf_SET(an_adv_1, 1);
    } else {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_NEXT_PAGEf_SET(an_adv_1, 0);
    }
    if (PHYMOD_AN_CAP_10G_KR_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_SET(an_adv_2, (1 << 2));
    }
    if (PHYMOD_AN_CAP_40G_KR4_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_SET(an_adv_2, (1 << 3));
    }
    if (PHYMOD_AN_CAP_40G_CR4_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_SET(an_adv_2, (1 << 4));
    }
    if (PHYMOD_AN_CAP_100G_CR4_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_SET(an_adv_2, (1 << 8));
    }
    if (PHYMOD_AN_CAP_100G_KR4_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_SET(an_adv_2, (1 << 7));
    }
    if (PHYMOD_AN_CAP_50G_CR_KR_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_AN_ADV_D43_D32f_SET(an_adv_3, (1 << 2));
    }
    if (PHYMOD_AN_CAP_100G_CR2_KR2_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_AN_ADV_D43_D32f_SET(an_adv_3, (1 << 3));
    }
    if (PHYMOD_AN_CAP_200G_CR4_KR4_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_AN_ADV_D43_D32f_SET(an_adv_3, (1 << 4));
    }

    if (PHYMOD_AN_CAP_25G_CR_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_KR_GET(an_ability->an_cap)) {
        /* For KR CR enable both*/
        if (PHYMOD_AN_CAP_25G_CR_GET(an_ability->an_cap)) {
            ieee_ability_25G.v[0] &= ~(1 << port_number);
        } else {
            ieee_ability_25G.v[0] |= (BARCHETTA_25G_KR << port_number);
        }
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_SET(an_adv_2, (1 << 9)|(1 << 10));
    }
    if (PHYMOD_AN_CAP_25G_KRS1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CRS1_GET(an_ability->an_cap)) {
        if (PHYMOD_AN_CAP_25G_CRS1_GET(an_ability->an_cap)) {
            ieee_ability_25G.v[0] &= ~(1 << (port_number + BARCHETTA_25G_IEEE_KRS_SHIFT));
        } else {
            ieee_ability_25G.v[0] |= (BARCHETTA_25G_KRS << (port_number+BARCHETTA_25G_IEEE_KRS_SHIFT));
        }
        /* For KRS CRS dont enable KR/CR*/
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_SET(an_adv_2, (1 << 9));
    }

    if (PHYMOD_AN_CAP_200G_CR4_KR4_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_AN_ADV_D43_D32f_SET(an_adv_3, (1 << 4));
    }
    if (PHYMOD_AN_CAP_25G_KR1_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_UD_UP_D31_D16f_SET(con_up_31_16, (1 << 4));
    }
    if (PHYMOD_AN_CAP_25G_CR1_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_UD_UP_D31_D16f_SET(con_up_31_16, (1 << 5));
    }
    if (PHYMOD_AN_CAP_50G_KR2_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_UD_UP_D31_D16f_SET(con_up_31_16, (1 << 8));
    }
    if (PHYMOD_AN_CAP_50G_CR2_GET(an_ability->an_cap)) {
        BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_UD_UP_D31_D16f_SET(con_up_31_16, (1 << 9));
    }

    if (PHYMOD_AN_CAP_ASYM_PAUSE_GET(an_ability)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_PAUSEf_SET(an_adv_1, 1);
    } else if (PHYMOD_AN_CAP_SYMM_PAUSE_GET(an_ability)) {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_PAUSEf_SET(an_adv_1, 2);
    } else {
        /* Defaults to Some PHY*/
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_PAUSEf_SET(an_adv_1, 4);
    }
    BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_SELECTOR_FIELDf_SET(an_adv_1, 1);

    if (PHYMOD_AN_CAP_25G_KR1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CR1_GET(an_ability->an_cap) ||
        PHYMOD_AN_CAP_50G_KR2_GET(an_ability->an_cap) || PHYMOD_AN_CAP_50G_CR2_GET(an_ability->an_cap)) {
        /* Has to be set only for consortium*/
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_NEXT_PAGEf_SET(an_adv_1, 1);
        PHYMOD_IF_ERR_RETURN(
            BCMI_BARCHETTA_WRITE_CONSORTIUM_AN_CAN_UP_31_16r(&phy->access, con_up_31_16));
        BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_CL73_CANTXENf_SET(can_ctrl, 1);
        BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_CL73_CANENf_SET(can_ctrl, 1);
    } else {
        BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_NEXT_PAGEf_SET(an_adv_1, 0);
        BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_CL73_CANTXENf_SET(can_ctrl, 0);
        BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_CL73_CANENf_SET(can_ctrl, 0);
    }
    if (an_ability->an_fec == 0) { /* HW Defaults*/
        if (PHYMOD_AN_CAP_10G_KR_GET(an_ability->an_cap) || PHYMOD_AN_CAP_40G_KR4_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_40G_CR4_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_REQUESTEDf_SET(an_adv_3, 0);
        }
        if (PHYMOD_AN_CAP_25G_KRS1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CRS1_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_25G_KR_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CR_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_25G_REQUESTEDf_SET(an_adv_3, 0);
        }
        if (PHYMOD_AN_CAP_25G_KR1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CR1_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_50G_KR2_GET(an_ability->an_cap) || PHYMOD_AN_CAP_50G_CR2_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_UD_UP_D47_D32f_SET(an_con_fec, 0);
        }
    } else if (an_ability->an_fec == 1) { /* Fec Ability*/
        if (PHYMOD_AN_CAP_10G_KR_GET(an_ability->an_cap) || PHYMOD_AN_CAP_40G_KR4_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_40G_CR4_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_REQUESTEDf_SET(an_adv_3, 1);
        }
        if (PHYMOD_AN_CAP_25G_KRS1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CRS1_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_25G_KR_GET(an_ability->an_cap) ||  PHYMOD_AN_CAP_25G_CR_GET(an_ability->an_cap)) {
            PHYMOD_DEBUG_ERROR(("ERROR: FEC Ability for 25 is not supported, please use FEC request option\n"));
            return PHYMOD_E_PARAM;
        }
        if (PHYMOD_AN_CAP_25G_KR1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CR1_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_50G_KR2_GET(an_ability->an_cap) || PHYMOD_AN_CAP_50G_CR2_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_UD_UP_D47_D32f_SET(an_con_fec, 0x200);
        }
        if (PHYMOD_AN_CAP_100G_CR4_GET(an_ability->an_cap) || PHYMOD_AN_CAP_100G_KR4_GET(an_ability->an_cap)){
			PHYMOD_DEBUG_ERROR(("ERROR: Invalid AN ability : CR4/KR4\n"));
            return PHYMOD_E_PARAM;
        }

    } else if (an_ability->an_fec == 2) { /* FEC Requested*/
        if (PHYMOD_AN_CAP_10G_KR_GET(an_ability->an_cap) || PHYMOD_AN_CAP_40G_KR4_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_40G_CR4_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_REQUESTEDf_SET(an_adv_3, 3);
        }
        if (PHYMOD_AN_CAP_25G_KRS1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CRS1_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_25G_KR_GET(an_ability->an_cap) ||  PHYMOD_AN_CAP_25G_CR_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_25G_REQUESTEDf_SET(an_adv_3, 2);
        }
        if (PHYMOD_AN_CAP_25G_KR1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CR1_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_50G_KR2_GET(an_ability->an_cap) || PHYMOD_AN_CAP_50G_CR2_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_UD_UP_D47_D32f_SET(an_con_fec, 0xA00);
        }
        if (PHYMOD_AN_CAP_100G_CR4_GET(an_ability->an_cap) || PHYMOD_AN_CAP_100G_KR4_GET(an_ability->an_cap)){
			PHYMOD_DEBUG_ERROR(("ERROR: Invalid AN ability : CR4/KR4\n"));
            return PHYMOD_E_PARAM;
        }
    } else if (an_ability->an_fec == 4) { /* RS FEC*/
        if (PHYMOD_AN_CAP_10G_KR_GET(an_ability->an_cap) || PHYMOD_AN_CAP_40G_KR4_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_40G_CR4_GET(an_ability->an_cap)) {
			PHYMOD_DEBUG_ERROR(("ERROR: Invalid AN ability \n"));
            return PHYMOD_E_PARAM;
        }
        if (PHYMOD_AN_CAP_25G_KRS1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CRS1_GET(an_ability->an_cap) ||
            PHYMOD_AN_CAP_25G_KR_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CR_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_25G_REQUESTEDf_SET(an_adv_3, 1);
        }
        if (PHYMOD_AN_CAP_25G_KR1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CR1_GET(an_ability->an_cap) ||
             PHYMOD_AN_CAP_50G_KR2_GET(an_ability->an_cap) || PHYMOD_AN_CAP_50G_CR2_GET(an_ability->an_cap)) {
            BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_UD_UP_D47_D32f_SET(an_con_fec, 0x500);
        }
        if (PHYMOD_AN_CAP_100G_CR4_GET(an_ability->an_cap) || PHYMOD_AN_CAP_100G_KR4_GET(an_ability->an_cap)){
			PHYMOD_DEBUG_ERROR(("ERROR: Invalid AN ability : CR4/KR4\n"));
            return PHYMOD_E_PARAM;
        }
    }
    if (an_ability->an_cap == 0) {
        if (config.data_rate == 25000 || config.data_rate == 50000) {
            BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_NEXT_PAGEf_SET(an_adv_1, 1);
            an_adv_2.v[0] = 0 ;
            con_up_31_16.v[0] = 0 ;
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_CONSORTIUM_AN_CAN_UP_31_16r(&phy->access, con_up_31_16));
            BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_CL73_CANTXENf_SET(can_ctrl, 1);
            BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_CL73_CANENf_SET(can_ctrl, 1);
        } else {
            an_adv_2.v[0] = 0 ;
            con_up_31_16.v[0] = 0 ;
            PHYMOD_IF_ERR_RETURN(
                BCMI_BARCHETTA_WRITE_CONSORTIUM_AN_CAN_UP_31_16r(&phy->access, con_up_31_16));
            BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_CL73_CANTXENf_SET(can_ctrl, 0);
            BCMI_BARCHETTA_CONSORTIUM_AN_CAN_CTRLr_CL73_CANENf_SET(can_ctrl, 0);
        }
    }
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r(&phy->access, an_adv_1));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r(&phy->access, an_adv_2));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r(&phy->access, an_adv_3));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_CONSORTIUM_AN_CAN_CTRLr(&phy->access, can_ctrl));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_CONSORTIUM_AN_CAN_UP_47_32r(&phy->access, an_con_fec));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_WRITE_CTRL_SWGPREGFr(&phy->access, ieee_ability_25G));

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_autoneg_ability_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_autoneg_ability_t* an_ability)
{
    BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_t an_adv_1;
    BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_t an_adv_2;
    BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_t an_adv_3;
    BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_t con_up_31_16;
    BCMI_BARCHETTA_DP_PORT_PORT_CFGr_t dp_port_cfg;
    barchetta_package_info_t pkg_info;
    int port_number = 0;
    BCMI_BARCHETTA_CTRL_SWGPREG8r_t cl72_en;
    BCMI_BARCHETTA_CTRL_SWGPREGFr_t ieee_ability_25G;
    BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_t an_con_fec;

    PHYMOD_MEMSET(&an_adv_1, 0, sizeof(BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_t));
    PHYMOD_MEMSET(&cl72_en, 0, sizeof(cl72_en));
    PHYMOD_MEMSET(&an_adv_2, 0, sizeof(BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_t));
    PHYMOD_MEMSET(&an_adv_3, 0, sizeof(BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_t));
    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&con_up_31_16, 0, sizeof(BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_t));
    PHYMOD_MEMSET(an_ability, 0, sizeof(plp_barchetta_phymod_autoneg_ability_t));
    PHYMOD_MEMSET(&ieee_ability_25G, 0, sizeof(BCMI_BARCHETTA_CTRL_SWGPREGFr_t));
    PHYMOD_MEMSET(&an_con_fec, 0, sizeof(BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_t));


    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid Port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }

    if (BARCHETTA_IS_SYSTEM_SIDE(phy)){
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_CTRL_SWGPREG8r(&phy->access, &cl72_en));
    } else {
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_reg_read(&phy->access, GPREG_AN_CL72_ENA_STS, &cl72_en.v[0]));
    }


    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_CTRL_SWGPREGFr(&phy->access, &ieee_ability_25G));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_CONSORTIUM_AN_CAN_UP_47_32r(&phy->access, &an_con_fec));

        PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1, BarchettaRegisterSelectIngress, BarchettaRegisterTypeAN, 0xffff));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r(&phy->access, &an_adv_1));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r(&phy->access, &an_adv_2));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r(&phy->access, &an_adv_3));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_CONSORTIUM_AN_CAN_UP_31_16r(&phy->access, &con_up_31_16));

    if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_GET(an_adv_2) & (1 << 2)) {
        PHYMOD_AN_CAP_10G_KR_SET(an_ability->an_cap);
    } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_GET(an_adv_2) & (1 << 3)) {
        PHYMOD_AN_CAP_40G_KR4_SET(an_ability->an_cap);
    } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_GET(an_adv_2) & (1 << 4)) {
        PHYMOD_AN_CAP_40G_CR4_SET(an_ability->an_cap);
    } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_GET(an_adv_2) & (1 << 7)) {
        PHYMOD_AN_CAP_100G_KR4_SET(an_ability->an_cap);
    } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_GET(an_adv_2) & (1 << 8)) {
        PHYMOD_AN_CAP_100G_CR4_SET(an_ability->an_cap);
    } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_GET(an_adv_2) & (1 << 10)) {
        if (ieee_ability_25G.v[0] & (1 << port_number)) {
            PHYMOD_AN_CAP_25G_KR_SET(an_ability->an_cap);
            PHYMOD_AN_CAP_25G_KRS1_SET(an_ability->an_cap);
        } else {
            PHYMOD_AN_CAP_25G_CR_SET(an_ability->an_cap);
            PHYMOD_AN_CAP_25G_CRS1_SET(an_ability->an_cap);
        }
    } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_TECHABILITYf_GET(an_adv_2) & (1 << 9)) {
        if (ieee_ability_25G.v[0] & (1 << (port_number + BARCHETTA_25G_IEEE_KRS_SHIFT))) {
        PHYMOD_AN_CAP_25G_KRS1_SET(an_ability->an_cap);
        } else {
        PHYMOD_AN_CAP_25G_CRS1_SET(an_ability->an_cap);
        }
    } else if (BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_UD_UP_D31_D16f_GET(con_up_31_16) & (1 << 4)) {
        PHYMOD_AN_CAP_25G_KR1_SET(an_ability->an_cap);
    } else if (BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_UD_UP_D31_D16f_GET(con_up_31_16) & (1 << 5)) {
        PHYMOD_AN_CAP_25G_CR1_SET(an_ability->an_cap);
    } else if (BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_UD_UP_D31_D16f_GET(con_up_31_16) &(1 << 8)) {
        PHYMOD_AN_CAP_50G_KR2_SET(an_ability->an_cap);
    } else if(BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_31_16r_UD_UP_D31_D16f_GET(con_up_31_16) & (1 <<9)) {
        PHYMOD_AN_CAP_50G_CR2_SET(an_ability->an_cap);
    }

    if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_AN_ADV_D43_D32f_GET(an_adv_3) & (1 << 2)) {
        PHYMOD_AN_CAP_50G_CR_KR_SET(an_ability->an_cap);
    } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_AN_ADV_D43_D32f_GET(an_adv_3) & (1 << 3)) {
        PHYMOD_AN_CAP_100G_CR2_KR2_SET(an_ability->an_cap);
    } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_AN_ADV_D43_D32f_GET(an_adv_3) & (1 << 4)) {
        PHYMOD_AN_CAP_200G_CR4_KR4_SET(an_ability->an_cap);
    }
    if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_PAUSEf_GET(an_adv_1) == 1) {
        PHYMOD_AN_CAP_ASYM_PAUSE_SET(an_ability);
    } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_1r_PAUSEf_GET(an_adv_1) == 2) {
        PHYMOD_AN_CAP_SYMM_PAUSE_SET(an_ability);
    }
    an_ability->an_cl72 = ((cl72_en.v[0] & (1 << port_number)) ? 1 :0) ;
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1, BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy), BarchettaRegisterTypePMD, 0xffff));
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_DP_PORT_PORT_CFGr(&phy->access, &dp_port_cfg));
    an_ability->an_master_lane = __plp_barchetta_log2n(BCMI_BARCHETTA_DP_PORT_PORT_CFGr_RX_DME_MAPf_GET(dp_port_cfg));

    if (PHYMOD_AN_CAP_10G_KR_GET(an_ability->an_cap) || PHYMOD_AN_CAP_40G_KR4_GET(an_ability->an_cap) ||
        PHYMOD_AN_CAP_40G_CR4_GET(an_ability->an_cap)) {
        if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_REQUESTEDf_GET(an_adv_3) == 1) {
            an_ability->an_fec = 1;
        } else if(BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_REQUESTEDf_GET(an_adv_3) == 3) {
            an_ability->an_fec = 2;
        } else {
            an_ability->an_fec = 0;
        }
    }
    if (PHYMOD_AN_CAP_25G_KRS1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CRS1_GET(an_ability->an_cap) ||
        PHYMOD_AN_CAP_25G_KR_GET(an_ability->an_cap) ||  PHYMOD_AN_CAP_25G_CR_GET(an_ability->an_cap)) {
        if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_25G_REQUESTEDf_GET(an_adv_3) == 2) {
            an_ability->an_fec = 2;
        } else if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_25G_REQUESTEDf_GET(an_adv_3) == 1) {
            an_ability->an_fec = 4;
        }
    }
    if (PHYMOD_AN_CAP_100G_CR4_GET(an_ability->an_cap) || PHYMOD_AN_CAP_100G_KR4_GET(an_ability->an_cap)){
        if (BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_3r_FEC_25G_REQUESTEDf_GET(an_adv_3) == 1) {
            an_ability->an_fec = 4;
        } else {
            an_ability->an_fec = 0;
        }
    }
    if (PHYMOD_AN_CAP_25G_KR1_GET(an_ability->an_cap) || PHYMOD_AN_CAP_25G_CR1_GET(an_ability->an_cap) ||
        PHYMOD_AN_CAP_50G_KR2_GET(an_ability->an_cap) || PHYMOD_AN_CAP_50G_CR2_GET(an_ability->an_cap)) {
        if (BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_UD_UP_D47_D32f_GET(an_con_fec) == 0xA00) {
            an_ability->an_fec = 2;
        } else if (BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_UD_UP_D47_D32f_GET(an_con_fec) == 0x500) {
            an_ability->an_fec = 4;
        } else if (BCMI_BARCHETTA_CONSORTIUM_AN_CAN_UP_47_32r_UD_UP_D47_D32f_GET(an_con_fec) == 0x200) {
            an_ability->an_fec = 1;
        } else {
            an_ability->an_fec = 0;
        }
    }


    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_autoneg_set(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_autoneg_control_t* an)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info    ;
    barchetta_modify_port_t  port_modify ;
    barchetta_config_pmd_t   pmd_cfg_rd  ;
    plp_barchetta_phymod_autoneg_ability_t an_ability_get_type ;
    int port_number = 0xFF, get_master_from_logical = 0, lane_index = 0;
    BCMI_BARCHETTA_CTRL_SWGPREG8r_t cl72_en;
    uint8_t msg_if_status = BARCHETTA_MSG_IF_RET_ERROR, logical_lane=0, msg_port_num=0;
#ifdef BCM_PLP_RMT_PHY_CAPABILITY
    uint32_t ld_spd = 0;
    barchetta_lane_data_rate_t lane_datarate ;
    barchetta_port_config_t port_cfg_rd ;
#endif

    PHYMOD_MEMSET(&an_ability_get_type,  0, sizeof(plp_barchetta_phymod_autoneg_ability_t));
    PHYMOD_MEMSET(&pkg_info,    0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&port_modify, 0, sizeof(barchetta_modify_port_t));
    PHYMOD_MEMSET(&cl72_en, 0, sizeof(cl72_en));
    PHYMOD_MEMSET(&pmd_cfg_rd,  0, sizeof(barchetta_config_pmd_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid Port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
    msg_port_num = port_number & 0xFF;
    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_CTRL_SWGPREG8r(&phy->access, &cl72_en));
    } else {
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_reg_read(&phy->access, GPREG_AN_CL72_ENA_STS, &cl72_en.v[0]));
    }
    msg_if_status = _plp_barchetta_msg_config_pmd_rd(pa, msg_port_num,
             ((BARCHETTA_IS_SYSTEM_SIDE(phy)) ? BARCHETTA_SIDE_SYS : BARCHETTA_SIDE_LINE), &pmd_cfg_rd);
#ifdef BCM_PLP_RMT_PHY_CAPABILITY
    for (lane_index = 0; lane_index < pkg_info.no_of_max_ports; lane_index ++) {
        if (phy->access.lane_mask & ( 1 <<lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_restore_lane_data_rate(phy, lane_index, &lane_datarate));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_msg_config_port_rd(&phy->access, port_number, &port_cfg_rd));

            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_read(&phy->access, GPREG_FW_TOMAHAWK_SPEED_ADR, &ld_spd));
            ld_spd &= ~(3 << (port_number*2));
            if ((lane_datarate == BARCHETTA_LANE_DATA_RATE_10P3125G) && (port_cfg_rd.port_lanes.duplex.num_sys_lanes == 1)) {
                ld_spd |= (BARCHETTA_PORT_10G << (port_number*2));
            } else if ((lane_datarate == BARCHETTA_LANE_DATA_RATE_25P78125G) && (port_cfg_rd.port_lanes.duplex.num_sys_lanes == 1)) {
                ld_spd |= (BARCHETTA_PORT_25G << (port_number*2));
            } else if ((lane_datarate == BARCHETTA_LANE_DATA_RATE_25P78125G) && (port_cfg_rd.port_lanes.duplex.num_sys_lanes == 2)) {
                ld_spd |= (BARCHETTA_PORT_50G << (port_number*2));
                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_reg_write(&phy->access, GPREG_FW_TOMAHAWK_SPEED_ADR, ld_spd));
                if (pmd_cfg_rd.an_opt) {
                    return PHYMOD_E_NONE;
                }
                break;
            } else {
                ld_spd |= (BARCHETTA_PORT_NULL << (port_number*2));
            }
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_write(&phy->access, GPREG_FW_TOMAHAWK_SPEED_ADR, ld_spd));
            if (pmd_cfg_rd.an_opt) {
                return PHYMOD_E_NONE;
            }
        }
    }
#endif
    if(msg_if_status == BARCHETTA_MSG_IF_RET_SUCCESS) {
        if(an->enable) {
            PHYMOD_IF_ERR_RETURN(
                  _plp_barchetta_phy_autoneg_ability_get(phy, &an_ability_get_type));
            for (lane_index = 0; lane_index < pkg_info.pkg_lanes; lane_index ++) {
                if (phy->access.lane_mask & (1 << lane_index)) {
                    if (get_master_from_logical == an_ability_get_type.an_master_lane) {
                        PHYMOD_IF_ERR_RETURN(
                            _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                        break;
                    } else {
                        get_master_from_logical++;
                    }
                }
            }
            pmd_cfg_rd.an_opt = 0x01 ;
            pmd_cfg_rd.an_master_lane = logical_lane;
            pmd_cfg_rd.an_use_pcs_mon = 0x0 ;
            pmd_cfg_rd.tx_training_opt = ((cl72_en.v[0] & (1 << port_number)) ? 1 :0) ;
            pmd_cfg_rd.fec_opt = 1 ;
        } else {
            PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_phy_autoneg_block_config(phy, NULL, 0));
            pmd_cfg_rd.tx_training_opt = pmd_cfg_rd.an_opt = 0x00 ;
        }
        PHYMOD_MEMCPY(&port_modify, &pmd_cfg_rd, sizeof(barchetta_config_pmd_t));
        if(BARCHETTA_MSG_IF_RET_ERROR ==_plp_barchetta_msg_modify_port(pa, &port_modify)) {
           PHYMOD_DEBUG_ERROR(("ERROR: Port modification Failed Due to MODIFY_PORT.Start() Error\n"));
           return PHYMOD_E_INTERNAL;
       }
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: Reading PMD configuration Failed Due to CONFIG_PMD.Read() Error\n"));
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_autoneg_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_autoneg_control_t* an, uint32_t *an_done)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info    ;
    BCMI_BARCHETTA_IEEE_AN_BLK0_AN_STATUSr_t an_status ;
    BCMI_BARCHETTA_IEEE_AN_BLK0_AN_CONTROLr_t an_enable;
    int port_number = 0xFF;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&an_enable, 0, sizeof(an_enable));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));

    PHYMOD_IF_ERR_RETURN(
             _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid Port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1, BarchettaRegisterSelectIngress, BarchettaRegisterTypeAN, 0xffff));

    PHYMOD_IF_ERR_RETURN(
        BCMI_BARCHETTA_READ_IEEE_AN_BLK0_AN_CONTROLr(pa, &an_enable));
    an->enable = BCMI_BARCHETTA_IEEE_AN_BLK0_AN_CONTROLr_AUTO_NEGOTIATIONENABLEf_GET(an_enable);
    /* Read the AN status */
    PHYMOD_IF_ERR_RETURN (
           BCMI_BARCHETTA_READ_IEEE_AN_BLK0_AN_STATUSr(pa, &an_status));

    if (BCMI_BARCHETTA_IEEE_AN_BLK0_AN_STATUSr_AUTO_NEGOTIATIONCOMPLETEf_GET(an_status)) {
        *an_done = 1;
    } else {
        *an_done = 0;
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_firmware_lane_config_set(const plp_barchetta_phymod_phy_access_t* phy,
        phymod_firmware_lane_config_t fw_lane_config) {
    struct blackhawk_barchetta_uc_lane_config_st lane_cfg_st ;
    barchetta_package_info_t pkg_info    ;
    barchetta_modify_port_t  port_modify ;
    barchetta_config_pmd_t   pmd_cfg_rd  ;
    int port_number = 0xFF, lane_index = 0;
    uint8_t logical_lane = 0, msg_port_num = 0;
    uint8_t msg_if_status = BARCHETTA_MSG_IF_RET_ERROR ;
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;

    PHYMOD_MEMSET(&pkg_info,    0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&port_modify, 0, sizeof(barchetta_modify_port_t));
    PHYMOD_MEMSET(&pmd_cfg_rd,  0, sizeof(barchetta_config_pmd_t));
    PHYMOD_MEMSET(&lane_cfg_st, 0, sizeof(struct blackhawk_barchetta_uc_lane_config_st));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));

    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid Port number 0xFF\n"));
       return PHYMOD_E_PARAM;
    }

    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1,
            BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy), BarchettaRegisterTypePMD, 0xffff));
    PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_init_blackhawk_barchetta_info(sa__));

    msg_port_num = port_number & 0xff;
    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        msg_if_status = _plp_barchetta_msg_config_pmd_rd(sa__, msg_port_num, BARCHETTA_SIDE_SYS, &pmd_cfg_rd);

    } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
        msg_if_status = _plp_barchetta_msg_config_pmd_rd(sa__, msg_port_num, BARCHETTA_SIDE_LINE, &pmd_cfg_rd);
    }
    for (lane_index = 0; lane_index < pkg_info.pkg_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
               _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            PHYMOD_IF_ERR_RETURN(
                wrv_usr_ctrl_pam4_chn_loss(fw_lane_config.DbLoss));
        }
    }
    if(msg_if_status == BARCHETTA_MSG_IF_RET_SUCCESS) {
        lane_cfg_st.field.lane_cfg_from_pcs        = (fw_lane_config.LaneConfigFromPCS     & 0xFF) ;
        lane_cfg_st.field.an_enabled               = (fw_lane_config.AnEnabled             & 0xFF) ;
        lane_cfg_st.field.dfe_on                   = (fw_lane_config.DfeOn                 & 0xFF) ;
        lane_cfg_st.field.dfe_lp_mode              = (fw_lane_config.LpDfeOn               & 0xFF) ;
        lane_cfg_st.field.force_brdfe_on           = (fw_lane_config.ForceBrDfe            & 0xFF) ;
        lane_cfg_st.field.media_type               = (fw_lane_config.MediaType             & 0xFF) ;
        lane_cfg_st.field.unreliable_los           = (fw_lane_config.UnreliableLos         & 0xFF) ;
        lane_cfg_st.field.scrambling_dis           = (fw_lane_config.ScramblingDisable     & 0xFF) ;
        lane_cfg_st.field.cl72_auto_polarity_en    = (fw_lane_config.Cl72AutoPolEn         & 0xFF) ;
        lane_cfg_st.field.cl72_restart_timeout_en  = (fw_lane_config.Cl72RestTO            & 0xFF) ;
        lane_cfg_st.field.force_es                 = (fw_lane_config.ForceES               & 0xFF) ;
        lane_cfg_st.field.force_ns                 = (fw_lane_config.ForceNS               & 0xFF) ;
        lane_cfg_st.field.lp_has_prec_en           = (fw_lane_config.LinkPartnerPrecoderEn & 0xFF) ;
        lane_cfg_st.field.force_pam4_mode          = (fw_lane_config.ForcePAM4Mode         & 0xFF) ;
        lane_cfg_st.field.force_nrz_mode           = (fw_lane_config.ForceNRZMode          & 0xFF) ;

        PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_INTERNAL_update_uc_lane_config_word(&lane_cfg_st));
        pmd_cfg_rd.lane_config_word = lane_cfg_st.word ;

        PHYMOD_MEMCPY(&port_modify, &pmd_cfg_rd, sizeof(barchetta_config_pmd_t));
        if(BARCHETTA_MSG_IF_RET_ERROR ==_plp_barchetta_msg_modify_port(sa__, &port_modify)) {
            PHYMOD_DEBUG_ERROR(("ERROR: Port modification Failed Due to MODIFY_PORT.Start() Error\n"));
           return PHYMOD_E_INTERNAL;
        }
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: Reading PMD configuration Failed Due to CONFIG_PMD.Read() Error\n"));
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_firmware_lane_config_get(const plp_barchetta_phymod_phy_access_t* phy,
        phymod_firmware_lane_config_t* fw_lane_config) {
    struct blackhawk_barchetta_uc_lane_config_st lane_cfg_st ;
    barchetta_package_info_t pkg_info    ;
    barchetta_config_pmd_t   pmd_cfg_rd  ;
    int port_number = 0xFF, lane_index = 0;
    uint8_t logical_lane = 0, msg_port_num = 0;
    uint8_t msg_if_status = BARCHETTA_MSG_IF_RET_ERROR ;
    err_code_t __err = 0;
    const plp_barchetta_phymod_access_t *sa__ = &phy->access;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&lane_cfg_st, 0, sizeof(struct blackhawk_barchetta_uc_lane_config_st));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(sa__, &pkg_info));

    PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));

    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid Port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice(phy, port_number, 1,
            BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy), BarchettaRegisterTypePMD, 0xffff));
    PHYMOD_IF_ERR_RETURN(
                plp_barchetta_blackhawk_barchetta_init_blackhawk_barchetta_info(sa__));

    msg_port_num = port_number & 0xFF;
    for (lane_index = 0; lane_index < pkg_info.pkg_lanes; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
               _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_TX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
            fw_lane_config->DbLoss = rdv_usr_ctrl_pam4_chn_loss();
            if (__err != 0) {
                PHYMOD_DEBUG_ERROR(("ERROR: Read DB loss failed\n"));
                return __err;
            }
            break;
        }
    }

    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        msg_if_status = _plp_barchetta_msg_config_pmd_rd(sa__, msg_port_num, BARCHETTA_SIDE_SYS, &pmd_cfg_rd);

    } else if (BARCHETTA_IS_LINE_SIDE(phy)) {
        msg_if_status = _plp_barchetta_msg_config_pmd_rd(sa__, msg_port_num, BARCHETTA_SIDE_LINE, &pmd_cfg_rd);
    }
    if(msg_if_status == BARCHETTA_MSG_IF_RET_SUCCESS) {
        lane_cfg_st.word = pmd_cfg_rd.lane_config_word ;
        PHYMOD_IF_ERR_RETURN(plp_barchetta_blackhawk_barchetta_INTERNAL_update_uc_lane_config_st(&lane_cfg_st));
        fw_lane_config->LaneConfigFromPCS     = lane_cfg_st.field.lane_cfg_from_pcs       ;
        fw_lane_config->AnEnabled             = lane_cfg_st.field.an_enabled              ;
        fw_lane_config->DfeOn                 = lane_cfg_st.field.dfe_on                  ;
        fw_lane_config->LpDfeOn               = lane_cfg_st.field.dfe_lp_mode             ;
        fw_lane_config->ForceBrDfe            = lane_cfg_st.field.force_brdfe_on          ;
        fw_lane_config->MediaType             = lane_cfg_st.field.media_type              ;
        fw_lane_config->UnreliableLos         = lane_cfg_st.field.unreliable_los          ;
        fw_lane_config->ScramblingDisable     = lane_cfg_st.field.scrambling_dis          ;
        fw_lane_config->Cl72AutoPolEn         = lane_cfg_st.field.cl72_auto_polarity_en   ;
        fw_lane_config->Cl72RestTO            = lane_cfg_st.field.cl72_restart_timeout_en ;
        fw_lane_config->ForceES               = lane_cfg_st.field.force_es                ;
        fw_lane_config->ForceNS               = lane_cfg_st.field.force_ns                ;
        fw_lane_config->LinkPartnerPrecoderEn = lane_cfg_st.field.lp_has_prec_en          ;
        fw_lane_config->ForcePAM4Mode         = lane_cfg_st.field.force_pam4_mode         ;
        fw_lane_config->ForceNRZMode          = lane_cfg_st.field.force_nrz_mode          ;
    }
    else {
        PHYMOD_DEBUG_ERROR(("ERROR: Reading PMD configuration Failed Due to CONFIG_PMD.Read() Error\n"));
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_autoneg_remote_ability_get(const plp_barchetta_phymod_phy_access_t* phy,
        plp_barchetta_phymod_autoneg_ability_t* an_ability_get_type)
{
    BCMI_BARCHETTA_IEEE_AN_BLK1_AN_LP_BASE_PAGE_ABILITY_2r_t lp_an_adv_2;
    barchetta_package_info_t pkg_info;
    int port_number = 0;
    uint32_t port_speed = 0;

    PHYMOD_MEMSET(&lp_an_adv_2, 0, sizeof(BCMI_BARCHETTA_IEEE_AN_BLK1_AN_ADVERTISEMENT_2r_t));
    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(an_ability_get_type, 0, sizeof(plp_barchetta_phymod_autoneg_ability_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(&phy->access, &pkg_info));
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if (port_number == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid Port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }
#ifdef BCM_PLP_RMT_PHY_CAPABILITY
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_read(&phy->access, GPREG_FW_PHY_PORT_SPEED_ADR, &port_speed));
    port_speed = port_speed & (3 << (port_number*2));
    port_speed >>= (port_number*2);
    if (port_speed == BARCHETTA_PORT_10G) {
        PHYMOD_AN_CAP_10G_KR_SET(an_ability_get_type->an_cap);
    } else if (port_speed == BARCHETTA_PORT_25G) {
        PHYMOD_AN_CAP_25G_KR_SET(an_ability_get_type->an_cap);
    } else if (port_speed == BARCHETTA_PORT_50G) {
        PHYMOD_AN_CAP_50G_KR2_SET(an_ability_get_type->an_cap);
    } else {
        an_ability_get_type->an_cap = 0;
    }
    an_ability_get_type->an_fec = 0;
    an_ability_get_type->pause = 0;
    an_ability_get_type->an_master_lane = 0;
#else
    (void) port_speed;
    (void) port_number;
    PHYMOD_DEBUG_ERROR(("Not available for general PHY\n"));
    return PHYMOD_E_UNAVAIL;
#endif
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int _plp_barchetta_get_port_lane(const plp_barchetta_phymod_phy_access_t* phy, int max_lane, int lane_map, int port_number, int *port_lane)
{
    int port_lane_map = 0, lane_index = 0, port_lane_index = 0;

    if (BARCHETTA_IS_SYSTEM_SIDE(phy)) {
        port_lane_map = plp_barchetta_sw_db[phy->access.addr][port_number].lane_map_info.lane_map.duplex_lane_map.sys_lane_map;
    } else {
        port_lane_map = plp_barchetta_sw_db[phy->access.addr][port_number].lane_map_info.lane_map.duplex_lane_map.line_lane_map;
    }
    for (lane_index = 0; lane_index < max_lane ; lane_index ++) {
       if (port_lane_map & (1 << lane_index)) {
            if (((1 << lane_index) & lane_map) == lane_map) {
                break;
            }
            port_lane_index++;
        }
    }

    if (lane_index >= max_lane) {
        PHYMOD_DEBUG_ERROR(("Error: Unable to find Port lane \n"));
        return PHYMOD_E_PARAM;
    }
    *port_lane = port_lane_index;

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_intr_enable_set(
        const plp_barchetta_phymod_phy_access_t* phy, uint32_t enable) {
    barchetta_package_info_t pkg_info;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t  top_eier_rd = 0 ;
    uint32_t  top_eier_wr = 0 ;
    uint32_t  m0_eier_rd  = 0 ;
    uint32_t  m0_eier_wr  = 0 ;
    uint32_t  common_misc_eier_rd = 0 ;
    uint32_t  common_misc_eier_wr = 0 ;
    uint32_t  px_A_eier_rd = 0;
    uint32_t  px_A_eier_wr = 0;
    uint32_t  px_B_eier_rd = 0;
    uint32_t  px_B_eier_wr = 0;
    uint32_t  mst_m0_eier_rd = 0 ;
    uint32_t  mst_m0_eier_wr = 0 ;

    int port_number = 0xFF;
    uint32_t intr_enable = 0;
    uint8_t  pll_idx = 0, sys_side = 0;
    uint32_t pkg_lane_map = __plp_barchetta_convert_logical_lanemap_to_package_lanemap(pa);

    PHYMOD_MEMSET(&pkg_info , 0, sizeof(barchetta_package_info_t));

    intr_enable = (enable >> 31) & 1;

    if (BARCHETTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }

    if ((enable & BARCHETTA_INTR_M0_MST_MISC) || (enable & BARCHETTA_INTR_M0_MST_MSGOUT)) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_TOP_EIER_ADDR, &top_eier_rd));
        top_eier_wr = ((top_eier_rd & ~TOP_M0_ENABLE_MASK) | ((intr_enable & 0x1) << TOP_M0_ENABLE_BIT_POS));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_TOP_EIER_ADDR,  top_eier_wr));
    }
    /* BARCHETTA_INTR_M0_MST_MISC */
    if (enable & BARCHETTA_INTR_M0_MST_MISC) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_M0_EIER_ADDR, &m0_eier_rd));
        m0_eier_wr = ((m0_eier_rd & ~M0_ENABLE_MST_MISC_INTR_MASK) | ((intr_enable & 0x1) << M0_ENABLE_MST_MISC_INTR_BIT_POS));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_M0_EIER_ADDR,  m0_eier_wr));
    }

    /* BARCHETTA_INTR_M0_MST_MSGOUT */
    if(enable & BARCHETTA_INTR_M0_MST_MSGOUT) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_M0_EIER_ADDR, &m0_eier_rd));
        m0_eier_wr = ((m0_eier_rd & ~M0_ENABLE_MST_MSGOUT_INTR_MASK) | ((intr_enable & 0x1) << M0_ENABLE_MST_MSGOUT_INTR_BIT_POS));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_M0_EIER_ADDR,  m0_eier_wr));
    }

    /* PLL interrupts */
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_pll_index(phy, &pll_idx));
    if ((enable & BARCHETTA_INTR_PLL_LOCK_FOUND) || (enable & BARCHETTA_INTR_PLL_LOCK_LOST)) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_TOP_EIER_ADDR, &top_eier_rd));
        top_eier_wr = ((top_eier_rd & ~TOP_M0_ENABLE_COMMON_MISC_MASK) | ((intr_enable & 0x1) <<TOP_M0_ENABLE_COMMON_MISC_BIT_POS));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_TOP_EIER_ADDR,  top_eier_wr));
    }
    if(enable & BARCHETTA_INTR_PLL_LOCK_FOUND) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_COMMON_MISC_EIER_ADDR, &common_misc_eier_rd));
        common_misc_eier_wr = (common_misc_eier_rd & ~(1 << (LIN_PMD_PLL0_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx*2))));
        common_misc_eier_wr |= ((intr_enable & 0x1) << (LIN_PMD_PLL0_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx*2)));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_COMMON_MISC_EIER_ADDR, common_misc_eier_wr));
    }
    if (enable & BARCHETTA_INTR_PLL_LOCK_LOST) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_COMMON_MISC_EIER_ADDR, &common_misc_eier_rd));
        common_misc_eier_wr = (common_misc_eier_rd & ~(1 << (LIN_PMD_PLL0_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx*2))));
        common_misc_eier_wr |= ((intr_enable & 0x1) << (LIN_PMD_PLL0_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx*2)));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_COMMON_MISC_EIER_ADDR,  common_misc_eier_wr));
    }

    /* Module Absent Interrupt */
    if (enable & BARCHETTA_INTR_PMD_MOD_ABS) {
        if(pkg_lane_map & 0xF) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_MST_M0_EIER_ADDR, &mst_m0_eier_rd));
            mst_m0_eier_wr = ((mst_m0_eier_rd & ~MOD_ABS_0_MASK) | ((intr_enable & 0x1) << MOD_ABS_0_BIT_POS)) ;
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_MST_M0_EIER_ADDR, mst_m0_eier_wr));
        }
        if(pkg_lane_map & 0xF0) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_MST_M0_EIER_ADDR, &mst_m0_eier_rd));
            mst_m0_eier_wr = ((mst_m0_eier_rd & ~MOD_ABS_1_MASK) | ((intr_enable & 0x1) << MOD_ABS_1_BIT_POS)) ;
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_MST_M0_EIER_ADDR, mst_m0_eier_wr));
        }
    }

    /* AN complete, AN restart, PMD Rx Signal Detect and PMD Rx lock Interrupt */
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    if((0 <= port_number) && (port_number <= 7)  ) {
        if (enable & (0x3F0)){
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_TOP_EIER_ADDR, &top_eier_rd));
            top_eier_wr = ((top_eier_rd & ~(0x1<<port_number)) | ((intr_enable & 0x1) << port_number)) ;
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_TOP_EIER_ADDR,  top_eier_wr));
        }
        if (enable & BARCHETTA_INTR_CL73_AN_COMPLETE) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_Px_A_EIER_BASE_ADDR + (6*port_number), &px_A_eier_rd));
            px_A_eier_wr = (px_A_eier_rd & ~(1 << (LIN_CL73_AN_COMPLETE_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF))));
            px_A_eier_wr |= ((intr_enable & 0x1) << (LIN_CL73_AN_COMPLETE_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF)));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_A_EIER_BASE_ADDR + (6*port_number),  px_A_eier_wr));
        }
        if (enable & BARCHETTA_INTR_CL73_AN_RESTARTED) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_Px_A_EIER_BASE_ADDR + (6*port_number), &px_A_eier_rd));
            px_A_eier_wr = (px_A_eier_rd & ~(1 << (LIN_CL73_AN_RESTARTED_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF))));
            px_A_eier_wr |= ((intr_enable & 0x1) << (LIN_CL73_AN_RESTARTED_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF)));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_A_EIER_BASE_ADDR + (6*port_number),  px_A_eier_wr));
        }
        if (enable & BARCHETTA_INTR_PMD_RX_SIGDET_FOUND) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR + (6*port_number), &px_B_eier_rd));
            px_B_eier_wr = (px_B_eier_rd & ~(1 << (LIN_PMD_RX_SIGDET_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF))));
            px_B_eier_wr |= ((intr_enable & 0x1) << (LIN_PMD_RX_SIGDET_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR + (6*port_number),  px_B_eier_wr));
        }
        if (enable & BARCHETTA_INTR_PMD_RX_SIGDET_LOST) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR + (6*port_number), &px_B_eier_rd));
            px_B_eier_wr = (px_B_eier_rd & ~(1 << (LIN_PMD_RX_SIGDET_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF))));
            px_B_eier_wr |= ((intr_enable & 0x1) << (LIN_PMD_RX_SIGDET_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR + (6*port_number),  px_B_eier_wr));
        }
        if (enable & BARCHETTA_INTR_PMD_RX_LOCK_FOUND) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR + (6*port_number), &px_B_eier_rd));
            px_B_eier_wr = (px_B_eier_rd & ~(1 << (LIN_PMD_RX_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF))));
            px_B_eier_wr |= ((intr_enable & 0x1) << (LIN_PMD_RX_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR + (6*port_number),  px_B_eier_wr));
        }
        if (enable & BARCHETTA_INTR_PMD_RX_LOCK_LOST) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR + (6*port_number), &px_B_eier_rd));
            px_B_eier_wr = (px_B_eier_rd & ~(1 << (LIN_PMD_RX_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF))));
            px_B_eier_wr |= ((intr_enable & 0x1) << (LIN_PMD_RX_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)));
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR + (6*port_number),  px_B_eier_wr));
        }
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("Invalid port number received")));
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_intr_enable_get(
        const plp_barchetta_phymod_phy_access_t* phy, uint32_t* enable) {
    barchetta_package_info_t pkg_info;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    int port_number = 0xFF;
    uint8_t   pll_idx = 0;
    uint8_t   sys_side = 0;
    uint32_t  m0_eier_rd  = 0 ;
    uint32_t  common_misc_eier_rd = 0 ;
    uint32_t  px_A_eier_rd = 0;
    uint32_t  px_B_eier_rd = 0;
    uint32_t  mst_m0_eier_rd = 0 ;
    uint32_t  pkg_lane_map = __plp_barchetta_convert_logical_lanemap_to_package_lanemap(pa);

    PHYMOD_MEMSET(&pkg_info , 0, sizeof(barchetta_package_info_t));

    if (BARCHETTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_pll_index(phy, &pll_idx));

    *enable = 0;

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_M0_EIER_ADDR, &m0_eier_rd));
    /* BARCHETTA_INTR_M0_MST_MISC */
    if(m0_eier_rd & M0_ENABLE_MST_MISC_INTR_MASK) {
        *enable |= BARCHETTA_INTR_M0_MST_MISC;
    }
    /* BARCHETTA_INTR_M0_MST_MSGOUT */
    if(m0_eier_rd & M0_ENABLE_MST_MSGOUT_INTR_MASK) {
        *enable |= BARCHETTA_INTR_M0_MST_MSGOUT;
    }

    /* PLL Interrupts */
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_COMMON_MISC_EIER_ADDR, &common_misc_eier_rd));
    if (common_misc_eier_rd & (1 << (LIN_PMD_PLL0_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx * 2)))) {
        *enable |= BARCHETTA_INTR_PLL_LOCK_FOUND ;
    }
    if (common_misc_eier_rd & (1 << (LIN_PMD_PLL0_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx * 2)))) {
        *enable |= BARCHETTA_INTR_PLL_LOCK_LOST ;
    }


    /* Module Absent Interrupt */
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_CTRL_MST_M0_EIER_ADDR, &mst_m0_eier_rd));
    if(pkg_lane_map & 0xF) {
        if(mst_m0_eier_rd & MOD_ABS_0_MASK) {
            *enable |= BARCHETTA_INTR_PMD_MOD_ABS ;
        }
    }
    if(pkg_lane_map & 0xF0) {
        if(mst_m0_eier_rd & MOD_ABS_1_MASK) {
            *enable |= BARCHETTA_INTR_PMD_MOD_ABS ;
        }
    }

    /* AN complete, AN restart, PMD Rx Signal Detect and PMD Rx lock Interrupt */
    if((port_number >= 0) && (port_number <= 7)) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa,
               (BARCHETTA_CTRL_Px_A_EIER_BASE_ADDR + (port_number << 2) + (port_number << 1)), &px_A_eier_rd));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa,
               (BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR + (port_number << 2) + (port_number << 1)), &px_B_eier_rd));

        if (px_A_eier_rd & (0x1 << (LIN_CL73_AN_COMPLETE_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF)))) {
            *enable |= BARCHETTA_INTR_CL73_AN_COMPLETE;
        }
        if (px_A_eier_rd & (0x1 << (LIN_CL73_AN_RESTARTED_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF)))) {
            *enable |= BARCHETTA_INTR_CL73_AN_RESTARTED;
        }
        if (px_B_eier_rd & (0x1 << (LIN_PMD_RX_SIGDET_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))) {
            *enable |= BARCHETTA_INTR_PMD_RX_SIGDET_FOUND;
        }
        if (px_B_eier_rd & (0x1 << (LIN_PMD_RX_SIGDET_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))) {
            *enable |= BARCHETTA_INTR_PMD_RX_SIGDET_LOST;
        }
        if (px_B_eier_rd & (0x1 << (LIN_PMD_RX_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))) {
            *enable |= BARCHETTA_INTR_PMD_RX_LOCK_FOUND;
        }
        if (px_B_eier_rd & (0x1 << (LIN_PMD_RX_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))) {
            *enable |= BARCHETTA_INTR_PMD_RX_LOCK_LOST;
        }
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("Invalid port number received")));
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_intr_status_get(
        const plp_barchetta_phymod_phy_access_t* phy, uint32_t* intr_status) {
    barchetta_package_info_t pkg_info;
    const plp_barchetta_phymod_access_t *pa = &phy->access;

    uint32_t event     = 0 ;
    uint32_t intr_type = 0 ;
    uint8_t pll_idx    = 0, sys_side = 0;
    int port_number    = 0 ;
    uint32_t  m0_sts_rd  = 0 ;
    uint32_t  common_misc_sts_rd = 0 ;
    uint32_t  px_A_sts_rd = 0;
    uint32_t  px_B_sts_rd = 0;
    uint32_t  mst_m0_sts_rd = 0;
    uint32_t  barchetta_EISR[BARCHETTA_MAX_INTR_REG] = {
                                                         BARCHETTA_CTRL_M0_EISR_ADDR,
                                                         BARCHETTA_CTRL_COMMON_MISC_EISR_ADDR,
                                                         BARCHETTA_CTRL_MST_M0_EISR_ADDR,
                                                         BARCHETTA_CTRL_Px_A_EISR_BASE_ADDR,
                                                         BARCHETTA_CTRL_Px_B_EISR_BASE_ADDR
                                                       };
    uint32_t  barchetta_EIPR[BARCHETTA_MAX_INTR_REG] = {
                                                         BARCHETTA_CTRL_M0_EIPR_ADDR,
                                                         BARCHETTA_CTRL_COMMON_MISC_EIPR_ADDR,
                                                         BARCHETTA_CTRL_MST_M0_EIPR_ADDR,
                                                         BARCHETTA_CTRL_Px_A_EIPR_BASE_ADDR,
                                                         BARCHETTA_CTRL_Px_B_EIPR_BASE_ADDR
                                                       };
    uint32_t *int_reg = NULL;

    uint32_t  pkg_lane_map = __plp_barchetta_convert_logical_lanemap_to_package_lanemap(pa);

    PHYMOD_MEMSET(&pkg_info , 0, sizeof(barchetta_package_info_t));

    if (BARCHETTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_pll_index(phy, &pll_idx));
    intr_type = (*intr_status) ? (*intr_status & 0x7FFFFFFF): 0xFFFF;
    event = (*intr_status & 0x80000000) ? 1: 0;
    *intr_status = 0;
    if(event) {
        int_reg = barchetta_EISR;
    } else {
        int_reg = barchetta_EIPR;
    }

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, int_reg[0], &m0_sts_rd));
    /* BARCHETTA_INTR_M0_MST_MISC */
    if (intr_type & BARCHETTA_INTR_M0_MST_MISC) {
        if(m0_sts_rd & M0_ENABLE_MST_MISC_INTR_MASK) {
            *intr_status |= BARCHETTA_INTR_M0_MST_MISC ;
        }
    }

    /* BARCHETTA_INTR_M0_MST_MSGOUT */
    if (intr_type & BARCHETTA_INTR_M0_MST_MSGOUT) {
        if (m0_sts_rd & M0_ENABLE_MST_MSGOUT_INTR_MASK) {
            *intr_status |= BARCHETTA_INTR_M0_MST_MSGOUT ;
        }
    }

    /* PLL Interrupts */
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, int_reg[1], &common_misc_sts_rd));
    if(intr_type & BARCHETTA_INTR_PLL_LOCK_FOUND) {
        if (common_misc_sts_rd & (1 << (LIN_PMD_PLL0_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx * 2)))) {
            *intr_status |= BARCHETTA_INTR_PLL_LOCK_FOUND ;
        }
    }
    if(intr_type & BARCHETTA_INTR_PLL_LOCK_LOST) {
        if (common_misc_sts_rd & (1 << (LIN_PMD_PLL0_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx * 2)))) {
            *intr_status |= BARCHETTA_INTR_PLL_LOCK_LOST ;
        }
    }

    /* Module Absent Interrupt */
    if (intr_type & BARCHETTA_INTR_PMD_MOD_ABS) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, int_reg[2], &mst_m0_sts_rd));
        if(pkg_lane_map & 0xF) {
            if((mst_m0_sts_rd & MOD_ABS_0_MASK)) {
                *intr_status |= BARCHETTA_INTR_PMD_MOD_ABS ;
            }
        }
        if(pkg_lane_map & 0xF0) {
            if((mst_m0_sts_rd & MOD_ABS_1_MASK)) {
                *intr_status |= BARCHETTA_INTR_PMD_MOD_ABS ;
            }
        }
    }

    /* AN complete, AN restart, PMD Rx Signal Detect and PMD Rx lock Interrupt */
    if((port_number >= 0) && (port_number <= 7)  ) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, (int_reg[3] + (6*port_number)), &px_A_sts_rd));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, (int_reg[4] + (6*port_number)), &px_B_sts_rd));
        if (intr_type & BARCHETTA_INTR_CL73_AN_COMPLETE) {
            if (px_A_sts_rd & (0x1 << (LIN_CL73_AN_COMPLETE_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF)))) {
                 *intr_status |= BARCHETTA_INTR_CL73_AN_COMPLETE;
            }
        }
        if (intr_type & BARCHETTA_INTR_CL73_AN_RESTARTED) {
            if (px_A_sts_rd & (0x1 << (LIN_CL73_AN_RESTARTED_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF)))) {
                *intr_status |= BARCHETTA_INTR_CL73_AN_RESTARTED;
            }
        }
        if (intr_type & BARCHETTA_INTR_PMD_RX_SIGDET_FOUND) {
            if (px_B_sts_rd & (0x1 << (LIN_PMD_RX_SIGDET_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))) {
                *intr_status |= BARCHETTA_INTR_PMD_RX_SIGDET_FOUND;
            }
        }
        if (intr_type & BARCHETTA_INTR_PMD_RX_SIGDET_LOST) {
            if (px_B_sts_rd & (0x1 << (LIN_PMD_RX_SIGDET_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))) {
                *intr_status |= BARCHETTA_INTR_PMD_RX_SIGDET_LOST;
            }
        }
        if (intr_type & BARCHETTA_INTR_PMD_RX_LOCK_FOUND) {
            if (px_B_sts_rd & (0x1 << (LIN_PMD_RX_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))) {
                *intr_status |= BARCHETTA_INTR_PMD_RX_LOCK_FOUND;
            }
        }
        if (intr_type & BARCHETTA_INTR_PMD_RX_LOCK_LOST) {
            if (px_B_sts_rd & (0x1 << (LIN_PMD_RX_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))) {
                *intr_status |= BARCHETTA_INTR_PMD_RX_LOCK_LOST;
            }
        }
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("Invalid port number received")));
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_intr_status_clear(
        const plp_barchetta_phymod_phy_access_t* phy, uint32_t intr_clr) {
    barchetta_package_info_t pkg_info;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    int port_number = 0xFF;
    uint8_t  pll_idx = 0;
    uint8_t  sys_side = 0 ;
    uint32_t pkg_lane_map = __plp_barchetta_convert_logical_lanemap_to_package_lanemap(pa);

    PHYMOD_MEMSET(&pkg_info , 0, sizeof(barchetta_package_info_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_number));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_pll_index(phy, &pll_idx));

    if (BARCHETTA_IS_LINE_SIDE(phy)) {
        sys_side = 0;
    } else {
        sys_side = 1;
    }

    /* BARCHETTA_INTR_M0_MST_MISC */
    if (intr_clr & BARCHETTA_INTR_M0_MST_MISC) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_M0_EISR_ADDR, (0x1 << M0_ENABLE_MST_MISC_INTR_BIT_POS)));
    }
    /* BARCHETTA_INTR_M0_MST_MSGOUT */
    if(intr_clr & BARCHETTA_INTR_M0_MST_MSGOUT) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_M0_EISR_ADDR,  (0x1 << M0_ENABLE_MST_MSGOUT_INTR_BIT_POS)));
    }

    /* PLL Interrupts */
    if(intr_clr & BARCHETTA_INTR_PLL_LOCK_FOUND) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_COMMON_MISC_EISR_ADDR,
                (0x1 << (LIN_PMD_PLL0_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx*2)))));
    }
    if (intr_clr & BARCHETTA_INTR_PLL_LOCK_LOST) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_COMMON_MISC_EISR_ADDR,
                (0x1 << (LIN_PMD_PLL0_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PLL_DIFF) + (pll_idx*2)))));
    }

    /* Module Absent Interrupt */
    if (intr_clr & BARCHETTA_INTR_PMD_MOD_ABS) {
        if(pkg_lane_map & 0xF) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_MST_M0_EISR_ADDR, (0x1 << MOD_ABS_0_BIT_POS)));
        }
        if(pkg_lane_map & 0xF0) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_MST_M0_EISR_ADDR, (0x1 << MOD_ABS_1_BIT_POS)));
        }
    }

    /* AN complete, AN restart, PMD Rx Signal Detect and PMD Rx lock Interrupt */
    if((port_number >= 0) && (port_number <= 7)  ) {
        if (intr_clr & BARCHETTA_INTR_CL73_AN_COMPLETE) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa,BARCHETTA_CTRL_Px_A_EISR_BASE_ADDR + (6*port_number),
                   (0x1 << (LIN_CL73_AN_COMPLETE_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF)))));
        }
        if (intr_clr & BARCHETTA_INTR_CL73_AN_RESTARTED) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_A_EISR_BASE_ADDR + (6*port_number),
                   (0x1 << (LIN_CL73_AN_RESTARTED_BIT_POS + (sys_side * LIN_SYS_INT_AN_DIFF)))));
        }
        if (intr_clr & BARCHETTA_INTR_PMD_RX_SIGDET_FOUND) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_B_EISR_BASE_ADDR + (6*port_number),
                   (0x1 << (LIN_PMD_RX_SIGDET_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))));
        }
        if (intr_clr & BARCHETTA_INTR_PMD_RX_SIGDET_LOST) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_B_EISR_BASE_ADDR + (6*port_number),
                   (0x1 << (LIN_PMD_RX_SIGDET_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))));
        }
        if (intr_clr & BARCHETTA_INTR_PMD_RX_LOCK_FOUND) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_B_EISR_BASE_ADDR + (6*port_number),
                   (0x1 << (LIN_PMD_RX_LOCK_FOUND_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))));
        }
        if (intr_clr & BARCHETTA_INTR_PMD_RX_LOCK_LOST) {
            PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_CTRL_Px_B_EISR_BASE_ADDR + (6*port_number),
                   (0x1 << (LIN_PMD_RX_LOCK_LOST_BIT_POS + (sys_side * LIN_SYS_INT_PMD_DIFF)))));
        }
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("Invalid port number received")));
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_gpio_config_set(
        const plp_barchetta_phymod_phy_access_t* phy, int pin_no, plp_barchetta_phymod_gpio_mode_t gpio_mode) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t  pad_ctrl_gpio_x_ctrl_rd = 0;
    uint32_t  pad_ctrl_gpio_x_ctrl_wr = 0;
    uint16_t data = 0;

    switch (gpio_mode) {
        case phymodGpioModeDisabled:
            return PHYMOD_E_NONE;
        case phymodGpioModeOutput:
            data = 0;
        break;
        case phymodGpioModeInput:
            data = 1;
        break;
        default:
             PHYMOD_DEBUG_ERROR(("ERROR: Invalid gpio mode\n"));
          return PHYMOD_E_PARAM;
    }

    if((pin_no >= 0) && (pin_no <= BARCHETTA_MAX_GPIO_PIN)) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_PAD_CTRL_GPIO_X_CTRL_BASE_ADDR + (pin_no << 1), &pad_ctrl_gpio_x_ctrl_rd));
        pad_ctrl_gpio_x_ctrl_wr = ((pad_ctrl_gpio_x_ctrl_rd & ~GPIO_X_CTRL_GPIO_X_OEBF_MASK) | (data << GPIO_X_CTRL_GPIO_X_OEBF_BIT_POS));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_PAD_CTRL_GPIO_X_CTRL_BASE_ADDR + (pin_no << 1),  pad_ctrl_gpio_x_ctrl_wr));
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }

    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_gpio_config_get(
       const plp_barchetta_phymod_phy_access_t* phy, int pin_no, plp_barchetta_phymod_gpio_mode_t* gpio_mode) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t  pad_ctrl_gpio_x_ctrl_rd = 0;
    uint16_t  data = 0;

    if((pin_no >= 0) && (pin_no <= BARCHETTA_MAX_GPIO_PIN)) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_PAD_CTRL_GPIO_X_CTRL_BASE_ADDR + (pin_no << 1), &pad_ctrl_gpio_x_ctrl_rd));
        data = (pad_ctrl_gpio_x_ctrl_rd & GPIO_X_CTRL_GPIO_X_OEBF_MASK) >> GPIO_X_CTRL_GPIO_X_OEBF_BIT_POS ;
        if(data) {
            *gpio_mode = phymodGpioModeInput ;
        } else {
            *gpio_mode = phymodGpioModeOutput ;
        }
    } else {
        *gpio_mode = phymodGpioModeDisabled ;
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_gpio_pin_value_set(
        const plp_barchetta_phymod_phy_access_t* phy, int pin_no, int value) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t  pad_ctrl_gpio_x_ctrl_rd = 0;
    uint32_t  pad_ctrl_gpio_x_ctrl_wr = 0;
    uint16_t  pull_up_dwn_data = ((value >> 0x1)& 0x1) ? 0x1 /* PULL UP */ : 0x2 /* PULL DOWN */ ;

    if((pin_no >= 0) && (pin_no <= BARCHETTA_MAX_GPIO_PIN)) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_PAD_CTRL_GPIO_X_CTRL_BASE_ADDR + (pin_no << 1), &pad_ctrl_gpio_x_ctrl_rd));
        pad_ctrl_gpio_x_ctrl_wr = ((pad_ctrl_gpio_x_ctrl_rd & ~GPIO_X_CTRL_GPIO_X_IBOF_MASK) | (0x1 << GPIO_X_CTRL_GPIO_X_IBOF_BIT_POS));
        pad_ctrl_gpio_x_ctrl_wr = ((pad_ctrl_gpio_x_ctrl_wr & ~GPIO_X_CTRL_GPIO_X_PULL_UP_DOWN_MASK) | (pull_up_dwn_data << GPIO_X_CTRL_GPIO_X_PULL_UP_DOWN_BIT_POS));
        pad_ctrl_gpio_x_ctrl_wr = ((pad_ctrl_gpio_x_ctrl_wr & ~GPIO_X_CTRL_GPIO_X_OUT_FRCVAL_MASK) | (((value & 0x1)?0x1:0x0) << GPIO_X_CTRL_GPIO_X_OUT_FRCVAL_BIT_POS));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, BARCHETTA_PAD_CTRL_GPIO_X_CTRL_BASE_ADDR + (pin_no << 1),  pad_ctrl_gpio_x_ctrl_wr));
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_gpio_pin_value_get(
        const plp_barchetta_phymod_phy_access_t* phy, int pin_no, int* value) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t pad_ctrl_gpio_0_sts_rd = 0;
    uint32_t pad_ctrl_gpio_x_ctrl_rd = 0;
    uint16_t pull_up_dwn_data = 0 ;
    uint16_t cfg_value = 0;
    if (pin_no > BARCHETTA_MAX_GPIO_PIN) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }

    if((pin_no >= 0) && (pin_no <= BARCHETTA_MAX_GPIO_PIN)) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_PAD_CTRL_GPIO_X_CTRL_BASE_ADDR + (pin_no << 1), &pad_ctrl_gpio_x_ctrl_rd));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(pa, BARCHETTA_PAD_CTRL_GPIO_X_STATUS_BASE_ADDR + (pin_no << 1), &pad_ctrl_gpio_0_sts_rd));
        pull_up_dwn_data = (((pad_ctrl_gpio_x_ctrl_rd & GPIO_X_CTRL_GPIO_X_PULL_UP_DOWN_MASK) >> GPIO_X_CTRL_GPIO_X_PULL_UP_DOWN_BIT_POS) & 0x03 ) ;
        if(pull_up_dwn_data == 0x0) {
            cfg_value = 0x2 ;
        } else if (pull_up_dwn_data == 0x1) {
            cfg_value = 0x1 ;
        } else if (pull_up_dwn_data == 0x2) {
            cfg_value = 0x0 ;
        }
        *value = (pad_ctrl_gpio_0_sts_rd & GPIO_X_STATUS_GPIO_X_DIN_MASK) >> GPIO_X_STATUS_GPIO_X_DIN_BIT_POS ;
        *value |= cfg_value << 1 ;
    } else {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_CONFIG, (_PHYMOD_MSG("Invalid GPIO pin selected")));
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
static int
__plp_barchetta_set_module_command(
         const plp_barchetta_phymod_access_t *pa, uint16_t xfer_addr,
         uint32_t slv_addr, unsigned char xfer_cnt, barchetta_i2c_module_cmd_t cmd) {
    BCMI_BARCHETTA_MODULE_CNTRL_CONTROLr_t      mod_ctrl;
    BCMI_BARCHETTA_MODULE_CNTRL_ADDRESSr_t      mod_add;
    BCMI_BARCHETTA_MODULE_CNTRL_STATUSr_t       mod_ctrl_sts;
    BCMI_BARCHETTA_MODULE_CNTRL_XFER_COUNTr_t   mod_xfer_cnt;
    BCMI_BARCHETTA_MODULE_CNTRL_XFER_ADDRESSr_t mod_xfer_add;

    uint16_t retry_count = 500, data = 0;
    uint32_t wait_timeout_us = 0;
    wait_timeout_us = ((2*(xfer_cnt+1))*100)/5;

    PHYMOD_MEMSET(&mod_ctrl_sts, 0, sizeof(BCMI_BARCHETTA_MODULE_CNTRL_STATUSr_t));
    PHYMOD_MEMSET(&mod_ctrl,     0, sizeof(BCMI_BARCHETTA_MODULE_CNTRL_CONTROLr_t));
    PHYMOD_MEMSET(&mod_add,      0, sizeof(BCMI_BARCHETTA_MODULE_CNTRL_ADDRESSr_t));
    PHYMOD_MEMSET(&mod_xfer_cnt, 0, sizeof(BCMI_BARCHETTA_MODULE_CNTRL_XFER_COUNTr_t));
    PHYMOD_MEMSET(&mod_xfer_add, 0, sizeof(BCMI_BARCHETTA_MODULE_CNTRL_XFER_ADDRESSr_t));

    if (cmd == BARCHETTA_FLUSH) {
        BCMI_BARCHETTA_MODULE_CNTRL_CONTROLr_SET(mod_ctrl,0xC000);
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_CONTROLr(pa, mod_ctrl));
    } else {
        BCMI_BARCHETTA_MODULE_CNTRL_XFER_ADDRESSr_SET(mod_xfer_add, xfer_addr);
        BCMI_BARCHETTA_MODULE_CNTRL_XFER_COUNTr_SET(mod_xfer_cnt, xfer_cnt);
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_XFER_ADDRESSr(pa, mod_xfer_add));
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_XFER_COUNTr(pa, mod_xfer_cnt));
        if (cmd == BARCHETTA_CURRENT_ADDRESS_READ) {
            BCMI_BARCHETTA_MODULE_CNTRL_CONTROLr_SET(mod_ctrl, 0x8001);
            PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_CONTROLr(pa, mod_ctrl));
        } else if (cmd == BARCHETTA_RANDOM_ADDRESS_READ ) {
            BCMI_BARCHETTA_MODULE_CNTRL_ADDRESSr_SET(mod_add, slv_addr);
            BCMI_BARCHETTA_MODULE_CNTRL_CONTROLr_SET(mod_ctrl,0x8003);
            PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_ADDRESSr(pa, mod_add));
            PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_CONTROLr(pa, mod_ctrl));
        } else {
            BCMI_BARCHETTA_MODULE_CNTRL_ADDRESSr_SET(mod_add, slv_addr);
            BCMI_BARCHETTA_MODULE_CNTRL_CONTROLr_SET(mod_ctrl, 0x8022);
            PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_ADDRESSr(pa, mod_add));
            PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_CONTROLr(pa, mod_ctrl));
        }
    }

    if ((cmd == BARCHETTA_CURRENT_ADDRESS_READ) ||(cmd == BARCHETTA_RANDOM_ADDRESS_READ) || (cmd == BARCHETTA_I2C_WRITE)) {
        do {
            PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_MODULE_CNTRL_STATUSr(pa, &mod_ctrl_sts));
            data = BCMI_BARCHETTA_MODULE_CNTRL_STATUSr_XACTION_DONEf_GET(mod_ctrl_sts);
            PHYMOD_USLEEP(wait_timeout_us);
        } while((data == 0) && --retry_count);
        if(!retry_count) {
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_TIMEOUT, (_PHYMOD_MSG("Module controller: I2C transaction failed..")));
        }
    }
    BCMI_BARCHETTA_MODULE_CNTRL_CONTROLr_SET(mod_ctrl, 0x3);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_CONTROLr(pa, mod_ctrl));

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_i2c_read(
        const plp_barchetta_phymod_phy_access_t* phy, uint32_t slv_dev_addr,
        uint32_t start_addr, uint32_t no_of_bytes, uint8_t* read_data) {
    BCMI_BARCHETTA_MODULE_CNTRL_DEV_IDr_t             dev_id ;
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_t         gen_ctrl1;
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_t         gen_ctrl3;
    BCMI_BARCHETTA_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_t  mod_ram_mdio_ctrl ;
    BCMI_BARCHETTA_PAD_CNTRL_GPIO_10_STATUSr_t        gpio_10_status    ;
    BCMI_BARCHETTA_PAD_CNTRL_GPIO_11_STATUSr_t        gpio_11_status    ;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t lower_page_start_addr = 0;
    uint32_t upper_page_start_addr = 0;
    uint32_t lower_page_bytes = 0;
    uint32_t upper_page_bytes = 0;
    uint32_t lower_page_flag  = 0;
    uint32_t upper_page_flag  = 0;
    uint16_t START_OF_NVRAM   = 0;
    uint32_t index     = 0;
    uint32_t rd_data   = 0;
    uint32_t pkg_lane_map = __plp_barchetta_convert_logical_lanemap_to_package_lanemap(pa);

    PHYMOD_MEMSET(&dev_id,            0, sizeof(BCMI_BARCHETTA_MODULE_CNTRL_DEV_IDr_t));
    PHYMOD_MEMSET(&gen_ctrl1,         0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_t));
    PHYMOD_MEMSET(&gen_ctrl3,         0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_t));
    PHYMOD_MEMSET(&mod_ram_mdio_ctrl, 0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_t));
    PHYMOD_MEMSET(&gpio_10_status,    0, sizeof(BCMI_BARCHETTA_PAD_CNTRL_GPIO_10_STATUSr_t));
    PHYMOD_MEMSET(&gpio_11_status,    0, sizeof(BCMI_BARCHETTA_PAD_CNTRL_GPIO_11_STATUSr_t));

    /* In case of module absent, return the error */
    if(pkg_lane_map & 0xF) {
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_PAD_CNTRL_GPIO_10_STATUSr(pa, &gpio_10_status));
        if(BCMI_BARCHETTA_PAD_CNTRL_GPIO_10_STATUSr_GPIO_10_DINf_GET(gpio_10_status)) {
            PHYMOD_DEBUG_ERROR(("Error : Module 0 absent\n"));
            return PHYMOD_E_INTERNAL;
        }
    } else if(pkg_lane_map & 0xF0) {
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_PAD_CNTRL_GPIO_11_STATUSr(pa, &gpio_11_status));
        if(BCMI_BARCHETTA_PAD_CNTRL_GPIO_11_STATUSr_GPIO_11_DINf_GET(gpio_11_status)) {
            PHYMOD_DEBUG_ERROR(("Error : Module 1 absent\n"));
            return PHYMOD_E_INTERNAL;
        }
    }

    /* qsfp mode or legacy mode */
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL3r(pa, &gen_ctrl3));
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_QSFP_MODEf_SET(gen_ctrl3,1);
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_UCSPI_SLOWf_SET(gen_ctrl3,0);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL3r(pa, gen_ctrl3));

    /* qsfp reset at beginning */
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL1r(pa, &gen_ctrl1));
    gen_ctrl1.v[0] &= ~(1 << 6);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL1r(pa, gen_ctrl1));
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL1r(pa, &gen_ctrl1));
    gen_ctrl1.v[0] |= (1 << 6);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL1r(pa, gen_ctrl1));

    /* Select QSFP module and associated NVRAM */
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL3r(pa, &gen_ctrl3));
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(pa, &mod_ram_mdio_ctrl));
    if (pkg_lane_map & 0xF) { /* Select QSFP module 0 */
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_EXTMOD_SELECTf_SET(gen_ctrl3, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_MD_EXTMOD_SELECTf_SET(mod_ram_mdio_ctrl, 0);
    } else if (pkg_lane_map & 0xF0) { /* Select QSFP module 1 */
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_EXTMOD_SELECTf_SET(gen_ctrl3, 1);
        BCMI_BARCHETTA_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_MD_EXTMOD_SELECTf_SET(mod_ram_mdio_ctrl, 1);
    }
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL3r(pa, gen_ctrl3));
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(pa, mod_ram_mdio_ctrl));

    /* Configure the slave device ID default is 0x50 */
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_MODULE_CNTRL_DEV_IDr(pa, &dev_id));
    BCMI_BARCHETTA_MODULE_CNTRL_DEV_IDr_SL_DEV_ADDf_SET(dev_id, slv_dev_addr);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_DEV_IDr(pa, dev_id));

    if(no_of_bytes == 0) {
        /* Perform module controller reset and FLUSH */
        PHYMOD_IF_ERR_RETURN(__plp_barchetta_set_module_command(pa, 0, 0, 0, BARCHETTA_FLUSH));
    }

    if ((no_of_bytes + start_addr) >= 256) {
        no_of_bytes = 255 - start_addr + 1;
    }

    /* To determine page to be written is lower page or upper page or
     * both lower and upper page
     */
    if ((start_addr+no_of_bytes - 1) > 127) {
        /* lower page */
        if (start_addr <= 127) {
            lower_page_bytes = 127 - start_addr + 1;
            lower_page_flag = 1;
            lower_page_start_addr = start_addr;
        }
        /* upper page */
        if ((start_addr + no_of_bytes) > 127) {
            upper_page_flag = 1;
            upper_page_bytes = no_of_bytes - lower_page_bytes;
            if(start_addr > 128) {
                upper_page_start_addr = start_addr;
            } else {
                upper_page_start_addr = 128;
            }
        }
    } else { /* only lower page */
        lower_page_bytes = no_of_bytes;
        lower_page_flag = 1;
        lower_page_start_addr = start_addr;
    }

    if (lower_page_flag) {
        PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_set_module_command(pa, 0, 0, 0, BARCHETTA_FLUSH));
        PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_set_module_command(pa,
                        (BARCHETTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + lower_page_start_addr),
                         lower_page_start_addr, lower_page_bytes - 1, BARCHETTA_RANDOM_ADDRESS_READ));
        lower_page_flag = 0;
    }

    /* Need to check with chip team how we can read upper page */
    if (upper_page_flag) {
        PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_set_module_command(pa, 0, 0, 0, BARCHETTA_FLUSH));
        PHYMOD_IF_ERR_RETURN(
                __plp_barchetta_set_module_command(pa,
                        (BARCHETTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + upper_page_start_addr),
                         upper_page_start_addr, upper_page_bytes - 1, BARCHETTA_RANDOM_ADDRESS_READ));
        upper_page_flag = 0;
    }

   /* Read data from NVRAM using I2C */
    for (index = 0; index < no_of_bytes; index++) {
        PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_read(pa, (0x10000 + BARCHETTA_MODULE_CNTRL_RAM_NVR0_ADR + start_addr + index),  &rd_data));
       read_data[index] = (unsigned char) (rd_data & 0xff);
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_phy_i2c_write(
        const plp_barchetta_phymod_phy_access_t* phy, uint32_t slv_dev_addr,
        uint32_t start_addr, uint32_t no_of_bytes, const uint8_t* write_data) {
    BCMI_BARCHETTA_MODULE_CNTRL_DEV_IDr_t             dev_id ;
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_t         gen_ctrl1;
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_t         gen_ctrl3;
    BCMI_BARCHETTA_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_t  mod_ram_mdio_ctrl ;
    BCMI_BARCHETTA_PAD_CNTRL_GPIO_10_STATUSr_t        gpio_10_status    ;
    BCMI_BARCHETTA_PAD_CNTRL_GPIO_11_STATUSr_t        gpio_11_status    ;

    uint32_t lower_page_start_addr = 0;
    uint32_t upper_page_start_addr = 0;
    uint32_t lower_page_bytes = 0;
    uint32_t upper_page_bytes = 0;
    uint32_t lower_page_flag  = 0;
    uint32_t upper_page_flag  = 0;
    uint16_t START_OF_NVRAM   = 0;
    uint32_t index     = 0;
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t pkg_lane_map = __plp_barchetta_convert_logical_lanemap_to_package_lanemap(pa);

    PHYMOD_MEMSET(&dev_id,            0, sizeof(BCMI_BARCHETTA_MODULE_CNTRL_DEV_IDr_t));
    PHYMOD_MEMSET(&gen_ctrl1,         0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL1r_t));
    PHYMOD_MEMSET(&gen_ctrl3,         0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_t));
    PHYMOD_MEMSET(&mod_ram_mdio_ctrl, 0, sizeof(BCMI_BARCHETTA_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_t));
    PHYMOD_MEMSET(&gpio_10_status,    0, sizeof(BCMI_BARCHETTA_PAD_CNTRL_GPIO_10_STATUSr_t));
    PHYMOD_MEMSET(&gpio_11_status,    0, sizeof(BCMI_BARCHETTA_PAD_CNTRL_GPIO_11_STATUSr_t));

    /* In case of module absent, return the error */
    if(pkg_lane_map & 0xF) {
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_PAD_CNTRL_GPIO_10_STATUSr(pa, &gpio_10_status));
        if(BCMI_BARCHETTA_PAD_CNTRL_GPIO_10_STATUSr_GPIO_10_DINf_GET(gpio_10_status)) {
            PHYMOD_DEBUG_ERROR(("Error : Module 0 absent\n"));
            return PHYMOD_E_INTERNAL;
        }
    } else if(pkg_lane_map & 0xF0) {
        PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_PAD_CNTRL_GPIO_11_STATUSr(pa, &gpio_11_status));
        if(BCMI_BARCHETTA_PAD_CNTRL_GPIO_11_STATUSr_GPIO_11_DINf_GET(gpio_11_status)) {
            PHYMOD_DEBUG_ERROR(("Error : Module 1 absent\n"));
            return PHYMOD_E_INTERNAL;
        }
    }

    /* qsfp mode or legacy mode */
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL3r(pa, &gen_ctrl3));
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_QSFP_MODEf_SET(gen_ctrl3,1);
    BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_UCSPI_SLOWf_SET(gen_ctrl3,0);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL3r(pa, gen_ctrl3));

    /* qsfp reset at beginning */
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL1r(pa, &gen_ctrl1));
    gen_ctrl1.v[0] &= ~(1 << 6);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL1r(pa, gen_ctrl1));
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL1r(pa, &gen_ctrl1));
    gen_ctrl1.v[0] |= (1 << 6);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL1r(pa, gen_ctrl1));

    /* Select QSFP module and associated NVRAM */
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_GEN_CONTROL3r(pa, &gen_ctrl3));
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(pa, &mod_ram_mdio_ctrl));
    if (pkg_lane_map & 0xF) { /* Select QSFP module 0 */
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_EXTMOD_SELECTf_SET(gen_ctrl3, 0);
        BCMI_BARCHETTA_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_MD_EXTMOD_SELECTf_SET(mod_ram_mdio_ctrl, 0);
    } else if (pkg_lane_map & 0xF0) { /* Select QSFP module 1 */
        BCMI_BARCHETTA_GEN_CNTRLS_GEN_CONTROL3r_EXTMOD_SELECTf_SET(gen_ctrl3,1);
        BCMI_BARCHETTA_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr_MD_EXTMOD_SELECTf_SET(mod_ram_mdio_ctrl, 1);
    }
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_GEN_CONTROL3r(pa, gen_ctrl3));
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_GEN_CNTRLS_MCTRL_RAM_MDIO_CTRLr(pa, mod_ram_mdio_ctrl));

    /* Configure the slave device ID default is 0x50 */
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_MODULE_CNTRL_DEV_IDr(pa, &dev_id));
    BCMI_BARCHETTA_MODULE_CNTRL_DEV_IDr_SL_DEV_ADDf_SET(dev_id, slv_dev_addr);
    PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_MODULE_CNTRL_DEV_IDr(pa, dev_id));

    if(no_of_bytes == 0) {
        /* Perform module controller reset and FLUSH */
        PHYMOD_IF_ERR_RETURN(__plp_barchetta_set_module_command(pa, 0, 0, 0, BARCHETTA_FLUSH));
        return PHYMOD_E_NONE;
    }

    /* if requested number of bytes are not within the boundary (0- 255)
     * need to calculate what maximum number of bytes can be taken into
     * account for reading or writing to module
     */
    if ((no_of_bytes + start_addr) >= 256) {
        no_of_bytes = 255 - start_addr + 1;
    }

    /* To determine page to be written is lower page or upper page or
     * both lower and upper page
     */
    if ((start_addr + no_of_bytes - 1) > 127) {
        /* lower page */
        if (start_addr <= 127) {
            lower_page_bytes = 127 - start_addr + 1;
            lower_page_flag  = 1;
            lower_page_start_addr = start_addr;
        }
        /* upper page */
        if ((start_addr + no_of_bytes) > 127) {
            upper_page_flag = 1;
            upper_page_bytes = no_of_bytes - lower_page_bytes;
            if(start_addr > 128) {
                upper_page_start_addr = start_addr;
            } else {
                upper_page_start_addr = 128;
            }
        }
    } else { /* only lower page */
        lower_page_bytes = no_of_bytes;
        lower_page_flag  = 1;
        lower_page_start_addr = start_addr;
    }

    /* Write data to NVRAM */
    for (index = 0; index < no_of_bytes; index++) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write( pa,
                                                   ( 0x10000 + BARCHETTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + start_addr + index),
                                                     write_data[index]));
    }

    if(lower_page_flag) {
        for (index = 0; index < (lower_page_bytes / 4); index ++) {
            PHYMOD_IF_ERR_RETURN(__plp_barchetta_set_module_command( pa,
                                                                 BARCHETTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + lower_page_start_addr + (4 * index),
                                                                 lower_page_start_addr + (4 * index), 3, BARCHETTA_I2C_WRITE));
        }
        if ((lower_page_bytes % 4) > 0) {
            PHYMOD_IF_ERR_RETURN(__plp_barchetta_set_module_command( pa, BARCHETTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + lower_page_start_addr + (4 * index),
                                                                 lower_page_start_addr + (4 * index), ((lower_page_bytes % 4) - 1), BARCHETTA_I2C_WRITE));
        }
        lower_page_flag = 0;
    }

    if(upper_page_flag) {
        for (index = 0; index < (upper_page_bytes / 4); index++) {
            PHYMOD_IF_ERR_RETURN(__plp_barchetta_set_module_command( pa, (BARCHETTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + upper_page_start_addr + (4 * index)),
                                                                 upper_page_start_addr + (4 * index), 3, BARCHETTA_I2C_WRITE));
        }
        if ((upper_page_bytes%4) > 0) {
            PHYMOD_IF_ERR_RETURN(__plp_barchetta_set_module_command( pa, (BARCHETTA_MODULE_CNTRL_RAM_NVR0_ADR + START_OF_NVRAM + upper_page_start_addr + (4 * index)),
                                                                 upper_page_start_addr + (4 * index), ((upper_page_bytes % 4) - 1), BARCHETTA_I2C_WRITE));
        }
        upper_page_flag = 0;
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_failover_mode_set(
    const plp_barchetta_phymod_phy_access_t* phy, unsigned int failover_mode) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    barchetta_port_config_t port_cfg_rd;
    uint8_t port_mode = 0 ;
    int port_num = 0;

    PHYMOD_MEMSET(&pkg_info,    0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&port_cfg_rd, 0, sizeof(barchetta_port_config_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    PHYMOD_IF_ERR_RETURN(
          _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_num));
    if (port_num == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_msg_config_port_rd(pa, port_num, &port_cfg_rd));

    port_mode = port_cfg_rd.port_mode ;

    /* If the port_mode is in FAILOVER and failover_mode is set, then start switching to fail over port */
    if ((port_mode == BARCHETTA_PORT_MODE_FAILOVER) && (failover_mode == 1)) {
        /* Start the process of switching to fail over port */
        if(BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_switch_mux_port_start(pa, port_num)) {
            return (BARCHETTA_MSG_IF_RET_ERROR);
        }
        /* wait until the process of switching to Fail over port */
        if(BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_switch_mux_port_start_result(pa, port_num)) {
            return (BARCHETTA_MSG_IF_RET_ERROR);
        }
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: Switch over between primary to secondary (or secondary to primary) port not possible since its not a fail over port..\n"));
        return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
int
_plp_barchetta_failover_mode_get(
    const plp_barchetta_phymod_phy_access_t* phy, unsigned int *failover_mode) {
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    barchetta_port_config_t port_cfg_rd;
    int port_num = 0;

    PHYMOD_MEMSET(&pkg_info,    0, sizeof(barchetta_package_info_t));
    PHYMOD_MEMSET(&port_cfg_rd, 0, sizeof(barchetta_port_config_t));

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    PHYMOD_IF_ERR_RETURN(
          _plp_barchetta_retrieve_hardware_port_number_from_sw_database(phy, pkg_info, &port_num));
    if (port_num == 0xFF) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid port number 0xFF\n"));
        return PHYMOD_E_PARAM;
    }

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_msg_config_port_rd(pa, port_num, &port_cfg_rd));

    *failover_mode = port_cfg_rd.port_mode ;

    return PHYMOD_E_NONE ;
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
*******************************************************************************/
uint8_t
_plp_barchetta_is_pam4_mode(const plp_barchetta_phymod_phy_access_t* phy) {
    uint32_t lane_cfg = 0;
    uint8_t pam4_mode = 0 ;
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_read(&phy->access, 0x1d1ad, &lane_cfg));
    if(lane_cfg & 0x4000) {
        pam4_mode = 1 ;
    }
    return(pam4_mode);
}

/*******************************************************************************
 PURPOSE: To reset SW database, allocated/unallocated port list and used GPREG

 COMMENT:
 *******************************************************************************/
int
_plp_barchetta_phy_init(const plp_barchetta_phymod_phy_access_t* phy, const plp_barchetta_phymod_phy_init_config_t* init_config)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    uint32_t offset = 0;
    int chip_id = 0;

    BCMI_BARCHETTA_CTRL_GPIO_INPUT_SEL_4r_t gpio_input_sel_4;

    PHYMOD_MEMSET(&gpio_input_sel_4,  0, sizeof(BCMI_BARCHETTA_CTRL_GPIO_INPUT_SEL_4r_t));
    PHYMOD_MEMSET(&plp_barchetta_sw_db[pa->addr], 0x00, sizeof(barchetta_sw_db_t)*BARCHETTA_NUM_OF_PORTS);

    PHYMOD_MEMSET(&plp_barchetta_allocated_port_list[pa->addr], 0xFF, sizeof(uint8_t)*BARCHETTA_NUM_OF_PORTS);

    plp_barchetta_unallocated_port_list[pa->addr][0] = 0x0;
    plp_barchetta_unallocated_port_list[pa->addr][1] = 0x1;
    plp_barchetta_unallocated_port_list[pa->addr][2] = 0x2;
    plp_barchetta_unallocated_port_list[pa->addr][3] = 0x3;
    plp_barchetta_unallocated_port_list[pa->addr][4] = 0x4;
    plp_barchetta_unallocated_port_list[pa->addr][5] = 0x5;
    plp_barchetta_unallocated_port_list[pa->addr][6] = 0x6;
    plp_barchetta_unallocated_port_list[pa->addr][7] = 0x7;

    /* Zero initialization of registers from address 0x1b000 to 0x1b007 */
    /* Zero initialization of registers from address 0x18590 to 0x18597 */
    for(offset=0; offset<= 0x7; offset++) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, (BARCHETTA_LANE_DATA_RATE_SAVE_GPREG_BASE_ADDR + offset),  0x0));
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, (BCMI_BARCHETTA_CTRL_SWGPREG0r + offset),  0x0));
    }
    /* Zero initialization of registers from address 0x18598 to 0x1859F */
    for(offset=0x8; offset<= 0xF; offset++) {
        PHYMOD_IF_ERR_RETURN(_plp_barchetta_reg_write(pa, (BCMI_BARCHETTA_CTRL_SWGPREG0r + offset),  0x0));
    }

    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_chip_id(&phy->access, &chip_id));
    if (chip_id == BARCHETTA_CHIP_81381) {
        if(BARCHETTA_IS_LINE_SIDE(phy)) {
            /* GPIO configuration for detecting module absent signal*/
            PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_READ_CTRL_GPIO_INPUT_SEL_4r(pa, &gpio_input_sel_4));
            BCMI_BARCHETTA_CTRL_GPIO_INPUT_SEL_4r_MOD_ABS_0_SEL_IP_GPIOf_SET(gpio_input_sel_4, BARCHETTA_MOD_ABS_0_GPIO_PIN) ;
            BCMI_BARCHETTA_CTRL_GPIO_INPUT_SEL_4r_MOD_ABS_1_SEL_IP_GPIOf_SET(gpio_input_sel_4, BARCHETTA_MOD_ABS_1_GPIO_PIN) ;
            PHYMOD_IF_ERR_RETURN(BCMI_BARCHETTA_WRITE_CTRL_GPIO_INPUT_SEL_4r(pa, gpio_input_sel_4));
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: Program pll_ctrl[63:60], AMS_PLL_COM_PLL_CONTROL_3, 0xD113[15:12] appropriately
 for both PLL0 and PLL1. Programming only on one side is needed for BCM81381, while both
 sides are needed for BCM81338. This function gets invoked 2 times for 81338 to program both
 SYS and LINE side separately.
 COMMENT:
 *******************************************************************************/
int
_plp_barchetta_synce_update_rx_clk(const plp_barchetta_phymod_phy_access_t* phy, uint8_t pll_0_pll_ctrl, uint8_t pll_1_pll_ctrl)
{
    uint32_t ams_pll_ctrl3 = 0, ams_pll_intctrl = 0;

    /* Set pll_index to read/write PLL0 registers */
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice_pll_index(phy, BARCHETTA_PLL_INDEX_0));
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_INTCTRL, &ams_pll_intctrl));
    /* Power on PLL0 if its not powered up.*/
    if (ams_pll_intctrl & BARCHETTA_PLL_INTCTRL_MASK) {
        ams_pll_intctrl &= ~BARCHETTA_PLL_INTCTRL_MASK;
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_INTCTRL, ams_pll_intctrl));
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_CONTROL_3, &ams_pll_ctrl3));
    ams_pll_ctrl3 = ((ams_pll_ctrl3 & ~BARCHETTA_PLL_CTRL_MASK) | (pll_0_pll_ctrl << BARCHETTA_PLL_CTRL_OFFSET));
    PHYMOD_IF_ERR_RETURN(
         _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_CONTROL_3, ams_pll_ctrl3));

   /* Set pll_index to read/write PLL1 registers */
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice_pll_index(phy, BARCHETTA_PLL_INDEX_1));
    ams_pll_intctrl = 0;
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_INTCTRL, &ams_pll_intctrl));
    if (ams_pll_intctrl & BARCHETTA_PLL_INTCTRL_MASK) {
        ams_pll_intctrl &= ~BARCHETTA_PLL_INTCTRL_MASK;
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_INTCTRL, ams_pll_intctrl));
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_CONTROL_3, &ams_pll_ctrl3));
    ams_pll_ctrl3 = ((ams_pll_ctrl3 & ~BARCHETTA_PLL_CTRL_MASK) | (pll_1_pll_ctrl << BARCHETTA_PLL_CTRL_OFFSET));
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_CONTROL_3, ams_pll_ctrl3));

    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: Query AMS_PLL_COM_PLL_CONTROL_3, 0xD113[15:12] configuration to determine which PLL
 has been programmed when SyncE was enabled.
 COMMENT:
 *******************************************************************************/
int
_plp_barchetta_synce_get_rx_clk(const plp_barchetta_phymod_phy_access_t* phy, uint8_t *pll_0_pll_ctrl, uint8_t *pll_1_pll_ctrl)
{
    uint32_t ams_pll_ctrl3 = 0;

    if (!pll_0_pll_ctrl || !pll_1_pll_ctrl) {
        PHYMOD_DEBUG_ERROR(("ERROR: Invalid pll_ctrl values\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice_pll_index(phy, BARCHETTA_PLL_INDEX_0));
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_CONTROL_3, &ams_pll_ctrl3));

    *pll_0_pll_ctrl = ((ams_pll_ctrl3 & BARCHETTA_PLL_CTRL_MASK) >> BARCHETTA_PLL_CTRL_OFFSET);

    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_set_slice_pll_index(phy, BARCHETTA_PLL_INDEX_1));
    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_PLL_COM_PLL_CONTROL_3, &ams_pll_ctrl3));
    *pll_1_pll_ctrl = ((ams_pll_ctrl3 & BARCHETTA_PLL_CTRL_MASK) >> BARCHETTA_PLL_CTRL_OFFSET);
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: To determine if SyncE is enabled on any output lane.
 This function will return current SyncE state and the lane on which its enabled.
 COMMENT:
 *******************************************************************************/
int
_plp_barchetta_is_synce_enabled(const plp_barchetta_phymod_phy_access_t* phy, uint8_t *is_enabled, uint8_t *enabled_lane)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t num_of_max_lanes = 0, logical_lane = 0;
    int8_t lane_index = 0;
    uint32_t ams_rx_ctrl0 = 0;

    if (!is_enabled || !enabled_lane) {
        PHYMOD_DEBUG_ERROR(("ERROR: synce enabled\n"));
        return PHYMOD_E_PARAM;
    }

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    num_of_max_lanes = pkg_info.pkg_lanes;

    for (lane_index = 0; lane_index < num_of_max_lanes; lane_index++) {
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
            BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_RX_CONTROL_0, &ams_rx_ctrl0));
        if (ams_rx_ctrl0 & (BARCHETTA_TPORT_ENA << BARCHETTA_AMS_RX_CTRL_TPORT_EN_OFFSET)) {
            *is_enabled = 1;
            *enabled_lane = lane_index;
            break;
        } else {
            *is_enabled = 0;
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: Enable/Disable SyncE with user provided configuration.

 COMMENT:
 *******************************************************************************/
int
_plp_barchetta_synce_config_set(const plp_barchetta_phymod_phy_access_t* phy, const phymod_synce_cfg_t* synce_cfg)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t lanes_sel = 0;
    uint8_t logical_lane = 0, is_synce_enabled = 0, enabled_lane = 0;
    int8_t lane_index = 0;
    uint8_t is_line_side = 0, divider = 0;
    uint32_t ams_rx_ctrl5 = 0, ams_rx_ctrl0 = 0;
    plp_barchetta_phymod_phy_access_t phy_temp;
    int chip_id = 0;

    if (synce_cfg->clkGenSquelchCfg > phymodClkGenSquelchNone) {
        PHYMOD_DEBUG_ERROR(("clkGenSquelchCfg out of range :0x%0x, valid values: 0x0 to 0x1\n",
        synce_cfg->squelchMonitorLanemap));
        return PHYMOD_E_PARAM;
    }

    if (synce_cfg->clkGenSquelchCfg == phymodClkGenSquelchDisable) {
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_is_synce_enabled(phy, &is_synce_enabled, &enabled_lane));
        if (is_synce_enabled) {
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_get_logical_lane_from_lane_map(phy, enabled_lane, &logical_lane));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

            /* Disabling Clock output for the RX lane */
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_RX_CONTROL_0, &ams_rx_ctrl0));
            ams_rx_ctrl0 = ((ams_rx_ctrl0 & ~BARCHETTA_AMS_RX_CTRL_TPORT_EN_MASK) | (BARCHETTA_TPORT_DIS << BARCHETTA_AMS_RX_CTRL_TPORT_EN_OFFSET));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_RX_CONTROL_0, ams_rx_ctrl0));
            /* Clear divider configuration */
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_RX_CONTROL_5, &ams_rx_ctrl5));
            ams_rx_ctrl5 = ((ams_rx_ctrl5 & ~BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_MASK) | (0 << BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_OFFSET));
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_RX_CONTROL_5, ams_rx_ctrl5));
        } else {
            PHYMOD_DEBUG_ERROR(("SyncE is already disabled\n"));
            return PHYMOD_E_PARAM;
        }
        return PHYMOD_E_NONE;
    }
    /* Determine if SyncE configuration is requested on LINE or SYS side */
    if (synce_cfg->rclk_if_side == 0) {
        is_line_side = 1;
    } else if (synce_cfg->rclk_if_side == 1) {
        is_line_side = 0;
    } else {
        PHYMOD_DEBUG_ERROR(("rclk_if_side out of range :0x%0x, valid values: 0x0 and 0x1\n",
        synce_cfg->rclk_if_side));
        return PHYMOD_E_PARAM;
    }

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_barchetta_phymod_phy_access_t));

    if (synce_cfg->recoveredClkLane <= 3) {
        lanes_sel = 0; /* Select lanes 0 to 3 */
    } else if ((synce_cfg->recoveredClkLane >= 4) && (synce_cfg->recoveredClkLane <= 7)){
        lanes_sel = 1; /* Select lanes 4 to 7 */
    } else {
        PHYMOD_DEBUG_ERROR(("recoveredClkLane is out of range :0x%0x. It should between 0 to 7.\n",
        synce_cfg->recoveredClkLane));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_IF_ERR_RETURN(
             _plp_barchetta_get_chip_id(&phy->access, &chip_id));
    if (chip_id == BARCHETTA_CHIP_81338) {
        /* Programming both sides are needed for BCM81338 as the output pin is shared between LINE and SYS side */
        phy_temp.port_loc = phymodPortLocSys;
        if (is_line_side == 1) {
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_synce_update_rx_clk(&phy_temp, plp_barchetta_pll_ctrl_81338_line[lanes_sel][0], plp_barchetta_pll_ctrl_81338_line[lanes_sel][1]));
            phy_temp.port_loc = phymodPortLocLine;
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_synce_update_rx_clk(&phy_temp, plp_barchetta_pll_ctrl_81338_line[lanes_sel][2], plp_barchetta_pll_ctrl_81338_line[lanes_sel][3]));
        } else {
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_synce_update_rx_clk(&phy_temp, plp_barchetta_pll_ctrl_81338_sys[lanes_sel][0], plp_barchetta_pll_ctrl_81338_sys[lanes_sel][1]));
            phy_temp.port_loc = phymodPortLocLine;
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_synce_update_rx_clk(&phy_temp, plp_barchetta_pll_ctrl_81338_sys[lanes_sel][2], plp_barchetta_pll_ctrl_81338_sys[lanes_sel][3]));
        }
    } else if (chip_id == BARCHETTA_CHIP_81381) {
        /* Programming only on one side is needed for BCM81381 as there are separate output pins for LINE and SYS side */
        phy_temp.port_loc = (is_line_side == 1) ? phymodPortLocLine : phymodPortLocSys;
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_synce_update_rx_clk(&phy_temp, plp_barchetta_pll_ctrl_81381[lanes_sel][BARCHETTA_PLL_INDEX_0],
                                                       plp_barchetta_pll_ctrl_81381[lanes_sel][BARCHETTA_PLL_INDEX_1]));
    } else {
        PHYMOD_DEBUG_ERROR(("%s() SyncE not supported on chip_id:0x%x!!!\n",
        __func__, chip_id));
        return PHYMOD_E_PARAM;
    }
    switch(synce_cfg->divider) {
        case phymodDivider1:
            divider = BARCHETTA_TESTCLK_DIV1;
            break;
        case phymodDivider2:
            divider = BARCHETTA_TESTCLK_DIV2;
            break;
        case phymodDivider4:
            divider = BARCHETTA_TESTCLK_DIV4;
            break;
        case phymodDivider8:
            divider = BARCHETTA_TESTCLK_DIV8;
            break;
        default:
            PHYMOD_DEBUG_ERROR(("%s() SyncE not supported with divider:0x%x!!!\n",
                __func__, synce_cfg->divider));
            return PHYMOD_E_PARAM;
    }
    /* Configure mux clock output for lanes 0 to 3 if recovered clock is to be configured on lanes 0 to 3*/
    if (synce_cfg->recoveredClkLane <= 3) {
        for (lane_index = 3; lane_index >= 0; lane_index--) {
            if (plp_barchetta_testclk_mux[synce_cfg->recoveredClkLane][lane_index] != -1) {
                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_RX_CONTROL_5, &ams_rx_ctrl5));
                ams_rx_ctrl5 = ((ams_rx_ctrl5 & ~BARCHETTA_AMS_RX_CTRL_TESTCLK_MUX_MASK) | (plp_barchetta_testclk_mux[synce_cfg->recoveredClkLane][lane_index] << BARCHETTA_AMS_RX_CTRL_TESTCLK_MUX_OFFSET));
                if (logical_lane == synce_cfg->recoveredClkLane) {
                    ams_rx_ctrl5 = ((ams_rx_ctrl5 & ~BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_MASK) | (divider << BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_OFFSET));
                }
                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_RX_CONTROL_5, ams_rx_ctrl5));

                if (logical_lane == synce_cfg->recoveredClkLane) {
                    /* Enabling Clock output for the RX lane */
                    PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_RX_CONTROL_0, &ams_rx_ctrl0));
                    ams_rx_ctrl0 = ((ams_rx_ctrl0 & ~BARCHETTA_AMS_RX_CTRL_TPORT_EN_MASK) | (BARCHETTA_TPORT_ENA << BARCHETTA_AMS_RX_CTRL_TPORT_EN_OFFSET));
                    PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_RX_CONTROL_0, ams_rx_ctrl0));
                }
            }
        }
    }

    /* Configure mux clock output for lanes 4 to 7 if recovered clock is to be configured on lanes 4 to 7*/
    if ((synce_cfg->recoveredClkLane >= 4) && (synce_cfg->recoveredClkLane <= 7)) {
        for (lane_index = 3; lane_index >= 0; lane_index--) {
            if (plp_barchetta_testclk_mux[synce_cfg->recoveredClkLane][lane_index] != -1) {
                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_get_logical_lane_from_lane_map(phy, lane_index + 4, &logical_lane));
                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
                        BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));

                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_RX_CONTROL_5, &ams_rx_ctrl5));
                ams_rx_ctrl5 = ((ams_rx_ctrl5 & ~BARCHETTA_AMS_RX_CTRL_TESTCLK_MUX_MASK) | (plp_barchetta_testclk_mux[synce_cfg->recoveredClkLane][lane_index] << BARCHETTA_AMS_RX_CTRL_TESTCLK_MUX_OFFSET));
                if (logical_lane == synce_cfg->recoveredClkLane) {
                    /* Updating clock divider for the RX lane */
                    ams_rx_ctrl5 = ((ams_rx_ctrl5 & ~BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_MASK) | (divider << BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_OFFSET));
                }
                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_RX_CONTROL_5, ams_rx_ctrl5));

                if (logical_lane == synce_cfg->recoveredClkLane) {
                    /* Enabling Clock output for the RX lane */
                    PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_RX_CONTROL_0, &ams_rx_ctrl0));
                    ams_rx_ctrl0 = ((ams_rx_ctrl0 & ~BARCHETTA_AMS_RX_CTRL_TPORT_EN_MASK) | (BARCHETTA_TPORT_ENA << BARCHETTA_AMS_RX_CTRL_TPORT_EN_OFFSET));
                    PHYMOD_IF_ERR_RETURN(
                        _plp_barchetta_reg_write(&phy->access, BARCHETTA_AMS_RX_CONTROL_0, ams_rx_ctrl0));
                }
            }
        }
    }
    return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: Retrieve current SyncE Configuration and return to the user.

 COMMENT:
 *******************************************************************************/
int
_plp_barchetta_synce_config_get(const plp_barchetta_phymod_phy_access_t* phy, phymod_synce_cfg_t* synce_cfg)
{
    const plp_barchetta_phymod_access_t *pa = &phy->access;
    barchetta_package_info_t pkg_info;
    uint8_t lanes_sel = 0, logical_lane = 0;
    uint8_t divider = 0;
    uint32_t ams_rx_ctrl5 = 0;
    plp_barchetta_phymod_phy_access_t phy_temp;
    uint8_t pll_0_pll_ctrl = 0, pll_1_pll_ctrl = 0;
    int chip_id = 0;
    uint8_t is_synce_enabled = 0;
    uint8_t enabled_lane = 0;

    PHYMOD_MEMSET(&pkg_info, 0, sizeof(barchetta_package_info_t));
    PHYMOD_IF_ERR_RETURN(_plp_barchetta_get_package_info(pa, &pkg_info));
    PHYMOD_MEMCPY(&phy_temp, phy, sizeof(plp_barchetta_phymod_phy_access_t));

    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_is_synce_enabled(phy, &is_synce_enabled, &enabled_lane));
    if (is_synce_enabled) {
        synce_cfg->recoveredClkLane = enabled_lane;
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_get_logical_lane_from_lane_map(phy, enabled_lane, &logical_lane));
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_set_slice(phy, 0xFFFF, 0xFFFF,
            BARCHETTA_GET_REG_SEL_FOR_RX_OP(phy) , BarchettaRegisterTypePMD, logical_lane));
        PHYMOD_IF_ERR_RETURN(
            _plp_barchetta_reg_read(&phy->access, BARCHETTA_AMS_RX_CONTROL_5, &ams_rx_ctrl5));
        divider = ((ams_rx_ctrl5 & BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_MASK) >> BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_OFFSET);
        switch(divider) {
            case BARCHETTA_TESTCLK_DIV1:
                synce_cfg->divider = phymodDivider1;
                break;
            case BARCHETTA_TESTCLK_DIV2:
                synce_cfg->divider = phymodDivider2;
                break;
            case BARCHETTA_TESTCLK_DIV4:
                synce_cfg->divider = phymodDivider4;
                break;
            case BARCHETTA_TESTCLK_DIV8:
                synce_cfg->divider = phymodDivider8;
                break;
            default:
                PHYMOD_DEBUG_ERROR(("%s() SyncE not supported with divider:0x%x!!!\n",
                    __func__, divider));
                return PHYMOD_E_PARAM;
            }
        if (synce_cfg->recoveredClkLane <= 3) {
            lanes_sel = 0; /* Select lanes 0 to 3 */
        } else if ((synce_cfg->recoveredClkLane >= 4) && (synce_cfg->recoveredClkLane <= 7)){
            lanes_sel = 1; /* Select lanes 4 to 7 */
        } else {
            PHYMOD_DEBUG_ERROR(("recoveredClkLane is out of range :0x%0x. It should between 0 and 7.\n",
                synce_cfg->recoveredClkLane));
            return PHYMOD_E_PARAM;
        }

        synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchNone;
        PHYMOD_IF_ERR_RETURN(
             _plp_barchetta_get_chip_id(&phy->access, &chip_id));
        if (chip_id == BARCHETTA_CHIP_81381) {
            phy_temp.port_loc = phymodPortLocSys;
            PHYMOD_IF_ERR_RETURN(
                _plp_barchetta_synce_get_rx_clk(&phy_temp, &pll_0_pll_ctrl, &pll_1_pll_ctrl));
            if ((pll_0_pll_ctrl == plp_barchetta_pll_ctrl_81381[lanes_sel][BARCHETTA_PLL_INDEX_0]) &&
                (pll_1_pll_ctrl == plp_barchetta_pll_ctrl_81381[lanes_sel][BARCHETTA_PLL_INDEX_1])) {
                synce_cfg->rclk_if_side = 1; /* SYS Side */
            } else {
                phy_temp.port_loc = phymodPortLocLine;
                PHYMOD_IF_ERR_RETURN(
                    _plp_barchetta_synce_get_rx_clk(&phy_temp, &pll_0_pll_ctrl, &pll_1_pll_ctrl));
                if ((pll_0_pll_ctrl == plp_barchetta_pll_ctrl_81381[lanes_sel][BARCHETTA_PLL_INDEX_0]) &&
                    (pll_1_pll_ctrl == plp_barchetta_pll_ctrl_81381[lanes_sel][BARCHETTA_PLL_INDEX_1])) {
                    synce_cfg->rclk_if_side = 0; /* LINE Side */
                }
            }
        } else if (chip_id == BARCHETTA_CHIP_81338) {
            /* clk_if_side is not supported in _plp_barchetta_synce_config_get API since the
             * output pin is shared between system and line side of BCM81338 chip
             */
             ; /* Do nothing */
        } else {
            PHYMOD_DEBUG_ERROR(("%s() SyncE not supported on chip_id:0x%x!!!\n",
            __func__, chip_id));
            return PHYMOD_E_PARAM;
        }
    } else {
        synce_cfg->clkGenSquelchCfg = phymodClkGenSquelchDisable;
    }
   return PHYMOD_E_NONE;
}

/*******************************************************************************
 PURPOSE: Maps phymod diagnostics access type to serdes access types
 COMMENT:
 *******************************************************************************/
int _plp_barchetta_srds_diag_access_enum_mapping(enum phymod_srds_diag_access_e phymod_access_type, enum plp_barchetta_srds_diag_access_enum *srds_access_type)
{
    switch(phymod_access_type) {
        case phymodSrdsRegRead:
            *srds_access_type = SRDS_REG_READ;
            break;
        case phymodSrdsRegRmw:
            *srds_access_type = SRDS_REG_RMW;
            break;
        case phymodSrdsCoreRamRdByte:
            *srds_access_type = SRDS_CORE_RAM_READ_BYTE;
            break;
        case phymodSrdsCoreRamRmwByte:
            *srds_access_type = SRDS_CORE_RAM_RMW_BYTE;
            break;
        case phymodSrdsCoreRamRdWord:
            *srds_access_type = SRDS_CORE_RAM_READ_WORD;
            break;
        case phymodSrdsCoreRamRmwWord:
            *srds_access_type = SRDS_CORE_RAM_RMW_WORD;
            break;
        case phymodSrdsLaneRamRdByte:
            *srds_access_type = SRDS_LANE_RAM_READ_BYTE;
            break;
        case phymodSrdsLaneRamRmwByte:
            *srds_access_type = SRDS_LANE_RAM_RMW_BYTE;
            break;
        case phymodSrdsLaneRamRdWord:
            *srds_access_type = SRDS_LANE_RAM_READ_WORD;
            break;
        case phymodSrdsLaneRamRmwWord:
            *srds_access_type = SRDS_LANE_RAM_RMW_WORD;
            break;
        case phymodSrdsGlobRamRdByte:
            *srds_access_type = SRDS_GLOB_RAM_READ_BYTE;
            break;
        case phymodSrdsGlobRamRmwByte:
            *srds_access_type = SRDS_GLOB_RAM_RMW_BYTE;
            break;
        case phymodSrdsGlobRamRdWord:
            *srds_access_type = SRDS_GLOB_RAM_READ_WORD;
            break;
        case phymodSrdsGlobRamRmwWord:
            *srds_access_type = SRDS_GLOB_RAM_RMW_WORD;
            break;
        case phymodSrdsProgRamRdByte:
            *srds_access_type = SRDS_PROG_RAM_READ_BYTE;
            break;
        case phymodSrdsProgRamRdWord:
            *srds_access_type = SRDS_PROG_RAM_READ_WORD;
            break;
        case phymodSrdsUcCmd:
            *srds_access_type = SRDS_UC_CMD;
            break;
        default:
            PHYMOD_DEBUG_ERROR(("%s() Invalid access type:0x%x!!!\n",
                __func__, phymod_access_type));
            return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;
}
/*******************************************************************************
 PURPOSE: Enables serdes diagnostics access for debugging

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_srds_diag_access_enable(const plp_barchetta_phymod_phy_access_t* phy,
    const phymod_srds_diag_access_cfg_t* diag_access_cfg)
{
    enum plp_barchetta_srds_diag_access_enum srds_access_type = 0;

    PHYMOD_IF_ERR_RETURN(
        _plp_barchetta_srds_diag_access_enum_mapping(diag_access_cfg->access_type, &srds_access_type));

    PHYMOD_IF_ERR_RETURN(
        plp_barchetta_blackhawk_barchetta_diag_access(&phy->access, srds_access_type, diag_access_cfg->addr,
            diag_access_cfg->data, diag_access_cfg->param));

   return PHYMOD_E_NONE;
}
