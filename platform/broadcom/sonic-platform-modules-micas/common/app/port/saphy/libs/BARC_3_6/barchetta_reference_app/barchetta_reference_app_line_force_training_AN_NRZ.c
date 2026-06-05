/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 *
 * Note: INITIALIZE/MEMSET EVERY FUNCTION PARAMETERS TO "0",
 *       TO AVOID FUTURE COMPATIBILITY ISSUES WITH THE DRIVER.
 */

/* Includes */
#include <barchetta_common.h>
#include <barchetta_config.h>

/* --------------------------------------------------------------------------------------- *
 *                         Barchetta reference application                                 *
 * --------------------------------------------------------------------------------------- *
 * This sample application demonstrate the usage of Line side Force Tx Training/AN API's   *
 *   1  - Connect to the board                                                             *
 *   2  - Initialize the PHY and download the firmware.                                    *
 *   3  - Setup logical lane-map for Tx/Rx at System and Line side.                        *
 *   4  - Configure operating mode parameters.                                             *
 *   [Note : 1 to 4 is test setup specific steps.                                          *
 * #ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE                                                 *
 *   5  - Configure PRBS generator (Tx) at System side                                     *
 *   6  - Configure PRBS checker (Rx) at System and Line side                              *
 * #endif                                                                                  *
 * #ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE                                                 *
 *   7  - Enable Force Tx Training at Line side for all the ports                          *
 * #else                                                                                   *
 *   7  - Set tech and FEC ability and Enable AN (with training) at Line side for all ports*
 * #endif                                                                                  *
 * #ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE                                                 *
 *   8  - Check the Force Tx Training status at Line side for all the ports                *
 * #else                                                                                   *
 *   9  - Check the AN status at Line side for all the ports                               *
 *   10 - Check FEC enable status                                                          *
 * #endif                                                                                  *
 * #ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE                                                 *
 *   11 - Check for link status and Rx PMD lock on both sides after Training/AN            *
 *   12 - Disable Tx/Rx PRBS at both System and Line side.                                 *
 * #endif                                                                                  *
 *   13 - Cleanup and close the connection to the board.                                   *
 * --------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------- *
 *  By default this app demonstrate the usage of AN API's.                                 *
 *  For force Tx training, enable ADD_FLAGS "BCM_PLP_FORCE_TX_TRAINING_ENABLE" in Makefile *
 * --------------------------------------------------------------------------------------- */


#ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE
#define OP_MODE_STR             "40G NRZ (4x10G) Retimer - Force Tx Training Demonstration (Line Side)"
#else
#define OP_MODE_STR             "40G NRZ (4x10G) Retimer - AN Demonstration (Line Side)"
#endif

#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1

/* System side parameters */
#define SYS_PORT_SPEED          40000
#define SYS_LANE_RATE           bcmpLplaneDataRate_10P3125G
#define SYS_MODULATION          bcmplpModulationNRZ
#define SYS_IF_TYPE             bcm_pm_InterfaceKR
#define SYS_IF_MODE             0
#define SYS_LL_MODE             LL_MODE_EN_BOTH_PATH
#define SYS_CLOCK_MODE          CLK_MODE_REC_CLK
#define SYS_FAILOVER_LANEMAP    0x00
#define SYS_NUM_LANES           4

unsigned int sys_lane_map_list[] = {
                                      0x0f,
                                      0xf0
                                   };

/* Line side parameters */
#define LINE_PORT_SPEED         40000
#define LINE_LANE_RATE          bcmpLplaneDataRate_10P3125G
#define LINE_MODULATION         bcmplpModulationNRZ
#define LINE_IF_TYPE            bcm_pm_InterfaceKR
#define LINE_IF_MODE            0
#define LINE_LL_MODE            LL_MODE_EN_BOTH_PATH
#define LINE_CLOCK_MODE         CLK_MODE_REC_CLK
#define LINE_FAILOVER_LANEMAP   0x00
#define LINE_NUM_LANES          4
unsigned int line_lane_map_list[] = {
                                      0x0f,
                                      0xf0
                                    };

/* Main entry to application */
int main(void)
{
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int phy_index;
    int lane_map_index;
    int total_lane_maps;
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;
#ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE
    unsigned int link_status;
    unsigned int pmd_lock_status;
    unsigned int training_ena_dis = DISABLE;
    unsigned int training_failure = TRAINING_FAILURE;
    unsigned int training_status  = UNTRAINED;
#else
    unsigned int an_ena_dis = DISABLE ;
    unsigned int an_done = 0;
    unsigned int fec_get = 0;
    unsigned short pause_ability_set  = 0 ;
    unsigned short pause_ability_get  = 0 ;
    unsigned short tech_ability_set   = 0 ;
    unsigned short tech_ability_get   = 0 ;
    unsigned short fec_ability_set    = 0x02 ; /* CL74 requested */
    unsigned short fec_ability_get    = 0 ;
#endif

#ifndef BCM_PLP_FORCE_TX_TRAINING_ENABLE
    bcm_plp_an_config_t an_config_set;
    bcm_plp_an_config_t an_config_get;
    memset(&an_config_set, 0, sizeof(bcm_plp_an_config_t));
    an_config_set.master_lane  = 0 ;
    an_config_set.cl72_en      = 1 ;
    an_config_set.tech_ability = bcmplpAnCap40G_KR4 ;
#endif

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
    printf("Total lane-maps = %d\n", total_lane_maps);

    /* -------------------------------------------------------------------------------- *
     * Configure operating mode parameters                                              *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];
            printf("System lane-map[%d] = 0x%x\n", lane_map_index, sys_lane_map);
            printf("Line lane-map[%d] = 0x%x\n", lane_map_index, line_lane_map);

            /* Set mode configuration at System side */
            phy_info.lane_map = sys_lane_map;
            phy_info.if_side  = SYS_SIDE;

            aux_mode_sys.lane_data_rate     = SYS_LANE_RATE;
            aux_mode_sys.modulation_mode    = SYS_MODULATION;
            aux_mode_sys.clock_mode         = SYS_CLOCK_MODE;
            aux_mode_sys.ll_mode            = SYS_LL_MODE;                  /* Must be 0x3 for non-FEC */
            aux_mode_sys.failover_lane_map  = SYS_FAILOVER_LANEMAP;         /* Must be 0x0 */

            printf("System side mode configuration parameters:\n");
            printf("Lane rate         = %d\n", aux_mode_sys.lane_data_rate);
            printf("Modulation mode   = %d\n", aux_mode_sys.modulation_mode);
            printf("Clock mode        = %d\n", aux_mode_sys.clock_mode);
            printf("Low-latency mode  = %d\n", aux_mode_sys.ll_mode);
            printf("Failover lane-map = %d\n", aux_mode_sys.failover_lane_map);
            printf("Port speed        = %d\n", SYS_PORT_SPEED);
            printf("Interface type    = %d\n", SYS_IF_TYPE);
            printf("Reference clock   = %d\n", REF_CLOCK);
            printf("Interface mode    = 0x%x\n", SYS_IF_MODE);

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

            printf("Line side mode configuration parameters:\n");
            printf("Lane rate         = %d\n", aux_mode_line.lane_data_rate);
            printf("Modulation mode   = %d\n", aux_mode_line.modulation_mode);
            printf("Clock mode        = %d\n", aux_mode_line.clock_mode);
            printf("Low-latency mode  = %d\n", aux_mode_line.ll_mode);
            printf("Failover lane-map = %d\n", aux_mode_line.failover_lane_map);
            printf("Port speed        = %d\n", LINE_PORT_SPEED);
            printf("Interface type    = %d\n", LINE_IF_TYPE);
            printf("Reference clock   = %d\n", REF_CLOCK);
            printf("Interface mode    = 0x%x\n", LINE_IF_MODE);

            retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, LINE_PORT_SPEED, LINE_IF_TYPE, REF_CLOCK, LINE_IF_MODE, (void *)&aux_mode_line);

            if (retval != TEST_SUCCESS) {
                printf("Failed to set mode configuration for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
        }
    }
	
#ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE
    /* -------------------------------------------------------------------------------- *
     * Configure PRBS generator (Tx) at System side                                     *
     * -------------------------------------------------------------------------------- */
    printf("PRBS configuration parameters:\n");
    printf("PRBS pattern  = %d\n", PRBS_PATTERN);
    printf("Inversion     = %d\n", INVERT_OFF);
    printf("Loopback      = %d\n", PRBS_NO_LOOPBACK);
    printf("Enable        = %d\n", ENABLE);
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map    = sys_lane_map_list[lane_map_index];

            /* Set PRBS generator (Tx) configuration at System side */
            phy_info.lane_map = sys_lane_map;
            phy_info.if_side  = SYS_SIDE;
            retval = bcm_plp_prbs_set(CHIP_NAME, phy_info, PRBS_CONFIG_TX, PRBS_PATTERN, INVERT_OFF, PRBS_NO_LOOPBACK, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("Failed to set PRBS generator (Tx) with polynomial %d for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                PRBS_PATTERN, phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
            printf("Enabled PRBS generators for lane-map 0x%x of System side\n", phy_info.lane_map);
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
            printf("Enabled PRBS checkers for lane-map 0x%x of System side\n", phy_info.lane_map);

            /* Set PRBS checker (Rx) configuration at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side = LINE_SIDE;
            retval = bcm_plp_prbs_set(CHIP_NAME, phy_info, PRBS_CONFIG_RX, PRBS_PATTERN, INVERT_OFF, PRBS_NO_LOOPBACK, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("Failed to set PRBS checker (Rx) with polynomial %d for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                PRBS_PATTERN, phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
            printf("Enabled PRBS checkers for lane-map 0x%x of Line side\n", phy_info.lane_map);
        }
    }
#endif
    sleep(3);

    /* -------------------------------------------------------------------------------- *
     * Start Force Tx training / AN at line side                                             *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            phy_info.if_side = LINE_SIDE;
            phy_info.lane_map = line_lane_map ;
#ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE
            retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("Failed to enable training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, retval);
               goto _barchetta_reference_app_error;
            }
            printf("Force Tx training is in progress for lane map = 0x%x...\n", phy_info.lane_map);
#else /* AN */
            /* To verify FEC functionality PCS traffic to be send */
            printf("\nPlease send the PCS traffic for lane map = 0x%x...\n", phy_info.lane_map);
            retval = bcm_plp_cl73_ability_set(CHIP_NAME, phy_info, tech_ability_set, fec_ability_set, pause_ability_set, an_config_set);
            if (retval != TEST_SUCCESS) {
                printf("Failed to set cl73 ability for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, retval);
               goto _barchetta_reference_app_error;
            }

            retval = bcm_plp_cl73_set(CHIP_NAME, phy_info, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("Failed to enable AN for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, retval);
               goto _barchetta_reference_app_error;
            }
            printf("Auto negotiation (CL73) is in progress for lane map = 0x%x...\n", phy_info.lane_map);
#endif
        }
    }
    sleep(5);
    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training/AN status at Line side                                     *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            phy_info.if_side = LINE_SIDE;
            phy_info.lane_map = line_lane_map ;
#ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE
            training_ena_dis = DISABLE ;
            retval = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
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
              printf("Failed to get training status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
              phy_info.lane_map, phy_info.phy_addr, retval);
              goto _barchetta_reference_app_error;
            } else {
                printf("Training status for lane-map 0x%x of PHY-%d at Line side as below :\n", phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
                printf("training_failure        = %s\n", training_failure ? "TRAINING_FAILURE" : "TRAINING_COMPLETE");
                printf("training_status         = %s\n", training_status ? "TRAINED" : "UNTRAINED");
            }
#else /* AN */
            tech_ability_get  = 0 ;
            fec_ability_get   = 0 ;
            pause_ability_get = 0 ;
            memset(&an_config_get, 0, sizeof(bcm_plp_an_config_t));

            retval = bcm_plp_cl73_ability_get(CHIP_NAME, phy_info, &tech_ability_get, &fec_ability_get, &pause_ability_get, &an_config_get);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get cl73 ability for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            } else {
                printf("CL73 ability get parameters for lane-map 0x%x of PHY-%d at Line side as below :\n",
                phy_info.lane_map, phy_info.phy_addr);
                printf("fec_ability_get                 = %d\n"   , fec_ability_get);
                printf("pause_ability_get               = 0x%x\n" , pause_ability_get);
                printf("an_config_get.master_lane       = %d\n"   , an_config_get.master_lane);
                printf("an_config_get.cl72_en           = %d\n"   , an_config_get.cl72_en);
                printf("an_config_get.tech_ability      = 0x%x\n" , an_config_get.tech_ability);
            }

            an_ena_dis = DISABLE ;
            an_done    = 0;
            retval = bcm_plp_cl73_get(CHIP_NAME, phy_info, &an_ena_dis, &an_done);
            if (retval != TEST_SUCCESS) {
              printf("Failed to get AN status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
              phy_info.lane_map, phy_info.phy_addr, retval);
              goto _barchetta_reference_app_error;
            } else {
                printf("AN status for lane-map 0x%x of PHY-%d at Line side as below :\n", phy_info.lane_map, phy_info.phy_addr);
                printf("an_enable_disable = %s\n", an_ena_dis ? "AN_ENABLE" : "AN_DISABLE");
                printf("an_done           = %d\n", an_done);
            }

            /* Validate an_done */
            if (an_done != 1) {
                retval = TEST_FAILURE;
                printf("AN not done for lane-map 0x%x of PHY-%d at Line side!\n", phy_info.lane_map, phy_info.phy_addr);
                goto _barchetta_reference_app_error;
            }
			/* get force fec status */
            fec_get = 0x10000; /* get cl74 fec status */
            retval = bcm_plp_fec_enable_get(CHIP_NAME,phy_info, &fec_get);
            if (retval != TEST_SUCCESS) {
                printf("Failed to get FEC cl74 status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n", phy_info.lane_map, phy_info.phy_addr, retval);
                goto _barchetta_reference_app_error;
            }
            printf("FEC CL74 is %s for lane-map 0x%x of PHY-%d at Line side \n", (fec_get == 1 )? "enabled" : "disabled", phy_info.lane_map, phy_info.phy_addr);
#endif
        }
    }

#ifdef BCM_PLP_FORCE_TX_TRAINING_ENABLE
    /* -------------------------------------------------------------------------------- *
     * Check for link status and Rx PMD lock                                            *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

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
#endif
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
