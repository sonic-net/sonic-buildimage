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
#include <stdio.h>
#include <aperta_common.h>

/* -------------------------------------------------------------------------------- *
 *                         Aperta reference application                             *
 * -------------------------------------------------------------------------------- *
 * This sample application configures the Aperta chip for 25G (1x25G)               *
 * macsec static bypass mode as follows:                                            *
 *    1. Connect to the board.                                                      *
 *    2. Initialize the PHY and download the firmware using broadcast method        *
 *    3. Initialize MACsec in static bypass mode                                    *
 *    4. Configure Tx/Rx polarity                                                   *
 *    5. Apply lane swap (if required)                                              *
 *    6. Configure operating mode parameters for 25G NRZ (1x25G) mode               *
 *    7. Setup link training (Not mandatory)                                        *
 *    8. Check link status                                                          *
 *    9. Set AN ability and enable AN                                               *
 *   10. Get AN ability and AN status                                               *
 *   11. Check link status                                                          *
 *   12. Run the Scallop CLI Shell  (optional)                                      *
 *   13. Cleanup and close the connection to the board.                             *
 * -------------------------------------------------------------------------------- *
 * Note 1 : This sample application is only for 81343 parts.                        *
 * -------------------------------------------------------------------------------- */
#ifndef  TRUE
#define  TRUE 1
#endif
#ifndef  FALSE
#define  FALSE 0
#endif

/* 25G (1x25G) NRZ Autoneg demonstration (IEEE) */
#define OP_MODE_STR    "25G (1x25G) NRZ Autoneg demonstration (IEEE)"

/* Enable Tx forced training */
#undef LINK_TRAINING_EN

#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1

/* Following defines are AN test specific configuration */
#define AN_MASTER_LANE  0
#define CL72_EN         1
#define TECH_ABILITY    bcmplpAnCap25G_KR
#define FEC_ABILITY     0
#define PAUSE_ABILITY   0

/* System side parameters */
#define SYS_PORT_SPEED          25000
#define SYS_LANE_RATE           bcmpLplaneDataRate_25P78125G
#define SYS_MODULATION          bcmplpModulationNRZ
#define SYS_FEC_MODE            bcmplpapertaNoFEC
#define SYS_IF_TYPE             bcm_pm_InterfaceKR
#define SYS_IF_MODE             0
#define SYS_PORT_TYPE           bcmplpPortTypePassthrough

unsigned int sys_lane_map_list[] =  {
                                      0x01,
                                      0x02,
                                      0x04,
                                      0x08,
                                      0x10,
                                      0x20,
                                      0x40,
                                      0x80
                                    };

/* Line side parameters */
#define LINE_PORT_SPEED         25000
#define LINE_LANE_RATE          bcmpLplaneDataRate_25P78125G
#define LINE_MODULATION         bcmplpModulationNRZ
#define LINE_FEC_MODE           bcmplpapertaNoFEC
#define LINE_IF_TYPE            bcm_pm_InterfaceKR
#define LINE_IF_MODE            0
#define LINE_PORT_TYPE          bcmplpPortTypePassthrough

unsigned int line_lane_map_list[] = {
                                      0x01,
                                      0x02,
                                      0x04,
                                      0x08,
                                      0x10,
                                      0x20,
                                      0x40,
                                      0x80
                                    };

int  phyid_all[33] = { 0x00, 0x01, -1 };  /* PHY_ID list, used by CLI Shell */

/* Main entry to application */
int main(int argc, char *argv[])
{
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int phy_index = 0;
    int side = 0;
    int lane_map_index;
    int total_lane_maps;
    int run_shell  = FALSE;
    char *board_sn = USB_DEV_SERIAL_CHIP0;

    unsigned int link_status_get;
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;

    bcm_plp_mac_access_t mac_info;
    bcm_plp_access_t phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_aperta_fw_init_params_t aperta_init_params;
    bcm_plp_mac_flow_control_t flow_option, flow_option_get;
    bcm_laneswap_map_t sys_laneswap_map, line_laneswap_map;
    bcm_plp_aperta_device_aux_modes_t aux_mode_sys_set, aux_mode_line_set;

    bcm_plp_an_config_t an_config_set;
    bcm_plp_an_config_t an_config_get;

#ifdef LINK_TRAINING_EN
    unsigned int training_ena_dis = DISABLE;
    unsigned int training_failure = TRAINING_FAILURE;
    unsigned int training_status  = UNTRAINED;
#endif

    unsigned int   an_ena_dis = DISABLE ;
    unsigned int   an_done = 0;
    unsigned short pause_ability_set  = 0 ;
    unsigned short pause_ability_get  = 0 ;
    unsigned short tech_ability_set   = 0 ;
    unsigned short tech_ability_get   = 0 ;
    unsigned short fec_ability_set    = 0 ;
    unsigned short fec_ability_get    = 0 ;

    /* Initialize structure instances to zero */
    memset(&mac_info, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    memset(&aperta_init_params,0, sizeof(bcm_plp_aperta_fw_init_params_t));
    memset(&sys_laneswap_map,  0, sizeof(bcm_laneswap_map_t));
    memset(&line_laneswap_map, 0, sizeof(bcm_laneswap_map_t));
    memset(&aux_mode_sys_set,  0, sizeof(bcm_plp_aperta_device_aux_modes_t));
    memset(&aux_mode_line_set, 0, sizeof(bcm_plp_aperta_device_aux_modes_t));
    memset(&an_config_set,     0, sizeof(bcm_plp_an_config_t));

    phy_info.platform_ctxt = &p_ctxt;

    /* -------------------------------------------------------------------------------- *
     * Connect to the board                                                             *
     * -------------------------------------------------------------------------------- */
    run_shell = ( argv[2] ) ? TRUE : FALSE;  /* run CLI shell if the 2nd argument given */
    board_sn  = ( argv[1] ) ? argv[1] : USB_DEV_SERIAL_CHIP0;   /* board's USB S/N      */
    retval = device_sn_open(board_sn);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to connect to the board (ret = %d)!\n", retval);
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
    aperta_init_params.pll1_vco_rate = bcmplpVco25p781G;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL; /* Call back function registration */
    firmware_load_type.fw_init_params = &aperta_init_params;

    phy_info.if_side = 0;
    phy_info.lane_map = ALL_LANE_MAP;

    printf("Reset the core for all PHY Ids on MDIO Bus...\n");
    for (phy_index = 0; phy_index <  NUM_OF_PHY ; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        printf("Resetting core of PHY-%d ...\n", phy_info.phy_addr);
        retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastCoreReset);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to perform core reset of PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_init_error;
        }
        sleep(1) ;
    }

    printf("Enable the broadcast method for all PHY Ids on MDIO Bus...\n");
    for (phy_index = 0; phy_index <  NUM_OF_PHY ; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastEnable);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to enable broadcast method for PHY-%d (ret = %d)!\n",phy_info.phy_addr, retval);
            goto _aperta_init_error;
        }
    }

    printf("Load the FW for only one PHY_ID0, internally it will broadcast firmware of similar type of phys on same MDIO bus...\n");
    phy_info.phy_addr = PHY_ID0;
    retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastFirmwareExecute);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to load broadcast firmware method for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        goto _aperta_init_error;
    }

    printf("Broadcast firmware download method verification for all PHY Ids on MDIO bus...\n");
    for (phy_index = 0; phy_index <  NUM_OF_PHY ; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastFirmwareVerify);
        if (retval != TEST_SUCCESS) {
          printf("FAIL: Failed to verify firmware broadcast method for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
          goto _aperta_init_error;
        }
    }

    printf("Disable the Broadcast firmware download method for all PHY Ids on MDIO bus...\n");
    for (phy_index = 0; phy_index <  NUM_OF_PHY ; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastEnd);
        if (retval != TEST_SUCCESS) {
          printf("FAIL: Failed to disable firmware broadcast method for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
          goto _aperta_init_error;
        }
    }

    printf("Read firmware info...\n");
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        retval = bcm_plp_firmware_info_get(CHIP_NAME, phy_info, &fw_ver, &fw_crc);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to get the firmware info for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
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
        printf("FAIL: Failed to get and set initial Tx/Rx polarity with ret = %d\n", retval);
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
            printf("FAIL: Failed to set lane swap for PHY-%d at System side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at System side!\n", phy_info.phy_addr);
        }

        phy_info.if_side = LINE_SIDE;
        retval = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &line_laneswap_map);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to set lane swap for PHY-%d at Line side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at Line side!\n", phy_info.phy_addr);
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Section 3: Mode configuration                                                    *
     * -------------------------------------------------------------------------------- */
   total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);

    /* Enable flow control and configure 25G (1x25G) mode in NRZ, no FEC */
    /* flow_option = bcmplpFlowcontrolTerminateGenerate; */
    flow_option = bcmplpFlowcontrolPassthrough;
    memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        mac_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        mac_info.phy_info.if_side = LINE_SIDE;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            mac_info.phy_info.lane_map = line_lane_map;

            retval = bcm_plp_mac_flow_control_set(CHIP_NAME, mac_info, flow_option);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to set flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",
                mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully set flow control option %d for lane-map 0x%x of PHY-%d!\n",
                flow_option, mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
            }
            retval = bcm_plp_mac_flow_control_get(CHIP_NAME, mac_info, &flow_option_get);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to get flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",
                mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully got flow control option %d for lane-map 0x%x of PHY-%d!\n",
                flow_option_get, mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
            }
            if (flow_option != flow_option_get) {
                printf("FAIL: flow options does not match for lane-map 0x%x of PHY-%d!\n",
                mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
                goto _aperta_config_error;
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * It is required to configure SYSTEM side first followed by LINE side              *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

            /* Set mode configuration at System side */
            phy_info.lane_map = sys_lane_map;
            phy_info.if_side  = SYS_SIDE;
            aux_mode_sys_set.lane_data_rate  = SYS_LANE_RATE;
            aux_mode_sys_set.modulation_mode = SYS_MODULATION;
            aux_mode_sys_set.fec_mode_sel    = SYS_FEC_MODE;
            aux_mode_sys_set.port_type       = SYS_PORT_TYPE;
            retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, SYS_PORT_SPEED, SYS_IF_TYPE, REF_CLOCK, SYS_IF_MODE, (void *)&aux_mode_sys_set);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at System side!\n",
                phy_info.lane_map, phy_info.phy_addr);
            }

            /* Set mode configuration at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side  = LINE_SIDE;
            aux_mode_line_set.lane_data_rate  = LINE_LANE_RATE;
            aux_mode_line_set.modulation_mode = LINE_MODULATION;
            aux_mode_line_set.fec_mode_sel    = LINE_FEC_MODE;
            aux_mode_line_set.port_type       = LINE_PORT_TYPE;
            retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, LINE_PORT_SPEED, LINE_IF_TYPE, REF_CLOCK, LINE_IF_MODE, (void *)&aux_mode_line_set);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at Line side!\n",
                phy_info.lane_map, phy_info.phy_addr);
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Section 4: Link training                                                         *
     * -------------------------------------------------------------------------------- */
#ifdef LINK_TRAINING_EN
    /* -------------------------------------------------------------------------------- *
     * Start Force Tx training                                                          *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];
            for(side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = (side == SYS_SIDE) ? SYS_SIDE : LINE_SIDE;
                phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map;
                retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr,(side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                } else {
                    printf("Force Tx training is in progress for lane map 0x%x of PHY-%d at %s side...\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                }
            }
        }
    }
    sleep(2);
    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training status                                                     *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];
            for(side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = (side == SYS_SIDE) ? SYS_SIDE : LINE_SIDE;
                phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map;
                training_ena_dis  = DISABLE ;
                retval = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
                if (retval != TEST_SUCCESS) {
                     printf("FAIL: Failed to get training feature for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                     phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                     goto _aperta_config_error;
                } else {
                    printf("Training enable/disable status for lane-map 0x%x of PHY-%d at %s side as below :\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                    printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
                }

                training_ena_dis = DISABLE;
                training_failure = TRAINING_FAILURE;
                training_status  = UNTRAINED;

                retval = bcm_plp_force_tx_training_status_get(CHIP_NAME, phy_info, &training_ena_dis, &training_failure, &training_status);
                if (retval != TEST_SUCCESS) {
                  printf("FAIL: Failed to get training status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                  phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                  goto _aperta_config_error;
                } else {
                    printf("Training status for lane-map 0x%x of PHY-%d at %s side as below :\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                    printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
                    printf("training_failure        = %s\n", training_failure ? "TRAINING_FAILURE" : "TRAINING_COMPLETE");
                    printf("training_status         = %s\n", training_status ? "TRAINED" : "UNTRAINED");
                }
            }
        }
    }
#endif /* LINK_TRAINING_EN */

    sleep(5);

    /* -------------------------------------------------------------------------------- *
     * Get link status on SYS and LINE side                                             *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            for (side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = side;
                phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map ;
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    if ( ! run_shell)    goto _aperta_config_error;
                }
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    if ( ! run_shell)    goto _aperta_config_error;
                }
                /* Validate the link status */
                if (LINK_ON != link_status_get) {
                    retval = TEST_FAILURE;
                    printf("Link status for lane-map 0x%x of PHY-%d at %s side is not as expected (expected: %d, actual: %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", LINK_ON, link_status_get);
                    if ( ! run_shell)    goto _aperta_config_error;
                }
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Ports are setup now and ready to test with traffic                               *
     * -------------------------------------------------------------------------------- */
    printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("MacSec is in static bypass mode\n");
    printf("Ports are setup now and ready to test with traffic\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

  if ( ! run_shell ) {
    /* -------------------------------------------------------------------------------- *
     * Start AN at aperta                                                               *
     * -------------------------------------------------------------------------------- */
    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Start AN at Aperta :\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    an_config_set.master_lane  = AN_MASTER_LANE ;
    an_config_set.cl72_en      = CL72_EN        ;
    an_config_set.tech_ability = TECH_ABILITY   ;
    fec_ability_set            = FEC_ABILITY    ;
    pause_ability_set          = PAUSE_ABILITY  ;

    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];
            for(side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = (side == SYS_SIDE) ? SYS_SIDE : LINE_SIDE;
                phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map ;
                retval = bcm_plp_cl73_ability_set(CHIP_NAME, phy_info, tech_ability_set, fec_ability_set, pause_ability_set, an_config_set);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to set cl73 ability for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                }

                retval = bcm_plp_cl73_set(CHIP_NAME, phy_info, ENABLE);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to enable AN for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                } else {
                    printf("AN is in progress for lane map 0x%x of PHY-%d at %s side...\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                }
            }
        }
    }

    sleep(5);

    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Get AN status from both side of Aperta :\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];
            for(side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = (side == SYS_SIDE) ? SYS_SIDE : LINE_SIDE;
                phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map ;

                tech_ability_get  = 0 ;
                fec_ability_get   = 0 ;
                pause_ability_get = 0 ;
                memset(&an_config_get, 0, sizeof(bcm_plp_an_config_t));

                retval = bcm_plp_cl73_ability_get(CHIP_NAME, phy_info, &tech_ability_get, &fec_ability_get, &pause_ability_get, &an_config_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to get cl73 ability for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                } else {
                    printf("CL73 ability get parameters for lane-map 0x%x of PHY-%d at %s side as below :\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                    printf("tech_ability_get(16-bit)           = 0x%x\n", tech_ability_get );
                    printf("fec_ability_get                    = %d\n"  , fec_ability_get  );
                    printf("pause_ability_get                  = %d\n"  , pause_ability_get);
                    printf("an_config_get.master_lane          = %d\n"  , an_config_get.master_lane);
                    printf("an_config_get.cl72_en              = %d\n"  , an_config_get.cl72_en);
                    printf("an_config_get.tech_ability(32-bit) = 0x%x\n", an_config_get.tech_ability);
                }

                an_ena_dis = DISABLE ;
                an_done    = 0;
                retval = bcm_plp_cl73_get(CHIP_NAME, phy_info, &an_ena_dis, &an_done);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to get AN status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                } else {
                    printf("AN status for lane-map 0x%x of PHY-%d at %s side as below :\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                    printf("an_enable_disable = %s\n", an_ena_dis ? "AN_ENABLE" : "AN_DISABLE");
                    printf("an_done           = %d\n", an_done);
                }

                /* Validate an_done */
                if (an_done != 1) {
                    retval = TEST_FAILURE;
                    printf("FAIL: AN not done for lane-map 0x%x of PHY-%d at %s side!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                }
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Get link status on SYS and LINE side                                             *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            for (side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = side;
                phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map ;
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                }
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                }
                /* Validate the link status */
                if (LINK_ON != link_status_get) {
                    retval = TEST_FAILURE;
                    printf("Link status for lane-map 0x%x of PHY-%d at %s side is not as expected (expected: %d, actual: %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", LINK_ON, link_status_get);
                    goto _aperta_config_error;
                }
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Ports are setup now after AN and ready to test with traffic                      *
     * -------------------------------------------------------------------------------- */
    printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("Ports are setup now after AN and ready to test with traffic\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
  }
#if defined(BCM_PLP_TIMESYNC_SUPPORT)
  else {
    /* -------------------------------------------------------------------------------- *
     * run the Scallop CLI Shell                                                        *
     * -------------------------------------------------------------------------------- */
    printf("\n========  %s CLI Reference App  ========\n", CHIP_NAME);
    scallop_main(argc, argv);
  }
#endif

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
            printf("FAIL: Failed to cleanup mac for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        }
        retval = bcm_plp_cleanup(CHIP_NAME, phy_info);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to cleanup PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
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
