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


#ifndef INCLUDE_AUTHMGR_CONTROL_H
#define INCLUDE_AUTHMGR_CONTROL_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef AUTHMGR_MAC_MOVE_ON
#define AUTHMGR_MAC_MOVE_ON  
#endif

typedef enum authmgrControlEvents_s
{
  /***************************************************************/
  /* Events shared with all                                      */
  /***************************************************************/
  /*100*/authmgrControlBegin = 100,

  /***************************************************************/
  /* Events from Management commands                             */
  /***************************************************************/
  
  /*101*/authmgrMgmtPortInitializeSet,
  /*102*/authmgrMgmtLogicalPortInitializeSet,
  /*103*/authmgrMgmtPortReauthenticateSet,
  /*105*/authmgrMgmtPortControlModeSet,
  /*106*/authmgrMgmtHostControlModeSet,
  /*110*/authmgrMgmtPortQuietPeriodSet,
  /*111*/authmgrMgmtPortReAuthPeriodSet,
  /*112*/authmgrMgmtPortReAuthEnabledSet,
  /*113*/authmgrMgmtPortStatsClear,
  /*114*/authmgrMgmtApplyConfigData,
  /*115*/authmgrMgmtApplyPortConfigData,
  /*116*/authmgrMgmtPortMaxUsersSet,
  /*118*/authmgrMgmtLogicalPortReauthenticateSet,
  /*119*/authmgrMethodOrderModify,
  /*120*/authmgrMethodPriorityModify,
  /*123*/authmgrPaeCapabilitiesEvent,
  /*124*/authmgrViolationModeSet,
  /*125*/authmgrMaxAuthAttemptsSet,

  /*129*/authmgrMgmtEvents, /*keep this last in sub group*/

  /***************************************************************/
  /* Events from network pdu received                            */
  /***************************************************************/
  /*130*/authmgrNetworkEvents,

  /***************************************************************/
  /* Events from AAA client                                      */
  /***************************************************************/
  /*131*/authmgrAaaInfoReceived,

  /*132*/authmgrAaaEvents,
  
  /***************************************************************/
  /* Events from interface state changes                         */
  /***************************************************************/
  /*133*/authmgrIntfChange,
  /*134*/authmgrIntfStartup,

  /*135*/authmgrIntfChangeEvents,

  /***************************************************************/
  /* Events from Vlan state changes                              */
  /***************************************************************/

  /*136*/authmgrVlanDeleteEvent,
  /*137*/authmgrVlanAddEvent,
  /*138*/authmgrVlanAddPortEvent,
  /*139*/authmgrVlanDeletePortEvent,
  /*140*/authmgrVlanPvidChangeEvent,
  /*140*/authmgrVlanConfDeleteEvent,
  /*140*/authmgrVlanConfPortDeleteEvent,
  /***************************************************************/
  /* Events from configurator                                    */
  /***************************************************************/
  /*141*/authmgrCnfgr,        

  /*142*/authmgrCnfgrEvents,

  /*143*/authmgrUnauthAddrCallBackEvent,
  /*145*/authmgrClientTimeout,
  /***************************************************************/
  /*147*/authmgrDelDuplicateEntry,
  /*148*/authmgrAddMacInMacDB,
  /*149*/authmgrClientCleanup,
  /***************************************************************/
  /* Events from Radius.                                         */
  /***************************************************************/
  /* authmgr app  timer events */
  /* 154*/ authmgrTimeTick,
  /* 155*/ authmgrAuthenticationStart,
  /* 156*/ authMgr8021xEnableDisable,
  /* 157*/ authmgrMabEnableDisable,
  /* 159*/ authmgrAuthMethodCallbackEvent,
  /* 164*/ authmgrMgmtAdminModeEnable,
  /* 165*/ authmgrMgmtAdminModeDisable,
  /* 167*/ authmgrDynamicVlanModeEnable,
  /* 168*/ authmgrDynamicVlanModeDisable,
  /* 169*/ authmgrMgmtPortInactivePeriodSet,

  /* 179*/ authmgrCtlPortInfoReset,
}authmgrControlEvents_t;

/* Message structure to hold responses from AAA client (i.e. RADIUS) */
typedef struct authmgrAaaMsg_s
{
  uint32 status;      /* status of response (i.e. RADIUS_STATUS_SUCCESS, etc.) */
  uint32 respLen;     /* length of data (response) being passed */
   uchar8 *pResponse;  /* pointer to response from AAA server */
} authmgrAaaMsg_t;

typedef struct authmgrIntfChangeParms_s 
{
  uint32 intfEvent;
  NIM_CORRELATOR_t nimCorrelator;
} authmgrIntfChangeParms_t;

typedef struct authmgrUnauthCallbackParms_s
{
   enetMacAddr_t  macAddr;
   ushort16       vlanId;
} authmgrUnauthCallbackParms_t;

typedef struct authmgrMgmtTimePeriod_s
{
   BOOL reAuthPeriodServer;
  uint32 val;
} authmgrMgmtTimePeriod_t;

typedef struct authmgrMsg_s
{
  uint32 event;
  uint32 intf;
  union
  {
    uint32              msgParm;
    netBufHandle        bufHandle;
    authmgrAaaMsg_t          authmgrAaaMsg;
    authmgrIntfChangeParms_t authmgrIntfChangeParms;
    NIM_STARTUP_PHASE_t    startupPhase;
    authmgrAuthRespParams_t authParams;
    authmgrMgmtTimePeriod_t timePeriod;
  }data;
} authmgrMsg_t;

typedef struct authmgrBulkMsg_s
{
  uint32 event;
  uint32 intf;
  union
  {
    authmgrUnauthCallbackParms_t   unauthParms;
  }data;
} authmgrBulkMsg_t;

typedef struct authmgrVlanMsg_s
{
  uint32 event;
  uint32 intf;
  union
  {
    dot1qNotifyData_t      vlanData;
  }data;
} authmgrVlanMsg_t;

#define AUTHMGR_MSG_COUNT       FD_AUTHMGR_MSG_COUNT
#define AUTHMGR_VLAN_MSG_COUNT  (16 * 1024)
#define AUTHMGR_TIMER_TICK      1000 /*in milliseconds*/

typedef RC_t(*authmgrStatusMapFn_t) (uint32 lIntIfNum, authmgrAuthRespParams_t *params);

typedef struct authmgrStatusMap_s
{
   AUTHMGR_STATUS_t  type;
  authmgrStatusMapFn_t  statusFn;
}authmgrStatusMap_t;

typedef RC_t(*authmgrPortControlChangeNotifyFn_t) (uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl);
typedef RC_t(*authmgrHostControlChangeNotifyFn_t) (uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t hostMode);
typedef RC_t(*authmgrClientEventNotifyFn_t) (uint32 intIfNum, uint32 event,  enetMacAddr_t *macAddr);
typedef RC_t(*authmgrMethodOperEnableGetFn_t) (uint32 intIfNum, uint32 *enable);

typedef struct authmgrMethodCallbackNotifyMap_s
{
  AUTHMGR_METHOD_t method;
  authmgrPortControlChangeNotifyFn_t portCtrlFn;
  authmgrHostControlChangeNotifyFn_t hostCtrlFn;
  authmgrClientEventNotifyFn_t  eventNotifyFn;
  authmgrMethodOperEnableGetFn_t    enableGetFn;
  authmgrMethodOperEnableGetFn_t    radiusEnabledGetFn;
}authmgrMethodCallbackNotifyMap_t;

extern RC_t authmgrStartTasks();
extern RC_t authmgrFillMsg(void *data, authmgrMsg_t *msg);
extern RC_t authmgrBulkFillMsg(void *data, authmgrBulkMsg_t *msg);
extern RC_t authmgrVlanFillMsg (void *data, authmgrVlanMsg_t * msg);
extern RC_t authmgrIssueCmd(uint32 event, uint32 intIfNum, void *data);
extern RC_t authmgrDispatchCmd(authmgrMsg_t *msg);
extern RC_t authmgrBulkDispatchCmd(authmgrBulkMsg_t *msg);
extern RC_t authmgrVlanDispatchCmd (authmgrVlanMsg_t * msg);
extern RC_t authmgrTimerAction();

extern RC_t authmgrCtlPortInitializeSet(uint32 intIfNum,  BOOL initialize);
extern RC_t authmgrCtlLogicalPortInitializeSet(uint32 lIntIfNum);
extern RC_t authmgrCtlPortReauthenticateSet(uint32 intIfNum,  BOOL reauthenticate);
extern RC_t authmgrCtlLogicalPortReauthenticateSet(uint32 lIntIfNum,  BOOL reauthenticate);
extern RC_t authmgrCtlPortControlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl);
extern RC_t authmgrCtlPortPaeCapabilitiesSet(uint32 intIfNum, uint32 capabilities);
extern RC_t authmgrCtlPortPaeCapabilitiesInGlobalDisableSet(uint32 intIfNum, uint32 paeCapabilities);
extern RC_t authmgrCtlPortQuietPeriodSet(uint32 intIfNum, uint32 quietPeriod);
extern RC_t authmgrCtlPortTxPeriodSet(uint32 intIfNum, uint32 txPeriod);
extern RC_t authmgrCtlPortReAuthPeriodSet(uint32 intIfNum, authmgrMgmtTimePeriod_t *params);
extern RC_t authmgrCtlPortReAuthEnabledSet(uint32 intIfNum,  BOOL reAuthEnabled);
extern RC_t authmgrCtlPortStatsClear(uint32 intIfNum);
extern RC_t authmgrCtlApplyConfigData(void);
extern RC_t authmgrCtlApplyPortConfigData(uint32 intIfNum);
extern RC_t authmgrRadiusServerVlanAssignmentHandle(uint32 intIfNum,uint32 vlanId);
extern RC_t authmgrRadiusServerVlanConversionHandle(const  char8 *vlanName, uint32 *vlanId);
extern RC_t authmgrVlanAssignmentEnable(authmgrLogicalPortInfo_t *logicalPortInfo,uint32 vlanId);
extern RC_t authmgrVlanAssignmentDisable(uint32 intIfNum,uint32 vlanId);
extern RC_t authmgrPortVlanAssignmentDisable(uint32 intIfNum);
extern RC_t authmgrApplyConfigCompleteCb(uint32 event);
extern void authmgrVlanChangeProcess(uint32 event, uint32 intIfNum, dot1qNotifyData_t *vlanData);
extern RC_t authmgrCheckMapPdu(uint32 intIfNum,  char8 *srcMac, uint32 *logicalPort, BOOL *existing_node);
extern RC_t authmgrCtlPortMaxUsersSet(uint32 intIfNum, uint32 maxUsers);
extern RC_t authmgrCtlApplyLogicalPortConfigData(uint32 lIntIfNum);
extern RC_t authmgrCtlResetLogicalPortSessionData(authmgrLogicalPortInfo_t *logicalPortInfo);
extern RC_t authmgrCtlStopLogicalPortSessionData(authmgrLogicalPortInfo_t *logicalPortInfo);
extern RC_t authmgrPortVlanMembershipSet(uint32 intIfNum, uint32 vlanId, BOOL flag);
extern RC_t authmgrVlanAddPortEventProcess(uint32 intIfNum,uint32 VlanId);
extern RC_t authmgrVlanAddEventProcess(uint32 intIfNum,uint32 VlanId);
extern RC_t authmgrVlanDeletePortEventProcess(uint32 intIfNum,uint32 VlanId);
extern RC_t authmgrVlanPVIDChangeEventProcess(uint32 intIfNum,uint32 vlanId);

extern RC_t authmgrCtlLogicalPortVlanAssignedReset(uint32 lIntIfNum);
extern RC_t authmgrCtlLogicalPortVlanAssignmentDisable(authmgrLogicalPortInfo_t *logicalPortInfo);
extern RC_t authmgrCtlPortUnauthAddrCallbackProcess(uint32 intIfNum, enetMacAddr_t macAddr, ushort16 vlanId);

/*MAB*/
extern RC_t authmgrCtlLogicalPortMABTimerStart(uint32 lIntIfNum);
extern RC_t authmgrCtlPortMABEnableSet(uint32 intIfNum);
extern RC_t authmgrCtlPortMABDisableSet(uint32 intIfNum);
extern RC_t authmgrCtlLogicalPortMABRemove(uint32 llIntIfNum);
extern RC_t authmgrCtlLogicalPortMABOperational(uint32 llIntIfNum);
extern RC_t authmgrCtlLogicalPortMABAuthFailGuestVlanSet(authmgrLogicalPortInfo_t *logicalPortInfo);
extern RC_t authmgrCtlLogicalPortMABGuestVlanReset(uint32 lIntIfNum);
extern RC_t authmgrCtlLogicalPortMABGenResp(uint32 lIntIfNum,  BOOL generateNak);

extern RC_t authmgrCtlPortReset(uint32 intIfNum,  BOOL initialize);

/* Authmgr Client Timeout API */
RC_t authmgrCtlLogicalPortClientTimeout(uint32 lIntIfNum);

/*********************************************************************
* @purpose  Disconnect the client
*
* @param    intIfNum       @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlClientCleanup (uint32 lIntIfNum);

/*********************************************************************
 * @purpose  Used to change port admin mode. 
 *
 * @param    intIfNum       @b{(input)) internal interface number
 * @param    adminMode      @b{(input)) administrative mode
 *
 * @returns   SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrCtlPortAdminMode(uint32 intIfNum, uint32 adminMode);

/*********************************************************************
 * @purpose control mode function to set the port control mode to auto 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPortControlAutoActionSet(uint32 intIfNum);

/*********************************************************************
 * @purpose control function to set the host mode to multi host 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrControlMultiHostActionSet(uint32 intIfNum);

/*********************************************************************
 * @purpose control function to set the host mode to single host mode 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrControlSingleAuthActionSet(uint32 intIfNum);

/*********************************************************************
 * @purpose control function to set the host mode to multi auth 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrControlMultAuthActionSet(uint32 intIfNum);

/*********************************************************************
 * @purpose control function to set the to force authorized 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPortControlForceAuthActionSet(uint32 intIfNum);

/*********************************************************************
 * @purpose control function to set the to force un-authorized 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPortControlForceUnAuthActionSet(uint32 intIfNum);

/*********************************************************************
 * @purpose function to clean up authmgr port oper info
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPortInfoCleanup(uint32 intIfNum);

/*********************************************************************
 * @purpose function to check policy validation based on host mode 
 *
 * @param    hostMode   @b{(input))  hostmode 
 * @param    *appyPolicy  @b{(input)) bool value 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrHostModeHwPolicyApply( AUTHMGR_HOST_CONTROL_t hostMode, uint32 intIfNum,  BOOL install);


/*********************************************************************
 * @purpose Set  authmgr authenticated client in specified VLAN
 *
 * @param    logicalPortInfo  @b{(input))  Logical Port Info node
 * @param    reason      @b{(input)) Reason for the assignment
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrClientVlanInfoSet(authmgrLogicalPortInfo_t *logicalPortInfo,
                               uint32 vlanId);

/*********************************************************************
 * @purpose Set  authmgr authenticated client in specified VLAN
 *
 * @param    logicalPortInfo  @b{(input))  Logical Port Info node
 * @param    reason      @b{(input)) Reason for the assignment
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrClientVlanInfoReset(uint32 physPort,
                               uint32 vlanId);


void authmgrTimerExpiryHdlr( APP_TMR_CTRL_BLK_t timerCtrlBlk, void* ptrData);

/*************************************************************************
 * @purpose  Starts the specified timer
 *
 * @param    intIfNum      @b{(input)}  Interface for starting the timer
 * @param    timerType     @b{(input)}  Interface/Timer type
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments none
 *
 * @end
 *************************************************************************/
RC_t authmgrTimerStart(authmgrLogicalPortInfo_t *logicalPortInfo, authmgrTimerType_t type);

RC_t authmgrTxPeriodGet(uint32 intIfNum, uint32 *val);

RC_t authmgrQuietPeriodGet(uint32 intIfNum, uint32 *val);

RC_t authmgrReAuthPeriodGet(uint32 intIfNum, uint32 *val);

RC_t authmgrServerTimeoutPeriodGet(uint32 intIfNum, uint32 *val);

RC_t authmgrHostModeMapInfoGet( AUTHMGR_HOST_CONTROL_t type, authmgrHostModeMap_t *elem);
/*********************************************************************
 * @purpose control mode function to set the port host mode 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPortCtrlHostModeSet(uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t hostMode);

/*********************************************************************
 * @purpose  Control function to handle the authentication method order changes 
 *
 * @param    intIfNum       @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrMethodOrderChangeProcess(uint32 intIfNum);
/*********************************************************************
 * @purpose  authmgr task which serves the request queue
 *
 * @param    none
 *
 * @returns  void
 *
 * @comments User-interface writes and  all are serviced off
 *           of the authmgrQueue
 *
 * @end
 *********************************************************************/
void authmgrTask();
/*********************************************************************
* @purpose  authmgr srvr task which serves the request queue
*
* @param    none
*
* @returns  void
*
* @comments external applications are serviced off
*           of the authmgrQueue
*
* @end
*********************************************************************/
void authmgrSrvrTask ();

/*********************************************************************
 * @purpose  Control function to handle the events received from methods 
 *
 * @param    intIfNum       @b{(input)) internal interface number
 * @param    status       @b{(input)) status from the calling applications like 
 802.1X/MAB/CP
 * @param    macAddr       @b{(input)) mac addr of the client 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrClientCallbackEventProcess(uint32 intIfNum,
                             authmgrAuthRespParams_t *callbackParams);

/*********************************************************************
 * @purpose  Function to Update the statistics
 *
 * @param    intIfNum       @b{(input)) internal interface number
 * @param    method         @b{(input)) 802.1x/mab/cp
 * @param    mode           @b{(input))  TRUE/ FALSE
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments  TRUE will update the attempts, and  FALSE will update the
 failed attempts
 *
 * @end
 *********************************************************************/
RC_t authmgrStatsUpdate(uint32 intIfNum,
                            AUTHMGR_METHOD_t method,
                           authmgrStatsUpdate_t status);

/*********************************************************************
 * @purpose  Get the next operationally enabled method on a interface 
 *
 * @param    intIfNum  @b{(input)) internal interface number 
 * @param    method  @b{(input))  input method for which next method is needed. 
 * @param    nextMethod  @b{(output)) pointer to the next method 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/

RC_t authmgrNextMethodGet(uint32 intIfNum,
                              AUTHMGR_METHOD_t *nextMethod);

RC_t authmgrRegisteredEntryFnMapGet( AUTHMGR_METHOD_t method, authmgrMethodCallbackNotifyMap_t *entry);

RC_t authmgrAuthenticationTrigger(authmgrLogicalPortInfo_t *logicalPortInfo);
RC_t authmgrPortEnabledMethodCountGet(uint32 physPort, uint32 *count);
/*********************************************************************
 * @purpose  Get the next operationally enabled method on a interface
 *
 * @param    intIfNum  @b{(input)) internal interface number
 * @param    method  @b{(input))  input method for which next method is needed.
 * @param    nextMethod  @b{(output)) pointer to the next method
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/

RC_t authmgrEnabledMethodNextGet(uint32 intIfNum,
     AUTHMGR_METHOD_t *nextMethod);

/*********************************************************************
 * @purpose updates the port pae capabilities
 *
 * @param   intIfNum
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPaeCapabilitiesEventProcess(uint32 intIfNum, uint32 mode);

/*********************************************************************
 * @purpose updates the port violation mode 
 *
 * @param   intIfNum
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrViolationModeSetAction(uint32 intIfNum,  AUTHMGR_PORT_AUTH_VIOLATION_MODE_t mode);

/*********************************************************************
 * @purpose updates the port max auth retry attempts 
 *
 * @param   intIfNum
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrAuthFailMaxRetryCountSetAction(uint32 intIfNum, uint32  count);

RC_t authmgrTimerReset(authmgrTimerType_t type);

RC_t authmgrMethodModifyAction(uint32 intIfNum);
RC_t authmgrLogicalPortReAuthPeriodGet(uint32 lIntfNum, uint32 *val);

 void authmgrAllTimersStart(authmgrTimerType_t type,  BOOL flag);
 void authmgrAuthClientsTimersRestart();

/*********************************************************************
 * @purpose updates the port max auth retry attempts 
 *
 * @param   intIfNum
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrAuthFailMaxRetryCountSetAction(uint32 intIfNum, uint32  count);

/*********************************************************************
 * @purpose  Set port control mode
 *
 * @param    intIfNum    @b{(input)) internal interface number
 * @param    portControl @b{(input)) port control mode
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPortCtrlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl);

/*********************************************************************
 * @purpose  Enable administrative mode setting for authmgr
 *
 * @param    none
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrCtlAdminModeEnable();

/*********************************************************************
 * @purpose  Disable administrative mode setting for authmgr
 *
 * @param    none
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrCtlAdminModeDisable();

/*********************************************************************
* @purpose  Initialize the Authmgr Port Structure with Default Values
*
* @param    intIfNum  @b{(input))  internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortInfoInitialize(uint32 intIfNum,  BOOL flag);

/*********************************************************************
* @purpose  Set values of the Logical Authmgr Port Structure
*           with Default Values of port it belongs to
*
* @param    logicalPortInfo  @b{(input))  Logical port Info
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortInfoInit(uint32 lIntIfNum);

/*********************************************************************
* @purpose  Populate Auth Manager Global Info structure
*
* @param    none
*
* @returns  none
*
* @comments
*
* @end
*********************************************************************/
void authmgrGlobalInfoPopulate();

/*********************************************************************
 * @purpose  Handle dynamic vlan enable event 
 *
 * @param    none
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrCtlDynamicVlanEnableProcess();

/*********************************************************************
 * @purpose  Disable administrative mode setting for authmgr
 *
 * @param    none
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrCtlDynamicVlanDisableProcess();

/*********************************************************************
* @purpose function to start timers on an interface
*
* @param phyIntf interface number
* @param type timer type
* @param flag start or stop
*
* @comments
*
* @end
*********************************************************************/
void authmgrIntfClientsTimerStart (uint32 phyIntf, authmgrTimerType_t type,
                                    BOOL flag);

/*********************************************************************
* @purpose  Used to get client inactivity timeout 
*
* @param    val     @b{(input)) periodic timeout in seconds
*
* @returns   SUCCESS
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlPortInactivityPeriodGet (uint32 intIfNum,
                                        uint32 * val);

/*********************************************************************
* @purpose function to start timers on an interface
*
* @param phyIntf interface number
* @param type timer type
* @param flag start or stop
*
* @comments
*
* @end
*********************************************************************/
void authmgrIntfClientsTimerStart (uint32 phyIntf, authmgrTimerType_t type,
                                    BOOL flag);

/*********************************************************************
* @purpose  Used to get client inactivity timeout 
*
* @param    val     @b{(input)) periodic timeout in seconds
*
* @returns   SUCCESS
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlPortInactivityPeriodGet (uint32 intIfNum,
                                        uint32 * val);

RC_t authmgrCtlPortMethodNoRespPeriodGet (uint32 intIfNum,
                                        uint32 * val);

/*********************************************************************
* @purpose  To close the authenticated sessions gracefully.
*
* @returns    SUCCESS
* @returns    FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrTerminateAuthSessions();

/* USE C Declarations */
#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_AUTHMGR_CONTROL_H */
