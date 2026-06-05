/*
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 *
 * Note: INITIALIZE/MEMSET ALL FUNCTION PARAMETERS TO "0",
 *       TO AVOID FUTURE COMPATIBILITY ISSUES WITH THE DRIVER.
 */

/* Includes */
#include <aperta_common.h>

/* --------------------------------------------------------------------------------------*
 *                         Aperta reference application                                  *
 * ------------------------------------------------------------------------------------- *
 * This application configures Aperta chip Flexing between 25G and 50G Retimer mode with *
 * Failover configuration macsec static bypass mode as follows:                          *
 * --------------------------------------------------------------------------------------*
 * Note 1 : This sample application is only for 81343 parts.                             *
 * --------------------------------------------------------------------------------------*/

/* PHY (MDIO) address of all ports must be listed in the array phyid_all[]  */
int  scallop_main(int argc, char *argv[]);
int  phyid_all[33] = { 0, 1, -1 };

#define OP_MODE_STR    "Flexing between 25G and 50G Retimer mode with Failover"

/* System side parameters */
#define SYS_PORT_SPEED_1          25000
#define SYS_LANE_RATE_1           bcmpLplaneDataRate_25P78125G
#define SYS_MODULATION_1          bcmplpModulationNRZ
#define SYS_FEC_MODE_1            bcmplpapertaRSFEC
#define SYS_IF_TYPE_1             bcm_pm_InterfaceKR
#define SYS_IF_MODE_1             0
#define SYS_PORT_TYPE_1           bcmplpPortTypePassthrough

unsigned int sys_lane_map_list[] =  {
                                      0x01, 0x02, 0x04, 0x08
                                    };
unsigned int sysm_failover_lane_map_list[] = {
                                      0x10, 0x20, 0x40, 0x80
                                    };

/* Line side parameters */
#define LINE_PORT_SPEED_1         25000
#define LINE_LANE_RATE_1          bcmpLplaneDataRate_25P78125G
#define LINE_MODULATION_1         bcmplpModulationNRZ
#define LINE_FEC_MODE_1           bcmplpapertaRSFEC
#define LINE_IF_TYPE_1            bcm_pm_InterfaceKR
#define LINE_IF_MODE_1            0
#define LINE_PORT_TYPE_1          bcmplpPortTypePassthrough

unsigned int line_lane_map_list[] = {
                                      0x01, 0x02, 0x04, 0x08
                                    };
unsigned int line_failover_lane_map_list[] = {
                                      0x0 , 0x0 , 0x0 , 0x0
                                    };


/* System side parameters */
#define SYS_PORT_SPEED_2          50000
#define SYS_LANE_RATE_2           bcmpLplaneDataRate_25P78125G
#define SYS_MODULATION_2          bcmplpModulationNRZ
#define SYS_FEC_MODE_2            bcmplpapertaNoFEC
#define SYS_IF_TYPE_2             bcm_pm_InterfaceAUI_C2C
#define SYS_IF_MODE_2             0
#define SYS_PORT_TYPE_2           bcmplpPortTypeGearBox

unsigned int sysm_lane_map_list_2[] =  {
                                      0x03, 0x0C
                                    };
unsigned int sysm_lane_map_list_2_failover[] =  {
                                      0x30, 0xC0
                                    };

/* Line side parameters */
#define LINE_PORT_SPEED_2         50000
#define LINE_LANE_RATE_2          bcmpLplaneDataRate_53P125G
#define LINE_MODULATION_2         bcmplpModulationPAM4
#define LINE_FEC_MODE_2           bcmplpapertaRSFEC
#define LINE_IF_TYPE_2            bcm_pm_InterfaceCAUI4_C2C
#define LINE_IF_MODE_2            0
#define LINE_PORT_TYPE_2          bcmplpPortTypeGearBox

unsigned int line_lane_map_list_2[]          = { 0x01, 0x04 };
unsigned int line_lane_map_list_2_failover[] = { 0x00, 0x00 };

/* Main entry to application */
int main(int argc, char *argv[])
{
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int phy_index = 0;
    int side = 0;
    int lane_map_index;
    int total_lane_maps;
    int speed_r = 0;
    int intf_type_r = 0;
    int r_clk_r = 0;
    int if_mode_r = 0;
    char *board_sn = USB_DEV_SERIAL_CHIP0;

    unsigned int link_status_get;
    unsigned int sysm_lane_map;
    unsigned int line_lane_map;
    unsigned int sysm_failover_lane_map;
    unsigned int line_failover_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;
#if defined(__PERFORM_FAILOVER_SWITCHING__)
    unsigned int failover_mode;
#endif

    bcm_plp_mac_access_t mac_info;
    bcm_plp_access_t phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_aperta_fw_init_params_t aperta_init_params;
    bcm_plp_mac_flow_control_t flow_option;
    bcm_laneswap_map_t sys_laneswap_map, line_laneswap_map;
    bcm_plp_aperta_device_aux_modes_t aux_mode_set;
    bcm_plp_aperta_device_aux_modes_t aux_mode_sysm_get;
    bcm_plp_aperta_device_aux_modes_t aux_mode_line_get;

    /* Initialize structure instances to zero */
    memset(&mac_info, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    memset(&aperta_init_params,0, sizeof(bcm_plp_aperta_fw_init_params_t));
    memset(&sys_laneswap_map,  0, sizeof(bcm_laneswap_map_t));
    memset(&line_laneswap_map, 0, sizeof(bcm_laneswap_map_t));
    memset(&aux_mode_set,  0, sizeof(bcm_plp_aperta_device_aux_modes_t));
    memset(&aux_mode_sysm_get,  0, sizeof(bcm_plp_aperta_device_aux_modes_t));
    memset(&aux_mode_line_get,  0, sizeof(bcm_plp_aperta_device_aux_modes_t));

    phy_info.platform_ctxt = &p_ctxt;
    /* -------------------------------------------------------------------------------- *
     * Connect to the board                                                             *
     * -------------------------------------------------------------------------------- */
    board_sn = ( argv[1] ) ? argv[1] : USB_DEV_SERIAL_CHIP0;
    retval = device_sn_open(board_sn);
    if (retval != TEST_SUCCESS) {
        printf("FAIL (%d): Failed to connect to the board (ret = %d)!\n",__LINE__, retval);
        return retval;
    }

    /* -------------------------------------------------------------------------------- *
     * Section 1: PHY, MACsec Initialization and MACsec Configuration                   *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Initialize the PHYs and download firmware using broadcast method and             *
     * configure the macsec in static bypass mode                                       *
     * -------------------------------------------------------------------------------- */
    aperta_init_params.macsec_static_bypass = 1;
    aperta_init_params.pll1_vco_rate = bcmplpVco26p562G;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL; /* Call back function registration */
    firmware_load_type.fw_init_params = &aperta_init_params;

    phy_info.if_side = 0;
    phy_info.lane_map = ALL_LANE_MAP;

    for (phy_index = 0; phy_index <  NUM_OF_PHY ; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        printf("FW download on PHY ID %d using Internal method...\n", phy_info.phy_addr);
        retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastNone);
        if (retval != TEST_SUCCESS) {
            printf("FAIL (%d): Failed to enable broadcast method for PHY-%d (ret = %d)!\n",__LINE__,phy_info.phy_addr, retval);
            goto _aperta_init_error;
        }
    }

    printf("Read firmware info...\n");
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        retval = bcm_plp_firmware_info_get(CHIP_NAME, phy_info, &fw_ver, &fw_crc);
        if (retval != TEST_SUCCESS) {
            printf("FAIL (%d): Failed to get the firmware info for PHY-%d (ret = %d)!\n",__LINE__, phy_info.phy_addr, retval);
            goto _aperta_init_error;
        } else {
            printf("Firmware info for PHY-%d: FW version 0x%x, FW CRC 0x%x\n", phy_info.phy_addr, fw_ver, fw_crc);
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Section 2: HW Configuration for Polarity and Laneswap if applicable              *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Applying polarity configurations                                                 *
     * -------------------------------------------------------------------------------- */
    retval = polarity_swap();
    if (retval != TEST_SUCCESS) {
        printf("FAIL (%d): Failed to get and set initial Tx/Rx polarity with ret = %d\n",__LINE__, retval);
        goto _aperta_config_error;
    }

    /* -------------------------------------------------------------------------------- *
     * Applying LANE TX/RX Mapping                                                      *
     * LANE Mapping can be skipped if there is no lane swap                             *
     * -------------------------------------------------------------------------------- */
    sys_laneswap_map.num_of_lanes = 8;
    /* System side lane swap can be set differently if there are swaps */
    sys_laneswap_map.lane_map_rx[0] = sys_laneswap_map.lane_map_tx[0] = 0;
    sys_laneswap_map.lane_map_rx[1] = sys_laneswap_map.lane_map_tx[1] = 1;
    sys_laneswap_map.lane_map_rx[2] = sys_laneswap_map.lane_map_tx[2] = 2;
    sys_laneswap_map.lane_map_rx[3] = sys_laneswap_map.lane_map_tx[3] = 3;
    sys_laneswap_map.lane_map_rx[4] = sys_laneswap_map.lane_map_tx[4] = 4;
    sys_laneswap_map.lane_map_rx[5] = sys_laneswap_map.lane_map_tx[5] = 5;
    sys_laneswap_map.lane_map_rx[6] = sys_laneswap_map.lane_map_tx[6] = 6;
    sys_laneswap_map.lane_map_rx[7] = sys_laneswap_map.lane_map_tx[7] = 7;

    line_laneswap_map.num_of_lanes = 8;
    /* Line side lane swap can be set differently if there are swaps */
    line_laneswap_map.lane_map_rx[0] = line_laneswap_map.lane_map_tx[0] = 0;
    line_laneswap_map.lane_map_rx[1] = line_laneswap_map.lane_map_tx[1] = 1;
    line_laneswap_map.lane_map_rx[2] = line_laneswap_map.lane_map_tx[2] = 2;
    line_laneswap_map.lane_map_rx[3] = line_laneswap_map.lane_map_tx[3] = 3;
    line_laneswap_map.lane_map_rx[4] = line_laneswap_map.lane_map_tx[4] = 4;
    line_laneswap_map.lane_map_rx[5] = line_laneswap_map.lane_map_tx[5] = 5;
    line_laneswap_map.lane_map_rx[6] = line_laneswap_map.lane_map_tx[6] = 6;
    line_laneswap_map.lane_map_rx[7] = line_laneswap_map.lane_map_tx[7] = 7;

    phy_info.lane_map = ALL_LANE_MAP;
    for (phy_index = 0; phy_index < NUM_OF_PHY ; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;

        phy_info.if_side = SYS_SIDE;
        retval = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &sys_laneswap_map);
        if (retval != TEST_SUCCESS) {
            printf("FAIL (%d): Failed to set lane swap for PHY-%d at System side (ret = %d)!\n",__LINE__, phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at System side!\n", phy_info.phy_addr);
        }

        phy_info.if_side = LINE_SIDE;
        retval = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &line_laneswap_map);
        if (retval != TEST_SUCCESS) {
            printf("FAIL (%d): Failed to set lane swap for PHY-%d at Line side (ret = %d)!\n",__LINE__, phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at Line side!\n", phy_info.phy_addr);
        }
    }

    flow_option = bcmplpFlowcontrolPassthrough;
    total_lane_maps = NUM_ARR_ELEMENTS(line_lane_map_list);
    memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        mac_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        mac_info.phy_info.if_side = LINE_SIDE;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            mac_info.phy_info.lane_map = line_lane_map;

            retval = bcm_plp_mac_flow_control_set(CHIP_NAME, mac_info, flow_option);
            if (retval != TEST_SUCCESS) {
                printf("FAIL (%d): Failed to set flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",__LINE__,
                mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully set flow control option %d for lane-map 0x%x of PHY-%d!\n",
                flow_option, mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
            }
        }
    }

#if  defined(__CONFIG_25G__)

    /* -------------------------------------------------------------------------------- *
     *  Mode configuration 25Gig                                                        *
     *  It is required to configure SYSTEM side first followed by LINE side             *
     * -------------------------------------------------------------------------------- */
    printf("\n\nMode Configuration Set\n");
    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sysm_lane_map = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];
            sysm_failover_lane_map = sysm_failover_lane_map_list[lane_map_index];
            line_failover_lane_map = line_failover_lane_map_list[lane_map_index];

            for(side = SYS_SIDE; side >= LINE_SIDE; side--) {
                memset(&aux_mode_set, 0, sizeof(bcm_plp_aperta_device_aux_modes_t));
                phy_info.if_side  = (side == SYS_SIDE) ? SYS_SIDE : LINE_SIDE;
                phy_info.lane_map = (side == SYS_SIDE) ? sysm_lane_map : line_lane_map;

                aux_mode_set.failover_config.lane_map = (side == SYS_SIDE) ? sysm_failover_lane_map : line_failover_lane_map;
                aux_mode_set.lane_data_rate  = (side == SYS_SIDE) ? SYS_LANE_RATE_1  : LINE_LANE_RATE_1  ;
                aux_mode_set.modulation_mode = (side == SYS_SIDE) ? SYS_MODULATION_1 : LINE_MODULATION_1 ;
                aux_mode_set.fec_mode_sel    = (side == SYS_SIDE) ? SYS_FEC_MODE_1   : LINE_FEC_MODE_1   ;
                aux_mode_set.port_type       = (side == SYS_SIDE) ? SYS_PORT_TYPE_1  : LINE_PORT_TYPE_1  ;
                retval = bcm_plp_mode_config_set( CHIP_NAME, phy_info,
                                                  (side == SYS_SIDE) ? SYS_PORT_SPEED_1 : LINE_PORT_SPEED_1,
                                                  (side == SYS_SIDE) ? SYS_IF_TYPE_1    : LINE_IF_TYPE_1,
                                                  REF_CLOCK,
                                                  (side == SYS_SIDE) ? SYS_IF_MODE_1    : LINE_IF_MODE_1,
                                                  (void *)&aux_mode_set
                                                );
                if (retval != TEST_SUCCESS) {
                    printf("FAIL (%d): Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",__LINE__,
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at %s side!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                }
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Mode Configuration Get                                                           *
     * -------------------------------------------------------------------------------- */
    printf("\nMode Configuration Get\n");
    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sysm_lane_map  = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

            /* Set mode configuration at System side */
            phy_info.lane_map = sysm_lane_map;
            phy_info.if_side  = SYS_SIDE;
            retval = bcm_plp_mode_config_get(CHIP_NAME, phy_info, &speed_r, &intf_type_r, &r_clk_r, &if_mode_r, (void *)&aux_mode_sysm_get);
            if (retval != TEST_SUCCESS) {
                printf("FAIL (%d): Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",__LINE__,
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at System side!\n",
                phy_info.lane_map, phy_info.phy_addr);
            }

            /* Set mode configuration at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side  = LINE_SIDE;
            retval = bcm_plp_mode_config_get(CHIP_NAME, phy_info, &speed_r, &intf_type_r, &r_clk_r, &if_mode_r, (void *)&aux_mode_line_get);
            if (retval != TEST_SUCCESS) {
                printf("FAIL (%d): Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",__LINE__,
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at Line side!\n",
                phy_info.lane_map, phy_info.phy_addr);
            }
        }
    }
    sleep(2);

    /* -------------------------------------------------------------------------------- *
     * Get link status on SYS and LINE side                                             *
     * -------------------------------------------------------------------------------- */
    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            sysm_lane_map  = sys_lane_map_list[lane_map_index];
            for (side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = side;
                phy_info.lane_map = (side == SYS_SIDE) ? sysm_lane_map : line_lane_map ;
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL (%d): Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",__LINE__,
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                }
                printf("Link status for lane-map 0x%x of PHY-%d at %s side (expected: %d, actual: %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", LINK_ON, link_status_get);
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Switch over to failover path                                                     *
     * -------------------------------------------------------------------------------- */
    printf("\n\n Switching to Failover Lane\n");
    failover_mode = ENABLE;
    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sysm_lane_map  = sys_lane_map_list[lane_map_index];
            phy_info.lane_map = sysm_lane_map;
            phy_info.if_side  = side = SYS_SIDE;
            retval = bcm_plp_failover_mode_set(CHIP_NAME, phy_info, failover_mode);
            if (retval != TEST_SUCCESS) {
                printf("Failed to switch over in failover path for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                goto _aperta_config_error;
            } else {
                printf("Successfully switched over to failover path for lane-map 0x%x of PHY-%d at %s side !\n",
                phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Get link status on SYS and LINE side                                             *
     * -------------------------------------------------------------------------------- */
    total_lane_maps = NUM_ARR_ELEMENTS(sysm_failover_lane_map_list);
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            sysm_lane_map  = sysm_failover_lane_map_list[lane_map_index];
            for (side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = side;
                phy_info.lane_map = (side == SYS_SIDE) ? sysm_lane_map : line_lane_map ;
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL (%d): Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",__LINE__,
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                }
                printf("Link status for lane-map 0x%x of PHY-%d at %s side (expected: %d, actual: %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", LINK_ON, link_status_get);
            }
        }
    }

#endif /* __CONFIG_25G__*/

    /* -------------------------------------------------------------------------------- *
     *  Mode configuration 50G                                                          *
     * It is required to configure SYSTEM side first followed by LINE side              *
     * -------------------------------------------------------------------------------- */
    printf("\n\nMode Configuration Set\n");
    total_lane_maps = NUM_ARR_ELEMENTS(sysm_lane_map_list_2);
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sysm_lane_map = sysm_lane_map_list_2[lane_map_index];
            line_lane_map = line_lane_map_list_2[lane_map_index];
            sysm_failover_lane_map = sysm_lane_map_list_2_failover[lane_map_index];
            line_failover_lane_map = line_lane_map_list_2_failover[lane_map_index];

            for(side = SYS_SIDE; side >= LINE_SIDE; side--) {
                memset(&aux_mode_set, 0, sizeof(bcm_plp_aperta_device_aux_modes_t));
                phy_info.if_side  = (side == SYS_SIDE) ? SYS_SIDE : LINE_SIDE;
                phy_info.lane_map = (side == SYS_SIDE) ? sysm_lane_map : line_lane_map;

                aux_mode_set.failover_config.lane_map = (side == SYS_SIDE) ? sysm_failover_lane_map
                                                                           : line_failover_lane_map;
                aux_mode_set.lane_data_rate  = (side == SYS_SIDE) ? SYS_LANE_RATE_2  : LINE_LANE_RATE_2  ;
                aux_mode_set.modulation_mode = (side == SYS_SIDE) ? SYS_MODULATION_2 : LINE_MODULATION_2 ;
                aux_mode_set.fec_mode_sel    = (side == SYS_SIDE) ? SYS_FEC_MODE_2   : LINE_FEC_MODE_2   ;
                aux_mode_set.port_type       = (side == SYS_SIDE) ? SYS_PORT_TYPE_2  : LINE_PORT_TYPE_2  ;
                retval = bcm_plp_mode_config_set( CHIP_NAME, phy_info,
                                                  (side == SYS_SIDE) ? SYS_PORT_SPEED_2 : LINE_PORT_SPEED_2,
                                                  (side == SYS_SIDE) ? SYS_IF_TYPE_2    : LINE_IF_TYPE_2,
                                                  REF_CLOCK,
                                                  (side == SYS_SIDE) ? SYS_IF_MODE_2    : LINE_IF_MODE_2,
                                                  (void *)&aux_mode_set
                                                );
                if (retval != TEST_SUCCESS) {
                    printf("FAIL (%d): Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",__LINE__,
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at %s side!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                }
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Mode Configuration Get                                                           *
     * -------------------------------------------------------------------------------- */
    printf("\nMode Configuration Get\n");
    total_lane_maps = NUM_ARR_ELEMENTS(sysm_lane_map_list_2);
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sysm_lane_map  = sysm_lane_map_list_2[lane_map_index];
            line_lane_map = line_lane_map_list_2[lane_map_index];

            /* get mode configuration at System side */
            phy_info.lane_map = sysm_lane_map;
            phy_info.if_side  = SYS_SIDE;
            retval = bcm_plp_mode_config_get(CHIP_NAME, phy_info, &speed_r, &intf_type_r, &r_clk_r, &if_mode_r, (void *)&aux_mode_sysm_get);
            if (retval != TEST_SUCCESS) {
                printf("FAIL (%d): Failed to get mode configuration parameters for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",__LINE__,
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully configured the System side of [0x%02x.0x%04x] (FOVmap=0x%04x FOVloc=0x%04x)\n",
                phy_info.phy_addr, phy_info.lane_map, aux_mode_sysm_get.failover_config.lane_map, aux_mode_sysm_get.failover_config.mux_location);
            }

            /* get mode configuration at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side  = LINE_SIDE;
            retval = bcm_plp_mode_config_get(CHIP_NAME, phy_info, &speed_r, &intf_type_r, &r_clk_r, &if_mode_r, (void *)&aux_mode_line_get);
            if (retval != TEST_SUCCESS) {
                printf("FAIL (%d): Failed to get mode configuration parameters for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",__LINE__,
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully configured the  Line  side of [0x%02x.0x%04x] (FOVmap=0x%04x FOVloc=0x%04x)\n",
                phy_info.phy_addr, phy_info.lane_map, aux_mode_line_get.failover_config.lane_map, aux_mode_line_get.failover_config.mux_location);
            }
        }
    }

#if defined(__PERFORM_FAILOVER_SWITCHING__)
    sleep(2);

    /* -------------------------------------------------------------------------------- *
     * Switch over to failover path                                                     *
     * -------------------------------------------------------------------------------- */
    printf("\n\n Switching to Failover Lane\n");
    failover_mode = ENABLE;
    total_lane_maps = NUM_ARR_ELEMENTS(sysm_lane_map_list_2);
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sysm_lane_map     = sysm_lane_map_list_2[lane_map_index];
            phy_info.lane_map = sysm_lane_map;
            phy_info.if_side  = side = SYS_SIDE;
            retval = bcm_plp_failover_mode_set(CHIP_NAME, phy_info, failover_mode);
            if (retval != TEST_SUCCESS) {
                printf("Failed to switch over in failover path for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                goto _aperta_config_error;
            } else {
                printf("Successfully switched over to failover path for lane-map 0x%x of PHY-%d at %s side !\n",
                phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
            }
        }
    }
#endif /* __PERFORM_FAILOVER_SWITCHING__*/

    /* -------------------------------------------------------------------------------- *
     * Get link status on SYS and LINE side                                             *
     * -------------------------------------------------------------------------------- */
    total_lane_maps = NUM_ARR_ELEMENTS(sysm_lane_map_list_2_failover);
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list_2[lane_map_index];
            sysm_lane_map  = sysm_lane_map_list_2_failover[lane_map_index];
            for (side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = side;
                phy_info.lane_map = (side == SYS_SIDE) ? sysm_lane_map : line_lane_map ;
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL (%d): Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",__LINE__,
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                }
                printf("Link status for [0x%02x.0x%04x] %s side (expected: %d, actual: %d)\n",
                phy_info.phy_addr, phy_info.lane_map, (side == SYS_SIDE) ? "System" : "  Line", LINK_ON, link_status_get);
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * run the Scallop CLI Shell                                                        *
     * -------------------------------------------------------------------------------- */
     printf("\n========  %s CLI Reference App  ========\n", CHIP_NAME);
     scallop_main(argc, argv);

_aperta_config_error:
    /* -------------------------------------------------------------------------------- *
     * PHY and MAC Cleanup                                                              *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side  = LINE_SIDE;
        memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        retval = bcm_plp_mac_cleanup(CHIP_NAME, mac_info);
        if (retval != TEST_SUCCESS) {
            printf("FAIL (%d): Failed to cleanup mac for PHY-%d (ret = %d)!\n",__LINE__, phy_info.phy_addr, retval);
        }
        retval = bcm_plp_cleanup(CHIP_NAME, phy_info);
        if (retval != TEST_SUCCESS) {
            printf("FAIL (%d): Failed to cleanup PHY-%d (ret = %d)!\n",__LINE__, phy_info.phy_addr, retval);
        }
    }
_aperta_init_error:
    /* Check for test result */
    if (retval == TEST_SUCCESS) {
        printf("Test for %s mode completed successfully\n", argv[0]);
    } else {
        printf("Test for %s mode failed!\n", argv[0]);
    }

    /* -------------------------------------------------------------------------------- *
     * Close the connections to the board                                               *
     * -------------------------------------------------------------------------------- */
    device_close();

    /* Return with test status */
    return retval;
}
