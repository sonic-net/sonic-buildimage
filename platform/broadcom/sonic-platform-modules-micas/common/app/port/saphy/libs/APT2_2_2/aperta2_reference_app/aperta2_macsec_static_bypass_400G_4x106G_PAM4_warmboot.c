/*
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * BCM APIs. This program might not work for all environments.
 *
 * Note: INITIALIZE/MEMSET EVERY FUNCTION PARAMETERS TO "0",
 *       TO AVOID FUTURE COMPATIBILITY ISSUES WITH THE DRIVER.
 */

/* Includes */
#include <aperta2_common.h>

/* -------------------------------------------------------------------------------- *
 *                         Aperta2 reference application                            *
 * -------------------------------------------------------------------------------- *
 * This sample application configures the Aperta chip for 400G(4x106G) PAM4 with    *
 * macsec static bypass mode as follows:                                            *
 *   1. Connect to the board.                                                       *
 *   2. Initialize the PHY With WARMBOOT                                            *
 *   3. Get Polarity and TXRX LaneSwap configuration                                *
 *   4. Get Mode parameters                                                         *
 *   5. Check for Training Status on System and Line side                           *
 *   6. Check Link Status on System and Line side                                   *
 *   7. Cleanup and close the connection to the board.                              *
 * -------------------------------------------------------------------------------- *
 * Note : This sample application is only for 85343 parts.                          *
 * -------------------------------------------------------------------------------- */

/* System side lanemaps */
unsigned int sys_lane_map_list[] =  {
                                      0x000F, 0x00F0, 0x0F00, 0xF000
                                    };

/* Line side lanemaps */
unsigned int line_lane_map_list[] = {
                                      0x000F, 0x00F0, 0x0F00, 0xF000
                                    };

/* Main entry to application */
int main(int argc, char *argv[])
{
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int side = 0;
    int lane_map_index;
    int total_lane_maps;

    unsigned int link_status_get;
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;
    int speed_get = 0, if_type_get = 0, ref_clk_get = 0, if_mode_get = 0;

    bcm_plp_mac_access_t mac_info;
    bcm_plp_access_t phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_aperta2_fw_init_params_t fw_init_params;
    bcm_plp_aperta2_device_aux_modes_t aux_mode_sys_set, aux_mode_line_set;
    bcm_plp_aperta2_device_aux_modes_t aux_mode_get;

    unsigned int training_ena_dis = DISABLE;
    unsigned int training_failure = TRAINING_FAILURE;
    unsigned int training_status  = UNTRAINED;

    /* Initialize structure instances to zero */
    memset(&mac_info, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    memset(&fw_init_params,0, sizeof(bcm_plp_aperta2_fw_init_params_t));
    memset(&aux_mode_sys_set,  0, sizeof(bcm_plp_aperta2_device_aux_modes_t));
    memset(&aux_mode_line_set, 0, sizeof(bcm_plp_aperta2_device_aux_modes_t));
    memset(&aux_mode_get, 0, sizeof(bcm_plp_aperta2_device_aux_modes_t));

    phy_info.platform_ctxt = &p_ctxt;

    /* -------------------------------------------------------------------------------- *
     * Connect to the board                                                             *
     * -------------------------------------------------------------------------------- */
    retval = device_open();
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to connect to the board (ret = %d)!\n", retval);
        return retval;
    }

    /* -------------------------------------------------------------------------------- *
     * Initialize the PHYs with Warmboot                                                *
     * -------------------------------------------------------------------------------- */
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodNone;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadSkip;
    firmware_load_type.fw_init_params = &fw_init_params;

    phy_info.if_side = 0;
    phy_info.lane_map = ALL_LANE_MAP;
    phy_info.phy_addr = PHY_ID;
    phy_info.flags = BCM_PLP_WARM_BOOT;
    printf("Initializing PHY-%d with Warmboot\n", phy_info.phy_addr);
    retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastNone);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: bcm_plp_init_fw_bcast failed to initialize PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        goto _aperta2_init_error;
    }

    /* Read firmware info */
    retval = bcm_plp_firmware_info_get(CHIP_NAME, phy_info, &fw_ver, &fw_crc);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get the firmware info for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        goto _aperta2_init_error;
    } else {
        printf("Firmware info for PHY-%d: FW version 0x%x, FW CRC 0x%x\n", phy_info.phy_addr, fw_ver, fw_crc);
    }


    /* -------------------------------------------------------------------------------- *
     * Get Polarity                                                                     *
     * -------------------------------------------------------------------------------- */
    retval = get_polarity();
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get Tx/Rx polarity with ret = %d\n", retval);
        goto _aperta2_config_error;
    }

    /* -------------------------------------------------------------------------------- *
     * Get TXRX LaneSwap                                                                *
     * -------------------------------------------------------------------------------- */
    retval = get_txrx_laneswap();
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get Tx/Rx Laneswap with ret = %d\n", retval);
        goto _aperta2_config_error;
    }

    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    /* -------------------------------------------------------------------------------- *
     * Mode config get after Warmboot                                                   *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        sys_lane_map  = sys_lane_map_list[lane_map_index];
        line_lane_map = line_lane_map_list[lane_map_index];

        memset(&aux_mode_get,  0, sizeof(bcm_plp_aperta2_device_aux_modes_t));

        /* Get mode configuration at System side */
        phy_info.lane_map = sys_lane_map;
        phy_info.if_side  = BCM_SYSTEM_SIDE;

        retval = bcm_plp_mode_config_get(CHIP_NAME, phy_info, &speed_get, &if_type_get, &ref_clk_get, &if_mode_get, (void *)&aux_mode_get);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to get mode configuration parameters for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("%s_SIDE: plp_Mode config get phy_id = %d,port_map = 0x%x Interface = %d, speed = %d if_mode:%d ref_clk:%d\n",
                    phy_info.if_side?"SYS":"LINE", phy_info.phy_addr, phy_info.lane_map, if_type_get, speed_get, if_mode_get, ref_clk_get);
            printf("plp_Mode config get lane_data_rate = %d modulation_mode = %d fec_mode_sel = %d\n",
                    aux_mode_get.lane_data_rate,aux_mode_get.modulation_mode,aux_mode_get.fec_mode_sel);
            printf("plp_Mode config get port_type = %d octal_crossing = %d ing_fixed_latency.enable = %d egr_fixed_latency.enable = %d\n\n",
                    aux_mode_get.port_type,aux_mode_get.octal_crossing,aux_mode_get.ing_fixed_latency.enable,aux_mode_get.egr_fixed_latency.enable);
        }

        /* Get mode configuration at Line side */
        phy_info.lane_map = line_lane_map;
        phy_info.if_side  = BCM_LINE_SIDE;

        retval = bcm_plp_mode_config_get(CHIP_NAME, phy_info, &speed_get, &if_type_get, &ref_clk_get, &if_mode_get, (void *)&aux_mode_get);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to get mode configuration parameters for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {

            printf("%s_SIDE: plp_Mode config get phy_id = %d,port_map 0x%x Interface = %d, speed = %d if_mode:%d ref_clk:%d\n",
                    phy_info.if_side?"SYS":"LINE", phy_info.phy_addr, phy_info.lane_map, if_type_get, speed_get, if_mode_get, ref_clk_get);
            printf("plp_Mode config get lane_data_rate = %d modulation_mode = %d fec_mode_sel = %d\n",
                    aux_mode_get.lane_data_rate,aux_mode_get.modulation_mode,aux_mode_get.fec_mode_sel);
            printf("plp_Mode config get port_type = %d octal_crossing = %d ing_fixed_latency.enable = %d egr_fixed_latency.enable = %d\n\n",
                    aux_mode_get.port_type,aux_mode_get.octal_crossing,aux_mode_get.ing_fixed_latency.enable,aux_mode_get.egr_fixed_latency.enable);
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training status at System side                                      *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        sys_lane_map = sys_lane_map_list[lane_map_index];
        phy_info.if_side  = BCM_SYSTEM_SIDE;
        phy_info.lane_map = sys_lane_map;
        training_ena_dis  = DISABLE ;
        retval = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to get training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("Training enable/disable status for lane-map 0x%x of PHY-%d at System side as below :\n",
            phy_info.lane_map, phy_info.phy_addr);
            printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
        }

        training_ena_dis = DISABLE;
        training_failure = TRAINING_FAILURE;
        training_status  = UNTRAINED;

        retval = bcm_plp_force_tx_training_status_get(CHIP_NAME, phy_info, &training_ena_dis, &training_failure, &training_status);
        if (retval != TEST_SUCCESS) {
          printf("FAIL: Failed to get training status for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
          phy_info.lane_map, phy_info.phy_addr, retval);
          goto _aperta2_config_error;
        } else {
            printf("Training status for lane-map 0x%x of PHY-%d at System side as below :\n", phy_info.lane_map, phy_info.phy_addr);
            printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
            printf("training_failure        = %s\n", training_failure ? "TRAINING_FAILURE" : "TRAINING_COMPLETE");
            printf("training_status         = %s\n", training_status ? "TRAINED" : "UNTRAINED");
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training status at Line side                                        *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        phy_info.if_side  = BCM_LINE_SIDE;
        phy_info.lane_map = line_lane_map;
        training_ena_dis  = DISABLE ;
        retval = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to get training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("Training enable/disable status for lane-map 0x%x of PHY-%d at Line side as below :\n",
            phy_info.lane_map, phy_info.phy_addr);
            printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
        }

        training_ena_dis = DISABLE;
        training_failure = TRAINING_FAILURE;
        training_status  = UNTRAINED;

        retval = bcm_plp_force_tx_training_status_get(CHIP_NAME, phy_info, &training_ena_dis, &training_failure, &training_status);
        if (retval != TEST_SUCCESS) {
          printf("FAIL: Failed to get training status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
          phy_info.lane_map, phy_info.phy_addr, retval);
          goto _aperta2_config_error;
        } else {
            printf("Training status for lane-map 0x%x of PHY-%d at Line side as below :\n", phy_info.lane_map, phy_info.phy_addr);
            printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
            printf("training_failure        = %s\n", training_failure ? "TRAINING_FAILURE" : "TRAINING_COMPLETE");
            printf("training_status         = %s\n", training_status ? "TRAINED" : "UNTRAINED");
        }
    }

    /* Getting link status on SYS and LINE side */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        sys_lane_map  = sys_lane_map_list[lane_map_index];
        for (side = BCM_SYSTEM_SIDE; side >= BCM_LINE_SIDE; side--) {
            phy_info.if_side  = side;
            phy_info.lane_map = (side == BCM_SYSTEM_SIDE) ? sys_lane_map : line_lane_map ;
            retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == BCM_SYSTEM_SIDE) ? "System" : "Line", retval);
                goto _aperta2_config_error;
            }
            retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == BCM_SYSTEM_SIDE) ? "System" : "Line", retval);
                goto _aperta2_config_error;
            }
            printf("Link status for lane-map 0x%x of PHY-%d at %s side is : %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, (side == BCM_SYSTEM_SIDE) ? "System" : "Line", link_status_get);
            /* Validate the link status */
            if (LINK_ON != link_status_get) {
                retval = TEST_FAILURE;
                printf("Link status for lane-map 0x%x of PHY-%d at %s side is not as expected (expected: %d, actual: %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == BCM_SYSTEM_SIDE) ? "System" : "Line", LINK_ON, link_status_get);
                goto _aperta2_config_error;
            }
        }
    }

_aperta2_config_error:
    /* -------------------------------------------------------------------------------- *
     * PHY and MAC Cleanup                                                              *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    phy_info.if_side  = BCM_LINE_SIDE;
    memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    retval = bcm_plp_mac_cleanup(CHIP_NAME, mac_info);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to cleanup mac for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
    }
    retval = bcm_plp_cleanup(CHIP_NAME, phy_info);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to cleanup PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
    }

_aperta2_init_error:
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
