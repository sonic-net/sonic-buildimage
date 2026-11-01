/*
 *  COPYRIGHT (c) 2012 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/**
 * @file main.c
 *
 * Test code for the XO2 I2C Embedded Configuration Access routines.
 * This provides the various demos/tests that demonstrate using I2C access to
 * the XO2 on the Pico board.
 * <p>
 * This code is specific to porting the XO2 ECA to the Lattice Mico32 platform
 * running on the Lattice ECP3 Versa eval board.  It is not generic code.
 * It is specific exmaple implementation of the XO2 ECA.
 * <p>
 * The test examples show the following:
 * <ul>
 * <li> I2C bus access to Pico Board (read a temperature sensor chip)
 * <li> Read XO2 Configuration information via I2C
 * <li> Program a converted JEDEC file into the XO2 flash via I2C
 * <li> Program new EBR values into just the UFM, while design continues operating
 * <li> Individual UFM page accesses while design is running.
 * </ul>
 * <code>
 * <pre>
 * ----------------------           ---------------------
 * |  ECP3 Versa Board  |           |   XO2 Pico Board  |
 * |    -------------   |           |                   |
 * |    |  ECP3      |  |           |  ---------------- |
 * |    |            |  |           |  |   XO2        | |
 * |    |     Mico32 |  |           |  |       -> Regs| |
 * |    |       I2C  |<-|-----------|->| I2C----> Cfg | |
 * |    -------------   |           |  |       -> UFM | |
 * ----------------------           ---------------------
 * </pre>
 * </code>
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"

#include "systypes.h"
#include "xo2_cmds.h"
#include "eca_drvr.h"
#include "cpld_i2c_port.h"
#include "cpld_i2c.h"
#include "xo2_api.h"
#include "xo2_dev.h"

/* -----------------------------------------------------------------------------------------
 Uncomment to enable menu option to select Offline mode.  By default all operations
 are done in Transparent mode because Offline mode is not supported with a standard
 XO2 Pico board.  A hardware mod must be made to the Pico board to enable I2C power
 at all times, which includes Offline mode and when programming a blank part.
 See RD1129.pdf for details on the mod.
----------------------------------------------------------------------------------------- */
/* #define ENABLE_ALL_MODES */

#include "ufm_0123.h"
#include "debug.h"

#define FIRMWARE_UPGRADE_RETRY_TIMES            (10)

#define UFM_START_PAGE 256   /* Start here for user data (beginning is EBR init vals) */

/* GLOBAL VARIABLES */
unsigned char UFMPagePattern;

unsigned int UFMNextPage;

int ispvme_i2c_upgrade(OC_I2C_t *oc_i2c)
{
    int status;
    XO2RegInfo_t regInfo;
    unsigned int statusReg;
    int retry, ret;

    XO2Handle_t xo2_dev;
    ECADrvrCalls_t i2c_drvr;

    i2c_drvr.pfCmdTransfer = &cpld_i2c_transfer;
    i2c_drvr.pfCmdOpenfile = &cpld_i2c_fd_open;
    i2c_drvr.pfCmdClosefile = &cpld_i2c_fd_close;
    i2c_drvr.pfmSecDelay = &MicoSleepMilliSecs;
    i2c_drvr.pfuSecDelay = &MicoSleepMicroSecs;
    xo2_dev.devType = oc_i2c->cpld_type;   /* must match the real device */
    xo2_dev.cfgEn = 0;  /* initial value - config not enabled at start (could also be done with an API init call) */
    xo2_dev.pDrvrCalls = &i2c_drvr;   /* driver routines to start/stop read/write over I2C */
    xo2_dev.pDrvrParams = (void *)oc_i2c;   /* extra info to pass onto the driver routines */
    xo2_dev.fd = i2c_drvr.pfCmdOpenfile(oc_i2c->masterBus);

    if (xo2_dev.fd < 0) {
        return -1;
    }

    UFMPagePattern = 0;
    UFMNextPage = UFM_START_PAGE;   /* Start here for user data (beginning is EBR init vals) */

    CPLD_I2C_ISPVME("\r\n===================================================\r\n");
    CPLD_I2C_ISPVME("ECP3 I2C MachXO2 Embedded Configuration Access Demo\r\n");
    CPLD_I2C_ISPVME("               Version 1.1.0  Dec 2014\r\n");
    CPLD_I2C_ISPVME("===================================================\r\n");

    retry = 0;
    ret = 0;
    CPLD_I2C_ISPVME("\r\n--------------------------------------------------------\r\n");
    CPLD_I2C_ISPVME(    "Program XO2 with JEDEC Data Structure\r\n");
    if (oc_i2c->ProgramMode == XO2ECA_PROGRAM_TRANSPARENT) {
        CPLD_I2C_ISPVME("  TRANSPARENT");
    } else {
        CPLD_I2C_ISPVME("  OFFLINE");
    }
    if (oc_i2c->VerifyMode) {
        CPLD_I2C_ISPVME(" + Verify\r\n");
    } else {
        CPLD_I2C_ISPVME("\r\n");
    }

    CPLD_I2C_ISPVME("--------------------------------------------------------\r\n");

    status = XO2ECA_apiProgram(&xo2_dev,
    oc_i2c->pCfgBits,
    oc_i2c->VerifyMode | oc_i2c->ProgramMode |
    XO2ECA_ERASE_PROG_CFG | XO2ECA_ERASE_PROG_UFM | XO2ECA_ERASE_PROG_FEATROW);

    if (status != OK) {
        printf("ERROR! Failed to Program! err=%d\r\n", status);
        retry++;
        ret = 1;
    } else {
        printf("=> Programming Successful.\r\n");
    }
    UFMPagePattern = 0;
    UFMNextPage = UFM_START_PAGE;   // Start here for user data (beginning is EBR init vals)

    printf("\r\n------------------------------------------------------------\r\n");
    printf(    "XO2 Device Hdw info (USERCODE, TraceID, DevID, status, etc.)\r\n");
    printf(    "------------------------------------------------------------\r\n");
    memset(&regInfo, 0, sizeof(regInfo));
    status = XO2ECA_apiGetHdwInfo(&xo2_dev, &regInfo);
    if (status != OK) {
        printf("ERROR! Failed to Get HdwInfo! err=%d\r\n", status);
        ret = status;
        goto err_out;
    }
    printf("DeviceID: 0x%08x\r\n", regInfo.devID);
    printf("USERCODE: 0x%08x\r\n", regInfo.UserCode);
    printf(" TraceID: %x %x %x %x %x %x %x %x\r\n",
            regInfo.TraceID[0],
            regInfo.TraceID[1],
            regInfo.TraceID[2],
            regInfo.TraceID[3],
            regInfo.TraceID[4],
            regInfo.TraceID[5],
            regInfo.TraceID[6],
            regInfo.TraceID[7]);
    status = XO2ECA_apiGetHdwStatus(&xo2_dev, &statusReg);
    printf("Status: 0x%x\r\n", statusReg);
    if (statusReg & 0x01) {
        printf("\tDONE\r\n");
    }
    if (statusReg & 0x02) {
        printf("\tBUSY\r\n");
    }
    if (statusReg & 0x04) {
        printf("\tFAIL\r\n");
    }

    printf("\tFlash Check: ");
    switch (statusReg>>4) {
    case 0:
        printf("No Err\r\n");
        break;
    case 1:
        printf("ID Err\r\n");
        break;
    case 2:
        printf("CMD Err\r\n");
        break;
    case 3:
        printf("CRC Err\r\n");
        break;
    case 4:
        printf("Preamble Err\r\n");
        break;
    case 5:
        printf("Abort Err\r\n");
        break;
    case 6:
        printf("Overflow Err\r\n");
        break;
    case 7:
        printf("SDM EOF\r\n");
        break;
    }
err_out:
    CPLD_I2C_ISPVME("\r\nExiting.\r\n");
    MicoSleepMilliSecs(500);
    i2c_drvr.pfCmdClosefile(xo2_dev.fd);

    return ret;
}

int ispvme_main_i2c(OC_I2C_t *cpld_i2c)
{
    int ret;
    unsigned char *ufm_CfgData;
    unsigned char *ufm_UFMData;
    XO2FeatureRow_t ufm_FeatureRow;
    XO2_JEDEC_t XO2_JEDEC;

    ufm_CfgData = NULL;
    ufm_UFMData = NULL;

    ufm_CfgData = (unsigned char *) malloc(UFM_CFG_DATA_MAX_LEN);
    if (ufm_CfgData == NULL) {
        CPLD_I2C_ERROR("malloc failed.\n");
        return -1;
    }

    ufm_UFMData = (unsigned char *) malloc(UFM_UFM_DATA_MAX_LEN);
    if (ufm_UFMData == NULL) {
        free(ufm_CfgData);
        CPLD_I2C_ERROR("malloc failed.\n");
        return -2;
    }

    memset(ufm_CfgData, 0, UFM_CFG_DATA_MAX_LEN);
    memset(ufm_UFMData, 0, UFM_UFM_DATA_MAX_LEN);
    memset(&ufm_FeatureRow, 0, sizeof(XO2FeatureRow_t));

    XO2_JEDEC.pCfgData = ufm_CfgData;
    XO2_JEDEC.pUFMData = ufm_UFMData;
    XO2_JEDEC.pFeatureRow = &ufm_FeatureRow;

    ret = ufm_convert_jed(cpld_i2c->file_path, cpld_i2c->cpld_type, &XO2_JEDEC);
    if (ret < 0) {
        CPLD_I2C_ISPVME("Failed to convert jed file name: %s ret: %d\n", cpld_i2c->file_path, ret);
        goto failed;
    }

    cpld_i2c->pCfgBits = &XO2_JEDEC;
    cpld_i2c->designNum = CPLD_DESIGN_NUMBER;

    ret = ispvme_i2c_upgrade(cpld_i2c);
    if (ret < 0) {
        CPLD_I2C_ISPVME("Failed to upgrade file name: %s ret: %d\n", cpld_i2c->file_path, ret);
        goto failed;
    }
failed:
    free(ufm_CfgData);
    free(ufm_UFMData);

    return ret;
}
