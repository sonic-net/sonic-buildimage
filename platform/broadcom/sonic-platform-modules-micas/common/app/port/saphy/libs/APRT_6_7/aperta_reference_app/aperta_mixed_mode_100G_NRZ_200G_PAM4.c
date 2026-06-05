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

/* -------------------------------------------------------------------------------- *
 *                         Aperta reference application                             *
 * -------------------------------------------------------------------------------- *
 * This application configures Aperta chip mixed mode of 100G NRZ and 200G PAM4     *
 * macsec static bypass mode as follows:                                            *
 *   1. Connect to the board.                                                       *
 *   2. Initialize the PHY and download the firmware using broadcast method         *
 *   3. Initialize MACsec in static bypass mode                                     *
 *   4. Configure Tx/Rx polarity                                                    *
 *   5. Apply lane swap (if required)                                               *
 *   6. Configure operating mode parameters for 100G NRZ mode in lanemap 0x0F       *
 *   7. Setup link training                                                         *
 *   8. Check link status                                                           *
 *   9. Configure operating mode parameters for 200G PAM4 mode in lanemap 0xF0       *
 *   10.Setup link training                                                         *
 *   11.Check link status                                                           *
 *   12.Cleanup and close the connection to the board.                              *
 * -------------------------------------------------------------------------------- *
 * Note 1 : This sample application is only for 81343 parts.                        *
 * -------------------------------------------------------------------------------- */

#define OP_MODE_STR    "100G NRZ and 200G PAM4 mixed mode demonstration"

#define MAX_FLEXING_ITERATION   2

/* Enable Tx forced training */
#define LINK_TRAINING_EN

#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1

/*--------------------------------- First Set --------------------------------------*/
/* System side parameters */
#define SYS_PORT_SPEED_4X_1          100000
#define SYS_LANE_RATE_4X_1           bcmpLplaneDataRate_25P78125G
#define SYS_MODULATION_4X_1          bcmplpModulationNRZ
#define SYS_FEC_MODE_4X_1            bcmplpapertaNoFEC
#define SYS_IF_TYPE_4X_1             bcm_pm_InterfaceKR
#define SYS_IF_MODE_4X_1             0
#define SYS_PORT_TYPE_4X_1           bcmplpPortTypePassthrough

unsigned int sys_lane_map_list_4x_1[] = {
                                          0x0F
                                        };

/* Line side parameters */
#define LINE_PORT_SPEED_4X_1         100000
#define LINE_LANE_RATE_4X_1          bcmpLplaneDataRate_25P78125G
#define LINE_MODULATION_4X_1         bcmplpModulationNRZ
#define LINE_FEC_MODE_4X_1           bcmplpapertaNoFEC
#define LINE_IF_TYPE_4X_1            bcm_pm_InterfaceKR
#define LINE_IF_MODE_4X_1            0
#define LINE_PORT_TYPE_4X_1          bcmplpPortTypePassthrough

unsigned int line_lane_map_list_4x_1[] ={
                                          0x0F
                                        };
/*--------------------------------- Second Set -------------------------------------*/
/* System side parameters */
#define SYS_PORT_SPEED_4X_2          200000
#define SYS_LANE_RATE_4X_2           bcmpLplaneDataRate_53P125G
#define SYS_MODULATION_4X_2          bcmplpModulationPAM4
#define SYS_FEC_MODE_4X_2            bcmplpapertaRS544
#define SYS_IF_TYPE_4X_2             bcm_pm_InterfaceKR
#define SYS_IF_MODE_4X_2             0
#define SYS_PORT_TYPE_4X_2           bcmplpPortTypePassthrough

unsigned int sys_lane_map_list_4x_2[] = {
                                          0xF0
                                        };

/* Line side parameters */
#define LINE_PORT_SPEED_4X_2         200000
#define LINE_LANE_RATE_4X_2          bcmpLplaneDataRate_53P125G
#define LINE_MODULATION_4X_2         bcmplpModulationPAM4
#define LINE_FEC_MODE_4X_2           bcmplpapertaRS544
#define LINE_IF_TYPE_4X_2            bcm_pm_InterfaceKR
#define LINE_IF_MODE_4X_2            0
#define LINE_PORT_TYPE_4X_2          bcmplpPortTypePassthrough

unsigned int line_lane_map_list_4x_2[] ={
                                          0xF0
                                        };
/*----------------------------------------------------------------------------------*/
/* Main entry to application */
int main(int argc, char *argv[])
{
    int retval = TEST_SUCCESS;
    int flex_cnt = 0;
    int p_ctxt = 5;
    int phy_index = 0;
    int side = 0;
    int lane_map_index;
    int total_lane_maps;

    unsigned int link_status_get;
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;

    bcm_plp_mac_access_t mac_info;
    bcm_plp_access_t phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_aperta_fw_init_params_t aperta_init_params;
    bcm_plp_mac_flow_control_t flow_option;
    bcm_laneswap_map_t sys_laneswap_map, line_laneswap_map;
    bcm_plp_aperta_device_aux_modes_t aux_mode_set;

    int sys_port_speed ;
    int line_port_speed ;
    int sys_if_type;
    int line_if_type;
    int sys_if_mode;
    int line_if_mode;
    int sys_lane_rate;
    int line_lane_rate;
    int sys_modulation;
    int line_modulation;
    int sys_fec_mode;
    int line_fec_mode;
    int sys_port_type;
    int line_port_type;
    unsigned int *line_lane_map_list;
    unsigned int *sys_lane_map_list;

#ifdef LINK_TRAINING_EN
    unsigned int training_ena_dis = DISABLE;
    unsigned int training_failure = TRAINING_FAILURE;
    unsigned int training_status  = UNTRAINED;
#endif

    /* Initialize structure instances to zero */
    memset(&mac_info, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    memset(&aperta_init_params,0, sizeof(bcm_plp_aperta_fw_init_params_t));
    memset(&sys_laneswap_map,  0, sizeof(bcm_laneswap_map_t));
    memset(&line_laneswap_map, 0, sizeof(bcm_laneswap_map_t));
    memset(&aux_mode_set,  0, sizeof(bcm_plp_aperta_device_aux_modes_t));

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

    for(flex_cnt=0; flex_cnt<MAX_FLEXING_ITERATION; flex_cnt++) {
        if(flex_cnt%2 == 0) {
            total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list_4x_1);
            sys_port_speed  = SYS_PORT_SPEED_4X_1;
            line_port_speed = LINE_PORT_SPEED_4X_1;
            sys_lane_rate   = SYS_LANE_RATE_4X_1;
            line_lane_rate  = LINE_LANE_RATE_4X_1;
            sys_modulation  = SYS_MODULATION_4X_1;
            line_modulation = LINE_MODULATION_4X_1;
            sys_fec_mode    = SYS_FEC_MODE_4X_1;
            line_fec_mode   = LINE_FEC_MODE_4X_1;
            sys_if_type     = SYS_IF_TYPE_4X_1;
            line_if_type    = LINE_IF_TYPE_4X_1;
            sys_if_mode     = SYS_IF_MODE_4X_1;
            line_if_mode    = LINE_IF_MODE_4X_1;
            sys_port_type   = SYS_PORT_TYPE_4X_1;
            line_port_type  = LINE_PORT_TYPE_4X_1;
            sys_lane_map_list  = sys_lane_map_list_4x_1;
            line_lane_map_list = line_lane_map_list_4x_1;
        } else {
            total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list_4x_2);
            sys_port_speed  = SYS_PORT_SPEED_4X_2;
            line_port_speed = LINE_PORT_SPEED_4X_2;
            sys_lane_rate   = SYS_LANE_RATE_4X_2;
            line_lane_rate  = LINE_LANE_RATE_4X_2;
            sys_modulation  = SYS_MODULATION_4X_2;
            line_modulation = LINE_MODULATION_4X_2;
            sys_fec_mode    = SYS_FEC_MODE_4X_2;
            line_fec_mode   = LINE_FEC_MODE_4X_2;
            sys_if_type     = SYS_IF_TYPE_4X_2;
            line_if_type    = LINE_IF_TYPE_4X_2;
            sys_if_mode     = SYS_IF_MODE_4X_2;
            line_if_mode    = LINE_IF_MODE_4X_2;
            sys_port_type   = SYS_PORT_TYPE_4X_2;
            line_port_type  = LINE_PORT_TYPE_4X_2;
            sys_lane_map_list  = sys_lane_map_list_4x_2;
            line_lane_map_list = line_lane_map_list_4x_2;
        }

        printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
        printf("\n Configure switch in required speed :\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* -------------------------------------------------------------------------------- *
         * Section 3: Mode configuration                                                    *
         * -------------------------------------------------------------------------------- */
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
            }
        }

        /* -------------------------------------------------------------------------------- *
         * It is required to configure SYSTEM side first followed by LINE side              *
         * -------------------------------------------------------------------------------- */
        for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
            phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
            for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
                sys_lane_map = sys_lane_map_list[lane_map_index];
                line_lane_map = line_lane_map_list[lane_map_index];
                for(side = SYS_SIDE; side >= LINE_SIDE; side--) {
                    memset(&aux_mode_set, 0, sizeof(bcm_plp_aperta_device_aux_modes_t));
                    phy_info.if_side  = (side == SYS_SIDE) ? SYS_SIDE : LINE_SIDE;
                    phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map;
                    aux_mode_set.lane_data_rate  = (side == SYS_SIDE) ? sys_lane_rate  : line_lane_rate  ;
                    aux_mode_set.modulation_mode = (side == SYS_SIDE) ? sys_modulation : line_modulation ;
                    aux_mode_set.fec_mode_sel    = (side == SYS_SIDE) ? sys_fec_mode   : line_fec_mode   ;
                    aux_mode_set.port_type       = (side == SYS_SIDE) ? sys_port_type  : line_port_type  ;
                    retval = bcm_plp_mode_config_set( CHIP_NAME, phy_info,
                                                      (side == SYS_SIDE) ? sys_port_speed : line_port_speed,
                                                      (side == SYS_SIDE) ? sys_if_type    : line_if_type,
                                                      REF_CLOCK,
                                                      (side == SYS_SIDE) ? sys_if_mode    : line_if_mode,
                                                      (void *)&aux_mode_set
                                                    );

                    if (retval != TEST_SUCCESS) {
                        printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
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

        sleep(5);

        /* -------------------------------------------------------------------------------- *
         * Get Force Tx training status                                                     *
         * -------------------------------------------------------------------------------- */
        for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
            phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
            for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
                sys_lane_map  = sys_lane_map_list[lane_map_index];
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
         * Ports are setup now and ready to test with traffic                               *
         * -------------------------------------------------------------------------------- */
        printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("MacSec is in static bypass mode\n");
        printf("Ports are setup now and ready to test with traffic\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    }
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
