/*
 * $Copyright: (c) 2021 Broadcom.
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
#include <aperta_common.h>
/* -------------------------------------------------------------------------------- *
 * HW Setup : System side connected to TH3 and Line side PHY-0 connected with PHY-1 *
 * -------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------- *
 *                         Aperta reference application                             *
 * -------------------------------------------------------------------------------- *
 * This sample application configures the Aperta chip in forward gearbox mode       *
 * (2x25G_NRZ --> 1x50G_PAM4) as follows:                                           *
 *   1. Connect to the board.                                                       *
 *   2. Initialize the PHY and download the firmware                                *
 *   3. Initialize MACsec in static bypass mode                                     *
 *   4. Configure Tx/Rx polarity                                                    *
 *   5. Configure lane swap accordingly                                             *
 *   6. Configure PHY-0 in forward gearbox mode                                     *
 *   7. Configure PHY-1 in retimer mode                                             *
 *   8. Setup link training                                                         *
 *   9. Check link status and send traffic                                          *
 *   10. Cleanup and close the connection to the board.                             *
 * -------------------------------------------------------------------------------- *
 * Note 1 : This sample application is only for 81343 parts.                        *
 * -------------------------------------------------------------------------------- */
#define OP_MODE_STR    "2x25G NRZ --> 1x50G PAM4 Gearbox Mode (Line side alternate lane)"

/* Enable Tx forced training */
#define LINK_TRAINING_EN

#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1
/*------------------------ PHY-0 configuration(GB Mode) ----------------------------*/
/* System side parameters */
#define SYS_PORT_SPEED_PHY0     50000
#define SYS_LANE_RATE_PHY0      bcmpLplaneDataRate_25P78125G
#define SYS_MODULATION_PHY0     bcmplpModulationNRZ
#define SYS_FEC_MODE_PHY0       bcmplpapertaNoFEC
#define SYS_IF_TYPE_PHY0        bcm_pm_InterfaceKR
#define SYS_IF_MODE_PHY0        0
#define SYS_PORT_TYPE           bcmplpPortTypeGearBox

unsigned int sys_lane_map_list_phy0[] = {
                                          0x03,
                                          0x0C,
                                          0x30,
                                          0xC0
                                        };

/* Line side parameters */
#define LINE_PORT_SPEED_PHY0    50000
#define LINE_LANE_RATE_PHY0     bcmpLplaneDataRate_53P125G
#define LINE_MODULATION_PHY0    bcmplpModulationPAM4
#define LINE_FEC_MODE_PHY0      bcmplpapertaRSFEC
#define LINE_IF_TYPE_PHY0       bcm_pm_InterfaceKR
#define LINE_IF_MODE_PHY0       0
#define LINE_PORT_TYPE          bcmplpPortTypeGearBox

unsigned int line_lane_map_list_phy0[] ={
                                          0x01,
                                          0x04,
                                          0x10,
                                          0x40
                                        };
/*---------------------------------------------------------------------------------------*/
/*------------------------ PHY-1 configuration(Retimer Mode) ----------------------------*/
/* System side parameters */
#define SYS_PORT_SPEED_PHY1     LINE_PORT_SPEED_PHY0
#define SYS_LANE_RATE_PHY1      LINE_LANE_RATE_PHY0
#define SYS_MODULATION_PHY1     LINE_MODULATION_PHY0
#define SYS_FEC_MODE_PHY1       LINE_FEC_MODE_PHY0
#define SYS_IF_TYPE_PHY1        LINE_IF_TYPE_PHY0
#define SYS_IF_MODE_PHY1        LINE_IF_MODE_PHY0
unsigned int sys_lane_map_list_phy1[]=  {
                                          0x02,
                                          0x08,
                                          0x20,
                                          0x80
                                        };

/* Line side parameters */
#define LINE_PORT_SPEED_PHY1    LINE_PORT_SPEED_PHY0
#define LINE_LANE_RATE_PHY1     LINE_LANE_RATE_PHY0
#define LINE_MODULATION_PHY1    LINE_MODULATION_PHY0
#define LINE_FEC_MODE_PHY1      LINE_FEC_MODE_PHY0
#define LINE_IF_TYPE_PHY1       LINE_IF_TYPE_PHY0
#define LINE_IF_MODE_PHY1       LINE_IF_MODE_PHY0

unsigned int line_lane_map_list_phy1[]= {
                                          0x02,
                                          0x08,
                                          0x20,
                                          0x80
                                        };
/*---------------------------------------------------------------------------------------*/

/* Main entry to application */
int main(int argc, char *argv[])
{
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int phy_index = 0;
    int side = 0;
    int lane_map_index;
    int total_lane_maps;
    int total_lane_maps_phy0;
    int total_lane_maps_phy1;
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
    bcm_plp_aperta_device_aux_modes_t aux_mode_set;

#ifdef LINK_TRAINING_EN
    unsigned int training_ena_dis = DISABLE;
    unsigned int training_failure = TRAINING_FAILURE;
    unsigned int training_status  = UNTRAINED;
#endif
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

    bcm_plp_laneswap_map_t sys_laneswap_map[] = {
        {
          CHIP_MAX_LANES,                       /* Total number of lanes */
          {{ 0,  1,  2,  3,  4,  5,  6,  7 }},  /* PHY-0 SYS : lane_map_rx[x]=y means that rx lane x is mapped to rx lane y */
           { 0,  1,  2,  3,  4,  5,  6,  7 }    /* PHY-0 SYS : lane_map_tx[x]=y means that tx lane x is mapped to tx lane y */
        },
        {
          CHIP_MAX_LANES,                       /* Total number of lanes */
          {{ 0,  1,  2,  3,  4,  5,  6,  7 }},  /* PHY-1 SYS : lane_map_rx[x]=y means that rx lane x is mapped to rx lane y */
           { 0,  1,  2,  3,  4,  5,  6,  7 }    /* PHY-1 SYS : lane_map_tx[x]=y means that tx lane x is mapped to tx lane y */
        }
    };

    bcm_plp_laneswap_map_t line_laneswap_map[]= {
        {
           CHIP_MAX_LANES,                       /* Total number of lanes */
           {{ 1,  0,  3,  2,  5,  4,  7,  6 }},  /* PHY-0 LINE : lane_map_rx[x]=y means that rx lane x is mapped to rx lane y */
            { 1,  0,  3,  2,  5,  4,  7,  6 }    /* PHY-0 LINE : lane_map_tx[x]=y means that tx lane x is mapped to tx lane y */
        },
        {
           CHIP_MAX_LANES,                       /* Total number of lanes */
           {{ 0,  1,  2,  3,  4,  5,  6,  7 }},  /* PHY-1 LINE : lane_map_rx[x]=y means that rx lane x is mapped to rx lane y */
            { 0,  1,  2,  3,  4,  5,  6,  7 }    /* PHY-1 LINE : lane_map_tx[x]=y means that tx lane x is mapped to tx lane y */
        }
    };

    /* Initialize structure instances to zero */
    memset(&mac_info, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));
    memset(&aperta_init_params,0, sizeof(bcm_plp_aperta_fw_init_params_t));

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
     * Initialize the PHYs and download firmware based on the test setup and            *
     * configure the macsec in static bypass mode                                       *
     * -------------------------------------------------------------------------------- */
    aperta_init_params.macsec_static_bypass  = 1;
    aperta_init_params.pll1_vco_rate = bcmplpVco26p562G;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.fw_init_params = &aperta_init_params;

    phy_info.if_side = 0;
    phy_info.lane_map = ALL_LANE_MAP;
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        printf("Initializing PHY-%d with loading internal firmware...\n", phy_info.phy_addr);
        retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastNone);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: bcm_plp_init_fw_bcast failed to initialize PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_init_error;
        }

        /* Read firmware info */
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

    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Board Polarity settings :\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    retval = polarity_swap();
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get and set initial Tx/Rx polarity with ret = %d\n", retval);
        goto _aperta_config_error;
    }

    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Set lane swap :\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        /* Set lane swap at System side */
        phy_info.if_side = SYS_SIDE;
        retval = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &sys_laneswap_map[phy_index]);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to set lane swap for PHY-%d at System side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at System side!\n", phy_info.phy_addr);
        }
        /* Set lane swap at Line side */
        phy_info.if_side = LINE_SIDE;
        retval = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &line_laneswap_map[phy_index]);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to set lane swap for PHY-%d at Line side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at Line side!\n", phy_info.phy_addr);
        }
    }

    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Mode configuration:\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    total_lane_maps_phy0 = NUM_ARR_ELEMENTS(sys_lane_map_list_phy0);
    total_lane_maps_phy1 = NUM_ARR_ELEMENTS(sys_lane_map_list_phy1);

    /* Enable flow control and configure 100G (4x25G) mode in NRZ, no FEC */
    /* flow_option = bcmplpFlowcontrolTerminateGenerate; */
    flow_option = bcmplpFlowcontrolPassthrough;
    memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        mac_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        mac_info.phy_info.if_side = LINE_SIDE;
        total_lane_maps   = (phy_index == 0) ? total_lane_maps_phy0 : total_lane_maps_phy1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map   = (phy_index == 0) ? line_lane_map_list_phy0[lane_map_index] : line_lane_map_list_phy1[lane_map_index];
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
        total_lane_maps   = (phy_index == 0) ? total_lane_maps_phy0 : total_lane_maps_phy1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map    = (phy_index == 0) ? sys_lane_map_list_phy0[lane_map_index]  : sys_lane_map_list_phy1[lane_map_index];
            line_lane_map   = (phy_index == 0) ? line_lane_map_list_phy0[lane_map_index] : line_lane_map_list_phy1[lane_map_index];
            sys_lane_rate   = (phy_index == 0) ? SYS_LANE_RATE_PHY0  : SYS_LANE_RATE_PHY1;
            line_lane_rate  = (phy_index == 0) ? LINE_LANE_RATE_PHY0 : LINE_LANE_RATE_PHY1;
            sys_modulation  = (phy_index == 0) ? SYS_MODULATION_PHY0 : SYS_MODULATION_PHY1;
            line_modulation = (phy_index == 0) ? LINE_MODULATION_PHY0: LINE_MODULATION_PHY1;
            sys_fec_mode    = (phy_index == 0) ? SYS_FEC_MODE_PHY0   : SYS_FEC_MODE_PHY1;
            line_fec_mode   = (phy_index == 0) ? LINE_FEC_MODE_PHY0  : LINE_FEC_MODE_PHY1;
            sys_port_speed  = (phy_index == 0) ? SYS_PORT_SPEED_PHY0 : SYS_PORT_SPEED_PHY1;
            line_port_speed = (phy_index == 0) ? LINE_PORT_SPEED_PHY0: LINE_PORT_SPEED_PHY1;
            sys_if_type     = (phy_index == 0) ? SYS_IF_TYPE_PHY0    : SYS_IF_TYPE_PHY1;
            line_if_type    = (phy_index == 0) ? LINE_IF_TYPE_PHY0   : LINE_IF_TYPE_PHY1;
            sys_if_mode     = (phy_index == 0) ? SYS_IF_MODE_PHY0    : SYS_IF_MODE_PHY1;
            line_if_mode    = (phy_index == 0) ? LINE_IF_MODE_PHY0   : LINE_IF_MODE_PHY1;

            for(side = SYS_SIDE; side >= LINE_SIDE; side--) {
                memset(&aux_mode_set, 0, sizeof(bcm_plp_aperta_device_aux_modes_t));
                phy_info.if_side  = (side == SYS_SIDE) ? SYS_SIDE : LINE_SIDE;
                phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map;
                aux_mode_set.lane_data_rate  = (side == SYS_SIDE) ? sys_lane_rate  : line_lane_rate  ;
                aux_mode_set.modulation_mode = (side == SYS_SIDE) ? sys_modulation : line_modulation ;
                aux_mode_set.fec_mode_sel    = (side == SYS_SIDE) ? sys_fec_mode   : line_fec_mode   ;
                aux_mode_set.port_type       = (side == SYS_SIDE) ? SYS_PORT_TYPE  : LINE_PORT_TYPE  ;
                retval = bcm_plp_mode_config_set( CHIP_NAME, phy_info,
                                                  (side == SYS_SIDE) ? sys_port_speed : line_port_speed,
                                                  (side == SYS_SIDE) ? sys_if_type    : line_if_type,
                                                  REF_CLOCK,
                                                  (side == SYS_SIDE) ? sys_if_mode    : line_if_mode,
                                                  (void *)&aux_mode_set
                                                );
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to set mode configuration for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
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
    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Start force Tx training:\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        total_lane_maps   = (phy_index == 0) ? total_lane_maps_phy0 : total_lane_maps_phy1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map    = (phy_index == 0) ? sys_lane_map_list_phy0[lane_map_index]  : sys_lane_map_list_phy1[lane_map_index];
            line_lane_map   = (phy_index == 0) ? line_lane_map_list_phy0[lane_map_index] : line_lane_map_list_phy1[lane_map_index];
            for(side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side  = (side == SYS_SIDE) ? SYS_SIDE : LINE_SIDE;
                phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map;
                retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                } else {
                    printf("Force Tx training is in progress for lane map 0x%x of PHY-%d at %s side...\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line");
                }
            }
        }
    }

    sleep(5);

    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Get force Tx training status:\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        total_lane_maps   = (phy_index == 0) ? total_lane_maps_phy0 : total_lane_maps_phy1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map    = (phy_index == 0) ? sys_lane_map_list_phy0[lane_map_index]  : sys_lane_map_list_phy1[lane_map_index];
            line_lane_map   = (phy_index == 0) ? line_lane_map_list_phy0[lane_map_index] : line_lane_map_list_phy1[lane_map_index];
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
#endif

    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Check for link status :\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        total_lane_maps   = (phy_index == 0) ? total_lane_maps_phy0 : total_lane_maps_phy1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map    = (phy_index == 0) ? sys_lane_map_list_phy0[lane_map_index]  : sys_lane_map_list_phy1[lane_map_index];
            line_lane_map   = (phy_index == 0) ? line_lane_map_list_phy0[lane_map_index] : line_lane_map_list_phy1[lane_map_index];
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
    printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("MacSec is in static bypass mode\n");
    printf("Ports are setup now and ready to test with traffic in gear box mode\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

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
