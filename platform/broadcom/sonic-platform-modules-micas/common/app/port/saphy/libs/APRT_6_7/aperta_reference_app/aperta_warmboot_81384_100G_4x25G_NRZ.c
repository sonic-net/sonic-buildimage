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
 * macsec static bypass mode as follows:                                            *
 *   1. Connect to the board.                                                       *
 *   2. Initialize the PHY in Warmboot mode                                         *
 *   3. Close the connection to the board.                                          *
 * -------------------------------------------------------------------------------- *
 * Note 1 : This sample application is only for 81384 parts.                        *
 * -------------------------------------------------------------------------------- */

/* 100G NRZ (4x25G) MACsec static bypass mode */
#define OP_MODE_STR    "100G NRZ (4x25G) MACsec static bypass in Warmboot mode"


unsigned int sys_lane_map_list[] =  {
                                      0x0F,
                                      0xF0
                                    };

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

    /* Initialize structure instances to zero */
    memset(&mac_info, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type,0, sizeof(bcm_plp_firmware_load_type_t));

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
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadSkip;

    phy_info.if_side = 0;
    phy_info.lane_map = ALL_LANE_MAP;

    phy_info.flags = 1; /* Set Warmboot flag; Skip HW configuration */

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

    total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);
    /* Getting link status on SYS and LINE side */
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

_aperta_config_error:
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
