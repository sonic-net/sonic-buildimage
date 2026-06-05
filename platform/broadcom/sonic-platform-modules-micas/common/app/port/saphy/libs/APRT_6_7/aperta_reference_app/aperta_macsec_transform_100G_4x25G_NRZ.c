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
 *                         Aperta reference application                             *
 * -------------------------------------------------------------------------------- *
 * This sample application configures the Aperta chip for 100G (4x25G)              *
 * macsec transform mode as follows:                                                *
 *   1. Connect to the board.                                                       *
 *   2. Initialize the PHY and download the firmware                                *
 *   3. Initialize MACsec in bypass disable mode                                    *
 *   4. Configure Tx/Rx polarity                                                    *
 *   5. Configure operating mode parameters for 100G NRZ (4x25G) mode               *
 *   6. Setup link training                                                         *
 *   7. Setup SA/vport for macsec transformation & macsec validation                *
 *   8. Update/Remove/Add SA operations for the secy device                         *
 *   9. Statistics/Dump/Window Size operations for secy                             *
 *   10.Uninitialize macsec, cleanup and close the connection to the board.         *
 * -------------------------------------------------------------------------------- *
 * Please Note : This sample application is only for 81343 parts.                        *
 * -------------------------------------------------------------------------------- */

/* 100G NRZ (4x25G) MACsec mode */
#define OP_MODE_STR    "100G NRZ (4x25G) MACsec mode"
#define DROP_TAGGED_PKTS 0
#define ALLOW_TAGGED_PKTS 1
#define SEQ0_TRANSREC_OFFSET 14

/* Enable Tx forced training */
#undef LINK_TRAINING_EN

/* Disable any bypass set in the Makefile */
#undef BCM_PLP_CONTROL_PACKET_BYPASS
#undef BCM_PLP_DATA_PACKET_BYPASS

/* Enable MACsec un-initialization */
#define MACSEC_UNINIT_EN

#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1

extern bcm_plp_cfye_vport_handle_t vport_handle[2][8][2];
extern bcm_plp_cfye_rule_handle_t  rule_handle[2][8][2];
extern bcm_plp_secy_sa_handle_t    secy_sahandle[2][8][2];
extern unsigned int vport_ids[2][8][2];

/* System side parameters */
#define SYS_PORT_SPEED          100000
#define SYS_LANE_RATE           bcmpLplaneDataRate_25P78125G
#define SYS_MODULATION          bcmplpModulationNRZ
#define SYS_FEC_MODE            bcmplpapertaNoFEC
#define SYS_IF_TYPE             bcm_pm_InterfaceCAUI4_C2C
#define SYS_IF_MODE             bcm_pm_Interface_mode_IEEE
#define SYS_PORT_TYPE           bcmplpPortTypePassthrough

unsigned int sys_lane_map_list[] =  {
                                      0x0F,
                                      0xF0
                                    };

/* Line side parameters */
#define LINE_PORT_SPEED         100000
#define LINE_LANE_RATE          bcmpLplaneDataRate_25P78125G
#define LINE_MODULATION         bcmplpModulationNRZ
#define LINE_FEC_MODE           bcmplpapertaNoFEC
#define LINE_IF_TYPE            bcm_pm_InterfaceCAUI4_C2M
#define LINE_IF_MODE            bcm_pm_Interface_mode_IEEE
#define LINE_PORT_TYPE          bcmplpPortTypePassthrough

unsigned int line_lane_map_list[] = {
                                      0x0F,
                                      0xF0
                                    };

/* Main entry to application */
int main(int argc, char *argv[])
{
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int phy_index = 0;
    int j, side = 0;
    int macsec_side = 0;
    int lane_map_index;
    int total_lane_maps;
    int device_bypass_enable = 0;
    int port = 0;
    unsigned int winsize, seq0;
    int mtusize_bytes = 0;
    bcm_plp_cfye_vport_t vportparams;
    unsigned int index_arr[5], *index_pp, num_index;
    unsigned int link_status_get;
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;

    bcm_plp_sec_phy_access_t sec_info;
    bcm_plp_mac_access_t mac_info;
    bcm_plp_access_t phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_aperta_fw_init_params_t aperta_init_params;
    bcm_plp_mac_flow_control_t flow_option, flow_option_get;
    bcm_laneswap_map_t sys_laneswap_map, line_laneswap_map;
    bcm_plp_aperta_device_aux_modes_t aux_mode_sys_set, aux_mode_line_set;
    bcm_plp_cfye_device_exceptions_t exception;
    bcm_plp_cfye_device_control_t dev_control;
    bcm_plp_cfye_device_t dev_init;

#ifdef LINK_TRAINING_EN
    unsigned int training_ena_dis = DISABLE;
    unsigned int training_failure = TRAINING_FAILURE;
    unsigned int training_status  = UNTRAINED;
#endif

    /* Initialize structure instances to zero */
    memset(&sec_info, 0, sizeof(bcm_plp_sec_phy_access_t));
    memset(&mac_info, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type, 0, sizeof(bcm_plp_firmware_load_type_t));
    memset(&aperta_init_params,0, sizeof(bcm_plp_aperta_fw_init_params_t));
    memset(&sys_laneswap_map,  0, sizeof(bcm_laneswap_map_t));
    memset(&line_laneswap_map, 0, sizeof(bcm_laneswap_map_t));
    memset(&aux_mode_sys_set,  0, sizeof(bcm_plp_aperta_device_aux_modes_t));
    memset(&aux_mode_line_set, 0, sizeof(bcm_plp_aperta_device_aux_modes_t));
    memset(&exception, 0, sizeof(bcm_plp_cfye_device_exceptions_t));
    memset(&dev_control, 0, sizeof(bcm_plp_cfye_device_control_t));
    memset(&dev_init, 0, sizeof(bcm_plp_cfye_device_t));

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
     * Initialize the PHYs and download firmware based on the test setup                *
     * -------------------------------------------------------------------------------- */
    aperta_init_params.pll1_vco_rate = bcmplpVco25p781G;
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
     * Initialize MACsec CfyE and SecY                                                  *
     * -------------------------------------------------------------------------------- */
    phy_info.lane_map = ALL_LANE_MAP;
    device_bypass_enable = 0;
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
            /* device_bypass_enable is set to 0, to trigger macsec transformation & validation */
            retval = macsec_initialize(sec_info, device_bypass_enable);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: MACSec Initialize for PHY-ID[%d], macsec_side[%d], return code [%d]\n",
                phy_info.phy_addr, sec_info.macsec_side, retval);
                goto _aperta_macsec_init_error;
            } else {
                printf("PASS: MACSec Initialize for PHY-ID[%d], macsec_side [%d] \n",
                phy_info.phy_addr, sec_info.macsec_side);
            }
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

    /* Config SecY set for device*/
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side  = LINE_SIDE;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            phy_info.lane_map = line_lane_map ;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
                retval = bcm_plp_secy_config_set(CHIP_NAME, &sec_info);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to perform secy config set for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS: Successfully performed secy config set for lane-map 0x%x of PHY-%d in %s path!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
                }
            }
        }
    }

    /* Enable flow control and configure 100G (4x25G) mode in NRZ, no FEC */
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
     * Start Force Tx training at system side                                           *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            phy_info.if_side = SYS_SIDE;
            phy_info.lane_map = sys_lane_map ;
            retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("Force Tx training is in progress for lane map 0x%x of PHY-%d at System side...\n",
                phy_info.lane_map, phy_info.phy_addr);
            }
        }
    }
    sleep(5);
    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training status at System side                                      *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            phy_info.if_side  = SYS_SIDE;
            phy_info.lane_map = sys_lane_map;
            training_ena_dis  = DISABLE ;
            retval = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to get training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
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
              goto _aperta_config_error;
            } else {
                printf("Training status for lane-map 0x%x of PHY-%d at System side as below :\n", phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
                printf("training_failure        = %s\n", training_failure ? "TRAINING_FAILURE" : "TRAINING_COMPLETE");
                printf("training_status         = %s\n", training_status ? "TRAINED" : "UNTRAINED");
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Start Force Tx training at Line side                                             *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            phy_info.if_side = LINE_SIDE;
            phy_info.lane_map = line_lane_map ;
            retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("Force Tx training is in progress for lane map 0x%x of PHY-%d at Line side...\n",
                phy_info.lane_map, phy_info.phy_addr);
            }
        }
    }
    sleep(5);
    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training status at Line side                                        *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            phy_info.if_side  = LINE_SIDE;
            phy_info.lane_map = line_lane_map;
            training_ena_dis  = DISABLE ;
            retval = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to get training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
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
              goto _aperta_config_error;
            } else {
                printf("Training status for lane-map 0x%x of PHY-%d at Line side as below :\n", phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
                printf("training_failure        = %s\n", training_failure ? "TRAINING_FAILURE" : "TRAINING_COMPLETE");
                printf("training_status         = %s\n", training_status ? "TRAINED" : "UNTRAINED");
            }
        }
    }
#endif

    /* Getting link status on SYS and LINE side */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            for (side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side = side;
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
     * Section 5 : Macsec Transformations and sample cfye & secy operations             *
     * -------------------------------------------------------------------------------- */
    gen_macsec_configs();

    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = line_lane_map_list[lane_map_index];

            /* Filling phy_info */
            phy_info.if_side = LINE_SIDE;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

            /*++++++++++++++++++++++++++++++++++++++++++++
                              install vPort
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
        memset(&vportparams, 0, sizeof(bcm_plp_cfye_vport_t));

        retval = bcm_plp_cfye_vport_add(CHIP_NAME, &sec_info, &vport_handle[phy_index][port][macsec_side], &vportparams);
        if (retval){
            printf("FAIL: bcm_plp_cfye_vport_add API failed for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                        goto _aperta_config_error;
        } else {
            printf("PASS: bcm_plp_cfye_vport_add API successful for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        }

        vportparams.pkt_extension = sec_info.macsec_side ? 0 : 3;
        retval = bcm_plp_cfye_vport_update(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side], &vportparams);
        if (retval){
            printf("FAIL: bcm_plp_cfye_vport_update API failed for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                        goto _aperta_config_error;
        } else {
            printf("PASS: bcm_plp_cfye_vport_update API successful for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        }

        vport_ids[phy_index][port][macsec_side] = bcm_plp_cfye_vport_id_get(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side]);
        printf("PASS: Successfully got vPort ID = %d for lane-map 0x%x of PHY-%d in %s path!\n",
                   vport_ids[phy_index][port][macsec_side], sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");

            }

            /*++++++++++++++++++++++++++++++++++++++++++++
             Perform cfye drop action if a vPort is missed
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for(macsec_side = 0 ; macsec_side < 2 ; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                dev_init.control_p = &dev_control;
                dev_init.control_p->exceptions_p = &exception;
                dev_init.control_p->exceptions_p->drop_action = BCM_PLP_CFYE_DROP_INTERNAL;
                retval = bcm_plp_cfye_device_update(CHIP_NAME, &sec_info, &dev_init);
                if(retval) {
                    printf("FAIL: bcm_plp_cfye_device_update for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                   install sa with transform record
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = macsec_install_sa_with_transform_record(sec_info, POLICY_BYPASS_NONE);
                if(retval) {
                    printf("FAIL: Failed to install SA with transform record for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                           Add rule policy
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = macsec_add_rule_policy(sec_info, POLICY_BYPASS_NONE);
                if(retval) {
                    printf("FAIL: Failed to add rule policy for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                              Enable rule
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
        retval = bcm_plp_cfye_rule_enable(CHIP_NAME, &sec_info, rule_handle[phy_index][port][macsec_side], 1);
        if(retval) {
            printf("FAIL: bcm_plp_cfye_rule_enable API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
        } else {
            printf("PASS : bcm_plp_cfye_rule_enable API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                               MTU Update
            +++++++++++++++++++++++++++++++++++++++++++++*/
            sec_info.macsec_side = EGRESS;
            mtusize_bytes = 1500;
            retval = macsec_mtu_update(sec_info, mtusize_bytes);
            if(retval) {
                printf("FAIL: Failed to update mtu for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                       sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
             Update rule to use a different destination mac
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = macsec_update_rule(sec_info);
                if(retval) {
                    printf("FAIL: Failed to update rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                           sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                }
            }
            /*++++++++++++++++++++++++++++++++++++++++++++
                       Disable rule
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
            retval = bcm_plp_cfye_rule_disable(CHIP_NAME, &sec_info, rule_handle[phy_index][port][macsec_side], 1);
            if(retval) {
            printf("FAIL: bcm_plp_cfye_rule_disable API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
             } else {
            printf("PASS : bcm_plp_cfye_rule_disable API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
             }
            }
            /*++++++++++++++++++++++++++++++++++++++++++++
                       Enable all rules
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
            retval = bcm_plp_cfye_rule_enable_disable(CHIP_NAME, &sec_info, NULL, NULL, 1, 0, 1);
        if(retval) {
            printf("FAIL: bcm_plp_cfye_rule_enable_disable API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                        goto _aperta_config_error;
        } else {
            printf("PASS : bcm_plp_cfye_rule_enable_disable API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        }
        }
            /*++++++++++++++++++++++++++++++++++++++++++++
                       Dump all rules
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
            retval = bcm_plp_cfye_diag_rule_dump(CHIP_NAME, &sec_info, NULL, 1);

                if(retval) {
            printf("FAIL: bcm_plp_cfye_diag_rule_dump API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
            } else {
            printf("PASS : bcm_plp_cfye_diag_rule_dump API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
            }
            }

            /*+++++++++++++++++++++++++++++++++++++++++++++++++++++
                    SA Update to drop macsec tagged packets
            +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
            sec_info.macsec_side = INGRESS;

            retval = macsec_sa_allowdrop_tagged_pkts(sec_info, DROP_TAGGED_PKTS);
            if(retval) {
                printf("FAIL: Failed to update SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, "INGRESS", retval);
                goto _aperta_config_error;
            }

            /*+++++++++++++++++++++++++++++++++++++++++++++++++++++
                                SA read
            +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;

                /* example read seq0 offset 14 */
                retval = bcm_plp_secy_sa_read(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side], SEQ0_TRANSREC_OFFSET, 1, &seq0);
                if(retval) {
                    printf("FAIL: bcm_plp_secy_sa_read API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS : bcm_plp_secy_sa_read API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] seq0[%d]\n",
                            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval, seq0);
                }
            }
         }
     }

    /* -------------------------------------------------------------------------------- *
     * Ports are setup now and ready to test with traffic with dm=00:00:00:08:01:01     *
     * Macsec Packets will be dropped
     * -------------------------------------------------------------------------------- */
#ifdef TRAFFIC_DEBUG
     retval = macsec_traffic_verify("00:00:00:08:01:01", 0xF, "drop");
     if (retval)
     {
         printf("Print_statistics() observed failure. Exiting... \n");
         goto _aperta_config_error;
         return retval;
     }
     retval = macsec_traffic_verify("00:00:00:08:01:01", 0xF0, "drop");
     if (retval)
     {
         printf("Print_statistics() observed failure. Exiting... \n");
         goto _aperta_config_error;
         return retval;
     }
#endif

    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = line_lane_map_list[lane_map_index];

            /* Filling phy_info */
            phy_info.if_side = LINE_SIDE;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

            /*+++++++++++++++++++++++++++++++++++++++++++++++++++++
                    SA Update to allow macsec tagged packets
            +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
            sec_info.macsec_side = INGRESS;

            retval = macsec_sa_allowdrop_tagged_pkts(sec_info, ALLOW_TAGGED_PKTS);
            if(retval) {
                printf("FAIL: Failed to update SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, "INGRESS", retval);
                goto _aperta_config_error;
            }

            /*+++++++++++++++++++++++++++++++++++++++++++++++++++++
                                SA read
            +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;

        /* example read seq0 offset 14 */
        retval = bcm_plp_secy_sa_read(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side], SEQ0_TRANSREC_OFFSET, 1, &seq0);
        if(retval) {
            printf("FAIL: bcm_plp_secy_sa_read API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
        } else {
            printf("PASS : bcm_plp_secy_sa_read API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] seq0[%d]\n",
                sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval, seq0);
            }
            }

            /*+++++++++++++++++++++++++++++++++++++++++++++++++++++
                                SA Dump
            +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
            retval = bcm_plp_secy_diag_sa_dump(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side], 0);
            if(retval) {
                printf("FAIL: bcm_plp_secy_diag_sa_dump API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
            } else {
            printf("PASS : bcm_plp_secy_diag_sa_dump API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
            }
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Ports are setup now and ready to test with traffic with dm=00:00:00:08:01:01     *
     * Macsec Packets will be allowed
     * -------------------------------------------------------------------------------- */
#ifdef TRAFFIC_DEBUG
     retval = macsec_traffic_verify("00:00:00:08:01:01", 0xF, "flow");
     if (retval)
     {
         printf("Print_statistics() observed failure. Exiting... \n");
         goto _aperta_config_error;
         return retval;
     }
     retval = macsec_traffic_verify("00:00:00:08:01:01", 0xF0, "flow");
     if (retval)
     {
         printf("Print_statistics() observed failure. Exiting... \n");
         goto _aperta_config_error;
         return retval;
     }
#endif

    /*+++++++++++++++++++++++++++++++++++++++++++++++++++++
    SA Window size update - change window size
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = line_lane_map_list[lane_map_index];
            /* Filling phy_info */
            phy_info.if_side = LINE_SIDE;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                winsize = 0x15E; /* Increase window size say to 350 */
                retval = macsec_sa_windowsize_update(sec_info, winsize);
                if(retval) {
                    printf("FAIL: Failed to read SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                }
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Ports are setup now and ready to test with traffic with dm=00:00:00:08:01:01     *
     * -------------------------------------------------------------------------------- */
#ifdef TRAFFIC_DEBUG
     retval = macsec_traffic_verify("00:00:00:08:01:01", 0xF, "flow");
     if (retval)
     {
         printf("Print_statistics() observed failure. Exiting... \n");
         goto _aperta_config_error;
         return retval;
     }
     retval = macsec_traffic_verify("00:00:00:08:01:01", 0xF0, "flow");
     if (retval)
     {
         printf("Print_statistics() observed failure. Exiting... \n");
         goto _aperta_config_error;
         return retval;
     }
#endif

    /*+++++++++++++++++++++++++++++++++++++++++++++++++++++
    - Check expired SA statistics
    - useful when there is no rollover/chaining and
      packet seq number thresholds are reached for an SA
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    index_pp = &index_arr[5];
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = line_lane_map_list[lane_map_index];
            /* Filling phy_info */
            phy_info.if_side = LINE_SIDE;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;

                /* SA Expire Statistics */
                retval = bcm_plp_secy_sa_expired_summary_checkandclear(CHIP_NAME, &sec_info, &index_pp, &num_index);
                if(retval) {
                    printf("FAILED: bcm_plp_secy_sa_expired_summary_checkandclear for PHY_ID[%d], Macse-Side[%d], return code =[%d] SA_Index[%d]\n", phy_index, macsec_side, retval, num_index);
                    goto _aperta_config_error;
                } else {
                    printf("PASSED: bcm_plp_secy_sa_expired_summary_checkandclear for PHY_ID[%d], Macse-Side[%d], return code =[%d] SA_Index[%d]\n", phy_index, macsec_side, retval, num_index);
                    for (j = 0; j < num_index; j++) {
                        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
                        printf("Expired SA Index Number = %d \n", index_pp[j]);
                        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
                    }
                }
            }
        }
    }


#ifdef MACSEC_UNINIT_EN
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = line_lane_map_list[lane_map_index];

            /* Filling phy_info */
            phy_info.if_side = LINE_SIDE;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

            /*++++++++++++++++++++++++++++++++++++++++++++
                       Disable all rules
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
        retval = bcm_plp_cfye_rule_enable_disable(CHIP_NAME, &sec_info, NULL, NULL, 0, 1, 1);

        if(retval) {
            printf("FAIL: bcm_plp_cfye_rule_enable_disable API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
        } else {
            printf("PASS : bcm_plp_cfye_rule_enable_disable API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                       remove rule
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;

            retval = bcm_plp_cfye_rule_remove(CHIP_NAME, &sec_info, rule_handle[phy_index][port][macsec_side]);

            if(retval) {
            printf("FAIL: bcm_plp_cfye_rule_remove API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                        goto _aperta_config_error;
            } else {
                printf("PASS : bcm_plp_cfye_rule_remove API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
            }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                       remove vPort
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
            retval = bcm_plp_cfye_vport_remove(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side]);
            if (retval){
            printf("FAIL: bcm_plp_cfye_vport_remove API failed for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
            } else {
            printf("PASS: bcm_plp_cfye_vport_remove API successful for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
            sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
            }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                       remove SA
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side]);
                if(retval) {
                    printf("FAIL: bcm_plp_secy_sa_remove API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                           sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS : bcm_plp_secy_sa_remove API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
               }
            }
        }
    }
#endif
_aperta_config_error:
#ifdef MACSEC_UNINIT_EN
    /* -------------------------------------------------------------------------------- *
     * Uninitialize MACsec CfyE and SecY                                                *
     * -------------------------------------------------------------------------------- */
    phy_info.lane_map = ALL_LANE_MAP;
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        for(macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
            retval = macsec_uninitialize(sec_info);
            if (retval) {
                printf("FAIL: Failed to uninitialize MACSec device for PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            } else {
                printf("PASS: Successfully uninitialized MACSec device for PHY-%d in %s path!\n",
                sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }
    }
#endif
_aperta_macsec_init_error:
#ifdef MACSEC_UNINIT_EN
    /* -------------------------------------------------------------------------------- *
     * PHY and MAC Cleanup                                                              *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side = LINE_SIDE;

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
#endif /* MACSEC_UNINIT_EN */

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
