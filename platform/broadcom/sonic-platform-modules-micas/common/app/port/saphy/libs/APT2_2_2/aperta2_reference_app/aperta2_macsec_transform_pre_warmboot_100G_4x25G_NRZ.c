/*
 * $Copyright: (c) 2024 Broadcom.
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
#include <aperta2_macsec_common.h>

/* -------------------------------------------------------------------------------- *
 *                         Aperta2 reference application                             *
 * -------------------------------------------------------------------------------- *
 * This sample application configures the Aperta2 chip for 100G (4x25G)              *
 * macsec transform mode as follows:                                                *
 *   1. Connect to the board.                                                       *
 *   2. Initialize the PHY and download the firmware                                *
 *   3. Register callbacks for subsequent warmboot                                  *
 *   3. Initialize MACsec in bypass disable mode                                    *
 *   4. Configure Tx/Rx polarity                                                    *
 *   5. Configure operating mode parameters for 100G NRZ (4x25G) mode               *
 *   6. Setup link training                                                         *
 *   7. Setup SA/vport for macsec transformation & macsec validation                *
 *   9. Statistics/Dump operations                                                  *
 *   10.Shutdown macsec software resources, let Hardware be up and operational      *
 * -------------------------------------------------------------------------------- *
 * Please Note : This sample application is only for 85343 parts.                   *
 * -------------------------------------------------------------------------------- */

/* 100G NRZ (4x25G) MACsec mode */
#define OP_MODE_STR    "100G NRZ (4x25G) MACsec Pre-Warmboot mode"

/* Enable Tx forced training */
#undef LINK_TRAINING_EN


#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1


/* System side parameters */
#define SYS_PORT_SPEED          100000
#define SYS_LANE_RATE           bcmpLplaneDataRate_25P78125G
#define SYS_MODULATION          bcmplpModulationNRZ
#define SYS_FEC_MODE            bcmplpaperta2NoFEC
#define SYS_IF_TYPE             bcm_pm_InterfaceCAUI4_C2C
#define SYS_IF_MODE             bcmplpInterfaceModeIEEE 
#define SYS_PORT_TYPE           bcmplpPortTypePassthrough
#define SYS_OCTAL_CROSSING      bcmplpNoOctalCrossing
unsigned int sys_lane_map_list[] =  {
                                      0x000F, 0x00F0, 0x0F00, 0xF000
                                    };

/* Line side parameters */
#define LINE_PORT_SPEED         100000
#define LINE_LANE_RATE          bcmpLplaneDataRate_25P78125G
#define LINE_MODULATION         bcmplpModulationNRZ
#define LINE_FEC_MODE           bcmplpaperta2NoFEC
#define LINE_IF_TYPE            bcm_pm_InterfaceCAUI4_C2M
#define LINE_IF_MODE            bcmplpInterfaceModeIEEE 
#define LINE_PORT_TYPE          bcmplpPortTypePassthrough
#define LINE_OCTAL_CROSSING     bcmplpNoOctalCrossing

unsigned int line_lane_map_list[] = {
                                      0x000F, 0x00F0, 0x0F00, 0xF000
                                    };

#define APP_MACSEC_WARMBOOT_NOF_AREAS 20

FILE *fd[APP_MACSEC_WARMBOOT_NOF_AREAS] = {NULL};
FILE *mfptr = NULL;

int phy_area[APP_MACSEC_WARMBOOT_NOF_AREAS];


bcm_plp_warmboot_status_t app_macsec_alloc(const bcm_plp_sec_phy_access_t *pa,
                                           const unsigned int storage_byte_count,
                                           unsigned int *const area_id_p);

bcm_plp_warmboot_status_t app_macsec_read(bcm_plp_sec_phy_access_t *pa,
                                          const unsigned int area_id,
                                          unsigned char *const data_p,
                                          const unsigned int byte_offset,
                                          const unsigned int byte_count);

bcm_plp_warmboot_status_t app_macsec_write(bcm_plp_sec_phy_access_t *pa,
                                           const unsigned int area_id,
                                           const unsigned char *const data_p,
                                           const unsigned int byte_offset,
                                           const unsigned int byte_count);

bcm_plp_warmboot_status_t app_macsec_free(bcm_plp_sec_phy_access_t *pa,
                                          const unsigned int area_id);

extern char* itoa(int nn, char* xstr, int usebase);


/* Main entry to application */
int main(int argc, char *argv[])
{
    int index;
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int side = 0;
    int macsec_side = 0;
    int lane_map_index;
    int total_lane_maps;
    int device_bypass_enable = 0;
    unsigned int link_status_get;
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;
    int area_id = 0;
    int paddr, mside, lmap;
    int fret = -1;

    bcm_plp_sec_phy_access_t sec_info;
    bcm_plp_mac_access_t mac_info;
    bcm_plp_access_t phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_mac_flow_control_t flow_option;
    bcm_laneswap_map_t sys_laneswap_map, line_laneswap_map;
    bcm_plp_aperta2_device_aux_modes_t aux_mode_sys_set, aux_mode_line_set;
    bcm_plp_cfye_device_exceptions_t exception;
    bcm_plp_cfye_device_control_t dev_control;
    bcm_plp_cfye_device_t dev_init;
    bcm_plp_macsec_warmboot_callbacks_t macsec_cb;
    bcm_plp_aperta2_fw_init_params_t fw_init_params;

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
    memset(&fw_init_params,     0, sizeof(bcm_plp_aperta2_fw_init_params_t));
    memset(&sys_laneswap_map,  0, sizeof(bcm_laneswap_map_t));
    memset(&line_laneswap_map, 0, sizeof(bcm_laneswap_map_t));
    memset(&aux_mode_sys_set,  0, sizeof(bcm_plp_aperta2_device_aux_modes_t));
    memset(&aux_mode_line_set, 0, sizeof(bcm_plp_aperta2_device_aux_modes_t));
    memset(&exception, 0, sizeof(bcm_plp_cfye_device_exceptions_t));
    memset(&dev_control, 0, sizeof(bcm_plp_cfye_device_control_t));
    memset(&dev_init, 0, sizeof(bcm_plp_cfye_device_t));
    memset(&macsec_cb, 0, sizeof(bcm_plp_macsec_warmboot_callbacks_t));

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
    fw_init_params.macsec_option = 0x0;  /* MacSec Mode */
    fw_init_params.ptp_option = 0x3;     /* PTP Bypass */
    fw_init_params.octal0.sys_vco  = APERTA2_VCO_51G;
    fw_init_params.octal0.line_vco = APERTA2_VCO_51G;
    fw_init_params.octal1.sys_vco  = APERTA2_VCO_51G;
    fw_init_params.octal1.line_vco = APERTA2_VCO_51G;

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

    phy_info.if_side = BCM_LINE_SIDE;
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
     * Register Warmboot for Macsec                                                     *
     * -------------------------------------------------------------------------------- */
    /*
     * Save the PHY/Port/Ingress/Egress mapping to Warmboot areas, to be
     * saved and restored.
     */
    mfptr = fopen(WARMBOOT_MASTER_FILE, "w+");
    if (mfptr == NULL) {
        printf("FAIL: fopen of %s!\n", WARMBOOT_MASTER_FILE);
        goto _aperta2_init_error;
    }

    macsec_cb.alloc_cb = (bcm_plp_warmboot_alloc_callback_t)app_macsec_alloc;
    macsec_cb.free_cb = (bcm_plp_warmboot_free_callback_t)app_macsec_free;
    macsec_cb.read_cb = (bcm_plp_warmboot_read_callback_t)app_macsec_read;
    macsec_cb.write_cb = (bcm_plp_warmboot_write_callback_t)app_macsec_write;

    phy_info.lane_map = OCTAL0_LANE_MAP;
    phy_info.phy_addr = PHY_ID;
    memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    sec_info.macsec_side = 0; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
    retval = bcm_plp_macsec_warmboot_register(CHIP_NAME, &sec_info, &macsec_cb);
    if (retval) {
        printf("FAIL: bcm_plp_macsec_warmboot_register (ret = %d)!\n", retval);
        goto _aperta2_init_error;
    } else {
        printf("Success : bcm_plp_macsec_warmboot_register\n");
    }

    /*
     * Initialize the Persistent memory managing meta data to -1
     */
    for (index = 0; index < APP_MACSEC_WARMBOOT_NOF_AREAS; index++) {
        phy_area[index] = -1;
        fd[index] = NULL;
    }
    /* -------------------------------------------------------------------------------- *
     * Initialize MACsec CfyE and SecY for both Octals                                  *
     * -------------------------------------------------------------------------------- */
    phy_info.lane_map = OCTAL0_LANE_MAP;
    device_bypass_enable = 0;
    phy_info.phy_addr = PHY_ID;
    memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (macsec_side = 0; macsec_side < 2; macsec_side++) {
        sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
        /* device_bypass_enable is set to 0, to trigger macsec transformation & validation */
        retval = macsec_ipsec_initialize(sec_info, device_bypass_enable);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: MACSec Initialize for PHY-ID[%d], macsec_side[%d], return code [%d]\n",
                    phy_info.phy_addr, sec_info.macsec_side, retval);
            goto _aperta2_macsec_init_error;
        } else {
            printf("PASS: MACSec Initialize for PHY-ID[%d], macsec_side [%d] \n",
                    phy_info.phy_addr, sec_info.macsec_side);
        }
    }
    phy_info.lane_map = OCTAL1_LANE_MAP;
    device_bypass_enable = 0;
    phy_info.phy_addr = PHY_ID;
    memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (macsec_side = 0; macsec_side < 2; macsec_side++) {
        sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
        /* device_bypass_enable is set to 0, to trigger macsec transformation & validation */
        retval = macsec_ipsec_initialize(sec_info, device_bypass_enable);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: MACSec Initialize for PHY-ID[%d], macsec_side[%d], return code [%d]\n",
                    phy_info.phy_addr, sec_info.macsec_side, retval);
            goto _aperta2_macsec_init_error;
        } else {
            printf("PASS: MACSec Initialize for PHY-ID[%d], macsec_side [%d] \n",
                    phy_info.phy_addr, sec_info.macsec_side);
        }
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
    }

   total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    /* -------------------------------------------------------------------------------- *
     * Section 4: Mode configuration                                                    *
     * -------------------------------------------------------------------------------- */
    /* -------------------------------------------------------------------------------- *
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
     * Section 4: Link training                                                         *
     * -------------------------------------------------------------------------------- */
#ifdef LINK_TRAINING_EN
    /* -------------------------------------------------------------------------------- *
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
#endif

    /* -------------------------------------------------------------------------------- *
     * Section 6: MACsec Configuration                                                  *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Add vport                                                                        * 
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        sys_lane_map  = sys_lane_map_list[lane_map_index];
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side;
            phy_info.if_side = macsec_side ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE; /* Mandatory field in case of Octal Xing*/
            phy_info.lane_map = macsec_side ? line_lane_map : sys_lane_map;   /* Mandatory field in case of Octal Xing*/
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t)); 
            retval = macsec_add_vport(sec_info);
            if (retval != TEST_SUCCESS) {
                goto _aperta2_config_error;
            }
        }
    }
    /* -------------------------------------------------------------------------------- *
     * Add SA with transform record                                                     *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        sys_lane_map  = sys_lane_map_list[lane_map_index];
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side;
            phy_info.if_side = macsec_side ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE; /* Mandatory field in case of Octal Xing*/
            phy_info.lane_map = macsec_side ? line_lane_map : sys_lane_map;   /* Mandatory field in case of Octal Xing*/
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t)); 
            retval = macsec_add_sa_with_transform_record(sec_info, POLICY_BYPASS_NONE);
            if (retval != TEST_SUCCESS) {
                goto _aperta2_config_error;
            }
        }
    }
    /* -------------------------------------------------------------------------------- *
     * Add rule policy                                                                  *
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        sys_lane_map  = sys_lane_map_list[lane_map_index];
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side;
            phy_info.if_side = macsec_side ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE; /* Mandatory field in case of Octal Xing*/
            phy_info.lane_map = macsec_side ? line_lane_map : sys_lane_map;   /* Mandatory field in case of Octal Xing*/
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t)); 
            retval = macsec_add_rule_policy(sec_info, POLICY_BYPASS_NONE);
            if (retval != TEST_SUCCESS) {
                goto _aperta2_config_error;
            }
        }
    } 
    /* -------------------------------------------------------------------------------- *
     * Dump all rules configured for both Octals                                        * 
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    phy_info.lane_map = OCTAL0_LANE_MAP;
    for (macsec_side = 0; macsec_side < 2; macsec_side++) {
        sec_info.macsec_side = macsec_side;
        phy_info.if_side = macsec_side ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE; /* Mandatory field in case of Octal Xing*/
        phy_info.lane_map = OCTAL0_LANE_MAP;                              /* Mandatory field in case of Octal Xing*/
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t)); 
        retval = bcm_plp_cfye_diag_rule_dump(CHIP_NAME, &sec_info, NULL, 1);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to dump rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta2_config_error;
        } else {
            printf("PASS: Successfully dump rule for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
    } 
    phy_info.phy_addr = PHY_ID;
    phy_info.lane_map = OCTAL1_LANE_MAP;
    for (macsec_side = 0; macsec_side < 2; macsec_side++) {
        sec_info.macsec_side = macsec_side;
        phy_info.if_side = macsec_side ? BCM_LINE_SIDE : BCM_SYSTEM_SIDE; /* Mandatory field in case of Octal Xing*/
        phy_info.lane_map = OCTAL1_LANE_MAP;                              /* Mandatory field in case of Octal Xing*/
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t)); 
        retval = bcm_plp_cfye_diag_rule_dump(CHIP_NAME, &sec_info, NULL, 1);
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to dump rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            goto _aperta2_config_error;
        } else {
            printf("PASS: Successfully dump rule for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
    } 

    /* -------------------------------------------------------------------------------- *
     * Section 7: Port status and Send the traffic                                      *
     * -------------------------------------------------------------------------------- */
    /* -------------------------------------------------------------------------------- *
     * Check Link Status of Ports                                                        * 
     * -------------------------------------------------------------------------------- */
    phy_info.phy_addr = PHY_ID;
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        line_lane_map = line_lane_map_list[lane_map_index];
        sys_lane_map  = sys_lane_map_list[lane_map_index];
        for (side = BCM_SYSTEM_SIDE; side >= BCM_LINE_SIDE; side--) {
            phy_info.if_side  = side;
            phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map ;
            retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                goto _aperta2_config_error;
            }
            retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                goto _aperta2_config_error;
            }
            printf("Link status for lane-map 0x%x of PHY-%d at %s side is : %d)!\n",
            phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", link_status_get);
            /* Validate the link status */
            if (LINK_ON != link_status_get) {
                retval = TEST_FAILURE;
                printf("Link status for lane-map 0x%x of PHY-%d at %s side is not as expected (expected: %d, actual: %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", LINK_ON, link_status_get);
                goto _aperta2_config_error;
            }
        }
    }
    /* -------------------------------------------------------------------------------- *
     * Ports are setup now and ready to test with traffic                               *
     * -------------------------------------------------------------------------------- */
     printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
     printf("Ports are MACSec enabled and UP. Please send the traffic\n");
     printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

    /* -------------------------------------------------------------------------------- *
     * Gracefully Shutdown Macsec Software Resources(Device keeps running)              *
     * Use this only if you intentionally need to shutdown the driver host to           *
     * Warmboot subsequently. The Driver also restores macsec config upon               *
     * unintended shutdowns, during a subsequent Warmboot                               *
     * -------------------------------------------------------------------------------- */
    /*
     * Close to flush written mappings
     */
    if (mfptr) {
        fclose(mfptr);
    }

    /*
     * Re-open to shutdown per area
     */
    mfptr = fopen(WARMBOOT_MASTER_FILE, "r");
    if (!mfptr) {
        printf("\n could not open %s\n", WARMBOOT_MASTER_FILE);
        goto _aperta2_init_error;
    }

    fret = fscanf(mfptr, "%1d\n%1d\n%1d\n%20x\n", &area_id, &paddr, &mside, &lmap);
    while (fret != EOF) {
        phy_info.lane_map = lmap;
        phy_info.phy_addr = paddr;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        sec_info.macsec_side = mside;
        printf("\nStarting warmboot_shutdown for area:%d\n", area_id);
        retval = bcm_plp_macsec_warmboot_shutdown(CHIP_NAME, &sec_info, area_id);
        if (retval) {
            printf("\nFAIL: bcm_plp_macsec_warmboot_shutdown for PHY-%d (ret = %d) area:%d !\n", phy_info.phy_addr, retval, area_id);
            goto _aperta2_init_error;
        } else {
            printf("\nSuccess : bcm_plp_macsec_warmboot_shutdown for PHY-%d: area:%d\n", phy_info.phy_addr, area_id);
        }
        fret = fscanf(mfptr, "%1d\n%1d\n%1d\n%20x\n", &area_id, &paddr, &mside, &lmap);
    }

    /*
     * Close to flush written mappings
     */
    if (mfptr)
        fclose(mfptr);

_aperta2_config_error:
    /* -------------------------------------------------------------------------------- *
     * Do Not Uninitialize MACsec CfyE and SecY if you Warmboot later                   *
     * -------------------------------------------------------------------------------- */

_aperta2_macsec_init_error:

    /* -------------------------------------------------------------------------------- *
     * No PHY and MAC Cleanup due to subsequent Warmboot                                *
     * -------------------------------------------------------------------------------- */
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


/*
 * Sample alloc function creates files to persist macsec config to be saved
 * during a cold boot and restored during a warmboot
 */
bcm_plp_warmboot_status_t app_macsec_alloc(const bcm_plp_sec_phy_access_t *pa,
                                           const unsigned int storage_byte_count,
                                           unsigned int *const area_id_p)
{
    unsigned int i;
    char *data_p;

    for (i=0; i<APP_MACSEC_WARMBOOT_NOF_AREAS; i++) {
        if (fd[i] == NULL) {
            char buf[4] = {'\0'};
            char filename[FILENAME_MAX_LENGTH] = {'\0'};

            sprintf(filename, "%s%s", WARMBOOT_FILE_PREFIX, itoa(i,buf,10));

            fd[i] = fopen(filename, "w+");

            if (fd[i] == NULL) {
                /* Allocation failed. */
                printf("\n open of file %s failed with error \n", filename);
                perror("Error : ");
                return BCM_PLP_WARMBOOT_ERROR_ALLOCATION;
            } else {
                *area_id_p = i;
                /* Initialized the memory with default 0 */
                data_p = (char *)calloc(storage_byte_count,sizeof(char));
                if(data_p == NULL){
                    printf("memory allocation failed \n");
                    return BCM_PLP_WARMBOOT_ERROR_ALLOCATION;
                }
                fwrite(data_p, 1, storage_byte_count,  fd[i]);
                /* free temporary allocated memory */
                free(data_p);
                phy_area[i] = pa->phy_info.phy_addr;
                fprintf(mfptr, "%d\n%d\n%d\n%0x\n", i, pa->phy_info.phy_addr, pa->macsec_side, pa->phy_info.lane_map);
                return BCM_PLP_WARMBOOT_STATUS_OK;
            }
        }
    }
    return BCM_PLP_WARMBOOT_INTERNAL_ERROR;
}

/*
 * Sample read callback function reads from files to restore config
 * during a warmboot
 */
bcm_plp_warmboot_status_t app_macsec_read(bcm_plp_sec_phy_access_t *pa,
                                          const unsigned int area_id,
                                          unsigned char *const data_p,
                                          const unsigned int byte_offset,
                                          const unsigned int byte_count)
{
    int rv = 0;
    char filename[FILENAME_MAX_LENGTH] = {'\0'};
    if (area_id >= APP_MACSEC_WARMBOOT_NOF_AREAS) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }
    if (fd[area_id] == NULL) {
        sprintf(filename, "%s%d", WARMBOOT_FILE_PREFIX,area_id);
        fd[area_id] = fopen(filename, "r+");
    }
    if (fd[area_id] == NULL || data_p == NULL) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }


    fseek(fd[area_id], byte_offset, SEEK_SET);

    rv = fread(data_p, 1, byte_count, fd[area_id]);
    if(rv <= 0) {
        printf("File read failed error: %d \n", rv);
        return BCM_PLP_WARMBOOT_INTERNAL_ERROR;
    }
    return BCM_PLP_WARMBOOT_STATUS_OK;
}


/*
 * Sample read callback function writes into files to restore config
 * during a warmboot
 */
bcm_plp_warmboot_status_t app_macsec_write(bcm_plp_sec_phy_access_t *pa,
                                           const unsigned int area_id,
                                           const unsigned char *const data_p,
                                           const unsigned int byte_offset,
                                           const unsigned int byte_count)
{
    if (area_id >= APP_MACSEC_WARMBOOT_NOF_AREAS) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }

    if (fd[area_id] == NULL || data_p == NULL) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }

    fseek(fd[area_id], byte_offset, SEEK_SET);
    fwrite(data_p, 1, byte_count,  fd[area_id]);

    return BCM_PLP_WARMBOOT_STATUS_OK;
}

/*
 * Sample write callback function writes to files to save config
 * before a warmboot
 */
bcm_plp_warmboot_status_t app_macsec_free(bcm_plp_sec_phy_access_t *pa,
                                          const unsigned int area_id)
{
    if (area_id >= APP_MACSEC_WARMBOOT_NOF_AREAS) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }

    if (fd[area_id] == NULL) {
        return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER;
    }
    fflush(fd[area_id]);

    fclose(fd[area_id]);
    fd[area_id] = NULL;


    return BCM_PLP_WARMBOOT_STATUS_OK;
}
