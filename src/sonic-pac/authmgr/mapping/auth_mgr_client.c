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

#include "auth_mgr_client.h"
#include "auth_mgr_timer.h"
#include "auth_mgr_struct.h"
#include "pacoper_common.h"
#include "pac_cfg_authmgr.h"
#include "auth_mgr_vlan_db.h"
#include "simapi.h"
#include "osapi.h"

extern authmgrCB_t *authmgrCB;

/*********************************************************************
* @purpose  Set the authmgr physical port authorization status
*
* @param    intIfNum   @b{(input)) internal interface number
* @param    portStatus @b{(input)) port authorization status setting
*
* @returns   SUCCESS
* @returns   FAILURE
* @returns   ERROR
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrIhPhysicalPortStatusSet (uint32 intIfNum,
                                         AUTHMGR_PORT_STATUS_t portStatus)
{
   INTF_STATES_t state;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  authmgrPortCfg_t *pCfg;
   AUTHMGR_PORT_STATUS_t status;
 

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                       "%s:%d: Intf %d, portStatus %d\n",
                       __FUNCTION__, __LINE__, intIfNum, portStatus);

  /* Set the port status in the driver */
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) ==  FALSE)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Interface %s not authmgr configurable", authmgrIntfIfNameGet(intIfNum));
    return  FAILURE;
  }

  state = nimGetIntfState (intIfNum);
  if ((state !=  INTF_ATTACHED)
      && (state !=  INTF_ATTACHING) && (state !=  INTF_DETACHING))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                         "%s:%d: Intf %s, state %d\n",
                         __FUNCTION__, __LINE__, authmgrIntfIfNameGet(intIfNum), state);
  }

  /* set the port status */
  if ( DOT1X_PAE_PORT_NONE_CAPABLE == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities)
  {
    status = portStatus;
  }
  else if (( AUTHMGR_SINGLE_AUTH_MODE ==
       authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode)
      || ( AUTHMGR_MULTI_AUTH_MODE ==
          authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode))
  {
    status =  AUTHMGR_PORT_STATUS_UNAUTHORIZED;
  }
  else
  {
    status = portStatus;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                       "%s:Setting the port-%d  to %s\n",
                       __FUNCTION__, intIfNum,
                       ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
                        portStatus) ? "Authorize" : "Unauthorize");

  if (nimGetIntfName(intIfNum,  ALIASNAME, ifName) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_ERROR,
             "Unable to get aliasName for interface %s", authmgrIntfIfNameGet(intIfNum));
    return  FAILURE;
  }

  /* apply the violation policy */
  if ( SUCCESS != authmgrViolationPolicyApply (intIfNum))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, intIfNum,
                         "%s:Unable to apply port violation policy for port-%s\n",
                         __FUNCTION__, authmgrIntfIfNameGet(intIfNum));
    return  FAILURE;
  }

  /* set the learning status */
  if ( SUCCESS != authmgrPortLearningModify (intIfNum))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, intIfNum,
                         "%s:Unable to modify port learning for port-%s\n",
                         __FUNCTION__, authmgrIntfIfNameGet(intIfNum));
    return  FAILURE;
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose  application function to add the authenticated client 
*
* @param    physPort   @b{(input)}  internal interface number
* @param    macAddr    @b{(input)}  MAC address of authorized client
* @param    vlanId     @b{(input)}  set to non-zero value to assign this client
*                                   to a VLAN
* @param    pTLV       @b{(input)}  pass a non-NULL pointer to apply a policy
*                                   for this client
* @param    blockVlanId     @b{(input)} vlan id in which the client is received 
*
* @returns   SUCCESS
* @returns   FAILURE
* @returns   ERROR
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrAuthenticatedClientAdd (uint32 physPort,
                                        enetMacAddr_t macAddr,
                                        ushort16 vlanId,
                                        ushort16 blockVlanId)
{
  RC_t rc =  SUCCESS, rc1 =  SUCCESS;
   char8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                       "%s:%d:adding the client %s, vlan= %d to port %s \n",
                       __FUNCTION__, __LINE__,
                       AUTHMGR_PRINT_MAC_ADDR(macAddr.addr), vlanId,
                       authmgrIntfIfNameGet(physPort));

  if (nimGetIntfName (physPort,  ALIASNAME, ifName) !=  SUCCESS)
  {
     LOGF ( LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %s", authmgrIntfIfNameGet(physPort));
    rc1 =  FAILURE;
    goto fail;
  }

  if (pacCfgIntfClientAdd(ifName, macAddr.addr, vlanId) !=  TRUE)
  {
     LOGF ( LOG_SEVERITY_ERROR,
             "Unable to add client on port %s", ifName);
    rc1 =  FAILURE;
    goto fail;
  }

  if ( SUCCESS == rc)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                         "%s:%d Inserting the mac to fdb table as static entry \n",
                         __FUNCTION__, __LINE__);
    if (0 != blockVlanId)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
          "%s:%d Unblocking the Client %s with vlan id- %d \n",
          __FUNCTION__, __LINE__,
          AUTHMGR_PRINT_MAC_ADDR(macAddr.addr), blockVlanId);

      if (blockVlanId != vlanId)
      {
        if (pacCfgIntfClientUnblock(ifName, macAddr.addr, blockVlanId) !=  TRUE)
        {
           LOGF ( LOG_SEVERITY_ERROR,
                   "Unable to block port %s", ifName);
          rc1 =  FAILURE;
        }

fail:
        if ( SUCCESS != rc1)
        {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
              "%s:%d Client operation not successful \n",
              __FUNCTION__, __LINE__);
        }
      }
    }
  } 

  return rc;
}

/*********************************************************************
* @purpose  Set the authmgr physical port authorization status
*
* @param    intIfNum   @b{(input)}  internal interface number
* @param    macAddr    @b{(input)}  MAC address of authorized client
* @param    vlanId     @b{(input)}  set to non-zero value to assign this client
*                                   to a VLAN
*
* @returns   SUCCESS
* @returns   FAILURE
* @returns   ERROR
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrAuthenticatedClientDelete (uint32 physPort,
                                           enetMacAddr_t macAddr,
                                           ushort16 vlanId)
{
  RC_t rc =  SUCCESS;
   char8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                       "%s:%d:removing the client "
                       "Mac Addr: %s from port %s \n",
                       __FUNCTION__, __LINE__, AUTHMGR_PRINT_MAC_ADDR(macAddr.addr),
                       authmgrIntfIfNameGet(physPort));

  if (nimGetIntfName (physPort,  ALIASNAME, ifName) !=  SUCCESS)
  {
     LOGF ( LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %s", authmgrIntfIfNameGet(physPort));
    rc =  FAILURE;
    goto label;
  }

  if (pacCfgIntfClientRemove(ifName, macAddr.addr, vlanId) !=  TRUE)
  {
     LOGF ( LOG_SEVERITY_ERROR,
             "Unable to add client on port %s", ifName);
    rc =  FAILURE;
  }


label:
  if ( SUCCESS != rc)
  {
     LOGF ( LOG_SEVERITY_NOTICE,
             "Error in removing the client details from the driver."
             " Could not remove client details from the driver");
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                         "\n%s:%d Error in removing the client details from the driver\n",
                         __FUNCTION__, __LINE__);
  }

  return rc;
}

/*********************************************************************
* @purpose function to  cleanup the client hw params such as vlan and other settings
*
* @param    logicalPortInfo   @b{(input)) client logical port info structure 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientHwInfoCleanup (authmgrLogicalPortInfo_t * logicalPortInfo)
{
  RC_t rc =  FAILURE;
  authmgrPortCfg_t *pCfg;
   BOOL valid =  FALSE;
  authmgrVlanType_t vlanType = 0;
  uint32 vlanId = 0;
  uint32 physPort = 0, lPort = 0, type = 0;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (authmgrIntfIsConfigurable (physPort, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  if ( AUTHMGR_PORT_AUTO !=
      authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode)
  {
    return  SUCCESS;
  }

  /* get the host policy for the current host mode */
  if ( SUCCESS == authmgrStaticFdbEntryValidCheck (physPort, &valid))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                         "Initiating the HW Info cleanup for the client %d\n",
                         logicalPortInfo->key.keyNum);

    if (valid)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                           "trying to remove the static fdb entry for the client %d\n",
                           logicalPortInfo->key.keyNum);

      rc =  SUCCESS;
      if (logicalPortInfo->client.attrCreateMask & (1<<(AUTHMGR_HW_ATTR_STATIC_FDB)))
      {
        /* remove the CPU inserted client */
        rc = authmgrAuthenticatedClientDelete (physPort,
            logicalPortInfo->client.
            suppMacAddr,
            logicalPortInfo->client.vlanId);
        logicalPortInfo->client.attrCreateMask &= ~(1<<(AUTHMGR_HW_ATTR_STATIC_FDB));
      }

      if ( SUCCESS != rc)
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                             "%s:%d unable to delete client on "
                             "port %d logicalPort %d, type %d vlan %d \n",
                             __FUNCTION__, __LINE__, physPort, lPort,
                             type, logicalPortInfo->client.vlanId);

        return  FAILURE;
      }
    }
  }

  /* take a back up of vlan id and Vlan type */
  vlanType = logicalPortInfo->client.vlanType;
  vlanId = logicalPortInfo->client.vlanId;

  /* reset the data from the logical node temporarily */
  logicalPortInfo->client.vlanId = 0;
  logicalPortInfo->client.vlanType = AUTHMGR_VLAN_UNASSIGNED;

  /* check if no clients are on the same vlan */
  if ( SUCCESS != authmgrVlanPortDeletionValidate (physPort, vlanId))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
                         "%s:%d unable to clear vlan participation "
                         "port %d is having additional clients on vlan %d \n",
                         __FUNCTION__, __LINE__, physPort, vlanId);
    return  SUCCESS;
  }


    rc = authmgrClientVlanInfoReset (physPort, vlanId);

  if ( SUCCESS != rc)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                         "%s:%d unable to clear vlan participation "
                         "port %d from vlan %d \n",
                         __FUNCTION__, __LINE__, physPort, vlanId);

    /* put back the backed up info */
    logicalPortInfo->client.vlanType = vlanType;
    logicalPortInfo->client.vlanId = vlanId;
  }

  return rc;
}

/*********************************************************************
* @purpose function to  add cient hw params such as vlan and other settings
*
* @param    logicalPortInfo   @b{(input)) logical port info structure 
* @param    macAddr   @b{(input)) mac address 
* @param    vlanId   @b{(input)) vlanId 
* @param    policyIndex   @b{(input)) policyIndex 
* @param    blockVlanId   @b{(input)) vlan id on which client is received 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientHwInfoAdd (authmgrLogicalPortInfo_t * logicalPortInfo,
                                 enetMacAddr_t macAddr,
                                 ushort16 vlanId,
                                 ushort16 blockVlanId)
{
  RC_t rc =  FAILURE;
  authmgrPortCfg_t *pCfg;
   BOOL valid =  FALSE;
  uint32 physPort = 0, lPort = 0, type = 0, mask = 0;
   char8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (nimGetIntfName (physPort,  ALIASNAME, ifName) !=  SUCCESS)
  {
     LOGF ( LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %s", authmgrIntfIfNameGet(physPort));
    return  FAILURE;
  }

  if (authmgrIntfIsConfigurable (physPort, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  if ( AUTHMGR_PORT_AUTO !=
      authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode)
  {
    return  SUCCESS;
  }

  /* see if static fdb entry is allowed */
  if ( SUCCESS ==
      authmgrStaticFdbEntryValidCheck (physPort, &valid))
  {
    if (valid)
    {
      rc =  SUCCESS;
      if (!((1<<(AUTHMGR_HW_ATTR_STATIC_FDB) & logicalPortInfo->client.attrCreateMask)))
      {
        rc = authmgrAuthenticatedClientAdd (physPort,
            macAddr, vlanId, blockVlanId);
      }

      if ( SUCCESS != rc)
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                             "%s:%d unable to add client on "
                             "port %d logicalPort %d, type %d vlan %d \n",
                             __FUNCTION__, __LINE__, physPort, lPort,
                             type, vlanId);

        authmgrClientHwAddFailPostHwCleanup(logicalPortInfo, mask);
        return  FAILURE;
      }

      mask |= (1<<(AUTHMGR_HW_ATTR_STATIC_FDB));
      logicalPortInfo->client.attrCreateMask |= (1<<(AUTHMGR_HW_ATTR_STATIC_FDB));

    }
     else
     {
       /* un Block traffic from this client. */
       if ( TRUE == logicalPortInfo->client.dataBlocked)
       {
         if (0 != logicalPortInfo->client.blockVlanId)
         {
           AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
               "%s, %d, disabling the settings for logicalInterface %d to permit traffic\n",
               __func__, __LINE__, logicalPortInfo->key.keyNum);
           if (pacCfgIntfClientUnblock(ifName, logicalPortInfo->client.suppMacAddr.addr, logicalPortInfo->client.blockVlanId) !=  TRUE)
           {
              LOGF ( LOG_SEVERITY_ERROR,
                      "Unable to block port %s", ifName);
             return  FAILURE;
           }
           logicalPortInfo->client.dataBlocked =  FALSE;
         }
       }
     }

    /* make the vlan participation */

    if ( SUCCESS != authmgrClientVlanInfoSet (logicalPortInfo, vlanId))
    {
      if (0 == authmgrCB->processInfo.vlanId)
      {
        authmgrCB->processInfo.vlanId = vlanId;
      }
      authmgrClientHwAddFailPostHwCleanup(logicalPortInfo, mask);
      return  FAILURE;
    }

    return  SUCCESS;
  }

  return  FAILURE;
}

/*********************************************************************
* @purpose  Set the authmgr client info
*
* @param    logicalPortInfo   @b{(input)) logical interface structure 
* @param    client_info @b{(output)) client info
*
* @returns  void
*
* @comments none
*
* @end
*********************************************************************/
static
void authmgrClientInfoPopulate(authmgrLogicalPortInfo_t *logicalPortInfo,
                               pac_authenticated_clients_oper_table_t *client_info)
{
  uint32 physPort = 0, lPort = 0, type = 0;

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  memset(client_info, 0, sizeof(*client_info));

  client_info->currentIdL = logicalPortInfo->client.currentIdL;
  client_info->auth_status = logicalPortInfo->client.logicalPortStatus;
  client_info->authenticatedMethod = logicalPortInfo->client.authenticatedMethod;

  memcpy(&client_info->serverState, logicalPortInfo->client.serverState, 
         logicalPortInfo->client.serverStateLen);
  client_info->serverStateLen = logicalPortInfo->client.serverStateLen;

  memcpy(&client_info->serverClass, logicalPortInfo->client.serverClass, 
         logicalPortInfo->client.serverClassLen);
  client_info->serverClassLen = logicalPortInfo->client.serverClassLen;

  client_info->sessionTimeoutRcvdFromRadius = logicalPortInfo->client.sessionTimeout;
  if ( TRUE == authmgrCB->globalInfo->authmgrPortInfo[physPort].reAuthPeriodServer)
  {
    client_info->sessionTimeoutOper = logicalPortInfo->client.sessionTimeout;
  }
  else if ( TRUE == authmgrCB->globalInfo->authmgrPortInfo[physPort].reAuthEnabled)
  {
    client_info->sessionTimeoutOper = authmgrCB->globalInfo->authmgrPortInfo[physPort].reAuthPeriod;
  }
  else
  {
    client_info->sessionTimeoutOper = 0;
  }

  osapiStrncpySafe(client_info->userName, logicalPortInfo->client.authmgrUserName, strlen(logicalPortInfo->client.authmgrUserName)+1);

  client_info->userNameLen = logicalPortInfo->client.authmgrUserNameLength;

  client_info->terminationAction = logicalPortInfo->client.terminationAction;

  client_info->vlanType = logicalPortInfo->client.vlanType;

  client_info->vlanId = logicalPortInfo->client.vlanId;

  client_info->sessionTime = logicalPortInfo->client.sessionTime;

  client_info->lastAuthTime = logicalPortInfo->client.lastAuthTime;

  client_info->backend_auth_method = logicalPortInfo->client.authMethod;
}

/*********************************************************************
* @purpose  Set the authmgr global info
*
* @param    global_info @b{(output)) global info
*
* @returns  void
*
* @comments none
*
* @end
*********************************************************************/
static
void authmgrGlobalAuthInfoPopulate(pac_global_oper_table_t *global_info)
{
  memset(global_info, 0, sizeof(*global_info));
  return;
}

/*********************************************************************
* @purpose  Set the authmgr client authorization status
*
* @param    logicalPortInfo   @b{(input)) logical interface structure 
* @param    portStatus @b{(input)) port authorization status setting
*
* @returns   SUCCESS
* @returns   FAILURE
* @returns   ERROR
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrClientStatusSet (authmgrLogicalPortInfo_t * logicalPortInfo,
                                 AUTHMGR_PORT_STATUS_t portStatus)
{
  uint32 physPort = 0, lPort = 0, type = 0;
   AUTHMGR_PORT_STATUS_t currentStatus;
  pac_authenticated_clients_oper_table_t client_info;
  pac_global_oper_table_t global_info;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                       "%s:Setting the Logical port-%d to %s\n", __FUNCTION__,
                       logicalPortInfo->key.keyNum,
                       (portStatus ==
                         AUTHMGR_PORT_STATUS_AUTHORIZED) ? "Authorize" :
                       "Unauthorize");

  /* Verify port status parm value */
  if (portStatus !=  AUTHMGR_PORT_STATUS_AUTHORIZED
      && portStatus !=  AUTHMGR_PORT_STATUS_UNAUTHORIZED)
  {
    return  FAILURE;
  }

  /*If setting to the same value, just return success */
  if (portStatus == logicalPortInfo->client.logicalPortStatus)
  {
    /* check if the client is authenticated
       as part of re-auth */
    if (( AUTHMGR_PORT_STATUS_AUTHORIZED == portStatus) &&
        (logicalPortInfo->protocol.reauth))
    {
      if (logicalPortInfo->client.sessionTimeout != 0)
          logicalPortInfo->client.lastAuthTime = simSystemUpTimeGet ();
    }

    authmgrClientInfoPopulate(logicalPortInfo, &client_info);

    PacAuthClientOperTblSet(physPort, logicalPortInfo->client.suppMacAddr, &client_info); 

    authmgrGlobalAuthInfoPopulate(&global_info);

    PacGlobalOperTblSet(&global_info); 

    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                         "%s:%d Status already set \n", __FUNCTION__, __LINE__);
    return  SUCCESS;
  }

  if ((( AUTHMGR_PORT_FORCE_UNAUTHORIZED ==
        authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode)
       && ( AUTHMGR_PORT_STATUS_AUTHORIZED == portStatus))
      ||
      (( AUTHMGR_PORT_FORCE_AUTHORIZED ==
        authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode)
       && ( AUTHMGR_PORT_STATUS_UNAUTHORIZED == portStatus)))
  {
    /* this combination is not allowed.. So just a sanity check */
    return  FAILURE;
  }

  /* */

  currentStatus = logicalPortInfo->client.logicalPortStatus;
  logicalPortInfo->client.logicalPortStatus = portStatus;

  if ( AUTHMGR_PORT_STATUS_AUTHORIZED == portStatus)
  {
    /* set the port status to authorized */
    authmgrCB->globalInfo->authmgrPortInfo[physPort].authCount++;
    if (authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode !=
         AUTHMGR_PORT_FORCE_AUTHORIZED)
    {
      authmgrCtlResetLogicalPortSessionData (logicalPortInfo);

      authmgrClientInfoPopulate(logicalPortInfo, &client_info);

      PacAuthClientOperTblSet(physPort, logicalPortInfo->client.suppMacAddr, &client_info); 

      authmgrGlobalAuthInfoPopulate(&global_info);

      PacGlobalOperTblSet(&global_info); 

    }
  }
  else
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[physPort].authCount > 0)
    {
      if ( AUTHMGR_PORT_STATUS_AUTHORIZED == currentStatus)
      {
        authmgrCB->globalInfo->authmgrPortInfo[physPort].authCount--;
      }
      if (authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode !=
           AUTHMGR_PORT_FORCE_UNAUTHORIZED)
      {
        PacAuthClientOperTblDel(physPort, logicalPortInfo->client.suppMacAddr); 

        authmgrGlobalAuthInfoPopulate(&global_info);
        PacGlobalOperTblSet(&global_info); 
      }
    }
  }

  if (((0 == authmgrCB->globalInfo->authmgrPortInfo[physPort].authCount) &&
       ( AUTHMGR_PORT_STATUS_UNAUTHORIZED == portStatus)) ||
      ((1 == authmgrCB->globalInfo->authmgrPortInfo[physPort].authCount) &&
       ( AUTHMGR_PORT_STATUS_AUTHORIZED == portStatus)))

  {
    authmgrIhPhysicalPortStatusSet (physPort, portStatus);
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose function to  cleanup the client sw info
*
* @param    logicalPortInfo   @b{(input)) logical interface structure 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientSwInfoCleanup (authmgrLogicalPortInfo_t * logicalPortInfo)
{
  RC_t rc =  SUCCESS, rc1 =  SUCCESS, rc2 =  SUCCESS, rc3 =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  uint32 physPort = 0, lPort = 0, type = 0;
   enetMacAddr_t nullMacAddr;

  memset (&nullMacAddr.addr, 0,  ENET_MAC_ADDR_LEN);

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (authmgrIntfIsConfigurable (physPort, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  if (0 != logicalPortInfo->authmgrMethodNoRespTimer.handle.timer)
  {
    rc =
      authmgrTimerDestroy (authmgrCB->globalInfo->authmgrTimerCB,
                           logicalPortInfo, AUTHMGR_METHOD_NO_RESP_TMR);
  }

  if (0 != logicalPortInfo->authmgrTimer.handle.timer)
  {
    rc =
      authmgrTimerDestroy (authmgrCB->globalInfo->authmgrTimerCB,
                           logicalPortInfo,
                           logicalPortInfo->authmgrTimer.cxt.type);
  }

  /* send the accounting update */
  if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
      logicalPortInfo->client.logicalPortStatus)
  {
    /* notify the associated method to disconnected client */

    if ( NULLPTR !=
        authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
                                                authenticatedMethod].
        eventNotifyFn)
    {
      rc =
        authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
                                                authenticatedMethod].eventNotifyFn
        (physPort, authmgrClientDisconnect,
         &logicalPortInfo->client.suppMacAddr);
    }
  }
  else
  {
    /* 802.1X/MAB clients may be in the process of authenticating.
        de-authenticate 802.1X/MAB clients */

    if ( NULLPTR !=
        authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.currentMethod].
        eventNotifyFn)
    {
      rc =
        authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.currentMethod].eventNotifyFn
        (physPort, authmgrClientDisconnect,
         &logicalPortInfo->client.suppMacAddr);
    }
  }

  /* set the client to un-authorized */

  rc1 =
    authmgrClientStatusSet (logicalPortInfo,
                             AUTHMGR_PORT_STATUS_UNAUTHORIZED);

  if ( TRUE == logicalPortInfo->protocol.eapSuccess)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
                         "%s:Send EAP Success instead of Failure for data client on port [%s]\n\r",
                         __FUNCTION__, authmgrIntfIfNameGet(physPort));

    logicalPortInfo->protocol.eapSuccess =  FALSE;
    authmgrTxCannedSuccess (logicalPortInfo->key.keyNum, AUTHMGR_LOGICAL_PORT);
  }
  else
  {
    authmgrTxCannedFail (logicalPortInfo->key.keyNum, AUTHMGR_LOGICAL_PORT);
  }

  /*remove supplicant mac address from Mac address Database */
  /*input check */
  if (0 !=
      memcmp (logicalPortInfo->client.suppMacAddr.addr, nullMacAddr.addr,
               ENET_MAC_ADDR_LEN))
  {
    rc2 = authmgrMacAddrInfoRemove (&(logicalPortInfo->client.suppMacAddr));
  }

  if (authmgrCB->globalInfo->authmgrPortInfo[physPort].numUsers > 0)
  {
    authmgrCB->globalInfo->authmgrPortInfo[physPort].numUsers--;
  }

  if (authmgrCB->globalInfo->authmgrPortInfo[physPort].numUsers == 0)
  {
    authmgrIhPhysicalPortStatusSet(physPort,  AUTHMGR_PORT_STATUS_UNAUTHORIZED);
  }

  /* Deallocate memory for Clients */
  rc3 = authmgrLogicalPortInfoDeAlloc (logicalPortInfo);

  if (( SUCCESS != rc) ||
      ( SUCCESS != rc1) || ( SUCCESS != rc2) || ( SUCCESS != rc3))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                         "%s:Unable to clean up client sw info on port-%s\n",
                         __FUNCTION__, authmgrIntfIfNameGet(physPort));
    return  FAILURE;
  }
  else
  {
    return  SUCCESS;
  }
}

/*********************************************************************
* @purpose function to  cleanup the client
*
* @param    logicalPortInfo   @b{(input)) logical interface structure 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientInfoCleanup (authmgrLogicalPortInfo_t * logicalPortInfo)
{
  RC_t rc =  SUCCESS, rc1 =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  uint32 physPort = 0, lPort = 0, type = 0;
   enetMacAddr_t zeroMac;
   BOOL valid =  FALSE;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  nimGetIntfName(physPort,  ALIASNAME, ifName); 
  if (authmgrIntfIsConfigurable (physPort, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
                       "%s:Deleting client authenticated with method %d on Physical port-%s VLAN type %s \n",
                       __FUNCTION__, logicalPortInfo->client.authenticatedMethod, authmgrIntfIfNameGet(physPort),
                       authmgrVlanTypeStringGet(logicalPortInfo->client.vlanType));
   LOGF ( LOG_SEVERITY_NOTICE,
           "Client %s is getting disconnected on port (%s) with VLAN type %s.",
            AUTHMGR_PRINT_MAC_ADDR(logicalPortInfo->client.suppMacAddr.addr),
            ifName, authmgrVlanTypeStringGet (logicalPortInfo->client.vlanType));

  authmgrCB->oldInfo.vlanId = logicalPortInfo->client.vlanId;
   
  /* clean up the client info from hw */
  if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
      logicalPortInfo->client.logicalPortStatus)
  {
    rc = authmgrClientHwInfoCleanup (logicalPortInfo);
  }
  else
  {
    if (( SUCCESS == authmgrHostIsDynamicNodeAllocCheck
         (authmgrCB->globalInfo->authmgrPortInfo[physPort].hostMode, &valid)) &&
        ( TRUE == valid))
    {
      /* if exists unblock the client */
      memset (&zeroMac, 0, sizeof ( enetMacAddr_t));

      if ((0 != memcmp (zeroMac.addr, logicalPortInfo->client.suppMacAddr.addr,
                         ENET_MAC_ADDR_LEN)) &&
          (0 != logicalPortInfo->client.blockVlanId))
      {
        if (pacCfgIntfClientUnblock(ifName, logicalPortInfo->client.suppMacAddr.addr, 
                                    logicalPortInfo->client.blockVlanId) !=  TRUE)
        {
           LOGF ( LOG_SEVERITY_ERROR,
                   "Unable to block port %s", ifName);
          rc1 =  FAILURE;
        }

        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
          "%s, %d,Enabling the settings for logicalInterface %d to receive further packets to CPU\n",
          __func__, __LINE__, logicalPortInfo->key.keyNum);

        logicalPortInfo->client.dataBlocked =  FALSE;

        if ( SUCCESS != rc1)
        {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                               "%s:%d unable to delete client on "
                               "port %d logicalPort %d, type %d vlan %d \n",
                               __FUNCTION__, __LINE__, physPort, logicalPortInfo->key.keyNum,
                               type, logicalPortInfo->client.vlanId);
        }
      }
    }
    else if ( FALSE == valid)
    {
      /* check if any blocked un-auth clients 
         are present */
      if (0 != logicalPortInfo->client.blockVlanId)
      {
        /* if exists unblock the client */
        memset (&zeroMac, 0, sizeof ( enetMacAddr_t));

        if ((0 != memcmp (zeroMac.addr, logicalPortInfo->client.suppMacAddr.addr,
                 ENET_MAC_ADDR_LEN)) &&
            (0 != logicalPortInfo->client.blockVlanId))
        {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
          "%s, %d,Enabling the settings for logicalInterface %d to receive further packets to CPU\n",
          __func__, __LINE__, logicalPortInfo->key.keyNum);

          logicalPortInfo->client.dataBlocked =  FALSE;

          if ( SUCCESS != rc1)
          {
            AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                "%s:%d unable to delete client on "
                "port %s logicalPort %d, type %d vlan %d \n",
                __FUNCTION__, __LINE__, authmgrIntfIfNameGet(physPort), lPort,
                type, logicalPortInfo->client.vlanId);
          }
        }
      }
    }
  }

  if ( SUCCESS == rc)
  {
    rc = authmgrClientSwInfoCleanup (logicalPortInfo);
  }

  return rc;
}

/*********************************************************************
* @purpose function to  check and deAllocate the client
*
* @param    logicalPortInfo   @b{(input)) logical interface structure 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientDisconnectAction (authmgrLogicalPortInfo_t *
                                       logicalPortInfo)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  uint32 physPort = 0, lPort = 0, type = 0;
   BOOL valid =  FALSE;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (authmgrIntfIsConfigurable (physPort, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  /* check if the client can be de-allocated */

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                       "checking if logicalInterface %d can be disconnected\n",
                       logicalPortInfo->key.keyNum);

  if ( TRUE != logicalPortInfo->protocol.heldTimerExpired)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                         "%s:Held time not expired for client on port -%s\n",
                         __FUNCTION__, authmgrIntfIfNameGet(physPort));
    return  FAILURE;
  }

  logicalPortInfo->protocol.heldTimerExpired =  FALSE;

  if ( SUCCESS ==
      authmgrHostIsDynamicNodeAllocCheck (authmgrCB->globalInfo->
                                          authmgrPortInfo[physPort].hostMode,
                                          &valid))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                         "logicalInterface %d is getting disconnected\n",
                         logicalPortInfo->key.keyNum);
    rc = authmgrClientInfoCleanup (logicalPortInfo);
  }
  return rc;
}

/*********************************************************************
* @purpose utility function to check if the client client params needs cleanup 
*          before  adding new params
* @param logicalPortInfo
* @param vlanId           
* @param policyId 
* @return  SUCCESS/ FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientInfoCleanupCheck (authmgrClientInfo_t * src,
                                       authmgrClientInfo_t * dst)
{
  AUTHMGR_IF_NULLPTR_RETURN_LOG (src);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (dst);

  if ((src->vlanId == dst->vlanId) &&
      ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
       src->logicalPortStatus))
  {
    return  SUCCESS;
  }

  return  FAILURE;
}

/*********************************************************************
* @purpose function to perform client related actions if client auth is
*          failure or timeout.
*
* @param logicalPortInfo  -- client logical port structure
* @return  SUCCESS/ FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientFailTimeoutAction (authmgrLogicalPortInfo_t *
                                        logicalPortInfo)
{
  uint32 physPort = 0, vlanId = 0;
  RC_t rc =  SUCCESS;
  authmgrClientInfo_t client;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);
  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  memset(&client, 0 , sizeof(authmgrClientInfo_t));


  nimGetIntfName(physPort,  ALIASNAME, ifName);


  /* check for the allowed data clients */

  client = logicalPortInfo->client;
  client.vlanId = vlanId;

  if ( SUCCESS ==
      authmgrClientInfoCleanupCheck (&logicalPortInfo->client, &client))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                         "%s:Nothing changed for logicalPort num-%d\n ", 
                         __FUNCTION__, logicalPortInfo->key.keyNum);
    return  SUCCESS;
  }

  if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
      logicalPortInfo->client.logicalPortStatus)
  {
    /* clean up previous data */
    if ( SUCCESS != authmgrClientHwInfoCleanup (logicalPortInfo))
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                           "%s:Unable to cleanup client hw info logicalPort num-%d\n",
                           __FUNCTION__, logicalPortInfo->key.keyNum);
    }
  }

  return rc;
}

RC_t authmgrBlockFdbCleanup(authmgrLogicalPortInfo_t *logicalPortInfo)
{
  uint32 physPort = 0;
  RC_t rc =  SUCCESS;
   enetMacAddr_t zeroMac;
   char8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  AUTHMGR_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  AUTHMGR_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  /* if exists unblock the client */
  memset (&zeroMac, 0, sizeof ( enetMacAddr_t));

  if ((0 != memcmp (zeroMac.addr, logicalPortInfo->client.suppMacAddr.addr,
           ENET_MAC_ADDR_LEN)) &&
      (0 != logicalPortInfo->client.blockVlanId))
  {
    /* mac exists */
    if (nimGetIntfName (physPort,  ALIASNAME, ifName) !=  SUCCESS)
    {
       LOGF ( LOG_SEVERITY_ERROR,
               "Unable to get alias for intf %s", authmgrIntfIfNameGet(physPort));
      return  FAILURE;
    }

    if ((pacCfgIntfClientUnblock(ifName, logicalPortInfo->client.suppMacAddr.addr, 
                                      logicalPortInfo->client.blockVlanId)) !=  TRUE)
    {
       LOGF ( LOG_SEVERITY_ERROR,
               "Unable to block port %s", ifName);
      rc =  FAILURE;
    }
  }
  return rc;
}


RC_t authmgrStaticFdbCleanup(authmgrLogicalPortInfo_t *logicalPortInfo)
{
  uint32 physPort = 0;
  RC_t rc =  SUCCESS;

  AUTHMGR_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  AUTHMGR_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  if (logicalPortInfo->client.attrCreateMask & (1<<(AUTHMGR_HW_ATTR_STATIC_FDB)))
  {
    rc = authmgrAuthenticatedClientDelete (physPort,
        logicalPortInfo->client.
        suppMacAddr,
        authmgrCB->processInfo.vlanId);
  }
  logicalPortInfo->client.attrCreateMask &= ~(1<<(AUTHMGR_HW_ATTR_STATIC_FDB));

  return rc;

}

/*************************************************************************
* @purpose  utility function to get function map entry for the given event 
*
* @param    event  @b{(input)} event
* @param    elem @b{(input)}  Pointer to map entry
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*
* @end
*************************************************************************/
RC_t authmgrHwCleanupEventFnMapGet (uint32 event,
                                       authmgrHwCleanupEventMap_t * elem)
{
  uint32 i = 0;
  static authmgrHwCleanupEventMap_t authmgrHwCleanupEventMapTable[] = {
    {AUTHMGR_HW_ATTR_STATIC_FDB, authmgrStaticFdbCleanup},
    {AUTHMGR_HW_ATTR_BLOCK_FDB, authmgrBlockFdbCleanup},
    {AUTHMGR_HW_ATTR_PVID,  NULLPTR}
  };

  for (i = 0;
       i <
       (sizeof (authmgrHwCleanupEventMapTable) / sizeof (authmgrHwCleanupEventMap_t));
       i++)
  {
    if (event == authmgrHwCleanupEventMapTable[i].event)
    {
      *elem = authmgrHwCleanupEventMapTable[i];
      return  SUCCESS;
    }
  }

  return  FAILURE;
}

/*********************************************************************
* @purpose utility function to check if the client ckpt params modified 
* @param src
* @param dst
* @return  SUCCESS/ FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientHwAddFailPostHwCleanup (authmgrLogicalPortInfo_t *logicalPortInfo,
                                             uint32 mask)
{
  uint32 i = 0;
  authmgrHwCleanupEventMap_t entry;
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  for (i = 0; i < AUTHMGR_HW_ATTR_LAST; i++)
  {
    if (mask & (1<< i))
    {
      memset(&entry, 0, sizeof(authmgrHwCleanupEventMap_t));

      authmgrHwCleanupEventFnMapGet(i, &entry);

      if ( NULLPTR != entry.cleanupFn)
      {
        entry.cleanupFn(logicalPortInfo);
      }
    }
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose function to  cleanup the authenticated client sw info
*
* @param    logicalPortInfo   @b{(input)) logical interface structure 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAuthenticatedClientSwInfoCleanup (authmgrLogicalPortInfo_t * logicalPortInfo)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  uint32 physPort = 0, lPort = 0, type = 0;
   enetMacAddr_t macAddr;
   uchar8 userName[AUTHMGR_USER_NAME_LEN] = {0};
  uint32 userNameLength = 0;

  memset(&macAddr, 0, sizeof( enetMacAddr_t));

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (authmgrIntfIsConfigurable (physPort, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  if (0 != logicalPortInfo->authmgrTimer.handle.timer)
  {
    rc =
      authmgrTimerDestroy (authmgrCB->globalInfo->authmgrTimerCB,
          logicalPortInfo,
          logicalPortInfo->authmgrTimer.cxt.type);
  }

  /* send the accounting update */
  if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
      logicalPortInfo->client.logicalPortStatus)
  {
    /* notify the associated method to disconnected client */

    if ( NULLPTR !=
        authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
        authenticatedMethod].
        eventNotifyFn)
    {
      rc =
        authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
        authenticatedMethod].eventNotifyFn
        (physPort, authmgrClientDisconnect,
         &logicalPortInfo->client.suppMacAddr);
    }
  }

  /* set the client to un-authorized */

  rc =
    authmgrClientStatusSet (logicalPortInfo,
         AUTHMGR_PORT_STATUS_UNAUTHORIZED);


  if ( TRUE == logicalPortInfo->protocol.eapSuccess)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
        "%s:Send EAP Success instead of Failure for data client on port [%d]\n\r",
        __FUNCTION__, physPort);

    logicalPortInfo->protocol.eapSuccess =  FALSE;
    authmgrTxCannedSuccess (logicalPortInfo->key.keyNum, AUTHMGR_LOGICAL_PORT);
  }
  else
  {
    authmgrTxCannedFail (logicalPortInfo->key.keyNum, AUTHMGR_LOGICAL_PORT);
  }

  memcpy(&macAddr, &logicalPortInfo->client.suppMacAddr, sizeof( enetMacAddr_t));

  memset(&logicalPortInfo->protocol, 0, sizeof(authmgrProtocolInfo_t));
  memset(&logicalPortInfo->client, 0, sizeof(authmgrClientInfo_t));
  memcpy(&logicalPortInfo->client.suppMacAddr, &macAddr, sizeof( enetMacAddr_t));
  memcpy(logicalPortInfo->client.authmgrUserName, userName, AUTHMGR_USER_NAME_LEN);
  logicalPortInfo->client.authmgrUserNameLength = userNameLength;

  return  SUCCESS;
}

/*********************************************************************
* @purpose function to  cleanup the authenticated client
*
* @param    logicalPortInfo   @b{(input)) logical interface structure 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAuthenticatedClientCleanup (authmgrLogicalPortInfo_t * logicalPortInfo)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  uint32 physPort = 0, lPort = 0, type = 0;
   enetMacAddr_t clientMac;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  uint32 vlanId = 0;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  nimGetIntfName(physPort,  ALIASNAME, ifName); 
  if (authmgrIntfIsConfigurable (physPort, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  if ( AUTHMGR_PORT_STATUS_AUTHORIZED !=
      logicalPortInfo->client.logicalPortStatus)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
                       "%s:client is not in authorized state %d on Physical port-%s VLAN type %s \n",
                       __FUNCTION__, logicalPortInfo->client.logicalPortStatus, ifName,
                       authmgrVlanTypeStringGet(logicalPortInfo->client.vlanType));
    /* return */
    return  FAILURE;
  }
  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
                       "%s:Deleting client authenticated with method %d on Physical port-%s VLAN type %s \n",
                       __FUNCTION__, logicalPortInfo->client.authenticatedMethod, ifName,
                       authmgrVlanTypeStringGet(logicalPortInfo->client.vlanType));

  memset (&clientMac, 0, sizeof ( enetMacAddr_t));
  memcpy(&clientMac, &logicalPortInfo->client.suppMacAddr, sizeof ( enetMacAddr_t));

  authmgrCB->oldInfo.vlanId = logicalPortInfo->client.vlanId;
   
  /* clean up the client info from hw */
  if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
      logicalPortInfo->client.logicalPortStatus)
  {
    rc = authmgrClientHwInfoCleanup (logicalPortInfo);

    authmgrAuthenticatedClientSwInfoCleanup(logicalPortInfo);
    logicalPortInfo->client.blockVlanId = vlanId;
    logicalPortInfo->client.logicalPortStatus =  AUTHMGR_PORT_STATUS_UNAUTHORIZED;
  }

  return rc;
}

/*********************************************************************
* @purpose function to  check and cleanup authenticated client's params
*
* @param    logicalPortInfo   @b{(input)) logical interface structure 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAuthenticatedClientCleanupAction (authmgrLogicalPortInfo_t *
                                       logicalPortInfo)
{
  return authmgrAuthenticatedClientCleanup(logicalPortInfo);
}


