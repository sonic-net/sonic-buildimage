/*
 *  COPYRIGHT (c) 2012 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/** @file XO2_cmds.h */

#ifndef LATTICE_XO2_CMDS_H
#define LATTICE_XO2_CMDS_H

#include "systypes.h"
#include "xo2_dev.h"

//--------------------------------------------
//      E R R O R   C O D E S
//--------------------------------------------
#define ERR_XO2_NOT_IN_CFG_MODE (-100)
#define ERR_XO2_NO_UFM          (-101)
#define ERR_XO2_EXCEEDS_UFM_SIZE (-102)
#define ERR_XO2_EXCEEDS_CFG_SIZE (-103)

#define XO2ECA_CMD_LOOP_TIMEOUT    10000 // number of times to poll in a loop before aborting
#define XO2ECA_CMD_ERASE_UFM   8
#define XO2ECA_CMD_ERASE_CFG   4
#define XO2ECA_CMD_ERASE_FTROW 2
#define XO2ECA_CMD_ERASE_SRAM  1

//--------------------------------------------
//      G e n e r a l   C o m m a n d s
//--------------------------------------------
int XO2ECAcmd_readDevID(XO2Handle_t *pXO2, unsigned int *pVal) ;
int XO2ECAcmd_readUserCode(XO2Handle_t *pXO2, unsigned int *pVal) ;
int XO2ECAcmd_setUserCode(XO2Handle_t *pXO2, unsigned int val) ;
int XO2ECAcmd_readTraceID(XO2Handle_t *pXO2, unsigned char *pVal) ;

int XO2ECAcmd_Bypass(XO2Handle_t *pXO2) ;

int XO2ECAcmd_closeCfgIF(XO2Handle_t *pXO2);
int XO2ECAcmd_openCfgIF(XO2Handle_t *pXO2, XO2CfgMode_t mode);

int XO2ECAcmd_readStatusReg(XO2Handle_t *pXO2, unsigned int *pVal) ;
int XO2ECAcmd_readBusyFlag(XO2Handle_t *pXO2, unsigned char *pVal) ;
int XO2ECAcmd_waitStatusBusy(XO2Handle_t *pXO2) ;
int XO2ECAcmd_waitBusyFlag(XO2Handle_t *pXO2) ;

int XO2ECAcmd_SetPage(XO2Handle_t *pXO2, XO2SectorMode_t mode, unsigned int pageNum) ;

int XO2ECAcmd_EraseFlash(XO2Handle_t *pXO2, unsigned char mode) ;
int XO2ECAcmd_SRAMErase(XO2Handle_t *pXO2) ;

//--------------------------------------------
//      C F G   C o m m a n d s
//--------------------------------------------

int XO2ECAcmd_setDone(XO2Handle_t *pXO2) ;
int XO2ECAcmd_Refresh(XO2Handle_t *pXO2);

int XO2ECAcmd_CfgErase(XO2Handle_t *pXO2) ;
int XO2ECAcmd_CfgResetAddr(XO2Handle_t *pXO2) ;
int XO2ECAcmd_CfgReadPage(XO2Handle_t *pXO2, unsigned char *pBuf) ;
int XO2ECAcmd_CfgWritePage(XO2Handle_t *pXO2, unsigned char *pBuf) ;

//--------------------------------------------
//      U F M   C o m m a n d s
//--------------------------------------------
int XO2ECAcmd_UFMErase(XO2Handle_t *pXO2) ;
int XO2ECAcmd_UFMResetAddr(XO2Handle_t *pXO2);
int XO2ECAcmd_UFMWritePage(XO2Handle_t *pXO2, unsigned char *pBuf) ;
int XO2ECAcmd_UFMReadPage(XO2Handle_t *pXO2, unsigned char *pBuf) ;

//--------------------------------------------
//     F E A T U R E  RO W   C o m m a n d s
//--------------------------------------------
int XO2ECAcmd_FeatureRowErase(XO2Handle_t *pXO2);
int XO2ECAcmd_FeatureRowWrite(XO2Handle_t *pXO2, XO2FeatureRow_t *pFeature) ;
int XO2ECAcmd_FeatureRowRead(XO2Handle_t *pXO2, XO2FeatureRow_t *pFeature) ;

#endif
