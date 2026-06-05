/*
 * $Copyright: (c) 2024 Broadcom.
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

typedef struct timespec hr_time_t;

uint16_t SW_DVDD[] =  {0x410, 0x411};

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

/*******************************************************************************
 PURPOSE: Function to read data from I2C interface
 COMMENT:
*******************************************************************************/
int i2c_read(void *user_acc, unsigned int addr, unsigned int *data)
{
#if defined(USE_FTDI)
    const unsigned char RD_FLAGS = 0xA0, L=1;
    FT_STATUS ftStatus;
    DWORD BytesRW = 0;
    unsigned char r = 0xCC, buf = RD_FLAGS | (0x1F & addr);
    FT_HANDLE read_ftHandle = ftHandle;
    if (!user_acc) {
        return TEST_FAILURE;
    }

    ftStatus = FT_Write(read_ftHandle, &buf, L, &BytesRW);

    if (ftStatus == FT_OK) {
        ftStatus = FT_Read(read_ftHandle, &r, L, &BytesRW);
        if (ftStatus == FT_OK) {
            *data = r ;
        }
    }
    return ftStatus;
#else
    /* Implement application specific 'read' mechanism */
    return BCM_PM_IF_UNAVAIL;
#endif
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

/*******************************************************************************
 PURPOSE: Function to read data from MDIO/I2C interface
 COMMENT:
*******************************************************************************/
int mdio_i2c_read(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int *data)
{
    int rv = 0;
    uint16_t dev_addr = 0;
    dev_addr = (reg_addr >> 16) & 0x1F;
    if(dev_addr == 30) {
        rv = i2c_read(user_acc, reg_addr, data);
    } else {
        rv = mdio_read(user_acc, mdio_addr, reg_addr, data);
    }
    return rv ;
}

/*******************************************************************************
 PURPOSE: Function to write data into I2C interface
 COMMENT:
*******************************************************************************/
int i2c_write(void *user_acc, unsigned int addr, unsigned int data)
{
#if defined(USE_FTDI)
    const unsigned char WR_FLAGS = 0xE0, L=2;
    FT_STATUS ftStatus;
    DWORD BytesRW = 0;
    unsigned char qbuf[2] = {WR_FLAGS | (0x1F & addr), (data & 0xFF)};
    FT_HANDLE write_ftHandle = ftHandle;

    if (!user_acc) {
        return TEST_FAILURE;
    }

    ftStatus = FT_Write(write_ftHandle, qbuf, L, &BytesRW);

    return ftStatus;
#else
    /* Implement application specific 'write' mechanism */
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

/*******************************************************************************
 PURPOSE: Function to write data into MDIO/I2C interface
 COMMENT:
*******************************************************************************/
int mdio_i2c_write(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int data)
{
    int rv = 0;
    uint16_t dev_addr = 0;
    dev_addr = (reg_addr >> 16) & 0x1F;
    if(dev_addr == 30) {
        rv = i2c_write(user_acc, reg_addr, data);
    } else {
        rv = mdio_write(user_acc, mdio_addr, reg_addr, data);
    }
    return rv ;
}

/*******************************************************************************
 PURPOSE: Function to set I2C clock
 COMMENT:
*******************************************************************************/
int setI2CClk(void *user_acc, unsigned int I2C_clk)
{
	int rv = 0 ;
    if (I2C_clk) {
        rv = i2c_write(user_acc, 0xF, I2C_clk);
    }
    return rv ;
}

/*******************************************************************************
 PURPOSE: Function to get FPGA ID and FPGA version
 COMMENT:
*******************************************************************************/
int getFtInfo(void *user_acc, unsigned int *fpgaId, unsigned int* fpgaVer)
{
    int k;
    int rv = 0;
    unsigned int addr[5] = {0x1B, 0x1C, 0x1D, 0x1E, 0x1F}, value[5] = {0};

    for (k=0; k<5; ++k) {
        rv = i2c_read(user_acc, addr[k], &value[k]);
        if (rv != FT_OK) {
            return TEST_FAILURE;
        }
    }

    *fpgaId    = (value[2] & 0xFF);
    *fpgaId  <<= 8; *fpgaId  |= (value[0] & 0xFF);
    *fpgaId  <<= 8; *fpgaId  |= (value[1] & 0xFF);

    *fpgaVer   = (value[3] & 0xFF);
    *fpgaVer <<= 8; *fpgaVer |= (value[4] & 0xFF);

     return rv ;
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
    unsigned int fpga_id, fpga_ver;

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
                            setI2CClk(&p_ctxt, SMBUS_SPEED_400K);
                            getFtInfo(&p_ctxt, &fpga_id, &fpga_ver);
                            printf("PLP FPGA Chip ID: %x, FPGA Code Version: %x\n", fpga_id, fpga_ver);
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

hr_time_t hr_time(hr_time_t* p)
{
    hr_time_t tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    if (p) {
        *p = tm;
    }
    return tm;
}

double hr_difftime_ms(hr_time_t *end, hr_time_t *begin)
{
    return (end->tv_sec - begin->tv_sec) * 1000 + (end->tv_nsec - begin->tv_nsec) / 1000000;
}

double hr_difftime(hr_time_t *end, hr_time_t *begin)
{
    return (hr_difftime_ms(end, begin) / 1000);
}


uint16_t run_STA(char *chip_name, actor_t *sta, double tmd)
{
    ++(sta->cnt);
    LOGX(LOG_INFO, "=>>{ [%s:%d] %3d @ %6.3f", sta->tag, sta->state, sta->cnt, tmd);

    if (sta->state == 0) {
        /* STA state 0: wait for regulator ready */
        LOGX(LOG_INFO, "   %s: check for regulator readiness", sta->tag);
        if ((get_RGC()->state) >= 1) {
            LOGX(LOG_INFO, "   %s: GRC reports regulator ready", sta->tag);
            sta->state = 1;
        } else {
            LOGX(LOG_INFO, "   %s: GRC did not reports regulator ready", sta->tag);
        }
    } else if (sta->state == 1) {
        /* STA state 1: enable AVS per user config */
        LOGX(LOG_INFO, "   %s: to config & enabel AVS in the FW", sta->tag);
        config_dut_as_avs_requestor(chip_name, sta->phy_info, (uint8_t)(sta->info[0]));
        sta->state = 2;
    } else if (sta->state == 2) {
        /*STA state 2: waiting for AVS init done*/
        uint16_t sts = show_AVS_status_from_STA(chip_name, sta->phy_info);

        if (0x111 == (sts & 0xFFF)) {
            LOGX(LOG_INFO, "   %s: AVS init done successfully. Move to tracking state.", sta->tag);
            sta->state = 3;
        } else if (0x20 == (sts & 0xFFF)) {
            LOGX(LOG_INFO, "   %s: AVS error occurred. ", sta->tag);
            sta->state = 4;
        }
    } else if (sta->state == 3) {
        /*STA state 3: in the tracking mode*/
        uint16_t sts = show_AVS_status_from_STA(chip_name, sta->phy_info);

        if (0x20 == (sts & 0xFFF)) {
            LOGX(LOG_INFO, "   %s: AVS error occurred. ", sta->tag);
            sta->state = 4;
#ifdef ENABLE_TEST_CODE
        } else {
            /*  AVS is working normally, do nothing unless we need to force voltage changes as 
                shown in the two special test case below. */
            if (sta->cnt == 20) {
                /* Special test case to force the voltage to go up by 4 ticks. 
                   The vmargin parameter is an absolute positive-only offset. */
                uint32_t vmargin = 4, vout_ctrl;
                int vout_diff;
                bcm_plp_reg_value_get(chip_name, sta->phy_info, 1, AVS_Vout_control_Adr, &vout_ctrl);
                vout_diff = (int)(vmargin) - (int)(vout_ctrl);
                bcm_plp_reg_value_set(chip_name, sta->phy_info, 1, AVS_Vout_control_Adr, vmargin);
                LOGX(LOG_INFO, "   %s: applies voltage disturbance of %+d steps (%+d mV) via board margin control",
                    sta->tag, vout_diff, vout_diff*5);
            }
            if (sta->cnt == 40) {
                /* Special test case to force the voltage to go down to up by 2 ticks after going up for 4 ticks. */
                uint32_t vmargin = 2, vout_ctrl;
                int vout_diff;
                bcm_plp_reg_value_get(chip_name, sta->phy_info, 1, AVS_Vout_control_Adr, &vout_ctrl);
                vout_diff = (int)(vmargin) - (int)(vout_ctrl);
                bcm_plp_reg_value_set(chip_name, sta->phy_info, 1, AVS_Vout_control_Adr, vmargin);
                LOGX(LOG_INFO, "   %s: applies voltage disturbance of %+d steps (%+d mV) via board margin control",
                    sta->tag, vout_diff, vout_diff*5);
            }
#endif
        }
    } else if (sta->state == 4) {
        /*STA state 4: error condition*/
        show_AVS_status_from_STA(chip_name, sta->phy_info);
        LOGX(LOG_INFO, "   %s: AVS error occurred. ", sta->tag);
    }
    LOGX(LOG_INFO, "}<<= [%s:%d] %3d @ %6.3f", sta->tag, sta->state, sta->cnt, tmd);
    return sta->state;
}

uint16_t run_RGC(char *chip_name, actor_t *rgc, double tmd)
{
    ++(rgc->cnt);
    LOGX(LOG_INFO, "=>>{ [%s:%d] %3d @ %6.3f", rgc->tag, rgc->state, rgc->cnt, tmd);

    if (rgc->state == 0) {
        /*GRC state 0: configure regulators*/
        uint8_t regu_rail_addr = 0x40;
        config_regulators(chip_name, rgc->phy_info, regu_rail_addr);
        set_DVDD_voltage(chip_name, rgc->phy_info, regu_rail_addr, 0, 750);

        /* regulator connection is good */
        LOGX(LOG_INFO, "   %s: regulator ready", rgc->tag);

        rgc->state = 1; /* to do handshake with AVS FW */
    } else if (rgc->state == 1) {
        const uint16_t HS0 = 0xFEED, HS1 = 0xCAFE;
        uint16_t rv;
        rv = get_avs_vrequest(chip_name, rgc->phy_info, (uint8_t)(rgc->info[0]));
        LOGX(LOG_INFO, "   %s: do handshake with AVS FW. reading back %04X", rgc->tag, rv);
        if (HS0 == rv) {
            put_avs_vresponse(chip_name, rgc->phy_info, (uint8_t)(rgc->info[0]), HS1);
            LOGX(LOG_INFO, "   %s: sending back %04X in response to %04X", rgc->tag, HS1, rv);
            rgc->state = 2; /* tracking */
        }
    } else if (rgc->state == 2) {
        const uint16_t lim_lo = 550, lim_hi = 760;
        uint16_t vout_req, dvdd;
        vout_req = get_avs_vrequest(chip_name, rgc->phy_info, (uint8_t)(rgc->info[0]));
        if ((0xC000 != (vout_req & 0xC000)) && (0x2000 == (vout_req & 0x2000))) {
            dvdd = vout_req & 0x3FF;
            LOGX(LOG_INFO, "   %s: AVS FW requests a new voltage %04X (%3d mV)", rgc->tag, vout_req, dvdd);
            dvdd = get_dvdd_with_limit(dvdd, lim_lo, lim_hi);
            set_DVDD_voltage(chip_name, rgc->phy_info, (uint8_t)(rgc->info[1]), 0, dvdd);
#ifdef CONFIRM_AFTER_WAIT
            usleep(100*1000);
            put_avs_vresponse(chip_name, rgc->phy_info, (uint8_t)(rgc->info[0]), dvdd);
            LOGX(LOG_INFO, "   %s: confirms to AVS FW %3d mV vout is available", rgc->tag, dvdd);
        }
#else
            LOGX(LOG_INFO, "   %s programs regulator output voltage to %3d mV", rgc->tag, dvdd);
            /* do not respond to AVS FW right away to allow regulator output to settle */
            set_actor_param(rgc, 2, dvdd);
            rgc->state = 3; /* to post response at next chance */
        }
    } else if (rgc->state == 3) {
        put_avs_vresponse(chip_name, rgc->phy_info, (uint8_t)(rgc->info[0]), rgc->info[2]);
        LOGX(LOG_INFO, "   %s: confirms to AVS FW %3d mV vout is available", rgc->tag, rgc->info[2]);
        rgc->state = 2; /* return to the state in polling vout request */
#endif
    }

    LOGX(LOG_INFO, "}<<= [%s:%d] %3d @ %6.3f", rgc->tag, rgc->state, rgc->cnt, tmd);
    return rgc->state;
}

uint16_t get_avs_vrequest(char *chip_name, bcm_plp_access_t plp_info, uint8_t avs_i2c_responder_addr)
{
    uint32_t rqst = 0;
    ial_config_t ialConfig ;

    memset(&ialConfig, 0, sizeof(ial_config_t));

    if (avs_i2c_responder_addr) {
        /* read from SMBus */
        configSmbusParam(chip_name, plp_info, &ialConfig, avs_i2c_responder_addr, /*PEC_enable=*/0, /*smbus_addr_mode=*/0);
        rqst = smbusReadWord(chip_name, plp_info, &ialConfig, 0x4);
        if (0xC000 == (rqst & 0xC000)) {
            LOGX(LOG_INFO, "   I2C[%02x.%d] ==> %04X (special msg)", avs_i2c_responder_addr, 4, rqst);
        } else {
            LOGX(LOG_INFO, "   I2C[%02x.%d] ==> %04X (%3d mV)", avs_i2c_responder_addr, 4, rqst, (rqst&0x3FF));
        }
    } else {
        /* read from MDIO */
        bcm_plp_reg_value_get(chip_name, plp_info, 1, AVS_Vout_mV_status_Adr, &rqst);
        if (0xC000 == (rqst & 0xC000)) {
            LOGX(LOG_INFO, "  MDIO[%04X] ==> %04X (special msg)", AVS_Vout_mV_status_Adr, rqst);
        } else {
            LOGX(LOG_INFO, "  MDIO[%04X] ==> %04X (%3d mV)", AVS_Vout_mV_status_Adr, rqst, (rqst&0x3FF));
        }
    }

    return (rqst & 0xFFFF);
}

void put_avs_vresponse(char *chip_name, bcm_plp_access_t plp_info, uint8_t avs_i2c_responder_addr, uint16_t resp)
{
    ial_config_t ialConfig ;

    memset(&ialConfig, 0, sizeof(ial_config_t));

    if (avs_i2c_responder_addr) {
        /* write to SMBus */
        configSmbusParam(chip_name, plp_info, &ialConfig, avs_i2c_responder_addr, /*PEC_enable=*/0, /*smbus_addr_mode=*/0);
        smbusWriteWord(chip_name, plp_info, &ialConfig, 0x8, resp);
        LOGX(LOG_INFO, "   I2C[%02x.%d] <== %04X (%3d mV)", avs_i2c_responder_addr, 8, resp, resp);
    } else {
        /* write to MDIO */
        bcm_plp_reg_value_set(chip_name, plp_info, 1, AVS_Vout_response_Adr, resp);
        LOGX(LOG_INFO, "  MDIO[%04X] <== %04X (%3d mV)", AVS_Vout_response_Adr, resp, resp);
    }
}

uint16_t get_dvdd_with_limit(uint16_t vout_req, uint16_t lim_lo, uint16_t lim_hi)
{
    if (vout_req < lim_lo) return lim_lo;
    if (vout_req > lim_hi) return lim_hi;
    return vout_req;
}

uint16_t get_regulator_out(char *chip_name, bcm_plp_access_t plp_info, uint16_t idx)
{
    return get_DVDD_voltage(chip_name, plp_info, (SW_DVDD[idx] >> 4), (SW_DVDD[idx] & 0xF));
}

void config_regulators(char *chip_name, bcm_plp_access_t plp_info, uint8_t rail_addr)
{
    uint16_t idx;
    set_rail_address(chip_name, plp_info, rail_addr, SW_DVDD, sizeof(SW_DVDD)/sizeof(SW_DVDD[0]));

    LOGX(LOG_INFO, "   Initial VOUT readings:");
    for (idx=0; idx<sizeof(SW_DVDD)/sizeof(SW_DVDD[0]); ++idx) {
        get_regulator_out(chip_name, plp_info, idx);
    }
}

void init_actor(actor_t* r, bcm_plp_access_t plp_info, char* tag)
{
    strncpy(r->tag, tag, 7);
    r->tag[7] = '\0';
    memcpy(&r->phy_info, &plp_info, sizeof(bcm_plp_access_t));
    r->cnt = r->state = 0;
    r->info[0] = r->info[1] = r->info[2] = r->info[3] = 0;
}

int set_actor_param(actor_t* r, uint16_t idx, uint16_t val)
{
    if (idx < 4) {
        r->info[idx] = val;
        return 0;
    }
    return 1;
}

actor_t * get_STA(void)
{
    return &aSTA;
}

actor_t * get_RGC(void)
{
    return &aRGC;
}

void run_scheduler(char *chip_name, uint32_t exit_cnt, uint16_t t1, uint16_t t2)
{
    static hr_time_t tm0;
    static int flag, sta_run = 0, rgc_run = 0;
    hr_time_t tmm;
    double tmd, df;
    uint32_t tmd_i, cnt = 0;

    if (flag == 0) {
        flag = 1;
        hr_time(&tm0);
    }

    do {
        hr_time(&tmm);
        tmd = hr_difftime(&tmm, &tm0);
        tmd_i = (uint32_t) (tmd);
        df = (tmd - tmd_i) * 1000;

        if (fabs(df - t1) < 3) {
            if (sta_run == 0) {
                ++cnt; sta_run = 1;
                run_STA(chip_name, get_STA(), tmd);
            }
        } else {
            sta_run = 0;
        }

        if (fabs(df - t2) < 3) {
            if (rgc_run == 0) {
                rgc_run = 1;
                run_RGC(chip_name, get_RGC(), tmd);
            }
        } else {
            rgc_run = 0;
        }

    } while (cnt < exit_cnt);
}

uint16_t show_AVS_status_from_STA(char *chip_name, bcm_plp_access_t plp_info)
{
    uint32_t cfg1, cntrl, sts, req;
    bcm_plp_reg_value_get(chip_name, plp_info, 1, AVS_I2C_address0_Adr, &cfg1);
    bcm_plp_reg_value_get(chip_name, plp_info, 1, AVS_FW_control_Adr, &cntrl);
    bcm_plp_reg_value_get(chip_name, plp_info, 1, AVS_FW_status_Adr, &sts);
    bcm_plp_reg_value_get(chip_name, plp_info, 1, AVS_Vout_mV_status_Adr, &req);

    LOGX(LOG_INFO, "   AVS_CFG1: %04X | AVS_CNTRL: %04X || AVS_STS: %04X | AVS_VOUT_REQ: %04X",
            cfg1, cntrl, sts, req);
    return (sts & 0xFFFF);
}

void config_dut_as_avs_requestor(char *chip_name, bcm_plp_access_t plp_info, uint8_t addr)
{
    uint16_t avs_cfg1 = (addr) ? ((0x80 | addr) << 8) : 0;
    bcm_plp_reg_value_set(chip_name, plp_info, 1, AVS_I2C_address0_Adr, avs_cfg1);

    /* enable AVS FW */
    bcm_plp_reg_value_set(chip_name, plp_info, 1, AVS_FW_control_Adr, 0x5);

    show_AVS_status_from_STA(chip_name, plp_info);
}

void test_close_loop_ext_regu_controller(char *chip_name, bcm_plp_access_t plp_info, uint8_t regu_rail_addr, uint8_t avs_i2c_responder_addr)
{
    init_actor(&aSTA, plp_info, "STA");
    set_actor_param(&aSTA, 0, avs_i2c_responder_addr);
    init_actor(&aRGC, plp_info, "RGC");
    set_actor_param(&aRGC, 0, avs_i2c_responder_addr);
    set_actor_param(&aRGC, 1, regu_rail_addr);
    run_scheduler(chip_name, 60, 250, 400);
}

#endif /* _APERTA2_COMMON_C_ */
