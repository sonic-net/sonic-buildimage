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

#ifndef INCLUDE_MAB_CONTROL_H
#define INCLUDE_MAB_CONTROL_H

/* USE C Declarations */
#ifdef __cplusplus
  extern "C" {
#endif

#include <netinet/in.h>
#include "auth_mgr_common.h"

typedef enum mabControlEvents_s
{
  /***************************************************************/
  /* Events shared with all                                      */
  /***************************************************************/
  /*100*/mabControlBegin = 100,

  /***************************************************************/
  /* Events from Management commands                             */
  /***************************************************************/
  /*101*/mabMgmtPortInitializeSet,
  /*102*/mabMgmtPortControlModeSet,
  /*103*/mabMgmtPortHostModeSet,
  /*104*/mabMgmtPortStatsClear,
  /*105*/mabMgmtApplyConfigData, // No calls to API
  /*106*/mabMgmtPortMABEnableSet,
  /*107*/mabMgmtPortMABDisableSet,

  /*120*/mabMgmtEvents = 120, /*keep this last in sub group*/

  /***************************************************************/
  /* Events from AAA client                                      */
  /***************************************************************/
  /*121*/mabAaaInfoReceived,
  /*122*/mabRadiusConfigUpdate,
  
  /***************************************************************/
  /* Events from interface state changes                         */
  /***************************************************************/
  /*123*/mabIntfChange,
  /*124*/mabIntfStartup,

  /***************************************************************/
  /* Events from Vlan state changes                              */
  /***************************************************************/
  /*131*/mabVlanDeleteEvent = 131,
  /*132*/mabVlanAddEvent,
  /*133*/mabVlanAddPortEvent,
  /*134*/mabVlanDeletePortEvent,
  /*135*/mabVlanPvidChangeEvent,

  /***************************************************************/
  /* Events from authentication manager.                        */
  /***************************************************************/
  /*136*/mabAuthMgrEvent,

  /*137*/mabAddMacInMacDB,

  /***************************************************************/
  /* App timer events.                     */
  /***************************************************************/
  /*138*/ mabTimeTick,
  

}mabControlEvents_t;

/* Message structure to Global RADIUS config updates */
typedef struct mabRadiusGlobal_s
{
  unsigned char nas_ip[64];
  unsigned char nas_id[64];
}mabRadiusGlobal_t;

/* Message structure to RADIUS config updates */
typedef struct mabRadiusServer_s
{
  unsigned int cmd;
  union {
    mab_radius_server_t server;
    mabRadiusGlobal_t globalCfg;
  }cmd_data;
}mabRadiusServer_t;

typedef struct mabIpAaddr {
 unsigned char af; /* AF_INET / AF_INET6 */
 union {
         struct in_addr v4;
         struct in6_addr v6;
 } u;
}mabIpAaddr_t;

/* Message structure to hold responses from AAA client (i.e. RADIUS) */
typedef struct mabAaaMsg_s
{
  void *resp;
  unsigned int len;
} mabAaaMsg_t;

typedef struct mabIntfChangeParms_s 
{
  uint32 intfEvent;
  NIM_CORRELATOR_t nimCorrelator;
} mabIntfChangeParms_t;

/* Message structure to hold responses from authentication manager */
typedef struct mabAuthmgrMsg_s
{
  uint32 event;      /* event  */
  enetMacAddr_t  clientMacAddr;     /* client mac addr*/
} mabAuthmgrMsg_t;

typedef struct mabMsg_s
{
  uint32 event;
  uint32 intf;
  union
  {
    uint32                msgParm;
    mabAaaMsg_t           mabAaaMsg;
    mabIntfChangeParms_t  mabIntfChangeParms;
    dot1qNotifyData_t     vlanData;
    NIM_STARTUP_PHASE_t   startupPhase;
    mabAuthmgrMsg_t       mabAuthmgrMsg;
    mabRadiusServer_t     mabRadiusCfgMsg;
  }data;
} mabMsg_t;


#define MAB_MSG_COUNT   FD_MAB_MSG_COUNT
#define MAB_TIMER_TICK  1000 /*in milliseconds*/

extern RC_t mabStartTasks();
extern void mabTask();
extern void mabSrvrTask ();
extern void mabEloopTask ();
extern RC_t mabFillMsg(void *data, mabMsg_t *msg);
extern RC_t mabIssueCmd(uint32 event, uint32 intIfNum, void *data);
extern RC_t mabDispatchCmd(mabMsg_t *msg);
extern RC_t mabTimerAction();
extern RC_t mabAuthmgrEventMapFnGet(uint32 event, mabAuthmgrEventFnMap_t *elem);
extern RC_t mabCtlPortInitializeSet(uint32 intIfNum,  BOOL initialize);
extern RC_t mabCtlPortControlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl);
extern RC_t mabPortCtrlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl);
extern RC_t mabCtlPortStatsClear(uint32 intIfNum);
extern RC_t mabCtlApplyConfigData(void);
extern RC_t mabCtlApplyPortConfigData(uint32 intIfNum);
extern RC_t mabRadiusServerVlanConversionHandle(const  char8 *vlanName, uint32 *vlanId);
extern RC_t mabVlanChangeCallback(dot1qNotifyData_t *vlanData, uint32 intIfNum, uint32 event);
extern void mabVlanChangeProcess(uint32 event, uint32 intIfNum, dot1qNotifyData_t *vlanData);
extern RC_t mabCheckMapPdu(uint32 intIfNum,  char8 *srcMac, uint32 *logicalPort, BOOL *existing_node);
extern RC_t mabVlanPVIDChangeEventProcess(uint32 intIfNum,uint32 vlanId);

extern RC_t mabCtlPortMABEnableSet(uint32 intIfNum);
extern RC_t mabCtlPortMABDisableSet(uint32 intIfNum);
extern RC_t mabCtlLogicalPortMABGenResp(uint32 lIntIfNum,  BOOL generateNak);

extern RC_t mabPortControlForceUnAuthActionSet(uint32 intIfNum);

int mab_socket_server_handle(int *listen_sock);
int mabPortClientAuthStatusUpdate(int intIfNum, unsigned char *addr, char *status, void *param);

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
RC_t mabPortControlAutoActionSet(uint32 intIfNum);

/*********************************************************************
 * @purpose control function to set the host mode to multi-domain-auth 
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
RC_t mabControlMultiDomainHostAuthActionSet(uint32 intIfNum);

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
RC_t mabControlMultiHostActionSet(uint32 intIfNum);

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
RC_t mabControlSingleAuthActionSet(uint32 intIfNum);

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
RC_t mabControlMultAuthActionSet(uint32 intIfNum);

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
RC_t mabPortControlForceAuthActionSet(uint32 intIfNum);

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
RC_t mabPortControlForceUnAuthActionSet(uint32 intIfNum);

/*********************************************************************
 * @purpose function to clean up mab port oper info
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
RC_t mabPortInfoCleanup(uint32 intIfNum);

RC_t mabHostModeMapInfoGet( AUTHMGR_HOST_CONTROL_t type, mabHostModeMap_t *elem);

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
RC_t mabPortCtrlHostModeSet(uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t hostMode);

/*********************************************************************
 * @purpose  Actions to be performed in the APM state DISCONNECTED
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabUnAuthenticatedAction(mabLogicalPortInfo_t *logicalPortInfo);

/*********************************************************************
 * @purpose  Actions to be performed in the APM state AUTHENTICATED
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabAuthenticatedAction(mabLogicalPortInfo_t *logicalPortInfo);

/*********************************************************************
 * @purpose  Actions to be performed in the APM state AUTHENTICATING
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabAuthenticatingAction(mabLogicalPortInfo_t *logicalPortInfo);

/*********************************************************************
 * @purpose  Add supplicant MAC in MAC database
 *
 * @param    lIntIfNum   @b{(input)} logical interface number that this PDU was received on
 *
 * @returns   SUCCESS or  FAILURE
 * 
 * @end
 *********************************************************************/
RC_t mabAddMac(uint32 lIntIfNum);

RC_t mabAuthenticationInitiate(uint32 intIfNum,  enetMacAddr_t suppMacAddr);

RC_t mabAuthmgrEventProcess(uint32 intIfNum, mabAuthmgrMsg_t *authmgrParams);

/*********************************************************************
* @purpose  Set values of the Logical Dot1x Port Structure
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
RC_t mabLogicalPortInfoInit(uint32 lIntIfNum);

/*********************************************************************
 * @purpose API to check and clear appTimer Deinit 
 *
 * @param   none 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabAppTimerDeInitCheck(void);

RC_t mabRadiusChangeHandle(mabRadiusServer_t *info);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_CONTROL_H */
