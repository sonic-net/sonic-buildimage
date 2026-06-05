/*
 * $Copyright: (c) 2023 Broadcom.
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
 * This sample application configures the Aperta2 chip for 200G(2x106G) PAM4 with   *
 * macsec static bypass mode as follows:                                            *
 *   1. Connect to the board.                                                       *
 *   2. Download the firmware and initialize the PHY in static bypass mode          *
 *   3. Configure Tx/Rx polarity                                                    *
 *   4. Configure operating mode parameters for 200G(2x106G) PAM4 mode              *
 *   5. Setup link training on System side                                          *
 *   6. Check link status                                                           *
 *   7. Set AN ability and enable Autoneg on Line side                              *
 *   8. Get AN ability and AN status                                                *
 *   9. Check link status                                                           *
 *   10.Cleanup and close the connection to the board.                              *
 * -------------------------------------------------------------------------------- *
 * Note : This sample application is only for 85343 parts.                          *
 * -------------------------------------------------------------------------------- */

/*AN test specific configuration */
#define AN_MASTER_LANE  0
#define CL72_EN         1
#define TECH_ABILITY    bcmplpAnCap200G_CR2_KR2
#define FEC_ABILITY     bcmplpFecRS544_2XN
#define PAUSE_ABILITY   0

/* Mode Config Defines */
/* System side parameters */
#define SYS_PORT_SPEED          200000
#define SYS_LANE_RATE           bcmplpLaneDataRate_106P25G
#define SYS_MODULATION          bcmplpModulationPAM4
#define SYS_FEC_MODE            bcmplpaperta2RS544_2XN
#define SYS_IF_TYPE             bcm_pm_InterfaceKR
#define SYS_IF_MODE             bcmplpInterfaceModeIEEE
#define SYS_PORT_TYPE           bcmplpPortTypePassthrough
#define SYS_OCTAL_CROSSING      bcmplpNoOctalCrossing

/* System side lanemaps */
unsigned int sys_lane_map_list[] =  {
                                      0x0003, 0x000C, 0x0030, 0x00C0, 0x0300, 0x0C00, 0x3000, 0xC000
                                    };

/* Line side parameters */
#define LINE_PORT_SPEED         200000
#define LINE_LANE_RATE          bcmplpLaneDataRate_106P25G
#define LINE_MODULATION         bcmplpModulationPAM4
#define LINE_FEC_MODE           bcmplpaperta2RS544_2XN
#define LINE_IF_TYPE            bcm_pm_InterfaceKR
#define LINE_IF_MODE            bcmplpInterfaceModeIEEE
#define LINE_PORT_TYPE          bcmplpPortTypePassthrough
#define LINE_OCTAL_CROSSING     bcmplpNoOctalCrossing

/* Line side lanemaps */
unsigned int line_lane_map_list[] = {
                                      0x0003, 0x000C, 0x0030, 0x00C0, 0x0300, 0x0C00, 0x3000, 0xC000
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

    bcm_plp_mac_access_t mac_info;
    bcm_plp_access_t phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_aperta2_fw_init_params_t fw_init_params;
    bcm_plp_mac_flow_control_t flow_option, flow_option_get;
    bcm_plp_aperta2_device_aux_modes_t aux_mode_sys_set, aux_mode_line_set;

    unsigned int training_ena_dis = DISABLE;
    unsigned int training_failure = TRAINING_FAILURE;
    unsigned int training_status  = UNTRAINED;
    
    bcm_plp_an_config_t an_config_set;
    bcm_plp_an_config_t an_config_get;

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
    memset(&fw_init_params,0, sizeof(bcm_plp_aperta2_fw_init_params_t));
    memset(&aux_mode_sys_set,  0, sizeof(bcm_plp_aperta2_device_aux_modes_t));
    memset(&aux_mode_line_set, 0, sizeof(bcm_plp_aperta2_device_aux_modes_t));
    memset(&an_config_set,     0, sizeof(bcm_plp_an_config_t)); 

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
    fw_init_params.macsec_option = 0xF;  /* MacSec Bypass */
    fw_init_params.ptp_option = 0x3;     /* PTP Bypass */
    fw_init_params.octal0.sys_vco  = APERTA2_VCO_53G;
    fw_init_params.octal0.line_vco = APERTA2_VCO_53G;
    fw_init_params.octal1.sys_vco  = APERTA2_VCO_53G;
    fw_init_params.octal1.line_vco = APERTA2_VCO_53G;

    fw_init_params.sys_lane_swap.num_of_lanes = CHIP_MAX_LANES;
    /* System side lane swap can be set differently if there are swaps */
    fw_init_params.sys_lane_swap.lane_map_rx[0]  = fw_init_params.sys_lane_swap.lane_map_tx[0]  = 0;
    fw_init_params.sys_lane_swap.lane_map_rx[1]  = fw_init_params.sys_lane_swap.lane_map_tx[1]  = 1;
    fw_init_params.sys_lane_swap.lane_map_rx[2]  = fw_init_params.sys_lane_swap.lane_map_tx[2]  = 2;
    fw_init_params.sys_lane_swap.lane_map_rx[3]  = fw_init_params.sys_lane_swap.lane_map_tx[3]  = 3;
    fw_init_params.sys_lane_swap.lane_map_rx[4]  = fw_init_params.sys_lane_swap.lane_map_tx[4]  = 4;
    fw_init_params.sys_lane_swap.lane_map_rx[5]  = fw_init_params.sys_lane_swap.lane_map_tx[5]  = 5;
    fw_init_params.sys_lane_swap.lane_map_rx[6]  = fw_init_params.sys_lane_swap.lane_map_tx[6]  = 6;
    fw_init_params.sys_lane_swap.lane_map_rx[7]  = fw_init_params.sys_lane_swap.lane_map_tx[7]  = 7;
    fw_init_params.sys_lane_swap.lane_map_rx[8]  = fw_init_params.sys_lane_swap.lane_map_tx[8]  = 8;
    fw_init_params.sys_lane_swap.lane_map_rx[9]  = fw_init_params.sys_lane_swap.lane_map_tx[9]  = 9;
    fw_init_params.sys_lane_swap.lane_map_rx[10] = fw_init_params.sys_lane_swap.lane_map_tx[10] = 10;
    fw_init_params.sys_lane_swap.lane_map_rx[11] = fw_init_params.sys_lane_swap.lane_map_tx[11] = 11;
    fw_init_params.sys_lane_swap.lane_map_rx[12] = fw_init_params.sys_lane_swap.lane_map_tx[12] = 12;
    fw_init_params.sys_lane_swap.lane_map_rx[13] = fw_init_params.sys_lane_swap.lane_map_tx[13] = 13;
    fw_init_params.sys_lane_swap.lane_map_rx[14] = fw_init_params.sys_lane_swap.lane_map_tx[14] = 14;
    fw_init_params.sys_lane_swap.lane_map_rx[15] = fw_init_params.sys_lane_swap.lane_map_tx[15] = 15;

    fw_init_params.line_lane_swap.num_of_lanes = CHIP_MAX_LANES;
    /* Line side lane swap can be set differently if there are swaps */
    fw_init_params.line_lane_swap.lane_map_rx[0]  = fw_init_params.line_lane_swap.lane_map_tx[0]  = 0;
    fw_init_params.line_lane_swap.lane_map_rx[1]  = fw_init_params.line_lane_swap.lane_map_tx[1]  = 1;
    fw_init_params.line_lane_swap.lane_map_rx[2]  = fw_init_params.line_lane_swap.lane_map_tx[2]  = 2;
    fw_init_params.line_lane_swap.lane_map_rx[3]  = fw_init_params.line_lane_swap.lane_map_tx[3]  = 3;
    fw_init_params.line_lane_swap.lane_map_rx[4]  = fw_init_params.line_lane_swap.lane_map_tx[4]  = 4;
    fw_init_params.line_lane_swap.lane_map_rx[5]  = fw_init_params.line_lane_swap.lane_map_tx[5]  = 5;
    fw_init_params.line_lane_swap.lane_map_rx[6]  = fw_init_params.line_lane_swap.lane_map_tx[6]  = 6;
    fw_init_params.line_lane_swap.lane_map_rx[7]  = fw_init_params.line_lane_swap.lane_map_tx[7]  = 7;
    fw_init_params.line_lane_swap.lane_map_rx[8]  = fw_init_params.line_lane_swap.lane_map_tx[8]  = 8;
    fw_init_params.line_lane_swap.lane_map_rx[9]  = fw_init_params.line_lane_swap.lane_map_tx[9]  = 9;
    fw_init_params.line_lane_swap.lane_map_rx[10] = fw_init_params.line_lane_swap.lane_map_tx[10] = 10;
    fw_init_params.line_lane_swap.lane_map_rx[11] = fw_init_params.line_lane_swap.lane_map_tx[11] = 11;
    fw_init_params.line_lane_swap.lane_map_rx[12] = fw_init_params.line_lane_swap.lane_map_tx[12] = 12;
    fw_init_params.line_lane_swap.lane_map_rx[13] = fw_init_params.line_lane_swap.lane_map_tx[13] = 13;
    fw_init_params.line_lane_swap.lane_map_rx[14] = fw_init_params.line_lane_swap.lane_map_tx[14] = 14;
    fw_init_params.line_lane_swap.lane_map_rx[15] = fw_init_params.line_lane_swap.lane_map_tx[15] = 15;

    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.fw_init_params = &fw_init_params;

    phy_info.if_side = 0;
    phy_info.lane_map = ALL_LANE_MAP;
    phy_info.phy_addr = PHY_ID;
    printf("Initializing PHY-%d with loading internal firmware...\n", phy_info.phy_addr);
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
     * Section 2: Applying polarity configurations                                      *
     * -------------------------------------------------------------------------------- */
    retval = polarity_swap();
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get and set initial Tx/Rx polarity with ret = %d\n", retval);
        goto _aperta2_config_error;
    }

    /* -------------------------------------------------------------------------------- *
     * Section 3: Enable flow control                                                   *
     * -------------------------------------------------------------------------------- */
    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    flow_option = bcmplpFlowcontrolPassthrough;
    memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    mac_info.phy_info.phy_addr = PHY_ID;
    mac_info.phy_info.if_side = BCM_LINE_SIDE;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        mac_info.phy_info.lane_map = line_lane_map;

        retval = bcm_plp_mac_flow_control_set(CHIP_NAME, mac_info, flow_option);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to set flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",
            mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("PASS: Successfully set flow control option %d for lane-map 0x%x of PHY-%d!\n",
            flow_option, mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
        }
        retval = bcm_plp_mac_flow_control_get(CHIP_NAME, mac_info, &flow_option_get);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to get flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",
            mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("PASS: Successfully got flow control option %d for lane-map 0x%x of PHY-%d!\n",
            flow_option_get, mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
        }
        if (flow_option != flow_option_get) {
            printf("FAIL: flow options does not match for lane-map 0x%x of PHY-%d!\n",
            mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
            goto _aperta2_config_error;
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Section 4: Mode configuration                                                    *
     * It is required to configure SYSTEM side first followed by LINE side              *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        sys_lane_map  = sys_lane_map_list[lane_map_index];
        line_lane_map = line_lane_map_list[lane_map_index];

        /* Set mode configuration at System side */
        phy_info.lane_map = sys_lane_map;
        phy_info.if_side  = BCM_SYSTEM_SIDE;
        aux_mode_sys_set.lane_data_rate           = SYS_LANE_RATE;
        aux_mode_sys_set.modulation_mode          = SYS_MODULATION;
        aux_mode_sys_set.fec_mode_sel             = SYS_FEC_MODE;
        aux_mode_sys_set.port_type                = SYS_PORT_TYPE;
        aux_mode_sys_set.octal_crossing           = SYS_OCTAL_CROSSING;

        retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, SYS_PORT_SPEED, SYS_IF_TYPE, REF_CLOCK, SYS_IF_MODE, (void *)&aux_mode_sys_set);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at System side!\n",
            phy_info.lane_map, phy_info.phy_addr);
        }

        /* Set mode configuration at Line side */
        phy_info.lane_map = line_lane_map;
        phy_info.if_side  = BCM_LINE_SIDE;
        aux_mode_line_set.lane_data_rate           = LINE_LANE_RATE;
        aux_mode_line_set.modulation_mode          = LINE_MODULATION;
        aux_mode_line_set.fec_mode_sel             = LINE_FEC_MODE;
        aux_mode_line_set.port_type                = LINE_PORT_TYPE;
        aux_mode_line_set.octal_crossing           = LINE_OCTAL_CROSSING;

        retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, LINE_PORT_SPEED, LINE_IF_TYPE, REF_CLOCK, LINE_IF_MODE, (void *)&aux_mode_line_set);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at Line side!\n",
            phy_info.lane_map, phy_info.phy_addr);
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Section 5: Link training                                                         *
     * Start Force Tx training at system side                                           *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        sys_lane_map = sys_lane_map_list[lane_map_index];
        phy_info.if_side = BCM_SYSTEM_SIDE;
        phy_info.lane_map = sys_lane_map ;
        retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("Force Tx training is in progress for lane map 0x%x of PHY-%d at System side...\n",
            phy_info.lane_map, phy_info.phy_addr);
        }
    }
    sleep(5);
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
     * Start Autoneg on Line side                                                       *
     * -------------------------------------------------------------------------------- */
    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Start Autoneg :\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    an_config_set.master_lane  = AN_MASTER_LANE ; /* APERTA2 supports master_lane as 0 by default */
    an_config_set.cl72_en      = CL72_EN        ;
    an_config_set.tech_ability = TECH_ABILITY   ;
    fec_ability_set            = FEC_ABILITY    ;
    pause_ability_set          = PAUSE_ABILITY  ;

    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        phy_info.if_side  = BCM_LINE_SIDE;
        phy_info.lane_map = line_lane_map;
        retval = bcm_plp_cl73_ability_set(CHIP_NAME, phy_info, tech_ability_set, fec_ability_set, pause_ability_set, an_config_set);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to set cl73 ability for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        }

        retval = bcm_plp_cl73_set(CHIP_NAME, phy_info, ENABLE);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to enable AN for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("AN is in progress for lane map 0x%x of PHY-%d at Line side...\n",
            phy_info.lane_map, phy_info.phy_addr);
        }
    }
    sleep(5);

    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Get AN status :\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];

        phy_info.if_side  = BCM_LINE_SIDE;
        phy_info.lane_map = line_lane_map;

        tech_ability_get  = 0 ;
        fec_ability_get   = 0 ;
        pause_ability_get = 0 ;
        memset(&an_config_get, 0, sizeof(bcm_plp_an_config_t));

        retval = bcm_plp_cl73_ability_get(CHIP_NAME, phy_info, &tech_ability_get, &fec_ability_get, &pause_ability_get, &an_config_get);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to get cl73 ability for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("CL73 ability get parameters for lane-map 0x%x of PHY-%d at Line side as below :\n",
            phy_info.lane_map, phy_info.phy_addr);
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
            printf("FAIL: Failed to get AN status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("AN status for lane-map 0x%x of PHY-%d at Line side as below :\n",
                                      phy_info.lane_map, phy_info.phy_addr);
            printf("an_enable_disable = %s\n", an_ena_dis ? "AN_ENABLE" : "AN_DISABLE");
            printf("an_done           = %d\n", an_done);
        }

        /* Validate an_done */
        if (an_done != AN_GOOD) {
            retval = TEST_FAILURE;
            printf("FAIL: AN not done for lane-map 0x%x of PHY-%d at Line side!\n",
            phy_info.lane_map, phy_info.phy_addr);
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Get link status on SYS and LINE side                                             *
     * -------------------------------------------------------------------------------- */
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

    /* -------------------------------------------------------------------------------- *
     * Ports are setup now and ready to test with traffic                               *
     * -------------------------------------------------------------------------------- */
     printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
     printf("MacSec is in static bypass mode\n");
     printf("Ports are setup now and ready to test with traffic\n");
     printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

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
