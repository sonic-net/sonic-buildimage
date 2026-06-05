/*
 *
 *
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */

/*
 * The app is tested with System Side connected to a Broadcom Switch,
 * that generates Egress traffic on BCM81343 PHY-0 and with Line Side
 * of BCM81343 PHY-0 connected to Line Side of BCM81343 PHY-1
 * BCM81343 PHY-1 is configured for Ingress and its System Side is
 * connected to the same Broadcom Switch.
 *
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 *
 * Note: INITIALIZE/MEMSET EVERY FUNCTION PARAMETERS TO "0",
 *       TO AVOID FUTURE COMPATIBILITY ISSUES WITH THE DRIVER.
 */

/* Includes */
#include <aperta_common.h>
#include <transform_records.h>

/* -------------------------------------------------------------------------------- *
 *                         Aperta reference application                             *
 * -------------------------------------------------------------------------------- *
 * This sample application configures the Aperta chip for 400G PAM4 (8x53G)         *
 * macsec transform mode as follows:                                                *
 *   1. Connect to the board                                                        *
 *   2. Initialize the PHY and downloads the firmware                               *
 *   3. Initialize MACsec in bypass disable mode                                    *
 *   4. Configure Tx/Rx polarity                                                    *
 *   5. Configure operating mode parameters for 400 PAM4 (8x53G) mode               *
 *   6. Setup link training or configure TX FIR settings                            *
 *   7. Enable SA Expiry Interrupt                                                  *
 *   8. Create/Add vPort(SC) on each phy-id for both Egress and Ingress             *
 *   9. Create/add SA for Egress and Ingress path                                   *
 *   10.SA chain to first SA (Egress direction), repeat for additional SA           *
 *   11.Send traffic needed for first SA to expire                                  *
 *   12.Check SA Expire Interrupt pertaining to first SA. Clear Interrupt           *
 *   13.On Egress side, remove expired SA and chain it back                         *
 *   14.On Ingress side, remove Expired SA and add new SA with SA parameters        *
 *   15.Check Statistics on Expired SAs                                             *
 *   16.Repeat steps 11-15                                                          *
 *   15.Cleanup and close the connection to the board.                              *
 * -------------------------------------------------------------------------------- *
 * Note 1 : This sample application is only for 81343 parts.                        *
 * -------------------------------------------------------------------------------- */


/* Enable MACsec un-initialization */
#define MACSEC_UNINIT_EN

/* Disable any bypass set in the Makefile */
#undef BCM_PLP_CONTROL_PACKET_BYPASS
#undef BCM_PLP_DATA_PACKET_BYPASS

#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1


/* Enable Tx forced training */
#define LINK_TRAINING_EN

/* System side parameters */
#define SYS_PORT_SPEED          400000
#define SYS_LANE_RATE           bcmpLplaneDataRate_53P125G
#define SYS_MODULATION          bcmplpModulationPAM4
#define SYS_FEC_MODE            bcmplpapertaRS544_2XN
#define SYS_IF_TYPE             bcm_pm_InterfaceAUI_C2C
#define SYS_IF_MODE             bcm_pm_Interface_mode_IEEE
#define SYS_PORT_TYPE           bcmplpPortTypePassthrough

unsigned int sys_lane_map_list[] =  {
                                      0xFF
                                    };

/* Line side parameters */
#define LINE_PORT_SPEED         400000
#define LINE_LANE_RATE          bcmpLplaneDataRate_53P125G
#define LINE_MODULATION         bcmplpModulationPAM4
#define LINE_FEC_MODE           bcmplpapertaRS544_2XN
#define LINE_IF_TYPE            bcm_pm_InterfaceAUI_C2M
#define LINE_IF_MODE            bcm_pm_Interface_mode_IEEE
#define LINE_PORT_TYPE          bcmplpPortTypePassthrough

unsigned int line_lane_map_list[] = {
                                      0xFF
                                    };

static bcm_plp_secy_sa_handle_t secy_sahandle_egress[4][2];
static bcm_plp_secy_sa_handle_t secy_sahandle_ingress[4][2];
extern bcm_plp_cfye_vport_handle_t vport_handle[2][8][2];
extern bcm_plp_cfye_rule_handle_t  rule_handle[2][8][2];
extern unsigned int vport_ids[2][8][2];

/* Main Start here */
int main(int argc, char *argv[])
{
    int macsec_side = 0;
    int count;
    int sf_enable_get = 0;
    int sf_enable_set;

    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int phy_index = 0;
    int side = 0;
    int lane_map_index;
    int total_lane_maps;
    int device_bypass_enable = 0;

    unsigned int link_status_get;
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;
    int port = 0;
    bcm_plp_access_t phy_info;
    bcm_plp_secy_sa_t sa_params;
    bcm_plp_secy_sa_t sa_params_e;
    bcm_plp_cfye_vport_t vportparams;
    bcm_plp_cfye_rule_t ruleparams;
    bcm_plp_sec_phy_access_t sec_info;
    bcm_plp_mac_access_t mac_info;
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
    memset(&vportparams, 0, sizeof(bcm_plp_cfye_vport_t));
    memset(&ruleparams, 0, sizeof(bcm_plp_cfye_rule_t));
    memset(&sa_params, 0, sizeof(bcm_plp_secy_sa_t));
    memset(&sa_params_e, 0, sizeof(bcm_plp_secy_sa_t));

    phy_info.platform_ctxt = &p_ctxt;

    /* -------------------------------------------------------------------------------- *
     * Connect to the board                                                             *
     * -------------------------------------------------------------------------------- */
    retval = device_open();
    if (retval) {
        printf("FAIL: Failed to connect to the board (ret = %d)!\n", retval);
        return retval;
    }
    /* -------------------------------------------------------------------------------- *
     * Section 1: PHY, MACsec Initialization and MACsec Configuration                   *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Initialize the PHYs and download firmware based on the test setup                *
     * -------------------------------------------------------------------------------- */
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
        if (retval) {
            printf("FAIL: bcm_plp_init_fw_bcast failed to initialize PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_init_error;
        }

        /* Read firmware info */
        retval = bcm_plp_firmware_info_get(CHIP_NAME, phy_info, &fw_ver, &fw_crc);
        if (retval) {
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
            if (retval) {
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
    if (retval) {
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
        if (retval) {
            printf("FAIL: Failed to set lane swap for PHY-%d at System side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at System side!\n", phy_info.phy_addr);
        }

        phy_info.if_side = LINE_SIDE;
        retval = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &line_laneswap_map);
        if (retval) {
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
                if (retval) {
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
    /* Enable flow control and configure 400G (8x53G) mode in PAM4 */
    flow_option = bcmplpFlowcontrolTerminateGenerate;
    /* flow_option = bcmplpFlowcontrolPassthrough; */
    memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        mac_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        mac_info.phy_info.if_side = LINE_SIDE;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            mac_info.phy_info.lane_map = line_lane_map;

            retval = bcm_plp_mac_flow_control_set(CHIP_NAME, mac_info, flow_option);
            if (retval) {
                printf("FAIL: Failed to set flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",
                        mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully set flow control option %d for lane-map 0x%x of PHY-%d!\n",
                        flow_option, mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
            }
            retval = bcm_plp_mac_flow_control_get(CHIP_NAME, mac_info, &flow_option_get);
            if (retval) {
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


    sf_enable_set = 1;
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

            /* Set MAC store and forward mode at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side = LINE_SIDE;

            printf("Setting MAC store and forward mode (%d) for lane-map 0x%x of PHY-%d at Line side...\n", sf_enable_set, phy_info.lane_map, phy_info.phy_addr);
            retval = bcm_plp_mac_store_and_forward_mode_set(CHIP_NAME, mac_info, sf_enable_set);
            if (retval) {
                printf("Failed to set MAC store and forward mode (%d) for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n", sf_enable_set, phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_init_error;
            }
            printf("Setting MAC store and forward mode (%d) for lane-map 0x%x of PHY-%d at Line side is successful\n", sf_enable_set, phy_info.lane_map, phy_info.phy_addr);

            /* Get MAC store and forward mode at Line side */
            printf("Getting MAC store and forward mode for lane-map 0x%x of PHY-%d at Line side...\n", phy_info.lane_map, phy_info.phy_addr);
            retval = bcm_plp_mac_store_and_forward_mode_get(CHIP_NAME, mac_info, &sf_enable_get);

            if (retval) {
                printf("Failed to get MAC store and forward mode for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n", phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_init_error;
            }

            /* Validate MAC store and forward mode at Line side */
            if (sf_enable_set != sf_enable_get) {
                printf("FAIL: MAC store and forward mode is not as expected for PHY-%d (expected: 0x%x, actual: 0x%x)!\n", phy_info.phy_addr, sf_enable_set, sf_enable_get);
                goto _aperta_init_error;
            }
            printf("Getting MAC store and forward mode for lane-map 0x%x of PHY-%d at Line side is successful\n", phy_info.lane_map, phy_info.phy_addr);
            printf("--------------------------------------------------------------------------------\n");
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
            if (retval) {
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
            if (retval) {
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
            if (retval) {
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
            if (retval) {
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
            if (retval) {
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
            if (retval) {
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
            if (retval) {
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
            if (retval) {
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
#else
    /* -------------------------------------------------------------------------------- *
     * Configure TX FIR settings                                                        *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        bcm_plp_pam4_tx_t tx_set;
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (side=1; side>=0; side-=1) {
            phy_info.if_side = side;
            phy_info.lane_map = ALL_LANE_MAP;
            tx_set.pre  = TX_SET_PAM4_DEF_PRE;
            tx_set.main = TX_SET_PAM4_DEF_MAIN;
            tx_set.post = TX_SET_PAM4_DEF_POST;
            tx_set.serdes_tx_tap_mode = TX_SET_PAM4_TAP_MODE;
            retval = bcm_plp_pam4_tx_set(CHIP_NAME, phy_info, &tx_set);
            if (retval) {
                printf("FAIL: Failed to set default Tx TAP values in all lanes of PHY-%d at %s side (ret = %d)!\n",
                        phy_info.phy_addr, (side==1) ? "System" : "Line", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully set default Tx TAP values in all lanes of PHY-%d at %s side!\n",
                        phy_info.phy_addr, (side==1) ? "System" : "Line");
            }
        }
    }
    sleep(5);
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
                if (retval) {
                    printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                            phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                }
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval) {
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


    /* Set Secy Interrupts */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        /* Filling phy_info */
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side  = LINE_SIDE;
        phy_info.lane_map = ALL_LANE_MAP;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        for(macsec_side = 0 ; macsec_side < 2 ; macsec_side ++)
        {
            bcm_plp_secy_intr_t secy_intr;
            bcm_plp_secy_intr_t secy_intr_ret;

            memset(&secy_intr, 0, sizeof(bcm_plp_secy_intr_t));
            memset(&secy_intr_ret, 0, sizeof(bcm_plp_secy_intr_t));

            sec_info.macsec_side = macsec_side;
            /* Set the Mask bits for Interrupt */
            /* Enabling below interrupts:
             * BCM_PLP_SECY_PHY_SA_EXPIRED_IRQ       11
             */

            secy_intr.event_mask = 0x800;
            secy_intr.f_global = 1;

            retval = bcm_plp_secy_intr_enable_set(CHIP_NAME, &sec_info, &secy_intr);
            if (retval) {
                printf("FAIL: Failed to enable SecY interrupt for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully enable SecY interrupt for lane-map 0x%x of PHY-%d in %s path event_mask 0x%x f_global 0x%x\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS",
                        secy_intr.event_mask, secy_intr.f_global);
            }
            /* Get the Interrupt value which was enabled */
            secy_intr_ret.f_global = 1;

            retval = bcm_plp_secy_intr_enable_get(CHIP_NAME, &sec_info, &secy_intr_ret);
            if (retval) {
                printf("FAIL: Failed to get SecY interrupt for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully enable SecY interrupt for lane-map 0x%x of PHY-%d in %s path event_mask 0x%x f_global 0x%x\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS",
                        secy_intr_ret.event_mask, secy_intr_ret.f_global);
            }
        }
    }

    /* Iterate through Phy-ids and set SA, vPort and rules */
    for(phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        /* Filling phy_info */
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side  = LINE_SIDE;
        phy_info.lane_map = ALL_LANE_MAP;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);
        /* Install vPort */
        for(macsec_side = 0 ; macsec_side < 2 ; macsec_side ++)
        {
            sec_info.macsec_side = macsec_side;
            vportparams.pkt_extension = macsec_side ? 3 : 0; /*As we are setting end station we are doing packet expansion by 24 bits as SCI will be implicit*/
            retval = bcm_plp_cfye_vport_add(CHIP_NAME, &sec_info,
                    &vport_handle[phy_index][port][macsec_side],
                    &vportparams);
            if (retval) {
                printf("FAIL: Failed to add vport for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully add vport for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        for(macsec_side = 0 ; macsec_side < 2 ; macsec_side ++)
        {
            sec_info.macsec_side = macsec_side;
            /* Get the vPort id from Handle */
            retval = bcm_plp_cfye_vport_index_get(CHIP_NAME, &sec_info,
                    vport_handle[phy_index][port][macsec_side],
                    &vport_ids[phy_index][port][macsec_side]);
            if (retval) {
                printf("FAIL: Failed to get vport index for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully get vport index for lane-map 0x%x of PHY-%d in %s path vPort ID = %d\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS",
                        vport_ids[phy_index][port][macsec_side]);
            }
        }

        /* install sa with transform record */
        /* INGRESS Param settings */
        sa_params.action_type = BCM_PLP_SECY_SA_ACTION_INGRESS;
        sa_params.drop_type = BCM_PLP_SECY_SA_DROP_CRC_ERROR;
        sa_params.dest_port = BCM_PLP_SECY_PORT_CONTROLLED;

        sa_params.params.ingress.validate_frames_tagged =
            BCM_PLP_SECY_FRAME_VALIDATE_STRICT;
        sa_params.params.ingress.fsa_inuse = 1;
        sa_params.params.ingress.freplay_protect = 1;
        sa_params.params.ingress.sci_p =
            discard_const(sci_basic_transform_ingress_p);
        sa_params.params.ingress.an = 0; /*Coming from the context word in Egress and Ingress direction*/
        sa_params.params.ingress.fallow_tagged = 1;
        sa_params.params.ingress.fallow_untagged = 0;
        sa_params.params.ingress.fvalidate_untagged = 0;
        /* size of the transform record in 32-bit words */
        sa_params.sa_word_count =
            sizeof(transform_record_basic_transform_ingress) /
            sizeof(uint32_t);
        sa_params.transform_record_p =
            transform_record_basic_transform_ingress;


        /* EGRESS Param settings */
        sa_params_e.action_type = BCM_PLP_SECY_SA_ACTION_EGRESS;
        sa_params_e.drop_type = BCM_PLP_SECY_SA_DROP_INTERNAL;
        sa_params_e.dest_port = BCM_PLP_SECY_PORT_CONTROLLED;

        sa_params_e.params.egress.fsa_inuse = 1;
        sa_params_e.params.egress.fprotect_frames = 1;
        sa_params_e.params.egress.finclude_sci = 1;
        sa_params_e.params.egress.fconf_protect = 1;
        sa_params_e.params.egress.fallow_data_pkts = 1;

        /* size of the transform record in 32-bit words */
        sa_params_e.sa_word_count =
            sizeof(transform_record_basic_transform_egress) /
            sizeof(uint32_t);
        sa_params_e.transform_record_p =
            transform_record_basic_transform_egress;

        /*Add SA at Egress */
        macsec_side = EGRESS;
        sec_info.macsec_side = macsec_side;
        retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                vport_ids[phy_index][port][macsec_side],
                &secy_sahandle_egress[0][phy_index],
                &sa_params_e
                );
        if (retval) {
            printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Add SA at Ingress */
        macsec_side = INGRESS;
        sec_info.macsec_side = macsec_side;
        retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                vport_ids[phy_index][port][macsec_side],
                &secy_sahandle_ingress[0][phy_index],
                &sa_params
                );
        if (retval) {
            printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /*Add other SA at Egress side (Chaining) */
        for (count = 1; count <=3; count +=1)
        {
            sa_params_e.params.egress.fsa_inuse = 0;

            switch (count)
            {
                case 1 :
                    {
                        sa_params_e.sa_word_count = sizeof(transform_record_basic_transform_egress1) / sizeof(uint32_t);
                        sa_params_e.transform_record_p = transform_record_basic_transform_egress1;
                        break;
                    }
                case 2:
                    {
                        sa_params_e.sa_word_count = sizeof(transform_record_basic_transform_egress2) / sizeof(uint32_t);
                        sa_params_e.transform_record_p = transform_record_basic_transform_egress2;
                        break;
                    }
                case 3 :
                    {
                        sa_params_e.sa_word_count = sizeof(transform_record_basic_transform_egress3) / sizeof(uint32_t);
                        sa_params_e.transform_record_p = transform_record_basic_transform_egress3;
                        break;
                    }
            }

            /* Chain SA at Egress */
            sec_info.macsec_side = EGRESS;
            retval = bcm_plp_secy_sa_chain(CHIP_NAME, &sec_info,
                    secy_sahandle_egress[count-1][phy_index],
                    &secy_sahandle_egress[count][phy_index],
                    &sa_params_e
                    );
            if (retval) {
                printf("FAIL: Failed to SA chain for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully SA chain for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }

            /*Add SA at Ingress side */
            sa_params.params.ingress.an = count; /*Coming from the context word in Egress and Ingress direction*/
            sa_params.params.ingress.fallow_tagged = 1;
            sa_params.params.ingress.fallow_untagged = 0;
            sa_params.params.ingress.fvalidate_untagged = 0;

            switch (count)
            {
                case 1 :
                    {
                        sa_params.sa_word_count =
                            sizeof(transform_record_basic_transform_ingress1) /
                            sizeof(uint32_t);
                        sa_params.transform_record_p =
                            transform_record_basic_transform_ingress1; break;
                    }

                case 2:
                    {
                        sa_params.sa_word_count =
                            sizeof(transform_record_basic_transform_ingress2) /
                            sizeof(uint32_t);
                        sa_params.transform_record_p =
                            transform_record_basic_transform_ingress2; break;
                    }

                case 3 :
                    {
                        sa_params.sa_word_count =
                            sizeof(transform_record_basic_transform_ingress3) /
                            sizeof(uint32_t);
                        sa_params.transform_record_p =
                            transform_record_basic_transform_ingress3; break;
                    }
            }

            /* Add SA at Ingress */
            sec_info.macsec_side = INGRESS;
            retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                    vport_ids[phy_index][port][sec_info.macsec_side],
                    &secy_sahandle_ingress[count][phy_index],
                    &sa_params
                    );
            if (retval) {
                printf("FAIL: Failed to SA add for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully SA add for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        for(macsec_side = 0 ; macsec_side < 2 ; macsec_side++)
        {
            sec_info.macsec_side = macsec_side;
            /* Define and Add Rules */
            ruleparams.mask.packet_type = 0x3; /* exact match on all these fields. */
            ruleparams.key.packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_OTHER;

            ruleparams.data_mask[0] = 0xffffffff; /* match on destination address.*/
            ruleparams.data_mask[1] = 0x0000ffff;
            ruleparams.policy.vport_handle = vport_handle[phy_index][port][macsec_side];
            ruleparams.data[0] = 0x9000000;
            ruleparams.data[1] = 0x101;

            retval = bcm_plp_cfye_rule_add( CHIP_NAME, &sec_info,
                    vport_handle[phy_index][port][macsec_side],
                    &rule_handle[phy_index][port][macsec_side],
                    &ruleparams
                    );
            if (retval) {
                printf("FAIL: Failed to add rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully add rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            /* Enable Rule */
            retval = bcm_plp_cfye_rule_enable(CHIP_NAME, &sec_info,
                    rule_handle[phy_index][port][macsec_side], 1);
            if (retval) {
                printf("FAIL: Failed to enable rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully enable rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }
    }


    /* All Set at this point of time. Send Traffic */
    /* Here code is running in loop for 2 times just to demonstrate re-keying concept i.e. for count = 0,1 */
    for (count = 0 ; count < 2; count +=1)
    {
        unsigned int index_arr[5], *index_pp, num_index;
        bcm_plp_secy_intr_t bcm_plp_secy_intr;
        index_pp = &index_arr[5];

        sec_info.phy_info.phy_addr = PHY_ID0;
        sec_info.macsec_side = EGRESS;
        sec_info.phy_info.lane_map = ALL_LANE_MAP;
        port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

        memset(&bcm_plp_secy_intr,0,sizeof(bcm_plp_secy_intr_t));

        bcm_plp_secy_intr.f_global = 1;

        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("\n Please send 64 packets now for SA to Expire:\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* Poll for the SA Expiry Event */
        while ((bcm_plp_secy_intr.event_mask & 0x800) == 0)
        {
            bcm_plp_secy_event_status_get(CHIP_NAME, &sec_info,&bcm_plp_secy_intr);
        }

        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("SA Has Expired \n\n");
        printf("\n++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* SA Expire Summary Statistics */
        retval = bcm_plp_secy_sa_expired_summary_checkandclear(CHIP_NAME, &sec_info,&index_pp,&num_index);
        if (retval) {
            printf("FAIL: Failed to SA expired summary for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        }

        int j;
        for (j = 0; j < num_index; j++)
        {
            printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
            printf("Expired SA Index Number = %d \n", index_pp[j]);
            printf("++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
        }

        printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("Check Packet Statistics (Encrypted/Decrypted bytes, RxCAM hit, Number of pkts transformed etc)\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* Clear Interrupt */
        retval = bcm_plp_secy_intr_status_clear(CHIP_NAME, &sec_info,&bcm_plp_secy_intr);
        if (retval) {
            printf("FAIL: Failed to clear SecY interrupt status for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully clear SecY interrupt status for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Remove the expired SA */
        retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,
                secy_sahandle_egress[0][sec_info.phy_info.phy_addr - PHY_ID0]);
        if (retval) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Chain a new SA with the newly acquired handle */
        sa_params_e.params.egress.fsa_inuse = 0;
        sa_params_e.sa_word_count =
            sizeof(transform_record_basic_transform_egress) /
            sizeof(uint32_t);
        transform_record_basic_transform_egress[14] = 0xffffffc0;
        transform_record_basic_transform_egress[2] = transform_record_basic_transform_egress[2] + (count+1);
        sa_params_e.transform_record_p =
            transform_record_basic_transform_egress;

        retval = bcm_plp_secy_sa_chain(CHIP_NAME, &sec_info,
                secy_sahandle_egress[3][sec_info.phy_info.phy_addr - PHY_ID0],
                &secy_sahandle_egress[0][sec_info.phy_info.phy_addr - PHY_ID0],
                &sa_params_e
                );
        if (retval) {
            printf("FAIL: Failed to SA chain for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA chain for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Ingress direction changes */
        sec_info.macsec_side = INGRESS;
        sec_info.phy_info.phy_addr = PHY_ID1;
        /* Remove the expired SA */
        retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,
                secy_sahandle_ingress[0][sec_info.phy_info.phy_addr - PHY_ID0]);

        if (retval) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        sa_params.params.ingress.fsa_inuse = 1;
        sa_params.params.ingress.an = 0;
        sa_params.sa_word_count =
            sizeof(transform_record_basic_transform_ingress) /
            sizeof(uint32_t);
        transform_record_basic_transform_ingress[14] = 0xffffffc1;
        transform_record_basic_transform_ingress[2]  = transform_record_basic_transform_ingress[2] + (count +1);
        sa_params.transform_record_p =
            transform_record_basic_transform_ingress;

        sec_info.phy_info.phy_addr = PHY_ID1;
        retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                vport_ids[sec_info.phy_info.phy_addr - PHY_ID0][port][sec_info.macsec_side],
                &secy_sahandle_ingress[0][sec_info.phy_info.phy_addr - PHY_ID0],
                &sa_params
                );
        if (retval) {
            printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Egress */
        sec_info.macsec_side = EGRESS;
        sec_info.phy_info.phy_addr = PHY_ID0;
        bcm_plp_secy_intr.event_mask = 0;
        bcm_plp_secy_intr.f_global = 1;

        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("\n Please send 64 packets now for SA to Expire:\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* Poll for the SA Expiry */
        while ((bcm_plp_secy_intr.event_mask & 0x800) == 0)
        {
            bcm_plp_secy_event_status_get(CHIP_NAME, &sec_info,&bcm_plp_secy_intr);
        }

        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf ("SA Has Expired \n\n");

        /* SA Expire Statistics */
        retval = bcm_plp_secy_sa_expired_summary_checkandclear(CHIP_NAME, &sec_info,&index_pp,&num_index);
        if (retval) {
            printf("FAIL: Failed to SA expired summary for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        }

        for (j = 0; j < num_index; j++)
        {
            printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
            printf("Expired SA Index Number = %d \n", index_pp[j]);
            printf("++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
        }
        printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("Check Packet Statistics (Encrypted/Decrypted bytes, RxCAM hit, Number of pkts transformed etc)\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* Clear Interrupt */
        retval = bcm_plp_secy_intr_status_clear(CHIP_NAME, &sec_info,&bcm_plp_secy_intr);
        if (retval) {
            printf("FAIL: Failed to clear SecY interrupt status for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully clear SecY interrupt status for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }


        /* Remove the expired SA */
        retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,secy_sahandle_egress[1][sec_info.phy_info.phy_addr - PHY_ID0]);
        if (retval) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }


        /* Chain a new SA with the newly acquired handle */
        sa_params_e.params.egress.fsa_inuse = 0;
        sa_params_e.sa_word_count =
            sizeof(transform_record_basic_transform_egress1) /
            sizeof(uint32_t);
        transform_record_basic_transform_egress1[14] = 0xffffffc0;
        transform_record_basic_transform_egress1[2]  = transform_record_basic_transform_egress1[2] + count +1;
        sa_params_e.transform_record_p =
            transform_record_basic_transform_egress1;

        retval = bcm_plp_secy_sa_chain(CHIP_NAME, &sec_info,
                secy_sahandle_egress[0][sec_info.phy_info.phy_addr - PHY_ID0],
                &secy_sahandle_egress[1][sec_info.phy_info.phy_addr - PHY_ID0],
                &sa_params_e
                );
        if (retval) {
            printf("FAIL: Failed to SA chain for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA chain for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Ingress direction changes */
        sec_info.macsec_side = INGRESS;
        sec_info.phy_info.phy_addr = PHY_ID1;
        /* Remove the expired SA */
        retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,
                secy_sahandle_ingress[1][sec_info.phy_info.phy_addr - PHY_ID0]);
        if (retval) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Ingress direction changes */
        sa_params.params.ingress.fsa_inuse = 1;
        sa_params.params.ingress.an = 1;
        sa_params.sa_word_count =
            sizeof(transform_record_basic_transform_ingress1) /
            sizeof(uint32_t);
        transform_record_basic_transform_ingress1[14] = 0xffffffc1;
        transform_record_basic_transform_ingress1[2] = transform_record_basic_transform_ingress1[2] + count +1;
        sa_params.transform_record_p =
            transform_record_basic_transform_ingress1;

        /* Add SA */
        retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                vport_ids[sec_info.phy_info.phy_addr - PHY_ID0][port][sec_info.macsec_side],
                &secy_sahandle_ingress[1][sec_info.phy_info.phy_addr - PHY_ID0],
                &sa_params
                );
        if (retval) {
            printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Egress */
        sec_info.macsec_side = EGRESS;
        sec_info.phy_info.phy_addr = PHY_ID0;
        bcm_plp_secy_intr.event_mask = 0;
        bcm_plp_secy_intr.f_global = 1;

        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("\n Please send 64 packets now for SA to Expire:\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* Poll for the SA Expiry Interrupt */
        while ((bcm_plp_secy_intr.event_mask & 0x800) == 0)
        {
            bcm_plp_secy_event_status_get(CHIP_NAME, &sec_info,&bcm_plp_secy_intr);
        }

        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf ("SA Has Expired \n\n");

        /* SA Expire Statistics */
        retval = bcm_plp_secy_sa_expired_summary_checkandclear(CHIP_NAME, &sec_info,&index_pp,&num_index);
        if (retval) {
            printf("FAIL: Failed to SA expired summary for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        }
        for (j = 0; j < num_index; j++)
        {
            printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
            printf("Expired SA Index Number = %d \n", index_pp[j]);
            printf("++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
        }

        printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("Check Packet Statistics (Encrypted/Decrypted bytes, RxCAM hit, Number of pkts transformed etc)\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* Clear Interrupt */
        retval = bcm_plp_secy_intr_status_clear(CHIP_NAME, &sec_info,&bcm_plp_secy_intr);
        if (retval) {
            printf("FAIL: Failed to clear SecY interrupt status for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully clear SecY interrupt status for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Remove the expired SA */
        retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,
                secy_sahandle_egress[2][sec_info.phy_info.phy_addr - PHY_ID0]);
        if (retval) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Ingress direction changes */
        /* Chain a new SA with the newly acquired handle */
        sa_params_e.params.egress.fsa_inuse = 0;
        sa_params_e.sa_word_count =
            sizeof(transform_record_basic_transform_egress2) /
            sizeof(uint32_t);
        transform_record_basic_transform_egress2[14] = 0xffffffc0;
        transform_record_basic_transform_egress2[2] = transform_record_basic_transform_egress2[2] + count +1;
        sa_params_e.transform_record_p =
            transform_record_basic_transform_egress2;

        retval = bcm_plp_secy_sa_chain(CHIP_NAME, &sec_info,
                secy_sahandle_egress[1][sec_info.phy_info.phy_addr - PHY_ID0],
                &secy_sahandle_egress[2][sec_info.phy_info.phy_addr - PHY_ID0],
                &sa_params_e
                );
        if (retval) {
            printf("FAIL: Failed to SA chain for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA chain for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Ingress direction changes */
        sec_info.macsec_side = INGRESS;
        sec_info.phy_info.phy_addr = PHY_ID1;

        /* Remove the expired SA */
        retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,
                secy_sahandle_ingress[2][sec_info.phy_info.phy_addr - PHY_ID0]);
        if (retval) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Ingress direction changes */
        sa_params.params.ingress.fsa_inuse = 1;
        sa_params.params.ingress.an = 2;
        sa_params.sa_word_count =
            sizeof(transform_record_basic_transform_ingress2) /
            sizeof(uint32_t);
        transform_record_basic_transform_ingress2[14] = 0xffffffc1;
        transform_record_basic_transform_ingress2[2] = transform_record_basic_transform_ingress2[2] + count + 1;
        sa_params.transform_record_p =
            transform_record_basic_transform_ingress2;

        /* Add SA */
        retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                vport_ids[sec_info.phy_info.phy_addr - PHY_ID0][port][sec_info.macsec_side],
                &secy_sahandle_ingress[2][sec_info.phy_info.phy_addr - PHY_ID0],
                &sa_params
                );
        if (retval) {
            printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Egress */
        sec_info.macsec_side = EGRESS;
        sec_info.phy_info.phy_addr = PHY_ID0;
        bcm_plp_secy_intr.event_mask = 0;
        bcm_plp_secy_intr.f_global = 1;

        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("\n Please send 64 packets now for SA to Expire:\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* Poll for the SA Expiry */
        while ((bcm_plp_secy_intr.event_mask & 0x800) == 0)
        {
            bcm_plp_secy_event_status_get(CHIP_NAME, &sec_info,&bcm_plp_secy_intr);
        }

        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf ("SA Has Expired \n\n");

        /* SA Expire Statistics */
        retval = bcm_plp_secy_sa_expired_summary_checkandclear(CHIP_NAME, &sec_info,&index_pp,&num_index);
        if (retval) {
            printf("FAIL: Failed to SA expired summary for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        }

        for (j = 0; j < num_index; j++)
        {
            printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
            printf("Expired SA Index Number = %d \n", index_pp[j]);
            printf("++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
        }

        printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("Check Packet Statistics (Encrypted/Decrypted bytes, RxCAM hit, Number of pkts transformed etc)\n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* Clear Interrupt */
        retval = bcm_plp_secy_intr_status_clear(CHIP_NAME, &sec_info,&bcm_plp_secy_intr);
        if (retval) {
            printf("FAIL: Failed to clear SecY interrupt status for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully clear SecY interrupt status for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Remove the expired SA */
        sec_info.phy_info.phy_addr = PHY_ID0;
        retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,secy_sahandle_egress[3][sec_info.phy_info.phy_addr - PHY_ID0]);
        if (retval) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Chain a new SA with the newly acquired handle */
        sa_params_e.params.egress.fsa_inuse = 0;
        sa_params_e.sa_word_count =
            sizeof(transform_record_basic_transform_egress3) /
            sizeof(uint32_t);
        transform_record_basic_transform_egress3[14] = 0xffffffc0;
        transform_record_basic_transform_egress3[2] = transform_record_basic_transform_egress3[2] + count +1;
        sa_params_e.transform_record_p =
            transform_record_basic_transform_egress3;

        retval = bcm_plp_secy_sa_chain(CHIP_NAME, &sec_info,
                secy_sahandle_egress[2][sec_info.phy_info.phy_addr - PHY_ID0],
                &secy_sahandle_egress[3][sec_info.phy_info.phy_addr - PHY_ID0],
                &sa_params_e
                );
        if (retval) {
            printf("FAIL: Failed to SA chain for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA chain for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Ingress direction changes */
        sec_info.macsec_side = INGRESS;
        sec_info.phy_info.phy_addr = PHY_ID1;

        /* Remove the expired SA */
        retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,
                secy_sahandle_ingress[3][sec_info.phy_info.phy_addr - PHY_ID0]);
        if (retval) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        sa_params.params.ingress.fsa_inuse = 1;
        sa_params.params.ingress.an = 3;
        sa_params.sa_word_count =
            sizeof(transform_record_basic_transform_ingress3) /
            sizeof(uint32_t);
        transform_record_basic_transform_ingress3[14] = 0xffffffc1;
        transform_record_basic_transform_ingress3[2] = transform_record_basic_transform_ingress3[2] + count +1;
        sa_params.transform_record_p =
            transform_record_basic_transform_ingress3;

        /* Add SA */
        retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                vport_ids[sec_info.phy_info.phy_addr - PHY_ID0][port][sec_info.macsec_side],
                &secy_sahandle_ingress[3][sec_info.phy_info.phy_addr - PHY_ID0],
                &sa_params
                );
        if (retval) {
            printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
    }

#ifdef MACSEC_UNINIT_EN
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.lane_map = ALL_LANE_MAP;

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
            if (retval) {
                printf("FAIL: Failed to disable rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully disable rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        /*++++++++++++++++++++++++++++++++++++++++++++
          remove rule
          +++++++++++++++++++++++++++++++++++++++++++++*/
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side;
            retval = bcm_plp_cfye_rule_remove(CHIP_NAME, &sec_info, rule_handle[phy_index][port][macsec_side]);
            if (retval) {
                printf("FAIL: Failed to remove rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully remove rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        /*++++++++++++++++++++++++++++++++++++++++++++
          remove vPort
          +++++++++++++++++++++++++++++++++++++++++++++*/
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side;
            retval = bcm_plp_cfye_vport_remove(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side]);
            if (retval) {
                printf("FAIL: Failed to remove vport for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully remove vport for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        /*++++++++++++++++++++++++++++++++++++++++++++
          remove SA
          +++++++++++++++++++++++++++++++++++++++++++++*/
        for (count = 0; count < 4; count++) {
            sec_info.macsec_side = EGRESS;
            retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info, secy_sahandle_egress[count][phy_index]);
            if (retval) {
                printf("FAIL: Failed to remove SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully remove SA for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        /*++++++++++++++++++++++++++++++++++++++++++++
          remove SA
          +++++++++++++++++++++++++++++++++++++++++++++*/
        for (count = 0; count < 4; count++) {
            sec_info.macsec_side = INGRESS;
            retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info, secy_sahandle_ingress[count][phy_index]);
            if (retval) {
                printf("FAIL: Failed to remove SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully remove SA for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
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
        if (retval) {
            printf("FAIL: Failed to cleanup mac for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        }
        retval = bcm_plp_cleanup(CHIP_NAME, phy_info);
        if (retval) {
            printf("FAIL: Failed to cleanup PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        }
    }
#endif /* MACSEC_UNINIT_EN */

_aperta_init_error:
    /* Check for test result */
    if (!retval) {
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

