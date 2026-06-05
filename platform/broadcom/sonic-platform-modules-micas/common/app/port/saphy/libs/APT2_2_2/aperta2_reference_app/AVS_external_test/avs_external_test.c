/*
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

#include "aperta2_common.h"

int main(int argc, char* argv[])
{
    int retval = TEST_SUCCESS;
    bcm_plp_access_t  phy_info;
    int p_ctxt = 5 ;
    unsigned int fw_ver = 0 ;
    unsigned int fw_crc = 0 ;

    bcm_plp_aperta2_fw_init_params_t fw_init_params;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_mac_access_t mac_info;

    /* Initialize structure instances to zero */
    memset(&fw_init_params,     0, sizeof(bcm_plp_aperta2_fw_init_params_t));
    memset(&firmware_load_type, 0, sizeof(bcm_plp_firmware_load_type_t));
    memset(&phy_info,           0, sizeof(bcm_plp_access_t));
    memset(&mac_info,           0, sizeof(bcm_plp_mac_access_t));

    phy_info.platform_ctxt = &p_ctxt;
    phy_info.lane_map = ALL_LANE_MAP;

    setLogLevel(LOG_INFO);
    {
        char log_fname[32];
        sprintf(log_fname, "%s_AVS_%s.log", CHIP_NAME, avs_i2c_resp_addr ? "I2C" : "MDIO");
        setLogFileName(log_fname, argc, argv);
    }

    printf("\n++++++++++++++++++++");
    printf("\n Connect to board :\n");
    printf("+++++++++++++++++++++\n");
    retval = device_open();
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to connect to the board (ret = %d)\n", retval);
        return retval;
    }

    printf("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    printf("\n Load FW into internal RAM:\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    fw_init_params.macsec_option = 0xf;  /* MacSec Mode */
    fw_init_params.ptp_option = 0x3;     /* PTP Bypass */
    fw_init_params.octal0.sys_vco  = APERTA2_VCO_53G;
    fw_init_params.octal0.line_vco = APERTA2_VCO_53G;
    fw_init_params.octal1.sys_vco  = APERTA2_VCO_53G;
    fw_init_params.octal1.line_vco = APERTA2_VCO_53G;

    fw_init_params.sys_lane_swap.num_of_lanes = 16;
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

    fw_init_params.line_lane_swap.num_of_lanes = 16;
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
    retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_i2c_read, mdio_i2c_write, &firmware_load_type, bcmpmFirmwareBroadcastNone);
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

    SMBus_read_regulator_status(CHIP_NAME, phy_info, REGULATOR_I2C_ADDR);

    test_close_loop_ext_regu_controller(CHIP_NAME, phy_info, REGULATOR_RAIL_ADDR, AVS_I2C_RESP_ADDR);

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
