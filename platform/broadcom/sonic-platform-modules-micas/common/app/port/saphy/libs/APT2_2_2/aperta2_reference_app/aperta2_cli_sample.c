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

/* -------------------------------------------------------------------------------- *
 *                         Aperta2 reference application                            *
 * -------------------------------------------------------------------------------- *
 * This sample application configures the Aperta2 chip for 400G PAM4 (8x53G)        *
 * macsec static bypass mode as follows:                                            *
 *   1. Connect to the board.                                                       *
 *   2. Download the firmware and initialize the PHY in static bypass mode          *
 *   3. Configure Tx/Rx polarity                                                    *
 *   4. Configure operating mode parameters for 400G PAM4 (8x53G) mode              *
 *   5. Run the Scallop Shell (CLI command line interface)                          *
 *   6. Cleanup and close the connection to the board.                              *
 * -------------------------------------------------------------------------------- *
 * Note : This sample application is only for 85343 parts.                          *
 * -------------------------------------------------------------------------------- */
#include "aperta2_common.h"

/* PHY (MDIO) address of all ports must be listed in the array phyid_all[]  */
int  phyid_all[33] = { 0x00, -1 };

int scallop_main(int argc, char *argv[]);

/* Mode Config Defines */
/* System side parameters */
#define SYS_PORT_SPEED          400000
#define SYS_LANE_RATE           bcmplpLaneDataRate_53P125G
#define SYS_MODULATION          bcmplpModulationPAM4
#define SYS_FEC_MODE            bcmplpaperta2RS544_2XN
#define SYS_IF_TYPE             bcm_pm_InterfaceKR
#define SYS_IF_MODE             bcmplpInterfaceModeIEEE
#define SYS_PORT_TYPE           bcmplpPortTypePassthrough
#define SYS_OCTAL_CROSSING      bcmplpNoOctalCrossing

/* System side lanemaps */
unsigned int sys_lane_map_list[] =  {
    0x00FF, 0xFF00
};

/* Line side parameters */
#define LINE_PORT_SPEED         400000
#define LINE_LANE_RATE          bcmplpLaneDataRate_53P125G
#define LINE_MODULATION         bcmplpModulationPAM4
#define LINE_FEC_MODE           bcmplpaperta2RS544_2XN
#define LINE_IF_TYPE            bcm_pm_InterfaceKR
#define LINE_IF_MODE            bcmplpInterfaceModeIEEE
#define LINE_PORT_TYPE          bcmplpPortTypePassthrough
#define LINE_OCTAL_CROSSING     bcmplpNoOctalCrossing

/* Line side lanemaps */
unsigned int line_lane_map_list[] = {
    0x00FF, 0xFF00
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
     * Section 1: PHY, MACsec Initialization and MACsec Configuration                   *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Initialize the PHYs and download firmware based on the test setup and            *
     * configure the macsec in static bypass mode                                       *
     * -------------------------------------------------------------------------------- */
    fw_init_params.macsec_option = 0xF;  /* MacSec Bypass */
    fw_init_params.ptp_option = 0;       /* PTP Enabled   */
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

    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    bcm_plp_mac_flow_control_t flow_option, flow_option_get;
    /* -------------------------------------------------------------------------------- *
     * Section 3: Enable flow control                                                   *
     * -------------------------------------------------------------------------------- */
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
        aux_mode_line_set.ing_fixed_latency.enable       = 1;
        aux_mode_line_set.ing_fixed_latency.dp_ck_cycles = 205; /* Refer to section 13.5: Recommended Fixed latency Setting of the user guide. */
        aux_mode_line_set.egr_fixed_latency.enable       = 1;
        aux_mode_line_set.egr_fixed_latency.dp_ck_cycles = 205; /* Refer to section 13.5: Recommended Fixed latency Setting of the user guide. */
        aux_mode_line_set.ts_config                      = bcmplpPTPPmEnabled;

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
     * Mode config get                                                                  *
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
            if((SYS_IF_TYPE != if_type_get) 
                && (SYS_PORT_SPEED != speed_get) 
                && (SYS_IF_MODE != if_mode_get) 
                && (REF_CLOCK != ref_clk_get) 
                /*&& (SYS_LANE_RATE != aux_mode_get.lane_data_rate) */
                && (SYS_MODULATION != aux_mode_get.modulation_mode) 
                && (SYS_FEC_MODE != aux_mode_get.fec_mode_sel)
                && (SYS_PORT_TYPE != aux_mode_get.port_type) 
                && (SYS_OCTAL_CROSSING != aux_mode_get.octal_crossing) 
                && (aux_mode_sys_set.ing_fixed_latency.enable != aux_mode_get.ing_fixed_latency.enable)                
                && (aux_mode_sys_set.egr_fixed_latency.enable != aux_mode_get.egr_fixed_latency.enable))                 
            {
                printf("%s_SIDE: bcm_plp_mode_config_get/set comparision failed\n", phy_info.if_side?"SYS":"LINE");
            }
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
            if((LINE_IF_TYPE != if_type_get) 
                && (LINE_PORT_SPEED != speed_get) 
                && (LINE_IF_MODE != if_mode_get) 
                && (REF_CLOCK != ref_clk_get) 
                /*&& (LINE_LANE_RATE != aux_mode_get.lane_data_rate) */
                && (LINE_MODULATION != aux_mode_get.modulation_mode) 
                && (LINE_FEC_MODE != aux_mode_get.fec_mode_sel)
                && (LINE_PORT_TYPE != aux_mode_get.port_type) 
                && (LINE_OCTAL_CROSSING != aux_mode_get.octal_crossing) 
                && (aux_mode_line_set.ing_fixed_latency.enable != aux_mode_get.ing_fixed_latency.enable)                
                && (aux_mode_line_set.egr_fixed_latency.enable != aux_mode_get.egr_fixed_latency.enable))                 
            {
                printf("%s_SIDE: bcm_plp_mode_config_get/set comparision failed\n", phy_info.if_side?"SYS":"LINE");
            }            
            printf("ing_fixed_latency.enable:%d\n", aux_mode_line_set.ing_fixed_latency.enable);
            printf("ing_fixed_latency.dp_ck_cycles:%d\n", aux_mode_line_set.ing_fixed_latency.dp_ck_cycles);
            printf("egr_fixed_latency.enable:%d\n", aux_mode_line_set.egr_fixed_latency.enable);
            printf("egr_fixed_latency.dp_ck_cycles:%d\n", aux_mode_line_set.egr_fixed_latency.dp_ck_cycles);
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Start Force Tx training at Line side                                             *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        phy_info.if_side = BCM_LINE_SIDE;
        phy_info.lane_map = line_lane_map ;
        retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, retval);
            goto _aperta2_config_error;
        } else {
            printf("Force Tx training is in progress for lane map 0x%x of PHY-%d at Line side...\n",
            phy_info.lane_map, phy_info.phy_addr);
        }
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
    }
    sleep(5);
    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training status at Line side                                        *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        phy_info.if_side  = BCM_LINE_SIDE;
        phy_info.lane_map = line_lane_map;

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

    /* -------------------------------------------------------------------------------- *
     * run the Scallop CLI Shell                                                        *
     * -------------------------------------------------------------------------------- */
    printf("\n========  %s CLI Reference App  ========\n", CHIP_NAME);
    scallop_main(argc, argv);

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
