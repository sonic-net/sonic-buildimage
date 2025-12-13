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


#include "auth_mgr_include.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "auth_mgr_struct.h"
#include "auth_mgr_api.h"
#include "auth_mgr_timer.h"

extern authmgrCB_t *authmgrCB;

#define AUTHMGR_DEBUG_PACKET_RX_FORMAT     "Pkt RX - Intf: %s (%d,%s),SrcMac: %s DestMac: %s Type: %s\n"
#define AUTHMGR_DEBUG_PACKET_RX_EAP_FORMAT "Pkt RX - Intf: %s (%d,%s),SrcMac: %s DestMac: %s Type: %s Code: %s Id:%d\n"
#define AUTHMGR_DEBUG_PACKET_TX_FORMAT     "Pkt TX - Intf: %s (%d,%s),SrcMac: %s DestMac: %s Type: %s Code: %s\n"
#define AUTHMGR_DEBUG_PACKET_TX_EAP_FORMAT "Pkt TX - Intf: %s (%d,%s),SrcMac: %s DestMac: %s Type: %s Code: %s Id:%d\n"

 BOOL authmgrDebugPacketTraceTxFlag =  FALSE;
 BOOL authmgrDebugPacketTraceRxFlag =  FALSE;
uint32 authmgrDebugTraceFlag = 0;
uint32 authmgrDebugTraceIntf = 0;

/*  Function prototypes */
void authmgrBuildTestIntfConfigData (authmgrPortCfg_t * pCfg,  ushort16 seed);
void authmgrConfigDataTestShow (void);


/*********************************************************************
* @purpose  Display the number of messages in the authmgr message queues
*
* @param  none
*
* @returns  void
*
* @comments devshell command
*
* @end
*********************************************************************/
void authmgrDebugMsgQueue ()
{
   int32 num;

  if (osapiMsgQueueGetNumMsgs (authmgrCB->authmgrBulkQueue, &num) ==  SUCCESS)
  {
    SYSAPI_PRINTF (
                   "Authmgr Messages in bulk queue: %d\n", num);
  }

  if (osapiMsgQueueGetNumMsgs (authmgrCB->authmgrQueue, &num) ==  SUCCESS)
  {
    SYSAPI_PRINTF (
                   "Authmgr Messages in queue: %d\n", num);
  }
}

/*********************************************************************
* @purpose  Display the ID of the authmgr Trace Buffer
*
* @param none
*
* @returns  void
*
* @comments devshell command
*
* @end
*********************************************************************/
void authmgrDebugTraceIdGet ()
{
  SYSAPI_PRINTF (
                 "Trace Id in use for authmgr is %d\n",
                 authmgrCB->globalInfo->authmgrInfo.traceId);
  SYSAPI_PRINTF (
                 "Use devshell traceBlockStart(traceId) and traceBlockStop(traceId)\n");
}

/*********************************************************************
* @purpose  Display the sizes of the authmgr structures
*
* @param none
*
* @returns  void
*
* @comments devshell command
*
* @end
*********************************************************************/
void authmgrDebugSizesShow ()
{
  SYSAPI_PRINTF (
                 "AUTHMGR Data Structures:\r\n");
  SYSAPI_PRINTF (
                 "----------------------\r\n");
  SYSAPI_PRINTF (
                 "sizeof authmgrCB->globalInfo = %zd\r\n",
                 sizeof (authmgrGlobalInfo_t));
  SYSAPI_PRINTF (
                 "sizeof authmgrPortInfo_t  = %zd\r\n",
                 sizeof (authmgrPortInfo_t));
  SYSAPI_PRINTF (
                 "sizeof authmgrPortStats_t = %zd\r\n",
                 sizeof (authmgrPortStats_t));
  SYSAPI_PRINTF (
                 "sizeof authmgrCfg_t       = %zd\r\n", sizeof (authmgrCfg_t));
  SYSAPI_PRINTF (
                 " MAX_PORT_COUNT       = %d\r\n",  MAX_PORT_COUNT);
  SYSAPI_PRINTF (
                 " AUTHMGR_INTF_MAX_COUNT = %d\r\n",
                  AUTHMGR_INTF_MAX_COUNT);
}

/*********************************************************************
* @purpose  Display the config info for the specified port
*
* @param    intIfNum @b{(input)) internal interface number
*
* @returns  void
*
* @comments devshell command
*
* @end
*********************************************************************/
void authmgrDebugPortCfgShow (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    SYSAPI_PRINTF (
                   "intIfNum(%d) is not a valid authmgr interface\r\n",
                   intIfNum);
    return;
  }

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    SYSAPI_PRINTF (
                   "intIfNum(%d) is not a valid authmgr interface\r\n",
                   intIfNum);
    return;
  }

  SYSAPI_PRINTF (
                 "AUTHMGR Cfg Info for port %d:\r\n", intIfNum);
  SYSAPI_PRINTF (
                 "--------------------------\r\n");

  SYSAPI_PRINTF (
                 "portControlMode           = %d", pCfg->portControlMode);
  switch (pCfg->portControlMode)
  {
  case  AUTHMGR_PORT_FORCE_UNAUTHORIZED:
    {
      SYSAPI_PRINTF (
                     " Force Unauthorized\r\n");
    }
    break;
  case  AUTHMGR_PORT_AUTO:
    {
      SYSAPI_PRINTF (" Auto\r\n");
    }
    break;
  case  AUTHMGR_PORT_FORCE_AUTHORIZED:
    {
      SYSAPI_PRINTF (
                     " Force Authorized\r\n");
    }
    break;
  default:
    {
      SYSAPI_PRINTF (" Unknown\r\n");
    }
    break;
  }

  SYSAPI_PRINTF (
                 "hostMode               = %s\r\n",
                 authmgrHostModeStringGet (pCfg->hostMode));

  SYSAPI_PRINTF (
                 "quietPeriod               = %d\r\n", pCfg->quietPeriod);
  SYSAPI_PRINTF (
                 "reAuthPeriod              = %d\r\n", pCfg->reAuthPeriod);

  SYSAPI_PRINTF (
                 "reAuthEnabled             = %d\r\n", pCfg->reAuthEnabled);

  SYSAPI_PRINTF (
                 "reAuthServerEnabled       = %d", pCfg->reAuthPeriodServer);
  if (authmgrCB->globalInfo->authmgrCfg->authmgrPortCfg[intIfNum].
      reAuthEnabled ==  TRUE)
  {
    SYSAPI_PRINTF (" True\r\n");
  }
  else if (pCfg->reAuthEnabled ==  FALSE)
  {
    SYSAPI_PRINTF (" False\r\n");
  }
  else
  {
    SYSAPI_PRINTF (" Unknown\r\n");
  }


  SYSAPI_PRINTF (
                 "Auth Server port Max Auth attempts = %d\r\n", pCfg->maxAuthAttempts);

}

/*********************************************************************
* @purpose  Display the status info for the specified port
*
* @param    lIntIfNum @b{(input)) Logical internal interface number
*
* @returns  void
*
* @comments devshell command
*
* @end
*********************************************************************/
void authmgrDebugPortMacInfoShow (uint32 lIntIfNum)
{
   uchar8 buf[64];
   uchar8 zeroV6[16];
  authmgrLogicalPortInfo_t *logicalPortInfo;
  uint32 physPort = 0, lPort = 0, type = 0;

  authmgrLogicalPortInfoTakeLock ();
  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  if (logicalPortInfo ==  NULLPTR)
  {
    authmgrLogicalPortInfoGiveLock ();
    return;
  }

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, lIntIfNum);

  if (authmgrIsValidIntf (physPort) !=  TRUE)
  {
    SYSAPI_PRINTF (
                   "intIfNum is not a valid authmgr interface(%d)\r\n",
                   physPort);
    authmgrLogicalPortInfoGiveLock ();
    return;
  }

  SYSAPI_PRINTF (
                 "AUTHMGR Info for port %d Phy port(%d) :\r\n", lIntIfNum,
                 physPort);
  SYSAPI_PRINTF (
                 "------------------------------\r\n");

  if (0 == logicalPortInfo->key.keyNum)
  {
    SYSAPI_PRINTF ("Port is in Use\r\n");
  }
  else
  {
    SYSAPI_PRINTF ("Port Not in Use\r\n");
  }

  SYSAPI_PRINTF (
                 "Port Status               = %d\n",
                 logicalPortInfo->client.logicalPortStatus);

  SYSAPI_PRINTF (
                 "\n\rTimers operational \n\r");

  SYSAPI_PRINTF (
                 "---------- --------------- --------------- ---------- --------------\n\r");

  SYSAPI_PRINTF (
                 "%s\n",
                 authmgrTimerTypeStringGet (logicalPortInfo->authmgrTimer.cxt.
                                            type));

  SYSAPI_PRINTF (
                 "auth state %s \r\n",
                 authmgrAuthStateStringGet (logicalPortInfo->protocol.
                                            authState));

  if (logicalPortInfo->client.serverStateLen > 0)
  {
    uint32 i;
    SYSAPI_PRINTF (
                   "serverState               = ");
    for (i = 0; i < logicalPortInfo->client.serverStateLen; i++)
    {
      SYSAPI_PRINTF (
                     "%02X", logicalPortInfo->client.serverState[i]);
    }
    SYSAPI_PRINTF ("\r\n");
  }
  else
  {
    SYSAPI_PRINTF (
                   "serverState               = NULL\r\n");
  }

  if (logicalPortInfo->client.serverClassLen > 0)
  {
    uint32 i;
    SYSAPI_PRINTF (
                   "serverClass               = ");
    for (i = 0; i < logicalPortInfo->client.serverClassLen; i++)
    {
      SYSAPI_PRINTF (
                     "%02X", logicalPortInfo->client.serverClass[i]);
    }
    SYSAPI_PRINTF ("\r\n");
  }
  else
  {
    SYSAPI_PRINTF (
                   "serverClass               = NULL\r\n");
  }

  SYSAPI_PRINTF (
                 "sessionTimeout            = %d\r\n",
                 logicalPortInfo->client.sessionTimeout);

  SYSAPI_PRINTF (
                 "terminationAction         = %d\r\n",
                 logicalPortInfo->client.terminationAction);

  memset (buf, 0, 32);
  osapiSnprintf (( char8 *) buf, sizeof (buf),
                 "%02X:%02X:%02X:%02X:%02X:%02X",
                 logicalPortInfo->client.suppMacAddr.addr[0],
                 logicalPortInfo->client.suppMacAddr.addr[1],
                 logicalPortInfo->client.suppMacAddr.addr[2],
                 logicalPortInfo->client.suppMacAddr.addr[3],
                 logicalPortInfo->client.suppMacAddr.addr[4],
                 logicalPortInfo->client.suppMacAddr.addr[5]);
  SYSAPI_PRINTF (
                 "suppMacAddr               = %s\r\n", buf);

  SYSAPI_PRINTF (
                 "reAuthenticating          = %d",
                 logicalPortInfo->client.reAuthenticating);
  if (logicalPortInfo->client.reAuthenticating ==  TRUE)
  {
    SYSAPI_PRINTF (" True\r\n");
  }
  else if (logicalPortInfo->client.reAuthenticating ==  FALSE)
  {
    SYSAPI_PRINTF (" False\r\n");
  }
  else
  {
    SYSAPI_PRINTF (" Unknown\r\n");
  }

  SYSAPI_PRINTF (
                 "Reauthentication count    = %d\r\n",
                 logicalPortInfo->client.reAuthCount);

  SYSAPI_PRINTF (
                 "suppRestarting            = %d",
                 logicalPortInfo->client.suppRestarting);
  if (logicalPortInfo->client.suppRestarting ==  TRUE)
  {
    SYSAPI_PRINTF (" True\r\n");
  }
  else if (logicalPortInfo->client.suppRestarting ==  FALSE)
  {
    SYSAPI_PRINTF (" False\r\n");
  }
  else
  {
    SYSAPI_PRINTF (" Unknown\r\n");
  }

  SYSAPI_PRINTF (
                 "authMethod                = %d",
                 logicalPortInfo->client.authMethod);
  switch (logicalPortInfo->client.authMethod)
  {
  case  AUTH_METHOD_UNDEFINED:
    {
      SYSAPI_PRINTF (" Undefined\r\n");
    }
    break;
  case  AUTH_METHOD_LOCAL:
    {
      SYSAPI_PRINTF (" Local\r\n");
    }
    break;
  case  AUTH_METHOD_RADIUS:
    {
      SYSAPI_PRINTF (" Radius\r\n");
    }
    break;
  case  AUTH_METHOD_REJECT:
    {
      SYSAPI_PRINTF (" Reject\r\n");
    }
    break;
  default:
    {
      SYSAPI_PRINTF (" Unknown\r\n");
    }
    break;
  }

  SYSAPI_PRINTF (
                 "Vlan type %s vlan Id   = %d\r\n",
                 authmgrVlanTypeStringGet (logicalPortInfo->client.vlanType),
                 logicalPortInfo->client.vlanId);

  SYSAPI_PRINTF (
                 "Client Session Timeout         = %d\r\n",
                 logicalPortInfo->client.clientTimeout);

  SYSAPI_PRINTF (
                 "Blocked Vlan Id                = %d\r\n",
                 logicalPortInfo->client.blockVlanId);

  memset(buf, 0, sizeof(buf));
  memset(zeroV6, 0, 16);

  authmgrLogicalPortInfoGiveLock ();
}

/*********************************************************************
* @purpose  Display the status info for the specified port
*
* @param    intIfNum @b{(input)) internal interface number
*
* @returns  void
*
* @comments devshell command
*
* @end
*********************************************************************/
void authmgrDebugPortInfoShow (uint32 intIfNum)
{
  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    SYSAPI_PRINTF (
                   "intIfNum is not a valid authmgr interface(%d)\r\n",
                   intIfNum);
    return;
  }

  SYSAPI_PRINTF (
                 "AUTHMGR Status Info for port %d:\r\n", intIfNum);
  SYSAPI_PRINTF (
                 "------------------------------\r\n");

  SYSAPI_PRINTF (
                 "initialize                = %d",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].initialize);
  if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].initialize ==  TRUE)
  {
    SYSAPI_PRINTF (" True\r\n");
  }
  else if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].initialize ==
            FALSE)
  {
    SYSAPI_PRINTF (" False\r\n");
  }
  else
  {
    SYSAPI_PRINTF (" Unknown\r\n");
  }

  SYSAPI_PRINTF (
                 "portControlMode           = %d",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].
                 portControlMode);
  switch (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode)
  {
  case  AUTHMGR_PORT_FORCE_UNAUTHORIZED:
    {
      SYSAPI_PRINTF (
                     " Force Unauthorized\r\n");
    }
    break;
  case  AUTHMGR_PORT_AUTO:
    {
      SYSAPI_PRINTF (" Auto\r\n");
    }
    break;
  case  AUTHMGR_PORT_FORCE_AUTHORIZED:
    {
      SYSAPI_PRINTF (
                     " Force Authorized\r\n");
    }
    break;
  default:
    {
      SYSAPI_PRINTF (" Unknown\r\n");
    }
    break;
  }

  SYSAPI_PRINTF (
                 "hostMode           = %s\r\n",
                 authmgrHostModeStringGet (authmgrCB->globalInfo->
                                           authmgrPortInfo[intIfNum].hostMode));

  SYSAPI_PRINTF (
                 "portEnabled               = %d",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled);
  if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled ==  TRUE)
  {
    SYSAPI_PRINTF (" True\r\n");
  }
  else if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled ==
            FALSE)
  {
    SYSAPI_PRINTF (" False\r\n");
  }
  else
  {
    SYSAPI_PRINTF (" Unknown\r\n");
  }

  SYSAPI_PRINTF (
                 "portStatus                = %d",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portStatus);
  if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portStatus ==
       AUTHMGR_PORT_STATUS_AUTHORIZED)
  {
    SYSAPI_PRINTF (" Authorized\r\n");
  }
  else if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portStatus ==
            AUTHMGR_PORT_STATUS_UNAUTHORIZED)
  {
    SYSAPI_PRINTF (" Unauthorized\r\n");
  }
  else
  {
    SYSAPI_PRINTF (" Unknown\r\n");
  }

  SYSAPI_PRINTF (
                 "nimStatus                = %d \n",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].nimStatus);

  SYSAPI_PRINTF (
                 "quietPeriod               = %d\r\n",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].quietPeriod);

  SYSAPI_PRINTF (
                 "reAuthPeriod              = %d\r\n",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthPeriod);

  SYSAPI_PRINTF (
                 "reAuthEnabled             = %d",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].
                 reAuthEnabled);
  if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthEnabled ==  TRUE)
  {
    SYSAPI_PRINTF (" True\r\n");
  }
  else if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthEnabled ==
            FALSE)
  {
    SYSAPI_PRINTF (" False\r\n");
  }
  else
  {
    SYSAPI_PRINTF (" Unknown\r\n");
  }


  SYSAPI_PRINTF (
                 "Number of Authorizations  = %d\r\n",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount);

  SYSAPI_PRINTF (
                 "Auth Fail retry count = %d\r\n",
                 authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authFailRetryMaxCount);
}

/*********************************************************************
* @purpose  Display the status for the specified port
*
* @param    intIfNum @b{(input)) internal interface number
*
* @returns  void
*
* @comments devshell command
*
* @end
*********************************************************************/
void authmgrDebugPortStatsShow (uint32 intIfNum)
{
  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    SYSAPI_PRINTF (
                   "intIfNum is not a valid interface(%d)\r\n", intIfNum);
    return;
  }

  SYSAPI_PRINTF (
                 "AUTHMGR Stats for port %d:\r\n", intIfNum);
  SYSAPI_PRINTF (
                 "-----------------------\r\n");

  SYSAPI_PRINTF ("\r\n");

  SYSAPI_PRINTF (
                 "AUTHMGR Debug Stats for port %d:\r\n", intIfNum);

  SYSAPI_PRINTF (
                 "-----------------------------\r\n");
  SYSAPI_PRINTF (
                 "dot1x authEntersAuthenticating                 = %d\r\n",
                 authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.
                 authEntersAuthenticating);
  SYSAPI_PRINTF (
                 "dot1x authAuthSuccess       = %d\r\n",
                 authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.
                 authSuccess);
  SYSAPI_PRINTF (
                 "dot1x authFailure      = %d\r\n",
                 authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.
                 authFailure);
  SYSAPI_PRINTF (
                 "auth authTimeout          = %d\r\n",
                 authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.
                 authTimeout);
}

/*********************************************************************
* @purpose  Set the authmgr log trace mode
*
* @param    mode     @b{(input)) log trace mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrDebugLogTraceModeSet (uint32 mode)
{
  /* Validate input parm */
  if (mode !=  ENABLE && mode !=  DISABLE)
  {
    return  FAILURE;
  }

  /* If not changing mode, just return success */
  if (mode == authmgrCB->globalInfo->authmgrCfg->authmgrLogTraceMode)
  {
    return  SUCCESS;
  }

  authmgrCB->globalInfo->authmgrCfg->authmgrLogTraceMode = mode;

  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Return the authmgr log trace mode
*
* @param    mode     @b{(input)) log trace mode
*
* @returns   SUCCESS
*
* @comments none
*
* @end
*********************************************************************/
uint32 authmgrDebugLogTraceModeGet ()
{
  return authmgrCB->globalInfo->authmgrCfg->authmgrLogTraceMode;
}

/*********************************************************************
* @purpose Trace authmgr  events
*
* @param   0 - disable tracing
*          1 - enable port status events
*          2 - enable port events tracing
*
*
* @returns void
*
* @notes
*
* @end
*
*********************************************************************/
void authmgrDebugTraceEvent (uint32 debug, uint32 intfNum)
{
  authmgrDebugTraceFlag = debug;
  authmgrDebugTraceIntf = intfNum;
}

/*********************************************************************
* @purpose show authmgr trace events help
*
* @param
*
*
* @returns void
*
* @notes
*
* @end
*
*********************************************************************/
void authmgrDebugTraceEventHelp ()
{
  SYSAPI_PRINTF (
                 "\n Use authmgrDebugTraceEvent(<debug>,<interface Number>) to trace various events");
  SYSAPI_PRINTF (
                 "\n Specify internal interface number to trace events for specific interface or 0 for all interfaces.");

  SYSAPI_PRINTF ("\n Trace Event Flags");
  SYSAPI_PRINTF ("\n-------------------");

  SYSAPI_PRINTF (
                 "\n Flag                            Description                                            Value");
  SYSAPI_PRINTF (
                 "\n ------------------------------- -----------------------------------------------------  -------");
  SYSAPI_PRINTF (
                 "\n AUTHMGR_TRACE_PORT_STATUS         Traces port authorization events                        %u",
                 AUTHMGR_TRACE_PORT_STATUS);
  SYSAPI_PRINTF (
                 "\n AUTHMGR_TRACE_EVENTS              Traces callback events                                  %u",
                 AUTHMGR_TRACE_EVENTS);
  SYSAPI_PRINTF (
                 "\n AUTHMGR_TRACE_API_CALLS           Traces authmgr send packet events                         %u",
                 AUTHMGR_TRACE_API_CALLS);
  SYSAPI_PRINTF (
                 "\n AUTHMGR_TRACE_FSM_EVENTS          Traces state machine events                             %u",
                 AUTHMGR_TRACE_FSM_EVENTS);
  SYSAPI_PRINTF (
                 "\n AUTHMGR_TRACE_FAILURE             Traces failure events such as authentication failure    %u",
                 AUTHMGR_TRACE_FAILURE);
  SYSAPI_PRINTF (
                 "\n AUTHMGR_TRACE_RADIUS              Traces RADIUS related events                            %u",
                 AUTHMGR_TRACE_RADIUS);
  SYSAPI_PRINTF (
                 "\n AUTHMGR_TRACE_TIMER               Traces Authmgr Timer Events                               %u",
                 AUTHMGR_TRACE_TIMER);
  SYSAPI_PRINTF (
                 "\n AUTHMGR_TRACE_MAC_ADDR_DB         Traces Mac Address Database related events              %u",
                 AUTHMGR_TRACE_MAC_ADDR_DB);
}

/*********************************************************************
* @purpose  Return Logical port Debug info
*
* @param    intIfNum @b{(input)) interface number
* @param    lIntIfNum @b{(input)) Logical internal interface number
* @param    debugInfo @b{(output)) debug info
*
* @returns   SUCCESS or  FAILURE
*
* @comments 
*
* @end
*********************************************************************/
RC_t authmgrDebugLogicalPortInfoNextGet (uint32 intIfNum, uint32 *lIntIfNum, 
                                            authmgrLogicalPortDebugInfo_t *debugInfo)
{
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;
   AUTHMGR_PORT_CONTROL_t portControl =  AUTHMGR_PORT_CONTROL_INVALID;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  portControl = pCfg->portControlMode;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);

  if ( AUTHMGR_PORT_AUTO != portControl)
  {
    return  FAILURE;
  }

  memset(debugInfo, 0, sizeof(*debugInfo));

  authmgrLogicalPortInfoTakeLock ();

  if ((logicalPortInfo = authmgrLogicalPortInfoGetNextNode (intIfNum,
                                                            lIntIfNum)) !=  NULLPTR)
  {
    debugInfo->key = logicalPortInfo->key;
    memcpy(&debugInfo->client, &logicalPortInfo->client, sizeof(debugInfo->client));
    memcpy(&debugInfo->protocol, &logicalPortInfo->protocol, sizeof(debugInfo->protocol));
  }
  else
  {
    rc =  FAILURE;
  }

  authmgrLogicalPortInfoGiveLock ();
  return rc;
}

/*********************************************************************
* @purpose  Display the status info for the specified port
*
* @param    lIntIfNum @b{(input)) Logical internal interface number
*
* @returns  void
*
* @comments devshell command
*
* @end
*********************************************************************/
void authmgrDebugLogicalPortInfoShow (uint32 intIfNum, uint32 lIntIfNum)
{
  authmgrLogicalPortInfo_t *logicalPortInfo;
  uint32 physPort = 0, lPort = 0, type = 0;
   BOOL all =  FALSE;

  if (lIntIfNum == 0)
  {
    all =  TRUE;
    logicalPortInfo = authmgrLogicalPortInfoFirstGet (intIfNum, &lIntIfNum);
  }
  else
  {
    authmgrLogicalPortInfoTakeLock ();
    logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  }

  if (logicalPortInfo ==  NULLPTR)
  {
    if (all ==  FALSE)
    {
      authmgrLogicalPortInfoGiveLock ();
    }
    printf ("Cannot get logical port info for this log interface %d \n",
            lIntIfNum);
    return;
  }

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (authmgrIsValidIntf (physPort) !=  TRUE)
  {
    SYSAPI_PRINTF (
                   "intIfNum is not a valid authmgr interface(%d)\r\n",
                   physPort);
    if (all ==  FALSE)
    {
      authmgrLogicalPortInfoGiveLock ();
    }
    return;
  }

  if (intIfNum != physPort)
  {
    sysapiPrintf ("Error! LogicalPort[%d] not belongs to Port[%d] \n\r",
                  logicalPortInfo->key.keyNum, intIfNum);
    if (all ==  FALSE)
    {
      authmgrLogicalPortInfoGiveLock ();
    }
    return;
  }

  SYSAPI_PRINTF (
                 "Port Control Mode         = %s\n\n\r",
                 (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].
                  portControlMode ==
                   AUTHMGR_PORT_AUTO) ? "Auto" : "non-auto Based");

  SYSAPI_PRINTF (
                 "host Mode         = %s\n\n\r",
                 authmgrHostModeStringGet (authmgrCB->globalInfo->
                                           authmgrPortInfo[intIfNum].hostMode));
  while (lIntIfNum != 0)
  {
    if (all ==  FALSE)
    {
      authmgrLogicalPortInfoGiveLock ();
    }

    authmgrDebugPortMacInfoShow (lIntIfNum);

    if (all ==  TRUE)
    {
      logicalPortInfo =
        authmgrLogicalPortInfoGetNextNode (intIfNum, &lIntIfNum);
      if (logicalPortInfo ==  NULLPTR)
      {
        lIntIfNum = 0;
      }
    }
    else
    {
      lIntIfNum = 0;
    }
  }                             /*While */
}

/*********************************************************************
* @purpose get the hostmode string from hostmode
*
* @param hostMode -- auth mgr host modes
* @return hostmode string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrHostModeStringGet ( AUTHMGR_HOST_CONTROL_t hostMode)
{
  switch (hostMode)
  {
  case  AUTHMGR_SINGLE_AUTH_MODE:
    return "AUTHMGR_SINGLE_AUTH_MODE";
  case  AUTHMGR_MULTI_HOST_MODE:
    return "AUTHMGR_MULTI_HOST_MODE";
  case  AUTHMGR_MULTI_AUTH_MODE:
    return "AUTHMGR_MULTI_AUTH_MODE";
  default:
    return "Unknown host mode";
  }
}

/*********************************************************************
* @purpose get the auth mgr port status string
*
* @param status -- auth mgr port status
* @return port status string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrAuthStatusStringGet ( AUTHMGR_PORT_STATUS_t status)
{
  switch (status)
  {
  case  AUTHMGR_PORT_STATUS_AUTHORIZED:
    return " AUTHMGR_PORT_STATUS_AUTHORIZED";
  case  AUTHMGR_PORT_STATUS_UNAUTHORIZED:
    return " AUTHMGR_PORT_STATUS_UNAUTHORIZED";
  case  AUTHMGR_PORT_STATUS_NA:
    return " AUTHMGR_PORT_STATUS_NA";
  default:
    return "Unknown status";
  }
}

/*********************************************************************
* @purpose get the client allocated node type string
*
* @param type client allocation node type
* @return node type string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrNodeTypeStringGet (authmgrNodeType_t type)
{
  switch (type)
  {
  case AUTHMGR_PHYSICAL:
    return "AUTHMGR_PHYSICAL";
  case AUTHMGR_LOGICAL:
    return "AUTHMGR_LOGICAL";
  default:
    return "Undefined";
  }
}

/*********************************************************************
* @purpose get the timer type string
*
* @param type -- auth mgr timer type
* @return timer type string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrTimerTypeStringGet (authmgrTimerType_t type)
{
  switch (type)
  {
  case AUTHMGR_QWHILE:
    return "AUTHMGR_QWHILE";
  case AUTHMGR_REAUTH_WHEN:
    return "AUTHMGR_REAUTH_WHEN";
  case AUTHMGR_METHOD_NO_RESP_TMR:
    return "AUTHMGR_METHOD_NO_RESP_TMR";
  default:
    return "Undefined";
  }
}

/*********************************************************************
* @purpose get the vlan type string
*
* @param type -- vlan type
* @return vlan string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrVlanTypeStringGet (authmgrVlanType_t type)
{
  switch (type)
  {
  case AUTHMGR_VLAN_RADIUS:
    return "RADIUS";
  case AUTHMGR_VLAN_BLOCKED:
    return "BLOCKED";
  default:
    return "Undefined";
  }
}

/*********************************************************************
* @purpose get the client type string
*
* @param type -- auth mgr client type
* @return client type string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrClientTypeStringGet (authmgrClientType_t type)
{
  switch (type)
  {
  case AUTHMGR_CLIENT_AWARE:
    return "AUTHMGR_CLIENT_AWARE";
  case AUTHMGR_CLIENT_UNAWARE:
    return "AUTHMGR_CLIENT_UNAWARE";
  case AUTHMGR_CLIENT_MAB:
    return "AUTHMGR_CLIENT_MAB";
  default:
    return "Undefined";
  }
}

/*********************************************************************
* @purpose get the authentication manager state string
*
* @param state authentication manager SM state
* @return state string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrAuthStateStringGet (AUTHMGR_STATES_t state)
{
  switch (state)
  {
  case AUTHMGR_INITIALIZE:
    return "AUTHMGR_INITIALIZE";
  case AUTHMGR_AUTHENTICATING:
    return "AUTHMGR_AUTHENTICATING";
  case AUTHMGR_AUTHENTICATED:
    return "AUTHMGR_AUTHENTICATED";
  case AUTHMGR_UNAUTHENTICATED:
    return "AUTHMGR_UNAUTHENTICATED";
  case AUTHMGR_HELD:
    return "AUTHMGR_HELD";
  default:
    return "Undefined";
  }
}

/*********************************************************************
* @purpose get the authentication method string
*
* @param method -- authentication method
* @return method string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrMethodStringGet ( AUTHMGR_METHOD_t method)
{
  switch (method)
  {
  case  AUTHMGR_METHOD_NONE:
    return " AUTHMGR_METHOD_NONE";
  case  AUTHMGR_METHOD_8021X:
    return " AUTHMGR_METHOD_8021X";
  case  AUTHMGR_METHOD_MAB:
    return " AUTHMGR_METHOD_MAB";
  default:
    return "Undefined";
  }
}

/*********************************************************************
* @purpose get the client auth response event string
*
* @param status client response event
* @return event string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrMethodStatusStringGet ( AUTHMGR_STATUS_t status)
{
  switch (status)
  {
  case  AUTHMGR_NEW_CLIENT:
    return " AUTHMGR_NEW_CLIENT";
  case  AUTHMGR_AUTH_FAIL:
    return " AUTHMGR_AUTH_FAIL";
  case  AUTHMGR_AUTH_SUCCESS:
    return " AUTHMGR_AUTH_SUCCESS";
  case  AUTHMGR_AUTH_TIMEOUT:
    return " AUTHMGR_AUTH_TIMEOUT";
  case  AUTHMGR_AUTH_SERVER_COMM_FAILURE:
    return " AUTHMGR_AUTH_SERVER_COMM_FAILURE";
  case  AUTHMGR_METHOD_CHANGE:
    return " AUTHMGR_METHOD_CHANGE";
  case  AUTHMGR_CLIENT_DISCONNECTED:
    return " AUTHMGR_CLIENT_DISCONNECTED";
  default:
    return "Undefined";
  }
}

/*********************************************************************
* @purpose get the state machine event string
*
* @param event authmgr state machine event
* @return state machine event string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrSmEventStringGet (authmgrSmEvents_t event)
{
  switch (event)
  {
  case authmgrInitialize:
    return "authmgrInitialize";
  case authmgrStartAuthenticate:
    return "authmgrStartAuthenticate";
  case authmgrAuthSuccess:
    return "authmgrAuthSuccess";
  case authmgrNotAuthSuccessNoNextMethod:
    return "authmgrNotAuthSuccessNoNextMethod";
  case authmgrNotAuthSuccessNextMethod:
    return "authmgrNotAuthSuccessNextMethod";
  case authmgrHeldTimerEqualsZero:
    return "authmgrHeldTimerEqualsZero";
  case authmgrStopAuthenticate:
    return "authmgrStopAuthenticate";
  case authmgrHigherAuthMethodAdded:
    return "authmgrHigherAuthMethodAdded";
  case authmgrReauthenticate:
    return "authmgrReauthenticate";
  case authmgrAuthFail:
    return "authmgrAuthFail";
  case authmgrAuthenticatedRxEapolStart:
    return "authenticatedRcvdEapolStart";
  case authmgrAbortAndRestartAuth:
    return "authmgrAbortAndRestartAuth";
  default:
    return "Undefined";
  }
}

/*********************************************************************
* @purpose get the authentication list type string
*
* @param status list type
* @return string for the list type
*
* @comments
*
* @end
*********************************************************************/
char *authmgrListTypeStringGet ( AUTHMGR_METHOD_TYPE_t status)
{
  switch (status)
  {
  case  AUTHMGR_TYPE_ORDER:
    return "Order";
  case  AUTHMGR_TYPE_PRIORITY:
    return "Priority";
  default:
    return "Undefined";
  }
}

/*********************************************************************
* @purpose get the authentication Back end method string
*
* @param authMethod BAM method
* @return method string
*
* @comments
*
* @end
*********************************************************************/
char *authmgrAuthMethodStringGet (uint32 authMethod)
{
  switch (authMethod)
  {
  case  AUTH_METHOD_UNDEFINED:
    return " AUTH_METHOD_UNDEFINED";
  case  AUTH_METHOD_RADIUS:
    return " AUTH_METHOD_RADIUS";
  case  AUTH_METHOD_LOCAL:
    return " AUTH_METHOD_LOCAL";
  case  AUTH_METHOD_REJECT:
    return " AUTH_METHOD_REJECT";
  default:
    return "Unknown";
  }
}

/*********************************************************************
* @purpose function to print the supp mac address
*
* @param suppMacAddr -- mac address
*
* @comments
*
* @end
*********************************************************************/
void authmgrSuppMacStringGet ( enetMacAddr_t * suppMacAddr)
{
   uchar8 buf[32];

  memset (buf, 0, sizeof (buf));
  osapiSnprintf (( char8 *) buf, sizeof (buf),
                 "%02X:%02X:%02X:%02X:%02X:%02X", suppMacAddr->addr[0],
                 suppMacAddr->addr[1], suppMacAddr->addr[2],
                 suppMacAddr->addr[3], suppMacAddr->addr[4],
                 suppMacAddr->addr[5]);
  sysapiPrintf ("%s\n", buf);
}

/*********************************************************************
* @purpose debug function to dump attr info
*
* @param attrInfo -- received client attr info
*
* @comments
*
* @end
*********************************************************************/
void authmgrAttrInfoDump (authmgrAuthAttributeInfo_t * attrInfo)
{
  if ( NULLPTR == attrInfo)
  {
    return;
  }

  sysapiPrintf ("serverState = %s\n", attrInfo->serverState);
  sysapiPrintf ("serverStateLen = %u\n", attrInfo->serverStateLen);

  sysapiPrintf ("serverClass = %s\n", attrInfo->serverClass);
  sysapiPrintf ("serverClassLen = %u\n", attrInfo->serverClassLen);
  sysapiPrintf ("sessionTimeout = %u\n", attrInfo->sessionTimeout);
  sysapiPrintf ("terminationAction = %u\n", attrInfo->terminationAction);
  sysapiPrintf ("accessLevel = %u\n", attrInfo->accessLevel);
  sysapiPrintf ("idFromServer = %u\n", attrInfo->idFromServer);
  sysapiPrintf ("vlanString = %s\n", attrInfo->vlanString);
  sysapiPrintf ("vlanId = %u\n", attrInfo->vlanId);
}

/*********************************************************************
* @purpose debug function to dump client info
*
* @param info -- client info of the logical port
*
* @comments
*
* @end
*********************************************************************/
void authmgrLogicalPortClientInfoDump (authmgrClientInfo_t * info)
{
  uint32 i = 0;

  if ( NULLPTR == info)
  {
    return;
  }
  sysapiPrintf ("clientType = %s\n",
                authmgrClientTypeStringGet (info->clientType));
  sysapiPrintf ("retryCount = %u\n", info->retryCount);
  sysapiPrintf ("reAuthCount = %u\n", info->reAuthCount);
  sysapiPrintf ("reAuthenticate = %u\n", info->reAuthenticate);
  sysapiPrintf ("currentMethod = %s\n",
                authmgrMethodStringGet (info->currentMethod));
  sysapiPrintf ("authenticatedMethod = %s\n",
                authmgrMethodStringGet (info->authenticatedMethod));
  sysapiPrintf ("Executed Methods \n");

  for (i = 0; i <  AUTHMGR_METHOD_LAST; i++)
  {
    sysapiPrintf ("%s  \n", authmgrMethodStringGet (info->executedMethod[i]));
  }
  sysapiPrintf ("\n");
  sysapiPrintf ("logicalPortStatus = %s\n",
                authmgrAuthStatusStringGet (info->logicalPortStatus));
  sysapiPrintf ("authmgrUserName = %s\n", info->authmgrUserName);
  sysapiPrintf ("authmgrUserNameLength = %u\n", info->authmgrUserNameLength);
  sysapiPrintf ("currentIdL = %u\n", info->currentIdL);
  sysapiPrintf ("supp mac addr ");
  authmgrSuppMacStringGet (&info->suppMacAddr);
  sysapiPrintf ("\n");
  sysapiPrintf ("vlanType = %s\n", authmgrVlanTypeStringGet (info->vlanType));
  sysapiPrintf ("vlanId = %u\n", (info->vlanId));
  sysapiPrintf ("blockvlanId = %u\n", (info->blockVlanId));

  sysapiPrintf ("suppRestarting = %u\n", (info->suppRestarting));
  sysapiPrintf ("authMethod = %s\n",
                authmgrAuthMethodStringGet (info->authMethod));
  sysapiPrintf ("sessionTime = %u\n", (info->sessionTime));
  sysapiPrintf ("clientTimeout = %u\n", (info->clientTimeout));
  sysapiPrintf ("sessionTimeout = %u\n", (info->sessionTimeout));
  sysapiPrintf ("terminationAction = %u\n", (info->terminationAction));
  sysapiPrintf ("\n");
}

/*********************************************************************
* @purpose function to get port from the key
*
* @param intIfNum -- interface number
* @param lIntIfNum -- key
* @return  SUCCCESS
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrLportPortGet (uint32 * intIfNum, uint32 * lIntIfNum)
{
  AUTHMGR_IF_NULLPTR_RETURN_LOG (intIfNum);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (lIntIfNum);
  AUTHMGR_PORT_GET (*intIfNum, *lIntIfNum);
  return  SUCCESS;
}

/*********************************************************************
* @purpose debug function for auth count
*
* @param intIfNum -- interface number
* @param flag --  TRUE incerment/ FALSE -- decrement
*
* @comments
*
* @end
*********************************************************************/
void authmgrAuthCountTest (uint32 intIfNum,  BOOL flag)
{
  sysapiPrintf ("IntIf Num  = %u, authCount %u \n", intIfNum,
                authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount);
  if (flag)
  {
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount++;
  }
  else
  {
    if (0 != authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount)
    {
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount--;
    }
  }
}

/*********************************************************************
* @purpose debug function to print User count on an interface
*
* @param intfIfNum
* 
*
* @comments
*
* @end
*********************************************************************/
void authmgrUserCountDump(uint32 intIfNum)
{
  sysapiPrintf ("Number of current users = %d\n\r", authmgrCB->globalInfo->authmgrPortInfo[intIfNum].numUsers);
  sysapiPrintf ("Max users allowed = %d\n\r", authmgrCB->globalInfo->authmgrPortInfo[intIfNum].maxUsers);
  return;
}

