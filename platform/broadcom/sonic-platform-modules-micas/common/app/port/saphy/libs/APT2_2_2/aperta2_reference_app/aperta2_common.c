/*
 * $Copyright: (c) 2023 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * BCM APIs. This program might not work for all environments.
 */

#ifndef _APERTA2_COMMON_C_
#define _APERTA2_COMMON_C_

/* Includes */
#include <aperta2_common.h>
#if defined(USE_FTDI)
/* Global variables */
FT_HANDLE ftHandle = {NULL};
#endif

/* Configure polarity settings */
int polarity_swap(void)
{
    unsigned int tx_pol = 0, rx_pol = 0, read_tx_pol = 0, read_rx_pol = 0;
    int retval = 0;

    bcm_plp_access_t phy_info;

    phy_info.platform_ctxt = &p_ctxt;

    phy_info.phy_addr = PHY_ID;
    phy_info.lane_map = ALL_LANE_MAP;
    phy_info.if_side = BCM_SYSTEM_SIDE;
    /* Reading existing polarity settings */
    retval = bcm_plp_polarity_get(CHIP_NAME, phy_info, &read_tx_pol, &read_rx_pol);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get the polarity for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
        phy_info.lane_map, phy_info.phy_addr, retval);
        return retval;
    } else {
        printf("PASS: Successfully got the existing polarity for lane-map 0x%x of PHY-%d at System side! (tx_pol_rd : 0x%x, rx_pol_rd : 0x%x)\n",
        phy_info.lane_map, phy_info.phy_addr, read_tx_pol, read_rx_pol);
    }
    /* Applying polarity swap configuration according to Board design */
    tx_pol = read_tx_pol;
    rx_pol = read_rx_pol;
    retval = bcm_plp_polarity_set(CHIP_NAME, phy_info, tx_pol, rx_pol);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to set the polarity for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
        phy_info.lane_map, phy_info.phy_addr, retval);
        return retval;
    } else {
        printf("PASS: Successfully set the polarity (tx_pol : 0x%x, rx_pol : 0x%x) for lane-map 0x%x of PHY-%d at System side!\n",
        tx_pol, rx_pol, phy_info.lane_map, phy_info.phy_addr);
    }

    phy_info.phy_addr = PHY_ID;
    phy_info.lane_map = ALL_LANE_MAP;
    phy_info.if_side = BCM_LINE_SIDE;
    /* Reading existing polarity settings */
    retval = bcm_plp_polarity_get(CHIP_NAME, phy_info, &read_tx_pol, &read_rx_pol);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get the polarity for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
        phy_info.lane_map, phy_info.phy_addr, retval);
        return retval;
    } else {
        printf("PASS: Successfully got the existing polarity for lane-map 0x%x of PHY-%d at Line side! (tx_pol_rd : 0x%x, rx_pol_rd : 0x%x)\n",
        phy_info.lane_map, phy_info.phy_addr, read_tx_pol, read_rx_pol);
    }

    /* Applying polarity swap configuration according to Board design */
    tx_pol = read_tx_pol ^ (0x1<<13); /* 13th is toggled */
    rx_pol = read_rx_pol ^ (0x1<<3 | 0x1<<6 | 0x1<<7); /* 3rd, 6th and 7th bit value is toggled */
    retval = bcm_plp_polarity_set(CHIP_NAME, phy_info, tx_pol, rx_pol);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to set the polarity for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
        phy_info.lane_map, phy_info.phy_addr, retval);
        return retval;
    } else {
        printf("PASS: Successfully set the polarity (tx_pol : 0x%x, rx_pol : 0x%x) for lane-map 0x%x of PHY-%d at Line side!\n",
        tx_pol, rx_pol, phy_info.lane_map, phy_info.phy_addr);
    }
    return retval;
}

/* Get polarity settings */
int get_polarity(void)
{
    unsigned int read_tx_pol = 0, read_rx_pol = 0;
    int retval = 0;

    bcm_plp_access_t phy_info;

    phy_info.platform_ctxt = &p_ctxt;

    phy_info.phy_addr = PHY_ID;
    phy_info.lane_map = ALL_LANE_MAP;
    phy_info.if_side = BCM_SYSTEM_SIDE;
    /* Reading existing polarity settings */
    retval = bcm_plp_polarity_get(CHIP_NAME, phy_info, &read_tx_pol, &read_rx_pol);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get the polarity for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
        phy_info.lane_map, phy_info.phy_addr, retval);
        return retval;
    } else {
        printf("PASS: Successfully got the existing polarity for lane-map 0x%x of PHY-%d at System side! (tx_pol_rd : 0x%x, rx_pol_rd : 0x%x)\n",
        phy_info.lane_map, phy_info.phy_addr, read_tx_pol, read_rx_pol);
    }

    phy_info.phy_addr = PHY_ID;
    phy_info.lane_map = ALL_LANE_MAP;
    phy_info.if_side = BCM_LINE_SIDE;
    /* Reading existing polarity settings */
    retval = bcm_plp_polarity_get(CHIP_NAME, phy_info, &read_tx_pol, &read_rx_pol);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to get the polarity for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
        phy_info.lane_map, phy_info.phy_addr, retval);
        return retval;
    } else {
        printf("PASS: Successfully got the existing polarity for lane-map 0x%x of PHY-%d at Line side!   (tx_pol_rd : 0x%x, rx_pol_rd : 0x%x)\n",
        phy_info.lane_map, phy_info.phy_addr, read_tx_pol, read_rx_pol);
    }

    return retval;
}

/* Get RXTX Laneswap configuration */
int get_txrx_laneswap(void)
{
    bcm_laneswap_map_t sys_laneswap_map_get;
    bcm_laneswap_map_t line_laneswap_map_get;
    int i = 0;
    int retval = 0;

    bcm_plp_access_t phy_info;
    phy_info.platform_ctxt = &p_ctxt;

    printf("Get logical laneswap information :");
    phy_info.phy_addr = PHY_ID;
    phy_info.if_side = SYS_SIDE;
    memset(&sys_laneswap_map_get, 0, sizeof(bcm_plp_logical_lane_map_t));
    retval = bcm_plp_rxtx_laneswap_get(CHIP_NAME, phy_info, &sys_laneswap_map_get);
    if (retval != 0) {
        printf("FAIL: Failed to get RXTX Laneswap for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
        phy_info.lane_map, phy_info.phy_addr, retval);
        return retval;
    } else {
        printf("PASS: RXTX Laneswap passed for lane-map 0x%x of PHY-%d at System side!\n",
        phy_info.lane_map, phy_info.phy_addr);
        printf("RX\t");
        for (i = 0; i < sys_laneswap_map_get.num_of_lanes; i++) {
            printf("%2d\t", sys_laneswap_map_get.lane_map_rx[i]);
        }
        printf("\nTX\t");
        for (i = 0; i < sys_laneswap_map_get.num_of_lanes; i++) {
            printf("%2d\t", sys_laneswap_map_get.lane_map_tx[i]);
        }
        printf("\n");
    }

    phy_info.if_side = LINE_SIDE;
    memset(&line_laneswap_map_get, 0, sizeof(bcm_plp_logical_lane_map_t));
    retval = bcm_plp_rxtx_laneswap_get(CHIP_NAME, phy_info, &line_laneswap_map_get);
    if (retval != 0) {
        printf("FAIL: Failed to get RXTX Laneswap for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
        phy_info.lane_map, phy_info.phy_addr, retval);
        return retval;
    } else {
        printf("PASS: RXTX Laneswap passed for lane-map 0x%x of PHY-%d at Line side!\n",
        phy_info.lane_map, phy_info.phy_addr);
        printf("RX\t");
        for (i = 0; i < line_laneswap_map_get.num_of_lanes; i++) {
            printf("%2d\t", line_laneswap_map_get.lane_map_rx[i]);
        }
        printf("\nTX\t");
        for (i = 0; i < line_laneswap_map_get.num_of_lanes; i++) {
            printf("%2d\t", line_laneswap_map_get.lane_map_tx[i]);
        }
        printf("\n");
    }
    return retval;
}

/* Function to read data from MDIO interface */
int mdio_read(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int *data)
{
#if defined(USE_FTDI)
    int ftStatus;
    DWORD BytesRW;
    unsigned char port_addr;
    unsigned char sbuf[6] = { 0 };
    unsigned char rbuf[2] = { 0 };
    unsigned char dev_addr = (reg_addr >> 16) & 0x1f;
    FT_HANDLE read_ftHandle;

    if (!user_acc)
        return TEST_FAILURE;

    read_ftHandle = ftHandle;
    port_addr = (unsigned char) mdio_addr;

    sbuf[0] = port_addr >> 1;
    sbuf[1] = ((port_addr & 1) << 7) | ((dev_addr & 0x1F) << 2) | 2;
    sbuf[2] = (reg_addr >> 8);
    sbuf[3] = (reg_addr & 0xFF);
    sbuf[4] = sbuf[0] | 0x30;
    sbuf[5] = sbuf[1];

    ftStatus = FT_Write(read_ftHandle, sbuf, sizeof(sbuf), &BytesRW);
    if (ftStatus == FT_OK) {
        ftStatus = FT_Read(read_ftHandle, rbuf, sizeof(rbuf), &BytesRW);
        if (ftStatus == FT_OK) {
            *data = rbuf[0];
            *data <<= 8;
            *data |= rbuf[1];
        }
    }
#ifdef BCM_PLP_MDIO_TRACE_READ_ENABLE
    printf( "MDIO_TRACE: READ Register Address: [0x%x] Data:[0x%x] MDIO Address: [0x%x] \n", reg_addr, *data, mdio_addr);
#endif
    return ftStatus;
#else
    /* Implement application specific 'read' mechanism */
    return BCM_PM_IF_UNAVAIL;
#endif
}

/* Function to write data to MDIO interface */
int mdio_write(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int data)
{
#if defined(USE_FTDI)
    int ftStatus;
    DWORD BytesRW;
    unsigned char sbuf[8] = { 0 };
    unsigned char dev_addr = (reg_addr >> 16) & 0x1f;
    unsigned char port_addr;
    FT_HANDLE write_ftHandle;

    if (!user_acc)
        return TEST_FAILURE;

    write_ftHandle = ftHandle;

#ifdef BCM_PLP_MDIO_TRACE_WRITE_ENABLE
    printf( "MDIO_TRACE: WRITE Register Address: [0x%x] Data:[0x%x] MDIO Address: [0x%x] \n", reg_addr, data, mdio_addr);
#endif

    port_addr = (unsigned char) mdio_addr;

    data = data & 0xFFFF;
    sbuf[0] = port_addr >> 1;
    sbuf[1] = ((port_addr & 1) << 7) | ((dev_addr & 0x1F) << 2) | 2;
    sbuf[2] = (reg_addr >> 8);
    sbuf[3] = (reg_addr & 0xFF);
    sbuf[4] = sbuf[0] | 0x10;
    sbuf[5] = sbuf[1];
    sbuf[6] = (data >> 8);
    sbuf[7] = (data & 0xFF);
    ftStatus = FT_Write(write_ftHandle, sbuf, sizeof(sbuf), &BytesRW);

    return ftStatus;
#else
    /* Implement application specific 'write' mechanism */
    return BCM_PM_IF_UNAVAIL;
#endif
}

/* Function to connect to the board through the given USB Serial Number */
int device_sn_open(char *board_sn)
{
#if defined(USE_FTDI)
    DWORD NumDevs = 0;
    int i;
    int j;
    int ftStatus = FT_OK;
    int retval;
    unsigned int chip_id = 0;
    unsigned int chip_rev = 0;
    bool chip_found = false;
    FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;

    ftStatus = FT_CreateDeviceInfoList(&NumDevs);
    if (ftStatus != FT_OK) {
        printf("Failed to create the USB device info list!\n");
        return ftStatus;
    }

    printf("Number of devices: %d\n", NumDevs);
    if (NumDevs > 0) {
        devInfo = (FT_DEVICE_LIST_INFO_NODE *) malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * NumDevs);
        if (devInfo == NULL) {
            printf("Failed to allocate memory for the FT Device Info node!\n");
            return TEST_FAILURE;
        }

        ftStatus = FT_GetDeviceInfoList(devInfo, &NumDevs);
        if (ftStatus != FT_OK) {
            printf("Failed to get the USB device info list!\n");
            goto _device_open_error;
        }

        printf("Ready to try %d devices\n", NumDevs);
        for (i = 0; i < NumDevs; i++) {
            printf("Device %2d : Flags = %08x, Type = %08x, ID = %08x, LocId = %08x\n",
                    i, devInfo[i].Flags, devInfo[i].Type, devInfo[i].ID, devInfo[i].LocId);
            printf("SerialNumber = %s, Description = %s\n", devInfo[i].SerialNumber, devInfo[i].Description);

            if (devInfo[i].Type == 0x8) {
                if ((strcmp(devInfo[i].SerialNumber, board_sn)) == 0) {
                    ftStatus = FT_Open(i, &ftHandle);
                    if (ftStatus != FT_OK) {
                        printf("Failed to open the USB device!\n");
                        goto _device_open_error;
                    }
                    printf("Using USB device serial number %s\n", devInfo[i].SerialNumber);

                    for (j = 0; j < 32; j++) {
                        retval = mdio_read(&p_ctxt, j, APRT2_FTDI_CHIP_ID_REG, &chip_id);
                        if (retval == 0 && chip_id != 0xFFFF) {
                            printf("Chip ID (USB Dev: %d, MDIO: %d): 0x%x\n", i, j, chip_id);
                            mdio_read(&p_ctxt, j, APRT2_FTDI_CHIP_REV_REG, &chip_rev);
                            chip_rev &= CHIP_REV_ID_BIT_MASK;
                            printf("Chip Rev (USB Dev: %d, MDIO: %d): 0x%x\n", i, j, chip_rev);

                            /* Check for the chip ID of first chip */
                            if (chip_id == CHIP_ID) {
                                chip_found = true;
                                printf("Chip found at USB device %d and MDIO address %d\n", i, j);
                                continue;
                            }
                        }
                        if (chip_found) {
                            goto _device_open_error;
                        }
                    }
                    /* Close the device if chip is not found */
                    if (!chip_found) {
                        printf("Chip is not responding to USB!\n");
                        FT_Close(ftHandle);
                        ftStatus = TEST_FAILURE;
                        goto _device_open_error;
                    }
                }
            }
        }

        /* Return with error if chip is not found */
        if (!chip_found) {
            printf("USB serial %s (for chip) is not connected to this machine!\n", USB_DEV_SERIAL_CHIP);
            ftStatus = TEST_FAILURE;
            goto _device_open_error;
        }
    }

_device_open_error:
    if (devInfo != NULL) free(devInfo);
    return ftStatus;
#else
    /* Implement application specific 'device_sn_open' mechanism */
    return BCM_PM_IF_UNAVAIL;
#endif
}

/* connect to the board through the default USB Serial Number */
int device_open(void) {
    return  device_sn_open(USB_DEV_SERIAL_CHIP);
}

/* Function to close the USB handlers */
int device_close(void)
{
#if defined(USE_FTDI)
    if (ftHandle != NULL) {
        FT_Close(ftHandle);
    }
    return TEST_SUCCESS;
#else
    /* Implement application specific 'device_close' mechanism */
    return BCM_PM_IF_UNAVAIL;
#endif
}

#endif /* _APERTA2_COMMON_C_ */
