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
#include "auth_mgr_exports.h"
#include "auth_mgr_include.h"
#include "auth_mgr_client.h"
#include "auth_mgr_timer.h"
#include "auth_mgr_struct.h"
#include "auth_mgr_vlan_db.h"

extern authmgrCB_t *authmgrCB;
extern authMgrVlanDbData_t *authmgrVlanCfgDb; /* Vlan configured state cache. */ 


/*********************************************************************
* @purpose  check if the port participation can be added for a vlan
*
* @param    physPort    @b{(input))  Port
* @param    vlanId      @b{(input)) vlan Id
*
* @returns   SUCCESS/ FAILRE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanPortParticipationValidate (uint32 physPort, uint32 vlanId)
{
  uint32 cnt = 0, i = 0;
  BOOL valid = FALSE;
  authmgrLogicalPortInfo_t *logicalPortInfo = NULLPTR;

  if ( AUTHMGR_PORT_AUTO ==
      authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode)
  {
    i = AUTHMGR_LOGICAL_PORT_ITERATE;
    while (( NULLPTR !=
          (logicalPortInfo =
           authmgrLogicalPortInfoGetNextNode (physPort, &i))
          && (0 != logicalPortInfo->key.keyNum)))
    {
      if (AUTHMGR_PORT_STATUS_AUTHORIZED == logicalPortInfo->client.logicalPortStatus)
      {
        cnt++;
        if (TRUE == logicalPortInfo->protocol.reauth)
        {
          valid = ((cnt == 1) ? TRUE : FALSE);
        }
      }
    }

    if (valid)
      return SUCCESS;

    if ((0 != authmgrCB->globalInfo->authmgrPortInfo[physPort].authVlan) &&
        (vlanId != authmgrCB->globalInfo->authmgrPortInfo[physPort].authVlan))
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
          "Received Vlan %d is not same as Port %d auth VlanId %d. \n",
          vlanId, physPort, authmgrCB->globalInfo->authmgrPortInfo[physPort].authVlan);
      return  FAILURE;
    }
  }
  return  SUCCESS;
}
/*********************************************************************
* @purpose  Processes  Vlan PVID Change Notify event.
*
* @param    intIfNum   @b{(input)} internal interface number
* @param    vlanId     @b{(input)} VlanId (new PVID on the port)
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanPVIDChangeEventProcess (uint32 physPort, uint32 vlanId)
{
  RC_t rc =  FAILURE;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  uint32 i = 0;
   BOOL valid =  FALSE;
  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, physPort,
                       "%s:PVID for port -%s changed to Vlan %d\n",
                       __FUNCTION__, authmgrIntfIfNameGet(physPort), vlanId);
  if (authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode ==
       AUTHMGR_PORT_AUTO)
  {
    /* Nothing to be done as port pvid change is also triggered by a client
       being authorized
       on the port in other host modes, where authmgr can aquire the port.
       The Admin cannot change the operational pvid of the port
       as it is acquired by authmgr. */
    /* check if the host policy is valid for the current host mode */
    if (( SUCCESS ==
         authmgrHostIsDynamicNodeAllocCheck (authmgrCB->globalInfo->
                                             authmgrPortInfo[physPort].hostMode,
                                             &valid)) && ( TRUE == valid))
    {
      i = AUTHMGR_LOGICAL_PORT_ITERATE;
      while (( NULLPTR !=
              (logicalPortInfo =
               authmgrLogicalPortInfoGetNextNode (physPort, &i))
              && (0 != logicalPortInfo->key.keyNum)))
      {
        if ( AUTHMGR_PORT_STATUS_AUTHORIZED == logicalPortInfo->client.logicalPortStatus) 
        {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
                               "pvid for port %s changed. logicalPort %d was authenticated on previous "
                               "pvid %d. vlan type %s. Cleaning up the client \n",
                               authmgrIntfIfNameGet(physPort), i,logicalPortInfo->client.vlanId,
                               authmgrVlanTypeStringGet (logicalPortInfo->
                                                         client.vlanType));
          /* invoke the cleanup */
          rc = authmgrClientInfoCleanup (logicalPortInfo);
          if ( SUCCESS != rc)
          {
            AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                                 "client cleanup for logicalPort %d is NOT successful\n",
                                 i);
          }
        }
      }
    }
  }
  return rc;
}
/*********************************************************************
* @purpose  Set vlan port participation from AUTHMGR
*
* @param    physPort    @b{(input)) interface number
* @param    vlanId      @b{(input)) vlan Id
*
* @returns   SUCCESS/ FAILURE/ ALREADY_CONFIGURED
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanParticipationSet (uint32 physPort, uint32 vlanId,  BOOL isSelfAcquired)
{
 RC_t rc =  FAILURE;
  uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1] = {'\0'};

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
	  "\nSetting Vlan Membership for port -%s and Vlan %d\n",
	  authmgrIntfIfNameGet(physPort), vlanId);

  if (nimGetIntfName(physPort,  ALIASNAME, ifName) ==  SUCCESS)
  {
	if ( AUTHMGR_MULTI_HOST_MODE ==
		authmgrCB->globalInfo->authmgrPortInfo[physPort].hostMode)
	{
	  rc = pacCfgVlanMemberRemove(1, ifName);
	}
	rc = pacCfgVlanMemberAdd(vlanId, ifName,  DOT1Q_MEMBER_UNTAGGED);
  }
  else
  {
	LOGF( LOG_SEVERITY_ERROR,
		"Unable to get aliasName for interface %s",
		authmgrIntfIfNameGet(physPort));
  }
  if ( SUCCESS == rc)
  {
    authmgrCB->globalInfo->authmgrPortInfo[physPort].authVlan = vlanId;
	LOGF ( LOG_SEVERITY_DEBUG,
		"Auth Manager - set Vlan Membership (%d) for port (%s).", 
		vlanId, authmgrIntfIfNameGet(physPort)); 
  }
  else 
  {
	LOGF ( LOG_SEVERITY_DEBUG,
		"Auth Manager - unable to set Vlan Membership (%d) for port (%s).",
		vlanId, authmgrIntfIfNameGet(physPort)); 
	AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
		"\nUnable to Set Vlan Membership for port -%s and Vlan %d\n",
		authmgrIntfIfNameGet(physPort), vlanId);
  }
  return rc;
}
/*********************************************************************
* @purpose  Reset vlan participation for the  Interface
*
* @param    physPort    @b{(input)) interface number
* @param    vlanId      @b{(input)) vlan Id
*
* @returns   SUCCESS/ FAILRE
*
* @comments
*
* @end
*********************************************************************/
static RC_t authmgrVlanParticipationReset (uint32 physPort,
                                              uint32 vlanId)
{
  RC_t rc =  SUCCESS;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  if (nimGetIntfName(physPort,  ALIASNAME, ifName) ==  SUCCESS)
  {
    /* Invoke API to set port PVID */
    rc = pacCfgVlanMemberRemove(vlanId, ifName);
    authmgrCB->globalInfo->authmgrPortInfo[physPort].authVlan = 0;
  }
  else
  {
     LOGF( LOG_SEVERITY_ERROR,
            "Unable to get aliasName for interface %s",
            authmgrIntfIfNameGet(physPort));
  }

  return rc;
}

/*********************************************************************
* @purpose  check if the port participation can be removed for a vlan
*
* @param    physPort    @b{(input))  Port
* @param    vlanId      @b{(input)) vlan Id
*
* @returns   SUCCESS/ FAILRE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanPortDeletionValidate (uint32 physPort, uint32 vlanId)
{
  uint32 i = 0;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  if ( AUTHMGR_PORT_AUTO ==
      authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode)
  {
	i = AUTHMGR_LOGICAL_PORT_ITERATE;
	while (( NULLPTR !=
		  (logicalPortInfo =
		   authmgrLogicalPortInfoGetNextNode (physPort, &i))
		  && (0 != logicalPortInfo->key.keyNum)))
	{
	  if (vlanId == logicalPortInfo->client.vlanId)
	  {
		AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
			"logicalPort %d is still a member of vlanId %d. \n",
			i, logicalPortInfo->client.vlanId);
		return  FAILURE;
	  }
	}
  }
  return  SUCCESS;
}
/*********************************************************************
* @purpose  check if the port can be aquired by authmgr
*
* @param    physPort    @b{(input))  Port
* @param    vlanId      @b{(input)) vlan Id
*
* @returns   SUCCESS/ FAILRE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanPortAcquireCheck (uint32 physPort)
{
   BOOL valid =  FALSE;
  if ( AUTHMGR_PORT_AUTO ==
      authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode)
  {
    /* check if the host policy is valid for the current host mode */
    if (( SUCCESS ==
         authmgrHostIsDynamicNodeAllocCheck (authmgrCB->globalInfo->
                                             authmgrPortInfo[physPort].hostMode,
                                             &valid)) && ( FALSE == valid))
    {
      return  SUCCESS;
    }
  }
  return  FAILURE;
}
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
RC_t authmgrClientVlanInfoSet (authmgrLogicalPortInfo_t * logicalPortInfo,
                                  uint32 vlanId)
{
  uint32 physPort = 0;
  RC_t rc =  SUCCESS;
   uchar8        ifName[ NIM_IF_ALIAS_SIZE + 1];
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);
  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

    /*
       simply set the port membership */
    /* set the port as vlan member */
    rc = authmgrVlanParticipationSet (physPort, vlanId,  FALSE);
    if ((rc !=  SUCCESS) && (rc !=  ALREADY_CONFIGURED))
    {
       LOGF( LOG_SEVERITY_ERROR,
              "VLAN participation set unsuccessful for port %s vlan %d", 
              authmgrIntfIfNameGet(physPort), vlanId);
      return  FAILURE;
    }

    if ( AUTHMGR_MULTI_HOST_MODE ==
          authmgrCB->globalInfo->authmgrPortInfo[physPort].hostMode)
    {
      if (nimGetIntfName(physPort,  ALIASNAME, ifName) ==  SUCCESS)
      {
        /* Invoke API to set port PVID */
        rc = authmgrPortPvidSet(physPort, vlanId);
      }
      else
      {
         LOGF( LOG_SEVERITY_ERROR,
                "Unable to get aliasName for interface %s",
                authmgrIntfIfNameGet(physPort));
      }
    }

  if ( ALREADY_CONFIGURED == rc) 
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
        "\n%s:vlan %u membership already configured on port %s\n",
        __FUNCTION__, vlanId, authmgrIntfIfNameGet(physPort));
    rc =  SUCCESS;
  }
  return rc;
}
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
RC_t authmgrClientVlanInfoReset (uint32 physPort, uint32 vlanId)
{
   uchar8        ifName[ NIM_IF_ALIAS_SIZE + 1];
  RC_t rc =  SUCCESS;

  /* set the port as vlan member */
  if ( SUCCESS != authmgrVlanParticipationReset (physPort, vlanId))
  {
    return  FAILURE;
  }

  if ( AUTHMGR_MULTI_HOST_MODE ==
        authmgrCB->globalInfo->authmgrPortInfo[physPort].hostMode)
  {
    if (nimGetIntfName(physPort,  ALIASNAME, ifName) ==  SUCCESS)
    {
      /* Invoke API to set port PVID */
      rc = authmgrPortPvidSet(physPort, 1);
      if (rc !=  SUCCESS)
      {
         LOGF ( LOG_SEVERITY_ERROR,
                 "Failed to set PVID of interface %s to default VLAN ID.\r\n", ifName);
      }
    }
  }

  return  SUCCESS;
}
/*********************************************************************
* @purpose function to get the type of the vlan
*
* @param type vlan type
* @param entry associated entry to the given vlan type
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanTypeInfoGet (authmgrVlanType_t type,
                                authmgrVlanInfoEntry_t * entry)
{
  uint32 i = 0;
  static authmgrVlanInfoEntry_t authmgrVlanInfoEntryTable[] = {
    {AUTHMGR_VLAN_RADIUS,  AUTHMGR_RADIUS_ASSIGNED_VLAN},
    {AUTHMGR_VLAN_DEFAULT,  AUTHMGR_DEFAULT_ASSIGNED_VLAN},
    {AUTHMGR_VLAN_UNASSIGNED,  AUTHMGR_NOT_ASSIGNED}
  };
  for (i = 0;
       i <
       (sizeof (authmgrVlanInfoEntryTable) / sizeof (authmgrVlanInfoEntry_t));
       i++)
  {
    if (type == authmgrVlanInfoEntryTable[i].type)
    {
      *entry = authmgrVlanInfoEntryTable[i];
      return  SUCCESS;
    }
  }
  return  FAILURE;
}
/*********************************************************************
* @purpose  Apply authmgr vlan assignment to a specific logical interface
*
* @param    intIfNum   @b{(input)) internal interface number
* @param    vlanId     @b{(input)) vlan id
* @param    tagging    @b{(input)) Port VLAN tag mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanPortAddProcess (uint32 intIfNum, uint32 vlanId,
                                   dot1qTaggingMode_t tagging)
{
  authmgrPortCfg_t *pCfg;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  FAILURE;
  }

  /* Save notification in local cache */
  if ( SUCCESS != authmgrVlanPortAddLocalUpdate(vlanId, intIfNum, tagging))
  {
       LOGF ( LOG_SEVERITY_WARNING,
               "Unable to save VLAN port add notification for vlan %d intfIfNum %s",
               vlanId, authmgrIntfIfNameGet(intIfNum));
  }

  if ( AUTHMGR_PORT_AUTO !=
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode)
  {
    return  SUCCESS;
  }

  

  return  SUCCESS;
}
/*********************************************************************
* @purpose  function to process vlan add event 
*
* @param    vlanId     @b{(input)) vlan id
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments This call should happen only in mac-based mode.
*
* @end
*********************************************************************/
RC_t authmgrVlanAddProcess (uint32 vlanId)
{
  /* Save notification in local cache */
  if ( SUCCESS != authmgrVlanAddLocalUpdate(vlanId))
  {
       LOGF ( LOG_SEVERITY_WARNING,
               "Unable to save VLAN add notification for vlan %d.", vlanId);
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose  function to process vlan delete event 
*
* @param    vlanId     @b{(input)) vlan id
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments This call should happen only in mac-based mode.
*
* @end
*********************************************************************/
RC_t authmgrVlanDeleteProcess (uint32 vlanId)
{
  RC_t nimRc =  SUCCESS;
  uint32 intIfNum;
  dot1qTaggingMode_t tagging =  DOT1Q_MEMBER_UNTAGGED;
  
  nimRc = authmgrFirstValidIntfNumber (&intIfNum);
  while (nimRc ==  SUCCESS)
  {
    /* delete the clients on this port for the vlan */
    authmgrVlanPortDeleteProcess (intIfNum, vlanId, tagging);
    nimRc = authmgrNextValidIntf (intIfNum, &intIfNum);
  }

  /* Save notification in local cache */
  if ( SUCCESS != authmgrVlanDeleteLocalUpdate(vlanId))
  {
       LOGF ( LOG_SEVERITY_WARNING,
               "Unable to save VLAN delete notification for vlan %d.", vlanId);
  }

  return  SUCCESS;
}
/*********************************************
 * @purpose  function to process vlan port delete event 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 * @param    vlanId @b{(input)} vlan id
 * @param    tagging    @b{(input)) Port VLAN tag mode
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanPortDeleteProcess (uint32 intIfNum, uint32 vlanId,
                                      dot1qTaggingMode_t tagging)
{
  RC_t rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 lIntIfNum, tmpCount = 0;
  authmgrPortCfg_t *pCfg;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  FAILURE;
  }

  /* Save notification in local cache */
  if ( SUCCESS != authmgrVlanPortDeleteLocalUpdate(vlanId, intIfNum, tagging))
  {
       LOGF ( LOG_SEVERITY_WARNING,
               "Unable to save VLAN port delete notification for vlan %d on port %s",
               vlanId, authmgrIntfIfNameGet(intIfNum));
  }

  if ( AUTHMGR_PORT_AUTO !=
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode)
  {
    return  SUCCESS;
  }

  if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode ==
       AUTHMGR_PORT_AUTO)
  {
    lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
    while ((logicalPortInfo =
            authmgrLogicalPortInfoGetNextNode (intIfNum,
                                               &lIntIfNum)) !=  NULLPTR)
    {
      if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
          logicalPortInfo->client.logicalPortStatus)
      {
        /* clean up the client */
        if (vlanId == logicalPortInfo->client.vlanId)
        {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                               "port %s is deleted from vlan %d. logicalPort %d is authenticated on same vlan. vlan type %s"
                               "Cleaning up the client \n",
                               authmgrIntfIfNameGet(intIfNum), vlanId, lIntIfNum,
                               authmgrVlanTypeStringGet (logicalPortInfo->
                                                         client.vlanType));
          /* invoke the cleanup */
          rc = authmgrClientInfoCleanup (logicalPortInfo);
          tmpCount++;
          if ( SUCCESS != rc)
          {
            AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, intIfNum,
                                 "client cleanup for logicalPort %d is NOT successful\n",
                                 lIntIfNum);
          }
        }
      }
    }
  }


  return  SUCCESS;
}

/********************************************************************
 * @purpose  function to process vlan configuration delete event 
 *
 * @param    vlanId @b{(input)} vlan id
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanConfDeleteProcess (uint32 vlanId)
{
  RC_t rc =  SUCCESS;
  RC_t nimRc =  SUCCESS;
  uint32 intIfNum;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 lIntIfNum = 0;
  
  nimRc = authmgrFirstValidIntfNumber (&intIfNum);
  while (nimRc ==  SUCCESS)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode ==
         AUTHMGR_PORT_AUTO)
    {
      lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
      while ((logicalPortInfo =
              authmgrLogicalPortInfoGetNextNode (intIfNum,
                                                 &lIntIfNum)) !=  NULLPTR)
      {
        if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
            logicalPortInfo->client.logicalPortStatus)
        {
          /* clean up the client */
          if (vlanId == logicalPortInfo->client.vlanId)
          {
            AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                                 "vlan %d is deleted. LogicalPort %d is authenticated on same vlan. vlan type %s"
                                 "Cleaning up the client \n",
                                 vlanId, lIntIfNum,
                                 authmgrVlanTypeStringGet (logicalPortInfo->client.vlanType));
            /* invoke the cleanup */
            rc = authmgrClientInfoCleanup (logicalPortInfo);

            if ( SUCCESS != rc)
            {
              AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, intIfNum,
                                   "client cleanup for logicalPort %d is NOT successful\n",
                                   lIntIfNum);
            }
          }
        }
      }
    }
    nimRc = authmgrNextValidIntf (intIfNum, &intIfNum);
  }
  return  SUCCESS;
}

/********************************************************************
 * @purpose  function to process vlan port configuration delete event 
 *
 * @param    vlanId @b{(input)} vlan id
 * @param    intIfNum @b{(input)} interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanConfPortDeleteProcess (uint32 intIfNum, uint32 vlanId)
{
  RC_t rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 lIntIfNum = 0;
  
  if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode ==
       AUTHMGR_PORT_AUTO)
  {
    lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
    while ((logicalPortInfo =
            authmgrLogicalPortInfoGetNextNode (intIfNum,
                                               &lIntIfNum)) !=  NULLPTR)
    {
      if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
          logicalPortInfo->client.logicalPortStatus)
      {
        /* clean up the client */
        if ((vlanId == logicalPortInfo->client.vlanId) && ( TRUE == logicalPortInfo->client.vlanTypePortCfg))
        {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                               "port %s is deleted from vlan %d. LogicalPort %d is authenticated on same vlan. vlan type %s"
                               "Cleaning up the client \n",
                               authmgrIntfIfNameGet(intIfNum), vlanId, lIntIfNum,
                               authmgrVlanTypeStringGet (logicalPortInfo->client.vlanType));
          /* invoke the cleanup */
          rc = authmgrClientInfoCleanup (logicalPortInfo);

          if ( SUCCESS != rc)
          {
            AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, intIfNum,
                                 "client cleanup for logicalPort %d is NOT successful\n",
                                 lIntIfNum);
          }
        }
      }
    }
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose function to parse vlan string and perform further actions.
*
* @param vlanString -- vlan string received
* @param physPort -- interface number
* @param vlanParams -- associated vlan params
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanStringParseValidate (const  char8 * vlanString,
                                        authmgrVlanStringParams_t * vlanParams)
{
  uint32 vlanId = 0;
  RC_t rc =  FAILURE;
  /* process the string received and validate the
     result for presense of VLAN */
  if ( SUCCESS != authmgrRadiusServerVlanConversionHandle(vlanString,&vlanId))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, 0,
                         "VLAN String [%s] parsing failure\n", vlanString);
    return  ERROR;
  }

  /* If given vlan is already there */
  rc = authmgrVlanCheckValid (vlanId);
  if ( FAILURE == rc)
  {
     LOGF ( LOG_SEVERITY_WARNING,
             "Invalid VLAN %d.", vlanId);
    return  ERROR;
  }
  vlanParams->vlanId = vlanId;
  return  SUCCESS;
}
/*********************************************************************
* @purpose  Function to handle VLAN change of a particular type.
*           This function essentially cleans up all clients authorized
*           on a certain VLAN type.
* 
* @param    intIfNum   @b{(input)} internal interface number
* @param    vlanType   @b{(input)} VLAN type
*
* @returns   SUCCESS
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanTypeModifyHandle(uint32 intIfNum, authmgrVlanType_t vlanType)
{
  uint32 lIntIfNum = 0;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  logicalPortInfo = authmgrLogicalPortInfoFirstGet (intIfNum, &lIntIfNum);
  while ( NULLPTR != logicalPortInfo)
  {
    if (vlanType == logicalPortInfo->client.vlanType)
    {
      authmgrClientInfoCleanup (logicalPortInfo);
    }
    logicalPortInfo =
      authmgrLogicalPortInfoGetNextNode (intIfNum, &lIntIfNum);
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose Set Port PVID
*
* @param    intIfNum  @b{(input)) interface
* @param    pvid   @b{(input)) port default vlan id
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortPvidSet(uint32 intIfNum,  ushort16 pvid)
{
   enetMacAddr_t macAddr;
   ushort16 oldPvid = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].pvid;

  memset(&macAddr, 0, sizeof(macAddr));

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                       "port %s, old pvid = %d, new pvid = %d.",
                       authmgrIntfIfNameGet(intIfNum), 
                       oldPvid, pvid);

  if (oldPvid == pvid)
  {
    return  SUCCESS;
  }

  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].pvid = pvid;
  return  SUCCESS;
}

