/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

#ifndef __BARCHETTA_COMMON_C__
#define __BARCHETTA_COMMON_C__
/*
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 */

/* Includes */
#include <barchetta_common.h>

#if defined(USE_FTDI)
/* Global variables */
FT_HANDLE ftHandle[2] = {NULL, NULL};
bool test_use_single_usb_dev;
#endif

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

#if defined (SINGLE_CHIP_SETUP)
    read_ftHandle = ftHandle[0];
#elif defined (DUAL_CHIP_SETUP)
    if (test_use_single_usb_dev) {
        read_ftHandle = ftHandle[0];
    } else {
        if (mdio_addr == PHY_ID0) {
            read_ftHandle = ftHandle[0];
        } else if (mdio_addr == PHY_ID1) {
            read_ftHandle = ftHandle[1];
            mdio_addr = 16;
        }
    }
#endif
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
    /* Implement your application specific 'read' mechanism */
    return 0;
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

#if defined (SINGLE_CHIP_SETUP)
    write_ftHandle = ftHandle[0];
#elif defined (DUAL_CHIP_SETUP)
    if (test_use_single_usb_dev) {
        write_ftHandle = ftHandle[0];
    } else {
        if (mdio_addr == PHY_ID0) {
            write_ftHandle = ftHandle[0];
        } else if (mdio_addr == PHY_ID1) {
            write_ftHandle = ftHandle[1];
            mdio_addr = 16;
        }
    }
#endif

    port_addr = (unsigned char) mdio_addr;
    data = data & 0xFFFF;
#ifdef BCM_PLP_MDIO_TRACE_WRITE_ENABLE
    printf( "MDIO_TRACE: WRITE Register Address: [0x%x] Data:[0x%x] MDIO Address: [0x%x] \n", reg_addr, data, mdio_addr); 
#endif
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
    /* Implement your application specific 'write' mechanism */
    return 0;
#endif
}

/* Function to connect to the board through USB */
int device_open(void)
{
#if defined(USE_FTDI)
    DWORD NumDevs = 0;
    int i;
    int j;
    int ftStatus = FT_OK;
    int retval;
    unsigned int chip_id = 0;
    unsigned int chip_rev = 0;
    int p_ctxt = 5;
    bool chip0_found = false;
    FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;
#ifdef DUAL_CHIP_SETUP
    bool chip1_found = false;
    test_use_single_usb_dev = (strcmp(USB_DEV_SERIAL_CHIP0, USB_DEV_SERIAL_CHIP1) == 0) ? true : false;
#endif /* DUAL_CHIP_SETUP */

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
                if ((strcmp(devInfo[i].SerialNumber, USB_DEV_SERIAL_CHIP0)) == 0) {
                    ftStatus = FT_Open(i, &ftHandle[0]);
                    if (ftStatus != FT_OK) {
                        printf("Failed to open the USB device!\n");
                        goto _device_open_error;
                    }
                    printf("Using USB device serial number %s\n", devInfo[i].SerialNumber);

                    for (j = 0; j < 32; j++) {
                        retval = mdio_read(&p_ctxt, j, BARCH_FTDI_CHIP_ID_REG, &chip_id);
                        if (retval == 0 && chip_id != 0xFFFF) {
                            printf("Chip ID (USB Dev: %d, MDIO: %d): 0x%x\n", i, j, chip_id);
                            mdio_read(&p_ctxt, j, BARCH_FTDI_CHIP_REV_REG, &chip_rev);
                            chip_rev &= CHIP_REV_ID_BIT_MASK;
                            printf("Chip Rev (USB Dev: %d, MDIO: %d): 0x%x\n", i, j, chip_rev);

                            /* Check for the chip ID of first chip */
                            if (chip_id == CHIP_ID0) {
                                chip0_found = true;
                                printf("Chip0 found at USB device %d and MDIO address %d\n", i, j);
                                continue;
                            }

#ifdef DUAL_CHIP_SETUP
                            /* Check for the chip ID of second chip */
                            if (test_use_single_usb_dev) {
                                if (chip_id == CHIP_ID1) {
                                    chip1_found = true;
                                    printf("Chip1 found at USB device %d and MDIO address %d\n", i, j);
                                }
                            }
#endif /* DUAL_CHIP_SETUP */
                        }
#if defined (SINGLE_CHIP_SETUP)
                        if (chip0_found) {
                            goto _device_open_error;
                        }
#elif defined (DUAL_CHIP_SETUP)
                        if (test_use_single_usb_dev) {
                            if (chip0_found && chip1_found) {
                                goto _device_open_error;
                            }
                        }
#endif
                    }
                    /* Close the device if chip is not found */
                    if (!chip0_found) {
                        printf("Chip0 is not responding to USB!\n");
                        FT_Close(ftHandle[0]);
                        ftStatus = TEST_FAILURE;
                        goto _device_open_error;
                    }
#ifdef DUAL_CHIP_SETUP
                    if (test_use_single_usb_dev) {
                        if (!chip1_found) {
                            printf("Chip1 is not responding to USB!\n");
                            FT_Close(ftHandle[0]);
                            ftStatus = TEST_FAILURE;
                            goto _device_open_error;
                        }
                    }
#endif /* DUAL_CHIP_SETUP */
                }

#ifdef DUAL_CHIP_SETUP
                if (test_use_single_usb_dev) {
                    /* Both USB serial numbers are same */
                    continue;
                }

                if ((strcmp(devInfo[i].SerialNumber, USB_DEV_SERIAL_CHIP1)) == 0) {
                    ftStatus = FT_Open(i, &ftHandle[1]);
                    if (ftStatus != FT_OK) {
                        printf("Failed to open the USB device!\n");
                        goto _device_open_error;
                    }

                    printf("Using USB device serial number %s\n", devInfo[i].SerialNumber);
                    ftStatus = FT_SetLatencyTimer(ftHandle[1], 2);
                    ftStatus = FT_SetTimeouts(ftHandle[1], 500, 500);
                    ftStatus = FT_SetLatencyTimer(ftHandle[1], 1);

                    for (j = 0; j < 32; j++) {
                        retval = mdio_read(&p_ctxt, j, BARCH_FTDI_CHIP_ID_REG, &chip_id);
                        if (retval == 0 && chip_id != 0xFFFF) {
                            printf("Chip ID (USB Dev: %d, MDIO: %d): 0x%x\n", i, j, chip_id);
                            mdio_read(&p_ctxt, j, BARCH_FTDI_CHIP_REV_REG, &chip_rev);
                            chip_rev &= CHIP_REV_ID_BIT_MASK;
                            printf("Chip Rev (USB Dev: %d, MDIO: %d): 0x%x\n", i, j, chip_rev);

                            /* Check for the chip ID and revision of second chip */
                            if (chip_id == CHIP_ID1) {
                                chip1_found = true;
                                printf("Chip1 found at USB device %d and MDIO address %d\n", i, j);
                                break;
                            }
                        }
                    }
                    /* Close the device if chip not found */
                    if (!chip1_found) {
                        printf("Chip1 is not responding to USB!\n");
                        FT_Close(ftHandle[1]);
                        ftStatus = TEST_FAILURE;
                        goto _device_open_error;
                    }
                }
#endif /* DUAL_CHIP_SETUP */
            }
        }

        /* Return with error if chip is not found */
        if (!chip0_found) {
            printf("USB serial %s (for chip0) is not connected to this machine!\n", USB_DEV_SERIAL_CHIP0);
            ftStatus = TEST_FAILURE;
            goto _device_open_error;
        }
#ifdef DUAL_CHIP_SETUP
        if (!chip1_found) {
            printf("USB serial %s (for chip1) is not connected to this machine!\n", USB_DEV_SERIAL_CHIP1);
            ftStatus = TEST_FAILURE;
            goto _device_open_error;
        }
#endif /* DUAL_CHIP_SETUP */
    }

_device_open_error:
    if (devInfo != NULL) free(devInfo);
    return ftStatus;
#else
    /* Implement your application specific 'device_open' mechanism */
    return 0;
#endif
}

/* Function to close the USB handlers */
int device_close(void)
{
#if defined(USE_FTDI)
    if (ftHandle[0] != NULL) FT_Close(ftHandle[0]);
#ifdef DUAL_CHIP_SETUP
    if (ftHandle[1] != NULL) FT_Close(ftHandle[1]);
#endif /* DUAL_CHIP_SETUP */

    return TEST_SUCCESS;
#else
    /* Implement your application specific 'device_close' mechanism */
    return 0;
#endif
}

int synce_test(char *chip_name, bcm_plp_access_t phy_info, unsigned int clkGenSquelchCfg, unsigned int squelchMonitorLanemap,
    unsigned int recoveredClkLane, unsigned int divider, unsigned int rclk_side)
{
    int rv = 0;
    bcm_plp_synce_cfg_t synce_cfg;

    memset(&synce_cfg, 0, sizeof(bcm_plp_synce_cfg_t));

    synce_cfg.clkGenSquelchCfg = clkGenSquelchCfg;
    synce_cfg.squelchMonitorLanemap = squelchMonitorLanemap;
    synce_cfg.recoveredClkLane = recoveredClkLane;
    synce_cfg.divider = divider;
    synce_cfg.rclk_if_side = rclk_side;

    /* SyncE Set*/
    rv = bcm_plp_synce_config_set(chip_name, phy_info, &synce_cfg);
    if(rv != 0) {
        printf("FAIL: bcm_plp_synce_config_set API failed for PHY_ID[%d], return code[%d]\n",
            phy_info.phy_addr, rv);
        return rv;
    } else {
        printf("PASS: bcm_plp_synce_config_set API for PHY_ID[%d], return code[%d]\n",
            phy_info.phy_addr, rv);
    }
    memset(&synce_cfg, 0, sizeof(bcm_plp_synce_cfg_t));

    rv = bcm_plp_synce_config_get(chip_name, phy_info, &synce_cfg);
    if(rv != 0) {
        printf("FAIL: bcm_plp_synce_config_get API failed for PHY_ID[%d], return code[%d]\n",
            phy_info.phy_addr, rv);
        return rv;
    } else {
        printf("PASS: bcm_plp_synce_config_get API for PHY_ID[%d], return code[%d]\n",
            phy_info.phy_addr, rv);
    }
    if ((clkGenSquelchCfg == bcmplpClkGenSquelchDisable) && (synce_cfg.clkGenSquelchCfg == bcmplpClkGenSquelchDisable)) {
        printf("PASS: bcm_plp_synce_config_get API returned correct SyncE configuration for PHY_ID[%d], return code[%d]\n",
            phy_info.phy_addr, rv);
    } else {
        if ((synce_cfg.clkGenSquelchCfg == clkGenSquelchCfg) &&
            (synce_cfg.rclk_if_side == rclk_side) &&
            (synce_cfg.recoveredClkLane == recoveredClkLane) &&
            (synce_cfg.divider == divider)) {
            printf("PASS: bcm_plp_synce_config_get API returned correct SyncE configuration for PHY_ID[%d], return code[%d]\n",
                phy_info.phy_addr, rv);
        }
        else {
            printf("FAIL: Expected values: ClkGenSquelchCfgEn:0x%x, recoveredClkLane:0x%x, Divider:0x%x, rclk_side:%d\n",
                clkGenSquelchCfg, recoveredClkLane, divider, rclk_side);
            printf("FAIL: Returned values: ClkGenSquelchCfgEn:0x%x, recoveredClkLane:0x%x, Divider:0x%x, rclk_side:0x%x\n",
                synce_cfg.clkGenSquelchCfg, synce_cfg.recoveredClkLane, synce_cfg.divider, synce_cfg.rclk_if_side);
            rv = -1;
        }
    }
    return rv;
}

#endif /* __BARCHETTA_COMMON_C__ */
