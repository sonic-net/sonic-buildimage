/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 *
 * Note: INITIALIZE/MEMSET EVERY FUNCTION PARAMETERS TO "0"
 *       TO AVOID FUTURE COMPATIBILITY ISSUES WITH THE DRIVER.
 */

/* Includes */
#include <barchetta_common.h>
#include <barchetta_config.h>

/* --------------------------------------------------------------------------------------- *
 *                         Barchetta reference application                                 *
 * --------------------------------------------------------------------------------------- *
 * This sample application demonstrate the usage of System side Force Tx Training for PAM4 *
 *   1  - Connect to the board.                                                            *
 *   2  - Initialize the PHY and download the firmware.                                    *
 *   3  - Setup logical lane-map for Tx/Rx at System and Line side.                        *
 *   4  - Configure operating mode parameters.                                             *
 *   [Note : 1 to 4 is test setup specific steps.                                          *
 *   5  - Configure PRBS generator (Tx) at Line side                                       *
 *   6  - Configure PRBS checker (Rx) at System and Line side                              *
 *   7  - Enable Force Tx Training at System side for all the ports                        *                                                                                                                                                                   *
 *   8  - Check the Force Tx Training status at System side for all the ports              *
 *   9  - Check for link status, PRBS lock and Rx PMD lock on both sides after training    *
 *   10 - Disable Tx/Rx PRBS at both System and Line side.                                 *
 *   11 - Cleanup and close the connection to the board.                                   *
 * --------------------------------------------------------------------------------------- */

#define OP_MODE_STR             "100G PAM4 (2x51.5625G) Retimer - System Side Force Tx Training Demonstration"

#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1

/* System side parameters */
#define SYS_PORT_SPEED          100000
#define SYS_LANE_RATE           bcmpLplaneDataRate_51P5625G
#define SYS_MODULATION          bcmplpModulationPAM4
#define SYS_FEC_MODE            bcmplpNoFEC
#define SYS_IF_TYPE             bcm_pm_InterfaceKR
#define SYS_IF_MODE             0
#define SYS_LL_MODE             LL_MODE_EN_BOTH_PATH
#define SYS_CLOCK_MODE          CLK_MODE_REC_CLK
#define SYS_FAILOVER_LANEMAP    0x00
#define SYS_NUM_LANES           2

unsigned int sys_lane_map_list[] = {
                                      0x03,
                                      0x0C,
                                      0x30,
                                      0xC0
                                   };

/* Line side parameters */
#define LINE_PORT_SPEED         100000
#define LINE_LANE_RATE          bcmpLplaneDataRate_51P5625G
#define LINE_MODULATION         bcmplpModulationPAM4
#define LINE_FEC_MODE           bcmplpNoFEC
#define LINE_IF_TYPE            bcm_pm_InterfaceKR
#define LINE_IF_MODE            0
#define LINE_LL_MODE            LL_MODE_EN_BOTH_PATH
#define LINE_CLOCK_MODE         CLK_MODE_REC_CLK
#define LINE_FAILOVER_LANEMAP   0x00
#define LINE_NUM_LANES          2
unsigned int line_lane_map_list[] = {
                                      0x03,
                                      0x0C,
                                      0x30,
                                      0xC0
                                    };

/* Main entry to application */
int main(void)
{
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int phy_index;
    int lane_map_index;
    int total_lane_maps;
    int time_val;

    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;
    unsigned int link_status;
    unsigned int prbs_lock;
    unsigned int prbs_lock_loss;
    unsigned int error_count;
    unsigned int pmd_lock_status;
    unsigned int training_ena_dis = DISABLE;
    unsigned int training_failure = TRAINING_FAILURE;
    unsigned int training_status  = UNTRAINED;

    bcm_plp_access_t  phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_barchetta_device_aux_modes_t aux_mode_sys;
    bcm_plp_barchetta_device_aux_modes_t aux_mode_line;

    /* Clear the structure instances */
    memset(&phy_info,      0, sizeof(bcm_plp_access_t));
    memset(&aux_mode_sys,  0, sizeof(bcm_plp_barchetta_device_aux_modes_t));
    memset(&aux_mode_line, 0, sizeof(bcm_plp_barchetta_device_aux_modes_t));
    memset(&firmware_load_type, 0, sizeof(bcm_plp_firmware_load_type_t));
    phy_info.platform_ctxt = &p_ctxt;

    /* -------------------------------------------------------------------------------- *
     * Connect to the board                                                             *
     * -------------------------------------------------------------------------------- */
    retval = device_open();
    if (retval != TEST_SUCCESS) {
        printf("Failed to connect to the board (ret = %d)!\n", retval);
        return retval;
    }

    /* -------------------------------------------------------------------------------- *
     * Initialize the PHYs and download firmware based on the test setup                *
     * -------------------------------------------------------------------------------- */
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.firmware_loader = NULL; /* Call back function registration */
    for (phy_index = 0; phy_index <  NUM_OF_PHY ; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        printf("Initializing PHY-%d with loading internal firmware...\n", phy_info.phy_addr);
        retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastNone);
        if (retval != TEST_SUCCESS) {
            printf("Failed to initialize PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _barchetta_reference_app_error;
        }

        /* Test for firmware info */
        retval = bcm_plp_firmware_info_get(CHIP_NAME, phy_info, &fw_ver, &fw_crc);
        if (retval != TEST_SUCCESS) {
            printf("Failed to get the firmware info for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _barchetta_reference_app_error;
        }
        printf("Firmware info for PHY-%d: FW version 0x%x, FW CRC 0x%x\n", phy_info.phy_addr, fw_ver, fw_crc);
    }

    /* -------------------------------------------------------------------------------- *
     * Setup logcial lane-map for Tx/Rx at System and Line side                         *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;

        /* Set logical lane-map at System side */
        phy_info.if_side = SYS_SIDE;
        retval = bcm_plp_logical_lane_set(CHIP_NAME, phy_info, sys_logical_lane_map[phy_index]);
        if (retval != TEST_SUCCESS) {
            printf("Failed to set logical lane-map for PHY-%d at System side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _barchetta_reference_app_error;
        }

        /* Set logical lane-map at Line side */
        phy_info.if_side = LINE_SIDE;
        retval = bcm_plp_logical_lane_set(CHIP_NAME, phy_info, line_logical_lane_map[phy_index]);
        if (retval != TEST_SUCCESS) {
            printf("Failed to set logical lane-map for PHY-%d at Line side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _barchetta_reference_app_error;
        }
    }

    /* Check for Sys and Line lane-map from config */
    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    if (total_lane_maps != NUM_ARR_ELEMENTS(line_lane_map_list)) {
        retval = TEST_FAILURE;
        printf("Config for Sys and Line lane-map array is of different size!\n");
        goto _barchetta_reference_app_error;
    }

    /* -------------------------------------------------------------------------------- *
     * Configure operating mode parameters                                              *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

            /* Set mode configuration at System side */
            phy_info.lane_map = sys_lane_map;
            phy_info.if_side  = SYS_SIDE;

            aux_mode_sys.lane_data_rate     = SYS_LANE_RATE;
            aux_mode_sys.modulation_mode    = SYS_MODULATION;
            aux_mode_sys.clock_mode         = SYS_CLOCK_MODE;
            aux_mode_sys.ll_mode            = SYS_LL_MODE;                  /* Must be 0x3 for non-FEC */
            aux_mode_sys.failover_lane_map  = SYS_FAILOVER_LANEMAP;         /* Must be 0x0 */

            retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, SYS_PORT_SPEED, SYS_IF_TYPE, REF_CLOCK, SYS_IF_MODE, (void *)&aux_mode_sys);

            if (retval != TEST_SUCCESS) {
                printf("Failed to set mode configuration for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }

            /* Set mode configuration at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side  = LINE_SIDE;

            aux_mode_line.lane_data_rate    = LINE_LANE_RATE;
            aux_mode_line.modulation_mode   = LINE_MODULATION;
            aux_mode_line.clock_mode        = LINE_CLOCK_MODE;
            aux_mode_line.ll_mode           = LINE_LL_MODE;                 /* Must be 0x3 for non-FEC */
            aux_mode_line.failover_lane_map = LINE_FAILOVER_LANEMAP;        /* Must be 0x0 */

            retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, LINE_PORT_SPEED, LINE_IF_TYPE, REF_CLOCK, LINE_IF_MODE, (void *)&aux_mode_line);

            if (retval != TEST_SUCCESS) {
                printf("Failed to set mode configuration for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Configure PRBS generator (Tx) at Line side                                       *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map    = line_lane_map_list[lane_map_index];

            /* Set PRBS generator (Tx) configuration at System side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side  = LINE_SIDE;
            retval = bcm_plp_prbs_set(CHIP_NAME, phy_info, PRBS_CONFIG_TX, PRBS_PATTERN, INVERT_OFF, PRBS_NO_LOOPBACK, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("Failed to set PRBS generator (Tx) with polynomial %d for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                PRBS_PATTERN, phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
        }
    }
    sleep(3);
    /* -------------------------------------------------------------------------------- *
     * Configure PRBS checker (Rx)                                                      *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

            /* Set PRBS checker (Rx) configuration at System side */
            phy_info.lane_map = sys_lane_map;
            phy_info.if_side = SYS_SIDE;
            retval = bcm_plp_prbs_set(CHIP_NAME, phy_info, PRBS_CONFIG_RX, PRBS_PATTERN, INVERT_OFF, PRBS_NO_LOOPBACK, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("Failed to set PRBS checker (Rx) with polynomial %d for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                PRBS_PATTERN, phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }

            /* Set PRBS checker (Rx) configuration at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side = LINE_SIDE;
            retval = bcm_plp_prbs_set(CHIP_NAME, phy_info, PRBS_CONFIG_RX, PRBS_PATTERN, INVERT_OFF, PRBS_NO_LOOPBACK, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("Failed to set PRBS checker (Rx) with polynomial %d for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                PRBS_PATTERN, phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
        }
    }
    sleep(3);
    /* -------------------------------------------------------------------------------- *
     * Start Force Tx training at System side                                           *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            phy_info.if_side = SYS_SIDE;
            phy_info.lane_map = sys_lane_map ;
            retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("Failed to enable training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, retval);
               goto _barchetta_reference_app_error;
            }
        }
    }
    sleep(10);
    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training status at System side                                      *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            phy_info.if_side = SYS_SIDE;
            phy_info.lane_map = sys_lane_map ;
            training_ena_dis = DISABLE ;
            retval = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
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
              printf("Failed to get training status for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
              phy_info.lane_map, phy_info.phy_addr, retval);
              goto _barchetta_reference_app_error;
            } else {
                printf("Training status for lane-map 0x%x of PHY-%d at System side as below :\n", phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
                printf("training_failure        = %s\n", training_failure ? "TRAINING_FAILURE" : "TRAINING_COMPLETE");
                printf("training_status         = %s\n", training_status ? "TRAINED" : "UNTRAINED");
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Check for link status, PRBS lock and Rx PMD lock                                 *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];
            time_val = 0;

            /* Check for link status at System side */
            phy_info.lane_map = sys_lane_map;
            phy_info.if_side = SYS_SIDE;
            retval = bcm_plp_link_status_get(CHIP_NAME, phy_info, &link_status);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get link status for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
            /* Validate the link status */
            if (LINK_ON != link_status) {
                retval = TEST_FAILURE;
                printf("Link status for lane-map 0x%x of PHY-%d at System side is not as expected (expected: %d, actual: %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, LINK_ON, link_status);
                goto _barchetta_reference_app_error;
            }

            /* Get PRBS checker (Rx) status at System side */
            retval = bcm_plp_prbs_rx_stat(CHIP_NAME, phy_info, time_val);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get PRBS checker (Rx) status for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }

            /* Get PRBS checker (Rx) lock status at System side */
            retval = bcm_plp_prbs_status_get(CHIP_NAME, phy_info, &prbs_lock, &prbs_lock_loss, &error_count);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get PRBS checker (Rx) lock status for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
            /* Validate the PRBS lock status */
            if (PRBS_LOCK != prbs_lock) {
                retval = TEST_FAILURE;
                printf("FAIL: PRBS lock for lane-map 0x%x of PHY-%d at System side is not as expected (expected: %d, actual: %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, PRBS_LOCK, prbs_lock);
            }

            /* Get Rx PMD lock status at System side */
            retval = bcm_plp_rx_pmd_lock_get(CHIP_NAME, phy_info, &pmd_lock_status);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get Rx PMD live lock status for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
            /* Validate the PMD lock status */
            if (PMD_LOCK != pmd_lock_status) {
                retval = TEST_FAILURE;
                printf("FAIL: Rx PMD live lock status for lane-map 0x%x of PHY-%d at System side is not as expected (expected: %d, actual: %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, PMD_LOCK, pmd_lock_status);
                goto _barchetta_reference_app_error;
            }

            /* Check for link status at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side = LINE_SIDE;
            retval = bcm_plp_link_status_get(CHIP_NAME, phy_info, &link_status);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get link status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
            /* Validate the link status */
            if (LINK_ON != link_status) {
                retval = TEST_FAILURE;
                printf("FAIL: Link status for lane-map 0x%x of PHY-%d at Line side is not as expected (expected: %d, actual: %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, LINK_ON, link_status);
                goto _barchetta_reference_app_error;
            }

            /* Get PRBS checker (Rx) status at Line side */
            retval = bcm_plp_prbs_rx_stat(CHIP_NAME, phy_info, time_val);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get PRBS checker (Rx) status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }

            /* Get PRBS checker (Rx) lock status at Line side */
            retval = bcm_plp_prbs_status_get(CHIP_NAME, phy_info, &prbs_lock, &prbs_lock_loss, &error_count);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get PRBS checker (Rx) lock status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
            /* Validate the PRBS lock status */
            if (PRBS_LOCK != prbs_lock) {
                retval = TEST_FAILURE;
                printf("FAIL: PRBS lock for lane-map 0x%x of PHY-%d at Line side is not as expected (expected: %d, actual: %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, PRBS_LOCK, prbs_lock);
            }

            /* Get Rx PMD lock status at Line side */
            retval = bcm_plp_rx_pmd_lock_get(CHIP_NAME, phy_info, &pmd_lock_status);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get Rx PMD live lock status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
            /* Validate the PMD lock status */
            if (PMD_LOCK != pmd_lock_status) {
                retval = TEST_FAILURE;
                printf("FAIL: Rx PMD live lock status for lane-map 0x%x of PHY-%d at Line side is not as expected (expected: %d, actual: %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, PMD_LOCK, pmd_lock_status);
                goto _barchetta_reference_app_error;
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Disable Tx/Rx PRBS at both Sys and Line side                                     *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

            /* Disable Tx/Rx PRBS at System side */
            phy_info.lane_map = sys_lane_map;
            phy_info.if_side = SYS_SIDE;
            retval = bcm_plp_prbs_clear(CHIP_NAME, phy_info, PRBS_CONFIG_TX_RX);
            if (retval != TEST_SUCCESS) {
                printf("Failed to disable Tx/Rx PRBS for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }

            /* Disable Tx/Rx PRBS at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side = LINE_SIDE;
            retval = bcm_plp_prbs_clear(CHIP_NAME, phy_info, PRBS_CONFIG_TX_RX);
            if (retval != TEST_SUCCESS) {
                printf("Failed to disable Tx/Rx PRBS for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
        }
    }

_barchetta_reference_app_error:
    /* Check for test result */
    if (retval == TEST_SUCCESS) {
        printf("Test for %s mode completed successfully\n", OP_MODE_STR);
    } else {
        printf("Test for %s mode failed!\n", OP_MODE_STR);
    }

    /* -------------------------------------------------------------------------------- *
     * PHY Cleanup                                                                      *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side = LINE_SIDE;
        retval = bcm_plp_cleanup(CHIP_NAME, phy_info);
        if (retval != TEST_SUCCESS) {
            printf("Failed to cleanup PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Close the connections to the board                                               *
     * -------------------------------------------------------------------------------- */
    device_close();

    /* Return with test status */
    return retval;
}
