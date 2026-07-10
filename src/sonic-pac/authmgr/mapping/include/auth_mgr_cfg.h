/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INCLUDE_AUTHMGR_CFG_H
#define INCLUDE_AUTHMGR_CFG_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "nim_data.h"

#define AUTHMGR_IS_READY (((authmgrCnfgrState == AUTHMGR_PHASE_INIT_3) || \
                        (authmgrCnfgrState == AUTHMGR_PHASE_EXECUTE) || \
                        (authmgrCnfgrState == AUTHMGR_PHASE_UNCONFIG_1)) ? ( TRUE) : ( FALSE))

typedef enum {
  AUTHMGR_PHASE_INIT_0 = 0,
  AUTHMGR_PHASE_INIT_1,
  AUTHMGR_PHASE_INIT_2,
  AUTHMGR_PHASE_WMU,
  AUTHMGR_PHASE_INIT_3,
  AUTHMGR_PHASE_EXECUTE,
  AUTHMGR_PHASE_UNCONFIG_1,
  AUTHMGR_PHASE_UNCONFIG_2,
} authmgrCnfgrState_t;

#define AUTHMGR_LLDP_PROFILES_MAX 128

typedef struct authmgrPortCfg_s
{
  nimConfigID_t configId;     /* NIM config ID for this interface*/
  /* if port is operating as supplicnat, 
     the functionality is redundant */
  /* Authentication methods */
   AUTHMGR_METHOD_t          methodList[ AUTHMGR_METHOD_LAST];
   AUTHMGR_METHOD_t          priorityList[ AUTHMGR_METHOD_LAST];
   AUTHMGR_PORT_CONTROL_t   portControlMode;     /* Current control mode setting by mgmt */
   AUTHMGR_HOST_CONTROL_t   hostMode;     /* Current host mode setting by mgmt */
   uint32 quietPeriod;       /* Initialization value for txWhen timer */
   uint32 reAuthPeriod;   /* Number of seconds between periodic reauthentication */
   BOOL   reAuthEnabled;  /* TRUE if reauthentication is enabled */
   uint32 maxUsers;        /*Maximum no. users in Mac-Based Authentication */
   uint32 maxAuthAttempts; /* Maximum number of times authentication may be reattempted by the user radius */
  /* variable to hold config related to session time out is to be used
     from radius server */
  BOOL reAuthPeriodServer;
  uint32 inActivityPeriod;   /* Number of seconds to wait after which clients can be cleaned up due to inactivity */
  uint32 intfConfigMask;
   uchar8 paeCapabilities;
} authmgrPortCfg_t;

typedef struct authmgrCfg_s
{
  fileHdr_t             cfgHdr;
  uint32                adminMode;
  uint32                authmgrLogTraceMode; /* Enable/disable log file tracing */
  uint32                vlanAssignmentMode;/* Global mode to enable vlan assignment */
  authmgrPortCfg_t           authmgrPortCfg[ AUTHMGR_INTF_MAX_COUNT]; /* Per-port config info */
  AUTHMGR_PORT_CONTROL_t   portControlMode;     /* Current control mode setting by mgmt */
  AUTHMGR_HOST_CONTROL_t   hostMode;     /* Current host mode setting by mgmt */
} authmgrCfg_t;

extern authmgrCfg_t *authmgrCfg;

typedef struct authmgrDebugCfgData_s 
{
   BOOL authmgrDebugPacketTraceTxFlag;
   BOOL authmgrDebugPacketTraceRxFlag;
} authmgrDebugCfgData_t;

typedef struct authmgrDebugCfg_s
{
  fileHdr_t          hdr;
  authmgrDebugCfgData_t   cfg;
  uint32             checkSum;
} authmgrDebugCfg_t;

extern RC_t authmgrSave(void);
extern  BOOL authmgrHasDataChanged(void);
extern void authmgrResetDataChanged(void);

extern RC_t authmgrCfgDump(void);
extern void authmgrBuildDefaultConfigData(void);
extern void authmgrBuildDefaultIntfConfigData(nimConfigID_t *configId, authmgrPortCfg_t *pCfg);

extern RC_t authmgrApplyConfigData(void);
extern RC_t authmgrApplyPortConfigData(uint32 intIfNum);
extern RC_t authmgrPortReset(uint32 intIfNum);

extern void authmgrApiCnfgrCommand( CNFGR_CMD_DATA_t *pCmdData);
extern RC_t authmgrInit(void);
extern void authmgrInitUndo();
extern RC_t authmgrCnfgrInitPhase1Process(void);
extern RC_t authmgrCnfgrInitPhase2Process(void);
extern RC_t authmgrCnfgrInitPhase3Process( BOOL warmRestart);
extern void authmgrCnfgrFiniPhase1Process();
extern void authmgrCnfgrFiniPhase2Process();
extern void authmgrCnfgrFiniPhase3Process();
extern RC_t authmgrCnfgrNoopProccess(  CNFGR_RESPONSE_t *pResponse,
                                        CNFGR_ERR_RC_t   *pReason );
extern RC_t authmgrCnfgrUconfigPhase2(  CNFGR_RESPONSE_t *pResponse,
                                         CNFGR_ERR_RC_t   *pReason );
extern void authmgrCnfgrParse( CNFGR_CMD_DATA_t *pCmdData);

extern RC_t authmgrLogicalPortInfoSetPortInfo(authmgrLogicalPortInfo_t *logicalPortInfo);
extern RC_t authmgrLogicalPortInfoInitialize(authmgrLogicalPortInfo_t *logicalPortInfo);
extern RC_t authmgrLogicalPortReset(authmgrLogicalPortInfo_t *logicalPortInfo);
extern void authmgrCnfgrTerminateProcess( CNFGR_CMD_DATA_t *pCmdData);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_CFG_H */
