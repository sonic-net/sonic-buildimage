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

#ifndef INCLUDE_AUTHMGR_DEBUG_H
#define INCLUDE_AUTHMGR_DEBUG_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#define AUTHMGR_TRACE(format,args...)                         \
{                                                           \
  if ( authmgrDebugLogTraceModeGet() ==  ENABLE)           \
  {                                                        \
     LOGF( LOG_SEVERITY_INFO,format,##args); \
  }                                                        \
}

#define AUTHMGR_ERROR_SEVERE(format,args...)                  \
{                                                           \
     LOGF( LOG_SEVERITY_ERROR,format,##args); \
}

/* logical port debug info */
typedef struct authmgrLogicalPortDebugInfo_s
{
 /* unique node identifier*/ 
  authmgrLogicalNodeKey_t key;
   char8 timers[200];

  /* protocol related info */
  authmgrProtocolInfo_t protocol;

  /* client specific non protocol data */
  authmgrClientInfo_t client;
 
} authmgrLogicalPortDebugInfo_t;

extern void authmgrDebugMsgQueue();
extern void authmgrDebugTraceIdGet();
extern void authmgrDebugSizesShow();
extern void authmgrDebugPortCfgShow(uint32 intIfNum);
extern void authmgrDebugPortInfoShow(uint32 intIfNum);
extern void authmgrDebugPortStatsShow(uint32 intIfNum);
extern RC_t authmgrDebugLogTraceModeSet(uint32 mode);
extern uint32 authmgrDebugLogTraceModeGet();
extern  BOOL authmgrDebugPacketTraceTxFlag;
extern  BOOL authmgrDebugPacketTraceRxFlag;

extern void authmgrBuildTestConfigData(void);
extern RC_t authmgrDebugSave(void);
extern void authmgrDebugBuildDefaultConfigData(uint32 ver);
extern  BOOL authmgrDebugHasDataChanged(void);
extern void authmgrDebugPacketTraceFlagGet( BOOL *transmitFlag, BOOL *receiveFlag);
extern RC_t authmgrDebugPacketTraceFlagSet( BOOL transmitFlag, BOOL receiveFlag);
extern void authmgrDebugPacketRxTrace(uint32 intIfNum,  netBufHandle bufHandle);
extern void authmgrDebugPacketTxTrace(uint32 intIfNum,  netBufHandle bufHandle);
extern void authmgrDebugPacketTrace(uint32 intIfNum,  netBufHandle bufHandle, BOOL rxFlag, BOOL txFlag);
extern void authmgrDebugPacketDump(uint32 flag,uint32 physPort,uint32 intIfNum, netBufHandle bufHandle);
extern void authmgrDebugDataDump(uint32 flag, uint32 physPort, uchar8  *data,uint32 len);

extern void authmgrDebugLogicalPortInfoShow(uint32 intIfNum, uint32 lIntIfNum);

#define AUTHMGR_USER_TRACE_TX(__fmt__, __args__... )                              \
      if (authmgrDebugPacketTraceTxFlag ==  TRUE)                               \
      {                                                                   \
        LOG_USER_TRACE( AUTHMGR_COMPONENT_ID, __fmt__,##__args__);        \
      }

#define AUTHMGR_USER_TRACE_RX(__fmt__, __args__... )                              \
      if (authmgrDebugPacketTraceRxFlag ==  TRUE)                               \
      {                                                                   \
        LOG_USER_TRACE( AUTHMGR_COMPONENT_ID, __fmt__,##__args__);        \
      }



#define AUTHMGR_TRACE_PORT_STATUS   0x0001
#define AUTHMGR_TRACE_EVENTS        0x0002
#define AUTHMGR_TRACE_API_CALLS     0x0004
#define AUTHMGR_TRACE_FSM_EVENTS    0x0008
#define AUTHMGR_TRACE_FAILURE       0x0010
#define AUTHMGR_TRACE_RADIUS        0x0020
#define AUTHMGR_TRACE_TIMER         0x0040
#define AUTHMGR_TRACE_MAC_ADDR_DB   0x0080
#define AUTHMGR_TRACE_CLIENT        0x0800

extern uint32 authmgrDebugTraceFlag;
extern uint32 authmgrDebugTraceIntf;

#define AUTHMGR_EVENT_TRACE(flag,intf,__fmt__, __args__...)         \
  /*if ((authmgrDebugTraceFlag&flag) != 0 && (intf==0 || authmgrDebugTraceIntf ==0 || intf == authmgrDebugTraceIntf))*/  \
  {  \
     char8  __buf1__[256];    \
    (void)osapiSnprintf (__buf1__, 256, __fmt__, ## __args__);  \
     LOGF(  LOG_SEVERITY_DEBUG, \
    "[%s:%d]%s",__FUNCTION__, __LINE__, __buf1__);  \
  }

void authmgrDevshellHelpPrint();

char *authmgrHostModeStringGet( AUTHMGR_HOST_CONTROL_t hostMode);
char *authmgrNodeTypeStringGet(authmgrNodeType_t type);
char *authmgrTimerTypeStringGet(authmgrTimerType_t type);
char *authmgrVlanTypeStringGet(authmgrVlanType_t type);
char *authmgrAuthStateStringGet(AUTHMGR_STATES_t state);
char *authmgrMethodStringGet( AUTHMGR_METHOD_t method);
char *authmgrMethodStatusStringGet( AUTHMGR_STATUS_t status);
char *authmgrSmEventStringGet(authmgrSmEvents_t event);
char *authmgrListTypeStringGet( AUTHMGR_METHOD_TYPE_t status);
char *authmgrClientTypeStringGet(authmgrClientType_t type);
char *authmgrAuthStatusStringGet( AUTHMGR_PORT_STATUS_t status);
void authmgrSuppMacStringGet( enetMacAddr_t *suppMacAddr);
char *authmgrAuthMethodStringGet(uint32 authMethod);
void authmgrDebugTraceEvent(uint32 debug,uint32 intfNum);
RC_t authmgrDebugTraceEventGet(uint32 *pDebug, uint32 *pIntfNum);

void authmgrBuildTestConfigData(void);
void authmgrDebugBuildDefaultConfigData(uint32 ver);
BOOL authmgrDebugHasDataChanged(void);

void authmgrDebugLogicalPortInfoShow(uint32 intIfNum, uint32 lIntIfNum);

RC_t authmgrLportPortGet(uint32 *intIfNum, uint32 *lIntIfNum);
void authmgrUserCountDump(uint32 intIfNum);
RC_t authmgrDebugLogicalPortInfoNextGet (uint32 intIfNum, uint32 *lIntIfNum, 
                                            authmgrLogicalPortDebugInfo_t *debugInfo);
  /* USE C Declarations */
#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_AUTHMGR_DEBUG_H*/
